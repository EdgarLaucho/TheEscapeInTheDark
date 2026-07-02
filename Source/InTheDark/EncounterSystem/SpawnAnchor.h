#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "EncounterSystem/EncounterTypes.h"
#include "SpawnAnchor.generated.h"

class UBillboardComponent;
class UArrowComponent;
class UNiagaraSystem;
class USoundBase;

UCLASS(Blueprintable, BlueprintType)
class INTHEDARK_API ASpawnAnchor : public AActor
{
	GENERATED_BODY()

public:
	ASpawnAnchor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor")
	FGameplayTagContainer AnchorTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Telegraph")
	TSoftObjectPtr<UNiagaraSystem> DefaultPreSpawnVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Telegraph")
	TSoftObjectPtr<USoundBase> DefaultPreSpawnSFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Telegraph", meta = (ClampMin = "0.0"))
	float DefaultLeadTime = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility")
	bool bBlockIfPlayerInFOV = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility", meta = (ClampMin = "0.0", ClampMax = "180.0", EditCondition = "bBlockIfPlayerInFOV"))
	float PlayerFOVAngleDegrees = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility", meta = (ClampMin = "0.0"))
	float MinDistanceToPlayer = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0"))
	float PointCooldown = 2.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Timing")
	float LastSpawnTimeSeconds = -1e9f;

	bool IsAvailableForSpawn(const AActor* PlayerActor) const;
	bool IsSpawnLocationOccupied(TSubclassOf<AActor> EnemyClass) const;
	AActor* PerformSpawn(TSubclassOf<AActor> EnemyClass, const FEnemySpawn& Directive);

	float ResolveLead(const FEnemySpawn& Directive) const;
	void PlayTelegraph(const FEnemySpawn& Directive) const;

protected:
	virtual void BeginPlay() override;

private:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Billboard;

	UPROPERTY()
	TObjectPtr<UArrowComponent> Arrow;
#endif
};
