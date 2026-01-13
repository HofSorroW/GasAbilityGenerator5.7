# NPC Schema Specification v1.0

## Status: READY

## Overview

Comprehensive YAML schema for defining complete NPCs with all Narrative Pro systems: dialogue, quests, goals, schedules, inventory, factions, and AI behavior.

## Production Modes

| Mode | Input | Trigger | Use Case |
|------|-------|---------|----------|
| **Batch** | `.npc.yaml` files in `ClaudeContext/NPCs/` | Commandlet with `-npc` flag | Mass NPC creation |
| **Editor UI** | Form fields in plugin window | "Create NPC" button | Quick single NPC |
| **Prompt** | Claude Code conversation | Chat command | Iterative design |

All modes share the same generators - only the input source differs.

---

## Schema Structure

```yaml
# ============================================================================
# NPC DEFINITION FILE (.npc.yaml)
# ============================================================================
# Single file defines complete NPC with all related assets
# Generated assets: NPCDef_, CD_, DBP_, Quest_, Goal_, Schedule_, BPA_, etc.

npc:
  # --------------------------------------------------------------------------
  # IDENTITY (Required)
  # --------------------------------------------------------------------------
  id: string                    # Unique ID (e.g., "Blacksmith_01")
  name: string                  # Display name (e.g., "Garrett the Blacksmith")

  # NPC Type determines which sections are relevant
  type: enum                    # vendor | quest_giver | enemy | companion | ambient

  # Output folder for generated assets
  folder: string                # e.g., "NPCs/Town" -> /Game/{Project}/NPCs/Town/

  # Blueprint class (if custom BP exists, otherwise generates one)
  blueprint: string             # Optional: "BP_Blacksmith" or leave empty to auto-generate

  # --------------------------------------------------------------------------
  # CHARACTER PROPERTIES (Maps to UCharacterDefinition / UNPCDefinition)
  # --------------------------------------------------------------------------
  character:
    level_range: [int, int]     # [min, max] e.g., [1, 10]
    attack_priority: float      # 0.0-1.0, higher = more likely target

    # Inventory
    default_currency: int       # Starting gold
    item_loadout:               # Items granted on spawn
      - collection: string      # IC_WeaponStock
        quantity: int           # Optional, default 1

    # Tags and Factions
    owned_tags:                 # FGameplayTagContainer
      - string                  # e.g., "State.Invulnerable"
    factions:                   # FGameplayTagContainer
      - string                  # e.g., "Narrative.Factions.Town"

    # Appearance
    appearance: string          # UCharacterAppearanceBase asset name

    # Abilities
    ability_config: string      # AC_Blacksmith
    activity_config: string     # ActConfig_Blacksmith

    # Unique NPC (only one instance allowed)
    unique: bool                # Default: true for named NPCs

  # --------------------------------------------------------------------------
  # VENDOR CONFIG (Only if type: vendor)
  # --------------------------------------------------------------------------
  vendor:
    shop_name: string           # "Garrett's Forge"
    buy_percentage: float       # 0.5 = pays 50% of item value
    sell_percentage: float      # 1.5 = charges 150% of item value
    trading_currency: int       # Shop's starting gold
    inventory:                  # Items for sale
      - collection: string      # IC_WeaponStock
      - collection: string      # IC_ArmorStock

  # --------------------------------------------------------------------------
  # SCHEDULE (Daily routine - generates UNPCActivitySchedule)
  # --------------------------------------------------------------------------
  schedule:
    - time: [float, float]      # [start_hour, end_hour] in 24h format
      goal: string              # Goal class to add (Goal_Work, Goal_Sleep)
      score: float              # Optional: override goal score
      location: string          # Optional: location tag or actor name

    # Example: Full day schedule
    # - time: [6, 12]
    #   goal: Goal_Work
    #   location: Forge
    # - time: [12, 13]
    #   goal: Goal_Eat
    #   location: Tavern
    # - time: [13, 18]
    #   goal: Goal_Work
    #   location: Forge
    # - time: [22, 6]
    #   goal: Goal_Sleep
    #   location: Home

  # --------------------------------------------------------------------------
  # GOALS (AI objectives - generates UNPCGoalItem blueprints)
  # --------------------------------------------------------------------------
  goals:
    - id: string                # Goal_DefendForge
      base_class: string        # Parent class (default: NPCGoalItem)
      default_score: float      # Priority score
      lifetime: float           # -1 = never expires
      remove_on_success: bool   # Auto-remove when completed
      save_goal: bool           # Persist across saves
      owned_tags:               # Tags granted while active
        - string
      block_tags:               # Tags that block this goal
        - string
      require_tags:             # Tags required to act on goal
        - string

  # --------------------------------------------------------------------------
  # GOAL GENERATORS (Dynamic goal creation - generates UNPCGoalGenerator)
  # --------------------------------------------------------------------------
  goal_generators:
    - id: string                # GoalGenerator_FindTarget
      base_class: string        # Parent class
      save_generator: bool      # Persist across saves
      # Custom properties set via reflection
      properties:
        key: value

  # --------------------------------------------------------------------------
  # DIALOGUE (Conversation tree - generates UDialogueBlueprint via v3.8)
  # --------------------------------------------------------------------------
  dialogue:
    # Basic dialogue properties
    free_movement: bool         # Allow player movement
    can_be_exited: bool         # ESC to exit
    priority: int               # Higher = more important

    # Dialogue tree (v3.8 format)
    tree:
      root: string              # ID of starting node
      nodes:
        - id: string
          type: enum            # npc | player
          speaker: string       # NPCDef reference for NPC nodes
          text: string          # Dialogue line
          option_text: string   # Player choice text (player nodes)
          duration: string      # auto | manual | <seconds>
          skippable: bool

          # Branching
          npc_replies: [string] # IDs of NPC response nodes
          player_replies: [string] # IDs of player choice nodes

          # Events fired during this node
          events:
            - type: string      # Event class name
              runtime: enum     # Start | End | Both
              properties:
                key: value

          # Conditions for node visibility
          conditions:
            - type: string      # Condition class name
              invert: bool      # Negate result
              properties:
                key: value

          # Alternative lines (random selection)
          alternatives:
            - text: string
              audio: string

  # --------------------------------------------------------------------------
  # TAGGED DIALOGUE (Contextual barks)
  # --------------------------------------------------------------------------
  barks:
    - tag: string               # Greeting.Friendly, Combat.Taunt
      lines:
        - text: string
          audio: string         # Optional audio asset
          weight: float         # Selection weight (default: 1.0)

  # --------------------------------------------------------------------------
  # QUESTS (Quest chains this NPC gives - generates UQuest blueprints)
  # --------------------------------------------------------------------------
  quests:
    - id: string                # Quest_ForgeSupplies
      name: string              # "Forge Supplies"
      description: string       # Quest journal text

      # Quest dialogue (optional - embedded in quest)
      dialogue: string          # DBP_ForgeSuppliesQuest

      # Quest structure (state machine)
      states:
        - id: string            # state_start, state_gather, state_complete
          type: enum            # regular | success | failure
          description: string   # State description for journal

      branches:
        - id: string            # branch_accept, branch_gather_ore
          from_state: string    # Source state ID
          to_state: string      # Destination state ID
          hidden: bool          # Hide from UI

          # Tasks required to take this branch
          tasks:
            - type: string      # Task class (GoToLocation, FindItem, etc.)
              quantity: int     # Required count
              description: string # Override auto-description
              optional: bool
              hidden: bool

              # Task-specific properties
              properties:
                key: value      # e.g., item_class: IronOre, location: MineEntrance

              # Navigation marker
              marker:
                enabled: bool
                location: [x, y, z]
                icon: string
                color: string   # Hex color

      # Rewards on completion
      rewards:
        currency: int
        xp: int
        items:
          - item: string        # EI_SteelSword
            quantity: int
        reputation:
          - faction: string     # Narrative.Factions.Town
            amount: int

      # Events triggered
      on_start:
        - type: string
          properties: {}
      on_complete:
        - type: string
          properties: {}
      on_fail:
        - type: string
          properties: {}

  # --------------------------------------------------------------------------
  # ACTIVITIES (AI behaviors - references existing BPA_ or generates new)
  # --------------------------------------------------------------------------
  activities:
    - id: string                # BPA_SmithWork
      base_class: string        # NarrativeActivityBase
      behavior_tree: string     # BT_Smith
      activity_name: string     # "Smithing"

      # Tags
      owned_tags: [string]
      block_tags: [string]
      require_tags: [string]

      # Goal support
      supported_goal: string    # Goal_Work
      interruptable: bool
      save_activity: bool

  # --------------------------------------------------------------------------
  # EVENTS (Narrative events this NPC triggers)
  # --------------------------------------------------------------------------
  events:
    - id: string                # NE_BlacksmithTrusts
      runtime: enum             # Start | End | Both
      filter: enum              # Anyone | OnlyNPCs | OnlyPlayers
      party_policy: enum        # Party | AllPartyMembers | PartyLeader
      refire_on_load: bool

  # --------------------------------------------------------------------------
  # COMBAT (Enemy-specific - AI combat behavior)
  # --------------------------------------------------------------------------
  combat:
    aggro_range: float          # Detection distance
    leash_range: float          # Max chase distance
    attack_range: float         # Melee/ranged attack distance

    # Abilities to use in combat
    abilities:
      - ability: string         # GA_MeleeAttack
        weight: float           # Selection weight
        cooldown: float

    # Loot on death
    loot_table:
      - collection: string      # IC_BanditLoot
        chance: float           # 0.0-1.0

---

## Generated Assets Mapping

| Schema Section | Generated Asset | Prefix | Narrative Pro Class |
|----------------|-----------------|--------|---------------------|
| `npc` | NPC Definition | NPCDef_ | UNPCDefinition |
| `npc.character` | Character Definition | CD_ | UCharacterDefinition |
| `npc.dialogue` | Dialogue Blueprint | DBP_ | UDialogueBlueprint |
| `npc.barks` | Tagged Dialogue Set | TDS_ | UTaggedDialogueSet |
| `npc.quests[]` | Quest Blueprint | Quest_ | UQuest (Blueprint) |
| `npc.goals[]` | Goal Item | Goal_ | UNPCGoalItem (Blueprint) |
| `npc.goal_generators[]` | Goal Generator | GoalGen_ | UNPCGoalGenerator (Blueprint) |
| `npc.schedule` | Activity Schedule | Schedule_ | UNPCActivitySchedule |
| `npc.activities[]` | Activity | BPA_ | UNPCActivity (Blueprint) |
| `npc.events[]` | Narrative Event | NE_ | UNarrativeEvent (Blueprint) |

---

## Example: Complete Vendor NPC

```yaml
npc:
  id: Blacksmith_01
  name: "Garrett the Blacksmith"
  type: vendor
  folder: NPCs/Town

  character:
    level_range: [1, 50]
    attack_priority: 0.2
    default_currency: 100
    owned_tags:
      - State.Invulnerable
    factions:
      - Narrative.Factions.Town
      - Narrative.Factions.FriendlyAll
    ability_config: AC_Villager
    unique: true

  vendor:
    shop_name: "Garrett's Forge"
    buy_percentage: 0.5
    sell_percentage: 1.5
    trading_currency: 500
    inventory:
      - collection: IC_WeaponStock
      - collection: IC_ArmorStock

  schedule:
    - time: [6, 18]
      goal: Goal_Work
      location: Forge
    - time: [18, 21]
      goal: Goal_Relax
      location: Tavern
    - time: [21, 6]
      goal: Goal_Sleep
      location: BlacksmithHome

  dialogue:
    free_movement: true
    can_be_exited: true
    tree:
      root: greeting
      nodes:
        - id: greeting
          type: npc
          speaker: NPCDef_Blacksmith
          text: "Welcome to my forge! Looking for quality steel?"
          player_replies: [shop, quest, bye]

        - id: shop
          type: player
          text: "Show me your wares."
          option_text: "Browse shop"
          events:
            - type: OpenVendorUI
              runtime: End

        - id: quest
          type: player
          text: "Need any help around the forge?"
          option_text: "Ask about work"
          conditions:
            - type: HasNotCompletedQuest
              properties:
                quest: Quest_ForgeSupplies
          npc_replies: [give_quest]

        - id: give_quest
          type: npc
          speaker: NPCDef_Blacksmith
          text: "Actually, I'm running low on iron ore. The mines east of town should have plenty."
          events:
            - type: StartQuest
              runtime: End
              properties:
                quest: Quest_ForgeSupplies

        - id: bye
          type: player
          text: "Farewell."
          option_text: "Leave"

  barks:
    - tag: Greeting.Friendly
      lines:
        - text: "Fine day for smithing!"
        - text: "Quality steel, fair prices!"
    - tag: Idle
      lines:
        - text: "*hammering sounds*"
        - text: "Good steel takes patience..."

  quests:
    - id: Quest_ForgeSupplies
      name: "Forge Supplies"
      description: "Garrett needs iron ore from the eastern mines."

      states:
        - id: start
          type: regular
          description: "Speak to Garrett about work"
        - id: gathering
          type: regular
          description: "Gather iron ore from the mines"
        - id: complete
          type: success
          description: "Quest complete"

      branches:
        - id: accept
          from_state: start
          to_state: gathering
          tasks:
            - type: FindItem
              quantity: 10
              properties:
                item_class: IronOre
              marker:
                enabled: true
                location: [1000, 2000, 0]

        - id: return
          from_state: gathering
          to_state: complete
          tasks:
            - type: TalkToNPC
              properties:
                npc: NPCDef_Blacksmith

      rewards:
        currency: 100
        xp: 50
        items:
          - item: EI_SteelSword
        reputation:
          - faction: Narrative.Factions.Town
            amount: 25
