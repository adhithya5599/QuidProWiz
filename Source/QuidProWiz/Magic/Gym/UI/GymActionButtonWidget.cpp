// Fill out your copyright notice in the Description page of Project Settings.

#include "GymActionButtonWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UGymActionButtonWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (ActionButton)
	{
		// The button is set not focusable in WBP_GymButton, so flying keeps keyboard focus after a click.
		ActionButton->OnClicked.AddDynamic(this, &UGymActionButtonWidget::HandleClicked);
	}
}

void UGymActionButtonWidget::Setup(const FGymActionEntry& InEntry)
{
	Entry = InEntry;

	if (ActionLabel)
	{
		ActionLabel->SetText(Entry.Label);
	}
}

void UGymActionButtonWidget::HandleClicked()
{
	OnActionClicked.ExecuteIfBound(Entry);
}
