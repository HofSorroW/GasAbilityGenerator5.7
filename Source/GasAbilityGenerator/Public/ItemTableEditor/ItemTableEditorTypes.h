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
 * Maps to NarrativePro item class hierarchy:
 * UNarrativeItem (base)
 *   UAmmoItem - Ammo for ranged weapons
 *   UGameplayEffectItem - Consumables that apply GE (potions, food)
 *   UWeaponAttachmentItem - Weapon attachments (scopes, grips)
 *   UEquippableItem - Base for equipment
 *     UEquippableItem_Clothing - Clothing with mesh
 *     UThrowableWeaponItem - Throwable weapons
 *     UWeaponItem - Base for weapons
 *       URangedWeaponItem - Ranged (rifles, bows)
 *       UMeleeWeaponItem - Melee (swords, axes)
 *       UMagicWeaponItem - Magic (staves, wands)
 */
UENUM(BlueprintType)
enum class EItemType : uint8
{
	// Base items (UNarrativeItem derivatives)
	Consumable UMETA(DisplayName = "Consumable"),           // UGameplayEffectItem
	Ammo UMETA(DisplayName = "Ammo"),                       // UAmmoItem
	WeaponAttachment UMETA(DisplayName = "Weapon Attachment"), // UWeaponAttachmentItem

	// Equipment (UEquippableItem derivatives)
	Equippable UMETA(DisplayName = "Equippable"),           // UEquippableItem
	Clothing UMETA(DisplayName = "Clothing"),               // UEquippableItem_Clothing
	ThrowableWeapon UMETA(DisplayName = "Throwable Weapon"), // UThrowableWeaponItem

	// Weapons (UWeaponItem derivatives)
	MeleeWeapon UMETA(DisplayName = "Melee Weapon"),        // UMeleeWeaponItem
	RangedWeapon UMETA(DisplayName = "Ranged Weapon"),      // URangedWeaponItem
	MagicWeapon UMETA(DisplayName = "Magic Weapon")         // UMagicWeaponItem
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
	UPROPERTY(meta = (IgnoreForMemberInitializationTest))
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
	// Item Description & Score
	//=========================================================================

	/** Item description shown in inventory/tooltips */
	UPROPERTY(EditAnywhere, Category = "Description")
	FString Description;

	/** AI priority score (0 = use BaseValue as fallback) */
	UPROPERTY(EditAnywhere, Category = "Description")
	float BaseScore = 0.0f;

	//=========================================================================
	// Combat Stats (Equipment) - Dynamic visibility
	//=========================================================================

	/** Attack rating - bonus damage (UEquippableItem) */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackRating = 0.0f;

	/** Armor rating - damage reduction (UEquippableItem) */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float ArmorRating = 0.0f;

	/** Stealth rating - stealth bonus/penalty (UEquippableItem) */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float StealthRating = 0.0f;

	//=========================================================================
	// Weapon Stats (UWeaponItem) - Dynamic visibility for weapons
	//=========================================================================

	/** Base attack damage (UWeaponItem) */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	float AttackDamage = 0.0f;

	/** Heavy attack multiplier (UWeaponItem) */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	float HeavyAttackDamageMultiplier = 1.5f;

	/** Weapon hand: TwoHanded, MainHand, OffHand, DualWieldable (UWeaponItem) */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	FString WeaponHand = TEXT("TwoHanded");

	/** Clip size for ammo-based weapons (UWeaponItem) */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	int32 ClipSize = 0;

	/** Required ammo class name (UWeaponItem) */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	FString RequiredAmmo;

	/** Allow manual reload (UWeaponItem) */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	bool bAllowManualReload = true;

	/** Bot attack range (UWeaponItem) */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	float BotAttackRange = 200.0f;

	//=========================================================================
	// Ranged Weapon Stats (URangedWeaponItem) - Dynamic visibility
	//=========================================================================

	/** Base spread in degrees (URangedWeaponItem) */
	UPROPERTY(EditAnywhere, Category = "RangedWeapon")
	float BaseSpreadDegrees = 1.0f;

	/** Max spread in degrees (URangedWeaponItem) */
	UPROPERTY(EditAnywhere, Category = "RangedWeapon")
	float MaxSpreadDegrees = 8.0f;

	/** Spread increase per shot (URangedWeaponItem) */
	UPROPERTY(EditAnywhere, Category = "RangedWeapon")
	float SpreadFireBump = 0.5f;

	/** Spread decrease speed (URangedWeaponItem) */
	UPROPERTY(EditAnywhere, Category = "RangedWeapon")
	float SpreadDecreaseSpeed = 4.0f;

	/** Aim FOV percentage 0.1-1.0 (URangedWeaponItem) */
	UPROPERTY(EditAnywhere, Category = "RangedWeapon")
	float AimFOVPct = 0.7f;

	//=========================================================================
	// Consumable Stats (UGameplayEffectItem) - Dynamic visibility
	//=========================================================================

	/** Consume one item per use (UNarrativeItem) */
	UPROPERTY(EditAnywhere, Category = "Consumable")
	bool bConsumeOnUse = true;