```

---

## Example: Enemy NPC

```yaml
npc:
  id: BanditLeader_01
  name: "Scarface"
  type: enemy
  folder: NPCs/Enemies
  blueprint: BP_BanditLeader

  character:
    level_range: [5, 15]
    attack_priority: 0.8
    default_currency: 50
    factions:
      - Narrative.Factions.Bandits
      - Narrative.Factions.HostileAll
    ability_config: AC_BanditLeader
    unique: true

  combat:
    aggro_range: 1500
    leash_range: 3000
    attack_range: 200
    abilities:
      - ability: GA_MeleeAttack
        weight: 1.0
      - ability: GA_HeavyAttack
        weight: 0.3
        cooldown: 5.0
    loot_table:
      - collection: IC_BanditLoot
        chance: 1.0
      - collection: IC_RareLoot
        chance: 0.1

  goals:
    - id: Goal_GuardCamp
      default_score: 50
      owned_tags:
        - State.Guarding

  barks:
    - tag: Combat.Taunt
      lines:
        - text: "You'll regret coming here!"
        - text: "Get them!"
    - tag: Combat.Death
      lines:
        - text: "Impossible..."
```

---

## Implementation Priority

1. **Quest Generator** - Most complex, enables quest-giver NPCs
2. **Schedule Generator** - Simple DataAsset, enables daily routines
3. **Goal Generator** - Blueprint-based, enables AI objectives
4. **NPC File Parser** - Orchestrates all generators from .npc.yaml

---

## Dependencies

| Generator | Depends On | Blocks |
|-----------|------------|--------|
| Schedule_ | Goal_ (references) | NPCDef_ (ActivitySchedules) |
| Quest_ | DBP_ (QuestDialogue), NE_ (events) | - |
| Goal_ | - | Schedule_, BPA_ |
| NPC Parser | All generators | - |

---

## Verification

1. **Batch Mode**: Run commandlet with `-npc` flag, check all assets created
2. **Editor UI**: Create NPC via form, verify in Content Browser
3. **Prompt Mode**: Generate via chat, verify manifest entries added

