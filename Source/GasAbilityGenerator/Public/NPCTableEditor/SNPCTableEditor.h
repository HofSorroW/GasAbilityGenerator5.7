// SNPCTableEditor.h
// NPC Table Editor Slate Widget - Excel-like spreadsheet for managing NPCs
// v4.5: Added validation and generation with FNPCDefinitionGenerator
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
 * v4.12.5: Changed to ManualWidth (fixed pixels) for consistent sizing
 */
struct FNPCTableColumn
{
	FName ColumnId;
	FText DisplayName;
	float ManualWidth;  // Fixed pixel width

	FNPCTableColumn(FName InId, const FText& InName, float InWidth = 80.0f)
		: ColumnId(InId)
		, DisplayName(InName)
		, ManualWidth(InWidth)
	{}
};

/**
 * Per-column filter state (multi-select support)
 */
struct FNPCColumnFilterState
{
	FString TextFilter;
	TSet<FString> SelectedValues;  // Empty = all, populated = only these values shown
	TArray<TSharedPtr<FString>> DropdownOptions;
};

/**
 * Column input type for NPC table
 */
enum class ENPCColumnInputType : uint8
{
	ReadOnly,           // Status - auto-calculated
	TextInput,          // NPC Name, NPC ID, Display Name, Shop Name, Notes
	AssetDropdown,      // Blueprint, AbilityConfig, ActivityConfig, Schedule, BehaviorTree, Appearance
	MultiSelectDropdown,// Factions, DefaultItems
	LevelRange,         // Min-Max combined display
	FloatSlider,        // AttackPriority (0.0-1.0)
	Checkbox            // IsVendor
};

/**
 * Get default column definitions for NPC table (v4.12.7 - 19 columns)
 * v4.12.7: Renamed columns (NPC File, NPC Name), removed NPCId/ShopName, added Dialogues/Items/Tags
 * v4.12.5: Changed to ManualWidth (fixed pixels) for consistent sizing
 */
inline TArray<FNPCTableColumn> GetNPCTableColumns()
{
	TArray<FNPCTableColumn> Columns;

	//=========================================================================
	// Core Identity (4 columns)
	//=========================================================================

	// 1. Status - read-only badge (auto-calculated)
	Columns.Add(FNPCTableColumn(TEXT("Status"), NSLOCTEXT("NPCTableEditor", "ColStatus", "Status"), 55.0f));

	// 2. NPCName - asset file name (required, no spaces) - v4.12.7: renamed from "NPC Name" to "NPC File"
	Columns.Add(FNPCTableColumn(TEXT("NPCName"), NSLOCTEXT("NPCTableEditor", "ColNPCName", "NPC File"), 120.0f));

	// 3. DisplayName - in-game display name - v4.12.7: renamed from "Display Name" to "NPC Name"
	Columns.Add(FNPCTableColumn(TEXT("DisplayName"), NSLOCTEXT("NPCTableEditor", "ColDisplayName", "NPC Name"), 120.0f));

	// 4. Blueprint - asset dropdown (ANarrativeNPCCharacter classes)
	Columns.Add(FNPCTableColumn(TEXT("Blueprint"), NSLOCTEXT("NPCTableEditor", "ColBlueprint", "Blueprint"), 120.0f));

	//=========================================================================
	// AI & Behavior (3 columns)
	//=========================================================================

	// 5. AbilityConfig - asset dropdown (AC_* DataAssets)
	Columns.Add(FNPCTableColumn(TEXT("AbilityConfig"), NSLOCTEXT("NPCTableEditor", "ColAbilityConfig", "Ability Cfg"), 100.0f));

	// 6. ActivityConfig - asset dropdown (AC_*Behavior, UNPCActivityConfiguration)
	Columns.Add(FNPCTableColumn(TEXT("ActivityConfig"), NSLOCTEXT("NPCTableEditor", "ColActivityConfig", "Activity Cfg"), 100.0f));

	// 7. Schedule - asset dropdown (Schedule_* DataAssets)
	Columns.Add(FNPCTableColumn(TEXT("Schedule"), NSLOCTEXT("NPCTableEditor", "ColSchedule", "Schedule"), 90.0f));

	//=========================================================================
	// Combat (3 columns)
	//=========================================================================

	// 8. LevelRange - dual spinbox "1-10"
	Columns.Add(FNPCTableColumn(TEXT("LevelRange"), NSLOCTEXT("NPCTableEditor", "ColLevelRange", "Level"), 65.0f));

	// 9. Factions - multi-select dropdown (gameplay tags)
	Columns.Add(FNPCTableColumn(TEXT("Factions"), NSLOCTEXT("NPCTableEditor", "ColFactions", "Factions"), 100.0f));

	// 10. AttackPriority - slider 0.0-1.0 - v4.12.7: renamed from "Prio" to "AP"
	Columns.Add(FNPCTableColumn(TEXT("AttackPriority"), NSLOCTEXT("NPCTableEditor", "ColAttackPriority", "AP"), 50.0f));

	//=========================================================================
	// Vendor (1 column) - v4.12.7: Removed ShopName column
	//=========================================================================

	// 11. bIsVendor - checkbox - v4.12.7: renamed from "V" to "Vendor"
	Columns.Add(FNPCTableColumn(TEXT("bIsVendor"), NSLOCTEXT("NPCTableEditor", "ColIsVendor", "Vendor"), 50.0f));

	//=========================================================================
	// Dialogues (1 column) - v4.12.7: New column
	//=========================================================================

	// 12. Dialogues - shows Dialogue + TaggedDialogueSet assets (both clickable)
	Columns.Add(FNPCTableColumn(TEXT("Dialogues"), NSLOCTEXT("NPCTableEditor", "ColDialogues", "Dialogues"), 150.0f));

	//=========================================================================
	// Items (2 columns) - v4.12.7: Renamed DefaultItems to IC, added Items column
	//=========================================================================

	// 13. DefaultItems - multi-select dropdown (IC_* collections) - v4.12.7: renamed from "Items" to "IC"
	Columns.Add(FNPCTableColumn(TEXT("DefaultItems"), NSLOCTEXT("NPCTableEditor", "ColDefaultItems", "IC"), 100.0f));

	// 14. InventoryItems - v4.12.7: New column for individual items with quantity
	Columns.Add(FNPCTableColumn(TEXT("InventoryItems"), NSLOCTEXT("NPCTableEditor", "ColInventoryItems", "Items"), 130.0f));

	// 15. SpawnerPOI - asset dropdown (POI tags from level)
	Columns.Add(FNPCTableColumn(TEXT("SpawnerPOI"), NSLOCTEXT("NPCTableEditor", "ColSpawnerPOI", "POI"), 100.0f));

	//=========================================================================
	// Meta (3 columns) - v4.12.7: Added Tags column
	//=========================================================================

	// 16. Appearance - asset dropdown (Appearance_* assets)
	Columns.Add(FNPCTableColumn(TEXT("Appearance"), NSLOCTEXT("NPCTableEditor", "ColAppearance", "Appearance"), 100.0f));

	// 17. Tags - multi-select dropdown (gameplay tags from DefaultTags.ini)
	Columns.Add(FNPCTableColumn(TEXT("Tags"), NSLOCTEXT("NPCTableEditor", "ColTags", "Tags"), 120.0f));

	// 18. Notes - text input (free text, with tooltip)
	Columns.Add(FNPCTableColumn(TEXT("Notes"), NSLOCTEXT("NPCTableEditor", "ColNotes", "Notes"), 150.0f));

	return Columns;
}

