#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ElementalGateDoor.generated.h"

class UStaticMeshComponent;
class AElementalSwitch;

UCLASS(Blueprintable, BlueprintType)
class INTHEDARK_API AElementalGateDoor : public AActor
{
	GENERATED_BODY()

public:
	AElementalGateDoor();

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Gate")
	FName DoorId;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Gate")
	TArray<TObjectPtr<AElementalSwitch>> Switches;

	UFUNCTION(BlueprintCallable, Category = "Gate")
	void NotifySwitchActivated();

	UFUNCTION(BlueprintPure, Category = "Gate")
	bool IsOpen() const { return bOpen; }

	UFUNCTION(BlueprintPure, Category = "Gate")
	FVector GetOpenRelativeLocation() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Gate")
	void OnDoorOpened();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate|Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gate", meta = (ClampMin = "0.0"))
	float SinkDepthOffset = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gate", meta = (ClampMin = "0.0"))
	float SinkTime = 0.8f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Gate")
	FVector ClosedRelativeLocation = FVector::ZeroVector;

private:
	UPROPERTY(Transient)
	bool bOpen = false;

	bool AreAllSwitchesActivated() const;
	void Open();
	void ApplyOpenState();
	void SnapGateToState(bool bClosed);
	void SetGateCollisionEnabled(bool bEnabled);
};
