// GasAbilityGenerator - Item Table Validator
// v4.8: Implementation

#include "ItemTableValidator.h"

FItemValidationResult FItemTableValidator::ValidateAll(const TArray<FItemTableRow>& Rows)
{
	FItemValidationResult Result;

	for (const FItemTableRow& Row : Rows)
	{
		if (!Row.bDeleted)
		{
			TArray<FItemValidationIssue> RowIssues = ValidateRow(Row, Rows);
			Result.Issues.Append(RowIssues);
		}
	}

	return Result;
}

TArray<FItemValidationIssue> FItemTableValidator::ValidateRow(const FItemTableRow& Row, const TArray<FItemTableRow>& AllRows)
{
	TArray<FItemValidationIssue> Issues;

	// Required fields
	if (Row.ItemName.IsEmpty())
	{
		Issues.Add(FItemValidationIssue(
			EItemValidationSeverity::Error,
			TEXT("Unknown"),
			TEXT("ItemName"),
			TEXT("Item name is required")));
		return Issues;  // Can't validate further without name
	}

	// Item name uniqueness
	if (!IsItemNameUnique(Row.ItemName, Row.RowId, AllRows))
	{
		Issues.Add(FItemValidationIssue(
			EItemValidationSeverity::Error,
			Row.ItemName,
			TEXT("ItemName"),
			TEXT("Item name must be unique")));
	}

	// Display name warning
	if (Row.DisplayName.IsEmpty())
	{
		Issues.Add(FItemValidationIssue(
			EItemValidationSeverity::Warning,
			Row.ItemName,
			TEXT("DisplayName"),
			TEXT("Display name is empty - item name will be used")));
	}

	// Type-specific validation
	Issues.Append(ValidateEquipmentSlot(Row));
	Issues.Append(ValidateCombatStats(Row));
	Issues.Append(ValidateWeaponProperties(Row));
	Issues.Append(ValidateConsumableProperties(Row));
	Issues.Append(ValidateAssetReferences(Row));
	Issues.Append(ValidateStackingProperties(Row));

	return Issues;
}

FItemValidationResult FItemTableValidator::ValidateAllAndCache(TArray<FItemTableRow>& Rows, const FGuid& ListsVersionGuid)
{
	FItemValidationResult Result;

	for (FItemTableRow& Row : Rows)
	{
		if (!Row.bDeleted)
		{
			TArray<FItemValidationIssue> RowIssues = ValidateRowAndCache(Row, Rows, ListsVersionGuid);
			Result.Issues.Append(RowIssues);
		}
	}

	return Result;
}

TArray<FItemValidationIssue> FItemTableValidator::ValidateRowAndCache(FItemTableRow& Row, const TArray<FItemTableRow>& AllRows, const FGuid& ListsVersionGuid)
{
	// Check if validation is stale
	uint32 CurrentInputHash = ComputeValidationInputHash(Row, ListsVersionGuid);
	if (Row.ValidationInputHash == CurrentInputHash && Row.ValidationState != EValidationState::Unknown)
	{
		// Return cached result
		return TArray<FItemValidationIssue>();
	}

	// Perform validation
	TArray<FItemValidationIssue> Issues = ValidateRow(Row, AllRows);

	// Update cache
	Row.ValidationInputHash = CurrentInputHash;
	Row.ValidationIssueCount = Issues.Num();

	if (Issues.Num() == 0)
	{
		Row.ValidationState = EValidationState::Valid;
		Row.ValidationSummary = TEXT("");
	}
	else
	{
		bool bHasError = false;
		for (const auto& Issue : Issues)
		{
			if (Issue.Severity == EItemValidationSeverity::Error)
			{
				bHasError = true;
				break;
			}
		}
		Row.ValidationState = bHasError ? EValidationState::Invalid : EValidationState::Valid;

		// Build summary
		TArray<FString> IssueSummaries;
		for (const auto& Issue : Issues)
		{
			IssueSummaries.Add(Issue.Message);
		}
		Row.ValidationSummary = FString::Join(IssueSummaries, TEXT("; "));
	}

	return Issues;
}

uint32 FItemTableValidator::ComputeValidationInputHash(const FItemTableRow& Row, const FGuid& ListsVersionGuid)
{
	uint32 Hash = Row.ComputeEditableFieldsHash();
	Hash = HashCombine(Hash, GetTypeHash(ListsVersionGuid));
	return Hash;
}

bool FItemTableValidator::IsItemNameUnique(const FString& ItemName, const FGuid& RowId, const TArray<FItemTableRow>& AllRows)
{
	for (const FItemTableRow& Row : AllRows)
	{
		if (Row.RowId != RowId && !Row.bDeleted && Row.ItemName == ItemName)
		{
			return false;
		}
	}
	return true;
}

