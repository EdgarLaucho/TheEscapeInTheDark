#include "SaveSystem/InTheDarkGameInstance.h"
#include "SaveSystem/InTheDarkSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Misc/CoreDelegates.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"

UInTheDarkGameInstance::UInTheDarkGameInstance() = default;

UInTheDarkGameInstance* UInTheDarkGameInstance::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;
	return Cast<UInTheDarkGameInstance>(UGameplayStatics::GetGameInstance(WorldContextObject));
}

void UInTheDarkGameInstance::Init()
{
	Super::Init();
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UInTheDarkGameInstance::OnPostLoadMapWithWorld);
	SaveSlotName = GetSlotName(CurrentSlotIndex);
}

void UInTheDarkGameInstance::Shutdown()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}
	if (bSaveDirty)
	{
		WriteSaveToDisk();
	}
	Super::Shutdown();
}

// ── Auxiliares ──────────────────────────────────────────────

FString UInTheDarkGameInstance::GetSlotName(int32 SlotIndex) const
{
	return FString::Printf(TEXT("Save_%d"), SlotIndex);
}

void UInTheDarkGameInstance::ResetCache()
{
	InventoryCache.Reset();
	ElementProgressionCache.Reset();
	CompanionPersonalityCache = FSavedCompanionPersonality();
	WorldStateCache.Reset();
	ClearedEncountersCache.Reset();
	PlayerStateCache = FSavedPlayerState();
	CachedLastMapName.Reset();
}

bool UInTheDarkGameInstance::IsValidActorID(const FString& ActorID)
{
	return !ActorID.IsEmpty() && !ActorID.Equals(TEXT("None"), ESearchCase::IgnoreCase);
}

void UInTheDarkGameInstance::ForceSlotName(const FString& Slot, int32 UserIdx)
{
	SaveSlotName = Slot;
	SaveUserIndex = UserIdx;
}

// ── Slots ─────────────────────────────────────────────────

void UInTheDarkGameInstance::SwitchToSlot(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= MaxSlots)
	{
		UE_LOG(LogTemp, Warning, TEXT("SwitchToSlot: index %d out of range [0..%d)"), SlotIndex, MaxSlots);
		return;
	}
	if (bSaveDirty)
	{
		WriteSaveToDisk();
	}
	CurrentSlotIndex = SlotIndex;
	SaveSlotName = GetSlotName(SlotIndex);
	LoadOrCreateSave();
}

FSaveSlotInfo UInTheDarkGameInstance::GetSlotInfo(int32 SlotIndex) const
{
	FSaveSlotInfo Info;
	Info.SlotIndex = SlotIndex;

	if (SlotIndex < 0 || SlotIndex >= MaxSlots)
	{
		Info.bIsEmpty = true;
		return Info;
	}

	const FString SlotName = GetSlotName(SlotIndex);
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex))
	{
		Info.bIsEmpty = true;
		return Info;
	}

	USaveGame* Raw = UGameplayStatics::LoadGameFromSlot(SlotName, SaveUserIndex);
	UInTheDarkSaveGame* Save = Cast<UInTheDarkSaveGame>(Raw);
	if (!Save)
	{
		Info.bIsEmpty = true;
		return Info;
	}

	Info.bIsEmpty = false;
	Info.DisplayMapName = Save->LastMapName;
	Info.SavedAt = Save->SavedAtUtc;
	return Info;
}

TArray<FSaveSlotInfo> UInTheDarkGameInstance::GetAllSlotInfos() const
{
	TArray<FSaveSlotInfo> Result;
	Result.Reserve(MaxSlots);
	for (int32 i = 0; i < MaxSlots; ++i)
	{
		Result.Add(GetSlotInfo(i));
	}
	return Result;
}

void UInTheDarkGameInstance::DeleteSlot(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= MaxSlots) return;

	const FString SlotName = GetSlotName(SlotIndex);
	UGameplayStatics::DeleteGameInSlot(SlotName, SaveUserIndex);

	if (SlotIndex == CurrentSlotIndex)
	{
		ResetCache();
		bHasLoadedSave = false;
		bSaveDirty = false;
	}
}

