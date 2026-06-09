#include "EncounterSystem/BossRoamManager.h"
#include "EncounterSystem/CombatArena.h"
#include "EncounterSystem/EncounterDirectorComponent.h"
#include "ObjectPool/ObjectPoolSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"

TWeakObjectPtr<ABossRoamManager> ABossRoamManager::ActiveInstance;

ABossRoamManager::ABossRoamManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

// ── Ciclo de vida ────────────────────────────────────────────────────────────

void ABossRoamManager::BeginPlay()
{
	Super::BeginPlay();
	ActiveInstance = this;
	bEnabled = bStartEnabled;

	if (bEnabled)
	{
		ScheduleNextAppearance();
	}
}

void ABossRoamManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ActiveInstance.Get() == this)
	{
		ActiveInstance.Reset();
	}

	ClearAllTimers();

	if (AActor* Boss = ActiveBoss.Get())
	{
		Boss->OnDestroyed.RemoveDynamic(this, &ABossRoamManager::OnBossDestroyedCallback);
		if (UObjectPoolSubsystem* Pool = GetPool())
			Pool->ReleaseToPool(Boss);
		else
			Boss->Destroy();
	}

	Super::EndPlay(EndPlayReason);
}

// ── API estática ──────────────────────────────────────────────────────────────

ABossRoamManager* ABossRoamManager::Get(UObject* /*WorldContextObject*/)
{
	return ActiveInstance.Get();
}

void ABossRoamManager::SetSystemEnabled(UObject* /*WorldContextObject*/, bool bEnable)
{
	if (ABossRoamManager* Mgr = ActiveInstance.Get())
	{
		Mgr->SetEnabled(bEnable);
	}
}

// ── Enable / Disable ──────────────────────────────────────────────────────────

void ABossRoamManager::SetEnabled(bool bEnable)
{
	if (bEnabled == bEnable) { return; }
	bEnabled = bEnable;

	if (!bEnabled)
	{
		ClearAllTimers();
		ReleaseBossToPool();
		CurrentState = EBossRoamState::Idle;
	}
	else
	{
		ScheduleNextAppearance();
	}
}

// ── API para el Blueprint del jefe ───────────────────────────────────────────

void ABossRoamManager::RequestBossReturn()
{
	if (CurrentState == EBossRoamState::Hunting)
	{
		TriggerReturn();
	}
}

void ABossRoamManager::ForceReturn()
{
	if (CurrentState == EBossRoamState::Hunting)
	{
		TriggerReturn();
	}
}

// ── Lógica de aparición ──────────────────────────────────────────────────────

void ABossRoamManager::ScheduleNextAppearance()
{
	CurrentState = EBossRoamState::Idle;

	float Delay;
	if (bFirstAppearance)
	{
		Delay = InitialDelay;
		bFirstAppearance = false;
	}
	else
	{
		Delay = FMath::RandRange(MinAppearanceInterval, MaxAppearanceInterval);
	}

	GetWorld()->GetTimerManager().SetTimer(
		AppearanceTimerHandle, this, &ABossRoamManager::TrySpawnBoss, Delay, false);
}

void ABossRoamManager::TrySpawnBoss()
{
	if (CurrentState != EBossRoamState::Idle || !bEnabled) { return; }

	// No interrumpir encuentros de oleadas activos.
	if (IsAnyCombatArenaActive())
	{
		// Reintenta usando el intervalo normal en lugar de esperar otro ciclo completo.
		const float RetryDelay = FMath::RandRange(MinAppearanceInterval, MaxAppearanceInterval);
		GetWorld()->GetTimerManager().SetTimer(
			AppearanceTimerHandle, this, &ABossRoamManager::TrySpawnBoss, RetryDelay, false);
		return;
	}

	FVector SpawnPoint;
	if (!FindValidSpawnPoint(SpawnPoint))
	{
		UE_LOG(LogTemp, Warning, TEXT("ABossRoamManager [%s]: no se encontró punto de spawn válido. Reintentando."), *GetName());
		ScheduleNextAppearance();
		return;
	}

	CurrentState = EBossRoamState::Arriving;
	LastPortalLocation = SpawnPoint;

	PlayPortalEffects(SpawnPoint);

	FTimerDelegate SpawnDelegate;
	SpawnDelegate.BindLambda([this, SpawnPoint]() { SpawnBossAt(SpawnPoint); });
	GetWorld()->GetTimerManager().SetTimer(PortalBossSpawnHandle, SpawnDelegate, PortalOpenDuration, false);
}

