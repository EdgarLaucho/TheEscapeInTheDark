#include "InputDeviceFunctionLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericApplication.h"

bool UInputDeviceFunctionLibrary::IsAnyGamepadConnected()
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}

	TSharedPtr<GenericApplication> PlatformApplication = FSlateApplication::Get().GetPlatformApplication();

	if (!PlatformApplication.IsValid())
	{
		return false;
	}

	return PlatformApplication->IsGamepadAttached();
}