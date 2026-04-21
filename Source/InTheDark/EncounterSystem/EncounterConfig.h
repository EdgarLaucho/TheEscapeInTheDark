#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "EncounterSystem/EncounterTypes.h"
#include "EncounterConfig.generated.h"

/**
 * Asset de encuentro. El diseñador coloca un ACombatArena en el nivel
 * y lo apunta a este asset. Contiene el script completo de oleadas, recompensas y cues de diálogo.
 */
UCLASS(BlueprintType)
class INTHEDARK_API UEncounterConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Tipo de encuentro. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	EEncounterKind Kind = EEncounterKind::Story;

	/** Oleadas del encuentro, ejecutadas en orden. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	TArray<FEncounterWave> Waves;

	/** Pausa silenciosa tras limpiar antes de disparar OnEncounterCleared (deja terminar las anims de muerte). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "0.0"))
	float PostClearBeatSeconds = 2.5f;

	/** Descriptor de recompensa entregado al arena al completar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	FEncounterReward Reward;

	/** Tags libres del diseñador ("Encounter.Chapter1.Outskirts"). No los usa el director. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	FGameplayTagContainer Tags;

	/** Si es true, fallar este encuentro (muerte del jugador) solicita recargar el checkpoint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	bool bReloadCheckpointOnFailure = true;

	/** Si es true, el estado de completado se persiste en el guardado (usa ACombatArena.EncounterId como clave). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	bool bPersistCleared = true;
};
