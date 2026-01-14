// SNPCTableEditor.h
// PROTOTYPE - NPC Table Editor Slate Widget
// This is example code for review, not compiled with the plugin
//
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
 * Row widget for the NPC table
 * Handles display and inline editing of a single NPC row
 */
class SNPCTableRow : public SMultiColumnTableRow<TSharedPtr<FNPCTableRow>>
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
	TSharedRef<SWidget> CreateAssetPickerCell(FSoftObjectPath& Value, const FString& AllowedClass);

	/** Create status indicator */
	TSharedRef<SWidget> CreateStatusCell();

	/** Mark row as modified */
	void MarkModified();
};

/**
 * Main NPC Table Editor Widget
 * Excel-like spreadsheet view for managing all NPCs
 */
class SNPCTableEditor : public SCompoundWidget
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

	/** Current sort column */
	FName SortColumn;
	EColumnSortMode::Type SortMode = EColumnSortMode::None;

	/** Column filter values */
	TMap<FName, FString> ColumnFilters;

	//=========================================================================
	// UI Construction
	//=========================================================================

	/** Build the toolbar (Add, Delete, Generate, Sync buttons) */
	TSharedRef<SWidget> BuildToolbar();

	/** Build the header row */
	TSharedRef<SHeaderRow> BuildHeaderRow();

	/** Build filter row */
	TSharedRef<SWidget> BuildFilterRow();

	/** Build status bar */
	TSharedRef<SWidget> BuildStatusBar();

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

	//=========================================================================
	// Filtering & Sorting
	//=========================================================================

	/** Apply search and filters */
	void ApplyFilters();

	/** Apply sorting */
	void ApplySorting();

	/** Search text changed */
	void OnSearchTextChanged(const FText& NewText);

	/** Column filter changed */
	void OnColumnFilterChanged(FName ColumnId, const FString& FilterText);

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
class SNPCTableEditorWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNPCTableEditorWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	/** The table editor widget */
	TSharedPtr<SNPCTableEditor> TableEditor;

	/** Current table data asset */
	UNPCTableData* CurrentTableData = nullptr;

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
