// Fill out your copyright notice in the Description page of Project Settings.


#include "AIBroomController.h"
#include "DataAsset/BroomAIDataAsset.h"
#include "Broom.h"
#include "Quaffle.h"
#include "Bludger.h"
#include "GoalRing.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/FloatingPawnMovement.h"

const FName AAIBroomController::BBKey_QuaffleLocation = TEXT("QuaffleLocation");
const FName AAIBroomController::BBKey_GoalLocation = TEXT("GoalLocation");
const FName AAIBroomController::BBKey_BludgerLocation = TEXT("BludgerLocation");
const FName AAIBroomController::BBKey_IsHoldingQuaffle = TEXT("IsHoldingQuaffle");
const FName AAIBroomController::BBKey_IsQuaffleFree = TEXT("IsQuaffleFree");
const FName AAIBroomController::BBKey_IsBludgerClose = TEXT("IsBludgerClose");
const FName AAIBroomController::BBKey_TargetGoal = TEXT("TargetGoal");

AAIBroomController::AAIBroomController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAIBroomController::BeginPlay()
{
	Super::BeginPlay();
	FindGameActors();
}

void AAIBroomController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!AIData) return;

	if (!BehaviorTree) return;

	RunBehaviorTree(BehaviorTree);
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

void AAIBroomController::FindGameActors()
{
	AActor* FoundQuaffle = UGameplayStatics::GetActorOfClass(GetWorld(), AQuaffle::StaticClass());
	QuaffleRef = Cast<AQuaffle>(FoundQuaffle);

	AActor* FoundBludger = UGameplayStatics::GetActorOfClass(GetWorld(), ABludger::StaticClass());
	BludgerRef = Cast<ABludger>(FoundBludger);
}

void AAIBroomController::UpdateBlackboard()
{
	if (!Blackboard) return;

	ABroom* Broom = GetControlledBroom();
	if (!Broom) return;

	if (QuaffleRef)
	{
		Blackboard->SetValueAsVector(BBKey_QuaffleLocation, QuaffleRef->GetActorLocation());
		Blackboard->SetValueAsBool(BBKey_IsQuaffleFree, QuaffleRef->CanBePickedUp());
	}

	if (BludgerRef)
	{
		const FVector BludgerLocation = BludgerRef->GetActorLocation();
		Blackboard->SetValueAsVector(BBKey_BludgerLocation, BludgerLocation);
		
		const float BludgerDistance = FVector::Dist(Broom->GetActorLocation(), BludgerLocation);
		Blackboard->SetValueAsBool(BBKey_IsBludgerClose, BludgerDistance <= AIData->BludgerEvadeDistance);
	}

	Blackboard->SetValueAsBool(BBKey_IsHoldingQuaffle, Broom->IsHoldingQuaffle());

	AGoalRing* BestGoal = FindBestGoal();
	if (BestGoal)
	{
		Blackboard->SetValueAsVector(BBKey_GoalLocation, BestGoal->GetAimDirection());
		Blackboard->SetValueAsObject(BBKey_TargetGoal, BestGoal);
	}
}

AGoalRing* AAIBroomController::FindBestGoal() const
{
	if (!AIData) return nullptr;

	AGoalRing* BestGoal = nullptr;
	float BestScore = -1.f;

	ABroom* Broom = GetControlledBroom();
	if (!Broom) return nullptr;

	for (const TWeakObjectPtr<AGoalRing>& GoalPtr : AGoalRing::GetRegisteredGoals())
	{
		AGoalRing* Goal = GoalPtr.Get();
		if (!Goal || !Goal->IsGoalEnabled()) continue;

		if (Goal->GetOwningTeam() != EGoalTeam::TeamA) continue;

		const float Distance = FVector::Dist(Broom->GetActorLocation(), Goal->GetActorLocation());

		const float GoalValue = Goal->GetScoreOverride() > 0 ? Goal->GetScoreOverride() : 10.f;
		const float Score = (GoalValue * AIData->HighValueGoalBias) - (Distance * 0.01f);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestGoal = Goal;
		}
	}

	return BestGoal;
}

ABroom* AAIBroomController::GetControlledBroom() const
{
	return Cast<ABroom>(GetPawn());
}

void AAIBroomController::FlyToLocation(const FVector& TargetLocation, float DeltaTime)
{
	ABroom* Broom = GetControlledBroom();
	if (!Broom || !AIData) return;

	Broom->GetBroomMovementComponent()->MaxSpeed = AIData->FlySpeed;

	const FVector CurrentLocation = Broom->GetActorLocation();
	const FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();
	const float Distance = FVector::Dist(CurrentLocation, TargetLocation);

	if (Distance < 50.f) return;

	const FVector Forward = Broom->GetActorForwardVector();
	const float ForwardDot = FVector::DotProduct(Forward, Direction);
	const float RightDot = FVector::DotProduct(Broom->GetActorRightVector(), Direction);
	const float UpDot = FVector::DotProduct(FVector::UpVector, Direction);

	Broom->PerformMove(ForwardDot > 0.f ? 1.f : -1.f);
	Broom->PerformAscend(FMath::Clamp(UpDot * 2.f, -1.f, 1.f));
	Broom->PerformTurn(FMath::Clamp(RightDot * AIData->TurnInterpSpeed, -1.f, 1.f));
}

void AAIBroomController::AttemptPickup()
{
	ABroom* Broom = GetControlledBroom();
	if (!Broom) return;

	Broom->PerformPickupQuaffle();
}

void AAIBroomController::AttemptThrow()
{
	ABroom* Broom = GetControlledBroom();
	if (!Broom || !AIData) return;

	const float AccuracyError = 1.f - AIData->ThrowAccuracy;
	const float ArcBias = FMath::RandRange(1.f - AccuracyError, 1.f + AccuracyError);

	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;

	AGoalRing* TargetGoal = Cast<AGoalRing>(BB->GetValueAsObject(TEXT("TargetGoal")));
	if (!TargetGoal) return;

	const FVector AimLocation = TargetGoal->GetAimDirection();

	AQuaffle* HeldQuaffle = Broom->GetHeldQuaffle();
	if (!HeldQuaffle) return;

	if (HeldQuaffle->ThrowToTarget(AimLocation, ArcBias))
	{
		Broom->NotifyQuaffleReleased();
	}

	Broom->PerformThrowQuaffle();
}

void AAIBroomController::EvadeBludger(float DeltaTime)
{
	ABroom* Broom = GetControlledBroom();
	if (!Broom || !BludgerRef || !AIData) return;

	const FVector AwayFromBludger = (Broom->GetActorLocation() - BludgerRef->GetActorLocation()).GetSafeNormal();
	const FVector EvadeTarget = Broom->GetActorLocation() + AwayFromBludger * AIData->BludgerEvadeStrength;

	FlyToLocation(EvadeTarget, DeltaTime);
}