#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AIBroomInterface.generated.h"

UENUM(BlueprintType)
enum class EAIAction : uint8
{
	FlyToLocation		UMETA(DisplayName = "Fly to Location"),
	EvadeBludger		UMETA(DisplayName = "Evade Bludger"),
	InterceptBludger	UMETA(DisplayName = "Intercept Bludger"),
	FollowPlayer		UMETA(DisplayName = "Follow Player"),
	PickupQuaffle		UMETA(DisplayName = "Pickup Quaffle"),
	ThrowQuaffle		UMETA(DisplayName = "Throw Quaffle")
};

UINTERFACE(MinimalAPI, Blueprintable)
class UAIBroomInterface : public UInterface
{
	GENERATED_BODY()
};

class QUIDPROWIZ_API IAIBroomInterface
{
	GENERATED_BODY()

public:

	virtual void FlyToLocation(const FVector& TargetLocation, float DeltaTime) = 0;

	virtual void EvadeBludger(float DeltaTime) {}
	virtual void InterceptBludger(float DeltaTime) {}
	virtual void FollowPlayer(float DeltaTime) {}
	virtual void AttemptPickup() {}
	virtual void AttemptThrow() {}

	virtual bool CanPerformAction(EAIAction Action) const { return false; }

	virtual FName GetLocationKeyForAction(EAIAction Action) const
	{
		return NAME_None;
	}

	virtual class ABroom* GetControlledBroom() const { return nullptr; }
	virtual class UAIBroomDataAssetBase* GetAIDataBase() const { return nullptr; }
};