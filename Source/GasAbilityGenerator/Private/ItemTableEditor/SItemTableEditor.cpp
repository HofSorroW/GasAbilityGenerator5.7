// SItemTableEditor.cpp
// Item Table Editor Implementation
// v4.8: Follows NPC/Dialogue table patterns with dynamic column visibility
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#include "ItemTableEditor/SItemTableEditor.h"
#include "ItemTableEditor/ItemXLSXWriter.h"
#include "ItemTableEditor/ItemXLSXReader.h"
#include "XLSXSupport/ItemXLSXSyncEngine.h"   // v4.12: 3-way sync engine
#include "XLSXSupport/SItemXLSXSyncDialog.h"  // v4.12: Sync dialog
#include "ItemTableEditor/ItemAssetSync.h"    // v4.12: Asset sync
#include "ItemTableConverter.h"
#include "ItemTableValidator.h"
#include "GasAbilityGeneratorGenerators.h"
#include "DesktopPlatformModule.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSpinBox.h"
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
#include "Items/NarrativeItem.h"
#include "Items/EquippableItem.h"
#include "Items/WeaponItem.h"
#include "Items/RangedWeaponItem.h"
#include "Items/MeleeWeaponItem.h"
#include "Items/MagicWeaponItem.h"
#include "Items/ThrowableWeaponItem.h"
#include "Items/AmmoItem.h"
#include "Items/GameplayEffectItem.h"
#include "Items/WeaponAttachmentItem.h"
#include "Engine/BlueprintCore.h"  // v4.12.5: FBlueprintTags for parent class filtering
#include "Widgets/Input/SComboButton.h"  // v4.12.6: For dropdown arrow
#include "Subsystems/AssetEditorSubsystem.h"  // v4.12.6: For opening assets in editor

#define LOCTEXT_NAMESPACE "ItemTableEditor"

//=============================================================================
// v4.12.5: Prefix Trimming Helper
// Strips common asset prefixes for cleaner display
//=============================================================================

static FString TrimAssetPrefix_Item(const FString& AssetName)
{
	// List of common prefixes to strip (order matters - longer prefixes first)
	static const TArray<FString> Prefixes = {
		TEXT("Appearance_"),  // Character appearances
		TEXT("Schedule_"),    // Activity schedules
		TEXT("AC_"),          // AbilityConfiguration, ActivityConfiguration
		TEXT("NPC_"),         // NPCDefinition
		TEXT("IC_"),          // ItemCollection
		TEXT("BP_"),          // Blueprint
		TEXT("GE_"),          // GameplayEffect
		TEXT("GA_"),          // GameplayAbility
		TEXT("EI_"),          // EquippableItem
		TEXT("DBP_"),         // DialogueBlueprint
		TEXT("QBP_"),         // QuestBlueprint
		TEXT("BT_"),          // BehaviorTree
		TEXT("BB_"),          // Blackboard
		TEXT("TS_"),          // TriggerSet
	};

	for (const FString& Prefix : Prefixes)
	{
		if (AssetName.StartsWith(Prefix))
		{
			return AssetName.Mid(Prefix.Len());
		}
	}
	return AssetName;
}

//=============================================================================
// SItemTableRow
//=============================================================================

void SItemTableRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	RowData = InArgs._RowData;
	OnRowModified = InArgs._OnRowModified;

	SMultiColumnTableRow<TSharedPtr<FItemTableRow>>::Construct(
		FSuperRowType::FArguments(),
		InOwnerTable
	);
}

TSharedRef<SWidget> SItemTableRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (!RowData.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	FItemTableRow& Row = *RowData;

	// Check dynamic visibility
	if (!ShouldShowColumn(ColumnName))
	{
		return SNullWidget::NullWidget;
	}

	if (ColumnName == TEXT("Status"))
	{
		return CreateStatusCell();
	}
	else if (ColumnName == TEXT("ItemName"))
	{
		// v4.12.6: Click-to-open + dropdown + trimmed display
		return CreateItemNameCell();
	}
	else if (ColumnName == TEXT("DisplayName"))
	{
		return CreateTextCell(Row.DisplayName, TEXT("Display name"));
	}
	else if (ColumnName == TEXT("ItemType"))
	{
		return CreateItemTypeCell();
	}
	else if (ColumnName == TEXT("EquipmentSlot"))
	{
		return CreateEquipmentSlotCell();
	}
	else if (ColumnName == TEXT("BaseValue"))
	{
		return CreateIntCell(Row.BaseValue, TEXT("Gold value"));
	}
	else if (ColumnName == TEXT("Weight"))
	{
		return CreateFloatCell(Row.Weight, TEXT("Weight in kg"));
	}
	else if (ColumnName == TEXT("AttackRating"))
	{
		return CreateFloatCell(Row.AttackRating, TEXT("Attack rating"));
	}
	else if (ColumnName == TEXT("ArmorRating"))
	{
		return CreateFloatCell(Row.ArmorRating, TEXT("Armor rating"));
	}
	else if (ColumnName == TEXT("ModifierGE"))
	{
		return CreateModifierGECell();
	}
	else if (ColumnName == TEXT("Abilities"))
	{
		return CreateAbilitiesCell();
	}
	// Weapon columns
	else if (ColumnName == TEXT("AttackDamage"))
	{
		return CreateFloatCell(Row.AttackDamage, TEXT("Base damage"));
	}
	else if (ColumnName == TEXT("ClipSize"))
	{
		return CreateIntCell(Row.ClipSize, TEXT("Clip size"));
	}
	else if (ColumnName == TEXT("WeaponHand"))
	{
		return CreateTextCell(Row.WeaponHand, TEXT("TwoHanded"));
	}
	// Ranged columns
	else if (ColumnName == TEXT("BaseSpreadDegrees"))
	{
		return CreateFloatCell(Row.BaseSpreadDegrees, TEXT("Spread"));
	}
	else if (ColumnName == TEXT("AimFOVPct"))
	{
		return CreateFloatCell(Row.AimFOVPct, TEXT("FOV %"));
	}
	else if (ColumnName == TEXT("ItemTags"))
	{
		return CreateTextCell(Row.ItemTags, TEXT("Item.Category.Type"));
	}
	else if (ColumnName == TEXT("bStackable"))
	{
		return CreateCheckboxCell(Row.bStackable);
	}
	else if (ColumnName == TEXT("MaxStackSize"))
	{
		return CreateIntCell(Row.MaxStackSize, TEXT("Max stack"));
	}
	else if (ColumnName == TEXT("Notes"))
	{
		return CreateNotesCell();
	}

	return SNullWidget::NullWidget;
}

bool SItemTableRow::ShouldShowColumn(FName ColumnId) const
{
	if (!RowData.IsValid()) return true;

	const FItemTableRow& Row = *RowData;

	// Dynamic visibility based on item type
	if (ColumnId == TEXT("AttackRating"))
	{
		return Row.IsEquipment();
	}
	if (ColumnId == TEXT("ArmorRating"))
	{
		return Row.IsArmor();
	}
	if (ColumnId == TEXT("StealthRating"))
	{
		return Row.IsEquipment();
	}
	// Weapon columns
	if (ColumnId == TEXT("AttackDamage") || ColumnId == TEXT("ClipSize") ||
	    ColumnId == TEXT("WeaponHand") || ColumnId == TEXT("HeavyAttackDamageMultiplier") ||
	    ColumnId == TEXT("bAllowManualReload") || ColumnId == TEXT("BotAttackRange"))
	{
		return Row.IsWeapon();
	}
	// Ranged weapon columns
	if (ColumnId == TEXT("BaseSpreadDegrees") || ColumnId == TEXT("MaxSpreadDegrees") ||
	    ColumnId == TEXT("SpreadFireBump") || ColumnId == TEXT("SpreadDecreaseSpeed") ||
	    ColumnId == TEXT("AimFOVPct"))
	{
		return Row.IsRangedWeapon();
	}
	// Consumable columns
	if (ColumnId == TEXT("bConsumeOnUse") || ColumnId == TEXT("UseRechargeDuration") ||
	    ColumnId == TEXT("bCanActivate") || ColumnId == TEXT("GameplayEffectClass"))
	{
		return Row.IsConsumable();
	}

	return true;
}

