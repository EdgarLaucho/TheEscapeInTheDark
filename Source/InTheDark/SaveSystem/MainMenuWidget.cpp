#include "SaveSystem/MainMenuWidget.h"
#include "SaveSystem/SaveSlotEntryWidget.h"
#include "SaveSystem/InTheDarkGameInstance.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateBrush.h"
#include "Styling/CoreStyle.h"

const FName UMainMenuWidget::DefaultGameLevel = TEXT("Lvl_TestPool");

namespace NavColors
{
	const FLinearColor BG        = FLinearColor(0.025f, 0.018f, 0.012f, 1.f);
	const FLinearColor PanelBG   = FLinearColor(0.055f, 0.040f, 0.028f, 0.97f);
	const FLinearColor Gold      = FLinearColor(0.88f,  0.70f,  0.28f,  1.f);
	const FLinearColor Subtitle  = FLinearColor(0.52f,  0.42f,  0.28f,  1.f);
	const FLinearColor White     = FLinearColor(0.92f,  0.88f,  0.80f,  1.f);
	const FLinearColor Divider   = FLinearColor(0.14f,  0.10f,  0.06f,  1.f);
	const FLinearColor BtnN      = FLinearColor(0.070f, 0.052f, 0.034f, 1.f);
	const FLinearColor BtnH      = FLinearColor(0.180f, 0.135f, 0.070f, 1.f);
	const FLinearColor BtnP      = FLinearColor(0.035f, 0.026f, 0.017f, 1.f);
	const FLinearColor DangerN   = FLinearColor(0.20f,  0.04f,  0.04f,  1.f);
	const FLinearColor DangerH   = FLinearColor(0.40f,  0.06f,  0.06f,  1.f);
	const FLinearColor DangerP   = FLinearColor(0.10f,  0.02f,  0.02f,  1.f);
}

UTextBlock* UMainMenuWidget::MakeText(const FString& Txt, int32 Size, const FLinearColor& Color, bool bBold)
{
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(FText::FromString(Txt));
	T->SetColorAndOpacity(FSlateColor(Color));
	T->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size));
	return T;
}

UButton* UMainMenuWidget::MakeNavButton(const FString& Label, bool bDanger)
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());

	UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Lbl->SetText(FText::FromString(Label));
	Lbl->SetColorAndOpacity(FSlateColor(NavColors::White));
	Lbl->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
	Lbl->SetJustification(ETextJustify::Center);
	Btn->AddChild(Lbl);

	const FLinearColor N = bDanger ? NavColors::DangerN : NavColors::BtnN;
	const FLinearColor H = bDanger ? NavColors::DangerH : NavColors::BtnH;
	const FLinearColor P = bDanger ? NavColors::DangerP : NavColors::BtnP;

	FButtonStyle Style;
	Style.Normal  = FSlateColorBrush(FSlateColor(N));
	Style.Hovered = FSlateColorBrush(FSlateColor(H));
	Style.Pressed = FSlateColorBrush(FSlateColor(P));
	Style.NormalPadding  = FMargin(0.f, 15.f);
	Style.PressedPadding = FMargin(0.f, 16.f, 0.f, 14.f);
	Btn->SetStyle(Style);
	return Btn;
}

void UMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
}

