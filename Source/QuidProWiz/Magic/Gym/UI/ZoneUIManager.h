// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../ZoneType.h"
#include "ZoneUIManager.generated.h"

class ABroom;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZoneChanged, EZoneType, NewZone);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class QUIDPROWIZ_API UZoneUIManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UZoneUIManager();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(BlueprintAssignable, Category = "Zone")
	FOnZoneChanged OnZoneChanged;

	UFUNCTION(BlueprintCallable, Category = "Zone")
	EZoneType GetCurrentZone() const { return CurrentZone; }

private:

	EZoneType CurrentZone = EZoneType::None;
	FDelegateHandle ZoneEnteredHandle;

	void HandleZoneEntered(ABroom* Broom, EZoneType NewZone);
};
