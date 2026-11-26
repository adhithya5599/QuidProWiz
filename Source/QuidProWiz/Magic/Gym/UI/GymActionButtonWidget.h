// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/GymDataAsset.h"
#include "GymActionButtonWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DELEGATE_OneParam(FOnGymActionClicked, const FGymActionEntry& /*Entry*/);

/** A gym panel button that carries one command (FGymActionEntry). Layout lives in WBP_GymButton. */
UCLASS(Abstract)
class QUIDPROWIZ_API UGymActionButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void Setup(const FGymActionEntry& InEntry);

	FOnGymActionClicked OnActionClicked;

protected:

	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ActionButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ActionLabel;

private:

	FGymActionEntry Entry;

	UFUNCTION()
	void HandleClicked();
};