// ── Player State ──────────────────────────────────────────

void UInTheDarkGameInstance::SavePlayerState(const FSavedPlayerState& State)
{
	PlayerStateCache = State;
	bSaveDirty = true;
}

void UInTheDarkGameInstance::SaveAtCheckpoint(FName CheckpointID, const FSavedPlayerState& State)
{
	PlayerStateCache = State;
	PlayerStateCache.LastCheckpointID = CheckpointID;
	bSaveDirty = true;
	WriteSaveToDiskAsync();
}

FTransform UInTheDarkGameInstance::GetSpawnTransform(UObject* WorldContextObject) const
{
	if (HasSavedTransform())
		return PlayerStateCache.Transform;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World)
	{
		for (TActorIterator<APlayerStart> It(World); It; ++It)
			return (*It)->GetActorTransform();
	}
	return FTransform::Identity;
}

// ── Inventory ─────────────────────────────────────────────

void UInTheDarkGameInstance::SetCachedInventory(const TArray<FSavedInventoryEntry>& Inventory)
{
	InventoryCache = Inventory;
	bSaveDirty = true;
}

void UInTheDarkGameInstance::AddInventoryEntry(FName ItemRowName, int32 Quantity)
{
	if (ItemRowName.IsNone() || Quantity <= 0) return;

	for (FSavedInventoryEntry& Entry : InventoryCache)
	{
		if (Entry.ItemRowName == ItemRowName)
		{
			Entry.Quantity += Quantity;
			bSaveDirty = true;
			return;
		}
	}
	InventoryCache.Emplace(ItemRowName, Quantity);
	bSaveDirty = true;
}

void UInTheDarkGameInstance::RemoveInventoryEntry(FName ItemRowName, int32 Quantity)
{
	if (ItemRowName.IsNone() || Quantity <= 0) return;

	for (int32 i = 0; i < InventoryCache.Num(); ++i)
	{
		if (InventoryCache[i].ItemRowName == ItemRowName)
		{
			InventoryCache[i].Quantity -= Quantity;
			if (InventoryCache[i].Quantity <= 0)
			{
				InventoryCache.RemoveAt(i);
			}
			bSaveDirty = true;
			return;
		}
	}
}

void UInTheDarkGameInstance::SetInventoryEntryQuantity(FName ItemRowName, int32 Quantity)
{
	if (ItemRowName.IsNone()) return;

	for (int32 i = 0; i < InventoryCache.Num(); ++i)
	{
		if (InventoryCache[i].ItemRowName == ItemRowName)
		{
			if (Quantity <= 0)
			{
				InventoryCache.RemoveAt(i);
			}
			else
			{
				InventoryCache[i].Quantity = Quantity;
			}
			bSaveDirty = true;
			return;
		}
	}
	if (Quantity > 0)
	{
		InventoryCache.Emplace(ItemRowName, Quantity);
		bSaveDirty = true;
	}
}

int32 UInTheDarkGameInstance::GetInventoryEntryQuantity(FName ItemRowName) const
{
	for (const FSavedInventoryEntry& Entry : InventoryCache)
	{
		if (Entry.ItemRowName == ItemRowName)
		{
			return Entry.Quantity;
		}
	}
	return 0;
}

void UInTheDarkGameInstance::ClearInventory()
{
	if (InventoryCache.Num() == 0) return;
	InventoryCache.Reset();
	bSaveDirty = true;
}

// ── World State ───────────────────────────────────────────

void UInTheDarkGameInstance::MarkWorldActor(FName Category, const FString& ActorID)
{
	if (Category.IsNone() || !IsValidActorID(ActorID)) return;

	TSet<FString>& Set = WorldStateCache.FindOrAdd(Category);
	const int32 SizeBefore = Set.Num();
	Set.Add(ActorID);
	if (Set.Num() > SizeBefore)
	{
		bSaveDirty = true;
	}
}

