#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayerMusicStateInterface.generated.h"

class USoundBase;

UINTERFACE(Blueprintable, meta = (DisplayName = "Player Music State"))
class INTHEDARK_API UPlayerMusicStateInterface : public UInterface
{
	GENERATED_BODY()
};

class INTHEDARK_API IPlayerMusicStateInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Music")
	USoundBase* GetCurrentMusic();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Music")
	void SetCurrentMusic(USoundBase* Music);
};