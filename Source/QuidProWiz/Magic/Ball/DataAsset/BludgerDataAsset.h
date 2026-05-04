// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BludgerDataAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class QUIDPROWIZ_API UBludgerDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Gameplay")
	float ChaseSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Gameplay")
	float HitRadius = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Gameplay")
	float SearchInterval = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Gameplay")
	float KnockbackForce = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Gameplay")
	float ChaseAcceleration = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Gameplay")
	float BoostSpeedMultiplier = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Gameplay")
	float BoostActiveDistance = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Gameplay")
	float RotationInterpSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Gameplay")
	float HitCooldown = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Visual")
	float StunDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Visual")
	float StunSpeedMultiplier = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Gameplay")
	float WanderSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Gameplay")
	float WanderRadius = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Gameplay")
	float WanderInterval = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Ragdoll")
	float RagDollDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Ragdoll")
	float RagDollImpulseStrength = 150000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Ragdoll")
	float RespawnDelay = 3.f;
};
