// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Broom.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "Ball/Quaffle.h"
#include "Kismet/GameplayStatics.h"
#include "Goal/GoalTargetingComponent.h"
#include "Goal/GoalRing.h"
#include "Game/QuidProWizGameStateBase.h"

// Sets default values
ABroom::ABroom()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Create collision component
	BroomCollisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BroomCollisionComponent"));
	BroomCollisionComponent->InitCapsuleSize(42.f, 96.0f);
	BroomCollisionComponent->SetGenerateOverlapEvents(true);
	RootComponent = BroomCollisionComponent;

	//Create mesh component
	BroomMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BroomMesh"));
	BroomMesh->SetupAttachment(RootComponent);

	//Create camera boom
	BroomCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("BroomCameraBoom"));
	BroomCameraBoom->SetupAttachment(RootComponent);
	BroomCameraBoom->TargetArmLength = 300.0f;
	BroomCameraBoom->bUsePawnControlRotation = false;
	BroomCameraBoom->bDoCollisionTest = true;

	RiderMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RiderMesh"));
	RiderMesh->SetupAttachment(RootComponent);

	//Create camera
	BroomCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BroomCamera"));
	BroomCamera->SetupAttachment(BroomCameraBoom, USpringArmComponent::SocketName);
	BroomCamera->SetRelativeLocation(FVector(FVector::ZeroVector));
	BroomCamera->bUsePawnControlRotation = false;

	// Create movement component
	BroomMovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("BroomMovementComponent"));
	BroomMovementComponent->MaxSpeed = 1000.f;

	GoalTargetingComponent = CreateDefaultSubobject<UGoalTargetingComponent>(TEXT("GoalTargetingComponent"));
}

// Called when the game starts or when spawned
void ABroom::BeginPlay()
{
	Super::BeginPlay();

	if (BroomMesh)
	{
		InitialMeshRotation = BroomMesh->GetRelativeRotation();
	}

	CurrentHeadingYaw = GetActorRotation().Yaw;

	if (IsLocallyControlled())
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
			{
				if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
					LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
				{
					Subsystem->AddMappingContext(BroomInputMappingContext, 0);
				}
			}
		}
	}

	AActor* FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), AQuaffle::StaticClass());
	QuaffleRef = Cast<AQuaffle>(FoundActor);

	if (!BroomData) return;
	
	BroomMovementComponent->MaxSpeed = BroomData->Speed;
	BroomMovementComponent->Acceleration = BroomData->Speed * 2.f;
	BroomMovementComponent->Deceleration = BroomData->Speed * 2.f;
	BroomMovementComponent->TurningBoost = BroomData->TurnSpeed;

	if (RiderMesh)
	{
		InitialSpawnTransform = RiderMesh->GetRelativeTransform();
	}
}

// Called every frame
void ABroom::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateHeading(DeltaTime);
	UpdateMeshTilt(DeltaTime);
}

void ABroom::UpdateHeading(float DeltaTime)
{
	CurrentHeadingYaw += CurrentTurnInput * BroomData->TurnSpeed * DeltaTime;
	const FRotator CurrentActorRotation = GetActorRotation();
	const FRotator TargetActorRotation = FRotator(0.f, CurrentHeadingYaw, 0.f);
	const FRotator NewActorRotation = FMath::RInterpTo(CurrentActorRotation, TargetActorRotation,
		DeltaTime, BroomData->HeadingInterpSpeed);
	
	SetActorRotation(NewActorRotation);
}

void ABroom::UpdateMeshTilt(float DeltaTime)
{
	//Rotate the mesh to match the control rotation
	const FRotator CurrentMeshRotation = BroomMesh->GetRelativeRotation();
	const float TargetPitch = -CurrentTurnInput * BroomData->BankAngle;

	FRotator TargetMeshRotation = InitialMeshRotation;
	TargetMeshRotation.Pitch += TargetPitch;
	TargetMeshRotation.Roll += -(CurrentAscendInput * BroomData->AscendTiltAngle);

	const FRotator NewMeshRotation = FMath::RInterpTo(CurrentMeshRotation, TargetMeshRotation,
		DeltaTime, BroomData->BankInterpSpeed);
	BroomMesh->SetRelativeRotation(NewMeshRotation);
}

