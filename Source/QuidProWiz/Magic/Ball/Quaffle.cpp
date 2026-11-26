// Fill out your copyright notice in the Description page of Project Settings.

#include "Quaffle.h"
#include "DataAsset/QuaffleDataAsset.h"
#include "QuidditchWorldSubsystem.h"
#include "Broom.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

const FQuaffleStateBase& FQuaffleStateBase::Get(EQuaffleState State)
{
	static const FQuaffleStateFree Free;
	static const FQuaffleStatePossessed Possessed;
	static const FQuaffleStateThrown Thrown;

	switch (State)
	{
	case EQuaffleState::Possessed:	return Possessed;
	case EQuaffleState::Thrown:		return Thrown;
	case EQuaffleState::Free:
	default:						return Free;
	}
}

AQuaffle::AQuaffle()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bAutoActivate = false;
	ProjectileMovement->ProjectileGravityScale = 0.6f;

	TrailEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailEffect"));
	TrailEffect->SetupAttachment(RootComponent);
	TrailEffect->SetAutoActivate(false);
}

void AQuaffle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AQuaffle, QuaffleState);
	DOREPLIFETIME(AQuaffle, PossessingPlayer);
}

void AQuaffle::BeginPlay()
{
	Super::BeginPlay();

	ApplyFlightTuning();

	if (QuaffleData)
	{
		if (QuaffleData->ThrowTrailEffect)
		{
			TrailEffect->SetAsset(QuaffleData->ThrowTrailEffect);
		}
	}

	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		Subsystem->RegisterQuaffle(this);
	}

	ApplyStateEffects();
}

void AQuaffle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		Subsystem->UnregisterQuaffle(this);
	}

	GetWorldTimerManager().ClearTimer(MissRespawnTimerHandle);
	Super::EndPlay(EndPlayReason);
}

float AQuaffle::GetPickupRadius() const
{
	return QuaffleData ? QuaffleData->PickupRadius : 200.f;
}

bool AQuaffle::CanBePickedUpBy(const AActor* PlayerActor) const
{
	return CanBePickedUp() && IsInPickupRange(PlayerActor);
}

bool AQuaffle::IsInPickupRange(const AActor* PlayerActor) const
{
	if (!IsValid(PlayerActor)) return false;
	return FVector::Dist(GetActorLocation(), PlayerActor->GetActorLocation()) <= GetPickupRadius();
}

void AQuaffle::SetQuaffleState(EQuaffleState NewState)
{
	if (!HasAuthority()) return;

	QuaffleState = NewState;
	ApplyStateEffects(); // OnRep doesn't fire on the server.
}

void AQuaffle::OnRep_QuaffleState()
{
	ApplyStateEffects();
}

void AQuaffle::ApplyStateEffects()
{
	const bool bPossessed = QuaffleState == EQuaffleState::Possessed;
	CollisionComponent->SetCollisionEnabled(bPossessed ? ECollisionEnabled::NoCollision
		: ECollisionEnabled::QueryAndPhysics);

	if (!TrailEffect) return;

	if (QuaffleState == EQuaffleState::Thrown && TrailEffect->GetAsset())
	{
		TrailEffect->Activate(true);
	}
	else
	{
		TrailEffect->Deactivate();
	}
}

void AQuaffle::PickUp(AActor* PlayerActor)
{
	if (!HasAuthority() || !CanBePickedUpBy(PlayerActor)) return;

	StopProjectile();
	CollisionComponent->ClearMoveIgnoreActors();

	PossessingPlayer = PlayerActor;
	SetQuaffleState(EQuaffleState::Possessed);

	const ABroom* Broom = Cast<ABroom>(PlayerActor);
	USkeletalMeshComponent* RiderMesh = Broom ? Broom->GetRiderMesh() : nullptr;
	if (!RiderMesh) return;

	// Socket lookups on a rider without a mesh only log warnings, so fall back to the component origin.
	const FName Socket = QuaffleData && RiderMesh->GetSkinnedAsset() ? QuaffleData->SocketName : NAME_None;
	const FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(RiderMesh, AttachmentRules, Socket);

	if (QuaffleData)
	{
		SetActorRelativeLocation(QuaffleData->HeldOffset);
		SetActorRelativeRotation(QuaffleData->HeldRotationOffset);
	}
}

void AQuaffle::Drop(AActor* DropActor)
{
	if (!HasAuthority() || !CanBeThrown()) return;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	StopProjectile();

	PossessingPlayer = nullptr;
	SetQuaffleState(EQuaffleState::Free);

	if (DropActor)
	{
		CollisionComponent->IgnoreActorWhenMoving(DropActor, false);
	}
}

