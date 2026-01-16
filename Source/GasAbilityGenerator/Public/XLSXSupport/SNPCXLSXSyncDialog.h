// GasAbilityGenerator - NPC XLSX Sync Dialog
// v4.11.1: Full-screen approval window with radio selection and row highlighting
// v4.5: UI for reviewing and resolving NPC sync conflicts

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "XLSXSupport/NPCXLSXSyncEngine.h"

class SWindow;

/**
 * User's action choice for the sync dialog
 */
enum class ENPCSyncDialogResult : uint8
{
	Cancel,      // User cancelled sync
	Apply        // User confirmed, apply changes
};

/**
 * v4.11.1: Full-screen sync approval dialog
 * - Radio-style selection (○/●)
 * - Row highlighting (Yellow = unresolved, Green = resolved)
 * - Columns: Status, NPCName, NPCId, Base (Last Export), UE, Excel, Action
 */
class GASABILITYGENERATOR_API SNPCXLSXSyncDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNPCXLSXSyncDialog) {}
		SLATE_ARGUMENT(FNPCSyncResult*, SyncResult)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Show modal dialog, returns true if user clicked Apply */
	static bool ShowModal(FNPCSyncResult& SyncResult, TSharedPtr<SWindow> ParentWindow = nullptr);

	/** Get the user's choice */
	ENPCSyncDialogResult GetResult() const { return DialogResult; }

private:
	FNPCSyncResult* SyncResult = nullptr;
	ENPCSyncDialogResult DialogResult = ENPCSyncDialogResult::Cancel;
	TWeakPtr<SWindow> DialogWindow;

	// Entry list
	TArray<TSharedPtr<FNPCSyncEntry>> DisplayedEntries;
	TSharedPtr<SListView<TSharedPtr<FNPCSyncEntry>>> ListView;

	// Filter state
	bool bShowUnchanged = false;
	bool bShowConflictsOnly = false;

	// UI Building
	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildStatusBar();
	TSharedRef<SWidget> BuildEntryList();
	TSharedRef<SWidget> BuildFooter();

	// List callbacks
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FNPCSyncEntry> Item, const TSharedRef<STableViewBase>& OwnerTable);

	// Actions
	FReply OnApplyClicked();
	FReply OnCancelClicked();
	void OnShowUnchangedChanged(ECheckBoxState NewState);
	void OnShowConflictsOnlyChanged(ECheckBoxState NewState);

	// Helpers
	void RefreshEntryList();
	FText GetSummaryText() const;
	bool CanApply() const;
};

/**
 * v4.11.1: Row widget with radio-style selection and background highlighting
 */
class SNPCSyncEntryRow : public SMultiColumnTableRow<TSharedPtr<FNPCSyncEntry>>
{
public:
	SLATE_BEGIN_ARGS(SNPCSyncEntryRow) {}
		SLATE_ARGUMENT(TSharedPtr<FNPCSyncEntry>, Entry)
		SLATE_EVENT(FSimpleDelegate, OnResolutionChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	TSharedPtr<FNPCSyncEntry> Entry;
	FSimpleDelegate OnResolutionChanged;

	// v4.11.1: New column cells
	TSharedRef<SWidget> CreateStatusCell();
	TSharedRef<SWidget> CreateNPCNameCell();
	TSharedRef<SWidget> CreateNPCIdCell();
	TSharedRef<SWidget> CreateBaseCell();
	TSharedRef<SWidget> CreateUECell();
	TSharedRef<SWidget> CreateExcelCell();
	TSharedRef<SWidget> CreateActionCell();

	// Legacy cells (kept for compatibility)
	TSharedRef<SWidget> CreatePreviewCell();
	TSharedRef<SWidget> CreateResolutionCell();
};
