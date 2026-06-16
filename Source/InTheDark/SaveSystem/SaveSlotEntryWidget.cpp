#include "SaveSystem/SaveSlotEntryWidget.h"
#include "SaveSystem/MainMenuWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateBrush.h"
#include "Styling/CoreStyle.h"

namespace SlotColors
{
	const FLinearColor CardBG      = FLinearColor(0.06f, 0.05f, 0.09f, 0.95f);
	const FLinearColor CardBGHover = FLinearColor(0.09f, 0.08f, 0.14f, 0.95f);
	const FLinearColor Gold        = FLinearColor(0.85f, 0.68f, 0.25f, 1.f);
	const FLinearColor White       = FLinearColor(0.9f,  0.88f, 0.85f, 1.f);
	const FLinearColor Gray        = FLinearColor(0.5f,  0.48f, 0.45f, 1.f);
	const FLinearColor BtnN        = FLinearColor(0.14f, 0.11f, 0.20f, 1.f);
	const FLinearColor BtnH        = FLinearColor(0.28f, 0.23f, 0.42f, 1.f);
	const FLinearColor BtnP        = FLinearColor(0.07f, 0.06f, 0.11f, 1.f);
	const FLinearColor DangerN     = FLinearColor(0.28f, 0.04f, 0.04f, 1.f);
	const FLinearColor DangerH     = FLinearColor(0.55f, 0.07f, 0.07f, 1.f);
	const FLinearColor DangerP     = FLinearColor(0.14f, 0.02f, 0.02f, 1.f);
	const FLinearColor ConfirmN    = FLinearColor(0.6f,  0.45f, 0.0f,  1.f);
	const FLinearColor ConfirmH    = FLinearColor(0.85f, 0.65f, 0.0f,  1.f);
}

UTextBlock* USaveSlotEntryWidget::MakeText(const FString& Txt, int32 Size, const FLinearColor& Color, bool bBold)
{
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(FText::FromString(Txt));
	T->SetColorAndOpacity(FSlateColor(Color));
	T->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size));
	return T;
}

UButton* USaveSlotEntryWidget::MakeBtn(const FString& Label, bool bDanger)
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());

	UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Lbl->SetText(FText::FromString(Label));
	Lbl->SetColorAndOpacity(FSlateColor(SlotColors::White));
	Lbl->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	Lbl->SetJustification(ETextJustify::Center);
	Btn->AddChild(Lbl);

	const FLinearColor N = bDanger ? SlotColors::DangerN : SlotColors::BtnN;
	const FLinearColor H = bDanger ? SlotColors::DangerH : SlotColors::BtnH;
	const FLinearColor P = bDanger ? SlotColors::DangerP : SlotColors::BtnP;

	FButtonStyle Style;
	Style.Normal  = FSlateColorBrush(FSlateColor(N));
	Style.Hovered = FSlateColorBrush(FSlateColor(H));
	Style.Pressed = FSlateColorBrush(FSlateColor(P));
	Style.NormalPadding  = FMargin(12.f, 7.f);
	Style.PressedPadding = FMargin(12.f, 8.f, 12.f, 6.f);
	Btn->SetStyle(Style);
	return Btn;
}

void USaveSlotEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Card background
	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("Card"));
	Card->SetBrushColor(SlotColors::CardBG);
	Card->SetPadding(FMargin(16.f, 12.f));
	WidgetTree->RootWidget = Card;

	// Main row: info | buttons
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Card->SetContent(Row);

	// Left: text info
	UVerticalBox* InfoCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	TXT_SlotTitle = MakeText(TEXT("RANURA 1"), 13, SlotColors::Gold, true);
	TXT_Status    = MakeText(TEXT("Nueva partida"), 11, SlotColors::White);
	TXT_Status->SetAutoWrapText(true);
	TXT_Date      = MakeText(TEXT(""), 10, SlotColors::Gray);

	InfoCol->AddChildToVerticalBox(TXT_SlotTitle);
	if (UVerticalBoxSlot* S = InfoCol->AddChildToVerticalBox(TXT_Status))
		S->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
	if (UVerticalBoxSlot* S = InfoCol->AddChildToVerticalBox(TXT_Date))
		S->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));

	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(InfoCol))
	{
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetVerticalAlignment(VAlign_Center);
	}

	// Right: buttons column
	UVerticalBox* BtnCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	BTN_Play  = MakeBtn(TEXT("JUGAR"),  false);
	BTN_Reset = MakeBtn(TEXT("BORRAR"), true);

	BtnCol->AddChildToVerticalBox(BTN_Play);
	if (UVerticalBoxSlot* S = BtnCol->AddChildToVerticalBox(BTN_Reset))
		S->SetPadding(FMargin(0.f, 5.f, 0.f, 0.f));

	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(BtnCol))
	{
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	}

	BTN_Play->OnClicked.AddDynamic(this,  &USaveSlotEntryWidget::HandlePlayClicked);
	BTN_Reset->OnClicked.AddDynamic(this, &USaveSlotEntryWidget::HandleResetClicked);
}

void USaveSlotEntryWidget::InitSlot(const FSaveSlotInfo& Info, UMainMenuWidget* InParentMenu)
{
	ParentMenu     = InParentMenu;
	SlotIndex      = Info.SlotIndex;
	bIsEmpty       = Info.bIsEmpty;
	bAwaitingConfirm = false;

	TXT_SlotTitle->SetText(FText::FromString(
		FString::Printf(TEXT("RANURA  %d"), Info.SlotIndex + 1)));

	if (bIsEmpty)
	{
		TXT_Status->SetText(FText::FromString(TEXT("Nueva partida")));
		TXT_Date->SetVisibility(ESlateVisibility::Collapsed);
		BTN_Reset->SetVisibility(ESlateVisibility::Collapsed);

		// Reset BORRAR label in case it was in confirm state
		if (UTextBlock* Lbl = Cast<UTextBlock>(BTN_Reset->GetChildAt(0)))
			Lbl->SetText(FText::FromString(TEXT("BORRAR")));
	}
	else
	{
		FString DisplayName = Info.DisplayMapName;
		const int32 MaxLen = 22;
		if (DisplayName.Len() > MaxLen)
			DisplayName = DisplayName.Left(MaxLen - 3) + TEXT("...");
		TXT_Status->SetText(FText::FromString(DisplayName));
		const FString DateStr = FString::Printf(TEXT("%02d/%02d/%04d  %02d:%02d"),
			Info.SavedAt.GetDay(), Info.SavedAt.GetMonth(), Info.SavedAt.GetYear(),
			Info.SavedAt.GetHour(), Info.SavedAt.GetMinute());
		TXT_Date->SetText(FText::FromString(DateStr));
		TXT_Date->SetVisibility(ESlateVisibility::Visible);
		BTN_Reset->SetVisibility(ESlateVisibility::Visible);
	}
}

void USaveSlotEntryWidget::HandlePlayClicked()
{
	if (ParentMenu) ParentMenu->OnSlotSelected(SlotIndex, bIsEmpty);
}

void USaveSlotEntryWidget::HandleResetClicked()
{
	if (!ParentMenu) return;

	if (!bAwaitingConfirm)
	{
		// First click: ask for confirmation
		bAwaitingConfirm = true;
		if (UTextBlock* Lbl = Cast<UTextBlock>(BTN_Reset->GetChildAt(0)))
		{
			Lbl->SetText(FText::FromString(TEXT("¿CONFIRMAR?")));

			FButtonStyle Style = BTN_Reset->GetStyle();
			Style.Normal  = FSlateColorBrush(FSlateColor(SlotColors::ConfirmN));
			Style.Hovered = FSlateColorBrush(FSlateColor(SlotColors::ConfirmH));
			BTN_Reset->SetStyle(Style);
		}
	}
	else
	{
		// Second click: actually delete
		bAwaitingConfirm = false;
		ParentMenu->OnSlotReset(SlotIndex);
	}
}
