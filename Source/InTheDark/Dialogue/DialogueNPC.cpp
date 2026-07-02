#include "Dialogue/DialogueNPC.h"
#include "Dialogue/DialogueSubsystem.h"
#include "SaveSystem/InTheDarkGameInstance.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ADialogueNPC::ADialogueNPC()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerZone = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerZone"));
	SetRootComponent(TriggerZone);
	TriggerZone->SetSphereRadius(200.f);
	TriggerZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerZone->SetGenerateOverlapEvents(true);
}

void ADialogueNPC::BeginPlay()
{
	Super::BeginPlay();
	TriggerZone->OnComponentBeginOverlap.AddDynamic(this, &ADialogueNPC::HandleBeginOverlap);
	TriggerZone->OnComponentEndOverlap.AddDynamic(this, &ADialogueNPC::HandleEndOverlap);
}

void ADialogueNPC::HandleBeginOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	const UWorld* World = GetWorld();
	if (!World) return;
	const APlayerController* PC = World->GetFirstPlayerController();
	if (PC && OtherActor == PC->GetPawn())
		bPlayerInRange = true;
}

void ADialogueNPC::HandleEndOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32)
{
	const UWorld* World = GetWorld();
	if (!World) return;
	const APlayerController* PC = World->GetFirstPlayerController();
	if (PC && OtherActor == PC->GetPawn())
		bPlayerInRange = false;
}

bool ADialogueNPC::CanTriggerDialogue() const
{
	if (!DialogueData || DialogueID.IsNone()) return false;
	const UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	return GI && !GI->IsDialogueSeen(DialogueID);
}

void ADialogueNPC::TriggerDialogue()
{
	if (!bPlayerInRange || bTriggeredThisSession || !DialogueData || DialogueID.IsNone()) return;

	const UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;

	UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(
		UGameplayStatics::GetGameInstance(World));
	if (!GI || GI->IsDialogueSeen(DialogueID)) return;

	bTriggeredThisSession = true;

	if (UDialogueSubsystem* Sub = GI->GetSubsystem<UDialogueSubsystem>())
		Sub->StartDialogue(DialogueData, DialogueID, PC);
}