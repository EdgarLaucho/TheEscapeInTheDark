#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ObjectPool/ObjectPoolTypes.h"
#include "SpawnPoint.generated.h"

class USpawnBehavior;
class UObjectPoolSubsystem;
class UBillboardComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnPointActorSpawned, AActor*, SpawnedActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnPointActorDespawned, AActor*, ReleasedActor);

UCLASS(BlueprintType, Blueprintable)
class INTHEDARK_API ASpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ASpawnPoint();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Point")
	TArray<FSpawnClassEntry> SpawnClasses;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Spawn Point")
	TObjectPtr<USpawnBehavior> Behavior;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Point")
	int32 MaxSpawnCount = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Point")
	bool bAutoActivate = false;

	UFUNCTION(BlueprintCallable, Category = "Spawn Point")
	AActor* RequestSpawn();

	UFUNCTION(BlueprintCallable, Category = "Spawn Point")
	AActor* RequestSpawnOfClass(TSubclassOf<AActor> OverrideClass);

	UFUNCTION(BlueprintCallable, Category = "Spawn Point", BlueprintPure)
	int32 GetActiveSpawnCount() const;

	UFUNCTION(BlueprintCallable, Category = "Spawn Point", BlueprintPure)
	int32 GetTotalSpawnCount() const { return CurrentSpawnCount; }

	UFUNCTION(BlueprintCallable, Category = "Spawn Point")
	void Activate();

	UFUNCTION(BlueprintCallable, Category = "Spawn Point")
	void Deactivate();

	UFUNCTION(BlueprintCallable, Category = "Spawn Point", BlueprintPure)
	bool IsPointActive() const { return bIsActive; }

	UPROPERTY(BlueprintAssignable, Category = "Spawn Point|Events")
	FOnSpawnPointActorSpawned OnActorSpawned;

	UPROPERTY(BlueprintAssignable, Category = "Spawn Point|Events")
	FOnSpawnPointActorDespawned OnActorDespawned;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> EditorBillboard;
#endif

private:
	TSubclassOf<AActor> SelectWeightedClass() const;

	UFUNCTION()
	void OnPoolActorReleased(AActor* Actor);

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	int32 CurrentSpawnCount = 0;
	bool bIsActive = false;
};
