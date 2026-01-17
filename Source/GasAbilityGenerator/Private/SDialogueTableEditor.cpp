// GasAbilityGenerator - Dialogue Table Editor Implementation
// v4.11.4: Show "(None)" for empty values in all columns for consistency
// v4.6: Added dirty indicator, save-on-close prompt, generation state display, soft delete
// v4.4: Added validation error feedback in status bar
// v4.3: Added XLSX export/import with sync support
// v4.2.14: Fixed Seq column not updating - changed to Text_Lambda for dynamic reads
// v4.2.13: Fixed status bar - stored STextBlock refs + explicit SetText() calls
//
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#include "SDialogueTableEditor.h"
#include "DialogueTableValidator.h"
#include "DialogueTableConverter.h"
#include "GasAbilityGeneratorGenerators.h"  // v4.10: For FDialogueBlueprintGenerator

DEFINE_LOG_CATEGORY_STATIC(LogGasAbilityGenerator, Log, All);  // v4.10: Provenance logging
#include "XLSXSupport/DialogueXLSXWriter.h"
#include "XLSXSupport/DialogueXLSXReader.h"
#include "XLSXSupport/DialogueXLSXSyncEngine.h"
#include "XLSXSupport/SDialogueXLSXSyncDialog.h"
#include "XLSXSupport/SDialogueTokenApplyPreview.h"
#include "XLSXSupport/DialogueAssetSync.h"
#include "GasAbilityGeneratorTypes.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SBox.h"
#include "Styling/AppStyle.h"
#include "Misc/FileHelper.h"
#include "DesktopPlatformModule.h"
#include "EditorDirectories.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/MessageDialog.h"
#include "Widgets/Docking/SDockTab.h"  // v4.6: For dirty indicator

#define LOCTEXT_NAMESPACE "DialogueTableEditor"

//=============================================================================
// SDialogueTableRow - Individual Row Widget
//=============================================================================

void SDialogueTableRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	RowDataEx = InArgs._RowData;
	OnRowModified = InArgs._OnRowModified;

	SMultiColumnTableRow<TSharedPtr<FDialogueTableRowEx>>::Construct(
		FSuperRowType::FArguments()
			.Padding(FMargin(2.0f, 1.0f)),
		InOwnerTable
	);
}

