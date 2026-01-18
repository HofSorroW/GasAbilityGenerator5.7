// GasAbilityGenerator - Quest XLSX Sync Dialog
// v4.12: Full-screen approval window with radio selection and row highlighting
// Per Table_Editors_Reference.md v4.11.1:
// - Radio-style selection (O/*)
// - Row highlighting (Yellow = unresolved, Green = resolved)
// - Apply only enabled when all resolved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "XLSXSupport/QuestXLSXSyncEngine.h"

class SWindow;

/**
 * User's action choice for the sync dialog
 */
enum class EQuestSyncDialogResult : uint8
{
	Cancel,      // User cancelled sync
	Apply        // User confirmed, apply changes
};

/**
 * v4.12: Full-screen sync approval dialog for Quest table
 * - Radio-style selection (O/*)
 * - Row highlighting (Yellow = unresolved, Green = resolved)
 * - Columns: Status, QuestName, StateID, Base (Last Export), UE, Excel, Action
 */
class GASABILITYGENERATOR_API SQuestXLSXSyncDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SQuestXLSXSyncDialog) {}
		SLATE_ARGUMENT(FQuestSyncResult*, SyncResult)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Show modal dialog, returns true if user clicked Apply */
	static bool ShowModal(FQuestSyncResult& SyncResult, TSharedPtr<SWindow> ParentWindow = nullptr);

	/** Get the user's choice */
	EQuestSyncDialogResult GetResult() const { return DialogResult; }

private:
	FQuestSyncResult* SyncResult = nullptr;
	EQuestSyncDialogResult DialogResult = EQuestSyncDialogResult::Cancel;
	TWeakPtr<SWindow> DialogWindow;

	// Entry list
	TArray<TSharedPtr<FQuestSyncEntry>> DisplayedEntries;
	TSharedPtr<SListView<TSharedPtr<FQuestSyncEntry>>> ListView;

	// Filter state
	bool bShowUnchanged = false;
	bool bShowConflictsOnly = false;

	// UI Building
	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildStatusBar();
	TSharedRef<SWidget> BuildEntryList();
	TSharedRef<SWidget> BuildFooter();

	// List callbacks
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FQuestSyncEntry> Item, const TSharedRef<STableViewBase>& OwnerTable);

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
 * v4.12: Row widget with radio-style selection and background highlighting
 * v4.12.2: Added validation column
 */
class SQuestSyncEntryRow : public SMultiColumnTableRow<TSharedPtr<FQuestSyncEntry>>
{
public:
	SLATE_BEGIN_ARGS(SQuestSyncEntryRow) {}
		SLATE_ARGUMENT(TSharedPtr<FQuestSyncEntry>, Entry)
		SLATE_EVENT(FSimpleDelegate, OnResolutionChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	TSharedPtr<FQuestSyncEntry> Entry;
	FSimpleDelegate OnResolutionChanged;

	// Column cells
	TSharedRef<SWidget> CreateStatusCell();
	TSharedRef<SWidget> CreateQuestNameCell();
	TSharedRef<SWidget> CreateStateIDCell();
	TSharedRef<SWidget> CreateBaseCell();
	TSharedRef<SWidget> CreateUECell();
	TSharedRef<SWidget> CreateExcelCell();
	TSharedRef<SWidget> CreateActionCell();

	// v4.12.2: Validation column
	TSharedRef<SWidget> CreateValidationCell();
};
