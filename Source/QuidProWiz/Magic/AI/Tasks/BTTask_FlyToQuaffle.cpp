// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_FlyToQuaffle.h"
#include "AIBroomController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Broom.h"
#include "DataAsset/BroomAIDataAsset.h"

UBTTask_FlyToQuaffle::UBTTask_FlyToQuaffle()
{
	NodeName = TEXT("Fly to Quaffle");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_FlyToQuaffle::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::InProgress;
}

void UBTTask_FlyToQuaffle::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIBroomController* Controller = Cast<AAIBroomController>(OwnerComp.GetAIOwner());
	if (!Controller)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}

	if (!BB->GetValueAsBool(TEXT("IsQuaffleFree")))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}

	const FVector QuaffleLocation = BB->GetValueAsVector(TEXT("QuaffleLocation"));
	Controller->FlyToLocation(QuaffleLocation, DeltaSeconds);

	ABroom* Broom = Controller->GetControlledBroom();
	if (Broom)
	{
		const float Distance = FVector::Dist(Broom->GetActorLocation(), QuaffleLocation);
		if (Distance <= Controller->GetAIData()->PickupApproachRadius)
		{
			Controller->AttemptPickup();

			if (Broom->IsHoldingQuaffle())
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
				return;
			}
		}
	}
}
