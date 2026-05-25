
#include "CompanionAIController.h"
#include "Broom.h"
#include "Bludger.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

const FName ACompanionAIController::BBKey_IsPlayerHoldingQuaffle = TEXT("IsPlayerHoldingQuaffle");
const FName ACompanionAIController::BBKey_InterceptLocation = TEXT("InterceptLocation");
const FName ACompanionAIController::BBKey_FollowLocation = TEXT("FollowLocation");
const FName ACompanionAIController::BBKey_IsBludgerThreatToPlayer = TEXT("IsBludgerThreatToPlayer");

ACompanionAIController::ACompanionAIController()
{
}

void ACompanionAIController::FollowPlayer(float DeltaTime)
{
	if (!Blackboard) return;

	const FVector FollowLocation = Blackboard->GetValueAsVector(BBKey_FollowLocation);

	FlyToLocation(FollowLocation, DeltaTime);
}

void ACompanionAIController::InterceptBludger(float DeltaTime)
{
	if (!Blackboard) return;

	const FVector InterceptLocation = Blackboard->GetValueAsVector(BBKey_InterceptLocation);

	FlyToLocation(InterceptLocation, DeltaTime);
}

void ACompanionAIController::FindGameActors()
{
	Super::FindGameActors();

	CompanionData = Cast<UCompanionAIDataAsset>(AIDataBase);
	if (!CompanionData) return;

}

void ACompanionAIController::UpdateBlackboard()
{
	if (!Blackboard) return;
	if (!PlayerBroom || !CompanionData) return;

	Blackboard->SetValueAsBool(BBKey_IsPlayerHoldingQuaffle, PlayerBroom->IsHoldingQuaffle());
	Blackboard->SetValueAsVector(BBKey_FollowLocation, ComputeFollowLocation());

	if (BludgerRef)
	{
		const float BludgerToPlayerDist = FVector::Dist(BludgerRef->GetActorLocation(), PlayerBroom->GetActorLocation());
		const bool bIsThreatToPlayer = BludgerToPlayerDist <= CompanionData->BludgerDefendDistance;

		Blackboard->SetValueAsBool(BBKey_IsBludgerThreatToPlayer, bIsThreatToPlayer);
		
		if (bIsThreatToPlayer)
		{
			Blackboard->SetValueAsVector(BBKey_InterceptLocation, ComputeInterceptLocation());
		}
	}
}

FVector ACompanionAIController::ComputeInterceptLocation() const
{
	if (!BludgerRef || !PlayerBroom || !CompanionData) return FVector::ZeroVector;

	const FVector BludgerLocation = BludgerRef->GetActorLocation();
	const FVector PlayerLocation = PlayerBroom->GetActorLocation();
	const FVector BludgerToPlayer = (PlayerLocation - BludgerLocation).GetSafeNormal();

	return PlayerLocation - BludgerToPlayer * CompanionData->InterceptOffset;
}

FVector ACompanionAIController::ComputeFollowLocation() const
{
	if (!PlayerBroom || !CompanionData) return FVector::ZeroVector;

	const FVector PlayerLocation = PlayerBroom->GetActorLocation();
	const FVector PlayerBackward = -PlayerBroom->GetActorForwardVector();
	const FVector PlayerRight = PlayerBroom->GetActorRightVector();

	return PlayerLocation + PlayerBackward * CompanionData->FollowDistance + PlayerRight *
		(CompanionData->FollowDistance * 0.5f) + FVector::UpVector * CompanionData->FollowHeight;
}

