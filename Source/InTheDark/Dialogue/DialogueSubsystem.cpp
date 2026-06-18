#include "Dialogue/DialogueSubsystem.h"
#include "Dialogue/DialogueWidget.h"
#include "SaveSystem/InTheDarkGameInstance.h"
#include "SaveSystem/SaveTypes.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UDialogueSubsystem::StartDialogue(UDialogueData* Data, FName DialogueID, APlayerController* PC)
{
	if (!Data || Data->Lines.IsEmpty() || !PC || bActive) return;

	ActiveData        = Data;
	ActivePC          = PC;
	ActiveDialogueID  = DialogueID;
	CurrentLineIndex  = 0;
	bActive           = true;

	Widget = CreateWidget<UDialogueWidget>(PC, UDialogueWidget::StaticClass());
	if (Widget)
	{
		Widget->OnAdvanceRequested.AddDynamic(this, &UDialogueSubsystem::HandleAdvanceRequested);
		Widget->AddToViewport(10);
	}

	// Parar y bloquear el movimiento del jugador
	if (ACharacter* Char = Cast<ACharacter>(PC->GetPawn()))
	{
		if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
			Move->StopMovementImmediately();
		Char->DisableInput(PC);
	}

	FInputModeUIOnly InputMode;
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);

	if (Widget)
		Widget->ShowLine(ActiveData->Lines[0]);
}

void UDialogueSubsystem::HandleAdvanceRequested()
{
	AdvanceLine();
}

void UDialogueSubsystem::AdvanceLine()
{
	if (!bActive || !ActiveData) return;

	++CurrentLineIndex;
	if (CurrentLineIndex >= ActiveData->Lines.Num())
	{
		EndDialogue();
		return;
	}

	if (Widget)
		Widget->ShowLine(ActiveData->Lines[CurrentLineIndex]);
}

void UDialogueSubsystem::EndDialogue()
{
	if (!bActive) return;
	bActive = false;

	SaveAndMarkSeen();

	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget = nullptr;
	}

	if (ActivePC)
	{
		if (ACharacter* Char = Cast<ACharacter>(ActivePC->GetPawn()))
			Char->EnableInput(ActivePC);

		FInputModeGameOnly InputMode;
		ActivePC->SetInputMode(InputMode);
		ActivePC->SetShowMouseCursor(false);
	}

	ActiveData       = nullptr;
	ActivePC         = nullptr;
	ActiveDialogueID = NAME_None;
}

void UDialogueSubsystem::SaveAndMarkSeen()
{
	UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(GetGameInstance());
	if (!GI) return;

	GI->MarkDialogueSeen(ActiveDialogueID);

	if (ActivePC)
	{
		if (APawn* Pawn = ActivePC->GetPawn())
		{
			FSavedPlayerState State = GI->GetPlayerState();
			State.Transform         = Pawn->GetActorTransform();
			State.LastCheckpointID  = ActiveDialogueID;
			GI->SaveAtCheckpoint(ActiveDialogueID, State);
		}
	}
}
