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

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UObjectPoolSubsystem* Pool = GI->GetSubsystem<UObjectPoolSubsystem>())
		{
			Pool->OnActorReleased.AddUniqueDynamic(this, &UEncounterDirectorComponent::HandleEnemyReleasedToPool);
		}
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

	// Si ningún spawn produjo enemigos y no hay pendientes, saltar.
	if (AliveEnemies.Num() == 0 && PendingSpawns.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterDirector: wave %d produced 0 enemies; treating as cleared."),
			CurrentWaveIndex);
		HandleWaveCleared();
	}
}

TArray<ASpawnAnchor*> UEncounterDirectorComponent::GetAvailableAnchorsForDirective(
	const FEnemySpawn& Directive,
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
		if (Directive.AnchorTag.IsValid() && !A->AnchorTags.HasTag(Directive.AnchorTag))
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
	UWorld* World = GetWorld();
	if (!World) { return; }

	TSet<ASpawnAnchor*> ReservedAnchors;

	for (const FEnemySpawn& Directive : Wave.Spawns)
	{
		if (Directive.Enemy.IsNull())
		{
			UE_LOG(LogTemp, Warning, TEXT("EncounterDirector: spawn directive has null Enemy class."));
			continue;
		}
		UClass* EnemyCls = Directive.Enemy.LoadSynchronous();
		if (!EnemyCls) { continue; }

		for (int32 i = 0; i < FMath::Max(1, Directive.Count); ++i)
		{
			TArray<ASpawnAnchor*> Candidates = GetAvailableAnchorsForDirective(Directive, EnemyCls, &ReservedAnchors);
			if (Candidates.Num() == 0)
			{
				// Fallback: cualquier anchor con el tag, ignorando FOV/distancia, pero nunca ocupacion/reserva.
				const ACombatArena* Arena = GetArena();
				if (Arena)
				{
					for (const TObjectPtr<ASpawnAnchor>& A : Arena->Anchors)
					{
						if (!A) continue;
						if (ReservedAnchors.Contains(A.Get())) continue;
						if (Directive.AnchorTag.IsValid() && !A->AnchorTags.HasTag(Directive.AnchorTag)) continue;
						if (A->IsSpawnLocationOccupied(EnemyCls)) continue;
						Candidates.Add(A);
					}
				}
			}
			if (Candidates.Num() == 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("EncounterDirector: no anchors for directive tag '%s'."),
					*Directive.AnchorTag.ToString());
				continue;
			}

			ASpawnAnchor* Picked = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
			ReservedAnchors.Add(Picked);
			const float Lead = Picked->ResolveLead(Directive);

			Picked->PlayTelegraph(Directive);

			if (Lead > 0.f)
			{
				const int32 Idx = PendingSpawns.Num();
				FPendingSpawn Pending;
				Pending.EnemyClass = EnemyCls;
				Pending.Anchor = Picked;
				Pending.Directive = Directive;
				PendingSpawns.Add(MoveTemp(Pending));
				FTimerDelegate TimerDel;
				TimerDel.BindUObject(this, &UEncounterDirectorComponent::ExecutePendingSpawn, Idx);
				World->GetTimerManager().SetTimer(PendingSpawns[Idx].TimerHandle, TimerDel, Lead, false);
			}
			else
			{
				AActor* Spawned = Picked->PerformSpawn(EnemyCls, Directive);
				TrackSpawnedEnemy(Spawned);
			}
		}
	}
}

void UEncounterDirectorComponent::ExecutePendingSpawn(int32 PendingIndex)
{
	if (!PendingSpawns.IsValidIndex(PendingIndex)) { return; }

	FPendingSpawn& Pending = PendingSpawns[PendingIndex];
	if (!Pending.Anchor.IsValid() || !Pending.EnemyClass) { return; }

	if (Pending.Anchor->IsSpawnLocationOccupied(Pending.EnemyClass))
	{
		TArray<ASpawnAnchor*> Candidates = GetAvailableAnchorsForDirective(Pending.Directive, Pending.EnemyClass);
		if (Candidates.Num() > 0)
		{
			Pending.Anchor = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
		}
		else if (Pending.RetryCount < 10)
		{
			++Pending.RetryCount;
			if (UWorld* World = GetWorld())
			{
				FTimerDelegate TimerDel;
				TimerDel.BindUObject(this, &UEncounterDirectorComponent::ExecutePendingSpawn, PendingIndex);
				World->GetTimerManager().SetTimer(Pending.TimerHandle, TimerDel, 0.35f, false);
			}
			return;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("EncounterDirector: skipping spawn for %s; no free anchor after retries."),
				*Pending.EnemyClass->GetName());
			return;
		}
	}

	AActor* Spawned = Pending.Anchor->PerformSpawn(Pending.EnemyClass, Pending.Directive);
	TrackSpawnedEnemy(Spawned);

	// Comprueba si fue el último spawn diferido y no quedan enemigos vivos.
	if (State == EEncounterState::WaveActive && AliveEnemies.Num() == 0 && !HasActivePendingSpawns())
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterDirector: all deferred spawns complete but 0 enemies alive; treating as cleared."));
		HandleWaveCleared();
	}
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
	UObjectPoolSubsystem* Pool = nullptr;
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		Pool = GI->GetSubsystem<UObjectPoolSubsystem>();
	}

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

void UEncounterDirectorComponent::EvaluateWaveContinuation()
{
	const FEncounterWave* Wave = GetCurrentWave();
	if (!Wave) { return; }

	if (HasActivePendingSpawns()) { return; }

	const int32 Remaining = GetAliveEnemyCount();
	const FWaveContinuation& Rule = Wave->Continuation;

	bool bAdvance = false;
	switch (Rule.Mode)
	{
	case EWaveContinuationMode::OnAllCleared:
		bAdvance = (Remaining == 0);
		break;
	case EWaveContinuationMode::OnRemainingAtOrBelow:
		bAdvance = (Remaining <= Rule.RemainingThreshold);
		break;
	case EWaveContinuationMode::OnElapsedSince:
		bAdvance = (WaveElapsed >= Rule.ElapsedSeconds);
		break;
	case EWaveContinuationMode::Hybrid:
		bAdvance = (Remaining <= Rule.RemainingThreshold) || (WaveElapsed >= Rule.ElapsedSeconds);
		break;
	}

	if (bAdvance)
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

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UObjectPoolSubsystem* Pool = GI->GetSubsystem<UObjectPoolSubsystem>())
		{
			Pool->OnActorReleased.RemoveDynamic(this, &UEncounterDirectorComponent::HandleEnemyReleasedToPool);
		}
	}

	OnEncounterCleared.Broadcast();
}
