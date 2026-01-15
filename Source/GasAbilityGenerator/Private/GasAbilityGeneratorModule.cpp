// GasAbilityGenerator v4.1
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v4.1: Added Dialogue Table Editor - batch dialogue creation from CSV
// v4.0: Added Quest Editor - visual editor for quest state machines
// v3.10.0: Added NPC Table Editor - Excel-like spreadsheet for managing NPCs

#include "GasAbilityGeneratorModule.h"
#include "GasAbilityGeneratorWindow.h"
#include "NPCTableEditor/SNPCTableEditor.h"
#include "QuestEditor/SQuestEditor.h"
#include "SDialogueTableEditor.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "GasAbilityGenerator"

static const FName GasAbilityGeneratorTabName("GasAbilityGenerator");
static const FName NPCTableEditorTabName("NPCTableEditor");
static const FName QuestEditorTabName("QuestEditor");
static const FName DialogueTableEditorTabName("DialogueTableEditor");

void FGasAbilityGeneratorModule::StartupModule()
{
	// Register startup callback for tool menus
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(
		this, &FGasAbilityGeneratorModule::RegisterMenus));

	// Register the tab spawner for main generator window
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		GasAbilityGeneratorTabName,
		FOnSpawnTab::CreateRaw(this, &FGasAbilityGeneratorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("TabTitle", "GAS Ability Generator"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	// Register the tab spawner for NPC Table Editor
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		NPCTableEditorTabName,
		FOnSpawnTab::CreateRaw(this, &FGasAbilityGeneratorModule::OnSpawnNPCTableEditorTab))
		.SetDisplayName(LOCTEXT("NPCTableEditorTabTitle", "NPC Table Editor"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	// Register the tab spawner for Quest Editor
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		QuestEditorTabName,
		FOnSpawnTab::CreateRaw(this, &FGasAbilityGeneratorModule::OnSpawnQuestEditorTab))
		.SetDisplayName(LOCTEXT("QuestEditorTabTitle", "Quest Editor"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	// Register the tab spawner for Dialogue Table Editor
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		DialogueTableEditorTabName,
		FOnSpawnTab::CreateRaw(this, &FGasAbilityGeneratorModule::OnSpawnDialogueTableEditorTab))
		.SetDisplayName(LOCTEXT("DialogueTableEditorTabTitle", "Dialogue Table Editor"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UE_LOG(LogTemp, Log, TEXT("[GasAbilityGenerator] v4.1 module loaded - Dialogue Table Editor"));
}

void FGasAbilityGeneratorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GasAbilityGeneratorTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(NPCTableEditorTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(QuestEditorTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DialogueTableEditorTabName);

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
		Section.AddMenuEntry(
			"NPCTableEditor",
			LOCTEXT("NPCTableEditorMenuLabel", "NPC Table Editor"),
			LOCTEXT("NPCTableEditorMenuTooltip", "Open the NPC Table Editor - Excel-like spreadsheet for managing NPCs"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FGasAbilityGeneratorModule::OpenNPCTableEditorWindow))
		);
		Section.AddMenuEntry(
			"QuestEditor",
			LOCTEXT("QuestEditorMenuLabel", "Quest Editor"),
			LOCTEXT("QuestEditorMenuTooltip", "Open the Quest Editor - visual editor for quest state machines"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FGasAbilityGeneratorModule::OpenQuestEditorWindow))
		);
		Section.AddMenuEntry(
			"DialogueTableEditor",
			LOCTEXT("DialogueTableEditorMenuLabel", "Dialogue Table Editor"),
			LOCTEXT("DialogueTableEditorMenuTooltip", "Open the Dialogue Table Editor - batch dialogue creation from CSV"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FGasAbilityGeneratorModule::OpenDialogueTableEditorWindow))
		);
	}
}

void FGasAbilityGeneratorModule::OpenPluginWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(GasAbilityGeneratorTabName);
}

void FGasAbilityGeneratorModule::OpenNPCTableEditorWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(NPCTableEditorTabName);
}

void FGasAbilityGeneratorModule::OpenQuestEditorWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(QuestEditorTabName);
}

void FGasAbilityGeneratorModule::OpenDialogueTableEditorWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(DialogueTableEditorTabName);
}

TSharedRef<SDockTab> FGasAbilityGeneratorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SGasAbilityGeneratorWindow)
		];
}

TSharedRef<SDockTab> FGasAbilityGeneratorModule::OnSpawnNPCTableEditorTab(const FSpawnTabArgs& SpawnTabArgs)
{
	TSharedPtr<SNPCTableEditorWindow> EditorWindow;

	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SAssignNew(EditorWindow, SNPCTableEditorWindow)
		];

	// v4.6: Pass tab reference for dirty indicator and save-on-close
	if (EditorWindow.IsValid())
	{
		EditorWindow->SetParentTab(Tab);
	}

	return Tab;
}

TSharedRef<SDockTab> FGasAbilityGeneratorModule::OnSpawnQuestEditorTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SQuestEditorWindow)
		];
}

TSharedRef<SDockTab> FGasAbilityGeneratorModule::OnSpawnDialogueTableEditorTab(const FSpawnTabArgs& SpawnTabArgs)
{
	TSharedPtr<SDialogueTableEditorWindow> EditorWindow;

	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SAssignNew(EditorWindow, SDialogueTableEditorWindow)
		];

	// v4.6: Pass tab reference for dirty indicator and save-on-close
	if (EditorWindow.IsValid())
	{
		EditorWindow->SetParentTab(Tab);
	}

	return Tab;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGasAbilityGeneratorModule, GasAbilityGenerator)
