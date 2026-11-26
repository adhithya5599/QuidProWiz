// Fill out your copyright notice in the Description page of Project Settings.

#include "Bludger.h"
#include "DataAsset/BludgerDataAsset.h"
#include "QuidditchWorldSubsystem.h"
#include "Broom.h"
#include "GoalRing.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

ABludger::ABludger()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Overlap);
}

void ABludger::BeginPlay()
{
	Super::BeginPlay();

	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		Subsystem->RegisterBludger(this);
	}

	// Clients only display the replicated Bludger.
	if (!HasAuthority() || !BludgerDataAsset)
	{
		SetActorTickEnabled(false);
		return;
	}

	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ABludger::OnOverlapBegin);

	GetWorldTimerManager().SetTimer(SearchTimerHandle, this, &ABludger::SearchForTarget,
		BludgerDataAsset->SearchInterval, true);

	PickNewWanderTarget();
}

void ABludger::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		Subsystem->UnregisterBludger(this);
	}

	GetWorldTimerManager().ClearTimer(SearchTimerHandle);
	GetWorldTimerManager().ClearTimer(HitCooldownTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void ABludger::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsTargetValid())
	{
		ChaseTarget(DeltaTime);
	}
	else
	{
		ChaseWanderTarget(DeltaTime);
	}
}

void ABludger::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bCanHit) return;

	if (ABroom* HitBroom = Cast<ABroom>(OtherActor))
	{
		OnHitBroom(HitBroom);
	}
}

void ABludger::SearchForTarget()
{
	if (!BludgerDataAsset) return;

	if (bTargetLocked && IsTargetValid())
	{
		TargetLockTimer -= BludgerDataAsset->SearchInterval;
		if (TargetLockTimer > 0.f) return;
		bTargetLocked = false;
	}

	const UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this);
	if (!Subsystem) return;

	ABroom* BestTarget = nullptr;
	float BestScore = -1.f;

	for (ABroom* Broom : Subsystem->GetBrooms())
	{
		const float Score = ScoreTarget(Broom);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Broom;
		}
	}

	if (BestTarget && ShouldSwitchTarget(BestTarget))
	{
		TargetBroom = BestTarget;
		bTargetLocked = true;
		TargetLockTimer = BludgerDataAsset->TargetLockDuration;
	}
	else if (!IsTargetValid())
	{
		TargetBroom = BestTarget;
	}
}

bool ABludger::IsTargetValid() const
{
	return IsValid(TargetBroom) && TargetBroom->IsHoldingQuaffle();
}

float ABludger::ScoreTarget(const ABroom* Broom) const
{
	if (!Broom || !BludgerDataAsset || !Broom->IsHoldingQuaffle()) return -1.f;

	float Score = 10.f;

	// IsPlayerControlled works on the server for remote players; IsLocallyControlled didn't.
	if (Broom->IsPlayerControlled())
	{
		Score += BludgerDataAsset->HumanPlayerPriority * 10.f;
	}

	if (const UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		float ClosestGoalDistance = TNumericLimits<float>::Max();
		for (const AGoalRing* Goal : Subsystem->GetGoals())
		{
			ClosestGoalDistance = FMath::Min(ClosestGoalDistance,
				FVector::Dist(Broom->GetActorLocation(), Goal->GetActorLocation()));
		}

		if (ClosestGoalDistance < TNumericLimits<float>::Max() && BludgerDataAsset->GoalThreatDistance > 0.f)
		{
			const float GoalThreat = FMath::Clamp(1.f - ClosestGoalDistance / BludgerDataAsset->GoalThreatDistance, 0.f, 1.f);
			Score += GoalThreat * BludgerDataAsset->GoalProximityWeight * 10.f;
		}
	}

	Score -= FVector::Dist(GetActorLocation(), Broom->GetActorLocation()) * 0.001f;
	return Score;
}

bool ABludger::ShouldSwitchTarget(const ABroom* NewTarget) const
{
	if (!NewTarget) return false;
	if (!IsTargetValid()) return true;
	if (NewTarget == TargetBroom) return false;

	return ScoreTarget(NewTarget) > ScoreTarget(TargetBroom) + BludgerDataAsset->TargetSwitchThreshold * 0.01f;
}