TSharedRef<SWidget> SDialogueTableRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (!RowDataEx.IsValid() || !RowDataEx->Data.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	FDialogueTableRow* RowData = RowDataEx->Data.Get();

	if (ColumnName == TEXT("Seq"))
	{
		return CreateSeqCell();
	}
	if (ColumnName == TEXT("Status"))
	{
		return CreateStatusCell();
	}
	if (ColumnName == TEXT("DialogueID"))
	{
		return CreateFNameCell(RowData->DialogueID, TEXT("DBP_NPCName"));
	}
	if (ColumnName == TEXT("NodeID"))
	{
		return CreateNodeIDCell();
	}
	if (ColumnName == TEXT("NodeType"))
	{
		return CreateNodeTypeCell();
	}
	if (ColumnName == TEXT("Speaker"))
	{
		return CreateSpeakerCell();
	}
	if (ColumnName == TEXT("Text"))
	{
		return CreateTextCell(RowData->Text, TEXT("Dialogue text..."), true);  // With tooltip
	}
	if (ColumnName == TEXT("OptionText"))
	{
		return CreateTextCell(RowData->OptionText, TEXT("Player choice text"), true);  // With tooltip
	}
	if (ColumnName == TEXT("Events"))
	{
		return CreateTokenCell(RowData->EventsTokenStr, RowData->bEventsValid, ETokenCategory::Event);
	}
	// v4.11.4: Split Conditions into Condition (type) and Options (parameters)
	if (ColumnName == TEXT("Condition"))
	{
		return CreateConditionTypeCell();
	}
	if (ColumnName == TEXT("Options"))
	{
		return CreateConditionOptionsCell();
	}
	if (ColumnName == TEXT("ParentNodeID"))
	{
		return CreateFNameCell(RowData->ParentNodeID, TEXT("parent_node"));
	}
	if (ColumnName == TEXT("NextNodeIDs"))
	{
		return CreateNextNodesCell();
	}
	if (ColumnName == TEXT("Skippable"))
	{
		return CreateSkippableCell();
	}
	if (ColumnName == TEXT("Notes"))
	{
		return CreateNotesCell();
	}

	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SDialogueTableRow::CreateSeqCell()
{
	// v4.5.2: Colored validation indicator + sequence display
	return SNew(SHorizontalBox)
		// Validation color indicator (thin colored bar)
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
		// Sequence number
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(STextBlock)
				.Text_Lambda([this]() { return FText::FromString(RowDataEx->SeqDisplay); })
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

// v4.7: Status cell with colored badge (matches NPC table editor)
TSharedRef<SWidget> SDialogueTableRow::CreateStatusCell()
{
	return SNew(SHorizontalBox)
		// Validation color stripe (4px)
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
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
				.BorderBackgroundColor_Lambda([this]()
				{
					if (RowDataEx.IsValid() && RowDataEx->Data.IsValid())
					{
						return FSlateColor(RowDataEx->Data->GetStatusColor());
					}
					return FSlateColor(FLinearColor::White);
				})
				.Padding(FMargin(4.0f, 1.0f))
				[
					SNew(STextBlock)
						.Text_Lambda([this]()
						{
							if (RowDataEx.IsValid() && RowDataEx->Data.IsValid())
							{
								return FText::FromString(RowDataEx->Data->GetStatusString());
							}
							return FText::FromString(TEXT("?"));
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
				]
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateNodeIDCell()
{
	FDialogueTableRow* RowData = RowDataEx->Data.Get();

	// Calculate indent based on depth (8px per level)
	float IndentSize = RowDataEx->Depth * 8.0f;

	return SNew(SHorizontalBox)
		// Indent spacer
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SSpacer)
				.Size(FVector2D(IndentSize, 1.0f))
		]
		// Node ID text
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableText)
				.Text_Lambda([RowData]()
				{
					// v4.11.4: Show "(None)" for empty values for consistency
					return (RowData->NodeID.IsNone() || RowData->NodeID.ToString().IsEmpty())
						? FText::FromString(TEXT("(None)"))
						: FText::FromName(RowData->NodeID);
				})
				.HintText(FText::FromString(TEXT("node_id")))
				.OnTextCommitted_Lambda([this, RowData](const FText& NewText, ETextCommit::Type)
				{
					FString Value = NewText.ToString();
					// Treat "(None)" input as clearing the field
					if (Value == TEXT("(None)"))
					{
						Value = TEXT("");
					}
					RowData->NodeID = Value.IsEmpty() ? NAME_None : FName(*Value);
					MarkModified();
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateTextCell(FString& Value, const FString& Hint, bool bWithTooltip)
{
	// v4.7: Add confirmation prompt for text changes (matches NPC editor)
	// v4.11.4: Show "(None)" for empty values for consistency
	TSharedRef<SWidget> EditableText = SNew(SEditableText)
		.Text_Lambda([&Value]()
		{
			return Value.IsEmpty() ? FText::FromString(TEXT("(None)")) : FText::FromString(Value);
		})
		.HintText(FText::FromString(Hint))
		.OnTextCommitted_Lambda([this, &Value, Hint](const FText& NewText, ETextCommit::Type)
		{
			FString NewValue = NewText.ToString();
			// Treat "(None)" input as clearing the field
			if (NewValue == TEXT("(None)"))
			{
				NewValue = TEXT("");
			}
			if (NewValue == Value)
			{
				return; // No change
			}

			// Show confirmation dialog
			FString DisplayOld = Value.IsEmpty() ? TEXT("(None)") : Value;
			FString DisplayNew = NewValue.IsEmpty() ? TEXT("(None)") : NewValue;
			// Truncate long strings for display
			if (DisplayOld.Len() > 50) DisplayOld = DisplayOld.Left(47) + TEXT("...");
			if (DisplayNew.Len() > 50) DisplayNew = DisplayNew.Left(47) + TEXT("...");

			EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
				FText::Format(NSLOCTEXT("DialogueTableEditor", "ConfirmTextChange", "Change '{0}' from '{1}' to '{2}'?\n\nThis will mark the node as modified."),
					FText::FromString(Hint),
					FText::FromString(DisplayOld),
					FText::FromString(DisplayNew)));

			if (Result == EAppReturnType::Yes)
			{
				Value = NewValue;
				MarkModified();
			}
		})
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9));

	// Add tooltip for Text/OptionText columns to show full content on hover
	if (bWithTooltip)
	{
		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			.ToolTipText_Lambda([&Value]()
			{
				return Value.IsEmpty() ? FText::GetEmpty() : FText::FromString(Value);
			})
			[
				EditableText
			];
	}

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			EditableText
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateFNameCell(FName& Value, const FString& Hint)
{
	// v4.7: Add confirmation prompt for FName changes (matches NPC editor)
	// v4.11.4: Show "(None)" for empty values for consistency
	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableText)
				.Text_Lambda([&Value]()
				{
					return (Value.IsNone() || Value.ToString().IsEmpty()) ? FText::FromString(TEXT("(None)")) : FText::FromName(Value);
				})
				.HintText(FText::FromString(Hint))
				.OnTextCommitted_Lambda([this, &Value, Hint](const FText& NewText, ETextCommit::Type)
				{
					FString ValueStr = NewText.ToString();
					// Treat "(None)" input as clearing the field
					if (ValueStr == TEXT("(None)"))
					{
						ValueStr = TEXT("");
					}
					FName NewValue = ValueStr.IsEmpty() ? NAME_None : FName(*ValueStr);
					if (NewValue == Value)
					{
						return; // No change
					}

					FString DisplayOld = Value.IsNone() ? TEXT("(None)") : Value.ToString();
					FString DisplayNew = NewValue.IsNone() ? TEXT("(None)") : NewValue.ToString();

					EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
						FText::Format(NSLOCTEXT("DialogueTableEditor", "ConfirmFNameChange", "Change '{0}' from '{1}' to '{2}'?\n\nThis will mark the node as modified."),
							FText::FromString(Hint),
							FText::FromString(DisplayOld),
							FText::FromString(DisplayNew)));

					if (Result == EAppReturnType::Yes)
					{
						Value = NewValue;
						MarkModified();
					}
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateNodeTypeCell()
{
	// v4.7: Add confirmation prompt for node type toggle (matches NPC editor)
	FDialogueTableRow* RowData = RowDataEx->Data.Get();

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableText)
				.Text_Lambda([RowData]()
				{
					return FText::FromString(RowData->NodeType == EDialogueTableNodeType::NPC ? TEXT("NPC") : TEXT("Player"));
				})
				.OnTextCommitted_Lambda([this, RowData](const FText& NewText, ETextCommit::Type)
				{
					FString TypeStr = NewText.ToString().ToLower();
					EDialogueTableNodeType NewType = (TypeStr.Contains(TEXT("player")) || TypeStr == TEXT("p"))
						? EDialogueTableNodeType::Player
						: EDialogueTableNodeType::NPC;

					if (NewType == RowData->NodeType)
					{
						return; // No change
					}

					FString OldTypeStr = RowData->NodeType == EDialogueTableNodeType::NPC ? TEXT("NPC") : TEXT("Player");
					FString NewTypeStr = NewType == EDialogueTableNodeType::NPC ? TEXT("NPC") : TEXT("Player");

					EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
						FText::Format(NSLOCTEXT("DialogueTableEditor", "ConfirmNodeType", "Change node type from '{0}' to '{1}'?\n\nThis will mark the node as modified."),
							FText::FromString(OldTypeStr),
							FText::FromString(NewTypeStr)));

					if (Result == EAppReturnType::Yes)
					{
						RowData->NodeType = NewType;
						MarkModified();
					}
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateSpeakerCell()
{
	FDialogueTableRow* RowData = RowDataEx->Data.Get();

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableText)
				.Text_Lambda([RowData]()
				{
					// v4.11.4: Show "(None)" for empty values for consistency
					// For Player nodes, show "Player" if Speaker is empty
					if (RowData->NodeType == EDialogueTableNodeType::Player)
					{
						if (RowData->Speaker.IsNone() || RowData->Speaker.ToString().IsEmpty())
						{
							return FText::FromString(TEXT("Player"));
						}
					}
					else // NPC node
					{
						if (RowData->Speaker.IsNone() || RowData->Speaker.ToString().IsEmpty())
						{
							return FText::FromString(TEXT("(None)"));
						}
					}
					return FText::FromName(RowData->Speaker);
				})
				.HintText(FText::FromString(TEXT("Speaker Name")))
				.OnTextCommitted_Lambda([this, RowData](const FText& NewText, ETextCommit::Type)
				{
					FString SpeakerStr = NewText.ToString();
					// Don't store "Player" in the data - keep it empty for player nodes
					if (RowData->NodeType == EDialogueTableNodeType::Player && SpeakerStr.Equals(TEXT("Player"), ESearchCase::IgnoreCase))
					{
						RowData->Speaker = NAME_None;
					}
					else
					{
						RowData->Speaker = FName(*SpeakerStr);
					}
					MarkModified();
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateNextNodesCell()
{
	FDialogueTableRow* RowData = RowDataEx->Data.Get();

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableText)
				.Text_Lambda([RowData]()
				{
					// v4.11.4: Show "(None)" for empty values for consistency
					if (RowData->NextNodeIDs.Num() == 0)
					{
						return FText::FromString(TEXT("(None)"));
					}
					FString Result;
					for (int32 i = 0; i < RowData->NextNodeIDs.Num(); i++)
					{
						if (i > 0) Result += TEXT(", ");
						Result += RowData->NextNodeIDs[i].ToString();
					}
					return FText::FromString(Result);
				})
				.HintText(FText::FromString(TEXT("node1, node2, ...")))
				.OnTextCommitted_Lambda([this, RowData](const FText& NewText, ETextCommit::Type)
				{
					FString Value = NewText.ToString();
					// Treat "(None)" input as clearing the field
					if (Value == TEXT("(None)"))
					{
						Value = TEXT("");
					}
					RowData->NextNodeIDs.Empty();
					TArray<FString> Parts;
					Value.ParseIntoArray(Parts, TEXT(","));
					for (FString& Part : Parts)
					{
						Part.TrimStartAndEndInline();
						if (!Part.IsEmpty())
						{
							RowData->NextNodeIDs.Add(FName(*Part));
						}
					}
					MarkModified();
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateSkippableCell()
{
	// v4.7: Add confirmation prompt for checkbox (matches NPC editor)
	FDialogueTableRow* RowData = RowDataEx->Data.Get();

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		.HAlign(HAlign_Center)
		[
			SNew(SCheckBox)
				.IsChecked_Lambda([RowData]()
				{
					return RowData->bSkippable ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([this, RowData](ECheckBoxState NewState)
				{
					bool bNewValue = (NewState == ECheckBoxState::Checked);
					if (bNewValue == RowData->bSkippable)
					{
						return; // No change
					}

					FString Action = bNewValue ? TEXT("Enable") : TEXT("Disable");
					EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
						FText::Format(NSLOCTEXT("DialogueTableEditor", "ConfirmSkippable", "{0} 'Skippable'?\n\nThis will mark the node as modified."),
							FText::FromString(Action)));

					if (Result == EAppReturnType::Yes)
					{
						RowData->bSkippable = bNewValue;
						MarkModified();
					}
				})
				.ToolTipText(LOCTEXT("SkippableTip", "Whether player can skip this line"))
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateNotesCell()
{
	FDialogueTableRow* RowData = RowDataEx->Data.Get();

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		.ToolTipText_Lambda([RowData]()
		{
			return RowData->Notes.IsEmpty() ? FText::GetEmpty() : FText::FromString(RowData->Notes);
		})
		[
			SNew(SEditableText)
				.Text_Lambda([RowData]()
				{
					// v4.11.4: Show "(None)" for empty values for consistency
					return RowData->Notes.IsEmpty() ? FText::FromString(TEXT("(None)")) : FText::FromString(RowData->Notes);
				})
				.HintText(LOCTEXT("NotesHint", "Designer notes..."))
				.OnTextCommitted_Lambda([this, RowData](const FText& NewText, ETextCommit::Type)
				{
					FString Value = NewText.ToString();
					// Treat "(None)" input as clearing the field
					if (Value == TEXT("(None)"))
					{
						Value = TEXT("");
					}
					RowData->Notes = Value;
					MarkModified();
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateTokenCell(FString& TokenStr, bool& bValid, ETokenCategory Category)
{
	// v4.4: Token cell with autocomplete dropdown
	// Red background for invalid tokens, normal for valid

	// Build autocomplete options from registry
	TArray<TSharedPtr<FString>> AutocompleteOptions;
	const FDialogueTokenRegistry& Registry = FDialogueTokenRegistry::Get();
	for (const FDialogueTokenSpec& Spec : Registry.GetAllSpecs())
	{
		if (Spec.Category == Category)
		{
			// Build token template with parameters
			FString Template = Spec.TokenName + TEXT("(");
			bool bFirst = true;
			for (const FDialogueTokenParam& Param : Spec.Params)
			{
				if (!bFirst) Template += TEXT(", ");
				Template += Param.ParamName.ToString() + TEXT("=");
				bFirst = false;
			}
			Template += TEXT(")");
			AutocompleteOptions.Add(MakeShared<FString>(Template));
		}
	}

	FString* TokenStrPtr = &TokenStr;
	bool* bValidPtr = &bValid;

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SEditableText)
					.Text_Lambda([TokenStrPtr]()
					{
						// Show "(None)" for empty values for consistency with NPC table
						return TokenStrPtr->IsEmpty() ? FText::FromString(TEXT("(None)")) : FText::FromString(*TokenStrPtr);
					})
					.HintText(Category == ETokenCategory::Event
						? LOCTEXT("EventsHint", "NE_BeginQuest(...)")
						: LOCTEXT("ConditionsHint", "NC_QuestState(...)"))
					.OnTextCommitted_Lambda([this, TokenStrPtr, bValidPtr](const FText& NewText, ETextCommit::Type)
					{
						FString Value = NewText.ToString();
						// Treat "(None)" input as clearing the field
						if (Value == TEXT("(None)"))
						{
							Value = TEXT("");
						}
						*TokenStrPtr = Value;
						// Validate the token
						FDialogueTokenRegistry& Reg = FDialogueTokenRegistry::Get();
						FTokenParseResult ParseResult = Reg.ParseTokenString(*TokenStrPtr);
						*bValidPtr = ParseResult.bSuccess || TokenStrPtr->IsEmpty();
						MarkModified();
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity_Lambda([bValidPtr]()
					{
						return *bValidPtr ? FSlateColor(FLinearColor::White) : FSlateColor(FLinearColor::Red);
					})
			]
			// Autocomplete dropdown button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SComboButton)
					.HasDownArrow(true)
					.ContentPadding(FMargin(2.0f, 0.0f))
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.OnGetMenuContent_Lambda([this, TokenStrPtr, bValidPtr, AutocompleteOptions, Category]() -> TSharedRef<SWidget>
					{
						TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox);

						// Add CLEAR option at top
						MenuContent->AddSlot()
						.AutoHeight()
						.Padding(2.0f)
						[
							SNew(SButton)
								.Text(LOCTEXT("ClearToken", "CLEAR"))
								.ToolTipText(LOCTEXT("ClearTooltip", "Remove all events/conditions from this node"))
								.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
								.OnClicked_Lambda([this, TokenStrPtr, bValidPtr]()
								{
									*TokenStrPtr = TEXT("CLEAR");
									*bValidPtr = true;
									MarkModified();
									FSlateApplication::Get().DismissAllMenus();
									return FReply::Handled();
								})
						];

						MenuContent->AddSlot()
						.AutoHeight()
						.Padding(2.0f)
						[
							SNew(SSeparator)
						];

						// Add token templates
						for (const TSharedPtr<FString>& Option : AutocompleteOptions)
						{
							MenuContent->AddSlot()
							.AutoHeight()
							.Padding(2.0f, 1.0f)
							[
								SNew(SButton)
									.Text(FText::FromString(*Option))
									.ButtonStyle(FAppStyle::Get(), "SimpleButton")
									.OnClicked_Lambda([this, TokenStrPtr, bValidPtr, Option]()
									{
										// If existing text, append with semicolon
										if (!TokenStrPtr->IsEmpty() && *TokenStrPtr != TEXT("CLEAR"))
										{
											*TokenStrPtr += TEXT("; ") + *Option;
										}
										else
										{
											*TokenStrPtr = *Option;
										}
										*bValidPtr = true;  // Template is valid structure
										MarkModified();
										FSlateApplication::Get().DismissAllMenus();
										return FReply::Handled();
									})
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
			]
		];
}

// v4.11.4: Helper to extract condition type from token string (e.g., "NC_HasDialogueNodePlayed" from "NC_HasDialogueNodePlayed(NodeId=X)")
static FString ExtractConditionType(const FString& TokenStr)
{
	if (TokenStr.IsEmpty())
	{
		return TEXT("");
	}
	int32 ParenIndex;
	if (TokenStr.FindChar(TEXT('('), ParenIndex))
	{
		return TokenStr.Left(ParenIndex);
	}
	return TokenStr;  // No parentheses, return as-is
}

// v4.11.4: Helper to extract options from token string (e.g., "NodeId=X" from "NC_HasDialogueNodePlayed(NodeId=X)")
static FString ExtractConditionOptions(const FString& TokenStr)
{
	if (TokenStr.IsEmpty())
	{
		return TEXT("");
	}
	int32 OpenParen, CloseParen;
	if (TokenStr.FindChar(TEXT('('), OpenParen) && TokenStr.FindLastChar(TEXT(')'), CloseParen))
	{
		if (CloseParen > OpenParen + 1)
		{
			return TokenStr.Mid(OpenParen + 1, CloseParen - OpenParen - 1);
		}
	}
	return TEXT("");  // No options found
}

TSharedRef<SWidget> SDialogueTableRow::CreateConditionTypeCell()
{
	FDialogueTableRow* RowData = RowDataEx->Data.Get();

	// Build autocomplete options from registry (conditions only)
	TArray<TSharedPtr<FString>> AutocompleteOptions;
	const FDialogueTokenRegistry& Registry = FDialogueTokenRegistry::Get();
	for (const FDialogueTokenSpec& Spec : Registry.GetAllSpecs())
	{
		if (Spec.Category == ETokenCategory::Condition)
		{
			AutocompleteOptions.Add(MakeShared<FString>(Spec.TokenName));
		}
	}

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SEditableText)
					.Text_Lambda([RowData]()
					{
						FString CondType = ExtractConditionType(RowData->ConditionsTokenStr);
						return CondType.IsEmpty() ? FText::FromString(TEXT("(None)")) : FText::FromString(CondType);
					})
					.HintText(LOCTEXT("ConditionTypeHint", "NC_..."))
					.OnTextCommitted_Lambda([this, RowData](const FText& NewText, ETextCommit::Type)
					{
						FString NewType = NewText.ToString();
						if (NewType == TEXT("(None)"))
						{
							NewType = TEXT("");
						}

						// Get existing options
						FString OldOptions = ExtractConditionOptions(RowData->ConditionsTokenStr);

						// Rebuild token string
						if (NewType.IsEmpty())
						{
							RowData->ConditionsTokenStr = TEXT("");
						}
						else if (OldOptions.IsEmpty())
						{
							RowData->ConditionsTokenStr = NewType + TEXT("()");
						}
						else
						{
							RowData->ConditionsTokenStr = NewType + TEXT("(") + OldOptions + TEXT(")");
						}

						// Validate
						FDialogueTokenRegistry& Reg = FDialogueTokenRegistry::Get();
						FTokenParseResult ParseResult = Reg.ParseTokenString(RowData->ConditionsTokenStr);
						RowData->bConditionsValid = ParseResult.bSuccess || RowData->ConditionsTokenStr.IsEmpty();
						MarkModified();
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity_Lambda([RowData]()
					{
						return RowData->bConditionsValid ? FSlateColor(FLinearColor::White) : FSlateColor(FLinearColor::Red);
					})
			]
			// Autocomplete dropdown button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SComboButton)
					.HasDownArrow(true)
					.ContentPadding(FMargin(2.0f, 0.0f))
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.OnGetMenuContent_Lambda([this, RowData, AutocompleteOptions]() -> TSharedRef<SWidget>
					{
						TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox);

						// Add CLEAR option at top
						MenuContent->AddSlot()
						.AutoHeight()
						.Padding(2.0f)
						[
							SNew(SButton)
								.Text(LOCTEXT("ClearCondition", "CLEAR"))
								.ToolTipText(LOCTEXT("ClearConditionTooltip", "Remove condition from this node"))
								.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
								.OnClicked_Lambda([this, RowData]()
								{
									RowData->ConditionsTokenStr = TEXT("");
									RowData->bConditionsValid = true;
									MarkModified();
									FSlateApplication::Get().DismissAllMenus();
									return FReply::Handled();
								})
						];

						MenuContent->AddSlot()
						.AutoHeight()
						.Padding(2.0f)
						[
							SNew(SSeparator)
						];

						// Add condition type templates
						for (const TSharedPtr<FString>& Option : AutocompleteOptions)
						{
							MenuContent->AddSlot()
							.AutoHeight()
							.Padding(2.0f, 1.0f)
							[
								SNew(SButton)
									.Text(FText::FromString(*Option))
									.ButtonStyle(FAppStyle::Get(), "SimpleButton")
									.OnClicked_Lambda([this, RowData, Option]()
									{
										// Keep existing options
										FString OldOptions = ExtractConditionOptions(RowData->ConditionsTokenStr);
										if (OldOptions.IsEmpty())
										{
											RowData->ConditionsTokenStr = *Option + TEXT("()");
										}
										else
										{
											RowData->ConditionsTokenStr = *Option + TEXT("(") + OldOptions + TEXT(")");
										}
										RowData->bConditionsValid = true;
										MarkModified();
										FSlateApplication::Get().DismissAllMenus();
										return FReply::Handled();
									})
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
			]
		];
}

TSharedRef<SWidget> SDialogueTableRow::CreateConditionOptionsCell()
{
	FDialogueTableRow* RowData = RowDataEx->Data.Get();

	return SNew(SBox)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SEditableText)
				.Text_Lambda([RowData]()
				{
					FString Options = ExtractConditionOptions(RowData->ConditionsTokenStr);
					return Options.IsEmpty() ? FText::FromString(TEXT("(None)")) : FText::FromString(Options);
				})
				.HintText(LOCTEXT("ConditionOptionsHint", "Param=Value"))
				.OnTextCommitted_Lambda([this, RowData](const FText& NewText, ETextCommit::Type)
				{
					FString NewOptions = NewText.ToString();
					if (NewOptions == TEXT("(None)"))
					{
						NewOptions = TEXT("");
					}

					// Get existing condition type
					FString CondType = ExtractConditionType(RowData->ConditionsTokenStr);

					// Rebuild token string
					if (CondType.IsEmpty())
					{
						// No condition type, can't set options without a type
						return;
					}

					RowData->ConditionsTokenStr = CondType + TEXT("(") + NewOptions + TEXT(")");

					// Validate
					FDialogueTokenRegistry& Reg = FDialogueTokenRegistry::Get();
					FTokenParseResult ParseResult = Reg.ParseTokenString(RowData->ConditionsTokenStr);
					RowData->bConditionsValid = ParseResult.bSuccess || RowData->ConditionsTokenStr.IsEmpty();
					MarkModified();
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ColorAndOpacity_Lambda([RowData]()
				{
					return RowData->bConditionsValid ? FSlateColor(FLinearColor::White) : FSlateColor(FLinearColor::Red);
				})
		];
}

void SDialogueTableRow::MarkModified()
{
	// v4.7: Update row status to Modified (unless New)
	if (RowDataEx.IsValid() && RowDataEx->Data.IsValid())
	{
		RowDataEx->Data->MarkModified();
	}
	OnRowModified.ExecuteIfBound();
}

//=============================================================================
// SDialogueTableEditor - Main Table Widget
//=============================================================================

void SDialogueTableEditor::Construct(const FArguments& InArgs)
{
	TableData = InArgs._TableData;
	OnDirtyStateChanged = InArgs._OnDirtyStateChanged;  // v4.6: Store delegate
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

		// Table - fills entire width
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(ListView, SListView<TSharedPtr<FDialogueTableRowEx>>)
				.ListItemsSource(&DisplayedRows)
				.OnGenerateRow(this, &SDialogueTableEditor::OnGenerateRow)
				.OnSelectionChanged(this, &SDialogueTableEditor::OnSelectionChanged)
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

	ApplyFlowOrder();
	UpdateStatusBar();  // v4.2.13: Initial status bar population
}

void SDialogueTableEditor::SetTableData(UDialogueTableData* InTableData)
{
	TableData = InTableData;
	SyncFromTableData();
	UpdateColumnFilterOptions();
	ApplyFlowOrder();
	UpdateStatusBar();  // v4.2.13
}

TSharedRef<SWidget> SDialogueTableEditor::BuildToolbar()
{
	return SNew(SHorizontalBox)

		// Add Row
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("AddRow", "+ Add Node"))
				.OnClicked(this, &SDialogueTableEditor::OnAddRowClicked)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
		]

		// v4.5.2: Duplicate (aligned with NPC editor)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("DuplicateRow", "Duplicate"))
				.OnClicked(this, &SDialogueTableEditor::OnDuplicateRowClicked)
				.ToolTipText(LOCTEXT("DuplicateTip", "Duplicate selected node with unique NodeID"))
		]

		// Delete (re-parents children)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("DeleteRows", "Delete"))
				.OnClicked(this, &SDialogueTableEditor::OnDeleteRowsClicked)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
				.ToolTipText(LOCTEXT("DeleteTip", "Delete selected node(s). Children are re-parented to grandparent."))
		]

		// Delete Branch (cascade delete)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("DeleteBranch", "Delete Branch"))
				.OnClicked(this, &SDialogueTableEditor::OnDeleteBranchClicked)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
				.ToolTipText(LOCTEXT("DeleteBranchTip", "Delete selected node(s) AND all their children (cascade delete)"))
		]

		// Separator
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SNew(SSeparator)
				.Orientation(Orient_Vertical)
		]

		// Reset Order
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("ResetOrder", "Reset Flow Order"))
				.OnClicked(this, &SDialogueTableEditor::OnResetOrderClicked)
				.ToolTipText(LOCTEXT("ResetOrderTip", "Sort by dialogue flow (root -> children -> grandchildren)"))
		]

		// Clear Filters
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("ClearFilters", "Clear Filters"))
				.OnClicked(this, &SDialogueTableEditor::OnClearFiltersClicked)
				.ToolTipText(LOCTEXT("ClearFiltersTip", "Clear all text and dropdown filters"))
		]

		// Spacer
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNullWidget::NullWidget
		]

		// v4.8.4: All heavy operation buttons disabled during bIsBusy
		// Validate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("Validate", "Validate"))
				.OnClicked(this, &SDialogueTableEditor::OnValidateClicked)
				.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Generate
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("Generate", "Generate Dialogues"))
				.OnClicked(this, &SDialogueTableEditor::OnGenerateClicked)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
				.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Export XLSX
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("ExportXLSX", "Export XLSX"))
				.OnClicked(this, &SDialogueTableEditor::OnExportXLSXClicked)
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
				.OnClicked(this, &SDialogueTableEditor::OnImportXLSXClicked)
				.ToolTipText(LOCTEXT("ImportXLSXTooltip", "Import from Excel format (.xlsx)"))
				.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Sync XLSX (3-way merge with conflict resolution)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("SyncXLSX", "Sync XLSX"))
				.OnClicked(this, &SDialogueTableEditor::OnSyncXLSXClicked)
				.ToolTipText(LOCTEXT("SyncXLSXTooltip", "Merge Excel changes with UE (3-way merge with conflict resolution)"))
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
				.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Separator before Asset Sync
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SNew(SSeparator)
				.Orientation(Orient_Vertical)
		]

		// Sync from Assets (v4.4 - read tokens from UDialogueBlueprint)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("SyncFromAssets", "Sync from Assets"))
				.OnClicked(this, &SDialogueTableEditor::OnSyncFromAssetsClicked)
				.ToolTipText(LOCTEXT("SyncFromAssetsTooltip", "Pull current events/conditions from UDialogueBlueprint assets into table"))
				.IsEnabled_Lambda([this]() { return !bIsBusy; })
		]

		// Apply to Assets (v4.4 - write tokens to UDialogueBlueprint)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("ApplyToAssets", "Apply to Assets"))
				.OnClicked(this, &SDialogueTableEditor::OnApplyToAssetsClicked)
				.ToolTipText(LOCTEXT("ApplyToAssetsTooltip", "Preview and apply token changes to UDialogueBlueprint assets"))
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
				.IsEnabled_Lambda([this]() { return !bIsBusy; })
		];
}

TSharedRef<SWidget> SDialogueTableEditor::BuildColumnHeaderContent(const FDialogueTableColumn& Col)
{
	FColumnFilterState& FilterState = ColumnFilters.FindOrAdd(Col.ColumnId);

	return SNew(SVerticalBox)

		// Column name
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(STextBlock)
				.Text(Col.DisplayName)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		]

		// Text filter - live filtering with OnTextChanged
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

		// Multi-select filter with checkboxes in a menu
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f, 1.0f)
		[
			SNew(SComboButton)
				.OnGetMenuContent_Lambda([this, ColumnId = Col.ColumnId]() -> TSharedRef<SWidget>
				{
					FColumnFilterState* State = ColumnFilters.Find(ColumnId);
					if (!State) return SNew(SBox);

					TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox);

					// Add "Select All" / "Clear All" buttons
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
								.OnClicked_Lambda([this, ColumnId, State]()
								{
									State->SelectedValues.Empty();
									ApplyFilters();
									return FReply::Handled();
								})
								.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(4.0f, 0.0f)
						[
							SNew(SButton)
								.Text(LOCTEXT("ClearSel", "None"))
								.OnClicked_Lambda([this, ColumnId, State]()
								{
									// Select all options (which will filter to nothing if all are selected)
									// Actually, we want "None" to mean clear selection = show all
									State->SelectedValues.Empty();
									ApplyFilters();
									return FReply::Handled();
								})
								.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						]
					];

					MenuContent->AddSlot()
					.AutoHeight()
					[
						SNew(SSeparator)
					];

					// Add checkbox for each option (skip the empty "(All)" option)
					for (const TSharedPtr<FString>& Option : State->DropdownOptions)
					{
						if (!Option.IsValid() || Option->IsEmpty()) continue;  // Skip "(All)"

						FString OptionValue = *Option;
						MenuContent->AddSlot()
						.AutoHeight()
						.Padding(4.0f, 1.0f)
						[
							SNew(SCheckBox)
								.IsChecked_Lambda([State, OptionValue]()
								{
									return State->SelectedValues.Contains(OptionValue) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
								})
								.OnCheckStateChanged_Lambda([this, State, OptionValue](ECheckBoxState NewState)
								{
									if (NewState == ECheckBoxState::Checked)
									{
										State->SelectedValues.Add(OptionValue);
									}
									else
									{
										State->SelectedValues.Remove(OptionValue);
									}
									ApplyFilters();
								})
								[
									SNew(STextBlock)
										.Text(FText::FromString(OptionValue))
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
						.Text_Lambda([&FilterState]()
						{
							if (FilterState.SelectedValues.Num() == 0)
							{
								return LOCTEXT("AllFilter", "(All)");
							}
							else if (FilterState.SelectedValues.Num() == 1)
							{
								for (const FString& Val : FilterState.SelectedValues)
								{
									return FText::FromString(Val);
								}
							}
							return FText::Format(LOCTEXT("MultiSelect", "({0} selected)"), FText::AsNumber(FilterState.SelectedValues.Num()));
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				]
		];
}

TSharedRef<SHeaderRow> SDialogueTableEditor::BuildHeaderRow()
{
	TSharedRef<SHeaderRow> Header = SNew(SHeaderRow);

	TArray<FDialogueTableColumn> Columns = GetDialogueTableColumns();

	for (const FDialogueTableColumn& Col : Columns)
	{
		Header->AddColumn(
			SHeaderRow::Column(Col.ColumnId)
				.DefaultLabel(Col.DisplayName)
				.FillWidth(Col.DefaultWidth)
				.SortMode(this, &SDialogueTableEditor::GetColumnSortMode, Col.ColumnId)
				.OnSort(this, &SDialogueTableEditor::OnColumnSortModeChanged)
				.HeaderContent()
				[
					BuildColumnHeaderContent(Col)
				]
		);
	}

	HeaderRow = Header;
	return Header;
}

TSharedRef<SWidget> SDialogueTableEditor::BuildStatusBar()
{
	// v4.2.13: Store STextBlock references for direct SetText() updates
	// This bypasses all Slate caching/invalidation issues by updating text directly
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SAssignNew(StatusTotalText, STextBlock)
				.Text(LOCTEXT("InitTotal", "Total: 0 nodes"))
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f)
		[
			SAssignNew(StatusDialoguesText, STextBlock)
				.Text(LOCTEXT("InitDialogues", "Dialogues: 0"))
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

		// v4.4: Token validation errors (shown in red when > 0)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f)
		[
			SAssignNew(StatusValidationText, STextBlock)
				.Text(LOCTEXT("InitValidation", ""))
				.ColorAndOpacity(FSlateColor(FLinearColor::Red))
		];
}

// v4.2.13: Direct SetText() update - bypasses all Slate caching issues
void SDialogueTableEditor::UpdateStatusBar()
{
	// Total nodes
	if (StatusTotalText.IsValid())
	{
		int32 TotalCount = AllRows.Num();
		StatusTotalText->SetText(FText::Format(
			LOCTEXT("TotalNodes", "Total: {0} nodes"),
			FText::AsNumber(TotalCount)
		));
	}

	// Unique dialogues count
	if (StatusDialoguesText.IsValid())
	{
		TSet<FName> UniqueDialogues;
		for (const auto& Row : AllRows)
		{
			if (Row->Data.IsValid() && !Row->Data->DialogueID.IsNone())
			{
				UniqueDialogues.Add(Row->Data->DialogueID);
			}
		}
		StatusDialoguesText->SetText(FText::Format(
			LOCTEXT("DialogueCount", "Dialogues: {0}"),
			FText::AsNumber(UniqueDialogues.Num())
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

	// v4.6: Status bar with precedence: Errors > Not validated > Out of date > Up to date
	if (StatusValidationText.IsValid())
	{
		int32 InvalidCount = 0;
		int32 UnknownCount = 0;
		int32 ActiveCount = 0;  // Non-deleted rows

		for (const auto& Row : AllRows)
		{
			if (Row->Data.IsValid() && !Row->Data->bDeleted)
			{
				ActiveCount++;
				EValidationState State = Row->Data->GetValidationState();
				if (State == EValidationState::Invalid)
				{
					InvalidCount++;
				}
				else if (State == EValidationState::Unknown)
				{
					UnknownCount++;
				}
			}
		}

		// Precedence: Errors (red) > Not validated (yellow) > Out of date (amber) > Up to date (green)
		if (InvalidCount > 0)
		{
			// Red: Validation errors exist
			StatusValidationText->SetText(FText::Format(
				LOCTEXT("ValidationErrors", "{0} errors"),
				FText::AsNumber(InvalidCount)
			));
			StatusValidationText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.2f, 0.2f)));  // Red
			StatusValidationText->SetVisibility(EVisibility::Visible);
		}
		else if (UnknownCount > 0)
		{
			// Yellow: Not yet validated
			StatusValidationText->SetText(FText::Format(
				LOCTEXT("ValidationUnknown", "Not validated ({0})"),
				FText::AsNumber(UnknownCount)
			));
			StatusValidationText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.8f, 0.2f)));  // Yellow
			StatusValidationText->SetVisibility(EVisibility::Visible);
		}
		else if (TableData && TableData->AreAssetsOutOfDate())
		{
			// Amber: Validated but assets out of date
			StatusValidationText->SetText(LOCTEXT("AssetsOutOfDate", "Out of date"));
			StatusValidationText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.6f, 0.0f)));  // Amber
			StatusValidationText->SetVisibility(EVisibility::Visible);
		}
		else if (ActiveCount > 0)
		{
			// Green: All validated and up to date
			StatusValidationText->SetText(LOCTEXT("AssetsUpToDate", "Up to date"));
			StatusValidationText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.8f, 0.2f)));  // Green
			StatusValidationText->SetVisibility(EVisibility::Visible);
		}
		else
		{
			// No active rows
			StatusValidationText->SetText(FText::GetEmpty());
			StatusValidationText->SetVisibility(EVisibility::Collapsed);
		}
	}
}

