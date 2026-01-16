// GasAbilityGenerator - NPC XLSX Sync Engine Implementation
// v4.4: 3-way merge for Excel ↔ UE synchronization
// v4.5: ApplyToAssets integration - write validated rows to UNPCDefinition assets
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

#include "XLSXSupport/NPCXLSXSyncEngine.h"
#include "NPCTableEditor/NPCAssetSync.h"
#include "NPCTableEditor/NPCTableValidator.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "AI/NPCDefinition.h"
#endif

//=============================================================================
// FNPCSyncEntry Implementation
//=============================================================================

FString FNPCSyncEntry::GetStatusText() const
{
	switch (Status)
	{
		case ENPCSyncStatus::Unchanged:        return TEXT("Unchanged");
		case ENPCSyncStatus::ModifiedInUE:     return TEXT("Modified in UE");
		case ENPCSyncStatus::ModifiedInExcel:  return TEXT("Modified in Excel");
		case ENPCSyncStatus::Conflict:         return TEXT("CONFLICT");
		case ENPCSyncStatus::AddedInUE:        return TEXT("Added in UE");
		case ENPCSyncStatus::AddedInExcel:     return TEXT("Added in Excel");
		case ENPCSyncStatus::DeletedInUE:      return TEXT("Deleted in UE");
		case ENPCSyncStatus::DeletedInExcel:   return TEXT("Deleted in Excel");
		case ENPCSyncStatus::DeleteConflict:   return TEXT("DELETE CONFLICT");
		default:                               return TEXT("Unknown");
	}
}

FLinearColor FNPCSyncEntry::GetStatusColor() const
{
	switch (Status)
	{
		case ENPCSyncStatus::Unchanged:        return FLinearColor(0.5f, 0.5f, 0.5f);  // Gray
		case ENPCSyncStatus::ModifiedInUE:     return FLinearColor(0.2f, 0.6f, 1.0f);  // Blue
		case ENPCSyncStatus::ModifiedInExcel:  return FLinearColor(0.2f, 0.8f, 0.2f);  // Green
		case ENPCSyncStatus::Conflict:         return FLinearColor(1.0f, 0.2f, 0.2f);  // Red
		case ENPCSyncStatus::AddedInUE:        return FLinearColor(0.2f, 0.6f, 1.0f);  // Blue
		case ENPCSyncStatus::AddedInExcel:     return FLinearColor(0.2f, 0.8f, 0.2f);  // Green
		case ENPCSyncStatus::DeletedInUE:      return FLinearColor(1.0f, 0.6f, 0.2f);  // Orange
		case ENPCSyncStatus::DeletedInExcel:   return FLinearColor(1.0f, 0.6f, 0.2f);  // Orange
		case ENPCSyncStatus::DeleteConflict:   return FLinearColor(1.0f, 0.2f, 0.2f);  // Red
		default:                               return FLinearColor::White;
	}
}

bool FNPCSyncEntry::RequiresResolution() const
{
	return Status == ENPCSyncStatus::Conflict || Status == ENPCSyncStatus::DeleteConflict;
}

FString FNPCSyncEntry::GetDisplayName() const
{
	// Return NPC name from whichever version exists
	if (UERow.IsValid()) return UERow->NPCName;
	if (ExcelRow.IsValid()) return ExcelRow->NPCName;
	if (BaseRow.IsValid()) return BaseRow->NPCName;
	return TEXT("(Unknown)");
}

//=============================================================================
// FNPCSyncResult Implementation
//=============================================================================

bool FNPCSyncResult::HasChanges() const
{
	return ModifiedInUECount > 0 ||
		   ModifiedInExcelCount > 0 ||
		   ConflictCount > 0 ||
		   AddedInUECount > 0 ||
		   AddedInExcelCount > 0 ||
		   DeletedCount > 0;
}

int32 FNPCSyncResult::GetTotalChanges() const
{
	return ModifiedInUECount +
		   ModifiedInExcelCount +
		   ConflictCount +
		   AddedInUECount +
		   AddedInExcelCount +
		   DeletedCount;
}

TArray<FNPCSyncEntry*> FNPCSyncResult::GetConflicts()
{
	TArray<FNPCSyncEntry*> Result;
	for (FNPCSyncEntry& Entry : Entries)
	{
		if (Entry.Status == ENPCSyncStatus::Conflict)
		{
			Result.Add(&Entry);
		}
	}
	return Result;
}

