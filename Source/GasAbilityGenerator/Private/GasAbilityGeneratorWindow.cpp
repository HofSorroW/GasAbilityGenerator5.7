// GasAbilityGenerator v3.0
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v3.0: Added Dry Run and Force checkboxes for metadata-aware regeneration
// v2.5.0: Renamed to GasAbilityGenerator for generic UE project compatibility
// v2.3.0: Added 12 new asset type generators with dependency-based generation order
// v2.2.0: Added manifest validation - only creates assets listed in manifest whitelist

#include "GasAbilityGeneratorWindow.h"
#include "GasAbilityGeneratorGenerators.h"
#include "GasAbilityGeneratorParser.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "DesktopPlatformModule.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "HAL/PlatformApplicationMisc.h"

#define LOCTEXT_NAMESPACE "GasAbilityGenerator"

void SGasAbilityGeneratorWindow::Construct(const FArguments& InArgs)
{
	bIsGenerating = false;

	LoadConfig();

	ChildSlot
	[
		SNew(SVerticalBox)

		// Title
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10, 10, 10, 5)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Title", "GAS Ability Generator v3.0"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
		]

		// Manifest Folder Path
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10, 5)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 10, 0)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ManifestFolder", "Manifest Folder:"))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0, 0, 5, 0)
			[
				SAssignNew(GuidesPathTextBox, SEditableTextBox)
				.Text(FText::FromString(GuidesFolderPath))
				.IsReadOnly(true)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("Browse", "Browse..."))
				.OnClicked(this, &SGasAbilityGeneratorWindow::OnBrowseClicked)
			]
		]

		// Status
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10, 5)
		[
			SAssignNew(StatusText, STextBlock)
			.Text(LOCTEXT("StatusReady", "Ready"))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10, 5)
		[
			SNew(SSeparator)
		]

		// v3.0: Options Row (Dry Run / Force)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10, 5)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 20, 0)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SAssignNew(DryRunCheckbox, SCheckBox)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(4, 0, 0, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DryRun", "Dry Run (Preview Only)"))
					.ToolTipText(LOCTEXT("DryRunTooltip", "Preview what would be created, modified, or skipped without making changes"))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SAssignNew(ForceCheckbox, SCheckBox)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(4, 0, 0, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Force", "Force Regenerate"))
					.ToolTipText(LOCTEXT("ForceTooltip", "Regenerate all assets even if they were manually edited (overwrite conflicts)"))
				]
			]
		]

		// Buttons
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10, 5)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 10, 0)
			[
				SAssignNew(GenerateTagsButton, SButton)
				.Text(LOCTEXT("GenerateTags", "Generate Tags"))
				.OnClicked(this, &SGasAbilityGeneratorWindow::OnGenerateTagsClicked)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 10, 0)
			[
				SAssignNew(GenerateAssetsButton, SButton)
				.Text(LOCTEXT("GenerateAssets", "Generate Assets"))
				.OnClicked(this, &SGasAbilityGeneratorWindow::OnGenerateAssetsClicked)
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("Settings", "Settings"))
				.OnClicked(this, &SGasAbilityGeneratorWindow::OnSettingsClicked)
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10, 5)
		[
			SNew(SSeparator)
		]

		// Log Header with buttons
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10, 5, 10, 0)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("LogLabel", "Log Output:"))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5, 0, 0, 0)
			[
				SAssignNew(CopyLogButton, SButton)
				.Text(LOCTEXT("CopyLog", "Copy Log"))
				.OnClicked(this, &SGasAbilityGeneratorWindow::OnCopyLogClicked)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5, 0, 0, 0)
			[
				SNew(SButton)
				.Text(LOCTEXT("ClearLog", "Clear Log"))
				.OnClicked(this, &SGasAbilityGeneratorWindow::OnClearLogClicked)
			]
		]

		// Log
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(10, 5)
		[
			SNew(SBox)
			.MinDesiredHeight(200)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SAssignNew(LogTextBox, SMultiLineEditableTextBox)
					.IsReadOnly(true)
					.BackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f))
				]
			]
		]
	];

	// Auto-load manifest if path is set
	if (!GuidesFolderPath.IsEmpty())
	{
		LoadManifest();
	}
}

SGasAbilityGeneratorWindow::~SGasAbilityGeneratorWindow()
{
	SaveConfig();
}

void SGasAbilityGeneratorWindow::LoadConfig()
{
	GConfig->GetString(TEXT("GasAbilityGenerator"), TEXT("ManifestFolderPath"), GuidesFolderPath, GEditorPerProjectIni);
}

