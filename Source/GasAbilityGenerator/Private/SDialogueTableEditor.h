// GasAbilityGenerator - Dialogue Table Editor
// v4.4: Added validation error feedback in status bar
// v4.3: Slate widget for batch dialogue creation with XLSX sync support
//
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/SHeaderRow.h"
#include "DialogueTableEditorTypes.h"

class SEditableText;
class SSearchBox;

/**
 * Column definition for the dialogue table
 */
struct FDialogueTableColumn
{
	FName ColumnId;
	FText DisplayName;
	float DefaultWidth;

	FDialogueTableColumn(FName InId, const FText& InName, float InWidth)
		: ColumnId(InId), DisplayName(InName), DefaultWidth(InWidth) {}
};

/** Get default column definitions for dialogue table (11 columns) */
inline TArray<FDialogueTableColumn> GetDialogueTableColumns()
{
	return {
		{ TEXT("Seq"),          FText::FromString(TEXT("Seq")),            0.03f },
		{ TEXT("DialogueID"),   FText::FromString(TEXT("Dialogue ID")),    0.09f },
		{ TEXT("NodeID"),       FText::FromString(TEXT("Node ID")),        0.10f },
		{ TEXT("NodeType"),     FText::FromString(TEXT("Type")),           0.05f },
		{ TEXT("Speaker"),      FText::FromString(TEXT("Speaker")),        0.08f },
		{ TEXT("Text"),         FText::FromString(TEXT("Text")),           0.20f },
		{ TEXT("OptionText"),   FText::FromString(TEXT("Option Text")),    0.10f },
		{ TEXT("ParentNodeID"), FText::FromString(TEXT("Parent")),         0.08f },
		{ TEXT("NextNodeIDs"),  FText::FromString(TEXT("Next Nodes")),     0.10f },
		{ TEXT("Skippable"),    FText::FromString(TEXT("Skip")),           0.04f },
		{ TEXT("Notes"),        FText::FromString(TEXT("Notes")),          0.13f },
	};
}

/**
 * Per-column filter state (supports multi-select)
 */
struct FColumnFilterState
{
	FString TextFilter;
	TSet<FString> SelectedValues;  // Multi-select: empty = all, otherwise OR logic
	TArray<TSharedPtr<FString>> DropdownOptions;
};

/**
 * Extended row data with calculated sequence/depth info
 */
struct FDialogueTableRowEx
{
	TSharedPtr<FDialogueTableRow> Data;
	int32 Sequence = 0;      // Order within dialogue (1, 2, 3...)
	int32 Depth = 0;         // Tree depth (0 = root, 1 = child, 2 = grandchild...)
	FString SeqDisplay;      // Display string like "1", "1.1", "1.2"
};

/**
 * Row widget for the dialogue table
 */
class SDialogueTableRow : public SMultiColumnTableRow<TSharedPtr<FDialogueTableRowEx>>
{
public:
	SLATE_BEGIN_ARGS(SDialogueTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FDialogueTableRowEx>, RowData)
		SLATE_EVENT(FSimpleDelegate, OnRowModified)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	TSharedPtr<FDialogueTableRowEx> RowDataEx;
	FSimpleDelegate OnRowModified;

	TSharedRef<SWidget> CreateTextCell(FString& Value, const FString& Hint = TEXT(""), bool bWithTooltip = false);
	TSharedRef<SWidget> CreateFNameCell(FName& Value, const FString& Hint = TEXT(""));
	TSharedRef<SWidget> CreateNodeTypeCell();
	TSharedRef<SWidget> CreateSpeakerCell();  // Shows "Player" for player nodes
	TSharedRef<SWidget> CreateNextNodesCell();
	TSharedRef<SWidget> CreateSeqCell();
	TSharedRef<SWidget> CreateNodeIDCell();  // With indentation
	TSharedRef<SWidget> CreateSkippableCell();  // Yes/No checkbox
	TSharedRef<SWidget> CreateNotesCell();  // Designer notes

	void MarkModified();
};

/**
 * Main Dialogue Table Editor Widget
 * 9-column table for managing dialogue trees with sequence tracking
 */
class SDialogueTableEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDialogueTableEditor) {}
		SLATE_ARGUMENT(UDialogueTableData*, TableData)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void RefreshList();
	TArray<TSharedPtr<FDialogueTableRowEx>> GetSelectedRows() const;

	/** Set the table data and refresh */
	void SetTableData(UDialogueTableData* InTableData);

