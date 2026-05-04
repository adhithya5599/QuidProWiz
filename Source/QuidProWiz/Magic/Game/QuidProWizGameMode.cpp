// Fill out your copyright notice in the Description page of Project Settings.


#include "QuidProWizGameMode.h"
#include "QuidProWizGameStateBase.h"

AQuidProWizGameMode::AQuidProWizGameMode()
{
	GameStateClass = AQuidProWizGameStateBase::StaticClass();
}