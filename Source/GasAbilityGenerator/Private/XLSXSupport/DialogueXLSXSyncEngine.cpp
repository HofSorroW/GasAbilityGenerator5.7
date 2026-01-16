// GasAbilityGenerator - Dialogue XLSX Sync Engine Implementation
// v4.4: 3-way merge for Excel ↔ UE synchronization with token support
// v4.4 Phase 4: Asset apply integration
//
// Phase 2: Partial Apply Logic
// - If TEXT valid but EVENTS invalid, apply TEXT only, preserve UE events
// - Invalid token NEVER wipes UE data - it flags an error instead
//
// Phase 4: Asset Apply Integration
// - ApplyTokensToAssets() writes validated tokens to UDialogueBlueprint assets
// - Groups rows by DialogueID and applies to each asset

#include "XLSXSupport/DialogueXLSXSyncEngine.h"
#include "XLSXSupport/DialogueAssetSync.h"
#include "AssetRegistry/AssetRegistryModule.h"

#if WITH_EDITOR
#include "DialogueBlueprint.h"
#endif

//=============================================================================
// FDialogueSyncEntry
//=============================================================================

FString FDialogueSyncEntry::GetStatusText() const
{
	switch (Status)
	{
		case EDialogueSyncStatus::Unchanged:       return TEXT("Unchanged");
		case EDialogueSyncStatus::ModifiedInUE:    return TEXT("Modified in UE");
		case EDialogueSyncStatus::ModifiedInExcel: return TEXT("Modified in Excel");
		case EDialogueSyncStatus::Conflict:        return TEXT("CONFLICT");
		case EDialogueSyncStatus::AddedInUE:       return TEXT("Added in UE");
		case EDialogueSyncStatus::AddedInExcel:    return TEXT("Added in Excel");
		case EDialogueSyncStatus::DeletedInUE:     return TEXT("Deleted in UE");
		case EDialogueSyncStatus::DeletedInExcel:  return TEXT("Deleted in Excel");
		case EDialogueSyncStatus::DeleteConflict:  return TEXT("Delete Conflict");
		default: return TEXT("Unknown");
	}
}

FLinearColor FDialogueSyncEntry::GetStatusColor() const
{
	switch (Status)
	{
		case EDialogueSyncStatus::Unchanged:       return FLinearColor(0.5f, 0.5f, 0.5f);  // Gray
		case EDialogueSyncStatus::ModifiedInUE:    return FLinearColor(0.2f, 0.6f, 1.0f);  // Blue
		case EDialogueSyncStatus::ModifiedInExcel: return FLinearColor(0.2f, 0.8f, 0.2f);  // Green
		case EDialogueSyncStatus::Conflict:        return FLinearColor(1.0f, 0.3f, 0.3f);  // Red
		case EDialogueSyncStatus::AddedInUE:       return FLinearColor(0.3f, 0.7f, 1.0f);  // Light Blue
		case EDialogueSyncStatus::AddedInExcel:    return FLinearColor(0.3f, 1.0f, 0.5f);  // Light Green
		case EDialogueSyncStatus::DeletedInUE:     return FLinearColor(1.0f, 0.6f, 0.2f);  // Orange
		case EDialogueSyncStatus::DeletedInExcel:  return FLinearColor(1.0f, 0.5f, 0.0f);  // Orange
		case EDialogueSyncStatus::DeleteConflict:  return FLinearColor(1.0f, 0.0f, 0.5f);  // Magenta
		default: return FLinearColor::White;
	}
}

bool FDialogueSyncEntry::RequiresResolution() const
{
	// v4.11: All statuses except Unchanged require user resolution
	// No more auto-accepting ModifiedInUE, ModifiedInExcel, Added*, Deleted*, etc.
	return Status != EDialogueSyncStatus::Unchanged;
}

//=============================================================================
// FDialogueSyncResult
//=============================================================================

