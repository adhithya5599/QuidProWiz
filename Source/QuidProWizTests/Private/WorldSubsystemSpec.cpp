// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "QuidditchTestWorld.h"
#include "QuidditchWorldSubsystem.h"
#include "Broom.h"
#include "Quaffle.h"
#include "Bludger.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FQuidditchWorldSubsystemSpec, "QuidProWiz.WorldSubsystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TUniquePtr<FQuidditchTestWorld> TestWorld;
END_DEFINE_SPEC(FQuidditchWorldSubsystemSpec)

void FQuidditchWorldSubsystemSpec::Define()
{
	BeforeEach([this]() { TestWorld = MakeUnique<FQuidditchTestWorld>(); });
	AfterEach([this]() { TestWorld.Reset(); });

	It("registers every match actor on spawn", [this]()
	{
		TestWorld->SpawnBroom(FVector::ZeroVector);
		TestWorld->SpawnQuaffle(FVector(500.f, 0.f, 0.f));
		TestWorld->SpawnQuaffle(FVector(900.f, 0.f, 0.f));
		TestWorld->SpawnBludger(FVector(0.f, 3000.f, 0.f));
		TestWorld->SpawnBludger(FVector(0.f, -3000.f, 0.f));
		TestWorld->SpawnGoal(FVector(5000.f, 0.f, 0.f), FRotator::ZeroRotator);

		const UQuidditchWorldSubsystem* Subsystem = TestWorld->GetSubsystem();
		if (!TestNotNull(TEXT("Subsystem"), Subsystem)) return;

		TestEqual(TEXT("Brooms"), Subsystem->GetBrooms().Num(), 1);
		TestEqual(TEXT("Quaffles"), Subsystem->GetQuaffles().Num(), 2);
		TestEqual(TEXT("Bludgers (more than one is supported)"), Subsystem->GetBludgers().Num(), 2);
		TestEqual(TEXT("Goals"), Subsystem->GetGoals().Num(), 1);
	});

	It("unregisters actors when they are destroyed", [this]()
	{
		ABludger* Bludger = TestWorld->SpawnBludger(FVector::ZeroVector);
		const UQuidditchWorldSubsystem* Subsystem = TestWorld->GetSubsystem();
		TestEqual(TEXT("Before destroy"), Subsystem->GetBludgers().Num(), 1);

		Bludger->Destroy();
		TestEqual(TEXT("After destroy"), Subsystem->GetBludgers().Num(), 0);
	});

	It("finds the nearest free Quaffle and skips held ones", [this]()
	{
		ABroom* Broom = TestWorld->SpawnBroom(FVector::ZeroVector);
		AQuaffle* Near = TestWorld->SpawnQuaffle(FVector(100.f, 0.f, 0.f));
		AQuaffle* Far = TestWorld->SpawnQuaffle(FVector(1500.f, 0.f, 0.f));
		const UQuidditchWorldSubsystem* Subsystem = TestWorld->GetSubsystem();

		TestTrue(TEXT("Nearest while both free"), Subsystem->FindNearestFreeQuaffle(FVector::ZeroVector) == Near);

		Broom->PerformPickupQuaffle();
		TestTrue(TEXT("Broom picked up the near one"), Broom->GetHeldQuaffle() == Near);
		TestTrue(TEXT("Nearest free skips the held one"), Subsystem->FindNearestFreeQuaffle(FVector::ZeroVector) == Far);
	});

	It("finds the nearest Bludger and reports its distance", [this]()
	{
		TestWorld->SpawnBludger(FVector(2000.f, 0.f, 0.f));
		ABludger* Near = TestWorld->SpawnBludger(FVector(0.f, 600.f, 0.f));

		float Distance = 0.f;
		TestTrue(TEXT("Nearest Bludger"), TestWorld->GetSubsystem()->FindNearestBludger(FVector::ZeroVector, &Distance) == Near);
		TestEqual(TEXT("Distance"), Distance, 600.f, 1.f);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
