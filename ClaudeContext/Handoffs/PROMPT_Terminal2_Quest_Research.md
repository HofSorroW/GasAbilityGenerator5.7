# Terminal 2 - Research Assignment: Quests & Dialogue Integration

## Context

You are part of a 3-terminal workflow building a comprehensive NPC pipeline for GasAbilityGenerator, an Unreal Engine 5.7 plugin that generates assets from YAML manifests.

**Terminal Roles:**
- **Terminal 1 (Writer + Research)**: Architecture, schema design, implementation
- **Terminal 2 (You)**: Research Quests & Dialogue systems
- **Terminal 3**: Research AI Goals, Schedules, Factions

**Goal:** Enable full NPC creation including dialogues, quests, goals, schedules, events - all from YAML definitions. Three production modes:
1. **Batch** - Process `.npc.yaml` files via commandlet
2. **Editor UI** - Form-based single NPC creation
3. **Prompt** - Generate via Claude Code conversation

## Your Assignment

Research Narrative Pro's Quest and Dialogue systems to produce implementation handoffs for Terminal 1.

### Research Targets

1. **UQuest Class Structure**
   - Header location: `Plugins/NarrativePro22B57/Source/NarrativeArsenal/Public/Tales/Quests/`
   - Quest states, branches, tasks
   - How quests are created programmatically

2. **Quest Tasks/Objectives**
   - `UQuestTask`, `UQuestBranch`, `UQuestState` classes
   - Task types (collect, kill, talk, explore, etc.)
   - Objective tracking

3. **Quest Rewards**
   - Currency rewards
   - Item rewards
   - Reputation/faction rewards
   - How rewards are defined and granted

4. **Dialogue → Quest Integration**
   - How dialogue nodes trigger `StartQuest`
   - Quest state checks in dialogue conditions
   - Quest completion callbacks

5. **Quest → NarrativeEvent Integration**
   - Events fired on quest start/complete/fail
   - How to link quests to narrative events

### Deliverables

Create these files in `ClaudeContext/Handoffs/`:

1. **`Quest_Generator_Handoff.md`** - Full specification for Quest_ generator
2. **`Dialogue_Quest_Integration_Handoff.md`** - How dialogue and quests connect

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
| UQuest | Tales/Quests/Quest.h | Main quest asset |

## Key Properties to Expose in YAML
| Property | Type | Suggested YAML Key | Notes |
|----------|------|-------------------|-------|
| QuestName | FText | name | Display name |

## Generator Implementation Notes
- How to create the asset programmatically
- Required includes
- Property setting via reflection if needed

## YAML Schema Example
```yaml
quests:
  - id: Quest_ForgeSupplies
    name: "Forge Supplies"
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
├── Tales/
│   ├── Quests/
│   │   ├── Quest.h
│   │   ├── QuestTask.h
│   │   ├── QuestBranch.h
│   │   └── QuestState.h
│   └── Dialogue/
│       ├── Dialogue.h
│       ├── DialogueNode.h
│       └── DialogueNode_NPC.h
```

Also check existing generator patterns:
```
Plugins/GasAbilityGenerator/Source/GasAbilityGenerator/
├── Private/GasAbilityGeneratorGenerators.cpp  (see existing generators)
├── Private/GasAbilityGeneratorParser.cpp      (YAML parsing patterns)
└── Public/GasAbilityGeneratorTypes.h          (FManifest* structs)
```

### Current Plugin Context

GasAbilityGenerator v3.8 already has:
- `DBP_` DialogueBlueprint generator with full dialogue tree support (v3.8)
- `NE_` NarrativeEvent generator
- `NPCDef_` NPC Definition with auto-create features
- 25+ asset generators total

The Quest generator will be new (`Quest_` prefix).

### Communication

- Write findings to handoff docs in `ClaudeContext/Handoffs/`
- Terminal 1 will read these and implement
- If you need info from Terminal 3 (goals, factions), note it as a dependency
- Mark status clearly: READY when Terminal 1 can start implementing

## Start Command

Begin by reading the Quest-related headers:
```
Read Plugins/NarrativePro22B57/Source/NarrativeArsenal/Public/Tales/Quests/Quest.h
```

Then explore the directory structure and related classes.
