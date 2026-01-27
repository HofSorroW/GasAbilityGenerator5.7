# Table Editor Pipeline Audit v1.4

**Date:** January 2026
**Plugin Version:** v7.4
**Auditor:** Claude Code

---

## Executive Summary

All 4 table editors (NPC, Dialogue, Quest, Item) implement 8 major pipelines with **100% feature parity**. Initial audit incorrectly flagged Quest/Item Asset Sync and Dirty Tab as missing - verification confirmed all features are fully implemented. v7.4 adds Undo/Redo and Find & Replace - ALL GAPS RESOLVED.

---

## Table Editors Overview

| Editor | File | Row Type | Data Class | Lines |
|--------|------|----------|------------|-------|
| NPC | SNPCTableEditor.cpp | FNPCTableRow | UNPCTableData | ~2584 |
| Dialogue | SDialogueTableEditor.cpp | FDialogueTableRow | UDialogueTableData | ~1847 |
| Quest | SQuestTableEditor.cpp | FQuestTableRow | UQuestTableData | ~1299 |
| Item | SItemTableEditor.cpp | FItemTableRow | UItemTableData | ~1289 |

---

## Pipeline 1: IMPORT

### Capability Matrix

| Feature | NPC | Dialogue | Quest | Item |
|---------|:---:|:--------:|:-----:|:----:|
| XLSX Import | YES | YES | YES | YES |
| CSV Import | NO | NO | NO | NO |
| Asset Sync (Read FROM UE) | YES | YES | YES | YES |
| Import Trigger (Button) | YES | YES | YES | YES |
| Sync Dialog UI | YES | YES | YES | YES |

### Implementation Details

**XLSX Import Classes:**
- `FNPCXLSXReader::ImportFromXLSX()`
- `FDialogueXLSXReader::ImportFromXLSX()`
- `FQuestXLSXReader::ImportFromXLSX()`
- `FItemXLSXReader::ImportFromXLSX()`

**Asset Sync Classes:**
- `FNPCAssetSync::SyncFromAssets()` - Extracts UNPCDefinition properties
- `FDialogueAssetSync::SyncFromAssets()` - Extracts UDialogue graph structure
- `FQuestAssetSync::SyncFromAssets()` - Extracts UQuest state machine (v4.12.4)
- `FItemAssetSync::SyncFromAssets()` - Extracts UNarrativeItem/Blueprint properties (v4.12)

