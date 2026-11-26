// Fill out your copyright notice in the Description page of Project Settings.

#include "BallBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

ABallBase::ABallBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Balls are simulated on the server and replicated to clients.
	bReplicates = true;
	SetReplicatingMovement(true);
	SetNetUpdateFrequency(60.f);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	RootComponent = CollisionComponent;

	CollisionComponent->InitSphereRadius(32.f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetSimulatePhysics(false);

	BallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BallMesh"));
	BallMesh->SetupAttachment(RootComponent);
	BallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABallBase::BeginPlay()
{
	Super::BeginPlay();

	SpawnTransform = GetActorTransform();
}

void ABallBase::ResetToSpawn()
{
	if (!HasAuthority()) return;

	SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
}
