# Generator Implementation Reference

**Consolidated:** 2026-01-15
**Plugin Version:** v4.3
**Status:** All generators IMPLEMENTED and operational

This document consolidates all implemented generator research/implementation handoffs into a single reference.

---

## Table of Contents

1. [Automation Coverage Summary](#1-automation-coverage-summary)
2. [Quest System (Quest_)](#2-quest-system)
3. [Dialogue System (DBP_)](#3-dialogue-system)
4. [Goal & Schedule System](#4-goal--schedule-system)
5. [NPC Package System](#5-npc-package-system)
6. [Item Pipeline](#6-item-pipeline)
7. [POI & Spawner Placement](#7-poi--spawner-placement)
8. [Widget Blueprint (WBP_)](#8-widget-blueprint-v43)
9. [Faction System](#9-faction-system)

---

## 1. Automation Coverage Summary

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

---

## 2. Quest System

### Generator
`FQuestGenerator` in GasAbilityGeneratorGenerators.cpp

### Manifest Structure
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

### Key Classes
- `UQuestBlueprint` - Quest asset type
- `UQuestState` - State nodes (Regular, Success, Failure)
- `UQuestBranch` - Transitions between states
- `UNarrativeTask` (BPT_*) - Task instances

### Supported Tasks
BPT_FindItem, BPT_FinishDialogue, BPT_Move, BPT_KillEnemy, BPT_InteractWithObject, BPT_WaitGameplayEvent

---

## 3. Dialogue System

### Generator
`FDialogueBlueprintGenerator` in GasAbilityGeneratorGenerators.cpp

### Manifest Structure
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

### Key Features
- Two-pass node creation and wiring
- NPC/Player node types with full properties
- Events (NE_*) and Conditions (NC_*)
- Quest shortcuts: start_quest, complete_quest_branch, fail_quest
- 12+ CDO properties (FreeMovement, Priority, etc.)

### CSV Pipeline (v4.0)
`FDialogueCSVParser` supports batch dialogue creation from spreadsheets.
Columns: Dialogue, NodeID, Type, Speaker, Text, OptionText, Replies, Conditions, Events

---

## 4. Goal & Schedule System

### Goal Generator
`FGoalItemGenerator` creates Goal_ Blueprint assets (UNPCGoalItem)

```yaml
goal_items:
  - name: Goal_DefendForge
    default_score: 75.0
    goal_lifetime: -1.0
    owned_tags: [State.Defending]
    block_tags: [State.Fleeing]
```

### Schedule Generator
`FActivityScheduleGenerator` creates Schedule_ DataAssets (UNPCActivitySchedule)

```yaml
activity_schedules:
  - name: Schedule_BlacksmithDay
    behaviors:
      - start_time: 600      # 6:00 AM
        end_time: 1800       # 6:00 PM
        goal_class: Goal_Work
        location: Forge
```

### Time Format
0-2400 where 100 = 1 hour (e.g., 600 = 6:00 AM, 1800 = 6:00 PM)

### Helper Class
`UScheduledBehavior_AddNPCGoalByClass` - Concrete helper for goal-based scheduling

---

## 5. NPC Package System

### Generator
`FNPCDefinitionGenerator` with auto-create support

### Manifest Structure
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

### Auto-Create Features
- `auto_create_dialogue` → DBP_{NPCName}Dialogue
- `auto_create_tagged_dialogue` → {NPCName}_TaggedDialogue
- `auto_create_item_loadout` → Populates DefaultItemLoadout

### Loot Table Roll Support
Full `TArray<FLootTableRoll>` population with items, quantities, collections, chance, num_rolls.

---

## 6. Item Pipeline

### Generator
`FEquippableItemGenerator` with weapon property chains

### Pipeline
`FPipelineProcessor` converts mesh files to EI_ assets:
1. Scan mesh folder for SM_/SK_ assets
2. Create EI_ DataAsset per mesh
3. Populate collections (IC_)
4. Link to NPC loadouts

### Weapon Support
- MeleeWeaponItem properties
- RangedWeaponItem properties (spread, recoil, etc.)
- Weapon attachments (holster_attachments, wield_attachments)

---

## 7. POI & Spawner Placement

### Generators
- `FPOIPlacementGenerator` - Places APOIActor in levels
- `FNPCSpawnerPlacementGenerator` - Places ANPCSpawner actors

### Manifest Structure
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

### Commandlet Flag
`-level="/Game/Maps/MainWorld"` loads world for actor placement

---

## 8. Widget Blueprint (v4.3)

### Generator
`FWidgetBlueprintGenerator` with widget tree construction

### Manifest Structure
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

### Supported Widget Types (25+)
CanvasPanel, VerticalBox, HorizontalBox, Overlay, Border, Button, TextBlock, RichTextBlock, Image, ProgressBar, Slider, CheckBox, EditableText, EditableTextBox, ComboBox, Spacer, SizeBox, ScaleBox, ScrollBox, UniformGridPanel, GridPanel, WidgetSwitcher, Throbber, CircularThrobber, NativeWidgetHost

### Slot Properties
- Canvas: anchors, position, size, alignment, auto_size
- Box: h_align, v_align, size_rule, fill_weight, padding

---

## 9. Faction System

### Status
**No generator needed** - Factions use FGameplayTag with runtime relationships.

### Storage
- Tags defined in DefaultGameplayTags.ini
- Relationships in `ANarrativeGameState::FactionAllianceMap`
- Assigned via NPCDefinition.DefaultFactions

### Existing Support
- Tag generator handles Narrative.Factions.* tags
- NPC generator populates DefaultFactions array

---

## Original Documents (Consolidated)

- Quest_Generator_MEGA_Handoff.md
- Quest_Pipeline_Handoff_v1_0.md
- Dialogue_Automation_Implementation_Handoff.md
- TERMINAL1_Goal_Schedule_Implementation.md
- NPC_Package_Generator_Handoff.md
- Item_Pipeline_MEGA_Handoff.md
- POI_Spawner_Automation_Handoff.md
- Faction_System_Handoff.md
- NPC_Schema_Specification.md
