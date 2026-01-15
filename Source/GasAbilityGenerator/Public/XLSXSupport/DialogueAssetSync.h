// GasAbilityGenerator - Dialogue Asset Sync
// v4.4 Phase 3: Read events/conditions from UDialogueBlueprint for [RO] columns
//
// Extracts event and condition data from dialogue assets and serializes
// them to token strings for display in Excel's read-only columns.

#pragma once

#include "CoreMinimal.h"
#include "DialogueTableEditorTypes.h"

class UDialogueBlueprint;

/**
 * Per-node event/condition data extracted from UDialogueBlueprint
 */
struct FDialogueNodeAssetData
{
	/** Serialized events token string (from UE asset) */
	FString EventsTokenStr;

	/** Serialized conditions token string (from UE asset) */
	FString ConditionsTokenStr;

	/** Node type from asset */
	EDialogueTableNodeType NodeType = EDialogueTableNodeType::NPC;

	/** Whether this node was found in the asset */
	bool bFoundInAsset = false;

	FDialogueNodeAssetData() = default;
};

/**
 * Result from syncing a dialogue blueprint
 */
struct FDialogueAssetSyncResult
{
	/** Whether the sync operation succeeded */
	bool bSuccess = false;

	/** Error message if sync failed */
	FString ErrorMessage;

	/** Map from DialogueID.NodeID to asset data */
	TMap<FString, FDialogueNodeAssetData> NodeData;

	/** Total nodes found in asset */
	int32 NodesFound = 0;

	/** Nodes with events */
	int32 NodesWithEvents = 0;

	/** Nodes with conditions */
	int32 NodesWithConditions = 0;

	/** Helper to get key for a dialogue row */
	static FString MakeKey(const FName& DialogueID, const FName& NodeID)
	{
		return FString::Printf(TEXT("%s.%s"), *DialogueID.ToString(), *NodeID.ToString());
	}
};

/**
 * Reads dialogue blueprints and extracts events/conditions for XLSX export
 *
 * This class bridges UDialogueBlueprint assets and the XLSX export system,
 * enabling the [RO]EVENTS_CURRENT and [RO]CONDITIONS_CURRENT columns to
 * show what's currently configured in the UE asset.
 *
 * Usage:
 * 1. Call SyncFromAsset() with a UDialogueBlueprint
 * 2. Pass result to Writer to populate [RO] columns
 * 3. Users see UE state vs their Excel edits side-by-side
 */
class GASABILITYGENERATOR_API FDialogueAssetSync
{
public:
	/**
	 * Extract events/conditions from a dialogue blueprint
	 * @param DialogueBlueprint - The dialogue asset to read from
	 * @return Result containing node data mapped by DialogueID.NodeID
	 */
	static FDialogueAssetSyncResult SyncFromAsset(UDialogueBlueprint* DialogueBlueprint);

	/**
	 * Extract events/conditions from a dialogue blueprint loaded by path
	 * @param AssetPath - Path to the dialogue blueprint asset
	 * @return Result containing node data mapped by DialogueID.NodeID
	 */
	static FDialogueAssetSyncResult SyncFromAssetPath(const FString& AssetPath);

	/**
	 * Sync multiple dialogue blueprints and merge results
	 * @param DialogueBlueprints - Array of dialogue assets to read from
	 * @return Combined result with all nodes from all dialogues
	 */
	static FDialogueAssetSyncResult SyncFromAssets(const TArray<UDialogueBlueprint*>& DialogueBlueprints);

	/**
	 * Populate [RO] columns in rows from asset sync data
	 * @param Rows - Rows to update (modified in place)
	 * @param SyncResult - Asset sync result containing current UE state
	 * @return Number of rows updated
	 */
	static int32 PopulateAssetData(TArray<FDialogueTableRow>& Rows, const FDialogueAssetSyncResult& SyncResult);

private:
	/**
	 * Extract data from a single dialogue node
	 * @param Node - The dialogue node to extract from
	 * @param OutData - Output data structure to populate
	 */
	static void ExtractNodeData(class UDialogueNode* Node, FDialogueNodeAssetData& OutData);

	/**
	 * Serialize events array to token string
	 * @param Events - Array of narrative events
	 * @return Token string representation
	 */
	static FString SerializeEventsToTokens(const TArray<class UNarrativeEvent*>& Events);

	/**
	 * Serialize conditions array to token string
	 * @param Conditions - Array of narrative conditions
	 * @return Token string representation
	 */
	static FString SerializeConditionsToTokens(const TArray<class UNarrativeCondition*>& Conditions);
};
