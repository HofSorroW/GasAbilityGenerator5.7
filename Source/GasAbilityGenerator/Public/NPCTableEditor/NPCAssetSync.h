// GasAbilityGenerator - NPC Asset Sync
// v4.5: Bidirectional sync between UNPCDefinition assets and NPC table rows
//
// Sync: Extract NPC data from existing assets for table population
// Apply: Write validated table row changes back to NPCDefinition assets

#pragma once

#include "CoreMinimal.h"
#include "NPCTableEditorTypes.h"

class UNPCDefinition;

/**
 * Per-NPC data extracted from UNPCDefinition asset
 */
struct FNPCAssetData
{
	/** NPC ID from asset */
	FString NPCId;

	/** Display name from asset */
	FString DisplayName;

	/** Blueprint class path */
	FSoftObjectPath Blueprint;

	/** Ability configuration */
	FSoftObjectPath AbilityConfig;

	/** Activity configuration */
	FSoftObjectPath ActivityConfig;

	/** Activity schedules (first one if multiple) */
	FSoftObjectPath Schedule;

	/** Min/Max levels */
	int32 MinLevel = 1;
	int32 MaxLevel = 10;

	/** Attack priority */
	float AttackPriority = 0.5f;

	/** Factions as comma-separated short names */
	FString Factions;

	/** Vendor settings */
	bool bIsVendor = false;
	FString ShopName;

	/** Appearance */
	FSoftObjectPath Appearance;

	/** Default items as comma-separated collection names (from DefaultItemLoadout) */
	FString DefaultItems;

	/** Whether this NPC was found in assets */
	bool bFoundInAsset = false;

	FNPCAssetData() = default;
};

/**
 * Result from syncing NPC assets to table rows
 */
struct FNPCAssetSyncResult
{
	/** Whether the sync operation succeeded */
	bool bSuccess = false;

	/** Error message if sync failed */
	FString ErrorMessage;

	/** Map from NPCName to asset data */
	TMap<FString, FNPCAssetData> NPCData;

	/** Total NPCs found */
	int32 NPCsFound = 0;

	/** NPCs with vendors enabled */
	int32 VendorNPCs = 0;

	/** NPCs with ability configs */
	int32 NPCsWithAbilityConfig = 0;
};

/**
 * Result from applying table rows to NPC assets
 */
struct FNPCAssetApplyResult
{
	/** Whether the apply operation succeeded */
	bool bSuccess = false;

	/** Error message if apply failed */
	FString ErrorMessage;

	/** Number of NPCs updated */
	int32 NPCsUpdated = 0;

	/** Number of NPCs created (new) */
	int32 NPCsCreated = 0;

	/** Number of NPCs skipped (not modified) */
	int32 NPCsSkippedNotModified = 0;

	/** Number of NPCs skipped (validation errors) */
	int32 NPCsSkippedValidation = 0;

	/** Number of NPCs skipped (no asset reference) */
	int32 NPCsSkippedNoAsset = 0;

	/** Number of NPCs skipped (read-only/plugin content) */
	int32 NPCsSkippedReadOnly = 0;

	/** NPCs that failed to save */
	TArray<FString> FailedNPCs;

	/** Read-only NPCs that were skipped */
	TArray<FString> ReadOnlyNPCs;

	/** Get total skipped count */
	int32 GetTotalSkipped() const
	{
		return NPCsSkippedNotModified + NPCsSkippedValidation + NPCsSkippedNoAsset + NPCsSkippedReadOnly;
	}
};

/**
 * Bidirectional sync between UNPCDefinition assets and NPC table rows
 *
 * Sync (Read): Extract NPC data from existing assets
 * - SyncFromAssets() scans all NPCDefinition assets and returns their data
 * - Used to populate table rows from existing UE assets
 *
 * Apply (Write): Write validated table row changes back to assets
 * - ApplyToAssets() writes validated rows back to UNPCDefinition assets
 * - Validates rows before apply (partial apply on validation errors)
 * - Supports creating new assets or updating existing ones
 *
 * Usage:
 * 1. SyncFromAssets() - Read current UE state into table rows
 * 2. User edits rows in table editor
 * 3. ApplyToAssets() - Write validated changes back to UE
 */
class GASABILITYGENERATOR_API FNPCAssetSync
{
public:
	/**
	 * Extract data from a single NPCDefinition asset
	 * @param NPCDef - The NPC asset to read from
	 * @return Asset data structure
	 */
	static FNPCAssetData SyncFromAsset(UNPCDefinition* NPCDef);

	/**
	 * Extract data from all NPCDefinition assets in the project
	 * @return Result containing all NPC data
	 */
	static FNPCAssetSyncResult SyncFromAllAssets();

	/**
	 * Populate table rows from asset sync data
	 * @param Rows - Rows to update (modified in place)
	 * @param SyncResult - Asset sync result containing current UE state
	 * @return Number of rows updated
	 */
	static int32 PopulateRowsFromAssets(TArray<FNPCTableRow>& Rows, const FNPCAssetSyncResult& SyncResult);

	//=========================================================================
	// Apply: Write table rows to NPC assets
	//=========================================================================

	/**
	 * Apply validated table rows to NPCDefinition assets
	 * @param Rows - Rows containing data to apply
	 * @param bCreateMissing - If true, creates new assets for rows without GeneratedNPCDef
	 * @param OutputFolder - Folder for new assets (default: /Game/NPCs)
	 * @return Result with counts of applied/skipped NPCs
	 */
	static FNPCAssetApplyResult ApplyToAssets(
		TArray<FNPCTableRow>& Rows,
		bool bCreateMissing = false,
		const FString& OutputFolder = TEXT("/Game/NPCs"));

	/**
	 * Apply a single table row to an NPCDefinition asset
	 * @param Row - Row containing data to apply
	 * @param NPCDef - The NPC asset to modify
	 * @return true if apply succeeded
	 */
	static bool ApplyRowToAsset(const FNPCTableRow& Row, UNPCDefinition* NPCDef);

private:
	/**
	 * Check if an asset is in a writable location
	 * @param Asset - Asset to check
	 * @return true if asset can be modified
	 */
	static bool IsAssetWritable(UObject* Asset);

	/**
	 * Save an NPCDefinition asset to disk
	 * @param NPCDef - Asset to save
	 * @return true if save succeeded
	 */
	static bool SaveAsset(UNPCDefinition* NPCDef);

	/**
	 * Convert faction gameplay tags to comma-separated short names
	 * @param Tags - Gameplay tag container
	 * @return Comma-separated faction short names
	 */
	static FString FactionsToString(const struct FGameplayTagContainer& Tags);

	/**
	 * Convert comma-separated faction names to gameplay tag container
	 * @param FactionsStr - Comma-separated faction names
	 * @param OutTags - Output tag container
	 */
	static void StringToFactions(const FString& FactionsStr, struct FGameplayTagContainer& OutTags);
};
