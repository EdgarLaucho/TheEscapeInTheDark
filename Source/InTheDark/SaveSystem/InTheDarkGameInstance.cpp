#include "SaveSystem/InTheDarkGameInstance.h"
#include "SaveSystem/InTheDarkSaveGame.h"
#include "Combat/ElementProgressionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Misc/CoreDelegates.h"
#include "TimerManager.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "EngineUtils.h"
#include "AI/PartnerStateInterface.h"
#include "SaveSystem/PlayerMusicStateInterface.h"

UInTheDarkGameInstance::UInTheDarkGameInstance()
{
	CompanionClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/AI/Partner/Blueprints/BP_PartnerAICharacter.BP_PartnerAICharacter_C")));
	DefaultFallbackMusic = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Level/Lvl/Sound/HouseMusic.HouseMusic")));
	LoadingScreenWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/Level/Lvl/MainMenu/UserInterface/WBP_LoadingScreen.WBP_LoadingScreen_C")));
}

UInTheDarkGameInstance* UInTheDarkGameInstance::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;
	return Cast<UInTheDarkGameInstance>(UGameplayStatics::GetGameInstance(WorldContextObject));
}

namespace
{
	FString CleanMapNameForSave(FString MapName)
	{
		if (MapName.StartsWith(TEXT("UEDPIE_")))
		{
			int32 FirstUnder = INDEX_NONE;
			int32 SecondUnder = INDEX_NONE;

			if (MapName.FindChar(TEXT('_'), FirstUnder))
			{
				const FString AfterPrefix = MapName.Mid(FirstUnder + 1);

				if (AfterPrefix.FindChar(TEXT('_'), SecondUnder))
				{
					MapName = AfterPrefix.Mid(SecondUnder + 1);
				}
			}
		}

		return MapName;
	}

	FSavedElementProgressionEntry MakeSavedElementProgressionEntry(const FElementProgressionData& Data)
	{
		FSavedElementProgressionEntry Entry;
		Entry.ElementName = Data.ElementName;
		Entry.Level = Data.Level;
		Entry.KillCount = Data.KillCount;
		Entry.DamageMultiplier = Data.DamageMultiplier;
		Entry.ScaleMultiplier = Data.ScaleMultiplier;
		Entry.MaxUnlockedComboStep = Data.MaxUnlockedComboStep;
		Entry.bUnlocked = Data.bUnlocked;
		return Entry;
	}
}

void UInTheDarkGameInstance::Init()
{
	Super::Init();
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UInTheDarkGameInstance::OnPostLoadMapWithWorld);
	SaveSlotName = GetSlotName(CurrentSlotIndex);
	LoadOrCreateSave();
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

FString UInTheDarkGameInstance::GetSlotName(int32 SlotIndex) const
{
	return FString::Printf(TEXT("Save_%d"), SlotIndex);
}

bool UInTheDarkGameInstance::IsValidSlotIndex(int32 SlotIndex) const
{
	return SlotIndex >= 0 && SlotIndex < MaxSlots;
}

bool UInTheDarkGameInstance::IsMainMenuMap(const FString& MapName) const
{
	if (MainMenuLevelName.IsNone()) return false;

	const FString MenuName = MainMenuLevelName.ToString();
	return MapName.Equals(MenuName) || MapName.EndsWith(TEXT("_") + MenuName);
}

void UInTheDarkGameInstance::ResetCache()
{
	ElementProgressionCache.Reset();
	CompanionPersonalityCache = FSavedCompanionPersonality();
	CompanionStateCache = FSavedCompanionState();
	TutorialStateCache = FSavedTutorialState();
	MusicStateCache = FSavedMusicState();
	WorldStateCache.Reset();
	ClearedEncountersCache.Reset();
	SeenDialoguesCache.Reset();
	PlayerStateCache = FSavedPlayerState();
	CachedLastMapName.Reset();
}

bool UInTheDarkGameInstance::IsValidActorID(const FString& ActorID)
{
	return !ActorID.IsEmpty() && !ActorID.Equals(TEXT("None"), ESearchCase::IgnoreCase);
}

