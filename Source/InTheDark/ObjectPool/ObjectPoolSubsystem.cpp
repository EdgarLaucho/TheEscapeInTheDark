#include "ObjectPool/ObjectPoolSubsystem.h"
#include "ObjectPool/PoolableInterface.h"
#include "ObjectPool/PoolCatalog.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UObjectIterator.h"

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
		if (!Actor)
		{
			return;
		}

		const AActor* ClassDefault = Actor->GetClass() ? Cast<AActor>(Actor->GetClass()->GetDefaultObject()) : nullptr;
		if (!ClassDefault)
		{
			return;
		}

		TArray<USceneComponent*> DefaultComponents;
		ClassDefault->GetComponents<USceneComponent>(DefaultComponents);

		TMap<FName, const USceneComponent*> DefaultsByName;
		for (const USceneComponent* DefaultComponent : DefaultComponents)
		{
			if (DefaultComponent)
			{
				DefaultsByName.Add(DefaultComponent->GetFName(), DefaultComponent);
			}
		}

		TArray<USceneComponent*> InstanceComponents;
		Actor->GetComponents<USceneComponent>(InstanceComponents);
		for (USceneComponent* InstanceComponent : InstanceComponents)
		{
			const USceneComponent* const* DefaultComponent = InstanceComponent
				? DefaultsByName.Find(InstanceComponent->GetFName())
				: nullptr;
			if (DefaultComponent && *DefaultComponent)
			{
				InstanceComponent->SetRelativeScale3D((*DefaultComponent)->GetRelativeScale3D());
			}
		}
	}
}

void UObjectPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	DebugConsoleCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Pool.Debug"),
		TEXT("Toggle Object Pool debug display"),
		FConsoleCommandDelegate::CreateUObject(this, &UObjectPoolSubsystem::ToggleDebugDisplay),
		ECVF_Default
	);

	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(
		this, &UObjectPoolSubsystem::OnWorldCleanup);
}

void UObjectPoolSubsystem::Deinitialize()
{
	if (DebugTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(DebugTickerHandle);
		DebugTickerHandle.Reset();
	}

	if (DebugConsoleCommand)
	{
		IConsoleManager::Get().UnregisterConsoleObject(DebugConsoleCommand);
		DebugConsoleCommand = nullptr;
	}

	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);

	DrainAllPools();
	Super::Deinitialize();
}

AActor* UObjectPoolSubsystem::AcquireFromPool(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform)
{
	return AcquireFromPoolWithCallback(WorldContextObject, ActorClass, SpawnTransform, FOnPoolRequestFulfilled());
}

AActor* UObjectPoolSubsystem::AcquireFromPoolWithCallback(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, const FOnPoolRequestFulfilled& OnFulfilled)
{
	if (!ActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ObjectPool: AcquireFromPool called with null class"));
		return nullptr;
	}

	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("ObjectPool: No valid world context"));
		return nullptr;
	}

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
		OnActorAcquired.Broadcast(Actor);
		UE_LOG(LogTemp, Log, TEXT("[Pool] REUSE %s (inactive left: %d)"),
			*ActorClass->GetName(), Pool.InactiveActors.Num());
		return Actor;
	}

	if (CanAcquire(Pool))
	{
		AActor* Actor = CreatePooledActor(World, ActorClass, SpawnTransform);
		if (Actor)
		{
			Pool.ActiveActors.Add(Actor);
			ActivateActor(Actor, SpawnTransform);
			OnActorAcquired.Broadcast(Actor);
			UE_LOG(LogTemp, Warning, TEXT("[Pool] CREATE NEW %s (total: %d)"),
				*ActorClass->GetName(), Pool.TotalCreated);
			return Actor;
		}
	}

	if (OnFulfilled.IsBound())
	{
		FPoolRequest Request;
		Request.ActorClass = ActorClass;
		Request.SpawnTransform = SpawnTransform;
		Request.OnFulfilled = OnFulfilled;
		Pool.QueuedRequests.Add(Request);
	}

	OnPoolExhausted.Broadcast(ActorClass);
	return nullptr;
}

void UObjectPoolSubsystem::ReleaseToPool(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	TSubclassOf<AActor> ActorClass = Actor->GetClass();

	if (!Pools.Contains(ActorClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("ObjectPool: Releasing actor of class %s but no pool exists. Destroying instead."), *ActorClass->GetName());
		Actor->Destroy();
		return;
	}

	FObjectPool& Pool = Pools[ActorClass];
	Pool.ActiveActors.Remove(Actor);
	DeactivateActor(Actor);
	Pool.InactiveActors.Add(Actor);

	OnActorReleased.Broadcast(Actor);

	TryFulfillQueuedRequests(ActorClass, Actor->GetWorld());
}

void UObjectPoolSubsystem::RegisterPool(TSubclassOf<AActor> ActorClass, const FPoolSettings& Settings)
{
	if (!ActorClass)
	{
		return;
	}

	FObjectPool& Pool = Pools.FindOrAdd(ActorClass);
	Pool.Settings = Settings;
}

