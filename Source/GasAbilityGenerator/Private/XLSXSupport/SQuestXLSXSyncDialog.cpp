// GasAbilityGenerator - Quest XLSX Sync Dialog Implementation
// v4.12: Full-screen approval window with radio selection and row highlighting
// v4.11: Show actual values, no pre-selection for non-Unchanged

#include "XLSXSupport/SQuestXLSXSyncDialog.h"
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

#define LOCTEXT_NAMESPACE "QuestXLSXSyncDialog"

//=============================================================================
// SQuestXLSXSyncDialog
//=============================================================================

void SQuestXLSXSyncDialog::Construct(const FArguments& InArgs)
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

bool SQuestXLSXSyncDialog::ShowModal(FQuestSyncResult& SyncResult, TSharedPtr<SWindow> ParentWindow)
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
		.Title(LOCTEXT("SyncDialogTitle", "QUEST SYNC APPROVAL"))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(WindowWidth, WindowHeight))
		.SupportsMinimize(false)
		.SupportsMaximize(true)
		.IsInitiallyMaximized(false);

	TSharedRef<SQuestXLSXSyncDialog> Dialog = SNew(SQuestXLSXSyncDialog)
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

	return Dialog->GetResult() == EQuestSyncDialogResult::Apply;
}

TSharedRef<SWidget> SQuestXLSXSyncDialog::BuildHeader()
{
	return SNew(SHorizontalBox)

		// Title
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("SyncHeader", "QUEST SYNC APPROVAL"))
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
				.OnCheckStateChanged(this, &SQuestXLSXSyncDialog::OnShowUnchangedChanged)
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
				.OnClicked(this, &SQuestXLSXSyncDialog::OnCancelClicked)
		]

		// Apply button
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(4, 0)
		[
			SNew(SButton)
				.Text(LOCTEXT("Apply", "Apply"))
				.OnClicked(this, &SQuestXLSXSyncDialog::OnApplyClicked)
				.IsEnabled_Lambda([this]() { return CanApply(); })
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
		];
}

TSharedRef<SWidget> SQuestXLSXSyncDialog::BuildStatusBar()
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

TSharedRef<SWidget> SQuestXLSXSyncDialog::BuildEntryList()
{
	// Columns: Status, QuestName, StateID, Base, UE, Excel, Action
	TSharedRef<SHeaderRow> HeaderRow = SNew(SHeaderRow)
		+ SHeaderRow::Column("Status")
			.DefaultLabel(LOCTEXT("StatusCol", "Status"))
			.FillWidth(0.10f)
		+ SHeaderRow::Column("QuestName")
			.DefaultLabel(LOCTEXT("QuestNameCol", "Quest Name"))
			.FillWidth(0.14f)
		+ SHeaderRow::Column("StateID")
			.DefaultLabel(LOCTEXT("StateIDCol", "State ID"))
			.FillWidth(0.10f)
		+ SHeaderRow::Column("Base")
			.DefaultLabel(LOCTEXT("BaseCol", "Last Export"))
			.FillWidth(0.18f)
		+ SHeaderRow::Column("UE")
			.DefaultLabel(LOCTEXT("UECol", "UE"))
			.FillWidth(0.18f)
		+ SHeaderRow::Column("Excel")
			.DefaultLabel(LOCTEXT("ExcelCol", "Excel"))
			.FillWidth(0.18f)
		+ SHeaderRow::Column("Action")
			.DefaultLabel(LOCTEXT("ActionCol", "Action"))
			.FillWidth(0.12f);

	return SAssignNew(ListView, SListView<TSharedPtr<FQuestSyncEntry>>)
		.ListItemsSource(&DisplayedEntries)
		.OnGenerateRow(this, &SQuestXLSXSyncDialog::OnGenerateRow)
		.HeaderRow(HeaderRow)
		.SelectionMode(ESelectionMode::None);
}

TSharedRef<SWidget> SQuestXLSXSyncDialog::BuildFooter()
{
	return SNew(SHorizontalBox)

		// Legend
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)

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

TSharedRef<ITableRow> SQuestXLSXSyncDialog::OnGenerateRow(TSharedPtr<FQuestSyncEntry> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SQuestSyncEntryRow, OwnerTable)
		.Entry(Item)
		.OnResolutionChanged_Lambda([this]()
		{
			// Force refresh to update Apply button state and row colors
			ListView->RequestListRefresh();
		});
}