TArray<FItemValidationIssue> FItemTableValidator::ValidateEquipmentSlot(const FItemTableRow& Row)
{
	TArray<FItemValidationIssue> Issues;

	// Equipment slot required for equippable items
	if (Row.ItemType != EItemType::Consumable && Row.EquipmentSlot.IsEmpty())
	{
		Issues.Add(FItemValidationIssue(
			EItemValidationSeverity::Warning,
			Row.ItemName,
			TEXT("EquipmentSlot"),
			TEXT("Equipment slot not specified for equippable item")));
	}

	// Validate slot format (should be a gameplay tag)
	if (!Row.EquipmentSlot.IsEmpty() && !Row.EquipmentSlot.Contains(TEXT(".")))
	{
		Issues.Add(FItemValidationIssue(
			EItemValidationSeverity::Warning,
			Row.ItemName,
			TEXT("EquipmentSlot"),
			TEXT("Equipment slot should be a gameplay tag (e.g., Narrative.Equipment.Slot.Weapon)")));
	}

	return Issues;
}

TArray<FItemValidationIssue> FItemTableValidator::ValidateWeaponProperties(const FItemTableRow& Row)
{
	TArray<FItemValidationIssue> Issues;

	if (Row.IsWeapon())
	{
		// Weapons should have attack damage
		if (Row.AttackDamage <= 0)
		{
			Issues.Add(FItemValidationIssue(
				EItemValidationSeverity::Warning,
				Row.ItemName,
				TEXT("AttackDamage"),
				TEXT("Weapon has no attack damage set")));
		}

		// Ranged weapons should have clip size
		if (Row.IsRangedWeapon() && Row.ClipSize <= 0)
		{
			Issues.Add(FItemValidationIssue(
				EItemValidationSeverity::Warning,
				Row.ItemName,
				TEXT("ClipSize"),
				TEXT("Ranged weapon has no clip size set")));
		}

		// Validate weapon hand
		if (!Row.WeaponHand.IsEmpty())
		{
			if (Row.WeaponHand != TEXT("TwoHanded") &&
				Row.WeaponHand != TEXT("MainHand") &&
				Row.WeaponHand != TEXT("OffHand") &&
				Row.WeaponHand != TEXT("DualWieldable"))
			{
				Issues.Add(FItemValidationIssue(
					EItemValidationSeverity::Warning,
					Row.ItemName,
					TEXT("WeaponHand"),
					TEXT("Invalid weapon hand - use TwoHanded, MainHand, OffHand, or DualWieldable")));
			}
		}

		// Ranged spread validation
		if (Row.IsRangedWeapon())
		{
			if (Row.BaseSpreadDegrees > Row.MaxSpreadDegrees)
			{
				Issues.Add(FItemValidationIssue(
					EItemValidationSeverity::Warning,
					Row.ItemName,
					TEXT("BaseSpreadDegrees"),
					TEXT("Base spread is greater than max spread")));
			}

			if (Row.AimFOVPct <= 0 || Row.AimFOVPct > 1.0f)
			{
				Issues.Add(FItemValidationIssue(
					EItemValidationSeverity::Warning,
					Row.ItemName,
					TEXT("AimFOVPct"),
					TEXT("Aim FOV percentage should be between 0 and 1")));
			}
		}

		// Heavy attack multiplier validation
		if (Row.HeavyAttackDamageMultiplier < 1.0f)
		{
			Issues.Add(FItemValidationIssue(
				EItemValidationSeverity::Warning,
				Row.ItemName,
				TEXT("HeavyAttackDamageMultiplier"),
				TEXT("Heavy attack multiplier is less than 1.0 - heavy attacks will do less damage")));
		}
	}
	else
	{
		// Non-weapons shouldn't have weapon-specific properties set
		if (Row.AttackDamage > 0)
		{
			Issues.Add(FItemValidationIssue(
				EItemValidationSeverity::Warning,
				Row.ItemName,
				TEXT("AttackDamage"),
				TEXT("Attack damage set on non-weapon item")));
		}

		if (Row.ClipSize > 0)
		{
			Issues.Add(FItemValidationIssue(
				EItemValidationSeverity::Warning,
				Row.ItemName,
				TEXT("ClipSize"),
				TEXT("Clip size set on non-weapon item")));
		}
	}

	return Issues;
}

