# Faction System Handoff

## Status
READY - No Generator Needed

## Overview
The Narrative Pro faction system uses **FGameplayTag** identifiers with runtime attitude tracking. Factions are NOT separate assets - they're gameplay tags in the `Narrative.Factions.*` hierarchy. This document explains the system for integration purposes.

## Key Finding: No Faction Generator Required

Factions in Narrative Pro are:
1. **Gameplay Tags** - Defined in DefaultGameplayTags.ini
2. **Runtime Relationships** - Stored in `ANarrativeGameState::FactionAllianceMap`
3. **SaveGame Data** - Persisted via save system

There is no `UFaction` DataAsset class. The existing tag generator and character definition generators already handle faction assignment.

## Narrative Pro Classes

| Class | Header Location | Purpose |
|-------|-----------------|---------|
| `ANarrativeGameState` | `UnrealFramework/NarrativeGameState.h` | Stores faction relationships |
| `FFactionAttitudeData` | `UnrealFramework/NarrativeGameState.h` | Per-faction attitude map |
| `UCharacterDefinition` | `Character/CharacterDefinition.h` | Characters have DefaultFactions |
| `UNPCDefinition` | `AI/NPCDefinition.h` | NPCs inherit DefaultFactions |

## Faction Storage Architecture

### FactionAllianceMap Structure

```cpp
// In ANarrativeGameState.h
UPROPERTY(SaveGame, EditAnywhere, BlueprintReadOnly, Category = "Factions",
    meta = (Categories = "Narrative.Factions"))
TMap<FGameplayTag, FFactionAttitudeData> FactionAllianceMap;
```

### FFactionAttitudeData Structure

```cpp
USTRUCT(BlueprintType)
struct FFactionAttitudeData
{
    // Map of faction -> attitude towards that faction
    UPROPERTY(BlueprintReadOnly, SaveGame, EditAnywhere, Category = "Faction",
        meta = (Categories = "Narrative.Factions"))
    TMap<FGameplayTag, TEnumAsByte<ETeamAttitude::Type>> AttitudeMap;
};
```

### Attitude Types

```cpp
// From GenericTeamAgentInterface.h (Engine)
namespace ETeamAttitude
{
    enum Type
    {
        Friendly,   // Allies
        Neutral,    // No opinion
        Hostile     // Enemies
    };
}
```

## How Factions Work

### 1. Tag Definition
Faction tags are defined in DefaultGameplayTags.ini:

```ini
+GameplayTagList=(Tag="Narrative.Factions.Player",DevComment="Player faction")
+GameplayTagList=(Tag="Narrative.Factions.Friendly",DevComment="Friendly NPCs")
+GameplayTagList=(Tag="Narrative.Factions.Enemy",DevComment="Hostile NPCs")
+GameplayTagList=(Tag="Narrative.Factions.Wildlife",DevComment="Wildlife creatures")
+GameplayTagList=(Tag="Narrative.Factions.Town.Merchants",DevComment="Town merchants")
```

### 2. Character Assignment
Characters have `DefaultFactions` (FGameplayTagContainer):

```cpp
// In UCharacterDefinition
UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Factions",
    meta = (Categories = "Narrative.Factions"))
FGameplayTagContainer DefaultFactions;
```

### 3. Relationship Queries
GameState provides lookup functions:

```cpp
// Check one faction's attitude toward another
ETeamAttitude::Type GetFactionAttitudeTowardsFaction(
    const FGameplayTag& SourceFaction,
    const FGameplayTag& TargetFaction);

// Check multiple factions (hostile > friendly > neutral priority)
ETeamAttitude::Type GetFactionsAttitudeTowardsFactions(
    const FGameplayTagContainer& SourceFactions,
    const FGameplayTagContainer& TargetFactions);

// Set faction relationship
void SetFactionAttitude(
    FGameplayTag SourceFaction,
    FGameplayTag TargetFaction,
    ETeamAttitude::Type NewAttitude);
```

### 4. Change Notification

```cpp
// Delegate for faction changes (AI rebinds perception)
UPROPERTY(BlueprintAssignable, Category = "Factions")
FOnFactionAttitudeChanged OnFactionAttitudeChanged;

// Signature: (Faction, OtherFaction, NewAttitude)
```

