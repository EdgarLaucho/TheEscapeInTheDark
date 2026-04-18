#pragma once

#include "CoreMinimal.h"
#include "SpawnSystem/SpawnBehavior.h"
#include "SpawnBehavior_Triggered.generated.h"

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Triggered"))
class INTHEDARK_API USpawnBehavior_Triggered : public USpawnBehavior
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Triggered")
	bool bOneShot = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Triggered", meta = (ClampMin = "1"))
	int32 SpawnCountPerTrigger = 1;

	UFUNCTION(BlueprintCallable, Category = "Triggered")
	void Trigger();

protected:
	virtual void ActivateBehavior_Implementation() override;
	virtual void DeactivateBehavior_Implementation() override;

private:
	bool bHasTriggered = false;
	bool bIsEnabled = false;
};
