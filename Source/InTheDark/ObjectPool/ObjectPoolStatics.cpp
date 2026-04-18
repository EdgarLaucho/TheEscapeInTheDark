#include "ObjectPool/ObjectPoolStatics.h"
#include "ObjectPool/ObjectPoolSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

static UObjectPoolSubsystem* GetSubsystemFromContext(UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
	{
		return nullptr;
	}

	return GI->GetSubsystem<UObjectPoolSubsystem>();
}

AActor* UObjectPoolStatics::AcquireFromPool(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform)
{
	if (UObjectPoolSubsystem* Subsystem = GetSubsystemFromContext(WorldContextObject))
	{
		return Subsystem->AcquireFromPool(WorldContextObject, ActorClass, SpawnTransform);
	}
	return nullptr;
}

void UObjectPoolStatics::ReleaseToPool(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	if (UObjectPoolSubsystem* Subsystem = GetSubsystemFromContext(Actor))
	{
		Subsystem->ReleaseToPool(Actor);
	}
}

void UObjectPoolStatics::PrewarmPool(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, int32 Count)
{
	if (UObjectPoolSubsystem* Subsystem = GetSubsystemFromContext(WorldContextObject))
	{
		Subsystem->PrewarmPool(WorldContextObject, ActorClass, Count);
	}
}

void UObjectPoolStatics::RegisterPool(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FPoolSettings& Settings)
{
	if (UObjectPoolSubsystem* Subsystem = GetSubsystemFromContext(WorldContextObject))
	{
		Subsystem->RegisterPool(ActorClass, Settings);
	}
}

void UObjectPoolStatics::ReleaseAllActive(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass)
{
	if (UObjectPoolSubsystem* Subsystem = GetSubsystemFromContext(WorldContextObject))
	{
		Subsystem->ReleaseAllActive(ActorClass);
	}
}

FPoolStats UObjectPoolStatics::GetPoolStats(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass)
{
	if (UObjectPoolSubsystem* Subsystem = GetSubsystemFromContext(WorldContextObject))
	{
		return Subsystem->GetPoolStats(ActorClass);
	}
	return FPoolStats();
}

bool UObjectPoolStatics::HasPool(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass)
{
	if (UObjectPoolSubsystem* Subsystem = GetSubsystemFromContext(WorldContextObject))
	{
		return Subsystem->HasPool(ActorClass);
	}
	return false;
}

UObjectPoolSubsystem* UObjectPoolStatics::GetObjectPoolSubsystem(UObject* WorldContextObject)
{
	return GetSubsystemFromContext(WorldContextObject);
}
