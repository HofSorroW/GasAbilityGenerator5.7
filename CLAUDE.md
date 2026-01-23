# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Quick Reference

```bash
# Most common: Build + headless generation
powershell -ExecutionPolicy Bypass -File "C:\Unreal Projects\NP22B57\Plugins\GasAbilityGenerator\Tools\claude_automation.ps1" -Action cycle

# Check results
powershell -ExecutionPolicy Bypass -File "C:\Unreal Projects\NP22B57\Plugins\GasAbilityGenerator\Tools\claude_automation.ps1" -Action logs
```

**Single source of truth:** `ClaudeContext/manifest.yaml`

---

## Overview

NP22B57 is an Unreal Engine 5.7 project using Narrative Pro Plugin v2.2 Beta. The project includes the Father Companion system - a transformable spider companion with 5 forms and 19 abilities implemented using the Gameplay Ability System (GAS).

GasAbilityGenerator is an Editor plugin (v4.27) that generates UE5 assets from YAML manifest definitions and CSV dialogue data.

## Project Paths

- Project: `C:\Unreal Projects\NP22B57\`
- GasAbilityGenerator Plugin: `C:\Unreal Projects\NP22B57\Plugins\GasAbilityGenerator\`
- Narrative Pro Plugin: `C:\Unreal Projects\NP22B57\Plugins\NarrativePro22B57\`
- Content: `C:\Unreal Projects\NP22B57\Content\`
- Config: `C:\Unreal Projects\NP22B57\Config\`

## User Paths

- Desktop: `C:\Users\Erdem\OneDrive\Desktop\` (OneDrive synced)

## Build Commands

```bash
# Visual Studio build
MSBuild "NP22B57.sln" /t:Build /p:Configuration="Development Editor" /p:Platform=Win64

# Unreal Build Tool
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" NP22B57Editor Win64 Development -Project="C:\Unreal Projects\NP22B57\NP22B57.uproject"
```

## Commandlet Usage

Generate assets from command line without launching the editor UI:
```bash
"C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Unreal Projects\NP22B57\NP22B57.uproject" -run=GasAbilityGenerator -manifest="C:\Unreal Projects\NP22B57\Plugins\GasAbilityGenerator\ClaudeContext\manifest.yaml"
```

### v3.0 Commandlet Flags

| Flag | Description |
|------|-------------|
| `-dryrun` | Preview what would be created/modified/skipped without making changes |
| `-force` | Regenerate all assets even if they were manually edited (override conflicts) |
| `-level="/Game/..."` | (v3.9.9) Load a World Partition level for POI/NPC Spawner placement |
| `-dialoguecsv="..."` | (v4.0) Parse dialogue CSV file for batch dialogue generation |

```bash
# Preview changes without generating
... -run=GasAbilityGenerator -manifest="..." -dryrun

# Force regeneration of all assets
... -run=GasAbilityGenerator -manifest="..." -force

# Generate assets AND place level actors (POIs, NPC Spawners)
... -run=GasAbilityGenerator -manifest="..." -level="/Game/Maps/MainWorld"

# Generate dialogues from CSV (v4.0 Quest Pipeline)
... -run=GasAbilityGenerator -manifest="..." -dialoguecsv="ClaudeContext/DialogueData.csv"
```

### v3.0/3.1 Regen/Diff Safety System

The metadata system tracks changes to detect what needs regeneration:

| Status | Condition | Action |
|--------|-----------|--------|
| **CREATE** | No existing asset | Generate new asset |
| **MODIFY** | Manifest changed, asset unchanged | Safe to regenerate |
| **SKIP** | No changes detected | Skip generation |
| **CONFLICT** | Both manifest AND asset changed | Requires `--force` or manual resolution |

**How it works:**
1. **Input Hash** - Computed from manifest definition via `Definition.ComputeHash()`
2. **Output Hash** - Computed from asset content via `ComputeDataAssetOutputHash()` or `ComputeBlueprintOutputHash()`
3. **Metadata Storage** - Dual storage system (v3.1):
   - `UGeneratorAssetMetadata` (UAssetUserData) - For assets supporting IInterface_AssetUserData (Animation Montages, etc.)
   - `UGeneratorMetadataRegistry` (DataAsset) - Fallback registry for UDataAsset, UBlueprint, UNiagaraSystem (stored at `/Game/{Project}/GeneratorMetadataRegistry`)

**v3.1 Note:** UDataAsset and UBlueprint don't implement IInterface_AssetUserData in UE5.7, so a central registry stores metadata for these asset types.

**Coverage:** All 32 asset generators have full support:
- 19 DataAsset types: E, IA, IMC, BB, BT, M, MF, MIC, FC, AM, AC (Ability+Activity), IC, NPCDef, CD, TaggedDialogue, NS, Schedule, CharAppearance, TriggerSet
- 13 Blueprint types: GE, GA, ActorBP, WidgetBP, AnimNotify, DialogueBP, EquippableBP, ActivityBP, NE, GameplayCue, Goal, Quest

## Automation Workflow

PowerShell automation script: `Tools/claude_automation.ps1`

| Action | Description | Internal Command |
|--------|-------------|------------------|
| `build` | Build the plugin | `UnrealBuildTool.exe NP22B57Editor Win64 Development` |
| `run` | Launch Unreal Editor | `UnrealEditor.exe "{project}"` |
| `logs` | Get generation logs (after closing editor) | Reads `NP22B57.log`, filters for GasAbilityGenerator entries |
| `full` | Build + launch editor | `build` then `run` |
| `generate` | Headless commandlet generation (no editor UI) | `UnrealEditor.exe -run=GasAbilityGenerator -manifest="..." -unattended -nosplash -nullrhi` |
| `cycle` | Build + headless generation (most common) | `build` then `generate` |

**Development Cycle (Editor UI):**
1. Edit source files
2. Run `build` action
3. If errors → read errors, fix, goto step 2
4. If success → run `run` action to launch editor
5. In Editor → Generate tags, then generate assets
6. Close Editor
7. Run `logs` action to check results
8. If errors → read logs, fix, goto step 1

**Headless Cycle (No Editor UI):**
1. Edit source files
2. Run `cycle` action (builds + generates via commandlet)
3. If build errors → fix and repeat
4. Check `Tools/Logs/commandlet_output.log` for generation results

## Log File Locations

| Log File | Path | Purpose |
|----------|------|---------|
| UBT Build | `Tools/Logs/ubt_latest.log` | Latest Unreal Build Tool output |
| Generation | `Tools/Logs/generation_latest.log` | Latest asset generation results (editor UI) |
| Commandlet | `Tools/Logs/commandlet_output.log` | Headless generation results (cycle/generate actions) |
| Full UE Log | `%LOCALAPPDATA%\UnrealEngine\5.7\Saved\Logs\NP22B57.log` | Complete Unreal Editor log |

**Build error format:**
```
FILE: path/to/file.cpp
LINE: 123
CODE: C2065
MSG:  'identifier': undeclared identifier
---
```

**Generation log categories:** ERRORS (connection failures, missing nodes), WARNINGS (unconnected pins), INFO (successful operations)

## Important Rules

### Mandatory Rules (Will Cause Failures)

**PowerShell Execution Policy** - ALWAYS use `-ExecutionPolicy Bypass`. System has script execution disabled.
```bash
powershell -ExecutionPolicy Bypass -File "script.ps1"  # CORRECT
powershell -File "script.ps1"                          # WRONG - UnauthorizedAccess error
```

**No Temp File Workarounds** - Never copy files to temp locations. Use proper path quoting:
```bash
-manifest="C:/Unreal Projects/NP22B57/..."  # CORRECT
cp file.yaml C:/Temp/file.yaml              # WRONG
```

**Delete Asset Folder Before Regeneration** - After plugin code changes, delete assets before regenerating:
```bash
rm -rf "/c/Unreal Projects/NP22B57/Content/FatherCompanion"
```

**Non-Asset Entries Must Be Nested** - Variables, properties, parameters, keys, nodes, connections must be inside parent assets:
```yaml
# CORRECT                          # WRONG
gameplay_abilities:                variables:
  - name: GA_Attack                  - name: Damage
    variables:
      - name: Damage