/**
 * Row widget for the NPC table (v4.1 - 18 columns)
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

	//=========================================================================
	// Cell Widget Creators
	//=========================================================================

	/** Create status indicator (read-only badge) */
	TSharedRef<SWidget> CreateStatusCell();

	/** Create text cell (editable) */
	TSharedRef<SWidget> CreateTextCell(FString& Value, const FString& Hint = TEXT(""));

	/** Create NPC name cell with NPC_ prefix trimming */
	TSharedRef<SWidget> CreateNPCNameCell();

	/** Create checkbox cell */
	TSharedRef<SWidget> CreateCheckboxCell(bool& Value);

	/** Create level range cell (dual spinbox "1-10") */
	TSharedRef<SWidget> CreateLevelRangeCell();

	/** Create float slider cell (0.0-1.0) */
	TSharedRef<SWidget> CreateFloatSliderCell(float& Value);

	/** Create asset dropdown cell - filtered by class/prefix */
	TSharedRef<SWidget> CreateAssetDropdownCell(FSoftObjectPath& Value, UClass* AssetClass, const FString& AssetPrefix);

	/** Create NPC blueprint dropdown - only shows ANarrativeNPCCharacter subclasses */
	TSharedRef<SWidget> CreateNPCBlueprintDropdownCell();

	/** Create multi-select dropdown cell (comma-separated values) */
	TSharedRef<SWidget> CreateMultiSelectDropdownCell(FString& Value, const TArray<FString>& Options);

	/** Create factions multi-select dropdown */
	TSharedRef<SWidget> CreateFactionsCell();

	/** Create items multi-select dropdown (IC_* collections) */
	TSharedRef<SWidget> CreateItemsCell();

	/** Create inventory items cell with quantity (v4.12.7) */
	TSharedRef<SWidget> CreateInventoryItemsCell();

	/** Create dialogues cell showing Dialogue + TaggedDialogueSet (v4.12.7) */
	TSharedRef<SWidget> CreateDialoguesCell();

	/** Create tags multi-select dropdown (v4.12.7) */
	TSharedRef<SWidget> CreateTagsCell();

	/** Create POI dropdown (from level POIs) */
	TSharedRef<SWidget> CreatePOIDropdownCell();

	//=========================================================================
	// Asset Scanning Helpers
	//=========================================================================

	/** Get all assets of given class with optional prefix filter */
	static TArray<FString> GetAssetsOfClass(UClass* AssetClass, const FString& Prefix = TEXT(""));

	/** Get all Blueprint classes inheriting from given class */
	static TArray<FString> GetBlueprintClassesOf(UClass* ParentClass, const FString& Prefix = TEXT(""));

	/** Get all faction gameplay tags */
	static TArray<FString> GetFactionTags();

	/** Get all item collection assets */
	static TArray<FString> GetItemCollections();

	/** Get all POI tags from level */
	static TArray<FString> GetPOITags();

	/** Mark row as modified */
	void MarkModified();
};

