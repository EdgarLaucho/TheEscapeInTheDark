#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Dialogue/DialogueData.h"
#include "DialogueWidget.generated.h"

class UTextBlock;
class UButton;
class USizeBox;
class UOverlay;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueAdvance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueLineFinished);

UCLASS()
class INTHEDARK_API UDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowLine(const FDialogueLine& Line);
	void SetClickToAdvanceEnabled(bool bEnabled);

	UPROPERTY(BlueprintAssignable)
	FOnDialogueAdvance OnAdvanceRequested;

	UPROPERTY(BlueprintAssignable)
	FOnDialogueLineFinished OnLineFinishedRevealing;

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildLayout();

	UFUNCTION()
	void HandleClick();

	void TickTypewriter();
	void FinishTypewriter();

	UTextBlock* DialogueText  = nullptr;
	UTextBlock* HintText      = nullptr;
	UButton*    ClickCatcher  = nullptr;

	FString      FullLineText;
	int32        CurrentCharIndex  = 0;
	bool         bTypewriterActive = false;
	bool         bClickToAdvanceEnabled = true;
	FTimerHandle TypewriterTimer;

	static constexpr float TypewriterInterval = 0.035f;
};
