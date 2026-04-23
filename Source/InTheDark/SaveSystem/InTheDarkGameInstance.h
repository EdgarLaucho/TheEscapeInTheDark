#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SaveSystem/SaveTypes.h"
#include "InTheDarkGameInstance.generated.h"

class UInTheDarkSaveGame;
class UWorld;
class USaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSaveLoaded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSaveWritten);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEncounterCleared, FName, EncounterId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveWrittenAsync, bool, bSuccess);

/**
 * Gestor del sistema de guardado. Posee la caché en memoria, gestiona los slots y el IO de disco.
 * Todo el estado persistente del juego pasa por aquí.
 */
UCLASS(BlueprintType, Blueprintable)
class INTHEDARK_API UInTheDarkGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UInTheDarkGameInstance();

	virtual void Init() override;
	virtual void Shutdown() override;

	/** Acceso directo — evita el Cast manual en BP y C++. */
	UFUNCTION(BlueprintPure, Category = "Save", meta = (WorldContext = "WorldContextObject", DisplayName = "Get InTheDark GameInstance"))
	static UInTheDarkGameInstance* Get(const UObject* WorldContextObject);

	// ──── Slots ─────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|Config")
	int32 MaxSlots = 3;

	/** Vuelca el estado sucio del slot actual, cambia y carga (o crea) el slot destino. */
	UFUNCTION(BlueprintCallable, Category = "Save|Slots")
	void SwitchToSlot(int32 SlotIndex);

	/** Lee los metadatos de un slot sin afectar la caché activa. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Slots")
	FSaveSlotInfo GetSlotInfo(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Slots")
	TArray<FSaveSlotInfo> GetAllSlotInfos() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Slots")
	int32 GetCurrentSlotIndex() const { return CurrentSlotIndex; }

	/** Elimina el archivo de guardado de un slot. Si es el slot activo, también resetea la caché. */
	UFUNCTION(BlueprintCallable, Category = "Save|Slots")
	void DeleteSlot(int32 SlotIndex);

	// ──── Estado del jugador ──────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Save|Player")
	void SavePlayerState(const FSavedPlayerState& State);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Player")
	const FSavedPlayerState& GetPlayerState() const { return PlayerStateCache; }

	/** Guarda el estado del jugador con el ID de checkpoint dado y escribe en disco de forma asíncrona. */
	UFUNCTION(BlueprintCallable, Category = "Save|Player")
	void SaveAtCheckpoint(FName CheckpointID, const FSavedPlayerState& State);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Player")
	FName GetLastCheckpointID() const { return PlayerStateCache.LastCheckpointID; }

	// ──── Inventario ──────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Inventory")
	const TArray<FSavedInventoryEntry>& GetCachedInventory() const { return InventoryCache; }

	UFUNCTION(BlueprintCallable, Category = "Save|Inventory")
	void SetCachedInventory(const TArray<FSavedInventoryEntry>& Inventory);

	UFUNCTION(BlueprintCallable, Category = "Save|Inventory")
	void AddInventoryEntry(FName ItemRowName, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Save|Inventory")
	void RemoveInventoryEntry(FName ItemRowName, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Save|Inventory")
	void SetInventoryEntryQuantity(FName ItemRowName, int32 Quantity);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Inventory")
	int32 GetInventoryEntryQuantity(FName ItemRowName) const;

	UFUNCTION(BlueprintCallable, Category = "Save|Inventory")
	void ClearInventory();

	// ──── World State (generic) ──────────────────────────────────────────────

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

	// Atajos — una línea, sin nuevas estructuras de datos.

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

	// ──── Encounters ──────────────────────────────────────────────

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

	// ──── Meta ──────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Save|Meta")
	void SetLastMapName(const FString& MapName);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save|Meta")
	FString GetLastMapName() const { return CachedLastMapName; }

	// ──── Progress ──────────────────────────────────────────────

	/** Borra todo el estado en memoria. NO escribe en disco — llama a WriteSaveToDisk después si es necesario. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void ClearProgress();

	// ──── Disk IO ──────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Save|Disk")
	bool WriteSaveToDisk();

	UFUNCTION(BlueprintCallable, Category = "Save|Disk")
	void WriteSaveToDiskAsync();

	UFUNCTION(BlueprintCallable, Category = "Save|Disk")
	bool LoadOrCreateSave();

	UFUNCTION(BlueprintCallable, Category = "Save|Disk")
	void LoadOrCreateSaveAsync();

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

	// ──── Events ──────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Save|Events")
	FOnSaveLoaded OnSaveLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Save|Events")
	FOnSaveWritten OnSaveWritten;

	UPROPERTY(BlueprintAssignable, Category = "Save|Events")
	FOnSaveWrittenAsync OnSaveWrittenAsync;

	UPROPERTY(BlueprintAssignable, Category = "Save|Events")
	FOnEncounterCleared OnEncounterCleared;

	/** Sobreescribe el nombre de slot. Solo para tests automatizados. */
	void ForceSlotName(const FString& Slot, int32 UserIdx = 0);

protected:
	void CopyCacheToPayload(UInTheDarkSaveGame& Payload) const;
	void CopyPayloadToCache(const UInTheDarkSaveGame& Payload);
	virtual bool MigrateSaveIfNeeded(UInTheDarkSaveGame& Payload);
	UInTheDarkSaveGame* BuildPayload() const;
	void HandleAsyncSaveCompleted(const FString& Slot, const int32 UserIndex, bool bSuccess);
	void HandleAsyncLoadCompleted(const FString& Slot, const int32 UserIndex, USaveGame* Loaded);
	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);
	void ResetCache();
	FString GetSlotName(int32 SlotIndex) const;

private:
	TArray<FSavedInventoryEntry> InventoryCache;
	TMap<FName, TSet<FString>> WorldStateCache;
	TSet<FName> ClearedEncountersCache;
	FSavedPlayerState PlayerStateCache;
	FString CachedLastMapName;
	FString SaveSlotName = TEXT("Save_0");
	int32 CurrentSlotIndex = 0;
	int32 SaveUserIndex = 0;
	bool bHasLoadedSave = false;
	bool bSaveDirty = false;
	bool bAsyncSaveInFlight = false;
	bool bAsyncLoadInFlight = false;
	FDelegateHandle PostLoadMapHandle;
};