```

### Workflow Rules

**Always Push After Commit** - Combine commit and push; don't wait for separate push request.

**Save Handoffs to ClaudeContext/Handoffs/** - All implementation guides and session handoffs go here only.

### Auto-Approved Actions (No Permission Needed)

| Action | Scope |
|--------|-------|
| Read files | `C:\Program Files\Epic Games\UE_5.7\`, `C:\Unreal Projects\NP22B57\`, `C:\Users\Erdem\OneDrive\Desktop\` |
| MSBuild | Any configuration for `NP22B57.sln` |
| UnrealEditor-Cmd.exe | Any commandlet or parameters |

### Generator Behavior

**Locked Exception: Father Validation (v4.16)** - DO NOT add project-specific validations to core generator code without approval. See `ClaudeContext/Handoffs/LOCKED_CONTRACTS.md`.

**Pure Function Bypass (v2.5.2+)**

The generator automatically handles exec connections to pure functions (like `GetAvatarActorFromActorInfo`). Pure functions have **no execution pins** in Blueprint, so exec-to-pure connections in the manifest are non-semantic.

**How it works:**
1. Generator pre-pass identifies pure nodes in event graphs
2. Exec connections targeting pure nodes are skipped (logged as handled)
3. Rerouted connections bypass pure nodes, connecting directly to the next impure node
4. Data connections to pure nodes work normally

**Manifest pattern (accepted but non-semantic):**
```yaml
connections:
  - from: [Event_Activate, Then]
    to: [GetAvatarActor, Exec]      # Skipped - GetAvatarActor is pure
  - from: [GetAvatarActor, Exec]
    to: [CastToFather, Exec]        # Rerouted to Event_Activate -> CastToFather
  - from: [GetAvatarActor, ReturnValue]
    to: [CastToFather, Object]      # Works normally (data flow)
```

**Clean pattern (preferred):**
```yaml
connections:
  - from: [Event_Activate, Then]
    to: [CastToFather, Exec]        # Direct exec connection
  - from: [GetAvatarActor, ReturnValue]
    to: [CastToFather, Object]      # Data flow only
```

Both patterns produce identical Blueprints. The bypass ensures backwards compatibility with existing manifests.

**Reference:** `ClaudeContext/Handoffs/Generator_Pure_Node_Bypass_Analysis_v2_0.md`

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| UnauthorizedAccess on PowerShell | Add `-ExecutionPolicy Bypass` before `-File` |
| Assets not regenerating after code changes | Delete `/Content/FatherCompanion/` folder - Asset Registry caches existing assets |
| "Class not found" warnings for Narrative Pro classes | Acceptable - generator can't resolve plugin paths but classes work at runtime |
| Manifest parsing failed | Check YAML indent (2 spaces, no tabs), validate nested structure |
| CONFLICT status on generation | Asset was manually edited; use `-force` flag or resolve manually |
| Build succeeds but editor crashes | Check `Tools/Logs/commandlet_output.log` for runtime errors |
| Event graph nodes not connecting | Verify pin names match exactly (case-sensitive): `Then`, `Exec`, `ReturnValue` |

---

## GasAbilityGenerator Plugin (v4.27)

Location: `Plugins/GasAbilityGenerator/`

**No automated tests.** Validation is performed via the `-dryrun` flag and generation log analysis.

### Architecture

```
manifest.yaml → Parser → FManifestData → Generators → UE5 Assets
     ↓                                        ↓
CSV Dialogue → CSVParser ──────────────────────┘
     ↓
