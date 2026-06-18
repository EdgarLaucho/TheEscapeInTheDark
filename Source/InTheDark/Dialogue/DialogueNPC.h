#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Dialogue/DialogueData.h"
#include "DialogueNPC.generated.h"

class USphereComponent;

UCLASS(Blueprintable, BlueprintType)
class INTHEDARK_API ADialogueNPC : public AActor
{
	GENERATED_BODY()

public:
	ADialogueNPC();

	/** ID único del diálogo. Se usa para marcar como visto y para el save.
	 *  NUNCA renombrar con saves activos en disco. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Dialogue")
	FName DialogueID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UDialogueData> DialogueData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<USphereComponent> TriggerZone;

	/** Llama desde el nodo BPI_Interactable::Interact del Blueprint derivado. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void TriggerDialogue();

	/** Usa en BPI_Interactable::CanInteract — false si el diálogo ya se vio o no hay data. */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool CanTriggerDialogue() const;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	bool bPlayerInRange         = false;
	bool bTriggeredThisSession  = false;
};
