#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Dialogue/DialogueData.h"
#include "DialogueWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueAdvance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueLineFinished);

UCLASS(Abstract)
class INTHEDARK_API UDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
	void ShowLine(const FDialogueLine& Line);

	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
	void SetClickToAdvanceEnabled(bool bEnabled);

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Dialogue")
	FOnDialogueAdvance OnAdvanceRequested;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Dialogue")
	FOnDialogueLineFinished OnLineFinishedRevealing;
};