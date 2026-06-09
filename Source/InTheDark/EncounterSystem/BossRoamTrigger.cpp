#include "EncounterSystem/BossRoamTrigger.h"
#include "EncounterSystem/BossRoamManager.h"
#include "Components/BoxComponent.h"
#include "Components/BillboardComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

ABossRoamTrigger::ABossRoamTrigger()
{
	PrimaryActorTick.bCanEverTick = false;
	bNetLoadOnClient = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(Root);
	TriggerBox->SetBoxExtent(FVector(200.f, 200.f, 100.f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->SetGenerateOverlapEvents(true);

#if WITH_EDITORONLY_DATA
	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	if (Billboard)
	{
		Billboard->SetupAttachment(Root);
		Billboard->bIsScreenSizeScaled = true;
	}
#endif
}

void ABossRoamTrigger::BeginPlay()
{
	Super::BeginPlay();
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ABossRoamTrigger::OnOverlapBegin);
}

void ABossRoamTrigger::OnOverlapBegin(UPrimitiveComponent* /*OverlappedComp*/, AActor* Other,
                                       UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/,
                                       bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	if (bTriggerOnce && bAlreadyTriggered) { return; }

	// Comprobar que es el pawn del jugador.
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || Other != PC->GetPawn()) { return; }

	bAlreadyTriggered = true;
	ABossRoamManager::SetSystemEnabled(this, bEnableOnOverlap);
}
