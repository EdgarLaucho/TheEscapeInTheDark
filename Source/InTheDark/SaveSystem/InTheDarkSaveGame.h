#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SaveSystem/SaveTypes.h"
#include "InTheDarkSaveGame.generated.h"

USTRUCT()
struct FWorldActorIDList
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	TArray<FString> IDs;
};

UCLASS(BlueprintType)
class INTHEDARK_API UInTheDarkSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	int32 SaveVersion = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	FDateTime SavedAtUtc = FDateTime(0);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|Player")
	FSavedPlayerState PlayerState;

	UPROPERTY(VisibleAnywhere, SaveGame, Category = "Save|World")
	TMap<FName, FWorldActorIDList> WorldState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|Encounters")
	TArray<FName> ClearedEncounters;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|Meta")
	FString LastMapName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|ElementProgression")
	TArray<FSavedElementProgressionEntry> ElementProgression;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|Companion")
	FSavedCompanionPersonality CompanionPersonality;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|Companion")
	FSavedCompanionState CompanionState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|Tutorial")
	FSavedTutorialState TutorialState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|Dialogue")
	TArray<FName> SeenDialogues;
};