// Fill out your copyright notice in the Description page of Project Settings.

#include "BroomMovementComponent.h"
#include "Broom.h"
#include "BroomDataAsset.h"
#include "GameFramework/Character.h"

UBroomMovementComponent::UBroomMovementComponent()
{
	// Brooms only ever fly.
	DefaultLandMovementMode = MOVE_Flying;
	DefaultWaterMovementMode = MOVE_Flying;
	NavAgentProps.bCanFly = true;
	NavAgentProps.bCanWalk = false;

	// Placed or uncontrolled brooms still hover and respond to physics.
	bRunPhysicsWithNoController = true;

	// The broom faces where it is flying (see ComputeOrientToMovementRotation).
	bOrientRotationToMovement = true;
	bUseControllerDesiredRotation = false;
	RotationRate = FRotator(0.f, 720.f, 0.f);

	GravityScale = 0.f;
	BrakingFrictionFactor = 1.f;
}

namespace BroomFlight
{
	/** Below this speed the broom counts as stopped: it can reverse and points straight at its input. */
	constexpr float StopSpeed = 60.f;

	/** Pulling back against the current direction (dot below this) brakes instead of turning round. */
	constexpr float BrakeDot = -0.3f;

	/** Above this horizontal speed the broom faces its velocity; below it, its input. */
	constexpr float FaceVelocitySpeed = 100.f;
}

float UBroomMovementComponent::GetCruiseSpeed() const
{
	return GetBaseSpeed() * GetStunScale();
}

void UBroomMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	const ABroom* Broom = GetBroomOwner();
	const UBroomDataAsset* Data = Broom ? Broom->GetBroomData() : nullptr;

	if (MovementMode != MOVE_Flying || !Data || !HasValidData() || HasAnimRootMotion() || DeltaTime < MIN_TICK_TIME)
	{
		Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
		return;
	}

	// Rates scale with the unstunned base speed; the speed limit includes boost and stun.
	const float Base = FMath::Max(GetBaseSpeed(), 1.f);
	const float Cruise = FMath::Max(GetCruiseSpeed(), 1.f);
	const float MaxSpeed = GetMaxSpeed();
	const float MaxAccel = FMath::Max(GetMaxAcceleration(), KINDA_SMALL_NUMBER);

	float Speed = Velocity.Size();
	FVector Direction = Speed > KINDA_SMALL_NUMBER ? Velocity / Speed : FVector::ZeroVector;

	// Acceleration is the move's input (direction * strength) scaled to MaxAccel.
	const float InputStrength = FMath::Clamp(Acceleration.Size() / MaxAccel, 0.f, 1.f);

	// No throttle: coast to a gentle stop along the current line (quickly if over the limit, e.g. stunned).
	if (InputStrength < 0.01f)
	{
		const float Rate = Speed > MaxSpeed ? Data->BrakeMultiplier : Data->CoastMultiplier;
		Speed = FMath::Max(0.f, Speed - Rate * Base * DeltaTime);
		Velocity = Direction * Speed;
		return;
	}

	const FVector Desired = Acceleration.GetSafeNormal();
	const float Alignment = Speed > BroomFlight::StopSpeed ? FVector::DotProduct(Direction, Desired) : 1.f;

	// Pulling back against the direction of travel brakes rather than flipping round.
	if (Alignment < BroomFlight::BrakeDot)
	{
		Speed = FMath::Max(0.f, Speed - Data->BrakeMultiplier * Base * DeltaTime);
		Velocity = Direction * Speed;
		return;
	}

	float TargetSpeed = MaxSpeed * InputStrength;

	if (Speed <= BroomFlight::StopSpeed)
	{
		// From a standstill, go straight where asked; backing up is slower.
		Direction = Desired;
		const FVector Facing = UpdatedComponent->GetForwardVector();
		if (FVector::DotProduct(FVector(Facing.X, Facing.Y, 0.f).GetSafeNormal(), FVector(Desired.X, Desired.Y, 0.f).GetSafeNormal()) < BroomFlight::BrakeDot)
		{
			TargetSpeed = FMath::Min(TargetSpeed, Data->ReverseSpeedFraction * Cruise);
		}
	}
	else
	{
		// Swing the flight direction toward the input at a speed-dependent turn rate: carved arcs.
		const float SpeedRatio = FMath::Clamp(Speed / Cruise, 0.f, 1.f);
		float TurnRate = FMath::Lerp(Data->TurnRateSlow, Data->TurnRateFast, SpeedRatio);
		if (bWantsToBoost)
		{
			TurnRate *= Data->BoostTurnRateScale;
		}

		const float MaxStep = FMath::DegreesToRadians(TurnRate) * DeltaTime;
		const float Angle = FMath::Acos(FMath::Clamp(FVector::DotProduct(Direction, Desired), -1.f, 1.f));
		if (Angle <= MaxStep)
		{
			Direction = Desired;
		}
		else
		{
			const FQuat FullTurn = FQuat::FindBetweenNormals(Direction, Desired);
			Direction = FQuat::Slerp(FQuat::Identity, FullTurn, MaxStep / Angle).RotateVector(Direction).GetSafeNormal();
		}
	}

	// Ease the speed toward the throttle's target.
	if (Speed < TargetSpeed)
	{
		const float AccelRate = (bWantsToBoost ? Data->SpeedBoostAccelerationMultiplier : Data->AccelerationMultiplier) * Base;
		Speed = FMath::Min(TargetSpeed, Speed + AccelRate * DeltaTime);
	}
	else
	{
		// Over the target (e.g. boost released): settle back without a jolt.
		const float SettleRate = FMath::Max(Data->CoastMultiplier, Data->AccelerationMultiplier * 0.5f) * Base;
		Speed = FMath::Max(TargetSpeed, Speed - SettleRate * DeltaTime);
	}

	Velocity = Direction * Speed;
}

