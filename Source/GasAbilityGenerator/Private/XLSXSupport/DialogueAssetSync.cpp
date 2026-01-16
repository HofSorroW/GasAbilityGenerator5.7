// GasAbilityGenerator - Dialogue Asset Sync Implementation
// v4.4 Phase 3+4: Bidirectional sync between UDialogueBlueprint and XLSX token strings
// Phase 3: Read events/conditions from UDialogueBlueprint for [RO] columns
// Phase 4: Apply validated token strings back to UDialogueBlueprint assets

#include "XLSXSupport/DialogueAssetSync.h"
#include "XLSXSupport/DialogueTokenRegistry.h"

#if WITH_EDITOR
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "UObject/UObjectGlobals.h"
#include "AssetRegistry/AssetRegistryModule.h"

// Narrative Pro includes
#include "DialogueBlueprint.h"  // UDialogueBlueprint from NarrativeDialogueEditor
#include "Tales/Dialogue.h"
#include "Tales/DialogueSM.h"
#include "Tales/NarrativeEvent.h"
#include "Tales/NarrativeCondition.h"

// v4.12: Include EdGraph for NodePosX extraction
#include "EdGraph/EdGraph.h"
#endif

FDialogueAssetSyncResult FDialogueAssetSync::SyncFromAsset(UDialogueBlueprint* DialogueBlueprint)
{
	FDialogueAssetSyncResult Result;

#if WITH_EDITOR
	if (!DialogueBlueprint)
	{
		Result.ErrorMessage = TEXT("DialogueBlueprint is null");
		return Result;
	}

	// Get the dialogue template from the blueprint
	UDialogue* DialogueTemplate = DialogueBlueprint->DialogueTemplate;
	if (!DialogueTemplate)
	{
		Result.ErrorMessage = TEXT("DialogueBlueprint has no DialogueTemplate");
		return Result;
	}

	// Get dialogue ID from asset name
	FName DialogueID = FName(*DialogueBlueprint->GetName());

	//=========================================================================
	// v4.12: Build graph position map (UDialogueNode* -> NodePosX)
	// DialogueGraph contains UDialogueGraphNode instances which have:
	// - DialogueNode: pointer to the runtime UDialogueNode
	// - NodePosX/NodePosY: visual position in graph editor (inherited from UEdGraphNode)
	//=========================================================================
	TMap<UDialogueNode*, int32> NodeToGraphPosX;
	TMap<UDialogueNode*, int32> NodeToGraphPosY;

	if (UEdGraph* DialogueGraph = DialogueBlueprint->DialogueGraph)
	{
		for (UEdGraphNode* GraphNode : DialogueGraph->Nodes)
		{
			if (!GraphNode) continue;

			// Check if this graph node has a DialogueNode property
			// UDialogueGraphNode has: UPROPERTY() class UDialogueNode* DialogueNode;
			if (FObjectProperty* DialogueNodeProp = FindFProperty<FObjectProperty>(GraphNode->GetClass(), TEXT("DialogueNode")))
			{
				UDialogueNode* RuntimeNode = Cast<UDialogueNode>(DialogueNodeProp->GetObjectPropertyValue_InContainer(GraphNode));
				if (RuntimeNode)
				{
					NodeToGraphPosX.Add(RuntimeNode, GraphNode->NodePosX);
					NodeToGraphPosY.Add(RuntimeNode, GraphNode->NodePosY);
				}
			}
		}
		UE_LOG(LogTemp, Log, TEXT("DialogueAssetSync: Built graph position map with %d entries"), NodeToGraphPosX.Num());
	}

	//=========================================================================
	// v4.8: Build parent node mapping (child ID -> parent ID)
	// This requires scanning all nodes to find who references whom
	//=========================================================================
	TMap<FName, FName> ChildToParentMap;

	// Scan NPC nodes for their children
	for (UDialogueNode_NPC* NPCNode : DialogueTemplate->NPCReplies)
	{
		if (!NPCNode) continue;
		FName ParentID = NPCNode->GetID();

		// NPC replies (next NPC nodes in chain)
		for (UDialogueNode_NPC* ChildNPC : NPCNode->NPCReplies)
		{
			if (ChildNPC)
			{
				ChildToParentMap.Add(ChildNPC->GetID(), ParentID);
			}
		}
		// Player replies (choices after NPC speaks)
		for (UDialogueNode_Player* ChildPlayer : NPCNode->PlayerReplies)
		{
			if (ChildPlayer)
			{
				ChildToParentMap.Add(ChildPlayer->GetID(), ParentID);
			}
		}
	}

	// Scan Player nodes for their children
	for (UDialogueNode_Player* PlayerNode : DialogueTemplate->PlayerReplies)
	{
		if (!PlayerNode) continue;
		FName ParentID = PlayerNode->GetID();

		// NPC replies (NPC response to player choice)
		for (UDialogueNode_NPC* ChildNPC : PlayerNode->NPCReplies)
		{
			if (ChildNPC)
			{
				ChildToParentMap.Add(ChildNPC->GetID(), ParentID);
			}
		}
		// Player replies (followup choices)
		for (UDialogueNode_Player* ChildPlayer : PlayerNode->PlayerReplies)
		{
			if (ChildPlayer)
			{
				ChildToParentMap.Add(ChildPlayer->GetID(), ParentID);
			}
		}
	}

	//=========================================================================
	// Process NPC replies
	//=========================================================================
	for (UDialogueNode_NPC* NPCNode : DialogueTemplate->NPCReplies)
	{
		if (!NPCNode) continue;

		FDialogueNodeAssetData NodeData;
		ExtractNodeData(NPCNode, NodeData);
		NodeData.NodeType = EDialogueTableNodeType::NPC;
		NodeData.bFoundInAsset = true;

		// Set parent from mapping
		if (FName* ParentID = ChildToParentMap.Find(NPCNode->GetID()))
		{
			NodeData.ParentNodeID = ParentID->ToString();
		}

		// v4.12: Set graph position from mapping
		if (int32* PosX = NodeToGraphPosX.Find(NPCNode))
		{
			NodeData.GraphPosX = *PosX;
			NodeData.bHasGraphPosition = true;
		}
		if (int32* PosY = NodeToGraphPosY.Find(NPCNode))
		{
			NodeData.GraphPosY = *PosY;
		}

		FString Key = FDialogueAssetSyncResult::MakeKey(DialogueID, NPCNode->GetID());
		Result.NodeData.Add(Key, NodeData);
		Result.NodesFound++;

		if (!NodeData.EventsTokenStr.IsEmpty())
		{
			Result.NodesWithEvents++;
		}
		if (!NodeData.ConditionsTokenStr.IsEmpty())
		{
			Result.NodesWithConditions++;
		}
	}

	//=========================================================================
	// Process Player replies
	//=========================================================================
	for (UDialogueNode_Player* PlayerNode : DialogueTemplate->PlayerReplies)
	{
		if (!PlayerNode) continue;

		FDialogueNodeAssetData NodeData;
		ExtractNodeData(PlayerNode, NodeData);
		NodeData.NodeType = EDialogueTableNodeType::Player;
		NodeData.bFoundInAsset = true;

		// Set parent from mapping
		if (FName* ParentID = ChildToParentMap.Find(PlayerNode->GetID()))
		{
			NodeData.ParentNodeID = ParentID->ToString();
		}

		// v4.12: Set graph position from mapping
		if (int32* PosX = NodeToGraphPosX.Find(PlayerNode))
		{
			NodeData.GraphPosX = *PosX;
			NodeData.bHasGraphPosition = true;
		}
		if (int32* PosY = NodeToGraphPosY.Find(PlayerNode))
		{
			NodeData.GraphPosY = *PosY;
		}

		FString Key = FDialogueAssetSyncResult::MakeKey(DialogueID, PlayerNode->GetID());
		Result.NodeData.Add(Key, NodeData);
		Result.NodesFound++;

		if (!NodeData.EventsTokenStr.IsEmpty())
		{
			Result.NodesWithEvents++;
		}
		if (!NodeData.ConditionsTokenStr.IsEmpty())
		{
			Result.NodesWithConditions++;
		}
	}

	Result.bSuccess = true;

	UE_LOG(LogTemp, Log, TEXT("DialogueAssetSync: Synced '%s' - %d nodes (%d with events, %d with conditions)"),
		*DialogueBlueprint->GetName(), Result.NodesFound, Result.NodesWithEvents, Result.NodesWithConditions);

#else
	Result.ErrorMessage = TEXT("DialogueAssetSync requires WITH_EDITOR");
#endif

	return Result;
}