private:
	UDialogueTableData* TableData = nullptr;
	TArray<TSharedPtr<FDialogueTableRowEx>> DisplayedRows;
	TArray<TSharedPtr<FDialogueTableRowEx>> AllRows;

	TSharedPtr<SListView<TSharedPtr<FDialogueTableRowEx>>> ListView;
	TSharedPtr<SHeaderRow> HeaderRow;

	FName SortColumn;
	EColumnSortMode::Type SortMode = EColumnSortMode::None;
	TMap<FName, FColumnFilterState> ColumnFilters;  // Per-column filter state

	// UI Construction
	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SHeaderRow> BuildHeaderRow();
	TSharedRef<SWidget> BuildStatusBar();

	// v4.2.13: Status bar with stored widget references for direct SetText() updates
	TSharedPtr<STextBlock> StatusTotalText;
	TSharedPtr<STextBlock> StatusDialoguesText;
	TSharedPtr<STextBlock> StatusShowingText;
	TSharedPtr<STextBlock> StatusSelectedText;
	TSharedPtr<STextBlock> StatusValidationText;  // v4.4: Token validation errors
	void UpdateStatusBar();  // Explicitly update all status text

	// List View Callbacks
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FDialogueTableRowEx> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnSelectionChanged(TSharedPtr<FDialogueTableRowEx> Item, ESelectInfo::Type SelectInfo);
	EColumnSortMode::Type GetColumnSortMode(FName ColumnId) const;
	void OnColumnSortModeChanged(EColumnSortPriority::Type Priority, const FName& ColumnId, EColumnSortMode::Type NewSortMode);

	// Actions
	FReply OnAddRowClicked();
	FReply OnDeleteRowsClicked();
	FReply OnDeleteBranchClicked();  // Cascade delete
	FReply OnClearFiltersClicked();  // Clear all filters
	FReply OnGenerateClicked();
	FReply OnExportCSVClicked();
	FReply OnExportXLSXClicked();  // v4.3: XLSX export with sync support
	FReply OnImportCSVClicked();
	FReply OnImportXLSXClicked();  // v4.3: XLSX import with sync support
	FReply OnSyncXLSXClicked();    // v4.3: XLSX sync with 3-way merge
	FReply OnSyncFromAssetsClicked();  // v4.4: Pull tokens from UDialogueBlueprint
	FReply OnApplyToAssetsClicked();  // v4.4: Apply tokens to UDialogueBlueprint
	FReply OnValidateClicked();
	FReply OnResetOrderClicked();

	// Filtering & Sorting
	void ApplyFilters();
	void ApplySorting();
	void ApplyFlowOrder();  // Sort by dialogue flow
	void InitializeColumnFilters();
	void UpdateColumnFilterOptions();
	void OnColumnTextFilterChanged(FName ColumnId, const FText& NewText);
	void OnColumnDropdownFilterChanged(FName ColumnId, TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
	FString GetColumnValue(const FDialogueTableRowEx& Row, FName ColumnId) const;
	TSharedRef<SWidget> BuildColumnHeaderContent(const FDialogueTableColumn& Col);

	// Sequence/Depth Calculation
	void CalculateSequences();
	int32 CalculateNodeDepth(const FDialogueTableRow& Row, const TMap<FName, TSharedPtr<FDialogueTableRowEx>>& NodeMap, TSet<FName>& Visited);

	// Data Management
	void SyncFromTableData();
	void SyncToTableData();
	void MarkDirty();
	void OnRowModified();

	// CSV Import/Export
	bool ImportFromCSV(const FString& FilePath);
	bool ExportToCSV(const FString& FilePath);
	TArray<FString> ParseCSVLine(const FString& Line);  // Proper CSV parsing
	FString EscapeCSVField(const FString& Field);
};

/**
 * Window that hosts the Dialogue Table Editor
 */
class SDialogueTableEditorWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDialogueTableEditorWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedPtr<SDialogueTableEditor> TableEditor;
	UDialogueTableData* CurrentTableData = nullptr;

	TSharedRef<SWidget> BuildMenuBar();

	void OnNewTable();
	void OnOpenTable();
	void OnSaveTable();

	UDialogueTableData* GetOrCreateTableData();
};
