// Fill out your copyright notice in the Description page of Project Settings.

#include "Broom.h"
#include "BroomDataAsset.h"
#include "SoundManager.h"
#include "Quaffle.h"
#include "Bludger.h"
#include "GoalRing.h"
#include "GoalTargetingComponent.h"
#include "QuidditchWorldSubsystem.h"
#include "QuidProWizGameStateBase.h"
#include "UI/ZoneUIManager.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace BroomConstants
{
	/** Minimum seconds between client pickup requests while the pickup input is held. */
	constexpr float PickupRequestInterval = 0.2f;
}

ABroom::ABroom()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicatingMovement(true);

	BroomCollisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BroomCollisionComponent"));
	BroomCollisionComponent->InitCapsuleSize(42.f, 96.0f);
	BroomCollisionComponent->SetGenerateOverlapEvents(true);
	RootComponent = BroomCollisionComponent;

	BroomMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BroomMesh"));
	BroomMesh->SetupAttachment(RootComponent);

	BroomCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("BroomCameraBoom"));
	BroomCameraBoom->SetupAttachment(RootComponent);
	BroomCameraBoom->TargetArmLength = 300.0f;
	BroomCameraBoom->bUsePawnControlRotation = false;
	BroomCameraBoom->bDoCollisionTest = true;

	RiderMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RiderMesh"));
	RiderMesh->SetupAttachment(RootComponent);

	BroomCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BroomCamera"));
	BroomCamera->SetupAttachment(BroomCameraBoom, USpringArmComponent::SocketName);
	BroomCamera->bUsePawnControlRotation = false;

	BroomMovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("BroomMovementComponent"));
	BroomMovementComponent->MaxSpeed = 1000.f;

	GoalTargetingComponent = CreateDefaultSubobject<UGoalTargetingComponent>(TEXT("GoalTargetingComponent"));
	ZoneUIManager = CreateDefaultSubobject<UZoneUIManager>(TEXT("ZoneUIManager"));
	SoundManager = CreateDefaultSubobject<USoundManager>(TEXT("SoundManager"));
}

void ABroom::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABroom, HeldQuaffle);
	DOREPLIFETIME(ABroom, bIsStunned);
	DOREPLIFETIME(ABroom, StunSpeedMultiplier);
	DOREPLIFETIME_CONDITION(ABroom, ReplicatedTiltInput, COND_SkipOwner);
}

void ABroom::BeginPlay()
{
	Super::BeginPlay();

	if (BroomMesh)
	{
		InitialMeshRotation = BroomMesh->GetRelativeRotation();
	}

	if (RiderMesh)
	{
		InitialRiderTransform = RiderMesh->GetRelativeTransform();
	}

	CurrentHeadingYaw = GetActorRotation().Yaw;

	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		Subsystem->RegisterBroom(this);
	}

	RefreshMovementSpeed();
}

void ABroom::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		Subsystem->UnregisterBroom(this);
	}

	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.ClearTimer(StunTimerHandle);
	TimerManager.ClearTimer(RagdollTimerHandle);
	TimerManager.ClearTimer(RespawnTimerHandle);
	TimerManager.ClearTimer(PickupCooldownTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void ABroom::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Heading is owned by whoever flies the broom (local player or server-side AI).
	if (IsLocallyControlled())
	{
		UpdateHeading(DeltaTime);

		if (HasAuthority())
		{
			ReplicatedTiltInput = FVector2D(CurrentTurnInput, CurrentAscendInput);
		}
	}

	UpdateMeshTilt(DeltaTime);
	UpdateBludgerWarning();
	UpdateClientMoveSync(DeltaTime);
}

// --- Input ------------------------------------------------------------------------------------