TSharedRef<ITableRow> SDialogueTableEditor::OnGenerateRow(TSharedPtr<FDialogueTableRowEx> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SDialogueTableRow, OwnerTable)
		.RowData(Item)
		.OnRowModified(FSimpleDelegate::CreateSP(this, &SDialogueTableEditor::OnRowModified));
}

void SDialogueTableEditor::OnSelectionChanged(TSharedPtr<FDialogueTableRowEx> Item, ESelectInfo::Type SelectInfo)
{
	UpdateStatusBar();  // v4.2.13: Update selected count
}

EColumnSortMode::Type SDialogueTableEditor::GetColumnSortMode(FName ColumnId) const
{
	return (SortColumn == ColumnId) ? SortMode : EColumnSortMode::None;
}

void SDialogueTableEditor::OnColumnSortModeChanged(EColumnSortPriority::Type Priority, const FName& ColumnId, EColumnSortMode::Type NewSortMode)
{
	SortColumn = ColumnId;
	SortMode = NewSortMode;
	ApplySorting();
	RefreshList();
}

void SDialogueTableEditor::RefreshList()
{
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
	UpdateStatusBar();  // v4.2.13: Explicit status bar update
}

TArray<TSharedPtr<FDialogueTableRowEx>> SDialogueTableEditor::GetSelectedRows() const
{
	TArray<TSharedPtr<FDialogueTableRowEx>> Selected;
	if (ListView.IsValid())
	{
		Selected = ListView->GetSelectedItems();
	}
	return Selected;
}

void SDialogueTableEditor::SyncFromTableData()
{
	AllRows.Empty();
	if (TableData)
	{
		for (FDialogueTableRow& Row : TableData->Rows)
		{
			TSharedPtr<FDialogueTableRowEx> RowEx = MakeShared<FDialogueTableRowEx>();
			RowEx->Data = MakeShared<FDialogueTableRow>(Row);
			AllRows.Add(RowEx);
		}
	}
	CalculateSequences();
}

void SDialogueTableEditor::SyncToTableData()
{
	if (TableData)
	{
		TableData->Rows.Empty();
		for (const TSharedPtr<FDialogueTableRowEx>& RowEx : AllRows)
		{
			if (RowEx->Data.IsValid())
			{
				TableData->Rows.Add(*RowEx->Data);
			}
		}
	}
}

