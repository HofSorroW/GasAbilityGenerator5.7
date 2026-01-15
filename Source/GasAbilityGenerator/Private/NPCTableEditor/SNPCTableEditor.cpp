// SNPCTableEditor.cpp
// NPC Table Editor Implementation
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#include "NPCTableEditor/SNPCTableEditor.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SSpinBox.h"
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
#include "GameplayTagsManager.h"
#include "EngineUtils.h"  // For TActorIterator
#include "GAS/AbilityConfiguration.h"
#include "AI/Activities/NPCActivityConfiguration.h"
#include "AI/Activities/NPCActivitySchedule.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Spawners/NPCSpawner.h"
#include "Spawners/NPCSpawnComponent.h"
#include "Items/InventoryComponent.h"  // Contains UItemCollection
#include "Character/CharacterAppearance.h"  // Contains UCharacterAppearanceBase
#include "Navigation/POIActor.h"  // APOIActor for POI scanning
#include "Tales/TriggerSet.h"  // UTriggerSet for schedule/triggers
#include "UnrealFramework/NarrativeNPCCharacter.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "UObject/UObjectIterator.h"
#include "Engine/BlueprintCore.h"  // FBlueprintTags for parent class filtering
#include "XLSXSupport/NPCXLSXWriter.h"
#include "XLSXSupport/NPCXLSXReader.h"
#include "XLSXSupport/NPCXLSXSyncEngine.h"
#include "XLSXSupport/SNPCXLSXSyncDialog.h"
#include "NPCTableEditor/NPCTableValidator.h"
#include "NPCTableEditor/NPCTableConverter.h"
#include "NPCTableEditor/NPCAssetSync.h"
#include "GasAbilityGeneratorGenerators.h"

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

	//=========================================================================
	// Core Identity (5 columns)
	//=========================================================================

	// 1. Status - read-only colored badge
	if (ColumnName == TEXT("Status"))
	{
		return CreateStatusCell();
	}

	// 2. NPCName - text input (required)
	if (ColumnName == TEXT("NPCName"))
	{
		return CreateTextCell(RowData->NPCName, TEXT("NPC_Name"));
	}

	// 3. NPCId - text input (required)
	if (ColumnName == TEXT("NPCId"))
	{
		return CreateTextCell(RowData->NPCId, TEXT("npc_id"));
	}

	// 4. DisplayName - text input
	if (ColumnName == TEXT("DisplayName"))
	{
		return CreateTextCell(RowData->DisplayName, TEXT("Display Name"));
	}

	// 5. Blueprint - asset dropdown (only NarrativeNPCCharacter subclasses)
	if (ColumnName == TEXT("Blueprint"))
	{
		return CreateNPCBlueprintDropdownCell();
	}

	//=========================================================================
	// AI & Behavior (4 columns) - Asset dropdowns
	//=========================================================================

	// 6. AbilityConfig - asset dropdown (AC_*)
	if (ColumnName == TEXT("AbilityConfig"))
	{
		return CreateAssetDropdownCell(RowData->AbilityConfig, UAbilityConfiguration::StaticClass(), TEXT("AC_"));
	}

	// 7. ActivityConfig - asset dropdown (ActConfig_*)
	if (ColumnName == TEXT("ActivityConfig"))
	{
		return CreateAssetDropdownCell(RowData->ActivityConfig, UNPCActivityConfiguration::StaticClass(), TEXT("ActConfig_"));
	}

	// 8. Schedule - asset dropdown (TriggerSets - TS_* or Schedule_*)
	if (ColumnName == TEXT("Schedule"))
	{
		return CreateAssetDropdownCell(RowData->Schedule, UTriggerSet::StaticClass(), TEXT(""));
	}

	//=========================================================================
	// Combat (3 columns)
	//=========================================================================

	// 9. LevelRange - dual spinbox "1-10"
	if (ColumnName == TEXT("LevelRange"))
	{
		return CreateLevelRangeCell();
	}

	// 11. Factions - multi-select dropdown
	if (ColumnName == TEXT("Factions"))
	{
		return CreateFactionsCell();
	}

	// 12. AttackPriority - float slider 0.0-1.0
	if (ColumnName == TEXT("AttackPriority"))
	{
		return CreateFloatSliderCell(RowData->AttackPriority);
	}

	//=========================================================================
	// Vendor (2 columns)
	//=========================================================================

	// 13. bIsVendor - checkbox
	if (ColumnName == TEXT("bIsVendor"))
	{
		return CreateCheckboxCell(RowData->bIsVendor);
	}

	// 14. ShopName - text input
	if (ColumnName == TEXT("ShopName"))
	{
		return CreateTextCell(RowData->ShopName, TEXT("Shop Name"));
	}

	//=========================================================================
	// Items & Spawning (2 columns)
	//=========================================================================

	// 15. DefaultItems - multi-select dropdown (IC_*)
	if (ColumnName == TEXT("DefaultItems"))
	{
		return CreateItemsCell();
	}

	// 16. SpawnerPOI - dropdown (POI tags)
	if (ColumnName == TEXT("SpawnerPOI"))
	{
		return CreatePOIDropdownCell();
	}

	//=========================================================================
	// Meta (2 columns)
	//=========================================================================

	// 17. Appearance - asset dropdown
	if (ColumnName == TEXT("Appearance"))
	{
		return CreateAssetDropdownCell(RowData->Appearance, UCharacterAppearanceBase::StaticClass(), TEXT("Appearance_"));
	}

	// 18. Notes - text input with tooltip preview for long text
	if (ColumnName == TEXT("Notes"))
	{
		// Store pointer to the string field for safe lambda capture
		FString* ValuePtr = &RowData->Notes;

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
					.Text_Lambda([ValuePtr]() { return FText::FromString(*ValuePtr); })
					.HintText(LOCTEXT("NotesHint", "Notes..."))
					.OnTextCommitted_Lambda([this, ValuePtr](const FText& NewText, ETextCommit::Type CommitType)
					{
						if (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus)
						{
							FString NewValue = NewText.ToString();
							if (NewValue != *ValuePtr)
							{
								// Confirmation prompt
								EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
									FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmNotes", "Change Notes from '{0}' to '{1}'?\n\nThis will mark the NPC as modified."),
										FText::FromString(ValuePtr->IsEmpty() ? TEXT("(Empty)") : *ValuePtr),
										FText::FromString(NewValue.IsEmpty() ? TEXT("(Empty)") : NewValue)));

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

	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SNPCTableRow::CreateStatusCell()
{
	FLinearColor StatusColor = RowData->GetStatusColor();
	FString StatusText = RowData->GetStatusString();

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

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableTextBox)
				.Text_Lambda([ValuePtr]()
				{
					// Show "(None)" for empty values instead of blank/placeholder
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
						if (NewValue != *ValuePtr)
						{
							// Confirmation prompt
							FString DisplayOld = ValuePtr->IsEmpty() ? TEXT("(None)") : *ValuePtr;
							FString DisplayNew = NewValue.IsEmpty() ? TEXT("(None)") : NewValue;
							EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
								FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmTextChange", "Change '{0}' from '{1}' to '{2}'?\n\nThis will mark the NPC as modified."),
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

TSharedRef<SWidget> SNPCTableRow::CreateCheckboxCell(bool& Value)
{
	bool* ValuePtr = &Value;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		.HAlign(HAlign_Center)
		[
			SNew(SCheckBox)
				.IsChecked_Lambda([ValuePtr]() { return *ValuePtr ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this, ValuePtr](ECheckBoxState NewState)
				{
					bool bNewValue = (NewState == ECheckBoxState::Checked);
					if (bNewValue != *ValuePtr)
					{
						// Confirmation prompt
						FString Action = bNewValue ? TEXT("Enable") : TEXT("Disable");
						EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
							FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmCheckbox", "{0} 'Is Vendor'?\n\nThis will mark the NPC as modified."),
								FText::FromString(Action)));

						if (Result == EAppReturnType::Yes)
						{
							*ValuePtr = bNewValue;
							MarkModified();
						}
					}
				})
		];
}

TSharedRef<SWidget> SNPCTableRow::CreateFactionsCell()
{
	// Multi-select dropdown for Narrative.Factions.* tags - shows short names (e.g., "Friendly" not "Narrative.Factions.Friendly")
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SComboButton)
				.OnGetMenuContent_Lambda([this]() -> TSharedRef<SWidget>
				{
					TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox);

					// Clear All button
					MenuContent->AddSlot()
					.AutoHeight()
					.Padding(4.0f, 2.0f)
					[
						SNew(SButton)
							.Text(NSLOCTEXT("NPCTableEditor", "ClearAllFactions", "(Clear All)"))
							.OnClicked_Lambda([this]()
							{
								if (!RowData->Factions.IsEmpty())
								{
									EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
										FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmClearFactions", "Clear all factions?\n\nCurrent: {0}"),
											FText::FromString(RowData->GetFactionsDisplay())));
									if (Result == EAppReturnType::Yes)
									{
										RowData->Factions.Empty();
										MarkModified();
									}
								}
								return FReply::Handled();
							})
					];

					// Separator
					MenuContent->AddSlot()
					.AutoHeight()
					[
						SNew(SSeparator)
					];

					// Get all Narrative.Factions.* tags from GameplayTagsManager
					TArray<FString> FactionTags;
					UGameplayTagsManager& TagManager = UGameplayTagsManager::Get();
					FGameplayTagContainer AllTags;
					TagManager.RequestAllGameplayTags(AllTags, true);

					for (const FGameplayTag& Tag : AllTags)
					{
						FString TagString = Tag.ToString();
						if (TagString.StartsWith(TEXT("Narrative.Factions.")))
						{
							FactionTags.Add(TagString);
						}
					}
					FactionTags.Sort();

					if (FactionTags.Num() == 0)
					{
						MenuContent->AddSlot()
						.AutoHeight()
						.Padding(4.0f, 2.0f)
						[
							SNew(STextBlock)
								.Text(NSLOCTEXT("NPCTableEditor", "NoFactions", "(No Narrative.Factions.* tags found)"))
								.Font(FCoreStyle::GetDefaultFontStyle("Italic", 8))
						];
					}
					else
					{
						// Checkboxes for each faction tag - show short names
						for (const FString& FullTag : FactionTags)
						{
							FString ShortName = FNPCTableRow::ToShortFactionName(FullTag);

							MenuContent->AddSlot()
							.AutoHeight()
							.Padding(4.0f, 1.0f)
							[
								SNew(SCheckBox)
									.IsChecked_Lambda([this, FullTag]()
									{
										return RowData->Factions.Contains(FullTag) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
									})
									.OnCheckStateChanged_Lambda([this, FullTag, ShortName](ECheckBoxState NewState)
									{
										TArray<FString> CurrentFactions;
										RowData->Factions.ParseIntoArray(CurrentFactions, TEXT(","));
										for (FString& F : CurrentFactions) { F = F.TrimStartAndEnd(); }

										bool bIsAdding = (NewState == ECheckBoxState::Checked);
										FText PromptText = bIsAdding
											? FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmAddFaction", "Add '{0}' to Factions?"), FText::FromString(ShortName))
											: FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmRemoveFaction", "Remove '{0}' from Factions?"), FText::FromString(ShortName));

										EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, PromptText);
										if (Result == EAppReturnType::Yes)
										{
											if (bIsAdding)
											{
												CurrentFactions.AddUnique(FullTag);
											}
											else
											{
												CurrentFactions.Remove(FullTag);
											}

											RowData->Factions = FString::Join(CurrentFactions, TEXT(", "));
											MarkModified();
										}
									})
									[
										SNew(STextBlock)
											.Text(FText::FromString(ShortName))
											.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
									]
							];
						}
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
						.Text_Lambda([this]()
						{
							if (RowData->Factions.IsEmpty())
							{
								return NSLOCTEXT("NPCTableEditor", "NoFactionsSelected", "(None)");
							}
							// Show short names comma-separated
							return FText::FromString(RowData->GetFactionsDisplay());
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				]
		];
}

TSharedRef<SWidget> SNPCTableRow::CreateLevelRangeCell()
{
	// Combined MinLevel-MaxLevel display using two spinboxes with confirmation prompts
	return SNew(SBox)
		.Padding(FMargin(2.0f, 2.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SSpinBox<int32>)
					.MinValue(1)
					.MaxValue(100)
					.Value_Lambda([this]() { return RowData->MinLevel; })
					.OnValueCommitted_Lambda([this](int32 NewValue, ETextCommit::Type CommitType)
					{
						if (CommitType != ETextCommit::OnCleared && NewValue != RowData->MinLevel)
						{
							EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
								FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmMinLevel", "Change Min Level from {0} to {1}?"),
									FText::AsNumber(RowData->MinLevel),
									FText::AsNumber(NewValue)));
							if (Result == EAppReturnType::Yes)
							{
								RowData->MinLevel = NewValue;
								// Ensure MaxLevel >= MinLevel
								if (RowData->MaxLevel < NewValue)
								{
									RowData->MaxLevel = NewValue;
								}
								MarkModified();
							}
						}
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
					.Text(FText::FromString(TEXT("-")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SSpinBox<int32>)
					.MinValue(1)
					.MaxValue(100)
					.Value_Lambda([this]() { return RowData->MaxLevel; })
					.OnValueCommitted_Lambda([this](int32 NewValue, ETextCommit::Type CommitType)
					{
						if (CommitType != ETextCommit::OnCleared && NewValue != RowData->MaxLevel)
						{
							EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
								FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmMaxLevel", "Change Max Level from {0} to {1}?"),
									FText::AsNumber(RowData->MaxLevel),
									FText::AsNumber(NewValue)));
							if (Result == EAppReturnType::Yes)
							{
								RowData->MaxLevel = NewValue;
								// Ensure MinLevel <= MaxLevel
								if (RowData->MinLevel > NewValue)
								{
									RowData->MinLevel = NewValue;
								}
								MarkModified();
							}
						}
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			]
		];
}

TSharedRef<SWidget> SNPCTableRow::CreateFloatSliderCell(float& Value)
{
	float* ValuePtr = &Value;

	// Text/number input for float values (0.0-1.0 range for AttackPriority)
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableTextBox)
				.Text_Lambda([ValuePtr]()
				{
					return FText::FromString(FString::Printf(TEXT("%.2f"), *ValuePtr));
				})
				.OnTextCommitted_Lambda([this, ValuePtr](const FText& NewText, ETextCommit::Type CommitType)
				{
					if (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus)
					{
						float NewValue = FCString::Atof(*NewText.ToString());
						// Clamp to valid range
						NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);

						if (FMath::Abs(NewValue - *ValuePtr) > KINDA_SMALL_NUMBER)
						{
							EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
								FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmPriority", "Change Attack Priority from {0} to {1}?"),
									FText::FromString(FString::Printf(TEXT("%.2f"), *ValuePtr)),
									FText::FromString(FString::Printf(TEXT("%.2f"), NewValue))));
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

TSharedRef<SWidget> SNPCTableRow::CreateItemsCell()
{
	// Multi-select dropdown for IC_* item collections - checkbox style like header filters
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SComboButton)
				.OnGetMenuContent_Lambda([this]() -> TSharedRef<SWidget>
				{
					TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox);

					// Clear All button
					MenuContent->AddSlot()
					.AutoHeight()
					.Padding(4.0f, 2.0f)
					[
						SNew(SButton)
							.Text(NSLOCTEXT("NPCTableEditor", "ClearAllItems", "(Clear All)"))
							.OnClicked_Lambda([this]()
							{
								if (!RowData->DefaultItems.IsEmpty())
								{
									EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
										FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmClearItems", "Clear all items?\n\nCurrent: {0}"),
											FText::FromString(RowData->DefaultItems)));
									if (Result == EAppReturnType::Yes)
									{
										RowData->DefaultItems.Empty();
										MarkModified();
									}
								}
								return FReply::Handled();
							})
					];

					// Separator
					MenuContent->AddSlot()
					.AutoHeight()
					[
						SNew(SSeparator)
					];

					// Get all ItemCollection assets
					FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
					TArray<FAssetData> Assets;
					Registry.Get().GetAssetsByClass(UItemCollection::StaticClass()->GetClassPathName(), Assets, true);

					// Checkboxes for each item collection
					for (const FAssetData& Asset : Assets)
					{
						FString AssetName = Asset.AssetName.ToString();

						MenuContent->AddSlot()
						.AutoHeight()
						.Padding(4.0f, 1.0f)
						[
							SNew(SCheckBox)
								.IsChecked_Lambda([this, AssetName]()
								{
									TArray<FString> CurrentItems;
									RowData->DefaultItems.ParseIntoArray(CurrentItems, TEXT(","));
									for (FString& Item : CurrentItems) { Item = Item.TrimStartAndEnd(); }
									return CurrentItems.Contains(AssetName) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
								})
								.OnCheckStateChanged_Lambda([this, AssetName](ECheckBoxState NewState)
								{
									TArray<FString> Items;
									RowData->DefaultItems.ParseIntoArray(Items, TEXT(","));
									for (FString& Item : Items) { Item = Item.TrimStartAndEnd(); }

									bool bIsAdding = (NewState == ECheckBoxState::Checked);
									FText PromptText = bIsAdding
										? FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmAddItem", "Add '{0}' to Default Items?"), FText::FromString(AssetName))
										: FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmRemoveItem", "Remove '{0}' from Default Items?"), FText::FromString(AssetName));

									EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, PromptText);
									if (Result == EAppReturnType::Yes)
									{
										if (bIsAdding)
										{
											Items.AddUnique(AssetName);
										}
										else
										{
											Items.Remove(AssetName);
										}

										RowData->DefaultItems = FString::Join(Items, TEXT(", "));
										MarkModified();
									}
								})
								[
									SNew(STextBlock)
										.Text(FText::FromString(AssetName))
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
						.Text_Lambda([this]()
						{
							if (RowData->DefaultItems.IsEmpty())
							{
								return NSLOCTEXT("NPCTableEditor", "NoItems", "(None)");
							}
							// Show all item names comma-separated
							return FText::FromString(RowData->DefaultItems);
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				]
		];
}

TSharedRef<SWidget> SNPCTableRow::CreatePOIDropdownCell()
{
	// Dropdown for POI selection from level - scans world for APOIActor instances
	// Shows short names (e.g., "Tavern" instead of "POI.Town.Tavern")

	// Helper lambda to extract short name from POI tag (last segment after final dot)
	auto GetShortPOIName = [](const FString& FullTag) -> FString
	{
		if (FullTag.IsEmpty()) return TEXT("(None)");

		int32 LastDotIndex;
		if (FullTag.FindLastChar(TEXT('.'), LastDotIndex) && LastDotIndex < FullTag.Len() - 1)
		{
			return FullTag.RightChop(LastDotIndex + 1);
		}
		return FullTag;  // No dot found, return as-is
	};

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SComboButton)
				.OnGetMenuContent_Lambda([this, GetShortPOIName]() -> TSharedRef<SWidget>
				{
					FMenuBuilder MenuBuilder(true, nullptr);

					// None option
					MenuBuilder.AddMenuEntry(
						NSLOCTEXT("NPCTableEditor", "ClearPOI", "(None)"),
						FText::GetEmpty(),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateLambda([this, GetShortPOIName]()
						{
							if (!RowData->SpawnerPOI.IsEmpty())
							{
								FString ShortName = GetShortPOIName(RowData->SpawnerPOI);
								EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
									FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmClearPOI", "Clear POI '{0}'?\n\nThis will mark the NPC as modified."),
										FText::FromString(ShortName)));
								if (Result == EAppReturnType::Yes)
								{
									RowData->SpawnerPOI.Empty();
									MarkModified();
								}
							}
						}))
					);

					MenuBuilder.AddSeparator();

					// Scan world for POI actors
					TArray<FString> POITags;
					if (GEditor && GEditor->GetEditorWorldContext().World())
					{
						UWorld* World = GEditor->GetEditorWorldContext().World();
						for (TActorIterator<APOIActor> POIIt(World); POIIt; ++POIIt)
						{
							APOIActor* POI = *POIIt;
							if (POI && POI->POITag.IsValid())
							{
								POITags.AddUnique(POI->POITag.ToString());
							}
						}
						POITags.Sort();
					}

					if (POITags.Num() == 0)
					{
						MenuBuilder.AddMenuEntry(
							NSLOCTEXT("NPCTableEditor", "NoPOIs", "(No POIs in level)"),
							NSLOCTEXT("NPCTableEditor", "NoPOIsTooltip", "Load a level with POI actors to see available POIs"),
							FSlateIcon(),
							FUIAction()
						);
					}
					else
					{
						for (const FString& POITag : POITags)
						{
							// Display short name but store full tag
							FString ShortName = GetShortPOIName(POITag);

							MenuBuilder.AddMenuEntry(
								FText::FromString(ShortName),
								FText::FromString(POITag),  // Full tag in tooltip
								FSlateIcon(),
								FUIAction(FExecuteAction::CreateLambda([this, POITag, ShortName, GetShortPOIName]()
								{
									if (POITag != RowData->SpawnerPOI)
									{
										FString OldShortName = RowData->SpawnerPOI.IsEmpty() ? TEXT("(None)") : GetShortPOIName(RowData->SpawnerPOI);
										EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
											FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmPOI", "Change Spawner POI from '{0}' to '{1}'?\n\nThis will mark the NPC as modified."),
												FText::FromString(OldShortName),
												FText::FromString(ShortName)));
										if (Result == EAppReturnType::Yes)
										{
											RowData->SpawnerPOI = POITag;
											MarkModified();
										}
									}
								}))
							);
						}
					}

					return MenuBuilder.MakeWidget();
				})
				.ButtonContent()
				[
					SNew(STextBlock)
						.Text_Lambda([this, GetShortPOIName]()
						{
							if (RowData->SpawnerPOI.IsEmpty())
							{
								return FText::FromString(TEXT("(None)"));
							}
							return FText::FromString(GetShortPOIName(RowData->SpawnerPOI));
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				]
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
							// Confirmation prompt
							FString CurrentValue = ValuePtr->IsNull() ? TEXT("(None)") : ValuePtr->GetAssetName();
							EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
								FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmClear", "Clear '{0}' and set to (None)?\n\nThis will mark the NPC as modified."),
									FText::FromString(CurrentValue)));

							if (Result == EAppReturnType::Yes)
							{
								ValuePtr->Reset();
								MarkModified();
							}
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
							FUIAction(FExecuteAction::CreateLambda([this, ValuePtr, AssetPath, AssetName]()
							{
								// Confirmation prompt
								FString CurrentValue = ValuePtr->IsNull() ? TEXT("(None)") : ValuePtr->GetAssetName();
								EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
									FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmChange", "Change from '{0}' to '{1}'?\n\nThis will mark the NPC as modified."),
										FText::FromString(CurrentValue),
										FText::FromString(AssetName)));

								if (Result == EAppReturnType::Yes)
								{
									*ValuePtr = AssetPath;
									MarkModified();
								}
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
							// Show "(None)" for null OR empty asset name
							if (ValuePtr->IsNull())
							{
								return FText::FromString(TEXT("(None)"));
							}
							FString AssetName = ValuePtr->GetAssetName();
							return FText::FromString(AssetName.IsEmpty() ? TEXT("(None)") : AssetName);
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				]
		];
}

