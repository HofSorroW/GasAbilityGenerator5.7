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

	// Combat stats
	if (Row.IsWeapon())
	{
		Def.AttackRating = Row.AttackRating;

		// Parse weapon config for additional properties
		TMap<FString, FString> WeaponProps = ParseWeaponConfig(Row.WeaponConfig);
		if (WeaponProps.Contains(TEXT("Damage")))
		{
			Def.AttackDamage = FCString::Atof(*WeaponProps[TEXT("Damage")]);
		}
		if (WeaponProps.Contains(TEXT("ClipSize")))
		{
			Def.ClipSize = FCString::Atoi(*WeaponProps[TEXT("ClipSize")]);
		}
		if (WeaponProps.Contains(TEXT("AimFOV")))
		{
			Def.AimFOVPct = FCString::Atof(*WeaponProps[TEXT("AimFOV")]);
		}
		if (WeaponProps.Contains(TEXT("BaseSpread")))
		{
			Def.BaseSpreadDegrees = FCString::Atof(*WeaponProps[TEXT("BaseSpread")]);
		}
		if (WeaponProps.Contains(TEXT("MaxSpread")))
		{
			Def.MaxSpreadDegrees = FCString::Atof(*WeaponProps[TEXT("MaxSpread")]);
		}
		if (WeaponProps.Contains(TEXT("HeavyMultiplier")))
		{
			Def.HeavyAttackDamageMultiplier = FCString::Atof(*WeaponProps[TEXT("HeavyMultiplier")]);
		}
		if (WeaponProps.Contains(TEXT("AmmoItem")))
		{
			Def.RequiredAmmo = WeaponProps[TEXT("AmmoItem")];
		}
	}
	else if (Row.IsArmor())
	{
		Def.ArmorRating = Row.ArmorRating;
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

	// Build weapon config if applicable
	if (Row.IsWeapon())
	{
		TMap<FString, FString> WeaponProps;
		if (Def.AttackDamage > 0)
		{
			WeaponProps.Add(TEXT("Damage"), FString::SanitizeFloat(Def.AttackDamage));
		}
		if (Def.ClipSize > 0)
		{
			WeaponProps.Add(TEXT("ClipSize"), FString::FromInt(Def.ClipSize));
		}
		if (Def.AimFOVPct > 0)
		{
			WeaponProps.Add(TEXT("AimFOV"), FString::SanitizeFloat(Def.AimFOVPct));
		}
		Row.WeaponConfig = BuildWeaponConfigToken(WeaponProps);
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
		case EItemType::Consumable: return TEXT("NarrativeItem");
		case EItemType::Equippable:
		default: return TEXT("EquippableItem");
	}
}

TMap<FString, FString> FItemTableConverter::ParseWeaponConfig(const FString& WeaponConfigToken)
{
	TMap<FString, FString> Props;
	if (WeaponConfigToken.IsEmpty()) return Props;

	// Parse: Weapon(Damage=50,ClipSize=30,AimFOV=0.5,...)
	int32 ParenStart = WeaponConfigToken.Find(TEXT("("));
	if (ParenStart != INDEX_NONE)
	{
		int32 ParenEnd = WeaponConfigToken.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (ParenEnd > ParenStart)
		{
			FString ParamsStr = WeaponConfigToken.Mid(ParenStart + 1, ParenEnd - ParenStart - 1);
			Props = ParseTokenParams(ParamsStr);
		}
	}

	return Props;
}

FString FItemTableConverter::BuildWeaponConfigToken(const TMap<FString, FString>& Properties)
{
	if (Properties.Num() == 0) return TEXT("");

	FString ParamsStr;
	for (const auto& Pair : Properties)
	{
		if (!ParamsStr.IsEmpty()) ParamsStr += TEXT(",");
		ParamsStr += FString::Printf(TEXT("%s=%s"), *Pair.Key, *Pair.Value);
	}

	return FString::Printf(TEXT("Weapon(%s)"), *ParamsStr);
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

TMap<FString, FString> FItemTableConverter::ParseTokenParams(const FString& ParamsStr)
{
	TMap<FString, FString> Params;
	if (ParamsStr.IsEmpty()) return Params;

	TArray<FString> ParamPairs;
	ParamsStr.ParseIntoArray(ParamPairs, TEXT(","), true);

	for (const FString& Pair : ParamPairs)
	{
		FString Key, Value;
		if (Pair.Split(TEXT("="), &Key, &Value))
		{
			Params.Add(Key.TrimStartAndEnd(), Value.TrimStartAndEnd());
		}
	}

	return Params;
}

FString FItemTableConverter::GetItemTypeString(EItemType ItemType)
{
	switch (ItemType)
	{
		case EItemType::RangedWeapon: return TEXT("RangedWeapon");
		case EItemType::MeleeWeapon: return TEXT("MeleeWeapon");
		case EItemType::Consumable: return TEXT("Consumable");
		case EItemType::Equippable:
		default: return TEXT("Equippable");
	}
}

EItemType FItemTableConverter::ParseItemType(const FString& TypeStr)
{
	if (TypeStr.Contains(TEXT("Ranged"))) return EItemType::RangedWeapon;
	if (TypeStr.Contains(TEXT("Melee"))) return EItemType::MeleeWeapon;
	if (TypeStr.Contains(TEXT("Consumable")) || TypeStr.Contains(TEXT("NarrativeItem"))) return EItemType::Consumable;
	return EItemType::Equippable;
}

FString FItemTableConverter::GetSubfolderForType(EItemType ItemType)
{
	switch (ItemType)
	{
		case EItemType::RangedWeapon: return TEXT("Weapons/Ranged");
		case EItemType::MeleeWeapon: return TEXT("Weapons/Melee");
		case EItemType::Consumable: return TEXT("Consumables");
		case EItemType::Equippable:
		default: return TEXT("Equipment");
	}
}
