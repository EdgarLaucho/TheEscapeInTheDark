#include "Puzzle/ElementalGateDoor.h"
#include "Puzzle/ElementalSwitch.h"
#include "SaveSystem/InTheDarkGameInstance.h"

#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

AElementalGateDoor::AElementalGateDoor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetMobility(EComponentMobility::Movable);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

void AElementalGateDoor::BeginPlay()
{
	Super::BeginPlay();

	ClosedRelativeLocation = MeshComponent ? MeshComponent->GetRelativeLocation() : FVector::ZeroVector;

	if (!DoorId.IsNone())
	{
		if (const UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(UGameplayStatics::GetGameInstance(this)))
			bOpen = GI->IsElementalGateOpened(DoorId.ToString());
	}

	ApplyOpenState();

	if (bOpen)
	{
		for (const TObjectPtr<AElementalSwitch>& Switch : Switches)
		{
			if (Switch) Switch->RestoreActivatedState();
		}
	}
}

void AElementalGateDoor::NotifySwitchActivated()
{
	if (bOpen || !AreAllSwitchesActivated()) return;

	Open();
}

bool AElementalGateDoor::AreAllSwitchesActivated() const
{
	if (Switches.IsEmpty()) return false;

	for (const TObjectPtr<AElementalSwitch>& Switch : Switches)
	{
		if (!Switch || !Switch->IsActivated()) return false;
	}

	return true;
}

void AElementalGateDoor::Open()
{
	if (bOpen) return;

	bOpen = true;
	SetGateCollisionEnabled(false);
	OnDoorOpened();

	if (!DoorId.IsNone())
	{
		if (UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(UGameplayStatics::GetGameInstance(this)))
		{
			GI->RegisterElementalGateOpened(DoorId.ToString());
			GI->WriteSaveToDisk();
		}
	}
}

void AElementalGateDoor::ApplyOpenState()
{
	SetGateCollisionEnabled(!bOpen);
	SnapGateToState(!bOpen);
}

void AElementalGateDoor::SnapGateToState(bool bClosed)
{
	if (MeshComponent)
		MeshComponent->SetRelativeLocation(bClosed ? ClosedRelativeLocation : GetOpenRelativeLocation());
}

void AElementalGateDoor::SetGateCollisionEnabled(bool bEnabled)
{
	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(this);

	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		if (!Primitive) continue;

		Primitive->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
}

FVector AElementalGateDoor::GetOpenRelativeLocation() const
{
	return ClosedRelativeLocation + FVector(0.f, 0.f, -SinkDepthOffset);
}