TSharedRef<SWidget> SNPCTableRow::CreateNPCBlueprintDropdownCell()
{
	// Dropdown that only shows Blueprint classes inheriting from ANarrativeNPCCharacter
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SComboButton)
				.OnGetMenuContent_Lambda([this]() -> TSharedRef<SWidget>
				{
					FMenuBuilder MenuBuilder(true, nullptr);

					// None option
					MenuBuilder.AddMenuEntry(
						NSLOCTEXT("NPCTableEditor", "NoneBlueprint", "(None)"),
						FText::GetEmpty(),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateLambda([this]()
						{
							// For TSoftClassPtr: extract Blueprint name from asset path
							FString CurrentValue = RowData->Blueprint.IsNull() ? TEXT("(None)") : FPaths::GetBaseFilename(RowData->Blueprint.GetAssetPath().ToString());
							EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
								FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmClearBlueprint", "Clear Blueprint from '{0}'?"),
									FText::FromString(CurrentValue)));
							if (Result == EAppReturnType::Yes)
							{
								RowData->Blueprint.Reset();
								MarkModified();
							}
						}))
					);

					MenuBuilder.AddSeparator();

					// Get all Blueprint assets and filter for ANarrativeNPCCharacter subclasses
					FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
					TArray<FAssetData> AllBlueprints;
					Registry.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), AllBlueprints, true);

					for (const FAssetData& Asset : AllBlueprints)
					{
						// Check if this blueprint's parent class is ANarrativeNPCCharacter or a subclass
						FAssetDataTagMapSharedView::FFindTagResult ParentClassTag = Asset.TagsAndValues.FindTag(FBlueprintTags::ParentClassPath);
						if (ParentClassTag.IsSet())
						{
							FString ParentClassPath = ParentClassTag.GetValue();

							// Check if parent class path contains NarrativeNPCCharacter or common NPC character classes
							if (ParentClassPath.Contains(TEXT("NarrativeNPCCharacter")) ||
								ParentClassPath.Contains(TEXT("NarrativeCharacter")) ||
								ParentClassPath.Contains(TEXT("BP_NarrativeNPC")))
							{
								FString AssetName = Asset.AssetName.ToString();
								FSoftObjectPath AssetPath = Asset.GetSoftObjectPath();

								MenuBuilder.AddMenuEntry(
									FText::FromString(AssetName),
									FText::FromString(Asset.GetObjectPathString()),
									FSlateIcon(),
									FUIAction(FExecuteAction::CreateLambda([this, AssetPath, AssetName]()
									{
										// For TSoftClassPtr: extract Blueprint name from asset path
										FString CurrentValue = RowData->Blueprint.IsNull() ? TEXT("(None)") : FPaths::GetBaseFilename(RowData->Blueprint.GetAssetPath().ToString());
										EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
											FText::Format(NSLOCTEXT("NPCTableEditor", "ConfirmChangeBlueprint", "Change Blueprint from '{0}' to '{1}'?"),
												FText::FromString(CurrentValue),
												FText::FromString(AssetName)));
										if (Result == EAppReturnType::Yes)
										{
											RowData->Blueprint = AssetPath;
											MarkModified();
										}
									}))
								);
							}
						}
					}

					return MenuBuilder.MakeWidget();
				})
				.ButtonContent()
				[
					SNew(STextBlock)
						.Text_Lambda([this]()
						{
							// Show "(None)" for null OR empty asset name
							if (RowData->Blueprint.IsNull())
							{
								return FText::FromString(TEXT("(None)"));
							}
							// For TSoftClassPtr: extract Blueprint name from asset path
							// Path format: "/Package/Path/BP_Name.BP_Name_C"
							// GetAssetPath() returns: "/Package/Path/BP_Name" -> extract "BP_Name"
							FString AssetName = FPaths::GetBaseFilename(RowData->Blueprint.GetAssetPath().ToString());
							if (AssetName.IsEmpty())
							{
								return FText::FromString(TEXT("(None)"));
							}
							return FText::FromString(AssetName);
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				]
		];
}

