// GasAbilityGenerator - Quest Table Converter
// v4.8: Converts Quest table rows to manifest definitions for generation
// Follows NPC/Dialogue converter patterns

#pragma once

#include "CoreMinimal.h"
#include "QuestTableEditor/QuestTableEditorTypes.h"
#include "GasAbilityGeneratorTypes.h"

/**
 * Converts Quest table rows to manifest definitions for generation
 */
class FQuestTableConverter
{
public:
	/**
	 * Convert all rows to manifest definitions
	 * Groups rows by QuestName and creates one FManifestQuestDefinition per quest
	 * @param Rows All Quest rows
	 * @param OutputFolder Base folder for generated assets
	 * @return Array of manifest definitions ready for generation
	 */
	static TArray<FManifestQuestDefinition> ConvertRowsToManifest(
		const TArray<FQuestTableRow>& Rows,
		const FString& OutputFolder = TEXT("/Game/Quests"));

	/**
	 * Convert a single quest (all rows with same QuestName) to a manifest definition
	 * @param QuestName Quest name to convert
	 * @param Rows All rows for this quest
	 * @param OutputFolder Base folder for generated assets
	 * @return Manifest definition
	 */
	static FManifestQuestDefinition ConvertQuestToDefinition(
		const FString& QuestName,
		const TArray<const FQuestTableRow*>& Rows,
		const FString& OutputFolder = TEXT("/Game/Quests"));

	/**
	 * Convert table row to quest state definition
	 * @param Row Table row to convert
	 * @return Quest state definition
	 */
	static FManifestQuestStateDefinition ConvertRowToState(const FQuestTableRow& Row);

	/**
	 * Parse Tasks token string into task definitions
	 * Format: BPT_FindItem(Item=EI_Ore,Count=10);BPT_Move(Location=POI_Forge)
	 * @param TasksToken Task token string
	 * @return Array of task definitions
	 */
	static TArray<FManifestQuestTaskDefinition> ParseTasks(const FString& TasksToken);

	/**
	 * Parse Events token string into event definitions
	 * Format: NE_GiveXP(Amount=50);NE_AddItem(Item=EI_Sword)
	 * @param EventsToken Event token string
	 * @return Array of event definitions
	 */
	static TArray<FManifestDialogueEventDefinition> ParseEvents(const FString& EventsToken);

	/**
	 * Parse Conditions token string into condition definitions
	 * Format: NC_HasItem(Item=EI_Key);NC_QuestComplete(Quest=Quest_Prologue)
	 * @param ConditionsToken Condition token string
	 * @return Array of condition definitions
	 */
	static TArray<FManifestDialogueConditionDefinition> ParseConditions(const FString& ConditionsToken);

	/**
	 * Parse Rewards token string into reward definition
	 * Format: Reward(Currency=100,XP=50,Items=EI_Sword:1|EI_Shield:1)
	 * @param RewardsToken Rewards token string
	 * @return Reward definition
	 */
	static FManifestQuestRewardDefinition ParseRewards(const FString& RewardsToken);

	/**
	 * Apply default settings to a definition
	 * @param Def Definition to apply defaults to
	 */
	static void ApplyDefaults(FManifestQuestDefinition& Def);

	/**
	 * Convert manifest definition back to table rows
	 * Used for "Sync from Assets" functionality
	 * @param Def Manifest definition
	 * @return Array of table rows
	 */
	static TArray<FQuestTableRow> ConvertDefinitionToRows(const FManifestQuestDefinition& Def);

private:
	/**
	 * Parse token parameters from string
	 * Format: Key=Value,Key2=Value2
	 * @param ParamsStr Parameters string
	 * @return Map of parameter key-value pairs
	 */
	static TMap<FString, FString> ParseTokenParams(const FString& ParamsStr);

	/**
	 * Get state type string from enum
	 * @param StateType State type enum
	 * @return State type string (regular/success/failure)
	 */
	static FString GetStateTypeString(EQuestStateType StateType);
};
