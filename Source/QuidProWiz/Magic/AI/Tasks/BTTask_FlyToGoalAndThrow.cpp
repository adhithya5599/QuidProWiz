// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_FlyToGoalAndThrow.h"
#include "AIBroomController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Broom.h"
#include "DataAsset/BroomAIDataAsset.h"
#include "GoalRing.h"

UBTTask_FlyToGoalAndThrow::UBTTask_FlyToGoalAndThrow()
{
	NodeName = TEXT("Fly to goal and Throw");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_FlyToGoalAndThrow::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::InProgress;
}

void UBTTask_FlyToGoalAndThrow::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIBroomController* Controller = Cast<AAIBroomController>(OwnerComp.GetAIOwner());
	if (!Controller)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	ABroom* Broom = Controller->GetControlledBroom();
	if (!Broom || !Broom->IsHoldingQuaffle())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AGoalRing* TargetGoal = Cast<AGoalRing>(BB->GetValueAsObject(TEXT("TargetGoal")));
	if (!TargetGoal)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const FVector BroomLocation = Broom->GetActorLocation();
	const FVector GoalLocation = TargetGoal->GetActorLocation();
	const FVector GoalForward = TargetGoal->GetActorForwardVector();
	const float Distance = FVector::Dist(BroomLocation, GoalLocation);

	const FVector ToGoal = (GoalLocation - BroomLocation).GetSafeNormal();
	const float SideDot = FVector::DotProduct(BroomLocation - GoalLocation, GoalForward);
	const bool bOnCorrectSide = SideDot < 0.f;

	if (!bOnCorrectSide)
	{
		const FVector CorrectSideLocation = GoalLocation + (-GoalForward * 1000.f) + FVector(0.f, 0.f, GoalLocation.Z);
		Controller->FlyToLocation(CorrectSideLocation, DeltaSeconds);

		return;
	}

	if (Distance <= Controller->GetAIData()->ThrowRange)
	{
		Controller->AttemptThrow();

		if (!Broom->IsHoldingQuaffle())
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}
	}
	else
	{
		Controller->FlyToLocation(TargetGoal->GetAimDirection(), DeltaSeconds);
	}
}
