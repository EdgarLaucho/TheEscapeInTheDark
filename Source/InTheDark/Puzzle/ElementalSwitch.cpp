#include "Puzzle/ElementalSwitch.h"
#include "Puzzle/ElementalGateDoor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

static FName NormalizeElementName(FName ElementName)
{
	FString AsString = ElementName.ToString();

	int32 SeparatorIndex;
	if (AsString.FindLastChar(TEXT(':'), SeparatorIndex))
	{
		AsString = AsString.Mid(SeparatorIndex + 1);
	}

	if (AsString == TEXT("NewEnumerator0")) return TEXT("Air");
	if (AsString == TEXT("NewEnumerator1")) return TEXT("Fire");
	if (AsString == TEXT("NewEnumerator2")) return TEXT("Water");
	if (AsString == TEXT("NewEnumerator3")) return TEXT("Earth");
	if (AsString == TEXT("NewEnumerator4")) return TEXT("AirFire");

	return FName(*AsString);
}

AElementalSwitch::AElementalSwitch()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
	HitBox->SetupAttachment(MeshComponent);
	HitBox->SetBoxExtent(FVector(150.f, 150.f, 150.f));
	HitBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	HitBox->SetGenerateOverlapEvents(true);
}

void AElementalSwitch::OnElementHit_Implementation(FName ElementName, AActor* HitInstigator)
{
	if (bActivated) return;
	if (RequiredElement.IsNone() || NormalizeElementName(ElementName) != RequiredElement) return;

	bActivated = true;
	ApplyActivatedState();
	OnSwitchActivated();

	if (OwningDoor) OwningDoor->NotifySwitchActivated();
}

void AElementalSwitch::RestoreActivatedState()
{
	bActivated = true;
	ApplyActivatedState();
}

void AElementalSwitch::ApplyActivatedState()
{
	if (MeshComponent)
	{
		MeshComponent->SetVisibility(false, true);
	}

	if (HitBox)
	{
		HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}