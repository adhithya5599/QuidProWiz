// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "QuidditchTestWorld.h"
#include "QuidditchWorldSubsystem.h"
#include "QuidProWizGameStateBase.h"
#include "Broom.h"
#include "GoalRing.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Drives the match rules through the GameState: countdown, when scoring and flying are
 * allowed, which team is credited, and the win condition.
 */
BEGIN_DEFINE_SPEC(FQuidditchMatchFlowSpec, "QuidProWiz.MatchFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TUniquePtr<FQuidditchTestWorld> TestWorld;
	AQuidProWizGameStateBase* GameState = nullptr;
	AGoalRing* TeamAGoal = nullptr;
	AGoalRing* TeamBGoal = nullptr;

	static constexpr int32 CountdownSeconds = 2;
	static constexpr int32 ScoreToWin = 30;

	void ScoreOn(AGoalRing* Goal)
	{
		TestWorld->GetSubsystem()->OnGoalScored.Broadcast(Goal, Goal->GetScoreValue());
	}

	void FinishCountdown()
	{
		TestWorld->Tick(CountdownSeconds + 0.1f);
	}
END_DEFINE_SPEC(FQuidditchMatchFlowSpec)

void FQuidditchMatchFlowSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld = MakeUnique<FQuidditchTestWorld>();
		TeamAGoal = TestWorld->SpawnGoal(FVector(5000.f, 0.f, 0.f), FRotator::ZeroRotator, EGoalTeam::TeamA);
		TeamBGoal = TestWorld->SpawnGoal(FVector(-5000.f, 0.f, 0.f), FRotator(0.f, 180.f, 0.f), EGoalTeam::TeamB);
		GameState = TestWorld->SpawnGameState(ScoreToWin, CountdownSeconds);
	});

	AfterEach([this]()
	{
		GameState = nullptr;
		TeamAGoal = TeamBGoal = nullptr;
		TestWorld.Reset();
	});

	Describe("Countdown", [this]()
	{
		It("starts waiting, with movement locked", [this]()
		{
			TestTrue(TEXT("Waiting to start"), GameState->GetMatchPhase() == EMatchPhase::WaitingToStart);
			TestEqual(TEXT("Countdown value"), GameState->GetCountdownSeconds(), CountdownSeconds);
			TestFalse(TEXT("Can't move yet"), GameState->CanMove());
		});

		It("starts the match when the countdown reaches zero", [this]()
		{
			FinishCountdown();
			TestTrue(TEXT("In progress"), GameState->GetMatchPhase() == EMatchPhase::InProgress);
			TestTrue(TEXT("Can move"), GameState->CanMove());
			TestEqual(TEXT("Countdown finished"), GameState->GetCountdownSeconds(), 0);
		});

		It("blocks broom input until the match starts", [this]()
		{
			ABroom* Broom = TestWorld->SpawnBroom(FVector::ZeroVector);
			TestWorld->SpawnQuaffle(FVector(100.f, 0.f, 0.f));

			Broom->PerformPickupQuaffle();
			TestFalse(TEXT("No pickup during countdown"), Broom->IsHoldingQuaffle());

			FinishCountdown();
			Broom->PerformPickupQuaffle();
			TestTrue(TEXT("Pickup once the match is live"), Broom->IsHoldingQuaffle());
		});
	});

	Describe("Scoring", [this]()
	{
		It("ignores goals before the match starts", [this]()
		{
			ScoreOn(TeamAGoal);
			TestEqual(TEXT("Team B score"), GameState->GetTeamBScore(), 0);
		});

		It("credits the team attacking the ring, not the one defending it", [this]()
		{
			FinishCountdown();
			ScoreOn(TeamAGoal);
			ScoreOn(TeamBGoal);
			ScoreOn(TeamBGoal);

			TestEqual(TEXT("Team B scored once into Team A's ring"), GameState->GetTeamBScore(), 10);
			TestEqual(TEXT("Team A scored twice into Team B's ring"), GameState->GetTeamAScore(), 20);
		});
	});

	Describe("Winning", [this]()
	{
		It("never ends on score when the score target is off (the default)", [this]()
		{
			FQuidditchTestWorld::SetPropertyValue(GameState, TEXT("ScoreToWin"), 0);
			FQuidditchTestWorld::SetPropertyValue(GameState, TEXT("SnitchReleaseScore"), 0);
			FinishCountdown();
			for (int32 Goal = 0; Goal < 30; ++Goal)
			{
				ScoreOn(TeamBGoal);
			}

			TestEqual(TEXT("Team A scored 300"), GameState->GetTeamAScore(), 300);
			TestTrue(TEXT("Still in progress"), GameState->GetMatchPhase() == EMatchPhase::InProgress);
		});

		It("ends the match when a team reaches the target score", [this]()
		{
			FinishCountdown();
			for (int32 Goal = 0; Goal < ScoreToWin / 10; ++Goal)
			{
				ScoreOn(TeamBGoal);
			}

			TestTrue(TEXT("Match ended"), GameState->GetMatchPhase() == EMatchPhase::MatchEnded);
			TestTrue(TEXT("Reported as ended"), GameState->IsMatchEnded());
			TestFalse(TEXT("Movement locked"), GameState->CanMove());
		});

		It("ignores further goals after the match has ended", [this]()
		{
			FinishCountdown();
			for (int32 Goal = 0; Goal < ScoreToWin / 10; ++Goal)
			{
				ScoreOn(TeamAGoal);
			}

			ScoreOn(TeamAGoal);
			TestEqual(TEXT("Score frozen at the winning total"), GameState->GetTeamBScore(), ScoreToWin);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
