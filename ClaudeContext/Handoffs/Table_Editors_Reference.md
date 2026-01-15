# Table Editors Reference

**Consolidated:** 2026-01-15
**Updated:** 2026-01-15
**Status:** v4.3 XLSX Complete, v4.4 Validated Tokens Design Locked

This document consolidates the Dialogue Table Editor and NPC Table Editor handoffs, including the XLSX sync system and validated token design.

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
| **XLSX Export/Import** | Yes (v4.3) | Planned |
| **XLSX 3-Way Sync** | Yes (v4.3) | Planned |
| **Sync from Assets** | Planned (v4.4) | Yes (scan NPCDefinitions) |
| **Apply to Assets** | Planned (v4.4) | Yes (update existing) |
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

## XLSX Import/Export (v4.3) - COMPLETED

**Status:** Implemented and committed (v4.3, 2026-01-15)

### Design Goals

1. **Foolproof parsing** - Sentinel row + column ID mapping = immune to Excel messiness
2. **Safe sync** - 3-way merge with BASE_HASH = no silent overwrites
3. **Safe deletion** - Explicit #STATE column = no accidental data loss
4. **Round-trip** - Export = Import format = predictable workflow

### XLSX Workbook Structure

```
TableData.xlsx (ZIP container with STORE mode)
├── [Content_Types].xml
├── _rels/.rels
├── xl/
│   ├── workbook.xml
│   ├── worksheets/
│   │   ├── sheet1.xml       <- Main data
│   │   └── sheet2.xml       <- _Lists (dropdowns)
│   └── _rels/workbook.xml.rels
└── docProps/app.xml
```

### Sheet Layout

```
┌─────────────────────────────────────────────────────────────────┐
│  Row 1: #NPC_SHEET_V1 | #ROW_GUID | NPC_NAME | NPC_ID | ...     │  ← Sentinel + Column IDs
│  Row 2: (Display)     | Row ID    | Name     | ID     | ...     │  ← Human headers
│  Row 3+: (data)       | guid-1    | Seth     | seth_1 | ...     │  ← Data rows
└─────────────────────────────────────────────────────────────────┘
```

### Safety Features

| Feature | Implementation | Purpose |
|---------|---------------|---------|
| **Sentinel Row** | `#NPC_SHEET_V1`, `#DIALOGUE_SHEET_V1` | Detect sheet type/version |
| **Column ID Row** | Machine-readable IDs in Row 1 | Map columns regardless of order |
| **Row GUID** | `FGuid` per row | Track identity across edits |
| **#STATE** | New/Modified/Synced/Deleted | Explicit deletion, no accidental loss |
| **#BASE_HASH** | Hash at export time | 3-way merge baseline |

### 3-Way Merge Logic

```
if (XL_HASH == BASE_HASH && UE_HASH == BASE_HASH):
    # No changes - skip
elif (XL_HASH != BASE_HASH && UE_HASH == BASE_HASH):
    # Excel changed only - apply Excel → UE
elif (XL_HASH == BASE_HASH && UE_HASH != BASE_HASH):
    # UE changed only - update Excel hashes
elif (XL_HASH != BASE_HASH && UE_HASH != BASE_HASH):
    # CONFLICT - require manual resolution
```

### ID Format Alignment

| Component | Type | Notes |
|-----------|------|-------|
| **Node ID** | FName | Matches Narrative Pro UDialogueNode.ID |
| **NPC ID** | FName | Matches UNPCDefinition.NPCID |
| **Row GUID** | FGuid | Internal sync tracking only |

### Dependencies

```cpp
// GasAbilityGenerator.Build.cs
PrivateDependencyModuleNames.AddRange(new string[] {
    "FileUtilities",  // FZipArchiveReader/Writer
    "XmlParser"       // FXmlFile
});
```

| API | Availability |
|-----|--------------|
| `FZipArchiveWriter` | `WITH_ENGINE` (Editor builds) |
| `FZipArchiveReader` | `WITH_EDITOR` only |
| `FXmlFile` | Runtime |

### Implementation Phases (All Complete)

