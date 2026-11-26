// Fill out your copyright notice in the Description page of Project Settings.

#include "GymSubsystem.h"
#include "GymSnitchTrainer.h"
#include "QuidditchWorldSubsystem.h"
#include "Broom.h"
#include "Quaffle.h"
#include "Bludger.h"
#include "Snitch.h"
#include "AIBroomController.h"
#include "GoalTargetingComponent.h"
#include "BroomDataAsset.h"
#include "DataAsset/QuaffleDataAsset.h"
#include "DataAsset/BludgerDataAsset.h"
#include "DataAsset/SnitchDataAsset.h"
#include "DataAsset/BroomAIDataAsset.h"
#include "DataAsset/GoalTargetingDataAsset.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "Gym"

UGymSubsystem* UGymSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UGymSubsystem>() : nullptr;
}

bool UGymSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UGymSubsystem::Deinitialize()
{
	// Data assets outlive the world (in the editor they're the real assets): undo play-time edits.
	ResetTuning();
	Super::Deinitialize();
}

// --- Tuning ----------------------------------------------------------------------------------

UObject* UGymSubsystem::ResolveTarget(EGymTuningTarget Target) const
{
	const UWorld* World = GetWorld();
	const UQuidditchWorldSubsystem* Quidditch = UQuidditchWorldSubsystem::Get(this);
	if (!World) return nullptr;

	const ABroom* PlayerBroom = Cast<ABroom>(UGameplayStatics::GetPlayerPawn(World, 0));

	switch (Target)
	{
	case EGymTuningTarget::PlayerBroom:
		return PlayerBroom ? PlayerBroom->GetBroomData() : nullptr;

	case EGymTuningTarget::PlayerGoalTargeting:
		return PlayerBroom && PlayerBroom->GetGoalTargeting() ? PlayerBroom->GetGoalTargeting()->GetTargetingData() : nullptr;

	case EGymTuningTarget::Quaffle:
		if (Quidditch)
		{
			for (const AQuaffle* Quaffle : Quidditch->GetQuaffles())
			{
				if (Quaffle->GetQuaffleData()) return Quaffle->GetQuaffleData();
			}
		}
		return nullptr;

	case EGymTuningTarget::Bludger:
		if (Quidditch)
		{
			for (const ABludger* Bludger : Quidditch->GetBludgers())
			{
				if (Bludger->GetBludgerData()) return Bludger->GetBludgerData();
			}
		}
		return nullptr;

	case EGymTuningTarget::Snitch:
		// The trainer's practice Snitch first, so gym sliders never touch the match Snitch's tuning.
		if (const AGymSnitchTrainer* Trainer = GetSnitchTrainer())
		{
			if (const ASnitch* Practice = Trainer->GetSnitch(); Practice && Practice->GetSnitchData())
			{
				return Practice->GetSnitchData();
			}
		}
		if (Quidditch)
		{
			for (const ASnitch* Snitch : Quidditch->GetSnitches())
			{
				if (Snitch->GetSnitchData()) return Snitch->GetSnitchData();
			}
		}
		return nullptr;

	case EGymTuningTarget::AIBroom:
		if (Quidditch)
		{
			for (const ABroom* Broom : Quidditch->GetBrooms())
			{
				if (const AAIBroomController* AI = Cast<AAIBroomController>(Broom->GetController()); AI && AI->GetAIData())
				{
					return AI->GetAIData();
				}
			}
		}
		return nullptr;

	case EGymTuningTarget::GameState:
		return World->GetGameState();
	}

	return nullptr;
}

FNumericProperty* UGymSubsystem::FindNumericProperty(const UObject* Object, FName PropertyName)
{
	if (!Object || PropertyName.IsNone()) return nullptr;
	return CastField<FNumericProperty>(Object->GetClass()->FindPropertyByName(PropertyName));
}

double UGymSubsystem::ReadNumeric(const UObject* Object, const FNumericProperty* Property)
{
	const void* Address = Property->ContainerPtrToValuePtr<void>(Object);
	return Property->IsFloatingPoint()
		? Property->GetFloatingPointPropertyValue(Address)
		: static_cast<double>(Property->GetSignedIntPropertyValue(Address));
}

void UGymSubsystem::WriteNumeric(UObject* Object, const FNumericProperty* Property, double Value)
{
	void* Address = Property->ContainerPtrToValuePtr<void>(Object);
	if (Property->IsFloatingPoint())
	{
		Property->SetFloatingPointPropertyValue(Address, Value);
	}
	else
	{
		Property->SetIntPropertyValue(Address, static_cast<int64>(FMath::RoundToDouble(Value)));
	}
}

bool UGymSubsystem::GetTunableValue(const FGymTunable& Tunable, float& OutValue) const
{
	const UObject* Object = ResolveTarget(Tunable.Target);
	const FNumericProperty* Property = FindNumericProperty(Object, Tunable.PropertyName);
	if (!Property) return false;

	OutValue = static_cast<float>(ReadNumeric(Object, Property));
	return true;
}

bool UGymSubsystem::SetTunableValue(const FGymTunable& Tunable, float NewValue)
{
	UObject* Object = ResolveTarget(Tunable.Target);
	const FNumericProperty* Property = FindNumericProperty(Object, Tunable.PropertyName);
	if (!Property) return false;

	const float Low = FMath::Min(Tunable.Min, Tunable.Max);
	const float High = FMath::Max(Tunable.Min, Tunable.Max);

	RememberOriginal(Object, Property);
	WriteNumeric(Object, Property, FMath::Clamp(NewValue, Low, High));
	return true;
}

