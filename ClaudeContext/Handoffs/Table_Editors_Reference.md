# Table Editors Reference

**Consolidated:** 2026-01-15
**Status:** Implemented in GasAbilityGenerator v4.2

This document consolidates the Dialogue Table Editor and NPC Table Editor handoffs.

---

## Feature Comparison

| Feature | Dialogue Table Editor | NPC Table Editor |
|---------|----------------------|------------------|
| **Columns** | 11 | 17 |
| **Primary Data** | Dialogue nodes | NPC definitions |
| **Tree Structure** | Yes (Parent/NextNodes) | No (flat list) |
| **Sequence Tracking** | Yes (Seq column) | No |
| **Cell Types** | Text, Checkbox | Text, Checkbox, AssetDropdown, MultiSelect, LevelRange, FloatSlider |
| **Add Row** | Smart (auto-populate from parent) | Basic |
| **Delete Row** | Re-parent children or cascade | Simple delete |
| **Duplicate Row** | No | Yes |
| **CSV Import/Export** | Yes (RFC 4180) | Yes |
| **Sync from Assets** | No | Yes (scan NPCDefinitions) |
| **Apply to Assets** | No (generates new) | Yes (update existing) |
| **POI Scanning** | No | Yes (from loaded world) |
| **Confirmation Prompts** | Delete only | All edits |
| **Status Bar** | Row counts | Row counts |
| **Multi-Select Filter** | Yes | Yes |
| **Text Filter** | Yes (live) | Yes (live) |
| **Save/Load Table** | Yes | Yes |
| **Implementation Size** | ~1200 lines | ~2600 lines |

---

## Overview

The plugin includes two Excel-like table editors for bulk content authoring:
1. **Dialogue Table Editor** (`SDialogueTableEditor`) - Batch dialogue creation
2. **NPC Table Editor** (`SNPCTableEditor`) - NPC management dashboard

Both editors share common UI patterns and integrate with the generation pipeline.

---

## Dialogue Table Editor

### Features (v4.2.8)
- 11 columns: Seq, DialogueID, NodeID, NodeType, Speaker, Text, OptionText, Parent, NextNodes, Skip, Notes
- FillWidth columns with proportional widths
- Live text filtering (OnTextChanged)
- Multi-select dropdown filters with checkbox UI
- (Empty) filter option for empty cells
- Clear All Filters button
- CSV Import/Export (RFC 4180 compliant)
- Smart Add Node with auto-populated fields
- Delete with re-parenting or cascade delete branch
- Skippable and Notes columns
- Preview tooltips for long text
- Status bar with live counts

### Files
- `SDialogueTableEditor.h` - Widget definitions
- `SDialogueTableEditor.cpp` - Full implementation (~1200 lines)
- `DialogueTableEditorTypes.h` - Row types, UDialogueTableData
- `DialogueTableConverter.h/cpp` - Row-to-manifest conversion
- `DialogueTableValidator.h/cpp` - Validation rules

### Column Mapping to Narrative Pro
| CSV Field | Narrative Pro Property |
|-----------|----------------------|
| DialogueID | UDialogueBlueprint asset name |
| NodeID | Node identifier in DialogueTemplate |
| NodeType=npc | UDialogueNode_NPC |
| NodeType=player | UDialogueNode_Player |
| Speaker | FSpeakerInfo.NPCDefinition |
| Text | FDialogueLine.Text |
| OptionText | Player choice label |
| Parent/NextNodes | Node linking (PlayerReplies/NPCReplies) |

---

## NPC Table Editor

### Features (v4.1)
- 17 columns covering identity, AI, combat, vendor, spawning, meta
- Sync from existing NPCDefinition assets
- POI scanning from loaded world
- Cell edit dropdowns with asset filtering
- Confirmation prompts for all edits
- CSV Export/Import
- Apply to Assets (write changes back to NPCDefinitions)
- Save/Open persistent table data

### Column Structure

**Core Identity (5):**
1. Status (auto-calculated)
2. NPC Name (asset name)
3. NPC ID
4. Display Name
5. Blueprint (NarrativeNPCCharacter filter)

**AI & Behavior (3):**
6. Ability Config
7. Activity Config
8. Schedule

**Combat (3):**
9. Level Range (min/max)
10. Factions
11. Attack Priority

