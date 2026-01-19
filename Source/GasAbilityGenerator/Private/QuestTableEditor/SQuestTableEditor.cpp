// SQuestTableEditor.cpp
// Quest Table Editor Implementation
// v4.8: Follows NPC/Dialogue table patterns
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#include "QuestTableEditor/SQuestTableEditor.h"
#include "QuestTableConverter.h"
#include "QuestTableValidator.h"
#include "QuestTableEditor/QuestXLSXWriter.h"  // v4.12: XLSX export
#include "QuestTableEditor/QuestXLSXReader.h"  // v4.12: XLSX import
#include "XLSXSupport/QuestXLSXSyncEngine.h"   // v4.12: 3-way sync engine
#include "XLSXSupport/SQuestXLSXSyncDialog.h"  // v4.12: Sync dialog
#include "QuestTableEditor/QuestAssetSync.h"   // v4.12: Asset sync
#include "GasAbilityGeneratorGenerators.h"
#include "DesktopPlatformModule.h"  // v4.12: File dialogs
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Views/STableRow.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/MessageDialog.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Tales/Quest.h"
#include "Tales/QuestSM.h"
#include "Tales/NarrativeNodeBase.h"
#include "QuestBlueprint.h"  // v4.12.5: For UQuestBlueprint sync

#define LOCTEXT_NAMESPACE "QuestTableEditor"

//=============================================================================
// Helper Functions
//=============================================================================

static FString TrimQuestPrefix(const FString& QuestName)
{
	// Strip Q_ or QBP_ prefix for cleaner display (case-insensitive)
	if (QuestName.StartsWith(TEXT("QBP_"), ESearchCase::IgnoreCase))
	{
		return QuestName.RightChop(4);
	}
	if (QuestName.StartsWith(TEXT("Q_"), ESearchCase::IgnoreCase))
	{
		return QuestName.RightChop(2);
	}
	return QuestName;
}

//=============================================================================
// SQuestTableRow
//=============================================================================

void SQuestTableRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	RowDataEx = InArgs._RowData;
	OnRowModified = InArgs._OnRowModified;

	// v4.12.5: Match NPC Table row padding
	SMultiColumnTableRow<TSharedPtr<FQuestTableRowEx>>::Construct(
		FSuperRowType::FArguments()
			.Padding(FMargin(2.0f, 1.0f)),
		InOwnerTable
	);
}

TSharedRef<SWidget> SQuestTableRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (!RowDataEx.IsValid() || !RowDataEx->Data.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	FQuestTableRow& Row = *RowDataEx->Data;

	if (ColumnName == TEXT("Status"))
	{
		return CreateStatusCell();
	}
	else if (ColumnName == TEXT("QuestName"))
	{
		return CreateQuestNameCell();
	}
	else if (ColumnName == TEXT("DisplayName"))
	{
		return CreateTextCell(Row.DisplayName, TEXT("Quest log name"));
	}
	else if (ColumnName == TEXT("StateID"))
	{
		return CreateTextCell(Row.StateID, TEXT("Start, Gathering, Complete..."));
	}
	else if (ColumnName == TEXT("StateType"))
	{
		return CreateStateTypeCell();
	}
	else if (ColumnName == TEXT("Description"))
	{
		return CreateTextCell(Row.Description, TEXT("Quest tracker text"));
	}
	else if (ColumnName == TEXT("ParentBranch"))
	{
		return CreateTextCell(Row.ParentBranch, TEXT("From state"));
	}
	else if (ColumnName == TEXT("Tasks"))
	{
		return CreateTokenCell(Row.Tasks, TEXT("BPT_FindItem(...)"));
	}
	else if (ColumnName == TEXT("Events"))
	{
		return CreateTokenCell(Row.Events, TEXT("NE_GiveXP(...)"));
	}
	else if (ColumnName == TEXT("Conditions"))
	{
		return CreateTokenCell(Row.Conditions, TEXT("NC_HasItem(...)"));
	}
	else if (ColumnName == TEXT("Rewards"))
	{
		return CreateTokenCell(Row.Rewards, TEXT("Reward(...)"));
	}
	else if (ColumnName == TEXT("Notes"))
	{
		return CreateNotesCell();
	}

	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SQuestTableRow::CreateStatusCell()
{
	// v4.12.5: Match NPC Table cell styling with validation stripe + status badge
	// Layout: [4px validation stripe] [status badge]
	return SNew(SHorizontalBox)
		// Validation stripe (4px colored bar on left - matches NPC/Dialogue pattern)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
				.WidthOverride(4.0f)
				[
					SNew(SBorder)
						.BorderBackgroundColor_Lambda([this]()
						{
							if (RowDataEx.IsValid() && RowDataEx->Data.IsValid())
							{
								return FSlateColor(RowDataEx->Data->GetValidationColor());
							}
							return FSlateColor(FLinearColor::White);
						})
				]
		]
		// Status badge
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding(FMargin(4.0f, 2.0f))
		.HAlign(HAlign_Center)
		[
			SNew(SBorder)
				.BorderBackgroundColor_Lambda([this]()
				{
					return RowDataEx.IsValid() && RowDataEx->Data.IsValid() ?
						FSlateColor(RowDataEx->Data->GetStatusColor()) : FSlateColor(FLinearColor::White);
				})
				.Padding(FMargin(6.0f, 2.0f))
				[
					SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return RowDataEx.IsValid() && RowDataEx->Data.IsValid() ?
								FText::FromString(RowDataEx->Data->GetStatusString()) : FText::GetEmpty();
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
				]
		];
}

TSharedRef<SWidget> SQuestTableRow::CreateTextCell(FString& Value, const FString& Hint)
{
	// v4.12.5: Match NPC Table cell styling with confirmation dialog
	FString* ValuePtr = &Value;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableTextBox)
				.Text_Lambda([ValuePtr]()
				{
					return FText::FromString(ValuePtr->IsEmpty() ? TEXT("(None)") : *ValuePtr);
				})
				.HintText(FText::FromString(Hint))
				.OnTextCommitted_Lambda([this, ValuePtr, Hint](const FText& NewText, ETextCommit::Type CommitType)
				{
					if (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus)
					{
						FString NewValue = NewText.ToString();
						// Treat "(None)" input as clearing the field
						if (NewValue == TEXT("(None)"))
						{
							NewValue = TEXT("");
						}

						// Only prompt if value actually changed
						if (*ValuePtr != NewValue)
						{
							FString DisplayOld = ValuePtr->IsEmpty() ? TEXT("(None)") : *ValuePtr;
							FString DisplayNew = NewValue.IsEmpty() ? TEXT("(None)") : NewValue;
							EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
								FText::Format(NSLOCTEXT("QuestTableEditor", "ConfirmTextChange", "Change '{0}' from '{1}' to '{2}'?\n\nThis will mark the Quest as modified."),
									FText::FromString(Hint),
									FText::FromString(DisplayOld),
									FText::FromString(DisplayNew)));

							if (Result == EAppReturnType::Yes)
							{
								*ValuePtr = NewValue;
								MarkModified();
							}
						}
					}
				})
				.SelectAllTextWhenFocused(true)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SQuestTableRow::CreateStateTypeCell()
{
	// v4.12.5: Plain text cell matching other tables (no colored background)
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(STextBlock)
				.Text_Lambda([this]()
				{
					return RowDataEx.IsValid() && RowDataEx->Data.IsValid() ?
						FText::FromString(RowDataEx->Data->GetStateTypeString()) : FText::GetEmpty();
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SQuestTableRow::CreateTokenCell(FString& Value, const FString& Hint)
{
	// v4.12.5: Match NPC Table cell styling with confirmation dialog
	FString* ValuePtr = &Value;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableTextBox)
				.Text_Lambda([ValuePtr]()
				{
					return FText::FromString(ValuePtr->IsEmpty() ? TEXT("(None)") : *ValuePtr);
				})
				.HintText(FText::FromString(Hint))
				.OnTextCommitted_Lambda([this, ValuePtr, Hint](const FText& NewText, ETextCommit::Type CommitType)
				{
					if (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus)
					{
						FString NewValue = NewText.ToString();
						if (NewValue == TEXT("(None)"))
						{
							NewValue = TEXT("");
						}

						if (*ValuePtr != NewValue)
						{
							FString DisplayOld = ValuePtr->IsEmpty() ? TEXT("(None)") : *ValuePtr;
							FString DisplayNew = NewValue.IsEmpty() ? TEXT("(None)") : NewValue;
							EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
								FText::Format(NSLOCTEXT("QuestTableEditor", "ConfirmTokenChange", "Change '{0}' from '{1}' to '{2}'?\n\nThis will mark the Quest as modified."),
									FText::FromString(Hint),
									FText::FromString(DisplayOld),
									FText::FromString(DisplayNew)));

							if (Result == EAppReturnType::Yes)
							{
								*ValuePtr = NewValue;
								MarkModified();
							}
						}
					}
				})
				.SelectAllTextWhenFocused(true)
				.Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
		];
}

