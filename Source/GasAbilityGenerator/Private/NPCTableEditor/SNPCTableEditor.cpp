// SNPCTableEditor.cpp
// NPC Table Editor Implementation
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#include "NPCTableEditor/SNPCTableEditor.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Images/SImage.h"
#include "PropertyCustomizationHelpers.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "DesktopPlatformModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Misc/MessageDialog.h"
#include "UObject/SavePackage.h"
#include "AI/NPCDefinition.h"
#include "GameplayTagContainer.h"
#include "GAS/AbilityConfiguration.h"
#include "AI/Activities/NPCActivityConfiguration.h"
#include "Spawners/NPCSpawner.h"
#include "UnrealFramework/NarrativeNPCCharacter.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "NPCTableEditor"

//=============================================================================
// SNPCTableRow - Individual Row Widget
//=============================================================================

void SNPCTableRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	RowData = InArgs._RowData;
	OnRowModified = InArgs._OnRowModified;

	SMultiColumnTableRow<TSharedPtr<FNPCTableRow>>::Construct(
		FSuperRowType::FArguments()
			.Padding(FMargin(2.0f, 1.0f)),
		InOwnerTable
	);
}

TSharedRef<SWidget> SNPCTableRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (!RowData.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	// 1. Status - colored indicator
	if (ColumnName == TEXT("Status"))
	{
		return CreateStatusCell();
	}

	// 2. NPCName - asset name
	if (ColumnName == TEXT("NPCName"))
	{
		return CreateTextCell(RowData->NPCName, TEXT("NPC_Name"));
	}

	// 3. NPCId - unique identifier
	if (ColumnName == TEXT("NPCId"))
	{
		return CreateTextCell(RowData->NPCId, TEXT("npc_id"));
	}

	// 4. DisplayName - player-facing name
	if (ColumnName == TEXT("DisplayName"))
	{
		return CreateTextCell(RowData->DisplayName, TEXT("Display Name"));
	}

	// 5. Factions - use dedicated cell for short name <-> full tag conversion
	if (ColumnName == TEXT("Factions"))
	{
		return CreateFactionsCell();
	}

	// 6. OwnedTags - gameplay state tags (use similar pattern as Factions)
	if (ColumnName == TEXT("OwnedTags"))
	{
		return CreateOwnedTagsCell();
	}

	// 7. AbilityConfig - dropdown select
	if (ColumnName == TEXT("AbilityConfig"))
	{
		return CreateAssetDropdownCell(RowData->AbilityConfig, UAbilityConfiguration::StaticClass(), TEXT("AC_"));
	}

	// 7. ActivityConfig - dropdown select
	if (ColumnName == TEXT("ActivityConfig"))
	{
		return CreateAssetDropdownCell(RowData->ActivityConfig, UNPCActivityConfiguration::StaticClass(), TEXT("ActConfig_"));
	}

	// 9. NPCBlueprint - character blueprint dropdown
	if (ColumnName == TEXT("NPCBlueprint"))
	{
		return CreateAssetDropdownCell(RowData->NPCBlueprint, UBlueprint::StaticClass(), TEXT("BP_"));
	}

	// 10. bIsVendor - vendor checkbox
	if (ColumnName == TEXT("bIsVendor"))
	{
		return CreateCheckboxCell(RowData->bIsVendor);
	}

	// 11. MinLevel - combat balance
	if (ColumnName == TEXT("MinLevel"))
	{
		return CreateNumberCell(RowData->MinLevel);
	}

	// 12. MaxLevel - combat balance
	if (ColumnName == TEXT("MaxLevel"))
	{
		return CreateNumberCell(RowData->MaxLevel);
	}

	// 13. AttackPriority - targeting priority (float)
	if (ColumnName == TEXT("AttackPriority"))
	{
		return CreateFloatCell(RowData->AttackPriority);
	}

	// 14. DefaultCurrency - starting gold
	if (ColumnName == TEXT("DefaultCurrency"))
	{
		return CreateNumberCell(RowData->DefaultCurrency);
	}

	// 15. TradingCurrency - vendor gold
	if (ColumnName == TEXT("TradingCurrency"))
	{
		return CreateNumberCell(RowData->TradingCurrency);
	}

	// 16. ShopName - vendor shop name
	if (ColumnName == TEXT("ShopName"))
	{
		return CreateTextCell(RowData->ShopName, TEXT("Shop Name"));
	}

	// 17. DefaultItems - starting equipment
	if (ColumnName == TEXT("DefaultItems"))
	{
		return CreateTextCell(RowData->DefaultItems, TEXT("EI_Sword, IC_Armor..."));
	}

	// 18. Spawners - discovered spawners
	if (ColumnName == TEXT("Spawners"))
	{
		return CreateTextCell(RowData->DiscoveredSpawners, TEXT("Spawner1, Spawner2..."));
	}

	// 19. Notes - designer notes
	if (ColumnName == TEXT("Notes"))
	{
		return CreateTextCell(RowData->Notes, TEXT("Notes..."));
	}

	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SNPCTableRow::CreateStatusCell()
{
	FLinearColor StatusColor = RowData->GetStatusColor();

	// If read-only (plugin content), show as "Plugin" with grey color
	FString StatusText = RowData->Status;
	if (RowData->bIsReadOnly)
	{
		StatusText = TEXT("Plugin");
		StatusColor = FLinearColor(0.4f, 0.4f, 0.4f); // Grey
	}

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		.HAlign(HAlign_Center)
		[
			SNew(SBorder)
				.BorderBackgroundColor(StatusColor)
				.Padding(FMargin(6.0f, 2.0f))
				[
					SNew(STextBlock)
						.Text(FText::FromString(StatusText))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
				]
		];
}

TSharedRef<SWidget> SNPCTableRow::CreateTextCell(FString& Value, const FString& Hint)
{
	// Store pointer to the string field for safe lambda capture
	FString* ValuePtr = &Value;
	bool bReadOnly = RowData.IsValid() && RowData->bIsReadOnly;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableTextBox)
				.Text_Lambda([ValuePtr]() { return FText::FromString(*ValuePtr); })
				.HintText(FText::FromString(Hint))
				.IsReadOnly(bReadOnly)
				.OnTextCommitted_Lambda([this, ValuePtr, bReadOnly](const FText& NewText, ETextCommit::Type)
				{
					if (!bReadOnly)
					{
						*ValuePtr = NewText.ToString();
						MarkModified();
					}
				})
				.SelectAllTextWhenFocused(!bReadOnly)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ForegroundColor(bReadOnly ? FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)) : FSlateColor::UseForeground())
		];
}

TSharedRef<SWidget> SNPCTableRow::CreateCheckboxCell(bool& Value)
{
	bool* ValuePtr = &Value;
	bool bReadOnly = RowData.IsValid() && RowData->bIsReadOnly;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		.HAlign(HAlign_Center)
		[
			SNew(SCheckBox)
				.IsChecked_Lambda([ValuePtr]() { return *ValuePtr ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.IsEnabled(!bReadOnly)
				.OnCheckStateChanged_Lambda([this, ValuePtr, bReadOnly](ECheckBoxState NewState)
				{
					if (!bReadOnly)
					{
						*ValuePtr = (NewState == ECheckBoxState::Checked);
						MarkModified();
					}
				})
		];
}

TSharedRef<SWidget> SNPCTableRow::CreateNumberCell(int32& Value)
{
	int32* ValuePtr = &Value;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableTextBox)
				.Text_Lambda([ValuePtr]() { return FText::AsNumber(*ValuePtr); })
				.OnTextCommitted_Lambda([this, ValuePtr](const FText& NewText, ETextCommit::Type)
				{
					*ValuePtr = FCString::Atoi(*NewText.ToString());
					MarkModified();
				})
				.SelectAllTextWhenFocused(true)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.Justification(ETextJustify::Right)
		];
}

