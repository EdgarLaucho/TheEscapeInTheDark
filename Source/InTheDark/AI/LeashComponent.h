#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LeashComponent.generated.h"

/*
 * ULeashComponent — componente que el SpawnArea usa para forzar el retorno
 * de un enemigo al área sin interferir con la lógica del AIController.
 *
 * Uso:
 *   - SpawnArea llama ActivateLeash(Target) / DeactivateLeash() desde C++.
 *   - El AIController BP comprueba IsLeashActive() al inicio de su Tick
 *     y llama MoveToLocation(GetLeashTarget()) si está activo.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeashDeactivated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeashActivated);

UCLASS(ClassGroup = AI, meta = (BlueprintSpawnableComponent),
       DisplayName = "Leash Component")
class INTHEDARK_API ULeashComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULeashComponent();

	// Activa el leash y establece el destino de retorno.
	void ActivateLeash(const FVector& Target);

	// Desactiva el leash; el controlador retoma su comportamiento normal.
	void DeactivateLeash();

	// Se emite cuando el leash se activa (jugador sale del área).
	UPROPERTY(BlueprintAssignable, Category = "Leash")
	FOnLeashActivated OnLeashActivated;

	// Se emite cuando el leash pasa de activo a inactivo.
	UPROPERTY(BlueprintAssignable, Category = "Leash")
	FOnLeashDeactivated OnLeashDeactivated;

	// Devuelve el LeashComponent de un actor. Úsalo en BP para obtener el target tipado.
	UFUNCTION(BlueprintPure, Category = "Leash", meta = (DefaultToSelf = "Actor"))
	static ULeashComponent* GetLeashComponent(AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "Leash")
	bool IsLeashActive() const { return bLeashActive; }

	UFUNCTION(BlueprintPure, Category = "Leash")
	FVector GetLeashTarget() const { return LeashTarget; }

private:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Leash",
	          meta = (AllowPrivateAccess = "true"))
	bool bLeashActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Leash",
	          meta = (AllowPrivateAccess = "true"))
	FVector LeashTarget = FVector::ZeroVector;
};
