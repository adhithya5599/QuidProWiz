// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BallBase.h"
#include "Bludger.generated.h"

/**
 * 
 */

class UBludgerDataAsset;
class ABroom;
class AQuaffle;

UCLASS()
class QUIDPROWIZ_API ABludger : public ABallBase
{
	GENERATED_BODY()

public:

	ABludger();

protected:

	virtual void BeginPlay() override;

public:

	virtual void Tick(float DeltaTime) override;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bludger | Data")
	UBludgerDataAsset* BludgerDataAsset;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:

	UPROPERTY()
	ABroom* TargetBroom = nullptr;

	UPROPERTY()
	AQuaffle* QuaffleRef = nullptr;

	FTimerHandle SearchTimerHandle;

	FVector CurrentVelocity = FVector::ZeroVector;
	FVector WanderTarget = FVector::ZeroVector;
	FTimerHandle WanderTimerHandle;
	bool bIsWandering = false;

	bool bCanHit = true;
	FTimerHandle HitCooldownTimerHandle;

	void ResetHitCoolDown();

	void SerchForTarget();
	void ChaseTarget(float DeltaTime);
	void OnHitBroom(ABroom* HitBroom);

	bool IsTargetValid() const;

	void PickNewWanderTarget();
	void ChaseWanderTarget(float DeltaTime);

	void ResetToRandomLocation();

	UPROPERTY(EditAnywhere, Category = "Bludger | Gameplay")
	float ResetRadius = 2000.f;

	UPROPERTY(EditAnywhere, Category = "Bludger | Gameplay")
	float ResetHeight = 600.f;

	float TargetLockTimer = 0.f;
	bool bTargetLocked = false;

	float ScoreTarget(ABroom* Broom) const;
	bool ShouldSwitchTarget(ABroom* NewTarget) const;
};
