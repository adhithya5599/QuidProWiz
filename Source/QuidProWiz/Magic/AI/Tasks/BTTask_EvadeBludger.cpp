// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_EvadeBludger.h"
#include "AIBroomController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_EvadeBludger::UBTTask_EvadeBludger()
{
	NodeName = TEXT("Evade Bludger");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_EvadeBludger::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EvadeTimer = 0.f;
	return EBTNodeResult::InProgress;
}

void UBTTask_EvadeBludger::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIBroomController* Controller = Cast<AAIBroomController>(OwnerComp.GetAIOwner());
	if (!Controller)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}

	EvadeTimer += DeltaSeconds;
	Controller->EvadeBludger(DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (BB && !BB->GetValueAsBool(TEXT("IsBludgerClose")))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	if (EvadeTimer >= EvadeDuration)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
