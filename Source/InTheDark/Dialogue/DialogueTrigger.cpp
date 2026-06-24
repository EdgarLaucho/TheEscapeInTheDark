#include "Dialogue/DialogueTrigger.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Dialogue/DialogueSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "SaveSystem/InTheDarkGameInstance.h"

ADialogueTrigger::ADialogueTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(SceneRoot);
	TriggerVolume->SetBoxExtent(FVector(200.f, 200.f, 120.f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);
}

void ADialogueTrigger::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerVolume)
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ADialogueTrigger::HandleBeginOverlap);
	}
}

void ADialogueTrigger::HandleBeginOverlap(
	UPrimitiveComponent*,
	AActor* OtherActor,
	UPrimitiveComponent*,
	int32,
	bool,
	const FHitResult&)
{
	if ((bTriggerOnce && bTriggeredThisSession) || !DialogueData || DialogueID.IsNone()) return;

	const UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || OtherActor != PC->GetPawn()) return;

	UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(UGameplayStatics::GetGameInstance(World));
	if (!GI) return;

	if (bSkipIfSeen && GI->IsDialogueSeen(DialogueID)) return;

	UDialogueSubsystem* DialogueSubsystem = GI->GetSubsystem<UDialogueSubsystem>();
	if (!DialogueSubsystem || DialogueSubsystem->IsDialogueActive()) return;

	bTriggeredThisSession = true;
	DialogueSubsystem->StartAmbientDialogue(DialogueData, DialogueID, PC, bMarkSeenOnComplete, LineHoldSeconds);
}
