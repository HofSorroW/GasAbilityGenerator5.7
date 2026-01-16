# Table Editors Reference

**Consolidated:** 2026-01-15
**Updated:** 2026-01-16
**Status:** v4.9.1 (GPT Safety Audit Complete)

This document consolidates the Dialogue Table Editor and NPC Table Editor handoffs, including the XLSX sync system, validated token design, and safety audit results.

---

## Version History

| Version | Date | Summary |
|---------|------|---------|
| **v4.9.1** | 2026-01-16 | P3 Determinism: Sort Dialogue sync by DialogueID+NodeID, user documentation |
| **v4.9** | 2026-01-16 | TriggerSet generator, button disable during busy state |
| **v4.8.4** | 2026-01-16 | P1/P2 Safety: Re-entrancy guards, save-fail abort, version check |
| **v4.7** | 2026-01-15 | Status column for Dialogue editor, asset discovery alignment |
| **v4.6** | 2026-01-15 | UX safety system, status bar refinements |
| **v4.5.4** | 2026-01-15 | XLSX-only format (CSV removed) |
| **v4.5.3** | 2026-01-15 | Validation coloring in Seq/Status columns |
| **v4.5.2** | 2026-01-15 | Duplicate row, GUID handling alignment |
| **v4.5.1** | 2026-01-15 | Validation cache system |
| **v4.4** | 2026-01-15 | Validated Token System, Apply to Assets, Sync from Assets |
| **v4.3** | 2026-01-15 | XLSX 3-way sync system |
| **v4.2** | 2026-01-14 | Status bar, filtering, multi-select dropdowns |
| **v4.1** | 2026-01-14 | Initial Dialogue and NPC Table Editors |

---

## Feature Comparison

| Feature | Dialogue Table Editor | NPC Table Editor |
|---------|----------------------|------------------|
| **Columns** | 14 (v4.7 added Status) | 17 |
| **Primary Data** | Dialogue nodes | NPC definitions |
| **Tree Structure** | Yes (Parent/NextNodes) | No (flat list) |
| **Sequence Tracking** | Yes (Seq column) | No |
| **Cell Types** | Text, Checkbox, Token | Text, Checkbox, AssetDropdown, MultiSelect, LevelRange, FloatSlider |
| **Add Row** | Smart (auto-populate from parent) | Basic |
| **Delete Row** | Re-parent children or cascade | Simple delete |
| **Duplicate Row** | Yes (v4.5.2) | Yes |
| **XLSX Export/Import** | Yes (v4.3) | Yes (v4.4) |
| **XLSX 3-Way Sync** | Yes (v4.3) | Yes (v4.4) |
| **Sync from Assets** | Yes (v4.4) | Yes (scan NPCDefinitions) |
| **Apply to Assets** | Yes (v4.4) | Yes (update existing) |
| **Token Preview Window** | Yes (v4.4) | No |
| **POI Scanning** | No | Yes (from loaded world) |
| **Confirmation Prompts** | Delete only | All edits |
| **Status Bar** | Row counts + validation | Row counts + validation |
| **Validation Caching** | Yes (v4.5.1) | Yes (v4.5.1) |
| **Staleness Detection** | Yes (hash-based) | Yes (hash-based) |
| **Validation Coloring** | Yes (Seq column bar, v4.5.3) | Yes (Status column) |
| **Multi-Select Filter** | Yes | Yes |
| **Text Filter** | Yes (live) | Yes (live) |
| **Save/Load Table** | Yes | Yes |
| **Re-entrancy Guard** | Yes (v4.8.4) | Yes (v4.8.4) |
| **Button Disable on Busy** | Yes (v4.9) | Yes (v4.9) |
| **Save-Fail Abort** | Yes (v4.8.4) | Yes (v4.8.4) |
| **Version Compatibility Check** | Yes (v4.8.4) | Yes (v4.8.4) |
| **Implementation Size** | ~1400 lines | ~2800 lines |

---

## Safety System (v4.8.4/v4.9/v4.9.1)

