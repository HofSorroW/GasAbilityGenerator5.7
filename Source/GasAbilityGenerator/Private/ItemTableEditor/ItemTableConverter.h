// GasAbilityGenerator - Item Table Converter
// v4.8: Converts Item table rows to manifest definitions for generation
// Follows NPC/Dialogue converter patterns

#pragma once

#include "CoreMinimal.h"
#include "ItemTableEditor/ItemTableEditorTypes.h"
#include "GasAbilityGeneratorTypes.h"

/**
 * Converts Item table rows to manifest definitions for generation
 */
class FItemTableConverter
{
public:
	/**
	 * Convert all rows to manifest definitions
	 * @param Rows All Item rows
	 * @param OutputFolder Base folder for generated assets
	 * @return Array of manifest definitions ready for generation
	 */
	static TArray<FManifestEquippableItemDefinition> ConvertRowsToManifest(
		const TArray<FItemTableRow>& Rows,
		const FString& OutputFolder = TEXT("/Game/Items"));

	/**
	 * Convert a single table row to a manifest definition
	 * @param Row Table row to convert
	 * @param OutputFolder Base folder for generated assets
	 * @return Manifest definition
	 */
	static FManifestEquippableItemDefinition ConvertRowToDefinition(
		const FItemTableRow& Row,
		const FString& OutputFolder = TEXT("/Game/Items"));

	/**
	 * Apply default settings to a definition
	 * @param Def Definition to apply defaults to
	 */
	static void ApplyDefaults(FManifestEquippableItemDefinition& Def);

	/**
	 * Convert manifest definition back to table row
	 * Used for "Sync from Assets" functionality
	 * @param Def Manifest definition
	 * @return Table row
	 */
	static FItemTableRow ConvertDefinitionToRow(const FManifestEquippableItemDefinition& Def);

	/**
	 * Get parent class name based on item type
	 * @param ItemType Item type enum
	 * @return Parent class name (EquippableItem, RangedWeaponItem, MeleeWeaponItem, NarrativeItem)
	 */
	static FString GetParentClassName(EItemType ItemType);

	/**
	 * Parse WeaponConfig token string into weapon properties
	 * Format: Weapon(Damage=50,ClipSize=30,AimFOV=0.5,...)
	 * @param WeaponConfigToken Weapon config token string
	 * @return Map of property key-value pairs
	 */
	static TMap<FString, FString> ParseWeaponConfig(const FString& WeaponConfigToken);

	/**
	 * Build WeaponConfig token from properties map
	 * @param Properties Map of property key-value pairs
	 * @return Formatted token string
	 */
	static FString BuildWeaponConfigToken(const TMap<FString, FString>& Properties);

	/**
	 * Parse comma-separated abilities string into array
	 * @param AbilitiesStr Comma-separated abilities
	 * @return Array of ability class names
	 */
	static TArray<FString> ParseAbilities(const FString& AbilitiesStr);

	/**
	 * Parse comma-separated item tags string into array
	 * @param TagsStr Comma-separated tags
	 * @return Array of gameplay tag strings
	 */
	static TArray<FString> ParseItemTags(const FString& TagsStr);

private:
	/**
	 * Parse token parameters from string
	 * Format: Key=Value,Key2=Value2
	 * @param ParamsStr Parameters string
	 * @return Map of parameter key-value pairs
	 */
	static TMap<FString, FString> ParseTokenParams(const FString& ParamsStr);

	/**
	 * Get item type string from enum
	 * @param ItemType Item type enum
	 * @return Item type string
	 */
	static FString GetItemTypeString(EItemType ItemType);

	/**
	 * Parse item type string to enum
	 * @param TypeStr Item type string
	 * @return Item type enum
	 */
	static EItemType ParseItemType(const FString& TypeStr);

	/**
	 * Get subfolder based on item type
	 * @param ItemType Item type enum
	 * @return Subfolder name (Equipment, Weapons/Ranged, Weapons/Melee, Consumables)
	 */
	static FString GetSubfolderForType(EItemType ItemType);
};
