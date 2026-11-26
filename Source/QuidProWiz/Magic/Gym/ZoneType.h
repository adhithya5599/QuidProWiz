#pragma once

#include "CoreMinimal.h"
#include "ZoneType.generated.h"

/** The gym's test areas. Each ATestZone volume announces one of these when a broom flies in. */
UENUM(BlueprintType)
enum class EZoneType : uint8
{
	None			UMETA(DisplayName = "None"),
	BroomMovement	UMETA(DisplayName = "Broom Movement"),
	QuaffleAndGoals	UMETA(DisplayName = "Quaffle and Goals"),
	Bludger			UMETA(DisplayName = "Bludger"),
	FullMatch		UMETA(DisplayName = "Full Match"),
	Snitch			UMETA(DisplayName = "Snitch"),
	Art				UMETA(DisplayName = "Art Gallery"),
};
