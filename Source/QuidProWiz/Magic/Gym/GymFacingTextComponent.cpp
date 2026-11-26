// Fill out your copyright notice in the Description page of Project Settings.

#include "GymFacingTextComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

UGymFacingTextComponent::UGymFacingTextComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.f;
	SetHorizontalAlignment(EHTA_Center);
	SetVerticalAlignment(EVRTA_TextCenter);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
}

void UGymFacingTextComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Cosmetic and local: dedicated servers have no camera.
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Camera) return;

	// Text renders along +X of the component, so point +X back at the camera.
	FVector ToCamera = Camera->GetCameraLocation() - GetComponentLocation();
	if (bYawOnly)
	{
		ToCamera.Z = 0.f;
	}
	if (ToCamera.IsNearlyZero()) return;

	SetWorldRotation(ToCamera.Rotation());
}
