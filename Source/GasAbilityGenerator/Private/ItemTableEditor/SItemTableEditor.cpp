// SItemTableEditor.cpp
// Item Table Editor Implementation
// v4.8: Follows NPC/Dialogue table patterns with dynamic column visibility
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#include "ItemTableEditor/SItemTableEditor.h"
#include "ItemTableEditor/ItemXLSXWriter.h"
#include "ItemTableEditor/ItemXLSXReader.h"
#include "XLSXSupport/ItemXLSXSyncEngine.h"   // v4.12: 3-way sync engine
#include "XLSXSupport/SItemXLSXSyncDialog.h"  // v4.12: Sync dialog
#include "ItemTableEditor/ItemAssetSync.h"    // v4.12: Asset sync
#include "ItemTableConverter.h"
#include "ItemTableValidator.h"
#include "GasAbilityGeneratorGenerators.h"
#include "DesktopPlatformModule.h"
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
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Items/NarrativeItem.h"
#include "Items/EquippableItem.h"
#include "Items/WeaponItem.h"
#include "Items/RangedWeaponItem.h"
#include "Items/MeleeWeaponItem.h"
#include "Items/MagicWeaponItem.h"
#include "Items/ThrowableWeaponItem.h"
#include "Items/AmmoItem.h"
#include "Items/GameplayEffectItem.h"
#include "Items/WeaponAttachmentItem.h"

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
	// Weapon columns
	else if (ColumnName == TEXT("AttackDamage"))
	{
		return CreateFloatCell(Row.AttackDamage, TEXT("Base damage"));
	}
	else if (ColumnName == TEXT("ClipSize"))
	{
		return CreateIntCell(Row.ClipSize, TEXT("Clip size"));
	}
	else if (ColumnName == TEXT("WeaponHand"))
	{
		return CreateTextCell(Row.WeaponHand, TEXT("TwoHanded"));
	}
	// Ranged columns
	else if (ColumnName == TEXT("BaseSpreadDegrees"))
	{
		return CreateFloatCell(Row.BaseSpreadDegrees, TEXT("Spread"));
	}
	else if (ColumnName == TEXT("AimFOVPct"))
	{
		return CreateFloatCell(Row.AimFOVPct, TEXT("FOV %"));
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
		return Row.IsEquipment();
	}
	if (ColumnId == TEXT("ArmorRating"))
	{
		return Row.IsArmor();
	}
	if (ColumnId == TEXT("StealthRating"))
	{
		return Row.IsEquipment();
	}
	// Weapon columns
	if (ColumnId == TEXT("AttackDamage") || ColumnId == TEXT("ClipSize") ||
	    ColumnId == TEXT("WeaponHand") || ColumnId == TEXT("HeavyAttackDamageMultiplier") ||
	    ColumnId == TEXT("bAllowManualReload") || ColumnId == TEXT("BotAttackRange"))
	{
		return Row.IsWeapon();
	}
	// Ranged weapon columns
	if (ColumnId == TEXT("BaseSpreadDegrees") || ColumnId == TEXT("MaxSpreadDegrees") ||
	    ColumnId == TEXT("SpreadFireBump") || ColumnId == TEXT("SpreadDecreaseSpeed") ||
	    ColumnId == TEXT("AimFOVPct"))
	{
		return Row.IsRangedWeapon();
	}
	// Consumable columns
	if (ColumnId == TEXT("bConsumeOnUse") || ColumnId == TEXT("UseRechargeDuration") ||
	    ColumnId == TEXT("bCanActivate") || ColumnId == TEXT("GameplayEffectClass"))
	{
		return Row.IsConsumable();
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

		// LEFT SIDE - Data actions
		// Add Item
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("AddItem", "+ Item"))
			.OnClicked(this, &SItemTableEditor::OnAddRowClicked)
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
		]

		// Duplicate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Duplicate", "Duplicate"))
			.OnClicked(this, &SItemTableEditor::OnDuplicateRowClicked)
		]

		// Delete
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Delete", "Delete"))
			.OnClicked(this, &SItemTableEditor::OnDeleteRowsClicked)
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
		]

		// Vertical separator
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SNew(SSeparator)
			.Orientation(Orient_Vertical)
		]

		// Clear Filters button
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("ClearFilters", "Clear Filters"))
			.OnClicked(this, &SItemTableEditor::OnClearFiltersClicked)
			.ToolTipText(LOCTEXT("ClearFiltersTip", "Reset all column filters"))
		]

		// SPACER - pushes right side buttons to the right
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNullWidget::NullWidget
		]

		// RIGHT SIDE - Generation/IO actions
		// Validate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Validate", "Validate"))
			.OnClicked(this, &SItemTableEditor::OnValidateClicked)
			.ToolTipText(LOCTEXT("ValidateTip", "Validate all rows for errors and warnings"))
			.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Generate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Generate", "Generate Items"))
			.OnClicked(this, &SItemTableEditor::OnGenerateClicked)
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
			.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Sync from Assets
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Sync", "Sync from Assets"))
			.OnClicked(this, &SItemTableEditor::OnSyncFromAssetsClicked)
			.ToolTipText(LOCTEXT("SyncTip", "Populate table from existing Item assets"))
			.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Export XLSX
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("ExportXLSX", "Export XLSX"))
			.OnClicked(this, &SItemTableEditor::OnExportXLSXClicked)
			.ToolTipText(LOCTEXT("ExportXLSXTooltip", "Export to Excel format (.xlsx)"))
			.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Import XLSX
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("ImportXLSX", "Import XLSX"))
			.OnClicked(this, &SItemTableEditor::OnImportXLSXClicked)
			.ToolTipText(LOCTEXT("ImportXLSXTooltip", "Import from Excel format (.xlsx)"))
			.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Sync XLSX
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("SyncXLSX", "Sync XLSX"))
			.OnClicked(this, &SItemTableEditor::OnSyncXLSXClicked)
			.ToolTipText(LOCTEXT("SyncXLSXTooltip", "Merge Excel changes with UE (3-way merge)"))
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
			.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Separator before Asset Actions
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SNew(SSeparator)
			.Orientation(Orient_Vertical)
		]

		// Save Table
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Save", "Save Table"))
			.OnClicked(this, &SItemTableEditor::OnSaveClicked)
			.ToolTipText(LOCTEXT("SaveTooltip", "Save the table data container"))
		]

		// Apply to Assets
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("ApplyToAssets", "Apply to Items"))
			.OnClicked(this, &SItemTableEditor::OnApplyToAssetsClicked)
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
			.ToolTipText(LOCTEXT("ApplyToAssetsTooltip", "Write changes back to Item assets"))
			.IsEnabled_Lambda([this]() { return !bIsBusy; })
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
	FItemColumnFilterState* State = ColumnFilters.Find(ColumnId);
	if (!State) return;

	// Handle special case: if currently showing all, initialize with all values then toggle
	if (State->SelectedValues.Num() == 0 || State->SelectedValues.Contains(TEXT("__NONE_SELECTED__")))
	{
		State->SelectedValues.Empty();
		if (bIsSelected)
		{
			// Just add this single value
			State->SelectedValues.Add(Value);
		}
		else
		{
			// Initialize with all values except this one
			TArray<FString> AllValues = GetUniqueColumnValues(ColumnId);
			for (const FString& V : AllValues)
			{
				if (V != Value)
				{
					State->SelectedValues.Add(V);
				}
			}
		}
	}
	else
	{
		// Toggle the specific value
		if (bIsSelected)
		{
			State->SelectedValues.Add(Value);
		}
		else
		{
			State->SelectedValues.Remove(Value);
		}
	}

	// If all values are selected, clear the filter (show all)
	TArray<FString> AllValues = GetUniqueColumnValues(ColumnId);
	if (State->SelectedValues.Num() == AllValues.Num())
	{
		State->SelectedValues.Empty();
	}

	ApplyFilters();
}

