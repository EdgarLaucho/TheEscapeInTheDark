#include "EncounterSystem/SpawnArea.h"
#include "EncounterSystem/SpawnAnchor.h"
#include "ObjectPool/ObjectPoolSubsystem.h"
#include "AI/LeashComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BillboardComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

namespace
{
	float GetSpawnFloorOffset(TSubclassOf<AActor> EnemyClass)
	{
		const AActor* ClassDefault = EnemyClass ? EnemyClass->GetDefaultObject<AActor>() : nullptr;
		const UCapsuleComponent* Capsule = ClassDefault ? ClassDefault->FindComponentByClass<UCapsuleComponent>() : nullptr;
		return Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 0.f;
	}

	FTransform BuildGroundedSpawnTransform(UWorld* World, TSubclassOf<AActor> EnemyClass, const FTransform& SourceTransform, const AActor* IgnoredActor)
	{
		FTransform Result = SourceTransform;
		if (!World || !EnemyClass)
		{
			return Result;
		}

		const float FloorOffset = GetSpawnFloorOffset(EnemyClass);
		if (FloorOffset <= 0.f)
		{
			return Result;
		}

		FVector Location = Result.GetLocation();
		const FVector TraceStart = Location + FVector(0.f, 0.f, FMath::Max(500.f, FloorOffset + 200.f));
		const FVector TraceEnd = Location - FVector(0.f, 0.f, 5000.f);

		FHitResult Hit;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SpawnAreaGroundTrace), false);
		if (IgnoredActor)
		{
			QueryParams.AddIgnoredActor(IgnoredActor);
		}

		if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
		{
			Location.Z = Hit.ImpactPoint.Z + FloorOffset + 2.f;
			Result.SetLocation(Location);
		}

		return Result;
	}
}

ASpawnArea::ASpawnArea()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	ActivationVolume = CreateDefaultSubobject<USphereComponent>(TEXT("ActivationVolume"));
	ActivationVolume->SetupAttachment(Root);
	ActivationVolume->SetSphereRadius(1500.f);
	ActivationVolume->SetCollisionProfileName(TEXT("Trigger"));
	ActivationVolume->SetGenerateOverlapEvents(true);

#if WITH_EDITORONLY_DATA
	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	Billboard->SetupAttachment(Root);
	Billboard->bIsScreenSizeScaled = true;
#endif
}

// ── Ciclo de vida ────────────────────────────────────────────────────────────

void ASpawnArea::BeginPlay()
{
	Super::BeginPlay();

	ActivationVolume->SetSphereRadius(Rules.AreaRadius);

	ActivationVolume->OnComponentBeginOverlap.AddDynamic(this, &ASpawnArea::OnOverlapBegin);
	ActivationVolume->OnComponentEndOverlap.AddDynamic(this, &ASpawnArea::OnOverlapEnd);

	RefreshPlayerInsideState();
}

void ASpawnArea::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DespawnAll();
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(LeashTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(DespawnCheckHandle);
	Super::EndPlay(EndPlayReason);
}

void ASpawnArea::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (ActivationVolume)
	{
		ActivationVolume->SetSphereRadius(Rules.AreaRadius);
	}
}

// ── Activación / desactivación ───────────────────────────────────────────────

void ASpawnArea::Activate()
{
	SpawnQuota = 0;
	GetWorld()->GetTimerManager().ClearTimer(DespawnCheckHandle);

	ApplyLeashStateToActiveEnemies();

	GetWorld()->GetTimerManager().SetTimer(
		SpawnTimerHandle, this, &ASpawnArea::TrySpawn,
		Rules.SpawnInterval, /*bLoop=*/true, Rules.InitialSpawnDelay);

	GetWorld()->GetTimerManager().SetTimer(
		LeashTimerHandle, this, &ASpawnArea::EnforceLeash, 0.5f, /*bLoop=*/true);
}

