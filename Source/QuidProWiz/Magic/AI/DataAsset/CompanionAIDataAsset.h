// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AIBroomDataAssetBase.h"
#include "CompanionAIDataAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class QUIDPROWIZ_API UCompanionAIDataAsset : public UAIBroomDataAssetBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion | Follow")
	float FollowDistance = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion | Follow")
	float FollowHeight = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion | Defend")
	float BludgerDefendDistance = 1200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Comapanion | Defend")
	float InterceptOffset = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion | Assist")
	float GoalAssistRange = 2000.f;
};
