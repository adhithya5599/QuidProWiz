// Fill out your copyright notice in the Description page of Project Settings.


#include "AIBroomControllerBase.h"
#include "DataAsset/AIBroomDataAssetBase.h"
#include "Broom.h"
#include "Quaffle.h"
#include "Bludger.h"
#include "GoalRing.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/FloatingPawnMovement.h"

const FName AAIBroomControllerBase::BBKey_BludgerLocation = TEXT("BludgerLocation");
const FName AAIBroomControllerBase::BBKey_IsBludgerClose = TEXT("IsBludgerClose");
const FName AAIBroomControllerBase::BBKey_PlayerLocation = TEXT("PlayerLocation");

AAIBroomControllerBase::AAIBroomControllerBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAIBroomControllerBase::BeginPlay()
{
	Super::BeginPlay();
	FindGameActors();
}

void AAIBroomControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!AIDataBase) return;

	if (!BehaviorTree) return;

	bool bSuccess = RunBehaviorTree(BehaviorTree);
}

void AAIBroomControllerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!AIDataBase) return;

	TimeSinceLastUpdate += DeltaTime;
	if (TimeSinceLastUpdate >= AIDataBase->BlackboardUpdateInterval)
	{
		TimeSinceLastUpdate = 0.f;
		UpdateSharedBlackboard();
		UpdateBlackboard();
	}
}

void AAIBroomControllerBase::FindGameActors()
{
	TArray<AActor*> FoundBrooms;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABroom::StaticClass(), FoundBrooms);

	for (AActor* Actor : FoundBrooms)
	{
		ABroom* Broom = Cast<ABroom>(Actor);
		if (Broom && Broom->IsLocallyControlled())
		{
			PlayerBroom = Broom;
			break;
		}
	}

	AActor* FoundBludger = UGameplayStatics::GetActorOfClass(GetWorld(), ABludger::StaticClass());
	BludgerRef = Cast<ABludger>(FoundBludger);
}

void AAIBroomControllerBase::UpdateSharedBlackboard()
{
	if (!Blackboard) return;

	ABroom* Broom = GetControlledBroom();
	if (!Broom) return;

	if (PlayerBroom)
	{
		Blackboard->SetValueAsVector(BBKey_PlayerLocation, PlayerBroom->GetActorLocation());
	}

	if (BludgerRef)
	{
		const FVector BludgerLocation = BludgerRef->GetActorLocation();
		Blackboard->SetValueAsVector(BBKey_BludgerLocation, BludgerLocation);

		const float BludgerDistance = FVector::Dist(Broom->GetActorLocation(), BludgerLocation);
		Blackboard->SetValueAsBool(BBKey_IsBludgerClose, BludgerDistance <= AIDataBase->BludgerEvadeDistance);
	}
}

void AAIBroomControllerBase::UpdateBlackboard()
{

}

void AAIBroomControllerBase::FlyToLocation(const FVector& TargetLocation, float DeltaTime)
{
	ABroom* Broom = GetControlledBroom();
	if (!Broom || !AIDataBase) return;

	const FVector CurrentLocation = Broom->GetActorLocation();
	const FVector ToTarget = TargetLocation - CurrentLocation;
	const float Distance = ToTarget.Size();

	if (Distance < 50.f) return;

	const FVector Direction = ToTarget.GetSafeNormal();
	const FVector Forward = Broom->GetActorForwardVector();
	const FVector Right = Broom->GetActorRightVector();

	const float RightDot = FVector::DotProduct(Right, Direction);
	const float UpDot = FVector::DotProduct(FVector::UpVector, Direction);

	const float SlowDownRadius = 300.f;
	const float SpeedFactor = Distance < SlowDownRadius ? FMath::Clamp(Distance / SlowDownRadius, 0.3f, 1.f) : 1.f;

	Broom->GetBroomMovementComponent()->MaxSpeed = AIDataBase->FlySpeed * SpeedFactor;

	Broom->PerformMove(1.f);
	Broom->PerformTurn(FMath::Clamp(RightDot * 2.f, -1.f, 1.f));
	Broom->PerformAscend(FMath::Clamp(UpDot * 2.f, -1.f, 1.f));
}

void AAIBroomControllerBase::EvadeBludger(float DeltaTime)
{
	ABroom* Broom = GetControlledBroom();
	if (!Broom || !BludgerRef || !AIDataBase) return;

	const FVector AwayFromBludger = (Broom->GetActorLocation() - BludgerRef->GetActorLocation()).GetSafeNormal();

	const FVector EvadeTarget = Broom->GetActorLocation() + AwayFromBludger * AIDataBase->BludgerEvadeStrength;

	FlyToLocation(EvadeTarget, DeltaTime);
}

ABroom* AAIBroomControllerBase::GetControlledBroom() const
{
	return Cast<ABroom>(GetPawn());
}