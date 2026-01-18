// GasAbilityGenerator - Quest XLSX Sync Engine Implementation
// v4.12: 3-way merge for Excel ↔ UE synchronization
// v4.11: Only Unchanged auto-resolves, all others require user approval
//
// Implements a 3-way merge algorithm similar to git:
// - Base = snapshot at last export
// - UE = current state in editor
// - Excel = imported changes
//
// Merge rules:
// - Base == UE == Excel: Unchanged (no action)
// - Base == UE, Excel differs: Apply Excel changes
// - Base == Excel, UE differs: Keep UE changes
// - All three differ: Conflict (user must resolve)

#include "XLSXSupport/QuestXLSXSyncEngine.h"
#include "QuestTableEditor/QuestTableValidator.h"

//=============================================================================
// FQuestSyncEntry Implementation
//=============================================================================

FString FQuestSyncEntry::GetStatusText() const
{
	switch (Status)
	{
		case EQuestSyncStatus::Unchanged:        return TEXT("Unchanged");
		case EQuestSyncStatus::ModifiedInUE:     return TEXT("Modified in UE");
		case EQuestSyncStatus::ModifiedInExcel:  return TEXT("Modified in Excel");
		case EQuestSyncStatus::Conflict:         return TEXT("CONFLICT");
		case EQuestSyncStatus::AddedInUE:        return TEXT("Added in UE");
		case EQuestSyncStatus::AddedInExcel:     return TEXT("Added in Excel");
		case EQuestSyncStatus::DeletedInUE:      return TEXT("Deleted in UE");
		case EQuestSyncStatus::DeletedInExcel:   return TEXT("Deleted in Excel");
		case EQuestSyncStatus::DeleteConflict:   return TEXT("DELETE CONFLICT");
		default:                                 return TEXT("Unknown");
	}
}

FLinearColor FQuestSyncEntry::GetStatusColor() const
{
	switch (Status)
	{
		case EQuestSyncStatus::Unchanged:        return FLinearColor(0.5f, 0.5f, 0.5f);  // Gray
		case EQuestSyncStatus::ModifiedInUE:     return FLinearColor(0.2f, 0.6f, 1.0f);  // Blue
		case EQuestSyncStatus::ModifiedInExcel:  return FLinearColor(0.2f, 0.8f, 0.2f);  // Green
		case EQuestSyncStatus::Conflict:         return FLinearColor(1.0f, 0.2f, 0.2f);  // Red
		case EQuestSyncStatus::AddedInUE:        return FLinearColor(0.2f, 0.6f, 1.0f);  // Blue
		case EQuestSyncStatus::AddedInExcel:     return FLinearColor(0.2f, 0.8f, 0.2f);  // Green
		case EQuestSyncStatus::DeletedInUE:      return FLinearColor(1.0f, 0.6f, 0.2f);  // Orange
		case EQuestSyncStatus::DeletedInExcel:   return FLinearColor(1.0f, 0.6f, 0.2f);  // Orange
		case EQuestSyncStatus::DeleteConflict:   return FLinearColor(1.0f, 0.2f, 0.2f);  // Red
		default:                                 return FLinearColor::White;
	}
}

bool FQuestSyncEntry::RequiresResolution() const
{
	// v4.11: All statuses except Unchanged require user resolution
	// No more auto-accepting ModifiedInUE, ModifiedInExcel, Added*, Deleted*, etc.
	return Status != EQuestSyncStatus::Unchanged;
}

FString FQuestSyncEntry::GetDisplayName() const
{
	// Return Quest name + State from whichever version exists
	FString QuestName, StateID;
	if (UERow.IsValid())
	{
		QuestName = UERow->QuestName;
		StateID = UERow->StateID;
	}
	else if (ExcelRow.IsValid())
	{
		QuestName = ExcelRow->QuestName;
		StateID = ExcelRow->StateID;
	}
	else if (BaseRow.IsValid())
	{
		QuestName = BaseRow->QuestName;
		StateID = BaseRow->StateID;
	}

	if (!StateID.IsEmpty())
	{
		return FString::Printf(TEXT("%s / %s"), *QuestName, *StateID);
	}
	return QuestName.IsEmpty() ? TEXT("(Unknown)") : QuestName;
}

//=============================================================================
// FQuestSyncResult Implementation
//=============================================================================