void ABludger::ChaseTarget(float DeltaTime)
{
	if (!IsTargetValid() || !BludgerDataAsset) return;

	const FVector DesiredVelocity = ComputeSeekForce(TargetBroom->GetActorLocation()) + ComputeAvoidanceForce();
	CurrentVelocity = FMath::VInterpTo(CurrentVelocity, DesiredVelocity, DeltaTime,
		BludgerDataAsset->ChaseSteeringInterpSpeed);

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetBroom->GetActorLocation());
	const float SpeedFactor = DistanceToTarget <= BludgerDataAsset->BoostActiveDistance
		? BludgerDataAsset->BoostSpeedMultiplier : 1.f;

	CurrentVelocity = CurrentVelocity.GetClampedToMaxSize(BludgerDataAsset->ChaseSpeed * SpeedFactor);

	MoveWithVelocity(DeltaTime, false);
}

void ABludger::ChaseWanderTarget(float DeltaTime)
{
	if (!BludgerDataAsset) return;

	if (FVector::Dist(GetActorLocation(), WanderTarget) < BludgerDataAsset->WanderArrivalRadius)
	{
		PickNewWanderTarget();
		return;
	}

	const FVector DesiredVelocity = ComputeWanderForce() + ComputeAvoidanceForce();
	CurrentVelocity = FMath::VInterpTo(CurrentVelocity, DesiredVelocity, DeltaTime,
		BludgerDataAsset->WanderSteeringInterpSpeed);

	MoveWithVelocity(DeltaTime, true);
}

void ABludger::MoveWithVelocity(float DeltaTime, bool bRepickWanderOnHit)
{
	FHitResult Hit;
	SetActorLocation(GetActorLocation() + CurrentVelocity * DeltaTime, true, &Hit);

	if (Hit.IsValidBlockingHit())
	{
		CurrentVelocity = FVector::VectorPlaneProject(CurrentVelocity, Hit.Normal) * 0.8f;
		if (bRepickWanderOnHit)
		{
			PickNewWanderTarget();
		}
	}

	if (!CurrentVelocity.IsNearlyZero())
	{
		const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), CurrentVelocity.Rotation(), DeltaTime,
			BludgerDataAsset->RotationInterpSpeed);
		SetActorRotation(NewRotation);
	}
}

void ABludger::PickNewWanderTarget()
{
	if (!BludgerDataAsset) return;

	const float Radius = BludgerDataAsset->WanderRadius;
	const FVector RandomOffset(FMath::RandRange(-Radius, Radius), FMath::RandRange(-Radius, Radius),
		FMath::RandRange(-Radius * 0.5f, Radius * 0.5f));

	WanderTarget = GetActorLocation() + RandomOffset;
}