void ASpawnArea::Deactivate()
{
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);

	ApplyLeashStateToActiveEnemies();

	if (!ActiveEnemies.IsEmpty())
	{
		GetWorld()->GetTimerManager().SetTimer(
			DespawnCheckHandle, this, &ASpawnArea::CheckDespawnOnLeave, 0.5f, /*bLoop=*/true);
	}
}

void ASpawnArea::SetPlayerInside(bool bNewPlayerInside, bool bBroadcastEvents)
{
	if (bPlayerInside == bNewPlayerInside) { return; }

	bPlayerInside = bNewPlayerInside;

	if (bPlayerInside)
	{
		Activate();
		if (bBroadcastEvents)
		{
			OnPlayerEntered.Broadcast();
		}
	}
	else
	{
		Deactivate();
		if (bBroadcastEvents)
		{
			OnPlayerLeft.Broadcast();
		}
	}
}

void ASpawnArea::RefreshPlayerInsideState()
{
	SetPlayerInside(IsPlayerInsideArea());
}

bool ASpawnArea::IsPlayerInsideArea() const
{
	const AActor* Player = GetPlayerActor();
	if (!Player || !ActivationVolume) { return false; }

	if (ActivationVolume->IsOverlappingActor(Player))
	{
		return true;
	}

	const float Radius = ActivationVolume->GetScaledSphereRadius();
	return FVector::DistSquared(Player->GetActorLocation(), ActivationVolume->GetComponentLocation()) <= FMath::Square(Radius);
}

// ── API pública ──────────────────────────────────────────────────────────────

int32 ASpawnArea::GetActiveEnemyCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& W : ActiveEnemies)
	{
		if (W.IsValid()) { ++Count; }
	}
	return Count;
}

void ASpawnArea::ForceActivate()
{
	SetPlayerInside(true, false);
}

void ASpawnArea::ForceDeactivate()
{
	bPlayerInside = false;
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(LeashTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(DespawnCheckHandle);
	DespawnAll();
}

void ASpawnArea::DespawnAll()
{
	TArray<TWeakObjectPtr<AActor>> ToRelease = ActiveEnemies;
	for (TWeakObjectPtr<AActor>& W : ToRelease)
	{
		if (AActor* Enemy = W.Get())
		{
			ReleaseEnemy(Enemy);
		}
	}
	ActiveEnemies.Empty();
}

void ASpawnArea::AutoBindAnchorsInRadius()
{
	BoundAnchors.Empty();

	UWorld* World = GetWorld();
	if (!World) { return; }

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(World, ASpawnAnchor::StaticClass(), Found);

	const float RadiusSq = FMath::Square(Rules.AreaRadius);
	for (AActor* Actor : Found)
	{
		if (FVector::DistSquared(Actor->GetActorLocation(), GetActorLocation()) <= RadiusSq)
		{
			BoundAnchors.Add(Cast<ASpawnAnchor>(Actor));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ASpawnArea [%s]: AutoBind encontró %d anchors dentro del radio."),
		*GetName(), BoundAnchors.Num());
}

// ── Callbacks de overlap ─────────────────────────────────────────────────────

void ASpawnArea::OnOverlapBegin(UPrimitiveComponent* /*Comp*/, AActor* Other,
                                UPrimitiveComponent* /*OtherComp*/, int32 /*BodyIndex*/,
                                bool /*bFromSweep*/, const FHitResult& /*Hit*/)
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || Other != PC->GetPawn()) { return; }

	SetPlayerInside(true);
}

void ASpawnArea::OnOverlapEnd(UPrimitiveComponent* /*Comp*/, AActor* Other,
                              UPrimitiveComponent* /*OtherComp*/, int32 /*BodyIndex*/)
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || Other != PC->GetPawn()) { return; }

	RefreshPlayerInsideState();
}

