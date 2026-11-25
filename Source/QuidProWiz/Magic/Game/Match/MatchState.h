// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MatchState.generated.h"

/** Replicated key for the match flow; exposed to Blueprints/UI. */
UENUM(BlueprintType)
enum class EMatchPhase : uint8
{
	WaitingToStart	UMETA(DisplayName = "Waiting To Start"),
	InProgress		UMETA(DisplayName = "In Progress"),
	MatchEnded		UMETA(DisplayName = "Match Ended"),
	ReturnToMenu	UMETA(DisplayName = "Return To Menu")
};

/**
 * State pattern for the match: each phase answers what is allowed. States are stateless, so one
 * shared instance per phase is used (flyweight) and the replicated EMatchPhase selects it.
 */
class FMatchStateBase
{
public:
	virtual ~FMatchStateBase() = default;

	virtual bool CanScore() const { return false; }
	virtual bool CanMove() const { return false; }
	virtual bool IsMatchEnded() const { return false; }
	virtual const TCHAR* GetStateName() const = 0;

	static const FMatchStateBase& Get(EMatchPhase Phase);
};

class FMatchStateWaitingToStart : public FMatchStateBase
{
public:
	virtual const TCHAR* GetStateName() const override { return TEXT("WaitingToStart"); }
};

class FMatchStateInProgress : public FMatchStateBase
{
public:
	virtual bool CanScore() const override { return true; }
	virtual bool CanMove() const override { return true; }
	virtual const TCHAR* GetStateName() const override { return TEXT("InProgress"); }
};

class FMatchStateMatchEnded : public FMatchStateBase
{
public:
	virtual bool IsMatchEnded() const override { return true; }
	virtual const TCHAR* GetStateName() const override { return TEXT("MatchEnded"); }
};

class FMatchStateReturnToMenu : public FMatchStateBase
{
public:
	virtual bool IsMatchEnded() const override { return true; }
	virtual const TCHAR* GetStateName() const override { return TEXT("ReturnToMenu"); }
};

inline const FMatchStateBase& FMatchStateBase::Get(EMatchPhase Phase)
{
	static const FMatchStateWaitingToStart WaitingToStart;
	static const FMatchStateInProgress InProgress;
	static const FMatchStateMatchEnded MatchEnded;
	static const FMatchStateReturnToMenu ReturnToMenu;

	switch (Phase)
	{
	case EMatchPhase::InProgress:	return InProgress;
	case EMatchPhase::MatchEnded:	return MatchEnded;
	case EMatchPhase::ReturnToMenu:	return ReturnToMenu;
	case EMatchPhase::WaitingToStart:
	default:						return WaitingToStart;
	}
}
