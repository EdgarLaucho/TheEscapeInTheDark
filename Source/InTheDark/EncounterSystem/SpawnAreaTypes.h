#pragma once

#include "CoreMinimal.h"
#include "SpawnAreaTypes.generated.h"

USTRUCT(BlueprintType)
struct INTHEDARK_API FSpawnAreaEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSubclassOf<AActor> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = 1))
	int32 MaxCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = 0.1f))
	float Weight = 1.f;
};

USTRUCT(BlueprintType)
struct INTHEDARK_API FSpawnAreaRules
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area", meta = (ClampMin = 100.f))
	float AreaRadius = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area", meta = (ClampMin = 0.f))
	float DespawnOffset = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (ClampMin = 1))
	int32 MaxSimultaneous = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (ClampMin = 0.5f))
	float SpawnInterval = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (ClampMin = 0.f))
	float InitialSpawnDelay = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	bool bRespawnOnDeath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Despawn")
	bool bRequireOutOfSightToDespawn = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Despawn", meta = (ClampMin = 10.f, ClampMax = 180.f))
	float VisibilityConeHalfAngle = 60.f;
};