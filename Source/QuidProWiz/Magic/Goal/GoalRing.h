// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QuidditchTeam.h"
#include "GoalRing.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USceneComponent;
class UGoalRingDataAsset;
class AQuaffle;

/**
 * A scoring ring. Scoring is decided on the server only; the result is published through
 * UQuidditchWorldSubsystem::OnGoalScored and the feedback is multicast to every client.
 */
UCLASS()
class QUIDPROWIZ_API AGoalRing : public AActor
{
	GENERATED_BODY()

public:

	AGoalRing();

	virtual void Tick(float DeltaTime) override;

	/** World-space point throws should aim at (just in front of the ring). */
	FVector GetAimDirection() const;

	bool IsGoalEnabled() const { return bGoalEnabled; }

	/** Team that defends this ring. */
	EGoalTeam GetOwningTeam() const { return OwningTeam; }

	/** Team credited when the Quaffle passes through this ring: the attackers. */
	EGoalTeam GetScoringTeam() const { return GetOpposingTeam(OwningTeam); }

	int32 GetScoreOverride() const { return ScoreOverride; }

	/** Points this ring is worth: the per-instance override, else the data asset value. */
	int32 GetScoreValue() const;

	/** Local-only cosmetic highlight used by aim assist. */
	void SetLocallyHighlighted(bool bHighlighted);

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal | Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal | Components")
	TObjectPtr<UStaticMeshComponent> RingMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal | Components")
	TObjectPtr<UBoxComponent> LockOnVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal | Components")
	TObjectPtr<UBoxComponent> ScoreVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal | Components")
	TObjectPtr<USceneComponent> AimDirectionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal | Gameplay")
	bool bGoalEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal | Data")
	TObjectPtr<UGoalRingDataAsset> GoalDataAsset = nullptr;

	/** Team that defends this ring; goals through it score for the other team. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal | Gameplay")
	EGoalTeam OwningTeam = EGoalTeam::TeamA;

	/** When > 0, replaces the data asset's ScoreValue for this ring (e.g. bonus rings in the gym). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal | Gameplay")
	int32 ScoreOverride = 0;

	UFUNCTION()
	void OnScoreVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	bool IsValidGoalScore(const AQuaffle* Quaffle) const;
	void RegisterScore(AQuaffle* Quaffle);

private:

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayScoreFeedback();

	void UpdatePulse(float DeltaTime);
	void ResetAfterScoreFlash();
	void ApplyIdleMaterial();

	bool bLocallyHighlighted = false;
	float PulseTimer = 0.f;
	FTimerHandle ScoreFlashTimerHandle;
};
