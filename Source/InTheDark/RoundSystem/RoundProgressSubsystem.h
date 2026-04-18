#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RoundProgressSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArenaCompletedSignature, FName, ArenaId);

UCLASS()
class INTHEDARK_API URoundProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Round Progress")
	void MarkCompleted(FName ArenaId);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Round Progress")
	bool IsCompleted(FName ArenaId) const;

	UFUNCTION(BlueprintCallable, Category = "Round Progress")
	void ResetArena(FName ArenaId);

	UFUNCTION(BlueprintCallable, Category = "Round Progress")
	void ResetAll();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Round Progress")
	TArray<FName> GetCompletedArenas() const;

	UFUNCTION(BlueprintCallable, Category = "Round Progress")
	void LoadCompletedArenas(const TArray<FName>& Arenas);

	UPROPERTY(BlueprintAssignable, Category = "Round Progress|Events")
	FOnArenaCompletedSignature OnArenaCompleted;

private:
	UPROPERTY()
	TSet<FName> CompletedArenas;
};
