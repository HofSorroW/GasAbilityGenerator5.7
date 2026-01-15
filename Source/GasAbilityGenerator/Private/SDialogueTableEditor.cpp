// GasAbilityGenerator - Dialogue Table Editor Implementation
// v4.2.8: Fixed status bar not updating - explicit invalidation required for sibling widgets
//
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#include "SDialogueTableEditor.h"
#include "DialogueTableValidator.h"
#include "DialogueTableConverter.h"
#include "GasAbilityGeneratorTypes.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SBox.h"
#include "Styling/AppStyle.h"
#include "Misc/FileHelper.h"
#include "DesktopPlatformModule.h"
#include "EditorDirectories.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "DialogueTableEditor"

//=============================================================================
// SDialogueTableRow - Individual Row Widget
//=============================================================================

void SDialogueTableRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	RowDataEx = InArgs._RowData;
	OnRowModified = InArgs._OnRowModified;

	SMultiColumnTableRow<TSharedPtr<FDialogueTableRowEx>>::Construct(
		FSuperRowType::FArguments()
			.Padding(FMargin(2.0f, 1.0f)),
		InOwnerTable
	);
}

TSharedRef<SWidget> SDialogueTableRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (!RowDataEx.IsValid() || !RowDataEx->Data.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	FDialogueTableRow* RowData = RowDataEx->Data.Get();

	if (ColumnName == TEXT("Seq"))
	{
		return CreateSeqCell();
	}
	if (ColumnName == TEXT("DialogueID"))
	{
		return CreateFNameCell(RowData->DialogueID, TEXT("DBP_NPCName"));
	}
	if (ColumnName == TEXT("NodeID"))
	{
		return CreateNodeIDCell();
	}
	if (ColumnName == TEXT("NodeType"))
	{
		return CreateNodeTypeCell();
	}
	if (ColumnName == TEXT("Speaker"))
	{
		return CreateSpeakerCell();
	}
	if (ColumnName == TEXT("Text"))
	{
		return CreateTextCell(RowData->Text, TEXT("Dialogue text..."), true);  // With tooltip
	}
	if (ColumnName == TEXT("OptionText"))
	{
		return CreateTextCell(RowData->OptionText, TEXT("Player choice text"), true);  // With tooltip
	}
	if (ColumnName == TEXT("ParentNodeID"))
	{
		return CreateFNameCell(RowData->ParentNodeID, TEXT("parent_node"));
	}
	if (ColumnName == TEXT("NextNodeIDs"))
	{
		return CreateNextNodesCell();
	}
	if (ColumnName == TEXT("Skippable"))
	{
		return CreateSkippableCell();
	}
	if (ColumnName == TEXT("Notes"))
	{
		return CreateNotesCell();
	}

	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SDialogueTableRow::CreateSeqCell()
{
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(STextBlock)
				.Text(FText::FromString(RowDataEx->SeqDisplay))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateNodeIDCell()
{
	FDialogueTableRow* RowData = RowDataEx->Data.Get();

	// Calculate indent based on depth (8px per level)
	float IndentSize = RowDataEx->Depth * 8.0f;

	return SNew(SHorizontalBox)
		// Indent spacer
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SSpacer)
				.Size(FVector2D(IndentSize, 1.0f))
		]
		// Node ID text
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableText)
				.Text_Lambda([RowData]() { return FText::FromName(RowData->NodeID); })
				.HintText(FText::FromString(TEXT("node_id")))
				.OnTextCommitted_Lambda([this, RowData](const FText& NewText, ETextCommit::Type)
				{
					RowData->NodeID = FName(*NewText.ToString());
					MarkModified();
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateTextCell(FString& Value, const FString& Hint, bool bWithTooltip)
{
	TSharedRef<SWidget> EditableText = SNew(SEditableText)
		.Text_Lambda([&Value]() { return FText::FromString(Value); })
		.HintText(FText::FromString(Hint))
		.OnTextCommitted_Lambda([this, &Value](const FText& NewText, ETextCommit::Type)
		{
			Value = NewText.ToString();
			MarkModified();
		})
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9));

	// Add tooltip for Text/OptionText columns to show full content on hover
	if (bWithTooltip)
	{
		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			.ToolTipText_Lambda([&Value]()
			{
				return Value.IsEmpty() ? FText::GetEmpty() : FText::FromString(Value);
			})
			[
				EditableText
			];
	}

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			EditableText
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateFNameCell(FName& Value, const FString& Hint)
{
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableText)
				.Text_Lambda([&Value]() { return FText::FromName(Value); })
				.HintText(FText::FromString(Hint))
				.OnTextCommitted_Lambda([this, &Value](const FText& NewText, ETextCommit::Type)
				{
					Value = FName(*NewText.ToString());
					MarkModified();
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateNodeTypeCell()
{
	FDialogueTableRow* RowData = RowDataEx->Data.Get();

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableText)
				.Text_Lambda([RowData]()
				{
					return FText::FromString(RowData->NodeType == EDialogueTableNodeType::NPC ? TEXT("NPC") : TEXT("Player"));
				})
				.OnTextCommitted_Lambda([this, RowData](const FText& NewText, ETextCommit::Type)
				{
					FString TypeStr = NewText.ToString().ToLower();
					if (TypeStr.Contains(TEXT("player")) || TypeStr == TEXT("p"))
					{
						RowData->NodeType = EDialogueTableNodeType::Player;
					}
					else
					{
						RowData->NodeType = EDialogueTableNodeType::NPC;
					}
					MarkModified();
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateSpeakerCell()
{
	FDialogueTableRow* RowData = RowDataEx->Data.Get();

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableText)
				.Text_Lambda([RowData]()
				{
					// For Player nodes, show "Player" if Speaker is empty
					if (RowData->NodeType == EDialogueTableNodeType::Player)
					{
						if (RowData->Speaker.IsNone() || RowData->Speaker.ToString().IsEmpty())
						{
							return FText::FromString(TEXT("Player"));
						}
					}
					return FText::FromName(RowData->Speaker);
				})
				.HintText(FText::FromString(TEXT("Speaker Name")))
				.OnTextCommitted_Lambda([this, RowData](const FText& NewText, ETextCommit::Type)
				{
					FString SpeakerStr = NewText.ToString();
					// Don't store "Player" in the data - keep it empty for player nodes
					if (RowData->NodeType == EDialogueTableNodeType::Player && SpeakerStr.Equals(TEXT("Player"), ESearchCase::IgnoreCase))
					{
						RowData->Speaker = NAME_None;
					}
					else
					{
						RowData->Speaker = FName(*SpeakerStr);
					}
					MarkModified();
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateNextNodesCell()
{
	FDialogueTableRow* RowData = RowDataEx->Data.Get();

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableText)
				.Text_Lambda([RowData]()
				{
					FString Result;
					for (int32 i = 0; i < RowData->NextNodeIDs.Num(); i++)
					{
						if (i > 0) Result += TEXT(", ");
						Result += RowData->NextNodeIDs[i].ToString();
					}
					return FText::FromString(Result);
				})
				.HintText(FText::FromString(TEXT("node1, node2, ...")))
				.OnTextCommitted_Lambda([this, RowData](const FText& NewText, ETextCommit::Type)
				{
					RowData->NextNodeIDs.Empty();
					TArray<FString> Parts;
					NewText.ToString().ParseIntoArray(Parts, TEXT(","));
					for (FString& Part : Parts)
					{
						Part.TrimStartAndEndInline();
						if (!Part.IsEmpty())
						{
							RowData->NextNodeIDs.Add(FName(*Part));
						}
					}
					MarkModified();
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateSkippableCell()
{
	FDialogueTableRow* RowData = RowDataEx->Data.Get();

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		.HAlign(HAlign_Center)
		[
			SNew(SCheckBox)
				.IsChecked_Lambda([RowData]()
				{
					return RowData->bSkippable ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([this, RowData](ECheckBoxState NewState)
				{
					RowData->bSkippable = (NewState == ECheckBoxState::Checked);
					MarkModified();
				})
				.ToolTipText(LOCTEXT("SkippableTip", "Whether player can skip this line"))
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateNotesCell()
{
	FDialogueTableRow* RowData = RowDataEx->Data.Get();

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		.ToolTipText_Lambda([RowData]()
		{
			return RowData->Notes.IsEmpty() ? FText::GetEmpty() : FText::FromString(RowData->Notes);
		})
		[
			SNew(SEditableText)
				.Text_Lambda([RowData]() { return FText::FromString(RowData->Notes); })
				.HintText(LOCTEXT("NotesHint", "Designer notes..."))
				.OnTextCommitted_Lambda([this, RowData](const FText& NewText, ETextCommit::Type)
				{
					RowData->Notes = NewText.ToString();
					MarkModified();
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

void SDialogueTableRow::MarkModified()
{
	OnRowModified.ExecuteIfBound();
}

//=============================================================================
// SDialogueTableEditor - Main Table Widget
//=============================================================================

void SDialogueTableEditor::Construct(const FArguments& InArgs)
{
	TableData = InArgs._TableData;
	SyncFromTableData();
	InitializeColumnFilters();
	UpdateColumnFilterOptions();

	ChildSlot
	[
		SNew(SVerticalBox)

		// Toolbar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			BuildToolbar()
		]

		// Separator
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
		]

		// Table - fills entire width
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(ListView, SListView<TSharedPtr<FDialogueTableRowEx>>)
				.ListItemsSource(&DisplayedRows)
				.OnGenerateRow(this, &SDialogueTableEditor::OnGenerateRow)
				.OnSelectionChanged(this, &SDialogueTableEditor::OnSelectionChanged)
				.SelectionMode(ESelectionMode::Multi)
				.HeaderRow(BuildHeaderRow())
		]

		// Status bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			BuildStatusBar()
		]
	];

	ApplyFlowOrder();
	UpdateStatusBar();  // Initialize status bar with correct values
}

void SDialogueTableEditor::SetTableData(UDialogueTableData* InTableData)
{
	TableData = InTableData;
	SyncFromTableData();
	UpdateColumnFilterOptions();
	ApplyFlowOrder();
	UpdateStatusBar();
}

TSharedRef<SWidget> SDialogueTableEditor::BuildToolbar()
{
	return SNew(SHorizontalBox)

		// Add Row
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("AddRow", "+ Add Node"))
				.OnClicked(this, &SDialogueTableEditor::OnAddRowClicked)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
		]

		// Delete (re-parents children)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("DeleteRows", "Delete"))
				.OnClicked(this, &SDialogueTableEditor::OnDeleteRowsClicked)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
				.ToolTipText(LOCTEXT("DeleteTip", "Delete selected node(s). Children are re-parented to grandparent."))
		]

		// Delete Branch (cascade delete)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("DeleteBranch", "Delete Branch"))
				.OnClicked(this, &SDialogueTableEditor::OnDeleteBranchClicked)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
				.ToolTipText(LOCTEXT("DeleteBranchTip", "Delete selected node(s) AND all their children (cascade delete)"))
		]

		// Separator
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SNew(SSeparator)
				.Orientation(Orient_Vertical)
		]

		// Reset Order
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("ResetOrder", "Reset Flow Order"))
				.OnClicked(this, &SDialogueTableEditor::OnResetOrderClicked)
				.ToolTipText(LOCTEXT("ResetOrderTip", "Sort by dialogue flow (root -> children -> grandchildren)"))
		]

		// Clear Filters
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("ClearFilters", "Clear Filters"))
				.OnClicked(this, &SDialogueTableEditor::OnClearFiltersClicked)
				.ToolTipText(LOCTEXT("ClearFiltersTip", "Clear all text and dropdown filters"))
		]

		// Spacer
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNullWidget::NullWidget
		]

		// Validate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("Validate", "Validate"))
				.OnClicked(this, &SDialogueTableEditor::OnValidateClicked)
		]

		// Generate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("Generate", "Generate Dialogues"))
				.OnClicked(this, &SDialogueTableEditor::OnGenerateClicked)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
		]

		// Export CSV
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("ExportCSV", "Export CSV"))
				.OnClicked(this, &SDialogueTableEditor::OnExportCSVClicked)
		]

		// Import CSV
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("ImportCSV", "Import CSV"))
				.OnClicked(this, &SDialogueTableEditor::OnImportCSVClicked)
		];
}

