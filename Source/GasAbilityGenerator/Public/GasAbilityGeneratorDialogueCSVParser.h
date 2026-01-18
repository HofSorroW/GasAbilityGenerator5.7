// Copyright 2026 GasAbilityGenerator Plugin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Locked/GasAbilityGeneratorTypes.h"

/**
 * CSV row representation for dialogue data
 */
struct FDialogueCSVRow
{
	FString Dialogue;      // DBP_* asset name
	FString NodeID;        // Unique node ID within dialogue
	FString Type;          // NPC or PLAYER
	FString Speaker;       // NPCDefinition reference (NPC_*)
	FString Text;          // Full spoken text
	FString OptionText;    // Short text for dialogue wheel (player only)
	FString Replies;       // Semicolon-separated node IDs, or END
	FString Conditions;    // NC_* references (Phase 2)
	FString Events;        // NE_* references (Phase 2)

	bool IsValid() const
	{
		return !Dialogue.IsEmpty() && !NodeID.IsEmpty() && !Type.IsEmpty();
	}

	bool IsNPCNode() const
	{
		return Type.Equals(TEXT("NPC"), ESearchCase::IgnoreCase);
	}

	bool IsPlayerNode() const
	{
		return Type.Equals(TEXT("PLAYER"), ESearchCase::IgnoreCase);
	}

	bool IsTerminal() const
	{
		return Replies.Equals(TEXT("END"), ESearchCase::IgnoreCase) || Replies.IsEmpty();
	}
};

/**
 * Parsed dialogue data grouped by dialogue name
 */
struct FParsedDialogueData
{
	FString DialogueName;                    // DBP_* name
	TArray<FDialogueCSVRow> Nodes;           // All nodes for this dialogue
	TSet<FString> Speakers;                  // Unique speakers referenced
	FString RootNodeID;                      // First node = root

	/** Compute hash for v3.0 change detection */
	uint64 ComputeHash() const
	{
		FString Combined;
		Combined += DialogueName;
		for (const auto& Node : Nodes)
		{
			Combined += Node.NodeID + Node.Type + Node.Speaker + Node.Text +
			            Node.OptionText + Node.Replies + Node.Conditions + Node.Events;
		}
		return FCrc::StrCrc32(*Combined);
	}
};

/**
 * Dialogue CSV Parser
 *
 * Parses CSV/Excel exported dialogue data and converts to FManifestDialogueBlueprintDefinition
 * for use with the existing dialogue blueprint generator.
 *
 * CSV Format:
 * Dialogue,NodeID,Type,Speaker,Text,OptionText,Replies,Conditions,Events
 * DBP_Blacksmith,greeting,NPC,NPC_Blacksmith,"Welcome!","",ask_work;goodbye,"",""
 *
 * Features:
 * - Multiple dialogues in single file (grouped by Dialogue column)
 * - Automatic root node detection (first node per dialogue)
 * - Connection resolution (Replies column -> npc_replies/player_replies)
 * - v3.0 hash computation for change detection
 */
class GASABILITYGENERATOR_API FDialogueCSVParser
{
public:

	/**
	 * Parse a CSV file containing dialogue data
	 * @param CSVFilePath Path to the CSV file
	 * @param OutDialogues Parsed dialogue definitions ready for generator
	 * @return true if parsing succeeded
	 */
	static bool ParseCSVFile(const FString& CSVFilePath, TArray<FManifestDialogueBlueprintDefinition>& OutDialogues);

	/**
	 * Parse CSV content string (for testing or embedded data)
	 * @param CSVContent CSV content as string
	 * @param OutDialogues Parsed dialogue definitions
	 * @return true if parsing succeeded
	 */
	static bool ParseCSVContent(const FString& CSVContent, TArray<FManifestDialogueBlueprintDefinition>& OutDialogues);

	/**
	 * Convert parsed CSV data to manifest definitions
	 * @param ParsedData Grouped dialogue data from CSV
	 * @param OutDefinition Manifest definition for generator
	 * @return true if conversion succeeded
	 */
	static bool ConvertToManifestDefinition(const FParsedDialogueData& ParsedData, FManifestDialogueBlueprintDefinition& OutDefinition);

private:

	/** Parse a single CSV line into a row struct */
	static bool ParseCSVLine(const FString& Line, FDialogueCSVRow& OutRow);

	/** Parse CSV value handling quoted strings */
	static FString ParseCSVValue(const FString& Value);

	/** Split CSV line respecting quoted values */
	static TArray<FString> SplitCSVLine(const FString& Line);

	/** Group rows by dialogue name */
	static TMap<FString, FParsedDialogueData> GroupByDialogue(const TArray<FDialogueCSVRow>& Rows);

	/** Validate dialogue structure (check for orphan nodes, missing connections) */
	static bool ValidateDialogueStructure(const FParsedDialogueData& Data, TArray<FString>& OutErrors);

	/** Convert CSV row to manifest node definition */
	static FManifestDialogueNodeDefinition ConvertRowToNode(const FDialogueCSVRow& Row, const FParsedDialogueData& DialogueData);

	/** Parse events string (e.g., "NE_BeginQuest:Quest_Test;NE_GiveXP:100") */
	static TArray<FManifestDialogueEventDefinition> ParseEventsString(const FString& EventsStr);

	/** Parse conditions string (e.g., "NC_IsQuestSucceeded:Quest_Test") */
	static TArray<FManifestDialogueConditionDefinition> ParseConditionsString(const FString& ConditionsStr);

	/** Determine reply arrays based on node types */
	static void ResolveReplies(const FDialogueCSVRow& Row, const FParsedDialogueData& DialogueData,
	                           TArray<FString>& OutNPCReplies, TArray<FString>& OutPlayerReplies);
};
