#include "RoundSystem/RoundProgressSubsystem.h"

void URoundProgressSubsystem::MarkCompleted(FName ArenaId)
{
	if (ArenaId.IsNone()) return;
	bool bAlreadyIn = false;
	CompletedArenas.Add(ArenaId, &bAlreadyIn);
	if (!bAlreadyIn)
	{
		OnArenaCompleted.Broadcast(ArenaId);
	}
}

bool URoundProgressSubsystem::IsCompleted(FName ArenaId) const
{
	return !ArenaId.IsNone() && CompletedArenas.Contains(ArenaId);
}

void URoundProgressSubsystem::ResetArena(FName ArenaId)
{
	CompletedArenas.Remove(ArenaId);
}

void URoundProgressSubsystem::ResetAll()
{
	CompletedArenas.Reset();
}

TArray<FName> URoundProgressSubsystem::GetCompletedArenas() const
{
	return CompletedArenas.Array();
}

void URoundProgressSubsystem::LoadCompletedArenas(const TArray<FName>& Arenas)
{
	CompletedArenas.Reset();
	for (const FName& Id : Arenas)
	{
		if (!Id.IsNone())
		{
			CompletedArenas.Add(Id);
		}
	}
}