void SGasAbilityGeneratorWindow::SaveConfig()
{
	GConfig->SetString(TEXT("GasAbilityGenerator"), TEXT("ManifestFolderPath"), *GuidesFolderPath, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

FReply SGasAbilityGeneratorWindow::OnBrowseClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		FString SelectedPath;
		const bool bOpened = DesktopPlatform->OpenDirectoryDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			TEXT("Select Manifest Folder"),
			FPaths::ProjectDir(),
			SelectedPath
		);

		if (bOpened)
		{
			GuidesFolderPath = SelectedPath;
			GuidesPathTextBox->SetText(FText::FromString(GuidesFolderPath));
			SaveConfig();
			LoadManifest();
		}
	}

	return FReply::Handled();
}

bool SGasAbilityGeneratorWindow::LoadManifest()
{
	FString ManifestPath = FPaths::Combine(GuidesFolderPath, TEXT("manifest.yaml"));

	if (!FPaths::FileExists(ManifestPath))
	{
		UpdateStatus(TEXT("ERROR: manifest.yaml not found"));
		AppendLog(FString::Printf(TEXT("ERROR: manifest.yaml not found at: %s"), *ManifestPath));
		return false;
	}

	FString ManifestContent;
	if (!FFileHelper::LoadFileToString(ManifestContent, *ManifestPath))
	{
		UpdateStatus(TEXT("ERROR: Failed to read manifest.yaml"));
		AppendLog(TEXT("ERROR: Failed to read manifest.yaml"));
		return false;
	}

	ParseManifest(ManifestContent);

	int32 TagCount = ManifestData.GetTagCount();
	int32 AssetCount = ManifestData.GetTotalAssetCount();

	UpdateStatus(FString::Printf(TEXT("Loaded: %d tags, %d assets"), TagCount, AssetCount));

	AppendLog(TEXT("=== MANIFEST LOADED ==="));
	AppendLog(FString::Printf(TEXT("Project Root: %s"), *ManifestData.ProjectRoot));
	AppendLog(TEXT(""));
	AppendLog(TEXT("Asset Breakdown:"));
	AppendLog(FString::Printf(TEXT("  Tags: %d"), TagCount));
	AppendLog(FString::Printf(TEXT("  Enumerations: %d"), ManifestData.Enumerations.Num()));
	AppendLog(FString::Printf(TEXT("  Input Actions: %d"), ManifestData.InputActions.Num()));
	AppendLog(FString::Printf(TEXT("  Input Mapping Contexts: %d"), ManifestData.InputMappingContexts.Num()));
	AppendLog(FString::Printf(TEXT("  Gameplay Effects: %d"), ManifestData.GameplayEffects.Num()));
	AppendLog(FString::Printf(TEXT("  Gameplay Abilities: %d"), ManifestData.GameplayAbilities.Num()));
	AppendLog(FString::Printf(TEXT("  Actor Blueprints: %d"), ManifestData.ActorBlueprints.Num()));
	AppendLog(FString::Printf(TEXT("  Widget Blueprints: %d"), ManifestData.WidgetBlueprints.Num()));
	AppendLog(FString::Printf(TEXT("  Blackboards: %d"), ManifestData.Blackboards.Num()));
	AppendLog(FString::Printf(TEXT("  Behavior Trees: %d"), ManifestData.BehaviorTrees.Num()));
	AppendLog(FString::Printf(TEXT("  Materials: %d"), ManifestData.Materials.Num()));
	AppendLog(FString::Printf(TEXT("TOTAL: %d tags, %d assets"), TagCount, AssetCount));

	return true;
}

void SGasAbilityGeneratorWindow::ParseManifest(const FString& ManifestContent)
{
	FGasAbilityGeneratorParser::ParseManifest(ManifestContent, ManifestData);
}

FReply SGasAbilityGeneratorWindow::OnGenerateTagsClicked()
{
	if (bIsGenerating)
	{
		AppendLog(TEXT("WARNING: Generation already in progress, ignoring duplicate request"));
		return FReply::Handled();
	}

	bIsGenerating = true;
	GenerateTags();
	bIsGenerating = false;

	return FReply::Handled();
}

FReply SGasAbilityGeneratorWindow::OnGenerateAssetsClicked()
{
	if (bIsGenerating)
	{
		AppendLog(TEXT("WARNING: Generation already in progress, ignoring duplicate request"));
		return FReply::Handled();
	}

	bIsGenerating = true;
	GenerateAssets();
	bIsGenerating = false;

	return FReply::Handled();
}

FReply SGasAbilityGeneratorWindow::OnSettingsClicked()
{
	return FReply::Handled();
}

