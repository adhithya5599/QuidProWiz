// Fill out your copyright notice in the Description page of Project Settings.


#include "TestZone.h"
#include "Components/BoxComponent.h"
#include "Broom.h"

FOnZoneEntered ATestZone::OnZoneEntered;

// Sets default values
ATestZone::ATestZone()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	ZoneVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneVolume"));
	RootComponent = ZoneVolume;

	ZoneVolume->SetBoxExtent(FVector(2500.f, 2500.f, 2000.f));
	ZoneVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZoneVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	ZoneVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ZoneVolume->SetGenerateOverlapEvents(true);
}

// Called when the game starts or when spawned
void ATestZone::BeginPlay()
{
	Super::BeginPlay();

	ZoneVolume->OnComponentBeginOverlap.AddDynamic(this, &ATestZone::OnZoneVolumeBeginOverlap);
	ZoneVolume->OnComponentEndOverlap.AddDynamic(this, &ATestZone::OnZoneVolumeEndOverlap);
}

void ATestZone::OnZoneVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, 
	const FHitResult& SweepResult)
{
	ABroom* Broom = Cast<ABroom>(OtherActor);
	if (!Broom) return;
	
	OnZoneEntered.Broadcast(ZoneType);
}

void ATestZone::OnZoneVolumeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABroom* Broom = Cast<ABroom>(OtherActor);
	if (!Broom) return;
}