void UInTheDarkGameInstance::RequestSaveSnapshot()
{
	if (bRequestingSaveSnapshot) return;
	bRequestingSaveSnapshot = true;
	CapturePlayerSnapshot();
	CaptureElementProgressionSnapshot();
	CaptureMusicSnapshot();
	OnSaveSnapshotRequested.Broadcast();
	CaptureElementProgressionSnapshot();
	CaptureMusicSnapshot();
	bRequestingSaveSnapshot = false;
}

void UInTheDarkGameInstance::CapturePlayerSnapshot()
{
	UWorld* World = GetWorld();
	if (!World || IsMainMenuMap(World->GetMapName())) return;

	APlayerController* PC = World->GetFirstPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn) return;

	PlayerStateCache.Transform = Pawn->GetActorTransform();
	PlayerStateCache.bHasSavedTransform = true;
	bSaveDirty = true;
}

void UInTheDarkGameInstance::CaptureElementProgressionSnapshot()
{
	UWorld* World = GetWorld();
	if (!World || IsMainMenuMap(World->GetMapName())) return;

	APlayerController* PC = World->GetFirstPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn) return;

	UElementProgressionComponent* ElementProgression = Pawn->FindComponentByClass<UElementProgressionComponent>();
	if (!ElementProgression) return;

	ElementProgressionCache.Reset();
	for (const FElementProgressionData& Data : ElementProgression->GetAllElementProgressionData())
	{
		if (!Data.ElementName.IsNone())
		{
			ElementProgressionCache.Add(MakeSavedElementProgressionEntry(Data));
		}
	}

	bSaveDirty = true;
}

void UInTheDarkGameInstance::SwitchToSlot(int32 SlotIndex)
{
	if (!IsValidSlotIndex(SlotIndex)) return;

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
	if (!IsValidSlotIndex(SlotIndex)) return;

	const FString SlotName = GetSlotName(SlotIndex);
	UGameplayStatics::DeleteGameInSlot(SlotName, SaveUserIndex);

	if (SlotIndex == CurrentSlotIndex)
	{
		ResetCache();
		bHasLoadedSave = false;
		bSaveDirty = false;
	}
}

void UInTheDarkGameInstance::StartNewGameFromMenu(UObject* WorldContextObject, int32 SlotIndex)
{
	if (!IsValidSlotIndex(SlotIndex) || DefaultGameLevelName.IsNone()) return;

	CurrentSlotIndex = SlotIndex;
	SaveSlotName = GetSlotName(SlotIndex);
	ClearProgress();
	SetLastMapName(DefaultGameLevelName.ToString());
	WriteSaveToDisk();
	UGameplayStatics::SetGamePaused(WorldContextObject, false);
	UGameplayStatics::OpenLevel(WorldContextObject, DefaultGameLevelName);
}

void UInTheDarkGameInstance::ContinueGameFromMenu(UObject* WorldContextObject, int32 SlotIndex)
{
	if (!IsValidSlotIndex(SlotIndex) || DefaultGameLevelName.IsNone()) return;

	SwitchToSlot(SlotIndex);
	FName TargetLevel = DefaultGameLevelName;

	if (!CachedLastMapName.IsEmpty() && !IsMainMenuMap(CachedLastMapName))
	{
		FString SavedMapName = CachedLastMapName;
		const FString DefaultMapName = DefaultGameLevelName.ToString();

		if (!SavedMapName.Equals(DefaultMapName) && DefaultMapName.EndsWith(SavedMapName))
			SavedMapName = DefaultMapName;

		TargetLevel = FName(*SavedMapName);
	}

	UGameplayStatics::SetGamePaused(WorldContextObject, false);
	UGameplayStatics::OpenLevel(WorldContextObject, TargetLevel);
}

void UInTheDarkGameInstance::SaveCurrentGameAndOpenMainMenu(UObject* WorldContextObject)
{
	if (bSaveDirty)
		WriteSaveToDisk();

	OpenMainMenuWithoutSaving(WorldContextObject);
}

