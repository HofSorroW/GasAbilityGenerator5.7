// GasAbilityGenerator - Quest Table Validator
// v4.8: Validates Quest table rows for correctness before generation
// Follows NPC/Dialogue validator patterns with validation cache support

#pragma once

#include "CoreMinimal.h"
#include "QuestTableEditor/QuestTableEditorTypes.h"

/**
 * Validation issue severity
 */
enum class EQuestValidationSeverity : uint8
{
	Error,		// Must be fixed before generation
	Warning		// Can generate but may cause issues
};

/**
 * Single validation issue
 */
struct FQuestValidationIssue
{
	EQuestValidationSeverity Severity;
	FString QuestName;
	FString StateID;
	FString FieldName;
	FString Message;

	FQuestValidationIssue(EQuestValidationSeverity InSeverity, const FString& InQuestName, const FString& InStateID, const FString& InFieldName, const FString& InMessage)
		: Severity(InSeverity)
		, QuestName(InQuestName)
		, StateID(InStateID)
		, FieldName(InFieldName)
		, Message(InMessage)
	{
	}

	FString ToString() const
	{
		FString SeverityStr = (Severity == EQuestValidationSeverity::Error) ? TEXT("ERROR") : TEXT("WARNING");
		if (FieldName.IsEmpty())
		{
			return FString::Printf(TEXT("[%s] %s/%s: %s"), *SeverityStr, *QuestName, *StateID, *Message);
		}
		return FString::Printf(TEXT("[%s] %s/%s.%s: %s"), *SeverityStr, *QuestName, *StateID, *FieldName, *Message);
	}
};

/**
 * Validation result containing all issues
 */
struct FQuestValidationResult
{
	TArray<FQuestValidationIssue> Issues;

	bool HasErrors() const
	{
		for (const auto& Issue : Issues)
		{
			if (Issue.Severity == EQuestValidationSeverity::Error)
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
			if (Issue.Severity == EQuestValidationSeverity::Warning)
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
			if (Issue.Severity == EQuestValidationSeverity::Error)
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
			if (Issue.Severity == EQuestValidationSeverity::Warning)
			{
				Count++;
			}
		}
		return Count;
	}
};

/**
 * Validates Quest table rows
 */
class FQuestTableValidator
{
public:
	/**
	 * Validate all rows in a Quest table
	 * @param Rows All Quest rows to validate
	 * @return Validation result with all issues
	 */
	static FQuestValidationResult ValidateAll(const TArray<FQuestTableRow>& Rows);

	/**
	 * Validate a single row
	 * @param Row Row to validate
	 * @param AllRows All rows (for cross-quest checking)
	 * @return Validation issues for this row
	 */
	static TArray<FQuestValidationIssue> ValidateRow(const FQuestTableRow& Row, const TArray<FQuestTableRow>& AllRows);

	/**
	 * Validate quest structure (states, branches, reachability)
	 * @param QuestName Quest name
	 * @param QuestRows All rows for this quest
	 * @return Validation issues for quest structure
	 */
	static TArray<FQuestValidationIssue> ValidateQuestStructure(const FString& QuestName, const TArray<const FQuestTableRow*>& QuestRows);

	//=========================================================================
	// Cache-writing validation methods
	//=========================================================================

	/**
	 * Validate all rows and write results to cache
	 * @param Rows All Quest rows to validate (modified in place)
	 * @param ListsVersionGuid Current lists version for staleness hash
	 * @return Validation result with all issues
	 */
	static FQuestValidationResult ValidateAllAndCache(TArray<FQuestTableRow>& Rows, const FGuid& ListsVersionGuid);

	/**
	 * Validate a single row and write results to its cache fields
	 * @param Row Row to validate (modified in place)
	 * @param AllRows All rows (for cross-checking)
	 * @param ListsVersionGuid Current lists version for staleness hash
	 * @return Validation issues for this row
	 */
	static TArray<FQuestValidationIssue> ValidateRowAndCache(FQuestTableRow& Row, const TArray<FQuestTableRow>& AllRows, const FGuid& ListsVersionGuid);

	/**
	 * Compute validation input hash for staleness detection
	 * @param Row Row to compute hash for
	 * @param ListsVersionGuid Current lists version
	 * @return Combined hash of editable fields + lists version
	 */
	static uint32 ComputeValidationInputHash(const FQuestTableRow& Row, const FGuid& ListsVersionGuid);

private:
	/** Check if StateID is unique within quest */
	static bool IsStateIdUnique(const FString& QuestName, const FString& StateID, const FGuid& RowId, const TArray<FQuestTableRow>& AllRows);

	/** Validate Tasks token syntax */
	static TArray<FQuestValidationIssue> ValidateTasksToken(const FQuestTableRow& Row);

	/** Validate Events token syntax */
	static TArray<FQuestValidationIssue> ValidateEventsToken(const FQuestTableRow& Row);

	/** Validate Conditions token syntax */
	static TArray<FQuestValidationIssue> ValidateConditionsToken(const FQuestTableRow& Row);

	/** Validate Rewards token syntax */
	static TArray<FQuestValidationIssue> ValidateRewardsToken(const FQuestTableRow& Row);

	/** Check if ParentBranch references a valid state */
	static bool IsParentBranchValid(const FString& QuestName, const FString& ParentBranch, const TArray<FQuestTableRow>& AllRows);
};