bool FDialogueSyncResult::HasChanges() const
{
	return ModifiedInUECount > 0 || ModifiedInExcelCount > 0 ||
	       ConflictCount > 0 || AddedInUECount > 0 || AddedInExcelCount > 0 ||
	       DeletedCount > 0;
}

int32 FDialogueSyncResult::GetTotalChanges() const
{
	return ModifiedInUECount + ModifiedInExcelCount + ConflictCount +
	       AddedInUECount + AddedInExcelCount + DeletedCount;
}

TArray<FDialogueSyncEntry*> FDialogueSyncResult::GetConflicts()
{
	TArray<FDialogueSyncEntry*> Result;
	for (FDialogueSyncEntry& Entry : Entries)
	{
		if (Entry.Status == EDialogueSyncStatus::Conflict ||
		    Entry.Status == EDialogueSyncStatus::DeleteConflict)
		{
			Result.Add(&Entry);
		}
	}
	return Result;
}

TArray<FDialogueSyncEntry*> FDialogueSyncResult::GetEntriesRequiringResolution()
{
	TArray<FDialogueSyncEntry*> Result;
	for (FDialogueSyncEntry& Entry : Entries)
	{
		if (Entry.RequiresResolution() && Entry.Resolution == EDialogueConflictResolution::Unresolved)
		{
			Result.Add(&Entry);
		}
	}
	return Result;
}

//=============================================================================
// FDialogueXLSXSyncEngine
//=============================================================================

int64 FDialogueXLSXSyncEngine::ComputeRowHash(const FDialogueTableRow& Row)
{
	// Hash all content fields (excluding RowId which is identity, not content)
	uint32 Hash = 0;
	Hash = HashCombine(Hash, GetTypeHash(Row.DialogueID));
	Hash = HashCombine(Hash, GetTypeHash(Row.NodeID));
	Hash = HashCombine(Hash, GetTypeHash(Row.Speaker));
	Hash = HashCombine(Hash, GetTypeHash(Row.ParentNodeID));
	Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Row.NodeType)));
	Hash = HashCombine(Hash, GetTypeHash(Row.Text));
	Hash = HashCombine(Hash, GetTypeHash(Row.OptionText));
	Hash = HashCombine(Hash, GetTypeHash(Row.Notes));
	Hash = HashCombine(Hash, GetTypeHash(Row.bSkippable));

	// v4.4: Include token strings in hash
	Hash = HashCombine(Hash, GetTypeHash(Row.EventsTokenStr));
	Hash = HashCombine(Hash, GetTypeHash(Row.ConditionsTokenStr));

	// Hash NextNodeIDs array
	for (const FName& NextNode : Row.NextNodeIDs)
	{
		Hash = HashCombine(Hash, GetTypeHash(NextNode));
	}

	return static_cast<int64>(Hash);
}

TMap<FGuid, const FDialogueTableRow*> FDialogueXLSXSyncEngine::BuildRowMap(const TArray<FDialogueTableRow>& Rows)
{
	TMap<FGuid, const FDialogueTableRow*> Map;
	for (const FDialogueTableRow& Row : Rows)
	{
		if (Row.RowId.IsValid())
		{
			Map.Add(Row.RowId, &Row);
		}
	}
	return Map;
}

