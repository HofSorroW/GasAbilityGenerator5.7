// GasAbilityGenerator - Dialogue XLSX Sync Engine
// v4.3: 3-way merge for Excel ↔ UE synchronization
//
// Sync flow:
// 1. Export creates base snapshot with per-row hashes
// 2. User edits in Excel (may add/delete/modify rows)
// 3. Sync compares: Base (from export) vs UE (current) vs Excel (imported)
// 4. Detects conflicts, presents resolution UI
// 5. Applies merged changes

#pragma once

#include "CoreMinimal.h"
#include "DialogueTableEditorTypes.h"

/**
 * Row sync status - result of 3-way comparison
 */
enum class EDialogueSyncStatus : uint8
{
	Unchanged,          // Base == UE == Excel (no action needed)
	ModifiedInUE,       // Base != UE, Base == Excel (keep UE changes)
	ModifiedInExcel,    // Base == UE, Base != Excel (apply Excel changes)
	Conflict,           // Base != UE && Base != Excel (user must resolve)
	AddedInUE,          // Not in Base, exists in UE only
	AddedInExcel,       // Not in Base, exists in Excel only
	DeletedInUE,        // In Base & Excel, missing in UE
	DeletedInExcel,     // In Base & UE, missing in Excel
	DeleteConflict      // Deleted in one, modified in other
};

/**
 * User's resolution choice for a conflict
 */
enum class EDialogueConflictResolution : uint8
{
	Unresolved,         // User hasn't decided yet
	KeepUE,             // Use UE version
	KeepExcel,          // Use Excel version
	KeepBoth,           // Add both as separate rows (for additions)
	Delete              // Remove the row
};

/**
 * Single row sync entry with comparison data
 */
struct FDialogueSyncEntry
{
	FGuid RowId;                              // Stable identifier
	EDialogueSyncStatus Status = EDialogueSyncStatus::Unchanged;
	EDialogueConflictResolution Resolution = EDialogueConflictResolution::Unresolved;

	// Row versions (nullptr if doesn't exist in that source)
	TSharedPtr<FDialogueTableRow> BaseRow;    // From last export
	TSharedPtr<FDialogueTableRow> UERow;      // Current in editor
	TSharedPtr<FDialogueTableRow> ExcelRow;   // From imported file

	// Hash values for comparison
	int64 BaseHash = 0;
	int64 UEHash = 0;
	int64 ExcelHash = 0;

	// For display
	FString GetStatusText() const;
	FLinearColor GetStatusColor() const;
	bool RequiresResolution() const;
};

/**
 * Overall sync operation result
 */
struct FDialogueSyncResult
{
	bool bSuccess = false;
	FString ErrorMessage;

	TArray<FDialogueSyncEntry> Entries;

	// Stats
	int32 UnchangedCount = 0;
	int32 ModifiedInUECount = 0;
	int32 ModifiedInExcelCount = 0;
	int32 ConflictCount = 0;
	int32 AddedInUECount = 0;
	int32 AddedInExcelCount = 0;
	int32 DeletedCount = 0;

	bool HasConflicts() const { return ConflictCount > 0; }
	bool HasChanges() const;
	int32 GetTotalChanges() const;

	// Get entries by status
	TArray<FDialogueSyncEntry*> GetConflicts();
	TArray<FDialogueSyncEntry*> GetEntriesRequiringResolution();
};

/**
 * Merged result ready to apply
 */
struct FDialogueMergeResult
{
	TArray<FDialogueTableRow> MergedRows;
	int32 AppliedFromUE = 0;
	int32 AppliedFromExcel = 0;
	int32 Deleted = 0;
	int32 Unchanged = 0;
};

/**
 * 3-way merge engine for dialogue table sync
 */
class GASABILITYGENERATOR_API FDialogueXLSXSyncEngine
{
public:
	/**
	 * Perform 3-way comparison between base, UE, and Excel versions
	 * @param BaseRows - Rows from last export (empty if first sync)
	 * @param UERows - Current rows in editor
	 * @param ExcelRows - Rows imported from Excel
	 * @return Sync result with all entries and their status
	 */
	static FDialogueSyncResult CompareSources(
		const TArray<FDialogueTableRow>& BaseRows,
		const TArray<FDialogueTableRow>& UERows,
		const TArray<FDialogueTableRow>& ExcelRows
	);

	/**
	 * Apply sync result to produce merged rows
	 * All conflicts must be resolved before calling
	 * @param SyncResult - Result from CompareSources with resolutions set
	 * @return Merged rows ready to replace current data
	 */
	static FDialogueMergeResult ApplySync(const FDialogueSyncResult& SyncResult);

	/**
	 * Auto-resolve non-conflicting entries
	 * Sets resolution for entries that don't require user input
	 */
	static void AutoResolveNonConflicts(FDialogueSyncResult& SyncResult);

	/**
	 * Check if all conflicts are resolved
	 */
	static bool AllConflictsResolved(const FDialogueSyncResult& SyncResult);

	/**
	 * Compute content hash for a row (excludes RowId)
	 */
	static int64 ComputeRowHash(const FDialogueTableRow& Row);

private:
	// Build lookup maps by RowId
	static TMap<FGuid, const FDialogueTableRow*> BuildRowMap(const TArray<FDialogueTableRow>& Rows);

	// Determine sync status from hash comparison
	static EDialogueSyncStatus DetermineStatus(
		const FDialogueTableRow* BaseRow,
		const FDialogueTableRow* UERow,
		const FDialogueTableRow* ExcelRow,
		int64 BaseHash, int64 UEHash, int64 ExcelHash
	);
};
