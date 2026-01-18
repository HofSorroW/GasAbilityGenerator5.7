// GasAbilityGenerator - Item Asset Sync Implementation
// v4.12: Bidirectional sync between UEquippableItem assets and Item table rows

#include "ItemTableEditor/ItemAssetSync.h"
#include "ItemTableEditor/ItemTableValidator.h"
#include "GasAbilityGeneratorTypes.h"
#include "GasAbilityGeneratorGenerators.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "GameplayTagContainer.h"

// Narrative Pro includes
#include "Items/NarrativeItem.h"
#include "Items/EquippableItem.h"
#include "Items/WeaponItem.h"
#include "Items/RangedWeaponItem.h"
#include "Items/MeleeWeaponItem.h"
#endif

FItemAssetData FItemAssetSync::SyncFromAsset(UNarrativeItem* ItemAsset)
{
	FItemAssetData Data;

#if WITH_EDITOR
	if (!ItemAsset)
	{
		return Data;
	}

	Data.bFoundInAsset = true;
	Data.ItemName = ItemAsset->GetName();

	// Remove EI_ prefix if present
	if (Data.ItemName.StartsWith(TEXT("EI_")))
	{
		Data.ItemName = Data.ItemName.RightChop(3);
	}

	Data.DisplayName = ItemAsset->DisplayName.ToString();
	Data.Description = ItemAsset->Description.ToString();
	Data.BaseValue = ItemAsset->BaseValue;
	Data.Weight = ItemAsset->Weight;
	Data.ItemClassName = ItemAsset->GetClass()->GetName();

	// v4.12: EquipmentSlot and combat ratings are on UEquippableItem (protected)
	// Use reflection to read them if needed, or skip for now
	// The generation flow handles these via FManifestEquippableItemDefinition

#endif

	return Data;
}

FItemAssetSyncResult FItemAssetSync::SyncFromAllAssets()
{
	FItemAssetSyncResult Result;

#if WITH_EDITOR
	// Get Asset Registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Force rescan to catch newly created assets
	TArray<FString> PathsToScan = { TEXT("/Game/") };
	AssetRegistry.ScanPathsSynchronous(PathsToScan, true /* bForceRescan */);

	// Find all NarrativeItem assets (includes all derivatives)
	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByClass(UNarrativeItem::StaticClass()->GetClassPathName(), AssetList, true);

	for (const FAssetData& AssetData : AssetList)
	{
		UNarrativeItem* ItemAsset = Cast<UNarrativeItem>(AssetData.GetAsset());
		if (!ItemAsset)
		{
			continue;
		}

		FString ItemAssetName = AssetData.AssetName.ToString();
		FItemAssetData ItemData = SyncFromAsset(ItemAsset);

		Result.ItemData.Add(ItemAssetName, ItemData);
		Result.ItemsFound++;

		// Categorize
		if (Cast<UWeaponItem>(ItemAsset))
		{
			Result.WeaponsFound++;
		}
		else if (Cast<UEquippableItem>(ItemAsset))
		{
			Result.ArmorFound++;
		}
	}

	Result.bSuccess = true;

	UE_LOG(LogTemp, Log, TEXT("ItemAssetSync: Synced %d Item assets (%d weapons, %d armor)"),
		Result.ItemsFound, Result.WeaponsFound, Result.ArmorFound);

#else
	Result.ErrorMessage = TEXT("ItemAssetSync requires WITH_EDITOR");
#endif

	return Result;
}

