// NPCTableEditorTypes.h
// NPC Table Editor - Data types
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/SoftObjectPath.h"
#include "NPCTableEditorTypes.generated.h"

/**
 * Single row in the NPC Table Editor
 * Represents one NPC with all its related asset references
 * Maps to UNPCDefinition + UCharacterDefinition fields
 */
USTRUCT(BlueprintType)
struct GASABILITYGENERATOR_API FNPCTableRow
{
	GENERATED_BODY()

	/** Unique row identifier for internal tracking */
	UPROPERTY()
	FGuid RowId;

	//=========================================================================
	// Identity (Required)
	//=========================================================================

	/** NPC Name - used for asset naming (NPCDef_{NPCName}) */
	UPROPERTY(EditAnywhere, Category = "Identity")
	FString NPCName;

	/** Unique NPC ID - runtime identifier (NPCID in NPCDefinition) */
	UPROPERTY(EditAnywhere, Category = "Identity")
	FString NPCId;

	/** Display name shown in-game (NPCName in NPCDefinition) */
	UPROPERTY(EditAnywhere, Category = "Identity")
	FString DisplayName;

	/** Can spawn multiple instances of this NPC? (false = unique NPC) */
	UPROPERTY(EditAnywhere, Category = "Identity")
	bool bAllowMultipleInstances = true;

	//=========================================================================
	// Asset References
	//=========================================================================

	/** NPC Blueprint - the character class (NPCClassPath) */
	UPROPERTY(EditAnywhere, Category = "Assets")
	FSoftObjectPath NPCBlueprint;

	/** Ability Configuration (from CharacterDefinition) */
	UPROPERTY(EditAnywhere, Category = "Assets")
	FSoftObjectPath AbilityConfig;

	/** Activity Configuration */
	UPROPERTY(EditAnywhere, Category = "Assets")
	FSoftObjectPath ActivityConfig;

	/** Activity Schedules (comma-separated asset names) */
	UPROPERTY(EditAnywhere, Category = "Assets")
	FString ActivitySchedules;

	/** Default Appearance */
	UPROPERTY(EditAnywhere, Category = "Assets")
	FSoftObjectPath DefaultAppearance;

	/** Trigger Sets (comma-separated asset names) */
	UPROPERTY(EditAnywhere, Category = "Assets")
	FString TriggerSets;

	//=========================================================================
	// Spawning / Location
	//=========================================================================

	/** POI or Spawner where this NPC appears (comma-separated list) */
	UPROPERTY(EditAnywhere, Category = "Location")
	FString SpawnerPOI;

	/** Level/Map name */
	UPROPERTY(EditAnywhere, Category = "Location")
	FString LevelName;

	/** Discovered spawners that reference this NPC (populated by Sync) */
	UPROPERTY(VisibleAnywhere, Category = "Location")
	FString DiscoveredSpawners;

	//=========================================================================
	// Inventory & Currency
	//=========================================================================

	/** Default currency this NPC has */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 DefaultCurrency = 0;

	/** Default item loadout (comma-separated: EI_Sword, IC_Weapons) */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FString DefaultItems;

	//=========================================================================
	// Vendor Settings
	//=========================================================================

	/** Is this NPC a vendor/merchant */
	UPROPERTY(EditAnywhere, Category = "Vendor")
	bool bIsVendor = false;

	/** Shop display name */
	UPROPERTY(EditAnywhere, Category = "Vendor")
	FString ShopName;

	/** Vendor's trading currency (gold for buying/selling) */
	UPROPERTY(EditAnywhere, Category = "Vendor")
	int32 TradingCurrency = 500;

	/** Buy percentage (0.5 = pays 50% of item value) */
	UPROPERTY(EditAnywhere, Category = "Vendor")
	float BuyItemPercentage = 0.5f;

	/** Sell percentage (1.5 = charges 150% of item value) */
	UPROPERTY(EditAnywhere, Category = "Vendor")
	float SellItemPercentage = 1.5f;

	/** Vendor inventory (comma-separated: IC_Weapons, IC_Potions) */
	UPROPERTY(EditAnywhere, Category = "Vendor")
	FString TradingItems;

	//=========================================================================
	// Tags & Factions
	//=========================================================================

	/** Faction tags (comma-separated, short names OK: Friendly, Town) */
	UPROPERTY(EditAnywhere, Category = "Tags")
	FString Factions;

