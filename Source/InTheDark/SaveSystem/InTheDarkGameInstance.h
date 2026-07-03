#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SaveSystem/SaveTypes.h"
#include "InTheDarkGameInstance.generated.h"

class UInTheDarkSaveGame;
class UWorld;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSaveLoaded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSaveWritten);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSaveSnapshotRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEncounterCleared, FName, EncounterId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveWrittenAsync, bool, bSuccess);

UCLASS(BlueprintType, Blueprintable)
class INTHEDARK_API UInTheDarkGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UInTheDarkGameInstance();

	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintPure, Category = "Save", meta = (WorldContext = "WorldContextObject", DisplayName = "Get InTheDark GameInstance"))
	static UInTheDarkGameInstance* Get(const UObject* WorldContextObject);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|Config")
	int32 MaxSlots = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Save|Config")
	FName DefaultGameLevelName = TEXT("Lvl_EscapeRouteFromTheDarkness");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Save|Config")
	FName MainMenuLevelName = TEXT("Lvl_MainMenuEscape");

	UFUNCTION(BlueprintCallable, Category = "Save|Slots")
	void SwitchToSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Slots")
	FSaveSlotInfo GetSlotInfo(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Slots")
	TArray<FSaveSlotInfo> GetAllSlotInfos() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Slots")
	int32 GetCurrentSlotIndex() const { return CurrentSlotIndex; }

	UFUNCTION(BlueprintCallable, Category = "Save|Slots")
	void DeleteSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Save|Menu", meta = (WorldContext = "WorldContextObject"))
	void StartNewGameFromMenu(UObject* WorldContextObject, int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Save|Menu", meta = (WorldContext = "WorldContextObject"))
	void ContinueGameFromMenu(UObject* WorldContextObject, int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Save|Menu", meta = (WorldContext = "WorldContextObject"))
	void SaveCurrentGameAndOpenMainMenu(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Save|Menu", meta = (WorldContext = "WorldContextObject"))
	void OpenMainMenuWithoutSaving(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Save|Player")
	void SavePlayerState(const FSavedPlayerState& State);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Player")
	const FSavedPlayerState& GetPlayerState() const { return PlayerStateCache; }

	UFUNCTION(BlueprintCallable, Category = "Save|Player")
	void SaveAtCheckpoint(FName CheckpointID, const FSavedPlayerState& State);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Player")
	FName GetLastCheckpointID() const { return PlayerStateCache.LastCheckpointID; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Player")
	bool HasSavedTransform() const { return PlayerStateCache.bHasSavedTransform || PlayerStateCache.LastCheckpointID != NAME_None; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Player", meta = (WorldContext = "WorldContextObject"))
	FTransform GetSpawnTransform(UObject* WorldContextObject) const;

	UFUNCTION(BlueprintCallable, Category = "Save|World")
	void MarkWorldActor(FName Category, const FString& ActorID);

	UFUNCTION(BlueprintCallable, Category = "Save|World")
	void UnmarkWorldActor(FName Category, const FString& ActorID);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|World")
	bool IsWorldActorMarked(FName Category, const FString& ActorID) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|World")
	TArray<FString> GetMarkedActors(FName Category) const;

	UFUNCTION(BlueprintCallable, Category = "Save|World")
	void ClearWorldCategory(FName Category);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|World")
	static bool IsValidActorID(const FString& ActorID);

	UFUNCTION(BlueprintCallable, Category = "Save|World|Chest")
	void RegisterOpenedChest(const FString& ChestID) { MarkWorldActor(FName("Chest"), ChestID); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|World|Chest")
	bool IsChestOpened(const FString& ChestID) const { return IsWorldActorMarked(FName("Chest"), ChestID); }

	UFUNCTION(BlueprintCallable, Category = "Save|World|Pickup")
	void RegisterCollectedPickup(const FString& PickupID) { MarkWorldActor(FName("Pickup"), PickupID); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|World|Pickup")
	bool IsPickupCollected(const FString& PickupID) const { return IsWorldActorMarked(FName("Pickup"), PickupID); }

	UFUNCTION(BlueprintCallable, Category = "Save|World|Door")
	void RegisterOpenedDoor(const FString& DoorID) { MarkWorldActor(FName("Door"), DoorID); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|World|Door")
	bool IsDoorOpened(const FString& DoorID) const { return IsWorldActorMarked(FName("Door"), DoorID); }

	UFUNCTION(BlueprintCallable, Category = "Save|ElementProgression")
	void UpdateElementProgression(const FSavedElementProgressionEntry& Entry);

	UFUNCTION(BlueprintCallable, Category = "Save|ElementProgression")
	void SetElementProgressionCache(const TArray<FSavedElementProgressionEntry>& Data);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|ElementProgression")
	const TArray<FSavedElementProgressionEntry>& GetElementProgressionCache() const { return ElementProgressionCache; }

	UFUNCTION(BlueprintCallable, Category = "Save|ElementProgression")
	void ClearElementProgression();

	UFUNCTION(BlueprintCallable, Category = "Save|Companion")
	void UpdateCompanionPersonality(float Courage, float Anxiety, float Confidence, float AggressionAffinity, float StealthAffinity);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Companion")
	FSavedCompanionPersonality GetCompanionPersonality() const { return CompanionPersonalityCache; }

	UFUNCTION(BlueprintCallable, Category = "Save|Companion")
	void UpdateCompanionState(const FTransform& Transform, uint8 CurrentStateValue, bool bHasAwoken);

	UFUNCTION(BlueprintCallable, Category = "Save|Companion")
	void MarkCompanionAwoken(const FTransform& Transform, uint8 CurrentStateValue);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Companion")
	FSavedCompanionState GetCompanionState() const { return CompanionStateCache; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Companion")
	bool HasSavedCompanionState() const { return CompanionStateCache.bHasSavedState; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Companion")
	bool HasCompanionAwoken() const { return CompanionStateCache.bHasAwoken; }

	UFUNCTION(BlueprintCallable, Category = "Save|Tutorial")
	void UpdateTutorialState(int32 SavedStep, bool bFinished);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Tutorial")
	FSavedTutorialState GetTutorialState() const { return TutorialStateCache; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Tutorial")
	int32 GetSavedTutorialStep() const { return TutorialStateCache.SavedStep; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Tutorial")
	bool IsTutorialFinished() const { return TutorialStateCache.bFinished; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Dialogue")
	bool IsDialogueSeen(FName DialogueID) const;

	UFUNCTION(BlueprintCallable, Category = "Save|Dialogue")
	void MarkDialogueSeen(FName DialogueID);

	UFUNCTION(BlueprintCallable, Category = "Save|Encounters")
	void MarkEncounterCleared(FName EncounterId);

	UFUNCTION(BlueprintCallable, Category = "Save|Encounters")
	void UnmarkEncounterCleared(FName EncounterId);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Encounters")
	bool IsEncounterCleared(FName EncounterId) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Encounters")
	TArray<FName> GetClearedEncounters() const;

	UFUNCTION(BlueprintCallable, Category = "Save|Encounters")
	void ClearEncounters();

	UFUNCTION(BlueprintCallable, Category = "Save|Meta")
	void SetLastMapName(const FString& MapName);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Meta")
	FString GetLastMapName() const { return CachedLastMapName; }

	UFUNCTION(BlueprintCallable, Category = "Save")
	void ClearProgress();

	UFUNCTION(BlueprintCallable, Category = "Save|Disk")
	bool WriteSaveToDisk();

	UFUNCTION(BlueprintCallable, Category = "Save|Disk")
	void WriteSaveToDiskAsync();

	UFUNCTION(BlueprintCallable, Category = "Save|Disk")
	bool LoadOrCreateSave();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Disk")
	bool HasLoadedSave() const { return bHasLoadedSave; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Disk")
	bool IsSaveDirty() const { return bSaveDirty; }

	UFUNCTION(BlueprintCallable, Category = "Save|Disk")
	bool DeleteSave();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Disk")
	bool DoesSaveSlotExist() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Save|Config")
	bool bAutosaveOnMapChange = false;

	UPROPERTY(BlueprintAssignable, Category = "Save|Events")
	FOnSaveLoaded OnSaveLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Save|Events")
	FOnSaveWritten OnSaveWritten;

	UPROPERTY(BlueprintAssignable, Category = "Save|Events")
	FOnSaveSnapshotRequested OnSaveSnapshotRequested;

	UPROPERTY(BlueprintAssignable, Category = "Save|Events")
	FOnSaveWrittenAsync OnSaveWrittenAsync;

	UPROPERTY(BlueprintAssignable, Category = "Save|Events")
	FOnEncounterCleared OnEncounterCleared;

protected:
	void CopyCacheToPayload(UInTheDarkSaveGame& Payload) const;
	void CopyPayloadToCache(const UInTheDarkSaveGame& Payload);
	virtual bool MigrateSaveIfNeeded(UInTheDarkSaveGame& Payload);
	UInTheDarkSaveGame* BuildPayload() const;
	void HandleAsyncSaveCompleted(const FString& Slot, const int32 UserIndex, bool bSuccess);
	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);
	void ResetCache();
	FString GetSlotName(int32 SlotIndex) const;
	bool IsValidSlotIndex(int32 SlotIndex) const;
	bool IsMainMenuMap(const FString& MapName) const;
	void RequestSaveSnapshot();
	void CapturePlayerSnapshot();

private:
	TArray<FSavedElementProgressionEntry> ElementProgressionCache;
	FSavedCompanionPersonality CompanionPersonalityCache;
	FSavedCompanionState CompanionStateCache;
	FSavedTutorialState TutorialStateCache;
	TMap<FName, TSet<FString>> WorldStateCache;
	TSet<FName> ClearedEncountersCache;
	TSet<FName> SeenDialoguesCache;
	FSavedPlayerState PlayerStateCache;
	FString CachedLastMapName;
	FString SaveSlotName = TEXT("Save_0");
	int32 CurrentSlotIndex = 0;
	int32 SaveUserIndex = 0;
	bool bHasLoadedSave = false;
	bool bSaveDirty = false;
	bool bAsyncSaveInFlight = false;
	bool bRequestingSaveSnapshot = false;
	FDelegateHandle PostLoadMapHandle;
};
