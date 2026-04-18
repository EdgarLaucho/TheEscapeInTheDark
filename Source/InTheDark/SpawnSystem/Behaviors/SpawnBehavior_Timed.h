#pragma once

#include "CoreMinimal.h"
#include "SpawnSystem/SpawnBehavior.h"
#include "SpawnBehavior_Timed.generated.h"

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Timed"))
class INTHEDARK_API USpawnBehavior_Timed : public USpawnBehavior
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timed", meta = (ClampMin = "0.1"))
	float SpawnInterval = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timed", meta = (ClampMin = "1"))
	int32 MaxActiveFromThisPoint = 3;

protected:
	virtual void ActivateBehavior_Implementation() override;
	virtual void DeactivateBehavior_Implementation() override;
	virtual void OnSpawnedActorReleased_Implementation(AActor* Actor) override;

private:
	void OnTimerTick();

	FTimerHandle SpawnTimerHandle;
};
