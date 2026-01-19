// SItemTableEditor.h
// Item Table Editor Slate Widget - Excel-like spreadsheet for managing Items
// v4.8: Follows NPC/Dialogue table patterns with XLSX sync, validation cache, soft delete
// Dynamic column visibility based on ItemType
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/SHeaderRow.h"
#include "ItemTableEditorTypes.h"

class SEditableText;
class SCheckBox;
class SSearchBox;

/**
 * Column definition for the Item table
 * v4.12.5: Changed to ManualWidth (fixed pixels) for consistent sizing
 */
struct FItemTableColumn
{
	FName ColumnId;
	FText DisplayName;
	float ManualWidth;  // Fixed pixel width
	bool bDynamicVisibility;  // True if visibility depends on ItemType

	FItemTableColumn(FName InId, const FText& InName, float InWidth = 80.0f, bool bInDynamic = false)
		: ColumnId(InId)
		, DisplayName(InName)
		, ManualWidth(InWidth)
		, bDynamicVisibility(bInDynamic)
	{}
};

/**
 * Per-column filter state (multi-select support)
 */
struct FItemColumnFilterState
{
	FString TextFilter;
	TSet<FString> SelectedValues;  // Empty = all, populated = only these values shown
	TArray<TSharedPtr<FString>> DropdownOptions;
};

/**
 * Get default column definitions for Item table (16 columns)
 * v4.12.5: Changed to ManualWidth (fixed pixels) for consistent sizing
 */
inline TArray<FItemTableColumn> GetItemTableColumns()
{
	TArray<FItemTableColumn> Columns;

	//=========================================================================
	// Core Identity (4 columns)
	//=========================================================================

	// 1. Status - read-only badge (auto-calculated)
	Columns.Add(FItemTableColumn(TEXT("Status"), NSLOCTEXT("ItemTableEditor", "ColStatus", "Status"), 55.0f));

	// 2. ItemName - EI_{ItemName} - v4.12.7: Renamed from "Item Name" to "Item File"
	Columns.Add(FItemTableColumn(TEXT("ItemName"), NSLOCTEXT("ItemTableEditor", "ColItemName", "Item File"), 130.0f));

	// 3. DisplayName - shown in-game
	Columns.Add(FItemTableColumn(TEXT("DisplayName"), NSLOCTEXT("ItemTableEditor", "ColDisplayName", "Display Name"), 130.0f));

	// 4. ItemType - determines parent class and visible columns (editable dropdown)
	Columns.Add(FItemTableColumn(TEXT("ItemType"), NSLOCTEXT("ItemTableEditor", "ColItemType", "Item Type"), 100.0f));

	//=========================================================================
	// Equipment (3 columns)
	//=========================================================================

	// 5. EquipmentSlot - gameplay tag
	Columns.Add(FItemTableColumn(TEXT("EquipmentSlot"), NSLOCTEXT("ItemTableEditor", "ColSlot", "Slot"), 100.0f));

	// 6. BaseValue - gold
	Columns.Add(FItemTableColumn(TEXT("BaseValue"), NSLOCTEXT("ItemTableEditor", "ColValue", "Value"), 65.0f));

	// 7. Weight - kg
	Columns.Add(FItemTableColumn(TEXT("Weight"), NSLOCTEXT("ItemTableEditor", "ColWeight", "Weight"), 65.0f));

	//=========================================================================
	// Combat Stats (2 columns) - Dynamic visibility
	//=========================================================================

	// 8. AttackRating - weapons only
	Columns.Add(FItemTableColumn(TEXT("AttackRating"), NSLOCTEXT("ItemTableEditor", "ColAttack", "Attack"), 65.0f, true));

	// 9. ArmorRating - armor only
	Columns.Add(FItemTableColumn(TEXT("ArmorRating"), NSLOCTEXT("ItemTableEditor", "ColArmor", "Armor"), 65.0f, true));

	//=========================================================================
	// References (2 columns)
	//=========================================================================

	// 10. ModifierGE - GE_*
	Columns.Add(FItemTableColumn(TEXT("ModifierGE"), NSLOCTEXT("ItemTableEditor", "ColModifier", "Modifier GE"), 110.0f));

	// 11. Abilities - comma-separated GA_*
	Columns.Add(FItemTableColumn(TEXT("Abilities"), NSLOCTEXT("ItemTableEditor", "ColAbilities", "Abilities"), 120.0f));

	//=========================================================================
	// Weapon Config (1 column) - Dynamic visibility
	//=========================================================================

	// 12. WeaponConfig - token: Weapon(Damage=50,ClipSize=30,...)
	Columns.Add(FItemTableColumn(TEXT("WeaponConfig"), NSLOCTEXT("ItemTableEditor", "ColWeaponConfig", "Weapon Config"), 160.0f, true));

	//=========================================================================
	// Tags & Stacking (3 columns)
	//=========================================================================

	// 13. ItemTags - gameplay tags
	Columns.Add(FItemTableColumn(TEXT("ItemTags"), NSLOCTEXT("ItemTableEditor", "ColTags", "Tags"), 120.0f));

	// 14. bStackable - checkbox
	Columns.Add(FItemTableColumn(TEXT("bStackable"), NSLOCTEXT("ItemTableEditor", "ColStackable", "Stack"), 50.0f));

	// 15. MaxStackSize
	Columns.Add(FItemTableColumn(TEXT("MaxStackSize"), NSLOCTEXT("ItemTableEditor", "ColMaxStack", "Max"), 50.0f));

	//=========================================================================
	// Meta (1 column)
	//=========================================================================

	// 16. Notes - designer notes
	Columns.Add(FItemTableColumn(TEXT("Notes"), NSLOCTEXT("ItemTableEditor", "ColNotes", "Notes"), 150.0f));

	return Columns;
}

