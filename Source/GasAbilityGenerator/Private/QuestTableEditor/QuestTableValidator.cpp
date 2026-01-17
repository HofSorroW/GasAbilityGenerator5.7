// GasAbilityGenerator - Quest Table Validator
// v4.8: Implementation

#include "QuestTableValidator.h"

FQuestValidationResult FQuestTableValidator::ValidateAll(const TArray<FQuestTableRow>& Rows)
{
	FQuestValidationResult Result;

	// Group by quest for structure validation
	TMap<FString, TArray<const FQuestTableRow*>> QuestGroups;
	for (const FQuestTableRow& Row : Rows)
	{
		if (!Row.bDeleted)
		{
			TArray<FQuestValidationIssue> RowIssues = ValidateRow(Row, Rows);
			Result.Issues.Append(RowIssues);

			if (!Row.QuestName.IsEmpty())
			{
				QuestGroups.FindOrAdd(Row.QuestName).Add(&Row);
			}
		}
	}

	// Validate each quest's structure
	for (const auto& Pair : QuestGroups)
	{
		TArray<FQuestValidationIssue> StructureIssues = ValidateQuestStructure(Pair.Key, Pair.Value);
		Result.Issues.Append(StructureIssues);
	}

	return Result;
}

TArray<FQuestValidationIssue> FQuestTableValidator::ValidateRow(const FQuestTableRow& Row, const TArray<FQuestTableRow>& AllRows)
{
	TArray<FQuestValidationIssue> Issues;

	// Required fields
	if (Row.QuestName.IsEmpty())
	{
		Issues.Add(FQuestValidationIssue(
			EQuestValidationSeverity::Error,
			TEXT("Unknown"),
			Row.StateID.IsEmpty() ? TEXT("Unknown") : Row.StateID,
			TEXT("QuestName"),
			TEXT("Quest name is required")));
	}

	if (Row.StateID.IsEmpty())
	{
		Issues.Add(FQuestValidationIssue(
			EQuestValidationSeverity::Error,
			Row.QuestName,
			TEXT("Unknown"),
			TEXT("StateID"),
			TEXT("State ID is required")));
	}

	// State ID uniqueness within quest
	if (!Row.StateID.IsEmpty() && !Row.QuestName.IsEmpty())
	{
		if (!IsStateIdUnique(Row.QuestName, Row.StateID, Row.RowId, AllRows))
		{
			Issues.Add(FQuestValidationIssue(
				EQuestValidationSeverity::Error,
				Row.QuestName,
				Row.StateID,
				TEXT("StateID"),
				TEXT("State ID must be unique within quest")));
		}
	}

	// Parent branch validation
	if (!Row.ParentBranch.IsEmpty() && !Row.QuestName.IsEmpty())
	{
		if (!IsParentBranchValid(Row.QuestName, Row.ParentBranch, AllRows))
		{
			Issues.Add(FQuestValidationIssue(
				EQuestValidationSeverity::Warning,
				Row.QuestName,
				Row.StateID,
				TEXT("ParentBranch"),
				FString::Printf(TEXT("Parent branch '%s' does not reference a valid state"), *Row.ParentBranch)));
		}
	}

	// Token validation
	Issues.Append(ValidateTasksToken(Row));
	Issues.Append(ValidateEventsToken(Row));
	Issues.Append(ValidateConditionsToken(Row));
	Issues.Append(ValidateRewardsToken(Row));

	// Success/Failure states should have rewards
	if ((Row.StateType == EQuestStateType::Success || Row.StateType == EQuestStateType::Failure) && Row.Rewards.IsEmpty())
	{
		Issues.Add(FQuestValidationIssue(
			EQuestValidationSeverity::Warning,
			Row.QuestName,
			Row.StateID,
			TEXT("Rewards"),
			FString::Printf(TEXT("%s state has no rewards defined"), *Row.GetStateTypeString())));
	}

	return Issues;
}

