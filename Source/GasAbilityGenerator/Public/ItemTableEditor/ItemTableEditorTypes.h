// ItemTableEditorTypes.h
// Item Table Editor - Data types (v4.8 - Following NPC/Dialogue table patterns)
// Implements: XLSX 3-way sync, validation cache, soft delete, generation tracking
// Dynamic column visibility based on ItemType
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/SoftObjectPath.h"
#include "DialogueTableEditorTypes.h"  // For EValidationState (shared enum)
#include "ItemTableEditorTypes.generated.h"

/**
 * Row status enum for Item table (sync state)
 */
UENUM(BlueprintType)
enum class EItemTableRowStatus : uint8
{
	New UMETA(DisplayName = "New"),
	Modified UMETA(DisplayName = "Modified"),
	Synced UMETA(DisplayName = "Synced"),
	Error UMETA(DisplayName = "Error")
};

/**
 * Item type enum - determines dynamic column visibility
 */
UENUM(BlueprintType)
enum class EItemType : uint8
{
	Equippable UMETA(DisplayName = "Equippable"),
	RangedWeapon UMETA(DisplayName = "Ranged Weapon"),
	MeleeWeapon UMETA(DisplayName = "Melee Weapon"),
	Consumable UMETA(DisplayName = "Consumable")
};

/**
 * Single row in the Item Table Editor (v4.8 - 16 columns)
 * Represents one equippable item / weapon
 * Maps to UEquippableItem, URangedWeaponItem, UMeleeWeaponItem
 */
USTRUCT(BlueprintType)
struct GASABILITYGENERATOR_API FItemTableRow
{
	GENERATED_BODY()

	/** Unique row identifier for internal tracking */
	UPROPERTY()
	FGuid RowId;

	//=========================================================================
	// Core Identity (4 columns)
	//=========================================================================

	/** Row status: New, Modified, Synced, Error (auto-calculated) */
	UPROPERTY(VisibleAnywhere, Category = "Identity")
	EItemTableRowStatus Status = EItemTableRowStatus::New;

	/** Item Name - used for asset naming (EI_{ItemName}) */
	UPROPERTY(EditAnywhere, Category = "Identity")
	FString ItemName;

	/** Display name shown in-game */
	UPROPERTY(EditAnywhere, Category = "Identity")
	FString DisplayName;

	/** Item type - determines parent class and visible columns */
	UPROPERTY(EditAnywhere, Category = "Identity")
	EItemType ItemType = EItemType::Equippable;

	//=========================================================================
	// Equipment (3 columns)
	//=========================================================================

	/** Equipment slot tag (e.g., "Narrative.Equipment.Slot.Weapon") */
	UPROPERTY(EditAnywhere, Category = "Equipment")
	FString EquipmentSlot;

	/** Base gold value */
	UPROPERTY(EditAnywhere, Category = "Equipment")
	int32 BaseValue = 0;

	/** Weight in kg (inventory management) */
	UPROPERTY(EditAnywhere, Category = "Equipment")
	float Weight = 1.0f;

	//=========================================================================
	// Combat Stats (2 columns) - Dynamic visibility
	//=========================================================================

	/** Attack rating - bonus damage (weapons) */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackRating = 0.0f;

	/** Armor rating - damage reduction (armor) */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float ArmorRating = 0.0f;

	//=========================================================================
	// References (2 columns)
	//=========================================================================

	/** Equipment modifier GameplayEffect (GE_*) */
	UPROPERTY(EditAnywhere, Category = "References")
	FSoftObjectPath ModifierGE;

	/** Abilities granted when equipped (comma-separated GA_* names) */
	UPROPERTY(EditAnywhere, Category = "References")
	FString Abilities;

	//=========================================================================
	// Weapon Config (1 column) - Token-based, dynamic visibility
	//=========================================================================

	/** Weapon config token: Weapon(Damage=50,ClipSize=30,ReloadTime=2.5,Range=2000) */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	FString WeaponConfig;

	//=========================================================================
	// Tags & Stacking (3 columns)
	//=========================================================================

	/** Item tags (comma-separated gameplay tags) */
	UPROPERTY(EditAnywhere, Category = "Tags")
	FString ItemTags;

	/** Can this item stack in inventory */
	UPROPERTY(EditAnywhere, Category = "Stacking")
	bool bStackable = false;

