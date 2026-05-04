// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GoalTargetingDataAsset.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class QUIDPROWIZ_API UGoalTargetingDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	float TargetInRange = 2000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	float MaxLockAngleDegrees = 45.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	float RetargetInterval = 0.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	float DistanceWeight = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	float AngleWeight = 800.f;
	
};