void SDialogueTableEditor::CalculateSequences()
{
	// Build node lookup map per dialogue
	TMap<FName, TArray<TSharedPtr<FDialogueTableRowEx>>> DialogueGroups;

	for (TSharedPtr<FDialogueTableRowEx>& RowEx : AllRows)
	{
		if (!RowEx->Data.IsValid()) continue;
		DialogueGroups.FindOrAdd(RowEx->Data->DialogueID).Add(RowEx);
	}

	// Process each dialogue with depth-first tree traversal
	for (auto& DialoguePair : DialogueGroups)
	{
		TArray<TSharedPtr<FDialogueTableRowEx>>& Rows = DialoguePair.Value;

		// Build local node map for this dialogue
		TMap<FName, TSharedPtr<FDialogueTableRowEx>> LocalNodeMap;
		for (TSharedPtr<FDialogueTableRowEx>& RowEx : Rows)
		{
			LocalNodeMap.Add(RowEx->Data->NodeID, RowEx);
		}

		// Calculate depth for each node
		TSet<FName> Visited;
		for (TSharedPtr<FDialogueTableRowEx>& RowEx : Rows)
		{
			Visited.Empty();
			RowEx->Depth = CalculateNodeDepth(*RowEx->Data, LocalNodeMap, Visited);
		}

		// Find root nodes (no parent)
		TArray<TSharedPtr<FDialogueTableRowEx>> RootNodes;
		for (TSharedPtr<FDialogueTableRowEx>& RowEx : Rows)
		{
			if (RowEx->Data->ParentNodeID.IsNone())
			{
				RootNodes.Add(RowEx);
			}
		}

		// v4.12: Sort root nodes by GraphPosX (left-to-right visual ordering)
		RootNodes.Sort([](const TSharedPtr<FDialogueTableRowEx>& A, const TSharedPtr<FDialogueTableRowEx>& B)
		{
			if (!A->Data.IsValid() || !B->Data.IsValid()) return false;
			// Nodes without graph position go to the end
			if (!A->Data->bHasGraphPosition && !B->Data->bHasGraphPosition) return false;
			if (!A->Data->bHasGraphPosition) return false; // A goes after B
			if (!B->Data->bHasGraphPosition) return true;  // B goes after A
			return A->Data->GraphPosX < B->Data->GraphPosX;
		});

		// Depth-first traversal to assign sequence numbers
		int32 Seq = 1;
		TSet<FName> TraversalVisited;

		// Recursive lambda for tree traversal
		TFunction<void(TSharedPtr<FDialogueTableRowEx>)> TraverseNode = [&](TSharedPtr<FDialogueTableRowEx> Node)
		{
			if (!Node.IsValid() || !Node->Data.IsValid()) return;
			if (TraversalVisited.Contains(Node->Data->NodeID)) return;

			TraversalVisited.Add(Node->Data->NodeID);
			Node->Sequence = Seq++;
			Node->SeqDisplay = FString::Printf(TEXT("%d"), Node->Sequence);

			// v4.12: Collect and sort children by GraphPosX before traversing
			TArray<TSharedPtr<FDialogueTableRowEx>> ChildNodes;
			for (const FName& ChildID : Node->Data->NextNodeIDs)
			{
				TSharedPtr<FDialogueTableRowEx>* ChildPtr = LocalNodeMap.Find(ChildID);
				if (ChildPtr && ChildPtr->IsValid())
				{
					ChildNodes.Add(*ChildPtr);
				}
			}

			// Sort children by GraphPosX (left-to-right visual ordering)
			ChildNodes.Sort([](const TSharedPtr<FDialogueTableRowEx>& A, const TSharedPtr<FDialogueTableRowEx>& B)
			{
				if (!A->Data.IsValid() || !B->Data.IsValid()) return false;
				if (!A->Data->bHasGraphPosition && !B->Data->bHasGraphPosition) return false;
				if (!A->Data->bHasGraphPosition) return false;
				if (!B->Data->bHasGraphPosition) return true;
				return A->Data->GraphPosX < B->Data->GraphPosX;
			});

			// Traverse sorted children
			for (TSharedPtr<FDialogueTableRowEx>& ChildNode : ChildNodes)
			{
				TraverseNode(ChildNode);
			}
		};

		// Start traversal from sorted root nodes
		for (TSharedPtr<FDialogueTableRowEx>& RootNode : RootNodes)
		{
			TraverseNode(RootNode);
		}

		// Handle any orphan nodes not reached by traversal (assign remaining sequence numbers)
		for (TSharedPtr<FDialogueTableRowEx>& RowEx : Rows)
		{
			if (!TraversalVisited.Contains(RowEx->Data->NodeID))
			{
				RowEx->Sequence = Seq++;
				RowEx->SeqDisplay = FString::Printf(TEXT("%d*"), RowEx->Sequence); // Mark orphans with *
			}
		}
	}
}

int32 SDialogueTableEditor::CalculateNodeDepth(const FDialogueTableRow& Row, const TMap<FName, TSharedPtr<FDialogueTableRowEx>>& NodeMap, TSet<FName>& Visited)
{
	// Prevent infinite loops
	if (Visited.Contains(Row.NodeID))
	{
		return 0;
	}
	Visited.Add(Row.NodeID);

	// Root node (no parent)
	if (Row.ParentNodeID.IsNone())
	{
		return 0;
	}

	// Find parent
	const TSharedPtr<FDialogueTableRowEx>* ParentPtr = NodeMap.Find(Row.ParentNodeID);
	if (!ParentPtr || !(*ParentPtr)->Data.IsValid())
	{
		return 0; // Parent not found, treat as root
	}

	// Recursive depth calculation
	return 1 + CalculateNodeDepth(*(*ParentPtr)->Data, NodeMap, Visited);
}

void SDialogueTableEditor::InitializeColumnFilters()
{
	TArray<FDialogueTableColumn> Columns = GetDialogueTableColumns();
	for (const FDialogueTableColumn& Col : Columns)
	{
		FColumnFilterState& State = ColumnFilters.Add(Col.ColumnId);
		State.DropdownOptions.Add(MakeShared<FString>(TEXT(""))); // "(All)" option
	}
}

void SDialogueTableEditor::UpdateColumnFilterOptions()
{
	TArray<FDialogueTableColumn> Columns = GetDialogueTableColumns();

	for (const FDialogueTableColumn& Col : Columns)
	{
		FColumnFilterState* State = ColumnFilters.Find(Col.ColumnId);
		if (!State) continue;

		// Remember previous selections
		TSet<FString> PreviousSelections = State->SelectedValues;

		State->DropdownOptions.Empty();
		State->DropdownOptions.Add(MakeShared<FString>(TEXT(""))); // "(All)" placeholder

		TSet<FString> UniqueValues;
		bool bHasEmptyValues = false;
		for (const auto& RowEx : AllRows)
		{
			FString Value = GetColumnValue(*RowEx, Col.ColumnId);
			if (Value.IsEmpty())
			{
				bHasEmptyValues = true;
			}
			else
			{
				UniqueValues.Add(Value);
			}
		}

		// Add "(Empty)" option if there are empty values
		if (bHasEmptyValues)
		{
			State->DropdownOptions.Add(MakeShared<FString>(TEXT("(Empty)")));
		}

		for (const FString& Value : UniqueValues)
		{
			State->DropdownOptions.Add(MakeShared<FString>(Value));
		}

		// Restore valid selections (remove any that no longer exist)
		TSet<FString> ValidSelections;
		for (const FString& PrevSel : PreviousSelections)
		{
			for (const auto& Option : State->DropdownOptions)
			{
				if (Option.IsValid() && *Option == PrevSel)
				{
					ValidSelections.Add(PrevSel);
					break;
				}
			}
		}
		State->SelectedValues = ValidSelections;
	}
}

FString SDialogueTableEditor::GetColumnValue(const FDialogueTableRowEx& RowEx, FName ColumnId) const
{
	if (!RowEx.Data.IsValid()) return TEXT("(None)");
	const FDialogueTableRow& Row = *RowEx.Data;

	if (ColumnId == TEXT("Seq")) return RowEx.SeqDisplay;
	if (ColumnId == TEXT("Status")) return Row.GetStatusString();  // v4.7

	// DialogueID - required field, show (None) if empty
	if (ColumnId == TEXT("DialogueID"))
	{
		if (Row.DialogueID.IsNone() || Row.DialogueID.ToString().IsEmpty() || Row.DialogueID.ToString() == TEXT("None"))
		{
			return TEXT("(None)");
		}
		return Row.DialogueID.ToString();
	}

	// NodeID - required field, show (None) if empty
	if (ColumnId == TEXT("NodeID"))
	{
		if (Row.NodeID.IsNone() || Row.NodeID.ToString().IsEmpty() || Row.NodeID.ToString() == TEXT("None"))
		{
			return TEXT("(None)");
		}
		return Row.NodeID.ToString();
	}

	if (ColumnId == TEXT("NodeType")) return Row.NodeType == EDialogueTableNodeType::NPC ? TEXT("NPC") : TEXT("Player");

	// Speaker - show "Player" for player nodes, (None) for empty NPC speaker
	if (ColumnId == TEXT("Speaker"))
	{
		if (Row.NodeType == EDialogueTableNodeType::Player)
		{
			if (Row.Speaker.IsNone() || Row.Speaker.ToString().IsEmpty() || Row.Speaker.ToString() == TEXT("None"))
			{
				return TEXT("Player");
			}
		}
		else // NPC node
		{
			if (Row.Speaker.IsNone() || Row.Speaker.ToString().IsEmpty() || Row.Speaker.ToString() == TEXT("None"))
			{
				return TEXT("(None)");
			}
		}
		return Row.Speaker.ToString();
	}

	// Text - show (None) if empty (important for dialogue)
	if (ColumnId == TEXT("Text"))
	{
		return Row.Text.IsEmpty() ? TEXT("(None)") : Row.Text;
	}

	// v4.11.4: Show "(None)" for empty values for consistency
	// OptionText - show (None) if empty for all nodes
	if (ColumnId == TEXT("OptionText"))
	{
		return Row.OptionText.IsEmpty() ? TEXT("(None)") : Row.OptionText;
	}

	// v4.11.4: Show "(None)" for empty values for consistency
	// ParentNodeID - empty for root nodes
	if (ColumnId == TEXT("ParentNodeID"))
	{
		if (Row.ParentNodeID.IsNone() || Row.ParentNodeID.ToString().IsEmpty() || Row.ParentNodeID.ToString() == TEXT("None"))
		{
			return TEXT("(None)");
		}
		return Row.ParentNodeID.ToString();
	}

	// NextNodeIDs - empty for leaf nodes
	if (ColumnId == TEXT("NextNodeIDs"))
	{
		if (Row.NextNodeIDs.Num() == 0)
		{
			return TEXT("(None)");
		}
		FString Result;
		for (int32 i = 0; i < Row.NextNodeIDs.Num(); i++)
		{
			if (i > 0) Result += TEXT(", ");
			Result += Row.NextNodeIDs[i].ToString();
		}
		return Result;
	}

	if (ColumnId == TEXT("Skippable")) return Row.bSkippable ? TEXT("Yes") : TEXT("No");

	// Events - show (None) if empty
	if (ColumnId == TEXT("Events"))
	{
		return Row.EventsTokenStr.IsEmpty() ? TEXT("(None)") : Row.EventsTokenStr;
	}

	// v4.11.4: Split Conditions into Condition (type) and Options (parameters)
	if (ColumnId == TEXT("Condition"))
	{
		FString CondType = ExtractConditionType(Row.ConditionsTokenStr);
		return CondType.IsEmpty() ? TEXT("(None)") : CondType;
	}

	if (ColumnId == TEXT("Options"))
	{
		FString Options = ExtractConditionOptions(Row.ConditionsTokenStr);
		return Options.IsEmpty() ? TEXT("(None)") : Options;
	}

	// Notes - optional, show (None) if empty for consistency
	if (ColumnId == TEXT("Notes"))
	{
		return Row.Notes.IsEmpty() ? TEXT("(None)") : Row.Notes;
	}

	return TEXT("(None)");
}

void SDialogueTableEditor::OnColumnTextFilterChanged(FName ColumnId, const FText& NewText)
{
	FColumnFilterState* State = ColumnFilters.Find(ColumnId);
	if (State)
	{
		State->TextFilter = NewText.ToString();
		ApplyFilters();
	}
}

void SDialogueTableEditor::OnColumnDropdownFilterChanged(FName ColumnId, TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	// Legacy function - kept for compatibility but multi-select uses SelectedValues directly
	FColumnFilterState* State = ColumnFilters.Find(ColumnId);
	if (State && NewValue.IsValid() && !NewValue->IsEmpty())
	{
		State->SelectedValues.Empty();
		State->SelectedValues.Add(*NewValue);
		ApplyFilters();
	}
}

void SDialogueTableEditor::ApplyFilters()
{
	DisplayedRows.Empty();

	for (TSharedPtr<FDialogueTableRowEx>& RowEx : AllRows)
	{
		bool bPassesAllFilters = true;

		for (const auto& FilterPair : ColumnFilters)
		{
			const FName& ColumnId = FilterPair.Key;
			const FColumnFilterState& FilterState = FilterPair.Value;

			FString ColumnValue = GetColumnValue(*RowEx, ColumnId);

			// Text filter
			if (!FilterState.TextFilter.IsEmpty())
			{
				if (!ColumnValue.ToLower().Contains(FilterState.TextFilter.ToLower()))
				{
					bPassesAllFilters = false;
					break;
				}
			}

			// Multi-select dropdown filter (empty = all pass, otherwise OR logic)
			if (FilterState.SelectedValues.Num() > 0)
			{
				bool bMatchesAny = false;
				for (const FString& SelectedValue : FilterState.SelectedValues)
				{
					// Special handling for "(Empty)" filter
					if (SelectedValue == TEXT("(Empty)"))
					{
						if (ColumnValue.IsEmpty())
						{
							bMatchesAny = true;
							break;
						}
					}
					else if (ColumnValue == SelectedValue)
					{
						bMatchesAny = true;
						break;
					}
				}
				if (!bMatchesAny)
				{
					bPassesAllFilters = false;
					break;
				}
			}
		}

		if (bPassesAllFilters)
		{
			DisplayedRows.Add(RowEx);
		}
	}

	ApplySorting();
	RefreshList();
}

void SDialogueTableEditor::ApplySorting()
{
	if (SortColumn == NAME_None || SortMode == EColumnSortMode::None)
	{
		return;
	}

	bool bAscending = (SortMode == EColumnSortMode::Ascending);

	DisplayedRows.Sort([this, bAscending](const TSharedPtr<FDialogueTableRowEx>& A, const TSharedPtr<FDialogueTableRowEx>& B)
	{
		FString ValueA = GetColumnValue(*A, SortColumn);
		FString ValueB = GetColumnValue(*B, SortColumn);
		return bAscending ? (ValueA < ValueB) : (ValueA > ValueB);
	});
}