void UObjectPoolSubsystem::PrewarmPool(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, int32 Count)
{
	if (!ActorClass || Count <= 0)
	{
		return;
	}

	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	FObjectPool& Pool = EnsurePool(ActorClass);

	for (int32 i = 0; i < Count; ++i)
	{
		if (Pool.Settings.MaxPoolSize >= 0 && Pool.TotalCreated >= Pool.Settings.MaxPoolSize)
		{
			break;
		}

		AActor* Actor = CreatePooledActor(World, ActorClass, FTransform::Identity);
		if (Actor)
		{
			DeactivateActor(Actor);
			Pool.InactiveActors.Add(Actor);
		}
	}
}

void UObjectPoolSubsystem::LoadFromCatalog(UObject* WorldContextObject, UPoolCatalog* Catalog)
{
	if (!Catalog)
	{
		return;
	}

	for (const FPoolCatalogEntry& Entry : Catalog->Entries)
	{
		if (Entry.ActorClass)
		{
			RegisterPool(Entry.ActorClass, Entry.Settings);

			if (Entry.Settings.PrewarmCount > 0)
			{
				PrewarmPool(WorldContextObject, Entry.ActorClass, Entry.Settings.PrewarmCount);
			}
		}
	}
}

void UObjectPoolSubsystem::ReleaseAllActive(TSubclassOf<AActor> ActorClass)
{
	if (!Pools.Contains(ActorClass))
	{
		return;
	}

	FObjectPool& Pool = Pools[ActorClass];

	TArray<TObjectPtr<AActor>> ActiveCopy = Pool.ActiveActors;

	for (AActor* Actor : ActiveCopy)
	{
		if (IsValid(Actor))
		{
			ReleaseToPool(Actor);
		}
	}
}

void UObjectPoolSubsystem::DrainPool(TSubclassOf<AActor> ActorClass)
{
	if (!Pools.Contains(ActorClass))
	{
		return;
	}

	FObjectPool& Pool = Pools[ActorClass];

	for (AActor* Actor : Pool.ActiveActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}

	for (AActor* Actor : Pool.InactiveActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}

	Pool.ActiveActors.Empty();
	Pool.InactiveActors.Empty();
	Pool.QueuedRequests.Empty();
	Pool.TotalCreated = 0;

	Pools.Remove(ActorClass);
}

void UObjectPoolSubsystem::DrainAllPools()
{
	TArray<TSubclassOf<AActor>> Classes;
	Pools.GetKeys(Classes);

	for (const auto& Class : Classes)
	{
		DrainPool(Class);
	}
}

FPoolStats UObjectPoolSubsystem::GetPoolStats(TSubclassOf<AActor> ActorClass) const
{
	FPoolStats Stats;

	if (const FObjectPool* Pool = Pools.Find(ActorClass))
	{
		Stats.ActiveCount = Pool->ActiveActors.Num();
		Stats.InactiveCount = Pool->InactiveActors.Num();
		Stats.QueuedRequests = Pool->QueuedRequests.Num();
		Stats.TotalCreated = Pool->TotalCreated;
	}

	return Stats;
}

TArray<TSubclassOf<AActor>> UObjectPoolSubsystem::GetRegisteredClasses() const
{
	TArray<TSubclassOf<AActor>> Classes;
	Pools.GetKeys(Classes);
	return Classes;
}

bool UObjectPoolSubsystem::HasPool(TSubclassOf<AActor> ActorClass) const
{
	return Pools.Contains(ActorClass);
}

void UObjectPoolSubsystem::ToggleDebugDisplay()
{
	bDebugEnabled = !bDebugEnabled;

	if (bDebugEnabled)
	{
		TWeakObjectPtr<UObjectPoolSubsystem> WeakThis(this);
		DebugTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda([WeakThis](float DeltaTime) -> bool
			{
				if (UObjectPoolSubsystem* Self = WeakThis.Get())
				{
					return Self->DebugTick(DeltaTime);
				}
				return false;
			}), 0.0f);

		UE_LOG(LogTemp, Log, TEXT("ObjectPool: Debug display ENABLED"));
	}
	else
	{
		if (DebugTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(DebugTickerHandle);
			DebugTickerHandle.Reset();
		}
		UE_LOG(LogTemp, Log, TEXT("ObjectPool: Debug display DISABLED"));
	}
}

