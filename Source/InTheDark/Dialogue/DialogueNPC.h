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

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Dialogue")
	FName DialogueID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UDialogueData> DialogueData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<USphereComponent> TriggerZone;

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void TriggerDialogue();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool CanTriggerDialogue() const;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	bool bPlayerInRange = false;
	bool bTriggeredThisSession = false;
};