void SNPCTableRow::MarkModified()
{
	if (RowData.IsValid() && RowData->Status != ENPCTableRowStatus::New)
	{
		RowData->Status = ENPCTableRowStatus::Modified;
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
	InitializeColumnFilters();
	UpdateColumnFilterOptions();

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

		// Separator
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
		]

		// Table - fills entire width (no horizontal scroll wrapper)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(ListView, SListView<TSharedPtr<FNPCTableRow>>)
				.ListItemsSource(&DisplayedRows)
				.OnGenerateRow(this, &SNPCTableEditor::OnGenerateRow)
				.OnSelectionChanged(this, &SNPCTableEditor::OnSelectionChanged)
				.SelectionMode(ESelectionMode::Multi)
				.HeaderRow(BuildHeaderRow())
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
	UpdateStatusBar();  // v4.5: Initial status bar population
}

void SNPCTableEditor::SetTableData(UNPCTableData* InTableData)
{
	TableData = InTableData;
	SyncFromTableData();
	UpdateColumnFilterOptions();
	ApplyFilters();
	UpdateStatusBar();  // v4.5
}

TSharedRef<SWidget> SNPCTableEditor::BuildToolbar()
{
	return SNew(SHorizontalBox)

		// LEFT SIDE - Data actions
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
				.OnClicked(this, &SNPCTableEditor::OnClearFiltersClicked)
				.ToolTipText(LOCTEXT("ClearFiltersTip", "Reset all column filters"))
		]

		// SPACER - pushes right side buttons to the right
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNullWidget::NullWidget
		]

		// RIGHT SIDE - Generation/IO actions
		// Validate (v4.5)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("Validate", "Validate"))
				.OnClicked(this, &SNPCTableEditor::OnValidateClicked)
				.ToolTipText(LOCTEXT("ValidateTip", "Validate all rows for errors and warnings"))
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

		// Export XLSX (Excel with sync support)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("ExportXLSX", "Export Excel"))
				.OnClicked(this, &SNPCTableEditor::OnExportXLSXClicked)
				.ToolTipText(LOCTEXT("ExportXLSXTooltip", "Export to Excel format (.xlsx) with sync metadata for round-trip editing"))
		]

		// Import XLSX (triggers 3-way merge)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("ImportXLSX", "Import Excel"))
				.OnClicked(this, &SNPCTableEditor::OnImportXLSXClicked)
				.ToolTipText(LOCTEXT("ImportXLSXTooltip", "Import from Excel format (.xlsx) with 3-way merge for conflict detection"))
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
		Header->AddColumn(
			SHeaderRow::Column(Col.ColumnId)
				.DefaultLabel(Col.DisplayName)
				.FillWidth(Col.DefaultWidth)  // Proportional fill width
				.SortMode(this, &SNPCTableEditor::GetColumnSortMode, Col.ColumnId)
				.OnSort(this, &SNPCTableEditor::OnColumnSortModeChanged)
				.HeaderContent()
				[
					BuildColumnHeaderContent(Col)
				]
		);
	}

	HeaderRow = Header;
	return Header;
}