TSharedRef<SWidget> SItemTableRow::CreateStatusCell()
{
	// v4.12.5: Match NPC Table cell styling with validation stripe + status badge
	// Layout: [4px validation stripe] [status badge]
	return SNew(SHorizontalBox)
		// Validation stripe (4px colored bar on left - matches NPC pattern)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
				.WidthOverride(4.0f)
				[
					SNew(SBorder)
						.BorderBackgroundColor_Lambda([this]()
						{
							if (RowData.IsValid())
							{
								return FSlateColor(RowData->GetValidationColor());
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
					return RowData.IsValid() ? FSlateColor(RowData->GetStatusColor()) : FSlateColor(FLinearColor::White);
				})
				.Padding(FMargin(6.0f, 2.0f))
				[
					SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return RowData.IsValid() ? FText::FromString(RowData->GetStatusString()) : FText::GetEmpty();
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
				]
		];
}

TSharedRef<SWidget> SItemTableRow::CreateTextCell(FString& Value, const FString& Hint)
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
								FText::Format(NSLOCTEXT("ItemTableEditor", "ConfirmTextChange", "Change '{0}' from '{1}' to '{2}'?\n\nThis will mark the Item as modified."),
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

TSharedRef<SWidget> SItemTableRow::CreateItemNameCell()
{
	// v4.12.6: Click-to-open Item asset + dropdown for single-select + trimmed display
	FString* ValuePtr = &RowData->ItemName;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SHorizontalBox)

			// Clickable hyperlink text - opens Item Blueprint asset in editor
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
							// v4.12.7 FIX: Find any Blueprint asset by name and open it
							// Removed parent class filter - any clickable item should open
							FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
							TArray<FAssetData> Assets;
							Registry.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), Assets, true);

							for (const FAssetData& Asset : Assets)
							{
								if (Asset.AssetName.ToString().Equals(*ValuePtr, ESearchCase::IgnoreCase))
								{
									// v4.12.7: Open any matching Blueprint without parent class filter
									// This allows BI_, BPNI_, EI_ and any other item types to be clickable
									if (UObject* LoadedAsset = Asset.GetAsset())
									{
										GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(LoadedAsset);
									}
									break;
								}
							}
						}
						return FReply::Handled();
					})
					.ToolTipText_Lambda([ValuePtr]()
					{
						if (ValuePtr->IsEmpty())
						{
							return FText::FromString(TEXT("No Item selected"));
						}
						return FText::Format(NSLOCTEXT("ItemTableEditor", "ClickToOpenItem", "Click to open: {0}"),
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
								return FText::FromString(TrimAssetPrefix_Item(*ValuePtr));
							})
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
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
							NSLOCTEXT("ItemTableEditor", "ItemNameNone", "(None)"),
							FText::GetEmpty(),
							FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([this, ValuePtr]()
							{
								if (!ValuePtr->IsEmpty())
								{
									EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
										FText::Format(NSLOCTEXT("ItemTableEditor", "ConfirmClearItemName", "Clear Item Name '{0}'?\n\nThis will mark the Item as modified."),
											FText::FromString(TrimAssetPrefix_Item(*ValuePtr))));
									if (Result == EAppReturnType::Yes)
									{
										ValuePtr->Empty();
										MarkModified();
									}
								}
							}))
						);

						MenuBuilder.AddSeparator();

						// Get all Item Blueprint assets - v4.12.7: Use prefix-based filtering instead of parent class
						FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
						TArray<FAssetData> Assets;
						Registry.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), Assets, true);

						// Collect and sort item assets
						TArray<FAssetData> ItemAssets;
						for (const FAssetData& Asset : Assets)
						{
							FString AssetName = Asset.AssetName.ToString();
							// v4.12.7: Include all item-prefixed Blueprints (BI_, BPNI_, EI_)
							if (AssetName.StartsWith(TEXT("BI_")) ||
								AssetName.StartsWith(TEXT("BPNI_")) ||
								AssetName.StartsWith(TEXT("EI_")))
							{
								ItemAssets.Add(Asset);
							}
						}

						// v4.12.7: Sort alphabetically
						ItemAssets.Sort([](const FAssetData& A, const FAssetData& B)
						{
							return A.AssetName.ToString() < B.AssetName.ToString();
						});

						for (const FAssetData& Asset : ItemAssets)
						{
							FString AssetName = Asset.AssetName.ToString();

							MenuBuilder.AddMenuEntry(
								FText::FromString(TrimAssetPrefix_Item(AssetName)),
								FText::FromString(Asset.GetObjectPathString()),
								FSlateIcon(),
								FUIAction(FExecuteAction::CreateLambda([this, ValuePtr, AssetName]()
								{
									if (*ValuePtr != AssetName)
									{
										FString OldValue = ValuePtr->IsEmpty() ? TEXT("(None)") : TrimAssetPrefix_Item(*ValuePtr);
										EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
											FText::Format(NSLOCTEXT("ItemTableEditor", "ConfirmItemNameChange", "Change Item Name from '{0}' to '{1}'?\n\nThis will mark the Item as modified."),
												FText::FromString(OldValue),
												FText::FromString(TrimAssetPrefix_Item(AssetName))));

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
		];
}

TSharedRef<SWidget> SItemTableRow::CreateFloatCell(float& Value, const FString& Hint)
{
	// v4.12.5: Use text input with confirmation dialog (matching NPC table pattern)
	float* ValuePtr = &Value;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableTextBox)
				.Text_Lambda([ValuePtr]() {
					return FText::FromString(FString::SanitizeFloat(*ValuePtr));
				})
				.HintText(FText::FromString(Hint))
				.OnTextCommitted_Lambda([this, ValuePtr, Hint](const FText& NewText, ETextCommit::Type CommitType) {
					if (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus)
					{
						float NewValue = FCString::Atof(*NewText.ToString());
						if (!FMath::IsNearlyEqual(*ValuePtr, NewValue))
						{
							// Confirmation prompt
							EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
								FText::Format(NSLOCTEXT("ItemTableEditor", "ConfirmFloatChange", "Change '{0}' from '{1}' to '{2}'?\n\nThis will mark the Item as modified."),
									FText::FromString(Hint),
									FText::FromString(FString::SanitizeFloat(*ValuePtr)),
									FText::FromString(FString::SanitizeFloat(NewValue))));

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

TSharedRef<SWidget> SItemTableRow::CreateIntCell(int32& Value, const FString& Hint)
{
	// v4.12.5: Use text input with confirmation dialog (matching NPC table pattern)
	int32* ValuePtr = &Value;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableTextBox)
				.Text_Lambda([ValuePtr]() {
					return FText::FromString(FString::FromInt(*ValuePtr));
				})
				.HintText(FText::FromString(Hint))
				.OnTextCommitted_Lambda([this, ValuePtr, Hint](const FText& NewText, ETextCommit::Type CommitType) {
					if (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus)
					{
						int32 NewValue = FCString::Atoi(*NewText.ToString());
						if (*ValuePtr != NewValue)
						{
							// Confirmation prompt
							EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
								FText::Format(NSLOCTEXT("ItemTableEditor", "ConfirmIntChange", "Change '{0}' from '{1}' to '{2}'?\n\nThis will mark the Item as modified."),
									FText::FromString(Hint),
									FText::FromString(FString::FromInt(*ValuePtr)),
									FText::FromString(FString::FromInt(NewValue))));

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

TSharedRef<SWidget> SItemTableRow::CreateCheckboxCell(bool& Value)
{
	// v4.12.5: Match NPC Table cell styling
	bool* ValuePtr = &Value;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		.HAlign(HAlign_Center)
		[
			SNew(SCheckBox)
				.IsChecked_Lambda([ValuePtr]() { return *ValuePtr ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this, ValuePtr](ECheckBoxState NewState) {
					*ValuePtr = (NewState == ECheckBoxState::Checked);
					MarkModified();
				})
		];
}

TSharedRef<SWidget> SItemTableRow::CreateItemTypeCell()
{
	// v4.12.5: Editable dropdown with EItemType enum values (matching NPC table pattern)
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SComboButton)
				.OnGetMenuContent_Lambda([this]() -> TSharedRef<SWidget>
				{
					FMenuBuilder MenuBuilder(true, nullptr);

					// All EItemType values
					struct FItemTypeOption { EItemType Type; FString Name; };
					static const TArray<FItemTypeOption> Options = {
						{ EItemType::Consumable, TEXT("Consumable") },
						{ EItemType::Ammo, TEXT("Ammo") },
						{ EItemType::WeaponAttachment, TEXT("Weapon Attachment") },
						{ EItemType::Equippable, TEXT("Equippable") },
						{ EItemType::Clothing, TEXT("Clothing") },
						{ EItemType::ThrowableWeapon, TEXT("Throwable Weapon") },
						{ EItemType::MeleeWeapon, TEXT("Melee Weapon") },
						{ EItemType::RangedWeapon, TEXT("Ranged Weapon") },
						{ EItemType::MagicWeapon, TEXT("Magic Weapon") }
					};

					for (const FItemTypeOption& Option : Options)
					{
						MenuBuilder.AddMenuEntry(
							FText::FromString(Option.Name),
							FText::GetEmpty(),
							FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([this, Option]()
							{
								if (!RowData.IsValid()) return;

								if (RowData->ItemType != Option.Type)
								{
									EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
										FText::Format(NSLOCTEXT("ItemTableEditor", "ConfirmItemTypeChange", "Change Item Type from '{0}' to '{1}'?\n\nThis will mark the Item as modified."),
											FText::FromString(RowData->GetItemTypeString()),
											FText::FromString(Option.Name)));

									if (Result == EAppReturnType::Yes)
									{
										RowData->ItemType = Option.Type;
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
					SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return RowData.IsValid() ? FText::FromString(RowData->GetItemTypeString()) : FText::GetEmpty();
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				]
		];
}

TSharedRef<SWidget> SItemTableRow::CreateEquipmentSlotCell()
{
	// v4.12.5: Editable dropdown with equipment slot tag options (matching NPC table pattern)
	FString* ValuePtr = &RowData->EquipmentSlot;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SComboButton)
				.OnGetMenuContent_Lambda([this, ValuePtr]() -> TSharedRef<SWidget>
				{
					FMenuBuilder MenuBuilder(true, nullptr);

					// None option
					MenuBuilder.AddMenuEntry(
						NSLOCTEXT("ItemTableEditor", "SlotNone", "(None)"),
						FText::GetEmpty(),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateLambda([this, ValuePtr]()
						{
							if (!ValuePtr->IsEmpty())
							{
								EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
									FText::Format(NSLOCTEXT("ItemTableEditor", "ConfirmClearSlot", "Clear Equipment Slot '{0}'?\n\nThis will mark the Item as modified."),
										FText::FromString(*ValuePtr)));
								if (Result == EAppReturnType::Yes)
								{
									ValuePtr->Empty();
									MarkModified();
								}
							}
						}))
					);

					MenuBuilder.AddSeparator();

					// Common equipment slot tags
					static const TArray<FString> SlotOptions = {
						TEXT("Narrative.Equipment.Slot.Head"),
						TEXT("Narrative.Equipment.Slot.Chest"),
						TEXT("Narrative.Equipment.Slot.Hands"),
						TEXT("Narrative.Equipment.Slot.Legs"),
						TEXT("Narrative.Equipment.Slot.Feet"),
						TEXT("Narrative.Equipment.Slot.Back"),
						TEXT("Narrative.Equipment.Slot.Weapon"),
						TEXT("Narrative.Equipment.Slot.WeaponMainhand"),
						TEXT("Narrative.Equipment.Slot.WeaponOffhand"),
						TEXT("Narrative.Equipment.Slot.Accessory"),
						TEXT("Narrative.Equipment.Slot.Ring"),
						TEXT("Narrative.Equipment.Slot.Necklace")
					};

					for (const FString& SlotTag : SlotOptions)
					{
						// Extract short name for display (last part after last dot)
						FString ShortName = SlotTag;
						int32 LastDot;
						if (SlotTag.FindLastChar(TEXT('.'), LastDot))
						{
							ShortName = SlotTag.RightChop(LastDot + 1);
						}

						MenuBuilder.AddMenuEntry(
							FText::FromString(ShortName),
							FText::FromString(SlotTag),  // Full tag as tooltip
							FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([this, ValuePtr, SlotTag, ShortName]()
							{
								if (*ValuePtr != SlotTag)
								{
									FString DisplayOld = ValuePtr->IsEmpty() ? TEXT("(None)") : *ValuePtr;
									EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
										FText::Format(NSLOCTEXT("ItemTableEditor", "ConfirmSlotChange", "Change Equipment Slot from '{0}' to '{1}'?\n\nThis will mark the Item as modified."),
											FText::FromString(DisplayOld),
											FText::FromString(ShortName)));

									if (Result == EAppReturnType::Yes)
									{
										*ValuePtr = SlotTag;
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
					SNew(STextBlock)
						.Text_Lambda([ValuePtr]()
						{
							if (ValuePtr->IsEmpty())
							{
								return FText::FromString(TEXT("(None)"));
							}
							// Extract short name for display
							FString ShortName = *ValuePtr;
							int32 LastDot;
							if (ValuePtr->FindLastChar(TEXT('.'), LastDot))
							{
								ShortName = ValuePtr->RightChop(LastDot + 1);
							}
							return FText::FromString(ShortName);
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				]
		];
}

TSharedRef<SWidget> SItemTableRow::CreateAssetDropdownCell(FSoftObjectPath& Value, UClass* AssetClass, const FString& AssetPrefix)
{
	// v4.12.6: Dual-function cell - clickable hyperlink to open asset + dropdown to change reference
	FSoftObjectPath* ValuePtr = &Value;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SHorizontalBox)

			// Clickable hyperlink text - opens asset in editor
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "NoBorder")
					.ContentPadding(FMargin(0))
					.OnClicked_Lambda([ValuePtr]() -> FReply
					{
						if (!ValuePtr->IsNull())
						{
							if (UObject* Asset = ValuePtr->TryLoad())
							{
								GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Asset);
							}
						}
						return FReply::Handled();
					})
					.ToolTipText_Lambda([ValuePtr]()
					{
						if (ValuePtr->IsNull())
						{
							return FText::FromString(TEXT("No asset selected"));
						}
						return FText::Format(NSLOCTEXT("ItemTableEditor", "ClickToOpenAsset", "Click to open: {0}"),
							FText::FromString(ValuePtr->GetAssetPathString()));
					})
					.Cursor(EMouseCursor::Hand)
					[
						SNew(STextBlock)
							.Text_Lambda([ValuePtr]()
							{
								if (ValuePtr->IsNull())
								{
									return FText::FromString(TEXT("(None)"));
								}
								return FText::FromString(TrimAssetPrefix_Item(ValuePtr->GetAssetName()));
							})
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							.ColorAndOpacity_Lambda([ValuePtr]()
							{
								// Blue color for clickable assets, gray for (None)
								if (ValuePtr->IsNull())
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
					.OnGetMenuContent_Lambda([this, ValuePtr, AssetClass, AssetPrefix]() -> TSharedRef<SWidget>
					{
						FMenuBuilder MenuBuilder(true, nullptr);

						// None option
						MenuBuilder.AddMenuEntry(
							NSLOCTEXT("ItemTableEditor", "None", "(None)"),
							FText::GetEmpty(),
							FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([this, ValuePtr]()
							{
								FString CurrentValue = ValuePtr->IsNull() ? TEXT("(None)") : ValuePtr->GetAssetName();
								EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
									FText::Format(NSLOCTEXT("ItemTableEditor", "ConfirmClear", "Clear '{0}' and set to (None)?\n\nThis will mark the Item as modified."),
										FText::FromString(CurrentValue)));

								if (Result == EAppReturnType::Yes)
								{
									ValuePtr->Reset();
									MarkModified();
								}
							}))
						);

						MenuBuilder.AddSeparator();

						// Get available assets - scan by prefix if no class specified
						FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
						TArray<FAssetData> Assets;

						if (AssetClass)
						{
							Registry.Get().GetAssetsByClass(AssetClass->GetClassPathName(), Assets, true);
						}
						else if (!AssetPrefix.IsEmpty())
						{
							// Scan all blueprints and filter by prefix
							Registry.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), Assets, true);
						}

						for (const FAssetData& Asset : Assets)
						{
							FString AssetName = Asset.AssetName.ToString();

							// Filter by prefix if specified
							if (!AssetPrefix.IsEmpty() && !AssetName.StartsWith(AssetPrefix))
							{
								continue;
							}

							FSoftObjectPath AssetPath = Asset.GetSoftObjectPath();

							MenuBuilder.AddMenuEntry(
								FText::FromString(TrimAssetPrefix_Item(AssetName)),
								FText::FromString(Asset.GetObjectPathString()),
								FSlateIcon(),
								FUIAction(FExecuteAction::CreateLambda([this, ValuePtr, AssetPath, AssetName]()
								{
									if (*ValuePtr != AssetPath)
									{
										FString OldValue = ValuePtr->IsNull() ? TEXT("(None)") : TrimAssetPrefix_Item(ValuePtr->GetAssetName());
										EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
											FText::Format(NSLOCTEXT("ItemTableEditor", "ConfirmAssetChange", "Change from '{0}' to '{1}'?\n\nThis will mark the Item as modified."),
												FText::FromString(OldValue),
												FText::FromString(TrimAssetPrefix_Item(AssetName))));

										if (Result == EAppReturnType::Yes)
										{
											*ValuePtr = AssetPath;
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
		];
}

TSharedRef<SWidget> SItemTableRow::CreateModifierGECell()
{
	// v4.12.6: Click-to-open + dropdown filtered to only show equipment modifier GEs + trimmed display
	FSoftObjectPath* ValuePtr = &RowData->ModifierGE;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SHorizontalBox)

			// Clickable hyperlink text - opens GE Blueprint asset in editor
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "NoBorder")
					.ContentPadding(FMargin(0))
					.OnClicked_Lambda([ValuePtr]() -> FReply
					{
						if (!ValuePtr->IsNull())
						{
							// v4.12.6 FIX: Path may point to Generated Class, need to open actual Blueprint
							if (UObject* LoadedAsset = ValuePtr->TryLoad())
							{
								// If it's a class (Blueprint Generated Class), get its source Blueprint
								if (UClass* AsClass = Cast<UClass>(LoadedAsset))
								{
									if (UBlueprint* BP = Cast<UBlueprint>(AsClass->ClassGeneratedBy))
									{
										GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(BP);
									}
								}
								else if (UBlueprint* BP = Cast<UBlueprint>(LoadedAsset))
								{
									// It's already a Blueprint
									GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(BP);
								}
								else
								{
									// Fallback - open whatever was loaded (DataAssets, etc.)
									GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(LoadedAsset);
								}
							}
						}
						return FReply::Handled();
					})
					.ToolTipText_Lambda([ValuePtr]()
					{
						if (ValuePtr->IsNull())
						{
							return FText::FromString(TEXT("No Modifier GE selected"));
						}
						return FText::Format(NSLOCTEXT("ItemTableEditor", "ClickToOpenModifier", "Click to open: {0}"),
							FText::FromString(ValuePtr->GetAssetName()));
					})
					.Cursor(EMouseCursor::Hand)
					[
						SNew(STextBlock)
							.Text_Lambda([ValuePtr]()
							{
								if (ValuePtr->IsNull())
								{
									return FText::FromString(TEXT("(None)"));
								}
								return FText::FromString(TrimAssetPrefix_Item(ValuePtr->GetAssetName()));
							})
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							.ColorAndOpacity_Lambda([ValuePtr]()
							{
								// Blue color for clickable assets, gray for (None)
								if (ValuePtr->IsNull())
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
							NSLOCTEXT("ItemTableEditor", "ModifierNone", "(None)"),
							FText::GetEmpty(),
							FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([this, ValuePtr]()
							{
								if (!ValuePtr->IsNull())
								{
									EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
										FText::Format(NSLOCTEXT("ItemTableEditor", "ConfirmClearModifier", "Clear Modifier GE '{0}'?\n\nThis will mark the Item as modified."),
											FText::FromString(TrimAssetPrefix_Item(ValuePtr->GetAssetName()))));
									if (Result == EAppReturnType::Yes)
									{
										ValuePtr->Reset();
										MarkModified();
									}
								}
							}))
						);

						MenuBuilder.AddSeparator();

						// Get GameplayEffect blueprints and filter for equipment modifiers
						FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
						TArray<FAssetData> Assets;
						Registry.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), Assets, true);

						for (const FAssetData& Asset : Assets)
						{
							FString AssetName = Asset.AssetName.ToString();

							// Must have GE_ prefix
							if (!AssetName.StartsWith(TEXT("GE_")))
							{
								continue;
							}

							// Filter for equipment modifiers:
							// 1. Asset path contains "Equipment" folder
							// 2. OR asset name contains "EquipmentModifier"
							// 3. OR asset is in a GameplayEffects/Equipment path
							FString AssetPathStr = Asset.GetObjectPathString();
							bool bIsEquipmentModifier =
								AssetPathStr.Contains(TEXT("/Equipment/")) ||
								AssetName.Contains(TEXT("EquipmentModifier")) ||
								AssetName.Contains(TEXT("Equipment"));

							if (!bIsEquipmentModifier)
							{
								continue;
							}

							FSoftObjectPath AssetPath = Asset.GetSoftObjectPath();

							MenuBuilder.AddMenuEntry(
								FText::FromString(TrimAssetPrefix_Item(AssetName)),
								FText::FromString(AssetPathStr),
								FSlateIcon(),
								FUIAction(FExecuteAction::CreateLambda([this, ValuePtr, AssetPath, AssetName]()
								{
									if (*ValuePtr != AssetPath)
									{
										FString OldValue = ValuePtr->IsNull() ? TEXT("(None)") : TrimAssetPrefix_Item(ValuePtr->GetAssetName());
										EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
											FText::Format(NSLOCTEXT("ItemTableEditor", "ConfirmModifierChange", "Change Modifier GE from '{0}' to '{1}'?\n\nThis will mark the Item as modified."),
												FText::FromString(OldValue),
												FText::FromString(TrimAssetPrefix_Item(AssetName))));

										if (Result == EAppReturnType::Yes)
										{
											*ValuePtr = AssetPath;
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
		];
}

TSharedRef<SWidget> SItemTableRow::CreateAbilitiesCell()
{
	// v4.12.6: Multi-select dropdown for GA_* abilities with click-to-open for each ability
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SHorizontalBox)

			// Clickable ability list - opens first ability on click
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "NoBorder")
					.ContentPadding(FMargin(0))
					.OnClicked_Lambda([this]() -> FReply
					{
						// On click, open the first ability (if exists)
						if (!RowData->Abilities.IsEmpty())
						{
							TArray<FString> Abilities;
							RowData->Abilities.ParseIntoArray(Abilities, TEXT(","));
							if (Abilities.Num() > 0)
							{
								FString FirstAbility = Abilities[0].TrimStartAndEnd();
								// Find and open the ability blueprint
								FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
								TArray<FAssetData> Assets;
								Registry.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), Assets, true);

								for (const FAssetData& Asset : Assets)
								{
									if (Asset.AssetName.ToString().Equals(FirstAbility, ESearchCase::IgnoreCase))
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
						return FReply::Handled();
					})
					.ToolTipText_Lambda([this]()
					{
						if (RowData->Abilities.IsEmpty())
						{
							return FText::FromString(TEXT("No abilities - use dropdown to add"));
						}
						TArray<FString> Abilities;
						RowData->Abilities.ParseIntoArray(Abilities, TEXT(","));
						if (Abilities.Num() == 1)
						{
							return FText::Format(NSLOCTEXT("ItemTableEditor", "ClickToOpenAbility", "Click to open: {0}"),
								FText::FromString(Abilities[0].TrimStartAndEnd()));
						}
						return FText::Format(NSLOCTEXT("ItemTableEditor", "ClickToOpenFirstAbility", "Click to open first ability: {0}\n({1} total abilities)"),
							FText::FromString(Abilities[0].TrimStartAndEnd()),
							FText::AsNumber(Abilities.Num()));
					})
					.Cursor(EMouseCursor::Hand)
					[
						SNew(STextBlock)
							.Text_Lambda([this]()
							{
								if (RowData->Abilities.IsEmpty())
								{
									return FText::FromString(TEXT("(None)"));
								}
								// Trim GA_ prefix from displayed abilities
								TArray<FString> Abilities;
								RowData->Abilities.ParseIntoArray(Abilities, TEXT(","));
								TArray<FString> TrimmedAbilities;
								for (const FString& Ability : Abilities)
								{
									TrimmedAbilities.Add(TrimAssetPrefix_Item(Ability.TrimStartAndEnd()));
								}
								return FText::FromString(FString::Join(TrimmedAbilities, TEXT(", ")));
							})
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							.ColorAndOpacity_Lambda([this]()
							{
								// Blue color for clickable assets, gray for (None)
								if (RowData->Abilities.IsEmpty())
								{
									return FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f));
								}
								return FSlateColor(FLinearColor(0.3f, 0.5f, 0.85f));
							})
					]
			]

			// Dropdown arrow - shows multi-select menu with click-to-open per ability
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SComboButton)
					.HasDownArrow(true)
					.ButtonStyle(FAppStyle::Get(), "NoBorder")
					.ContentPadding(FMargin(2.0f, 0.0f))
					.OnGetMenuContent_Lambda([this]() -> TSharedRef<SWidget>
					{
						TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox);

						// Clear All button
						MenuContent->AddSlot()
						.AutoHeight()
						.Padding(4.0f, 2.0f)
						[
							SNew(SButton)
								.Text(NSLOCTEXT("ItemTableEditor", "ClearAllAbilities", "(Clear All)"))
								.OnClicked_Lambda([this]()
								{
									if (!RowData->Abilities.IsEmpty())
									{
										EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
											FText::Format(NSLOCTEXT("ItemTableEditor", "ConfirmClearAbilities", "Clear all abilities?\n\nCurrent: {0}"),
												FText::FromString(RowData->Abilities)));
										if (Result == EAppReturnType::Yes)
										{
											RowData->Abilities.Empty();
											MarkModified();
										}
									}
									return FReply::Handled();
								})
						];

						MenuContent->AddSlot()
						.AutoHeight()
						.Padding(4.0f, 0.0f)
						[
							SNew(SSeparator)
						];

						// Get GameplayAbility blueprints (GA_*)
						FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
						TArray<FAssetData> Assets;
						Registry.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), Assets, true);

						for (const FAssetData& Asset : Assets)
						{
							FString AssetName = Asset.AssetName.ToString();
							FSoftObjectPath AssetPath = Asset.GetSoftObjectPath();

							// Must have GA_ prefix (GameplayAbility)
							if (!AssetName.StartsWith(TEXT("GA_")))
							{
								continue;
							}

							MenuContent->AddSlot()
							.AutoHeight()
							.Padding(4.0f, 1.0f)
							[
								SNew(SHorizontalBox)

								// Checkbox for selection
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(SCheckBox)
										.IsChecked_Lambda([this, AssetName]()
										{
											TArray<FString> CurrentAbilities;
											RowData->Abilities.ParseIntoArray(CurrentAbilities, TEXT(","));
											for (FString& Ability : CurrentAbilities) { Ability = Ability.TrimStartAndEnd(); }
											return CurrentAbilities.Contains(AssetName) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
										})
										.OnCheckStateChanged_Lambda([this, AssetName](ECheckBoxState NewState)
										{
											TArray<FString> Abilities;
											RowData->Abilities.ParseIntoArray(Abilities, TEXT(","));
											for (FString& Ability : Abilities) { Ability = Ability.TrimStartAndEnd(); }

											// Check if already exists
											bool bAlreadyExists = false;
											int32 ExistingIndex = INDEX_NONE;
											for (int32 i = 0; i < Abilities.Num(); i++)
											{
												if (Abilities[i].Equals(AssetName, ESearchCase::IgnoreCase))
												{
													bAlreadyExists = true;
													ExistingIndex = i;
													break;
												}
											}

											if (NewState == ECheckBoxState::Checked)
											{
												if (!bAlreadyExists)
												{
													EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
														FText::Format(NSLOCTEXT("ItemTableEditor", "ConfirmAddAbility", "Add ability '{0}' to this item?\n\nThis will mark the Item as modified."),
															FText::FromString(TrimAssetPrefix_Item(AssetName))));
													if (Result == EAppReturnType::Yes)
													{
														Abilities.Add(AssetName);
														RowData->Abilities = FString::Join(Abilities, TEXT(", "));
														MarkModified();
													}
												}
											}
											else
											{
												if (bAlreadyExists)
												{
													EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
														FText::Format(NSLOCTEXT("ItemTableEditor", "ConfirmRemoveAbility", "Remove ability '{0}' from this item?\n\nThis will mark the Item as modified."),
															FText::FromString(TrimAssetPrefix_Item(AssetName))));
													if (Result == EAppReturnType::Yes)
													{
														Abilities.RemoveAt(ExistingIndex);
														RowData->Abilities = FString::Join(Abilities, TEXT(", "));
														MarkModified();
													}
												}
											}
										})
								]

								// Clickable ability name - opens asset
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.VAlign(VAlign_Center)
								.Padding(4.0f, 0.0f, 0.0f, 0.0f)
								[
									SNew(SButton)
										.ButtonStyle(FAppStyle::Get(), "NoBorder")
										.ContentPadding(FMargin(0))
										.OnClicked_Lambda([AssetPath]() -> FReply
										{
											if (UObject* LoadedAsset = AssetPath.TryLoad())
											{
												GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(LoadedAsset);
											}
											return FReply::Handled();
										})
										.ToolTipText(FText::Format(NSLOCTEXT("ItemTableEditor", "ClickToOpenAbilityInMenu", "Click to open: {0}"),
											FText::FromString(AssetName)))
										.Cursor(EMouseCursor::Hand)
										[
											SNew(STextBlock)
												.Text(FText::FromString(TrimAssetPrefix_Item(AssetName)))
												.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
												.ColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.5f, 0.85f)))
										]
								]
							];
						}

						return SNew(SBox)
							.MaxDesiredHeight(400.0f)
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
						SNew(SBox)
							.WidthOverride(12.0f)
							.HeightOverride(12.0f)
					]
			]
		];
}

TSharedRef<SWidget> SItemTableRow::CreateTokenCell(FString& Value, const FString& Hint)
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
								FText::Format(NSLOCTEXT("ItemTableEditor", "ConfirmTokenChange", "Change '{0}' from '{1}' to '{2}'?\n\nThis will mark the Item as modified."),
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

TSharedRef<SWidget> SItemTableRow::CreateNotesCell()
{
	// v4.12.5: Match NPC Table cell styling with confirmation dialog
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
								FText::Format(NSLOCTEXT("ItemTableEditor", "ConfirmNotesChange", "Change Notes from '{0}' to '{1}'?\n\nThis will mark the Item as modified."),
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

void SItemTableRow::MarkModified()
{
	if (RowData.IsValid())
	{
		RowData->Status = EItemTableRowStatus::Modified;
		RowData->InvalidateValidation();
	}
	OnRowModified.ExecuteIfBound();
}

//=============================================================================
// SItemTableEditor
//=============================================================================

void SItemTableEditor::Construct(const FArguments& InArgs)
{
	TableData = InArgs._TableData;
	OnDirtyStateChanged = InArgs._OnDirtyStateChanged;
	TransactionStack = MakeShared<FTableEditorTransactionStack>(50);  // v7.2: Initialize undo/redo stack

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

		// v7.2: Find & Replace bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f, 0.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("FindLabel", "Find:"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SAssignNew(FindTextBox, SEditableTextBox)
				.MinDesiredWidth(150.0f)
				.HintText(LOCTEXT("FindHint", "Search text..."))
				.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type) {
					FindText = Text.ToString();
					PerformSearch();
				})
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("FindPrev", "<"))
				.ToolTipText(LOCTEXT("FindPrevTip", "Find Previous"))
				.OnClicked(this, &SItemTableEditor::OnFindPrevClicked)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("FindNext", ">"))
				.ToolTipText(LOCTEXT("FindNextTip", "Find Next"))
				.OnClicked(this, &SItemTableEditor::OnFindNextClicked)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(10.0f, 2.0f, 2.0f, 2.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ReplaceLabel", "Replace:"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SAssignNew(ReplaceTextBox, SEditableTextBox)
				.MinDesiredWidth(150.0f)
				.HintText(LOCTEXT("ReplaceHint", "Replace with..."))
				.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type) {
					ReplaceText = Text.ToString();
				})
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Replace", "Replace"))
				.OnClicked(this, &SItemTableEditor::OnReplaceClicked)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ReplaceAll", "Replace All"))
				.OnClicked(this, &SItemTableEditor::OnReplaceAllClicked)
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			]
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
				SAssignNew(ListView, SListView<TSharedPtr<FItemTableRow>>)
				.ListItemsSource(&DisplayedRows)
				.OnGenerateRow(this, &SItemTableEditor::OnGenerateRow)
				.OnSelectionChanged(this, &SItemTableEditor::OnSelectionChanged)
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

TSharedRef<SWidget> SItemTableEditor::BuildToolbar()
{
	return SNew(SHorizontalBox)

		// LEFT SIDE - Data actions
		// Add Item
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("AddItem", "+ Item"))
			.OnClicked(this, &SItemTableEditor::OnAddRowClicked)
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
		]

		// Duplicate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Duplicate", "Duplicate"))
			.OnClicked(this, &SItemTableEditor::OnDuplicateRowClicked)
		]

		// Delete
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Delete", "Delete"))
			.OnClicked(this, &SItemTableEditor::OnDeleteRowsClicked)
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
		]

		// v7.2: Undo
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Undo", "Undo"))
			.OnClicked(this, &SItemTableEditor::OnUndoClicked)
			.ToolTipText_Lambda([this]() -> FText {
				if (TransactionStack.IsValid() && TransactionStack->CanUndo())
				{
					return FText::Format(LOCTEXT("UndoTipFormat", "Undo: {0} (Ctrl+Z)"),
						FText::FromString(TransactionStack->GetUndoDescription()));
				}
				return LOCTEXT("UndoTipEmpty", "Nothing to undo (Ctrl+Z)");
			})
			.IsEnabled(this, &SItemTableEditor::CanUndo)
		]

		// v7.2: Redo
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Redo", "Redo"))
			.OnClicked(this, &SItemTableEditor::OnRedoClicked)
			.ToolTipText_Lambda([this]() -> FText {
				if (TransactionStack.IsValid() && TransactionStack->CanRedo())
				{
					return FText::Format(LOCTEXT("RedoTipFormat", "Redo: {0} (Ctrl+Y)"),
						FText::FromString(TransactionStack->GetRedoDescription()));
				}
				return LOCTEXT("RedoTipEmpty", "Nothing to redo (Ctrl+Y)");
			})
			.IsEnabled(this, &SItemTableEditor::CanRedo)
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
			.OnClicked(this, &SItemTableEditor::OnClearFiltersClicked)
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
			.OnClicked(this, &SItemTableEditor::OnValidateClicked)
			.ToolTipText(LOCTEXT("ValidateTip", "Validate all rows for errors and warnings"))
			.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Generate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Generate", "Generate Items"))
			.OnClicked(this, &SItemTableEditor::OnGenerateClicked)
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
			.OnClicked(this, &SItemTableEditor::OnSyncFromAssetsClicked)
			.ToolTipText(LOCTEXT("SyncTip", "Populate table from existing Item assets"))
			.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Export XLSX
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("ExportXLSX", "Export XLSX"))
			.OnClicked(this, &SItemTableEditor::OnExportXLSXClicked)
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
			.OnClicked(this, &SItemTableEditor::OnImportXLSXClicked)
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
			.OnClicked(this, &SItemTableEditor::OnSyncXLSXClicked)
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
			.OnClicked(this, &SItemTableEditor::OnSaveClicked)
			.ToolTipText(LOCTEXT("SaveTooltip", "Save the table data container"))
		]

		// Apply to Assets
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("ApplyToAssets", "Apply to Items"))
			.OnClicked(this, &SItemTableEditor::OnApplyToAssetsClicked)
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
			.ToolTipText(LOCTEXT("ApplyToAssetsTooltip", "Write changes back to Item assets"))
			.IsEnabled_Lambda([this]() { return !bIsBusy; })
		];
}

