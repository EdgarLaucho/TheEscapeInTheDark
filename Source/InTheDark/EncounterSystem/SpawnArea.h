#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EncounterSystem/SpawnAreaTypes.h"
#include "SpawnArea.generated.h"

class USphereComponent;
class UBillboardComponent;
class ASpawnAnchor;
class UObjectPoolSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpawnAreaSimpleEvent);

/*
 * ASpawnArea — área de spawn ambiental continuo, al estilo God of War.
 *
 * - Activa el spawn cuando el jugador entra en AreaRadius.
 * - Los enemigos no pueden alejarse más de AreaRadius del centro (leash).
 * - Cuando el jugador se aleja (AreaRadius + DespawnOffset) los enemigos
 *   se despawnean cuando no son visibles (o inmediatamente si bRequireOutOfSight=false).
 * - Reutiliza ObjectPoolSubsystem. Opcionalmente usa ASpawnAnchor del nivel
 *   como puntos de spawn; si no hay, genera puntos aleatorios dentro del radio.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Spawn Area"))
class INTHEDARK_API ASpawnArea : public AActor
{
	GENERATED_BODY()

public:
	ASpawnArea();

	// ── Configuración ────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnArea|Enemies")
	TArray<FSpawnAreaEntry> EnemyEntries;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnArea|Rules")
	FSpawnAreaRules Rules;

	// Anchors explícitos. Si está vacío, se generan puntos aleatorios dentro del radio.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "SpawnArea|Anchors")
	TArray<TObjectPtr<ASpawnAnchor>> BoundAnchors;

	// ── Eventos ──────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "SpawnArea|Events")
	FOnSpawnAreaSimpleEvent OnPlayerEntered;

	UPROPERTY(BlueprintAssignable, Category = "SpawnArea|Events")
	FOnSpawnAreaSimpleEvent OnPlayerLeft;

	UPROPERTY(BlueprintAssignable, Category = "SpawnArea|Events")
	FOnSpawnAreaSimpleEvent OnAreaCleared;

	// ── API pública ──────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "SpawnArea")
	bool IsPlayerInside() const { return bPlayerInside; }

	UFUNCTION(BlueprintPure, Category = "SpawnArea")
	int32 GetActiveEnemyCount() const;

	// Activa el spawn manualmente aunque el jugador no esté dentro.
	UFUNCTION(BlueprintCallable, Category = "SpawnArea")
	void ForceActivate();

	// Desactiva el spawn y despawnea todos los enemigos inmediatamente.
	UFUNCTION(BlueprintCallable, Category = "SpawnArea")
	void ForceDeactivate();

	// Despawnea todos los enemigos activos (ignora visibilidad).
	UFUNCTION(BlueprintCallable, Category = "SpawnArea")
	void DespawnAll();

	// Auto-busca ASpawnAnchor dentro del radio y los asigna a BoundAnchors.
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "SpawnArea|Authoring")
	void AutoBindAnchorsInRadius();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	// ── Componentes ──────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, Category = "SpawnArea|Components")
	TObjectPtr<USphereComponent> ActivationVolume;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Billboard;
#endif

	// ── Estado runtime ───────────────────────────────────────────────────────

	bool bPlayerInside = false;
	TArray<TWeakObjectPtr<AActor>> ActiveEnemies;
	// Cuando bRespawnOnDeath=false, lleva la cuenta de cuántos se han spawneado este ciclo.
	int32 SpawnQuota = 0;

	FTimerHandle SpawnTimerHandle;
	FTimerHandle LeashTimerHandle;
	FTimerHandle DespawnCheckHandle;

	// ── Activación / desactivación ───────────────────────────────────────────

	void Activate();
	void Deactivate();
	void SetPlayerInside(bool bNewPlayerInside, bool bBroadcastEvents = true);
	void RefreshPlayerInsideState();
	bool IsPlayerInsideArea() const;

	// ── Callbacks de overlap ─────────────────────────────────────────────────

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* Comp, AActor* Other, UPrimitiveComponent* OtherComp,
	                    int32 BodyIndex, bool bFromSweep, const FHitResult& Hit);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* Comp, AActor* Other,
	                  UPrimitiveComponent* OtherComp, int32 BodyIndex);

	UFUNCTION()
	void OnEnemyDestroyed(AActor* DestroyedActor);

	// ── Lógica periódica ─────────────────────────────────────────────────────

	void TrySpawn();
	void EnforceLeash();
	void CheckDespawnOnLeave();

	// ── Helpers ──────────────────────────────────────────────────────────────

	void ReleaseEnemy(AActor* Enemy);
	void ApplyLeashState(AActor* Enemy) const;
	void ApplyLeashStateToActiveEnemies() const;
	bool IsVisibleToPlayer(const AActor* Enemy) const;
	bool GetRandomSpawnTransform(FTransform& OutTransform) const;
	TSubclassOf<AActor> PickEnemyClass() const;
	int32 CountActiveOfClass(TSubclassOf<AActor> Class) const;
	void CleanDeadEntries();
	AActor* GetPlayerActor() const;
	UObjectPoolSubsystem* GetPool() const;
};
