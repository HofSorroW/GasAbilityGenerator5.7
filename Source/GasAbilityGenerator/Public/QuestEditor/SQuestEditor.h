// SQuestEditor.h
// Quest Editor Slate Widget - Visual editor for creating and testing Quest assets
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "GasAbilityGeneratorTypes.h"

class SEditableTextBox;
class SMultiLineEditableTextBox;
class SCheckBox;

/**
 * Tree item types for quest structure visualization
 */
enum class EQuestTreeItemType : uint8
{
	Quest,      // Root quest node
	State,      // Quest state
	Branch,     // Branch within a state
	Task        // Task within a branch
};

/**
 * Tree item for quest structure visualization
 */
struct FQuestTreeItem : TSharedFromThis<FQuestTreeItem>
{
	EQuestTreeItemType Type;
	FString Id;
	FString DisplayText;
	FString StateType;  // For states: regular/success/failure
	int32 StateIndex = -1;
	int32 BranchIndex = -1;
	int32 TaskIndex = -1;

	TArray<TSharedPtr<FQuestTreeItem>> Children;
	TWeakPtr<FQuestTreeItem> Parent;

	FQuestTreeItem(EQuestTreeItemType InType, const FString& InId, const FString& InText)
		: Type(InType), Id(InId), DisplayText(InText) {}
};

/**
 * Main Quest Editor Widget
 * Visual editor for creating quest state machines
 */
class GASABILITYGENERATOR_API SQuestEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SQuestEditor) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	//=========================================================================
	// Quest Data
	//=========================================================================

	/** Current quest definition being edited */
	FManifestQuestDefinition QuestDefinition;

	/** Tree items for visualization */
	TArray<TSharedPtr<FQuestTreeItem>> RootItems;

	/** Currently selected tree item */
	TSharedPtr<FQuestTreeItem> SelectedItem;

	//=========================================================================
	// UI Widgets
	//=========================================================================

	/** Quest name text box */
	TSharedPtr<SEditableTextBox> QuestNameBox;

	/** Quest display name text box */
	TSharedPtr<SEditableTextBox> DisplayNameBox;

	/** Quest description text box */
	TSharedPtr<SMultiLineEditableTextBox> DescriptionBox;

	/** Is tracked checkbox */
	TSharedPtr<SCheckBox> TrackedCheckbox;

	/** Tree view widget */
	TSharedPtr<STreeView<TSharedPtr<FQuestTreeItem>>> TreeView;

	/** Output log */
	TSharedPtr<SMultiLineEditableTextBox> OutputLog;

	/** Details panel content */
	TSharedPtr<SVerticalBox> DetailsPanel;

	//=========================================================================
	// UI Construction
	//=========================================================================

	/** Build the toolbar */
	TSharedRef<SWidget> BuildToolbar();

	/** Build the quest properties panel */
	TSharedRef<SWidget> BuildPropertiesPanel();

	/** Build the tree view panel */
	TSharedRef<SWidget> BuildTreePanel();

	/** Build the details panel (right side) */
	TSharedRef<SWidget> BuildDetailsPanel();

	/** Build state details */
	void BuildStateDetails(int32 StateIndex);

	/** Build branch details */
	void BuildBranchDetails(int32 StateIndex, int32 BranchIndex);

	/** Build task details */
	void BuildTaskDetails(int32 StateIndex, int32 BranchIndex, int32 TaskIndex);

	//=========================================================================
	// Tree View Callbacks
	//=========================================================================

	/** Generate row for tree view */
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FQuestTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/** Get children for tree item */
	void OnGetChildren(TSharedPtr<FQuestTreeItem> Item, TArray<TSharedPtr<FQuestTreeItem>>& OutChildren);

	/** Selection changed */
	void OnSelectionChanged(TSharedPtr<FQuestTreeItem> Item, ESelectInfo::Type SelectInfo);

	//=========================================================================
	// Actions
	//=========================================================================

	/** Add new state */
	FReply OnAddStateClicked();

	/** Add new branch to selected state */
	FReply OnAddBranchClicked();

	/** Add new task to selected branch */
	FReply OnAddTaskClicked();

	/** Delete selected item */
	FReply OnDeleteClicked();

	/** Generate quest asset */
	FReply OnGenerateClicked();

	/** Load from YAML */
	FReply OnLoadYAMLClicked();

	/** Save to YAML */
	FReply OnSaveYAMLClicked();

	/** Clear quest */
	FReply OnClearClicked();

	//=========================================================================
	// Tree Management
	//=========================================================================

	/** Rebuild tree from quest definition */
	void RebuildTree();

	/** Log message to output */
	void LogMessage(const FString& Message);

	/** Get state type color */
	FSlateColor GetStateTypeColor(const FString& StateType) const;

	/** Update details panel based on selection */
	void UpdateDetailsPanel();
};

/**
 * Tab/Window that hosts the Quest Editor
 */
class GASABILITYGENERATOR_API SQuestEditorWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SQuestEditorWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	/** The quest editor widget */
	TSharedPtr<SQuestEditor> QuestEditor;
};