void SDialogueTableEditor::ApplyFlowOrder()
{
	// Reset sort state
	SortColumn = NAME_None;
	SortMode = EColumnSortMode::None;

	// First apply filters to get the filtered set
	DisplayedRows.Empty();
	for (TSharedPtr<FDialogueTableRowEx>& RowEx : AllRows)
	{
		bool bPassesAllFilters = true;

		for (const auto& FilterPair : ColumnFilters)
		{
			const FName& ColumnId = FilterPair.Key;
			const FColumnFilterState& FilterState = FilterPair.Value;

			FString ColumnValue = GetColumnValue(*RowEx, ColumnId);

			// Text filter
			if (!FilterState.TextFilter.IsEmpty())
			{
				if (!ColumnValue.ToLower().Contains(FilterState.TextFilter.ToLower()))
				{
					bPassesAllFilters = false;
					break;
				}
			}

			// Multi-select dropdown filter (empty = all pass, otherwise OR logic)
			if (FilterState.SelectedValues.Num() > 0)
			{
				bool bMatchesAny = false;
				for (const FString& SelectedValue : FilterState.SelectedValues)
				{
					// Special handling for "(Empty)" filter
					if (SelectedValue == TEXT("(Empty)"))
					{
						if (ColumnValue.IsEmpty())
						{
							bMatchesAny = true;
							break;
						}
					}
					else if (ColumnValue == SelectedValue)
					{
						bMatchesAny = true;
						break;
					}
				}
				if (!bMatchesAny)
				{
					bPassesAllFilters = false;
					break;
				}
			}
		}

		if (bPassesAllFilters)
		{
			DisplayedRows.Add(RowEx);
		}
	}

	// Then sort by DialogueID first, then by Sequence (flow order)
	DisplayedRows.Sort([](const TSharedPtr<FDialogueTableRowEx>& A, const TSharedPtr<FDialogueTableRowEx>& B)
	{
		if (!A->Data.IsValid() || !B->Data.IsValid()) return false;

		// First by DialogueID
		FString DialogueA = A->Data->DialogueID.ToString();
		FString DialogueB = B->Data->DialogueID.ToString();
		if (DialogueA != DialogueB)
		{
			return DialogueA < DialogueB;
		}

		// Then by Sequence (tree traversal order)
		return A->Sequence < B->Sequence;
	});

	RefreshList();
}

FReply SDialogueTableEditor::OnResetOrderClicked()
{
	ApplyFlowOrder();
	return FReply::Handled();
}

void SDialogueTableEditor::MarkDirty()
{
	SyncToTableData();
	if (TableData)
	{
		TableData->MarkPackageDirty();

		// v4.6: Notify owner of dirty state change
		OnDirtyStateChanged.ExecuteIfBound();
	}
}

void SDialogueTableEditor::OnRowModified()
{
	CalculateSequences();
	UpdateColumnFilterOptions();
	MarkDirty();
}

//=============================================================================
// Actions
//=============================================================================

FReply SDialogueTableEditor::OnAddRowClicked()
{
	TSharedPtr<FDialogueTableRowEx> NewRowEx = MakeShared<FDialogueTableRowEx>();
	NewRowEx->Data = MakeShared<FDialogueTableRow>();

	TArray<TSharedPtr<FDialogueTableRowEx>> Selected = GetSelectedRows();
	int32 InsertIndex = AllRows.Num(); // Default: add to end

	if (Selected.Num() > 0)
	{
		// Smart Add: Insert below selected row with auto-populated fields
		TSharedPtr<FDialogueTableRowEx> SelectedRow = Selected[0];
		FDialogueTableRow* ParentData = SelectedRow->Data.Get();

		if (ParentData)
		{
			// Copy DialogueID from selected row
			NewRowEx->Data->DialogueID = ParentData->DialogueID;

			// Set ParentNodeID to selected row's NodeID
			NewRowEx->Data->ParentNodeID = ParentData->NodeID;

			// Toggle NodeType (NPC -> Player, Player -> NPC)
			NewRowEx->Data->NodeType = (ParentData->NodeType == EDialogueTableNodeType::NPC)
				? EDialogueTableNodeType::Player
				: EDialogueTableNodeType::NPC;

			// For NPC nodes, find and copy the nearest NPC speaker from ancestors
			if (NewRowEx->Data->NodeType == EDialogueTableNodeType::NPC)
			{
				// Build node map for this dialogue
				TMap<FName, TSharedPtr<FDialogueTableRow>> NodeMap;
				for (const auto& Row : AllRows)
				{
					if (Row->Data.IsValid() && Row->Data->DialogueID == ParentData->DialogueID)
					{
						NodeMap.Add(Row->Data->NodeID, Row->Data);
					}
				}

				// Walk up the tree to find nearest NPC speaker
				FName CurrentNodeID = ParentData->NodeID;
				TSet<FName> Visited;
				while (!CurrentNodeID.IsNone() && !Visited.Contains(CurrentNodeID))
				{
					Visited.Add(CurrentNodeID);
					TSharedPtr<FDialogueTableRow>* NodePtr = NodeMap.Find(CurrentNodeID);
					if (NodePtr && NodePtr->IsValid())
					{
						FDialogueTableRow* Node = NodePtr->Get();
						if (Node->NodeType == EDialogueTableNodeType::NPC && !Node->Speaker.IsNone())
						{
							NewRowEx->Data->Speaker = Node->Speaker;
							break;
						}
						CurrentNodeID = Node->ParentNodeID;
					}
					else
					{
						break;
					}
				}
			}

			// Generate unique NodeID
			FString BaseID = TEXT("new_node");
			int32 Counter = 1;
			FName NewNodeID = FName(*FString::Printf(TEXT("%s_%d"), *BaseID, Counter));

			// Check for uniqueness
			TSet<FName> ExistingIDs;
			for (const auto& Row : AllRows)
			{
				if (Row->Data.IsValid())
				{
					ExistingIDs.Add(Row->Data->NodeID);
				}
			}
			while (ExistingIDs.Contains(NewNodeID))
			{
				Counter++;
				NewNodeID = FName(*FString::Printf(TEXT("%s_%d"), *BaseID, Counter));
			}
			NewRowEx->Data->NodeID = NewNodeID;

			// Prepend new NodeID to parent's NextNodeIDs (so it appears right after parent in flow)
			if (!ParentData->NextNodeIDs.Contains(NewNodeID))
			{
				ParentData->NextNodeIDs.Insert(NewNodeID, 0);
			}

			// Find insert position (after selected row)
			InsertIndex = AllRows.IndexOfByKey(SelectedRow);
			if (InsertIndex != INDEX_NONE)
			{
				InsertIndex++; // Insert after selected
			}
			else
			{
				InsertIndex = AllRows.Num();
			}
		}
	}
	else
	{
		// No selection: Pre-fill DialogueID from filter if set
		FColumnFilterState* DialogueFilter = ColumnFilters.Find(TEXT("DialogueID"));
		if (DialogueFilter && DialogueFilter->SelectedValues.Num() == 1)
		{
			// Use first selected value if only one is selected
			for (const FString& Val : DialogueFilter->SelectedValues)
			{
				NewRowEx->Data->DialogueID = FName(*Val);
				break;
			}
		}

		// Generate unique NodeID for root node
		FString BaseID = TEXT("root");
		int32 Counter = 1;
		FName NewNodeID = FName(*BaseID);

		TSet<FName> ExistingIDs;
		for (const auto& Row : AllRows)
		{
			if (Row->Data.IsValid())
			{
				ExistingIDs.Add(Row->Data->NodeID);
			}
		}
		if (ExistingIDs.Contains(NewNodeID))
		{
			while (ExistingIDs.Contains(NewNodeID))
			{
				Counter++;
				NewNodeID = FName(*FString::Printf(TEXT("%s_%d"), *BaseID, Counter));
			}
		}
		NewRowEx->Data->NodeID = NewNodeID;
	}

	// Insert at calculated position in AllRows
	if (InsertIndex >= 0 && InsertIndex < AllRows.Num())
	{
		AllRows.Insert(NewRowEx, InsertIndex);
	}
	else
	{
		AllRows.Add(NewRowEx);
	}

	// Also insert into DisplayedRows at the same relative position (don't re-sort)
	if (Selected.Num() > 0)
	{
		// Find position in DisplayedRows (after selected row)
		int32 DisplayIndex = DisplayedRows.IndexOfByKey(Selected[0]);
		if (DisplayIndex != INDEX_NONE)
		{
			DisplayedRows.Insert(NewRowEx, DisplayIndex + 1);
		}
		else
		{
			DisplayedRows.Add(NewRowEx);
		}
	}
	else
	{
		DisplayedRows.Add(NewRowEx);
	}

	CalculateSequences();
	UpdateColumnFilterOptions();
	RefreshList();  // Just refresh, don't re-sort
	MarkDirty();

	// Select the new row
	if (ListView.IsValid())
	{
		ListView->SetSelection(NewRowEx);
		ListView->RequestScrollIntoView(NewRowEx);
	}

	return FReply::Handled();
}

// v4.5.2: Duplicate row (aligned with NPC editor)
FReply SDialogueTableEditor::OnDuplicateRowClicked()
{
	TArray<TSharedPtr<FDialogueTableRowEx>> Selected = GetSelectedRows();

	if (Selected.Num() == 0 || !TableData)
	{
		return FReply::Handled();
	}

	// Find index in TableData
	SyncToTableData();  // Ensure TableData is up to date
	int32 SourceIndex = TableData->FindRowIndexByGuid(Selected[0]->Data->RowId);
	if (SourceIndex != INDEX_NONE)
	{
		FDialogueTableRow* NewRow = TableData->DuplicateRow(SourceIndex);
		if (NewRow)
		{
			// Create a new RowEx for the duplicated row
			TSharedPtr<FDialogueTableRowEx> NewRowEx = MakeShared<FDialogueTableRowEx>();
			NewRowEx->Data = MakeShared<FDialogueTableRow>(*NewRow);
			AllRows.Add(NewRowEx);
		}
	}

	ApplyFilters();
	MarkDirty();
	return FReply::Handled();
}

