#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CheckpointActor.generated.h"

class UBoxComponent;

UCLASS(Blueprintable, BlueprintType)
class INTHEDARK_API ACheckpointActor : public AActor
{
	GENERATED_BODY()

public:
	ACheckpointActor();

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Checkpoint")
	FName CheckpointID;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkpoint")
	TObjectPtr<UBoxComponent> TriggerBox;

	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintImplementableEvent, Category = "Checkpoint")
	void OnCheckpointReached(AActor* PlayerPawn);
};