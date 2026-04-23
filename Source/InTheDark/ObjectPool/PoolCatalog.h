#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ObjectPool/ObjectPoolTypes.h"
#include "PoolCatalog.generated.h"

UCLASS(BlueprintType)
class INTHEDARK_API UPoolCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool Catalog")
	TArray<FPoolCatalogEntry> Entries;
};