void ABroom::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput) return;

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (BroomInputMappingContext)
			{
				Subsystem->AddMappingContext(BroomInputMappingContext, 0);
			}
		}
	}

	EnhancedInput->BindAction(BroomMovementAction, ETriggerEvent::Triggered, this, &ABroom::Move);

	EnhancedInput->BindAction(BroomTurnAction, ETriggerEvent::Triggered, this, &ABroom::Turn);
	EnhancedInput->BindAction(BroomTurnAction, ETriggerEvent::Completed, this, &ABroom::Turn);

	EnhancedInput->BindAction(BroomAscendAction, ETriggerEvent::Triggered, this, &ABroom::Ascend);
	EnhancedInput->BindAction(BroomAscendAction, ETriggerEvent::Completed, this, &ABroom::Ascend);

	EnhancedInput->BindAction(BroomSpeedBoostAction, ETriggerEvent::Started, this, &ABroom::SpeedBoost);
	EnhancedInput->BindAction(BroomSpeedBoostAction, ETriggerEvent::Completed, this, &ABroom::SpeedBoost);

	EnhancedInput->BindAction(QuafflePickupAction, ETriggerEvent::Triggered, this, &ABroom::AttemptPickupQuaffle);
	EnhancedInput->BindAction(QuaffleThrowAction, ETriggerEvent::Started, this, &ABroom::ThrowQuaffle);
}

void ABroom::Move(const FInputActionValue& Value) { PerformMove(Value.Get<float>()); }
void ABroom::Ascend(const FInputActionValue& Value) { PerformAscend(Value.Get<float>()); }
void ABroom::Turn(const FInputActionValue& Value) { PerformTurn(Value.Get<float>()); }
void ABroom::SpeedBoost(const FInputActionValue& Value) { PerformSpeedBoost(Value.Get<bool>()); }
void ABroom::AttemptPickupQuaffle() { PerformPickupQuaffle(); }
void ABroom::ThrowQuaffle() { PerformThrowQuaffle(); }

// --- Flight -----------------------------------------------------------------------------------

void ABroom::UpdateHeading(float DeltaTime)
{
	if (!BroomData) return;

	CurrentHeadingYaw += CurrentTurnInput * BroomData->TurnSpeed * DeltaTime;
	const FRotator TargetRotation(0.f, CurrentHeadingYaw, 0.f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, BroomData->HeadingInterpSpeed));
}

void ABroom::UpdateMeshTilt(float DeltaTime)
{
	if (!BroomData || !BroomMesh) return;

	const FVector2D TiltInput = IsLocallyControlled()
		? FVector2D(CurrentTurnInput, CurrentAscendInput)
		: ReplicatedTiltInput;

	FRotator TargetMeshRotation = InitialMeshRotation;
	TargetMeshRotation.Pitch += -TiltInput.X * BroomData->BankAngle;
	TargetMeshRotation.Roll += -TiltInput.Y * BroomData->AscendTiltAngle;

	BroomMesh->SetRelativeRotation(FMath::RInterpTo(BroomMesh->GetRelativeRotation(), TargetMeshRotation,
		DeltaTime, BroomData->BankInterpSpeed));
}

void ABroom::PerformMove(float AxisValue)
{
	if (bIsStunned || !IsMatchInProgress()) return;
	if (!Controller || FMath::IsNearlyZero(AxisValue)) return;

	AddMovementInput(GetActorForwardVector(), AxisValue);
}

void ABroom::PerformAscend(float AxisValue)
{
	if (bIsStunned || !IsMatchInProgress()) return;

	CurrentAscendInput = AxisValue;
	if (!Controller) return;
	AddMovementInput(FVector::UpVector, CurrentAscendInput);
}

void ABroom::PerformTurn(float AxisValue)
{
	if (bIsStunned || !IsMatchInProgress()) return;

	CurrentTurnInput = AxisValue;
}

void ABroom::PerformSpeedBoost(bool bIsActive)
{
	bBoostActive = bIsActive;
	RefreshMovementSpeed();
}

void ABroom::SetSpeedOverride(float NewSpeedOverride)
{
	if (FMath::IsNearlyEqual(SpeedOverride, NewSpeedOverride)) return;

	SpeedOverride = NewSpeedOverride;
	RefreshMovementSpeed();
}

