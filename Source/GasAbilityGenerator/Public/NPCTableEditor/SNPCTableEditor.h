// SNPCTableEditor.h
// NPC Table Editor Slate Widget - Excel-like spreadsheet for managing NPCs
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/SHeaderRow.h"
#include "NPCTableEditorTypes.h"

class SEditableText;
class SCheckBox;
class SSearchBox;

/**
 * Column definition for the NPC table
 */
struct FNPCTableColumn
{
	FName ColumnId;
	FText DisplayName;
	float DefaultWidth;
	bool bCanSort;

	FNPCTableColumn(FName InId, const FText& InName, float InWidth = 1.0f, bool bInCanSort = true)
		: ColumnId(InId)
		, DisplayName(InName)
		, DefaultWidth(InWidth)
		, bCanSort(bInCanSort)
	{}
};

/**
 * Get default column definitions for NPC table
 * Only columns that can sync from NPCDefinition
 * Width values are in pixels (fixed width, not fill width)
 */
inline TArray<FNPCTableColumn> GetNPCTableColumns()
{
	TArray<FNPCTableColumn> Columns;

	// 1. Status - generation state (small badge)
	Columns.Add(FNPCTableColumn(TEXT("Status"), NSLOCTEXT("NPCTableEditor", "ColStatus", "Status"), 55.0f));

	// 2. NPCName - asset/internal name
	Columns.Add(FNPCTableColumn(TEXT("NPCName"), NSLOCTEXT("NPCTableEditor", "ColNPCName", "NPC Name"), 140.0f));

	// 3. NPCId - unique identifier
	Columns.Add(FNPCTableColumn(TEXT("NPCId"), NSLOCTEXT("NPCTableEditor", "ColNPCId", "NPC ID"), 120.0f));

	// 4. DisplayName - what player sees
	Columns.Add(FNPCTableColumn(TEXT("DisplayName"), NSLOCTEXT("NPCTableEditor", "ColDisplayName", "Display Name"), 130.0f));

	// 5. Factions - friendly/hostile (critical for gameplay)
	Columns.Add(FNPCTableColumn(TEXT("Factions"), NSLOCTEXT("NPCTableEditor", "ColFactions", "Factions"), 100.0f));

	// 6. OwnedTags - gameplay state tags
	Columns.Add(FNPCTableColumn(TEXT("OwnedTags"), NSLOCTEXT("NPCTableEditor", "ColOwnedTags", "Tags"), 100.0f));

	// 7. AbilityConfig - dropdown select
	Columns.Add(FNPCTableColumn(TEXT("AbilityConfig"), NSLOCTEXT("NPCTableEditor", "ColAbilityConfig", "Ability"), 110.0f, false));

	// 8. ActivityConfig - dropdown select
	Columns.Add(FNPCTableColumn(TEXT("ActivityConfig"), NSLOCTEXT("NPCTableEditor", "ColActivityConfig", "Activity"), 110.0f, false));

	// 9. NPCBlueprint - character blueprint
	Columns.Add(FNPCTableColumn(TEXT("NPCBlueprint"), NSLOCTEXT("NPCTableEditor", "ColNPCBlueprint", "Blueprint"), 120.0f, false));

	// 10. bIsVendor - vendor type (checkbox)
	Columns.Add(FNPCTableColumn(TEXT("bIsVendor"), NSLOCTEXT("NPCTableEditor", "ColIsVendor", "V"), 30.0f));

	// 11. MinLevel - combat balance (small number)
	Columns.Add(FNPCTableColumn(TEXT("MinLevel"), NSLOCTEXT("NPCTableEditor", "ColMinLevel", "Min"), 35.0f));

	// 12. MaxLevel - combat balance (small number)
	Columns.Add(FNPCTableColumn(TEXT("MaxLevel"), NSLOCTEXT("NPCTableEditor", "ColMaxLevel", "Max"), 35.0f));

	// 13. AttackPriority - targeting priority
	Columns.Add(FNPCTableColumn(TEXT("AttackPriority"), NSLOCTEXT("NPCTableEditor", "ColAttackPriority", "Prio"), 40.0f));

	// 14. DefaultCurrency - starting gold
	Columns.Add(FNPCTableColumn(TEXT("DefaultCurrency"), NSLOCTEXT("NPCTableEditor", "ColDefaultCurrency", "Gold"), 50.0f));

	// 15. TradingCurrency - vendor gold (for vendors)
	Columns.Add(FNPCTableColumn(TEXT("TradingCurrency"), NSLOCTEXT("NPCTableEditor", "ColTradingCurrency", "Shop$"), 50.0f));

	// 16. ShopName - vendor shop name
	Columns.Add(FNPCTableColumn(TEXT("ShopName"), NSLOCTEXT("NPCTableEditor", "ColShopName", "Shop Name"), 100.0f));

	// 17. DefaultItems - starting equipment
	Columns.Add(FNPCTableColumn(TEXT("DefaultItems"), NSLOCTEXT("NPCTableEditor", "ColDefaultItems", "Items"), 120.0f));

	// 18. Spawners - where this NPC can spawn
	Columns.Add(FNPCTableColumn(TEXT("Spawners"), NSLOCTEXT("NPCTableEditor", "ColSpawners", "Spawners"), 180.0f, false));

	// 19. Notes - designer notes
	Columns.Add(FNPCTableColumn(TEXT("Notes"), NSLOCTEXT("NPCTableEditor", "ColNotes", "Notes"), 150.0f));

	return Columns;
}

