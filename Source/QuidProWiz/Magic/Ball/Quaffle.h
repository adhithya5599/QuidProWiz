// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BallBase.h"
#include "Quaffle.generated.h"

class UProjectileMovementComponent;
class UNiagaraComponent;
class UQuaffleDataAsset;

/** Replicated key for the Quaffle's state. */
UENUM(BlueprintType)
enum class EQuaffleState : uint8
{
	Free		UMETA(DisplayName = "Free"),
	Possessed	UMETA(DisplayName = "Possessed"),
	Thrown		UMETA(DisplayName = "Thrown")
};

/**
 * State pattern: each state answers what the Quaffle may do. States are stateless, so a single
 * shared instance per state is used (flyweight) and the replicated EQuaffleState selects it.
 * Add a state (Pass, Steal...) by adding an enum value and a class here.
 */
class QUIDPROWIZ_API FQuaffleStateBase
{
public:
	virtual ~FQuaffleStateBase() = default;

	virtual bool CanPickUp() const { return false; }
	virtual bool CanThrow() const { return false; }
	virtual bool CanScore() const { return false; }
	virtual const TCHAR* GetName() const = 0;

	static const FQuaffleStateBase& Get(EQuaffleState State);
};

class FQuaffleStateFree : public FQuaffleStateBase
{
public:
	virtual bool CanPickUp() const override { return true; }
	virtual const TCHAR* GetName() const override { return TEXT("Free"); }
};

class FQuaffleStatePossessed : public FQuaffleStateBase
{
public:
	virtual bool CanThrow() const override { return true; }
	virtual const TCHAR* GetName() const override { return TEXT("Possessed"); }
};

class FQuaffleStateThrown : public FQuaffleStateBase
{
public:
	virtual bool CanScore() const override { return true; }
	virtual const TCHAR* GetName() const override { return TEXT("Thrown"); }
};

/**
 * The scoring ball. Server-authoritative: only the server changes state, attaches, throws or
 * resets it. Movement and attachment replicate; clients react to state changes in OnRep.
 */
UCLASS()
class QUIDPROWIZ_API AQuaffle : public ABallBase
{
	GENERATED_BODY()

public:

	AQuaffle();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool CanBePickedUpBy(const AActor* PlayerActor) const;
	bool IsInPickupRange(const AActor* PlayerActor) const;

	bool CanBePickedUp() const { return GetState().CanPickUp(); }
	bool CanBeThrown() const { return GetState().CanThrow(); }
	bool CanScore() const { return GetState().CanScore(); }

	EQuaffleState GetQuaffleState() const { return QuaffleState; }
	AActor* GetPossessingPlayer() const { return PossessingPlayer; }
	FVector GetLastThrownLocation() const { return LastThrownStartLocation; }
	float GetPickupRadius() const;

	UFUNCTION(BlueprintPure, Category = "Quaffle | State")
	FString GetCurrentStateName() const { return GetState().GetName(); }

	/** Server: attach to the player's rider socket. */
	void PickUp(AActor* PlayerActor);

	/** Server: detach in place and become free (e.g. the holder was hit). */
	void Drop(AActor* DropActor);

	/** Server: solve a ballistic arc to TargetLocation and launch. ArcBias scales flight time. */
	bool ThrowToTarget(const FVector& TargetLocation, float ArcBias = 1.f);

	/** Server: called by a goal ring when this Quaffle scores. */
	void NotifyScored();

	/** Server: back to the spawn point, free to pick up (a holder drops it first). */
	virtual void ResetToSpawn() override;

	UQuaffleDataAsset* GetQuaffleData() const { return QuaffleData; }

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quaffle | Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quaffle | Effects")
	TObjectPtr<UNiagaraComponent> TrailEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quaffle | Data")
	TObjectPtr<UQuaffleDataAsset> QuaffleData;

private:

	UPROPERTY(ReplicatedUsing = OnRep_QuaffleState)
	EQuaffleState QuaffleState = EQuaffleState::Free;

	UPROPERTY(Replicated)
	TObjectPtr<AActor> PossessingPlayer = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> LastThrower = nullptr;

	FVector LastThrownStartLocation = FVector::ZeroVector;
	FTimerHandle MissRespawnTimerHandle;

	const FQuaffleStateBase& GetState() const { return FQuaffleStateBase::Get(QuaffleState); }

	/** Server: change state and apply its local side effects. */
	void SetQuaffleState(EQuaffleState NewState);

	UFUNCTION()
	void OnRep_QuaffleState();

	/** Collision, trail and physics setup that every machine applies for the current state. */
	void ApplyStateEffects();

	FVector ComputeThrowStartLocation(const AActor* Holder) const;
	bool LaunchWithVelocity(const FVector& LaunchVelocity, AActor* PreviousHolder, const FVector& StartLocation);
	void StopProjectile();
	void OnMissRespawn();

	/** Copies throw speed and gravity from the data asset onto the projectile movement. */
	void ApplyFlightTuning();
};
