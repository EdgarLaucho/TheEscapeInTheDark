#include "SpawnSystem/SpawnPoint.h"
#include "SpawnSystem/SpawnBehavior.h"
#include "ObjectPool/ObjectPoolSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/BillboardComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "DrawDebugHelpers.h"

ASpawnPoint::ASpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

#if WITH_EDITORONLY_DATA
	EditorBillboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("EditorBillboard"));
	if (EditorBillboard)
	{
		EditorBillboard->SetupAttachment(Root);
		EditorBillboard->bIsScreenSizeScaled = true;

		static ConstructorHelpers::FObjectFinder<UTexture2D> IconFinder(
			TEXT("/Engine/EditorResources/S_Note"));
		if (IconFinder.Succeeded())
		{
			EditorBillboard->SetSprite(IconFinder.Object);
		}
	}
#endif
}

void ASpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UObjectPoolSubsystem* Pool = GI->GetSubsystem<UObjectPoolSubsystem>())
		{
			Pool->OnActorReleased.AddDynamic(this, &ASpawnPoint::OnPoolActorReleased);
		}
	}

	if (Behavior)
	{
		Behavior->InitializeBehavior(this);
	}

	if (bAutoActivate)
	{
		Activate();
	}
}

void ASpawnPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Deactivate();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UObjectPoolSubsystem* Pool = GI->GetSubsystem<UObjectPoolSubsystem>())
		{
			Pool->OnActorReleased.RemoveDynamic(this, &ASpawnPoint::OnPoolActorReleased);
		}
	}

	Super::EndPlay(EndPlayReason);
}

AActor* ASpawnPoint::RequestSpawn()
{
	if (!bIsActive)
	{
		return nullptr;
	}

	if (MaxSpawnCount >= 0 && CurrentSpawnCount >= MaxSpawnCount)
	{
		return nullptr;
	}

	TSubclassOf<AActor> ClassToSpawn = SelectWeightedClass();
	return RequestSpawnOfClass(ClassToSpawn);
}

AActor* ASpawnPoint::RequestSpawnOfClass(TSubclassOf<AActor> OverrideClass)
{
	if (!OverrideClass || !bIsActive)
	{
		return nullptr;
	}

	if (MaxSpawnCount >= 0 && CurrentSpawnCount >= MaxSpawnCount)
	{
		return nullptr;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return nullptr;
	}

	UObjectPoolSubsystem* PoolSubsystem = GI->GetSubsystem<UObjectPoolSubsystem>();
	if (!PoolSubsystem)
	{
		return nullptr;
	}

	AActor* Actor = PoolSubsystem->AcquireFromPool(this, OverrideClass, GetActorTransform());
	if (Actor)
	{
		SpawnedActors.Add(Actor);
		CurrentSpawnCount++;
		OnActorSpawned.Broadcast(Actor);
	}

	return Actor;
}

int32 ASpawnPoint::GetActiveSpawnCount() const
{
	int32 Count = 0;
	for (const auto& Actor : SpawnedActors)
	{
		if (IsValid(Actor))
		{
			Count++;
		}
	}
	return Count;
}

void ASpawnPoint::Activate()
{
	if (bIsActive)
	{
		return;
	}

	bIsActive = true;

	if (Behavior)
	{
		Behavior->ActivateBehavior();
	}
}

void ASpawnPoint::Deactivate()
{
	if (!bIsActive)
	{
		return;
	}

	bIsActive = false;

	if (Behavior)
	{
		Behavior->DeactivateBehavior();
	}
}

TSubclassOf<AActor> ASpawnPoint::SelectWeightedClass() const
{
	if (SpawnClasses.Num() == 0)
	{
		return nullptr;
	}

	if (SpawnClasses.Num() == 1)
	{
		return SpawnClasses[0].ActorClass;
	}

	float TotalWeight = 0.f;
	for (const auto& Entry : SpawnClasses)
	{
		TotalWeight += Entry.Weight;
	}

	if (TotalWeight <= 0.f)
	{
		return SpawnClasses[0].ActorClass;
	}

	float Random = FMath::FRandRange(0.f, TotalWeight);
	float Accumulated = 0.f;

	for (const auto& Entry : SpawnClasses)
	{
		Accumulated += Entry.Weight;
		if (Random <= Accumulated)
		{
			return Entry.ActorClass;
		}
	}

	return SpawnClasses.Last().ActorClass;
}

void ASpawnPoint::OnPoolActorReleased(AActor* Actor)
{
	if (SpawnedActors.Contains(Actor))
	{
		SpawnedActors.Remove(Actor);
		OnActorDespawned.Broadcast(Actor);

		if (Behavior)
		{
			Behavior->OnSpawnedActorReleased(Actor);
		}
	}
}
