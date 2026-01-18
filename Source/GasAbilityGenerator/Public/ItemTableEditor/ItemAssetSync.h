// GasAbilityGenerator - Item Asset Sync
// v4.12: Bidirectional sync between UEquippableItem assets and Item table rows
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ItemTableEditor/ItemTableEditorTypes.h"

/**
 * Summary of apply operation results
 */
struct FItemAssetApplySummary
{
	int32 AssetsProcessed = 0;
	int32 AssetsCreated = 0;
	int32 AssetsModified = 0;
	int32 AssetsSkippedNotModified = 0;
	int32 AssetsSkippedValidation = 0;
	int32 AssetsSkippedNoAsset = 0;
	int32 AssetsSkippedReadOnly = 0;
	TArray<FString> FailedItems;
	TArray<FString> ReadOnlyItems;
	FString ErrorMessage;
	bool bSuccess = false;
};

/**
 * Data extracted from an item asset for sync
 */
struct FItemAssetData
{
	bool bFoundInAsset = false;
	FString ItemName;
	FString DisplayName;
	FString Description;
	int32 BaseValue = 0;
	float Weight = 1.0f;
	float AttackRating = 0.0f;
	float ArmorRating = 0.0f;
	float AttackDamage = 0.0f;
	FString EquipmentSlot;
	FString ItemClassName;  // To determine item type
	FString Abilities;      // Comma-separated GA_* ability class names (from EquipmentAbilities)
};

/**
 * Result of syncing from all Item assets
 */
struct FItemAssetSyncResult
{
	bool bSuccess = false;
	FString ErrorMessage;
	int32 ItemsFound = 0;
	int32 WeaponsFound = 0;
	int32 ArmorFound = 0;
	TMap<FString, FItemAssetData> ItemData;  // Key = Item asset name
};

/**
 * Item Asset Sync - bidirectional sync between item assets and table rows
 *
 * Pattern follows NPCAssetSync:
 * - SyncFromAsset: Read item data into struct
 * - SyncFromAllAssets: Find all item assets and read them
 * - ApplyToAssets: Write table rows back to item assets via generation
 */
class GASABILITYGENERATOR_API FItemAssetSync
{
public:
	/**
	 * Read item data from a single item asset
	 * @param ItemAsset - The item asset to read from
	 * @return Extracted item data
	 */
	static FItemAssetData SyncFromAsset(class UNarrativeItem* ItemAsset);

	/**
	 * Find and sync all item assets in the project
	 * @return Result containing all item data
	 */
	static FItemAssetSyncResult SyncFromAllAssets();

	/**
	 * Apply table rows to Item assets (regenerate)
	 * @param Rows - Item table rows to apply
	 * @param OutputFolder - Output folder for generated assets
	 * @param bCreateMissing - If true, create new assets for rows without existing assets
	 * @return Summary of apply results
	 */
	static FItemAssetApplySummary ApplyToAssets(
		TArray<FItemTableRow>& Rows,
		const FString& OutputFolder,
		bool bCreateMissing = false);

	/**
	 * Populate table rows from sync result
	 * @param Rows - Rows to update
	 * @param SyncResult - Result from SyncFromAllAssets
	 * @return Number of rows updated
	 */
	static int32 PopulateRowsFromAssets(
		TArray<FItemTableRow>& Rows,
		const FItemAssetSyncResult& SyncResult);

private:
	/** Check if asset package is writable */
	static bool IsAssetWritable(UObject* Asset);

	/** Build manifest item definition from row */
	static struct FManifestEquippableItemDefinition BuildItemDefinition(
		const FItemTableRow& Row,
		const FString& OutputFolder);

	/** Convert EItemType to parent class name */
	static FString GetParentClassName(EItemType ItemType);
};
