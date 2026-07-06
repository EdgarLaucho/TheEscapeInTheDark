#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPtr.h"
#include "CombatArena.generated.h"

class APawn;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Companion")
	TSoftClassPtr<AActor> CompanionClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Companion")
	FVector CompanionFallbackOffset = FVector(-150.f, 120.f, 20.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Companion")
	uint8 CompanionEncounterStateValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Companion")
	float CompanionCheckInterval = 2.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter|Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter|Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter|Components")
	TObjectPtr<UBoxComponent> ContainmentVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter|Components")
	TObjectPtr<UEncounterDirectorComponent> Director;

	UFUNCTION(BlueprintCallable, Category = "Encounter")
	void RequestStart();

	UFUNCTION(BlueprintCallable, Category = "Encounter|Companion")
	void EnsureCompanionInsideEncounter(AActor* PlayerOverride = nullptr);

	void NotifyEncounterCleared();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

private:
	bool bAlreadyStartedThisSession = false;
	FTimerHandle CompanionCheckTimerHandle;

	bool LookupIsAlreadyCleared() const;
	void LockEntryGates();
	void UnlockEntryGates();
	void UnlockExitGates();
	void UnlockGatesForClearedState();
	void CheckCompanionDistance();
	AActor* FindCompanionActor() const;
	bool IsInsideContainmentVolume(const FVector& Location) const;
	FVector ClampLocationToContainmentVolume(const FVector& Location) const;
	FTransform BuildCompanionEncounterTransform(const AActor* PlayerActor) const;
	void ReactivateCompanionAfterTeleport(AActor* CompanionActor) const;
};