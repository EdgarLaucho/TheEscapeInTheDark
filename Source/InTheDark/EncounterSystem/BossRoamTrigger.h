#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossRoamTrigger.generated.h"

class UBoxComponent;
class UBillboardComponent;

/**
 * Volumen rectangular que activa o desactiva el sistema de apariciones del jefe
 * cuando el jugador lo atraviesa.
 *
 * Redimensiona el box desde el panel de detalles (Box Extent) o arrastrando
 * las aristas en el viewport con la herramienta de escala.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Boss Roam Trigger"))
class INTHEDARK_API ABossRoamTrigger : public AActor
{
	GENERATED_BODY()

public:
	ABossRoamTrigger();

	/** true = activa el sistema al pasar. false = lo desactiva. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossRoamTrigger")
	bool bEnableOnOverlap = false;

	/** Si es true, el trigger solo se dispara una vez durante la sesión. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossRoamTrigger")
	bool bTriggerOnce = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BossRoamTrigger|Components")
	TObjectPtr<UBoxComponent> TriggerBox;

protected:
	virtual void BeginPlay() override;

private:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Billboard;
#endif

	bool bAlreadyTriggered = false;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* Other,
	                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                    bool bFromSweep, const FHitResult& SweepResult);
};