The table editors implement a comprehensive safety system based on a GPT-assisted audit. The audit identified potential issues across three priority levels.

### GPT Audit Summary

| Priority | Focus | Status |
|----------|-------|--------|
| **P1** | Truthfulness & Re-entrancy | ✅ Complete (v4.8.4) |
| **P2** | Contract Correctness | ✅ Complete (v4.8.4/v4.9) |
| **P3** | Determinism & User Expectations | ✅ Complete (v4.9.1) |

### P1 — Truthfulness & Re-entrancy

Ensures UI always reflects true data state and prevents concurrent operation corruption.

| Protection | Description | Implementation |
|------------|-------------|----------------|
| **Validation Invalidation on Edit** | Cell edits clear cached validation state | `MarkModified()` calls `InvalidateValidation()` |
| **Validation Invalidation on Import/Sync** | Merged rows have stale validation cleared | Loop after `ApplySync()` calls `InvalidateValidation()` on all rows |
| **Re-entrancy Guard** | Prevents double-click corruption | `bIsBusy` flag with `TGuardValue` pattern |
| **Button Disable on Busy** | Visual feedback during operations | `.IsEnabled_Lambda([this]() { return !bIsBusy; })` |
| **File-Locked Messaging** | Clear error when Excel has file open | "If the file is open in Excel, please close it and try again" |

**Re-entrancy Guard Pattern:**
```cpp
// Member variable
bool bIsBusy = false;

// In operation handler
FReply OnGenerateClicked()
{
    if (bIsBusy) return FReply::Handled();  // Early exit if busy
    TGuardValue<bool> BusyGuard(bIsBusy, true);  // Auto-reset on scope exit

    // ... perform operation ...
    return FReply::Handled();
}
```

### P2 — Contract Correctness

Ensures operations complete fully or abort cleanly with user notification.

| Protection | Description | Implementation |
|------------|-------------|----------------|
| **Save-Fail Abort** | Generate aborts if pre-save fails | Checks `SavePackage()` return, shows error, returns early |
| **FORMAT_VERSION Check** | Hard abort on newer file version | Compares `FormatVersion > FORMAT_VERSION`, shows "Please update plugin" |

**Save-Fail Pattern:**
```cpp
bool bSaveSuccess = UPackage::SavePackage(Package, Asset, *PackageFilename, SaveArgs);
if (!bSaveSuccess)
{
    FMessageDialog::Open(EAppMsgType::Ok,
        LOCTEXT("SaveFailed", "Failed to save asset. Check if file is locked."));
    return;  // Abort generation
}
```

**Version Check Pattern:**
```cpp
if (!ImportResult.FormatVersion.IsEmpty() && ImportResult.FormatVersion > FORMAT_VERSION)
{
    FMessageDialog::Open(EAppMsgType::Ok,
        FText::Format(LOCTEXT("NewerVersionAbort",
            "This file was exported by a newer version (v{0}).\n"
            "Your version supports up to v{1}.\n"
            "Please update the plugin."),
            FText::FromString(ImportResult.FormatVersion),
            FText::FromString(FORMAT_VERSION)));
    return FReply::Handled();  // Hard abort
}
```

### P3 — Determinism & User Expectations (v4.9.1)

Ensures consistent behavior and documents non-obvious system behaviors.

| Item | Issue | Resolution |
|------|-------|------------|
| **Dialogue Sync Ordering** | TSet iteration produced non-deterministic preview order | Added sort by DialogueID+NodeID in `DialogueXLSXSyncEngine.cpp:309-330` |
| **ID Rename Behavior** | Users may expect "rename = new row" | Documented GUID-based identity (see User Notes section) |
| **Merged Cells** | Silent data loss on import | Documented as unsupported (see User Notes section) |

**Determinism Audit Results:**

