// GasAbilityGenerator - NPC XLSX Sync Dialog Implementation
// v4.11.1: Full-screen approval window with radio selection and row highlighting
// v4.11: Show actual values, no pre-selection for non-Unchanged
// v4.5: UI for reviewing and resolving NPC sync conflicts

#include "XLSXSupport/SNPCXLSXSyncDialog.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
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

		// Header with title and buttons
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16.0f, 12.0f)
		[
			BuildHeader()
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
				.Thickness(2.0f)
		]

		// Status bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16.0f, 8.0f)
		[
			BuildStatusBar()
		]

		// Entry list
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(8.0f, 4.0f)
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
		.Padding(16.0f, 12.0f)
		[
			BuildFooter()
		]
	];
}

bool SNPCXLSXSyncDialog::ShowModal(FNPCSyncResult& SyncResult, TSharedPtr<SWindow> ParentWindow)
{
	// v4.11.2: Auto-fit to screen size (80% of work area, min 800x600)
	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);

	const FPlatformRect& WorkArea = DisplayMetrics.PrimaryDisplayWorkAreaRect;
	float WorkAreaWidth = WorkArea.Right - WorkArea.Left;
	float WorkAreaHeight = WorkArea.Bottom - WorkArea.Top;

	// Use 80% of screen, with minimum bounds
	float WindowWidth = FMath::Max(800.0f, WorkAreaWidth * 0.8f);
	float WindowHeight = FMath::Max(600.0f, WorkAreaHeight * 0.8f);

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("SyncDialogTitle", "SYNC APPROVAL"))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(WindowWidth, WindowHeight))
		.SupportsMinimize(false)
		.SupportsMaximize(true)
		.IsInitiallyMaximized(false);

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
	return SNew(SHorizontalBox)

		// Title
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("SyncHeader", "SYNC APPROVAL"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
		]

		// Spacer
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNullWidget::NullWidget
		]

		// Filter checkboxes
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
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

		// Cancel button
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(4, 0)
		[
			SNew(SButton)
				.Text(LOCTEXT("Cancel", "Cancel"))
				.OnClicked(this, &SNPCXLSXSyncDialog::OnCancelClicked)
		]

		// Apply button
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(4, 0)
		[
			SNew(SButton)
				.Text(LOCTEXT("Apply", "Apply"))
				.OnClicked(this, &SNPCXLSXSyncDialog::OnApplyClicked)
				.IsEnabled_Lambda([this]() { return CanApply(); })
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
		];
}

TSharedRef<SWidget> SNPCXLSXSyncDialog::BuildStatusBar()
{
	return SNew(SHorizontalBox)

		// Change count and resolution progress
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
				.Text_Lambda([this]() { return GetSummaryText(); })
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
		];
}

TSharedRef<SWidget> SNPCXLSXSyncDialog::BuildEntryList()
{
	// v4.12.2: Columns - Status, Validation, NPCName, NPCId, Base, UE, Excel, Action
	TSharedRef<SHeaderRow> HeaderRow = SNew(SHeaderRow)
		+ SHeaderRow::Column("Status")
			.DefaultLabel(LOCTEXT("StatusCol", "Status"))
			.FillWidth(0.08f)
		+ SHeaderRow::Column("Validation")
			.DefaultLabel(LOCTEXT("ValidationCol", "Valid"))
			.FillWidth(0.06f)
		+ SHeaderRow::Column("NPCName")
			.DefaultLabel(LOCTEXT("NPCNameCol", "NPC Name"))
			.FillWidth(0.12f)
		+ SHeaderRow::Column("NPCId")
			.DefaultLabel(LOCTEXT("NPCIdCol", "NPC ID"))
			.FillWidth(0.10f)
		+ SHeaderRow::Column("Base")
			.DefaultLabel(LOCTEXT("BaseCol", "Last Export"))
			.FillWidth(0.16f)
		+ SHeaderRow::Column("UE")
			.DefaultLabel(LOCTEXT("UECol", "UE"))
			.FillWidth(0.16f)
		+ SHeaderRow::Column("Excel")
			.DefaultLabel(LOCTEXT("ExcelCol", "Excel"))
			.FillWidth(0.16f)
		+ SHeaderRow::Column("Action")
			.DefaultLabel(LOCTEXT("ActionCol", "Action"))
			.FillWidth(0.10f);

	return SAssignNew(ListView, SListView<TSharedPtr<FNPCSyncEntry>>)
		.ListItemsSource(&DisplayedEntries)
		.OnGenerateRow(this, &SNPCXLSXSyncDialog::OnGenerateRow)
		.HeaderRow(HeaderRow)
		.SelectionMode(ESelectionMode::None);
}

