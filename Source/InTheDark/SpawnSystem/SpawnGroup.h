#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpawnGroup.generated.h"

class ASpawnPoint;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGroupCleared);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGroupSpawnComplete, int32, TotalSpawned);

UCLASS(BlueprintType, Blueprintable)
class INTHEDARK_API ASpawnGroup : public AActor
{
	GENERATED_BODY()

public:
	ASpawnGroup();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Group")
	TArray<TObjectPtr<ASpawnPoint>> SpawnPoints;

	UFUNCTION(BlueprintCallable, Category = "Spawn Group")
	void ActivateAll();

	UFUNCTION(BlueprintCallable, Category = "Spawn Group")
	void DeactivateAll();

	UFUNCTION(BlueprintCallable, Category = "Spawn Group")
	void SpawnFromRandom(int32 Count, float Interval = 0.f);

	UFUNCTION(BlueprintCallable, Category = "Spawn Group")
	void SpawnFromAll(float Interval = 0.f);

	UFUNCTION(BlueprintCallable, Category = "Spawn Group")
	void SpawnFromRandomFiltered(int32 Count, float Interval, const TArray<TSubclassOf<AActor>>& ClassFilter);

	UFUNCTION(BlueprintCallable, Category = "Spawn Group")
	void SpawnFromAllFiltered(float Interval, const TArray<TSubclassOf<AActor>>& ClassFilter);

	UFUNCTION(BlueprintCallable, Category = "Spawn Group", BlueprintPure)
	int32 GetActiveCount() const;

	UPROPERTY(BlueprintAssignable, Category = "Spawn Group|Events")
	FOnGroupCleared OnGroupCleared;

	UPROPERTY(BlueprintAssignable, Category = "Spawn Group|Events")
	FOnGroupSpawnComplete OnGroupSpawnComplete;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	struct FSpawnQueueEntry
	{
		ASpawnPoint* Point = nullptr;
		TSubclassOf<AActor> OverrideClass = nullptr;
	};

	void ExecuteNextSpawn();
	void CheckGroupCleared();

	UFUNCTION()
	void OnPointActorSpawned(AActor* Actor);

	UFUNCTION()
	void OnPointActorDespawned(AActor* Actor);

	TArray<FSpawnQueueEntry> SpawnQueue;
	FTimerHandle SpawnTimerHandle;
	float CurrentSpawnInterval = 0.f;
	int32 CompletedSpawnCount = 0;
	int32 TotalGroupSpawns = 0;
};
