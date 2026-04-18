#pragma once

#include "CoreMinimal.h"
#include "SpawnSystem/SpawnBehavior.h"
#include "SpawnBehavior_Proximity.generated.h"

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Proximity"))
class INTHEDARK_API USpawnBehavior_Proximity : public USpawnBehavior
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Proximity", meta = (ClampMin = "1.0"))
	float TriggerRadius = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Proximity")
	bool bOneShot = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Proximity", meta = (EditCondition = "!bOneShot", ClampMin = "0.0"))
	float RespawnDelay = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Proximity", meta = (ClampMin = "1"))
	int32 SpawnCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Proximity", meta = (ClampMin = "0.05"))
	float CheckInterval = 0.2f;

protected:
	virtual void ActivateBehavior_Implementation() override;
	virtual void DeactivateBehavior_Implementation() override;
	virtual void OnSpawnedActorReleased_Implementation(AActor* Actor) override;

private:
	void CheckProximity();
	void OnRespawnReady();

	FTimerHandle CheckTimerHandle;
	FTimerHandle RespawnTimerHandle;
	bool bHasTriggered = false;
	bool bWaitingRespawn = false;
};
