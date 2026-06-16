#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class USaveSlotEntryWidget;
class UVerticalBox;
class UTextBlock;
class UButton;

UCLASS()
class INTHEDARK_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void OnSlotSelected(int32 SlotIndex, bool bIsEmpty);
	void OnSlotReset(int32 SlotIndex);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

private:
	UPROPERTY() TObjectPtr<UVerticalBox> MainPanel;
	UPROPERTY() TObjectPtr<UVerticalBox> SlotPanel;
	UPROPERTY() TObjectPtr<UButton> BTN_Play;
	UPROPERTY() TObjectPtr<UButton> BTN_Quit;
	UPROPERTY() TObjectPtr<UButton> BTN_Back;
	UPROPERTY() TArray<TObjectPtr<USaveSlotEntryWidget>> SlotWidgets;

	static const FName DefaultGameLevel;

	void BuildLayout();
	void RefreshSlots();
	void ShowMainPanel();
	void ShowSlotPanel();

	UFUNCTION() void HandlePlay();
	UFUNCTION() void HandleQuit();
	UFUNCTION() void HandleBack();

	UTextBlock* MakeText(const FString& Txt, int32 Size, const FLinearColor& Color, bool bBold = false);
	UButton*    MakeNavButton(const FString& Label, bool bDanger = false);
};