TSharedRef<SWidget> SDialogueTableEditor::BuildColumnHeaderContent(const FDialogueTableColumn& Col)
{
	FColumnFilterState& FilterState = ColumnFilters.FindOrAdd(Col.ColumnId);

	return SNew(SVerticalBox)

		// Column name
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(STextBlock)
				.Text(Col.DisplayName)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		]

		// Text filter - live filtering with OnTextChanged
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f, 1.0f)
		[
			SNew(SEditableText)
				.HintText(LOCTEXT("FilterHint", "Filter..."))
				.OnTextChanged_Lambda([this, ColumnId = Col.ColumnId](const FText& NewText)
				{
					OnColumnTextFilterChanged(ColumnId, NewText);
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		]

		// Multi-select filter with checkboxes in a menu
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f, 1.0f)
		[
			SNew(SComboButton)
				.OnGetMenuContent_Lambda([this, ColumnId = Col.ColumnId]() -> TSharedRef<SWidget>
				{
					FColumnFilterState* State = ColumnFilters.Find(ColumnId);
					if (!State) return SNew(SBox);

					TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox);

					// Add "Select All" / "Clear All" buttons
					MenuContent->AddSlot()
					.AutoHeight()
					.Padding(4.0f, 2.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
								.Text(LOCTEXT("SelectAll", "All"))
								.OnClicked_Lambda([this, ColumnId, State]()
								{
									State->SelectedValues.Empty();
									ApplyFilters();
									return FReply::Handled();
								})
								.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(4.0f, 0.0f)
						[
							SNew(SButton)
								.Text(LOCTEXT("ClearSel", "None"))
								.OnClicked_Lambda([this, ColumnId, State]()
								{
									// Select all options (which will filter to nothing if all are selected)
									// Actually, we want "None" to mean clear selection = show all
									State->SelectedValues.Empty();
									ApplyFilters();
									return FReply::Handled();
								})
								.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						]
					];

					MenuContent->AddSlot()
					.AutoHeight()
					[
						SNew(SSeparator)
					];

					// Add checkbox for each option (skip the empty "(All)" option)
					for (const TSharedPtr<FString>& Option : State->DropdownOptions)
					{
						if (!Option.IsValid() || Option->IsEmpty()) continue;  // Skip "(All)"

						FString OptionValue = *Option;
						MenuContent->AddSlot()
						.AutoHeight()
						.Padding(4.0f, 1.0f)
						[
							SNew(SCheckBox)
								.IsChecked_Lambda([State, OptionValue]()
								{
									return State->SelectedValues.Contains(OptionValue) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
								})
								.OnCheckStateChanged_Lambda([this, State, OptionValue](ECheckBoxState NewState)
								{
									if (NewState == ECheckBoxState::Checked)
									{
										State->SelectedValues.Add(OptionValue);
									}
									else
									{
										State->SelectedValues.Remove(OptionValue);
									}
									ApplyFilters();
								})
								[
									SNew(STextBlock)
										.Text(FText::FromString(OptionValue))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								]
						];
					}

					return SNew(SBox)
						.MaxDesiredHeight(300.0f)
						[
							SNew(SScrollBox)
							+ SScrollBox::Slot()
							[
								MenuContent
							]
						];
				})
				.ButtonContent()
				[
					SNew(STextBlock)
						.Text_Lambda([&FilterState]()
						{
							if (FilterState.SelectedValues.Num() == 0)
							{
								return LOCTEXT("AllFilter", "(All)");
							}
							else if (FilterState.SelectedValues.Num() == 1)
							{
								for (const FString& Val : FilterState.SelectedValues)
								{
									return FText::FromString(Val);
								}
							}
							return FText::Format(LOCTEXT("MultiSelect", "({0} selected)"), FText::AsNumber(FilterState.SelectedValues.Num()));
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				]
		];
}

TSharedRef<SHeaderRow> SDialogueTableEditor::BuildHeaderRow()
{
	TSharedRef<SHeaderRow> Header = SNew(SHeaderRow);

	TArray<FDialogueTableColumn> Columns = GetDialogueTableColumns();

	for (const FDialogueTableColumn& Col : Columns)
	{
		Header->AddColumn(
			SHeaderRow::Column(Col.ColumnId)
				.DefaultLabel(Col.DisplayName)
				.FillWidth(Col.DefaultWidth)
				.SortMode(this, &SDialogueTableEditor::GetColumnSortMode, Col.ColumnId)
				.OnSort(this, &SDialogueTableEditor::OnColumnSortModeChanged)
				.HeaderContent()
				[
					BuildColumnHeaderContent(Col)
				]
		);
	}

	HeaderRow = Header;
	return Header;
}

TSharedRef<SWidget> SDialogueTableEditor::BuildStatusBar()
{
	// v4.2.8: Use SAssignNew to store StatusBar reference for explicit invalidation
	// Text_Lambda is evaluated during Paint, but ListView->RequestListRefresh() only
	// invalidates the ListView, not sibling widgets. We must explicitly invalidate.
	return SAssignNew(StatusBar, SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SNew(STextBlock)
				.Text_Lambda([this]()
				{
					UE_LOG(LogTemp, Verbose, TEXT("[DialogueTableEditor] Status bar lambda called - AllRows: %d"), AllRows.Num());
					return FText::Format(
						LOCTEXT("TotalNodes", "Total: {0} nodes"),
						FText::AsNumber(AllRows.Num())
					);
				})
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f)
		[
			SNew(STextBlock)
				.Text_Lambda([this]()
				{
					TSet<FName> UniqueDialogues;
					for (const auto& Row : AllRows)
					{
						if (Row->Data.IsValid() && !Row->Data->DialogueID.IsNone())
						{
							UniqueDialogues.Add(Row->Data->DialogueID);
						}
					}
					return FText::Format(
						LOCTEXT("DialogueCount", "Dialogues: {0}"),
						FText::AsNumber(UniqueDialogues.Num())
					);
				})
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f)
		[
			SNew(STextBlock)
				.Text_Lambda([this]()
				{
					return FText::Format(
						LOCTEXT("ShowingCount", "Showing: {0}"),
						FText::AsNumber(DisplayedRows.Num())
					);
				})
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f)
		[
			SNew(STextBlock)
				.Text_Lambda([this]()
				{
					int32 Selected = ListView.IsValid() ? ListView->GetNumItemsSelected() : 0;
					return FText::Format(
						LOCTEXT("SelectedCount", "Selected: {0}"),
						FText::AsNumber(Selected)
					);
				})
		];
}

void SDialogueTableEditor::UpdateStatusBar()
{
	// v4.2.8: Explicitly invalidate status bar to trigger repaint
	// Text_Lambda is only evaluated during Paint, so we must force a repaint.
	// ListView->RequestListRefresh() only invalidates the ListView, not siblings.
	if (StatusBar.IsValid())
	{
		StatusBar->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

TSharedRef<ITableRow> SDialogueTableEditor::OnGenerateRow(TSharedPtr<FDialogueTableRowEx> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SDialogueTableRow, OwnerTable)
		.RowData(Item)
		.OnRowModified(FSimpleDelegate::CreateSP(this, &SDialogueTableEditor::OnRowModified));
}

void SDialogueTableEditor::OnSelectionChanged(TSharedPtr<FDialogueTableRowEx> Item, ESelectInfo::Type SelectInfo)
{
	// Update selected count in status bar
	UpdateStatusBar();
}

EColumnSortMode::Type SDialogueTableEditor::GetColumnSortMode(FName ColumnId) const
{
	return (SortColumn == ColumnId) ? SortMode : EColumnSortMode::None;
}

void SDialogueTableEditor::OnColumnSortModeChanged(EColumnSortPriority::Type Priority, const FName& ColumnId, EColumnSortMode::Type NewSortMode)
{
	SortColumn = ColumnId;
	SortMode = NewSortMode;
	ApplySorting();
	RefreshList();
}

void SDialogueTableEditor::RefreshList()
{
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}

	// Update status bar numbers directly
	UpdateStatusBar();
}

TArray<TSharedPtr<FDialogueTableRowEx>> SDialogueTableEditor::GetSelectedRows() const
{
	TArray<TSharedPtr<FDialogueTableRowEx>> Selected;
	if (ListView.IsValid())
	{
		Selected = ListView->GetSelectedItems();
	}
	return Selected;
}

void SDialogueTableEditor::SyncFromTableData()
{
	AllRows.Empty();
	if (TableData)
	{
		for (FDialogueTableRow& Row : TableData->Rows)
		{
			TSharedPtr<FDialogueTableRowEx> RowEx = MakeShared<FDialogueTableRowEx>();
			RowEx->Data = MakeShared<FDialogueTableRow>(Row);
			AllRows.Add(RowEx);
		}
	}
	CalculateSequences();
}

void SDialogueTableEditor::SyncToTableData()
{
	if (TableData)
	{
		TableData->Rows.Empty();
		for (const TSharedPtr<FDialogueTableRowEx>& RowEx : AllRows)
		{
			if (RowEx->Data.IsValid())
			{
				TableData->Rows.Add(*RowEx->Data);
			}
		}
	}
}

void SDialogueTableEditor::CalculateSequences()
{
	// Build node lookup map per dialogue
	TMap<FName, TArray<TSharedPtr<FDialogueTableRowEx>>> DialogueGroups;

	for (TSharedPtr<FDialogueTableRowEx>& RowEx : AllRows)
	{
		if (!RowEx->Data.IsValid()) continue;
		DialogueGroups.FindOrAdd(RowEx->Data->DialogueID).Add(RowEx);
	}

	// Process each dialogue with depth-first tree traversal
	for (auto& DialoguePair : DialogueGroups)
	{
		TArray<TSharedPtr<FDialogueTableRowEx>>& Rows = DialoguePair.Value;

		// Build local node map for this dialogue
		TMap<FName, TSharedPtr<FDialogueTableRowEx>> LocalNodeMap;
		for (TSharedPtr<FDialogueTableRowEx>& RowEx : Rows)
		{
			LocalNodeMap.Add(RowEx->Data->NodeID, RowEx);
		}

		// Calculate depth for each node
		TSet<FName> Visited;
		for (TSharedPtr<FDialogueTableRowEx>& RowEx : Rows)
		{
			Visited.Empty();
			RowEx->Depth = CalculateNodeDepth(*RowEx->Data, LocalNodeMap, Visited);
		}

		// Find root nodes (no parent)
		TArray<TSharedPtr<FDialogueTableRowEx>> RootNodes;
		for (TSharedPtr<FDialogueTableRowEx>& RowEx : Rows)
		{
			if (RowEx->Data->ParentNodeID.IsNone())
			{
				RootNodes.Add(RowEx);
			}
		}

		// Depth-first traversal to assign sequence numbers
		int32 Seq = 1;
		TSet<FName> TraversalVisited;

		// Recursive lambda for tree traversal
		TFunction<void(TSharedPtr<FDialogueTableRowEx>)> TraverseNode = [&](TSharedPtr<FDialogueTableRowEx> Node)
		{
			if (!Node.IsValid() || !Node->Data.IsValid()) return;
			if (TraversalVisited.Contains(Node->Data->NodeID)) return;

			TraversalVisited.Add(Node->Data->NodeID);
			Node->Sequence = Seq++;
			Node->SeqDisplay = FString::Printf(TEXT("%d"), Node->Sequence);

			// Traverse children in order of NextNodeIDs
			for (const FName& ChildID : Node->Data->NextNodeIDs)
			{
				TSharedPtr<FDialogueTableRowEx>* ChildPtr = LocalNodeMap.Find(ChildID);
				if (ChildPtr && ChildPtr->IsValid())
				{
					TraverseNode(*ChildPtr);
				}
			}
		};

		// Start traversal from root nodes
		for (TSharedPtr<FDialogueTableRowEx>& RootNode : RootNodes)
		{
			TraverseNode(RootNode);
		}

		// Handle any orphan nodes not reached by traversal (assign remaining sequence numbers)
		for (TSharedPtr<FDialogueTableRowEx>& RowEx : Rows)
		{
			if (!TraversalVisited.Contains(RowEx->Data->NodeID))
			{
				RowEx->Sequence = Seq++;
				RowEx->SeqDisplay = FString::Printf(TEXT("%d*"), RowEx->Sequence); // Mark orphans with *
			}
		}
	}
}

int32 SDialogueTableEditor::CalculateNodeDepth(const FDialogueTableRow& Row, const TMap<FName, TSharedPtr<FDialogueTableRowEx>>& NodeMap, TSet<FName>& Visited)
{
	// Prevent infinite loops
	if (Visited.Contains(Row.NodeID))
	{
		return 0;
	}
	Visited.Add(Row.NodeID);

	// Root node (no parent)
	if (Row.ParentNodeID.IsNone())
	{
		return 0;
	}

	// Find parent
	const TSharedPtr<FDialogueTableRowEx>* ParentPtr = NodeMap.Find(Row.ParentNodeID);
	if (!ParentPtr || !(*ParentPtr)->Data.IsValid())
	{
		return 0; // Parent not found, treat as root
	}

	// Recursive depth calculation
	return 1 + CalculateNodeDepth(*(*ParentPtr)->Data, NodeMap, Visited);
}

void SDialogueTableEditor::InitializeColumnFilters()
{
	TArray<FDialogueTableColumn> Columns = GetDialogueTableColumns();
	for (const FDialogueTableColumn& Col : Columns)
	{
		FColumnFilterState& State = ColumnFilters.Add(Col.ColumnId);
		State.DropdownOptions.Add(MakeShared<FString>(TEXT(""))); // "(All)" option
	}
}

void SDialogueTableEditor::UpdateColumnFilterOptions()
{
	TArray<FDialogueTableColumn> Columns = GetDialogueTableColumns();

	for (const FDialogueTableColumn& Col : Columns)
	{
		FColumnFilterState* State = ColumnFilters.Find(Col.ColumnId);
		if (!State) continue;

		// Remember previous selections
		TSet<FString> PreviousSelections = State->SelectedValues;

		State->DropdownOptions.Empty();
		State->DropdownOptions.Add(MakeShared<FString>(TEXT(""))); // "(All)" placeholder

		TSet<FString> UniqueValues;
		bool bHasEmptyValues = false;
		for (const auto& RowEx : AllRows)
		{
			FString Value = GetColumnValue(*RowEx, Col.ColumnId);
			if (Value.IsEmpty())
			{
				bHasEmptyValues = true;
			}
			else
			{
				UniqueValues.Add(Value);
			}
		}

		// Add "(Empty)" option if there are empty values
		if (bHasEmptyValues)
		{
			State->DropdownOptions.Add(MakeShared<FString>(TEXT("(Empty)")));
		}

		for (const FString& Value : UniqueValues)
		{
			State->DropdownOptions.Add(MakeShared<FString>(Value));
		}

		// Restore valid selections (remove any that no longer exist)
		TSet<FString> ValidSelections;
		for (const FString& PrevSel : PreviousSelections)
		{
			for (const auto& Option : State->DropdownOptions)
			{
				if (Option.IsValid() && *Option == PrevSel)
				{
					ValidSelections.Add(PrevSel);
					break;
				}
			}
		}
		State->SelectedValues = ValidSelections;
	}
}

FString SDialogueTableEditor::GetColumnValue(const FDialogueTableRowEx& RowEx, FName ColumnId) const
{
	if (!RowEx.Data.IsValid()) return TEXT("");
	const FDialogueTableRow& Row = *RowEx.Data;

	if (ColumnId == TEXT("Seq")) return RowEx.SeqDisplay;
	if (ColumnId == TEXT("DialogueID")) return Row.DialogueID.ToString();
	if (ColumnId == TEXT("NodeID")) return Row.NodeID.ToString();
	if (ColumnId == TEXT("NodeType")) return Row.NodeType == EDialogueTableNodeType::NPC ? TEXT("NPC") : TEXT("Player");
	if (ColumnId == TEXT("Speaker"))
	{
		// For Player nodes, show "Player" if Speaker is empty
		if (Row.NodeType == EDialogueTableNodeType::Player && (Row.Speaker.IsNone() || Row.Speaker.ToString().IsEmpty()))
		{
			return TEXT("Player");
		}
		return Row.Speaker.ToString();
	}
	if (ColumnId == TEXT("Text")) return Row.Text;
	if (ColumnId == TEXT("OptionText")) return Row.OptionText;
	if (ColumnId == TEXT("ParentNodeID")) return Row.ParentNodeID.ToString();
	if (ColumnId == TEXT("NextNodeIDs"))
	{
		FString Result;
		for (int32 i = 0; i < Row.NextNodeIDs.Num(); i++)
		{
			if (i > 0) Result += TEXT(", ");
			Result += Row.NextNodeIDs[i].ToString();
		}
		return Result;
	}
	if (ColumnId == TEXT("Skippable")) return Row.bSkippable ? TEXT("Yes") : TEXT("No");
	if (ColumnId == TEXT("Notes")) return Row.Notes;
	return TEXT("");
}

void SDialogueTableEditor::OnColumnTextFilterChanged(FName ColumnId, const FText& NewText)
{
	FColumnFilterState* State = ColumnFilters.Find(ColumnId);
	if (State)
	{
		State->TextFilter = NewText.ToString();
		ApplyFilters();
	}
}

void SDialogueTableEditor::OnColumnDropdownFilterChanged(FName ColumnId, TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	// Legacy function - kept for compatibility but multi-select uses SelectedValues directly
	FColumnFilterState* State = ColumnFilters.Find(ColumnId);
	if (State && NewValue.IsValid() && !NewValue->IsEmpty())
	{
		State->SelectedValues.Empty();
		State->SelectedValues.Add(*NewValue);
		ApplyFilters();
	}
}

void SDialogueTableEditor::ApplyFilters()
{
	DisplayedRows.Empty();

	for (TSharedPtr<FDialogueTableRowEx>& RowEx : AllRows)
	{
		bool bPassesAllFilters = true;

		for (const auto& FilterPair : ColumnFilters)
		{
			const FName& ColumnId = FilterPair.Key;
			const FColumnFilterState& FilterState = FilterPair.Value;

			FString ColumnValue = GetColumnValue(*RowEx, ColumnId);

			// Text filter
			if (!FilterState.TextFilter.IsEmpty())
			{
				if (!ColumnValue.ToLower().Contains(FilterState.TextFilter.ToLower()))
				{
					bPassesAllFilters = false;
					break;
				}
			}

			// Multi-select dropdown filter (empty = all pass, otherwise OR logic)
			if (FilterState.SelectedValues.Num() > 0)
			{
				bool bMatchesAny = false;
				for (const FString& SelectedValue : FilterState.SelectedValues)
				{
					// Special handling for "(Empty)" filter
					if (SelectedValue == TEXT("(Empty)"))
					{
						if (ColumnValue.IsEmpty())
						{
							bMatchesAny = true;
							break;
						}
					}
					else if (ColumnValue == SelectedValue)
					{
						bMatchesAny = true;
						break;
					}
				}
				if (!bMatchesAny)
				{
					bPassesAllFilters = false;
					break;
				}
			}
		}

		if (bPassesAllFilters)
		{
			DisplayedRows.Add(RowEx);
		}
	}

	ApplySorting();
	RefreshList();
}

void SDialogueTableEditor::ApplySorting()
{
	if (SortColumn == NAME_None || SortMode == EColumnSortMode::None)
	{
		return;
	}

	bool bAscending = (SortMode == EColumnSortMode::Ascending);

	DisplayedRows.Sort([this, bAscending](const TSharedPtr<FDialogueTableRowEx>& A, const TSharedPtr<FDialogueTableRowEx>& B)
	{
		FString ValueA = GetColumnValue(*A, SortColumn);
		FString ValueB = GetColumnValue(*B, SortColumn);
		return bAscending ? (ValueA < ValueB) : (ValueA > ValueB);
	});
}

void SDialogueTableEditor::ApplyFlowOrder()
{
	// Reset sort state
	SortColumn = NAME_None;
	SortMode = EColumnSortMode::None;

	// First apply filters to get the filtered set
	DisplayedRows.Empty();
	for (TSharedPtr<FDialogueTableRowEx>& RowEx : AllRows)
	{
		bool bPassesAllFilters = true;

		for (const auto& FilterPair : ColumnFilters)
		{
			const FName& ColumnId = FilterPair.Key;
			const FColumnFilterState& FilterState = FilterPair.Value;

			FString ColumnValue = GetColumnValue(*RowEx, ColumnId);

			// Text filter
			if (!FilterState.TextFilter.IsEmpty())
			{
				if (!ColumnValue.ToLower().Contains(FilterState.TextFilter.ToLower()))
				{
					bPassesAllFilters = false;
					break;
				}
			}

			// Multi-select dropdown filter (empty = all pass, otherwise OR logic)
			if (FilterState.SelectedValues.Num() > 0)
			{
				bool bMatchesAny = false;
				for (const FString& SelectedValue : FilterState.SelectedValues)
				{
					// Special handling for "(Empty)" filter
					if (SelectedValue == TEXT("(Empty)"))
					{
						if (ColumnValue.IsEmpty())
						{
							bMatchesAny = true;
							break;
						}
					}
					else if (ColumnValue == SelectedValue)
					{
						bMatchesAny = true;
						break;
					}
				}
				if (!bMatchesAny)
				{
					bPassesAllFilters = false;
					break;
				}
			}
		}

		if (bPassesAllFilters)
		{
			DisplayedRows.Add(RowEx);
		}
	}

	// Then sort by DialogueID first, then by Sequence (flow order)
	DisplayedRows.Sort([](const TSharedPtr<FDialogueTableRowEx>& A, const TSharedPtr<FDialogueTableRowEx>& B)
	{
		if (!A->Data.IsValid() || !B->Data.IsValid()) return false;

		// First by DialogueID
		FString DialogueA = A->Data->DialogueID.ToString();
		FString DialogueB = B->Data->DialogueID.ToString();
		if (DialogueA != DialogueB)
		{
			return DialogueA < DialogueB;
		}

		// Then by Sequence (tree traversal order)
		return A->Sequence < B->Sequence;
	});

	RefreshList();
}

FReply SDialogueTableEditor::OnResetOrderClicked()
{
	ApplyFlowOrder();
	return FReply::Handled();
}

void SDialogueTableEditor::MarkDirty()
{
	SyncToTableData();
	if (TableData)
	{
		TableData->MarkPackageDirty();
	}
}

void SDialogueTableEditor::OnRowModified()
{
	CalculateSequences();
	UpdateColumnFilterOptions();
	MarkDirty();
}

//=============================================================================
// Actions
//=============================================================================

FReply SDialogueTableEditor::OnAddRowClicked()
{
	TSharedPtr<FDialogueTableRowEx> NewRowEx = MakeShared<FDialogueTableRowEx>();
	NewRowEx->Data = MakeShared<FDialogueTableRow>();

	TArray<TSharedPtr<FDialogueTableRowEx>> Selected = GetSelectedRows();
	int32 InsertIndex = AllRows.Num(); // Default: add to end

	if (Selected.Num() > 0)
	{
		// Smart Add: Insert below selected row with auto-populated fields
		TSharedPtr<FDialogueTableRowEx> SelectedRow = Selected[0];
		FDialogueTableRow* ParentData = SelectedRow->Data.Get();

		if (ParentData)
		{
			// Copy DialogueID from selected row
			NewRowEx->Data->DialogueID = ParentData->DialogueID;

			// Set ParentNodeID to selected row's NodeID
			NewRowEx->Data->ParentNodeID = ParentData->NodeID;

			// Toggle NodeType (NPC -> Player, Player -> NPC)
			NewRowEx->Data->NodeType = (ParentData->NodeType == EDialogueTableNodeType::NPC)
				? EDialogueTableNodeType::Player
				: EDialogueTableNodeType::NPC;

			// For NPC nodes, find and copy the nearest NPC speaker from ancestors
			if (NewRowEx->Data->NodeType == EDialogueTableNodeType::NPC)
			{
				// Build node map for this dialogue
				TMap<FName, TSharedPtr<FDialogueTableRow>> NodeMap;
				for (const auto& Row : AllRows)
				{
					if (Row->Data.IsValid() && Row->Data->DialogueID == ParentData->DialogueID)
					{
						NodeMap.Add(Row->Data->NodeID, Row->Data);
					}
				}

				// Walk up the tree to find nearest NPC speaker
				FName CurrentNodeID = ParentData->NodeID;
				TSet<FName> Visited;
				while (!CurrentNodeID.IsNone() && !Visited.Contains(CurrentNodeID))
				{
					Visited.Add(CurrentNodeID);
					TSharedPtr<FDialogueTableRow>* NodePtr = NodeMap.Find(CurrentNodeID);
					if (NodePtr && NodePtr->IsValid())
					{
						FDialogueTableRow* Node = NodePtr->Get();
						if (Node->NodeType == EDialogueTableNodeType::NPC && !Node->Speaker.IsNone())
						{
							NewRowEx->Data->Speaker = Node->Speaker;
							break;
						}
						CurrentNodeID = Node->ParentNodeID;
					}
					else
					{
						break;
					}
				}
			}

			// Generate unique NodeID
			FString BaseID = TEXT("new_node");
			int32 Counter = 1;
			FName NewNodeID = FName(*FString::Printf(TEXT("%s_%d"), *BaseID, Counter));

			// Check for uniqueness
			TSet<FName> ExistingIDs;
			for (const auto& Row : AllRows)
			{
				if (Row->Data.IsValid())
				{
					ExistingIDs.Add(Row->Data->NodeID);
				}
			}
			while (ExistingIDs.Contains(NewNodeID))
			{
				Counter++;
				NewNodeID = FName(*FString::Printf(TEXT("%s_%d"), *BaseID, Counter));
			}
			NewRowEx->Data->NodeID = NewNodeID;

			// Prepend new NodeID to parent's NextNodeIDs (so it appears right after parent in flow)
			if (!ParentData->NextNodeIDs.Contains(NewNodeID))
			{
				ParentData->NextNodeIDs.Insert(NewNodeID, 0);
			}

			// Find insert position (after selected row)
			InsertIndex = AllRows.IndexOfByKey(SelectedRow);
			if (InsertIndex != INDEX_NONE)
			{
				InsertIndex++; // Insert after selected
			}
			else
			{
				InsertIndex = AllRows.Num();
			}
		}
	}
	else
	{
		// No selection: Pre-fill DialogueID from filter if set
		FColumnFilterState* DialogueFilter = ColumnFilters.Find(TEXT("DialogueID"));
		if (DialogueFilter && DialogueFilter->SelectedValues.Num() == 1)
		{
			// Use first selected value if only one is selected
			for (const FString& Val : DialogueFilter->SelectedValues)
			{
				NewRowEx->Data->DialogueID = FName(*Val);
				break;
			}
		}

		// Generate unique NodeID for root node
		FString BaseID = TEXT("root");
		int32 Counter = 1;
		FName NewNodeID = FName(*BaseID);

		TSet<FName> ExistingIDs;
		for (const auto& Row : AllRows)
		{
			if (Row->Data.IsValid())
			{
				ExistingIDs.Add(Row->Data->NodeID);
			}
		}
		if (ExistingIDs.Contains(NewNodeID))
		{
			while (ExistingIDs.Contains(NewNodeID))
			{
				Counter++;
				NewNodeID = FName(*FString::Printf(TEXT("%s_%d"), *BaseID, Counter));
			}
		}
		NewRowEx->Data->NodeID = NewNodeID;
	}

	// Insert at calculated position in AllRows
	if (InsertIndex >= 0 && InsertIndex < AllRows.Num())
	{
		AllRows.Insert(NewRowEx, InsertIndex);
	}
	else
	{
		AllRows.Add(NewRowEx);
	}

	// Also insert into DisplayedRows at the same relative position (don't re-sort)
	if (Selected.Num() > 0)
	{
		// Find position in DisplayedRows (after selected row)
		int32 DisplayIndex = DisplayedRows.IndexOfByKey(Selected[0]);
		if (DisplayIndex != INDEX_NONE)
		{
			DisplayedRows.Insert(NewRowEx, DisplayIndex + 1);
		}
		else
		{
			DisplayedRows.Add(NewRowEx);
		}
	}
	else
	{
		DisplayedRows.Add(NewRowEx);
	}

	CalculateSequences();
	UpdateColumnFilterOptions();
	RefreshList();  // Just refresh, don't re-sort
	MarkDirty();

	// Select the new row
	if (ListView.IsValid())
	{
		ListView->SetSelection(NewRowEx);
		ListView->RequestScrollIntoView(NewRowEx);
	}

	return FReply::Handled();
}

FReply SDialogueTableEditor::OnDeleteRowsClicked()
{
	TArray<TSharedPtr<FDialogueTableRowEx>> Selected = GetSelectedRows();

	if (Selected.Num() == 0)
	{
		return FReply::Handled();
	}

	// Show confirmation prompt
	FText Message = FText::Format(
		LOCTEXT("DeleteConfirm", "Are you sure you want to delete {0} node(s)?\nChildren will be re-parented to the grandparent."),
		FText::AsNumber(Selected.Num())
	);

	if (FMessageDialog::Open(EAppMsgType::YesNo, Message) != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	// Build node map for finding parents and children
	TMap<FName, TSharedPtr<FDialogueTableRowEx>> NodeMap;
	for (const auto& Row : AllRows)
	{
		if (Row->Data.IsValid())
		{
			FName UniqueKey = FName(*FString::Printf(TEXT("%s.%s"),
				*Row->Data->DialogueID.ToString(), *Row->Data->NodeID.ToString()));
			NodeMap.Add(UniqueKey, Row);
		}
	}

	for (const TSharedPtr<FDialogueTableRowEx>& RowEx : Selected)
	{
		if (!RowEx->Data.IsValid()) continue;

		FName DeletedNodeID = RowEx->Data->NodeID;
		FName GrandparentID = RowEx->Data->ParentNodeID;
		FName DialogueID = RowEx->Data->DialogueID;

		// Find parent and remove this node from parent's NextNodeIDs
		TSharedPtr<FDialogueTableRowEx>* ParentPtr = nullptr;
		if (!GrandparentID.IsNone())
		{
			FName ParentKey = FName(*FString::Printf(TEXT("%s.%s"),
				*DialogueID.ToString(), *GrandparentID.ToString()));
			ParentPtr = NodeMap.Find(ParentKey);
			if (ParentPtr && (*ParentPtr)->Data.IsValid())
			{
				(*ParentPtr)->Data->NextNodeIDs.Remove(DeletedNodeID);
			}
		}

		// Re-parent children: update their ParentNodeID to grandparent, add them to grandparent's NextNodeIDs
		for (const auto& Row : AllRows)
		{
			if (Row->Data.IsValid() &&
				Row->Data->DialogueID == DialogueID &&
				Row->Data->ParentNodeID == DeletedNodeID)
			{
				// This row's parent is being deleted - re-parent to grandparent
				Row->Data->ParentNodeID = GrandparentID;

				// Add this child to grandparent's NextNodeIDs (if grandparent exists)
				if (ParentPtr && (*ParentPtr)->Data.IsValid())
				{
					if (!(*ParentPtr)->Data->NextNodeIDs.Contains(Row->Data->NodeID))
					{
						(*ParentPtr)->Data->NextNodeIDs.Add(Row->Data->NodeID);
					}
				}
			}
		}

		// Remove from AllRows and DisplayedRows
		AllRows.Remove(RowEx);
		DisplayedRows.Remove(RowEx);
	}

	CalculateSequences();
	UpdateColumnFilterOptions();
	RefreshList();
	MarkDirty();
	return FReply::Handled();
}

FReply SDialogueTableEditor::OnDeleteBranchClicked()
{
	TArray<TSharedPtr<FDialogueTableRowEx>> Selected = GetSelectedRows();

	if (Selected.Num() == 0)
	{
		return FReply::Handled();
	}

	// Collect all nodes to delete (selected + all descendants)
	TSet<FName> NodesToDelete;  // DialogueID.NodeID keys

	// Build node map
	TMap<FName, TSharedPtr<FDialogueTableRowEx>> NodeMap;
	for (const auto& Row : AllRows)
	{
		if (Row->Data.IsValid())
		{
			FName UniqueKey = FName(*FString::Printf(TEXT("%s.%s"),
				*Row->Data->DialogueID.ToString(), *Row->Data->NodeID.ToString()));
			NodeMap.Add(UniqueKey, Row);
		}
	}

	// Recursive function to collect node and all descendants
	TFunction<void(FName DialogueID, FName NodeID)> CollectBranch = [&](FName DialogueID, FName NodeID)
	{
		FName Key = FName(*FString::Printf(TEXT("%s.%s"), *DialogueID.ToString(), *NodeID.ToString()));
		if (NodesToDelete.Contains(Key)) return;  // Already collected
		NodesToDelete.Add(Key);

		// Find all children (nodes whose ParentNodeID matches this NodeID)
		for (const auto& Row : AllRows)
		{
			if (Row->Data.IsValid() &&
				Row->Data->DialogueID == DialogueID &&
				Row->Data->ParentNodeID == NodeID)
			{
				CollectBranch(DialogueID, Row->Data->NodeID);
			}
		}
	};

	// Collect all branches starting from selected nodes
	for (const TSharedPtr<FDialogueTableRowEx>& RowEx : Selected)
	{
		if (RowEx->Data.IsValid())
		{
			CollectBranch(RowEx->Data->DialogueID, RowEx->Data->NodeID);
		}
	}

	// Show confirmation with count
	FText Message = FText::Format(
		LOCTEXT("DeleteBranchConfirm", "Are you sure you want to delete {0} node(s) and ALL their descendants?\n(Total nodes to delete: {1})"),
		FText::AsNumber(Selected.Num()),
		FText::AsNumber(NodesToDelete.Num())
	);

	if (FMessageDialog::Open(EAppMsgType::YesNo, Message) != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	// Remove from parent's NextNodeIDs for root nodes of deletion
	for (const TSharedPtr<FDialogueTableRowEx>& RowEx : Selected)
	{
		if (!RowEx->Data.IsValid()) continue;

		if (!RowEx->Data->ParentNodeID.IsNone())
		{
			FName ParentKey = FName(*FString::Printf(TEXT("%s.%s"),
				*RowEx->Data->DialogueID.ToString(), *RowEx->Data->ParentNodeID.ToString()));
			TSharedPtr<FDialogueTableRowEx>* ParentPtr = NodeMap.Find(ParentKey);
			if (ParentPtr && (*ParentPtr)->Data.IsValid())
			{
				(*ParentPtr)->Data->NextNodeIDs.Remove(RowEx->Data->NodeID);
			}
		}
	}

	// Delete all collected nodes
	AllRows.RemoveAll([&NodesToDelete](const TSharedPtr<FDialogueTableRowEx>& Row)
	{
		if (!Row->Data.IsValid()) return false;
		FName Key = FName(*FString::Printf(TEXT("%s.%s"),
			*Row->Data->DialogueID.ToString(), *Row->Data->NodeID.ToString()));
		return NodesToDelete.Contains(Key);
	});

	DisplayedRows.RemoveAll([&NodesToDelete](const TSharedPtr<FDialogueTableRowEx>& Row)
	{
		if (!Row->Data.IsValid()) return false;
		FName Key = FName(*FString::Printf(TEXT("%s.%s"),
			*Row->Data->DialogueID.ToString(), *Row->Data->NodeID.ToString()));
		return NodesToDelete.Contains(Key);
	});

	CalculateSequences();
	UpdateColumnFilterOptions();
	RefreshList();
	MarkDirty();
	return FReply::Handled();
}

FReply SDialogueTableEditor::OnClearFiltersClicked()
{
	// Clear all text and dropdown filters
	for (auto& FilterPair : ColumnFilters)
	{
		FilterPair.Value.TextFilter.Empty();
		FilterPair.Value.SelectedValues.Empty();  // Clear multi-select
	}

	// Apply filters to refresh the view
	ApplyFlowOrder();
	return FReply::Handled();
}

FReply SDialogueTableEditor::OnValidateClicked()
{
	SyncToTableData();

	if (!TableData || TableData->Rows.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoData", "No dialogue data to validate."));
		return FReply::Handled();
	}

	FDialogueValidationResult Result = FDialogueTableValidator::ValidateAll(TableData->Rows);

	if (Result.Issues.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ValidationSuccess", "All dialogues are valid!"));
	}
	else
	{
		FString Message = FString::Printf(TEXT("Found %d errors, %d warnings:\n\n"),
			Result.GetErrorCount(), Result.GetWarningCount());

		for (int32 i = 0; i < FMath::Min(Result.Issues.Num(), 10); i++)
		{
			Message += Result.Issues[i].ToString() + TEXT("\n");
		}

		if (Result.Issues.Num() > 10)
		{
			Message += FString::Printf(TEXT("\n... and %d more issues"), Result.Issues.Num() - 10);
		}

		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));
	}

	return FReply::Handled();
}

FReply SDialogueTableEditor::OnGenerateClicked()
{
	SyncToTableData();

	if (!TableData || TableData->Rows.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoDataGenerate", "No dialogue data to generate."));
		return FReply::Handled();
	}

	FDialogueValidationResult ValidationResult = FDialogueTableValidator::ValidateAll(TableData->Rows);
	if (ValidationResult.HasErrors())
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("ValidationErrors", "Cannot generate: validation errors found. Please run Validate to see details."));
		return FReply::Handled();
	}

	TMap<FName, FManifestDialogueBlueprintDefinition> Definitions =
		FDialogueTableConverter::ConvertRowsToManifest(TableData->Rows);

	FString Message = FString::Printf(TEXT("Ready to generate %d dialogue(s):\n\n"), Definitions.Num());
	for (const auto& Pair : Definitions)
	{
		Message += FString::Printf(TEXT("- %s (%d nodes)\n"),
			*Pair.Key.ToString(),
			Pair.Value.DialogueTree.Nodes.Num());
	}

	Message += TEXT("\nGeneration would use FDialogueBlueprintGenerator.\n");
	Message += TEXT("(Full generation integration coming in next update)");

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));

	return FReply::Handled();
}

