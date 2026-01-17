// SItemTableEditor.cpp
// Item Table Editor Implementation
// v4.8: Follows NPC/Dialogue table patterns with dynamic column visibility
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#include "ItemTableEditor/SItemTableEditor.h"
#include "ItemTableConverter.h"
#include "ItemTableValidator.h"
#include "GasAbilityGeneratorGenerators.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSpinBox.h"
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

#define LOCTEXT_NAMESPACE "ItemTableEditor"

//=============================================================================
// SItemTableRow
//=============================================================================

void SItemTableRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	RowData = InArgs._RowData;
	OnRowModified = InArgs._OnRowModified;

	SMultiColumnTableRow<TSharedPtr<FItemTableRow>>::Construct(
		FSuperRowType::FArguments(),
		InOwnerTable
	);
}

TSharedRef<SWidget> SItemTableRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (!RowData.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	FItemTableRow& Row = *RowData;

	// Check dynamic visibility
	if (!ShouldShowColumn(ColumnName))
	{
		return SNullWidget::NullWidget;
	}

	if (ColumnName == TEXT("Status"))
	{
		return CreateStatusCell();
	}
	else if (ColumnName == TEXT("ItemName"))
	{
		return CreateTextCell(Row.ItemName, TEXT("Item name (no EI_ prefix)"));
	}
	else if (ColumnName == TEXT("DisplayName"))
	{
		return CreateTextCell(Row.DisplayName, TEXT("Display name"));
	}
	else if (ColumnName == TEXT("ItemType"))
	{
		return CreateItemTypeCell();
	}
	else if (ColumnName == TEXT("EquipmentSlot"))
	{
		return CreateEquipmentSlotCell();
	}
	else if (ColumnName == TEXT("BaseValue"))
	{
		return CreateIntCell(Row.BaseValue, TEXT("Gold value"));
	}
	else if (ColumnName == TEXT("Weight"))
	{
		return CreateFloatCell(Row.Weight, TEXT("Weight in kg"));
	}
	else if (ColumnName == TEXT("AttackRating"))
	{
		return CreateFloatCell(Row.AttackRating, TEXT("Attack rating"));
	}
	else if (ColumnName == TEXT("ArmorRating"))
	{
		return CreateFloatCell(Row.ArmorRating, TEXT("Armor rating"));
	}
	else if (ColumnName == TEXT("ModifierGE"))
	{
		return CreateAssetDropdownCell(Row.ModifierGE, nullptr, TEXT("GE_"));
	}
	else if (ColumnName == TEXT("Abilities"))
	{
		return CreateTextCell(Row.Abilities, TEXT("GA_Ability1,GA_Ability2"));
	}
	else if (ColumnName == TEXT("WeaponConfig"))
	{
		return CreateTokenCell(Row.WeaponConfig, TEXT("Weapon(Damage=50,ClipSize=30)"));
	}
	else if (ColumnName == TEXT("ItemTags"))
	{
		return CreateTextCell(Row.ItemTags, TEXT("Item.Category.Type"));
	}
	else if (ColumnName == TEXT("bStackable"))
	{
		return CreateCheckboxCell(Row.bStackable);
	}
	else if (ColumnName == TEXT("MaxStackSize"))
	{
		return CreateIntCell(Row.MaxStackSize, TEXT("Max stack"));
	}
	else if (ColumnName == TEXT("Notes"))
	{
		return CreateNotesCell();
	}

	return SNullWidget::NullWidget;
}

bool SItemTableRow::ShouldShowColumn(FName ColumnId) const
{
	if (!RowData.IsValid()) return true;

	const FItemTableRow& Row = *RowData;

	// Dynamic visibility based on item type
	if (ColumnId == TEXT("AttackRating"))
	{
		return Row.IsWeapon();
	}
	if (ColumnId == TEXT("ArmorRating"))
	{
		return Row.IsArmor();
	}
	if (ColumnId == TEXT("WeaponConfig"))
	{
		return Row.IsWeapon();
	}

	return true;
}

