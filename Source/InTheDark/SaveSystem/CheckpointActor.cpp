#include "SaveSystem/CheckpointActor.h"
#include "SaveSystem/InTheDarkGameInstance.h"
#include "SaveSystem/SaveTypes.h"
#include "Components/BoxComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ACheckpointActor::ACheckpointActor()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(200.f, 200.f, 200.f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
}

void ACheckpointActor::BeginPlay()
{
	Super::BeginPlay();
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ACheckpointActor::HandleOverlap);
}

void ACheckpointActor::HandleOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (CheckpointID.IsNone()) return;

	const UWorld* World = GetWorld();
	if (!World) return;

	const APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || OtherActor != PC->GetPawn()) return;

	UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(
		UGameplayStatics::GetGameInstance(World));
	if (!GI) return;

	// Copia el estado actual del jugador (salud ya sincronizada por BP_Player) y actualiza la posición.
	FSavedPlayerState State = GI->GetPlayerState();
	State.Transform = OtherActor->GetActorTransform();
	State.LastCheckpointID = CheckpointID;

	GI->SaveAtCheckpoint(CheckpointID, State);
	OnCheckpointReached(OtherActor);
}
