# Delegate Binding System - Final Reference

**Version:** 1.0 (Consolidated from v7.3-v7.7 audit documents)
**Date:** 2026-01-28
**Status:** CLOSED - Track E removed, AbilityTasks recommended for damage detection

---

## Executive Summary

Delegate bindings with `const FGameplayEffectSpec&` reference parameters caused editor crashes due to a UE5 Blueprint compilation stage ordering bug. After investigating multiple fix tracks, Track E (Native Bridge Pattern) was implemented in v7.5.7, then removed in v7.7.0 due to complexity. The recommended approach is to use GAS AbilityTasks instead.

---

## The Problem

### Crash Pattern
Assets with `delegate_bindings` referencing `OnDamagedBy`, `OnDealtDamage`, or `OnHealedBy` delegates crash when loaded via `StaticLoadObject` with a `FindPinChecked` assertion failure at `EdGraphNode.h:586`.

### Root Cause (Proven)
Narrative Pro delegates include reference parameters that Blueprint cannot handle:

```cpp
// CRASHES - Has const reference parameter
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDamagedBy,
    UNarrativeAbilitySystemComponent*, DamagerCauserASC,
    const float, Damage,
    const FGameplayEffectSpec&, Spec);  // <-- TRIGGER

// SAFE - No reference parameters
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDied,
    AActor*, KilledActor,
    UNarrativeAbilitySystemComponent*, KilledActorASC);
```

### Technical Root Cause Chain

| Step | What Happens |
|------|--------------|
| 1 | Generator creates CustomEvent with delegate params |
| 2 | `ConvertPropertyToPinType` called on `const FGameplayEffectSpec&` param |
| 3 | `bIsReference = false` because UE requires `CPF_OutParm\|CPF_ReferenceParm` (input refs only have `CPF_ReferenceParm`) |
| 4 | Handler's function signature lacks `CPF_ReferenceParm` flag |
| 5 | Asset saved and reloaded |
| 6 | **STAGE VIII**: `FastGenerateSkeletonClass` runs |
| 7 | `RegisterConvertibleDelegates` called (BEFORE node reconstruction) |
| 8 | `IsSignatureCompatibleWith` fails due to `CPF_ReferenceParm` mismatch |
| 9 | Code enters float-conversion branch, calls `FindPinChecked(PSC_Self)` |
| 10 | **STAGE IX** hasn't run yet - Self pin not allocated |
| 11 | **CRASH** - `FindPinChecked` asserts on missing pin |

**Key insight:** The Self pin is accessed in STAGE VIII but allocated in STAGE IX.

---

## Fix Tracks Evaluated

| Track | Approach | Risk | Outcome |
|-------|----------|------|---------|
| A | Engine null-check at KismetCompiler.cpp:5845 | Low | Requires engine patch |
| B | Fix stage ordering in compilation | High | Architectural change |
| C | Generator workaround (skip bindings) | Medium | Loses functionality |
| D | Schema fix for reference detection | Medium | May affect other systems |
| **E** | **Native Bridge Component** | Low | **Implemented, then removed** |

---

## Track E Implementation (v7.5.5 - v7.5.7)

### What It Did
Created `UDamageEventBridge` C++ component that:
1. Bound to native delegates with exact signatures (including `const FGameplayEffectSpec&`)
2. Wrapped event data into `FDamageEventSummary` (Blueprint-friendly struct)
3. Broadcast via Blueprint-safe multicast delegate

### Components Created
- `GasAbilityGeneratorRuntime` module (Runtime type)
- `UDamageEventBridge` component
- `FDamageEventSummary` struct
- Two-pass delegate binding generation
- Contract 10.1 (skip final compile for delegate bindings)

### Why It Was Removed (v7.7.0)

1. **Complexity**: Required maintaining separate runtime module
2. **Two-pass compilation**: Delegate abilities required special Contract 10.1 exception
3. **Brittleness**: Bridge pattern difficult to debug and maintain
4. **AbilityTasks exist**: Epic's GAS provides `WaitAttributeChange`, `WaitGameplayEffectApplied`, etc.

---

## Current State (v7.7.0+)

