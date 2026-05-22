// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BallBase.h"
#include "DataAsset/QuaffleDataAsset.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Quaffle.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UProjectileMovementComponent;
struct FTimerHandle;

class QuaffleStateBase
{
public:
	virtual ~QuaffleStateBase() = default;

	virtual bool CanPickUp() const { return false; }
	virtual bool CanThrow() const { return false; }
	virtual bool CanScore() const { return false; }
};

class QuaffleStateFree : public QuaffleStateBase
{
public:
	virtual bool CanPickUp() const override { return true; }
};

class QuaffleStatePossessed : public QuaffleStateBase
{
public:
	virtual bool CanThrow() const override { return true; }
};

class QuaffleStateThrown : public QuaffleStateBase
{
public:
	virtual bool CanScore() const override { return true; }
};

class QuaffleStateScored : public QuaffleStateBase
{
public:

};

UCLASS()
class QUIDPROWIZ_API AQuaffle : public ABallBase
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AQuaffle();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	bool CanBePickedUpBy(AActor* PlayerActor) const;
	bool IsInPickupRange(AActor* PlayerActor) const;
	void PickUp(AActor* PlayerActor);

	bool CanBePickedUp() const { return CurrentState->CanPickUp(); }
	bool CanBeThrown() const { return CurrentState->CanThrow(); }
	bool CanScore() const { return CurrentState->CanScore(); }

	FVector GetPreviousLocation() const { return PreviousLocation; }
	FVector GetLastThrownLocation() const { return LastThrownStartLocation; }

	void NotifyScored();

	bool ThrowToTarget(const FVector& TargetLocation, float ArcBias = 1.f);

	UFUNCTION(BlueprintPure, Category = "Quaffle | State")
	FString GetCurrentStateName() const
	{
		if (CurrentState->CanPickUp()) return TEXT("Free");
		if (CurrentState->CanThrow()) return TEXT("Possessed");
		if (CurrentState->CanScore()) return TEXT("Thrown");
		return TEXT("Scored");
	}

	float GetPickupRadius() const
	{
		return QuaffleData ? QuaffleData->PickupRadius : 200.f;
	}

	void Drop(AActor* DropActor);

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quaffle | Components")
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY()
	AActor* PossessingPlayer = nullptr;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quaffle | Data")
	UQuaffleDataAsset* QuaffleData;

	bool LaunchWithVelocity(const FVector& LaunchVelocity, AActor* PreviousHolder);

private:

	FTransform InitialSpawnTransform;

	//FTimerHandle ThrowRecoveryTimerHandle;

	UPROPERTY()
	AActor* LastThrower = nullptr;

	FVector PreviousLocation = FVector::ZeroVector;
	FVector CurrentLocationCached = FVector::ZeroVector;
	FVector LastThrownStartLocation = FVector::ZeroVector;

	TUniquePtr<QuaffleStateBase> CurrentState;
	void SetState(TUniquePtr<QuaffleStateBase> NewState) { CurrentState = MoveTemp(NewState); }

	FVector ComputeThrowStartLocation(AActor* Holder) const;

private:
	
	float TimeSinceLastPositionBroadcast = 0.f;

	UPROPERTY(EditAnywhere, Category = "Quaffle | Network")
	float PositionBroadcastInterval = 0.016f; // ~60 broadcasts per second
	
	FTimerHandle MissRespawnTimerHandle;

	UPROPERTY(EditAnywhere, Category = "Quaffle | Gameplay")
	float MissRespawnDelay = 3.f;

	void OnMissRespawn();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quaffle | Effects")
	UNiagaraComponent* TrailEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Quaffle | Effects")
	UNiagaraSystem* ThrowTrailSystem = nullptr;

private:

	void ActivateTrail();
	void DeactivateTrail();
};
