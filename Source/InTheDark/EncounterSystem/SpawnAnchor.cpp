#include "EncounterSystem/SpawnAnchor.h"
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "ObjectPool/ObjectPoolSubsystem.h"

namespace
{
	float GetEncounterSpawnFloorOffset(TSubclassOf<AActor> EnemyClass)
	{
		const AActor* ClassDefault = EnemyClass ? EnemyClass->GetDefaultObject<AActor>() : nullptr;
		const UCapsuleComponent* Capsule = ClassDefault ? ClassDefault->FindComponentByClass<UCapsuleComponent>() : nullptr;
		return Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 0.f;
	}

	FTransform BuildEncounterGroundedSpawnTransform(UWorld* World, TSubclassOf<AActor> EnemyClass, const FTransform& SourceTransform, const AActor* IgnoredActor)
	{
		FTransform Result = SourceTransform;
		if (!World || !EnemyClass)
		{
			return Result;
		}

		const float FloorOffset = GetEncounterSpawnFloorOffset(EnemyClass);
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

bool ASpawnAnchor::IsAvailableForSpawn(const AActor* PlayerActor) const
{
	const UWorld* World = GetWorld();
	if (!World) { return false; }

	// Comprueba el cooldown.
	if (PointCooldown > 0.f)
	{
		const float Now = World->GetTimeSeconds();
		if (Now - LastSpawnTimeSeconds < PointCooldown)
		{
			return false;
		}
	}

	if (!PlayerActor) { return true; }

	const FVector Delta = GetActorLocation() - PlayerActor->GetActorLocation();
	const float DistSq = Delta.SizeSquared();

	if (MinDistanceToPlayer > 0.f && DistSq < FMath::Square(MinDistanceToPlayer))
	{
		return false;
	}

	if (bBlockIfPlayerInFOV && PlayerFOVAngleDegrees > 0.f)
	{
		const FVector ViewDir = PlayerActor->GetActorForwardVector().GetSafeNormal();
		const FVector ToAnchor = Delta.GetSafeNormal();
		const float CosHalfFOV = FMath::Cos(FMath::DegreesToRadians(PlayerFOVAngleDegrees));
		if (FVector::DotProduct(ViewDir, ToAnchor) > CosHalfFOV)
		{
			// Dentro del cono de FOV -> visible, bloquear.
			return false;
		}
	}

	return true;
}

float ASpawnAnchor::ResolveLead(const FEnemySpawn& Directive) const
{
	return Directive.PreSpawnLead > 0.f ? Directive.PreSpawnLead : DefaultLeadTime;
}

void ASpawnAnchor::PlayTelegraph(const FEnemySpawn& Directive) const
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	const TSoftObjectPtr<UNiagaraSystem>& VfxSoft = Directive.PreSpawnVFXOverride.IsNull()
		? DefaultPreSpawnVFX : Directive.PreSpawnVFXOverride;

	if (!VfxSoft.IsNull())
	{
		UNiagaraSystem* Vfx = VfxSoft.LoadSynchronous();
		if (Vfx)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World, Vfx, GetActorLocation(), GetActorRotation(),
					FVector(1.f), /*auto destruir*/ true);
		}
	}

	const TSoftObjectPtr<USoundBase>& SfxSoft = Directive.PreSpawnSFXOverride.IsNull()
		? DefaultPreSpawnSFX : Directive.PreSpawnSFXOverride;

	if (!SfxSoft.IsNull())
	{
		USoundBase* Sfx = SfxSoft.LoadSynchronous();
		if (Sfx)
		{
			UGameplayStatics::PlaySoundAtLocation(World, Sfx, GetActorLocation());
		}
	}
}

AActor* ASpawnAnchor::PerformSpawn(TSubclassOf<AActor> EnemyClass, const FEnemySpawn& Directive)
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
	const FTransform SpawnTransform = BuildEncounterGroundedSpawnTransform(World, EnemyClass, GetActorTransform(), this);

	// Intenta obtener del ObjectPool primero.
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UObjectPoolSubsystem* Pool = GI->GetSubsystem<UObjectPoolSubsystem>())
		{
			AActor* Acquired = Pool->AcquireFromPool(this, EnemyClass, SpawnTransform);
			if (Acquired)
			{
				return Acquired;
			}
				// Pool agotado o no registrado — caer al spawn directo.
		}
	}

	// Fallback: spawn directo.
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = GetOwner();

	AActor* Spawned = World->SpawnActor<AActor>(EnemyClass, SpawnTransform, Params);
	if (!Spawned)
	{
		UE_LOG(LogTemp, Error, TEXT("ASpawnAnchor::PerformSpawn: SpawnActor returned null for %s"),
			*EnemyClass->GetName());
	}
	return Spawned;
}
