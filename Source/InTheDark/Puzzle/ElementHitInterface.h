#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ElementHitInterface.generated.h"

UINTERFACE(Blueprintable, meta = (DisplayName = "Element Hit Target"))
class INTHEDARK_API UElementHitInterface : public UInterface
{
	GENERATED_BODY()
};

class INTHEDARK_API IElementHitInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Element")
	void OnElementHit(FName ElementName, AActor* HitInstigator);
};