| Area | Uses TSet/TMap | Deterministic? | Notes |
|------|----------------|----------------|-------|
| Data Row Export | No (TArray) | ✅ Yes | Rows exported in array order |
| FString Columns | No (FString) | ✅ Yes | Multi-values preserve authored order |
| NPC Sync Engine | Yes | ✅ Yes | Sorts by display name (line 196) |
| Dialogue Sync Engine | Yes | ✅ Yes (v4.9.1) | Now sorts by DialogueID+NodeID |
| _Lists Sheet | Yes | ⚠️ No | Reference data only, order irrelevant |

**Sort Implementation (DialogueXLSXSyncEngine.cpp):**
```cpp
// v4.9.1: Sort entries by DialogueID + NodeID for consistent display order
Result.Entries.Sort([](const FDialogueSyncEntry& A, const FDialogueSyncEntry& B)
{
    const FDialogueTableRow* RowA = A.UERow.Get() ? A.UERow.Get() :
        (A.ExcelRow.Get() ? A.ExcelRow.Get() : A.BaseRow.Get());
    const FDialogueTableRow* RowB = B.UERow.Get() ? B.UERow.Get() :
        (B.ExcelRow.Get() ? B.ExcelRow.Get() : B.BaseRow.Get());

    if (!RowA || !RowB) return A.RowId < B.RowId;  // Fallback to GUID

    if (RowA->DialogueID != RowB->DialogueID)
        return RowA->DialogueID.LexicalLess(RowB->DialogueID);
    return RowA->NodeID.LexicalLess(RowB->NodeID);
});
```

### Protected Operations

Both editors apply `bIsBusy` guard and button disable to:
- Validate
- Generate
- Export XLSX
- Import XLSX
- Sync XLSX (Dialogue only)
- Sync from Assets
- Apply to Assets

### Items Explicitly Not Implemented

| Item | Reason |
|------|--------|
| **P3.3: ID change reporting in Generate** | Sync preview already shows all changes before apply; adding post-generate reporting would duplicate information |
| **_Lists sheet sorting** | Dropdown order has no semantic meaning; sorting adds code for zero user benefit |
| **Merged cell detection** | Documentation warning is sufficient; detection code is non-trivial for a problem that rarely occurs in data tables |

---

## Important User Notes (v4.9.1)

### ID Rename Behavior

Row identity in both editors is based on `#ROW_GUID`, a hidden immutable GUID column. This means:

| Action | Result |
|--------|--------|
| **Rename DIALOGUE_ID** | Row is tracked by GUID → ID change updates existing asset |
| **Rename NODE_ID** | Row is tracked by GUID → node is renamed, not duplicated |
| **Rename NPC name** | Row is tracked by GUID → NPCDefinition is updated |

**Why this matters:** Users familiar with CSV workflows may expect "rename = new row." Our GUID-based identity system ensures renaming is a non-destructive update operation, not a create-new operation. You can safely rename IDs without creating duplicate assets.

### Merged Cells Not Supported

Excel merged cells are **not supported** in XLSX import. If you merge cells in Excel:

- Only the top-left cell of a merged range contains the value
- All other cells in the merge range read as empty
- This can cause silent data loss (empty Speaker, empty Text, etc.)

**Best practice:** Never merge cells in the data area of your XLSX file. Use merging only in separate header/documentation sheets that aren't imported.

---

## Overview

The plugin includes two Excel-like table editors for bulk content authoring:
1. **Dialogue Table Editor** (`SDialogueTableEditor`) - Batch dialogue creation
2. **NPC Table Editor** (`SNPCTableEditor`) - NPC management dashboard

Both editors share common UI patterns and integrate with the generation pipeline.

---

## Dialogue Table Editor

### Features (v4.5.4)
- 11 columns: Seq, DialogueID, NodeID, NodeType, Speaker, Text, OptionText, Parent, NextNodes, Skip, Notes
- FillWidth columns with proportional widths
- Live text filtering (OnTextChanged)
- Multi-select dropdown filters with checkbox UI
- (Empty) filter option for empty cells
- Clear All Filters button
- XLSX Import/Export with 3-way sync support
- Smart Add Node with auto-populated fields
- Delete with re-parenting or cascade delete branch
- Skippable and Notes columns
- Preview tooltips for long text
- Status bar with live counts
- Validation coloring in Seq column

