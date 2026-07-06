#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EncounterTargetInterface.generated.h"

UINTERFACE(Blueprintable, meta = (DisplayName = "Encounter Target"))
class INTHEDARK_API UEncounterTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class INTHEDARK_API IEncounterTargetInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Encounter")
	void OnEncounterSpawned(AActor* Player);
};