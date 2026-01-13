# Terminal 3 - Research Assignment: AI Goals, Schedules & Factions

## Context

You are part of a 3-terminal workflow building a comprehensive NPC pipeline for GasAbilityGenerator, an Unreal Engine 5.7 plugin that generates assets from YAML manifests.

**Terminal Roles:**
- **Terminal 1 (Writer + Research)**: Architecture, schema design, implementation
- **Terminal 2**: Research Quests & Dialogue systems
- **Terminal 3 (You)**: Research AI Goals, Schedules, Factions

**Goal:** Enable full NPC creation including dialogues, quests, goals, schedules, events - all from YAML definitions. Three production modes:
1. **Batch** - Process `.npc.yaml` files via commandlet
2. **Editor UI** - Form-based single NPC creation
3. **Prompt** - Generate via Claude Code conversation

## Your Assignment

Research Narrative Pro's AI behavior systems to produce implementation handoffs for Terminal 1.

### Research Targets

1. **Goals System**
   - `UNPCGoalItem` - Individual goal definitions
   - `UNPCGoalGenerator` - Dynamic goal creation
   - Goal priorities, triggers, conditions
   - Header location: `Plugins/NarrativePro22B57/Source/NarrativeArsenal/Public/AI/`

2. **Schedule System**
   - `UNPCActivitySchedule` - Daily routines
   - Time blocks, locations, activities
   - How schedules trigger activity switches
   - Schedule → Activity → BehaviorTree flow

3. **Activity System Integration**
   - How `BPA_` activities connect to goals
   - `ActConfig_` activity configurations
   - Activity scoring and selection

4. **Faction System**
   - `UFaction` or faction data structures
   - Reputation tracking
   - Faction-based AI behavior (hostile, friendly, neutral)
   - How faction affects dialogue/quest availability

5. **BehaviorTree Linkage**
   - How goals specify behavior trees
   - Blackboard requirements for goals
   - Goal → BT → Activity flow

### Deliverables

Create these files in `ClaudeContext/Handoffs/`:

1. **`Goal_Generator_Handoff.md`** - Specification for Goal_ generator
2. **`Schedule_Generator_Handoff.md`** - Specification for Schedule_ generator
3. **`Faction_System_Handoff.md`** - How factions work, what we can/should generate

### Handoff Document Format

```markdown
# [Feature] Implementation Handoff

## Status
READY | IN_PROGRESS | NEEDS_INFO | BLOCKED

## Overview
[1-2 sentence summary]

## Narrative Pro Classes
| Class | Header Location | Purpose |
|-------|-----------------|---------|
| UNPCGoalItem | AI/Goals/NPCGoalItem.h | Goal definition |

## Key Properties to Expose in YAML
| Property | Type | Suggested YAML Key | Notes |
|----------|------|-------------------|-------|
| GoalPriority | float | priority | 0-100 scale |

## Generator Implementation Notes
- How to create the asset programmatically
- Required includes
- Property setting via reflection if needed

## YAML Schema Example
```yaml
goals:
  - id: Goal_DefendForge
    priority: 80
    # ... full example
```

## Dependencies
- What must exist before this works
- What this blocks

## Verification Steps
1. How to test the generated asset
2. What to check in editor
```

### Key Files to Read

Start with these Narrative Pro headers:
```
Plugins/NarrativePro22B57/Source/NarrativeArsenal/Public/
├── AI/
│   ├── Goals/
│   │   ├── NPCGoalItem.h
│   │   └── NPCGoalGenerator.h
│   ├── Activities/
│   │   ├── NPCActivity.h
│   │   └── NPCActivitySchedule.h
│   └── NarrativeNPCController.h
├── Characters/
│   └── NarrativeNPCCharacter.h
└── Factions/
    └── [faction related files]
```

Also check existing generator patterns:
```
Plugins/GasAbilityGenerator/Source/GasAbilityGenerator/
├── Private/GasAbilityGeneratorGenerators.cpp  (see existing generators)
│   - FActivityGenerator (BPA_) - relevant pattern
│   - FActivityConfigurationGenerator (ActConfig_)
│   - FBehaviorTreeGenerator (BT_)
├── Private/GasAbilityGeneratorParser.cpp      (YAML parsing patterns)
└── Public/GasAbilityGeneratorTypes.h          (FManifest* structs)
```

### Current Plugin Context

GasAbilityGenerator v3.8 already has:
- `BPA_` Activity generator with tags, goal type support
- `ActConfig_` Activity Configuration with goal generators array
- `BT_` Behavior Tree generator with node trees
- `BB_` Blackboard generator
- `NPCDef_` with activity_schedules array (references, doesn't create)

New generators needed:
- `Goal_` - UNPCGoalItem assets
- `GoalGen_` - UNPCGoalGenerator assets (maybe)
- `Schedule_` - UNPCActivitySchedule assets

### Communication

- Write findings to handoff docs in `ClaudeContext/Handoffs/`
- Terminal 1 will read these and implement
- If you need info from Terminal 2 (quests, dialogue), note it as a dependency
- Mark status clearly: READY when Terminal 1 can start implementing

### Key Questions to Answer

1. Can Goals be created as DataAssets or are they Blueprints?
2. What's the minimum viable Schedule definition?
3. How does the faction system store reputation - per-NPC or global?
4. Are GoalGenerators needed or can we just use static Goals?

## Start Command

Begin by exploring the AI directory structure:
```
List files in Plugins/NarrativePro22B57/Source/NarrativeArsenal/Public/AI/
```

Then read the goal and schedule headers.