void UInTheDarkGameInstance::UnmarkWorldActor(FName Category, const FString& ActorID)
{
	TSet<FString>* Set = WorldStateCache.Find(Category);
	if (Set && Set->Remove(ActorID) > 0)
	{
		bSaveDirty = true;
		if (Set->Num() == 0)
		{
			WorldStateCache.Remove(Category);
		}
	}
}

bool UInTheDarkGameInstance::IsWorldActorMarked(FName Category, const FString& ActorID) const
{
	const TSet<FString>* Set = WorldStateCache.Find(Category);
	return Set && Set->Contains(ActorID);
}

TArray<FString> UInTheDarkGameInstance::GetMarkedActors(FName Category) const
{
	const TSet<FString>* Set = WorldStateCache.Find(Category);
	return Set ? Set->Array() : TArray<FString>();
}

void UInTheDarkGameInstance::ClearWorldCategory(FName Category)
{
	if (WorldStateCache.Remove(Category) > 0)
	{
		bSaveDirty = true;
	}
}

// ── Element Progression ───────────────────────────────────

void UInTheDarkGameInstance::UpdateElementProgression(const FSavedElementProgressionEntry& Entry)
{
	if (Entry.ElementName.IsNone()) return;

	for (FSavedElementProgressionEntry& Existing : ElementProgressionCache)
	{
		if (Existing.ElementName == Entry.ElementName)
		{
			Existing = Entry;
			bSaveDirty = true;
			return;
		}
	}
	ElementProgressionCache.Add(Entry);
	bSaveDirty = true;
}

void UInTheDarkGameInstance::SetElementProgressionCache(const TArray<FSavedElementProgressionEntry>& Data)
{
	ElementProgressionCache = Data;
	bSaveDirty = true;
}

void UInTheDarkGameInstance::ClearElementProgression()
{
	if (ElementProgressionCache.Num() == 0) return;
	ElementProgressionCache.Reset();
	bSaveDirty = true;
}

// ── Companion Personality ─────────────────────────────────

void UInTheDarkGameInstance::UpdateCompanionPersonality(float Courage, float Anxiety, float Confidence, float AggressionAffinity, float StealthAffinity)
{
	CompanionPersonalityCache.Courage = Courage;
	CompanionPersonalityCache.Anxiety = Anxiety;
	CompanionPersonalityCache.Confidence = Confidence;
	CompanionPersonalityCache.AggressionAffinity = AggressionAffinity;
	CompanionPersonalityCache.StealthAffinity = StealthAffinity;
	bSaveDirty = true;
}

// ── Encounters ────────────────────────────────────────────

void UInTheDarkGameInstance::MarkEncounterCleared(FName EncounterId)
{
	if (EncounterId.IsNone()) return;

	const int32 SizeBefore = ClearedEncountersCache.Num();
	ClearedEncountersCache.Add(EncounterId);
	if (ClearedEncountersCache.Num() > SizeBefore)
	{
		bSaveDirty = true;
		OnEncounterCleared.Broadcast(EncounterId);
	}
}

void UInTheDarkGameInstance::UnmarkEncounterCleared(FName EncounterId)
{
	if (ClearedEncountersCache.Remove(EncounterId) > 0)
	{
		bSaveDirty = true;
	}
}

bool UInTheDarkGameInstance::IsEncounterCleared(FName EncounterId) const
{
	return !EncounterId.IsNone() && ClearedEncountersCache.Contains(EncounterId);
}

TArray<FName> UInTheDarkGameInstance::GetClearedEncounters() const
{
	return ClearedEncountersCache.Array();
}

void UInTheDarkGameInstance::ClearEncounters()
{
	if (ClearedEncountersCache.Num() == 0) return;
	ClearedEncountersCache.Reset();
	bSaveDirty = true;
}

// ── Meta ──────────────────────────────────────────────────

void UInTheDarkGameInstance::SetLastMapName(const FString& MapName)
{
	FString CleanName = MapName;
	// Strip PIE prefix so save files always store the bare map name
	if (CleanName.StartsWith(TEXT("UEDPIE_")))
	{
		int32 LastUnder = INDEX_NONE;
		CleanName.FindLastChar(TEXT('_'), LastUnder);
		if (LastUnder != INDEX_NONE)
			CleanName = CleanName.Mid(LastUnder + 1);
	}
	if (CachedLastMapName == CleanName) return;
	CachedLastMapName = CleanName;
	bSaveDirty = true;
}