### Files
- `SDialogueTableEditor.h` - Widget definitions
- `SDialogueTableEditor.cpp` - Full implementation (~1200 lines)
- `DialogueTableEditorTypes.h` - Row types, UDialogueTableData
- `DialogueTableConverter.h/cpp` - Row-to-manifest conversion
- `DialogueTableValidator.h/cpp` - Validation rules

### Column Mapping to Narrative Pro
| XLSX Field | Narrative Pro Property |
|-----------|----------------------|
| DIALOGUE_ID | UDialogueBlueprint asset name |
| NODE_ID | Node identifier in DialogueTemplate |
| NODE_TYPE=npc | UDialogueNode_NPC |
| NODE_TYPE=player | UDialogueNode_Player |
| SPEAKER | FSpeakerInfo.NPCDefinition |
| TEXT | FDialogueLine.Text |
| OPTION_TEXT | Player choice label |
| EVENTS | Node Events (token format) |
| CONDITIONS | Node Conditions (token format) |
| PARENT_NODE_ID | Parent node link |
| NEXT_NODE_IDS | Node linking (PlayerReplies/NPCReplies) |

---

## NPC Table Editor

### Features (v4.5.4)
- 17 columns covering identity, AI, combat, vendor, spawning, meta
- Sync from existing NPCDefinition assets
- POI scanning from loaded world
- Cell edit dropdowns with asset filtering
- Confirmation prompts for all edits
- XLSX Export/Import with 3-way sync support
- Apply to Assets (write changes back to NPCDefinitions)
- Save/Open persistent table data
- Validation coloring in Status column

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

| Button | Version | Function |
|--------|---------|----------|
| Export XLSX | v4.3 | Creates workbook with dialogues + _Meta sheet |
| Import XLSX | v4.3 | Parses and replaces table data |
| Sync XLSX | v4.3 | 3-way merge with conflict resolution dialog |
| Sync from Assets | v4.4 | Pull current events/conditions from UDialogueBlueprint into table |
| Apply to Assets | v4.4 | Opens preview window, applies approved tokens to UDialogueBlueprint |

### Status Bar (v4.4)

The status bar displays live counts and validation feedback:

```
Total: 45 nodes | Dialogues: 3 | Showing: 42 | Selected: 2 | Token Errors: 3
```

| Element | Description |
|---------|-------------|
| Total | All nodes in AllRows |
| Dialogues | Unique DialogueID count |
| Showing | Nodes after filtering (DisplayedRows) |
| Selected | Currently selected rows |
| Token Errors | Rows with invalid EventsTokenStr or ConditionsTokenStr (red, hidden if 0) |

Validation errors are set during XLSX import when tokens fail parsing.

---

## Validated Token System (v4.4) - IMPLEMENTED

**Status:** Fully implemented (2026-01-15)

### Problem Statement

The v4.3 XLSX sync only handles text fields. Complex dialogue data (Events, Conditions, Goals) cannot be authored in Excel, limiting the "author outside UE" goal.

### Solution: Validated Tokens with Preview Window

Use **validated token syntax** that enables real authoring with safety guarantees, combined with a **full-screen preview window** for reviewing changes before applying to UDialogueBlueprint assets.

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

### Implementation Phases (v4.4) - ALL COMPLETE

| Phase | Scope | Status |
|-------|-------|--------|
| 1 | FDialogueTokenRegistry core | ✅ Complete |
| 2 | Token specs for starter set | ✅ Complete |
| 3 | Serializer (UE → tokens) | ✅ Complete |
| 4 | Deserializer (tokens → UE) with Apply to Asset | ✅ Complete |
| 5 | Update DialogueXLSXWriter | ✅ Complete |
| 6 | Update DialogueXLSXReader | ✅ Complete |
| 7 | Token Apply Preview Window | ✅ Complete |
| 8 | Apply to Assets button integration | ✅ Complete |

### Implemented Files (v4.4)

