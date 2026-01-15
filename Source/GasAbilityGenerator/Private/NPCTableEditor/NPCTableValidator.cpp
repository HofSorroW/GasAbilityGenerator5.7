// GasAbilityGenerator - NPC Table Validator Implementation
// v4.5: Validates NPC table rows for correctness before generation

#include "NPCTableEditor/NPCTableValidator.h"

FNPCValidationResult FNPCTableValidator::ValidateAll(const TArray<FNPCTableRow>& Rows)
{
	FNPCValidationResult Result;

	for (const FNPCTableRow& Row : Rows)
	{
		TArray<FNPCValidationIssue> RowIssues = ValidateRow(Row, Rows);
		Result.Issues.Append(RowIssues);
	}

	return Result;
}

TArray<FNPCValidationIssue> FNPCTableValidator::ValidateRow(const FNPCTableRow& Row, const TArray<FNPCTableRow>& AllRows)
{
	TArray<FNPCValidationIssue> Issues;

	// Use NPCName for display, fallback to RowId if empty
	FString DisplayName = Row.NPCName.IsEmpty() ? Row.RowId.ToString() : Row.NPCName;

	//=========================================================================
	// Required Fields (Errors)
	//=========================================================================

	// NPCName is required
	if (Row.NPCName.IsEmpty())
	{
		Issues.Add(FNPCValidationIssue(
			ENPCValidationSeverity::Error,
			DisplayName,
			TEXT("NPCName"),
			TEXT("NPC Name is required")
		));
	}
	else
	{
		// NPCName should not contain spaces (used for asset naming)
		if (Row.NPCName.Contains(TEXT(" ")))
		{
			Issues.Add(FNPCValidationIssue(
				ENPCValidationSeverity::Error,
				DisplayName,
				TEXT("NPCName"),
				TEXT("NPC Name cannot contain spaces (used for asset naming)")
			));
		}

		// NPCName must be unique
		if (!IsNPCNameUnique(Row.NPCName, Row.RowId, AllRows))
		{
			Issues.Add(FNPCValidationIssue(
				ENPCValidationSeverity::Error,
				DisplayName,
				TEXT("NPCName"),
				TEXT("NPC Name must be unique")
			));
		}
	}

	// NPCId is required
	if (Row.NPCId.IsEmpty())
	{
		Issues.Add(FNPCValidationIssue(
			ENPCValidationSeverity::Error,
			DisplayName,
			TEXT("NPCId"),
			TEXT("NPC ID is required")
		));
	}
	else
	{
		// NPCId must be unique
		if (!IsNPCIdUnique(Row.NPCId, Row.RowId, AllRows))
		{
			Issues.Add(FNPCValidationIssue(
				ENPCValidationSeverity::Error,
				DisplayName,
				TEXT("NPCId"),
				TEXT("NPC ID must be unique")
			));
		}
	}

	//=========================================================================
	// Level Range Validation (Errors)
	//=========================================================================

	if (Row.MinLevel < 1)
	{
		Issues.Add(FNPCValidationIssue(
			ENPCValidationSeverity::Error,
			DisplayName,
			TEXT("MinLevel"),
			TEXT("Min Level must be at least 1")
		));
	}

	if (Row.MaxLevel < 1)
	{
		Issues.Add(FNPCValidationIssue(
			ENPCValidationSeverity::Error,
			DisplayName,
			TEXT("MaxLevel"),
			TEXT("Max Level must be at least 1")
		));
	}

	if (Row.MinLevel > Row.MaxLevel)
	{
		Issues.Add(FNPCValidationIssue(
			ENPCValidationSeverity::Error,
			DisplayName,
			TEXT("LevelRange"),
			FString::Printf(TEXT("Min Level (%d) cannot be greater than Max Level (%d)"), Row.MinLevel, Row.MaxLevel)
		));
	}

	//=========================================================================
	// Vendor Validation (Errors)
	//=========================================================================

	if (Row.bIsVendor && Row.ShopName.IsEmpty())
	{
		Issues.Add(FNPCValidationIssue(
			ENPCValidationSeverity::Error,
			DisplayName,
			TEXT("ShopName"),
			TEXT("Shop Name is required when Is Vendor is enabled")
		));
	}

	//=========================================================================
	// Attack Priority Validation (Errors)
	//=========================================================================

	if (Row.AttackPriority < 0.0f || Row.AttackPriority > 1.0f)
	{
		Issues.Add(FNPCValidationIssue(
			ENPCValidationSeverity::Error,
			DisplayName,
			TEXT("AttackPriority"),
			FString::Printf(TEXT("Attack Priority must be between 0.0 and 1.0 (current: %.2f)"), Row.AttackPriority)
		));
	}

	//=========================================================================
	// Warnings (can still generate but may cause issues)
	//=========================================================================

	// DisplayName warning (recommended but not required)
	if (Row.DisplayName.IsEmpty())
	{
		Issues.Add(FNPCValidationIssue(
			ENPCValidationSeverity::Warning,
			DisplayName,
			TEXT("DisplayName"),
			TEXT("Display Name is empty - NPC will have no in-game name")
		));
	}

	// Blueprint warning (recommended but not required)
	if (Row.Blueprint.IsNull())
	{
		Issues.Add(FNPCValidationIssue(
			ENPCValidationSeverity::Warning,
			DisplayName,
			TEXT("Blueprint"),
			TEXT("No NPC Blueprint assigned - NPC will need manual setup")
		));
	}

	// AbilityConfig warning
	if (Row.AbilityConfig.IsNull())
	{
		Issues.Add(FNPCValidationIssue(
			ENPCValidationSeverity::Warning,
			DisplayName,
			TEXT("AbilityConfig"),
			TEXT("No Ability Configuration assigned - NPC will have no abilities")
		));
	}

	// ActivityConfig warning
	if (Row.ActivityConfig.IsNull())
	{
		Issues.Add(FNPCValidationIssue(
			ENPCValidationSeverity::Warning,
			DisplayName,
			TEXT("ActivityConfig"),
			TEXT("No Activity Configuration assigned - NPC will have no activities")
		));
	}

	// Factions warning
	if (Row.Factions.IsEmpty())
	{
		Issues.Add(FNPCValidationIssue(
			ENPCValidationSeverity::Warning,
			DisplayName,
			TEXT("Factions"),
			TEXT("No factions assigned - NPC will be faction-less")
		));
	}

	return Issues;
}

bool FNPCTableValidator::IsNPCIdUnique(const FString& NPCId, const FGuid& RowId, const TArray<FNPCTableRow>& AllRows)
{
	for (const FNPCTableRow& Row : AllRows)
	{
		// Skip self
		if (Row.RowId == RowId)
		{
			continue;
		}

		if (Row.NPCId.Equals(NPCId, ESearchCase::IgnoreCase))
		{
			return false;
		}
	}
	return true;
}

bool FNPCTableValidator::IsNPCNameUnique(const FString& NPCName, const FGuid& RowId, const TArray<FNPCTableRow>& AllRows)
{
	for (const FNPCTableRow& Row : AllRows)
	{
		// Skip self
		if (Row.RowId == RowId)
		{
			continue;
		}

		if (Row.NPCName.Equals(NPCName, ESearchCase::IgnoreCase))
		{
			return false;
		}
	}
	return true;
}
