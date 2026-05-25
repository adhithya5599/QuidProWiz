#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "AIBroomInterface.h"
#include "BTTask_PerformAction.generated.h"

UCLASS()
class QUIDPROWIZ_API UBTTask_PerformAction : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_PerformAction();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Task")
	EAIAction Action = EAIAction::FlyToLocation;

	UPROPERTY(EditAnywhere, Category = "Task")
	FName LocationKeyName = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Task")
	float AcceptanceRadius = 100.f;
};