TArray<FString> SItemTableEditor::GetUniqueColumnValues(FName ColumnId) const
{
	// Filter dropdowns show values from the current table data (AllRows)
	TSet<FString> UniqueSet;

	for (const TSharedPtr<FItemTableRow>& Row : AllRows)
	{
		FString Value = GetColumnValue(Row, ColumnId);
		// Empty cells become "(None)" for display consistency
		if (Value.IsEmpty())
		{
			Value = TEXT("(None)");
		}
		UniqueSet.Add(Value);
	}

	TArray<FString> Result = UniqueSet.Array();
	Result.Sort();
	return Result;
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
	// Weapon properties
	if (ColumnId == TEXT("AttackDamage")) return FString::SanitizeFloat(Data.AttackDamage);
	if (ColumnId == TEXT("ClipSize")) return FString::FromInt(Data.ClipSize);
	if (ColumnId == TEXT("WeaponHand")) return Data.WeaponHand;
	if (ColumnId == TEXT("HeavyAttackDamageMultiplier")) return FString::SanitizeFloat(Data.HeavyAttackDamageMultiplier);
	if (ColumnId == TEXT("bAllowManualReload")) return Data.bAllowManualReload ? TEXT("Yes") : TEXT("No");
	if (ColumnId == TEXT("BotAttackRange")) return FString::SanitizeFloat(Data.BotAttackRange);
	// Ranged properties
	if (ColumnId == TEXT("BaseSpreadDegrees")) return FString::SanitizeFloat(Data.BaseSpreadDegrees);
	if (ColumnId == TEXT("MaxSpreadDegrees")) return FString::SanitizeFloat(Data.MaxSpreadDegrees);
	if (ColumnId == TEXT("AimFOVPct")) return FString::SanitizeFloat(Data.AimFOVPct);
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
			FString Value = GetColumnValue(Row, Pair.Key);

			// Check text filter
			if (!State.TextFilter.IsEmpty())
			{
				if (!Value.Contains(State.TextFilter))
				{
					bPassesFilter = false;
					break;
				}
			}

			// Check dropdown filter (SelectedValues)
			if (State.SelectedValues.Num() > 0 && !State.SelectedValues.Contains(TEXT("__NONE_SELECTED__")))
			{
				// Normalize empty values to "(None)" for consistency with GetUniqueColumnValues
				FString NormalizedValue = Value.IsEmpty() ? TEXT("(None)") : Value;
				if (!State.SelectedValues.Contains(NormalizedValue))
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
	// v4.8.4: Re-entrancy guard
	if (bIsBusy) return FReply::Handled();
	TGuardValue<bool> BusyGuard(bIsBusy, true);

	if (!TableData)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoTableData", "No table data available. Please create or open a table first."));
		return FReply::Handled();
	}

	// Get Asset Registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Force Asset Registry rescan before querying
	TArray<FString> PathsToScan = { TEXT("/Game/") };
	UE_LOG(LogTemp, Log, TEXT("[ItemTableEditor] Rescanning Asset Registry for paths: %s"), *FString::Join(PathsToScan, TEXT(", ")));
	AssetRegistry.ScanPathsSynchronous(PathsToScan, true /* bForceRescan */);

	// Find all EquippableItem assets (this is the base class for all equipment)
	TArray<FAssetData> AssetList;
	FTopLevelAssetPath ClassPath = UEquippableItem::StaticClass()->GetClassPathName();
	AssetRegistry.GetAssetsByClass(ClassPath, AssetList, true);
	UE_LOG(LogTemp, Log, TEXT("[ItemTableEditor] Found %d UEquippableItem assets"), AssetList.Num());

	if (AssetList.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoItemsFound", "No EquippableItem assets found in the project.\n\nCreate EquippableItem assets (EI_*) first, then sync."));
		return FReply::Handled();
	}

	// Preserve existing Notes values before clearing rows (user-entered, not stored in Item asset)
	TMap<FString, FString> PreservedNotes;
	for (const FItemTableRow& ExistingRow : TableData->Rows)
	{
		if (!ExistingRow.Notes.IsEmpty())
		{
			PreservedNotes.Add(ExistingRow.ItemName, ExistingRow.Notes);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[ItemTableEditor] Preserved %d Notes before sync"), PreservedNotes.Num());

	// Clear existing rows and populate from assets
	TableData->Rows.Empty();
	int32 SyncedCount = 0;

	for (const FAssetData& AssetData : AssetList)
	{
		UEquippableItem* Item = Cast<UEquippableItem>(AssetData.GetAsset());
		if (!Item)
		{
			continue;
		}

		FItemTableRow& Row = TableData->AddRow();

		// Core Identity
		Row.ItemName = AssetData.AssetName.ToString();

		// Get DisplayName via reflection (it's a protected FText property)
		if (FProperty* DisplayNameProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("DisplayName")))
		{
			FText* DisplayNamePtr = DisplayNameProp->ContainerPtrToValuePtr<FText>(Item);
			if (DisplayNamePtr)
			{
				Row.DisplayName = DisplayNamePtr->ToString();
			}
		}

		// Determine ItemType based on class hierarchy (most specific first)
		if (Item->IsA(URangedWeaponItem::StaticClass()))
		{
			Row.ItemType = EItemType::RangedWeapon;
		}
		else if (Item->IsA(UMeleeWeaponItem::StaticClass()))
		{
			Row.ItemType = EItemType::MeleeWeapon;
		}
		else if (Item->IsA(UMagicWeaponItem::StaticClass()))
		{
			Row.ItemType = EItemType::MagicWeapon;
		}
		else if (Item->IsA(UThrowableWeaponItem::StaticClass()))
		{
			Row.ItemType = EItemType::ThrowableWeapon;
		}
		else if (Item->IsA(UEquippableItem_Clothing::StaticClass()))
		{
			Row.ItemType = EItemType::Clothing;
		}
		else
		{
			Row.ItemType = EItemType::Equippable;
		}

		// Equipment slot - get first slot from EquippableSlots container
		// Access via reflection since EquippableSlots is protected
		if (FProperty* SlotsProperty = UEquippableItem::StaticClass()->FindPropertyByName(TEXT("EquippableSlots")))
		{
			FGameplayTagContainer* Slots = SlotsProperty->ContainerPtrToValuePtr<FGameplayTagContainer>(Item);
			if (Slots && Slots->Num() > 0)
			{
				TArray<FGameplayTag> TagArray;
				Slots->GetGameplayTagArray(TagArray);
				if (TagArray.Num() > 0)
				{
					Row.EquipmentSlot = TagArray[0].ToString();
				}
			}
		}

		// Base value and weight from NarrativeItem base class
		if (FProperty* ValueProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("BaseValue")))
		{
			int32* Value = ValueProp->ContainerPtrToValuePtr<int32>(Item);
			if (Value)
			{
				Row.BaseValue = *Value;
			}
		}

		if (FProperty* WeightProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("Weight")))
		{
			float* Weight = WeightProp->ContainerPtrToValuePtr<float>(Item);
			if (Weight)
			{
				Row.Weight = *Weight;
			}
		}

		// Description from NarrativeItem
		if (FProperty* DescProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("Description")))
		{
			FText* DescPtr = DescProp->ContainerPtrToValuePtr<FText>(Item);
			if (DescPtr)
			{
				Row.Description = DescPtr->ToString();
			}
		}

		// BaseScore from NarrativeItem
		if (FProperty* ScoreProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("BaseScore")))
		{
			float* Score = ScoreProp->ContainerPtrToValuePtr<float>(Item);
			if (Score)
			{
				Row.BaseScore = *Score;
			}
		}

		// Combat stats via reflection (protected properties on UEquippableItem)
		if (FProperty* AttackProp = UEquippableItem::StaticClass()->FindPropertyByName(TEXT("AttackRating")))
		{
			float* Attack = AttackProp->ContainerPtrToValuePtr<float>(Item);
			if (Attack)
			{
				Row.AttackRating = *Attack;
			}
		}

		if (FProperty* ArmorProp = UEquippableItem::StaticClass()->FindPropertyByName(TEXT("ArmorRating")))
		{
			float* Armor = ArmorProp->ContainerPtrToValuePtr<float>(Item);
			if (Armor)
			{
				Row.ArmorRating = *Armor;
			}
		}

		if (FProperty* StealthProp = UEquippableItem::StaticClass()->FindPropertyByName(TEXT("StealthRating")))
		{
			float* Stealth = StealthProp->ContainerPtrToValuePtr<float>(Item);
			if (Stealth)
			{
				Row.StealthRating = *Stealth;
			}
		}

		// Weapon stats (if this is a weapon)
		if (Row.IsWeapon())
		{
			if (FProperty* DamageProp = UWeaponItem::StaticClass()->FindPropertyByName(TEXT("AttackDamage")))
			{
				float* Damage = DamageProp->ContainerPtrToValuePtr<float>(Item);
				if (Damage)
				{
					Row.AttackDamage = *Damage;
				}
			}

			if (FProperty* HeavyMultProp = UWeaponItem::StaticClass()->FindPropertyByName(TEXT("HeavyAttackDamageMultiplier")))
			{
				float* Mult = HeavyMultProp->ContainerPtrToValuePtr<float>(Item);
				if (Mult)
				{
					Row.HeavyAttackDamageMultiplier = *Mult;
				}
			}

			if (FProperty* HandProp = UWeaponItem::StaticClass()->FindPropertyByName(TEXT("WeaponHand")))
			{
				uint8* HandValue = HandProp->ContainerPtrToValuePtr<uint8>(Item);
				if (HandValue)
				{
					switch (*HandValue)
					{
						case 0: Row.WeaponHand = TEXT("TwoHanded"); break;
						case 1: Row.WeaponHand = TEXT("MainHand"); break;
						case 2: Row.WeaponHand = TEXT("OffHand"); break;
						case 3: Row.WeaponHand = TEXT("DualWieldable"); break;
						default: Row.WeaponHand = TEXT("TwoHanded"); break;
					}
				}
			}

			if (FProperty* ClipProp = UWeaponItem::StaticClass()->FindPropertyByName(TEXT("ClipSize")))
			{
				int32* Clip = ClipProp->ContainerPtrToValuePtr<int32>(Item);
				if (Clip)
				{
					Row.ClipSize = *Clip;
				}
			}

			if (FProperty* ReloadProp = UWeaponItem::StaticClass()->FindPropertyByName(TEXT("bAllowManualReload")))
			{
				bool* Reload = ReloadProp->ContainerPtrToValuePtr<bool>(Item);
				if (Reload)
				{
					Row.bAllowManualReload = *Reload;
				}
			}

			if (FProperty* RangeProp = UWeaponItem::StaticClass()->FindPropertyByName(TEXT("BotAttackRange")))
			{
				float* Range = RangeProp->ContainerPtrToValuePtr<float>(Item);
				if (Range)
				{
					Row.BotAttackRange = *Range;
				}
			}
		}

		// Ranged weapon stats
		if (Row.IsRangedWeapon())
		{
			if (FProperty* SpreadProp = URangedWeaponItem::StaticClass()->FindPropertyByName(TEXT("BaseSpreadDegrees")))
			{
				float* Spread = SpreadProp->ContainerPtrToValuePtr<float>(Item);
				if (Spread)
				{
					Row.BaseSpreadDegrees = *Spread;
				}
			}

			if (FProperty* MaxSpreadProp = URangedWeaponItem::StaticClass()->FindPropertyByName(TEXT("MaxSpreadDegrees")))
			{
				float* MaxSpread = MaxSpreadProp->ContainerPtrToValuePtr<float>(Item);
				if (MaxSpread)
				{
					Row.MaxSpreadDegrees = *MaxSpread;
				}
			}

			if (FProperty* BumpProp = URangedWeaponItem::StaticClass()->FindPropertyByName(TEXT("SpreadFireBump")))
			{
				float* Bump = BumpProp->ContainerPtrToValuePtr<float>(Item);
				if (Bump)
				{
					Row.SpreadFireBump = *Bump;
				}
			}

			if (FProperty* DecProp = URangedWeaponItem::StaticClass()->FindPropertyByName(TEXT("SpreadDecreaseSpeed")))
			{
				float* Dec = DecProp->ContainerPtrToValuePtr<float>(Item);
				if (Dec)
				{
					Row.SpreadDecreaseSpeed = *Dec;
				}
			}

			if (FProperty* FOVProp = URangedWeaponItem::StaticClass()->FindPropertyByName(TEXT("AimFOVPct")))
			{
				float* FOV = FOVProp->ContainerPtrToValuePtr<float>(Item);
				if (FOV)
				{
					Row.AimFOVPct = *FOV;
				}
			}
		}

		// Consumable stats from NarrativeItem
		if (FProperty* ConsumeProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("bConsumeOnUse")))
		{
			bool* Consume = ConsumeProp->ContainerPtrToValuePtr<bool>(Item);
			if (Consume)
			{
				Row.bConsumeOnUse = *Consume;
			}
		}

		if (FProperty* RechargeProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("UseRechargeDuration")))
		{
			float* Recharge = RechargeProp->ContainerPtrToValuePtr<float>(Item);
			if (Recharge)
			{
				Row.UseRechargeDuration = *Recharge;
			}
		}

		if (FProperty* ActivateProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("bCanActivate")))
		{
			bool* Activate = ActivateProp->ContainerPtrToValuePtr<bool>(Item);
			if (Activate)
			{
				Row.bCanActivate = *Activate;
			}
		}

		// Equipment effect (modifier GE)
		if (FProperty* EffectProp = UEquippableItem::StaticClass()->FindPropertyByName(TEXT("EquipmentEffect")))
		{
			TSubclassOf<UGameplayEffect>* Effect = EffectProp->ContainerPtrToValuePtr<TSubclassOf<UGameplayEffect>>(Item);
			if (Effect && *Effect)
			{
				Row.ModifierGE = FSoftObjectPath((*Effect)->GetPathName());
			}
		}

		// Stackable and max stack from NarrativeItem
		if (FProperty* StackProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("bStackable")))
		{
			bool* Stackable = StackProp->ContainerPtrToValuePtr<bool>(Item);
			if (Stackable)
			{
				Row.bStackable = *Stackable;
			}
		}

		if (FProperty* MaxStackProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("MaxStackSize")))
		{
			int32* MaxStack = MaxStackProp->ContainerPtrToValuePtr<int32>(Item);
			if (MaxStack)
			{
				Row.MaxStackSize = *MaxStack;
			}
		}

		// Item tags
		if (FProperty* TagsProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("ItemTags")))
		{
			FGameplayTagContainer* Tags = TagsProp->ContainerPtrToValuePtr<FGameplayTagContainer>(Item);
			if (Tags && Tags->Num() > 0)
			{
				TArray<FString> TagStrings;
				for (const FGameplayTag& ItemTag : *Tags)
				{
					TagStrings.Add(ItemTag.ToString());
				}
				Row.ItemTags = FString::Join(TagStrings, TEXT(", "));
			}
		}

		// Restore preserved Notes
		if (FString* PreservedNote = PreservedNotes.Find(Row.ItemName))
		{
			Row.Notes = *PreservedNote;
		}

		// Mark as synced
		Row.Status = EItemTableRowStatus::Synced;
		Row.LastSyncedHash = Row.ComputeSyncHash();

		SyncedCount++;
	}

	// Update table
	MarkDirty();
	SyncFromTableData();
	RefreshList();

	FString Summary = FString::Printf(TEXT("Synced %d items from %d EquippableItem assets"), SyncedCount, AssetList.Num());
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Summary));

	return FReply::Handled();
}

