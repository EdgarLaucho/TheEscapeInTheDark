#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EncounterGate.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable, BlueprintType)
class INTHEDARK_API AEncounterGate : public AActor
{
	GENERATED_BODY()

public:
	AEncounterGate();

	UFUNCTION(BlueprintCallable, Category = "Gate")
	void Lock();

	UFUNCTION(BlueprintCallable, Category = "Gate")
	void Unlock();

	UFUNCTION(BlueprintPure, Category = "Gate")
	bool IsLocked() const { return bLocked; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Gate")
	void OnGateLocked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Gate")
	void OnGateUnlocked();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate|Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gate")
	bool bStartLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gate", meta = (ClampMin = "0.0"))
	float SinkDepthOffset = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gate", meta = (ClampMin = "0.0"))
	float SinkTime = 0.8f;

private:
	UPROPERTY(Transient)
	bool bLocked = false;

	UPROPERTY(Transient)
	FVector ClosedRelativeLocation = FVector::ZeroVector;

	void ApplyLockState();
	void SnapGateToState(bool bClosed);
	void SetGateCollisionEnabled(bool bEnabled);
	FVector GetOpenRelativeLocation() const;
};