TSharedRef<SWidget> SNPCTableEditor::BuildColumnHeaderContent(const FNPCTableColumn& Col)
{
	FNPCColumnFilterState& FilterState = ColumnFilters.FindOrAdd(Col.ColumnId);

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
					FNPCColumnFilterState* State = ColumnFilters.Find(ColumnId);
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
									FNPCColumnFilterState* S = ColumnFilters.Find(ColumnId);
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
									FNPCColumnFilterState* S = ColumnFilters.Find(ColumnId);
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
						// Display the value directly - empty cells now return "(None)" from GetColumnValue
						FString DisplayText = OptionValue;

						MenuContent->AddSlot()
						.AutoHeight()
						.Padding(4.0f, 1.0f)
						[
							SNew(SCheckBox)
								.IsChecked_Lambda([this, ColumnId, OptionValue]()
								{
									FNPCColumnFilterState* S = ColumnFilters.Find(ColumnId);
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
							FNPCColumnFilterState* State = ColumnFilters.Find(ColumnId);
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

void SNPCTableEditor::InitializeColumnFilters()
{
	TArray<FNPCTableColumn> Columns = GetNPCTableColumns();
	for (const FNPCTableColumn& Col : Columns)
	{
		FNPCColumnFilterState& State = ColumnFilters.Add(Col.ColumnId);
		State.DropdownOptions.Add(MakeShared<FString>(TEXT(""))); // "(All)" option placeholder
	}
}

void SNPCTableEditor::UpdateColumnFilterOptions()
{
	TArray<FNPCTableColumn> Columns = GetNPCTableColumns();

	for (const FNPCTableColumn& Col : Columns)
	{
		FNPCColumnFilterState* State = ColumnFilters.Find(Col.ColumnId);
		if (!State) continue;

		State->DropdownOptions.Empty();
		State->DropdownOptions.Add(MakeShared<FString>(TEXT(""))); // "(All)"

		TSet<FString> UniqueValues;
		for (const auto& Row : AllRows)
		{
			FString Value = GetColumnValue(Row, Col.ColumnId);
			UniqueValues.Add(Value);
		}

		TArray<FString> SortedValues = UniqueValues.Array();
		SortedValues.Sort();

		for (const FString& Value : SortedValues)
		{
			State->DropdownOptions.Add(MakeShared<FString>(Value));
		}
	}
}

void SNPCTableEditor::OnColumnTextFilterChanged(FName ColumnId, const FText& NewText)
{
	FNPCColumnFilterState* State = ColumnFilters.Find(ColumnId);
	if (State)
	{
		State->TextFilter = NewText.ToString();
		ApplyFilters();
	}
}

void SNPCTableEditor::OnColumnDropdownFilterChanged(FName ColumnId, const FString& Value, bool bIsSelected)
{
	FNPCColumnFilterState* State = ColumnFilters.Find(ColumnId);
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

FReply SNPCTableEditor::OnClearFiltersClicked()
{
	for (auto& FilterPair : ColumnFilters)
	{
		FilterPair.Value.TextFilter.Empty();
		FilterPair.Value.SelectedValues.Empty();
	}
	ApplyFilters();
	return FReply::Handled();
}

// Helper to get asset name with "(None)" for null/empty
// For TSoftObjectPtr paths (DataAssets): GetAssetName() returns the asset name
// For TSoftClassPtr paths (Blueprints): GetAssetName() returns the UClass name (with "_C")
//   so we use GetAssetPath() to get the Blueprint asset name instead
static FString GetAssetDisplayName(const FSoftObjectPath& Path, bool bIsClassPath = false)
{
	if (Path.IsNull())
	{
		return TEXT("(None)");
	}

	FString AssetName;
	if (bIsClassPath)
	{
		// For TSoftClassPtr: extract Blueprint name from asset path
		// Path format: "/Package/Path/BP_Name.BP_Name_C"
		// GetAssetPath() returns: "/Package/Path/BP_Name"
		AssetName = FPaths::GetBaseFilename(Path.GetAssetPath().ToString());
	}
	else
	{
		// For TSoftObjectPtr: GetAssetName() returns the asset name directly
		AssetName = Path.GetAssetName();
	}

	if (AssetName.IsEmpty())
	{
		return TEXT("(None)");
	}
	return AssetName;
}

FString SNPCTableEditor::GetColumnValue(const TSharedPtr<FNPCTableRow>& Row, FName ColumnId) const
{
	if (!Row.IsValid()) return TEXT("(None)");

	// Core Identity (5)
	if (ColumnId == TEXT("Status")) return Row->GetStatusString();
	if (ColumnId == TEXT("NPCName")) return Row->NPCName.IsEmpty() ? TEXT("(None)") : Row->NPCName;
	if (ColumnId == TEXT("NPCId")) return Row->NPCId.IsEmpty() ? TEXT("(None)") : Row->NPCId;
	if (ColumnId == TEXT("DisplayName")) return Row->DisplayName.IsEmpty() ? TEXT("(None)") : Row->DisplayName;
	if (ColumnId == TEXT("Blueprint")) return GetAssetDisplayName(Row->Blueprint, true);  // TSoftClassPtr - use asset path

	// AI & Behavior (4)
	if (ColumnId == TEXT("AbilityConfig")) return GetAssetDisplayName(Row->AbilityConfig);
	if (ColumnId == TEXT("ActivityConfig")) return GetAssetDisplayName(Row->ActivityConfig);
	if (ColumnId == TEXT("Schedule")) return GetAssetDisplayName(Row->Schedule);
	if (ColumnId == TEXT("BehaviorTree")) return GetAssetDisplayName(Row->BehaviorTree);

	// Combat (3)
	if (ColumnId == TEXT("LevelRange")) return Row->GetLevelRangeDisplay();
	if (ColumnId == TEXT("Factions")) return Row->GetFactionsDisplay().IsEmpty() ? TEXT("(None)") : Row->GetFactionsDisplay();
	if (ColumnId == TEXT("AttackPriority")) return FString::Printf(TEXT("%.2f"), Row->AttackPriority);

	// Vendor (2)
	if (ColumnId == TEXT("bIsVendor")) return Row->bIsVendor ? TEXT("Yes") : TEXT("No");
	if (ColumnId == TEXT("ShopName")) return Row->ShopName.IsEmpty() ? TEXT("(None)") : Row->ShopName;

	// Items & Spawning (2)
	if (ColumnId == TEXT("DefaultItems")) return Row->DefaultItems.IsEmpty() ? TEXT("(None)") : Row->DefaultItems;
	if (ColumnId == TEXT("SpawnerPOI")) return Row->SpawnerPOI.IsEmpty() ? TEXT("(None)") : Row->SpawnerPOI;

	// Meta (2)
	if (ColumnId == TEXT("Appearance"))
	{
		if (Row->Appearance.IsNull())
		{
			return TEXT("(None)");
		}
		FString AppearanceName = Row->Appearance.GetAssetName();
		if (AppearanceName.IsEmpty())
		{
			return TEXT("(None)");
		}
		// Strip "Appearance." or "Appearance_" prefix for shorter display
		if (AppearanceName.StartsWith(TEXT("Appearance.")))
		{
			AppearanceName = AppearanceName.RightChop(11);  // Remove "Appearance."
		}
		else if (AppearanceName.StartsWith(TEXT("Appearance_")))
		{
			AppearanceName = AppearanceName.RightChop(11);  // Remove "Appearance_"
		}
		return AppearanceName;
	}
	if (ColumnId == TEXT("Notes")) return Row->Notes.IsEmpty() ? TEXT("(None)") : Row->Notes;

	return TEXT("(None)");
}

TArray<FString> SNPCTableEditor::GetUniqueColumnValues(FName ColumnId) const
{
	// Filter dropdowns show values from the current table data (AllRows)
	// Cell edit dropdowns (CreateAssetDropdownCell) separately scan project for all available assets
	// Only include empty option if there are actually empty cells in that column
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
	// v4.5: Store STextBlock references for direct SetText() updates
	// This bypasses all Slate caching/invalidation issues by updating text directly
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SAssignNew(StatusTotalText, STextBlock)
				.Text(LOCTEXT("InitTotal", "Total: 0 NPCs"))
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f)
		[
			SAssignNew(StatusShowingText, STextBlock)
				.Text(LOCTEXT("InitShowing", "Showing: 0"))
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f)
		[
			SAssignNew(StatusSelectedText, STextBlock)
				.Text(LOCTEXT("InitSelected", "Selected: 0"))
		]

		// v4.5: Validation errors (shown in red when > 0)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f)
		[
			SAssignNew(StatusValidationText, STextBlock)
				.Text(LOCTEXT("InitValidation", ""))
				.ColorAndOpacity(FSlateColor(FLinearColor::Red))
		];
}

// v4.5: Direct SetText() update - bypasses all Slate caching issues
void SNPCTableEditor::UpdateStatusBar()
{
	// Total NPCs
	if (StatusTotalText.IsValid())
	{
		StatusTotalText->SetText(FText::Format(
			LOCTEXT("TotalNPCs", "Total: {0} NPCs"),
			FText::AsNumber(AllRows.Num())
		));
	}

	// Currently displayed rows (after filtering)
	if (StatusShowingText.IsValid())
	{
		StatusShowingText->SetText(FText::Format(
			LOCTEXT("ShowingCount", "Showing: {0}"),
			FText::AsNumber(DisplayedRows.Num())
		));
	}

	// Selected rows
	if (StatusSelectedText.IsValid())
	{
		int32 SelectedCount = ListView.IsValid() ? ListView->GetNumItemsSelected() : 0;
		StatusSelectedText->SetText(FText::Format(
			LOCTEXT("SelectedCount", "Selected: {0}"),
			FText::AsNumber(SelectedCount)
		));
	}

	// v4.5: Validation errors count
	if (StatusValidationText.IsValid())
	{
		// Quick validation check
		TArray<FNPCTableRow> RowsToValidate;
		for (const auto& RowPtr : AllRows)
		{
			if (RowPtr.IsValid())
			{
				RowsToValidate.Add(*RowPtr);
			}
		}

		FNPCValidationResult ValidationResult = FNPCTableValidator::ValidateAll(RowsToValidate);
		int32 ErrorCount = ValidationResult.GetErrorCount();

		if (ErrorCount > 0)
		{
			StatusValidationText->SetText(FText::Format(
				LOCTEXT("ValidationErrors", "Errors: {0}"),
				FText::AsNumber(ErrorCount)
			));
			StatusValidationText->SetVisibility(EVisibility::Visible);
		}
		else
		{
			StatusValidationText->SetText(FText::GetEmpty());
			StatusValidationText->SetVisibility(EVisibility::Collapsed);
		}
	}
}

TSharedRef<ITableRow> SNPCTableEditor::OnGenerateRow(TSharedPtr<FNPCTableRow> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SNPCTableRow, OwnerTable)
		.RowData(Item)
		.OnRowModified(FSimpleDelegate::CreateSP(this, &SNPCTableEditor::OnRowModified));
}

void SNPCTableEditor::OnSelectionChanged(TSharedPtr<FNPCTableRow> Item, ESelectInfo::Type SelectInfo)
{
	UpdateStatusBar();  // v4.5: Update selected count
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
	UpdateStatusBar();  // v4.5: Explicit status bar update
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

		// Check each column filter (text filter + multi-select dropdown)
		for (const auto& FilterPair : ColumnFilters)
		{
			const FName& ColumnId = FilterPair.Key;
			const FNPCColumnFilterState& FilterState = FilterPair.Value;
			FString ColumnValue = GetColumnValue(Row, ColumnId);

			// 1. Text filter - case-insensitive contains
			if (!FilterState.TextFilter.IsEmpty())
			{
				if (!ColumnValue.ToLower().Contains(FilterState.TextFilter.ToLower()))
				{
					bPassesFilters = false;
					break;
				}
			}

			// 2. Multi-select dropdown filter
			// Empty SelectedValues = show all (no filter)
			// __NONE_SELECTED__ marker = show nothing
			// Otherwise, only show rows where column value is in SelectedValues
			if (FilterState.SelectedValues.Num() > 0)
			{
				if (FilterState.SelectedValues.Contains(TEXT("__NONE_SELECTED__")))
				{
					// None selected - filter out everything
					bPassesFilters = false;
					break;
				}

				// Check if column value is in the selected values set
				if (!FilterState.SelectedValues.Contains(ColumnValue))
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
		// String-based columns
		if (SortColumn == TEXT("NPCName") || SortColumn == TEXT("NPCId") ||
			SortColumn == TEXT("DisplayName") || SortColumn == TEXT("Status") ||
			SortColumn == TEXT("Factions") || SortColumn == TEXT("ShopName") ||
			SortColumn == TEXT("DefaultItems") || SortColumn == TEXT("SpawnerPOI") || SortColumn == TEXT("Notes"))
		{
			FString ValueA, ValueB;
			if (SortColumn == TEXT("NPCName")) { ValueA = A->NPCName; ValueB = B->NPCName; }
			else if (SortColumn == TEXT("NPCId")) { ValueA = A->NPCId; ValueB = B->NPCId; }
			else if (SortColumn == TEXT("DisplayName")) { ValueA = A->DisplayName; ValueB = B->DisplayName; }
			else if (SortColumn == TEXT("Status")) { ValueA = A->GetStatusString(); ValueB = B->GetStatusString(); }
			else if (SortColumn == TEXT("Factions")) { ValueA = A->Factions; ValueB = B->Factions; }
			else if (SortColumn == TEXT("ShopName")) { ValueA = A->ShopName; ValueB = B->ShopName; }
			else if (SortColumn == TEXT("DefaultItems")) { ValueA = A->DefaultItems; ValueB = B->DefaultItems; }
			else if (SortColumn == TEXT("SpawnerPOI")) { ValueA = A->SpawnerPOI; ValueB = B->SpawnerPOI; }
			else if (SortColumn == TEXT("Notes")) { ValueA = A->Notes; ValueB = B->Notes; }
			return bAscending ? (ValueA < ValueB) : (ValueA > ValueB);
		}

		// LevelRange - sort by MinLevel
		if (SortColumn == TEXT("LevelRange"))
		{
			return bAscending ? (A->MinLevel < B->MinLevel) : (A->MinLevel > B->MinLevel);
		}

		// Float columns
		if (SortColumn == TEXT("AttackPriority"))
		{
			return bAscending ? (A->AttackPriority < B->AttackPriority) : (A->AttackPriority > B->AttackPriority);
		}

		// Boolean columns
		if (SortColumn == TEXT("bIsVendor"))
		{
			return bAscending ? (A->bIsVendor < B->bIsVendor) : (A->bIsVendor > B->bIsVendor);
		}

		return false;
	});
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

FReply SNPCTableEditor::OnValidateClicked()
{
	// v4.5: Validate all rows and show detailed results
	TArray<FNPCTableRow> RowsToValidate;
	for (const TSharedPtr<FNPCTableRow>& RowPtr : AllRows)
	{
		if (RowPtr.IsValid())
		{
			RowsToValidate.Add(*RowPtr);
		}
	}

	FNPCValidationResult ValidationResult = FNPCTableValidator::ValidateAll(RowsToValidate);

	// Update row status based on validation
	for (TSharedPtr<FNPCTableRow>& Row : AllRows)
	{
		if (!Row.IsValid()) continue;

		bool bHasError = false;
		bool bHasWarning = false;

		for (const FNPCValidationIssue& Issue : ValidationResult.Issues)
		{
			if (Issue.NPCName == Row->NPCName || (Row->NPCName.IsEmpty() && Issue.NPCName == Row->RowId.ToString()))
			{
				if (Issue.Severity == ENPCValidationSeverity::Error)
				{
					bHasError = true;
				}
				else if (Issue.Severity == ENPCValidationSeverity::Warning)
				{
					bHasWarning = true;
				}
			}
		}

		if (bHasError)
		{
			Row->Status = ENPCTableRowStatus::Error;
		}
		else if (bHasWarning)
		{
			Row->Status = ENPCTableRowStatus::Modified;  // Use Modified as warning indicator
		}
		else if (Row->Status == ENPCTableRowStatus::Error)
		{
			Row->Status = ENPCTableRowStatus::New;  // Clear previous error if now valid
		}
	}

	RefreshList();
	UpdateStatusBar();

	// Build message with all issues
	int32 ErrorCount = ValidationResult.GetErrorCount();
	int32 WarningCount = ValidationResult.Issues.Num() - ErrorCount;

	FString Message;
	if (ErrorCount == 0 && WarningCount == 0)
	{
		Message = TEXT("All rows passed validation!");
	}
	else
	{
		Message = FString::Printf(TEXT("Validation found %d error(s) and %d warning(s):\n\n"), ErrorCount, WarningCount);

		// Show errors first
		for (const FNPCValidationIssue& Issue : ValidationResult.Issues)
		{
			if (Issue.Severity == ENPCValidationSeverity::Error)
			{
				Message += FString::Printf(TEXT("[ERROR] %s.%s: %s\n"), *Issue.NPCName, *Issue.FieldName, *Issue.Message);
			}
		}

		// Then warnings
		for (const FNPCValidationIssue& Issue : ValidationResult.Issues)
		{
			if (Issue.Severity == ENPCValidationSeverity::Warning)
			{
				Message += FString::Printf(TEXT("[WARN] %s.%s: %s\n"), *Issue.NPCName, *Issue.FieldName, *Issue.Message);
			}
		}
	}

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));

	return FReply::Handled();
}

FReply SNPCTableEditor::OnGenerateClicked()
{
	// v4.5: Validate first, then generate using FNPCDefinitionGenerator

	// Step 1: Gather all valid rows
	TArray<FNPCTableRow> RowsToValidate;
	for (const TSharedPtr<FNPCTableRow>& RowPtr : AllRows)
	{
		if (RowPtr.IsValid())
		{
			RowsToValidate.Add(*RowPtr);
		}
	}

	if (RowsToValidate.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoRows", "No NPC rows to generate. Add NPCs first."));
		return FReply::Handled();
	}

	// Step 2: Validate all rows
	FNPCValidationResult ValidationResult = FNPCTableValidator::ValidateAll(RowsToValidate);
	int32 ValidationErrorCount = ValidationResult.GetErrorCount();

	if (ValidationErrorCount > 0)
	{
		// Show validation errors and abort
		FString ErrorMessage = FString::Printf(TEXT("Cannot generate: %d validation error(s) found.\n\nFix these errors first:\n\n"), ValidationErrorCount);
		for (const FNPCValidationIssue& Issue : ValidationResult.Issues)
		{
			if (Issue.Severity == ENPCValidationSeverity::Error)
			{
				ErrorMessage += FString::Printf(TEXT("[ERROR] %s.%s: %s\n"), *Issue.NPCName, *Issue.FieldName, *Issue.Message);
			}
		}
		ErrorMessage += TEXT("\nClick 'Validate' button for full details.");

		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ErrorMessage));

		// Update status on rows with errors
		for (TSharedPtr<FNPCTableRow>& Row : AllRows)
		{
			if (!Row.IsValid()) continue;
			for (const FNPCValidationIssue& Issue : ValidationResult.Issues)
			{
				if (Issue.Severity == ENPCValidationSeverity::Error &&
					(Issue.NPCName == Row->NPCName || (Row->NPCName.IsEmpty() && Issue.NPCName == Row->RowId.ToString())))
				{
					Row->Status = ENPCTableRowStatus::Error;
					break;
				}
			}
		}
		RefreshList();
		UpdateStatusBar();
		return FReply::Handled();
	}

	// Step 3: Convert rows to manifest definitions
	TArray<FManifestNPCDefinitionDefinition> Definitions = FNPCTableConverter::ConvertRowsToManifest(RowsToValidate);

	// Step 4: Generate NPCDefinition assets
	int32 GeneratedCount = 0;
	int32 SkippedCount = 0;
	int32 ErrorCount = 0;
	TArray<FString> GenerationErrors;

	for (const FManifestNPCDefinitionDefinition& Def : Definitions)
	{
		FGenerationResult Result = FNPCDefinitionGenerator::Generate(Def);

		// Extract NPCName from definition name (remove "NPCDef_" prefix)
		FString NPCNameFromDef = Def.Name;
		if (NPCNameFromDef.StartsWith(TEXT("NPCDef_")))
		{
			NPCNameFromDef = NPCNameFromDef.Mid(7);
		}

		if (Result.Status == EGenerationStatus::New)
		{
			GeneratedCount++;

			// Update row status
			for (TSharedPtr<FNPCTableRow>& Row : AllRows)
			{
				if (Row.IsValid() && Row->NPCName == NPCNameFromDef)
				{
					Row->Status = ENPCTableRowStatus::Synced;
					break;
				}
			}
		}
		else if (Result.Status == EGenerationStatus::Skipped)
		{
			SkippedCount++;

			// Keep existing status for skipped rows
			for (TSharedPtr<FNPCTableRow>& Row : AllRows)
			{
				if (Row.IsValid() && Row->NPCName == NPCNameFromDef)
				{
					if (Row->Status != ENPCTableRowStatus::Synced)
					{
						Row->Status = ENPCTableRowStatus::Synced;  // Already exists is also synced
					}
					break;
				}
			}
		}
		else  // Failed or Deferred
		{
			ErrorCount++;
			GenerationErrors.Add(FString::Printf(TEXT("%s: %s"), *Def.Name, *Result.Message));

			// Update row status
			for (TSharedPtr<FNPCTableRow>& Row : AllRows)
			{
				if (Row.IsValid() && Row->NPCName == NPCNameFromDef)
				{
					Row->Status = ENPCTableRowStatus::Error;
					break;
				}
			}
		}
	}

	RefreshList();
	MarkDirty();
	UpdateStatusBar();

	// Step 5: Show results
	FString ResultMessage = FString::Printf(
		TEXT("NPC Definition Generation Complete!\n\n")
		TEXT("Generated: %d\n")
		TEXT("Skipped (already exists): %d\n")
		TEXT("Errors: %d\n")
		TEXT("Warnings: %d"),
		GeneratedCount,
		SkippedCount,
		ErrorCount,
		ValidationResult.Issues.Num() - ValidationErrorCount);

	if (GenerationErrors.Num() > 0)
	{
		ResultMessage += TEXT("\n\nGeneration Errors:\n");
		for (const FString& Error : GenerationErrors)
		{
			ResultMessage += FString::Printf(TEXT("  - %s\n"), *Error);
		}
	}

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ResultMessage));

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

	UE_LOG(LogTemp, Log, TEXT("[NPCTableEditor] Found %d NPCDefinition assets"), AssetList.Num());

	//=========================================================================
	// Build POI mapping: NPCDefinition -> nearest POI tag
	// Scan world for NPCSpawners and POIActors
	//=========================================================================
	TMap<UNPCDefinition*, FString> NPCToPOIMap;

	if (GEditor && GEditor->GetEditorWorldContext().World())
	{
		UWorld* World = GEditor->GetEditorWorldContext().World();

		// Collect all POI actors and their locations
		TArray<TPair<FVector, FString>> POILocations;
		for (TActorIterator<APOIActor> POIIt(World); POIIt; ++POIIt)
		{
			APOIActor* POI = *POIIt;
			if (POI && POI->POITag.IsValid())
			{
				POILocations.Add(TPair<FVector, FString>(POI->GetActorLocation(), POI->POITag.ToString()));
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[NPCTableEditor] Found %d POI actors in world"), POILocations.Num());

		// Scan NPCSpawners and map NPCDefinitions to their spawn locations
		for (TActorIterator<ANPCSpawner> SpawnerIt(World); SpawnerIt; ++SpawnerIt)
		{
			ANPCSpawner* Spawner = *SpawnerIt;
			if (!Spawner) continue;

			FVector SpawnerLocation = Spawner->GetActorLocation();

			// Get all NPCSpawnComponents on this spawner
			TArray<UNPCSpawnComponent*> SpawnComponents;
			Spawner->GetComponents<UNPCSpawnComponent>(SpawnComponents);

			for (UNPCSpawnComponent* SpawnComp : SpawnComponents)
			{
				if (SpawnComp && SpawnComp->NPCToSpawn)
				{
					// Find nearest POI to this spawner
					float NearestDistSq = FLT_MAX;
					FString NearestPOI;

					for (const auto& POIPair : POILocations)
					{
						float DistSq = FVector::DistSquared(SpawnerLocation, POIPair.Key);
						if (DistSq < NearestDistSq)
						{
							NearestDistSq = DistSq;
							NearestPOI = POIPair.Value;
						}
					}

					if (!NearestPOI.IsEmpty())
					{
						NPCToPOIMap.Add(SpawnComp->NPCToSpawn, NearestPOI);
						UE_LOG(LogTemp, Log, TEXT("[NPCTableEditor] Mapped %s -> %s (dist: %.0f)"),
							*SpawnComp->NPCToSpawn->GetName(), *NearestPOI, FMath::Sqrt(NearestDistSq));
					}
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[NPCTableEditor] Built POI map with %d entries"), NPCToPOIMap.Num());

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

		//=========================================================================
		// Core Identity (5 columns)
		//=========================================================================
		Row.NPCName = AssetData.AssetName.ToString();
		Row.NPCId = NPCDef->NPCID.ToString();
		Row.DisplayName = NPCDef->NPCName.ToString();

		// Blueprint - NPCClassPath
		// Use ToString() to get the raw Blueprint asset path, not ToSoftObjectPath()
		// which resolves to the generated UClass (adds "_C" suffix)
		if (!NPCDef->NPCClassPath.IsNull())
		{
			Row.Blueprint = FSoftObjectPath(NPCDef->NPCClassPath.ToString());
		}

		//=========================================================================
		// AI & Behavior (4 columns)
		//=========================================================================
		// AbilityConfiguration is on CharacterDefinition base class
		if (NPCDef->AbilityConfiguration)
		{
			Row.AbilityConfig = FSoftObjectPath(NPCDef->AbilityConfiguration);
		}
		if (!NPCDef->ActivityConfiguration.IsNull())
		{
			Row.ActivityConfig = NPCDef->ActivityConfiguration.ToSoftObjectPath();
		}
		// Schedule - from TriggerSets array first (UTriggerSet assets)
		// TriggerSets are on CharacterDefinition base class
		if (NPCDef->TriggerSets.Num() > 0 && !NPCDef->TriggerSets[0].IsNull())
		{
			Row.Schedule = NPCDef->TriggerSets[0].ToSoftObjectPath();
			UE_LOG(LogTemp, Log, TEXT("[NPCTableEditor] %s: Schedule from TriggerSets[0] = %s"),
				*Row.NPCName, *Row.Schedule.GetAssetName());
		}
		// Fallback: check ActivitySchedules (UNPCActivitySchedule assets)
		else if (NPCDef->ActivitySchedules.Num() > 0 && !NPCDef->ActivitySchedules[0].IsNull())
		{
			Row.Schedule = NPCDef->ActivitySchedules[0].ToSoftObjectPath();
			UE_LOG(LogTemp, Log, TEXT("[NPCTableEditor] %s: Schedule from ActivitySchedules[0] = %s"),
				*Row.NPCName, *Row.Schedule.GetAssetName());
		}
		// BehaviorTree - not directly on NPCDefinition, would need to come from ActivityConfiguration

		//=========================================================================
		// Combat (3 columns)
		//=========================================================================
		Row.MinLevel = NPCDef->MinLevel;
		Row.MaxLevel = NPCDef->MaxLevel;
		Row.AttackPriority = NPCDef->AttackPriority;

		// Factions - convert FGameplayTagContainer to comma-separated short names for display
		TArray<FString> FactionShortNames;
		for (const FGameplayTag& FactionTag : NPCDef->DefaultFactions)
		{
			FactionShortNames.Add(FNPCTableRow::ToShortFactionName(FactionTag.ToString()));
		}
		Row.Factions = FString::Join(FactionShortNames, TEXT(", "));

		//=========================================================================
		// Vendor (2 columns)
		//=========================================================================
		Row.bIsVendor = NPCDef->bIsVendor;
		Row.ShopName = NPCDef->ShopFriendlyName.ToString();

		//=========================================================================
		// Items & Spawning (2 columns)
		//=========================================================================
		// DefaultItems - extract IC_ references from DefaultItemLoadout loot table rolls
		TArray<FString> ItemCollectionNames;
		UE_LOG(LogTemp, Log, TEXT("[NPCTableEditor] %s: DefaultItemLoadout has %d rolls"),
			*Row.NPCName, NPCDef->DefaultItemLoadout.Num());
		for (const FLootTableRoll& Roll : NPCDef->DefaultItemLoadout)
		{
			UE_LOG(LogTemp, Log, TEXT("[NPCTableEditor]   Roll has %d ItemCollectionsToGrant"),
				Roll.ItemCollectionsToGrant.Num());
			for (const TObjectPtr<UItemCollection>& IC : Roll.ItemCollectionsToGrant)
			{
				if (IC)
				{
					ItemCollectionNames.AddUnique(IC->GetName());
					UE_LOG(LogTemp, Log, TEXT("[NPCTableEditor]     Found IC: %s"), *IC->GetName());
				}
			}
		}
		Row.DefaultItems = FString::Join(ItemCollectionNames, TEXT(", "));
		if (!Row.DefaultItems.IsEmpty())
		{
			UE_LOG(LogTemp, Log, TEXT("[NPCTableEditor] %s: DefaultItems = %s"), *Row.NPCName, *Row.DefaultItems);
		}
		// SpawnerPOI - from NPCSpawner -> nearest POI mapping
		if (FString* POITag = NPCToPOIMap.Find(NPCDef))
		{
			Row.SpawnerPOI = *POITag;
		}

		//=========================================================================
		// Meta (2 columns)
		//=========================================================================
		// Appearance - from CharacterDefinition base class
		if (!NPCDef->DefaultAppearance.IsNull())
		{
			Row.Appearance = NPCDef->DefaultAppearance.ToSoftObjectPath();
			UE_LOG(LogTemp, Log, TEXT("[NPCTableEditor] %s: Appearance = %s"),
				*Row.NPCName, *Row.Appearance.GetAssetName());
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[NPCTableEditor] %s: DefaultAppearance is NULL"), *Row.NPCName);
		}
		// Notes - user-added, not from assets

		// Generated asset reference (internal tracking)
		Row.GeneratedNPCDef = AssetData.GetSoftObjectPath();
		Row.Status = ENPCTableRowStatus::Synced;

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

	// Helper to escape CSV fields
	auto EscapeCSV = [](const FString& Value) -> FString
	{
		if (Value.Contains(TEXT(",")) || Value.Contains(TEXT("\"")))
		{
			return FString::Printf(TEXT("\"%s\""), *Value.Replace(TEXT("\""), TEXT("\"\"")));
		}
		return Value;
	};

	// Header row - 18 columns matching v4.1 structure
	CSV += TEXT("NPCName,NPCId,DisplayName,Blueprint,AbilityConfig,ActivityConfig,Schedule,BehaviorTree,MinLevel,MaxLevel,Factions,AttackPriority,IsVendor,ShopName,DefaultItems,SpawnerPOI,Appearance,Notes\n");

	// Data rows
	for (const TSharedPtr<FNPCTableRow>& Row : AllRows)
	{
		CSV += FString::Printf(TEXT("%s,%s,%s,%s,%s,%s,%s,%s,%d,%d,%s,%.2f,%s,%s,%s,%s,%s,%s\n"),
			*EscapeCSV(Row->NPCName),
			*EscapeCSV(Row->NPCId),
			*EscapeCSV(Row->DisplayName),
			*EscapeCSV(Row->Blueprint.IsNull() ? TEXT("") : FPaths::GetBaseFilename(Row->Blueprint.GetAssetPath().ToString())),
			*EscapeCSV(Row->AbilityConfig.IsNull() ? TEXT("") : Row->AbilityConfig.GetAssetName()),
			*EscapeCSV(Row->ActivityConfig.IsNull() ? TEXT("") : Row->ActivityConfig.GetAssetName()),
			*EscapeCSV(Row->Schedule.IsNull() ? TEXT("") : Row->Schedule.GetAssetName()),
			*EscapeCSV(Row->BehaviorTree.IsNull() ? TEXT("") : Row->BehaviorTree.GetAssetName()),
			Row->MinLevel,
			Row->MaxLevel,
			*EscapeCSV(Row->GetFactionsDisplay()),
			Row->AttackPriority,
			Row->bIsVendor ? TEXT("TRUE") : TEXT("FALSE"),
			*EscapeCSV(Row->ShopName),
			*EscapeCSV(Row->DefaultItems),
			*EscapeCSV(Row->SpawnerPOI),
			*EscapeCSV(Row->Appearance.IsNull() ? TEXT("") : Row->Appearance.GetAssetName()),
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
						int32 ImportedCount = 0;
						// Skip header row
						for (int32 i = 1; i < Lines.Num(); i++)
						{
							TArray<FString> Cells;
							Lines[i].ParseIntoArray(Cells, TEXT(","));

							// CSV format (18 columns):
							// 0:NPCName, 1:NPCId, 2:DisplayName, 3:Blueprint, 4:AbilityConfig,
							// 5:ActivityConfig, 6:Schedule, 7:BehaviorTree, 8:MinLevel, 9:MaxLevel,
							// 10:Factions, 11:AttackPriority, 12:IsVendor, 13:ShopName, 14:DefaultItems,
							// 15:SpawnerPOI, 16:Appearance, 17:Notes
							if (Cells.Num() >= 10) // Minimum columns to be useful
							{
								FNPCTableRow& NewRow = TableData->AddRow();

								// Core Identity
								NewRow.NPCName = Cells[0].TrimQuotes();
								NewRow.NPCId = Cells.Num() > 1 ? Cells[1].TrimQuotes() : NewRow.NPCName.ToLower().Replace(TEXT(" "), TEXT("_"));
								NewRow.DisplayName = Cells.Num() > 2 ? Cells[2].TrimQuotes() : NewRow.NPCName;
								// Blueprint, AbilityConfig, ActivityConfig, Schedule, BehaviorTree - skip paths for now (indices 3-7)

								// Combat
								NewRow.MinLevel = Cells.Num() > 8 ? FCString::Atoi(*Cells[8].TrimQuotes()) : 1;
								NewRow.MaxLevel = Cells.Num() > 9 ? FCString::Atoi(*Cells[9].TrimQuotes()) : 10;
								if (Cells.Num() > 10) NewRow.SetFactionsFromDisplay(Cells[10].TrimQuotes());
								NewRow.AttackPriority = Cells.Num() > 11 ? FCString::Atof(*Cells[11].TrimQuotes()) : 0.5f;

								// Vendor
								NewRow.bIsVendor = Cells.Num() > 12 && Cells[12].TrimQuotes().ToUpper() == TEXT("TRUE");
								NewRow.ShopName = Cells.Num() > 13 ? Cells[13].TrimQuotes() : TEXT("");

								// Items & Spawning
								NewRow.DefaultItems = Cells.Num() > 14 ? Cells[14].TrimQuotes() : TEXT("");
								NewRow.SpawnerPOI = Cells.Num() > 15 ? Cells[15].TrimQuotes() : TEXT("");

								// Meta
								// Appearance - skip path for now (index 16)
								NewRow.Notes = Cells.Num() > 17 ? Cells[17].TrimQuotes() : TEXT("");

								ImportedCount++;
							}
						}

						SyncFromTableData();
						MarkDirty();

						FMessageDialog::Open(EAppMsgType::Ok,
							FText::Format(LOCTEXT("ImportSuccess", "Imported {0} NPCs from:\n{1}"),
								FText::AsNumber(ImportedCount),
								FText::FromString(OutFiles[0])));
					}
				}
			}
		}
	}

	return FReply::Handled();
}

//=============================================================================
// XLSX Export/Import (Excel format with 3-way sync support)
//=============================================================================

FReply SNPCTableEditor::OnExportXLSXClicked()
{
#if WITH_EDITOR
	// Build array of FNPCTableRow from shared pointers
	TArray<FNPCTableRow> RowsToExport;
	for (const TSharedPtr<FNPCTableRow>& Row : AllRows)
	{
		if (Row.IsValid())
		{
			RowsToExport.Add(*Row);
		}
	}

	if (RowsToExport.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("ExportXLSXEmpty", "No rows to export. Add some NPCs first."));
		return FReply::Handled();
	}

	// Open file dialog
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		TArray<FString> OutFiles;
		if (DesktopPlatform->SaveFileDialog(
			nullptr,
			TEXT("Export NPC Table to Excel"),
			FPaths::ProjectDir(),
			TEXT("NPCTable.xlsx"),
			TEXT("Excel Files (*.xlsx)|*.xlsx"),
			0,
			OutFiles))
		{
			if (OutFiles.Num() > 0)
			{
				FString ErrorMessage;
				if (FNPCXLSXWriter::ExportToXLSX(RowsToExport, OutFiles[0], ErrorMessage))
				{
					FMessageDialog::Open(EAppMsgType::Ok,
						FText::Format(LOCTEXT("ExportXLSXSuccess", "Exported {0} NPCs to:\n{1}\n\nYou can now edit in Excel and import back."),
							FText::AsNumber(RowsToExport.Num()),
							FText::FromString(OutFiles[0])));
				}
				else
				{
					FMessageDialog::Open(EAppMsgType::Ok,
						FText::Format(LOCTEXT("ExportXLSXFailed", "Failed to export:\n{0}"),
							FText::FromString(ErrorMessage)));
				}
			}
		}
	}
#else
	FMessageDialog::Open(EAppMsgType::Ok,
		LOCTEXT("ExportXLSXEditorOnly", "XLSX export requires Editor builds."));
#endif

	return FReply::Handled();
}