TSharedRef<SWidget> SNPCXLSXSyncDialog::BuildFooter()
{
	return SNew(SHorizontalBox)

		// Legend
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)

			// v4.12.2: Error legend (red)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 8, 0)
			[
				SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(0.8f, 0.2f, 0.2f, 0.3f))  // Red
					.Padding(FMargin(8, 2))
					[
						SNew(STextBlock)
							.Text(LOCTEXT("LegendError", "Error"))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 8, 0)
			[
				SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(0.8f, 0.6f, 0.2f, 0.3f))  // Yellow
					.Padding(FMargin(8, 2))
					[
						SNew(STextBlock)
							.Text(LOCTEXT("LegendUnresolved", "Unresolved"))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(0.2f, 0.6f, 0.2f, 0.3f))  // Green
					.Padding(FMargin(8, 2))
					[
						SNew(STextBlock)
							.Text(LOCTEXT("LegendResolved", "Resolved"))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					]
			]
		]

		// Spacer
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNullWidget::NullWidget
		]

		// Apply instruction
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
				.Text_Lambda([this]()
				{
					if (CanApply())
					{
						return LOCTEXT("ReadyToApply", "All changes resolved. Click Apply to confirm.");
					}
					return LOCTEXT("ResolveFirst", "Resolve all changes before applying.");
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Italic", 10))
				.ColorAndOpacity_Lambda([this]()
				{
					return CanApply() ? FLinearColor(0.2f, 0.8f, 0.2f) : FLinearColor(0.8f, 0.6f, 0.2f);
				})
		];
}