void ASpawnArea::OnEnemyDestroyed(AActor* DestroyedActor)
{
	ActiveEnemies.RemoveAll([DestroyedActor](const TWeakObjectPtr<AActor>& W)
	{
		return W.Get() == DestroyedActor;
	});

	// Si ya no quedan enemigos y el jugador tampoco está, notificar área despejada.
	CleanDeadEntries();
	if (ActiveEnemies.IsEmpty() && !bPlayerInside)
	{
		GetWorld()->GetTimerManager().ClearTimer(LeashTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(DespawnCheckHandle);
		OnAreaCleared.Broadcast();
	}
}

// ── Lógica periódica ─────────────────────────────────────────────────────────

void ASpawnArea::TrySpawn()
{
	CleanDeadEntries();

	if (ActiveEnemies.Num() >= Rules.MaxSimultaneous) { return; }

	// Si no respawnea en muerte, parar cuando el cupo inicial esté cubierto.
	if (!Rules.bRespawnOnDeath && SpawnQuota >= Rules.MaxSimultaneous)
	{
		GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
		return;
	}

	TSubclassOf<AActor> EnemyClass = PickEnemyClass();
	if (!EnemyClass) { return; }

	// Determinar transform de spawn.
	FTransform SpawnTransform;
	bool bFound = false;

	if (!BoundAnchors.IsEmpty())
	{
		const AActor* Player = GetPlayerActor();
		TArray<ASpawnAnchor*> Available;
		for (const TObjectPtr<ASpawnAnchor>& Anchor : BoundAnchors)
		{
			if (Anchor && Anchor->IsAvailableForSpawn(Player))
			{
				Available.Add(Anchor);
			}
		}
		if (!Available.IsEmpty())
		{
			ASpawnAnchor* Chosen = Available[FMath::RandRange(0, Available.Num() - 1)];
			SpawnTransform = Chosen->GetActorTransform();
			bFound = true;
		}
	}

	if (!bFound)
	{
		bFound = GetRandomSpawnTransform(SpawnTransform);
	}

	if (!bFound) { return; }

	SpawnTransform = BuildGroundedSpawnTransform(GetWorld(), EnemyClass, SpawnTransform, this);

	// Intentar adquirir del pool; si falla, spawn directo.
	AActor* Spawned = nullptr;
	if (UObjectPoolSubsystem* Pool = GetPool())
	{
		Spawned = Pool->AcquireFromPool(this, EnemyClass, SpawnTransform);
	}
	if (!Spawned)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Spawned = GetWorld()->SpawnActor<AActor>(EnemyClass, SpawnTransform, Params);
	}

	if (IsValid(Spawned))
	{
		Spawned->OnDestroyed.AddDynamic(this, &ASpawnArea::OnEnemyDestroyed);
		ActiveEnemies.Add(Spawned);
		ApplyLeashState(Spawned);
		++SpawnQuota;
	}
}

void ASpawnArea::EnforceLeash()
{
	RefreshPlayerInsideState();
	CleanDeadEntries();

	if (ActiveEnemies.IsEmpty())
	{
		if (!bPlayerInside)
			GetWorld()->GetTimerManager().ClearTimer(LeashTimerHandle);
		return;
	}

	// Catch-up: asegura el estado correcto si algún enemigo spawneó
	// después del cambio de bPlayerInside (evita el delay del timer).
	for (const TWeakObjectPtr<AActor>& WeakEnemy : ActiveEnemies)
	{
		AActor* Enemy = WeakEnemy.Get();
		if (!Enemy) { continue; }
		ApplyLeashState(Enemy);
	}
}

