# FindCharacter Composite Node Investigation Handoff (v1.2)

**Date**: 2026-01-30
**Version**: v7.8.45
**Status**: ALL FIXES IMPLEMENTED AND VERIFIED - 202/202 assets generating successfully
**Priority**: P0/P1 - RESOLVED

---

## Executive Summary

Attempted to create a `FindCharacterFromSubsystem` composite node type (similar to existing `SpawnNPC` pattern) to look up NPCs by `CharacterDefinition` via `NarrativeCharacterSubsystem::FindCharacter()`. The composite node approach failed because **internal connections made via `MakeLinkTo()` do not trigger pin type propagation callbacks needed for DynamicCast nodes**.

Additionally, BPA_Alert had **manifest property name errors** and **type mismatches** that required multiple fixes.

**ALL ISSUES NOW RESOLVED** - Generation runs with 0 failures.

---

## AUDIT SESSION FINDINGS (2026-01-30)

### Claude-GPT Cross-Validation Results

GPT challenged the original hypothesis that "Blueprint variables can't be found via FindPropertyByName." This challenge was **VALIDATED** - the real issues were:

1. **FindCharacterFromSubsystem**: `MakeLinkTo()` doesn't trigger `NotifyPinConnectionListChanged()` needed for DynamicCast wildcard pin type resolution
2. **BPA_Alert PropertySet failures**: **Wrong property names in manifest** - the properties simply don't exist with those names
3. **BPA_Alert Type mismatches**: Several connections had incompatible types requiring DynamicCast nodes
4. **BPA_Alert ConstructObjectFromClass**: NPCGoalItem is abstract - cannot be constructed directly

---

## P0: FindCharacter Composite Node - FIXED (v7.8.45)

### Problem Statement
BPA_FormationFollow needs to find a character by `CharacterDefinition` when the direct `TargetToFollow` reference is invalid. The pattern requires:

```
GetWorldSubsystem<NarrativeCharacterSubsystem>() → Cast → FindCharacter(CharacterDefinition) → Result
```

### Root Cause (VALIDATED)
Connections made via `UEdGraphPin::MakeLinkTo()` do NOT trigger `NotifyPinConnectionListChanged()`, which is required for:
- DynamicCast nodes to resolve their wildcard output pin types
- Pin type propagation during graph construction

### Fix Applied (v7.8.45)
Changed `MakeLinkTo()` to `Schema->TryCreateConnection()` in `CreateFindCharacterFromSubsystemNode()`:

```cpp
// v7.8.45: AUDIT FIX - TryCreateConnection triggers NotifyPinConnectionListChanged
const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
Schema->TryCreateConnection(SubsystemReturnPin, CastObjectPin);
Schema->TryCreateConnection(CastResultPin, FindCharSelfPin);
```

### Result
BPA_FormationFollow now compiles successfully.

---

## P1: BPA_Alert PropertySet Failures - FIXED (v7.8.43-v7.8.44)

### Root Cause (VALIDATED via Screenshots)
The manifest used **incorrect property names** that don't exist on the target Blueprint classes.

### Fixes Applied

#### Property Name Corrections (v7.8.43)

| Node ID | Old (WRONG) | New (CORRECT) |
|---------|-------------|---------------|
| SetMoveTargetLocation | `variable_name: TargetLocation` | `variable_name: Destination` |
| SetFleeTarget | `variable_name: FleeTarget` | `variable_name: FleeFromTarget` |
| SetMoveGoalScore | `variable_name: GoalScore` | `variable_name: DefaultScore` |
| SetFleeGoalScore | `variable_name: GoalScore` | `variable_name: DefaultScore` |

#### Type Mismatch Fixes (v7.8.43)

| Issue | Fix |
|-------|-----|
| Vector → Transform for Destination | Added `MakeDestinationTransform` (MakeTransform node) |
| Pawn → Character for PlayAnimMontage | Added `CastPawnToCharacter` DynamicCast |
| ActorComponent → NPCActivityComponent | Added `CastToReinfActivityComp` and `CastToGathererActivityComp` |
| NPCGoalItem → Goal_Alert | Added `CastCachedGoalForMove` and `CastCachedGoalForFlee` |

#### ConstructObjectFromClass Fix (v7.8.44)

