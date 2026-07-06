#include "ObjectPool/ObjectPoolSubsystem.h"
#include "ObjectPool/PoolableInterface.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"

namespace
{
	FVector GetClassDefaultActorScale(TSubclassOf<AActor> ActorClass)
	{
		const AActor* ClassDefault = ActorClass ? Cast<AActor>(ActorClass->GetDefaultObject()) : nullptr;
		return ClassDefault ? ClassDefault->GetActorScale3D() : FVector::OneVector;
	}

	FTransform BuildPoolActivationTransform(TSubclassOf<AActor> ActorClass, const FTransform& SourceTransform)
	{
		FTransform Result = SourceTransform;
		Result.SetScale3D(GetClassDefaultActorScale(ActorClass));
		return Result;
	}

	void RestoreSceneComponentScalesFromClassDefaults(AActor* Actor)
	{
		if (!Actor) return;

		const AActor* ClassDefault = Actor->GetClass() ? Cast<AActor>(Actor->GetClass()->GetDefaultObject()) : nullptr;

		if (!ClassDefault) return;

		TArray<USceneComponent*> DefaultComponents;
		ClassDefault->GetComponents<USceneComponent>(DefaultComponents);

		TMap<FName, const USceneComponent*> DefaultsByName;
		for (const USceneComponent* DefaultComponent : DefaultComponents)
		{
			if (DefaultComponent) DefaultsByName.Add(DefaultComponent->GetFName(), DefaultComponent);
		}

		TArray<USceneComponent*> InstanceComponents;
		Actor->GetComponents<USceneComponent>(InstanceComponents);

		for (USceneComponent* InstanceComponent : InstanceComponents)
		{
			const USceneComponent* const* DefaultComponent = InstanceComponent ? DefaultsByName.Find(InstanceComponent->GetFName()) : nullptr;

			if (DefaultComponent && *DefaultComponent) InstanceComponent->SetRelativeScale3D((*DefaultComponent)->GetRelativeScale3D());
		}
	}
}

void UObjectPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UObjectPoolSubsystem::OnWorldCleanup);
}

void UObjectPoolSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);

	DrainAllPools();
	Super::Deinitialize();
}

AActor* UObjectPoolSubsystem::AcquireFromPool(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform)
{
	if (!ActorClass) return nullptr;

	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;

	if (!World) return nullptr;

	FObjectPool& Pool = EnsurePool(ActorClass);
	CleanupPool(Pool);

	if (Pool.InactiveActors.Num() > 0)
	{
		AActor* Actor = Pool.InactiveActors.Pop();
		Pool.ActiveActors.Add(Actor);
		ActivateActor(Actor, SpawnTransform);

		if (!IsValid(Actor))
		{
			Pool.ActiveActors.Remove(Actor);
			return nullptr;
		}

		return Actor;
	}

	AActor* Actor = CreatePooledActor(World, ActorClass, SpawnTransform);
	if (Actor)
	{
		Pool.ActiveActors.Add(Actor);
		ActivateActor(Actor, SpawnTransform);
		return Actor;
	}

	return nullptr;
}

void UObjectPoolSubsystem::WarmUpPool(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform)
{
	if (!ActorClass) return;

	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World) return;

	FObjectPool& Pool = EnsurePool(ActorClass);
	CleanupPool(Pool);

	if (!Pool.InactiveActors.IsEmpty() || !Pool.ActiveActors.IsEmpty()) return;

	AActor* Actor = CreatePooledActor(World, ActorClass, SpawnTransform);
	if (!Actor) return;

	DeactivateActor(Actor);
	Pool.InactiveActors.Add(Actor);
}

void UObjectPoolSubsystem::ReleaseToPool(AActor* Actor)
{
	if (!IsValid(Actor)) return;

	const TSubclassOf<AActor> ActorClass = Actor->GetClass();
	FObjectPool* Pool = Pools.Find(ActorClass);

	if (!Pool)
	{
		Actor->Destroy();
		return;
	}

	Pool->ActiveActors.Remove(Actor);
	DeactivateActor(Actor);
	Pool->InactiveActors.Add(Actor);

	OnActorReleased.Broadcast(Actor);
}

