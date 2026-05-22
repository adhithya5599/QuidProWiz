// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GoalRingDataAsset.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class QUIDPROWIZ_API UGoalRingDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal | Gameplay")
	int32 ScoreValue = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal | Gameplay")
	float LockOnVolumeExtent = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal | Gameplay")
	float ScoreVolumeExtent = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal | Visual")
	UMaterialInterface* NormalMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal | Visual")
	UMaterialInterface* HighlightedMaterial = nullptr;	

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Goal | Visual")
	UMaterialInterface* ScoreFlashMaterial = nullptr;
};
