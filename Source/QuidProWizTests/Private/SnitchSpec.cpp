// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "QuidditchTestWorld.h"
#include "QuidditchWorldSubsystem.h"
#include "QuidProWizGameStateBase.h"
#include "Broom.h"
#include "Snitch.h"
#include "GoalRing.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * The Snitch end-to-end: dormant until released (by score or by time), evasive flight,
 * and the catch that ends the match for the catcher's team regardless of score.
 * Rings: Team A defends one end, Team B the other; the arena centre is between them.
 */
BEGIN_DEFINE_SPEC(FQuidditchSnitchSpec, "QuidProWiz.Snitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TUniquePtr<FQuidditchTestWorld> TestWorld;
	AQuidProWizGameStateBase* GameState = nullptr;
	AGoalRing* TeamAGoal = nullptr;
	AGoalRing* TeamBGoal = nullptr;

	static constexpr int32 CountdownSeconds = 1;
	inline static const FVector ArenaCenter = FVector(0.f, 0.f, 500.f);
	inline static const FVector SnitchStart = FVector(0.f, 0.f, 1100.f);

	void ScoreOn(AGoalRing* Goal)
	{
		TestWorld->GetSubsystem()->OnGoalScored.Broadcast(Goal, Goal->GetScoreValue());
	}

	void FinishCountdown()
	{
		TestWorld->Tick(CountdownSeconds + 0.1f);
	}

	void SetReleaseRules(int32 ReleaseScore, float TimeMin, float TimeMax)
	{
		FQuidditchTestWorld::SetPropertyValue(GameState, TEXT("SnitchReleaseScore"), ReleaseScore);
		FQuidditchTestWorld::SetPropertyValue(GameState, TEXT("SnitchReleaseTimeMin"), TimeMin);
		FQuidditchTestWorld::SetPropertyValue(GameState, TEXT("SnitchReleaseTimeMax"), TimeMax);
	}

	/** Releases the Snitch through the score rule (Team A scores once into Team B's ring). */
	void ReleaseByScore()
	{
		SetReleaseRules(10, 0.f, 0.f);
		FinishCountdown();
		ScoreOn(TeamBGoal);
	}
END_DEFINE_SPEC(FQuidditchSnitchSpec)

void FQuidditchSnitchSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld = MakeUnique<FQuidditchTestWorld>();
		TeamAGoal = TestWorld->SpawnGoal(FVector(-2000.f, 0.f, 500.f), FRotator::ZeroRotator, EGoalTeam::TeamA);
		TeamBGoal = TestWorld->SpawnGoal(FVector(2000.f, 0.f, 500.f), FRotator(0.f, 180.f, 0.f), EGoalTeam::TeamB);
		GameState = TestWorld->SpawnGameState(/*ScoreToWin*/ 0, CountdownSeconds);
		SetReleaseRules(30, 0.f, 0.f); // by score only, unless a test changes it
	});

	AfterEach([this]()
	{
		GameState = nullptr;
		TeamAGoal = TeamBGoal = nullptr;
		TestWorld.Reset();
	});

	Describe("Release", [this]()
	{
		It("stays hidden and still until released", [this]()
		{
			ASnitch* Snitch = TestWorld->SpawnSnitch(SnitchStart);
			FinishCountdown();
			TestWorld->Tick(1.f);

			TestTrue(TEXT("Dormant"), Snitch->GetSnitchState() == ESnitchState::Dormant);
			TestTrue(TEXT("Hidden"), Snitch->IsHidden());
			TestEqual(TEXT("Hasn't moved"), Snitch->GetActorLocation(), SnitchStart, 1.f);
			TestFalse(TEXT("GameState agrees"), GameState->IsSnitchReleased());
		});

		It("is released when a team reaches the release score", [this]()
		{
			ASnitch* Snitch = TestWorld->SpawnSnitch(SnitchStart);
			FinishCountdown();

			ScoreOn(TeamBGoal);
			ScoreOn(TeamBGoal);
			TestFalse(TEXT("Not yet at 20 points"), GameState->IsSnitchReleased());

			ScoreOn(TeamBGoal);
			TestTrue(TEXT("Released at 30 points"), GameState->IsSnitchReleased());
			TestTrue(TEXT("Snitch active"), Snitch->IsActive());
			TestFalse(TEXT("Snitch visible"), Snitch->IsHidden());
		});

		It("is released at the random time even if nobody scores", [this]()
		{
			ASnitch* Snitch = TestWorld->SpawnSnitch(SnitchStart);
			SetReleaseRules(0, 2.f, 2.f);
			FinishCountdown();

			TestWorld->Tick(1.5f);
			TestFalse(TEXT("Not before its time"), GameState->IsSnitchReleased());

			TestWorld->Tick(1.f);
			TestTrue(TEXT("Released on time"), GameState->IsSnitchReleased());
			TestTrue(TEXT("Snitch active"), Snitch->IsActive());
		});

		It("spawns above the middle of the pitch when none is placed", [this]()
		{
			ReleaseByScore();

			const TArray<ASnitch*> Snitches = TestWorld->GetSubsystem()->GetSnitches();
			if (!TestEqual(TEXT("One Snitch spawned"), Snitches.Num(), 1)) return;

			TestTrue(TEXT("Active"), Snitches[0]->IsActive());
			TestEqual(TEXT("Spawned above the arena centre"), Snitches[0]->GetActorLocation(),
				ArenaCenter + FVector(0.f, 0.f, 600.f), 1.f);
		});

		It("is never released before the match starts", [this]()
		{
			ASnitch* Snitch = TestWorld->SpawnSnitch(SnitchStart);
			GameState->ReleaseSnitch();

			TestFalse(TEXT("Not released during the countdown"), GameState->IsSnitchReleased());
			TestTrue(TEXT("Still dormant"), Snitch->GetSnitchState() == ESnitchState::Dormant);
		});
	});

	Describe("Catching", [this]()
	{
		It("ends the match and the catcher's team wins regardless of score", [this]()
		{
			ASnitch* Snitch = TestWorld->SpawnSnitch(SnitchStart);
			SetReleaseRules(10, 0.f, 0.f);
			FinishCountdown();

			// Team B takes the lead (and releases the Snitch at 10 points)...
			ScoreOn(TeamAGoal);
			ScoreOn(TeamAGoal);
			if (!TestTrue(TEXT("Released"), Snitch->IsActive())) return;

			// ...but a Team A broom catches it.
			ABroom* Catcher = TestWorld->SpawnBroom(Snitch->GetActorLocation());
			TestTrue(TEXT("Catcher is Team A"), Catcher->GetTeam() == EGoalTeam::TeamA);
			TestWorld->TickUntil(0.5f, [this]() { return GameState->IsMatchEnded(); });

			TestTrue(TEXT("Match ended"), GameState->GetMatchPhase() == EMatchPhase::MatchEnded);
			TestTrue(TEXT("Ended by the Snitch"), GameState->WasEndedBySnitch());
			TestTrue(TEXT("Team A wins"), GameState->GetWinningTeam() == EGoalTeam::TeamA);
			TestTrue(TEXT("Even though Team B had more points"), GameState->GetTeamBScore() > GameState->GetTeamAScore());
			TestTrue(TEXT("Snitch caught"), Snitch->GetSnitchState() == ESnitchState::Caught);
		});

		It("can't be caught by a stunned broom", [this]()
		{
			ASnitch* Snitch = TestWorld->SpawnSnitch(SnitchStart);
			ReleaseByScore();

			ABroom* Broom = TestWorld->SpawnBroom(FVector(0.f, 3000.f, 500.f));
			Broom->ApplyStun(5.f, 0.f);
			Broom->SetActorLocation(Snitch->GetActorLocation());
			TestWorld->Tick(0.2f);

			TestTrue(TEXT("Match continues"), GameState->GetMatchPhase() == EMatchPhase::InProgress);
			TestTrue(TEXT("Snitch still free"), Snitch->IsActive());
		});
	});

	Describe("Flight", [this]()
	{
		It("flees from a broom that gets close", [this]()
		{
			ASnitch* Snitch = TestWorld->SpawnSnitch(SnitchStart);
			ReleaseByScore();

			ABroom* Chaser = TestWorld->SpawnBroom(SnitchStart + FVector(500.f, 0.f, 0.f));
			const float StartDistance = FVector::Dist(Snitch->GetActorLocation(), Chaser->GetActorLocation());

			TestWorld->Tick(1.f);
			const float EndDistance = FVector::Dist(Snitch->GetActorLocation(), Chaser->GetActorLocation());

			AddInfo(FString::Printf(TEXT("Distance %.0f -> %.0f in 1s"), StartDistance, EndDistance));
			TestTrue(TEXT("Got at least 300 units further away"), EndDistance > StartDistance + 300.f);
		});

		It("keeps flying inside the arena", [this]()
		{
			ASnitch* Snitch = TestWorld->SpawnSnitch(SnitchStart);
			ReleaseByScore();

			const float ArenaRadius = 3500.f; // USnitchDataAsset default
			float FurthestOut = 0.f;
			float Travelled = 0.f;
			FVector Last = Snitch->GetActorLocation();

			TestWorld->TickUntil(15.f, [&]()
			{
				const FVector Now = Snitch->GetActorLocation();
				const FVector FromCenter = Now - ArenaCenter;
				FurthestOut = FMath::Max(FurthestOut, FVector(FromCenter.X, FromCenter.Y, 0.f).Size());
				Travelled += FVector::Dist(Now, Last);
				Last = Now;
				return false;
			});

			AddInfo(FString::Printf(TEXT("Travelled %.0f units; furthest %.0f from centre"), Travelled, FurthestOut));
			TestTrue(TEXT("Moved around"), Travelled > 3000.f);
			TestTrue(TEXT("Stayed near the arena (small overshoot allowed)"), FurthestOut < ArenaRadius + 800.f);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
