#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "GameFramework/Actor.h"
#include "PlayerTutorialManager.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class ETutorialStepType : uint8
{
	ShowOnly UMETA(DisplayName = "Show Only"),
	PressAction UMETA(DisplayName = "Press Action"),
	HoldAndPressAction UMETA(DisplayName = "Hold And Press Action"),
	WaitForBlueprintEvent UMETA(DisplayName = "Wait For Blueprint Event")
};

USTRUCT(BlueprintType)
struct FTutorialStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	ETutorialStepType StepType = ETutorialStepType::PressAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	FText StepText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	FText KeyboardStepText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	FText GamepadStepText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	UTexture2D* StepImage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Input")
	UInputAction* RequiredAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Input")
	UInputAction* RequiredHeldAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	bool bAutoAdvanceAfterShow = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	float AutoAdvanceDelay = 2.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTutorialStepChanged, int32, NewStep, const FTutorialStep&, StepData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTutorialFinished);

UCLASS()
class INTHEDARK_API APlayerTutorialManager : public AActor
{
	GENERATED_BODY()

public:
	APlayerTutorialManager();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	int32 CurrentStep = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	bool bControlPressed = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Save")
	bool bTutorialFinished = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Save")
	int32 SavedTutorialStep = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	bool bUsingGamepad = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	TArray<FTutorialStep> TutorialSteps;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial")
	FTutorialStepChanged OnTutorialStepChanged;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial")
	FTutorialFinished OnTutorialFinished;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Input")
	UInputAction* MoveAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Input")
	UInputAction* JumpAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Input")
	UInputAction* LookAround = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Input")
	UInputAction* Slide = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Input")
	UInputAction* Attack1 = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Input")
	UInputAction* Attack2 = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Input")
	UInputAction* Lockon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Input")
	UInputAction* ChangeElement = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Input")
	UInputAction* FusionModeAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Input")
	UInputAction* FusionNextAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Input")
	UInputAction* SwitchTarget = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void NextStep();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ShowStep();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void CompleteCurrentStep();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void CompleteCurrentStepFromEvent();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	bool IsControlPressed() const;

	UFUNCTION(BlueprintCallable, Category = "Tutorial|Input")
	void NotifyActionStarted(UInputAction* Action);

	UFUNCTION(BlueprintCallable, Category = "Tutorial|Input")
	void NotifyActionCompleted(UInputAction* Action);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	FTutorialStep GetCurrentTutorialStep() const;

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SetUsingGamepad(bool bNewUsingGamepad);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	FText GetCurrentStepDisplayText() const;

	UFUNCTION(BlueprintCallable, Category = "Tutorial|Save")
	void ApplyLoadedTutorialState(int32 LoadedStep, bool bLoadedTutorialFinished);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void InitializeTutorialInputDevice(bool bInitialUsingGamepad);
	
	
private:
	UPROPERTY()
	TSet<TObjectPtr<UInputAction>> HeldActions;

	float AutoAdvanceTimer = 0.0f;

	bool IsValidCurrentStep() const;
	void FinishTutorial();
	void UpdateTutorialTickEnabled();
};