int32 FItemAssetSync::PopulateRowsFromAssets(
	TArray<FItemTableRow>& Rows,
	const FItemAssetSyncResult& SyncResult)
{
	int32 UpdatedCount = 0;

	// Build a map of ItemName -> Asset data for quick lookup
	// SAFETY: These pointers are valid because SyncResult is passed by const& and
	// remains alive for the entire function. Do NOT store this map as a class member
	// or return it - pointers would become dangling when SyncResult is destroyed.
	TMap<FString, const FItemAssetData*> ItemNameToData;
	for (const auto& Pair : SyncResult.ItemData)
	{
		const FItemAssetData& Data = Pair.Value;
		if (Data.bFoundInAsset)
		{
			ItemNameToData.Add(Data.ItemName, &Data);
			// Also add with EI_ prefix stripped
			FString ShortName = Pair.Key;
			if (ShortName.StartsWith(TEXT("EI_")))
			{
				ShortName = ShortName.RightChop(3);
			}
			ItemNameToData.Add(ShortName, &Data);
		}
	}

	// Update rows that match item names
	for (FItemTableRow& Row : Rows)
	{
		const FItemAssetData** DataPtr = ItemNameToData.Find(Row.ItemName);
		if (DataPtr && *DataPtr)
		{
			const FItemAssetData& Data = **DataPtr;

			// Update fields from asset data
			if (Row.DisplayName.IsEmpty() && !Data.DisplayName.IsEmpty())
			{
				Row.DisplayName = Data.DisplayName;
			}
			if (Row.Description.IsEmpty() && !Data.Description.IsEmpty())
			{
				Row.Description = Data.Description;
			}
			if (Row.BaseValue == 0)
			{
				Row.BaseValue = Data.BaseValue;
			}
			if (FMath::IsNearlyEqual(Row.Weight, 1.0f))
			{
				Row.Weight = Data.Weight;
			}
			if (Row.EquipmentSlot.IsEmpty() && !Data.EquipmentSlot.IsEmpty())
			{
				Row.EquipmentSlot = Data.EquipmentSlot;
			}
			if (FMath::IsNearlyZero(Row.AttackRating))
			{
				Row.AttackRating = Data.AttackRating;
			}
			if (FMath::IsNearlyZero(Row.ArmorRating))
			{
				Row.ArmorRating = Data.ArmorRating;
			}
			if (FMath::IsNearlyZero(Row.AttackDamage))
			{
				Row.AttackDamage = Data.AttackDamage;
			}

			Row.Status = EItemTableRowStatus::Synced;
			UpdatedCount++;
		}
	}

	return UpdatedCount;
}

FItemAssetApplySummary FItemAssetSync::ApplyToAssets(
	TArray<FItemTableRow>& Rows,
	const FString& OutputFolder,
	bool bCreateMissing)
{
	FItemAssetApplySummary Result;

#if WITH_EDITOR
	// Step 1: Validate all rows
	FItemValidationResult ValidationResult = FItemTableValidator::ValidateAll(Rows);

	// Build set of items with validation errors
	TSet<FString> ItemsWithErrors;
	for (const FItemValidationIssue& Issue : ValidationResult.Issues)
	{
		if (Issue.Severity == EItemValidationSeverity::Error)
		{
			ItemsWithErrors.Add(Issue.ItemName);
		}
	}

	// Step 2: Process each row
	for (FItemTableRow& Row : Rows)
	{
		if (Row.bDeleted || Row.ItemName.IsEmpty())
		{
			continue;
		}

		Result.AssetsProcessed++;

		// Skip rows with validation errors
		if (ItemsWithErrors.Contains(Row.ItemName))
		{
			Result.AssetsSkippedValidation++;
			continue;
		}

		// Skip rows that haven't been modified
		if (Row.Status != EItemTableRowStatus::Modified && Row.Status != EItemTableRowStatus::New)
		{
			Result.AssetsSkippedNotModified++;
			continue;
		}

		// Check if asset exists
		bool bAssetExists = !Row.GeneratedItem.IsNull();

		if (!bAssetExists && !bCreateMissing)
		{
			Result.AssetsSkippedNoAsset++;
			continue;
		}

		// Build manifest definition and generate
		FManifestEquippableItemDefinition Definition = BuildItemDefinition(Row, OutputFolder);

		FGenerationResult GenResult = FEquippableItemGenerator::Generate(Definition);

		if (GenResult.Status == EGenerationStatus::New)
		{
			if (bAssetExists)
			{
				Result.AssetsModified++;
			}
			else
			{
				Result.AssetsCreated++;
			}

			// Update row status and asset reference
			Row.Status = EItemTableRowStatus::Synced;
			Row.GeneratedItem = FSoftObjectPath(GenResult.AssetPath);
		}
		else if (GenResult.Status == EGenerationStatus::Skipped)
		{
			Result.AssetsSkippedNotModified++;
		}
		else
		{
			Result.FailedItems.Add(Row.ItemName);
		}
	}

	Result.bSuccess = true;

	UE_LOG(LogTemp, Log, TEXT("ItemAssetSync: Applied %d items (%d created, %d modified, %d skipped)"),
		Result.AssetsProcessed,
		Result.AssetsCreated,
		Result.AssetsModified,
		Result.AssetsSkippedNotModified + Result.AssetsSkippedValidation + Result.AssetsSkippedNoAsset);

#else
	Result.ErrorMessage = TEXT("ItemAssetSync requires WITH_EDITOR");
#endif

	return Result;
}

