// GasAbilityGenerator - Dialogue Asset Sync
// v4.4 Phase 4: Bidirectional sync - read from and write to UDialogueBlueprint
//
// Phase 3: Extract events/conditions from dialogue assets for [RO] columns
// Phase 4: Apply validated tokens from Excel to dialogue assets

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
 * Result from applying tokens to a dialogue blueprint (Phase 4)
 */
struct FDialogueAssetApplyResult
{
	/** Whether the apply operation succeeded */
	bool bSuccess = false;

	/** Error message if apply failed */
	FString ErrorMessage;

	/** Number of nodes updated */
	int32 NodesUpdated = 0;

	/** Number of events applied */
	int32 EventsApplied = 0;

	/** Number of conditions applied */
	int32 ConditionsApplied = 0;

	/** Number of nodes where events were cleared */
	int32 EventsCleared = 0;

	/** Number of nodes where conditions were cleared */
	int32 ConditionsCleared = 0;

	/** Nodes that were skipped due to validation errors */
	int32 NodesSkippedValidation = 0;

	/** Nodes not found in asset */
	int32 NodesNotFound = 0;
};

/**
 * Bidirectional sync between UDialogueBlueprint and XLSX token strings
 *
 * Phase 3 (Read): Extract events/conditions from dialogue assets
 * - SyncFromAsset() reads UDialogueBlueprint and returns token strings
 * - Used to populate [RO]EVENTS_CURRENT and [RO]CONDITIONS_CURRENT columns
 *
 * Phase 4 (Write): Apply validated tokens to dialogue assets
 * - ApplyTokensToAsset() writes validated tokens to UDialogueBlueprint
 * - Only applies tokens that pass validation (partial apply)
 * - Supports CLEAR token to remove all events/conditions
 *
 * Usage:
 * 1. SyncFromAsset() - Read current UE state for [RO] columns
 * 2. User edits EVENTS/CONDITIONS columns in Excel
 * 3. ApplyTokensToAsset() - Write validated changes back to UE
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

	//=========================================================================
	// Phase 4: Apply tokens to dialogue assets
	//=========================================================================

	/**
	 * Apply validated tokens from rows to a dialogue blueprint
	 * @param DialogueBlueprint - The dialogue asset to modify
	 * @param Rows - Rows containing EventsTokenStr and ConditionsTokenStr to apply
	 * @return Result with counts of applied/skipped tokens
	 */
	static FDialogueAssetApplyResult ApplyTokensToAsset(UDialogueBlueprint* DialogueBlueprint, const TArray<FDialogueTableRow>& Rows);

	/**
	 * Apply tokens to a dialogue blueprint loaded by path
	 * @param AssetPath - Path to the dialogue blueprint asset
	 * @param Rows - Rows containing tokens to apply
	 * @return Result with counts of applied/skipped tokens
	 */
	static FDialogueAssetApplyResult ApplyTokensToAssetPath(const FString& AssetPath, const TArray<FDialogueTableRow>& Rows);

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

	//=========================================================================
	// Phase 4: Apply helpers
	//=========================================================================

	/**
	 * Apply events token string to a dialogue node
	 * @param Node - The dialogue node to modify
	 * @param EventsTokenStr - Token string to apply
	 * @param OutResult - Result to update with counts
	 * @return true if events were applied successfully
	 */
	static bool ApplyEventsToNode(class UDialogueNode* Node, const FString& EventsTokenStr, FDialogueAssetApplyResult& OutResult);

	/**
	 * Apply conditions token string to a dialogue node
	 * @param Node - The dialogue node to modify
	 * @param ConditionsTokenStr - Token string to apply
	 * @param OutResult - Result to update with counts
	 * @return true if conditions were applied successfully
	 */
	static bool ApplyConditionsToNode(class UDialogueNode* Node, const FString& ConditionsTokenStr, FDialogueAssetApplyResult& OutResult);
};
