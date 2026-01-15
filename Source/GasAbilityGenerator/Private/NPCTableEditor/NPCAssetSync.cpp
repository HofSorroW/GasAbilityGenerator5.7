// GasAbilityGenerator - NPC Asset Sync Implementation
// v4.5: Bidirectional sync between UNPCDefinition assets and NPC table rows

#include "NPCTableEditor/NPCAssetSync.h"
#include "NPCTableEditor/NPCTableValidator.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"

// Narrative Pro includes
#include "AI/NPCDefinition.h"
#include "GAS/AbilityConfiguration.h"
#include "AI/Activities/NPCActivityConfiguration.h"
#include "AI/Activities/NPCActivitySchedule.h"
#include "Items/InventoryComponent.h"  // UItemCollection
#include "Tales/TriggerSet.h"
#include "UnrealFramework/NarrativeNPCCharacter.h"
#endif

FNPCAssetData FNPCAssetSync::SyncFromAsset(UNPCDefinition* NPCDef)
{
	FNPCAssetData Data;

#if WITH_EDITOR
	if (!NPCDef)
	{
		return Data;
	}

	Data.bFoundInAsset = true;

	//=========================================================================
	// Core Identity
	//=========================================================================
	Data.NPCId = NPCDef->NPCID.ToString();
	Data.DisplayName = NPCDef->NPCName.ToString();

	// Blueprint - NPCClassPath
	if (!NPCDef->NPCClassPath.IsNull())
	{
		Data.Blueprint = FSoftObjectPath(NPCDef->NPCClassPath.ToString());
	}

	//=========================================================================
	// AI & Behavior
	//=========================================================================
	if (NPCDef->AbilityConfiguration)
	{
		Data.AbilityConfig = FSoftObjectPath(NPCDef->AbilityConfiguration);
	}
	if (!NPCDef->ActivityConfiguration.IsNull())
	{
		Data.ActivityConfig = NPCDef->ActivityConfiguration.ToSoftObjectPath();
	}
	// Schedule - from TriggerSets array first
	if (NPCDef->TriggerSets.Num() > 0 && !NPCDef->TriggerSets[0].IsNull())
	{
		Data.Schedule = NPCDef->TriggerSets[0].ToSoftObjectPath();
	}
	// Fallback: check ActivitySchedules
	else if (NPCDef->ActivitySchedules.Num() > 0 && !NPCDef->ActivitySchedules[0].IsNull())
	{
		Data.Schedule = NPCDef->ActivitySchedules[0].ToSoftObjectPath();
	}

	//=========================================================================
	// Combat
	//=========================================================================
	Data.MinLevel = NPCDef->MinLevel;
	Data.MaxLevel = NPCDef->MaxLevel;
	Data.AttackPriority = NPCDef->AttackPriority;
	Data.Factions = FactionsToString(NPCDef->DefaultFactions);

	//=========================================================================
	// Vendor
	//=========================================================================
	Data.bIsVendor = NPCDef->bIsVendor;
	Data.ShopName = NPCDef->ShopFriendlyName.ToString();

	//=========================================================================
	// Meta
	//=========================================================================
	if (!NPCDef->DefaultAppearance.IsNull())
	{
		Data.Appearance = NPCDef->DefaultAppearance.ToSoftObjectPath();
	}

#endif

	return Data;
}

FNPCAssetSyncResult FNPCAssetSync::SyncFromAllAssets()
{
	FNPCAssetSyncResult Result;

#if WITH_EDITOR
	// Get Asset Registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Find all NPCDefinition assets
	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByClass(UNPCDefinition::StaticClass()->GetClassPathName(), AssetList, true);

	for (const FAssetData& AssetData : AssetList)
	{
		UNPCDefinition* NPCDef = Cast<UNPCDefinition>(AssetData.GetAsset());
		if (!NPCDef)
		{
			continue;
		}

		FString NPCName = AssetData.AssetName.ToString();
		FNPCAssetData AssetDataStruct = SyncFromAsset(NPCDef);

		Result.NPCData.Add(NPCName, AssetDataStruct);
		Result.NPCsFound++;

		if (AssetDataStruct.bIsVendor)
		{
			Result.VendorNPCs++;
		}
		if (!AssetDataStruct.AbilityConfig.IsNull())
		{
			Result.NPCsWithAbilityConfig++;
		}
	}

	Result.bSuccess = true;

	UE_LOG(LogTemp, Log, TEXT("NPCAssetSync: Synced %d NPCDefinition assets (%d vendors, %d with ability configs)"),
		Result.NPCsFound, Result.VendorNPCs, Result.NPCsWithAbilityConfig);

#else
	Result.ErrorMessage = TEXT("NPCAssetSync requires WITH_EDITOR");
#endif

	return Result;
}

