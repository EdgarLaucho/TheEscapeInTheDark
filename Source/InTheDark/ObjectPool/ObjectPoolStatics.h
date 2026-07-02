#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ObjectPoolStatics.generated.h"

UCLASS()
class INTHEDARK_API UObjectPoolStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Object Pool", meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "ActorClass"))
	static AActor* AcquireFromPool(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform);

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	static void ReleaseToPool(AActor* Actor);
};