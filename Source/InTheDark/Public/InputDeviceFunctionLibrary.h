#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InputDeviceFunctionLibrary.generated.h"

UCLASS()
class INTHEDARK_API UInputDeviceFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Input")
	static bool IsAnyGamepadConnected();
};