void UInTheDarkGameInstance::OpenMainMenuWithoutSaving(UObject* WorldContextObject)
{
	if (MainMenuLevelName.IsNone()) return;
	UGameplayStatics::SetGamePaused(WorldContextObject, false);
	UGameplayStatics::OpenLevel(WorldContextObject, MainMenuLevelName);
}

void UInTheDarkGameInstance::SavePlayerState(const FSavedPlayerState& State)
{
	PlayerStateCache = State;
	PlayerStateCache.bHasSavedTransform = true;
	bSaveDirty = true;
}

void UInTheDarkGameInstance::SaveAtCheckpoint(FName CheckpointID, const FSavedPlayerState& State)
{
	PlayerStateCache = State;
	PlayerStateCache.LastCheckpointID = CheckpointID;
	PlayerStateCache.bHasSavedTransform = true;
	bSaveDirty = true;
	WriteSaveToDiskAsync();
}

FTransform UInTheDarkGameInstance::GetSpawnTransform(UObject* WorldContextObject) const
{
	if (HasValidSavedPlayerTransform())
		return PlayerStateCache.Transform;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (World)
	{
		for (TActorIterator<APlayerStart> It(World); It; ++It)
			return (*It)->GetActorTransform();
	}

	return FTransform::Identity;
}

void UInTheDarkGameInstance::MarkWorldActor(FName Category, const FString& ActorID)
{
	if (Category.IsNone() || !IsValidActorID(ActorID)) return;

	TSet<FString>& Set = WorldStateCache.FindOrAdd(Category);
	const int32 SizeBefore = Set.Num();
	Set.Add(ActorID);

	if (Set.Num() > SizeBefore) bSaveDirty = true;
}

