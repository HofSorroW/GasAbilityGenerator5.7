// GasAbilityGenerator - Dialogue XLSX Sync Dialog Implementation
// v4.11.1: Full-screen approval window with radio selection and row highlighting
// v4.11: Show actual values, no pre-selection for non-Unchanged
// v4.3: UI for reviewing and resolving sync conflicts

#include "XLSXSupport/SDialogueXLSXSyncDialog.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
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

bool SDialogueXLSXSyncDialog::ShowModal(FDialogueSyncResult& SyncResult, TSharedPtr<SWindow> ParentWindow)
{
	// v4.11.1: Full-screen approval window
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("SyncDialogTitle", "SYNC APPROVAL"))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(1200, 800))  // Larger default size
		.SupportsMinimize(false)
		.SupportsMaximize(true)
		.IsInitiallyMaximized(false);

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
				.OnCheckStateChanged(this, &SDialogueXLSXSyncDialog::OnShowUnchangedChanged)
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
				.OnClicked(this, &SDialogueXLSXSyncDialog::OnCancelClicked)
		]

		// Apply button
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(4, 0)
		[
			SNew(SButton)
				.Text(LOCTEXT("Apply", "Apply"))
				.OnClicked(this, &SDialogueXLSXSyncDialog::OnApplyClicked)
				.IsEnabled_Lambda([this]() { return CanApply(); })
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
		];
}

TSharedRef<SWidget> SDialogueXLSXSyncDialog::BuildStatusBar()
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

TSharedRef<SWidget> SDialogueXLSXSyncDialog::BuildEntryList()
{
	// v4.11.1: Columns - Status, DialogueID, NodeID, Base, UE, Excel, Action
	TSharedRef<SHeaderRow> HeaderRow = SNew(SHeaderRow)
		+ SHeaderRow::Column("Status")
			.DefaultLabel(LOCTEXT("StatusCol", "Status"))
			.FillWidth(0.10f)
		+ SHeaderRow::Column("DialogueID")
			.DefaultLabel(LOCTEXT("DialogueIDCol", "Dialogue"))
			.FillWidth(0.12f)
		+ SHeaderRow::Column("NodeID")
			.DefaultLabel(LOCTEXT("NodeIDCol", "Node ID"))
			.FillWidth(0.10f)
		+ SHeaderRow::Column("Base")
			.DefaultLabel(LOCTEXT("BaseCol", "Last Export"))
			.FillWidth(0.20f)
		+ SHeaderRow::Column("UE")
			.DefaultLabel(LOCTEXT("UECol", "UE"))
			.FillWidth(0.20f)
		+ SHeaderRow::Column("Excel")
			.DefaultLabel(LOCTEXT("ExcelCol", "Excel"))
			.FillWidth(0.20f)
		+ SHeaderRow::Column("Action")
			.DefaultLabel(LOCTEXT("ActionCol", "Action"))
			.FillWidth(0.08f);

	return SAssignNew(ListView, SListView<TSharedPtr<FDialogueSyncEntry>>)
		.ListItemsSource(&DisplayedEntries)
		.OnGenerateRow(this, &SDialogueXLSXSyncDialog::OnGenerateRow)
		.HeaderRow(HeaderRow)
		.SelectionMode(ESelectionMode::None);
}

TSharedRef<SWidget> SDialogueXLSXSyncDialog::BuildFooter()
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