Mesh Files → Pipeline → Items → Collections → NPC Loadouts
```

| Class | Location | Purpose |
|-------|----------|---------|
| `FGasAbilityGeneratorModule` | GasAbilityGeneratorModule.cpp | Plugin lifecycle, menu registration |
| `FGasAbilityGeneratorParser` | GasAbilityGeneratorParser.cpp | YAML parsing to FManifestData |
| `FDialogueCSVParser` | GasAbilityGeneratorDialogueCSVParser.cpp | v4.0 CSV dialogue parsing |
| `SGasAbilityGeneratorWindow` | GasAbilityGeneratorWindow.cpp | Slate UI for editor |
| `UGasAbilityGeneratorCommandlet` | GasAbilityGeneratorCommandlet.cpp | Command-line generation |
| `F*Generator` classes | GasAbilityGeneratorGenerators.cpp | 25+ asset generators |
| `FPipelineProcessor` | GasAbilityGeneratorPipeline.cpp | v3.9.8 Mesh-to-Item pipeline |
| `UScheduledBehavior_AddNPCGoalByClass` | GasScheduledBehaviorHelpers.cpp | v3.9.1 Schedule behavior helper |
| `GasAbilityGeneratorTypes.h` | Public/ | FManifest* structs, validation types |
| `GasAbilityGeneratorMetadata.h` | Public/ | v3.0 Regen/Diff metadata classes |
| `GasAbilityGeneratorFactories.h` | Public/ | Asset factory classes |
| `GasAbilityGeneratorSpecs.h` | Public/ | Spec handling utilities |
| `SNPCTableEditor` | NPCTableEditor/ | NPC table editor UI |
| `SQuestTableEditor` | QuestTableEditor/ | Quest table editor UI (v4.8) |
| `SItemTableEditor` | ItemTableEditor/ | Item table editor UI (v4.8) |
| `SDialogueTableEditor` | SDialogueTableEditor.cpp/h | Dialogue table editor UI |
| `FDialogueTableConverter` | DialogueTableConverter.cpp/h | XLSX ↔ DialogueBlueprint conversion |
| `FDialogueTableValidator` | DialogueTableValidator.cpp/h | Token validation, error detection |
| XLSX Support | XLSXSupport/ | OpenXLSX integration for Excel files |

**Naming Convention:** All structs use `FManifest*` prefix (e.g., `FManifestData`, `FManifestGameplayAbilityDefinition`).

### Table Editors (v4.8)

The plugin includes Excel-like table editors for bulk content authoring:
- **NPC Table Editor** (`SNPCTableEditor`) - 17 columns, sync from/to NPCDefinition assets
- **Dialogue Table Editor** (`SDialogueTableEditor`) - 11 columns, tree structure, CSV/XLSX import/export
- **Quest Table Editor** (`SQuestTableEditor`) - 12 columns, quest grouping, state machine view (v4.8)
- **Item Table Editor** (`SItemTableEditor`) - 16 columns, dynamic visibility, type-based fields (v4.8)

**Common Systems** (consistent across all 4 table editors):
- Validation cache with `ValidationInputHash` staleness detection
- XLSX 3-way sync with `LastSyncedHash`
- Soft delete (`bDeleted` flag)
- Generation tracking (`LastGeneratedHash`, `AreAssetsOutOfDate()`)
- Re-entrancy guard (`bIsBusy`)
- Status badges with colored indicators
- Tab dirty state and parent tab integration

**Full documentation:** See `ClaudeContext/Handoffs/Table_Editors_Reference.md` for:
- Feature comparison and column structure
- Slate development patterns (Text_Lambda vs SetText, filter state, cell styling)
- XLSX 3-way sync system (v4.3)
- Validated Token design (v4.4)

### Supported Asset Types (Generated by Plugin)

| Prefix | Asset Type | Manifest Section | Generator Class |
|--------|------------|------------------|-----------------|
| E_ | Enumerations | `enumerations` | FEnumerationGenerator |
| IA_ | Input Actions | `input_actions` | FInputActionGenerator |
| IMC_ | Input Mapping Contexts | `input_mapping_contexts` | FInputMappingContextGenerator |
| GE_ | Gameplay Effects | `gameplay_effects` | FGameplayEffectGenerator |
| GA_ | Gameplay Abilities | `gameplay_abilities` | FGameplayAbilityGenerator |
| BP_ | Actor Blueprints | `actor_blueprints` | FActorBlueprintGenerator |
| BPC_ | Component Blueprints | `component_blueprints` | FActorComponentBlueprintGenerator |
| WBP_ | Widget Blueprints | `widget_blueprints` | FWidgetBlueprintGenerator |
| DBP_ | Dialogue Blueprints | `dialogue_blueprints` | FDialogueBlueprintGenerator |
| BB_ | Blackboards | `blackboards` | FBlackboardGenerator |
| BT_ | Behavior Trees | `behavior_trees` | FBehaviorTreeGenerator |
| M_ | Materials | `materials` | FMaterialGenerator |
| MF_ | Material Functions | `material_functions` | FMaterialFunctionGenerator |
| MIC_ | Material Instances | `material_instances` | FMaterialInstanceGenerator |
| FC_ | Float Curves | `float_curves` | FFloatCurveGenerator |
| AM_ | Animation Montages | `animation_montages` | FAnimationMontageGenerator |
| NAS_ | Animation Notifies | `animation_notifies` | FAnimationNotifyGenerator |
| EI_ | Equippable Items | `equippable_items` | FEquippableItemGenerator |
| BPA_ | Activities | `activities` | FActivityGenerator |
| AC_ | Ability Configurations | `ability_configurations` | FAbilityConfigurationGenerator |
| AC_ | Activity Configurations | `activity_configurations` | FActivityConfigurationGenerator |
| IC_ | Item Collections | `item_collections` | FItemCollectionGenerator |
| NE_ | Narrative Events | `narrative_events` | FNarrativeEventGenerator |
| NPC_ | NPC Definitions | `npc_definitions` | FNPCDefinitionGenerator |
| CD_ | Character Definitions | `character_definitions` | FCharacterDefinitionGenerator |
| NS_ | Niagara Systems | `niagara_systems` | FNiagaraSystemGenerator |
| Schedule_ | Activity Schedules | `activity_schedules` | FActivityScheduleGenerator |
| Goal_ | Goal Items | `goal_items` / `goals` | FGoalItemGenerator |
| Quest_ | Quests | `quests` | FQuestGenerator |
| - | Gameplay Tags | `tags` | FTagGenerator |
| - | Tagged Dialogue Sets | `tagged_dialogue_sets` | FTaggedDialogueSetGenerator |

### Other Asset Prefixes (Not Generated)

#### Referenced from Narrative Pro

| Prefix | Type | Location | Usage |
|--------|------|----------|-------|
| BTS_ | BT Service | NarrativePro/AI/ | Standard services (BTS_SetAIFocus, BTS_ClearAIFocus) |
| BTT_ | BT Task (Blueprint) | NarrativePro/AI/ | Standard tasks (BTT_SetBehaviorTree) |
| BTTask_ | BT Task (C++) | C++ only | Built-in tasks (BTTask_ActivateAbilityByClass) |
| EQS_ | Environment Query | NarrativePro/AI/ | Standard queries (EQS_Actor_SensedAttackTarget) |
| EQSContext_ | EQS Context | C++ only | Query contexts (EQSContext_PlayerPawn) |
| GoalGenerator_ | Goal Generator | NarrativePro/Goals/ | Goal generators (GoalGenerator_Attack) - v3.9 adds Goal_ generator |
| BPT_ | Blueprint Trigger | NarrativePro/Schedules/ | Trigger conditions (BPT_TimeOfDayRange, BPT_Always) |
| BI_ | Blueprint Item | NarrativePro/Items/ | Item system (BI_NarrativeBook) - Father uses NPC system |
| DT_ | Data Table | Standard UE5 | Configuration tables - referenced, not generated |

#### External Dependencies (art/source assets)

| Prefix | Type | Source | Notes |
|--------|------|--------|-------|
| A_ | Animation Sequence | External (Maya/Blender) | Source animations for montages |
| SM_ | Static Mesh | External (Maya/Blender) | 3D models and props |
| T_ | Texture | External (Photoshop/Substance) | Material textures |
| SK_ | Skeletal Mesh | External (Maya/Blender) | Character meshes |

#### Nested Inside Other Assets (not standalone)

| Type | Nested In | Example |
|------|-----------|---------|
| TS_ (Trigger Set) | NPCDefinition | Schedule containers |
| Variables | GA_, BP_, WBP_ | Blueprint variables |
| Event Graph Nodes | GA_, BP_, WBP_ | Blueprint logic |
| Blackboard Keys | BB_ | Blackboard data |
| Material Parameters | M_ | Texture/scalar params |

#### Gameplay Cues (GC_)

Gameplay Cues use `GameplayCue.*` tags in manifest `tags` section:

| Tag | Cue Type | Purpose |
|-----|----------|---------|
| GameplayCue.Father.FormTransition | Burst | Form change VFX |
| GameplayCue.Father.Attack | Burst | Melee hit impact |
| GameplayCue.Father.LaserShot | Burst | Laser projectile |
| GameplayCue.Father.Mark | Burst | Enemy marked |
| GameplayCue.Father.ProximityStrike | BurstLatent | AOE damage pulse |
| GameplayCue.Father.Sacrifice | BurstLatent | Sacrifice sequence |

Cue assets (GC_*) are created in-editor referencing these tags. Parent class determined by effect duration:
- **Burst effects:** `GameplayCueNotify_Burst` (instant VFX)
- **Latent effects:** `GameplayCueNotify_BurstLatent` (duration VFX)
- **Persistent effects:** `AGameplayCueNotify_Actor` (looping VFX)

### Automatic Project Detection

The plugin auto-detects your Unreal project name and creates assets under `/Game/{ProjectName}/`. No need to set `project_root` in manifest unless you want a custom path.

### Manifest Structure

```yaml
# Optional: Override auto-detected project root
project_root: /Game/FatherCompanion
tags_ini_path: Config/DefaultGameplayTags.ini