void UInTheDarkGameInstance::UnmarkWorldActor(FName Category, const FString& ActorID)
{
	TSet<FString>* Set = WorldStateCache.Find(Category);
	if (Set && Set->Remove(ActorID) > 0)
	{
		bSaveDirty = true;
		if (Set->Num() == 0) WorldStateCache.Remove(Category);
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
	if (WorldStateCache.Remove(Category) > 0) bSaveDirty = true;
}

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

void UInTheDarkGameInstance::UpdateCompanionPersonality(float Courage, float Anxiety, float Confidence, float AggressionAffinity, float StealthAffinity)
{
	CompanionPersonalityCache.Courage = Courage;
	CompanionPersonalityCache.Anxiety = Anxiety;
	CompanionPersonalityCache.Confidence = Confidence;
	CompanionPersonalityCache.AggressionAffinity = AggressionAffinity;
	CompanionPersonalityCache.StealthAffinity = StealthAffinity;
	bSaveDirty = true;
}

void UInTheDarkGameInstance::UpdateCompanionState(const FTransform& Transform, uint8 CurrentStateValue, bool bHasAwoken)
{
	CompanionStateCache.bHasSavedState = true;
	CompanionStateCache.bHasAwoken = CompanionStateCache.bHasAwoken || bHasAwoken;
	CompanionStateCache.Transform = Transform;
	CompanionStateCache.CurrentStateValue = CurrentStateValue;
	bSaveDirty = true;
}

void UInTheDarkGameInstance::MarkCompanionAwoken(const FTransform& Transform, uint8 CurrentStateValue)
{
	UpdateCompanionState(Transform, CurrentStateValue, true);
}

void UInTheDarkGameInstance::UpdateTutorialState(int32 SavedStep, bool bFinished)
{
	const int32 CleanStep = FMath::Max(0, SavedStep);

	if (TutorialStateCache.SavedStep == CleanStep && TutorialStateCache.bFinished == bFinished)
		return;

	TutorialStateCache.SavedStep = CleanStep;
	TutorialStateCache.bFinished = bFinished;
	bSaveDirty = true;
}

void UInTheDarkGameInstance::SetCurrentMusicZone(FName ZoneId, USoundBase* Music, float FadeTime)
{
	const FName MusicId = Music ? Music->GetFName() : NAME_None;
	PlayMusicByAsset(Music, MusicId, ZoneId, FadeTime);
}

void UInTheDarkGameInstance::PlayMusicByAsset(USoundBase* Music, FName MusicId, FName ZoneId, float FadeTime)
{
	if (!Music) return;

	const FString NewPath = FSoftObjectPath(Music).ToString();
	const bool bSameMusic = MusicStateCache.bShouldBePlaying && MusicStateCache.MusicAssetPath == NewPath;

	MusicStateCache.CurrentMusicId = MusicId.IsNone() ? Music->GetFName() : MusicId;
	MusicStateCache.CurrentMusicZoneId = ZoneId;
	MusicStateCache.MusicAssetPath = NewPath;
	MusicStateCache.bShouldBePlaying = true;
	bSaveDirty = true;

	UAudioComponent* AudioComponent = GetPlayerMusicAudioComponent();
	if (bSameMusic && AudioComponent && AudioComponent->IsPlaying())
	{
		SetPlayerControllerCurrentMusic(Music);
		return;
	}

	ApplyMusicToAudioComponent(Music, FadeTime);
}

void UInTheDarkGameInstance::RestoreMusicFromSave(float FadeTime)
{
	if (!MusicStateCache.bShouldBePlaying)
		return;

	USoundBase* Music = nullptr;
	if (!MusicStateCache.MusicAssetPath.IsEmpty())
	{
		Music = Cast<USoundBase>(FSoftObjectPath(MusicStateCache.MusicAssetPath).TryLoad());
	}

	if (!Music && !DefaultFallbackMusic.IsNull())
	{
		Music = DefaultFallbackMusic.LoadSynchronous();
	}

	if (!Music) return;

	ApplyMusicToAudioComponent(Music, FadeTime);
}

void UInTheDarkGameInstance::StopMusic(float FadeTime, bool bRememberSilence)
{
	if (UAudioComponent* AudioComponent = GetPlayerMusicAudioComponent())
	{
		if (FadeTime > 0.f)
			AudioComponent->FadeOut(FadeTime, 0.f);
		else
			AudioComponent->Stop();
	}
	else if (ActiveMusicComponent)
	{
		if (FadeTime > 0.f)
			ActiveMusicComponent->FadeOut(FadeTime, 0.f);
		else
			ActiveMusicComponent->Stop();
	}

	if (bRememberSilence)
	{
		MusicStateCache.bShouldBePlaying = false;
		bSaveDirty = true;
	}
}

void UInTheDarkGameInstance::CaptureMusicSnapshot()
{
	if (USoundBase* CurrentMusic = GetPlayerControllerCurrentMusic())
	{
		MusicStateCache.CurrentMusicId = CurrentMusic->GetFName();
		if (MusicStateCache.CurrentMusicZoneId.IsNone())
			MusicStateCache.CurrentMusicZoneId = MusicStateCache.CurrentMusicId;
		MusicStateCache.MusicAssetPath = FSoftObjectPath(CurrentMusic).ToString();
		MusicStateCache.bShouldBePlaying = true;
		bSaveDirty = true;
		return;
	}

	UAudioComponent* AudioComponent = GetPlayerMusicAudioComponent();
	USoundBase* Sound = AudioComponent ? AudioComponent->Sound : nullptr;
	if (Sound)
	{
		MusicStateCache.CurrentMusicId = Sound->GetFName();
		if (MusicStateCache.CurrentMusicZoneId.IsNone())
			MusicStateCache.CurrentMusicZoneId = MusicStateCache.CurrentMusicId;
		MusicStateCache.MusicAssetPath = FSoftObjectPath(Sound).ToString();
		MusicStateCache.bShouldBePlaying = AudioComponent->IsPlaying();
		bSaveDirty = true;
	}
}

bool UInTheDarkGameInstance::IsDialogueSeen(FName DialogueID) const
{
	return !DialogueID.IsNone() && SeenDialoguesCache.Contains(DialogueID);
}

void UInTheDarkGameInstance::MarkDialogueSeen(FName DialogueID)
{
	if (DialogueID.IsNone()) return;

	const int32 SizeBefore = SeenDialoguesCache.Num();
	SeenDialoguesCache.Add(DialogueID);

	if (SeenDialoguesCache.Num() > SizeBefore)
		bSaveDirty = true;
}

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
		bSaveDirty = true;
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

void UInTheDarkGameInstance::SetLastMapName(const FString& MapName)
{
	const FString CleanName = CleanMapNameForSave(MapName);
	if (CachedLastMapName == CleanName) return;
	CachedLastMapName = CleanName;
	bSaveDirty = true;
}

void UInTheDarkGameInstance::ClearProgress()
{
	ResetCache();
	bSaveDirty = true;
}

void UInTheDarkGameInstance::CopyCacheToPayload(UInTheDarkSaveGame& Payload) const
{
	Payload.SaveVersion = 1;
	Payload.SavedAtUtc = FDateTime::UtcNow();
	Payload.PlayerState = PlayerStateCache;
	Payload.ElementProgression = ElementProgressionCache;
	Payload.CompanionPersonality = CompanionPersonalityCache;
	Payload.CompanionState = CompanionStateCache;
	Payload.TutorialState = TutorialStateCache;
	Payload.MusicState = MusicStateCache;
	Payload.LastMapName = CachedLastMapName;

	Payload.WorldState.Reset();

	for (const auto& Pair : WorldStateCache)
	{
		FWorldActorIDList List;
		List.IDs = Pair.Value.Array();
		Payload.WorldState.Add(Pair.Key, MoveTemp(List));
	}

	Payload.ClearedEncounters = ClearedEncountersCache.Array();
	Payload.SeenDialogues = SeenDialoguesCache.Array();
}

void UInTheDarkGameInstance::CopyPayloadToCache(const UInTheDarkSaveGame& Payload)
{
	PlayerStateCache = Payload.PlayerState;
	ElementProgressionCache = Payload.ElementProgression;
	CompanionPersonalityCache = Payload.CompanionPersonality;
	CompanionStateCache = Payload.CompanionState;
	TutorialStateCache = Payload.TutorialState;
	MusicStateCache = Payload.MusicState;
	CachedLastMapName = Payload.LastMapName;

	WorldStateCache.Reset();
	for (const auto& Pair : Payload.WorldState)
	{
		TSet<FString>& Set = WorldStateCache.FindOrAdd(Pair.Key);
		for (const FString& ID : Pair.Value.IDs)
		{
			Set.Add(ID);
		}
	}

	ClearedEncountersCache.Reset();
	for (const FName& Id : Payload.ClearedEncounters)
	{
		ClearedEncountersCache.Add(Id);
	}

	SeenDialoguesCache.Reset();
	for (const FName& Id : Payload.SeenDialogues)
	{
		SeenDialoguesCache.Add(Id);
	}
}

bool UInTheDarkGameInstance::MigrateSaveIfNeeded(UInTheDarkSaveGame& Payload)
{
	if (Payload.SaveVersion <= 0) Payload.SaveVersion = 1;
	return true;
}

UInTheDarkSaveGame* UInTheDarkGameInstance::BuildPayload() const
{
	UInTheDarkSaveGame* Payload = Cast<UInTheDarkSaveGame>(UGameplayStatics::CreateSaveGameObject(UInTheDarkSaveGame::StaticClass()));

	if (Payload) CopyCacheToPayload(*Payload);
	return Payload;
}

bool UInTheDarkGameInstance::DoesSaveSlotExist() const
{
	return UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex);
}

