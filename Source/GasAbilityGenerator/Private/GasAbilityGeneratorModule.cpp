// GasAbilityGenerator v4.0
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v4.0: Added Spec DataAsset workflow with Content Browser context menus
// v3.10.0: Added NPC Table Editor - Excel-like spreadsheet for managing NPCs

#include "GasAbilityGeneratorModule.h"
#include "GasAbilityGeneratorWindow.h"
#include "GasAbilityGeneratorSpecs.h"
#include "NPCTableEditor/SNPCTableEditor.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Misc/MessageDialog.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "GasAbilityGenerator"

static const FName GasAbilityGeneratorTabName("GasAbilityGenerator");
static const FName NPCTableEditorTabName("NPCTableEditor");

void FGasAbilityGeneratorModule::StartupModule()
{
	// Register asset category for Content Browser
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	GasAbilityGeneratorCategory = AssetTools.RegisterAdvancedAssetCategory(
		FName("GasAbilityGenerator"),
		LOCTEXT("GasAbilityGeneratorCategory", "GasAbilityGenerator"));

	// Register startup callback for tool menus
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(
		this, &FGasAbilityGeneratorModule::RegisterMenus));

	// Register Spec context menus
	RegisterSpecContextMenus();

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

	UE_LOG(LogTemp, Log, TEXT("[GasAbilityGenerator] v4.0 module loaded - Spec DataAsset workflow + NPC Table Editor"));
}

void FGasAbilityGeneratorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GasAbilityGeneratorTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(NPCTableEditorTabName);

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
	}
}

