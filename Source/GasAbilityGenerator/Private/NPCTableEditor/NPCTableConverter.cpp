// GasAbilityGenerator - NPC Table Converter Implementation
// v4.5: Converts NPC table rows to manifest definitions for generation

#include "NPCTableEditor/NPCTableConverter.h"

TArray<FManifestNPCDefinitionDefinition> FNPCTableConverter::ConvertRowsToManifest(
	const TArray<FNPCTableRow>& Rows,
	const FString& OutputFolder)
{
	TArray<FManifestNPCDefinitionDefinition> Definitions;

	for (const FNPCTableRow& Row : Rows)
	{
		// Skip invalid rows
		if (!Row.IsValid())
		{
			continue;
		}

		FManifestNPCDefinitionDefinition Def = ConvertRowToDefinition(Row, OutputFolder);
		Definitions.Add(Def);
	}

	return Definitions;
}

FManifestNPCDefinitionDefinition FNPCTableConverter::ConvertRowToDefinition(
	const FNPCTableRow& Row,
	const FString& OutputFolder)
{
	FManifestNPCDefinitionDefinition Def;

	//=========================================================================
	// Core Identity
	//=========================================================================
	Def.Name = FString::Printf(TEXT("NPC_%s"), *Row.NPCName);
	Def.Folder = OutputFolder;
	Def.NPCID = Row.NPCId;
	Def.NPCName = Row.DisplayName;  // NPCName in definition is the display name

	// Blueprint - convert soft object path to class path string
	if (!Row.Blueprint.IsNull())
	{
		// For TSoftClassPtr, we need the path with _C suffix
		FString BlueprintPath = Row.Blueprint.ToString();
		if (!BlueprintPath.EndsWith(TEXT("_C")))
		{
			BlueprintPath += TEXT("_C");
		}
		Def.NPCClassPath = BlueprintPath;
	}

	//=========================================================================
	// AI & Behavior
	//=========================================================================
	Def.AbilityConfiguration = GetAssetNameFromPath(Row.AbilityConfig);
	Def.ActivityConfiguration = GetAssetNameFromPath(Row.ActivityConfig);

	// Schedule is stored as activity schedule array
	if (!Row.Schedule.IsNull())
	{
		Def.ActivitySchedules.Add(GetAssetNameFromPath(Row.Schedule));
	}

	//=========================================================================
	// Combat
	//=========================================================================
	Def.MinLevel = Row.MinLevel;
	Def.MaxLevel = Row.MaxLevel;
	Def.DefaultFactions = ParseFactions(Row.Factions);
	Def.AttackPriority = Row.AttackPriority;

	//=========================================================================
	// Vendor
	//=========================================================================
	Def.bIsVendor = Row.bIsVendor;
	Def.ShopFriendlyName = Row.ShopName;

	//=========================================================================
	// Items & Spawning
	//=========================================================================
	// DefaultItems converts to item loadout collections
	Def.DefaultItemLoadoutCollections = ParseItems(Row.DefaultItems);
	if (Def.DefaultItemLoadoutCollections.Num() > 0)
	{
		Def.bAutoCreateItemLoadout = true;
	}

	// SpawnerPOI is not stored in definition - it's used for spawner placement
	// (handled separately by FNPCSpawnerPlacementGenerator)

	//=========================================================================
	// Meta
	//=========================================================================
	Def.DefaultAppearance = GetAssetNameFromPath(Row.Appearance);

	// Apply default values
	ApplyDefaults(Def);

	return Def;
}

void FNPCTableConverter::ApplyDefaults(FManifestNPCDefinitionDefinition& Def)
{
	// Default to allowing multiple instances
	Def.bAllowMultipleInstances = true;

	// Default vendor pricing if vendor is enabled
	if (Def.bIsVendor)
	{
		if (Def.TradingCurrency <= 0)
		{
			Def.TradingCurrency = 500;  // Default vendor gold
		}
		if (Def.BuyItemPercentage <= 0.0f)
		{
			Def.BuyItemPercentage = 0.5f;  // 50% of item value
		}
		if (Def.SellItemPercentage <= 0.0f)
		{
			Def.SellItemPercentage = 1.5f;  // 150% of item value
		}
	}
}

TArray<FString> FNPCTableConverter::ParseFactions(const FString& Factions)
{
	TArray<FString> Result;

	if (Factions.IsEmpty())
	{
		return Result;
	}

	TArray<FString> Parts;
	Factions.ParseIntoArray(Parts, TEXT(","));

	for (FString& Part : Parts)
	{
		Part = Part.TrimStartAndEnd();
		if (!Part.IsEmpty())
		{
			Result.Add(Part);
		}
	}

	return Result;
}

TArray<FString> FNPCTableConverter::ParseItems(const FString& Items)
{
	TArray<FString> Result;

	if (Items.IsEmpty())
	{
		return Result;
	}

	TArray<FString> Parts;
	Items.ParseIntoArray(Parts, TEXT(","));

	for (FString& Part : Parts)
	{
		Part = Part.TrimStartAndEnd();
		if (!Part.IsEmpty())
		{
			Result.Add(Part);
		}
	}

	return Result;
}

FString FNPCTableConverter::GetAssetNameFromPath(const FSoftObjectPath& Path)
{
	if (Path.IsNull())
	{
		return FString();
	}

	// Get the asset name from the path
	// Path format: "/Game/Path/AssetName.AssetName" -> "AssetName"
	return Path.GetAssetName();
}
