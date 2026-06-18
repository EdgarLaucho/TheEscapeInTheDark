#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DialogueData.generated.h"

USTRUCT(BlueprintType)
struct INTHEDARK_API FDialogueLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName SpeakerName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue", meta = (MultiLine = true))
	FText LineText;
};

/** DataAsset que contiene una secuencia de líneas de diálogo.
 *  Crear en el editor: click derecho → Miscellaneous → Data Asset → DialogueData.
 *  Asignar al ADialogueNPC correspondiente.
 */
UCLASS(BlueprintType)
class INTHEDARK_API UDialogueData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FDialogueLine> Lines;
};
