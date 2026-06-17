#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dialogue/DialogueData.h"
#include "DialogueSubsystem.generated.h"

class UDialogueWidget;
class APlayerController;

UCLASS()
class INTHEDARK_API UDialogueSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Inicia el diálogo. No hace nada si ya hay uno activo o si DialogueData está vacío. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void StartDialogue(UDialogueData* Data, FName DialogueID, APlayerController* PC);

	/** Avanza a la siguiente línea. Si era la última, cierra el diálogo. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void AdvanceLine();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsDialogueActive() const { return bActive; }

private:
	void EndDialogue();

	UFUNCTION()
	void HandleAdvanceRequested();

	void SaveAndMarkSeen();

	UPROPERTY()
	TObjectPtr<UDialogueWidget> Widget;

	UPROPERTY()
	TObjectPtr<UDialogueData> ActiveData;

	UPROPERTY()
	TObjectPtr<APlayerController> ActivePC;

	FName ActiveDialogueID;
	int32 CurrentLineIndex = 0;
	bool  bActive          = false;
};
