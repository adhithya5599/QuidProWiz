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
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ABroom* Broom = Controller ? Controller->GetControlledBroom() : nullptr;

	// Each failure path returns: the old version kept running and dereferenced null pointers.
	if (!Controller || !BB || !Broom || !Controller->GetAIData() ||
		!BB->GetValueAsBool(AAIBroomController::BBKey_IsQuaffleFree))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const FVector QuaffleLocation = BB->GetValueAsVector(AAIBroomController::BBKey_QuaffleLocation);
	Controller->FlyToLocation(QuaffleLocation, DeltaSeconds);

	if (FVector::Dist(Broom->GetActorLocation(), QuaffleLocation) <= Controller->GetAIData()->PickupApproachRadius)
	{
		Controller->AttemptPickup();

		if (Broom->IsHoldingQuaffle())
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
	}
}
