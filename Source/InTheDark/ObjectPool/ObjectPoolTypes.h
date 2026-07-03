#pragma once

#include "CoreMinimal.h"
#include "ObjectPoolTypes.generated.h"

class AActor;

USTRUCT()
struct FObjectPool
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> InactiveActors;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> ActiveActors;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorReleased, AActor*, Actor);