	/** Maximum stack size (if stackable) */
	UPROPERTY(EditAnywhere, Category = "Stacking")
	int32 MaxStackSize = 1;

	//=========================================================================
	// Meta (1 column)
	//=========================================================================

	/** Designer notes/comments */
	UPROPERTY(EditAnywhere, Category = "Meta")
	FString Notes;

	//=========================================================================
	// Soft Delete (v4.6 pattern)
	//=========================================================================

	/** Soft delete flag - row skipped during generation */
	UPROPERTY(EditAnywhere, Category = "Meta")
	bool bDeleted = false;

	//=========================================================================
	// Internal (not displayed as columns)
	//=========================================================================

	/** Generated Item asset path (after generation) */
	UPROPERTY(VisibleAnywhere, Category = "Internal")
	FSoftObjectPath GeneratedItem;

	//=========================================================================
	// Validation cache (Transient - UI only, not serialized)
	//=========================================================================

	/** Current validation state (Unknown/Valid/Invalid) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Validation")
	EValidationState ValidationState = EValidationState::Unknown;

	/** Summary of validation issues */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Validation")
	FString ValidationSummary;

	/** Count of validation issues */
	UPROPERTY(Transient)
	int32 ValidationIssueCount = 0;

	/** Hash of inputs used for validation */
	UPROPERTY(Transient)
	uint32 ValidationInputHash = 0;

	//=========================================================================
	// XLSX Sync Hash (persisted - for 3-way merge comparison)
	//=========================================================================

	/** Hash of row content at last successful sync operation */
	UPROPERTY(EditAnywhere, Category = "Sync")
	int64 LastSyncedHash = 0;

	/** Compute sync hash for 3-way merge comparison */
	int64 ComputeSyncHash() const
	{
		return static_cast<int64>(ComputeEditableFieldsHash());
	}

	/** Invalidate cached validation */
	void InvalidateValidation()
	{
		ValidationState = EValidationState::Unknown;
		ValidationSummary.Empty();
		ValidationIssueCount = 0;
		ValidationInputHash = 0;
	}

	/** Get validation state accessor */
	EValidationState GetValidationState() const { return ValidationState; }