FReply SNPCTableEditor::OnImportXLSXClicked()
{
#if WITH_EDITOR
	if (!TableData)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("ImportXLSXNoTable", "No table data. Create or open a table first."));
		return FReply::Handled();
	}

	// Open file dialog
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		TArray<FString> OutFiles;
		if (DesktopPlatform->OpenFileDialog(
			nullptr,
			TEXT("Import NPC Table from Excel"),
			FPaths::ProjectDir(),
			TEXT(""),
			TEXT("Excel Files (*.xlsx)|*.xlsx"),
			0,
			OutFiles))
		{
			if (OutFiles.Num() > 0)
			{
				// Import from XLSX
				FNPCXLSXImportResult ImportResult = FNPCXLSXReader::ImportFromXLSX(OutFiles[0]);
				if (!ImportResult.bSuccess)
				{
					FMessageDialog::Open(EAppMsgType::Ok,
						FText::Format(LOCTEXT("ImportXLSXFailed", "Failed to import:\n{0}"),
							FText::FromString(ImportResult.ErrorMessage)));
					return FReply::Handled();
				}

				// Build base rows (from last export, if available - currently empty for fresh import)
				// TODO: Load base rows from stored snapshot for 3-way merge
				TArray<FNPCTableRow> BaseRows;  // Empty = first sync, all Excel changes accepted

				// Build current UE rows
				TArray<FNPCTableRow> UERows;
				for (const TSharedPtr<FNPCTableRow>& Row : AllRows)
				{
					if (Row.IsValid())
					{
						UERows.Add(*Row);
					}
				}

				// Perform 3-way comparison
				FNPCSyncResult SyncResult = FNPCXLSXSyncEngine::CompareSources(
					BaseRows, UERows, ImportResult.Rows);

				// Auto-resolve non-conflicts first
				FNPCXLSXSyncEngine::AutoResolveNonConflicts(SyncResult);

				// Show sync dialog for review and conflict resolution
				if (SyncResult.HasChanges() || SyncResult.HasConflicts())
				{
					if (!SNPCXLSXSyncDialog::ShowModal(SyncResult))
					{
						// User cancelled - don't apply changes
						return FReply::Handled();
					}
				}

				// Apply the sync
				FNPCMergeResult MergeResult = FNPCXLSXSyncEngine::ApplySync(SyncResult);

				// Replace table data with merged rows
				TableData->Rows.Empty();
				for (const FNPCTableRow& Row : MergeResult.MergedRows)
				{
					TableData->Rows.Add(Row);
				}

				// Refresh UI
				SyncFromTableData();
				MarkDirty();

				// Show summary
				FMessageDialog::Open(EAppMsgType::Ok,
					FText::Format(LOCTEXT("ImportXLSXSuccess",
						"Imported from Excel:\n"
						"  - {0} rows from Excel\n"
						"  - {1} rows kept from UE\n"
						"  - {2} rows deleted\n"
						"  - {3} rows unchanged\n\n"
						"Total: {4} rows"),
						FText::AsNumber(MergeResult.AppliedFromExcel),
						FText::AsNumber(MergeResult.AppliedFromUE),
						FText::AsNumber(MergeResult.Deleted),
						FText::AsNumber(MergeResult.Unchanged),
						FText::AsNumber(MergeResult.MergedRows.Num())));
			}
		}
	}
