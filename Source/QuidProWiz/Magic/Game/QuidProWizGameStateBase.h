// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Match/MatchState.h"
#include "QuidProWizGameStateBase.generated.h"

class AGoalRing;
class ABroom;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScoreUpdated, int32, TeamAScore, int32, TeamBScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchStateChanged, FString, NewStateName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchEnded, bool, bTeamWon, int32, WinningScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCountdownUpdated, int32, SecondsRemaining);

/**
 * Owns the match rules and flow. The server runs the countdown, scoring and win check; the
 * phase, scores and countdown replicate, and every machine raises the UI delegates and local
 * feedback (sounds, camera shake) from the OnRep handlers.
 */
UCLASS()
class QUIDPROWIZ_API AQuidProWizGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:

	AQuidProWizGameStateBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- UI delegates (raised on every machine) -----------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Match")
	FOnScoreUpdated OnScoreUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Match")
	FOnMatchStateChanged OnMatchStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Match")
	FOnMatchEnded OnMatchEnded;

	UPROPERTY(BlueprintAssignable, Category = "Match")
	FOnCountdownUpdated OnCountdownUpdated;

	// --- Queries ------------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Match")
	int32 GetTeamAScore() const { return TeamAScore; }

	UFUNCTION(BlueprintCallable, Category = "Match")
	int32 GetTeamBScore() const { return TeamBScore; }

	UFUNCTION(BlueprintCallable, Category = "Match")
	FString GetMatchStateName() const { return GetMatchState().GetStateName(); }

	UFUNCTION(BlueprintPure, Category = "Match")
	EMatchPhase GetMatchPhase() const { return MatchPhase; }

	UFUNCTION(BlueprintCallable, Category = "Match")
	bool CanScore() const { return GetMatchState().CanScore(); }

	UFUNCTION(BlueprintCallable, Category = "Match")
	bool CanMove() const { return GetMatchState().CanMove(); }

	UFUNCTION(BlueprintCallable, Category = "Match")
	bool IsMatchEnded() const { return GetMatchState().IsMatchEnded(); }

	UFUNCTION(BlueprintCallable, Category = "Match")
	int32 GetCountdownSeconds() const { return CountdownSeconds; }

	UFUNCTION(BlueprintCallable, Category = "Match")
	int32 GetScoreToWin() const { return ScoreToWin; }

	/** Server: credit a goal. Ignored outside the InProgress phase. */
	void AddScore(AGoalRing* GoalRing, int32 Value);

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 ScoreToWin = 150;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 CountdownDuration = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	float ReturnMenuDelay = 5.f;

private:

	UPROPERTY(ReplicatedUsing = OnRep_MatchPhase)
	EMatchPhase MatchPhase = EMatchPhase::WaitingToStart;

	UPROPERTY(ReplicatedUsing = OnRep_TeamAScore)
	int32 TeamAScore = 0;

	UPROPERTY(ReplicatedUsing = OnRep_TeamBScore)
	int32 TeamBScore = 0;

	UPROPERTY(ReplicatedUsing = OnRep_CountdownSeconds)
	int32 CountdownSeconds = 0;

	UPROPERTY(Replicated)
	bool bTeamAWon = false;

	UFUNCTION()
	void OnRep_MatchPhase();

	UFUNCTION()
	void OnRep_TeamAScore(int32 OldScore);

	UFUNCTION()
	void OnRep_TeamBScore(int32 OldScore);

	UFUNCTION()
	void OnRep_CountdownSeconds();

	const FMatchStateBase& GetMatchState() const { return FMatchStateBase::Get(MatchPhase); }

	/** Server: switch phase and run the same handler clients run in OnRep. */
	void SetMatchPhase(EMatchPhase NewPhase);

	void StartCountdown();
	void OnCountdownTick();
	void EndMatch(bool bTeamAWins);
	void HandleReturnToMenu();
	bool CheckWinCondition(int32 Score) const { return Score >= ScoreToWin; }

	void HandleScoreChanged(int32 OldScore, int32 NewScore);

	/** Runs Func for each broom controlled by a local player on this machine. */
	void ForEachLocalPlayerBroom(TFunctionRef<void(ABroom&)> Func) const;

	FDelegateHandle GoalScoredHandle;
	FTimerHandle CountdownTimerHandle;
	FTimerHandle ReturnToMenuTimerHandle;
};