TSharedRef<SWidget> SNPCTableRow::CreateFloatCell(float& Value)
{
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableText)
				.Text_Lambda([&Value]() { return FText::AsNumber(Value); })
				.OnTextCommitted_Lambda([this, &Value](const FText& NewText, ETextCommit::Type)
				{
					Value = FCString::Atof(*NewText.ToString());
					MarkModified();
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.Justification(ETextJustify::Right)
		];
}

TSharedRef<SWidget> SNPCTableRow::CreateAssetPickerCell(FSoftObjectPath& Value, UClass* AllowedClass)
{
	// Display asset name as clickable text
	// TODO: Could be improved with SObjectPropertyEntryBox for proper picker
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableText)
				.Text_Lambda([&Value]()
				{
					if (Value.IsNull()) return FText::FromString(TEXT("(None)"));
					return FText::FromString(Value.GetAssetName());
				})
				.HintText(LOCTEXT("PickAsset", "Click to pick asset..."))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.7f, 1.0f)))
				.OnTextCommitted_Lambda([this, &Value](const FText& NewText, ETextCommit::Type)
				{
					// Allow manual path entry for now
					FString PathStr = NewText.ToString();
					if (PathStr.IsEmpty() || PathStr == TEXT("(None)"))
					{
						Value.Reset();
					}
					else
					{
						Value.SetPath(PathStr);
					}
					MarkModified();
				})
		];
}

TSharedRef<SWidget> SNPCTableRow::CreateFactionsCell()
{
	// Display short faction names (e.g., "Friendly, Town")
	// When editing, user can enter short names and they get converted to full tags
	bool bReadOnly = RowData.IsValid() && RowData->bIsReadOnly;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableTextBox)
				.Text_Lambda([this]() -> FText
				{
					// Show short names for display
					return FText::FromString(RowData->GetFactionsDisplay());
				})
				.HintText(LOCTEXT("FactionHint", "Friendly, Town"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.IsReadOnly(bReadOnly)
				.SelectAllTextWhenFocused(!bReadOnly)
				.ForegroundColor(bReadOnly ? FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)) : FSlateColor::UseForeground())
				.OnTextCommitted_Lambda([this, bReadOnly](const FText& NewText, ETextCommit::Type CommitType)
				{
					if (!bReadOnly && (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus))
					{
						// Convert short names to full tags when committed
						RowData->SetFactionsFromDisplay(NewText.ToString());
						MarkModified();
					}
				})
		];
}

TSharedRef<SWidget> SNPCTableRow::CreateOwnedTagsCell()
{
	// Display short tag names (e.g., "Invulnerable, Guard")
	// When editing, user can enter short names and they get converted to full tags
	bool bReadOnly = RowData.IsValid() && RowData->bIsReadOnly;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableTextBox)
				.Text_Lambda([this]() -> FText
				{
					// Show short names for display
					return FText::FromString(RowData->GetOwnedTagsDisplay());
				})
				.HintText(LOCTEXT("OwnedTagsHint", "Invulnerable, Guard"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.IsReadOnly(bReadOnly)
				.SelectAllTextWhenFocused(!bReadOnly)
				.ForegroundColor(bReadOnly ? FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)) : FSlateColor::UseForeground())
				.OnTextCommitted_Lambda([this, bReadOnly](const FText& NewText, ETextCommit::Type CommitType)
				{
					if (!bReadOnly && (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus))
					{
						// Convert short names to full tags when committed
						RowData->SetOwnedTagsFromDisplay(NewText.ToString());
						MarkModified();
					}
				})
		];
}

TSharedRef<SWidget> SNPCTableRow::CreateAssetDropdownCell(FSoftObjectPath& Value, UClass* AssetClass, const FString& AssetPrefix)
{
	FSoftObjectPath* ValuePtr = &Value;

	// Use a combo button that builds menu on-demand (avoids lifetime issues)
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SComboButton)
				.OnGetMenuContent_Lambda([this, ValuePtr, AssetClass]() -> TSharedRef<SWidget>
				{
					FMenuBuilder MenuBuilder(true, nullptr);

					// None option
					MenuBuilder.AddMenuEntry(
						NSLOCTEXT("NPCTableEditor", "None", "(None)"),
						FText::GetEmpty(),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateLambda([this, ValuePtr]()
						{
							ValuePtr->Reset();
							MarkModified();
						}))
					);

					MenuBuilder.AddSeparator();

					// Get available assets
					FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
					TArray<FAssetData> Assets;
					Registry.Get().GetAssetsByClass(AssetClass->GetClassPathName(), Assets, true);

					for (const FAssetData& Asset : Assets)
					{
						FString AssetName = Asset.AssetName.ToString();
						FSoftObjectPath AssetPath = Asset.GetSoftObjectPath();

						MenuBuilder.AddMenuEntry(
							FText::FromString(AssetName),
							FText::FromString(Asset.GetObjectPathString()),
							FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([this, ValuePtr, AssetPath]()
							{
								*ValuePtr = AssetPath;
								MarkModified();
							}))
						);
					}

					return MenuBuilder.MakeWidget();
				})
				.ButtonContent()
				[
					SNew(STextBlock)
						.Text_Lambda([ValuePtr]()
						{
							return FText::FromString(ValuePtr->IsNull() ? TEXT("(None)") : ValuePtr->GetAssetName());
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				]
		];
}

void SNPCTableRow::MarkModified()
{
	if (RowData.IsValid() && RowData->Status != TEXT("New"))
	{
		RowData->Status = TEXT("Modified");
	}
	OnRowModified.ExecuteIfBound();
}

//=============================================================================
// SNPCTableEditor - Main Table Widget
//=============================================================================

void SNPCTableEditor::Construct(const FArguments& InArgs)
{
	TableData = InArgs._TableData;
	SyncFromTableData();

	ChildSlot
	[
		SNew(SVerticalBox)

		// Toolbar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			BuildToolbar()
		]

		// Separator (filters are now integrated into header row)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
		]

		// Table - fills available space
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SBox)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SScrollBox)
					.Orientation(Orient_Horizontal)
					+ SScrollBox::Slot()
					.FillSize(1.0f)
					[
						SAssignNew(ListView, SListView<TSharedPtr<FNPCTableRow>>)
							.ListItemsSource(&DisplayedRows)
							.OnGenerateRow(this, &SNPCTableEditor::OnGenerateRow)
							.OnSelectionChanged(this, &SNPCTableEditor::OnSelectionChanged)
							.SelectionMode(ESelectionMode::Multi)
							.HeaderRow(BuildHeaderRow())
					]
			]
		]

		// Status bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			BuildStatusBar()
		]
	];

	ApplyFilters();
}

void SNPCTableEditor::SetTableData(UNPCTableData* InTableData)
{
	TableData = InTableData;
	SyncFromTableData();
}

TSharedRef<SWidget> SNPCTableEditor::BuildToolbar()
{
	return SNew(SHorizontalBox)

		// Add Row
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("AddRow", "+ Add NPC"))
				.OnClicked(this, &SNPCTableEditor::OnAddRowClicked)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
		]

		// Duplicate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("DuplicateRow", "Duplicate"))
				.OnClicked(this, &SNPCTableEditor::OnDuplicateRowClicked)
		]

		// Delete
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("DeleteRows", "Delete"))
				.OnClicked(this, &SNPCTableEditor::OnDeleteRowsClicked)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
		]

		// Spacer
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNullWidget::NullWidget
		]

		// Generate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("Generate", "Generate Assets"))
				.OnClicked(this, &SNPCTableEditor::OnGenerateClicked)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
		]

		// Sync
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("Sync", "Sync from Assets"))
				.OnClicked(this, &SNPCTableEditor::OnSyncFromAssetsClicked)
		]

		// Export CSV
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("ExportCSV", "Export CSV"))
				.OnClicked(this, &SNPCTableEditor::OnExportCSVClicked)
		]

		// Import CSV
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("ImportCSV", "Import CSV"))
				.OnClicked(this, &SNPCTableEditor::OnImportCSVClicked)
		]

		// Save Table button
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("SaveTable", "Save Table"))
				.OnClicked(this, &SNPCTableEditor::OnSaveClicked)
				.ToolTipText(LOCTEXT("SaveTableTooltip", "Save the table data container"))
		]

		// Apply to Assets button
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("ApplyToAssets", "Apply to NPCs"))
				.OnClicked(this, &SNPCTableEditor::OnApplyToAssetsClicked)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
				.ToolTipText(LOCTEXT("ApplyToAssetsTooltip", "Write changes back to NPCDefinition assets"))
		];
}

