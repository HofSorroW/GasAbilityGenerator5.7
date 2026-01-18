// GasAbilityGenerator v3.9
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v3.9: NPC Pipeline - ActivitySchedules, GoalItems, Quests parsing
// v2.8.3: Function override parsing for parent class functions (HandleDeath, etc.)
// v2.6.14: Prefix validation for all asset types as secondary safeguard against parser bugs
// v2.6.7: Deferred asset retry mechanism for dependency resolution
// v2.6.6: GE assets created as Blueprints for CooldownGameplayEffectClass compatibility
// v2.6.5: Added Niagara System parser
// v2.5.0: Renamed to GasAbilityGenerator for generic UE project compatibility
// v2.4.0: Added inline event graph and variables parsing for gameplay abilities
// v2.3.0: Added 12 new asset type parsers with dependency-based generation order
// v2.2.0: Added event graph parsing support
// v2.1.8: Added enumeration parsing support

#pragma once

#include "CoreMinimal.h"
#include "Locked/GasAbilityGeneratorTypes.h"

/**
 * YAML Parser for manifest and incremental files
 * Parses manifest.yaml and individual *.yaml files for asset generation
 */
class GASABILITYGENERATOR_API FGasAbilityGeneratorParser
{
public:
	/**
	 * Parse the main manifest.yaml file
	 * @param ManifestContent Raw YAML content
	 * @param OutData Parsed manifest data
	 * @return true if parsing succeeded
	 */
	static bool ParseManifest(const FString& ManifestContent, FManifestData& OutData);

	/**
	 * Parse an incremental *.yaml file for a single asset
	 * @param YamlContent Raw YAML content
	 * @param OutAssetName The asset name extracted from the file
	 * @param OutData The manifest data to update with the parsed asset
	 * @return true if parsing succeeded
	 */
	static bool ParseIncrementalFile(const FString& YamlContent, FString& OutAssetName, FManifestData& OutData);

private:
	// Section parsers
	static void ParseTags(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseEnumerations(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseInputActions(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseInputMappingContexts(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseGameplayEffects(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseGameplayAbilities(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseActorBlueprints(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseWidgetBlueprints(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseBlackboards(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseBehaviorTrees(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseMaterials(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseMaterialFunctions(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);  // v2.6.12
	static void ParseMaterialInstances(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);  // v4.9
	static void ParseEventGraphs(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);

	// v2.3.0: New asset type parsers
	static void ParseFloatCurves(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseAnimationMontages(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseAnimationNotifies(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseDialogueBlueprints(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseEquippableItems(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseActivities(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseAbilityConfigurations(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseActivityConfigurations(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseItemCollections(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseNarrativeEvents(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseGameplayCues(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);  // v4.0
	static void ParseNPCDefinitions(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseCharacterDefinitions(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseCharacterAppearances(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);  // v4.8.3
	static void ParseTaggedDialogueSets(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseNiagaraSystems(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);  // v2.6.5
	static void ParseFXPresets(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);  // v4.9

	// v3.9: NPC Pipeline parsers
	static void ParseActivitySchedules(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseGoalItems(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseQuests(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);

	// v4.9: TriggerSet parser
	static void ParseTriggerSets(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);

	// v3.9.8: Mesh-to-Item Pipeline parsers
	static void ParsePipelineConfig(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParsePipelineItems(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParsePipelineCollections(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParsePipelineLoadouts(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);

	// v3.9.9: POI & NPC Spawner Placement parsers
	static void ParsePOIPlacements(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);
	static void ParseNPCSpawnerPlacements(const TArray<FString>& Lines, int32& LineIndex, FManifestData& OutData);

	// v2.2.0: Event graph helper parsers
	static void ParseGraphNodes(const TArray<FString>& Lines, int32& LineIndex, int32 SubsectionIndent, FManifestEventGraphDefinition& OutGraph);
	static void ParseGraphConnections(const TArray<FString>& Lines, int32& LineIndex, int32 SubsectionIndent, FManifestEventGraphDefinition& OutGraph);
	static FManifestGraphPinReference ParsePinReference(const FString& Value);

	// v2.8.3: Function override parser
	static void ParseFunctionOverrides(const TArray<FString>& Lines, int32& LineIndex, int32 SubsectionIndent, TArray<FManifestFunctionOverrideDefinition>& OutOverrides);

	// Helper functions
	static FString StripYamlComment(const FString& Value);  // v3.9.10: Strip inline YAML comments
	static FString GetLineValue(const FString& Line);
	static int32 GetIndentLevel(const FString& Line);
	static bool IsArrayItem(const FString& Line);
	static FString GetArrayItemValue(const FString& Line);
	static bool IsSectionHeader(const FString& Line, const FString& SectionName);
	static bool ShouldExitSection(const FString& Line, int32 SectionIndent);
	// v3.9.8: Vector/Rotator string parsing helpers
	static FVector ParseVectorFromString(const FString& VectorStr);
	static FRotator ParseRotatorFromString(const FString& RotatorStr);
};