FVector ABludger::ComputeAvoidanceForce() const
{
	if (!BludgerDataAsset) return FVector::ZeroVector;

	const UBludgerDataAsset& Data = *BludgerDataAsset;
	UWorld* World = GetWorld();
	const FVector Location = GetActorLocation();
	const FCollisionShape Probe = FCollisionShape::MakeSphere(Data.AvoidanceSphereRadius);
	const float AvoidScale = Data.ObstacleAvoidanceStrength * Data.ChaseSpeed;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(BludgerAvoidance), false, this);
	FVector AvoidanceForce = FVector::ZeroVector;

	// Floor and ceiling: probed once (they used to be re-probed and re-added for every forward ray).
	const float VerticalRange = Data.ObstacleDetectionRange * 0.5f;
	for (const FVector& Direction : { FVector::DownVector, FVector::UpVector })
	{
		FHitResult Hit;
		if (World->SweepSingleByChannel(Hit, Location, Location + Direction * VerticalRange, FQuat::Identity,
			ECC_WorldStatic, Probe, Params))
		{
			AvoidanceForce -= Direction * (1.f - Hit.Distance / VerticalRange) * AvoidScale;
		}
	}

	// Forward fan, ignoring the target so the Bludger doesn't swerve away from its prey.
	if (TargetBroom)
	{
		Params.AddIgnoredActor(TargetBroom);
	}

	const FVector Forward = GetActorForwardVector();
	const int32 NumRays = FMath::Max(Data.NumAvoidanceRays, 1);
	const float AngleStep = NumRays > 1 ? Data.AvoidanceArcDegrees / (NumRays - 1) : 0.f;
	const float StartAngle = NumRays > 1 ? -Data.AvoidanceArcDegrees * 0.5f : 0.f;

	for (int32 RayIndex = 0; RayIndex < NumRays; ++RayIndex)
	{
		const FVector RayDirection = Forward.RotateAngleAxis(StartAngle + AngleStep * RayIndex, FVector::UpVector);
		const FVector RayEnd = Location + RayDirection * Data.ObstacleDetectionRange;

		FHitResult Hit;
		const bool bHit = World->SweepSingleByChannel(Hit, Location, RayEnd, FQuat::Identity, ECC_WorldStatic,
			Probe, Params);

		if (bHit)
		{
			AvoidanceForce += Hit.Normal * (1.f - Hit.Distance / Data.ObstacleDetectionRange) * AvoidScale;
		}

		if (Data.bDrawDebug)
		{
			DrawDebugLine(World, Location, bHit ? Hit.ImpactPoint : RayEnd, bHit ? FColor::Red : FColor::Green,
				false, -1.f, 0, 1.f);
		}
	}

	return AvoidanceForce;
}

FVector ABludger::ComputeSeekForce(const FVector& TargetLocation) const
{
	if (!BludgerDataAsset) return FVector::ZeroVector;
	return (TargetLocation - GetActorLocation()).GetSafeNormal() * BludgerDataAsset->ChaseSpeed;
}

FVector ABludger::ComputeWanderForce() const
{
	if (!BludgerDataAsset) return FVector::ZeroVector;
	return (WanderTarget - GetActorLocation()).GetSafeNormal() * BludgerDataAsset->WanderSpeed;
}

void ABludger::OnHitBroom(ABroom* HitBroom)
{
	if (!HasAuthority() || !HitBroom || !bCanHit || !BludgerDataAsset) return;
	if (!HitBroom->IsHoldingQuaffle()) return;

	bCanHit = false;

	const FVector ImpulseDirection = (HitBroom->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	HitBroom->TriggerRagdoll(ImpulseDirection, BludgerDataAsset->RagDollImpulseStrength,
		BludgerDataAsset->RagDollDuration, BludgerDataAsset->RespawnDelay);

	MulticastPlayHitEffect(GetActorLocation());
	ResetToRandomLocation();

	GetWorldTimerManager().ClearTimer(HitCooldownTimerHandle);
	GetWorldTimerManager().SetTimer(HitCooldownTimerHandle, this, &ABludger::ResetHitCooldown,
		BludgerDataAsset->HitCooldown, false);
}

void ABludger::MulticastPlayHitEffect_Implementation(FVector_NetQuantize Location)
{
	if (BludgerDataAsset && BludgerDataAsset->HitEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BludgerDataAsset->HitEffect, Location);
	}
}

void ABludger::ResetToSpawn()
{
	if (!HasAuthority()) return;

	Super::ResetToSpawn();
	CurrentVelocity = FVector::ZeroVector;
	TargetBroom = nullptr;
	bTargetLocked = false;
	TargetLockTimer = 0.f;
	PickNewWanderTarget();
}

void ABludger::ResetToRandomLocation()
{
	if (!BludgerDataAsset) return;

	const float Radius = BludgerDataAsset->ResetRadius;
	const float Height = BludgerDataAsset->ResetHeight;
	const FVector RandomOffset(FMath::RandRange(-Radius, Radius), FMath::RandRange(-Radius, Radius),
		FMath::RandRange(Height * 0.5f, Height));

	SetActorLocation(GetActorLocation() + RandomOffset);
	CurrentVelocity = FVector::ZeroVector;
	PickNewWanderTarget();
}