TSharedRef<SHeaderRow> SNPCTableEditor::BuildHeaderRow()
{
	TSharedRef<SHeaderRow> Header = SNew(SHeaderRow);

	TArray<FNPCTableColumn> Columns = GetNPCTableColumns();

	for (const FNPCTableColumn& Col : Columns)
	{
		// Create header content with column label + integrated filter
		TSharedRef<SWidget> HeaderContent = SNew(SVerticalBox)
			// Column label
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 2.0f)
			[
				SNew(STextBlock)
					.Text(Col.DisplayName)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			]
			// Filter row (integrated into header)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 2.0f, 0.0f, 4.0f)
			[
				SNew(SBorder)
					// Yellow background when filter is active
					.BorderBackgroundColor_Lambda([this, ColumnId = Col.ColumnId]() -> FSlateColor
					{
						bool bHasTextFilter = ColumnFilters.Contains(ColumnId);
						bool bHasSelectionFilter = ColumnSelectionFilters.Contains(ColumnId);
						if (bHasTextFilter || bHasSelectionFilter)
						{
							return FSlateColor(FLinearColor(0.8f, 0.6f, 0.1f, 0.3f)); // Yellow tint
						}
						return FSlateColor(FLinearColor::Transparent);
					})
					.Padding(2.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNew(SEditableTextBox)
								.HintText(LOCTEXT("FilterHintShort", "Filter..."))
								.OnTextChanged_Lambda([this, ColumnId = Col.ColumnId](const FText& NewText)
								{
									OnColumnFilterChanged(NewText, ColumnId);
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SComboButton)
								.HasDownArrow(true)
								.ContentPadding(FMargin(1.0f, 0.0f))
								.ButtonStyle(FAppStyle::Get(), "SimpleButton")
								.OnGetMenuContent_Lambda([this, ColumnId = Col.ColumnId]() -> TSharedRef<SWidget>
								{
									TArray<FString> UniqueValues = GetUniqueColumnValues(ColumnId);
									TSet<FString>* SelectionSet = ColumnSelectionFilters.Find(ColumnId);

							FMenuBuilder MenuBuilder(true, nullptr);
							MenuBuilder.AddMenuEntry(
								LOCTEXT("SelectAll", "Select All"),
								FText::GetEmpty(),
								FSlateIcon(),
								FUIAction(FExecuteAction::CreateLambda([this, ColumnId]()
								{
									ColumnSelectionFilters.Remove(ColumnId);
									ApplyFilters();
								}))
							);
							MenuBuilder.AddMenuEntry(
								LOCTEXT("ClearAll", "Clear All"),
								FText::GetEmpty(),
								FSlateIcon(),
								FUIAction(FExecuteAction::CreateLambda([this, ColumnId]()
								{
									ColumnSelectionFilters.Add(ColumnId, TSet<FString>());
									ApplyFilters();
								}))
							);
							MenuBuilder.AddSeparator();
							for (const FString& Value : UniqueValues)
							{
								MenuBuilder.AddMenuEntry(
									FText::FromString(Value.IsEmpty() ? TEXT("(Empty)") : Value),
									FText::GetEmpty(),
									FSlateIcon(),
									FUIAction(
										FExecuteAction::CreateLambda([this, ColumnId, Value]()
										{
											TSet<FString>& Set = ColumnSelectionFilters.FindOrAdd(ColumnId);
											if (Set.Num() == 0)
											{
												TArray<FString> AllValues = GetUniqueColumnValues(ColumnId);
												for (const FString& V : AllValues) Set.Add(V);
											}
											if (Set.Contains(Value)) Set.Remove(Value);
											else Set.Add(Value);
											TArray<FString> AllValues = GetUniqueColumnValues(ColumnId);
											if (Set.Num() == AllValues.Num()) ColumnSelectionFilters.Remove(ColumnId);
											ApplyFilters();
										}),
										FCanExecuteAction(),
										FIsActionChecked::CreateLambda([this, ColumnId, Value]()
										{
											TSet<FString>* Set = ColumnSelectionFilters.Find(ColumnId);
											return !Set || Set->Contains(Value);
										})
									),
									NAME_None,
									EUserInterfaceActionType::ToggleButton
								);
							}
							return MenuBuilder.MakeWidget();
						})
								.ButtonContent()
								[
									SNew(SImage)
										.Image(FAppStyle::GetBrush("Icons.Filter"))
										.DesiredSizeOverride(FVector2D(10, 10))
								]
						]
					]  // Close SBorder
				];

		Header->AddColumn(
			SHeaderRow::Column(Col.ColumnId)
				.DefaultTooltip(Col.DisplayName)
				.ManualWidth(Col.DefaultWidth)
				.SortMode(this, &SNPCTableEditor::GetColumnSortMode, Col.ColumnId)
				.OnSort(this, &SNPCTableEditor::OnColumnSortModeChanged)
				.HeaderContent()
				[
					HeaderContent
				]
		);
	}

	HeaderRow = Header;
	return Header;
}

TSharedRef<SWidget> SNPCTableEditor::BuildFilterRow()
{
	TSharedRef<SHorizontalBox> FilterRow = SNew(SHorizontalBox);

	TArray<FNPCTableColumn> Columns = GetNPCTableColumns();

	for (const FNPCTableColumn& Col : Columns)
	{
		FilterRow->AddSlot()
			.AutoWidth()
			.Padding(2.0f, 0.0f)
			[
				SNew(SBox)
				.WidthOverride(Col.DefaultWidth)  // Match column width
				[
					SNew(SHorizontalBox)

					// Text filter
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SEditableTextBox)
							.HintText(FText::Format(LOCTEXT("FilterHint", "Filter {0}..."), Col.DisplayName))
							.OnTextChanged_Lambda([this, ColumnId = Col.ColumnId](const FText& NewText)
							{
								OnColumnFilterChanged(NewText, ColumnId);
							})
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					]

					// Checkbox dropdown button
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SComboButton)
							.HasDownArrow(true)
							.ContentPadding(FMargin(2.0f, 0.0f))
							.ButtonStyle(FAppStyle::Get(), "SimpleButton")
							.OnGetMenuContent_Lambda([this, ColumnId = Col.ColumnId]() -> TSharedRef<SWidget>
						{
							TArray<FString> UniqueValues = GetUniqueColumnValues(ColumnId);
							TSet<FString>* SelectionSet = ColumnSelectionFilters.Find(ColumnId);

							FMenuBuilder MenuBuilder(true, nullptr);

							// Select All / Clear All
							MenuBuilder.AddMenuEntry(
								LOCTEXT("SelectAll", "Select All"),
								FText::GetEmpty(),
								FSlateIcon(),
								FUIAction(FExecuteAction::CreateLambda([this, ColumnId, UniqueValues]()
								{
									ColumnSelectionFilters.Remove(ColumnId);
									ApplyFilters();
								}))
							);

							MenuBuilder.AddMenuEntry(
								LOCTEXT("ClearAll", "Clear All"),
								FText::GetEmpty(),
								FSlateIcon(),
								FUIAction(FExecuteAction::CreateLambda([this, ColumnId]()
								{
									ColumnSelectionFilters.Add(ColumnId, TSet<FString>());
									ApplyFilters();
								}))
							);

							MenuBuilder.AddSeparator();

							// Checkbox for each unique value
							for (const FString& Value : UniqueValues)
							{
								bool bIsChecked = !SelectionSet || SelectionSet->Contains(Value);

								MenuBuilder.AddMenuEntry(
									FText::FromString(Value.IsEmpty() ? TEXT("(Empty)") : Value),
									FText::GetEmpty(),
									FSlateIcon(),
									FUIAction(
										FExecuteAction::CreateLambda([this, ColumnId, Value]()
										{
											TSet<FString>& Set = ColumnSelectionFilters.FindOrAdd(ColumnId);

											// If set is empty, initialize with all values
											if (Set.Num() == 0)
											{
												TArray<FString> AllValues = GetUniqueColumnValues(ColumnId);
												for (const FString& V : AllValues)
												{
													Set.Add(V);
												}
											}

											// Toggle the value
											if (Set.Contains(Value))
											{
												Set.Remove(Value);
											}
											else
											{
												Set.Add(Value);
											}

											// If all values selected, remove the filter
											TArray<FString> AllValues = GetUniqueColumnValues(ColumnId);
											if (Set.Num() == AllValues.Num())
											{
												ColumnSelectionFilters.Remove(ColumnId);
											}

											ApplyFilters();
										}),
										FCanExecuteAction(),
										FIsActionChecked::CreateLambda([this, ColumnId, Value]()
										{
											TSet<FString>* Set = ColumnSelectionFilters.Find(ColumnId);
											return !Set || Set->Contains(Value);
										})
									),
									NAME_None,
									EUserInterfaceActionType::ToggleButton
								);
							}

							return MenuBuilder.MakeWidget();
						})
						.ButtonContent()
						[
							SNew(SImage)
								.Image(FAppStyle::GetBrush("Icons.Filter"))
								.DesiredSizeOverride(FVector2D(12, 12))
						]
				]
			]  // Close SBox
		];
	}

	return FilterRow;
}

