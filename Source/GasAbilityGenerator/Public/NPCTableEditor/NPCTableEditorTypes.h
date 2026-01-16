// NPCTableEditorTypes.h
// NPC Table Editor - Data types (v4.6 - generation tracking, soft delete)
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/SoftObjectPath.h"
#include "DialogueTableEditorTypes.h"  // For EValidationState (shared enum)
#include "NPCTableEditorTypes.generated.h"

/**
 * Row status enum for NPC table (sync state)
 */
UENUM(BlueprintType)
enum class ENPCTableRowStatus : uint8
{
	New UMETA(DisplayName = "New"),
	Modified UMETA(DisplayName = "Modified"),
	Synced UMETA(DisplayName = "Synced"),
	Error UMETA(DisplayName = "Error")
};

/**
 * Single row in the NPC Table Editor (v4.1 - 18 columns)
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
	// Core Identity (5 columns)
	//=========================================================================

	/** Row status: New, Modified, Synced, Error (auto-calculated) */
	UPROPERTY(VisibleAnywhere, Category = "Identity")
	ENPCTableRowStatus Status = ENPCTableRowStatus::New;

	/** NPC Name - used for asset naming (NPC_{NPCName}) */
	UPROPERTY(EditAnywhere, Category = "Identity")
	FString NPCName;

	/** Unique NPC ID - runtime identifier */
	UPROPERTY(EditAnywhere, Category = "Identity")
	FString NPCId;

	/** Display name shown in-game */
	UPROPERTY(EditAnywhere, Category = "Identity")
	FString DisplayName;

	/** NPC Blueprint - TSoftClassPtr<ANarrativeNPCCharacter> */
	UPROPERTY(EditAnywhere, Category = "Identity")
	FSoftObjectPath Blueprint;

	//=========================================================================
	// AI & Behavior (4 columns) - Dropdown asset pickers
	//=========================================================================

	/** Ability Configuration - AC_* DataAsset */
	UPROPERTY(EditAnywhere, Category = "AI")
	FSoftObjectPath AbilityConfig;

	/** Activity Configuration - ActConfig_* DataAsset */
	UPROPERTY(EditAnywhere, Category = "AI")
	FSoftObjectPath ActivityConfig;

	/** Activity Schedule - Schedule_* DataAsset */
	UPROPERTY(EditAnywhere, Category = "AI")
	FSoftObjectPath Schedule;

	/** Behavior Tree override - BT_* asset (optional) */
	UPROPERTY(EditAnywhere, Category = "AI")
	FSoftObjectPath BehaviorTree;

	//=========================================================================
	// Combat (3 columns)
	//=========================================================================

	/** Min level for NPC */
	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 MinLevel = 1;

	/** Max level for NPC */
	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 MaxLevel = 10;

	/** Faction tags (multi-select dropdown) */
	UPROPERTY(EditAnywhere, Category = "Combat")
	FString Factions;

	/** Attack priority (0.0-1.0, higher = more likely to be targeted) */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackPriority = 0.5f;

	//=========================================================================
	// Vendor (2 columns)
	//=========================================================================

	/** Is this NPC a vendor/merchant */
	UPROPERTY(EditAnywhere, Category = "Vendor")
	bool bIsVendor = false;

	/** Shop display name */
	UPROPERTY(EditAnywhere, Category = "Vendor")
	FString ShopName;

	//=========================================================================
	// Items & Spawning (2 columns)
	//=========================================================================

	/** Default item loadout - IC_* collections (multi-select dropdown) */
	UPROPERTY(EditAnywhere, Category = "Items")
	FString DefaultItems;

	/** POI where this NPC spawns (dropdown from level POIs) */
	UPROPERTY(EditAnywhere, Category = "Spawning")
	FString SpawnerPOI;

	//=========================================================================
	// Meta (2 columns)
	//=========================================================================

	/** Appearance preset - dropdown asset picker */
	UPROPERTY(EditAnywhere, Category = "Meta")
	FSoftObjectPath Appearance;

	/** Designer notes/comments */
	UPROPERTY(EditAnywhere, Category = "Meta")
	FString Notes;

	//=========================================================================
	// Soft Delete (v4.6)
	//=========================================================================

	/** Soft delete flag - row skipped during generation, asset untouched */
	UPROPERTY(EditAnywhere, Category = "Meta")
	bool bDeleted = false;

	//=========================================================================
	// Internal (not displayed as columns)
	//=========================================================================

	/** Generated NPCDefinition asset path (after generation) */
	UPROPERTY(VisibleAnywhere, Category = "Internal")
	FSoftObjectPath GeneratedNPCDef;

	//=========================================================================
	// v4.5: Validation cache (Transient - UI only, not serialized)
	//=========================================================================

	/** Current validation state (Unknown/Valid/Invalid) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Validation")
	EValidationState ValidationState = EValidationState::Unknown;

	/** Summary of validation issues (empty if valid) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Validation")
	FString ValidationSummary;

	/** Count of validation issues */
	UPROPERTY(Transient)
	int32 ValidationIssueCount = 0;

	/** Hash of inputs used for validation (staleness detection) */
	UPROPERTY(Transient)
	uint32 ValidationInputHash = 0;

	/** Invalidate cached validation (call on row edit) */
	void InvalidateValidation()
	{
		ValidationState = EValidationState::Unknown;
		ValidationSummary.Empty();
		ValidationIssueCount = 0;
		ValidationInputHash = 0;
	}

	/** Get validation state - accessor for consistency with Dialogue editor */
	EValidationState GetValidationState() const { return ValidationState; }

	/** Compute hash of editable fields (for validation staleness detection) */
	uint32 ComputeEditableFieldsHash() const
	{
		uint32 Hash = 0;
		Hash = HashCombine(Hash, GetTypeHash(NPCName));
		Hash = HashCombine(Hash, GetTypeHash(NPCId));
		Hash = HashCombine(Hash, GetTypeHash(DisplayName));
		Hash = HashCombine(Hash, GetTypeHash(Blueprint.ToString()));
		Hash = HashCombine(Hash, GetTypeHash(AbilityConfig.ToString()));
		Hash = HashCombine(Hash, GetTypeHash(ActivityConfig.ToString()));
		Hash = HashCombine(Hash, GetTypeHash(Schedule.ToString()));
		Hash = HashCombine(Hash, GetTypeHash(BehaviorTree.ToString()));
		Hash = HashCombine(Hash, GetTypeHash(MinLevel));
		Hash = HashCombine(Hash, GetTypeHash(MaxLevel));
		Hash = HashCombine(Hash, GetTypeHash(Factions));
		Hash = HashCombine(Hash, GetTypeHash(AttackPriority));
		Hash = HashCombine(Hash, GetTypeHash(bIsVendor));
		Hash = HashCombine(Hash, GetTypeHash(ShopName));
		Hash = HashCombine(Hash, GetTypeHash(DefaultItems));
		Hash = HashCombine(Hash, GetTypeHash(SpawnerPOI));
		Hash = HashCombine(Hash, GetTypeHash(Appearance.ToString()));
		Hash = HashCombine(Hash, GetTypeHash(bDeleted));  // v4.6: Include soft delete
		return Hash;
	}

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

	/** Get level range as display string "1-10" */
	FString GetLevelRangeDisplay() const
	{
		return FString::Printf(TEXT("%d-%d"), MinLevel, MaxLevel);
	}

	/** Set level range from display string "1-10" */
	void SetLevelRangeFromDisplay(const FString& DisplayString)
	{
		TArray<FString> Parts;
		DisplayString.ParseIntoArray(Parts, TEXT("-"));
		if (Parts.Num() >= 2)
		{
			MinLevel = FCString::Atoi(*Parts[0]);
			MaxLevel = FCString::Atoi(*Parts[1]);
		}
	}

	/** Get status color for UI */
	FLinearColor GetStatusColor() const
	{
		switch (Status)
		{
			case ENPCTableRowStatus::New: return FLinearColor(0.2f, 0.6f, 1.0f); // Blue
			case ENPCTableRowStatus::Modified: return FLinearColor(1.0f, 0.8f, 0.2f); // Yellow
			case ENPCTableRowStatus::Synced: return FLinearColor(0.2f, 0.8f, 0.2f); // Green
			case ENPCTableRowStatus::Error: return FLinearColor(1.0f, 0.2f, 0.2f); // Red
			default: return FLinearColor::White;
		}
	}

	/** Get status as string for display */
	FString GetStatusString() const
	{
		switch (Status)
		{
			case ENPCTableRowStatus::New: return TEXT("New");
			case ENPCTableRowStatus::Modified: return TEXT("Modified");
			case ENPCTableRowStatus::Synced: return TEXT("Synced");
			case ENPCTableRowStatus::Error: return TEXT("Error");
			default: return TEXT("Unknown");
		}
	}

	/** Get validation color for row stripe (matches Dialogue table pattern) */
	FLinearColor GetValidationColor() const
	{
		switch (ValidationState)
		{
			case EValidationState::Valid: return FLinearColor(0.2f, 0.8f, 0.2f);    // Green
			case EValidationState::Invalid: return FLinearColor(1.0f, 0.2f, 0.2f);  // Red
			case EValidationState::Unknown: return FLinearColor(1.0f, 0.8f, 0.2f);  // Yellow
			default: return FLinearColor::White;
		}
	}

	//=========================================================================
	// Faction Tag Helpers
	//=========================================================================

	/** Convert short faction name to full tag (e.g., "Friendly" -> "Narrative.Factions.Friendly") */
	static FString ToFullFactionTag(const FString& ShortName)
	{
		FString Trimmed = ShortName.TrimStartAndEnd();
		if (Trimmed.IsEmpty()) return TEXT("");
		if (Trimmed.StartsWith(TEXT("Narrative.Factions."))) return Trimmed;
		return FString::Printf(TEXT("Narrative.Factions.%s"), *Trimmed);
	}

	/** Convert full faction tag to short name */
	static FString ToShortFactionName(const FString& FullTag)
	{
		FString Trimmed = FullTag.TrimStartAndEnd();
		if (Trimmed.StartsWith(TEXT("Narrative.Factions.")))
		{
			return Trimmed.RightChop(19);
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

	/** Set factions from short names */
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
	// v4.5: Validation versioning (persisted - staleness survives restart)
	//=========================================================================

	/** GUID updated when asset lists are regenerated. Used for validation staleness detection. */
	UPROPERTY(EditAnywhere, Category = "Validation")
	FGuid ListsVersionGuid;

	/** Bump ListsVersionGuid to invalidate all cached validation */
	void BumpListsVersion()
	{
		ListsVersionGuid = FGuid::NewGuid();
	}

	//=========================================================================
	// v4.6: Generation tracking (for "Assets out of date" indicator)
	//=========================================================================

	/** Hash of all authored fields at last successful generation */
	UPROPERTY(VisibleAnywhere, Category = "Generation")
	uint32 LastGeneratedHash = 0;

	/** Timestamp of last generation attempt */
	UPROPERTY(VisibleAnywhere, Category = "Generation")
	FDateTime LastGeneratedTime;

	/** Number of failures in last generation (0 = full success) */
	UPROPERTY(VisibleAnywhere, Category = "Generation")
	int32 LastGenerateFailureCount = 0;

	/** Compute hash of all authored row data (excludes validation cache, transients) */
	uint32 ComputeAllRowsHash() const
	{
		uint32 Hash = 0;
		for (const FNPCTableRow& Row : Rows)
		{
			Hash = HashCombine(Hash, Row.ComputeEditableFieldsHash());
		}
		return Hash;
	}

	/** Check if assets are out of date (need regeneration) */
	bool AreAssetsOutOfDate() const
	{
		return ComputeAllRowsHash() != LastGeneratedHash;
	}

	/** Update generation tracking after successful generation */
	void OnGenerationComplete(int32 FailureCount)
	{
		LastGeneratedTime = FDateTime::Now();
		LastGenerateFailureCount = FailureCount;
		if (FailureCount == 0)
		{
			LastGeneratedHash = ComputeAllRowsHash();
		}
		// If failures > 0, hash stays stale (correct behavior)
	}

	//=========================================================================
	// Row Management
	//=========================================================================

	/** Add a new empty row */
	FNPCTableRow& AddRow()
	{
		FNPCTableRow& NewRow = Rows.AddDefaulted_GetRef();
		NewRow.RowId = FGuid::NewGuid();
		NewRow.Status = ENPCTableRowStatus::New;
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
			NewRow.Status = ENPCTableRowStatus::New;
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
