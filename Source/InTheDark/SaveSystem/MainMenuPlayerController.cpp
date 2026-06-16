#include "SaveSystem/MainMenuPlayerController.h"
#include "SaveSystem/MainMenuGameMode.h"
#include "SaveSystem/MainMenuWidget.h"

void AMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetInputMode(FInputModeUIOnly());
	bShowMouseCursor = true;

	AMainMenuGameMode* GM = GetWorld() ? Cast<AMainMenuGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
	if (!GM || !GM->MainMenuWidgetClass) return;

	MainMenuWidgetInstance = CreateWidget<UMainMenuWidget>(this, GM->MainMenuWidgetClass);
	if (MainMenuWidgetInstance)
	{
		MainMenuWidgetInstance->AddToViewport();
	}
}