void SNPCTableEditor::OnColumnFilterChanged(const FText& NewText, FName ColumnId)
{
	FString FilterText = NewText.ToString();
	if (FilterText.IsEmpty())
	{
		ColumnFilters.Remove(ColumnId);
	}
	else
	{
		ColumnFilters.Add(ColumnId, FilterText);
	}
	ApplyFilters();
}

FString SNPCTableEditor::GetColumnValue(const TSharedPtr<FNPCTableRow>& Row, FName ColumnId) const
{
	if (!Row.IsValid()) return TEXT("");

	if (ColumnId == TEXT("Status")) return Row->bIsReadOnly ? TEXT("Plugin") : Row->Status;
	if (ColumnId == TEXT("NPCName")) return Row->NPCName;
	if (ColumnId == TEXT("NPCId")) return Row->NPCId;
	if (ColumnId == TEXT("DisplayName")) return Row->DisplayName;
	if (ColumnId == TEXT("Factions")) return Row->GetFactionsDisplay();
	if (ColumnId == TEXT("OwnedTags")) return Row->GetOwnedTagsDisplay();
	if (ColumnId == TEXT("AbilityConfig")) return Row->AbilityConfig.IsNull() ? TEXT("") : Row->AbilityConfig.GetAssetName();
	if (ColumnId == TEXT("ActivityConfig")) return Row->ActivityConfig.IsNull() ? TEXT("") : Row->ActivityConfig.GetAssetName();
	if (ColumnId == TEXT("NPCBlueprint")) return Row->NPCBlueprint.IsNull() ? TEXT("") : Row->NPCBlueprint.GetAssetName();
	if (ColumnId == TEXT("bIsVendor")) return Row->bIsVendor ? TEXT("Yes") : TEXT("No");
	if (ColumnId == TEXT("MinLevel")) return FString::FromInt(Row->MinLevel);
	if (ColumnId == TEXT("MaxLevel")) return FString::FromInt(Row->MaxLevel);
	if (ColumnId == TEXT("AttackPriority")) return FString::SanitizeFloat(Row->AttackPriority);
	if (ColumnId == TEXT("DefaultCurrency")) return FString::FromInt(Row->DefaultCurrency);
	if (ColumnId == TEXT("TradingCurrency")) return FString::FromInt(Row->TradingCurrency);
	if (ColumnId == TEXT("ShopName")) return Row->ShopName;
	if (ColumnId == TEXT("DefaultItems")) return Row->DefaultItems;
	if (ColumnId == TEXT("Spawners")) return Row->DiscoveredSpawners;
	if (ColumnId == TEXT("Notes")) return Row->Notes;

	return TEXT("");
}

TArray<FString> SNPCTableEditor::GetUniqueColumnValues(FName ColumnId) const
{
	TSet<FString> UniqueSet;

	for (const TSharedPtr<FNPCTableRow>& Row : AllRows)
	{
		FString Value = GetColumnValue(Row, ColumnId);
		UniqueSet.Add(Value);
	}

	TArray<FString> Result = UniqueSet.Array();
	Result.Sort();
	return Result;
}

TSharedRef<SWidget> SNPCTableEditor::BuildStatusBar()
{
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SNew(STextBlock)
				.Text_Lambda([this]()
				{
					return FText::Format(
						LOCTEXT("RowCount", "Total: {0} NPCs"),
						FText::AsNumber(AllRows.Num())
					);
				})
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f)
		[
			SNew(STextBlock)
				.Text_Lambda([this]()
				{
					return FText::Format(
						LOCTEXT("DisplayedCount", "Showing: {0}"),
						FText::AsNumber(DisplayedRows.Num())
					);
				})
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f)
		[
			SNew(STextBlock)
				.Text_Lambda([this]()
				{
					int32 Selected = ListView.IsValid() ? ListView->GetNumItemsSelected() : 0;
					return FText::Format(
						LOCTEXT("SelectedCount", "Selected: {0}"),
						FText::AsNumber(Selected)
					);
				})
		];
}

TSharedRef<ITableRow> SNPCTableEditor::OnGenerateRow(TSharedPtr<FNPCTableRow> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SNPCTableRow, OwnerTable)
		.RowData(Item)
		.OnRowModified(FSimpleDelegate::CreateSP(this, &SNPCTableEditor::OnRowModified));
}

void SNPCTableEditor::OnSelectionChanged(TSharedPtr<FNPCTableRow> Item, ESelectInfo::Type SelectInfo)
{
	// Could update a details panel here
}

EColumnSortMode::Type SNPCTableEditor::GetColumnSortMode(FName ColumnId) const
{
	if (SortColumn == ColumnId)
	{
		return SortMode;
	}
	return EColumnSortMode::None;
}

void SNPCTableEditor::OnColumnSortModeChanged(EColumnSortPriority::Type Priority, const FName& ColumnId, EColumnSortMode::Type NewSortMode)
{
	SortColumn = ColumnId;
	SortMode = NewSortMode;
	ApplySorting();
	RefreshList();
}