bool FQuestSyncResult::HasChanges() const
{
	return ModifiedInUECount > 0 ||
		   ModifiedInExcelCount > 0 ||
		   ConflictCount > 0 ||
		   AddedInUECount > 0 ||
		   AddedInExcelCount > 0 ||
		   DeletedCount > 0;
}

int32 FQuestSyncResult::GetTotalChanges() const
{
	return ModifiedInUECount +
		   ModifiedInExcelCount +
		   ConflictCount +
		   AddedInUECount +
		   AddedInExcelCount +
		   DeletedCount;
}

TArray<FQuestSyncEntry*> FQuestSyncResult::GetConflicts()
{
	TArray<FQuestSyncEntry*> Result;
	for (FQuestSyncEntry& Entry : Entries)
	{
		if (Entry.Status == EQuestSyncStatus::Conflict)
		{
			Result.Add(&Entry);
		}
	}
	return Result;
}

TArray<FQuestSyncEntry*> FQuestSyncResult::GetEntriesRequiringResolution()
{
	TArray<FQuestSyncEntry*> Result;
	for (FQuestSyncEntry& Entry : Entries)
	{
		if (Entry.RequiresResolution())
		{
			Result.Add(&Entry);
		}
	}
	return Result;
}

//=============================================================================
// FQuestXLSXSyncEngine Implementation
//=============================================================================

FQuestSyncResult FQuestXLSXSyncEngine::CompareSources(
	const TArray<FQuestTableRow>& BaseRows,
	const TArray<FQuestTableRow>& UERows,
	const TArray<FQuestTableRow>& ExcelRows)
{
	FQuestSyncResult Result;
	Result.bSuccess = true;

	// Build lookup maps for O(1) access by RowId
	TMap<FGuid, const FQuestTableRow*> BaseMap = BuildRowMap(BaseRows);
	TMap<FGuid, const FQuestTableRow*> UEMap = BuildRowMap(UERows);
	TMap<FGuid, const FQuestTableRow*> ExcelMap = BuildRowMap(ExcelRows);

	// Collect all unique RowIds from all three sources
	TSet<FGuid> AllRowIds;
	for (const FQuestTableRow& Row : BaseRows) AllRowIds.Add(Row.RowId);
	for (const FQuestTableRow& Row : UERows) AllRowIds.Add(Row.RowId);
	for (const FQuestTableRow& Row : ExcelRows) AllRowIds.Add(Row.RowId);

	// Process each unique RowId
	for (const FGuid& RowId : AllRowIds)
	{
		const FQuestTableRow* BaseRow = BaseMap.FindRef(RowId);
		const FQuestTableRow* UERow = UEMap.FindRef(RowId);
		const FQuestTableRow* ExcelRow = ExcelMap.FindRef(RowId);

		// Compute hashes (0 if row doesn't exist)
		int64 BaseHash = BaseRow ? ComputeRowHash(*BaseRow) : 0;
		int64 UEHash = UERow ? ComputeRowHash(*UERow) : 0;
		int64 ExcelHash = ExcelRow ? ComputeRowHash(*ExcelRow) : 0;

		// Create sync entry
		FQuestSyncEntry Entry;
		Entry.RowId = RowId;
		Entry.BaseHash = BaseHash;
		Entry.UEHash = UEHash;
		Entry.ExcelHash = ExcelHash;

		// Store row copies (as shared pointers for safe reference)
		if (BaseRow) Entry.BaseRow = MakeShared<FQuestTableRow>(*BaseRow);
		if (UERow) Entry.UERow = MakeShared<FQuestTableRow>(*UERow);
		if (ExcelRow) Entry.ExcelRow = MakeShared<FQuestTableRow>(*ExcelRow);

		// Determine sync status
		Entry.Status = DetermineStatus(BaseRow, UERow, ExcelRow, BaseHash, UEHash, ExcelHash);

		// Update statistics
		switch (Entry.Status)
		{
			case EQuestSyncStatus::Unchanged:        Result.UnchangedCount++; break;
			case EQuestSyncStatus::ModifiedInUE:     Result.ModifiedInUECount++; break;
			case EQuestSyncStatus::ModifiedInExcel:  Result.ModifiedInExcelCount++; break;
			case EQuestSyncStatus::Conflict:         Result.ConflictCount++; break;
			case EQuestSyncStatus::AddedInUE:        Result.AddedInUECount++; break;
			case EQuestSyncStatus::AddedInExcel:     Result.AddedInExcelCount++; break;
			case EQuestSyncStatus::DeletedInUE:      Result.DeletedCount++; break;
			case EQuestSyncStatus::DeletedInExcel:   Result.DeletedCount++; break;
			case EQuestSyncStatus::DeleteConflict:   Result.ConflictCount++; break;
		}

		Result.Entries.Add(Entry);
	}

	//-------------------------------------------------------------------------
	// v4.12.2: Validate Excel rows and populate validation status
	//-------------------------------------------------------------------------

	// Validate all Excel rows
	FQuestValidationResult ValidationResult = FQuestTableValidator::ValidateAll(ExcelRows);

	// Build map of QuestName/StateID -> validation issues for quick lookup
	// Key format: "QuestName|StateID"
	TMap<FString, TArray<FQuestValidationIssue>> IssuesByKey;
	for (const FQuestValidationIssue& Issue : ValidationResult.Issues)
	{
		FString Key = FString::Printf(TEXT("%s|%s"), *Issue.QuestName, *Issue.StateID);
		IssuesByKey.FindOrAdd(Key).Add(Issue);
	}

	// Apply validation results to sync entries
	for (FQuestSyncEntry& Entry : Result.Entries)
	{
		// Only validate entries that have Excel data (the source being imported)
		if (!Entry.ExcelRow.IsValid())
		{
			continue;
		}

		FString Key = FString::Printf(TEXT("%s|%s"), *Entry.ExcelRow->QuestName, *Entry.ExcelRow->StateID);
		if (TArray<FQuestValidationIssue>* Issues = IssuesByKey.Find(Key))
		{
			bool bHasError = false;
			bool bHasWarning = false;

			for (const FQuestValidationIssue& Issue : *Issues)
			{
				Entry.ValidationMessages.Add(Issue.ToString());

				if (Issue.Severity == EQuestValidationSeverity::Error)
				{
					bHasError = true;
					Result.ValidationErrorCount++;
				}
				else if (Issue.Severity == EQuestValidationSeverity::Warning)
				{
					bHasWarning = true;
					Result.ValidationWarningCount++;
				}
			}

			// Set status based on worst severity
			if (bHasError)
			{
				Entry.ValidationStatus = EQuestSyncValidationStatus::Error;
			}
			else if (bHasWarning)
			{
				Entry.ValidationStatus = EQuestSyncValidationStatus::Warning;
			}
		}
	}

	// Sort entries by QuestName + StateID for consistent display
	Result.Entries.Sort([](const FQuestSyncEntry& A, const FQuestSyncEntry& B)
	{
		return A.GetDisplayName() < B.GetDisplayName();
	});

	return Result;
}

