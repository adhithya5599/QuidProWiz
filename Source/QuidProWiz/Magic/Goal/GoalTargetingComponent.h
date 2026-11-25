#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DataAsset/GoalTargetingDataAsset.h"
#include "GoalTargetingComponent.generated.h"

class AGoalRing;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class QUIDPROWIZ_API UGoalTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UGoalTargetingComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
		FActorComponentTickFunction* ThisTickFunction) override;

	AGoalRing* GetCurrentTargetGoal() const { return CurrentTargetGoal; }

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting")
	TObjectPtr<UGoalTargetingDataAsset> TargetingDataAsset = nullptr;

	virtual bool IsGoalValidTarget(const AGoalRing* Goal, const FVector& CameraLocation,
		const FVector& CameraForward) const;

	virtual float ScoreTargetGoal(const AGoalRing* Goal, const FVector& CameraLocation,
		const FVector& CameraForward) const;

private:

	float TimeUntilRetarget = 0.f;

	UPROPERTY()
	TObjectPtr<AGoalRing> CurrentTargetGoal = nullptr;

	AGoalRing* FindBestGoalTarget() const;
	void SetCurrentTargetGoal(AGoalRing* NewGoal);
	bool IsLocallyControlled() const;
};