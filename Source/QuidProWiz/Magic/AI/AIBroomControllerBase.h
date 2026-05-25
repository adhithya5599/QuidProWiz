// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "DataAsset/AIBroomDataAssetBase.h"
#include "AIBroomInterface.h"
#include "AIBroomControllerBase.generated.h"

/**
 * 
 */

class UBehaviorTree;
class ABroom;
class AQuaffle;
class ABludger;
class AGoalRing;

UCLASS(Abstract)
class QUIDPROWIZ_API AAIBroomControllerBase : public AAIController, public IAIBroomInterface
{
	GENERATED_BODY()

public:

	AAIBroomControllerBase();

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

	virtual void FlyToLocation(const FVector& TargetLocation, float DeltaTime) override;
	virtual void EvadeBludger(float DeltaTime) override;
	virtual ABroom* GetControlledBroom() const override;
	
	virtual UAIBroomDataAssetBase* GetAIDataBase() const override { return AIDataBase; }

	ABludger* GetBludger() const { return BludgerRef; }

	static const FName BBKey_BludgerLocation;
	static const FName BBKey_IsBludgerClose;
	static const FName BBKey_PlayerLocation;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	UBehaviorTree* BehaviorTree;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
	UAIBroomDataAssetBase* AIDataBase;

	virtual void FindGameActors();

	virtual void UpdateBlackboard();

	UPROPERTY()
	ABroom* PlayerBroom = nullptr;

	UPROPERTY()
	ABludger* BludgerRef = nullptr;

private:

	float TimeSinceLastUpdate = 0.f;

	void UpdateSharedBlackboard();
};