TSharedRef<SHeaderRow> SItemTableEditor::BuildHeaderRow()
{
	TSharedRef<SHeaderRow> Header = SNew(SHeaderRow);
	TArray<FItemTableColumn> Columns = GetItemTableColumns();

	for (const FItemTableColumn& Col : Columns)
	{
		Header->AddColumn(
			SHeaderRow::Column(Col.ColumnId)
			.DefaultLabel(Col.DisplayName)
			.ManualWidth(Col.ManualWidth)  // v4.12.5: Fixed pixel width
			.SortMode(this, &SItemTableEditor::GetColumnSortMode, Col.ColumnId)
			.OnSort(this, &SItemTableEditor::OnColumnSortModeChanged)
			.HeaderContent()
			[
				BuildColumnHeaderContent(Col)
			]
		);
	}

	HeaderRow = Header;
	return Header;
}

TSharedRef<SWidget> SItemTableEditor::BuildColumnHeaderContent(const FItemTableColumn& Col)
{
	FItemColumnFilterState& FilterState = ColumnFilters.FindOrAdd(Col.ColumnId);

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
				FItemColumnFilterState* State = ColumnFilters.Find(ColumnId);
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
							FItemColumnFilterState* S = ColumnFilters.Find(ColumnId);
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
							FItemColumnFilterState* S = ColumnFilters.Find(ColumnId);
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
							FItemColumnFilterState* S = ColumnFilters.Find(ColumnId);
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
					FItemColumnFilterState* State = ColumnFilters.Find(ColumnId);
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

TSharedRef<SWidget> SItemTableEditor::BuildStatusBar()
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
				SAssignNew(StatusByTypeText, STextBlock)
				.Text(LOCTEXT("StatusByType", ""))
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

void SItemTableEditor::UpdateStatusBar()
{
	if (!TableData) return;

	int32 TotalRows = 0;
	int32 EquipCount = 0, RangedCount = 0, MeleeCount = 0, ConsumCount = 0;
	int32 ValidationErrors = 0;

	for (const FItemTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			TotalRows++;
			switch (Row.ItemType)
			{
				case EItemType::Equippable: EquipCount++; break;
				case EItemType::RangedWeapon: RangedCount++; break;
				case EItemType::MeleeWeapon: MeleeCount++; break;
				case EItemType::Consumable: ConsumCount++; break;
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
	if (StatusByTypeText.IsValid())
	{
		FString TypeStr = FString::Printf(TEXT("E:%d R:%d M:%d C:%d"), EquipCount, RangedCount, MeleeCount, ConsumCount);
		StatusByTypeText->SetText(FText::FromString(TypeStr));
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

void SItemTableEditor::InitializeColumnFilters()
{
	TArray<FItemTableColumn> Columns = GetItemTableColumns();
	for (const FItemTableColumn& Col : Columns)
	{
		ColumnFilters.Add(Col.ColumnId, FItemColumnFilterState());
	}
}

void SItemTableEditor::SyncFromTableData()
{
	AllRows.Empty();
	if (!TableData) return;

	for (FItemTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			AllRows.Add(MakeShared<FItemTableRow>(Row));
		}
	}
}

void SItemTableEditor::RefreshList()
{
	SyncFromTableData();
	ApplyFilters();
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
	UpdateStatusBar();
}

void SItemTableEditor::SetTableData(UItemTableData* InTableData)
{
	TableData = InTableData;
	RefreshList();
}

TArray<TSharedPtr<FItemTableRow>> SItemTableEditor::GetSelectedRows() const
{
	TArray<TSharedPtr<FItemTableRow>> Selected;
	if (ListView.IsValid())
	{
		Selected = ListView->GetSelectedItems();
	}
	return Selected;
}

TSharedRef<ITableRow> SItemTableEditor::OnGenerateRow(TSharedPtr<FItemTableRow> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SItemTableRow, OwnerTable)
		.RowData(Item)
		.OnRowModified(FSimpleDelegate::CreateSP(this, &SItemTableEditor::OnRowModified));
}

void SItemTableEditor::OnSelectionChanged(TSharedPtr<FItemTableRow> Item, ESelectInfo::Type SelectInfo)
{
	UpdateStatusBar();
}

EColumnSortMode::Type SItemTableEditor::GetColumnSortMode(FName ColumnId) const
{
	if (SortColumn == ColumnId) return SortMode;
	return EColumnSortMode::None;
}

void SItemTableEditor::OnColumnSortModeChanged(EColumnSortPriority::Type Priority, const FName& ColumnId, EColumnSortMode::Type InSortMode)
{
	SortColumn = ColumnId;
	SortMode = InSortMode;
	ApplySorting();
}

void SItemTableEditor::OnColumnTextFilterChanged(FName ColumnId, const FText& NewText)
{
	if (FItemColumnFilterState* State = ColumnFilters.Find(ColumnId))
	{
		State->TextFilter = NewText.ToString();
		ApplyFilters();
	}
}

void SItemTableEditor::OnColumnDropdownFilterChanged(FName ColumnId, const FString& Value, bool bIsSelected)
{
	FItemColumnFilterState* State = ColumnFilters.Find(ColumnId);
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

			// v4.12.7 FIX: If removing the last item, set __NONE_SELECTED__ instead of empty
			// Empty means "show all", but we want "show none" when user deselects the last item
			if (State->SelectedValues.Num() == 0)
			{
				State->SelectedValues.Add(TEXT("__NONE_SELECTED__"));
			}
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

TArray<FString> SItemTableEditor::GetUniqueColumnValues(FName ColumnId) const
{
	// Filter dropdowns show values from the current table data (AllRows)
	TSet<FString> UniqueSet;

	for (const TSharedPtr<FItemTableRow>& Row : AllRows)
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

FReply SItemTableEditor::OnClearFiltersClicked()
{
	for (auto& Pair : ColumnFilters)
	{
		Pair.Value.TextFilter.Empty();
		Pair.Value.SelectedValues.Empty();
	}
	ApplyFilters();
	return FReply::Handled();
}

FString SItemTableEditor::GetColumnValue(const TSharedPtr<FItemTableRow>& Row, FName ColumnId) const
{
	if (!Row.IsValid()) return TEXT("");

	const FItemTableRow& Data = *Row;

	if (ColumnId == TEXT("Status")) return Data.GetStatusString();
	if (ColumnId == TEXT("ItemName")) return Data.ItemName;
	if (ColumnId == TEXT("DisplayName")) return Data.DisplayName;
	if (ColumnId == TEXT("ItemType")) return Data.GetItemTypeString();
	if (ColumnId == TEXT("EquipmentSlot")) return Data.EquipmentSlot;
	if (ColumnId == TEXT("BaseValue")) return FString::FromInt(Data.BaseValue);
	if (ColumnId == TEXT("Weight")) return FString::SanitizeFloat(Data.Weight);
	if (ColumnId == TEXT("AttackRating")) return FString::SanitizeFloat(Data.AttackRating);
	if (ColumnId == TEXT("ArmorRating")) return FString::SanitizeFloat(Data.ArmorRating);
	if (ColumnId == TEXT("ModifierGE")) return Data.ModifierGE.GetAssetName();
	if (ColumnId == TEXT("Abilities")) return Data.Abilities;
	// Weapon properties
	if (ColumnId == TEXT("AttackDamage")) return FString::SanitizeFloat(Data.AttackDamage);
	if (ColumnId == TEXT("ClipSize")) return FString::FromInt(Data.ClipSize);
	if (ColumnId == TEXT("WeaponHand")) return Data.WeaponHand;
	if (ColumnId == TEXT("HeavyAttackDamageMultiplier")) return FString::SanitizeFloat(Data.HeavyAttackDamageMultiplier);
	if (ColumnId == TEXT("bAllowManualReload")) return Data.bAllowManualReload ? TEXT("Yes") : TEXT("No");
	if (ColumnId == TEXT("BotAttackRange")) return FString::SanitizeFloat(Data.BotAttackRange);
	// Ranged properties
	if (ColumnId == TEXT("BaseSpreadDegrees")) return FString::SanitizeFloat(Data.BaseSpreadDegrees);
	if (ColumnId == TEXT("MaxSpreadDegrees")) return FString::SanitizeFloat(Data.MaxSpreadDegrees);
	if (ColumnId == TEXT("AimFOVPct")) return FString::SanitizeFloat(Data.AimFOVPct);
	if (ColumnId == TEXT("ItemTags")) return Data.ItemTags;
	if (ColumnId == TEXT("bStackable")) return Data.bStackable ? TEXT("Yes") : TEXT("No");
	if (ColumnId == TEXT("MaxStackSize")) return FString::FromInt(Data.MaxStackSize);
	if (ColumnId == TEXT("Notes")) return Data.Notes;

	return TEXT("");
}

void SItemTableEditor::ApplyFilters()
{
	DisplayedRows.Empty();

	for (const TSharedPtr<FItemTableRow>& Row : AllRows)
	{
		bool bPassesFilter = true;

		for (const auto& Pair : ColumnFilters)
		{
			const FItemColumnFilterState& State = Pair.Value;
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

void SItemTableEditor::ApplySorting()
{
	if (SortColumn.IsNone() || SortMode == EColumnSortMode::None)
	{
		return;
	}

	DisplayedRows.Sort([this](const TSharedPtr<FItemTableRow>& A, const TSharedPtr<FItemTableRow>& B) {
		FString ValueA = GetColumnValue(A, SortColumn);
		FString ValueB = GetColumnValue(B, SortColumn);

		if (SortMode == EColumnSortMode::Ascending)
		{
			return ValueA < ValueB;
		}
		return ValueA > ValueB;
	});
}

FReply SItemTableEditor::OnAddRowClicked()
{
	if (!TableData) return FReply::Handled();

	// Create new row with undo support
	TSharedPtr<FItemTableRow> NewRow = MakeShared<FItemTableRow>();
	NewRow->RowId = FGuid::NewGuid();
	NewRow->ItemName = TEXT("NewItem");
	NewRow->ItemType = EItemType::Equippable;
	NewRow->Status = EItemTableRowStatus::New;

	AddRowWithUndo(NewRow);
	MarkDirty();
	RefreshList();
	return FReply::Handled();
}

FReply SItemTableEditor::OnDeleteRowsClicked()
{
	if (!TableData) return FReply::Handled();

	TArray<TSharedPtr<FItemTableRow>> Selected = GetSelectedRows();
	if (Selected.Num() == 0) return FReply::Handled();

	// Use undo-aware delete
	DeleteRowsWithUndo(Selected);
	MarkDirty();
	RefreshList();
	return FReply::Handled();
}

FReply SItemTableEditor::OnDuplicateRowClicked()
{
	if (!TableData) return FReply::Handled();

	TArray<TSharedPtr<FItemTableRow>> Selected = GetSelectedRows();
	if (Selected.Num() == 0) return FReply::Handled();

	for (const TSharedPtr<FItemTableRow>& Row : Selected)
	{
		if (Row.IsValid())
		{
			// Create duplicate with undo support
			TSharedPtr<FItemTableRow> NewRow = MakeShared<FItemTableRow>(*Row);
			NewRow->RowId = FGuid::NewGuid();
			NewRow->ItemName += TEXT("_Copy");
			NewRow->Status = EItemTableRowStatus::New;
			NewRow->GeneratedItem.Reset();
			NewRow->InvalidateValidation();

			AddRowWithUndo(NewRow);
		}
	}

	MarkDirty();
	RefreshList();
	return FReply::Handled();
}

FReply SItemTableEditor::OnValidateClicked()
{
	if (!TableData) return FReply::Handled();

	FItemValidationResult Result = FItemTableValidator::ValidateAllAndCache(TableData->Rows, TableData->ListsVersionGuid);

	RefreshList();

	FString Summary = FString::Printf(TEXT("Validation complete: %d errors, %d warnings"),
		Result.GetErrorCount(), Result.GetWarningCount());

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Summary));

	return FReply::Handled();
}

FReply SItemTableEditor::OnGenerateClicked()
{
	if (!TableData || bIsBusy) return FReply::Handled();

	bIsBusy = true;

	// Validate first
	FItemValidationResult ValidationResult = FItemTableValidator::ValidateAllAndCache(TableData->Rows, TableData->ListsVersionGuid);
	if (ValidationResult.HasErrors())
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("ValidationErrors", "Cannot generate: Please fix validation errors first."));
		bIsBusy = false;
		RefreshList();
		return FReply::Handled();
	}

	// Convert to manifest definitions
	TArray<FManifestEquippableItemDefinition> Definitions = FItemTableConverter::ConvertRowsToManifest(
		TableData->Rows, TableData->OutputFolder);

	int32 Generated = 0;
	int32 Failed = 0;

	for (const FManifestEquippableItemDefinition& Def : Definitions)
	{
		FGenerationResult Result = FEquippableItemGenerator::Generate(Def);
		if (Result.Status == EGenerationStatus::New)
		{
			Generated++;
		}
		else if (Result.Status == EGenerationStatus::Failed)
		{
			Failed++;
			UE_LOG(LogTemp, Warning, TEXT("Failed to generate item %s: %s"), *Def.Name, *Result.Message);
		}
	}

	// Update generation tracking
	TableData->OnGenerationComplete(Failed);

	// Mark rows as synced
	for (FItemTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			Row.Status = EItemTableRowStatus::Synced;
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

FReply SItemTableEditor::OnSyncFromAssetsClicked()
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
	UE_LOG(LogTemp, Log, TEXT("[ItemTableEditor] Rescanning Asset Registry for paths: %s"), *FString::Join(PathsToScan, TEXT(", ")));
	AssetRegistry.ScanPathsSynchronous(PathsToScan, true /* bForceRescan */);

	// v4.12.5: Search for Blueprint assets whose parent class is UNarrativeItem or subclass
	// NarrativeItem is a UObject, not UDataAsset, so items are stored as Blueprint assets
	TArray<FAssetData> AllBlueprints;
	AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), AllBlueprints, true);

	// Filter blueprints by parent class - look for Item class hierarchy
	TArray<FAssetData> AssetList;
	for (const FAssetData& Asset : AllBlueprints)
	{
		FAssetDataTagMapSharedView::FFindTagResult ParentClassTag = Asset.TagsAndValues.FindTag(FBlueprintTags::ParentClassPath);
		if (ParentClassTag.IsSet())
		{
			FString ParentClassPath = ParentClassTag.GetValue();

			// Check if parent class is in the item class hierarchy
			if (ParentClassPath.Contains(TEXT("NarrativeItem")) ||
				ParentClassPath.Contains(TEXT("EquippableItem")) ||
				ParentClassPath.Contains(TEXT("WeaponItem")) ||
				ParentClassPath.Contains(TEXT("RangedWeaponItem")) ||
				ParentClassPath.Contains(TEXT("MeleeWeaponItem")) ||
				ParentClassPath.Contains(TEXT("MagicWeaponItem")) ||
				ParentClassPath.Contains(TEXT("ThrowableWeaponItem")) ||
				ParentClassPath.Contains(TEXT("AmmoItem")) ||
				ParentClassPath.Contains(TEXT("GameplayEffectItem")) ||
				ParentClassPath.Contains(TEXT("WeaponAttachmentItem")))
			{
				AssetList.Add(Asset);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[ItemTableEditor] Found %d Item Blueprint assets (from %d total blueprints)"), AssetList.Num(), AllBlueprints.Num());

	if (AssetList.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoItemsFound", "No Item Blueprint assets found in the project.\n\nCreate Item assets first, then sync."));
		return FReply::Handled();
	}

	// Preserve existing Notes values before clearing rows (user-entered, not stored in Item asset)
	TMap<FString, FString> PreservedNotes;
	for (const FItemTableRow& ExistingRow : TableData->Rows)
	{
		if (!ExistingRow.Notes.IsEmpty())
		{
			PreservedNotes.Add(ExistingRow.ItemName, ExistingRow.Notes);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[ItemTableEditor] Preserved %d Notes before sync"), PreservedNotes.Num());

	// Clear existing rows and populate from assets
	TableData->Rows.Empty();
	int32 SyncedCount = 0;

	for (const FAssetData& AssetData : AssetList)
	{
		// v4.12.5: Load Blueprint and get CDO (Class Default Object) to read properties
		UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
		if (!Blueprint || !Blueprint->GeneratedClass)
		{
			continue;
		}

		UNarrativeItem* Item = Cast<UNarrativeItem>(Blueprint->GeneratedClass->GetDefaultObject());
		if (!Item)
		{
			continue;
		}

		FItemTableRow& Row = TableData->AddRow();

		// Core Identity
		Row.ItemName = AssetData.AssetName.ToString();

		// Get DisplayName via reflection (it's a protected FText property)
		if (FProperty* DisplayNameProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("DisplayName")))
		{
			FText* DisplayNamePtr = DisplayNameProp->ContainerPtrToValuePtr<FText>(Item);
			if (DisplayNamePtr)
			{
				Row.DisplayName = DisplayNamePtr->ToString();
			}
		}

		// Determine ItemType based on class hierarchy (most specific first)
		// v4.12.5: Added checks for non-EquippableItem types (Ammo, Consumable, etc.)
		if (Item->IsA(URangedWeaponItem::StaticClass()))
		{
			Row.ItemType = EItemType::RangedWeapon;
		}
		else if (Item->IsA(UMeleeWeaponItem::StaticClass()))
		{
			Row.ItemType = EItemType::MeleeWeapon;
		}
		else if (Item->IsA(UMagicWeaponItem::StaticClass()))
		{
			Row.ItemType = EItemType::MagicWeapon;
		}
		else if (Item->IsA(UThrowableWeaponItem::StaticClass()))
		{
			Row.ItemType = EItemType::ThrowableWeapon;
		}
		else if (Item->IsA(UEquippableItem_Clothing::StaticClass()))
		{
			Row.ItemType = EItemType::Clothing;
		}
		else if (Item->IsA(UAmmoItem::StaticClass()))
		{
			Row.ItemType = EItemType::Ammo;
		}
		else if (Item->IsA(UGameplayEffectItem::StaticClass()))
		{
			Row.ItemType = EItemType::Consumable;
		}
		else if (Item->IsA(UWeaponAttachmentItem::StaticClass()))
		{
			Row.ItemType = EItemType::WeaponAttachment;
		}
		else if (Item->IsA(UEquippableItem::StaticClass()))
		{
			Row.ItemType = EItemType::Equippable;
		}
		else
		{
			// Generic NarrativeItem that doesn't fit any specific category
			Row.ItemType = EItemType::Consumable;  // Default for non-equippable items
		}

		// Equipment slot - get first slot from EquippableSlots container
		// v4.12.5: Only access EquippableItem properties if item actually inherits from it
		// (AmmoItem, GameplayEffectItem inherit directly from NarrativeItem, not EquippableItem)
		if (Item->IsA(UEquippableItem::StaticClass()))
		{
			if (FProperty* SlotsProperty = UEquippableItem::StaticClass()->FindPropertyByName(TEXT("EquippableSlots")))
			{
				FGameplayTagContainer* Slots = SlotsProperty->ContainerPtrToValuePtr<FGameplayTagContainer>(Item);
				if (Slots && Slots->Num() > 0)
				{
					TArray<FGameplayTag> TagArray;
					Slots->GetGameplayTagArray(TagArray);
					if (TagArray.Num() > 0)
					{
						Row.EquipmentSlot = TagArray[0].ToString();
					}
				}
			}
		}

		// Base value and weight from NarrativeItem base class
		if (FProperty* ValueProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("BaseValue")))
		{
			int32* Value = ValueProp->ContainerPtrToValuePtr<int32>(Item);
			if (Value)
			{
				Row.BaseValue = *Value;
			}
		}

		if (FProperty* WeightProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("Weight")))
		{
			float* Weight = WeightProp->ContainerPtrToValuePtr<float>(Item);
			if (Weight)
			{
				Row.Weight = *Weight;
			}
		}

		// Description from NarrativeItem
		if (FProperty* DescProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("Description")))
		{
			FText* DescPtr = DescProp->ContainerPtrToValuePtr<FText>(Item);
			if (DescPtr)
			{
				Row.Description = DescPtr->ToString();
			}
		}

		// BaseScore from NarrativeItem
		if (FProperty* ScoreProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("BaseScore")))
		{
			float* Score = ScoreProp->ContainerPtrToValuePtr<float>(Item);
			if (Score)
			{
				Row.BaseScore = *Score;
			}
		}

		// Combat stats via reflection (protected properties on UEquippableItem)
		// v4.12.5: Only access if item inherits from EquippableItem
		if (Item->IsA(UEquippableItem::StaticClass()))
		{
			if (FProperty* AttackProp = UEquippableItem::StaticClass()->FindPropertyByName(TEXT("AttackRating")))
			{
				float* Attack = AttackProp->ContainerPtrToValuePtr<float>(Item);
				if (Attack)
				{
					Row.AttackRating = *Attack;
				}
			}

			if (FProperty* ArmorProp = UEquippableItem::StaticClass()->FindPropertyByName(TEXT("ArmorRating")))
			{
				float* Armor = ArmorProp->ContainerPtrToValuePtr<float>(Item);
				if (Armor)
				{
					Row.ArmorRating = *Armor;
				}
			}

			if (FProperty* StealthProp = UEquippableItem::StaticClass()->FindPropertyByName(TEXT("StealthRating")))
			{
				float* Stealth = StealthProp->ContainerPtrToValuePtr<float>(Item);
				if (Stealth)
				{
					Row.StealthRating = *Stealth;
				}
			}
		}

		// Weapon stats (if this is a weapon)
		// v4.12.5: Use Item->IsA() for safe property access
		if (Item->IsA(UWeaponItem::StaticClass()))
		{
			if (FProperty* DamageProp = UWeaponItem::StaticClass()->FindPropertyByName(TEXT("AttackDamage")))
			{
				float* Damage = DamageProp->ContainerPtrToValuePtr<float>(Item);
				if (Damage)
				{
					Row.AttackDamage = *Damage;
				}
			}

			if (FProperty* HeavyMultProp = UWeaponItem::StaticClass()->FindPropertyByName(TEXT("HeavyAttackDamageMultiplier")))
			{
				float* Mult = HeavyMultProp->ContainerPtrToValuePtr<float>(Item);
				if (Mult)
				{
					Row.HeavyAttackDamageMultiplier = *Mult;
				}
			}

			if (FProperty* HandProp = UWeaponItem::StaticClass()->FindPropertyByName(TEXT("WeaponHand")))
			{
				uint8* HandValue = HandProp->ContainerPtrToValuePtr<uint8>(Item);
				if (HandValue)
				{
					switch (*HandValue)
					{
						case 0: Row.WeaponHand = TEXT("TwoHanded"); break;
						case 1: Row.WeaponHand = TEXT("MainHand"); break;
						case 2: Row.WeaponHand = TEXT("OffHand"); break;
						case 3: Row.WeaponHand = TEXT("DualWieldable"); break;
						default: Row.WeaponHand = TEXT("TwoHanded"); break;
					}
				}
			}

			if (FProperty* ClipProp = UWeaponItem::StaticClass()->FindPropertyByName(TEXT("ClipSize")))
			{
				int32* Clip = ClipProp->ContainerPtrToValuePtr<int32>(Item);
				if (Clip)
				{
					Row.ClipSize = *Clip;
				}
			}

			if (FProperty* ReloadProp = UWeaponItem::StaticClass()->FindPropertyByName(TEXT("bAllowManualReload")))
			{
				bool* Reload = ReloadProp->ContainerPtrToValuePtr<bool>(Item);
				if (Reload)
				{
					Row.bAllowManualReload = *Reload;
				}
			}

			if (FProperty* RangeProp = UWeaponItem::StaticClass()->FindPropertyByName(TEXT("BotAttackRange")))
			{
				float* Range = RangeProp->ContainerPtrToValuePtr<float>(Item);
				if (Range)
				{
					Row.BotAttackRange = *Range;
				}
			}
		}

		// Ranged weapon stats
		// v4.12.5: Use Item->IsA() for safe property access
		if (Item->IsA(URangedWeaponItem::StaticClass()))
		{
			if (FProperty* SpreadProp = URangedWeaponItem::StaticClass()->FindPropertyByName(TEXT("BaseSpreadDegrees")))
			{
				float* Spread = SpreadProp->ContainerPtrToValuePtr<float>(Item);
				if (Spread)
				{
					Row.BaseSpreadDegrees = *Spread;
				}
			}

			if (FProperty* MaxSpreadProp = URangedWeaponItem::StaticClass()->FindPropertyByName(TEXT("MaxSpreadDegrees")))
			{
				float* MaxSpread = MaxSpreadProp->ContainerPtrToValuePtr<float>(Item);
				if (MaxSpread)
				{
					Row.MaxSpreadDegrees = *MaxSpread;
				}
			}

			if (FProperty* BumpProp = URangedWeaponItem::StaticClass()->FindPropertyByName(TEXT("SpreadFireBump")))
			{
				float* Bump = BumpProp->ContainerPtrToValuePtr<float>(Item);
				if (Bump)
				{
					Row.SpreadFireBump = *Bump;
				}
			}

			if (FProperty* DecProp = URangedWeaponItem::StaticClass()->FindPropertyByName(TEXT("SpreadDecreaseSpeed")))
			{
				float* Dec = DecProp->ContainerPtrToValuePtr<float>(Item);
				if (Dec)
				{
					Row.SpreadDecreaseSpeed = *Dec;
				}
			}

			if (FProperty* FOVProp = URangedWeaponItem::StaticClass()->FindPropertyByName(TEXT("AimFOVPct")))
			{
				float* FOV = FOVProp->ContainerPtrToValuePtr<float>(Item);
				if (FOV)
				{
					Row.AimFOVPct = *FOV;
				}
			}
		}

		// Consumable stats from NarrativeItem
		if (FProperty* ConsumeProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("bConsumeOnUse")))
		{
			bool* Consume = ConsumeProp->ContainerPtrToValuePtr<bool>(Item);
			if (Consume)
			{
				Row.bConsumeOnUse = *Consume;
			}
		}

		if (FProperty* RechargeProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("UseRechargeDuration")))
		{
			float* Recharge = RechargeProp->ContainerPtrToValuePtr<float>(Item);
			if (Recharge)
			{
				Row.UseRechargeDuration = *Recharge;
			}
		}

		if (FProperty* ActivateProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("bCanActivate")))
		{
			bool* Activate = ActivateProp->ContainerPtrToValuePtr<bool>(Item);
			if (Activate)
			{
				Row.bCanActivate = *Activate;
			}
		}

		// Equipment effect (modifier GE)
		// v4.12.5: Only access if item inherits from EquippableItem
		if (Item->IsA(UEquippableItem::StaticClass()))
		{
			if (FProperty* EffectProp = UEquippableItem::StaticClass()->FindPropertyByName(TEXT("EquipmentEffect")))
			{
				TSubclassOf<UGameplayEffect>* Effect = EffectProp->ContainerPtrToValuePtr<TSubclassOf<UGameplayEffect>>(Item);
				if (Effect && *Effect)
				{
					Row.ModifierGE = FSoftObjectPath((*Effect)->GetPathName());
				}
			}
		}

		// Stackable and max stack from NarrativeItem
		if (FProperty* StackProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("bStackable")))
		{
			bool* Stackable = StackProp->ContainerPtrToValuePtr<bool>(Item);
			if (Stackable)
			{
				Row.bStackable = *Stackable;
			}
		}

		if (FProperty* MaxStackProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("MaxStackSize")))
		{
			int32* MaxStack = MaxStackProp->ContainerPtrToValuePtr<int32>(Item);
			if (MaxStack)
			{
				Row.MaxStackSize = *MaxStack;
			}
		}

		// Item tags
		if (FProperty* TagsProp = UNarrativeItem::StaticClass()->FindPropertyByName(TEXT("ItemTags")))
		{
			FGameplayTagContainer* Tags = TagsProp->ContainerPtrToValuePtr<FGameplayTagContainer>(Item);
			if (Tags && Tags->Num() > 0)
			{
				TArray<FString> TagStrings;
				for (const FGameplayTag& ItemTag : *Tags)
				{
					TagStrings.Add(ItemTag.ToString());
				}
				Row.ItemTags = FString::Join(TagStrings, TEXT(", "));
			}
		}

		// Restore preserved Notes
		if (FString* PreservedNote = PreservedNotes.Find(Row.ItemName))
		{
			Row.Notes = *PreservedNote;
		}

		// Mark as synced
		Row.Status = EItemTableRowStatus::Synced;
		Row.LastSyncedHash = Row.ComputeSyncHash();

		SyncedCount++;
	}

	// Update table
	MarkDirty();
	SyncFromTableData();
	RefreshList();

	FString Summary = FString::Printf(TEXT("Synced %d items from all item types"), SyncedCount);
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Summary));

	return FReply::Handled();
}

