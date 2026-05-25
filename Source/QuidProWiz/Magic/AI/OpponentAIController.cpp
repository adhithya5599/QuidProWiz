
#include "OpponentAIController.h"
#include "GoalRing.h"
#include "Quaffle.h"
#include "Broom.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

const FName AOpponentAIController::BBKey_QuaffleLocation = TEXT("QuaffleLocation");
const FName AOpponentAIController::BBKey_GoalLocation = TEXT("GoalLocation");
const FName AOpponentAIController::BBKey_IsHoldingQuaffle = TEXT("IsHoldingQuaffle");
const FName AOpponentAIController::BBKey_IsQuaffleFree = TEXT("IsQuaffleFree");
const FName AOpponentAIController::BBKey_TargetGoal = TEXT("TargetGoal");

AOpponentAIController::AOpponentAIController()
{

}

void AOpponentAIController::FindGameActors()
{
	Super::FindGameActors();

	OpponentData = Cast<UOpponentAIDataAsset>(AIDataBase);
	if (!OpponentData) return;
}

void AOpponentAIController::UpdateBlackboard()
{
	if (!Blackboard) return;

	ABroom* Broom = GetControlledBroom();
	if (!Broom || !OpponentData) return;

	AQuaffle* NearestQuaffle = FindNearestFreeQuaffle();
	if (NearestQuaffle)
	{
		Blackboard->SetValueAsVector(BBKey_QuaffleLocation, NearestQuaffle->GetActorLocation());
		Blackboard->SetValueAsBool(BBKey_IsQuaffleFree, true);
	}
	else
	{
		Blackboard->SetValueAsBool(BBKey_IsQuaffleFree, false);
	}

	Blackboard->SetValueAsBool(BBKey_IsHoldingQuaffle, Broom->IsHoldingQuaffle());

	AGoalRing* BestGoal = FindBestGoal();
	if (BestGoal)
	{
		Blackboard->SetValueAsVector(BBKey_GoalLocation, BestGoal->GetAimDirection());
		Blackboard->SetValueAsObject(BBKey_TargetGoal, BestGoal);
	}
}

AQuaffle* AOpponentAIController::FindNearestFreeQuaffle() const
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AQuaffle::StaticClass(), FoundActors);

	AQuaffle* NearestQuaffle = nullptr;
	float NearestDistance = TNumericLimits<float>::Max();

	ABroom* Broom = GetControlledBroom();
	if (!Broom) return nullptr;

	for (AActor* Actor : FoundActors)
	{
		AQuaffle* Quaffle = Cast<AQuaffle>(Actor);
		if (!Quaffle || !Quaffle->CanBePickedUp()) continue;

		const float Distance = FVector::Dist(Broom->GetActorLocation(), Quaffle->GetActorLocation());

		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			NearestQuaffle = Quaffle;
		}
	}

	return NearestQuaffle;
}

AGoalRing* AOpponentAIController::FindBestGoal() const
{
	if (!OpponentData) return nullptr;

	ABroom* Broom = GetControlledBroom();
	if (!Broom) return nullptr;

	AGoalRing* BestGoal = nullptr;
	float BestScore = -1.f;

	for (const TWeakObjectPtr<AGoalRing>& GoalPtr : AGoalRing::GetRegisteredGoals())
	{
		AGoalRing* Goal = GoalPtr.Get();
		if (!Goal || !Goal->IsGoalEnabled()) continue;
		if (Goal->GetOwningTeam() != EGoalTeam::TeamA) continue;

		const float Distance = FVector::Dist(Broom->GetActorLocation(), Goal->GetActorLocation());

		const float GoalValue = Goal->GetScoreOverride() > 0 ? Goal->GetScoreOverride() : 10.f;

		const float SideDot = FVector::DotProduct(Broom->GetActorLocation() - Goal->GetActorLocation(),
			Goal->GetActorForwardVector());
		const float SideBonus = SideDot < 0.f ? 50.f : 0.f;

		const float Score = (GoalValue * OpponentData->HighValueGoalBias) + SideBonus - (Distance * 0.01f);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestGoal = Goal;
		}
	}

	return BestGoal;
}

void AOpponentAIController::AttemptPickup()
{
	ABroom* Broom = GetControlledBroom();
	if (!Broom) return;
	Broom->PerformPickupQuaffle();
}

void AOpponentAIController::AttemptThrow()
{
	ABroom* Broom = GetControlledBroom();
	if (!Broom || !OpponentData) return;

	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;

	AGoalRing* TargetGoal = Cast<AGoalRing>(BB->GetValueAsObject(BBKey_TargetGoal));
	if (!TargetGoal) return;

	const FVector AimLocation = TargetGoal->GetAimDirection();

	AQuaffle* HeldQuaffle = Broom->GetHeldQuaffle();
	if (!HeldQuaffle) return;

	const float AccuracyError = 1.f - OpponentData->ThrowAccuracy;
	const float ArcBias = FMath::RandRange(1.f - AccuracyError, 1.f + AccuracyError);

	if (HeldQuaffle->ThrowToTarget(AimLocation, ArcBias))
	{
		Broom->NotifyQuaffleReleased();
	}
}