### What Was Removed

**Generator Code (~650 lines):**
- `RequiresNativeBridge()` - Bridge detection
- `GetBridgeEventName()` - Event name helper
- `GenerateBridgeBasedBinding()` - Bridge binding logic
- `GenerateBridgeBindingNodes()` - Node creation
- All FDamageEventSummary references

**Runtime Module (entire module deleted):**
- DamageEventBridge.h
- DamageEventBridge.cpp
- GasAbilityGeneratorRuntime.Build.cs

**Manifest Changes:**
- GA_ProtectiveDome: Removed damage absorption handler (17 nodes, 59 connections)
- GA_StealthField: Removed stealth break handlers (2 events, 6 connections)

### Contract Changes

Contract 10.1 exception removed. All abilities now go through final compilation per Contract 10.

### Affected Delegates

| Delegate | Has Ref Param | Route |
|----------|---------------|-------|
| OnDamagedBy | `const FGameplayEffectSpec&` | **DO NOT USE** |
| OnDealtDamage | `const FGameplayEffectSpec&` | **DO NOT USE** |
| OnHealedBy | `const FGameplayEffectSpec&` | **DO NOT USE** |
| OnDied | None (pointers only) | Safe for Blueprint |
| OnKilled | None (pointers only) | Safe for Blueprint |

---

## Recommended Alternative: AbilityTasks

If damage detection functionality is needed, use GAS AbilityTask pattern:

### For Damage Detection (e.g., GA_ProtectiveDome)
```yaml
# Use WaitAttributeChange to monitor health
- id: WaitHealthChange
  type: AbilityTaskWaitAttributeChange
  properties:
    attribute: Health
    compare_type: LessThan
```

### For Effect Detection (e.g., GA_StealthField)
```yaml
# Use WaitGameplayEffectApplied to detect damage effects
- id: WaitDamageEffect
  type: AbilityTaskWaitGameplayEffectApplied
  properties:
    filter_tag: Effect.Damage
```

---

## Verification (v7.7.0)

```
Pre-validation: 0 errors, 17 warnings, 406 checks
RESULT: New=194 Skipped=0 Failed=0
```

All 5 originally affected abilities generate successfully:
- GA_FatherCrawler: 48/48 nodes, 59/59 connections
- GA_FatherArmor: 58/58 nodes, 74/74 connections
- GA_FatherSymbiote: 77/77 nodes, 106/106 connections
- GA_ProtectiveDome: 29/29 nodes, 35/35 connections
- GA_StealthField: 21/21 nodes, 24/24 connections

---

## Key Files Referenced (Historical)

| File | Lines | Content |
|------|-------|---------|
| BlueprintCompilationManager.cpp | 1061-1082, 1260-1278 | Stage VIII/IX ordering |
| KismetCompiler.cpp | 5845 | RegisterConvertibleDelegates crash point |
| EdGraphSchema_K2.cpp | 3673 | bIsReference logic |
| Class.cpp | 7806-7847 | IsSignatureCompatibleWith |
| K2Node_CreateDelegate.cpp | 366-372 | Epic's null check acknowledgment |
| NarrativeAbilitySystemComponent.h | 56+ | Delegate signatures |

---

## Version History

| Version | Date | Status |
|---------|------|--------|
| v7.3 | 2026-01-27 | Root cause identified, Track E designed |
| v7.5.5 | 2026-01-27 | Two-pass binding implemented |
| v7.5.6 | 2026-01-27 | Runtime module created |
| v7.5.7 | 2026-01-27 | Contract 10.1 added, Track E complete |
| v7.7.0 | 2026-01-28 | **Track E removed**, Contract 10 restored |

---

## Conclusion

The delegate binding crash is a UE5 engine issue with input const-ref parameters in dynamic delegates. Rather than maintaining a complex workaround (Track E), the generator now:

1. **Avoids** delegates with `const FGameplayEffectSpec&` parameters
2. **Uses** safe delegates (`OnDied`, `OnKilled`) when needed
3. **Recommends** AbilityTasks for damage/effect detection

This approach is simpler, more maintainable, and aligns with Epic's intended GAS patterns.
