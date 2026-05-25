#pragma once

#include "CoreMinimal.h"
#include "AIBroomControllerBase.h"
#include "DataAsset/CompanionAIDataAsset.h"
#include "CompanionAIController.generated.h"

UCLASS()
class QUIDPROWIZ_API ACompanionAIController : public AAIBroomControllerBase
{
	GENERATED_BODY()

public:

	ACompanionAIController();

	virtual void FollowPlayer(float DeltaTime) override;
	virtual void InterceptBludger(float DeltaTime) override;

	virtual bool CanPerformAction(EAIAction Action) const override
	{
		switch (Action)
		{
			case EAIAction::FlyToLocation:
			case EAIAction::EvadeBludger:
			case EAIAction::InterceptBludger:
			case EAIAction::FollowPlayer:
				return true;
			default:
				return false;
		}
	}

	virtual FName GetLocationKeyForAction(EAIAction Action) const override
	{
		switch (Action)
		{
			case EAIAction::InterceptBludger:
				return BBKey_InterceptLocation;
			case EAIAction::FollowPlayer:
				return BBKey_FollowLocation;
			default:
				return NAME_None;
		}
	}

	UCompanionAIDataAsset* GetCompanionData() const { return CompanionData; }

	static const FName BBKey_IsPlayerHoldingQuaffle;
	static const FName BBKey_InterceptLocation;
	static const FName BBKey_FollowLocation;
	static const FName BBKey_IsBludgerThreatToPlayer;

protected:

	virtual void FindGameActors() override;
	virtual void UpdateBlackboard() override;

private:

	UPROPERTY()
	UCompanionAIDataAsset* CompanionData = nullptr;

	FVector ComputeInterceptLocation() const;
	FVector ComputeFollowLocation() const;
};