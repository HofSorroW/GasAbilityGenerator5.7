// GasAbilityGenerator - Quest XLSX Sync Engine
// v4.12: 3-way merge for Excel ↔ UE synchronization (following NPC pattern)
//
// Sync flow:
// 1. Export creates base snapshot with per-row hashes
// 2. User edits in Excel (may add/delete/modify rows)
// 3. Sync compares: Base (from export) vs UE (current) vs Excel (imported)
// 4. Detects conflicts, presents resolution UI
// 5. Applies merged changes
//
// Per Table_Editors_Reference.md v4.11:
// - Only Case 1 (Unchanged) auto-resolves
// - ALL 13 other cases require explicit user approval

#pragma once

#include "CoreMinimal.h"
#include "QuestTableEditor/QuestTableEditorTypes.h"

/**
 * Row sync status - result of 3-way comparison
 * Determines what action should be taken for each row
 */
enum class EQuestSyncStatus : uint8
{
	Unchanged,          // Base == UE == Excel (no action needed)
	ModifiedInUE,       // Base != UE, Base == Excel (keep UE changes)
	ModifiedInExcel,    // Base == UE, Base != Excel (apply Excel changes)
	Conflict,           // Base != UE && Base != Excel (user must resolve)
	AddedInUE,          // Not in Base, exists in UE only
	AddedInExcel,       // Not in Base, exists in Excel only
	DeletedInUE,        // In Base & Excel, missing in UE
	DeletedInExcel,     // In Base & UE, explicitly deleted in Excel (#STATE=Deleted)
	DeleteConflict      // Deleted in one source, modified in other
};

/**
 * User's resolution choice for a conflict
 */
enum class EQuestConflictResolution : uint8
{
	Unresolved,         // User hasn't decided yet
	KeepUE,             // Use UE version
	KeepExcel,          // Use Excel version
	KeepBoth,           // Add both as separate rows (generates new GUIDs)
	Delete              // Remove the row entirely
};

/**
 * Single row sync entry with comparison data
 * Contains all versions of the row and computed hashes
 */
struct FQuestSyncEntry
{
	FGuid RowId;                                   // Stable row identifier
	EQuestSyncStatus Status = EQuestSyncStatus::Unchanged;
	EQuestConflictResolution Resolution = EQuestConflictResolution::Unresolved;

	// Row versions (nullptr if doesn't exist in that source)
	TSharedPtr<FQuestTableRow> BaseRow;            // From last export snapshot
	TSharedPtr<FQuestTableRow> UERow;              // Current in editor
	TSharedPtr<FQuestTableRow> ExcelRow;           // From imported file

	// Hash values for content comparison (excludes RowId)
	int64 BaseHash = 0;
	int64 UEHash = 0;
	int64 ExcelHash = 0;

	//-------------------------------------------------------------------------
	// Display helpers
	//-------------------------------------------------------------------------

	/** Get human-readable status text */
	FString GetStatusText() const;

	/** Get color for status indicator */
	FLinearColor GetStatusColor() const;

	/** Check if this entry needs user resolution before merge */
	bool RequiresResolution() const;

	/** Get quest name for display (from whichever version exists) */
	FString GetDisplayName() const;
};

/**
 * Overall sync operation result
 * Contains all entries and summary statistics
 */
struct FQuestSyncResult
{
	bool bSuccess = false;
	FString ErrorMessage;

	TArray<FQuestSyncEntry> Entries;

	// Summary statistics
	int32 UnchangedCount = 0;
	int32 ModifiedInUECount = 0;
	int32 ModifiedInExcelCount = 0;
	int32 ConflictCount = 0;
	int32 AddedInUECount = 0;
	int32 AddedInExcelCount = 0;
	int32 DeletedCount = 0;

	/** Check if there are any conflicts requiring resolution */
	bool HasConflicts() const { return ConflictCount > 0; }

	/** Check if there are any changes to apply */
	bool HasChanges() const;

	/** Get total number of changes (adds + modifies + deletes) */
	int32 GetTotalChanges() const;

	/** Get entries that are conflicts */
	TArray<FQuestSyncEntry*> GetConflicts();

	/** Get entries that require user resolution (conflicts + delete conflicts) */
	TArray<FQuestSyncEntry*> GetEntriesRequiringResolution();
};

/**
 * Merged result ready to apply
 */
struct FQuestMergeResult
{
	TArray<FQuestTableRow> MergedRows;   // Final rows to replace current data
	int32 AppliedFromUE = 0;             // Rows kept from UE version
	int32 AppliedFromExcel = 0;          // Rows applied from Excel
	int32 Deleted = 0;                   // Rows removed
	int32 Unchanged = 0;                 // Rows with no changes
};

/**
 * 3-way merge engine for Quest table sync
 *
 * Usage:
 * 1. Call CompareSources() with base, UE, and Excel rows
 * 2. Present conflicts to user via UI
 * 3. Set Resolution on conflict entries
 * 4. Call ApplySync() to get merged rows
 */
class GASABILITYGENERATOR_API FQuestXLSXSyncEngine
{
public:
	/**
	 * Perform 3-way comparison between base, UE, and Excel versions
	 * @param BaseRows - Rows from last export (empty array if first sync)
	 * @param UERows - Current rows in editor
	 * @param ExcelRows - Rows imported from Excel file
	 * @return Sync result with all entries and their status
	 */
	static FQuestSyncResult CompareSources(
		const TArray<FQuestTableRow>& BaseRows,
		const TArray<FQuestTableRow>& UERows,
		const TArray<FQuestTableRow>& ExcelRows
	);

	/**
	 * Apply sync result to produce merged rows
	 * All conflicts must be resolved before calling (check AllConflictsResolved)
	 * @param SyncResult - Result from CompareSources with resolutions set
	 * @return Merged rows ready to replace current table data
	 */
	static FQuestMergeResult ApplySync(const FQuestSyncResult& SyncResult);

	/**
	 * Auto-resolve entries that don't require user input
	 * v4.11: Only Unchanged auto-resolves. All other statuses require explicit approval.
	 */
	static void AutoResolveNonConflicts(FQuestSyncResult& SyncResult);

	/**
	 * Check if all conflicts have been resolved
	 * @return true if ApplySync can be safely called
	 */
	static bool AllConflictsResolved(const FQuestSyncResult& SyncResult);

	/**
	 * Compute content hash for a row (excludes RowId for identity-independent comparison)
	 * Same algorithm as writer for consistency
	 */
	static int64 ComputeRowHash(const FQuestTableRow& Row);

private:
	//-------------------------------------------------------------------------
	// Internal helpers
	//-------------------------------------------------------------------------

	/** Build lookup map from rows by RowId for O(1) access */
	static TMap<FGuid, const FQuestTableRow*> BuildRowMap(const TArray<FQuestTableRow>& Rows);

	/** Determine sync status from presence and hash comparison */
	static EQuestSyncStatus DetermineStatus(
		const FQuestTableRow* BaseRow,
		const FQuestTableRow* UERow,
		const FQuestTableRow* ExcelRow,
		int64 BaseHash, int64 UEHash, int64 ExcelHash
	);
};