void ABroom::RefreshMovementSpeed()
{
	if (!BroomData || !BroomMovementComponent) return;

	// Single owner for flight speed: base (data asset or AI override), then boost, then stun.
	const float BaseSpeed = SpeedOverride > 0.f ? SpeedOverride : BroomData->Speed;

	float MaxSpeed = BaseSpeed;
	float Acceleration = BaseSpeed * BroomData->AccelerationMultiplier;

	if (bBoostActive)
	{
		MaxSpeed = BaseSpeed * BroomData->SpeedBoostMultiplier;
		Acceleration = BaseSpeed * BroomData->SpeedBoostAccelerationMultiplier;
	}

	if (bIsStunned)
	{
		MaxSpeed *= StunSpeedMultiplier;
		Acceleration *= StunSpeedMultiplier;
	}

	BroomMovementComponent->MaxSpeed = MaxSpeed;
	BroomMovementComponent->Acceleration = Acceleration;
	BroomMovementComponent->Deceleration = BaseSpeed * BroomData->AccelerationMultiplier;
	BroomMovementComponent->TurningBoost = BroomData->TurnSpeed;
}

// --- Quaffle ----------------------------------------------------------------------------------

void ABroom::PerformPickupQuaffle()
{
	if (!IsMatchInProgress() || HeldQuaffle || bIsStunned) return;

	if (HasAuthority())
	{
		ServerPickupQuaffle_Implementation();
		return;
	}

	// Client: only ask the server when a free Quaffle is actually in range, and not every frame.
	const UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this);
	const AQuaffle* Nearest = Subsystem ? Subsystem->FindNearestFreeQuaffle(GetActorLocation()) : nullptr;
	if (!Nearest || !Nearest->CanBePickedUpBy(this)) return;

	const float Now = GetWorld()->GetTimeSeconds();
	if (LastPickupRequestTime >= 0.f && Now - LastPickupRequestTime < BroomConstants::PickupRequestInterval) return;
	LastPickupRequestTime = Now;

	ServerPickupQuaffle();
}

void ABroom::ServerPickupQuaffle_Implementation()
{
	if (!IsMatchInProgress() || HeldQuaffle || bIsStunned || !CanPickupQuaffle()) return;

	const UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this);
	AQuaffle* Nearest = Subsystem ? Subsystem->FindNearestFreeQuaffle(GetActorLocation()) : nullptr;
	if (!Nearest || !Nearest->CanBePickedUpBy(this)) return;

	Nearest->PickUp(this);
	if (Nearest->GetPossessingPlayer() == this)
	{
		HeldQuaffle = Nearest;
		MulticastPlayQuaffleSound(false);
	}
}

void ABroom::PerformThrowQuaffle()
{
	if (!GoalTargetingComponent) return;
	ThrowQuaffleAtGoal(GoalTargetingComponent->GetCurrentTargetGoal(), 1.f);
}

void ABroom::ThrowQuaffleAtGoal(AGoalRing* Goal, float ArcBias)
{
	if (!IsMatchInProgress() || !HeldQuaffle || !Goal) return;

	if (HasAuthority())
	{
		ServerThrowQuaffleAtGoal_Implementation(Goal, ArcBias);
	}
	else
	{
		ServerThrowQuaffleAtGoal(Goal, ArcBias);
	}
}

void ABroom::ServerThrowQuaffleAtGoal_Implementation(AGoalRing* Goal, float ArcBias)
{
	if (!IsMatchInProgress() || !HeldQuaffle || !IsValid(Goal) || !Goal->IsGoalEnabled()) return;

	// ArcBias may come from a client, so keep it in a sane range.
	const float ClampedArcBias = FMath::Clamp(ArcBias, 0.5f, 1.5f);
	if (HeldQuaffle->ThrowToTarget(Goal->GetAimDirection(), ClampedArcBias))
	{
		HeldQuaffle = nullptr;
		MulticastPlayQuaffleSound(true);
	}
}

void ABroom::MulticastPlayQuaffleSound_Implementation(bool bThrow)
{
	if (!SoundManager) return;

	if (bThrow)
	{
		SoundManager->PlayQuaffleThrow();
	}
	else
	{
		SoundManager->PlayQuafflePickup();
	}
}

