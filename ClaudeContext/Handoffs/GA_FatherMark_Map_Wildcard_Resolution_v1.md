# GA_FatherMark Implementation Handoff - Map Wildcard Resolution Issue

**Date:** 2026-01-31
**Version:** v7.8.56
**Status:** GA_FatherMark COMPLETE, BP_FatherCompanion BLOCKED by Map wildcard bug

---

## Executive Summary

GA_FatherMark (the core marking ability) is **fully generated and working** (61/61 nodes, 74/74 connections). However, BP_FatherCompanion fails to generate due to a **Map wildcard type resolution bug** affecting the `DestroyMarkWidgetForEnemy` helper function.

---

## What Works

| Component | Status | Details |
|-----------|--------|---------|
| GA_FatherMark | ✅ COMPLETE | 61 nodes, 74 connections, all sections functional |
| AddToMarkedEnemies | ✅ COMPLETE | Array operations work correctly |
| RemoveFromMarkedEnemies | ✅ COMPLETE | Array_RemoveItem works correctly |
| MarkEnemy | ⏸️ BLOCKED | Depends on BP_FatherCompanion completing |
| DestroyMarkWidgetForEnemy | ❌ BLOCKED | Map wildcard resolution bug |

---

## The Problem

### Symptom
```
ERROR: Can't connect pins - Actor Object Reference is not compatible with Widget Component Object Reference (by ref).
FAILED to connect: FunctionEntry.Enemy -> FindWidget.Key
```

### Root Cause (From Engine Research)

When UE5 creates an `FEdGraphPinType` for a `Map<Actor, WidgetComponent>`:

| Pin Type Field | Content | Expected Use |
|----------------|---------|--------------|
| `PinCategory` | object | KEY type category |
| `PinSubCategoryObject` | Actor class | KEY type class |
| `ContainerType` | Map | Container indicator |
| `PinValueType.TerminalCategory` | object | VALUE type category |
| `PinValueType.TerminalSubCategoryObject` | WidgetComponent | VALUE type class |

