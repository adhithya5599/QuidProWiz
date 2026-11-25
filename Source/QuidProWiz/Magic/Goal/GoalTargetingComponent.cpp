
#include "GoalTargetingComponent.h"
#include "GoalRing.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "Broom.h"
#include "QuidditchWorldSubsystem.h"

UGoalTargetingComponent::UGoalTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(false);
}

void UGoalTargetingComponent::BeginPlay()
{
	Super::BeginPlay();

	// Aim assist is purely local; TickComponent also skips brooms that aren't locally controlled.
	// (Possession can happen after BeginPlay, so local control can't be decided here.)
	if (!TargetingDataAsset)
	{
		SetComponentTickEnabled(false);
		return;
	}

	TimeUntilRetarget = TargetingDataAsset->RetargetInterval;
}

void UGoalTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType, 
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsLocallyControlled()) return;

	if (!TargetingDataAsset) return;

	TimeUntilRetarget -= DeltaTime;
	if (TimeUntilRetarget > 0.f) return;

	TimeUntilRetarget = TargetingDataAsset->RetargetInterval;

	AGoalRing* BestGoal = FindBestGoalTarget();
	SetCurrentTargetGoal(BestGoal);
}

bool UGoalTargetingComponent::IsLocallyControlled() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}

bool UGoalTargetingComponent::IsGoalValidTarget(const AGoalRing* Goal, const FVector& CameraLocation, 
	const FVector& CameraForward) const
{
	if (!Goal || !Goal->IsGoalEnabled() || !TargetingDataAsset) return false;

	const FVector ToCamera = (CameraLocation - Goal->GetActorLocation()).GetSafeNormal();
	const float GoalFacingDot = FVector::DotProduct(ToCamera, Goal->GetActorForwardVector());
	if (GoalFacingDot >= 0.f) return false;

	const float Distance = FVector::Dist(CameraLocation, Goal->GetAimDirection());
	if (Distance > TargetingDataAsset->TargetInRange) return false;

	const FVector ToGoalDirection = (Goal->GetAimDirection() - CameraLocation).GetSafeNormal();
	const float Dot = FVector::DotProduct(CameraForward, ToGoalDirection);
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));
	if (AngleDeg > TargetingDataAsset->MaxLockAngleDegrees) return false;

	return true;
}

float UGoalTargetingComponent::ScoreTargetGoal(const AGoalRing* Goal, const FVector& CameraLocation, 
	const FVector& CameraForward) const
{
	if (!Goal || !TargetingDataAsset) return TNumericLimits<float>::Max();

	const float Distance = FVector::Dist(CameraLocation, Goal->GetAimDirection());
	const FVector ToGoalDirection = (Goal->GetAimDirection() - CameraLocation).GetSafeNormal();
	const float Dot = FVector::DotProduct(CameraForward, ToGoalDirection);
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

	return (Distance * TargetingDataAsset->DistanceWeight) + (AngleDeg * TargetingDataAsset->AngleWeight);
}

AGoalRing* UGoalTargetingComponent::FindBestGoalTarget() const
{
	const ABroom* OwnerBroom = Cast<ABroom>(GetOwner());
	if (!OwnerBroom) return nullptr;

	const UCameraComponent* Camera = OwnerBroom->GetBroomCamera();
	if (!Camera) return nullptr;

	const FVector CameraLocation = Camera->GetComponentLocation();
	const FVector CameraForward = Camera->GetForwardVector();

	AGoalRing* BestGoal = nullptr;
	float BestScore = TNumericLimits<float>::Max();

	const UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this);
	if (!Subsystem) return nullptr;

	for (AGoalRing* Goal : Subsystem->GetGoals())
	{
		if (!IsGoalValidTarget(Goal, CameraLocation, CameraForward)) continue;

		const float Score = ScoreTargetGoal(Goal, CameraLocation, CameraForward);
		if (Score < BestScore)
		{
			BestScore = Score;
			BestGoal = Goal;
		}
	}

	return BestGoal;
}

void UGoalTargetingComponent::SetCurrentTargetGoal(AGoalRing* NewGoal)
{
	if (CurrentTargetGoal == NewGoal) return;

	if (CurrentTargetGoal)
	{
		CurrentTargetGoal->SetLocallyHighlighted(false);
	}

	CurrentTargetGoal = NewGoal;
	if (CurrentTargetGoal)
	{
		CurrentTargetGoal->SetLocallyHighlighted(true);
	}
}