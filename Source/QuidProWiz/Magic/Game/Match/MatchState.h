// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class MatchStateBase
{
public:
	
	virtual ~MatchStateBase() = default;
	virtual bool CanScore() const { return false; }
	virtual bool CanMove() const { return false; }
	virtual bool IsWaitingToStart() const { return false; }
	virtual bool IsInProgress() const { return false; }
	virtual bool IsMatchEnded() const { return false; }
	virtual FString GetStateName() const { return TEXT("Unknown"); }

};

class MatchStateWaitingToStart : public MatchStateBase
{
public:

	virtual bool CanMove() const override { return false; }
	virtual bool IsWaitingToStart() const override { return true; }
	virtual FString GetStateName() const override { return TEXT("WaitingToStart"); }

};

class MatchStateInProgress : public MatchStateBase
{
public:

	virtual bool CanScore() const override { return true; }
	virtual bool CanMove() const override { return true; }
	virtual bool IsInProgress() const override { return true; }
	virtual FString GetStateName() const override { return TEXT("InProgress"); }
};

class MatchStateMatchEnded : public MatchStateBase
{
public:

	virtual bool IsMatchEnded() const override { return true; }
	virtual FString GetStateName() const override { return TEXT("MatchEnded"); }
};

class MatchStateReturnToMenu : public MatchStateBase
{
public:

	virtual FString GetStateName() const override { return TEXT("ReturnToMenu"); }
};