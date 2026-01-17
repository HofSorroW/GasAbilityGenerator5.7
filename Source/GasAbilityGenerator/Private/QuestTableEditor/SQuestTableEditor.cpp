// SQuestTableEditor.cpp
// Quest Table Editor Implementation
// v4.8: Follows NPC/Dialogue table patterns
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#include "QuestTableEditor/SQuestTableEditor.h"
#include "QuestTableConverter.h"
#include "QuestTableValidator.h"
#include "GasAbilityGeneratorGenerators.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Views/STableRow.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/MessageDialog.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "QuestTableEditor"

//=============================================================================
// SQuestTableRow
//=============================================================================

void SQuestTableRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	RowDataEx = InArgs._RowData;
	OnRowModified = InArgs._OnRowModified;

	SMultiColumnTableRow<TSharedPtr<FQuestTableRowEx>>::Construct(
		FSuperRowType::FArguments(),
		InOwnerTable
	);
}

TSharedRef<SWidget> SQuestTableRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (!RowDataEx.IsValid() || !RowDataEx->Data.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	FQuestTableRow& Row = *RowDataEx->Data;

	if (ColumnName == TEXT("Status"))
	{
		return CreateStatusCell();
	}
	else if (ColumnName == TEXT("QuestName"))
	{
		return CreateQuestNameCell();
	}
	else if (ColumnName == TEXT("DisplayName"))
	{
		return CreateTextCell(Row.DisplayName, TEXT("Quest log name"));
	}
	else if (ColumnName == TEXT("StateID"))
	{
		return CreateTextCell(Row.StateID, TEXT("Start, Gathering, Complete..."));
	}
	else if (ColumnName == TEXT("StateType"))
	{
		return CreateStateTypeCell();
	}
	else if (ColumnName == TEXT("Description"))
	{
		return CreateTextCell(Row.Description, TEXT("Quest tracker text"));
	}
	else if (ColumnName == TEXT("ParentBranch"))
	{
		return CreateTextCell(Row.ParentBranch, TEXT("From state"));
	}
	else if (ColumnName == TEXT("Tasks"))
	{
		return CreateTokenCell(Row.Tasks, TEXT("BPT_FindItem(...)"));
	}
	else if (ColumnName == TEXT("Events"))
	{
		return CreateTokenCell(Row.Events, TEXT("NE_GiveXP(...)"));
	}
	else if (ColumnName == TEXT("Conditions"))
	{
		return CreateTokenCell(Row.Conditions, TEXT("NC_HasItem(...)"));
	}
	else if (ColumnName == TEXT("Rewards"))
	{
		return CreateTokenCell(Row.Rewards, TEXT("Reward(...)"));
	}
	else if (ColumnName == TEXT("Notes"))
	{
		return CreateNotesCell();
	}

	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SQuestTableRow::CreateStatusCell()
{
	FQuestTableRow& Row = *RowDataEx->Data;

	return SNew(SBox)
		.Padding(FMargin(4, 2))
		.HAlign(HAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(Row.GetStatusColor())
			.Padding(FMargin(4, 1))
			[
				SNew(STextBlock)
				.Text(FText::FromString(Row.GetStatusString()))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FLinearColor::White)
			]
		];
}

TSharedRef<SWidget> SQuestTableRow::CreateTextCell(FString& Value, const FString& Hint)
{
	return SNew(SBox)
		.Padding(FMargin(4, 2))
		[
			SNew(SEditableTextBox)
			.Text(FText::FromString(Value))
			.HintText(FText::FromString(Hint))
			.OnTextCommitted_Lambda([this, &Value](const FText& NewText, ETextCommit::Type) {
				Value = NewText.ToString();
				MarkModified();
			})
		];
}

TSharedRef<SWidget> SQuestTableRow::CreateStateTypeCell()
{
	FQuestTableRow& Row = *RowDataEx->Data;

	return SNew(SBox)
		.Padding(FMargin(4, 2))
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(Row.GetStateTypeColor())
			.Padding(FMargin(4, 1))
			[
				SNew(STextBlock)
				.Text(FText::FromString(Row.GetStateTypeString()))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			]
		];
}

