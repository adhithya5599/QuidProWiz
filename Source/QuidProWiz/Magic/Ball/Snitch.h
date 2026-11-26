// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BallBase.h"
#include "Snitch.generated.h"

class USnitchDataAsset;
class ABroom;

/** Replicated lifecycle of the Snitch. */
UENUM(BlueprintType)
enum class ESnitchState : uint8
{
	Dormant	UMETA(DisplayName = "Dormant"),
	Active	UMETA(DisplayName = "Active"),
	Caught	UMETA(DisplayName = "Caught")
};

/**
 * The match-ending ball. It starts dormant (hidden, no collision) until the GameState releases
 * it, then flies erratically around the arena, darting sideways and fleeing nearby brooms.
 * Touching it with a broom that isn't stunned catches it; the catch is reported through
 * UQuidditchWorldSubsystem::OnSnitchCaught and the GameState decides the match.
 *
 * All behaviour runs on the server; state and movement replicate.
 */
UCLASS()
class QUIDPROWIZ_API ASnitch : public ABallBase
{
	GENERATED_BODY()

public:

	ASnitch();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server: wake the Snitch up and let it fly around ArenaCenter. */
	void Release(const FVector& ArenaCenter);

	/** Server: hide it again at its spawn point, ready for another Release. */
	virtual void ResetToSpawn() override;

	UFUNCTION(BlueprintPure, Category = "Snitch")
	ESnitchState GetSnitchState() const { return SnitchState; }

	bool IsActive() const { return SnitchState == ESnitchState::Active; }

	/** False for practice Snitches: the match ignores their catches and never releases them. */
	bool EndsMatchWhenCaught() const { return bEndsMatchWhenCaught; }
	void SetEndsMatchWhenCaught(bool bInEndsMatch) { bEndsMatchWhenCaught = bInEndsMatch; }

	USnitchDataAsset* GetSnitchData() const { return SnitchData; }

	/** Swap the tuning asset (e.g. a smaller practice arena). Takes effect immediately. */
	void SetSnitchData(USnitchDataAsset* InData);

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Optional; without one the Snitch uses the data asset class defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Snitch | Data")
	TObjectPtr<USnitchDataAsset> SnitchData;

	/** Catching this Snitch decides the match. Turn off for gym/practice Snitches. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Snitch")
	bool bEndsMatchWhenCaught = true;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:

	UPROPERTY(ReplicatedUsing = OnRep_SnitchState)
	ESnitchState SnitchState = ESnitchState::Dormant;

	FVector ArenaCenter = FVector::ZeroVector;
	FVector WanderTarget = FVector::ZeroVector;
	FVector CurrentVelocity = FVector::ZeroVector;
	FVector JinkVelocity = FVector::ZeroVector;
	float TimeUntilJink = 0.f;

	const USnitchDataAsset& GetData() const;

	UFUNCTION()
	void OnRep_SnitchState();

	/** Visibility, collision and ticking for the current state (every machine). */
	void ApplyStateEffects();

	void SetSnitchState(ESnitchState NewState);
	void TryCatch(ABroom* Broom);
	void PickWanderTarget();
	FVector ComputeDesiredVelocity() const;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayEffect(bool bCaught);
};
