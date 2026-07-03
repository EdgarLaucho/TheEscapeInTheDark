#pragma once

#include "CoreMinimal.h"
#include "SaveTypes.generated.h"

/** Entrada de inventario: nombre de fila + cantidad. */
USTRUCT(BlueprintType)
struct INTHEDARK_API FSavedInventoryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Inventory")
	FName ItemRowName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Inventory")
	int32 Quantity = 0;

	FSavedInventoryEntry() = default;
	FSavedInventoryEntry(FName InRowName, int32 InQuantity)
		: ItemRowName(InRowName), Quantity(InQuantity) {}
};

/** Estado del jugador en el momento del guardado. */
USTRUCT(BlueprintType)
struct INTHEDARK_API FSavedPlayerState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Player")
	FTransform Transform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Player")
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Player")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Player")
	FName LastCheckpointID = NAME_None;
};

/** Progresión de un elemento guardada en disco. */
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

/** Personalidad del compañero IA guardada en disco. */
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
struct INTHEDARK_API FSavedTutorialState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Tutorial")
	int32 SavedStep = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Tutorial")
	bool bFinished = false;
};

/** Metadatos ligeros del slot mostrados en el menú de selección de guardado. */
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
