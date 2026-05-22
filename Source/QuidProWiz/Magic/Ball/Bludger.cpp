// Fill out your copyright notice in the Description page of Project Settings.


#include "Bludger.h"
#include "DataAsset/BludgerDataAsset.h"
#include "Broom.h"
#include "Quaffle.h"
#include "GoalRing.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ABludger::ABludger()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Overlap);
}

void ABludger::BeginPlay()
{
	Super::BeginPlay();

	if (!BludgerDataAsset) return;

	AActor* FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), AQuaffle::StaticClass());
	QuaffleRef = Cast<AQuaffle>(FoundActor);

	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ABludger::OnOverlapBegin);

	GetWorldTimerManager().SetTimer(SearchTimerHandle, this, &ABludger::SerchForTarget,
		BludgerDataAsset->SearchInterval, true);

	PickNewWanderTarget();
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

	ABroom* HitBroom = Cast<ABroom>(OtherActor);
	if (!HitBroom) return;

	OnHitBroom(HitBroom);
}

void ABludger::SerchForTarget()
{
	if (!BludgerDataAsset) return;

	if (bTargetLocked && IsTargetValid())
	{
		TargetLockTimer -= BludgerDataAsset->SearchInterval;
		if (TargetLockTimer > 0.f) return;
		bTargetLocked = false;
	}

	TArray<AActor*> FoundBrooms;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABroom::StaticClass(), FoundBrooms);

	ABroom* BestTarget = nullptr;
	float BestScore = -1.f;

	for (AActor* Actor : FoundBrooms)
	{
		ABroom* Broom = Cast<ABroom>(Actor);
		if (!Broom) return;

		if (!Broom->IsHoldingQuaffle()) continue;

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

void ABludger::ChaseTarget(float DeltaTime)
{
	if (!IsTargetValid()) return;
	if (!BludgerDataAsset) return;

	const FVector CurrentLocation = GetActorLocation();
	const FVector TargetLocation = TargetBroom->GetActorLocation();
	const FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();
	const float Distance = FVector::Dist(CurrentLocation, TargetLocation);

	const float TargetSpeed = Distance <= BludgerDataAsset->BoostActiveDistance
		? BludgerDataAsset->ChaseSpeed * BludgerDataAsset->BoostSpeedMultiplier
		: BludgerDataAsset->ChaseSpeed;

	const FVector TargetVelocity = Direction * TargetSpeed;
	CurrentVelocity = FMath::VInterpTo(CurrentVelocity, TargetVelocity, DeltaTime,
		BludgerDataAsset->ChaseAcceleration / TargetSpeed);

	FHitResult Hit;
	SetActorLocation(CurrentLocation + CurrentVelocity * DeltaTime, true, &Hit);

	const FRotator CurrentRotation = GetActorRotation();
	const FRotator TargetRotation = Direction.Rotation();
	const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime,
		BludgerDataAsset->RotationInterpSpeed);
	SetActorRotation(NewRotation);
}

bool ABludger::IsTargetValid() const
{
	return IsValid(TargetBroom) && TargetBroom->IsHoldingQuaffle();
}

void ABludger::PickNewWanderTarget()
{
	if (!BludgerDataAsset) return;

	const FVector RandomOffset = FVector(FMath::RandRange(-BludgerDataAsset->WanderRadius, BludgerDataAsset->WanderRadius),
		FMath::RandRange(-BludgerDataAsset->WanderRadius, BludgerDataAsset->WanderRadius), 
		FMath::RandRange(-BludgerDataAsset->WanderRadius * 0.5f, BludgerDataAsset->WanderRadius * 0.5f));

	WanderTarget = GetActorLocation() + RandomOffset;
}

