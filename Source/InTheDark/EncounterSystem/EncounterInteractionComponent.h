#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EncounterInteractionComponent.generated.h"

class ACombatArena;

/**
 * Se adjunta a un actor de runa/altar. Cuando el sistema de interacción llama a TriggerStart,
 * lo reenvía al ACombatArena objetivo.
 *
 * Puede adjuntarse como hijo de ACombatArena (auto-target) o a un altar independiente
 * en otro lugar del nivel que activa un arena lejano.
 */
UCLASS(ClassGroup = (Encounter), meta = (BlueprintSpawnableComponent))
class INTHEDARK_API UEncounterInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEncounterInteractionComponent();

	/** Arena a iniciar. Si es null, usa el owner si es ACombatArena. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Encounter")
	TObjectPtr<ACombatArena> TargetArena;

	/** Llama desde el nodo BP_Interface::Interact de la runa/altar. */
	UFUNCTION(BlueprintCallable, Category = "Encounter")
	void TriggerStart();
};
