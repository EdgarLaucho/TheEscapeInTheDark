#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoundConfig.generated.h"

USTRUCT(BlueprintType)
struct FRoundWave
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FName WaveName = TEXT("Wave");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave",
		meta = (ClampMin = "1", EditCondition = "!bSpawnFromAllPoints"))
	int32 SpawnCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	bool bSpawnFromAllPoints = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "0.0"))
	float SpawnInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "0.0"))
	float DelayBeforeWave = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TArray<TSoftClassPtr<AActor>> ClassFilter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Watchdog", meta = (ClampMin = "0.0"))
	float MaxWaveDurationSeconds = 0.f;
};

UCLASS(BlueprintType)
class INTHEDARK_API URoundConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round Config")
	FName ConfigId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round Config")
	TArray<FRoundWave> Waves;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round Config")
	bool bLoop = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round Config", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float DifficultyMultiplier = 1.0f;
};
