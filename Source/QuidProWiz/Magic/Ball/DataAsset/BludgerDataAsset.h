// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BludgerDataAsset.generated.h"

class UNiagaraSystem;

/**
 * Tuning and presentation for Bludgers.
 */
UCLASS(BlueprintType)
class QUIDPROWIZ_API UBludgerDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bludger | Gameplay")
	float ChaseSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bludger | Gameplay")
	float SearchInterval = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bludger | Gameplay")
	float BoostSpeedMultiplier = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bludger | Gameplay")
	float BoostActiveDistance = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bludger | Gameplay")
	float RotationInterpSpeed = 8.f;

	/** How quickly velocity steers toward the desired direction while chasing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bludger | Gameplay")
	float ChaseSteeringInterpSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bludger | Gameplay")
	float HitCooldown = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bludger | Wander")
	float WanderSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bludger | Wander")
	float WanderRadius = 500.f;

	/** How quickly velocity steers toward the wander point. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bludger | Wander")
	float WanderSteeringInterpSpeed = 2.f;

	/** Distance at which a wander point counts as reached and a new one is picked. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bludger | Wander")
	float WanderArrivalRadius = 100.f;

	/** After a hit the Bludger teleports to a random point within this horizontal radius... */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bludger | Reset")
	float ResetRadius = 2000.f;

	/** ...and between half of this and this height above where it was. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bludger | Reset")
	float ResetHeight = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bludger | Ragdoll")
	float RagDollDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bludger | Ragdoll")
	float RagDollImpulseStrength = 150000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bludger | Ragdoll")
	float RespawnDelay = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Targeting")
	float HumanPlayerPriority = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Targeting")
	float GoalProximityWeight = 1.5f;

	/** Carriers closer than this to a goal raise the threat score (0 at this distance, max at the ring). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Targeting")
	float GoalThreatDistance = 5000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Targeting")
	float TargetSwitchThreshold = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Targeting")
	float TargetLockDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Avoidance")
	float ObstacleDetectionRange = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Avoidance")
	float ObstacleAvoidanceStrength = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Avoidance")
	float AvoidanceSphereRadius = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Avoidance")
	int32 NumAvoidanceRays = 5;

	/** Total horizontal spread of the forward avoidance probes, in degrees. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Avoidance")
	float AvoidanceArcDegrees = 60.f;

	/** Draw avoidance probes in the world (development aid). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Debug")
	bool bDrawDebug = false;

	/** Optional Niagara effect spawned where a Bludger hits a broom. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | VFX")
	TObjectPtr<UNiagaraSystem> HitEffect = nullptr;
};
