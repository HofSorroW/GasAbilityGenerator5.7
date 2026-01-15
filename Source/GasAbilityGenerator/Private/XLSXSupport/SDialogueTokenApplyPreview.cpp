// GasAbilityGenerator - Dialogue Token Apply Preview Window Implementation
// v4.4: Full-screen preview for token changes

#include "XLSXSupport/SDialogueTokenApplyPreview.h"
#include "XLSXSupport/DialogueAssetSync.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Images/SImage.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "DialogueTokenApplyPreview"

//=============================================================================
// STokenChangeCard - Individual change card
//=============================================================================

void STokenChangeCard::Construct(const FArguments& InArgs)
{
	Entry = InArgs._ChangeEntry;
	OnApprovalChanged = InArgs._OnApprovalChanged;

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8.0f)
		[
			SNew(SVerticalBox)

			// Header: Node info
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				BuildHeader()
			]

			// Events comparison (if changed)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 2)
			[
				SNew(SBox)
				.Visibility(Entry->bEventsChanged ? EVisibility::Visible : EVisibility::Collapsed)
				[
					BuildComparisonRow(
						TEXT("EVENTS"),
						Entry->CurrentEvents,
						Entry->NewEvents,
						Entry->bEventsChanged,
						Entry->bEventsApproved,
						Entry->bEventsValid,
						Entry->EventsValidationError
					)
				]
			]

			// Conditions comparison (if changed)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 2)
			[
				SNew(SBox)
				.Visibility(Entry->bConditionsChanged ? EVisibility::Visible : EVisibility::Collapsed)
				[
					BuildComparisonRow(
						TEXT("CONDITIONS"),
						Entry->CurrentConditions,
						Entry->NewConditions,
						Entry->bConditionsChanged,
						Entry->bConditionsApproved,
						Entry->bConditionsValid,
						Entry->ConditionsValidationError
					)
				]
			]

			// Notes row
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 8, 0, 0)
			[
				BuildNotesRow()
			]
		]
	];
}

TSharedRef<SWidget> STokenChangeCard::BuildHeader()
{
	return SNew(SVerticalBox)

		// Node ID and Speaker
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%s"), *Entry->NodeID.ToString())))
				.Font(FAppStyle::GetFontStyle("Bold"))
				.ColorAndOpacity(FLinearColor(0.2f, 0.6f, 1.0f))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("(%s)"), *Entry->Speaker)))
				.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
			]
		]

		// Dialogue text (truncated)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 4, 0, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("\"") + Entry->DialogueText.Left(80) + (Entry->DialogueText.Len() > 80 ? TEXT("...") : TEXT("")) + TEXT("\"")))
			.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
			.Font(FAppStyle::GetFontStyle("Italic"))
		];
}

