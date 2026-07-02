#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EncounterSystem/SpawnAreaTypes.h"
#include "SpawnArea.generated.h"

class USphereComponent;
class UBillboardComponent;
class ASpawnAnchor;
class UObjectPoolSubsystem;

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

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "SpawnArea|Anchors")
	TArray<TObjectPtr<ASpawnAnchor>> BoundAnchors;

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
	int32 SpawnQuota = 0;

	FTimerHandle SpawnTimerHandle;
	FTimerHandle LeashTimerHandle;
	FTimerHandle DespawnCheckHandle;

	void Activate();
	void Deactivate();
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