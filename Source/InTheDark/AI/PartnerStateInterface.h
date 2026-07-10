#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PartnerStateInterface.generated.h"

UINTERFACE(Blueprintable, meta = (DisplayName = "Partner State"))
class INTHEDARK_API UPartnerStateInterface : public UInterface
{
	GENERATED_BODY()
};

class INTHEDARK_API IPartnerStateInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Partner")
	void ApplyPartnerState(uint8 NewState);
};
