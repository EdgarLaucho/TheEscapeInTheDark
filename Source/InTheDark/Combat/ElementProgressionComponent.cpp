#include "Combat/ElementProgressionComponent.h"
#include "SaveSystem/InTheDarkGameInstance.h"

UElementProgressionComponent::UElementProgressionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UElementProgressionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UElementProgressionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
}

FElementProgressionData* UElementProgressionComponent::FindElementProgressionData(FName ElementName)
{
	for (FElementProgressionData& Data : ElementProgressionData)
	{
		if (Data.ElementName == ElementName)
		{
			return &Data;
		}
	}
	return nullptr;
}

bool UElementProgressionComponent::GetElementProgressionData(FName ElementName, FElementProgressionData& OutData) const
{
	for (const FElementProgressionData& Data : ElementProgressionData)
	{
		if (Data.ElementName == ElementName)
		{
			OutData = Data;
			return true;
		}
	}
	return false;
}

static FSavedElementProgressionEntry MakeSavedEntry(const FElementProgressionData& Data)
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

void UElementProgressionComponent::UnlockElement(FName ElementName)
{
	FElementProgressionData* Data = FindElementProgressionData(ElementName);

	if (!Data)
		return;

	Data->bUnlocked = true;

	if (UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(
		GetWorld() ? GetWorld()->GetGameInstance() : nullptr))
	{
		GI->UpdateElementProgression(MakeSavedEntry(*Data));
	}
}

TArray<FName> UElementProgressionComponent::GetUnlockedElements() const
{
	TArray<FName> Result;

	for (const FElementProgressionData& Data : ElementProgressionData)
	{
		if (Data.bUnlocked && !Data.bIsFusionElement)
		{
			Result.Add(Data.ElementName);
		}
	}

	return Result;
}

void UElementProgressionComponent::AddKillToElement(FName ElementName, int32 KillAmount)
{
	FElementProgressionData* Data = FindElementProgressionData(ElementName);

	if (!Data)
		return;
	
	if (!Data -> bUnlocked)
		return;
	

	if (Data-> Level >= Data->MaxLevel)
		return;
	

	Data->KillCount += KillAmount;
	while (Data->Level < Data->MaxLevel)
	{
		const int32 RequiredIndex = Data->Level-1;

		if (!Data->KillsRequiredPerLevel.IsValidIndex(RequiredIndex))
			return;

		const int32 RequiredKills = Data->KillsRequiredPerLevel[RequiredIndex];

		if (Data->KillCount<RequiredKills)
			return;

		Data->KillCount -= RequiredKills;
		Data->Level++;
	
		Data->MaxUnlockedComboStep= Data->Level-1;
		Data->DamageMultiplier += 0.15f;
		Data->ScaleMultiplier +=0.10f;
	}

	if (UInTheDarkGameInstance* GI = Cast<UInTheDarkGameInstance>(
		GetWorld() ? GetWorld()->GetGameInstance() : nullptr))
	{
		GI->UpdateElementProgression(MakeSavedEntry(*Data));
	}
}

const TArray<FElementProgressionData>& UElementProgressionComponent::GetAllElementProgressionData() const
{
	return ElementProgressionData;
}

void UElementProgressionComponent::RestoreFromSave(const TArray<FSavedElementProgressionEntry>& SavedData)
{
	for (const FSavedElementProgressionEntry& Saved : SavedData)
	{
		FElementProgressionData* Data = FindElementProgressionData(Saved.ElementName);
		if (!Data) continue;
		Data->Level                = Saved.Level;
		Data->KillCount            = Saved.KillCount;
		Data->DamageMultiplier     = Saved.DamageMultiplier;
		Data->ScaleMultiplier      = Saved.ScaleMultiplier;
		Data->MaxUnlockedComboStep = Saved.MaxUnlockedComboStep;
		Data->bUnlocked            = Saved.bUnlocked;
	}
}