void FGasAbilityGeneratorModule::RegisterSpecContextMenus()
{
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
	{
		//=====================================================================
		// NPC Package Spec Context Menu
		//=====================================================================
		UToolMenu* NPCSpecMenu = UToolMenus::Get()->ExtendMenu(
			"ContentBrowser.AssetContextMenu.NPCPackageSpec");

		if (NPCSpecMenu)
		{
			FToolMenuSection& NPCSection = NPCSpecMenu->FindOrAddSection("GasAbilityGenerator");
			NPCSection.Label = LOCTEXT("GasAbilityGeneratorSection", "GasAbilityGenerator");

			NPCSection.AddMenuEntry(
				"GenerateFromNPCSpec",
				LOCTEXT("GenerateNPC", "Generate NPC Assets"),
				LOCTEXT("GenerateNPCTooltip", "Generate all assets defined in this NPC spec (NPCDef_, AC_, Schedule_, DBP_, Quest_)"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"),
				FUIAction(FExecuteAction::CreateStatic(&FGasAbilityGeneratorModule::GenerateFromSelectedNPCSpecs))
			);

			NPCSection.AddMenuEntry(
				"ValidateNPCSpec",
				LOCTEXT("ValidateNPC", "Validate Spec"),
				LOCTEXT("ValidateNPCTooltip", "Check spec for errors without generating"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Check"),
				FUIAction(FExecuteAction::CreateStatic(&FGasAbilityGeneratorModule::ValidateSelectedNPCSpecs))
			);

			NPCSection.AddMenuEntry(
				"PreviewNPCSpec",
				LOCTEXT("PreviewNPC", "Preview Generation Plan"),
				LOCTEXT("PreviewNPCTooltip", "Show what assets would be created or modified"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Info"),
				FUIAction(FExecuteAction::CreateStatic(&FGasAbilityGeneratorModule::PreviewSelectedNPCSpecs))
			);
		}

		//=====================================================================
		// Quest Spec Context Menu
		//=====================================================================
		UToolMenu* QuestSpecMenu = UToolMenus::Get()->ExtendMenu(
			"ContentBrowser.AssetContextMenu.QuestSpec");

		if (QuestSpecMenu)
		{
			FToolMenuSection& QuestSection = QuestSpecMenu->FindOrAddSection("GasAbilityGenerator");
			QuestSection.Label = LOCTEXT("GasAbilityGeneratorSection", "GasAbilityGenerator");

			QuestSection.AddMenuEntry(
				"GenerateFromQuestSpec",
				LOCTEXT("GenerateQuest", "Generate Quest"),
				LOCTEXT("GenerateQuestTooltip", "Generate quest blueprint from this spec"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"),
				FUIAction(FExecuteAction::CreateStatic(&FGasAbilityGeneratorModule::GenerateFromSelectedQuestSpecs))
			);

			QuestSection.AddMenuEntry(
				"ValidateQuestSpec",
				LOCTEXT("ValidateQuest", "Validate Spec"),
				LOCTEXT("ValidateQuestTooltip", "Check spec for errors without generating"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Check"),
				FUIAction(FExecuteAction::CreateStatic(&FGasAbilityGeneratorModule::ValidateSelectedQuestSpecs))
			);
		}

		//=====================================================================
		// Item Spec Context Menu
		//=====================================================================
		UToolMenu* ItemSpecMenu = UToolMenus::Get()->ExtendMenu(
			"ContentBrowser.AssetContextMenu.ItemSpec");

		if (ItemSpecMenu)
		{
			FToolMenuSection& ItemSection = ItemSpecMenu->FindOrAddSection("GasAbilityGenerator");
			ItemSection.Label = LOCTEXT("GasAbilityGeneratorSection", "GasAbilityGenerator");

			ItemSection.AddMenuEntry(
				"GenerateFromItemSpec",
				LOCTEXT("GenerateItem", "Generate Item"),
				LOCTEXT("GenerateItemTooltip", "Generate item asset from this spec"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"),
				FUIAction(FExecuteAction::CreateStatic(&FGasAbilityGeneratorModule::GenerateFromSelectedItemSpecs))
			);

			ItemSection.AddMenuEntry(
				"ValidateItemSpec",
				LOCTEXT("ValidateItem", "Validate Spec"),
				LOCTEXT("ValidateItemTooltip", "Check spec for errors without generating"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Check"),
				FUIAction(FExecuteAction::CreateStatic(&FGasAbilityGeneratorModule::ValidateSelectedItemSpecs))
			);
		}
	}));
}

void FGasAbilityGeneratorModule::OpenPluginWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(GasAbilityGeneratorTabName);
}

void FGasAbilityGeneratorModule::OpenNPCTableEditorWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(NPCTableEditorTabName);
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
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SNPCTableEditorWindow)
		];
}

//=============================================================================
// NPC Package Spec Handlers
//=============================================================================

void FGasAbilityGeneratorModule::GenerateFromSelectedNPCSpecs()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	TArray<UNPCPackageSpec*> Specs;
	for (const FAssetData& AssetData : SelectedAssets)
	{
		if (UNPCPackageSpec* Spec = Cast<UNPCPackageSpec>(AssetData.GetAsset()))
		{
			Specs.Add(Spec);
		}
	}

	if (Specs.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoNPCSpecsSelected", "No NPC Package Specs selected."));
		return;
	}

	// Validate all specs first
	TArray<FString> AllErrors;
	for (UNPCPackageSpec* Spec : Specs)
	{
		TArray<FString> Errors;
		if (!Spec->Validate(Errors))
		{
			for (const FString& Error : Errors)
			{
				AllErrors.Add(FString::Printf(TEXT("%s: %s"), *Spec->BaseName, *Error));
			}
		}
	}

	if (AllErrors.Num() > 0)
	{
		FString ErrorMessage = TEXT("Validation failed:\n\n");
		for (const FString& Error : AllErrors)
		{
			ErrorMessage += FString::Printf(TEXT("- %s\n"), *Error);
		}
		ErrorMessage += TEXT("\nFix these errors before generating.");
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ErrorMessage));
		return;
	}

	FScopedTransaction Transaction(LOCTEXT("GenerateFromNPCSpecs", "Generate from NPC Specs"));

	int32 SuccessCount = 0;

	for (UNPCPackageSpec* Spec : Specs)
	{
		UNPCPackageSpec* MergedSpec = Spec->GetMergedSpec();

		UE_LOG(LogTemp, Log, TEXT("[NPC Spec] Would generate for: %s"), *MergedSpec->BaseName);
		UE_LOG(LogTemp, Log, TEXT("  - NPCDef_%s"), *MergedSpec->BaseName);

		if (MergedSpec->Abilities.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("  - AC_%s"), *MergedSpec->BaseName);
		}

		if (MergedSpec->Activities.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("  - ActConfig_%s"), *MergedSpec->BaseName);
		}

		if (MergedSpec->Schedule.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("  - Schedule_%s"), *MergedSpec->BaseName);
		}

		if (MergedSpec->bAutoCreateDialogue)
		{
			UE_LOG(LogTemp, Log, TEXT("  - DBP_%s"), *MergedSpec->BaseName);
		}

		for (const FSpecGoalDefinition& Goal : MergedSpec->Goals)
		{
			UE_LOG(LogTemp, Log, TEXT("  - Goal_%s_%s"), *MergedSpec->BaseName, *Goal.Id);
		}

		for (const FSpecQuestDefinition& Quest : MergedSpec->Quests)
		{
			UE_LOG(LogTemp, Log, TEXT("  - Quest_%s_%s"), *MergedSpec->BaseName, *Quest.Id);
		}

		Spec->LastGeneratedTime = FDateTime::Now();
		Spec->MarkPackageDirty();
		SuccessCount++;
	}

	FString ResultMessage = FString::Printf(
		TEXT("Generation complete (preview mode).\n\nSpecs processed: %d\nCheck Output Log for details."),
		SuccessCount);

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ResultMessage));
}

