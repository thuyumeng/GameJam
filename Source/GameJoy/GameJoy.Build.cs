// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GameJoy : ModuleRules
{
	public GameJoy(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"GameJoy",
			"GameJoy/Jungle",
			"GameJoy/Jungle/AI",
			"GameJoy/Jungle/Interfaces",
			"GameJoy/Variant_Platforming",
			"GameJoy/Variant_Platforming/Animation",
			"GameJoy/Variant_Combat",
			"GameJoy/Variant_Combat/AI",
			"GameJoy/Variant_Combat/Animation",
			"GameJoy/Variant_Combat/Gameplay",
			"GameJoy/Variant_Combat/Interfaces",
			"GameJoy/Variant_Combat/UI",
			"GameJoy/Variant_SideScrolling",
			"GameJoy/Variant_SideScrolling/AI",
			"GameJoy/Variant_SideScrolling/Gameplay",
			"GameJoy/Variant_SideScrolling/Interfaces",
			"GameJoy/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
