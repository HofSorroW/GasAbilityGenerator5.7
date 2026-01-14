// SQuestEditor.cpp
// Quest Editor Implementation
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#include "QuestEditor/SQuestEditor.h"
#include "GasAbilityGeneratorGenerators.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Views/STableRow.h"
#include "Styling/AppStyle.h"
#include "DesktopPlatformModule.h"
#include "Misc/FileHelper.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformApplicationMisc.h"

#define LOCTEXT_NAMESPACE "QuestEditor"

//=============================================================================
// SQuestEditor
//=============================================================================

void SQuestEditor::Construct(const FArguments& InArgs)
{
	// Initialize with a default quest
	QuestDefinition.Name = TEXT("Quest_New");
	QuestDefinition.QuestName = TEXT("New Quest");
	QuestDefinition.QuestDescription = TEXT("Quest description");
	QuestDefinition.bTracked = true;
	QuestDefinition.Folder = TEXT("Quests");

	// Add a default start state
	FManifestQuestStateDefinition StartState;
	StartState.Id = TEXT("Start");
	StartState.Description = TEXT("Quest started");
	StartState.Type = TEXT("regular");
	QuestDefinition.States.Add(StartState);

	// Build UI
	ChildSlot
	[
		SNew(SVerticalBox)

		// Toolbar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			BuildToolbar()
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
		]

		// Main content area
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(5)
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)

			// Left panel - Properties + Tree
			+ SSplitter::Slot()
			.Value(0.35f)
			[
				SNew(SVerticalBox)

				// Quest Properties
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					BuildPropertiesPanel()
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 5)
				[
					SNew(SSeparator)
				]

				// Tree View
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					BuildTreePanel()
				]
			]

			// Right panel - Details
			+ SSplitter::Slot()
			.Value(0.65f)
			[
				SNew(SVerticalBox)

				// Details Panel
				+ SVerticalBox::Slot()
				.FillHeight(0.7f)
				[
					BuildDetailsPanel()
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 5)
				[
					SNew(SSeparator)
				]

				// Output Log
				+ SVerticalBox::Slot()
				.FillHeight(0.3f)
				[
					SNew(SExpandableArea)
					.AreaTitle(LOCTEXT("OutputLog", "Output Log"))
					.InitiallyCollapsed(false)
					.BodyContent()
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(OutputLog, SMultiLineEditableTextBox)
							.IsReadOnly(true)
							.BackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f))
						]
					]
				]
			]
		]
	];

	// Build initial tree
	RebuildTree();
	LogMessage(TEXT("Quest Editor initialized. Create states, branches, and tasks to build your quest."));
}

TSharedRef<SWidget> SQuestEditor::BuildToolbar()
{
	return SNew(SHorizontalBox)

		// Title
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(5, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Title", "Quest Editor"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNullWidget::NullWidget
		]

		// Add State
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("AddState", "+ State"))
			.ToolTipText(LOCTEXT("AddStateTooltip", "Add a new state to the quest"))
			.OnClicked(this, &SQuestEditor::OnAddStateClicked)
		]

		// Add Branch
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("AddBranch", "+ Branch"))
			.ToolTipText(LOCTEXT("AddBranchTooltip", "Add a branch to the selected state"))
			.OnClicked(this, &SQuestEditor::OnAddBranchClicked)
		]

		// Add Task
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("AddTask", "+ Task"))
			.ToolTipText(LOCTEXT("AddTaskTooltip", "Add a task to the selected branch"))
			.OnClicked(this, &SQuestEditor::OnAddTaskClicked)
		]

		// Delete
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("Delete", "Delete"))
			.ToolTipText(LOCTEXT("DeleteTooltip", "Delete the selected item"))
			.OnClicked(this, &SQuestEditor::OnDeleteClicked)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(10, 0, 2, 0)
		[
			SNew(SSeparator)
			.Orientation(Orient_Vertical)
		]

		// Generate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("Generate", "Generate Quest"))
			.ToolTipText(LOCTEXT("GenerateTooltip", "Generate the UQuestBlueprint asset"))
			.OnClicked(this, &SQuestEditor::OnGenerateClicked)
		]

		// Clear
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("Clear", "Clear"))
			.ToolTipText(LOCTEXT("ClearTooltip", "Clear the quest and start fresh"))
			.OnClicked(this, &SQuestEditor::OnClearClicked)
		];
}