| Phase | Scope | Status |
|-------|-------|--------|
| 1 | Add FGuid RowId to FDialogueTableRow | ✅ Complete |
| 2 | DialogueXLSXWriter (export) | ✅ Complete |
| 3 | DialogueXLSXReader (import) | ✅ Complete |
| 4 | DialogueXLSXSyncEngine (3-way merge) + SDialogueXLSXSyncDialog | ✅ Complete |

### Implemented Files

```
Source/GasAbilityGenerator/
├── Public/XLSXSupport/
│   ├── DialogueXLSXWriter.h
│   ├── DialogueXLSXReader.h
│   └── DialogueXLSXSyncEngine.h
└── Private/XLSXSupport/
    ├── DialogueXLSXWriter.cpp
    ├── DialogueXLSXReader.cpp
    ├── DialogueXLSXSyncEngine.cpp
    └── SDialogueXLSXSyncDialog.cpp
```

### Toolbar Buttons Added

| Button | Function |
|--------|----------|
| Export XLSX | Creates workbook with dialogues + _Meta sheet |
| Import XLSX | Parses and replaces table data |
| Sync XLSX | 3-way merge with conflict resolution dialog |

---

## Validated Token System (v4.4) - DESIGN LOCKED

**Status:** Design locked, awaiting implementation approval

### Problem Statement

The v4.3 XLSX sync only handles text fields. Complex dialogue data (Events, Conditions, Goals) cannot be authored in Excel, limiting the "author outside UE" goal.

### Solution: Option 2 - Validated Tokens

Instead of making Events/Conditions read-only (too limiting) or fully relational sheets (too complex), use **validated token syntax** that enables real authoring with safety guarantees.

### Core Safety Principles

| Principle | Implementation |
|-----------|----------------|
| **Invalid token never wipes UE** | Parse failure → preserve UE value, flag error |
| **Partial apply** | If TEXT valid but EVENTS invalid, apply TEXT only |
| **Empty = unchanged** | Blank cell preserves UE value (not clear) |
| **Explicit clear** | `CLEAR` token required to remove all events |
| **Replace semantics** | Valid token list becomes complete node events |

### Column Structure (v4.4)

| Column | Type | Behavior |
|--------|------|----------|
| `EVENTS` | Editable tokens | Validated on import, applied if valid |
| `[RO]EVENTS_CURRENT` | Read-only | UE's current events (for reference/copy) |
| `CONDITIONS` | Editable tokens | Same as EVENTS |
| `[RO]CONDITIONS_CURRENT` | Read-only | UE's current conditions |
| TEXT, SPEAKER, etc. | Editable | Same as v4.3 |

### Token Grammar

**Strict function-style syntax:**
```
TOKEN_TYPE(ParamName=Value, ParamName2=Value2)
```

**Multiple tokens separated by semicolon:**
```
NE_BeginQuest(QuestId=Q_Intro); NE_SetFlag(Name=TalkedToSeth, Value=true)
```

**Special tokens:**
```
CLEAR                    # Explicitly clear all events/conditions
UNSUPPORTED(ClassName)   # Read-only fallback for unknown types
```

### Parameter Types

| Type | Example | Validation |
|------|---------|------------|
| `IdRef` | `QuestId=Q_Intro` | Must exist in _Lists sheet |
| `Bool` | `Value=true` | true/false only |
| `Int` | `Count=5` | Integer value |
| `Float` | `P=0.25` | Decimal value |
| `Enum` | `State=Active` | Must match enum values |
| `String` | `Name=TalkedToSeth` | Any text |

### v4.4 Starter Token Set (Option A - Minimal)

**Events (4 tokens):**

| Token | Parameters | Maps To |
|-------|------------|---------|
| `NE_BeginQuest` | `QuestId` (IdRef, required) | TBD: UNE_* class |
| `NE_CompleteQuestBranch` | `QuestId` (IdRef), `BranchId` (IdRef) | TBD: UNE_* class |
| `NE_GiveItem` | `ItemId` (IdRef), `Count` (Int, default=1) | TBD: UNE_* class |
| `NE_SetFlag` | `Name` (String), `Value` (Bool) | TBD: UNE_* class |

**Conditions (3 tokens):**

| Token | Parameters | Maps To |
|-------|------------|---------|
| `NC_QuestState` | `QuestId` (IdRef), `State` (Enum: Active/Complete/Failed) | TBD: UNC_* class |
| `NC_HasItem` | `ItemId` (IdRef), `MinCount` (Int, default=1) | TBD: UNC_* class |
| `NC_Flag` | `Name` (String), `Value` (Bool) | TBD: UNC_* class |

