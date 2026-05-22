// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DataAsset/SoundDataAsset.h"
#include "SoundManager.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class QUIDPROWIZ_API USoundManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USoundManager();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayBroomHitByBludger();

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayQuafflePickup();

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayQuaffleThrow();

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayGoalScored();

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayMatchStart();

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayCountdownBeep();

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayMatchWon();

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayMatchLost();

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void StartBroomFlyingLoop();

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void StopBroomFlyingLoop();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	USoundDataAsset* SoundData;

private:

	UPROPERTY()
	UAudioComponent* FlyingLoopComponent = nullptr;

	void PlaySoundAtLocation(USoundBase* Sound);
	void PlaySound2D(USoundBase* Sound);
};
