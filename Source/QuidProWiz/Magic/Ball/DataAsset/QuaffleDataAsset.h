// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "QuaffleDataAsset.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class QUIDPROWIZ_API UQuaffleDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Gameplay")
	float PickupRadius = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Gameplay")
	float MinThrowArcTime = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Gameplay")
	float MaxThrowArcTime = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Gameplay")
	float ThrowStartForwardOffset = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Gameplay")
	float ThrowStartVerticalOffset = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Attachment")
	FName SocketName = TEXT("QuaffleSocket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Attachment")
	FVector HeldOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Quaffle|Attachment")
	FRotator HeldRotationOffset = FRotator::ZeroRotator;

};