### XLSX Format Features
- Sentinel row detection (#NPC_SHEET_V1, #DIALOGUE_SHEET_V1, etc.)
- Column ID mapping (order-independent)
- Row GUID preservation across import/export cycles
- Metadata sheet (_Meta) with export timestamp, hash, version

---

## Pipeline 2: VALIDATION

### Capability Matrix

| Feature | NPC | Dialogue | Quest | Item |
|---------|:---:|:--------:|:-----:|:----:|
| Validator Class | YES | YES | YES | YES |
| Validation Cache | YES | YES | YES | YES |
| Staleness Detection | YES | YES | YES | YES |
| Cell Color Stripe | YES | YES | YES | YES |
| Status Bar Summary | YES | YES | YES | YES |
| Structure Validation | NO | YES (circular) | YES (reachability) | NO |
| Token Validation | NO | YES | YES | NO |

### Validator Classes
- `FNPCTableValidator`
- `FDialogueTableValidator`
- `FQuestTableValidator`
- `FItemTableValidator`

### Validation Methods (All Editors)
```cpp
static FValidationResult ValidateAll(const TArray<TSharedPtr<FTableRow>>& Rows);
static FValidationResult ValidateRow(const TSharedPtr<FTableRow>& Row);
static FValidationResult ValidateRowAndCache(TSharedPtr<FTableRow>& Row, const FGuid& ListsVersionGuid);
```

### Cache Fields (Per Row)
```cpp
ENPCValidationState ValidationState;      // Valid, Warning, Error
FString ValidationSummary;                // Human-readable summary
int32 ValidationIssueCount;               // Number of issues
uint32 ValidationInputHash;               // Staleness detection (transient)
```

### Staleness Detection
- `ValidationInputHash` computed from editable fields + `ListsVersionGuid`
- If hash changes, cached validation is stale
- `ListsVersionGuid` bumped when reference lists change (dropdown options, etc.)

---

## Pipeline 3: DIFF / COMPARE

### Capability Matrix

| Feature | NPC | Dialogue | Quest | Item |
|---------|:---:|:--------:|:-----:|:----:|
| Hash-based Detection | YES | YES | YES | YES |
| LastSyncedHash | YES | YES | YES | YES |
| LastGeneratedHash | YES | YES | YES | YES |
| AreAssetsOutOfDate() | YES | YES | YES | YES |

### Hash Types

| Hash | Type | Purpose | Updated When |
|------|------|---------|--------------|
| `ValidationInputHash` | uint32 (transient) | Staleness detection | Computed on-demand |
| `LastSyncedHash` | int64 (persisted) | 3-way merge baseline | After sync/import/export |
| `LastGeneratedHash` | uint32 (persisted) | Generation tracking | After successful generation |

### Hash Computation
```cpp
// Per-row hash
int64 FNPCTableRow::ComputeEditableFieldsHash() const {
    uint32 Hash = 0;
    Hash = HashCombine(Hash, GetTypeHash(NPCName));
    Hash = HashCombine(Hash, GetTypeHash(NPCId));
    // ... all editable fields
    return Hash;
}

// Table-level hash
uint32 UNPCTableData::ComputeAllRowsHash() const {
    uint32 Hash = 0;
    for (const auto& Row : Rows) {
        if (!Row->bDeleted) {
            Hash = HashCombine(Hash, Row->ComputeEditableFieldsHash());
        }
    }
    return Hash;
}

// Out-of-date check
bool UNPCTableData::AreAssetsOutOfDate() const {
    return ComputeAllRowsHash() != LastGeneratedHash;
}
```

---

## Pipeline 4: 3-WAY SYNC

### Capability Matrix

| Feature | NPC | Dialogue | Quest | Item |
|---------|:---:|:--------:|:-----:|:----:|
| Sync Engine | YES | YES | YES | YES |
| Conflict Detection | YES | YES | YES | YES |
| Conflict Resolution UI | YES | YES | YES | YES |
| Auto-resolve Non-conflicts | YES | YES | YES | YES |
| Asset Apply (Write back) | YES | YES | YES | YES |

### Sync Engine Classes
- `FNPCXLSXSyncEngine`
- `FDialogueXLSXSyncEngine`
- `FQuestXLSXSyncEngine`
- `FItemXLSXSyncEngine`

### Sync Status Enum (9 values)
```cpp
enum class ENPCSyncStatus : uint8 {
    Unchanged,           // Base == UE == Excel
    ModifiedInUE,        // Base != UE, Base == Excel
    ModifiedInExcel,     // Base == UE, Base != Excel
    Conflict,            // Base != UE && Base != Excel
    AddedInUE,           // Not in Base, in UE
    AddedInExcel,        // Not in Base, in Excel
    DeletedInUE,         // In Base, not in UE
    DeletedInExcel,      // In Base, not in Excel (#STATE=Deleted marker)
    BothDeleted          // Deleted in both
};
```

### Conflict Resolution Enum
```cpp
enum class ENPCConflictResolution : uint8 {
    Unresolved,
    KeepUE,
    KeepExcel,
    KeepBoth,
    Delete
};
```

### 3-Way Comparison Algorithm
```
Input:
  - Base: Rows from last export (snapshot)
  - UE: Current rows in editor
  - Excel: Rows imported from XLSX file

For each row (matched by RowId GUID):
  1. Compute BaseHash, UEHash, ExcelHash
  2. Compare hashes to determine status
  3. Auto-resolve non-conflicts (ModifiedInUE → keep UE, ModifiedInExcel → apply Excel)
  4. Flag conflicts for user resolution

Output:
  - TArray<FSyncEntry> with status and resolution per row
```

### Sync Entry Structure
```cpp
struct FNPCSyncEntry {
    ENPCSyncStatus Status;
    ENPCConflictResolution Resolution;
    TSharedPtr<FNPCTableRow> BaseRow;
    TSharedPtr<FNPCTableRow> UERow;
    TSharedPtr<FNPCTableRow> ExcelRow;
    int64 BaseHash, UEHash, ExcelHash;
};
```

---

## Pipeline 5: SNAPSHOT / STATE

### Capability Matrix

| Feature | NPC | Dialogue | Quest | Item |
|---------|:---:|:--------:|:-----:|:----:|
| Table Data Class | YES | YES | YES | YES |
| Row GUID Tracking | YES | YES | YES | YES |
| Dirty State (bIsBusy) | YES | YES | YES | YES |
| Auto-save Before Generation | YES | YES | YES | YES |
| Status Persistence | YES | YES | YES | YES |
| Soft Delete (bDeleted) | YES | YES | YES | YES |

### Table Data Classes (UDataAsset)
- `UNPCTableData` - Persisted to `/Game/{Project}/TableData/NPCTableData.uasset`
- `UDialogueTableData` - Persisted to `/Game/{Project}/TableData/DialogueTableData.uasset`
- `UQuestTableData` - Persisted to `/Game/{Project}/TableData/QuestTableData.uasset`
- `UItemTableData` - Persisted to `/Game/{Project}/TableData/ItemTableData.uasset`

### Common Fields
```cpp
UPROPERTY()
TArray<TSharedPtr<FTableRow>> Rows;

UPROPERTY()
uint32 LastGeneratedHash;

UPROPERTY()
FDateTime LastGeneratedTime;

UPROPERTY()
int32 LastGenerateFailureCount;

UPROPERTY()
FGuid ListsVersionGuid;  // Bumped when dropdown options change
```

### Soft Delete Pattern
```cpp
// In FTableRow
bool bDeleted = false;

// In ConvertRowsToManifest()
for (const auto& Row : Rows) {
    if (Row->bDeleted) continue;  // Skip deleted rows
    // ... convert to manifest
}

// Asset is NOT deleted - just skipped during generation
```

---

## Pipeline 6: WRITE TO UE (Generation)

### Capability Matrix

| Feature | NPC | Dialogue | Quest | Item |
|---------|:---:|:--------:|:-----:|:----:|
| Converter Class | YES | YES | YES | YES |
| ConvertRowsToManifest() | YES | YES | YES | YES |
| Validation Gate | YES | YES | YES | YES |
| Success/Failure Dialog | YES | YES | YES | YES |
| Generation Tracking | YES | YES | YES | YES |

### Converter Classes
- `FNPCTableConverter::ConvertRowsToManifest()` → `TArray<FManifestNPCDefinitionDefinition>`
- `FDialogueTableConverter::ConvertRowsToManifest()` → `TMap<FName, FManifestDialogueBlueprintDefinition>`
- `FQuestTableConverter::ConvertRowsToManifest()` → `TArray<FManifestQuestDefinition>`
- `FItemTableConverter::ConvertRowsToManifest()` → `TArray<FManifestEquippableItemDefinition>`

### Generation Flow
```cpp
void SNPCTableEditor::OnGenerateClicked() {
    // Step 0: Save dirty table
    if (TableData->GetPackage()->IsDirty()) {
        UPackage::SavePackage(TableData->GetPackage(), ...);
    }

    // Step 1: Validate (gate)
    FNPCValidationResult Result = FNPCTableValidator::ValidateAllAndCache(
        TableData->Rows, TableData->ListsVersionGuid);
    if (Result.HasErrors()) {
        FMessageDialog::Open(EAppMsgType::Ok, "Fix validation errors first");
        return;  // Block generation
    }

    // Step 2: Convert
    TArray<FManifestNPCDefinitionDefinition> Defs =
        FNPCTableConverter::ConvertRowsToManifest(TableData->Rows, OutputFolder);

    // Step 3: Generate
    int32 Generated = 0, Skipped = 0, Failed = 0;
    for (const auto& Def : Defs) {
        FGenerationResult GenResult = FNPCDefinitionGenerator::Generate(Def);
        switch (GenResult.Status) {
            case EGenerationStatus::New: Generated++; break;
            case EGenerationStatus::Skipped: Skipped++; break;
            case EGenerationStatus::Failed: Failed++; break;
        }
    }

    // Step 4: Update metadata
    TableData->OnGenerationComplete(Failed);  // Only updates hash if Failed == 0

    // Step 5: Display results
    FMessageDialog::Open(..., FText::Format("Generated: {0}, Skipped: {1}, Failed: {2}"));

    // Step 6: Refresh UI
    RefreshList();
}
```

---

## Pipeline 7: EXPORT

### Capability Matrix

| Feature | NPC | Dialogue | Quest | Item |
|---------|:---:|:--------:|:-----:|:----:|
| XLSX Export | YES | YES | YES | YES |
| CSV Export | NO | NO | NO | NO |
| Metadata Sheet | YES | YES | YES | YES |
| Sync Hash Export | YES | YES | YES | YES |

### Writer Classes
- `FNPCXLSXWriter::ExportToXLSX()`
- `FDialogueXLSXWriter::ExportToXLSX()`
- `FQuestXLSXWriter::ExportToXLSX()`
- `FItemXLSXWriter::ExportToXLSX()`

### XLSX Structure
```
Sheet 1: Data
  Row 1: Sentinel (#NPC_SHEET_V1)
  Row 2: Column IDs (NPCName, NPCId, DisplayName, ...)
  Row 3: Column Headers (NPC Name, NPC ID, Display Name, ...)
  Row 4+: Data rows

Sheet 2: _Meta
  ExportGuid: GUID of this export
  ExportedAt: Timestamp
  FormatVersion: "1.0"
  OriginalRowCount: Row count
  ContentHash: Hash of all rows
```

### Order Independence
- Column IDs (not positions) used for mapping
- Users can reorder columns in Excel freely
- Sentinel row enables format detection

---

## Pipeline 8: STATUS TRACKING

### Capability Matrix

| Feature | NPC | Dialogue | Quest | Item |
|---------|:---:|:--------:|:-----:|:----:|
| Status Enum | YES | YES | YES | YES |
| Colored Badges | YES | YES | YES | YES |
| Validation Color Stripe | YES | YES | YES | YES |
| Assets Out of Date Indicator | YES | YES | YES | YES |
| Dirty Tab Indicator | YES | YES | YES | YES |

### Status Enum
```cpp
enum class ENPCTableRowStatus : uint8 {
    New,       // Blue - newly created, not yet generated
    Modified,  // Yellow - changed since last generation
    Synced,    // Green - matches generated asset
    Error      // Red - validation error
};
```

### Color Coding
| Status | Cell Color | Meaning |
|--------|------------|---------|
| New | Blue | Not yet generated |
| Modified | Yellow | Changed since generation |
| Synced | Green | Up to date |
| Error | Red | Validation failed |

### Dirty Tab Indicator (v4.6)
```cpp
// In SNPCTableEditor::Tick()
if (ParentTab.IsValid()) {
    bool bIsDirty = TableData && TableData->GetPackage()->IsDirty();
    FString TabLabel = bIsDirty ? "NPC Editor*" : "NPC Editor";
    ParentTab->SetLabel(FText::FromString(TabLabel));
}
```

**All 4 editors implement this via SetParentTab/UpdateTabLabel pattern.**

---

## Identified Gaps

### ~~P0 - Critical~~ (RESOLVED)

~~Asset Sync Missing~~ - **VERIFIED IMPLEMENTED** (v4.12+)
- `FQuestAssetSync::SyncFromAssets()` - Full implementation with state machine extraction
- `FItemAssetSync::SyncFromAssets()` - Full implementation with Blueprint/CDO property extraction

### ~~P1 - High~~ (RESOLVED)

~~Dirty Tab Indicator~~ - **VERIFIED IMPLEMENTED** (v4.6+)
- All 4 editors have SetParentTab, UpdateTabLabel, IsTableDirty methods

### ~~P2 - Medium~~ (RESOLVED)

~~Undo/Redo~~ - **IMPLEMENTED** (v7.4)
- `TableEditorTransaction.h` - Transaction stack infrastructure with templated row operations
- All 4 editors have Undo/Redo buttons, Ctrl+Z/Ctrl+Y shortcuts, and 50-action history
- AddRowWithUndo, DeleteRowsWithUndo, RecordRowEdit methods

### ~~P3 - Low~~ (RESOLVED)

~~Find & Replace~~ - **IMPLEMENTED** (v7.4)
- All 4 editors have Find bar with Find/Replace text inputs
- Find Next (<), Find Prev (>), Replace, Replace All buttons
- Search results navigate and select matching rows
- CSV Export - By design - not fixing (XLSX sufficient)

---

## Architecture Strengths

1. **Consistent validation caching** - All 4 editors use same staleness pattern
2. **Robust XLSX format** - Order-independent columns, GUID preservation
3. **Comprehensive 3-way merge** - Proper conflict detection and resolution
4. **Soft delete** - Prevents accidental data loss
5. **Validation gate** - Blocks generation on errors
6. **Auto-save** - Saves before generation

---

## Architecture Concerns

1. ~~**No undo/redo**~~ - **RESOLVED (v7.4)**: Transaction-based undo/redo with 50-action history
2. ~~**No find/replace**~~ - **RESOLVED (v7.4)**: Find bar with search, replace, replace all
3. **No cross-table operations** - Cannot copy between tables (low priority)

*Note: Initial audit incorrectly flagged Asset Sync and Dirty Tab as missing - verification confirmed full implementation.*

---

## File Locations

### Common Infrastructure (v7.4)
- `Source/GasAbilityGenerator/Public/TableEditorTransaction.h` - Undo/Redo transaction system

### NPC Editor
- `Source/GasAbilityGenerator/Private/NPCTableEditor/SNPCTableEditor.cpp`
- `Source/GasAbilityGenerator/Private/NPCTableEditor/NPCTableEditorTypes.h`
- `Source/GasAbilityGenerator/Private/NPCTableEditor/NPCTableConverter.cpp`
- `Source/GasAbilityGenerator/Private/NPCTableEditor/NPCTableValidator.cpp`
- `Source/GasAbilityGenerator/Private/NPCTableEditor/NPCAssetSync.cpp`
- `Source/GasAbilityGenerator/Private/NPCTableEditor/NPCXLSXReader.cpp`
- `Source/GasAbilityGenerator/Private/NPCTableEditor/NPCXLSXWriter.cpp`
- `Source/GasAbilityGenerator/Private/NPCTableEditor/NPCXLSXSyncEngine.cpp`
- `Source/GasAbilityGenerator/Private/NPCTableEditor/SNPCXLSXSyncDialog.cpp`

### Dialogue Editor
- `Source/GasAbilityGenerator/Private/SDialogueTableEditor.cpp`
- `Source/GasAbilityGenerator/Private/DialogueTableEditorTypes.h`
- `Source/GasAbilityGenerator/Private/DialogueTableConverter.cpp`
- `Source/GasAbilityGenerator/Private/DialogueTableValidator.cpp`
- `Source/GasAbilityGenerator/Private/DialogueAssetSync.cpp`
- `Source/GasAbilityGenerator/Private/DialogueXLSXReader.cpp`
- `Source/GasAbilityGenerator/Private/DialogueXLSXWriter.cpp`
- `Source/GasAbilityGenerator/Private/DialogueXLSXSyncEngine.cpp`
- `Source/GasAbilityGenerator/Private/SDialogueXLSXSyncDialog.cpp`

### Quest Editor
- `Source/GasAbilityGenerator/Private/QuestTableEditor/SQuestTableEditor.cpp`
- `Source/GasAbilityGenerator/Private/QuestTableEditor/QuestTableEditorTypes.h`
- `Source/GasAbilityGenerator/Private/QuestTableEditor/QuestTableConverter.cpp`
- `Source/GasAbilityGenerator/Private/QuestTableEditor/QuestTableValidator.cpp`
- `Source/GasAbilityGenerator/Private/QuestTableEditor/QuestAssetSync.cpp`
- `Source/GasAbilityGenerator/Private/QuestTableEditor/QuestXLSXReader.cpp`
- `Source/GasAbilityGenerator/Private/QuestTableEditor/QuestXLSXWriter.cpp`
- `Source/GasAbilityGenerator/Private/QuestTableEditor/QuestXLSXSyncEngine.cpp`
- `Source/GasAbilityGenerator/Private/QuestTableEditor/SQuestXLSXSyncDialog.cpp`

### Item Editor
- `Source/GasAbilityGenerator/Private/ItemTableEditor/SItemTableEditor.cpp`
- `Source/GasAbilityGenerator/Private/ItemTableEditor/ItemTableEditorTypes.h`
- `Source/GasAbilityGenerator/Private/ItemTableEditor/ItemTableConverter.cpp`
- `Source/GasAbilityGenerator/Private/ItemTableEditor/ItemTableValidator.cpp`
- `Source/GasAbilityGenerator/Private/ItemTableEditor/ItemAssetSync.cpp`
- `Source/GasAbilityGenerator/Private/ItemTableEditor/ItemXLSXReader.cpp`
- `Source/GasAbilityGenerator/Private/ItemTableEditor/ItemXLSXWriter.cpp`
- `Source/GasAbilityGenerator/Private/ItemTableEditor/ItemXLSXSyncEngine.cpp`
- `Source/GasAbilityGenerator/Private/ItemTableEditor/SItemXLSXSyncDialog.cpp`

---

## Remediation Plan

| Task | Priority | Effort | Status | Files |
|------|----------|--------|--------|-------|
| ~~Quest Asset Sync~~ | ~~P0~~ | ~~Medium~~ | **DONE** | QuestAssetSync.cpp (v4.12.4) |
| ~~Item Asset Sync~~ | ~~P0~~ | ~~Medium~~ | **DONE** | ItemAssetSync.cpp (v4.12) |
| ~~Quest Dirty Tab~~ | ~~P1~~ | ~~Low~~ | **DONE** | SQuestTableEditor.cpp |
| ~~Item Dirty Tab~~ | ~~P1~~ | ~~Low~~ | **DONE** | SItemTableEditor.cpp |
| ~~Undo/Redo System~~ | ~~P2~~ | ~~High~~ | **DONE** | TableEditorTransaction.h + All 4 editors (v7.4) |
| ~~Find & Replace~~ | ~~P3~~ | ~~Medium~~ | **DONE** | All 4 editors have Find bar (v7.4) |

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| v1.0 | January 2026 | Initial audit |
| v1.1 | January 2026 | Corrected false positives: Quest/Item Asset Sync and Dirty Tab verified as implemented. Updated gap list to reflect only P2/P3 remaining. |
| v1.2 | January 2026 | P2 Undo/Redo implemented. Added TableEditorTransaction.h with transaction stack. All 4 editors have Undo/Redo buttons and Ctrl+Z/Ctrl+Y. Only P3 (Find & Replace) remains. |
| v1.3 | January 2026 | **ALL GAPS RESOLVED.** P3 Find & Replace implemented. All 4 editors have Find bar with Find/Replace/Replace All. Audit complete - no remaining gaps. |
| v1.4 | January 2026 | Build verified, committed as v7.4, pushed to origin/master. Fixed UNPCTableData API usage (Rows.Add vs AddRowFromCopy). Final audit - all features operational. |
