// Fill out your copyright notice in the Description page of Project Settings.


#include "QuidProWizGameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Broom.h"

AQuidProWizGameStateBase::AQuidProWizGameStateBase()
{
	CurrentState = MakeUnique<MatchStateWaitingToStart>();
}

void AQuidProWizGameStateBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	AGoalRing::OnGoalScored.AddUObject(this, &AQuidProWizGameStateBase::AddScore);

	StartCountdown();
}

void AQuidProWizGameStateBase::SetMatchState(TUniquePtr<MatchStateBase> NewState)
{
	if (!NewState) return;

	CurrentState = MoveTemp(NewState);

	OnMatchStateChanged.Broadcast(CurrentState->GetStateName());
}

void AQuidProWizGameStateBase::StartCountdown()
{
	SetMatchState(MakeUnique<MatchStateWaitingToStart>());

	CountdownSeconds = CountdownDuration;
	OnCountdownUpdated.Broadcast(CountdownSeconds);
	
	GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
	GetWorldTimerManager().SetTimer(CountdownTimerHandle, this,
		&AQuidProWizGameStateBase::OnCountdownTick, 1.f, true);
}

void AQuidProWizGameStateBase::OnCountdownTick()
{
	CountdownSeconds--;
	OnCountdownUpdated.Broadcast(CountdownSeconds);

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || !PC->IsLocalController()) continue;

		ABroom* Broom = Cast<ABroom>(PC->GetPawn());
		if (!Broom) continue;

		if (USoundManager* SM = Broom->GetSoundManager())
		{
			if (CountdownSeconds > 0)
			{
				SM->PlayCountdownBeep();
			}
			else
			{
				SM->PlayMatchStart();
			}
		}
	}

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
	if (!GoalRing) return;

	// Determine which team scored based on the goal ring's team
	TriggerGoalScoredShakeForAllPlayers();

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

void AQuidProWizGameStateBase::TriggerGoalScoredShakeForAllPlayers()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || !PC->IsLocalController()) continue;

		APawn* Pawn = PC->GetPawn();
		if (!Pawn) continue;

		ABroom* Broom = Cast<ABroom>(Pawn);
		if (!Broom) continue;

		Broom->TriggerGoalScoredShake();

		if (USoundManager* SM = Broom->GetSoundManager())
		{
			SM->PlayGoalScored();
		}
	}
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