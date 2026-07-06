#include "EncounterSystem/CombatArena.h"
#include "EncounterSystem/EncounterConfig.h"
#include "EncounterSystem/EncounterDirectorComponent.h"
#include "EncounterSystem/EncounterGate.h"
#include "EncounterSystem/SpawnAnchor.h"
#include "SaveSystem/InTheDarkGameInstance.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

ACombatArena::ACombatArena()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(Root);
	TriggerVolume->SetBoxExtent(FVector(1000.f, 1000.f, 300.f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);

	ContainmentVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ContainmentVolume"));
	ContainmentVolume->SetupAttachment(Root);
	ContainmentVolume->SetBoxExtent(FVector(1500.f, 1500.f, 500.f));
	ContainmentVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ContainmentVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	ContainmentVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ContainmentVolume->SetGenerateOverlapEvents(true);

	Director = CreateDefaultSubobject<UEncounterDirectorComponent>(TEXT("Director"));
	CompanionClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/AI/Partner/Blueprints/BP_PartnerAICharacter.BP_PartnerAICharacter_C")));
}

void ACombatArena::BeginPlay()
{
	Super::BeginPlay();

	if (bSkipIfAlreadyCleared && LookupIsAlreadyCleared())
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ACombatArena::UnlockGatesForClearedState);
		return;
	}

	if (TriggerVolume && bAutoStartOnOverlap)
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ACombatArena::HandleTriggerOverlap);
	}
}

bool ACombatArena::LookupIsAlreadyCleared() const
{
	if (EncounterId.IsNone()) return false;

	const UWorld* World = GetWorld();
	if (!World) return false;

	const UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(UGameplayStatics::GetGameInstance(World));
	return GI && GI->IsEncounterCleared(EncounterId);
}

void ACombatArena::HandleTriggerOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (bAlreadyStartedThisSession) return;

	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const AActor* PlayerPawn = PC ? PC->GetPawn() : nullptr;

	if (Other == PlayerPawn) RequestStart();
}

void ACombatArena::RequestStart()
{
	if (bAlreadyStartedThisSession || LookupIsAlreadyCleared()) return;

	if (!Director || !Config) return;

	bAlreadyStartedThisSession = true;
	LockEntryGates();
	Director->StartEncounter();
	EnsureCompanionInsideEncounter();
}

void ACombatArena::EnsureCompanionInsideEncounter(AActor* PlayerOverride)
{
	if (!Director || !Director->IsEncounterActive()) return;

	AActor* CompanionActor = FindCompanionActor();
	if (!IsValid(CompanionActor)) return;

	if (IsInsideContainmentVolume(CompanionActor->GetActorLocation())) return;

	AActor* PlayerActor = PlayerOverride;

	if (!PlayerActor)
	{
		const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		PlayerActor = PC ? PC->GetPawn() : nullptr;
	}

	if (!IsValid(PlayerActor)) return;

	const FTransform TargetTransform = BuildCompanionEncounterTransform(PlayerActor);

	CompanionActor->SetActorTransform(TargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
	ReactivateCompanionAfterTeleport(CompanionActor);
}

void ACombatArena::NotifyEncounterCleared()
{
	if (bUnlockEntryGatesOnClear) UnlockEntryGates();

	UnlockExitGates();

	if (Config && Config->bPersistCleared && !EncounterId.IsNone())
	{
		if (UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(UGameplayStatics::GetGameInstance(GetWorld())))
		{
			GI->MarkEncounterCleared(EncounterId);
			GI->WriteSaveToDisk();
		}
	}
}

void ACombatArena::LockEntryGates()
{
	for (const TObjectPtr<AEncounterGate>& Gate : EntryGates)
	{
		if (Gate) Gate->Lock();
	}
}

void ACombatArena::UnlockEntryGates()
{
	for (const TObjectPtr<AEncounterGate>& Gate : EntryGates)
	{
		if (Gate) Gate->Unlock();
	}
}

void ACombatArena::UnlockExitGates()
{
	for (const TObjectPtr<AEncounterGate>& Gate : ExitGates)
	{
		if (Gate) Gate->Unlock();
	}
}

void ACombatArena::UnlockGatesForClearedState()
{
	for (const TObjectPtr<AEncounterGate>& Gate : EntryGates)
	{
		if (Gate) Gate->SetLockedInstant(!bUnlockEntryGatesOnClear);
	}

	for (const TObjectPtr<AEncounterGate>& Gate : ExitGates)
	{
		if (Gate) Gate->SetLockedInstant(false);
	}
}

AActor* ACombatArena::FindCompanionActor() const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	UClass* LoadedCompanionClass = CompanionClass.LoadSynchronous();

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor)) continue;

		if (LoadedCompanionClass && Actor->IsA(LoadedCompanionClass))
			return Actor;

		if (!LoadedCompanionClass && Actor->GetClass()->GetName().Contains(TEXT("PartnerAICharacter")))
			return Actor;
	}

	return nullptr;
}

bool ACombatArena::IsInsideContainmentVolume(const FVector& Location) const
{
	if (!ContainmentVolume) return true;

	const FVector LocalLocation = ContainmentVolume->GetComponentTransform().InverseTransformPosition(Location);
	const FVector Extent = ContainmentVolume->GetUnscaledBoxExtent();

	return FMath::Abs(LocalLocation.X) <= Extent.X
		&& FMath::Abs(LocalLocation.Y) <= Extent.Y
		&& FMath::Abs(LocalLocation.Z) <= Extent.Z;
}