TSharedRef<ITableRow> SNPCXLSXSyncDialog::OnGenerateRow(TSharedPtr<FNPCSyncEntry> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SNPCSyncEntryRow, OwnerTable)
		.Entry(Item)
		.OnResolutionChanged_Lambda([this]()
		{
			// Force refresh to update Apply button state and row colors
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
			// Apply filters - by default hide Unchanged
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

	// v4.11.1: Count changes requiring resolution and how many are resolved
	int32 TotalChanges = 0;
	int32 ResolvedCount = 0;

	for (const FNPCSyncEntry& Entry : SyncResult->Entries)
	{
		if (Entry.RequiresResolution())
		{
			TotalChanges++;
			if (Entry.Resolution != ENPCConflictResolution::Unresolved)
			{
				ResolvedCount++;
			}
		}
	}

	// v4.12.2: Include validation counts if there are errors/warnings
	FString SummaryStr = FString::Printf(TEXT("%d changes | %d of %d resolved"), TotalChanges, ResolvedCount, TotalChanges);

	if (SyncResult->ValidationErrorCount > 0)
	{
		SummaryStr += FString::Printf(TEXT(" | %d errors"), SyncResult->ValidationErrorCount);
	}
	if (SyncResult->ValidationWarningCount > 0)
	{
		SummaryStr += FString::Printf(TEXT(" | %d warnings"), SyncResult->ValidationWarningCount);
	}

	return FText::FromString(SummaryStr);
}

bool SNPCXLSXSyncDialog::CanApply() const
{
	// v4.12.2: Also check for validation errors
	return SyncResult && FNPCXLSXSyncEngine::AllConflictsResolved(*SyncResult) && !SyncResult->HasValidationErrors();
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
			.Padding(FMargin(2.0f, 1.0f))
			.Style(&FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row")),
		InOwnerTable
	);
}

TSharedRef<SWidget> SNPCSyncEntryRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (!Entry.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	// v4.11.1: Wrap all cells in colored border for row highlighting
	TSharedRef<SWidget> CellContent = SNullWidget::NullWidget;

	if (ColumnName == TEXT("Status"))
	{
		CellContent = CreateStatusCell();
	}
	else if (ColumnName == TEXT("Validation"))
	{
		CellContent = CreateValidationCell();
	}
	else if (ColumnName == TEXT("NPCName"))
	{
		CellContent = CreateNPCNameCell();
	}
	else if (ColumnName == TEXT("NPCId"))
	{
		CellContent = CreateNPCIdCell();
	}
	else if (ColumnName == TEXT("Base"))
	{
		CellContent = CreateBaseCell();
	}
	else if (ColumnName == TEXT("UE"))
	{
		CellContent = CreateUECell();
	}
	else if (ColumnName == TEXT("Excel"))
	{
		CellContent = CreateExcelCell();
	}
	else if (ColumnName == TEXT("Action"))
	{
		CellContent = CreateActionCell();
	}

	// v4.12.2: Apply row background color - red for errors, yellow for unresolved, green for resolved
	FLinearColor BgColor = FLinearColor::Transparent;
	if (Entry->ValidationStatus == ENPCSyncValidationStatus::Error)
	{
		BgColor = FLinearColor(0.8f, 0.2f, 0.2f, 0.2f);  // Red for errors
	}
	else if (Entry->RequiresResolution())
	{
		if (Entry->Resolution == ENPCConflictResolution::Unresolved)
		{
			BgColor = FLinearColor(0.8f, 0.6f, 0.2f, 0.15f);  // Yellow
		}
		else
		{
			BgColor = FLinearColor(0.2f, 0.6f, 0.2f, 0.15f);  // Green
		}
	}

	return SNew(SBorder)
		.BorderBackgroundColor(BgColor)
		.Padding(0)
		[
			CellContent
		];
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

TSharedRef<SWidget> SNPCSyncEntryRow::CreateBaseCell()
{
	// v4.11.1: Show Base (Last Export) value - informational only
	if (!Entry->BaseRow.IsValid())
	{
		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			[
				SNew(STextBlock)
					.Text(LOCTEXT("NoBase", "—"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
			];
	}

	// Build preview: NPCName [AbilityConfig]
	FString Preview = Entry->BaseRow->NPCName;
	if (!Entry->BaseRow->AbilityConfig.IsNull())
	{
		FString ACName = FPaths::GetBaseFilename(Entry->BaseRow->AbilityConfig.GetAssetName());
		if (!ACName.IsEmpty())
		{
			Preview += TEXT(" [") + ACName + TEXT("]");
		}
	}
	if (Preview.Len() > 25) Preview = Preview.Left(22) + TEXT("...");

	// For unchanged, show the value without dimming
	FLinearColor TextColor = Entry->RequiresResolution() ? FLinearColor(0.6f, 0.6f, 0.6f) : FLinearColor::White;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(STextBlock)
				.Text(FText::FromString(Preview))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ColorAndOpacity(TextColor)
				.ToolTipText(FText::Format(
					LOCTEXT("BaseTooltip", "Last Export (Base):\nName: {0}\nDisplay: {1}\nLevel: {2}-{3}"),
					FText::FromString(Entry->BaseRow->NPCName),
					FText::FromString(Entry->BaseRow->DisplayName),
					FText::AsNumber(Entry->BaseRow->MinLevel),
					FText::AsNumber(Entry->BaseRow->MaxLevel)))
		];
}

TSharedRef<SWidget> SNPCSyncEntryRow::CreateUECell()
{
	// v4.11.1: UE value with radio-style selection (○/●)
	if (!Entry->UERow.IsValid())
	{
		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			[
				SNew(STextBlock)
					.Text(LOCTEXT("UEDeleted", "—"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
			];
	}

	// Build preview: NPCName [AbilityConfig]
	FString Preview = Entry->UERow->NPCName;
	if (!Entry->UERow->AbilityConfig.IsNull())
	{
		FString ACName = FPaths::GetBaseFilename(Entry->UERow->AbilityConfig.GetAssetName());
		if (!ACName.IsEmpty())
		{
			Preview += TEXT(" [") + ACName + TEXT("]");
		}
	}
	if (Preview.Len() > 22) Preview = Preview.Left(19) + TEXT("...");

	// For unchanged rows, just show the value
	if (!Entry->RequiresResolution())
	{
		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			[
				SNew(STextBlock)
					.Text(FText::FromString(Preview))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ToolTipText(FText::Format(
						LOCTEXT("UETooltipSimple", "UE:\nName: {0}\nDisplay: {1}\nLevel: {2}-{3}"),
						FText::FromString(Entry->UERow->NPCName),
						FText::FromString(Entry->UERow->DisplayName),
						FText::AsNumber(Entry->UERow->MinLevel),
						FText::AsNumber(Entry->UERow->MaxLevel)))
			];
	}

	// Radio-style selection
	bool bSelected = (Entry->Resolution == ENPCConflictResolution::KeepUE);
	FString RadioIcon = bSelected ? TEXT("●") : TEXT("○");

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(2))
				.OnClicked_Lambda([this]()
				{
					Entry->Resolution = ENPCConflictResolution::KeepUE;
					OnResolutionChanged.ExecuteIfBound();
					return FReply::Handled();
				})
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0, 0, 4, 0)
					[
						SNew(STextBlock)
							.Text(FText::FromString(RadioIcon))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
							.ColorAndOpacity(bSelected ? FLinearColor(0.2f, 0.6f, 1.0f) : FLinearColor(0.5f, 0.5f, 0.5f))
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(FText::FromString(Preview))
							.Font(FCoreStyle::GetDefaultFontStyle(bSelected ? "Bold" : "Regular", 9))
							.ToolTipText(FText::Format(
								LOCTEXT("UETooltip", "UE:\nName: {0}\nDisplay: {1}\nLevel: {2}-{3}"),
								FText::FromString(Entry->UERow->NPCName),
								FText::FromString(Entry->UERow->DisplayName),
								FText::AsNumber(Entry->UERow->MinLevel),
								FText::AsNumber(Entry->UERow->MaxLevel)))
					]
				]
		];
}

TSharedRef<SWidget> SNPCSyncEntryRow::CreateExcelCell()
{
	// v4.11.1: Excel value with radio-style selection (○/●)
	if (!Entry->ExcelRow.IsValid())
	{
		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			[
				SNew(STextBlock)
					.Text(LOCTEXT("ExcelDeleted", "—"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
			];
	}

	// Build preview: NPCName [AbilityConfig]
	FString Preview = Entry->ExcelRow->NPCName;
	if (!Entry->ExcelRow->AbilityConfig.IsNull())
	{
		FString ACName = FPaths::GetBaseFilename(Entry->ExcelRow->AbilityConfig.GetAssetName());
		if (!ACName.IsEmpty())
		{
			Preview += TEXT(" [") + ACName + TEXT("]");
		}
	}
	if (Preview.Len() > 22) Preview = Preview.Left(19) + TEXT("...");

	// For unchanged rows, just show the value
	if (!Entry->RequiresResolution())
	{
		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			[
				SNew(STextBlock)
					.Text(FText::FromString(Preview))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ToolTipText(FText::Format(
						LOCTEXT("ExcelTooltipSimple", "Excel:\nName: {0}\nDisplay: {1}\nLevel: {2}-{3}"),
						FText::FromString(Entry->ExcelRow->NPCName),
						FText::FromString(Entry->ExcelRow->DisplayName),
						FText::AsNumber(Entry->ExcelRow->MinLevel),
						FText::AsNumber(Entry->ExcelRow->MaxLevel)))
			];
	}

	// Radio-style selection
	bool bSelected = (Entry->Resolution == ENPCConflictResolution::KeepExcel);
	FString RadioIcon = bSelected ? TEXT("●") : TEXT("○");

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(2))
				.OnClicked_Lambda([this]()
				{
					Entry->Resolution = ENPCConflictResolution::KeepExcel;
					OnResolutionChanged.ExecuteIfBound();
					return FReply::Handled();
				})
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0, 0, 4, 0)
					[
						SNew(STextBlock)
							.Text(FText::FromString(RadioIcon))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
							.ColorAndOpacity(bSelected ? FLinearColor(0.2f, 0.8f, 0.2f) : FLinearColor(0.5f, 0.5f, 0.5f))
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(FText::FromString(Preview))
							.Font(FCoreStyle::GetDefaultFontStyle(bSelected ? "Bold" : "Regular", 9))
							.ToolTipText(FText::Format(
								LOCTEXT("ExcelTooltip", "Excel:\nName: {0}\nDisplay: {1}\nLevel: {2}-{3}"),
								FText::FromString(Entry->ExcelRow->NPCName),
								FText::FromString(Entry->ExcelRow->DisplayName),
								FText::AsNumber(Entry->ExcelRow->MinLevel),
								FText::AsNumber(Entry->ExcelRow->MaxLevel)))
					]
				]
		];
}

