// GasAbilityGenerator v3.10.0
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
//
// v3.10.0 Features:
// - Added NPC Table Editor - Excel-like spreadsheet for managing NPCs
//
// v2.5.0 Features:
// - Renamed to GasAbilityGenerator for generic UE project compatibility
// - Removed project-specific dependencies

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

	/** Spawn the plugin tab */
	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);

	/** Spawn the NPC Table Editor tab */
	TSharedRef<SDockTab> OnSpawnNPCTableEditorTab(const FSpawnTabArgs& SpawnTabArgs);
};
