// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BallBase.h"
#include "Bludger.generated.h"

class UBludgerDataAsset;
class ABroom;

/**
 * Hunts the Quaffle carrier with steering behaviours (seek / wander / avoid). All behaviour
 * runs on the server; movement replicates. Any number of Bludgers can exist.
 */
UCLASS()
class QUIDPROWIZ_API ABludger : public ABallBase
{
	GENERATED_BODY()

public:

	ABludger();

	virtual void Tick(float DeltaTime) override;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Data")
	TObjectPtr<UBludgerDataAsset> BludgerDataAsset;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:

	UPROPERTY()
	TObjectPtr<ABroom> TargetBroom = nullptr;

	FVector CurrentVelocity = FVector::ZeroVector;
	FVector WanderTarget = FVector::ZeroVector;

	float TargetLockTimer = 0.f;
	bool bTargetLocked = false;
	bool bCanHit = true;

	FTimerHandle SearchTimerHandle;
	FTimerHandle HitCooldownTimerHandle;

	void SearchForTarget();
	bool IsTargetValid() const;
	float ScoreTarget(const ABroom* Broom) const;
	bool ShouldSwitchTarget(const ABroom* NewTarget) const;

	void ChaseTarget(float DeltaTime);
	void ChaseWanderTarget(float DeltaTime);
	void PickNewWanderTarget();

	/** Applies velocity with a sweep, slides off blocking hits and turns to face travel. */
	void MoveWithVelocity(float DeltaTime, bool bRepickWanderOnHit);

	FVector ComputeAvoidanceForce() const;
	FVector ComputeSeekForce(const FVector& TargetLocation) const;
	FVector ComputeWanderForce() const;

	void OnHitBroom(ABroom* HitBroom);
	void ResetToRandomLocation();
	void ResetHitCooldown() { bCanHit = true; }

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitEffect(FVector_NetQuantize Location);
};
