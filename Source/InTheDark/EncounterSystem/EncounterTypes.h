#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/StreamableManager.h"
#include "EncounterTypes.generated.h"

class AActor;
class UNiagaraSystem;
class USoundBase;
class UAnimMontage;

/** Clasificación del encuentro. Determina la política de checkpoint y el telegrafeo. */
UENUM(BlueprintType)
enum class EEncounterKind : uint8
{
	/** Historia principal. Fallar recarga el último checkpoint. Suele estar bloqueado y tiene recompensa. */
	Story,
	/** Desafío opcional (runa de arena). Fallar no recarga checkpoint; se puede reintentar. */
	Challenge
};

/** Cómo decide el director que una oleada ha terminado y debe empezar la siguiente. */
UENUM(BlueprintType)
enum class EWaveContinuationMode : uint8
{
	/** Siguiente oleada solo cuando todos los enemigos de la oleada actual están muertos. */
	OnAllCleared,
	/** Siguiente oleada cuando queden N enemigos o menos vivos. */
	OnRemainingAtOrBelow,
	/** Siguiente oleada tras T segundos desde que comenzó la oleada actual. */
	OnElapsedSince,
	/** Siguiente oleada cuando restantes<=N O tiempo>=T (lo que ocurra primero). */
	Hybrid
};

/** Reglas que determinan cuándo una oleada encadena con la siguiente. */
USTRUCT(BlueprintType)
struct INTHEDARK_API FWaveContinuation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Continuation")
	EWaveContinuationMode Mode = EWaveContinuationMode::OnAllCleared;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Continuation",
		meta = (EditCondition = "Mode != EWaveContinuationMode::OnAllCleared"))
	int32 RemainingThreshold = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Continuation",
		meta = (EditCondition = "Mode != EWaveContinuationMode::OnAllCleared"))
	float ElapsedSeconds = 8.f;
};

/** Directiva de spawn individual dentro de una oleada. */
USTRUCT(BlueprintType)
struct INTHEDARK_API FEnemySpawn
{
	GENERATED_BODY()

	/** Clase de enemigo a spawnear. Soft para no cargar todos los paquetes de enemigos al inicio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSoftClassPtr<AActor> Enemy;

	/** Número de copias del enemigo a spawnear para esta directiva. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "1"))
	int32 Count = 1;

	/** Filtro por tag de anchor. El director elige anchors cuyo AnchorTags contenga este tag. Vacío = cualquiera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	FGameplayTag AnchorTag;

	/** Tag de rol (Role.Boss, Role.Elite, Role.Grunt). Usado por VFX/HUD. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	FGameplayTag RoleTag;

	/** Duración del telegrafeo (segundos) antes de que el enemigo aparezca en el anchor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "0.0"))
	float PreSpawnLead = 0.8f;

	/** VFX por spawn opcional (usa el del anchor por defecto si no se asigna). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
	TSoftObjectPtr<UNiagaraSystem> PreSpawnVFXOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
	TSoftObjectPtr<USoundBase> PreSpawnSFXOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emerge")
	TSoftObjectPtr<UAnimMontage> EmergeMontageOverride;
};

/** Oleada individual dentro de un encuentro. */
USTRUCT(BlueprintType)
struct INTHEDARK_API FEncounterWave
{
	GENERATED_BODY()

	/** Nombre visible solo para el diseñador. No aparece en juego. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FName WaveName = TEXT("Wave");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TArray<FEnemySpawn> Spawns;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	FWaveContinuation Continuation;

	/** Segundos de pausa antes de que esta oleada empiece a spawnear (tras limpiar la anterior). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "0.0"))
	float DelayBeforeWave = 0.f;

	/** Tag de diálogo disparado cuando la oleada empieza a spawnear. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banter")
	FGameplayTag BanterStartTag;

	/** Disparado cuando se cumple el criterio de finalización de la oleada. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banter")
	FGameplayTag BanterEndTag;
};

/** Descriptor de recompensa post-encuentro. */
USTRUCT(BlueprintType)
struct INTHEDARK_API FEncounterReward
{
	GENERATED_BODY()

	/** Actor de recompensa a spawnear en el anchor de recompensa del arena (cofre, pickup, altar). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	TSoftClassPtr<AActor> RewardClass;

	/** Tags a los que puede reaccionar el sistema de progresión (ej: "Reward.XPSmall"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	FGameplayTagContainer RewardTags;
};
