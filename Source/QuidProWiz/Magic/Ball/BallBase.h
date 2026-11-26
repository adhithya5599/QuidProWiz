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

	/** Where the ball was when play began. */
	const FTransform& GetSpawnTransform() const { return SpawnTransform; }

	/**
	 * Server: put the ball back where it started, in its starting state. Each ball type overrides
	 * this to clear its own flight state (Template Method); the base just teleports.
	 */
	virtual void ResetToSpawn();

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball | Components")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball | Components")
	TObjectPtr<UStaticMeshComponent> BallMesh;

private:

	FTransform SpawnTransform;
};
