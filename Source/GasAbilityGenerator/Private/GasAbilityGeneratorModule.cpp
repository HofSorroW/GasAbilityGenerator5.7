// GasAbilityGenerator v2.5.7
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v2.5.7: Added NPC system extensions - suffix-based section names (_tags, _gameplay_effects, etc.)
// v2.5.5: Fixed project_root not applied to all generators (GE_, IA_, IMC_, E_, etc.)

#include "GasAbilityGeneratorModule.h"
#include "GasAbilityGeneratorWindow.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "GasAbilityGenerator"

static const FName GasAbilityGeneratorTabName("GasAbilityGenerator");

void FGasAbilityGeneratorModule::StartupModule()
{
	// Register startup callback for tool menus
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(
		this, &FGasAbilityGeneratorModule::RegisterMenus));

	// Register the tab spawner
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		GasAbilityGeneratorTabName,
		FOnSpawnTab::CreateRaw(this, &FGasAbilityGeneratorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("TabTitle", "GAS Ability Generator"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UE_LOG(LogTemp, Log, TEXT("[GasAbilityGenerator] v2.5.7 module loaded"));
}

void FGasAbilityGeneratorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GasAbilityGeneratorTabName);

	UE_LOG(LogTemp, Log, TEXT("[GasAbilityGenerator] Module shutdown"));
}

void FGasAbilityGeneratorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	// Add to Tools menu
	UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	if (ToolsMenu)
	{
		FToolMenuSection& Section = ToolsMenu->FindOrAddSection("Generation");
		Section.AddMenuEntry(
			"GasAbilityGenerator",
			LOCTEXT("MenuLabel", "GAS Ability Generator"),
			LOCTEXT("MenuTooltip", "Open the GAS Ability Generator window"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FGasAbilityGeneratorModule::OpenPluginWindow))
		);
	}
}

void FGasAbilityGeneratorModule::OpenPluginWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(GasAbilityGeneratorTabName);
}

TSharedRef<SDockTab> FGasAbilityGeneratorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SGasAbilityGeneratorWindow)
		];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGasAbilityGeneratorModule, GasAbilityGenerator)
