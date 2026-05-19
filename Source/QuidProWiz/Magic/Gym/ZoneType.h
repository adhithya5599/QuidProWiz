#pragma once

UENUM(BlueprintType)
enum class EZoneType : uint8
{
	None			UMETA(DisplayName = "None"),
	BroomMovement	UMETA(DisplayName = "Broom Movement"),
	QuaffleAndGoals	UMETA(DisplayName = "Quaffle and Goals"),
	Bludger			UMETA(DisplayName = "Bludger"),
	FullMatch		UMETA(DisplayName = "Full Match"),
};