tags:
  - tag: Ability.Father.Attack
    comment: Description

enumerations:
  - name: E_FatherForm
    folder: Enumerations
    values:
      - Crawler
      - Armor

gameplay_abilities:
  - name: GA_FatherAttack
    parent_class: NarrativeGameplayAbility
    folder: Abilities/Crawler
    instancing_policy: InstancedPerActor
    tags:
      ability_tags:
        - Ability.Father.Attack
    variables:
      - name: DamageAmount
        type: Float
        default_value: "25.0"
    event_graph: AttackGraph

# v3.1: AbilityConfiguration with TSubclassOf resolution
ability_configurations:
  - name: AC_FatherCrawler
    folder: AbilityConfigs
    abilities: [GA_FatherAttack, GA_FatherLaserShot]
    startup_effects: [GE_FatherCrawlerStats]      # Applied once at spawn
    default_attributes: GE_FatherAttributes       # Attribute initialization

# v3.1: ActivityConfiguration with goal generators
# v4.8.2: Uses AC_ prefix (same as AbilityConfiguration, differentiated by UClass)
activity_configurations:
  - name: AC_FatherBehavior
    folder: ActivityConfigs
    activities: [BPA_FatherFollow, BPA_FormationFollow]
    goal_generators: [GoalGenerator_Attack]       # Goal generation classes
    rescore_interval: 1.0

# v3.1: BehaviorTree with node tree structure
behavior_trees:
  - name: BT_FatherFollow
    blackboard: BB_Father
    folder: AI/BehaviorTrees
    root_type: Selector                           # Selector or Sequence
    nodes:
      - id: MoveToPlayer
        type: Task
        task_class: BTTask_MoveTo
        blackboard_key: TargetPlayer
        decorators:
          - class: BTDecorator_Blackboard
            blackboard_key: TargetPlayer
            operation: IsSet
        services:
          - class: BTService_BlueprintBase
            interval: 0.5

# v3.2: NarrativeEvent with event configuration
narrative_events:
  - name: NE_FatherAwakens
    folder: Events
    event_runtime: Start                          # Start, End, Both
    event_filter: OnlyPlayers                     # Anyone, OnlyNPCs, OnlyPlayers
    party_policy: Party                           # Party, AllPartyMembers, PartyLeader
    refire_on_load: false
    npc_targets: [NPC_Father]                  # Logged for manual setup
    character_targets: []
    player_targets: []

# v3.5: DialogueBlueprint with dialogue configuration
dialogue_blueprints:
  - name: DBP_FatherGreeting
    parent_class: NarrativeDialogue
    folder: Dialogues
    free_movement: true                           # Allow movement during dialogue
    unskippable: false                            # Cannot skip dialogue lines
    can_be_exited: true                           # Can exit via ESC
    cinematic_bars: false                         # Show letterbox bars
    auto_rotate_speakers: true                    # Auto-face speakers
    auto_stop_movement: false                     # Stop NPC movement
    priority: 0                                   # Higher = more important
    end_dialogue_distance: 500.0                  # Auto-end if players move away
    # v3.5: Additional UDialogue properties
    default_head_bone_name: head                  # Bone for camera focus (default: head)
    dialogue_blend_out_time: 0.5                  # Camera blend-out duration
    adjust_player_transform: false                # Move player to face speaker
    speakers:                                     # Logged for manual setup
      - npc_definition: NPC_Father
        node_color: "#FF6600"
        owned_tags: [Father.State.Speaking]
    # v3.8: Full dialogue tree generation
    dialogue_tree:
      root: greeting_node                         # ID of root node
      nodes:
        - id: greeting_node
          type: npc                               # npc or player
          speaker: NPC_Father
          text: "Hello, traveler!"
          duration: auto                          # auto, manual, or seconds
          skippable: true
          player_replies:                         # Links to player choice nodes
            - friendly_response
            - hostile_response
        - id: friendly_response
          type: player
          text: "Hello, friend!"
          option_text: "Greet warmly"             # Text shown in dialogue wheel
          npc_replies: [farewell_node]
        - id: hostile_response
          type: player
          text: "Leave me alone."
          option_text: "Be rude"
          events:                                 # Events fired during node
            - type: NarrativeEvent
              runtime: Start                      # Start, End, Both
              properties:
                event_class: NE_PlayerRude
          npc_replies: [angry_farewell]
        - id: farewell_node
          type: npc
          speaker: NPC_Father
          text: "Safe travels!"
          conditions:                             # Node visibility conditions
            - type: HasGameplayTag
              not: false                          # Invert condition
              properties:
                tag: Father.State.Friendly

# v3.5: CharacterDefinition with full property support
character_definitions:
  - name: CD_Merchant
    folder: Characters/Definitions
    # v3.5: Core properties (arrays populate FGameplayTagContainer)
    default_owned_tags:
      - State.Invulnerable
      - Character.Merchant
    default_factions:
      - Narrative.Factions.Friendly
      - Narrative.Factions.Town
    default_currency: 1000                        # Starting gold
    attack_priority: 0.2                          # Low priority target
    # v3.5: Additional properties
    default_appearance: Appearance_Merchant       # TSoftObjectPtr<UObject>
    trigger_sets:                                 # TArray<FNPCTriggerSet>
      - TS_MerchantSchedule
      - TS_MerchantCombat
    ability_configuration: AC_Merchant            # FObjectProperty

