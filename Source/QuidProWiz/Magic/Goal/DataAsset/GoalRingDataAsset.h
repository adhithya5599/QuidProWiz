// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GoalRingDataAsset.generated.h"

class UMaterialInterface;
class UNiagaraSystem;

/**
 * Tuning and presentation for goal rings.
 */
UCLASS(Blueprintable)
class QUIDPROWIZ_API UGoalRingDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal | Gameplay")
	int32 ScoreValue = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal | Gameplay")
	float LockOnVolumeExtent = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal | Gameplay")
	float ScoreVolumeExtent = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal | Visual")
	TObjectPtr<UMaterialInterface> NormalMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal | Visual")
	TObjectPtr<UMaterialInterface> HighlightedMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal | Visual")
	TObjectPtr<UMaterialInterface> ScoreFlashMaterial = nullptr;

	/** How long the score flash material stays on after a goal. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal | Visual", meta = (ClampMin = "0"))
	float ScoreFlashDuration = 0.5f;

	/** Pulse speed of the locally highlighted (aim-assist target) ring. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal | Visual", meta = (ClampMin = "0"))
	float HighlightPulseSpeed = 3.f;

	/** Extra scale added at the peak of the highlight pulse. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal | Visual", meta = (ClampMin = "0"))
	float HighlightPulseIntensity = 0.3f;

	/** Optional Niagara effect spawned at the ring when a goal is scored. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal | VFX")
	TObjectPtr<UNiagaraSystem> ScoreEffect = nullptr;
};
