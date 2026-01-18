// GasAbilityGenerator - Item XLSX Sync Dialog Implementation
// v4.12: Full-screen approval window with radio selection and row highlighting
// v4.11: Show actual values, no pre-selection for non-Unchanged

#include "XLSXSupport/SItemXLSXSyncDialog.h"
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

#define LOCTEXT_NAMESPACE "ItemXLSXSyncDialog"

//=============================================================================
// SItemXLSXSyncDialog
//=============================================================================

void SItemXLSXSyncDialog::Construct(const FArguments& InArgs)
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

bool SItemXLSXSyncDialog::ShowModal(FItemSyncResult& SyncResult, TSharedPtr<SWindow> ParentWindow)
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
		.Title(LOCTEXT("SyncDialogTitle", "ITEM SYNC APPROVAL"))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(WindowWidth, WindowHeight))
		.SupportsMinimize(false)
		.SupportsMaximize(true)
		.IsInitiallyMaximized(false);

	TSharedRef<SItemXLSXSyncDialog> Dialog = SNew(SItemXLSXSyncDialog)
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

	return Dialog->GetResult() == EItemSyncDialogResult::Apply;
}

TSharedRef<SWidget> SItemXLSXSyncDialog::BuildHeader()
{
	return SNew(SHorizontalBox)

		// Title
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("SyncHeader", "ITEM SYNC APPROVAL"))
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
				.OnCheckStateChanged(this, &SItemXLSXSyncDialog::OnShowUnchangedChanged)
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
				.OnClicked(this, &SItemXLSXSyncDialog::OnCancelClicked)
		]

		// Apply button
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(4, 0)
		[
			SNew(SButton)
				.Text(LOCTEXT("Apply", "Apply"))
				.OnClicked(this, &SItemXLSXSyncDialog::OnApplyClicked)
				.IsEnabled_Lambda([this]() { return CanApply(); })
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
		];
}

TSharedRef<SWidget> SItemXLSXSyncDialog::BuildStatusBar()
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

TSharedRef<SWidget> SItemXLSXSyncDialog::BuildEntryList()
{
	// v4.12.2: Columns - Status, Validation, ItemName, ItemType, Base, UE, Excel, Action
	TSharedRef<SHeaderRow> HeaderRow = SNew(SHeaderRow)
		+ SHeaderRow::Column("Status")
			.DefaultLabel(LOCTEXT("StatusCol", "Status"))
			.FillWidth(0.08f)
		+ SHeaderRow::Column("Validation")
			.DefaultLabel(LOCTEXT("ValidationCol", "Valid"))
			.FillWidth(0.06f)
		+ SHeaderRow::Column("ItemName")
			.DefaultLabel(LOCTEXT("ItemNameCol", "Item Name"))
			.FillWidth(0.12f)
		+ SHeaderRow::Column("ItemType")
			.DefaultLabel(LOCTEXT("ItemTypeCol", "Type"))
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

	return SAssignNew(ListView, SListView<TSharedPtr<FItemSyncEntry>>)
		.ListItemsSource(&DisplayedEntries)
		.OnGenerateRow(this, &SItemXLSXSyncDialog::OnGenerateRow)
		.HeaderRow(HeaderRow)
		.SelectionMode(ESelectionMode::None);
}

TSharedRef<SWidget> SItemXLSXSyncDialog::BuildFooter()
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

TSharedRef<ITableRow> SItemXLSXSyncDialog::OnGenerateRow(TSharedPtr<FItemSyncEntry> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SItemSyncEntryRow, OwnerTable)
		.Entry(Item)
		.OnResolutionChanged_Lambda([this]()
		{
			// Force refresh to update Apply button state and row colors
			ListView->RequestListRefresh();
		});
}

