// Fill out your copyright notice in the Description page of Project Settings.

#include "AIBroomController.h"
#include "DataAsset/BroomAIDataAsset.h"
#include "QuidditchWorldSubsystem.h"
#include "Broom.h"
#include "Quaffle.h"
#include "Bludger.h"
#include "GoalRing.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

const FName AAIBroomController::BBKey_QuaffleLocation = TEXT("QuaffleLocation");
const FName AAIBroomController::BBKey_GoalLocation = TEXT("GoalLocation");
const FName AAIBroomController::BBKey_BludgerLocation = TEXT("BludgerLocation");
const FName AAIBroomController::BBKey_IsHoldingQuaffle = TEXT("IsHoldingQuaffle");
const FName AAIBroomController::BBKey_IsQuaffleFree = TEXT("IsQuaffleFree");
const FName AAIBroomController::BBKey_IsBludgerClose = TEXT("IsBludgerClose");
const FName AAIBroomController::BBKey_TargetGoal = TEXT("TargetGoal");

namespace BroomAIConstants
{
	constexpr float ArrivalRadius = 50.f;
	constexpr float SlowDownRadius = 400.f;
	constexpr float MinApproachSpeedFactor = 0.3f;
	constexpr float SteeringGain = 2.f;
	constexpr float CorrectSideBonus = 50.f;
	constexpr float DistancePenaltyPerUnit = 0.01f;
}

AAIBroomController::AAIBroomController()
{
	PrimaryActorTick.bCanEverTick = true;

	// The broom steers by rotating this controller's yaw; don't snap it back to the pawn's facing.
	bSetControlRotationFromPawnOrientation = false;
}

void AAIBroomController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (ABroom* Broom = GetControlledBroom())
	{
		Broom->SetTeam(Team);
	}

	if (AIData && BehaviorTree)
	{
		RunBehaviorTree(BehaviorTree);
	}
}

void AAIBroomController::OnUnPossess()
{
	if (ABroom* Broom = GetControlledBroom())
	{
		Broom->SetSpeedOverride(0.f);
	}

	Super::OnUnPossess();
}

void AAIBroomController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!AIData) return;

	TimeSinceLastUpdate += DeltaTime;
	if (TimeSinceLastUpdate >= AIData->BlackboardUpdateInterval)
	{
		TimeSinceLastUpdate = 0.f;
		UpdateBlackboard();
	}
}

ABroom* AAIBroomController::GetControlledBroom() const
{
	return Cast<ABroom>(GetPawn());
}

ABludger* AAIBroomController::FindNearestBludger(float* OutDistance) const
{
	const ABroom* Broom = GetControlledBroom();
	const UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this);
	return Broom && Subsystem ? Subsystem->FindNearestBludger(Broom->GetActorLocation(), OutDistance) : nullptr;
}

void AAIBroomController::UpdateBlackboard()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	ABroom* Broom = GetControlledBroom();
	const UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this);
	if (!BB || !Broom || !Subsystem) return;

	if (const AQuaffle* FreeQuaffle = Subsystem->FindNearestFreeQuaffle(Broom->GetActorLocation()))
	{
		BB->SetValueAsVector(BBKey_QuaffleLocation, FreeQuaffle->GetActorLocation());
		BB->SetValueAsBool(BBKey_IsQuaffleFree, true);
	}
	else
	{
		BB->SetValueAsBool(BBKey_IsQuaffleFree, false);
	}

	float BludgerDistance = 0.f;
	if (const ABludger* Bludger = FindNearestBludger(&BludgerDistance))
	{
		BB->SetValueAsVector(BBKey_BludgerLocation, Bludger->GetActorLocation());
		BB->SetValueAsBool(BBKey_IsBludgerClose, BludgerDistance <= AIData->BludgerEvadeDistance);
	}
	else
	{
		BB->SetValueAsBool(BBKey_IsBludgerClose, false);
	}

	BB->SetValueAsBool(BBKey_IsHoldingQuaffle, Broom->IsHoldingQuaffle());

	if (AGoalRing* BestGoal = FindBestGoal())
	{
		BB->SetValueAsVector(BBKey_GoalLocation, BestGoal->GetAimDirection());
		BB->SetValueAsObject(BBKey_TargetGoal, BestGoal);
	}
}

