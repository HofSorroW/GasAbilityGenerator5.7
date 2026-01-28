# Track E Removal Audit v7.7

## Summary

Track E (Native Bridge Pattern) has been completely removed from the GasAbilityGenerator plugin. All abilities now use AbilityTasks instead of the UDamageEventBridge for damage event handling.

**Version**: 7.7.0
**Date**: 2026-01-28
**Verification**: 194/194 assets generated successfully

---

## What Was Removed

### 1. Generator Code (~650 lines removed from GasAbilityGeneratorGenerators.cpp)

| Item | Description |
|------|-------------|
| `RequiresNativeBridge()` | Function detecting delegates with FGameplayEffectSpec params |
| `GetBridgeEventName()` | Helper to generate bridge event names |
| `GenerateBridgeBasedBinding()` | Bridge binding generation logic |
| `GenerateBridgeBindingNodes()` | Node creation for bridge pattern |
| `FValidatedBinding::bRequiresBridge` | Struct field removed |
| FDamageEventSummary type resolution | Removed from ResolveType() |
| FDamageEventSummary well-known struct | Removed from struct table |
| `#include "DamageEventBridge.h"` | Header include removed |

### 2. Runtime Module Removed

| Module | Location |
|--------|----------|
| GasAbilityGeneratorRuntime | Entire module deleted |
| DamageEventBridge.h | Header deleted |
| DamageEventBridge.cpp | Implementation deleted |

**Build.cs change**: Removed `GasAbilityGeneratorRuntime` from PrivateDependencyModuleNames.

### 3. Manifest Changes (2 abilities)

#### GA_ProtectiveDome
- **Removed**: `HandleDomeDamageAbsorption` CustomEvent node
- **Removed**: `BreakDamageEvent_Absorb` BreakStruct node
- **Removed**: 17 damage calculation nodes (GetFatherRef_Absorb, IsValid_Father_Absorb, Branch_Valid_Father, etc.)
- **Removed**: 59 connections for damage absorption handler flow
- **Kept**: Player death reset handler (Event_HandlePlayerDeathReset) - uses Actor params, not bridge

**Impact**: Dome no longer charges via damage absorption. Dome still activates/deactivates with form changes.

#### GA_StealthField
- **Removed**: `HandleStealthDamageBreak` CustomEvent node
- **Removed**: `HandleStealthAttackBreak` CustomEvent node
- **Removed**: `EndAbility_DamageBreak` and `EndAbility_AttackBreak` nodes
- **Removed**: 6 connections for stealth break handler flow

**Impact**: Stealth no longer breaks on damage. Stealth now ends only via 8-second duration timeout.

---

## Why Track E Was Removed

### The Problem
Narrative Pro's damage delegates (`OnDamagedBy`, `OnDealtDamage`) have signatures with `const FGameplayEffectSpec&` reference parameters. These are not Blueprint-compatible - Kismet crashes when trying to create handler functions with these signatures.

### Track E Solution (Now Removed)
Track E created `UDamageEventBridge` as a native C++ intermediary that:
1. Subscribed to the native delegate
2. Wrapped data into `FDamageEventSummary` (Blueprint-friendly struct)
3. Called a Blueprint CustomEvent with the summary

### Why Removed
1. **Complexity**: Required maintaining separate runtime module
2. **Two-pass compilation**: Delegate abilities required special Contract 10.1 exception (skip final compile)
3. **Brittleness**: Bridge pattern difficult to debug and maintain
4. **AbilityTasks exist**: Epic's GAS provides `WaitAttributeChange`, `WaitGameplayEffectApplied`, etc.

---

## Contract Changes

### Contract 10 Restored (Was 10.1)
```
Contract 10: Every Blueprint generator must call CompileBlueprint() and fail on errors.
```

The Contract 10.1 exception (skip final compile for delegate bindings) has been removed. All abilities now go through final compilation.

---

## Future Implementation (If Needed)

If damage detection functionality is needed in the future, use AbilityTask pattern:

### For GA_ProtectiveDome (Damage Absorption)
```yaml
# Use WaitAttributeChange to monitor player health
- id: WaitHealthChange
  type: AbilityTaskWaitAttributeChange
  properties:
    attribute: Health  # From NarrativeAttributeSetBase
    compare_type: LessThan
```

### For GA_StealthField (Break on Damage)
```yaml
# Use WaitGameplayEffectApplied to detect damage effects
- id: WaitDamageEffect
  type: AbilityTaskWaitGameplayEffectApplied
  properties:
    filter_tag: Effect.Damage
```

---

## Verification

```
=== GENERATION RESULTS ===
Pre-validation: 0 errors, 17 warnings, 406 checks
RESULT: New=194 Skipped=0 Failed=0
```

All 5 originally affected abilities now generate successfully:
- GA_FatherCrawler: 48/48 nodes, 59/59 connections
- GA_FatherArmor: 58/58 nodes, 74/74 connections
- GA_FatherSymbiote: 77/77 nodes, 106/106 connections
- GA_ProtectiveDome: 29/29 nodes, 35/35 connections
- GA_StealthField: 21/21 nodes, 24/24 connections

---

## Files Modified

| File | Change |
|------|--------|
| GasAbilityGeneratorGenerators.cpp | Removed ~650 lines of Track E code |
| GasAbilityGeneratorGenerators.h | Removed 3 function declarations |
| GasAbilityGenerator.Build.cs | Removed GasAbilityGeneratorRuntime dependency |
| manifest.yaml | Removed orphaned handlers from GA_ProtectiveDome, GA_StealthField |

| File | Change |
|------|--------|
| DamageEventBridge.h | DELETED |
| DamageEventBridge.cpp | DELETED |
| GasAbilityGeneratorRuntime module | DELETED |