TSharedRef<SWidget> SNPCSyncEntryRow::CreateActionCell()
{
	// v4.11.1: Action column - shows REMOVE option only when deletion makes sense
	bool bUEDeleted = !Entry->UERow.IsValid();
	bool bExcelDeleted = !Entry->ExcelRow.IsValid();

	// Only show REMOVE for cases where one side is missing (cases 6-14 in design)
	if (!(bUEDeleted || bExcelDeleted))
	{
		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			[
				SNew(STextBlock)
					.Text(LOCTEXT("NoAction", "—"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
			];
	}

	// For unchanged, no action needed
	if (!Entry->RequiresResolution())
	{
		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			[
				SNew(STextBlock)
					.Text(LOCTEXT("NoAction", "—"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
			];
	}

	// Radio-style REMOVE option
	bool bSelected = (Entry->Resolution == ENPCConflictResolution::Delete);
	FString RadioIcon = bSelected ? TEXT("●") : TEXT("○");

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(2))
				.OnClicked_Lambda([this]()
				{
					Entry->Resolution = ENPCConflictResolution::Delete;
					OnResolutionChanged.ExecuteIfBound();
					return FReply::Handled();
				})
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0, 0, 4, 0)
					[
						SNew(STextBlock)
							.Text(FText::FromString(RadioIcon))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
							.ColorAndOpacity(bSelected ? FLinearColor(0.9f, 0.2f, 0.2f) : FLinearColor(0.5f, 0.5f, 0.5f))
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("Remove", "REMOVE"))
							.Font(FCoreStyle::GetDefaultFontStyle(bSelected ? "Bold" : "Regular", 9))
							.ColorAndOpacity(bSelected ? FLinearColor(0.9f, 0.2f, 0.2f) : FLinearColor::White)
					]
				]
		];
}

