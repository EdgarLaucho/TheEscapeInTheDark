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

	UFUNCTION(BlueprintCallable, Category = "Gate")
	void SetLockedInstant(bool bNewLocked);

	UFUNCTION(BlueprintPure, Category = "Gate")
	bool IsLocked() const { return bLocked; }

	UFUNCTION(BlueprintPure, Category = "Gate")
	FVector GetOpenRelativeLocation() const;

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

protected:
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Gate")
	FVector ClosedRelativeLocation = FVector::ZeroVector;

private:
	UPROPERTY(Transient)
	bool bLocked = false;

	void ApplyLockState();
	void SnapGateToState(bool bClosed);
	void SetGateCollisionEnabled(bool bEnabled);
};