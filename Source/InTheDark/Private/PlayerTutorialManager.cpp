#include "PlayerTutorialManager.h"

APlayerTutorialManager::APlayerTutorialManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APlayerTutorialManager::BeginPlay()
{
	Super::BeginPlay();

	CurrentStep = 0;
	SavedTutorialStep = CurrentStep;
	bTutorialFinished = false;
	bControlPressed = false;
	AutoAdvanceTimer = 0.0f;
	HeldActions.Empty();
}

void APlayerTutorialManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bTutorialFinished || !IsValidCurrentStep())
	{
		return;
	}

	const FTutorialStep& Step = TutorialSteps[CurrentStep];

	if (Step.bAutoAdvanceAfterShow)
	{
		AutoAdvanceTimer += DeltaTime;

		if (AutoAdvanceTimer >= Step.AutoAdvanceDelay)
		{
			NextStep();
		}
	}
}

void APlayerTutorialManager::NextStep()
{
	if (bTutorialFinished)
	{
		return;
	}

	CurrentStep++;
	SavedTutorialStep = CurrentStep;
	bControlPressed = false;
	AutoAdvanceTimer = 0.0f;
	HeldActions.Empty();

	if (!IsValidCurrentStep())
	{
		FinishTutorial();
		return;
	}

	ShowStep();
}

void APlayerTutorialManager::ShowStep()
{
	if (!IsValidCurrentStep())
	{
		FinishTutorial();
		return;
	}

	AutoAdvanceTimer = 0.0f;
	OnTutorialStepChanged.Broadcast(CurrentStep, TutorialSteps[CurrentStep]);
}

void APlayerTutorialManager::CompleteCurrentStep()
{
	if (bTutorialFinished || !IsValidCurrentStep())
	{
		return;
	}

	NextStep();
}

void APlayerTutorialManager::CompleteCurrentStepFromEvent()
{
	if (bTutorialFinished || !IsValidCurrentStep())
	{
		return;
	}

	const FTutorialStep& Step = TutorialSteps[CurrentStep];

	if (Step.StepType == ETutorialStepType::WaitForBlueprintEvent)
	{
		NextStep();
	}
}

bool APlayerTutorialManager::IsControlPressed() const
{
	return bControlPressed;
}

void APlayerTutorialManager::NotifyActionStarted(UInputAction* Action)
{
	if (!Action || bTutorialFinished || !IsValidCurrentStep())
	{
		return;
	}

	const FTutorialStep& Step = TutorialSteps[CurrentStep];

	HeldActions.Add(Action);

	if (Action == FusionModeAction)
	{
		bControlPressed = true;
	}

	switch (Step.StepType)
	{
	case ETutorialStepType::PressAction:
		if (Action == Step.RequiredAction)
		{
			NextStep();
		}
		break;

	case ETutorialStepType::HoldAndPressAction:
		if (Action == Step.RequiredAction && HeldActions.Contains(Step.RequiredHeldAction))
		{
			NextStep();
		}
		break;

	case ETutorialStepType::ShowOnly:
	case ETutorialStepType::WaitForBlueprintEvent:
	default:
		break;
	}
}

void APlayerTutorialManager::NotifyActionCompleted(UInputAction* Action)
{
	if (!Action)
	{
		return;
	}

	HeldActions.Remove(Action);

	if (Action == FusionModeAction)
	{
		bControlPressed = false;
	}
}

FTutorialStep APlayerTutorialManager::GetCurrentTutorialStep() const
{
	if (!IsValidCurrentStep())
	{
		return FTutorialStep();
	}

	return TutorialSteps[CurrentStep];
}

void APlayerTutorialManager::SetUsingGamepad(bool bNewUsingGamepad)
{
	if (bUsingGamepad == bNewUsingGamepad)
	{
		return;
	}

	bUsingGamepad = bNewUsingGamepad;

	if (!bTutorialFinished && IsValidCurrentStep())
	{
		ShowStep();
	}
}

FText APlayerTutorialManager::GetCurrentStepDisplayText() const
{
	if (!IsValidCurrentStep())
	{
		return FText::GetEmpty();
	}

	const FTutorialStep& Step = TutorialSteps[CurrentStep];

	if (bUsingGamepad && !Step.GamepadStepText.IsEmpty())
	{
		return Step.GamepadStepText;
	}

	if (!bUsingGamepad && !Step.KeyboardStepText.IsEmpty())
	{
		return Step.KeyboardStepText;
	}

	return Step.StepText;
}

bool APlayerTutorialManager::IsValidCurrentStep() const
{
	return TutorialSteps.IsValidIndex(CurrentStep);
}

void APlayerTutorialManager::FinishTutorial()
{
	if (bTutorialFinished)
	{
		return;
	}

	bTutorialFinished = true;
	SavedTutorialStep = CurrentStep;
	bControlPressed = false;
	AutoAdvanceTimer = 0.0f;
	HeldActions.Empty();

	OnTutorialFinished.Broadcast();
}

void APlayerTutorialManager::ApplyLoadedTutorialState(int32 LoadedStep, bool bLoadedTutorialFinished)
{
	bTutorialFinished = bLoadedTutorialFinished;
	CurrentStep = FMath::Clamp(LoadedStep, 0, TutorialSteps.Num());
	SavedTutorialStep = CurrentStep;

	bControlPressed = false;
	AutoAdvanceTimer = 0.0f;
	HeldActions.Empty();

	if (bTutorialFinished)
	{
		OnTutorialFinished.Broadcast();
		return;
	}

	if (IsValidCurrentStep())
	{
		ShowStep();
	}
	else
	{
		FinishTutorial();
	}
}

void APlayerTutorialManager::InitializeTutorialInputDevice(bool bInitialUsingGamepad)
{
	bUsingGamepad = bInitialUsingGamepad;

	if (!bTutorialFinished && IsValidCurrentStep())
	{
		ShowStep();
	}
}