FReply SItemTableEditor::OnExportXLSXClicked()
{
	if (!TableData || bIsBusy) return FReply::Handled();

	TGuardValue<bool> BusyGuard(bIsBusy, true);

	// Sync UI changes back to TableData
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

	// Show save dialog
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return FReply::Handled();
	}

	TArray<FString> OutFiles;
	FString DefaultPath = FPaths::ProjectSavedDir() / TEXT("Exports");
	FString DefaultFile = TEXT("ItemTable.xlsx");

	bool bOpened = DesktopPlatform->SaveFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Export Item Table"),
		DefaultPath,
		DefaultFile,
		TEXT("Excel Files (*.xlsx)|*.xlsx"),
		0,
		OutFiles
	);

	if (bOpened && OutFiles.Num() > 0)
	{
		FString Error;
		if (FItemXLSXWriter::ExportToXLSX(TableData->Rows, OutFiles[0], Error))
		{
			// Update sync hashes after successful export
			for (FItemTableRow& Row : TableData->Rows)
			{
				Row.LastSyncedHash = Row.ComputeSyncHash();
				Row.Status = EItemTableRowStatus::Synced;
			}

			MarkDirty();
			RefreshList();

			FMessageDialog::Open(EAppMsgType::Ok,
				FText::Format(LOCTEXT("ExportSuccess", "Exported {0} items to:\n{1}"),
					FText::AsNumber(TableData->Rows.Num()),
					FText::FromString(OutFiles[0])));
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok,
				FText::Format(LOCTEXT("ExportFailed", "Export failed: {0}"),
					FText::FromString(Error)));
		}
	}

	return FReply::Handled();
}