TSharedRef<SWidget> STokenChangeCard::BuildComparisonRow(
	const FString& Label,
	const FString& Current,
	const FString& New,
	bool bChanged,
	bool& bApproved,
	bool bValid,
	const FString& ValidationError)
{
	FLinearColor NewColor = bValid ? FLinearColor(0.2f, 0.8f, 0.2f) : FLinearColor(1.0f, 0.3f, 0.3f);

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(6.0f)
		[
			SNew(SHorizontalBox)

			// Label
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 12, 0)
			[
				SNew(SBox)
				.WidthOverride(100)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Label))
					.Font(FAppStyle::GetFontStyle("Bold"))
				]
			]

			// Current value
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.Padding(4)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(6)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Current.IsEmpty() ? TEXT("(none)") : Current))
					.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
					.AutoWrapText(true)
				]
			]

			// Arrow
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(8, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("->")))
				.Font(FAppStyle::GetFontStyle("Bold"))
			]

			// New value
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.Padding(4)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.BorderBackgroundColor(bValid ? FLinearColor(0.1f, 0.2f, 0.1f) : FLinearColor(0.3f, 0.1f, 0.1f))
				.Padding(6)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(New.IsEmpty() ? TEXT("(none)") : New))
						.ColorAndOpacity(NewColor)
						.AutoWrapText(true)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBox)
						.Visibility(bValid ? EVisibility::Collapsed : EVisibility::Visible)
						[
							SNew(STextBlock)
							.Text(FText::FromString(ValidationError))
							.ColorAndOpacity(FLinearColor(1.0f, 0.5f, 0.5f))
							.Font(FAppStyle::GetFontStyle("Small"))
						]
					]
				]
			]

			// Approve/Deny checkbox
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(12, 0, 0, 0)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SCheckBox)
					.IsChecked(bApproved && bValid ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
					.IsEnabled(bValid)
					.OnCheckStateChanged_Lambda([this, &bApproved](ECheckBoxState NewState)
					{
						bApproved = (NewState == ECheckBoxState::Checked);
						OnApprovalChanged.ExecuteIfBound();
					})
					.ToolTipText(bValid ? LOCTEXT("ApproveTooltip", "Approve this change") : LOCTEXT("InvalidTooltip", "Cannot approve - validation error"))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4, 0, 0, 0)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(bValid ? LOCTEXT("Approve", "Apply") : LOCTEXT("Invalid", "Invalid"))
					.ColorAndOpacity(bValid ? FLinearColor::White : FLinearColor(1.0f, 0.5f, 0.5f))
				]
			]
		];
}

TSharedRef<SWidget> STokenChangeCard::BuildNotesRow()
{
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 8, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Notes", "Note:"))
			.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SEditableTextBox)
			.Text(FText::FromString(Entry->ApprovalNotes))
			.HintText(LOCTEXT("NotesHint", "Add a note about this change..."))
			.OnTextChanged_Lambda([this](const FText& NewText)
			{
				Entry->ApprovalNotes = NewText.ToString();
			})
		];
}

FSlateColor STokenChangeCard::GetChangeColor(bool bChanged, bool bValid) const
{
	if (!bValid) return FSlateColor(FLinearColor(1.0f, 0.3f, 0.3f));
	if (bChanged) return FSlateColor(FLinearColor(0.2f, 0.8f, 0.2f));
	return FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f));
}

//=============================================================================
// SDialogueTokenApplyPreview - Main preview window
//=============================================================================

void SDialogueTokenApplyPreview::Construct(const FArguments& InArgs)
{
	ImportedRows = InArgs._ImportedRows;
	DialogueAssetPath = InArgs._DialogueAssetPath;
	OnConfirmed = InArgs._OnConfirmed;

	// Load and compare data
	LoadAndCompareData();
	UpdateContext();

	ChildSlot
	[
		SNew(SVerticalBox)

		// Main content area
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)

			// Left panel - Context
			+ SSplitter::Slot()
			.Value(0.25f)
			[
				BuildLeftPanel()
			]

			// Right panel - Changes
			+ SSplitter::Slot()
			.Value(0.75f)
			[
				BuildRightPanel()
			]
		]

		// Bottom bar - Actions
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			BuildBottomBar()
		]
	];
}

