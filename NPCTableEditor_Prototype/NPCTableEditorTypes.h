// NPCTableEditorTypes.h
// PROTOTYPE - NPC Table Editor Data Types
// This is example code for review, not compiled with the plugin
//
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"
#include "NPCTableEditorTypes.generated.h"

/**
 * Single row in the NPC Table Editor
 * Represents one NPC with all its related asset references
 */
USTRUCT()
struct FNPCTableRow
{
	GENERATED_BODY()

	/** Unique row identifier for internal tracking */
	UPROPERTY()
	FGuid RowId;

	//=========================================================================
	// Identity (Required)
	//=========================================================================

	/** NPC Name - used for asset naming (NPCDef_{NPCName}) */
	UPROPERTY()
	FString NPCName;

	/** Unique NPC ID - runtime identifier */
	UPROPERTY()
	FString NPCId;

	/** Display name shown in-game */
	UPROPERTY()
	FString DisplayName;

	//=========================================================================
	// Asset References (Dropdowns with filtering)
	//=========================================================================

	/** NPC Blueprint - the character class */
	// Filter: /Script/NarrativeArsenal.NarrativeNPCCharacter
	UPROPERTY()
	FSoftObjectPath NPCBlueprint;

	/** Ability Configuration - abilities, effects, attributes */
	// Filter: /Script/NarrativeArsenal.AbilityConfiguration
	UPROPERTY()
	FSoftObjectPath AbilityConfig;

	/** Activity Configuration - AI activities and goals */
	// Filter: /Script/NarrativeArsenal.NPCActivityConfiguration
	UPROPERTY()
	FSoftObjectPath ActivityConfig;

	/** Schedule - daily routine */
	// Filter: /Script/NarrativeArsenal.NPCActivitySchedule
	UPROPERTY()
	FSoftObjectPath Schedule;

	/** Dialogue - conversation tree */
	// Filter: /Script/NarrativeArsenal.Dialogue
	UPROPERTY()
	FSoftObjectPath Dialogue;

	/** Tagged Dialogue - contextual barks */
	// Filter: /Script/NarrativeArsenal.TaggedDialogueSet
	UPROPERTY()
	FSoftObjectPath TaggedDialogue;

	//=========================================================================
	// Spawning / Location
	//=========================================================================

	/** POI or Spawner where this NPC appears */
	UPROPERTY()
	FString SpawnerPOI;

	/** Level/Map name */
	UPROPERTY()
	FString LevelName;

	//=========================================================================
	// Vendor Settings
	//=========================================================================

	/** Is this NPC a vendor/merchant */
	UPROPERTY()
	bool bIsVendor = false;

	/** Shop display name */
	UPROPERTY()
	FString ShopName;

	/** Starting gold */
	UPROPERTY()
	int32 TradingCurrency = 500;

	//=========================================================================
	// Items (comma-separated for table display)
	//=========================================================================

	/** Default item loadout (comma-separated: EI_Sword, EI_Shield) */
	UPROPERTY()
	FString DefaultItems;

	/** Vendor inventory (comma-separated: IC_Weapons, IC_Potions) */
	UPROPERTY()
	FString ShopItems;

	//=========================================================================
	// Tags & Factions
	//=========================================================================

	/** Faction tags (comma-separated) */
	UPROPERTY()
	FString Factions;

	/** Owned tags (comma-separated) */
	UPROPERTY()
	FString OwnedTags;

	//=========================================================================
	// Combat
	//=========================================================================

	/** Min level */
	UPROPERTY()
	int32 MinLevel = 1;

	/** Max level */
	UPROPERTY()
	int32 MaxLevel = 10;

	/** Attack priority (0.0-1.0) */
	UPROPERTY()
	float AttackPriority = 0.5f;

	//=========================================================================
	// Status / Metadata
	//=========================================================================

	/** Row status: New, Modified, Synced, Error */
	UPROPERTY()
	FString Status = TEXT("New");

	/** Generated NPCDefinition asset path (after generation) */
	UPROPERTY()
	FSoftObjectPath GeneratedNPCDef;

	/** Notes/comments */
	UPROPERTY()
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
};

/**
 * Container for all NPC table data
 * Saved as a single DataAsset for persistence
 */
UCLASS()
class UNPCTableData : public UDataAsset
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
	FNPCTableRow& DuplicateRow(int32 SourceIndex)
	{
		if (Rows.IsValidIndex(SourceIndex))
		{
			FNPCTableRow NewRow = Rows[SourceIndex];
			NewRow.RowId = FGuid::NewGuid();
			NewRow.NPCName += TEXT("_Copy");
			NewRow.NPCId += TEXT("_copy");
			NewRow.Status = TEXT("New");
			NewRow.GeneratedNPCDef.Reset();
			return Rows.Add_GetRef(NewRow);
		}
		return AddRow();
	}

	/** Delete row by index */
	void DeleteRow(int32 Index)
	{
		if (Rows.IsValidIndex(Index))
		{
			Rows.RemoveAt(Index);
		}
	}

	/** Find row by NPC Name */
	FNPCTableRow* FindByName(const FString& NPCName)
	{
		return Rows.FindByPredicate([&](const FNPCTableRow& Row)
		{
			return Row.NPCName == NPCName;
		});
	}

	/** Get row count */
	int32 GetRowCount() const { return Rows.Num(); }

	/** Mark all rows as modified */
	void MarkAllModified()
	{
		for (FNPCTableRow& Row : Rows)
		{
			if (Row.Status == TEXT("Synced"))
			{
				Row.Status = TEXT("Modified");
			}
		}
	}
};