TSharedRef<SWidget> SQuestTableRow::CreateQuestNameCell()
{
	// v4.12.6: Click-to-open Quest asset + dropdown for single-select + trimmed display
	FString* ValuePtr = &RowDataEx->Data->QuestName;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SBorder)
				.BorderBackgroundColor_Lambda([this]()
				{
					// Visual grouping indicator for first row in quest
					return RowDataEx.IsValid() && RowDataEx->bIsFirstInQuest ?
						FLinearColor(0.15f, 0.15f, 0.2f) : FLinearColor(0.1f, 0.1f, 0.1f);
				})
				.Padding(FMargin(4.0f, 2.0f))
				[
					SNew(SHorizontalBox)

					// Clickable hyperlink text - opens Quest Blueprint asset in editor
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "NoBorder")
							.ContentPadding(FMargin(0))
							.OnClicked_Lambda([ValuePtr]() -> FReply
							{
								if (!ValuePtr->IsEmpty())
								{
									// Find Quest Blueprint asset by name
									FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
									TArray<FAssetData> Assets;
									Registry.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), Assets, true);

									for (const FAssetData& Asset : Assets)
									{
										if (Asset.AssetName.ToString().Equals(*ValuePtr, ESearchCase::IgnoreCase))
										{
											// Check if this is a Quest blueprint
											FAssetDataTagMapSharedView::FFindTagResult ParentClassTag = Asset.TagsAndValues.FindTag(FBlueprintTags::ParentClassPath);
											if (ParentClassTag.IsSet())
											{
												FString ParentClassPath = ParentClassTag.GetValue();
												if (ParentClassPath.Contains(TEXT("Quest")) ||
													Asset.AssetName.ToString().StartsWith(TEXT("Q_")) ||
													Asset.AssetName.ToString().StartsWith(TEXT("QBP_")))
												{
													if (UObject* LoadedAsset = Asset.GetAsset())
													{
														GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(LoadedAsset);
													}
													break;
												}
											}
										}
									}
								}
								return FReply::Handled();
							})
							.ToolTipText_Lambda([ValuePtr]()
							{
								if (ValuePtr->IsEmpty())
								{
									return FText::FromString(TEXT("No Quest selected"));
								}
								return FText::Format(NSLOCTEXT("QuestTableEditor", "ClickToOpenQuest", "Click to open: {0}"),
									FText::FromString(*ValuePtr));
							})
							.Cursor(EMouseCursor::Hand)
							[
								SNew(STextBlock)
									.Text_Lambda([ValuePtr]()
									{
										if (ValuePtr->IsEmpty())
										{
											return FText::FromString(TEXT("(None)"));
										}
										return FText::FromString(TrimQuestPrefix(*ValuePtr));
									})
									.Font_Lambda([this]()
									{
										return RowDataEx.IsValid() && RowDataEx->bIsFirstInQuest ?
											FCoreStyle::GetDefaultFontStyle("Bold", 9) :
											FCoreStyle::GetDefaultFontStyle("Regular", 9);
									})
									.ColorAndOpacity_Lambda([ValuePtr]()
									{
										// Blue color for clickable assets, gray for (None)
										if (ValuePtr->IsEmpty())
										{
											return FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f));
										}
										return FSlateColor(FLinearColor(0.3f, 0.5f, 0.85f));
									})
							]
					]

					// Dropdown arrow - shows selection menu
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SComboButton)
							.HasDownArrow(true)
							.ButtonStyle(FAppStyle::Get(), "NoBorder")
							.ContentPadding(FMargin(2.0f, 0.0f))
							.OnGetMenuContent_Lambda([this, ValuePtr]() -> TSharedRef<SWidget>
							{
								FMenuBuilder MenuBuilder(true, nullptr);

								// None option
								MenuBuilder.AddMenuEntry(
									NSLOCTEXT("QuestTableEditor", "QuestNone", "(None)"),
									FText::GetEmpty(),
									FSlateIcon(),
									FUIAction(FExecuteAction::CreateLambda([this, ValuePtr]()
									{
										if (!ValuePtr->IsEmpty())
										{
											EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
												FText::Format(NSLOCTEXT("QuestTableEditor", "ConfirmClearQuest", "Clear Quest '{0}'?\n\nThis will mark the Quest as modified."),
													FText::FromString(TrimQuestPrefix(*ValuePtr))));
											if (Result == EAppReturnType::Yes)
											{
												ValuePtr->Empty();
												MarkModified();
											}
										}
									}))
								);

								MenuBuilder.AddSeparator();

								// Get all Quest Blueprint assets (Q_*, QBP_*)
								FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
								TArray<FAssetData> Assets;
								Registry.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), Assets, true);

								for (const FAssetData& Asset : Assets)
								{
									FString AssetName = Asset.AssetName.ToString();

									// Must have Q_ or QBP_ prefix or Quest in parent class
									bool bIsQuest = AssetName.StartsWith(TEXT("Q_")) || AssetName.StartsWith(TEXT("QBP_"));
									if (!bIsQuest)
									{
										FAssetDataTagMapSharedView::FFindTagResult ParentClassTag = Asset.TagsAndValues.FindTag(FBlueprintTags::ParentClassPath);
										if (ParentClassTag.IsSet())
										{
											bIsQuest = ParentClassTag.GetValue().Contains(TEXT("Quest"));
										}
									}

									if (!bIsQuest)
									{
										continue;
									}

									MenuBuilder.AddMenuEntry(
										FText::FromString(TrimQuestPrefix(AssetName)),
										FText::FromString(Asset.GetObjectPathString()),
										FSlateIcon(),
										FUIAction(FExecuteAction::CreateLambda([this, ValuePtr, AssetName]()
										{
											if (*ValuePtr != AssetName)
											{
												FString OldValue = ValuePtr->IsEmpty() ? TEXT("(None)") : TrimQuestPrefix(*ValuePtr);
												EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
													FText::Format(NSLOCTEXT("QuestTableEditor", "ConfirmQuestNameChange", "Change Quest Name from '{0}' to '{1}'?\n\nThis will mark the Quest as modified."),
														FText::FromString(OldValue),
														FText::FromString(TrimQuestPrefix(AssetName))));

												if (Result == EAppReturnType::Yes)
												{
													*ValuePtr = AssetName;
													MarkModified();
												}
											}
										}))
									);
								}

								return MenuBuilder.MakeWidget();
							})
							.ButtonContent()
							[
								SNew(SBox)
									.WidthOverride(12.0f)
									.HeightOverride(12.0f)
							]
					]
				]
		];
}

TSharedRef<SWidget> SQuestTableRow::CreateNotesCell()
{
	// v4.12.5: Match NPC Table cell styling with confirmation dialog
	FString* ValuePtr = &RowDataEx->Data->Notes;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		.ToolTipText_Lambda([ValuePtr]() -> FText
		{
			// Show full text as tooltip when hovering (useful for long notes)
			if (ValuePtr->Len() > 30)
			{
				return FText::FromString(*ValuePtr);
			}
			return FText::GetEmpty();
		})
		[
			SNew(SEditableTextBox)
				.Text_Lambda([ValuePtr]()
				{
					return FText::FromString(ValuePtr->IsEmpty() ? TEXT("(None)") : *ValuePtr);
				})
				.HintText(LOCTEXT("NotesHint", "Designer notes"))
				.OnTextCommitted_Lambda([this, ValuePtr](const FText& NewText, ETextCommit::Type CommitType)
				{
					if (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus)
					{
						FString NewValue = NewText.ToString();
						if (NewValue == TEXT("(None)"))
						{
							NewValue = TEXT("");
						}

						if (*ValuePtr != NewValue)
						{
							FString DisplayOld = ValuePtr->IsEmpty() ? TEXT("(None)") : *ValuePtr;
							FString DisplayNew = NewValue.IsEmpty() ? TEXT("(None)") : NewValue;
							EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
								FText::Format(NSLOCTEXT("QuestTableEditor", "ConfirmNotesChange", "Change Notes from '{0}' to '{1}'?\n\nThis will mark the Quest as modified."),
									FText::FromString(DisplayOld),
									FText::FromString(DisplayNew)));

							if (Result == EAppReturnType::Yes)
							{
								*ValuePtr = NewValue;
								MarkModified();
							}
						}
					}
				})
				.SelectAllTextWhenFocused(true)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

void SQuestTableRow::MarkModified()
{
	if (RowDataEx.IsValid() && RowDataEx->Data.IsValid())
	{
		RowDataEx->Data->Status = EQuestTableRowStatus::Modified;
		RowDataEx->Data->InvalidateValidation();
	}
	OnRowModified.ExecuteIfBound();
}

//=============================================================================
// SQuestTableEditor
//=============================================================================

void SQuestTableEditor::Construct(const FArguments& InArgs)
{
	TableData = InArgs._TableData;
	OnDirtyStateChanged = InArgs._OnDirtyStateChanged;

	InitializeColumnFilters();
	SyncFromTableData();

	ChildSlot
	[
		SNew(SVerticalBox)

		// Toolbar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			BuildToolbar()
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
		]

		// Table content
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SAssignNew(ListView, SListView<TSharedPtr<FQuestTableRowEx>>)
				.ListItemsSource(&DisplayedRows)
				.OnGenerateRow(this, &SQuestTableEditor::OnGenerateRow)
				.OnSelectionChanged(this, &SQuestTableEditor::OnSelectionChanged)
				.SelectionMode(ESelectionMode::Multi)
				.HeaderRow(BuildHeaderRow())
			]
		]

		// Status bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			BuildStatusBar()
		]
	];

	ApplyFilters();
	UpdateStatusBar();
}

