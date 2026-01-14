// SNPCTableEditor.cpp
// PROTOTYPE - NPC Table Editor Implementation
// This is example code for review, not compiled with the plugin
//
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#include "SNPCTableEditor.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Images/SImage.h"
#include "PropertyCustomizationHelpers.h" // For asset picker
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"

#define LOCTEXT_NAMESPACE "NPCTableEditor"

//=============================================================================
// SNPCTableRow - Individual Row Widget
//=============================================================================

void SNPCTableRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	RowData = InArgs._RowData;
	OnRowModified = InArgs._OnRowModified;

	SMultiColumnTableRow<TSharedPtr<FNPCTableRow>>::Construct(
		FSuperRowType::FArguments()
			.Padding(FMargin(2.0f, 1.0f)),
		InOwnerTable
	);
}

TSharedRef<SWidget> SNPCTableRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (!RowData.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	// Status column - colored indicator
	if (ColumnName == TEXT("Status"))
	{
		return CreateStatusCell();
	}

	// Identity columns
	if (ColumnName == TEXT("NPCName"))
	{
		return CreateTextCell(RowData->NPCName, TEXT("Enter NPC name..."));
	}
	if (ColumnName == TEXT("NPCId"))
	{
		return CreateTextCell(RowData->NPCId, TEXT("unique_id"));
	}
	if (ColumnName == TEXT("DisplayName"))
	{
		return CreateTextCell(RowData->DisplayName, TEXT("Display Name"));
	}

	// Asset reference columns
	if (ColumnName == TEXT("NPCBlueprint"))
	{
		return CreateAssetPickerCell(RowData->NPCBlueprint, TEXT("/Script/NarrativeArsenal.NarrativeNPCCharacter"));
	}
	if (ColumnName == TEXT("AbilityConfig"))
	{
		return CreateAssetPickerCell(RowData->AbilityConfig, TEXT("/Script/NarrativeArsenal.AbilityConfiguration"));
	}
	if (ColumnName == TEXT("ActivityConfig"))
	{
		return CreateAssetPickerCell(RowData->ActivityConfig, TEXT("/Script/NarrativeArsenal.NPCActivityConfiguration"));
	}
	if (ColumnName == TEXT("Schedule"))
	{
		return CreateAssetPickerCell(RowData->Schedule, TEXT("/Script/NarrativeArsenal.NPCActivitySchedule"));
	}
	if (ColumnName == TEXT("Dialogue"))
	{
		return CreateAssetPickerCell(RowData->Dialogue, TEXT("/Script/NarrativeArsenal.Dialogue"));
	}

	// Location columns
	if (ColumnName == TEXT("SpawnerPOI"))
	{
		return CreateTextCell(RowData->SpawnerPOI, TEXT("POI_Location"));
	}
	if (ColumnName == TEXT("LevelName"))
	{
		return CreateTextCell(RowData->LevelName, TEXT("MapName"));
	}

	// Vendor columns
	if (ColumnName == TEXT("bIsVendor"))
	{
		return CreateCheckboxCell(RowData->bIsVendor);
	}
	if (ColumnName == TEXT("ShopName"))
	{
		return CreateTextCell(RowData->ShopName, TEXT("Shop Name"));
	}

	// Item columns
	if (ColumnName == TEXT("DefaultItems"))
	{
		return CreateTextCell(RowData->DefaultItems, TEXT("EI_Item1, EI_Item2"));
	}
	if (ColumnName == TEXT("ShopItems"))
	{
		return CreateTextCell(RowData->ShopItems, TEXT("IC_Collection1"));
	}

	// Tags
	if (ColumnName == TEXT("Factions"))
	{
		return CreateTextCell(RowData->Factions, TEXT("Faction.Friendly"));
	}

	// Combat
	if (ColumnName == TEXT("MinLevel"))
	{
		return CreateNumberCell(RowData->MinLevel);
	}
	if (ColumnName == TEXT("MaxLevel"))
	{
		return CreateNumberCell(RowData->MaxLevel);
	}

	// Notes
	if (ColumnName == TEXT("Notes"))
	{
		return CreateTextCell(RowData->Notes, TEXT("Notes..."));
	}

	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SNPCTableRow::CreateStatusCell()
{
	FLinearColor StatusColor = RowData->GetStatusColor();

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		.HAlign(HAlign_Center)
		[
			SNew(SBorder)
				.BorderBackgroundColor(StatusColor)
				.Padding(FMargin(6.0f, 2.0f))
				[
					SNew(STextBlock)
						.Text(FText::FromString(RowData->Status))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.ColorAndOpacity(FSlateColor(FLinearColor::Black))
				]
		];
}

TSharedRef<SWidget> SNPCTableRow::CreateTextCell(FString& Value, const FString& Hint)
{
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableText)
				.Text_Lambda([&Value]() { return FText::FromString(Value); })
				.HintText(FText::FromString(Hint))
				.OnTextCommitted_Lambda([this, &Value](const FText& NewText, ETextCommit::Type)
				{
					Value = NewText.ToString();
					MarkModified();
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SNPCTableRow::CreateCheckboxCell(bool& Value)
{
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		.HAlign(HAlign_Center)
		[
			SNew(SCheckBox)
				.IsChecked_Lambda([&Value]() { return Value ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this, &Value](ECheckBoxState NewState)
				{
					Value = (NewState == ECheckBoxState::Checked);
					MarkModified();
				})
		];
}

TSharedRef<SWidget> SNPCTableRow::CreateNumberCell(int32& Value)
{
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableText)
				.Text_Lambda([&Value]() { return FText::AsNumber(Value); })
				.OnTextCommitted_Lambda([this, &Value](const FText& NewText, ETextCommit::Type)
				{
					Value = FCString::Atoi(*NewText.ToString());
					MarkModified();
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.Justification(ETextJustify::Right)
		];
}

TSharedRef<SWidget> SNPCTableRow::CreateFloatCell(float& Value)
{
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableText)
				.Text_Lambda([&Value]() { return FText::AsNumber(Value); })
				.OnTextCommitted_Lambda([this, &Value](const FText& NewText, ETextCommit::Type)
				{
					Value = FCString::Atof(*NewText.ToString());
					MarkModified();
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.Justification(ETextJustify::Right)
		];
}

TSharedRef<SWidget> SNPCTableRow::CreateAssetPickerCell(FSoftObjectPath& Value, const FString& AllowedClass)
{
	// For prototype, just show as text with asset name
	// Real implementation would use SObjectPropertyEntryBox with AllowedClass filter
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableText)
				.Text_Lambda([&Value]()
				{
					if (Value.IsNull()) return FText::FromString(TEXT("(None)"));
					return FText::FromString(Value.GetAssetName());
				})
				.HintText(LOCTEXT("PickAsset", "Click to pick asset..."))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.7f, 1.0f)))
				// In real implementation: OnMouseButtonDown opens asset picker dialog
		];
}

