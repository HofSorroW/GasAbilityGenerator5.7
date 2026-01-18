// GasAbilityGenerator v4.12.4
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
//
// v4.12.4 Features:
// - Full sync-from-assets for Item abilities (EquipmentAbilities extraction)
// - Full sync-from-assets for Quest tasks/events/rewards
//
// v4.8 Features:
// - Added Quest Table Editor - Excel-like spreadsheet for managing quests (following NPC/Dialogue patterns)
// - Added Item Table Editor - Excel-like spreadsheet for managing items (following NPC/Dialogue patterns)
//
// v4.1 Features:
// - Added Dialogue Table Editor - batch dialogue creation from CSV
//
// v3.10.0 Features:
// - Added NPC Table Editor - Excel-like spreadsheet for managing NPCs

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SDockTab;
class FSpawnTabArgs;

/**
 * GAS Ability Generator Plugin Module
 * Generates Gameplay Ability System assets from YAML manifest definitions.
 */
class GASABILITYGENERATOR_API FGasAbilityGeneratorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Register menu extensions */
	void RegisterMenus();

	/** Open the plugin window */
	void OpenPluginWindow();

	/** Open the NPC Table Editor window */
	void OpenNPCTableEditorWindow();

	/** Open the Quest Table Editor window (v4.8) */
	void OpenQuestTableEditorWindow();

	/** Open the Item Table Editor window (v4.8) */
	void OpenItemTableEditorWindow();

	/** Open the Dialogue Table Editor window */
	void OpenDialogueTableEditorWindow();

	/** Spawn the plugin tab */
	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);

	/** Spawn the NPC Table Editor tab */
	TSharedRef<SDockTab> OnSpawnNPCTableEditorTab(const FSpawnTabArgs& SpawnTabArgs);

	/** Spawn the Quest Table Editor tab (v4.8) */
	TSharedRef<SDockTab> OnSpawnQuestTableEditorTab(const FSpawnTabArgs& SpawnTabArgs);

	/** Spawn the Item Table Editor tab (v4.8) */
	TSharedRef<SDockTab> OnSpawnItemTableEditorTab(const FSpawnTabArgs& SpawnTabArgs);

	/** Spawn the Dialogue Table Editor tab */
	TSharedRef<SDockTab> OnSpawnDialogueTableEditorTab(const FSpawnTabArgs& SpawnTabArgs);
};
