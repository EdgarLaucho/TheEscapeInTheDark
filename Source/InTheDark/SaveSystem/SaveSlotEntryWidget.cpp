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
	const FLinearColor CardBG      = FLinearColor(0.055f, 0.040f, 0.028f, 0.95f);
	const FLinearColor CardBGHover = FLinearColor(0.085f, 0.062f, 0.042f, 0.95f);
	const FLinearColor Gold        = FLinearColor(0.88f,  0.70f,  0.28f,  1.f);
	const FLinearColor White       = FLinearColor(0.92f,  0.88f,  0.80f,  1.f);
	const FLinearColor Gray        = FLinearColor(0.52f,  0.46f,  0.36f,  1.f);
	const FLinearColor BtnN        = FLinearColor(0.100f, 0.075f, 0.050f, 1.f);
	const FLinearColor BtnH        = FLinearColor(0.220f, 0.165f, 0.090f, 1.f);
	const FLinearColor BtnP        = FLinearColor(0.050f, 0.038f, 0.025f, 1.f);
	const FLinearColor DangerN     = FLinearColor(0.22f,  0.04f,  0.04f,  1.f);
	const FLinearColor DangerH     = FLinearColor(0.45f,  0.07f,  0.07f,  1.f);
	const FLinearColor DangerP     = FLinearColor(0.11f,  0.02f,  0.02f,  1.f);
	const FLinearColor ConfirmN    = FLinearColor(0.55f,  0.42f,  0.00f,  1.f);
	const FLinearColor ConfirmH    = FLinearColor(0.80f,  0.62f,  0.00f,  1.f);
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

	// Outline border (creates card border effect)
	UBorder* OutlineBdr = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("Outline"));
	OutlineBdr->SetBrushColor(FLinearColor(0.24f, 0.16f, 0.06f, 0.85f));
	OutlineBdr->SetPadding(FMargin(1.f));
	WidgetTree->RootWidget = OutlineBdr;

	// Card background
	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("Card"));
	Card->SetBrushColor(SlotColors::CardBG);
	Card->SetPadding(FMargin(16.f, 14.f));
	OutlineBdr->SetContent(Card);

	// Main row: info | buttons
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Card->SetContent(Row);

	// Left: text info
	UVerticalBox* InfoCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	TXT_SlotTitle = MakeText(TEXT("SLOT 1"), 13, SlotColors::Gold, true);
	TXT_Status    = MakeText(TEXT("New Game"), 11, SlotColors::White);
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
	BTN_Play  = MakeBtn(TEXT("PLAY"),   false);
	BTN_Reset = MakeBtn(TEXT("DELETE"), true);

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
		FString::Printf(TEXT("SLOT  %d"), Info.SlotIndex + 1)));

	if (bIsEmpty)
	{
		TXT_Status->SetText(FText::FromString(TEXT("New Game")));
		TXT_Date->SetVisibility(ESlateVisibility::Collapsed);
		BTN_Reset->SetVisibility(ESlateVisibility::Collapsed);

		// Reset BORRAR label in case it was in confirm state
		if (UTextBlock* Lbl = Cast<UTextBlock>(BTN_Reset->GetChildAt(0)))
			Lbl->SetText(FText::FromString(TEXT("DELETE")));
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
			Lbl->SetText(FText::FromString(TEXT("CONFIRM?")));

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
