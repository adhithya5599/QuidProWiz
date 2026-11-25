// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "QuidditchTestWorld.h"
#include "Broom.h"
#include "Quaffle.h"
#include "Bludger.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FQuidditchBludgerSpec, "QuidProWiz.Bludger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TUniquePtr<FQuidditchTestWorld> TestWorld;
END_DEFINE_SPEC(FQuidditchBludgerSpec)

void FQuidditchBludgerSpec::Define()
{
	BeforeEach([this]() { TestWorld = MakeUnique<FQuidditchTestWorld>(); });
	AfterEach([this]() { TestWorld.Reset(); });

	It("hunts down the Quaffle carrier and knocks the Quaffle loose", [this]()
	{
		ABroom* Carrier = TestWorld->SpawnBroom(FVector::ZeroVector);
		AQuaffle* Quaffle = TestWorld->SpawnQuaffle(FVector(100.f, 0.f, 0.f));
		Carrier->PerformPickupQuaffle();
		if (!TestTrue(TEXT("Carrier holds the Quaffle"), Carrier->IsHoldingQuaffle())) return;

		TestWorld->SpawnBludger(FVector(800.f, 0.f, 0.f));

		const bool bHit = TestWorld->TickUntil(5.f, [Carrier]() { return Carrier->IsStunned(); });
		TestTrue(TEXT("Carrier was hit within 5s"), bHit);
		TestFalse(TEXT("Carrier lost the Quaffle"), Carrier->IsHoldingQuaffle());
		TestTrue(TEXT("Quaffle is loose"), Quaffle->CanBePickedUp());
	});

	It("leaves players without the Quaffle alone", [this]()
	{
		ABroom* Bystander = TestWorld->SpawnBroom(FVector::ZeroVector);
		TestWorld->SpawnBludger(FVector(300.f, 0.f, 0.f));

		bool bEverStunned = false;
		TestWorld->TickUntil(3.f, [&]()
		{
			bEverStunned |= Bystander->IsStunned();
			return false;
		});

		TestFalse(TEXT("Bystander never stunned"), bEverStunned);
	});

	It("prefers the carrier when several brooms are around", [this]()
	{
		ABroom* Bystander = TestWorld->SpawnBroom(FVector(600.f, 0.f, 0.f));
		ABroom* Carrier = TestWorld->SpawnBroom(FVector(0.f, 1200.f, 0.f));
		TestWorld->SpawnQuaffle(FVector(0.f, 1300.f, 0.f));
		Carrier->PerformPickupQuaffle();

		// The Bludger starts right next to the bystander, but should go after the carrier.
		TestWorld->SpawnBludger(FVector(800.f, 0.f, 0.f));

		TestTrue(TEXT("Carrier hit within 6s"), TestWorld->TickUntil(6.f, [Carrier]() { return Carrier->IsStunned(); }));
		TestFalse(TEXT("Bystander untouched"), Bystander->IsStunned());
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