TSharedRef<SWidget> SQuestTableRow::CreateTokenCell(FString& Value, const FString& Hint)
{
	return SNew(SBox)
		.Padding(FMargin(4, 2))
		[
			SNew(SEditableTextBox)
			.Text(FText::FromString(Value))
			.HintText(FText::FromString(Hint))
			.Font(FCoreStyle::GetDefaultFontStyle("Mono", 8))
			.OnTextCommitted_Lambda([this, &Value](const FText& NewText, ETextCommit::Type) {
				Value = NewText.ToString();
				MarkModified();
			})
		];
}

TSharedRef<SWidget> SQuestTableRow::CreateQuestNameCell()
{
	FQuestTableRow& Row = *RowDataEx->Data;

	// Show quest name with visual grouping indicator
	FLinearColor BgColor = RowDataEx->bIsFirstInQuest ?
		FLinearColor(0.15f, 0.15f, 0.2f) : FLinearColor(0.1f, 0.1f, 0.1f);

	return SNew(SBox)
		.Padding(FMargin(4, 2))
		[
			SNew(SBorder)
			.BorderBackgroundColor(BgColor)
			.Padding(FMargin(4, 2))
			[
				SNew(SEditableTextBox)
				.Text(FText::FromString(Row.QuestName))
				.HintText(LOCTEXT("QuestNameHint", "Quest name"))
				.Font(FCoreStyle::GetDefaultFontStyle(RowDataEx->bIsFirstInQuest ? "Bold" : "Regular", 9))
				.OnTextCommitted_Lambda([this, &Row](const FText& NewText, ETextCommit::Type) {
					Row.QuestName = NewText.ToString();
					MarkModified();
				})
			]
		];
}

TSharedRef<SWidget> SQuestTableRow::CreateNotesCell()
{
	FQuestTableRow& Row = *RowDataEx->Data;

	return SNew(SBox)
		.Padding(FMargin(4, 2))
		[
			SNew(SEditableTextBox)
			.Text(FText::FromString(Row.Notes))
			.HintText(LOCTEXT("NotesHint", "Designer notes"))
			.ToolTipText(FText::FromString(Row.Notes))
			.OnTextCommitted_Lambda([this, &Row](const FText& NewText, ETextCommit::Type) {
				Row.Notes = NewText.ToString();
				MarkModified();
			})
		];
}

void SQuestTableRow::MarkModified()
{
	if (RowDataEx.IsValid() && RowDataEx->Data.IsValid())
	{
		RowDataEx->Data->Status = EQuestTableRowStatus::Modified;
		RowDataEx->Data->InvalidateValidation();
	}
	OnRowModified.ExecuteIfBound();
}

//=============================================================================
// SQuestTableEditor
//=============================================================================

void SQuestTableEditor::Construct(const FArguments& InArgs)
{
	TableData = InArgs._TableData;
	OnDirtyStateChanged = InArgs._OnDirtyStateChanged;

	InitializeColumnFilters();
	SyncFromTableData();

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

		// Table content
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SAssignNew(ListView, SListView<TSharedPtr<FQuestTableRowEx>>)
				.ListItemsSource(&DisplayedRows)
				.OnGenerateRow(this, &SQuestTableEditor::OnGenerateRow)
				.OnSelectionChanged(this, &SQuestTableEditor::OnSelectionChanged)
				.SelectionMode(ESelectionMode::Multi)
				.HeaderRow(BuildHeaderRow())
			]
		]

		// Status bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			BuildStatusBar()
		]
	];

	ApplyFilters();
	UpdateStatusBar();
}

