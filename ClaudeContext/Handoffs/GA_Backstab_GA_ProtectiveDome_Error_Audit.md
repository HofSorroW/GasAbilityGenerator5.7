# GA_Backstab & GA_ProtectiveDome Error Audit

**Date:** 2026-01-24 (Original) | 2026-01-25 (v4.31-v4.32.2 Implementation)
**Status:** RESOLVED - GA_Backstab and GA_ProtectiveDome both SKIP (already exist)
**Context:** Debug analysis of event graph generation failures + BTS/GoalGenerator fixes

---

## Executive Summary

**ORIGINAL CLAIM:** Two abilities (GA_ProtectiveDome, GA_Backstab) fail event graph generation due to manifest syntax issues.

**RESOLUTION (v4.31-v4.32.2):**
- GA_Backstab: Fixed function name `BP_ApplyGameplayEffectSpecToOwner` → `K2_ApplyGameplayEffectSpecToOwner`
- GA_ProtectiveDome: Already working (SKIP)
- BTS_CalculateFormationPosition: Fixed MakeLiteralName for blackboard KeyName params
- BTS_AdjustFormationSpeed: Fixed Object→Actor cast + MakeLiteralName
- Parser: Added `class:` field handling for TSubclassOf in actor_blueprints

| Issue | Original Claim | Resolution | Status |
|-------|----------------|------------|--------|
| 1. `parameters:` block | NOT supported | ✅ Parser already converts | **LOCKED** |
| 2. Pin `Spec`/`Tag` | Not found | ✅ Works with correct function names | **RESOLVED** |
| 3. Custom function call | Resolution fails | ✅ v4.31 fixes applied | **RESOLVED** |
| 4. TSubclassOf variables | Type mismatch | ✅ v4.32.2 parser fix | **RESOLVED** |
| 5. Blackboard KeyName | By-ref param error | ✅ v4.32.2 MakeLiteralName | **RESOLVED** |
| 6. Object→Actor cast | Type mismatch | ✅ v4.32.2 manifest fix | **RESOLVED** |

---

## v4.32.2 Code Changes (2026-01-25)

### Change 1: Parser - TSubclassOf Variable Handling

**File:** `GasAbilityGeneratorParser.cpp` (lines 2272-2285)

Added `class:` field handling to ParseActorBlueprints:
```cpp
else if (TrimmedLine.StartsWith(TEXT("class:")))
{
    // v4.32.2: For Object/Class/TSubclassOf types, combine with Type field
    // e.g., type: Class + class: NarrativeNPCCharacter -> Class:NarrativeNPCCharacter
    FString ClassValue = GetLineValue(TrimmedLine);
    if (!CurrentVar.Type.IsEmpty())
    {
        CurrentVar.Type = CurrentVar.Type + TEXT(":") + ClassValue;
    }
}
```

### Change 2: Function Remapping - UE5 Double Precision

**File:** `GasAbilityGeneratorGenerators.cpp` (lines 10399-10410)

Added arithmetic operation remapping:
```cpp
DeprecatedFunctionRemapping.Add(TEXT("Add_FloatFloat"), TEXT("Add_DoubleDouble"));
DeprecatedFunctionRemapping.Add(TEXT("Subtract_FloatFloat"), TEXT("Subtract_DoubleDouble"));
DeprecatedFunctionRemapping.Add(TEXT("Multiply_FloatFloat"), TEXT("Multiply_DoubleDouble"));
DeprecatedFunctionRemapping.Add(TEXT("Divide_FloatFloat"), TEXT("Divide_DoubleDouble"));
```

### Change 3: FunctionResolver - Actor Gameplay Tag Checking

**File:** `GasAbilityGeneratorFunctionResolver.cpp` (line 114)

Added WellKnown entry:
```cpp
WellKnownFunctions.Add(TEXT("ActorHasMatchingGameplayTag"), UBlueprintGameplayTagLibrary::StaticClass());
```

---

## Manifest Fixes (v4.32.2)

### BTS_CalculateFormationPosition
- Added `MakeKeyName_TargetCharacter` and `MakeKeyName_TargetLocation` nodes
- Connected MakeLiteralName outputs to blackboard KeyName pins
- Resolves "by ref params expect a valid input" error