FQuestMergeResult FQuestXLSXSyncEngine::ApplySync(const FQuestSyncResult& SyncResult)
{
	FQuestMergeResult Result;

	for (const FQuestSyncEntry& Entry : SyncResult.Entries)
	{
		bool bShouldInclude = false;
		FQuestTableRow RowToInclude;

		switch (Entry.Status)
		{
			case EQuestSyncStatus::Unchanged:
				// Keep UE version (they're identical anyway)
				if (Entry.UERow.IsValid())
				{
					RowToInclude = *Entry.UERow;
					RowToInclude.Status = EQuestTableRowStatus::Synced;
					bShouldInclude = true;
					Result.Unchanged++;
				}
				break;

			case EQuestSyncStatus::ModifiedInUE:
				// Keep UE changes
				if (Entry.UERow.IsValid())
				{
					RowToInclude = *Entry.UERow;
					RowToInclude.Status = EQuestTableRowStatus::Modified;
					bShouldInclude = true;
					Result.AppliedFromUE++;
				}
				break;

			case EQuestSyncStatus::ModifiedInExcel:
				// Apply Excel changes
				if (Entry.ExcelRow.IsValid())
				{
					RowToInclude = *Entry.ExcelRow;
					RowToInclude.Status = EQuestTableRowStatus::Modified;
					bShouldInclude = true;
					Result.AppliedFromExcel++;
				}
				break;

			case EQuestSyncStatus::AddedInUE:
				// Include new UE row
				if (Entry.UERow.IsValid())
				{
					RowToInclude = *Entry.UERow;
					RowToInclude.Status = EQuestTableRowStatus::New;
					bShouldInclude = true;
					Result.AppliedFromUE++;
				}
				break;

			case EQuestSyncStatus::AddedInExcel:
				// Include new Excel row
				if (Entry.ExcelRow.IsValid())
				{
					RowToInclude = *Entry.ExcelRow;
					RowToInclude.Status = EQuestTableRowStatus::New;
					bShouldInclude = true;
					Result.AppliedFromExcel++;
				}
				break;

			case EQuestSyncStatus::DeletedInUE:
			case EQuestSyncStatus::DeletedInExcel:
				// Don't include - row is deleted
				Result.Deleted++;
				break;

			case EQuestSyncStatus::Conflict:
			case EQuestSyncStatus::DeleteConflict:
				// Apply user's resolution
				switch (Entry.Resolution)
				{
					case EQuestConflictResolution::KeepUE:
						if (Entry.UERow.IsValid())
						{
							RowToInclude = *Entry.UERow;
							RowToInclude.Status = EQuestTableRowStatus::Modified;
							bShouldInclude = true;
							Result.AppliedFromUE++;
						}
						break;

					case EQuestConflictResolution::KeepExcel:
						if (Entry.ExcelRow.IsValid())
						{
							RowToInclude = *Entry.ExcelRow;
							RowToInclude.Status = EQuestTableRowStatus::Modified;
							bShouldInclude = true;
							Result.AppliedFromExcel++;
						}
						break;

					case EQuestConflictResolution::KeepBoth:
						// Add both with new GUIDs for the duplicates
						if (Entry.UERow.IsValid())
						{
							FQuestTableRow UECopy = *Entry.UERow;
							UECopy.Status = EQuestTableRowStatus::Modified;
							Result.MergedRows.Add(UECopy);
							Result.AppliedFromUE++;
						}
						if (Entry.ExcelRow.IsValid())
						{
							FQuestTableRow ExcelCopy = *Entry.ExcelRow;
							ExcelCopy.RowId = FGuid::NewGuid();  // New GUID to avoid collision
							ExcelCopy.StateID += TEXT("_Excel");  // Disambiguate name
							ExcelCopy.Status = EQuestTableRowStatus::New;
							Result.MergedRows.Add(ExcelCopy);
							Result.AppliedFromExcel++;
						}
						break;

					case EQuestConflictResolution::Delete:
						Result.Deleted++;
						break;

					default:
						// Unresolved - should not happen if AllConflictsResolved() was checked
						// Default to keeping UE version
						if (Entry.UERow.IsValid())
						{
							RowToInclude = *Entry.UERow;
							RowToInclude.Status = EQuestTableRowStatus::Modified;
							bShouldInclude = true;
						}
						break;
				}
				break;
		}

		if (bShouldInclude)
		{
			Result.MergedRows.Add(RowToInclude);
		}
	}

	return Result;
}

