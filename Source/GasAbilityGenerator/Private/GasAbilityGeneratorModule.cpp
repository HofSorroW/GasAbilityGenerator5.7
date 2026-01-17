// GasAbilityGenerator v4.8
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v4.8: Added Quest Table Editor and Item Table Editor (following NPC/Dialogue patterns)
// v4.1: Added Dialogue Table Editor - batch dialogue creation from CSV
// v3.10.0: Added NPC Table Editor - Excel-like spreadsheet for managing NPCs

#include "GasAbilityGeneratorModule.h"
#include "GasAbilityGeneratorWindow.h"
#include "NPCTableEditor/SNPCTableEditor.h"
#include "QuestTableEditor/SQuestTableEditor.h"
#include "ItemTableEditor/SItemTableEditor.h"
#include "SDialogueTableEditor.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "GasAbilityGenerator"

static const FName GasAbilityGeneratorTabName("GasAbilityGenerator");
static const FName NPCTableEditorTabName("NPCTableEditor");
static const FName QuestTableEditorTabName("QuestTableEditor");
static const FName ItemTableEditorTabName("ItemTableEditor");
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

	// Register the tab spawner for Quest Table Editor (v4.8)
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		QuestTableEditorTabName,
		FOnSpawnTab::CreateRaw(this, &FGasAbilityGeneratorModule::OnSpawnQuestTableEditorTab))
		.SetDisplayName(LOCTEXT("QuestTableEditorTabTitle", "Quest Table Editor"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	// Register the tab spawner for Item Table Editor (v4.8)
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		ItemTableEditorTabName,
		FOnSpawnTab::CreateRaw(this, &FGasAbilityGeneratorModule::OnSpawnItemTableEditorTab))
		.SetDisplayName(LOCTEXT("ItemTableEditorTabTitle", "Item Table Editor"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	// Register the tab spawner for Dialogue Table Editor
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		DialogueTableEditorTabName,
		FOnSpawnTab::CreateRaw(this, &FGasAbilityGeneratorModule::OnSpawnDialogueTableEditorTab))
		.SetDisplayName(LOCTEXT("DialogueTableEditorTabTitle", "Dialogue Table Editor"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UE_LOG(LogTemp, Log, TEXT("[GasAbilityGenerator] v4.8 module loaded - Quest/Item Table Editors"));
}

void FGasAbilityGeneratorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GasAbilityGeneratorTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(NPCTableEditorTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(QuestTableEditorTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ItemTableEditorTabName);
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
			"QuestTableEditor",
			LOCTEXT("QuestTableEditorMenuLabel", "Quest Table Editor"),
			LOCTEXT("QuestTableEditorMenuTooltip", "Open the Quest Table Editor - Excel-like spreadsheet for managing quests"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FGasAbilityGeneratorModule::OpenQuestTableEditorWindow))
		);
		Section.AddMenuEntry(
			"ItemTableEditor",
			LOCTEXT("ItemTableEditorMenuLabel", "Item Table Editor"),
			LOCTEXT("ItemTableEditorMenuTooltip", "Open the Item Table Editor - Excel-like spreadsheet for managing items"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FGasAbilityGeneratorModule::OpenItemTableEditorWindow))
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

void FGasAbilityGeneratorModule::OpenQuestTableEditorWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(QuestTableEditorTabName);
}

void FGasAbilityGeneratorModule::OpenItemTableEditorWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(ItemTableEditorTabName);
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

TSharedRef<SDockTab> FGasAbilityGeneratorModule::OnSpawnQuestTableEditorTab(const FSpawnTabArgs& SpawnTabArgs)
{
	TSharedPtr<SQuestTableEditorWindow> EditorWindow;

	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SAssignNew(EditorWindow, SQuestTableEditorWindow)
		];

	// v4.8: Pass tab reference for dirty indicator and save-on-close
	if (EditorWindow.IsValid())
	{
		EditorWindow->SetParentTab(Tab);
	}

	return Tab;
}

TSharedRef<SDockTab> FGasAbilityGeneratorModule::OnSpawnItemTableEditorTab(const FSpawnTabArgs& SpawnTabArgs)
{
	TSharedPtr<SItemTableEditorWindow> EditorWindow;

	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SAssignNew(EditorWindow, SItemTableEditorWindow)
		];

	// v4.8: Pass tab reference for dirty indicator and save-on-close
	if (EditorWindow.IsValid())
	{
		EditorWindow->SetParentTab(Tab);
	}

	return Tab;
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
