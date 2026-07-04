#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpawnArea.generated.h"

class USphereComponent;
class UBillboardComponent;
class UObjectPoolSubsystem;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Despawn")
	bool bRequireOutOfSightToDespawn = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Despawn", meta = (ClampMin = 10.f, ClampMax = 180.f))
	float VisibilityConeHalfAngle = 60.f;

};

UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Spawn Area"))
class INTHEDARK_API ASpawnArea : public AActor
{
	GENERATED_BODY()

public:
	ASpawnArea();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnArea|Enemies")
	TArray<FSpawnAreaEntry> EnemyEntries;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnArea|Rules")
	FSpawnAreaRules Rules;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "SpawnArea|Components")
	TObjectPtr<USphereComponent> ActivationVolume;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Billboard;
#endif

	bool bPlayerInside = false;
	TArray<TWeakObjectPtr<AActor>> ActiveEnemies;

	FTimerHandle SpawnTimerHandle;
	FTimerHandle LeashTimerHandle;
	FTimerHandle DespawnCheckHandle;

	void Activate();
	void Deactivate();
	void WarmUpEnemyPools();
	void SetPlayerInside(bool bNewPlayerInside);
	void RefreshPlayerInsideState();
	bool IsPlayerInsideArea() const;
	void DespawnAll();

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* Comp, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Hit);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* Comp, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex);

	UFUNCTION()
	void OnEnemyDestroyed(AActor* DestroyedActor);

	void TrySpawn();
	AActor* SpawnEnemy(TSubclassOf<AActor> EnemyClass);
	void RegisterSpawnedEnemy(AActor* Spawned);
	void EnforceLeash();
	void CheckDespawnOnLeave();
	void ReleaseEnemy(AActor* Enemy);
	void ApplyLeashState(AActor* Enemy) const;
	void ApplyLeashStateToActiveEnemies() const;
	bool IsVisibleToPlayer(const AActor* Enemy) const;
	bool GetRandomSpawnTransform(FTransform& OutTransform) const;
	TSubclassOf<AActor> PickEnemyClass() const;
	int32 CountActiveOfClass(TSubclassOf<AActor> Class) const;
	void CleanDeadEntries();
	AActor* GetPlayerActor() const;
	UObjectPoolSubsystem* GetPool() const;
};
