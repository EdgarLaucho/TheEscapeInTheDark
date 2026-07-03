#include "Dialogue/DialogueSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Dialogue/DialogueWidget.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "SaveSystem/InTheDarkGameInstance.h"
#include "SaveSystem/SaveTypes.h"

void UDialogueSubsystem::StartDialogue(UDialogueData* Data, FName DialogueID, APlayerController* PC, float LineHoldSeconds)
{
	const bool bAutoAdvance = LineHoldSeconds > 0.0f;
	StartDialogueWithMode(Data, DialogueID, PC, bAutoAdvance ? EDialoguePlaybackMode::Ambient : EDialoguePlaybackMode::Interactive, true, LineHoldSeconds, false);
}

void UDialogueSubsystem::StartAmbientDialogue(UDialogueData* Data, FName DialogueID, APlayerController* PC, bool bMarkSeen, float LineHoldSeconds, bool bBlockMovement)
{
	StartDialogueWithMode(Data, DialogueID, PC, EDialoguePlaybackMode::Ambient, bMarkSeen, LineHoldSeconds, bBlockMovement);
}

void UDialogueSubsystem::StartDialogueWithMode(UDialogueData* Data, FName DialogueID, APlayerController* PC, EDialoguePlaybackMode PlaybackMode, bool bMarkSeen, float LineHoldSeconds, bool bBlockMovement)
{
	if (!Data || Data->Lines.IsEmpty() || !PC || bActive) return;

	if (!WidgetClass)
	{
		WidgetClass = LoadClass<UDialogueWidget>(nullptr, TEXT("/Game/Blueprints/Dialogues/WBP_Dialogue.WBP_Dialogue_C"));
	}
	
	if (!WidgetClass) return;

	Widget = CreateWidget<UDialogueWidget>(PC, WidgetClass);
	if (!Widget) return;

	const bool bInteractive = PlaybackMode == EDialoguePlaybackMode::Interactive;
	const bool bShouldBlockMovement = bInteractive || bBlockMovement;

	ActiveData = Data;
	ActivePC = PC;
	ActiveDialogueID = DialogueID;
	ActivePlaybackMode = PlaybackMode;
	CurrentLineIndex = 0;
	bActive = true;
	bMarkSeenOnEnd = bMarkSeen;
	bSaveCheckpointOnEnd = bInteractive;
	bBlockedMovementForActiveDialogue = bShouldBlockMovement;
	ActiveAmbientLineHoldSeconds = LineHoldSeconds > 0.f ? LineHoldSeconds : DefaultAmbientLineHoldSeconds;

	Widget->SetClickToAdvanceEnabled(bInteractive);
	Widget->OnAdvanceRequested.AddDynamic(this, &UDialogueSubsystem::HandleAdvanceRequested);
	Widget->OnLineFinishedRevealing.AddDynamic(this, &UDialogueSubsystem::HandleLineFinishedRevealing);
	Widget->AddToViewport(10);

	if (bShouldBlockMovement)
	{
		if (ACharacter* Char = Cast<ACharacter>(PC->GetPawn()))
		{
			if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
			{
				Move->StopMovementImmediately();
			}

			Char->DisableInput(PC);
		}
	}

	if (bInteractive)
	{
		FInputModeUIOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);
	}

	Widget->ShowLine(ActiveData->Lines[0]);
}

void UDialogueSubsystem::HandleAdvanceRequested()
{
	if (ActivePlaybackMode != EDialoguePlaybackMode::Interactive) return;
	AdvanceLine();
}

void UDialogueSubsystem::HandleLineFinishedRevealing()
{
	if (!bActive || ActivePlaybackMode != EDialoguePlaybackMode::Ambient) return;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AmbientAdvanceTimer);
		World->GetTimerManager().SetTimer(AmbientAdvanceTimer, this, &UDialogueSubsystem::AdvanceLine, GetHoldSecondsForCurrentLine(), false);
	}
}

float UDialogueSubsystem::GetHoldSecondsForCurrentLine() const
{
	if (!ActiveData || !ActiveData->Lines.IsValidIndex(CurrentLineIndex))
		return ActiveAmbientLineHoldSeconds;

	const float OverrideSeconds = ActiveData->Lines[CurrentLineIndex].LineHoldSecondsOverride;
	return OverrideSeconds > 0.0f ? OverrideSeconds : ActiveAmbientLineHoldSeconds;
}

void UDialogueSubsystem::AdvanceLine()
{
	if (!bActive || !ActiveData) return;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AmbientAdvanceTimer);
	}

	++CurrentLineIndex;
	if (CurrentLineIndex >= ActiveData->Lines.Num())
	{
		EndDialogue();
		return;
	}

	if (Widget)
	{
		Widget->ShowLine(ActiveData->Lines[CurrentLineIndex]);
	}
}

void UDialogueSubsystem::EndDialogue()
{
	if (!bActive) return;
	bActive = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AmbientAdvanceTimer);
	}

	SaveAndMarkSeen();

	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget = nullptr;
	}

	if (ActivePC && bBlockedMovementForActiveDialogue)
	{
		if (ACharacter* Char = Cast<ACharacter>(ActivePC->GetPawn()))
		{
			Char->EnableInput(ActivePC);
		}
	}

	if (ActivePC && ActivePlaybackMode == EDialoguePlaybackMode::Interactive)
	{
		FInputModeGameOnly InputMode;
		ActivePC->SetInputMode(InputMode);
		ActivePC->SetShowMouseCursor(false);
	}

	ActiveData = nullptr;
	ActivePC = nullptr;
	ActiveDialogueID = NAME_None;
	CurrentLineIndex = 0;
	bMarkSeenOnEnd = true;
	bSaveCheckpointOnEnd = true;
	bBlockedMovementForActiveDialogue = false;
	ActivePlaybackMode = EDialoguePlaybackMode::Interactive;
	ActiveAmbientLineHoldSeconds = DefaultAmbientLineHoldSeconds;
}

void UDialogueSubsystem::SaveAndMarkSeen()
{
	if (!bMarkSeenOnEnd || ActiveDialogueID.IsNone()) return;

	UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(GetGameInstance());
	if (!GI) return;

	GI->MarkDialogueSeen(ActiveDialogueID);

	if (bSaveCheckpointOnEnd && ActivePC)
	{
		if (APawn* Pawn = ActivePC->GetPawn())
		{
			FSavedPlayerState State = GI->GetPlayerState();
			State.Transform = Pawn->GetActorTransform();
			State.LastCheckpointID = ActiveDialogueID;
			GI->SaveAtCheckpoint(ActiveDialogueID, State);
		}
	}
	else if (GI->IsSaveDirty())
	{
		GI->WriteSaveToDiskAsync();
	}
}