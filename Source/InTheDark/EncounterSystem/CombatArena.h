#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EncounterSystem/EncounterTypes.h"
#include "CombatArena.generated.h"

class UBoxComponent;
class UEncounterConfig;
class UEncounterDirectorComponent;
class ASpawnAnchor;
class AEncounterGate;

/**
 * Actor principal de una zona de encuentro.
 *
 * Flujo de implementación:
 *   1. Coloca ACombatArena en el nivel y ajusta el TriggerVolume al camino del jugador.
 *   2. Rellena Anchors (arrastra ASpawnAnchor o usa auto-bind en el BP derivado).
 *   3. Rellena Gates (opcional, para bloquear la retirada).
 *   4. Apunta Config a un UEncounterConfig.
 *   5. Asigna EncounterId con un FName único (se usa para persistencia en el guardado).
 *
 * En runtime:
 *   - Al hacer overlap el jugador (o interacción), llama a StartEncounter.
 *   - Al limpiar el encuentro, lo marca en UInTheDarkGameInstance y spawnea la recompensa.
 *   - Al morir el jugador, el director dispara OnEncounterFailed, que recarga el checkpoint
 *     si Config->bReloadCheckpointOnFailure está activo.
 */
UCLASS(Blueprintable, BlueprintType)
class INTHEDARK_API ACombatArena : public AActor
{
	GENERATED_BODY()

public:
	ACombatArena();

	/* ---------- Autoría ---------- */

	/** ID estable. Se persiste en UInTheDarkGameInstance::ClearedEncounters. NUNCA renombrar con saves activos. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Encounter|Authoring")
	FName EncounterId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Authoring")
	TObjectPtr<UEncounterConfig> Config;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Encounter|Authoring")
	TArray<TObjectPtr<ASpawnAnchor>> Anchors;

	/** Puertas de entrada: se CIERRAN al iniciar el encuentro. Opcional. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Encounter|Authoring")
	TArray<TObjectPtr<AEncounterGate>> EntryGates;

	/** Si es true, las EntryGates se abren también al completar el encuentro. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Encounter|Authoring")
	bool bUnlockEntryGatesOnClear = false;

	/** Puertas de salida: se ABREN al completar el encuentro. Opcional. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Encounter|Authoring")
	TArray<TObjectPtr<AEncounterGate>> ExitGates;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Authoring")
	bool bAutoStartOnOverlap = true;

	/** Si es true y el guardado ya marca este EncounterId como completado, no hace nada en BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Authoring")
	bool bSkipIfAlreadyCleared = true;

	/* ---------- Componentes ---------- */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter|Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter|Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter|Components")
	TObjectPtr<USceneComponent> RewardAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter|Components")
	TObjectPtr<UEncounterDirectorComponent> Director;

	/* ---------- API externa ---------- */

	/** Inicia el encuentro. Idempotente si ya está en curso o completado. */
	UFUNCTION(BlueprintCallable, Category = "Encounter")
	void RequestStart();

	/** Spawnea el actor de recompensa en el RewardAnchor según Config->Reward. */
	UFUNCTION(BlueprintCallable, Category = "Encounter")
	void SpawnReward();

	/**
	 * Utilidad de editor: busca en el nivel todos los ASpawnAnchor cuyo AnchorTags coincida
	 * con algún tag de Filter (o todos si Filter está vacío) y rellena el array Anchors.
	 * Devuelve el número de anchors enlazados.
	 */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Encounter|Authoring")
	int32 AutoBindAnchorsByTag(FGameplayTagContainer Filter);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UFUNCTION()
	void HandleEncounterStarted();

	UFUNCTION()
	void HandleEncounterCleared();

	UFUNCTION()
	void HandleEncounterFailed();

private:
	bool bAlreadyStartedThisSession = false;
	bool LookupIsAlreadyCleared() const;
	void LockEntryGates();
	void UnlockEntryGates();
	void UnlockExitGates();
	void UnlockGatesForClearedState();
};
