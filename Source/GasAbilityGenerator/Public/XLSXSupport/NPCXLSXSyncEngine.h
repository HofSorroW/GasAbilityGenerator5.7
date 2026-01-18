// GasAbilityGenerator - NPC XLSX Sync Engine
// v4.4: 3-way merge for Excel ↔ UE synchronization
//
// Sync flow:
// 1. Export creates base snapshot with per-row hashes
// 2. User edits in Excel (may add/delete/modify rows)
// 3. Sync compares: Base (from export) vs UE (current) vs Excel (imported)
// 4. Detects conflicts, presents resolution UI
// 5. Applies merged changes
//
// This prevents silent data loss by:
// - Detecting when both Excel and UE changed the same row (conflict)
// - Requiring explicit user resolution for conflicts
// - Tracking deletions explicitly (#STATE column)

#pragma once

#include "CoreMinimal.h"
#include "NPCTableEditor/NPCTableEditorTypes.h"

/**
 * Row sync status - result of 3-way comparison
 * Determines what action should be taken for each row
 */
enum class ENPCSyncStatus : uint8
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
enum class ENPCConflictResolution : uint8
{
	Unresolved,         // User hasn't decided yet
	KeepUE,             // Use UE version
	KeepExcel,          // Use Excel version
	KeepBoth,           // Add both as separate rows (generates new GUIDs)
	Delete              // Remove the row entirely
};

/**
 * v4.12.2: Validation status for sync entries
 */
enum class ENPCSyncValidationStatus : uint8
{
	Valid,          // No validation issues
	Warning,        // Has warnings but can proceed
	Error           // Has errors, cannot generate
};

/**
 * Single row sync entry with comparison data
 * Contains all versions of the row and computed hashes
 */
struct FNPCSyncEntry
{
	FGuid RowId;                                   // Stable row identifier
	ENPCSyncStatus Status = ENPCSyncStatus::Unchanged;
	ENPCConflictResolution Resolution = ENPCConflictResolution::Unresolved;

	// Row versions (nullptr if doesn't exist in that source)
	TSharedPtr<FNPCTableRow> BaseRow;              // From last export snapshot
	TSharedPtr<FNPCTableRow> UERow;                // Current in editor
	TSharedPtr<FNPCTableRow> ExcelRow;             // From imported file

	// Hash values for content comparison (excludes RowId)
	int64 BaseHash = 0;
	int64 UEHash = 0;
	int64 ExcelHash = 0;

	// v4.12.2: Validation status for the Excel row (source being imported)
	ENPCSyncValidationStatus ValidationStatus = ENPCSyncValidationStatus::Valid;
	TArray<FString> ValidationMessages;            // Human-readable validation messages

	//-------------------------------------------------------------------------
	// Display helpers
	//-------------------------------------------------------------------------

	/** Get human-readable status text */
	FString GetStatusText() const;

	/** Get color for status indicator */
	FLinearColor GetStatusColor() const;

	/** Check if this entry needs user resolution before merge */
	bool RequiresResolution() const;

	/** Get NPC name for display (from whichever version exists) */
	FString GetDisplayName() const;
};

/**
 * Overall sync operation result
 * Contains all entries and summary statistics
 */
struct FNPCSyncResult
{
	bool bSuccess = false;
	FString ErrorMessage;

	TArray<FNPCSyncEntry> Entries;

	// Summary statistics
	int32 UnchangedCount = 0;
	int32 ModifiedInUECount = 0;
	int32 ModifiedInExcelCount = 0;
	int32 ConflictCount = 0;
	int32 AddedInUECount = 0;
	int32 AddedInExcelCount = 0;
	int32 DeletedCount = 0;

	// v4.12.2: Validation statistics
	int32 ValidationErrorCount = 0;
	int32 ValidationWarningCount = 0;

	/** Check if there are any conflicts requiring resolution */
	bool HasConflicts() const { return ConflictCount > 0; }

	/** v4.12.2: Check if there are validation errors */
	bool HasValidationErrors() const { return ValidationErrorCount > 0; }

	/** Check if there are any changes to apply */
	bool HasChanges() const;

	/** Get total number of changes (adds + modifies + deletes) */
	int32 GetTotalChanges() const;

	/** Get entries that are conflicts */
	TArray<FNPCSyncEntry*> GetConflicts();

	/** Get entries that require user resolution (conflicts + delete conflicts) */
	TArray<FNPCSyncEntry*> GetEntriesRequiringResolution();
};

/**
 * Merged result ready to apply
 */