void UGymSubsystem::RememberOriginal(UObject* Object, const FNumericProperty* Property)
{
	const FName Name = Property->GetFName();
	const bool bAlreadyKnown = OriginalValues.ContainsByPredicate([Object, Name](const FOriginalValue& Entry)
	{
		return Entry.Object.Get() == Object && Entry.PropertyName == Name;
	});

	if (!bAlreadyKnown)
	{
		OriginalValues.Add({ Object, Name, ReadNumeric(Object, Property) });
	}
}

void UGymSubsystem::ResetTuning()
{
	for (const FOriginalValue& Entry : OriginalValues)
	{
		if (UObject* Object = Entry.Object.Get())
		{
			if (const FNumericProperty* Property = FindNumericProperty(Object, Entry.PropertyName))
			{
				WriteNumeric(Object, Property, Entry.Value);
			}
		}
	}
	OriginalValues.Reset();
}

int32 UGymSubsystem::KeepTuning()
{
	int32 NumAssets = 0;

#if WITH_EDITOR
	TSet<UObject*> Assets;
	for (const FOriginalValue& Entry : OriginalValues)
	{
		if (UObject* Object = Entry.Object.Get(); Object && Object->IsAsset())
		{
			Assets.Add(Object);
		}
	}

	for (UObject* Asset : Assets)
	{
		Asset->MarkPackageDirty();
	}
	NumAssets = Assets.Num();

	// Keep only the non-asset (per-world) entries; assets now hold the kept values.
	OriginalValues.RemoveAll([](const FOriginalValue& Entry)
	{
		const UObject* Object = Entry.Object.Get();
		return !Object || Object->IsAsset();
	});
#endif

	return NumAssets;
}

// --- Commands --------------------------------------------------------------------------------

FText UGymSubsystem::ExecuteAction(const FGymActionEntry& Entry)
{
	UWorld* World = GetWorld();
	if (!World) return FText::GetEmpty();

	const UQuidditchWorldSubsystem* Quidditch = UQuidditchWorldSubsystem::Get(this);
	const bool bAuthority = World->GetNetMode() != NM_Client;

	switch (Entry.Action)
	{
	case EGymAction::ResetTuning:
	{
		const int32 Count = GetNumChangedValues();
		ResetTuning();
		return FText::Format(LOCTEXT("ResetTuning", "Reset {0} value(s)."), Count);
	}

	case EGymAction::KeepTuning:
	{
#if WITH_EDITOR
		const int32 Count = KeepTuning();
		return FText::Format(LOCTEXT("KeepTuning", "Kept values in {0} asset(s). Save them in the editor."), Count);
#else
		return LOCTEXT("KeepTuningEditorOnly", "Keeping values only works in the editor.");
#endif
	}

	case EGymAction::ApplyAIPreset:
	{
		if (!Entry.AIPreset || !Quidditch) return FText::GetEmpty();
		int32 Count = 0;
		for (const ABroom* Broom : Quidditch->GetBrooms())
		{
			if (AAIBroomController* AI = Cast<AAIBroomController>(Broom->GetController()))
			{
				AI->SetAIDataAsset(Entry.AIPreset);
				++Count;
			}
		}
		return FText::Format(LOCTEXT("AIPreset", "{0}: applied to {1} AI broom(s)."), Entry.Label, Count);
	}

	case EGymAction::ResetQuaffles:
		if (bAuthority && Quidditch)
		{
			for (AQuaffle* Quaffle : Quidditch->GetQuaffles()) { Quaffle->ResetToSpawn(); }
		}
		return LOCTEXT("ResetQuaffles", "Quaffles reset.");

	case EGymAction::ResetBludgers:
		if (bAuthority && Quidditch)
		{
			for (ABludger* Bludger : Quidditch->GetBludgers()) { Bludger->ResetToSpawn(); }
		}
		return LOCTEXT("ResetBludgers", "Bludgers reset.");

	case EGymAction::ResetPracticeSnitch:
		if (AGymSnitchTrainer* Trainer = GetSnitchTrainer(); bAuthority && Trainer)
		{
			Trainer->RestartPractice();
		}
		return LOCTEXT("ResetSnitch", "Practice Snitch released again.");

	case EGymAction::RestartLevel:
		if (bAuthority)
		{
			UGameplayStatics::OpenLevel(World, FName(*UGameplayStatics::GetCurrentLevelName(World)));
		}
		return FText::GetEmpty();
	}

	return FText::GetEmpty();
}

// --- Stations --------------------------------------------------------------------------------

void UGymSubsystem::RegisterSnitchTrainer(AGymSnitchTrainer* Trainer)
{
	SnitchTrainers.AddUnique(Trainer);
}

void UGymSubsystem::UnregisterSnitchTrainer(AGymSnitchTrainer* Trainer)
{
	SnitchTrainers.Remove(Trainer);
}

AGymSnitchTrainer* UGymSubsystem::GetSnitchTrainer() const
{
	for (const TWeakObjectPtr<AGymSnitchTrainer>& Trainer : SnitchTrainers)
	{
		if (Trainer.IsValid()) return Trainer.Get();
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
