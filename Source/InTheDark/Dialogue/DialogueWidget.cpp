#include "Dialogue/DialogueWidget.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Engine/World.h"

void UDialogueWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
}

void UDialogueWidget::BuildLayout()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	WidgetTree->RootWidget = Root;

	// ── Layer 0: transparent full-screen click catcher ────────────────────
	ClickCatcher = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	FButtonStyle EmptyStyle;
	EmptyStyle.Normal         = FSlateNoResource();
	EmptyStyle.Hovered        = FSlateNoResource();
	EmptyStyle.Pressed        = FSlateNoResource();
	EmptyStyle.NormalPadding  = FMargin(0.f);
	EmptyStyle.PressedPadding = FMargin(0.f);
	ClickCatcher->SetStyle(EmptyStyle);
	ClickCatcher->OnClicked.AddDynamic(this, &UDialogueWidget::HandleClick);
	if (UOverlaySlot* S = Root->AddChildToOverlay(ClickCatcher))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Fill);
	}

	// ── Layer 1: visual panel (HitTestInvisible) ──────────────────────────
	USizeBox* PanelSB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	PanelSB->SetWidthOverride(920.f);
	PanelSB->SetHeightOverride(220.f);
	PanelSB->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UOverlaySlot* S = Root->AddChildToOverlay(PanelSB))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Bottom);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 40.f));
	}

	// Thin warm outline border (gold-brown, 1px)
	UBorder* OutlineBdr = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	OutlineBdr->SetBrushColor(FLinearColor(0.32f, 0.22f, 0.08f, 0.80f));
	OutlineBdr->SetPadding(FMargin(1.f));
	PanelSB->SetContent(OutlineBdr);

	// Main dark warm panel
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Panel->SetBrushColor(FLinearColor(0.022f, 0.016f, 0.010f, 0.97f));
	Panel->SetPadding(FMargin(26.f, 18.f, 26.f, 16.f));
	OutlineBdr->SetContent(Panel);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(VBox);

	// ── Speaker row: [3px gold accent bar] [name] ────────────────────────
	UHorizontalBox* SpeakerRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* VS = VBox->AddChildToVerticalBox(SpeakerRow))
		VS->SetHorizontalAlignment(HAlign_Fill);

	USizeBox* AccentSB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	AccentSB->SetWidthOverride(3.f);
	UBorder* AccentBdr = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	AccentBdr->SetBrushColor(FLinearColor(0.88f, 0.70f, 0.25f, 1.f));
	AccentBdr->SetPadding(FMargin(0.f));
	AccentSB->SetContent(AccentBdr);
	if (UHorizontalBoxSlot* HS = SpeakerRow->AddChildToHorizontalBox(AccentSB))
		HS->SetVerticalAlignment(VAlign_Fill);

	USizeBox* GapSB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	GapSB->SetWidthOverride(12.f);
	SpeakerRow->AddChildToHorizontalBox(GapSB);

	SpeakerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SpeakerText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
	SpeakerText->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.70f, 0.25f, 1.f)));
	if (UHorizontalBoxSlot* HS = SpeakerRow->AddChildToHorizontalBox(SpeakerText))
	{
		HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		HS->SetVerticalAlignment(VAlign_Center);
	}

	// ── Thin separator line ───────────────────────────────────────────────
	USizeBox* SepSB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	SepSB->SetHeightOverride(1.f);
	UBorder* SepBdr = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	SepBdr->SetBrushColor(FLinearColor(0.18f, 0.12f, 0.06f, 1.f));
	SepBdr->SetPadding(FMargin(0.f));
	SepSB->SetContent(SepBdr);
	if (UVerticalBoxSlot* VS = VBox->AddChildToVerticalBox(SepSB))
	{
		VS->SetHorizontalAlignment(HAlign_Fill);
		VS->SetPadding(FMargin(0.f, 10.f, 0.f, 14.f));
	}

	// ── Dialogue text ─────────────────────────────────────────────────────
	DialogueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	DialogueText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 13));
	DialogueText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.86f, 0.78f, 1.f)));
	DialogueText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* VS = VBox->AddChildToVerticalBox(DialogueText))
	{
		VS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		VS->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	// ── Hint ──────────────────────────────────────────────────────────────
	HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	HintText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.40f, 0.30f, 0.14f, 1.f)));
	HintText->SetText(FText::FromString(TEXT("[ click to advance ]")));
	HintText->SetJustification(ETextJustify::Right);
	if (UVerticalBoxSlot* VS = VBox->AddChildToVerticalBox(HintText))
		VS->SetHorizontalAlignment(HAlign_Fill);
}

void UDialogueWidget::SetClickToAdvanceEnabled(bool bEnabled)
{
	bClickToAdvanceEnabled = bEnabled;

	if (WidgetTree && WidgetTree->RootWidget)
	{
		WidgetTree->RootWidget->SetVisibility(bClickToAdvanceEnabled
			? ESlateVisibility::Visible
			: ESlateVisibility::HitTestInvisible);
	}

	if (ClickCatcher)
	{
		ClickCatcher->SetVisibility(bClickToAdvanceEnabled
			? ESlateVisibility::Visible
			: ESlateVisibility::HitTestInvisible);
	}

	if (HintText)
	{
		HintText->SetVisibility(bClickToAdvanceEnabled
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}

void UDialogueWidget::ShowLine(const FDialogueLine& Line)
{
	if (SpeakerText)
		SpeakerText->SetText(FText::FromName(Line.SpeakerName));

	FullLineText      = Line.LineText.ToString();
	CurrentCharIndex  = 0;
	bTypewriterActive = true;

	if (DialogueText)
		DialogueText->SetText(FText::GetEmpty());

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TypewriterTimer, this,
			&UDialogueWidget::TickTypewriter, TypewriterInterval, true);
	}
}

void UDialogueWidget::TickTypewriter()
{
	if (!bTypewriterActive) return;

	++CurrentCharIndex;
	if (DialogueText)
		DialogueText->SetText(FText::FromString(FullLineText.Left(CurrentCharIndex)));

	if (CurrentCharIndex >= FullLineText.Len())
		FinishTypewriter();
}

void UDialogueWidget::FinishTypewriter()
{
	bTypewriterActive = false;
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(TypewriterTimer);

	if (DialogueText)
		DialogueText->SetText(FText::FromString(FullLineText));

	OnLineFinishedRevealing.Broadcast();
}

void UDialogueWidget::HandleClick()
{
	if (!bClickToAdvanceEnabled) return;

	if (bTypewriterActive)
		FinishTypewriter();
	else
		OnAdvanceRequested.Broadcast();
}