//=========================================================================
// Phase 4: Apply tokens to dialogue assets
//=========================================================================

FDialogueAssetApplyResult FDialogueAssetSync::ApplyTokensToAsset(UDialogueBlueprint* DialogueBlueprint, const TArray<FDialogueTableRow>& Rows)
{
	FDialogueAssetApplyResult Result;

#if WITH_EDITOR
	if (!DialogueBlueprint)
	{
		Result.ErrorMessage = TEXT("DialogueBlueprint is null");
		return Result;
	}

	UDialogue* DialogueTemplate = DialogueBlueprint->DialogueTemplate;
	if (!DialogueTemplate)
	{
		Result.ErrorMessage = TEXT("DialogueBlueprint has no DialogueTemplate");
		return Result;
	}

	FName DialogueID = FName(*DialogueBlueprint->GetName());

	// Build a map of NodeID -> DialogueNode for quick lookup
	TMap<FName, UDialogueNode*> NodeMap;

	for (UDialogueNode_NPC* NPCNode : DialogueTemplate->NPCReplies)
	{
		if (NPCNode)
		{
			NodeMap.Add(NPCNode->GetID(), NPCNode);
		}
	}

	for (UDialogueNode_Player* PlayerNode : DialogueTemplate->PlayerReplies)
	{
		if (PlayerNode)
		{
			NodeMap.Add(PlayerNode->GetID(), PlayerNode);
		}
	}

	// Process each row that belongs to this dialogue
	for (const FDialogueTableRow& Row : Rows)
	{
		// Skip rows that don't belong to this dialogue
		if (Row.DialogueID != DialogueID)
		{
			continue;
		}

		// Skip rows that don't have valid tokens
		if (!Row.AreTokensValid())
		{
			Result.NodesSkippedValidation++;
			continue;
		}

		// Find the node
		UDialogueNode** FoundNode = NodeMap.Find(Row.NodeID);
		if (!FoundNode || !*FoundNode)
		{
			Result.NodesNotFound++;
			continue;
		}

		UDialogueNode* Node = *FoundNode;
		bool bNodeUpdated = false;

		// Apply events if token string is non-empty
		if (!Row.EventsTokenStr.IsEmpty())
		{
			if (ApplyEventsToNode(Node, Row.EventsTokenStr, Result))
			{
				bNodeUpdated = true;
			}
		}

		// Apply conditions if token string is non-empty
		if (!Row.ConditionsTokenStr.IsEmpty())
		{
			if (ApplyConditionsToNode(Node, Row.ConditionsTokenStr, Result))
			{
				bNodeUpdated = true;
			}
		}

		if (bNodeUpdated)
		{
			Result.NodesUpdated++;
		}
	}

	Result.bSuccess = true;

	UE_LOG(LogTemp, Log, TEXT("DialogueAssetSync: Applied to '%s' - %d nodes updated, %d events, %d conditions, %d skipped (validation), %d not found"),
		*DialogueBlueprint->GetName(), Result.NodesUpdated, Result.EventsApplied, Result.ConditionsApplied,
		Result.NodesSkippedValidation, Result.NodesNotFound);

	// Mark the blueprint as modified
	if (Result.NodesUpdated > 0)
	{
		DialogueBlueprint->Modify();
		DialogueBlueprint->MarkPackageDirty();
	}

#else
	Result.ErrorMessage = TEXT("DialogueAssetSync requires WITH_EDITOR");
#endif

	return Result;
}

