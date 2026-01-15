// GasAbilityGenerator - Dialogue XLSX Sync Dialog
// v4.3: UI for reviewing and resolving sync conflicts

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "XLSXSupport/DialogueXLSXSyncEngine.h"

class SWindow;

/**
 * User's action choice for the sync dialog
 */
enum class EDialogueSyncDialogResult : uint8
{
	Cancel,      // User cancelled sync
	Apply        // User confirmed, apply changes
};

/**
 * Sync dialog for reviewing Excel ↔ UE changes and resolving conflicts
 */
class GASABILITYGENERATOR_API SDialogueXLSXSyncDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDialogueXLSXSyncDialog) {}
		SLATE_ARGUMENT(FDialogueSyncResult*, SyncResult)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Show modal dialog, returns true if user clicked Apply */
	static bool ShowModal(FDialogueSyncResult& SyncResult, TSharedPtr<SWindow> ParentWindow = nullptr);

	/** Get the user's choice */
	EDialogueSyncDialogResult GetResult() const { return DialogResult; }

private:
	FDialogueSyncResult* SyncResult = nullptr;
	EDialogueSyncDialogResult DialogResult = EDialogueSyncDialogResult::Cancel;
	TWeakPtr<SWindow> DialogWindow;

	// Entry list
	TArray<TSharedPtr<FDialogueSyncEntry>> DisplayedEntries;
	TSharedPtr<SListView<TSharedPtr<FDialogueSyncEntry>>> ListView;

	// Filter state
	bool bShowUnchanged = false;
	bool bShowConflictsOnly = false;

	// UI Building
	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildEntryList();
	TSharedRef<SWidget> BuildFooter();

	// List callbacks
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FDialogueSyncEntry> Item, const TSharedRef<STableViewBase>& OwnerTable);

	// Actions
	FReply OnApplyClicked();
	FReply OnCancelClicked();
	void OnShowUnchangedChanged(ECheckBoxState NewState);
	void OnShowConflictsOnlyChanged(ECheckBoxState NewState);

	// Helpers
	void RefreshEntryList();
	void UpdateStats();
	FText GetSummaryText() const;
	bool CanApply() const;
};

/**
 * Row widget for sync entry display
 */
class SDialogueSyncEntryRow : public SMultiColumnTableRow<TSharedPtr<FDialogueSyncEntry>>
{
public:
	SLATE_BEGIN_ARGS(SDialogueSyncEntryRow) {}
		SLATE_ARGUMENT(TSharedPtr<FDialogueSyncEntry>, Entry)
		SLATE_EVENT(FSimpleDelegate, OnResolutionChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	TSharedPtr<FDialogueSyncEntry> Entry;
	FSimpleDelegate OnResolutionChanged;

	TSharedRef<SWidget> CreateStatusCell();
	TSharedRef<SWidget> CreateNodeIDCell();
	TSharedRef<SWidget> CreateDialogueIDCell();
	TSharedRef<SWidget> CreateTextPreviewCell();
	TSharedRef<SWidget> CreateResolutionCell();

	void OnResolutionSelected(TSharedPtr<FString> Selection, ESelectInfo::Type SelectInfo);
};