TSharedRef<SWidget> SQuestEditor::BuildPropertiesPanel()
{
	return SNew(SExpandableArea)
		.AreaTitle(LOCTEXT("QuestProperties", "Quest Properties"))
		.InitiallyCollapsed(false)
		.BodyContent()
		[
			SNew(SVerticalBox)

			// Quest Asset Name
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5, 2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 5, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AssetName", "Asset Name:"))
					.MinDesiredWidth(80)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SAssignNew(QuestNameBox, SEditableTextBox)
					.Text(FText::FromString(QuestDefinition.Name))
					.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type) {
						QuestDefinition.Name = Text.ToString();
					})
				]
			]

			// Quest Display Name
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5, 2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 5, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DisplayName", "Display Name:"))
					.MinDesiredWidth(80)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SAssignNew(DisplayNameBox, SEditableTextBox)
					.Text(FText::FromString(QuestDefinition.QuestName))
					.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type) {
						QuestDefinition.QuestName = Text.ToString();
					})
				]
			]

			// Description
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5, 2)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Description", "Description:"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 2)
				[
					SNew(SBox)
					.MinDesiredHeight(50)
					[
						SAssignNew(DescriptionBox, SMultiLineEditableTextBox)
						.Text(FText::FromString(QuestDefinition.QuestDescription))
						.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type) {
							QuestDefinition.QuestDescription = Text.ToString();
						})
					]
				]
			]

			// Tracked checkbox
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5, 2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SAssignNew(TrackedCheckbox, SCheckBox)
					.IsChecked(QuestDefinition.bTracked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) {
						QuestDefinition.bTracked = (NewState == ECheckBoxState::Checked);
					})
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(5, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Tracked", "Tracked (show navigation markers)"))
				]
			]

			// Folder
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5, 2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 5, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Folder", "Folder:"))
					.MinDesiredWidth(80)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SEditableTextBox)
					.Text(FText::FromString(QuestDefinition.Folder))
					.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type) {
						QuestDefinition.Folder = Text.ToString();
					})
				]
			]
		];
}

TSharedRef<SWidget> SQuestEditor::BuildTreePanel()
{
	return SNew(SExpandableArea)
		.AreaTitle(LOCTEXT("QuestStructure", "Quest Structure"))
		.InitiallyCollapsed(false)
		.BodyContent()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(2)
			[
				SAssignNew(TreeView, STreeView<TSharedPtr<FQuestTreeItem>>)
				.TreeItemsSource(&RootItems)
				.OnGenerateRow(this, &SQuestEditor::OnGenerateRow)
				.OnGetChildren(this, &SQuestEditor::OnGetChildren)
				.OnSelectionChanged(this, &SQuestEditor::OnSelectionChanged)
				.SelectionMode(ESelectionMode::Single)
			]
		];
}

TSharedRef<SWidget> SQuestEditor::BuildDetailsPanel()
{
	return SNew(SExpandableArea)
		.AreaTitle(LOCTEXT("Details", "Details"))
		.InitiallyCollapsed(false)
		.BodyContent()
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(DetailsPanel, SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SelectItem", "Select a state, branch, or task to edit its properties."))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)))
				]
			]
		];
}

TSharedRef<ITableRow> SQuestEditor::OnGenerateRow(TSharedPtr<FQuestTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	FText ItemText;
	FSlateColor TextColor = FSlateColor(FLinearColor::White);

	switch (Item->Type)
	{
	case EQuestTreeItemType::Quest:
		ItemText = FText::Format(LOCTEXT("QuestItem", "[Quest] {0}"), FText::FromString(Item->DisplayText));
		TextColor = FSlateColor(FLinearColor(0.2f, 0.6f, 1.0f));
		break;

	case EQuestTreeItemType::State:
		ItemText = FText::Format(LOCTEXT("StateItem", "[{0}] {1}"),
			FText::FromString(Item->StateType.ToUpper()),
			FText::FromString(Item->DisplayText));
		TextColor = GetStateTypeColor(Item->StateType);
		break;

	case EQuestTreeItemType::Branch:
		ItemText = FText::Format(LOCTEXT("BranchItem", "-> {0}"), FText::FromString(Item->DisplayText));
		TextColor = FSlateColor(FLinearColor(0.8f, 0.8f, 0.2f));
		break;

	case EQuestTreeItemType::Task:
		ItemText = FText::Format(LOCTEXT("TaskItem", "  * {0}"), FText::FromString(Item->DisplayText));
		TextColor = FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f));
		break;
	}

	return SNew(STableRow<TSharedPtr<FQuestTreeItem>>, OwnerTable)
		[
			SNew(STextBlock)
			.Text(ItemText)
			.ColorAndOpacity(TextColor)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
		];
}

