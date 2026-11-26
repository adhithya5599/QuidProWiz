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

	// The movement component turns the broom toward the controller's yaw at its rotation rate.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);

	// The Character's mesh component is an empty visual root: the movement component offsets it
	// to smooth corrections on remote machines, and the broom and rider ride along with it.
	USceneComponent* VisualRoot = GetMesh();

	BroomMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BroomMesh"));
	BroomMesh->SetupAttachment(VisualRoot);

	RiderMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RiderMesh"));
	RiderMesh->SetupAttachment(VisualRoot);

	BroomCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("BroomCameraBoom"));
	BroomCameraBoom->SetupAttachment(RootComponent);
	BroomCameraBoom->TargetArmLength = 300.0f;
	BroomCameraBoom->bUsePawnControlRotation = false;
	BroomCameraBoom->bDoCollisionTest = true;

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

	if (BroomMesh)
	{
		InitialMeshRotation = BroomMesh->GetRelativeRotation();
	}

	if (RiderMesh)
	{
		InitialRiderTransform = RiderMesh->GetRelativeTransform();
	}

	LastVisualYaw = GetActorRotation().Yaw;

	if (BroomData)
	{
		GetCharacterMovement()->RotationRate = FRotator(0.f, BroomData->HeadingRotationRate, 0.f);
	}

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

	// Start the controller facing the way the broom faces, so it doesn't swing round on possession.
	if (Controller && IsLocallyControlled())
	{
		Controller->SetControlRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
	}
}

void ABroom::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Re-read each frame so live tuning of the data asset applies straight away.
	if (BroomData)
	{
		GetCharacterMovement()->RotationRate.Yaw = BroomData->HeadingRotationRate;
	}

	UpdateControlHeading(DeltaTime);
	UpdateMeshTilt(DeltaTime);
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

void ABroom::UpdateControlHeading(float DeltaTime)
{
	if (!BroomData || !Controller || !IsLocallyControlled()) return;
	if (FMath::IsNearlyZero(CurrentTurnInput)) return;

	FRotator ControlRotation = Controller->GetControlRotation();
	ControlRotation.Yaw += CurrentTurnInput * BroomData->TurnSpeed * DeltaTime;
	ControlRotation.Pitch = 0.f;
	ControlRotation.Roll = 0.f;
	Controller->SetControlRotation(ControlRotation);
}

void ABroom::UpdateMeshTilt(float DeltaTime)
{
	if (!BroomData || !BroomMesh || DeltaTime <= 0.f) return;

	const float Yaw = GetActorRotation().Yaw;
	const float YawRate = FMath::FindDeltaAngleDegrees(LastVisualYaw, Yaw) / DeltaTime;
	LastVisualYaw = Yaw;

	const float TurnAmount = BroomData->TurnSpeed > 0.f ? FMath::Clamp(YawRate / BroomData->TurnSpeed, -1.f, 1.f) : 0.f;
	const float MaxSpeed = GetCurrentMaxSpeed();
	const float AscendAmount = MaxSpeed > 0.f ? FMath::Clamp(GetVelocity().Z / MaxSpeed, -1.f, 1.f) : 0.f;

	FRotator TargetMeshRotation = InitialMeshRotation;
	TargetMeshRotation.Pitch += -TurnAmount * BroomData->BankAngle;
	TargetMeshRotation.Roll += -AscendAmount * BroomData->AscendTiltAngle;

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
	if (!Controller || FMath::IsNearlyZero(AxisValue)) return;
	AddMovementInput(FVector::UpVector, CurrentAscendInput);
}

void ABroom::PerformTurn(float AxisValue)
{
	if (bIsStunned || !IsMatchInProgress()) return;

	CurrentTurnInput = AxisValue;
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
		RiderMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
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
