// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BroomMovementComponent.generated.h"

class ABroom;

/**
 * Client-predicted broom flight built on the Character Movement Component's flying mode.
 *
 * The owning client simulates its moves immediately and the server replays them; mismatches are
 * corrected by the standard CMC reconciliation. Boost is part of each saved move (a compressed
 * flag) so it is predicted too. Speed is derived every frame from the broom's data asset, any AI
 * speed override and the replicated stun multiplier, so tuning changes apply live.
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

protected:

	virtual void InitializeComponent() override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

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
