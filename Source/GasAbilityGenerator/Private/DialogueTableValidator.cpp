// GasAbilityGenerator - Dialogue Table Validator Implementation
// v4.0: Validates dialogue table rows for correctness

#include "DialogueTableValidator.h"

FDialogueValidationResult FDialogueTableValidator::ValidateAll(const TArray<FDialogueTableRow>& Rows)
{
	FDialogueValidationResult Result;

	// Validate each row individually
	for (const FDialogueTableRow& Row : Rows)
	{
		TArray<FDialogueValidationIssue> RowIssues = ValidateRow(Row, Rows);
		Result.Issues.Append(RowIssues);
	}

	// Group rows by DialogueID and validate tree structure
	TMap<FName, TArray<FDialogueTableRow>> DialogueGroups;
	for (const FDialogueTableRow& Row : Rows)
	{
		if (!Row.DialogueID.IsNone())
		{
			DialogueGroups.FindOrAdd(Row.DialogueID).Add(Row);
		}
	}

	for (const auto& Pair : DialogueGroups)
	{
		TArray<FDialogueValidationIssue> TreeIssues = ValidateDialogueTree(Pair.Key, Pair.Value);
		Result.Issues.Append(TreeIssues);
	}

	return Result;
}

TArray<FDialogueValidationIssue> FDialogueTableValidator::ValidateRow(const FDialogueTableRow& Row, const TArray<FDialogueTableRow>& AllRows)
{
	TArray<FDialogueValidationIssue> Issues;

	// Required: DialogueID
	if (Row.DialogueID.IsNone())
	{
		Issues.Add(FDialogueValidationIssue(
			EDialogueValidationSeverity::Error,
			NAME_None,
			Row.NodeID,
			TEXT("DialogueID is required")
		));
	}

	// Required: NodeID
	if (Row.NodeID.IsNone())
	{
		Issues.Add(FDialogueValidationIssue(
			EDialogueValidationSeverity::Error,
			Row.DialogueID,
			NAME_None,
			TEXT("NodeID is required")
		));
	}

	// NPC nodes require Text
	if (Row.NodeType == EDialogueTableNodeType::NPC && Row.Text.IsEmpty())
	{
		Issues.Add(FDialogueValidationIssue(
			EDialogueValidationSeverity::Error,
			Row.DialogueID,
			Row.NodeID,
			TEXT("NPC nodes require Text")
		));
	}

	// NPC nodes require Speaker
	if (Row.NodeType == EDialogueTableNodeType::NPC && Row.Speaker.IsNone())
	{
		Issues.Add(FDialogueValidationIssue(
			EDialogueValidationSeverity::Warning,
			Row.DialogueID,
			Row.NodeID,
			TEXT("NPC nodes should have a Speaker")
		));
	}

	// Player nodes require OptionText
	if (Row.NodeType == EDialogueTableNodeType::Player && Row.OptionText.IsEmpty())
	{
		Issues.Add(FDialogueValidationIssue(
			EDialogueValidationSeverity::Error,
			Row.DialogueID,
			Row.NodeID,
			TEXT("Player nodes require OptionText")
		));
	}

	// Player nodes should have Text (what player actually says)
	if (Row.NodeType == EDialogueTableNodeType::Player && Row.Text.IsEmpty())
	{
		Issues.Add(FDialogueValidationIssue(
			EDialogueValidationSeverity::Warning,
			Row.DialogueID,
			Row.NodeID,
			TEXT("Player nodes should have Text (what player says)")
		));
	}

	// Validate ParentNodeID reference exists (if specified)
	if (!Row.ParentNodeID.IsNone())
	{
		bool bParentFound = false;
		for (const FDialogueTableRow& OtherRow : AllRows)
		{
			if (OtherRow.DialogueID == Row.DialogueID && OtherRow.NodeID == Row.ParentNodeID)
			{
				bParentFound = true;
				break;
			}
		}
		if (!bParentFound)
		{
			Issues.Add(FDialogueValidationIssue(
				EDialogueValidationSeverity::Error,
				Row.DialogueID,
				Row.NodeID,
				FString::Printf(TEXT("ParentNodeID '%s' not found in dialogue"), *Row.ParentNodeID.ToString())
			));
		}
	}

	// Validate NextNodeIDs references exist
	for (const FName& NextID : Row.NextNodeIDs)
	{
		if (NextID.IsNone())
		{
			continue;
		}
		bool bNextFound = false;
		for (const FDialogueTableRow& OtherRow : AllRows)
		{
			if (OtherRow.DialogueID == Row.DialogueID && OtherRow.NodeID == NextID)
			{
				bNextFound = true;
				break;
			}
		}
		if (!bNextFound)
		{
			Issues.Add(FDialogueValidationIssue(
				EDialogueValidationSeverity::Error,
				Row.DialogueID,
				Row.NodeID,
				FString::Printf(TEXT("NextNodeID '%s' not found in dialogue"), *NextID.ToString())
			));
		}
	}

	// Check for duplicate NodeID within same dialogue
	int32 DuplicateCount = 0;
	for (const FDialogueTableRow& OtherRow : AllRows)
	{
		if (OtherRow.DialogueID == Row.DialogueID && OtherRow.NodeID == Row.NodeID)
		{
			DuplicateCount++;
		}
	}
	if (DuplicateCount > 1)
	{
		Issues.Add(FDialogueValidationIssue(
			EDialogueValidationSeverity::Error,
			Row.DialogueID,
			Row.NodeID,
			TEXT("Duplicate NodeID in dialogue")
		));
	}

	return Issues;
}

