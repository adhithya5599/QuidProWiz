// Fill out your copyright notice in the Description page of Project Settings.

#include "GymResetPad.h"
#include "GymSubsystem.h"
#include "GymFacingTextComponent.h"
#include "Broom.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AGymResetPad::AGymResetPad()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	RootComponent = Trigger;
	Trigger->SetBoxExtent(FVector(250.f, 250.f, 200.f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);

	PadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PadMesh"));
	PadMesh->SetupAttachment(Trigger);
	PadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PadMesh->SetRelativeLocation(FVector(0.f, 0.f, -195.f));
	PadMesh->SetRelativeScale3D(FVector(5.f, 5.f, 0.05f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		PadMesh->SetStaticMesh(Cylinder.Object);
	}

	LabelText = CreateDefaultSubobject<UGymFacingTextComponent>(TEXT("LabelText"));
	LabelText->SetupAttachment(Trigger);
	LabelText->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	LabelText->SetWorldSize(60.f);
}

void AGymResetPad::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	LabelText->SetText(Label);
	LabelText->SetTextRenderColor(LabelColor.ToFColor(true));
}

void AGymResetPad::BeginPlay()
{
	Super::BeginPlay();

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGymResetPad::OnTriggerBeginOverlap);
}

void AGymResetPad::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !Cast<ABroom>(OtherActor)) return;

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFiredTime < Cooldown) return;
	LastFiredTime = Now;

	if (UGymSubsystem* Gym = UGymSubsystem::Get(this))
	{
		FGymActionEntry Entry;
		Entry.Action = Action;
		Entry.Label = Label;
		Gym->ExecuteAction(Entry);
	}
}