**New Files:**
```
Source/GasAbilityGenerator/
├── Public/XLSXSupport/
│   ├── DialogueTokenRegistry.h      - Token specs and validation
│   ├── DialogueAssetSync.h          - Asset read/write operations
│   └── SDialogueTokenApplyPreview.h - Preview window widget
└── Private/XLSXSupport/
    ├── DialogueTokenRegistry.cpp    - Token parsing and serialization
    ├── DialogueAssetSync.cpp        - Apply tokens to UDialogueBlueprint
    └── SDialogueTokenApplyPreview.cpp - Full-screen preview UI (~700 lines)
```

**Modified Files:**
```
DialogueXLSXWriter.cpp     - Exports events/conditions as tokens
DialogueXLSXReader.cpp     - Parses token columns from XLSX
DialogueXLSXSyncEngine.cpp - ApplyTokensToAssets() integration
SDialogueTableEditor.cpp   - "Apply to Assets" toolbar button
SDialogueTableEditor.h     - OnApplyToAssetsClicked() handler
```

### Token Apply Preview Window

The preview window provides full visibility into token changes before applying to UDialogueBlueprint assets.

**Layout:**
```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Token Apply Preview                                  │
├───────────────────────┬─────────────────────────────────────────────────────┤
│ Context Panel         │  Change Cards (Scrollable)                          │
│                       │                                                      │
│ Quest: Q_Intro        │  ┌─────────────────────────────────────────────────┐│
│ Dialogue: DBP_Seth    │  │ Node: greeting_01  Speaker: Seth                ││
│ Speakers:             │  │ "Hello, traveler!"                              ││
│  • Seth               │  ├──────────────────────┬──────────────────────────┤│
│  • Player             │  │ CURRENT              │ NEW                      ││
│                       │  │ (empty)              │ NE_BeginQuest(Q_Intro)   ││
│ ─────────────────     │  │ [✓] Approve Events   │                          ││
│ Summary:              │  ├──────────────────────┴──────────────────────────┤│
│  Total Changes: 5     │  │ Notes: ________________________________         ││
│  Events: 3            │  └─────────────────────────────────────────────────┘│
│  Conditions: 2        │                                                      │
│                       │  ┌─────────────────────────────────────────────────┐│
│  Approved: 5          │  │ Node: choice_01  Speaker: Player                ││
│  Denied: 0            │  │ "I'll help you."                                ││
│                       │  ├──────────────────────┬──────────────────────────┤│
│                       │  │ CURRENT              │ NEW                      ││
│                       │  │ NC_HasItem(Sword)    │ NC_QuestState(Q_Intro,   ││
│                       │  │                      │   State=Active)          ││
│                       │  │ [✓] Approve Conds    │                          ││
│                       │  └─────────────────────────────────────────────────┘│
├───────────────────────┴─────────────────────────────────────────────────────┤
│                    [ Cancel ]                    [ Apply Approved ]          │
└─────────────────────────────────────────────────────────────────────────────┘
```

**Key Features:**
- Left panel shows Quest/Dialogue/Speaker context (always visible)
- Live counters update as user approves/denies changes
- Side-by-side Current vs New comparison for each change
- Per-change approve/deny checkboxes with notes field
- Only approved changes are applied when clicking "Apply Approved"
- Validation errors shown inline (red highlight + error message)

**Data Structures:**

```cpp
// Single token change entry
struct FTokenChangeEntry
{
    TSharedPtr<FDialogueTableRow> Row;
    FName NodeID;
    FString Speaker;
    FString DialogueText;

    // Current state (from UE asset)
    FString CurrentEvents;
    FString CurrentConditions;

    // New state (from Excel import)
    FString NewEvents;
    FString NewConditions;

    // Change tracking
    bool bEventsChanged;
    bool bConditionsChanged;

    // User approval
    bool bEventsApproved = true;
    bool bConditionsApproved = true;
    FString ApprovalNotes;

    // Validation
    bool bEventsValid = true;
    bool bConditionsValid = true;
    FString EventsValidationError;
    FString ConditionsValidationError;
};

// Context for left panel
struct FTokenApplyContext
{
    FString QuestName;
    FName DialogueID;
    FString DialogueName;
    TArray<FString> Speakers;

    int32 TotalChanges;
    int32 EventChanges;
    int32 ConditionChanges;
    int32 ApprovedCount;
    int32 DeniedCount;
};
```

