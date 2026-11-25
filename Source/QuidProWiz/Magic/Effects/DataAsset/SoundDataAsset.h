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
	TObjectPtr<USoundBase> BroomFlyingLoop = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | Broom")
	TObjectPtr<USoundBase> BroomSpeedBoost = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | Broom")
	TObjectPtr<USoundBase> BroomHitByBludger = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | Quaffle")
	TObjectPtr<USoundBase> QuafflePickup = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | Quaffle")
	TObjectPtr<USoundBase> QuaffleThrow = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | Quaffle")
	TObjectPtr<USoundBase> QuaffleScore = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | Bludger")
	TObjectPtr<USoundBase> BludgerChase = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | Bludger")
	TObjectPtr<USoundBase> BludgerHit = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | UI")
	TObjectPtr<USoundBase> CountDownBeep = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | UI")
	TObjectPtr<USoundBase> MatchStart = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | UI")
	TObjectPtr<USoundBase> GoalScored = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | UI")
	TObjectPtr<USoundBase> MatchWon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds | UI")
	TObjectPtr<USoundBase> MatchLost = nullptr;
};