TSharedRef<SWidget> SQuestTableEditor::BuildToolbar()
{
	return SNew(SHorizontalBox)

		// LEFT SIDE - Data actions
		// Add Quest
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("AddQuest", "+ Quest"))
			.ToolTipText(LOCTEXT("AddQuestTooltip", "Add a new quest with Start state"))
			.OnClicked(this, &SQuestTableEditor::OnAddQuestClicked)
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
		]

		// Add State
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("AddState", "+ State"))
			.ToolTipText(LOCTEXT("AddStateTooltip", "Add a new state row"))
			.OnClicked(this, &SQuestTableEditor::OnAddRowClicked)
		]

		// Duplicate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Duplicate", "Duplicate"))
			.OnClicked(this, &SQuestTableEditor::OnDuplicateRowClicked)
		]

		// Delete
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Delete", "Delete"))
			.OnClicked(this, &SQuestTableEditor::OnDeleteRowsClicked)
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
		]

		// Vertical separator
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SNew(SSeparator)
			.Orientation(Orient_Vertical)
		]

		// Clear Filters button
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("ClearFilters", "Clear Filters"))
			.OnClicked(this, &SQuestTableEditor::OnClearFiltersClicked)
			.ToolTipText(LOCTEXT("ClearFiltersTip", "Reset all column filters"))
		]

		// SPACER - pushes right side buttons to the right
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNullWidget::NullWidget
		]

		// RIGHT SIDE - Generation/IO actions
		// Validate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Validate", "Validate"))
			.OnClicked(this, &SQuestTableEditor::OnValidateClicked)
			.ToolTipText(LOCTEXT("ValidateTip", "Validate all rows for errors and warnings"))
			.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Generate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Generate", "Generate Quests"))
			.OnClicked(this, &SQuestTableEditor::OnGenerateClicked)
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
			.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Sync from Assets
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Sync", "Sync from Assets"))
			.OnClicked(this, &SQuestTableEditor::OnSyncFromAssetsClicked)
			.ToolTipText(LOCTEXT("SyncTip", "Populate table from existing Quest assets"))
			.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Export XLSX
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("ExportXLSX", "Export XLSX"))
			.OnClicked(this, &SQuestTableEditor::OnExportXLSXClicked)
			.ToolTipText(LOCTEXT("ExportXLSXTooltip", "Export to Excel format (.xlsx)"))
			.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Import XLSX
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("ImportXLSX", "Import XLSX"))
			.OnClicked(this, &SQuestTableEditor::OnImportXLSXClicked)
			.ToolTipText(LOCTEXT("ImportXLSXTooltip", "Import from Excel format (.xlsx)"))
			.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Sync XLSX
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("SyncXLSX", "Sync XLSX"))
			.OnClicked(this, &SQuestTableEditor::OnSyncXLSXClicked)
			.ToolTipText(LOCTEXT("SyncXLSXTooltip", "Merge Excel changes with UE (3-way merge)"))
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
			.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Separator before Asset Actions
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SNew(SSeparator)
			.Orientation(Orient_Vertical)
		]

		// Save Table
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Save", "Save Table"))
			.OnClicked(this, &SQuestTableEditor::OnSaveClicked)
			.ToolTipText(LOCTEXT("SaveTooltip", "Save the table data container"))
		]

		// Apply to Assets
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("ApplyToAssets", "Apply to Quests"))
			.OnClicked(this, &SQuestTableEditor::OnApplyToAssetsClicked)
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
			.ToolTipText(LOCTEXT("ApplyToAssetsTooltip", "Write changes back to Quest assets"))
			.IsEnabled_Lambda([this]() { return !bIsBusy; })
		];
}

TSharedRef<SHeaderRow> SQuestTableEditor::BuildHeaderRow()
{
	TSharedRef<SHeaderRow> Header = SNew(SHeaderRow);
	TArray<FQuestTableColumn> Columns = GetQuestTableColumns();

	for (const FQuestTableColumn& Col : Columns)
	{
		Header->AddColumn(
			SHeaderRow::Column(Col.ColumnId)
			.DefaultLabel(Col.DisplayName)
			.ManualWidth(Col.ManualWidth)  // v4.12.5: Fixed pixel width
			.SortMode(this, &SQuestTableEditor::GetColumnSortMode, Col.ColumnId)
			.OnSort(this, &SQuestTableEditor::OnColumnSortModeChanged)
			.HeaderContent()
			[
				BuildColumnHeaderContent(Col)
			]
		);
	}

	HeaderRow = Header;
	return Header;
}

TSharedRef<SWidget> SQuestTableEditor::BuildColumnHeaderContent(const FQuestTableColumn& Col)
{
	FQuestColumnFilterState& FilterState = ColumnFilters.FindOrAdd(Col.ColumnId);

	return SNew(SVerticalBox)

		// 1. Column name
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(STextBlock)
			.Text(Col.DisplayName)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		]

		// 2. Text filter (live filtering - OnTextChanged)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f, 1.0f)
		[
			SNew(SEditableText)
			.HintText(LOCTEXT("FilterHint", "Filter..."))
			.OnTextChanged_Lambda([this, ColumnId = Col.ColumnId](const FText& NewText)
			{
				OnColumnTextFilterChanged(ColumnId, NewText);
			})
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		]

		// 3. Multi-select dropdown (SComboButton with checkbox menu)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f, 1.0f)
		[
			SNew(SComboButton)
			.OnGetMenuContent_Lambda([this, ColumnId = Col.ColumnId]() -> TSharedRef<SWidget>
			{
				FQuestColumnFilterState* State = ColumnFilters.Find(ColumnId);
				if (!State)
				{
					return SNullWidget::NullWidget;
				}

				TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox);

				// Header with All/None buttons
				MenuContent->AddSlot()
				.AutoHeight()
				.Padding(4.0f, 2.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("SelectAll", "All"))
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.OnClicked_Lambda([this, ColumnId]()
						{
							FQuestColumnFilterState* S = ColumnFilters.Find(ColumnId);
							if (S) S->SelectedValues.Empty();
							ApplyFilters();
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(4.0f, 0.0f)
					[
						SNew(SButton)
						.Text(LOCTEXT("ClearSel", "None"))
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.OnClicked_Lambda([this, ColumnId]()
						{
							FQuestColumnFilterState* S = ColumnFilters.Find(ColumnId);
							if (S)
							{
								// Mark all as deselected by putting empty string marker
								S->SelectedValues.Empty();
								S->SelectedValues.Add(TEXT("__NONE_SELECTED__"));
							}
							ApplyFilters();
							return FReply::Handled();
						})
					]
				];

				// Separator
				MenuContent->AddSlot()
				.AutoHeight()
				[
					SNew(SSeparator)
				];

				// Get unique values for this column
				TArray<FString> UniqueValues = GetUniqueColumnValues(ColumnId);

				// Checkboxes for each unique value in the column
				for (const FString& OptionValue : UniqueValues)
				{
					FString DisplayText = OptionValue;

					MenuContent->AddSlot()
					.AutoHeight()
					.Padding(4.0f, 1.0f)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this, ColumnId, OptionValue]()
						{
							FQuestColumnFilterState* S = ColumnFilters.Find(ColumnId);
							if (!S || S->SelectedValues.Num() == 0)
							{
								return ECheckBoxState::Checked;  // Empty = all selected
							}
							if (S->SelectedValues.Contains(TEXT("__NONE_SELECTED__")))
							{
								return ECheckBoxState::Unchecked;  // None selected marker
							}
							return S->SelectedValues.Contains(OptionValue)
								? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
						})
						.OnCheckStateChanged_Lambda([this, ColumnId, OptionValue](ECheckBoxState NewState)
						{
							OnColumnDropdownFilterChanged(ColumnId, OptionValue, NewState == ECheckBoxState::Checked);
						})
						[
							SNew(STextBlock)
							.Text(FText::FromString(DisplayText))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						]
					];
				}

				return SNew(SBox)
					.MaxDesiredHeight(300.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							MenuContent
						]
					];
			})
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text_Lambda([this, ColumnId = Col.ColumnId]()
				{
					FQuestColumnFilterState* State = ColumnFilters.Find(ColumnId);
					if (!State || State->SelectedValues.Num() == 0)
					{
						return LOCTEXT("AllFilter", "(All)");
					}
					if (State->SelectedValues.Contains(TEXT("__NONE_SELECTED__")))
					{
						return LOCTEXT("NoneFilter", "(None)");
					}
					if (State->SelectedValues.Num() == 1)
					{
						FString Val = *State->SelectedValues.CreateConstIterator();
						return FText::FromString(Val.IsEmpty() ? TEXT("(Empty)") : Val);
					}
					return FText::Format(LOCTEXT("MultiSelect", "({0} selected)"),
						FText::AsNumber(State->SelectedValues.Num()));
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			]
		];
}

