#include "EncounterSystem/EncounterDirectorComponent.h"
#include "EncounterSystem/CombatArena.h"
#include "EncounterSystem/EncounterConfig.h"
#include "EncounterSystem/SpawnAnchor.h"
#include "ObjectPool/ObjectPoolSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

UEncounterDirectorComponent::UEncounterDirectorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetAutoActivate(true);
}

void UEncounterDirectorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (State != EEncounterState::WaveActive)
	{
		return;
	}

	WaveElapsed += DeltaTime;
	EvaluateWaveContinuation();
}

ACombatArena* UEncounterDirectorComponent::GetArena() const
{
	return Cast<ACombatArena>(GetOwner());
}

UEncounterConfig* UEncounterDirectorComponent::GetConfig() const
{
	const ACombatArena* Arena = GetArena();
	return Arena ? Arena->Config : nullptr;
}

const FEncounterWave* UEncounterDirectorComponent::GetCurrentWave() const
{
	const UEncounterConfig* Cfg = GetConfig();
	if (!Cfg || !Cfg->Waves.IsValidIndex(CurrentWaveIndex)) { return nullptr; }
	return &Cfg->Waves[CurrentWaveIndex];
}

AActor* UEncounterDirectorComponent::GetPlayerActor() const
{
	const UWorld* World = GetWorld();
	if (!World) { return nullptr; }
	APlayerController* PC = World->GetFirstPlayerController();
	return PC ? PC->GetPawn() : nullptr;
}

UObjectPoolSubsystem* UEncounterDirectorComponent::GetPool() const
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		return GI->GetSubsystem<UObjectPoolSubsystem>();
	}
	return nullptr;
}

int32 UEncounterDirectorComponent::GetAliveEnemyCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& E : AliveEnemies)
	{
		if (E.IsValid() && !E->IsActorBeingDestroyed()) { ++Count; }
	}
	return Count;
}

void UEncounterDirectorComponent::StartEncounter()
{
	if (State != EEncounterState::Idle)
	{
		return;
	}
	const UEncounterConfig* Cfg = GetConfig();
	if (!Cfg || Cfg->Waves.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterDirector: no config or no waves on %s"),
			*GetOwner()->GetName());
		return;
	}

	State = EEncounterState::Starting;
	CurrentWaveIndex = -1;
	AliveEnemies.Reset();
	PendingSpawns.Reset();
	WaveElapsed = 0.f;

	if (UObjectPoolSubsystem* Pool = GetPool())
	{
		Pool->OnActorReleased.AddUniqueDynamic(this, &UEncounterDirectorComponent::HandleEnemyReleasedToPool);
	}

	SetComponentTickEnabled(true);

	BeginNextWave();
}

void UEncounterDirectorComponent::BeginNextWave()
{
	const UEncounterConfig* Cfg = GetConfig();
	if (!Cfg) { return; }

	CurrentWaveIndex += 1;
	if (!Cfg->Waves.IsValidIndex(CurrentWaveIndex))
	{
		HandleEncounterCleared();
		return;
	}

	const FEncounterWave& Wave = Cfg->Waves[CurrentWaveIndex];
	if (Wave.DelayBeforeWave > 0.f)
	{
		State = EEncounterState::Starting;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(DelayTimerHandle, this,
				&UEncounterDirectorComponent::BeginWaveActuallyNow,
				Wave.DelayBeforeWave, false);
		}
	}
	else
	{
		BeginWaveActuallyNow();
	}
}

void UEncounterDirectorComponent::BeginWaveActuallyNow()
{
	const FEncounterWave* Wave = GetCurrentWave();
	if (!Wave) { return; }

	State = EEncounterState::WaveActive;
	WaveElapsed = 0.f;

	SpawnDirectives(*Wave);

	if (AliveEnemies.Num() == 0 && PendingSpawns.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterDirector: wave %d produced 0 enemies; treating as cleared."),
			CurrentWaveIndex);
		HandleWaveCleared();
	}
}

TArray<ASpawnAnchor*> UEncounterDirectorComponent::GetAvailableAnchors(
	TSubclassOf<AActor> EnemyClass,
	const TSet<ASpawnAnchor*>* ReservedAnchors) const
{
	TArray<ASpawnAnchor*> Out;
	const ACombatArena* Arena = GetArena();
	if (!Arena) { return Out; }

	AActor* Player = GetPlayerActor();

	for (const TObjectPtr<ASpawnAnchor>& A : Arena->Anchors)
	{
		if (!A) { continue; }
		if (ReservedAnchors && ReservedAnchors->Contains(A.Get()))
		{
			continue;
		}
		if (!A->IsAvailableForSpawn(Player))
		{
			continue;
		}
		if (EnemyClass && A->IsSpawnLocationOccupied(EnemyClass))
		{
			continue;
		}
		Out.Add(A);
	}
	return Out;
}

