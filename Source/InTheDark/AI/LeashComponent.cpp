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

	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (AAIController* AIC = Cast<AAIController>(Pawn->GetController()))
		{
			AIC->StopMovement();
		}
	}

	OnLeashDeactivated.Broadcast();
}
