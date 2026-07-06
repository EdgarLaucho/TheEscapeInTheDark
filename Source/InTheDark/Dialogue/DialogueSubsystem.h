#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dialogue/DialogueData.h"
#include "DialogueSubsystem.generated.h"

class UDialogueWidget;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueEnded, FName, DialogueID);

enum class EDialoguePlaybackMode : uint8
{
	Interactive,
	Ambient
};

UCLASS()
class INTHEDARK_API UDialogueSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void StartDialogue(UDialogueData* Data, FName DialogueID, APlayerController* PC, float LineHoldSeconds = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void StartAmbientDialogue(UDialogueData* Data, FName DialogueID, APlayerController* PC, bool bMarkSeen = true, float LineHoldSeconds = 2.0f, bool bBlockMovement = false);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void AdvanceLine();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsDialogueActive() const { return bActive; }

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnDialogueEnded OnDialogueEnded;

private:
	void StartDialogueWithMode(UDialogueData* Data, FName DialogueID, APlayerController* PC, EDialoguePlaybackMode PlaybackMode, bool bMarkSeen, float LineHoldSeconds, bool bBlockMovement);

	void EndDialogue();

	UFUNCTION()
	void HandleAdvanceRequested();

	UFUNCTION()
	void HandleLineFinishedRevealing();

	float GetHoldSecondsForCurrentLine() const;

	void SaveAndMarkSeen();

	UPROPERTY(Transient)
	TSubclassOf<UDialogueWidget> WidgetClass;

	UPROPERTY()
	TObjectPtr<UDialogueWidget> Widget;

	UPROPERTY()
	TObjectPtr<UDialogueData> ActiveData;

	UPROPERTY()
	TObjectPtr<APlayerController> ActivePC;

	FName ActiveDialogueID;
	int32 CurrentLineIndex = 0;
	bool bActive = false;
	bool bMarkSeenOnEnd = true;
	bool bSaveCheckpointOnEnd = true;
	bool bBlockedMovementForActiveDialogue = false;
	EDialoguePlaybackMode ActivePlaybackMode = EDialoguePlaybackMode::Interactive;
	FTimerHandle AmbientAdvanceTimer;
	float ActiveAmbientLineHoldSeconds = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Dialogue|Ambient")
	float DefaultAmbientLineHoldSeconds = 2.0f;
};