// ── Progress ──────────────────────────────────────────────

void UInTheDarkGameInstance::ClearProgress()
{
	ResetCache();
	bSaveDirty = true;
}

// ── Serialization ─────────────────────────────────────────

void UInTheDarkGameInstance::CopyCacheToPayload(UInTheDarkSaveGame& Payload) const
{
	Payload.SaveVersion = 1;
	Payload.SavedAtUtc = FDateTime::UtcNow();
	Payload.PlayerState = PlayerStateCache;
	Payload.Inventory = InventoryCache;
	Payload.ElementProgression = ElementProgressionCache;
	Payload.CompanionPersonality = CompanionPersonalityCache;
	Payload.LastMapName = CachedLastMapName;

	// Estado del mundo: TMap<FName, TSet<FString>> -> TMap<FName, FWorldActorIDList>
	Payload.WorldState.Reset();
	for (const auto& Pair : WorldStateCache)
	{
		FWorldActorIDList List;
		List.IDs = Pair.Value.Array();
		Payload.WorldState.Add(Pair.Key, MoveTemp(List));
	}

	// Encuentros: TSet<FName> -> TArray<FName>
	Payload.ClearedEncounters = ClearedEncountersCache.Array();
}

void UInTheDarkGameInstance::CopyPayloadToCache(const UInTheDarkSaveGame& Payload)
{
	PlayerStateCache = Payload.PlayerState;
	InventoryCache = Payload.Inventory;
	ElementProgressionCache = Payload.ElementProgression;
	CompanionPersonalityCache = Payload.CompanionPersonality;
	CachedLastMapName = Payload.LastMapName;

	// Estado del mundo: TMap<FName, FWorldActorIDList> -> TMap<FName, TSet<FString>>
	WorldStateCache.Reset();
	for (const auto& Pair : Payload.WorldState)
	{
		TSet<FString>& Set = WorldStateCache.FindOrAdd(Pair.Key);
		for (const FString& ID : Pair.Value.IDs)
		{
			Set.Add(ID);
		}
	}

	// Encuentros: TArray<FName> -> TSet<FName>
	ClearedEncountersCache.Reset();
	for (const FName& Id : Payload.ClearedEncounters)
	{
		ClearedEncountersCache.Add(Id);
	}
}

bool UInTheDarkGameInstance::MigrateSaveIfNeeded(UInTheDarkSaveGame& Payload)
{
	// Añadir casos de migración aquí conforme evolucione SaveVersion.
	if (Payload.SaveVersion <= 0)
	{
		Payload.SaveVersion = 1;
	}
	return true;
}

UInTheDarkSaveGame* UInTheDarkGameInstance::BuildPayload() const
{
	UInTheDarkSaveGame* Payload = Cast<UInTheDarkSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UInTheDarkSaveGame::StaticClass()));
	if (Payload)
	{
		CopyCacheToPayload(*Payload);
	}
	return Payload;
}

// ── Disk IO ───────────────────────────────────────────────

bool UInTheDarkGameInstance::DoesSaveSlotExist() const
{
	return UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex);
}