**The Bug:** Wildcard pins (like Map_Find's Key input) resolve using `PinCategory`/`PinSubCategoryObject` (the KEY type), but the engine stores the KEY in the primary fields and VALUE in `PinValueType`. This is backwards from what Map_Find expects.

When Map_Find connects to a Map variable:
- TargetMap wildcard → connects successfully
- Key wildcard → resolves to WidgetComponent (VALUE type from primary fields) instead of Actor (KEY type)
- Value wildcard → resolves correctly

### Engine Code Location
- `EdGraphSchema_K2.cpp::ConvertPropertyToPinType()` lines 3623-3722
- `EdGraphSchema_K2.cpp::ArePinTypesCompatible()` lines 4488-4687
- Wildcard resolution at lines 4659-4671 doesn't check `ContainerType == Map`

---

## Generator Enhancements Made (v7.8.56)

### 1. Map Variable Support Added
**Files Modified:**
- `GasAbilityGeneratorTypes.h` - Added `KeyType`, `ValueType` fields to `FManifestActorVariableDefinition`
- `GasAbilityGeneratorParser.cpp` - Parse `key_type:` and `value_type:` fields (4 locations)
- `GasAbilityGeneratorGenerators.cpp` - Set `EPinContainerType::Map` and configure `PinValueType` (6 locations)

### 2. Manifest Format for Maps
```yaml
- name: MarkWidgetMap
  type: Object
  class: WidgetComponent
  container: Map
  key_type: Actor
  value_type: WidgetComponent
```

### 3. DestroyComponent Function Resolution
**File:** `GasAbilityGeneratorFunctionResolver.cpp`
- Added `DestroyComponent` to WellKnownFunctions
- Added `UActorComponent::StaticClass()` to FallbackClasses

---

## Manifest Status

**File:** `ClaudeContext/manifest.yaml`

### BP_FatherCompanion Variables
```yaml
variables:
  - name: MarkedEnemies
    type: Object
    class: Actor
    container: Array          # ✅ Works

  - name: MarkWidgetMap
    type: Object
    class: WidgetComponent
    container: Map            # ❌ Wildcard bug
    key_type: Actor
    value_type: WidgetComponent
```

### Helper Functions Order (Circular Dependency Fix)
```yaml
custom_functions:
  # These MUST be first (GA_FatherMark depends on them)
  - function_name: AddToMarkedEnemies      # ✅ Works
  - function_name: RemoveFromMarkedEnemies # ✅ Works
  - function_name: DestroyMarkWidgetForEnemy # ❌ Blocked

  # This MUST be last (depends on GA_FatherMark)
  - function_name: MarkEnemy               # ⏸️ Not reached
```

---

## Potential Solutions

### Option 1: Fix Wildcard Resolution in Generator (Recommended)
When connecting to Map_Find/Map_Remove, explicitly set the Key pin type from the Map's key type info instead of relying on wildcard resolution.

**Location:** `FEventGraphGenerator::CreateConnection()` or a new `ConfigureMapPinTypes()` helper

**Approach:**
1. Detect when target node is Map_Find/Map_Remove
2. Find the connected TargetMap pin
3. Extract key type from the Map variable's `PinValueType` (confusingly, this is where UE stores the key)
4. Explicitly set Key pin type before connection

### Option 2: Create Explicit Map Operation Nodes
Instead of using CallFunction with Map_Find/Map_Remove, create specialized node types:
- `MapFind` - with explicit key/value type properties
- `MapRemove` - with explicit key type property

### Option 3: Avoid Maps in Custom Functions
Redesign DestroyMarkWidgetForEnemy to use a different approach:
- Store widgets in a parallel array (same index as MarkedEnemies)
- Use array index lookup instead of Map key lookup

---

## Files to Review

| File | Purpose |
|------|---------|
| `manifest.yaml` lines 9946-9951 | MarkWidgetMap variable definition |
| `manifest.yaml` lines 10016-10077 | DestroyMarkWidgetForEnemy function |
| `GasAbilityGeneratorGenerators.cpp` | Map variable creation (search: `EPinContainerType::Map`) |
| `GasAbilityGeneratorFunctionResolver.cpp` | DestroyComponent resolution |

---

## Test Commands

```bash
# Delete assets and regenerate
rm -rf "/c/Unreal Projects/NP22B57/Content/FatherCompanion/Actors/BP_FatherCompanion.uasset"
rm -rf "/c/Unreal Projects/NP22B57/Content/FatherCompanion/Abilities/Actions/GA_FatherMark.uasset"

# Build and generate
powershell -ExecutionPolicy Bypass -Command "& 'C:\Unreal Projects\NP22B57\Plugins\GasAbilityGenerator\Tools\claude_automation.ps1' -Action cycle"

# Check specific errors
powershell -ExecutionPolicy Bypass -Command "Get-Content 'C:\Unreal Projects\NP22B57\Plugins\GasAbilityGenerator\Tools\Logs\commandlet_output.log' | Select-String -Pattern 'MarkWidgetMap|Map<|Key.*Widget|FAIL'"
```

---

## Contract Compliance

| Contract | Status | Notes |
|----------|--------|-------|
| Contract 24 (SetByCaller) | ✅ Compliant | No SetByCaller for damage with NarrativeDamageExecCalc |
| Contract 25 (No Simplify) | ✅ Compliant | Enhancing generator, not simplifying ability |
| LOCKED RULE 7 | ✅ Following | Generator serves the design |

---

## Next Session Priority

1. **Fix Map wildcard resolution** - Either in CreateConnection or with explicit node types
2. **Verify BP_FatherCompanion generates** - All 4 custom functions should succeed
3. **Test GA_FatherMark in editor** - Verify mark mechanics work at runtime
4. **Commit changes** - v7.8.56 with Map support (partial) and GA_FatherMark

---

## Session Context

- Started from context recovery after session compaction
- GA_FatherMark was the focus per Joint Audit v2.0 authorization
- Circular dependency between GA_FatherMark and BP_FatherCompanion was resolved by reordering custom functions
- Multiple manifest fixes: Array format, pin names (SingleTag, Output), function input patterns