TSharedRef<SWidget> SItemTableRow::CreateStatusCell()
{
	FItemTableRow& Row = *RowData;

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

TSharedRef<SWidget> SItemTableRow::CreateTextCell(FString& Value, const FString& Hint)
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

TSharedRef<SWidget> SItemTableRow::CreateFloatCell(float& Value, const FString& Hint)
{
	return SNew(SBox)
		.Padding(FMargin(4, 2))
		[
			SNew(SSpinBox<float>)
			.Value(Value)
			.MinValue(0.0f)
			.MaxValue(10000.0f)
			.MinSliderValue(0.0f)
			.MaxSliderValue(100.0f)
			.OnValueCommitted_Lambda([this, &Value](float NewValue, ETextCommit::Type) {
				Value = NewValue;
				MarkModified();
			})
		];
}

TSharedRef<SWidget> SItemTableRow::CreateIntCell(int32& Value, const FString& Hint)
{
	return SNew(SBox)
		.Padding(FMargin(4, 2))
		[
			SNew(SSpinBox<int32>)
			.Value(Value)
			.MinValue(0)
			.MaxValue(100000)
			.OnValueCommitted_Lambda([this, &Value](int32 NewValue, ETextCommit::Type) {
				Value = NewValue;
				MarkModified();
			})
		];
}

TSharedRef<SWidget> SItemTableRow::CreateCheckboxCell(bool& Value)
{
	return SNew(SBox)
		.Padding(FMargin(4, 2))
		.HAlign(HAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked(Value ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged_Lambda([this, &Value](ECheckBoxState NewState) {
				Value = (NewState == ECheckBoxState::Checked);
				MarkModified();
			})
		];
}

TSharedRef<SWidget> SItemTableRow::CreateItemTypeCell()
{
	FItemTableRow& Row = *RowData;

	return SNew(SBox)
		.Padding(FMargin(4, 2))
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(Row.GetItemTypeColor())
			.Padding(FMargin(4, 1))
			[
				SNew(STextBlock)
				.Text(FText::FromString(Row.GetItemTypeString()))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			]
		];
}

TSharedRef<SWidget> SItemTableRow::CreateEquipmentSlotCell()
{
	FItemTableRow& Row = *RowData;

	return SNew(SBox)
		.Padding(FMargin(4, 2))
		[
			SNew(SEditableTextBox)
			.Text(FText::FromString(Row.EquipmentSlot))
			.HintText(LOCTEXT("SlotHint", "Narrative.Equipment.Slot.X"))
			.OnTextCommitted_Lambda([this, &Row](const FText& NewText, ETextCommit::Type) {
				Row.EquipmentSlot = NewText.ToString();
				MarkModified();
			})
		];
}

TSharedRef<SWidget> SItemTableRow::CreateAssetDropdownCell(FSoftObjectPath& Value, UClass* AssetClass, const FString& AssetPrefix)
{
	// Simplified text input for asset paths
	return SNew(SBox)
		.Padding(FMargin(4, 2))
		[
			SNew(SEditableTextBox)
			.Text(FText::FromString(Value.GetAssetName()))
			.HintText(FText::FromString(AssetPrefix + TEXT("...")))
			.OnTextCommitted_Lambda([this, &Value](const FText& NewText, ETextCommit::Type) {
				// Simple path construction
				FString AssetName = NewText.ToString();
				if (!AssetName.IsEmpty())
				{
					Value = FSoftObjectPath(FString::Printf(TEXT("/Game/Effects/%s.%s"), *AssetName, *AssetName));
				}
				else
				{
					Value.Reset();
				}
				MarkModified();
			})
		];
}

TSharedRef<SWidget> SItemTableRow::CreateTokenCell(FString& Value, const FString& Hint)
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

TSharedRef<SWidget> SItemTableRow::CreateNotesCell()
{
	FItemTableRow& Row = *RowData;

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

void SItemTableRow::MarkModified()
{
	if (RowData.IsValid())
	{
		RowData->Status = EItemTableRowStatus::Modified;
		RowData->InvalidateValidation();
	}
	OnRowModified.ExecuteIfBound();
}

//=============================================================================
// SItemTableEditor
//=============================================================================

void SItemTableEditor::Construct(const FArguments& InArgs)
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
				SAssignNew(ListView, SListView<TSharedPtr<FItemTableRow>>)
				.ListItemsSource(&DisplayedRows)
				.OnGenerateRow(this, &SItemTableEditor::OnGenerateRow)
				.OnSelectionChanged(this, &SItemTableEditor::OnSelectionChanged)
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

TSharedRef<SWidget> SItemTableEditor::BuildToolbar()
{
	return SNew(SHorizontalBox)

		// Title
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(5, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Title", "Item Table Editor"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNullWidget::NullWidget
		]

		// Add Item
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("AddItem", "+ Item"))
			.OnClicked(this, &SItemTableEditor::OnAddRowClicked)
		]

		// Delete
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("Delete", "Delete"))
			.OnClicked(this, &SItemTableEditor::OnDeleteRowsClicked)
		]

		// Duplicate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("Duplicate", "Duplicate"))
			.OnClicked(this, &SItemTableEditor::OnDuplicateRowClicked)
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
			.OnClicked(this, &SItemTableEditor::OnValidateClicked)
		]

		// Generate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("Generate", "Generate"))
			.OnClicked(this, &SItemTableEditor::OnGenerateClicked)
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
			.OnClicked(this, &SItemTableEditor::OnExportXLSXClicked)
		]

		// Import XLSX
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("ImportXLSX", "Import"))
			.OnClicked(this, &SItemTableEditor::OnImportXLSXClicked)
		]

		// Save
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("Save", "Save"))
			.OnClicked(this, &SItemTableEditor::OnSaveClicked)
		];
}