FReply SDialogueTableEditor::OnDeleteRowsClicked()
{
	TArray<TSharedPtr<FDialogueTableRowEx>> Selected = GetSelectedRows();

	if (Selected.Num() == 0)
	{
		return FReply::Handled();
	}

	// Show confirmation prompt
	FText Message = FText::Format(
		LOCTEXT("DeleteConfirm", "Are you sure you want to delete {0} node(s)?\nChildren will be re-parented to the grandparent."),
		FText::AsNumber(Selected.Num())
	);

	if (FMessageDialog::Open(EAppMsgType::YesNo, Message) != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	// Build node map for finding parents and children
	TMap<FName, TSharedPtr<FDialogueTableRowEx>> NodeMap;
	for (const auto& Row : AllRows)
	{
		if (Row->Data.IsValid())
		{
			FName UniqueKey = FName(*FString::Printf(TEXT("%s.%s"),
				*Row->Data->DialogueID.ToString(), *Row->Data->NodeID.ToString()));
			NodeMap.Add(UniqueKey, Row);
		}
	}

	for (const TSharedPtr<FDialogueTableRowEx>& RowEx : Selected)
	{
		if (!RowEx->Data.IsValid()) continue;

		FName DeletedNodeID = RowEx->Data->NodeID;
		FName GrandparentID = RowEx->Data->ParentNodeID;
		FName DialogueID = RowEx->Data->DialogueID;

		// Find parent and remove this node from parent's NextNodeIDs
		TSharedPtr<FDialogueTableRowEx>* ParentPtr = nullptr;
		if (!GrandparentID.IsNone())
		{
			FName ParentKey = FName(*FString::Printf(TEXT("%s.%s"),
				*DialogueID.ToString(), *GrandparentID.ToString()));
			ParentPtr = NodeMap.Find(ParentKey);
			if (ParentPtr && (*ParentPtr)->Data.IsValid())
			{
				(*ParentPtr)->Data->NextNodeIDs.Remove(DeletedNodeID);
			}
		}

		// Re-parent children: update their ParentNodeID to grandparent, add them to grandparent's NextNodeIDs
		for (const auto& Row : AllRows)
		{
			if (Row->Data.IsValid() &&
				Row->Data->DialogueID == DialogueID &&
				Row->Data->ParentNodeID == DeletedNodeID)
			{
				// This row's parent is being deleted - re-parent to grandparent
				Row->Data->ParentNodeID = GrandparentID;

				// Add this child to grandparent's NextNodeIDs (if grandparent exists)
				if (ParentPtr && (*ParentPtr)->Data.IsValid())
				{
					if (!(*ParentPtr)->Data->NextNodeIDs.Contains(Row->Data->NodeID))
					{
						(*ParentPtr)->Data->NextNodeIDs.Add(Row->Data->NodeID);
					}
				}
			}
		}

		// Remove from AllRows and DisplayedRows
		AllRows.Remove(RowEx);
		DisplayedRows.Remove(RowEx);
	}

	CalculateSequences();
	UpdateColumnFilterOptions();
	RefreshList();
	MarkDirty();

	return FReply::Handled();
}

FReply SDialogueTableEditor::OnDeleteBranchClicked()
{
	TArray<TSharedPtr<FDialogueTableRowEx>> Selected = GetSelectedRows();

	if (Selected.Num() == 0)
	{
		return FReply::Handled();
	}

	// Collect all nodes to delete (selected + all descendants)
	TSet<FName> NodesToDelete;  // DialogueID.NodeID keys

	// Build node map
	TMap<FName, TSharedPtr<FDialogueTableRowEx>> NodeMap;
	for (const auto& Row : AllRows)
	{
		if (Row->Data.IsValid())
		{
			FName UniqueKey = FName(*FString::Printf(TEXT("%s.%s"),
				*Row->Data->DialogueID.ToString(), *Row->Data->NodeID.ToString()));
			NodeMap.Add(UniqueKey, Row);
		}
	}

	// Recursive function to collect node and all descendants
	TFunction<void(FName DialogueID, FName NodeID)> CollectBranch = [&](FName DialogueID, FName NodeID)
	{
		FName Key = FName(*FString::Printf(TEXT("%s.%s"), *DialogueID.ToString(), *NodeID.ToString()));
		if (NodesToDelete.Contains(Key)) return;  // Already collected
		NodesToDelete.Add(Key);

		// Find all children (nodes whose ParentNodeID matches this NodeID)
		for (const auto& Row : AllRows)
		{
			if (Row->Data.IsValid() &&
				Row->Data->DialogueID == DialogueID &&
				Row->Data->ParentNodeID == NodeID)
			{
				CollectBranch(DialogueID, Row->Data->NodeID);
			}
		}
	};

	// Collect all branches starting from selected nodes
	for (const TSharedPtr<FDialogueTableRowEx>& RowEx : Selected)
	{
		if (RowEx->Data.IsValid())
		{
			CollectBranch(RowEx->Data->DialogueID, RowEx->Data->NodeID);
		}
	}

	// Show confirmation with count
	FText Message = FText::Format(
		LOCTEXT("DeleteBranchConfirm", "Are you sure you want to delete {0} node(s) and ALL their descendants?\n(Total nodes to delete: {1})"),
		FText::AsNumber(Selected.Num()),
		FText::AsNumber(NodesToDelete.Num())
	);

	if (FMessageDialog::Open(EAppMsgType::YesNo, Message) != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	// Remove from parent's NextNodeIDs for root nodes of deletion
	for (const TSharedPtr<FDialogueTableRowEx>& RowEx : Selected)
	{
		if (!RowEx->Data.IsValid()) continue;

		if (!RowEx->Data->ParentNodeID.IsNone())
		{
			FName ParentKey = FName(*FString::Printf(TEXT("%s.%s"),
				*RowEx->Data->DialogueID.ToString(), *RowEx->Data->ParentNodeID.ToString()));
			TSharedPtr<FDialogueTableRowEx>* ParentPtr = NodeMap.Find(ParentKey);
			if (ParentPtr && (*ParentPtr)->Data.IsValid())
			{
				(*ParentPtr)->Data->NextNodeIDs.Remove(RowEx->Data->NodeID);
			}
		}
	}

	// Delete all collected nodes
	AllRows.RemoveAll([&NodesToDelete](const TSharedPtr<FDialogueTableRowEx>& Row)
	{
		if (!Row->Data.IsValid()) return false;
		FName Key = FName(*FString::Printf(TEXT("%s.%s"),
			*Row->Data->DialogueID.ToString(), *Row->Data->NodeID.ToString()));
		return NodesToDelete.Contains(Key);
	});

	DisplayedRows.RemoveAll([&NodesToDelete](const TSharedPtr<FDialogueTableRowEx>& Row)
	{
		if (!Row->Data.IsValid()) return false;
		FName Key = FName(*FString::Printf(TEXT("%s.%s"),
			*Row->Data->DialogueID.ToString(), *Row->Data->NodeID.ToString()));
		return NodesToDelete.Contains(Key);
	});

	CalculateSequences();
	UpdateColumnFilterOptions();
	RefreshList();
	MarkDirty();
	return FReply::Handled();
}

FReply SDialogueTableEditor::OnClearFiltersClicked()
{
	// Clear all text and dropdown filters
	for (auto& FilterPair : ColumnFilters)
	{
		FilterPair.Value.TextFilter.Empty();
		FilterPair.Value.SelectedValues.Empty();  // Clear multi-select
	}

	// Apply filters to refresh the view
	ApplyFlowOrder();
	return FReply::Handled();
}

FReply SDialogueTableEditor::OnValidateClicked()
{
	// v4.8.4: Re-entrancy guard
	if (bIsBusy) return FReply::Handled();
	TGuardValue<bool> BusyGuard(bIsBusy, true);

	SyncToTableData();

	if (!TableData || TableData->Rows.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoData", "No dialogue data to validate."));
		return FReply::Handled();
	}

	// v4.5.2: Ensure ListsVersionGuid is valid (aligned with NPC editor)
	if (!TableData->ListsVersionGuid.IsValid())
	{
		TableData->ListsVersionGuid = FGuid::NewGuid();
	}

	// v4.5: Use cache-writing validation
	FDialogueValidationResult Result = FDialogueTableValidator::ValidateAllAndCache(
		TableData->Rows,
		TableData->ListsVersionGuid
	);

	// Refresh UI to show validation state
	RefreshList();
	UpdateStatusBar();

	// v4.5.2: Aligned message format with NPC editor
	int32 ErrorCount = Result.GetErrorCount();
	int32 WarningCount = Result.GetWarningCount();

	FString Message;
	if (ErrorCount == 0 && WarningCount == 0)
	{
		Message = TEXT("All rows passed validation!");
	}
	else
	{
		Message = FString::Printf(TEXT("Validation found %d error(s) and %d warning(s):\n\n"), ErrorCount, WarningCount);

		// Show errors first (up to 10 total)
		int32 ShownCount = 0;
		for (const FDialogueValidationIssue& Issue : Result.Issues)
		{
			if (ShownCount >= 10) break;
			if (Issue.Severity == EDialogueValidationSeverity::Error)
			{
				Message += Issue.ToString() + TEXT("\n");
				ShownCount++;
			}
		}

		// Then warnings
		for (const FDialogueValidationIssue& Issue : Result.Issues)
		{
			if (ShownCount >= 10) break;
			if (Issue.Severity == EDialogueValidationSeverity::Warning)
			{
				Message += Issue.ToString() + TEXT("\n");
				ShownCount++;
			}
		}

		if (Result.Issues.Num() > 10)
		{
			Message += FString::Printf(TEXT("\n... and %d more issues"), Result.Issues.Num() - 10);
		}
	}

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));
	return FReply::Handled();
}

FReply SDialogueTableEditor::OnGenerateClicked()
{
	// v4.8.4: Re-entrancy guard
	if (bIsBusy) return FReply::Handled();
	TGuardValue<bool> BusyGuard(bIsBusy, true);

	// v4.6: Save before Generate, validation gate dialog, soft delete support

	//=========================================================================
	// Step 0: Silent save before generation (Rule #1)
	//=========================================================================
	SyncToTableData();

	// v4.8.3: Empty guard
	if (!TableData || TableData->Rows.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoRowsToGenerateDialogue", "No rows to generate."));
		return FReply::Handled();
	}

	if (TableData)
	{
		UPackage* Package = TableData->GetPackage();
		if (Package && Package->IsDirty())
		{
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Standalone;
			FString PackageFileName = FPackageName::LongPackageNameToFilename(
				Package->GetName(), FPackageName::GetAssetPackageExtension());
			bool bSaveSuccess = UPackage::SavePackage(Package, TableData, *PackageFileName, SaveArgs);

			// v4.8.4: Abort generation if save fails
			if (!bSaveSuccess)
			{
				FMessageDialog::Open(EAppMsgType::Ok,
					LOCTEXT("SaveFailedBeforeGenerateDialogue", "Failed to save TableData before generation.\n\nGeneration aborted to prevent data loss."));
				return FReply::Handled();
			}
			UE_LOG(LogTemp, Log, TEXT("Dialogue Table Editor: Auto-saved TableData before generation"));
		}
	}

	//=========================================================================
	// Step 1: Gather all rows (skip soft-deleted)
	//=========================================================================
	TArray<FDialogueTableRow> RowsToValidate;
	int32 DeletedCount = 0;
	if (TableData)
	{
		for (const FDialogueTableRow& Row : TableData->Rows)
		{
			if (Row.bDeleted)
			{
				DeletedCount++;
				continue;  // v4.6: Skip soft-deleted rows
			}
			RowsToValidate.Add(Row);
		}
	}

	if (RowsToValidate.Num() == 0)
	{
		FString Message = DeletedCount > 0
			? FString::Printf(TEXT("No active dialogue rows to generate. All %d rows are soft-deleted."), DeletedCount)
			: TEXT("No dialogue data to generate.");
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));
		return FReply::Handled();
	}

	//=========================================================================
	// Step 2: Validate with cache-writing
	//=========================================================================
	if (!TableData->ListsVersionGuid.IsValid())
	{
		TableData->ListsVersionGuid = FGuid::NewGuid();
	}

	FDialogueValidationResult ValidationResult = FDialogueTableValidator::ValidateAllAndCache(
		RowsToValidate,
		TableData->ListsVersionGuid
	);
	RefreshList();
	UpdateStatusBar();

	int32 ValidationErrorCount = ValidationResult.GetErrorCount();

	if (ValidationErrorCount > 0)
	{
		// v4.6: Show validation gate dialog with override option
		FString ErrorMessage = FString::Printf(TEXT("Validation found %d error(s).\n\n"), ValidationErrorCount);

		// Show first 5 errors
		int32 ShownErrors = 0;
		for (const FDialogueValidationIssue& Issue : ValidationResult.Issues)
		{
			if (Issue.Severity == EDialogueValidationSeverity::Error && ShownErrors < 5)
			{
				ErrorMessage += FString::Printf(TEXT("[ERROR] %s.%s: %s\n"),
					*Issue.DialogueID.ToString(), *Issue.NodeID.ToString(), *Issue.Message);
				ShownErrors++;
			}
		}
		if (ValidationErrorCount > 5)
		{
			ErrorMessage += FString::Printf(TEXT("\n... and %d more errors\n"), ValidationErrorCount - 5);
		}

		ErrorMessage += TEXT("\nErrors may cause incomplete or broken asset generation.\n\n");
		ErrorMessage += TEXT("Click 'Yes' to generate anyway (not recommended)\n");
		ErrorMessage += TEXT("Click 'No' to cancel and fix issues first");

		EAppReturnType::Type Response = FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(ErrorMessage));
		if (Response != EAppReturnType::Yes)
		{
			return FReply::Handled();
		}
		// User chose to generate anyway - continue with generation
		UE_LOG(LogTemp, Warning, TEXT("Dialogue Table Editor: User overrode validation gate with %d errors"), ValidationErrorCount);
	}

	//=========================================================================
	// Step 3: Convert rows to manifest definitions
	//=========================================================================
	TMap<FName, FManifestDialogueBlueprintDefinition> Definitions =
		FDialogueTableConverter::ConvertRowsToManifest(RowsToValidate);

	FString Message = FString::Printf(TEXT("Ready to generate %d dialogue(s):\n\n"), Definitions.Num());
	for (const auto& Pair : Definitions)
	{
		Message += FString::Printf(TEXT("- %s (%d nodes)\n"),
			*Pair.Key.ToString(),
			Pair.Value.DialogueTree.Nodes.Num());
	}

	if (DeletedCount > 0)
	{
		Message += FString::Printf(TEXT("\nSoft-deleted (skipped): %d"), DeletedCount);
	}

	//=========================================================================
	// Step 4: Generate dialogue assets (v4.10)
	//=========================================================================
	UE_LOG(LogGasAbilityGenerator, Log, TEXT("Generating Dialogue assets from Table Editor"));

	int32 SuccessCount = 0;
	int32 SkippedCount = 0;
	int32 FailCount = 0;

	for (const auto& Pair : Definitions)
	{
		FGenerationResult Result = FDialogueBlueprintGenerator::Generate(
			Pair.Value,
			TEXT(""),   // Empty = auto-detect via GetProjectRoot()
			nullptr     // ManifestData not used
		);

		if (Result.Status == EGenerationStatus::New)
		{
			SuccessCount++;
			UE_LOG(LogGasAbilityGenerator, Log, TEXT("Generated: %s"), *Pair.Key.ToString());
		}
		else if (Result.Status == EGenerationStatus::Skipped)
		{
			SkippedCount++;
			UE_LOG(LogGasAbilityGenerator, Log, TEXT("Skipped (exists): %s"), *Pair.Key.ToString());
		}
		else if (Result.Status == EGenerationStatus::Failed)
		{
			FailCount++;
			UE_LOG(LogGasAbilityGenerator, Warning, TEXT("Failed: %s - %s"), *Pair.Key.ToString(), *Result.Message);
		}
	}

	// v4.6: Update generation tracking
	if (TableData)
	{
		TableData->OnGenerationComplete(FailCount);
	}

	Message += FString::Printf(TEXT("\n\nGeneration Complete:\n"));
	Message += FString::Printf(TEXT("  Created: %d\n"), SuccessCount);
	Message += FString::Printf(TEXT("  Skipped (exists): %d\n"), SkippedCount);
	Message += FString::Printf(TEXT("  Failed: %d"), FailCount);

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));

	return FReply::Handled();
}

FReply SDialogueTableEditor::OnExportXLSXClicked()
{
	// v4.8.4: Re-entrancy guard
	if (bIsBusy) return FReply::Handled();
	TGuardValue<bool> BusyGuard(bIsBusy, true);

	SyncToTableData();

	// v4.8.3: Empty guard
	if (!TableData || TableData->Rows.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoRowsToExport", "No rows to export."));
		return FReply::Handled();
	}

	TArray<FString> OutFiles;
	FDesktopPlatformModule::Get()->SaveFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Export Dialogue Table to Excel"),
		FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_EXPORT),
		TEXT("DialogueTable.xlsx"),
		TEXT("Excel Files (*.xlsx)|*.xlsx"),
		EFileDialogFlags::None,
		OutFiles
	);

	if (OutFiles.Num() == 0)
	{
		return FReply::Handled();
	}

	// Get rows from TableData
	TArray<FDialogueTableRow> Rows;
	if (TableData)
	{
		Rows = TableData->Rows;
	}

	FString ErrorMsg;
	if (FDialogueXLSXWriter::ExportToXLSX(Rows, OutFiles[0], ErrorMsg))
	{
		FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_EXPORT, FPaths::GetPath(OutFiles[0]));

		// Update source file path
		if (TableData)
		{
			TableData->SourceFilePath = OutFiles[0];

			// v4.11: Update LastSyncedHash on all rows after successful export
			// This enables 3-way merge to detect changes since export
			for (FDialogueTableRow& Row : TableData->Rows)
			{
				Row.LastSyncedHash = Row.ComputeSyncHash();
			}
			MarkDirty();
		}

		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(LOCTEXT("XLSXExportSuccess", "Exported {0} rows to:\n{1}"),
				FText::AsNumber(Rows.Num()),
				FText::FromString(OutFiles[0])));
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(LOCTEXT("XLSXExportFailed", "Export failed:\n{0}"),
				FText::FromString(ErrorMsg)));
	}

	return FReply::Handled();
}

