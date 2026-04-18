#include "SpawnSystem/Behaviors/SpawnBehavior_Timed.h"
#include "SpawnSystem/SpawnPoint.h"
#include "Engine/World.h"
#include "TimerManager.h"

void USpawnBehavior_Timed::ActivateBehavior_Implementation()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		SpawnTimerHandle,
		FTimerDelegate::CreateUObject(this, &USpawnBehavior_Timed::OnTimerTick),
		SpawnInterval, true);
}

void USpawnBehavior_Timed::DeactivateBehavior_Implementation()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(SpawnTimerHandle);
}

void USpawnBehavior_Timed::OnTimerTick()
{
	if (!OwnerSpawnPoint)
	{
		return;
	}

	if (OwnerSpawnPoint->GetActiveSpawnCount() < MaxActiveFromThisPoint)
	{
		OwnerSpawnPoint->RequestSpawn();
	}
}

void USpawnBehavior_Timed::OnSpawnedActorReleased_Implementation(AActor* Actor)
{

}