FReply SGasAbilityGeneratorWindow::OnCopyLogClicked()
{
	if (LogTextBox.IsValid())
	{
		FString LogContent = LogTextBox->GetText().ToString();
		FPlatformApplicationMisc::ClipboardCopy(*LogContent);
		UpdateStatus(TEXT("Log copied to clipboard"));
	}
	return FReply::Handled();
}

FReply SGasAbilityGeneratorWindow::OnClearLogClicked()
{
	ClearLog();
	UpdateStatus(TEXT("Log cleared"));
	return FReply::Handled();
}

void SGasAbilityGeneratorWindow::GenerateTags()
{
	if (ManifestData.Tags.Num() == 0)
	{
		AppendLog(TEXT("No tags to generate"));
		return;
	}

	FString TagsIniPath = ManifestData.TagsIniPath;
	if (TagsIniPath.IsEmpty())
	{
		TagsIniPath = TEXT("Config/DefaultGameplayTags.ini");
	}

	bool bIsAbsolutePath = !FPaths::IsRelative(TagsIniPath) && TagsIniPath.Len() > 2 && TagsIniPath[1] == TEXT(':');

	if (bIsAbsolutePath)
	{
		// Already absolute, use as-is
	}
	else if (TagsIniPath.StartsWith(TEXT("/Config/")) || TagsIniPath.StartsWith(TEXT("Config/")))
	{
		FString RelativePath = TagsIniPath;
		if (RelativePath.StartsWith(TEXT("/")))
		{
			RelativePath = RelativePath.Mid(1);
		}
		TagsIniPath = FPaths::Combine(FPaths::ProjectDir(), RelativePath);
	}
	else
	{
		TagsIniPath = FPaths::Combine(FPaths::ProjectConfigDir(), TagsIniPath);
	}

	FPaths::NormalizeFilename(TagsIniPath);

	AppendLog(FString::Printf(TEXT("Generating %d tags to: %s"), ManifestData.Tags.Num(), *TagsIniPath));

	FGenerationSummary Summary = FTagGenerator::GenerateTags(ManifestData.Tags, TagsIniPath);

	ShowResultsDialog(Summary);
}