TArray<FDialogueValidationIssue> FDialogueTableValidator::ValidateDialogueTree(FName DialogueID, const TArray<FDialogueTableRow>& DialogueRows)
{
	TArray<FDialogueValidationIssue> Issues;

	if (DialogueRows.Num() == 0)
	{
		Issues.Add(FDialogueValidationIssue(
			EDialogueValidationSeverity::Error,
			DialogueID,
			NAME_None,
			TEXT("Dialogue has no nodes")
		));
		return Issues;
	}

	// Find root nodes (nodes with no parent)
	TArray<const FDialogueTableRow*> RootNodes;
	for (const FDialogueTableRow& Row : DialogueRows)
	{
		if (Row.ParentNodeID.IsNone())
		{
			RootNodes.Add(&Row);
		}
	}

	if (RootNodes.Num() == 0)
	{
		Issues.Add(FDialogueValidationIssue(
			EDialogueValidationSeverity::Error,
			DialogueID,
			NAME_None,
			TEXT("Dialogue has no root node (all nodes have parents)")
		));
	}
	else if (RootNodes.Num() > 1)
	{
		// Multiple roots - warn but don't error (could be intentional)
		FString RootNames;
		for (const FDialogueTableRow* Root : RootNodes)
		{
			if (!RootNames.IsEmpty()) RootNames += TEXT(", ");
			RootNames += Root->NodeID.ToString();
		}
		Issues.Add(FDialogueValidationIssue(
			EDialogueValidationSeverity::Warning,
			DialogueID,
			NAME_None,
			FString::Printf(TEXT("Multiple root nodes found: %s"), *RootNames)
		));
	}

	// Check for circular references
	for (const FDialogueTableRow& Row : DialogueRows)
	{
		TSet<FName> Visited;
		if (HasCircularReference(Row, DialogueRows, Visited))
		{
			Issues.Add(FDialogueValidationIssue(
				EDialogueValidationSeverity::Error,
				DialogueID,
				Row.NodeID,
				TEXT("Circular reference detected in dialogue tree")
			));
			break; // Only report once per dialogue
		}
	}

	// Check for orphan nodes (not reachable from any root)
	TSet<FName> ReachableNodes;
	TArray<FName> ToVisit;
	for (const FDialogueTableRow* Root : RootNodes)
	{
		ToVisit.Add(Root->NodeID);
	}

	while (ToVisit.Num() > 0)
	{
		FName Current = ToVisit.Pop();
		if (ReachableNodes.Contains(Current))
		{
			continue;
		}
		ReachableNodes.Add(Current);

		// Find this node and add its children
		for (const FDialogueTableRow& Row : DialogueRows)
		{
			if (Row.NodeID == Current)
			{
				for (const FName& NextID : Row.NextNodeIDs)
				{
					if (!NextID.IsNone() && !ReachableNodes.Contains(NextID))
					{
						ToVisit.Add(NextID);
					}
				}
				break;
			}
		}
	}

	// Find orphans
	for (const FDialogueTableRow& Row : DialogueRows)
	{
		if (!ReachableNodes.Contains(Row.NodeID) && !Row.ParentNodeID.IsNone())
		{
			Issues.Add(FDialogueValidationIssue(
				EDialogueValidationSeverity::Warning,
				DialogueID,
				Row.NodeID,
				TEXT("Node is not reachable from any root")
			));
		}
	}

	return Issues;
}

bool FDialogueTableValidator::HasCircularReference(const FDialogueTableRow& StartRow, const TArray<FDialogueTableRow>& DialogueRows, TSet<FName>& Visited)
{
	if (Visited.Contains(StartRow.NodeID))
	{
		return true;
	}

	Visited.Add(StartRow.NodeID);

	for (const FName& NextID : StartRow.NextNodeIDs)
	{
		if (NextID.IsNone())
		{
			continue;
		}

		for (const FDialogueTableRow& Row : DialogueRows)
		{
			if (Row.NodeID == NextID)
			{
				TSet<FName> VisitedCopy = Visited;
				if (HasCircularReference(Row, DialogueRows, VisitedCopy))
				{
					return true;
				}
				break;
			}
		}
	}

	return false;
}