EDialogueSyncStatus FDialogueXLSXSyncEngine::DetermineStatus(
	const FDialogueTableRow* BaseRow,
	const FDialogueTableRow* UERow,
	const FDialogueTableRow* ExcelRow,
	int64 BaseHash, int64 UEHash, int64 ExcelHash)
{
	// Case 1: Row exists in all three
	if (BaseRow && UERow && ExcelRow)
	{
		if (BaseHash == UEHash && BaseHash == ExcelHash)
		{
			return EDialogueSyncStatus::Unchanged;
		}
		if (BaseHash != UEHash && BaseHash == ExcelHash)
		{
			return EDialogueSyncStatus::ModifiedInUE;
		}
		if (BaseHash == UEHash && BaseHash != ExcelHash)
		{
			return EDialogueSyncStatus::ModifiedInExcel;
		}
		// Both modified
		if (UEHash == ExcelHash)
		{
			// Same change made in both - treat as unchanged
			return EDialogueSyncStatus::Unchanged;
		}
		return EDialogueSyncStatus::Conflict;
	}

	// Case 2: Row added (not in base)
	if (!BaseRow)
	{
		if (UERow && !ExcelRow)
		{
			return EDialogueSyncStatus::AddedInUE;
		}
		if (!UERow && ExcelRow)
		{
			return EDialogueSyncStatus::AddedInExcel;
		}
		if (UERow && ExcelRow)
		{
			// Added in both - check if same content
			if (UEHash == ExcelHash)
			{
				return EDialogueSyncStatus::Unchanged;
			}
			return EDialogueSyncStatus::Conflict;
		}
	}

	// Case 3: Row deleted
	if (BaseRow)
	{
		if (!UERow && ExcelRow)
		{
			// Deleted in UE
			if (BaseHash == ExcelHash)
			{
				return EDialogueSyncStatus::DeletedInUE;
			}
			// Modified in Excel but deleted in UE
			return EDialogueSyncStatus::DeleteConflict;
		}
		if (UERow && !ExcelRow)
		{
			// Deleted in Excel
			if (BaseHash == UEHash)
			{
				return EDialogueSyncStatus::DeletedInExcel;
			}
			// Modified in UE but deleted in Excel
			return EDialogueSyncStatus::DeleteConflict;
		}
		if (!UERow && !ExcelRow)
		{
			// Deleted in both - nothing to do
			return EDialogueSyncStatus::Unchanged;
		}
	}

	return EDialogueSyncStatus::Unchanged;
}

