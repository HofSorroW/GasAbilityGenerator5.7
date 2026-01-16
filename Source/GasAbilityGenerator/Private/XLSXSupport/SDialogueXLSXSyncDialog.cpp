// GasAbilityGenerator - Dialogue XLSX Sync Dialog Implementation
// v4.11: Show actual values, no pre-selection for non-Unchanged, row highlighting
// v4.3: UI for reviewing and resolving sync conflicts

#include "XLSXSupport/SDialogueXLSXSyncDialog.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "DialogueXLSXSyncDialog"

//=============================================================================
// SDialogueXLSXSyncDialog
//=============================================================================

void SDialogueXLSXSyncDialog::Construct(const FArguments& InArgs)
{
	SyncResult = InArgs._SyncResult;

	// Build display entries from sync result
	RefreshEntryList();

	ChildSlot
	[
		SNew(SVerticalBox)

		// Header with summary
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f)
		[
			BuildHeader()
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
		]

		// Entry list
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(4.0f)
		[
			BuildEntryList()
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
		]

		// Footer with buttons
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f)
		[
			BuildFooter()
		]
	];
}

bool SDialogueXLSXSyncDialog::ShowModal(FDialogueSyncResult& SyncResult, TSharedPtr<SWindow> ParentWindow)
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("SyncDialogTitle", "XLSX Sync - Review Changes"))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(900, 600))
		.SupportsMinimize(false)
		.SupportsMaximize(true);

	TSharedRef<SDialogueXLSXSyncDialog> Dialog = SNew(SDialogueXLSXSyncDialog)
		.SyncResult(&SyncResult);

	Dialog->DialogWindow = Window;

	Window->SetContent(Dialog);

	if (ParentWindow.IsValid())
	{
		FSlateApplication::Get().AddModalWindow(Window, ParentWindow);
	}
	else
	{
		FSlateApplication::Get().AddModalWindow(Window, FSlateApplication::Get().GetActiveTopLevelWindow());
	}

	return Dialog->GetResult() == EDialogueSyncDialogResult::Apply;
}

TSharedRef<SWidget> SDialogueXLSXSyncDialog::BuildHeader()
{
	return SNew(SVerticalBox)

		// Title
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 8)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("SyncHeader", "Review Sync Changes"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
		]

		// Summary
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
				.Text_Lambda([this]() { return GetSummaryText(); })
		]

		// Filter options
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 8, 0, 0)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 16, 0)
			[
				SNew(SCheckBox)
					.IsChecked(bShowUnchanged ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
					.OnCheckStateChanged(this, &SDialogueXLSXSyncDialog::OnShowUnchangedChanged)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("ShowUnchanged", "Show Unchanged"))
					]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
					.IsChecked(bShowConflictsOnly ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
					.OnCheckStateChanged(this, &SDialogueXLSXSyncDialog::OnShowConflictsOnlyChanged)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("ShowConflictsOnly", "Show Conflicts Only"))
					]
			]
		];
}

TSharedRef<SWidget> SDialogueXLSXSyncDialog::BuildEntryList()
{
	TSharedRef<SHeaderRow> HeaderRow = SNew(SHeaderRow)
		+ SHeaderRow::Column("Status")
			.DefaultLabel(LOCTEXT("StatusCol", "Status"))
			.FillWidth(0.15f)
		+ SHeaderRow::Column("DialogueID")
			.DefaultLabel(LOCTEXT("DialogueIDCol", "Dialogue"))
			.FillWidth(0.15f)
		+ SHeaderRow::Column("NodeID")
			.DefaultLabel(LOCTEXT("NodeIDCol", "Node ID"))
			.FillWidth(0.15f)
		+ SHeaderRow::Column("Text")
			.DefaultLabel(LOCTEXT("TextCol", "Text Preview"))
			.FillWidth(0.35f)
		+ SHeaderRow::Column("Resolution")
			.DefaultLabel(LOCTEXT("ResolutionCol", "Resolution"))
			.FillWidth(0.20f);

	return SAssignNew(ListView, SListView<TSharedPtr<FDialogueSyncEntry>>)
		.ListItemsSource(&DisplayedEntries)
		.OnGenerateRow(this, &SDialogueXLSXSyncDialog::OnGenerateRow)
		.HeaderRow(HeaderRow)
		.SelectionMode(ESelectionMode::None);
}

