// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BallBase.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS(Abstract)
class QUIDPROWIZ_API ABallBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABallBase();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball | Components")
	USphereComponent* CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball | Components")
	UStaticMeshComponent* BallMesh;
};