bool FItemAssetSync::IsAssetWritable(UObject* Asset)
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

FString FItemAssetSync::GetParentClassName(EItemType ItemType)
{
	switch (ItemType)
	{
		case EItemType::Consumable:
			return TEXT("GameplayEffectItem");
		case EItemType::Ammo:
			return TEXT("AmmoItem");
		case EItemType::WeaponAttachment:
			return TEXT("WeaponAttachmentItem");
		case EItemType::Equippable:
			return TEXT("EquippableItem");
		case EItemType::Clothing:
			return TEXT("EquippableItem_Clothing");
		case EItemType::ThrowableWeapon:
			return TEXT("ThrowableWeaponItem");
		case EItemType::MeleeWeapon:
			return TEXT("MeleeWeaponItem");
		case EItemType::RangedWeapon:
			return TEXT("RangedWeaponItem");
		case EItemType::MagicWeapon:
			return TEXT("MagicWeaponItem");
		default:
			return TEXT("EquippableItem");
	}
}

FManifestEquippableItemDefinition FItemAssetSync::BuildItemDefinition(
	const FItemTableRow& Row,
	const FString& OutputFolder)
{
	FManifestEquippableItemDefinition Def;

	Def.Name = FString::Printf(TEXT("EI_%s"), *Row.ItemName);
	Def.Folder = OutputFolder.Replace(TEXT("/Game/"), TEXT(""));
	Def.ParentClass = GetParentClassName(Row.ItemType);

	// Base properties
	Def.DisplayName = Row.DisplayName.IsEmpty() ? Row.ItemName : Row.DisplayName;
	Def.Description = Row.Description;
	Def.BaseValue = Row.BaseValue;
	Def.Weight = Row.Weight;
	Def.BaseScore = Row.BaseScore;

	// Equipment slot
	if (!Row.EquipmentSlot.IsEmpty())
	{
		Def.EquipmentSlot = Row.EquipmentSlot;
	}

	// Equipment stats (UEquippableItem)
	Def.AttackRating = Row.AttackRating;
	Def.ArmorRating = Row.ArmorRating;
	Def.StealthRating = Row.StealthRating;

	// Weapon stats (UWeaponItem)
	if (Row.ItemType == EItemType::MeleeWeapon ||
		Row.ItemType == EItemType::RangedWeapon ||
		Row.ItemType == EItemType::MagicWeapon ||
		Row.ItemType == EItemType::ThrowableWeapon)
	{
		Def.AttackDamage = Row.AttackDamage;
		Def.HeavyAttackDamageMultiplier = Row.HeavyAttackDamageMultiplier;
		Def.WeaponHand = Row.WeaponHand;
		Def.ClipSize = Row.ClipSize;
		Def.RequiredAmmo = Row.RequiredAmmo;
		Def.bAllowManualReload = Row.bAllowManualReload;
		Def.BotAttackRange = Row.BotAttackRange;
	}

	// Ranged weapon stats (URangedWeaponItem)
	if (Row.ItemType == EItemType::RangedWeapon)
	{
		Def.BaseSpreadDegrees = Row.BaseSpreadDegrees;
		Def.MaxSpreadDegrees = Row.MaxSpreadDegrees;
		Def.SpreadFireBump = Row.SpreadFireBump;
		Def.SpreadDecreaseSpeed = Row.SpreadDecreaseSpeed;
		Def.AimFOVPct = Row.AimFOVPct;
	}

	// Consumable (UNarrativeItem)
	Def.bConsumeOnUse = Row.bConsumeOnUse;
	Def.bStackable = Row.bStackable;
	Def.MaxStackSize = Row.MaxStackSize;

	return Def;
}