void UMainMenuWidget::BuildLayout()
{
	// ── Root overlay (full screen) ─────────────────────────────────────
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), FName("Root"));
	WidgetTree->RootWidget = Root;

	// Full-screen dark background
	UBorder* BGBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("BG"));
	BGBorder->SetBrushColor(NavColors::BG);
	BGBorder->SetPadding(FMargin(0.f));
	if (UOverlaySlot* S = Root->AddChildToOverlay(BGBorder))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Fill);
	}

	// Content panel: fixed width, centered
	USizeBox* ContentSB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName("ContentSB"));
	ContentSB->SetWidthOverride(500.f);
	if (UOverlaySlot* S = Root->AddChildToOverlay(ContentSB))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Center);
	}

	// Thin outline border around the panel
	UBorder* OutlineBdr = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("Outline"));
	OutlineBdr->SetBrushColor(FLinearColor(0.32f, 0.22f, 0.08f, 1.f));
	OutlineBdr->SetPadding(FMargin(1.f));
	ContentSB->SetContent(OutlineBdr);

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("PanelBG"));
	PanelBorder->SetBrushColor(NavColors::PanelBG);
	PanelBorder->SetPadding(FMargin(52.f, 44.f, 52.f, 40.f));
	OutlineBdr->SetContent(PanelBorder);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName("VBox"));
	PanelBorder->SetContent(VBox);

	// ── Title ──────────────────────────────────────────────────────────
	UTextBlock* TitleLine1 = MakeText(TEXT("THE ESCAPE"), 40, NavColors::Gold, true);
	TitleLine1->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(TitleLine1))
		S->SetHorizontalAlignment(HAlign_Fill);

	// Short centered ornament line between title lines
	USizeBox* OrnSB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName("OrnSB"));
	OrnSB->SetWidthOverride(60.f);
	OrnSB->SetHeightOverride(1.f);
	UBorder* OrnBdr = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("OrnBdr"));
	OrnBdr->SetBrushColor(FLinearColor(0.88f, 0.70f, 0.25f, 0.55f));
	OrnBdr->SetPadding(FMargin(0.f));
	OrnSB->SetContent(OrnBdr);
	if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(OrnSB))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 8.f));
	}

	UTextBlock* TitleLine2 = MakeText(TEXT("IN  THE  DARK"), 15, NavColors::Subtitle);
	TitleLine2->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(TitleLine2))
		S->SetHorizontalAlignment(HAlign_Fill);

	// ── Divider ────────────────────────────────────────────────────────
	USizeBox* DivSB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName("DivSB"));
	DivSB->SetHeightOverride(1.f);
	UBorder* DivBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("DivBdr"));
	DivBorder->SetBrushColor(NavColors::Divider);
	DivBorder->SetPadding(FMargin(0.f));
	DivSB->SetContent(DivBorder);
	if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(DivSB))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetPadding(FMargin(0.f, 28.f));
	}

	// ── Main panel (PLAY / QUIT GAME) ──────────────────────────────────
	MainPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName("MainPanel"));
	if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(MainPanel))
		S->SetHorizontalAlignment(HAlign_Fill);

	BTN_Play = MakeNavButton(TEXT("PLAY"));
	if (UVerticalBoxSlot* S = MainPanel->AddChildToVerticalBox(BTN_Play))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	}

	BTN_Quit = MakeNavButton(TEXT("QUIT GAME"), true);
	if (UVerticalBoxSlot* S = MainPanel->AddChildToVerticalBox(BTN_Quit))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}

	// ── Slot panel ─────────────────────────────────────────────────────
	SlotPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName("SlotPanel"));
	SlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(SlotPanel))
		S->SetHorizontalAlignment(HAlign_Fill);

	UTextBlock* SlotHeader = MakeText(TEXT("SELECT  SLOT"), 11, NavColors::Subtitle, true);
	SlotHeader->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* S = SlotPanel->AddChildToVerticalBox(SlotHeader))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));
	}

	APlayerController* PC = GetOwningPlayer();
	for (int32 i = 0; i < 3; ++i)
	{
		USaveSlotEntryWidget* SlotW = CreateWidget<USaveSlotEntryWidget>(PC, USaveSlotEntryWidget::StaticClass());
		if (SlotW)
		{
			SlotWidgets.Add(SlotW);
			if (UVerticalBoxSlot* S = SlotPanel->AddChildToVerticalBox(SlotW))
			{
				S->SetHorizontalAlignment(HAlign_Fill);
				S->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
			}
		}
	}

	BTN_Back = MakeNavButton(TEXT("← BACK"));
	if (UVerticalBoxSlot* S = SlotPanel->AddChildToVerticalBox(BTN_Back))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}

	// ── Version footer ─────────────────────────────────────────────────
	UTextBlock* VersionText = MakeText(TEXT("The Escape in the Dark  ·  v0.1"), 9,
		FLinearColor(0.26f, 0.20f, 0.12f, 1.f));
	VersionText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(VersionText))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetPadding(FMargin(0.f, 28.f, 0.f, 0.f));
	}

	// ── Bind buttons ───────────────────────────────────────────────────
	BTN_Play->OnClicked.AddDynamic(this, &UMainMenuWidget::HandlePlay);
	BTN_Quit->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleQuit);
	BTN_Back->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleBack);
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshSlots();
	ShowMainPanel();
}

void UMainMenuWidget::RefreshSlots()
{
	UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(GetGameInstance());
	if (!GI) return;

	const TArray<FSaveSlotInfo> SlotInfos = GI->GetAllSlotInfos();
	for (int32 i = 0; i < SlotWidgets.Num() && SlotInfos.IsValidIndex(i); ++i)
	{
		SlotWidgets[i]->InitSlot(SlotInfos[i], this);
	}
}

void UMainMenuWidget::ShowMainPanel()
{
	if (MainPanel) MainPanel->SetVisibility(ESlateVisibility::Visible);
	if (SlotPanel) SlotPanel->SetVisibility(ESlateVisibility::Collapsed);
}

void UMainMenuWidget::ShowSlotPanel()
{
	if (MainPanel) MainPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (SlotPanel) SlotPanel->SetVisibility(ESlateVisibility::Visible);
}

void UMainMenuWidget::HandlePlay()  { ShowSlotPanel(); }
void UMainMenuWidget::HandleBack()  { ShowMainPanel(); }

void UMainMenuWidget::HandleQuit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UMainMenuWidget::OnSlotSelected(int32 SlotIndex, bool bIsEmpty)
{
	UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(GetGameInstance());
	if (!GI) return;

	GI->SwitchToSlot(SlotIndex);
	UGameplayStatics::OpenLevel(this, DefaultGameLevel);
}

void UMainMenuWidget::OnSlotReset(int32 SlotIndex)
{
	UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(GetGameInstance());
	if (!GI) return;

	GI->DeleteSlot(SlotIndex);
	RefreshSlots();
}