void SGasAbilityGeneratorWindow::GenerateAssets()
{
	// v3.0: Check dry run and force modes from checkboxes
	bool bDryRun = DryRunCheckbox.IsValid() && DryRunCheckbox->IsChecked();
	bool bForce = ForceCheckbox.IsValid() && ForceCheckbox->IsChecked();

	if (bDryRun)
	{
		AppendLog(TEXT("=== DRY RUN MODE - No changes will be made ==="));
	}
	else if (bForce)
	{
		AppendLog(TEXT("=== FORCE MODE - Will overwrite conflicted assets ==="));
	}

	AppendLog(TEXT("Starting asset generation..."));

	// v3.0: Set modes before generation
	FGeneratorBase::SetDryRunMode(bDryRun);
	FGeneratorBase::SetForceMode(bForce);
	FGeneratorBase::ClearDryRunSummary();

	// Set manifest path for metadata tracking
	FString ManifestPath = FPaths::Combine(GuidesFolderPath, TEXT("manifest.yaml"));
	FGeneratorBase::SetManifestPath(ManifestPath);

	FGeneratorBase::SetActiveManifest(&ManifestData);
	AppendLog(FString::Printf(TEXT("Manifest validation enabled: %d assets whitelisted"),
		ManifestData.GetAssetWhitelist().Num()));

	FGenerationSummary Summary;

	// PHASE 1: No Dependencies
	for (const auto& Definition : ManifestData.Enumerations)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FEnumerationGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	for (const auto& Definition : ManifestData.FloatCurves)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FFloatCurveGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	for (const auto& Definition : ManifestData.InputActions)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FInputActionGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	for (const auto& Definition : ManifestData.InputMappingContexts)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FInputMappingContextGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// PHASE 2: Base Assets
	for (const auto& Definition : ManifestData.GameplayEffects)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FGameplayEffectGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// Actor Blueprints must be generated BEFORE Gameplay Abilities
	// because abilities may reference actor blueprints (e.g., BP_FatherCompanion)
	for (const auto& Definition : ManifestData.ActorBlueprints)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FActorBlueprintGenerator::Generate(Definition, ManifestData.ProjectRoot, &ManifestData);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	for (const auto& Definition : ManifestData.GameplayAbilities)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FGameplayAbilityGenerator::Generate(Definition, ManifestData.ProjectRoot);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	for (const auto& Definition : ManifestData.AnimationMontages)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FAnimationMontageGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	for (const auto& Definition : ManifestData.AnimationNotifies)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FAnimationNotifyGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// PHASE 3: Blueprint Assets
	for (const auto& Definition : ManifestData.Blackboards)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FBlackboardGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	for (const auto& Definition : ManifestData.BehaviorTrees)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FBehaviorTreeGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	for (const auto& Definition : ManifestData.Materials)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FMaterialGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// ActorBlueprints moved to PHASE 2 (before GameplayAbilities)

	for (const auto& Definition : ManifestData.WidgetBlueprints)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FWidgetBlueprintGenerator::Generate(Definition, &ManifestData);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	for (const auto& Definition : ManifestData.DialogueBlueprints)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FDialogueBlueprintGenerator::Generate(Definition, ManifestData.ProjectRoot, &ManifestData);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// PHASE 4: Data Assets
	for (const auto& Definition : ManifestData.EquippableItems)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FEquippableItemGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	for (const auto& Definition : ManifestData.Activities)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FActivityGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	for (const auto& Definition : ManifestData.AbilityConfigurations)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FAbilityConfigurationGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	for (const auto& Definition : ManifestData.NarrativeEvents)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FNarrativeEventGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// PHASE 5: Configuration Assets
	for (const auto& Definition : ManifestData.ActivityConfigurations)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FActivityConfigurationGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	for (const auto& Definition : ManifestData.ItemCollections)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FItemCollectionGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	// PHASE 6: Definition Assets
	for (const auto& Definition : ManifestData.NPCDefinitions)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FNPCDefinitionGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	for (const auto& Definition : ManifestData.CharacterDefinitions)
	{
		UpdateStatus(FString::Printf(TEXT("Generating: %s"), *Definition.Name));
		FGenerationResult Result = FCharacterDefinitionGenerator::Generate(Definition);
		Summary.AddResult(Result);
		AppendLog(FString::Printf(TEXT("[%s] %s"),
			Result.Status == EGenerationStatus::New ? TEXT("NEW") :
			Result.Status == EGenerationStatus::Skipped ? TEXT("SKIP") : TEXT("FAIL"),
			*Result.AssetName));
	}

	FGeneratorBase::ClearActiveManifest();

	// v3.0: Show appropriate results dialog based on mode
	if (bDryRun)
	{
		UpdateStatus(TEXT("Dry run complete. No changes made."));
		ShowDryRunResultsDialog(FGeneratorBase::GetDryRunSummary());
	}
	else
	{
		UpdateStatus(TEXT("Generation complete. Refresh Content Browser."));
		ShowResultsDialog(Summary);
	}

	// v3.0: Clean up modes
	FGeneratorBase::SetDryRunMode(false);
	FGeneratorBase::SetForceMode(false);
}

void SGasAbilityGeneratorWindow::ShowResultsDialog(const FGenerationSummary& Summary)
{
	FString ResultsMessage = FString::Printf(
		TEXT("GAS ABILITY GENERATOR RESULTS\n")
		TEXT("=============================\n\n")
		TEXT("Plugin Version: 3.0\n\n")
		TEXT("SUMMARY:\n")
		TEXT("  NEW:     %d assets created\n")
		TEXT("  SKIPPED: %d assets (already exist)\n")
		TEXT("  FAILED:  %d assets\n")
		TEXT("  TOTAL:   %d assets processed\n\n"),
		Summary.NewCount,
		Summary.SkippedCount,
		Summary.FailedCount,
		Summary.GetTotal()
	);

	TMap<FString, TArray<const FGenerationResult*>> CategorizedResults;
	for (const FGenerationResult& Result : Summary.Results)
	{
		CategorizedResults.FindOrAdd(Result.Category).Add(&Result);
	}

	for (const auto& Category : CategorizedResults)
	{
		ResultsMessage += FString::Printf(TEXT("%s:\n"), *Category.Key);
		for (const FGenerationResult* Result : Category.Value)
		{
			FString Indicator;
			switch (Result->Status)
			{
			case EGenerationStatus::New:
				Indicator = TEXT("+");
				break;
			case EGenerationStatus::Skipped:
				Indicator = TEXT("-");
				break;
			case EGenerationStatus::Failed:
				Indicator = TEXT("X");
				break;
			}
			ResultsMessage += FString::Printf(TEXT("  %s %s\n"), *Indicator, *Result->AssetName);
		}
		ResultsMessage += TEXT("\n");
	}

	AppendLog(ResultsMessage);

	UE_LOG(LogTemp, Log, TEXT("\n%s"), *ResultsMessage);
}