FReply SItemTableEditor::OnImportXLSXClicked()
{
	if (!TableData || bIsBusy) return FReply::Handled();

	TGuardValue<bool> BusyGuard(bIsBusy, true);

	// Show open dialog
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return FReply::Handled();
	}

	TArray<FString> OutFiles;
	FString DefaultPath = FPaths::ProjectSavedDir() / TEXT("Exports");

	bool bOpened = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Import Item Table"),
		DefaultPath,
		TEXT(""),
		TEXT("Excel Files (*.xlsx)|*.xlsx"),
		0,
		OutFiles
	);

	if (bOpened && OutFiles.Num() > 0)
	{
		// Validate file format
		if (!FItemXLSXReader::IsValidItemXLSX(OutFiles[0]))
		{
			FMessageDialog::Open(EAppMsgType::Ok,
				LOCTEXT("InvalidFormat", "Invalid file format. Please select an Item Table XLSX file exported from this editor."));
			return FReply::Handled();
		}

		TArray<FItemTableRow> ImportedRows;
		FString Error;

		if (FItemXLSXReader::ImportFromXLSX(OutFiles[0], ImportedRows, Error))
		{
			// Replace table data with imported rows
			TableData->Rows = ImportedRows;

			// Invalidate validation for all rows
			for (FItemTableRow& Row : TableData->Rows)
			{
				Row.InvalidateValidation();
			}

			MarkDirty();
			SyncFromTableData();
			RefreshList();

			FMessageDialog::Open(EAppMsgType::Ok,
				FText::Format(LOCTEXT("ImportSuccess", "Imported {0} items from:\n{1}"),
					FText::AsNumber(ImportedRows.Num()),
					FText::FromString(OutFiles[0])));
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok,
				FText::Format(LOCTEXT("ImportFailed", "Import failed: {0}"),
					FText::FromString(Error)));
		}
	}

	return FReply::Handled();
}

