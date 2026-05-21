// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_CheckBool.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_CheckBool::UBTTask_CheckBool()
{
	NodeName = TEXT("Check Bool");
}

EBTNodeResult::Type UBTTask_CheckBool::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemmory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	if (KeyName == NAME_None) return EBTNodeResult::Failed;

	const bool bValue = BB->GetValueAsBool(KeyName);
	return bValue == bExpectedValue ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}