TSharedRef<SWidget> SDialogueXLSXSyncDialog::BuildFooter()
{
	return SNew(SHorizontalBox)

		// Spacer
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNullWidget::NullWidget
		]

		// Cancel button
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4, 0)
		[
			SNew(SButton)
				.Text(LOCTEXT("Cancel", "Cancel"))
				.OnClicked(this, &SDialogueXLSXSyncDialog::OnCancelClicked)
		]

		// Apply button
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4, 0)
		[
			SNew(SButton)
				.Text(LOCTEXT("Apply", "Apply Changes"))
				.OnClicked(this, &SDialogueXLSXSyncDialog::OnApplyClicked)
				.IsEnabled_Lambda([this]() { return CanApply(); })
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
		];
}

TSharedRef<ITableRow> SDialogueXLSXSyncDialog::OnGenerateRow(TSharedPtr<FDialogueSyncEntry> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SDialogueSyncEntryRow, OwnerTable)
		.Entry(Item)
		.OnResolutionChanged_Lambda([this]()
		{
			// Force refresh to update Apply button state
			ListView->RequestListRefresh();
		});
}

FReply SDialogueXLSXSyncDialog::OnApplyClicked()
{
	DialogResult = EDialogueSyncDialogResult::Apply;
	if (DialogWindow.IsValid())
	{
		DialogWindow.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

FReply SDialogueXLSXSyncDialog::OnCancelClicked()
{
	DialogResult = EDialogueSyncDialogResult::Cancel;
	if (DialogWindow.IsValid())
	{
		DialogWindow.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

void SDialogueXLSXSyncDialog::OnShowUnchangedChanged(ECheckBoxState NewState)
{
	bShowUnchanged = (NewState == ECheckBoxState::Checked);
	RefreshEntryList();
}

void SDialogueXLSXSyncDialog::OnShowConflictsOnlyChanged(ECheckBoxState NewState)
{
	bShowConflictsOnly = (NewState == ECheckBoxState::Checked);
	RefreshEntryList();
}

void SDialogueXLSXSyncDialog::RefreshEntryList()
{
	DisplayedEntries.Empty();

	if (SyncResult)
	{
		for (FDialogueSyncEntry& Entry : SyncResult->Entries)
		{
			// Apply filters
			if (!bShowUnchanged && Entry.Status == EDialogueSyncStatus::Unchanged)
			{
				continue;
			}
			if (bShowConflictsOnly && !Entry.RequiresResolution())
			{
				continue;
			}

			DisplayedEntries.Add(MakeShared<FDialogueSyncEntry>(Entry));
		}
	}

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

FText SDialogueXLSXSyncDialog::GetSummaryText() const
{
	if (!SyncResult)
	{
		return FText::GetEmpty();
	}

	return FText::Format(
		LOCTEXT("SyncSummary", "Total: {0} rows | Unchanged: {1} | Modified in UE: {2} | Modified in Excel: {3} | Conflicts: {4} | Added: {5} | Deleted: {6}"),
		FText::AsNumber(SyncResult->Entries.Num()),
		FText::AsNumber(SyncResult->UnchangedCount),
		FText::AsNumber(SyncResult->ModifiedInUECount),
		FText::AsNumber(SyncResult->ModifiedInExcelCount),
		FText::AsNumber(SyncResult->ConflictCount),
		FText::AsNumber(SyncResult->AddedInUECount + SyncResult->AddedInExcelCount),
		FText::AsNumber(SyncResult->DeletedCount)
	);
}

bool SDialogueXLSXSyncDialog::CanApply() const
{
	return SyncResult && FDialogueXLSXSyncEngine::AllConflictsResolved(*SyncResult);
}

//=============================================================================
// SDialogueSyncEntryRow
//=============================================================================

void SDialogueSyncEntryRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	Entry = InArgs._Entry;
	OnResolutionChanged = InArgs._OnResolutionChanged;

	SMultiColumnTableRow<TSharedPtr<FDialogueSyncEntry>>::Construct(
		FSuperRowType::FArguments()
			.Padding(FMargin(2.0f, 1.0f)),
		InOwnerTable
	);
}

TSharedRef<SWidget> SDialogueSyncEntryRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (!Entry.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	if (ColumnName == TEXT("Status"))
	{
		return CreateStatusCell();
	}
	if (ColumnName == TEXT("DialogueID"))
	{
		return CreateDialogueIDCell();
	}
	if (ColumnName == TEXT("NodeID"))
	{
		return CreateNodeIDCell();
	}
	if (ColumnName == TEXT("Text"))
	{
		return CreateTextPreviewCell();
	}
	if (ColumnName == TEXT("Resolution"))
	{
		return CreateResolutionCell();
	}

	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SDialogueSyncEntryRow::CreateStatusCell()
{
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(STextBlock)
				.Text(FText::FromString(Entry->GetStatusText()))
				.ColorAndOpacity(Entry->GetStatusColor())
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		];
}

TSharedRef<SWidget> SDialogueSyncEntryRow::CreateDialogueIDCell()
{
	// Get DialogueID from whichever version exists
	FName DialogueID = NAME_None;
	if (Entry->UERow.IsValid())
	{
		DialogueID = Entry->UERow->DialogueID;
	}
	else if (Entry->ExcelRow.IsValid())
	{
		DialogueID = Entry->ExcelRow->DialogueID;
	}
	else if (Entry->BaseRow.IsValid())
	{
		DialogueID = Entry->BaseRow->DialogueID;
	}

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(STextBlock)
				.Text(FText::FromName(DialogueID))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SDialogueSyncEntryRow::CreateNodeIDCell()
{
	FName NodeID = NAME_None;
	if (Entry->UERow.IsValid())
	{
		NodeID = Entry->UERow->NodeID;
	}
	else if (Entry->ExcelRow.IsValid())
	{
		NodeID = Entry->ExcelRow->NodeID;
	}
	else if (Entry->BaseRow.IsValid())
	{
		NodeID = Entry->BaseRow->NodeID;
	}

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(STextBlock)
				.Text(FText::FromName(NodeID))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SDialogueSyncEntryRow::CreateTextPreviewCell()
{
	// Show both UE and Excel text for comparison if different
	FString PreviewText;

	if (Entry->Status == EDialogueSyncStatus::Conflict || Entry->Status == EDialogueSyncStatus::DeleteConflict)
	{
		FString UEText = Entry->UERow.IsValid() ? Entry->UERow->Text : TEXT("(deleted)");
		FString ExcelText = Entry->ExcelRow.IsValid() ? Entry->ExcelRow->Text : TEXT("(deleted)");

		// Truncate long text
		if (UEText.Len() > 30) UEText = UEText.Left(30) + TEXT("...");
		if (ExcelText.Len() > 30) ExcelText = ExcelText.Left(30) + TEXT("...");

		PreviewText = FString::Printf(TEXT("UE: %s | Excel: %s"), *UEText, *ExcelText);
	}
	else
	{
		if (Entry->UERow.IsValid())
		{
			PreviewText = Entry->UERow->Text;
		}
		else if (Entry->ExcelRow.IsValid())
		{
			PreviewText = Entry->ExcelRow->Text;
		}

		if (PreviewText.Len() > 60)
		{
			PreviewText = PreviewText.Left(60) + TEXT("...");
		}
	}

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(STextBlock)
				.Text(FText::FromString(PreviewText))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ToolTipText_Lambda([this]()
				{
					if (Entry->UERow.IsValid())
					{
						return FText::FromString(Entry->UERow->Text);
					}
					return FText::GetEmpty();
				})
		];
}

TSharedRef<SWidget> SDialogueSyncEntryRow::CreateResolutionCell()
{
	// v4.11: All entries except Unchanged require explicit user selection
	// Show actual values in buttons, not generic "UE"/"Excel" labels

	// Helper to get truncated text preview
	auto GetTextPreview = [](const TSharedPtr<FDialogueTableRow>& Row, int32 MaxLen) -> FString
	{
		if (!Row.IsValid()) return TEXT("(none)");
		FString Text = Row->Text;
		if (Text.IsEmpty()) Text = Row->OptionText;  // Player nodes use OptionText
		if (Text.IsEmpty()) return TEXT("(empty)");
		if (Text.Len() > MaxLen) return Text.Left(MaxLen - 3) + TEXT("...");
		return Text;
	};

	// For Unchanged (doesn't require resolution), show auto-resolved
	if (!Entry->RequiresResolution())
	{
		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			[
				SNew(STextBlock)
					.Text(LOCTEXT("AutoResolved", "Unchanged"))
					.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
					.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
			];
	}

	// Get actual values for buttons
	FString UELabel = GetTextPreview(Entry->UERow, 20);
	FString ExcelLabel = GetTextPreview(Entry->ExcelRow, 20);
	FString UETooltip = Entry->UERow.IsValid() ? Entry->UERow->Text : TEXT("(deleted)");
	FString ExcelTooltip = Entry->ExcelRow.IsValid() ? Entry->ExcelRow->Text : TEXT("(deleted)");

	// Check if this is a delete scenario (one side missing)
	bool bUEDeleted = !Entry->UERow.IsValid();
	bool bExcelDeleted = !Entry->ExcelRow.IsValid();

	// v4.11: Use buttons showing actual values, starts unselected
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SHorizontalBox)

			// UE value button (if UE row exists)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(1.0f, 0.0f)
			[
				SNew(SButton)
					.Text(FText::FromString(bUEDeleted ? TEXT("(deleted)") : UELabel))
					.ToolTipText(FText::FromString(FString::Printf(TEXT("UE: %s"), *UETooltip)))
					.OnClicked_Lambda([this]()
					{
						Entry->Resolution = EDialogueConflictResolution::KeepUE;
						OnResolutionChanged.ExecuteIfBound();
						return FReply::Handled();
					})
					.ButtonStyle(FAppStyle::Get(), Entry->Resolution == EDialogueConflictResolution::KeepUE ? "FlatButton.Primary" : "FlatButton.Default")
					.IsEnabled(!bUEDeleted)  // Disabled if UE row doesn't exist
			]

			// Excel value button (if Excel row exists)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(1.0f, 0.0f)
			[
				SNew(SButton)
					.Text(FText::FromString(bExcelDeleted ? TEXT("(deleted)") : ExcelLabel))
					.ToolTipText(FText::FromString(FString::Printf(TEXT("Excel: %s"), *ExcelTooltip)))
					.OnClicked_Lambda([this]()
					{
						Entry->Resolution = EDialogueConflictResolution::KeepExcel;
						OnResolutionChanged.ExecuteIfBound();
						return FReply::Handled();
					})
					.ButtonStyle(FAppStyle::Get(), Entry->Resolution == EDialogueConflictResolution::KeepExcel ? "FlatButton.Success" : "FlatButton.Default")
					.IsEnabled(!bExcelDeleted)  // Disabled if Excel row doesn't exist
			]

			// Delete button (for cases where deletion is an option)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(1.0f, 0.0f)
			[
				SNew(SButton)
					.Text(FText::FromString(TEXT("Remove")))
					.ToolTipText(LOCTEXT("RemoveTooltip", "Remove this row from the table"))
					.OnClicked_Lambda([this]()
					{
						Entry->Resolution = EDialogueConflictResolution::Delete;
						OnResolutionChanged.ExecuteIfBound();
						return FReply::Handled();
					})
					.ButtonStyle(FAppStyle::Get(), Entry->Resolution == EDialogueConflictResolution::Delete ? "FlatButton.Danger" : "FlatButton.Default")
					// v4.11: Show Remove for any case where deletion makes sense (one side missing)
					.Visibility((bUEDeleted || bExcelDeleted) ? EVisibility::Visible : EVisibility::Collapsed)
			]
		];
}

void SDialogueSyncEntryRow::OnResolutionSelected(TSharedPtr<FString> Selection, ESelectInfo::Type SelectInfo)
{
	if (!Selection.IsValid() || !Entry.IsValid())
	{
		return;
	}

	if (*Selection == TEXT("Keep UE"))
	{
		Entry->Resolution = EDialogueConflictResolution::KeepUE;
	}
	else if (*Selection == TEXT("Keep Excel"))
	{
		Entry->Resolution = EDialogueConflictResolution::KeepExcel;
	}
	else if (*Selection == TEXT("Delete"))
	{
		Entry->Resolution = EDialogueConflictResolution::Delete;
	}
	else
	{
		Entry->Resolution = EDialogueConflictResolution::Unresolved;
	}

	OnResolutionChanged.ExecuteIfBound();
}

#undef LOCTEXT_NAMESPACE
