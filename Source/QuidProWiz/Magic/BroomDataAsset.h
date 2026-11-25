// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BroomDataAsset.generated.h"

/**
 * Flight tuning for a broom (player or AI).
 */
UCLASS(Blueprintable)
class QUIDPROWIZ_API UBroomDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement")
	float Speed = 1000.f;

	/** Acceleration and deceleration as a multiple of Speed when not boosting. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement", meta = (ClampMin = "0"))
	float AccelerationMultiplier = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement")
	float TurnSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement")
	float BankAngle = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement")
	float BankInterpSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement")
	float AscendTiltAngle = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement")
	float HeadingInterpSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement")
	float SpeedBoostMultiplier = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Movement")
	float SpeedBoostAccelerationMultiplier = 4.f;

	/** The local player gets a warning when any Bludger is closer than this. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Feedback")
	float BludgerWarningDistance = 800.f;

	/**
	 * Network (interim client-authoritative sync): how often the owning client sends its
	 * transform to the server, and how far it may move per second before the server rejects it.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Network", meta = (ClampMin = "1"))
	float ClientMoveSyncRate = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Network", meta = (ClampMin = "0"))
	float MaxClientMoveSpeedTolerance = 1.5f;
};