void SQuestEditor::OnGetChildren(TSharedPtr<FQuestTreeItem> Item, TArray<TSharedPtr<FQuestTreeItem>>& OutChildren)
{
	if (Item.IsValid())
	{
		OutChildren = Item->Children;
	}
}

void SQuestEditor::OnSelectionChanged(TSharedPtr<FQuestTreeItem> Item, ESelectInfo::Type SelectInfo)
{
	SelectedItem = Item;
	UpdateDetailsPanel();
}

FSlateColor SQuestEditor::GetStateTypeColor(const FString& StateType) const
{
	if (StateType.Equals(TEXT("success"), ESearchCase::IgnoreCase))
	{
		return FSlateColor(FLinearColor(0.2f, 0.8f, 0.2f));  // Green
	}
	else if (StateType.Equals(TEXT("failure"), ESearchCase::IgnoreCase))
	{
		return FSlateColor(FLinearColor(0.8f, 0.2f, 0.2f));  // Red
	}
	return FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f));  // Gray for regular
}

void SQuestEditor::RebuildTree()
{
	RootItems.Empty();

	// Create quest root
	TSharedPtr<FQuestTreeItem> QuestItem = MakeShared<FQuestTreeItem>(
		EQuestTreeItemType::Quest,
		QuestDefinition.Name,
		QuestDefinition.QuestName
	);

	// Add states
	for (int32 StateIdx = 0; StateIdx < QuestDefinition.States.Num(); StateIdx++)
	{
		const auto& State = QuestDefinition.States[StateIdx];

		TSharedPtr<FQuestTreeItem> StateItem = MakeShared<FQuestTreeItem>(
			EQuestTreeItemType::State,
			State.Id,
			FString::Printf(TEXT("%s: %s"), *State.Id, *State.Description)
		);
		StateItem->StateType = State.Type;
		StateItem->StateIndex = StateIdx;
		StateItem->Parent = QuestItem;

		// Add branches
		for (int32 BranchIdx = 0; BranchIdx < State.Branches.Num(); BranchIdx++)
		{
			const auto& Branch = State.Branches[BranchIdx];

			FString BranchText = Branch.DestinationState;
			if (!Branch.Id.IsEmpty())
			{
				BranchText = FString::Printf(TEXT("[%s] -> %s"), *Branch.Id, *Branch.DestinationState);
			}

			TSharedPtr<FQuestTreeItem> BranchItem = MakeShared<FQuestTreeItem>(
				EQuestTreeItemType::Branch,
				Branch.Id,
				BranchText
			);
			BranchItem->StateIndex = StateIdx;
			BranchItem->BranchIndex = BranchIdx;
			BranchItem->Parent = StateItem;

			// Add tasks
			for (int32 TaskIdx = 0; TaskIdx < Branch.Tasks.Num(); TaskIdx++)
			{
				const auto& Task = Branch.Tasks[TaskIdx];

				FString TaskText = Task.TaskClass;
				if (!Task.Argument.IsEmpty())
				{
					TaskText += FString::Printf(TEXT(" (%s)"), *Task.Argument);
				}
				if (Task.Quantity > 1)
				{
					TaskText += FString::Printf(TEXT(" x%d"), Task.Quantity);
				}

				TSharedPtr<FQuestTreeItem> TaskItem = MakeShared<FQuestTreeItem>(
					EQuestTreeItemType::Task,
					Task.TaskClass,
					TaskText
				);
				TaskItem->StateIndex = StateIdx;
				TaskItem->BranchIndex = BranchIdx;
				TaskItem->TaskIndex = TaskIdx;
				TaskItem->Parent = BranchItem;

				BranchItem->Children.Add(TaskItem);
			}

			StateItem->Children.Add(BranchItem);
		}

		QuestItem->Children.Add(StateItem);
	}

	RootItems.Add(QuestItem);

	if (TreeView.IsValid())
	{
		TreeView->RequestTreeRefresh();

		// Expand all items
		for (const auto& Root : RootItems)
		{
			TreeView->SetItemExpansion(Root, true);
			for (const auto& Child : Root->Children)
			{
				TreeView->SetItemExpansion(Child, true);
				for (const auto& GrandChild : Child->Children)
				{
					TreeView->SetItemExpansion(GrandChild, true);
				}
			}
		}
	}
}

