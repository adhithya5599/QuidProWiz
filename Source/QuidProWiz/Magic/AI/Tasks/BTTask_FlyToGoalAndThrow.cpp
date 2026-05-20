// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_FlyToGoalAndThrow.h"
#include "AIBroomController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Broom.h"
#include "DataAsset/BroomAIDataAsset.h"

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

	const FVector GoalLocation = BB->GetValueAsVector(TEXT("GoalLocation"));
	const float Distance = FVector::Dist(Broom->GetActorLocation(), GoalLocation);

	if (Distance <= Controller->GetAIData()->ThrowRange)
	{
		Controller->AttemptThrow();

		if (!Broom->IsHoldingQuaffle())
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
	}
	else
	{
		Controller->FlyToLocation(GoalLocation, DeltaSeconds);
	}
}
