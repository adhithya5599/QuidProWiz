// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "QuidditchTestWorld.h"
#include "QuidditchWorldSubsystem.h"
#include "Broom.h"
#include "Quaffle.h"
#include "GoalRing.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Throws a real Quaffle at a real ring and watches the subsystem's goal event.
 * The ring sits at the origin facing +X; "in front" (the scoring side) is -X.
 */
BEGIN_DEFINE_SPEC(FQuidditchGoalScoringSpec, "QuidProWiz.GoalScoring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TUniquePtr<FQuidditchTestWorld> TestWorld;
	AGoalRing* Goal = nullptr;
	int32 GoalsScored = 0;
	int32 LastScoreValue = 0;
	FDelegateHandle GoalHandle;

	/** Spawns a broom with the Quaffle at Location facing Yaw and throws it at the ring. */
	AQuaffle* ThrowFrom(const FVector& Location, float Yaw)
	{
		ABroom* Broom = TestWorld->SpawnBroom(Location, FRotator(0.f, Yaw, 0.f));
		AQuaffle* Quaffle = TestWorld->SpawnQuaffle(Location + FRotator(0.f, Yaw, 0.f).Vector() * 100.f);
		Broom->PerformPickupQuaffle();
		TestTrue(TEXT("Thrower holds the Quaffle"), Broom->IsHoldingQuaffle());
		Quaffle->ThrowToTarget(Goal->GetActorLocation());
		return Quaffle;
	}
END_DEFINE_SPEC(FQuidditchGoalScoringSpec)

void FQuidditchGoalScoringSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld = MakeUnique<FQuidditchTestWorld>();
		Goal = TestWorld->SpawnGoal(FVector::ZeroVector, FRotator::ZeroRotator, EGoalTeam::TeamA);
		GoalsScored = 0;
		LastScoreValue = 0;
		GoalHandle = TestWorld->GetSubsystem()->OnGoalScored.AddLambda([this](AGoalRing*, int32 Value)
		{
			++GoalsScored;
			LastScoreValue = Value;
		});
	});

	AfterEach([this]()
	{
		Goal = nullptr;
		TestWorld.Reset();
	});

	It("scores when thrown through the front of the ring", [this]()
	{
		AQuaffle* Quaffle = ThrowFrom(FVector(-1500.f, 0.f, 0.f), 0.f);

		TestTrue(TEXT("Goal scored within 1.5s"), TestWorld->TickUntil(1.5f, [this]() { return GoalsScored > 0; }));
		TestEqual(TEXT("Exactly one goal"), GoalsScored, 1);
		TestEqual(TEXT("Default goal value"), LastScoreValue, 10);
		TestTrue(TEXT("Quaffle reset and free after scoring"), Quaffle->CanBePickedUp());
	});

	It("does not score when thrown through the back of the ring", [this]()
	{
		ThrowFrom(FVector(1500.f, 0.f, 0.f), 180.f);

		TestWorld->Tick(1.5f);
		TestEqual(TEXT("No goal from behind"), GoalsScored, 0);
	});

	It("uses the per-ring score override", [this]()
	{
		FQuidditchTestWorld::SetPropertyValue(Goal, TEXT("ScoreOverride"), 30);
		TestEqual(TEXT("Override reported"), Goal->GetScoreValue(), 30);

		ThrowFrom(FVector(-1500.f, 0.f, 0.f), 0.f);
		TestWorld->TickUntil(1.5f, [this]() { return GoalsScored > 0; });
		TestEqual(TEXT("Scored with override value"), LastScoreValue, 30);
	});

	It("ignores a disabled ring", [this]()
	{
		FQuidditchTestWorld::SetPropertyValue(Goal, TEXT("bGoalEnabled"), false);

		ThrowFrom(FVector(-1500.f, 0.f, 0.f), 0.f);
		TestWorld->Tick(1.5f);
		TestEqual(TEXT("No goal on disabled ring"), GoalsScored, 0);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
