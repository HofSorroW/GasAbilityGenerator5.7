// GasAbilityGenerator - NPC XLSX Sync Dialog Implementation
// v4.11: Show actual values, no pre-selection for non-Unchanged
// v4.5: UI for reviewing and resolving NPC sync conflicts

#include "XLSXSupport/SNPCXLSXSyncDialog.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "NPCXLSXSyncDialog"

//=============================================================================
// SNPCXLSXSyncDialog
//=============================================================================

void SNPCXLSXSyncDialog::Construct(const FArguments& InArgs)
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

bool SNPCXLSXSyncDialog::ShowModal(FNPCSyncResult& SyncResult, TSharedPtr<SWindow> ParentWindow)
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("SyncDialogTitle", "NPC XLSX Sync - Review Changes"))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(950, 600))
		.SupportsMinimize(false)
		.SupportsMaximize(true);

	TSharedRef<SNPCXLSXSyncDialog> Dialog = SNew(SNPCXLSXSyncDialog)
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

	return Dialog->GetResult() == ENPCSyncDialogResult::Apply;
}

TSharedRef<SWidget> SNPCXLSXSyncDialog::BuildHeader()
{
	return SNew(SVerticalBox)

		// Title
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 8)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("SyncHeader", "Review NPC Sync Changes"))
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
					.OnCheckStateChanged(this, &SNPCXLSXSyncDialog::OnShowUnchangedChanged)
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
					.OnCheckStateChanged(this, &SNPCXLSXSyncDialog::OnShowConflictsOnlyChanged)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("ShowConflictsOnly", "Show Conflicts Only"))
					]
			]
		];
}

TSharedRef<SWidget> SNPCXLSXSyncDialog::BuildEntryList()
{
	TSharedRef<SHeaderRow> HeaderRow = SNew(SHeaderRow)
		+ SHeaderRow::Column("Status")
			.DefaultLabel(LOCTEXT("StatusCol", "Status"))
			.FillWidth(0.12f)
		+ SHeaderRow::Column("NPCName")
			.DefaultLabel(LOCTEXT("NPCNameCol", "NPC Name"))
			.FillWidth(0.20f)
		+ SHeaderRow::Column("NPCId")
			.DefaultLabel(LOCTEXT("NPCIdCol", "NPC ID"))
			.FillWidth(0.18f)
		+ SHeaderRow::Column("Preview")
			.DefaultLabel(LOCTEXT("PreviewCol", "Change Preview"))
			.FillWidth(0.30f)
		+ SHeaderRow::Column("Resolution")
			.DefaultLabel(LOCTEXT("ResolutionCol", "Resolution"))
			.FillWidth(0.20f);

	return SAssignNew(ListView, SListView<TSharedPtr<FNPCSyncEntry>>)
		.ListItemsSource(&DisplayedEntries)
		.OnGenerateRow(this, &SNPCXLSXSyncDialog::OnGenerateRow)
		.HeaderRow(HeaderRow)
		.SelectionMode(ESelectionMode::None);
}

TSharedRef<SWidget> SNPCXLSXSyncDialog::BuildFooter()
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
				.OnClicked(this, &SNPCXLSXSyncDialog::OnCancelClicked)
		]

		// Apply button
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4, 0)
		[
			SNew(SButton)
				.Text(LOCTEXT("Apply", "Apply Changes"))
				.OnClicked(this, &SNPCXLSXSyncDialog::OnApplyClicked)
				.IsEnabled_Lambda([this]() { return CanApply(); })
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
		];
}

TSharedRef<ITableRow> SNPCXLSXSyncDialog::OnGenerateRow(TSharedPtr<FNPCSyncEntry> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SNPCSyncEntryRow, OwnerTable)
		.Entry(Item)
		.OnResolutionChanged_Lambda([this]()
		{
			// Force refresh to update Apply button state
			ListView->RequestListRefresh();
		});
}

