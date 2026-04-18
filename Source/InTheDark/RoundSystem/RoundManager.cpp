#include "RoundSystem/RoundManager.h"
#include "RoundSystem/RoundConfig.h"
#include "RoundSystem/RoundProgressSubsystem.h"
#include "SpawnSystem/SpawnGroup.h"
#include "SpawnSystem/SpawnPoint.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

ARoundManager::ARoundManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void ARoundManager::BeginPlay()
{
	Super::BeginPlay();

	if (bShowDebugOverlay)
	{
		SetActorTickEnabled(true);
	}

	if (bSkipIfAlreadyCompleted && !ArenaId.IsNone())
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (URoundProgressSubsystem* Progress = GI->GetSubsystem<URoundProgressSubsystem>())
			{
				if (Progress->IsCompleted(ArenaId))
				{
					UE_LOG(LogTemp, Log, TEXT("ARoundManager '%s': Arena '%s' already completed — skipping."),
						*GetName(), *ArenaId.ToString());
					State = ERoundState::Completed;
					OnRoundCompleted.Broadcast();
					return;
				}
			}
		}
	}

	if (bAutoStart)
	{
		StartRound();
	}
}

void ARoundManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindGroupDelegates(ActiveBoundGroup);

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TM = World->GetTimerManager();
		TM.ClearTimer(DelayTimerHandle);
		TM.ClearTimer(WatchdogTimerHandle);
		TM.ClearTimer(CountdownTickHandle);
	}

	if (BoundPlayer.IsValid())
	{
		BoundPlayer->OnDestroyed.RemoveDynamic(this, &ARoundManager::HandlePlayerDestroyed);
		BoundPlayer.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void ARoundManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bShowDebugOverlay)
	{
		DrawDebugOverlay();
	}
}

int32 ARoundManager::GetWaveCount() const
{
	return Config ? Config->Waves.Num() : 0;
}

void ARoundManager::BindGroupDelegates(ASpawnGroup* Group)
{
	if (!Group) return;
	Group->OnGroupSpawnComplete.AddUniqueDynamic(this, &ARoundManager::HandleGroupSpawnComplete);
	Group->OnGroupCleared.AddUniqueDynamic(this, &ARoundManager::HandleGroupCleared);
}

void ARoundManager::UnbindGroupDelegates(ASpawnGroup* Group)
{
	if (!Group) return;
	Group->OnGroupSpawnComplete.RemoveDynamic(this, &ARoundManager::HandleGroupSpawnComplete);
	Group->OnGroupCleared.RemoveDynamic(this, &ARoundManager::HandleGroupCleared);
}

ASpawnGroup* ARoundManager::ResolveSpawnGroupForWave(int32 WaveIndex) const
{
	for (const FWaveSpawnGroupOverride& Ovr : WaveSpawnGroupOverrides)
	{
		if (Ovr.WaveIndex == WaveIndex && IsValid(Ovr.SpawnGroup))
		{
			return Ovr.SpawnGroup;
		}
	}
	return SpawnGroup;
}

int32 ARoundManager::ResolveSpawnCountForWave(const FRoundWave& Wave) const
{
	if (Wave.bSpawnFromAllPoints) return 0;
	const float Mult = Config ? Config->DifficultyMultiplier : 1.0f;
	return FMath::Max(1, FMath::RoundToInt(Wave.SpawnCount * Mult));
}

void ARoundManager::TryBindPlayerDeath()
{
	if (!bFailOnPlayerDeath) return;
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn) return;
	BoundPlayer = Pawn;
	Pawn->OnDestroyed.AddUniqueDynamic(this, &ARoundManager::HandlePlayerDestroyed);
}

void ARoundManager::HandlePlayerDestroyed(AActor* DestroyedActor)
{
	if (!IsRoundActive()) return;
	FailRound(TEXT("PlayerDeath"));
}

void ARoundManager::StartRound()
{
	if (!Config || Config->Waves.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ARoundManager '%s': StartRound — Config null or empty Waves."), *GetName());
		return;
	}
	if (!SpawnGroup)
	{
		UE_LOG(LogTemp, Warning, TEXT("ARoundManager '%s': StartRound — SpawnGroup is null."), *GetName());
		return;
	}
	if (IsRoundActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("ARoundManager '%s': StartRound — already active."), *GetName());
		return;
	}

	CurrentWaveIndex = INDEX_NONE;
	OnRoundStarted.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("ARoundManager '%s': Round started (%d waves, x%.2f difficulty)."),
		*GetName(), Config->Waves.Num(), Config->DifficultyMultiplier);

	TryBindPlayerDeath();

	BeginNextWave();
}