void FQuestXLSXSyncEngine::AutoResolveNonConflicts(FQuestSyncResult& SyncResult)
{
	// v4.11: Only Unchanged auto-resolves. All other statuses require explicit user approval.
	// This prevents accidental acceptance of changes the user might want to review.
	for (FQuestSyncEntry& Entry : SyncResult.Entries)
	{
		if (Entry.Status == EQuestSyncStatus::Unchanged)
		{
			Entry.Resolution = EQuestConflictResolution::KeepUE;  // Either is fine, content is identical
		}
		// All other statuses remain Unresolved, requiring user selection in approval UI
	}
}

bool FQuestXLSXSyncEngine::AllConflictsResolved(const FQuestSyncResult& SyncResult)
{
	for (const FQuestSyncEntry& Entry : SyncResult.Entries)
	{
		if (Entry.RequiresResolution() && Entry.Resolution == EQuestConflictResolution::Unresolved)
		{
			return false;
		}
	}
	return true;
}

int64 FQuestXLSXSyncEngine::ComputeRowHash(const FQuestTableRow& Row)
{
	// Hash all content fields (same as writer for consistency)
	// Excludes RowId which is for identity tracking, not content comparison
	int64 Hash = 0;

	// Core Identity
	Hash ^= GetTypeHash(Row.QuestName);
	Hash ^= GetTypeHash(Row.DisplayName);

	// State Definition
	Hash ^= GetTypeHash(Row.StateID);
	Hash ^= GetTypeHash(static_cast<uint8>(Row.StateType));
	Hash ^= GetTypeHash(Row.Description);
	Hash ^= GetTypeHash(Row.ParentBranch);

	// Logic (token-based)
	Hash ^= GetTypeHash(Row.Tasks);
	Hash ^= GetTypeHash(Row.Events);
	Hash ^= GetTypeHash(Row.Conditions);

	// Rewards & Meta
	Hash ^= GetTypeHash(Row.Rewards);
	Hash ^= GetTypeHash(Row.Notes);

	// Soft delete
	Hash ^= GetTypeHash(Row.bDeleted);

	return Hash;
}

