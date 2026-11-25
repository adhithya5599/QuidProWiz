// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AIBroomController.generated.h"

class UBehaviorTree;
class UBroomAIDataAsset;
class ABroom;
class AQuaffle;
class ABludger;
class AGoalRing;

/**
 * Server-side opponent brain. Senses the world into the blackboard on an interval and exposes
 * high-level actions that behavior tree tasks call. All actions go through the broom's
 * Perform* API, so the AI obeys exactly the same rules as a player.
 */
UCLASS()
class QUIDPROWIZ_API AAIBroomController : public AAIController
{
	GENERATED_BODY()

public:

	AAIBroomController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaTime) override;

	void FlyToLocation(const FVector& TargetLocation, float DeltaTime);
	void AttemptPickup();
	void AttemptThrow();
	void EvadeBludger(float DeltaTime);

	ABroom* GetControlledBroom() const;
	UBroomAIDataAsset* GetAIData() const { return AIData; }

	// Blackboard keys (must match BB_BroomAI).
	static const FName BBKey_QuaffleLocation;
	static const FName BBKey_GoalLocation;
	static const FName BBKey_BludgerLocation;
	static const FName BBKey_IsHoldingQuaffle;
	static const FName BBKey_IsBludgerClose;
	static const FName BBKey_IsQuaffleFree;
	static const FName BBKey_TargetGoal;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
	TObjectPtr<UBroomAIDataAsset> AIData;

private:

	float TimeSinceLastUpdate = 0.f;

	void UpdateBlackboard();
	AGoalRing* FindBestGoal() const;
	ABludger* FindNearestBludger(float* OutDistance = nullptr) const;
};
