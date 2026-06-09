#include "AI/LeashComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"

ULeashComponent::ULeashComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

ULeashComponent* ULeashComponent::GetLeashComponent(AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<ULeashComponent>() : nullptr;
}

void ULeashComponent::ActivateLeash(const FVector& Target)
{
	LeashTarget = Target;
	bLeashActive = true;

	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (AAIController* AIC = Cast<AAIController>(Pawn->GetController()))
		{
			AIC->StopMovement();
			// Limpiar el foco para que el enemigo deje de mirar al jugador.
			AIC->ClearFocus(EAIFocusPriority::Gameplay);
		}
	}

	OnLeashActivated.Broadcast();
}

void ULeashComponent::DeactivateLeash()
{
	if (!bLeashActive) { return; }

	bLeashActive = false;
	LeashTarget = FVector::ZeroVector;

	// Cancelar el MoveToLocation activo para que el BehaviorTree
	// arranque desde cero en su siguiente tick en lugar de quedarse quieto.
	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (AAIController* AIC = Cast<AAIController>(Pawn->GetController()))
		{
			AIC->StopMovement();
		}
	}

	OnLeashDeactivated.Broadcast();
}
