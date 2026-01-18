// GasAbilityGenerator - Quest Asset Sync
// v4.12: Bidirectional sync between UQuest assets and Quest table rows
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QuestTableEditor/QuestTableEditorTypes.h"

/**
 * Summary of apply operation results
 */
struct FQuestAssetApplySummary
{
	int32 AssetsProcessed = 0;
	int32 AssetsCreated = 0;
	int32 AssetsModified = 0;
	int32 AssetsSkippedNotModified = 0;
	int32 AssetsSkippedValidation = 0;
	int32 AssetsSkippedNoAsset = 0;
	int32 AssetsSkippedReadOnly = 0;
	TArray<FString> FailedQuests;
	TArray<FString> ReadOnlyQuests;
	FString ErrorMessage;
	bool bSuccess = false;
};

/**
 * Data extracted from a UQuest asset for sync
 */
struct FQuestAssetData
{
	bool bFoundInAsset = false;
	FString QuestName;
	FString DisplayName;
	bool bIsTracked = false;
	TArray<FString> StateIDs;
	TArray<EQuestStateType> StateTypes;
	TArray<FString> StateDescriptions;
};

/**
 * Result of syncing from all Quest assets
 */
struct FQuestAssetSyncResult
{
	bool bSuccess = false;
	FString ErrorMessage;
	int32 QuestsFound = 0;
	TMap<FString, FQuestAssetData> QuestData;  // Key = Quest asset name
};

/**
 * Quest Asset Sync - bidirectional sync between UQuest assets and table rows
 *
 * Pattern follows NPCAssetSync:
 * - SyncFromAsset: Read quest data into struct
 * - SyncFromAllAssets: Find all quest assets and read them
 * - ApplyToAssets: Write table rows back to quest assets via generation
 */
class GASABILITYGENERATOR_API FQuestAssetSync
{
public:
	/**
	 * Read quest data from a single UQuest asset
	 * @param QuestBlueprint - The quest blueprint to read from
	 * @return Extracted quest data
	 */
	static FQuestAssetData SyncFromAsset(class UQuestBlueprint* QuestBlueprint);

	/**
	 * Find and sync all UQuest assets in the project
	 * @return Result containing all quest data
	 */
	static FQuestAssetSyncResult SyncFromAllAssets();

	/**
	 * Apply table rows to Quest assets (regenerate)
	 * Groups rows by QuestName, converts to manifest definitions, and generates
	 * @param Rows - Quest table rows to apply
	 * @param OutputFolder - Output folder for generated assets
	 * @param bCreateMissing - If true, create new assets for rows without existing assets
	 * @return Summary of apply results
	 */
	static FQuestAssetApplySummary ApplyToAssets(
		TArray<FQuestTableRow>& Rows,
		const FString& OutputFolder,
		bool bCreateMissing = false);

	/**
	 * Populate table rows from sync result
	 * @param Rows - Rows to update
	 * @param SyncResult - Result from SyncFromAllAssets
	 * @return Number of rows updated
	 */
	static int32 PopulateRowsFromAssets(
		TArray<FQuestTableRow>& Rows,
		const FQuestAssetSyncResult& SyncResult);

private:
	/** Check if asset package is writable */
	static bool IsAssetWritable(UObject* Asset);

	/** Build manifest quest definition from grouped rows */
	static struct FManifestQuestDefinition BuildQuestDefinition(
		const FString& QuestName,
		const TArray<FQuestTableRow*>& QuestRows,
		const FString& OutputFolder);
};