FReply SItemTableEditor::OnExportXLSXClicked()
{
	if (!TableData || bIsBusy) return FReply::Handled();

	TGuardValue<bool> BusyGuard(bIsBusy, true);

	// Sync UI changes back to TableData
	for (const TSharedPtr<FItemTableRow>& Row : AllRows)
	{
		if (Row.IsValid())
		{
			int32 Index = TableData->FindRowIndexByGuid(Row->RowId);
			if (Index != INDEX_NONE)
			{
				TableData->Rows[Index] = *Row;
			}
		}
	}

	// Show save dialog
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return FReply::Handled();
	}

	TArray<FString> OutFiles;
	FString DefaultPath = FPaths::ProjectSavedDir() / TEXT("Exports");
	FString DefaultFile = TEXT("ItemTable.xlsx");

	bool bOpened = DesktopPlatform->SaveFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Export Item Table"),
		DefaultPath,
		DefaultFile,
		TEXT("Excel Files (*.xlsx)|*.xlsx"),
		0,
		OutFiles
	);

	if (bOpened && OutFiles.Num() > 0)
	{
		FString Error;
		if (FItemXLSXWriter::ExportToXLSX(TableData->Rows, OutFiles[0], Error))
		{
			// Update sync hashes after successful export
			for (FItemTableRow& Row : TableData->Rows)
			{
				Row.LastSyncedHash = Row.ComputeSyncHash();
				Row.Status = EItemTableRowStatus::Synced;
			}

			MarkDirty();
			RefreshList();

			FMessageDialog::Open(EAppMsgType::Ok,
				FText::Format(LOCTEXT("ExportSuccess", "Exported {0} items to:\n{1}"),
					FText::AsNumber(TableData->Rows.Num()),
					FText::FromString(OutFiles[0])));
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok,
				FText::Format(LOCTEXT("ExportFailed", "Export failed: {0}"),
					FText::FromString(Error)));
		}
	}

	return FReply::Handled();
}

