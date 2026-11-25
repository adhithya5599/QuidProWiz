// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "QuidditchTeam.generated.h"

/**
 * The two sides of a match. (Named EGoalTeam for historical reasons; renaming it would break
 * values already saved in levels and Blueprints.)
 *
 * Brooms belong to a team, and a goal ring belongs to the team that DEFENDS it: putting the
 * Quaffle through a ring scores for the opposing team.
 */
UENUM(BlueprintType)
enum class EGoalTeam : uint8
{
	TeamA UMETA(DisplayName = "Team A"),
	TeamB UMETA(DisplayName = "Team B")
};

inline EGoalTeam GetOpposingTeam(EGoalTeam Team)
{
	return Team == EGoalTeam::TeamA ? EGoalTeam::TeamB : EGoalTeam::TeamA;
}
