// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SnitchDataAsset.generated.h"

class UNiagaraSystem;

/**
 * Flight and presentation tuning for the Snitch. Release and win rules live on the GameState.
 */
UCLASS(BlueprintType)
class QUIDPROWIZ_API USnitchDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Speed while wandering with nobody close. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Snitch | Flight", meta = (ClampMin = "0"))
	float CruiseSpeed = 700.f;

	/** Speed while fleeing a nearby broom. Above a broom's normal speed, below its boost. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Snitch | Flight", meta = (ClampMin = "0"))
	float EvadeSpeed = 1400.f;

	/** How quickly velocity turns toward the desired direction. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Snitch | Flight", meta = (ClampMin = "0"))
	float SteeringInterpSpeed = 3.f;

	/** A broom closer than this makes the Snitch flee. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Snitch | Flight", meta = (ClampMin = "0"))
	float EvadeRadius = 1200.f;

	/** Seconds between random sideways darts (picked between min and max). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Snitch | Flight", meta = (ClampMin = "0.05"))
	float JinkIntervalMin = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Snitch | Flight", meta = (ClampMin = "0.05"))
	float JinkIntervalMax = 1.6f;

	/** Sideways speed added by each dart. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Snitch | Flight", meta = (ClampMin = "0"))
	float JinkStrength = 900.f;

	/** The Snitch stays within this horizontal distance of the arena centre (the middle of the goal rings). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Snitch | Arena", meta = (ClampMin = "100"))
	float ArenaRadius = 3500.f;

	/** Wander height range relative to the arena centre. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Snitch | Arena")
	float MinHeightOffset = -200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Snitch | Arena")
	float MaxHeightOffset = 1200.f;

	/** Radius of the catch sphere; touching it with a broom catches the Snitch. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Snitch | Catch", meta = (ClampMin = "1"))
	float CatchRadius = 60.f;

	/** Optional Niagara effect when the Snitch is released. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Snitch | VFX")
	TObjectPtr<UNiagaraSystem> ReleaseEffect = nullptr;

	/** Optional Niagara effect when the Snitch is caught. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Snitch | VFX")
	TObjectPtr<UNiagaraSystem> CaughtEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Snitch | Debug")
	bool bDrawDebug = false;
};
