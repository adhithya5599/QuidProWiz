// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_CheckBool.generated.h"

/**
 * 
 */
UCLASS()
class QUIDPROWIZ_API UBTTask_CheckBool : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_CheckBool();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemmory) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName KeyName =  NAME_None;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	bool bExpectedValue = true;
};