void SNPCTableEditor::RefreshList()
{
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

TArray<TSharedPtr<FNPCTableRow>> SNPCTableEditor::GetSelectedRows() const
{
	TArray<TSharedPtr<FNPCTableRow>> Selected;
	if (ListView.IsValid())
	{
		Selected = ListView->GetSelectedItems();
	}
	return Selected;
}

void SNPCTableEditor::SyncFromTableData()
{
	AllRows.Empty();
	if (TableData)
	{
		for (FNPCTableRow& Row : TableData->Rows)
		{
			AllRows.Add(MakeShared<FNPCTableRow>(Row));
		}
	}
	ApplyFilters();
}

void SNPCTableEditor::ApplyFilters()
{
	DisplayedRows.Empty();

	for (TSharedPtr<FNPCTableRow>& Row : AllRows)
	{
		bool bPassesFilters = true;

		// Global search filter
		if (!SearchText.IsEmpty())
		{
			FString SearchLower = SearchText.ToLower();
			bool bMatch =
				Row->NPCName.ToLower().Contains(SearchLower) ||
				Row->NPCId.ToLower().Contains(SearchLower) ||
				Row->DisplayName.ToLower().Contains(SearchLower) ||
				Row->Notes.ToLower().Contains(SearchLower);

			if (!bMatch) bPassesFilters = false;
		}

		// Per-column text filters
		if (bPassesFilters)
		{
			for (const auto& Filter : ColumnFilters)
			{
				FString FilterLower = Filter.Value.ToLower();
				FString ColumnValue = GetColumnValue(Row, Filter.Key);

				if (!ColumnValue.ToLower().Contains(FilterLower))
				{
					bPassesFilters = false;
					break;
				}
			}
		}

		// Per-column selection filters (checkbox)
		if (bPassesFilters)
		{
			for (const auto& SelectionFilter : ColumnSelectionFilters)
			{
				FString ColumnValue = GetColumnValue(Row, SelectionFilter.Key);

				// If the set exists and doesn't contain this value, filter it out
				if (!SelectionFilter.Value.Contains(ColumnValue))
				{
					bPassesFilters = false;
					break;
				}
			}
		}

		if (bPassesFilters)
		{
			DisplayedRows.Add(Row);
		}
	}

	ApplySorting();
	RefreshList();
}

void SNPCTableEditor::ApplySorting()
{
	if (SortColumn == NAME_None || SortMode == EColumnSortMode::None)
	{
		return;
	}

	bool bAscending = (SortMode == EColumnSortMode::Ascending);

	DisplayedRows.Sort([this, bAscending](const TSharedPtr<FNPCTableRow>& A, const TSharedPtr<FNPCTableRow>& B)
	{
		// String columns
		if (SortColumn == TEXT("NPCName") || SortColumn == TEXT("NPCId") ||
			SortColumn == TEXT("DisplayName") || SortColumn == TEXT("Status") ||
			SortColumn == TEXT("Factions") || SortColumn == TEXT("AbilityConfig") ||
			SortColumn == TEXT("ActivityConfig"))
		{
			FString ValueA, ValueB;
			if (SortColumn == TEXT("NPCName")) { ValueA = A->NPCName; ValueB = B->NPCName; }
			else if (SortColumn == TEXT("NPCId")) { ValueA = A->NPCId; ValueB = B->NPCId; }
			else if (SortColumn == TEXT("DisplayName")) { ValueA = A->DisplayName; ValueB = B->DisplayName; }
			else if (SortColumn == TEXT("Status")) { ValueA = A->Status; ValueB = B->Status; }
			else if (SortColumn == TEXT("Factions")) { ValueA = A->Factions; ValueB = B->Factions; }
			else if (SortColumn == TEXT("AbilityConfig")) { ValueA = A->AbilityConfig.GetAssetName(); ValueB = B->AbilityConfig.GetAssetName(); }
			else if (SortColumn == TEXT("ActivityConfig")) { ValueA = A->ActivityConfig.GetAssetName(); ValueB = B->ActivityConfig.GetAssetName(); }
			return bAscending ? (ValueA < ValueB) : (ValueA > ValueB);
		}

		// Integer columns
		if (SortColumn == TEXT("MinLevel") || SortColumn == TEXT("MaxLevel"))
		{
			int32 ValueA = 0, ValueB = 0;
			if (SortColumn == TEXT("MinLevel")) { ValueA = A->MinLevel; ValueB = B->MinLevel; }
			else if (SortColumn == TEXT("MaxLevel")) { ValueA = A->MaxLevel; ValueB = B->MaxLevel; }
			return bAscending ? (ValueA < ValueB) : (ValueA > ValueB);
		}

		// Boolean columns
		if (SortColumn == TEXT("bIsVendor"))
		{
			return bAscending ? (A->bIsVendor < B->bIsVendor) : (A->bIsVendor > B->bIsVendor);
		}

		return false;
	});
}

void SNPCTableEditor::OnSearchTextChanged(const FText& NewText)
{
	SearchText = NewText.ToString();
	ApplyFilters();
}

void SNPCTableEditor::OnRowModified()
{
	// Sync changes from AllRows back to TableData
	if (TableData)
	{
		for (const TSharedPtr<FNPCTableRow>& Row : AllRows)
		{
			if (Row.IsValid())
			{
				// Find matching row in TableData by GUID and update it
				FNPCTableRow* DataRow = TableData->FindRowByGuid(Row->RowId);
				if (DataRow)
				{
					*DataRow = *Row; // Copy all fields back
				}
			}
		}
	}
	MarkDirty();
}

void SNPCTableEditor::MarkDirty()
{
	if (TableData)
	{
		TableData->MarkPackageDirty();
	}
}

//=============================================================================
// Actions
//=============================================================================

FReply SNPCTableEditor::OnAddRowClicked()
{
	if (TableData)
	{
		FNPCTableRow& NewRow = TableData->AddRow();
		AllRows.Add(MakeShared<FNPCTableRow>(NewRow));
		ApplyFilters();
		MarkDirty();
	}
	return FReply::Handled();
}

FReply SNPCTableEditor::OnDeleteRowsClicked()
{
	TArray<TSharedPtr<FNPCTableRow>> Selected = GetSelectedRows();

	if (Selected.Num() == 0)
	{
		return FReply::Handled();
	}

	// Remove from AllRows and TableData
	for (const TSharedPtr<FNPCTableRow>& Row : Selected)
	{
		AllRows.RemoveAll([&Row](const TSharedPtr<FNPCTableRow>& Item)
		{
			return Item->RowId == Row->RowId;
		});

		if (TableData)
		{
			TableData->RemoveRowByGuid(Row->RowId);
		}
	}

	ApplyFilters();
	MarkDirty();
	return FReply::Handled();
}

FReply SNPCTableEditor::OnDuplicateRowClicked()
{
	TArray<TSharedPtr<FNPCTableRow>> Selected = GetSelectedRows();

	if (Selected.Num() == 0 || !TableData)
	{
		return FReply::Handled();
	}

	// Find index in TableData
	int32 SourceIndex = TableData->FindRowIndexByGuid(Selected[0]->RowId);
	if (SourceIndex != INDEX_NONE)
	{
		FNPCTableRow* NewRow = TableData->DuplicateRow(SourceIndex);
		if (NewRow)
		{
			AllRows.Add(MakeShared<FNPCTableRow>(*NewRow));
		}
	}

	ApplyFilters();
	MarkDirty();
	return FReply::Handled();
}

FReply SNPCTableEditor::OnGenerateClicked()
{
	// TODO: Implement actual asset generation using existing generators
	// For now, just mark valid rows as synced, invalid as error

	int32 GeneratedCount = 0;
	int32 ErrorCount = 0;

	for (TSharedPtr<FNPCTableRow>& Row : AllRows)
	{
		if (Row->IsValid())
		{
			Row->Status = TEXT("Synced");
			GeneratedCount++;
			// Would call generators here:
			// - FNPCDefinitionGenerator::Generate(Row)
			// - etc.
		}
		else
		{
			Row->Status = TEXT("Error");
			ErrorCount++;
		}
	}

	RefreshList();
	MarkDirty();

	// Show message
	FMessageDialog::Open(EAppMsgType::Ok,
		FText::Format(LOCTEXT("GenerateComplete", "Asset generation complete!\n\nGenerated: {0}\nErrors: {1}"),
			FText::AsNumber(GeneratedCount),
			FText::AsNumber(ErrorCount)));

	return FReply::Handled();
}

FReply SNPCTableEditor::OnSyncFromAssetsClicked()
{
	if (!TableData)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoTableData", "No table data available. Please create or open a table first."));
		return FReply::Handled();
	}

	// Get Asset Registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Find all NPCDefinition assets
	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByClass(UNPCDefinition::StaticClass()->GetClassPathName(), AssetList, true);

	if (AssetList.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoNPCsFound", "No NPCDefinition assets found in the project.\n\nCreate NPCDefinition assets (NPCDef_*) first, then sync."));
		return FReply::Handled();
	}

	// Build reverse lookup: NPCDefinition path -> Spawners that reference it
	// This scans all level assets for NPCSpawner actors
	TMap<FString, TArray<FString>> NPCToSpawnersMap;

	UE_LOG(LogTemp, Log, TEXT("[NPCTableEditor] Starting spawner discovery..."));

	// Get all referencers for each NPCDefinition
	for (const FAssetData& NPCAsset : AssetList)
	{
		TArray<FAssetIdentifier> Referencers;
		AssetRegistry.GetReferencers(NPCAsset.PackageName, Referencers);

		TArray<FString> SpawnerNames;
		for (const FAssetIdentifier& Ref : Referencers)
		{
			FString RefPath = Ref.PackageName.ToString();

			// Skip empty or invalid paths
			if (RefPath.IsEmpty() || !RefPath.StartsWith(TEXT("/")))
			{
				continue;
			}

			// Skip GUID-looking paths (contain long hex strings like "A1B2C3D4E5F6...")
			FString BaseName = FPaths::GetBaseFilename(RefPath);
			if (BaseName.Len() > 20 && !BaseName.Contains(TEXT("_")))
			{
				// Likely a GUID or hash, skip it
				continue;
			}

			// Check if this is a level or a spawner blueprint
			if (RefPath.Contains(TEXT("/Maps/")) || RefPath.Contains(TEXT("/Levels/")))
			{
				// It's a level - extract level name
				FString LevelName = FPaths::GetBaseFilename(RefPath);
				SpawnerNames.AddUnique(FString::Printf(TEXT("[%s]"), *LevelName));
			}
			else if (RefPath.Contains(TEXT("/Game/")))
			{
				// Only include assets from /Game/ folder
				FString AssetName = FPaths::GetBaseFilename(RefPath);
				if (AssetName.Contains(TEXT("Spawner")) || AssetName.StartsWith(TEXT("BP_")) || AssetName.StartsWith(TEXT("NPC")))
				{
					SpawnerNames.AddUnique(AssetName);
				}
			}
		}

		if (SpawnerNames.Num() > 0)
		{
			NPCToSpawnersMap.Add(NPCAsset.GetObjectPathString(), SpawnerNames);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[NPCTableEditor] Found spawner references for %d NPCs"), NPCToSpawnersMap.Num());

	// Clear existing rows and populate from assets
	TableData->Rows.Empty();
	int32 SyncedCount = 0;

	for (const FAssetData& AssetData : AssetList)
	{
		UNPCDefinition* NPCDef = Cast<UNPCDefinition>(AssetData.GetAsset());
		if (!NPCDef)
		{
			continue;
		}

		FNPCTableRow& Row = TableData->AddRow();

		// Identity
		Row.NPCName = AssetData.AssetName.ToString();
		Row.NPCId = NPCDef->NPCID.ToString();
		Row.DisplayName = NPCDef->NPCName.ToString();
		Row.bAllowMultipleInstances = NPCDef->bAllowMultipleInstances;

		// Combat
		Row.MinLevel = NPCDef->MinLevel;
		Row.MaxLevel = NPCDef->MaxLevel;
		Row.AttackPriority = NPCDef->AttackPriority;

		// Vendor
		Row.bIsVendor = NPCDef->bIsVendor;
		Row.ShopName = NPCDef->ShopFriendlyName.ToString();
		Row.TradingCurrency = NPCDef->TradingCurrency;
		Row.BuyItemPercentage = NPCDef->BuyItemPercentage;
		Row.SellItemPercentage = NPCDef->SellItemPercentage;

		// Assets - store paths
		if (!NPCDef->NPCClassPath.IsNull())
		{
			Row.NPCBlueprint = NPCDef->NPCClassPath.ToSoftObjectPath();
		}
		if (!NPCDef->ActivityConfiguration.IsNull())
		{
			Row.ActivityConfig = NPCDef->ActivityConfiguration.ToSoftObjectPath();
		}
		// AbilityConfiguration is on CharacterDefinition base class
		if (NPCDef->AbilityConfiguration)
		{
			Row.AbilityConfig = FSoftObjectPath(NPCDef->AbilityConfiguration);
		}

		// Factions - convert FGameplayTagContainer to comma-separated short names for display
		TArray<FString> FactionShortNames;
		for (const FGameplayTag& FactionTag : NPCDef->DefaultFactions)
		{
			FactionShortNames.Add(FNPCTableRow::ToShortFactionName(FactionTag.ToString()));
		}
		Row.Factions = FString::Join(FactionShortNames, TEXT(", "));

		// Owned Tags - convert FGameplayTagContainer to comma-separated short names for display
		TArray<FString> TagShortNames;
		for (const FGameplayTag& OwnedTag : NPCDef->DefaultOwnedTags)
		{
			TagShortNames.Add(FNPCTableRow::ToShortStateName(OwnedTag.ToString()));
		}
		Row.OwnedTags = FString::Join(TagShortNames, TEXT(", "));

		// Note: When generating, these short names will be converted back to full tags
		// e.g., "Bandits" -> "Narrative.Factions.Bandits"

		// Currency
		Row.DefaultCurrency = NPCDef->DefaultCurrency;

		// Discovered spawners
		TArray<FString>* Spawners = NPCToSpawnersMap.Find(AssetData.GetObjectPathString());
		if (Spawners && Spawners->Num() > 0)
		{
			Row.DiscoveredSpawners = FString::Join(*Spawners, TEXT(", "));
		}

		// Generated asset reference
		Row.GeneratedNPCDef = AssetData.GetSoftObjectPath();
		Row.Status = TEXT("Synced");

		// Check if this is plugin content (read-only)
		Row.bIsReadOnly = !AssetData.PackageName.ToString().StartsWith(TEXT("/Game/"));

		SyncedCount++;
	}

	TableData->LastSyncTime = FDateTime::Now();
	SyncFromTableData();
	MarkDirty();

	FMessageDialog::Open(EAppMsgType::Ok,
		FText::Format(LOCTEXT("SyncComplete", "Synced {0} NPCDefinition assets from project.\n\nYou can now edit them in the table and regenerate."),
			FText::AsNumber(SyncedCount)));

	return FReply::Handled();
}

FReply SNPCTableEditor::OnExportCSVClicked()
{
	// Build CSV content
	FString CSV;

	// Header row - matches the 10 columns displayed
	CSV += TEXT("NPCName,DisplayName,Factions,SpawnerPOI,LevelName,IsVendor,MinLevel,MaxLevel,DefaultItems,Notes\n");

	// Data rows
	for (const TSharedPtr<FNPCTableRow>& Row : AllRows)
	{
		// Escape fields that might contain commas
		auto EscapeCSV = [](const FString& Value) -> FString
		{
			if (Value.Contains(TEXT(",")) || Value.Contains(TEXT("\"")))
			{
				return FString::Printf(TEXT("\"%s\""), *Value.Replace(TEXT("\""), TEXT("\"\"")));
			}
			return Value;
		};

		CSV += FString::Printf(TEXT("%s,%s,%s,%s,%s,%s,%d,%d,%s,%s\n"),
			*EscapeCSV(Row->NPCName),
			*EscapeCSV(Row->DisplayName),
			*EscapeCSV(Row->GetFactionsDisplay()),
			*EscapeCSV(Row->SpawnerPOI),
			*EscapeCSV(Row->LevelName),
			Row->bIsVendor ? TEXT("TRUE") : TEXT("FALSE"),
			Row->MinLevel,
			Row->MaxLevel,
			*EscapeCSV(Row->DefaultItems),
			*EscapeCSV(Row->Notes)
		);
	}

	// Open file dialog
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		TArray<FString> OutFiles;
		if (DesktopPlatform->SaveFileDialog(
			nullptr,
			TEXT("Export NPC Table to CSV"),
			FPaths::ProjectDir(),
			TEXT("NPCTable.csv"),
			TEXT("CSV Files (*.csv)|*.csv"),
			0,
			OutFiles))
		{
			if (OutFiles.Num() > 0)
			{
				FFileHelper::SaveStringToFile(CSV, *OutFiles[0]);
				FMessageDialog::Open(EAppMsgType::Ok,
					FText::Format(LOCTEXT("ExportSuccess", "Exported {0} NPCs to:\n{1}"),
						FText::AsNumber(AllRows.Num()),
						FText::FromString(OutFiles[0])));
			}
		}
	}

	return FReply::Handled();
}

FReply SNPCTableEditor::OnImportCSVClicked()
{
	// Open file dialog
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform && TableData)
	{
		TArray<FString> OutFiles;
		if (DesktopPlatform->OpenFileDialog(
			nullptr,
			TEXT("Import NPC Table from CSV"),
			FPaths::ProjectDir(),
			TEXT(""),
			TEXT("CSV Files (*.csv)|*.csv"),
			0,
			OutFiles))
		{
			if (OutFiles.Num() > 0)
			{
				FString FileContent;
				if (FFileHelper::LoadFileToString(FileContent, *OutFiles[0]))
				{
					// Parse CSV (basic implementation)
					TArray<FString> Lines;
					FileContent.ParseIntoArrayLines(Lines);

					if (Lines.Num() > 1)
					{
						// Skip header row
						for (int32 i = 1; i < Lines.Num(); i++)
						{
							TArray<FString> Cells;
							Lines[i].ParseIntoArray(Cells, TEXT(","));

							if (Cells.Num() >= 10)
							{
								FNPCTableRow& NewRow = TableData->AddRow();
								NewRow.NPCName = Cells[0].TrimQuotes();
								NewRow.NPCId = Cells[0].TrimQuotes().ToLower().Replace(TEXT(" "), TEXT("_")); // Auto-generate from name
								NewRow.DisplayName = Cells[1].TrimQuotes();
								NewRow.SetFactionsFromDisplay(Cells[2].TrimQuotes());
								NewRow.SpawnerPOI = Cells[3].TrimQuotes();
								NewRow.LevelName = Cells[4].TrimQuotes();
								NewRow.bIsVendor = Cells[5].TrimQuotes().ToUpper() == TEXT("TRUE");
								NewRow.MinLevel = FCString::Atoi(*Cells[6].TrimQuotes());
								NewRow.MaxLevel = FCString::Atoi(*Cells[7].TrimQuotes());
								NewRow.DefaultItems = Cells[8].TrimQuotes();
								NewRow.Notes = Cells[9].TrimQuotes();
							}
						}

						SyncFromTableData();
						MarkDirty();

						FMessageDialog::Open(EAppMsgType::Ok,
							FText::Format(LOCTEXT("ImportSuccess", "Imported {0} NPCs from:\n{1}"),
								FText::AsNumber(Lines.Num() - 1),
								FText::FromString(OutFiles[0])));
					}
				}
			}
		}
	}

	return FReply::Handled();
}

