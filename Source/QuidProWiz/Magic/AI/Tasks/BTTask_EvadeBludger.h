// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EvadeBludger.generated.h"

/**
 * Flies away from the nearest Bludger until it is no longer close or EvadeDuration runs out.
 * Per-AI timing lives in node memory: task nodes are shared between every AI running the tree.
 */
UCLASS()
class QUIDPROWIZ_API UBTTask_EvadeBludger : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_EvadeBludger();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;

private:

	UPROPERTY(EditAnywhere, Category = "Task")
	float EvadeDuration = 1.5f;
};
