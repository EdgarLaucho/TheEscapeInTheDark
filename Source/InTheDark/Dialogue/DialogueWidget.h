#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Dialogue/DialogueData.h"
#include "DialogueWidget.generated.h"

class UTextBlock;
class UBorder;
class UButton;
class UVerticalBox;
class USizeBox;
class UOverlay;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueAdvance);

UCLASS()
class INTHEDARK_API UDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowLine(const FDialogueLine& Line);

	UPROPERTY(BlueprintAssignable)
	FOnDialogueAdvance OnAdvanceRequested;

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildLayout();

	UFUNCTION()
	void HandleClick();

	void TickTypewriter();
	void FinishTypewriter();

	UTextBlock* SpeakerText   = nullptr;
	UTextBlock* DialogueText  = nullptr;

	FString      FullLineText;
	int32        CurrentCharIndex  = 0;
	bool         bTypewriterActive = false;
	FTimerHandle TypewriterTimer;

	static constexpr float TypewriterInterval = 0.035f;
};
