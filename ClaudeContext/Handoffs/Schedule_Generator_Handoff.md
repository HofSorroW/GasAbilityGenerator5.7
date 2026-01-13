# Schedule Generator Implementation Handoff

## Status
READY

## Overview
Generate `Schedule_` DataAssets (UNPCActivitySchedule) that define NPC daily routines. Schedules contain time-based behaviors that add/remove goals at specific times of day (0-2400 scale where 100 = 1 hour).

## Narrative Pro Classes

| Class | Header Location | Purpose |
|-------|-----------------|---------|
| `UNPCActivitySchedule` | `AI/Activities/NPCActivitySchedule.h` | DataAsset containing scheduled behaviors |
| `UScheduledBehavior` | `UnrealFramework/NarrativeGameState.h` | Base scheduled behavior (time binding) |
| `UScheduledBehavior_NPC` | `AI/Activities/NPCActivitySchedule.h` | NPC-specific scheduled behavior |
| `UScheduledBehavior_AddNPCGoal` | `AI/Activities/NPCActivitySchedule.h` | Adds goal at start, removes at end |

## Class Hierarchy

```
UObject
└── UScheduledBehavior (Abstract, Blueprintable)
    └── UScheduledBehavior_NPC
        └── UScheduledBehavior_AddNPCGoal (Abstract, Blueprintable)
            └── Blueprint subclasses that provide goals

UDataAsset
└── UNPCActivitySchedule (Blueprintable)
```

## UNPCActivitySchedule Structure

```cpp
UCLASS(Blueprintable, BlueprintType)
class UNPCActivitySchedule : public UDataAsset
{
    // Instanced array of scheduled behaviors
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "NPC Activity")
    TArray<TObjectPtr<UScheduledBehavior_NPC>> Activities;
};
```

## UScheduledBehavior Properties

| Property | Type | Purpose |
|----------|------|---------|
| `bDisabled` | bool | Disable this behavior |
| `StartTime` | float | Time to start (0-2400) |
| `EndTime` | float | Time to end (0-2400) |

## UScheduledBehavior_AddNPCGoal Properties

| Property | Type | Purpose |
|----------|------|---------|
| `ScoreOverride` | float | Override goal's default score (>0 to use) |
| `bReselect` | bool | Trigger activity reselection after adding goal |

## Time of Day Scale

```
0000 = Midnight
0600 = 6:00 AM
1200 = Noon
1800 = 6:00 PM
2400 = Midnight (wraps to 0)
```

## Key Properties to Expose in YAML

### Schedule Definition

| Property | Type | YAML Key | Notes |
|----------|------|----------|-------|
| `Name` | FString | `name` | Asset name (Schedule_*) |
| `Folder` | FString | `folder` | Content subfolder |
| `Activities` | Array | `activities` | List of scheduled behaviors |

### Scheduled Behavior Entry

| Property | Type | YAML Key | Notes |
|----------|------|----------|-------|
| `type` | FString | `type` | Behavior class (AddNPCGoal, custom) |
| `start_time` | float | `start_time` | 0-2400 |
| `end_time` | float | `end_time` | 0-2400 |
| `disabled` | bool | `disabled` | Skip this entry |
| `goal_class` | FString | `goal_class` | For AddNPCGoal type |
| `score_override` | float | `score_override` | Override goal score |
| `reselect` | bool | `reselect` | Trigger reselection |

## Generator Implementation Notes

### DataAsset Creation

```cpp
FGenerationResult FScheduleGenerator::Generate(const FManifestScheduleDefinition& Definition)
{
    FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);

    UPackage* Package = CreatePackage(*AssetPath);

    // Create the schedule DataAsset
    UNPCActivitySchedule* Schedule = NewObject<UNPCActivitySchedule>(
        Package, *Definition.Name, RF_Public | RF_Standalone);

    if (!Schedule)
    {
        return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create schedule"));
    }
```

### Creating Instanced Behaviors

The challenge is creating instanced UObject subobjects. Use `NewObject` with the schedule as outer:

```cpp
// For each scheduled activity entry
for (const FManifestScheduledBehaviorEntry& Entry : Definition.Activities)
{
    // Determine behavior class
    UClass* BehaviorClass = nullptr;

    if (Entry.Type == TEXT("AddNPCGoal"))
    {
        BehaviorClass = UScheduledBehavior_AddNPCGoal::StaticClass();
        // Note: AddNPCGoal is abstract, need to find concrete subclass or create BP
    }
    else
    {
        // Find custom behavior class
        BehaviorClass = FindClass(Entry.Type);
    }

    if (!BehaviorClass)
    {
        LogGeneration(FString::Printf(TEXT("  Warning: Behavior class '%s' not found"), *Entry.Type));
        continue;
    }

    // Create instanced subobject
    UScheduledBehavior_NPC* Behavior = NewObject<UScheduledBehavior_NPC>(
        Schedule,                           // Outer = the schedule
        BehaviorClass,
        NAME_None,
        RF_Transactional | RF_DefaultSubObject);

    // Set properties
    Behavior->bDisabled = Entry.bDisabled;
    Behavior->StartTime = Entry.StartTime;
    Behavior->EndTime = Entry.EndTime;

    // For AddNPCGoal subclass
    if (UScheduledBehavior_AddNPCGoal* GoalBehavior = Cast<UScheduledBehavior_AddNPCGoal>(Behavior))
    {
        // Set via reflection since properties are protected
        if (Entry.ScoreOverride > 0.f)
        {
            FFloatProperty* ScoreProp = CastField<FFloatProperty>(
                GoalBehavior->GetClass()->FindPropertyByName(TEXT("ScoreOverride")));
            if (ScoreProp)
            {
                ScoreProp->SetPropertyValue_InContainer(GoalBehavior, Entry.ScoreOverride);
            }
        }

        FBoolProperty* ReselectProp = CastField<FBoolProperty>(
            GoalBehavior->GetClass()->FindPropertyByName(TEXT("bReselect")));
        if (ReselectProp)
        {
            ReselectProp->SetPropertyValue_InContainer(GoalBehavior, Entry.bReselect);
        }
    }

    // Add to schedule
    Schedule->Activities.Add(Behavior);
}
```

