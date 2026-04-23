#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "EncounterSystem/EncounterTypes.h"
#include "SpawnAnchor.generated.h"

class UBillboardComponent;
class UArrowComponent;
class UNiagaraSystem;
class USoundBase;
class UAnimMontage;
class UObjectPoolSubsystem;

/**
 * Punto de spawn dirigido para encuentros. Los anchors tienen tags para que el director elija
 * cuáles cubren una directiva de spawn (ej: "Anchor.North", "Role.Boss").
 *
 * Reglas de propiedad:
 *   - Los anchors son actores independientes en el nivel O hijos de ACombatArena.
 *     El director los descubre a través del array ACombatArena.Anchors en tiempo de ejecución.
 *   - El filtrado en runtime aplica: (1) no visibilidad en FOV, (2) distancia, (3) cooldown.
 */
UCLASS(Blueprintable, BlueprintType)
class INTHEDARK_API ASpawnAnchor : public AActor
{
	GENERATED_BODY()

public:
	ASpawnAnchor();

	/** Tags del diseñador que coinciden con FEnemySpawn.AnchorTag. Ej: (Anchor.North, Role.Grunt). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor")
	FGameplayTagContainer AnchorTags;

	/** VFX de telegrafeo por defecto (reemplazable por spawn). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Telegraph")
	TSoftObjectPtr<UNiagaraSystem> DefaultPreSpawnVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Telegraph")
	TSoftObjectPtr<USoundBase> DefaultPreSpawnSFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emerge")
	TSoftObjectPtr<UAnimMontage> DefaultEmergeMontage;

	/** Lead de telegrafeo por defecto si la directiva tiene PreSpawnLead <= 0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Telegraph", meta = (ClampMin = "0.0"))
	float DefaultLeadTime = 0.8f;

	/** Si es true, descarta este anchor cuando el jugador lo está mirando. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility")
	bool bBlockIfPlayerInFOV = true;

	/** Ángulo de medio FOV; mayor = más estricto ("detrás de la espalda"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility", meta = (ClampMin = "0.0", ClampMax = "180.0", EditCondition = "bBlockIfPlayerInFOV"))
	float PlayerFOVAngleDegrees = 60.f;

	/** Distancia mínima al jugador para que este anchor sea válido. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visibility", meta = (ClampMin = "0.0"))
	float MinDistanceToPlayer = 300.f;

	/** Cooldown por anchor para evitar spam de spawns en la misma posición. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0"))
	float PointCooldown = 2.f;

	/** Último tiempo de juego en que se ejecutó PerformSpawn en este anchor. -BIG_NUMBER = nunca. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Timing")
	float LastSpawnTimeSeconds = -1e9f;

	/** Devuelve true si este anchor puede usarse para spawnear ahora con el pawn del jugador dado. */
	UFUNCTION(BlueprintCallable, Category = "Anchor")
	bool IsAvailableForSpawn(const AActor* PlayerActor) const;

	/**
	 * Spawnea el enemigo usando el ObjectPool si está disponible, o SpawnActor directamente.
	 * No reproduce el telegrafeo — el director gestiona el timing.
	 */
	UFUNCTION(BlueprintCallable, Category = "Anchor")
	AActor* PerformSpawn(TSubclassOf<AActor> EnemyClass, const FEnemySpawn& Directive);

	/** Devuelve el lead efectivo (override de directiva > default del anchor). */
	float ResolveLead(const FEnemySpawn& Directive) const;

	/** Reproduce el telegrafeo VFX/SFX. Público para que el director lo llame antes del timer. */
	void PlayTelegraph(const FEnemySpawn& Directive) const;

protected:
	virtual void BeginPlay() override;

private:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Billboard;

	UPROPERTY()
	TObjectPtr<UArrowComponent> Arrow;
#endif
};
