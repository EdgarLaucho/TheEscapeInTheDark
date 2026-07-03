#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatArena.generated.h"

class UBoxComponent;
class UEncounterConfig;
class UEncounterDirectorComponent;
class ASpawnAnchor;
class AEncounterGate;

UCLASS(Blueprintable, BlueprintType)
class INTHEDARK_API ACombatArena : public AActor
{
	GENERATED_BODY()

public:
	ACombatArena();

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Encounter|Authoring")
	FName EncounterId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Authoring")
	TObjectPtr<UEncounterConfig> Config;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Encounter|Authoring")
	TArray<TObjectPtr<ASpawnAnchor>> Anchors;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Encounter|Authoring")
	TArray<TObjectPtr<AEncounterGate>> EntryGates;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Encounter|Authoring")
	bool bUnlockEntryGatesOnClear = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Encounter|Authoring")
	TArray<TObjectPtr<AEncounterGate>> ExitGates;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Authoring")
	bool bAutoStartOnOverlap = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Authoring")
	bool bSkipIfAlreadyCleared = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter|Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter|Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter|Components")
	TObjectPtr<UEncounterDirectorComponent> Director;

	UFUNCTION(BlueprintCallable, Category = "Encounter")
	void RequestStart();

	void NotifyEncounterCleared();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

private:
	bool bAlreadyStartedThisSession = false;

	bool LookupIsAlreadyCleared() const;
	void LockEntryGates();
	void UnlockEntryGates();
	void UnlockExitGates();
	void UnlockGatesForClearedState();
};