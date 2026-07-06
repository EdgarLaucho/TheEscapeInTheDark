#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LeashComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeashDeactivated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeashActivated);

UCLASS(ClassGroup = AI, meta = (BlueprintSpawnableComponent),
       DisplayName = "Leash Component")
class INTHEDARK_API ULeashComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULeashComponent();

	void ActivateLeash(const FVector& Target);

	void DeactivateLeash();

	UPROPERTY(BlueprintAssignable, Category = "Leash")
	FOnLeashActivated OnLeashActivated;

	UPROPERTY(BlueprintAssignable, Category = "Leash")
	FOnLeashDeactivated OnLeashDeactivated;

	UFUNCTION(BlueprintPure, Category = "Leash", meta = (DefaultToSelf = "Actor"))
	static ULeashComponent* GetLeashComponent(AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "Leash")
	bool IsLeashActive() const { return bLeashActive; }

	UFUNCTION(BlueprintPure, Category = "Leash")
	FVector GetLeashTarget() const { return LeashTarget; }

private:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Leash",
	          meta = (AllowPrivateAccess = "true"))
	bool bLeashActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Leash",
	          meta = (AllowPrivateAccess = "true"))
	FVector LeashTarget = FVector::ZeroVector;
};
