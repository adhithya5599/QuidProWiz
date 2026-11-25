// Copyright Epic Games, Inc. All Rights Reserved.

#include "QuidditchTestWorld.h"
#include "QuidditchWorldSubsystem.h"
#include "Broom.h"
#include "BroomDataAsset.h"
#include "Quaffle.h"
#include "DataAsset/QuaffleDataAsset.h"
#include "Bludger.h"
#include "DataAsset/BludgerDataAsset.h"
#include "Snitch.h"
#include "DataAsset/SnitchDataAsset.h"
#include "DataAsset/GoalRingDataAsset.h"
#include "QuidProWizGameStateBase.h"
#include "AIBroomController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"

FQuidditchTestWorld::FQuidditchTestWorld()
{
	World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld*/ false, TEXT("QuidditchTestWorld"));
	check(World);

	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);

	World->InitializeActorsForPlay(FURL());
	World->GetWorldSettings()->NotifyBeginPlay();
}

FQuidditchTestWorld::~FQuidditchTestWorld()
{
	if (!World) return;

	World->EndPlay(EEndPlayReason::Quit);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(/*bInformEngineOfWorld*/ false);
	World->RemoveFromRoot();
	World = nullptr;
}

UQuidditchWorldSubsystem* FQuidditchTestWorld::GetSubsystem() const
{
	return World ? World->GetSubsystem<UQuidditchWorldSubsystem>() : nullptr;
}

void FQuidditchTestWorld::StepWorld(float StepSeconds)
{
	// Timers and tick functions run at most once per engine frame (keyed on GFrameCounter), so a
	// simulated frame has to advance the counter or only the first step would do anything.
	++GFrameCounter;
	World->Tick(LEVELTICK_All, StepSeconds);
}

void FQuidditchTestWorld::Tick(float Seconds, float StepSeconds)
{
	for (float Elapsed = 0.f; Elapsed < Seconds; Elapsed += StepSeconds)
	{
		StepWorld(StepSeconds);
	}
}

bool FQuidditchTestWorld::TickUntil(float MaxSeconds, TFunctionRef<bool()> Condition, float StepSeconds)
{
	for (float Elapsed = 0.f; Elapsed < MaxSeconds; Elapsed += StepSeconds)
	{
		if (Condition()) return true;
		StepWorld(StepSeconds);
	}
	return Condition();
}

void FQuidditchTestWorld::SetObjectProperty(UObject* Target, FName PropertyName, UObject* Value)
{
	FObjectPropertyBase* Property = Target ? FindFProperty<FObjectPropertyBase>(Target->GetClass(), PropertyName) : nullptr;
	check(Property);
	Property->SetObjectPropertyValue_InContainer(Target, Value);
}

template <typename TActor>
TActor* FQuidditchTestWorld::SpawnConfigured(const FTransform& Transform, TFunctionRef<void(TActor&)> Configure)
{
	TActor* Actor = World->SpawnActorDeferred<TActor>(TActor::StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	check(Actor);

	Configure(*Actor);
	Actor->FinishSpawning(Transform);
	return Actor;
}

ABroom* FQuidditchTestWorld::SpawnBroom(const FVector& Location, const FRotator& Rotation)
{
	return SpawnConfigured<ABroom>(FTransform(Rotation, Location), [](ABroom& Broom)
	{
		SetObjectProperty(&Broom, TEXT("BroomData"), NewObject<UBroomDataAsset>(GetTransientPackage()));
	});
}

AQuaffle* FQuidditchTestWorld::SpawnQuaffle(const FVector& Location)
{
	return SpawnConfigured<AQuaffle>(FTransform(Location), [](AQuaffle& Quaffle)
	{
		SetObjectProperty(&Quaffle, TEXT("QuaffleData"), NewObject<UQuaffleDataAsset>(GetTransientPackage()));
	});
}

ABludger* FQuidditchTestWorld::SpawnBludger(const FVector& Location)
{
	return SpawnConfigured<ABludger>(FTransform(Location), [](ABludger& Bludger)
	{
		SetObjectProperty(&Bludger, TEXT("BludgerDataAsset"), NewObject<UBludgerDataAsset>(GetTransientPackage()));
	});
}

ASnitch* FQuidditchTestWorld::SpawnSnitch(const FVector& Location)
{
	return SpawnConfigured<ASnitch>(FTransform(Location), [](ASnitch& Snitch)
	{
		SetObjectProperty(&Snitch, TEXT("SnitchData"), NewObject<USnitchDataAsset>(GetTransientPackage()));
	});
}

AGoalRing* FQuidditchTestWorld::SpawnGoal(const FVector& Location, const FRotator& Rotation, EGoalTeam Team)
{
	return SpawnConfigured<AGoalRing>(FTransform(Rotation, Location), [Team](AGoalRing& Goal)
	{
		SetObjectProperty(&Goal, TEXT("GoalDataAsset"), NewObject<UGoalRingDataAsset>(GetTransientPackage()));
		SetPropertyValue(&Goal, TEXT("OwningTeam"), Team);
	});
}

void FQuidditchTestWorld::Possess(ABroom* Broom)
{
	AAIBroomController* Controller = World->SpawnActor<AAIBroomController>();
	check(Controller);
	Controller->Possess(Broom);
}

AQuidProWizGameStateBase* FQuidditchTestWorld::SpawnGameState(int32 ScoreToWin, int32 CountdownDuration)
{
	UWorld* LocalWorld = World;
	return SpawnConfigured<AQuidProWizGameStateBase>(FTransform::Identity,
		[LocalWorld, ScoreToWin, CountdownDuration](AQuidProWizGameStateBase& GameState)
	{
		SetPropertyValue(&GameState, TEXT("ScoreToWin"), ScoreToWin);
		SetPropertyValue(&GameState, TEXT("CountdownDuration"), CountdownDuration);
		LocalWorld->SetGameState(&GameState);
	});
}
