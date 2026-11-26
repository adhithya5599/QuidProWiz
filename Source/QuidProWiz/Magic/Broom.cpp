// Fill out your copyright notice in the Description page of Project Settings.

#include "Broom.h"
#include "BroomDataAsset.h"
#include "BroomMovementComponent.h"
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
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace BroomConstants
{
	/** Minimum seconds between client pickup requests while the pickup input is held. */
	constexpr float PickupRequestInterval = 0.2f;
}

ABroom::ABroom(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UBroomMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	// The movement component turns the broom to face its direction of travel.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);

	// The Character's mesh component is an empty visual root: the movement component offsets it
	// to smooth corrections on remote machines. The pivot under it leans with the motion, and the
	// broom and rider ride on the pivot.
	VisualPivot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualPivot"));
	VisualPivot->SetupAttachment(GetMesh());

	BroomMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BroomMesh"));
	BroomMesh->SetupAttachment(VisualPivot);

	RiderMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RiderMesh"));
	RiderMesh->SetupAttachment(VisualPivot);

	// The camera looks along the aim (the controller's rotation) and trails the broom with lag.
	BroomCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("BroomCameraBoom"));
	BroomCameraBoom->SetupAttachment(RootComponent);
	BroomCameraBoom->TargetArmLength = 420.0f;
	BroomCameraBoom->bUsePawnControlRotation = true;
	BroomCameraBoom->bInheritRoll = false;
	BroomCameraBoom->bDoCollisionTest = true;
	BroomCameraBoom->bEnableCameraLag = true;
	BroomCameraBoom->bEnableCameraRotationLag = true;
	BroomCameraBoom->CameraLagMaxDistance = 400.f;

	BroomCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BroomCamera"));
	BroomCamera->SetupAttachment(BroomCameraBoom, USpringArmComponent::SocketName);
	BroomCamera->bUsePawnControlRotation = false;

	GoalTargetingComponent = CreateDefaultSubobject<UGoalTargetingComponent>(TEXT("GoalTargetingComponent"));
	ZoneUIManager = CreateDefaultSubobject<UZoneUIManager>(TEXT("ZoneUIManager"));
	SoundManager = CreateDefaultSubobject<USoundManager>(TEXT("SoundManager"));
}

void ABroom::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABroom, HeldQuaffle);
	DOREPLIFETIME(ABroom, Team);
	DOREPLIFETIME(ABroom, bIsStunned);
	DOREPLIFETIME(ABroom, StunSpeedMultiplier);
}

void ABroom::BeginPlay()
{
	Super::BeginPlay();

	// Lean around the middle of the capsule rather than the mesh root at the rider's feet:
	// move the pivot up and the meshes down by the same amount (world positions unchanged).
	if (VisualPivot && GetMesh())
	{
		const FVector LeanCentre(0.f, 0.f, -GetMesh()->GetRelativeLocation().Z);
		VisualPivot->SetRelativeLocation(LeanCentre);
		for (USceneComponent* Visual : { static_cast<USceneComponent*>(BroomMesh), static_cast<USceneComponent*>(RiderMesh) })
		{
			if (Visual && Visual->GetAttachParent() == VisualPivot)
			{
				Visual->AddRelativeLocation(-LeanCentre);
			}
		}
	}

	if (RiderMesh)
	{
		InitialRiderTransform = RiderMesh->GetRelativeTransform();
	}

	LastVisualYaw = GetActorRotation().Yaw;
	HeldAim = GetActorRotation();

	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		Subsystem->RegisterBroom(this);
	}
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

void ABroom::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Start the aim facing the way the broom faces, so it doesn't swing round on possession.
	if (Controller && IsLocallyControlled())
	{
		Controller->SetControlRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
	}

	bFreeLook = false;
	bReturningFromFreeLook = false;

	// Limit how far the player can aim up and down.
	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller); PlayerController && PlayerController->PlayerCameraManager && BroomData)
	{
		PlayerController->PlayerCameraManager->ViewPitchMin = -BroomData->MaxAimPitch;
		PlayerController->PlayerCameraManager->ViewPitchMax = BroomData->MaxAimPitch;
	}
}

void ABroom::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateSteering(DeltaTime);
	UpdateCamera(DeltaTime);
	UpdateVisualTilt(DeltaTime);
	UpdateBludgerWarning();
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

	if (BroomLookAction)
	{
		EnhancedInput->BindAction(BroomLookAction, ETriggerEvent::Triggered, this, &ABroom::Look);
	}

	if (BroomFreeLookAction)
	{
		EnhancedInput->BindAction(BroomFreeLookAction, ETriggerEvent::Started, this, &ABroom::FreeLook);
		EnhancedInput->BindAction(BroomFreeLookAction, ETriggerEvent::Completed, this, &ABroom::FreeLook);
	}

	EnhancedInput->BindAction(BroomSpeedBoostAction, ETriggerEvent::Started, this, &ABroom::SpeedBoost);
	EnhancedInput->BindAction(BroomSpeedBoostAction, ETriggerEvent::Completed, this, &ABroom::SpeedBoost);

	EnhancedInput->BindAction(QuafflePickupAction, ETriggerEvent::Triggered, this, &ABroom::AttemptPickupQuaffle);
	EnhancedInput->BindAction(QuaffleThrowAction, ETriggerEvent::Started, this, &ABroom::ThrowQuaffle);
}

