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

	UPROPERTY()
	int32 TotalCreated = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorReleased, AActor*, Actor);