FDialogueSyncResult FDialogueXLSXSyncEngine::CompareSources(
	const TArray<FDialogueTableRow>& BaseRows,
	const TArray<FDialogueTableRow>& UERows,
	const TArray<FDialogueTableRow>& ExcelRows)
{
	FDialogueSyncResult Result;
	Result.bSuccess = true;

	// Build lookup maps
	TMap<FGuid, const FDialogueTableRow*> BaseMap = BuildRowMap(BaseRows);
	TMap<FGuid, const FDialogueTableRow*> UEMap = BuildRowMap(UERows);
	TMap<FGuid, const FDialogueTableRow*> ExcelMap = BuildRowMap(ExcelRows);

	// Collect all unique RowIds
	TSet<FGuid> AllRowIds;
	for (const auto& Pair : BaseMap) AllRowIds.Add(Pair.Key);
	for (const auto& Pair : UEMap) AllRowIds.Add(Pair.Key);
	for (const auto& Pair : ExcelMap) AllRowIds.Add(Pair.Key);

	// Compare each row
	for (const FGuid& RowId : AllRowIds)
	{
		const FDialogueTableRow* BaseRow = BaseMap.FindRef(RowId);
		const FDialogueTableRow* UERow = UEMap.FindRef(RowId);
		const FDialogueTableRow* ExcelRow = ExcelMap.FindRef(RowId);

		// Compute hashes
		int64 BaseHash = BaseRow ? ComputeRowHash(*BaseRow) : 0;
		int64 UEHash = UERow ? ComputeRowHash(*UERow) : 0;
		int64 ExcelHash = ExcelRow ? ComputeRowHash(*ExcelRow) : 0;

		// Determine status
		EDialogueSyncStatus Status = DetermineStatus(BaseRow, UERow, ExcelRow, BaseHash, UEHash, ExcelHash);

		// Create entry
		FDialogueSyncEntry Entry;
		Entry.RowId = RowId;
		Entry.Status = Status;
		Entry.BaseHash = BaseHash;
		Entry.UEHash = UEHash;
		Entry.ExcelHash = ExcelHash;

		// Copy row data
		if (BaseRow)
		{
			Entry.BaseRow = MakeShared<FDialogueTableRow>(*BaseRow);
		}
		if (UERow)
		{
			Entry.UERow = MakeShared<FDialogueTableRow>(*UERow);
		}
		if (ExcelRow)
		{
			Entry.ExcelRow = MakeShared<FDialogueTableRow>(*ExcelRow);
		}

		// Update stats
		switch (Status)
		{
			case EDialogueSyncStatus::Unchanged:       Result.UnchangedCount++; break;
			case EDialogueSyncStatus::ModifiedInUE:    Result.ModifiedInUECount++; break;
			case EDialogueSyncStatus::ModifiedInExcel: Result.ModifiedInExcelCount++; break;
			case EDialogueSyncStatus::Conflict:        Result.ConflictCount++; break;
			case EDialogueSyncStatus::AddedInUE:       Result.AddedInUECount++; break;
			case EDialogueSyncStatus::AddedInExcel:    Result.AddedInExcelCount++; break;
			case EDialogueSyncStatus::DeletedInUE:
			case EDialogueSyncStatus::DeletedInExcel:  Result.DeletedCount++; break;
			case EDialogueSyncStatus::DeleteConflict:  Result.ConflictCount++; break;
		}

		Result.Entries.Add(MoveTemp(Entry));
	}

	// v4.9.1: Sort entries by DialogueID + NodeID for consistent display order
	// (Matches NPC sync engine pattern which sorts by display name)
	Result.Entries.Sort([](const FDialogueSyncEntry& A, const FDialogueSyncEntry& B)
	{
		// Get the best available row for each entry (prefer UE, then Excel, then Base)
		const FDialogueTableRow* RowA = A.UERow.Get() ? A.UERow.Get() : (A.ExcelRow.Get() ? A.ExcelRow.Get() : A.BaseRow.Get());
		const FDialogueTableRow* RowB = B.UERow.Get() ? B.UERow.Get() : (B.ExcelRow.Get() ? B.ExcelRow.Get() : B.BaseRow.Get());

		// Fallback to GUID comparison if no row data available
		if (!RowA || !RowB)
		{
			return A.RowId < B.RowId;
		}

		// Sort by DialogueID first, then by NodeID
		if (RowA->DialogueID != RowB->DialogueID)
		{
			return RowA->DialogueID.LexicalLess(RowB->DialogueID);
		}
		return RowA->NodeID.LexicalLess(RowB->NodeID);
	});

	return Result;
}

void FDialogueXLSXSyncEngine::AutoResolveNonConflicts(FDialogueSyncResult& SyncResult)
{
	// v4.11: Only Unchanged auto-resolves. All other statuses require explicit user approval.
	// This prevents accidental acceptance of changes the user might want to review.
	for (FDialogueSyncEntry& Entry : SyncResult.Entries)
	{
		if (Entry.Status == EDialogueSyncStatus::Unchanged)
		{
			Entry.Resolution = EDialogueConflictResolution::KeepUE;  // Either is fine, content is identical
		}
		// All other statuses remain Unresolved, requiring user selection in approval UI
	}
}

bool FDialogueXLSXSyncEngine::AllConflictsResolved(const FDialogueSyncResult& SyncResult)
{
	for (const FDialogueSyncEntry& Entry : SyncResult.Entries)
	{
		if (Entry.RequiresResolution() && Entry.Resolution == EDialogueConflictResolution::Unresolved)
		{
			return false;
		}
	}
	return true;
}

