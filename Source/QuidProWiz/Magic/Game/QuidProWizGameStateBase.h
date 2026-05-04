// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "QuidProWizGameStateBase.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScoreUpdated, int32, TeamAScore, int32, TeamBScore);

class AGoalRing;

UCLASS()
class QUIDPROWIZ_API AQuidProWizGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
	
public:

	AQuidProWizGameStateBase();
	
	virtual void PostInitializeComponents() override;

	UPROPERTY(BlueprintAssignable, Category = "Score")
	FOnScoreUpdated OnScoreUpdated;

	UFUNCTION(BlueprintCallable, Category = "Score")
	int32 GetTeamAScore() const { return TeamAScore; }

	UFUNCTION(BlueprintCallable, Category = "Score")
	int32 GetTeamBScore() const { return TeamBScore; }

	void AddScore(AGoalRing* GoalRing, int32 Value);

private:

	int32 TeamAScore = 0;
	int32 TeamBScore = 0;
};
