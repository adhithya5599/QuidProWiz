// Fill out your copyright notice in the Description page of Project Settings.


#include "QuidProWizGameInstance.h"
#include "Engine/Engine.h"

void UQuidProWizGameInstance::HostGame()
{
	if (!GetWorld()) return;

	UE_LOG(LogTemp, Log, TEXT("GameInstance: Hosting game on map %s"), *GameMapName);
	const FString TravelURL = FString::Printf(TEXT("%s?listen"), *GameMapName);
	GetWorld()->ServerTravel(TravelURL);
}

void UQuidProWizGameInstance::JoinGame(const FString& IPAddress)
{
    if (!GetWorld()) return;

    APlayerController* PlayerController = GetFirstLocalPlayerController();
    if (!PlayerController) return;

    // Append port if not already included
    FString FullAddress = IPAddress;
    if (!IPAddress.Contains(TEXT(":")))
    {
        FullAddress = FString::Printf(TEXT("%s:7777"), *IPAddress);
    }

    UE_LOG(LogTemp, Log, TEXT("GameInstance: Joining game at %s"), *FullAddress);
    PlayerController->ClientTravel(FullAddress, ETravelType::TRAVEL_Absolute);
}