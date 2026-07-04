#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/ElementHitInterface.h"
#include "ElementalSwitch.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class AElementalGateDoor;

UCLASS(Blueprintable, BlueprintType)
class INTHEDARK_API AElementalSwitch : public AActor, public IElementHitInterface
{
	GENERATED_BODY()

public:
	AElementalSwitch();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Switch")
	FName RequiredElement;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Switch")
	TObjectPtr<AElementalGateDoor> OwningDoor;

	UFUNCTION(BlueprintPure, Category = "Switch")
	bool IsActivated() const { return bActivated; }

	virtual void OnElementHit_Implementation(FName ElementName, AActor* HitInstigator) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Switch")
	void OnSwitchActivated();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Switch|Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Switch|Components")
	TObjectPtr<UBoxComponent> HitBox;

private:
	UPROPERTY(Transient)
	bool bActivated = false;
};