	/** Owned tags (comma-separated, short names OK: Invulnerable) */
	UPROPERTY(EditAnywhere, Category = "Tags")
	FString OwnedTags;

	//=========================================================================
	// Combat
	//=========================================================================

	/** Min level */
	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 MinLevel = 1;

	/** Max level */
	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 MaxLevel = 10;

	/** Attack priority (0.0-1.0, higher = more likely to be targeted) */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackPriority = 0.5f;

	//=========================================================================
	// Status / Metadata
	//=========================================================================

	/** Row status: New, Modified, Synced, Error */
	UPROPERTY(VisibleAnywhere, Category = "Status")
	FString Status = TEXT("New");

	/** Is this row from plugin content (read-only, can't be saved) */
	UPROPERTY(VisibleAnywhere, Category = "Status")
	bool bIsReadOnly = false;

	/** Generated NPCDefinition asset path (after generation) */
	UPROPERTY(VisibleAnywhere, Category = "Status")
	FSoftObjectPath GeneratedNPCDef;

	/** Notes/comments */
	UPROPERTY(EditAnywhere, Category = "Metadata")
	FString Notes;

	//=========================================================================
	// Methods
	//=========================================================================

	FNPCTableRow()
	{
		RowId = FGuid::NewGuid();
	}

	bool IsValid() const
	{
		return !NPCName.IsEmpty() && !NPCId.IsEmpty();
	}

	/** Get status color for UI */
	FLinearColor GetStatusColor() const
	{
		if (Status == TEXT("New")) return FLinearColor(0.2f, 0.6f, 1.0f); // Blue
		if (Status == TEXT("Modified")) return FLinearColor(1.0f, 0.8f, 0.2f); // Yellow
		if (Status == TEXT("Synced")) return FLinearColor(0.2f, 0.8f, 0.2f); // Green
		if (Status == TEXT("Error")) return FLinearColor(1.0f, 0.2f, 0.2f); // Red
		return FLinearColor::White;
	}

	//=========================================================================
	// Tag Helpers - Convert short names to full tags
	//=========================================================================

	/** Convert short faction name to full tag (e.g., "Friendly" -> "Narrative.Factions.Friendly") */
	static FString ToFullFactionTag(const FString& ShortName)
	{
		FString Trimmed = ShortName.TrimStartAndEnd();
		if (Trimmed.IsEmpty()) return TEXT("");
		if (Trimmed.StartsWith(TEXT("Narrative.Factions."))) return Trimmed;
		return FString::Printf(TEXT("Narrative.Factions.%s"), *Trimmed);
	}

	/** Convert full faction tag to short name (e.g., "Narrative.Factions.Friendly" -> "Friendly") */
	static FString ToShortFactionName(const FString& FullTag)
	{
		FString Trimmed = FullTag.TrimStartAndEnd();
		if (Trimmed.StartsWith(TEXT("Narrative.Factions.")))
		{
			return Trimmed.RightChop(19); // Length of "Narrative.Factions."
		}
		return Trimmed;
	}

	/** Convert short state tag to full tag (e.g., "Invulnerable" -> "Narrative.State.Invulnerable") */
	static FString ToFullStateTag(const FString& ShortName)
	{
		FString Trimmed = ShortName.TrimStartAndEnd();
		if (Trimmed.IsEmpty()) return TEXT("");
		if (Trimmed.StartsWith(TEXT("Narrative.State."))) return Trimmed;
		if (Trimmed.StartsWith(TEXT("State."))) return FString::Printf(TEXT("Narrative.%s"), *Trimmed);
		return FString::Printf(TEXT("Narrative.State.%s"), *Trimmed);
	}

	/** Convert full state tag to short name */
	static FString ToShortStateName(const FString& FullTag)
	{
		FString Trimmed = FullTag.TrimStartAndEnd();
		if (Trimmed.StartsWith(TEXT("Narrative.State.")))
		{
			return Trimmed.RightChop(16); // Length of "Narrative.State."
		}
		return Trimmed;
	}

	/** Get factions as short display string */
	FString GetFactionsDisplay() const
	{
		TArray<FString> Tags;
		Factions.ParseIntoArray(Tags, TEXT(","));
		TArray<FString> ShortNames;
		for (const FString& Tag : Tags)
		{
			ShortNames.Add(ToShortFactionName(Tag));
		}
		return FString::Join(ShortNames, TEXT(", "));
	}

