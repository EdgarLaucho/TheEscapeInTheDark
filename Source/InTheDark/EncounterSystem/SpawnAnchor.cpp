#include "EncounterSystem/SpawnAnchor.h"
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "ObjectPool/ObjectPoolSubsystem.h"

FTransform ASpawnAnchor::BuildGroundedSpawnTransform(UWorld* World, TSubclassOf<AActor> EnemyClass, const FTransform& SourceTransform, const AActor* IgnoredActor)
{
	const AActor* ClassDefault = EnemyClass ? Cast<AActor>(EnemyClass->GetDefaultObject()) : nullptr;

	FTransform Result = SourceTransform;
	Result.SetScale3D(ClassDefault ? ClassDefault->GetActorScale3D() : FVector::OneVector);
	if (!World || !EnemyClass)
	{
		return Result;
	}

	const UCapsuleComponent* Capsule = ClassDefault ? ClassDefault->FindComponentByClass<UCapsuleComponent>() : nullptr;
	const float FloorOffset = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 0.f;
	if (FloorOffset <= 0.f)
	{
		return Result;
	}

	FVector Location = Result.GetLocation();
	const FVector TraceStart = Location + FVector(0.f, 0.f, FMath::Max(500.f, FloorOffset + 200.f));
	const FVector TraceEnd = Location - FVector(0.f, 0.f, 5000.f);

	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EncounterSpawnGroundTrace), false);
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

ASpawnAnchor::ASpawnAnchor()
{
	PrimaryActorTick.bCanEverTick = false;
	bNetLoadOnClient = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

#if WITH_EDITORONLY_DATA
	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	if (Billboard)
	{
		Billboard->SetupAttachment(Root);
		Billboard->bIsScreenSizeScaled = true;
	}

	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Facing"));
	if (Arrow)
	{
		Arrow->SetupAttachment(Root);
		Arrow->ArrowColor = FColor::Red;
		Arrow->ArrowSize = 1.f;
	}
#endif
}

void ASpawnAnchor::BeginPlay()
{
	Super::BeginPlay();
	LastSpawnTimeSeconds = -1e9f;
}

bool ASpawnAnchor::IsAvailableForSpawn() const
{
	const UWorld* World = GetWorld();
	if (!World) { return false; }

	if (PointCooldown > 0.f && World->GetTimeSeconds() - LastSpawnTimeSeconds < PointCooldown)
	{
		return false;
	}

	return true;
}

AActor* ASpawnAnchor::PerformSpawn(TSubclassOf<AActor> EnemyClass)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (!EnemyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASpawnAnchor::PerformSpawn: null EnemyClass at %s"), *GetName());
		return nullptr;
	}

	LastSpawnTimeSeconds = World->GetTimeSeconds();
	const FTransform SpawnTransform = BuildGroundedSpawnTransform(World, EnemyClass, GetActorTransform(), this);

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UObjectPoolSubsystem* Pool = GI->GetSubsystem<UObjectPoolSubsystem>())
		{
			AActor* Acquired = Pool->AcquireFromPool(this, EnemyClass, SpawnTransform);
			if (Acquired)
			{
				return Acquired;
			}
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = GetOwner();

	AActor* Spawned = World->SpawnActor<AActor>(EnemyClass, SpawnTransform, Params);
	
	if (!Spawned)
	{
		UE_LOG(LogTemp, Error, TEXT("ASpawnAnchor::PerformSpawn: SpawnActor returned null for %s"), *EnemyClass->GetName());
	}

	return Spawned;
}