bool UInTheDarkGameInstance::WriteSaveToDisk()
{
	RequestSaveSnapshot();

	UInTheDarkSaveGame* Payload = BuildPayload();

	if (!Payload) return false;

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

	RequestSaveSnapshot();

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

void UInTheDarkGameInstance::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (!LoadedWorld) return;

	if (DoesSaveSlotExist() && !IsMainMenuMap(LoadedWorld->GetMapName()))
	{
		SetLastMapName(LoadedWorld->GetMapName());
		PendingLoadRestoreAttempts = 0;
		bWaitingForSavedPlayerGround = false;
		FTimerDelegate RestoreDelegate = FTimerDelegate::CreateUObject(this, &UInTheDarkGameInstance::RestoreLoadedWorldState, LoadedWorld);
		LoadedWorld->GetTimerManager().SetTimerForNextTick(RestoreDelegate);
		return;
	}

	if (bAutosaveOnMapChange && bSaveDirty)
		WriteSaveToDiskAsync();
}

bool UInTheDarkGameInstance::HasValidSavedPlayerTransform() const
{
	if (!PlayerStateCache.bHasSavedTransform) return false;

	const FVector Location = PlayerStateCache.Transform.GetLocation();
	const FQuat Rotation = PlayerStateCache.Transform.GetRotation();
	const FVector Scale = PlayerStateCache.Transform.GetScale3D();

	return Location.ContainsNaN() == false
		&& Rotation.ContainsNaN() == false
		&& Scale.ContainsNaN() == false;
}

