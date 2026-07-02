#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EncounterSystem/EncounterTypes.h"
#include "EncounterDirectorComponent.generated.h"

class UEncounterConfig;
class ASpawnAnchor;
class ACombatArena;
class UObjectPoolSubsystem;

UENUM(BlueprintType)
enum class EEncounterState : uint8
{
	Idle,
	Starting,
	WaveActive,
	PostClear,
	Cleared
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEncounterSimpleEvent);

UCLASS(ClassGroup = (Encounter), meta = (BlueprintSpawnableComponent))
class INTHEDARK_API UEncounterDirectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEncounterDirectorComponent();

	void StartEncounter();

	UPROPERTY(BlueprintAssignable, Category = "Encounter|Events")
	FEncounterSimpleEvent OnEncounterCleared;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(Transient)
	EEncounterState State = EEncounterState::Idle;

	UPROPERTY(Transient)
	int32 CurrentWaveIndex = -1;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> AliveEnemies;

	struct FPendingSpawn
	{
		TSubclassOf<AActor> EnemyClass;
		TWeakObjectPtr<ASpawnAnchor> Anchor;
		FTimerHandle TimerHandle;
		int32 RetryCount = 0;
	};
	TArray<FPendingSpawn> PendingSpawns;

	UPROPERTY(Transient)
	float WaveElapsed = 0.f;

	FTimerHandle DelayTimerHandle;
	FTimerHandle PostClearTimerHandle;

	void BeginNextWave();
	void BeginWaveActuallyNow();
	void SpawnDirectives(const FEncounterWave& Wave);
	void ExecutePendingSpawn(int32 PendingIndex);
	void CancelPendingSpawns();
	void TrackSpawnedEnemy(AActor* Enemy);
	void ReleaseAliveEnemiesToPool();
	int32 GetAliveEnemyCount() const;
	bool HasActivePendingSpawns() const;
	void EvaluateWaveContinuation();
	void HandleWaveCleared();
	void HandleEncounterCleared();

	UFUNCTION()
	void HandleEnemyDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void HandleEnemyReleasedToPool(AActor* ReleasedActor);

	ACombatArena* GetArena() const;
	UEncounterConfig* GetConfig() const;
	const FEncounterWave* GetCurrentWave() const;
	TArray<ASpawnAnchor*> GetAvailableAnchors(TSubclassOf<AActor> EnemyClass, const TSet<ASpawnAnchor*>* ReservedAnchors = nullptr) const;
	AActor* GetPlayerActor() const;
};