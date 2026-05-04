// Fill out your copyright notice in the Description page of Project Settings.


#include "QuidProWizGameStateBase.h"
#include "../Goal/GoalRing.h"

AQuidProWizGameStateBase::AQuidProWizGameStateBase()
{
}

void AQuidProWizGameStateBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	UE_LOG(LogTemp, Warning, TEXT("QuidProWizGameStateBase PostInitializeComponents called"));

	AGoalRing::OnGoalScored.AddUObject(this, &AQuidProWizGameStateBase::AddScore);
	UE_LOG(LogTemp, Warning, TEXT("OnGoalScored delegate bound: %d"),
		AGoalRing::OnGoalScored.IsBound());
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
	}
	else
	{
		TeamBScore += Value;
	}

	UE_LOG(LogTemp, Warning, TEXT("Score after update | TeamA: %d | TeamB: %d"),
		TeamAScore, TeamBScore);
	OnScoreUpdated.Broadcast(TeamAScore, TeamBScore);
}