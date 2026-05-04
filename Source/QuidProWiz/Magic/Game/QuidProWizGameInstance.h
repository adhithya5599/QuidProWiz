// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "QuidProWizGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class QUIDPROWIZ_API UQuidProWizGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Session")
	void HostGame();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void JoinGame(const FString& IPAddress);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Session")
	FString GameMapName = TEXT("/Game/Levels/Lvl_Gym");
	
};
