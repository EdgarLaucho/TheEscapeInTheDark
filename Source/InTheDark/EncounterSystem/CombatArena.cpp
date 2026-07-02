#include "EncounterSystem/CombatArena.h"
#include "EncounterSystem/EncounterConfig.h"
#include "EncounterSystem/EncounterDirectorComponent.h"
#include "EncounterSystem/EncounterGate.h"
#include "EncounterSystem/SpawnAnchor.h"
#include "ObjectPool/ObjectPoolSubsystem.h"
#include "SaveSystem/InTheDarkGameInstance.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ACombatArena::ACombatArena()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(Root);
	TriggerVolume->SetBoxExtent(FVector(1000.f, 1000.f, 300.f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);

	Director = CreateDefaultSubobject<UEncounterDirectorComponent>(TEXT("Director"));
}

void ACombatArena::BeginPlay()
{
	Super::BeginPlay();

	if (bSkipIfAlreadyCleared && LookupIsAlreadyCleared())
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ACombatArena::UnlockGatesForClearedState);
		return;
	}

	if (TriggerVolume && bAutoStartOnOverlap)
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ACombatArena::HandleTriggerOverlap);
	}

	if (Director)
	{
		Director->OnEncounterCleared.AddDynamic(this, &ACombatArena::HandleEncounterCleared);
	}

	if (!Config)
	{
		return;
	}

	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	UObjectPoolSubsystem* Pool = GI ? GI->GetSubsystem<UObjectPoolSubsystem>() : nullptr;
	if (!Pool)
	{
		return;
	}

	TMap<UClass*, int32> MaxPerClass;
	for (const FEncounterWave& Wave : Config->Waves)
	{
		for (const FEnemySpawn& Spawn : Wave.Spawns)
		{
			if (UClass* EnemyClass = Spawn.Enemy.LoadSynchronous())
			{
				int32& Best = MaxPerClass.FindOrAdd(EnemyClass);
				Best = FMath::Max(Best, Spawn.Count);
			}
		}
	}

	for (const TPair<UClass*, int32>& Pair : MaxPerClass)
	{
		if (!Pool->HasPool(Pair.Key))
		{
			FPoolSettings Settings;
			Settings.PrewarmCount = Pair.Value;
			Settings.MaxPoolSize = Pair.Value * 3;
			Settings.bAutoExpand = true;
			Pool->RegisterPool(Pair.Key, Settings);
		}

		Pool->PrewarmPool(this, Pair.Key, Pair.Value);
	}
}

bool ACombatArena::LookupIsAlreadyCleared() const
{
	if (EncounterId.IsNone())
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(UGameplayStatics::GetGameInstance(World));
	return GI && GI->IsEncounterCleared(EncounterId);
}

void ACombatArena::HandleTriggerOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*,
	int32, bool, const FHitResult&)
{
	if (bAlreadyStartedThisSession)
	{
		return;
	}

	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const AActor* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (Other == PlayerPawn)
	{
		RequestStart();
	}
}

void ACombatArena::RequestStart()
{
	if (bAlreadyStartedThisSession || LookupIsAlreadyCleared())
	{
		return;
	}

	if (!Director || !Config)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACombatArena::RequestStart: missing Director or Config on %s"), *GetName());
		return;
	}

	bAlreadyStartedThisSession = true;
	LockEntryGates();
	Director->StartEncounter();
}

void ACombatArena::HandleEncounterCleared()
{
	if (bUnlockEntryGatesOnClear)
	{
		UnlockEntryGates();
	}

	UnlockExitGates();

	if (Config && Config->bPersistCleared && !EncounterId.IsNone())
	{
		if (UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(UGameplayStatics::GetGameInstance(GetWorld())))
		{
			GI->MarkEncounterCleared(EncounterId);
			GI->WriteSaveToDisk();
		}
	}
}

void ACombatArena::LockEntryGates()
{
	for (const TObjectPtr<AEncounterGate>& Gate : EntryGates)
	{
		if (Gate)
		{
			Gate->Lock();
		}
	}
}

void ACombatArena::UnlockEntryGates()
{
	for (const TObjectPtr<AEncounterGate>& Gate : EntryGates)
	{
		if (Gate)
		{
			Gate->Unlock();
		}
	}
}

void ACombatArena::UnlockExitGates()
{
	for (const TObjectPtr<AEncounterGate>& Gate : ExitGates)
	{
		if (Gate)
		{
			Gate->Unlock();
		}
	}
}

void ACombatArena::UnlockGatesForClearedState()
{
	if (bUnlockEntryGatesOnClear)
	{
		UnlockEntryGates();
	}
	else
	{
		LockEntryGates();
	}

	UnlockExitGates();
}