void SQuestEditor::UpdateDetailsPanel()
{
	if (!DetailsPanel.IsValid())
		return;

	DetailsPanel->ClearChildren();

	if (!SelectedItem.IsValid())
	{
		DetailsPanel->AddSlot()
		.AutoHeight()
		.Padding(10)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SelectItem2", "Select a state, branch, or task to edit its properties."))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)))
		];
		return;
	}

	switch (SelectedItem->Type)
	{
	case EQuestTreeItemType::State:
		BuildStateDetails(SelectedItem->StateIndex);
		break;
	case EQuestTreeItemType::Branch:
		BuildBranchDetails(SelectedItem->StateIndex, SelectedItem->BranchIndex);
		break;
	case EQuestTreeItemType::Task:
		BuildTaskDetails(SelectedItem->StateIndex, SelectedItem->BranchIndex, SelectedItem->TaskIndex);
		break;
	default:
		DetailsPanel->AddSlot()
		.AutoHeight()
		.Padding(10)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("QuestSelected", "Quest properties are shown in the left panel."))
		];
		break;
	}
}

void SQuestEditor::BuildStateDetails(int32 StateIndex)
{
	if (!QuestDefinition.States.IsValidIndex(StateIndex))
		return;

	FManifestQuestStateDefinition& State = QuestDefinition.States[StateIndex];

	// State ID
	DetailsPanel->AddSlot()
	.AutoHeight()
	.Padding(10, 5)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("StateDetails", "State Details"))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
	];

	DetailsPanel->AddSlot()
	.AutoHeight()
	.Padding(10, 2)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 10, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("StateId", "ID:"))
			.MinDesiredWidth(100)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SEditableTextBox)
			.Text(FText::FromString(State.Id))
			.OnTextCommitted_Lambda([this, StateIndex](const FText& Text, ETextCommit::Type) {
				if (QuestDefinition.States.IsValidIndex(StateIndex))
				{
					QuestDefinition.States[StateIndex].Id = Text.ToString();
					RebuildTree();
				}
			})
		]
	];

	// State Description
	DetailsPanel->AddSlot()
	.AutoHeight()
	.Padding(10, 2)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 10, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("StateDesc", "Description:"))
			.MinDesiredWidth(100)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SEditableTextBox)
			.Text(FText::FromString(State.Description))
			.OnTextCommitted_Lambda([this, StateIndex](const FText& Text, ETextCommit::Type) {
				if (QuestDefinition.States.IsValidIndex(StateIndex))
				{
					QuestDefinition.States[StateIndex].Description = Text.ToString();
					RebuildTree();
				}
			})
		]
	];

	// State Type dropdown
	TSharedPtr<FString> CurrentType = MakeShared<FString>(State.Type);
	TArray<TSharedPtr<FString>> TypeOptions;
	TypeOptions.Add(MakeShared<FString>(TEXT("regular")));
	TypeOptions.Add(MakeShared<FString>(TEXT("success")));
	TypeOptions.Add(MakeShared<FString>(TEXT("failure")));

	DetailsPanel->AddSlot()
	.AutoHeight()
	.Padding(10, 2)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 10, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("StateType", "Type:"))
			.MinDesiredWidth(100)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&TypeOptions)
			.InitiallySelectedItem(TypeOptions[State.Type == TEXT("success") ? 1 : (State.Type == TEXT("failure") ? 2 : 0)])
			.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) {
				return SNew(STextBlock).Text(FText::FromString(*Item));
			})
			.OnSelectionChanged_Lambda([this, StateIndex](TSharedPtr<FString> Selected, ESelectInfo::Type) {
				if (Selected.IsValid() && QuestDefinition.States.IsValidIndex(StateIndex))
				{
					QuestDefinition.States[StateIndex].Type = *Selected;
					RebuildTree();
				}
			})
			.Content()
			[
				SNew(STextBlock)
				.Text(FText::FromString(State.Type))
			]
		]
	];
}

