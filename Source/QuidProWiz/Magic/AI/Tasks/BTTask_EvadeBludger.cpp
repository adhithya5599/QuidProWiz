// Fill out your copyright notice in the Description page of Project Settings.

#include "BTTask_EvadeBludger.h"
#include "AIBroomController.h"
#include "BehaviorTree/BlackboardComponent.h"

namespace
{
	struct FEvadeBludgerMemory
	{
		float ElapsedSeconds = 0.f;
	};
}

UBTTask_EvadeBludger::UBTTask_EvadeBludger()
{
	NodeName = TEXT("Evade Bludger");
	bNotifyTick = true;
}

uint16 UBTTask_EvadeBludger::GetInstanceMemorySize() const
{
	return sizeof(FEvadeBludgerMemory);
}

EBTNodeResult::Type UBTTask_EvadeBludger::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	reinterpret_cast<FEvadeBludgerMemory*>(NodeMemory)->ElapsedSeconds = 0.f;
	return EBTNodeResult::InProgress;
}

void UBTTask_EvadeBludger::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIBroomController* Controller = Cast<AAIBroomController>(OwnerComp.GetAIOwner());
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Controller || !BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FEvadeBludgerMemory* Memory = reinterpret_cast<FEvadeBludgerMemory*>(NodeMemory);
	Memory->ElapsedSeconds += DeltaSeconds;
	Controller->EvadeBludger(DeltaSeconds);

	if (!BB->GetValueAsBool(AAIBroomController::BBKey_IsBludgerClose) || Memory->ElapsedSeconds >= EvadeDuration)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
