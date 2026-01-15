// GasAbilityGenerator - NPC Apply Preview Window Implementation
// v4.5: Full-screen preview for NPC changes

#include "NPCTableEditor/SNPCApplyPreview.h"
#include "NPCTableEditor/NPCAssetSync.h"
#include "NPCTableEditor/NPCTableValidator.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "AI/NPCDefinition.h"
#include "AssetRegistry/AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "NPCApplyPreview"

//=============================================================================
// SNPCChangeCard - Individual NPC change card
//=============================================================================

void SNPCChangeCard::Construct(const FArguments& InArgs)
{
	Entry = InArgs._ChangeEntry;
	OnApprovalChanged = InArgs._OnApprovalChanged;

	FLinearColor BorderColor = Entry->bIsNew ? FLinearColor(0.2f, 0.6f, 0.2f) : FLinearColor(0.2f, 0.4f, 0.6f);
	if (!Entry->bValid)
	{
		BorderColor = FLinearColor(0.6f, 0.2f, 0.2f);
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(BorderColor * 0.3f)
		.Padding(8.0f)
		[
			SNew(SVerticalBox)

			// Header: NPC info
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				BuildHeader()
			]

			// Field changes
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildFieldChanges()
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

TSharedRef<SWidget> SNPCChangeCard::BuildHeader()
{
	FString StatusText = Entry->bIsNew ? TEXT("[NEW]") : TEXT("[MODIFY]");
	FLinearColor StatusColor = Entry->bIsNew ? FLinearColor(0.2f, 0.8f, 0.2f) : FLinearColor(0.2f, 0.6f, 1.0f);

	if (!Entry->bValid)
	{
		StatusText = TEXT("[INVALID]");
		StatusColor = FLinearColor(1.0f, 0.3f, 0.3f);
	}
	else if (!Entry->bAssetFound && !Entry->bIsNew)
	{
		StatusText = TEXT("[NO ASSET]");
		StatusColor = FLinearColor(0.8f, 0.6f, 0.2f);
	}

	return SNew(SHorizontalBox)

		// Approval checkbox
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 8, 0)
		[
			SNew(SCheckBox)
			.IsChecked(Entry->bApproved && Entry->bValid ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.IsEnabled(Entry->bValid && (Entry->bAssetFound || Entry->bIsNew))
			.OnCheckStateChanged(this, &SNPCChangeCard::OnApprovalCheckChanged)
		]

		// Status badge
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 8, 0)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
			.Padding(FMargin(6, 2))
			[
				SNew(STextBlock)
				.Text(FText::FromString(StatusText))
				.Font(FAppStyle::GetFontStyle("Bold"))
				.ColorAndOpacity(StatusColor)
			]
		]

		// NPC Name
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Entry->NPCName))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		]

		// NPC ID
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(12, 0, 0, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("(%s)"), *Entry->NPCId)))
			.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
		]

		// Display Name
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		.Padding(12, 0, 0, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Entry->DisplayName))
			.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
		]

		// Change count
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("ChangeCount", "{0} changes"), FText::AsNumber(Entry->GetChangedFieldCount())))
			.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
		];
}

TSharedRef<SWidget> SNPCChangeCard::BuildFieldChanges()
{
	TSharedRef<SVerticalBox> FieldsBox = SNew(SVerticalBox);

	// Show validation error if invalid
	if (!Entry->bValid)
	{
		FieldsBox->AddSlot()
		.AutoHeight()
		.Padding(0, 0, 0, 8)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
			.BorderBackgroundColor(FLinearColor(0.3f, 0.1f, 0.1f))
			.Padding(8)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Entry->ValidationError))
				.ColorAndOpacity(FLinearColor(1.0f, 0.5f, 0.5f))
				.AutoWrapText(true)
			]
		];
	}

	// Show field changes
	for (const FNPCFieldChange& Field : Entry->FieldChanges)
	{
		if (Field.bChanged)
		{
			FieldsBox->AddSlot()
			.AutoHeight()
			.Padding(0, 2)
			[
				BuildFieldRow(Field)
			];
		}
	}

	// If new NPC, show all fields
	if (Entry->bIsNew && Entry->FieldChanges.Num() == 0)
	{
		FieldsBox->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NewNPC", "New NPC will be created with current table values"))
			.ColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.2f))
			.Font(FAppStyle::GetFontStyle("Italic"))
		];
	}

	return FieldsBox;
}