TArray<FNPCSyncEntry*> FNPCSyncResult::GetEntriesRequiringResolution()
{
	TArray<FNPCSyncEntry*> Result;
	for (FNPCSyncEntry& Entry : Entries)
	{
		if (Entry.RequiresResolution())
		{
			Result.Add(&Entry);
		}
	}
	return Result;
}

//=============================================================================
// FNPCXLSXSyncEngine Implementation
//=============================================================================

FNPCSyncResult FNPCXLSXSyncEngine::CompareSources(
	const TArray<FNPCTableRow>& BaseRows,
	const TArray<FNPCTableRow>& UERows,
	const TArray<FNPCTableRow>& ExcelRows)
{
	FNPCSyncResult Result;
	Result.bSuccess = true;

	// Build lookup maps for O(1) access by RowId
	TMap<FGuid, const FNPCTableRow*> BaseMap = BuildRowMap(BaseRows);
	TMap<FGuid, const FNPCTableRow*> UEMap = BuildRowMap(UERows);
	TMap<FGuid, const FNPCTableRow*> ExcelMap = BuildRowMap(ExcelRows);

	// Collect all unique RowIds from all three sources
	TSet<FGuid> AllRowIds;
	for (const FNPCTableRow& Row : BaseRows) AllRowIds.Add(Row.RowId);
	for (const FNPCTableRow& Row : UERows) AllRowIds.Add(Row.RowId);
	for (const FNPCTableRow& Row : ExcelRows) AllRowIds.Add(Row.RowId);

	// Process each unique RowId
	for (const FGuid& RowId : AllRowIds)
	{
		const FNPCTableRow* BaseRow = BaseMap.FindRef(RowId);
		const FNPCTableRow* UERow = UEMap.FindRef(RowId);
		const FNPCTableRow* ExcelRow = ExcelMap.FindRef(RowId);

		// Compute hashes (0 if row doesn't exist)
		int64 BaseHash = BaseRow ? ComputeRowHash(*BaseRow) : 0;
		int64 UEHash = UERow ? ComputeRowHash(*UERow) : 0;
		int64 ExcelHash = ExcelRow ? ComputeRowHash(*ExcelRow) : 0;

		// Create sync entry
		FNPCSyncEntry Entry;
		Entry.RowId = RowId;
		Entry.BaseHash = BaseHash;
		Entry.UEHash = UEHash;
		Entry.ExcelHash = ExcelHash;

		// Store row copies (as shared pointers for safe reference)
		if (BaseRow) Entry.BaseRow = MakeShared<FNPCTableRow>(*BaseRow);
		if (UERow) Entry.UERow = MakeShared<FNPCTableRow>(*UERow);
		if (ExcelRow) Entry.ExcelRow = MakeShared<FNPCTableRow>(*ExcelRow);

		// Determine sync status
		Entry.Status = DetermineStatus(BaseRow, UERow, ExcelRow, BaseHash, UEHash, ExcelHash);

		// Update statistics
		switch (Entry.Status)
		{
			case ENPCSyncStatus::Unchanged:        Result.UnchangedCount++; break;
			case ENPCSyncStatus::ModifiedInUE:     Result.ModifiedInUECount++; break;
			case ENPCSyncStatus::ModifiedInExcel:  Result.ModifiedInExcelCount++; break;
			case ENPCSyncStatus::Conflict:         Result.ConflictCount++; break;
			case ENPCSyncStatus::AddedInUE:        Result.AddedInUECount++; break;
			case ENPCSyncStatus::AddedInExcel:     Result.AddedInExcelCount++; break;
			case ENPCSyncStatus::DeletedInUE:      Result.DeletedCount++; break;
			case ENPCSyncStatus::DeletedInExcel:   Result.DeletedCount++; break;
			case ENPCSyncStatus::DeleteConflict:   Result.ConflictCount++; break;
		}

		Result.Entries.Add(Entry);
	}

	// Sort entries by NPC name for consistent display
	Result.Entries.Sort([](const FNPCSyncEntry& A, const FNPCSyncEntry& B)
	{
		return A.GetDisplayName() < B.GetDisplayName();
	});

	return Result;
}

