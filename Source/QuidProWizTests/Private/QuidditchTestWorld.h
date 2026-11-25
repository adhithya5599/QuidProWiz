// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GoalRing.h"

class UWorld;
class UQuidditchWorldSubsystem;
class ABroom;
class AQuaffle;
class ABludger;
class AQuidProWizGameStateBase;

/**
 * A throwaway standalone game world for automation tests.
 *
 * Each test builds its own scenario (brooms, balls, rings, game state) from the C++ classes with
 * freshly created data assets at their default tuning, ticks time forward deterministically and
 * checks the outcome. The world is torn down when the object is destroyed, so tests can't leak
 * state into each other. Running standalone means this machine has authority, like a server.
 */
class FQuidditchTestWorld
{
public:

	FQuidditchTestWorld();
	~FQuidditchTestWorld();

	FQuidditchTestWorld(const FQuidditchTestWorld&) = delete;
	FQuidditchTestWorld& operator=(const FQuidditchTestWorld&) = delete;

	UWorld* GetWorld() const { return World; }
	UQuidditchWorldSubsystem* GetSubsystem() const;

	/** Advances the world by Seconds in fixed steps (timers, ticks, movement, overlaps). */
	void Tick(float Seconds, float StepSeconds = 1.f / 60.f);

	/** Ticks until Condition is true or MaxSeconds pass. Returns whether Condition became true. */
	bool TickUntil(float MaxSeconds, TFunctionRef<bool()> Condition, float StepSeconds = 1.f / 60.f);

	ABroom* SpawnBroom(const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);
	AQuaffle* SpawnQuaffle(const FVector& Location);
	ABludger* SpawnBludger(const FVector& Location);
	AGoalRing* SpawnGoal(const FVector& Location, const FRotator& Rotation, EGoalTeam Team = EGoalTeam::TeamA);

	/** Spawns the match game state and registers it as the world's game state. */
	AQuidProWizGameStateBase* SpawnGameState(int32 ScoreToWin, int32 CountdownDuration);

	/** Assigns a (possibly protected) reflected object property, e.g. an actor's data asset slot. */
	static void SetObjectProperty(UObject* Target, FName PropertyName, UObject* Value);

	/** Writes a plain (possibly protected) reflected property such as an int or enum. */
	template <typename TValue>
	static void SetPropertyValue(UObject* Target, FName PropertyName, const TValue& Value)
	{
		FProperty* Property = Target ? FindFProperty<FProperty>(Target->GetClass(), PropertyName) : nullptr;
		check(Property && Property->GetSize() == sizeof(TValue));
		*Property->ContainerPtrToValuePtr<TValue>(Target) = Value;
	}

private:

	UWorld* World = nullptr;

	void StepWorld(float StepSeconds);

	template <typename TActor>
	TActor* SpawnConfigured(const FTransform& Transform, TFunctionRef<void(TActor&)> Configure);
};