TSharedRef<SWidget> SQuestTableEditor::BuildToolbar()
{
	return SNew(SHorizontalBox)

		// Title
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(5, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Title", "Quest Table Editor"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNullWidget::NullWidget
		]

		// Add Quest
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("AddQuest", "+ Quest"))
			.ToolTipText(LOCTEXT("AddQuestTooltip", "Add a new quest with Start state"))
			.OnClicked(this, &SQuestTableEditor::OnAddQuestClicked)
		]

		// Add State
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("AddState", "+ State"))
			.ToolTipText(LOCTEXT("AddStateTooltip", "Add a new state row"))
			.OnClicked(this, &SQuestTableEditor::OnAddRowClicked)
		]

		// Delete
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("Delete", "Delete"))
			.OnClicked(this, &SQuestTableEditor::OnDeleteRowsClicked)
		]

		// Duplicate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("Duplicate", "Duplicate"))
			.OnClicked(this, &SQuestTableEditor::OnDuplicateRowClicked)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(10, 0, 2, 0)
		[
			SNew(SSeparator)
			.Orientation(Orient_Vertical)
		]

		// Validate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("Validate", "Validate"))
			.OnClicked(this, &SQuestTableEditor::OnValidateClicked)
		]

		// Generate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("Generate", "Generate"))
			.OnClicked(this, &SQuestTableEditor::OnGenerateClicked)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(10, 0, 2, 0)
		[
			SNew(SSeparator)
			.Orientation(Orient_Vertical)
		]

		// Export XLSX
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("ExportXLSX", "Export"))
			.OnClicked(this, &SQuestTableEditor::OnExportXLSXClicked)
		]

		// Import XLSX
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("ImportXLSX", "Import"))
			.OnClicked(this, &SQuestTableEditor::OnImportXLSXClicked)
		]

		// Save
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("Save", "Save"))
			.OnClicked(this, &SQuestTableEditor::OnSaveClicked)
		];
}

TSharedRef<SHeaderRow> SQuestTableEditor::BuildHeaderRow()
{
	TSharedRef<SHeaderRow> Header = SNew(SHeaderRow);
	TArray<FQuestTableColumn> Columns = GetQuestTableColumns();

	for (const FQuestTableColumn& Col : Columns)
	{
		Header->AddColumn(
			SHeaderRow::Column(Col.ColumnId)
			.DefaultLabel(Col.DisplayName)
			.FillWidth(Col.DefaultWidth)
			.SortMode(this, &SQuestTableEditor::GetColumnSortMode, Col.ColumnId)
			.OnSort(this, &SQuestTableEditor::OnColumnSortModeChanged)
			.HeaderContent()
			[
				BuildColumnHeaderContent(Col)
			]
		);
	}

	HeaderRow = Header;
	return Header;
}

TSharedRef<SWidget> SQuestTableEditor::BuildColumnHeaderContent(const FQuestTableColumn& Col)
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(STextBlock)
			.Text(Col.DisplayName)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(SSearchBox)
			.HintText(LOCTEXT("FilterHint", "Filter..."))
			.OnTextChanged_Lambda([this, ColumnId = Col.ColumnId](const FText& NewText) {
				OnColumnTextFilterChanged(ColumnId, NewText);
			})
		];
}

