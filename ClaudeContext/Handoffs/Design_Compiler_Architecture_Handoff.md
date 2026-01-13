# Design Compiler Architecture Handoff

## Overview

This document captures the complete design for a "Design Compiler" - a pre-processing stage that transforms high-level, human-friendly game design specs into the flat manifest.yaml consumed by GasAbilityGenerator.

**Status:** Deferred (captured for future implementation)
**Priority:** Low (convenience, not capability)
**Prerequisite:** Complete core Narrative Pro generators first (Factions, Encounters)

---

## Architecture Diagram

```
Current Pipeline (v3.9.9):
┌─────────────────┐     ┌────────────────┐     ┌─────────────┐
│  manifest.yaml  │ ──► │   Generators   │ ──► │  UE Assets  │
│  (single file)  │     │  (25+ types)   │     │             │
└─────────────────┘     └────────────────┘     └─────────────┘

Proposed Pipeline (with Compiler):
┌─────────────────────────────────────┐
│  DesignSpec/                        │
│    globals/                         │
│      gameplay_tags.yaml             │
│      attributes.yaml                │
│      factions.yaml                  │
│    items/                           │
│      weapons.yaml                   │
│      consumables.yaml               │
│    abilities/                       │
│      abilities_player.yaml          │
│      abilities_faction_bandits.yaml │
│    npcs/                            │
│      templates.yaml                 │
│      archetypes.yaml                │
│      unique_npcs.yaml               │
│    quests/                          │
│      q_main_01.yaml                 │
│      q_side_bandits.yaml            │
│    dialogue/                        │
│      dlg_doctor.yaml                │
│      dlg_bandit_barks.yaml          │
│    world/                           │
│      locations.yaml                 │
│      encounters.yaml                │
└─────────────────────────────────────┘
                 │
                 ▼
        ┌─────────────────┐
        │    Compiler     │
        │  (resolve refs, │
        │   templates,    │
        │   validation)   │
        └─────────────────┘
                 │
                 ▼
        ┌─────────────────┐
        │  manifest.yaml  │  (flattened, resolved)
        └─────────────────┘
                 │
                 ▼
        ┌─────────────────┐
        │   Generators    │
        └─────────────────┘
                 │
                 ▼
        ┌─────────────────┐
        │   UE Assets     │
        └─────────────────┘
```

---

## GPT's Original Unified Spec Format

### Core Conventions

#### Stable IDs
```yaml
# IDs never change - version instead of rename
fac_bandits, npc_arch_bandit_grunt, ab_dash, wep_ar_01, q_main_01, dlg_doc_intro

# If redesign needed:
ab_dash_v2  # NOT renaming ab_dash
```

#### References
All cross-links use `_id` or `_ids` suffix:
```yaml
abilities: [ab_dash, ab_grenade]      # wrong
ability_ids: [ab_dash, ab_grenade]    # correct

weapon: wep_ar_01                      # wrong
weapon_id: wep_ar_01                   # correct
```

#### Localization (TR/EN)
```yaml
# Inline format (recommended for now)
name: {en: "Bandits", tr: "Haydutlar"}
description: {en: "Raiders.", tr: "Yagmacilar."}

# OR loc_key format (for full loc pipeline later)
name_loc: "FACTION_BANDITS_NAME"
```

#### Tags as Lingua Franca
```yaml
# Factions
Faction.Bandits, Faction.Corp

# Quest state
Quest.Main01.Stage.02

# Dialogue flags
Flag.HelpedDoctor

# Ability taxonomy
Ability.Weapon.Grenade, Ability.Tech.Cloak
```

---

### Complete Schema Definitions

#### 1. gameplay_tags.yaml
```yaml
meta: {spec_version: 1, game_id: game_openworld}

gameplay_tags:
  - "Faction.Bandits"
  - "Faction.Corp"
  - "Ability.Movement.Dash"
  - "Quest.Main01.Stage.S2"
  - "Damage.Ballistic"
  - "Damage.Explosive"
```

