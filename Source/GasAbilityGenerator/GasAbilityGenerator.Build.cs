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
			"EnhancedInput",

			// v2.6.5: Niagara VFX system generation
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			// Blueprint generation
			"BlueprintGraph",
			"Kismet",
			"KismetCompiler",
			"GraphEditor",

			// v4.15: AbilityTask node generation (Track B GAS Audit)
			"GameplayAbilitiesEditor",
			"GameplayTasksEditor",  // Required by K2Node_LatentAbilityCall

			// Widget blueprint generation
			"UMG",
			"UMGEditor",

			// Behavior tree generation
			"AIGraph",
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

			// v3.7: Dialogue blueprint generation with full tree support
			"NarrativeDialogueEditor",

			// v3.9.4: Quest blueprint generation with full state machine support
			"NarrativeQuestEditor",

			// Property handling
			"PropertyEditor",
			"DetailCustomizations",
			
			// JSON/YAML parsing
			"Json",
			"JsonUtilities",

			// v2.6.5: Niagara editor for system creation
			"NiagaraEditor",

			// v4.3: XLSX export/import support
			"FileUtilities",
			"XmlParser"
		});

		// Enable exceptions for YAML parsing
		bEnableExceptions = true;
	}
}