	/** Cooldown between uses in seconds (UNarrativeItem) */
	UPROPERTY(EditAnywhere, Category = "Consumable")
	float UseRechargeDuration = 0.0f;

	/** Can be activated/deactivated (UNarrativeItem) */
	UPROPERTY(EditAnywhere, Category = "Consumable")
	bool bCanActivate = false;

	/** GameplayEffect to apply when used (UGameplayEffectItem) */
	UPROPERTY(EditAnywhere, Category = "Consumable")
	FString GameplayEffectClass;

	//=========================================================================
	// References
	//=========================================================================

	/** Equipment modifier GameplayEffect (GE_*) */
	UPROPERTY(EditAnywhere, Category = "References")
	FSoftObjectPath ModifierGE;

	/** Abilities granted when equipped (comma-separated GA_* names) */
	UPROPERTY(EditAnywhere, Category = "References")
	FString Abilities;

	//=========================================================================
	// Fragments (1 column) - Modifiers that can be added to items
	//=========================================================================

	/** Item fragments: AmmoFragment, PoisonableFragment (comma-separated) */
	UPROPERTY(EditAnywhere, Category = "Fragments")
	FString Fragments;

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
		// Identity
		Hash = HashCombine(Hash, GetTypeHash(ItemName));
		Hash = HashCombine(Hash, GetTypeHash(DisplayName));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(ItemType)));
		Hash = HashCombine(Hash, GetTypeHash(EquipmentSlot));
		Hash = HashCombine(Hash, GetTypeHash(BaseValue));
		Hash = HashCombine(Hash, GetTypeHash(Weight));
		// Description
		Hash = HashCombine(Hash, GetTypeHash(Description));
		Hash = HashCombine(Hash, GetTypeHash(BaseScore));
		// Combat
		Hash = HashCombine(Hash, GetTypeHash(AttackRating));
		Hash = HashCombine(Hash, GetTypeHash(ArmorRating));
		Hash = HashCombine(Hash, GetTypeHash(StealthRating));
		// Weapon
		Hash = HashCombine(Hash, GetTypeHash(AttackDamage));
		Hash = HashCombine(Hash, GetTypeHash(HeavyAttackDamageMultiplier));
		Hash = HashCombine(Hash, GetTypeHash(WeaponHand));
		Hash = HashCombine(Hash, GetTypeHash(ClipSize));
		Hash = HashCombine(Hash, GetTypeHash(RequiredAmmo));
		Hash = HashCombine(Hash, GetTypeHash(bAllowManualReload));
		Hash = HashCombine(Hash, GetTypeHash(BotAttackRange));
		// Ranged
		Hash = HashCombine(Hash, GetTypeHash(BaseSpreadDegrees));
		Hash = HashCombine(Hash, GetTypeHash(MaxSpreadDegrees));
		Hash = HashCombine(Hash, GetTypeHash(SpreadFireBump));
		Hash = HashCombine(Hash, GetTypeHash(SpreadDecreaseSpeed));
		Hash = HashCombine(Hash, GetTypeHash(AimFOVPct));
		// Consumable
		Hash = HashCombine(Hash, GetTypeHash(bConsumeOnUse));
		Hash = HashCombine(Hash, GetTypeHash(UseRechargeDuration));
		Hash = HashCombine(Hash, GetTypeHash(bCanActivate));
		Hash = HashCombine(Hash, GetTypeHash(GameplayEffectClass));
		// References
		Hash = HashCombine(Hash, GetTypeHash(ModifierGE.ToString()));
		Hash = HashCombine(Hash, GetTypeHash(Abilities));
		Hash = HashCombine(Hash, GetTypeHash(Fragments));
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
		return ItemType == EItemType::RangedWeapon ||
		       ItemType == EItemType::MeleeWeapon ||
		       ItemType == EItemType::MagicWeapon ||
		       ItemType == EItemType::ThrowableWeapon;
	}

	/** Check if armor columns should be visible */
	bool IsArmor() const
	{
		return (ItemType == EItemType::Equippable || ItemType == EItemType::Clothing) &&
		       EquipmentSlot.Contains(TEXT("Armor"));
	}

	/** Check if this is an equippable item (has equipment stats) */
	bool IsEquipment() const
	{
		return ItemType == EItemType::Equippable ||
		       ItemType == EItemType::Clothing ||
		       IsWeapon();
	}

	/** Check if ranged weapon columns should be visible */
	bool IsRangedWeapon() const
	{
		return ItemType == EItemType::RangedWeapon;
	}

	/** Check if consumable columns should be visible */
	bool IsConsumable() const
	{
		return ItemType == EItemType::Consumable;
	}

	/** Check if ammo columns should be visible */
	bool IsAmmo() const
	{
		return ItemType == EItemType::Ammo;
	}

	/** Check if attachment columns should be visible */
	bool IsWeaponAttachment() const
	{
		return ItemType == EItemType::WeaponAttachment;
	}

	/** Get parent class name based on item type */
	FString GetParentClassName() const
	{
		switch (ItemType)
		{
			case EItemType::RangedWeapon: return TEXT("RangedWeaponItem");
			case EItemType::MeleeWeapon: return TEXT("MeleeWeaponItem");
			case EItemType::MagicWeapon: return TEXT("MagicWeaponItem");
			case EItemType::ThrowableWeapon: return TEXT("ThrowableWeaponItem");
			case EItemType::Clothing: return TEXT("EquippableItem_Clothing");
			case EItemType::Consumable: return TEXT("GameplayEffectItem");
			case EItemType::Ammo: return TEXT("AmmoItem");
			case EItemType::WeaponAttachment: return TEXT("WeaponAttachmentItem");
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
			case EItemType::Consumable: return TEXT("Consumable");
			case EItemType::Ammo: return TEXT("Ammo");
			case EItemType::WeaponAttachment: return TEXT("Attachment");
			case EItemType::Equippable: return TEXT("Equippable");
			case EItemType::Clothing: return TEXT("Clothing");
			case EItemType::ThrowableWeapon: return TEXT("Throwable");
			case EItemType::MeleeWeapon: return TEXT("Melee");
			case EItemType::RangedWeapon: return TEXT("Ranged");
			case EItemType::MagicWeapon: return TEXT("Magic");
			default: return TEXT("Unknown");
		}
	}

	/** Get item type color */
	FLinearColor GetItemTypeColor() const
	{
		switch (ItemType)
		{
			// Weapons - warm colors
			case EItemType::RangedWeapon: return FLinearColor(0.8f, 0.4f, 0.2f);     // Orange
			case EItemType::MeleeWeapon: return FLinearColor(0.8f, 0.2f, 0.2f);      // Red
			case EItemType::MagicWeapon: return FLinearColor(0.6f, 0.2f, 0.8f);      // Purple
			case EItemType::ThrowableWeapon: return FLinearColor(0.8f, 0.6f, 0.2f);  // Yellow-Orange

			// Equipment - blue tones
			case EItemType::Equippable: return FLinearColor(0.4f, 0.4f, 0.8f);       // Blue
			case EItemType::Clothing: return FLinearColor(0.3f, 0.6f, 0.8f);         // Light Blue

			// Consumables & attachments - green tones
			case EItemType::Consumable: return FLinearColor(0.2f, 0.8f, 0.4f);       // Green
			case EItemType::Ammo: return FLinearColor(0.6f, 0.6f, 0.3f);             // Olive
			case EItemType::WeaponAttachment: return FLinearColor(0.5f, 0.5f, 0.5f); // Gray

			default: return FLinearColor::White;
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
		// Weapons
		if (TypeString.Equals(TEXT("Ranged"), ESearchCase::IgnoreCase) ||
			TypeString.Equals(TEXT("RangedWeapon"), ESearchCase::IgnoreCase) ||
			TypeString.Equals(TEXT("RangedWeaponItem"), ESearchCase::IgnoreCase)) return EItemType::RangedWeapon;
		if (TypeString.Equals(TEXT("Melee"), ESearchCase::IgnoreCase) ||
			TypeString.Equals(TEXT("MeleeWeapon"), ESearchCase::IgnoreCase) ||
			TypeString.Equals(TEXT("MeleeWeaponItem"), ESearchCase::IgnoreCase)) return EItemType::MeleeWeapon;
		if (TypeString.Equals(TEXT("Magic"), ESearchCase::IgnoreCase) ||
			TypeString.Equals(TEXT("MagicWeapon"), ESearchCase::IgnoreCase) ||
			TypeString.Equals(TEXT("MagicWeaponItem"), ESearchCase::IgnoreCase)) return EItemType::MagicWeapon;
		if (TypeString.Equals(TEXT("Throwable"), ESearchCase::IgnoreCase) ||
			TypeString.Equals(TEXT("ThrowableWeapon"), ESearchCase::IgnoreCase) ||
			TypeString.Equals(TEXT("ThrowableWeaponItem"), ESearchCase::IgnoreCase)) return EItemType::ThrowableWeapon;

		// Equipment
		if (TypeString.Equals(TEXT("Clothing"), ESearchCase::IgnoreCase) ||
			TypeString.Equals(TEXT("EquippableItem_Clothing"), ESearchCase::IgnoreCase)) return EItemType::Clothing;

		// Base items
		if (TypeString.Equals(TEXT("Consumable"), ESearchCase::IgnoreCase) ||
			TypeString.Equals(TEXT("GameplayEffectItem"), ESearchCase::IgnoreCase)) return EItemType::Consumable;
		if (TypeString.Equals(TEXT("Ammo"), ESearchCase::IgnoreCase) ||
			TypeString.Equals(TEXT("AmmoItem"), ESearchCase::IgnoreCase)) return EItemType::Ammo;
		if (TypeString.Equals(TEXT("Attachment"), ESearchCase::IgnoreCase) ||
			TypeString.Equals(TEXT("WeaponAttachment"), ESearchCase::IgnoreCase) ||
			TypeString.Equals(TEXT("WeaponAttachmentItem"), ESearchCase::IgnoreCase)) return EItemType::WeaponAttachment;

		return EItemType::Equippable;
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
