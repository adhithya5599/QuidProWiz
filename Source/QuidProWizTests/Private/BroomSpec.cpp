// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "QuidditchTestWorld.h"
#include "Broom.h"
#include "BroomDataAsset.h"
#include "Quaffle.h"
#include "GameFramework/FloatingPawnMovement.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FQuidditchBroomSpec, "QuidProWiz.Broom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TUniquePtr<FQuidditchTestWorld> TestWorld;
	ABroom* Broom = nullptr;
	const UBroomDataAsset* Defaults = nullptr;

	float MaxSpeed() const { return Broom->GetBroomMovementComponent()->MaxSpeed; }
END_DEFINE_SPEC(FQuidditchBroomSpec)

void FQuidditchBroomSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld = MakeUnique<FQuidditchTestWorld>();
		Broom = TestWorld->SpawnBroom(FVector::ZeroVector);
		Defaults = GetDefault<UBroomDataAsset>();
	});

	AfterEach([this]()
	{
		Broom = nullptr;
		TestWorld.Reset();
	});

	Describe("Speed", [this]()
	{
		It("flies at the data asset speed by default", [this]()
		{
			TestEqual(TEXT("Max speed"), MaxSpeed(), Defaults->Speed);
		});

		It("multiplies speed while boosting", [this]()
		{
			Broom->PerformSpeedBoost(true);
			TestEqual(TEXT("Boosted"), MaxSpeed(), Defaults->Speed * Defaults->SpeedBoostMultiplier);

			Broom->PerformSpeedBoost(false);
			TestEqual(TEXT("Back to normal"), MaxSpeed(), Defaults->Speed);
		});

		It("keeps stun in charge even when boost is pressed or released", [this]()
		{
			constexpr float StunMultiplier = 0.2f;
			Broom->ApplyStun(5.f, StunMultiplier);
			TestEqual(TEXT("Stunned"), MaxSpeed(), Defaults->Speed * StunMultiplier);

			Broom->PerformSpeedBoost(true);
			TestEqual(TEXT("Boost while stunned stays scaled"), MaxSpeed(),
				Defaults->Speed * Defaults->SpeedBoostMultiplier * StunMultiplier);

			Broom->PerformSpeedBoost(false);
			TestEqual(TEXT("Releasing boost doesn't clear stun"), MaxSpeed(), Defaults->Speed * StunMultiplier);

			Broom->RecoverFromStun();
			TestEqual(TEXT("Recovered"), MaxSpeed(), Defaults->Speed);
		});

		It("uses the AI speed override as its base speed", [this]()
		{
			Broom->SetSpeedOverride(800.f);
			TestEqual(TEXT("Override"), MaxSpeed(), 800.f);

			Broom->SetSpeedOverride(0.f);
			TestEqual(TEXT("Override cleared"), MaxSpeed(), Defaults->Speed);
		});
	});

	Describe("Stun", [this]()
	{
		It("wears off after its duration", [this]()
		{
			Broom->ApplyStun(1.f, 0.f);
			TestTrue(TEXT("Stunned"), Broom->IsStunned());

			TestWorld->Tick(1.1f);
			TestFalse(TEXT("Recovered"), Broom->IsStunned());
		});

		It("prevents picking up the Quaffle", [this]()
		{
			TestWorld->SpawnQuaffle(FVector(100.f, 0.f, 0.f));
			Broom->ApplyStun(5.f, 0.f);

			Broom->PerformPickupQuaffle();
			TestFalse(TEXT("Stunned broom can't pick up"), Broom->IsHoldingQuaffle());
		});
	});

	Describe("Bludger hit (ragdoll)", [this]()
	{
		It("drops the Quaffle, blocks pickup and recovers on respawn", [this]()
		{
			AQuaffle* Quaffle = TestWorld->SpawnQuaffle(FVector(100.f, 0.f, 0.f));
			Broom->PerformPickupQuaffle();
			if (!TestTrue(TEXT("Holding before hit"), Broom->IsHoldingQuaffle())) return;

			constexpr float RagdollDuration = 2.f;
			constexpr float RespawnDelay = 3.f;
			Broom->TriggerRagdoll(FVector::ForwardVector, 1000.f, RagdollDuration, RespawnDelay);

			TestFalse(TEXT("Quaffle dropped"), Broom->IsHoldingQuaffle());
			TestTrue(TEXT("Quaffle is free"), Quaffle->CanBePickedUp());
			TestTrue(TEXT("Stunned"), Broom->IsStunned());
			TestFalse(TEXT("Pickup on cooldown"), Broom->CanPickupQuaffle());

			// Between the end of the ragdoll and the respawn the broom must stay grounded.
			TestWorld->Tick(RagdollDuration + 0.5f);
			TestTrue(TEXT("Still stunned until respawn"), Broom->IsStunned());

			TestWorld->Tick(RespawnDelay - RagdollDuration);
			TestFalse(TEXT("Recovered after respawn"), Broom->IsStunned());
			TestTrue(TEXT("Can pick up again"), Broom->CanPickupQuaffle());
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
