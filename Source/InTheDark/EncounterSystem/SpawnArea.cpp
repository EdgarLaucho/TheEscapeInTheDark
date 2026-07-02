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
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	float GetSpawnAreaFloorOffset(TSubclassOf<AActor> EnemyClass)
	{
		const AActor* ClassDefault = EnemyClass ? Cast<AActor>(EnemyClass->GetDefaultObject()) : nullptr;
		const UCapsuleComponent* Capsule = ClassDefault ? ClassDefault->FindComponentByClass<UCapsuleComponent>() : nullptr;
		return Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 0.f;
	}

	FVector GetSpawnAreaDefaultScale(TSubclassOf<AActor> EnemyClass)
	{
		const AActor* ClassDefault = EnemyClass ? Cast<AActor>(EnemyClass->GetDefaultObject()) : nullptr;
		return ClassDefault ? ClassDefault->GetActorScale3D() : FVector::OneVector;
	}

	FTransform BuildSpawnAreaGroundedSpawnTransform(UWorld* World, TSubclassOf<AActor> EnemyClass, const FTransform& SourceTransform, const AActor* IgnoredActor)
	{
		FTransform Result = SourceTransform;
		Result.SetScale3D(GetSpawnAreaDefaultScale(EnemyClass));
		if (!World || !EnemyClass)
		{
			return Result;
		}

		const float FloorOffset = GetSpawnAreaFloorOffset(EnemyClass);
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

void ASpawnArea::Activate()
{
	SpawnQuota = 0;
	GetWorld()->GetTimerManager().ClearTimer(DespawnCheckHandle);

	ApplyLeashStateToActiveEnemies();

	GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &ASpawnArea::TrySpawn, Rules.SpawnInterval, true, Rules.InitialSpawnDelay);

	GetWorld()->GetTimerManager().SetTimer(LeashTimerHandle, this, &ASpawnArea::EnforceLeash, 0.5f, true);
}

void ASpawnArea::Deactivate()
{
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);

	ApplyLeashStateToActiveEnemies();

	if (!ActiveEnemies.IsEmpty())
	{
		GetWorld()->GetTimerManager().SetTimer(DespawnCheckHandle, this, &ASpawnArea::CheckDespawnOnLeave, 0.5f, true);
	}
}

void ASpawnArea::SetPlayerInside(bool bNewPlayerInside)
{
	if (bPlayerInside == bNewPlayerInside) { return; }

	bPlayerInside = bNewPlayerInside;

	if (bPlayerInside)
	{
		Activate();
	}
	else
	{
		Deactivate();
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

void ASpawnArea::OnOverlapBegin(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || Other != PC->GetPawn()) { return; }

	SetPlayerInside(true);
}

void ASpawnArea::OnOverlapEnd(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32)
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || Other != PC->GetPawn())
	{ 
		return; 
	}

	RefreshPlayerInsideState();
}

void ASpawnArea::OnEnemyDestroyed(AActor* DestroyedActor)
{
	ActiveEnemies.RemoveAll([DestroyedActor](const TWeakObjectPtr<AActor>& W)
	{
		return W.Get() == DestroyedActor;
	});

	CleanDeadEntries();
	if (ActiveEnemies.IsEmpty() && !bPlayerInside)
	{
		GetWorld()->GetTimerManager().ClearTimer(LeashTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(DespawnCheckHandle);
	}
}

void ASpawnArea::TrySpawn()
{
	CleanDeadEntries();

	if (ActiveEnemies.Num() >= Rules.MaxSimultaneous) 
	{ 
		return; 
	}

	if (!Rules.bRespawnOnDeath && SpawnQuota >= Rules.MaxSimultaneous)
	{
		GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
		return;
	}

	TSubclassOf<AActor> EnemyClass = PickEnemyClass();
	if (!EnemyClass) 
	{ 
		return; 
	}

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

	SpawnTransform = BuildSpawnAreaGroundedSpawnTransform(GetWorld(), EnemyClass, SpawnTransform, this);

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

	for (const TWeakObjectPtr<AActor>& WeakEnemy : ActiveEnemies)
	{
		AActor* Enemy = WeakEnemy.Get();

		if (!Enemy) 
		{ 
			continue; 
		}

		ApplyLeashState(Enemy);
	}
}

void ASpawnArea::CheckDespawnOnLeave()
{
	if (bPlayerInside)
	{
		GetWorld()->GetTimerManager().ClearTimer(DespawnCheckHandle);
		return;
	}

	const AActor* Player = GetPlayerActor();
	if (!Player) { return; }

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
		if (!Enemy) 
		{ 
			continue; 
		}

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
	}
}

