#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SpawnBehavior.generated.h"

class ASpawnPoint;

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class INTHEDARK_API USpawnBehavior : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, Category = "Spawn Behavior")
	void InitializeBehavior(ASpawnPoint* InOwner);

	UFUNCTION(BlueprintNativeEvent, Category = "Spawn Behavior")
	void ActivateBehavior();

	UFUNCTION(BlueprintNativeEvent, Category = "Spawn Behavior")
	void DeactivateBehavior();

	UFUNCTION(BlueprintNativeEvent, Category = "Spawn Behavior")
	void OnSpawnedActorReleased(AActor* Actor);

	virtual UWorld* GetWorld() const override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Spawn Behavior")
	TObjectPtr<ASpawnPoint> OwnerSpawnPoint;
};