void ARoundManager::StopRound()
{
	if (State == ERoundState::Idle) return;

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TM = World->GetTimerManager();
		TM.ClearTimer(DelayTimerHandle);
		TM.ClearTimer(WatchdogTimerHandle);
		TM.ClearTimer(CountdownTickHandle);
	}

	if (ActiveBoundGroup)
	{
		ActiveBoundGroup->DeactivateAll();
		UnbindGroupDelegates(ActiveBoundGroup);
		ActiveBoundGroup = nullptr;
	}

	State = ERoundState::Idle;
	CurrentWaveIndex = INDEX_NONE;
	OnRoundStopped.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("ARoundManager '%s': Round stopped."), *GetName());
}

void ARoundManager::PauseRound()
{
	if (!IsRoundActive() || State == ERoundState::Paused) return;

	PreviousStateBeforePause = State;

	UWorld* World = GetWorld();
	if (!World) return;
	FTimerManager& TM = World->GetTimerManager();

	if (DelayTimerHandle.IsValid()) TM.PauseTimer(DelayTimerHandle);
	if (WatchdogTimerHandle.IsValid()) TM.PauseTimer(WatchdogTimerHandle);
	if (CountdownTickHandle.IsValid()) TM.PauseTimer(CountdownTickHandle);

	State = ERoundState::Paused;
	OnRoundPaused.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("ARoundManager '%s': Paused (was %d)."), *GetName(), (int32)PreviousStateBeforePause);
}

void ARoundManager::ResumeRound()
{
	if (State != ERoundState::Paused) return;

	UWorld* World = GetWorld();
	if (!World) return;
	FTimerManager& TM = World->GetTimerManager();

	if (DelayTimerHandle.IsValid()) TM.UnPauseTimer(DelayTimerHandle);
	if (WatchdogTimerHandle.IsValid()) TM.UnPauseTimer(WatchdogTimerHandle);
	if (CountdownTickHandle.IsValid()) TM.UnPauseTimer(CountdownTickHandle);

	State = PreviousStateBeforePause;
	OnRoundResumed.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("ARoundManager '%s': Resumed (now %d)."), *GetName(), (int32)State);
}

void ARoundManager::FailRound(FName Reason)
{
	if (!IsRoundActive() && State != ERoundState::Paused) return;

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TM = World->GetTimerManager();
		TM.ClearTimer(DelayTimerHandle);
		TM.ClearTimer(WatchdogTimerHandle);
		TM.ClearTimer(CountdownTickHandle);
	}

	if (ActiveBoundGroup)
	{
		ActiveBoundGroup->DeactivateAll();
		UnbindGroupDelegates(ActiveBoundGroup);
		ActiveBoundGroup = nullptr;
	}

	State = ERoundState::Failed;
	OnRoundFailed.Broadcast(Reason);
	UE_LOG(LogTemp, Warning, TEXT("ARoundManager '%s': Round FAILED — %s"), *GetName(), *Reason.ToString());
}

void ARoundManager::BeginNextWave()
{
	const int32 NextIndex = CurrentWaveIndex + 1;
	if (!Config || !Config->Waves.IsValidIndex(NextIndex))
	{

		State = ERoundState::Completed;
		OnRoundCompleted.Broadcast();
		UE_LOG(LogTemp, Log, TEXT("ARoundManager '%s': Round COMPLETED."), *GetName());

		if (!ArenaId.IsNone())
		{
			if (UGameInstance* GI = GetGameInstance())
			{
				if (URoundProgressSubsystem* Progress = GI->GetSubsystem<URoundProgressSubsystem>())
				{
					Progress->MarkCompleted(ArenaId);
				}
			}
		}

		if (Config && Config->bLoop)
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimerForNextTick([this]() { StartRound(); });
			}
		}
		return;
	}

	CurrentWaveIndex = NextIndex;
	const FRoundWave& Wave = Config->Waves[CurrentWaveIndex];

	State = ERoundState::WaitingDelay;
	StateEnteredTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	CurrentCountdownRemaining = Wave.DelayBeforeWave;

	UE_LOG(LogTemp, Log, TEXT("ARoundManager '%s': Wave %d ('%s') queued — pre-wave countdown %.2fs."),
		*GetName(), CurrentWaveIndex, *Wave.WaveName.ToString(), Wave.DelayBeforeWave);

	OnPreWaveStarted.Broadcast(CurrentWaveIndex, Wave.DelayBeforeWave);

	UWorld* World = GetWorld();
	if (!World) return;

	if (Wave.DelayBeforeWave <= KINDA_SMALL_NUMBER)
	{
		StartCurrentWaveSpawn();
		return;
	}

	World->GetTimerManager().SetTimer(
		CountdownTickHandle, this, &ARoundManager::OnPreWaveCountdownTick,
		0.25f, true, 0.25f);

	World->GetTimerManager().SetTimer(
		DelayTimerHandle, this, &ARoundManager::StartCurrentWaveSpawn,
		Wave.DelayBeforeWave, false);
}