FReply SDialogueTableEditor::OnExportCSVClicked()
{
	SyncToTableData();

	TArray<FString> OutFiles;
	FDesktopPlatformModule::Get()->SaveFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Export Dialogue Table"),
		FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_EXPORT),
		TEXT("DialogueTable.csv"),
		TEXT("CSV Files (*.csv)|*.csv"),
		EFileDialogFlags::None,
		OutFiles
	);

	if (OutFiles.Num() == 0)
	{
		return FReply::Handled();
	}

	ExportToCSV(OutFiles[0]);
	FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_EXPORT, FPaths::GetPath(OutFiles[0]));

	return FReply::Handled();
}

FReply SDialogueTableEditor::OnImportCSVClicked()
{
	TArray<FString> OutFiles;
	FDesktopPlatformModule::Get()->OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Import Dialogue Table"),
		FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_IMPORT),
		TEXT(""),
		TEXT("CSV Files (*.csv)|*.csv"),
		EFileDialogFlags::None,
		OutFiles
	);

	if (OutFiles.Num() == 0)
	{
		return FReply::Handled();
	}

	if (ImportFromCSV(OutFiles[0]))
	{
		FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_IMPORT, FPaths::GetPath(OutFiles[0]));
		if (TableData)
		{
			TableData->SourceCSVPath = OutFiles[0];
		}
	}

	return FReply::Handled();
}

