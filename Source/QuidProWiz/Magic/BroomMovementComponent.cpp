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

	// Heading comes from the controller's yaw (predicted with each move), not from velocity.
	bOrientRotationToMovement = false;
	bUseControllerDesiredRotation = true;
	RotationRate = FRotator(0.f, 540.f, 0.f);

	GravityScale = 0.f;
	BrakingFrictionFactor = 1.f;
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
	return GetBaseSpeed() * AccelerationScale * GetStunScale();
}

float UBroomMovementComponent::GetMaxBrakingDeceleration() const
{
	const ABroom* Broom = GetBroomOwner();
	const UBroomDataAsset* Data = Broom ? Broom->GetBroomData() : nullptr;
	if (!Data) return Super::GetMaxBrakingDeceleration();

	return GetBaseSpeed() * Data->AccelerationMultiplier;
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
