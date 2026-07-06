#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EncounterSystem/EncounterTypes.h"
#include "EncounterDirectorComponent.generated.h"

class UEncounterConfig;
class ASpawnAnchor;
class ACombatArena;
class UObjectPoolSubsystem;
class UPrimitiveComponent;
struct FStreamableHandle;

UENUM()
enum class EEncounterState : uint8
{
	Idle,
	WaveDelay,
	WaveActive,
	Cleared
};

UCLASS(ClassGroup = (Encounter), meta = (BlueprintSpawnableComponent))
class INTHEDARK_API UEncounterDirectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void StartEncounter();
	bool IsEncounterActive() const;

private:
	UPROPERTY(Transient)
	EEncounterState State = EEncounterState::Idle;

	UPROPERTY(Transient)
	int32 CurrentWaveIndex = -1;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> AliveEnemies;

	TArray<TSubclassOf<AActor>> SpawnQueue;

	FTimerHandle SpawnTimerHandle;
	FTimerHandle DelayTimerHandle;
	FTimerHandle PostClearTimerHandle;
	TSharedPtr<FStreamableHandle> EncounterPreloadHandle;
	TMap<FSoftObjectPath, TWeakObjectPtr<UClass>> PreloadedEnemyClasses;

	void PreloadEncounterClasses();
	void HandleEncounterClassesLoaded();
	TSubclassOf<AActor> ResolveEnemyClass(const FEnemySpawn& Directive) const;
	void BeginNextWave();
	void StartWave();
	void WarmUpCurrentWavePools(const FEncounterWave& Wave);
	void SpawnNextInQueue();
	ASpawnAnchor* ChooseFreeAnchor() const;
	void TrackSpawnedEnemy(AActor* Enemy);
	void ReleaseAliveEnemiesToPool();
	int32 GetAliveEnemyCount() const;
	void CheckWaveCleared();
	void HandleWaveCleared();
	void HandleEncounterCleared();

	UFUNCTION()
	void HandleEnemyDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void HandleEnemyReleasedToPool(AActor* ReleasedActor);

	UFUNCTION()
	void HandleEnemyLeftContainment(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	bool IsEnemyPendingRemoval(const AActor* Enemy) const;
	void ReturnEnemyToAnchor(AActor* Enemy);

	ACombatArena* GetArena() const;
	UEncounterConfig* GetConfig() const;
	const FEncounterWave* GetCurrentWave() const;
	UObjectPoolSubsystem* GetPool() const;
};
