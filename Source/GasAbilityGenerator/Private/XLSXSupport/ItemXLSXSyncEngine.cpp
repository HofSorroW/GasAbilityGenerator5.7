// GasAbilityGenerator - Item XLSX Sync Engine Implementation
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

#include "XLSXSupport/ItemXLSXSyncEngine.h"
#include "ItemTableEditor/ItemTableValidator.h"

//=============================================================================
// FItemSyncEntry Implementation
//=============================================================================

FString FItemSyncEntry::GetStatusText() const
{
	switch (Status)
	{
		case EItemSyncStatus::Unchanged:        return TEXT("Unchanged");
		case EItemSyncStatus::ModifiedInUE:     return TEXT("Modified in UE");
		case EItemSyncStatus::ModifiedInExcel:  return TEXT("Modified in Excel");
		case EItemSyncStatus::Conflict:         return TEXT("CONFLICT");
		case EItemSyncStatus::AddedInUE:        return TEXT("Added in UE");
		case EItemSyncStatus::AddedInExcel:     return TEXT("Added in Excel");
		case EItemSyncStatus::DeletedInUE:      return TEXT("Deleted in UE");
		case EItemSyncStatus::DeletedInExcel:   return TEXT("Deleted in Excel");
		case EItemSyncStatus::DeleteConflict:   return TEXT("DELETE CONFLICT");
		default:                                return TEXT("Unknown");
	}
}

FLinearColor FItemSyncEntry::GetStatusColor() const
{
	switch (Status)
	{
		case EItemSyncStatus::Unchanged:        return FLinearColor(0.5f, 0.5f, 0.5f);  // Gray
		case EItemSyncStatus::ModifiedInUE:     return FLinearColor(0.2f, 0.6f, 1.0f);  // Blue
		case EItemSyncStatus::ModifiedInExcel:  return FLinearColor(0.2f, 0.8f, 0.2f);  // Green
		case EItemSyncStatus::Conflict:         return FLinearColor(1.0f, 0.2f, 0.2f);  // Red
		case EItemSyncStatus::AddedInUE:        return FLinearColor(0.2f, 0.6f, 1.0f);  // Blue
		case EItemSyncStatus::AddedInExcel:     return FLinearColor(0.2f, 0.8f, 0.2f);  // Green
		case EItemSyncStatus::DeletedInUE:      return FLinearColor(1.0f, 0.6f, 0.2f);  // Orange
		case EItemSyncStatus::DeletedInExcel:   return FLinearColor(1.0f, 0.6f, 0.2f);  // Orange
		case EItemSyncStatus::DeleteConflict:   return FLinearColor(1.0f, 0.2f, 0.2f);  // Red
		default:                                return FLinearColor::White;
	}
}

bool FItemSyncEntry::RequiresResolution() const
{
	// v4.11: All statuses except Unchanged require user resolution
	// No more auto-accepting ModifiedInUE, ModifiedInExcel, Added*, Deleted*, etc.
	return Status != EItemSyncStatus::Unchanged;
}

FString FItemSyncEntry::GetDisplayName() const
{
	// Return Item name from whichever version exists
	if (UERow.IsValid()) return UERow->ItemName;
	if (ExcelRow.IsValid()) return ExcelRow->ItemName;
	if (BaseRow.IsValid()) return BaseRow->ItemName;
	return TEXT("(Unknown)");
}

//=============================================================================
// FItemSyncResult Implementation
//=============================================================================

bool FItemSyncResult::HasChanges() const
{
	return ModifiedInUECount > 0 ||
		   ModifiedInExcelCount > 0 ||
		   ConflictCount > 0 ||
		   AddedInUECount > 0 ||
		   AddedInExcelCount > 0 ||
		   DeletedCount > 0;
}

int32 FItemSyncResult::GetTotalChanges() const
{
	return ModifiedInUECount +
		   ModifiedInExcelCount +
		   ConflictCount +
		   AddedInUECount +
		   AddedInExcelCount +
		   DeletedCount;
}

TArray<FItemSyncEntry*> FItemSyncResult::GetConflicts()
{
	TArray<FItemSyncEntry*> Result;
	for (FItemSyncEntry& Entry : Entries)
	{
		if (Entry.Status == EItemSyncStatus::Conflict)
		{
			Result.Add(&Entry);
		}
	}
	return Result;
}

