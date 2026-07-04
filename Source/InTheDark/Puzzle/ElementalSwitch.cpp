#include "Puzzle/ElementalSwitch.h"
#include "Puzzle/ElementalGateDoor.h"
#include "Components/StaticMeshComponent.h"

static FName NormalizeElementName(FName ElementName)
{
	if (ElementName == TEXT("NewEnumerator0")) return TEXT("Air");
	if (ElementName == TEXT("NewEnumerator1")) return TEXT("Fire");
	if (ElementName == TEXT("NewEnumerator2")) return TEXT("Water");
	if (ElementName == TEXT("NewEnumerator3")) return TEXT("Earth");
	if (ElementName == TEXT("NewEnumerator4")) return TEXT("AirFire");

	return ElementName;
}

AElementalSwitch::AElementalSwitch()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	MeshComponent->SetGenerateOverlapEvents(true);
}

void AElementalSwitch::OnElementHit_Implementation(FName ElementName, AActor* HitInstigator)
{
	if (bActivated) return;
	if (RequiredElement.IsNone() || NormalizeElementName(ElementName) != RequiredElement) return;

	bActivated = true;

	if (MeshComponent)
	{
		MeshComponent->SetVisibility(false, true);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	OnSwitchActivated();

	if (OwningDoor) OwningDoor->NotifySwitchActivated();
}
