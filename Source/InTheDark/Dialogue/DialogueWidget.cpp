#include "Dialogue/DialogueWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"

void UDialogueWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
}

void UDialogueWidget::BuildLayout()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	WidgetTree->RootWidget = Root;

	ClickCatcher = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	FButtonStyle EmptyStyle;
	EmptyStyle.Normal = FSlateNoResource();
	EmptyStyle.Hovered = FSlateNoResource();
	EmptyStyle.Pressed = FSlateNoResource();
	EmptyStyle.NormalPadding = FMargin(0.f);
	EmptyStyle.PressedPadding = FMargin(0.f);
	ClickCatcher->SetStyle(EmptyStyle);
	ClickCatcher->OnClicked.AddDynamic(this, &UDialogueWidget::HandleClick);

	if (UOverlaySlot* S = Root->AddChildToOverlay(ClickCatcher))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* SubtitleBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	SubtitleBox->SetWidthOverride(1500.f);
	SubtitleBox->SetMaxDesiredWidth(1500.f);
	SubtitleBox->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (UOverlaySlot* S = Root->AddChildToOverlay(SubtitleBox))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Bottom);
		S->SetPadding(FMargin(80.f, 0.f, 80.f, 76.f));
	}

	DialogueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FSlateFontInfo DialogueFont = FCoreStyle::GetDefaultFontStyle("Bold", 22);
	DialogueFont.OutlineSettings.OutlineSize = 2;
	DialogueFont.OutlineSettings.OutlineColor = FLinearColor::Black;

	DialogueText->SetFont(DialogueFont);
	DialogueText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	DialogueText->SetJustification(ETextJustify::Center);
	DialogueText->SetAutoWrapText(true);
	DialogueText->SetWrapTextAt(1500.f);
	DialogueText->SetShadowOffset(FVector2D(1.f, 1.f));
	DialogueText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
	SubtitleBox->SetContent(DialogueText);

	HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	HintText->SetVisibility(ESlateVisibility::Collapsed);
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
		HintText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UDialogueWidget::ShowLine(const FDialogueLine& Line)
{
	const FString SpeakerName = Line.SpeakerName.IsNone() ? FString() : Line.SpeakerName.ToString();
	FullLineText = SpeakerName.IsEmpty()
		? Line.LineText.ToString()
		: FString::Printf(TEXT("%s: %s"), *SpeakerName, *Line.LineText.ToString());
	CurrentCharIndex = 0;
	bTypewriterActive = true;

	if (DialogueText)
	{
		DialogueText->SetText(FText::GetEmpty());
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TypewriterTimer,
			this,
			&UDialogueWidget::TickTypewriter,
			TypewriterInterval,
			true);
	}
}

void UDialogueWidget::TickTypewriter()
{
	if (!bTypewriterActive)
	{
		return;
	}

	++CurrentCharIndex;
	if (DialogueText)
	{
		DialogueText->SetText(FText::FromString(FullLineText.Left(CurrentCharIndex)));
	}

	if (CurrentCharIndex >= FullLineText.Len())
	{
		FinishTypewriter();
	}
}

void UDialogueWidget::FinishTypewriter()
{
	bTypewriterActive = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TypewriterTimer);
	}

	if (DialogueText)
	{
		DialogueText->SetText(FText::FromString(FullLineText));
	}

	OnLineFinishedRevealing.Broadcast();
}

void UDialogueWidget::HandleClick()
{
	if (!bClickToAdvanceEnabled)
	{
		return;
	}

	if (bTypewriterActive)
	{
		FinishTypewriter();
	}
	else
	{
		OnAdvanceRequested.Broadcast();
	}
}
