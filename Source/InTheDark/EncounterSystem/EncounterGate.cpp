#include "EncounterSystem/EncounterGate.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

namespace
{
	float ReadFloatPropertyByName(const UObject* Object, FName PropertyName)
	{
		if (!Object)
		{
			return 0.f;
		}

		if (const FDoubleProperty* DoubleProperty = FindFProperty<FDoubleProperty>(Object->GetClass(), PropertyName))
		{
			return static_cast<float>(DoubleProperty->GetPropertyValue_InContainer(Object));
		}

		if (const FFloatProperty* FloatProperty = FindFProperty<FFloatProperty>(Object->GetClass(), PropertyName))
		{
			return FloatProperty->GetPropertyValue_InContainer(Object);
		}

		return 0.f;
	}
}

AEncounterGate::AEncounterGate()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(Root);
	MeshComponent->SetMobility(EComponentMobility::Movable);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

void AEncounterGate::BeginPlay()
{
	Super::BeginPlay();
	ClosedRelativeLocation = MeshComponent ? MeshComponent->GetRelativeLocation() : FVector::ZeroVector;
	bLocked = bStartLocked;
	ApplyLockState();
}

void AEncounterGate::Lock()
{
	if (bLocked) { return; }
	bLocked = true;
	SetGateCollisionEnabled(true);
	StartGateMove(true);
	OnGateLocked();
}

void AEncounterGate::Unlock()
{
	if (!bLocked) { return; }
	bLocked = false;
	SetGateCollisionEnabled(false);
	StartGateMove(false);
	OnGateUnlocked();
}

void AEncounterGate::FinishUnlock()
{
	SnapGateToState(false);
	SetGateCollisionEnabled(false);
}

void AEncounterGate::ApplyLockState()
{
	SetGateCollisionEnabled(bLocked);
	SnapGateToState(bLocked);
}

void AEncounterGate::SnapGateToState(bool bClosed)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GateMoveTimerHandle);
	}

	if (MeshComponent)
	{
		MeshComponent->SetRelativeLocation(bClosed ? ClosedRelativeLocation : GetOpenRelativeLocation());
	}
}

void AEncounterGate::StartGateMove(bool bClosed)
{
	if (!MeshComponent)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		SnapGateToState(bClosed);
		return;
	}

	MoveStartRelativeLocation = MeshComponent->GetRelativeLocation();
	MoveTargetRelativeLocation = bClosed ? ClosedRelativeLocation : GetOpenRelativeLocation();
	MoveElapsedSeconds = 0.f;
	MoveDurationSeconds = FMath::Max(0.f, GetConfiguredSinkTime());

	World->GetTimerManager().ClearTimer(GateMoveTimerHandle);
	if (MoveDurationSeconds <= KINDA_SMALL_NUMBER || MoveStartRelativeLocation.Equals(MoveTargetRelativeLocation))
	{
		MeshComponent->SetRelativeLocation(MoveTargetRelativeLocation);
		return;
	}

	World->GetTimerManager().SetTimer(GateMoveTimerHandle, this, &AEncounterGate::UpdateGateMove, 1.f / 60.f, true);
}

void AEncounterGate::UpdateGateMove()
{
	if (!MeshComponent)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(GateMoveTimerHandle);
		}

		return;
	}

	const UWorld* World = GetWorld();
	const float DeltaSeconds = World ? World->GetDeltaSeconds() : 1.f / 60.f;
	MoveElapsedSeconds += DeltaSeconds;

	const float Alpha = MoveDurationSeconds > KINDA_SMALL_NUMBER ? FMath::Clamp(MoveElapsedSeconds / MoveDurationSeconds, 0.f, 1.f) : 1.f;
	const float SmoothAlpha = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
	MeshComponent->SetRelativeLocation(FMath::Lerp(MoveStartRelativeLocation, MoveTargetRelativeLocation, SmoothAlpha));

	if (Alpha >= 1.f)
	{
		if (UWorld* MutableWorld = GetWorld())
		{
			MutableWorld->GetTimerManager().ClearTimer(GateMoveTimerHandle);
		}
		MeshComponent->SetRelativeLocation(MoveTargetRelativeLocation);
	}
}

void AEncounterGate::SetGateCollisionEnabled(bool bEnabled)
{
	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(this);
	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		if (!Primitive)
		{
			continue;
		}

		Primitive->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
}

FVector AEncounterGate::GetOpenRelativeLocation() const
{
	return ClosedRelativeLocation + FVector(0.f, 0.f, -GetConfiguredSinkDepthOffset());
}

float AEncounterGate::GetConfiguredSinkDepthOffset() const
{
	return ReadFloatPropertyByName(this, TEXT("SinkDepthOffset"));
}

float AEncounterGate::GetConfiguredSinkTime() const
{
	return ReadFloatPropertyByName(this, TEXT("SinkTime"));
}