FReply SQuestXLSXSyncDialog::OnApplyClicked()
{
	DialogResult = EQuestSyncDialogResult::Apply;
	if (DialogWindow.IsValid())
	{
		DialogWindow.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

FReply SQuestXLSXSyncDialog::OnCancelClicked()
{
	DialogResult = EQuestSyncDialogResult::Cancel;
	if (DialogWindow.IsValid())
	{
		DialogWindow.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

void SQuestXLSXSyncDialog::OnShowUnchangedChanged(ECheckBoxState NewState)
{
	bShowUnchanged = (NewState == ECheckBoxState::Checked);
	RefreshEntryList();
}

void SQuestXLSXSyncDialog::OnShowConflictsOnlyChanged(ECheckBoxState NewState)
{
	bShowConflictsOnly = (NewState == ECheckBoxState::Checked);
	RefreshEntryList();
}

void SQuestXLSXSyncDialog::RefreshEntryList()
{
	DisplayedEntries.Empty();

	if (SyncResult)
	{
		for (FQuestSyncEntry& Entry : SyncResult->Entries)
		{
			// Apply filters - by default hide Unchanged
			if (!bShowUnchanged && Entry.Status == EQuestSyncStatus::Unchanged)
			{
				continue;
			}
			if (bShowConflictsOnly && !Entry.RequiresResolution())
			{
				continue;
			}

			DisplayedEntries.Add(MakeShared<FQuestSyncEntry>(Entry));
		}
	}

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

FText SQuestXLSXSyncDialog::GetSummaryText() const
{
	if (!SyncResult)
	{
		return FText::GetEmpty();
	}

	// v4.11.1: Count changes requiring resolution and how many are resolved
	int32 TotalChanges = 0;
	int32 ResolvedCount = 0;

	for (const FQuestSyncEntry& Entry : SyncResult->Entries)
	{
		if (Entry.RequiresResolution())
		{
			TotalChanges++;
			if (Entry.Resolution != EQuestConflictResolution::Unresolved)
			{
				ResolvedCount++;
			}
		}
	}

	// Format: "12 changes | 5 of 12 resolved"
	return FText::Format(
		LOCTEXT("SyncSummaryNew", "{0} changes | {1} of {0} resolved"),
		FText::AsNumber(TotalChanges),
		FText::AsNumber(ResolvedCount)
	);
}

bool SQuestXLSXSyncDialog::CanApply() const
{
	return SyncResult && FQuestXLSXSyncEngine::AllConflictsResolved(*SyncResult);
}

//=============================================================================
// SQuestSyncEntryRow
//=============================================================================

void SQuestSyncEntryRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	Entry = InArgs._Entry;
	OnResolutionChanged = InArgs._OnResolutionChanged;

	SMultiColumnTableRow<TSharedPtr<FQuestSyncEntry>>::Construct(
		FSuperRowType::FArguments()
			.Padding(FMargin(2.0f, 1.0f))
			.Style(&FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row")),
		InOwnerTable
	);
}

TSharedRef<SWidget> SQuestSyncEntryRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (!Entry.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	// Wrap all cells in colored border for row highlighting
	TSharedRef<SWidget> CellContent = SNullWidget::NullWidget;

	if (ColumnName == TEXT("Status"))
	{
		CellContent = CreateStatusCell();
	}
	else if (ColumnName == TEXT("QuestName"))
	{
		CellContent = CreateQuestNameCell();
	}
	else if (ColumnName == TEXT("StateID"))
	{
		CellContent = CreateStateIDCell();
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

	// Apply row background color
	FLinearColor BgColor = FLinearColor::Transparent;
	if (Entry->RequiresResolution())
	{
		if (Entry->Resolution == EQuestConflictResolution::Unresolved)
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

TSharedRef<SWidget> SQuestSyncEntryRow::CreateStatusCell()
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

TSharedRef<SWidget> SQuestSyncEntryRow::CreateQuestNameCell()
{
	// Get QuestName from whichever version exists
	FString QuestName;
	if (Entry->UERow.IsValid())
	{
		QuestName = Entry->UERow->QuestName;
	}
	else if (Entry->ExcelRow.IsValid())
	{
		QuestName = Entry->ExcelRow->QuestName;
	}
	else if (Entry->BaseRow.IsValid())
	{
		QuestName = Entry->BaseRow->QuestName;
	}

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(STextBlock)
				.Text(FText::FromString(QuestName))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SQuestSyncEntryRow::CreateStateIDCell()
{
	FString StateID;
	if (Entry->UERow.IsValid())
	{
		StateID = Entry->UERow->StateID;
	}
	else if (Entry->ExcelRow.IsValid())
	{
		StateID = Entry->ExcelRow->StateID;
	}
	else if (Entry->BaseRow.IsValid())
	{
		StateID = Entry->BaseRow->StateID;
	}

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(STextBlock)
				.Text(FText::FromString(StateID))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SQuestSyncEntryRow::CreateBaseCell()
{
	// Show Base (Last Export) value - informational only
	if (!Entry->BaseRow.IsValid())
	{
		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			[
				SNew(STextBlock)
					.Text(LOCTEXT("NoBase", "-"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
			];
	}

	// Build preview: Description (truncated)
	FString Preview = Entry->BaseRow->Description;
	if (Preview.IsEmpty()) Preview = Entry->BaseRow->StateID;
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
					LOCTEXT("BaseTooltip", "Last Export (Base):\nQuest: {0}\nState: {1}\nDescription: {2}"),
					FText::FromString(Entry->BaseRow->QuestName),
					FText::FromString(Entry->BaseRow->StateID),
					FText::FromString(Entry->BaseRow->Description)))
		];
}

TSharedRef<SWidget> SQuestSyncEntryRow::CreateUECell()
{
	// UE value with radio-style selection
	if (!Entry->UERow.IsValid())
	{
		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			[
				SNew(STextBlock)
					.Text(LOCTEXT("UEDeleted", "-"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
			];
	}

	// Build preview
	FString Preview = Entry->UERow->Description;
	if (Preview.IsEmpty()) Preview = Entry->UERow->StateID;
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
						LOCTEXT("UETooltipSimple", "UE:\nQuest: {0}\nState: {1}\nDescription: {2}"),
						FText::FromString(Entry->UERow->QuestName),
						FText::FromString(Entry->UERow->StateID),
						FText::FromString(Entry->UERow->Description)))
			];
	}

	// Radio-style selection
	bool bSelected = (Entry->Resolution == EQuestConflictResolution::KeepUE);
	FString RadioIcon = bSelected ? TEXT("*") : TEXT("O");

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(2))
				.OnClicked_Lambda([this]()
				{
					Entry->Resolution = EQuestConflictResolution::KeepUE;
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
								LOCTEXT("UETooltip", "UE:\nQuest: {0}\nState: {1}\nDescription: {2}"),
								FText::FromString(Entry->UERow->QuestName),
								FText::FromString(Entry->UERow->StateID),
								FText::FromString(Entry->UERow->Description)))
					]
				]
		];
}

TSharedRef<SWidget> SQuestSyncEntryRow::CreateExcelCell()
{
	// Excel value with radio-style selection
	if (!Entry->ExcelRow.IsValid())
	{
		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			[
				SNew(STextBlock)
					.Text(LOCTEXT("ExcelDeleted", "-"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
			];
	}

	// Build preview
	FString Preview = Entry->ExcelRow->Description;
	if (Preview.IsEmpty()) Preview = Entry->ExcelRow->StateID;
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
						LOCTEXT("ExcelTooltipSimple", "Excel:\nQuest: {0}\nState: {1}\nDescription: {2}"),
						FText::FromString(Entry->ExcelRow->QuestName),
						FText::FromString(Entry->ExcelRow->StateID),
						FText::FromString(Entry->ExcelRow->Description)))
			];
	}

	// Radio-style selection
	bool bSelected = (Entry->Resolution == EQuestConflictResolution::KeepExcel);
	FString RadioIcon = bSelected ? TEXT("*") : TEXT("O");

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(2))
				.OnClicked_Lambda([this]()
				{
					Entry->Resolution = EQuestConflictResolution::KeepExcel;
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
								LOCTEXT("ExcelTooltip", "Excel:\nQuest: {0}\nState: {1}\nDescription: {2}"),
								FText::FromString(Entry->ExcelRow->QuestName),
								FText::FromString(Entry->ExcelRow->StateID),
								FText::FromString(Entry->ExcelRow->Description)))
					]
				]
		];
}

TSharedRef<SWidget> SQuestSyncEntryRow::CreateActionCell()
{
	// Action column - shows REMOVE option only when deletion makes sense
	bool bUEDeleted = !Entry->UERow.IsValid();
	bool bExcelDeleted = !Entry->ExcelRow.IsValid();

	// Only show REMOVE for cases where one side is missing (cases 6-14 in design)
	if (!(bUEDeleted || bExcelDeleted))
	{
		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			[
				SNew(STextBlock)
					.Text(LOCTEXT("NoAction", "-"))
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
					.Text(LOCTEXT("NoAction", "-"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
			];
	}

	// Radio-style REMOVE option
	bool bSelected = (Entry->Resolution == EQuestConflictResolution::Delete);
	FString RadioIcon = bSelected ? TEXT("*") : TEXT("O");

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(2))
				.OnClicked_Lambda([this]()
				{
					Entry->Resolution = EQuestConflictResolution::Delete;
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

#undef LOCTEXT_NAMESPACE