TSharedRef<SWidget> SQuestTableEditor::BuildStatusBar()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(5, 3))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5, 0)
			[
				SAssignNew(StatusTotalText, STextBlock)
				.Text(LOCTEXT("StatusTotal", "Total: 0"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5, 0)
			[
				SAssignNew(StatusQuestsText, STextBlock)
				.Text(LOCTEXT("StatusQuests", "Quests: 0"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5, 0)
			[
				SAssignNew(StatusShowingText, STextBlock)
				.Text(LOCTEXT("StatusShowing", "Showing: 0"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5, 0)
			[
				SAssignNew(StatusSelectedText, STextBlock)
				.Text(LOCTEXT("StatusSelected", "Selected: 0"))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5, 0)
			[
				SAssignNew(StatusValidationText, STextBlock)
				.Text(LOCTEXT("StatusValidation", ""))
			]
		];
}

void SQuestTableEditor::UpdateStatusBar()
{
	if (!TableData) return;

	int32 TotalRows = 0;
	TSet<FString> UniqueQuests;
	int32 ValidationErrors = 0;

	for (const FQuestTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			TotalRows++;
			if (!Row.QuestName.IsEmpty())
			{
				UniqueQuests.Add(Row.QuestName);
			}
			if (Row.ValidationState == EValidationState::Invalid)
			{
				ValidationErrors++;
			}
		}
	}

	if (StatusTotalText.IsValid())
	{
		StatusTotalText->SetText(FText::Format(LOCTEXT("StatusTotalFmt", "Total: {0}"), FText::AsNumber(TotalRows)));
	}
	if (StatusQuestsText.IsValid())
	{
		StatusQuestsText->SetText(FText::Format(LOCTEXT("StatusQuestsFmt", "Quests: {0}"), FText::AsNumber(UniqueQuests.Num())));
	}
	if (StatusShowingText.IsValid())
	{
		StatusShowingText->SetText(FText::Format(LOCTEXT("StatusShowingFmt", "Showing: {0}"), FText::AsNumber(DisplayedRows.Num())));
	}
	if (StatusSelectedText.IsValid())
	{
		StatusSelectedText->SetText(FText::Format(LOCTEXT("StatusSelectedFmt", "Selected: {0}"), FText::AsNumber(GetSelectedRows().Num())));
	}
	if (StatusValidationText.IsValid())
	{
		if (ValidationErrors > 0)
		{
			StatusValidationText->SetText(FText::Format(LOCTEXT("StatusValidationFmt", "Errors: {0}"), FText::AsNumber(ValidationErrors)));
			StatusValidationText->SetColorAndOpacity(FLinearColor(1.0f, 0.2f, 0.2f));
		}
		else
		{
			StatusValidationText->SetText(LOCTEXT("StatusValidationOK", "Valid"));
			StatusValidationText->SetColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.2f));
		}
	}
}

void SQuestTableEditor::InitializeColumnFilters()
{
	TArray<FQuestTableColumn> Columns = GetQuestTableColumns();
	for (const FQuestTableColumn& Col : Columns)
	{
		ColumnFilters.Add(Col.ColumnId, FQuestColumnFilterState());
	}
}

void SQuestTableEditor::SyncFromTableData()
{
	AllRows.Empty();
	if (!TableData) return;

	// Group rows by quest for visual indicator
	TMap<FString, int32> QuestFirstRowIndex;
	int32 Index = 0;

	for (FQuestTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			TSharedPtr<FQuestTableRowEx> RowEx = MakeShared<FQuestTableRowEx>();
			RowEx->Data = MakeShared<FQuestTableRow>(Row);

			// Check if this is first row of quest
			if (!Row.QuestName.IsEmpty() && !QuestFirstRowIndex.Contains(Row.QuestName))
			{
				QuestFirstRowIndex.Add(Row.QuestName, Index);
				RowEx->bIsFirstInQuest = true;
			}

			AllRows.Add(RowEx);
			Index++;
		}
	}
}

void SQuestTableEditor::RefreshList()
{
	SyncFromTableData();
	ApplyFilters();
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
	UpdateStatusBar();
}

void SQuestTableEditor::SetTableData(UQuestTableData* InTableData)
{
	TableData = InTableData;
	RefreshList();
}

TArray<TSharedPtr<FQuestTableRowEx>> SQuestTableEditor::GetSelectedRows() const
{
	TArray<TSharedPtr<FQuestTableRowEx>> Selected;
	if (ListView.IsValid())
	{
		Selected = ListView->GetSelectedItems();
	}
	return Selected;
}

TSharedRef<ITableRow> SQuestTableEditor::OnGenerateRow(TSharedPtr<FQuestTableRowEx> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SQuestTableRow, OwnerTable)
		.RowData(Item)
		.OnRowModified(FSimpleDelegate::CreateSP(this, &SQuestTableEditor::OnRowModified));
}

void SQuestTableEditor::OnSelectionChanged(TSharedPtr<FQuestTableRowEx> Item, ESelectInfo::Type SelectInfo)
{
	UpdateStatusBar();
}

EColumnSortMode::Type SQuestTableEditor::GetColumnSortMode(FName ColumnId) const
{
	if (SortColumn == ColumnId)
	{
		return SortMode;
	}
	return EColumnSortMode::None;
}

void SQuestTableEditor::OnColumnSortModeChanged(EColumnSortPriority::Type Priority, const FName& ColumnId, EColumnSortMode::Type InSortMode)
{
	SortColumn = ColumnId;
	SortMode = InSortMode;
	ApplySorting();
}

void SQuestTableEditor::OnColumnTextFilterChanged(FName ColumnId, const FText& NewText)
{
	if (FQuestColumnFilterState* State = ColumnFilters.Find(ColumnId))
	{
		State->TextFilter = NewText.ToString();
		ApplyFilters();
	}
}

void SQuestTableEditor::OnColumnDropdownFilterChanged(FName ColumnId, const FString& Value, bool bIsSelected)
{
	FQuestColumnFilterState* State = ColumnFilters.Find(ColumnId);
	if (!State) return;

	// Handle special case: if currently showing all, initialize with all values then toggle
	if (State->SelectedValues.Num() == 0 || State->SelectedValues.Contains(TEXT("__NONE_SELECTED__")))
	{
		State->SelectedValues.Empty();
		if (bIsSelected)
		{
			// Just add this single value
			State->SelectedValues.Add(Value);
		}
		else
		{
			// Initialize with all values except this one
			TArray<FString> AllValues = GetUniqueColumnValues(ColumnId);
			for (const FString& V : AllValues)
			{
				if (V != Value)
				{
					State->SelectedValues.Add(V);
				}
			}
		}
	}
	else
	{
		// Toggle the specific value
		if (bIsSelected)
		{
			State->SelectedValues.Add(Value);
		}
		else
		{
			State->SelectedValues.Remove(Value);
		}
	}

	// If all values are selected, clear the filter (show all)
	TArray<FString> AllValues = GetUniqueColumnValues(ColumnId);
	if (State->SelectedValues.Num() == AllValues.Num())
	{
		State->SelectedValues.Empty();
	}

	ApplyFilters();
}

TArray<FString> SQuestTableEditor::GetUniqueColumnValues(FName ColumnId) const
{
	// Filter dropdowns show values from the current table data (AllRows)
	TSet<FString> UniqueSet;

	for (const TSharedPtr<FQuestTableRowEx>& Row : AllRows)
	{
		FString Value = GetColumnValue(Row, ColumnId);
		// Empty cells become "(None)" for display consistency
		if (Value.IsEmpty())
		{
			Value = TEXT("(None)");
		}
		UniqueSet.Add(Value);
	}

	TArray<FString> Result = UniqueSet.Array();
	Result.Sort();
	return Result;
}

FReply SQuestTableEditor::OnClearFiltersClicked()
{
	for (auto& Pair : ColumnFilters)
	{
		Pair.Value.TextFilter.Empty();
		Pair.Value.SelectedValues.Empty();
	}
	ApplyFilters();
	return FReply::Handled();
}