TSharedRef<SWidget> SQuestTableEditor::BuildStatusBar()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(5, 3))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5, 0)
			[
				SAssignNew(StatusTotalText, STextBlock)
				.Text(LOCTEXT("StatusTotal", "Total: 0"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5, 0)
			[
				SAssignNew(StatusQuestsText, STextBlock)
				.Text(LOCTEXT("StatusQuests", "Quests: 0"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5, 0)
			[
				SAssignNew(StatusShowingText, STextBlock)
				.Text(LOCTEXT("StatusShowing", "Showing: 0"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5, 0)
			[
				SAssignNew(StatusSelectedText, STextBlock)
				.Text(LOCTEXT("StatusSelected", "Selected: 0"))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5, 0)
			[
				SAssignNew(StatusValidationText, STextBlock)
				.Text(LOCTEXT("StatusValidation", ""))
			]
		];
}

void SQuestTableEditor::UpdateStatusBar()
{
	if (!TableData) return;

	int32 TotalRows = 0;
	TSet<FString> UniqueQuests;
	int32 ValidationErrors = 0;

	for (const FQuestTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			TotalRows++;
			if (!Row.QuestName.IsEmpty())
			{
				UniqueQuests.Add(Row.QuestName);
			}
			if (Row.ValidationState == EValidationState::Invalid)
			{
				ValidationErrors++;
			}
		}
	}

	if (StatusTotalText.IsValid())
	{
		StatusTotalText->SetText(FText::Format(LOCTEXT("StatusTotalFmt", "Total: {0}"), FText::AsNumber(TotalRows)));
	}
	if (StatusQuestsText.IsValid())
	{
		StatusQuestsText->SetText(FText::Format(LOCTEXT("StatusQuestsFmt", "Quests: {0}"), FText::AsNumber(UniqueQuests.Num())));
	}
	if (StatusShowingText.IsValid())
	{
		StatusShowingText->SetText(FText::Format(LOCTEXT("StatusShowingFmt", "Showing: {0}"), FText::AsNumber(DisplayedRows.Num())));
	}
	if (StatusSelectedText.IsValid())
	{
		StatusSelectedText->SetText(FText::Format(LOCTEXT("StatusSelectedFmt", "Selected: {0}"), FText::AsNumber(GetSelectedRows().Num())));
	}
	if (StatusValidationText.IsValid())
	{
		if (ValidationErrors > 0)
		{
			StatusValidationText->SetText(FText::Format(LOCTEXT("StatusValidationFmt", "Errors: {0}"), FText::AsNumber(ValidationErrors)));
			StatusValidationText->SetColorAndOpacity(FLinearColor(1.0f, 0.2f, 0.2f));
		}
		else
		{
			StatusValidationText->SetText(LOCTEXT("StatusValidationOK", "Valid"));
			StatusValidationText->SetColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.2f));
		}
	}
}

void SQuestTableEditor::InitializeColumnFilters()
{
	TArray<FQuestTableColumn> Columns = GetQuestTableColumns();
	for (const FQuestTableColumn& Col : Columns)
	{
		ColumnFilters.Add(Col.ColumnId, FQuestColumnFilterState());
	}
}

void SQuestTableEditor::SyncFromTableData()
{
	AllRows.Empty();
	if (!TableData) return;

	// Group rows by quest for visual indicator
	TMap<FString, int32> QuestFirstRowIndex;
	int32 Index = 0;

	for (FQuestTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			TSharedPtr<FQuestTableRowEx> RowEx = MakeShared<FQuestTableRowEx>();
			RowEx->Data = MakeShared<FQuestTableRow>(Row);

			// Check if this is first row of quest
			if (!Row.QuestName.IsEmpty() && !QuestFirstRowIndex.Contains(Row.QuestName))
			{
				QuestFirstRowIndex.Add(Row.QuestName, Index);
				RowEx->bIsFirstInQuest = true;
			}

			AllRows.Add(RowEx);
			Index++;
		}
	}
}

void SQuestTableEditor::RefreshList()
{
	SyncFromTableData();
	ApplyFilters();
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
	UpdateStatusBar();
}

void SQuestTableEditor::SetTableData(UQuestTableData* InTableData)
{
	TableData = InTableData;
	RefreshList();
}

TArray<TSharedPtr<FQuestTableRowEx>> SQuestTableEditor::GetSelectedRows() const
{
	TArray<TSharedPtr<FQuestTableRowEx>> Selected;
	if (ListView.IsValid())
	{
		Selected = ListView->GetSelectedItems();
	}
	return Selected;
}

TSharedRef<ITableRow> SQuestTableEditor::OnGenerateRow(TSharedPtr<FQuestTableRowEx> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SQuestTableRow, OwnerTable)
		.RowData(Item)
		.OnRowModified(FSimpleDelegate::CreateSP(this, &SQuestTableEditor::OnRowModified));
}

void SQuestTableEditor::OnSelectionChanged(TSharedPtr<FQuestTableRowEx> Item, ESelectInfo::Type SelectInfo)
{
	UpdateStatusBar();
}

EColumnSortMode::Type SQuestTableEditor::GetColumnSortMode(FName ColumnId) const
{
	if (SortColumn == ColumnId)
	{
		return SortMode;
	}
	return EColumnSortMode::None;
}

void SQuestTableEditor::OnColumnSortModeChanged(EColumnSortPriority::Type Priority, const FName& ColumnId, EColumnSortMode::Type InSortMode)
{
	SortColumn = ColumnId;
	SortMode = InSortMode;
	ApplySorting();
}

