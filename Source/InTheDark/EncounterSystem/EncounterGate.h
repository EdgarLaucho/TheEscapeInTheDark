#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EncounterGate.generated.h"

class UStaticMeshComponent;
class UNiagaraComponent;

/**
 * Barrera de bloqueo de paso que el encuentro cierra mientras hay una oleada activa.
 * Bloquea solo al jugador. La colision se controla aqui; la animacion visual
 * se implementa en el Blueprint derivado moviendo el mesh.
 */
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

	/** Hook de Blueprint para efectos visuales al cerrar (runas, niebla, etc). */
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

	/** Si es true, empieza bloqueado. Normalmente false; el arena lo bloquea al iniciar. */
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