**TBD:** Actual Narrative Pro class names to be determined from screenshots/headers.

### Token Registry Architecture

```cpp
// Parameter definition
struct FDialogueTokenParam
{
    FName ParamName;           // "QuestId"
    ETokenParamType Type;      // IdRef, Bool, Int, Float, Enum, String
    bool bRequired;            // Must be present
    FName UEPropertyName;      // Maps to UObject property
    FString DefaultValue;      // Optional default
};

// Token specification
struct FDialogueTokenSpec
{
    FString TokenName;         // "NE_BeginQuest"
    ETokenCategory Category;   // Event or Condition
    UClass* UEClass;           // UNarrativeEvent subclass
    TArray<FDialogueTokenParam> Params;

    // Forward: Token string → UObject instance
    UObject* Deserialize(const FString& TokenStr, FString& OutError);

    // Reverse: UObject → Token string (for [RO] columns)
    FString Serialize(UObject* EventObj);
};

// Registry (singleton)
class FDialogueTokenRegistry
{
    TArray<FDialogueTokenSpec> Specs;

    FDialogueTokenSpec* FindByTokenName(const FString& Name);
    FDialogueTokenSpec* FindByClass(UClass* Class);

    // Export: UE events → token strings (with UNSUPPORTED fallback)
    FString SerializeEvents(const TArray<UObject*>& Events);

    // Import: token strings → UE events (with validation)
    bool DeserializeTokens(const FString& TokenStr,
                           TArray<UObject*>& OutEvents,
                           FString& OutError);

    // Validation: check all IdRefs exist
    bool ValidateIdRefs(const FString& TokenStr,
                        const TSet<FString>& ValidIds,
                        FString& OutError);
};
```

### Validation Flow

```
┌─────────────────┐
│ Parse Token     │ → Syntax error? → Preserve UE, flag row
└────────┬────────┘
         ↓
┌─────────────────┐
│ Find Spec       │ → Unknown token? → Preserve UE, flag row
└────────┬────────┘
         ↓
┌─────────────────┐
│ Validate Params │ → Missing required? Wrong type? → Preserve UE, flag row
└────────┬────────┘
         ↓
┌─────────────────┐
│ Validate IdRefs │ → ID not in _Lists? → Preserve UE, flag row
└────────┬────────┘
         ↓
┌─────────────────┐
│ Instantiate     │ → Create UObject, set properties
└────────┬────────┘
         ↓
┌─────────────────┐
│ Apply to Node   │ → Replace node's event/condition list
└─────────────────┘
```

### Partial Apply Behavior

| Scenario | TEXT | EVENTS | Result |
|----------|------|--------|--------|
| Both valid | "Hello" | `NE_BeginQuest(Q_Intro)` | Apply both |
| TEXT valid, EVENTS invalid | "Hello" | `NE_BginQuest(Q_Intro)` (typo) | Apply TEXT, preserve UE events, flag row |
| TEXT empty, EVENTS valid | "" | `NE_BeginQuest(Q_Intro)` | Clear TEXT, apply EVENTS |
| Both empty | "" | "" | Preserve both (empty = unchanged) |

### [RO] Column Generation

**For supported events:**
```
NE_BeginQuest(QuestId=Q_Intro)
NE_SetFlag(Name=TalkedToSeth, Value=true)
```

**For unsupported events (no spec match):**
```
UNSUPPORTED(UNE_PlayMontage)
UNSUPPORTED(UNE_CustomEvent)
```

This allows writers to see what's there without promising editability.

### _Lists Sheet Enhancement

Add valid ID lists for validation:

| Column | Content |
|--------|---------|
| `QUEST_IDS` | Q_Intro, Q_ForgeSupplies, ... |
| `ITEM_IDS` | EI_Sword, EI_Potion, ... |
| `GOAL_IDS` | Goal_Work, Goal_Sleep, ... |
| `QUEST_STATES` | Active, Complete, Failed |

Generated from asset scan at export time.

### Hash Calculation (Updated)