**Bidirectional Workflow:**

```
┌─────────────────────┐     ┌──────────────────────┐     ┌─────────────────────┐
│  UDialogueBlueprint │ ←── │  Dialogue Table      │ ──→ │  UDialogueBlueprint │
│  (UE Assets)        │     │  Editor              │     │  (UE Assets)        │
└─────────────────────┘     └──────────────────────┘     └─────────────────────┘
         ↑                           │                            ↑
    Sync from Assets            Edit tokens                 Apply to Assets
    (read current)              in table                    (write changes)
```

**Sync from Assets → Edit → Apply to Assets:**

1. **Sync from Assets** - Pull current events/conditions from UDialogueBlueprint
   - Searches `/Game/Dialogues/` and `/Game/` paths
   - Only fills empty token cells (preserves existing edits)
   - Shows summary: dialogues found/missing, nodes with events/conditions

2. **Edit in Table** - Modify EventsTokenStr and ConditionsTokenStr columns
   - Status bar shows "Token Errors: N" for invalid syntax
   - Validation runs during XLSX import

3. **Apply to Assets** - Write validated tokens to UDialogueBlueprint
   - Preview window shows all changes with context
   - User approves/denies each change individually
   - Only approved changes are written to assets

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

### Resolved Items

1. ✅ **Narrative Pro class mapping** - UNarrativeEvent and UNarrativeCondition base classes identified
2. ✅ **Property name mapping** - Uses reflection to set properties on event/condition objects
3. ✅ **ID type format** - Uses FString for token parameters, converted to appropriate types during deserialization

### Known Limitations

1. **UNSUPPORTED tokens** - Custom/unknown event/condition types shown as `UNSUPPORTED(ClassName)` - cannot be edited in Excel
2. **Manual validation** - Users should validate in-editor after applying tokens to confirm events fire correctly
3. **No undo** - Applied changes cannot be undone; recommend saving assets before bulk apply

### Future Enhancements (Dialogue Table Editor)

**Token System Expansion:**
| Item | Description |
|------|-------------|
| More Event tokens | NE_GiveXP, NE_PlaySound, NE_SpawnActor, NE_TeleportPlayer, etc. |
| More Condition tokens | NC_HasCompletedQuest, NC_HasTag, NC_LevelCheck, NC_TimeOfDay, etc. |
| _Lists sheet validation | Populate Quest IDs, Item IDs, NPC IDs from asset scan for autocomplete/validation |

**Workflow Improvements:**
| Item | Description |
|------|-------------|
| Bulk token copy | Copy events/conditions from one node to multiple selected nodes |
| Token autocomplete | Dropdown suggestions while typing tokens in cells |
| Validation on cell edit | Immediate feedback when typing invalid token syntax |
| Token templates | Pre-defined token combinations for common patterns (e.g., "Start Quest + Give Item") |

**Quality of Life:**
| Item | Description |
|------|-------------|
| Keyboard shortcuts | Ctrl+D duplicate, Ctrl+Shift+V paste tokens only |
| Undo/Redo | Track changes for reversal |
| Diff view | Show what changed since last save/export |

---

## Validation Cache System (v4.5.1) - IMPLEMENTED

**Status:** Fully implemented (2026-01-15)

### Problem Statement

The NPC and Dialogue table editors had inconsistent validation approaches:
- Dialogue stored validation state directly on row struct (`bEventsValid`, `bConditionsValid`)
- NPC computed validation on-demand via external validator class

This caused:
1. Inconsistent APIs between editors
2. Status bar re-validating on every update (O(n) cost)
3. No staleness detection when underlying data changed

### Solution: Unified Validation Cache

Both editors now use **Option A with discipline**: cache validation results on the row struct, but the validator remains the single source of truth.