//=============================================================================
// CSV Import/Export with proper RFC 4180 parsing
//=============================================================================

TArray<FString> SDialogueTableEditor::ParseCSVLine(const FString& Line)
{
	TArray<FString> Fields;
	FString CurrentField;
	bool bInQuotes = false;

	for (int32 i = 0; i < Line.Len(); i++)
	{
		TCHAR c = Line[i];

		if (bInQuotes)
		{
			if (c == TEXT('"'))
			{
				// Check for escaped quote ""
				if (i + 1 < Line.Len() && Line[i + 1] == TEXT('"'))
				{
					CurrentField += TEXT('"');
					i++; // Skip next quote
				}
				else
				{
					bInQuotes = false;
				}
			}
			else
			{
				CurrentField += c;
			}
		}
		else
		{
			if (c == TEXT('"'))
			{
				bInQuotes = true;
			}
			else if (c == TEXT(','))
			{
				Fields.Add(CurrentField);
				CurrentField.Empty();
			}
			else
			{
				CurrentField += c;
			}
		}
	}

	// Add last field
	Fields.Add(CurrentField);

	return Fields;
}

bool SDialogueTableEditor::ExportToCSV(const FString& FilePath)
{
	FString CSV;

	// Header (11 columns)
	CSV += TEXT("DialogueID,NodeID,NodeType,Speaker,Text,OptionText,ParentNodeID,NextNodeIDs,Skippable,Notes\n");

	// Rows
	for (const TSharedPtr<FDialogueTableRowEx>& RowEx : AllRows)
	{
		if (!RowEx->Data.IsValid()) continue;
		const FDialogueTableRow& Row = *RowEx->Data;

		FString NextNodes;
		for (int32 i = 0; i < Row.NextNodeIDs.Num(); i++)
		{
			if (i > 0) NextNodes += TEXT(";"); // Use semicolon for NextNodeIDs separator
			NextNodes += Row.NextNodeIDs[i].ToString();
		}

		CSV += FString::Printf(TEXT("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n"),
			*EscapeCSVField(Row.DialogueID.ToString()),
			*EscapeCSVField(Row.NodeID.ToString()),
			Row.NodeType == EDialogueTableNodeType::NPC ? TEXT("npc") : TEXT("player"),
			*EscapeCSVField(Row.Speaker.ToString()),
			*EscapeCSVField(Row.Text),
			*EscapeCSVField(Row.OptionText),
			*EscapeCSVField(Row.ParentNodeID.ToString()),
			*EscapeCSVField(NextNodes),
			Row.bSkippable ? TEXT("yes") : TEXT("no"),
			*EscapeCSVField(Row.Notes)
		);
	}

	if (FFileHelper::SaveStringToFile(CSV, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(LOCTEXT("ExportSuccess", "Exported {0} rows to:\n{1}"),
				FText::AsNumber(AllRows.Num()),
				FText::FromString(FilePath)));
		return true;
	}

	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ExportFailed", "Failed to export CSV file."));
	return false;
}