FDialogueAssetApplyResult FDialogueAssetSync::ApplyTokensToAssetPath(const FString& AssetPath, const TArray<FDialogueTableRow>& Rows)
{
	FDialogueAssetApplyResult Result;

#if WITH_EDITOR
	UDialogueBlueprint* DialogueBlueprint = LoadObject<UDialogueBlueprint>(nullptr, *AssetPath);
	if (!DialogueBlueprint)
	{
		Result.ErrorMessage = FString::Printf(TEXT("Could not load DialogueBlueprint at '%s'"), *AssetPath);
		return Result;
	}

	return ApplyTokensToAsset(DialogueBlueprint, Rows);
#else
	Result.ErrorMessage = TEXT("DialogueAssetSync requires WITH_EDITOR");
	return Result;
#endif
}

FDialogueAssetSyncResult FDialogueAssetSync::SyncFromAssetPath(const FString& AssetPath)
{
	FDialogueAssetSyncResult Result;

#if WITH_EDITOR
	// Load the dialogue blueprint
	UDialogueBlueprint* DialogueBlueprint = LoadObject<UDialogueBlueprint>(nullptr, *AssetPath);
	if (!DialogueBlueprint)
	{
		Result.ErrorMessage = FString::Printf(TEXT("Could not load DialogueBlueprint at '%s'"), *AssetPath);
		return Result;
	}

	return SyncFromAsset(DialogueBlueprint);
#else
	Result.ErrorMessage = TEXT("DialogueAssetSync requires WITH_EDITOR");
	return Result;
#endif
}

