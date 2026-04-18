#include "SpawnSystem/Behaviors/SpawnBehavior_Triggered.h"
#include "SpawnSystem/SpawnPoint.h"

void USpawnBehavior_Triggered::ActivateBehavior_Implementation()
{
	bIsEnabled = true;
	bHasTriggered = false;
}

void USpawnBehavior_Triggered::DeactivateBehavior_Implementation()
{
	bIsEnabled = false;
}

void USpawnBehavior_Triggered::Trigger()
{
	if (!bIsEnabled || !OwnerSpawnPoint)
	{
		return;
	}

	if (bOneShot && bHasTriggered)
	{
		return;
	}

	bHasTriggered = true;

	for (int32 i = 0; i < SpawnCountPerTrigger; ++i)
	{
		OwnerSpawnPoint->RequestSpawn();
	}
}
