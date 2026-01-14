// GasAbilityGenerator - Dialogue Table Converter Implementation
// v4.0: Converts dialogue table rows to manifest definitions

#include "DialogueTableConverter.h"

TMap<FName, FManifestDialogueBlueprintDefinition> FDialogueTableConverter::ConvertRowsToManifest(const TArray<FDialogueTableRow>& Rows)
{
	TMap<FName, FManifestDialogueBlueprintDefinition> Result;

	// Group rows by DialogueID
	TMap<FName, TArray<FDialogueTableRow>> DialogueGroups;
	for (const FDialogueTableRow& Row : Rows)
	{
		if (!Row.DialogueID.IsNone())
		{
			DialogueGroups.FindOrAdd(Row.DialogueID).Add(Row);
		}
	}

	// Convert each dialogue group
	for (const auto& Pair : DialogueGroups)
	{
		FName DialogueID = Pair.Key;
		const TArray<FDialogueTableRow>& DialogueRows = Pair.Value;

		FManifestDialogueBlueprintDefinition Def;
		Def.Name = DialogueID.ToString();
		Def.Folder = TEXT("Dialogues");

		// Build dialogue tree
		Def.DialogueTree = BuildDialogueTree(DialogueRows);

		// Get unique speakers and create speaker definitions
		TArray<FName> Speakers = GetUniqueSpeakers(DialogueRows);
		for (const FName& SpeakerName : Speakers)
		{
			FManifestDialogueSpeakerDefinition SpeakerDef;
			SpeakerDef.SpeakerID = SpeakerName.ToString();
			// Note: NPCDefinition would need to be resolved separately
			// For now, leave it empty - manual setup or Phase 2
			Def.Speakers.Add(SpeakerDef);
		}

		// Apply sensible defaults
		ApplyDefaults(Def);

		Result.Add(DialogueID, Def);
	}

	return Result;
}

FManifestDialogueTreeDefinition FDialogueTableConverter::BuildDialogueTree(const TArray<FDialogueTableRow>& DialogueRows)
{
	FManifestDialogueTreeDefinition Tree;

	// Build a lookup map for node types
	TMap<FName, EDialogueTableNodeType> NodeTypeMap;
	for (const FDialogueTableRow& Row : DialogueRows)
	{
		NodeTypeMap.Add(Row.NodeID, Row.NodeType);
	}

	// Find root node (node with no parent)
	for (const FDialogueTableRow& Row : DialogueRows)
	{
		if (Row.ParentNodeID.IsNone())
		{
			Tree.RootNodeId = Row.NodeID.ToString();
			break;
		}
	}

	// If no explicit root found, use first node
	if (Tree.RootNodeId.IsEmpty() && DialogueRows.Num() > 0)
	{
		Tree.RootNodeId = DialogueRows[0].NodeID.ToString();
	}

	// Convert all rows to nodes
	for (const FDialogueTableRow& Row : DialogueRows)
	{
		FManifestDialogueNodeDefinition Node = ConvertRowToNode(Row);

		// Categorize NextNodeIDs into NPCReplies/PlayerReplies based on child type
		for (const FName& NextID : Row.NextNodeIDs)
		{
			if (NextID.IsNone())
			{
				continue;
			}

			EDialogueTableNodeType* ChildType = NodeTypeMap.Find(NextID);
			if (ChildType)
			{
				if (*ChildType == EDialogueTableNodeType::NPC)
				{
					Node.NPCReplies.Add(NextID.ToString());
				}
				else
				{
					Node.PlayerReplies.Add(NextID.ToString());
				}
			}
		}

		Tree.Nodes.Add(Node);
	}

	return Tree;
}

void FDialogueTableConverter::ApplyDefaults(FManifestDialogueBlueprintDefinition& Def)
{
	// Dialogue-level defaults (from Narrative Pro standard settings)
	Def.ParentClass = TEXT("NarrativeDialogue");
	Def.bShowCinematicBars = true;
	Def.bFreeMovement = false;
	Def.bCanBeExited = true;
	Def.bAutoStopMovement = true;
	Def.bAutoRotateSpeakers = false;
	Def.bUnskippable = false;
	Def.Priority = 0;
	Def.EndDialogueDist = -1.0f;
	Def.DefaultHeadBoneName = TEXT("head");
	Def.DialogueBlendOutTime = 0.5f;
	Def.bAdjustPlayerTransform = false;
}

FManifestDialogueNodeDefinition FDialogueTableConverter::ConvertRowToNode(const FDialogueTableRow& Row)
{
	FManifestDialogueNodeDefinition Node;

	Node.Id = Row.NodeID.ToString();
	Node.Type = (Row.NodeType == EDialogueTableNodeType::NPC) ? TEXT("npc") : TEXT("player");
	Node.Speaker = Row.Speaker.ToString();
	Node.Text = Row.Text;
	Node.OptionText = Row.OptionText;

	// Node-level defaults
	Node.Duration = TEXT("Default");
	Node.DurationSeconds = 0.0f;
	Node.bIsSkippable = true;
	Node.bAutoSelectIfOnly = true;
	Node.bAutoSelect = false;

	// NextNodeIDs will be categorized into NPCReplies/PlayerReplies
	// in BuildDialogueTree after all nodes are created
	// Store them temporarily - they'll be processed in a second pass

	return Node;
}

TArray<FName> FDialogueTableConverter::GetUniqueSpeakers(const TArray<FDialogueTableRow>& DialogueRows)
{
	TSet<FName> SpeakerSet;
	for (const FDialogueTableRow& Row : DialogueRows)
	{
		if (Row.NodeType == EDialogueTableNodeType::NPC && !Row.Speaker.IsNone())
		{
			SpeakerSet.Add(Row.Speaker);
		}
	}
	return SpeakerSet.Array();
}
