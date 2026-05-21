// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BroomDataAsset.h"
#include "SoundManager.h"
#include "Broom.generated.h"

class UStaticMeshComponent;
class UInputAction;
class UFloatingPawnMovement;
class UCapsuleComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
struct FInputActionValue;
class USkeletalMeshComponent;
class AQuaffle;
class AGoalRing;
class UGoalTargetingComponent;
class UZoneUIManager;

UCLASS()
class QUIDPROWIZ_API ABroom : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABroom();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	//Broom movement component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Broom|Movement")
	UFloatingPawnMovement* BroomMovementComponent;

	//Broom mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Broom|Mesh")
	UStaticMeshComponent* BroomMesh;

	//Broom collision component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Broom|Collision")
	UCapsuleComponent* BroomCollisionComponent;

	//Broom camera boom
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Broom|Camera")
	USpringArmComponent* BroomCameraBoom;

	//Broom camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Broom|Camera")
	UCameraComponent* BroomCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Broom|Data")
	UBroomDataAsset* BroomData;

public:

	//User input variables
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Input")
	UInputAction* BroomMovementAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Input")
	UInputAction* BroomTurnAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Input")
	UInputAction* BroomSpeedBoostAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Input")
	UInputAction* BroomAscendAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Input")
	UInputMappingContext* BroomInputMappingContext;

protected:

	//Input functions
	void Move(const FInputActionValue& Value);
	void Ascend(const FInputActionValue& Value);
	void Turn(const FInputActionValue& Value);
	void SpeedBoost(const FInputActionValue& Value);

	//Broom movement functions
	//void UpdateMovement(float DeltaTime);
	virtual void UpdateHeading(float DeltaTime);
	virtual void UpdateMeshTilt(float DeltaTime);

	float CurrentTurnInput = 0.f;
	float CurrentAscendInput = 0.f;
	float CurrentHeadingYaw = 0.f;

	FRotator InitialMeshRotation = FRotator::ZeroRotator;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Broom|Rider")
	USkeletalMeshComponent* RiderMesh;
	AQuaffle* HeldQuaffle = nullptr;

public:

	UFUNCTION(BlueprintCallable, Category = "Broom|Components")
	USkeletalMeshComponent* GetRiderMesh() const { return RiderMesh; }

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Input")
	UInputAction* QuafflePickupAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Input")
	UInputAction* QuaffleThrowAction;

	void AttemptPickupQuaffle();
	
	void ThrowQuaffle();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Broom|Targeting")
	UGoalTargetingComponent* GoalTargetingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Broom|UI")
	UZoneUIManager* ZoneUIManager;

public:

	UCameraComponent* GetBroomCamera() const { return BroomCamera; }

	void NotifyQuafflePickedUp(AQuaffle* PickedUpQuaffle);
	void NotifyQuaffleReleased();

	bool IsHoldingQuaffle() const { return HeldQuaffle != nullptr; }
	void DropQuaffle();
	void ApplyStun(float Duration, float SpeedMultiplier);
	void RecoverFromStun();

	void TriggerRagdoll(const FVector& ImpulseDirection, float ImpulseStrength, float RagdollDuration,
		float RespawnDelay);

	void PerformMove(float AxisValue);
	void PerformAscend(float AxisValue);
	void PerformTurn(float AxisValue);
	void PerformSpeedBoost(bool bIsActive);
	void PerformPickupQuaffle();
	void PerformThrowQuaffle();

	AQuaffle* FindNearestQuaffle() const;

	AQuaffle* GetHeldQuaffle() const { return HeldQuaffle; }

	UFloatingPawnMovement* GetBroomMovementComponent() const { return BroomMovementComponent; }

	bool CanPickupQuaffle() const { return !bPickupCooldown; }
	void StartPickupCooldown(float Duration);

private:

	bool bIsStunned = false;
	FTimerHandle StunTimerHandle;

	bool bIsRagDolling = false;
	FTimerHandle RagdollTimerHandle;
	FTimerHandle RespawnTimerHandle;
	FTransform RespawnTransform;

	FTransform InitialSpawnTransform;

	bool bPickupCooldown = false;
	FTimerHandle PickupCooldownTimerHandle;
	void ResetPickupCooldown() { bPickupCooldown = false; }

	void DetachRider(const FVector& ImpulseDirection, float ImpulseStrength);
	void Respawn();
	
	bool IsMatchInProgress() const;

public:

	UPROPERTY(EditDefaultsOnly, Category = "Broom|Camera")
	TSubclassOf<UCameraShakeBase> BludgerHitShakeClass;

	UPROPERTY(EditDefaultsOnly, Category = "Broom|Camera")
	TSubclassOf<UCameraShakeBase> GoalScoredShakeClass;

	void TriggerBludgerHitShake();
	void TriggerGoalScoredShake();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Broom | Sound")
	USoundManager* SoundManager;

public:

	USoundManager* GetSoundManager() const { return SoundManager; }
};
