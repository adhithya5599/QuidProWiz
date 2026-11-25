// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "QuaffleDataAsset.generated.h"

class UNiagaraSystem;

/**
 * Tuning and presentation for the Quaffle.
 */
UCLASS(Blueprintable)
class QUIDPROWIZ_API UQuaffleDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Gameplay")
	float PickupRadius = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Gameplay")
	float MinThrowArcTime = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Gameplay")
	float MaxThrowArcTime = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Gameplay")
	float ThrowStartForwardOffset = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Gameplay")
	float ThrowStartVerticalOffset = 30.f;

	/** Top speed of a thrown Quaffle; also sets the flight time used by the throw solver. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Gameplay", meta = (ClampMin = "1"))
	float ThrowMaxSpeed = 3000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Gameplay", meta = (ClampMin = "0"))
	float ThrowGravityScale = 0.6f;

	/** Seconds after a missed throw before the Quaffle resets to its spawn point. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Gameplay", meta = (ClampMin = "0"))
	float MissRespawnDelay = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Attachment")
	FName SocketName = TEXT("QuaffleSocket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Attachment")
	FVector HeldOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Attachment")
	FRotator HeldRotationOffset = FRotator::ZeroRotator;

	/** Optional Niagara trail shown while the Quaffle is in flight. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|VFX")
	TObjectPtr<UNiagaraSystem> ThrowTrailEffect = nullptr;
};
