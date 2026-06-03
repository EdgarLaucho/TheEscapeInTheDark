#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ElementFusionComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INTHEDARK_API UElementFusionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UElementFusionComponent();

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Fusion")
	FName FusionElementName= "AirFire";

	UPROPERTY(BlueprintReadOnly,Category="Fusion")
	FName BaseElementBeforeFusion = NAME_None;

	UPROPERTY(BlueprintReadOnly,Category="Fusion")
	FName CurrentFusionElement = NAME_None;

	UPROPERTY(BlueprintReadOnly,Category="Fusion")
	bool bFusionActive = false;

	UPROPERTY(BlueprintReadOnly,Category="Fusion")
	bool bFusionOnCooldown = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion")
	float FusionDuration = 10.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion")
	float FusionCooldown = 20.f;
	
	UFUNCTION(BlueprintCallable, Category="Fusion")
	bool CanActivateFusion()const;

	UFUNCTION(BlueprintCallable, Category="Fusion")
	void ActivateFusion(FName CurrentElement, FName FusionResultElement);

	UFUNCTION(BlueprintCallable, Category="Fusion")
	FName GetActiveElementName() const;

	UFUNCTION(BlueprintCallable, Category="Fusion")
	void EndFusion();

	UFUNCTION(BlueprintCallable, Category="Fusion")
	void EndFusionCooldown();
	
protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
