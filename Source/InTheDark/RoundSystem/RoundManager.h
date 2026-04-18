#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoundConfig.h"
#include "RoundManager.generated.h"

class ASpawnGroup;
class URoundConfig;

UENUM(BlueprintType)
enum class ERoundState : uint8
{
	Idle             UMETA(DisplayName = "Idle"),
	WaitingDelay     UMETA(DisplayName = "Pre-Wave Countdown"),
	Spawning         UMETA(DisplayName = "Spawning"),
	WaitingClear     UMETA(DisplayName = "Waiting For Clear"),
	Paused           UMETA(DisplayName = "Paused"),
	Failed           UMETA(DisplayName = "Failed"),
	Completed        UMETA(DisplayName = "Completed"),
};

USTRUCT(BlueprintType)
struct FWaveSpawnGroupOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override", meta = (ClampMin = "0"))
	int32 WaveIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	TObjectPtr<ASpawnGroup> SpawnGroup;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoundStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPreWaveStarted, int32, WaveIndex, float, CountdownSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPreWaveTick, int32, WaveIndex, float, RemainingSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveStarted, int32, WaveIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveCleared, int32, WaveIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoundCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoundStopped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoundPaused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoundResumed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoundFailed, FName, Reason);

UCLASS(BlueprintType, Blueprintable)
class INTHEDARK_API ARoundManager : public AActor
{
	GENERATED_BODY()

public:
	ARoundManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round Manager")
	TObjectPtr<URoundConfig> Config;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round Manager")
	TObjectPtr<ASpawnGroup> SpawnGroup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round Manager")
	TArray<FWaveSpawnGroupOverride> WaveSpawnGroupOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round Manager|Persistence")
	FName ArenaId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round Manager|Persistence")
	bool bSkipIfAlreadyCompleted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round Manager")
	bool bAutoStart = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round Manager|Fail")
	bool bFailOnPlayerDeath = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round Manager|Debug")
	bool bShowDebugOverlay = false;

	UFUNCTION(BlueprintCallable, Category = "Round Manager")
	void StartRound();

	UFUNCTION(BlueprintCallable, Category = "Round Manager")
	void StopRound();

	UFUNCTION(BlueprintCallable, Category = "Round Manager")
	void PauseRound();

	UFUNCTION(BlueprintCallable, Category = "Round Manager")
	void ResumeRound();

	UFUNCTION(BlueprintCallable, Category = "Round Manager")
	void FailRound(FName Reason);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Round Manager")
	ERoundState GetState() const { return State; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Round Manager")
	int32 GetCurrentWaveIndex() const { return CurrentWaveIndex; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Round Manager")
	int32 GetWaveCount() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Round Manager")
	bool IsRoundActive() const
	{
		return State != ERoundState::Idle
			&& State != ERoundState::Completed
			&& State != ERoundState::Failed;
	}

	UFUNCTION(Exec, Category = "Round Manager|Debug")
	void SkipWave();

	UFUNCTION(Exec, Category = "Round Manager|Debug")
	void ForceCompleteRound();

	UFUNCTION(Exec, Category = "Round Manager|Debug")
	void DumpRoundState();

	UPROPERTY(BlueprintAssignable, Category = "Round Manager|Events")
	FOnRoundStarted OnRoundStarted;

	UPROPERTY(BlueprintAssignable, Category = "Round Manager|Events")
	FOnPreWaveStarted OnPreWaveStarted;

	UPROPERTY(BlueprintAssignable, Category = "Round Manager|Events")
	FOnPreWaveTick OnPreWaveTick;

	UPROPERTY(BlueprintAssignable, Category = "Round Manager|Events")
	FOnWaveStarted OnWaveStarted;

	UPROPERTY(BlueprintAssignable, Category = "Round Manager|Events")
	FOnWaveCleared OnWaveCleared;

	UPROPERTY(BlueprintAssignable, Category = "Round Manager|Events")
	FOnRoundCompleted OnRoundCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Round Manager|Events")
	FOnRoundStopped OnRoundStopped;

	UPROPERTY(BlueprintAssignable, Category = "Round Manager|Events")
	FOnRoundPaused OnRoundPaused;

	UPROPERTY(BlueprintAssignable, Category = "Round Manager|Events")
	FOnRoundResumed OnRoundResumed;

	UPROPERTY(BlueprintAssignable, Category = "Round Manager|Events")
	FOnRoundFailed OnRoundFailed;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void BeginNextWave();
	void StartCurrentWaveSpawn();
	void OnWatchdogElapsed();
	void OnPreWaveCountdownTick();
	void TryBindPlayerDeath();

	ASpawnGroup* ResolveSpawnGroupForWave(int32 WaveIndex) const;
	int32 ResolveSpawnCountForWave(const FRoundWave& Wave) const;
	void DrawDebugOverlay() const;

	UFUNCTION()
	void HandleGroupSpawnComplete(int32 TotalSpawned);

	UFUNCTION()
	void HandleGroupCleared();

	UFUNCTION()
	void HandlePlayerDestroyed(AActor* DestroyedActor);

	void BindGroupDelegates(ASpawnGroup* Group);
	void UnbindGroupDelegates(ASpawnGroup* Group);

	UPROPERTY()
	ERoundState State = ERoundState::Idle;

	UPROPERTY()
	ERoundState PreviousStateBeforePause = ERoundState::Idle;

	int32 CurrentWaveIndex = INDEX_NONE;

	FTimerHandle DelayTimerHandle;
	FTimerHandle WatchdogTimerHandle;
	FTimerHandle CountdownTickHandle;

	float CurrentCountdownRemaining = 0.f;
	float StateEnteredTime = 0.f;
	int32 LastWaveSpawnedCount = 0;

	UPROPERTY()
	TObjectPtr<ASpawnGroup> ActiveBoundGroup;

	UPROPERTY()
	TWeakObjectPtr<AActor> BoundPlayer;
};