FString SQuestTableEditor::GetColumnValue(const TSharedPtr<FQuestTableRowEx>& Row, FName ColumnId) const
{
	if (!Row.IsValid() || !Row->Data.IsValid()) return TEXT("");

	const FQuestTableRow& Data = *Row->Data;

	if (ColumnId == TEXT("Status")) return Data.GetStatusString();
	if (ColumnId == TEXT("QuestName")) return Data.QuestName;
	if (ColumnId == TEXT("DisplayName")) return Data.DisplayName;
	if (ColumnId == TEXT("StateID")) return Data.StateID;
	if (ColumnId == TEXT("StateType")) return Data.GetStateTypeString();
	if (ColumnId == TEXT("Description")) return Data.Description;
	if (ColumnId == TEXT("ParentBranch")) return Data.ParentBranch;
	if (ColumnId == TEXT("Tasks")) return Data.Tasks;
	if (ColumnId == TEXT("Events")) return Data.Events;
	if (ColumnId == TEXT("Conditions")) return Data.Conditions;
	if (ColumnId == TEXT("Rewards")) return Data.Rewards;
	if (ColumnId == TEXT("Notes")) return Data.Notes;

	return TEXT("");
}

void SQuestTableEditor::ApplyFilters()
{
	DisplayedRows.Empty();

	for (const TSharedPtr<FQuestTableRowEx>& Row : AllRows)
	{
		bool bPassesFilter = true;

		for (const auto& Pair : ColumnFilters)
		{
			const FQuestColumnFilterState& State = Pair.Value;
			FString Value = GetColumnValue(Row, Pair.Key);

			// Check text filter
			if (!State.TextFilter.IsEmpty())
			{
				if (!Value.Contains(State.TextFilter))
				{
					bPassesFilter = false;
					break;
				}
			}

			// Check dropdown filter (SelectedValues)
			if (State.SelectedValues.Num() > 0 && !State.SelectedValues.Contains(TEXT("__NONE_SELECTED__")))
			{
				// Normalize empty values to "(None)" for consistency with GetUniqueColumnValues
				FString NormalizedValue = Value.IsEmpty() ? TEXT("(None)") : Value;
				if (!State.SelectedValues.Contains(NormalizedValue))
				{
					bPassesFilter = false;
					break;
				}
			}
		}

		if (bPassesFilter)
		{
			DisplayedRows.Add(Row);
		}
	}

	ApplySorting();

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
	UpdateStatusBar();
}

void SQuestTableEditor::ApplySorting()
{
	if (SortColumn.IsNone() || SortMode == EColumnSortMode::None)
	{
		ApplyQuestGrouping();
		return;
	}

	DisplayedRows.Sort([this](const TSharedPtr<FQuestTableRowEx>& A, const TSharedPtr<FQuestTableRowEx>& B) {
		FString ValueA = GetColumnValue(A, SortColumn);
		FString ValueB = GetColumnValue(B, SortColumn);

		if (SortMode == EColumnSortMode::Ascending)
		{
			return ValueA < ValueB;
		}
		return ValueA > ValueB;
	});
}

void SQuestTableEditor::ApplyQuestGrouping()
{
	// Sort by quest name, then by state (to group quest states together)
	DisplayedRows.Sort([](const TSharedPtr<FQuestTableRowEx>& A, const TSharedPtr<FQuestTableRowEx>& B) {
		if (!A->Data.IsValid() || !B->Data.IsValid()) return false;

		if (A->Data->QuestName != B->Data->QuestName)
		{
			return A->Data->QuestName < B->Data->QuestName;
		}

		// Put Start state first
		bool AIsStart = A->Data->StateID.Equals(TEXT("Start"), ESearchCase::IgnoreCase);
		bool BIsStart = B->Data->StateID.Equals(TEXT("Start"), ESearchCase::IgnoreCase);
		if (AIsStart != BIsStart) return AIsStart;

		return A->Data->StateID < B->Data->StateID;
	});
}

FReply SQuestTableEditor::OnAddRowClicked()
{
	if (!TableData) return FReply::Handled();

	FQuestTableRow& NewRow = TableData->AddRow();
	NewRow.QuestName = TEXT("NewQuest");
	NewRow.StateID = TEXT("NewState");

	MarkDirty();
	RefreshList();
	return FReply::Handled();
}

FReply SQuestTableEditor::OnAddQuestClicked()
{
	if (!TableData) return FReply::Handled();

	// Generate unique quest name
	int32 QuestNum = TableData->GetUniqueQuestNames().Num() + 1;
	FString QuestName = FString::Printf(TEXT("Quest%d"), QuestNum);

	// Add Start state
	FQuestTableRow& StartRow = TableData->AddRow();
	StartRow.QuestName = QuestName;
	StartRow.DisplayName = FString::Printf(TEXT("Quest %d"), QuestNum);
	StartRow.StateID = TEXT("Start");
	StartRow.StateType = EQuestStateType::Regular;
	StartRow.Description = TEXT("Quest started");

	// Add Complete state
	FQuestTableRow& CompleteRow = TableData->AddRow();
	CompleteRow.QuestName = QuestName;
	CompleteRow.StateID = TEXT("Complete");
	CompleteRow.StateType = EQuestStateType::Success;
	CompleteRow.Description = TEXT("Quest completed");
	CompleteRow.ParentBranch = TEXT("Start");

	MarkDirty();
	RefreshList();
	return FReply::Handled();
}

FReply SQuestTableEditor::OnDeleteRowsClicked()
{
	if (!TableData) return FReply::Handled();

	TArray<TSharedPtr<FQuestTableRowEx>> Selected = GetSelectedRows();
	if (Selected.Num() == 0) return FReply::Handled();

	// Soft delete
	for (const TSharedPtr<FQuestTableRowEx>& Row : Selected)
	{
		if (Row.IsValid() && Row->Data.IsValid())
		{
			// Find in TableData and mark deleted
			for (FQuestTableRow& TableRow : TableData->Rows)
			{
				if (TableRow.RowId == Row->Data->RowId)
				{
					TableRow.bDeleted = true;
					break;
				}
			}
		}
	}

	MarkDirty();
	RefreshList();
	return FReply::Handled();
}

FReply SQuestTableEditor::OnDuplicateRowClicked()
{
	if (!TableData) return FReply::Handled();

	TArray<TSharedPtr<FQuestTableRowEx>> Selected = GetSelectedRows();
	if (Selected.Num() == 0) return FReply::Handled();

	for (const TSharedPtr<FQuestTableRowEx>& Row : Selected)
	{
		if (Row.IsValid() && Row->Data.IsValid())
		{
			FQuestTableRow NewRow = *Row->Data;
			NewRow.RowId = FGuid::NewGuid();
			NewRow.StateID += TEXT("_Copy");
			NewRow.Status = EQuestTableRowStatus::New;
			NewRow.GeneratedQuest.Reset();
			NewRow.InvalidateValidation();

			TableData->Rows.Add(NewRow);
		}
	}

	MarkDirty();
	RefreshList();
	return FReply::Handled();
}

FReply SQuestTableEditor::OnValidateClicked()
{
	if (!TableData) return FReply::Handled();

	FQuestValidationResult Result = FQuestTableValidator::ValidateAllAndCache(TableData->Rows, TableData->ListsVersionGuid);

	RefreshList();

	// Show summary
	FString Summary = FString::Printf(TEXT("Validation complete: %d errors, %d warnings"),
		Result.GetErrorCount(), Result.GetWarningCount());

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Summary));

	return FReply::Handled();
}

FReply SQuestTableEditor::OnGenerateClicked()
{
	if (!TableData || bIsBusy) return FReply::Handled();

	bIsBusy = true;

	// Validate first
	FQuestValidationResult ValidationResult = FQuestTableValidator::ValidateAllAndCache(TableData->Rows, TableData->ListsVersionGuid);
	if (ValidationResult.HasErrors())
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("ValidationErrors", "Cannot generate: Please fix validation errors first."));
		bIsBusy = false;
		RefreshList();
		return FReply::Handled();
	}

	// Convert to manifest definitions
	TArray<FManifestQuestDefinition> Definitions = FQuestTableConverter::ConvertRowsToManifest(
		TableData->Rows, TableData->OutputFolder);

	int32 Generated = 0;
	int32 Failed = 0;

	for (const FManifestQuestDefinition& Def : Definitions)
	{
		FGenerationResult Result = FQuestGenerator::Generate(Def);
		if (Result.Status == EGenerationStatus::New)
		{
			Generated++;
		}
		else if (Result.Status == EGenerationStatus::Failed)
		{
			Failed++;
			UE_LOG(LogTemp, Warning, TEXT("Failed to generate quest %s: %s"), *Def.Name, *Result.Message);
		}
	}

	// Update generation tracking
	TableData->OnGenerationComplete(Failed);

	// Mark rows as synced
	for (FQuestTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			Row.Status = EQuestTableRowStatus::Synced;
			Row.LastSyncedHash = Row.ComputeSyncHash();
		}
	}

	MarkDirty();
	RefreshList();

	FString Summary = FString::Printf(TEXT("Generation complete: %d generated, %d failed"), Generated, Failed);
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Summary));

	bIsBusy = false;
	return FReply::Handled();
}