/**
 * Row widget for the NPC table
 * Handles display and inline editing of a single NPC row
 */
class GASABILITYGENERATOR_API SNPCTableRow : public SMultiColumnTableRow<TSharedPtr<FNPCTableRow>>
{
public:
	SLATE_BEGIN_ARGS(SNPCTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FNPCTableRow>, RowData)
		SLATE_EVENT(FSimpleDelegate, OnRowModified)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable);

	/** Generate widget for each column */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	TSharedPtr<FNPCTableRow> RowData;
	FSimpleDelegate OnRowModified;

	/** Create text cell (editable) */
	TSharedRef<SWidget> CreateTextCell(FString& Value, const FString& Hint = TEXT(""));

	/** Create checkbox cell */
	TSharedRef<SWidget> CreateCheckboxCell(bool& Value);

	/** Create number cell */
	TSharedRef<SWidget> CreateNumberCell(int32& Value);

	/** Create float cell */
	TSharedRef<SWidget> CreateFloatCell(float& Value);

	/** Create asset picker cell */
	TSharedRef<SWidget> CreateAssetPickerCell(FSoftObjectPath& Value, UClass* AllowedClass);

	/** Create status indicator */
	TSharedRef<SWidget> CreateStatusCell();

	/** Create factions cell - displays short names, converts to/from full tags */
	TSharedRef<SWidget> CreateFactionsCell();

	/** Create owned tags cell - displays short names, converts to/from full tags */
	TSharedRef<SWidget> CreateOwnedTagsCell();

	/** Create asset dropdown cell - lists available assets of given class */
	TSharedRef<SWidget> CreateAssetDropdownCell(FSoftObjectPath& Value, UClass* AssetClass, const FString& AssetPrefix);

	/** Mark row as modified */
	void MarkModified();
};

/**
 * Main NPC Table Editor Widget
 * Excel-like spreadsheet view for managing all NPCs
 */
class GASABILITYGENERATOR_API SNPCTableEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNPCTableEditor) {}
		SLATE_ARGUMENT(UNPCTableData*, TableData)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Refresh the table view */
	void RefreshList();

	/** Get selected rows */
	TArray<TSharedPtr<FNPCTableRow>> GetSelectedRows() const;

	/** Set the table data */
	void SetTableData(UNPCTableData* InTableData);

