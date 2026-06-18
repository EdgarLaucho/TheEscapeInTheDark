#include "Dialogue/DialogueWidget.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
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

	// ── Capa 0: botón transparente que captura todos los clicks ──────────────
	UButton* ClickCatcher = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
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

	// ── Capa 1: panel visual (HitTestInvisible — los clicks pasan a la capa 0) ──
	USizeBox* PanelSB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	PanelSB->SetHeightOverride(180.f);
	PanelSB->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UOverlaySlot* S = Root->AddChildToOverlay(PanelSB))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Bottom);
		S->SetPadding(FMargin(80.f, 0.f, 80.f, 40.f));
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Panel->SetBrushColor(FLinearColor(0.02f, 0.015f, 0.04f, 0.92f));
	Panel->SetPadding(FMargin(28.f, 18.f));
	PanelSB->SetContent(Panel);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(VBox);

	// Nombre del hablante
	SpeakerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SpeakerText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 13));
	SpeakerText->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.70f, 0.25f, 1.f)));
	if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(SpeakerText))
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

	// Texto del diálogo
	DialogueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	DialogueText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12));
	DialogueText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.88f, 0.85f, 1.f)));
	DialogueText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(DialogueText))
	{
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	// Hint
	UTextBlock* HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	HintText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.48f, 0.42f, 0.36f, 1.f)));
	HintText->SetText(FText::FromString(TEXT("[ Click to continue ]")));
	HintText->SetJustification(ETextJustify::Right);
	if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(HintText))
		S->SetHorizontalAlignment(HAlign_Fill);
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
}

void UDialogueWidget::HandleClick()
{
	if (bTypewriterActive)
		FinishTypewriter();
	else
		OnAdvanceRequested.Broadcast();
}
