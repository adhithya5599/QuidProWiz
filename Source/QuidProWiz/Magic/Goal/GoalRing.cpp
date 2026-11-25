// Fill out your copyright notice in the Description page of Project Settings.

#include "GoalRing.h"
#include "DataAsset/GoalRingDataAsset.h"
#include "QuidditchWorldSubsystem.h"
#include "Quaffle.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

AGoalRing::AGoalRing()
{
	// Tick only runs the local highlight pulse, so it is enabled on demand.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
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

void AGoalRing::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		ScoreVolume->OnComponentBeginOverlap.AddDynamic(this, &AGoalRing::OnScoreVolumeBeginOverlap);
	}

	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		Subsystem->RegisterGoal(this);
	}

	if (GoalDataAsset)
	{
		LockOnVolume->SetBoxExtent(FVector(GoalDataAsset->LockOnVolumeExtent));
		ScoreVolume->SetBoxExtent(FVector(GoalDataAsset->ScoreVolumeExtent));
	}

	ApplyIdleMaterial();
}

void AGoalRing::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		Subsystem->UnregisterGoal(this);
	}

	GetWorldTimerManager().ClearTimer(ScoreFlashTimerHandle);
	Super::EndPlay(EndPlayReason);
}

int32 AGoalRing::GetScoreValue() const
{
	if (ScoreOverride > 0) return ScoreOverride;
	return GoalDataAsset ? GoalDataAsset->ScoreValue : 10;
}

void AGoalRing::OnScoreVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bGoalEnabled) return;

	AQuaffle* Quaffle = Cast<AQuaffle>(OtherActor);
	if (!Quaffle || !Quaffle->CanScore()) return;

	if (IsValidGoalScore(Quaffle))
	{
		RegisterScore(Quaffle);
	}
}

bool AGoalRing::IsValidGoalScore(const AQuaffle* Quaffle) const
{
	if (!Quaffle) return false;

	// The throw must start in front of the ring so balls can't score through the back.
	const float ThrowStartDot = FVector::DotProduct(
		Quaffle->GetLastThrownLocation() - GetActorLocation(), GetActorForwardVector());
	return ThrowStartDot < 0.f;
}

void AGoalRing::RegisterScore(AQuaffle* Quaffle)
{
	if (!Quaffle) return;

	Quaffle->NotifyScored();
	MulticastPlayScoreFeedback();

	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		Subsystem->OnGoalScored.Broadcast(this, GetScoreValue());
	}
}

void AGoalRing::MulticastPlayScoreFeedback_Implementation()
{
	if (!GoalDataAsset) return;

	if (GoalDataAsset->ScoreEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, GoalDataAsset->ScoreEffect,
			GetActorLocation(), GetActorRotation());
	}

	if (RingMesh && GoalDataAsset->ScoreFlashMaterial)
	{
		RingMesh->SetMaterial(0, GoalDataAsset->ScoreFlashMaterial);

		GetWorldTimerManager().ClearTimer(ScoreFlashTimerHandle);
		GetWorldTimerManager().SetTimer(ScoreFlashTimerHandle, this,
			&AGoalRing::ResetAfterScoreFlash, GoalDataAsset->ScoreFlashDuration, false);
	}
}

void AGoalRing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdatePulse(DeltaTime);
}

void AGoalRing::UpdatePulse(float DeltaTime)
{
	if (!RingMesh || !GoalDataAsset) return;

	PulseTimer += DeltaTime * GoalDataAsset->HighlightPulseSpeed;
	const float PulseValue = (FMath::Sin(PulseTimer) + 1.f) * 0.5f;
	const float NewScale = 1.f + PulseValue * GoalDataAsset->HighlightPulseIntensity;
	RingMesh->SetRelativeScale3D(FVector(NewScale));
}

void AGoalRing::ResetAfterScoreFlash()
{
	ApplyIdleMaterial();
}

void AGoalRing::ApplyIdleMaterial()
{
	if (!RingMesh || !GoalDataAsset) return;

	UMaterialInterface* Material = bLocallyHighlighted && GoalDataAsset->HighlightedMaterial
		? GoalDataAsset->HighlightedMaterial.Get()
		: GoalDataAsset->NormalMaterial.Get();

	if (Material)
	{
		RingMesh->SetMaterial(0, Material);
	}
}

FVector AGoalRing::GetAimDirection() const
{
	return AimDirectionComponent ? AimDirectionComponent->GetComponentLocation()
		: GetActorLocation() - GetActorForwardVector() * 75.f;
}

void AGoalRing::SetLocallyHighlighted(bool bHighlighted)
{
	if (bLocallyHighlighted == bHighlighted) return;

	bLocallyHighlighted = bHighlighted;
	PulseTimer = 0.f;
	SetActorTickEnabled(bHighlighted);

	if (!bHighlighted && RingMesh)
	{
		RingMesh->SetRelativeScale3D(FVector::OneVector);
	}

	// Don't stomp an in-progress score flash; it restores the idle material when it ends.
	if (!GetWorldTimerManager().IsTimerActive(ScoreFlashTimerHandle))
	{
		ApplyIdleMaterial();
	}
}
