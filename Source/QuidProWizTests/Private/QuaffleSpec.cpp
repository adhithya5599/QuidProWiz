// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "QuidditchTestWorld.h"
#include "Broom.h"
#include "Quaffle.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FQuidditchQuaffleSpec, "QuidProWiz.Quaffle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TUniquePtr<FQuidditchTestWorld> TestWorld;
END_DEFINE_SPEC(FQuidditchQuaffleSpec)

void FQuidditchQuaffleSpec::Define()
{
	BeforeEach([this]() { TestWorld = MakeUnique<FQuidditchTestWorld>(); });
	AfterEach([this]() { TestWorld.Reset(); });

	Describe("Pickup", [this]()
	{
		It("is picked up by a broom within range", [this]()
		{
			ABroom* Broom = TestWorld->SpawnBroom(FVector::ZeroVector);
			AQuaffle* Quaffle = TestWorld->SpawnQuaffle(FVector(150.f, 0.f, 0.f));

			Broom->PerformPickupQuaffle();

			TestTrue(TEXT("Broom holds the Quaffle"), Broom->GetHeldQuaffle() == Quaffle);
			TestTrue(TEXT("Quaffle knows its holder"), Quaffle->GetPossessingPlayer() == Broom);
			TestEqual(TEXT("State"), Quaffle->GetCurrentStateName(), FString(TEXT("Possessed")));
		});

		It("is not picked up from outside the pickup radius", [this]()
		{
			ABroom* Broom = TestWorld->SpawnBroom(FVector::ZeroVector);
			// Twice the default 200-unit pickup radius.
			AQuaffle* Quaffle = TestWorld->SpawnQuaffle(FVector(400.f, 0.f, 0.f));

			Broom->PerformPickupQuaffle();

			TestFalse(TEXT("Broom holds nothing"), Broom->IsHoldingQuaffle());
			TestTrue(TEXT("Quaffle still free"), Quaffle->CanBePickedUp());
		});

		It("can't be taken from another player's hands", [this]()
		{
			ABroom* Holder = TestWorld->SpawnBroom(FVector::ZeroVector);
			ABroom* Rival = TestWorld->SpawnBroom(FVector(0.f, 100.f, 0.f));
			TestWorld->SpawnQuaffle(FVector(50.f, 50.f, 0.f));

			Holder->PerformPickupQuaffle();
			Rival->PerformPickupQuaffle();

			TestTrue(TEXT("First broom keeps it"), Holder->IsHoldingQuaffle());
			TestFalse(TEXT("Rival gets nothing"), Rival->IsHoldingQuaffle());
		});

		It("becomes free again when dropped", [this]()
		{
			ABroom* Broom = TestWorld->SpawnBroom(FVector::ZeroVector);
			AQuaffle* Quaffle = TestWorld->SpawnQuaffle(FVector(100.f, 0.f, 0.f));
			Broom->PerformPickupQuaffle();

			Broom->DropQuaffle();

			TestFalse(TEXT("Broom released it"), Broom->IsHoldingQuaffle());
			TestTrue(TEXT("Quaffle is free"), Quaffle->CanBePickedUp());
			TestNull(TEXT("No holder"), Quaffle->GetPossessingPlayer());
		});
	});

	Describe("Throwing", [this]()
	{
		It("flies through the aimed point (throw solver accuracy)", [this]()
		{
			ABroom* Broom = TestWorld->SpawnBroom(FVector::ZeroVector);
			AQuaffle* Quaffle = TestWorld->SpawnQuaffle(FVector(100.f, 0.f, 0.f));
			Broom->PerformPickupQuaffle();

			const FVector Target(1500.f, 0.f, 200.f);
			if (!TestTrue(TEXT("Throw accepted"), Quaffle->ThrowToTarget(Target))) return;
			TestTrue(TEXT("A thrown Quaffle can score"), Quaffle->CanScore());

			float ClosestDistance = TNumericLimits<float>::Max();
			TestWorld->TickUntil(1.5f, [&]()
			{
				ClosestDistance = FMath::Min(ClosestDistance, FVector::Dist(Quaffle->GetActorLocation(), Target));
				return false;
			});

			// The goal's score volume is 120 units in each direction, so this is well inside it.
			AddInfo(FString::Printf(TEXT("Closest approach to the aim point: %.1f units"), ClosestDistance));
			TestTrue(TEXT("Passes within 60 units of the aim point"), ClosestDistance <= 60.f);
		});

		It("returns to its spawn point after a missed throw", [this]()
		{
			const FVector SpawnLocation(100.f, 0.f, 0.f);
			ABroom* Broom = TestWorld->SpawnBroom(FVector::ZeroVector);
			AQuaffle* Quaffle = TestWorld->SpawnQuaffle(SpawnLocation);
			Broom->PerformPickupQuaffle();
			Quaffle->ThrowToTarget(FVector(4000.f, 0.f, 0.f));

			// Default MissRespawnDelay is 3 seconds.
			TestWorld->Tick(3.2f);

			TestTrue(TEXT("Quaffle is free again"), Quaffle->CanBePickedUp());
			TestEqual(TEXT("Back at its spawn point"), Quaffle->GetActorLocation(), SpawnLocation, 1.f);
		});

		It("can't be thrown when nobody holds it", [this]()
		{
			AQuaffle* Quaffle = TestWorld->SpawnQuaffle(FVector::ZeroVector);
			TestFalse(TEXT("Throw rejected"), Quaffle->ThrowToTarget(FVector(1000.f, 0.f, 0.f)));
			TestTrue(TEXT("Still free"), Quaffle->CanBePickedUp());
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