TSharedRef<SWidget> SNPCChangeCard::BuildFieldRow(const FNPCFieldChange& Field)
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(6.0f)
		[
			SNew(SHorizontalBox)

			// Field name
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 12, 0)
			[
				SNew(SBox)
				.WidthOverride(120)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Field.FieldName))
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
					.Text(FText::FromString(Field.CurrentValue.IsEmpty() ? TEXT("(none)") : Field.CurrentValue))
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
				.BorderBackgroundColor(FLinearColor(0.1f, 0.2f, 0.1f))
				.Padding(6)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Field.NewValue.IsEmpty() ? TEXT("(none)") : Field.NewValue))
					.ColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.2f))
					.AutoWrapText(true)
				]
			]
		];
}

TSharedRef<SWidget> SNPCChangeCard::BuildNotesRow()
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

void SNPCChangeCard::OnApprovalCheckChanged(ECheckBoxState NewState)
{
	Entry->bApproved = (NewState == ECheckBoxState::Checked);
	OnApprovalChanged.ExecuteIfBound();
}

//=============================================================================
// SNPCApplyPreview - Main preview window
//=============================================================================

void SNPCApplyPreview::Construct(const FArguments& InArgs)
{
	RowsToApply = InArgs._RowsToApply;
	NPCAssetPath = InArgs._NPCAssetPath;
	OnConfirmed = InArgs._OnConfirmed;

	Context.OutputFolder = NPCAssetPath;

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

TSharedRef<SWidget> SNPCApplyPreview::BuildLeftPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(12.0f)
		[
			SNew(SScrollBox)

			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)

				// Title
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 16)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("PreviewTitle", "APPLY TO ASSETS"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 4, 0, 0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("PreviewSubtitle", "Review changes before applying"))
						.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
					]
				]

				// Output folder
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 16)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("OutputFolderLabel", "OUTPUT FOLDER"))
						.Font(FAppStyle::GetFontStyle("Bold"))
						.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 4, 0, 0)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Context.OutputFolder))
						.ColorAndOpacity(FLinearColor(0.2f, 0.6f, 1.0f))
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
						.Text(FText::Format(LOCTEXT("TotalNPCs", "{0} NPCs total"), FText::AsNumber(Context.TotalNPCs)))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 4, 0, 0)
					[
						SNew(STextBlock)
						.Text(FText::Format(LOCTEXT("ModifiedNPCs", "{0} modified"), FText::AsNumber(Context.ModifiedCount)))
						.ColorAndOpacity(FLinearColor(0.2f, 0.6f, 1.0f))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 4, 0, 0)
					[
						SNew(STextBlock)
						.Text(FText::Format(LOCTEXT("NewNPCs", "{0} new"), FText::AsNumber(Context.NewCount)))
						.ColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.2f))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 4, 0, 0)
					[
						SNew(STextBlock)
						.Text(FText::Format(LOCTEXT("UnchangedNPCs", "{0} unchanged"), FText::AsNumber(Context.UnchangedCount)))
						.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 4, 0, 0)
					[
						SNew(SBox)
						.Visibility(Context.InvalidCount > 0 ? EVisibility::Visible : EVisibility::Collapsed)
						[
							SNew(STextBlock)
							.Text(FText::Format(LOCTEXT("InvalidNPCs", "{0} invalid"), FText::AsNumber(Context.InvalidCount)))
							.ColorAndOpacity(FLinearColor(1.0f, 0.3f, 0.3f))
						]
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 16, 0, 0)
				[
					SNew(SSeparator)
				]

				// Approval counts
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 16, 0, 0)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ApprovalLabel", "APPROVAL STATUS"))
						.Font(FAppStyle::GetFontStyle("Bold"))
						.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 8, 0, 0)
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
					.Padding(0, 4, 0, 0)
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

TSharedRef<SWidget> SNPCApplyPreview::BuildRightPanel()
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
				.Text(FText::Format(LOCTEXT("ChangesHeader", "NPC CHANGES ({0})"), FText::AsNumber(ChangeEntries.Num())))
				.Font(FAppStyle::GetFontStyle("Bold"))
			]

			// Change list
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SScrollBox)

				+ SScrollBox::Slot()
				[
					SAssignNew(ChangeListView, SListView<TSharedPtr<FNPCChangeEntry>>)
					.ListItemsSource(&ChangeEntries)
					.OnGenerateRow(this, &SNPCApplyPreview::OnGenerateChangeRow)
					.SelectionMode(ESelectionMode::None)
				]
			]
		];
}