TSharedRef<SHeaderRow> SItemTableEditor::BuildHeaderRow()
{
	TSharedRef<SHeaderRow> Header = SNew(SHeaderRow);
	TArray<FItemTableColumn> Columns = GetItemTableColumns();

	for (const FItemTableColumn& Col : Columns)
	{
		Header->AddColumn(
			SHeaderRow::Column(Col.ColumnId)
			.DefaultLabel(Col.DisplayName)
			.FillWidth(Col.DefaultWidth)
			.SortMode(this, &SItemTableEditor::GetColumnSortMode, Col.ColumnId)
			.OnSort(this, &SItemTableEditor::OnColumnSortModeChanged)
			.HeaderContent()
			[
				BuildColumnHeaderContent(Col)
			]
		);
	}

	HeaderRow = Header;
	return Header;
}

TSharedRef<SWidget> SItemTableEditor::BuildColumnHeaderContent(const FItemTableColumn& Col)
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

TSharedRef<SWidget> SItemTableEditor::BuildStatusBar()
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
				SAssignNew(StatusByTypeText, STextBlock)
				.Text(LOCTEXT("StatusByType", ""))
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

void SItemTableEditor::UpdateStatusBar()
{
	if (!TableData) return;

	int32 TotalRows = 0;
	int32 EquipCount = 0, RangedCount = 0, MeleeCount = 0, ConsumCount = 0;
	int32 ValidationErrors = 0;

	for (const FItemTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			TotalRows++;
			switch (Row.ItemType)
			{
				case EItemType::Equippable: EquipCount++; break;
				case EItemType::RangedWeapon: RangedCount++; break;
				case EItemType::MeleeWeapon: MeleeCount++; break;
				case EItemType::Consumable: ConsumCount++; break;
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
	if (StatusByTypeText.IsValid())
	{
		FString TypeStr = FString::Printf(TEXT("E:%d R:%d M:%d C:%d"), EquipCount, RangedCount, MeleeCount, ConsumCount);
		StatusByTypeText->SetText(FText::FromString(TypeStr));
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

void SItemTableEditor::InitializeColumnFilters()
{
	TArray<FItemTableColumn> Columns = GetItemTableColumns();
	for (const FItemTableColumn& Col : Columns)
	{
		ColumnFilters.Add(Col.ColumnId, FItemColumnFilterState());
	}
}

void SItemTableEditor::SyncFromTableData()
{
	AllRows.Empty();
	if (!TableData) return;

	for (FItemTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			AllRows.Add(MakeShared<FItemTableRow>(Row));
		}
	}
}

void SItemTableEditor::RefreshList()
{
	SyncFromTableData();
	ApplyFilters();
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
	UpdateStatusBar();
}

void SItemTableEditor::SetTableData(UItemTableData* InTableData)
{
	TableData = InTableData;
	RefreshList();
}

TArray<TSharedPtr<FItemTableRow>> SItemTableEditor::GetSelectedRows() const
{
	TArray<TSharedPtr<FItemTableRow>> Selected;
	if (ListView.IsValid())
	{
		Selected = ListView->GetSelectedItems();
	}
	return Selected;
}

TSharedRef<ITableRow> SItemTableEditor::OnGenerateRow(TSharedPtr<FItemTableRow> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SItemTableRow, OwnerTable)
		.RowData(Item)
		.OnRowModified(FSimpleDelegate::CreateSP(this, &SItemTableEditor::OnRowModified));
}

void SItemTableEditor::OnSelectionChanged(TSharedPtr<FItemTableRow> Item, ESelectInfo::Type SelectInfo)
{
	UpdateStatusBar();
}

EColumnSortMode::Type SItemTableEditor::GetColumnSortMode(FName ColumnId) const
{
	if (SortColumn == ColumnId) return SortMode;
	return EColumnSortMode::None;
}

void SItemTableEditor::OnColumnSortModeChanged(EColumnSortPriority::Type Priority, const FName& ColumnId, EColumnSortMode::Type InSortMode)
{
	SortColumn = ColumnId;
	SortMode = InSortMode;
	ApplySorting();
}

void SItemTableEditor::OnColumnTextFilterChanged(FName ColumnId, const FText& NewText)
{
	if (FItemColumnFilterState* State = ColumnFilters.Find(ColumnId))
	{
		State->TextFilter = NewText.ToString();
		ApplyFilters();
	}
}

void SItemTableEditor::OnColumnDropdownFilterChanged(FName ColumnId, const FString& Value, bool bIsSelected)
{
	ApplyFilters();
}

FReply SItemTableEditor::OnClearFiltersClicked()
{
	for (auto& Pair : ColumnFilters)
	{
		Pair.Value.TextFilter.Empty();
		Pair.Value.SelectedValues.Empty();
	}
	ApplyFilters();
	return FReply::Handled();
}

FString SItemTableEditor::GetColumnValue(const TSharedPtr<FItemTableRow>& Row, FName ColumnId) const
{
	if (!Row.IsValid()) return TEXT("");

	const FItemTableRow& Data = *Row;

	if (ColumnId == TEXT("Status")) return Data.GetStatusString();
	if (ColumnId == TEXT("ItemName")) return Data.ItemName;
	if (ColumnId == TEXT("DisplayName")) return Data.DisplayName;
	if (ColumnId == TEXT("ItemType")) return Data.GetItemTypeString();
	if (ColumnId == TEXT("EquipmentSlot")) return Data.EquipmentSlot;
	if (ColumnId == TEXT("BaseValue")) return FString::FromInt(Data.BaseValue);
	if (ColumnId == TEXT("Weight")) return FString::SanitizeFloat(Data.Weight);
	if (ColumnId == TEXT("AttackRating")) return FString::SanitizeFloat(Data.AttackRating);
	if (ColumnId == TEXT("ArmorRating")) return FString::SanitizeFloat(Data.ArmorRating);
	if (ColumnId == TEXT("ModifierGE")) return Data.ModifierGE.GetAssetName();
	if (ColumnId == TEXT("Abilities")) return Data.Abilities;
	if (ColumnId == TEXT("WeaponConfig")) return Data.WeaponConfig;
	if (ColumnId == TEXT("ItemTags")) return Data.ItemTags;
	if (ColumnId == TEXT("bStackable")) return Data.bStackable ? TEXT("Yes") : TEXT("No");
	if (ColumnId == TEXT("MaxStackSize")) return FString::FromInt(Data.MaxStackSize);
	if (ColumnId == TEXT("Notes")) return Data.Notes;

	return TEXT("");
}

void SItemTableEditor::ApplyFilters()
{
	DisplayedRows.Empty();

	for (const TSharedPtr<FItemTableRow>& Row : AllRows)
	{
		bool bPassesFilter = true;

		for (const auto& Pair : ColumnFilters)
		{
			const FItemColumnFilterState& State = Pair.Value;
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

void SItemTableEditor::ApplySorting()
{
	if (SortColumn.IsNone() || SortMode == EColumnSortMode::None)
	{
		return;
	}

	DisplayedRows.Sort([this](const TSharedPtr<FItemTableRow>& A, const TSharedPtr<FItemTableRow>& B) {
		FString ValueA = GetColumnValue(A, SortColumn);
		FString ValueB = GetColumnValue(B, SortColumn);

		if (SortMode == EColumnSortMode::Ascending)
		{
			return ValueA < ValueB;
		}
		return ValueA > ValueB;
	});
}

FReply SItemTableEditor::OnAddRowClicked()
{
	if (!TableData) return FReply::Handled();

	FItemTableRow& NewRow = TableData->AddRow();
	NewRow.ItemName = TEXT("NewItem");
	NewRow.ItemType = EItemType::Equippable;

	MarkDirty();
	RefreshList();
	return FReply::Handled();
}

FReply SItemTableEditor::OnDeleteRowsClicked()
{
	if (!TableData) return FReply::Handled();

	TArray<TSharedPtr<FItemTableRow>> Selected = GetSelectedRows();
	if (Selected.Num() == 0) return FReply::Handled();

	// Soft delete
	for (const TSharedPtr<FItemTableRow>& Row : Selected)
	{
		if (Row.IsValid())
		{
			for (FItemTableRow& TableRow : TableData->Rows)
			{
				if (TableRow.RowId == Row->RowId)
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

FReply SItemTableEditor::OnDuplicateRowClicked()
{
	if (!TableData) return FReply::Handled();

	TArray<TSharedPtr<FItemTableRow>> Selected = GetSelectedRows();
	if (Selected.Num() == 0) return FReply::Handled();

	for (const TSharedPtr<FItemTableRow>& Row : Selected)
	{
		if (Row.IsValid())
		{
			FItemTableRow NewRow = *Row;
			NewRow.RowId = FGuid::NewGuid();
			NewRow.ItemName += TEXT("_Copy");
			NewRow.Status = EItemTableRowStatus::New;
			NewRow.GeneratedItem.Reset();
			NewRow.InvalidateValidation();

			TableData->Rows.Add(NewRow);
		}
	}

	MarkDirty();
	RefreshList();
	return FReply::Handled();
}

FReply SItemTableEditor::OnValidateClicked()
{
	if (!TableData) return FReply::Handled();

	FItemValidationResult Result = FItemTableValidator::ValidateAllAndCache(TableData->Rows, TableData->ListsVersionGuid);

	RefreshList();

	FString Summary = FString::Printf(TEXT("Validation complete: %d errors, %d warnings"),
		Result.GetErrorCount(), Result.GetWarningCount());

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Summary));

	return FReply::Handled();
}

FReply SItemTableEditor::OnGenerateClicked()
{
	if (!TableData || bIsBusy) return FReply::Handled();

	bIsBusy = true;

	// Validate first
	FItemValidationResult ValidationResult = FItemTableValidator::ValidateAllAndCache(TableData->Rows, TableData->ListsVersionGuid);
	if (ValidationResult.HasErrors())
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("ValidationErrors", "Cannot generate: Please fix validation errors first."));
		bIsBusy = false;
		RefreshList();
		return FReply::Handled();
	}

	// Convert to manifest definitions
	TArray<FManifestEquippableItemDefinition> Definitions = FItemTableConverter::ConvertRowsToManifest(
		TableData->Rows, TableData->OutputFolder);

	int32 Generated = 0;
	int32 Failed = 0;

	for (const FManifestEquippableItemDefinition& Def : Definitions)
	{
		FGenerationResult Result = FEquippableItemGenerator::Generate(Def);
		if (Result.Status == EGenerationStatus::New)
		{
			Generated++;
		}
		else if (Result.Status == EGenerationStatus::Failed)
		{
			Failed++;
			UE_LOG(LogTemp, Warning, TEXT("Failed to generate item %s: %s"), *Def.Name, *Result.Message);
		}
	}

	// Update generation tracking
	TableData->OnGenerationComplete(Failed);

	// Mark rows as synced
	for (FItemTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			Row.Status = EItemTableRowStatus::Synced;
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

FReply SItemTableEditor::OnSyncFromAssetsClicked()
{
	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NotImplemented", "Sync from Assets not yet implemented."));
	return FReply::Handled();
}

FReply SItemTableEditor::OnExportXLSXClicked()
{
	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NotImplemented", "XLSX Export not yet implemented."));
	return FReply::Handled();
}

FReply SItemTableEditor::OnImportXLSXClicked()
{
	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NotImplemented", "XLSX Import not yet implemented."));
	return FReply::Handled();
}

FReply SItemTableEditor::OnSyncXLSXClicked()
{
	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NotImplemented", "XLSX Sync not yet implemented."));
	return FReply::Handled();
}

FReply SItemTableEditor::OnSaveClicked()
{
	if (!TableData) return FReply::Handled();

	// Copy modified data back to TableData
	for (const TSharedPtr<FItemTableRow>& Row : AllRows)
	{
		if (Row.IsValid())
		{
			int32 Index = TableData->FindRowIndexByGuid(Row->RowId);
			if (Index != INDEX_NONE)
			{
				TableData->Rows[Index] = *Row;
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

FReply SItemTableEditor::OnApplyToAssetsClicked()
{
	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NotImplemented", "Apply to Assets not yet implemented."));
	return FReply::Handled();
}

void SItemTableEditor::MarkDirty()
{
	if (TableData)
	{
		TableData->MarkPackageDirty();
	}
	OnDirtyStateChanged.ExecuteIfBound();
}

void SItemTableEditor::OnRowModified()
{
	MarkDirty();
	UpdateStatusBar();
}

void SItemTableEditor::UpdateDynamicColumnVisibility()
{
	// TODO: Update header column visibility based on item type filter
}

//=============================================================================
// SItemTableEditorWindow
//=============================================================================

void SItemTableEditorWindow::Construct(const FArguments& InArgs)
{
	BaseTabLabel = LOCTEXT("ItemEditorTab", "Item Table Editor");

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
			SAssignNew(TableEditor, SItemTableEditor)
			.TableData(GetOrCreateTableData())
			.OnDirtyStateChanged(FOnItemTableDirtyStateChanged::CreateSP(this, &SItemTableEditorWindow::UpdateTabLabel))
		]
	];
}

void SItemTableEditorWindow::SetParentTab(TSharedPtr<SDockTab> InTab)
{
	ParentTab = InTab;
	UpdateTabLabel();
}

TSharedRef<SWidget> SItemTableEditorWindow::BuildMenuBar()
{
	FMenuBarBuilder MenuBuilder(nullptr);

	MenuBuilder.AddPullDownMenu(
		LOCTEXT("FileMenu", "File"),
		LOCTEXT("FileMenuTooltip", "File operations"),
		FNewMenuDelegate::CreateLambda([this](FMenuBuilder& Builder)
		{
			Builder.AddMenuEntry(
				LOCTEXT("NewTable", "New Table"),
				LOCTEXT("NewTableTooltip", "Create a new item table"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SItemTableEditorWindow::OnNewTable))
			);
			Builder.AddMenuEntry(
				LOCTEXT("OpenTable", "Open Table..."),
				LOCTEXT("OpenTableTooltip", "Open an existing item table"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SItemTableEditorWindow::OnOpenTable))
			);
			Builder.AddMenuSeparator();
			Builder.AddMenuEntry(
				LOCTEXT("SaveTable", "Save"),
				LOCTEXT("SaveTableTooltip", "Save the current table"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SItemTableEditorWindow::OnSaveTable))
			);
		})
	);

	return MenuBuilder.MakeWidget();
}

void SItemTableEditorWindow::OnNewTable()
{
	CurrentTableData = GetOrCreateTableData();
	CurrentTableData->Rows.Empty();
	if (TableEditor.IsValid())
	{
		TableEditor->SetTableData(CurrentTableData.Get());
	}
}

void SItemTableEditorWindow::OnOpenTable()
{
	// TODO: Implement open file dialog
}

void SItemTableEditorWindow::OnSaveTable()
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

void SItemTableEditorWindow::OnSaveTableAs()
{
	// TODO: Implement save as
}

UItemTableData* SItemTableEditorWindow::GetOrCreateTableData()
{
	if (CurrentTableData.IsValid())
	{
		return CurrentTableData.Get();
	}

	// Try to load default table
	FString DefaultPath = TEXT("/Game/Tables/ItemTableData");
	UItemTableData* Data = LoadObject<UItemTableData>(nullptr, *DefaultPath);

	if (!Data)
	{
		// Create new
		FString PackagePath = TEXT("/Game/Tables/ItemTableData");
		UPackage* Package = CreatePackage(*PackagePath);
		Data = NewObject<UItemTableData>(Package, TEXT("ItemTableData"), RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(Data);
		Data->MarkPackageDirty();
	}

	CurrentTableData = Data;
	return Data;
}

void SItemTableEditorWindow::UpdateTabLabel()
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

bool SItemTableEditorWindow::IsTableDirty() const
{
	if (CurrentTableData.IsValid())
	{
		return CurrentTableData->GetPackage()->IsDirty();
	}
	return false;
}

bool SItemTableEditorWindow::CanCloseTab() const
{
	if (IsTableDirty())
	{
		EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNoCancel,
			LOCTEXT("SaveBeforeClose", "Save changes before closing?"));

		if (Result == EAppReturnType::Yes)
		{
			const_cast<SItemTableEditorWindow*>(this)->OnSaveTable();
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