TArray<FItemSyncEntry*> FItemSyncResult::GetEntriesRequiringResolution()
{
	TArray<FItemSyncEntry*> Result;
	for (FItemSyncEntry& Entry : Entries)
	{
		if (Entry.RequiresResolution())
		{
			Result.Add(&Entry);
		}
	}
	return Result;
}

//=============================================================================
// FItemXLSXSyncEngine Implementation
//=============================================================================

FItemSyncResult FItemXLSXSyncEngine::CompareSources(
	const TArray<FItemTableRow>& BaseRows,
	const TArray<FItemTableRow>& UERows,
	const TArray<FItemTableRow>& ExcelRows)
{
	FItemSyncResult Result;
	Result.bSuccess = true;

	// Build lookup maps for O(1) access by RowId
	TMap<FGuid, const FItemTableRow*> BaseMap = BuildRowMap(BaseRows);
	TMap<FGuid, const FItemTableRow*> UEMap = BuildRowMap(UERows);
	TMap<FGuid, const FItemTableRow*> ExcelMap = BuildRowMap(ExcelRows);

	// Collect all unique RowIds from all three sources
	TSet<FGuid> AllRowIds;
	for (const FItemTableRow& Row : BaseRows) AllRowIds.Add(Row.RowId);
	for (const FItemTableRow& Row : UERows) AllRowIds.Add(Row.RowId);
	for (const FItemTableRow& Row : ExcelRows) AllRowIds.Add(Row.RowId);

	// Process each unique RowId
	for (const FGuid& RowId : AllRowIds)
	{
		const FItemTableRow* BaseRow = BaseMap.FindRef(RowId);
		const FItemTableRow* UERow = UEMap.FindRef(RowId);
		const FItemTableRow* ExcelRow = ExcelMap.FindRef(RowId);

		// Compute hashes (0 if row doesn't exist)
		int64 BaseHash = BaseRow ? ComputeRowHash(*BaseRow) : 0;
		int64 UEHash = UERow ? ComputeRowHash(*UERow) : 0;
		int64 ExcelHash = ExcelRow ? ComputeRowHash(*ExcelRow) : 0;

		// Create sync entry
		FItemSyncEntry Entry;
		Entry.RowId = RowId;
		Entry.BaseHash = BaseHash;
		Entry.UEHash = UEHash;
		Entry.ExcelHash = ExcelHash;

		// Store row copies (as shared pointers for safe reference)
		if (BaseRow) Entry.BaseRow = MakeShared<FItemTableRow>(*BaseRow);
		if (UERow) Entry.UERow = MakeShared<FItemTableRow>(*UERow);
		if (ExcelRow) Entry.ExcelRow = MakeShared<FItemTableRow>(*ExcelRow);

		// Determine sync status
		Entry.Status = DetermineStatus(BaseRow, UERow, ExcelRow, BaseHash, UEHash, ExcelHash);

		// Update statistics
		switch (Entry.Status)
		{
			case EItemSyncStatus::Unchanged:        Result.UnchangedCount++; break;
			case EItemSyncStatus::ModifiedInUE:     Result.ModifiedInUECount++; break;
			case EItemSyncStatus::ModifiedInExcel:  Result.ModifiedInExcelCount++; break;
			case EItemSyncStatus::Conflict:         Result.ConflictCount++; break;
			case EItemSyncStatus::AddedInUE:        Result.AddedInUECount++; break;
			case EItemSyncStatus::AddedInExcel:     Result.AddedInExcelCount++; break;
			case EItemSyncStatus::DeletedInUE:      Result.DeletedCount++; break;
			case EItemSyncStatus::DeletedInExcel:   Result.DeletedCount++; break;
			case EItemSyncStatus::DeleteConflict:   Result.ConflictCount++; break;
		}

		Result.Entries.Add(Entry);
	}

	//-------------------------------------------------------------------------
	// v4.12.2: Validate Excel rows and populate validation status
	//-------------------------------------------------------------------------

	// Validate all Excel rows
	FItemValidationResult ValidationResult = FItemTableValidator::ValidateAll(ExcelRows);

	// Build map of ItemName -> validation issues for quick lookup
	TMap<FString, TArray<FItemValidationIssue>> IssuesByItemName;
	for (const FItemValidationIssue& Issue : ValidationResult.Issues)
	{
		IssuesByItemName.FindOrAdd(Issue.ItemName).Add(Issue);
	}

	// Apply validation results to sync entries
	for (FItemSyncEntry& Entry : Result.Entries)
	{
		// Only validate entries that have Excel data (the source being imported)
		if (!Entry.ExcelRow.IsValid())
		{
			continue;
		}

		const FString& ItemName = Entry.ExcelRow->ItemName;
		if (TArray<FItemValidationIssue>* Issues = IssuesByItemName.Find(ItemName))
		{
			bool bHasError = false;
			bool bHasWarning = false;

			for (const FItemValidationIssue& Issue : *Issues)
			{
				Entry.ValidationMessages.Add(Issue.ToString());

				if (Issue.Severity == EItemValidationSeverity::Error)
				{
					bHasError = true;
					Result.ValidationErrorCount++;
				}
				else if (Issue.Severity == EItemValidationSeverity::Warning)
				{
					bHasWarning = true;
					Result.ValidationWarningCount++;
				}
			}

			// Set status based on worst severity
			if (bHasError)
			{
				Entry.ValidationStatus = EItemSyncValidationStatus::Error;
			}
			else if (bHasWarning)
			{
				Entry.ValidationStatus = EItemSyncValidationStatus::Warning;
			}
		}
	}

	// Sort entries by Item name for consistent display
	Result.Entries.Sort([](const FItemSyncEntry& A, const FItemSyncEntry& B)
	{
		return A.GetDisplayName() < B.GetDisplayName();
	});

	return Result;
}

