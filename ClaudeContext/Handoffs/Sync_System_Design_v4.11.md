# XLSX Sync System Design - v4.11

**Date:** 2026-01-16
**Status:** DESIGN (Pending Implementation)
**Primary Doc:** Table_Editors_Reference.md (Section: v4.11 Design)

This is a backup/standalone document for the Sync Approval System design.

---

## Problem Statement

Deep audit revealed critical gaps in the XLSX sync system:

1. **Dialogue Import bypasses sync** - `OnImportXLSXClicked()` directly replaces `TableData->Rows = Result.Rows` without comparing to UE data
2. **NPC Import uses sync but with empty BaseRows** - Comparison works but can't distinguish "who changed"
3. **BaseRows TODO never completed** - Both editors have `TArray<...> BaseRows; // Empty = first sync`
4. **Auto-resolve hides changes** - Non-conflict statuses auto-resolve without user review

---

## Why BaseRows Snapshot is Flawed

The original design: "Store BaseRows at export time, compare against it at import."

**Fatal flaw scenario:**
```
1. Export XLSX     → Base = {Seth, AC: RunAndGun}
2. Edit in UE      → UE = {Seth, AC: Stealth}  (NO export, Base unchanged)
3. Edit in Excel   → Excel = {Seth, AC: Melee}
4. Import Excel    → Compare against STALE Base
```

Result: System claims to know "who changed" based on stale data. If UE and Excel changed different fields, user might accept Excel and **silently lose UE changes**.

---

## Solution: Per-Row LastSyncedHash

Instead of separate BaseRows snapshot, store the sync hash **per row** in TableData:

```cpp
struct FDialogueTableRow
{
    // ... existing fields ...

    // v4.11: Updated ONLY after successful sync/import/export
    UPROPERTY(Transient)
    int64 LastSyncedHash = 0;  // 0 = never synced
};
```

**Update rules:**

| Action | LastSyncedHash Updated? |
|--------|-------------------------|
| Edit row in table | No |
| Save TableData | No |
| Export XLSX | Yes - set to current content hash |
| Import/Sync (after apply) | Yes - set to merged content hash |

---

## Complete Sync Decision Matrix

**Legend:**
- `H1` = LastSyncedHash (Base - state at last sync)
- `H2` = Current UE table content
- `H3` = Imported Excel content
- `0` = Never synced (no base)
- `—` = Row doesn't exist

| # | NPC | Base | UE | Excel | Status | Auto? | User Prompt | Option 1 | Option 2 | Option 3 | Action |
|---|-----|------|-----|-------|--------|-------|-------------|----------|----------|----------|--------|
| 1 | Seth | RunAndGun | RunAndGun | RunAndGun | Unchanged | Yes | — | — | — | — | — |
| 2 | Seth | RunAndGun | RunAndGun | Stealth | ModifiedInExcel | No | "Excel changed AC" | `RunAndGun` | `Stealth` | — | — |
| 3 | Seth | RunAndGun | Melee | RunAndGun | ModifiedInUE | No | "UE changed AC" | `RunAndGun` | `Melee` | — | — |
| 4 | Seth | RunAndGun | Stealth | Stealth | Converged | No | "Both changed same way" | `RunAndGun` | `Stealth` | — | — |
| 5 | Seth | RunAndGun | Melee | Stealth | **Conflict** | No | "All three differ" | `RunAndGun` | `Melee` | `Stealth` | — |
| 6 | Seth | RunAndGun | RunAndGun | *(deleted)* | DeletedInExcel | No | "Excel deleted row" | `RunAndGun` | — | — | `REMOVE` |
| 7 | Seth | RunAndGun | *(deleted)* | RunAndGun | DeletedInUE | No | "UE deleted row" | `RunAndGun` | — | — | `REMOVE` |
| 8 | Seth | RunAndGun | Melee | *(deleted)* | **DeleteConflict** | No | "UE modified, Excel deleted" | `RunAndGun` | `Melee` | — | `REMOVE` |
| 9 | Seth | RunAndGun | *(deleted)* | Stealth | **DeleteConflict** | No | "UE deleted, Excel modified" | `RunAndGun` | — | `Stealth` | `REMOVE` |
| 10 | Seth | RunAndGun | *(deleted)* | *(deleted)* | BothDeleted | No | "Both deleted row" | `RunAndGun` | — | — | `REMOVE` |
| 11 | Marcus | *(none)* | Melee | *(missing)* | AddedInUE | No | "New row in UE only" | — | `Melee` | — | `REMOVE` |
| 12 | Elena | *(none)* | *(missing)* | Stealth | AddedInExcel | No | "New row in Excel only" | — | — | `Stealth` | `REMOVE` |
| 13 | Marcus | *(none)* | Melee | Melee | NewIdentical | No | "New row, both identical" | — | `Melee` | — | `REMOVE` |
| 14 | Marcus | *(none)* | Melee | Stealth | **Conflict** | No | "New row, content differs" | — | `Melee` | `Stealth` | `REMOVE` |