void UEncounterDirectorComponent::SpawnDirectives(const FEncounterWave& Wave)
{
	TSet<ASpawnAnchor*> ReservedAnchors;

	for (const FEnemySpawn& Directive : Wave.Spawns)
	{
		SpawnDirective(Directive, ReservedAnchors);
	}
}

void UEncounterDirectorComponent::SpawnDirective(const FEnemySpawn& Directive, TSet<ASpawnAnchor*>& ReservedAnchors)
{
	if (Directive.Enemy.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterDirector: spawn directive has null Enemy class."));
		return;
	}

	UClass* EnemyClass = Directive.Enemy.LoadSynchronous();
	if (!EnemyClass)
	{
		return;
	}

	for (int32 i = 0; i < FMath::Max(1, Directive.Count); ++i)
	{
		ASpawnAnchor* Anchor = ChooseAnchorForSpawn(EnemyClass, ReservedAnchors);
		if (!Anchor)
		{
			UE_LOG(LogTemp, Warning, TEXT("EncounterDirector: no free anchors for %s."), *EnemyClass->GetName());
			continue;
		}

		const float Lead = FMath::Max(0.f, Directive.PreSpawnLead);
		if (Lead > 0.f)
		{
			ScheduleSpawn(EnemyClass, Anchor, Lead);
		}
		else
		{
			TrackSpawnedEnemy(Anchor->PerformSpawn(EnemyClass));
		}
	}
}

ASpawnAnchor* UEncounterDirectorComponent::ChooseAnchorForSpawn(TSubclassOf<AActor> EnemyClass, TSet<ASpawnAnchor*>& ReservedAnchors) const
{
	TArray<ASpawnAnchor*> Candidates = GetAvailableAnchors(EnemyClass, &ReservedAnchors);
	if (Candidates.IsEmpty())
	{
		const ACombatArena* Arena = GetArena();
		if (Arena)
		{
			for (const TObjectPtr<ASpawnAnchor>& Anchor : Arena->Anchors)
			{
				if (!Anchor) { continue; }
				if (ReservedAnchors.Contains(Anchor.Get())) { continue; }
				if (Anchor->IsSpawnLocationOccupied(EnemyClass)) { continue; }
				Candidates.Add(Anchor);
			}
		}
	}

	if (Candidates.IsEmpty())
	{
		return nullptr;
	}

	ASpawnAnchor* Picked = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	ReservedAnchors.Add(Picked);
	return Picked;
}

void UEncounterDirectorComponent::ScheduleSpawn(TSubclassOf<AActor> EnemyClass, ASpawnAnchor* Anchor, float LeadSeconds)
{
	UWorld* World = GetWorld();
	if (!World || !Anchor || !EnemyClass)
	{
		return;
	}

	const int32 PendingIndex = PendingSpawns.AddDefaulted();
	FPendingSpawn& Pending = PendingSpawns[PendingIndex];
	Pending.EnemyClass = EnemyClass;
	Pending.Anchor = Anchor;

	FTimerDelegate TimerDel;
	TimerDel.BindUObject(this, &UEncounterDirectorComponent::ExecutePendingSpawn, PendingIndex);
	World->GetTimerManager().SetTimer(Pending.TimerHandle, TimerDel, LeadSeconds, false);
}

void UEncounterDirectorComponent::ExecutePendingSpawn(int32 PendingIndex)
{
	if (!PendingSpawns.IsValidIndex(PendingIndex)) { return; }

	FPendingSpawn& Pending = PendingSpawns[PendingIndex];
	if (!TryPreparePendingSpawn(Pending, PendingIndex))
	{
		return;
	}

	AActor* Spawned = Pending.Anchor->PerformSpawn(Pending.EnemyClass);
	TrackSpawnedEnemy(Spawned);

	if (State == EEncounterState::WaveActive && AliveEnemies.Num() == 0 && !HasActivePendingSpawns())
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterDirector: all deferred spawns complete but 0 enemies alive; treating as cleared."));
		HandleWaveCleared();
	}
}

bool UEncounterDirectorComponent::TryPreparePendingSpawn(FPendingSpawn& Pending, int32 PendingIndex)
{
	if (!Pending.Anchor.IsValid() || !Pending.EnemyClass)
	{
		return false;
	}

	if (!Pending.Anchor->IsSpawnLocationOccupied(Pending.EnemyClass))
	{
		return true;
	}

	TArray<ASpawnAnchor*> Candidates = GetAvailableAnchors(Pending.EnemyClass);
	if (!Candidates.IsEmpty())
	{
		Pending.Anchor = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
		return true;
	}

	if (Pending.RetryCount < 10)
	{
		++Pending.RetryCount;
		if (UWorld* World = GetWorld())
		{
			FTimerDelegate TimerDel;
			TimerDel.BindUObject(this, &UEncounterDirectorComponent::ExecutePendingSpawn, PendingIndex);
			World->GetTimerManager().SetTimer(Pending.TimerHandle, TimerDel, 0.35f, false);
		}
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("EncounterDirector: skipping spawn for %s; no free anchor after retries."),
		*Pending.EnemyClass->GetName());
	return false;
}

