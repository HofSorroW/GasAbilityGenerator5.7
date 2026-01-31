# Generator System Master Reference v7.8.55

Consolidated reference for GasAbilityGenerator systems, Narrative Pro asset coverage, and CDO property audit.

**Last Updated:** January 31, 2026
**Version:** v7.8.55
**Build Status:** SUCCESS (205 assets, 0 failures)

---

## Table of Contents

1. [Generator Status Summary](#generator-status-summary)
2. [Narrative Pro Core Assets](#narrative-pro-core-assets)
3. [Item System](#item-system)
4. [Goal System](#goal-system)
5. [Gameplay Cue System](#gameplay-cue-system)
6. [Behavior Tree System](#behavior-tree-system)
7. [Blueprint Trigger System](#blueprint-trigger-system)
8. [Blueprint Activity System](#blueprint-activity-system)
9. [Blueprint Condition System](#blueprint-condition-system)
10. [Gameplay Effect Enhancements](#gameplay-effect-enhancements)
11. [Gameplay Ability Enhancements](#gameplay-ability-enhancements)
12. [Narrative Event Enhancements](#narrative-event-enhancements)
13. [CDO Audit Status](#cdo-audit-status)
14. [Implementation Details](#implementation-details)
15. [Troubleshooting](#troubleshooting)

---

## Generator Status Summary

### Complete Generators (All CDO Properties Verified)

| Generator | Prefix | Status | Properties |
|-----------|--------|--------|------------|
| AbilityConfiguration | AC_ | COMPLETE | All |
| ActivityConfiguration | AC_ | COMPLETE | All |
| Activity | BPA_ | COMPLETE | All + v7.8.52 enhancements |
| BlueprintCondition | BPC_ | **NEW v7.8.52** | Full UNarrativeCondition support |
| ComponentBlueprint | BPC_ | FIXED | Added bAutoActivate |
| CharacterDefinition | CD_ | COMPLETE | All |
| DialogueBlueprint | DBP_ | COMPLETE | All |
| EquippableItem | EI_ | COMPLETE | Stats, Attachments, Weapon |
| GameplayAbility | GA_ | **FIXED v7.8.55** | +7 properties (CostGE, 5 tag containers, NetSecurityPolicy) |
| GameplayCue | GC_ | COMPLETE | Full GCN Effects |
| GoalItem | Goal_ | COMPLETE | All |
| GoalGenerator | GoalGenerator_ | COMPLETE | All |
| NPCDefinition | NPC_ | COMPLETE | All |
| Schedule | Schedule_ | FIXED | Added bReselect |
| TaggedDialogueSets | TDS_ | COMPLETE | All |

### Generators with Remaining Gaps

| Generator | Prefix | Gaps | Priority |
|-----------|--------|------|----------|
| GameplayEffect | GE_ | 4 (TagRequirements, GrantedAbilities) | HIGH |
| NarrativeEvent | NE_ | 1 (Conditions array) | MEDIUM |
| Quest | Quest_ | 1 (InheritableStates) | MEDIUM |

---

## Narrative Pro Core Assets

**Path:** `Plugins/NarrativePro22B57/Content/Pro/Core/`
**Total Assets:** 4,823

### Folder Structure

| Folder | Count | Purpose |
|--------|-------|---------|
| Abilities/ | 111 | GameplayAbilities, GameplayEffects, Cues, Configurations |
| AI/ | 143 | Activities, BehaviorTrees, Goals, Services, EQS |
| Audio/ | 466 | Sound files, Music, Foley |
| BP/ | 237 | Actor Blueprints, Materials |
| Character/ | 1,700+ | Animations, Skeletons, Materials |
| CharCreator/ | 124 | Character creator UI/data |
| Data/ | 58 | Input Actions, Perks, Profiles |
| Inventory/ | 53 | Items, Books |
| Tales/ | 91 | Dialogues, Events, Tasks, Triggers |
| UI/ | 759 | Widgets, Text Styles, Buttons |
| VFX/ | 303 | Niagara Systems, Materials |
| Weapons/ | 193 | Weapon models, animations, FX |

### Generator-Supported Asset Types

| Prefix | Count | Type | Generator Status |
|--------|-------|------|------------------|
| GA_ | 61 | Gameplay Abilities | v7.8.55 Complete |
| GE_ | 33 | Gameplay Effects | v7.8.52 (4 gaps) |
| GC_ | 10 | Gameplay Cues | v7.8.52 Complete |
| AC_ | 13 | Ability/Activity Configs | Complete |
| BPA_ | 17 | NPC Activities | v7.8.52 Complete |
| Goal_ | 8 | Goal Items | v7.8.52 Complete |
| BB_ | 11 | Blackboards | Complete |
| BT_ | 15 | Behavior Trees | Complete |
| NPC_ | 8 | NPC Definitions | Complete |
| NE_ | 12 | Narrative Events | v7.8.52 (1 gap) |
| IA_ | 27 | Input Actions | Complete |
| BP_ | 90 | Actor Blueprints | Complete |
| WBP_ | 96 | Widget Blueprints | Complete |
| BPT_ | 22 | Blueprint Triggers | v7.8.52 Complete |
| BPC_ | 9 | Blueprint Conditions | **v7.8.52 NEW** |

### Reference-Only Asset Types (Not Generated)

| Prefix | Count | Reason |
|--------|-------|--------|
| BTS_ | 16 | Abstract base classes |
| BTTask_ | 8 | Abstract base classes |
| M_ | 1,100+ | Art materials |
| T_ | 900+ | Textures |
| A_ | 152 | Animations (external) |
| AM_ | 122 | Animation Montages |
| NS_ | 25 | Niagara Systems |
| S_ | 215+ | Sound Effects |

---

## Item System

### Stats (FNarrativeItemStat)

Narrative Pro uses dynamic variable binding for item stats.

```cpp
USTRUCT(BlueprintType)
struct FNarrativeItemStat
{
    FText StatDisplayName;      // "Damage", "Weight", etc.
    FString StringVariable;     // Variable name for GetStringVariable() binding
    FText StatTooltip;          // Tooltip shown on hover
};
```

**YAML Syntax:**
```yaml
stats:
  - stat_display_name: Damage
    string_variable: Damage
    stat_tooltip: "The base damage this weapon deals."
```

### Weapon Attachment Configs

TMap<FGameplayTag, FWeaponAttachmentConfig> for holster/wield positions.

**YAML Syntax:**
```yaml
holster_attachment_configs:
  Narrative.Equipment.Slot.Weapon.BackA:
    socket_name: Socket_BackA
    location: [-0.82, -9.97, 28.73]
    rotation: [179.96, 0.0, 179.99]
    scale: [1.0, 1.0, 1.0]

wield_attachment_configs:
  Narrative.Equipment.WieldSlot.Mainhand:
    socket_name: weapon_r
    location: [0, 0, 0]
    rotation: [-90, 0, 0]
```

### Equippable Item Properties

| Property | Type | Description |
|----------|------|-------------|
| `display_name` | FText | Item name |
| `description` | FText | Item description |
| `equipment_slot` | FGameplayTag | Slot tag |
| `attack_rating` | float | Bonus attack damage |
| `armor_rating` | float | Damage reduction |
| `weapon_visual_class` | TSoftClassPtr | AWeaponVisual class |
| `weapon_hand` | Enum | TwoHanded, MainHand, OffHand |
| `attack_damage` | float | Base damage |
| `clip_size` | int32 | Magazine capacity |

---

## Goal System

### Goal Items (UNPCGoalItem)

```cpp
UCLASS(Blueprintable, BlueprintType)
class UNPCGoalItem : public UObject
{
    float DefaultScore = 50.0f;         // Priority when active
    float GoalLifetime = -1.0f;         // -1 = never expires
    bool bRemoveOnSucceeded = false;    // Remove after completion
    bool bSaveGoal = false;             // Persist across saves
    FGameplayTagContainer OwnedTags;    // Tags granted while active
    FGameplayTagContainer BlockTags;    // Tags that block this goal
    FGameplayTagContainer RequireTags;  // Tags required to pursue goal
};
```

**YAML Syntax:**
```yaml
goal_items:
  - name: Goal_DefendArea
    folder: AI/Goals
    default_score: 75.0
    goal_lifetime: -1.0
    remove_on_succeeded: false
    save_goal: true
    owned_tags: [State.Defending]
    block_tags: [State.Fleeing]
    require_tags: [State.Alive]
    variables:
      - name: DefenseRadius
        type: Float
        default_value: "500.0"
    event_graph: EventGraph_DefendArea
```

### Goal Generators (UNPCGoalGenerator)

```cpp
UCLASS(Blueprintable, BlueprintType)
class UNPCGoalGenerator : public UObject
{
    bool bSaveGoalGenerator = false;    // Persist across saves
};
```

**YAML Syntax:**
```yaml
goal_generators:
  - name: GoalGenerator_Attack
    folder: AI/GoalGenerators
    save_goal_generator: false
    variables:
      - name: QueryTemplate
        type: Object
        class: EnvQuery
      - name: AttackGoalClass
        type: Class
        class: NPCGoalItem
    event_graph: EventGraph_AttackGenerator
```

---

## Gameplay Cue System

### Parent Classes

| Parent Class | Duration | Use Case |
|--------------|----------|----------|
| `GameplayCueNotify_Burst` | Instant | One-shot VFX |
| `GameplayCueNotify_BurstLatent` | Duration | Timed VFX (DOTs, buffs) |
| `AGameplayCueNotify_Actor` | Looping | Persistent VFX (auras) |

### GCN Effects System

**YAML Syntax:**
```yaml
gameplay_cues:
  - name: GC_TakeDamage
    folder: VFX/GameplayCues
    parent_class: GameplayCueNotify_BurstLatent
    gameplay_cue_tag: GameplayCue.TakeDamage
    auto_attach_to_owner: true

    gcn_defaults:
      default_spawn_condition:
        locally_controlled_source: true
        chance_to_play: 1.0
      default_placement_info:
        socket_name: FX_Hit
        attach_policy: AttachToTarget

    gcn_effects:
      burst_particles:
        - niagara_system: /Game/VFX/NS_BloodSplat
          spawn_condition:
            locally_controlled_source: true
          placement:
            socket_name: FX_Hit
            attach_policy: AttachToTarget

      burst_sounds:
        - sound: /Game/Audio/SFX_FleshHit
          volume_multiplier: 1.0

      burst_camera_shake:
        camera_shake_class: /Game/VFX/CS_LightHit
        shake_scale: 0.5

      burst_decal:
        decal_material: /Game/Materials/M_Blood
        decal_size: [50, 50, 50]
        fade_out_start_delay: 10.0
        fade_out_duration: 5.0
```

---

## Behavior Tree System

### BT Services (BTS_)

**Parent Class:** `BTService_BlueprintBase`

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `interval` | float | 0.5 | Tick interval |
| `random_deviation` | float | 0.1 | Random variance |
| `call_tick_on_search_start` | bool | true | Tick on BT search start |
| `restart_timer_on_each_activation` | bool | false | Reset timer on reactivation |

**YAML Syntax:**
```yaml
bt_services:
  - name: BTS_AdjustFollowSpeed
    folder: AI/Services
    interval: 0.5
    random_deviation: 0.1
    node_name: "Adjust Follow Speed"
    variables:
      - name: TargetDistance
        type: Float
    event_graph: EventGraph_AdjustFollowSpeed
```

### BT Tasks (BTTask_)

**Parent Class:** `BTTask_BlueprintBase`

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `tick_interval` | float | -1.0 | -1 = no tick |
| `ignore_restart_self` | bool | false | Ignore restart requests |

**YAML Syntax:**
```yaml
bt_tasks:
  - name: BTTask_ActivateAbility
    folder: AI/Tasks
    tick_interval: -1.0
    node_name: "Activate Ability"
    variables:
      - name: AbilityClass
        type: Class
        class: GameplayAbility
    event_graph: EventGraph_ActivateAbility
```

---

## Blueprint Trigger System

### BPT_ Task Types (22 types)

| Class | Purpose | Key Properties |
|-------|---------|----------------|
| `BPT_FindItem` | Find/acquire item | ItemClass, Quantity |
| `BPT_GoToLocation` | Navigate to location | LocationName, Radius |
| `BPT_FinishDialogue` | Complete dialogue | DialogueClass, NodeID |
| `BPT_KillNPC` | Kill NPC | NPCDefinition, Quantity |
| `BPT_TimeOfDayRange` | Wait for time range | StartTime, EndTime |

**YAML Syntax:**
```yaml
blueprint_triggers:
  - name: BPT_CollectOre
    folder: Quests/Tasks
    parent_class: BPT_FindItem
    node_name: "Collect Iron Ore"
    variables:
      - name: TargetQuantity
        type: Int
        default_value: "10"
```

---

## Blueprint Activity System

### Enhanced Activity Properties (v7.8.52)

| Property | Type | Description |
|----------|------|-------------|
| `attack_tagged_dialogue` | FGameplayTag | Tagged dialogue during attack |
| `weapon_types` | TArray<TSubclassOf> | Allowed weapon types |
| `follow_goal` | TSubclassOf | Goal class for following |
| `interact_sub_goal` | TSubclassOf | Sub-goal for interactions |
| `bb_key_follow_distance` | FName | Blackboard key |

**YAML Syntax:**
```yaml
activities:
  - name: BPA_CustomAttack
    folder: AI/Activities
    parent_class: NarrativeActivityBase
    behavior_tree: BT_AttackBehavior
    attack_tagged_dialogue: Dialogue.Combat.Attack
    weapon_types: [MeleeWeaponItem]
    owned_tags: [State.Attacking]
    block_tags: [State.Dead]
    require_tags: [State.Alive]
```

---

## Blueprint Condition System

### NEW v7.8.52: BPC_ Blueprint Condition Generator

Generates UNarrativeCondition subclass Blueprints for dialogue/quest conditions.

**Existing Narrative Pro BPC_ Classes:**
- BPC_DifficultyCheck
- BPC_HasFollowGoalFor
- BPC_HasForm
- BPC_HasItem
- BPC_HasPerk
- BPC_IsItemEquipped
- BPC_LevelCheck

**YAML Syntax:**
```yaml
blueprint_conditions:
  - name: BPC_HasCustomTag
    folder: Conditions
    parent_class: NarrativeCondition
    not: false
    condition_filter: AnyCharacter    # DontTarget, AnyCharacter, OnlyNPCs, OnlyPlayers
    party_policy: AnyPlayerPasses     # AnyPlayerPasses, PartyPasses, AllPlayersPass

    npc_targets:
      - NPC_Father

    properties:
      GameplayTag: "Father.State.Alive"

    variables:
      - name: TagToCheck
        type: GameplayTag
        instance_editable: true
```

---

## Gameplay Effect Enhancements

### Execution Calculations

```yaml
gameplay_effects:
  - name: GE_LaserDamage
    duration_policy: Instant
    executions:
      - calculation_class: NarrativeDamageExecCalc
        modifiers:
          - captured_attribute: NarrativeAttributeSetBase.AttackDamage
            captured_source: Source
            captured_status: Snapshotted
            modifier_op: Multiplicative
            magnitude_value: 2.0
        conditional_effects:
          - effect_class: GE_CriticalDamage
            required_source_tags: [State.CriticalStrike]
```

### Attribute-Based Magnitude

```yaml
modifiers:
  - attribute: NarrativeAttributeSetBase.Stamina
    operation: Additive
    magnitude_type: AttributeBased
    attribute_based:
      coefficient: 1.0
      backing_attribute: NarrativeAttributeSetBase.StaminaRegenRate
      attribute_source: Source
      snapshot: false
      calculation_type: AttributeMagnitude
```

### Per-Modifier Tag Requirements

```yaml
modifiers:
  - attribute: NarrativeAttributeSetBase.AttackDamage
    operation: Additive
    magnitude_value: 25.0
    target_tags:
      must_have_tags: [State.Fleeing]
      must_not_have_tags: [State.Invulnerable]
```

---

## Gameplay Ability Enhancements

### v7.8.55 New Properties (7 Total)

| Property | Type | YAML Field |
|----------|------|------------|
| CostGameplayEffectClass | TSubclassOf | `cost_gameplay_effect` |
| BlockAbilitiesWithTag | FGameplayTagContainer | `block_abilities_with_tag` |
| SourceRequiredTags | FGameplayTagContainer | `source_required_tags` |
| SourceBlockedTags | FGameplayTagContainer | `source_blocked_tags` |
| TargetRequiredTags | FGameplayTagContainer | `target_required_tags` |
| TargetBlockedTags | FGameplayTagContainer | `target_blocked_tags` |
| NetSecurityPolicy | Enum | `net_security_policy` |

**YAML Syntax:**
```yaml
gameplay_abilities:
  - name: GA_Example
    cost_gameplay_effect: GE_ManaCost
    net_security_policy: ServerOnlyExecution
    tags:
      ability_tags: [Ability.Attack]
      block_abilities_with_tag: [Ability.Exclusive]
      source_required_tags: [State.Alive]
      source_blocked_tags: [State.Stunned]
      target_required_tags: [State.Alive]
      target_blocked_tags: [State.Invulnerable]
```

### Ability Triggers

```yaml
ability_triggers:
  - trigger_tag: Event.Damage.Received
    trigger_source: GameplayEvent
  - trigger_tag: State.LowHealth
    trigger_source: OwnedTagAdded
```

### Firearm Config

| Property | Default | Description |
|----------|---------|-------------|
| `is_automatic` | false | Hold to continuously fire |
| `rate_of_fire` | 0.2 | Seconds between shots |
| `burst_amount` | -1 | Shots per burst (-1 = not burst) |
| `trace_distance` | 10000.0 | Max range |
| `trace_radius` | 0.0 | Sphere trace radius |

### Motion Warping Config

| Property | Default | Description |
|----------|---------|-------------|
| `should_warp` | false | Enable motion warping |
| `warp_maintain_dist` | 100.0 | Distance to maintain |
| `min_warp_dist` | 20.0 | Minimum warp |
| `max_warp_dist` | 250.0 | Maximum warp |

---

## Narrative Event Enhancements

### Variables Support

```yaml
narrative_events:
  - name: NE_GiveReward
    folder: Events
    event_runtime: Start
    event_filter: OnlyPlayers
    variables:
      - name: RewardAmount
        type: Int
        default_value: "100"
        instance_editable: true
    event_graph: NE_GiveReward_EventGraph
```

### Override Functions

| Function | Parameters | Purpose |
|----------|------------|---------|
| `ExecuteEvent` | Target, Controller, NarrativeComponent | Main execution |
| `OnActivate` | - | Called on activation |
| `OnDeactivate` | - | Called on deactivation |

### Condition Types (19 types)

| Category | Conditions |
|----------|------------|
| Quests | IsQuestAtState, IsQuestInProgress, IsQuestSucceeded, IsQuestFailed |
| Inventory | HasItemInInventory, IsItemEquipped |
| Time | IsDayTime, IsTimeInRange |
| UtilityAI | HasFollowGoal, HasInteractGoal |
| General | CheckDifficulty, CheckLevel, HasCompletedDataTask |

---

## CDO Audit Status

### Completed Fixes

| Fix | Version | Description |
|-----|---------|-------------|
| Schedule_.bReselect | v7.8.52 | Triggers activity reselection |
| BPC_.bAutoActivate | v7.8.52 | Component auto-activation |
| BPC_ Generator | v7.8.52 | NEW: UNarrativeCondition support |
| GA_ 7 properties | v7.8.55 | CostGE, 5 tag containers, NetSecurityPolicy |

### Remaining Gaps (Future Work)

| Generator | Gap | Priority |
|-----------|-----|----------|
| GE_ | ApplicationTagRequirements | HIGH |
| GE_ | RemovalTagRequirements | HIGH |
| GE_ | OngoingTagRequirements | HIGH |
| GE_ | GrantedAbilities | HIGH |
| NE_ | Conditions array | MEDIUM |
| Quest_ | InheritableStates | MEDIUM |

---

## Implementation Details

### Parser State Machine

```
goal_items:
  ├── - name: Goal_X          → Create new FManifestGoalItemDefinition
  │   ├── folder:             → CurrentDef.Folder
  │   ├── default_score:      → CurrentDef.DefaultScore
  │   ├── variables:          → bInVariables = true
  │   │   └── - name: Var     → CurrentVariable.Name
  │   └── event_graph:        → CurrentDef.EventGraph
```

### Generator Pipeline

1. **Validation**: Check whitelist, compute hash, check metadata
2. **Parent Class**: Find parent in Narrative Pro
3. **Blueprint Creation**: Factory creates Blueprint
4. **Variables**: Add via `FBlueprintEditorUtils::AddMemberVariable()`
5. **First Compile**: Compile after variables added
6. **CDO Setup**: Set base class properties
7. **Event Graph**: Generate via FEventGraphGenerator
8. **Final Compile**: Contract 10 compile gate
9. **Save & Cache**: Save package, cache for same-session resolution

### Variable Types Supported

| Type | YAML | UE Type |
|------|------|---------|
| Bool | `type: Bool` | bool |
| Int | `type: Int` | int32 |
| Float | `type: Float` | float |
| Object | `type: Object` + `class: X` | UObject* |
| Class | `type: Class` + `class: X` | TSubclassOf<X> |
| Array | `container: Array` | TArray<T> |
| Map | `container: Map` | TMap<K,V> |

---

## Troubleshooting

### Common Build Errors

| Error | Cause | Fix |
|-------|-------|-----|
| C2011: struct redefinition | Duplicate struct definitions | Remove older version |
| C2039: 'X' is not a member | Old code references removed field | Update function |
| C4293: shift count too large | GetTypeHash() returns uint32 | Cast to uint64 before shift |

### Validation Commands

```bash
# Quick build check
powershell -ExecutionPolicy Bypass -File "Tools/claude_automation.ps1" -Action build

# Full cycle (build + headless generation)
powershell -ExecutionPolicy Bypass -File "Tools/claude_automation.ps1" -Action cycle

# Check generation logs
powershell -ExecutionPolicy Bypass -File "Tools/claude_automation.ps1" -Action logs
```

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| v7.8.55 | 2026-01-31 | GA_ 7 new properties (CostGE, tag containers, NetSecurityPolicy) |
| v7.8.54 | 2026-01-31 | BPC_ Blueprint Condition generator, Schedule bReselect, Component bAutoActivate |
| v7.8.53 | 2026-01-31 | BPT generator complete, Goal_ CDO verification |
| v7.8.52 | 2026-01-31 | Item stats, Gameplay Cues, BT Services/Tasks, GE/GA/NE enhancements |

---

**Document Version:** v7.8.55
**Build Verified:** SUCCESS (205 assets, 0 failures)
**Author:** Claude (Anthropic)