FReply SNPCTableEditor::OnApplyToAssetsClicked()
{
	int32 UpdatedCount = 0;
	int32 SkippedCount = 0;
	int32 ReadOnlySkipped = 0;
	int32 NotModifiedSkipped = 0;
	TArray<FString> FailedAssets;
	TArray<FString> ReadOnlyAssets;

	for (const TSharedPtr<FNPCTableRow>& Row : AllRows)
	{
		if (!Row.IsValid() || Row->GeneratedNPCDef.IsNull())
		{
			SkippedCount++;
			continue;
		}

		// Only apply changes to rows that were actually modified
		if (Row->Status != TEXT("Modified") && Row->Status != TEXT("New"))
		{
			NotModifiedSkipped++;
			continue;
		}

		// Load the NPCDefinition asset
		UNPCDefinition* NPCDef = Cast<UNPCDefinition>(Row->GeneratedNPCDef.TryLoad());
		if (!NPCDef)
		{
			FailedAssets.Add(Row->NPCName);
			continue;
		}

		// Check if asset is in a writable location (only /Game/ is writable, not plugin content)
		UPackage* Package = NPCDef->GetOutermost();
		if (Package)
		{
			FString PackagePath = Package->GetName();
			// Skip assets in plugin folders (NarrativePro, etc.) - they're read-only
			if (!PackagePath.StartsWith(TEXT("/Game/")))
			{
				ReadOnlySkipped++;
				ReadOnlyAssets.Add(Row->NPCName);
				continue;
			}
		}

		// Update properties from row
		NPCDef->NPCID = FName(*Row->NPCId);
		NPCDef->NPCName = FText::FromString(Row->DisplayName);
		NPCDef->bAllowMultipleInstances = Row->bAllowMultipleInstances;
		NPCDef->MinLevel = Row->MinLevel;
		NPCDef->MaxLevel = Row->MaxLevel;
		NPCDef->AttackPriority = Row->AttackPriority;
		NPCDef->bIsVendor = Row->bIsVendor;
		NPCDef->ShopFriendlyName = FText::FromString(Row->ShopName);
		NPCDef->TradingCurrency = Row->TradingCurrency;
		NPCDef->BuyItemPercentage = Row->BuyItemPercentage;
		NPCDef->SellItemPercentage = Row->SellItemPercentage;
		NPCDef->DefaultCurrency = Row->DefaultCurrency;

		// Update Factions - convert from comma-separated short names to FGameplayTagContainer
		NPCDef->DefaultFactions.Reset();
		TArray<FString> FactionNames;
		Row->Factions.ParseIntoArray(FactionNames, TEXT(","));
		for (const FString& FactionName : FactionNames)
		{
			FString FullFactionTag = FNPCTableRow::ToFullFactionTag(FactionName);
			if (!FullFactionTag.IsEmpty())
			{
				FGameplayTag GameplayTag = FGameplayTag::RequestGameplayTag(FName(*FullFactionTag), false);
				if (GameplayTag.IsValid())
				{
					NPCDef->DefaultFactions.AddTag(GameplayTag);
				}
			}
		}

		// Update OwnedTags
		NPCDef->DefaultOwnedTags.Reset();
		TArray<FString> TagNames;
		Row->OwnedTags.ParseIntoArray(TagNames, TEXT(","));
		for (const FString& TagName : TagNames)
		{
			FString FullStateTag = FNPCTableRow::ToFullStateTag(TagName);
			if (!FullStateTag.IsEmpty())
			{
				FGameplayTag GameplayTag = FGameplayTag::RequestGameplayTag(FName(*FullStateTag), false);
				if (GameplayTag.IsValid())
				{
					NPCDef->DefaultOwnedTags.AddTag(GameplayTag);
				}
			}
		}

		// Update asset references
		if (!Row->ActivityConfig.IsNull())
		{
			NPCDef->ActivityConfiguration = TSoftObjectPtr<UNPCActivityConfiguration>(Row->ActivityConfig);
		}

		// Update NPCBlueprint (NPCClassPath)
		if (!Row->NPCBlueprint.IsNull())
		{
			NPCDef->NPCClassPath = TSoftClassPtr<ANarrativeNPCCharacter>(FSoftObjectPath(Row->NPCBlueprint));
		}

		// Update AbilityConfiguration
		if (!Row->AbilityConfig.IsNull())
		{
			NPCDef->AbilityConfiguration = Cast<UAbilityConfiguration>(Row->AbilityConfig.TryLoad());
		}

		// Mark package dirty and save (Package already in scope from read-only check above)
		if (Package)
		{
			Package->MarkPackageDirty();
			FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Standalone;

			if (UPackage::SavePackage(Package, NPCDef, *PackageFileName, SaveArgs))
			{
				Row->Status = TEXT("Synced");
				UpdatedCount++;
			}
			else
			{
				FailedAssets.Add(Row->NPCName);
			}
		}
	}

	RefreshList();

	FString Message = FString::Printf(TEXT("Applied changes to %d NPCDefinition assets."), UpdatedCount);
	if (NotModifiedSkipped > 0)
	{
		Message += FString::Printf(TEXT("\nUnchanged: %d"), NotModifiedSkipped);
	}
	if (SkippedCount > 0)
	{
		Message += FString::Printf(TEXT("\nNo asset reference: %d"), SkippedCount);
	}
	if (ReadOnlySkipped > 0)
	{
		Message += FString::Printf(TEXT("\nRead-only (plugin content): %d"), ReadOnlySkipped);
	}
	if (FailedAssets.Num() > 0)
	{
		Message += FString::Printf(TEXT("\nFailed to save: %s"), *FString::Join(FailedAssets, TEXT(", ")));
	}

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));

	return FReply::Handled();
}