void UEncounterDirectorComponent::CancelPendingSpawns()
{
	if (UWorld* World = GetWorld())
	{
		for (FPendingSpawn& P : PendingSpawns)
		{
			World->GetTimerManager().ClearTimer(P.TimerHandle);
		}
	}
	PendingSpawns.Reset();
}

void UEncounterDirectorComponent::TrackSpawnedEnemy(AActor* Enemy)
{
	if (!Enemy) { return; }
	AliveEnemies.Add(Enemy);
	Enemy->OnDestroyed.AddDynamic(this, &UEncounterDirectorComponent::HandleEnemyDestroyed);
}

void UEncounterDirectorComponent::ReleaseAliveEnemiesToPool()
{
	UObjectPoolSubsystem* Pool = GetPool();
	for (const TWeakObjectPtr<AActor>& E : AliveEnemies)
	{
		if (!E.IsValid()) { continue; }
		if (Pool && Pool->HasPool(E->GetClass()))
		{
			Pool->ReleaseToPool(E.Get());
		}
		else
		{
			E->Destroy();
		}
	}
}

void UEncounterDirectorComponent::HandleEnemyDestroyed(AActor* DestroyedActor)
{
	AliveEnemies.RemoveAll([DestroyedActor](const TWeakObjectPtr<AActor>& E) {
		return !E.IsValid() || E.Get() == DestroyedActor;
	});
	if (State == EEncounterState::WaveActive)
	{
		EvaluateWaveContinuation();
	}
}

void UEncounterDirectorComponent::HandleEnemyReleasedToPool(AActor* ReleasedActor)
{
	const int32 Removed = AliveEnemies.RemoveAll([ReleasedActor](const TWeakObjectPtr<AActor>& E) {
		return E.Get() == ReleasedActor;
	});
	if (Removed > 0 && State == EEncounterState::WaveActive)
	{
		EvaluateWaveContinuation();
	}
}

bool UEncounterDirectorComponent::HasActivePendingSpawns() const
{
	const UWorld* World = GetWorld();
	if (!World) { return false; }
	for (const FPendingSpawn& P : PendingSpawns)
	{
		if (World->GetTimerManager().IsTimerActive(P.TimerHandle)) { return true; }
	}
	return false;
}

bool UEncounterDirectorComponent::ShouldAdvanceWave(const FWaveContinuation& Rule, int32 Remaining) const
{
	switch (Rule.Mode)
	{
	case EWaveContinuationMode::OnAllCleared:
		return Remaining == 0;
	case EWaveContinuationMode::OnRemainingAtOrBelow:
		return Remaining <= Rule.RemainingThreshold;
	case EWaveContinuationMode::OnElapsedSince:
		return WaveElapsed >= Rule.ElapsedSeconds;
	case EWaveContinuationMode::Hybrid:
		return Remaining <= Rule.RemainingThreshold || WaveElapsed >= Rule.ElapsedSeconds;
	}
	return false;
}

void UEncounterDirectorComponent::EvaluateWaveContinuation()
{
	const FEncounterWave* Wave = GetCurrentWave();
	if (!Wave || HasActivePendingSpawns())
	{
		return;
	}

	if (ShouldAdvanceWave(Wave->Continuation, GetAliveEnemyCount()))
	{
		HandleWaveCleared();
	}
}

void UEncounterDirectorComponent::HandleWaveCleared()
{
	if (State != EEncounterState::WaveActive) { return; }

	State = EEncounterState::PostClear;

	const UEncounterConfig* Cfg = GetConfig();
	const bool bLastWave = Cfg && !Cfg->Waves.IsValidIndex(CurrentWaveIndex + 1);

	if (bLastWave)
	{
		if (Cfg && Cfg->PostClearBeatSeconds > 0.f && GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(PostClearTimerHandle, this,
				&UEncounterDirectorComponent::HandleEncounterCleared,
				Cfg->PostClearBeatSeconds, false);
		}
		else
		{
			HandleEncounterCleared();
		}
	}
	else
	{
		BeginNextWave();
	}
}

void UEncounterDirectorComponent::HandleEncounterCleared()
{
	if (State == EEncounterState::Cleared) { return; }
	State = EEncounterState::Cleared;
	SetComponentTickEnabled(false);
	CancelPendingSpawns();
	ReleaseAliveEnemiesToPool();
	AliveEnemies.Reset();

	if (UObjectPoolSubsystem* Pool = GetPool())
	{
		Pool->OnActorReleased.RemoveDynamic(this, &UEncounterDirectorComponent::HandleEnemyReleasedToPool);
	}

	if (ACombatArena* Arena = GetArena())
	{
		Arena->NotifyEncounterCleared();
	}
	OnEncounterCleared.Broadcast();
}
