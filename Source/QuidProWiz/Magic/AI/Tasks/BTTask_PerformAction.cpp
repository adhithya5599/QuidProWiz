
#include "BTTask_PerformAction.h"
#include "AIBroomInterface.h"
#include "AIBroomControllerBase.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Broom.h"

UBTTask_PerformAction::UBTTask_PerformAction()
{
	NodeName = TEXT("Perform Action");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_PerformAction::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	IAIBroomInterface* AI = Cast<IAIBroomInterface>(OwnerComp.GetAIOwner());
	if (!AI) return EBTNodeResult::Failed;

	if (!AI->CanPerformAction(Action)) return EBTNodeResult::Failed;

	switch (Action)
	{
		case EAIAction::PickupQuaffle:
			AI->AttemptPickup();
			return EBTNodeResult::Succeeded;

		case EAIAction::ThrowQuaffle:
			AI->AttemptThrow();
			return EBTNodeResult::Succeeded;

		default:
			return EBTNodeResult::InProgress;
	}
}

void UBTTask_PerformAction::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	IAIBroomInterface* AI = Cast<IAIBroomInterface>(OwnerComp.GetAIOwner());
	if (!AI)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	switch (Action)
	{
		case EAIAction::FlyToLocation:
		{
			const FName KeyToUse = LocationKeyName != NAME_None ? LocationKeyName : AI->GetLocationKeyForAction(Action);
			const FVector Target = BB->GetValueAsVector(KeyToUse);
			if (Target.IsZero())
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
				return;
			}

			ABroom* Broom = AI->GetControlledBroom();
			if (!Broom)
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
				return;
			}

			const float Distance = FVector::Dist(Broom->GetActorLocation(), Target);
			if (Distance <= AcceptanceRadius)
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
				return;
			}

			AI->FlyToLocation(Target, DeltaSeconds);
			break;
		}

		case EAIAction::EvadeBludger:
			AI->EvadeBludger(DeltaSeconds);
			if (!BB->GetValueAsBool(TEXT("IsBludgerClose")))
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			}
			break;

		case EAIAction::InterceptBludger:
			AI->InterceptBludger(DeltaSeconds);
			if (!BB->GetValueAsBool(TEXT("IsBludgerThreatToPlayer")))
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			}
			break;

		case EAIAction::FollowPlayer:
			AI->FollowPlayer(DeltaSeconds);
			break;

		default:
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			break;
	}
}