FReply SQuestTableEditor::OnSyncFromAssetsClicked()
{
	// v4.8.4: Re-entrancy guard
	if (bIsBusy) return FReply::Handled();
	TGuardValue<bool> BusyGuard(bIsBusy, true);

	if (!TableData)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoTableData", "No table data available. Please create or open a table first."));
		return FReply::Handled();
	}

	// Get Asset Registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Force Asset Registry rescan before querying
	TArray<FString> PathsToScan = { TEXT("/Game/") };
	UE_LOG(LogTemp, Log, TEXT("[QuestTableEditor] Rescanning Asset Registry for paths: %s"), *FString::Join(PathsToScan, TEXT(", ")));
	AssetRegistry.ScanPathsSynchronous(PathsToScan, true /* bForceRescan */);

	// Find all QuestBlueprint assets (QBP_* in Narrative Pro)
	TArray<FAssetData> AssetList;
	FTopLevelAssetPath ClassPath = UQuestBlueprint::StaticClass()->GetClassPathName();
	AssetRegistry.GetAssetsByClass(ClassPath, AssetList, true);
	UE_LOG(LogTemp, Log, TEXT("[QuestTableEditor] Found %d UQuestBlueprint assets"), AssetList.Num());

	if (AssetList.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoQuestsFound", "No Quest assets found in the project.\n\nCreate Quest assets (QBP_*) first, then sync."));
		return FReply::Handled();
	}

	// Preserve existing Notes values before clearing rows (user-entered, not stored in Quest asset)
	TMap<FString, FString> PreservedNotes;  // Key: "QuestName_StateID"
	for (const FQuestTableRow& ExistingRow : TableData->Rows)
	{
		if (!ExistingRow.Notes.IsEmpty())
		{
			FString Key = FString::Printf(TEXT("%s_%s"), *ExistingRow.QuestName, *ExistingRow.StateID);
			PreservedNotes.Add(Key, ExistingRow.Notes);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[QuestTableEditor] Preserved %d Notes before sync"), PreservedNotes.Num());

	// Clear existing rows and populate from assets
	TableData->Rows.Empty();
	int32 SyncedCount = 0;

	for (const FAssetData& AssetData : AssetList)
	{
		// v4.12.5: Use UQuestBlueprint and access QuestTemplate
		UQuestBlueprint* QuestBlueprint = Cast<UQuestBlueprint>(AssetData.GetAsset());
		if (!QuestBlueprint)
		{
			continue;
		}

		UQuest* Quest = QuestBlueprint->QuestTemplate;
		if (!Quest)
		{
			UE_LOG(LogTemp, Warning, TEXT("[QuestTableEditor] QuestBlueprint %s has no QuestTemplate"), *AssetData.AssetName.ToString());
			continue;
		}

		FString QuestName = AssetData.AssetName.ToString();
		FString DisplayName = Quest->GetQuestName().ToString();
		FString Description = Quest->GetQuestDescription().ToString();

		// Get states from the Quest via public accessor
		TArray<UQuestState*> States = Quest->GetStates();

		UE_LOG(LogTemp, Log, TEXT("[QuestTableEditor] Quest %s has %d states"), *QuestName, States.Num());

		// Create a row for each state
		for (UQuestState* State : States)
		{
			FQuestTableRow& Row = TableData->AddRow();

			// Core Identity
			Row.QuestName = QuestName;
			Row.DisplayName = DisplayName;

			// State Definition - access ID via reflection since it's protected
			if (FProperty* IDProp = UNarrativeNodeBase::StaticClass()->FindPropertyByName(TEXT("ID")))
			{
				FName* IDPtr = IDProp->ContainerPtrToValuePtr<FName>(State);
				if (IDPtr)
				{
					Row.StateID = IDPtr->ToString();
				}
			}
			Row.Description = State->Description.ToString();

			// State Type
			switch (State->StateNodeType)
			{
				case EStateNodeType::Success:
					Row.StateType = EQuestStateType::Success;
					break;
				case EStateNodeType::Failure:
					Row.StateType = EQuestStateType::Failure;
					break;
				default:
					Row.StateType = EQuestStateType::Regular;
					break;
			}

			// ParentBranch - find which branch leads to this state
			// Access Branches via reflection
			if (FArrayProperty* BranchesProperty = CastField<FArrayProperty>(UQuest::StaticClass()->FindPropertyByName(TEXT("Branches"))))
			{
				FScriptArrayHelper BranchHelper(BranchesProperty, BranchesProperty->ContainerPtrToValuePtr<void>(Quest));
				for (int32 b = 0; b < BranchHelper.Num(); ++b)
				{
					UQuestBranch* Branch = *reinterpret_cast<UQuestBranch**>(BranchHelper.GetRawPtr(b));
					if (Branch && Branch->DestinationState == State)
					{
						// Access Branch ID via reflection since it's protected
						if (FProperty* BranchIDProp = UNarrativeNodeBase::StaticClass()->FindPropertyByName(TEXT("ID")))
						{
							FName* BranchIDPtr = BranchIDProp->ContainerPtrToValuePtr<FName>(Branch);
							if (BranchIDPtr)
							{
								Row.ParentBranch = BranchIDPtr->ToString();
							}
						}
						break;
					}
				}
			}

			// Tasks, Events, Conditions, Rewards - from branches that START from this state
			// These would need more complex parsing from the branch tasks
			// For now, leave empty - they can be filled in manually or via XLSX import

			// Restore preserved Notes
			FString Key = FString::Printf(TEXT("%s_%s"), *Row.QuestName, *Row.StateID);
			if (FString* PreservedNote = PreservedNotes.Find(Key))
			{
				Row.Notes = *PreservedNote;
			}

			// Mark as synced
			Row.Status = EQuestTableRowStatus::Synced;
			Row.LastSyncedHash = Row.ComputeSyncHash();

			SyncedCount++;
		}
	}

	// Update table
	MarkDirty();
	SyncFromTableData();
	RefreshList();

	FString Summary = FString::Printf(TEXT("Synced %d states from %d Quest assets"), SyncedCount, AssetList.Num());
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Summary));

	return FReply::Handled();
}

FReply SQuestTableEditor::OnExportXLSXClicked()
{
	if (!TableData) return FReply::Handled();

	// Show save dialog
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return FReply::Handled();

	TArray<FString> OutFilenames;
	const bool bOpened = DesktopPlatform->SaveFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(AsShared()),
		TEXT("Export Quests to XLSX"),
		FPaths::ProjectDir(),
		TEXT("Quests.xlsx"),
		TEXT("Excel Files (*.xlsx)|*.xlsx"),
		EFileDialogFlags::None,
		OutFilenames
	);

	if (bOpened && OutFilenames.Num() > 0)
	{
		// Sync table data before export
		for (const TSharedPtr<FQuestTableRowEx>& RowEx : AllRows)
		{
			if (RowEx.IsValid() && RowEx->Data.IsValid())
			{
				int32 Index = TableData->FindRowIndexByGuid(RowEx->Data->RowId);
				if (Index != INDEX_NONE)
				{
					TableData->Rows[Index] = *RowEx->Data;
				}
			}
		}

		FString Error;
		if (FQuestXLSXWriter::ExportToXLSX(TableData->Rows, OutFilenames[0], Error))
		{
			// Update sync hashes after successful export
			for (FQuestTableRow& Row : TableData->Rows)
			{
				Row.LastSyncedHash = Row.ComputeSyncHash();
			}
			TableData->MarkPackageDirty();

			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
				LOCTEXT("ExportSuccess", "Exported {0} quest states to:\n{1}"),
				FText::AsNumber(TableData->Rows.Num()),
				FText::FromString(OutFilenames[0])
			));
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
				LOCTEXT("ExportFailed", "Export failed:\n{0}"),
				FText::FromString(Error)
			));
		}
	}

	return FReply::Handled();
}