Hash includes ALL editable columns:
- TEXT, SPEAKER_ID, OPTION_TEXT, NOTES, SKIPPABLE (existing)
- EVENTS, CONDITIONS (new in v4.4)

[RO] columns NOT included in hash.

### Implementation Phases (v4.4)

| Phase | Scope | Dependencies |
|-------|-------|--------------|
| 1 | FDialogueTokenRegistry core | None |
| 2 | Token specs for starter set | Narrative Pro class mapping |
| 3 | Serializer (UE → tokens) | Registry |
| 4 | Deserializer (tokens → UE) | Registry |
| 5 | Update DialogueXLSXWriter | Serializer |
| 6 | Update DialogueXLSXReader | Deserializer |
| 7 | Update DialogueXLSXSyncEngine | Partial apply logic |
| 8 | Update _Lists sheet generation | Asset scan for IDs |

### Files to Create/Modify (v4.4)

**New:**
```
Source/GasAbilityGenerator/
├── Public/XLSXSupport/
│   └── DialogueTokenRegistry.h
└── Private/XLSXSupport/
    └── DialogueTokenRegistry.cpp
```

**Modify:**
```
DialogueXLSXWriter.cpp    - Add [RO] columns, use registry
DialogueXLSXReader.cpp    - Parse tokens, validate via registry
DialogueXLSXSyncEngine.cpp - Partial apply logic
```

### Alignment with v3.0 Regen/Diff System

The token system follows the same safety patterns as the v3.0 Regen/Diff system:

| v3.0 Pattern | v4.4 Token Equivalent |
|--------------|----------------------|
| Input hash (manifest) | BASE_HASH (at export) |
| Output hash (asset) | UE_HASH (current state) |
| CONFLICT detection | XL_HASH ≠ BASE_HASH AND UE_HASH ≠ BASE_HASH |
| SKIP if unchanged | XL_HASH == BASE_HASH AND UE_HASH == BASE_HASH |
| --force override | User selects "Use Excel" in conflict dialog |
| Per-asset metadata | Per-row GUID + hashes |

### Open Items

1. **Narrative Pro class mapping** - Need actual UNE_*/UNC_* class names from headers or screenshots
2. **Property name mapping** - Need actual FName property names for QuestId, ItemId, etc.
3. **ID type format** - Confirm whether IDs are FName, FString, or soft object paths

---

## Development Guide

This section provides patterns for implementing Slate table editors (`SListView`, `SMultiColumnTableRow`).

### Pattern Selection Summary

| UI Element | Pattern | Reason |
|------------|---------|--------|
| Row cell (display only) | `Text_Lambda` | Updates when data changes, only evaluated when visible |
| Row cell (editable) | `Text_Lambda` + `OnTextCommitted_Lambda` | Same as above + handles user edits |
| Status bar | `SAssignNew` + `SetText()` | Avoids per-frame polling, explicit updates only |
| Global counters | `SAssignNew` + `SetText()` | Same as status bar |

### Row Cell Text Updates - Use `Text_Lambda`

For table row cells that display data which can change (e.g., sequence numbers, computed values), **always use `Text_Lambda`** to ensure the display updates when underlying data changes:

```cpp
// CORRECT - Dynamic text that updates when RowData changes
TSharedRef<SWidget> CreateSeqCell()
{
    return SNew(SBox)
        .Padding(FMargin(4.0f, 2.0f))
        [
            SNew(STextBlock)
                .Text_Lambda([this]() { return FText::FromString(RowDataEx->SeqDisplay); })
        ];
}

// CORRECT - Editable cells with dynamic display
TSharedRef<SWidget> CreateNextNodesCell()
{
    return SNew(SEditableText)
        .Text_Lambda([RowData]()
        {
            FString Result;
            for (int32 i = 0; i < RowData->NextNodeIDs.Num(); i++)
            {
                if (i > 0) Result += TEXT(", ");
                Result += RowData->NextNodeIDs[i].ToString();
            }
            return FText::FromString(Result);
        })
        .OnTextCommitted_Lambda([this, RowData](const FText& NewText, ETextCommit::Type) { ... });
}

// WRONG - Static text set once at construction, never updates
TSharedRef<SWidget> CreateSeqCell()
{
    return SNew(STextBlock)
        .Text(FText::FromString(RowDataEx->SeqDisplay))  // BUG: Won't update!
}
```

