#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SaveSystem/SaveTypes.h"
#include "SaveSlotEntryWidget.generated.h"

class UTextBlock;
class UButton;
class UBorder;
class UMainMenuWidget;

UCLASS()
class INTHEDARK_API USaveSlotEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitSlot(const FSaveSlotInfo& Info, UMainMenuWidget* InParentMenu);

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY() TObjectPtr<UTextBlock> TXT_SlotTitle;
	UPROPERTY() TObjectPtr<UTextBlock> TXT_Status;
	UPROPERTY() TObjectPtr<UTextBlock> TXT_Date;
	UPROPERTY() TObjectPtr<UButton>    BTN_Play;
	UPROPERTY() TObjectPtr<UButton>    BTN_Reset;

	UPROPERTY() TObjectPtr<UMainMenuWidget> ParentMenu;
	int32 SlotIndex  = 0;
	bool  bIsEmpty   = true;
	bool  bAwaitingConfirm = false;

	UFUNCTION() void HandlePlayClicked();
	UFUNCTION() void HandleResetClicked();

	UTextBlock* MakeText(const FString& Txt, int32 Size, const FLinearColor& Color, bool bBold = false);
	UButton*    MakeBtn(const FString& Label, bool bDanger);
};
