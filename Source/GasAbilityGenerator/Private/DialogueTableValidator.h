// GasAbilityGenerator - Dialogue Table Validator
// v4.0: Validates dialogue table rows for correctness

#pragma once

#include "CoreMinimal.h"
#include "DialogueTableEditorTypes.h"

/**
 * Validation issue severity
 */
enum class EDialogueValidationSeverity : uint8
{
	Error,		// Must be fixed before generation
	Warning		// Can generate but may cause issues
};

/**
 * Single validation issue
 */
struct FDialogueValidationIssue
{
	EDialogueValidationSeverity Severity;
	FName DialogueID;
	FName NodeID;
	FString Message;

	FDialogueValidationIssue(EDialogueValidationSeverity InSeverity, FName InDialogueID, FName InNodeID, const FString& InMessage)
		: Severity(InSeverity)
		, DialogueID(InDialogueID)
		, NodeID(InNodeID)
		, Message(InMessage)
	{
	}

	FString ToString() const
	{
		FString SeverityStr = (Severity == EDialogueValidationSeverity::Error) ? TEXT("ERROR") : TEXT("WARNING");
		if (NodeID.IsNone())
		{
			return FString::Printf(TEXT("[%s] %s: %s"), *SeverityStr, *DialogueID.ToString(), *Message);
		}
		return FString::Printf(TEXT("[%s] %s.%s: %s"), *SeverityStr, *DialogueID.ToString(), *NodeID.ToString(), *Message);
	}
};

/**
 * Validation result containing all issues
 */
struct FDialogueValidationResult
{
	TArray<FDialogueValidationIssue> Issues;

	bool HasErrors() const
	{
		for (const auto& Issue : Issues)
		{
			if (Issue.Severity == EDialogueValidationSeverity::Error)
			{
				return true;
			}
		}
		return false;
	}

	bool HasWarnings() const
	{
		for (const auto& Issue : Issues)
		{
			if (Issue.Severity == EDialogueValidationSeverity::Warning)
			{
				return true;
			}
		}
		return false;
	}

	int32 GetErrorCount() const
	{
		int32 Count = 0;
		for (const auto& Issue : Issues)
		{
			if (Issue.Severity == EDialogueValidationSeverity::Error)
			{
				Count++;
			}
		}
		return Count;
	}

	int32 GetWarningCount() const
	{
		int32 Count = 0;
		for (const auto& Issue : Issues)
		{
			if (Issue.Severity == EDialogueValidationSeverity::Warning)
			{
				Count++;
			}
		}
		return Count;
	}
};

/**
 * Validates dialogue table rows
 */
class FDialogueTableValidator
{
public:
	/**
	 * Validate all rows in a dialogue table
	 * @param Rows All dialogue rows to validate
	 * @return Validation result with all issues
	 */
	static FDialogueValidationResult ValidateAll(const TArray<FDialogueTableRow>& Rows);

	/**
	 * Validate a single row
	 * @param Row Row to validate
	 * @param AllRows All rows (for reference checking)
	 * @return Validation issues for this row
	 */
	static TArray<FDialogueValidationIssue> ValidateRow(const FDialogueTableRow& Row, const TArray<FDialogueTableRow>& AllRows);

	/**
	 * Validate dialogue tree structure for a single dialogue
	 * @param DialogueID The dialogue to validate
	 * @param DialogueRows All rows belonging to this dialogue
	 * @return Validation issues for the tree structure
	 */
	static TArray<FDialogueValidationIssue> ValidateDialogueTree(FName DialogueID, const TArray<FDialogueTableRow>& DialogueRows);

private:
	/** Check for circular references in dialogue tree */
	static bool HasCircularReference(const FDialogueTableRow& StartRow, const TArray<FDialogueTableRow>& DialogueRows, TSet<FName>& Visited);
};