void ABroom::Move(const FInputActionValue& Value) { PerformMove(Value.Get<float>()); }
void ABroom::Ascend(const FInputActionValue& Value) { PerformAscend(Value.Get<float>()); }
void ABroom::Turn(const FInputActionValue& Value) { PerformTurn(Value.Get<float>()); }
void ABroom::Look(const FInputActionValue& Value) { PerformLook(Value.Get<FVector2D>()); }
void ABroom::FreeLook(const FInputActionValue& Value) { PerformFreeLook(Value.Get<bool>()); }
void ABroom::SpeedBoost(const FInputActionValue& Value) { PerformSpeedBoost(Value.Get<bool>()); }
void ABroom::AttemptPickupQuaffle() { PerformPickupQuaffle(); }
void ABroom::ThrowQuaffle() { PerformThrowQuaffle(); }

// --- Flight -----------------------------------------------------------------------------------

UBroomMovementComponent* ABroom::GetBroomMovement() const
{
	return Cast<UBroomMovementComponent>(GetCharacterMovement());
}

float ABroom::GetCurrentSpeed() const
{
	return GetVelocity().Size();
}

float ABroom::GetCurrentMaxSpeed() const
{
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	return Movement ? Movement->GetMaxSpeed() : 0.f;
}

FRotator ABroom::GetAimRotation() const
{
	if (bFreeLook || bReturningFromFreeLook)
	{
		return HeldAim;
	}
	return Controller ? Controller->GetControlRotation() : GetActorRotation();
}

void ABroom::UpdateSteering(float DeltaTime)
{
	if (!BroomData || !Controller || !IsLocallyControlled()) return;

	// Keyboard turning ramps in and out instead of starting and stopping dead.
	CurrentTurnInput = FMath::FInterpTo(CurrentTurnInput, TargetTurnInput, DeltaTime, BroomData->TurnInputSmoothing);
	if (FMath::Abs(CurrentTurnInput) < 0.001f && FMath::IsNearlyZero(TargetTurnInput))
	{
		CurrentTurnInput = 0.f;
	}

	if (!FMath::IsNearlyZero(CurrentTurnInput))
	{
		const float YawDelta = CurrentTurnInput * BroomData->TurnSpeed * DeltaTime;
		if (bFreeLook || bReturningFromFreeLook)
		{
			HeldAim.Yaw += YawDelta; // keyboard still steers the held course
		}
		if (!bFreeLook)
		{
			FRotator ControlRotation = Controller->GetControlRotation();
			ControlRotation.Yaw += YawDelta;
			Controller->SetControlRotation(ControlRotation);
		}
	}

	// After free look the camera swings back behind the broom; then aim and camera are one again.
	if (bReturningFromFreeLook)
	{
		const FRotator Current = Controller->GetControlRotation();
		const FRotator Next = FMath::RInterpTo(Current, HeldAim, DeltaTime, BroomData->FreeLookReturnSpeed);
		Controller->SetControlRotation(Next);
		if ((Next - HeldAim).GetNormalized().IsNearlyZero(1.f))
		{
			bReturningFromFreeLook = false;
		}
	}

	// A held "turn" key is re-sent every frame; clear it so releasing the key stops the turn.
	TargetTurnInput = 0.f;
}

void ABroom::UpdateCamera(float DeltaTime)
{
	if (!BroomData || !IsLocallyControlled() || !BroomCameraBoom || !BroomCamera) return;

	const UBroomMovementComponent* Movement = GetBroomMovement();
	const float Cruise = Movement ? FMath::Max(Movement->GetCruiseSpeed(), 1.f) : 1.f;
	const float BoostedTop = Cruise * FMath::Max(BroomData->SpeedBoostMultiplier, 1.f);
	const float SpeedAlpha = FMath::Clamp(GetCurrentSpeed() / BoostedTop, 0.f, 1.f);

	const float TargetLength = FMath::Lerp(BroomData->CameraDistance, BroomData->CameraDistanceAtSpeed, SpeedAlpha);
	BroomCameraBoom->TargetArmLength = FMath::FInterpTo(BroomCameraBoom->TargetArmLength, TargetLength, DeltaTime, 3.f);
	BroomCameraBoom->SocketOffset = FVector(0.f, 0.f, BroomData->CameraHeight);
	BroomCameraBoom->CameraLagSpeed = BroomData->CameraLagSpeed;
	BroomCameraBoom->CameraRotationLagSpeed = BroomData->CameraRotationLagSpeed;
	BroomCameraBoom->bEnableCameraLag = BroomData->CameraLagSpeed > 0.f;
	BroomCameraBoom->bEnableCameraRotationLag = BroomData->CameraRotationLagSpeed > 0.f;

	// Widen the view while boosting fast.
	const bool bBoosting = Movement && Movement->WantsToBoost();
	const float BoostAlpha = bBoosting ? FMath::Clamp((GetCurrentSpeed() - Cruise) / FMath::Max(BoostedTop - Cruise, 1.f), 0.f, 1.f) : 0.f;
	const float TargetFOV = FMath::Lerp(BroomData->CameraFOV, BroomData->CameraBoostFOV, BoostAlpha);
	BroomCamera->SetFieldOfView(FMath::FInterpTo(BroomCamera->FieldOfView, TargetFOV, DeltaTime, BroomData->CameraFOVInterpSpeed));
}