TArray<FItemValidationIssue> FItemTableValidator::ValidateConsumableProperties(const FItemTableRow& Row)
{
	TArray<FItemValidationIssue> Issues;

	if (Row.IsConsumable())
	{
		// Consumables should typically consume on use
		if (!Row.bConsumeOnUse)
		{
			Issues.Add(FItemValidationIssue(
				EItemValidationSeverity::Warning,
				Row.ItemName,
				TEXT("bConsumeOnUse"),
				TEXT("Consumable item doesn't consume on use - is this intentional?")));
		}

		// If has gameplay effect, validate format
		if (!Row.GameplayEffectClass.IsEmpty() && !Row.GameplayEffectClass.StartsWith(TEXT("GE_")))
		{
			Issues.Add(FItemValidationIssue(
				EItemValidationSeverity::Warning,
				Row.ItemName,
				TEXT("GameplayEffectClass"),
				TEXT("Gameplay effect should start with GE_ prefix")));
		}
	}
	else
	{
		// Non-consumables shouldn't have consumable properties
		if (Row.bConsumeOnUse)
		{
			Issues.Add(FItemValidationIssue(
				EItemValidationSeverity::Warning,
				Row.ItemName,
				TEXT("bConsumeOnUse"),
				TEXT("Non-consumable item has consume on use enabled")));
		}
	}

	// Ammo validation
	if (Row.IsAmmo())
	{
		if (!Row.bStackable)
		{
			Issues.Add(FItemValidationIssue(
				EItemValidationSeverity::Warning,
				Row.ItemName,
				TEXT("bStackable"),
				TEXT("Ammo should typically be stackable")));
		}
	}

	return Issues;
}

TArray<FItemValidationIssue> FItemTableValidator::ValidateAssetReferences(const FItemTableRow& Row)
{
	TArray<FItemValidationIssue> Issues;

	// Modifier GE validation (check format, not existence)
	if (Row.ModifierGE.IsValid())
	{
		FString AssetName = Row.ModifierGE.GetAssetName();
		if (!AssetName.StartsWith(TEXT("GE_")))
		{
			Issues.Add(FItemValidationIssue(
				EItemValidationSeverity::Warning,
				Row.ItemName,
				TEXT("ModifierGE"),
				TEXT("Modifier GE should start with GE_ prefix")));
		}
	}

	// Abilities validation
	if (!Row.Abilities.IsEmpty())
	{
		TArray<FString> AbilityList;
		Row.Abilities.ParseIntoArray(AbilityList, TEXT(","), true);

		for (const FString& Ability : AbilityList)
		{
			FString Trimmed = Ability.TrimStartAndEnd();
			if (!Trimmed.StartsWith(TEXT("GA_")))
			{
				Issues.Add(FItemValidationIssue(
					EItemValidationSeverity::Warning,
					Row.ItemName,
					TEXT("Abilities"),
					FString::Printf(TEXT("Ability '%s' should start with GA_ prefix"), *Trimmed)));
			}
		}
	}

	return Issues;
}

TArray<FItemValidationIssue> FItemTableValidator::ValidateCombatStats(const FItemTableRow& Row)
{
	TArray<FItemValidationIssue> Issues;

	// Attack rating for weapons
	if (Row.IsWeapon() && Row.AttackRating <= 0)
	{
		Issues.Add(FItemValidationIssue(
			EItemValidationSeverity::Warning,
			Row.ItemName,
			TEXT("AttackRating"),
			TEXT("Weapon has no attack rating")));
	}

	// Attack rating shouldn't be set for non-weapons
	if (!Row.IsWeapon() && Row.AttackRating > 0)
	{
		Issues.Add(FItemValidationIssue(
			EItemValidationSeverity::Warning,
			Row.ItemName,
			TEXT("AttackRating"),
			TEXT("Attack rating specified for non-weapon item")));
	}

	// Armor rating for armor
	if (Row.IsArmor() && Row.ArmorRating <= 0)
	{
		Issues.Add(FItemValidationIssue(
			EItemValidationSeverity::Warning,
			Row.ItemName,
			TEXT("ArmorRating"),
			TEXT("Armor has no armor rating")));
	}

	// Armor rating shouldn't be set for weapons
	if (Row.IsWeapon() && Row.ArmorRating > 0)
	{
		Issues.Add(FItemValidationIssue(
			EItemValidationSeverity::Warning,
			Row.ItemName,
			TEXT("ArmorRating"),
			TEXT("Armor rating specified for weapon item")));
	}

	return Issues;
}

TArray<FItemValidationIssue> FItemTableValidator::ValidateStackingProperties(const FItemTableRow& Row)
{
	TArray<FItemValidationIssue> Issues;

	// Stackable validation
	if (Row.bStackable && Row.MaxStackSize <= 1)
	{
		Issues.Add(FItemValidationIssue(
			EItemValidationSeverity::Warning,
			Row.ItemName,
			TEXT("MaxStackSize"),
			TEXT("Stackable item has max stack size <= 1")));
	}

	if (!Row.bStackable && Row.MaxStackSize > 1)
	{
		Issues.Add(FItemValidationIssue(
			EItemValidationSeverity::Warning,
			Row.ItemName,
			TEXT("bStackable"),
			TEXT("Non-stackable item has max stack size > 1")));
	}

	// Equipment shouldn't be stackable
	if (Row.bStackable && (Row.ItemType == EItemType::Equippable || Row.IsWeapon()))
	{
		Issues.Add(FItemValidationIssue(
			EItemValidationSeverity::Warning,
			Row.ItemName,
			TEXT("bStackable"),
			TEXT("Equipment/weapons typically shouldn't be stackable")));
	}

	return Issues;
}