	/** Set factions from short names, converting to full tags */
	void SetFactionsFromDisplay(const FString& DisplayString)
	{
		TArray<FString> Names;
		DisplayString.ParseIntoArray(Names, TEXT(","));
		TArray<FString> FullTags;
		for (const FString& Name : Names)
		{
			FString FullTag = ToFullFactionTag(Name);
			if (!FullTag.IsEmpty())
			{
				FullTags.Add(FullTag);
			}
		}
		Factions = FString::Join(FullTags, TEXT(", "));
	}

	/** Get owned tags as short display string */
	FString GetOwnedTagsDisplay() const
	{
		TArray<FString> Tags;
		OwnedTags.ParseIntoArray(Tags, TEXT(","));
		TArray<FString> ShortNames;
		for (const FString& Tag : Tags)
		{
			ShortNames.Add(ToShortStateName(Tag));
		}
		return FString::Join(ShortNames, TEXT(", "));
	}

	/** Set owned tags from short names, converting to full tags */
	void SetOwnedTagsFromDisplay(const FString& DisplayString)
	{
		TArray<FString> Names;
		DisplayString.ParseIntoArray(Names, TEXT(","));
		TArray<FString> FullTags;
		for (const FString& Name : Names)
		{
			FString FullTag = ToFullStateTag(Name);
			if (!FullTag.IsEmpty())
			{
				FullTags.Add(FullTag);
			}
		}
		OwnedTags = FString::Join(FullTags, TEXT(", "));
	}
};

/**
 * Container for all NPC table data
 * Saved as a single DataAsset for persistence
 */
UCLASS(BlueprintType)
class GASABILITYGENERATOR_API UNPCTableData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** All NPC rows in the table */
	UPROPERTY(EditAnywhere, Category = "Data")
	TArray<FNPCTableRow> Rows;

	/** Project name for asset paths */
	UPROPERTY(EditAnywhere, Category = "Settings")
	FString ProjectName = TEXT("NP22B57");

	/** Default output folder */
	UPROPERTY(EditAnywhere, Category = "Settings")
	FString OutputFolder = TEXT("/Game/NPCs");

	/** Last sync timestamp */
	UPROPERTY(VisibleAnywhere, Category = "Metadata")
	FDateTime LastSyncTime;

	//=========================================================================
	// Row Management
	//=========================================================================

	/** Add a new empty row */
	FNPCTableRow& AddRow()
	{
		FNPCTableRow& NewRow = Rows.AddDefaulted_GetRef();
		NewRow.RowId = FGuid::NewGuid();
		NewRow.Status = TEXT("New");
		return NewRow;
	}

	/** Duplicate an existing row */
	FNPCTableRow* DuplicateRow(int32 SourceIndex)
	{
		if (Rows.IsValidIndex(SourceIndex))
		{
			FNPCTableRow NewRow = Rows[SourceIndex];
			NewRow.RowId = FGuid::NewGuid();
			NewRow.NPCName += TEXT("_Copy");
			NewRow.NPCId += TEXT("_copy");
			NewRow.Status = TEXT("New");
			NewRow.GeneratedNPCDef.Reset();
			Rows.Add(NewRow);
			return &Rows.Last();
		}
		return nullptr;
	}

	/** Delete row by index */
	void DeleteRow(int32 Index)
	{
		if (Rows.IsValidIndex(Index))
		{
			Rows.RemoveAt(Index);
		}
	}

	/** Find row by GUID */
	FNPCTableRow* FindRowByGuid(const FGuid& InRowId)
	{
		return Rows.FindByPredicate([&InRowId](const FNPCTableRow& Row)
		{
			return Row.RowId == InRowId;
		});
	}

	/** Find row index by GUID */
	int32 FindRowIndexByGuid(const FGuid& InRowId) const
	{
		return Rows.IndexOfByPredicate([&InRowId](const FNPCTableRow& Row)
		{
			return Row.RowId == InRowId;
		});
	}

	/** Remove row by GUID */
	bool RemoveRowByGuid(const FGuid& InRowId)
	{
		int32 Index = FindRowIndexByGuid(InRowId);
		if (Index != INDEX_NONE)
		{
			Rows.RemoveAt(Index);
			return true;
		}
		return false;
	}

	/** Get row count */
	int32 GetRowCount() const { return Rows.Num(); }
};