void SNPCTableRow::MarkModified()
{
	if (RowData.IsValid() && RowData->Status != TEXT("New"))
	{
		RowData->Status = TEXT("Modified");
	}
	OnRowModified.ExecuteIfBound();
}

//=============================================================================
// SNPCTableEditor - Main Table Widget
//=============================================================================

void SNPCTableEditor::Construct(const FArguments& InArgs)
{
	TableData = InArgs._TableData;
	SyncFromTableData();

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

		// Search box
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f, 2.0f)
		[
			SAssignNew(SearchBox, SSearchBox)
				.OnTextChanged(this, &SNPCTableEditor::OnSearchTextChanged)
				.HintText(LOCTEXT("SearchHint", "Search NPCs..."))
		]

		// Separator
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
		]

		// Table
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SScrollBox)
				.Orientation(Orient_Horizontal)
				+ SScrollBox::Slot()
				[
					SAssignNew(ListView, SListView<TSharedPtr<FNPCTableRow>>)
						.ListItemsSource(&DisplayedRows)
						.OnGenerateRow(this, &SNPCTableEditor::OnGenerateRow)
						.OnSelectionChanged(this, &SNPCTableEditor::OnSelectionChanged)
						.SelectionMode(ESelectionMode::Multi)
						.HeaderRow(BuildHeaderRow())
				]
		]

		// Status bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			BuildStatusBar()
		]
	];

	ApplyFilters();
}

TSharedRef<SWidget> SNPCTableEditor::BuildToolbar()
{
	return SNew(SHorizontalBox)

		// Add Row
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("AddRow", "+ Add NPC"))
				.OnClicked(this, &SNPCTableEditor::OnAddRowClicked)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
		]

		// Duplicate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("DuplicateRow", "Duplicate"))
				.OnClicked(this, &SNPCTableEditor::OnDuplicateRowClicked)
		]

		// Delete
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("DeleteRows", "Delete"))
				.OnClicked(this, &SNPCTableEditor::OnDeleteRowsClicked)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
		]

		// Spacer
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNullWidget::NullWidget
		]

		// Generate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("Generate", "Generate Assets"))
				.OnClicked(this, &SNPCTableEditor::OnGenerateClicked)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
		]

		// Sync
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("Sync", "Sync from Assets"))
				.OnClicked(this, &SNPCTableEditor::OnSyncFromAssetsClicked)
		]

		// Export CSV
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("ExportCSV", "Export CSV"))
				.OnClicked(this, &SNPCTableEditor::OnExportCSVClicked)
		]

		// Import CSV
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("ImportCSV", "Import CSV"))
				.OnClicked(this, &SNPCTableEditor::OnImportCSVClicked)
		];
}

