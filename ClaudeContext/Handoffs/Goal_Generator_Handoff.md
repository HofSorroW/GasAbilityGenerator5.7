# Goal Generator Implementation Handoff

## Status
READY

## Overview
Generate `Goal_` Blueprint assets deriving from `UNPCGoalItem`. Goals are UObjects (not DataAssets) that represent objectives for NPCs to pursue. They are created at runtime by GoalGenerators or added manually.

## Narrative Pro Classes

| Class | Header Location | Purpose |
|-------|-----------------|---------|
| `UNPCGoalItem` | `AI/Activities/NPCGoalItem.h` | Base goal class - abstract, Blueprintable |
| `UNPCGoalGenerator` | `AI/Activities/NPCGoalGenerator.h` | Creates goals dynamically |
| `FNPCGoalContainer` | `AI/Activities/NPCGoalItem.h` | Container struct for goals |
| `FSavedGoalItem` | `AI/Activities/NPCGoalItem.h` | Save/load serialization struct |

## Class Hierarchy

```
UObject
└── UNPCGoalItem (Abstract, Blueprintable, EditInlineNew)
    └── Goal_Attack, Goal_Flee, Goal_FollowCharacter, etc. (Blueprints)
```

## Key Properties to Expose in YAML

| Property | Type | YAML Key | Default | Notes |
|----------|------|----------|---------|-------|
| `OwnedTags` | FGameplayTagContainer | `owned_tags` | [] | Tags granted when activity acts on goal |
| `BlockTags` | FGameplayTagContainer | `block_tags` | [] | Tags that prevent goal from being acted on |
| `RequireTags` | FGameplayTagContainer | `require_tags` | [] | Tags required on owner to act on goal |
| `DefaultScore` | float | `default_score` | 1.0 | Priority score for goal selection |
| `bRemoveOnSucceeded` | bool | `remove_on_succeeded` | true | Auto-remove when activity succeeds |
| `GoalLifetime` | float | `goal_lifetime` | -1.0 | Expiry time (<0 = never expire) |
| `bSaveGoal` | bool | `save_goal` | false | Persist goal to save game |

## Additional BlueprintNativeEvent Functions

Goals often override these in Blueprint:
- `GetDebugString()` - Debug description
- `GetGoalKey()` - Unique identifier object
- `GetGoalScore()` - Dynamic score calculation
- `ShouldCleanup()` - Check if goal is invalid (e.g., target died)
- `Initialize()` - Called when goal is added
- `OnRemoved()` - Called when goal is removed

## Generator Implementation Notes

### Blueprint Creation Pattern

Follow `FActivityGenerator::Generate` pattern (line 8370 in GasAbilityGeneratorGenerators.cpp):

```cpp
// Find parent class
UClass* ParentClass = nullptr;
if (!Definition.ParentClass.IsEmpty())
{
    ParentClass = FindClass(Definition.ParentClass);
}
if (!ParentClass)
{
    ParentClass = UNPCGoalItem::StaticClass();
}

// Create Blueprint
UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
Factory->ParentClass = ParentClass;

UBlueprint* Blueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
    UBlueprint::StaticClass(), Package, *Definition.Name,
    RF_Public | RF_Standalone, nullptr, GWarn));

// Compile
FKismetEditorUtilities::CompileBlueprint(Blueprint);

// Set CDO properties via reflection
UObject* CDO = Blueprint->GeneratedClass->GetDefaultObject();
```

### Property Setting via Reflection

```cpp
// OwnedTags (FGameplayTagContainer)
FStructProperty* TagsProp = CastField<FStructProperty>(CDO->GetClass()->FindPropertyByName(TEXT("OwnedTags")));
if (TagsProp)
{
    FGameplayTagContainer* TagContainer = TagsProp->ContainerPtrToValuePtr<FGameplayTagContainer>(CDO);
    for (const FString& TagString : Definition.OwnedTags)
    {
        FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
        if (Tag.IsValid())
        {
            TagContainer->AddTag(Tag);
        }
    }
}

// DefaultScore (float)
FFloatProperty* ScoreProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("DefaultScore")));
if (ScoreProp)
{
    ScoreProp->SetPropertyValue_InContainer(CDO, Definition.DefaultScore);
}

// bRemoveOnSucceeded (bool)
FBoolProperty* RemoveProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bRemoveOnSucceeded")));
if (RemoveProp)
{
    RemoveProp->SetPropertyValue_InContainer(CDO, Definition.bRemoveOnSucceeded);
}
```

### Required Includes

```cpp
#include "AI/Activities/NPCGoalItem.h"
#include "AI/Activities/NPCGoalGenerator.h"
#include "GameplayTagContainer.h"
```

### Build.cs Dependency

Already present: `NarrativeArsenal` in PrivateDependencyModuleNames

## YAML Schema Example

```yaml
goals:
  - name: Goal_DefendForge
    folder: AI/Goals
    parent_class: NPCGoalItem           # Optional, defaults to UNPCGoalItem

    # Tag-based activation/blocking
    owned_tags:
      - State.Defending
      - AI.Goal.Active
    block_tags:
      - State.Incapacitated
      - State.Fleeing
    require_tags:
      - State.Alive

    # Scoring and lifecycle
    default_score: 80.0                 # High priority goal
    remove_on_succeeded: true           # Remove when defender completes
    goal_lifetime: -1.0                 # Never expires (-1)
    save_goal: false                    # Don't persist to save

  - name: Goal_PatrolArea
    folder: AI/Goals
    owned_tags:
      - State.Patrolling
    default_score: 20.0                 # Low priority
    remove_on_succeeded: false          # Keep patrolling
    goal_lifetime: 3600.0               # 1 hour in-game lifetime
```

## Existing Narrative Pro Goals (Reference)

| Goal Class | Purpose |
|------------|---------|
| `Goal_Attack` | Attack a target |
| `Goal_Flee` | Flee from threat |
| `Goal_FollowCharacter` | Follow another character |
| `Goal_MoveToDestination` | Move to location |
| `Goal_Idle` | Idle behavior |
| `Goal_Interact` | Interact with object |
| `Goal_Patrol` | Patrol route |
| `Goal_DriveToDestination` | Vehicle navigation |

## Dependencies

**Before this works:**
- `FGameplayTag` system must have tags registered
- No asset dependencies (Goals are standalone)

**This blocks:**
- `BPA_` Activities that specify `SupportedGoalType`
- `ActConfig_` that reference goal generators

## Verification Steps

1. Generated asset appears in Content Browser at `{ProjectRoot}/{Folder}/Goal_*.uasset`
2. Blueprint opens without errors in Blueprint Editor
3. CDO shows correct property values in Details panel:
   - OwnedTags populated
   - DefaultScore set
   - bRemoveOnSucceeded correct
4. Activity with matching `SupportedGoalType` can use the goal
5. `GetClass()->GetSuperClass()` returns `UNPCGoalItem`

## GoalGenerator Notes (Optional Generator)

GoalGenerators (`GoalGenerator_*`) are also Blueprints but are more complex:
- Override `InitializeGoalGenerator()` to bind events
- Call `AddGoalItem()` to create goals dynamically
- Examples: `GoalGenerator_Attack` creates `Goal_Attack` when sensing enemies

**Recommendation:** Implement Goal_ generator first. GoalGenerator_ can be a future enhancement since static goals cover most use cases.

## Automation Level

**Medium** - Creates Blueprint with CDO properties. Manual setup needed for:
- Blueprint graph logic (if any custom score calculations)
- Custom BlueprintNativeEvent overrides
