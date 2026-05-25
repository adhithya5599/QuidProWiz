// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AIBroomDataAssetBase.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class QUIDPROWIZ_API UAIBroomDataAssetBase : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI | Movement")
	float FlySpeed = 800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI | Movement")
	float TurnInterpSpeed = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI | Bludger")
	float BludgerEvadeDistance = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI | Bludger")
	float BludgerEvadeStrength = 1200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI | Performance")
	float BlackboardUpdateInterval = 0.1f;
};
