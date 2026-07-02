#pragma once

#include "CoreMinimal.h"
#include "EncounterTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EWaveContinuationMode : uint8
{
	OnAllCleared,
	OnRemainingAtOrBelow,
	OnElapsedSince,
	Hybrid
};

USTRUCT(BlueprintType)
struct INTHEDARK_API FWaveContinuation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Continuation")
	EWaveContinuationMode Mode = EWaveContinuationMode::OnAllCleared;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Continuation", meta = (EditCondition = "Mode != EWaveContinuationMode::OnAllCleared"))
	int32 RemainingThreshold = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Continuation", meta = (EditCondition = "Mode != EWaveContinuationMode::OnAllCleared"))
	float ElapsedSeconds = 8.f;
};

USTRUCT(BlueprintType)
struct INTHEDARK_API FEnemySpawn
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSoftClassPtr<AActor> Enemy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "1"))
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "0.0"))
	float PreSpawnLead = 0.8f;
};

USTRUCT(BlueprintType)
struct INTHEDARK_API FEncounterWave
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FName WaveName = TEXT("Wave");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TArray<FEnemySpawn> Spawns;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FWaveContinuation Continuation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "0.0"))
	float DelayBeforeWave = 0.f;
};