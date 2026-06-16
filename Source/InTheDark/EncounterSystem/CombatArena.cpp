#include "EncounterSystem/CombatArena.h"
#include "EncounterSystem/EncounterConfig.h"
#include "EncounterSystem/EncounterDirectorComponent.h"
#include "EncounterSystem/EncounterGate.h"
#include "EncounterSystem/SpawnAnchor.h"
#include "SaveSystem/InTheDarkGameInstance.h"
#include "ObjectPool/ObjectPoolSubsystem.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
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

	RewardAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("RewardAnchor"));
	RewardAnchor->SetupAttachment(Root);

	Director = CreateDefaultSubobject<UEncounterDirectorComponent>(TEXT("Director"));
}

void ACombatArena::BeginPlay()
{
	Super::BeginPlay();

	if (bSkipIfAlreadyCleared && LookupIsAlreadyCleared())
	{
		UnlockAllGates();
		UE_LOG(LogTemp, Log, TEXT("CombatArena '%s' (%s): already cleared; skipping."),
			*GetName(), *EncounterId.ToString());
		return;
	}

	if (TriggerVolume && bAutoStartOnOverlap)
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ACombatArena::HandleTriggerOverlap);
	}

	if (Director)
	{
		Director->OnEncounterStarted.AddDynamic(this, &ACombatArena::HandleEncounterStarted);
		Director->OnEncounterCleared.AddDynamic(this, &ACombatArena::HandleEncounterCleared);
		Director->OnEncounterFailed.AddDynamic(this, &ACombatArena::HandleEncounterFailed);
	}

	// Precalienta el pool con cada clase de enemigo de todas las oleadas.
	if (Config)
	{
		if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
		{
			if (UObjectPoolSubsystem* Pool = GI->GetSubsystem<UObjectPoolSubsystem>())
			{
				TMap<UClass*, int32> MaxPerClass;
				for (const FEncounterWave& Wave : Config->Waves)
				{
					for (const FEnemySpawn& Spawn : Wave.Spawns)
					{
						if (Spawn.Enemy.IsNull())
						{
							UE_LOG(LogTemp, Warning, TEXT("[CombatArena '%s'] Wave '%s' has a Spawn entry with NULL Enemy class"),
								*GetName(), *Wave.WaveName.ToString());
							continue;
						}
						UClass* Cls = Spawn.Enemy.LoadSynchronous();
						if (!Cls)
						{
							UE_LOG(LogTemp, Warning, TEXT("[CombatArena '%s'] Wave '%s' failed to load enemy class from soft ptr '%s'"),
								*GetName(), *Wave.WaveName.ToString(), *Spawn.Enemy.ToString());
							continue;
						}
						int32& Best = MaxPerClass.FindOrAdd(Cls);
						Best = FMath::Max(Best, Spawn.Count);
						UE_LOG(LogTemp, Log, TEXT("[CombatArena '%s'] Wave '%s' declares %d x %s (running max=%d)"),
							*GetName(), *Wave.WaveName.ToString(), Spawn.Count, *Cls->GetName(), Best);
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
					UE_LOG(LogTemp, Log, TEXT("CombatArena '%s': prewarmed %d x %s"),
						*GetName(), Pair.Value, *Pair.Key->GetName());
				}
			}
		}
	}
}

bool ACombatArena::LookupIsAlreadyCleared() const
{
	if (EncounterId.IsNone()) { return false; }
	const UWorld* World = GetWorld();
	if (!World) { return false; }
	UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(UGameplayStatics::GetGameInstance(World));
	return GI && GI->IsEncounterCleared(EncounterId);
}

void ACombatArena::HandleTriggerOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*,
	int32, bool, const FHitResult&)
{
	if (bAlreadyStartedThisSession) { return; }
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const AActor* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (!PlayerPawn || Other != PlayerPawn) { return; }

	RequestStart();
}

void ACombatArena::RequestStart()
{
	if (bAlreadyStartedThisSession) { return; }
	if (!Director || !Config)
	{
		UE_LOG(LogTemp, Warning, TEXT("ACombatArena::RequestStart: missing Director or Config on %s"),
			*GetName());
		return;
	}
	if (LookupIsAlreadyCleared()) { return; }

	bAlreadyStartedThisSession = true;
	LockAllGates();
	Director->StartEncounter();
}

void ACombatArena::HandleEncounterStarted()
{
	// Reservado para listeners de Blueprint / en el futuro para musica, efectos, dialogos, etc. Intencionalmente vacío en C++.
}

void ACombatArena::HandleEncounterCleared()
{
	UnlockAllGates();
	SpawnReward();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green,
			FString::Printf(TEXT("ENCUENTRO COMPLETADO: %s"), *EncounterId.ToString()));
	}

	if (Config && Config->bPersistCleared && !EncounterId.IsNone())
	{
		if (UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(
				UGameplayStatics::GetGameInstance(GetWorld())))
		{
			GI->MarkEncounterCleared(EncounterId);
			GI->WriteSaveToDisk();
		}
	}
}

void ACombatArena::HandleEncounterFailed()
{
	UnlockAllGates();
	if (Config && Config->bReloadCheckpointOnFailure)
	{
		if (UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(
				UGameplayStatics::GetGameInstance(GetWorld())))
		{
			// De momento recarga de nivel. En futuro carga de checkpoint concreto.
			GI->LoadOrCreateSave();
			UGameplayStatics::OpenLevel(GetWorld(), FName(*UGameplayStatics::GetCurrentLevelName(GetWorld(), true)));
		}
	}
}

void ACombatArena::SpawnReward()
{
	if (!Config) { return; }
	if (Config->Reward.RewardClass.IsNull() || !RewardAnchor) { return; }

	UClass* Cls = Config->Reward.RewardClass.LoadSynchronous();
	if (!Cls) { return; }

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	GetWorld()->SpawnActor<AActor>(Cls, RewardAnchor->GetComponentTransform(), P);
}

void ACombatArena::LockAllGates()
{
	for (const TObjectPtr<AEncounterGate>& G : Gates)
	{
		if (G) { G->Lock(); }
	}
}

void ACombatArena::UnlockAllGates()
{
	for (const TObjectPtr<AEncounterGate>& G : Gates)
	{
		if (G) { G->Unlock(); }
	}
}

int32 ACombatArena::AutoBindAnchorsByTag(FGameplayTagContainer Filter)
{
	Anchors.Empty();
	UWorld* World = GetWorld();
	if (!World) { return 0; }

	for (TActorIterator<ASpawnAnchor> It(World); It; ++It)
	{
		ASpawnAnchor* Anchor = *It;
		if (!Anchor) { continue; }

		if (Filter.IsEmpty() || Anchor->AnchorTags.HasAny(Filter))
		{
			Anchors.AddUnique(Anchor);
		}
	}

	Modify();
	MarkPackageDirty();
	UE_LOG(LogTemp, Log, TEXT("ACombatArena::AutoBindAnchorsByTag bound %d anchors on %s"),
		Anchors.Num(), *GetName());
	return Anchors.Num();
}
