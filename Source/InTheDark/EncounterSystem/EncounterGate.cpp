#include "EncounterSystem/EncounterGate.h"
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
	bLocked = bStartLocked;
	ApplyLockState();
}

void AEncounterGate::Lock()
{
	if (bLocked) { return; }
	bLocked = true;
	ApplyLockState();
	OnGateLocked();
}

void AEncounterGate::Unlock()
{
	if (!bLocked) { return; }
	bLocked = false;
	if (MeshComponent)
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	OnGateUnlocked();
}

void AEncounterGate::FinishUnlock()
{
	if (MeshComponent)
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetVisibility(false, true);
	}
}

void AEncounterGate::ApplyLockState()
{
	if (MeshComponent)
	{
		MeshComponent->SetCollisionEnabled(bLocked ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		MeshComponent->SetVisibility(bLocked, true);
	}
}