void FGasAbilityGeneratorModule::ValidateSelectedNPCSpecs()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	TArray<FString> ValidationErrors;
	int32 SpecCount = 0;

	for (const FAssetData& AssetData : SelectedAssets)
	{
		if (UNPCPackageSpec* Spec = Cast<UNPCPackageSpec>(AssetData.GetAsset()))
		{
			SpecCount++;
			TArray<FString> Errors;
			if (!Spec->Validate(Errors))
			{
				for (const FString& Error : Errors)
				{
					ValidationErrors.Add(FString::Printf(TEXT("%s: %s"), *Spec->BaseName, *Error));
				}
			}
		}
	}

	if (SpecCount == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoNPCSpecsSelectedValidate", "No NPC Package Specs selected."));
		return;
	}

	FString ResultMessage;
	if (ValidationErrors.Num() == 0)
	{
		ResultMessage = FString::Printf(TEXT("Validation passed!\n\n%d spec(s) validated with no errors."), SpecCount);
	}
	else
	{
		ResultMessage = TEXT("VALIDATION ERRORS:\n\n");
		for (const FString& Error : ValidationErrors)
		{
			ResultMessage += FString::Printf(TEXT("- %s\n"), *Error);
		}
	}

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ResultMessage));
}

void FGasAbilityGeneratorModule::PreviewSelectedNPCSpecs()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	FString PreviewMessage = TEXT("GENERATION PLAN:\n\n");
	int32 SpecCount = 0;

	for (const FAssetData& AssetData : SelectedAssets)
	{
		if (UNPCPackageSpec* Spec = Cast<UNPCPackageSpec>(AssetData.GetAsset()))
		{
			SpecCount++;
			UNPCPackageSpec* MergedSpec = Spec->GetMergedSpec();

			PreviewMessage += FString::Printf(TEXT("=== %s ===\n"), *MergedSpec->BaseName);
			PreviewMessage += FString::Printf(TEXT("  [CREATE] NPCDef_%s\n"), *MergedSpec->BaseName);

			if (MergedSpec->Abilities.Num() > 0)
			{
				PreviewMessage += FString::Printf(TEXT("  [CREATE] AC_%s (%d abilities)\n"),
					*MergedSpec->BaseName, MergedSpec->Abilities.Num());
			}

			if (MergedSpec->Activities.Num() > 0)
			{
				PreviewMessage += FString::Printf(TEXT("  [CREATE] ActConfig_%s (%d activities)\n"),
					*MergedSpec->BaseName, MergedSpec->Activities.Num());
			}

			for (const FSpecGoalDefinition& Goal : MergedSpec->Goals)
			{
				PreviewMessage += FString::Printf(TEXT("  [CREATE] Goal_%s_%s\n"),
					*MergedSpec->BaseName, *Goal.Id);
			}

			if (MergedSpec->Schedule.Num() > 0)
			{
				PreviewMessage += FString::Printf(TEXT("  [CREATE] Schedule_%s (%d entries)\n"),
					*MergedSpec->BaseName, MergedSpec->Schedule.Num());
			}

			if (MergedSpec->bAutoCreateDialogue)
			{
				PreviewMessage += FString::Printf(TEXT("  [CREATE] DBP_%s\n"), *MergedSpec->BaseName);
			}

			if (MergedSpec->bAutoCreateTaggedDialogue)
			{
				PreviewMessage += FString::Printf(TEXT("  [CREATE] %s_TaggedDialogue\n"), *MergedSpec->BaseName);
			}

			for (const FSpecQuestDefinition& Quest : MergedSpec->Quests)
			{
				PreviewMessage += FString::Printf(TEXT("  [CREATE] Quest_%s_%s\n"),
					*MergedSpec->BaseName, *Quest.Id);
			}

			PreviewMessage += TEXT("\n");
		}
	}

	if (SpecCount == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoNPCSpecsSelectedPreview", "No NPC Package Specs selected."));
		return;
	}

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(PreviewMessage));
}