void SQuestTableEditor::OnColumnTextFilterChanged(FName ColumnId, const FText& NewText)
{
	if (FQuestColumnFilterState* State = ColumnFilters.Find(ColumnId))
	{
		State->TextFilter = NewText.ToString();
		ApplyFilters();
	}
}

void SQuestTableEditor::OnColumnDropdownFilterChanged(FName ColumnId, const FString& Value, bool bIsSelected)
{
	// TODO: Implement dropdown filter support
	ApplyFilters();
}

FReply SQuestTableEditor::OnClearFiltersClicked()
{
	for (auto& Pair : ColumnFilters)
	{
		Pair.Value.TextFilter.Empty();
		Pair.Value.SelectedValues.Empty();
	}
	ApplyFilters();
	return FReply::Handled();
}

FString SQuestTableEditor::GetColumnValue(const TSharedPtr<FQuestTableRowEx>& Row, FName ColumnId) const
{
	if (!Row.IsValid() || !Row->Data.IsValid()) return TEXT("");

	const FQuestTableRow& Data = *Row->Data;

	if (ColumnId == TEXT("Status")) return Data.GetStatusString();
	if (ColumnId == TEXT("QuestName")) return Data.QuestName;
	if (ColumnId == TEXT("DisplayName")) return Data.DisplayName;
	if (ColumnId == TEXT("StateID")) return Data.StateID;
	if (ColumnId == TEXT("StateType")) return Data.GetStateTypeString();
	if (ColumnId == TEXT("Description")) return Data.Description;
	if (ColumnId == TEXT("ParentBranch")) return Data.ParentBranch;
	if (ColumnId == TEXT("Tasks")) return Data.Tasks;
	if (ColumnId == TEXT("Events")) return Data.Events;
	if (ColumnId == TEXT("Conditions")) return Data.Conditions;
	if (ColumnId == TEXT("Rewards")) return Data.Rewards;
	if (ColumnId == TEXT("Notes")) return Data.Notes;

	return TEXT("");
}

void SQuestTableEditor::ApplyFilters()
{
	DisplayedRows.Empty();

	for (const TSharedPtr<FQuestTableRowEx>& Row : AllRows)
	{
		bool bPassesFilter = true;

		for (const auto& Pair : ColumnFilters)
		{
			const FQuestColumnFilterState& State = Pair.Value;
			if (!State.TextFilter.IsEmpty())
			{
				FString Value = GetColumnValue(Row, Pair.Key);
				if (!Value.Contains(State.TextFilter))
				{
					bPassesFilter = false;
					break;
				}
			}
		}

		if (bPassesFilter)
		{
			DisplayedRows.Add(Row);
		}
	}

	ApplySorting();

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
	UpdateStatusBar();
}

void SQuestTableEditor::ApplySorting()
{
	if (SortColumn.IsNone() || SortMode == EColumnSortMode::None)
	{
		ApplyQuestGrouping();
		return;
	}

	DisplayedRows.Sort([this](const TSharedPtr<FQuestTableRowEx>& A, const TSharedPtr<FQuestTableRowEx>& B) {
		FString ValueA = GetColumnValue(A, SortColumn);
		FString ValueB = GetColumnValue(B, SortColumn);

		if (SortMode == EColumnSortMode::Ascending)
		{
			return ValueA < ValueB;
		}
		return ValueA > ValueB;
	});
}

void SQuestTableEditor::ApplyQuestGrouping()
{
	// Sort by quest name, then by state (to group quest states together)
	DisplayedRows.Sort([](const TSharedPtr<FQuestTableRowEx>& A, const TSharedPtr<FQuestTableRowEx>& B) {
		if (!A->Data.IsValid() || !B->Data.IsValid()) return false;

		if (A->Data->QuestName != B->Data->QuestName)
		{
			return A->Data->QuestName < B->Data->QuestName;
		}

		// Put Start state first
		bool AIsStart = A->Data->StateID.Equals(TEXT("Start"), ESearchCase::IgnoreCase);
		bool BIsStart = B->Data->StateID.Equals(TEXT("Start"), ESearchCase::IgnoreCase);
		if (AIsStart != BIsStart) return AIsStart;

		return A->Data->StateID < B->Data->StateID;
	});
}