FReply SNPCTableEditor::OnSaveClicked()
{
	if (TableData)
	{
		// Make sure all changes are synced
		for (const TSharedPtr<FNPCTableRow>& Row : AllRows)
		{
			if (Row.IsValid())
			{
				FNPCTableRow* DataRow = TableData->FindRowByGuid(Row->RowId);
				if (DataRow)
				{
					*DataRow = *Row;
				}
			}
		}

		// Check if this is a transient object (needs Save As)
		UPackage* Package = TableData->GetOutermost();
		bool bIsTransient = !Package || Package == GetTransientPackage() || Package->GetName().StartsWith(TEXT("/Engine/Transient"));

		if (bIsTransient)
		{
			// Create a new package for this asset
			FString PackagePath = TEXT("/Game/NPCs/NPCTableData");
			FString PackageName = FPackageName::ObjectPathToPackageName(PackagePath);

			// Create the package
			UPackage* NewPackage = CreatePackage(*PackageName);
			NewPackage->FullyLoad();

			// Create a new object in the package
			UNPCTableData* NewTableData = NewObject<UNPCTableData>(NewPackage, UNPCTableData::StaticClass(), TEXT("NPCTableData"), RF_Public | RF_Standalone);

			// Copy data
			NewTableData->Rows = TableData->Rows;
			NewTableData->ProjectName = TableData->ProjectName;
			NewTableData->OutputFolder = TableData->OutputFolder;
			NewTableData->LastSyncTime = TableData->LastSyncTime;

			// Mark for save
			NewPackage->MarkPackageDirty();
			FAssetRegistryModule::AssetCreated(NewTableData);

			// Save the package
			FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Standalone;

			if (UPackage::SavePackage(NewPackage, NewTableData, *PackageFileName, SaveArgs))
			{
				// Update our reference to the new persistent object
				TableData = NewTableData;

				FMessageDialog::Open(EAppMsgType::Ok,
					FText::Format(LOCTEXT("SaveSuccess", "Saved {0} NPCs to:\n{1}"),
						FText::AsNumber(TableData->Rows.Num()),
						FText::FromString(PackageFileName)));
			}
			else
			{
				FMessageDialog::Open(EAppMsgType::Ok,
					LOCTEXT("SaveFailed", "Failed to save table data."));
			}
		}
		else
		{
			// Save existing package
			FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Standalone;

			if (UPackage::SavePackage(Package, TableData, *PackageFileName, SaveArgs))
			{
				FMessageDialog::Open(EAppMsgType::Ok,
					FText::Format(LOCTEXT("SaveSuccess", "Saved {0} NPCs to:\n{1}"),
						FText::AsNumber(TableData->Rows.Num()),
						FText::FromString(PackageFileName)));
			}
			else
			{
				FMessageDialog::Open(EAppMsgType::Ok,
					LOCTEXT("SaveFailed", "Failed to save table data. Make sure the file is not read-only."));
			}
		}
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoTableToSave", "No table data to save. Create or open a table first."));
	}
	return FReply::Handled();
}

