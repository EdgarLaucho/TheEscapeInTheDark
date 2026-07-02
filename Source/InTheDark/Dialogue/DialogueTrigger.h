#pragma once

#include "CoreMinimal.h"
#include "Dialogue/DialogueData.h"
#include "GameFramework/Actor.h"
#include "DialogueTrigger.generated.h"

class UBoxComponent;
class USceneComponent;

UCLASS(Blueprintable, BlueprintType)
class INTHEDARK_API ADialogueTrigger : public AActor
{
	GENERATED_BODY()

public:
	ADialogueTrigger();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Dialogue")
	FName DialogueID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UDialogueData> DialogueData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	bool bTriggerOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	bool bSkipIfSeen = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	bool bMarkSeenOnComplete = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	bool bBlockMovementDuringDialogue = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue", meta = (ClampMin = "0.0"))
	float LineHoldSeconds = 2.0f;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	bool bTriggeredThisSession = false;
};