TArray<FQuestValidationIssue> FQuestTableValidator::ValidateQuestStructure(const FString& QuestName, const TArray<const FQuestTableRow*>& QuestRows)
{
	TArray<FQuestValidationIssue> Issues;

	if (QuestRows.Num() == 0)
	{
		Issues.Add(FQuestValidationIssue(
			EQuestValidationSeverity::Error,
			QuestName,
			TEXT(""),
			TEXT(""),
			TEXT("Quest has no states")));
		return Issues;
	}

	// Check for Start state
	bool bHasStart = false;
	bool bHasSuccess = false;
	bool bHasFailure = false;

	for (const FQuestTableRow* Row : QuestRows)
	{
		if (Row->StateID.Equals(TEXT("Start"), ESearchCase::IgnoreCase))
		{
			bHasStart = true;
		}
		if (Row->StateType == EQuestStateType::Success)
		{
			bHasSuccess = true;
		}
		if (Row->StateType == EQuestStateType::Failure)
		{
			bHasFailure = true;
		}
	}

	// Warn if no explicit Start state (will use first state)
	if (!bHasStart)
	{
		Issues.Add(FQuestValidationIssue(
			EQuestValidationSeverity::Warning,
			QuestName,
			TEXT(""),
			TEXT(""),
			TEXT("Quest has no 'Start' state - first state will be used")));
	}

	// Warn if no success state
	if (!bHasSuccess)
	{
		Issues.Add(FQuestValidationIssue(
			EQuestValidationSeverity::Warning,
			QuestName,
			TEXT(""),
			TEXT(""),
			TEXT("Quest has no Success state")));
	}

	return Issues;
}

FQuestValidationResult FQuestTableValidator::ValidateAllAndCache(TArray<FQuestTableRow>& Rows, const FGuid& ListsVersionGuid)
{
	FQuestValidationResult Result;

	for (FQuestTableRow& Row : Rows)
	{
		if (!Row.bDeleted)
		{
			TArray<FQuestValidationIssue> RowIssues = ValidateRowAndCache(Row, Rows, ListsVersionGuid);
			Result.Issues.Append(RowIssues);
		}
	}

	return Result;
}

TArray<FQuestValidationIssue> FQuestTableValidator::ValidateRowAndCache(FQuestTableRow& Row, const TArray<FQuestTableRow>& AllRows, const FGuid& ListsVersionGuid)
{
	// Check if validation is stale
	uint32 CurrentInputHash = ComputeValidationInputHash(Row, ListsVersionGuid);
	if (Row.ValidationInputHash == CurrentInputHash && Row.ValidationState != EValidationState::Unknown)
	{
		// Return cached result (no issues to return, already counted)
		return TArray<FQuestValidationIssue>();
	}

	// Perform validation
	TArray<FQuestValidationIssue> Issues = ValidateRow(Row, AllRows);

	// Update cache
	Row.ValidationInputHash = CurrentInputHash;
	Row.ValidationIssueCount = Issues.Num();

	if (Issues.Num() == 0)
	{
		Row.ValidationState = EValidationState::Valid;
		Row.ValidationSummary = TEXT("");
	}
	else
	{
		bool bHasError = false;
		for (const auto& Issue : Issues)
		{
			if (Issue.Severity == EQuestValidationSeverity::Error)
			{
				bHasError = true;
				break;
			}
		}
		Row.ValidationState = bHasError ? EValidationState::Invalid : EValidationState::Valid;

		// Build summary
		TArray<FString> IssueSummaries;
		for (const auto& Issue : Issues)
		{
			IssueSummaries.Add(Issue.Message);
		}
		Row.ValidationSummary = FString::Join(IssueSummaries, TEXT("; "));
	}

	return Issues;
}

uint32 FQuestTableValidator::ComputeValidationInputHash(const FQuestTableRow& Row, const FGuid& ListsVersionGuid)
{
	uint32 Hash = Row.ComputeEditableFieldsHash();
	Hash = HashCombine(Hash, GetTypeHash(ListsVersionGuid));
	return Hash;
}

bool FQuestTableValidator::IsStateIdUnique(const FString& QuestName, const FString& StateID, const FGuid& RowId, const TArray<FQuestTableRow>& AllRows)
{
	for (const FQuestTableRow& Row : AllRows)
	{
		if (Row.RowId != RowId && !Row.bDeleted &&
			Row.QuestName == QuestName && Row.StateID == StateID)
		{
			return false;
		}
	}
	return true;
}

