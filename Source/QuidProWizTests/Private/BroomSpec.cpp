// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "QuidditchTestWorld.h"
#include "Broom.h"
#include "BroomDataAsset.h"
#include "Quaffle.h"
#include "GameFramework/CharacterMovementComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FQuidditchBroomSpec, "QuidProWiz.Broom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TUniquePtr<FQuidditchTestWorld> TestWorld;
	ABroom* Broom = nullptr;
	const UBroomDataAsset* Defaults = nullptr;

	float MaxSpeed() const { return Broom->GetCurrentMaxSpeed(); }

	/** Holds the given inputs every frame for Seconds. */
	void FlyFor(float Seconds, float Forward, float Turn = 0.f)
	{
		ABroom* LocalBroom = Broom;
		TestWorld->TickUntil(Seconds, [LocalBroom, Forward, Turn]()
		{
			LocalBroom->PerformMove(Forward);
			LocalBroom->PerformTurn(Turn);
			return false;
		});
	}

	/** Points the broom's aim (its controller's rotation), like moving the mouse. */
	void Aim(const FRotator& Rotation) const
	{
		Broom->GetController()->SetControlRotation(Rotation);
	}

	/** Angle in degrees between the flight direction and Direction. */
	float FlightAngleTo(const FVector& Direction) const
	{
		const FVector Velocity = Broom->GetVelocity().GetSafeNormal();
		return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Velocity, Direction.GetSafeNormal()), -1.f, 1.f)));
	}
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

	Describe("Flight (Character Movement, flying mode)", [this]()
	{
		BeforeEach([this]() { TestWorld->Possess(Broom); });

		It("flies forward under input without exceeding its speed limit", [this]()
		{
			FlyFor(1.f, 1.f);

			TestTrue(TEXT("Moved forward"), Broom->GetActorLocation().X > 300.f);
			TestTrue(TEXT("Within the speed limit"), Broom->GetCurrentSpeed() <= MaxSpeed() + 1.f);
		});

		It("covers more ground while boosting", [this]()
		{
			FlyFor(1.f, 1.f);
			const float NormalDistance = Broom->GetActorLocation().X;

			Broom->SetActorLocation(FVector::ZeroVector);
			Broom->GetCharacterMovement()->StopMovementImmediately();
			Broom->PerformSpeedBoost(true);
			FlyFor(1.f, 1.f);

			AddInfo(FString::Printf(TEXT("Normal %.0f vs boosted %.0f units in 1s"), NormalDistance, Broom->GetActorLocation().X));
			TestTrue(TEXT("Boosted flight goes further"), Broom->GetActorLocation().X > NormalDistance * 1.5f);
		});

		It("flies where it is aimed, climbing when aimed up", [this]()
		{
			const FRotator UpAndRight(30.f, 90.f, 0.f);
			Aim(UpAndRight);
			FlyFor(1.5f, 1.f);

			AddInfo(FString::Printf(TEXT("Flight is %.1f degrees off the aim"), FlightAngleTo(UpAndRight.Vector())));
			TestTrue(TEXT("Flying along the aim"), FlightAngleTo(UpAndRight.Vector()) < 3.f);
			TestTrue(TEXT("Climbed"), Broom->GetActorLocation().Z > 300.f);
			TestTrue(TEXT("Went right (+Y)"), Broom->GetActorLocation().Y > 500.f);
			TestTrue(TEXT("Faces its direction of travel"), FMath::Abs(FMath::FindDeltaAngleDegrees(Broom->GetActorRotation().Yaw, 90.f)) < 5.f);
		});

		It("carves a turn and keeps its momentum instead of snapping", [this]()
		{
			FlyFor(2.f, 1.f); // up to speed along +X
			const float CruiseSpeed = Broom->GetCurrentSpeed();

			Aim(FRotator(0.f, 90.f, 0.f));
			FlyFor(0.1f, 1.f);
			const float EarlyAngle = FlightAngleTo(FVector::RightVector);
			TestTrue(TEXT("Still mid-turn after 0.1s (no snap)"), EarlyAngle > 60.f);

			FlyFor(1.f, 1.f);
			AddInfo(FString::Printf(TEXT("Angle to the new heading: %.1f after 0.1s, %.1f after 1.1s; speed %.0f -> %.0f"),
				EarlyAngle, FlightAngleTo(FVector::RightVector), CruiseSpeed, Broom->GetCurrentSpeed()));
			TestTrue(TEXT("Completed the turn within about a second"), FlightAngleTo(FVector::RightVector) < 3.f);
			TestTrue(TEXT("Kept its speed through the turn"), Broom->GetCurrentSpeed() > CruiseSpeed * 0.95f);
		});

		It("turns the aim with the keyboard at the data asset's turn speed, ramping in", [this]()
		{
			Broom->GetBroomData()->TurnSpeed = 120.f;
			FlyFor(0.05f, 0.f, 1.f);
			const float EarlyYaw = Broom->GetController()->GetControlRotation().Yaw;
			FlyFor(0.95f, 0.f, 1.f);
			const float Yaw = FRotator::NormalizeAxis(Broom->GetController()->GetControlRotation().Yaw);

			AddInfo(FString::Printf(TEXT("Aim yaw after 0.05s: %.1f, after 1s: %.1f"), EarlyYaw, Yaw));
			TestTrue(TEXT("Ramps in rather than jumping"), EarlyYaw < 120.f * 0.05f);
			TestTrue(TEXT("Turned close to 120 degrees in a second"), Yaw > 95.f && Yaw < 125.f);
		});

		It("coasts gently with no throttle and brakes hard when pulled back", [this]()
		{
			FlyFor(2.f, 1.f);
			const float Cruise = Broom->GetCurrentSpeed();

			FlyFor(0.5f, 0.f);
			const float Coasting = Broom->GetCurrentSpeed();

			Broom->GetCharacterMovement()->Velocity = FVector(Cruise, 0.f, 0.f);
			FlyFor(0.5f, -1.f);
			const float Braking = Broom->GetCurrentSpeed();

			AddInfo(FString::Printf(TEXT("From %.0f: coasting %.0f, braking %.0f after 0.5s"), Cruise, Coasting, Braking));
			TestTrue(TEXT("Coasting keeps most of the speed"), Coasting > Cruise * 0.6f);
			TestTrue(TEXT("Braking slows much faster"), Braking < Coasting * 0.5f);
			TestTrue(TEXT("Braking doesn't spin the broom round"), FMath::Abs(Broom->GetActorRotation().Yaw) < 5.f);
		});

		It("banks into turns", [this]()
		{
			FlyFor(1.5f, 1.f);
			Aim(FRotator(0.f, 90.f, 0.f));
			FlyFor(0.3f, 1.f);

			const float Roll = Broom->GetVisualPivot()->GetRelativeRotation().Roll;
			AddInfo(FString::Printf(TEXT("Bank while turning right: %.1f degrees"), Roll));
			TestTrue(TEXT("Leaning into a right turn"), Roll > 10.f);
		});

		It("keeps its course during free look", [this]()
		{
			FlyFor(1.f, 1.f);
			Broom->PerformFreeLook(true);
			Aim(FRotator(0.f, 120.f, 0.f)); // look over the shoulder
			FlyFor(1.f, 1.f);

			TestTrue(TEXT("Still flying along +X"), FlightAngleTo(FVector::ForwardVector) < 3.f);

			Broom->PerformFreeLook(false);
			FlyFor(1.f, 1.f);
			TestTrue(TEXT("Camera swung back behind the broom"),
				FMath::Abs(FMath::FindDeltaAngleDegrees(Broom->GetController()->GetControlRotation().Yaw, 0.f)) < 5.f);
		});

		It("doesn't fly while stunned", [this]()
		{
			Broom->ApplyStun(5.f, 0.f);
			FlyFor(1.f, 1.f);

			TestTrue(TEXT("Stayed put"), Broom->GetActorLocation().Size() < 1.f);
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