FItemMergeResult FItemXLSXSyncEngine::ApplySync(const FItemSyncResult& SyncResult)
{
	FItemMergeResult Result;

	for (const FItemSyncEntry& Entry : SyncResult.Entries)
	{
		bool bShouldInclude = false;
		FItemTableRow RowToInclude;

		switch (Entry.Status)
		{
			case EItemSyncStatus::Unchanged:
				// Keep UE version (they're identical anyway)
				if (Entry.UERow.IsValid())
				{
					RowToInclude = *Entry.UERow;
					RowToInclude.Status = EItemTableRowStatus::Synced;
					bShouldInclude = true;
					Result.Unchanged++;
				}
				break;

			case EItemSyncStatus::ModifiedInUE:
				// Keep UE changes
				if (Entry.UERow.IsValid())
				{
					RowToInclude = *Entry.UERow;
					RowToInclude.Status = EItemTableRowStatus::Modified;
					bShouldInclude = true;
					Result.AppliedFromUE++;
				}
				break;

			case EItemSyncStatus::ModifiedInExcel:
				// Apply Excel changes
				if (Entry.ExcelRow.IsValid())
				{
					RowToInclude = *Entry.ExcelRow;
					RowToInclude.Status = EItemTableRowStatus::Modified;
					bShouldInclude = true;
					Result.AppliedFromExcel++;
				}
				break;

			case EItemSyncStatus::AddedInUE:
				// Include new UE row
				if (Entry.UERow.IsValid())
				{
					RowToInclude = *Entry.UERow;
					RowToInclude.Status = EItemTableRowStatus::New;
					bShouldInclude = true;
					Result.AppliedFromUE++;
				}
				break;

			case EItemSyncStatus::AddedInExcel:
				// Include new Excel row
				if (Entry.ExcelRow.IsValid())
				{
					RowToInclude = *Entry.ExcelRow;
					RowToInclude.Status = EItemTableRowStatus::New;
					bShouldInclude = true;
					Result.AppliedFromExcel++;
				}
				break;

			case EItemSyncStatus::DeletedInUE:
			case EItemSyncStatus::DeletedInExcel:
				// Don't include - row is deleted
				Result.Deleted++;
				break;

			case EItemSyncStatus::Conflict:
			case EItemSyncStatus::DeleteConflict:
				// Apply user's resolution
				switch (Entry.Resolution)
				{
					case EItemConflictResolution::KeepUE:
						if (Entry.UERow.IsValid())
						{
							RowToInclude = *Entry.UERow;
							RowToInclude.Status = EItemTableRowStatus::Modified;
							bShouldInclude = true;
							Result.AppliedFromUE++;
						}
						break;

					case EItemConflictResolution::KeepExcel:
						if (Entry.ExcelRow.IsValid())
						{
							RowToInclude = *Entry.ExcelRow;
							RowToInclude.Status = EItemTableRowStatus::Modified;
							bShouldInclude = true;
							Result.AppliedFromExcel++;
						}
						break;

					case EItemConflictResolution::KeepBoth:
						// Add both with new GUIDs for the duplicates
						if (Entry.UERow.IsValid())
						{
							FItemTableRow UECopy = *Entry.UERow;
							UECopy.Status = EItemTableRowStatus::Modified;
							Result.MergedRows.Add(UECopy);
							Result.AppliedFromUE++;
						}
						if (Entry.ExcelRow.IsValid())
						{
							FItemTableRow ExcelCopy = *Entry.ExcelRow;
							ExcelCopy.RowId = FGuid::NewGuid();  // New GUID to avoid collision
							ExcelCopy.ItemName += TEXT("_Excel");  // Disambiguate name
							ExcelCopy.Status = EItemTableRowStatus::New;
							Result.MergedRows.Add(ExcelCopy);
							Result.AppliedFromExcel++;
						}
						break;

					case EItemConflictResolution::Delete:
						Result.Deleted++;
						break;

					default:
						// Unresolved - should not happen if AllConflictsResolved() was checked
						// Default to keeping UE version
						if (Entry.UERow.IsValid())
						{
							RowToInclude = *Entry.UERow;
							RowToInclude.Status = EItemTableRowStatus::Modified;
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

void FItemXLSXSyncEngine::AutoResolveNonConflicts(FItemSyncResult& SyncResult)
{
	// v4.11: Only Unchanged auto-resolves. All other statuses require explicit user approval.
	// This prevents accidental acceptance of changes the user might want to review.
	for (FItemSyncEntry& Entry : SyncResult.Entries)
	{
		if (Entry.Status == EItemSyncStatus::Unchanged)
		{
			Entry.Resolution = EItemConflictResolution::KeepUE;  // Either is fine, content is identical
		}
		// All other statuses remain Unresolved, requiring user selection in approval UI
	}
}

bool FItemXLSXSyncEngine::AllConflictsResolved(const FItemSyncResult& SyncResult)
{
	for (const FItemSyncEntry& Entry : SyncResult.Entries)
	{
		if (Entry.RequiresResolution() && Entry.Resolution == EItemConflictResolution::Unresolved)
		{
			return false;
		}
	}
	return true;
}

int64 FItemXLSXSyncEngine::ComputeRowHash(const FItemTableRow& Row)
{
	// Hash all content fields (same as writer for consistency)
	// Excludes RowId which is for identity tracking, not content comparison
	int64 Hash = 0;

	// Core Identity
	Hash ^= GetTypeHash(Row.ItemName);
	Hash ^= GetTypeHash(Row.DisplayName);
	Hash ^= GetTypeHash(static_cast<uint8>(Row.ItemType));
	Hash ^= GetTypeHash(Row.EquipmentSlot);
	Hash ^= GetTypeHash(Row.BaseValue);
	Hash ^= GetTypeHash(Row.Weight);

	// Description
	Hash ^= GetTypeHash(Row.Description);
	Hash ^= GetTypeHash(Row.BaseScore);

	// Combat
	Hash ^= GetTypeHash(Row.AttackRating);
	Hash ^= GetTypeHash(Row.ArmorRating);
	Hash ^= GetTypeHash(Row.StealthRating);

	// Weapon
	Hash ^= GetTypeHash(Row.AttackDamage);
	Hash ^= GetTypeHash(Row.HeavyAttackDamageMultiplier);
	Hash ^= GetTypeHash(Row.WeaponHand);
	Hash ^= GetTypeHash(Row.ClipSize);
	Hash ^= GetTypeHash(Row.RequiredAmmo);
	Hash ^= GetTypeHash(Row.bAllowManualReload);
	Hash ^= GetTypeHash(Row.BotAttackRange);

	// Ranged
	Hash ^= GetTypeHash(Row.BaseSpreadDegrees);
	Hash ^= GetTypeHash(Row.MaxSpreadDegrees);
	Hash ^= GetTypeHash(Row.SpreadFireBump);
	Hash ^= GetTypeHash(Row.SpreadDecreaseSpeed);
	Hash ^= GetTypeHash(Row.AimFOVPct);

	// Consumable
	Hash ^= GetTypeHash(Row.bConsumeOnUse);
	Hash ^= GetTypeHash(Row.UseRechargeDuration);
	Hash ^= GetTypeHash(Row.bCanActivate);
	Hash ^= GetTypeHash(Row.GameplayEffectClass);

	// References
	Hash ^= GetTypeHash(Row.ModifierGE.ToString());
	Hash ^= GetTypeHash(Row.Abilities);
	Hash ^= GetTypeHash(Row.Fragments);
	Hash ^= GetTypeHash(Row.ItemTags);
	Hash ^= GetTypeHash(Row.bStackable);
	Hash ^= GetTypeHash(Row.MaxStackSize);

	// Meta
	Hash ^= GetTypeHash(Row.Notes);
	Hash ^= GetTypeHash(Row.bDeleted);

	return Hash;
}

// SAFETY: Returns map with pointers into the source Rows array. Caller MUST ensure
// the Rows array remains valid and unmodified while using the returned map.
// Used for O(1) lookup during 3-way merge comparison within CompareSources().
TMap<FGuid, const FItemTableRow*> FItemXLSXSyncEngine::BuildRowMap(const TArray<FItemTableRow>& Rows)
{
	TMap<FGuid, const FItemTableRow*> Map;
	for (const FItemTableRow& Row : Rows)
	{
		Map.Add(Row.RowId, &Row);
	}
	return Map;
}

EItemSyncStatus FItemXLSXSyncEngine::DetermineStatus(
	const FItemTableRow* BaseRow,
	const FItemTableRow* UERow,
	const FItemTableRow* ExcelRow,
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
		return EItemSyncStatus::AddedInUE;
	}

	if (!bInBase && !bInUE && bInExcel)
	{
		// Only in Excel - added in spreadsheet
		return EItemSyncStatus::AddedInExcel;
	}

	if (bInBase && !bInUE && bInExcel)
	{
		// In Base and Excel but not UE - deleted locally
		return EItemSyncStatus::DeletedInUE;
	}

	if (bInBase && bInUE && !bInExcel)
	{
		// In Base and UE but not Excel - deleted in spreadsheet
		// Note: This could also be detected via #STATE=Deleted column
		return EItemSyncStatus::DeletedInExcel;
	}

	if (!bInBase && bInUE && bInExcel)
	{
		// In both UE and Excel but not Base - added in both
		// Check if they're the same (same content added independently)
		if (UEHash == ExcelHash)
		{
			return EItemSyncStatus::Unchanged;  // Same addition in both
		}
		else
		{
			return EItemSyncStatus::Conflict;  // Different additions with same GUID (rare)
		}
	}

	if (bInBase && !bInUE && !bInExcel)
	{
		// Was in Base but deleted in both - already deleted, no action
		return EItemSyncStatus::DeletedInUE;  // Treat as already handled
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
			return EItemSyncStatus::Unchanged;
		}

		if (BaseHash == UEHash && BaseHash != ExcelHash)
		{
			// Excel changed, UE unchanged - apply Excel
			return EItemSyncStatus::ModifiedInExcel;
		}

		if (BaseHash != UEHash && BaseHash == ExcelHash)
		{
			// UE changed, Excel unchanged - keep UE
			return EItemSyncStatus::ModifiedInUE;
		}

		if (BaseHash != UEHash && BaseHash != ExcelHash)
		{
			// Both changed - check if they made the same changes
			if (UEHash == ExcelHash)
			{
				// Same changes made in both - no conflict
				return EItemSyncStatus::Unchanged;  // Converged
			}
			else
			{
				// Different changes - conflict
				return EItemSyncStatus::Conflict;
			}
		}
	}

	// Fallback - should not reach here with valid input
	return EItemSyncStatus::Unchanged;
}
