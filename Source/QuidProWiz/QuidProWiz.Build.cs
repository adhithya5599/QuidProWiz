// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class QuidProWiz : ModuleRules
{
	public QuidProWiz(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] 
		{ "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AIModule", 
			"GameplayTasks", "NavigationSystem", "CinematicCamera" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		PublicIncludePaths.AddRange(new string[] {
			"QuidProWiz/Magic",
			"QuidProWiz/Magic/Ball",
			"QuidProWiz/Magic/Goal",
			"QuidProWiz/Magic/AI",
			"QuidProWiz/Magic/Game",
			"QuidProWiz/Magic/Gym",
			"QuidProWiz/Magic/Effects"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