private:
	/** The data asset containing all rows */
	UNPCTableData* TableData = nullptr;

	/** Filtered/sorted view of rows */
	TArray<TSharedPtr<FNPCTableRow>> DisplayedRows;

	/** All rows as shared pointers */
	TArray<TSharedPtr<FNPCTableRow>> AllRows;

	/** The list view widget */
	TSharedPtr<SListView<TSharedPtr<FNPCTableRow>>> ListView;

	/** Header row */
	TSharedPtr<SHeaderRow> HeaderRow;

	/** Search box */
	TSharedPtr<SSearchBox> SearchBox;

	/** Current search text */
	FString SearchText;

	/** Per-column filter values (text) */
	TMap<FName, FString> ColumnFilters;

	/** Per-column filter values (selection - set of allowed values) */
	TMap<FName, TSet<FString>> ColumnSelectionFilters;

	/** Current sort column */
	FName SortColumn;
	EColumnSortMode::Type SortMode = EColumnSortMode::None;

	//=========================================================================
	// UI Construction
	//=========================================================================

	/** Build the toolbar (Add, Delete, Generate, Sync buttons) */
	TSharedRef<SWidget> BuildToolbar();

	/** Build the header row */
	TSharedRef<SHeaderRow> BuildHeaderRow();

	/** Build status bar */
	TSharedRef<SWidget> BuildStatusBar();

	/** Build filter row */
	TSharedRef<SWidget> BuildFilterRow();

	/** Column filter changed */
	void OnColumnFilterChanged(const FText& NewText, FName ColumnId);

	/** Get unique values for a column */
	TArray<FString> GetUniqueColumnValues(FName ColumnId) const;

	/** Get column value from row */
	FString GetColumnValue(const TSharedPtr<FNPCTableRow>& Row, FName ColumnId) const;

	//=========================================================================
	// List View Callbacks
	//=========================================================================

	/** Generate row widget */
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FNPCTableRow> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/** Selection changed */
	void OnSelectionChanged(TSharedPtr<FNPCTableRow> Item, ESelectInfo::Type SelectInfo);

	/** Column sort clicked */
	EColumnSortMode::Type GetColumnSortMode(FName ColumnId) const;
	void OnColumnSortModeChanged(EColumnSortPriority::Type Priority, const FName& ColumnId, EColumnSortMode::Type SortMode);

	//=========================================================================
	// Actions
	//=========================================================================

	/** Add new row */
	FReply OnAddRowClicked();

	/** Delete selected rows */
	FReply OnDeleteRowsClicked();

	/** Duplicate selected row */
	FReply OnDuplicateRowClicked();

	/** Generate assets from selected/all rows */
	FReply OnGenerateClicked();

	/** Sync from existing assets */
	FReply OnSyncFromAssetsClicked();

	/** Export to CSV */
	FReply OnExportCSVClicked();

	/** Import from CSV */
	FReply OnImportCSVClicked();

	/** Save the table data */
	FReply OnSaveClicked();

	/** Apply changes back to NPCDefinition assets */
	FReply OnApplyToAssetsClicked();

	//=========================================================================
	// Filtering & Sorting
	//=========================================================================

	/** Apply search and filters */
	void ApplyFilters();

	/** Apply sorting */
	void ApplySorting();

	/** Search text changed */
	void OnSearchTextChanged(const FText& NewText);

	//=========================================================================
	// Data Management
	//=========================================================================

	/** Sync AllRows from TableData */
	void SyncFromTableData();

	/** Mark table dirty */
	void MarkDirty();

	/** Row was modified callback */
	void OnRowModified();
};

/**
 * Tab/Window that hosts the NPC Table Editor
 */
class GASABILITYGENERATOR_API SNPCTableEditorWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNPCTableEditorWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	/** The table editor widget */
	TSharedPtr<SNPCTableEditor> TableEditor;

	/** Current table data asset */
	TWeakObjectPtr<UNPCTableData> CurrentTableData;

	/** Build menu bar */
	TSharedRef<SWidget> BuildMenuBar();

	/** File menu actions */
	void OnNewTable();
	void OnOpenTable();
	void OnSaveTable();
	void OnSaveTableAs();

	/** Create or get default table data */
	UNPCTableData* GetOrCreateTableData();
};