//=============================================================================
// Quest Spec Handlers
//=============================================================================

void FGasAbilityGeneratorModule::GenerateFromSelectedQuestSpecs()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	TArray<UQuestSpec*> Specs;
	for (const FAssetData& AssetData : SelectedAssets)
	{
		if (UQuestSpec* Spec = Cast<UQuestSpec>(AssetData.GetAsset()))
		{
			Specs.Add(Spec);
		}
	}

	if (Specs.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoQuestSpecsSelected", "No Quest Specs selected."));
		return;
	}

	TArray<FString> AllErrors;
	for (UQuestSpec* Spec : Specs)
	{
		TArray<FString> Errors;
		if (!Spec->Validate(Errors))
		{
			for (const FString& Error : Errors)
			{
				AllErrors.Add(FString::Printf(TEXT("%s: %s"), *Spec->QuestName, *Error));
			}
		}
	}

	if (AllErrors.Num() > 0)
	{
		FString ErrorMessage = TEXT("Validation failed:\n\n");
		for (const FString& Error : AllErrors)
		{
			ErrorMessage += FString::Printf(TEXT("- %s\n"), *Error);
		}
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ErrorMessage));
		return;
	}

	FString ResultMessage = FString::Printf(
		TEXT("Quest generation preview:\n\n%d quest(s) would be generated.\nActual generation not yet implemented."),
		Specs.Num());

	for (UQuestSpec* Spec : Specs)
	{
		UE_LOG(LogTemp, Log, TEXT("[Quest Spec] Would generate: Quest_%s"), *Spec->QuestName);
		Spec->LastGeneratedTime = FDateTime::Now();
		Spec->MarkPackageDirty();
	}

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ResultMessage));
}