void SGasAbilityGeneratorWindow::AppendLog(const FString& Message)
{
	if (LogTextBox.IsValid())
	{
		FString CurrentText = LogTextBox->GetText().ToString();
		if (!CurrentText.IsEmpty())
		{
			CurrentText += TEXT("\n");
		}
		CurrentText += Message;
		LogTextBox->SetText(FText::FromString(CurrentText));
	}
}

void SGasAbilityGeneratorWindow::ClearLog()
{
	if (LogTextBox.IsValid())
	{
		LogTextBox->SetText(FText::GetEmpty());
	}
}

void SGasAbilityGeneratorWindow::UpdateStatus(const FString& Status)
{
	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(Status));
	}
}

void SGasAbilityGeneratorWindow::ShowDryRunResultsDialog(const FDryRunSummary& DryRunSummary)
{
	// Format the dry run report similar to commandlet output
	FString ResultsMessage = FString::Printf(
		TEXT("========================================\n")
		TEXT("GasAbilityGenerator Dry Run Report\n")
		TEXT("========================================\n")
		TEXT("SUMMARY: %d CREATE, %d MODIFY, %d SKIP, %d CONFLICT\n\n"),
		DryRunSummary.CreateCount,
		DryRunSummary.ModifyCount,
		DryRunSummary.SkipCount,
		DryRunSummary.ConflictCount
	);

	// CREATE section
	if (DryRunSummary.CreateCount > 0)
	{
		ResultsMessage += FString::Printf(TEXT("--- CREATE (%d new assets) ---\n"), DryRunSummary.CreateCount);
		for (const FDryRunResult& Result : DryRunSummary.Results)
		{
			if (Result.Status == EDryRunStatus::WillCreate)
			{
				ResultsMessage += FString::Printf(TEXT("[CREATE] %s\n"), *Result.AssetName);
			}
		}
		ResultsMessage += TEXT("\n");
	}

	// MODIFY section
	if (DryRunSummary.ModifyCount > 0)
	{
		ResultsMessage += FString::Printf(TEXT("--- MODIFY (%d manifest changes, no manual edits) ---\n"), DryRunSummary.ModifyCount);
		for (const FDryRunResult& Result : DryRunSummary.Results)
		{
			if (Result.Status == EDryRunStatus::WillModify)
			{
				ResultsMessage += FString::Printf(TEXT("[MODIFY] %s"), *Result.AssetName);
				if (!Result.Reason.IsEmpty())
				{
					ResultsMessage += FString::Printf(TEXT(" - %s"), *Result.Reason);
				}
				ResultsMessage += TEXT("\n");
			}
		}
		ResultsMessage += TEXT("\n");
	}

	// CONFLICT section
	if (DryRunSummary.ConflictCount > 0)
	{
		ResultsMessage += FString::Printf(TEXT("--- CONFLICTS (%d require attention) ---\n"), DryRunSummary.ConflictCount);
		for (const FDryRunResult& Result : DryRunSummary.Results)
		{
			if (Result.Status == EDryRunStatus::Conflicted)
			{
				ResultsMessage += FString::Printf(TEXT("[CONFLICT] %s\n"), *Result.AssetName);
				if (!Result.Reason.IsEmpty())
				{
					ResultsMessage += FString::Printf(TEXT("  Reason: %s\n"), *Result.Reason);
				}
				ResultsMessage += TEXT("  Action: Use Force Regenerate to overwrite or resolve manually\n");
			}
		}
		ResultsMessage += TEXT("\n");
	}

	// SKIP section (abbreviated if many)
	if (DryRunSummary.SkipCount > 0)
	{
		ResultsMessage += FString::Printf(TEXT("--- SKIP (%d unchanged) ---\n"), DryRunSummary.SkipCount);
		int32 SkipShown = 0;
		const int32 MaxSkipToShow = 10;
		for (const FDryRunResult& Result : DryRunSummary.Results)
		{
			if (Result.Status == EDryRunStatus::WillSkip)
			{
				if (SkipShown < MaxSkipToShow)
				{
					ResultsMessage += FString::Printf(TEXT("[SKIP] %s - No changes\n"), *Result.AssetName);
					SkipShown++;
				}
			}
		}
		if (DryRunSummary.SkipCount > MaxSkipToShow)
		{
			ResultsMessage += FString::Printf(TEXT("... and %d more unchanged assets\n"), DryRunSummary.SkipCount - MaxSkipToShow);
		}
	}

	AppendLog(ResultsMessage);

	UE_LOG(LogTemp, Log, TEXT("\n%s"), *ResultsMessage);
}

#undef LOCTEXT_NAMESPACE