void ABroom::DropQuaffle()
{
	if (!HasAuthority() || !HeldQuaffle) return;

	HeldQuaffle->Drop(this);
	HeldQuaffle = nullptr;
}

void ABroom::StartPickupCooldown(float Duration)
{
	bPickupCooldown = true;
	GetWorldTimerManager().ClearTimer(PickupCooldownTimerHandle);
	GetWorldTimerManager().SetTimer(PickupCooldownTimerHandle, this, &ABroom::ResetPickupCooldown, Duration, false);
}

// --- Stun / ragdoll ---------------------------------------------------------------------------

void ABroom::ApplyStun(float Duration, float SpeedMultiplier)
{
	if (!HasAuthority() || bIsStunned) return;

	bIsStunned = true;
	StunSpeedMultiplier = SpeedMultiplier;
	OnRep_IsStunned(); // OnRep doesn't fire on the server.

	GetWorldTimerManager().ClearTimer(StunTimerHandle);
	GetWorldTimerManager().SetTimer(StunTimerHandle, this, &ABroom::RecoverFromStun, Duration, false);
}

void ABroom::RecoverFromStun()
{
	if (!HasAuthority() || !bIsStunned) return;

	GetWorldTimerManager().ClearTimer(StunTimerHandle);
	bIsStunned = false;
	StunSpeedMultiplier = 1.f;
	OnRep_IsStunned();
}

void ABroom::OnRep_IsStunned()
{
	if (bIsStunned)
	{
		CurrentTurnInput = 0.f;
		CurrentAscendInput = 0.f;
	}

	RefreshMovementSpeed();
	OnStunChanged.Broadcast(bIsStunned);
}

void ABroom::TriggerRagdoll(const FVector& ImpulseDirection, float ImpulseStrength, float RagdollDuration,
	float RespawnDelay)
{
	if (!HasAuthority() || bIsRagdolling || !RiderMesh) return;

	bIsRagdolling = true;
	RespawnTransform = GetActorTransform();

	DropQuaffle();
	StartPickupCooldown(RespawnDelay);

	// Stay stunned until the respawn so the hidden broom can't be flown around.
	ApplyStun(FMath::Max(RagdollDuration, RespawnDelay), 0.f);
	TriggerBludgerHitShake();

	MulticastBeginRagdoll(ImpulseDirection, ImpulseStrength, RagdollDuration);

	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ABroom::Respawn, RespawnDelay, false);
}

void ABroom::MulticastBeginRagdoll_Implementation(FVector_NetQuantizeNormal ImpulseDirection, float ImpulseStrength,
	float RagdollDuration)
{
	if (SoundManager)
	{
		SoundManager->PlayBroomHitByBludger();
	}

	if (BroomMesh)
	{
		BroomMesh->SetVisibility(false);
	}

	if (RiderMesh)
	{
		RiderMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		RiderMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		RiderMesh->SetCollisionResponseToAllChannels(ECR_Block);
		RiderMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		RiderMesh->SetSimulatePhysics(true);

		const FVector Impulse = (FVector(ImpulseDirection) + FVector::UpVector).GetSafeNormal() * ImpulseStrength;
		RiderMesh->AddImpulse(Impulse, NAME_None, true);
	}

	GetWorldTimerManager().ClearTimer(RagdollTimerHandle);
	GetWorldTimerManager().SetTimer(RagdollTimerHandle, this, &ABroom::HandleRagdollTimeout, RagdollDuration, false);
}

void ABroom::HandleRagdollTimeout()
{
	if (!RiderMesh) return;

	RiderMesh->SetSimulatePhysics(false);
	RiderMesh->SetVisibility(false);
}

void ABroom::Respawn()
{
	if (!HasAuthority()) return;

	bIsRagdolling = false;
	MulticastFinishRespawn(RespawnTransform.GetLocation(), RespawnTransform.Rotator());
	RecoverFromStun();
}

