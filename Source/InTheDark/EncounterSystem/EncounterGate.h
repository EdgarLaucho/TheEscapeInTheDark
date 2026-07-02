#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EncounterGate.generated.h"

class UStaticMeshComponent;
class UNiagaraComponent;

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

	UFUNCTION(BlueprintCallable, Category = "Gate")
	void FinishUnlock();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate|Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gate")
	bool bStartLocked = false;

private:
	UPROPERTY(Transient)
	bool bLocked = false;

	UPROPERTY(Transient)
	FVector ClosedRelativeLocation = FVector::ZeroVector;

	FTimerHandle GateMoveTimerHandle;
	FVector MoveStartRelativeLocation = FVector::ZeroVector;
	FVector MoveTargetRelativeLocation = FVector::ZeroVector;
	float MoveElapsedSeconds = 0.f;
	float MoveDurationSeconds = 0.f;

	void ApplyLockState();
	void SnapGateToState(bool bClosed);
	void StartGateMove(bool bClosed);
	void UpdateGateMove();
	void SetGateCollisionEnabled(bool bEnabled);
	FVector GetOpenRelativeLocation() const;
	float GetConfiguredSinkDepthOffset() const;
	float GetConfiguredSinkTime() const;
};