bool SDialogueTableEditor::ImportFromCSV(const FString& FilePath)
{
	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ImportFailed", "Failed to read CSV file."));
		return false;
	}

	TArray<FString> Lines;
	FileContent.ParseIntoArrayLines(Lines);

	if (Lines.Num() < 2)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ImportEmpty", "CSV file is empty or has no data rows."));
		return false;
	}

	AllRows.Empty();

	// Skip header, parse data rows
	for (int32 i = 1; i < Lines.Num(); i++)
	{
		FString Line = Lines[i].TrimStartAndEnd();
		if (Line.IsEmpty()) continue;

		TArray<FString> Fields = ParseCSVLine(Line);

		if (Fields.Num() >= 8)
		{
			TSharedPtr<FDialogueTableRowEx> RowEx = MakeShared<FDialogueTableRowEx>();
			RowEx->Data = MakeShared<FDialogueTableRow>();
			FDialogueTableRow& Row = *RowEx->Data;

			Row.DialogueID = FName(*Fields[0].TrimStartAndEnd());
			Row.NodeID = FName(*Fields[1].TrimStartAndEnd());
			Row.NodeType = Fields[2].ToLower().Contains(TEXT("player"))
				? EDialogueTableNodeType::Player
				: EDialogueTableNodeType::NPC;
			Row.Speaker = FName(*Fields[3].TrimStartAndEnd());
			Row.Text = Fields[4].TrimStartAndEnd();
			Row.OptionText = Fields[5].TrimStartAndEnd();
			Row.ParentNodeID = FName(*Fields[6].TrimStartAndEnd());

			// Parse NextNodeIDs - support both comma and semicolon separators
			FString NextNodes = Fields[7].TrimStartAndEnd();
			TArray<FString> NodeParts;
			// First try semicolon (new format), then comma (old format)
			if (NextNodes.Contains(TEXT(";")))
			{
				NextNodes.ParseIntoArray(NodeParts, TEXT(";"));
			}
			else
			{
				NextNodes.ParseIntoArray(NodeParts, TEXT(","));
			}
			for (FString& Part : NodeParts)
			{
				Part.TrimStartAndEndInline();
				if (!Part.IsEmpty())
				{
					Row.NextNodeIDs.Add(FName(*Part));
				}
			}

			// Skippable column (optional - default true)
			if (Fields.Num() >= 9)
			{
				FString SkipVal = Fields[8].TrimStartAndEnd().ToLower();
				Row.bSkippable = !SkipVal.Equals(TEXT("no")) && !SkipVal.Equals(TEXT("false")) && !SkipVal.Equals(TEXT("0"));
			}

			// Notes column (optional)
			if (Fields.Num() >= 10)
			{
				Row.Notes = Fields[9].TrimStartAndEnd();
			}

			AllRows.Add(RowEx);
		}
	}

	SyncToTableData();
	CalculateSequences();
	UpdateColumnFilterOptions();
	ApplyFlowOrder();

	FMessageDialog::Open(EAppMsgType::Ok,
		FText::Format(LOCTEXT("ImportSuccess", "Imported {0} dialogue nodes from CSV."),
			FText::AsNumber(AllRows.Num())));

	return true;
}