void UObjectPoolSubsystem::DrainAllPools()
{
	for (auto& Pair : Pools)
	{
		FObjectPool& Pool = Pair.Value;

		for (AActor* Actor : Pool.ActiveActors)
		{
			if (IsValid(Actor)) Actor->Destroy();
		}

		for (AActor* Actor : Pool.InactiveActors)
		{
			if (IsValid(Actor)) Actor->Destroy();
		}
	}

	Pools.Empty();
}

AActor* UObjectPoolSubsystem::CreatePooledActor(UWorld* World, TSubclassOf<AActor> ActorClass, const FTransform& Transform)
{
	if (!World || !ActorClass) return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const FTransform SpawnTransform = BuildPoolActivationTransform(ActorClass, Transform);
	AActor* Actor = World->SpawnActor<AActor>(ActorClass, SpawnTransform, SpawnParams);

	if (Actor) RestoreSceneComponentScalesFromClassDefaults(Actor);

	return Actor;
}

void UObjectPoolSubsystem::DeactivateActor(AActor* Actor)
{
	if (!IsValid(Actor)) return;

	Actor->SetActorHiddenInGame(true);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);

	if (ACharacter* Character = Cast<ACharacter>(Actor))
	{
		if (AAIController* AIC = Cast<AAIController>(Character->GetController()))
			AIC->StopMovement();

		
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_None);
		}

		if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
			Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
			Mesh->SetAllBodiesSimulatePhysics(false);
	}

	if (Actor->GetClass()->ImplementsInterface(UPoolableInterface::StaticClass()))
		IPoolableInterface::Execute_OnReleasedToPool(Actor);

	Actor->SetActorHiddenInGame(true);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);
}

void UObjectPoolSubsystem::ActivateActor(AActor* Actor, const FTransform& Transform)
{
	if (!IsValid(Actor)) return;

	const FTransform ActivationTransform = BuildPoolActivationTransform(Actor->GetClass(), Transform);
	Actor->SetActorTransform(ActivationTransform);
	RestoreSceneComponentScalesFromClassDefaults(Actor);
	Actor->SetActorHiddenInGame(false);
	Actor->SetActorEnableCollision(true);
	Actor->SetActorTickEnabled(true);

	if (ACharacter* Character = Cast<ACharacter>(Actor))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			Mesh->SetAllBodiesSimulatePhysics(false);
			Mesh->SetCollisionProfileName(TEXT("CharacterMesh"));
			Mesh->SetVisibility(true);

			if (ACharacter* CDO = Cast<ACharacter>(Actor->GetClass()->GetDefaultObject()))
			{
				if (USkeletalMeshComponent* CDOMesh = CDO->GetMesh())
				{
					Mesh->SetRelativeLocationAndRotation(CDOMesh->GetRelativeLocation(), CDOMesh->GetRelativeRotation());
					Mesh->SetRelativeScale3D(CDOMesh->GetRelativeScale3D());
				}
			}
		}

		if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
			Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
			Movement->SetMovementMode(MOVE_Walking);

		if (!Character->GetController()) Character->SpawnDefaultController();
	}

	if (Actor->GetClass()->ImplementsInterface(UPoolableInterface::StaticClass()))
	{
		IPoolableInterface::Execute_OnAcquiredFromPool(Actor);
	}
}

void UObjectPoolSubsystem::CleanupPool(FObjectPool& Pool)
{
	Pool.InactiveActors.RemoveAll([](const TObjectPtr<AActor>& Actor) 
	{ 
		return !IsValid(Actor); 
	});

	Pool.ActiveActors.RemoveAll([](const TObjectPtr<AActor>& Actor) 
	{ 
		return !IsValid(Actor); 
	});
}

FObjectPool& UObjectPoolSubsystem::EnsurePool(TSubclassOf<AActor> ActorClass)
{
	return Pools.FindOrAdd(ActorClass);
}

void UObjectPoolSubsystem::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (!World || World->WorldType != EWorldType::Game) return;

	Pools.Empty();
}