void ABroom::MulticastFinishRespawn_Implementation(FVector_NetQuantize10 Location, FRotator Rotation)
{
	GetWorldTimerManager().ClearTimer(RagdollTimerHandle);

	if (RiderMesh)
	{
		RiderMesh->SetSimulatePhysics(false);
		RiderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RiderMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		RiderMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		RiderMesh->SetRelativeTransform(InitialRiderTransform);
		RiderMesh->SetVisibility(true);
	}

	if (BroomMesh)
	{
		BroomMesh->SetVisibility(true);
	}

	// Teleport on every machine, including the owning client whose position isn't replicated.
	SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	CurrentHeadingYaw = Rotation.Yaw;
}

// --- Network movement sync (interim) ----------------------------------------------------------

bool ABroom::IsAutonomousClient() const
{
	return !HasAuthority() && IsLocallyControlled();
}

void ABroom::UpdateClientMoveSync(float DeltaTime)
{
	if (!IsAutonomousClient() || !BroomData) return;

	TimeSinceMoveSync += DeltaTime;
	if (TimeSinceMoveSync < 1.f / BroomData->ClientMoveSyncRate) return;
	TimeSinceMoveSync = 0.f;

	ServerSyncMovement(GetActorLocation(), GetActorRotation().Yaw, FVector2D(CurrentTurnInput, CurrentAscendInput));
}

void ABroom::ServerSyncMovement_Implementation(FVector_NetQuantize10 Location, float Yaw, FVector2D TiltInput)
{
	if (!BroomData) return;

	const float Now = GetWorld()->GetTimeSeconds();
	const float Elapsed = LastServerMoveSyncTime < 0.f ? 1.f : FMath::Clamp(Now - LastServerMoveSyncTime, 0.f, 1.f);
	LastServerMoveSyncTime = Now;

	// Reject moves faster than a boosting broom could fly (plus slack for jitter).
	const float MaxPossibleSpeed = BroomData->Speed * FMath::Max(1.f, BroomData->SpeedBoostMultiplier);
	const float AllowedDistance = MaxPossibleSpeed * Elapsed * BroomData->MaxClientMoveSpeedTolerance + 100.f;

	if (bIsRagdolling || FVector::Dist(Location, GetActorLocation()) > AllowedDistance)
	{
		ClientCorrectPosition(GetActorLocation(), GetActorRotation().Yaw);
		return;
	}

	SetActorLocationAndRotation(Location, FRotator(0.f, Yaw, 0.f));
	ReplicatedTiltInput = FVector2D(FMath::Clamp(TiltInput.X, -1.f, 1.f), FMath::Clamp(TiltInput.Y, -1.f, 1.f));
}

void ABroom::ClientCorrectPosition_Implementation(FVector_NetQuantize10 Location, float Yaw)
{
	SetActorLocationAndRotation(Location, FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	CurrentHeadingYaw = Yaw;
	if (BroomMovementComponent)
	{
		BroomMovementComponent->StopMovementImmediately();
	}
}

// --- Feedback ---------------------------------------------------------------------------------

bool ABroom::IsMatchInProgress() const
{
	const AQuidProWizGameStateBase* GameState = GetWorld()->GetGameState<AQuidProWizGameStateBase>();
	return !GameState || GameState->CanMove();
}

void ABroom::TriggerBludgerHitShake()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !BludgerHitShakeClass) return;

	PC->ClientStartCameraShake(BludgerHitShakeClass);
}

void ABroom::TriggerGoalScoredShake()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !GoalScoredShakeClass) return;

	PC->ClientStartCameraShake(GoalScoredShakeClass);
}

void ABroom::UpdateBludgerWarning()
{
	bBludgerWarningActive = false;
	if (!IsLocallyControlled() || !BroomData) return;

	const UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this);
	if (!Subsystem) return;

	float Distance = 0.f;
	const ABludger* Nearest = Subsystem->FindNearestBludger(GetActorLocation(), &Distance);
	bBludgerWarningActive = Nearest && Distance <= BroomData->BludgerWarningDistance;
}