FRotator UBroomMovementComponent::ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const
{
	const FVector Velocity2D(Velocity.X, Velocity.Y, 0.f);
	if (Velocity2D.Size() > BroomFlight::FaceVelocitySpeed)
	{
		// Face the direction of travel; when backing up, keep facing forward.
		const FVector Facing = CurrentRotation.Vector();
		const bool bReversing = FVector::DotProduct(FVector(Facing.X, Facing.Y, 0.f).GetSafeNormal(), Velocity2D.GetSafeNormal()) < BroomFlight::BrakeDot;
		const FVector Heading = bReversing ? -Velocity2D : Velocity2D;
		return FRotator(0.f, Heading.Rotation().Yaw, 0.f);
	}

	const FVector Input2D(Acceleration.X, Acceleration.Y, 0.f);
	if (Input2D.SizeSquared() > KINDA_SMALL_NUMBER)
	{
		const FVector Facing = CurrentRotation.Vector();
		// Pulling back from a standstill backs up without turning round.
		if (FVector::DotProduct(FVector(Facing.X, Facing.Y, 0.f).GetSafeNormal(), Input2D.GetSafeNormal()) >= BroomFlight::BrakeDot)
		{
			return FRotator(0.f, Input2D.Rotation().Yaw, 0.f);
		}
	}

	return FRotator(0.f, CurrentRotation.Yaw, 0.f);
}

void UBroomMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
	SetMovementMode(MOVE_Flying);
}

const ABroom* UBroomMovementComponent::GetBroomOwner() const
{
	return Cast<ABroom>(CharacterOwner);
}

float UBroomMovementComponent::GetBaseSpeed() const
{
	const ABroom* Broom = GetBroomOwner();
	if (!Broom) return MaxFlySpeed;

	if (Broom->GetSpeedOverride() > 0.f) return Broom->GetSpeedOverride();

	const UBroomDataAsset* Data = Broom->GetBroomData();
	return Data ? Data->Speed : MaxFlySpeed;
}

float UBroomMovementComponent::GetStunScale() const
{
	const ABroom* Broom = GetBroomOwner();
	return Broom && Broom->IsStunned() ? Broom->GetStunSpeedMultiplier() : 1.f;
}

float UBroomMovementComponent::GetMaxSpeed() const
{
	const ABroom* Broom = GetBroomOwner();
	const UBroomDataAsset* Data = Broom ? Broom->GetBroomData() : nullptr;
	if (!Data) return Super::GetMaxSpeed();

	const float BoostScale = bWantsToBoost ? Data->SpeedBoostMultiplier : 1.f;
	return GetBaseSpeed() * BoostScale * GetStunScale();
}

float UBroomMovementComponent::GetMaxAcceleration() const
{
	const ABroom* Broom = GetBroomOwner();
	const UBroomDataAsset* Data = Broom ? Broom->GetBroomData() : nullptr;
	if (!Data) return Super::GetMaxAcceleration();

	const float AccelerationScale = bWantsToBoost ? Data->SpeedBoostAccelerationMultiplier : Data->AccelerationMultiplier;
	return FMath::Max(GetBaseSpeed() * AccelerationScale * GetStunScale(), 1.f);
}

float UBroomMovementComponent::GetMaxBrakingDeceleration() const
{
	const ABroom* Broom = GetBroomOwner();
	const UBroomDataAsset* Data = Broom ? Broom->GetBroomData() : nullptr;
	if (!Data) return Super::GetMaxBrakingDeceleration();

	return GetBaseSpeed() * Data->BrakeMultiplier;
}

void UBroomMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	// Server side: apply the client's boost state for this move.
	bWantsToBoost = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

FNetworkPredictionData_Client* UBroomMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		UBroomMovementComponent* MutableThis = const_cast<UBroomMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Broom(*this);
	}

	return ClientPredictionData;
}

// --- Saved move ---------------------------------------------------------------------------------

void FSavedMove_Broom::Clear()
{
	Super::Clear();
	bSavedWantsToBoost = false;
}

uint8 FSavedMove_Broom::GetCompressedFlags() const
{
	uint8 Flags = Super::GetCompressedFlags();
	if (bSavedWantsToBoost)
	{
		Flags |= FLAG_Custom_0;
	}
	return Flags;
}

bool FSavedMove_Broom::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_Broom* NewBroomMove = static_cast<const FSavedMove_Broom*>(NewMove.Get());
	if (bSavedWantsToBoost != NewBroomMove->bSavedWantsToBoost)
	{
		return false;
	}

	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_Broom::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel,
	FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	if (const UBroomMovementComponent* Movement = Cast<UBroomMovementComponent>(Character->GetCharacterMovement()))
	{
		bSavedWantsToBoost = Movement->WantsToBoost();
	}
}

void FSavedMove_Broom::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	// Replaying this move after a correction: restore the boost state it was made with.
	if (UBroomMovementComponent* Movement = Cast<UBroomMovementComponent>(Character->GetCharacterMovement()))
	{
		Movement->SetWantsToBoost(bSavedWantsToBoost);
	}
}

FSavedMovePtr FNetworkPredictionData_Client_Broom::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Broom());
}
