#include "SaveSystem/MainMenuGameMode.h"
#include "SaveSystem/MainMenuPlayerController.h"
#include "SaveSystem/MainMenuWidget.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	PlayerControllerClass = AMainMenuPlayerController::StaticClass();
	DefaultPawnClass      = nullptr;
	HUDClass              = nullptr;
	MainMenuWidgetClass   = UMainMenuWidget::StaticClass();
}