bool UObjectPoolSubsystem::DebugTick(float DeltaTime)
{
	if (!bDebugEnabled || !GEngine)
	{
		return false;
	}

	int32 Key = 50000;
	GEngine->AddOnScreenDebugMessage(Key++, 0.0f, FColor::Yellow, TEXT("========== OBJECT POOL =========="));

	if (Pools.Num() == 0)
	{
		GEngine->AddOnScreenDebugMessage(Key++, 0.0f, FColor::Silver, TEXT("  (no pools registered)"));
	}

	for (const auto& Pair : Pools)
	{
		if (!Pair.Key) continue;

		const FObjectPool& Pool = Pair.Value;
		const int32 Active = Pool.ActiveActors.Num();
		const int32 Inactive = Pool.InactiveActors.Num();
		const int32 Queued = Pool.QueuedRequests.Num();

		FString Msg = FString::Printf(TEXT("  %s | Active: %d | Ready: %d | Queued: %d | Total: %d"),
			*Pair.Key->GetName(), Active, Inactive, Queued, Pool.TotalCreated);

		FColor Color = Inactive > 0 ? FColor::Green : (Active > 0 ? FColor::Orange : FColor::Red);
		GEngine->AddOnScreenDebugMessage(Key++, 0.0f, Color, Msg);
	}

	return true;
}

AActor* UObjectPoolSubsystem::CreatePooledActor(UWorld* World, TSubclassOf<AActor> ActorClass, const FTransform& Transform)
{
	if (!World || !ActorClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const FTransform SpawnTransform = BuildPoolActivationTransform(ActorClass, Transform);
	AActor* Actor = World->SpawnActor<AActor>(ActorClass, SpawnTransform, SpawnParams);
	if (Actor)
	{
		RestoreSceneComponentScalesFromClassDefaults(Actor);
		FObjectPool& Pool = Pools.FindOrAdd(ActorClass);
		Pool.TotalCreated++;
	}

	return Actor;
}

void UObjectPoolSubsystem::DeactivateActor(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(Actor))
	{
		if (AAIController* AIC = Cast<AAIController>(Character->GetController()))
		{
			AIC->StopMovement();
		}
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_None);
		}
		if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			Mesh->SetAllBodiesSimulatePhysics(false);
		}
	}

	if (Actor->GetClass()->ImplementsInterface(UPoolableInterface::StaticClass()))
	{
		IPoolableInterface::Execute_OnReleasedToPool(Actor);
	}

	Actor->SetActorHiddenInGame(true);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);
}

void UObjectPoolSubsystem::ActivateActor(AActor* Actor, const FTransform& Transform)
{
	if (!IsValid(Actor))
	{
		return;
	}

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
					Mesh->SetRelativeLocationAndRotation(
						CDOMesh->GetRelativeLocation(),
						CDOMesh->GetRelativeRotation());
					Mesh->SetRelativeScale3D(CDOMesh->GetRelativeScale3D());
				}
			}
		}
		if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
		if (!Character->GetController())
		{
			Character->SpawnDefaultController();
		}
	}

	if (Actor->GetClass()->ImplementsInterface(UPoolableInterface::StaticClass()))
	{
		IPoolableInterface::Execute_OnAcquiredFromPool(Actor);
	}
}

void UObjectPoolSubsystem::CleanupPool(FObjectPool& Pool)
{
	Pool.InactiveActors.RemoveAll([](const TObjectPtr<AActor>& Actor) { return !IsValid(Actor); });
	Pool.ActiveActors.RemoveAll([](const TObjectPtr<AActor>& Actor) { return !IsValid(Actor); });
}

void UObjectPoolSubsystem::TryFulfillQueuedRequests(TSubclassOf<AActor> ActorClass, UWorld* World)
{
	if (!Pools.Contains(ActorClass))
	{
		return;
	}

	FObjectPool& Pool = Pools[ActorClass];

	while (Pool.QueuedRequests.Num() > 0 && Pool.InactiveActors.Num() > 0)
	{
		FPoolRequest Request = Pool.QueuedRequests[0];
		Pool.QueuedRequests.RemoveAt(0);

		AActor* Actor = Pool.InactiveActors.Pop();
		Pool.ActiveActors.Add(Actor);
		ActivateActor(Actor, Request.SpawnTransform);

		OnActorAcquired.Broadcast(Actor);

		if (Request.OnFulfilled.IsBound())
		{
			Request.OnFulfilled.Execute(Actor);
		}
	}
}

FObjectPool& UObjectPoolSubsystem::EnsurePool(TSubclassOf<AActor> ActorClass)
{
	if (!Pools.Contains(ActorClass))
	{
		FObjectPool NewPool;
		Pools.Add(ActorClass, NewPool);
	}
	return Pools[ActorClass];
}

bool UObjectPoolSubsystem::CanAcquire(const FObjectPool& Pool) const
{
	if (!Pool.Settings.bAutoExpand)
	{
		return false;
	}

	if (Pool.Settings.MaxPoolSize < 0)
	{
		return true;
	}

	return Pool.TotalCreated < Pool.Settings.MaxPoolSize;
}

void UObjectPoolSubsystem::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (!World || World->WorldType != EWorldType::Game)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("ObjectPool: world cleanup, clearing pools."));

	for (auto& Pair : Pools)
	{
		FObjectPool& Pool = Pair.Value;
		Pool.ActiveActors.Empty();
		Pool.InactiveActors.Empty();
		Pool.QueuedRequests.Empty();
		Pool.TotalCreated = 0;
	}

	Pools.Empty();
}


