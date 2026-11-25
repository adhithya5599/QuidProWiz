// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Quaffle.h"
#include "Match/MatchState.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FQuidditchStateRulesSpec, "QuidProWiz.StateRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FQuidditchStateRulesSpec)

void FQuidditchStateRulesSpec::Define()
{
	Describe("Quaffle states", [this]()
	{
		It("only a free Quaffle can be picked up", [this]()
		{
			TestTrue(TEXT("Free"), FQuaffleStateBase::Get(EQuaffleState::Free).CanPickUp());
			TestFalse(TEXT("Possessed"), FQuaffleStateBase::Get(EQuaffleState::Possessed).CanPickUp());
			TestFalse(TEXT("Thrown"), FQuaffleStateBase::Get(EQuaffleState::Thrown).CanPickUp());
		});

		It("only a possessed Quaffle can be thrown", [this]()
		{
			TestFalse(TEXT("Free"), FQuaffleStateBase::Get(EQuaffleState::Free).CanThrow());
			TestTrue(TEXT("Possessed"), FQuaffleStateBase::Get(EQuaffleState::Possessed).CanThrow());
			TestFalse(TEXT("Thrown"), FQuaffleStateBase::Get(EQuaffleState::Thrown).CanThrow());
		});

		It("only a thrown Quaffle can score", [this]()
		{
			TestFalse(TEXT("Free"), FQuaffleStateBase::Get(EQuaffleState::Free).CanScore());
			TestFalse(TEXT("Possessed"), FQuaffleStateBase::Get(EQuaffleState::Possessed).CanScore());
			TestTrue(TEXT("Thrown"), FQuaffleStateBase::Get(EQuaffleState::Thrown).CanScore());
		});
	});

	Describe("Match phases", [this]()
	{
		It("only allows flying and scoring while in progress", [this]()
		{
			for (EMatchPhase Phase : { EMatchPhase::WaitingToStart, EMatchPhase::InProgress,
				EMatchPhase::MatchEnded, EMatchPhase::ReturnToMenu })
			{
				const FMatchStateBase& State = FMatchStateBase::Get(Phase);
				const bool bInProgress = Phase == EMatchPhase::InProgress;
				const FString Name = State.GetStateName();

				TestEqual(FString::Printf(TEXT("%s CanMove"), *Name), State.CanMove(), bInProgress);
				TestEqual(FString::Printf(TEXT("%s CanScore"), *Name), State.CanScore(), bInProgress);
			}
		});

		It("reports the match as ended once it has ended", [this]()
		{
			TestFalse(TEXT("WaitingToStart"), FMatchStateBase::Get(EMatchPhase::WaitingToStart).IsMatchEnded());
			TestFalse(TEXT("InProgress"), FMatchStateBase::Get(EMatchPhase::InProgress).IsMatchEnded());
			TestTrue(TEXT("MatchEnded"), FMatchStateBase::Get(EMatchPhase::MatchEnded).IsMatchEnded());
			TestTrue(TEXT("ReturnToMenu"), FMatchStateBase::Get(EMatchPhase::ReturnToMenu).IsMatchEnded());
		});

		It("keeps the state names the UI binds to", [this]()
		{
			TestEqual(TEXT("WaitingToStart"), FString(FMatchStateBase::Get(EMatchPhase::WaitingToStart).GetStateName()), FString(TEXT("WaitingToStart")));
			TestEqual(TEXT("InProgress"), FString(FMatchStateBase::Get(EMatchPhase::InProgress).GetStateName()), FString(TEXT("InProgress")));
			TestEqual(TEXT("MatchEnded"), FString(FMatchStateBase::Get(EMatchPhase::MatchEnded).GetStateName()), FString(TEXT("MatchEnded")));
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
