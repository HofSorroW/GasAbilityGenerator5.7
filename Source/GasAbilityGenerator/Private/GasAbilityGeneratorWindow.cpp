// GasAbilityGenerator v3.7
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v3.7: Added NPC Creation feature - one-click NPC asset generation with v3.0 hash safety
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
#include "Internationalization/Regex.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/DataTable.h"
#include "Misc/PackageName.h"
#include "Kismet2/KismetEditorUtilities.h"  // v3.7: For NPC Creation blueprint generation

#define LOCTEXT_NAMESPACE "GasAbilityGenerator"

// v3.7: NPC Creation asset status for v3.0 hash safety
enum class ENPCAssetStatus : uint8
{
	Create,     // Asset doesn't exist - will create
	Modify,     // Manifest changed, asset not manually edited - safe to regenerate
	Skip,       // No changes or manually edited - skip
	Conflict    // Both manifest and asset changed - requires force
};

// v3.7: Compute input hash for NPC Definition from UI parameters
static uint64 ComputeNPCInputHash(const FString& NPCName)
{
	// Hash includes all parameters that affect the generated NPC
	// This creates a unique identifier for the current configuration
	uint64 Hash = GetTypeHash(NPCName);

	// Include fixed parameters (these are hardcoded in the UI for now)
	Hash ^= GetTypeHash(FString(TEXT("AC_NPC_Default")));  // AbilityConfiguration
	Hash ^= GetTypeHash(FString(TEXT("AC_RunAndGun")));    // ActivityConfiguration
	Hash ^= GetTypeHash(FString(TEXT("TableToRoll")));     // Using TableToRoll method (like Seth)
	Hash ^= GetTypeHash(FString(TEXT("Narrative.State.Invulnerable"))); // OwnedTags
	Hash ^= GetTypeHash(FString(TEXT("Narrative.Factions.Heroes"))); // Factions
	Hash ^= GetTypeHash(FString(TEXT("BuyItemPercentage=0.0"))); // Default non-vendor
	Hash ^= GetTypeHash(FString(TEXT("SellItemPercentage=0.0"))); // Default non-vendor

	// Version identifier for hash compatibility
	Hash ^= 0x37000002ULL; // v3.7 NPC UI version 2 (TableToRoll, 0% buy/sell)

	return Hash;
}

// v3.7: Check asset status using v3.0 metadata system
static ENPCAssetStatus CheckNPCAssetStatus(
	const FString& PackagePath,
	uint64 InputHash,
	FString& OutStatusReason)
{
	// Check if asset exists
	if (!FPackageName::DoesPackageExist(PackagePath))
	{
		OutStatusReason = TEXT("New asset");
		return ENPCAssetStatus::Create;
	}

	// Try to load asset
	FString FullAssetPath = PackagePath + TEXT(".") + FPackageName::GetShortName(PackagePath);
	UObject* ExistingAsset = StaticLoadObject(UObject::StaticClass(), nullptr, *FullAssetPath);

	if (!ExistingAsset)
	{
		OutStatusReason = TEXT("Exists but couldn't load");
		return ENPCAssetStatus::Skip;
	}

	// Check for generator metadata
	UGeneratorAssetMetadata* Metadata = FGeneratorBase::GetAssetMetadata(ExistingAsset);

	if (!Metadata)
	{
		// No metadata - likely manual asset, skip to avoid overwriting
		OutStatusReason = TEXT("No generator metadata (likely manual)");
		return ENPCAssetStatus::Skip;
	}

	// Has metadata - compare hashes
	bool bInputChanged = Metadata->HasInputChanged(InputHash);

	// Compute current output hash
	uint64 CurrentOutputHash = FGeneratorBase::ComputeDataAssetOutputHash(ExistingAsset);
	bool bOutputChanged = Metadata->HasOutputChanged(CurrentOutputHash);

	if (!bInputChanged && !bOutputChanged)
	{
		OutStatusReason = TEXT("No changes (hashes match)");
		return ENPCAssetStatus::Skip;
	}
	else if (bInputChanged && !bOutputChanged)
	{
		OutStatusReason = TEXT("Config changed, no manual edits");
		return ENPCAssetStatus::Modify;
	}
	else if (bInputChanged && bOutputChanged)
	{
		OutStatusReason = TEXT("CONFLICT: Both config AND asset changed");
		return ENPCAssetStatus::Conflict;
	}
	else // !bInputChanged && bOutputChanged
	{
		OutStatusReason = TEXT("Manually edited, config unchanged");
		return ENPCAssetStatus::Skip;
	}
}