## Current GasAbilityGenerator Support

### Tags Generator (Already Implemented)
Faction tags are generated via the `tags` section:

```yaml
tags:
  - tag: Narrative.Factions.Father
    comment: Father companion faction
  - tag: Narrative.Factions.Player
    comment: Player faction
  - tag: Narrative.Factions.Warden
    comment: Warden enemy faction
```

### NPCDefinition Generator (Already Implemented)
NPCs get factions via `default_factions`:

```yaml
npc_definitions:
  - name: NPCDef_Blacksmith
    default_factions:
      - Narrative.Factions.Friendly
      - Narrative.Factions.Town.Merchants
```

### CharacterDefinition Generator (Already Implemented)
Characters get factions via `default_factions`:

```yaml
character_definitions:
  - name: CD_Merchant
    default_factions:
      - Narrative.Factions.Friendly
      - Narrative.Factions.Town
```

## What's NOT Generated

### Faction Relationships
Faction attitudes (Hostile/Friendly/Neutral) are:
1. **Configured in GameState Blueprint** - Initial relationships
2. **Modified at runtime** - Quest completions, player actions
3. **Saved to disk** - Via NarrativeSaveSystem

These are NOT defined in manifest.yaml because:
- Relationships are dynamic
- Initial values typically set in BP_GameState defaults
- Changes occur through gameplay

### Recommendation: Optional Faction Config

If desired, a future enhancement could support:

```yaml
# OPTIONAL - Future enhancement
faction_config:
  initial_relationships:
    - source: Narrative.Factions.Player
      target: Narrative.Factions.Friendly
      attitude: Friendly
    - source: Narrative.Factions.Player
      target: Narrative.Factions.Enemy
      attitude: Hostile
    - source: Narrative.Factions.Enemy
      target: Narrative.Factions.Friendly
      attitude: Hostile
```

This could generate a DataTable or configure GameState defaults.

## How AI Uses Factions

### Perception Check
NPCs use factions to determine hostility:

```cpp
// In AI perception
ETeamAttitude::Type Attitude = GameState->GetFactionsAttitudeTowardsFactions(
    MyCharacter->GetFactions(),
    PerceivedCharacter->GetFactions());

if (Attitude == ETeamAttitude::Hostile)
{
    // Add attack goal, enter combat
}
```

### EQS Filtering
Environment Queries filter by faction:

```cpp
// EnvQueryTest_Team.h
// Filters query results by team/faction alignment
```

### Attack Priority
Characters have `AttackPriority` affecting target selection:

```cpp
// In UCharacterDefinition
UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attack Priority")
float AttackPriority;  // Higher = more likely to be attacked
```

## Integration Points

### With Dialogues
Faction can gate dialogue availability:
- NPC won't talk to hostile factions
- Certain dialogue options locked by reputation

### With Quests
Quests can modify faction relationships:
- Complete quest -> Faction becomes friendly
- Betray faction -> Becomes hostile

### With Combat
AI goal generation uses faction:
- `GoalGenerator_Attack` creates goals for hostile targets
- `GoalGenerator_Flee` triggers when threatened

## Verification Steps

1. **Faction tags exist** in DefaultGameplayTags.ini
2. **NPCs have DefaultFactions** populated in NPCDefinition
3. **GameState has FactionAllianceMap** configured (in BP defaults)
4. **AI perception** correctly identifies hostiles
5. **Faction changes** propagate via delegate

## Summary

| Aspect | Generation Status |
|--------|-------------------|
| Faction Tags | Generated (tags section) |
| Character Factions | Generated (default_factions) |
| NPC Factions | Generated (default_factions) |
| Faction Relationships | NOT generated (runtime/GameState) |
| Attitude Changes | NOT generated (gameplay events) |

## Automation Level

**N/A** - No dedicated generator needed. Existing generators handle:
- `FTagGenerator` - Creates faction tags
- `FNPCDefinitionGenerator` - Sets DefaultFactions
- `FCharacterDefinitionGenerator` - Sets DefaultFactions

Faction relationships are configured in GameState Blueprint or modified at runtime through gameplay systems.