TArray<FQuestValidationIssue> FQuestTableValidator::ValidateTasksToken(const FQuestTableRow& Row)
{
	TArray<FQuestValidationIssue> Issues;
	if (Row.Tasks.IsEmpty()) return Issues;

	// Basic token syntax validation
	// Expected: BPT_TaskName(Param=Value);BPT_TaskName2(...)
	TArray<FString> Tokens;
	Row.Tasks.ParseIntoArray(Tokens, TEXT(";"), true);

	for (const FString& Token : Tokens)
	{
		FString Trimmed = Token.TrimStartAndEnd();
		if (Trimmed.IsEmpty()) continue;

		// Check for BPT_ prefix
		if (!Trimmed.StartsWith(TEXT("BPT_")))
		{
			Issues.Add(FQuestValidationIssue(
				EQuestValidationSeverity::Warning,
				Row.QuestName,
				Row.StateID,
				TEXT("Tasks"),
				FString::Printf(TEXT("Task '%s' should start with BPT_ prefix"), *Trimmed)));
		}

		// Check parentheses balance
		int32 OpenCount = 0, CloseCount = 0;
		for (TCHAR C : Trimmed)
		{
			if (C == TEXT('(')) OpenCount++;
			if (C == TEXT(')')) CloseCount++;
		}
		if (OpenCount != CloseCount)
		{
			Issues.Add(FQuestValidationIssue(
				EQuestValidationSeverity::Error,
				Row.QuestName,
				Row.StateID,
				TEXT("Tasks"),
				FString::Printf(TEXT("Unbalanced parentheses in task '%s'"), *Trimmed)));
		}
	}

	return Issues;
}

TArray<FQuestValidationIssue> FQuestTableValidator::ValidateEventsToken(const FQuestTableRow& Row)
{
	TArray<FQuestValidationIssue> Issues;
	if (Row.Events.IsEmpty()) return Issues;

	TArray<FString> Tokens;
	Row.Events.ParseIntoArray(Tokens, TEXT(";"), true);

	for (const FString& Token : Tokens)
	{
		FString Trimmed = Token.TrimStartAndEnd();
		if (Trimmed.IsEmpty()) continue;

		// Check for NE_ or BPE_ prefix
		if (!Trimmed.StartsWith(TEXT("NE_")) && !Trimmed.StartsWith(TEXT("BPE_")))
		{
			Issues.Add(FQuestValidationIssue(
				EQuestValidationSeverity::Warning,
				Row.QuestName,
				Row.StateID,
				TEXT("Events"),
				FString::Printf(TEXT("Event '%s' should start with NE_ or BPE_ prefix"), *Trimmed)));
		}
	}

	return Issues;
}

TArray<FQuestValidationIssue> FQuestTableValidator::ValidateConditionsToken(const FQuestTableRow& Row)
{
	TArray<FQuestValidationIssue> Issues;
	if (Row.Conditions.IsEmpty()) return Issues;

	TArray<FString> Tokens;
	Row.Conditions.ParseIntoArray(Tokens, TEXT(";"), true);

	for (const FString& Token : Tokens)
	{
		FString Trimmed = Token.TrimStartAndEnd();
		if (Trimmed.IsEmpty()) continue;

		// Remove NOT prefix for validation
		if (Trimmed.StartsWith(TEXT("!")))
		{
			Trimmed = Trimmed.Mid(1);
		}

		// Check for NC_ prefix
		if (!Trimmed.StartsWith(TEXT("NC_")))
		{
			Issues.Add(FQuestValidationIssue(
				EQuestValidationSeverity::Warning,
				Row.QuestName,
				Row.StateID,
				TEXT("Conditions"),
				FString::Printf(TEXT("Condition '%s' should start with NC_ prefix"), *Trimmed)));
		}
	}

	return Issues;
}

TArray<FQuestValidationIssue> FQuestTableValidator::ValidateRewardsToken(const FQuestTableRow& Row)
{
	TArray<FQuestValidationIssue> Issues;
	if (Row.Rewards.IsEmpty()) return Issues;

	// Check for Reward( prefix
	if (!Row.Rewards.StartsWith(TEXT("Reward(")))
	{
		Issues.Add(FQuestValidationIssue(
			EQuestValidationSeverity::Warning,
			Row.QuestName,
			Row.StateID,
			TEXT("Rewards"),
			TEXT("Rewards should use format: Reward(Currency=X,XP=Y,Items=...)")));
	}

	// Check parentheses
	if (!Row.Rewards.Contains(TEXT("(")) || !Row.Rewards.Contains(TEXT(")")))
	{
		Issues.Add(FQuestValidationIssue(
			EQuestValidationSeverity::Error,
			Row.QuestName,
			Row.StateID,
			TEXT("Rewards"),
			TEXT("Invalid rewards format - missing parentheses")));
	}

	return Issues;
}

bool FQuestTableValidator::IsParentBranchValid(const FString& QuestName, const FString& ParentBranch, const TArray<FQuestTableRow>& AllRows)
{
	// ParentBranch should reference an existing StateID in the same quest
	for (const FQuestTableRow& Row : AllRows)
	{
		if (!Row.bDeleted && Row.QuestName == QuestName && Row.StateID == ParentBranch)
		{
			return true;
		}
	}
	return false;
}