	/** Compute hash of editable fields */
	uint32 ComputeEditableFieldsHash() const
	{
		uint32 Hash = 0;
		Hash = HashCombine(Hash, GetTypeHash(ItemName));
		Hash = HashCombine(Hash, GetTypeHash(DisplayName));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(ItemType)));
		Hash = HashCombine(Hash, GetTypeHash(EquipmentSlot));
		Hash = HashCombine(Hash, GetTypeHash(BaseValue));
		Hash = HashCombine(Hash, GetTypeHash(Weight));
		Hash = HashCombine(Hash, GetTypeHash(AttackRating));
		Hash = HashCombine(Hash, GetTypeHash(ArmorRating));
		Hash = HashCombine(Hash, GetTypeHash(ModifierGE.ToString()));
		Hash = HashCombine(Hash, GetTypeHash(Abilities));
		Hash = HashCombine(Hash, GetTypeHash(WeaponConfig));
		Hash = HashCombine(Hash, GetTypeHash(ItemTags));
		Hash = HashCombine(Hash, GetTypeHash(bStackable));
		Hash = HashCombine(Hash, GetTypeHash(MaxStackSize));
		Hash = HashCombine(Hash, GetTypeHash(bDeleted));
		return Hash;
	}

	//=========================================================================
	// Methods
	//=========================================================================

	FItemTableRow()
	{
		RowId = FGuid::NewGuid();
	}

	bool IsValid() const
	{
		return !ItemName.IsEmpty();
	}

	/** Check if weapon columns should be visible */
	bool IsWeapon() const
	{
		return ItemType == EItemType::RangedWeapon || ItemType == EItemType::MeleeWeapon;
	}

	/** Check if armor columns should be visible */
	bool IsArmor() const
	{
		return ItemType == EItemType::Equippable && EquipmentSlot.Contains(TEXT("Armor"));
	}

	/** Get parent class name based on item type */
	FString GetParentClassName() const
	{
		switch (ItemType)
		{
			case EItemType::RangedWeapon: return TEXT("RangedWeaponItem");
			case EItemType::MeleeWeapon: return TEXT("MeleeWeaponItem");
			case EItemType::Consumable: return TEXT("NarrativeItem");
			case EItemType::Equippable:
			default: return TEXT("EquippableItem");
		}
	}

	/** Get status color for UI */
	FLinearColor GetStatusColor() const
	{
		switch (Status)
		{
			case EItemTableRowStatus::New: return FLinearColor(0.2f, 0.6f, 1.0f);
			case EItemTableRowStatus::Modified: return FLinearColor(1.0f, 0.8f, 0.2f);
			case EItemTableRowStatus::Synced: return FLinearColor(0.2f, 0.8f, 0.2f);
			case EItemTableRowStatus::Error: return FLinearColor(1.0f, 0.2f, 0.2f);
			default: return FLinearColor::White;
		}
	}

	/** Get status as string */
	FString GetStatusString() const
	{
		switch (Status)
		{
			case EItemTableRowStatus::New: return TEXT("New");
			case EItemTableRowStatus::Modified: return TEXT("Modified");
			case EItemTableRowStatus::Synced: return TEXT("Synced");
			case EItemTableRowStatus::Error: return TEXT("Error");
			default: return TEXT("Unknown");
		}
	}

	/** Get item type as string */
	FString GetItemTypeString() const
	{
		switch (ItemType)
		{
			case EItemType::Equippable: return TEXT("Equippable");
			case EItemType::RangedWeapon: return TEXT("Ranged");
			case EItemType::MeleeWeapon: return TEXT("Melee");
			case EItemType::Consumable: return TEXT("Consumable");
			default: return TEXT("Unknown");
		}
	}

	/** Get item type color */
	FLinearColor GetItemTypeColor() const
	{
		switch (ItemType)
		{
			case EItemType::RangedWeapon: return FLinearColor(0.8f, 0.4f, 0.2f); // Orange
			case EItemType::MeleeWeapon: return FLinearColor(0.8f, 0.2f, 0.2f); // Red
			case EItemType::Consumable: return FLinearColor(0.2f, 0.8f, 0.4f); // Green
			case EItemType::Equippable:
			default: return FLinearColor(0.4f, 0.4f, 0.8f); // Blue
		}
	}

	/** Get validation color for row stripe */
	FLinearColor GetValidationColor() const
	{
		switch (ValidationState)
		{
			case EValidationState::Valid: return FLinearColor(0.2f, 0.8f, 0.2f);
			case EValidationState::Invalid: return FLinearColor(1.0f, 0.2f, 0.2f);
			case EValidationState::Unknown: return FLinearColor(1.0f, 0.8f, 0.2f);
			default: return FLinearColor::White;
		}
	}

	/** Parse item type from string */
	static EItemType ParseItemType(const FString& TypeString)
	{
		if (TypeString.Equals(TEXT("Ranged"), ESearchCase::IgnoreCase) ||
			TypeString.Equals(TEXT("RangedWeapon"), ESearchCase::IgnoreCase)) return EItemType::RangedWeapon;
		if (TypeString.Equals(TEXT("Melee"), ESearchCase::IgnoreCase) ||
			TypeString.Equals(TEXT("MeleeWeapon"), ESearchCase::IgnoreCase)) return EItemType::MeleeWeapon;
		if (TypeString.Equals(TEXT("Consumable"), ESearchCase::IgnoreCase)) return EItemType::Consumable;
		return EItemType::Equippable;
	}

	//=========================================================================
	// Weapon Config Token Helpers
	//=========================================================================

	/** Parse weapon config token into key-value pairs */
	TMap<FString, FString> ParseWeaponConfig() const
	{
		TMap<FString, FString> Result;
		// Expected format: Weapon(Damage=50,ClipSize=30,ReloadTime=2.5)
		FString Content = WeaponConfig;
		if (Content.StartsWith(TEXT("Weapon(")))
		{
			Content = Content.RightChop(7); // Remove "Weapon("
			Content = Content.LeftChop(1);  // Remove trailing ")"
		}

		TArray<FString> Pairs;
		Content.ParseIntoArray(Pairs, TEXT(","));
		for (const FString& Pair : Pairs)
		{
			FString Key, Value;
			if (Pair.Split(TEXT("="), &Key, &Value))
			{
				Result.Add(Key.TrimStartAndEnd(), Value.TrimStartAndEnd());
			}
		}
		return Result;
	}

	/** Build weapon config token from key-value pairs */
	void SetWeaponConfigFromMap(const TMap<FString, FString>& Config)
	{
		TArray<FString> Pairs;
		for (const auto& Pair : Config)
		{
			Pairs.Add(FString::Printf(TEXT("%s=%s"), *Pair.Key, *Pair.Value));
		}
		WeaponConfig = FString::Printf(TEXT("Weapon(%s)"), *FString::Join(Pairs, TEXT(",")));
	}
};

