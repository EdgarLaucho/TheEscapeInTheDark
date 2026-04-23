#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
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
	Cleared,
	Failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEncounterSimpleEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEncounterWaveEvent, int32, WaveIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEncounterBanterCue, FGameplayTag, CueTag);

/**
 * Ejecutor de oleadas basado en FSM. Vive en ACombatArena y lee
 * ACombatArena::Anchors / Config / Gates a través del owner.
 *
 * Ciclo de vida:
 *   Idle --StartEncounter()--> Starting -> WaveActive -> PostClear -> (WaveActive o Cleared)
 *   cualquier estado --muerte del jugador--> Failed (si Config->bReloadCheckpointOnFailure)
 */
UCLASS(ClassGroup = (Encounter), meta = (BlueprintSpawnableComponent))
class INTHEDARK_API UEncounterDirectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEncounterDirectorComponent();

	/* ---------- API ---------- */

	UFUNCTION(BlueprintCallable, Category = "Encounter")
	void StartEncounter();

	UFUNCTION(BlueprintCallable, Category = "Encounter")
	void AbortEncounter();

	UFUNCTION(BlueprintPure, Category = "Encounter")
	EEncounterState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Encounter")
	int32 GetCurrentWaveIndex() const { return CurrentWaveIndex; }

	UFUNCTION(BlueprintPure, Category = "Encounter")
	int32 GetAliveEnemyCount() const;

	/* ---------- Eventos ---------- */

	UPROPERTY(BlueprintAssignable, Category = "Encounter|Events")
	FEncounterSimpleEvent OnEncounterStarted;

	UPROPERTY(BlueprintAssignable, Category = "Encounter|Events")
	FEncounterWaveEvent OnWaveStarted;

	UPROPERTY(BlueprintAssignable, Category = "Encounter|Events")
	FEncounterWaveEvent OnWaveCleared;

	UPROPERTY(BlueprintAssignable, Category = "Encounter|Events")
	FEncounterSimpleEvent OnEncounterCleared;

	UPROPERTY(BlueprintAssignable, Category = "Encounter|Events")
	FEncounterSimpleEvent OnEncounterFailed;

	/** Disparado con los tags de banter de oleada. El sistema de diálogo escucha aquí. */
	UPROPERTY(BlueprintAssignable, Category = "Encounter|Events")
	FEncounterBanterCue OnBanterCue;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(Transient)
	EEncounterState State = EEncounterState::Idle;

	UPROPERTY(Transient)
	int32 CurrentWaveIndex = -1;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> AliveEnemies;

	/** Spawns pendientes esperando a que expire el lead de telegrafeo. */
	struct FPendingSpawn
	{
		TSubclassOf<AActor> EnemyClass;
		TWeakObjectPtr<ASpawnAnchor> Anchor;
		FEnemySpawn Directive;
		FTimerHandle TimerHandle;
	};
	TArray<FPendingSpawn> PendingSpawns;

	UPROPERTY(Transient)
	float WaveElapsed = 0.f;

	FTimerHandle DelayTimerHandle;
	FTimerHandle PostClearTimerHandle;

	/* --- pasos internos --- */

	void BeginNextWave();
	void BeginWaveActuallyNow();
	void SpawnDirectives(const FEncounterWave& Wave);
	void ExecutePendingSpawn(int32 PendingIndex);
	void CancelPendingSpawns();
	void TrackSpawnedEnemy(AActor* Enemy);
	void ReleaseAliveEnemiesToPool();
	bool HasActivePendingSpawns() const;
	void EvaluateWaveContinuation();
	void HandleWaveCleared();
	void HandleEncounterCleared();
	void HandleEncounterFailed();

	UFUNCTION()
	void HandleEnemyDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void HandleEnemyReleasedToPool(AActor* ReleasedActor);

	ACombatArena* GetArena() const;
	UEncounterConfig* GetConfig() const;
	const FEncounterWave* GetCurrentWave() const;
	TArray<ASpawnAnchor*> GetAvailableAnchorsForDirective(const FEnemySpawn& Directive) const;
	AActor* GetPlayerActor() const;

	/** Dispara un cue de banter. Los tags nulos se ignoran. */
	void FireBanter(FGameplayTag Tag);
};