# v3.7: NPCDefinition with full property support and auto-create
npc_definitions:
  - name: NPC_Blacksmith
    folder: NPCs/Definitions
    npc_id: Blacksmith_01
    npc_name: Garrett the Blacksmith
    npc_blueprint: BP_Blacksmith                  # TSoftClassPtr<ANarrativeNPCCharacter>
    ability_configuration: AC_Blacksmith
    activity_configuration: AC_BlacksmithBehavior
    min_level: 1
    max_level: 10
    allow_multiple_instances: false
    # v3.3: Dialogue properties
    dialogue: DBP_BlacksmithDialogue              # Main conversation dialogue
    tagged_dialogue_set: TDS_BlacksmithBarks     # Contextual barks
    # v3.3: Vendor properties
    is_vendor: true
    trading_currency: 500                         # Starting gold
    buy_item_percentage: 0.5                      # Pays 50% of item value
    sell_item_percentage: 1.5                     # Charges 150% of item value
    shop_name: "Garrett's Forge"
    # v3.3: CharacterDefinition properties
    default_appearance: Appearance_Blacksmith
    default_currency: 100
    default_owned_tags: [State.Invulnerable]      # Array or single value
    default_factions: [Narrative.Factions.Friendly, Narrative.Factions.Town]
    attack_priority: 0.5                          # Low priority target
    # v3.6: Activity schedules (NPC daily routines)
    activity_schedules:                           # TArray<TSoftObjectPtr<UNPCActivitySchedule>>
      - Schedule_BlacksmithDay
      - Schedule_BlacksmithNight
    # v3.7: Auto-create related assets (full NPC package)
    auto_create_dialogue: true                    # Creates DBP_BlacksmithDialogue
    auto_create_tagged_dialogue: true             # Creates Blacksmith_TaggedDialogue
    auto_create_item_loadout: true                # Populates DefaultItemLoadout
    default_item_loadout_collections:             # v3.7: Simple item collections (legacy)
      - IC_RifleWithAmmo
      - IC_ExampleArmorSet
    # v3.9.5: Full loot table roll support (preferred over default_item_loadout_collections)
    default_item_loadout:                         # TArray<FLootTableRoll> - items granted at spawn
      - items:                                    # TArray<FItemWithQuantity>
          - item: EI_IronSword
            quantity: 1
          - item: EI_HealthPotion
            quantity: 5
        item_collections:                         # TArray<TObjectPtr<UItemCollection>>
          - IC_BlacksmithTools
        chance: 1.0                               # Roll success chance (0.0-1.0)
        num_rolls: 1                              # Times to roll this entry
      - table_to_roll: /Game/Tables/DT_RandomLoot  # Optional: DataTable-based rolling
        chance: 0.5
        num_rolls: 2
    trading_item_loadout:                         # TArray<FLootTableRoll> - vendor inventory (bIsVendor=true)
      - item_collections:
          - IC_WeaponsForSale
          - IC_ArmorForSale
        chance: 1.0

# v3.3: EquippableItem with full property support
equippable_items:
  - name: EI_DragonbaneArmor
    folder: Items/Equipment
    parent_class: EquippableItem
    display_name: "Dragonbane Armor"
    description: "Forged from dragon scales, this armor provides exceptional protection against fire."
    equipment_slot: Narrative.Equipment.Slot.Chest
    equipment_modifier_ge: GE_DragonbaneArmorStats
    # v3.3: EquippableItem stat properties
    attack_rating: 5.0                            # Bonus attack damage
    armor_rating: 25.0                            # Damage reduction
    stealth_rating: -10.0                         # Stealth penalty (heavy armor)
    # v3.3: NarrativeItem properties
    thumbnail: "/Game/UI/Icons/T_DragonbaneArmor"
    weight: 15.0                                  # Weight in kg
    base_value: 500                               # Gold value
    base_score: 75.0                              # AI priority score
    stackable: false
    max_stack_size: 1
    use_recharge_duration: 0.0
    item_tags:
      - Item.Armor.Heavy
      - Item.Element.Fire.Resistant
    abilities_to_grant:
      - GA_FireResistance

# v3.3: Activity with full property support
activities:
  - name: BPA_PatrolRoute
    folder: AI/Activities
    parent_class: NarrativeActivityBase
    behavior_tree: BT_Patrol
    activity_name: "Patrol Route"
    description: "NPC patrols a predefined route"
    # v3.3: NarrativeActivityBase tag properties
    owned_tags:
      - State.Patrolling
    block_tags:
      - State.InCombat
      - State.Fleeing
    require_tags:
      - State.Alive
    # v3.3: NPCActivity properties
    supported_goal_type: Goal_Patrol                # TSubclassOf<UNPCGoalItem>
    is_interruptable: true                          # Can be interrupted by higher priority activities
    save_activity: false                            # Don't persist across saves

# v3.4: WeaponItem with full weapon property support
equippable_items:
  - name: EI_FatherRifleWeapon
    folder: Items/Weapons
    parent_class: RangedWeaponItem                  # Enables WeaponItem + RangedWeaponItem properties
    display_name: "Father's Laser Rifle"
    description: "A high-powered laser rifle formed from Father's Engineer mode."
    equipment_slot: Narrative.Equipment.Slot.Weapon
    # v3.4: WeaponItem properties
    weapon_visual_class: "/Game/Weapons/WV_LaserRifle"  # TSoftClassPtr<AWeaponVisual>
    weapon_hand: TwoHanded                          # TwoHanded, MainHand, OffHand, DualWieldable
    pawn_follows_control_rotation: true
    pawn_orients_rotation_to_movement: false
    attack_damage: 50.0
    heavy_attack_damage_multiplier: 2.0
    allow_manual_reload: true
    required_ammo: EI_EnergyCells                   # TSubclassOf<UNarrativeItem>
    bots_consume_ammo: false
    bot_attack_range: 2000.0
    clip_size: 30
    # v3.4: RangedWeaponItem properties
    aim_fov_pct: 0.5                                # 50% FOV when aiming
    base_spread_degrees: 1.0
    max_spread_degrees: 8.0
    spread_fire_bump: 0.3
    spread_decrease_speed: 4.0
    # v3.4: Weapon ability arrays
    weapon_abilities:                               # Granted when wielded alone
      - GA_LaserShot
    mainhand_abilities:                             # Granted when wielded in mainhand
      - GA_LaserShot
    offhand_abilities: []                           # No offhand abilities

# v3.9: ActivitySchedule for NPC daily routines
# Uses UScheduledBehavior_AddNPCGoalByClass - concrete helper for goal-based scheduling
# Time format: 0-2400 where 100 = 1 hour (e.g., 600 = 6:00 AM, 1800 = 6:00 PM)
# NOTE on Locations: Narrative Pro handles locations via goal-specific classes, not schedule-level:
#   - Goal_MoveToDestination (GoToLocation) - walk to a point
#   - Goal_DriveToDestination - drive a vehicle to a point
#   - Goal_FlyToDestination - fly to a point
#   To schedule NPC location changes, create goal subclasses with preset destinations
#   (e.g., Goal_GoToForge, Goal_GoToTavern) and reference those in the schedule.
activity_schedules:
  - name: Schedule_BlacksmithDay
    folder: AI/Schedules
    behaviors:
      - start_time: 600                             # 6:00 AM
        end_time: 1200                              # 12:00 PM (noon)
        goal_class: Goal_Work                       # UNPCGoalItem class to add
        score_override: 100.0                       # Priority score (-1 = use goal's default)
        reselect: true                              # Trigger activity reselection
      - start_time: 1200
        end_time: 1300                              # 1:00 PM
        goal_class: Goal_Eat
      - start_time: 1300
        end_time: 1800                              # 6:00 PM
        goal_class: Goal_Work
      - start_time: 2200                            # 10:00 PM
        end_time: 600                               # Next day 6:00 AM (wraps)
        goal_class: Goal_Sleep