### Shared Components

**EValidationState enum** (in `DialogueTableEditorTypes.h`):
```cpp
UENUM(BlueprintType)
enum class EValidationState : uint8
{
    Unknown UMETA(DisplayName = "Unknown"),  // Yellow - needs validation
    Valid UMETA(DisplayName = "Valid"),      // Green - passed validation
    Invalid UMETA(DisplayName = "Invalid")   // Red - has errors
};
```

**ListsVersionGuid** - Persisted on TableData, bumped when asset lists change:
```cpp
// In UDialogueTableData / UNPCTableData:
UPROPERTY(EditAnywhere, Category = "Validation")
FGuid ListsVersionGuid;

void BumpListsVersion() { ListsVersionGuid = FGuid::NewGuid(); }
```

### Validation Cache Fields

**Dialogue rows** (`FDialogueTableRow`):
```cpp
// Existing token validation (unchanged)
UPROPERTY(Transient) bool bEventsValid = true;
UPROPERTY(Transient) FString EventsValidationError;
UPROPERTY(Transient) bool bConditionsValid = true;
UPROPERTY(Transient) FString ConditionsValidationError;

// New staleness detection
UPROPERTY(Transient) uint32 ValidationInputHash = 0;
```

**NPC rows** (`FNPCTableRow`):
```cpp
UPROPERTY(Transient) EValidationState ValidationState = EValidationState::Unknown;
UPROPERTY(Transient) FString ValidationSummary;  // e.g., "2 error(s), 1 warning(s)"
UPROPERTY(Transient) int32 ValidationIssueCount = 0;
UPROPERTY(Transient) uint32 ValidationInputHash = 0;
```

### Hash Recipe for Staleness Detection

```cpp
ValidationInputHash = Hash(
    EditableFieldsHash +      // Row's editable fields
    ListsVersionGuid +        // Asset lists version
    SpecVersion               // Token registry version (Dialogue only)
)
```

**Staleness rule:** If stored hash doesn't match computed hash → validation is stale (Unknown state).

### Validator API (Cache-Writing Methods)

**FDialogueTableValidator**:
```cpp
// Validate all rows and write results to cache
static FDialogueValidationResult ValidateAllAndCache(
    TArray<FDialogueTableRow>& Rows,
    const FGuid& ListsVersionGuid
);

// Validate single row and write cache
static TArray<FDialogueValidationIssue> ValidateRowAndCache(
    FDialogueTableRow& Row,
    const TArray<FDialogueTableRow>& AllRows,
    const FGuid& ListsVersionGuid
);

// Compute hash for staleness detection
static uint32 ComputeValidationInputHash(
    const FDialogueTableRow& Row,
    const FGuid& ListsVersionGuid
);
```

**FNPCTableValidator**:
```cpp
// Same pattern
static FNPCValidationResult ValidateAllAndCache(
    TArray<FNPCTableRow>& Rows,
    const FGuid& ListsVersionGuid
);

static TArray<FNPCValidationIssue> ValidateRowAndCache(
    FNPCTableRow& Row,
    const TArray<FNPCTableRow>& AllRows,
    const FGuid& ListsVersionGuid
);

static uint32 ComputeValidationInputHash(
    const FNPCTableRow& Row,
    const FGuid& ListsVersionGuid
);
```

### Editor Integration

**OnValidateClicked / OnGenerateClicked:**
```cpp
// Call cache-writing validation
FDialogueValidationResult Result = FDialogueTableValidator::ValidateAllAndCache(
    TableData->Rows,
    TableData->ListsVersionGuid
);
RefreshList();
UpdateStatusBar();
```

