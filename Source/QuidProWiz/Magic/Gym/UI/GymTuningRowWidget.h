// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/GymDataAsset.h"
#include "GymTuningRowWidget.generated.h"

class UTextBlock;
class USlider;

/**
 * One slider row on the gym panel. Reads its starting value from the live object and writes
 * every change straight back through UGymSubsystem, so the effect is immediate.
 * The Blueprint subclass (WBP_GymTuningRow) only provides layout and style.
 */
UCLASS(Abstract)
class QUIDPROWIZ_API UGymTuningRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void Setup(const FGymTunable& InTunable);

	/** Re-read the live value (e.g. after Reset). */
	void RefreshFromTarget();

	const FGymTunable& GetTunable() const { return Tunable; }

protected:

	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> ValueSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ValueText;

	/** Colour of the value text when the target can't be found in this level. */
	UPROPERTY(EditDefaultsOnly, Category = "Gym")
	FSlateColor UnavailableColor = FSlateColor(FLinearColor(0.6f, 0.3f, 0.3f));

private:

	FGymTunable Tunable;
	bool bUpdatingFromCode = false;

	UFUNCTION()
	void HandleSliderChanged(float NewValue);

	void ShowValue(float Value);
};
