# Item, Goal, Gameplay Cue, BT, BPT, BPA, GE, GA & NE Enhancement Reference v7.8.52

Complete reference for the GasAbilityGenerator's Item, Goal, Gameplay Cue, Behavior Tree, Blueprint Triggers, Blueprint Activities, Gameplay Effect, Gameplay Ability, and Narrative Event enhancement systems, aligned with Narrative Pro's actual structure.

---

## Table of Contents

1. [Item System](#item-system)
   - [Stats (FNarrativeItemStat)](#stats-fnarrativeitemstat)
   - [Weapon Attachment Configs](#weapon-attachment-configs)
   - [Equippable Item Properties](#equippable-item-properties)
2. [Goal System](#goal-system)
   - [Goal Items (UNPCGoalItem)](#goal-items-unpcgoalitem)
   - [Goal Generators (UNPCGoalGenerator)](#goal-generators-unpcgoalgenerator)
3. [Gameplay Cue System](#gameplay-cue-system)
   - [Parent Classes](#parent-classes)
   - [GCN Effects System](#gcn-effects-system)
   - [Spawn Condition Override](#spawn-condition-override)
   - [Placement Info Override](#placement-info-override)
   - [Custom Functions](#custom-functions)
4. [Behavior Tree System](#behavior-tree-system)
   - [BT Services (BTS_)](#bt-services-bts_)
   - [BT Tasks (BTTask_)](#bt-tasks-bttask_)
5. [Blueprint Trigger System](#blueprint-trigger-system)
   - [BPT_ Task Types](#bpt_-task-types)
   - [Common BPT Properties](#common-bpt-properties)
   - [Location-Based Tasks](#location-based-tasks)
   - [Time-Based Tasks](#time-based-tasks)
6. [Blueprint Activity System](#blueprint-activity-system)
   - [BPA_ Activity Types](#bpa_-activity-types)
   - [Enhanced Activity Properties](#enhanced-activity-properties)
   - [Attack Activity Config](#attack-activity-config)
   - [Follow Activity Config](#follow-activity-config)
7. [Gameplay Effect Enhancements](#gameplay-effect-enhancements)
   - [Execution Calculations](#execution-calculations)
   - [Attribute-Based Magnitude](#attribute-based-magnitude)
   - [Per-Modifier Tag Requirements](#per-modifier-tag-requirements)
   - [Conditional Gameplay Effects](#conditional-gameplay-effects)
   - [Periodic Inhibition Policy](#periodic-inhibition-policy)
8. [Gameplay Ability Enhancements](#gameplay-ability-enhancements)
   - [Ability Triggers](#ability-triggers)
   - [Cues Section](#cues-section)
   - [Damage Section](#damage-section)
   - [Firearm Ability Config](#firearm-ability-config)
   - [Motion Warping Config](#motion-warping-config)
   - [Execution Config](#execution-config)
9. [Narrative Event Enhancements](#narrative-event-enhancements)
   - [Variables Support](#ne-variables-support)
   - [EventGraph Support](#ne-eventgraph-support)
   - [Override Functions](#ne-override-functions)
   - [Conditions](#ne-conditions)
10. [YAML Syntax Reference](#yaml-syntax-reference)
11. [Implementation Details](#implementation-details)
12. [Narrative Pro Header Analysis](#narrative-pro-header-analysis)

---

## Item System

### Stats (FNarrativeItemStat)

Narrative Pro uses dynamic variable binding for item stats, not static values.

**Narrative Pro Structure (from NarrativeItem.h):**
```cpp
USTRUCT(BlueprintType)
struct FNarrativeItemStat
{
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText StatDisplayName;      // "Damage", "Weight", etc.

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString StringVariable;     // Variable name for GetStringVariable() binding

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText StatTooltip;          // Tooltip shown on hover
};
```

**How It Works:**
1. UI calls `GetStringVariable(StringVariable)` on the item at runtime
2. Item returns current value as string (e.g., "50" for damage)
3. This allows dynamic values that change based on item state/buffs

**YAML Syntax:**
```yaml
stats:
  - stat_display_name: Damage
    string_variable: Damage
    stat_tooltip: "The base damage this weapon deals."
  - stat_display_name: Weight
    string_variable: Weight
    stat_tooltip: "The weight of the item in kg."
```

**Generator Output:**
- Sets `StatDisplayName` (FText)
- Sets `StringVariable` (FString)
- Sets `StatTooltip` (FText)

---

### Weapon Attachment Configs

Narrative Pro uses TMap<FGameplayTag, FWeaponAttachmentConfig> for holster/wield positions.

**Narrative Pro Structure (from WeaponItem.h):**
```cpp
USTRUCT(BlueprintType)
struct FWeaponAttachmentConfig
{
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SocketName;           // Socket on skeletal mesh

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FTransform Offset;          // Location, Rotation, Scale
};

// In UWeaponItem:
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
TMap<FGameplayTag, FWeaponAttachmentConfig> HolsterAttachmentConfigs;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
TMap<FGameplayTag, FWeaponAttachmentConfig> WieldAttachmentConfigs;
```

**YAML Syntax (TMap-style):**
```yaml
holster_attachment_configs:
  Narrative.Equipment.Slot.Weapon.BackA:
    socket_name: Socket_BackA
    location: [-0.82, -9.97, 28.73]
    rotation: [179.96, 0.0, 179.99]
    scale: [1.0, 1.0, 1.0]
  Narrative.Equipment.Slot.Weapon.BackB:
    socket_name: Socket_BackB
    location: [-2.85, 0.03, 22.69]
    rotation: [179.94, 0.91, 179.97]

wield_attachment_configs:
  Narrative.Equipment.WieldSlot.Mainhand:
    socket_name: weapon_r
    location: [0, 0, 0]
    rotation: [-90, 0, 0]
  Narrative.Equipment.WieldSlot.Offhand:
    socket_name: weapon_l
    location: [0, 0, 0]
    rotation: [-90, 0, 0]
```

**Key Points:**
- GameplayTag is the map key (slot identifier)
- SocketName is FName for mesh socket
- Location/Rotation/Scale form the FTransform Offset
- Scale defaults to [1.0, 1.0, 1.0] if not specified

---

### Equippable Item Properties

Full property list for `equippable_items:` section.

**Base Properties (UNarrativeItem):**
| Property | Type | Description |
|----------|------|-------------|
| `display_name` | FText | Item name shown in UI |
| `description` | FText | Item description |
| `thumbnail` | Texture2D path | Icon for inventory |
| `weight` | float | Item weight |
| `base_value` | int32 | Gold value |
| `base_score` | float | AI priority score |
| `stackable` | bool | Can stack in inventory |
| `max_stack_size` | int32 | Max stack count |
| `item_tags` | FGameplayTagContainer | Categorization tags |

**Equippable Properties (UEquippableItem):**
| Property | Type | Description |
|----------|------|-------------|
| `equipment_slot` | FGameplayTag | Slot tag (e.g., `Narrative.Equipment.Slot.Chest`) |
| `equipment_modifier_ge` | TSubclassOf | GE applied when equipped |
| `attack_rating` | float | Bonus attack damage |
| `armor_rating` | float | Damage reduction |
| `stealth_rating` | float | Stealth modifier |

**Weapon Properties (UWeaponItem):**
| Property | Type | Description |
|----------|------|-------------|
| `weapon_visual_class` | TSoftClassPtr | AWeaponVisual class |
| `weapon_hand` | Enum | TwoHanded, MainHand, OffHand, DualWieldable |
| `attack_damage` | float | Base damage |
| `heavy_attack_damage_multiplier` | float | Heavy attack multiplier |
| `weapon_abilities` | TArray | Abilities granted when wielded |

**Ranged Weapon Properties (URangedWeaponItem):**
| Property | Type | Description |
|----------|------|-------------|
| `required_ammo` | TSubclassOf | Ammo item class |
| `clip_size` | int32 | Magazine capacity |
| `aim_fov_pct` | float | FOV percentage when aiming |
| `base_spread_degrees` | float | Base weapon spread |
| `max_spread_degrees` | float | Maximum spread |

---

## Goal System

### Goal Items (UNPCGoalItem)

Goal Items represent AI objectives that NPCs pursue.

**Narrative Pro Structure (from NPCGoalItem.h):**
```cpp
UCLASS(Blueprintable, BlueprintType)
class UNPCGoalItem : public UObject
{
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float DefaultScore = 50.0f;         // Priority when active

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float GoalLifetime = -1.0f;         // -1 = never expires

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bRemoveOnSucceeded = false;    // Remove after completion

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bSaveGoal = false;             // Persist across saves

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayTagContainer OwnedTags;    // Tags granted while active

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayTagContainer BlockTags;    // Tags that block this goal

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayTagContainer RequireTags;  // Tags required to pursue goal
};
```

**Example Goals from Narrative Pro:**

| Goal Class | Purpose | Key Variables |
|------------|---------|---------------|
| Goal_Attack | Attack a target | TargetActor, TargetClass, EngageDistance, bMeleeAttack |
| Goal_FollowCharacter | Follow another character | TargetCharacter, FollowDistance, FollowMode |
| Goal_MoveToDestination | Move to a location | DestinationName, DestinationActor |
| Goal_Patrol | Patrol between points | PatrolPoints, PatrolMode |
| Goal_Sleep | Sleep at location | SleepLocation, WakeTime |

**YAML Syntax:**
```yaml
goal_items:
  - name: Goal_DefendArea
    folder: AI/Goals
    parent_class: NPCGoalItem
    default_score: 75.0
    goal_lifetime: -1.0
    remove_on_succeeded: false
    save_goal: true
    owned_tags:
      - State.Defending
    block_tags:
      - State.Fleeing
    require_tags:
      - State.Alive
    variables:
      - name: DefenseRadius
        type: Float
        default_value: "500.0"
        instance_editable: true
      - name: TargetActor
        type: Object
        class: Actor
    event_graph: EventGraph_DefendArea
```

**v7.8.52 Enhancements:**
- `variables:` - Add Blueprint variables (same as actor_blueprints)
- `event_graph:` - Reference named event graph for custom logic

---

### Goal Generators (UNPCGoalGenerator)

Goal Generators dynamically create goals based on conditions.

**Narrative Pro Structure (from NPCGoalGenerator.h):**
```cpp
UCLASS(Blueprintable, BlueprintType)
class UNPCGoalGenerator : public UObject
{
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bSaveGoalGenerator = false;    // Persist across saves

    // Blueprint events:
    // InitializeGoalGenerator() - Called when generator starts
    // GenerateGoals() - Called to create new goals
};
```

**Example Generators from Narrative Pro:**

| Generator Class | Purpose | Key Variables |
|-----------------|---------|---------------|
| GoalGenerator_Attack | Generate attack goals from EQS | QueryTemplate, RunMode, AttackGoalClass, AttackAffiliationMap |
| GoalGenerator_Patrol | Generate patrol goals | PatrolRoutes, PatrolMode |
| GoalGenerator_Schedule | Generate goals from schedule | ScheduleData |

**YAML Syntax:**
```yaml
goal_generators:
  - name: GoalGenerator_DefendBase
    folder: AI/GoalGenerators
    parent_class: NPCGoalGenerator
    save_goal_generator: false
    variables:
      - name: QueryTemplate
        type: Object
        class: EnvQuery
      - name: DefenseGoalClass
        type: Class
        class: NPCGoalItem
      - name: DefenseRadius
        type: Float
        default_value: "1000.0"
    event_graph: EventGraph_DefenseGenerator
```

**Generator Properties:**
| Property | Type | Description |
|----------|------|-------------|
| `parent_class` | string | Base class (default: NPCGoalGenerator) |
| `save_goal_generator` | bool | Persist across saves |
| `variables` | array | Blueprint variables |
| `event_graph` | string | Named event graph reference |

---

## Gameplay Cue System

Gameplay Cues provide VFX/SFX feedback for GAS events (damage, abilities, effects).

### Parent Classes

Three parent classes with different capabilities:

| Parent Class | Duration | Overridable Functions | Use Case |
|--------------|----------|----------------------|----------|
| `GameplayCueNotify_Burst` | Instant | 5 | One-shot VFX (hit impacts) |
| `GameplayCueNotify_BurstLatent` | Duration | 27+ | Timed VFX (DOTs, buffs) |
| `AGameplayCueNotify_Actor` | Looping | 30+ | Persistent VFX (auras) |

**GameplayCueNotify_Burst (Simple):**
- `OnBurst()` - Main execution
- `GetBurstSpawnConditionOverride()`
- `GetBurstPlacementInfoOverride()`

**GameplayCueNotify_BurstLatent (Common):**
- All Burst functions plus:
- `GCN_Defaults`: DefaultSpawnCondition, DefaultPlacementInfo
- `GCN_Effects`: BurstParticles, BurstSounds, BurstCameraShake, etc.
- 27 overridable functions for customization

### GCN Effects System

Full effect configuration available in `gcn_effects:` section.

**BurstParticles (Niagara):**
```yaml
gcn_effects:
  burst_particles:
    - niagara_system: /Game/VFX/NS_FireExplosion
      cast_shadow: false
      spawn_condition:
        locally_controlled_source: true
        chance_to_play: 1.0
      placement:
        socket_name: FX_Socket
        attach_policy: AttachToTarget
        attachment_rule: SnapToTarget
```

**BurstSounds:**
```yaml
  burst_sounds:
    - sound: /Game/Audio/SFX_Explosion
      volume_multiplier: 1.0
      pitch_multiplier: 1.0
      spawn_condition:
        locally_controlled_source: true
      placement:
        attach_policy: AttachToTarget
```

**BurstCameraShake:**
```yaml
  burst_camera_shake:
    camera_shake_class: /Game/VFX/CS_HeavyImpact
    shake_scale: 1.0
    play_space: CameraLocal
    play_in_world: false
    world_inner_radius: 500.0
    world_outer_radius: 1000.0
```

**BurstCameraLensEffect:**
```yaml
  burst_camera_lens_effect:
    lens_effect_class: /Game/VFX/LE_BloodSplatter
    play_in_world: false
    world_inner_radius: 200.0
    world_outer_radius: 800.0
```

**BurstForceFeedback:**
```yaml
  burst_force_feedback:
    force_feedback_effect: /Game/Feedback/FF_Explosion
    force_feedback_tag: Feedback.Explosion
    is_looping: false
    play_in_world: false
    world_intensity: 1.0
```

**BurstDecal:**
```yaml
  burst_decal:
    decal_material: /Game/Materials/M_BloodDecal
    decal_size: [100, 100, 100]
    override_fade_out: true
    fade_out_start_delay: 5.0
    fade_out_duration: 2.0
```

### Spawn Condition Override

Controls when effects play.

| Property | Type | Description |
|----------|------|-------------|
| `locally_controlled_source` | bool | Play on local player's effects |
| `locally_controlled_policy` | bool | Respect local control policy |
| `chance_to_play` | float | Probability (0.0-1.0) |
| `allowed_surface_types` | array | Surface types that allow play |
| `rejected_surface_types` | array | Surface types that block play |

### Placement Info Override

Controls where effects spawn.

| Property | Type | Description |
|----------|------|-------------|
| `socket_name` | FName | Socket on mesh |
| `attach_policy` | Enum | AttachToTarget, DoNotAttach |
| `attachment_rule` | Enum | SnapToTarget, KeepRelative, KeepWorld |
| `rotation_override` | Rotator | Custom rotation [P, Y, R] |
| `scale_override` | Vector | Custom scale [X, Y, Z] |

### Custom Functions

Gameplay Cues support custom replicated functions.

```yaml
functions:
  - name: PlayCustomEffect
    replicates: Multicast       # None, Server, Client, Multicast
    reliable: true
    inputs:
      - name: Intensity
        type: Float
      - name: TargetActor
        type: Object
        class: Actor
    outputs:
      - name: bSuccess
        type: Bool
```

**Replication Options:**
| Value | Description |
|-------|-------------|
| `None` | Local only |
| `Server` | Client → Server RPC |
| `Client` | Server → Owning Client RPC |
| `Multicast` | Server → All Clients RPC |

### Complete Gameplay Cue YAML Example

```yaml
gameplay_cues:
  - name: GC_TakeDamage
    folder: VFX/GameplayCues
    parent_class: GameplayCueNotify_BurstLatent
    gameplay_cue_tag: GameplayCue.TakeDamage
    auto_attach_to_owner: true
    is_override: false
    num_preallocated_instances: 0

    # Variables
    variables:
      - name: DamageType
        type: Byte
        default_value: "0"
      - name: ParticleScale
        type: Float
        default_value: "1.0"

    # GCN Defaults
    gcn_defaults:
      default_spawn_condition:
        locally_controlled_source: true
        chance_to_play: 1.0
      default_placement_info:
        socket_name: FX_Hit
        attach_policy: AttachToTarget

    # GCN Effects
    gcn_effects:
      burst_particles:
        - niagara_system: /Game/VFX/NS_BloodSplat
          cast_shadow: false
          spawn_condition:
            locally_controlled_source: true
          placement:
            socket_name: FX_Hit
            attach_policy: AttachToTarget

      burst_sounds:
        - sound: /Game/Audio/SFX_FleshHit
          volume_multiplier: 1.0
          pitch_multiplier: 1.0

      burst_camera_shake:
        camera_shake_class: /Game/VFX/CS_LightHit
        shake_scale: 0.5

      burst_decal:
        decal_material: /Game/Materials/M_Blood
        decal_size: [50, 50, 50]
        override_fade_out: true
        fade_out_start_delay: 10.0
        fade_out_duration: 5.0

    # Custom functions
    functions:
      - name: ApplyVisualDamage
        replicates: Multicast
        reliable: true
        inputs:
          - name: DamageAmount
            type: Float

  - name: GC_WeaponFire
    folder: VFX/GameplayCues
    parent_class: GameplayCueNotify_Burst
    gameplay_cue_tag: GameplayCue.Weapon.Fire
    variables:
      - name: MuzzleFlashScale
        type: Float
        default_value: "1.0"
```

### Generator Features

**Supported:**
- All parent class types (Burst, BurstLatent, Actor)
- Full GCN Effects system (particles, sounds, shake, lens, feedback, decal)
- Spawn condition and placement overrides
- Blueprint variables with all types
- Custom functions with replication
- Event graph for OnBurst/custom logic
- Metadata and hash caching

**CDO Properties Set:**
- `GameplayCueTag` (FGameplayTag)
- `bAutoAttachToOwner` (bool)
- `bIsOverride` (bool)
- `NumPreallocatedInstances` (int32)
- All GCN Defaults properties
- All GCN Effects arrays

---

## Behavior Tree System

BT Services and BT Tasks are standalone Blueprint assets used in Behavior Trees for NPC AI logic.

### BT Services (BTS_)

BT Services tick on intervals while their parent BT node is active. They're used for continuous monitoring or updating.

**Parent Class:** `BTService_BlueprintBase`

**Key Events:**
- `Event Receive Activation AI` - Called when parent node becomes active
- `Event Receive Tick AI` - Called on interval while parent is active
- `Event Receive Deactivation AI` - Called when parent node deactivates

**CDO Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `Interval` | float | 0.5 | Tick interval in seconds |
| `RandomDeviation` | float | 0.1 | Random variance on interval |
| `bCallTickOnSearchStart` | bool | true | Execute tick when BT search starts |
| `bRestartTimerOnEachActivation` | bool | false | Reset timer on parent reactivation |
| `NodeName` | FString | - | Display name in BT editor |
| `CustomDescription` | FString | - | Description shown in BT editor |

**YAML Syntax:**
```yaml
bt_services:
  - name: BTS_AdjustFollowSpeed
    folder: AI/Services
    parent_class: BTService_BlueprintBase
    interval: 0.5
    random_deviation: 0.1
    call_tick_on_search_start: true
    restart_timer_on_each_activation: false
    node_name: "Adjust Follow Speed"
    description: "Adjusts NPC movement speed based on distance to target"
    variables:
      - name: TargetDistance
        type: Float
        default_value: "0.0"
      - name: MinSpeed
        type: Float
        default_value: "100.0"
      - name: MaxSpeed
        type: Float
        default_value: "600.0"
    event_graph: EventGraph_AdjustFollowSpeed
```

**Generated Output:**
- BTS_ Blueprint inheriting from BTService_BlueprintBase
- Variables added via FBlueprintEditorUtils
- CDO properties set (Interval, RandomDeviation, etc.)
- Event graph generated if specified

---

### BT Tasks (BTTask_)

BT Tasks execute once and must call FinishExecute() with success or failure. They're used for discrete actions.

**Parent Class:** `BTTask_BlueprintBase`

**Key Events:**
- `Event Receive Execute AI` - Called when task starts
- `Event Receive Tick AI` - Called on interval if TickInterval >= 0
- `Event Receive Abort AI` - Called when task is aborted

**Important:** Tasks MUST call `Finish Execute` node to complete.

**CDO Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `TickInterval` | float | -1.0 | -1 = no tick, else tick interval |
| `bIgnoreRestartSelf` | bool | false | Ignore restart requests for self |
| `NodeName` | FString | - | Display name in BT editor |
| `CustomDescription` | FString | - | Description shown in BT editor |

**YAML Syntax:**
```yaml
bt_tasks:
  - name: BTTask_ActivateAbility
    folder: AI/Tasks
    parent_class: BTTask_BlueprintBase
    tick_interval: -1.0
    ignore_restart_self: false
    node_name: "Activate Ability"
    description: "Activates a gameplay ability on the controlled pawn"
    variables:
      - name: AbilityClass
        type: Class
        class: GameplayAbility
        instance_editable: true
      - name: bWaitForEnd
        type: Bool
        default_value: "true"
    event_graph: EventGraph_ActivateAbility
```

**Generated Output:**
- BTTask_ Blueprint inheriting from BTTask_BlueprintBase
- Variables added via FBlueprintEditorUtils
- CDO properties set (TickInterval, etc.)
- Event graph generated if specified

---

### Blackboard Key Types

BT Services and Tasks often reference blackboard keys. Key types:

| Type | Pin Color | Description |
|------|-----------|-------------|
| Object | Blue | Actor/Object reference |
| Vector | Yellow | Location/direction |
| Rotator | Purple | Rotation |
| Float | Green | Numeric value |
| Int | Cyan | Integer value |
| Bool | Red | True/false |
| Name | Pink | FName value |
| String | Magenta | FString value |
| Class | Orange | TSubclassOf |
| Enum | Light Blue | Enumeration |

**Accessing Blackboard in Event Graph:**
```yaml
event_graphs:
  - name: EventGraph_BTS_Example
    nodes:
      - id: GetBlackboardComponent
        type: CallFunction
        function: GetBlackboardComponent
        class: BehaviorTreeComponent
        target_self: true
      - id: GetValueAsObject
        type: CallFunction
        function: GetValueAsObject
        class: BlackboardComponent
        properties:
          KeyName: TargetActor
    connections:
      - from: [Event_ReceiveTickAI, Then]
        to: [GetBlackboardComponent, Exec]
      - from: [GetBlackboardComponent, ReturnValue]
        to: [GetValueAsObject, Target]
```

---

### Complete BT System Example

```yaml
# BT Services - tick-based monitoring
bt_services:
  - name: BTS_UpdateCombatTarget
    folder: AI/Services
    interval: 0.3
    random_deviation: 0.1
    node_name: "Update Combat Target"
    description: "Updates the target based on aggro and distance"
    variables:
      - name: MaxTargetDistance
        type: Float
        default_value: "2000.0"
      - name: CurrentTarget
        type: Object
        class: Character

  - name: BTS_CheckAmmo
    folder: AI/Services
    interval: 1.0
    node_name: "Check Ammo"
    variables:
      - name: MinAmmoThreshold
        type: Int
        default_value: "5"

# BT Tasks - discrete actions
bt_tasks:
  - name: BTTask_FireWeapon
    folder: AI/Tasks
    node_name: "Fire Weapon"
    description: "Fires the equipped weapon at current target"
    variables:
      - name: BurstCount
        type: Int
        default_value: "3"
      - name: BurstDelay
        type: Float
        default_value: "0.1"

  - name: BTTask_Reload
    folder: AI/Tasks
    tick_interval: 0.5
    node_name: "Reload"
    variables:
      - name: ReloadTime
        type: Float
        default_value: "2.0"

  - name: BTTask_ActivateAbility
    folder: AI/Tasks
    node_name: "Activate Ability"
    variables:
      - name: AbilityClass
        type: Class
        class: GameplayAbility
        instance_editable: true
      - name: TargetActor
        type: Object
        class: Actor
```

---

### BT Asset Prefixes

| Prefix | Asset Type | Manifest Section |
|--------|------------|------------------|
| `BTS_` | BT Service | `bt_services:` |
| `BTTask_` | BT Task | `bt_tasks:` |

---

## Blueprint Trigger System

Blueprint Triggers (BPT_) are Quest task types used in Narrative Pro's quest branches. v7.8.52 adds full generator support.

### BPT_ Task Types

Narrative Pro includes 23 BPT_ task types for quest progression:

| Class | Purpose | Key Properties |
|-------|---------|----------------|
| `BPT_AddGoalAndWait` | Add NPC goal and wait for completion | GoalClass, NPCDefinition |
| `BPT_Always` | Always succeeds (placeholder) | - |
| `BPT_CompleteQuest` | Complete another quest | QuestClass, SuccessState |
| `BPT_ConsumeItem` | Consume item from inventory | ItemClass, Quantity |
| `BPT_EnterSubQuest` | Enter a sub-quest | SubQuestClass |
| `BPT_FindItem` | Find/acquire item | ItemClass, Quantity |
| `BPT_FinishDialogue` | Complete dialogue | DialogueClass, NodeID |
| `BPT_GoToLocation` | Navigate to location | LocationName, Radius |
| `BPT_HasEquippedItem` | Check equipped item | ItemClass, SlotTag |
| `BPT_HasItem` | Check item in inventory | ItemClass, Quantity |
| `BPT_InteractWith` | Interact with object | InteractableClass |
| `BPT_KillNPC` | Kill NPC | NPCDefinition, Quantity |
| `BPT_ObtainItem` | Obtain specific item | ItemClass, Quantity |
| `BPT_PlayDialogue` | Play dialogue sequence | DialogueClass |
| `BPT_ReachDestination` | Reach destination | DestinationName, Radius |
| `BPT_SendNarrativeEvent` | Send narrative event | EventClass |
| `BPT_SpawnAndInteract` | Spawn then interact | SpawnClass, InteractableClass |
| `BPT_StartQuest` | Start another quest | QuestClass |
| `BPT_TalkToNPC` | Talk to NPC | NPCDefinition, DialogueClass |
| `BPT_TimeOfDayRange` | Wait for time range | StartTime, EndTime |
| `BPT_TriggerActivated` | Wait for trigger | TriggerTag |
| `BPT_UseItem` | Use item from inventory | ItemClass |
| `BPT_Wait` | Wait for duration | Duration |

### Common BPT Properties

**YAML Syntax:**
```yaml
blueprint_triggers:
  - name: BPT_CustomFindItem
    folder: Quests/Tasks
    parent_class: BPT_FindItem
    node_name: "Find Custom Item"
    description: "Custom task to find a specific item"

    # Common properties
    variables:
      - name: RequiredQuantity
        type: Int
        default_value: "1"
        instance_editable: true
    event_graph: EventGraph_CustomFindItem
```

**Base Properties (UQuestTask):**

| Property | Type | Description |
|----------|------|-------------|
| `parent_class` | string | Base BPT_ class to inherit from |
| `node_name` | FString | Display name in quest editor |
| `description` | FString | Task description |
| `variables` | array | Blueprint variables |
| `event_graph` | string | Named event graph reference |

---

### Location-Based Tasks

Location tasks (BPT_GoToLocation, BPT_ReachDestination) use navigation markers.

**YAML Syntax:**
```yaml
blueprint_triggers:
  - name: BPT_GoToBlacksmith
    folder: Quests/Tasks
    parent_class: BPT_GoToLocation
    node_name: "Go to Blacksmith"

    # Location properties
    location_name: "Blacksmith_Shop"
    acceptance_radius: 200.0
    use_navigation_marker: true
    navigation_marker_text: "Go to Blacksmith"

    variables:
      - name: MarkerColor
        type: LinearColor
        default_value: "(R=1.0,G=0.5,B=0.0,A=1.0)"
```

**Location Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `location_name` | FString | - | Named location in world |
| `acceptance_radius` | float | 100.0 | Distance to complete task |
| `use_navigation_marker` | bool | true | Show UI marker |
| `navigation_marker_text` | FString | - | Text shown on marker |
| `navigation_marker_icon` | Texture2D | - | Icon shown on marker |

---

### Time-Based Tasks

Time tasks (BPT_TimeOfDayRange, BPT_Wait) control quest timing.

**YAML Syntax:**
```yaml
blueprint_triggers:
  - name: BPT_WaitForNight
    folder: Quests/Tasks
    parent_class: BPT_TimeOfDayRange
    node_name: "Wait for Night"

    # Time range (0-2400 format, 100 = 1 hour)
    start_time: 2000      # 8:00 PM
    end_time: 600         # 6:00 AM (next day)

  - name: BPT_WaitShort
    parent_class: BPT_Wait
    wait_duration: 5.0    # 5 seconds
```

**Time Properties:**

| Property | Type | Description |
|----------|------|-------------|
| `start_time` | int32 | Start time (0-2400) |
| `end_time` | int32 | End time (0-2400) |
| `wait_duration` | float | Wait time in seconds |

---

### Complete BPT Example

```yaml
blueprint_triggers:
  # Item collection task
  - name: BPT_CollectOre
    folder: Quests/Tasks/Mining
    parent_class: BPT_FindItem
    node_name: "Collect Iron Ore"
    description: "Collect iron ore from the mines"
    variables:
      - name: OreType
        type: Class
        class: NarrativeItem
        instance_editable: true
      - name: TargetQuantity
        type: Int
        default_value: "10"
    event_graph: EventGraph_CollectOre

  # Location task with custom marker
  - name: BPT_GoToMine
    folder: Quests/Tasks/Mining
    parent_class: BPT_GoToLocation
    node_name: "Travel to Mine"
    location_name: "IronMine_Entrance"
    acceptance_radius: 300.0
    use_navigation_marker: true
    navigation_marker_text: "Mine Entrance"

  # Dialogue task
  - name: BPT_TalkToMiner
    folder: Quests/Tasks/Mining
    parent_class: BPT_TalkToNPC
    node_name: "Talk to Miner"
    variables:
      - name: MinerNPC
        type: Object
        class: NPCDefinition
        instance_editable: true
```

---

## Blueprint Activity System

Blueprint Activities (BPA_) are NPC AI activities. v7.8.52 adds comprehensive support for all 23 activity types with enhanced properties.

### BPA_ Activity Types

Narrative Pro includes 23 BPA_ activity types:

| Class | Purpose | Key Properties |
|-------|---------|----------------|
| `BPA_Attack` | Combat attack behavior | TargetActor, AttackType |
| `BPA_Attack_Melee` | Melee attack | WeaponTypes, AttackTaggedDialogue |
| `BPA_Attack_Melee_Unarmed` | Unarmed melee | - |
| `BPA_Attack_Ranged` | Ranged attack | WeaponTypes, AimOffset |
| `BPA_BruteAttack` | Heavy attack pattern | ChargeDuration, StaggerTime |
| `BPA_EnterVehicle` | Enter vehicle | VehicleClass |
| `BPA_ExitVehicle` | Exit vehicle | ExitLocation |
| `BPA_Flee` | Flee from threat | FleeDistance, FleeSpeed |
| `BPA_Flee_FindCover` | Flee to cover | CoverQuery, MinCoverDist |
| `BPA_Follow_Character` | Follow character | FollowGoal, FollowDistance |
| `BPA_Idle` | Idle state | IdleAnimations |
| `BPA_Interact` | Interact with object | InteractSubGoal, InteractionClass |
| `BPA_MoveToLocation` | Move to location | DestinationGoal |
| `BPA_Patrol` | Patrol route | PatrolPoints, PatrolMode |
| `BPA_PursueTarget` | Pursue target | PursueSpeed, MaxPursueDist |
| `BPA_RangedAttack` | Ranged attack | ProjectileClass |
| `BPA_Rest` | Rest/sleep | RestDuration |
| `BPA_Roam` | Random roaming | RoamRadius, RoamInterval |
| `BPA_Search` | Search for target | SearchRadius, SearchDuration |
| `BPA_Strafe` | Strafe movement | StrafeDirection, StrafeSpeed |
| `BPA_TakeCover` | Take cover | CoverQuery |
| `BPA_UseAbility` | Use gameplay ability | AbilityClass |
| `BPA_Wander` | Wander aimlessly | WanderRadius |

---

### Enhanced Activity Properties

v7.8.52 adds these properties to all activities:

**YAML Syntax:**
```yaml
activities:
  - name: BPA_CustomAttack
    folder: AI/Activities
    parent_class: NarrativeActivityBase
    behavior_tree: BT_AttackBehavior
    activity_name: "Custom Attack"
    description: "Enhanced attack activity"

    # Tag containers
    owned_tags:
      - State.Attacking
    block_tags:
      - State.Dead
      - State.Stunned
    require_tags:
      - State.Alive

    # v7.8.52 Enhanced properties
    attack_tagged_dialogue: Dialogue.Combat.Attack
    weapon_types:
      - MeleeWeaponItem
      - RangedWeaponItem
    follow_goal: Goal_FollowPlayer
    interact_sub_goal: Goal_Interact
    bb_key_follow_distance: FollowDistance

    # Blueprint support
    variables:
      - name: AttackRange
        type: Float
        default_value: "200.0"
    event_graph: EventGraph_CustomAttack
```

**Enhanced Properties (v7.8.52):**

| Property | Type | Description |
|----------|------|-------------|
| `attack_tagged_dialogue` | FGameplayTag | Tagged dialogue during attack |
| `weapon_types` | TArray<TSubclassOf<UWeaponItem>> | Allowed weapon types |
| `follow_goal` | TSubclassOf<UNPCGoalItem> | Goal class for following |
| `interact_sub_goal` | TSubclassOf<UNPCGoalItem> | Sub-goal for interactions |
| `bb_key_follow_distance` | FName | Blackboard key for follow distance |
| `variables` | array | Blueprint variables |
| `event_graph` | string | Named event graph reference |

---

### Attack Activity Config

Attack activities have specialized properties for combat.

**YAML Syntax:**
```yaml
activities:
  - name: BPA_MeleeAttack
    parent_class: BPA_Attack_Melee
    folder: AI/Activities/Combat
    behavior_tree: BT_MeleeCombat

    # Combat properties
    attack_tagged_dialogue: Dialogue.Combat.MeleeAttack
    weapon_types:
      - MeleeWeaponItem
      - FistWeaponItem

    # Tag control
    owned_tags:
      - State.InCombat
      - State.Attacking.Melee
    block_tags:
      - State.Dead
      - State.Stunned
      - State.Fleeing
    require_tags:
      - State.Alive
      - State.HasWeapon

    variables:
      - name: AttackDamageMultiplier
        type: Float
        default_value: "1.0"
      - name: bCanCombo
        type: Bool
        default_value: "true"
```

---

### Follow Activity Config

Follow activities for companion/escort behavior.

**YAML Syntax:**
```yaml
activities:
  - name: BPA_FollowPlayer
    parent_class: BPA_Follow_Character
    folder: AI/Activities/Follow
    behavior_tree: BT_Follow
    activity_name: "Follow Player"

    # Follow config
    follow_goal: Goal_FollowPlayer
    bb_key_follow_distance: FollowDistance

    # Tags
    owned_tags:
      - State.Following
    require_tags:
      - State.Alive

    variables:
      - name: MinFollowDistance
        type: Float
        default_value: "150.0"
      - name: MaxFollowDistance
        type: Float
        default_value: "500.0"
      - name: FollowSpeedMultiplier
        type: Float
        default_value: "1.0"
```

---

### Complete BPA Example

```yaml
activities:
  # Ranged attack activity
  - name: BPA_RangedCombat
    folder: AI/Activities/Combat
    parent_class: BPA_Attack_Ranged
    behavior_tree: BT_RangedCombat
    activity_name: "Ranged Combat"
    description: "NPC uses ranged weapons to attack"

    attack_tagged_dialogue: Dialogue.Combat.Ranged
    weapon_types:
      - RangedWeaponItem

    owned_tags:
      - State.InCombat
      - State.Attacking.Ranged
    block_tags:
      - State.Dead
      - State.Fleeing
      - State.Reloading
    require_tags:
      - State.Alive
      - State.HasAmmo

    variables:
      - name: OptimalRange
        type: Float
        default_value: "800.0"
      - name: MinRange
        type: Float
        default_value: "300.0"
      - name: bTakeCoverBetweenShots
        type: Bool
        default_value: "true"
    event_graph: EventGraph_RangedCombat

  # Patrol activity
  - name: BPA_GuardPatrol
    folder: AI/Activities/Patrol
    parent_class: BPA_Patrol
    behavior_tree: BT_Patrol
    activity_name: "Guard Patrol"

    owned_tags:
      - State.Patrolling
    block_tags:
      - State.InCombat

    variables:
      - name: PatrolSpeed
        type: Float
        default_value: "200.0"
      - name: WaitAtPoints
        type: Bool
        default_value: "true"
      - name: PointWaitTime
        type: Float
        default_value: "3.0"

  # Companion follow
  - name: BPA_CompanionFollow
    folder: AI/Activities/Companion
    parent_class: BPA_Follow_Character
    behavior_tree: BT_CompanionFollow
    activity_name: "Companion Follow"

    follow_goal: Goal_FollowPlayer
    interact_sub_goal: Goal_InteractWithWorld
    bb_key_follow_distance: CompanionFollowDist

    owned_tags:
      - State.Following
      - State.Companion
    require_tags:
      - State.Alive

    variables:
      - name: TeleportDistance
        type: Float
        default_value: "2000.0"
        instance_editable: true
```

---

### BPT/BPA Asset Prefixes

| Prefix | Asset Type | Manifest Section |
|--------|------------|------------------|
| `BPT_` | Blueprint Trigger (Quest Task) | `blueprint_triggers:` |
| `BPA_` | Blueprint Activity (NPC Activity) | `activities:` |

---

## Gameplay Effect Enhancements

v7.8.52 adds comprehensive support for advanced Gameplay Effect features including execution calculations, attribute-based magnitudes, and per-modifier tag requirements.

### Execution Calculations

Execution Calculations (ExecCalcs) perform complex damage/heal calculations using captured attributes.

**Narrative Pro Execution Classes:**

| Class | Purpose | Captured Attributes |
|-------|---------|---------------------|
| `NarrativeDamageExecCalc` | Damage calculation | AttackDamage, Armor |
| `NarrativeHealExecution` | Heal calculation | Heal |

**YAML Syntax - Simple Execution:**
```yaml
gameplay_effects:
  - name: GE_LaserDamage
    folder: Effects
    duration_policy: Instant
    executions:
      - calculation_class: NarrativeDamageExecCalc
```

**YAML Syntax - Execution with Modifiers:**
```yaml
gameplay_effects:
  - name: GE_HealOverTime
    folder: Effects
    duration_policy: HasDuration
    duration_magnitude: 5.0
    period: 1.0
    execute_periodic_on_application: true
    executions:
      - calculation_class: NarrativeHealExecution
        modifiers:
          - captured_attribute: NarrativeAttributeSetBase.Heal
            captured_source: Source
            captured_status: Snapshotted
            modifier_op: Additive
            magnitude_type: SetByCaller
            setbycaller_tag: SetByCaller.Heal
```

**Execution Modifier Properties:**

| Property | Type | Description |
|----------|------|-------------|
| `captured_attribute` | string | Attribute to capture (e.g., "NarrativeAttributeSetBase.Heal") |
| `captured_source` | string | "Source" or "Target" |
| `captured_status` | string | "Snapshotted" or "NotSnapshotted" |
| `modifier_op` | string | Additive, Multiplicative, Override |
| `magnitude_type` | string | ScalableFloat, SetByCaller, AttributeBased |
| `scalable_float_value` | float | Value for ScalableFloat magnitude |
| `setbycaller_tag` | string | Tag for SetByCaller magnitude |

---

### Attribute-Based Magnitude

Attribute-based magnitude calculates modifier values from captured attributes with coefficients and curves.

**YAML Syntax:**
```yaml
gameplay_effects:
  - name: GE_StaminaRegen
    folder: Effects
    duration_policy: Infinite
    period: 0.5
    modifiers:
      - attribute: NarrativeAttributeSetBase.Stamina
        operation: Additive
        magnitude_type: AttributeBased
        attribute_based:
          coefficient: 1.0
          pre_multiply_additive_value: 0.0
          post_multiply_additive_value: 0.0
          backing_attribute: NarrativeAttributeSetBase.StaminaRegenRate
          attribute_source: Source
          snapshot: false
          calculation_type: AttributeMagnitude
```

**Attribute-Based Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `coefficient` | float | 1.0 | Multiplier applied to captured value |
| `pre_multiply_additive_value` | float | 0.0 | Added before coefficient multiplication |
| `post_multiply_additive_value` | float | 0.0 | Added after coefficient multiplication |
| `backing_attribute` | string | - | Attribute to capture (e.g., "NarrativeAttributeSetBase.StaminaRegenRate") |
| `attribute_source` | string | "Source" | "Source" or "Target" |
| `snapshot` | bool | false | Capture at effect application vs live update |
| `curve_table` | string | - | Optional curve table path for scaling |
| `curve_row` | string | - | Row name in curve table |
| `calculation_type` | string | "AttributeMagnitude" | AttributeMagnitude, AttributeBaseValue, AttributeBonusMagnitude |

**Calculation Types:**
- `AttributeMagnitude` - Uses full current value (base + bonuses)
- `AttributeBaseValue` - Uses only base value
- `AttributeBonusMagnitude` - Uses only bonus modifiers

---

### Per-Modifier Tag Requirements

Each modifier can have source and target tag requirements that control when it applies.

**YAML Syntax:**
```yaml
gameplay_effects:
  - name: GE_BonusDamageToFleeing
    folder: Effects
    duration_policy: Instant
    modifiers:
      - attribute: NarrativeAttributeSetBase.AttackDamage
        operation: Additive
        magnitude_type: ScalableFloat
        magnitude_value: 25.0
        target_tags:
          must_have_tags:
            - State.Fleeing
          must_not_have_tags:
            - State.Invulnerable
      - attribute: NarrativeAttributeSetBase.AttackDamage
        operation: Multiplicative
        magnitude_value: 1.5
        source_tags:
          must_have_tags:
            - State.Empowered
```

**Tag Requirement Properties:**

| Property | Type | Description |
|----------|------|-------------|
| `source_tags.must_have_tags` | array | Tags the effect source must have |
| `source_tags.must_not_have_tags` | array | Tags the effect source must NOT have |
| `target_tags.must_have_tags` | array | Tags the effect target must have |
| `target_tags.must_not_have_tags` | array | Tags the effect target must NOT have |

---

### Conditional Gameplay Effects

Execution calculations can trigger additional effects based on conditions.

**YAML Syntax:**
```yaml
gameplay_effects:
  - name: GE_AttackWithCritChance
    folder: Effects
    duration_policy: Instant
    executions:
      - calculation_class: NarrativeDamageExecCalc
        conditional_effects:
          - effect_class: GE_CriticalHitBonus
            required_source_tags:
              - State.HasCriticalChance
          - effect_class: GE_ApplyBleed
            required_source_tags:
              - State.HasBleedingWeapon
```

**Conditional Effect Properties:**

| Property | Type | Description |
|----------|------|-------------|
| `effect_class` | string | GE_ class to apply when conditions met |
| `required_source_tags` | array | Tags the source must have for effect to apply |

---

### Periodic Inhibition Policy

Controls behavior when a periodic effect is inhibited (blocked by tags) and then uninhibited.

**YAML Syntax:**
```yaml
gameplay_effects:
  - name: GE_PoisonDoT
    folder: Effects
    duration_policy: HasDuration
    duration_magnitude: 10.0
    period: 1.0
    periodic_inhibition_policy: ExecuteAndResetPeriod
    modifiers:
      - attribute: NarrativeAttributeSetBase.Health
        operation: Additive
        magnitude_value: -10.0
```

**Policy Options:**

| Policy | Description |
|--------|-------------|
| `NeverReset` | Timer continues; missed ticks are lost |
| `ResetPeriod` | Timer resets; missed ticks are lost |
| `ExecuteAndResetPeriod` | Immediately executes missed tick, then resets timer |

---

### Complete GE Enhancement Example

```yaml
gameplay_effects:
  # Healing effect using NarrativeHealExecution
  - name: GE_HealingAura
    folder: Effects/Healing
    duration_policy: HasDuration
    duration_magnitude: 10.0
    period: 1.0
    execute_periodic_on_application: true
    periodic_inhibition_policy: ExecuteAndResetPeriod
    executions:
      - calculation_class: NarrativeHealExecution
        modifiers:
          - captured_attribute: NarrativeAttributeSetBase.Heal
            captured_source: Source
            captured_status: Snapshotted
            modifier_op: Additive
            magnitude_type: AttributeBased
            attribute_based:
              coefficient: 0.1
              backing_attribute: NarrativeAttributeSetBase.MaxHealth
              attribute_source: Target
              snapshot: false
        conditional_effects:
          - effect_class: GE_RemovePoison
            required_source_tags:
              - State.HolyHealer
    granted_tags:
      - Effect.Healing.Aura

  # Damage with attribute-based scaling and conditional crits
  - name: GE_PowerAttack
    folder: Effects/Damage
    duration_policy: Instant
    executions:
      - calculation_class: NarrativeDamageExecCalc
        modifiers:
          - captured_attribute: NarrativeAttributeSetBase.AttackDamage
            captured_source: Source
            captured_status: Snapshotted
            modifier_op: Multiplicative
            magnitude_type: ScalableFloat
            magnitude_value: 2.0
        conditional_effects:
          - effect_class: GE_CriticalDamage
            required_source_tags:
              - State.CriticalStrike

  # Stamina regeneration based on attribute
  - name: GE_StaminaRecovery
    folder: Effects/Regen
    duration_policy: Infinite
    period: 0.5
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
        target_tags:
          must_not_have_tags:
            - State.Exhausted
            - State.Dead
```

---

## Gameplay Ability Enhancements

v7.8.52 adds comprehensive NarrativeGameplayAbility property support based on Narrative Pro screenshots.

### Ability Triggers

Event-based ability activation using FAbilityTriggerData array.

**Narrative Pro Structure (from GameplayAbility.h):**
```cpp
USTRUCT()
struct FAbilityTriggerData
{
    UPROPERTY(EditAnywhere)
    FGameplayTag TriggerTag;

    UPROPERTY(EditAnywhere)
    TEnumAsByte<EGameplayAbilityTriggerSource::Type> TriggerSource;
};

// TriggerSource options:
// - GameplayEvent: Triggered by gameplay event with matching tag
// - OwnedTagAdded: Triggered when tag is added to owner
// - OwnedTagPresent: Triggered while tag is present on owner
```

**YAML Syntax:**
```yaml
gameplay_abilities:
  - name: GA_ReactiveDefense
    parent_class: NarrativeGameplayAbility
    ability_triggers:
      - trigger_tag: Event.Damage.Received
        trigger_source: GameplayEvent
      - trigger_tag: State.LowHealth
        trigger_source: OwnedTagAdded
```

**Generator Output:**
- Populates `AbilityTriggers` TArray<FAbilityTriggerData>
- Each entry sets TriggerTag (FGameplayTag) and TriggerSource (enum)

---

### Cues Section

Gameplay cue tags for visual/audio feedback during combat.

**YAML Syntax:**
```yaml
gameplay_abilities:
  - name: GA_RangedAttack
    parent_class: NarrativeGameplayAbility
    # Cues section
    fire_cue_tag: GameplayCue.Weapon.Fire
    fire_cue_tag_no_impact: GameplayCue.Weapon.FireNoImpact
```

**Properties:**

| Property | Type | Description |
|----------|------|-------------|
| `fire_cue_tag` | FGameplayTag | Cue triggered on weapon fire with impact |
| `fire_cue_tag_no_impact` | FGameplayTag | Cue triggered on weapon fire without impact |

---

### Damage Section

Combat damage configuration for abilities.

**YAML Syntax:**
```yaml
gameplay_abilities:
  - name: GA_MeleeAttack
    parent_class: NarrativeGameplayAbility
    # Damage section
    damage_effect_class: GE_MeleeDamage
    default_attack_damage: 50.0
    requires_ammo: false
    draw_debug_traces: false
```

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `damage_effect_class` | TSubclassOf<UGameplayEffect> | None | GE applied when damage is dealt |
| `default_attack_damage` | float | 0.0 | Base damage value for the ability |
| `requires_ammo` | bool | false | Whether ability requires ammo to activate |
| `draw_debug_traces` | bool | false | Draw debug traces for hit detection |

---

### Firearm Ability Config

Ranged weapon ability configuration.

**YAML Syntax:**
```yaml
gameplay_abilities:
  - name: GA_RifleShot
    parent_class: NarrativeGameplayAbility
    # Firearm config
    is_automatic: true
    rate_of_fire: 0.1
    burst_amount: 3
    trace_distance: 15000.0
    trace_radius: 5.0
    trace_multi: false
```

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `is_automatic` | bool | false | Hold to continuously fire |
| `rate_of_fire` | float | 0.2 | Seconds between shots |
| `burst_amount` | int | -1 | Shots per burst (-1 = not burst mode) |
| `trace_distance` | float | 10000.0 | Max range for hit traces |
| `trace_radius` | float | 0.0 | Radius for sphere traces (0 = line trace) |
| `trace_multi` | bool | false | Whether to hit multiple targets |

---

### Motion Warping Config

Motion warping for melee attack abilities.

**YAML Syntax:**
```yaml
gameplay_abilities:
  - name: GA_MeleeAttack
    parent_class: GA_Melee_Unarmed
    # Warping config
    should_warp: true
    warp_maintain_dist: 100.0
    min_warp_dist: 20.0
    max_warp_dist: 250.0
```

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `should_warp` | bool | false | Enable motion warping to target |
| `warp_maintain_dist` | float | 100.0 | Distance to maintain from target |
| `min_warp_dist` | float | 20.0 | Minimum warp distance |
| `max_warp_dist` | float | 250.0 | Maximum warp distance |

---

### Execution Config

Execution/finisher ability configuration.

**YAML Syntax:**
```yaml
gameplay_abilities:
  - name: GA_Execution
    parent_class: NarrativeGameplayAbility
    # Execution config
    execution_gameplay_tag: Ability.Execution.Stab
    execution_invulnerability: true
```

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `execution_gameplay_tag` | FGameplayTag | None | Tag identifying this execution type |
| `execution_invulnerability` | bool | false | Grant invulnerability during execution |

---

### Complete GA Enhancement Example

```yaml
gameplay_abilities:
  # Ranged weapon with full firearm config
  - name: GA_LaserRifle
    parent_class: NarrativeGameplayAbility
    folder: Abilities/Ranged
    instancing_policy: InstancedPerActor
    net_execution_policy: LocalPredicted
    input_tag: InputTag.Ability.Fire

    # Cues
    fire_cue_tag: GameplayCue.Weapon.LaserFire
    fire_cue_tag_no_impact: GameplayCue.Weapon.LaserMiss

    # Damage
    damage_effect_class: GE_LaserDamage
    default_attack_damage: 75.0
    requires_ammo: true

    # Firearm Config
    is_automatic: true
    rate_of_fire: 0.15
    trace_distance: 20000.0
    trace_radius: 2.0

    # Event-based triggers
    ability_triggers:
      - trigger_tag: Event.Combat.Engage
        trigger_source: GameplayEvent

  # Melee attack with motion warping
  - name: GA_SwordSlash
    parent_class: GA_Melee_Unarmed
    folder: Abilities/Melee
    instancing_policy: InstancedPerActor
    net_execution_policy: ServerInitiated

    # Damage
    damage_effect_class: GE_MeleeDamage
    default_attack_damage: 100.0

    # Motion Warping
    should_warp: true
    warp_maintain_dist: 75.0
    min_warp_dist: 30.0
    max_warp_dist: 200.0

  # Execution ability
  - name: GA_Finisher
    parent_class: NarrativeGameplayAbility
    folder: Abilities/Special
    execution_gameplay_tag: Ability.Execution.Finisher
    execution_invulnerability: true
    damage_effect_class: GE_ExecutionDamage
    default_attack_damage: 500.0
```

---

## Narrative Event Enhancements

v7.8.52 adds Variables and EventGraph support to NarrativeEvent Blueprints, enabling custom logic in override functions.

### NE Variables Support

Blueprint variables for custom event data and logic.

**YAML Syntax:**
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
      - name: RewardItem
        type: Object
        class: NarrativeItem
      - name: bShowNotification
        type: Bool
        default_value: "true"
```

**Variable Properties:**

| Property | Type | Description |
|----------|------|-------------|
| `name` | string | Variable name |
| `type` | string | Variable type (Int, Float, Bool, Object, Class, etc.) |
| `class` | string | Class for Object/Class types |
| `default_value` | string | Default value |
| `instance_editable` | bool | Expose to Details panel |
| `blueprint_read_only` | bool | Read-only in Blueprints |

---

### NE EventGraph Support

Event graph for implementing override functions (ExecuteEvent, OnActivate, OnDeactivate).

**YAML Syntax:**
```yaml
narrative_events:
  - name: NE_GiveXP
    folder: Events
    event_runtime: End
    event_filter: OnlyPlayers
    variables:
      - name: XPAmount
        type: Int
        default_value: "100"
    event_graph: NE_GiveXP_EventGraph

event_graphs:
  - name: NE_GiveXP_EventGraph
    nodes:
      - id: ExecuteEvent
        type: Event
        properties:
          event_name: ExecuteEvent
      - id: GetXPComponent
        type: CallFunction
        properties:
          function: GetComponentByClass
          class: XPComponent
      - id: AddXP
        type: CallFunction
        properties:
          function: AddXP
          target_self: false
    connections:
      - from: [ExecuteEvent, Then]
        to: [GetXPComponent, Exec]
      - from: [ExecuteEvent, Target]
        to: [GetXPComponent, self]
      - from: [GetXPComponent, Then]
        to: [AddXP, Exec]
      - from: [GetXPComponent, ReturnValue]
        to: [AddXP, Target]
```

---

### NE Override Functions

The 5 overridable functions available in NarrativeEvent Blueprints:

| Function | Parameters | Purpose |
|----------|------------|---------|
| `ExecuteEvent` | Target (AActor*), Controller (AController*), NarrativeComponent (UTalesComponent*) | Main execution logic |
| `GetGraphDisplayText` | - | Display text for editor graph |
| `GetHintText` | - | Hint text for UI |
| `OnActivate` | - | Called when event activates |
| `OnDeactivate` | - | Called when event deactivates |

**ExecuteEvent Example:**
```yaml
event_graphs:
  - name: NE_CustomEvent_Graph
    nodes:
      - id: ExecuteEvent
        type: Event
        properties:
          event_name: ExecuteEvent
      # ExecuteEvent provides: Target, Controller, NarrativeComponent pins
      - id: PrintTarget
        type: PrintString
        properties:
          message: "Event executed on target!"
    connections:
      - from: [ExecuteEvent, Then]
        to: [PrintTarget, Exec]
```

---

### NE Conditions

Event conditions determine when the event can fire. Supports 19 condition types.

**YAML Syntax:**
```yaml
narrative_events:
  - name: NE_QuestComplete
    folder: Events
    event_runtime: End
    conditions:
      - type: IsQuestAtState
        properties:
          quest: Quest_MainStory
          state: Complete
      - type: HasItemInInventory
        not: true
        properties:
          item_class: EI_SpecialKey
```

**Condition Types:**

| Category | Condition | Properties |
|----------|-----------|------------|
| Appearance | `HasForm` | form |
| Dialogue | `HasDialogueNodePlayed` | dialogue, node |
| General | `CheckDifficulty` | difficulty |
| General | `CheckLevel` | level, comparison |
| General | `HasCompletedDataTask` | task |
| Interaction | `IsOccupyingInteractable` | interactable |
| Inventory | `HasItemInInventory` | item_class, quantity |
| Inventory | `IsItemEquipped` | item_class |
| Quests | `IsQuestAtState` | quest, state |
| Quests | `IsQuestInProgress` | quest |
| Quests | `IsQuestSucceeded` | quest |
| Quests | `IsQuestFailed` | quest |
| Quests | `IsQuestStartedOrFinished` | quest |
| Skill Trees | `HasPerk` | perk |
| Time of Day | `IsDayTime` | - |
| Time of Day | `IsTimeInRange` | start_time, end_time |
| UtilityAI | `HasFollowGoal` | - |
| UtilityAI | `HasInteractGoal` | - |

---

### Complete NarrativeEvent Example

```yaml
narrative_events:
  # Event that grants gold when quest completes
  - name: NE_QuestRewardGold
    folder: Events/Rewards
    event_runtime: End
    event_filter: OnlyPlayers
    party_policy: PartyLeader
    refire_on_load: false

    # Conditions - only fire if quest succeeded
    conditions:
      - type: IsQuestSucceeded
        properties:
          quest: Quest_FindArtifact

    # Custom variables
    variables:
      - name: GoldAmount
        type: Int
        default_value: "500"
        instance_editable: true

    # Event graph for custom logic
    event_graph: NE_QuestRewardGold_EventGraph

event_graphs:
  - name: NE_QuestRewardGold_EventGraph
    nodes:
      - id: ExecuteEvent
        type: Event
        properties:
          event_name: ExecuteEvent
      - id: GetInventory
        type: CallFunction
        properties:
          function: GetInventoryComponent
      - id: AddCurrency
        type: CallFunction
        properties:
          function: AddCurrency
    connections:
      - from: [ExecuteEvent, Then]
        to: [GetInventory, Exec]
      - from: [ExecuteEvent, Target]
        to: [GetInventory, self]
      - from: [GetInventory, Then]
        to: [AddCurrency, Exec]
```

---

## YAML Syntax Reference

### Complete Item Example

```yaml
equippable_items:
  - name: EI_LaserRifle
    folder: Items/Weapons
    parent_class: RangedWeaponItem
    display_name: "Laser Rifle"
    description: "A high-powered laser weapon."

    # Stats (dynamic variable binding)
    stats:
      - stat_display_name: Damage
        string_variable: Damage
        stat_tooltip: "Base weapon damage"
      - stat_display_name: Range
        string_variable: Range
        stat_tooltip: "Effective range in meters"

    # Weapon properties
    attack_damage: 50.0
    clip_size: 30
    required_ammo: EI_EnergyCells

    # Attachment configs (TMap format)
    holster_attachment_configs:
      Narrative.Equipment.Slot.Weapon.BackA:
        socket_name: Socket_BackA
        location: [0, 0, 30]
        rotation: [180, 0, 180]

    wield_attachment_configs:
      Narrative.Equipment.WieldSlot.Mainhand:
        socket_name: weapon_r
        location: [0, 0, 0]
        rotation: [-90, 0, 0]
```

### Complete Goal System Example

```yaml
# Goal Items
goal_items:
  - name: Goal_Attack
    folder: AI/Goals
    default_score: 100.0
    goal_lifetime: 30.0
    remove_on_succeeded: true
    save_goal: false
    owned_tags: [State.Attacking]
    block_tags: [State.Dead, State.Stunned]
    require_tags: [State.Alive]
    variables:
      - name: TargetActor
        type: Object
        class: Character
      - name: EngageDistance
        type: Float
        default_value: "500.0"
      - name: bMeleeAttack
        type: Bool
        default_value: "false"
    event_graph: EventGraph_AttackGoal

  - name: Goal_FollowPlayer
    folder: AI/Goals
    default_score: 50.0
    goal_lifetime: -1.0
    remove_on_succeeded: false
    variables:
      - name: FollowDistance
        type: Float
        default_value: "300.0"
      - name: FollowMode
        type: Byte
        default_value: "0"

# Goal Generators
goal_generators:
  - name: GoalGenerator_Attack
    folder: AI/GoalGenerators
    save_goal_generator: false
    variables:
      - name: QueryTemplate
        type: Object
        class: EnvQuery
        instance_editable: true
      - name: RunMode
        type: Byte
        default_value: "0"
      - name: AttackGoalClass
        type: Class
        class: NPCGoalItem
      - name: AttackAffiliationMap
        type: Object
        class: DataAsset
    event_graph: EventGraph_AttackGenerator
```

---

## Implementation Details

### Parser State Machine

**Goal Items Parsing:**
```
goal_items:
  ├── - name: Goal_X          → Create new FManifestGoalItemDefinition
  │   ├── folder:             → CurrentDef.Folder
  │   ├── parent_class:       → CurrentDef.ParentClass
  │   ├── default_score:      → CurrentDef.DefaultScore
  │   ├── variables:          → bInVariables = true
  │   │   └── - name: Var     → CurrentVariable.Name
  │   │       ├── type:       → CurrentVariable.Type
  │   │       └── class:      → CurrentVariable.Class
  │   └── event_graph:        → CurrentDef.EventGraph
```

**Goal Generators Parsing:**
```
goal_generators:
  ├── - name: GoalGenerator_X → Create new FManifestGoalGeneratorDefinition
  │   ├── folder:             → CurrentDef.Folder
  │   ├── parent_class:       → CurrentDef.ParentClass
  │   ├── save_goal_generator:→ CurrentDef.bSaveGoalGenerator
  │   ├── variables:          → bInVariables = true
  │   └── event_graph:        → CurrentDef.EventGraph
```

### Generator Pipeline

1. **Validation**: Check whitelist, compute hash, check metadata
2. **Parent Class**: Find UNPCGoalItem/UNPCGoalGenerator in Narrative Pro
3. **Blueprint Creation**: Factory creates Blueprint with parent class
4. **Variables**: Add via `FBlueprintEditorUtils::AddMemberVariable()`
5. **First Compile**: Compile after variables added
6. **CDO Setup**: Set base class properties (DefaultScore, OwnedTags, etc.)
7. **Event Graph**: If specified, lookup and generate via FEventGraphGenerator
8. **Final Compile**: Contract 10 compile gate
9. **Save & Cache**: Save package, cache class for same-session resolution

### Gameplay Cue Parsing

**Parser State Machine:**
```
gameplay_cues:
  ├── - name: GC_X               → Create new FManifestGameplayCueDefinition
  │   ├── folder:                → CurrentDef.Folder
  │   ├── parent_class:          → CurrentDef.ParentClass
  │   ├── gameplay_cue_tag:      → CurrentDef.GameplayCueTag
  │   ├── auto_attach_to_owner:  → CurrentDef.bAutoAttachToOwner
  │   ├── variables:             → bInVariables = true
  │   │   └── (standard var parsing)
  │   ├── gcn_defaults:          → bInGCNDefaults = true
  │   │   ├── default_spawn_condition:
  │   │   │   ├── locally_controlled_source: → SpawnCondition.LocallyControlledSource
  │   │   │   └── chance_to_play:           → SpawnCondition.ChanceToPlay
  │   │   └── default_placement_info:
  │   │       ├── socket_name:   → PlacementInfo.SocketName
  │   │       └── attach_policy: → PlacementInfo.AttachPolicy
  │   ├── gcn_effects:           → bInGCNEffects = true
  │   │   ├── burst_particles:   → bInBurstParticles = true
  │   │   │   └── - niagara_system: → Add FManifestBurstParticleEffect
  │   │   ├── burst_sounds:      → bInBurstSounds = true
  │   │   │   └── - sound:       → Add FManifestBurstSoundEffect
  │   │   ├── burst_camera_shake: → Parse FManifestBurstCameraShake
  │   │   ├── burst_camera_lens_effect: → Parse FManifestBurstCameraLensEffect
  │   │   ├── burst_force_feedback: → Parse FManifestBurstForceFeedback
  │   │   └── burst_decal:       → Parse FManifestBurstDecal
  │   ├── functions:             → bInFunctions = true
  │   │   └── - name: FuncX      → Add FManifestCueFunctionDefinition
  │   │       ├── replicates:    → Replicates (None/Server/Client/Multicast)
  │   │       ├── reliable:      → bReliable
  │   │       └── inputs/outputs: → Parameters
  │   └── event_graph:           → CurrentDef.EventGraph
```

### Files Modified (v7.8.52)

| File | Changes |
|------|---------|
| `GasAbilityGeneratorTypes.h` | FManifestGoalItemDefinition, FManifestGoalGeneratorDefinition, FManifestGameplayCueDefinition + 10 supporting structs |
| `GasAbilityGeneratorParser.h` | ParseGoalGenerators, ParseGameplayCues declarations |
| `GasAbilityGeneratorParser.cpp` | ParseGoalItems updated, ParseGoalGenerators, ParseGameplayCues (~400 lines) |
| `GasAbilityGeneratorGenerators.h` | FGoalItemGenerator, FGoalGeneratorGenerator, FGameplayCueGenerator classes |
| `GasAbilityGeneratorGenerators.cpp` | All three generators with full feature support |
| `GasAbilityGeneratorCommandlet.cpp` | GoalGenerators loop, GameplayCues loop |

---

## Narrative Pro Header Analysis

### NPCGoalItem.h Key Points

```cpp
// Base properties - all supported in generator
float DefaultScore;
float GoalLifetime;
bool bRemoveOnSucceeded;
bool bSaveGoal;
FGameplayTagContainer OwnedTags;
FGameplayTagContainer BlockTags;
FGameplayTagContainer RequireTags;

// Virtual functions - override via event_graph
virtual void InitializeGoalItem();
virtual void OnGoalSucceeded();
virtual void OnGoalFailed();
virtual float GetGoalScore();
```

### NPCGoalGenerator.h Key Points

```cpp
// Base properties - supported in generator
bool bSaveGoalGenerator;

// Virtual functions - override via event_graph
virtual void InitializeGoalGenerator();
virtual TArray<UNPCGoalItem*> GenerateGoals();
```

### WeaponItem.h Attachment System

```cpp
// TMap structure for attachments
TMap<FGameplayTag, FWeaponAttachmentConfig> HolsterAttachmentConfigs;
TMap<FGameplayTag, FWeaponAttachmentConfig> WieldAttachmentConfigs;

// Slot tags used as keys:
// Narrative.Equipment.Slot.Weapon.BackA
// Narrative.Equipment.Slot.Weapon.BackB
// Narrative.Equipment.WieldSlot.Mainhand
// Narrative.Equipment.WieldSlot.Offhand
```

---

## Quick Reference

### Variable Types Supported

| Type | YAML | UE Type |
|------|------|---------|
| Bool | `type: Bool` | bool |
| Byte | `type: Byte` | uint8 |
| Int | `type: Int` | int32 |
| Float | `type: Float` | float |
| String | `type: String` | FString |
| Name | `type: Name` | FName |
| Text | `type: Text` | FText |
| Vector | `type: Vector` | FVector |
| Rotator | `type: Rotator` | FRotator |
| Object | `type: Object` + `class: X` | UObject* |
| Class | `type: Class` + `class: X` | TSubclassOf<X> |
| Struct | `type: Struct` + `class: X` | Custom struct |

### Container Types

| Container | YAML | UE Type |
|-----------|------|---------|
| Single | (none) | T |
| Array | `container: Array` | TArray<T> |
| Set | `container: Set` | TSet<T> |
| Map | `container: Map` | TMap<K,V> |

### Asset Prefixes (v7.8.52)

| Prefix | Asset Type | Manifest Section |
|--------|------------|------------------|
| `EI_` | Equippable Items | `equippable_items:` |
| `Goal_` | Goal Items | `goal_items:` |
| `GoalGenerator_` | Goal Generators | `goal_generators:` |
| `GC_` | Gameplay Cues | `gameplay_cues:` |
| `BTS_` | BT Services | `bt_services:` |
| `BTTask_` | BT Tasks | `bt_tasks:` |

---

## Build Verification Status

### v7.8.52 Implementation Complete ✓

**Build Status:** SUCCESS (January 2026)

**Files Changed:**
| File | Lines Changed | Summary |
|------|---------------|---------|
| `GasAbilityGeneratorTypes.h` | +500 | GCN Effects (11 structs) + BT Service/Task (2 structs) + BPT struct + BPA enhancements + GE Enhancements (6 structs) |
| `GasAbilityGeneratorParser.h` | +5 | ParseGoalGenerators, ParseGameplayCues, ParseBTServices, ParseBTTasks, ParseBlueprintTriggers |
| `GasAbilityGeneratorParser.cpp` | +1100 | Full GCN Effects + BT Service/Task + BPT + BPA enhancements + GE Enhancement parsing |
| `GasAbilityGeneratorGenerators.h` | +40 | FGameplayCueGenerator, FBTServiceGenerator, FBTTaskGenerator, FBlueprintTriggerGenerator |
| `GasAbilityGeneratorGenerators.cpp` | +1000 | All generator implementations + BPT generator + BPA enhancements + GE execution/magnitude enhancements |
| `GasAbilityGeneratorCommandlet.cpp` | +48 | GameplayCues + BT Services/Tasks + BPT generation loops |
| `GasAbilityGeneratorWindow.cpp` | +12 | BPT generation loop |
| `CLAUDE.md` | +100 | YAML syntax documentation |

**v7.8.52 GE Enhancements (New):**
- `FManifestTagRequirement` - Per-modifier source/target tag requirements
- `FManifestAttributeBasedMagnitude` - Attribute capture with coefficients and curves
- `FManifestGEExecutionModifier` - Execution calculation modifier definition
- `FManifestConditionalGameplayEffect` - Conditional effect application
- `FManifestGEExecution` - Full execution definition with modifiers and conditionals
- Updated `FManifestModifierDefinition` with AttributeBased and SourceTags/TargetTags
- Updated `FManifestGameplayEffectDefinition` with Executions array and PeriodicInhibitionPolicy
- Generator support for NarrativeHealExecution and NarrativeDamageExecCalc
- Generator support for FAttributeBasedFloat magnitude type
- Generator support for per-modifier FGameplayTagRequirements
- Generator support for FConditionalGameplayEffect in executions

**v7.8.52 GA Enhancements (New):**
- `FManifestAbilityTriggerDefinition` - Event-based ability activation (TriggerTag + TriggerSource)
- `AbilityTriggers` TArray support - Multiple trigger events per ability
- Cues section: `FireCueTag`, `FireCueTagNoImpact` (FGameplayTag)
- Damage section: `DamageEffectClass`, `DefaultAttackDamage`, `bRequiresAmmo`, `bDrawDebugTraces`
- Firearm config: `bIsAutomatic`, `RateOfFire`, `BurstAmount`, `TraceDistance`, `TraceRadius`, `bTraceMulti`
- Motion warping: `bShouldWarp`, `WarpMaintainDist`, `MinWarpDist`, `MaxWarpDist`
- Execution config: `ExecutionGameplayTag`, `bExecutionInvulnerability`
- Updated `FManifestGameplayAbilityDefinition` with all 19 new fields
- Generator support for all NarrativeGameplayAbility CDO properties

**v7.8.52 NE (NarrativeEvent) Enhancements (New):**
- `Variables` TArray support - Blueprint variables for custom event logic
- `EventGraph` support - Override functions (ExecuteEvent, OnActivate, OnDeactivate, etc.)
- Variable parsing with type, default_value, instance_editable, class support
- Event graph generation with dependency deferral support
- Updated recompile condition to include variables and event graph
- Added ManifestData parameter to FNarrativeEventGenerator::Generate() for EventGraph lookup
- Updated call sites in Commandlet.cpp and Window.cpp to pass &ManifestData

**Duplicate Removal (Critical Fixes):**
- Removed old v4.0 FManifestGameplayCueDefinition struct (used CueType field)
- Removed old v4.0 supporting structs (SpawnCondition, Placement, BurstEffects)
- Removed duplicate GameplayCues array in FManifestData
- Removed duplicate ParseGameplayCues declaration
- Removed duplicate FGameplayCueGenerator class definition
- Removed old FGameplayCueGenerator::Generate implementation (310 lines)
- Fixed shift overflow in ComputeHash (cast GetTypeHash to uint64 before << 40)

### Struct Hierarchy (New v7.8.52)

**Gameplay Cue Structs:**
```
FManifestGameplayCueDefinition
├── FManifestGCNDefaults
│   ├── FManifestSpawnConditionOverride
│   └── FManifestPlacementInfoOverride
├── FManifestGCNEffects
│   ├── TArray<FManifestBurstParticleEffect>
│   │   ├── FManifestSpawnConditionOverride
│   │   └── FManifestPlacementInfoOverride
│   ├── TArray<FManifestBurstSoundEffect>
│   │   ├── FManifestSpawnConditionOverride
│   │   └── FManifestPlacementInfoOverride
│   ├── FManifestBurstCameraShake
│   ├── FManifestBurstCameraLensEffect
│   ├── FManifestBurstForceFeedback
│   └── FManifestBurstDecal
├── TArray<FManifestActorVariableDefinition>
└── TArray<FManifestCueFunctionDefinition>
```

**Gameplay Effect Enhancement Structs:**
```
FManifestGameplayEffectDefinition
├── TArray<FManifestModifierDefinition>
│   ├── FManifestAttributeBasedMagnitude    // v7.8.52
│   │   ├── Coefficient, PreMultiply, PostMultiply
│   │   ├── AttributeToCapture, AttributeSource
│   │   ├── bSnapshot
│   │   └── CurveTable, CurveRowName, CalculationType
│   ├── FManifestTagRequirement SourceTags  // v7.8.52
│   │   ├── MustHaveTags
│   │   └── MustNotHaveTags
│   └── FManifestTagRequirement TargetTags  // v7.8.52
├── TArray<FManifestGEExecution>             // v7.8.52
│   ├── CalculationClass
│   ├── TArray<FManifestGEExecutionModifier>
│   │   ├── CapturedAttribute, CapturedSource, CapturedStatus
│   │   ├── ModifierOp, MagnitudeType
│   │   ├── ScalableFloatValue, SetByCallerTag
│   │   └── FManifestAttributeBasedMagnitude
│   └── TArray<FManifestConditionalGameplayEffect>
│       ├── EffectClass
│       └── RequiredSourceTags
└── PeriodicInhibitionPolicy                 // v7.8.52
```

---

## Troubleshooting

### Common Build Errors

**C2011: struct redefinition**
- Cause: Duplicate struct definitions in Types.h
- Fix: Search for `struct FManifest*` and remove older version

**C2039: 'X' is not a member**
- Cause: Old code references removed struct fields
- Fix: Remove old function implementations referencing old field names

**C4293: shift count negative or too large**
- Cause: `GetTypeHash()` returns uint32, shifting by ≥32 bits is UB
- Fix: Cast to uint64 before shifting: `static_cast<uint64>(GetTypeHash(X)) << 40`

**C2535: member function already defined**
- Cause: Duplicate function declarations in header
- Fix: Search header for duplicate `static void ParseX(` declarations

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

**Document Version:** v7.8.52
**Last Updated:** January 31, 2026
**Build Verified:** ✓ SUCCESS (200 assets generated)
**Author:** Claude (Anthropic)

**v7.8.52 Changelog:**
- Added Blueprint Triggers (BPT_) generator support:
  - FManifestBlueprintTriggerDefinition struct with all 23 BPT task type properties
  - ParseBlueprintTriggers in Parser.cpp with location, time, dialogue, and item task parsing
  - FBlueprintTriggerGenerator with full CDO property support
  - Generation loops in Commandlet.cpp and Window.cpp
- Added Blueprint Activity (BPA_) enhancements:
  - AttackTaggedDialogue (FGameplayTag for combat dialogue)
  - WeaponTypes (TArray<TSubclassOf<UWeaponItem>>)
  - FollowGoal (TSubclassOf<UNPCGoalItem>)
  - InteractSubGoal (TSubclassOf<UNPCGoalItem>)
  - BBKeyFollowDistance (FName blackboard key)
  - Variables and EventGraph support
- Added BT Services (BTS_) and BT Tasks (BTTask_) generator support
- Added full GE enhancement support:
  - NarrativeHealExecution and NarrativeDamageExecCalc execution calculations
  - Attribute-based magnitude with coefficients and curve tables
  - Per-modifier source/target tag requirements
  - Conditional gameplay effects within executions
  - Periodic inhibition policy (NeverReset, ResetPeriod, ExecuteAndResetPeriod)
- Added NarrativeEvent enhancements:
  - Variables support (Blueprint variables via FBlueprintEditorUtils::AddMemberVariable)
  - EventGraph support (ExecuteEvent, OnActivate, OnDeactivate, GetGraphDisplayText, GetHintText overrides)
  - ManifestData parameter added to FNarrativeEventGenerator::Generate for event graph lookup
- Fixed duplicate GameplayCue generation loop in commandlet
- Fixed ComputeHash shift overflow (cast GetTypeHash to uint64)
- Fixed bBlueprintReadOnly parsing removal (field doesn't exist in FManifestActorVariableDefinition)