void ASpawnArea::CheckDespawnOnLeave()
{
	// Si el jugador volvió al overlap, el timer ya fue cancelado en Activate().
	if (bPlayerInside)
	{
		GetWorld()->GetTimerManager().ClearTimer(DespawnCheckHandle);
		return;
	}

	const AActor* Player = GetPlayerActor();
	if (!Player) { return; }

	// Solo despawnear cuando el jugador supera AreaRadius + DespawnOffset.
	const float DespawnRadiusSq = FMath::Square(Rules.AreaRadius + Rules.DespawnOffset);
	if (FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) < DespawnRadiusSq)
	{
		return;
	}

	CleanDeadEntries();

	TArray<AActor*> ToRelease;
	for (const TWeakObjectPtr<AActor>& WeakEnemy : ActiveEnemies)
	{
		AActor* Enemy = WeakEnemy.Get();
		if (!Enemy) { continue; }

		if (!Rules.bRequireOutOfSightToDespawn || !IsVisibleToPlayer(Enemy))
		{
			ToRelease.Add(Enemy);
		}
	}

	for (AActor* Enemy : ToRelease)
	{
		ReleaseEnemy(Enemy);
	}

	CleanDeadEntries();

	if (ActiveEnemies.IsEmpty())
	{
		GetWorld()->GetTimerManager().ClearTimer(DespawnCheckHandle);
		GetWorld()->GetTimerManager().ClearTimer(LeashTimerHandle);
		OnAreaCleared.Broadcast();
	}
}

// ── Helpers ──────────────────────────────────────────────────────────────────

void ASpawnArea::ReleaseEnemy(AActor* Enemy)
{
	if (!Enemy) { return; }

	Enemy->OnDestroyed.RemoveDynamic(this, &ASpawnArea::OnEnemyDestroyed);
	ActiveEnemies.RemoveAll([Enemy](const TWeakObjectPtr<AActor>& W) { return W.Get() == Enemy; });

	if (UObjectPoolSubsystem* Pool = GetPool())
	{
		Pool->ReleaseToPool(Enemy);
	}
	else
	{
		Enemy->Destroy();
	}
}

void ASpawnArea::ApplyLeashState(AActor* Enemy) const
{
	if (!Enemy) { return; }

	ULeashComponent* Leash = Enemy->FindComponentByClass<ULeashComponent>();
	if (!Leash) { return; }

	if (bPlayerInside)
	{
		if (Leash->IsLeashActive())
		{
			Leash->DeactivateLeash();
		}
	}
	else
	{
		// Solo activar una vez con el centro del área como origen del random walk.
		if (!Leash->IsLeashActive())
		{
			Leash->ActivateLeash(GetActorLocation());
		}
	}
}

FVector ASpawnArea::GetClosestPointInAreaToPlayer() const
{
	const FVector AreaCenter = GetActorLocation();

	const AActor* Player = GetPlayerActor();
	if (!Player) { return AreaCenter; }

	// Margen interior para que el destino quede claramente DENTRO del área
	// (no pegado al borde) y los enemigos no terminen sobre el límite del leash.
	static constexpr float InnerMargin = 150.f;
	const float MaxDist = FMath::Max(0.f, Rules.AreaRadius - InnerMargin);

	// Dirección horizontal del centro del área hacia el jugador.
	FVector ToPlayer = Player->GetActorLocation() - AreaCenter;
	ToPlayer.Z = 0.f;
	const float DistToPlayer = ToPlayer.Size();
	const FVector Dir = DistToPlayer > KINDA_SMALL_NUMBER ? ToPlayer / DistToPlayer : GetActorForwardVector();

	// Punto del área más cercano al jugador: como el jugador está fuera al salir,
	// es el del borde en su dirección, recortado por el margen interior.
	const FVector Point = AreaCenter + Dir * FMath::Min(DistToPlayer, MaxDist);

	// Proyectar al NavMesh para garantizar que el destino sea alcanzable; si no
	// hay navegación o no encuentra punto cercano, se usa el calculado.
	if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation Projected;
		const FVector QueryExtent(250.f, 250.f, 500.f);
		if (NavSys->ProjectPointToNavigation(Point, Projected, QueryExtent))
		{
			return Projected.Location;
		}
	}

	return Point;
}

void ASpawnArea::ApplyLeashStateToActiveEnemies() const
{
	for (const TWeakObjectPtr<AActor>& WeakEnemy : ActiveEnemies)
	{
		ApplyLeashState(WeakEnemy.Get());
	}
}

