// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BroomDataAsset.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class QUIDPROWIZ_API UBroomDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement")
	float Speed = 1000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement")
	float StrafeSpeed = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement")
	float TurnSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement")
	float BankAngle = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement")
	float BankInterpSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement")
	float AscendTiltAngle = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement")
	float HeadingInterpSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement")
	float SpeedBoostMultiplier = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement")
	float SpeedBoostAccelerationMultiplier = 4.f;
};