void UInTheDarkGameInstance::RestoreLoadedWorldState(UWorld* LoadedWorld)
{
	if (!LoadedWorld || IsMainMenuMap(LoadedWorld->GetMapName())) return;

	APlayerController* PC = LoadedWorld->GetFirstPlayerController();
	APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;

	if (!PlayerPawn && PendingLoadRestoreAttempts < 20)
	{
		++PendingLoadRestoreAttempts;
		FTimerHandle RetryHandle;
		FTimerDelegate RetryDelegate = FTimerDelegate::CreateUObject(this, &UInTheDarkGameInstance::RestoreLoadedWorldState, LoadedWorld);
		LoadedWorld->GetTimerManager().SetTimer(RetryHandle, RetryDelegate, 0.1f, false);
		return;
	}

	if (PlayerPawn && HasValidSavedPlayerTransform())
	{
		ShowLoadingScreen();

		FTransform SafePlayerTransform;
		if (!BuildSafePlayerLoadTransform(LoadedWorld, PlayerPawn, SafePlayerTransform))
		{
			if (!bWaitingForSavedPlayerGround)
			{
				bWaitingForSavedPlayerGround = true;
				PreparePlayerForStreamingRestore(PlayerPawn);
			}

			if (PendingLoadRestoreAttempts < 80)
			{
				++PendingLoadRestoreAttempts;
				FTimerHandle RetryHandle;
				FTimerDelegate RetryDelegate = FTimerDelegate::CreateUObject(this, &UInTheDarkGameInstance::RestoreLoadedWorldState, LoadedWorld);
				LoadedWorld->GetTimerManager().SetTimer(RetryHandle, RetryDelegate, 0.1f, false);
				return;
			}

			PlayerPawn->SetActorTransform(GetFallbackPlayerStartTransform(LoadedWorld), false, nullptr, ETeleportType::TeleportPhysics);
		}
		else
		{
			PlayerPawn->SetActorTransform(SafePlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
		}

		bWaitingForSavedPlayerGround = false;
		FinishPlayerStreamingRestore(PlayerPawn);
		HideLoadingScreen();
	}

	if (PlayerPawn && CompanionStateCache.bHasAwoken)
	{
		if (AActor* CompanionActor = FindCompanionActor(LoadedWorld))
		{
			const FTransform CompanionTransform = BuildCompanionLoadTransform(LoadedWorld, PlayerPawn);
			CompanionActor->SetActorTransform(CompanionTransform, false, nullptr, ETeleportType::TeleportPhysics);
			ReactivateLoadedCompanion(CompanionActor);
		}
	}

	RestoreMusicFromSave(0.25f);

	if (bAutosaveOnMapChange && bSaveDirty)
		WriteSaveToDiskAsync();
}

bool UInTheDarkGameInstance::BuildSafePlayerLoadTransform(UWorld* World, const APawn* PlayerPawn, FTransform& OutTransform) const
{
	if (!World || !PlayerPawn || !HasValidSavedPlayerTransform()) return false;

	FVector TargetLocation = PlayerStateCache.Transform.GetLocation();

	if (UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(World))
	{
		FNavLocation ProjectedLocation;
		if (NavSystem->ProjectPointToNavigation(TargetLocation, ProjectedLocation, FVector(250.f, 250.f, 500.f)))
		{
			TargetLocation = ProjectedLocation.Location;
		}
	}

	FHitResult Hit;
	const FVector TraceStart = TargetLocation + FVector(0.f, 0.f, 800.f);
	const FVector TraceEnd = TargetLocation - FVector(0.f, 0.f, 3000.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PlayerLoadGroundTrace), false, PlayerPawn);

	if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
		return false;

	if (!Hit.bBlockingHit)
		return false;

	TargetLocation = Hit.Location + FVector(0.f, 0.f, 8.f);
	OutTransform = PlayerStateCache.Transform;
	OutTransform.SetLocation(TargetLocation);
	return true;
}