**Why `Text_Lambda` is safe for row cells:**
- Only evaluated when the row widget is visible
- Just reads cached values (no expensive computation)
- No logging or side effects in the lambda

### Status Bar / Global UI - Use Explicit `SetText()`

For status bars and global UI elements that summarize data, **use stored widget references with explicit `SetText()` calls** to avoid performance issues:

```cpp
// In header:
TSharedPtr<STextBlock> StatusTotalText;
TSharedPtr<STextBlock> StatusSelectedText;

// In BuildStatusBar():
SAssignNew(StatusTotalText, STextBlock)
    .Text(LOCTEXT("InitTotal", "Total: 0 nodes"))

// Call UpdateStatusBar() explicitly when data changes:
void UpdateStatusBar()
{
    if (StatusTotalText.IsValid())
    {
        StatusTotalText->SetText(FText::Format(
            LOCTEXT("TotalNodes", "Total: {0} nodes"),
            FText::AsNumber(AllRows.Num())
        ));
    }
}

// WRONG - Lambda with logging = thousands of log calls per second
SNew(STextBlock)
    .Text_Lambda([this]()
    {
        UE_LOG(LogTemp, Warning, TEXT("Called every frame!"));  // PERFORMANCE BUG
        return FText::Format(...);
    })
```

**When to call `UpdateStatusBar()`:**
- After adding/deleting rows
- After selection changes (`OnSelectionChanged`)
- After applying filters
- After `RefreshList()`

### Column Definitions with Proportional Widths

Define columns with `FillWidth()` proportions that sum to ~1.0:

```cpp
// Column struct for reusability
struct FDialogueTableColumn
{
    FName ColumnId;
    FText DisplayName;
    float DefaultWidth;  // Proportion of total width (0.0 - 1.0)
};

// Define columns (proportions should sum to ~1.0)
inline TArray<FDialogueTableColumn> GetDialogueTableColumns()
{
    return {
        { TEXT("Seq"),          FText::FromString(TEXT("Seq")),          0.03f },  // Narrow
        { TEXT("DialogueID"),   FText::FromString(TEXT("Dialogue ID")),  0.09f },
        { TEXT("NodeID"),       FText::FromString(TEXT("Node ID")),      0.10f },
        { TEXT("NodeType"),     FText::FromString(TEXT("Type")),         0.05f },  // Narrow
        { TEXT("Speaker"),      FText::FromString(TEXT("Speaker")),      0.08f },
        { TEXT("Text"),         FText::FromString(TEXT("Text")),         0.20f },  // Wide - main content
        { TEXT("OptionText"),   FText::FromString(TEXT("Option Text")),  0.10f },
        { TEXT("ParentNodeID"), FText::FromString(TEXT("Parent")),       0.08f },
        { TEXT("NextNodeIDs"),  FText::FromString(TEXT("Next Nodes")),   0.10f },
        { TEXT("Skippable"),    FText::FromString(TEXT("Skip")),         0.04f },  // Narrow
        { TEXT("Notes"),        FText::FromString(TEXT("Notes")),        0.13f },
    };
}

// Build header row with proportional widths
TSharedRef<SHeaderRow> BuildHeaderRow()
{
    TSharedRef<SHeaderRow> Header = SNew(SHeaderRow);
    for (const FDialogueTableColumn& Col : GetDialogueTableColumns())
    {
        Header->AddColumn(
            SHeaderRow::Column(Col.ColumnId)
                .DefaultLabel(Col.DisplayName)
                .FillWidth(Col.DefaultWidth)  // Proportional width
                .SortMode(this, &SDialogueTableEditor::GetColumnSortMode, Col.ColumnId)
                .OnSort(this, &SDialogueTableEditor::OnColumnSortModeChanged)
                .HeaderContent()[ BuildColumnHeaderContent(Col) ]
        );
    }
    return Header;
}
```

### Column Header with Filter Text + Multi-Select Dropdown

Each column header contains: title, text filter, and multi-select dropdown:

