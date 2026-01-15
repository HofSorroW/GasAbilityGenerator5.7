// GasAbilityGenerator - Dialogue Token Apply Preview Window
// v4.4: Full-screen preview for token changes before applying to UDialogueBlueprint
//
// Shows context (Quest, Dialogue, Speakers) on left panel
// Shows side-by-side Current vs New changes on right panel
// User can approve/deny each change and add notes

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "DialogueTableEditorTypes.h"

/**
 * Single token change entry for preview
 */
struct FTokenChangeEntry
{
	/** Row this change belongs to */
	TSharedPtr<FDialogueTableRow> Row;

	/** Node identification */
	FName NodeID;
	FString Speaker;
	FString DialogueText;

	/** Current state (from UE asset) */
	FString CurrentEvents;
	FString CurrentConditions;

	/** New state (from Excel import) */
	FString NewEvents;
	FString NewConditions;

	/** Whether events changed */
	bool bEventsChanged = false;

	/** Whether conditions changed */
	bool bConditionsChanged = false;

	/** User approval state */
	bool bEventsApproved = true;
	bool bConditionsApproved = true;

	/** User notes for this change */
	FString ApprovalNotes;

	/** Validation state */
	bool bEventsValid = true;
	bool bConditionsValid = true;
	FString EventsValidationError;
	FString ConditionsValidationError;

	bool HasChanges() const { return bEventsChanged || bConditionsChanged; }
	bool IsFullyApproved() const { return (!bEventsChanged || bEventsApproved) && (!bConditionsChanged || bConditionsApproved); }
};

/**
 * Context info for the left panel
 */
struct FTokenApplyContext
{
	FString QuestName;
	FName DialogueID;
	FString DialogueName;
	TArray<FString> Speakers;

	int32 TotalChanges = 0;
	int32 EventChanges = 0;
	int32 ConditionChanges = 0;
	int32 ApprovedCount = 0;
	int32 DeniedCount = 0;
};

/**
 * Result from the preview window
 */
struct FTokenApplyPreviewResult
{
	bool bConfirmed = false;
	TArray<FTokenChangeEntry> ApprovedChanges;
	int32 TotalApproved = 0;
	int32 TotalDenied = 0;
};

DECLARE_DELEGATE_OneParam(FOnTokenApplyConfirmed, const FTokenApplyPreviewResult&);

/**
 * Single change card widget - shows one node's changes
 */
class STokenChangeCard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STokenChangeCard) {}
		SLATE_ARGUMENT(TSharedPtr<FTokenChangeEntry>, ChangeEntry)
		SLATE_EVENT(FSimpleDelegate, OnApprovalChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedPtr<FTokenChangeEntry> Entry;
	FSimpleDelegate OnApprovalChanged;

	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildComparisonRow(const FString& Label, const FString& Current, const FString& New, bool bChanged, bool& bApproved, bool bValid, const FString& ValidationError);
	TSharedRef<SWidget> BuildNotesRow();

	void OnEventsApprovalChanged(ECheckBoxState NewState);
	void OnConditionsApprovalChanged(ECheckBoxState NewState);
	void OnNotesChanged(const FText& NewText);

	FSlateColor GetChangeColor(bool bChanged, bool bValid) const;
};

/**
 * Main preview window - shows all changes with context
 */
class GASABILITYGENERATOR_API SDialogueTokenApplyPreview : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDialogueTokenApplyPreview) {}
		SLATE_ARGUMENT(TArray<FDialogueTableRow>, ImportedRows)
		SLATE_ARGUMENT(FString, DialogueAssetPath)
		SLATE_EVENT(FOnTokenApplyConfirmed, OnConfirmed)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Static method to open preview window */
	static TSharedPtr<SWindow> OpenPreviewWindow(
		const TArray<FDialogueTableRow>& ImportedRows,
		const FString& DialogueAssetPath,
		FOnTokenApplyConfirmed OnConfirmed
	);

private:
	TArray<FDialogueTableRow> ImportedRows;
	FString DialogueAssetPath;
	FOnTokenApplyConfirmed OnConfirmed;

	FTokenApplyContext Context;
	TArray<TSharedPtr<FTokenChangeEntry>> ChangeEntries;

	TSharedPtr<SListView<TSharedPtr<FTokenChangeEntry>>> ChangeListView;

	// Context panel widgets for live updates
	TSharedPtr<STextBlock> ApprovedCountText;
	TSharedPtr<STextBlock> DeniedCountText;

	// Build UI
	TSharedRef<SWidget> BuildLeftPanel();
	TSharedRef<SWidget> BuildRightPanel();
	TSharedRef<SWidget> BuildBottomBar();

	// List view callbacks
	TSharedRef<ITableRow> OnGenerateChangeRow(TSharedPtr<FTokenChangeEntry> Entry, const TSharedRef<STableViewBase>& OwnerTable);

	// Load current state from UE assets and compare with imported
	void LoadAndCompareData();
	void UpdateContext();
	void UpdateApprovalCounts();

	// Actions
	FReply OnApplyClicked();
	FReply OnCancelClicked();
	void OnApprovalChanged();

	// Close the window
	TSharedPtr<SWindow> ParentWindow;
	void CloseWindow();
};
