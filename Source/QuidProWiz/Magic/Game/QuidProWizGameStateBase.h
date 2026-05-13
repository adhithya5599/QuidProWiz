// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Match/MatchState.h"
#include "../Goal/GoalRing.h"
#include "QuidProWizGameStateBase.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScoreUpdated, int32, TeamAScore, int32, TeamBScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchStateChanged, FString, NewStateName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchEnded, bool, bTeamWon, int32, WinningScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCountdownUpdated, int32, SecondsRemaining);

class AGoalRing;

UCLASS()
class QUIDPROWIZ_API AQuidProWizGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
	
public:

	AQuidProWizGameStateBase();
	
	virtual void PostInitializeComponents() override;

	//Delegates
	UPROPERTY(BlueprintAssignable, Category = "Match")
	FOnScoreUpdated OnScoreUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Match")
	FOnMatchStateChanged OnMatchStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Match")
	FOnMatchEnded OnMatchEnded;

	UPROPERTY(BlueprintAssignable, Category = "Match")
	FOnCountdownUpdated OnCountdownUpdated;

	//Getters
	UFUNCTION(BlueprintCallable, Category = "Match")
	int32 GetTeamAScore() const { return TeamAScore; }

	UFUNCTION(BlueprintCallable, Category = "Match")
	int32 GetTeamBScore() const { return TeamBScore; }

	UFUNCTION(BlueprintCallable, Category = "Match")
	FString GetMatchStateName() const { return CurrentState->GetStateName(); }

	UFUNCTION(BlueprintCallable, Category = "Match")
	bool CanScore() const { return CurrentState->CanScore(); }

	UFUNCTION(BlueprintCallable, Category = "Match")
	bool CanMove() const { return CurrentState->CanMove(); }

	UFUNCTION(BlueprintCallable, Category = "Match")
	bool IsMatchEnded() const { return CurrentState->IsMatchEnded(); }

	UFUNCTION(BlueprintCallable, Category = "Match")
	int32 GetCountdownSeconds() const { return CountdownSeconds; }

	UFUNCTION(BlueprintCallable, Category = "Match")
	int32 GetScoreToWin() const { return ScoreToWin; }

	void AddScore(AGoalRing* GoalRing, int32 Value);

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 ScoreToWin = 150;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 CountdownDuration = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	float ReturnMenuDelay = 5.f;

private:

	TUniquePtr<MatchStateBase> CurrentState;

	void SetMatchState(TUniquePtr<MatchStateBase> NewState);

	int32 TeamAScore = 0;
	int32 TeamBScore = 0;
	int32 CountdownSeconds = 0;

	FTimerHandle CountdownTimerHandle;
	FTimerHandle ReturnToMenuTimerHandle;

	void StartCountdown();
	void OnCountdownTick();
	void StartMatch();
	void EndMatch(bool bTeamAWon);
	void HandleReturnToMenu();

	bool CheckWinCondition(int32 Score) const;
};