// Called to bind functionality to input
void ABroom::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (!IsLocallyControlled()) return;

	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if(UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInput->BindAction(BroomMovementAction, ETriggerEvent::Triggered, this, &ABroom::Move);
		
		EnhancedInput->BindAction(BroomTurnAction, ETriggerEvent::Triggered, this, &ABroom::Turn);
		EnhancedInput->BindAction(BroomTurnAction, ETriggerEvent::Completed, this, &ABroom::Turn);
		
		EnhancedInput->BindAction(BroomAscendAction, ETriggerEvent::Triggered, this, &ABroom::Ascend);
		EnhancedInput->BindAction(BroomAscendAction, ETriggerEvent::Completed, this, &ABroom::Ascend);

		EnhancedInput->BindAction(BroomSpeedBoostAction, ETriggerEvent::Started, 
			this, &ABroom::SpeedBoost);
		EnhancedInput->BindAction(BroomSpeedBoostAction, ETriggerEvent::Completed, 
			this, &ABroom::SpeedBoost);

		EnhancedInput->BindAction(QuafflePickupAction, ETriggerEvent::Triggered,
			this, &ABroom::AttemptPickupQuaffle);

		EnhancedInput->BindAction(QuaffleThrowAction, ETriggerEvent::Started, this, &ABroom::ThrowQuaffle);
	}
}

void ABroom::Move(const FInputActionValue& Value)
{
	PerformMove(Value.Get<float>());
}

void ABroom::Ascend(const FInputActionValue& Value)
{
	PerformAscend(Value.Get<float>());
}

void ABroom::Turn(const FInputActionValue& Value)
{
	PerformTurn(Value.Get<float>());
}

void ABroom::SpeedBoost(const FInputActionValue& Value)
{
	PerformSpeedBoost(Value.Get<bool>());
}

void ABroom::AttemptPickupQuaffle()
{
	PerformPickupQuaffle();
}

void ABroom::ThrowQuaffle()
{
	PerformThrowQuaffle();
}

void ABroom::NotifyQuafflePickedUp(AQuaffle* PickedUpQuaffle)
{
	HeldQuaffle = PickedUpQuaffle;
}

void ABroom::NotifyQuaffleReleased()
{
	HeldQuaffle = nullptr;
}

void ABroom::DropQuaffle()
{
	if(!HeldQuaffle) return;
	
	HeldQuaffle->Drop(this);
	HeldQuaffle = nullptr;
}

void ABroom::ApplyStun(float Duration, float SpeedMultiplier)
{
	if (bIsStunned) return;
	if (!BroomData) return;

	bIsStunned = true;

	BroomMovementComponent->MaxSpeed = BroomData->Speed * SpeedMultiplier;
	BroomMovementComponent->Acceleration = BroomData->Speed * SpeedMultiplier;

	GetWorldTimerManager().ClearTimer(StunTimerHandle);
	GetWorldTimerManager().SetTimer(StunTimerHandle, this, &ABroom::RecoverFromStun, Duration, false);
}

void ABroom::TriggerRagdoll(const FVector& ImpulseDirection, float ImpulseStrength, float RagdollDuration, 
	float RespawnDelay)
{
	if (bIsRagDolling) return;
	if (!RiderMesh) return;

	bIsRagDolling = true;

	RespawnTransform = GetActorTransform();

	DropQuaffle();
	ApplyStun(RagdollDuration, 0.f);	
	DetachRider(ImpulseDirection, ImpulseStrength);

	if (BroomMesh)
	{
		BroomMesh->SetVisibility(false);
	}

	GetWorldTimerManager().ClearTimer(RagdollTimerHandle);
	GetWorldTimerManager().SetTimer(RagdollTimerHandle, [this]()
		{
			if (RiderMesh)
			{
				RiderMesh->SetSimulatePhysics(false);
				RiderMesh->SetVisibility(false);
			}

		}, RagdollDuration, false);

	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ABroom::Respawn,
		RespawnDelay, false);
}

