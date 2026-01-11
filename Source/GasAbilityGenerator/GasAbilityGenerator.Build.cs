// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GasAbilityGenerator : ModuleRules
{
	public GasAbilityGenerator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Slate",
			"SlateCore",
			"UnrealEd",
			"EditorFramework",
			"ToolMenus",
			"Projects",
			"DesktopPlatform",
			"ApplicationCore",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"AIModule",
			"NavigationSystem",
			"EnhancedInput"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			// Blueprint generation
			"BlueprintGraph",
			"Kismet",
			"KismetCompiler",
			"GraphEditor",
			
			// Widget blueprint generation
			"UMG",
			"UMGEditor",
			
			// Behavior tree generation
			"BehaviorTreeEditor",
			
			// Material generation
			"MaterialEditor",
			
			// Asset creation
			"AssetTools",
			"AssetRegistry",
			"ContentBrowser",
			
			// Narrative Pro integration
			"NarrativeArsenal",
			"NarrativePro",
			
			// Property handling
			"PropertyEditor",
			"DetailCustomizations",
			
			// JSON/YAML parsing
			"Json",
			"JsonUtilities"
		});

		// Enable exceptions for YAML parsing
		bEnableExceptions = true;
	}
}
