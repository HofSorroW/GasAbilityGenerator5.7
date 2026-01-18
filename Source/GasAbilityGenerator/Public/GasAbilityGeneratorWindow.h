// GasAbilityGenerator v3.7
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v3.7: Added NPC Creation feature - one-click NPC asset generation
// v3.0: Added Dry Run and Force checkboxes for metadata-aware regeneration

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Locked/GasAbilityGeneratorTypes.h"

class SMultiLineEditableTextBox;
class SEditableTextBox;
class SButton;
class STextBlock;
class SCheckBox;

/**
 * Main UI Window for GAS Ability Generator
 * v3.7: Added NPC Creation feature - one-click NPC asset generation
 * v3.0: Added Dry Run and Force checkboxes for metadata-aware regeneration
 * v2.5.0: Renamed to GasAbilityGenerator for generic UE project compatibility
 * v2.1.8: Added enumeration generation before blueprints to fix enum variable types
 * v2.0.9: Implements generation guard to prevent duplicate passes
 */
class GASABILITYGENERATOR_API SGasAbilityGeneratorWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGasAbilityGeneratorWindow) {}
	SLATE_END_ARGS()

	/** Constructs the widget */
	void Construct(const FArguments& InArgs);

	/** Destructor */
	virtual ~SGasAbilityGeneratorWindow();

private:
	// UI Elements
	TSharedPtr<SEditableTextBox> GuidesPathTextBox;
	TSharedPtr<SMultiLineEditableTextBox> LogTextBox;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SButton> GenerateTagsButton;
	TSharedPtr<SButton> GenerateAssetsButton;
	TSharedPtr<SButton> CopyLogButton;

	// v3.0: Dry Run / Force mode checkboxes
	TSharedPtr<SCheckBox> DryRunCheckbox;
	TSharedPtr<SCheckBox> ForceCheckbox;

	// v3.7: NPC Creation UI elements
	TSharedPtr<SEditableTextBox> NPCNameTextBox;
	TSharedPtr<SButton> CreateNPCButton;

	// State
	FString GuidesFolderPath;
	FManifestData ManifestData;

	/** v2.0.9 FIX: Guard flag to prevent duplicate generation passes */
	bool bIsGenerating;

	// Button Handlers
	FReply OnBrowseClicked();
	FReply OnGenerateTagsClicked();
	FReply OnGenerateAssetsClicked();
	FReply OnSettingsClicked();
	FReply OnCopyLogClicked();
	FReply OnClearLogClicked();

	// v3.7: NPC Creation handlers
	FReply OnCreateNPCClicked();
	void OnNPCNameChanged(const FText& NewText);
	bool IsCreateNPCButtonEnabled() const;

	// Helper Functions
	void LoadConfig();
	void SaveConfig();
	bool LoadManifest();
	void ParseManifest(const FString& ManifestContent);
	void AppendLog(const FString& Message);
	void ClearLog();
	void UpdateStatus(const FString& Status);
	void ShowResultsDialog(const FGenerationSummary& Summary);
	void ShowDryRunResultsDialog(const FDryRunSummary& DryRunSummary);

	// Generation Functions
	void GenerateTags();
	void GenerateAssets();

	/** Run the full generation pipeline with guard check */
	void RunGeneration();

	// v3.7: NPC Creation Functions
	/** Creates all NPC assets (8 types) in Content/NPC/{NPCName}/ folder */
	void CreateNPCAssets(const FString& NPCName);
};
