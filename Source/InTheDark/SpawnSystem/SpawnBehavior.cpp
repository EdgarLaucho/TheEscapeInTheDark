#include "SpawnSystem/SpawnBehavior.h"
#include "SpawnSystem/SpawnPoint.h"

void USpawnBehavior::InitializeBehavior_Implementation(ASpawnPoint* InOwner)
{
	OwnerSpawnPoint = InOwner;
}

void USpawnBehavior::ActivateBehavior_Implementation()
{

}

void USpawnBehavior::DeactivateBehavior_Implementation()
{

}

void USpawnBehavior::OnSpawnedActorReleased_Implementation(AActor* Actor)
{

}

UWorld* USpawnBehavior::GetWorld() const
{
	if (OwnerSpawnPoint)
	{
		return OwnerSpawnPoint->GetWorld();
	}

	if (const AActor* OuterActor = GetTypedOuter<AActor>())
	{
		return OuterActor->GetWorld();
	}

	return nullptr;
}
