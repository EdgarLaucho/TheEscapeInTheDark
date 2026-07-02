#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EncounterSystem/EncounterTypes.h"
#include "EncounterDirectorComponent.generated.h"

class UEncounterConfig;
class ASpawnAnchor;
class ACombatArena;
class UObjectPoolSubsystem;
struct FStreamableHandle;

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
	TSharedPtr<FStreamableHandle> EncounterPreloadHandle;
	TMap<FSoftObjectPath, TWeakObjectPtr<UClass>> PreloadedEnemyClasses;

	void PreloadEncounterClasses();
	void HandleEncounterClassesLoaded();
	TSubclassOf<AActor> ResolveEnemyClass(const FEnemySpawn& Directive) const;
	void BeginNextWave();
	void BeginWaveActuallyNow();
	void SpawnDirectives(const FEncounterWave& Wave);
	void SpawnDirective(const FEnemySpawn& Directive, TSet<ASpawnAnchor*>& ReservedAnchors);
	ASpawnAnchor* ChooseAnchorForSpawn(TSubclassOf<AActor> EnemyClass, TSet<ASpawnAnchor*>& ReservedAnchors) const;
	void ScheduleSpawn(TSubclassOf<AActor> EnemyClass, ASpawnAnchor* Anchor, float LeadSeconds);
	void ExecutePendingSpawn(int32 PendingIndex);
	bool TryPreparePendingSpawn(FPendingSpawn& Pending, int32 PendingIndex);
	void CancelPendingSpawns();
	void TrackSpawnedEnemy(AActor* Enemy);
	void ReleaseAliveEnemiesToPool();
	int32 GetAliveEnemyCount() const;
	bool HasActivePendingSpawns() const;
	bool ShouldAdvanceWave(const FWaveContinuation& Rule, int32 Remaining) const;
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
	UObjectPoolSubsystem* GetPool() const;
};