/** v4.6: Delegate for dirty state changes */
DECLARE_DELEGATE(FOnTableDirtyStateChanged);

/**
 * Main NPC Table Editor Widget
 * Excel-like spreadsheet view for managing all NPCs
 */
class GASABILITYGENERATOR_API SNPCTableEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNPCTableEditor) {}
		SLATE_ARGUMENT(UNPCTableData*, TableData)
		SLATE_EVENT(FOnTableDirtyStateChanged, OnDirtyStateChanged)  // v4.6
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

	/** Per-column filter state (text + multi-select) */
	TMap<FName, FNPCColumnFilterState> ColumnFilters;

	/** Current sort column */
	FName SortColumn;
	EColumnSortMode::Type SortMode = EColumnSortMode::None;

	//=========================================================================
	// Status Bar (v4.5 - stored widget references for direct SetText() updates)
	//=========================================================================
	TSharedPtr<STextBlock> StatusTotalText;
	TSharedPtr<STextBlock> StatusShowingText;
	TSharedPtr<STextBlock> StatusSelectedText;
	TSharedPtr<STextBlock> StatusValidationText;  // Validation errors (colored by state)

	/** Explicitly update all status bar text */
	void UpdateStatusBar();

	//=========================================================================
	// UI Construction
	//=========================================================================

	/** Build the toolbar (Add, Delete, Generate, Sync buttons) */
	TSharedRef<SWidget> BuildToolbar();

	/** Build the header row */
	TSharedRef<SHeaderRow> BuildHeaderRow();

	/** Build column header content (stacked: name + text filter + dropdown) */
	TSharedRef<SWidget> BuildColumnHeaderContent(const FNPCTableColumn& Col);

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

	/** Validate all rows */
	FReply OnValidateClicked();

	/** Generate assets from selected/all rows */
	FReply OnGenerateClicked();

	/** Sync from existing assets */
	FReply OnSyncFromAssetsClicked();

	/** Export to Excel (.xlsx) */
	FReply OnExportXLSXClicked();

	/** Import from Excel (.xlsx) */
	FReply OnImportXLSXClicked();

	/** Sync with Excel file (3-way merge) */
	FReply OnSyncXLSXClicked();

	/** Save the table data */
	FReply OnSaveClicked();

	/** Apply changes back to NPCDefinition assets */
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

	/** v4.6: Delegate for notifying owner of dirty state changes */
	FOnTableDirtyStateChanged OnDirtyStateChanged;

	/** v4.8.4: Re-entrancy guard - prevents double-clicks on long operations */
	bool bIsBusy = false;
};

/**
 * Tab/Window that hosts the NPC Table Editor
 * v4.6: Added dirty indicator and save-on-close prompt
 */
class GASABILITYGENERATOR_API SNPCTableEditorWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNPCTableEditorWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** v4.6: Set the parent tab for dirty indicator updates */
	void SetParentTab(TSharedPtr<SDockTab> InTab);

private:
	/** The table editor widget */
	TSharedPtr<SNPCTableEditor> TableEditor;

	/** Current table data asset */
	TWeakObjectPtr<UNPCTableData> CurrentTableData;

	/** v4.6: Parent tab for dirty indicator */
	TWeakPtr<SDockTab> ParentTab;

	/** v4.6: Base tab label (without dirty indicator) */
	FText BaseTabLabel;

	/** Build menu bar */
	TSharedRef<SWidget> BuildMenuBar();

	/** File menu actions */
	void OnNewTable();
	void OnOpenTable();
	void OnSaveTable();
	void OnSaveTableAs();

	/** Create or get default table data */
	UNPCTableData* GetOrCreateTableData();

	/** v4.6: Update tab label with dirty indicator */
	void UpdateTabLabel();

	/** v4.6: Check if TableData is dirty */
	bool IsTableDirty() const;

	/** v4.6: Handle tab close request - prompt to save if dirty (FCanCloseTab delegate) */
	bool CanCloseTab() const;
};