#### 2. enums.yaml
```yaml
enums:
  - id: e_combat_range
    name: {en: "Combat Range", tr: "Catisma Menzili"}
    values:
      - {id: close, name: {en: "Close", tr: "Yakin"}}
      - {id: mid,   name: {en: "Mid", tr: "Orta"}}
      - {id: long,  name: {en: "Long", tr: "Uzak"}}

  - id: e_weapon_hand
    name: {en: "Weapon Hand", tr: "Silah Eli"}
    values:
      - {id: two_handed,     name: {en: "Two Handed", tr: "Cift El"}}
      - {id: main_hand,      name: {en: "Main Hand", tr: "Ana El"}}
      - {id: off_hand,       name: {en: "Off Hand", tr: "Yan El"}}
      - {id: dual_wieldable, name: {en: "Dual Wieldable", tr: "Cift Kullanim"}}
```

#### 3. attributes.yaml
```yaml
attributes:
  - id: attr_core
    name: {en: "Core Attributes", tr: "Temel Nitelikler"}
    unreal:
      attribute_set_class: "/Game/GAS/AS_Core.AS_Core_C"
    fields:
      - {name: Health,    type: float, clamp: {min: 0, max: 9999}}
      - {name: Stamina,   type: float, clamp: {min: 0, max: 9999}}
      - {name: Shield,    type: float, clamp: {min: 0, max: 9999}}
      - {name: MoveSpeed, type: float, clamp: {min: 0, max: 2000}}
```

#### 4. factions.yaml
```yaml
factions:
  - id: fac_bandits
    name: {en: "Bandits", tr: "Haydutlar"}
    description: {en: "Raiders and smugglers.", tr: "Yagmacilar ve kacakcilar."}
    tags: ["Faction.Bandits"]
    relationships:
      default_disposition:
        fac_corp: hostile
        fac_civilians: hostile
    reputation:
      min: -100
      max: 100
      thresholds:
        - {value: -50, state: hostile}
        - {value:  10, state: neutral}
        - {value:  50, state: friendly}
    ai_doctrine:
      bravery: 0.6
      teamwork: 0.4
      accuracy: 0.35
      preferred_ranges: ["close", "mid"]
    loot_profile_id: loot_bandits_common

  - id: fac_corp
    name: {en: "Corporate Security", tr: "Sirket Guvenligi"}
    tags: ["Faction.Corp"]
    relationships:
      default_disposition:
        fac_bandits: hostile
        fac_civilians: friendly
    ai_doctrine:
      bravery: 0.75
      teamwork: 0.75
      accuracy: 0.55
      preferred_ranges: ["mid", "long"]
```

#### 5. items.yaml / weapons.yaml
```yaml
items:
  - id: it_medkit_small
    name: {en: "Small Medkit", tr: "Kucuk Ilk Yardim"}
    description: {en: "Restores health.", tr: "Can yeniler."}
    tags: ["Item.Consumable.Medkit"]
    type: consumable
    stack: {max: 5}
    use:
      ability_id: ab_use_medkit_small
      cooldown_s: 2.0

weapons:
  - id: wep_ar_01
    name: {en: "AR-01", tr: "AR-01"}
    tags: ["Item.Weapon.AR", "Damage.Ballistic"]
    type: firearm
    firearm:
      ammo_type_id: ammo_556
      mag_size: 30
      fire_modes: ["auto", "burst"]
      rate_of_fire_rpm: 700
      recoil_profile_id: recoil_ar_01
      spread_profile_id: spread_ar_01
      damage:
        per_shot: 22
        damage_type_tag: "Damage.Ballistic"
        falloff:
          start_m: 25
          end_m: 80
    attachments:
      slots: ["optic", "muzzle", "underbarrel"]
```

