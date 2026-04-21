#include "EncounterSystem/EncounterInteractionComponent.h"
#include "EncounterSystem/CombatArena.h"

UEncounterInteractionComponent::UEncounterInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEncounterInteractionComponent::TriggerStart()
{
	ACombatArena* Arena = TargetArena;
	if (!Arena)
	{
		Arena = Cast<ACombatArena>(GetOwner());
	}
	if (!Arena)
	{
		UE_LOG(LogTemp, Warning, TEXT("UEncounterInteractionComponent::TriggerStart: no TargetArena and owner is not ACombatArena (%s)"),
			*GetOwner()->GetName());
		return;
	}
	Arena->RequestStart();
}
