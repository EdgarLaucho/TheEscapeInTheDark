#pragma once

#include "CoreMinimal.h"
#include "SpawnSystem/SpawnBehavior.h"
#include "SpawnBehavior_WaveControlled.generated.h"

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Wave Controlled"))
class INTHEDARK_API USpawnBehavior_WaveControlled : public USpawnBehavior
{
	GENERATED_BODY()

protected:
	virtual void ActivateBehavior_Implementation() override;
	virtual void DeactivateBehavior_Implementation() override;
};
