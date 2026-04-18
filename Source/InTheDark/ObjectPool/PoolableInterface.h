#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PoolableInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable, meta = (DisplayName = "Poolable"))
class UPoolableInterface : public UInterface
{
	GENERATED_BODY()
};

class INTHEDARK_API IPoolableInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Object Pool")
	void OnAcquiredFromPool();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Object Pool")
	void OnReleasedToPool();
};