FReply SQuestTableEditor::OnAddRowClicked()
{
	if (!TableData) return FReply::Handled();

	FQuestTableRow& NewRow = TableData->AddRow();
	NewRow.QuestName = TEXT("NewQuest");
	NewRow.StateID = TEXT("NewState");

	MarkDirty();
	RefreshList();
	return FReply::Handled();
}

FReply SQuestTableEditor::OnAddQuestClicked()
{
	if (!TableData) return FReply::Handled();

	// Generate unique quest name
	int32 QuestNum = TableData->GetUniqueQuestNames().Num() + 1;
	FString QuestName = FString::Printf(TEXT("Quest%d"), QuestNum);

	// Add Start state
	FQuestTableRow& StartRow = TableData->AddRow();
	StartRow.QuestName = QuestName;
	StartRow.DisplayName = FString::Printf(TEXT("Quest %d"), QuestNum);
	StartRow.StateID = TEXT("Start");
	StartRow.StateType = EQuestStateType::Regular;
	StartRow.Description = TEXT("Quest started");

	// Add Complete state
	FQuestTableRow& CompleteRow = TableData->AddRow();
	CompleteRow.QuestName = QuestName;
	CompleteRow.StateID = TEXT("Complete");
	CompleteRow.StateType = EQuestStateType::Success;
	CompleteRow.Description = TEXT("Quest completed");
	CompleteRow.ParentBranch = TEXT("Start");

	MarkDirty();
	RefreshList();
	return FReply::Handled();
}

FReply SQuestTableEditor::OnDeleteRowsClicked()
{
	if (!TableData) return FReply::Handled();

	TArray<TSharedPtr<FQuestTableRowEx>> Selected = GetSelectedRows();
	if (Selected.Num() == 0) return FReply::Handled();

	// Soft delete
	for (const TSharedPtr<FQuestTableRowEx>& Row : Selected)
	{
		if (Row.IsValid() && Row->Data.IsValid())
		{
			// Find in TableData and mark deleted
			for (FQuestTableRow& TableRow : TableData->Rows)
			{
				if (TableRow.RowId == Row->Data->RowId)
				{
					TableRow.bDeleted = true;
					break;
				}
			}
		}
	}

	MarkDirty();
	RefreshList();
	return FReply::Handled();
}

FReply SQuestTableEditor::OnDuplicateRowClicked()
{
	if (!TableData) return FReply::Handled();

	TArray<TSharedPtr<FQuestTableRowEx>> Selected = GetSelectedRows();
	if (Selected.Num() == 0) return FReply::Handled();

	for (const TSharedPtr<FQuestTableRowEx>& Row : Selected)
	{
		if (Row.IsValid() && Row->Data.IsValid())
		{
			FQuestTableRow NewRow = *Row->Data;
			NewRow.RowId = FGuid::NewGuid();
			NewRow.StateID += TEXT("_Copy");
			NewRow.Status = EQuestTableRowStatus::New;
			NewRow.GeneratedQuest.Reset();
			NewRow.InvalidateValidation();

			TableData->Rows.Add(NewRow);
		}
	}

	MarkDirty();
	RefreshList();
	return FReply::Handled();
}

FReply SQuestTableEditor::OnValidateClicked()
{
	if (!TableData) return FReply::Handled();

	FQuestValidationResult Result = FQuestTableValidator::ValidateAllAndCache(TableData->Rows, TableData->ListsVersionGuid);

	RefreshList();

	// Show summary
	FString Summary = FString::Printf(TEXT("Validation complete: %d errors, %d warnings"),
		Result.GetErrorCount(), Result.GetWarningCount());

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Summary));

	return FReply::Handled();
}