FReply SQuestTableEditor::OnImportXLSXClicked()
{
	if (!TableData) return FReply::Handled();

	// Show open dialog
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return FReply::Handled();

	TArray<FString> OutFilenames;
	const bool bOpened = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(AsShared()),
		TEXT("Import Quests from XLSX"),
		FPaths::ProjectDir(),
		TEXT(""),
		TEXT("Excel Files (*.xlsx)|*.xlsx"),
		EFileDialogFlags::None,
		OutFilenames
	);

	if (bOpened && OutFilenames.Num() > 0)
	{
		// Validate the file
		if (!FQuestXLSXReader::IsValidQuestXLSX(OutFilenames[0]))
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("InvalidFile", "The selected file is not a valid Quest XLSX file.\nMake sure it was exported from the Quest Table Editor."));
			return FReply::Handled();
		}

		TArray<FQuestTableRow> ImportedRows;
		FString Error;
		if (FQuestXLSXReader::ImportFromXLSX(OutFilenames[0], ImportedRows, Error))
		{
			// Replace all rows with imported data
			TableData->Rows = ImportedRows;

			// Rebuild UI
			AllRows.Empty();
			for (FQuestTableRow& Row : TableData->Rows)
			{
				TSharedPtr<FQuestTableRowEx> RowEx = MakeShared<FQuestTableRowEx>();
				RowEx->Data = MakeShared<FQuestTableRow>(Row);
				AllRows.Add(RowEx);
			}
			DisplayedRows = AllRows;

			if (ListView.IsValid())
			{
				ListView->RequestListRefresh();
			}

			// Invalidate validation
			for (TSharedPtr<FQuestTableRowEx>& RowEx : AllRows)
			{
				if (RowEx.IsValid() && RowEx->Data.IsValid())
				{
					RowEx->Data->InvalidateValidation();
				}
			}

			TableData->MarkPackageDirty();
			UpdateStatusBar();
			OnDirtyStateChanged.ExecuteIfBound();

			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
				LOCTEXT("ImportSuccess", "Imported {0} quest states from:\n{1}"),
				FText::AsNumber(ImportedRows.Num()),
				FText::FromString(OutFilenames[0])
			));
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
				LOCTEXT("ImportFailed", "Import failed:\n{0}"),
				FText::FromString(Error)
			));
		}
	}

	return FReply::Handled();
}

FReply SQuestTableEditor::OnSyncXLSXClicked()
{
	if (!TableData || bIsBusy) return FReply::Handled();

	TGuardValue<bool> BusyGuard(bIsBusy, true);

	// Show open dialog to select XLSX file
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return FReply::Handled();

	TArray<FString> OutFilenames;
	const bool bOpened = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(AsShared()),
		TEXT("Select Quest XLSX for Sync"),
		FPaths::ProjectDir(),
		TEXT(""),
		TEXT("Excel Files (*.xlsx)|*.xlsx"),
		EFileDialogFlags::None,
		OutFilenames
	);

	if (!bOpened || OutFilenames.Num() == 0)
	{
		return FReply::Handled();
	}

	// Validate the file format
	if (!FQuestXLSXReader::IsValidQuestXLSX(OutFilenames[0]))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("InvalidSyncFile",
			"The selected file is not a valid Quest XLSX file.\n"
			"Make sure it was exported from the Quest Table Editor."));
		return FReply::Handled();
	}

	// Sync UI changes back to TableData first
	for (const TSharedPtr<FQuestTableRowEx>& RowEx : AllRows)
	{
		if (RowEx.IsValid() && RowEx->Data.IsValid())
		{
			int32 Index = TableData->FindRowIndexByGuid(RowEx->Data->RowId);
			if (Index != INDEX_NONE)
			{
				TableData->Rows[Index] = *RowEx->Data;
			}
		}
	}

	// Import rows from Excel
	TArray<FQuestTableRow> ExcelRows;
	FString ImportError;
	if (!FQuestXLSXReader::ImportFromXLSX(OutFilenames[0], ExcelRows, ImportError))
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("SyncImportFailed", "Failed to read Excel file:\n{0}"),
			FText::FromString(ImportError)));
		return FReply::Handled();
	}

	// Build base rows (reconstruct from LastSyncedHash - rows as they were at last export)
	// In a full implementation, we'd store base snapshots; here we use current rows with LastSyncedHash
	// as the "base" reference point
	TArray<FQuestTableRow> BaseRows;
	TArray<FQuestTableRow> UERows;

	for (const FQuestTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			UERows.Add(Row);
			// Base is conceptually the row at the time of last export
			// We reconstruct it by checking if the hash has changed
			if (Row.LastSyncedHash != 0)
			{
				BaseRows.Add(Row);  // Row existed at last export
			}
		}
	}

	// Perform 3-way comparison
	FQuestSyncResult SyncResult = FQuestXLSXSyncEngine::CompareSources(BaseRows, UERows, ExcelRows);

	if (!SyncResult.bSuccess)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("SyncCompareFailed", "Sync comparison failed:\n{0}"),
			FText::FromString(SyncResult.ErrorMessage)));
		return FReply::Handled();
	}

	// Auto-resolve unchanged entries (v4.11: only Unchanged auto-resolves)
	FQuestXLSXSyncEngine::AutoResolveNonConflicts(SyncResult);

	// Check if there are any changes
	if (!SyncResult.HasChanges())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoSyncChanges",
			"No changes detected between UE and Excel.\n"
			"Both sources are in sync."));
		return FReply::Handled();
	}

	// Show approval dialog
	if (!SQuestXLSXSyncDialog::ShowModal(SyncResult))
	{
		// User cancelled
		return FReply::Handled();
	}

	// Check all entries are resolved
	if (!FQuestXLSXSyncEngine::AllConflictsResolved(SyncResult))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("UnresolvedConflicts",
			"Some entries have not been resolved.\n"
			"Please resolve all conflicts before applying."));
		return FReply::Handled();
	}

	// Apply sync result
	FQuestMergeResult MergeResult = FQuestXLSXSyncEngine::ApplySync(SyncResult);

	// Replace TableData rows with merged result
	TableData->Rows.Empty();
	for (const FQuestTableRow& MergedRow : MergeResult.MergedRows)
	{
		FQuestTableRow& NewRow = TableData->Rows.Add_GetRef(MergedRow);
		NewRow.LastSyncedHash = NewRow.ComputeSyncHash();
		NewRow.Status = EQuestTableRowStatus::Synced;
	}

	MarkDirty();
	SyncFromTableData();
	RefreshList();

	// Show summary
	FString Summary = FString::Printf(
		TEXT("Sync complete!\n\n"
			"Applied from UE: %d\n"
			"Applied from Excel: %d\n"
			"Deleted: %d\n"
			"Unchanged: %d"),
		MergeResult.AppliedFromUE,
		MergeResult.AppliedFromExcel,
		MergeResult.Deleted,
		MergeResult.Unchanged);

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Summary));

	return FReply::Handled();
}

FReply SQuestTableEditor::OnSaveClicked()
{
	if (!TableData) return FReply::Handled();

	// Copy modified data back to TableData
	for (const TSharedPtr<FQuestTableRowEx>& RowEx : AllRows)
	{
		if (RowEx.IsValid() && RowEx->Data.IsValid())
		{
			int32 Index = TableData->FindRowIndexByGuid(RowEx->Data->RowId);
			if (Index != INDEX_NONE)
			{
				TableData->Rows[Index] = *RowEx->Data;
			}
		}
	}

	TableData->MarkPackageDirty();

	UPackage* Package = TableData->GetPackage();
	if (Package)
	{
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Standalone;
		FString PackageFileName = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		UPackage::SavePackage(Package, TableData, *PackageFileName, SaveArgs);
	}

	OnDirtyStateChanged.ExecuteIfBound();
	return FReply::Handled();
}

FReply SQuestTableEditor::OnApplyToAssetsClicked()
{
	if (!TableData || bIsBusy) return FReply::Handled();

	TGuardValue<bool> BusyGuard(bIsBusy, true);

	// Step 1: Sync UI changes back to TableData
	for (const TSharedPtr<FQuestTableRowEx>& RowEx : AllRows)
	{
		if (RowEx.IsValid() && RowEx->Data.IsValid())
		{
			int32 Index = TableData->FindRowIndexByGuid(RowEx->Data->RowId);
			if (Index != INDEX_NONE)
			{
				TableData->Rows[Index] = *RowEx->Data;
			}
		}
	}

	// Step 2: Gather rows to apply (non-deleted rows only)
	TArray<FQuestTableRow> RowsToApply;
	for (const FQuestTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			RowsToApply.Add(Row);
		}
	}

	if (RowsToApply.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoRowsToApply", "No quest rows to apply. Add quests first."));
		return FReply::Handled();
	}

	// Step 3: Confirm with user
	FText ConfirmMessage = FText::Format(
		LOCTEXT("ApplyConfirm", "This will regenerate Quest assets for {0} quest state(s).\n\n"
			"Modified rows will update existing assets.\n"
			"New rows will create new assets.\n\n"
			"Continue?"),
		FText::AsNumber(RowsToApply.Num()));

	EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, ConfirmMessage);
	if (Result != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	// Step 4: Apply to assets
	FString OutputFolder = TableData ? TableData->OutputFolder : TEXT("/Game/Quests");

	FQuestAssetApplySummary Summary = FQuestAssetSync::ApplyToAssets(
		RowsToApply,
		OutputFolder,
		true);  // bCreateMissing = true

	// Step 5: Update row statuses
	for (FQuestTableRow& AppliedRow : RowsToApply)
	{
		for (TSharedPtr<FQuestTableRowEx>& RowEx : AllRows)
		{
			if (RowEx.IsValid() && RowEx->Data.IsValid() && RowEx->Data->RowId == AppliedRow.RowId)
			{
				RowEx->Data->Status = AppliedRow.Status;
				RowEx->Data->GeneratedQuest = AppliedRow.GeneratedQuest;
				break;
			}
		}
	}

	RefreshList();
	UpdateStatusBar();

	// Step 6: Show result summary
	FMessageDialog::Open(EAppMsgType::Ok,
		FText::Format(LOCTEXT("ApplyComplete",
			"Apply complete!\n\n"
			"Quests processed: {0}\n"
			"Assets created: {1}\n"
			"Assets modified: {2}\n"
			"Skipped (unchanged): {3}\n"
			"Skipped (validation): {4}\n"
			"Skipped (no asset): {5}\n"
			"Failed: {6}"),
			FText::AsNumber(Summary.AssetsProcessed),
			FText::AsNumber(Summary.AssetsCreated),
			FText::AsNumber(Summary.AssetsModified),
			FText::AsNumber(Summary.AssetsSkippedNotModified),
			FText::AsNumber(Summary.AssetsSkippedValidation),
			FText::AsNumber(Summary.AssetsSkippedNoAsset),
			FText::AsNumber(Summary.FailedQuests.Num())));

	return FReply::Handled();
}

