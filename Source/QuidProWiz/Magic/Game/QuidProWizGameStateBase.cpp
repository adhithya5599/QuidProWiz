// Fill out your copyright notice in the Description page of Project Settings.

#include "QuidProWizGameStateBase.h"
#include "QuidditchWorldSubsystem.h"
#include "GoalRing.h"
#include "Broom.h"
#include "SoundManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AQuidProWizGameStateBase::AQuidProWizGameStateBase()
{
}

void AQuidProWizGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AQuidProWizGameStateBase, MatchPhase);
	DOREPLIFETIME(AQuidProWizGameStateBase, TeamAScore);
	DOREPLIFETIME(AQuidProWizGameStateBase, TeamBScore);
	DOREPLIFETIME(AQuidProWizGameStateBase, CountdownSeconds);
	DOREPLIFETIME(AQuidProWizGameStateBase, bTeamAWon);
}

void AQuidProWizGameStateBase::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority()) return;

	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		GoalScoredHandle = Subsystem->OnGoalScored.AddUObject(this, &AQuidProWizGameStateBase::AddScore);
	}

	StartCountdown();
}

void AQuidProWizGameStateBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		Subsystem->OnGoalScored.Remove(GoalScoredHandle);
	}

	GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
	GetWorldTimerManager().ClearTimer(ReturnToMenuTimerHandle);
	Super::EndPlay(EndPlayReason);
}

// --- Phase ------------------------------------------------------------------------------------

void AQuidProWizGameStateBase::SetMatchPhase(EMatchPhase NewPhase)
{
	if (!HasAuthority()) return;

	MatchPhase = NewPhase;
	OnRep_MatchPhase();
}

void AQuidProWizGameStateBase::OnRep_MatchPhase()
{
	OnMatchStateChanged.Broadcast(GetMatchStateName());

	if (MatchPhase == EMatchPhase::MatchEnded)
	{
		OnMatchEnded.Broadcast(bTeamAWon, bTeamAWon ? TeamAScore : TeamBScore);
	}
}

void AQuidProWizGameStateBase::StartCountdown()
{
	SetMatchPhase(EMatchPhase::WaitingToStart);

	CountdownSeconds = CountdownDuration;
	OnRep_CountdownSeconds();

	GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
	GetWorldTimerManager().SetTimer(CountdownTimerHandle, this,
		&AQuidProWizGameStateBase::OnCountdownTick, 1.f, true);
}

void AQuidProWizGameStateBase::OnCountdownTick()
{
	CountdownSeconds = FMath::Max(CountdownSeconds - 1, 0);
	OnRep_CountdownSeconds();

	if (CountdownSeconds <= 0)
	{
		GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
		SetMatchPhase(EMatchPhase::InProgress);
	}
}

void AQuidProWizGameStateBase::OnRep_CountdownSeconds()
{
	OnCountdownUpdated.Broadcast(CountdownSeconds);

	// The initial value is announced silently; each tick after that beeps, and zero starts the match.
	if (CountdownSeconds == CountdownDuration) return;

	const int32 Seconds = CountdownSeconds;
	ForEachLocalPlayerBroom([Seconds](ABroom& Broom)
	{
		if (USoundManager* Sound = Broom.GetSoundManager())
		{
			if (Seconds > 0)
			{
				Sound->PlayCountdownBeep();
			}
			else
			{
				Sound->PlayMatchStart();
			}
		}
	});
}

// --- Scoring ----------------------------------------------------------------------------------

void AQuidProWizGameStateBase::AddScore(AGoalRing* GoalRing, int32 Value)
{
	if (!HasAuthority() || !GoalRing || !CanScore()) return;

	const bool bTeamA = GoalRing->GetOwningTeam() == EGoalTeam::TeamA;
	int32& TeamScore = bTeamA ? TeamAScore : TeamBScore;

	const int32 OldScore = TeamScore;
	TeamScore += Value;
	HandleScoreChanged(OldScore, TeamScore);

	if (CheckWinCondition(TeamScore))
	{
		EndMatch(bTeamA);
	}
}

void AQuidProWizGameStateBase::OnRep_TeamAScore(int32 OldScore)
{
	HandleScoreChanged(OldScore, TeamAScore);
}

void AQuidProWizGameStateBase::OnRep_TeamBScore(int32 OldScore)
{
	HandleScoreChanged(OldScore, TeamBScore);
}

void AQuidProWizGameStateBase::HandleScoreChanged(int32 OldScore, int32 NewScore)
{
	OnScoreUpdated.Broadcast(TeamAScore, TeamBScore);

	// Feedback only for real goals, not for a late joiner receiving the current score.
	if (NewScore <= OldScore || !HasActorBegunPlay()) return;

	ForEachLocalPlayerBroom([](ABroom& Broom)
	{
		Broom.TriggerGoalScoredShake();
		if (USoundManager* Sound = Broom.GetSoundManager())
		{
			Sound->PlayGoalScored();
		}
	});
}

// --- End of match -----------------------------------------------------------------------------

void AQuidProWizGameStateBase::EndMatch(bool bTeamAWins)
{
	bTeamAWon = bTeamAWins;
	SetMatchPhase(EMatchPhase::MatchEnded); // CanMove() is now false everywhere, so input stops.

	GetWorldTimerManager().ClearTimer(ReturnToMenuTimerHandle);
	GetWorldTimerManager().SetTimer(ReturnToMenuTimerHandle, this,
		&AQuidProWizGameStateBase::HandleReturnToMenu, ReturnMenuDelay, false);
}

void AQuidProWizGameStateBase::HandleReturnToMenu()
{
	SetMatchPhase(EMatchPhase::ReturnToMenu);

	UWorld* World = GetWorld();
	if (!World) return;

	if (World->GetNetMode() == NM_Standalone)
	{
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
	}
	else
	{
		// Bring connected clients along to the restarted level.
		World->ServerTravel(UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()) + TEXT("?listen"));
	}
}

// --- Helpers ----------------------------------------------------------------------------------

void AQuidProWizGameStateBase::ForEachLocalPlayerBroom(TFunctionRef<void(ABroom&)> Func) const
{
	const UWorld* World = GetWorld();
	if (!World) return;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (!PC || !PC->IsLocalController()) continue;

		if (ABroom* Broom = Cast<ABroom>(PC->GetPawn()))
		{
			Func(*Broom);
		}
	}
}