**NPC editor note:** Since NPC uses `TSharedPtr` copies in `AllRows`, validation results must be copied back:
```cpp
// Validate a local copy
TArray<FNPCTableRow> RowsToValidate;
for (const auto& RowPtr : AllRows) { RowsToValidate.Add(*RowPtr); }

FNPCValidationResult Result = FNPCTableValidator::ValidateAllAndCache(RowsToValidate, ListsVersionGuid);

// Copy cache fields back by matching RowId
for (int32 i = 0; i < RowsToValidate.Num() && i < AllRows.Num(); i++)
{
    if (AllRows[i]->RowId == RowsToValidate[i].RowId)
    {
        AllRows[i]->ValidationState = RowsToValidate[i].ValidationState;
        AllRows[i]->ValidationSummary = RowsToValidate[i].ValidationSummary;
        AllRows[i]->ValidationIssueCount = RowsToValidate[i].ValidationIssueCount;
        AllRows[i]->ValidationInputHash = RowsToValidate[i].ValidationInputHash;
    }
}
```

**UpdateStatusBar (O(1) cache read):**
```cpp
void UpdateStatusBar()
{
    int32 InvalidCount = 0, UnknownCount = 0;
    for (const auto& RowPtr : AllRows)
    {
        if (RowPtr->ValidationState == EValidationState::Invalid)
            InvalidCount++;
        else if (RowPtr->ValidationState == EValidationState::Unknown)
            UnknownCount++;
    }
    // Update status text using cached counts
}
```

### Files Modified

| File | Changes |
|------|---------|
| `DialogueTableEditorTypes.h` | Added `EValidationState` enum, `ValidationInputHash` field, `ComputeEditableFieldsHash()` |
| `NPCTableEditorTypes.h` | Added cache fields, `ComputeEditableFieldsHash()`, `InvalidateValidation()` |
| `DialogueTokenRegistry.h` | Added `SpecVersion` constant |
| `DialogueTableValidator.h/.cpp` | Added `ValidateAllAndCache()`, `ValidateRowAndCache()`, `ComputeValidationInputHash()` |
| `NPCTableValidator.h/.cpp` | Same as above |
| `SDialogueTableEditor.cpp` | Updated `OnValidateClicked()`, `OnGenerateClicked()` to use cache |
| `SNPCTableEditor.cpp` | Updated same methods, plus `UpdateStatusBar()` for O(1) reads |

### Commits

| Commit | Description |
|--------|-------------|
| `64acc9f` | v4.5: Add validation cache fields and hash infrastructure |
| `9ac4220` | v4.5: Wire up validators with cache-writing methods |
| `e80a177` | v4.5.1: Wire editors to use ValidateAllAndCache |
| `ff87352` | v4.5.2: NPC/Dialogue alignment (Duplicate, GUID handling, status bar) |
| `c5d6d98` | v4.5.3: Minor consistency (GetValidationState, AddRow, validation coloring) |

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

## Safety System Files Reference

Files modified during the v4.8.4/v4.9/v4.9.1 safety audit:

| File | Safety Features |
|------|-----------------|
| `SNPCTableEditor.cpp` | `bIsBusy` guard, `IsEnabled_Lambda` on 6 buttons, save-fail abort, version check |
| `SDialogueTableEditor.cpp` | `bIsBusy` guard, `IsEnabled_Lambda` on 7 buttons, save-fail abort, version check (x2) |
| `DialogueXLSXSyncEngine.cpp` | Sort entries by DialogueID+NodeID (v4.9.1) |
| `NPCXLSXSyncEngine.cpp` | Already sorted by display name (no changes needed) |
| `NPCTableEditorTypes.h` | `InvalidateValidation()` method |
| `DialogueTableEditorTypes.h` | `InvalidateValidation()` method |

**Key Patterns by Version:**

| Version | Pattern | Location |
|---------|---------|----------|
| v4.8.4 | `TGuardValue<bool> BusyGuard(bIsBusy, true)` | All heavy operation handlers |
| v4.9 | `.IsEnabled_Lambda([this]() { return !bIsBusy; })` | All heavy operation buttons |
| v4.9.1 | `Result.Entries.Sort([](A, B) { ... })` | DialogueXLSXSyncEngine.cpp:309 |

---

## Original Documents (Consolidated)

- v4.1_DialogueTableEditor_Handoff.md (deleted)
- v4.1_NPCTableEditor_Handoff.md (deleted)
