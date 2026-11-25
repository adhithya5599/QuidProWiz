// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

/**
 * Editor-only module holding the project's automation tests (QA scenarios).
 * Kept separate from the game module so test code never ships in game builds.
 */
public class QuidProWizTests : ModuleRules
{
	public QuidProWizTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"QuidProWiz"
		});
	}
}
