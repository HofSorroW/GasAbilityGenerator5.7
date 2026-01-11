// GasAbilityGenerator v2.5.0
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SDockTab;
class FSpawnTabArgs;

/**
 * GAS Ability Generator Plugin Module
 * Generates Gameplay Ability System assets from YAML manifest definitions.
 * Works with any Unreal Engine 5.x project using GAS.
 *
 * v2.5.0 Features:
 * - Renamed to GasAbilityGenerator for generic UE project compatibility
 * - Removed project-specific dependencies
 *
 * v2.4.5 Features:
 * - GetPinTypeFromString now falls back to AActor when Object:ClassName class not found
 * - Plain "Object" type now defaults to AActor reference (was Boolean)
 * - Added Class field to FManifestActorVariableDefinition struct
 * - Fixes "Boolean is not compatible with Object Reference" errors
 *
 * v2.4.4 Features:
 * - Enhanced function name resolution with K2_/BP_ prefix fallbacks
 *
 * v2.4.1 Features:
 * - Enhanced FindParentClass to search Blueprint class hierarchy
 * - Enhanced FindPinByName with comprehensive pin name aliases
 *
 * v2.1.8 Features:
 * - Added UserDefinedEnum (E_*) asset generation
 * - Enumerations generated BEFORE blueprints to fix enum variable types
 * - Added FindUserDefinedEnum() helper for enum lookup in blueprints
 * - GetPinTypeFromString() now properly resolves E_ prefixed enum types
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

	/** Spawn the plugin tab */
	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);
};