FDialogueAssetSyncResult FDialogueAssetSync::SyncFromAssets(const TArray<UDialogueBlueprint*>& DialogueBlueprints)
{
	FDialogueAssetSyncResult CombinedResult;
	CombinedResult.bSuccess = true;

	for (UDialogueBlueprint* DialogueBlueprint : DialogueBlueprints)
	{
		FDialogueAssetSyncResult SingleResult = SyncFromAsset(DialogueBlueprint);
		if (!SingleResult.bSuccess)
		{
			CombinedResult.bSuccess = false;
			CombinedResult.ErrorMessage += SingleResult.ErrorMessage + TEXT("; ");
			continue;
		}

		// Merge node data
		for (const auto& Pair : SingleResult.NodeData)
		{
			CombinedResult.NodeData.Add(Pair.Key, Pair.Value);
		}

		CombinedResult.NodesFound += SingleResult.NodesFound;
		CombinedResult.NodesWithEvents += SingleResult.NodesWithEvents;
		CombinedResult.NodesWithConditions += SingleResult.NodesWithConditions;
	}

	return CombinedResult;
}

FDialogueAssetSyncResult FDialogueAssetSync::SyncFromAllAssets(const FString& PathFilter)
{
	FDialogueAssetSyncResult Result;

#if WITH_EDITOR
	// Get Asset Registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// =========================================================================
	// v4.7.1: Force Asset Registry rescan before querying
	// This handles file-copied assets that weren't duplicated via Unreal Editor
	// =========================================================================
	if (!PathFilter.IsEmpty())
	{
		TArray<FString> PathsToScan = { PathFilter };
		UE_LOG(LogTemp, Log, TEXT("[DialogueAssetSync] Rescanning Asset Registry for paths: %s"), *PathFilter);
		AssetRegistry.ScanPathsSynchronous(PathsToScan, true /* bForceRescan */);
	}

	// Find all UDialogueBlueprint assets
	TArray<FAssetData> AssetList;
	FTopLevelAssetPath ClassPath = UDialogueBlueprint::StaticClass()->GetClassPathName();
	UE_LOG(LogTemp, Warning, TEXT("[DialogueAssetSync] Searching for class: %s"), *ClassPath.ToString());
	AssetRegistry.GetAssetsByClass(ClassPath, AssetList, true);
	UE_LOG(LogTemp, Warning, TEXT("[DialogueAssetSync] Found %d total UDialogueBlueprint assets BEFORE filter"), AssetList.Num());

	// Debug: Log first few assets to see their paths
	for (int32 i = 0; i < FMath::Min(5, AssetList.Num()); i++)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DialogueAssetSync] Asset[%d]: PackagePath='%s', AssetName='%s'"),
			i, *AssetList[i].PackagePath.ToString(), *AssetList[i].AssetName.ToString());
	}

	// =========================================================================
	// TEST ONLY: Filter to PathFilter (e.g., TestData) for testing
	// REVERT THIS AFTER TESTING - Remove or make PathFilter empty by default
	// =========================================================================
	TArray<FAssetData> FilteredAssetList;
	for (const FAssetData& Asset : AssetList)
	{
		FString PackagePath = Asset.PackagePath.ToString();
		// Use Contains to handle /Game/Game/TestData or /Game/TestData
		if (PathFilter.IsEmpty() || PackagePath.Contains(PathFilter))
		{
			FilteredAssetList.Add(Asset);
			if (!PathFilter.IsEmpty())
			{
				UE_LOG(LogTemp, Log, TEXT("[DialogueAssetSync] Matched: %s"), *PackagePath);
			}
		}
	}

	if (!PathFilter.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DialogueAssetSync] TEST MODE - Filtered to %d assets containing '%s'"), FilteredAssetList.Num(), *PathFilter);
	}
	// =========================================================================
	// END TEST ONLY
	// =========================================================================

	Result.bSuccess = true;

	for (const FAssetData& AssetData : FilteredAssetList)
	{
		UDialogueBlueprint* DialogueBP = Cast<UDialogueBlueprint>(AssetData.GetAsset());
		if (!DialogueBP)
		{
			continue;
		}

		FDialogueAssetSyncResult SingleResult = SyncFromAsset(DialogueBP);
		if (SingleResult.bSuccess)
		{
			// Merge node data
			for (const auto& Pair : SingleResult.NodeData)
			{
				Result.NodeData.Add(Pair.Key, Pair.Value);
			}
			Result.NodesFound += SingleResult.NodesFound;
			Result.NodesWithEvents += SingleResult.NodesWithEvents;
			Result.NodesWithConditions += SingleResult.NodesWithConditions;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("DialogueAssetSync: Synced %d dialogue assets, %d nodes total"),
		FilteredAssetList.Num(), Result.NodesFound);

#else
	Result.ErrorMessage = TEXT("DialogueAssetSync requires WITH_EDITOR");
#endif

	return Result;
}

