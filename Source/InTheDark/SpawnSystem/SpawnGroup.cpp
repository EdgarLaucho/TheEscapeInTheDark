#include "SpawnSystem/SpawnGroup.h"
#include "SpawnSystem/SpawnPoint.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"

ASpawnGroup::ASpawnGroup()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void ASpawnGroup::BeginPlay()
{
	Super::BeginPlay();

	for (ASpawnPoint* Point : SpawnPoints)
	{
		if (IsValid(Point))
		{
			Point->OnActorSpawned.AddDynamic(this, &ASpawnGroup::OnPointActorSpawned);
			Point->OnActorDespawned.AddDynamic(this, &ASpawnGroup::OnPointActorDespawned);
		}
	}
}

void ASpawnGroup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);

	for (ASpawnPoint* Point : SpawnPoints)
	{
		if (IsValid(Point))
		{
			Point->OnActorSpawned.RemoveDynamic(this, &ASpawnGroup::OnPointActorSpawned);
			Point->OnActorDespawned.RemoveDynamic(this, &ASpawnGroup::OnPointActorDespawned);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ASpawnGroup::ActivateAll()
{
	for (ASpawnPoint* Point : SpawnPoints)
	{
		if (IsValid(Point))
		{
			Point->Activate();
		}
	}
}

void ASpawnGroup::DeactivateAll()
{
	for (ASpawnPoint* Point : SpawnPoints)
	{
		if (IsValid(Point))
		{
			Point->Deactivate();
		}
	}
}

void ASpawnGroup::SpawnFromRandom(int32 Count, float Interval)
{
	if (SpawnPoints.Num() == 0 || Count <= 0)
	{
		return;
	}

	SpawnQueue.Empty();
	for (int32 i = 0; i < Count; ++i)
	{
		int32 RandomIndex = FMath::RandRange(0, SpawnPoints.Num() - 1);
		ASpawnPoint* Point = SpawnPoints[RandomIndex];
		if (IsValid(Point))
		{
			SpawnQueue.Add({ Point });
		}
	}

	CompletedSpawnCount = 0;
	CurrentSpawnInterval = Interval;

	if (Interval <= 0.f)
	{

		while (SpawnQueue.Num() > 0)
		{
			ExecuteNextSpawn();
		}
		OnGroupSpawnComplete.Broadcast(CompletedSpawnCount);
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			SpawnTimerHandle, this, &ASpawnGroup::ExecuteNextSpawn,
			Interval, true, 0.f);
	}
}

void ASpawnGroup::SpawnFromAll(float Interval)
{
	SpawnQueue.Empty();
	for (ASpawnPoint* Point : SpawnPoints)
	{
		if (IsValid(Point))
		{
			SpawnQueue.Add({ Point });
		}
	}

	CompletedSpawnCount = 0;
	CurrentSpawnInterval = Interval;

	if (Interval <= 0.f)
	{
		while (SpawnQueue.Num() > 0)
		{
			ExecuteNextSpawn();
		}
		OnGroupSpawnComplete.Broadcast(CompletedSpawnCount);
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			SpawnTimerHandle, this, &ASpawnGroup::ExecuteNextSpawn,
			Interval, true, 0.f);
	}
}

int32 ASpawnGroup::GetActiveCount() const
{
	int32 Total = 0;
	for (const ASpawnPoint* Point : SpawnPoints)
	{
		if (IsValid(Point))
		{
			Total += Point->GetActiveSpawnCount();
		}
	}
	return Total;
}

void ASpawnGroup::SpawnFromRandomFiltered(int32 Count, float Interval, const TArray<TSubclassOf<AActor>>& ClassFilter)
{
	if (SpawnPoints.Num() == 0 || Count <= 0)
	{
		return;
	}

	SpawnQueue.Empty();
	for (int32 i = 0; i < Count; ++i)
	{
		const int32 RandomIndex = FMath::RandRange(0, SpawnPoints.Num() - 1);
		ASpawnPoint* Point = SpawnPoints[RandomIndex];
		if (!IsValid(Point)) continue;

		TSubclassOf<AActor> Override = nullptr;
		if (ClassFilter.Num() > 0)
		{
			Override = ClassFilter[FMath::RandRange(0, ClassFilter.Num() - 1)];
		}
		SpawnQueue.Add({ Point, Override });
	}

	CompletedSpawnCount = 0;
	CurrentSpawnInterval = Interval;

	if (Interval <= 0.f)
	{
		while (SpawnQueue.Num() > 0)
		{
			ExecuteNextSpawn();
		}
		OnGroupSpawnComplete.Broadcast(CompletedSpawnCount);
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			SpawnTimerHandle, this, &ASpawnGroup::ExecuteNextSpawn,
			Interval, true, 0.f);
	}
}

void ASpawnGroup::SpawnFromAllFiltered(float Interval, const TArray<TSubclassOf<AActor>>& ClassFilter)
{
	SpawnQueue.Empty();
	for (ASpawnPoint* Point : SpawnPoints)
	{
		if (!IsValid(Point)) continue;
		TSubclassOf<AActor> Override = nullptr;
		if (ClassFilter.Num() > 0)
		{
			Override = ClassFilter[FMath::RandRange(0, ClassFilter.Num() - 1)];
		}
		SpawnQueue.Add({ Point, Override });
	}

	CompletedSpawnCount = 0;
	CurrentSpawnInterval = Interval;

	if (Interval <= 0.f)
	{
		while (SpawnQueue.Num() > 0)
		{
			ExecuteNextSpawn();
		}
		OnGroupSpawnComplete.Broadcast(CompletedSpawnCount);
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			SpawnTimerHandle, this, &ASpawnGroup::ExecuteNextSpawn,
			Interval, true, 0.f);
	}
}

void ASpawnGroup::ExecuteNextSpawn()
{
	if (SpawnQueue.Num() == 0)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		OnGroupSpawnComplete.Broadcast(CompletedSpawnCount);
		return;
	}

	FSpawnQueueEntry Entry = SpawnQueue[0];
	SpawnQueue.RemoveAt(0);

	if (IsValid(Entry.Point))
	{
		AActor* Spawned = Entry.OverrideClass
			? Entry.Point->RequestSpawnOfClass(Entry.OverrideClass)
			: Entry.Point->RequestSpawn();
		if (Spawned)
		{
			CompletedSpawnCount++;
		}
	}

	if (SpawnQueue.Num() == 0)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		OnGroupSpawnComplete.Broadcast(CompletedSpawnCount);
	}
}

void ASpawnGroup::OnPointActorSpawned(AActor* Actor)
{
	TotalGroupSpawns++;
}

void ASpawnGroup::OnPointActorDespawned(AActor* Actor)
{
	CheckGroupCleared();
}

void ASpawnGroup::CheckGroupCleared()
{
	if (TotalGroupSpawns > 0 && GetActiveCount() == 0)
	{
		OnGroupCleared.Broadcast();
	}
}