void ARoundManager::OnPreWaveCountdownTick()
{
	CurrentCountdownRemaining = FMath::Max(0.f, CurrentCountdownRemaining - 0.25f);
	OnPreWaveTick.Broadcast(CurrentWaveIndex, CurrentCountdownRemaining);
	if (CurrentCountdownRemaining <= 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(CountdownTickHandle);
		}
	}
}

void ARoundManager::StartCurrentWaveSpawn()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CountdownTickHandle);
	}

	if (!Config || !Config->Waves.IsValidIndex(CurrentWaveIndex)) return;

	const FRoundWave& Wave = Config->Waves[CurrentWaveIndex];

	ASpawnGroup* GroupForWave = ResolveSpawnGroupForWave(CurrentWaveIndex);
	if (!GroupForWave)
	{
		UE_LOG(LogTemp, Warning, TEXT("ARoundManager '%s': Wave %d has no SpawnGroup. Aborting wave."),
			*GetName(), CurrentWaveIndex);
		BeginNextWave();
		return;
	}

	if (ActiveBoundGroup != GroupForWave)
	{
		UnbindGroupDelegates(ActiveBoundGroup);
		ActiveBoundGroup = GroupForWave;
		BindGroupDelegates(ActiveBoundGroup);
	}

	GroupForWave->ActivateAll();

	State = ERoundState::Spawning;
	StateEnteredTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	OnWaveStarted.Broadcast(CurrentWaveIndex);

	TArray<TSubclassOf<AActor>> ResolvedFilter;
	for (const TSoftClassPtr<AActor>& SoftClass : Wave.ClassFilter)
	{
		if (UClass* C = SoftClass.LoadSynchronous())
		{
			ResolvedFilter.Add(C);
		}
	}

	const int32 EffectiveCount = ResolveSpawnCountForWave(Wave);

	UE_LOG(LogTemp, Log, TEXT("ARoundManager '%s': Wave %d started ('%s'). FromAll=%d Count=%d Interval=%.2fs Filter=%d Group=%s"),
		*GetName(), CurrentWaveIndex, *Wave.WaveName.ToString(),
		Wave.bSpawnFromAllPoints ? 1 : 0, EffectiveCount, Wave.SpawnInterval,
		ResolvedFilter.Num(), *GroupForWave->GetName());

	if (Wave.bSpawnFromAllPoints)
	{
		if (ResolvedFilter.Num() > 0)
			GroupForWave->SpawnFromAllFiltered(Wave.SpawnInterval, ResolvedFilter);
		else
			GroupForWave->SpawnFromAll(Wave.SpawnInterval);
	}
	else
	{
		if (ResolvedFilter.Num() > 0)
			GroupForWave->SpawnFromRandomFiltered(EffectiveCount, Wave.SpawnInterval, ResolvedFilter);
		else
			GroupForWave->SpawnFromRandom(EffectiveCount, Wave.SpawnInterval);
	}

	if (Wave.MaxWaveDurationSeconds > KINDA_SMALL_NUMBER)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				WatchdogTimerHandle, this, &ARoundManager::OnWatchdogElapsed,
				Wave.MaxWaveDurationSeconds, false);
		}
	}
}