FString SDialogueTableEditor::EscapeCSVField(const FString& Field)
{
	if (Field.Contains(TEXT(",")) || Field.Contains(TEXT("\"")) || Field.Contains(TEXT("\n")) || Field.Contains(TEXT(";")))
	{
		return TEXT("\"") + Field.Replace(TEXT("\""), TEXT("\"\"")) + TEXT("\"");
	}
	return Field;
}

//=============================================================================
// SDialogueTableEditorWindow
//=============================================================================

void SDialogueTableEditorWindow::Construct(const FArguments& InArgs)
{
	CurrentTableData = GetOrCreateTableData();

	ChildSlot
	[
		SNew(SVerticalBox)

		// Menu bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			BuildMenuBar()
		]

		// Main editor
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(TableEditor, SDialogueTableEditor)
				.TableData(CurrentTableData)
		]
	];
}

TSharedRef<SWidget> SDialogueTableEditorWindow::BuildMenuBar()
{
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("New", "New"))
				.OnClicked_Lambda([this]() { OnNewTable(); return FReply::Handled(); })
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("Open", "Open"))
				.OnClicked_Lambda([this]() { OnOpenTable(); return FReply::Handled(); })
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("Save", "Save"))
				.OnClicked_Lambda([this]() { OnSaveTable(); return FReply::Handled(); })
		];
}

