// GasAbilityGenerator - NPC Table Converter
// v4.5: Converts NPC table rows to manifest definitions for generation

#pragma once

#include "CoreMinimal.h"
#include "NPCTableEditor/NPCTableEditorTypes.h"
#include "GasAbilityGeneratorTypes.h"

/**
 * Converts NPC table rows to manifest definitions for generation
 */
class FNPCTableConverter
{
public:
	/**
	 * Convert all rows to manifest definitions
	 * @param Rows All NPC rows
	 * @param OutputFolder Base folder for generated assets
	 * @return Array of manifest definitions ready for generation
	 */
	static TArray<FManifestNPCDefinitionDefinition> ConvertRowsToManifest(
		const TArray<FNPCTableRow>& Rows,
		const FString& OutputFolder = TEXT("/Game/NPCs"));

	/**
	 * Convert a single table row to a manifest definition
	 * @param Row Table row to convert
	 * @param OutputFolder Base folder for generated assets
	 * @return Manifest definition
	 */
	static FManifestNPCDefinitionDefinition ConvertRowToDefinition(
		const FNPCTableRow& Row,
		const FString& OutputFolder = TEXT("/Game/NPCs"));

	/**
	 * Apply default settings to a definition
	 * @param Def Definition to apply defaults to
	 */
	static void ApplyDefaults(FManifestNPCDefinitionDefinition& Def);

private:
	/**
	 * Convert comma-separated faction string to array
	 * @param Factions Comma-separated factions
	 * @return Array of faction tag strings
	 */
	static TArray<FString> ParseFactions(const FString& Factions);

	/**
	 * Convert comma-separated items string to array
	 * @param Items Comma-separated item collection names
	 * @return Array of item collection names
	 */
	static TArray<FString> ParseItems(const FString& Items);

	/**
	 * Extract asset name from soft object path
	 * @param Path Soft object path
	 * @return Asset name only
	 */
	static FString GetAssetNameFromPath(const FSoftObjectPath& Path);
};
