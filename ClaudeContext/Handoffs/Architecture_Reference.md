# Architecture Reference

**Consolidated:** 2026-01-28
**Plugin Version:** v7.8.0
**Status:** Complete reference for plugin architecture, generators, validation, and automation coverage

This document consolidates architecture documentation, generator implementation patterns, fail-fast system, pipeline validation, and coverage analysis.

---

## Table of Contents

1. [Generation Report System (v4.7)](#1-generation-report-system-v47)
2. [Generator Implementation Reference](#2-generator-implementation-reference)
3. [Table Editor 3-Way Sync System (v4.12)](#3-table-editor-3-way-sync-system-v412)
4. [Automation Coverage Analysis](#4-automation-coverage-analysis)
5. [Historical Automation Status](#5-historical-automation-status)
6. [Known TODOs & Placeholders](#6-known-todos--placeholders)
7. [Future: Design Compiler & Spec DataAssets](#7-future-design-compiler--spec-dataassets)
8. [Narrative Pro Integration Strategy](#8-narrative-pro-integration-strategy)
9. [Development Pipeline](#9-development-pipeline)
10. [Fail-Fast System (v4.23+)](#10-fail-fast-system-v423)
11. [Pipeline Gap Analysis (v7.0)](#11-pipeline-gap-analysis-v70)
12. [Function Resolver Parity (v4.31+)](#12-function-resolver-parity-v431)
13. [Generator Pattern Fixes (v4.31-v4.32.3)](#13-generator-pattern-fixes-v431-v4323)

---

## 1. Generation Report System (v4.7)

Machine-readable generation reports for CI/CD integration and debugging. Reports are saved as both UDataAsset (for in-editor inspection) and JSON (for external tooling).

### Key Files

| File | Purpose |
|------|---------|
| `GasAbilityGeneratorReport.h` | Report structs and helper class |
| `GasAbilityGeneratorReport.cpp` | Report implementation |
| `GasAbilityGeneratorCommandlet.cpp` | Report creation hooks (commandlet) |
| `GasAbilityGeneratorWindow.cpp` | Report creation hooks (editor UI v4.12) |
| `GasAbilityGeneratorTypes.h` | FGenerationResult with AssetPath/GeneratorId |

### Report Structure

**FGenerationReportItem** - Each generated asset produces one report item:

```cpp
struct FGenerationReportItem
{
    FString AssetPath;          // "/Game/FatherCompanion/Abilities/GA_FatherAttack"
    FString AssetName;          // "GA_FatherAttack"
    FString GeneratorId;        // "GameplayAbility"
    FString ExecutedStatus;     // "New", "Skipped", "Failed", "Deferred"
    FString PlannedStatus;      // Dry-run: "WillCreate", "WillModify", "WillSkip", "Conflicted"
    bool bHasPlannedStatus;     // True for dry-run reports
    bool bAssetExistedBeforeRun;
    FString Reason;             // Success/failure message
    TArray<FGenerationError> Errors;
    TArray<FString> Warnings;
};
```

**FGenerationError** - Structured error for programmatic handling:

```cpp
struct FGenerationError
{
    FString ErrorCode;      // "E_PARENT_CLASS_NOT_FOUND"
    FString ContextPath;    // "Abilities/GA_FatherAttack/parent_class"
    FString Message;        // Human-readable description
    FString SuggestedFix;   // Actionable fix suggestion
};
```

**UGenerationReport** - Full report as UDataAsset:

```cpp
class UGenerationReport : public UDataAsset
{
    FGuid RunId;
    FDateTime Timestamp;
    FString GeneratorVersion;  // "4.8.0"
    FString ManifestFilePath;
    int64 ManifestHash;
    bool bIsDryRun;
    bool bIsForceRun;
    TArray<FGenerationReportItem> Items;

    // Summary counts
    int32 CountNew, CountSkipped, CountFailed, CountDeferred;
    int32 CountWillCreate, CountWillModify, CountWillSkip, CountConflicted;
};
```

### Output Locations

| Type | Path |
|------|------|
| UDataAsset | `/Game/Generated/Reports/GR_YYYYMMDD_HHMMSS_GUID8` |
| JSON | `Saved/GasAbilityGenerator/Reports/GR_YYYYMMDD_HHMMSS_GUID8.json` |
| Dry-run | Prefix: `GR_DryRun_...` |

### Generator IDs

| GeneratorId | Asset Type |
|-------------|------------|
| Enumeration | E_ enumerations |
| InputAction | IA_ input actions |
| InputMappingContext | IMC_ input mappings |
| GameplayEffect | GE_ gameplay effects |
| GameplayAbility | GA_ gameplay abilities |
| ActorBlueprint | BP_ actor blueprints |
| WidgetBlueprint | WBP_ widget blueprints |
| Blackboard | BB_ blackboards |
| BehaviorTree | BT_ behavior trees |
| Material | M_ materials |
| MaterialFunction | MF_ material functions |
| MaterialInstance | MIC_ material instances (v4.9) |
| FloatCurve | FC_ float curves |
| AnimationMontage | AM_ animation montages |
| AnimationNotify | NAS_ animation notifies |
| DialogueBlueprint | DBP_ dialogue blueprints |
| EquippableItem | EI_ equippable items |
| Activity | BPA_ activities |
| AbilityConfiguration | AC_ ability configs |
| ActivityConfiguration | AC_ activity configs |
| ItemCollection | IC_ item collections |
| NarrativeEvent | NE_ narrative events |
| GameplayCue | GC_ gameplay cues |
| NPCDefinition | NPC_ NPC definitions |
| CharacterDefinition | CD_ character definitions |
| CharacterAppearance | Appearance_ appearances |
| TriggerSet | TS_ trigger sets |
| TaggedDialogueSet | TDS_ tagged dialogue |
| NiagaraSystem | NS_ niagara systems |
| ActivitySchedule | Schedule_ schedules |
| GoalItem | Goal_ goals |
| Quest | Quest_ quests |
| POIPlacement | Level actor POI |
| NPCSpawnerPlacement | Level actor spawner |
| Pipeline | Mesh-to-Item pipeline (v4.12) |

### JSON Report Example

```json
{
    "runId": "6069154F4974CAF380CA139A5941F34D",
    "timestamp": "2026-01-17T14:03:32.615Z",
    "generatorVersion": "4.8.0",
    "manifestFilePath": "C:/Unreal Projects/NP22B57/.../manifest.yaml",
    "manifestHash": 2971715670,
    "isDryRun": false,
    "isForceRun": false,
    "countNew": 155,
    "countSkipped": 0,
    "countFailed": 0,
    "countDeferred": 0,
    "items": [
        {
            "assetPath": "/Game/FatherCompanion/Enums/E_FatherForm",
            "assetName": "E_FatherForm",
            "generatorId": "Enumeration",
            "executedStatus": "New",
            "reason": "Created successfully",
            "errors": [],
            "warnings": []
        }
    ]
}
```

### Implementation Pattern

Each generator populates AssetPath and GeneratorId before returning:

```cpp
FGenerationResult Result;
// ... generation logic ...

Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
Result.AssetPath = AssetPath;  // Full content browser path
Result.GeneratorId = TEXT("GameplayAbility");
Result.DetermineCategory();
return Result;
```

For level actors (no content browser path):

```cpp
Result.AssetPath = FString::Printf(TEXT("LevelActor/POI/%s"), *Definition.POITag);
Result.GeneratorId = TEXT("POIPlacement");
```

### CI/CD Integration

Parse the JSON report to:
1. Verify expected assets were generated (`countNew`)
2. Detect failures (`countFailed`, `items[].errors`)
3. Identify conflicts requiring manual resolution (`countConflicted`)
4. Track manifest changes via `manifestHash`
5. Correlate runs via `runId`

---

## 2. Generator Implementation Reference

All generators are IMPLEMENTED and operational in GasAbilityGenerator v4.8.

### Automation Coverage Summary

| Asset Type | Level | Key Features |
|------------|-------|--------------|
| Quest_ | 95% | State machine, tasks, branches, rewards |
| DBP_ | 98% | Full dialogue tree, events, conditions |
| Goal_ | 95% | Blueprint creation, tags, lifecycle |
| Schedule_ | 95% | Time-based behaviors with goals |
| NPCDef_ | 98% | Full package with auto-create |
| EI_ | 98% | Weapons, armor, all properties |
| WBP_ | 95% | Widget tree, panels, slots (v4.3) |
| POI/Spawner | 90% | Level actor placement |

### 2.1 Quest System

**Generator:** `FQuestGenerator` in GasAbilityGeneratorGenerators.cpp

**Manifest Structure:**
```yaml
quests:
  - name: Quest_ForgeSupplies
    quest_name: "Forge Supplies"
    questgiver: NPCDef_Blacksmith
    states:
      - id: Start
        type: regular
      - id: Complete
        type: success
    branches:
      - id: AcceptQuest
        from_state: Start
        to_state: Gathering
        tasks:
          - task_class: BPT_FinishDialogue
            properties:
              dialogue: DBP_BlacksmithQuest
    rewards:
      currency: 100
      xp: 50
      items: [EI_IronSword]
```

**Key Classes:**
- `UQuestBlueprint` - Quest asset type
- `UQuestState` - State nodes (Regular, Success, Failure)
- `UQuestBranch` - Transitions between states
- `UNarrativeTask` (BPT_*) - Task instances

**Supported Tasks:** BPT_FindItem, BPT_FinishDialogue, BPT_Move, BPT_KillEnemy, BPT_InteractWithObject, BPT_WaitGameplayEvent

### 2.2 Dialogue System

**Generator:** `FDialogueBlueprintGenerator` in GasAbilityGeneratorGenerators.cpp

**Manifest Structure:**
```yaml
dialogue_blueprints:
  - name: DBP_Blacksmith
    dialogue_tree:
      root: greeting
      nodes:
        - id: greeting
          type: npc
          speaker: NPCDef_Blacksmith
          text: "Welcome!"
          player_replies: [ask_work, leave]
        - id: ask_work
          type: player
          text: "Do you have work?"
          option_text: "Ask about work"
          start_quest: Quest_ForgeSupplies
          npc_replies: [work_response]
```

**Key Features:**
- Two-pass node creation and wiring
- NPC/Player node types with full properties
- Events (NE_*) and Conditions (NC_*)
- Quest shortcuts: start_quest, complete_quest_branch, fail_quest
- 12+ CDO properties (FreeMovement, Priority, etc.)

**CSV Pipeline (v4.0):** `FDialogueCSVParser` supports batch dialogue creation from spreadsheets. Columns: Dialogue, NodeID, Type, Speaker, Text, OptionText, Replies, Conditions, Events

### 2.3 Goal & Schedule System

**Goal Generator:** `FGoalItemGenerator` creates Goal_ Blueprint assets (UNPCGoalItem)

```yaml
goal_items:
  - name: Goal_DefendForge
    default_score: 75.0
    goal_lifetime: -1.0
    owned_tags: [State.Defending]
    block_tags: [State.Fleeing]
```

**Schedule Generator:** `FActivityScheduleGenerator` creates Schedule_ DataAssets (UNPCActivitySchedule)

```yaml
activity_schedules:
  - name: Schedule_BlacksmithDay
    behaviors:
      - start_time: 600      # 6:00 AM
        end_time: 1800       # 6:00 PM
        goal_class: Goal_Work
        location: Forge
```

**Time Format:** 0-2400 where 100 = 1 hour (e.g., 600 = 6:00 AM, 1800 = 6:00 PM)

**Helper Class:** `UScheduledBehavior_AddNPCGoalByClass` - Concrete helper for goal-based scheduling

### 2.4 NPC Package System

**Generator:** `FNPCDefinitionGenerator` with auto-create support

**Manifest Structure:**
```yaml
npc_definitions:
  - name: NPCDef_Blacksmith
    npc_id: Blacksmith_01
    npc_name: Garrett the Blacksmith
    auto_create_dialogue: true
    auto_create_tagged_dialogue: true
    auto_create_item_loadout: true
    default_item_loadout:
      - items:
          - item: EI_Hammer
            quantity: 1
        item_collections: [IC_BlacksmithTools]
    trading_item_loadout:
      - item_collections: [IC_WeaponsForSale]
```

**Auto-Create Features:**
- `auto_create_dialogue` → DBP_{NPCName}Dialogue
- `auto_create_tagged_dialogue` → {NPCName}_TaggedDialogue
- `auto_create_item_loadout` → Populates DefaultItemLoadout

**Loot Table Roll Support:** Full `TArray<FLootTableRoll>` population with items, quantities, collections, chance, num_rolls.

### 2.5 Item Pipeline

**Generator:** `FEquippableItemGenerator` with weapon property chains

**Pipeline:** `FPipelineProcessor` converts mesh files to EI_ assets:
1. Scan mesh folder for SM_/SK_ assets
2. Create EI_ DataAsset per mesh
3. Populate collections (IC_)
4. Link to NPC loadouts

**Weapon Support:**
- MeleeWeaponItem properties
- RangedWeaponItem properties (spread, recoil, etc.)
- Weapon attachments (holster_attachments, wield_attachments)

### 2.6 POI & Spawner Placement

**Generators:**
- `FPOIPlacementGenerator` - Places APOIActor in levels
- `FNPCSpawnerPlacementGenerator` - Places ANPCSpawner actors

**Manifest Structure:**
```yaml
poi_placements:
  - poi_tag: POI.Town.Forge
    location: "100, 200, 0"
    display_name: "Blacksmith Forge"
    linked_pois: [POI.Town.Square]

npc_spawner_placements:
  - name: Spawner_Blacksmith
    near_poi: POI.Town.Forge
    npcs:
      - npc_definition: NPCDef_Blacksmith
```

**Commandlet Flag:** `-level="/Game/Maps/MainWorld"` loads world for actor placement

### 2.7 Widget Blueprint (v4.3)

**Generator:** `FWidgetBlueprintGenerator` with widget tree construction

**Manifest Structure:**
```yaml
widget_blueprints:
  - name: WBP_UltimatePanel
    widget_tree:
      root_widget: RootCanvas
      widgets:
        - id: RootCanvas
          type: CanvasPanel
          children: [ContentBox]
        - id: ContentBox
          type: VerticalBox
          is_variable: true
          slot:
            anchors: BottomCenter
            position: "0, -50"
          children: [Label, Bar]
        - id: Label
          type: TextBlock
          text: "ULTIMATE"
          slot:
            h_align: Center
        - id: Bar
          type: ProgressBar
          is_variable: true
          slot:
            size_rule: Fill
            padding: "10, 5, 10, 5"
```

**Supported Widget Types (25+):** CanvasPanel, VerticalBox, HorizontalBox, Overlay, Border, Button, TextBlock, RichTextBlock, Image, ProgressBar, Slider, CheckBox, EditableText, EditableTextBox, ComboBox, Spacer, SizeBox, ScaleBox, ScrollBox, UniformGridPanel, GridPanel, WidgetSwitcher, Throbber, CircularThrobber, NativeWidgetHost

**Slot Properties:**
- Canvas: anchors, position, size, alignment, auto_size
- Box: h_align, v_align, size_rule, fill_weight, padding

### 2.8 Faction System

**Status:** No generator needed - Factions use FGameplayTag with runtime relationships.

**Storage:**
- Tags defined in DefaultGameplayTags.ini
- Relationships in `ANarrativeGameState::FactionAllianceMap`
- Assigned via NPCDefinition.DefaultFactions

**Existing Support:**
- Tag generator handles Narrative.Factions.* tags
- NPC generator populates DefaultFactions array

### 2.9 Asset Existence Checking Patterns (v4.12.4)

**Problem:** `FPackageName::DoesPackageExist()` and Asset Registry queries rely on cached data that doesn't update when files are deleted externally (e.g., via file explorer or `rm -rf`). This caused generators to incorrectly skip creation of assets that didn't exist on disk.

**Two-Tier Approach:**

| Use Case | Method | When |
|----------|--------|------|
| **Listing/Discovery** | Asset Registry + forced rescan | Finding existing assets, populating dropdowns, sync operations |
| **Create/Skip Decisions** | Disk-truth via IFileManager | Determining whether to generate an asset |

**Canonical Helper:**

```cpp
// v4.12.4: DISK-TRUTH CHECK - Do NOT replace with AssetRegistry or DoesPackageExist!
// Canonical helper for create/skip decisions. Requires valid long package name (not object path).
// Contract: Caller must pass package path like "/Game/Folder/AssetName", not "/Game/Folder/AssetName.AssetName"
bool FGeneratorBase::DoesAssetExistOnDisk(const FString& AssetPath)
{
    // Normalize: ensure leading slash
    FString PackageName = AssetPath;
    if (!PackageName.StartsWith(TEXT("/")))
    {
        PackageName = TEXT("/") + PackageName;
    }

    // Convert to filesystem path and check actual file existence
    FString FilePath;
    if (FPackageName::TryConvertLongPackageNameToFilename(PackageName, FilePath,
        FPackageName::GetAssetPackageExtension()))
    {
        return IFileManager::Get().FileExists(*FilePath);
    }

    // Conversion failed = malformed package path = cannot exist on disk
    return false;
}
```

**Key Rules:**

1. **Create/Skip must use disk-truth:** All generators call `DoesAssetExistOnDisk()` before deciding to skip
2. **Listing uses registry with rescan:** Sync operations call `ScanPathsSynchronous(paths, true /*bForceRescan*/)` before `GetAssetsByClass()`
3. **Fail-fast for malformed paths:** If path conversion fails, return false immediately (don't attempt registry fallback)
4. **Package path format:** `/Game/Folder/AssetName` (not object path `/Game/Folder/AssetName.AssetName`)

**Fixed Locations:**
- `GasAbilityGeneratorGenerators.cpp:DoesAssetExistOnDisk()` - Core helper
- `GasAbilityGeneratorWindow.cpp:CheckNPCAssetStatus()` - NPC status display

### 2.10 Decision Path Golden Rule (v4.12.5)

**LOCKED:** Decision paths (create/skip/conflict determination) must use registry-aware APIs:
- `GetMetadataEx()` - returns `FGeneratorMetadata` struct
- `HasMetadataEx()` - returns bool
- `TryGetMetadata()` - single lookup with explicit success/failure (v4.12.5)

**NEVER** use `GetMetadata()` or `GetAssetMetadata()` directly in decision paths. These only check `UAssetUserData` and miss registry-stored metadata for `UDataAsset`/`UBlueprint`/`UNiagaraSystem` assets.

**Rationale:** `UDataAsset` and `UBlueprint` don't implement `IInterface_AssetUserData` in UE5.7, so metadata for these asset types is stored in a central registry (`UGeneratorMetadataRegistry`). Decision paths must check both sources to avoid incorrectly classifying generated assets as "manual."

**Fixed Locations (v4.12.5):**
- `GasAbilityGeneratorGenerators.cpp:CheckExistsWithMetadata()` - Main decision path
- `GasAbilityGeneratorWindow.cpp:CheckNPCAssetStatus()` - UI status display

**See:** `v4.12.5_Registry_Aware_Metadata_Fix.md` for full implementation details.

### 2.11 LOCKED Tier Architecture (v4.12.5)

The plugin uses a 3-tier architecture for code governance:

| Tier | Purpose | Enforcement |
|------|---------|-------------|
| **LOCKED** | Data-safety and CI-safety invariants | Git hook guard, explicit approval required |
| **EVOLVABLE** | Generators, parsers, validators | Normal development flow |
| **COSMETIC** | UI, display logic | Minimal constraints |

**Key Documents:**
- `LOCKED_CONTRACTS.md` - 9 invariants that must remain true through any refactor
- `LOCKED_SYSTEMS.md` - Maps each contract to implementation files/functions

**Enforcement Mechanism:**
- Git hook guard (`Tools/locked_guard.py`) blocks commits touching locked files
- Bypass requires explicit `[LOCKED-CHANGE-APPROVED]` token in commit message
- Run `Tools/install_git_hooks.ps1` once after cloning to enable

**Locked Files (v4.12.5):**
- Contract docs: `ClaudeContext/Handoffs/LOCKED_*.md`
- Metadata: `Locked/GasAbilityGeneratorMetadata.h/cpp`
- Types: `Locked/GasAbilityGeneratorTypes.h`
- Generators (temporary): `GasAbilityGeneratorGenerators.cpp/h`
- Sync engines: `XLSXSupport/*`

**Rationale:** Separates "must never break" invariants from "can evolve" implementations. Prevents accidental regression of safety-critical code paths during UI or capability changes.

#### Implementation Status (v4.12.5)

**Original Plan:**
- Create three tier folders: `Locked/`, `Evolvable/`, `Cosmetic/`
- Move ALL files to appropriate tiers
- Phase A: Move files + update includes
- Phase B: Include hygiene cleanup

**Actual Implementation (Minimal Approach):**
- Only created `Locked/` folder
- Only moved 3 core files: `GasAbilityGeneratorMetadata.h/cpp`, `GasAbilityGeneratorTypes.h`
- Phase A completed, Phase B deferred

**Deviation Rationale:**
- `GasAbilityGeneratorGenerators.h/cpp` has 9+ dependents - moving causes excessive include churn
- `XLSXSupport/` already correctly grouped as a unit
- UI files have mixed concerns (locked sync logic + evolvable display)
- Minimal approach provides enforcement with less risk

**Files NOT Moved (kept in place):**

| File | Reason |
|------|--------|
| `GasAbilityGeneratorGenerators.h/cpp` | Mixed content (locked invariants + evolvable generators), many dependents |
| `XLSXSupport/*` | Already grouped, sync engines are locked within folder |
| `GasAbilityGeneratorCommandlet.h/cpp` | Mixed (locked contracts + evolvable CLI features) |
| `*TableEditor*`, `*Window*` | Mixed UI + logic, low benefit to move |

#### Future Considerations

**Phase B: Include Hygiene (Deferred)**
- Normalize include paths for consistency
- Remove transitive includes (headers already pulled in by other headers)
- Update `Build.cs` with `PublicIncludePaths` if needed
- Only necessary if compile times degrade or include errors appear

**Full Restructure (Optional Future Work)**
1. Create `Evolvable/` folder - move parsers, validators, converters
2. Create `Cosmetic/` folder - move window, table editor UI components
3. Split `GasAbilityGeneratorGenerators.cpp` monolith:
   - Extract locked core (IsForceMode, IsDryRunMode, DoesAssetExistOnDisk) to `Locked/GeneratorCore.cpp`
   - Keep individual generators in `Evolvable/Generators/`
4. Update all include paths
5. Complete Phase B hygiene

**Triggers for Full Restructure:**
- Generator count exceeds 40+ (monolith becomes unwieldy)
- Multiple developers working simultaneously on different tiers
- Need to enforce stricter code review boundaries

**Enforcement Limitations:**
- Current git hook is client-side only (honor system)
- Stronger enforcement options: CODEOWNERS, branch protection, server-side hooks, CI hash gate
- Current approach provides audit trail and psychological friction, not technical prevention

---

## 3. Table Editor 3-Way Sync System (v4.12)

All four table editors (NPC, Dialogue, Quest, Item) implement a 3-way merge system for Excel ↔ UE synchronization.

### Architecture

```
Export creates base snapshot with per-row hashes
     ↓
User edits in Excel (may add/delete/modify rows)
     ↓
Sync compares: Base (from export) vs UE (current) vs Excel (imported)
     ↓
Detects conflicts, presents resolution UI
     ↓
Applies merged changes
```

### Key Files

| File | Purpose |
|------|---------|
| `XLSXSupport/NPCXLSXSyncEngine.h/cpp` | NPC 3-way sync engine |
| `XLSXSupport/DialogueXLSXSyncEngine.h/cpp` | Dialogue 3-way sync engine |
| `XLSXSupport/QuestXLSXSyncEngine.h/cpp` | Quest 3-way sync engine (v4.12) |
| `XLSXSupport/ItemXLSXSyncEngine.h/cpp` | Item 3-way sync engine (v4.12) |
| `XLSXSupport/SNPCXLSXSyncDialog.h/cpp` | NPC sync approval dialog |
| `XLSXSupport/SDialogueXLSXSyncDialog.h/cpp` | Dialogue sync approval dialog |
| `XLSXSupport/SQuestXLSXSyncDialog.h/cpp` | Quest sync approval dialog (v4.12) |
| `XLSXSupport/SItemXLSXSyncDialog.h/cpp` | Item sync approval dialog (v4.12) |

### v4.11 Rule: Only Unchanged Auto-Resolves

Per Table_Editors_Reference.md v4.11:
- **Case 1 (Unchanged)**: Base == UE == Excel → Auto-resolves to KeepUE
- **All 13 other cases**: Require explicit user approval in full-screen window

### Sync Status Enum

```cpp
enum class E*SyncStatus : uint8
{
    Unchanged,          // Base == UE == Excel (no action needed)
    ModifiedInUE,       // Base != UE, Base == Excel (keep UE changes)
    ModifiedInExcel,    // Base == UE, Base != Excel (apply Excel changes)
    Conflict,           // Base != UE && Base != Excel (user must resolve)
    AddedInUE,          // Not in Base, exists in UE only
    AddedInExcel,       // Not in Base, exists in Excel only
    DeletedInUE,        // In Base & Excel, missing in UE
    DeletedInExcel,     // In Base & UE, explicitly deleted in Excel
    DeleteConflict      // Deleted in one source, modified in other
};
```

### Approval Dialog UI Features

- **Window Size**: 80% of screen work area (minimum 800×600)
- **Radio Selection**: ○/● for value selection
- **Row Highlighting**: Yellow = unresolved, Green = resolved
- **Action Column**: REMOVE option only for cases 6-14 (where deletion possible)
- **Apply Button**: Only enabled when all entries resolved

### Usage Pattern

```cpp
// In OnSyncXLSXClicked:
FQuestSyncResult SyncResult = FQuestXLSXSyncEngine::CompareSources(BaseRows, UERows, ExcelRows);
FQuestXLSXSyncEngine::AutoResolveNonConflicts(SyncResult);

if (SyncResult.HasChanges())
{
    if (SQuestXLSXSyncDialog::ShowModal(SyncResult))
    {
        FQuestMergeResult Merged = FQuestXLSXSyncEngine::ApplySync(SyncResult);
        // Apply Merged.MergedRows to TableData
    }
}
```

---

## 4. Automation Coverage Analysis

### Coverage Statistics (v4.8)

| Asset Type | Header Properties | Manifest Fields | Generator Coverage | Gap % |
|------------|------------------|-----------------|-------------------|-------|
| UNarrativeGameplayAbility | 2 | 2 | 100% | **0%** |
| UNPCDefinition | 16 | 16 | 100% | **0%** |
| UCharacterDefinition | 8 | 8 | 100% | **0%** |
| UCharacterAppearance | 3 | 3 | 100% | **0%** |
| UEquippableItem | 7 | 7 | 100% | **0%** |
| UWeaponItem | 18 | 18 | 100% | **0%** |
| URangedWeaponItem | 14 | 14 | 100% | **0%** |
| UNarrativeItem (Base) | 20 | 20 | 100% | **0%** |
| UDialogue | 17 | 17 | 100% | **0%** |
| UNPCActivityConfiguration | 3 | 3 | 100% | **0%** |
| UNPCGoalItem | 8 | 8 | 100% | **0%** |
| UNarrativeActivityBase | 4 | 4 | 100% | **0%** |
| UAbilityConfiguration | 3 | 3 | 100% | **0%** |

**Overall Coverage: ~99%**

### Remaining Gaps (MINIMAL)

**Asset Types Not Generated:**

| Asset Type | Reason |
|------------|--------|
| UTriggerSet | Complex event system with editor-only visual scripting |

**Properties with Partial Support:**

| Property | Class | Status |
|----------|-------|--------|
| Meshes (FMeshAppearanceData) | UCharacterAppearance | Basic mesh path supported; complex struct properties (materials, morphs, transforms) logged for editor |

These remaining gaps represent edge cases requiring editor visual tools.

---

## 5. Historical Automation Status

All features IMPLEMENTED in GasAbilityGenerator v4.8.

### v3.10 - RangedWeaponItem Enhancements

| Feature | Status |
|---------|--------|
| RangedWeaponItem spread properties | COMPLETE |
| Crosshair widget | COMPLETE |
| Aim render properties | COMPLETE |
| Recoil vectors | COMPLETE |
| Weapon attachments | COMPLETE |
| Clothing mesh materials | COMPLETE |

### v4.0 - Generator Enhancements

| Feature | Status |
|---------|--------|
| Gameplay Cue Generator (GC_) | COMPLETE |
| Narrative Event NPC Targets | COMPLETE |
| Dialogue Speaker Automation | COMPLETE |
| Float Curve Enhancement | COMPLETE |
| Material Function Wiring | COMPLETE |
| Blackboard Key Types | COMPLETE |

### v4.0 - Automation Gaps Closed

| Feature | Status |
|---------|--------|
| Quest Rewards & Questgiver | COMPLETE |
| NPC TradingItemLoadout | COMPLETE |
| Dialogue Quest Shortcuts | COMPLETE |
| Item Usage Properties | COMPLETE |
| Weapon Attachments | COMPLETE |

### Automation Coverage by Asset Type

| Asset Type | Automation Level | Key Properties |
|------------|------------------|----------------|
| Tags | 100% | Full generation to INI |
| Input Actions/Mappings | 100% | Full with value type |
| Enumerations | 100% | Full with display names |
| Gameplay Effects | 100% | Full Blueprint with modifiers |
| Gameplay Abilities | 98% | Full with event graphs, tags, policies |
| Gameplay Cues | 75% | Burst/BurstLatent/Actor types |
| Actor Blueprints | 95% | Event graphs, function overrides |
| Widget Blueprints | 95% | Widget tree layout, variables |
| Dialogue Blueprints | 98% | Full tree, events, conditions |
| Quest Blueprints | 95% | State machine, tasks, rewards |
| Behavior Trees | 95% | Nodes, decorators, services |
| Blackboards | 100% | All key types |
| Materials | 85% | Expressions, parameters |
| Material Functions | 85% | Expressions, connections |
| Float Curves | 100% | Keys, interpolation |
| Animation Montages | 90% | Sections, notifies |
| Equippable Items | 98% | Full weapon support |
| Activities | 95% | Tags, goal types |
| Activity Schedules | 95% | Time-based behaviors |
| Goals | 95% | Score, tags, lifecycle |
| NPC Definitions | 98% | Full with auto-create |
| Character Definitions | 95% | Full property support |
| Narrative Events | 90% | Runtime, filter, targets |
| Niagara Systems | 70% | Template duplication |
| Item Collections | 100% | Items with quantities |
| Tagged Dialogue Sets | 85% | Dialogue arrays |
| POI Placements | 90% | Level actor placement |
| NPC Spawner Placements | 90% | Level actor placement |

### Key Technical Implementations

- **TSubclassOf Resolution:** AbilityConfiguration, ActivityConfiguration now properly resolve class references via LoadClass<>
- **Loot Table Roll Population:** Full FLootTableRoll struct support for NPCDefinition item loadouts via reflection
- **Quest State Machine:** Complete UQuestState, UQuestBranch, UNarrativeTask creation without graph nodes
- **Dialogue Tree Generation:** Full node graph with NPC/Player nodes, events, conditions, quest shortcuts
- **Widget Tree Construction:** Three-pass system: create widgets, build hierarchy, configure slots/properties

---

## 6. Known TODOs & Placeholders

**See:** [`TODO_Tracking.md`](TODO_Tracking.md) for the consolidated TODO list.

All TODOs from this section have been moved to the consolidated tracking file as of v4.12.4.

### Summary (v4.12.4)
- **Completed:** 8 HIGH/MEDIUM priority tasks (NPC asset creation, DEBUG code removal, token string building, etc.)
- **Pending:** 4 MEDIUM priority refactoring tasks
- **Low Priority:** Performance testing and test framework implementation

---

## 7. Future: Design Compiler & Spec DataAssets

Two complementary approaches for improved authoring workflow (deferred for future implementation):

### 7.1 Design Compiler Architecture

**Purpose:** Transform high-level, human-friendly game design specs into the flat manifest.yaml consumed by GasAbilityGenerator.

**Pipeline:**
```
DesignSpec/                    ┌─────────────────┐
├── globals/                   │    Compiler     │
├── items/                 ──► │  (resolve refs, │ ──► manifest.yaml ──► Generators
├── abilities/                 │   templates)    │
├── npcs/                      └─────────────────┘
└── quests/
```

**Features:**
- **Include System** - Split manifest across multiple files
- **Templates & Inheritance** - Base templates with override support
- **Variables & Constants** - $CONSTANT substitution
- **Conditional Blocks** - Build variants (demo vs full)
- **Pre-Generation Validation** - Reference checking, localization completeness

**Implementation Options:**
1. Python Compiler (~500-800 lines) - Recommended
2. C++ Compiler (in plugin) - More complex
3. Lightweight Include Only (~50 lines) - Good intermediate step

### 7.2 Spec DataAsset Workflow

**Purpose:** Replace YAML-centric workflow with native UE DataAssets + Content Browser right-click generation.

**Benefits:**
- Zero UI development (uses Details panel)
- Free dropdowns (TSubclassOf asset pickers)
- Free validation (UPROPERTY meta tags)
- Free undo/redo (FScopedTransaction)
- Batch support via commandlet

**Spec Types:**

| Spec | Generates |
|------|-----------|
| UNPCPackageSpec | NPCDef_, AC_, ActConfig_, Goal_, Schedule_, DBP_, Quest_ |
| UQuestSpec | Quest_ |
| UItemSpec | EI_ |

**Template Inheritance:**
```
Specs/Templates/
└── NPC_Template_Merchant.uasset    ← Base template
    bIsVendor = true
    Schedule = [Work 6-18]

Specs/NPCs/
└── NPC_Blacksmith_Spec.uasset      ← Inherits
    ParentTemplate = NPC_Template_Merchant
    BaseName = "Blacksmith"          ← Override
```

**Context Menu Actions:**
- **Generate NPC Assets** - Creates all defined assets
- **Validate Spec** - Check for errors without generating
- **Preview Generation Plan** - Show what would be created

### Implementation Status

| Feature | Status | Priority |
|---------|--------|----------|
| Design Compiler | Deferred | Low |
| Spec DataAssets | Deferred | Low |
| Include System Only | Ready | Medium |

**Prerequisites:**
1. Complete core Narrative Pro generators
2. Manifest exceeds ~2000 lines
3. Multiple designers need simultaneous editing

---

## 8. Narrative Pro Integration Strategy

### Philosophy: Work WITH Narrative Pro

Instead of generating everything from scratch:
1. **Catalog** what Narrative Pro already provides as ready-made assets
2. **Reference** NP assets directly when they fit our needs
3. **Generate** only project-specific assets that fill gaps

### Narrative Pro Built-in Assets (REFERENCE, don't generate)

| Type | Count | Examples | Usage |
|------|-------|----------|-------|
| **GA_** | 46+ | `GA_Melee_Unarmed`, `GA_Attack_Magic_Proj`, `GA_Sprint` | **Subclass** for Father abilities |
| **GE_** | 41+ | `GE_Damage_SetByCaller`, `GE_EquipmentModifier` | **Reference** or copy patterns |
| **BT_** | 15 | `BT_FollowCharacter`, `BT_Attack_Melee`, `BT_Patrol` | **Reference** for Father AI |
| **BB_** | 11 | `BB_FollowCharacter`, `BB_Attack`, `BB_Idle` | **Reference** directly |
| **BPA_** | 17 | `BPA_FollowCharacter`, `BPA_Attack_Melee`, `BPA_Flee` | **Subclass** for Father activities |
| **AC_** | 15 | `AC_NPC_Default`, `AC_Melee`, `AC_Marksman` | **Reference** or customize |
| **Goal_** | 8 | `Goal_Attack`, `Goal_FollowCharacter`, `Goal_Idle` | **Reference** directly |
| **GoalGenerator_** | 4 | `GoalGenerator_Attack`, `GoalGenerator_Flee` | **Reference** directly |
| **IA_** | 28+ | `IA_Attack`, `IA_Move`, `IA_Ability1` | **Use directly** |
| **BTS_** | 16 | `BTS_SetAIFocus`, `BTS_Attack` | **Reference** directly |
| **WBP_** | 50+ | Base widgets, HUD elements, inventory UI | **Subclass** for custom UI |

**NP Asset Locations:**
- Abilities: `/NarrativePro22B57/Content/Pro/Core/Abilities/`
- AI System: `/NarrativePro22B57/Content/Pro/Core/AI/`
- Input: `/NarrativePro22B57/Content/Pro/Core/Data/Input/`
- UI: `/NarrativePro22B57/Content/Pro/Core/UI/`

### Gaps Our Generator Fills (GENERATE these)

| Type | What to Generate | Why |
|------|------------------|-----|
| **NPCDef_** | `NPCDef_Father`, enemy definitions | NP has none - project specific |
| **CD_** | Character definitions | Project-specific characters |
| **ActConfig_** | `ActConfig_Father` | NP has none - project specific |
| **Custom GA_** | `GA_FatherAttack`, `GA_FatherLaserShot` | Father-specific (subclass NP's GA_) |
| **Custom GE_** | `GE_FatherFormStats`, form effects | Father-specific configurations |
| **Custom BPA_** | `BPA_FatherFollow` | Father-specific (subclass NP's BPA_) |
| **EI_** | `EI_FatherCrawlerForm`, form items | Father form equipment |
| **NE_** | `NE_FatherAwakens` | Story events |
| **DBP_** | `DBP_FatherDialogue` | Father conversations |
| **Tags** | `Father.*`, `Ability.Father.*` | Project gameplay tags |
| **E_** | `E_FatherForm` | Project enumerations |

### Hybrid Approach Example

```yaml
# Reference NP's existing BT directly (no generation needed)
# Don't add BT_FollowCharacter - use NP's directly

# Generate Father-specific ability that EXTENDS NP's base
gameplay_abilities:
  - name: GA_FatherAttack
    parent_class: GA_Melee_Unarmed    # Inherits NP's motion warping!
    folder: Abilities/Father
    # ... Father-specific configuration
```

---

## 9. Development Pipeline

### Claude-Assisted Workflow

| Step | Action | Owner |
|------|--------|-------|
| 1 | Feed Narrative Pro update data | User |
| 2 | Analyze changes, update Technical Reference | Claude |
| 3 | Create/update implementation guides together | User + Claude |
| 4 | Debug and revise guides | User + Claude |
| 5 | Update manifest.yaml with new assets/tags | Claude |
| 6 | Run generator (commandlet or editor) | User |
| 7 | Debug assets, rinse and repeat | User + Claude |

### Manifest as Single Source of Truth

| Question | Answer |
|----------|--------|
| Where are tags defined? | manifest.yaml |
| Where are assets defined? | manifest.yaml |
| What generates assets? | GasAbilityGenerator commandlet or editor UI |
| What validates changes? | Regen/Diff system with metadata hashes |

### Key Design Decisions

| Item | Decision |
|------|----------|
| Overwrite behavior | Skip existing, use metadata to detect changes |
| Conflict handling | CONFLICT status requires `--force` flag |
| State persistence | Metadata stored on assets + registry |
| Folder path | Remembered via editor config |

---

## 10. Fail-Fast System (v4.23+)

The fail-fast system ensures assets fail loudly at generation time rather than silently producing broken assets.

### R1/R2/R3 Classification Framework (LOCKED)

| Class | Condition | Action |
|-------|-----------|--------|
| **R1** | Manifest explicitly references something → cannot resolve | **HARD FAIL** |
| **R2** | Manifest omits optional field → documented default | ALLOWED (no warn) |
| **R3** | Structural invalidity (quests, graphs, etc.) | **HARD FAIL** |

**Key Separator:** "Manifest referenced?" YES/NO

### Fallback Classification

| Type | Description | Action |
|------|-------------|--------|
| **Type M (Masking)** | Manifest referenced X → X missing → generation continued | **ELIMINATE** |
| **Type D (Default)** | Manifest omitted optional X → uses default | ALLOWED if documented |

### Failure Point Summary (118 Total)

| Category | Count | Type M | Type D | Scope |
|----------|-------|--------|--------|-------|
| A (INFO Manual Setup) | 12 | 12 | 0 | Pipeline |
| B (WARN Missing Class) | 28 | 28 | 0 | Pipeline |
| C (Reward Class) | 3 | 3 | 0 | Pipeline |
| D (Comment Deferred) | 4 | 4 | 0 | Pipeline |
| S (Secondary Warnings) | 51 | 51 | 0 | Pipeline |
| F (Fallback Patterns) | 12 | 11 | 1 | Pipeline |
| T (Token Registry) | 2 | 2 | 0 | Pipeline |
| W (Window/Editor) | 6 | 0 | 6 | EditorUX |
| **TOTAL** | **118** | **111** | **7** | - |

### Phase 3 Failure Report Format

Every hard fail produces structured diagnostic output:
```
[E_ERROR_CODE] AssetName | Subsystem: X | Human message | ClassPath: /Script/Module.UClass | SuperClassPath: /Script/Module.UParent | RequestedProperty: PropertyName
```

**Subsystems:** GAS, Item, NPC, Dialogue, Quest, BT, Material, Niagara, Token, General

### 8 Proven Generator Bugs

Properties exist in NarrativePro source but `FindPropertyByName()` fails at runtime:

| Property | Header | Declaration |
|----------|--------|-------------|
| PartySpeakerInfo | Dialogue.h:205 | `TArray<FPlayerSpeakerInfo>` |
| EquipmentAbilities | EquippableItem.h:136 | `TArray<TSubclassOf<UNarrativeGameplayAbility>>` |
| Stats | NarrativeItem.h:288 | `TArray<FNarrativeItemStat>` |
| ActivitiesToGrant | NarrativeItem.h:280 | `TArray<TSubclassOf<UNPCActivity>>` |
| PickupMeshData | NarrativeItem.h:509 | `FPickupMeshData` |
| TraceData | RangedWeaponItem.h:86 | `FCombatTraceData` |
| NPCTargets | NarrativeEvent.h:173 | `TArray<TObjectPtr<UNPCDefinition>>` |
| CharacterTargets | NarrativeEvent.h:169 | `TArray<TObjectPtr<UCharacterDefinition>>` |

**Root Cause Hypotheses:** H1 (Wrong UClass), H2 (Module timing), H3 (Name mismatch), H4 (WITH_EDITORONLY_DATA guards), H5 (BP-generated class layout)

---

## 11. Pipeline Gap Analysis (v7.0)

Full audit of manifest → parser → pre-validator → generator → logging pipeline.

### Gap Resolution Summary

| Severity | Count | Fixed | Remaining |
|----------|-------|-------|-----------|
| **CRITICAL** | 6 | 6 | 0 |
| **HIGH** | 14 | 14 | 0 |
| **MEDIUM** | 18 | 18 | 0 |
| **LOW** | 9 | 0 | 9 |

**Generation Result:** 194/194 assets (0 errors, 17 warnings - down from 57 after v7.0 false positive fix)

### Node Creation Validation

| Node Type | Required Input | Status |
|-----------|----------------|--------|
| VariableSet | Value pin | FIXED v4.40 |
| PropertySet | Value pin | FIXED v4.40 |
| Branch | Condition pin | FIXED v4.40 |
| DynamicCast | Object pin | FIXED v4.40 |
| SpawnActor | Class pin | FIXED v4.40.2 |
| Sequence | Exec input | FIXED v4.40.2 |
| Delay | Duration pin | FIXED v4.40.2 |
| GetArrayItem | Array + Index | FIXED v4.40.2 |
| MakeArray | Element pins | FIXED v4.40.2 |

### Pre-Validation Rule Coverage

| Rule ID | Category | Status |
|---------|----------|--------|
| C1-C2 | Class path/parent resolution | COVERED |
| F1-F2 | Function resolution | COVERED |
| A1-A2 | AttributeSet/Attribute | COVERED |
| R1-R3 | Asset references | COVERED |
| T1-T2 | Tags (normal/SetByCaller) | COVERED |
| K1-K2 | Tokens | FIXED v4.40.3 |
| D1-D2 | Delegate existence/signature | FIXED v4.40 |
| N1-N2 | Node inputs/Connections | FIXED v4.40.2/v4.40.3 |

### Key Fixes by Version

| Version | Fixes |
|---------|-------|
| v4.40 | Stale log deletion, metadata mutex, VariableSet validation, delegate binding validation, MakeLiteral* nodes, SafeSavePackage helper |
| v4.40.2 | SpawnActor/Delay/GetArrayItem/MakeArray/Sequence validation, asset registry notification, connection pre-validation |
| v4.40.3 | Token validation, widget/dialogue/quest/BT/NPC pre-validation |
| v7.0 | N1 false positives fixed (variable_name as valid pin), PropertyGet for Blackboard, Cast→VariableSet auto-inference |

---

## 12. Function Resolver Parity (v4.31+)

Ensures PreValidator and Generator use identical function resolution logic.

### Contract (LOCKED)

> "PreValidator function resolution behavior must be identical to Generator function resolution behavior. Both use `FGasAbilityGeneratorFunctionResolver::ResolveFunction()` as single source of truth."

### Architecture

```
┌─────────────────────────────────────────────────────────┐
│     FGasAbilityGeneratorFunctionResolver                │
│  ┌─────────────────────────────────────────────────┐    │
│  │  static WellKnownFunctions (single table)       │    │
│  │  ResolveFunction() - 4-step cascade             │    │
│  │    1. WellKnown probe (variants)                │    │
│  │    2. Explicit class (exact)                    │    │
│  │    3. Parent chain (exact)                      │    │
│  │    4. Library fallback (exact, 13 classes)      │    │
│  └─────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────┘
              │                           │
              ▼                           ▼
┌─────────────────────────┐   ┌─────────────────────────┐
│  Generator              │   │  PreValidator           │
│  (uses ResolveFunction) │   │  (uses ResolveFunction) │
└─────────────────────────┘   └─────────────────────────┘
```

### Resolution Order

1. **WellKnownFunctions probe** - Try variants (Name, K2_Name, BP_Name)
2. **Explicit class** - If `class:` specified, exact name only
3. **Parent chain** - If `target_self` or from parent, exact name only
4. **Library fallback** - 13 classes in order, exact name only

### Library Fallback Classes

UKismetSystemLibrary, UKismetMathLibrary, UGameplayStatics, UGameplayAbility, UAbilitySystemBlueprintLibrary, UAbilitySystemComponent, ACharacter, AActor, APawn, UCharacterMovementComponent, USceneComponent, USkeletalMeshComponent, UPrimitiveComponent

### Key Files

| File | Purpose |
|------|---------|
| `Public/GasAbilityGeneratorFunctionResolver.h` | Resolver interface |
| `Private/GasAbilityGeneratorFunctionResolver.cpp` | Resolution implementation |

---

## 13. Generator Pattern Fixes (v4.31-v4.32.3)

Locked decisions from debugging event graph generation failures.

### Locked Decisions

| ID | Topic | Pattern |
|----|-------|---------|
| D1 | `parameters:` block | Parser converts to properties automatically |
| D2 | Function names | Use `K2_*` prefix (e.g., `K2_ApplyGameplayEffectSpecToOwner`) |
| D3 | TSubclassOf parsing | `type: Class` + `class: ClassName` → `Class:ClassName` |
| D4 | By-ref params | Use `MakeLiteralName` nodes for blackboard KeyName params |
| D5 | Object→Actor cast | Required `DynamicCast` for GetValueAsObject → Actor functions |
| D6 | Double precision | Arithmetic remapped: `Add_FloatFloat` → `Add_DoubleDouble` |
| D7 | VariableGet pins | Use variable name as pin name, not "Value" |
| D8 | Self reference | Use `target_self: true` on CallFunction, not Self node |
| D9 | Array params | Use `MakeArray` for SphereOverlapActors ObjectTypes/ActorsToIgnore |
| D10 | Component cast | Use `DynamicCast` after GetComponentByClass for specific types |
| D11 | Effect search paths | NPC/Enemy subdirectories added for startup_effects |

### Code Changes (v4.32.2-v4.32.3)

**Parser - TSubclassOf Handling** (`GasAbilityGeneratorParser.cpp:2272-2285`):
```cpp
else if (TrimmedLine.StartsWith(TEXT("class:")))
{
    FString ClassValue = GetLineValue(TrimmedLine);
    if (!CurrentVar.Type.IsEmpty())
    {
        CurrentVar.Type = CurrentVar.Type + TEXT(":") + ClassValue;
    }
}
```

**Function Remapping** (`GasAbilityGeneratorGenerators.cpp:10399-10410`):
```cpp
DeprecatedFunctionRemapping.Add(TEXT("Add_FloatFloat"), TEXT("Add_DoubleDouble"));
DeprecatedFunctionRemapping.Add(TEXT("Subtract_FloatFloat"), TEXT("Subtract_DoubleDouble"));
DeprecatedFunctionRemapping.Add(TEXT("Multiply_FloatFloat"), TEXT("Multiply_DoubleDouble"));
DeprecatedFunctionRemapping.Add(TEXT("Divide_FloatFloat"), TEXT("Divide_DoubleDouble"));
```

**Effect Search Paths** (`GasAbilityGeneratorGenerators.cpp:19580-19591`):
```cpp
SearchPaths.Add(FString::Printf(TEXT("%s/NPCs/Support/Effects/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
SearchPaths.Add(FString::Printf(TEXT("%s/Enemies/Possessed/Effects/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
SearchPaths.Add(FString::Printf(TEXT("%s/Enemies/Warden/Effects/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
SearchPaths.Add(FString::Printf(TEXT("%s/Enemies/Biomech/Effects/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
```

---

## Original Documents (Consolidated)

- v4.7_Report_System_Reference.md (merged)
- Generator_Implementation_Reference.md (merged)
- Gap_Analysis_NarrativePro_v1.md (merged)
- Completed_Automation_Reference.md (merged)
- Design_Compiler_Architecture_Handoff.md (merged)
- Spec_DataAsset_UX_Handoff.md (merged)
- NarrativePro_Asset_Strategy_Log.md (merged v4.12.5)
- Father_Ability_Generator_Plugin_v7_8_2_Specification.md (merged v4.12.5)
- Fail_Fast_Audit_FINAL.md (merged v7.8)
- Pipeline_Gap_Analysis_Audit_v1_0.md (merged v7.8)
- PreValidator_Generator_Parity_Audit_v1.md (merged v7.8)
- GA_Backstab_GA_ProtectiveDome_Error_Audit.md (merged v7.8)