void SDialogueTableEditorWindow::OnNewTable()
{
	CurrentTableData = GetOrCreateTableData();
	CurrentTableData->Rows.Empty();
	CurrentTableData->SourceCSVPath.Empty();

	if (TableEditor.IsValid())
	{
		TableEditor->SetTableData(CurrentTableData);
	}
}

void SDialogueTableEditorWindow::OnOpenTable()
{
}

void SDialogueTableEditorWindow::OnSaveTable()
{
	if (CurrentTableData)
	{
		CurrentTableData->MarkPackageDirty();

		UPackage* Package = CurrentTableData->GetOutermost();
		FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage::SavePackage(Package, CurrentTableData, *PackageFileName, SaveArgs);

		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Saved", "Dialogue table saved."));
	}
}

UDialogueTableData* SDialogueTableEditorWindow::GetOrCreateTableData()
{
	FString AssetPath = TEXT("/Game/FatherCompanion/DialogueTableData");
	UDialogueTableData* Data = LoadObject<UDialogueTableData>(nullptr, *AssetPath);

	if (!Data)
	{
		FString PackagePath = TEXT("/Game/FatherCompanion");
		FString AssetName = TEXT("DialogueTableData");

		UPackage* Package = CreatePackage(*FString::Printf(TEXT("%s/%s"), *PackagePath, *AssetName));
		Data = NewObject<UDialogueTableData>(Package, *AssetName, RF_Public | RF_Standalone);

		FAssetRegistryModule::AssetCreated(Data);
		Data->MarkPackageDirty();
	}

	return Data;
}

#undef LOCTEXT_NAMESPACE
