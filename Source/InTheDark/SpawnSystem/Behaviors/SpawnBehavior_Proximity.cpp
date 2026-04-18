#include "SpawnSystem/Behaviors/SpawnBehavior_Proximity.h"
#include "SpawnSystem/SpawnPoint.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"

void USpawnBehavior_Proximity::ActivateBehavior_Implementation()
{
	bHasTriggered = false;
	bWaitingRespawn = false;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		CheckTimerHandle,
		FTimerDelegate::CreateUObject(this, &USpawnBehavior_Proximity::CheckProximity),
		CheckInterval, true);
}

void USpawnBehavior_Proximity::DeactivateBehavior_Implementation()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(CheckTimerHandle);
	World->GetTimerManager().ClearTimer(RespawnTimerHandle);
}

void USpawnBehavior_Proximity::CheckProximity()
{
	if (!OwnerSpawnPoint || (bOneShot && bHasTriggered) || bWaitingRespawn)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->GetPawn())
	{
		return;
	}

	const float Distance = FVector::Dist(
		PC->GetPawn()->GetActorLocation(),
		OwnerSpawnPoint->GetActorLocation());

	if (Distance <= TriggerRadius)
	{
		bHasTriggered = true;

		for (int32 i = 0; i < SpawnCount; ++i)
		{
			OwnerSpawnPoint->RequestSpawn();
		}
	}
}

void USpawnBehavior_Proximity::OnSpawnedActorReleased_Implementation(AActor* Actor)
{
	if (!bOneShot && bHasTriggered && !bWaitingRespawn)
	{
		bWaitingRespawn = true;

		UWorld* World = GetWorld();
		if (World)
		{
			World->GetTimerManager().SetTimer(
				RespawnTimerHandle,
				FTimerDelegate::CreateUObject(this, &USpawnBehavior_Proximity::OnRespawnReady),
				RespawnDelay, false);
		}
	}
}

void USpawnBehavior_Proximity::OnRespawnReady()
{
	bHasTriggered = false;
	bWaitingRespawn = false;

}
