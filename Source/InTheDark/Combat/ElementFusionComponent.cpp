#include "Combat/ElementFusionComponent.h"

UElementFusionComponent::UElementFusionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UElementFusionComponent::BeginPlay()
{
	Super::BeginPlay();
}


void UElementFusionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

bool UElementFusionComponent::CanActivateFusion() const
{
	return !bFusionActive && !bFusionOnCooldown;
}

void UElementFusionComponent::ActivateFusion(FName CurrentElement, FName FusionResultElement)
{
	if (!CanActivateFusion())
		return;

	BaseElementBeforeFusion = CurrentElement;
	CurrentFusionElement = FusionResultElement;
	bFusionActive = true;
}

FName UElementFusionComponent::GetActiveElementName() const
{
	if (bFusionActive)
	{
		return CurrentFusionElement;
	}
	return BaseElementBeforeFusion;
}

void UElementFusionComponent::EndFusion()
{
	bFusionActive = false;
	bFusionOnCooldown = true;
	CurrentFusionElement= NAME_None;
}

void UElementFusionComponent::EndFusionCooldown()
{
	bFusionOnCooldown = false;
}