FReply SNPCXLSXSyncDialog::OnApplyClicked()
{
	DialogResult = ENPCSyncDialogResult::Apply;
	if (DialogWindow.IsValid())
	{
		DialogWindow.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

FReply SNPCXLSXSyncDialog::OnCancelClicked()
{
	DialogResult = ENPCSyncDialogResult::Cancel;
	if (DialogWindow.IsValid())
	{
		DialogWindow.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

void SNPCXLSXSyncDialog::OnShowUnchangedChanged(ECheckBoxState NewState)
{
	bShowUnchanged = (NewState == ECheckBoxState::Checked);
	RefreshEntryList();
}

void SNPCXLSXSyncDialog::OnShowConflictsOnlyChanged(ECheckBoxState NewState)
{
	bShowConflictsOnly = (NewState == ECheckBoxState::Checked);
	RefreshEntryList();
}

void SNPCXLSXSyncDialog::RefreshEntryList()
{
	DisplayedEntries.Empty();

	if (SyncResult)
	{
		for (FNPCSyncEntry& Entry : SyncResult->Entries)
		{
			// Apply filters
			if (!bShowUnchanged && Entry.Status == ENPCSyncStatus::Unchanged)
			{
				continue;
			}
			if (bShowConflictsOnly && !Entry.RequiresResolution())
			{
				continue;
			}

			DisplayedEntries.Add(MakeShared<FNPCSyncEntry>(Entry));
		}
	}

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

FText SNPCXLSXSyncDialog::GetSummaryText() const
{
	if (!SyncResult)
	{
		return FText::GetEmpty();
	}

	return FText::Format(
		LOCTEXT("SyncSummary", "Total: {0} NPCs | Unchanged: {1} | Modified in UE: {2} | Modified in Excel: {3} | Conflicts: {4} | Added: {5} | Deleted: {6}"),
		FText::AsNumber(SyncResult->Entries.Num()),
		FText::AsNumber(SyncResult->UnchangedCount),
		FText::AsNumber(SyncResult->ModifiedInUECount),
		FText::AsNumber(SyncResult->ModifiedInExcelCount),
		FText::AsNumber(SyncResult->ConflictCount),
		FText::AsNumber(SyncResult->AddedInUECount + SyncResult->AddedInExcelCount),
		FText::AsNumber(SyncResult->DeletedCount)
	);
}

bool SNPCXLSXSyncDialog::CanApply() const
{
	return SyncResult && FNPCXLSXSyncEngine::AllConflictsResolved(*SyncResult);
}

//=============================================================================
// SNPCSyncEntryRow
//=============================================================================

void SNPCSyncEntryRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	Entry = InArgs._Entry;
	OnResolutionChanged = InArgs._OnResolutionChanged;

	SMultiColumnTableRow<TSharedPtr<FNPCSyncEntry>>::Construct(
		FSuperRowType::FArguments()
			.Padding(FMargin(2.0f, 1.0f)),
		InOwnerTable
	);
}

TSharedRef<SWidget> SNPCSyncEntryRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (!Entry.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	if (ColumnName == TEXT("Status"))
	{
		return CreateStatusCell();
	}
	if (ColumnName == TEXT("NPCName"))
	{
		return CreateNPCNameCell();
	}
	if (ColumnName == TEXT("NPCId"))
	{
		return CreateNPCIdCell();
	}
	if (ColumnName == TEXT("Preview"))
	{
		return CreatePreviewCell();
	}
	if (ColumnName == TEXT("Resolution"))
	{
		return CreateResolutionCell();
	}

	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SNPCSyncEntryRow::CreateStatusCell()
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

TSharedRef<SWidget> SNPCSyncEntryRow::CreateNPCNameCell()
{
	// Get NPCName from whichever version exists
	FString NPCName;
	if (Entry->UERow.IsValid())
	{
		NPCName = Entry->UERow->NPCName;
	}
	else if (Entry->ExcelRow.IsValid())
	{
		NPCName = Entry->ExcelRow->NPCName;
	}
	else if (Entry->BaseRow.IsValid())
	{
		NPCName = Entry->BaseRow->NPCName;
	}

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(STextBlock)
				.Text(FText::FromString(NPCName))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SNPCSyncEntryRow::CreateNPCIdCell()
{
	FString NPCId;
	if (Entry->UERow.IsValid())
	{
		NPCId = Entry->UERow->NPCId;
	}
	else if (Entry->ExcelRow.IsValid())
	{
		NPCId = Entry->ExcelRow->NPCId;
	}
	else if (Entry->BaseRow.IsValid())
	{
		NPCId = Entry->BaseRow->NPCId;
	}

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(STextBlock)
				.Text(FText::FromString(NPCId))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SNPCSyncEntryRow::CreatePreviewCell()
{
	// Build a preview of what changed
	FString PreviewText;

	if (Entry->Status == ENPCSyncStatus::Conflict || Entry->Status == ENPCSyncStatus::DeleteConflict)
	{
		// Show both versions for conflicts
		TArray<FString> Differences;

		if (Entry->UERow.IsValid() && Entry->ExcelRow.IsValid())
		{
			// Compare key fields
			if (Entry->UERow->DisplayName != Entry->ExcelRow->DisplayName)
			{
				Differences.Add(FString::Printf(TEXT("Display: %s vs %s"),
					*Entry->UERow->DisplayName, *Entry->ExcelRow->DisplayName));
			}
			if (Entry->UERow->MinLevel != Entry->ExcelRow->MinLevel ||
				Entry->UERow->MaxLevel != Entry->ExcelRow->MaxLevel)
			{
				Differences.Add(FString::Printf(TEXT("Level: %d-%d vs %d-%d"),
					Entry->UERow->MinLevel, Entry->UERow->MaxLevel,
					Entry->ExcelRow->MinLevel, Entry->ExcelRow->MaxLevel));
			}
			if (Entry->UERow->bIsVendor != Entry->ExcelRow->bIsVendor)
			{
				Differences.Add(TEXT("Vendor status differs"));
			}
		}
		else if (Entry->UERow.IsValid())
		{
			PreviewText = TEXT("Excel: (deleted)");
		}
		else if (Entry->ExcelRow.IsValid())
		{
			PreviewText = TEXT("UE: (deleted)");
		}

		if (Differences.Num() > 0)
		{
			PreviewText = FString::Join(Differences, TEXT(" | "));
			if (PreviewText.Len() > 50)
			{
				PreviewText = PreviewText.Left(47) + TEXT("...");
			}
		}
	}
	else if (Entry->Status == ENPCSyncStatus::AddedInExcel || Entry->Status == ENPCSyncStatus::AddedInUE)
	{
		PreviewText = TEXT("New NPC");
	}
	else if (Entry->Status == ENPCSyncStatus::DeletedInExcel || Entry->Status == ENPCSyncStatus::DeletedInUE)
	{
		PreviewText = TEXT("Deleted");
	}
	else if (Entry->Status == ENPCSyncStatus::ModifiedInExcel || Entry->Status == ENPCSyncStatus::ModifiedInUE)
	{
		// Show what changed
		TArray<FString> Changes;
		const FNPCTableRow* Source = Entry->Status == ENPCSyncStatus::ModifiedInExcel
			? (Entry->ExcelRow.IsValid() ? Entry->ExcelRow.Get() : nullptr)
			: (Entry->UERow.IsValid() ? Entry->UERow.Get() : nullptr);
		const FNPCTableRow* Base = Entry->BaseRow.IsValid() ? Entry->BaseRow.Get() : nullptr;

		if (Source && Base)
		{
			if (Source->DisplayName != Base->DisplayName) Changes.Add(TEXT("DisplayName"));
			if (Source->MinLevel != Base->MinLevel || Source->MaxLevel != Base->MaxLevel) Changes.Add(TEXT("Level"));
			if (Source->bIsVendor != Base->bIsVendor) Changes.Add(TEXT("Vendor"));
			if (Source->Blueprint != Base->Blueprint) Changes.Add(TEXT("Blueprint"));
			if (Source->AbilityConfig != Base->AbilityConfig) Changes.Add(TEXT("AbilityConfig"));
		}

		if (Changes.Num() > 0)
		{
			PreviewText = FString::Printf(TEXT("Changed: %s"), *FString::Join(Changes, TEXT(", ")));
		}
		else
		{
			PreviewText = TEXT("Modified");
		}
	}
	else
	{
		PreviewText = TEXT("-");
	}

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(STextBlock)
				.Text(FText::FromString(PreviewText))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ToolTipText_Lambda([this]()
				{
					// Show full details in tooltip
					FString Details;
					if (Entry->UERow.IsValid())
					{
						Details += FString::Printf(TEXT("UE: %s (%s) L%d-%d"),
							*Entry->UERow->NPCName, *Entry->UERow->DisplayName,
							Entry->UERow->MinLevel, Entry->UERow->MaxLevel);
					}
					if (Entry->ExcelRow.IsValid())
					{
						if (!Details.IsEmpty()) Details += TEXT("\n");
						Details += FString::Printf(TEXT("Excel: %s (%s) L%d-%d"),
							*Entry->ExcelRow->NPCName, *Entry->ExcelRow->DisplayName,
							Entry->ExcelRow->MinLevel, Entry->ExcelRow->MaxLevel);
					}
					return FText::FromString(Details);
				})
		];
}

TSharedRef<SWidget> SNPCSyncEntryRow::CreateResolutionCell()
{
	// v4.11: All entries except Unchanged require explicit user selection
	// Show actual values in buttons, not generic "UE"/"Excel" labels

	// Helper to get a meaningful preview (NPC Name + AbilityConfig)
	auto GetNPCPreview = [](const TSharedPtr<FNPCTableRow>& Row, int32 MaxLen) -> FString
	{
		if (!Row.IsValid()) return TEXT("(none)");
		FString Preview = Row->NPCName;
		// Add ability config suffix if present
		if (!Row->AbilityConfig.IsNull())
		{
			FString ACName = FPaths::GetBaseFilename(Row->AbilityConfig.GetAssetName());
			if (!ACName.IsEmpty())
			{
				Preview += TEXT(" [") + ACName + TEXT("]");
			}
		}
		if (Preview.Len() > MaxLen) return Preview.Left(MaxLen - 3) + TEXT("...");
		return Preview;
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
	FString UELabel = GetNPCPreview(Entry->UERow, 25);
	FString ExcelLabel = GetNPCPreview(Entry->ExcelRow, 25);

	// Build tooltips with full details
	auto BuildTooltip = [](const TSharedPtr<FNPCTableRow>& Row, const FString& Source) -> FString
	{
		if (!Row.IsValid()) return Source + TEXT(": (deleted)");
		return FString::Printf(TEXT("%s:\nName: %s\nDisplay: %s\nLevel: %d-%d\nAbilityConfig: %s"),
			*Source, *Row->NPCName, *Row->DisplayName, Row->MinLevel, Row->MaxLevel,
			Row->AbilityConfig.IsNull() ? TEXT("(none)") : *Row->AbilityConfig.GetAssetName());
	};

	FString UETooltip = BuildTooltip(Entry->UERow, TEXT("UE"));
	FString ExcelTooltip = BuildTooltip(Entry->ExcelRow, TEXT("Excel"));

	// Check if this is a delete scenario (one side missing)
	bool bUEDeleted = !Entry->UERow.IsValid();
	bool bExcelDeleted = !Entry->ExcelRow.IsValid();

	// v4.11: Use buttons showing actual values, starts unselected
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SHorizontalBox)

			// UE value button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(1.0f, 0.0f)
			[
				SNew(SButton)
					.Text(FText::FromString(bUEDeleted ? TEXT("(deleted)") : UELabel))
					.ToolTipText(FText::FromString(UETooltip))
					.OnClicked_Lambda([this]()
					{
						Entry->Resolution = ENPCConflictResolution::KeepUE;
						OnResolutionChanged.ExecuteIfBound();
						return FReply::Handled();
					})
					.ButtonStyle(FAppStyle::Get(), Entry->Resolution == ENPCConflictResolution::KeepUE ? "FlatButton.Primary" : "FlatButton.Default")
					.IsEnabled(!bUEDeleted)
			]

			// Excel value button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(1.0f, 0.0f)
			[
				SNew(SButton)
					.Text(FText::FromString(bExcelDeleted ? TEXT("(deleted)") : ExcelLabel))
					.ToolTipText(FText::FromString(ExcelTooltip))
					.OnClicked_Lambda([this]()
					{
						Entry->Resolution = ENPCConflictResolution::KeepExcel;
						OnResolutionChanged.ExecuteIfBound();
						return FReply::Handled();
					})
					.ButtonStyle(FAppStyle::Get(), Entry->Resolution == ENPCConflictResolution::KeepExcel ? "FlatButton.Success" : "FlatButton.Default")
					.IsEnabled(!bExcelDeleted)
			]

			// Remove button (for cases where deletion is an option)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(1.0f, 0.0f)
			[
				SNew(SButton)
					.Text(FText::FromString(TEXT("Remove")))
					.ToolTipText(LOCTEXT("RemoveTooltip", "Remove this NPC from the table"))
					.OnClicked_Lambda([this]()
					{
						Entry->Resolution = ENPCConflictResolution::Delete;
						OnResolutionChanged.ExecuteIfBound();
						return FReply::Handled();
					})
					.ButtonStyle(FAppStyle::Get(), Entry->Resolution == ENPCConflictResolution::Delete ? "FlatButton.Danger" : "FlatButton.Default")
					// v4.11: Show Remove for any case where deletion makes sense
					.Visibility((bUEDeleted || bExcelDeleted) ? EVisibility::Visible : EVisibility::Collapsed)
			]
		];
}

#undef LOCTEXT_NAMESPACE
