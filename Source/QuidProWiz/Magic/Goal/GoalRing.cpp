// Fill out your copyright notice in the Description page of Project Settings.


#include "GoalRing.h"
#include "Quaffle.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"

TArray<TWeakObjectPtr<AGoalRing>> AGoalRing::RegisteredGoals;
FOnGoalScored AGoalRing::OnGoalScored;

// Sets default values
AGoalRing::AGoalRing()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	RingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RingMesh"));
	RingMesh->SetupAttachment(Root);
	RingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LockOnVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("LockOnVolume"));
	LockOnVolume->SetupAttachment(Root);
	LockOnVolume->SetBoxExtent(FVector(300.0f, 300.0f, 300.0f));
	LockOnVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LockOnVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	LockOnVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	LockOnVolume->SetGenerateOverlapEvents(true);

	ScoreVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ScoreVolume"));
	ScoreVolume->SetupAttachment(Root);
	ScoreVolume->SetBoxExtent(FVector(30.f, 120.0f, 120.0f));
	ScoreVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ScoreVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	ScoreVolume->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	ScoreVolume->SetGenerateOverlapEvents(true);

	AimDirectionComponent = CreateDefaultSubobject<USceneComponent>(TEXT("AimDirectionComponent"));
	AimDirectionComponent->SetupAttachment(Root);
	AimDirectionComponent->SetRelativeLocation(FVector(-75.0f, 0.0f, 0.0f));
}

// Called when the game starts or when spawned
void AGoalRing::BeginPlay()
{
	Super::BeginPlay();
	
	ScoreVolume->OnComponentBeginOverlap.AddDynamic(this, &AGoalRing::OnScoreVolumeBeginOverlap);
	
	PurgeStaleGoals();
	RegisteredGoals.Add(this);

	if (GoalDataAsset && RingMesh)
	{
		RingMesh->SetMaterial(0, GoalDataAsset->NormalMaterial);

		LockOnVolume->SetBoxExtent(FVector(GoalDataAsset->LockOnVolumeExtent));
		ScoreVolume->SetBoxExtent(FVector(GoalDataAsset->ScoreVolumeExtent, GoalDataAsset->ScoreVolumeExtent,
			GoalDataAsset->ScoreVolumeExtent));
	}
}

void AGoalRing::OnScoreVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bGoalEnabled) return;

	AQuaffle* Quaffle = Cast<AQuaffle>(OtherActor);
	if (!Quaffle) return;

	if (!Quaffle->CanScore()) return;

	if (IsValidGoalScore(Quaffle))
	{
		RegisterScore(Quaffle);
	}
}

bool AGoalRing::IsValidGoalScore(AQuaffle* Quaffle) const
{
	if (!Quaffle) return false;

	const FVector RingLocation = GetActorLocation();
	const FVector RingForward = GetActorForwardVector();

	// Verify throw originated from the correct side
	const float ThrowStartDot = FVector::DotProduct(
		Quaffle->GetLastThrownLocation() - RingLocation, RingForward);
	const bool bThrownFromCorrectSide = ThrowStartDot < 0.f;

	return bThrownFromCorrectSide;
}

void AGoalRing::RegisterScore(AQuaffle* Quaffle)
{
	if (!Quaffle) return;

	const int32 ScoreValue = ScoreOverride > 0 ? ScoreOverride : 
		(GoalDataAsset ? GoalDataAsset->ScoreValue : 10);

	Quaffle->NotifyScored();
	TriggerScoreFlash();
	OnGoalScored.Broadcast(this, ScoreValue);
}

void AGoalRing::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RegisteredGoals.Remove(this);
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AGoalRing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bLocallyHighlighted)
	{
		UpdatePulse(DeltaTime);
	}
	else
	{
		PulseTimer = 0.f;
	}
}

void AGoalRing::UpdatePulse(float DeltaTime)
{
	if (!RingMesh) return;

	PulseTimer += DeltaTime * PulseSpeed;
	const float PulseValue = (FMath::Sin(PulseTimer) + 1.f) / 2.f;

	const float BaseScale = 1.f;
	const float NewScale = BaseScale + (PulseValue * PulseIntensity);
	RingMesh->SetRelativeScale3D(FVector(NewScale, NewScale, NewScale));
}

void AGoalRing::TriggerScoreFlash()
{
	if (!RingMesh || !ScoreFlashMaterial) return;

	RingMesh->SetMaterial(0, ScoreFlashMaterial);

	GetWorldTimerManager().ClearTimer(ScoreFlashTimerHandle);
	GetWorldTimerManager().SetTimer(ScoreFlashTimerHandle, this, 
		&AGoalRing::ResetAfterScoreFlash, ScoreFlashDuration, false);
}

void AGoalRing::ResetAfterScoreFlash()
{
	if (!RingMesh) return;

	if (GoalDataAsset && GoalDataAsset->NormalMaterial)
	{
		RingMesh->SetMaterial(0, GoalDataAsset->NormalMaterial);
	}
}

FVector AGoalRing::GetAimDirection() const
{
	return AimDirectionComponent ? AimDirectionComponent->GetComponentLocation() : 
		GetActorLocation() - (GetActorForwardVector() * 0.75f);
}

void AGoalRing::SetLocallyHighlighted(bool bHighlighted)
{
	if (bLocallyHighlighted == bHighlighted) return;

	bLocallyHighlighted = bHighlighted;

	if (!RingMesh) return;

	if (!bHighlighted)
	{
		RingMesh->SetRelativeScale3D(FVector::OneVector);
	}

	if (bLocallyHighlighted && GoalDataAsset && GoalDataAsset->HighlightedMaterial)
	{
		RingMesh->SetMaterial(0, GoalDataAsset->HighlightedMaterial);
	}
	else if (GoalDataAsset && GoalDataAsset->NormalMaterial)
	{
		RingMesh->SetMaterial(0, GoalDataAsset->NormalMaterial);
	}
}

void AGoalRing::PurgeStaleGoals()
{
	RegisteredGoals.RemoveAll([](const TWeakObjectPtr<AGoalRing>& GoalPtr)
	{
		return !GoalPtr.IsValid();
	});
}