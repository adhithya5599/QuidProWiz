// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Match/MatchState.h"
#include "QuidditchTeam.h"
#include "QuidProWizGameStateBase.generated.h"

class AGoalRing;
class ABroom;
class ASnitch;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScoreUpdated, int32, TeamAScore, int32, TeamBScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchStateChanged, FString, NewStateName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchEnded, bool, bTeamWon, int32, WinningScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCountdownUpdated, int32, SecondsRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSnitchReleased);

/**
 * Owns the match rules and flow. The server runs the countdown, scoring, Snitch release and the
 * win decision; the phase, scores and countdown replicate, and every machine raises the UI
 * delegates and local feedback (sounds, camera shake) from the OnRep handlers.
 *
 * Win rule: catching the Snitch ends the match and the catcher's team wins regardless of score.
 * The Snitch is released when a team reaches SnitchReleaseScore or at a random time, whichever
 * comes first. ScoreToWin is an optional extra rule (off by default).
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

	UPROPERTY(BlueprintAssignable, Category = "Match|Snitch")
	FOnSnitchReleased OnSnitchReleased;

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

	UFUNCTION(BlueprintPure, Category = "Match|Snitch")
	bool IsSnitchReleased() const { return bSnitchReleased; }

	UFUNCTION(BlueprintPure, Category = "Match|Snitch")
	bool WasEndedBySnitch() const { return bEndedBySnitch; }

	/** Winner once the match has ended. */
	UFUNCTION(BlueprintPure, Category = "Match")
	EGoalTeam GetWinningTeam() const { return bTeamAWon ? EGoalTeam::TeamA : EGoalTeam::TeamB; }

	/** Server: credit a goal to the team attacking GoalRing. Ignored outside the InProgress phase. */
	void AddScore(AGoalRing* GoalRing, int32 Value);

	/** Server: release the Snitch now (if it hasn't been already). */
	void ReleaseSnitch();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Optional: end the match when a team reaches this score. 0 = off (only the Snitch ends a match). */
	UPROPERTY(EditDefaultsOnly, Category = "Match", meta = (ClampMin = "0"))
	int32 ScoreToWin = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 CountdownDuration = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	float ReturnMenuDelay = 5.f;

	/** Snitch spawned at the arena centre when no Snitch is placed in the level. */
	UPROPERTY(EditDefaultsOnly, Category = "Match|Snitch")
	TSubclassOf<ASnitch> SnitchClass;

	/** Release the Snitch when either team reaches this score. 0 = never by score. */
	UPROPERTY(EditDefaultsOnly, Category = "Match|Snitch", meta = (ClampMin = "0"))
	int32 SnitchReleaseScore = 100;

	/** Release the Snitch at a random time between these (seconds after the match starts). Max 0 = never by time. */
	UPROPERTY(EditDefaultsOnly, Category = "Match|Snitch", meta = (ClampMin = "0"))
	float SnitchReleaseTimeMin = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Match|Snitch", meta = (ClampMin = "0"))
	float SnitchReleaseTimeMax = 180.f;

	/** Height above the arena centre at which a spawned Snitch appears. */
	UPROPERTY(EditDefaultsOnly, Category = "Match|Snitch")
	float SnitchSpawnHeight = 600.f;

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

	UPROPERTY(ReplicatedUsing = OnRep_SnitchReleased)
	bool bSnitchReleased = false;

	UPROPERTY(Replicated)
	bool bEndedBySnitch = false;

	UFUNCTION()
	void OnRep_SnitchReleased();

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
	bool CheckWinCondition(int32 Score) const { return ScoreToWin > 0 && Score >= ScoreToWin; }

	void ScheduleSnitchRelease();
	void HandleSnitchCaught(ASnitch* Snitch, ABroom* Catcher);

	void HandleScoreChanged(int32 OldScore, int32 NewScore);

	/** Runs Func for each broom controlled by a local player on this machine. */
	void ForEachLocalPlayerBroom(TFunctionRef<void(ABroom&)> Func) const;

	FDelegateHandle GoalScoredHandle;
	FDelegateHandle SnitchCaughtHandle;
	FTimerHandle SnitchReleaseTimerHandle;
	FTimerHandle CountdownTimerHandle;
	FTimerHandle ReturnToMenuTimerHandle;
};