bool UInTheDarkGameInstance::WriteSaveToDisk()
{
	UInTheDarkSaveGame* Payload = BuildPayload();
	if (!Payload)
	{
		UE_LOG(LogTemp, Error, TEXT("WriteSaveToDisk: failed to create payload."));
		return false;
	}

	const bool bOk = UGameplayStatics::SaveGameToSlot(Payload, SaveSlotName, SaveUserIndex);
	if (bOk)
	{
		bSaveDirty = false;
		OnSaveWritten.Broadcast();
		UE_LOG(LogTemp, Log, TEXT("WriteSaveToDisk: slot '%s' OK."), *SaveSlotName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WriteSaveToDisk: failed for slot '%s'."), *SaveSlotName);
	}
	return bOk;
}

void UInTheDarkGameInstance::WriteSaveToDiskAsync()
{
	if (bAsyncSaveInFlight) return;

	UInTheDarkSaveGame* Payload = BuildPayload();
	if (!Payload)
	{
		OnSaveWrittenAsync.Broadcast(false);
		return;
	}

	bAsyncSaveInFlight = true;
	FAsyncSaveGameToSlotDelegate Callback;
	Callback.BindUObject(this, &UInTheDarkGameInstance::HandleAsyncSaveCompleted);
	UGameplayStatics::AsyncSaveGameToSlot(Payload, SaveSlotName, SaveUserIndex, Callback);
}

void UInTheDarkGameInstance::HandleAsyncSaveCompleted(const FString& Slot, const int32 UserIndex, bool bSuccess)
{
	bAsyncSaveInFlight = false;
	if (bSuccess)
	{
		bSaveDirty = false;
		OnSaveWritten.Broadcast();
	}
	OnSaveWrittenAsync.Broadcast(bSuccess);
}

bool UInTheDarkGameInstance::LoadOrCreateSave()
{
	if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
	{
		ResetCache();
		bHasLoadedSave = true;
		bSaveDirty = false;
		UE_LOG(LogTemp, Log, TEXT("LoadOrCreateSave: no slot '%s', created empty cache."), *SaveSlotName);
		OnSaveLoaded.Broadcast();
		return true;
	}

	USaveGame* Raw = UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex);
	UInTheDarkSaveGame* Loaded = Cast<UInTheDarkSaveGame>(Raw);
	if (!Loaded)
	{
		UE_LOG(LogTemp, Error, TEXT("LoadOrCreateSave: invalid payload in slot '%s'."), *SaveSlotName);
		bHasLoadedSave = false;
		return false;
	}

	if (!MigrateSaveIfNeeded(*Loaded))
	{
		bHasLoadedSave = false;
		return false;
	}

	CopyPayloadToCache(*Loaded);
	bHasLoadedSave = true;
	bSaveDirty = false;
	UE_LOG(LogTemp, Log, TEXT("LoadOrCreateSave: loaded slot '%s'."), *SaveSlotName);
	OnSaveLoaded.Broadcast();
	return true;
}

void UInTheDarkGameInstance::LoadOrCreateSaveAsync()
{
	if (bAsyncLoadInFlight) return;

	if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
	{
		LoadOrCreateSave();
		return;
	}

	bAsyncLoadInFlight = true;
	FAsyncLoadGameFromSlotDelegate Callback;
	Callback.BindUObject(this, &UInTheDarkGameInstance::HandleAsyncLoadCompleted);
	UGameplayStatics::AsyncLoadGameFromSlot(SaveSlotName, SaveUserIndex, Callback);
}

void UInTheDarkGameInstance::HandleAsyncLoadCompleted(const FString& Slot, const int32 UserIndex, USaveGame* Loaded)
{
	bAsyncLoadInFlight = false;

	UInTheDarkSaveGame* Typed = Cast<UInTheDarkSaveGame>(Loaded);
	if (!Typed || !MigrateSaveIfNeeded(*Typed))
	{
		bHasLoadedSave = false;
		return;
	}

	CopyPayloadToCache(*Typed);
	bHasLoadedSave = true;
	bSaveDirty = false;
	OnSaveLoaded.Broadcast();
}

bool UInTheDarkGameInstance::DeleteSave()
{
	const bool bOk = UGameplayStatics::DeleteGameInSlot(SaveSlotName, SaveUserIndex);
	if (bOk)
	{
		ResetCache();
		bHasLoadedSave = false;
		bSaveDirty = false;
	}
	return bOk;
}

// ── Map hook ──────────────────────────────────────────────

void UInTheDarkGameInstance::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (!LoadedWorld) return;
	if (DoesSaveSlotExist())
	{
		SetLastMapName(LoadedWorld->GetMapName());
	}
	if (bAutosaveOnMapChange && bSaveDirty)
	{
		WriteSaveToDiskAsync();
	}
}
