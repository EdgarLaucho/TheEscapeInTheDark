#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dialogue/DialogueData.h"
#include "DialogueSubsystem.generated.h"

class UDialogueWidget;
class APlayerController;

UENUM(BlueprintType)
enum class EDialoguePlaybackMode : uint8
{
	Interactive UMETA(DisplayName = "Interactive"),
	Ambient UMETA(DisplayName = "Ambient")
};

UCLASS()
class INTHEDARK_API UDialogueSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Starts the classic click-through dialogue that locks player input. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void StartDialogue(UDialogueData* Data, FName DialogueID, APlayerController* PC);

	/** Starts a narrated dialogue that keeps gameplay input active and advances automatically. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void StartAmbientDialogue(UDialogueData* Data, FName DialogueID, APlayerController* PC,
		bool bMarkSeen = true, float LineHoldSeconds = 2.0f);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void StartDialogueWithMode(UDialogueData* Data, FName DialogueID, APlayerController* PC,
		EDialoguePlaybackMode PlaybackMode, bool bMarkSeen = true, float LineHoldSeconds = 2.0f);

	/** Advances to the next line. If this was the last line, closes the dialogue. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void AdvanceLine();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsDialogueActive() const { return bActive; }

private:
	void EndDialogue();

	UFUNCTION()
	void HandleAdvanceRequested();

	UFUNCTION()
	void HandleLineFinishedRevealing();

	void SaveAndMarkSeen();

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
	EDialoguePlaybackMode ActivePlaybackMode = EDialoguePlaybackMode::Interactive;
	FTimerHandle AmbientAdvanceTimer;
	float ActiveAmbientLineHoldSeconds = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Dialogue|Ambient")
	float DefaultAmbientLineHoldSeconds = 2.0f;
};