| Node | Old (WRONG) | New (CORRECT) |
|------|-------------|---------------|
| ConstructMoveGoal | `class: NPCGoalItem` | `class: /NarrativePro/Pro/Core/AI/Activities/GoToLocation/Goal_MoveToDestination.Goal_MoveToDestination_C` |
| ConstructFleeGoal | `class: NPCGoalItem` | `class: /NarrativePro/Pro/Core/AI/Activities/Attacks/Goals/Goal_Flee.Goal_Flee_C` |

**Reason**: NPCGoalItem is an abstract class and cannot be constructed directly.

### Result
BPA_Alert now compiles successfully with all 84 connections.

---

## P2: AutoLayoutNodes Entry Point Detection - DEFERRED

### Issue
`AutoLayoutNodes` only checks for `Event` and `CustomEvent` as entry points, not `FunctionEntry`.

### Impact
Diagnostics only - false positive orphan warnings for function overrides. Does not affect functionality.

### Status
Deferred as non-blocking polish issue.

---

## Implementation Checklist - COMPLETE

### P0 - FindCharacterFromSubsystem (Generator Code)
- [x] Change `MakeLinkTo()` to `Schema->TryCreateConnection()` in `CreateFindCharacterFromSubsystemNode()`
- [x] Verify connections survive compilation
- [x] Test BPA_FormationFollow generates with full connections

### P1 - BPA_Alert PropertySet Nodes (Manifest)
- [x] Change `SetMoveTargetLocation.variable_name` from `TargetLocation` to `Destination`
- [x] Add MakeTransform node for Vector→Transform conversion
- [x] Change `SetFleeTarget.variable_name` from `FleeTarget` to `FleeFromTarget`
- [x] Change `SetMoveGoalScore.variable_name` from `GoalScore` to `DefaultScore`
- [x] Change `SetFleeGoalScore.variable_name` from `GoalScore` to `DefaultScore`
- [x] Add DynamicCast nodes for type mismatches (5 nodes added)
- [x] Fix ConstructObjectFromClass to use concrete Blueprint class paths

### P2 - AutoLayoutNodes (Generator Code)
- [ ] Add FunctionEntry to entry point detection in AutoLayoutNodes (deferred - non-blocking)

---

## Final Test Results

```
RESULT: New=202 Skipped=0 Failed=0 Deferred=0 Cascaded=0 CascadeRoots=0 Total=202 Headless=true
```

All 202 assets generated successfully with 0 failures.

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| v1.0 | 2026-01-29 | Initial handoff - documented investigation and failure modes |
| v1.1 | 2026-01-30 | **AUDIT COMPLETE** - GPT cross-validation confirmed root causes. FindCharacterFromSubsystem needs TryCreateConnection (not MakeLinkTo). BPA_Alert failures are manifest property name errors: TargetLocation→Destination, FleeTarget→FleeFromTarget, GoalScore→DefaultScore. Added screenshot evidence from Narrative Pro Blueprint assets. |
| v1.2 | 2026-01-30 | **ALL FIXES IMPLEMENTED** - P0 generator fix (v7.8.45), P1 manifest fixes (v7.8.43-v7.8.44). Added type mismatch fixes and ConstructObjectFromClass concrete class paths. 202/202 assets generating successfully. |

---

## References

### Blueprint Asset Paths (Narrative Pro)
- Goal_MoveToDestination: `/NarrativePro/Pro/Core/AI/Activities/GoToLocation/Goal_MoveToDestination`
- Goal_Flee: `/NarrativePro/Pro/Core/AI/Activities/Attacks/Goals/Goal_Flee`

### Engine Headers
- `NarrativeCharacterSubsystem.h:154-155` - FindCharacter is BlueprintCallable
- `NPCGoalItem.h:54-55` - DefaultScore is BlueprintReadWrite

### Generator Code
- `CreateFindCharacterFromSubsystemNode`: `GasAbilityGeneratorGenerators.cpp:14521-14665`
- `CreateVariableSetNode`: `GasAbilityGeneratorGenerators.cpp:12811-12955`
- `FindClassByName` (Asset Registry search): `GasAbilityGeneratorFunctionResolver.cpp:629-754`

### Manifest Sections
- BPA_Alert: `manifest.yaml` lines ~13237-13948