TSharedRef<SWidget> SDialogueTokenApplyPreview::BuildLeftPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(12.0f)
		[
			SNew(SScrollBox)

			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)

				// Quest section
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 16)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("QuestLabel", "QUEST"))
						.Font(FAppStyle::GetFontStyle("Bold"))
						.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 4, 0, 0)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Context.QuestName.IsEmpty() ? TEXT("(Not linked to quest)") : Context.QuestName))
						.Font(FAppStyle::GetFontStyle("Large"))
					]
				]

				// Dialogue section
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 16)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("DialogueLabel", "DIALOGUE"))
						.Font(FAppStyle::GetFontStyle("Bold"))
						.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 4, 0, 0)
					[
						SNew(STextBlock)
						.Text(FText::FromName(Context.DialogueID))
						.Font(FAppStyle::GetFontStyle("Large"))
						.ColorAndOpacity(FLinearColor(0.2f, 0.6f, 1.0f))
					]
				]

				// Speakers section
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 16)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SpeakersLabel", "SPEAKERS"))
						.Font(FAppStyle::GetFontStyle("Bold"))
						.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 4, 0, 0)
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Join(Context.Speakers, TEXT("\n"))))
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSeparator)
				]

				// Summary section
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 16, 0, 0)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SummaryLabel", "SUMMARY"))
						.Font(FAppStyle::GetFontStyle("Bold"))
						.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 8, 0, 0)
					[
						SNew(STextBlock)
						.Text(FText::Format(LOCTEXT("TotalChanges", "{0} changes"), FText::AsNumber(Context.TotalChanges)))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 2, 0, 0)
					[
						SNew(STextBlock)
						.Text(FText::Format(LOCTEXT("EventChanges", "{0} event changes"), FText::AsNumber(Context.EventChanges)))
						.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 2, 0, 0)
					[
						SNew(STextBlock)
						.Text(FText::Format(LOCTEXT("ConditionChanges", "{0} condition changes"), FText::AsNumber(Context.ConditionChanges)))
						.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 12, 0, 0)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ApprovedLabel", "Approved: "))
							.ColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.2f))
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SAssignNew(ApprovedCountText, STextBlock)
							.Text(FText::AsNumber(Context.ApprovedCount))
							.ColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.2f))
							.Font(FAppStyle::GetFontStyle("Bold"))
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 2, 0, 0)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("DeniedLabel", "Skipped: "))
							.ColorAndOpacity(FLinearColor(1.0f, 0.5f, 0.5f))
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SAssignNew(DeniedCountText, STextBlock)
							.Text(FText::AsNumber(Context.DeniedCount))
							.ColorAndOpacity(FLinearColor(1.0f, 0.5f, 0.5f))
							.Font(FAppStyle::GetFontStyle("Bold"))
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SDialogueTokenApplyPreview::BuildRightPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(8.0f)
		[
			SNew(SVerticalBox)

			// Header
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8, 8, 8, 16)
			[
				SNew(STextBlock)
				.Text(FText::Format(LOCTEXT("ChangesHeader", "CHANGES ({0})"), FText::AsNumber(ChangeEntries.Num())))
				.Font(FAppStyle::GetFontStyle("Bold"))
			]

			// Change list
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SScrollBox)

				+ SScrollBox::Slot()
				[
					SAssignNew(ChangeListView, SListView<TSharedPtr<FTokenChangeEntry>>)
					.ListItemsSource(&ChangeEntries)
					.OnGenerateRow(this, &SDialogueTokenApplyPreview::OnGenerateChangeRow)
					.SelectionMode(ESelectionMode::None)
				]
			]
		];
}

TSharedRef<SWidget> SDialogueTokenApplyPreview::BuildBottomBar()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(12.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SSpacer)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 8, 0)
			[
				SNew(SButton)
				.Text_Lambda([this]() { return FText::Format(LOCTEXT("ApplyButton", "Apply Approved ({0})"), FText::AsNumber(Context.ApprovedCount)); })
				.OnClicked(this, &SDialogueTokenApplyPreview::OnApplyClicked)
				.IsEnabled_Lambda([this]() { return Context.ApprovedCount > 0; })
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("CancelButton", "Cancel"))
				.OnClicked(this, &SDialogueTokenApplyPreview::OnCancelClicked)
			]
		];
}

TSharedRef<ITableRow> SDialogueTokenApplyPreview::OnGenerateChangeRow(TSharedPtr<FTokenChangeEntry> Entry, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FTokenChangeEntry>>, OwnerTable)
		.Padding(FMargin(0, 4))
		[
			SNew(STokenChangeCard)
			.ChangeEntry(Entry)
			.OnApprovalChanged(FSimpleDelegate::CreateSP(this, &SDialogueTokenApplyPreview::OnApprovalChanged))
		];
}