void ABroom::PerformMove(float AxisValue)
{
	if (bIsStunned) return;
	if (!IsMatchInProgress()) return;
	if (!Controller || FMath::IsNearlyZero(AxisValue)) return;

	AddMovementInput(GetActorForwardVector(), AxisValue);
}

void ABroom::PerformAscend(float AxisValue)
{
	if (bIsStunned) return;
	if (!IsMatchInProgress()) return;

	CurrentAscendInput = AxisValue;
	if (!Controller) return;
	AddMovementInput(FVector::UpVector, CurrentAscendInput);
}

void ABroom::PerformTurn(float AxisValue)
{
	if (bIsStunned) return;
	if (!IsMatchInProgress()) return;

	CurrentTurnInput = AxisValue;
}

void ABroom::PerformSpeedBoost(bool bIsActive)
{
	if (!BroomData) return;

	if(bIsActive)
	{
		BroomMovementComponent->MaxSpeed = BroomData->Speed * BroomData->SpeedBoostMultiplier;
		BroomMovementComponent->Acceleration = BroomData->Speed * BroomData->SpeedBoostAccelerationMultiplier;
	}
	else
	{
		BroomMovementComponent->MaxSpeed = BroomData->Speed;
		BroomMovementComponent->Acceleration = BroomData->Speed * 2.f;
	}
}

void ABroom::PerformPickupQuaffle()
{
	if (!IsMatchInProgress()) return;
	if (HeldQuaffle) return;
	if (!QuaffleRef) return;
	if (!QuaffleRef->CanBePickedUpBy(this)) return;

	QuaffleRef->PickUp(this);
	HeldQuaffle = QuaffleRef;
}

void ABroom::PerformThrowQuaffle()
{
	if (!IsMatchInProgress()) return;
	if (!HeldQuaffle) return;
	if (!GoalTargetingComponent) return;

	AGoalRing* LockedGoal = GoalTargetingComponent->GetCurrentTargetGoal();
	if (!LockedGoal) return;

	const FVector AimLocation = LockedGoal->GetAimDirection();
	if (HeldQuaffle->ThrowToTarget(AimLocation, 1.f))
	{
		HeldQuaffle = nullptr;
	}
}

void ABroom::RecoverFromStun()
{
	bIsStunned = false;

	if (!BroomData) return;

	BroomMovementComponent->MaxSpeed = BroomData->Speed;
	BroomMovementComponent->Acceleration = BroomData->Speed * 2.f;
}

void ABroom::DetachRider(const FVector& ImpulseDirection, float ImpulseStrength)
{
	if (!RiderMesh) return;

	RiderMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	RiderMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RiderMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	RiderMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	RiderMesh->SetSimulatePhysics(true);

	const FVector Impulse = (ImpulseDirection + FVector::UpVector).GetSafeNormal() * ImpulseStrength;
	RiderMesh->AddImpulse(Impulse, NAME_None, true);
}

void ABroom::Respawn()
{
	bIsRagDolling = false;

	if (!RiderMesh) return;

	RiderMesh->SetSimulatePhysics(false);
	RiderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RiderMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);

	RiderMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	RiderMesh->SetRelativeTransform(InitialSpawnTransform);
	RiderMesh->SetVisibility(true);

	if (BroomMesh)
	{
		BroomMesh->SetVisibility(true);
	}

	SetActorTransform(RespawnTransform);

	RecoverFromStun();
}

bool ABroom::IsMatchInProgress() const
{
	AQuidProWizGameStateBase* GameState = GetWorld()->GetGameState<AQuidProWizGameStateBase>();
	if (!GameState) return true;
	return GameState->CanMove();
}