FReply SItemXLSXSyncDialog::OnApplyClicked()
{
	DialogResult = EItemSyncDialogResult::Apply;
	if (DialogWindow.IsValid())
	{
		DialogWindow.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

FReply SItemXLSXSyncDialog::OnCancelClicked()
{
	DialogResult = EItemSyncDialogResult::Cancel;
	if (DialogWindow.IsValid())
	{
		DialogWindow.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

void SItemXLSXSyncDialog::OnShowUnchangedChanged(ECheckBoxState NewState)
{
	bShowUnchanged = (NewState == ECheckBoxState::Checked);
	RefreshEntryList();
}

void SItemXLSXSyncDialog::OnShowConflictsOnlyChanged(ECheckBoxState NewState)
{
	bShowConflictsOnly = (NewState == ECheckBoxState::Checked);
	RefreshEntryList();
}

void SItemXLSXSyncDialog::RefreshEntryList()
{
	DisplayedEntries.Empty();

	if (SyncResult)
	{
		for (FItemSyncEntry& Entry : SyncResult->Entries)
		{
			// Apply filters - by default hide Unchanged
			if (!bShowUnchanged && Entry.Status == EItemSyncStatus::Unchanged)
			{
				continue;
			}
			if (bShowConflictsOnly && !Entry.RequiresResolution())
			{
				continue;
			}

			DisplayedEntries.Add(MakeShared<FItemSyncEntry>(Entry));
		}
	}

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

FText SItemXLSXSyncDialog::GetSummaryText() const
{
	if (!SyncResult)
	{
		return FText::GetEmpty();
	}

	// v4.11.1: Count changes requiring resolution and how many are resolved
	int32 TotalChanges = 0;
	int32 ResolvedCount = 0;

	for (const FItemSyncEntry& Entry : SyncResult->Entries)
	{
		if (Entry.RequiresResolution())
		{
			TotalChanges++;
			if (Entry.Resolution != EItemConflictResolution::Unresolved)
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

bool SItemXLSXSyncDialog::CanApply() const
{
	// v4.12.2: Also check for validation errors
	return SyncResult && FItemXLSXSyncEngine::AllConflictsResolved(*SyncResult) && !SyncResult->HasValidationErrors();
}

//=============================================================================
// SItemSyncEntryRow
//=============================================================================

void SItemSyncEntryRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	Entry = InArgs._Entry;
	OnResolutionChanged = InArgs._OnResolutionChanged;

	SMultiColumnTableRow<TSharedPtr<FItemSyncEntry>>::Construct(
		FSuperRowType::FArguments()
			.Padding(FMargin(2.0f, 1.0f))
			.Style(&FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row")),
		InOwnerTable
	);
}

TSharedRef<SWidget> SItemSyncEntryRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (!Entry.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	// v4.12.2: Wrap all cells in colored border for row highlighting
	TSharedRef<SWidget> CellContent = SNullWidget::NullWidget;

	if (ColumnName == TEXT("Status"))
	{
		CellContent = CreateStatusCell();
	}
	else if (ColumnName == TEXT("Validation"))
	{
		CellContent = CreateValidationCell();
	}
	else if (ColumnName == TEXT("ItemName"))
	{
		CellContent = CreateItemNameCell();
	}
	else if (ColumnName == TEXT("ItemType"))
	{
		CellContent = CreateItemTypeCell();
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
	if (Entry->ValidationStatus == EItemSyncValidationStatus::Error)
	{
		BgColor = FLinearColor(0.8f, 0.2f, 0.2f, 0.2f);  // Red for errors
	}
	else if (Entry->RequiresResolution())
	{
		if (Entry->Resolution == EItemConflictResolution::Unresolved)
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

TSharedRef<SWidget> SItemSyncEntryRow::CreateStatusCell()
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

TSharedRef<SWidget> SItemSyncEntryRow::CreateItemNameCell()
{
	// Get ItemName from whichever version exists
	FString ItemName;
	if (Entry->UERow.IsValid())
	{
		ItemName = Entry->UERow->ItemName;
	}
	else if (Entry->ExcelRow.IsValid())
	{
		ItemName = Entry->ExcelRow->ItemName;
	}
	else if (Entry->BaseRow.IsValid())
	{
		ItemName = Entry->BaseRow->ItemName;
	}

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(STextBlock)
				.Text(FText::FromString(ItemName))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SItemSyncEntryRow::CreateItemTypeCell()
{
	FString ItemType;
	FLinearColor TypeColor = FLinearColor::White;
	if (Entry->UERow.IsValid())
	{
		ItemType = Entry->UERow->GetItemTypeString();
		TypeColor = Entry->UERow->GetItemTypeColor();
	}
	else if (Entry->ExcelRow.IsValid())
	{
		ItemType = Entry->ExcelRow->GetItemTypeString();
		TypeColor = Entry->ExcelRow->GetItemTypeColor();
	}
	else if (Entry->BaseRow.IsValid())
	{
		ItemType = Entry->BaseRow->GetItemTypeString();
		TypeColor = Entry->BaseRow->GetItemTypeColor();
	}

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(STextBlock)
				.Text(FText::FromString(ItemType))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ColorAndOpacity(TypeColor)
		];
}

TSharedRef<SWidget> SItemSyncEntryRow::CreateBaseCell()
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

	// Build preview: DisplayName (truncated)
	FString Preview = Entry->BaseRow->DisplayName;
	if (Preview.IsEmpty()) Preview = Entry->BaseRow->ItemName;
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
					LOCTEXT("BaseTooltip", "Last Export (Base):\nName: {0}\nDisplay: {1}\nValue: {2}"),
					FText::FromString(Entry->BaseRow->ItemName),
					FText::FromString(Entry->BaseRow->DisplayName),
					FText::AsNumber(Entry->BaseRow->BaseValue)))
		];
}

TSharedRef<SWidget> SItemSyncEntryRow::CreateUECell()
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
	FString Preview = Entry->UERow->DisplayName;
	if (Preview.IsEmpty()) Preview = Entry->UERow->ItemName;
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
						LOCTEXT("UETooltipSimple", "UE:\nName: {0}\nDisplay: {1}\nValue: {2}"),
						FText::FromString(Entry->UERow->ItemName),
						FText::FromString(Entry->UERow->DisplayName),
						FText::AsNumber(Entry->UERow->BaseValue)))
			];
	}

	// Radio-style selection
	bool bSelected = (Entry->Resolution == EItemConflictResolution::KeepUE);
	FString RadioIcon = bSelected ? TEXT("*") : TEXT("O");

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(2))
				.OnClicked_Lambda([this]()
				{
					Entry->Resolution = EItemConflictResolution::KeepUE;
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
								LOCTEXT("UETooltip", "UE:\nName: {0}\nDisplay: {1}\nValue: {2}"),
								FText::FromString(Entry->UERow->ItemName),
								FText::FromString(Entry->UERow->DisplayName),
								FText::AsNumber(Entry->UERow->BaseValue)))
					]
				]
		];
}

