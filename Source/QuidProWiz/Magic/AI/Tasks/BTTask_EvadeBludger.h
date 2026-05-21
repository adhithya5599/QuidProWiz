// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EvadeBludger.generated.h"

/**
 * 
 */
UCLASS()
class QUIDPROWIZ_API UBTTask_EvadeBludger : public UBTTaskNode
{
	GENERATED_BODY()
	
public:

	UBTTask_EvadeBludger();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:

	float EvadeTimer = 0.f;
	
	UPROPERTY(EditAnywhere, Category = "Task")
	float EvadeDuration = 1.5f;
};
