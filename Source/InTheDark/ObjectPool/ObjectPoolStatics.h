#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ObjectPool/ObjectPoolTypes.h"
#include "ObjectPoolStatics.generated.h"

class UObjectPoolSubsystem;

UCLASS()
class INTHEDARK_API UObjectPoolStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Object Pool", meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "ActorClass"))
	static AActor* AcquireFromPool(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform);

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	static void ReleaseToPool(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", meta = (WorldContext = "WorldContextObject"))
	static void PrewarmPool(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, int32 Count);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", meta = (WorldContext = "WorldContextObject"))
	static void RegisterPool(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FPoolSettings& Settings);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", meta = (WorldContext = "WorldContextObject"))
	static void ReleaseAllActive(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static FPoolStats GetPoolStats(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static bool HasPool(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static UObjectPoolSubsystem* GetObjectPoolSubsystem(UObject* WorldContextObject);
};
