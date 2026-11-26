// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TextRenderComponent.h"
#include "GymFacingTextComponent.generated.h"

/**
 * World-space text that turns (around Z) to face the local player's camera, so gym signs read
 * correctly from any direction instead of appearing mirrored from behind.
 */
UCLASS(ClassGroup = (Gym), meta = (BlueprintSpawnableComponent))
class QUIDPROWIZ_API UGymFacingTextComponent : public UTextRenderComponent
{
	GENERATED_BODY()

public:

	UGymFacingTextComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Only turn around Z (signs stay upright). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facing")
	bool bYawOnly = true;
};