int32 FDialogueAssetSync::PopulateAssetData(TArray<FDialogueTableRow>& Rows, const FDialogueAssetSyncResult& SyncResult)
{
	int32 UpdatedCount = 0;

	for (FDialogueTableRow& Row : Rows)
	{
		FString Key = FDialogueAssetSyncResult::MakeKey(Row.DialogueID, Row.NodeID);
		const FDialogueNodeAssetData* AssetData = SyncResult.NodeData.Find(Key);

		if (AssetData && AssetData->bFoundInAsset)
		{
			// Note: We don't modify EventsTokenStr/ConditionsTokenStr here
			// Those are the EDITABLE columns authored by users in Excel
			// The [RO] columns are populated during export via separate fields
			// For now, we can add helper fields to FDialogueTableRow if needed
			// Or the Writer can look up the SyncResult directly
			UpdatedCount++;
		}
	}

	return UpdatedCount;
}

#if WITH_EDITOR
void FDialogueAssetSync::ExtractNodeData(UDialogueNode* Node, FDialogueNodeAssetData& OutData)
{
	if (!Node) return;

	// UDialogueNode inherits from UNarrativeNodeBase which has Events and Conditions arrays
	// Get events from the node's Events array (inherited from UNarrativeNodeBase)
	OutData.EventsTokenStr = SerializeEventsToTokens(Node->Events);

	// Get conditions from the node's Conditions array (inherited from UNarrativeNodeBase)
	OutData.ConditionsTokenStr = SerializeConditionsToTokens(Node->Conditions);

	// Also check Line.Conditions for dialogue-specific conditions
	if (Node->Line.Conditions.Num() > 0)
	{
		TArray<UNarrativeCondition*> LineConditions;
		for (const auto& Condition : Node->Line.Conditions)
		{
			if (Condition)
			{
				LineConditions.Add(Condition);
			}
		}

		FString LineConditionsStr = SerializeConditionsToTokens(LineConditions);
		if (!LineConditionsStr.IsEmpty())
		{
			if (!OutData.ConditionsTokenStr.IsEmpty())
			{
				OutData.ConditionsTokenStr += TEXT("; ");
			}
			OutData.ConditionsTokenStr += LineConditionsStr;
		}
	}

	//=========================================================================
	// v4.8: Extract full dialogue content
	// v4.11.4: Check multiple sources for text (Line.Text, AlternativeLines, GetDialogueText)
	//=========================================================================

	// v4.11.4: Use GetDialogueText() as primary source - it handles Line.Text and alternatives internally
	OutData.Text = Node->GetDialogueText().ToString();

	// Fallback to Line.Text directly if GetDialogueText() returned empty (shouldn't happen, but safe)
	if (OutData.Text.IsEmpty())
	{
		OutData.Text = Node->Line.Text.ToString();
	}

	// Skippable flag
	OutData.bSkippable = Node->bIsSkippable;

	// Extract Speaker for NPC nodes
	if (UDialogueNode_NPC* NPCNode = Cast<UDialogueNode_NPC>(Node))
	{
		OutData.Speaker = NPCNode->GetSpeakerID().ToString();
	}

	// Extract OptionText for Player nodes
	if (UDialogueNode_Player* PlayerNode = Cast<UDialogueNode_Player>(Node))
	{
		// OptionText is protected, use the public getter
		OutData.OptionText = PlayerNode->GetOptionText(nullptr).ToString();
	}

	// Extract NextNodeIDs (child nodes)
	TArray<FString> NextNodeNames;
	for (UDialogueNode_NPC* NPCReply : Node->NPCReplies)
	{
		if (NPCReply)
		{
			NextNodeNames.Add(NPCReply->GetID().ToString());
		}
	}
	for (UDialogueNode_Player* PlayerReply : Node->PlayerReplies)
	{
		if (PlayerReply)
		{
			NextNodeNames.Add(PlayerReply->GetID().ToString());
		}
	}
	OutData.NextNodeIDs = FString::Join(NextNodeNames, TEXT(", "));
}

