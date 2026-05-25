#pragma once

#include "CoreMinimal.h"
#include "AIBroomControllerBase.h"
#include "DataAsset/OpponentAIDataAsset.h"
#include "OpponentAIController.generated.h"

class AQuaffle;
class AGoalRing;

UCLASS()
class QUIDPROWIZ_API AOpponentAIController : public AAIBroomControllerBase
{
	GENERATED_BODY()

public:

	AOpponentAIController();

	virtual void AttemptPickup() override;
	virtual void AttemptThrow() override;

	virtual bool CanPerformAction(EAIAction Action) const override
	{
		switch (Action)
		{
			case EAIAction::FlyToLocation:
			case EAIAction::EvadeBludger:
			case EAIAction::PickupQuaffle:
			case EAIAction::ThrowQuaffle:
				return true;
			default:
				return false;
		}
	}

	virtual FName GetLocationKeyForAction(EAIAction Action) const override
	{
		switch (Action)
		{
			case EAIAction::PickupQuaffle:
				return BBKey_QuaffleLocation;
			case EAIAction::ThrowQuaffle:
				return BBKey_GoalLocation;
			default:
				return NAME_None;
		}
	}

	UOpponentAIDataAsset* GetOpponentData() const { return OpponentData; }

	static const FName BBKey_QuaffleLocation;
	static const FName BBKey_GoalLocation;
	static const FName BBKey_IsHoldingQuaffle;
	static const FName BBKey_IsQuaffleFree;
	static const FName BBKey_TargetGoal;

protected:

	virtual void FindGameActors() override;
	virtual void UpdateBlackboard() override;

private:

	UPROPERTY()
	UOpponentAIDataAsset* OpponentData = nullptr;

	AGoalRing* FindBestGoal() const;
	AQuaffle* FindNearestFreeQuaffle() const;
};