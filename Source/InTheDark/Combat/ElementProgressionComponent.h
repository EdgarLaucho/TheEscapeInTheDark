#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ElementProgressionComponent.generated.h"

USTRUCT(BlueprintType)
struct FElementProgressionData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="ElementProgression")
	FName ElementName = NAME_None;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="ElementProgression")
	int32 Level= 1;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="ElementProgression")
	int32 KillCount = 0;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="ElementProgression")
	int32 MaxLevel = 3;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="ElementProgression")
	TArray<int32> KillsRequiredPerLevel ={5,15};

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="ElementProgression")
	float DamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="ElementProgression")
	float ScaleMultiplier = 1.f;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="ElementProgression")
	int32 MaxUnlockedComboStep =0;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="ElementProgression")
	bool bUnlocked = false;
	
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INTHEDARK_API UElementProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UElementProgressionComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="ElementProgression")
	TArray<FElementProgressionData> ElementProgressionData;

	UFUNCTION(BlueprintCallable,Category="ElementProgression")
	void AddKillToElement(FName ElementName, int32 KillAmount = 1);

	UFUNCTION(BlueprintCallable,Category="ElementProgression")
	void UnlockElement(FName ElementName);

	UFUNCTION(BlueprintCallable,Category="ElementProgression")
	bool GetElementProgressionData(FName ElementName, FElementProgressionData& OutData) const;

private:
	FElementProgressionData* FindElementProgressionData(FName ElementName);
	
};