FDialogueMergeResult FDialogueXLSXSyncEngine::ApplySync(const FDialogueSyncResult& SyncResult)
{
	FDialogueMergeResult Result;

	for (const FDialogueSyncEntry& Entry : SyncResult.Entries)
	{
		switch (Entry.Resolution)
		{
			case EDialogueConflictResolution::KeepUE:
				if (Entry.UERow.IsValid())
				{
					Result.MergedRows.Add(*Entry.UERow);
					if (Entry.Status == EDialogueSyncStatus::Unchanged)
					{
						Result.Unchanged++;
					}
					else
					{
						Result.AppliedFromUE++;
					}
				}
				break;

			case EDialogueConflictResolution::KeepExcel:
				if (Entry.ExcelRow.IsValid())
				{
					// v4.4 Phase 2: Partial Apply Logic
					// If tokens are invalid, apply text fields only, preserve UE's tokens
					if (!Entry.ExcelRow->AreTokensValid() && Entry.UERow.IsValid())
					{
						// Create merged row: Excel text + UE tokens
						FDialogueTableRow MergedRow = *Entry.ExcelRow;

						// Preserve UE's token fields if Excel's are invalid
						if (!Entry.ExcelRow->bEventsValid)
						{
							MergedRow.EventsTokenStr = Entry.UERow->EventsTokenStr;
							MergedRow.bEventsValid = true;  // UE's tokens are assumed valid
							MergedRow.EventsValidationError.Empty();
						}
						if (!Entry.ExcelRow->bConditionsValid)
						{
							MergedRow.ConditionsTokenStr = Entry.UERow->ConditionsTokenStr;
							MergedRow.bConditionsValid = true;
							MergedRow.ConditionsValidationError.Empty();
						}

						Result.MergedRows.Add(MergedRow);
						Result.AppliedFromExcel++;  // Still counts as Excel apply (partial)
					}
					else
					{
						// All tokens valid OR no UE row to preserve from - apply as-is
						Result.MergedRows.Add(*Entry.ExcelRow);
						Result.AppliedFromExcel++;
					}
				}
				break;

			case EDialogueConflictResolution::KeepBoth:
				// Add both rows (for additions that should be kept)
				if (Entry.UERow.IsValid())
				{
					Result.MergedRows.Add(*Entry.UERow);
					Result.AppliedFromUE++;
				}
				if (Entry.ExcelRow.IsValid())
				{
					// Generate new RowId to avoid duplicate
					FDialogueTableRow ExcelCopy = *Entry.ExcelRow;
					ExcelCopy.RowId = FGuid::NewGuid();
					Result.MergedRows.Add(ExcelCopy);
					Result.AppliedFromExcel++;
				}
				break;

			case EDialogueConflictResolution::Delete:
				Result.Deleted++;
				break;

			case EDialogueConflictResolution::Unresolved:
				// Should not happen if AllConflictsResolved was checked
				// Default to keeping UE version
				if (Entry.UERow.IsValid())
				{
					Result.MergedRows.Add(*Entry.UERow);
					Result.AppliedFromUE++;
				}
				else if (Entry.ExcelRow.IsValid())
				{
					Result.MergedRows.Add(*Entry.ExcelRow);
					Result.AppliedFromExcel++;
				}
				break;
		}
	}

	return Result;
}

//=============================================================================
// Phase 4: Apply Tokens to Assets
//=============================================================================