#### 6. abilities.yaml (GAS Bundle)
```yaml
abilities:
  - id: ab_dash
    name: {en: "Dash", tr: "Atilma"}
    description: {en: "Quick burst movement.", tr: "Kisa sureli hiz patlamasi."}
    tags:
      ability_tags: ["Ability.Movement.Dash"]
      activation_required: []
      activation_blocked: ["State.Rooted"]
    input:
      action_id: ia_dash
      default_key: "LeftShift"
    gas:
      ga:
        asset_name: "GA_Dash"
        output_path: "/Game/GAS/Abilities/Generated"
        instancing_policy: "InstancedPerActor"
        net_execution: "LocalPredicted"
      costs:
        - id: cost_stamina_20
          type: attribute
          attribute: "Stamina"
          op: "Add"
          magnitude: -20
          ge_id: ge_cost_stamina_20
      cooldown:
        seconds: 3.5
        ge_id: ge_cd_dash_3_5
        tags_granted: ["Cooldown.Dash"]
      effects_on_activate:
        - ge_id: ge_dash_impulse
    cues:
      - cue_tag: "GameplayCue.Ability.Dash"
        niagara:
          mode: template
          template_system: "/Game/VFX/Templates/NS_Dash"
    ai:
      can_use: true
      score_model:
        base: 0.2
        add:
          - {when: {state: UnderFire}, value: 0.6}
          - {when: {health_pct_below: 0.35}, value: 0.7}
        block:
          - {when: {stamina_pct_below: 0.15}}

gameplay_effects:
  - id: ge_cd_dash_3_5
    type: cooldown
    duration: {policy: HasDuration, seconds: 3.5}
    granted_tags: ["Cooldown.Dash"]

  - id: ge_cost_stamina_20
    type: cost
    duration: {policy: Instant}
    modifiers:
      - {attribute: "Stamina", op: Add, magnitude: -20}
```

#### 7. npc_archetypes.yaml
```yaml
npc_archetypes:
  - id: npc_arch_bandit_grunt
    name: {en: "Bandit Grunt", tr: "Haydut Piyade"}
    faction_id: fac_bandits
    tags: ["AI.Role.Grunt", "Faction.Bandits"]
    pawn:
      blueprint: "/Game/NPC/BP_BanditGrunt"
      movement:
        walk_speed: 320
        run_speed: 520
    attributes:
      set_id: attr_core
      base: {Health: 120, Stamina: 80, Shield: 0, MoveSpeed: 600}
    loadout:
      primary_weapon_id: wep_ar_01
      items:
        - {item_id: it_medkit_small, qty: 1}
    abilities:
      granted_ids: [ab_dash, ab_grenade_frag]
      ai_priority:
        ab_grenade_frag: 0.7
        ab_dash: 0.4
    ai:
      blackboard_id: bb_humanoid_combat
      behavior_tree_id: bt_bandit_combat
      perception: {sight_range: 1800, hearing_range: 1200}
      combat:
        preferred_range: mid
        retreat_below_health_pct: 0.2
    dialogue:
      bark_set_id: dlg_bandit_barks

unique_npcs:
  - id: npc_doctor_aylin
    name: {en: "Dr. Aylin", tr: "Dr. Aylin"}
    faction_id: fac_civilians
    tags: ["NPC.QuestGiver", "NPC.Merchant.Meds"]
    pawn: {blueprint: "/Game/NPC/BP_DoctorAylin"}
    dialogue: {conversation_set_ids: [dlg_doc_intro]}
    vendor: {shop_id: shop_meds_basic}
    quest_roles:
      gives_quest_ids: [q_main_01]
```

#### 8. blackboards.yaml / behavior_trees.yaml
```yaml
blackboards:
  - id: bb_humanoid_combat
    keys:
      - {name: TargetActor, type: Object, base_class: Actor}
      - {name: LastKnownLocation, type: Vector}
      - {name: IsAlerted, type: Bool}
      - {name: CombatRange, type: Enum, enum_id: e_combat_range}

behavior_trees:
  - id: bt_bandit_combat
    root:
      type: Selector
      children:
        - type: Sequence
          name: {en: "Engage Target", tr: "Hedefe Saldir"}
          decorators:
            - {type: BlackboardBool, key: IsAlerted, is: true}
          children:
            - {type: Task, task_id: task_pick_combat_position}
            - {type: Task, task_id: task_attack_target}
        - type: Sequence
          name: {en: "Patrol", tr: "Devriye"}
          children:
            - {type: Task, task_id: task_patrol_route}
```