void ABroom::UpdateVisualTilt(float DeltaTime)
{
	if (!BroomData || !VisualPivot || DeltaTime <= 0.f || bIsRagdolling) return;

	// Bank from how fast the heading is changing.
	const float Yaw = GetActorRotation().Yaw;
	const float YawRate = FMath::FindDeltaAngleDegrees(LastVisualYaw, Yaw) / DeltaTime;
	LastVisualYaw = Yaw;

	const float BankReference = FMath::Max(BroomData->TurnRateFast, 1.f);
	const float TurnAmount = FMath::Clamp(YawRate / BankReference, -1.f, 1.f);

	// Pitch the nose with the climb/dive angle, fading out when nearly hovering.
	const FVector Velocity = GetVelocity();
	const float Speed2D = FVector(Velocity.X, Velocity.Y, 0.f).Size();
	const float ClimbAngle = FMath::RadiansToDegrees(FMath::Atan2(Velocity.Z, FMath::Max(Speed2D, 1.f)));
	const float PitchFade = FMath::Clamp(Velocity.Size() / 300.f, 0.f, 1.f);
	const float Pitch = FMath::Clamp(ClimbAngle, -BroomData->AscendTiltAngle, BroomData->AscendTiltAngle) * PitchFade;

	const FRotator Target(Pitch, 0.f, TurnAmount * BroomData->BankAngle);
	VisualPivot->SetRelativeRotation(FMath::RInterpTo(VisualPivot->GetRelativeRotation(), Target, DeltaTime, BroomData->BankInterpSpeed));
}

void ABroom::PerformMove(float AxisValue)
{
	if (bIsStunned || !IsMatchInProgress()) return;
	if (!Controller || FMath::IsNearlyZero(AxisValue)) return;

	// Fly toward the aim, climbing or diving with its pitch.
	AddMovementInput(GetAimRotation().Vector(), AxisValue);
}

void ABroom::PerformAscend(float AxisValue)
{
	if (bIsStunned || !IsMatchInProgress()) return;

	CurrentAscendInput = AxisValue;
	if (!Controller || FMath::IsNearlyZero(AxisValue)) return;

	const float Strength = BroomData ? BroomData->VerticalInputStrength : 1.f;
	AddMovementInput(FVector::UpVector, CurrentAscendInput * Strength);
}

void ABroom::PerformTurn(float AxisValue)
{
	if (bIsStunned || !IsMatchInProgress()) return;

	TargetTurnInput = FMath::Clamp(AxisValue, -1.f, 1.f);
}

void ABroom::PerformLook(const FVector2D& LookAxis)
{
	if (!Controller || LookAxis.IsNearlyZero()) return;

	// Looking during the swing back after free look takes over the aim straight away.
	if (bReturningFromFreeLook)
	{
		bReturningFromFreeLook = false;
	}

	const float Sensitivity = BroomData ? BroomData->LookSensitivity : 1.f;
	AddControllerYawInput(LookAxis.X * Sensitivity);
	AddControllerPitchInput(LookAxis.Y * Sensitivity);
}

void ABroom::PerformFreeLook(bool bActive)
{
	if (!Controller) return;

	if (bActive && !bFreeLook)
	{
		HeldAim = GetAimRotation();
		bFreeLook = true;
		bReturningFromFreeLook = false;
	}
	else if (!bActive && bFreeLook)
	{
		bFreeLook = false;
		bReturningFromFreeLook = true;
	}
}

void ABroom::PerformSpeedBoost(bool bIsActive)
{
	if (UBroomMovementComponent* Movement = GetBroomMovement())
	{
		Movement->SetWantsToBoost(bIsActive);
	}
}

void ABroom::SetTeam(EGoalTeam NewTeam)
{
	if (HasAuthority())
	{
		Team = NewTeam;
	}
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

	// Speed limits are read from the stun state by the movement component every frame.
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

	// Server-side teleport; the movement component corrects the owning client automatically.
	TeleportTo(RespawnTransform.GetLocation(), RespawnTransform.Rotator(), false, true);
	MulticastFinishRespawn();
	RecoverFromStun();
}

void ABroom::MulticastFinishRespawn_Implementation()
{
	GetWorldTimerManager().ClearTimer(RagdollTimerHandle);

	if (RiderMesh)
	{
		RiderMesh->SetSimulatePhysics(false);
		RiderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RiderMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		RiderMesh->AttachToComponent(VisualPivot ? VisualPivot.Get() : GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		RiderMesh->SetRelativeTransform(InitialRiderTransform);
		RiderMesh->SetVisibility(true);
	}

	if (BroomMesh)
	{
		BroomMesh->SetVisibility(true);
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