FDialogueAssetApplySummary FDialogueXLSXSyncEngine::ApplyTokensToAssets(
	const TArray<FDialogueTableRow>& Rows,
	const FString& DialogueAssetPath)
{
	FDialogueAssetApplySummary Summary;

#if WITH_EDITOR
	// Group rows by DialogueID
	TMap<FName, TArray<FDialogueTableRow>> RowsByDialogue;
	for (const FDialogueTableRow& Row : Rows)
	{
		// Only include rows that have tokens to apply
		if (!Row.EventsTokenStr.IsEmpty() || !Row.ConditionsTokenStr.IsEmpty())
		{
			RowsByDialogue.FindOrAdd(Row.DialogueID).Add(Row);
		}
	}

	if (RowsByDialogue.Num() == 0)
	{
		Summary.bSuccess = true;
		Summary.ErrorMessage = TEXT("No tokens to apply");
		return Summary;
	}

	// Get asset registry for finding dialogue assets
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Process each dialogue
	for (const auto& Pair : RowsByDialogue)
	{
		const FName& DialogueID = Pair.Key;
		const TArray<FDialogueTableRow>& DialogueRows = Pair.Value;

		Summary.AssetsProcessed++;

		// Find the dialogue asset
		// Search for DialogueBlueprint with matching name
		FARFilter Filter;
		Filter.ClassPaths.Add(UDialogueBlueprint::StaticClass()->GetClassPathName());
		Filter.PackagePaths.Add(FName(*DialogueAssetPath));
		Filter.bRecursivePaths = true;

		TArray<FAssetData> AssetDataList;
		AssetRegistry.GetAssets(Filter, AssetDataList);

		// Find matching asset by name
		UDialogueBlueprint* DialogueBlueprint = nullptr;
		for (const FAssetData& AssetData : AssetDataList)
		{
			if (AssetData.AssetName == DialogueID)
			{
				DialogueBlueprint = Cast<UDialogueBlueprint>(AssetData.GetAsset());
				break;
			}
		}

		if (!DialogueBlueprint)
		{
			// Try loading directly by common path patterns
			TArray<FString> SearchPaths = {
				FString::Printf(TEXT("%s%s.%s"), *DialogueAssetPath, *DialogueID.ToString(), *DialogueID.ToString()),
				FString::Printf(TEXT("%sDialogues/%s.%s"), *DialogueAssetPath, *DialogueID.ToString(), *DialogueID.ToString()),
				FString::Printf(TEXT("/Game/Dialogues/%s.%s"), *DialogueID.ToString(), *DialogueID.ToString()),
				FString::Printf(TEXT("/Game/%s.%s"), *DialogueID.ToString(), *DialogueID.ToString())
			};

			for (const FString& Path : SearchPaths)
			{
				DialogueBlueprint = LoadObject<UDialogueBlueprint>(nullptr, *Path);
				if (DialogueBlueprint)
				{
					break;
				}
			}
		}

		if (!DialogueBlueprint)
		{
			FString ErrorMsg = FString::Printf(TEXT("Could not find dialogue asset: %s"), *DialogueID.ToString());
			Summary.AssetResults.Add(DialogueID, ErrorMsg);
			UE_LOG(LogTemp, Warning, TEXT("DialogueXLSXSyncEngine: %s"), *ErrorMsg);
			continue;
		}

		// Apply tokens to the dialogue
		FDialogueAssetApplyResult ApplyResult = FDialogueAssetSync::ApplyTokensToAsset(DialogueBlueprint, DialogueRows);

		// Accumulate results
		if (ApplyResult.NodesUpdated > 0)
		{
			Summary.AssetsModified++;
		}
		Summary.TotalNodesUpdated += ApplyResult.NodesUpdated;
		Summary.TotalEventsApplied += ApplyResult.EventsApplied;
		Summary.TotalConditionsApplied += ApplyResult.ConditionsApplied;
		Summary.TotalEventsCleared += ApplyResult.EventsCleared;
		Summary.TotalConditionsCleared += ApplyResult.ConditionsCleared;
		Summary.TotalNodesSkipped += ApplyResult.NodesSkippedValidation;
		Summary.TotalNodesNotFound += ApplyResult.NodesNotFound;

		// Record per-asset result
		FString ResultStr = FString::Printf(TEXT("%d nodes, %d events, %d conditions"),
			ApplyResult.NodesUpdated, ApplyResult.EventsApplied, ApplyResult.ConditionsApplied);
		Summary.AssetResults.Add(DialogueID, ResultStr);
	}

	Summary.bSuccess = true;

	UE_LOG(LogTemp, Log, TEXT("DialogueXLSXSyncEngine: Applied tokens to %d assets (%d modified), %d nodes updated, %d events, %d conditions"),
		Summary.AssetsProcessed, Summary.AssetsModified, Summary.TotalNodesUpdated,
		Summary.TotalEventsApplied, Summary.TotalConditionsApplied);

#else
	Summary.ErrorMessage = TEXT("ApplyTokensToAssets requires WITH_EDITOR");
#endif

	return Summary;
}
