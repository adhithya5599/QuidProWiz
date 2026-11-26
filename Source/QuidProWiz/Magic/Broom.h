// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "QuidditchTeam.h"
#include "Broom.generated.h"

class UStaticMeshComponent;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
struct FInputActionValue;
class USkeletalMeshComponent;
class UCameraShakeBase;
class UBroomDataAsset;
class UBroomMovementComponent;
class USoundManager;
class AQuaffle;
class AGoalRing;
class UGoalTargetingComponent;
class UZoneUIManager;

/**
 * The flying character used by players and AI.
 *
 * Steering: the controller's rotation is the aim. Mouse/stick look and the A/D keys rotate it,
 * the camera looks along it, and throttle flies the broom toward it (pitch included, so looking
 * up climbs and looking down dives). Holding free look lets the camera look around while the
 * broom keeps its course. AI controllers steer the same way by rotating their control rotation.
 *
 * Movement is client-predicted by UBroomMovementComponent (momentum flight in the Character
 * Movement Component's flying mode; see its comment). Both input and AI controllers drive the
 * broom through the same Perform* API, so every rule (stun, match phase, cooldowns) applies to
 * both. Gameplay outcomes (pickup, throw, hits, stun) are decided on the server via Server RPCs.
 *
 * Visual components hang off VisualPivot under the Character's mesh component: the movement
 * component smooths the mesh on remote machines, and the pivot banks and pitches with the
 * motion, so the rider leans with the broom.
 */
UCLASS()
class QUIDPROWIZ_API ABroom : public ACharacter
{
	GENERATED_BODY()

public:

	ABroom(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Banks and pitches with the motion; the broom and rider meshes ride on it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Broom|Mesh")
	TObjectPtr<USceneComponent> VisualPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Broom|Mesh")
	TObjectPtr<UStaticMeshComponent> BroomMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Broom|Camera")
	TObjectPtr<USpringArmComponent> BroomCameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Broom|Camera")
	TObjectPtr<UCameraComponent> BroomCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Broom|Rider")
	TObjectPtr<USkeletalMeshComponent> RiderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Broom|Targeting")
	TObjectPtr<UGoalTargetingComponent> GoalTargetingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Broom|UI")
	TObjectPtr<UZoneUIManager> ZoneUIManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Broom|Sound")
	TObjectPtr<USoundManager> SoundManager;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Broom|Data")
	TObjectPtr<UBroomDataAsset> BroomData;

	/** Side this broom plays for. Players default to Team A; AI controllers assign their own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Broom|Team")
	EGoalTeam Team = EGoalTeam::TeamA;

public:

	// --- Input assets -------------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Input")
	TObjectPtr<UInputAction> BroomMovementAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Input")
	TObjectPtr<UInputAction> BroomTurnAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Input")
	TObjectPtr<UInputAction> BroomSpeedBoostAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Input")
	TObjectPtr<UInputAction> BroomAscendAction;

	/** Axis2D: mouse / right stick aiming. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Input")
	TObjectPtr<UInputAction> BroomLookAction;

	/** Held: look around without changing course. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Input")
	TObjectPtr<UInputAction> BroomFreeLookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Input")
	TObjectPtr<UInputMappingContext> BroomInputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Broom|Camera")
	TSubclassOf<UCameraShakeBase> BludgerHitShakeClass;

	UPROPERTY(EditDefaultsOnly, Category = "Broom|Camera")
	TSubclassOf<UCameraShakeBase> GoalScoredShakeClass;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Input")
	TObjectPtr<UInputAction> QuafflePickupAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Broom|Input")
	TObjectPtr<UInputAction> QuaffleThrowAction;

public:

	// --- Action API shared by player input and AI ---------------------------------------------

	/** Throttle toward the aim: 1 = full ahead, negative = brake / back up. */
	void PerformMove(float AxisValue);

	/** Climb (+) or dive (-) straight up/down on top of the throttle. */
	void PerformAscend(float AxisValue);

	/** Keyboard turning: rotates the aim left (-) / right (+) at the data asset's turn speed. */
	void PerformTurn(float AxisValue);

	/** Mouse / stick aiming (X = yaw, Y = pitch, already scaled by the input mapping). */
	void PerformLook(const FVector2D& LookAxis);

	/** While true the camera looks around and the broom keeps its course. */
	void PerformFreeLook(bool bActive);

	void PerformSpeedBoost(bool bIsActive);

	/** Where the broom is steering: the controller's rotation, or the held course during free look. */
	FRotator GetAimRotation() const;

	bool IsFreeLooking() const { return bFreeLook; }

	/** Picks up the nearest free Quaffle in range (asks the server when called on a client). */
	void PerformPickupQuaffle();

	/** Throws at the aim-assist target goal. */
	void PerformThrowQuaffle();

	/** Throws the held Quaffle at Goal. ArcBias scales flight time (AI uses it for inaccuracy). */
	void ThrowQuaffleAtGoal(AGoalRing* Goal, float ArcBias = 1.f);

	/** AI speed cap; <= 0 returns to the data asset speed. */
	void SetSpeedOverride(float NewSpeedOverride) { SpeedOverride = NewSpeedOverride; }

	// --- Server-side gameplay -----------------------------------------------------------------

	void DropQuaffle();
	void ApplyStun(float Duration, float SpeedMultiplier);
	void RecoverFromStun();
	void TriggerRagdoll(const FVector& ImpulseDirection, float ImpulseStrength, float RagdollDuration,
		float RespawnDelay);
	void StartPickupCooldown(float Duration);

	/** Server: assign the broom to a team (replicates). */
	void SetTeam(EGoalTeam NewTeam);

	// --- Queries ------------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Broom|Team")
	EGoalTeam GetTeam() const { return Team; }

	UFUNCTION(BlueprintCallable, Category = "Broom|Components")
	USkeletalMeshComponent* GetRiderMesh() const { return RiderMesh; }

	UFUNCTION(BlueprintPure, Category = "Broom|Movement")
	UBroomMovementComponent* GetBroomMovement() const;

	/** Current flight speed in cm/s. */
	UFUNCTION(BlueprintPure, Category = "Broom|Movement")
	float GetCurrentSpeed() const;

	/** Current speed limit after AI override, boost and stun. */
	UFUNCTION(BlueprintPure, Category = "Broom|Movement")
	float GetCurrentMaxSpeed() const;

	UBroomDataAsset* GetBroomData() const { return BroomData; }
	USceneComponent* GetVisualPivot() const { return VisualPivot; }
	USpringArmComponent* GetCameraBoom() const { return BroomCameraBoom; }
	UGoalTargetingComponent* GetGoalTargeting() const { return GoalTargetingComponent; }
	UZoneUIManager* GetZoneUI() const { return ZoneUIManager; }
	UCameraComponent* GetBroomCamera() const { return BroomCamera; }
	USoundManager* GetSoundManager() const { return SoundManager; }
	AQuaffle* GetHeldQuaffle() const { return HeldQuaffle; }
	bool IsHoldingQuaffle() const { return HeldQuaffle != nullptr; }
	bool IsStunned() const { return bIsStunned; }
	float GetStunSpeedMultiplier() const { return StunSpeedMultiplier; }
	float GetSpeedOverride() const { return SpeedOverride; }
	bool CanPickupQuaffle() const { return !bPickupCooldown; }

	void TriggerBludgerHitShake();
	void TriggerGoalScoredShake();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStunChanged, bool, bIsStunned);

