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

void UDialogueSubsystem::StartDialogue(UDialogueData* Data, FName DialogueID, APlayerController* PC)
{
	StartDialogueWithMode(Data, DialogueID, PC, EDialoguePlaybackMode::Interactive, true);
}

void UDialogueSubsystem::StartAmbientDialogue(
	UDialogueData* Data,
	FName DialogueID,
	APlayerController* PC,
	bool bMarkSeen,
	float LineHoldSeconds)
{
	StartDialogueWithMode(Data, DialogueID, PC, EDialoguePlaybackMode::Ambient, bMarkSeen, LineHoldSeconds);
}

void UDialogueSubsystem::StartDialogueWithMode(
	UDialogueData* Data,
	FName DialogueID,
	APlayerController* PC,
	EDialoguePlaybackMode PlaybackMode,
	bool bMarkSeen,
	float LineHoldSeconds)
{
	if (!Data || Data->Lines.IsEmpty() || !PC || bActive) return;

	ActiveData = Data;
	ActivePC = PC;
	ActiveDialogueID = DialogueID;
	ActivePlaybackMode = PlaybackMode;
	CurrentLineIndex = 0;
	bActive = true;
	bMarkSeenOnEnd = bMarkSeen;
	bSaveCheckpointOnEnd = PlaybackMode == EDialoguePlaybackMode::Interactive;
	ActiveAmbientLineHoldSeconds = LineHoldSeconds > 0.f ? LineHoldSeconds : DefaultAmbientLineHoldSeconds;

	Widget = CreateWidget<UDialogueWidget>(PC, UDialogueWidget::StaticClass());
	if (Widget)
	{
		const bool bInteractive = PlaybackMode == EDialoguePlaybackMode::Interactive;
		Widget->SetClickToAdvanceEnabled(bInteractive);
		Widget->OnAdvanceRequested.AddDynamic(this, &UDialogueSubsystem::HandleAdvanceRequested);
		Widget->OnLineFinishedRevealing.AddDynamic(this, &UDialogueSubsystem::HandleLineFinishedRevealing);
		Widget->AddToViewport(10);
	}

	if (PlaybackMode == EDialoguePlaybackMode::Interactive)
	{
		if (ACharacter* Char = Cast<ACharacter>(PC->GetPawn()))
		{
			if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
			{
				Move->StopMovementImmediately();
			}
			Char->DisableInput(PC);
		}

		FInputModeUIOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);
	}

	if (Widget)
	{
		Widget->ShowLine(ActiveData->Lines[0]);
	}
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
		World->GetTimerManager().SetTimer(
			AmbientAdvanceTimer,
			this,
			&UDialogueSubsystem::AdvanceLine,
			ActiveAmbientLineHoldSeconds,
			false);
	}
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

	if (ActivePC && ActivePlaybackMode == EDialoguePlaybackMode::Interactive)
	{
		if (ACharacter* Char = Cast<ACharacter>(ActivePC->GetPawn()))
		{
			Char->EnableInput(ActivePC);
		}

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