void SQuestEditor::BuildBranchDetails(int32 StateIndex, int32 BranchIndex)
{
	if (!QuestDefinition.States.IsValidIndex(StateIndex))
		return;
	if (!QuestDefinition.States[StateIndex].Branches.IsValidIndex(BranchIndex))
		return;

	FManifestQuestBranchDefinition& Branch = QuestDefinition.States[StateIndex].Branches[BranchIndex];

	DetailsPanel->AddSlot()
	.AutoHeight()
	.Padding(10, 5)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("BranchDetails", "Branch Details"))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
	];

	// Branch ID
	DetailsPanel->AddSlot()
	.AutoHeight()
	.Padding(10, 2)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 10, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BranchId", "ID (optional):"))
			.MinDesiredWidth(120)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SEditableTextBox)
			.Text(FText::FromString(Branch.Id))
			.OnTextCommitted_Lambda([this, StateIndex, BranchIndex](const FText& Text, ETextCommit::Type) {
				if (QuestDefinition.States.IsValidIndex(StateIndex) &&
					QuestDefinition.States[StateIndex].Branches.IsValidIndex(BranchIndex))
				{
					QuestDefinition.States[StateIndex].Branches[BranchIndex].Id = Text.ToString();
					RebuildTree();
				}
			})
		]
	];

	// Destination State
	DetailsPanel->AddSlot()
	.AutoHeight()
	.Padding(10, 2)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 10, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("DestState", "Destination State:"))
			.MinDesiredWidth(120)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SEditableTextBox)
			.Text(FText::FromString(Branch.DestinationState))
			.OnTextCommitted_Lambda([this, StateIndex, BranchIndex](const FText& Text, ETextCommit::Type) {
				if (QuestDefinition.States.IsValidIndex(StateIndex) &&
					QuestDefinition.States[StateIndex].Branches.IsValidIndex(BranchIndex))
				{
					QuestDefinition.States[StateIndex].Branches[BranchIndex].DestinationState = Text.ToString();
					RebuildTree();
				}
			})
		]
	];

	// Hidden checkbox
	DetailsPanel->AddSlot()
	.AutoHeight()
	.Padding(10, 2)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked(Branch.bHidden ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged_Lambda([this, StateIndex, BranchIndex](ECheckBoxState NewState) {
				if (QuestDefinition.States.IsValidIndex(StateIndex) &&
					QuestDefinition.States[StateIndex].Branches.IsValidIndex(BranchIndex))
				{
					QuestDefinition.States[StateIndex].Branches[BranchIndex].bHidden = (NewState == ECheckBoxState::Checked);
				}
			})
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(5, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Hidden", "Hidden (don't show in journal)"))
		]
	];
}

