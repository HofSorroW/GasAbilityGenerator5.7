// GasAbilityGenerator - Dialogue Table Converter
// v4.0: Converts dialogue table rows to manifest definitions

#pragma once

#include "CoreMinimal.h"
#include "DialogueTableEditorTypes.h"
#include "GasAbilityGeneratorTypes.h"

/**
 * Converts dialogue table rows to manifest definitions for generation
 */
class FDialogueTableConverter
{
public:
	/**
	 * Convert all rows to manifest definitions, grouped by DialogueID
	 * @param Rows All dialogue rows
	 * @return Map of DialogueID -> manifest definition ready for generation
	 */
	static TMap<FName, FManifestDialogueBlueprintDefinition> ConvertRowsToManifest(const TArray<FDialogueTableRow>& Rows);

	/**
	 * Build a dialogue tree definition from flat rows
	 * @param DialogueRows Rows belonging to a single dialogue
	 * @return Dialogue tree definition
	 */
	static FManifestDialogueTreeDefinition BuildDialogueTree(const TArray<FDialogueTableRow>& DialogueRows);

	/**
	 * Apply default settings to a dialogue definition
	 * These are the "sensible defaults" from Narrative Pro
	 * @param Def Definition to apply defaults to
	 */
	static void ApplyDefaults(FManifestDialogueBlueprintDefinition& Def);

	/**
	 * Convert a single table row to a dialogue node definition
	 * @param Row Table row to convert
	 * @return Dialogue node definition
	 */
	static FManifestDialogueNodeDefinition ConvertRowToNode(const FDialogueTableRow& Row);

	/**
	 * Get unique speakers from dialogue rows
	 * @param DialogueRows Rows to extract speakers from
	 * @return Array of unique speaker names
	 */
	static TArray<FName> GetUniqueSpeakers(const TArray<FDialogueTableRow>& DialogueRows);
};