TSharedRef<SWidget> SNPCApplyPreview::BuildBottomBar()
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
				.OnClicked(this, &SNPCApplyPreview::OnApplyClicked)
				.IsEnabled_Lambda([this]() { return Context.ApprovedCount > 0; })
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("CancelButton", "Cancel"))
				.OnClicked(this, &SNPCApplyPreview::OnCancelClicked)
			]
		];
}

TSharedRef<ITableRow> SNPCApplyPreview::OnGenerateChangeRow(TSharedPtr<FNPCChangeEntry> Entry, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FNPCChangeEntry>>, OwnerTable)
		.Padding(FMargin(0, 4))
		[
			SNew(SNPCChangeCard)
			.ChangeEntry(Entry)
			.OnApprovalChanged(FSimpleDelegate::CreateSP(this, &SNPCApplyPreview::OnApprovalChanged))
		];
}

void SNPCApplyPreview::LoadAndCompareData()
{
	ChangeEntries.Empty();

	// Sync from all existing assets to get current state
	FNPCAssetSyncResult SyncResult = FNPCAssetSync::SyncFromAllAssets();

	// Also try to find assets via AssetRegistry for rows with GeneratedNPCDef
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	for (const FNPCTableRow& Row : RowsToApply)
	{
		TSharedPtr<FNPCChangeEntry> Entry = MakeShared<FNPCChangeEntry>();
		Entry->Row = MakeShared<FNPCTableRow>(Row);
		Entry->NPCName = Row.NPCName;
		Entry->NPCId = Row.NPCId;
		Entry->DisplayName = Row.DisplayName;

		// Validate the row (pass empty array for AllRows since we're validating individually)
		TArray<FNPCValidationIssue> Issues = FNPCTableValidator::ValidateRow(Row, RowsToApply);
		for (const FNPCValidationIssue& Issue : Issues)
		{
			if (Issue.Severity == ENPCValidationSeverity::Error)
			{
				Entry->bValid = false;
				if (!Entry->ValidationError.IsEmpty()) Entry->ValidationError += TEXT("; ");
				Entry->ValidationError += Issue.Message;
			}
		}

		// Try to find existing asset data
		const FNPCAssetData* AssetData = SyncResult.NPCData.Find(Row.NPCName);

		if (AssetData && AssetData->bFoundInAsset)
		{
			Entry->bAssetFound = true;
			Entry->bIsNew = false;

			// Compare fields
			auto AddFieldChange = [&Entry](const FString& Name, const FString& Current, const FString& New)
			{
				FNPCFieldChange Change;
				Change.FieldName = Name;
				Change.CurrentValue = Current;
				Change.NewValue = New;
				Change.bChanged = (Current != New);
				Entry->FieldChanges.Add(Change);
			};

			AddFieldChange(TEXT("NPCId"), AssetData->NPCId, Row.NPCId);
			AddFieldChange(TEXT("DisplayName"), AssetData->DisplayName, Row.DisplayName);
			AddFieldChange(TEXT("MinLevel"), FString::FromInt(AssetData->MinLevel), FString::FromInt(Row.MinLevel));
			AddFieldChange(TEXT("MaxLevel"), FString::FromInt(AssetData->MaxLevel), FString::FromInt(Row.MaxLevel));
			AddFieldChange(TEXT("AttackPriority"), FString::SanitizeFloat(AssetData->AttackPriority), FString::SanitizeFloat(Row.AttackPriority));
			AddFieldChange(TEXT("IsVendor"), AssetData->bIsVendor ? TEXT("Yes") : TEXT("No"), Row.bIsVendor ? TEXT("Yes") : TEXT("No"));
			AddFieldChange(TEXT("ShopName"), AssetData->ShopName, Row.ShopName);
			AddFieldChange(TEXT("Factions"), AssetData->Factions, Row.Factions);

			// Soft object paths - compare by asset name
			FString CurrentBlueprint = AssetData->Blueprint.IsValid() ? FPaths::GetBaseFilename(AssetData->Blueprint.GetAssetPathString()) : TEXT("");
			FString NewBlueprint = Row.Blueprint.IsValid() ? FPaths::GetBaseFilename(Row.Blueprint.GetAssetPathString()) : TEXT("");
			AddFieldChange(TEXT("Blueprint"), CurrentBlueprint, NewBlueprint);

			FString CurrentAbilityConfig = AssetData->AbilityConfig.IsValid() ? FPaths::GetBaseFilename(AssetData->AbilityConfig.GetAssetPathString()) : TEXT("");
			FString NewAbilityConfig = Row.AbilityConfig.IsValid() ? FPaths::GetBaseFilename(Row.AbilityConfig.GetAssetPathString()) : TEXT("");
			AddFieldChange(TEXT("AbilityConfig"), CurrentAbilityConfig, NewAbilityConfig);

			FString CurrentActivityConfig = AssetData->ActivityConfig.IsValid() ? FPaths::GetBaseFilename(AssetData->ActivityConfig.GetAssetPathString()) : TEXT("");
			FString NewActivityConfig = Row.ActivityConfig.IsValid() ? FPaths::GetBaseFilename(Row.ActivityConfig.GetAssetPathString()) : TEXT("");
			AddFieldChange(TEXT("ActivityConfig"), CurrentActivityConfig, NewActivityConfig);
		}
		else if (!Row.GeneratedNPCDef.IsNull())
		{
			// Row has asset reference but we didn't find it in full scan - try direct load
			Entry->bAssetFound = true;
			Entry->bIsNew = false;
			// Show as modified with all current values as new
			Entry->FieldChanges.Add({TEXT("(Asset exists)"), TEXT("-"), TEXT("Will update"), false});
		}
		else
		{
			// New NPC - no existing asset
			Entry->bAssetFound = false;
			Entry->bIsNew = true;
		}

		// Auto-deny invalid entries
		Entry->bApproved = Entry->bValid && (Entry->bAssetFound || Entry->bIsNew);

		// Only add entries with changes or new NPCs
		if (Entry->HasChanges() || Entry->bIsNew)
		{
			ChangeEntries.Add(Entry);
		}
	}
}