void SQuestEditor::BuildTaskDetails(int32 StateIndex, int32 BranchIndex, int32 TaskIndex)
{
	if (!QuestDefinition.States.IsValidIndex(StateIndex))
		return;
	if (!QuestDefinition.States[StateIndex].Branches.IsValidIndex(BranchIndex))
		return;
	if (!QuestDefinition.States[StateIndex].Branches[BranchIndex].Tasks.IsValidIndex(TaskIndex))
		return;

	FManifestQuestTaskDefinition& Task = QuestDefinition.States[StateIndex].Branches[BranchIndex].Tasks[TaskIndex];

	DetailsPanel->AddSlot()
	.AutoHeight()
	.Padding(10, 5)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("TaskDetails", "Task Details"))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
	];

	// Task Class
	DetailsPanel->AddSlot()
	.AutoHeight()
	.Padding(10, 2)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 10, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TaskClass", "Task Class:"))
			.MinDesiredWidth(100)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SEditableTextBox)
			.Text(FText::FromString(Task.TaskClass))
			.HintText(LOCTEXT("TaskClassHint", "BPT_FindItem, BPT_GoToLocation, BPT_FinishDialogue..."))
			.OnTextCommitted_Lambda([this, StateIndex, BranchIndex, TaskIndex](const FText& Text, ETextCommit::Type) {
				if (QuestDefinition.States.IsValidIndex(StateIndex) &&
					QuestDefinition.States[StateIndex].Branches.IsValidIndex(BranchIndex) &&
					QuestDefinition.States[StateIndex].Branches[BranchIndex].Tasks.IsValidIndex(TaskIndex))
				{
					QuestDefinition.States[StateIndex].Branches[BranchIndex].Tasks[TaskIndex].TaskClass = Text.ToString();
					RebuildTree();
				}
			})
		]
	];

	// Argument
	DetailsPanel->AddSlot()
	.AutoHeight()
	.Padding(10, 2)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 10, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TaskArg", "Argument:"))
			.MinDesiredWidth(100)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SEditableTextBox)
			.Text(FText::FromString(Task.Argument))
			.HintText(LOCTEXT("TaskArgHint", "Item name, location, NPC name..."))
			.OnTextCommitted_Lambda([this, StateIndex, BranchIndex, TaskIndex](const FText& Text, ETextCommit::Type) {
				if (QuestDefinition.States.IsValidIndex(StateIndex) &&
					QuestDefinition.States[StateIndex].Branches.IsValidIndex(BranchIndex) &&
					QuestDefinition.States[StateIndex].Branches[BranchIndex].Tasks.IsValidIndex(TaskIndex))
				{
					QuestDefinition.States[StateIndex].Branches[BranchIndex].Tasks[TaskIndex].Argument = Text.ToString();
					RebuildTree();
				}
			})
		]
	];

	// Quantity
	DetailsPanel->AddSlot()
	.AutoHeight()
	.Padding(10, 2)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 10, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TaskQty", "Quantity:"))
			.MinDesiredWidth(100)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SEditableTextBox)
			.Text(FText::AsNumber(Task.Quantity))
			.MinDesiredWidth(60)
			.OnTextCommitted_Lambda([this, StateIndex, BranchIndex, TaskIndex](const FText& Text, ETextCommit::Type) {
				if (QuestDefinition.States.IsValidIndex(StateIndex) &&
					QuestDefinition.States[StateIndex].Branches.IsValidIndex(BranchIndex) &&
					QuestDefinition.States[StateIndex].Branches[BranchIndex].Tasks.IsValidIndex(TaskIndex))
				{
					QuestDefinition.States[StateIndex].Branches[BranchIndex].Tasks[TaskIndex].Quantity = FCString::Atoi(*Text.ToString());
					RebuildTree();
				}
			})
		]
	];

	// Optional checkbox
	DetailsPanel->AddSlot()
	.AutoHeight()
	.Padding(10, 2)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked(Task.bOptional ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged_Lambda([this, StateIndex, BranchIndex, TaskIndex](ECheckBoxState NewState) {
				if (QuestDefinition.States.IsValidIndex(StateIndex) &&
					QuestDefinition.States[StateIndex].Branches.IsValidIndex(BranchIndex) &&
					QuestDefinition.States[StateIndex].Branches[BranchIndex].Tasks.IsValidIndex(TaskIndex))
				{
					QuestDefinition.States[StateIndex].Branches[BranchIndex].Tasks[TaskIndex].bOptional = (NewState == ECheckBoxState::Checked);
				}
			})
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(5, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Optional", "Optional"))
		]
	];

	// Hidden checkbox
	DetailsPanel->AddSlot()
	.AutoHeight()
	.Padding(10, 2)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked(Task.bHidden ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged_Lambda([this, StateIndex, BranchIndex, TaskIndex](ECheckBoxState NewState) {
				if (QuestDefinition.States.IsValidIndex(StateIndex) &&
					QuestDefinition.States[StateIndex].Branches.IsValidIndex(BranchIndex) &&
					QuestDefinition.States[StateIndex].Branches[BranchIndex].Tasks.IsValidIndex(TaskIndex))
				{
					QuestDefinition.States[StateIndex].Branches[BranchIndex].Tasks[TaskIndex].bHidden = (NewState == ECheckBoxState::Checked);
				}
			})
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(5, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TaskHidden", "Hidden"))
		]
	];
}

FReply SQuestEditor::OnAddStateClicked()
{
	FManifestQuestStateDefinition NewState;
	NewState.Id = FString::Printf(TEXT("State_%d"), QuestDefinition.States.Num());
	NewState.Description = TEXT("New state");
	NewState.Type = TEXT("regular");

	QuestDefinition.States.Add(NewState);
	RebuildTree();

	LogMessage(FString::Printf(TEXT("Added state: %s"), *NewState.Id));
	return FReply::Handled();
}

