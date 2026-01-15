# Table Editors Reference

**Consolidated:** 2026-01-15
**Status:** Implemented in GasAbilityGenerator v4.2

This document consolidates the Dialogue Table Editor and NPC Table Editor handoffs.

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

## Shared UI Patterns

### Button Styles
```cpp
.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")   // Generate
.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")   // Add
.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")    // Delete
.ButtonStyle(FAppStyle::Get(), "SimpleButton")         // Menu buttons
```

### Font Styles
- Headers: `FCoreStyle::GetDefaultFontStyle("Bold", 9)`
- Cell text: `FCoreStyle::GetDefaultFontStyle("Regular", 9)`
- Filter text: `FCoreStyle::GetDefaultFontStyle("Regular", 8)`

### Multi-Select Filter Pattern
```cpp
struct FColumnFilterState
{
    FString TextFilter;
    TSet<FString> SelectedValues;  // Empty = all pass, OR logic
    TArray<TSharedPtr<FString>> DropdownOptions;
};
```

### Status Bar Update (v4.2.8 Fix)
```cpp
// Store reference via SAssignNew
SAssignNew(StatusBar, SHorizontalBox)...

// Invalidate explicitly after data changes
StatusBar->Invalidate(EInvalidateWidgetReason::Paint);
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