bool ASpawnArea::IsVisibleToPlayer(const AActor* Enemy) const
{
	const AActor* Player = GetPlayerActor();
	if (!Player || !Enemy) { return false; }

	const FVector ToEnemy = (Enemy->GetActorLocation() - Player->GetActorLocation()).GetSafeNormal();
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(Rules.VisibilityConeHalfAngle));
	return FVector::DotProduct(Player->GetActorForwardVector(), ToEnemy) > CosHalfAngle;
}

bool ASpawnArea::GetRandomSpawnTransform(FTransform& OutTransform) const
{
	const AActor* Player = GetPlayerActor();
	const FVector Center = GetActorLocation();
	static constexpr float MinSpawnDist = 300.f;
	static constexpr float MinPlayerDist = 300.f;

	for (int32 Attempt = 0; Attempt < 8; ++Attempt)
	{
		// Punto aleatorio en el disco horizontal del área.
		const FVector2D RandDir = FMath::RandPointInCircle(1.f);
		const float Dist = FMath::RandRange(MinSpawnDist, Rules.AreaRadius);
		FVector Candidate = Center + FVector(RandDir.X, RandDir.Y, 0.f) * Dist;
		Candidate.Z = Center.Z + 100.f;

		// Traza al suelo para pegar el spawn al terreno.
		if (UWorld* World = GetWorld())
		{
			FHitResult Hit;
			if (World->LineTraceSingleByChannel(Hit, Candidate, Candidate - FVector(0.f, 0.f, 1000.f), ECC_WorldStatic))
			{
				Candidate = Hit.ImpactPoint + FVector(0.f, 0.f, 10.f);
			}
		}

		// Rechazar si está demasiado cerca del jugador.
		if (Player && FVector::DistSquared(Candidate, Player->GetActorLocation()) < FMath::Square(MinPlayerDist))
		{
			continue;
		}

		const float Yaw = FMath::RandRange(0.f, 360.f);
		OutTransform = FTransform(FRotator(0.f, Yaw, 0.f), Candidate);
		return true;
	}

	return false;
}

TSubclassOf<AActor> ASpawnArea::PickEnemyClass() const
{
	struct FEligible { TSubclassOf<AActor> Class; float Weight; };
	TArray<FEligible> Eligible;
	float TotalWeight = 0.f;

	for (const FSpawnAreaEntry& Entry : EnemyEntries)
	{
		if (!Entry.EnemyClass) { continue; }
		if (CountActiveOfClass(Entry.EnemyClass) < Entry.MaxCount)
		{
			Eligible.Add({ Entry.EnemyClass, Entry.Weight });
			TotalWeight += Entry.Weight;
		}
	}

	if (Eligible.IsEmpty() || TotalWeight <= 0.f) { return nullptr; }

	float Rand = FMath::RandRange(0.f, TotalWeight);
	for (const FEligible& E : Eligible)
	{
		Rand -= E.Weight;
		if (Rand <= 0.f) { return E.Class; }
	}

	return Eligible.Last().Class;
}

int32 ASpawnArea::CountActiveOfClass(TSubclassOf<AActor> Class) const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& W : ActiveEnemies)
	{
		const AActor* A = W.Get();
		if (A && A->GetClass()->IsChildOf(Class)) { ++Count; }
	}
	return Count;
}

void ASpawnArea::CleanDeadEntries()
{
	ActiveEnemies.RemoveAll([](const TWeakObjectPtr<AActor>& W) { return !W.IsValid(); });
}

AActor* ASpawnArea::GetPlayerActor() const
{
	const UWorld* World = GetWorld();
	if (!World) { return nullptr; }
	const APlayerController* PC = World->GetFirstPlayerController();
	return PC ? PC->GetPawn() : nullptr;
}

UObjectPoolSubsystem* ASpawnArea::GetPool() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UObjectPoolSubsystem>() : nullptr;
}