void UInTheDarkGameInstance::PreparePlayerForStreamingRestore(APawn* PlayerPawn) const
{
	if (!PlayerPawn) return;

	FTransform StreamingTransform = PlayerStateCache.Transform;
	StreamingTransform.SetLocation(PlayerStateCache.Transform.GetLocation() + FVector(0.f, 0.f, 600.f));
	PlayerPawn->SetActorTransform(StreamingTransform, false, nullptr, ETeleportType::TeleportPhysics);
	PlayerPawn->SetActorHiddenInGame(true);
	PlayerPawn->SetActorEnableCollision(false);

	if (ACharacter* Character = Cast<ACharacter>(PlayerPawn))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
	}
}

void UInTheDarkGameInstance::FinishPlayerStreamingRestore(APawn* PlayerPawn) const
{
	if (!PlayerPawn) return;

	PlayerPawn->SetActorHiddenInGame(false);
	PlayerPawn->SetActorEnableCollision(true);

	if (ACharacter* Character = Cast<ACharacter>(PlayerPawn))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_Walking);
		}
	}
}

FTransform UInTheDarkGameInstance::GetFallbackPlayerStartTransform(UWorld* World) const
{
	if (World)
	{
		for (TActorIterator<APlayerStart> It(World); It; ++It)
			return (*It)->GetActorTransform();
	}

	return FTransform::Identity;
}

void UInTheDarkGameInstance::ShowLoadingScreen()
{
	if (ActiveLoadingScreenWidget && ActiveLoadingScreenWidget->IsInViewport())
		return;

	UClass* WidgetClass = LoadingScreenWidgetClass.LoadSynchronous();
	if (!WidgetClass) return;

	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC) return;

	ActiveLoadingScreenWidget = CreateWidget<UUserWidget>(PC, WidgetClass);
	if (ActiveLoadingScreenWidget)
	{
		ActiveLoadingScreenWidget->AddToViewport(1000);
	}
}

void UInTheDarkGameInstance::HideLoadingScreen()
{
	if (ActiveLoadingScreenWidget)
	{
		ActiveLoadingScreenWidget->RemoveFromParent();
		ActiveLoadingScreenWidget = nullptr;
	}
}

UAudioComponent* UInTheDarkGameInstance::GetPlayerMusicAudioComponent() const
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	return PC ? PC->FindComponentByClass<UAudioComponent>() : nullptr;
}

USoundBase* UInTheDarkGameInstance::GetPlayerControllerCurrentMusic() const
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;

	if (!PC || !PC->GetClass()->ImplementsInterface(UPlayerMusicStateInterface::StaticClass()))
		return nullptr;

	return IPlayerMusicStateInterface::Execute_GetCurrentMusic(PC);
}

void UInTheDarkGameInstance::SetPlayerControllerCurrentMusic(USoundBase* Music) const
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;

	if (PC && PC->GetClass()->ImplementsInterface(UPlayerMusicStateInterface::StaticClass()))
		IPlayerMusicStateInterface::Execute_SetCurrentMusic(PC, Music);
}