FReply SQuestEditor::OnAddBranchClicked()
{
	if (!SelectedItem.IsValid())
	{
		LogMessage(TEXT("ERROR: Select a state first to add a branch."));
		return FReply::Handled();
	}

	int32 StateIndex = SelectedItem->StateIndex;
	if (SelectedItem->Type == EQuestTreeItemType::Branch || SelectedItem->Type == EQuestTreeItemType::Task)
	{
		StateIndex = SelectedItem->StateIndex;
	}
	else if (SelectedItem->Type != EQuestTreeItemType::State)
	{
		LogMessage(TEXT("ERROR: Select a state to add a branch."));
		return FReply::Handled();
	}

	if (!QuestDefinition.States.IsValidIndex(StateIndex))
	{
		LogMessage(TEXT("ERROR: Invalid state index."));
		return FReply::Handled();
	}

	FManifestQuestBranchDefinition NewBranch;
	NewBranch.DestinationState = TEXT("");

	QuestDefinition.States[StateIndex].Branches.Add(NewBranch);
	RebuildTree();

	LogMessage(FString::Printf(TEXT("Added branch to state: %s"), *QuestDefinition.States[StateIndex].Id));
	return FReply::Handled();
}

FReply SQuestEditor::OnAddTaskClicked()
{
	if (!SelectedItem.IsValid())
	{
		LogMessage(TEXT("ERROR: Select a branch first to add a task."));
		return FReply::Handled();
	}

	int32 StateIndex = -1;
	int32 BranchIndex = -1;

	if (SelectedItem->Type == EQuestTreeItemType::Branch)
	{
		StateIndex = SelectedItem->StateIndex;
		BranchIndex = SelectedItem->BranchIndex;
	}
	else if (SelectedItem->Type == EQuestTreeItemType::Task)
	{
		StateIndex = SelectedItem->StateIndex;
		BranchIndex = SelectedItem->BranchIndex;
	}
	else
	{
		LogMessage(TEXT("ERROR: Select a branch to add a task."));
		return FReply::Handled();
	}

	if (!QuestDefinition.States.IsValidIndex(StateIndex) ||
		!QuestDefinition.States[StateIndex].Branches.IsValidIndex(BranchIndex))
	{
		LogMessage(TEXT("ERROR: Invalid state/branch index."));
		return FReply::Handled();
	}

	FManifestQuestTaskDefinition NewTask;
	NewTask.TaskClass = TEXT("BPT_FindItem");
	NewTask.Argument = TEXT("");
	NewTask.Quantity = 1;

	QuestDefinition.States[StateIndex].Branches[BranchIndex].Tasks.Add(NewTask);
	RebuildTree();

	LogMessage(TEXT("Added task to branch"));
	return FReply::Handled();
}

FReply SQuestEditor::OnDeleteClicked()
{
	if (!SelectedItem.IsValid())
	{
		LogMessage(TEXT("ERROR: Select an item to delete."));
		return FReply::Handled();
	}

	switch (SelectedItem->Type)
	{
	case EQuestTreeItemType::State:
		if (QuestDefinition.States.IsValidIndex(SelectedItem->StateIndex))
		{
			FString StateId = QuestDefinition.States[SelectedItem->StateIndex].Id;
			QuestDefinition.States.RemoveAt(SelectedItem->StateIndex);
			LogMessage(FString::Printf(TEXT("Deleted state: %s"), *StateId));
		}
		break;

	case EQuestTreeItemType::Branch:
		if (QuestDefinition.States.IsValidIndex(SelectedItem->StateIndex) &&
			QuestDefinition.States[SelectedItem->StateIndex].Branches.IsValidIndex(SelectedItem->BranchIndex))
		{
			QuestDefinition.States[SelectedItem->StateIndex].Branches.RemoveAt(SelectedItem->BranchIndex);
			LogMessage(TEXT("Deleted branch"));
		}
		break;

	case EQuestTreeItemType::Task:
		if (QuestDefinition.States.IsValidIndex(SelectedItem->StateIndex) &&
			QuestDefinition.States[SelectedItem->StateIndex].Branches.IsValidIndex(SelectedItem->BranchIndex) &&
			QuestDefinition.States[SelectedItem->StateIndex].Branches[SelectedItem->BranchIndex].Tasks.IsValidIndex(SelectedItem->TaskIndex))
		{
			QuestDefinition.States[SelectedItem->StateIndex].Branches[SelectedItem->BranchIndex].Tasks.RemoveAt(SelectedItem->TaskIndex);
			LogMessage(TEXT("Deleted task"));
		}
		break;

	default:
		LogMessage(TEXT("Cannot delete quest root."));
		return FReply::Handled();
	}

	SelectedItem.Reset();
	RebuildTree();
	UpdateDetailsPanel();
	return FReply::Handled();
}

