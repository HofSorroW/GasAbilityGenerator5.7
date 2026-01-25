# GA_Backstab & GA_ProtectiveDome Error Audit

**Date:** 2026-01-24 (Original) | 2026-01-25 (v4.31-v4.32.3 Implementation)
**Status:** ✅ FULLY RESOLVED - 187/187 assets passing (0 failures)
**Context:** Debug analysis of event graph generation failures + BTS/GoalGenerator/AC fixes

---

## Executive Summary

**ORIGINAL CLAIM:** Two abilities (GA_ProtectiveDome, GA_Backstab) fail event graph generation due to manifest syntax issues.

**FINAL RESOLUTION (v4.31-v4.32.3):**
- GA_Backstab: Fixed function name `BP_ApplyGameplayEffectSpecToOwner` → `K2_ApplyGameplayEffectSpecToOwner`
- GA_ProtectiveDome: Already working (SKIP)
- BTS_CalculateFormationPosition: Fixed MakeLiteralName for blackboard KeyName params
- BTS_AdjustFormationSpeed: Fixed Object→Actor cast + MakeLiteralName
- BTS_CheckExplosionProximity: Fixed BP_ApplyGameplayEffectToSelf + K2_DestroyActor
- BTS_HealNearbyAllies: Fixed BP_ApplyGameplayEffectToSelf + full GE path
- GoalGenerator_Alert: Fixed MakeArray nodes + DynamicCast to NPCActivityComponent
- GoalGenerator_RandomAggression: Fixed pin naming + target_self for SetTimer
- AC_* startup_effects: Added NPC/Enemy subfolder search paths (v4.32.3)

| Issue | Original Claim | Resolution | Status |
|-------|----------------|------------|--------|
| 1. `parameters:` block | NOT supported | ✅ Parser already converts | **LOCKED** |
| 2. Pin `Spec`/`Tag` | Not found | ✅ Works with correct function names | **RESOLVED** |
| 3. Custom function call | Resolution fails | ✅ v4.31 fixes applied | **RESOLVED** |
| 4. TSubclassOf variables | Type mismatch | ✅ v4.32.2 parser fix | **RESOLVED** |
| 5. Blackboard KeyName | By-ref param error | ✅ v4.32.2 MakeLiteralName | **RESOLVED** |
| 6. Object→Actor cast | Type mismatch | ✅ v4.32.2 manifest fix | **RESOLVED** |
| 7. VariableGet pin naming | Wrong pin name | ✅ v4.32.3 manifest fix | **RESOLVED** |
| 8. Self node reference | Self.Reference fails | ✅ target_self: true | **RESOLVED** |
| 9. SphereOverlapActors arrays | MakeArray required | ✅ v4.32.3 manifest fix | **RESOLVED** |
| 10. GetComponentByClass cast | ActorComponent→NPCActivityComponent | ✅ DynamicCast node | **RESOLVED** |
| 11. AC startup_effects paths | Effect not found | ✅ v4.32.3 search paths | **RESOLVED** |

---

## v4.32.3 Code Changes (2026-01-25)

### Change 1: AbilityConfiguration startup_effects Search Paths

**File:** `GasAbilityGeneratorGenerators.cpp` (lines 19580-19591)

Added search paths for NPC and enemy effect subdirectories:
```cpp
// v4.32.3: NPC and enemy effect subfolders
SearchPaths.Add(FString::Printf(TEXT("%s/NPCs/Support/Effects/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
SearchPaths.Add(FString::Printf(TEXT("%s/Enemies/Possessed/Effects/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
SearchPaths.Add(FString::Printf(TEXT("%s/Enemies/Warden/Effects/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
SearchPaths.Add(FString::Printf(TEXT("%s/Enemies/Biomech/Effects/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
```

This resolves startup_effects references to GE_* assets in NPC/Enemy subdirectories.

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

## Final Status (2026-01-25 ~03:55)

### Result: 187/187 Assets Passing (0 Failures)

All assets now generate successfully:
- ✅ GA_Backstab (SKIP)
- ✅ GA_ProtectiveDome (SKIP)
- ✅ BTS_CalculateFormationPosition (SKIP)
- ✅ BTS_AdjustFormationSpeed (SKIP)
- ✅ BTS_CheckExplosionProximity (SKIP)
- ✅ BTS_HealNearbyAllies (SKIP)
- ✅ GoalGenerator_Alert (SKIP)
- ✅ GoalGenerator_RandomAggression (SKIP)
- ✅ AC_SupportBuffer (SKIP)
- ✅ AC_PossessedExploder (SKIP)
- ✅ AC_WardenHusk (SKIP)
- ✅ AC_WardenCore (SKIP)
- ✅ AC_BiomechHost (SKIP)
- ✅ AC_BiomechCreature (SKIP)
- ✅ All other 173 assets (SKIP)

---

## Locked Decisions (Final - 2026-01-25)

| Decision ID | Topic | Status | Evidence |
|-------------|-------|--------|----------|
| D1 | `parameters:` block | ✅ **LOCKED** | Source code proves parser converts |
| D2 | Function names (K2_*) | ✅ **LOCKED** | K2_ApplyGameplayEffectSpecToOwner works |
| D3 | TSubclassOf parsing | ✅ **LOCKED** | v4.32.2 parser fix deployed |
| D4 | MakeLiteralName for by-ref | ✅ **LOCKED** | Pattern works for blackboard keys |
| D5 | Object→Actor cast | ✅ **LOCKED** | Required for GetValueAsObject → Actor functions |
| D6 | Double precision remapping | ✅ **LOCKED** | Add_DoubleDouble etc. |
| D7 | Pin naming (VariableGet) | ✅ **LOCKED** | Use variable name, not "Value" |
| D8 | Self node → target_self | ✅ **LOCKED** | target_self: true on CallFunction |
| D9 | SphereOverlapActors arrays | ✅ **LOCKED** | MakeArray for ObjectTypes/ActorsToIgnore |
| D10 | GetComponentByClass cast | ✅ **LOCKED** | DynamicCast to specific component type |
| D11 | startup_effects search paths | ✅ **LOCKED** | v4.32.3 adds NPC/Enemy subdirs |

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
