// GasAbilityGenerator - Dialogue Asset Sync Implementation
// v4.4 Phase 3: Read events/conditions from UDialogueBlueprint for [RO] columns

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

	// Process NPC replies
	for (UDialogueNode_NPC* NPCNode : DialogueTemplate->NPCReplies)
	{
		if (!NPCNode) continue;

		FDialogueNodeAssetData NodeData;
		ExtractNodeData(NPCNode, NodeData);
		NodeData.NodeType = EDialogueTableNodeType::NPC;
		NodeData.bFoundInAsset = true;

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

	// Process Player replies
	for (UDialogueNode_Player* PlayerNode : DialogueTemplate->PlayerReplies)
	{
		if (!PlayerNode) continue;

		FDialogueNodeAssetData NodeData;
		ExtractNodeData(PlayerNode, NodeData);
		NodeData.NodeType = EDialogueTableNodeType::Player;
		NodeData.bFoundInAsset = true;

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
#endif
