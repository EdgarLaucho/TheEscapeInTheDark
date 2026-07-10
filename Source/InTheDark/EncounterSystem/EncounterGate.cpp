#include "EncounterSystem/EncounterGate.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"

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
	if (bLocked) return;

	bLocked = true;
	SetGateCollisionEnabled(true);
	OnGateLocked();
}

void AEncounterGate::Unlock()
{
	if (!bLocked) return;

	bLocked = false;
	SetGateCollisionEnabled(false);
	OnGateUnlocked();
}

void AEncounterGate::SetLockedInstant(bool bNewLocked)
{
	bLocked = bNewLocked;
	ApplyLockState();
}

void AEncounterGate::ApplyLockState()
{
	SetGateCollisionEnabled(bLocked);
	SnapGateToState(bLocked);
}

void AEncounterGate::SnapGateToState(bool bClosed)
{
	if (MeshComponent)
		MeshComponent->SetRelativeLocation(bClosed ? ClosedRelativeLocation : GetOpenRelativeLocation());
}

void AEncounterGate::SetGateCollisionEnabled(bool bEnabled)
{
	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(this);

	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		if (!Primitive) continue;

		Primitive->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
}

FVector AEncounterGate::GetOpenRelativeLocation() const
{
	return ClosedRelativeLocation + FVector(0.f, 0.f, -SinkDepthOffset);
}