FNPCMergeResult FNPCXLSXSyncEngine::ApplySync(const FNPCSyncResult& SyncResult)
{
	FNPCMergeResult Result;

	for (const FNPCSyncEntry& Entry : SyncResult.Entries)
	{
		bool bShouldInclude = false;
		FNPCTableRow RowToInclude;

		switch (Entry.Status)
		{
			case ENPCSyncStatus::Unchanged:
				// Keep UE version (they're identical anyway)
				if (Entry.UERow.IsValid())
				{
					RowToInclude = *Entry.UERow;
					RowToInclude.Status = ENPCTableRowStatus::Synced;
					bShouldInclude = true;
					Result.Unchanged++;
				}
				break;

			case ENPCSyncStatus::ModifiedInUE:
				// Keep UE changes
				if (Entry.UERow.IsValid())
				{
					RowToInclude = *Entry.UERow;
					RowToInclude.Status = ENPCTableRowStatus::Modified;
					bShouldInclude = true;
					Result.AppliedFromUE++;
				}
				break;

			case ENPCSyncStatus::ModifiedInExcel:
				// Apply Excel changes
				if (Entry.ExcelRow.IsValid())
				{
					RowToInclude = *Entry.ExcelRow;
					RowToInclude.Status = ENPCTableRowStatus::Modified;
					bShouldInclude = true;
					Result.AppliedFromExcel++;
				}
				break;

			case ENPCSyncStatus::AddedInUE:
				// Include new UE row
				if (Entry.UERow.IsValid())
				{
					RowToInclude = *Entry.UERow;
					RowToInclude.Status = ENPCTableRowStatus::New;
					bShouldInclude = true;
					Result.AppliedFromUE++;
				}
				break;

			case ENPCSyncStatus::AddedInExcel:
				// Include new Excel row
				if (Entry.ExcelRow.IsValid())
				{
					RowToInclude = *Entry.ExcelRow;
					RowToInclude.Status = ENPCTableRowStatus::New;
					bShouldInclude = true;
					Result.AppliedFromExcel++;
				}
				break;

			case ENPCSyncStatus::DeletedInUE:
			case ENPCSyncStatus::DeletedInExcel:
				// Don't include - row is deleted
				Result.Deleted++;
				break;

			case ENPCSyncStatus::Conflict:
			case ENPCSyncStatus::DeleteConflict:
				// Apply user's resolution
				switch (Entry.Resolution)
				{
					case ENPCConflictResolution::KeepUE:
						if (Entry.UERow.IsValid())
						{
							RowToInclude = *Entry.UERow;
							RowToInclude.Status = ENPCTableRowStatus::Modified;
							bShouldInclude = true;
							Result.AppliedFromUE++;
						}
						break;

					case ENPCConflictResolution::KeepExcel:
						if (Entry.ExcelRow.IsValid())
						{
							RowToInclude = *Entry.ExcelRow;
							RowToInclude.Status = ENPCTableRowStatus::Modified;
							bShouldInclude = true;
							Result.AppliedFromExcel++;
						}
						break;

					case ENPCConflictResolution::KeepBoth:
						// Add both with new GUIDs for the duplicates
						if (Entry.UERow.IsValid())
						{
							FNPCTableRow UECopy = *Entry.UERow;
							UECopy.Status = ENPCTableRowStatus::Modified;
							Result.MergedRows.Add(UECopy);
							Result.AppliedFromUE++;
						}
						if (Entry.ExcelRow.IsValid())
						{
							FNPCTableRow ExcelCopy = *Entry.ExcelRow;
							ExcelCopy.RowId = FGuid::NewGuid();  // New GUID to avoid collision
							ExcelCopy.NPCName += TEXT("_Excel");  // Disambiguate name
							ExcelCopy.Status = ENPCTableRowStatus::New;
							Result.MergedRows.Add(ExcelCopy);
							Result.AppliedFromExcel++;
						}
						break;

					case ENPCConflictResolution::Delete:
						Result.Deleted++;
						break;

					default:
						// Unresolved - should not happen if AllConflictsResolved() was checked
						// Default to keeping UE version
						if (Entry.UERow.IsValid())
						{
							RowToInclude = *Entry.UERow;
							RowToInclude.Status = ENPCTableRowStatus::Modified;
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

void FNPCXLSXSyncEngine::AutoResolveNonConflicts(FNPCSyncResult& SyncResult)
{
	for (FNPCSyncEntry& Entry : SyncResult.Entries)
	{
		// Skip entries that require user resolution
		if (Entry.RequiresResolution())
		{
			continue;
		}

		// Auto-set resolution based on status
		switch (Entry.Status)
		{
			case ENPCSyncStatus::Unchanged:
			case ENPCSyncStatus::ModifiedInUE:
			case ENPCSyncStatus::AddedInUE:
				Entry.Resolution = ENPCConflictResolution::KeepUE;
				break;

			case ENPCSyncStatus::ModifiedInExcel:
			case ENPCSyncStatus::AddedInExcel:
				Entry.Resolution = ENPCConflictResolution::KeepExcel;
				break;

			case ENPCSyncStatus::DeletedInUE:
			case ENPCSyncStatus::DeletedInExcel:
				Entry.Resolution = ENPCConflictResolution::Delete;
				break;

			default:
				break;
		}
	}
}

bool FNPCXLSXSyncEngine::AllConflictsResolved(const FNPCSyncResult& SyncResult)
{
	for (const FNPCSyncEntry& Entry : SyncResult.Entries)
	{
		if (Entry.RequiresResolution() && Entry.Resolution == ENPCConflictResolution::Unresolved)
		{
			return false;
		}
	}
	return true;
}

int64 FNPCXLSXSyncEngine::ComputeRowHash(const FNPCTableRow& Row)
{
	// Hash all content fields (same as writer for consistency)
	// Excludes RowId which is for identity tracking, not content comparison
	int64 Hash = 0;

	// Core Identity
	Hash ^= GetTypeHash(Row.NPCName);
	Hash ^= GetTypeHash(Row.NPCId);
	Hash ^= GetTypeHash(Row.DisplayName);
	Hash ^= GetTypeHash(Row.Blueprint.ToString());

	// AI & Behavior
	Hash ^= GetTypeHash(Row.AbilityConfig.ToString());
	Hash ^= GetTypeHash(Row.ActivityConfig.ToString());
	Hash ^= GetTypeHash(Row.Schedule.ToString());
	Hash ^= GetTypeHash(Row.BehaviorTree.ToString());

	// Combat
	Hash ^= GetTypeHash(Row.MinLevel);
	Hash ^= GetTypeHash(Row.MaxLevel);
	Hash ^= GetTypeHash(Row.Factions);
	Hash ^= GetTypeHash(Row.AttackPriority);

	// Vendor
	Hash ^= GetTypeHash(Row.bIsVendor);
	Hash ^= GetTypeHash(Row.ShopName);

	// Items & Spawning
	Hash ^= GetTypeHash(Row.DefaultItems);
	Hash ^= GetTypeHash(Row.SpawnerPOI);

	// Meta
	Hash ^= GetTypeHash(Row.Appearance.ToString());
	Hash ^= GetTypeHash(Row.Notes);

	return Hash;
}

TMap<FGuid, const FNPCTableRow*> FNPCXLSXSyncEngine::BuildRowMap(const TArray<FNPCTableRow>& Rows)
{
	TMap<FGuid, const FNPCTableRow*> Map;
	for (const FNPCTableRow& Row : Rows)
	{
		Map.Add(Row.RowId, &Row);
	}
	return Map;
}

ENPCSyncStatus FNPCXLSXSyncEngine::DetermineStatus(
	const FNPCTableRow* BaseRow,
	const FNPCTableRow* UERow,
	const FNPCTableRow* ExcelRow,
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
		return ENPCSyncStatus::AddedInUE;
	}

	if (!bInBase && !bInUE && bInExcel)
	{
		// Only in Excel - added in spreadsheet
		return ENPCSyncStatus::AddedInExcel;
	}

	if (bInBase && !bInUE && bInExcel)
	{
		// In Base and Excel but not UE - deleted locally
		return ENPCSyncStatus::DeletedInUE;
	}

	if (bInBase && bInUE && !bInExcel)
	{
		// In Base and UE but not Excel - deleted in spreadsheet
		// Note: This could also be detected via #STATE=Deleted column
		return ENPCSyncStatus::DeletedInExcel;
	}

	if (!bInBase && bInUE && bInExcel)
	{
		// In both UE and Excel but not Base - added in both
		// Check if they're the same (same content added independently)
		if (UEHash == ExcelHash)
		{
			return ENPCSyncStatus::Unchanged;  // Same addition in both
		}
		else
		{
			return ENPCSyncStatus::Conflict;  // Different additions with same GUID (rare)
		}
	}

	if (bInBase && !bInUE && !bInExcel)
	{
		// Was in Base but deleted in both - already deleted, no action
		return ENPCSyncStatus::DeletedInUE;  // Treat as already handled
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
			return ENPCSyncStatus::Unchanged;
		}

		if (BaseHash == UEHash && BaseHash != ExcelHash)
		{
			// Excel changed, UE unchanged - apply Excel
			return ENPCSyncStatus::ModifiedInExcel;
		}

		if (BaseHash != UEHash && BaseHash == ExcelHash)
		{
			// UE changed, Excel unchanged - keep UE
			return ENPCSyncStatus::ModifiedInUE;
		}

		if (BaseHash != UEHash && BaseHash != ExcelHash)
		{
			// Both changed - check if they made the same changes
			if (UEHash == ExcelHash)
			{
				// Same changes made in both - no conflict
				return ENPCSyncStatus::Unchanged;  // Converged
			}
			else
			{
				// Different changes - conflict
				return ENPCSyncStatus::Conflict;
			}
		}
	}

	// Fallback - should not reach here with valid input
	return ENPCSyncStatus::Unchanged;
}

//=============================================================================
// v4.5: Apply Rows to Assets
//=============================================================================

FNPCAssetApplySummary FNPCXLSXSyncEngine::ApplyToAssets(
	TArray<FNPCTableRow>& Rows,
	const FString& NPCAssetPath,
	bool bCreateMissing)
{
	FNPCAssetApplySummary Summary;

#if WITH_EDITOR
	// Step 1: Filter rows that have changes to apply
	TArray<FNPCTableRow*> RowsToApply;
	for (FNPCTableRow& Row : Rows)
	{
		// Only apply rows that are Modified or New
		if (Row.Status == ENPCTableRowStatus::Modified || Row.Status == ENPCTableRowStatus::New)
		{
			RowsToApply.Add(&Row);
		}
		else
		{
			Summary.AssetsSkippedNotModified++;
		}
	}

	if (RowsToApply.Num() == 0)
	{
		Summary.bSuccess = true;
		Summary.ErrorMessage = TEXT("No modified rows to apply");
		return Summary;
	}

	// Step 2: Validate all rows first
	TArray<FNPCTableRow> ValidationRows;
	for (FNPCTableRow* RowPtr : RowsToApply)
	{
		ValidationRows.Add(*RowPtr);
	}

	FNPCValidationResult ValidationResult = FNPCTableValidator::ValidateAll(ValidationRows);

	// Build set of rows with validation errors
	TSet<FString> RowsWithErrors;
	for (const FNPCValidationIssue& Issue : ValidationResult.Issues)
	{
		if (Issue.Severity == ENPCValidationSeverity::Error)
		{
			RowsWithErrors.Add(Issue.NPCName);
		}
	}

	// Step 3: Get asset registry for finding NPC assets
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Step 4: Process each row
	for (FNPCTableRow* RowPtr : RowsToApply)
	{
		FNPCTableRow& Row = *RowPtr;
		Summary.AssetsProcessed++;

		// Skip rows with validation errors
		if (RowsWithErrors.Contains(Row.NPCName) || RowsWithErrors.Contains(Row.RowId.ToString()))
		{
			Summary.AssetsSkippedValidation++;
			Summary.AssetResults.Add(Row.NPCName, TEXT("Skipped: Validation errors"));
			continue;
		}

		// Try to find the NPCDefinition asset
		UNPCDefinition* NPCDef = nullptr;

		// First: Check if row has a direct asset reference
		if (!Row.GeneratedNPCDef.IsNull())
		{
			NPCDef = Cast<UNPCDefinition>(Row.GeneratedNPCDef.TryLoad());
		}

		// Second: Search by NPCName using AssetRegistry
		if (!NPCDef)
		{
			FARFilter Filter;
			Filter.ClassPaths.Add(UNPCDefinition::StaticClass()->GetClassPathName());
			Filter.PackagePaths.Add(FName(*NPCAssetPath));
			Filter.bRecursivePaths = true;

			TArray<FAssetData> AssetDataList;
			AssetRegistry.GetAssets(Filter, AssetDataList);

			// Find matching asset by name pattern
			FString SearchName = FString::Printf(TEXT("NPC_%s"), *Row.NPCName);
			for (const FAssetData& AssetData : AssetDataList)
			{
				if (AssetData.AssetName.ToString() == SearchName ||
				    AssetData.AssetName.ToString() == Row.NPCName)
				{
					NPCDef = Cast<UNPCDefinition>(AssetData.GetAsset());
					if (NPCDef)
					{
						// Cache the reference for future use
						Row.GeneratedNPCDef = FSoftObjectPath(NPCDef);
						break;
					}
				}
			}
		}

		// Third: Try common path patterns
		if (!NPCDef)
		{
			TArray<FString> SearchPaths = {
				FString::Printf(TEXT("%sNPC_%s.NPC_%s"), *NPCAssetPath, *Row.NPCName, *Row.NPCName),
				FString::Printf(TEXT("%sNPCs/NPC_%s.NPC_%s"), *NPCAssetPath, *Row.NPCName, *Row.NPCName),
				FString::Printf(TEXT("/Game/NPCs/NPC_%s.NPC_%s"), *Row.NPCName, *Row.NPCName),
				FString::Printf(TEXT("/Game/NPCs/Definitions/NPC_%s.NPC_%s"), *Row.NPCName, *Row.NPCName)
			};

			for (const FString& Path : SearchPaths)
			{
				NPCDef = LoadObject<UNPCDefinition>(nullptr, *Path);
				if (NPCDef)
				{
					Row.GeneratedNPCDef = FSoftObjectPath(NPCDef);
					break;
				}
			}
		}

		// Handle missing asset
		if (!NPCDef)
		{
			if (bCreateMissing)
			{
				// TODO: Create new asset via FNPCDefinitionGenerator
				Summary.AssetsCreated++;
				Summary.AssetResults.Add(Row.NPCName, TEXT("Created: New asset"));
				continue;
			}
			else
			{
				Summary.AssetsSkippedNoAsset++;
				Summary.AssetResults.Add(Row.NPCName, TEXT("Skipped: No asset found"));
				continue;
			}
		}

		// Check if asset is writable (not in plugin content)
		UPackage* Package = NPCDef->GetOutermost();
		if (Package)
		{
			FString PackagePath = Package->GetName();
			if (!PackagePath.StartsWith(TEXT("/Game/")))
			{
				Summary.AssetsSkippedReadOnly++;
				Summary.AssetResults.Add(Row.NPCName, TEXT("Skipped: Read-only (plugin content)"));
				continue;
			}
		}

		// Apply row data to asset
		if (FNPCAssetSync::ApplyRowToAsset(Row, NPCDef))
		{
			// Save the asset
			Package = NPCDef->GetOutermost();
			if (Package)
			{
				Package->MarkPackageDirty();

				FString PackageFileName = FPackageName::LongPackageNameToFilename(
					Package->GetName(),
					FPackageName::GetAssetPackageExtension());

				FSavePackageArgs SaveArgs;
				SaveArgs.TopLevelFlags = RF_Standalone;

				if (UPackage::SavePackage(Package, NPCDef, *PackageFileName, SaveArgs))
				{
					Row.Status = ENPCTableRowStatus::Synced;
					Summary.AssetsModified++;
					Summary.AssetResults.Add(Row.NPCName, TEXT("Updated: Applied changes"));
				}
				else
				{
					Summary.FailedNPCs.Add(Row.NPCName);
					Summary.AssetResults.Add(Row.NPCName, TEXT("Failed: Could not save asset"));
				}
			}
		}
		else
		{
			Summary.FailedNPCs.Add(Row.NPCName);
			Summary.AssetResults.Add(Row.NPCName, TEXT("Failed: Could not apply row data"));
		}
	}

	Summary.bSuccess = true;

	UE_LOG(LogTemp, Log, TEXT("NPCXLSXSyncEngine: Applied to %d assets (%d modified, %d created, %d skipped, %d failed)"),
		Summary.AssetsProcessed, Summary.AssetsModified, Summary.AssetsCreated,
		Summary.AssetsSkippedNotModified + Summary.AssetsSkippedValidation + Summary.AssetsSkippedNoAsset + Summary.AssetsSkippedReadOnly,
		Summary.FailedNPCs.Num());

#else
	Summary.ErrorMessage = TEXT("ApplyToAssets requires WITH_EDITOR");
#endif

	return Summary;
}