#else
	FMessageDialog::Open(EAppMsgType::Ok,
		LOCTEXT("ImportXLSXEditorOnly", "XLSX import requires Editor builds."));
#endif

	return FReply::Handled();
}

FReply SNPCTableEditor::OnApplyToAssetsClicked()
{
	// v4.5: Use FNPCXLSXSyncEngine for bidirectional sync with AssetRegistry lookup

	// Step 1: Gather rows to apply
	TArray<FNPCTableRow> RowsToApply;
	for (const TSharedPtr<FNPCTableRow>& RowPtr : AllRows)
	{
		if (RowPtr.IsValid())
		{
			RowsToApply.Add(*RowPtr);
		}
	}

	if (RowsToApply.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoRowsToApply", "No NPC rows to apply. Add NPCs first."));
		return FReply::Handled();
	}

	// Step 2: Apply changes using FNPCXLSXSyncEngine (with AssetRegistry lookup)
	FString NPCAssetPath = TableData ? TableData->OutputFolder : TEXT("/Game/NPCs");
	FNPCAssetApplySummary Result = FNPCXLSXSyncEngine::ApplyToAssets(
		RowsToApply,
		NPCAssetPath,
		false);  // bCreateMissing - don't create new assets, use Generate for that

	// Step 3: Update row statuses based on results
	for (TSharedPtr<FNPCTableRow>& RowPtr : AllRows)
	{
		if (!RowPtr.IsValid()) continue;

		// Find the corresponding row in the applied array
		for (const FNPCTableRow& AppliedRow : RowsToApply)
		{
			if (AppliedRow.RowId == RowPtr->RowId)
			{
				RowPtr->Status = AppliedRow.Status;
				// Also update GeneratedNPCDef if it was found via AssetRegistry
				if (!AppliedRow.GeneratedNPCDef.IsNull())
				{
					RowPtr->GeneratedNPCDef = AppliedRow.GeneratedNPCDef;
				}
				break;
			}
		}
	}

	RefreshList();
	UpdateStatusBar();

	// Step 4: Build result message
	FString Message = FString::Printf(TEXT("Applied changes to %d NPCDefinition assets."), Result.AssetsModified);

	if (Result.AssetsSkippedNotModified > 0)
	{
		Message += FString::Printf(TEXT("\nUnchanged: %d"), Result.AssetsSkippedNotModified);
	}
	if (Result.AssetsSkippedValidation > 0)
	{
		Message += FString::Printf(TEXT("\nSkipped (validation errors): %d"), Result.AssetsSkippedValidation);
	}
	if (Result.AssetsSkippedNoAsset > 0)
	{
		Message += FString::Printf(TEXT("\nNo asset found: %d"), Result.AssetsSkippedNoAsset);
	}
	if (Result.AssetsSkippedReadOnly > 0)
	{
		Message += FString::Printf(TEXT("\nRead-only (plugin content): %d"), Result.AssetsSkippedReadOnly);
	}
	if (Result.FailedNPCs.Num() > 0)
	{
		Message += FString::Printf(TEXT("\nFailed to save: %s"), *FString::Join(Result.FailedNPCs, TEXT(", ")));
	}

	// Show per-asset details if there are only a few
	if (Result.AssetResults.Num() > 0 && Result.AssetResults.Num() <= 10)
	{
		Message += TEXT("\n\nDetails:");
		for (const auto& Pair : Result.AssetResults)
		{
			Message += FString::Printf(TEXT("\n  %s: %s"), *Pair.Key, *Pair.Value);
		}
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
