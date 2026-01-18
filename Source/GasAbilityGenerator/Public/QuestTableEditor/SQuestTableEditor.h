// SQuestTableEditor.h
// Quest Table Editor Slate Widget - Excel-like spreadsheet for managing Quests
// v4.8: Follows NPC/Dialogue table patterns with XLSX sync, validation cache, soft delete
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/SHeaderRow.h"
#include "QuestTableEditorTypes.h"

class SEditableText;
class SCheckBox;
class SSearchBox;

/**
 * Column definition for the Quest table
 * v4.12.5: Changed to ManualWidth (fixed pixels) for consistent sizing
 */
struct FQuestTableColumn
{
	FName ColumnId;
	FText DisplayName;
	float ManualWidth;  // Fixed pixel width

	FQuestTableColumn(FName InId, const FText& InName, float InWidth = 80.0f)
		: ColumnId(InId)
		, DisplayName(InName)
		, ManualWidth(InWidth)
	{}
};

/**
 * Per-column filter state (multi-select support)
 */
struct FQuestColumnFilterState
{
	FString TextFilter;
	TSet<FString> SelectedValues;  // Empty = all, populated = only these values shown
	TArray<TSharedPtr<FString>> DropdownOptions;
};

/**
 * Get default column definitions for Quest table (12 columns)
 * v4.12.5: Changed to ManualWidth (fixed pixels) for consistent sizing
 */
inline TArray<FQuestTableColumn> GetQuestTableColumns()
{
	TArray<FQuestTableColumn> Columns;

	//=========================================================================
	// Core Identity (3 columns)
	//=========================================================================

	// 1. Status - read-only badge (auto-calculated)
	Columns.Add(FQuestTableColumn(TEXT("Status"), NSLOCTEXT("QuestTableEditor", "ColStatus", "Status"), 55.0f));

	// 2. QuestName - groups rows into same quest
	Columns.Add(FQuestTableColumn(TEXT("QuestName"), NSLOCTEXT("QuestTableEditor", "ColQuestName", "Quest"), 130.0f));

	// 3. DisplayName - shown in quest log
	Columns.Add(FQuestTableColumn(TEXT("DisplayName"), NSLOCTEXT("QuestTableEditor", "ColDisplayName", "Display Name"), 130.0f));

	//=========================================================================
	// State Definition (4 columns)
	//=========================================================================

	// 4. StateID - unique within quest
	Columns.Add(FQuestTableColumn(TEXT("StateID"), NSLOCTEXT("QuestTableEditor", "ColStateID", "State ID"), 100.0f));

	// 5. StateType - Regular/Success/Failure
	Columns.Add(FQuestTableColumn(TEXT("StateType"), NSLOCTEXT("QuestTableEditor", "ColStateType", "Type"), 70.0f));

	// 6. Description - shown in quest tracker
	Columns.Add(FQuestTableColumn(TEXT("Description"), NSLOCTEXT("QuestTableEditor", "ColDescription", "Description"), 180.0f));

	// 7. ParentBranch - branch leading TO this state
	Columns.Add(FQuestTableColumn(TEXT("ParentBranch"), NSLOCTEXT("QuestTableEditor", "ColParentBranch", "Parent"), 80.0f));

	//=========================================================================
	// Logic (3 columns) - Token-based
	//=========================================================================

	// 8. Tasks - token string: BPT_FindItem(Item=EI_Ore,Count=10)
	Columns.Add(FQuestTableColumn(TEXT("Tasks"), NSLOCTEXT("QuestTableEditor", "ColTasks", "Tasks"), 160.0f));

	// 9. Events - token string: NE_GiveXP(Amount=50)
	Columns.Add(FQuestTableColumn(TEXT("Events"), NSLOCTEXT("QuestTableEditor", "ColEvents", "Events"), 160.0f));

	// 10. Conditions - token string: NC_HasItem(Item=EI_Key)
	Columns.Add(FQuestTableColumn(TEXT("Conditions"), NSLOCTEXT("QuestTableEditor", "ColConditions", "Conditions"), 160.0f));

	//=========================================================================
	// Rewards & Meta (2 columns)
	//=========================================================================

	// 11. Rewards - token string: Reward(Currency=100,XP=50)
	Columns.Add(FQuestTableColumn(TEXT("Rewards"), NSLOCTEXT("QuestTableEditor", "ColRewards", "Rewards"), 160.0f));

	// 12. Notes - designer notes
	Columns.Add(FQuestTableColumn(TEXT("Notes"), NSLOCTEXT("QuestTableEditor", "ColNotes", "Notes"), 150.0f));

	return Columns;
}

/**
 * Extended row data with quest grouping info
 */
struct FQuestTableRowEx
{
	TSharedPtr<FQuestTableRow> Data;
	int32 QuestGroupIndex = 0;  // Index within quest group
	bool bIsFirstInQuest = false;  // First row of a quest (show quest name)
};

/**
 * Row widget for the Quest table (v4.8 - 12 columns)
 * Handles display and inline editing of a single Quest state row
 */