### BTS_AdjustFormationSpeed
- Added `CastToActor` node between GetLeader and GetLeaderVelocity
- Added MakeLiteralName for TargetCharacter key
- Resolves "Object Reference is not compatible with Actor" error

### BTS_CheckExplosionProximity
- Added MakeLiteralName for AttackTarget key
- Updated GE_ExplosionDamage path to full path

### BTS_HealNearbyAllies
- Removed dangling connections to non-existent nodes (copy/paste error)

---

## Current Status (2026-01-25 ~00:25)

### Passing Assets
- ✅ GA_Backstab (SKIP - exists)
- ✅ GA_ProtectiveDome (SKIP - exists)
- ✅ BTS_CalculateFormationPosition (NEW)
- ✅ BTS_AdjustFormationSpeed (NEW)
- ✅ BP_WardenHusk (SKIP)
- ✅ BP_BiomechHost (SKIP)

### Remaining Failures (12)
| Asset | Error | Root Cause |
|-------|-------|------------|
| BTS_CheckExplosionProximity | ApplyExplosionDamage not found | GE_ExplosionDamage path resolution |
| BTS_HealNearbyAllies | HasFriendlyTag/ApplyHeal not found | ActorHasMatchingGameplayTag + GE_SupportHeal |
| GoalGenerator_Alert | Event graph failed | Pin name issues (Value vs variable name) |
| GoalGenerator_RandomAggression | Event graph failed | Similar pin naming issues |
| AC_* (8 configs) | Cascade failures | Depend on failing BTS/GoalGenerators |

---

## Known Issues - Pin Naming

The GoalGenerator failures show pin naming issues:
```
[E_PIN_POSITION_NOT_FOUND] Pin 'Value' not found on node 'Get bAlertBroadcast'
```

**Root Cause:** VariableGet nodes output the variable NAME as pin name, not "Value".
- Wrong: `[GetBroadcast, Value]`
- Correct: `[GetBroadcast, bAlertBroadcast]`

This pattern appears throughout GoalGenerator_Alert and GoalGenerator_RandomAggression manifests.

---

## Locked Decisions (Updated 2026-01-25)

| Decision ID | Topic | Status | Evidence |
|-------------|-------|--------|----------|
| D1 | `parameters:` block | ✅ **LOCKED** | Source code proves parser converts |
| D2 | Function names (K2_*) | ✅ **LOCKED** | K2_ApplyGameplayEffectSpecToOwner works |
| D3 | TSubclassOf parsing | ✅ **LOCKED** | v4.32.2 parser fix deployed |
| D4 | MakeLiteralName for by-ref | ✅ **LOCKED** | Pattern works for blackboard keys |
| D5 | Object→Actor cast | ✅ **LOCKED** | Required for GetValueAsObject → Actor functions |
| D6 | Double precision remapping | ✅ **LOCKED** | Add_DoubleDouble etc. |
| D7 | Pin naming (VariableGet) | ⚠️ **MANIFEST FIX** | Use variable name, not "Value" |

---

## Reference Files

| File | Purpose |
|------|---------|
| `GasAbilityGeneratorParser.cpp:2272-2285` | v4.32.2 TSubclassOf class: handling |
| `GasAbilityGeneratorGenerators.cpp:10399-10410` | v4.32.2 arithmetic remapping |
| `GasAbilityGeneratorFunctionResolver.cpp:114` | v4.32.2 ActorHasMatchingGameplayTag |
| `manifest.yaml:9116-9220` | BTS_CalculateFormationPosition with MakeLiteralName |

---

## Appendix: v4.31 Changes (Still Relevant)

### FunctionResolver - Blueprint FunctionGraph Resolution
- Added Step 5 to resolution cascade for custom BP functions
- Used for CheckBackstabCondition and similar

### Generation Order
- Custom functions generated BEFORE event graph
- Ensures function exists when event graph references it

### CallFunction Self-Member Reference
- When `Resolved.bFound == true` but `Function == nullptr`
- Uses `SetSelfMember()` for Blueprint-defined functions