#### 9. quests.yaml
```yaml
quests:
  - id: q_main_01
    name: {en: "First Blood", tr: "Ilk Kan"}
    category: main
    giver_npc_id: npc_doctor_aylin
    tags: ["Quest.Main01"]
    rewards:
      xp: 250
      items: [{item_id: it_medkit_small, qty: 2}]
    variables:
      - {id: v_bandits_killed, type: int, default: 0}
    stages:
      - id: S1
        name: {en: "Meet the Doctor", tr: "Doktorla Konus"}
        objectives:
          - id: O1_Talk
            type: talk_to_npc
            npc_id: npc_doctor_aylin
            on_complete:
              - {event: Quest.AdvanceStage, quest_id: q_main_01, to_stage: S2}

      - id: S2
        name: {en: "Clear the Ambush", tr: "Pusuyu Temizle"}
        objectives:
          - id: O2_Kill
            type: kill
            target_tags: ["KillTarget.Bandit"]
            count: 6
            progress_var: v_bandits_killed
            on_complete:
              - {event: Quest.AdvanceStage, quest_id: q_main_01, to_stage: S3}

      - id: S3
        name: {en: "Return to Doctor", tr: "Doktora Don"}
        objectives:
          - id: O3_Return
            type: talk_to_npc
            npc_id: npc_doctor_aylin
            on_complete:
              - {event: Quest.Complete, quest_id: q_main_01}
```

#### 10. dialogue.yaml
```yaml
dialogue_sets:
  - id: dlg_doc_intro
    participants:
      - {role: npc, npc_id: npc_doctor_aylin}
      - {role: player}
    entry_node_id: n1
    nodes:
      - id: n1
        speaker: npc
        text: {en: "I need your help.", tr: "Yardimina ihtiyacim var."}
        responses:
          - id: r1
            text: {en: "What's happening?", tr: "Ne oluyor?"}
            next_node_id: n2
          - id: r2
            text: {en: "No.", tr: "Hayir."}
            next_node_id: n_end

      - id: n2
        speaker: npc
        text: {en: "Bandits ambushed my route.", tr: "Haydutlar yolumu pusuya dusurdu."}
        responses:
          - id: r_accept
            text: {en: "I'm in.", tr: "Tamam."}
            actions:
              - {type: give_quest, quest_id: q_main_01}
              - {type: set_quest_stage, quest_id: q_main_01, stage_id: S1}
            next_node_id: n_end

      - id: n_end
        speaker: npc
        text: {en: "Be careful.", tr: "Dikkatli ol."}
        end: true
```

#### 11. locations.yaml / encounters.yaml
```yaml
locations:
  - id: loc_ambush_road_01
    name: {en: "Ambush Road", tr: "Pusu Yolu"}
    region_id: reg_valley
    tags: ["Location.Ambush"]
    unreal:
      volume_path: "/Game/World/Volumes/V_AmbushRoad01"

encounters:
  - id: enc_bandit_ambush_01
    location_id: loc_ambush_road_01
    activation:
      conditions:
        - {type: quest_stage_is, quest_id: q_main_01, stage_id: S2}
    spawns:
      - {npc_archetype_id: npc_arch_bandit_grunt, count: 6}
    on_clear:
      loot_table_id: loot_bandits_common
```

---

## Compiler Features

### 1. Include System
```yaml
# master.yaml
meta: {spec_version: 1}

include:
  - globals/gameplay_tags.yaml
  - globals/factions.yaml
  - abilities/player.yaml
  - npcs/archetypes.yaml
  - quests/main.yaml
```

Compiler merges all included files into single manifest.

### 2. Templates & Inheritance
```yaml
# templates.yaml
templates:
  - id: tpl_bandit_base
    faction_id: fac_bandits
    level_range: [1, 5]
    tags: [State.Hostile]

# bandits.yaml
npcs:
  - id: npc_bandit_grunt
    extends: tpl_bandit_base      # inherits all fields
    name: {en: "Grunt", tr: "Piyade"}
    health: 120

  - id: npc_bandit_elite
    extends: tpl_bandit_base
    name: {en: "Elite", tr: "Elit"}
    health: 200
    level_range: [5, 10]          # override inherited field
```

### 3. Variables & Constants
```yaml
# constants.yaml
constants:
  BANDIT_BASE_HEALTH: 100
  BANDIT_BASE_DAMAGE: 15
  CORP_BASE_HEALTH: 150

# bandits.yaml
npcs:
  - id: npc_bandit_grunt
    health: $BANDIT_BASE_HEALTH
    damage: $BANDIT_BASE_DAMAGE

  - id: npc_bandit_heavy
    health: $BANDIT_BASE_HEALTH * 1.5    # expressions supported
    damage: $BANDIT_BASE_DAMAGE * 1.2
```

