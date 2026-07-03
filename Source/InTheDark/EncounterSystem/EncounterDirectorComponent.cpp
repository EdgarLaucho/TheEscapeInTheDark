#include "EncounterSystem/EncounterDirectorComponent.h"
#include "EncounterSystem/CombatArena.h"
#include "EncounterSystem/EncounterConfig.h"
#include "EncounterSystem/EncounterTargetInterface.h"
#include "EncounterSystem/SpawnAnchor.h"
#include "ObjectPool/ObjectPoolSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

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
		UE_LOG(LogTemp, Warning, TEXT("EncounterDirector: no config or no waves on %s"), *GetOwner()->GetName());
		return;
	}

	CurrentWaveIndex = -1;
	AliveEnemies.Reset();
	SpawnQueue.Reset();

	if (UObjectPoolSubsystem* Pool = GetPool())
	{
		Pool->OnActorReleased.AddUniqueDynamic(this, &UEncounterDirectorComponent::HandleEnemyReleasedToPool);
	}

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

	State = EEncounterState::WaveDelay;

	const FEncounterWave& Wave = Cfg->Waves[CurrentWaveIndex];
	if (Wave.DelayBeforeWave > 0.f && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(DelayTimerHandle, this,
			&UEncounterDirectorComponent::StartWave, Wave.DelayBeforeWave, false);
	}
	else
	{
		StartWave();
	}
}

void UEncounterDirectorComponent::StartWave()
{
	const FEncounterWave* Wave = GetCurrentWave();
	if (!Wave) { return; }

	SpawnQueue.Reset();
	for (const FEnemySpawn& Spawn : Wave->Spawns)
	{
		UClass* EnemyClass = Spawn.Enemy.LoadSynchronous();
		if (!EnemyClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("EncounterDirector: wave %d has a null enemy class."), CurrentWaveIndex);
			continue;
		}
		for (int32 i = 0; i < FMath::Max(1, Spawn.Count); ++i)
		{
			SpawnQueue.Add(EnemyClass);
		}
	}

	State = EEncounterState::WaveActive;

	const ACombatArena* Arena = GetArena();
	if (SpawnQueue.IsEmpty() || !Arena || Arena->Anchors.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("EncounterDirector: wave %d has no enemies or no anchors; treating as cleared."), CurrentWaveIndex);
		HandleWaveCleared();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(SpawnTimerHandle, this,
			&UEncounterDirectorComponent::SpawnNextInQueue, Wave->SecondsBetweenSpawns, true);
	}
	SpawnNextInQueue();
}

void UEncounterDirectorComponent::SpawnNextInQueue()
{
	const FEncounterWave* Wave = GetCurrentWave();
	if (State != EEncounterState::WaveActive || !Wave) { return; }

	if (SpawnQueue.IsEmpty())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(SpawnTimerHandle);
		}
		CheckWaveCleared();
		return;
	}

	if (GetAliveEnemyCount() >= Wave->MaxAlive) { return; }

	ASpawnAnchor* Anchor = ChooseFreeAnchor();
	if (!Anchor) { return; }

	TSubclassOf<AActor> EnemyClass = SpawnQueue[0];
	SpawnQueue.RemoveAt(0);
	TrackSpawnedEnemy(Anchor->PerformSpawn(EnemyClass));
}

ASpawnAnchor* UEncounterDirectorComponent::ChooseFreeAnchor() const
{
	const ACombatArena* Arena = GetArena();
	if (!Arena) { return nullptr; }

	TArray<ASpawnAnchor*> Free;
	for (const TObjectPtr<ASpawnAnchor>& Anchor : Arena->Anchors)
	{
		if (Anchor && Anchor->IsAvailableForSpawn())
		{
			Free.Add(Anchor);
		}
	}
	return Free.IsEmpty() ? nullptr : Free[FMath::RandRange(0, Free.Num() - 1)];
}

void UEncounterDirectorComponent::TrackSpawnedEnemy(AActor* Enemy)
{
	if (!Enemy)
	{
		return; 
	}

	AliveEnemies.Add(Enemy);
	Enemy->OnDestroyed.AddDynamic(this, &UEncounterDirectorComponent::HandleEnemyDestroyed);

	if (Enemy->GetClass()->ImplementsInterface(UEncounterTargetInterface::StaticClass()))
	{
		IEncounterTargetInterface::Execute_OnEncounterSpawned(Enemy, UGameplayStatics::GetPlayerPawn(this, 0));
	}
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
	CheckWaveCleared();
}

void UEncounterDirectorComponent::HandleEnemyReleasedToPool(AActor* ReleasedActor)
{
	AliveEnemies.RemoveAll([ReleasedActor](const TWeakObjectPtr<AActor>& E) {
		return E.Get() == ReleasedActor;
	});
	CheckWaveCleared();
}

void UEncounterDirectorComponent::CheckWaveCleared()
{
	if (State != EEncounterState::WaveActive) { return; }

	if (SpawnQueue.IsEmpty() && GetAliveEnemyCount() == 0)
	{
		HandleWaveCleared();
	}
}

void UEncounterDirectorComponent::HandleWaveCleared()
{
	if (State != EEncounterState::WaveActive) { return; }

	State = EEncounterState::WaveDelay;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}

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

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
		World->GetTimerManager().ClearTimer(DelayTimerHandle);
		World->GetTimerManager().ClearTimer(PostClearTimerHandle);
	}

	SpawnQueue.Reset();
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
}
