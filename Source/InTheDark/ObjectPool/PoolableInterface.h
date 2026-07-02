#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PoolableInterface.generated.h"

UINTERFACE(Blueprintable, meta = (DisplayName = "Poolable"))
class INTHEDARK_API UPoolableInterface : public UInterface
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