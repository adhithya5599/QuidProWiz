// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZoneType.h"
#include "GymDataAsset.generated.h"

class UBroomAIDataAsset;

/** Which live object a gym slider edits. Resolved by UGymSubsystem::ResolveTarget. */
UENUM(BlueprintType)
enum class EGymTuningTarget : uint8
{
	/** The local player's UBroomDataAsset. */
	PlayerBroom,
	/** The local player's UGoalTargetingDataAsset (aim assist). */
	PlayerGoalTargeting,
	/** The first Quaffle's UQuaffleDataAsset. */
	Quaffle,
	/** The first Bludger's UBludgerDataAsset. */
	Bludger,
	/** The practice Snitch's USnitchDataAsset (falls back to any Snitch with a data asset). */
	Snitch,
	/** The first AI broom controller's UBroomAIDataAsset. */
	AIBroom,
	/** The live GameState (match rules such as ScoreToWin). */
	GameState,
};

/** One slider: a numeric property on a target object, edited live through reflection. */
USTRUCT(BlueprintType)
struct FGymTunable
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tunable")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tunable")
	EGymTuningTarget Target = EGymTuningTarget::PlayerBroom;

	/** Exact C++ name of a float, double or integer property on the target, e.g. "Speed". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tunable")
	FName PropertyName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tunable")
	float Min = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tunable")
	float Max = 1.f;

	/** Slider step; 0 = continuous. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tunable", meta = (ClampMin = "0"))
	float Step = 0.f;

	/** Digits shown after the decimal point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tunable", meta = (ClampMin = "0", ClampMax = "4"))
	int32 Decimals = 0;

	/** Suffix shown after the value, e.g. "cm/s". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tunable")
	FText Units;
};

/** What a gym panel button does. Executed by UGymSubsystem::ExecuteAction (Command pattern). */
UENUM(BlueprintType)
enum class EGymAction : uint8
{
	/** Put every value changed in this session back to what it was. */
	ResetTuning,
	/** Editor only: keep the changed values and mark their data assets dirty so they can be saved. */
	KeepTuning,
	/** Swap every AI broom to the given AI data asset. */
	ApplyAIPreset,
	/** Send every Quaffle back to its spawn point. */
	ResetQuaffles,
	/** Send every Bludger back to its spawn point. */
	ResetBludgers,
	/** Catch-and-release the practice Snitch from its trainer again. */
	ResetPracticeSnitch,
	/** Abandon the current flight-course lap. */
	ResetFlightCourse,
	/** Reload the current level (restarts the match). */
	RestartLevel,
};

/** A button on a zone's panel. */
USTRUCT(BlueprintType)
struct FGymActionEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	EGymAction Action = EGymAction::ResetTuning;

	/** Only for ApplyAIPreset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action", meta = (EditCondition = "Action == EGymAction::ApplyAIPreset", EditConditionHides))
	TObjectPtr<UBroomAIDataAsset> AIPreset;
};

/** Everything the gym panel shows for one zone. */
USTRUCT(BlueprintType)
struct FGymZoneInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	EZoneType Zone = EZoneType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone", meta = (MultiLine = "true"))
	FText Description;

	/** Colour used for the panel accent and the zone's signage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	FLinearColor AccentColor = FLinearColor(0.2f, 0.5f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone", meta = (TitleProperty = "Label"))
	TArray<FGymTunable> Tunables;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone", meta = (TitleProperty = "Label"))
	TArray<FGymActionEntry> Actions;
};

/**
 * The whole gym in one asset: each zone's title, help text, sliders and buttons.
 * Adding a slider is a data change (name the property and its range); no Blueprint or C++ edit.
 */
UCLASS(BlueprintType)
class QUIDPROWIZ_API UGymDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gym", meta = (TitleProperty = "Title"))
	TArray<FGymZoneInfo> Zones;

	/** Info for Zone, or nullptr if the asset has no entry for it. */
	const FGymZoneInfo* FindZone(EZoneType Zone) const
	{
		return Zones.FindByPredicate([Zone](const FGymZoneInfo& Info) { return Info.Zone == Zone; });
	}
};