void UInTheDarkGameInstance::ApplyMusicToAudioComponent(USoundBase* Music, float FadeTime)
{
	if (!Music) return;

	if (UAudioComponent* AudioComponent = GetPlayerMusicAudioComponent())
	{
		if (AudioComponent->Sound == Music && AudioComponent->IsPlaying())
		{
			SetPlayerControllerCurrentMusic(Music);
			return;
		}

		AudioComponent->SetSound(Music);
		SetPlayerControllerCurrentMusic(Music);

		if (FadeTime > 0.f)
			AudioComponent->FadeIn(FadeTime);
		else
			AudioComponent->Play();

		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	if (ActiveMusicComponent && ActiveMusicComponent->IsPlaying())
	{
		ActiveMusicComponent->Stop();
	}

	ActiveMusicComponent = UGameplayStatics::SpawnSound2D(World, Music, 1.f, 1.f, 0.f, nullptr, true, false);
	if (ActiveMusicComponent && FadeTime > 0.f)
	{
		ActiveMusicComponent->FadeIn(FadeTime);
	}
}

AActor* UInTheDarkGameInstance::FindCompanionActor(UWorld* World) const
{
	if (!World) return nullptr;

	UClass* LoadedCompanionClass = CompanionClass.LoadSynchronous();

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor)) continue;

		if (LoadedCompanionClass && Actor->IsA(LoadedCompanionClass))
			return Actor;

		if (!LoadedCompanionClass && Actor->GetClass()->GetName().Contains(TEXT("PartnerAICharacter")))
			return Actor;
	}

	return nullptr;
}

FTransform UInTheDarkGameInstance::BuildCompanionLoadTransform(UWorld* World, const APawn* PlayerPawn) const
{
	if (!World || !PlayerPawn) return FTransform::Identity;

	const FRotator PlayerRotation = PlayerPawn->GetActorRotation();
	FVector DesiredLocation = PlayerPawn->GetActorLocation()
		+ PlayerPawn->GetActorForwardVector() * CompanionLoadOffset.X
		+ PlayerPawn->GetActorRightVector() * CompanionLoadOffset.Y
		+ FVector(0.f, 0.f, CompanionLoadOffset.Z);

	if (UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(World))
	{
		FNavLocation ProjectedLocation;
		if (NavSystem->ProjectPointToNavigation(DesiredLocation, ProjectedLocation, FVector(300.f, 300.f, 500.f)))
		{
			DesiredLocation = ProjectedLocation.Location;
		}
	}

	FHitResult Hit;
	const FVector TraceStart = DesiredLocation + FVector(0.f, 0.f, 300.f);
	const FVector TraceEnd = DesiredLocation - FVector(0.f, 0.f, 1200.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CompanionLoadGroundTrace), false, PlayerPawn);

	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
	{
		DesiredLocation = Hit.Location + FVector(0.f, 0.f, 5.f);
	}

	return FTransform(PlayerRotation, DesiredLocation, FVector::OneVector);
}

void UInTheDarkGameInstance::ReactivateLoadedCompanion(AActor* CompanionActor)
{
	if (!IsValid(CompanionActor)) return;

	CompanionActor->SetActorHiddenInGame(false);
	CompanionActor->SetActorEnableCollision(true);
	CompanionActor->SetActorTickEnabled(true);

	if (APawn* CompanionPawn = Cast<APawn>(CompanionActor))
	{
		if (!CompanionPawn->GetController())
			CompanionPawn->SpawnDefaultController();
	}

	if (ACharacter* CompanionCharacter = Cast<ACharacter>(CompanionActor))
	{
		if (UCharacterMovementComponent* Movement = CompanionCharacter->GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
	}

	if (CompanionActor->GetClass()->ImplementsInterface(UPartnerStateInterface::StaticClass()))
	{
		const uint8 SavedState = CompanionStateCache.CurrentStateValue;
		const uint8 StateToApply = (SavedState == 0 || SavedState == 1) ? CompanionLoadedStateValue : SavedState;
		IPartnerStateInterface::Execute_ApplyPartnerState(CompanionActor, StateToApply);
	}
}