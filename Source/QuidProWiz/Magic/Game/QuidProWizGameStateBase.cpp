// Fill out your copyright notice in the Description page of Project Settings.


#include "QuidProWizGameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AQuidProWizGameStateBase::AQuidProWizGameStateBase()
{
	CurrentState = MakeUnique<MatchStateWaitingToStart>();
}

void AQuidProWizGameStateBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	UE_LOG(LogTemp, Warning, TEXT("QuidProWizGameStateBase PostInitializeComponents called"));

	AGoalRing::OnGoalScored.AddUObject(this, &AQuidProWizGameStateBase::AddScore);
	UE_LOG(LogTemp, Warning, TEXT("OnGoalScored delegate bound: %d"),
		AGoalRing::OnGoalScored.IsBound());

	StartCountdown();
}

void AQuidProWizGameStateBase::SetMatchState(TUniquePtr<MatchStateBase> NewState)
{
	if (!NewState) return;

	CurrentState = MoveTemp(NewState);
	UE_LOG(LogTemp, Log, TEXT("Match state changed to: %s"), *CurrentState->GetStateName());

	OnMatchStateChanged.Broadcast(CurrentState->GetStateName());
}

void AQuidProWizGameStateBase::StartCountdown()
{
	SetMatchState(MakeUnique<MatchStateWaitingToStart>());

	CountdownSeconds = CountdownDuration;
	OnCountdownUpdated.Broadcast(CountdownSeconds);
	
	UE_LOG(LogTemp, Log, TEXT("Countdown started: %d seconds"), CountdownSeconds);

	GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
	GetWorldTimerManager().SetTimer(CountdownTimerHandle, this,
		&AQuidProWizGameStateBase::OnCountdownTick, 1.f, true);
}

void AQuidProWizGameStateBase::OnCountdownTick()
{
	CountdownSeconds--;
	OnCountdownUpdated.Broadcast(CountdownSeconds);

	if (CountdownSeconds <= 0)
	{
		GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
		StartMatch();
	}
}

void AQuidProWizGameStateBase::StartMatch()
{
	SetMatchState(MakeUnique<MatchStateInProgress>());
}

void AQuidProWizGameStateBase::AddScore(AGoalRing* GoalRing, int32 Value)
{
	UE_LOG(LogTemp, Warning, TEXT("AddScore called | Value: %d | TeamA: %d | TeamB: %d"),
		Value, TeamAScore, TeamBScore);

	if (!GoalRing) return;

	// Determine which team scored based on the goal ring's team
	if(GoalRing->GetOwningTeam() == EGoalTeam::TeamA)
	{
		TeamAScore += Value;
		OnScoreUpdated.Broadcast(TeamAScore, TeamBScore);
		
		if (CheckWinCondition(TeamAScore))
		{
			EndMatch(true);
		}
	}
	else
	{
		TeamBScore += Value;
		OnScoreUpdated.Broadcast(TeamAScore, TeamBScore);

		if (CheckWinCondition(TeamBScore))
		{
			EndMatch(false);
		}
	}
}

bool AQuidProWizGameStateBase::CheckWinCondition(int32 Score) const
{
	return Score >= ScoreToWin;
}

void AQuidProWizGameStateBase::EndMatch(bool bTeamAWon)
{
	SetMatchState(MakeUnique<MatchStateMatchEnded>());

	const int32 WinningScore = bTeamAWon ? TeamAScore : TeamBScore;
	OnMatchEnded.Broadcast(bTeamAWon, WinningScore);

	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(true);
	}

	GetWorldTimerManager().ClearTimer(ReturnToMenuTimerHandle);
	GetWorldTimerManager().SetTimer(ReturnToMenuTimerHandle, this,
		&AQuidProWizGameStateBase::HandleReturnToMenu, ReturnMenuDelay, false);
}

void AQuidProWizGameStateBase::HandleReturnToMenu()
{
	SetMatchState(MakeUnique<MatchStateReturnToMenu>());

	UGameplayStatics::OpenLevel(GetWorld(),FName(*GetWorld()->GetName()));
}