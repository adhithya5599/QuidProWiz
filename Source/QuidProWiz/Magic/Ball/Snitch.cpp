// Fill out your copyright notice in the Description page of Project Settings.

#include "Snitch.h"
#include "DataAsset/SnitchDataAsset.h"
#include "QuidditchWorldSubsystem.h"
#include "Broom.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"

namespace SnitchConstants
{
	constexpr float WanderArrivalRadius = 200.f;
	constexpr float JinkDecaySpeed = 2.f;
}

ASnitch::ASnitch()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Only brooms (overlap, to catch) and level geometry (block, to slide off walls) matter.
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetGenerateOverlapEvents(true);
}

void ASnitch::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASnitch, SnitchState);
}

const USnitchDataAsset& ASnitch::GetData() const
{
	return SnitchData ? *SnitchData : *GetDefault<USnitchDataAsset>();
}

void ASnitch::BeginPlay()
{
	Super::BeginPlay();

	CollisionComponent->SetSphereRadius(GetData().CatchRadius);

	if (HasAuthority())
	{
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ASnitch::OnOverlapBegin);
	}

	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		Subsystem->RegisterSnitch(this);
	}

	ApplyStateEffects();
}

void ASnitch::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		Subsystem->UnregisterSnitch(this);
	}

	Super::EndPlay(EndPlayReason);
}

// --- State ------------------------------------------------------------------------------------

void ASnitch::SetSnitchState(ESnitchState NewState)
{
	if (!HasAuthority()) return;

	SnitchState = NewState;
	ApplyStateEffects(); // OnRep doesn't fire on the server.
}

void ASnitch::OnRep_SnitchState()
{
	ApplyStateEffects();
}

void ASnitch::ApplyStateEffects()
{
	const bool bActive = SnitchState == ESnitchState::Active;

	SetActorHiddenInGame(!bActive);
	CollisionComponent->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

	// Only the server flies the Snitch; clients receive its replicated movement.
	SetActorTickEnabled(bActive && HasAuthority());
}

void ASnitch::Release(const FVector& InArenaCenter)
{
	if (!HasAuthority() || SnitchState != ESnitchState::Dormant) return;

	ArenaCenter = InArenaCenter;
	CurrentVelocity = FVector::ZeroVector;
	JinkVelocity = FVector::ZeroVector;
	TimeUntilJink = FMath::FRandRange(GetData().JinkIntervalMin, GetData().JinkIntervalMax);
	PickWanderTarget();

	SetSnitchState(ESnitchState::Active);
	MulticastPlayEffect(false);
}

void ASnitch::ResetToSpawn()
{
	if (!HasAuthority()) return;

	Super::ResetToSpawn();
	CurrentVelocity = FVector::ZeroVector;
	JinkVelocity = FVector::ZeroVector;
	SetSnitchState(ESnitchState::Dormant);
}

void ASnitch::SetSnitchData(USnitchDataAsset* InData)
{
	SnitchData = InData;
	CollisionComponent->SetSphereRadius(GetData().CatchRadius);
}

// --- Catching ---------------------------------------------------------------------------------

void ASnitch::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryCatch(Cast<ABroom>(OtherActor));
}

void ASnitch::TryCatch(ABroom* Broom)
{
	if (!HasAuthority() || !IsActive() || !Broom || Broom->IsStunned()) return;

	SetSnitchState(ESnitchState::Caught);
	MulticastPlayEffect(true);

	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		Subsystem->OnSnitchCaught.Broadcast(this, Broom);
	}
}

void ASnitch::MulticastPlayEffect_Implementation(bool bCaught)
{
	UNiagaraSystem* Effect = bCaught ? GetData().CaughtEffect.Get() : GetData().ReleaseEffect.Get();
	if (Effect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Effect, GetActorLocation());
	}
}

// --- Flight -----------------------------------------------------------------------------------

void ASnitch::PickWanderTarget()
{
	const USnitchDataAsset& Data = GetData();
	const FVector2D Offset = FMath::RandPointInCircle(Data.ArenaRadius);
	const float Height = FMath::FRandRange(Data.MinHeightOffset, Data.MaxHeightOffset);
	WanderTarget = ArenaCenter + FVector(Offset.X, Offset.Y, Height);
}

