// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BroomMovementComponent.generated.h"

class ABroom;

/**
 * Client-predicted broom flight built on the Character Movement Component's flying mode.
 *
 * Flight model (CalcVelocity): the input vector says where the rider wants to go and how hard.
 * The velocity's direction swings toward it at a limited turn rate (fast when slow, slower at top
 * speed), and its speed eases toward the throttle's target, so turns carve smooth arcs and the
 * broom keeps its momentum. With no throttle it coasts to a gentle stop; pulling back while moving
 * brakes. The broom turns to face its direction of travel.
 *
 * Everything depends only on the move's input, velocity, boost flag and the data asset, so the
 * owning client predicts it exactly and the server replays it (standard CMC reconciliation).
 * Boost is part of each saved move (a compressed flag).
 */
UCLASS()
class QUIDPROWIZ_API UBroomMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:

	UBroomMovementComponent();

	/** Local intent to boost; sent to the server with every move. */
	void SetWantsToBoost(bool bNewWantsToBoost) { bWantsToBoost = bNewWantsToBoost; }
	bool WantsToBoost() const { return bWantsToBoost; }

	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
	virtual float GetMaxBrakingDeceleration() const override;

	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	/** Base flight speed after the AI override and stun, before boost (cm/s). */
	float GetCruiseSpeed() const;

protected:

	virtual void InitializeComponent() override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
	virtual FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const override;

private:

	bool bWantsToBoost = false;

	const ABroom* GetBroomOwner() const;
	float GetBaseSpeed() const;
	float GetStunScale() const;
};

/** Saved move that remembers whether the player was boosting. */
class FSavedMove_Broom : public FSavedMove_Character
{
public:
	typedef FSavedMove_Character Super;

	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel,
		FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* Character) override;

private:
	bool bSavedWantsToBoost = false;
};

class FNetworkPredictionData_Client_Broom : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;

	explicit FNetworkPredictionData_Client_Broom(const UCharacterMovementComponent& ClientMovement)
		: Super(ClientMovement)
	{
	}

	virtual FSavedMovePtr AllocateNewMove() override;
};
