// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BroomDataAsset.generated.h"

/**
 * Flight, steering, look and camera tuning for a broom (player or AI).
 *
 * The flight model is momentum based: the broom flies where it is aimed, its velocity swings
 * toward the aim at a limited turn rate (so turns carve instead of snapping), and it coasts when
 * no throttle is held. Every value is read live, so the gym sliders apply immediately.
 */
UCLASS(Blueprintable)
class QUIDPROWIZ_API UBroomDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	// --- Flight ---------------------------------------------------------------------------------

	/** Top speed in cm/s with full throttle (before boost). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Flight", meta = (ClampMin = "0"))
	float Speed = 1100.f;

	/** How fast the broom gains speed, as a multiple of Speed per second. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Flight", meta = (ClampMin = "0"))
	float AccelerationMultiplier = 1.6f;

	/** How fast holding reverse throttle slows the broom, as a multiple of Speed per second. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Flight", meta = (ClampMin = "0"))
	float BrakeMultiplier = 2.5f;

	/** How fast the broom coasts to a stop with no throttle, as a multiple of Speed per second. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Flight", meta = (ClampMin = "0"))
	float CoastMultiplier = 0.6f;

	/** Backing up is allowed at this fraction of Speed once the broom has stopped. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Flight", meta = (ClampMin = "0", ClampMax = "1"))
	float ReverseSpeedFraction = 0.35f;

	/** How fast the flight direction swings toward the aim when slow (deg/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Flight", meta = (ClampMin = "1"))
	float TurnRateSlow = 300.f;

	/** How fast the flight direction swings toward the aim at top speed (deg/s). Lower = wider arcs. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Flight", meta = (ClampMin = "1"))
	float TurnRateFast = 170.f;

	/** Turn rate is multiplied by this while boosting (wider, faster arcs). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Flight", meta = (ClampMin = "0.05"))
	float BoostTurnRateScale = 0.75f;

	/** Strength of the climb/dive keys, as a fraction of full throttle. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Flight", meta = (ClampMin = "0", ClampMax = "1"))
	float VerticalInputStrength = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Flight", meta = (ClampMin = "1"))
	float SpeedBoostMultiplier = 1.9f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Flight", meta = (ClampMin = "0"))
	float SpeedBoostAccelerationMultiplier = 2.5f;

	// --- Steering -------------------------------------------------------------------------------

	/** Keyboard turning (A/D) rotates the aim at up to this many degrees per second. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Steering", meta = (ClampMin = "0"))
	float TurnSpeed = 130.f;

	/** How quickly keyboard turning ramps up and down (higher = snappier). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Steering", meta = (ClampMin = "0"))
	float TurnInputSmoothing = 8.f;

	/** Mouse / stick look sensitivity multiplier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Steering", meta = (ClampMin = "0"))
	float LookSensitivity = 1.f;

	/** The aim can point this far up or down (degrees). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Steering", meta = (ClampMin = "0", ClampMax = "89"))
	float MaxAimPitch = 70.f;

	/** How fast the camera swings back behind the broom when free look ends. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Steering", meta = (ClampMin = "0"))
	float FreeLookReturnSpeed = 8.f;

	// --- Look and feel --------------------------------------------------------------------------

	/** Roll into turns (degrees at full turn rate). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Visual")
	float BankAngle = 35.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Visual", meta = (ClampMin = "0"))
	float BankInterpSpeed = 6.f;

	/** The broom's nose follows the climb/dive angle up to this many degrees. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Visual")
	float AscendTiltAngle = 35.f;

	// --- Camera ---------------------------------------------------------------------------------

	/** Camera distance behind the broom when still. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Camera", meta = (ClampMin = "0"))
	float CameraDistance = 420.f;

	/** Camera distance at top boosted speed (pulls back to sell speed). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Camera", meta = (ClampMin = "0"))
	float CameraDistanceAtSpeed = 560.f;

	/** Camera height above the rider. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Camera")
	float CameraHeight = 70.f;

	/** Camera position follow speed (higher = tighter, 0 = rigid). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Camera", meta = (ClampMin = "0"))
	float CameraLagSpeed = 10.f;

	/** Camera rotation follow speed (higher = tighter, 0 = rigid). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Camera", meta = (ClampMin = "0"))
	float CameraRotationLagSpeed = 16.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Camera", meta = (ClampMin = "5", ClampMax = "170"))
	float CameraFOV = 85.f;

	/** Field of view while boosting at speed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Camera", meta = (ClampMin = "5", ClampMax = "170"))
	float CameraBoostFOV = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Camera", meta = (ClampMin = "0"))
	float CameraFOVInterpSpeed = 4.f;

	// --- Feedback -------------------------------------------------------------------------------

	/** The local player gets a warning when any Bludger is closer than this. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Feedback")
	float BludgerWarningDistance = 800.f;
};
