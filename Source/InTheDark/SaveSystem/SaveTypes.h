#pragma once

#include "CoreMinimal.h"
#include "SaveTypes.generated.h"

USTRUCT(BlueprintType)
struct INTHEDARK_API FSavedPlayerState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Player")
	FTransform Transform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Player")
	bool bHasSavedTransform = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Player")
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Player")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Player")
	FName LastCheckpointID = NAME_None;
};

USTRUCT(BlueprintType)
struct INTHEDARK_API FSavedElementProgressionEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "ElementProgression")
	FName ElementName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "ElementProgression")
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "ElementProgression")
	int32 KillCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "ElementProgression")
	float DamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "ElementProgression")
	float ScaleMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "ElementProgression")
	int32 MaxUnlockedComboStep = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "ElementProgression")
	bool bUnlocked = false;
};

USTRUCT(BlueprintType)
struct INTHEDARK_API FSavedCompanionPersonality
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Companion")
	float Courage = 51.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Companion")
	float Anxiety = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Companion")
	float Confidence = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Companion")
	float AggressionAffinity = 31.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Companion")
	float StealthAffinity = 30.f;
};

USTRUCT(BlueprintType)
struct INTHEDARK_API FSavedCompanionState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Companion")
	bool bHasSavedState = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Companion")
	bool bHasAwoken = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Companion")
	FTransform Transform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Companion")
	uint8 CurrentStateValue = 0;
};

USTRUCT(BlueprintType)
struct INTHEDARK_API FSavedTutorialState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Tutorial")
	int32 SavedStep = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Tutorial")
	bool bFinished = false;
};

USTRUCT(BlueprintType)
struct INTHEDARK_API FSavedMusicState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Music")
	FName CurrentMusicId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Music")
	FName CurrentMusicZoneId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Music")
	FString MusicAssetPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Music")
	bool bShouldBePlaying = false;
};

USTRUCT(BlueprintType)
struct INTHEDARK_API FSaveSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int32 SlotIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	bool bIsEmpty = true;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FString DisplayMapName;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FDateTime SavedAt = FDateTime(0);
};