/**
 * Column definition for the table view
 */
struct FNPCTableColumn
{
	FName ColumnId;
	FText DisplayName;
	float DefaultWidth;
	bool bCanSort;
	bool bCanFilter;
	bool bIsAssetPicker;  // Shows dropdown with asset filtering
	FString AllowedClass; // For asset pickers

	FNPCTableColumn(
		FName InId,
		FText InName,
		float InWidth = 100.0f,
		bool bSort = true,
		bool bFilter = true,
		bool bAssetPicker = false,
		const FString& InAllowedClass = TEXT(""))
		: ColumnId(InId)
		, DisplayName(InName)
		, DefaultWidth(InWidth)
		, bCanSort(bSort)
		, bCanFilter(bFilter)
		, bIsAssetPicker(bAssetPicker)
		, AllowedClass(InAllowedClass)
	{}
};

/**
 * Get default column definitions
 */
inline TArray<FNPCTableColumn> GetDefaultColumns()
{
	TArray<FNPCTableColumn> Columns;

	// Identity
	Columns.Add(FNPCTableColumn(TEXT("Status"), NSLOCTEXT("NPCTable", "Status", "Status"), 60.0f));
	Columns.Add(FNPCTableColumn(TEXT("NPCName"), NSLOCTEXT("NPCTable", "NPCName", "NPC Name"), 120.0f));
	Columns.Add(FNPCTableColumn(TEXT("NPCId"), NSLOCTEXT("NPCTable", "NPCId", "NPC ID"), 100.0f));
	Columns.Add(FNPCTableColumn(TEXT("DisplayName"), NSLOCTEXT("NPCTable", "DisplayName", "Display Name"), 120.0f));

	// Asset References (with filtering)
	Columns.Add(FNPCTableColumn(TEXT("NPCBlueprint"), NSLOCTEXT("NPCTable", "Blueprint", "Blueprint"), 150.0f, true, true, true, TEXT("/Script/NarrativeArsenal.NarrativeNPCCharacter")));
	Columns.Add(FNPCTableColumn(TEXT("AbilityConfig"), NSLOCTEXT("NPCTable", "AbilityConfig", "Ability Config"), 130.0f, true, true, true, TEXT("/Script/NarrativeArsenal.AbilityConfiguration")));
	Columns.Add(FNPCTableColumn(TEXT("ActivityConfig"), NSLOCTEXT("NPCTable", "ActivityConfig", "Activity Config"), 130.0f, true, true, true, TEXT("/Script/NarrativeArsenal.NPCActivityConfiguration")));
	Columns.Add(FNPCTableColumn(TEXT("Schedule"), NSLOCTEXT("NPCTable", "Schedule", "Schedule"), 120.0f, true, true, true, TEXT("/Script/NarrativeArsenal.NPCActivitySchedule")));
	Columns.Add(FNPCTableColumn(TEXT("Dialogue"), NSLOCTEXT("NPCTable", "Dialogue", "Dialogue"), 120.0f, true, true, true, TEXT("/Script/NarrativeArsenal.Dialogue")));

	// Location
	Columns.Add(FNPCTableColumn(TEXT("SpawnerPOI"), NSLOCTEXT("NPCTable", "SpawnerPOI", "Spawner/POI"), 120.0f));
	Columns.Add(FNPCTableColumn(TEXT("LevelName"), NSLOCTEXT("NPCTable", "LevelName", "Level"), 100.0f));

	// Vendor
	Columns.Add(FNPCTableColumn(TEXT("bIsVendor"), NSLOCTEXT("NPCTable", "IsVendor", "Vendor"), 50.0f));
	Columns.Add(FNPCTableColumn(TEXT("ShopName"), NSLOCTEXT("NPCTable", "ShopName", "Shop Name"), 100.0f));

	// Items
	Columns.Add(FNPCTableColumn(TEXT("DefaultItems"), NSLOCTEXT("NPCTable", "DefaultItems", "Default Items"), 150.0f));
	Columns.Add(FNPCTableColumn(TEXT("ShopItems"), NSLOCTEXT("NPCTable", "ShopItems", "Shop Items"), 150.0f));

	// Tags
	Columns.Add(FNPCTableColumn(TEXT("Factions"), NSLOCTEXT("NPCTable", "Factions", "Factions"), 120.0f));

	// Combat
	Columns.Add(FNPCTableColumn(TEXT("MinLevel"), NSLOCTEXT("NPCTable", "MinLevel", "Min Lvl"), 50.0f));
	Columns.Add(FNPCTableColumn(TEXT("MaxLevel"), NSLOCTEXT("NPCTable", "MaxLevel", "Max Lvl"), 50.0f));

	// Notes
	Columns.Add(FNPCTableColumn(TEXT("Notes"), NSLOCTEXT("NPCTable", "Notes", "Notes"), 200.0f));

	return Columns;
}