FReply SQuestTableEditor::OnGenerateClicked()
{
	if (!TableData || bIsBusy) return FReply::Handled();

	bIsBusy = true;

	// Validate first
	FQuestValidationResult ValidationResult = FQuestTableValidator::ValidateAllAndCache(TableData->Rows, TableData->ListsVersionGuid);
	if (ValidationResult.HasErrors())
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("ValidationErrors", "Cannot generate: Please fix validation errors first."));
		bIsBusy = false;
		RefreshList();
		return FReply::Handled();
	}

	// Convert to manifest definitions
	TArray<FManifestQuestDefinition> Definitions = FQuestTableConverter::ConvertRowsToManifest(
		TableData->Rows, TableData->OutputFolder);

	int32 Generated = 0;
	int32 Failed = 0;

	for (const FManifestQuestDefinition& Def : Definitions)
	{
		FGenerationResult Result = FQuestGenerator::Generate(Def);
		if (Result.Status == EGenerationStatus::New)
		{
			Generated++;
		}
		else if (Result.Status == EGenerationStatus::Failed)
		{
			Failed++;
			UE_LOG(LogTemp, Warning, TEXT("Failed to generate quest %s: %s"), *Def.Name, *Result.Message);
		}
	}

	// Update generation tracking
	TableData->OnGenerationComplete(Failed);

	// Mark rows as synced
	for (FQuestTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			Row.Status = EQuestTableRowStatus::Synced;
			Row.LastSyncedHash = Row.ComputeSyncHash();
		}
	}

	MarkDirty();
	RefreshList();

	FString Summary = FString::Printf(TEXT("Generation complete: %d generated, %d failed"), Generated, Failed);
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Summary));

	bIsBusy = false;
	return FReply::Handled();
}

FReply SQuestTableEditor::OnSyncFromAssetsClicked()
{
	// TODO: Implement sync from existing Quest assets
	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NotImplemented", "Sync from Assets not yet implemented."));
	return FReply::Handled();
}

FReply SQuestTableEditor::OnExportXLSXClicked()
{
	// TODO: Implement XLSX export
	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NotImplemented", "XLSX Export not yet implemented."));
	return FReply::Handled();
}

FReply SQuestTableEditor::OnImportXLSXClicked()
{
	// TODO: Implement XLSX import
	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NotImplemented", "XLSX Import not yet implemented."));
	return FReply::Handled();
}

FReply SQuestTableEditor::OnSyncXLSXClicked()
{
	// TODO: Implement XLSX 3-way sync
	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NotImplemented", "XLSX Sync not yet implemented."));
	return FReply::Handled();
}

FReply SQuestTableEditor::OnSaveClicked()
{
	if (!TableData) return FReply::Handled();

	// Copy modified data back to TableData
	for (const TSharedPtr<FQuestTableRowEx>& RowEx : AllRows)
	{
		if (RowEx.IsValid() && RowEx->Data.IsValid())
		{
			int32 Index = TableData->FindRowIndexByGuid(RowEx->Data->RowId);
			if (Index != INDEX_NONE)
			{
				TableData->Rows[Index] = *RowEx->Data;
			}
		}
	}

	TableData->MarkPackageDirty();

	UPackage* Package = TableData->GetPackage();
	if (Package)
	{
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Standalone;
		FString PackageFileName = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		UPackage::SavePackage(Package, TableData, *PackageFileName, SaveArgs);
	}

	OnDirtyStateChanged.ExecuteIfBound();
	return FReply::Handled();
}

FReply SQuestTableEditor::OnApplyToAssetsClicked()
{
	// TODO: Implement apply to existing assets
	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NotImplemented", "Apply to Assets not yet implemented."));
	return FReply::Handled();
}

void SQuestTableEditor::MarkDirty()
{
	if (TableData)
	{
		TableData->MarkPackageDirty();
	}
	OnDirtyStateChanged.ExecuteIfBound();
}

void SQuestTableEditor::OnRowModified()
{
	MarkDirty();
	UpdateStatusBar();
}

//=============================================================================
// SQuestTableEditorWindow
//=============================================================================

void SQuestTableEditorWindow::Construct(const FArguments& InArgs)
{
	BaseTabLabel = LOCTEXT("QuestEditorTab", "Quest Table Editor");

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			BuildMenuBar()
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(TableEditor, SQuestTableEditor)
			.TableData(GetOrCreateTableData())
			.OnDirtyStateChanged(FOnQuestTableDirtyStateChanged::CreateSP(this, &SQuestTableEditorWindow::UpdateTabLabel))
		]
	];
}

void SQuestTableEditorWindow::SetParentTab(TSharedPtr<SDockTab> InTab)
{
	ParentTab = InTab;
	UpdateTabLabel();
}

