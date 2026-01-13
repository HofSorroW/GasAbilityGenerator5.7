// GasAbilityGenerator v4.0
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
//
// v4.0 Features:
// - Spec DataAssets for native UE workflow
// - Content Browser right-click generation
// - Template inheritance via ParentTemplate
//
// v2.5.0 Features:
// - Renamed to GasAbilityGenerator for generic UE project compatibility
// - Removed project-specific dependencies

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IAssetTools.h"

class SDockTab;
class FSpawnTabArgs;
class UNPCPackageSpec;
class UQuestSpec;
class UItemSpec;

/**
 * GAS Ability Generator Plugin Module
 * Generates Gameplay Ability System assets from YAML manifest definitions
 * and Spec DataAssets (v4.0+).
 */
class GASABILITYGENERATOR_API FGasAbilityGeneratorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	//=========================================================================
	// v4.0: Spec DataAsset Context Menu Handlers
	//=========================================================================

	/** Generate assets from selected NPC Package Specs */
	static void GenerateFromSelectedNPCSpecs();

	/** Validate selected NPC Package Specs without generating */
	static void ValidateSelectedNPCSpecs();

	/** Preview what would be generated from selected NPC Package Specs */
	static void PreviewSelectedNPCSpecs();

	/** Generate assets from selected Quest Specs */
	static void GenerateFromSelectedQuestSpecs();

	/** Validate selected Quest Specs */
	static void ValidateSelectedQuestSpecs();

	/** Generate assets from selected Item Specs */
	static void GenerateFromSelectedItemSpecs();

	/** Validate selected Item Specs */
	static void ValidateSelectedItemSpecs();

private:
	/** Register menu extensions */
	void RegisterMenus();

	/** Register Content Browser context menu extensions for Spec assets */
	void RegisterSpecContextMenus();

	/** Open the plugin window */
	void OpenPluginWindow();

	/** Spawn the plugin tab */
	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);

	/** Asset category handle for "GasAbilityGenerator" */
	EAssetTypeCategories::Type GasAbilityGeneratorCategory = EAssetTypeCategories::None;
};