void ASpawnArea::ReleaseEnemy(AActor* Enemy)
{
	if (!Enemy)
	{ 
		return; 
	}

	Enemy->OnDestroyed.RemoveDynamic(this, &ASpawnArea::OnEnemyDestroyed);
	ActiveEnemies.RemoveAll([Enemy](const TWeakObjectPtr<AActor>& W) 
	{ 
		return W.Get() == Enemy; 
	});

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
	if (!Enemy) 
	{ 
		return; 
	}

	ULeashComponent* Leash = Enemy->FindComponentByClass<ULeashComponent>();
	if (!Leash) 
	{ 
		return; 
	}

	if (bPlayerInside)
	{
		if (Leash->IsLeashActive())
		{
			Leash->DeactivateLeash();
		}
	}
	else
	{
		if (!Leash->IsLeashActive())
		{
			Leash->ActivateLeash(GetActorLocation());
		}
	}
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
	if (!Player || !Enemy) 
	{ 
		return false;
	}

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
		const FVector2D RandDir = FMath::RandPointInCircle(1.f);
		const float Dist = FMath::RandRange(MinSpawnDist, Rules.AreaRadius);
		FVector Candidate = Center + FVector(RandDir.X, RandDir.Y, 0.f) * Dist;
		Candidate.Z = Center.Z + 100.f;

		if (UWorld* World = GetWorld())
		{
			FHitResult Hit;
			if (World->LineTraceSingleByChannel(Hit, Candidate, Candidate - FVector(0.f, 0.f, 1000.f), ECC_WorldStatic))
			{
				Candidate = Hit.ImpactPoint + FVector(0.f, 0.f, 10.f);
			}
		}

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
		if (!Entry.EnemyClass) 
		{
			 continue; 
		}

		if (CountActiveOfClass(Entry.EnemyClass) < Entry.MaxCount)
		{
			Eligible.Add({ Entry.EnemyClass, Entry.Weight });
			TotalWeight += Entry.Weight;
		}
	}

	if (Eligible.IsEmpty() || TotalWeight <= 0.f) 
	{ 
		return nullptr; 
	}

	float Rand = FMath::RandRange(0.f, TotalWeight);
	for (const FEligible& E : Eligible)
	{
		Rand -= E.Weight;
		if (Rand <= 0.f) 
		{
			return E.Class;
		}
	}

	return Eligible.Last().Class;
}

int32 ASpawnArea::CountActiveOfClass(TSubclassOf<AActor> Class) const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& W : ActiveEnemies)
	{
		const AActor* A = W.Get();
		if (A && A->GetClass()->IsChildOf(Class))
		{ 
			++Count; 
		}
	}

	return Count;
}

void ASpawnArea::CleanDeadEntries()
{
	ActiveEnemies.RemoveAll([](const TWeakObjectPtr<AActor>& W)
	{ 
		return !W.IsValid(); 
	});
}

AActor* ASpawnArea::GetPlayerActor() const
{
	const UWorld* World = GetWorld();
	if (!World) 
	{ 
		return nullptr; 
	}

	const APlayerController* PC = World->GetFirstPlayerController();
	return PC ? PC->GetPawn() : nullptr;
}

UObjectPoolSubsystem* ASpawnArea::GetPool() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UObjectPoolSubsystem>() : nullptr;
}