//=============================================================================
// SNPCTableEditorWindow - Host Window
//=============================================================================

void SNPCTableEditorWindow::Construct(const FArguments& InArgs)
{
	CurrentTableData = GetOrCreateTableData();

	ChildSlot
	[
		SNew(SVerticalBox)

		// Menu bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			BuildMenuBar()
		]

		// Table editor
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(TableEditor, SNPCTableEditor)
				.TableData(CurrentTableData.Get())
		]
	];
}

TSharedRef<SWidget> SNPCTableEditorWindow::BuildMenuBar()
{
	FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(nullptr);

	// File menu
	MenuBarBuilder.AddPullDownMenu(
		LOCTEXT("FileMenu", "File"),
		LOCTEXT("FileMenuTooltip", "File operations"),
		FNewMenuDelegate::CreateLambda([this](FMenuBuilder& MenuBuilder)
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("NewTable", "New Table"),
				LOCTEXT("NewTableTooltip", "Create a new empty table"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SNPCTableEditorWindow::OnNewTable))
			);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("OpenTable", "Open Table..."),
				LOCTEXT("OpenTableTooltip", "Open an existing table data asset"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SNPCTableEditorWindow::OnOpenTable))
			);

			MenuBuilder.AddMenuSeparator();

			MenuBuilder.AddMenuEntry(
				LOCTEXT("SaveTable", "Save"),
				LOCTEXT("SaveTableTooltip", "Save the current table"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SNPCTableEditorWindow::OnSaveTable))
			);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("SaveTableAs", "Save As..."),
				LOCTEXT("SaveTableAsTooltip", "Save the table to a new location"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SNPCTableEditorWindow::OnSaveTableAs))
			);
		})
	);

	return MenuBarBuilder.MakeWidget();
}

void SNPCTableEditorWindow::OnNewTable()
{
	CurrentTableData = GetOrCreateTableData();
	if (CurrentTableData.IsValid())
	{
		CurrentTableData->Rows.Empty();
		if (TableEditor.IsValid())
		{
			TableEditor->SetTableData(CurrentTableData.Get());
		}
	}
}

void SNPCTableEditorWindow::OnOpenTable()
{
	// Use content browser asset picker
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FOpenAssetDialogConfig Config;
	Config.DialogTitleOverride = LOCTEXT("OpenTableTitle", "Open NPC Table Data");
	Config.bAllowMultipleSelection = false;
	Config.AssetClassNames.Add(UNPCTableData::StaticClass()->GetClassPathName());

	TArray<FAssetData> SelectedAssets = ContentBrowserModule.Get().CreateModalOpenAssetDialog(Config);
	if (SelectedAssets.Num() > 0)
	{
		UNPCTableData* LoadedTable = Cast<UNPCTableData>(SelectedAssets[0].GetAsset());
		if (LoadedTable)
		{
			CurrentTableData = LoadedTable;
			if (TableEditor.IsValid())
			{
				TableEditor->SetTableData(LoadedTable);
			}
		}
	}
}

void SNPCTableEditorWindow::OnSaveTable()
{
	if (CurrentTableData.IsValid())
	{
		UPackage* Package = CurrentTableData->GetOutermost();
		if (Package)
		{
			FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Standalone;
			UPackage::SavePackage(Package, CurrentTableData.Get(), *PackageFileName, SaveArgs);
		}
	}
}

void SNPCTableEditorWindow::OnSaveTableAs()
{
	// Would implement save as dialog
	OnSaveTable();
}

UNPCTableData* SNPCTableEditorWindow::GetOrCreateTableData()
{
	// Try to load default table data
	FString DefaultPath = TEXT("/Game/NPCs/NPCTableData");
	UNPCTableData* TableData = LoadObject<UNPCTableData>(nullptr, *DefaultPath);

	if (!TableData)
	{
		// Create new transient table data
		TableData = NewObject<UNPCTableData>();
	}

	return TableData;
}

#undef LOCTEXT_NAMESPACE