void SQuestTableEditor::MarkDirty()
{
	if (TableData)
	{
		TableData->MarkPackageDirty();
	}
	OnDirtyStateChanged.ExecuteIfBound();
}

void SQuestTableEditor::OnRowModified()
{
	MarkDirty();
	UpdateStatusBar();
}

//=============================================================================
// SQuestTableEditorWindow
//=============================================================================

void SQuestTableEditorWindow::Construct(const FArguments& InArgs)
{
	BaseTabLabel = LOCTEXT("QuestEditorTab", "Quest Table Editor");

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			BuildMenuBar()
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(TableEditor, SQuestTableEditor)
			.TableData(GetOrCreateTableData())
			.OnDirtyStateChanged(FOnQuestTableDirtyStateChanged::CreateSP(this, &SQuestTableEditorWindow::UpdateTabLabel))
		]
	];
}

void SQuestTableEditorWindow::SetParentTab(TSharedPtr<SDockTab> InTab)
{
	ParentTab = InTab;
	UpdateTabLabel();
}

TSharedRef<SWidget> SQuestTableEditorWindow::BuildMenuBar()
{
	FMenuBarBuilder MenuBuilder(nullptr);

	MenuBuilder.AddPullDownMenu(
		LOCTEXT("FileMenu", "File"),
		LOCTEXT("FileMenuTooltip", "File operations"),
		FNewMenuDelegate::CreateLambda([this](FMenuBuilder& Builder)
		{
			Builder.AddMenuEntry(
				LOCTEXT("NewTable", "New Table"),
				LOCTEXT("NewTableTooltip", "Create a new quest table"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQuestTableEditorWindow::OnNewTable))
			);
			Builder.AddMenuEntry(
				LOCTEXT("OpenTable", "Open Table..."),
				LOCTEXT("OpenTableTooltip", "Open an existing quest table"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQuestTableEditorWindow::OnOpenTable))
			);
			Builder.AddMenuSeparator();
			Builder.AddMenuEntry(
				LOCTEXT("SaveTable", "Save"),
				LOCTEXT("SaveTableTooltip", "Save the current table"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQuestTableEditorWindow::OnSaveTable))
			);
		})
	);

	return MenuBuilder.MakeWidget();
}

void SQuestTableEditorWindow::OnNewTable()
{
	CurrentTableData = GetOrCreateTableData();
	CurrentTableData->Rows.Empty();
	if (TableEditor.IsValid())
	{
		TableEditor->SetTableData(CurrentTableData.Get());
	}
}

void SQuestTableEditorWindow::OnOpenTable()
{
	// Use content browser asset picker
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FOpenAssetDialogConfig Config;
	Config.DialogTitleOverride = LOCTEXT("OpenQuestTableTitle", "Open Quest Table Data");
	Config.bAllowMultipleSelection = false;
	Config.AssetClassNames.Add(UQuestTableData::StaticClass()->GetClassPathName());

	TArray<FAssetData> SelectedAssets = ContentBrowserModule.Get().CreateModalOpenAssetDialog(Config);
	if (SelectedAssets.Num() > 0)
	{
		UQuestTableData* LoadedTable = Cast<UQuestTableData>(SelectedAssets[0].GetAsset());
		if (LoadedTable)
		{
			CurrentTableData = LoadedTable;
			if (TableEditor.IsValid())
			{
				TableEditor->SetTableData(LoadedTable);
			}
			UpdateTabLabel();
		}
	}
}

void SQuestTableEditorWindow::OnSaveTable()
{
	if (CurrentTableData.IsValid())
	{
		CurrentTableData->MarkPackageDirty();

		UPackage* Package = CurrentTableData->GetPackage();
		if (Package)
		{
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Standalone;
			FString PackageFileName = FPackageName::LongPackageNameToFilename(
				Package->GetName(), FPackageName::GetAssetPackageExtension());
			UPackage::SavePackage(Package, CurrentTableData.Get(), *PackageFileName, SaveArgs);
		}

		UpdateTabLabel();
	}
}

void SQuestTableEditorWindow::OnSaveTableAs()
{
	// Use content browser save asset dialog
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FSaveAssetDialogConfig Config;
	Config.DialogTitleOverride = LOCTEXT("SaveQuestTableAsTitle", "Save Quest Table As");
	Config.DefaultPath = TEXT("/Game/Tables");
	Config.DefaultAssetName = TEXT("QuestTableData");
	Config.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;

	FString SelectedPath = ContentBrowserModule.Get().CreateModalSaveAssetDialog(Config);
	if (!SelectedPath.IsEmpty())
	{
		// Create new package
		FString PackagePath = FPackageName::ObjectPathToPackageName(SelectedPath);
		FString AssetName = FPackageName::GetLongPackageAssetName(SelectedPath);

		UPackage* NewPackage = CreatePackage(*PackagePath);
		if (NewPackage)
		{
			// Create a new UQuestTableData object in the new package
			UQuestTableData* NewTableData = NewObject<UQuestTableData>(NewPackage, *AssetName, RF_Public | RF_Standalone);

			// Copy data from current table
			if (CurrentTableData.IsValid())
			{
				NewTableData->Rows = CurrentTableData->Rows;
			}

			// Register with asset registry
			FAssetRegistryModule::AssetCreated(NewTableData);

			// Save the package
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Standalone;
			FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
			UPackage::SavePackage(NewPackage, NewTableData, *PackageFileName, SaveArgs);

			// Switch to the new table
			CurrentTableData = NewTableData;
			if (TableEditor.IsValid())
			{
				TableEditor->SetTableData(NewTableData);
			}
			UpdateTabLabel();
		}
	}
}

UQuestTableData* SQuestTableEditorWindow::GetOrCreateTableData()
{
	if (CurrentTableData.IsValid())
	{
		return CurrentTableData.Get();
	}

	// Try to load default table
	FString DefaultPath = TEXT("/Game/Tables/QuestTableData");
	UQuestTableData* Data = LoadObject<UQuestTableData>(nullptr, *DefaultPath);

	if (!Data)
	{
		// Create new
		FString PackagePath = TEXT("/Game/Tables/QuestTableData");
		UPackage* Package = CreatePackage(*PackagePath);
		Data = NewObject<UQuestTableData>(Package, TEXT("QuestTableData"), RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(Data);
		Data->MarkPackageDirty();
	}

	CurrentTableData = Data;
	return Data;
}

void SQuestTableEditorWindow::UpdateTabLabel()
{
	if (TSharedPtr<SDockTab> Tab = ParentTab.Pin())
	{
		FText Label = BaseTabLabel;
		if (IsTableDirty())
		{
			Label = FText::Format(LOCTEXT("DirtyTabLabel", "{0}*"), Label);
		}
		Tab->SetLabel(Label);
	}
}

bool SQuestTableEditorWindow::IsTableDirty() const
{
	if (CurrentTableData.IsValid())
	{
		return CurrentTableData->GetPackage()->IsDirty();
	}
	return false;
}

bool SQuestTableEditorWindow::CanCloseTab() const
{
	if (IsTableDirty())
	{
		EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNoCancel,
			LOCTEXT("SaveBeforeClose", "Save changes before closing?"));

		if (Result == EAppReturnType::Yes)
		{
			const_cast<SQuestTableEditorWindow*>(this)->OnSaveTable();
			return true;
		}
		else if (Result == EAppReturnType::No)
		{
			return true;
		}
		return false;  // Cancel
	}
	return true;
}

#undef LOCTEXT_NAMESPACE
