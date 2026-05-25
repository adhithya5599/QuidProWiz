
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

EBTNodeResult::Type UBTTask_PerformAction::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UE_LOG(LogTemp, Warning, TEXT("ExecuteTask called | Action: %d"), (int32)Action);

	AAIController* Controller = OwnerComp.GetAIOwner();
	if (!Controller)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecuteTask: No controller"));
		return EBTNodeResult::Failed;
	}

	UE_LOG(LogTemp, Warning, TEXT("ExecuteTask: Controller class: %s"),
		*Controller->GetClass()->GetName());

	if (!Controller->GetClass()->ImplementsInterface(UAIBroomInterface::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecuteTask: Interface not implemented"));
		return EBTNodeResult::Failed;
	}

	IAIBroomInterface* AI = Cast<IAIBroomInterface>(Controller);
	if (!AI)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecuteTask: Cast to interface failed"));
		return EBTNodeResult::Failed;
	}

	UE_LOG(LogTemp, Warning, TEXT("ExecuteTask: CanPerformAction %d = %d"),
		(int32)Action, (int32)AI->CanPerformAction(Action));

	if (!AI->CanPerformAction(Action))
	{
		return EBTNodeResult::Failed;
	}

	switch (Action)
	{
	case EAIAction::PickupQuaffle:
		UE_LOG(LogTemp, Warning, TEXT("ExecuteTask: PICKUP EXECUTING"));
		AI->AttemptPickup();
		return EBTNodeResult::Succeeded;

	case EAIAction::ThrowQuaffle:
		UE_LOG(LogTemp, Warning, TEXT("ExecuteTask: THROW EXECUTING"));
		AI->AttemptThrow();
		return EBTNodeResult::Succeeded;

	default:
		return EBTNodeResult::InProgress;
	}
}

void UBTTask_PerformAction::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	if (!Controller)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	IAIBroomInterface* AI = nullptr;
	if (Controller->GetClass()->ImplementsInterface(UAIBroomInterface::StaticClass()))
	{
		AI = Cast<IAIBroomInterface>(Controller);
	}

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

		case EAIAction::FlyToAndPickup:
		{
			const FName KeyToUse = LocationKeyName != NAME_None ? LocationKeyName :
				AI->GetLocationKeyForAction(EAIAction::PickupQuaffle);

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
				AI->AttemptPickup();

				if (Broom->IsHoldingQuaffle())
				{
					FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
				}
				return;
			}

			AI->FlyToLocation(Target, DeltaSeconds);
			break;
		}

		case EAIAction::FlyToAndThrow:
		{
			const FName KeyToUse = LocationKeyName != NAME_None ? LocationKeyName :
				AI->GetLocationKeyForAction(EAIAction::ThrowQuaffle);

			const FVector Target = BB->GetValueAsVector(KeyToUse);
			if (Target.IsZero())
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
				return;
			}

			ABroom* Broom = AI->GetControlledBroom();
			if (!Broom || !Broom->IsHoldingQuaffle())
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
				return;
			}

			const float Distance = FVector::Dist(Broom->GetActorLocation(), Target);

			if (Distance <= AcceptanceRadius)
			{
				AI->AttemptThrow();

				if (!Broom->IsHoldingQuaffle())
				{
					FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
				}
				return;
			}

			AI->FlyToLocation(Target, DeltaSeconds);
			break;
		}

		default:
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			break;
	}
}