TSharedRef<SHeaderRow> SNPCTableEditor::BuildHeaderRow()
{
	TSharedRef<SHeaderRow> Header = SNew(SHeaderRow);

	TArray<FNPCTableColumn> Columns = GetDefaultColumns();

	for (const FNPCTableColumn& Col : Columns)
	{
		Header->AddColumn(
			SHeaderRow::Column(Col.ColumnId)
				.DefaultLabel(Col.DisplayName)
				.DefaultTooltip(Col.DisplayName)
				.FillWidth(Col.DefaultWidth)
				.SortMode(this, &SNPCTableEditor::GetColumnSortMode, Col.ColumnId)
				.OnSort(this, &SNPCTableEditor::OnColumnSortModeChanged)
		);
	}

	HeaderRow = Header;
	return Header;
}

TSharedRef<SWidget> SNPCTableEditor::BuildStatusBar()
{
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SNew(STextBlock)
				.Text_Lambda([this]()
				{
					return FText::Format(
						LOCTEXT("RowCount", "Total: {0} NPCs"),
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
					return FText::Format(
						LOCTEXT("DisplayedCount", "Showing: {0}"),
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

TSharedRef<ITableRow> SNPCTableEditor::OnGenerateRow(TSharedPtr<FNPCTableRow> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SNPCTableRow, OwnerTable)
		.RowData(Item)
		.OnRowModified(FSimpleDelegate::CreateSP(this, &SNPCTableEditor::OnRowModified));
}

void SNPCTableEditor::OnSelectionChanged(TSharedPtr<FNPCTableRow> Item, ESelectInfo::Type SelectInfo)
{
	// Could update a details panel here
}

EColumnSortMode::Type SNPCTableEditor::GetColumnSortMode(FName ColumnId) const
{
	if (SortColumn == ColumnId)
	{
		return SortMode;
	}
	return EColumnSortMode::None;
}

void SNPCTableEditor::OnColumnSortModeChanged(EColumnSortPriority::Type Priority, const FName& ColumnId, EColumnSortMode::Type NewSortMode)
{
	SortColumn = ColumnId;
	SortMode = NewSortMode;
	ApplySorting();
	RefreshList();
}

void SNPCTableEditor::RefreshList()
{
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

TArray<TSharedPtr<FNPCTableRow>> SNPCTableEditor::GetSelectedRows() const
{
	TArray<TSharedPtr<FNPCTableRow>> Selected;
	if (ListView.IsValid())
	{
		Selected = ListView->GetSelectedItems();
	}
	return Selected;
}

void SNPCTableEditor::SyncFromTableData()
{
	AllRows.Empty();
	if (TableData)
	{
		for (FNPCTableRow& Row : TableData->Rows)
		{
			AllRows.Add(MakeShared<FNPCTableRow>(Row));
		}
	}
	ApplyFilters();
}

void SNPCTableEditor::ApplyFilters()
{
	DisplayedRows.Empty();

	for (TSharedPtr<FNPCTableRow>& Row : AllRows)
	{
		// Search filter
		if (!SearchText.IsEmpty())
		{
			FString SearchLower = SearchText.ToLower();
			bool bMatch =
				Row->NPCName.ToLower().Contains(SearchLower) ||
				Row->NPCId.ToLower().Contains(SearchLower) ||
				Row->DisplayName.ToLower().Contains(SearchLower) ||
				Row->Notes.ToLower().Contains(SearchLower);

			if (!bMatch) continue;
		}

		DisplayedRows.Add(Row);
	}

	ApplySorting();
	RefreshList();
}

void SNPCTableEditor::ApplySorting()
{
	if (SortColumn == NAME_None || SortMode == EColumnSortMode::None)
	{
		return;
	}

	bool bAscending = (SortMode == EColumnSortMode::Ascending);

	DisplayedRows.Sort([this, bAscending](const TSharedPtr<FNPCTableRow>& A, const TSharedPtr<FNPCTableRow>& B)
	{
		FString ValueA, ValueB;

		if (SortColumn == TEXT("NPCName")) { ValueA = A->NPCName; ValueB = B->NPCName; }
		else if (SortColumn == TEXT("NPCId")) { ValueA = A->NPCId; ValueB = B->NPCId; }
		else if (SortColumn == TEXT("DisplayName")) { ValueA = A->DisplayName; ValueB = B->DisplayName; }
		else if (SortColumn == TEXT("Status")) { ValueA = A->Status; ValueB = B->Status; }
		else if (SortColumn == TEXT("LevelName")) { ValueA = A->LevelName; ValueB = B->LevelName; }
		else return false;

		return bAscending ? (ValueA < ValueB) : (ValueA > ValueB);
	});
}

void SNPCTableEditor::OnSearchTextChanged(const FText& NewText)
{
	SearchText = NewText.ToString();
	ApplyFilters();
}

void SNPCTableEditor::OnRowModified()
{
	MarkDirty();
}

void SNPCTableEditor::MarkDirty()
{
	if (TableData)
	{
		TableData->MarkPackageDirty();
	}
}

//=============================================================================
// Actions
//=============================================================================

FReply SNPCTableEditor::OnAddRowClicked()
{
	if (TableData)
	{
		FNPCTableRow& NewRow = TableData->AddRow();
		AllRows.Add(MakeShared<FNPCTableRow>(NewRow));
		ApplyFilters();
		MarkDirty();
	}
	return FReply::Handled();
}

FReply SNPCTableEditor::OnDeleteRowsClicked()
{
	TArray<TSharedPtr<FNPCTableRow>> Selected = GetSelectedRows();

	if (Selected.Num() == 0)
	{
		return FReply::Handled();
	}

	// Remove from AllRows and TableData
	for (const TSharedPtr<FNPCTableRow>& Row : Selected)
	{
		AllRows.RemoveAll([&Row](const TSharedPtr<FNPCTableRow>& Item)
		{
			return Item->RowId == Row->RowId;
		});

		if (TableData)
		{
			TableData->RemoveRow(Row->RowId);
		}
	}

	ApplyFilters();
	MarkDirty();
	return FReply::Handled();
}

FReply SNPCTableEditor::OnDuplicateRowClicked()
{
	TArray<TSharedPtr<FNPCTableRow>> Selected = GetSelectedRows();

	if (Selected.Num() == 0 || !TableData)
	{
		return FReply::Handled();
	}

	// Find index in TableData
	for (int32 i = 0; i < TableData->Rows.Num(); i++)
	{
		if (TableData->Rows[i].RowId == Selected[0]->RowId)
		{
			FNPCTableRow& NewRow = TableData->DuplicateRow(i);
			AllRows.Add(MakeShared<FNPCTableRow>(NewRow));
			break;
		}
	}

	ApplyFilters();
	MarkDirty();
	return FReply::Handled();
}

FReply SNPCTableEditor::OnGenerateClicked()
{
	// TODO: Implement actual asset generation
	// For now, just mark all as synced

	for (TSharedPtr<FNPCTableRow>& Row : AllRows)
	{
		if (Row->IsValid())
		{
			Row->Status = TEXT("Synced");
			// Would call generators here:
			// - FNPCDefinitionGenerator::Generate(Row)
			// - etc.
		}
		else
		{
			Row->Status = TEXT("Error");
		}
	}

	RefreshList();
	MarkDirty();

	// Show message
	// FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("GenerateComplete", "Asset generation complete!"));

	return FReply::Handled();
}

FReply SNPCTableEditor::OnSyncFromAssetsClicked()
{
	// TODO: Scan project for existing NPCDefinition assets
	// and populate/update rows from them

	if (TableData)
	{
		TableData->LastSyncTime = FDateTime::Now();
	}

	return FReply::Handled();
}

FReply SNPCTableEditor::OnExportCSVClicked()
{
	// Build CSV content
	FString CSV;

	// Header row
	CSV += TEXT("NPCName,NPCId,DisplayName,SpawnerPOI,LevelName,IsVendor,ShopName,DefaultItems,Factions,MinLevel,MaxLevel,Notes\n");

	// Data rows
	for (const TSharedPtr<FNPCTableRow>& Row : AllRows)
	{
		CSV += FString::Printf(TEXT("%s,%s,%s,%s,%s,%s,%s,%s,%s,%d,%d,%s\n"),
			*Row->NPCName,
			*Row->NPCId,
			*Row->DisplayName,
			*Row->SpawnerPOI,
			*Row->LevelName,
			Row->bIsVendor ? TEXT("TRUE") : TEXT("FALSE"),
			*Row->ShopName,
			*Row->DefaultItems,
			*Row->Factions,
			Row->MinLevel,
			Row->MaxLevel,
			*Row->Notes
		);
	}

	// Would use file dialog to save
	// FFileHelper::SaveStringToFile(CSV, *FilePath);

	return FReply::Handled();
}

FReply SNPCTableEditor::OnImportCSVClicked()
{
	// TODO: Open file dialog and parse CSV
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
