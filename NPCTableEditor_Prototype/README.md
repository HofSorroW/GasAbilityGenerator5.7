# NPC Table Editor - Prototype

**Location:** `Plugins/GasAbilityGenerator/NPCTableEditor_Prototype/`

This is a **prototype/example** of the NPC Table Editor design. These files are NOT compiled with the plugin - they're for review and discussion before integration.

---

## Concept

An Excel-like spreadsheet editor inside Unreal Engine for managing all NPCs in one place.

### Problems it solves:
1. **Scattered data** - Currently NPC data is spread across multiple windows (NPCDefinition, AbilityConfig, ActivityConfig, Schedule, etc.)
2. **No overview** - Hard to see all NPCs at a glance
3. **Tedious copy-paste** - Creating similar NPCs requires opening many assets
4. **Documentation** - No easy way to document "which NPC has which items, at which spawner"

### Solution:
One table view where each **row = one NPC** with all its settings visible and editable.

---

## Files

| File | Purpose |
|------|---------|
| `NPCTableEditorTypes.h` | Data structures (`FNPCTableRow`, `UNPCTableData`, column definitions) |
| `SNPCTableEditor.h` | Slate widget header (table view, toolbar, actions) |
| `SNPCTableEditor.cpp` | Slate widget implementation |
| `ExampleData.h` | Sample NPC data (Blacksmith, Guards, Bandits, Father) |
| `README.md` | This file |

---

## UI Layout

```
+------------------------------------------------------------------+
| [+ Add NPC] [Duplicate] [Delete]     [Generate Assets] [Sync] [CSV] |
+------------------------------------------------------------------+
| [Search NPCs...]                                                    |
+------------------------------------------------------------------+
| Status | NPC Name   | NPC ID      | Blueprint    | AbilityConfig | ... |
+--------+------------+-------------+--------------+---------------+-----+
| Synced | Blacksmith | blacksmith_01 | BP_Blacksmith | AC_Craftsman | ... |
| Synced | Merchant   | merchant_01   | BP_Merchant   | AC_Civilian  | ... |
| New    | Bandit     | bandit_01     | BP_Bandit     | AC_BanditMelee | ... |
| Error  | (empty)    | (empty)       |              |               | ... |
+--------+------------+-------------+--------------+---------------+-----+
| Total: 4 NPCs | Showing: 4 | Selected: 1                           |
+------------------------------------------------------------------+
```

---

## Columns

### Identity (Required)
- **Status** - New / Modified / Synced / Error (color-coded)
- **NPCName** - Used for asset naming (NPCDef_{NPCName})
- **NPCId** - Unique runtime identifier
- **DisplayName** - Shown in-game

### Asset References (Dropdown pickers with filtering)
- **NPCBlueprint** - Filter: `NarrativeNPCCharacter`
- **AbilityConfig** - Filter: `AbilityConfiguration`
- **ActivityConfig** - Filter: `NPCActivityConfiguration`
- **Schedule** - Filter: `NPCActivitySchedule`
- **Dialogue** - Filter: `Dialogue`

### Location
- **SpawnerPOI** - POI or Spawner name
- **LevelName** - Map name

### Vendor
- **IsVendor** - Checkbox
- **ShopName** - Display name

### Items (comma-separated)
- **DefaultItems** - `EI_Sword, EI_Shield`
- **ShopItems** - `IC_Weapons, IC_Potions`

### Combat
- **MinLevel** / **MaxLevel**
- **Factions** - Comma-separated tags

### Metadata
- **Notes** - Free-form comments

---

## Features

### Inline Editing
Click any cell to edit directly. Changes mark row as "Modified".

### Asset Pickers
Asset reference columns show filtered dropdowns - only relevant assets appear.

### Sorting & Filtering
- Click column header to sort
- Search box filters by name/id/notes
- (Future: per-column filter dropdowns)

### Actions

| Button | Action |
|--------|--------|
| **+ Add NPC** | Add empty row |
| **Duplicate** | Copy selected row with "_Copy" suffix |
| **Delete** | Remove selected rows |
| **Generate Assets** | Create/update UE assets from rows |
| **Sync from Assets** | Scan project, update table from existing assets |
| **Export CSV** | Save table to CSV file |
| **Import CSV** | Load table from CSV file |

### Status Colors
- 🔵 **New** - Not yet generated
- 🟡 **Modified** - Changed since last generation
- 🟢 **Synced** - Matches generated assets
- 🔴 **Error** - Missing required fields

---

## Data Flow

```
[Create/Edit]
     │
     ▼
┌─────────────────┐
│ UNPCTableData   │  ← DataAsset saved with project
│ (Table rows)    │
└────────┬────────┘
         │
    [Generate]
         │
         ▼
┌─────────────────┐
│ NPCDef_*, AC_*, │  ← Real Narrative Pro assets
│ Schedule_*, etc │
└────────┬────────┘
         │
    [Sync]
         │
         ▼
┌─────────────────┐
│ UNPCTableData   │  ← Table updated from assets
│ (refreshed)     │
└─────────────────┘
```

---

## Next Steps

1. **Review this prototype** - Does the design make sense?
2. **Discuss changes** - What columns to add/remove?
3. **Implement for real** - Move to plugin source, compile, test
4. **Iterate** - Add features based on usage

---

## Example Data

See `ExampleData.h` for sample NPCs:
- Town NPCs: Blacksmith, Merchant, Innkeeper
- Guards: Gate Guard, Patrol Guard
- Quest NPCs: Elder Thomas
- Enemies: Bandit, Bandit Archer, Bandit Leader
- Companion: Father (your spider companion)

---

## Questions for Review

1. **Columns** - Are these the right columns? Missing anything important?
2. **Workflow** - Generate first, then sync? Or sync first to discover existing?
3. **Asset pickers** - Show full path or just asset name?
4. **Arrays** - Better way to handle multiple items than comma-separated?
5. **Grouping** - Should we add row grouping (by level, by faction)?
6. **Validation** - What errors should block generation?
