// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AIBroomDataAssetBase.h"
#include "OpponentAIDataAsset.generated.h"

/**
 *
 */
UCLASS(BlueprintType)
class QUIDPROWIZ_API UOpponentAIDataAsset : public UAIBroomDataAssetBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI | Attack")
	float ThrowRange = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI | Attack")
	float ThrowAccuracy = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI | Attack")
	float HighValueGoalBias = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI | Pickup")
	float PickupApproachRadius = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI | Target")
	float HumanPlayerPriority = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI | Target")
	float GoalProximityWeight = 1.5f;
};
