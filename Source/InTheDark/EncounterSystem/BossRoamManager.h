#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossRoamManager.generated.h"

class UNiagaraSystem;
class USoundBase;
class UObjectPoolSubsystem;

UENUM(BlueprintType)
enum class EBossRoamState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Arriving	UMETA(DisplayName = "Arriving"),
	Hunting		UMETA(DisplayName = "Hunting"),
	Returning	UMETA(DisplayName = "Returning"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBossArrived,   AActor*, Boss, FVector, PortalLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBossReturning, AActor*, Boss, FVector, PortalLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBossDeparted);

/**
 * ABossRoamManager — gestiona las apariciones periódicas del jefe a través de portales.
 *
 * Solo debe haber una instancia en escena. Usa SetSystemEnabled() estático para
 * activar/desactivar el sistema desde cualquier Blueprint sin necesidad de referencia.
 *
 * Flujo de integración con el Blueprint del jefe:
 *   1. Escuchar OnBossArrived  → activar búsqueda/persecución.
 *   2. Cuando la IA pierde al jugador → llamar RequestBossReturn().
 *   3. Escuchar OnBossReturning → reproducir animación de salida.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Boss Roam Manager"))
class INTHEDARK_API ABossRoamManager : public AActor
{
	GENERATED_BODY()

public:
	ABossRoamManager();

	// ── Timing ───────────────────────────────────────────────────────────────

	/** Tiempo de espera antes de la primera aparición al inicio del nivel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossRoam|Timing", meta = (ClampMin = "0.0"))
	float InitialDelay = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossRoam|Timing", meta = (ClampMin = "1.0"))
	float MinAppearanceInterval = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossRoam|Timing", meta = (ClampMin = "1.0"))
	float MaxAppearanceInterval = 200.f;

	/** Tiempo entre la apertura del portal y la aparición/desaparición del jefe. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossRoam|Timing", meta = (ClampMin = "0.0"))
	float PortalOpenDuration = 2.f;

	// ── Spawn ────────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossRoam|Spawn", meta = (ClampMin = "0.0"))
	float SpawnRadiusMin = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossRoam|Spawn", meta = (ClampMin = "0.0"))
	float SpawnRadiusMax = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossRoam|Spawn")
	TSoftClassPtr<AActor> BossClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossRoam|Spawn")
	TSoftObjectPtr<UNiagaraSystem> PortalOpenVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossRoam|Spawn")
	TSoftObjectPtr<USoundBase> PortalOpenSFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossRoam|Spawn", meta = (ClampMin = "1"))
	int32 SpawnPointMaxAttempts = 10;

	/** Distancia en cm delante del jefe donde aparece el portal de salida. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossRoam|Spawn", meta = (ClampMin = "0.0"))
	float PortalForwardOffset = 100.f;

	// ── Visibilidad (para el punto de spawn inicial) ─────────────────────────

	/** Si es true, no spawnea en puntos que el jugador esté mirando. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossRoam|Visibility")
	bool bBlockSpawnIfPlayerFacing = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossRoam|Visibility",
	          meta = (ClampMin = "0.0", ClampMax = "180.0", EditCondition = "bBlockSpawnIfPlayerFacing"))
	float PlayerFOVAngleDegrees = 60.f;

	// ── Configuración ─────────────────────────────────────────────────────────

	/** Si es false, el sistema no arranca al hacer BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossRoam|Config")
	bool bStartEnabled = true;

	// ── Eventos ──────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "BossRoam|Events")
	FOnBossArrived OnBossArrived;

	UPROPERTY(BlueprintAssignable, Category = "BossRoam|Events")
	FOnBossReturning OnBossReturning;

	UPROPERTY(BlueprintAssignable, Category = "BossRoam|Events")
	FOnBossDeparted OnBossDeparted;

	// ── API estática (callable desde cualquier Blueprint sin referencia) ──────

	/** Activa o desactiva el sistema. Si se desactiva con el jefe visible, lo despawnea inmediatamente. */
	UFUNCTION(BlueprintCallable, Category = "BossRoam", meta = (WorldContext = "WorldContextObject"))
	static void SetSystemEnabled(UObject* WorldContextObject, bool bEnable);

	/** Devuelve la instancia activa del manager (null si no hay ninguna en escena). */
	UFUNCTION(BlueprintPure, Category = "BossRoam", meta = (WorldContext = "WorldContextObject"))
	static ABossRoamManager* Get(UObject* WorldContextObject);

	// ── API para el Blueprint del jefe ───────────────────────────────────────

	/** La IA llama esto cuando decide que el jefe debe irse. */
	UFUNCTION(BlueprintCallable, Category = "BossRoam")
	void RequestBossReturn();

	/** Forzar retorno inmediato (debugging, eventos de juego). */
	UFUNCTION(BlueprintCallable, Category = "BossRoam")
	void ForceReturn();

	UFUNCTION(BlueprintPure, Category = "BossRoam")
	EBossRoamState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "BossRoam")
	AActor* GetActiveBoss() const { return ActiveBoss.Get(); }

	UFUNCTION(BlueprintPure, Category = "BossRoam")
	bool IsEnabled() const { return bEnabled; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// ── Singleton ─────────────────────────────────────────────────────────────
	static TWeakObjectPtr<ABossRoamManager> ActiveInstance;

	// ── Estado runtime ────────────────────────────────────────────────────────
	EBossRoamState CurrentState = EBossRoamState::Idle;
	TWeakObjectPtr<AActor> ActiveBoss;
	FVector LastPortalLocation = FVector::ZeroVector;
	bool bEnabled = true;
	bool bFirstAppearance = true;

	// ── Timers ────────────────────────────────────────────────────────────────
	FTimerHandle AppearanceTimerHandle;
	FTimerHandle PortalBossSpawnHandle;
	FTimerHandle PortalDespawnHandle;
	FTimerHandle PlayerCheckHandle;

	// ── Ciclo principal ───────────────────────────────────────────────────────
	void ScheduleNextAppearance();
	void TrySpawnBoss();
	void SpawnBossAt(FVector Location);
	void TriggerReturn();
	void DespawnBoss();

	// ── Helpers ───────────────────────────────────────────────────────────────
	void SetEnabled(bool bEnable);
	void ReleaseBossToPool();
	void CheckPlayerAlive();
	bool IsAnyCombatArenaActive() const;
	bool FindValidSpawnPoint(FVector& OutLocation) const;
	void PlayPortalEffects(FVector Location) const;

	UFUNCTION()
	void OnBossDestroyedCallback(AActor* DestroyedActor);

	AActor* GetPlayerActor() const;
	UObjectPoolSubsystem* GetPool() const;
	void ClearAllTimers();
};