FString FDialogueAssetSync::SerializeEventsToTokens(const TArray<UNarrativeEvent*>& Events)
{
	if (Events.Num() == 0) return TEXT("");

	const FDialogueTokenRegistry& Registry = FDialogueTokenRegistry::Get();
	TArray<FString> TokenStrings;

	for (UNarrativeEvent* Event : Events)
	{
		if (!Event) continue;

		FString TokenStr = Registry.SerializeEvent(Event);
		if (!TokenStr.IsEmpty())
		{
			TokenStrings.Add(TokenStr);
		}
	}

	return FString::Join(TokenStrings, TEXT("; "));
}

FString FDialogueAssetSync::SerializeConditionsToTokens(const TArray<UNarrativeCondition*>& Conditions)
{
	if (Conditions.Num() == 0) return TEXT("");

	const FDialogueTokenRegistry& Registry = FDialogueTokenRegistry::Get();
	TArray<FString> TokenStrings;

	for (UNarrativeCondition* Condition : Conditions)
	{
		if (!Condition) continue;

		FString TokenStr = Registry.SerializeCondition(Condition);
		if (!TokenStr.IsEmpty())
		{
			// Add NOT prefix if condition is negated
			if (Condition->bNot)
			{
				TokenStr = TEXT("NOT ") + TokenStr;
			}
			TokenStrings.Add(TokenStr);
		}
	}

	return FString::Join(TokenStrings, TEXT("; "));
}

