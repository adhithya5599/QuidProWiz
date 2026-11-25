// Fill out your copyright notice in the Description page of Project Settings.

#include "QuidditchWorldSubsystem.h"
#include "Engine/World.h"
#include "GoalRing.h"
#include "Quaffle.h"
#include "Bludger.h"
#include "Broom.h"

namespace QuidditchRegistry
{
	template <typename T>
	void Add(TArray<TWeakObjectPtr<T>>& Entries, T* Object)
	{
		if (!Object) return;
		Entries.RemoveAll([](const TWeakObjectPtr<T>& Entry) { return !Entry.IsValid(); });
		Entries.AddUnique(Object);
	}

	template <typename T>
	void Remove(TArray<TWeakObjectPtr<T>>& Entries, T* Object)
	{
		Entries.RemoveAll([Object](const TWeakObjectPtr<T>& Entry)
		{
			return !Entry.IsValid() || Entry.Get() == Object;
		});
	}

	template <typename T>
	T* FindNearest(const TArray<T*>& Candidates, const FVector& Location, float* OutDistance,
		TFunctionRef<bool(const T&)> Filter)
	{
		T* Nearest = nullptr;
		float NearestDistSq = TNumericLimits<float>::Max();

		for (T* Candidate : Candidates)
		{
			if (!Candidate || !Filter(*Candidate)) continue;

			const float DistSq = FVector::DistSquared(Location, Candidate->GetActorLocation());
			if (DistSq < NearestDistSq)
			{
				NearestDistSq = DistSq;
				Nearest = Candidate;
			}
		}

		if (OutDistance)
		{
			*OutDistance = Nearest ? FMath::Sqrt(NearestDistSq) : TNumericLimits<float>::Max();
		}
		return Nearest;
	}
}

UQuidditchWorldSubsystem* UQuidditchWorldSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UQuidditchWorldSubsystem>() : nullptr;
}

bool UQuidditchWorldSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UQuidditchWorldSubsystem::RegisterGoal(AGoalRing* Goal) { QuidditchRegistry::Add(Goals, Goal); }
void UQuidditchWorldSubsystem::UnregisterGoal(AGoalRing* Goal) { QuidditchRegistry::Remove(Goals, Goal); }
void UQuidditchWorldSubsystem::RegisterQuaffle(AQuaffle* Quaffle) { QuidditchRegistry::Add(Quaffles, Quaffle); }
void UQuidditchWorldSubsystem::UnregisterQuaffle(AQuaffle* Quaffle) { QuidditchRegistry::Remove(Quaffles, Quaffle); }
void UQuidditchWorldSubsystem::RegisterBludger(ABludger* Bludger) { QuidditchRegistry::Add(Bludgers, Bludger); }
void UQuidditchWorldSubsystem::UnregisterBludger(ABludger* Bludger) { QuidditchRegistry::Remove(Bludgers, Bludger); }
void UQuidditchWorldSubsystem::RegisterBroom(ABroom* Broom) { QuidditchRegistry::Add(Brooms, Broom); }
void UQuidditchWorldSubsystem::UnregisterBroom(ABroom* Broom) { QuidditchRegistry::Remove(Brooms, Broom); }

AQuaffle* UQuidditchWorldSubsystem::FindNearestFreeQuaffle(const FVector& Location, float* OutDistance) const
{
	return QuidditchRegistry::FindNearest<AQuaffle>(GetQuaffles(), Location, OutDistance,
		[](const AQuaffle& Quaffle) { return Quaffle.CanBePickedUp(); });
}

ABludger* UQuidditchWorldSubsystem::FindNearestBludger(const FVector& Location, float* OutDistance) const
{
	return QuidditchRegistry::FindNearest<ABludger>(GetBludgers(), Location, OutDistance,
		[](const ABludger&) { return true; });
}
