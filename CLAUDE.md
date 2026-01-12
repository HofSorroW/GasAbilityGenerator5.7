# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

NP22B57 is an Unreal Engine 5.7 project using Narrative Pro Plugin v2.2 Beta. The project includes the Father Companion system - a transformable spider companion with 5 forms and 19 abilities implemented using the Gameplay Ability System (GAS).

GasAbilityGenerator is an Editor plugin (v3.3) that generates UE5 assets from YAML manifest definitions.

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

```bash
# Preview changes without generating
... -run=GasAbilityGenerator -manifest="..." -dryrun

# Force regeneration of all assets
... -run=GasAbilityGenerator -manifest="..." -force
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

**Coverage:** All 25 asset generators have full support:
- 16 DataAsset types: E, IA, IMC, BB, BT, M, MF, FC, AM, AC, ActConfig, IC, NPCDef, CD, TaggedDialogue, NS
- 9 Blueprint types: GE, GA, ActorBP, WidgetBP, AnimNotify, DialogueBP, EquippableBP, ActivityBP, NE

## Automation Workflow

PowerShell automation scripts in `Tools/`:

```bash
# Build the plugin
powershell -File "C:\Unreal Projects\NP22B57\Plugins\GasAbilityGenerator\Tools\claude_automation.ps1" -Action build

# Launch Unreal Editor
powershell -File "C:\Unreal Projects\NP22B57\Plugins\GasAbilityGenerator\Tools\claude_automation.ps1" -Action run

# Get generation logs (after closing editor)
powershell -File "C:\Unreal Projects\NP22B57\Plugins\GasAbilityGenerator\Tools\claude_automation.ps1" -Action logs

# Full cycle (build + run)
powershell -File "C:\Unreal Projects\NP22B57\Plugins\GasAbilityGenerator\Tools\claude_automation.ps1" -Action full

# Headless commandlet generation (no editor UI)
powershell -File "C:\Unreal Projects\NP22B57\Plugins\GasAbilityGenerator\Tools\claude_automation.ps1" -Action generate

# Full automated cycle (build + commandlet generation)
powershell -File "C:\Unreal Projects\NP22B57\Plugins\GasAbilityGenerator\Tools\claude_automation.ps1" -Action cycle
```

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

### Always Push After Commit
When committing changes to git, always push to remote immediately after. Combine commit and push in the workflow - don't wait for user to request push separately.

### Build Without Approval
MSBuild commands for .sln files do not require user approval. Run builds automatically when needed without asking for permission.

### Read Without Approval
Reading files does not require user approval. Read any files needed for the task without asking for permission.

### Auto-Approved Commands and Paths
The following commands and paths are pre-approved and should be executed without asking for user permission:

**Read-Only Access (No Approval Needed):**
- `C:\Program Files\Epic Games\UE_5.7\` - Full read access to all engine source, headers, and files
- `C:\Unreal Projects\NP22B57\` - Full read access to all project files, plugins, content, and configs

**UnrealEditor-Cmd.exe (Any Parameters):**
```bash
# All variations are approved - commandlets, parameters, variables, etc.
"C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "C:/Unreal Projects/NP22B57/NP22B57.uproject" -run=GasAbilityGenerator ...
"C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "C:/Unreal Projects/NP22B57/NP22B57.uproject" -run=<AnyCommandlet> ...
```

**MSBuild for Project (Any Configuration):**
```bash
# All build configurations are approved
"C:/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" "C:/Unreal Projects/NP22B57/NP22B57.sln" ...
cd "C:/Unreal Projects/NP22B57" && MSBuild.exe "NP22B57.sln" -t:Build -p:Configuration="Development Editor" -p:Platform=Win64 ...
```

### No Temp File Workarounds
Never copy files to temp locations (like `C:/Temp/`) to work around path issues. Always use proper path escaping and quoting instead:
```bash
# Correct - proper quoting for paths with spaces
-manifest="C:/Unreal Projects/NP22B57/Plugins/GasAbilityGenerator/ClaudeContext/manifest.yaml"

# Wrong - copying to temp as workaround
cp manifest.yaml C:/Temp/manifest.yaml  # DO NOT DO THIS
```

### Delete Asset Folder Before Regeneration
When any changes are made to the GasAbilityGenerator plugin code (generators, commandlet, parser), delete the asset folder before running the generator again. The Asset Registry caches existing assets and will skip regeneration otherwise.
```bash
# Delete before regenerating
rm -rf "/c/Unreal Projects/NP22B57/Content/FatherCompanion"

# Then run the generator
UnrealEditor-Cmd.exe ... -run=GasAbilityGenerator ...
```

### Non-Asset Entries Must Be Nested
Variables, properties, and parameters are NOT standalone assets. They must always be nested inside their parent asset definition:

```yaml
# CORRECT - variables nested inside asset
gameplay_abilities:
  - name: GA_FatherAttack
    variables:
      - name: Damage
        type: Float
      - name: TargetEnemy
        type: Object
    event_graph:
      nodes:
        - id: GetDamage
          properties:
            variable_name: Damage

