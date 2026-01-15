// GasAbilityGenerator - NPC Table Validator
// v4.5: Validates NPC table rows for correctness before generation

#pragma once

#include "CoreMinimal.h"
#include "NPCTableEditor/NPCTableEditorTypes.h"

/**
 * Validation issue severity
 */
enum class ENPCValidationSeverity : uint8
{
	Error,		// Must be fixed before generation
	Warning		// Can generate but may cause issues
};

/**
 * Single validation issue
 */
struct FNPCValidationIssue
{
	ENPCValidationSeverity Severity;
	FString NPCName;
	FString FieldName;
	FString Message;

	FNPCValidationIssue(ENPCValidationSeverity InSeverity, const FString& InNPCName, const FString& InFieldName, const FString& InMessage)
		: Severity(InSeverity)
		, NPCName(InNPCName)
		, FieldName(InFieldName)
		, Message(InMessage)
	{
	}

	FString ToString() const
	{
		FString SeverityStr = (Severity == ENPCValidationSeverity::Error) ? TEXT("ERROR") : TEXT("WARNING");
		if (FieldName.IsEmpty())
		{
			return FString::Printf(TEXT("[%s] %s: %s"), *SeverityStr, *NPCName, *Message);
		}
		return FString::Printf(TEXT("[%s] %s.%s: %s"), *SeverityStr, *NPCName, *FieldName, *Message);
	}
};

/**
 * Validation result containing all issues
 */
struct FNPCValidationResult
{
	TArray<FNPCValidationIssue> Issues;

	bool HasErrors() const
	{
		for (const auto& Issue : Issues)
		{
			if (Issue.Severity == ENPCValidationSeverity::Error)
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
			if (Issue.Severity == ENPCValidationSeverity::Warning)
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
			if (Issue.Severity == ENPCValidationSeverity::Error)
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
			if (Issue.Severity == ENPCValidationSeverity::Warning)
			{
				Count++;
			}
		}
		return Count;
	}
};

/**
 * Validates NPC table rows
 */
class FNPCTableValidator
{
public:
	/**
	 * Validate all rows in an NPC table
	 * @param Rows All NPC rows to validate
	 * @return Validation result with all issues
	 */
	static FNPCValidationResult ValidateAll(const TArray<FNPCTableRow>& Rows);

	/**
	 * Validate a single row
	 * @param Row Row to validate
	 * @param AllRows All rows (for uniqueness checking)
	 * @return Validation issues for this row
	 */
	static TArray<FNPCValidationIssue> ValidateRow(const FNPCTableRow& Row, const TArray<FNPCTableRow>& AllRows);

	//=========================================================================
	// v4.5: Cache-writing validation methods
	//=========================================================================

	/**
	 * Validate all rows and write results to cache
	 * @param Rows All NPC rows to validate (modified in place)
	 * @param ListsVersionGuid Current lists version for staleness hash
	 * @return Validation result with all issues
	 */
	static FNPCValidationResult ValidateAllAndCache(TArray<FNPCTableRow>& Rows, const FGuid& ListsVersionGuid);

	/**
	 * Validate a single row and write results to its cache fields
	 * @param Row Row to validate (modified in place)
	 * @param AllRows All rows (for uniqueness checking)
	 * @param ListsVersionGuid Current lists version for staleness hash
	 * @return Validation issues for this row
	 */
	static TArray<FNPCValidationIssue> ValidateRowAndCache(FNPCTableRow& Row, const TArray<FNPCTableRow>& AllRows, const FGuid& ListsVersionGuid);

	/**
	 * Compute validation input hash for staleness detection
	 * @param Row Row to compute hash for
	 * @param ListsVersionGuid Current lists version
	 * @return Combined hash of editable fields + lists version
	 */
	static uint32 ComputeValidationInputHash(const FNPCTableRow& Row, const FGuid& ListsVersionGuid);

private:
	/** Check if NPCId is unique across all rows */
	static bool IsNPCIdUnique(const FString& NPCId, const FGuid& RowId, const TArray<FNPCTableRow>& AllRows);

	/** Check if NPCName is unique across all rows */
	static bool IsNPCNameUnique(const FString& NPCName, const FGuid& RowId, const TArray<FNPCTableRow>& AllRows);
};