**Vendor (2):**
12. Is Vendor
13. Shop Name

**Items & Spawning (2):**
14. Default Items
15. Spawner/POI

**Meta (2):**
16. Appearance
17. Notes

### Files
- `NPCTableEditorTypes.h` - FNPCTableRow, UNPCTableData
- `SNPCTableEditor.h` - Widget declarations
- `SNPCTableEditor.cpp` - Full implementation (~2600 lines)

---

## UI/UX Standards Comparison

### Cell Widget Patterns

| Pattern | Dialogue Table Editor | NPC Table Editor |
|---------|----------------------|------------------|
| **Text Input Widget** | `SEditableText` | `SEditableTextBox` |
| **Cell Padding** | `FMargin(4.0f, 2.0f)` | `FMargin(4.0f, 2.0f)` |
| **Dynamic Text** | `Text_Lambda([&Value]())` | `Text_Lambda([ValuePtr]())` |
| **Confirmation Prompt** | Delete operations only | All cell edits |
| **Checkbox Alignment** | `HAlign_Center` | `HAlign_Center` |
| **Tooltip Support** | Yes (for Text/Notes) | No |

### Cell Type Implementations

**Dialogue Table Editor (2 types):**
```cpp
// Text cell - SEditableText with lambda
SNew(SEditableText)
    .Text_Lambda([&Value]() { return FText::FromString(Value); })
    .OnTextCommitted_Lambda([this, &Value](const FText& NewText, ETextCommit::Type)
    {
        Value = NewText.ToString();
        MarkModified();
    })
    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))

// Checkbox cell - immediate update
SNew(SCheckBox)
    .IsChecked_Lambda([RowData]() { return RowData->bSkippable ? ... })
    .OnCheckStateChanged_Lambda([this, RowData](ECheckBoxState NewState) { ... })
```

**NPC Table Editor (6 types):**
```cpp
// Text cell - SEditableTextBox with confirmation prompt
SNew(SEditableTextBox)
    .Text_Lambda([ValuePtr]() { return FText::FromString(*ValuePtr); })
    .OnTextCommitted_Lambda([this, ValuePtr, Hint](const FText& NewText, ETextCommit::Type)
    {
        EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
            FText::Format(LOCTEXT("ConfirmEdit", "Change {0}?"), ...));
        if (Result == EAppReturnType::Yes) { *ValuePtr = NewText.ToString(); }
    })

// Level range - dual spinboxes
SNew(SHorizontalBox)
    + SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpinBox<int32>)...]
    + SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text("-")]
    + SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpinBox<int32>)...]

// Float slider - SSlider with value display
SNew(SHorizontalBox)
    + SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSlider)...]
    + SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock)...]

// Asset dropdown - SComboButton with asset picker
SNew(SComboButton)
    .OnGetMenuContent_Lambda([this, ...]() { return BuildAssetPicker(...); })

// Multi-select dropdown - SComboButton with checkbox list
SNew(SComboButton)
    .OnGetMenuContent_Lambda([this, ...]() { return BuildMultiSelectMenu(...); })

// Checkbox - with confirmation prompt
SNew(SCheckBox)
    .OnCheckStateChanged_Lambda([this, ValuePtr](ECheckBoxState NewState)
    {
        EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, ...);
        if (Result == EAppReturnType::Yes) { *ValuePtr = (NewState == Checked); }
    })
```

### Confirmation Prompt Behavior

| Editor | Scope | Prompt Text Pattern |
|--------|-------|---------------------|
| **Dialogue** | Delete row, Delete branch | "Are you sure you want to delete {N} node(s)?" |
| **NPC** | All cell edits | "Change {FieldName} from '{Old}' to '{New}'?" |
| **NPC** | Delete row | "Are you sure you want to delete {NPC Name}?" |

### Shared UI Patterns

#### Button Styles
```cpp
.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")   // Generate, Apply
.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")   // Add
.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")    // Delete
.ButtonStyle(FAppStyle::Get(), "SimpleButton")         // Menu filter buttons
```

#### Font Styles
```cpp
FCoreStyle::GetDefaultFontStyle("Bold", 9)     // Column headers
FCoreStyle::GetDefaultFontStyle("Regular", 9)  // Cell text
FCoreStyle::GetDefaultFontStyle("Regular", 8)  // Filter text, dropdown options
```