FReply SDialogueTableEditor::OnImportXLSXClicked()
{
	// v4.11: Import now uses sync comparison like NPC Table Editor
	// Previously directly replaced TableData->Rows without comparing to UE data
	// Now shows sync approval dialog for user to review all changes

	// v4.8.4: Re-entrancy guard
	if (bIsBusy) return FReply::Handled();
	TGuardValue<bool> BusyGuard(bIsBusy, true);

	// Sync pending UI changes to TableData first
	SyncToTableData();

	TArray<FString> OutFiles;
	FDesktopPlatformModule::Get()->OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Import Dialogue Table from Excel"),
		FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_IMPORT),
		TEXT(""),
		TEXT("Excel Files (*.xlsx)|*.xlsx"),
		EFileDialogFlags::None,
		OutFiles
	);

	if (OutFiles.Num() == 0)
	{
		return FReply::Handled();
	}

	// Import Excel file
	FDialogueXLSXImportResult ImportResult = FDialogueXLSXReader::ImportFromXLSX(OutFiles[0]);
	if (!ImportResult.bSuccess)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(LOCTEXT("ImportXLSXFailed", "Failed to read Excel file:\n{0}"),
				FText::FromString(ImportResult.ErrorMessage)));
		return FReply::Handled();
	}

	// v4.8.4: Hard abort if file was created by a newer version of the tool
	if (!ImportResult.FormatVersion.IsEmpty() && ImportResult.FormatVersion > FDialogueXLSXWriter::FORMAT_VERSION)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(LOCTEXT("NewerVersionAbortDialogueImport",
				"This file was exported by a newer version of the plugin (v{0}).\n"
				"Your version supports up to v{1}.\n\n"
				"Please update the plugin before importing this file."),
				FText::FromString(ImportResult.FormatVersion),
				FText::FromString(FDialogueXLSXWriter::FORMAT_VERSION)));
		return FReply::Handled();
	}

	FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_IMPORT, FPaths::GetPath(OutFiles[0]));

	// Get current UE rows
	TArray<FDialogueTableRow> UERows;
	if (TableData)
	{
		UERows = TableData->Rows;
	}

	// For now, use empty base (first sync). In future, base would come from LastSyncedHash comparison.
	// TODO v4.11: Use per-row LastSyncedHash for true 3-way merge
	TArray<FDialogueTableRow> BaseRows;

	// Perform 3-way comparison
	FDialogueSyncResult SyncResult = FDialogueXLSXSyncEngine::CompareSources(BaseRows, UERows, ImportResult.Rows);

	// v4.11: Only Unchanged auto-resolves - all other statuses require user approval
	FDialogueXLSXSyncEngine::AutoResolveNonConflicts(SyncResult);

	// Check if there are any changes
	if (!SyncResult.HasChanges())
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("ImportNoChanges", "No changes detected. Excel file matches current UE data."));
		return FReply::Handled();
	}

	// Show sync dialog for user approval (v4.11: all non-Unchanged changes require approval)
	if (SDialogueXLSXSyncDialog::ShowModal(SyncResult))
	{
		// User clicked Apply - merge changes
		FDialogueMergeResult MergeResult = FDialogueXLSXSyncEngine::ApplySync(SyncResult);

		// v4.8.4: Invalidate validation on all merged rows
		// v4.11: Update LastSyncedHash after sync apply
		for (FDialogueTableRow& Row : MergeResult.MergedRows)
		{
			Row.InvalidateValidation();
			Row.LastSyncedHash = Row.ComputeSyncHash();
		}

		// Update TableData
		if (TableData)
		{
			TableData->Rows = MergeResult.MergedRows;
			TableData->SourceFilePath = OutFiles[0];
			MarkDirty();
		}

		// Refresh view
		SyncFromTableData();
		RefreshList();

		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(LOCTEXT("ImportComplete", "Import complete!\n\nApplied from UE: {0}\nApplied from Excel: {1}\nDeleted: {2}\nUnchanged: {3}"),
				FText::AsNumber(MergeResult.AppliedFromUE),
				FText::AsNumber(MergeResult.AppliedFromExcel),
				FText::AsNumber(MergeResult.Deleted),
				FText::AsNumber(MergeResult.Unchanged)));
	}

	return FReply::Handled();
}

FReply SDialogueTableEditor::OnSyncXLSXClicked()
{
	// v4.8.4: Re-entrancy guard
	if (bIsBusy) return FReply::Handled();
	TGuardValue<bool> BusyGuard(bIsBusy, true);

	SyncToTableData();

	// Open file dialog
	TArray<FString> OutFiles;
	FDesktopPlatformModule::Get()->OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Sync Dialogue Table from Excel"),
		FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_IMPORT),
		TEXT(""),
		TEXT("Excel Files (*.xlsx)|*.xlsx"),
		EFileDialogFlags::None,
		OutFiles
	);

	if (OutFiles.Num() == 0)
	{
		return FReply::Handled();
	}

	// Import Excel file
	FDialogueXLSXImportResult ImportResult = FDialogueXLSXReader::ImportFromXLSX(OutFiles[0]);
	if (!ImportResult.bSuccess)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(LOCTEXT("SyncImportFailed", "Failed to read Excel file:\n{0}"),
				FText::FromString(ImportResult.ErrorMessage)));
		return FReply::Handled();
	}

	// v4.8.4: Hard abort if file was created by a newer version of the tool
	if (!ImportResult.FormatVersion.IsEmpty() && ImportResult.FormatVersion > FDialogueXLSXWriter::FORMAT_VERSION)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(LOCTEXT("NewerVersionAbortDialogueSync",
				"This file was exported by a newer version of the plugin (v{0}).\n"
				"Your version supports up to v{1}.\n\n"
				"Please update the plugin before syncing this file."),
				FText::FromString(ImportResult.FormatVersion),
				FText::FromString(FDialogueXLSXWriter::FORMAT_VERSION)));
		return FReply::Handled();
	}

	FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_IMPORT, FPaths::GetPath(OutFiles[0]));

	// Get current UE rows
	TArray<FDialogueTableRow> UERows;
	if (TableData)
	{
		UERows = TableData->Rows;
	}

	// For now, use empty base (first sync). In future, base would come from stored export snapshot.
	// TODO: Store base rows in TableData after export for true 3-way merge
	TArray<FDialogueTableRow> BaseRows;

	// Perform 3-way comparison
	FDialogueSyncResult SyncResult = FDialogueXLSXSyncEngine::CompareSources(BaseRows, UERows, ImportResult.Rows);

	// Auto-resolve non-conflicts
	FDialogueXLSXSyncEngine::AutoResolveNonConflicts(SyncResult);

	// Check if there are any changes
	if (!SyncResult.HasChanges())
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("SyncNoChanges", "No changes detected. Excel file matches current UE data."));
		return FReply::Handled();
	}

	// Show sync dialog
	if (SDialogueXLSXSyncDialog::ShowModal(SyncResult))
	{
		// User clicked Apply - merge changes
		FDialogueMergeResult MergeResult = FDialogueXLSXSyncEngine::ApplySync(SyncResult);

		// v4.8.4: Invalidate validation on all merged rows
		// v4.11: Update LastSyncedHash after sync apply
		// ApplySync copies UE rows via struct copy, preserving stale validation state
		for (FDialogueTableRow& Row : MergeResult.MergedRows)
		{
			Row.InvalidateValidation();
			Row.LastSyncedHash = Row.ComputeSyncHash();
		}

		// Update TableData
		if (TableData)
		{
			TableData->Rows = MergeResult.MergedRows;
			TableData->SourceFilePath = OutFiles[0];
			MarkDirty();
		}

		// Refresh view
		SyncFromTableData();
		RefreshList();

		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(LOCTEXT("SyncComplete", "Sync complete!\n\nApplied from UE: {0}\nApplied from Excel: {1}\nDeleted: {2}\nUnchanged: {3}"),
				FText::AsNumber(MergeResult.AppliedFromUE),
				FText::AsNumber(MergeResult.AppliedFromExcel),
				FText::AsNumber(MergeResult.Deleted),
				FText::AsNumber(MergeResult.Unchanged)));
	}

	return FReply::Handled();
}

FReply SDialogueTableEditor::OnSyncFromAssetsClicked()
{
	// v4.8.4: Re-entrancy guard
	if (bIsBusy) return FReply::Handled();
	TGuardValue<bool> BusyGuard(bIsBusy, true);

	// v4.7: Scan all DialogueBlueprint assets and create/update rows (matches NPC editor behavior)
	// Previously only updated existing rows - now creates rows from discovered assets

	if (!TableData)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoTableData", "No table data available. Please create or open a table first."));
		return FReply::Handled();
	}

	// =========================================================================
	// v4.7.1: Force Asset Registry rescan before querying
	// This handles file-copied assets that weren't duplicated via Unreal Editor
	// =========================================================================
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// =========================================================================
	// v4.7.1: Force Asset Registry rescan before querying
	// This handles file-copied assets that weren't duplicated via Unreal Editor
	// =========================================================================
	TArray<FString> PathsToScan = { TEXT("/Game/") };
	const FString PathFilter = TEXT("");  // Empty = no filtering, scan all /Game/ assets
	UE_LOG(LogTemp, Log, TEXT("[DialogueTableEditor] Rescanning Asset Registry for paths: %s"), *FString::Join(PathsToScan, TEXT(", ")));
	AssetRegistry.ScanPathsSynchronous(PathsToScan, true /* bForceRescan */);

	// =========================================================================
	// v4.8: First, get all dialogue assets from AssetRegistry to handle empty dialogues
	// =========================================================================
	TArray<FAssetData> AllDialogueAssets;
	FTopLevelAssetPath ClassPath(TEXT("/Script/NarrativeDialogueEditor"), TEXT("DialogueBlueprint"));
	AssetRegistry.GetAssetsByClass(ClassPath, AllDialogueAssets, true);

	// Filter to path
	TArray<FAssetData> FilteredAssets;
	for (const FAssetData& Asset : AllDialogueAssets)
	{
		if (PathFilter.IsEmpty() || Asset.PackagePath.ToString().Contains(PathFilter))
		{
			FilteredAssets.Add(Asset);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[DialogueTableEditor] Found %d DialogueBlueprint assets after filter"), FilteredAssets.Num());

	if (FilteredAssets.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoDialoguesFound", "No DialogueBlueprint assets found in the project.\n\nCreate DialogueBlueprint assets (DBP_*) first, then sync."));
		return FReply::Handled();
	}

	// Sync all dialogue assets using AssetRegistry (filtered by PathFilter)
	FDialogueAssetSyncResult SyncResult = FDialogueAssetSync::SyncFromAllAssets(PathFilter);

	UE_LOG(LogTemp, Log, TEXT("[DialogueTableEditor] Found %d dialogue nodes from assets"), SyncResult.NodesFound);

	// Track which dialogues have nodes (for empty dialogue detection)
	TSet<FString> DialoguesWithNodes;
	for (const auto& Pair : SyncResult.NodeData)
	{
		FString DialogueIDStr, NodeIDStr;
		if (Pair.Key.Split(TEXT("."), &DialogueIDStr, &NodeIDStr))
		{
			DialoguesWithNodes.Add(DialogueIDStr);
		}
	}

	// Build map of existing rows by DialogueID.NodeID key for quick lookup
	TMap<FString, TSharedPtr<FDialogueTableRowEx>> ExistingRowMap;
	for (auto& RowEx : AllRows)
	{
		if (RowEx->Data.IsValid())
		{
			FString Key = FDialogueAssetSyncResult::MakeKey(RowEx->Data->DialogueID, RowEx->Data->NodeID);
			ExistingRowMap.Add(Key, RowEx);
		}
	}

	int32 RowsCreated = 0;
	int32 RowsUpdated = 0;

	// Process each discovered node - create new rows or update existing
	for (const auto& Pair : SyncResult.NodeData)
	{
		const FString& Key = Pair.Key;
		const FDialogueNodeAssetData& NodeData = Pair.Value;

		// Parse DialogueID.NodeID from key
		FString DialogueIDStr, NodeIDStr;
		if (!Key.Split(TEXT("."), &DialogueIDStr, &NodeIDStr))
		{
			continue;
		}

		TSharedPtr<FDialogueTableRowEx>* ExistingRowPtr = ExistingRowMap.Find(Key);
		if (ExistingRowPtr && ExistingRowPtr->IsValid())
		{
			// Update existing row
			TSharedPtr<FDialogueTableRow> RowData = (*ExistingRowPtr)->Data;
			if (RowData.IsValid())
			{
				// v4.11.4: Debug logging for text sync
				UE_LOG(LogTemp, Log, TEXT("[SyncFromAssets] UPDATE existing row: %s"), *Key);
				UE_LOG(LogTemp, Log, TEXT("  Asset Text: '%s' (len=%d)"),
					NodeData.Text.Len() > 50 ? *(NodeData.Text.Left(50) + TEXT("...")) : *NodeData.Text,
					NodeData.Text.Len());
				UE_LOG(LogTemp, Log, TEXT("  Row Text: '%s' (len=%d, empty=%d)"),
					RowData->Text.Len() > 50 ? *(RowData->Text.Left(50) + TEXT("...")) : *RowData->Text,
					RowData->Text.Len(),
					RowData->Text.IsEmpty() ? 1 : 0);

				// Update tokens from asset (only if table is empty - don't overwrite user edits)
				bool bUpdated = false;
				if (RowData->EventsTokenStr.IsEmpty() && !NodeData.EventsTokenStr.IsEmpty())
				{
					RowData->EventsTokenStr = NodeData.EventsTokenStr;
					RowData->bEventsValid = true;
					bUpdated = true;
				}
				if (RowData->ConditionsTokenStr.IsEmpty() && !NodeData.ConditionsTokenStr.IsEmpty())
				{
					RowData->ConditionsTokenStr = NodeData.ConditionsTokenStr;
					RowData->bConditionsValid = true;
					bUpdated = true;
				}
				// Update NodeType from asset
				RowData->NodeType = NodeData.NodeType;

				// v4.8: Update full dialogue content from asset (only if table field is empty)
				if (RowData->Speaker.IsNone() && !NodeData.Speaker.IsEmpty())
				{
					RowData->Speaker = FName(*NodeData.Speaker);
					bUpdated = true;
				}
				if (RowData->Text.IsEmpty() && !NodeData.Text.IsEmpty())
				{
					UE_LOG(LogTemp, Log, TEXT("  -> Updating Text from asset"));
					RowData->Text = NodeData.Text;
					bUpdated = true;
				}
				if (RowData->OptionText.IsEmpty() && !NodeData.OptionText.IsEmpty())
				{
					RowData->OptionText = NodeData.OptionText;
					bUpdated = true;
				}
				if (RowData->ParentNodeID.IsNone() && !NodeData.ParentNodeID.IsEmpty())
				{
					RowData->ParentNodeID = FName(*NodeData.ParentNodeID);
					bUpdated = true;
				}
				if (RowData->NextNodeIDs.Num() == 0 && !NodeData.NextNodeIDs.IsEmpty())
				{
					TArray<FString> NextIDStrings;
					NodeData.NextNodeIDs.ParseIntoArray(NextIDStrings, TEXT(","));
					for (const FString& IDStr : NextIDStrings)
					{
						FString Trimmed = IDStr.TrimStartAndEnd();
						if (!Trimmed.IsEmpty())
						{
							RowData->NextNodeIDs.Add(FName(*Trimmed));
						}
					}
					bUpdated = true;
				}

				// v4.12: Always update graph position (transient - used for ordering)
				if (NodeData.bHasGraphPosition)
				{
					RowData->GraphPosX = NodeData.GraphPosX;
					RowData->GraphPosY = NodeData.GraphPosY;
					RowData->bHasGraphPosition = true;
				}

				// Mark as synced
				RowData->Status = EDialogueTableRowStatus::Synced;
				if (bUpdated)
				{
					RowsUpdated++;
				}
			}
		}
		else
		{
			// Create new row from asset data
			// v4.11.4: Debug logging for text sync
			UE_LOG(LogTemp, Log, TEXT("[SyncFromAssets] CREATE new row: %s"), *Key);
			UE_LOG(LogTemp, Log, TEXT("  Asset Text: '%s' (len=%d)"),
				NodeData.Text.Len() > 50 ? *(NodeData.Text.Left(50) + TEXT("...")) : *NodeData.Text,
				NodeData.Text.Len());

			TSharedPtr<FDialogueTableRow> NewRowData = MakeShared<FDialogueTableRow>();
			NewRowData->RowId = FGuid::NewGuid();
			NewRowData->DialogueID = FName(*DialogueIDStr);
			NewRowData->NodeID = FName(*NodeIDStr);
			NewRowData->NodeType = NodeData.NodeType;
			NewRowData->EventsTokenStr = NodeData.EventsTokenStr;
			NewRowData->ConditionsTokenStr = NodeData.ConditionsTokenStr;
			NewRowData->bEventsValid = true;
			NewRowData->bConditionsValid = true;
			NewRowData->Status = EDialogueTableRowStatus::Synced;

			// v4.8: Populate full dialogue content from asset sync
			NewRowData->Speaker = FName(*NodeData.Speaker);
			NewRowData->Text = NodeData.Text;
			NewRowData->OptionText = NodeData.OptionText;
			NewRowData->bSkippable = NodeData.bSkippable;
			NewRowData->ParentNodeID = FName(*NodeData.ParentNodeID);
			// Parse NextNodeIDs from comma-separated string to TArray<FName>
			if (!NodeData.NextNodeIDs.IsEmpty())
			{
				TArray<FString> NextIDStrings;
				NodeData.NextNodeIDs.ParseIntoArray(NextIDStrings, TEXT(","));
				for (const FString& IDStr : NextIDStrings)
				{
					FString Trimmed = IDStr.TrimStartAndEnd();
					if (!Trimmed.IsEmpty())
					{
						NewRowData->NextNodeIDs.Add(FName(*Trimmed));
					}
				}
			}

			// v4.12: Populate graph position for ordering
			if (NodeData.bHasGraphPosition)
			{
				NewRowData->GraphPosX = NodeData.GraphPosX;
				NewRowData->GraphPosY = NodeData.GraphPosY;
				NewRowData->bHasGraphPosition = true;
			}

			// Create wrapper and add to list
			TSharedPtr<FDialogueTableRowEx> NewRowEx = MakeShared<FDialogueTableRowEx>();
			NewRowEx->Data = NewRowData;
			NewRowEx->Sequence = 0;
			NewRowEx->Depth = 0;
			AllRows.Add(NewRowEx);

			RowsCreated++;
		}
	}

	// =========================================================================
	// v4.8: Create placeholder rows for empty dialogues (dialogues with no nodes)
	// =========================================================================
	int32 EmptyDialoguesFound = 0;
	for (const FAssetData& Asset : FilteredAssets)
	{
		FString DialogueName = Asset.AssetName.ToString();
		if (!DialoguesWithNodes.Contains(DialogueName))
		{
			// This dialogue exists but has no nodes - create placeholder row
			UE_LOG(LogTemp, Log, TEXT("[DialogueTableEditor] Found empty dialogue: %s"), *DialogueName);

			TSharedPtr<FDialogueTableRow> NewRowData = MakeShared<FDialogueTableRow>();
			NewRowData->RowId = FGuid::NewGuid();
			NewRowData->DialogueID = FName(*DialogueName);
			NewRowData->NodeID = FName(TEXT("_root"));  // Placeholder node ID
			NewRowData->NodeType = EDialogueTableNodeType::NPC;
			NewRowData->Text = TEXT("(Empty dialogue - add nodes in Unreal Editor)");
			NewRowData->Status = EDialogueTableRowStatus::Synced;

			TSharedPtr<FDialogueTableRowEx> NewRowEx = MakeShared<FDialogueTableRowEx>();
			NewRowEx->Data = NewRowData;
			NewRowEx->Sequence = 0;
			NewRowEx->Depth = 0;
			AllRows.Add(NewRowEx);

			EmptyDialoguesFound++;
			RowsCreated++;
		}
	}

	if (EmptyDialoguesFound > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DialogueTableEditor] Found %d empty dialogues (no nodes)"), EmptyDialoguesFound);
	}

	// Recalculate sequences and refresh
	CalculateSequences();
	SyncToTableData();
	UpdateColumnFilterOptions();
	ApplyFilters();
	RefreshList();
	UpdateStatusBar();

	if (RowsCreated > 0)
	{
		MarkDirty();
	}

	// Show result
	FMessageDialog::Open(EAppMsgType::Ok,
		FText::Format(LOCTEXT("SyncFromAssetsCompleteV48",
			"Synced {0} DialogueBlueprint assets from project.\n\n"
			"Dialogue nodes found: {1}\n"
			"Empty dialogues: {2}\n"
			"Rows created: {3}\n"
			"Rows updated: {4}\n"
			"Nodes with events: {5}\n"
			"Nodes with conditions: {6}\n\n"
			"You can now edit them in the table and regenerate."),
			FText::AsNumber(FilteredAssets.Num()),
			FText::AsNumber(SyncResult.NodesFound),
			FText::AsNumber(EmptyDialoguesFound),
			FText::AsNumber(RowsCreated),
			FText::AsNumber(RowsUpdated),
			FText::AsNumber(SyncResult.NodesWithEvents),
			FText::AsNumber(SyncResult.NodesWithConditions)));

	return FReply::Handled();
}