void FGasAbilityGeneratorModule::ValidateSelectedQuestSpecs()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	TArray<FString> ValidationErrors;
	int32 SpecCount = 0;

	for (const FAssetData& AssetData : SelectedAssets)
	{
		if (UQuestSpec* Spec = Cast<UQuestSpec>(AssetData.GetAsset()))
		{
			SpecCount++;
			TArray<FString> Errors;
			if (!Spec->Validate(Errors))
			{
				for (const FString& Error : Errors)
				{
					ValidationErrors.Add(FString::Printf(TEXT("%s: %s"), *Spec->QuestName, *Error));
				}
			}
		}
	}

	if (SpecCount == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoQuestSpecsSelectedValidate", "No Quest Specs selected."));
		return;
	}

	FString ResultMessage;
	if (ValidationErrors.Num() == 0)
	{
		ResultMessage = FString::Printf(TEXT("Validation passed!\n\n%d quest spec(s) validated."), SpecCount);
	}
	else
	{
		ResultMessage = TEXT("VALIDATION ERRORS:\n\n");
		for (const FString& Error : ValidationErrors)
		{
			ResultMessage += FString::Printf(TEXT("- %s\n"), *Error);
		}
	}

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ResultMessage));
}

//=============================================================================
// Item Spec Handlers
//=============================================================================

void FGasAbilityGeneratorModule::GenerateFromSelectedItemSpecs()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	TArray<UItemSpec*> Specs;
	for (const FAssetData& AssetData : SelectedAssets)
	{
		if (UItemSpec* Spec = Cast<UItemSpec>(AssetData.GetAsset()))
		{
			Specs.Add(Spec);
		}
	}

	if (Specs.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoItemSpecsSelected", "No Item Specs selected."));
		return;
	}

	TArray<FString> AllErrors;
	for (UItemSpec* Spec : Specs)
	{
		TArray<FString> Errors;
		if (!Spec->Validate(Errors))
		{
			for (const FString& Error : Errors)
			{
				AllErrors.Add(FString::Printf(TEXT("%s: %s"), *Spec->ItemName, *Error));
			}
		}
	}

	if (AllErrors.Num() > 0)
	{
		FString ErrorMessage = TEXT("Validation failed:\n\n");
		for (const FString& Error : AllErrors)
		{
			ErrorMessage += FString::Printf(TEXT("- %s\n"), *Error);
		}
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ErrorMessage));
		return;
	}

	FString ResultMessage = FString::Printf(
		TEXT("Item generation preview:\n\n%d item(s) would be generated.\nActual generation not yet implemented."),
		Specs.Num());

	for (UItemSpec* Spec : Specs)
	{
		UE_LOG(LogTemp, Log, TEXT("[Item Spec] Would generate: EI_%s"), *Spec->ItemName);
		Spec->LastGeneratedTime = FDateTime::Now();
		Spec->MarkPackageDirty();
	}

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ResultMessage));
}

void FGasAbilityGeneratorModule::ValidateSelectedItemSpecs()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	TArray<FString> ValidationErrors;
	int32 SpecCount = 0;

	for (const FAssetData& AssetData : SelectedAssets)
	{
		if (UItemSpec* Spec = Cast<UItemSpec>(AssetData.GetAsset()))
		{
			SpecCount++;
			TArray<FString> Errors;
			if (!Spec->Validate(Errors))
			{
				for (const FString& Error : Errors)
				{
					ValidationErrors.Add(FString::Printf(TEXT("%s: %s"), *Spec->ItemName, *Error));
				}
			}
		}
	}

	if (SpecCount == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoItemSpecsSelectedValidate", "No Item Specs selected."));
		return;
	}

	FString ResultMessage;
	if (ValidationErrors.Num() == 0)
	{
		ResultMessage = FString::Printf(TEXT("Validation passed!\n\n%d item spec(s) validated."), SpecCount);
	}
	else
	{
		ResultMessage = TEXT("VALIDATION ERRORS:\n\n");
		for (const FString& Error : ValidationErrors)
		{
			ResultMessage += FString::Printf(TEXT("- %s\n"), *Error);
		}
	}

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ResultMessage));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGasAbilityGeneratorModule, GasAbilityGenerator)
