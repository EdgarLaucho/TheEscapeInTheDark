#include "AI/LeashComponent.h"

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
}

void ULeashComponent::DeactivateLeash()
{
	if (!bLeashActive) { return; }

	bLeashActive = false;
	LeashTarget = FVector::ZeroVector;
	OnLeashDeactivated.Broadcast();
}
