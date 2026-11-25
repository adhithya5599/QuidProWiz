// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ZoneType.h"
#include "QuidditchWorldSubsystem.generated.h"

class AGoalRing;
class AQuaffle;
class ABludger;
class ABroom;
class ASnitch;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnQuidditchGoalScored, AGoalRing* /*Goal*/, int32 /*ScoreValue*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnQuidditchZoneEntered, ABroom* /*Broom*/, EZoneType /*Zone*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnQuidditchSnitchCaught, ASnitch* /*Snitch*/, ABroom* /*Catcher*/);

/**
 * Per-world registry and event hub for match actors (Service Locator + Observer).
 *
 * Replaces the old static arrays/delegates on AGoalRing and ATestZone, which outlived
 * PIE sessions and were never unbound, and the GetAllActorsOfClass scans that assumed
 * a single Quaffle/Bludger. Every machine (server and each client) owns its own
 * instance; actors register in BeginPlay and unregister in EndPlay.
 */
UCLASS()
class QUIDPROWIZ_API UQuidditchWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	/** Convenience accessor; returns nullptr for worlds that don't run gameplay. */
	static UQuidditchWorldSubsystem* Get(const UObject* WorldContextObject);

	void RegisterGoal(AGoalRing* Goal);
	void UnregisterGoal(AGoalRing* Goal);
	void RegisterQuaffle(AQuaffle* Quaffle);
	void UnregisterQuaffle(AQuaffle* Quaffle);
	void RegisterBludger(ABludger* Bludger);
	void UnregisterBludger(ABludger* Bludger);
	void RegisterBroom(ABroom* Broom);
	void UnregisterBroom(ABroom* Broom);
	void RegisterSnitch(ASnitch* Snitch);
	void UnregisterSnitch(ASnitch* Snitch);

	TArray<AGoalRing*> GetGoals() const { return Resolve(Goals); }
	TArray<AQuaffle*> GetQuaffles() const { return Resolve(Quaffles); }
	TArray<ABludger*> GetBludgers() const { return Resolve(Bludgers); }
	TArray<ABroom*> GetBrooms() const { return Resolve(Brooms); }
	TArray<ASnitch*> GetSnitches() const { return Resolve(Snitches); }

	/** Middle of the pitch: the average position of all goal rings (or zero if there are none). */
	FVector GetArenaCenter() const;

	/** Nearest Quaffle that is currently free to pick up, or nullptr. */
	AQuaffle* FindNearestFreeQuaffle(const FVector& Location, float* OutDistance = nullptr) const;

	/** Nearest Bludger to Location, or nullptr. */
	ABludger* FindNearestBludger(const FVector& Location, float* OutDistance = nullptr) const;

	/** Raised on the server when a valid goal is scored. */
	FOnQuidditchGoalScored OnGoalScored;

	/** Raised on every machine when a broom enters a gym test zone. */
	FOnQuidditchZoneEntered OnZoneEntered;

	/** Raised on the server when a broom catches the Snitch. */
	FOnQuidditchSnitchCaught OnSnitchCaught;

protected:

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:

	TArray<TWeakObjectPtr<AGoalRing>> Goals;
	TArray<TWeakObjectPtr<AQuaffle>> Quaffles;
	TArray<TWeakObjectPtr<ABludger>> Bludgers;
	TArray<TWeakObjectPtr<ABroom>> Brooms;
	TArray<TWeakObjectPtr<ASnitch>> Snitches;

	template <typename T>
	static TArray<T*> Resolve(const TArray<TWeakObjectPtr<T>>& Entries)
	{
		TArray<T*> Result;
		Result.Reserve(Entries.Num());
		for (const TWeakObjectPtr<T>& Entry : Entries)
		{
			if (T* Object = Entry.Get())
			{
				Result.Add(Object);
			}
		}
		return Result;
	}
};