int32 FNPCAssetSync::PopulateRowsFromAssets(TArray<FNPCTableRow>& Rows, const FNPCAssetSyncResult& SyncResult)
{
	int32 UpdatedCount = 0;

	for (FNPCTableRow& Row : Rows)
	{
		const FNPCAssetData* AssetData = SyncResult.NPCData.Find(Row.NPCName);
		if (AssetData && AssetData->bFoundInAsset)
		{
			// Update row fields from asset data
			Row.NPCId = AssetData->NPCId;
			Row.DisplayName = AssetData->DisplayName;
			Row.Blueprint = AssetData->Blueprint;
			Row.AbilityConfig = AssetData->AbilityConfig;
			Row.ActivityConfig = AssetData->ActivityConfig;
			Row.Schedule = AssetData->Schedule;
			Row.MinLevel = AssetData->MinLevel;
			Row.MaxLevel = AssetData->MaxLevel;
			Row.AttackPriority = AssetData->AttackPriority;
			Row.Factions = AssetData->Factions;
			Row.bIsVendor = AssetData->bIsVendor;
			Row.ShopName = AssetData->ShopName;
			Row.Appearance = AssetData->Appearance;
			Row.Status = ENPCTableRowStatus::Synced;
			UpdatedCount++;
		}
	}

	return UpdatedCount;
}

//=========================================================================
// Apply: Write table rows to NPC assets
//=========================================================================

FNPCAssetApplyResult FNPCAssetSync::ApplyToAssets(
	TArray<FNPCTableRow>& Rows,
	bool bCreateMissing,
	const FString& OutputFolder)
{
	FNPCAssetApplyResult Result;

#if WITH_EDITOR
	// Step 1: Validate all rows
	TArray<FNPCTableRow> RowsForValidation;
	for (const FNPCTableRow& Row : Rows)
	{
		RowsForValidation.Add(Row);
	}

	FNPCValidationResult ValidationResult = FNPCTableValidator::ValidateAll(RowsForValidation);

	// Build set of rows with validation errors
	TSet<FString> RowsWithErrors;
	for (const FNPCValidationIssue& Issue : ValidationResult.Issues)
	{
		if (Issue.Severity == ENPCValidationSeverity::Error)
		{
			RowsWithErrors.Add(Issue.NPCName);
		}
	}

	// Step 2: Process each row
	for (FNPCTableRow& Row : Rows)
	{
		// Skip rows with validation errors
		if (RowsWithErrors.Contains(Row.NPCName) || RowsWithErrors.Contains(Row.RowId.ToString()))
		{
			Result.NPCsSkippedValidation++;
			continue;
		}

		// Skip rows that haven't been modified
		if (Row.Status != ENPCTableRowStatus::Modified && Row.Status != ENPCTableRowStatus::New)
		{
			Result.NPCsSkippedNotModified++;
			continue;
		}

		// Check if row has an existing asset reference
		if (Row.GeneratedNPCDef.IsNull())
		{
			if (bCreateMissing)
			{
				// TODO: Create new asset via FNPCDefinitionGenerator
				Result.NPCsCreated++;
			}
			else
			{
				Result.NPCsSkippedNoAsset++;
			}
			continue;
		}

		// Load the NPCDefinition asset
		UNPCDefinition* NPCDef = Cast<UNPCDefinition>(Row.GeneratedNPCDef.TryLoad());
		if (!NPCDef)
		{
			Result.FailedNPCs.Add(Row.NPCName);
			continue;
		}

		// Check if asset is writable
		if (!IsAssetWritable(NPCDef))
		{
			Result.NPCsSkippedReadOnly++;
			Result.ReadOnlyNPCs.Add(Row.NPCName);
			continue;
		}

		// Apply row data to asset
		if (ApplyRowToAsset(Row, NPCDef))
		{
			// Save the asset
			if (SaveAsset(NPCDef))
			{
				Row.Status = ENPCTableRowStatus::Synced;
				Result.NPCsUpdated++;
			}
			else
			{
				Result.FailedNPCs.Add(Row.NPCName);
			}
		}
		else
		{
			Result.FailedNPCs.Add(Row.NPCName);
		}
	}

	Result.bSuccess = true;

	UE_LOG(LogTemp, Log, TEXT("NPCAssetSync: Applied to %d NPCs, skipped %d (not modified), %d (validation), %d (no asset), %d (read-only), %d failed"),
		Result.NPCsUpdated, Result.NPCsSkippedNotModified, Result.NPCsSkippedValidation,
		Result.NPCsSkippedNoAsset, Result.NPCsSkippedReadOnly, Result.FailedNPCs.Num());

#else
	Result.ErrorMessage = TEXT("NPCAssetSync requires WITH_EDITOR");
#endif

	return Result;
}