/**
 * Row widget for the Item table (v4.8 - 16 columns)
 * Handles display and inline editing of a single Item row
 */
class GASABILITYGENERATOR_API SItemTableRow : public SMultiColumnTableRow<TSharedPtr<FItemTableRow>>
{
public:
	SLATE_BEGIN_ARGS(SItemTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FItemTableRow>, RowData)
		SLATE_EVENT(FSimpleDelegate, OnRowModified)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable);

	/** Generate widget for each column */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	TSharedPtr<FItemTableRow> RowData;
	FSimpleDelegate OnRowModified;

	//=========================================================================
	// Cell Widget Creators
	//=========================================================================

	/** Create status indicator (read-only badge) */
	TSharedRef<SWidget> CreateStatusCell();

	/** Create text cell (editable) */
	TSharedRef<SWidget> CreateTextCell(FString& Value, const FString& Hint = TEXT(""));

	/** v4.12.6: Create item name cell with click-to-open + dropdown + trimmed display */
	TSharedRef<SWidget> CreateItemNameCell();

	/** Create float cell */
	TSharedRef<SWidget> CreateFloatCell(float& Value, const FString& Hint = TEXT(""));

	/** Create int cell */
	TSharedRef<SWidget> CreateIntCell(int32& Value, const FString& Hint = TEXT(""));

	/** Create checkbox cell */
	TSharedRef<SWidget> CreateCheckboxCell(bool& Value);

	/** Create item type dropdown */
	TSharedRef<SWidget> CreateItemTypeCell();

	/** Create equipment slot dropdown */
	TSharedRef<SWidget> CreateEquipmentSlotCell();

	/** Create asset dropdown cell - filtered by class/prefix */
	TSharedRef<SWidget> CreateAssetDropdownCell(FSoftObjectPath& Value, UClass* AssetClass, const FString& AssetPrefix);

	/** Create modifier GE dropdown - filtered for equipment modifiers only */
	TSharedRef<SWidget> CreateModifierGECell();

	/** Create abilities multi-select dropdown - GA_* assets with checkboxes */
	TSharedRef<SWidget> CreateAbilitiesCell();

	/** Create token cell (WeaponConfig, ItemTags) with validation */
	TSharedRef<SWidget> CreateTokenCell(FString& Value, const FString& Hint);

	/** Create notes cell with tooltip */
	TSharedRef<SWidget> CreateNotesCell();

	/** Mark row as modified */
	void MarkModified();

	/** Check if column should be visible for this item type */
	bool ShouldShowColumn(FName ColumnId) const;
};

/** v4.8: Delegate for dirty state changes */
DECLARE_DELEGATE(FOnItemTableDirtyStateChanged);

/**
 * Main Item Table Editor Widget
 * Excel-like spreadsheet view for managing all Items
 */