FReply SQuestEditor::OnGenerateClicked()
{
	// Validate
	if (QuestDefinition.Name.IsEmpty())
	{
		LogMessage(TEXT("ERROR: Quest asset name is required."));
		return FReply::Handled();
	}

	if (QuestDefinition.States.Num() == 0)
	{
		LogMessage(TEXT("ERROR: Quest must have at least one state."));
		return FReply::Handled();
	}

	// Ensure start state is set
	if (QuestDefinition.StartState.IsEmpty() && QuestDefinition.States.Num() > 0)
	{
		QuestDefinition.StartState = QuestDefinition.States[0].Id;
	}

	LogMessage(FString::Printf(TEXT("Generating quest: %s"), *QuestDefinition.Name));
	LogMessage(FString::Printf(TEXT("  States: %d"), QuestDefinition.States.Num()));

	int32 TotalBranches = 0;
	int32 TotalTasks = 0;
	for (const auto& State : QuestDefinition.States)
	{
		TotalBranches += State.Branches.Num();
		for (const auto& Branch : State.Branches)
		{
			TotalTasks += Branch.Tasks.Num();
		}
	}
	LogMessage(FString::Printf(TEXT("  Branches: %d"), TotalBranches));
	LogMessage(FString::Printf(TEXT("  Tasks: %d"), TotalTasks));

	// Generate
	FGenerationResult Result = FQuestGenerator::Generate(QuestDefinition);

	if (Result.Status == EGenerationStatus::New)
	{
		LogMessage(FString::Printf(TEXT("SUCCESS: Quest generated: %s"), *QuestDefinition.Name));
	}
	else if (Result.Status == EGenerationStatus::Skipped)
	{
		LogMessage(FString::Printf(TEXT("SKIPPED: Quest already exists: %s"), *QuestDefinition.Name));
	}
	else
	{
		LogMessage(FString::Printf(TEXT("FAILED: %s"), *Result.Message));
	}

	return FReply::Handled();
}

FReply SQuestEditor::OnClearClicked()
{
	QuestDefinition = FManifestQuestDefinition();
	QuestDefinition.Name = TEXT("Quest_New");
	QuestDefinition.QuestName = TEXT("New Quest");
	QuestDefinition.QuestDescription = TEXT("Quest description");
	QuestDefinition.bTracked = true;
	QuestDefinition.Folder = TEXT("Quests");

	FManifestQuestStateDefinition StartState;
	StartState.Id = TEXT("Start");
	StartState.Description = TEXT("Quest started");
	StartState.Type = TEXT("regular");
	QuestDefinition.States.Add(StartState);

	// Update UI
	if (QuestNameBox.IsValid())
		QuestNameBox->SetText(FText::FromString(QuestDefinition.Name));
	if (DisplayNameBox.IsValid())
		DisplayNameBox->SetText(FText::FromString(QuestDefinition.QuestName));
	if (DescriptionBox.IsValid())
		DescriptionBox->SetText(FText::FromString(QuestDefinition.QuestDescription));
	if (TrackedCheckbox.IsValid())
		TrackedCheckbox->SetIsChecked(ECheckBoxState::Checked);

	SelectedItem.Reset();
	RebuildTree();
	UpdateDetailsPanel();

	LogMessage(TEXT("Quest cleared and reset to defaults."));
	return FReply::Handled();
}

void SQuestEditor::LogMessage(const FString& Message)
{
	if (OutputLog.IsValid())
	{
		FString CurrentText = OutputLog->GetText().ToString();
		FString Timestamp = FDateTime::Now().ToString(TEXT("[%H:%M:%S] "));
		CurrentText += Timestamp + Message + TEXT("\n");
		OutputLog->SetText(FText::FromString(CurrentText));
	}
}

//=============================================================================
// SQuestEditorWindow
//=============================================================================

void SQuestEditorWindow::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SAssignNew(QuestEditor, SQuestEditor)
	];
}

#undef LOCTEXT_NAMESPACE
