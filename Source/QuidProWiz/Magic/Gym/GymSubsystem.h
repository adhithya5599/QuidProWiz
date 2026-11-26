// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/GymDataAsset.h"
#include "GymSubsystem.generated.h"

class AGymSnitchTrainer;
class AGymFlightCourse;
class FNumericProperty;

/**
 * Gym-only services: live tuning of data assets from the gym panel, the panel's button commands,
 * and a registry of practice stations (Snitch trainers).
 *
 * Tuning writes straight into the live objects through reflection, so every system that reads its
 * data asset each frame reacts immediately. Data assets are shared with the editor, so the first
 * time a value changes its original is remembered and put back when the world ends (or on
 * ResetTuning) - play-testing never silently edits an asset. KeepTuning (editor only) keeps the
 * values and marks the assets dirty so they can be saved.
 *
 * Tuning is local to the machine it runs on; the gym is meant to be played standalone.
 */
UCLASS()
class QUIDPROWIZ_API UGymSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	static UGymSubsystem* Get(const UObject* WorldContextObject);

	virtual void Deinitialize() override;

	// --- Tuning ------------------------------------------------------------------------------

	/** The live object a tunable edits, or nullptr if it isn't in this world. */
	UObject* ResolveTarget(EGymTuningTarget Target) const;

	/** Current value of a tunable; false if its target or property can't be found. */
	bool GetTunableValue(const FGymTunable& Tunable, float& OutValue) const;

	/** Writes a tunable (clamped to its range); false if its target or property can't be found. */
	bool SetTunableValue(const FGymTunable& Tunable, float NewValue);

	/** Puts every changed value back. */
	void ResetTuning();

	/** Editor only: keeps the changed values and marks their packages dirty. Returns how many assets. */
	int32 KeepTuning();

	int32 GetNumChangedValues() const { return OriginalValues.Num(); }

	// --- Commands ----------------------------------------------------------------------------

	/** Runs a panel button. Returns a short message for the UI (may be empty). */
	FText ExecuteAction(const FGymActionEntry& Entry);

	// --- Stations ----------------------------------------------------------------------------

	void RegisterSnitchTrainer(AGymSnitchTrainer* Trainer);
	void UnregisterSnitchTrainer(AGymSnitchTrainer* Trainer);
	AGymSnitchTrainer* GetSnitchTrainer() const;

	void RegisterFlightCourse(AGymFlightCourse* Course);
	void UnregisterFlightCourse(AGymFlightCourse* Course);
	AGymFlightCourse* GetFlightCourse() const;

protected:

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:

	struct FOriginalValue
	{
		TWeakObjectPtr<UObject> Object;
		FName PropertyName;
		double Value = 0.0;
	};

	TArray<FOriginalValue> OriginalValues;
	TArray<TWeakObjectPtr<AGymSnitchTrainer>> SnitchTrainers;
	TArray<TWeakObjectPtr<AGymFlightCourse>> FlightCourses;

	static FNumericProperty* FindNumericProperty(const UObject* Object, FName PropertyName);
	static double ReadNumeric(const UObject* Object, const FNumericProperty* Property);
	static void WriteNumeric(UObject* Object, const FNumericProperty* Property, double Value);

	void RememberOriginal(UObject* Object, const FNumericProperty* Property);
};