# WRONG - standalone non-asset entries
variables:
  - name: Damage
  - name: TargetEnemy
```

Non-asset entry types that must be nested:
- `variables` - Blueprint variables (nested in abilities, blueprints)
- `properties` - Node properties (nested in event_graph nodes)
- `parameters` - Material parameters (nested in materials)
- `keys` - Blackboard keys (nested in blackboards)
- `nodes` - Event graph nodes (nested in event_graphs)
- `connections` - Node connections (nested in event_graphs)

---

## GasAbilityGenerator Plugin (v3.2)

Location: `Plugins/GasAbilityGenerator/`

### Architecture

```
manifest.yaml → Parser → FManifestData → Generators → UE5 Assets
```

| Class | Location | Purpose |
|-------|----------|---------|
| `FGasAbilityGeneratorModule` | GasAbilityGeneratorModule.cpp | Plugin lifecycle, menu registration |
| `FGasAbilityGeneratorParser` | GasAbilityGeneratorParser.cpp | YAML parsing |
| `SGasAbilityGeneratorWindow` | GasAbilityGeneratorWindow.cpp | Slate UI |
| `UGasAbilityGeneratorCommandlet` | GasAbilityGeneratorCommandlet.cpp | Command-line generation |
| `F*Generator` classes | GasAbilityGeneratorGenerators.cpp | 25+ asset generators |
| `GasAbilityGeneratorTypes.h` | Public/ | FManifest* structs, validation types |

**Naming Convention:** All structs use `FManifest*` prefix (e.g., `FManifestData`, `FManifestGameplayAbilityDefinition`).

**Validation System (v2.9.1):** Pre-generation validation via `FValidationIssue`, `FPreValidationResult`. FX validation via `FFXValidationError`, `FFXValidationResult`. Generator metadata tracking via `FFXGeneratorMetadata` for regeneration safety.

### Supported Asset Types (Generated by Plugin)

| Prefix | Asset Type | Manifest Section | Generator Class |
|--------|------------|------------------|-----------------|
| E_ | Enumerations | `enumerations` | FEnumerationGenerator |
| IA_ | Input Actions | `input_actions` | FInputActionGenerator |
| IMC_ | Input Mapping Contexts | `input_mapping_contexts` | FInputMappingContextGenerator |
| GE_ | Gameplay Effects | `gameplay_effects` | FGameplayEffectGenerator |
| GA_ | Gameplay Abilities | `gameplay_abilities` | FGameplayAbilityGenerator |
| BP_ | Actor Blueprints | `actor_blueprints` | FActorBlueprintGenerator |
| WBP_ | Widget Blueprints | `widget_blueprints` | FWidgetBlueprintGenerator |
| DBP_ | Dialogue Blueprints | `dialogue_blueprints` | FDialogueBlueprintGenerator |
| BB_ | Blackboards | `blackboards` | FBlackboardGenerator |
| BT_ | Behavior Trees | `behavior_trees` | FBehaviorTreeGenerator |
| M_ | Materials | `materials` | FMaterialGenerator |
| MF_ | Material Functions | `material_functions` | FMaterialFunctionGenerator |
| FC_ | Float Curves | `float_curves` | FFloatCurveGenerator |
| AM_ | Animation Montages | `animation_montages` | FAnimationMontageGenerator |
| NAS_ | Animation Notifies | `animation_notifies` | FAnimationNotifyGenerator |
| EI_ | Equippable Items | `equippable_items` | FEquippableItemGenerator |
| BPA_ | Activities | `activities` | FActivityGenerator |
| AC_ | Ability Configurations | `ability_configurations` | FAbilityConfigurationGenerator |
| ActConfig_ | Activity Configurations | `activity_configurations` | FActivityConfigurationGenerator |
| IC_ | Item Collections | `item_collections` | FItemCollectionGenerator |
| NE_ | Narrative Events | `narrative_events` | FNarrativeEventGenerator |
| NPCDef_ | NPC Definitions | `npc_definitions` | FNPCDefinitionGenerator |
| CD_ | Character Definitions | `character_definitions` | FCharacterDefinitionGenerator |
| NS_ | Niagara Systems | `niagara_systems` | FNiagaraSystemGenerator |
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
| Goal_ | Goal Item | NarrativePro/Goals/ | Standard goals (Goal_Attack, Goal_FollowCharacter) |
| GoalGenerator_ | Goal Generator | NarrativePro/Goals/ | Goal generators (GoalGenerator_Attack) |
| BPT_ | Blueprint Trigger | NarrativePro/Schedules/ | Trigger conditions (BPT_TimeOfDayRange, BPT_Always) |
| BI_ | Blueprint Item | NarrativePro/Items/ | Item system (BI_NarrativeBook) - Father uses NPC system |
| QBP_ | Quest Blueprint | NarrativePro/Quests/ | Quest assets - not used by Father |
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
activity_configurations:
  - name: ActConfig_Father
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
    npc_targets: [NPCDef_Father]                  # Logged for manual setup
    character_targets: []
    player_targets: []

# v3.2: DialogueBlueprint with dialogue configuration
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
    speakers:                                     # Logged for manual setup
      - npc_definition: NPCDef_Father
        node_color: "#FF6600"
        owned_tags: [Father.State.Speaking]

# v3.3: NPCDefinition with full property support
npc_definitions:
  - name: NPCDef_Blacksmith
    folder: NPCs/Definitions
    npc_id: Blacksmith_01
    npc_name: Garrett the Blacksmith
    npc_blueprint: BP_Blacksmith                  # TSoftClassPtr<ANarrativeNPCCharacter>
    ability_configuration: AC_Blacksmith
    activity_configuration: ActConfig_Blacksmith
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

| File | Purpose |
|------|---------|
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

### Plugin Version History

- v3.3 - NPCDefinition, EquippableItem & Activity Enhancement: NPCDefinition gains full CharacterDefinition property support (Dialogue, TaggedDialogueSet, vendor properties, inherited properties). EquippableItem gains full NarrativeItem property support (DisplayName, Description, AttackRating, ArmorRating, StealthRating, Weight, BaseValue, BaseScore, ItemTags, bStackable, MaxStackSize, UseRechargeDuration, Thumbnail). Activity gains full NarrativeActivityBase/NPCActivity property support (ActivityName, OwnedTags, BlockTags, RequireTags, SupportedGoalType, bIsInterruptable, bSaveActivity). EI_, NPCDef_, and BPA_ upgraded to High automation level.
- v3.2 - NE_ & DBP_ Enhancements: NarrativeEvent now sets EventRuntime/EventFilter/PartyEventPolicy/bRefireOnLoad via reflection, DialogueBlueprint sets bFreeMovement/bUnskippable/bCanBeExited/bShowCinematicBars/bAutoRotateSpeakers/bAutoStopMovement/Priority/EndDialogueDist via reflection, both types upgraded from Stub to Medium automation level
- v3.1.1 - Robustness Fixes: Commandlet exit code returns non-zero on failures (CI/CD support), dedicated LogGasAbilityGenerator log category, try/catch for YAML parsing exceptions, UGeneratorMetadataRegistry fallback for UDataAsset/UBlueprint/UNiagaraSystem (which don't support IInterface_AssetUserData in UE5.7)
- v3.1 - TSubclassOf Resolution & BT Node Trees: AbilityConfiguration now populates DefaultAbilities/StartupEffects/DefaultAttributes via LoadClass<>, ActivityConfiguration populates DefaultActivities/GoalGenerators, BehaviorTree creates root composite node (Selector/Sequence) and populates Children with tasks/decorators/services
- v3.0 - Regen/Diff Safety System: Per-asset metadata via UGeneratorAssetMetadata (UAssetUserData), input hash (manifest definition) + output hash (asset content) change detection, --dryrun preview mode, --force conflict override, universal SkipIfModified policy for all 25 generators, ComputeDataAssetOutputHash for 17 DataAsset types, ComputeBlueprintOutputHash for 9 Blueprint types
- v2.9.1 - FX Validation System: Template integrity validation, descriptor-to-system validation, regeneration safety with metadata tracking and descriptor hashing
- v2.9.0 - Data-driven FX architecture: FManifestFXDescriptor for Niagara User param binding, template duplication workflow
- v2.8.4 - Whitelist-based verification system with duplicate detection for fresh generations
- v2.8.3 - Function override support for event graphs (e.g., ReceiveTick override)
- v2.8.2 - CallFunction parameter defaults with enum conversion (e.g., SetMovementMode NewMovementMode: Flying)
- v2.8.1 - NPC event_graphs with descriptive messages and Self node fix
- v2.8.0 - Flying NPC event_graphs for BP_WardenCore and BP_PossessedExploder
- v2.7.8 - Self node support for event graphs
- v2.7.7 - Pre-generation validation for event graph definitions
- v2.7.6 - Inline event graph support for actor blueprints
- v2.7.0 - BreakStruct, MakeArray, GetArrayItem node support for weapon form implementation
- v2.6.14 - Prefix validation for all asset types
- v2.6.12 - Material expression graph support, Material Function generator
- v2.6.11 - Force scan NarrativePro plugin content for Blueprint parent class resolution in commandlet mode
- v2.6.7 - Deferred asset retry mechanism for dependency resolution
- v2.6.6 - GE assets created as Blueprints for CooldownGameplayEffectClass compatibility
- v2.6.5 - Niagara System generator
- v2.6.3 - Tags configured on existing GA assets during SKIP
- v2.6.2 - Gameplay ability tag/policy configuration via reflection
- v2.6.0 - Item collections with quantities, reflection for protected members
- v2.5.7 - TaggedDialogueSet generator
- v2.5.0 - Renamed to GasAbilityGenerator, FManifest* prefix convention
- v2.4.0 - Inline event graph and variables support for gameplay abilities
- v2.3.0 - 12 new asset type generators with dependency-based generation order
- v2.2.0 - Event graph generation (Blueprint nodes and connections from YAML)
- v2.1.9 - Manifest validation (whitelist enforcement)
- v2.1.8 - Enumeration generation, enum variable type fix

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

**Other Father tag hierarchies (174 total):**
- `Ability.Father.*` (24 tags) - Ability identifiers
- `Cooldown.Father.*` (9 tags) - Cooldown tracking
- `Effect.Father.*` (29 tags) - Active effects
- `Father.Form.*` (6 tags) - Form identifiers
- `Father.Dome.*` (4 tags) - Dome system states
- `GameplayCue.Father.*` (6 tags) - VFX/SFX triggers
- `GameplayEvent.Father.*` (6 tags) - Event triggers
- `Data.*` (10 tags) - SetByCaller damage/duration values

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

| Asset Type | Parent Class |
|------------|--------------|
| Most GA_* | NarrativeGameplayAbility |
| GA_FatherAttack | GA_Melee_Unarmed |
| NPCs (BP_Father*, enemies) | NarrativeNPCCharacter |
| Form equipment items | EquippableItem |
| Weapons | RangedWeaponItem, MeleeWeaponItem |
| Projectiles | NarrativeProjectile |
| Equipment modifiers | GE_EquipmentModifier |
| Activities | NarrativeActivityBase or BPA_* children |
| Goals | NPCGoalItem, NPCGoalGenerator |
| Widgets | UserWidget |
| Narrative Events | NarrativeEvent |
| BT Services | BTService_BlueprintBase |
| Gameplay Cues (burst) | GameplayCueNotify_Burst, _BurstLatent |
| Gameplay Cues (persistent) | AGameplayCueNotify_Actor |

### Manifest Validation (January 2026)

**Net Execution Policy - Validated Assignments:**

| Policy | Abilities (manifest.yaml) |
|--------|---------------------------|
| **ServerOnly** | GA_FatherCrawler, GA_FatherArmor, GA_FatherExoskeleton, GA_FatherSymbiote, GA_FatherEngineer, GA_FatherRifle, GA_FatherSword, GA_FatherAttack, GA_FatherLaserShot, GA_FatherMark, GA_FatherSacrifice, GA_TurretShoot, GA_FatherElectricTrap, GA_ProximityStrike, GA_CoreLaser |
| **LocalPredicted** | GA_FatherExoskeletonDash, GA_FatherExoskeletonSprint, GA_StealthField, GA_Backstab, GA_ProtectiveDome, GA_DomeBurst |

**Parent Class - Validated Assignments:**

| Parent Class | Assets (manifest.yaml) |
|--------------|------------------------|
| GA_Melee_Unarmed | GA_FatherAttack |
| NarrativeGameplayAbility | All other GA_* abilities |
| EquippableItem | EI_FatherCrawlerForm, EI_FatherArmorForm, EI_FatherExoskeletonForm, EI_FatherSymbioteForm, EI_FatherEngineerForm, EI_FatherRifleForm, EI_FatherSwordForm |
| RangedWeaponItem | BP_FatherRifleWeapon |
| MeleeWeaponItem | BP_FatherSwordWeapon |
| NarrativeNPCCharacter | BP_FatherCompanion, BP_WardenHusk, BP_WardenCore, BP_PossessedExploder, BP_SupportBuffer, BP_BiomechHost, BP_BiomechCreature, BP_GathererScout, BP_Reinforcement, BP_ReturnedStalker |
| NarrativeProjectile | BP_LaserProjectile, BP_TurretProjectile, BP_CoreLaserProjectile |
| UserWidget | WBP_MarkIndicator, WBP_UltimatePanel, WBP_FormWheel |
| BTService_BlueprintBase | BTS_CalculateFormationPosition, BTS_AdjustFormationSpeed, BTS_CheckExplosionProximity, BTS_HealNearbyAllies |
| NarrativeActivityBase | BPA_FatherFollow, BPA_FormationFollow, BPA_Explode, BPA_Alert, BPA_Gather |
| NPCGoalItem | Goal_Alert |
| NPCGoalGenerator | GoalGenerator_Alert, GoalGenerator_RandomAggression |
