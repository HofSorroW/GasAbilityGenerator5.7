// GasAbilityGenerator - NPC Apply Preview Window
// v4.5: Full-screen preview for NPC changes before applying to UNPCDefinition
//
// Shows summary (total, modified, new, skipped) on left panel
// Shows side-by-side Current vs New changes on right panel
// User can approve/deny each NPC change and add notes

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "NPCTableEditor/NPCTableEditorTypes.h"

/**
 * Single field change for preview
 */
struct FNPCFieldChange
{
	FString FieldName;
	FString CurrentValue;
	FString NewValue;
	bool bChanged = false;
};

/**
 * Single NPC change entry for preview
 */
struct FNPCChangeEntry
{
	/** Row this change belongs to */
	TSharedPtr<FNPCTableRow> Row;

	/** NPC identification */
	FString NPCName;
	FString NPCId;
	FString DisplayName;

	/** Whether this is a new NPC (no existing asset) */
	bool bIsNew = false;

	/** Whether the existing asset was found */
	bool bAssetFound = false;

	/** Field changes */
	TArray<FNPCFieldChange> FieldChanges;

	/** User approval state */
	bool bApproved = true;

	/** User notes for this change */
	FString ApprovalNotes;

	/** Validation state */
	bool bValid = true;
	FString ValidationError;

	bool HasChanges() const { return FieldChanges.Num() > 0 || bIsNew; }
	int32 GetChangedFieldCount() const
	{
		int32 Count = 0;
		for (const FNPCFieldChange& Field : FieldChanges)
		{
			if (Field.bChanged) Count++;
		}
		return Count;
	}
};

/**
 * Context info for the left panel
 */
struct FNPCApplyContext
{
	int32 TotalNPCs = 0;
	int32 ModifiedCount = 0;
	int32 NewCount = 0;
	int32 UnchangedCount = 0;
	int32 InvalidCount = 0;
	int32 ApprovedCount = 0;
	int32 DeniedCount = 0;

	FString OutputFolder;
};

/**
 * Result from the preview window
 */
struct FNPCApplyPreviewResult
{
	bool bConfirmed = false;
	TArray<FNPCChangeEntry> ApprovedChanges;
	int32 TotalApproved = 0;
	int32 TotalDenied = 0;
};

DECLARE_DELEGATE_OneParam(FOnNPCApplyConfirmed, const FNPCApplyPreviewResult&);

/**
 * Single NPC change card widget - shows one NPC's changes
 */
class SNPCChangeCard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNPCChangeCard) {}
		SLATE_ARGUMENT(TSharedPtr<FNPCChangeEntry>, ChangeEntry)
		SLATE_EVENT(FSimpleDelegate, OnApprovalChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedPtr<FNPCChangeEntry> Entry;
	FSimpleDelegate OnApprovalChanged;

	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildFieldChanges();
	TSharedRef<SWidget> BuildFieldRow(const FNPCFieldChange& Field);
	TSharedRef<SWidget> BuildNotesRow();

	void OnApprovalCheckChanged(ECheckBoxState NewState);
};

/**
 * Main preview window - shows all NPC changes with context
 */
class GASABILITYGENERATOR_API SNPCApplyPreview : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNPCApplyPreview) {}
		SLATE_ARGUMENT(TArray<FNPCTableRow>, RowsToApply)
		SLATE_ARGUMENT(FString, NPCAssetPath)
		SLATE_EVENT(FOnNPCApplyConfirmed, OnConfirmed)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Static method to open preview window */
	static TSharedPtr<SWindow> OpenPreviewWindow(
		const TArray<FNPCTableRow>& RowsToApply,
		const FString& NPCAssetPath,
		FOnNPCApplyConfirmed OnConfirmed
	);

private:
	TArray<FNPCTableRow> RowsToApply;
	FString NPCAssetPath;
	FOnNPCApplyConfirmed OnConfirmed;

	FNPCApplyContext Context;
	TArray<TSharedPtr<FNPCChangeEntry>> ChangeEntries;

	TSharedPtr<SListView<TSharedPtr<FNPCChangeEntry>>> ChangeListView;

	// Context panel widgets for live updates
	TSharedPtr<STextBlock> ApprovedCountText;
	TSharedPtr<STextBlock> DeniedCountText;

	// Build UI
	TSharedRef<SWidget> BuildLeftPanel();
	TSharedRef<SWidget> BuildRightPanel();
	TSharedRef<SWidget> BuildBottomBar();

	// List view callbacks
	TSharedRef<ITableRow> OnGenerateChangeRow(TSharedPtr<FNPCChangeEntry> Entry, const TSharedRef<STableViewBase>& OwnerTable);

	// Load current state from UE assets and compare with rows
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
