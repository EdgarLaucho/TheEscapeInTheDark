#pragma once

#include "CoreMinimal.h"
#include "SpawnAreaTypes.generated.h"

// Una entrada de tipo de enemigo para el SpawnArea.
USTRUCT(BlueprintType)
struct INTHEDARK_API FSpawnAreaEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSubclassOf<AActor> EnemyClass;

	// Máximo de esta clase vivos simultáneamente dentro del área.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = 1))
	int32 MaxCount = 2;

	// Peso relativo de selección. Mayor = se elige con más frecuencia.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = 0.1f))
	float Weight = 1.f;
};

// Reglas de spawn y comportamiento del área.
USTRUCT(BlueprintType)
struct INTHEDARK_API FSpawnAreaRules
{
	GENERATED_BODY()

	// Radio de la esfera (cm): zona de activación + leash de enemigos.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area", meta = (ClampMin = 100.f))
	float AreaRadius = 1500.f;

	// Distancia extra más allá del radio a la que el jugador dispara el despawn.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area", meta = (ClampMin = 0.f))
	float DespawnOffset = 500.f;

	// Cap total de enemigos vivos al mismo tiempo en esta área.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (ClampMin = 1))
	int32 MaxSimultaneous = 4;

	// Segundos entre intentos de spawn mientras el jugador esté dentro.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (ClampMin = 0.5f))
	float SpawnInterval = 5.f;

	// Retraso antes del primer spawn al entrar el jugador.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (ClampMin = 0.f))
	float InitialSpawnDelay = 1.f;

	// Si es true, los enemigos muertos se respawnean tras SpawnInterval.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	bool bRespawnOnDeath = true;

	// Si es true, espera a que el enemigo no sea visible antes de despawnearlo.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Despawn")
	bool bRequireOutOfSightToDespawn = true;

	// Semisángulo (grados) del cono de visión del jugador para el check de visibilidad.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Despawn", meta = (ClampMin = 10.f, ClampMax = 180.f))
	float VisibilityConeHalfAngle = 60.f;
};