FVector ASnitch::ComputeDesiredVelocity() const
{
	const USnitchDataAsset& Data = GetData();
	const FVector Location = GetActorLocation();

	// Keep inside the arena: head back toward the centre when out of bounds.
	const FVector FromCenter = Location - ArenaCenter;
	if (FVector(FromCenter.X, FromCenter.Y, 0.f).Size() > Data.ArenaRadius ||
		FromCenter.Z < Data.MinHeightOffset || FromCenter.Z > Data.MaxHeightOffset)
	{
		return (WanderTarget - Location).GetSafeNormal() * Data.EvadeSpeed;
	}

	// Flee the nearest broom that gets too close.
	const ABroom* Nearest = nullptr;
	float NearestDistSq = FMath::Square(Data.EvadeRadius);
	if (const UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		for (const ABroom* Broom : Subsystem->GetBrooms())
		{
			const float DistSq = FVector::DistSquared(Location, Broom->GetActorLocation());
			if (DistSq < NearestDistSq)
			{
				NearestDistSq = DistSq;
				Nearest = Broom;
			}
		}
	}

	if (Nearest)
	{
		return (Location - Nearest->GetActorLocation()).GetSafeNormal() * Data.EvadeSpeed;
	}

	return (WanderTarget - Location).GetSafeNormal() * Data.CruiseSpeed;
}

void ASnitch::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsActive()) return;

	const USnitchDataAsset& Data = GetData();

	// Follow live tuning of the catch radius.
	if (!FMath::IsNearlyEqual(CollisionComponent->GetUnscaledSphereRadius(), Data.CatchRadius))
	{
		CollisionComponent->SetSphereRadius(Data.CatchRadius);
	}

	// A broom already touching the Snitch (e.g. one that just recovered from a stun) can catch it too.
	TArray<AActor*> Overlapping;
	CollisionComponent->GetOverlappingActors(Overlapping, ABroom::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		TryCatch(Cast<ABroom>(Actor));
		if (!IsActive()) return;
	}

	if (FVector::Dist(GetActorLocation(), WanderTarget) < SnitchConstants::WanderArrivalRadius)
	{
		PickWanderTarget();
	}

	CurrentVelocity = FMath::VInterpTo(CurrentVelocity, ComputeDesiredVelocity(), DeltaTime, Data.SteeringInterpSpeed);

	// Erratic sideways darts that fade out between jinks.
	TimeUntilJink -= DeltaTime;
	if (TimeUntilJink <= 0.f)
	{
		const FVector Forward = CurrentVelocity.IsNearlyZero() ? FVector::ForwardVector : CurrentVelocity.GetSafeNormal();
		FVector Side = FVector::CrossProduct(Forward, FVector::UpVector).GetSafeNormal();
		if (FMath::RandBool()) Side *= -1.f;
		JinkVelocity = (Side + FVector(0.f, 0.f, FMath::FRandRange(-0.5f, 0.5f))).GetSafeNormal() * Data.JinkStrength;
		TimeUntilJink = FMath::FRandRange(Data.JinkIntervalMin, Data.JinkIntervalMax);
	}
	JinkVelocity = FMath::VInterpTo(JinkVelocity, FVector::ZeroVector, DeltaTime, SnitchConstants::JinkDecaySpeed);

	const FVector Velocity = CurrentVelocity + JinkVelocity;

	FHitResult Hit;
	SetActorLocation(GetActorLocation() + Velocity * DeltaTime, true, &Hit);
	if (Hit.IsValidBlockingHit())
	{
		CurrentVelocity = FVector::VectorPlaneProject(CurrentVelocity, Hit.Normal);
		JinkVelocity = FVector::ZeroVector;
		PickWanderTarget();
	}

	if (!Velocity.IsNearlyZero())
	{
		SetActorRotation(Velocity.Rotation());
	}

	if (Data.bDrawDebug)
	{
		DrawDebugSphere(GetWorld(), WanderTarget, 50.f, 8, FColor::Yellow, false, -1.f);
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + Velocity * 0.25f, FColor::Orange, false, -1.f);
	}
}