	UPROPERTY(BlueprintAssignable, Category = "Broom | Effects")
	FOnStunChanged OnStunChanged;

	UFUNCTION(BlueprintCallable, Category = "Broom | Effects")
	bool IsBludgerWarningActive() const { return bBludgerWarningActive; }

protected:

	// Enhanced Input handlers
	void Move(const FInputActionValue& Value);
	void Ascend(const FInputActionValue& Value);
	void Turn(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void FreeLook(const FInputActionValue& Value);
	void SpeedBoost(const FInputActionValue& Value);
	void AttemptPickupQuaffle();
	void ThrowQuaffle();

	/** Local: smoothed keyboard turning and the camera's swing back after free look. */
	virtual void UpdateSteering(float DeltaTime);

	/** Local: camera distance, lag and field of view from speed and boost. */
	virtual void UpdateCamera(float DeltaTime);

	/** Cosmetic bank/pitch derived from observed motion, so it looks the same on every machine. */
	virtual void UpdateVisualTilt(float DeltaTime);

private:

	// --- Replicated state ---------------------------------------------------------------------

	UPROPERTY(Replicated)
	TObjectPtr<AQuaffle> HeldQuaffle = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_IsStunned)
	bool bIsStunned = false;

	UPROPERTY(Replicated)
	float StunSpeedMultiplier = 1.f;

	UFUNCTION()
	void OnRep_IsStunned();

	// --- RPCs ---------------------------------------------------------------------------------

	UFUNCTION(Server, Reliable)
	void ServerPickupQuaffle();

	UFUNCTION(Server, Reliable)
	void ServerThrowQuaffleAtGoal(AGoalRing* Goal, float ArcBias);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayQuaffleSound(bool bThrow);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBeginRagdoll(FVector_NetQuantizeNormal ImpulseDirection, float ImpulseStrength, float RagdollDuration);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFinishRespawn();

	// --- Local state --------------------------------------------------------------------------

	float TargetTurnInput = 0.f;
	float CurrentTurnInput = 0.f;
	float CurrentAscendInput = 0.f;
	float LastVisualYaw = 0.f;
	FTransform InitialRiderTransform;

	/** Course held while free looking (and while the camera swings back afterwards). */
	FRotator HeldAim = FRotator::ZeroRotator;
	bool bFreeLook = false;
	bool bReturningFromFreeLook = false;

	float SpeedOverride = 0.f;

	bool bIsRagdolling = false;
	FTransform RespawnTransform;
	FTimerHandle StunTimerHandle;
	FTimerHandle RagdollTimerHandle;
	FTimerHandle RespawnTimerHandle;

	bool bPickupCooldown = false;
	FTimerHandle PickupCooldownTimerHandle;
	float LastPickupRequestTime = -1.f;

	bool bBludgerWarningActive = false;

	void ResetPickupCooldown() { bPickupCooldown = false; }
	void HandleRagdollTimeout();
	void Respawn();
	bool IsMatchInProgress() const;
	void UpdateBludgerWarning();
};
