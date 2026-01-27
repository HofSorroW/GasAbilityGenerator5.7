# Delegate Binding Crash Audit v7.3

**Date:** 2026-01-27
**Auditors:** Claude (Opus 4.5) + GPT (consensus)
**Status:** ROOT CAUSE IDENTIFIED - Fix pending

---

## Executive Summary

Float-bearing delegate bindings (OnDamagedBy, OnDealtDamage, OnHealedBy) cause editor crash on asset load. The crash occurs in `RegisterConvertibleDelegates` when the compiler attempts to access a Self pin that is missing from the `UK2Node_CreateDelegate` node after Blueprint reconstruction.

---

## Proven Findings

### A/B Isolation Test Results

| Test | Delegate | Float Parameter | Result |
|------|----------|-----------------|--------|
| Test A | OnDied | None | **PASS** - Editor reload survives |
| Test B | OnDamagedBy | `Float Damage` | **CRASH** - Editor startup crash |

### Crash Analysis

**Crash Location:**
```
KismetCompiler.cpp:5845
FKismetCompilerContext::RegisterConvertibleDelegates()
```

**Crash Trigger:**
```cpp
// Line 5845 - CRASH POINT
const UEdGraphPin* DelegateNodeSelfPin = CreateDelegateNode->FindPinChecked(UEdGraphSchema_K2::PSC_Self);
```

**Gate Conditions:**
1. `BoundFunction && DelegateSignature` must be valid
2. `DoSignaturesHaveConvertibleFloatTypes()` must return `HasConvertibleFloatParams`

Non-float delegates bypass Gate 2, never touching the Self pin lookup.

### Evidence Chain

1. **Generation succeeds:** Commandlet reports 47/47 nodes, 59/59 connections
2. **Editor crashes on startup:** Content Browser discovery triggers compile-on-load
3. **Quarantine test:** Removing the float-delegate asset allows editor to start
4. **Single asset sufficient:** GA_ProtectiveDome alone crashes the editor

---

## Root Cause

The `UK2Node_CreateDelegate` node loses its Self pin during Blueprint reconstruction/compile-on-load. The pin exists at generation time but is not present when the compiler runs `RegisterConvertibleDelegates`.

**Why float delegates crash and non-float delegates survive:**
- Float delegates trigger `RegisterConvertibleDelegates` which calls `FindPinChecked(PSC_Self)`
- Non-float delegates bypass this code path entirely
- The Self pin is missing in BOTH cases, but only float delegates expose the defect

---

## Fix Tracks

### Track A: Root Fix (Recommended)

Ensure `UK2Node_CreateDelegate` reconstruction preserves the Self pin.

**Investigation areas:**
- `AllocateDefaultPins()` timing relative to skeleton state
- Node modifications after initial pin allocation
- Save/load serialization of pin state
- `PostReconstructNode()` or `ReconstructNode()` behavior

### Track B: Safety Hardening

Prevent `StaticLoadObject` in metadata checks from triggering compile-on-load.

**Location:** `GasAbilityGeneratorGenerators.cpp:1193` (`CheckExistsWithMetadata`)

**Limitation:** Does not fix the underlying node defect; only prevents generator from exposing it.

---

## Temporary Mitigation

All delegate bindings on GA_ProtectiveDome have been disabled in manifest.yaml:

```yaml
# v7.3 AUDIT: Delegate bindings DISABLED pending crash fix
# See: ClaudeContext/Handoffs/Delegate_Binding_Crash_Audit_v7_3.md
# delegate_bindings:
#   - id: BindDomeDamageAbsorption
#     delegate: OnDamagedBy
#     ...
```

---

## Affected Delegates (Float Parameters)

| Delegate | Class | Float Parameter |
|----------|-------|-----------------|
| OnDamagedBy | NarrativeAbilitySystemComponent | `Float Damage` |
| OnDealtDamage | NarrativeAbilitySystemComponent | `Float Damage` |
| OnHealedBy | NarrativeAbilitySystemComponent | `Float Heal` |

**Safe Delegates (No Float):**
- OnDied
- OnKilled

---

## Files Modified

| File | Change |
|------|--------|
| `ClaudeContext/manifest.yaml` | Disabled delegate_bindings on GA_ProtectiveDome |
| `Content/FatherCompanion/Abilities/Actions/GA_ProtectiveDome.uasset` | Regenerated without delegate bindings |

---

## Next Steps

1. **Implement Track A fix** - Investigate UK2Node_CreateDelegate pin persistence
2. **Re-enable delegate bindings** - After fix is verified
3. **Expand to other GAs** - GA_Sacrifice, GA_DomeBurst also have delegate bindings (currently disabled)

---

## Audit Verification Commands

```bash
# Reproduce crash (float delegate)
# 1. Enable OnDamagedBy in manifest
# 2. Delete GA_ProtectiveDome.uasset
# 3. Run cycle
# 4. Launch editor -> CRASH

# Verify fix (non-float delegate)
# 1. Enable OnDied only in manifest
# 2. Delete GA_ProtectiveDome.uasset
# 3. Run cycle
# 4. Launch editor -> SUCCESS
```