FReply SDialogueTableEditor::OnApplyToAssetsClicked()
{
	// v4.8.4: Re-entrancy guard
	if (bIsBusy) return FReply::Handled();
	TGuardValue<bool> BusyGuard(bIsBusy, true);

	// v4.4: Open preview window to review and apply token changes to UDialogueBlueprint assets

	// First sync to ensure we have latest data
	SyncToTableData();

	// Get all rows with tokens
	TArray<FDialogueTableRow> RowsWithTokens;
	if (TableData)
	{
		for (const FDialogueTableRow& Row : TableData->Rows)
		{
			if (!Row.EventsTokenStr.IsEmpty() || !Row.ConditionsTokenStr.IsEmpty())
			{
				RowsWithTokens.Add(Row);
			}
		}
	}

	if (RowsWithTokens.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoTokensToApply", "No token changes to apply.\n\nAdd events or conditions to dialogue nodes in the EVENTS or CONDITIONS columns, then use this button to apply them to the dialogue assets."));
		return FReply::Handled();
	}

	// Open preview window
	TWeakPtr<SDialogueTableEditor> WeakEditor = SharedThis(this);

	SDialogueTokenApplyPreview::OpenPreviewWindow(
		RowsWithTokens,
		TEXT("/Game/"),
		FOnTokenApplyConfirmed::CreateLambda([WeakEditor](const FTokenApplyPreviewResult& Result)
		{
			if (!Result.bConfirmed || Result.ApprovedChanges.Num() == 0)
			{
				return;
			}

			// Build rows from approved changes
			TArray<FDialogueTableRow> ApprovedRows;
			for (const FTokenChangeEntry& Entry : Result.ApprovedChanges)
			{
				if (Entry.Row.IsValid())
				{
					// Only include approved fields
					FDialogueTableRow ApplyRow = *Entry.Row;

					// Clear unapproved tokens
					if (!Entry.bEventsApproved)
					{
						ApplyRow.EventsTokenStr.Empty();
					}
					if (!Entry.bConditionsApproved)
					{
						ApplyRow.ConditionsTokenStr.Empty();
					}

					ApprovedRows.Add(ApplyRow);
				}
			}

			// Apply to assets
			FDialogueAssetApplySummary ApplySummary = FDialogueXLSXSyncEngine::ApplyTokensToAssets(
				ApprovedRows,
				TEXT("/Game/")
			);

			// Show result
			FMessageDialog::Open(EAppMsgType::Ok,
				FText::Format(LOCTEXT("ApplyComplete",
					"Apply complete!\n\n"
					"Assets processed: {0}\n"
					"Assets modified: {1}\n"
					"Nodes updated: {2}\n"
					"Events applied: {3}\n"
					"Conditions applied: {4}\n"
					"Nodes skipped (invalid): {5}"),
					FText::AsNumber(ApplySummary.AssetsProcessed),
					FText::AsNumber(ApplySummary.AssetsModified),
					FText::AsNumber(ApplySummary.TotalNodesUpdated),
					FText::AsNumber(ApplySummary.TotalEventsApplied),
					FText::AsNumber(ApplySummary.TotalConditionsApplied),
					FText::AsNumber(ApplySummary.TotalNodesSkipped)));
		})
	);

	return FReply::Handled();
}


//=============================================================================
// SDialogueTableEditorWindow
//=============================================================================

void SDialogueTableEditorWindow::Construct(const FArguments& InArgs)
{
	CurrentTableData = GetOrCreateTableData();
	BaseTabLabel = LOCTEXT("DialogueTableEditorTabLabel", "Dialogue Table Editor");

	ChildSlot
	[
		SNew(SVerticalBox)

		// Menu bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			BuildMenuBar()
		]

		// Main editor
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(TableEditor, SDialogueTableEditor)
				.TableData(CurrentTableData)
				.OnDirtyStateChanged(FOnDialogueTableDirtyStateChanged::CreateSP(this, &SDialogueTableEditorWindow::UpdateTabLabel))  // v4.6
		]
	];
}

TSharedRef<SWidget> SDialogueTableEditorWindow::BuildMenuBar()
{
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("New", "New"))
				.OnClicked_Lambda([this]() { OnNewTable(); return FReply::Handled(); })
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("Open", "Open"))
				.OnClicked_Lambda([this]() { OnOpenTable(); return FReply::Handled(); })
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f)
		[
			SNew(SButton)
				.Text(LOCTEXT("Save", "Save"))
				.OnClicked_Lambda([this]() { OnSaveTable(); return FReply::Handled(); })
		];
}

void SDialogueTableEditorWindow::OnNewTable()
{
	CurrentTableData = GetOrCreateTableData();
	CurrentTableData->Rows.Empty();
	CurrentTableData->SourceFilePath.Empty();

	if (TableEditor.IsValid())
	{
		TableEditor->SetTableData(CurrentTableData);
	}
}

void SDialogueTableEditorWindow::OnOpenTable()
{
}

void SDialogueTableEditorWindow::OnSaveTable()
{
	if (CurrentTableData)
	{
		CurrentTableData->MarkPackageDirty();

		UPackage* Package = CurrentTableData->GetOutermost();
		FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage::SavePackage(Package, CurrentTableData, *PackageFileName, SaveArgs);

		// v4.6: Update tab label to remove dirty indicator
		UpdateTabLabel();

		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Saved", "Dialogue table saved."));
	}
}

UDialogueTableData* SDialogueTableEditorWindow::GetOrCreateTableData()
{
	FString AssetPath = TEXT("/Game/FatherCompanion/DialogueTableData");
	UDialogueTableData* Data = LoadObject<UDialogueTableData>(nullptr, *AssetPath);

	if (!Data)
	{
		FString PackagePath = TEXT("/Game/FatherCompanion");
		FString AssetName = TEXT("DialogueTableData");

		UPackage* Package = CreatePackage(*FString::Printf(TEXT("%s/%s"), *PackagePath, *AssetName));
		Data = NewObject<UDialogueTableData>(Package, *AssetName, RF_Public | RF_Standalone);

		FAssetRegistryModule::AssetCreated(Data);

		// v4.8.3: Save immediately so asset exists on disk (prevents perpetual dirty state)
		FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage::SavePackage(Package, Data, *PackageFileName, SaveArgs);
	}

	return Data;
}

//=========================================================================
// v4.6: Dirty indicator and save-on-close prompt
//=========================================================================

void SDialogueTableEditorWindow::SetParentTab(TSharedPtr<SDockTab> InTab)
{
	ParentTab = InTab;
	BaseTabLabel = LOCTEXT("DialogueTableEditorTabLabel", "Dialogue Table Editor");

	if (InTab.IsValid())
	{
		// Set up tab close request handler (FCanCloseTab expects bool() signature)
		InTab->SetCanCloseTab(SDockTab::FCanCloseTab::CreateSP(this, &SDialogueTableEditorWindow::CanCloseTab));
		UpdateTabLabel();
	}
}

void SDialogueTableEditorWindow::UpdateTabLabel()
{
	TSharedPtr<SDockTab> Tab = ParentTab.Pin();
	if (!Tab.IsValid())
	{
		return;
	}

	if (IsTableDirty())
	{
		// Add asterisk to indicate dirty state
		Tab->SetLabel(FText::Format(LOCTEXT("DirtyTabLabel", "*{0}"), BaseTabLabel));
	}
	else
	{
		Tab->SetLabel(BaseTabLabel);
	}
}

bool SDialogueTableEditorWindow::IsTableDirty() const
{
	if (CurrentTableData)
	{
		UPackage* Package = CurrentTableData->GetPackage();
		return Package && Package->IsDirty();
	}
	return false;
}

bool SDialogueTableEditorWindow::CanCloseTab() const
{
	if (!IsTableDirty())
	{
		return true;  // Allow close
	}

	// Show save prompt
	EAppReturnType::Type Response = FMessageDialog::Open(
		EAppMsgType::YesNoCancel,
		LOCTEXT("SaveBeforeClose", "Dialogue Table has unsaved changes.\n\nDo you want to save before closing?")
	);

	switch (Response)
	{
		case EAppReturnType::Yes:
			// Call OnSaveTable via const_cast (FCanCloseTab requires const)
			const_cast<SDialogueTableEditorWindow*>(this)->OnSaveTable();
			return true;  // Close after save

		case EAppReturnType::No:
			return true;  // Close without saving

		case EAppReturnType::Cancel:
		default:
			return false;  // Don't close
	}
}

#undef LOCTEXT_NAMESPACE