# v3.9: GoalItem for AI objectives
goal_items:
  - name: Goal_DefendForge
    folder: AI/Goals
    parent_class: NPCGoalItem                       # Default parent class
    default_score: 75.0                             # Priority when active
    goal_lifetime: -1.0                             # -1 = never expires
    remove_on_succeeded: false                      # Keep goal after completion
    save_goal: true                                 # Persist across saves
    owned_tags:                                     # Tags granted while active
      - State.Defending
    block_tags:                                     # Tags that block this goal
      - State.Fleeing
    require_tags:                                   # Tags required to pursue goal
      - State.Alive

# v3.9.3: Quest blueprint with state machine definition
# NOTE: Questgiver Pattern in Narrative Pro:
#   NPC (NPCDefinition.dialogue) → Dialogue (with start_quest event) → Quest begins
#   The `questgiver` field is for documentation - UQuest has no Questgiver property.
#   The actual questgiver is the NPC whose dialogue contains a start_quest event for this quest.
# NOTE: Quest State Machine - FULLY AUTOMATED (v4.13+):
#   The generator creates UQuestState and UQuestBranch runtime objects using:
#   - Quest->AddState(State) for each state
#   - Quest->AddBranch(Branch) for each branch
#   - Quest->SetQuestStartState() for the initial state
#   UEdGraph visual nodes (editor UI) are not generated - runtime state machine works without them.
quests:
  - name: Quest_ForgeSupplies
    folder: Quests/Town
    quest_name: "Forge Supplies"
    description: "Gather iron ore for the blacksmith."
    questgiver: NPC_Blacksmith                        # Documentation only - links to NPC whose dialogue starts this quest
    is_tracked: true
    states:
      - id: Start
        type: regular                               # regular, success, failure
        description: "Talk to the blacksmith"
      - id: Gathering
        type: regular
        description: "Gather 10 iron ore"
      - id: Return
        type: regular
        description: "Return to the blacksmith"
      - id: Complete
        type: success                               # Quest succeeds at this state
        description: "Quest completed!"
    branches:
      - id: AcceptQuest
        from_state: Start
        to_state: Gathering
        description: "I'll gather the ore"
        tasks:
          - task_class: BPT_FinishDialogue          # Narrative Pro task classes
            properties:
              dialogue: DBP_BlacksmithQuest
      - id: GatherOre
        from_state: Gathering
        to_state: Return
        description: "Gather iron ore"
        tasks:
          - task_class: BPT_FindItem
            quantity: 10
            properties:
              item_class: IronOre
      - id: ReturnToBlacksmith
        from_state: Return
        to_state: Complete
        description: "Talk to blacksmith"
        tasks:
          - task_class: BPT_FinishDialogue
            properties:
              dialogue: DBP_BlacksmithReturn
    rewards:
      currency: 100
      xp: 50
      items: [EI_IronSword]

# v4.3: Widget Blueprint with full widget tree layout
widget_blueprints:
  - name: WBP_UltimatePanel
    folder: UI
    parent_class: UserWidget
    variables:
      - name: CurrentCharge
        type: Float
        default: 0.0
    widget_tree:
      root_widget: RootCanvas
      widgets:
        - id: RootCanvas
          type: CanvasPanel
          children: [ContentBox]
        - id: ContentBox
          type: VerticalBox
          is_variable: true                         # Exposes as Blueprint variable
          slot:
            anchors: BottomCenter                   # TopLeft, Center, BottomCenter, etc.
            alignment: "0.5, 1.0"                   # Pivot point
            position: "0, -50"                      # Offset from anchor
          children: [ChargeLabel, ChargeBar]
        - id: ChargeLabel
          type: TextBlock
          text: "ULTIMATE"
          slot:
            h_align: Center                         # Left, Center, Right, Fill
            v_align: Center                         # Top, Center, Bottom, Fill
          properties:
            Font.Size: 18
        - id: ChargeBar
          type: ProgressBar
          is_variable: true
          slot:
            h_align: Fill
            size_rule: Fill                         # Auto or Fill
            padding: "10, 5, 10, 5"                 # L, T, R, B
          properties:
            BarFillType: LeftToRight
            Percent: 0.0

# v4.19: Component Blueprint with variables, event dispatchers, functions, and tick config
component_blueprints:
  - name: BPC_HealthComponent
    folder: Components
    parent_class: ActorComponent                  # Must derive from UActorComponent
    variables:
      - name: MaxHealth
        type: Float
        default_value: "100.0"
      - name: CurrentHealth
        type: Float
        default_value: "100.0"
    event_dispatchers:
      - name: OnHealthChanged
        parameters:
          - name: NewHealth
            type: Float
          - name: DeltaHealth
            type: Float
      - name: OnDeath
        parameters: []                            # No parameters
    functions:
      - name: TakeDamage
        pure: false                               # Has side effects
        inputs:
          - name: DamageAmount
            type: Float
        outputs:
          - name: RemainingHealth
            type: Float
      - name: GetHealthPercent
        pure: true                                # No side effects, no exec pins
        inputs: []
        outputs:
          - name: Percent
            type: Float
    tick:
      can_ever_tick: true
      start_with_tick_enabled: false
      tick_interval: 0.0                          # 0 = every frame when enabled
```

### Event Graph Generation

Define Blueprint event graphs in YAML:

```yaml
event_graphs:
  - name: EventGraph_Example
    nodes:
      - id: BeginPlay
        type: Event
        properties:
          event_name: ReceiveBeginPlay
      - id: PrintHello
        type: PrintString
        properties:
          message: "Hello World!"
    connections:
      - from: [BeginPlay, Then]
        to: [PrintHello, Exec]

actor_blueprints:
  - name: BP_Example
    parent_class: Actor
    folder: Blueprints
    event_graph: EventGraph_Example