TSharedRef<SWidget> SItemSyncEntryRow::CreateExcelCell()
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
	FString Preview = Entry->ExcelRow->DisplayName;
	if (Preview.IsEmpty()) Preview = Entry->ExcelRow->ItemName;
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
						LOCTEXT("ExcelTooltipSimple", "Excel:\nName: {0}\nDisplay: {1}\nValue: {2}"),
						FText::FromString(Entry->ExcelRow->ItemName),
						FText::FromString(Entry->ExcelRow->DisplayName),
						FText::AsNumber(Entry->ExcelRow->BaseValue)))
			];
	}

	// Radio-style selection
	bool bSelected = (Entry->Resolution == EItemConflictResolution::KeepExcel);
	FString RadioIcon = bSelected ? TEXT("*") : TEXT("O");

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(2))
				.OnClicked_Lambda([this]()
				{
					Entry->Resolution = EItemConflictResolution::KeepExcel;
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
								LOCTEXT("ExcelTooltip", "Excel:\nName: {0}\nDisplay: {1}\nValue: {2}"),
								FText::FromString(Entry->ExcelRow->ItemName),
								FText::FromString(Entry->ExcelRow->DisplayName),
								FText::AsNumber(Entry->ExcelRow->BaseValue)))
					]
				]
		];
}

TSharedRef<SWidget> SItemSyncEntryRow::CreateActionCell()
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
	bool bSelected = (Entry->Resolution == EItemConflictResolution::Delete);
	FString RadioIcon = bSelected ? TEXT("*") : TEXT("O");

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(2))
				.OnClicked_Lambda([this]()
				{
					Entry->Resolution = EItemConflictResolution::Delete;
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

TSharedRef<SWidget> SItemSyncEntryRow::CreateValidationCell()
{
	// v4.12.2: Show validation status with icon and tooltip
	FString Icon;
	FLinearColor IconColor;
	FText Tooltip;

	switch (Entry->ValidationStatus)
	{
	case EItemSyncValidationStatus::Error:
		Icon = TEXT("✗");
		IconColor = FLinearColor(0.9f, 0.2f, 0.2f);
		Tooltip = FText::FromString(FString::Join(Entry->ValidationMessages, TEXT("\n")));
		break;
	case EItemSyncValidationStatus::Warning:
		Icon = TEXT("⚠");
		IconColor = FLinearColor(0.9f, 0.7f, 0.2f);
		Tooltip = FText::FromString(FString::Join(Entry->ValidationMessages, TEXT("\n")));
		break;
	case EItemSyncValidationStatus::Valid:
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

#undef LOCTEXT_NAMESPACE
