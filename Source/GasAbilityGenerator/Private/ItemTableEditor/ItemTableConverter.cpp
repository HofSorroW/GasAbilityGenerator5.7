// GasAbilityGenerator - Item Table Converter
// v4.8: Implementation

#include "ItemTableConverter.h"

TArray<FManifestEquippableItemDefinition> FItemTableConverter::ConvertRowsToManifest(
	const TArray<FItemTableRow>& Rows,
	const FString& OutputFolder)
{
	TArray<FManifestEquippableItemDefinition> Definitions;

	for (const FItemTableRow& Row : Rows)
	{
		if (!Row.bDeleted && !Row.ItemName.IsEmpty())
		{
			FManifestEquippableItemDefinition Def = ConvertRowToDefinition(Row, OutputFolder);
			ApplyDefaults(Def);
			Definitions.Add(Def);
		}
	}

	return Definitions;
}

FManifestEquippableItemDefinition FItemTableConverter::ConvertRowToDefinition(
	const FItemTableRow& Row,
	const FString& OutputFolder)
{
	FManifestEquippableItemDefinition Def;

	// Asset name with EI_ prefix
	Def.Name = Row.ItemName.StartsWith(TEXT("EI_")) ? Row.ItemName : FString::Printf(TEXT("EI_%s"), *Row.ItemName);
	Def.DisplayName = Row.DisplayName;
	Def.ParentClass = GetParentClassName(Row.ItemType);
	Def.Folder = OutputFolder / GetSubfolderForType(Row.ItemType);

	// Equipment properties
	Def.EquipmentSlot = Row.EquipmentSlot;
	Def.BaseValue = Row.BaseValue;
	Def.Weight = Row.Weight;

	// Combat stats - use explicit columns
	if (Row.IsWeapon())
	{
		Def.AttackRating = Row.AttackRating;
		Def.AttackDamage = Row.AttackDamage;
		Def.HeavyAttackDamageMultiplier = Row.HeavyAttackDamageMultiplier;
		Def.ClipSize = Row.ClipSize;
		Def.RequiredAmmo = Row.RequiredAmmo;
		Def.bAllowManualReload = Row.bAllowManualReload;
		Def.BotAttackRange = Row.BotAttackRange;

		// Ranged weapon properties
		if (Row.IsRangedWeapon())
		{
			Def.BaseSpreadDegrees = Row.BaseSpreadDegrees;
			Def.MaxSpreadDegrees = Row.MaxSpreadDegrees;
			Def.SpreadFireBump = Row.SpreadFireBump;
			Def.SpreadDecreaseSpeed = Row.SpreadDecreaseSpeed;
			Def.AimFOVPct = Row.AimFOVPct;
		}
	}
	else if (Row.IsArmor())
	{
		Def.ArmorRating = Row.ArmorRating;
	}

	// Consumable properties
	if (Row.IsConsumable())
	{
		Def.bConsumeOnUse = Row.bConsumeOnUse;
		Def.UseRechargeDuration = Row.UseRechargeDuration;
		Def.bCanActivate = Row.bCanActivate;
	}

	// References
	if (Row.ModifierGE.IsValid())
	{
		Def.EquipmentModifierGE = Row.ModifierGE.GetAssetName();
	}
	Def.AbilitiesToGrant = ParseAbilities(Row.Abilities);
	Def.ItemTags = ParseItemTags(Row.ItemTags);

	// Stacking
	Def.bStackable = Row.bStackable;
	Def.MaxStackSize = Row.MaxStackSize;

	return Def;
}

void FItemTableConverter::ApplyDefaults(FManifestEquippableItemDefinition& Def)
{
	if (Def.Folder.IsEmpty())
	{
		Def.Folder = TEXT("/Game/Items");
	}
	if (Def.ParentClass.IsEmpty())
	{
		Def.ParentClass = TEXT("EquippableItem");
	}
}

FItemTableRow FItemTableConverter::ConvertDefinitionToRow(const FManifestEquippableItemDefinition& Def)
{
	FItemTableRow Row;

	// Extract item name (remove EI_ prefix for display)
	Row.ItemName = Def.Name;
	if (Row.ItemName.StartsWith(TEXT("EI_")))
	{
		Row.ItemName = Row.ItemName.Mid(3);
	}

	Row.DisplayName = Def.DisplayName;
	Row.ItemType = ParseItemType(Def.ParentClass);
	Row.EquipmentSlot = Def.EquipmentSlot;
	Row.BaseValue = Def.BaseValue;
	Row.Weight = Def.Weight;
	Row.AttackRating = Def.AttackRating;
	Row.ArmorRating = Def.ArmorRating;

	if (!Def.EquipmentModifierGE.IsEmpty())
	{
		Row.ModifierGE = FSoftObjectPath(FString::Printf(TEXT("/Game/Effects/%s.%s"), *Def.EquipmentModifierGE, *Def.EquipmentModifierGE));
	}

	// Build abilities string
	Row.Abilities = FString::Join(Def.AbilitiesToGrant, TEXT(","));

	// Set weapon properties from definition - explicit columns
	if (Row.IsWeapon())
	{
		Row.AttackDamage = Def.AttackDamage;
		Row.HeavyAttackDamageMultiplier = Def.HeavyAttackDamageMultiplier;
		Row.ClipSize = Def.ClipSize;
		Row.RequiredAmmo = Def.RequiredAmmo;
		Row.bAllowManualReload = Def.bAllowManualReload;
		Row.BotAttackRange = Def.BotAttackRange;

		// Ranged weapon properties
		if (Row.IsRangedWeapon())
		{
			Row.BaseSpreadDegrees = Def.BaseSpreadDegrees;
			Row.MaxSpreadDegrees = Def.MaxSpreadDegrees;
			Row.SpreadFireBump = Def.SpreadFireBump;
			Row.SpreadDecreaseSpeed = Def.SpreadDecreaseSpeed;
			Row.AimFOVPct = Def.AimFOVPct;
		}
	}

	// Set consumable properties from definition
	if (Row.IsConsumable())
	{
		Row.bConsumeOnUse = Def.bConsumeOnUse;
		Row.UseRechargeDuration = Def.UseRechargeDuration;
		Row.bCanActivate = Def.bCanActivate;
	}

	// Build tags string
	Row.ItemTags = FString::Join(Def.ItemTags, TEXT(","));

	Row.bStackable = Def.bStackable;
	Row.MaxStackSize = Def.MaxStackSize;

	return Row;
}