bool FNPCAssetSync::ApplyRowToAsset(const FNPCTableRow& Row, UNPCDefinition* NPCDef)
{
#if WITH_EDITOR
	if (!NPCDef)
	{
		return false;
	}

	//=========================================================================
	// Core Identity
	//=========================================================================
	NPCDef->NPCID = FName(*Row.NPCId);
	NPCDef->NPCName = FText::FromString(Row.DisplayName);

	// Blueprint - NPCClassPath
	if (!Row.Blueprint.IsNull())
	{
		NPCDef->NPCClassPath = TSoftClassPtr<ANarrativeNPCCharacter>(FSoftObjectPath(Row.Blueprint));
	}

	//=========================================================================
	// AI & Behavior
	//=========================================================================
	if (!Row.AbilityConfig.IsNull())
	{
		NPCDef->AbilityConfiguration = Cast<UAbilityConfiguration>(Row.AbilityConfig.TryLoad());
	}

	if (!Row.ActivityConfig.IsNull())
	{
		NPCDef->ActivityConfiguration = TSoftObjectPtr<UNPCActivityConfiguration>(Row.ActivityConfig);
	}

	// Schedule - we don't overwrite TriggerSets/ActivitySchedules arrays to avoid data loss
	// User should manage those in the editor

	//=========================================================================
	// Combat
	//=========================================================================
	NPCDef->MinLevel = Row.MinLevel;
	NPCDef->MaxLevel = Row.MaxLevel;
	NPCDef->AttackPriority = Row.AttackPriority;

	// Factions
	StringToFactions(Row.Factions, NPCDef->DefaultFactions);

	//=========================================================================
	// Vendor
	//=========================================================================
	NPCDef->bIsVendor = Row.bIsVendor;
	NPCDef->ShopFriendlyName = FText::FromString(Row.ShopName);

	//=========================================================================
	// Items - DefaultItems not overwritten to avoid data loss
	// SpawnerPOI - Not stored on NPCDefinition, managed by NPCSpawner actors
	//=========================================================================

	//=========================================================================
	// Meta
	//=========================================================================
	if (!Row.Appearance.IsNull())
	{
		NPCDef->DefaultAppearance = TSoftObjectPtr<UObject>(Row.Appearance);
	}

	return true;

#else
	return false;
#endif
}

bool FNPCAssetSync::IsAssetWritable(UObject* Asset)
{
#if WITH_EDITOR
	if (!Asset)
	{
		return false;
	}

	UPackage* Package = Asset->GetOutermost();
	if (!Package)
	{
		return false;
	}

	FString PackagePath = Package->GetName();

	// Only /Game/ assets are writable, plugin content is read-only
	return PackagePath.StartsWith(TEXT("/Game/"));

#else
	return false;
#endif
}

bool FNPCAssetSync::SaveAsset(UNPCDefinition* NPCDef)
{
#if WITH_EDITOR
	if (!NPCDef)
	{
		return false;
	}

	UPackage* Package = NPCDef->GetOutermost();
	if (!Package)
	{
		return false;
	}

	Package->MarkPackageDirty();

	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(),
		FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;

	return UPackage::SavePackage(Package, NPCDef, *PackageFileName, SaveArgs);

#else
	return false;
#endif
}

FString FNPCAssetSync::FactionsToString(const FGameplayTagContainer& Tags)
{
	TArray<FString> ShortNames;

	for (const FGameplayTag& Tag : Tags)
	{
		ShortNames.Add(FNPCTableRow::ToShortFactionName(Tag.ToString()));
	}

	return FString::Join(ShortNames, TEXT(", "));
}

void FNPCAssetSync::StringToFactions(const FString& FactionsStr, FGameplayTagContainer& OutTags)
{
	OutTags.Reset();

	TArray<FString> FactionNames;
	FactionsStr.ParseIntoArray(FactionNames, TEXT(","));

	for (const FString& FactionName : FactionNames)
	{
		FString FullTag = FNPCTableRow::ToFullFactionTag(FactionName);
		if (!FullTag.IsEmpty())
		{
			FGameplayTag GameplayTag = FGameplayTag::RequestGameplayTag(FName(*FullTag), false);
			if (GameplayTag.IsValid())
			{
				OutTags.AddTag(GameplayTag);
			}
		}
	}
}
