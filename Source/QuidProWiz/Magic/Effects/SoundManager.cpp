// Fill out your copyright notice in the Description page of Project Settings.


#include "SoundManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

// Sets default values for this component's properties
USoundManager::USoundManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void USoundManager::BeginPlay()
{
	Super::BeginPlay();

	// ...

	if (!SoundData) return;

	StartBroomFlyingLoop();
	
}

void USoundManager::PlaySoundAtLocation(USoundBase* Sound)
{
	if (!Sound) return;
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound, GetOwner()->GetActorLocation());
}

void USoundManager::PlaySound2D(USoundBase* Sound)
{
	if (!Sound) return;
	UGameplayStatics::PlaySound2D(GetWorld(), Sound);
}

void USoundManager::PlayBroomHitByBludger()
{
	if (!SoundData) return;
	PlaySoundAtLocation(SoundData->QuafflePickup);
}

void USoundManager::PlayQuafflePickup()
{
	if (!SoundData) return;
	PlaySoundAtLocation(SoundData->QuaffleThrow);
}

void USoundManager::PlayQuaffleThrow()
{
	if (!SoundData) return;
	PlaySoundAtLocation(SoundData->GoalScored);
}

void USoundManager::PlayGoalScored()
{
	if (!SoundData) return;
	PlaySound2D(SoundData->GoalScored);
}

void USoundManager::PlayMatchStart()
{
	if (!SoundData) return;
	PlaySound2D(SoundData->MatchStart);
}

void USoundManager::PlayCountdownBeep()
{
	if (!SoundData) return;
	PlaySound2D(SoundData->CountDownBeep);
}

void USoundManager::PlayMatchWon()
{
	if (!SoundData) return;
	PlaySound2D(SoundData->MatchWon);
}

void USoundManager::PlayMatchLost()
{
	if (!SoundData) return;
	PlaySound2D(SoundData->MatchLost);
}

void USoundManager::StartBroomFlyingLoop()
{
	if (!SoundData || !SoundData->BroomFlyingLoop) return;

	if (FlyingLoopComponent)
	{
		FlyingLoopComponent->Stop();
		FlyingLoopComponent = nullptr;
	}

	FlyingLoopComponent = UGameplayStatics::SpawnSoundAttached(SoundData->BroomFlyingLoop,
		GetOwner()->GetRootComponent());
}

void USoundManager::StopBroomFlyingLoop()
{
	if (!FlyingLoopComponent) return;
	FlyingLoopComponent->FadeOut(0.5f, 0.f);
	FlyingLoopComponent = nullptr;
}