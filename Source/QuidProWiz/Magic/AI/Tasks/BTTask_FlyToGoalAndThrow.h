// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FlyToGoalAndThrow.generated.h"

/**
 * 
 */
UCLASS()
class QUIDPROWIZ_API UBTTask_FlyToGoalAndThrow : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_FlyToGoalAndThrow();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
