// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AIBroomController.generated.h"

/**
 * 
 */

class UBehaviorTree;
class UBlackboardComponent;
class UBroomAIDataAsset;
class ABroom;
class AQuaffle;
class ABludger;
class AGoalRing;

UCLASS()
class QUIDPROWIZ_API AAIBroomController : public AAIController
{
	GENERATED_BODY()

public:

	AAIBroomController();

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

	void FlyToLocation(const FVector& TargetLocation, float DeltaTime);
	void AttemptPickup();
	void AttemptThrow();
	void EvadeBludger(float DeltaTime);

	ABroom* GetControlledBroom() const;
	UBroomAIDataAsset* GetAIData() const { return AIData; }

protected:

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	UBehaviorTree* BehaviorTree;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	UBroomAIDataAsset* AIData;

private:

	static const FName BBKey_QuaffleLocation;
	static const FName BBKey_GoalLocation;
	static const FName BBKey_BludgerLocation;
	static const FName BBKey_IsHoldingQuaffle;
	static const FName BBKey_IsBludgerClose;
	static const FName BBKey_IsQuaffleFree;
	static const FName BBKey_TargetGoal;

	UPROPERTY()
	AQuaffle* QuaffleRef = nullptr;

	UPROPERTY()
	ABludger* BludgerRef = nullptr;

	float TimeSinceLastUpdate = 0.f;

	void UpdateBlackboard();
	void FindGameActors();
	AGoalRing* FindBestGoal() const;
};
