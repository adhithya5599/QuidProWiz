// Fill out your copyright notice in the Description page of Project Settings.

#include "GymTuningRowWidget.h"
#include "GymSubsystem.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "Gym"

void UGymTuningRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (ValueSlider)
	{
		ValueSlider->OnValueChanged.AddDynamic(this, &UGymTuningRowWidget::HandleSliderChanged);
	}
}

void UGymTuningRowWidget::Setup(const FGymTunable& InTunable)
{
	Tunable = InTunable;

	if (LabelText)
	{
		LabelText->SetText(Tunable.Label);
	}

	if (ValueSlider)
	{
		bUpdatingFromCode = true;
		ValueSlider->SetMinValue(FMath::Min(Tunable.Min, Tunable.Max));
		ValueSlider->SetMaxValue(FMath::Max(Tunable.Min, Tunable.Max));
		ValueSlider->SetStepSize(Tunable.Step > 0.f ? Tunable.Step : 0.f);
		bUpdatingFromCode = false;
	}

	RefreshFromTarget();
}

void UGymTuningRowWidget::RefreshFromTarget()
{
	const UGymSubsystem* Gym = UGymSubsystem::Get(this);
	float Value = 0.f;
	const bool bFound = Gym && Gym->GetTunableValue(Tunable, Value);

	if (ValueSlider)
	{
		bUpdatingFromCode = true;
		ValueSlider->SetValue(Value);
		ValueSlider->SetIsEnabled(bFound);
		bUpdatingFromCode = false;
	}

	if (!bFound)
	{
		if (ValueText)
		{
			ValueText->SetText(LOCTEXT("NotInLevel", "n/a"));
			ValueText->SetColorAndOpacity(UnavailableColor);
		}
		return;
	}

	ShowValue(Value);
}

void UGymTuningRowWidget::HandleSliderChanged(float NewValue)
{
	if (bUpdatingFromCode) return;

	if (Tunable.Step > 0.f)
	{
		NewValue = FMath::GridSnap(NewValue, Tunable.Step);
	}

	if (UGymSubsystem* Gym = UGymSubsystem::Get(this); Gym && Gym->SetTunableValue(Tunable, NewValue))
	{
		float Applied = NewValue;
		Gym->GetTunableValue(Tunable, Applied);
		ShowValue(Applied);
	}
}

void UGymTuningRowWidget::ShowValue(float Value)
{
	if (!ValueText) return;

	FNumberFormattingOptions Format;
	Format.MinimumFractionalDigits = Tunable.Decimals;
	Format.MaximumFractionalDigits = Tunable.Decimals;

	const FText Number = FText::AsNumber(Value, &Format);
	ValueText->SetText(Tunable.Units.IsEmpty()
		? Number
		: FText::Format(LOCTEXT("ValueWithUnits", "{0} {1}"), Number, Tunable.Units));
}

#undef LOCTEXT_NAMESPACE
