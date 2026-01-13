# Terminal 3 Research Summary: AI Goals, Schedules & Factions

## Research Complete: 2026-01-13

## Key Findings

### 1. Goals (Goal_) - Blueprint Generator Needed
- **Class:** `UNPCGoalItem` - Abstract UObject (NOT DataAsset)
- **Pattern:** Blueprintable, EditInlineNew
- **Creation:** Runtime objects, created by GoalGenerators or manually
- **Generator Status:** READY - Follow FActivityGenerator pattern
- **Handoff:** `Goal_Generator_Handoff.md`

### 2. Schedules (Schedule_) - DataAsset Generator Needed
- **Class:** `UNPCActivitySchedule` - UDataAsset
- **Contains:** `TArray<UScheduledBehavior_NPC*> Activities` (instanced)
- **Complexity:** High - instanced subobjects, abstract behavior class
- **Generator Status:** READY with caveats
- **Handoff:** `Schedule_Generator_Handoff.md`

### 3. Factions - NO Generator Needed
- **System:** FGameplayTag-based, NOT DataAssets
- **Storage:** `ANarrativeGameState::FactionAllianceMap`
- **Current Support:** Already handled by tag and NPC generators
- **Generator Status:** N/A - existing generators sufficient
- **Handoff:** `Faction_System_Handoff.md`

## Answers to Key Questions

| Question | Answer |
|----------|--------|
| Goals: DataAsset or Blueprint? | **Blueprint** - UObject subclass, not DataAsset |
| Minimum viable Schedule? | DataAsset with Activities array containing UScheduledBehavior_NPC entries (StartTime, EndTime, behavior class) |
| Faction storage: per-NPC or global? | **Global** - ANarrativeGameState stores faction relationships |
| GoalGenerators needed? | **Optional** - Static goals can be added via AddGoal() |

## Recommended Implementation Order

1. **Goal_ Generator** (Medium complexity)
   - Straightforward Blueprint creation
   - Follow FActivityGenerator pattern exactly
   - Set CDO properties via reflection

2. **Schedule_ Generator** (High complexity)
   - DataAsset creation is simple
   - Populating Activities array requires concrete behavior classes
   - Consider reference-only approach first

3. **GoalGenerator_** (Future, optional)
   - More complex Blueprint graph needed
   - InitializeGoalGenerator() requires event bindings
   - Defer unless explicitly needed

## Discovered Classes (Header Locations)

```
NarrativeArsenal/Public/AI/Activities/
├── NPCGoalItem.h          - UNPCGoalItem, FNPCGoalContainer
├── NPCGoalGenerator.h     - UNPCGoalGenerator
├── NPCActivitySchedule.h  - UNPCActivitySchedule, UScheduledBehavior_NPC
├── NPCActivity.h          - UNPCActivity
├── NPCActivityComponent.h - UNPCActivityComponent (manages goals)
├── NPCActivityConfiguration.h - UNPCActivityConfiguration
└── NarrativeActivityBase.h - Base activity class

NarrativeArsenal/Public/UnrealFramework/
├── NarrativeGameState.h   - Faction storage, UScheduledBehavior base
└── NarrativeTeamAgentInterface.h - Team/faction interface
```

## Existing Narrative Pro Assets (Reference)

### Goals
- Goal_Attack, Goal_Flee, Goal_FollowCharacter
- Goal_MoveToDestination, Goal_Idle, Goal_Interact, Goal_Patrol

### GoalGenerators
- GoalGenerator_Attack, GoalGenerator_Flee, GoalGenerator_Interact

### Schedules
- Schedule_Luca, Schedule_Nirvana (Demo examples)

## Dependencies Between Systems

```
Tags (FTagGenerator)
    │
    ├── Factions (Narrative.Factions.*)
    │       │
    │       └── NPCDefinition.DefaultFactions
    │           CharacterDefinition.DefaultFactions
    │
    └── Goal Tags (optional)

Goal_ Blueprints (NEW)
    │
    ├── BPA_ Activities (SupportedGoalType)
    │
    └── Schedule_ (UScheduledBehavior_AddNPCGoal)

Schedule_ DataAssets (NEW)
    │
    └── NPCDefinition.ActivitySchedules
```

## Terminal 1 Action Items

1. **Implement FGoalGenerator** (Priority: High)
   - Create Blueprint deriving from UNPCGoalItem
   - Set OwnedTags, BlockTags, RequireTags, DefaultScore, etc.
   - See detailed implementation in `Goal_Generator_Handoff.md`

2. **Implement FScheduleGenerator** (Priority: Medium)
   - Create UNPCActivitySchedule DataAsset
   - Challenge: Instanced subobjects need concrete behavior classes
   - May need behavior BP classes created first
   - See detailed implementation in `Schedule_Generator_Handoff.md`

3. **Update Manifest Types** (Required)
   - Add `FManifestGoalDefinition` struct
   - Add `FManifestScheduleDefinition` struct
   - Add `TArray<FManifestGoalDefinition> Goals` to FManifestData
   - Add `TArray<FManifestScheduleDefinition> Schedules` to FManifestData

## No Changes Needed

- Faction system - existing tag/NPC generators handle it
- GoalGenerators - defer until explicit need
- ActivityConfiguration - already implemented

## Research Notes

The Narrative Pro AI system is well-designed with clear separation:
- **Goals** define WHAT to do
- **Activities** define HOW to do it (behavior tree)
- **Schedules** define WHEN to do it
- **Factions** define WHO is friend/foe

Goals are the most immediately useful addition - they complete the "what" piece that activities need.