class GASABILITYGENERATOR_API SQuestTableRow : public SMultiColumnTableRow<TSharedPtr<FQuestTableRowEx>>
{
public:
	SLATE_BEGIN_ARGS(SQuestTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FQuestTableRowEx>, RowData)
		SLATE_EVENT(FSimpleDelegate, OnRowModified)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable);

	/** Generate widget for each column */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	TSharedPtr<FQuestTableRowEx> RowDataEx;
	FSimpleDelegate OnRowModified;

	//=========================================================================
	// Cell Widget Creators
	//=========================================================================

	/** Create status indicator (read-only badge) */
	TSharedRef<SWidget> CreateStatusCell();

	/** Create text cell (editable) */
	TSharedRef<SWidget> CreateTextCell(FString& Value, const FString& Hint = TEXT(""));

	/** Create state type dropdown */
	TSharedRef<SWidget> CreateStateTypeCell();

	/** Create token cell (Tasks, Events, Conditions, Rewards) with validation */
	TSharedRef<SWidget> CreateTokenCell(FString& Value, const FString& Hint);

	/** Create quest name cell (with visual grouping) */
	TSharedRef<SWidget> CreateQuestNameCell();

	/** Create notes cell with tooltip */
	TSharedRef<SWidget> CreateNotesCell();

	/** Mark row as modified */
	void MarkModified();
};

/** v4.8: Delegate for dirty state changes */
DECLARE_DELEGATE(FOnQuestTableDirtyStateChanged);

/**
 * Main Quest Table Editor Widget
 * Excel-like spreadsheet view for managing all Quest states
 */
class GASABILITYGENERATOR_API SQuestTableEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SQuestTableEditor) {}
		SLATE_ARGUMENT(UQuestTableData*, TableData)
		SLATE_EVENT(FOnQuestTableDirtyStateChanged, OnDirtyStateChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Refresh the table view */
	void RefreshList();

	/** Get selected rows */
	TArray<TSharedPtr<FQuestTableRowEx>> GetSelectedRows() const;

	/** Set the table data */
	void SetTableData(UQuestTableData* InTableData);

private:
	/** The data asset containing all rows */
	UQuestTableData* TableData = nullptr;

	/** Filtered/sorted view of rows */
	TArray<TSharedPtr<FQuestTableRowEx>> DisplayedRows;

	/** All rows as shared pointers */
	TArray<TSharedPtr<FQuestTableRowEx>> AllRows;

	/** The list view widget */
	TSharedPtr<SListView<TSharedPtr<FQuestTableRowEx>>> ListView;

	/** Header row */
	TSharedPtr<SHeaderRow> HeaderRow;

	/** Per-column filter state (text + multi-select) */
	TMap<FName, FQuestColumnFilterState> ColumnFilters;

	/** Current sort column */
	FName SortColumn;
	EColumnSortMode::Type SortMode = EColumnSortMode::None;

	//=========================================================================
	// Status Bar (stored widget references for direct SetText() updates)
	//=========================================================================
	TSharedPtr<STextBlock> StatusTotalText;
	TSharedPtr<STextBlock> StatusQuestsText;  // Unique quest count
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
	TSharedRef<SWidget> BuildColumnHeaderContent(const FQuestTableColumn& Col);

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
	FString GetColumnValue(const TSharedPtr<FQuestTableRowEx>& Row, FName ColumnId) const;

	//=========================================================================
	// List View Callbacks
	//=========================================================================

	/** Generate row widget */
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FQuestTableRowEx> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/** Selection changed */
	void OnSelectionChanged(TSharedPtr<FQuestTableRowEx> Item, ESelectInfo::Type SelectInfo);

	/** Column sort clicked */
	EColumnSortMode::Type GetColumnSortMode(FName ColumnId) const;
	void OnColumnSortModeChanged(EColumnSortPriority::Type Priority, const FName& ColumnId, EColumnSortMode::Type SortMode);

	//=========================================================================
	// Actions
	//=========================================================================

	/** Add new row */
	FReply OnAddRowClicked();

	/** Add new quest (adds Start state automatically) */
	FReply OnAddQuestClicked();

	/** Delete selected rows (soft delete) */
	FReply OnDeleteRowsClicked();

	/** Duplicate selected row */
	FReply OnDuplicateRowClicked();

	/** Validate all rows */
	FReply OnValidateClicked();

	/** Generate assets from selected/all rows */
	FReply OnGenerateClicked();

	/** Sync from existing Quest assets */
	FReply OnSyncFromAssetsClicked();

	/** Export to Excel (.xlsx) */
	FReply OnExportXLSXClicked();

	/** Import from Excel (.xlsx) */
	FReply OnImportXLSXClicked();

	/** XLSX 3-way sync */
	FReply OnSyncXLSXClicked();

	/** Save the table data */
	FReply OnSaveClicked();

	/** Apply changes back to Quest assets */
	FReply OnApplyToAssetsClicked();

	//=========================================================================
	// Filtering & Sorting
	//=========================================================================

	/** Apply all filters and refresh display */
	void ApplyFilters();

	/** Apply sorting to displayed rows */
	void ApplySorting();

	/** Apply quest grouping order */
	void ApplyQuestGrouping();

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
	FOnQuestTableDirtyStateChanged OnDirtyStateChanged;

	/** Re-entrancy guard - prevents double-clicks on long operations */
	bool bIsBusy = false;
};

/**
 * Tab/Window that hosts the Quest Table Editor
 * v4.8: Added dirty indicator and save-on-close prompt
 */
class GASABILITYGENERATOR_API SQuestTableEditorWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SQuestTableEditorWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Set the parent tab for dirty indicator updates */
	void SetParentTab(TSharedPtr<SDockTab> InTab);

private:
	/** The table editor widget */
	TSharedPtr<SQuestTableEditor> TableEditor;

	/** Current table data asset */
	TWeakObjectPtr<UQuestTableData> CurrentTableData;

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
	UQuestTableData* GetOrCreateTableData();

	/** Update tab label with dirty indicator */
	void UpdateTabLabel();

	/** Check if TableData is dirty */
	bool IsTableDirty() const;

	/** Handle tab close request - prompt to save if dirty */
	bool CanCloseTab() const;
};