TSharedRef<ITableRow> SDialogueXLSXSyncDialog::OnGenerateRow(TSharedPtr<FDialogueSyncEntry> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SDialogueSyncEntryRow, OwnerTable)
		.Entry(Item)
		.OnResolutionChanged_Lambda([this]()
		{
			// Force refresh to update Apply button state and row colors
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
			// Apply filters - by default hide Unchanged
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

	// v4.11.1: Count changes requiring resolution and how many are resolved
	int32 TotalChanges = 0;
	int32 ResolvedCount = 0;

	for (const FDialogueSyncEntry& Entry : SyncResult->Entries)
	{
		if (Entry.RequiresResolution())
		{
			TotalChanges++;
			if (Entry.Resolution != EDialogueConflictResolution::Unresolved)
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

	// v4.11.1: Row background color based on resolution state
	FLinearColor RowColor = FLinearColor::Transparent;
	if (Entry.IsValid() && Entry->RequiresResolution())
	{
		if (Entry->Resolution == EDialogueConflictResolution::Unresolved)
		{
			RowColor = FLinearColor(0.8f, 0.6f, 0.2f, 0.15f);  // Yellow - unresolved
		}
		else
		{
			RowColor = FLinearColor(0.2f, 0.6f, 0.2f, 0.15f);  // Green - resolved
		}
	}

	SMultiColumnTableRow<TSharedPtr<FDialogueSyncEntry>>::Construct(
		FSuperRowType::FArguments()
			.Padding(FMargin(2.0f, 1.0f))
			.Style(&FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row")),
		InOwnerTable
	);

	// Apply row color via SetColorAndOpacity on content
	SetColorAndOpacity(FLinearColor::White);
}

TSharedRef<SWidget> SDialogueSyncEntryRow::GenerateWidgetForColumn(const FName& ColumnName)
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
	else if (ColumnName == TEXT("DialogueID"))
	{
		CellContent = CreateDialogueIDCell();
	}
	else if (ColumnName == TEXT("NodeID"))
	{
		CellContent = CreateNodeIDCell();
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
		if (Entry->Resolution == EDialogueConflictResolution::Unresolved)
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

TSharedRef<SWidget> SDialogueSyncEntryRow::CreateBaseCell()
{
	// v4.11.1: Show Base (Last Export) value with radio selection
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

	// For unchanged rows, just show the value (no selection needed)
	if (!Entry->RequiresResolution())
	{
		FString Text = Entry->BaseRow->Text;
		if (Text.Len() > 30) Text = Text.Left(27) + TEXT("...");

		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			[
				SNew(STextBlock)
					.Text(FText::FromString(Text))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ToolTipText(FText::FromString(Entry->BaseRow->Text))
			];
	}

	// For entries requiring resolution, Base is informational only (not selectable)
	// The design shows Base value for reference during conflict resolution
	FString Text = Entry->BaseRow->Text;
	if (Text.Len() > 30) Text = Text.Left(27) + TEXT("...");

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(STextBlock)
				.Text(FText::FromString(Text))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))  // Dimmed since not selectable
				.ToolTipText(FText::Format(
					LOCTEXT("BaseTooltip", "Last Export (Base):\n{0}"),
					FText::FromString(Entry->BaseRow->Text)))
		];
}

TSharedRef<SWidget> SDialogueSyncEntryRow::CreateUECell()
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

	// For unchanged rows, just show the value
	if (!Entry->RequiresResolution())
	{
		FString Text = Entry->UERow->Text;
		if (Text.Len() > 30) Text = Text.Left(27) + TEXT("...");

		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			[
				SNew(STextBlock)
					.Text(FText::FromString(Text))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ToolTipText(FText::FromString(Entry->UERow->Text))
			];
	}

	// Radio-style selection
	FString Text = Entry->UERow->Text;
	if (Text.IsEmpty()) Text = Entry->UERow->OptionText;
	if (Text.Len() > 25) Text = Text.Left(22) + TEXT("...");

	bool bSelected = (Entry->Resolution == EDialogueConflictResolution::KeepUE);
	FString RadioIcon = bSelected ? TEXT("●") : TEXT("○");

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(2))
				.OnClicked_Lambda([this]()
				{
					Entry->Resolution = EDialogueConflictResolution::KeepUE;
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
							.Text(FText::FromString(Text))
							.Font(FCoreStyle::GetDefaultFontStyle(bSelected ? "Bold" : "Regular", 9))
							.ToolTipText(FText::Format(
								LOCTEXT("UETooltip", "UE Value:\n{0}"),
								FText::FromString(Entry->UERow->Text)))
					]
				]
		];
}

TSharedRef<SWidget> SDialogueSyncEntryRow::CreateExcelCell()
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

	// For unchanged rows, just show the value
	if (!Entry->RequiresResolution())
	{
		FString Text = Entry->ExcelRow->Text;
		if (Text.Len() > 30) Text = Text.Left(27) + TEXT("...");

		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			[
				SNew(STextBlock)
					.Text(FText::FromString(Text))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ToolTipText(FText::FromString(Entry->ExcelRow->Text))
			];
	}

	// Radio-style selection
	FString Text = Entry->ExcelRow->Text;
	if (Text.IsEmpty()) Text = Entry->ExcelRow->OptionText;
	if (Text.Len() > 25) Text = Text.Left(22) + TEXT("...");

	bool bSelected = (Entry->Resolution == EDialogueConflictResolution::KeepExcel);
	FString RadioIcon = bSelected ? TEXT("●") : TEXT("○");

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(2))
				.OnClicked_Lambda([this]()
				{
					Entry->Resolution = EDialogueConflictResolution::KeepExcel;
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
							.Text(FText::FromString(Text))
							.Font(FCoreStyle::GetDefaultFontStyle(bSelected ? "Bold" : "Regular", 9))
							.ToolTipText(FText::Format(
								LOCTEXT("ExcelTooltip", "Excel Value:\n{0}"),
								FText::FromString(Entry->ExcelRow->Text)))
					]
				]
		];
}

TSharedRef<SWidget> SDialogueSyncEntryRow::CreateActionCell()
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
	bool bSelected = (Entry->Resolution == EDialogueConflictResolution::Delete);
	FString RadioIcon = bSelected ? TEXT("●") : TEXT("○");

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(2))
				.OnClicked_Lambda([this]()
				{
					Entry->Resolution = EDialogueConflictResolution::Delete;
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

TSharedRef<SWidget> SDialogueSyncEntryRow::CreateTextPreviewCell()
{
	// Legacy - kept for compatibility but not used in new layout
	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SDialogueSyncEntryRow::CreateResolutionCell()
{
	// Legacy - kept for compatibility but not used in new layout
	return SNullWidget::NullWidget;
}

void SDialogueSyncEntryRow::OnResolutionSelected(TSharedPtr<FString> Selection, ESelectInfo::Type SelectInfo)
{
	// Legacy - kept for compatibility
}

#undef LOCTEXT_NAMESPACE