### 4. Conditional Blocks
```yaml
# For build variants (demo vs full)
quests:
  - id: q_main_01
    # always included

  - id: q_side_advanced
    if: $BUILD_TYPE != "demo"     # only in full build
```

### 5. Pre-Generation Validation

Compiler validates before outputting manifest:

| Rule | Example |
|------|---------|
| All `_id` references exist | `faction_id: fac_bandits` - fac_bandits must be defined |
| No circular dependencies | ability → effect → ability not allowed |
| Localization complete | Every `name:` has both `en:` and `tr:` |
| Tags exist | All referenced tags in `gameplay_tags` section |
| Enum values valid | `combat_range: close` - close must be in enum |

---

## Implementation Plan (When Ready)

### Option A: Python Compiler (Recommended)
- ~500-800 lines of Python
- Easy to iterate and debug
- Run as pre-build step: `python compile_specs.py → manifest.yaml`

```python
# compile_specs.py (pseudocode)
class DesignCompiler:
    def __init__(self):
        self.entities = {}
        self.templates = {}
        self.constants = {}

    def load_file(self, path):
        """Load YAML, process includes recursively"""

    def resolve_templates(self):
        """Apply 'extends' inheritance"""

    def resolve_variables(self):
        """Replace $CONSTANTS with values"""

    def validate(self):
        """Check all references, localization, etc."""

    def emit_manifest(self, output_path):
        """Write flat manifest.yaml for generators"""
```

### Option B: C++ Compiler (In Plugin)
- Integrated into commandlet
- No external dependencies
- More complex to maintain

### Option C: Lightweight Include Only
- Just add include support to existing parser
- ~50 lines of C++ in FGasAbilityGeneratorParser
- No templates/variables/validation
- Good intermediate step

```cpp
// In ParseManifest()
if (IsSectionHeader(Line, TEXT("include:")))
{
    // Parse include paths, load and merge each file
    for (const FString& IncludePath : IncludePaths)
    {
        FString IncludeContent;
        FFileHelper::LoadFileToString(IncludeContent, *IncludePath);
        ParseManifest(IncludeContent, OutData); // Recursive merge
    }
}
```

---

## Mapping to Current Generators

| Spec Section | Generator | Output |
|--------------|-----------|--------|
| `gameplay_tags` | FTagGenerator | DefaultGameplayTags.ini |
| `enums` | FEnumerationGenerator | E_* DataAssets |
| `factions` | FFactionDatabaseGenerator (NEW) | UFactionDatabase |
| `items` | FEquippableItemGenerator | EI_* DataAssets |
| `weapons` | FEquippableItemGenerator | EI_* with weapon properties |
| `abilities` | FGameplayAbilityGenerator | GA_* Blueprints |
| `gameplay_effects` | FGameplayEffectGenerator | GE_* Blueprints |
| `npc_archetypes` | FNPCDefinitionGenerator | NPCDef_* DataAssets |
| `unique_npcs` | FNPCDefinitionGenerator | NPCDef_* DataAssets |
| `blackboards` | FBlackboardGenerator | BB_* DataAssets |
| `behavior_trees` | FBehaviorTreeGenerator | BT_* DataAssets |
| `dialogue_sets` | FDialogueBlueprintGenerator | DBP_* Blueprints |
| `quests` | FQuestGenerator | Quest_* Blueprints |
| `locations` | FPOIPlacementGenerator | APOIActor in level |
| `encounters` | FNPCSpawnerPlacementGenerator | ANPCSpawner in level |

---

## When to Implement

**Prerequisites:**
1. All core generators complete (Factions, Encounters with conditions)
2. Manifest exceeds ~2000 lines
3. Multiple designers need to edit simultaneously
4. Template/inheritance patterns become obvious

**Estimated Effort:**
- Lightweight includes: 1-2 hours
- Full Python compiler: 1-2 days
- Full C++ compiler: 3-5 days

---

## References

- GPT conversation: Comprehensive unified spec format proposal
- Claude analysis: Cost/benefit breakdown, implementation options
- Current manifest: `ClaudeContext/manifest.yaml`
- Parser: `Source/GasAbilityGenerator/Private/GasAbilityGeneratorParser.cpp`