AGoalRing* AAIBroomController::FindBestGoal() const
{
	const ABroom* Broom = GetControlledBroom();
	const UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this);
	if (!AIData || !Broom || !Subsystem) return nullptr;

	AGoalRing* BestGoal = nullptr;
	float BestScore = -TNumericLimits<float>::Max();
	const FVector BroomLocation = Broom->GetActorLocation();

	for (AGoalRing* Goal : Subsystem->GetGoals())
	{
		if (!Goal->IsGoalEnabled()) continue;

		// Attack the other team's rings only.
		if (Goal->GetOwningTeam() == Broom->GetTeam()) continue;

		const float Distance = FVector::Dist(BroomLocation, Goal->GetActorLocation());
		const float SideDot = FVector::DotProduct(BroomLocation - Goal->GetActorLocation(), Goal->GetActorForwardVector());
		const float SideBonus = SideDot < 0.f ? BroomAIConstants::CorrectSideBonus : 0.f;

		const float Score = Goal->GetScoreValue() * AIData->HighValueGoalBias + SideBonus
			- Distance * BroomAIConstants::DistancePenaltyPerUnit;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestGoal = Goal;
		}
	}

	return BestGoal;
}

void AAIBroomController::FlyToLocation(const FVector& TargetLocation, float DeltaTime)
{
	ABroom* Broom = GetControlledBroom();
	if (!Broom || !AIData) return;

	const FVector ToTarget = TargetLocation - Broom->GetActorLocation();
	const float Distance = ToTarget.Size();
	if (Distance < BroomAIConstants::ArrivalRadius) return;

	const FVector Direction = ToTarget.GetSafeNormal();
	const float RightDot = FVector::DotProduct(Broom->GetActorRightVector(), Direction);
	const float UpDot = FVector::DotProduct(FVector::UpVector, Direction);

	const float SpeedFactor = Distance < BroomAIConstants::SlowDownRadius
		? FMath::Clamp(Distance / BroomAIConstants::SlowDownRadius, BroomAIConstants::MinApproachSpeedFactor, 1.f)
		: 1.f;

	Broom->SetSpeedOverride(AIData->FlySpeed * SpeedFactor);
	Broom->PerformMove(1.f);
	Broom->PerformTurn(FMath::Clamp(RightDot * BroomAIConstants::SteeringGain, -1.f, 1.f));
	Broom->PerformAscend(FMath::Clamp(UpDot * BroomAIConstants::SteeringGain, -1.f, 1.f));
}

void AAIBroomController::AttemptPickup()
{
	if (ABroom* Broom = GetControlledBroom())
	{
		Broom->PerformPickupQuaffle();
	}
}

void AAIBroomController::AttemptThrow()
{
	ABroom* Broom = GetControlledBroom();
	const UBlackboardComponent* BB = GetBlackboardComponent();
	if (!Broom || !AIData || !BB) return;

	AGoalRing* TargetGoal = Cast<AGoalRing>(BB->GetValueAsObject(BBKey_TargetGoal));
	if (!TargetGoal) return;

	// Lower accuracy widens the random spread of the arc's flight time.
	const float AccuracyError = 1.f - AIData->ThrowAccuracy;
	const float ArcBias = FMath::RandRange(1.f - AccuracyError, 1.f + AccuracyError);

	Broom->ThrowQuaffleAtGoal(TargetGoal, ArcBias);
}

void AAIBroomController::EvadeBludger(float DeltaTime)
{
	ABroom* Broom = GetControlledBroom();
	const ABludger* Bludger = FindNearestBludger();
	if (!Broom || !Bludger || !AIData) return;

	const FVector AwayFromBludger = (Broom->GetActorLocation() - Bludger->GetActorLocation()).GetSafeNormal();
	FlyToLocation(Broom->GetActorLocation() + AwayFromBludger * AIData->BludgerEvadeStrength, DeltaTime);
}
