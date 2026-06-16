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

const FName UMainMenuWidget::DefaultGameLevel = TEXT("Lvl_EscapeRouteFromTheDarkness");

namespace NavColors
{
	const FLinearColor BG        = FLinearColor(0.015f, 0.012f, 0.025f, 1.f);
	const FLinearColor PanelBG   = FLinearColor(0.04f,  0.035f, 0.07f,  0.94f);
	const FLinearColor Gold      = FLinearColor(0.88f,  0.70f,  0.25f,  1.f);
	const FLinearColor Subtitle  = FLinearColor(0.48f,  0.40f,  0.30f,  1.f);
	const FLinearColor White     = FLinearColor(0.90f,  0.88f,  0.85f,  1.f);
	const FLinearColor Divider   = FLinearColor(0.20f,  0.16f,  0.28f,  1.f);
	const FLinearColor BtnN      = FLinearColor(0.10f,  0.08f,  0.16f,  1.f);
	const FLinearColor BtnH      = FLinearColor(0.24f,  0.20f,  0.38f,  1.f);
	const FLinearColor BtnP      = FLinearColor(0.05f,  0.04f,  0.09f,  1.f);
	const FLinearColor DangerN   = FLinearColor(0.22f,  0.03f,  0.03f,  1.f);
	const FLinearColor DangerH   = FLinearColor(0.48f,  0.06f,  0.06f,  1.f);
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
	ContentSB->SetWidthOverride(480.f);
	if (UOverlaySlot* S = Root->AddChildToOverlay(ContentSB))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Center);
	}

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("PanelBG"));
	PanelBorder->SetBrushColor(NavColors::PanelBG);
	PanelBorder->SetPadding(FMargin(48.f, 44.f));
	ContentSB->SetContent(PanelBorder);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName("VBox"));
	PanelBorder->SetContent(VBox);

	// ── Title ──────────────────────────────────────────────────────────
	UTextBlock* TitleLine1 = MakeText(TEXT("THE ESCAPE"), 40, NavColors::Gold, true);
	TitleLine1->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(TitleLine1))
		S->SetHorizontalAlignment(HAlign_Fill);

	UTextBlock* TitleLine2 = MakeText(TEXT("IN  THE  DARK"), 16, NavColors::Subtitle);
	TitleLine2->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(TitleLine2))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}

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
		S->SetPadding(FMargin(0.f, 26.f));
	}

	// ── Main panel (JUGAR / SALIR) ─────────────────────────────────────
	MainPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName("MainPanel"));
	if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(MainPanel))
		S->SetHorizontalAlignment(HAlign_Fill);

	BTN_Play = MakeNavButton(TEXT("JUGAR"));
	if (UVerticalBoxSlot* S = MainPanel->AddChildToVerticalBox(BTN_Play))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	BTN_Quit = MakeNavButton(TEXT("SALIR DEL JUEGO"), true);
	if (UVerticalBoxSlot* S = MainPanel->AddChildToVerticalBox(BTN_Quit))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}

	// ── Slot panel (ranuras de guardado) ───────────────────────────────
	SlotPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName("SlotPanel"));
	SlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(SlotPanel))
		S->SetHorizontalAlignment(HAlign_Fill);

	UTextBlock* SlotHeader = MakeText(TEXT("SELECCIONAR RANURA"), 12, NavColors::Subtitle, true);
	SlotHeader->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* S = SlotPanel->AddChildToVerticalBox(SlotHeader))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	}

	APlayerController* PC = GetOwningPlayer();
	for (int32 i = 0; i < 3; ++i)
	{
		USaveSlotEntryWidget* SlotW = CreateWidget<USaveSlotEntryWidget>(PC, USaveSlotEntryWidget::StaticClass());
		if (SlotW)
		{
			SlotWidgets.Add(SlotW);
			if (UVerticalBoxSlot* S = SlotPanel->AddChildToVerticalBox(SlotW))
				S->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}
	}

	BTN_Back = MakeNavButton(TEXT("← VOLVER"));
	if (UVerticalBoxSlot* S = SlotPanel->AddChildToVerticalBox(BTN_Back))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
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

	FName LevelToLoad = DefaultGameLevel;
	if (!bIsEmpty)
	{
		const FString LastMap = GI->GetLastMapName();
		if (!LastMap.IsEmpty())
			LevelToLoad = FName(*LastMap);
	}

	UGameplayStatics::OpenLevel(this, LevelToLoad);
}

void UMainMenuWidget::OnSlotReset(int32 SlotIndex)
{
	UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(GetGameInstance());
	if (!GI) return;

	GI->DeleteSlot(SlotIndex);
	RefreshSlots();
}
