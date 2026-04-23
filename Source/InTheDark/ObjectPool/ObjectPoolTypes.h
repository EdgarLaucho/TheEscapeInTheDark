#pragma once

#include "CoreMinimal.h"
#include "ObjectPoolTypes.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct INTHEDARK_API FPoolSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Settings")
	int32 PrewarmCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Settings")
	int32 MaxPoolSize = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Settings")
	bool bAutoExpand = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Settings")
	bool bAutoRegister = true;
};

USTRUCT(BlueprintType)
struct INTHEDARK_API FPoolStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Pool Stats")
	int32 ActiveCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Pool Stats")
	int32 InactiveCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Pool Stats")
	int32 QueuedRequests = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Pool Stats")
	int32 TotalCreated = 0;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPoolRequestFulfilled, AActor*, Actor);

struct FPoolRequest
{
	TSubclassOf<AActor> ActorClass;
	FTransform SpawnTransform;
	FOnPoolRequestFulfilled OnFulfilled;
};

USTRUCT()
struct FObjectPool
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> InactiveActors;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> ActiveActors;

	UPROPERTY()
	FPoolSettings Settings;

	TArray<FPoolRequest> QueuedRequests;

	UPROPERTY()
	int32 TotalCreated = 0;
};

USTRUCT(BlueprintType)
struct INTHEDARK_API FSpawnClassEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSubclassOf<AActor> ActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "0.01"))
	float Weight = 1.0f;
};

USTRUCT(BlueprintType)
struct INTHEDARK_API FPoolCatalogEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Catalog")
	TSubclassOf<AActor> ActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Catalog")
	FPoolSettings Settings;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPoolExhausted, TSubclassOf<AActor>, ActorClass);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorAcquired, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorReleased, AActor*, Actor);