FVector AQuaffle::ComputeThrowStartLocation(const AActor* Holder) const
{
	const float ForwardOffset = QuaffleData ? QuaffleData->ThrowStartForwardOffset : 150.f;
	const float VerticalOffset = QuaffleData ? QuaffleData->ThrowStartVerticalOffset : 30.f;

	return Holder->GetActorLocation() + Holder->GetActorForwardVector() * ForwardOffset +
		FVector(0.f, 0.f, VerticalOffset);
}

bool AQuaffle::ThrowToTarget(const FVector& TargetLocation, float ArcBias)
{
	if (!HasAuthority() || !CanBeThrown() || !IsValid(PossessingPlayer)) return false;

	// Re-read every throw so live tuning (the gym sliders) applies to the next throw.
	ApplyFlightTuning();

	AActor* PreviousHolder = PossessingPlayer;
	const FVector ThrowStartLocation = ComputeThrowStartLocation(PreviousHolder);

	const FVector Delta = TargetLocation - ThrowStartLocation;
	const FVector DeltaXY(Delta.X, Delta.Y, 0.f);

	const float DistanceXY = DeltaXY.Size();
	if (DistanceXY <= KINDA_SMALL_NUMBER) return false;

	const float Gravity = FMath::Abs(GetWorld()->GetGravityZ()) * ProjectileMovement->ProjectileGravityScale;
	if (Gravity <= KINDA_SMALL_NUMBER) return false;

	const float MinArcTime = QuaffleData ? QuaffleData->MinThrowArcTime : 0.18f;
	const float MaxArcTime = QuaffleData ? QuaffleData->MaxThrowArcTime : 0.65f;
	const float TravelTime = FMath::Clamp(DistanceXY / ProjectileMovement->MaxSpeed * ArcBias, MinArcTime, MaxArcTime);

	// Solve the arc from the same point the ball is launched from (these used to differ).
	const FVector VelocityXY = DeltaXY / TravelTime;
	const float VelocityZ = (Delta.Z / TravelTime) + (0.5f * Gravity * TravelTime);
	const FVector LaunchVelocity = VelocityXY + FVector::UpVector * VelocityZ;

	return LaunchWithVelocity(LaunchVelocity, PreviousHolder, ThrowStartLocation);
}

bool AQuaffle::LaunchWithVelocity(const FVector& LaunchVelocity, AActor* PreviousHolder, const FVector& StartLocation)
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	LastThrownStartLocation = StartLocation;
	SetActorLocation(StartLocation);

	PossessingPlayer = nullptr;
	LastThrower = PreviousHolder;
	SetQuaffleState(EQuaffleState::Thrown);

	CollisionComponent->SetSimulatePhysics(false);
	CollisionComponent->IgnoreActorWhenMoving(PreviousHolder, true);

	ProjectileMovement->SetUpdatedComponent(CollisionComponent);
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Velocity = LaunchVelocity;
	ProjectileMovement->UpdateComponentVelocity();
	ProjectileMovement->Activate(true);

	const float MissDelay = QuaffleData ? QuaffleData->MissRespawnDelay : 3.f;
	GetWorldTimerManager().ClearTimer(MissRespawnTimerHandle);
	GetWorldTimerManager().SetTimer(MissRespawnTimerHandle, this, &AQuaffle::OnMissRespawn, MissDelay, false);

	return true;
}

void AQuaffle::StopProjectile()
{
	if (!ProjectileMovement) return;

	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Deactivate();
}

void AQuaffle::NotifyScored()
{
	if (!HasAuthority() || !CanScore()) return;

	ResetToSpawn();
}

void AQuaffle::OnMissRespawn()
{
	// Only reset if still in flight; a goal resets it immediately instead.
	if (!HasAuthority() || !CanScore()) return;

	ResetToSpawn();
}

void AQuaffle::ApplyFlightTuning()
{
	if (!QuaffleData) return;

	ProjectileMovement->MaxSpeed = QuaffleData->ThrowMaxSpeed;
	ProjectileMovement->ProjectileGravityScale = QuaffleData->ThrowGravityScale;
}

void AQuaffle::ResetToSpawn()
{
	if (!HasAuthority()) return;

	// A held Quaffle is released by its holder first so the broom doesn't keep a stale pointer.
	if (ABroom* Holder = Cast<ABroom>(PossessingPlayer); Holder && Holder->GetHeldQuaffle() == this)
	{
		Holder->DropQuaffle();
	}

	GetWorldTimerManager().ClearTimer(MissRespawnTimerHandle);

	StopProjectile();
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	CollisionComponent->ClearMoveIgnoreActors();

	Super::ResetToSpawn();
	PossessingPlayer = nullptr;
	LastThrower = nullptr;

	SetQuaffleState(EQuaffleState::Free);
}