```cpp
TSharedRef<SWidget> BuildColumnHeaderContent(const FDialogueTableColumn& Col)
{
    FColumnFilterState& FilterState = ColumnFilters.FindOrAdd(Col.ColumnId);

    return SNew(SVerticalBox)
        // Column title
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2.0f)
        [
            SNew(STextBlock)
                .Text(Col.DisplayName)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
        ]

        // Live text filter (filters as you type)
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

        // Multi-select dropdown with checkboxes
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2.0f, 1.0f)
        [
            SNew(SComboButton)
                .OnGetMenuContent_Lambda([this, ColumnId = Col.ColumnId]() -> TSharedRef<SWidget>
                {
                    // Build scrollable checkbox menu from unique column values
                    return BuildFilterDropdownMenu(ColumnId);
                })
                .ButtonContent()
                [
                    SNew(STextBlock)
                        .Text_Lambda([&FilterState]()
                        {
                            if (FilterState.SelectedValues.Num() == 0)
                                return LOCTEXT("AllFilter", "(All)");
                            if (FilterState.SelectedValues.Num() == 1)
                                return FText::FromString(*FilterState.SelectedValues.begin());
                            return FText::Format(LOCTEXT("MultiSel", "({0} selected)"),
                                FText::AsNumber(FilterState.SelectedValues.Num()));
                        })
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                ]
        ];
}
```

### Row Cell Styling Patterns

```cpp
// Standard text cell with padding and tooltip
TSharedRef<SWidget> CreateTextCell(FString& Value, const FString& Hint, bool bWithTooltip)
{
    TSharedRef<SWidget> EditableText = SNew(SEditableText)
        .Text_Lambda([&Value]() { return FText::FromString(Value); })
        .HintText(FText::FromString(Hint))
        .OnTextCommitted_Lambda([this, &Value](const FText& NewText, ETextCommit::Type)
        {
            Value = NewText.ToString();
            MarkModified();
        })
        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9));

    return SNew(SBox)
        .Padding(FMargin(4.0f, 2.0f))
        .ToolTipText_Lambda([&Value, bWithTooltip]()
        {
            return (bWithTooltip && !Value.IsEmpty()) ? FText::FromString(Value) : FText::GetEmpty();
        })
        [ EditableText ];
}

// Checkbox cell (centered)
TSharedRef<SWidget> CreateCheckboxCell(bool& bValue, const FText& Tooltip)
{
    return SNew(SBox)
        .Padding(FMargin(4.0f, 2.0f))
        .HAlign(HAlign_Center)
        [
            SNew(SCheckBox)
                .IsChecked_Lambda([&bValue]()
                {
                    return bValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                })
                .OnCheckStateChanged_Lambda([this, &bValue](ECheckBoxState NewState)
                {
                    bValue = (NewState == ECheckBoxState::Checked);
                    MarkModified();
                })
                .ToolTipText(Tooltip)
        ];
}

// Indented cell (for tree hierarchy display)
TSharedRef<SWidget> CreateIndentedCell(FName& Value, int32 Depth, const FString& Hint)
{
    float IndentSize = Depth * 8.0f;  // 8px per tree level

    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .AutoWidth()
        [ SNew(SSpacer).Size(FVector2D(IndentSize, 1.0f)) ]
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(FMargin(4.0f, 2.0f))
        [
            SNew(SEditableText)
                .Text_Lambda([&Value]() { return FText::FromName(Value); })
                .HintText(FText::FromString(Hint))
                .OnTextCommitted_Lambda([this, &Value](const FText& NewText, ETextCommit::Type)
                {
                    Value = FName(*NewText.ToString());
                    MarkModified();
                })
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
        ];
}
```

### Toolbar Button Styles

```cpp
TSharedRef<SWidget> BuildToolbar()
{
    return SNew(SHorizontalBox)
        // Primary action (green)
        + SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
        [
            SNew(SButton)
                .Text(LOCTEXT("AddRow", "+ Add Node"))
                .OnClicked(this, &SDialogueTableEditor::OnAddRowClicked)
                .ButtonStyle(FAppStyle::Get(), "FlatButton.Success")  // Green
        ]
        // Destructive action (red)
        + SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
        [
            SNew(SButton)
                .Text(LOCTEXT("Delete", "Delete"))
                .OnClicked(this, &SDialogueTableEditor::OnDeleteRowsClicked)
                .ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")   // Red
        ]
        // Important action (blue)
        + SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
        [
            SNew(SButton)
                .Text(LOCTEXT("Generate", "Generate"))
                .OnClicked(this, &SDialogueTableEditor::OnGenerateClicked)
                .ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")  // Blue
        ]
        // Neutral action (default style)
        + SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
        [
            SNew(SButton)
                .Text(LOCTEXT("Export", "Export CSV"))
                .OnClicked(this, &SDialogueTableEditor::OnExportCSVClicked)
                // No ButtonStyle = default gray
        ];
}
```