#### Row Padding
```cpp
// Both editors use same row padding
FSuperRowType::FArguments().Padding(FMargin(2.0f, 1.0f))
```

#### Multi-Select Filter Pattern
```cpp
struct FColumnFilterState
{
    FString TextFilter;                          // Live text filter
    TSet<FString> SelectedValues;                // Empty = all pass, OR logic
    TArray<TSharedPtr<FString>> DropdownOptions; // Unique values from column
};

// Filter dropdown with checkbox menu
SNew(SComboButton)
    .OnGetMenuContent_Lambda([this, ColumnId]() -> TSharedRef<SWidget>
    {
        TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox);

        // All/None buttons
        MenuContent->AddSlot()[
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()[SNew(SButton).Text("All").ButtonStyle("SimpleButton")]
            + SHorizontalBox::Slot()[SNew(SButton).Text("None").ButtonStyle("SimpleButton")]
        ];

        // Checkbox for each unique value
        for (const auto& Option : State->DropdownOptions)
        {
            MenuContent->AddSlot()[
                SNew(SCheckBox)
                    .IsChecked_Lambda([State, OptionValue]() { return State->SelectedValues.Contains(...); })
                    .OnCheckStateChanged_Lambda([this, State, OptionValue](...) { ApplyFilters(); })
                    [SNew(STextBlock).Text(FText::FromString(*Option))]
            ];
        }

        return SNew(SScrollBox)[MenuContent];
    })
    .ButtonContent()[
        SNew(STextBlock)
            .Text_Lambda([&FilterState]()
            {
                if (FilterState.SelectedValues.Num() == 0) return "(All)";
                if (FilterState.SelectedValues.Num() == 1) return FirstValue;
                return FString::Printf("(%d selected)", Num);
            })
    ]
```

#### Status Bar Patterns

**Dialogue Editor (v4.2.8+):** Direct SetText() for reliability
```cpp
// Store reference
SAssignNew(StatusTotalText, STextBlock)

// Update explicitly
void UpdateStatusBar()
{
    if (StatusTotalText.IsValid())
    {
        StatusTotalText->SetText(FText::Format(LOCTEXT(...), FText::AsNumber(Count)));
    }
}
```

**NPC Editor:** Text_Lambda with Invalidate()
```cpp
SNew(STextBlock)
    .Text_Lambda([this]() { return FText::Format(..., AllRows.Num()); })

// After data changes
StatusBar->Invalidate(EInvalidateWidgetReason::Paint);
```

### Selection and Navigation

| Feature | Dialogue | NPC |
|---------|----------|-----|
| **Selection Mode** | `ESelectionMode::Multi` | `ESelectionMode::Multi` |
| **Scroll Into View** | Yes (after add) | Yes (after add) |
| **Keyboard Shortcuts** | None | None |
| **Column Sorting** | Click header | Click header |
| **Default Sort** | Flow order (tree traversal) | Status, then Name |

### Tree Structure (Dialogue Only)

```cpp
// Calculate depth from parent chain
int32 CalculateNodeDepth(const FDialogueTableRow& Row, ...)
{
    if (Row.ParentNodeID.IsNone()) return 0;
    return 1 + CalculateNodeDepth(*ParentRow, ...);
}

// Visual indent in NodeID column
float IndentSize = RowEx->Depth * 8.0f;
SNew(SSpacer).Size(FVector2D(IndentSize, 1.0f))

// Sequence display (tree traversal order)
Node->SeqDisplay = FString::Printf(TEXT("%d"), Node->Sequence);
// Orphans marked with asterisk
Node->SeqDisplay = FString::Printf(TEXT("%d*"), Node->Sequence);
```

---

## Dependencies

### Narrative Pro Headers
```cpp
#include "AI/NPCDefinition.h"
#include "GAS/AbilityConfiguration.h"
#include "AI/Activities/NPCActivityConfiguration.h"
#include "Spawners/NPCSpawner.h"
#include "Navigation/POIActor.h"
```

---

## Original Documents (Consolidated)

- v4.1_DialogueTableEditor_Handoff.md (deleted)
- v4.1_NPCTableEditor_Handoff.md (deleted)