struct FNPCMergeResult
{
	TArray<FNPCTableRow> MergedRows;   // Final rows to replace current data
	int32 AppliedFromUE = 0;           // Rows kept from UE version
	int32 AppliedFromExcel = 0;        // Rows applied from Excel
	int32 Deleted = 0;                 // Rows removed
	int32 Unchanged = 0;               // Rows with no changes
};

/**
 * Summary of applying table rows to NPC assets (v4.5)
 */
struct FNPCAssetApplySummary
{
	bool bSuccess = false;
	FString ErrorMessage;

	/** Number of NPC assets processed */
	int32 AssetsProcessed = 0;

	/** Number of assets modified */
	int32 AssetsModified = 0;

	/** Number of assets created (new) */
	int32 AssetsCreated = 0;

	/** Assets skipped (not modified) */
	int32 AssetsSkippedNotModified = 0;

	/** Assets skipped (validation errors) */
	int32 AssetsSkippedValidation = 0;

	/** Assets skipped (no asset reference) */
	int32 AssetsSkippedNoAsset = 0;

	/** Assets skipped (read-only/plugin content) */
	int32 AssetsSkippedReadOnly = 0;

	/** NPCs that failed to save */
	TArray<FString> FailedNPCs;

	/** Per-asset results for detailed reporting */
	TMap<FString, FString> AssetResults;  // NPCName -> result summary
};

/**
 * 3-way merge engine for NPC table sync
 *
 * Usage:
 * 1. Call CompareSources() with base, UE, and Excel rows
 * 2. Present conflicts to user via UI
 * 3. Set Resolution on conflict entries
 * 4. Call ApplySync() to get merged rows
 */
class GASABILITYGENERATOR_API FNPCXLSXSyncEngine
{
public:
	/**
	 * Perform 3-way comparison between base, UE, and Excel versions
	 * @param BaseRows - Rows from last export (empty array if first sync)
	 * @param UERows - Current rows in editor
	 * @param ExcelRows - Rows imported from Excel file
	 * @return Sync result with all entries and their status
	 */
	static FNPCSyncResult CompareSources(
		const TArray<FNPCTableRow>& BaseRows,
		const TArray<FNPCTableRow>& UERows,
		const TArray<FNPCTableRow>& ExcelRows
	);

	/**
	 * Apply sync result to produce merged rows
	 * All conflicts must be resolved before calling (check AllConflictsResolved)
	 * @param SyncResult - Result from CompareSources with resolutions set
	 * @return Merged rows ready to replace current table data
	 */
	static FNPCMergeResult ApplySync(const FNPCSyncResult& SyncResult);

	/**
	 * Apply validated table rows to NPCDefinition assets (v4.5)
	 * Uses AssetRegistry to find existing assets, falls back to path patterns
	 * @param Rows - Rows containing data to apply (typically from MergedRows)
	 * @param NPCAssetPath - Base path to search for NPC assets (e.g., "/Game/NPCs")
	 * @param bCreateMissing - If true, create new assets for rows without GeneratedNPCDef
	 * @return Summary of apply operation across all assets
	 */
	static FNPCAssetApplySummary ApplyToAssets(
		TArray<FNPCTableRow>& Rows,
		const FString& NPCAssetPath = TEXT("/Game/"),
		bool bCreateMissing = false
	);

	/**
	 * Auto-resolve entries that don't require user input
	 * Sets resolution for non-conflict entries based on their status
	 */
	static void AutoResolveNonConflicts(FNPCSyncResult& SyncResult);

	/**
	 * Check if all conflicts have been resolved
	 * @return true if ApplySync can be safely called
	 */
	static bool AllConflictsResolved(const FNPCSyncResult& SyncResult);

	/**
	 * Compute content hash for a row (excludes RowId for identity-independent comparison)
	 * Same algorithm as writer for consistency
	 */
	static int64 ComputeRowHash(const FNPCTableRow& Row);

private:
	//-------------------------------------------------------------------------
	// Internal helpers
	//-------------------------------------------------------------------------

	/** Build lookup map from rows by RowId for O(1) access */
	static TMap<FGuid, const FNPCTableRow*> BuildRowMap(const TArray<FNPCTableRow>& Rows);

	/** Determine sync status from presence and hash comparison */
	static ENPCSyncStatus DetermineStatus(
		const FNPCTableRow* BaseRow,
		const FNPCTableRow* UERow,
		const FNPCTableRow* ExcelRow,
		int64 BaseHash, int64 UEHash, int64 ExcelHash
	);
};