void ARoundManager::OnWatchdogElapsed()
{
	if (State != ERoundState::Spawning && State != ERoundState::WaitingClear) return;

	UE_LOG(LogTemp, Warning,
		TEXT("ARoundManager '%s': WATCHDOG — Wave %d exceeded MaxWaveDurationSeconds. State=%d ActiveCount=%d. Logging only."),
		*GetName(), CurrentWaveIndex, (int32)State,
		ActiveBoundGroup ? ActiveBoundGroup->GetActiveCount() : -1);
}

void ARoundManager::HandleGroupSpawnComplete(int32 TotalSpawned)
{
	if (State != ERoundState::Spawning) return;

	State = ERoundState::WaitingClear;
	StateEnteredTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	LastWaveSpawnedCount = TotalSpawned;

	UE_LOG(LogTemp, Log, TEXT("ARoundManager '%s': Wave %d spawn complete (%d enemies). Waiting for clear."),
		*GetName(), CurrentWaveIndex, TotalSpawned);

	if (TotalSpawned == 0 && ActiveBoundGroup && ActiveBoundGroup->GetActiveCount() == 0)
	{
		HandleGroupCleared();
	}
}

void ARoundManager::HandleGroupCleared()
{
	if (State != ERoundState::WaitingClear && State != ERoundState::Spawning) return;

	const int32 ClearedIndex = CurrentWaveIndex;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WatchdogTimerHandle);
	}

	OnWaveCleared.Broadcast(ClearedIndex);
	UE_LOG(LogTemp, Log, TEXT("ARoundManager '%s': Wave %d cleared."), *GetName(), ClearedIndex);

	BeginNextWave();
}

void ARoundManager::SkipWave()
{
	if (!IsRoundActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("ARoundManager '%s': SkipWave ignored — round not active."), *GetName());
		return;
	}

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TM = World->GetTimerManager();
		TM.ClearTimer(DelayTimerHandle);
		TM.ClearTimer(WatchdogTimerHandle);
		TM.ClearTimer(CountdownTickHandle);
	}

	if (ActiveBoundGroup)
	{
		ActiveBoundGroup->DeactivateAll();
	}

	UE_LOG(LogTemp, Log, TEXT("ARoundManager '%s': Wave %d FORCE-skipped."), *GetName(), CurrentWaveIndex);
	OnWaveCleared.Broadcast(CurrentWaveIndex);
	BeginNextWave();
}

void ARoundManager::ForceCompleteRound()
{
	if (!Config) return;

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TM = World->GetTimerManager();
		TM.ClearTimer(DelayTimerHandle);
		TM.ClearTimer(WatchdogTimerHandle);
		TM.ClearTimer(CountdownTickHandle);
	}
	if (ActiveBoundGroup)
	{
		ActiveBoundGroup->DeactivateAll();
	}

	CurrentWaveIndex = Config->Waves.Num() - 1;
	UE_LOG(LogTemp, Log, TEXT("ARoundManager '%s': Round FORCE-completed."), *GetName());
	BeginNextWave();
}

void ARoundManager::DumpRoundState()
{
	const float TimeInState = GetWorld() ? GetWorld()->GetTimeSeconds() - StateEnteredTime : 0.f;
	UE_LOG(LogTemp, Log,
		TEXT("ARoundManager '%s' STATE DUMP: State=%d Wave=%d/%d TimeInState=%.2fs Active=%d ArenaId=%s"),
		*GetName(), (int32)State, CurrentWaveIndex,
		GetWaveCount(), TimeInState,
		ActiveBoundGroup ? ActiveBoundGroup->GetActiveCount() : -1,
		*ArenaId.ToString());
}

void ARoundManager::DrawDebugOverlay() const
{
	if (!GEngine) return;
	const float TimeInState = GetWorld() ? GetWorld()->GetTimeSeconds() - StateEnteredTime : 0.f;
	const int32 ActiveCount = ActiveBoundGroup ? ActiveBoundGroup->GetActiveCount() : -1;
	const FString Msg = FString::Printf(
		TEXT("[%s] %s | Wave %d/%d | t=%.1fs | Active=%d | Arena=%s"),
		*GetName(),
		*UEnum::GetValueAsString(State),
		CurrentWaveIndex + 1,
		GetWaveCount(),
		TimeInState,
		ActiveCount,
		*ArenaId.ToString());
	const uint64 Key = (uint64)GetUniqueID();
	GEngine->AddOnScreenDebugMessage(Key, 1.5f, FColor::Yellow, Msg);
}
