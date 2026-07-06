#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EncounterSystem/EncounterTypes.h"
#include "EncounterConfig.generated.h"

UCLASS(BlueprintType)
class INTHEDARK_API UEncounterConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	TArray<FEncounterWave> Waves;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "0.0"))
	float PostClearBeatSeconds = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	bool bPersistCleared = true;
};