FReply SItemTableEditor::OnSyncXLSXClicked()
{
	if (!TableData || bIsBusy) return FReply::Handled();

	TGuardValue<bool> BusyGuard(bIsBusy, true);

	// Show open dialog to select XLSX file
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return FReply::Handled();

	TArray<FString> OutFiles;
	FString DefaultPath = FPaths::ProjectSavedDir() / TEXT("Exports");

	bool bOpened = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Select Item XLSX for Sync"),
		DefaultPath,
		TEXT(""),
		TEXT("Excel Files (*.xlsx)|*.xlsx"),
		0,
		OutFiles
	);

	if (!bOpened || OutFiles.Num() == 0)
	{
		return FReply::Handled();
	}

	// Validate the file format
	if (!FItemXLSXReader::IsValidItemXLSX(OutFiles[0]))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("InvalidSyncFile",
			"The selected file is not a valid Item XLSX file.\n"
			"Make sure it was exported from the Item Table Editor."));
		return FReply::Handled();
	}

	// Sync UI changes back to TableData first
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

	// Import rows from Excel
	TArray<FItemTableRow> ExcelRows;
	FString ImportError;
	if (!FItemXLSXReader::ImportFromXLSX(OutFiles[0], ExcelRows, ImportError))
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("SyncImportFailed", "Failed to read Excel file:\n{0}"),
			FText::FromString(ImportError)));
		return FReply::Handled();
	}

	// Build base rows and UE rows
	TArray<FItemTableRow> BaseRows;
	TArray<FItemTableRow> UERows;

	for (const FItemTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			UERows.Add(Row);
			// Base is conceptually the row at the time of last export
			// Rows with non-zero LastSyncedHash existed at last export
			if (Row.LastSyncedHash != 0)
			{
				BaseRows.Add(Row);
			}
		}
	}

	// Perform 3-way comparison
	FItemSyncResult SyncResult = FItemXLSXSyncEngine::CompareSources(BaseRows, UERows, ExcelRows);

	if (!SyncResult.bSuccess)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("SyncCompareFailed", "Sync comparison failed:\n{0}"),
			FText::FromString(SyncResult.ErrorMessage)));
		return FReply::Handled();
	}

	// Auto-resolve unchanged entries (v4.11: only Unchanged auto-resolves)
	FItemXLSXSyncEngine::AutoResolveNonConflicts(SyncResult);

	// Check if there are any changes
	if (!SyncResult.HasChanges())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoSyncChanges",
			"No changes detected between UE and Excel.\n"
			"Both sources are in sync."));
		return FReply::Handled();
	}

	// Show approval dialog
	if (!SItemXLSXSyncDialog::ShowModal(SyncResult))
	{
		// User cancelled
		return FReply::Handled();
	}

	// Check all entries are resolved
	if (!FItemXLSXSyncEngine::AllConflictsResolved(SyncResult))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("UnresolvedConflicts",
			"Some entries have not been resolved.\n"
			"Please resolve all conflicts before applying."));
		return FReply::Handled();
	}

	// Apply sync result
	FItemMergeResult MergeResult = FItemXLSXSyncEngine::ApplySync(SyncResult);

	// Replace TableData rows with merged result
	TableData->Rows.Empty();
	for (const FItemTableRow& MergedRow : MergeResult.MergedRows)
	{
		FItemTableRow& NewRow = TableData->Rows.Add_GetRef(MergedRow);
		NewRow.LastSyncedHash = NewRow.ComputeSyncHash();
		NewRow.Status = EItemTableRowStatus::Synced;
	}

	MarkDirty();
	SyncFromTableData();
	RefreshList();

	// Show summary
	FString Summary = FString::Printf(
		TEXT("Sync complete!\n\n"
			"Applied from UE: %d\n"
			"Applied from Excel: %d\n"
			"Deleted: %d\n"
			"Unchanged: %d"),
		MergeResult.AppliedFromUE,
		MergeResult.AppliedFromExcel,
		MergeResult.Deleted,
		MergeResult.Unchanged);

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Summary));

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
	if (!TableData || bIsBusy) return FReply::Handled();

	TGuardValue<bool> BusyGuard(bIsBusy, true);

	// Step 1: Sync UI changes back to TableData
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

	// Step 2: Gather rows to apply (non-deleted rows only)
	TArray<FItemTableRow> RowsToApply;
	for (const FItemTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			RowsToApply.Add(Row);
		}
	}

	if (RowsToApply.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoRowsToApply", "No item rows to apply. Add items first."));
		return FReply::Handled();
	}

	// Step 3: Confirm with user
	FText ConfirmMessage = FText::Format(
		LOCTEXT("ApplyConfirm", "This will regenerate Item assets for {0} item(s).\n\n"
			"Modified rows will update existing assets.\n"
			"New rows will create new assets.\n\n"
			"Continue?"),
		FText::AsNumber(RowsToApply.Num()));

	EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, ConfirmMessage);
	if (Result != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	// Step 4: Apply to assets
	FString OutputFolder = TableData ? TableData->OutputFolder : TEXT("/Game/Items");

	FItemAssetApplySummary Summary = FItemAssetSync::ApplyToAssets(
		RowsToApply,
		OutputFolder,
		true);  // bCreateMissing = true

	// Step 5: Update row statuses
	for (FItemTableRow& AppliedRow : RowsToApply)
	{
		for (TSharedPtr<FItemTableRow>& Row : AllRows)
		{
			if (Row.IsValid() && Row->RowId == AppliedRow.RowId)
			{
				Row->Status = AppliedRow.Status;
				Row->GeneratedItem = AppliedRow.GeneratedItem;
				break;
			}
		}
	}

	RefreshList();
	UpdateStatusBar();

	// Step 6: Show result summary
	FMessageDialog::Open(EAppMsgType::Ok,
		FText::Format(LOCTEXT("ApplyComplete",
			"Apply complete!\n\n"
			"Items processed: {0}\n"
			"Assets created: {1}\n"
			"Assets modified: {2}\n"
			"Skipped (unchanged): {3}\n"
			"Skipped (validation): {4}\n"
			"Skipped (no asset): {5}\n"
			"Failed: {6}"),
			FText::AsNumber(Summary.AssetsProcessed),
			FText::AsNumber(Summary.AssetsCreated),
			FText::AsNumber(Summary.AssetsModified),
			FText::AsNumber(Summary.AssetsSkippedNotModified),
			FText::AsNumber(Summary.AssetsSkippedValidation),
			FText::AsNumber(Summary.AssetsSkippedNoAsset),
			FText::AsNumber(Summary.FailedItems.Num())));

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
	// Determine which item types are visible in the filtered data
	bool bHasWeapons = false;
	bool bHasRanged = false;
	bool bHasEquippables = false;

	for (const TSharedPtr<FItemTableRow>& Row : DisplayedRows)
	{
		if (!Row.IsValid()) continue;

		switch (Row->ItemType)
		{
			case EItemType::RangedWeapon:
				bHasRanged = true;
				bHasWeapons = true;
				break;
			case EItemType::MeleeWeapon:
			case EItemType::MagicWeapon:
			case EItemType::ThrowableWeapon:
				bHasWeapons = true;
				break;
			case EItemType::Equippable:
			case EItemType::Clothing:
				bHasEquippables = true;
				break;
			default:
				break;
		}
	}

	// v4.12: Column visibility state (could be used by header row widget)
	// For now, just refresh the list which will use ShouldShowColumn in each row
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
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
	// Use content browser asset picker
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FOpenAssetDialogConfig Config;
	Config.DialogTitleOverride = LOCTEXT("OpenItemTableTitle", "Open Item Table Data");
	Config.bAllowMultipleSelection = false;
	Config.AssetClassNames.Add(UItemTableData::StaticClass()->GetClassPathName());

	TArray<FAssetData> SelectedAssets = ContentBrowserModule.Get().CreateModalOpenAssetDialog(Config);
	if (SelectedAssets.Num() > 0)
	{
		UItemTableData* LoadedTable = Cast<UItemTableData>(SelectedAssets[0].GetAsset());
		if (LoadedTable)
		{
			CurrentTableData = LoadedTable;
			if (TableEditor.IsValid())
			{
				TableEditor->SetTableData(LoadedTable);
			}
			UpdateTabLabel();
		}
	}
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
	// Use content browser save asset dialog
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FSaveAssetDialogConfig Config;
	Config.DialogTitleOverride = LOCTEXT("SaveItemTableAsTitle", "Save Item Table As");
	Config.DefaultPath = TEXT("/Game/Tables");
	Config.DefaultAssetName = TEXT("ItemTableData");
	Config.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;

	FString SelectedPath = ContentBrowserModule.Get().CreateModalSaveAssetDialog(Config);
	if (!SelectedPath.IsEmpty())
	{
		// Create new package
		FString PackagePath = FPackageName::ObjectPathToPackageName(SelectedPath);
		FString AssetName = FPackageName::GetLongPackageAssetName(SelectedPath);

		UPackage* NewPackage = CreatePackage(*PackagePath);
		if (NewPackage)
		{
			// Create a new UItemTableData object in the new package
			UItemTableData* NewTableData = NewObject<UItemTableData>(NewPackage, *AssetName, RF_Public | RF_Standalone);

			// Copy data from current table
			if (CurrentTableData.IsValid())
			{
				NewTableData->Rows = CurrentTableData->Rows;
			}

			// Register with asset registry
			FAssetRegistryModule::AssetCreated(NewTableData);

			// Save the package
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Standalone;
			FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
			UPackage::SavePackage(NewPackage, NewTableData, *PackageFileName, SaveArgs);

			// Switch to the new table
			CurrentTableData = NewTableData;
			if (TableEditor.IsValid())
			{
				TableEditor->SetTableData(NewTableData);
			}
			UpdateTabLabel();
		}
	}
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
