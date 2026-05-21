// Fill out your copyright notice in the Description page of Project Settings.


#include "Quaffle.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Broom.h"
#include "TimerManager.h"

// Sets default values
AQuaffle::AQuaffle()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Collision presets
	CollisionComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CollisionComponent->SetSimulatePhysics(false);

	//Projectile movement presets
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bAutoActivate = false;
	ProjectileMovement->ProjectileGravityScale = .6f;

	CurrentState = MakeUnique<QuaffleStateFree>();
	PossessingPlayer = nullptr;
}

// Called when the game starts or when spawned
void AQuaffle::BeginPlay()
{
	Super::BeginPlay();

	InitialSpawnTransform = GetActorTransform();
	PreviousLocation = GetActorLocation();
	CurrentLocationCached = PreviousLocation;

	if (!QuaffleData) return;
}

// Called every frame
void AQuaffle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PreviousLocation = CurrentLocationCached;
	CurrentLocationCached = GetActorLocation();
}

bool AQuaffle::CanBePickedUpBy(AActor* PlayerActor) const
{
	return IsInPickupRange(PlayerActor) && CurrentState->CanPickUp();
}

bool AQuaffle::IsInPickupRange(AActor* PlayerActor) const
{
	if (!IsValid(PlayerActor)) return false;
	return FVector::Dist(GetActorLocation(), PlayerActor->GetActorLocation()) <= QuaffleData->PickupRadius;
}

void AQuaffle::PickUp(AActor* PlayerActor)
{
	if (!CanBePickedUpBy(PlayerActor)) return;

	PossessingPlayer = PlayerActor;
	SetState(MakeUnique<QuaffleStatePossessed>());

	if (ProjectileMovement)
	{
		ProjectileMovement->SetUpdatedComponent(CollisionComponent);
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ABroom* Broom = Cast<ABroom>(PlayerActor);
	if (!Broom) return;

	USkeletalMeshComponent* RiderMesh = Broom->GetRiderMesh();

	if (RiderMesh)
	{
		FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
		AttachToComponent(RiderMesh, AttachmentRules, QuaffleData->SocketName);
		SetActorRelativeLocation(QuaffleData->HeldOffset);
		SetActorRelativeRotation(QuaffleData->HeldRotationOffset);
	}
}

void AQuaffle::NotifyScored()
{
	if (!CurrentState->CanScore()) return;

	GetWorldTimerManager().ClearTimer(MissRespawnTimerHandle);

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	SetState(MakeUnique<QuaffleStateScored>());

	SetActorTransform(InitialSpawnTransform);
	PossessingPlayer = nullptr;
	LastThrower = nullptr;	

	TimeSinceLastPositionBroadcast = 0.f;

	SetState(MakeUnique<QuaffleStateFree>());

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void AQuaffle::Drop(AActor* DropActor)
{
	if (!CurrentState->CanThrow()) return;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	PossessingPlayer = nullptr;
	SetState(MakeUnique<QuaffleStateFree>());

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	if (DropActor)
	{
		CollisionComponent->IgnoreActorWhenMoving(DropActor, false);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}
}

bool AQuaffle::LaunchWithVelocity(const FVector& LaunchVelocity, AActor* PreviousHolder)
{
	if (!CurrentState->CanThrow() || !IsValid(PossessingPlayer)) return false;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	const FVector ThrowStartLocation = PreviousHolder->GetActorLocation() +
		PreviousHolder->GetActorForwardVector() * 150.f +
		FVector(0.f, 0.f, 3.f);

	LastThrownStartLocation = ThrowStartLocation;

	SetActorLocation(ThrowStartLocation);

	PossessingPlayer = nullptr;
	LastThrower = PreviousHolder;
	SetState(MakeUnique<QuaffleStateThrown>());

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetSimulatePhysics(false);
	CollisionComponent->IgnoreActorWhenMoving(PreviousHolder, true);

	if (ProjectileMovement)
	{
		ProjectileMovement->SetUpdatedComponent(CollisionComponent);
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();

		ProjectileMovement->Velocity = LaunchVelocity;
		ProjectileMovement->UpdateComponentVelocity();
		ProjectileMovement->Activate(true);
		
		TimeSinceLastPositionBroadcast = 0.f;

		GetWorldTimerManager().ClearTimer(MissRespawnTimerHandle);
		GetWorldTimerManager().SetTimer(MissRespawnTimerHandle, this, &AQuaffle::OnMissRespawn,
			MissRespawnDelay,false);
	}

	return true;
}

FVector AQuaffle::ComputeThrowStartLocation(AActor* Holder) const
{
	return Holder->GetActorLocation() +
		Holder->GetActorForwardVector() * QuaffleData->ThrowStartForwardOffset +
		FVector(0.f, 0.f, QuaffleData->ThrowStartVerticalOffset);
}

bool AQuaffle::ThrowToTarget(const FVector& TargetLocation, float ArcBias)
{
	if (!CurrentState->CanThrow() || !IsValid(PossessingPlayer)) return false;

	AActor* PreviousHolder = PossessingPlayer;
	const FVector ThrowStartLocation = ComputeThrowStartLocation(PreviousHolder);

	const FVector Delta = TargetLocation - ThrowStartLocation;
	const FVector DeltaXY(Delta.X, Delta.Y, 0.f);

	const float DistanceXY = DeltaXY.Size();
	if (DistanceXY <= KINDA_SMALL_NUMBER) return false;

	const float Gravity = FMath::Abs(GetWorld()->GetGravityZ()) * ProjectileMovement->ProjectileGravityScale;
	if (Gravity <= KINDA_SMALL_NUMBER) return false;

	float TravelTime = DistanceXY / ProjectileMovement->MaxSpeed;
	TravelTime = FMath::Clamp(TravelTime * ArcBias, QuaffleData->MinThrowArcTime, QuaffleData->MaxThrowArcTime);

	const FVector VelocityXY = DeltaXY / TravelTime;
	const float VelocityZ = (Delta.Z / TravelTime) + (0.5f * Gravity * TravelTime);
	const FVector LaunchVelocity = VelocityXY + FVector::UpVector * VelocityZ;

	return LaunchWithVelocity(LaunchVelocity, PreviousHolder);
}

void AQuaffle::OnMissRespawn()
{
	if (!HasAuthority()) return;

	// Only respawn if still in thrown state — if scored this won't fire
	if (!CurrentState->CanScore()) return;

	NotifyScored();
}