// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BroomAIDataAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class QUIDPROWIZ_API UBroomAIDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI | Movement")
	float FlySpeed = 800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI | Movement")
	float TurnInterpSpeed = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI | Movement")
	float PickupApproachRadius = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI | Attack")
	float ThrowRange = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI | Attack")
	float ThrowAccuracy = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI | Attack")
	float HighValueGoalBias = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI | Defense")
	float BludgerEvadeDistance = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI | Defense")
	float BludgerEvadeStrength = 1200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI | Defense")
	float BlackboardUpdateInterval = 0.1f;
};
