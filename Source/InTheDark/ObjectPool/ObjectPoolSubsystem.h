#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ObjectPool/ObjectPoolTypes.h"
#include "ObjectPoolSubsystem.generated.h"

UCLASS()
class INTHEDARK_API UObjectPoolSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Object Pool", meta = (WorldContext = "WorldContextObject"))
	AActor* AcquireFromPool(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform);

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	void ReleaseToPool(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	void RegisterPool(TSubclassOf<AActor> ActorClass, const FPoolSettings& Settings);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", meta = (WorldContext = "WorldContextObject"))
	void PrewarmPool(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, int32 Count);

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	void DrainAllPools();

	UFUNCTION(BlueprintCallable, Category = "Object Pool", BlueprintPure)
	bool HasPool(TSubclassOf<AActor> ActorClass) const;

	UPROPERTY(BlueprintAssignable, Category = "Object Pool|Events")
	FOnActorReleased OnActorReleased;

private:
	UPROPERTY()
	TMap<TSubclassOf<AActor>, FObjectPool> Pools;

	AActor* CreatePooledActor(UWorld* World, TSubclassOf<AActor> ActorClass, const FTransform& Transform);
	void DeactivateActor(AActor* Actor);
	void ActivateActor(AActor* Actor, const FTransform& Transform);
	void CleanupPool(FObjectPool& Pool);
	FObjectPool& EnsurePool(TSubclassOf<AActor> ActorClass);
	bool CanAcquire(const FObjectPool& Pool) const;

	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	FDelegateHandle WorldCleanupHandle;
};