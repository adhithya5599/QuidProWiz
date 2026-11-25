// Fill out your copyright notice in the Description page of Project Settings.


#include "ZoneUIManager.h"
#include "QuidditchWorldSubsystem.h"
#include "Broom.h"

// Sets default values for this component's properties
UZoneUIManager::UZoneUIManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UZoneUIManager::BeginPlay()
{
	Super::BeginPlay();

	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		ZoneEnteredHandle = Subsystem->OnZoneEntered.AddUObject(this, &UZoneUIManager::HandleZoneEntered);
	}
}

void UZoneUIManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		Subsystem->OnZoneEntered.Remove(ZoneEnteredHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UZoneUIManager::HandleZoneEntered(ABroom* Broom, EZoneType NewZone)
{
	// Only react to this component's own broom, and only where a player sees the UI.
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || Broom != OwnerPawn || !OwnerPawn->IsLocallyControlled()) return;

	CurrentZone = NewZone;
	OnZoneChanged.Broadcast(NewZone);
}