TSharedRef<SWidget> SQuestTableEditorWindow::BuildMenuBar()
{
	FMenuBarBuilder MenuBuilder(nullptr);

	MenuBuilder.AddPullDownMenu(
		LOCTEXT("FileMenu", "File"),
		LOCTEXT("FileMenuTooltip", "File operations"),
		FNewMenuDelegate::CreateLambda([this](FMenuBuilder& Builder)
		{
			Builder.AddMenuEntry(
				LOCTEXT("NewTable", "New Table"),
				LOCTEXT("NewTableTooltip", "Create a new quest table"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQuestTableEditorWindow::OnNewTable))
			);
			Builder.AddMenuEntry(
				LOCTEXT("OpenTable", "Open Table..."),
				LOCTEXT("OpenTableTooltip", "Open an existing quest table"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQuestTableEditorWindow::OnOpenTable))
			);
			Builder.AddMenuSeparator();
			Builder.AddMenuEntry(
				LOCTEXT("SaveTable", "Save"),
				LOCTEXT("SaveTableTooltip", "Save the current table"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQuestTableEditorWindow::OnSaveTable))
			);
		})
	);

	return MenuBuilder.MakeWidget();
}

void SQuestTableEditorWindow::OnNewTable()
{
	CurrentTableData = GetOrCreateTableData();
	CurrentTableData->Rows.Empty();
	if (TableEditor.IsValid())
	{
		TableEditor->SetTableData(CurrentTableData.Get());
	}
}

void SQuestTableEditorWindow::OnOpenTable()
{
	// TODO: Implement open file dialog
}

void SQuestTableEditorWindow::OnSaveTable()
{
	if (CurrentTableData.IsValid())
	{
		CurrentTableData->MarkPackageDirty();

		UPackage* Package = CurrentTableData->GetPackage();
		if (Package)
		{
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Standalone;
			FString PackageFileName = FPackageName::LongPackageNameToFilename(
				Package->GetName(), FPackageName::GetAssetPackageExtension());
			UPackage::SavePackage(Package, CurrentTableData.Get(), *PackageFileName, SaveArgs);
		}

		UpdateTabLabel();
	}
}

void SQuestTableEditorWindow::OnSaveTableAs()
{
	// TODO: Implement save as
}

UQuestTableData* SQuestTableEditorWindow::GetOrCreateTableData()
{
	if (CurrentTableData.IsValid())
	{
		return CurrentTableData.Get();
	}

	// Try to load default table
	FString DefaultPath = TEXT("/Game/Tables/QuestTableData");
	UQuestTableData* Data = LoadObject<UQuestTableData>(nullptr, *DefaultPath);

	if (!Data)
	{
		// Create new
		FString PackagePath = TEXT("/Game/Tables/QuestTableData");
		UPackage* Package = CreatePackage(*PackagePath);
		Data = NewObject<UQuestTableData>(Package, TEXT("QuestTableData"), RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(Data);
		Data->MarkPackageDirty();
	}

	CurrentTableData = Data;
	return Data;
}

void SQuestTableEditorWindow::UpdateTabLabel()
{
	if (TSharedPtr<SDockTab> Tab = ParentTab.Pin())
	{
		FText Label = BaseTabLabel;
		if (IsTableDirty())
		{
			Label = FText::Format(LOCTEXT("DirtyTabLabel", "{0}*"), Label);
		}
		Tab->SetLabel(Label);
	}
}

bool SQuestTableEditorWindow::IsTableDirty() const
{
	if (CurrentTableData.IsValid())
	{
		return CurrentTableData->GetPackage()->IsDirty();
	}
	return false;
}

bool SQuestTableEditorWindow::CanCloseTab() const
{
	if (IsTableDirty())
	{
		EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNoCancel,
			LOCTEXT("SaveBeforeClose", "Save changes before closing?"));

		if (Result == EAppReturnType::Yes)
		{
			const_cast<SQuestTableEditorWindow*>(this)->OnSaveTable();
			return true;
		}
		else if (Result == EAppReturnType::No)
		{
			return true;
		}
		return false;  // Cancel
	}
	return true;
}

#undef LOCTEXT_NAMESPACE