class GASABILITYGENERATOR_API SItemTableEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SItemTableEditor) {}
		SLATE_ARGUMENT(UItemTableData*, TableData)
		SLATE_EVENT(FOnItemTableDirtyStateChanged, OnDirtyStateChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Refresh the table view */
	void RefreshList();

	/** Get selected rows */
	TArray<TSharedPtr<FItemTableRow>> GetSelectedRows() const;

	/** Set the table data */
	void SetTableData(UItemTableData* InTableData);

private:
	/** The data asset containing all rows */
	UItemTableData* TableData = nullptr;

	/** Filtered/sorted view of rows */
	TArray<TSharedPtr<FItemTableRow>> DisplayedRows;

	/** All rows as shared pointers */
	TArray<TSharedPtr<FItemTableRow>> AllRows;

	/** The list view widget */
	TSharedPtr<SListView<TSharedPtr<FItemTableRow>>> ListView;

	/** Header row */
	TSharedPtr<SHeaderRow> HeaderRow;

	/** Per-column filter state (text + multi-select) */
	TMap<FName, FItemColumnFilterState> ColumnFilters;

	/** Current sort column */
	FName SortColumn;
	EColumnSortMode::Type SortMode = EColumnSortMode::None;

	/** Current item type filter for dynamic column visibility */
	TOptional<EItemType> ItemTypeFilter;

	//=========================================================================
	// Status Bar (stored widget references for direct SetText() updates)
	//=========================================================================
	TSharedPtr<STextBlock> StatusTotalText;
	TSharedPtr<STextBlock> StatusByTypeText;  // Count by type
	TSharedPtr<STextBlock> StatusShowingText;
	TSharedPtr<STextBlock> StatusSelectedText;
	TSharedPtr<STextBlock> StatusValidationText;

	/** Explicitly update all status bar text */
	void UpdateStatusBar();

	//=========================================================================
	// UI Construction
	//=========================================================================

	/** Build the toolbar */
	TSharedRef<SWidget> BuildToolbar();

	/** Build the header row */
	TSharedRef<SHeaderRow> BuildHeaderRow();

	/** Build column header content (stacked: name + text filter + dropdown) */
	TSharedRef<SWidget> BuildColumnHeaderContent(const FItemTableColumn& Col);

	/** Build status bar */
	TSharedRef<SWidget> BuildStatusBar();

	/** Initialize column filter state for all columns */
	void InitializeColumnFilters();

	/** Update dropdown options for all column filters */
	void UpdateColumnFilterOptions();

	/** Column text filter changed */
	void OnColumnTextFilterChanged(FName ColumnId, const FText& NewText);

	/** Column dropdown filter selection changed */
	void OnColumnDropdownFilterChanged(FName ColumnId, const FString& Value, bool bIsSelected);

	/** Clear all filters */
	FReply OnClearFiltersClicked();

	/** Get unique values for a column (includes empty option) */
	TArray<FString> GetUniqueColumnValues(FName ColumnId) const;

	/** Get column value from row */
	FString GetColumnValue(const TSharedPtr<FItemTableRow>& Row, FName ColumnId) const;

	/** Update column visibility based on selected item type */
	void UpdateDynamicColumnVisibility();

	//=========================================================================
	// List View Callbacks
	//=========================================================================

	/** Generate row widget */
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FItemTableRow> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/** Selection changed */
	void OnSelectionChanged(TSharedPtr<FItemTableRow> Item, ESelectInfo::Type SelectInfo);

	/** Column sort clicked */
	EColumnSortMode::Type GetColumnSortMode(FName ColumnId) const;
	void OnColumnSortModeChanged(EColumnSortPriority::Type Priority, const FName& ColumnId, EColumnSortMode::Type SortMode);

	//=========================================================================
	// Actions
	//=========================================================================

	/** Add new row */
	FReply OnAddRowClicked();

	/** Delete selected rows (soft delete) */
	FReply OnDeleteRowsClicked();

	/** Duplicate selected row */
	FReply OnDuplicateRowClicked();

	/** Validate all rows */
	FReply OnValidateClicked();

	/** Generate assets from selected/all rows */
	FReply OnGenerateClicked();

	/** Sync from existing Item assets */
	FReply OnSyncFromAssetsClicked();

	/** Export to Excel (.xlsx) */
	FReply OnExportXLSXClicked();

	/** Import from Excel (.xlsx) */
	FReply OnImportXLSXClicked();

	/** XLSX 3-way sync */
	FReply OnSyncXLSXClicked();

	/** Save the table data */
	FReply OnSaveClicked();

	/** Apply changes back to Item assets */
	FReply OnApplyToAssetsClicked();

	//=========================================================================
	// Filtering & Sorting
	//=========================================================================

	/** Apply all filters and refresh display */
	void ApplyFilters();

	/** Apply sorting to displayed rows */
	void ApplySorting();

	//=========================================================================
	// Data Management
	//=========================================================================

	/** Sync AllRows from TableData */
	void SyncFromTableData();

	/** Mark table dirty */
	void MarkDirty();

	/** Row was modified callback */
	void OnRowModified();

	/** Delegate for notifying owner of dirty state changes */
	FOnItemTableDirtyStateChanged OnDirtyStateChanged;

	/** Re-entrancy guard - prevents double-clicks on long operations */
	bool bIsBusy = false;
};

/**
 * Tab/Window that hosts the Item Table Editor
 * v4.8: Added dirty indicator and save-on-close prompt
 */
class GASABILITYGENERATOR_API SItemTableEditorWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SItemTableEditorWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Set the parent tab for dirty indicator updates */
	void SetParentTab(TSharedPtr<SDockTab> InTab);

private:
	/** The table editor widget */
	TSharedPtr<SItemTableEditor> TableEditor;

	/** Current table data asset */
	TWeakObjectPtr<UItemTableData> CurrentTableData;

	/** Parent tab for dirty indicator */
	TWeakPtr<SDockTab> ParentTab;

	/** Base tab label (without dirty indicator) */
	FText BaseTabLabel;

	/** Build menu bar */
	TSharedRef<SWidget> BuildMenuBar();

	/** File menu actions */
	void OnNewTable();
	void OnOpenTable();
	void OnSaveTable();
	void OnSaveTableAs();

	/** Create or get default table data */
	UItemTableData* GetOrCreateTableData();

	/** Update tab label with dirty indicator */
	void UpdateTabLabel();

	/** Check if TableData is dirty */
	bool IsTableDirty() const;

	/** Handle tab close request - prompt to save if dirty */
	bool CanCloseTab() const;
};