void ABossRoamManager::SpawnBossAt(FVector Location)
{
	if (CurrentState != EBossRoamState::Arriving || !bEnabled) { return; }

	TSubclassOf<AActor> ResolvedClass = BossClass.LoadSynchronous();
	if (!ResolvedClass)
	{
		UE_LOG(LogTemp, Error, TEXT("ABossRoamManager [%s]: BossClass no está asignado."), *GetName());
		ScheduleNextAppearance();
		return;
	}

	const FTransform SpawnTransform(FRotator::ZeroRotator, Location);

	AActor* Boss = nullptr;
	if (UObjectPoolSubsystem* Pool = GetPool())
	{
		Boss = Pool->AcquireFromPool(this, ResolvedClass, SpawnTransform);
	}
	if (!Boss)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Boss = GetWorld()->SpawnActor<AActor>(ResolvedClass, SpawnTransform, Params);
	}

	if (!Boss)
	{
		UE_LOG(LogTemp, Error, TEXT("ABossRoamManager [%s]: no se pudo spawnear el jefe."), *GetName());
		ScheduleNextAppearance();
		return;
	}

	ActiveBoss = Boss;
	Boss->OnDestroyed.AddDynamic(this, &ABossRoamManager::OnBossDestroyedCallback);

	CurrentState = EBossRoamState::Hunting;

	OnBossArrived.Broadcast(Boss, Location);

	// Comprobación periódica de si el jugador sigue vivo.
	GetWorld()->GetTimerManager().SetTimer(
		PlayerCheckHandle, this, &ABossRoamManager::CheckPlayerAlive, 1.f, true);
}

// ── Lógica de retorno ────────────────────────────────────────────────────────

void ABossRoamManager::TriggerReturn()
{
	GetWorld()->GetTimerManager().ClearTimer(PlayerCheckHandle);

	FVector PortalLocation = LastPortalLocation;
	if (const AActor* Boss = ActiveBoss.Get())
	{
		PortalLocation = Boss->GetActorLocation()
			+ Boss->GetActorForwardVector() * PortalForwardOffset;
	}
	LastPortalLocation = PortalLocation;

	CurrentState = EBossRoamState::Returning;

	PlayPortalEffects(PortalLocation);
	OnBossReturning.Broadcast(ActiveBoss.Get(), PortalLocation);

	GetWorld()->GetTimerManager().SetTimer(
		PortalDespawnHandle, this, &ABossRoamManager::DespawnBoss, PortalOpenDuration, false);
}

void ABossRoamManager::DespawnBoss()
{
	ReleaseBossToPool();
	OnBossDeparted.Broadcast();

	if (bEnabled)
	{
		ScheduleNextAppearance();
	}
	else
	{
		CurrentState = EBossRoamState::Idle;
	}
}

// ── Helpers ──────────────────────────────────────────────────────────────────

void ABossRoamManager::ReleaseBossToPool()
{
	if (AActor* Boss = ActiveBoss.Get())
	{
		Boss->OnDestroyed.RemoveDynamic(this, &ABossRoamManager::OnBossDestroyedCallback);
		if (UObjectPoolSubsystem* Pool = GetPool())
			Pool->ReleaseToPool(Boss);
		else
			Boss->Destroy();
	}
	ActiveBoss.Reset();
}

void ABossRoamManager::CheckPlayerAlive()
{
	if (CurrentState != EBossRoamState::Hunting) { return; }

	if (GetPlayerActor() == nullptr)
	{
		// El jugador murió — desactivar el sistema sin programar nueva aparición.
		SetEnabled(false);
	}
}

bool ABossRoamManager::IsAnyCombatArenaActive() const
{
	TArray<AActor*> Arenas;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACombatArena::StaticClass(), Arenas);

	for (const AActor* Actor : Arenas)
	{
		const ACombatArena* Arena = Cast<ACombatArena>(Actor);
		if (!Arena || !Arena->Director) { continue; }

		const EEncounterState State = Arena->Director->GetState();
		if (State == EEncounterState::Starting  ||
		    State == EEncounterState::WaveActive ||
		    State == EEncounterState::PostClear)
		{
			return true;
		}
	}
	return false;
}