FReply SItemTableEditor::OnImportXLSXClicked()
{
	if (!TableData || bIsBusy) return FReply::Handled();

	TGuardValue<bool> BusyGuard(bIsBusy, true);

	// Show open dialog
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return FReply::Handled();
	}

	TArray<FString> OutFiles;
	FString DefaultPath = FPaths::ProjectSavedDir() / TEXT("Exports");

	bool bOpened = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Import Item Table"),
		DefaultPath,
		TEXT(""),
		TEXT("Excel Files (*.xlsx)|*.xlsx"),
		0,
		OutFiles
	);

	if (bOpened && OutFiles.Num() > 0)
	{
		// Validate file format
		if (!FItemXLSXReader::IsValidItemXLSX(OutFiles[0]))
		{
			FMessageDialog::Open(EAppMsgType::Ok,
				LOCTEXT("InvalidFormat", "Invalid file format. Please select an Item Table XLSX file exported from this editor."));
			return FReply::Handled();
		}

		TArray<FItemTableRow> ImportedRows;
		FString Error;

		if (FItemXLSXReader::ImportFromXLSX(OutFiles[0], ImportedRows, Error))
		{
			// Replace table data with imported rows
			TableData->Rows = ImportedRows;

			// Invalidate validation for all rows
			for (FItemTableRow& Row : TableData->Rows)
			{
				Row.InvalidateValidation();
			}

			MarkDirty();
			SyncFromTableData();
			RefreshList();

			FMessageDialog::Open(EAppMsgType::Ok,
				FText::Format(LOCTEXT("ImportSuccess", "Imported {0} items from:\n{1}"),
					FText::AsNumber(ImportedRows.Num()),
					FText::FromString(OutFiles[0])));
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok,
				FText::Format(LOCTEXT("ImportFailed", "Import failed: {0}"),
					FText::FromString(Error)));
		}
	}

	return FReply::Handled();
}

