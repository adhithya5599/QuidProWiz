// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/GymDataAsset.h"
#include "GymResetPad.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UGymFacingTextComponent;

/**
 * A floor pad that runs a gym command when a broom flies through it (reset the Quaffles,
 * the Bludgers, the practice Snitch, or restart the level). Replaces the level-Blueprint
 * trigger boxes; the command itself lives in UGymSubsystem::ExecuteAction.
 */
UCLASS()
class QUIDPROWIZ_API AGymResetPad : public AActor
{
	GENERATED_BODY()

public:

	AGymResetPad();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reset Pad")
	EGymAction Action = EGymAction::ResetQuaffles;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reset Pad")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reset Pad")
	FLinearColor LabelColor = FLinearColor::White;

	/** Seconds before the pad can fire again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reset Pad", meta = (ClampMin = "0"))
	float Cooldown = 2.f;

	UPROPERTY(VisibleAnywhere, Category = "Reset Pad")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, Category = "Reset Pad")
	TObjectPtr<UStaticMeshComponent> PadMesh;

	UPROPERTY(VisibleAnywhere, Category = "Reset Pad")
	TObjectPtr<UGymFacingTextComponent> LabelText;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:

	double LastFiredTime = -1000.0;
};
