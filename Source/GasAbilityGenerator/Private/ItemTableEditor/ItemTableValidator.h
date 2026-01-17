// GasAbilityGenerator - Item Table Validator
// v4.8: Validates Item table rows for correctness before generation
// Follows NPC/Dialogue validator patterns with validation cache support

#pragma once

#include "CoreMinimal.h"
#include "ItemTableEditor/ItemTableEditorTypes.h"

/**
 * Validation issue severity
 */
enum class EItemValidationSeverity : uint8
{
	Error,		// Must be fixed before generation
	Warning		// Can generate but may cause issues
};

/**
 * Single validation issue
 */
struct FItemValidationIssue
{
	EItemValidationSeverity Severity;
	FString ItemName;
	FString FieldName;
	FString Message;

	FItemValidationIssue(EItemValidationSeverity InSeverity, const FString& InItemName, const FString& InFieldName, const FString& InMessage)
		: Severity(InSeverity)
		, ItemName(InItemName)
		, FieldName(InFieldName)
		, Message(InMessage)
	{
	}

	FString ToString() const
	{
		FString SeverityStr = (Severity == EItemValidationSeverity::Error) ? TEXT("ERROR") : TEXT("WARNING");
		if (FieldName.IsEmpty())
		{
			return FString::Printf(TEXT("[%s] %s: %s"), *SeverityStr, *ItemName, *Message);
		}
		return FString::Printf(TEXT("[%s] %s.%s: %s"), *SeverityStr, *ItemName, *FieldName, *Message);
	}
};

/**
 * Validation result containing all issues
 */
struct FItemValidationResult
{
	TArray<FItemValidationIssue> Issues;

	bool HasErrors() const
	{
		for (const auto& Issue : Issues)
		{
			if (Issue.Severity == EItemValidationSeverity::Error)
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
			if (Issue.Severity == EItemValidationSeverity::Warning)
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
			if (Issue.Severity == EItemValidationSeverity::Error)
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
			if (Issue.Severity == EItemValidationSeverity::Warning)
			{
				Count++;
			}
		}
		return Count;
	}
};

/**
 * Validates Item table rows
 */
class FItemTableValidator
{
public:
	/**
	 * Validate all rows in an Item table
	 * @param Rows All Item rows to validate
	 * @return Validation result with all issues
	 */
	static FItemValidationResult ValidateAll(const TArray<FItemTableRow>& Rows);

	/**
	 * Validate a single row
	 * @param Row Row to validate
	 * @param AllRows All rows (for uniqueness checking)
	 * @return Validation issues for this row
	 */
	static TArray<FItemValidationIssue> ValidateRow(const FItemTableRow& Row, const TArray<FItemTableRow>& AllRows);

	//=========================================================================
	// Cache-writing validation methods
	//=========================================================================

	/**
	 * Validate all rows and write results to cache
	 * @param Rows All Item rows to validate (modified in place)
	 * @param ListsVersionGuid Current lists version for staleness hash
	 * @return Validation result with all issues
	 */
	static FItemValidationResult ValidateAllAndCache(TArray<FItemTableRow>& Rows, const FGuid& ListsVersionGuid);

	/**
	 * Validate a single row and write results to its cache fields
	 * @param Row Row to validate (modified in place)
	 * @param AllRows All rows (for uniqueness checking)
	 * @param ListsVersionGuid Current lists version for staleness hash
	 * @return Validation issues for this row
	 */
	static TArray<FItemValidationIssue> ValidateRowAndCache(FItemTableRow& Row, const TArray<FItemTableRow>& AllRows, const FGuid& ListsVersionGuid);

	/**
	 * Compute validation input hash for staleness detection
	 * @param Row Row to compute hash for
	 * @param ListsVersionGuid Current lists version
	 * @return Combined hash of editable fields + lists version
	 */
	static uint32 ComputeValidationInputHash(const FItemTableRow& Row, const FGuid& ListsVersionGuid);

private:
	/** Check if ItemName is unique across all rows */
	static bool IsItemNameUnique(const FString& ItemName, const FGuid& RowId, const TArray<FItemTableRow>& AllRows);

	/** Validate equipment slot for item type */
	static TArray<FItemValidationIssue> ValidateEquipmentSlot(const FItemTableRow& Row);

	/** Validate WeaponConfig token syntax */
	static TArray<FItemValidationIssue> ValidateWeaponConfig(const FItemTableRow& Row);

	/** Validate referenced assets exist (ModifierGE, Abilities) */
	static TArray<FItemValidationIssue> ValidateAssetReferences(const FItemTableRow& Row);

	/** Validate combat stats for item type */
	static TArray<FItemValidationIssue> ValidateCombatStats(const FItemTableRow& Row);

	/** Validate stacking properties */
	static TArray<FItemValidationIssue> ValidateStackingProperties(const FItemTableRow& Row);
};