void ABludger::ChaseWanderTarget(float DeltaTime)
{
	if (!BludgerDataAsset) return;

	const FVector CurrentLocation = GetActorLocation();
	const float Distance = FVector::Dist(CurrentLocation, WanderTarget);

	if (Distance < 100.f)
	{
		PickNewWanderTarget();
		return;
	}

	const FVector Direction = (WanderTarget - CurrentLocation).GetSafeNormal();
	const FVector TargetVelocity = Direction * BludgerDataAsset->WanderSpeed;
	CurrentVelocity = FMath::VInterpTo(CurrentVelocity, TargetVelocity, DeltaTime,
		2.f);

	FHitResult Hit;
	SetActorLocation(CurrentLocation + CurrentVelocity * DeltaTime, true, &Hit);

	const FRotator TargetRotation = Direction.Rotation();
	const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 
		BludgerDataAsset->RotationInterpSpeed);
	SetActorRotation(NewRotation);
}

void ABludger::ResetToRandomLocation()
{
	const FVector CurrentLocation = GetActorLocation();

	const FVector RandomOffset = FVector(FMath::RandRange(-ResetRadius, ResetRadius),
		FMath::RandRange(-ResetRadius, ResetRadius), 
		FMath::RandRange(ResetHeight * 0.5f, ResetHeight));

	const FVector NewLocation = CurrentLocation + RandomOffset;
	SetActorLocation(NewLocation);

	CurrentVelocity = FVector::ZeroVector;
	PickNewWanderTarget();
}

float ABludger::ScoreTarget(ABroom* Broom) const
{
	if (!Broom || !BludgerDataAsset) return -1.f;

	float Score = 0.f;

	if (!Broom->IsHoldingQuaffle()) return -1.f;
	Score += 10.f;

	if (Broom->IsLocallyControlled())
	{
		Score += BludgerDataAsset->HumanPlayerPriority * 10.f;
	}

	float ClosestGoalDistance = TNumericLimits<float>::Max();
	for (const TWeakObjectPtr<AGoalRing>& GoalPtr : AGoalRing::GetRegisteredGoals())
	{
		AGoalRing* Goal = GoalPtr.Get();
		if (!Goal) continue;

		const float Distance = FVector::Dist(Broom->GetActorLocation(), Goal->GetActorLocation());

		if (Distance < ClosestGoalDistance)
		{
			ClosestGoalDistance = Distance;
		}
	}

	if (ClosestGoalDistance < TNumericLimits<float>::Max())
	{
		const float GoalThreat = FMath::Clamp(1.f - (ClosestGoalDistance / 5000.f), 0.f, 1.f);
		Score += GoalThreat * BludgerDataAsset->GoalProximityWeight * 10.f;
	}

	const float DistanceToBroom = FVector::Dist(GetActorLocation(), Broom->GetActorLocation());
	Score -= DistanceToBroom * 0.001f;

	return Score;
}

bool ABludger::ShouldSwitchTarget(ABroom* NewTarget) const
{
	if (!NewTarget) return false;

	if (!IsTargetValid()) return true;

	if (NewTarget == TargetBroom) return false;

	const float CurrentScore = ScoreTarget(TargetBroom);
	const float NewScore = ScoreTarget(NewTarget);

	return NewScore > CurrentScore + BludgerDataAsset->TargetSwitchThreshold * 0.01f;
}

void ABludger::OnHitBroom(ABroom* HitBroom)
{
	if (!HitBroom) return;
	if (!HitBroom->IsHoldingQuaffle()) return;
	if(!bCanHit) return;
	if (!BludgerDataAsset) return;

	bCanHit = false;

	const FVector ImpulseDirection = (HitBroom->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	HitBroom->TriggerRagdoll(ImpulseDirection, BludgerDataAsset->RagDollImpulseStrength,
		BludgerDataAsset->RagDollDuration, BludgerDataAsset->RespawnDelay);

	ResetToRandomLocation();

	CurrentVelocity = FVector::ZeroVector;

	GetWorldTimerManager().ClearTimer(HitCooldownTimerHandle);
	GetWorldTimerManager().SetTimer(HitCooldownTimerHandle, this, &ABludger::ResetHitCoolDown,
		BludgerDataAsset->HitCooldown, false);
}

void ABludger::ResetHitCoolDown()
{
	bCanHit = true;
}