bool ABossRoamManager::FindValidSpawnPoint(FVector& OutLocation) const
{
	const AActor* Player = GetPlayerActor();
	if (!Player) { return false; }

	UWorld* World = GetWorld();
	const FVector PlayerPos = Player->GetActorLocation();
	const FVector ViewDir  = Player->GetActorForwardVector().GetSafeNormal();
	const float CosHalfFOV = FMath::Cos(FMath::DegreesToRadians(PlayerFOVAngleDegrees));

	for (int32 Attempt = 0; Attempt < SpawnPointMaxAttempts; ++Attempt)
	{
		const float Angle  = FMath::RandRange(0.f, 2.f * PI);
		const float Radius = FMath::RandRange(SpawnRadiusMin, SpawnRadiusMax);

		FVector Candidate = PlayerPos + FVector(FMath::Cos(Angle) * Radius,
		                                        FMath::Sin(Angle) * Radius, 0.f);
		Candidate.Z = PlayerPos.Z + 100.f;

		// Traza al suelo.
		if (World)
		{
			FHitResult GroundHit;
			if (World->LineTraceSingleByChannel(
				GroundHit, Candidate, Candidate - FVector(0.f, 0.f, 1000.f), ECC_WorldStatic))
			{
				Candidate = GroundHit.ImpactPoint + FVector(0.f, 0.f, 10.f);
			}
		}

		// Rechazar si hay techo (punto interior de un edificio).
		if (World)
		{
			FHitResult CeilingHit;
			const FVector TraceStart = Candidate + FVector(0.f, 0.f, 50.f);
			const FVector TraceEnd   = Candidate + FVector(0.f, 0.f, 30000.f);
			if (World->LineTraceSingleByChannel(CeilingHit, TraceStart, TraceEnd, ECC_WorldStatic))
			{
				continue;
			}
		}

		// Rechazar si el jugador mira hacia este punto.
		if (bBlockSpawnIfPlayerFacing)
		{
			const FVector ToCandidate = (Candidate - PlayerPos).GetSafeNormal();
			if (FVector::DotProduct(ViewDir, ToCandidate) > CosHalfFOV)
			{
				continue;
			}
		}

		OutLocation = Candidate;
		return true;
	}

	return false;
}

void ABossRoamManager::PlayPortalEffects(FVector Location) const
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	if (!PortalOpenVFX.IsNull())
	{
		UNiagaraSystem* Vfx = PortalOpenVFX.LoadSynchronous();
		if (Vfx)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World, Vfx, Location, FRotator::ZeroRotator, FVector(1.f), true);
		}
	}

	if (!PortalOpenSFX.IsNull())
	{
		USoundBase* Sfx = PortalOpenSFX.LoadSynchronous();
		if (Sfx)
		{
			UGameplayStatics::PlaySoundAtLocation(World, Sfx, Location);
		}
	}
}

void ABossRoamManager::OnBossDestroyedCallback(AActor* /*DestroyedActor*/)
{
	GetWorld()->GetTimerManager().ClearTimer(PlayerCheckHandle);
	GetWorld()->GetTimerManager().ClearTimer(PortalDespawnHandle);
	ActiveBoss.Reset();

	if (bEnabled)
	{
		ScheduleNextAppearance();
	}
	else
	{
		CurrentState = EBossRoamState::Idle;
	}
}

AActor* ABossRoamManager::GetPlayerActor() const
{
	const UWorld* World = GetWorld();
	if (!World) { return nullptr; }
	const APlayerController* PC = World->GetFirstPlayerController();
	return PC ? PC->GetPawn() : nullptr;
}

UObjectPoolSubsystem* ABossRoamManager::GetPool() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UObjectPoolSubsystem>() : nullptr;
}

void ABossRoamManager::ClearAllTimers()
{
	UWorld* World = GetWorld();
	if (!World) { return; }
	FTimerManager& TM = World->GetTimerManager();
	TM.ClearTimer(AppearanceTimerHandle);
	TM.ClearTimer(PortalBossSpawnHandle);
	TM.ClearTimer(PortalDespawnHandle);
	TM.ClearTimer(PlayerCheckHandle);
}