// v3.7: Store NPC asset metadata after creation/modification
static void StoreNPCAssetMetadata(
	UObject* Asset,
	const FString& GeneratorId,
	const FString& AssetKey,
	uint64 InputHash)
{
	if (!Asset) return;

	FGeneratorMetadata Metadata;
	Metadata.GeneratorId = GeneratorId;
	Metadata.ManifestPath = TEXT("NPC_Creation_UI"); // Special marker for UI-generated assets
	Metadata.ManifestAssetKey = AssetKey;
	Metadata.InputHash = InputHash;
	Metadata.OutputHash = FGeneratorBase::ComputeDataAssetOutputHash(Asset);
	Metadata.GeneratorVersion = TEXT("3.7");
	Metadata.Timestamp = FDateTime::Now();
	Metadata.bIsGenerated = true;

	FGeneratorBase::StoreAssetMetadata(Asset, Metadata);
}

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
			.Text(LOCTEXT("Title", "GAS Ability Generator v3.7"))
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

		// v3.7: NPC Creation Section
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
				.Text(LOCTEXT("NPCName", "NPC Name:"))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(0.3f)
			.Padding(0, 0, 10, 0)
			[
				SAssignNew(NPCNameTextBox, SEditableTextBox)
				.HintText(LOCTEXT("NPCNameHint", "Enter NPC name..."))
				.OnTextChanged(this, &SGasAbilityGeneratorWindow::OnNPCNameChanged)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(CreateNPCButton, SButton)
				.Text(LOCTEXT("CreateNPC", "Create NPC"))
				.OnClicked(this, &SGasAbilityGeneratorWindow::OnCreateNPCClicked)
				.IsEnabled(this, &SGasAbilityGeneratorWindow::IsCreateNPCButtonEnabled)
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNullWidget::NullWidget
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

// ============================================================================
// v3.7: NPC Creation Functions
// ============================================================================

void SGasAbilityGeneratorWindow::OnNPCNameChanged(const FText& NewText)
{
	// The button will automatically update its enabled state via IsCreateNPCButtonEnabled
}

bool SGasAbilityGeneratorWindow::IsCreateNPCButtonEnabled() const
{
	if (!NPCNameTextBox.IsValid())
	{
		return false;
	}

	FString NPCName = NPCNameTextBox->GetText().ToString().TrimStartAndEnd();
	return !NPCName.IsEmpty();
}

FReply SGasAbilityGeneratorWindow::OnCreateNPCClicked()
{
	if (!NPCNameTextBox.IsValid())
	{
		return FReply::Handled();
	}

	FString NPCName = NPCNameTextBox->GetText().ToString().TrimStartAndEnd();
	if (NPCName.IsEmpty())
	{
		AppendLog(TEXT("ERROR: NPC Name is required"));
		return FReply::Handled();
	}

	// Validate NPC name (alphanumeric and underscores, can start with number)
	FRegexPattern Pattern(TEXT("^[a-zA-Z0-9_]+$"));
	FRegexMatcher Matcher(Pattern, NPCName);
	if (!Matcher.FindNext())
	{
		AppendLog(TEXT("ERROR: NPC Name must contain only letters, numbers, and underscores"));
		return FReply::Handled();
	}

	CreateNPCAssets(NPCName);

	return FReply::Handled();
}

void SGasAbilityGeneratorWindow::CreateNPCAssets(const FString& NPCName)
{
	AppendLog(TEXT(""));
	AppendLog(TEXT("=============================================="));
	AppendLog(FString::Printf(TEXT("Creating NPC: %s"), *NPCName));
	AppendLog(TEXT("=============================================="));

	// Base path: /Game/NPC/{NPCName}/
	FString BasePath = FString::Printf(TEXT("/Game/NPC/%s"), *NPCName);

	int32 SuccessCount = 0;
	int32 FailCount = 0;
	int32 SkipCount = 0;
	int32 ModifyCount = 0;
	int32 ConflictCount = 0;

	// v3.7: Compute input hash for v3.0 safety system
	uint64 InputHash = ComputeNPCInputHash(NPCName);

	// Store created assets for linking
	UObject* CreatedNPCDef = nullptr;
	UBlueprint* CreatedDialogue = nullptr;
	UObject* CreatedTaggedDialogueSet = nullptr;
	UBlueprint* CreatedGreetingsDialogue = nullptr;
	UDataTable* CreatedItemLoadout = nullptr;
	UBlueprint* CreatedTriggerBP = nullptr;

	// v3.7: Track asset statuses for Phase 2 decisions
	ENPCAssetStatus NPCDefStatus = ENPCAssetStatus::Skip;
	ENPCAssetStatus DialogueStatus = ENPCAssetStatus::Skip;
	ENPCAssetStatus TDSStatus = ENPCAssetStatus::Skip;
	ENPCAssetStatus GreetingsStatus = ENPCAssetStatus::Skip;
	ENPCAssetStatus ItemLoadoutStatus = ENPCAssetStatus::Skip;
	ENPCAssetStatus TriggerBPStatus = ENPCAssetStatus::Skip;

	// Get required classes
	UClass* NPCDefClass = FindObject<UClass>(nullptr, TEXT("/Script/NarrativeArsenal.NPCDefinition"));
	UClass* DialogueClass = FindObject<UClass>(nullptr, TEXT("/Script/NarrativeArsenal.Dialogue"));
	UClass* TDSClass = FindObject<UClass>(nullptr, TEXT("/Script/NarrativeArsenal.TaggedDialogueSet"));
	UClass* AbilityConfigClass = FindObject<UClass>(nullptr, TEXT("/Script/NarrativeArsenal.AbilityConfiguration"));
	UScriptStruct* LootTableRowStruct = FindObject<UScriptStruct>(nullptr, TEXT("/Script/NarrativeArsenal.LootTableRow"));

	// ========== PHASE 1: Create/Check all assets with v3.0 hash safety ==========
	AppendLog(TEXT("--- Phase 1: Checking Assets (v3.0 Hash Safety) ---"));

	// Asset 1: NPC Definition - NPC_{Name}.uasset
	{
		FString AssetName = FString::Printf(TEXT("NPC_%s"), *NPCName);
		FString PackagePath = FString::Printf(TEXT("%s/%s"), *BasePath, *AssetName);

		FString StatusReason;
		NPCDefStatus = CheckNPCAssetStatus(PackagePath, InputHash, StatusReason);

		switch (NPCDefStatus)
		{
			case ENPCAssetStatus::Create:
				if (NPCDefClass)
				{
					UPackage* Package = CreatePackage(*PackagePath);
					if (Package)
					{
						CreatedNPCDef = NewObject<UObject>(Package, NPCDefClass, *AssetName, RF_Public | RF_Standalone);
						if (CreatedNPCDef)
						{
							FAssetRegistryModule::AssetCreated(CreatedNPCDef);
							AppendLog(FString::Printf(TEXT("[CREATE] %s (UNPCDefinition)"), *AssetName));
							SuccessCount++;
						}
						else
						{
							AppendLog(FString::Printf(TEXT("[FAIL] %s - Could not create object"), *AssetName));
							FailCount++;
						}
					}
				}
				else
				{
					AppendLog(FString::Printf(TEXT("[FAIL] %s - UNPCDefinition class not found"), *AssetName));
					FailCount++;
				}
				break;

			case ENPCAssetStatus::Modify:
				CreatedNPCDef = LoadObject<UObject>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[MODIFY] %s - %s"), *AssetName, *StatusReason));
				ModifyCount++;
				break;

			case ENPCAssetStatus::Skip:
				CreatedNPCDef = LoadObject<UObject>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[SKIP] %s - %s"), *AssetName, *StatusReason));
				SkipCount++;
				break;

			case ENPCAssetStatus::Conflict:
				CreatedNPCDef = LoadObject<UObject>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[CONFLICT] %s - %s"), *AssetName, *StatusReason));
				ConflictCount++;
				break;
		}
	}

	// Asset 2: Main Dialogue - DBP_{Name}.uasset
	{
		FString AssetName = FString::Printf(TEXT("DBP_%s"), *NPCName);
		FString PackagePath = FString::Printf(TEXT("%s/%s"), *BasePath, *AssetName);

		FString StatusReason;
		DialogueStatus = CheckNPCAssetStatus(PackagePath, InputHash, StatusReason);

		switch (DialogueStatus)
		{
			case ENPCAssetStatus::Create:
				if (DialogueClass)
				{
					UPackage* Package = CreatePackage(*PackagePath);
					if (Package)
					{
						CreatedDialogue = FKismetEditorUtilities::CreateBlueprint(
							DialogueClass, Package, *AssetName, BPTYPE_Normal,
							UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
						if (CreatedDialogue)
						{
							FAssetRegistryModule::AssetCreated(CreatedDialogue);
							AppendLog(FString::Printf(TEXT("[CREATE] %s (UDialogue - Main)"), *AssetName));
							SuccessCount++;
						}
						else
						{
							AppendLog(FString::Printf(TEXT("[FAIL] %s - Could not create blueprint"), *AssetName));
							FailCount++;
						}
					}
				}
				else
				{
					AppendLog(FString::Printf(TEXT("[FAIL] %s - UDialogue class not found"), *AssetName));
					FailCount++;
				}
				break;

			case ENPCAssetStatus::Modify:
				CreatedDialogue = LoadObject<UBlueprint>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[MODIFY] %s - %s"), *AssetName, *StatusReason));
				ModifyCount++;
				break;

			case ENPCAssetStatus::Skip:
				CreatedDialogue = LoadObject<UBlueprint>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[SKIP] %s - %s"), *AssetName, *StatusReason));
				SkipCount++;
				break;

			case ENPCAssetStatus::Conflict:
				CreatedDialogue = LoadObject<UBlueprint>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[CONFLICT] %s - %s"), *AssetName, *StatusReason));
				ConflictCount++;
				break;
		}
	}

	// Asset 3: Tagged Dialogue Set - {Name}_TaggedDialogue.uasset
	{
		FString AssetName = FString::Printf(TEXT("%s_TaggedDialogue"), *NPCName);
		FString PackagePath = FString::Printf(TEXT("%s/%s"), *BasePath, *AssetName);

		FString StatusReason;
		TDSStatus = CheckNPCAssetStatus(PackagePath, InputHash, StatusReason);

		switch (TDSStatus)
		{
			case ENPCAssetStatus::Create:
				if (TDSClass)
				{
					UPackage* Package = CreatePackage(*PackagePath);
					if (Package)
					{
						CreatedTaggedDialogueSet = NewObject<UObject>(Package, TDSClass, *AssetName, RF_Public | RF_Standalone);
						if (CreatedTaggedDialogueSet)
						{
							FAssetRegistryModule::AssetCreated(CreatedTaggedDialogueSet);
							AppendLog(FString::Printf(TEXT("[CREATE] %s (UTaggedDialogueSet)"), *AssetName));
							SuccessCount++;
						}
						else
						{
							AppendLog(FString::Printf(TEXT("[FAIL] %s - Could not create object"), *AssetName));
							FailCount++;
						}
					}
				}
				else
				{
					AppendLog(FString::Printf(TEXT("[FAIL] %s - UTaggedDialogueSet class not found"), *AssetName));
					FailCount++;
				}
				break;

			case ENPCAssetStatus::Modify:
				CreatedTaggedDialogueSet = LoadObject<UObject>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[MODIFY] %s - %s"), *AssetName, *StatusReason));
				ModifyCount++;
				break;

			case ENPCAssetStatus::Skip:
				CreatedTaggedDialogueSet = LoadObject<UObject>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[SKIP] %s - %s"), *AssetName, *StatusReason));
				SkipCount++;
				break;

			case ENPCAssetStatus::Conflict:
				CreatedTaggedDialogueSet = LoadObject<UObject>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[CONFLICT] %s - %s"), *AssetName, *StatusReason));
				ConflictCount++;
				break;
		}
	}

	// Asset 4: Tagged Dialogue (Greetings) - DBP_{Name}_Greetings.uasset
	{
		FString AssetName = FString::Printf(TEXT("DBP_%s_Greetings"), *NPCName);
		FString PackagePath = FString::Printf(TEXT("%s/%s"), *BasePath, *AssetName);

		FString StatusReason;
		GreetingsStatus = CheckNPCAssetStatus(PackagePath, InputHash, StatusReason);

		switch (GreetingsStatus)
		{
			case ENPCAssetStatus::Create:
				if (DialogueClass)
				{
					UPackage* Package = CreatePackage(*PackagePath);
					if (Package)
					{
						CreatedGreetingsDialogue = FKismetEditorUtilities::CreateBlueprint(
							DialogueClass, Package, *AssetName, BPTYPE_Normal,
							UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
						if (CreatedGreetingsDialogue)
						{
							FAssetRegistryModule::AssetCreated(CreatedGreetingsDialogue);
							AppendLog(FString::Printf(TEXT("[CREATE] %s (UDialogue - Greeting)"), *AssetName));
							SuccessCount++;
						}
						else
						{
							AppendLog(FString::Printf(TEXT("[FAIL] %s - Could not create blueprint"), *AssetName));
							FailCount++;
						}
					}
				}
				else
				{
					AppendLog(FString::Printf(TEXT("[FAIL] %s - UDialogue class not found"), *AssetName));
					FailCount++;
				}
				break;

			case ENPCAssetStatus::Modify:
				CreatedGreetingsDialogue = LoadObject<UBlueprint>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[MODIFY] %s - %s"), *AssetName, *StatusReason));
				ModifyCount++;
				break;

			case ENPCAssetStatus::Skip:
				CreatedGreetingsDialogue = LoadObject<UBlueprint>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[SKIP] %s - %s"), *AssetName, *StatusReason));
				SkipCount++;
				break;

			case ENPCAssetStatus::Conflict:
				CreatedGreetingsDialogue = LoadObject<UBlueprint>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[CONFLICT] %s - %s"), *AssetName, *StatusReason));
				ConflictCount++;
				break;
		}
	}

	// Asset 5: Item Loadout DataTable - ItemLoadout_{Name}.uasset
	// Uses ItemCollectionsToGrant like Seth's example (IC_RifleWithAmmo, IC_ExampleArmorSet)
	{
		FString AssetName = FString::Printf(TEXT("ItemLoadout_%s"), *NPCName);
		FString PackagePath = FString::Printf(TEXT("%s/%s"), *BasePath, *AssetName);

		FString StatusReason;
		ItemLoadoutStatus = CheckNPCAssetStatus(PackagePath, InputHash, StatusReason);

		switch (ItemLoadoutStatus)
		{
			case ENPCAssetStatus::Create:
				if (LootTableRowStruct)
				{
					UPackage* Package = CreatePackage(*PackagePath);
					if (Package)
					{
						CreatedItemLoadout = NewObject<UDataTable>(Package, *AssetName, RF_Public | RF_Standalone);
						if (CreatedItemLoadout)
						{
							CreatedItemLoadout->RowStruct = LootTableRowStruct;

							// Add default row with Item Collections (like Seth's loadout)
							uint8* RowData = (uint8*)FMemory::Malloc(LootTableRowStruct->GetStructureSize());
							LootTableRowStruct->InitializeStruct(RowData);

							// Load Item Collections
							UObject* IC_RifleWithAmmo = LoadObject<UObject>(nullptr, TEXT("/NarrativePro/Pro/Demo/Items/Examples/Items/Weapons/IC_RifleWithAmmo.IC_RifleWithAmmo"));
							UObject* IC_ExampleArmorSet = LoadObject<UObject>(nullptr, TEXT("/NarrativePro/Pro/Demo/Items/Examples/Items/Clothing/IC_ExampleArmorSet.IC_ExampleArmorSet"));

							if (FProperty* ItemCollectionsProp = LootTableRowStruct->FindPropertyByName(TEXT("ItemCollectionsToGrant")))
							{
								FArrayProperty* ArrayProp = CastField<FArrayProperty>(ItemCollectionsProp);
								if (ArrayProp)
								{
									FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(RowData));
									int32 NumCollections = (IC_RifleWithAmmo ? 1 : 0) + (IC_ExampleArmorSet ? 1 : 0);
									if (NumCollections > 0)
									{
										ArrayHelper.Resize(NumCollections);
										FObjectProperty* InnerObjProp = CastField<FObjectProperty>(ArrayProp->Inner);
										if (InnerObjProp)
										{
											int32 Index = 0;
											if (IC_RifleWithAmmo) { InnerObjProp->SetObjectPropertyValue(ArrayHelper.GetRawPtr(Index++), IC_RifleWithAmmo); }
											if (IC_ExampleArmorSet) { InnerObjProp->SetObjectPropertyValue(ArrayHelper.GetRawPtr(Index++), IC_ExampleArmorSet); }
										}
									}
								}
							}

							if (FProperty* ChanceProp = LootTableRowStruct->FindPropertyByName(TEXT("Chance")))
							{
								float* ChancePtr = ChanceProp->ContainerPtrToValuePtr<float>(RowData);
								*ChancePtr = 1.0f;
							}

							CreatedItemLoadout->AddRow(FName(TEXT("NewRow")), *(FTableRowBase*)RowData);
							LootTableRowStruct->DestroyStruct(RowData);
							FMemory::Free(RowData);

							FAssetRegistryModule::AssetCreated(CreatedItemLoadout);
							AppendLog(FString::Printf(TEXT("[CREATE] %s (UDataTable - ItemLoadout)"), *AssetName));
							SuccessCount++;
						}
						else
						{
							AppendLog(FString::Printf(TEXT("[FAIL] %s - Could not create DataTable"), *AssetName));
							FailCount++;
						}
					}
				}
				else
				{
					AppendLog(FString::Printf(TEXT("[FAIL] %s - FLootTableRow struct not found"), *AssetName));
					FailCount++;
				}
				break;

			case ENPCAssetStatus::Modify:
				CreatedItemLoadout = LoadObject<UDataTable>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[MODIFY] %s - %s"), *AssetName, *StatusReason));
				ModifyCount++;
				break;

			case ENPCAssetStatus::Skip:
				CreatedItemLoadout = LoadObject<UDataTable>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[SKIP] %s - %s"), *AssetName, *StatusReason));
				SkipCount++;
				break;

			case ENPCAssetStatus::Conflict:
				CreatedItemLoadout = LoadObject<UDataTable>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[CONFLICT] %s - %s"), *AssetName, *StatusReason));
				ConflictCount++;
				break;
		}
	}

	// Asset 6: Sound Wave placeholder
	AppendLog(FString::Printf(TEXT("[INFO] %s_Voice (USoundWave) - Import audio manually"), *NPCName));

	// Asset 7: Level Sequence placeholder
	AppendLog(FString::Printf(TEXT("[INFO] LS_%s_Greet (ULevelSequence) - Create in Sequencer"), *NPCName));

	// Asset 8: Cinematic Trigger Blueprint - BP_Trigger_Cinematic_{Name}.uasset
	{
		FString AssetName = FString::Printf(TEXT("BP_Trigger_Cinematic_%s"), *NPCName);
		FString PackagePath = FString::Printf(TEXT("%s/%s"), *BasePath, *AssetName);

		FString StatusReason;
		TriggerBPStatus = CheckNPCAssetStatus(PackagePath, InputHash, StatusReason);

		switch (TriggerBPStatus)
		{
			case ENPCAssetStatus::Create:
			{
				UPackage* Package = CreatePackage(*PackagePath);
				if (Package)
				{
					CreatedTriggerBP = FKismetEditorUtilities::CreateBlueprint(
						AActor::StaticClass(), Package, *AssetName, BPTYPE_Normal,
						UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
					if (CreatedTriggerBP)
					{
						FAssetRegistryModule::AssetCreated(CreatedTriggerBP);
						AppendLog(FString::Printf(TEXT("[CREATE] %s (AActor Blueprint - Trigger)"), *AssetName));
						SuccessCount++;
					}
					else
					{
						AppendLog(FString::Printf(TEXT("[FAIL] %s - Could not create blueprint"), *AssetName));
						FailCount++;
					}
				}
				break;
			}

			case ENPCAssetStatus::Modify:
				CreatedTriggerBP = LoadObject<UBlueprint>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[MODIFY] %s - %s"), *AssetName, *StatusReason));
				ModifyCount++;
				break;

			case ENPCAssetStatus::Skip:
				CreatedTriggerBP = LoadObject<UBlueprint>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[SKIP] %s - %s"), *AssetName, *StatusReason));
				SkipCount++;
				break;

			case ENPCAssetStatus::Conflict:
				CreatedTriggerBP = LoadObject<UBlueprint>(nullptr, *PackagePath);
				AppendLog(FString::Printf(TEXT("[CONFLICT] %s - %s"), *AssetName, *StatusReason));
				ConflictCount++;
				break;
		}
	}

	// ========== PHASE 2: Link assets together (only for CREATE or MODIFY) ==========
	AppendLog(TEXT(""));
	AppendLog(TEXT("--- Phase 2: Linking Assets ---"));

	// v3.7: Only link NPC Definition if status is CREATE or MODIFY
	bool bShouldLinkNPCDef = (NPCDefStatus == ENPCAssetStatus::Create || NPCDefStatus == ENPCAssetStatus::Modify);
	bool bShouldLinkTDS = (TDSStatus == ENPCAssetStatus::Create || TDSStatus == ENPCAssetStatus::Modify);

	if (bShouldLinkNPCDef && CreatedNPCDef && NPCDefClass)
	{
		// Set NPCID
		if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("NPCID")))
		{
			FName* Ptr = Prop->ContainerPtrToValuePtr<FName>(CreatedNPCDef);
			*Ptr = FName(*NPCName);
			AppendLog(FString::Printf(TEXT("[LINK] NPCID = %s"), *NPCName));
		}

		// Set NPCName (display name)
		if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("NPCName")))
		{
			FText* Ptr = Prop->ContainerPtrToValuePtr<FText>(CreatedNPCDef);
			*Ptr = FText::FromString(NPCName);
			AppendLog(FString::Printf(TEXT("[LINK] NPCName = %s"), *NPCName));
		}

		// Set Dialogue -> DBP_{Name} (TSoftClassPtr)
		if (CreatedDialogue && CreatedDialogue->GeneratedClass)
		{
			if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("Dialogue")))
			{
				FSoftClassProperty* SoftClassProp = CastField<FSoftClassProperty>(Prop);
				if (SoftClassProp)
				{
					FSoftObjectPtr* Ptr = SoftClassProp->GetPropertyValuePtr_InContainer(CreatedNPCDef);
					*Ptr = FSoftObjectPtr(CreatedDialogue->GeneratedClass);
					AppendLog(FString::Printf(TEXT("[LINK] Dialogue -> DBP_%s"), *NPCName));
				}
			}
		}

		// Set TaggedDialogueSet -> {Name}_TaggedDialogue (TSoftObjectPtr)
		if (CreatedTaggedDialogueSet)
		{
			if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("TaggedDialogueSet")))
			{
				FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Prop);
				if (SoftObjProp)
				{
					FSoftObjectPtr* Ptr = SoftObjProp->GetPropertyValuePtr_InContainer(CreatedNPCDef);
					*Ptr = FSoftObjectPtr(CreatedTaggedDialogueSet);
					AppendLog(FString::Printf(TEXT("[LINK] TaggedDialogueSet -> %s_TaggedDialogue"), *NPCName));
				}
			}
		}

		// Set AbilityConfiguration -> AC_NPC_Default (shared default)
		UObject* DefaultAbilityConfig = LoadObject<UObject>(nullptr, TEXT("/NarrativePro/Pro/Core/Abilities/Configurations/AC_NPC_Default.AC_NPC_Default"));
		if (DefaultAbilityConfig)
		{
			if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("AbilityConfiguration")))
			{
				FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop);
				if (ObjProp)
				{
					ObjProp->SetObjectPropertyValue_InContainer(CreatedNPCDef, DefaultAbilityConfig);
					AppendLog(TEXT("[LINK] AbilityConfiguration -> AC_NPC_Default"));
				}
			}
		}
		else
		{
			AppendLog(TEXT("[WARN] AC_NPC_Default not found - AbilityConfiguration not set"));
		}

		// Set DefaultItemLoadout to reference ItemLoadout_{Name} DataTable (like Seth)
		// DefaultItemLoadout is TArray<FLootTableRoll> from CharacterDefinition - uses TableToRoll property
		if (CreatedItemLoadout)
		{
			if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("DefaultItemLoadout")))
			{
				FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop);
				if (ArrayProp)
				{
					FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(CreatedNPCDef));
					ArrayHelper.Resize(1);

					// Get the inner struct type (FLootTableRoll)
					FStructProperty* InnerStructProp = CastField<FStructProperty>(ArrayProp->Inner);
					if (InnerStructProp)
					{
						void* ElementPtr = ArrayHelper.GetRawPtr(0);
						InnerStructProp->Struct->InitializeStruct(ElementPtr);

						// Set TableToRoll to reference the ItemLoadout DataTable (like Seth)
						if (FProperty* TableProp = InnerStructProp->Struct->FindPropertyByName(TEXT("TableToRoll")))
						{
							FObjectProperty* ObjProp = CastField<FObjectProperty>(TableProp);
							if (ObjProp)
							{
								ObjProp->SetObjectPropertyValue(TableProp->ContainerPtrToValuePtr<void>(ElementPtr), CreatedItemLoadout);
								AppendLog(FString::Printf(TEXT("[LINK] DefaultItemLoadout[0].TableToRoll -> ItemLoadout_%s"), *NPCName));
							}
						}

						// Set NumRolls = 1 (like Seth)
						if (FProperty* NumRollsProp = InnerStructProp->Struct->FindPropertyByName(TEXT("NumRolls")))
						{
							int32* NumRollsPtr = NumRollsProp->ContainerPtrToValuePtr<int32>(ElementPtr);
							*NumRollsPtr = 1;
							AppendLog(TEXT("[LINK] DefaultItemLoadout[0].NumRolls = 1"));
						}

						// Set Chance = 1.0 (like Seth)
						if (FProperty* ChanceProp = InnerStructProp->Struct->FindPropertyByName(TEXT("Chance")))
						{
							float* ChancePtr = ChanceProp->ContainerPtrToValuePtr<float>(ElementPtr);
							*ChancePtr = 1.0f;
							AppendLog(TEXT("[LINK] DefaultItemLoadout[0].Chance = 1.0"));
						}
					}
				}
				else
				{
					AppendLog(TEXT("[WARN] DefaultItemLoadout is not an array property"));
				}
			}
			else
			{
				AppendLog(TEXT("[WARN] DefaultItemLoadout property not found on NPCDefinition"));
			}
		}
		else
		{
			AppendLog(TEXT("[WARN] ItemLoadout DataTable not available for DefaultItemLoadout"));
		}

		// Set DefaultOwnedTags (like Seth: Narrative.State.Invulnerable)
		if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("DefaultOwnedTags")))
		{
			FStructProperty* StructProp = CastField<FStructProperty>(Prop);
			if (StructProp && StructProp->Struct == FGameplayTagContainer::StaticStruct())
			{
				FGameplayTagContainer* TagContainer = StructProp->ContainerPtrToValuePtr<FGameplayTagContainer>(CreatedNPCDef);
				TagContainer->AddTag(FGameplayTag::RequestGameplayTag(FName("Narrative.State.Invulnerable")));
				AppendLog(TEXT("[LINK] DefaultOwnedTags = Narrative.State.Invulnerable"));
			}
		}

		// Set DefaultFactions (like Seth: Narrative.Factions.Heroes)
		if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("DefaultFactions")))
		{
			FStructProperty* StructProp = CastField<FStructProperty>(Prop);
			if (StructProp && StructProp->Struct == FGameplayTagContainer::StaticStruct())
			{
				FGameplayTagContainer* TagContainer = StructProp->ContainerPtrToValuePtr<FGameplayTagContainer>(CreatedNPCDef);
				TagContainer->AddTag(FGameplayTag::RequestGameplayTag(FName("Narrative.Factions.Heroes")));
				AppendLog(TEXT("[LINK] DefaultFactions = Narrative.Factions.Heroes"));
			}
		}

		// Set ActivityConfiguration -> AC_RunAndGun (like Seth) - TSoftObjectPtr
		UObject* DefaultActivityConfig = LoadObject<UObject>(nullptr, TEXT("/NarrativePro/Pro/Core/AI/Configs/AC_RunAndGun.AC_RunAndGun"));
		if (DefaultActivityConfig)
		{
			if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("ActivityConfiguration")))
			{
				FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Prop);
				if (SoftObjProp)
				{
					FSoftObjectPtr* Ptr = SoftObjProp->GetPropertyValuePtr_InContainer(CreatedNPCDef);
					*Ptr = FSoftObjectPtr(DefaultActivityConfig);
					AppendLog(TEXT("[LINK] ActivityConfiguration -> AC_RunAndGun"));
				}
				else
				{
					AppendLog(TEXT("[WARN] ActivityConfiguration is not FSoftObjectProperty"));
				}
			}
		}
		else
		{
			AppendLog(TEXT("[WARN] AC_RunAndGun not found - ActivityConfiguration not set"));
		}

		// Set DefaultAppearance -> Apperance_Manny (like Seth)
		UObject* DefaultAppearance = LoadObject<UObject>(nullptr, TEXT("/NarrativePro/Pro/Core/Character/Biped/Appearances/Mannequin/Apperance_Manny.Apperance_Manny"));
		if (DefaultAppearance)
		{
			if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("DefaultAppearance")))
			{
				FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Prop);
				if (SoftObjProp)
				{
					FSoftObjectPtr* Ptr = SoftObjProp->GetPropertyValuePtr_InContainer(CreatedNPCDef);
					*Ptr = FSoftObjectPtr(DefaultAppearance);
					AppendLog(TEXT("[LINK] DefaultAppearance -> Apperance_Manny"));
				}
			}
		}
		else
		{
			AppendLog(TEXT("[WARN] Apperance_Manny not found - DefaultAppearance not set"));
		}

		// Set NPCClassPath -> BP_NarrativeNPCCharacter (default NPC blueprint like Seth)
		UClass* DefaultNPCClass = LoadClass<AActor>(nullptr, TEXT("/NarrativePro/Pro/Core/Character/Biped/BP_NarrativeNPCCharacter.BP_NarrativeNPCCharacter_C"));
		if (DefaultNPCClass)
		{
			if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("NPCClassPath")))
			{
				FSoftClassProperty* SoftClassProp = CastField<FSoftClassProperty>(Prop);
				if (SoftClassProp)
				{
					FSoftObjectPtr* Ptr = SoftClassProp->GetPropertyValuePtr_InContainer(CreatedNPCDef);
					*Ptr = FSoftObjectPtr(DefaultNPCClass);
					AppendLog(TEXT("[LINK] NPCClassPath -> BP_NarrativeNPCCharacter"));
				}
			}
		}
		else
		{
			AppendLog(TEXT("[WARN] BP_NarrativeNPCCharacter not found - NPCClassPath not set"));
		}

		// Set MinLevel = 1 (like Seth)
		if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("MinLevel")))
		{
			int32* Ptr = Prop->ContainerPtrToValuePtr<int32>(CreatedNPCDef);
			*Ptr = 1;
			AppendLog(TEXT("[LINK] MinLevel = 1"));
		}

		// Set MaxLevel = 100 (like Seth)
		if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("MaxLevel")))
		{
			int32* Ptr = Prop->ContainerPtrToValuePtr<int32>(CreatedNPCDef);
			*Ptr = 100;
			AppendLog(TEXT("[LINK] MaxLevel = 100"));
		}

		// Set bAllowMultipleInstances = false (unique NPC like Seth)
		if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("bAllowMultipleInstances")))
		{
			bool* Ptr = Prop->ContainerPtrToValuePtr<bool>(CreatedNPCDef);
			*Ptr = false;
			AppendLog(TEXT("[LINK] bAllowMultipleInstances = false"));
		}

		// Set bIsVendor = false (not a vendor by default)
		if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("bIsVendor")))
		{
			bool* Ptr = Prop->ContainerPtrToValuePtr<bool>(CreatedNPCDef);
			*Ptr = false;
			AppendLog(TEXT("[LINK] bIsVendor = false"));
		}

		// Set TradingCurrency = 0 (no currency for non-vendor)
		if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("TradingCurrency")))
		{
			int32* Ptr = Prop->ContainerPtrToValuePtr<int32>(CreatedNPCDef);
			*Ptr = 0;
			AppendLog(TEXT("[LINK] TradingCurrency = 0"));
		}

		// Set BuyItemPercentage = 0.0 (like Seth - default non-vendor)
		if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("BuyItemPercentage")))
		{
			float* Ptr = Prop->ContainerPtrToValuePtr<float>(CreatedNPCDef);
			*Ptr = 0.0f;
			AppendLog(TEXT("[LINK] BuyItemPercentage = 0.0"));
		}

		// Set SellItemPercentage = 0.0 (like Seth - default non-vendor)
		if (FProperty* Prop = NPCDefClass->FindPropertyByName(TEXT("SellItemPercentage")))
		{
			float* Ptr = Prop->ContainerPtrToValuePtr<float>(CreatedNPCDef);
			*Ptr = 0.0f;
			AppendLog(TEXT("[LINK] SellItemPercentage = 0.0"));
		}

		// Mark NPC Definition dirty and save
		CreatedNPCDef->GetOutermost()->MarkPackageDirty();
	}

	// Link TaggedDialogueSet -> Add Greetings dialogue to its Dialogues array (only for CREATE or MODIFY)
	if (bShouldLinkTDS && CreatedTaggedDialogueSet && CreatedGreetingsDialogue && TDSClass)
	{
		if (FProperty* Prop = TDSClass->FindPropertyByName(TEXT("Dialogues")))
		{
			FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop);
			if (ArrayProp)
			{
				FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(CreatedTaggedDialogueSet));

				// Get the inner struct (FTaggedDialogueEntry)
				FStructProperty* InnerStructProp = CastField<FStructProperty>(ArrayProp->Inner);
				if (InnerStructProp)
				{
					ArrayHelper.Resize(1);
					void* ElementPtr = ArrayHelper.GetRawPtr(0);

					// Set DialogueClass in the entry
					if (FProperty* DialogueClassProp = InnerStructProp->Struct->FindPropertyByName(TEXT("DialogueClass")))
					{
						FSoftClassProperty* SoftClassProp = CastField<FSoftClassProperty>(DialogueClassProp);
						if (SoftClassProp && CreatedGreetingsDialogue->GeneratedClass)
						{
							FSoftObjectPtr* Ptr = SoftClassProp->GetPropertyValuePtr_InContainer(ElementPtr);
							*Ptr = FSoftObjectPtr(CreatedGreetingsDialogue->GeneratedClass);
							AppendLog(FString::Printf(TEXT("[LINK] TaggedDialogueSet.Dialogues += DBP_%s_Greetings"), *NPCName));
						}
					}
				}
			}
		}
		CreatedTaggedDialogueSet->GetOutermost()->MarkPackageDirty();
	}

	// ========== PHASE 3: Save all assets and store metadata ==========
	AppendLog(TEXT(""));
	AppendLog(TEXT("--- Phase 3: Saving Assets ---"));

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;

	// v3.7: Lambda to save asset and store metadata
	auto SaveAssetWithMetadata = [&](UObject* Asset, const FString& AssetName, const FString& GeneratorId, ENPCAssetStatus Status)
	{
		if (Asset && (Status == ENPCAssetStatus::Create || Status == ENPCAssetStatus::Modify))
		{
			UPackage* Package = Asset->GetOutermost();
			FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
			if (UPackage::SavePackage(Package, Asset, *PackageFileName, SaveArgs))
			{
				// Store metadata for v3.0 hash safety
				StoreNPCAssetMetadata(Asset, GeneratorId, AssetName, InputHash);
				AppendLog(FString::Printf(TEXT("[SAVED] %s"), *AssetName));
			}
		}
	};

	SaveAssetWithMetadata(CreatedNPCDef, FString::Printf(TEXT("NPC_%s"), *NPCName), TEXT("NPC_UI"), NPCDefStatus);
	SaveAssetWithMetadata(CreatedDialogue, FString::Printf(TEXT("DBP_%s"), *NPCName), TEXT("DBP_UI"), DialogueStatus);
	SaveAssetWithMetadata(CreatedTaggedDialogueSet, FString::Printf(TEXT("%s_TaggedDialogue"), *NPCName), TEXT("TDS_UI"), TDSStatus);
	SaveAssetWithMetadata(CreatedGreetingsDialogue, FString::Printf(TEXT("DBP_%s_Greetings"), *NPCName), TEXT("DBP_UI"), GreetingsStatus);
	SaveAssetWithMetadata(CreatedItemLoadout, FString::Printf(TEXT("ItemLoadout_%s"), *NPCName), TEXT("DT_UI"), ItemLoadoutStatus);
	SaveAssetWithMetadata(CreatedTriggerBP, FString::Printf(TEXT("BP_Trigger_Cinematic_%s"), *NPCName), TEXT("BP_UI"), TriggerBPStatus);

	AppendLog(TEXT(""));
	AppendLog(TEXT("----------------------------------------------"));
	AppendLog(FString::Printf(TEXT("NPC Creation Complete: %d created, %d modified, %d skipped, %d conflicts, %d failed"),
		SuccessCount, ModifyCount, SkipCount, ConflictCount, FailCount));
	AppendLog(FString::Printf(TEXT("Assets in: %s"), *BasePath));
	AppendLog(TEXT("----------------------------------------------"));

	UpdateStatus(FString::Printf(TEXT("NPC '%s' - %d created, %d modified, %d skipped"), *NPCName, SuccessCount, ModifyCount, SkipCount));
}

#undef LOCTEXT_NAMESPACE