//=========================================================================
// Phase 4: Apply helpers
//=========================================================================

bool FDialogueAssetSync::ApplyEventsToNode(UDialogueNode* Node, const FString& EventsTokenStr, FDialogueAssetApplyResult& OutResult)
{
	if (!Node) return false;

	const FDialogueTokenRegistry& Registry = FDialogueTokenRegistry::Get();
	FTokenDeserializeResult DeserializeResult = Registry.DeserializeEvents(EventsTokenStr, Node);

	if (!DeserializeResult.bSuccess)
	{
		// Validation should have caught this, but log anyway
		UE_LOG(LogTemp, Warning, TEXT("DialogueAssetSync: Failed to deserialize events '%s': %s"),
			*EventsTokenStr, *DeserializeResult.ErrorMessage);
		return false;
	}

	// Handle CLEAR token
	if (DeserializeResult.bShouldClear)
	{
		if (Node->Events.Num() > 0)
		{
			Node->Events.Empty();
			OutResult.EventsCleared++;
		}
	}

	// Apply new events
	if (DeserializeResult.Objects.Num() > 0)
	{
		// If not clearing, we replace existing events (design choice: XLSX is authoritative)
		if (!DeserializeResult.bShouldClear)
		{
			Node->Events.Empty();
		}

		for (UObject* EventObj : DeserializeResult.Objects)
		{
			if (UNarrativeEvent* Event = Cast<UNarrativeEvent>(EventObj))
			{
				Node->Events.Add(Event);
				OutResult.EventsApplied++;
			}
		}
	}

	return true;
}

bool FDialogueAssetSync::ApplyConditionsToNode(UDialogueNode* Node, const FString& ConditionsTokenStr, FDialogueAssetApplyResult& OutResult)
{
	if (!Node) return false;

	const FDialogueTokenRegistry& Registry = FDialogueTokenRegistry::Get();
	FTokenDeserializeResult DeserializeResult = Registry.DeserializeConditions(ConditionsTokenStr, Node);

	if (!DeserializeResult.bSuccess)
	{
		// Validation should have caught this, but log anyway
		UE_LOG(LogTemp, Warning, TEXT("DialogueAssetSync: Failed to deserialize conditions '%s': %s"),
			*ConditionsTokenStr, *DeserializeResult.ErrorMessage);
		return false;
	}

	// Handle CLEAR token
	if (DeserializeResult.bShouldClear)
	{
		if (Node->Conditions.Num() > 0)
		{
			Node->Conditions.Empty();
			OutResult.ConditionsCleared++;
		}
		// Also clear Line.Conditions
		if (Node->Line.Conditions.Num() > 0)
		{
			Node->Line.Conditions.Empty();
		}
	}

	// Apply new conditions
	if (DeserializeResult.Objects.Num() > 0)
	{
		// If not clearing, we replace existing conditions (design choice: XLSX is authoritative)
		if (!DeserializeResult.bShouldClear)
		{
			Node->Conditions.Empty();
		}

		for (UObject* ConditionObj : DeserializeResult.Objects)
		{
			if (UNarrativeCondition* Condition = Cast<UNarrativeCondition>(ConditionObj))
			{
				Node->Conditions.Add(Condition);
				OutResult.ConditionsApplied++;
			}
		}
	}

	return true;
}
#endif