TMap<FGuid, const FQuestTableRow*> FQuestXLSXSyncEngine::BuildRowMap(const TArray<FQuestTableRow>& Rows)
{
	TMap<FGuid, const FQuestTableRow*> Map;
	for (const FQuestTableRow& Row : Rows)
	{
		Map.Add(Row.RowId, &Row);
	}
	return Map;
}

EQuestSyncStatus FQuestXLSXSyncEngine::DetermineStatus(
	const FQuestTableRow* BaseRow,
	const FQuestTableRow* UERow,
	const FQuestTableRow* ExcelRow,
	int64 BaseHash, int64 UEHash, int64 ExcelHash)
{
	const bool bInBase = (BaseRow != nullptr);
	const bool bInUE = (UERow != nullptr);
	const bool bInExcel = (ExcelRow != nullptr);

	//-------------------------------------------------------------------------
	// Existence-based status (row present in some sources but not others)
	//-------------------------------------------------------------------------

	if (!bInBase && bInUE && !bInExcel)
	{
		// Only in UE - added locally
		return EQuestSyncStatus::AddedInUE;
	}

	if (!bInBase && !bInUE && bInExcel)
	{
		// Only in Excel - added in spreadsheet
		return EQuestSyncStatus::AddedInExcel;
	}

	if (bInBase && !bInUE && bInExcel)
	{
		// In Base and Excel but not UE - deleted locally
		return EQuestSyncStatus::DeletedInUE;
	}

	if (bInBase && bInUE && !bInExcel)
	{
		// In Base and UE but not Excel - deleted in spreadsheet
		// Note: This could also be detected via #STATE=Deleted column
		return EQuestSyncStatus::DeletedInExcel;
	}

	if (!bInBase && bInUE && bInExcel)
	{
		// In both UE and Excel but not Base - added in both
		// Check if they're the same (same content added independently)
		if (UEHash == ExcelHash)
		{
			return EQuestSyncStatus::Unchanged;  // Same addition in both
		}
		else
		{
			return EQuestSyncStatus::Conflict;  // Different additions with same GUID (rare)
		}
	}

	if (bInBase && !bInUE && !bInExcel)
	{
		// Was in Base but deleted in both - already deleted, no action
		return EQuestSyncStatus::DeletedInUE;  // Treat as already handled
	}

	//-------------------------------------------------------------------------
	// Hash-based status (row exists in all relevant sources)
	//-------------------------------------------------------------------------

	if (bInBase && bInUE && bInExcel)
	{
		// Row exists in all three sources - compare hashes

		if (BaseHash == UEHash && BaseHash == ExcelHash)
		{
			// All identical - no changes
			return EQuestSyncStatus::Unchanged;
		}

		if (BaseHash == UEHash && BaseHash != ExcelHash)
		{
			// Excel changed, UE unchanged - apply Excel
			return EQuestSyncStatus::ModifiedInExcel;
		}

		if (BaseHash != UEHash && BaseHash == ExcelHash)
		{
			// UE changed, Excel unchanged - keep UE
			return EQuestSyncStatus::ModifiedInUE;
		}

		if (BaseHash != UEHash && BaseHash != ExcelHash)
		{
			// Both changed - check if they made the same changes
			if (UEHash == ExcelHash)
			{
				// Same changes made in both - no conflict
				return EQuestSyncStatus::Unchanged;  // Converged
			}
			else
			{
				// Different changes - conflict
				return EQuestSyncStatus::Conflict;
			}
		}
	}

	// Fallback - should not reach here with valid input
	return EQuestSyncStatus::Unchanged;
}
