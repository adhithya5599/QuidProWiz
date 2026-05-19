// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZoneType.h"
#include "TestZone.generated.h"

class UBoxComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnZoneEntered, EZoneType);

UCLASS()
class QUIDPROWIZ_API ATestZone : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATestZone();

	static FOnZoneEntered OnZoneEntered;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	UBoxComponent* ZoneVolume;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	EZoneType ZoneType = EZoneType::None;

private:

	bool bPlayerInZone = false;

	UFUNCTION()
	void OnZoneVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnZoneVolumeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