### Filter State Structure

```cpp
// Per-column filter state (supports text + multi-select)
struct FColumnFilterState
{
    FString TextFilter;                      // Text substring match
    TSet<FString> SelectedValues;            // Multi-select: empty = all, non-empty = OR logic
    TArray<TSharedPtr<FString>> DropdownOptions;  // Unique values for dropdown
};

// Store per-column filters
TMap<FName, FColumnFilterState> ColumnFilters;

// Apply filters to DisplayedRows
void ApplyFilters()
{
    DisplayedRows.Empty();
    for (TSharedPtr<FRowType>& Row : AllRows)
    {
        bool bPassesAllFilters = true;
        for (const auto& FilterPair : ColumnFilters)
        {
            const FColumnFilterState& Filter = FilterPair.Value;
            FString Value = GetColumnValue(*Row, FilterPair.Key);

            // Text filter (case-insensitive contains)
            if (!Filter.TextFilter.IsEmpty() &&
                !Value.ToLower().Contains(Filter.TextFilter.ToLower()))
            {
                bPassesAllFilters = false;
                break;
            }

            // Multi-select filter (OR logic)
            if (Filter.SelectedValues.Num() > 0 &&
                !Filter.SelectedValues.Contains(Value))
            {
                bPassesAllFilters = false;
                break;
            }
        }
        if (bPassesAllFilters) DisplayedRows.Add(Row);
    }
    RefreshList();
}
```

### UI/UX Standards Quick Reference

| Pattern | Dialogue Editor | NPC Editor | Guideline |
|---------|-----------------|------------|-----------|
| **Text Input** | `SEditableText` | `SEditableTextBox` | Use `SEditableText` for inline; `SEditableTextBox` for standalone |
| **Cell Padding** | `FMargin(4.0f, 2.0f)` | `FMargin(4.0f, 2.0f)` | **Standard: 4px horizontal, 2px vertical** |
| **Row Padding** | `FMargin(2.0f, 1.0f)` | `FMargin(2.0f, 1.0f)` | **Standard: 2px horizontal, 1px vertical** |
| **Confirmation** | Delete only | All edits | Use for destructive/irreversible operations |
| **Header Font** | Bold, 9pt | Bold, 9pt | **Standard: Bold 9** |
| **Cell Font** | Regular, 9pt | Regular, 9pt | **Standard: Regular 9** |
| **Filter Font** | Regular, 8pt | Regular, 8pt | **Standard: Regular 8** |
| **Tree Indent** | 8px per level | N/A | For hierarchical data only |

**Button Styles:**
- `"FlatButton.Success"` - Add operations (green)
- `"FlatButton.Danger"` - Delete operations (red)
- `"FlatButton.Primary"` - Primary action like Generate/Apply (blue)
- `"SimpleButton"` - Menu filter buttons (subtle)
- Default (no style) - Neutral operations (gray)

**When to use confirmation prompts:**
- ✅ Delete operations (both editors)
- ✅ All cell edits (NPC editor - modifies existing assets)
- ❌ Cell edits for new data (Dialogue editor - generates new assets)

**Cell Type Selection:**
| Data Type | Widget | Example |
|-----------|--------|---------|
| Single-line text | `SEditableText` / `SEditableTextBox` | Names, IDs |
| Checkbox | `SCheckBox` with `HAlign_Center` | Boolean flags |
| Integer range | Dual `SSpinBox<int32>` with "-" separator | Level range |
| Float 0-1 | `SSlider` with value display | Attack priority |
| Asset reference | `SComboButton` with asset picker menu | Blueprint selection |
| Multi-value | `SComboButton` with checkbox menu | Factions |

---

## Original Documents (Consolidated)

- v4.1_DialogueTableEditor_Handoff.md (deleted)
- v4.1_NPCTableEditor_Handoff.md (deleted)