```

#### Supported Node Types

| Type | Properties | Description |
|------|------------|-------------|
| Event | event_name | BeginPlay, Tick, EndPlay, etc. |
| CustomEvent | event_name | User-defined events |
| CallFunction | function, class, target_self | Function calls |
| Branch | - | If/Then/Else |
| VariableGet/Set | variable_name | Variable access |
| PropertyGet/Set | property_name, target_class | Property access |
| Sequence | num_outputs | Multiple outputs |
| Delay | duration | Timer delay |
| PrintString | message, duration | Debug output |
| DynamicCast | target_class | Type casting |
| ForEachLoop | - | Array iteration |
| SpawnActor | actor_class_variable | Spawn actors |
| BreakStruct | struct_type | Break struct into member pins |
| MakeArray | element_type, num_elements | Create array from elements |
| GetArrayItem | - | Access array element by index |
| Self | - | Reference to blueprint self |

### Parser Field Aliases

The YAML parser accepts multiple field names for the same property:

| Canonical Name | Alias | Used In |
|----------------|-------|---------|
| `npc_name:` | `display_name:` | npc_definitions |
| `npc_class_path:` | `npc_blueprint:` | npc_definitions |
| `dialogue_sound_attenuation:` | `sound_attenuation:` | dialogue_blueprints |

**Prefer canonical names** in new manifests. Aliases exist for backward compatibility.

### Search Paths

When looking for classes/enums, the plugin searches:
1. Native UE classes
2. `/Game/{ProjectName}/Blueprints/`, `/Game/{ProjectName}/Characters/`, `/Game/{ProjectName}/Actors/`
3. `/Game/{ProjectName}/Enums/`, `/Game/{ProjectName}/Enumerations/`, `/Game/{ProjectName}/Data/`
4. `/Game/` root

### Dependencies (Build.cs)

- **Blueprint Generation:** BlueprintGraph, Kismet, KismetCompiler
- **GAS:** GameplayAbilities, GameplayTags, GameplayTasks
- **Editor:** UnrealEd, AssetTools, AssetRegistry
- **Optional (Narrative Pro):** NarrativeArsenal, NarrativePro

### Key Files in ClaudeContext/

**IMPORTANT:** All handoff documents must be saved to `ClaudeContext/Handoffs/`. This is the single location for implementation guides, research notes, and session handoffs.

| File | Purpose |
|------|---------|
| `Handoffs/Father_Companion_GAS_Audit_Locked_Decisions.md` | **v2.0** GAS lifecycle audit, EndAbility rules, severity matrix, Track A/B strategy |
| `Handoffs/VFX_System_Reference.md` | v4.9/4.9.1 VFX automation, MIC session cache, whitelist verification |
| `Handoffs/v4.8_Coverage_Expansion_Handoff.md` | Quest/Item Table Editors implementation guide |
| `Handoffs/v4.7_Report_System_Reference.md` | Machine-readable report system, JSON export, structured errors |
| `Handoffs/Table_Editors_Reference.md` | NPC/Dialogue editor patterns, XLSX sync, Validated Tokens |
| `Handoffs/v4.6_UX_Safety_System_ProcessMap.md` | Auto-save, soft delete, validation gate |
| `Handoffs/Sync_System_Design_v4.11.md` | 3-way sync architecture |
| `Handoffs/Architecture_Reference.md` | Plugin architecture overview |
| `Handoffs/Generator_Implementation_Reference.md` | Generator patterns and techniques |
| `Handoffs/Gap_Analysis_NarrativePro_v1.md` | Narrative Pro coverage analysis |
| `Handoffs/Completed_Automation_Reference.md` | Automation status by asset type |
| `Handoffs/TSoftClassPtr_Investigation.md` | Class pointer resolution research |
| `manifest.yaml` | Single source of truth for all assets |
| `Father_Ability_Generator_Plugin_v7_8_2_Specification.md` | Plugin architecture and workflows |
| `Father_Companion_Technical_Reference_v6_0.md` | GAS patterns, tags, Narrative Pro integration |
| `Father_Companion_Guide_Format_Reference_v3_0.md` | Formatting standards for implementation guides |
| `Father_Companion_System_Design_Document_v2_0.md` | System overview, 5 forms, abilities |
| `DefaultGameplayTags_FatherCompanion_v4_0.ini` | Actual gameplay tag values |
| `NarrativePro_Headers/*.h` | Narrative Pro C++ API reference |

### Critical Rules

**DO:**
- Use Blueprint-only solutions for implementation guides
- Use Narrative Pro native systems (NarrativeGameplayAbility, NarrativeNPCCharacter)
- Follow manifest.yaml as single source of truth
- Generate enumerations BEFORE blueprints that reference them
- Use SetByCaller tags for dynamic damage values
- Check asset existence before generation (skip if exists)

**DO NOT:**
- Use GAS Companion plugin (removed from project)
- Use NarrativeCharacter class (use NarrativeNPCCharacter)
- Use UGameplayAbility class (use NarrativeGameplayAbility)
- Write C++ in implementation guides
- Use attribute-based movement speed (use CharacterMovement directly)
- Spawn spider from player BeginPlay (use NPCSpawner system)
- Change parent classes from what's defined in the Technical Reference or Parent Class Matrix
  - Parent classes are intentional (e.g., GA_Melee_Unarmed provides motion warping)
  - Generator warnings about missing parent classes are acceptable if the class exists in Narrative Pro
  - The generator may not resolve plugin paths, but the class will work at runtime

### Father Companion Forms

1. **Crawler** - Default, follows player, melee/laser attacks
2. **Armor** - Attaches to chest, protective dome, damage absorption
3. **Exoskeleton** - Attaches to back, speed boost, dash, stealth
4. **Symbiote** - Merges with player, stat boosts, proximity damage
5. **Engineer** - Turret mode, electric traps, auto-targeting

### Error Code Reference

| Code | Severity | Description |
|------|----------|-------------|
| `E_PREVAL_CLASS_NOT_FOUND` | Error | Referenced class doesn't exist |
| `E_PREVAL_FUNCTION_NOT_FOUND` | Error | Referenced function doesn't exist on class |
| `E_PREVAL_ATTRIBUTE_NOT_FOUND` | Error | Referenced attribute doesn't exist on AttributeSet |
| `E_PREVAL_ASSET_NOT_FOUND` | Error | Referenced asset path doesn't exist |
| `E_PREVAL_TAG_NOT_REGISTERED` | Warning | Gameplay tag not in DefaultGameplayTags.ini |
| `E_PREVAL_TOKEN_UNSUPPORTED` | Error | Token type not supported by generator |
| `E_CIRCULAR_DEPENDENCY` | Error | Circular reference detected (A→B→C→A) |
| `E_CASCADE_SKIPPED` | Info | Skipped due to dependency failure |
| `E_DELEGATE_NOT_FOUND` | Error | Delegate name doesn't exist on class |
| `E_DELEGATE_SIGNATURE_MISMATCH` | Error | Handler signature doesn't match delegate |
| `E_DELEGATE_SOURCE_INVALID` | Error | Variable source for delegate not found |
| `W_TRANSITION_INVALID` | Warning | Form transition missing required tags |

### Plugin Version History (Recent)

| Version | Summary |
|---------|---------|
| **v4.27** | GA→GA dependency scanning for TSubclassOf; automatic dome burst on form exit; dependency sort order audit (Decisions 22-24) |
| **v4.26** | Session cache for TSubclassOf resolution; external reference detection; 156/156 assets |
| **v4.25** | Dependency ordering (topological sort); cascade skip logic |
| **v4.24** | Pre-validation system; reflection-based semantic checks before generation |
| **v4.23** | Fail-fast audit; manifest defects block generation; 97 error codes |
| **v4.21** | Delegate binding automation for GAS abilities |
| **v4.20** | Event graph node placement; layered layout algorithm |

For complete version history, see [CHANGELOG.md](CHANGELOG.md).

---

## Narrative Pro Plugin (v2.2 Beta)

Location: `Plugins/NarrativePro22B57/`

### Modules

| Module | Type | Purpose |
|--------|------|---------|
| NarrativePro | Runtime | Core animation and logging |
| NarrativeArsenal | Runtime | Main gameplay systems (GAS, AI, Items, Dialogue, Quests) |
| NarrativeSaveSystem | Runtime | Save/load functionality |
| NarrativeCommonUI | Runtime | CommonUI widgets and input |
| NarrativeQuestEditor | Editor | Quest asset editor |
| NarrativeDialogueEditor | Editor | Dialogue asset editor |
| NarrativeArsenalEditor | Editor | Arsenal asset editors |

### Core Systems (in NarrativeArsenal)

- **Tales System** - Quest and Dialogue management via `UTalesComponent`
- **GAS Integration** - `NarrativeGameplayAbility`, `NarrativeAbilitySystemComponent`, damage/heal exec calcs
- **AI/NPCs** - `NarrativeNPCController`, NPC activities, goals, Mass Entity integration
- **Inventory** - `InventoryComponent`, equippable items, weapons (melee, ranged, magic)
- **Interaction** - `InteractionComponent`, `InteractableComponent`
- **Character Creator** - Appearance customization system
- **Vehicles** - Chaos vehicles integration with Mass traffic

### Key Classes

- `UTalesComponent` - Add to PlayerController for quest/dialogue functionality
- `UDialogue` - Dialogue asset with speakers, lines, camera shots
- `UQuest` - Quest asset with states, branches, tasks
- `ANarrativeCharacter` - Base character class
- `ANarrativeNPCCharacter` - NPC character with AI support
- `ANarrativePlayerCharacter` - Player character
- `UNarrativeGameplayAbility` - Base ability class for GAS

### Plugin Dependencies

GameplayAbilities, EnhancedInput, CommonUI, MotionWarping, MassGameplay, MassCrowd, MassAI, ChaosVehicles, PoseSearch, and more.

---

## Important Reference Patterns

### Gameplay Tag Hierarchies (CRITICAL)

**Authoritative Source:** `DefaultGameplayTags_FatherCompanion_v4_0.ini`

All Father-specific state tags use `Father.State.*` format (NOT `State.Father.*`):

| Tag | Purpose |
|-----|---------|
| `Father.State.Alive` | Required for form activation |
| `Father.State.Attached` | Father attached to player |
| `Father.State.Dormant` | Post-sacrifice dormant state |
| `Father.State.Transitioning` | Blocks form change during 5s VFX |
| `Father.State.SymbioteLocked` | Blocks form change during 30s Symbiote |
| `Father.State.Attacking` | Currently executing attack |
| `Father.State.Dashing` | Dash in progress |
| `Father.State.Deployed` | Engineer turret mode active |

**Separate `State.*` tags are Narrative Pro built-ins:**
- `State.Invulnerable` - Blocks all damage (Narrative Pro)
- `State.Invisible` - Character invisible (Narrative Pro)

**Other Father tag hierarchies (216 total in manifest):**
- `Ability.Father.*` (23 tags) - Ability identifiers
- `Cooldown.Father.*` (9 tags) - Cooldown tracking
- `Effect.Father.*` (31 tags) - Active effects
- `Father.State.*` (20 tags) - Father state tracking
- `Father.Form.*` (7 tags) - Form identifiers (5 main + Rifle/Sword weapon forms)
- `Father.Dome.*` (4 tags) - Dome system states
- `GameplayCue.Father.*` (14 tags) - VFX/SFX triggers
- `Data.*` (13 tags) - SetByCaller damage/duration values

### Replication Patterns (GAS Multiplayer)

**Net Execution Policy by Ability Owner:**

| Owner | Policy | Reason |
|-------|--------|--------|
| Player input abilities | **Local Predicted** | Responsive feel, client prediction |
| NPC/AI owned abilities | **Server Only** | AI runs on server, no prediction needed |
| Cross-actor grants | **Server Only** | Server authority for multi-actor ops |
| Visual-only abilities | **Server Initiated** | Visual feedback with server start |

**ASC Replication Mode:**

| Character Type | Mode | Reason |
|----------------|------|--------|
| Player | **Mixed** | Full state for owner, minimal for others |
| NPC (Father) | **Minimal** | Reduced bandwidth, server handles logic |

**Variable Replication Conditions:**

| Condition | Use For |
|-----------|---------|
| **Initial Only** | Set once at spawn (OwnerPlayer) |
| **None** (with RepNotify) | All clients must see (CurrentForm, IsAttached) |
| **Not Replicated** | GA-local variables (effect handles) |

### Parent Class Matrix

| Prefix | Asset Type | Parent Class |
|--------|------------|--------------|
| GA_* | Gameplay Abilities | NarrativeGameplayAbility |
| GA_FatherAttack | Melee Attack | GA_Melee_Unarmed (motion warping) |
| GE_* | Gameplay Effects | UGameplayEffect (Blueprint) |
| BP_* | Actor Blueprints | Actor, NarrativeNPCCharacter |
| WBP_* | Widget Blueprints | UserWidget |
| DBP_* | Dialogue Blueprints | NarrativeDialogue |
| EI_* | Equippable Items | EquippableItem |
| EI_* (weapons) | Weapon Items | RangedWeaponItem, MeleeWeaponItem |
| BPA_* | Activities | NarrativeActivityBase |
| Goal_* | Goal Items | NPCGoalItem |
| GoalGenerator_* | Goal Generators | NPCGoalGenerator |
| Quest_* | Quests | UQuest (Blueprint) |
| NE_* | Narrative Events | NarrativeEvent |
| GC_* (burst) | Gameplay Cues | GameplayCueNotify_Burst, _BurstLatent |
| GC_* (looping) | Gameplay Cues | AGameplayCueNotify_Actor |
| BTS_* | BT Services | BTService_BlueprintBase |
| NPC_* | NPC Definitions | UNPCDefinition (DataAsset) |
| CD_* | Character Definitions | UCharacterDefinition (DataAsset) |

### Manifest Validation (January 2026)

**Net Execution Policy - Validated Assignments:**

| Policy | Abilities (manifest.yaml) |
|--------|---------------------------|
| **ServerOnly** | GA_FatherCrawler, GA_FatherArmor, GA_FatherExoskeleton, GA_FatherSymbiote, GA_FatherEngineer, GA_FatherRifle, GA_FatherSword, GA_FatherAttack, GA_FatherLaserShot, GA_FatherMark, GA_FatherSacrifice, GA_TurretShoot, GA_FatherElectricTrap, GA_ProximityStrike, GA_CoreLaser |
| **LocalPredicted** | GA_FatherExoskeletonDash, GA_FatherExoskeletonSprint, GA_StealthField, GA_Backstab, GA_ProtectiveDome, GA_DomeBurst |