FString FItemTableConverter::GetParentClassName(EItemType ItemType)
{
	switch (ItemType)
	{
		case EItemType::RangedWeapon: return TEXT("RangedWeaponItem");
		case EItemType::MeleeWeapon: return TEXT("MeleeWeaponItem");
		case EItemType::MagicWeapon: return TEXT("MagicWeaponItem");
		case EItemType::ThrowableWeapon: return TEXT("ThrowableWeaponItem");
		case EItemType::Ammo: return TEXT("AmmoItem");
		case EItemType::WeaponAttachment: return TEXT("WeaponAttachmentItem");
		case EItemType::Clothing: return TEXT("ClothingItem");
		case EItemType::Consumable: return TEXT("GameplayEffectItem");
		case EItemType::Equippable:
		default: return TEXT("EquippableItem");
	}
}

TArray<FString> FItemTableConverter::ParseAbilities(const FString& AbilitiesStr)
{
	TArray<FString> Abilities;
	if (AbilitiesStr.IsEmpty()) return Abilities;

	AbilitiesStr.ParseIntoArray(Abilities, TEXT(","), true);
	for (FString& Ability : Abilities)
	{
		Ability = Ability.TrimStartAndEnd();
	}

	return Abilities;
}

TArray<FString> FItemTableConverter::ParseItemTags(const FString& TagsStr)
{
	TArray<FString> Tags;
	if (TagsStr.IsEmpty()) return Tags;

	TagsStr.ParseIntoArray(Tags, TEXT(","), true);
	for (FString& Tag : Tags)
	{
		Tag = Tag.TrimStartAndEnd();
	}

	return Tags;
}

FString FItemTableConverter::GetItemTypeString(EItemType ItemType)
{
	switch (ItemType)
	{
		case EItemType::RangedWeapon: return TEXT("RangedWeapon");
		case EItemType::MeleeWeapon: return TEXT("MeleeWeapon");
		case EItemType::MagicWeapon: return TEXT("MagicWeapon");
		case EItemType::ThrowableWeapon: return TEXT("ThrowableWeapon");
		case EItemType::Ammo: return TEXT("Ammo");
		case EItemType::WeaponAttachment: return TEXT("WeaponAttachment");
		case EItemType::Clothing: return TEXT("Clothing");
		case EItemType::Consumable: return TEXT("Consumable");
		case EItemType::Equippable:
		default: return TEXT("Equippable");
	}
}

EItemType FItemTableConverter::ParseItemType(const FString& TypeStr)
{
	if (TypeStr.Contains(TEXT("Ranged"))) return EItemType::RangedWeapon;
	if (TypeStr.Contains(TEXT("Melee"))) return EItemType::MeleeWeapon;
	if (TypeStr.Contains(TEXT("Magic"))) return EItemType::MagicWeapon;
	if (TypeStr.Contains(TEXT("Throwable"))) return EItemType::ThrowableWeapon;
	if (TypeStr.Contains(TEXT("Ammo"))) return EItemType::Ammo;
	if (TypeStr.Contains(TEXT("Attachment"))) return EItemType::WeaponAttachment;
	if (TypeStr.Contains(TEXT("Clothing"))) return EItemType::Clothing;
	if (TypeStr.Contains(TEXT("Consumable")) || TypeStr.Contains(TEXT("GameplayEffect"))) return EItemType::Consumable;
	return EItemType::Equippable;
}

FString FItemTableConverter::GetSubfolderForType(EItemType ItemType)
{
	switch (ItemType)
	{
		case EItemType::RangedWeapon: return TEXT("Weapons/Ranged");
		case EItemType::MeleeWeapon: return TEXT("Weapons/Melee");
		case EItemType::MagicWeapon: return TEXT("Weapons/Magic");
		case EItemType::ThrowableWeapon: return TEXT("Weapons/Throwable");
		case EItemType::Ammo: return TEXT("Ammo");
		case EItemType::WeaponAttachment: return TEXT("Attachments");
		case EItemType::Clothing: return TEXT("Clothing");
		case EItemType::Consumable: return TEXT("Consumables");
		case EItemType::Equippable:
		default: return TEXT("Equipment");
	}
}