### Important: Abstract Class Handling

`UScheduledBehavior_AddNPCGoal` is **abstract** - it requires `ProvideGoal()` to be overridden. Options:

1. **Reference existing Blueprint behaviors** - User creates ScheduledBehavior_AddGoal_* Blueprints that override ProvideGoal()
2. **Create Blueprint with event graph** - Generator creates BP subclass with ProvideGoal implementation
3. **Use concrete subclasses from Narrative Pro** - Check if any non-abstract versions exist

**Recommendation:** Option 1 - Reference existing behaviors. The YAML specifies behavior class names, generator instantiates them.

### Required Includes

```cpp
#include "AI/Activities/NPCActivitySchedule.h"
#include "UnrealFramework/NarrativeGameState.h"  // For UScheduledBehavior
```

## YAML Schema Example

```yaml
schedules:
  - name: Schedule_BlacksmithDay
    folder: NPCs/Schedules
    activities:
      # Morning: Open shop
      - type: ScheduledBehavior_OpenShop    # Custom BP class
        start_time: 800                      # 8:00 AM
        end_time: 2000                       # 8:00 PM

      # Work hours: Stay at forge
      - type: ScheduledBehavior_AddNPCGoal_StayAtLocation
        start_time: 800
        end_time: 1200
        score_override: 50.0
        reselect: true

      # Lunch break
      - type: ScheduledBehavior_AddNPCGoal_GoToTavern
        start_time: 1200
        end_time: 1300
        score_override: 60.0

      # Afternoon work
      - type: ScheduledBehavior_AddNPCGoal_StayAtLocation
        start_time: 1300
        end_time: 2000
        score_override: 50.0

      # Night: Go home and sleep
      - type: ScheduledBehavior_AddNPCGoal_GoHome
        start_time: 2000
        end_time: 800                        # Wraps to next day
        score_override: 70.0

  - name: Schedule_GuardPatrol
    folder: NPCs/Schedules
    activities:
      # Day shift patrol
      - type: ScheduledBehavior_AddNPCGoal_Patrol
        start_time: 600
        end_time: 1800
        score_override: 40.0
        reselect: true

      # Night shift: Higher alert
      - type: ScheduledBehavior_AddNPCGoal_Patrol
        start_time: 1800
        end_time: 600
        score_override: 60.0                 # More vigilant at night
```

## Integration with NPCDefinition

NPCDefinitions already reference schedules via `ActivitySchedules` array:

```yaml
npc_definitions:
  - name: NPCDef_Blacksmith
    activity_schedules:
      - Schedule_BlacksmithDay
      - Schedule_BlacksmithNight           # Multiple schedules supported
```

The generator for NPCDef_ already handles `activity_schedules` as `TArray<TSoftObjectPtr<UNPCActivitySchedule>>`.

## Existing Schedule Examples (Narrative Pro Demo)

| Schedule | Location | Purpose |
|----------|----------|---------|
| `Schedule_Luca` | Pro/Demo/Character/Definitions/Luca/ | Demo NPC daily routine |
| `Schedule_Nirvana` | Pro/Demo/Character/Definitions/Nirvana/ | Demo NPC daily routine |

## Dependencies

**Before this works:**
- `ScheduledBehavior_*` Blueprint classes must exist (behavior types)
- `Goal_*` classes if using AddNPCGoal behaviors

**This blocks:**
- NPCDefinitions that reference schedules in `activity_schedules`

## Verification Steps

1. Generated asset appears at `{ProjectRoot}/{Folder}/Schedule_*.uasset`
2. DataAsset opens in editor showing Activities array
3. Each activity entry shows:
   - Correct behavior class type
   - StartTime/EndTime values
   - ScoreOverride (if set)
4. Assigning schedule to NPCDefinition works via `activity_schedules`
5. In-game: NPC behavior changes at scheduled times

## Implementation Complexity

**High** - This generator is complex because:
1. Must create instanced subobjects (not standalone assets)
2. `UScheduledBehavior_AddNPCGoal` is abstract
3. Need concrete behavior Blueprint classes to reference
4. Time wraparound handling (end < start = overnight)

## Alternative Approach: Reference-Only

Instead of creating behaviors programmatically, the generator could:
1. Create empty `UNPCActivitySchedule` DataAsset
2. Log which behavior classes should be manually added
3. Let user populate via editor

**Recommendation:** Start with reference-only approach, then enhance to full generation if needed.

## Automation Level

**Low-Medium** - Due to abstract class and instanced subobject complexity:
- Can create DataAsset container
- Populating Activities array requires concrete behavior classes
- Manual editor work likely needed for initial behavior BP creation