void SDialogueTokenApplyPreview::LoadAndCompareData()
{
	ChangeEntries.Empty();

	// Get unique dialogue IDs from imported rows
	TSet<FName> DialogueIDs;
	for (const FDialogueTableRow& Row : ImportedRows)
	{
		DialogueIDs.Add(Row.DialogueID);
	}

	// For each dialogue, sync from asset and compare
	for (const FName& DialogueID : DialogueIDs)
	{
		// Try to load the dialogue blueprint
		FDialogueAssetSyncResult SyncResult = FDialogueAssetSync::SyncFromAssetPath(
			FString::Printf(TEXT("/Game/Dialogues/%s.%s"), *DialogueID.ToString(), *DialogueID.ToString())
		);

		// If not found, try alternate paths
		if (!SyncResult.bSuccess)
		{
			SyncResult = FDialogueAssetSync::SyncFromAssetPath(
				FString::Printf(TEXT("%s%s.%s"), *DialogueAssetPath, *DialogueID.ToString(), *DialogueID.ToString())
			);
		}

		// Compare imported rows with current asset state
		for (const FDialogueTableRow& Row : ImportedRows)
		{
			if (Row.DialogueID != DialogueID) continue;

			// Get current state from sync result
			FString Key = FDialogueAssetSyncResult::MakeKey(Row.DialogueID, Row.NodeID);
			const FDialogueNodeAssetData* AssetData = SyncResult.NodeData.Find(Key);

			FString CurrentEvents = AssetData ? AssetData->EventsTokenStr : TEXT("");
			FString CurrentConditions = AssetData ? AssetData->ConditionsTokenStr : TEXT("");

			// Check if there are changes
			bool bEventsChanged = (Row.EventsTokenStr != CurrentEvents) && !Row.EventsTokenStr.IsEmpty();
			bool bConditionsChanged = (Row.ConditionsTokenStr != CurrentConditions) && !Row.ConditionsTokenStr.IsEmpty();

			if (!bEventsChanged && !bConditionsChanged) continue;

			// Create change entry
			TSharedPtr<FTokenChangeEntry> Entry = MakeShared<FTokenChangeEntry>();
			Entry->Row = MakeShared<FDialogueTableRow>(Row);
			Entry->NodeID = Row.NodeID;
			Entry->Speaker = Row.Speaker.ToString();
			Entry->DialogueText = Row.Text;

			Entry->CurrentEvents = CurrentEvents;
			Entry->CurrentConditions = CurrentConditions;
			Entry->NewEvents = Row.EventsTokenStr;
			Entry->NewConditions = Row.ConditionsTokenStr;

			Entry->bEventsChanged = bEventsChanged;
			Entry->bConditionsChanged = bConditionsChanged;

			Entry->bEventsValid = Row.bEventsValid;
			Entry->bConditionsValid = Row.bConditionsValid;
			Entry->EventsValidationError = Row.EventsValidationError;
			Entry->ConditionsValidationError = Row.ConditionsValidationError;

			// Auto-deny invalid tokens
			Entry->bEventsApproved = Entry->bEventsValid;
			Entry->bConditionsApproved = Entry->bConditionsValid;

			ChangeEntries.Add(Entry);
		}

		// Set context from first dialogue found
		if (Context.DialogueID.IsNone() && DialogueIDs.Num() > 0)
		{
			Context.DialogueID = DialogueID;
		}
	}
}