FVector ACombatArena::ClampLocationToContainmentVolume(const FVector& Location) const
{
	if (!ContainmentVolume) return Location;

	const FTransform VolumeTransform = ContainmentVolume->GetComponentTransform();
	const FVector Extent = ContainmentVolume->GetUnscaledBoxExtent();
	FVector LocalLocation = VolumeTransform.InverseTransformPosition(Location);

	const FVector Margin(75.f, 75.f, 25.f);
	const FVector SafeExtent(
		FMath::Max(0.f, Extent.X - Margin.X),
		FMath::Max(0.f, Extent.Y - Margin.Y),
		FMath::Max(0.f, Extent.Z - Margin.Z)
	);

	LocalLocation.X = FMath::Clamp(LocalLocation.X, -SafeExtent.X, SafeExtent.X);
	LocalLocation.Y = FMath::Clamp(LocalLocation.Y, -SafeExtent.Y, SafeExtent.Y);
	LocalLocation.Z = FMath::Clamp(LocalLocation.Z, -SafeExtent.Z, SafeExtent.Z);

	return VolumeTransform.TransformPosition(LocalLocation);
}

FTransform ACombatArena::BuildCompanionEncounterTransform(const AActor* PlayerActor) const
{
	if (!PlayerActor) return FTransform::Identity;

	TArray<FVector> CandidateLocations;
	CandidateLocations.Add(PlayerActor->GetActorLocation()
		+ PlayerActor->GetActorForwardVector() * CompanionFallbackOffset.X
		+ PlayerActor->GetActorRightVector() * CompanionFallbackOffset.Y
		+ FVector(0.f, 0.f, CompanionFallbackOffset.Z));
	CandidateLocations.Add(PlayerActor->GetActorLocation() - PlayerActor->GetActorForwardVector() * 150.f + FVector(0.f, 0.f, CompanionFallbackOffset.Z));
	CandidateLocations.Add(PlayerActor->GetActorLocation() + PlayerActor->GetActorRightVector() * 150.f + FVector(0.f, 0.f, CompanionFallbackOffset.Z));
	CandidateLocations.Add(PlayerActor->GetActorLocation() - PlayerActor->GetActorRightVector() * 150.f + FVector(0.f, 0.f, CompanionFallbackOffset.Z));

	if (UWorld* World = GetWorld())
	{
		for (FVector& DesiredLocation : CandidateLocations)
		{
			if (UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(World))
			{
				FNavLocation ProjectedLocation;
				if (NavSystem->ProjectPointToNavigation(DesiredLocation, ProjectedLocation, FVector(300.f, 300.f, 500.f)))
					DesiredLocation = ProjectedLocation.Location;
			}

			FHitResult Hit;
			const FVector TraceStart = DesiredLocation + FVector(0.f, 0.f, 300.f);
			const FVector TraceEnd = DesiredLocation - FVector(0.f, 0.f, 1200.f);
			FCollisionQueryParams Params(SCENE_QUERY_STAT(EncounterCompanionGroundTrace), false, PlayerActor);

			if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
				DesiredLocation = Hit.Location + FVector(0.f, 0.f, 5.f);

			if (IsInsideContainmentVolume(DesiredLocation))
				return FTransform(PlayerActor->GetActorRotation(), DesiredLocation, FVector::OneVector);
		}

		FVector FallbackLocation = ClampLocationToContainmentVolume(CandidateLocations[0]);
		FHitResult Hit;
		const FVector TraceStart = FallbackLocation + FVector(0.f, 0.f, 300.f);
		const FVector TraceEnd = FallbackLocation - FVector(0.f, 0.f, 1200.f);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(EncounterCompanionFallbackGroundTrace), false, PlayerActor);

		if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
			FallbackLocation = Hit.Location + FVector(0.f, 0.f, 5.f);

		FallbackLocation = ClampLocationToContainmentVolume(FallbackLocation);
		return FTransform(PlayerActor->GetActorRotation(), FallbackLocation, FVector::OneVector);
	}

	return FTransform(PlayerActor->GetActorRotation(), ClampLocationToContainmentVolume(CandidateLocations[0]), FVector::OneVector);
}

void ACombatArena::ReactivateCompanionAfterTeleport(AActor* CompanionActor) const
{
	if (!IsValid(CompanionActor)) return;

	CompanionActor->SetActorHiddenInGame(false);
	CompanionActor->SetActorEnableCollision(true);
	CompanionActor->SetActorTickEnabled(true);

	if (APawn* CompanionPawn = Cast<APawn>(CompanionActor))
	{
		if (!CompanionPawn->GetController())
			CompanionPawn->SpawnDefaultController();
	}

	if (ACharacter* CompanionCharacter = Cast<ACharacter>(CompanionActor))
	{
		if (UCharacterMovementComponent* Movement = CompanionCharacter->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_Walking);
		}
	}

	if (UFunction* SetStateFunction = CompanionActor->FindFunction(TEXT("SetPartnerState")))
	{
		struct FSetPartnerStateParams
		{
			uint8 NewState = 0;
		};

		FSetPartnerStateParams Params;
		Params.NewState = CompanionEncounterStateValue;
		CompanionActor->ProcessEvent(SetStateFunction, &Params);
	}
}