**Key design decisions:**
- **Only Case 1 (Unchanged) auto-resolves** - all others require explicit user approval
- **No pre-selection** - user must click to approve each row (prevents accidental acceptance)
- **Action column conditional** - REMOVE only appears when row can be deleted (cases 6-14)
- **Shows actual values** - not "Keep UE" but `AC_Melee`, `AC_Stealth`, etc.

---

## Full-Screen Approval Window

**Flow:**
1. User imports XLSX
2. System compares Base/UE/Excel for all rows
3. Status bar shows: "12 changes detected, 3 conflicts"
4. User clicks **"Generate"**
5. **Full-screen approval window opens**
6. User reviews all, clicks to select preferred value for each row
7. When all resolved → **"Apply"** button enables
8. Final confirmation prompt → Changes applied

**Window Layout:**

```
+-------------------------------------------------------------------------------------+
| SYNC APPROVAL                                                    [Cancel] [Apply]   |
+-------------------------------------------------------------------------------------+
| 12 changes | 0 of 12 resolved                                                       |
+----------+-----------------+------------------+------------------+------------------+------------+
|   NPC    |     Field       |   Last Export    |       UE         |     Excel        |   Action   |
+----------+-----------------+------------------+------------------+------------------+------------+
| Seth     | AbilityConfig   |  o AC_RunAndGun  |  o AC_Melee      |  o AC_Stealth    |     -      |
+----------+-----------------+------------------+------------------+------------------+------------+
| Seth     | BehaviorTree    |  o BT_Guard      |  o BT_Guard      |  o BT_Patrol     |     -      |
+----------+-----------------+------------------+------------------+------------------+------------+
| Marcus   | (NEW ROW)       |       -          |  o AC_Melee      |       -          |  o REMOVE  |
+----------+-----------------+------------------+------------------+------------------+------------+
| Elena    | (NEW ROW)       |       -          |       -          |  o AC_Stealth    |  o REMOVE  |
+----------+-----------------+------------------+------------------+------------------+------------+
| Jake     | (DELETED)       |  o AC_RunAndGun  |       -          |       -          |  o REMOVE  |
+----------+-----------------+------------------+------------------+------------------+------------+
                                                                   [Apply] (disabled until all resolved)
```

**Interaction:**

| Element | Behavior |
|---------|----------|
| `o` | Unselected option (clickable) |
| `*` | Selected option (user clicked) |
| `-` | Not available (doesn't exist) |
| Click on cell | Selects that value, deselects others in row |
| Row highlight | Yellow = unresolved, Green = resolved |
| **[Apply]** | Enabled only when all rows have a selection |

**Action Column Logic:**

| Condition | Action Column |
|-----------|---------------|
| All three exist (cases 1-5) | `-` (no remove option) |
| Any side missing (cases 6-14) | `o REMOVE` (clickable) |

**Mutual exclusion per row:** Either a value cell OR REMOVE is selected, never both.

---

## Fixes Required

| Component | Current State | Fix Needed |
|-----------|---------------|------------|
| `FDialogueTableRow` | No LastSyncedHash | Add `int64 LastSyncedHash` field |
| `FNPCTableRow` | No LastSyncedHash | Add `int64 LastSyncedHash` field |
| `OnImportXLSXClicked` (Dialogue) | Direct replace | Use sync comparison like NPC |
| `AutoResolveNonConflicts` | Auto-resolves 8 statuses | Only auto-resolve Unchanged |
| `SDialogueXLSXSyncDialog` | Shows "Keep UE" labels | Show actual values |
| `SNPCXLSXSyncDialog` | Shows "Keep UE" labels | Show actual values |
| Export functions | Don't update hash | Update LastSyncedHash after export |
| Sync apply functions | Don't update hash | Update LastSyncedHash after apply |

---

## Implementation Priority

| Priority | Task |
|----------|------|
| P0 | Add LastSyncedHash to row structs |
| P0 | Update AutoResolveNonConflicts (only Unchanged) |
| P1 | Fix Dialogue Import to use sync comparison |
| P1 | Update sync dialogs to show actual values |
| P1 | Add Action column with conditional REMOVE |
| P2 | Update Export to set LastSyncedHash |
| P2 | Update Sync Apply to set LastSyncedHash |

---

## Code Locations

**Current sync implementation:**
- `SDialogueTableEditor.cpp:2580-2668` - OnImportXLSXClicked (THE GAP)
- `SDialogueTableEditor.cpp:2670-2779` - OnSyncXLSXClicked
- `SNPCTableEditor.cpp:3083-3211` - OnImportXLSXClicked (uses sync)
- `DialogueXLSXSyncEngine.cpp:336-377` - AutoResolveNonConflicts
- `NPCXLSXSyncEngine.cpp:348-381` - AutoResolveNonConflicts
- `DialogueXLSXSyncEngine.cpp:153-236` - DetermineStatus
- `SDialogueXLSXSyncDialog.cpp` - Current sync dialog UI
- `SNPCXLSXSyncDialog.cpp` - Current sync dialog UI

**Row structs:**
- `DialogueTableEditorTypes.h` - FDialogueTableRow
- `NPCTableEditorTypes.h` - FNPCTableRow

---

*Design document created 2026-01-16*