TSharedRef<SWidget> SNPCSyncEntryRow::CreateValidationCell()
{
	// v4.12.2: Show validation status with icon and tooltip
	FString Icon;
	FLinearColor IconColor;
	FText Tooltip;

	switch (Entry->ValidationStatus)
	{
	case ENPCSyncValidationStatus::Error:
		Icon = TEXT("✗");
		IconColor = FLinearColor(0.9f, 0.2f, 0.2f);
		Tooltip = FText::FromString(FString::Join(Entry->ValidationMessages, TEXT("\n")));
		break;
	case ENPCSyncValidationStatus::Warning:
		Icon = TEXT("⚠");
		IconColor = FLinearColor(0.9f, 0.7f, 0.2f);
		Tooltip = FText::FromString(FString::Join(Entry->ValidationMessages, TEXT("\n")));
		break;
	case ENPCSyncValidationStatus::Valid:
	default:
		Icon = TEXT("✓");
		IconColor = FLinearColor(0.2f, 0.8f, 0.2f);
		Tooltip = LOCTEXT("ValidationValid", "No validation issues");
		break;
	}

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
				.Text(FText::FromString(Icon))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
				.ColorAndOpacity(IconColor)
				.ToolTipText(Tooltip)
		];
}

TSharedRef<SWidget> SNPCSyncEntryRow::CreatePreviewCell()
{
	// Legacy - kept for compatibility but not used in new layout
	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SNPCSyncEntryRow::CreateResolutionCell()
{
	// Legacy - kept for compatibility but not used in new layout
	return SNullWidget::NullWidget;
}

#undef LOCTEXT_NAMESPACE
