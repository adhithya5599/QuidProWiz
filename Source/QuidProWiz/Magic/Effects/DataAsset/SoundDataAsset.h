// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoundDataAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class QUIDPROWIZ_API USoundDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | Broom")
	USoundBase* BroomFlyingLoop = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | Broom")
	USoundBase* BroomSpeedBoost = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | Broom")
	USoundBase* BroomHitByBludger = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | Quaffle")
	USoundBase* QuafflePickup = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | Quaffle")
	USoundBase* QuaffleThrow = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | Quaffle")
	USoundBase* QuaffleScore = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | Bludger")
	USoundBase* BludgerChase = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | Bludger")
	USoundBase* BludgerHit = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | UI")
	USoundBase* CountDownBeep = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | UI")
	USoundBase* MatchStart = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | UI")
	USoundBase* GoalScored = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | UI")
	USoundBase* MatchWon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | UI")
	USoundBase* MatchLost = nullptr;
};