void SNPCApplyPreview::UpdateContext()
{
	Context.TotalNPCs = RowsToApply.Num();
	Context.ModifiedCount = 0;
	Context.NewCount = 0;
	Context.UnchangedCount = 0;
	Context.InvalidCount = 0;

	for (const TSharedPtr<FNPCChangeEntry>& Entry : ChangeEntries)
	{
		if (!Entry->bValid)
		{
			Context.InvalidCount++;
		}
		else if (Entry->bIsNew)
		{
			Context.NewCount++;
		}
		else if (Entry->GetChangedFieldCount() > 0)
		{
			Context.ModifiedCount++;
		}
		else
		{
			Context.UnchangedCount++;
		}
	}

	// Count NPCs not in change list as unchanged
	Context.UnchangedCount += (Context.TotalNPCs - ChangeEntries.Num());

	UpdateApprovalCounts();
}

void SNPCApplyPreview::UpdateApprovalCounts()
{
	Context.ApprovedCount = 0;
	Context.DeniedCount = 0;

	for (const TSharedPtr<FNPCChangeEntry>& Entry : ChangeEntries)
	{
		if (Entry->bApproved)
		{
			Context.ApprovedCount++;
		}
		else
		{
			Context.DeniedCount++;
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

void SNPCApplyPreview::OnApprovalChanged()
{
	UpdateApprovalCounts();
}

FReply SNPCApplyPreview::OnApplyClicked()
{
	FNPCApplyPreviewResult Result;
	Result.bConfirmed = true;

	for (const TSharedPtr<FNPCChangeEntry>& Entry : ChangeEntries)
	{
		if (Entry->bApproved)
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

FReply SNPCApplyPreview::OnCancelClicked()
{
	FNPCApplyPreviewResult Result;
	Result.bConfirmed = false;

	OnConfirmed.ExecuteIfBound(Result);
	CloseWindow();

	return FReply::Handled();
}

void SNPCApplyPreview::CloseWindow()
{
	if (ParentWindow.IsValid())
	{
		ParentWindow->RequestDestroyWindow();
	}
}

TSharedPtr<SWindow> SNPCApplyPreview::OpenPreviewWindow(
	const TArray<FNPCTableRow>& RowsToApply,
	const FString& NPCAssetPath,
	FOnNPCApplyConfirmed OnConfirmed)
{
	TSharedPtr<SNPCApplyPreview> PreviewWidget;

	TSharedPtr<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("WindowTitle", "Apply NPC Changes"))
		.ClientSize(FVector2D(1200, 800))
		.SupportsMaximize(true)
		.SupportsMinimize(true)
		[
			SAssignNew(PreviewWidget, SNPCApplyPreview)
			.RowsToApply(RowsToApply)
			.NPCAssetPath(NPCAssetPath)
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