/**
 * Container for all Item table data
 * Saved as a single DataAsset for persistence
 */
UCLASS(BlueprintType)
class GASABILITYGENERATOR_API UItemTableData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** All item rows in the table */
	UPROPERTY(EditAnywhere, Category = "Data")
	TArray<FItemTableRow> Rows;

	/** Project name for asset paths */
	UPROPERTY(EditAnywhere, Category = "Settings")
	FString ProjectName = TEXT("NP22B57");

	/** Default output folder */
	UPROPERTY(EditAnywhere, Category = "Settings")
	FString OutputFolder = TEXT("/Game/Items");

	/** Last sync timestamp */
	UPROPERTY(VisibleAnywhere, Category = "Metadata")
	FDateTime LastSyncTime;

	//=========================================================================
	// Validation versioning
	//=========================================================================

	/** GUID updated when asset lists are regenerated */
	UPROPERTY(EditAnywhere, Category = "Validation")
	FGuid ListsVersionGuid;

	void BumpListsVersion()
	{
		ListsVersionGuid = FGuid::NewGuid();
	}

	//=========================================================================
	// Generation tracking
	//=========================================================================

	/** Hash of all authored fields at last successful generation */
	UPROPERTY(VisibleAnywhere, Category = "Generation")
	uint32 LastGeneratedHash = 0;

	/** Timestamp of last generation attempt */
	UPROPERTY(VisibleAnywhere, Category = "Generation")
	FDateTime LastGeneratedTime;

	/** Number of failures in last generation */
	UPROPERTY(VisibleAnywhere, Category = "Generation")
	int32 LastGenerateFailureCount = 0;

	/** Compute hash of all authored row data */
	uint32 ComputeAllRowsHash() const
	{
		uint32 Hash = 0;
		for (const FItemTableRow& Row : Rows)
		{
			Hash = HashCombine(Hash, Row.ComputeEditableFieldsHash());
		}
		return Hash;
	}

	/** Check if assets are out of date */
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
	}

	//=========================================================================
	// Row Management
	//=========================================================================

	/** Add a new empty row */
	FItemTableRow& AddRow()
	{
		FItemTableRow& NewRow = Rows.AddDefaulted_GetRef();
		NewRow.RowId = FGuid::NewGuid();
		NewRow.Status = EItemTableRowStatus::New;
		return NewRow;
	}

	/** Duplicate an existing row */
	FItemTableRow* DuplicateRow(int32 SourceIndex)
	{
		if (Rows.IsValidIndex(SourceIndex))
		{
			FItemTableRow NewRow = Rows[SourceIndex];
			NewRow.RowId = FGuid::NewGuid();
			NewRow.ItemName += TEXT("_Copy");
			NewRow.Status = EItemTableRowStatus::New;
			NewRow.GeneratedItem.Reset();
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
	FItemTableRow* FindRowByGuid(const FGuid& InRowId)
	{
		return Rows.FindByPredicate([&InRowId](const FItemTableRow& Row)
		{
			return Row.RowId == InRowId;
		});
	}

	/** Find row index by GUID */
	int32 FindRowIndexByGuid(const FGuid& InRowId) const
	{
		return Rows.IndexOfByPredicate([&InRowId](const FItemTableRow& Row)
		{
			return Row.RowId == InRowId;
		});
	}

	/** Get rows by item type */
	TArray<FItemTableRow*> GetRowsByType(EItemType Type)
	{
		TArray<FItemTableRow*> Result;
		for (FItemTableRow& Row : Rows)
		{
			if (Row.ItemType == Type && !Row.bDeleted)
			{
				Result.Add(&Row);
			}
		}
		return Result;
	}

	/** Get row count */
	int32 GetRowCount() const { return Rows.Num(); }
};