FReply SItemTableEditor::OnSyncXLSXClicked()
{
	if (!TableData || bIsBusy) return FReply::Handled();

	TGuardValue<bool> BusyGuard(bIsBusy, true);

	// Show open dialog to select XLSX file
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return FReply::Handled();

	TArray<FString> OutFiles;
	FString DefaultPath = FPaths::ProjectSavedDir() / TEXT("Exports");

	bool bOpened = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Select Item XLSX for Sync"),
		DefaultPath,
		TEXT(""),
		TEXT("Excel Files (*.xlsx)|*.xlsx"),
		0,
		OutFiles
	);

	if (!bOpened || OutFiles.Num() == 0)
	{
		return FReply::Handled();
	}

	// Validate the file format
	if (!FItemXLSXReader::IsValidItemXLSX(OutFiles[0]))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("InvalidSyncFile",
			"The selected file is not a valid Item XLSX file.\n"
			"Make sure it was exported from the Item Table Editor."));
		return FReply::Handled();
	}

	// Sync UI changes back to TableData first
	for (const TSharedPtr<FItemTableRow>& Row : AllRows)
	{
		if (Row.IsValid())
		{
			int32 Index = TableData->FindRowIndexByGuid(Row->RowId);
			if (Index != INDEX_NONE)
			{
				TableData->Rows[Index] = *Row;
			}
		}
	}

	// Import rows from Excel
	TArray<FItemTableRow> ExcelRows;
	FString ImportError;
	if (!FItemXLSXReader::ImportFromXLSX(OutFiles[0], ExcelRows, ImportError))
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("SyncImportFailed", "Failed to read Excel file:\n{0}"),
			FText::FromString(ImportError)));
		return FReply::Handled();
	}

	// Build base rows and UE rows
	TArray<FItemTableRow> BaseRows;
	TArray<FItemTableRow> UERows;

	for (const FItemTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			UERows.Add(Row);
			// Base is conceptually the row at the time of last export
			// Rows with non-zero LastSyncedHash existed at last export
			if (Row.LastSyncedHash != 0)
			{
				BaseRows.Add(Row);
			}
		}
	}

	// Perform 3-way comparison
	FItemSyncResult SyncResult = FItemXLSXSyncEngine::CompareSources(BaseRows, UERows, ExcelRows);

	if (!SyncResult.bSuccess)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("SyncCompareFailed", "Sync comparison failed:\n{0}"),
			FText::FromString(SyncResult.ErrorMessage)));
		return FReply::Handled();
	}

	// Auto-resolve unchanged entries (v4.11: only Unchanged auto-resolves)
	FItemXLSXSyncEngine::AutoResolveNonConflicts(SyncResult);

	// Check if there are any changes
	if (!SyncResult.HasChanges())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoSyncChanges",
			"No changes detected between UE and Excel.\n"
			"Both sources are in sync."));
		return FReply::Handled();
	}

	// Show approval dialog
	if (!SItemXLSXSyncDialog::ShowModal(SyncResult))
	{
		// User cancelled
		return FReply::Handled();
	}

	// Check all entries are resolved
	if (!FItemXLSXSyncEngine::AllConflictsResolved(SyncResult))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("UnresolvedConflicts",
			"Some entries have not been resolved.\n"
			"Please resolve all conflicts before applying."));
		return FReply::Handled();
	}

	// Apply sync result
	FItemMergeResult MergeResult = FItemXLSXSyncEngine::ApplySync(SyncResult);

	// Replace TableData rows with merged result
	TableData->Rows.Empty();
	for (const FItemTableRow& MergedRow : MergeResult.MergedRows)
	{
		FItemTableRow& NewRow = TableData->Rows.Add_GetRef(MergedRow);
		NewRow.LastSyncedHash = NewRow.ComputeSyncHash();
		NewRow.Status = EItemTableRowStatus::Synced;
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

FReply SItemTableEditor::OnSaveClicked()
{
	if (!TableData) return FReply::Handled();

	// Copy modified data back to TableData
	for (const TSharedPtr<FItemTableRow>& Row : AllRows)
	{
		if (Row.IsValid())
		{
			int32 Index = TableData->FindRowIndexByGuid(Row->RowId);
			if (Index != INDEX_NONE)
			{
				TableData->Rows[Index] = *Row;
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

FReply SItemTableEditor::OnApplyToAssetsClicked()
{
	if (!TableData || bIsBusy) return FReply::Handled();

	TGuardValue<bool> BusyGuard(bIsBusy, true);

	// Step 1: Sync UI changes back to TableData
	for (const TSharedPtr<FItemTableRow>& Row : AllRows)
	{
		if (Row.IsValid())
		{
			int32 Index = TableData->FindRowIndexByGuid(Row->RowId);
			if (Index != INDEX_NONE)
			{
				TableData->Rows[Index] = *Row;
			}
		}
	}

	// Step 2: Gather rows to apply (non-deleted rows only)
	TArray<FItemTableRow> RowsToApply;
	for (const FItemTableRow& Row : TableData->Rows)
	{
		if (!Row.bDeleted)
		{
			RowsToApply.Add(Row);
		}
	}

	if (RowsToApply.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoRowsToApply", "No item rows to apply. Add items first."));
		return FReply::Handled();
	}

	// Step 3: Confirm with user
	FText ConfirmMessage = FText::Format(
		LOCTEXT("ApplyConfirm", "This will regenerate Item assets for {0} item(s).\n\n"
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
	FString OutputFolder = TableData ? TableData->OutputFolder : TEXT("/Game/Items");

	FItemAssetApplySummary Summary = FItemAssetSync::ApplyToAssets(
		RowsToApply,
		OutputFolder,
		true);  // bCreateMissing = true

	// Step 5: Update row statuses
	for (FItemTableRow& AppliedRow : RowsToApply)
	{
		for (TSharedPtr<FItemTableRow>& Row : AllRows)
		{
			if (Row.IsValid() && Row->RowId == AppliedRow.RowId)
			{
				Row->Status = AppliedRow.Status;
				Row->GeneratedItem = AppliedRow.GeneratedItem;
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
			"Items processed: {0}\n"
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
			FText::AsNumber(Summary.FailedItems.Num())));

	return FReply::Handled();
}

void SItemTableEditor::MarkDirty()
{
	if (TableData)
	{
		TableData->MarkPackageDirty();
	}
	OnDirtyStateChanged.ExecuteIfBound();
}

void SItemTableEditor::OnRowModified()
{
	MarkDirty();
	UpdateStatusBar();
}

void SItemTableEditor::UpdateDynamicColumnVisibility()
{
	// Determine which item types are visible in the filtered data
	bool bHasWeapons = false;
	bool bHasRanged = false;
	bool bHasEquippables = false;

	for (const TSharedPtr<FItemTableRow>& Row : DisplayedRows)
	{
		if (!Row.IsValid()) continue;

		switch (Row->ItemType)
		{
			case EItemType::RangedWeapon:
				bHasRanged = true;
				bHasWeapons = true;
				break;
			case EItemType::MeleeWeapon:
			case EItemType::MagicWeapon:
			case EItemType::ThrowableWeapon:
				bHasWeapons = true;
				break;
			case EItemType::Equippable:
			case EItemType::Clothing:
				bHasEquippables = true;
				break;
			default:
				break;
		}
	}

	// v4.12: Column visibility state (could be used by header row widget)
	// For now, just refresh the list which will use ShouldShowColumn in each row
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

//=============================================================================
// SItemTableEditorWindow
//=============================================================================

void SItemTableEditorWindow::Construct(const FArguments& InArgs)
{
	BaseTabLabel = LOCTEXT("ItemEditorTab", "Item Table Editor");

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
			SAssignNew(TableEditor, SItemTableEditor)
			.TableData(GetOrCreateTableData())
			.OnDirtyStateChanged(FOnItemTableDirtyStateChanged::CreateSP(this, &SItemTableEditorWindow::UpdateTabLabel))
		]
	];
}

void SItemTableEditorWindow::SetParentTab(TSharedPtr<SDockTab> InTab)
{
	ParentTab = InTab;
	UpdateTabLabel();
}

TSharedRef<SWidget> SItemTableEditorWindow::BuildMenuBar()
{
	FMenuBarBuilder MenuBuilder(nullptr);

	MenuBuilder.AddPullDownMenu(
		LOCTEXT("FileMenu", "File"),
		LOCTEXT("FileMenuTooltip", "File operations"),
		FNewMenuDelegate::CreateLambda([this](FMenuBuilder& Builder)
		{
			Builder.AddMenuEntry(
				LOCTEXT("NewTable", "New Table"),
				LOCTEXT("NewTableTooltip", "Create a new item table"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SItemTableEditorWindow::OnNewTable))
			);
			Builder.AddMenuEntry(
				LOCTEXT("OpenTable", "Open Table..."),
				LOCTEXT("OpenTableTooltip", "Open an existing item table"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SItemTableEditorWindow::OnOpenTable))
			);
			Builder.AddMenuSeparator();
			Builder.AddMenuEntry(
				LOCTEXT("SaveTable", "Save"),
				LOCTEXT("SaveTableTooltip", "Save the current table"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SItemTableEditorWindow::OnSaveTable))
			);
		})
	);

	return MenuBuilder.MakeWidget();
}

void SItemTableEditorWindow::OnNewTable()
{
	CurrentTableData = GetOrCreateTableData();
	CurrentTableData->Rows.Empty();
	if (TableEditor.IsValid())
	{
		TableEditor->SetTableData(CurrentTableData.Get());
	}
}

void SItemTableEditorWindow::OnOpenTable()
{
	// Use content browser asset picker
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FOpenAssetDialogConfig Config;
	Config.DialogTitleOverride = LOCTEXT("OpenItemTableTitle", "Open Item Table Data");
	Config.bAllowMultipleSelection = false;
	Config.AssetClassNames.Add(UItemTableData::StaticClass()->GetClassPathName());

	TArray<FAssetData> SelectedAssets = ContentBrowserModule.Get().CreateModalOpenAssetDialog(Config);
	if (SelectedAssets.Num() > 0)
	{
		UItemTableData* LoadedTable = Cast<UItemTableData>(SelectedAssets[0].GetAsset());
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

void SItemTableEditorWindow::OnSaveTable()
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

void SItemTableEditorWindow::OnSaveTableAs()
{
	// Use content browser save asset dialog
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FSaveAssetDialogConfig Config;
	Config.DialogTitleOverride = LOCTEXT("SaveItemTableAsTitle", "Save Item Table As");
	Config.DefaultPath = TEXT("/Game/Tables");
	Config.DefaultAssetName = TEXT("ItemTableData");
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
			// Create a new UItemTableData object in the new package
			UItemTableData* NewTableData = NewObject<UItemTableData>(NewPackage, *AssetName, RF_Public | RF_Standalone);

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

UItemTableData* SItemTableEditorWindow::GetOrCreateTableData()
{
	if (CurrentTableData.IsValid())
	{
		return CurrentTableData.Get();
	}

	// Try to load default table
	FString DefaultPath = TEXT("/Game/Tables/ItemTableData");
	UItemTableData* Data = LoadObject<UItemTableData>(nullptr, *DefaultPath);

	if (!Data)
	{
		// Create new
		FString PackagePath = TEXT("/Game/Tables/ItemTableData");
		UPackage* Package = CreatePackage(*PackagePath);
		Data = NewObject<UItemTableData>(Package, TEXT("ItemTableData"), RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(Data);
		Data->MarkPackageDirty();
	}

	CurrentTableData = Data;
	return Data;
}

void SItemTableEditorWindow::UpdateTabLabel()
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

bool SItemTableEditorWindow::IsTableDirty() const
{
	if (CurrentTableData.IsValid())
	{
		return CurrentTableData->GetPackage()->IsDirty();
	}
	return false;
}

bool SItemTableEditorWindow::CanCloseTab() const
{
	if (IsTableDirty())
	{
		EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNoCancel,
			LOCTEXT("SaveBeforeClose", "Save changes before closing?"));

		if (Result == EAppReturnType::Yes)
		{
			const_cast<SItemTableEditorWindow*>(this)->OnSaveTable();
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

//=============================================================================
// v7.2: Undo/Redo System Implementation
//=============================================================================

FReply SItemTableEditor::OnUndoClicked()
{
	if (TransactionStack.IsValid() && TransactionStack->CanUndo())
	{
		TransactionStack->Undo();
		ApplyFilters();
		MarkDirty();
	}
	return FReply::Handled();
}

FReply SItemTableEditor::OnRedoClicked()
{
	if (TransactionStack.IsValid() && TransactionStack->CanRedo())
	{
		TransactionStack->Redo();
		ApplyFilters();
		MarkDirty();
	}
	return FReply::Handled();
}

bool SItemTableEditor::CanUndo() const
{
	return TransactionStack.IsValid() && TransactionStack->CanUndo();
}

bool SItemTableEditor::CanRedo() const
{
	return TransactionStack.IsValid() && TransactionStack->CanRedo();
}

FReply SItemTableEditor::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// Ctrl+Z = Undo
	if (InKeyEvent.IsControlDown() && InKeyEvent.GetKey() == EKeys::Z)
	{
		return OnUndoClicked();
	}
	// Ctrl+Y = Redo
	if (InKeyEvent.IsControlDown() && InKeyEvent.GetKey() == EKeys::Y)
	{
		return OnRedoClicked();
	}
	return FReply::Unhandled();
}

void SItemTableEditor::AddRowWithUndo(TSharedPtr<FItemTableRow> NewRow)
{
	if (!NewRow.IsValid() || !TransactionStack.IsValid())
	{
		return;
	}

	class FItemAddRowTransaction : public FTableEditorTransaction
	{
	public:
		FItemAddRowTransaction(SItemTableEditor* InEditor, TSharedPtr<FItemTableRow> InRow)
			: Editor(InEditor), Row(InRow)
		{
		}

		virtual void Execute() override
		{
			if (Editor && Row.IsValid())
			{
				Editor->AllRows.Add(Row);
				if (Editor->TableData)
				{
					Editor->TableData->Rows.Add(*Row);
				}
			}
		}

		virtual void Undo() override
		{
			if (Editor && Row.IsValid())
			{
				Editor->AllRows.RemoveAll([this](const TSharedPtr<FItemTableRow>& Item)
				{
					return Item->RowId == Row->RowId;
				});
				if (Editor->TableData)
				{
					Editor->TableData->Rows.RemoveAll([this](const FItemTableRow& Item)
					{
						return Item.RowId == Row->RowId;
					});
				}
			}
		}

		virtual FString GetDescription() const override
		{
			return FString::Printf(TEXT("Add Item: %s"), *Row->ItemName);
		}

	private:
		SItemTableEditor* Editor;
		TSharedPtr<FItemTableRow> Row;
	};

	auto Transaction = MakeShared<FItemAddRowTransaction>(this, NewRow);
	Transaction->Execute();
	TransactionStack->AddTransaction(Transaction);
}

void SItemTableEditor::DeleteRowsWithUndo(const TArray<TSharedPtr<FItemTableRow>>& RowsToDelete)
{
	if (RowsToDelete.Num() == 0 || !TransactionStack.IsValid())
	{
		return;
	}

	class FItemDeleteRowsTransaction : public FTableEditorTransaction
	{
	public:
		FItemDeleteRowsTransaction(SItemTableEditor* InEditor, const TArray<TSharedPtr<FItemTableRow>>& InRows)
			: Editor(InEditor)
		{
			for (const TSharedPtr<FItemTableRow>& Row : InRows)
			{
				int32 Index = Editor->AllRows.IndexOfByPredicate([&Row](const TSharedPtr<FItemTableRow>& Item)
				{
					return Item->RowId == Row->RowId;
				});
				if (Index != INDEX_NONE)
				{
					DeletedRows.Add(TPair<int32, TSharedPtr<FItemTableRow>>(Index, MakeShared<FItemTableRow>(*Row)));
				}
			}
			DeletedRows.Sort([](const auto& A, const auto& B) { return A.Key > B.Key; });
		}

		virtual void Execute() override
		{
			if (!Editor) return;

			for (const auto& Pair : DeletedRows)
			{
				Editor->AllRows.RemoveAll([&Pair](const TSharedPtr<FItemTableRow>& Item)
				{
					return Item->RowId == Pair.Value->RowId;
				});
				if (Editor->TableData)
				{
					Editor->TableData->Rows.RemoveAll([&Pair](const FItemTableRow& Item)
					{
						return Item.RowId == Pair.Value->RowId;
					});
				}
			}
		}

		virtual void Undo() override
		{
			if (!Editor) return;

			for (int32 i = DeletedRows.Num() - 1; i >= 0; --i)
			{
				const auto& Pair = DeletedRows[i];
				TSharedPtr<FItemTableRow> RestoredRow = MakeShared<FItemTableRow>(*Pair.Value);

				if (Pair.Key <= Editor->AllRows.Num())
				{
					Editor->AllRows.Insert(RestoredRow, Pair.Key);
				}
				else
				{
					Editor->AllRows.Add(RestoredRow);
				}

				if (Editor->TableData)
				{
					Editor->TableData->Rows.Add(*RestoredRow);
				}
			}
		}

		virtual FString GetDescription() const override
		{
			if (DeletedRows.Num() == 1)
			{
				return FString::Printf(TEXT("Delete Item: %s"), *DeletedRows[0].Value->ItemName);
			}
			return FString::Printf(TEXT("Delete %d Items"), DeletedRows.Num());
		}

	private:
		SItemTableEditor* Editor;
		TArray<TPair<int32, TSharedPtr<FItemTableRow>>> DeletedRows;
	};

	auto Transaction = MakeShared<FItemDeleteRowsTransaction>(this, RowsToDelete);
	Transaction->Execute();
	TransactionStack->AddTransaction(Transaction);
}

void SItemTableEditor::RecordRowEdit(TSharedPtr<FItemTableRow> Row, const FItemTableRow& OldState)
{
	if (!Row.IsValid() || !TransactionStack.IsValid())
	{
		return;
	}

	class FItemEditRowTransaction : public FTableEditorTransaction
	{
	public:
		FItemEditRowTransaction(TSharedPtr<FItemTableRow> InRow, const FItemTableRow& InOldState)
			: Row(InRow)
			, OldState(MakeShared<FItemTableRow>(InOldState))
			, NewState(MakeShared<FItemTableRow>(*InRow))
		{
		}

		virtual void Execute() override
		{
			if (Row.IsValid() && NewState.IsValid())
			{
				FGuid SavedRowId = Row->RowId;
				*Row = *NewState;
				Row->RowId = SavedRowId;
			}
		}

		virtual void Undo() override
		{
			if (Row.IsValid() && OldState.IsValid())
			{
				FGuid SavedRowId = Row->RowId;
				*Row = *OldState;
				Row->RowId = SavedRowId;
			}
		}

		virtual FString GetDescription() const override
		{
			return FString::Printf(TEXT("Edit Item: %s"), *Row->ItemName);
		}

	private:
		TSharedPtr<FItemTableRow> Row;
		TSharedPtr<FItemTableRow> OldState;
		TSharedPtr<FItemTableRow> NewState;
	};

	auto Transaction = MakeShared<FItemEditRowTransaction>(Row, OldState);
	TransactionStack->AddTransaction(Transaction);
}

//=============================================================================
// v7.2: Find & Replace Implementation
//=============================================================================

void SItemTableEditor::PerformSearch()
{
	SearchResults.Empty();
	CurrentMatchIndex = -1;

	if (FindText.IsEmpty())
	{
		return;
	}

	TArray<FItemTableColumn> Columns = GetItemTableColumns();

	for (int32 RowIndex = 0; RowIndex < DisplayedRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FItemTableRow>& Row = DisplayedRows[RowIndex];
		if (!Row.IsValid()) continue;

		for (const FItemTableColumn& Col : Columns)
		{
			FString CellValue = GetColumnValue(Row, Col.ColumnId);
			if (CellValue.Contains(FindText, ESearchCase::IgnoreCase))
			{
				SearchResults.Add(TPair<int32, FName>(RowIndex, Col.ColumnId));
			}
		}
	}

	if (SearchResults.Num() > 0)
	{
		CurrentMatchIndex = 0;
		NavigateToMatch(CurrentMatchIndex);
	}
}

void SItemTableEditor::NavigateToMatch(int32 MatchIndex)
{
	if (MatchIndex < 0 || MatchIndex >= SearchResults.Num())
	{
		return;
	}

	const TPair<int32, FName>& Match = SearchResults[MatchIndex];
	int32 RowIndex = Match.Key;

	if (RowIndex >= 0 && RowIndex < DisplayedRows.Num())
	{
		if (ListView.IsValid())
		{
			ListView->SetSelection(DisplayedRows[RowIndex]);
			ListView->RequestScrollIntoView(DisplayedRows[RowIndex]);
		}
	}
}

FReply SItemTableEditor::OnFindNextClicked()
{
	if (FindTextBox.IsValid())
	{
		FindText = FindTextBox->GetText().ToString();
	}

	if (SearchResults.Num() == 0)
	{
		PerformSearch();
	}

	if (SearchResults.Num() > 0)
	{
		CurrentMatchIndex = (CurrentMatchIndex + 1) % SearchResults.Num();
		NavigateToMatch(CurrentMatchIndex);
	}

	return FReply::Handled();
}

FReply SItemTableEditor::OnFindPrevClicked()
{
	if (FindTextBox.IsValid())
	{
		FindText = FindTextBox->GetText().ToString();
	}

	if (SearchResults.Num() == 0)
	{
		PerformSearch();
	}

	if (SearchResults.Num() > 0)
	{
		CurrentMatchIndex = (CurrentMatchIndex - 1 + SearchResults.Num()) % SearchResults.Num();
		NavigateToMatch(CurrentMatchIndex);
	}

	return FReply::Handled();
}

FReply SItemTableEditor::OnReplaceClicked()
{
	if (FindText.IsEmpty() || CurrentMatchIndex < 0 || CurrentMatchIndex >= SearchResults.Num())
	{
		return FReply::Handled();
	}

	if (ReplaceTextBox.IsValid())
	{
		ReplaceText = ReplaceTextBox->GetText().ToString();
	}

	const TPair<int32, FName>& Match = SearchResults[CurrentMatchIndex];
	int32 RowIndex = Match.Key;
	FName ColumnId = Match.Value;

	if (RowIndex >= 0 && RowIndex < DisplayedRows.Num())
	{
		TSharedPtr<FItemTableRow> Row = DisplayedRows[RowIndex];
		if (Row.IsValid())
		{
			FString CurrentValue = GetColumnValue(Row, ColumnId);
			FString NewValue = CurrentValue.Replace(*FindText, *ReplaceText);

			// Set new value based on column
			if (ColumnId == TEXT("ItemName")) Row->ItemName = NewValue;
			else if (ColumnId == TEXT("DisplayName")) Row->DisplayName = NewValue;
			else if (ColumnId == TEXT("Description")) Row->Description = NewValue;

			MarkDirty();
			RefreshList();
			OnFindNextClicked();
		}
	}

	return FReply::Handled();
}

FReply SItemTableEditor::OnReplaceAllClicked()
{
	if (FindText.IsEmpty())
	{
		return FReply::Handled();
	}

	if (ReplaceTextBox.IsValid())
	{
		ReplaceText = ReplaceTextBox->GetText().ToString();
	}

	PerformSearch();
	int32 ReplacementCount = 0;

	for (int32 i = SearchResults.Num() - 1; i >= 0; --i)
	{
		const TPair<int32, FName>& Match = SearchResults[i];
		int32 RowIndex = Match.Key;
		FName ColumnId = Match.Value;

		if (RowIndex >= 0 && RowIndex < DisplayedRows.Num())
		{
			TSharedPtr<FItemTableRow> Row = DisplayedRows[RowIndex];
			if (Row.IsValid())
			{
				FString CurrentValue = GetColumnValue(Row, ColumnId);
				FString NewValue = CurrentValue.Replace(*FindText, *ReplaceText);

				if (ColumnId == TEXT("ItemName")) Row->ItemName = NewValue;
				else if (ColumnId == TEXT("DisplayName")) Row->DisplayName = NewValue;
				else if (ColumnId == TEXT("Description")) Row->Description = NewValue;

				ReplacementCount++;
			}
		}
	}

	if (ReplacementCount > 0)
	{
		MarkDirty();
		RefreshList();
		SearchResults.Empty();
		CurrentMatchIndex = -1;
	}

	FMessageDialog::Open(EAppMsgType::Ok,
		FText::Format(LOCTEXT("ReplaceAllResult", "Replaced {0} occurrence(s)."), FText::AsNumber(ReplacementCount)));

	return FReply::Handled();
}

bool SItemTableEditor::IsCellMatch(int32 RowIndex, FName ColumnId) const
{
	for (const TPair<int32, FName>& Match : SearchResults)
	{
		if (Match.Key == RowIndex && Match.Value == ColumnId)
		{
			return true;
		}
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