void SDialogueTokenApplyPreview::UpdateContext()
{
	// Collect speakers
	TSet<FString> SpeakerSet;
	for (const TSharedPtr<FTokenChangeEntry>& Entry : ChangeEntries)
	{
		if (!Entry->Speaker.IsEmpty())
		{
			SpeakerSet.Add(Entry->Speaker);
		}
	}
	Context.Speakers = SpeakerSet.Array();

	// Count changes
	Context.TotalChanges = ChangeEntries.Num();
	Context.EventChanges = 0;
	Context.ConditionChanges = 0;

	for (const TSharedPtr<FTokenChangeEntry>& Entry : ChangeEntries)
	{
		if (Entry->bEventsChanged) Context.EventChanges++;
		if (Entry->bConditionsChanged) Context.ConditionChanges++;
	}

	UpdateApprovalCounts();
}

void SDialogueTokenApplyPreview::UpdateApprovalCounts()
{
	Context.ApprovedCount = 0;
	Context.DeniedCount = 0;

	for (const TSharedPtr<FTokenChangeEntry>& Entry : ChangeEntries)
	{
		if (Entry->bEventsChanged)
		{
			if (Entry->bEventsApproved) Context.ApprovedCount++;
			else Context.DeniedCount++;
		}
		if (Entry->bConditionsChanged)
		{
			if (Entry->bConditionsApproved) Context.ApprovedCount++;
			else Context.DeniedCount++;
		}
	}

	// Update UI
	if (ApprovedCountText.IsValid())
	{
		ApprovedCountText->SetText(FText::AsNumber(Context.ApprovedCount));
	}
	if (DeniedCountText.IsValid())
	{
		DeniedCountText->SetText(FText::AsNumber(Context.DeniedCount));
	}
}

void SDialogueTokenApplyPreview::OnApprovalChanged()
{
	UpdateApprovalCounts();
}

FReply SDialogueTokenApplyPreview::OnApplyClicked()
{
	FTokenApplyPreviewResult Result;
	Result.bConfirmed = true;

	for (const TSharedPtr<FTokenChangeEntry>& Entry : ChangeEntries)
	{
		if (Entry->IsFullyApproved() || Entry->bEventsApproved || Entry->bConditionsApproved)
		{
			// Copy approval notes back to row
			if (Entry->Row.IsValid())
			{
				Entry->Row->Notes = Entry->ApprovalNotes.IsEmpty() ? Entry->Row->Notes : Entry->ApprovalNotes;
			}

			Result.ApprovedChanges.Add(*Entry);
		}
	}

	Result.TotalApproved = Context.ApprovedCount;
	Result.TotalDenied = Context.DeniedCount;

	OnConfirmed.ExecuteIfBound(Result);
	CloseWindow();

	return FReply::Handled();
}

FReply SDialogueTokenApplyPreview::OnCancelClicked()
{
	FTokenApplyPreviewResult Result;
	Result.bConfirmed = false;

	OnConfirmed.ExecuteIfBound(Result);
	CloseWindow();

	return FReply::Handled();
}

void SDialogueTokenApplyPreview::CloseWindow()
{
	if (ParentWindow.IsValid())
	{
		ParentWindow->RequestDestroyWindow();
	}
}

TSharedPtr<SWindow> SDialogueTokenApplyPreview::OpenPreviewWindow(
	const TArray<FDialogueTableRow>& ImportedRows,
	const FString& DialogueAssetPath,
	FOnTokenApplyConfirmed OnConfirmed)
{
	TSharedPtr<SDialogueTokenApplyPreview> PreviewWidget;

	TSharedPtr<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("WindowTitle", "Apply Token Changes"))
		.ClientSize(FVector2D(1200, 800))
		.SupportsMaximize(true)
		.SupportsMinimize(true)
		[
			SAssignNew(PreviewWidget, SDialogueTokenApplyPreview)
			.ImportedRows(ImportedRows)
			.DialogueAssetPath(DialogueAssetPath)
			.OnConfirmed(OnConfirmed)
		];

	if (PreviewWidget.IsValid())
	{
		PreviewWidget->ParentWindow = Window;
	}

	FSlateApplication::Get().AddWindow(Window.ToSharedRef());

	return Window;
}

#undef LOCTEXT_NAMESPACE
