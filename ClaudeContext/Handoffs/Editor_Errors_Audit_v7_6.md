# Editor Errors Audit v7.6

**Date:** 2026-01-28
**Status:** ⚠️ SUPERSEDED (v7.7.0) - Track E removed. See `Track_E_Removal_Audit_v7_7.md`

> **NOTE (v7.7.0):** This document describes delegate binding errors that existed **before** Track E was removed.
> All delegate bindings described here (GA_ProtectiveDome, GA_StealthField, GA_FatherCrawler, etc.) have been
> **removed** from the manifest in v7.7.0. The abilities still function but without damage detection features.
>
> The issues documented here were part of why Track E was ultimately removed - the editor compile pass
> would clear delegate bindings regardless of the generator's Contract 10.1 exception.

**Original Context (Historical):** Contract 10.1 skips final compile for delegate-binding abilities to prevent SelectedFunctionName clearing. However, the editor performs its own compile when loading Blueprints, which clears the delegate bindings anyway.

---

## Summary

| Severity | Count | Type |
|----------|-------|------|
| HIGH (broken functionality) | 5 | Delegate binding errors |
| LOW (warning only) | 2 | Redundant cast warnings |

**Root Cause:** UK2Node_CreateDelegate's `ReconstructNode()` clears `SelectedFunctionName` during editor compile when it can't validate the delegate signature. This affects both:
- **Regular path:** CustomEvent handlers in the same Blueprint
- **Bridge path:** UDamageEventBridge handlers (generated Blueprint class, not native C++)

---

## Errors by Ability

### 1. GA_FatherArmor

**Errors:**
```
Create Event  : missing a function/event name.
Failed to create property K2Node_CreateDelegate_OutputDelegate_1 from <None> due to a bad or unknown type (Delegate)
Create Event Signature Error: No function/event specified.
```

**Delegate Bindings:**
| ID | Delegate | Source | Handler | Path | Status |
|----|----------|--------|---------|------|--------|
| BindArmorDamage | OnDamagedBy | OwnerASC | HandleArmorDamageReceived | Bridge | WORKS |
| BindFatherDeath | OnDied | FatherASC | HandleFatherDied | Regular | BROKEN |

**Impact:** Death handling won't trigger. When Father dies in Armor form, cleanup logic (detach, end ability) won't execute.

**Suggested Fix:** Remove `BindFatherDeath` delegate binding. Add `HandleDeath` function override instead (same pattern as BP_WardenHusk).

---

### 2. GA_FatherCrawler

**Errors:**
```
Create Event  : missing a function/event name.
Create Event  : missing a function/event name.
Failed to create property K2Node_CreateDelegate_OutputDelegate_1 from <None> due to a bad or unknown type (Delegate)
Failed to create property K2Node_CreateDelegate_OutputDelegate_2 from <None> due to a bad or unknown type (Delegate)
Create Event Signature Error: No function/event specified.
Create Event Signature Error: No function/event specified.
```

**Delegate Bindings:**
| ID | Delegate | Source | Handler | Path | Status |
|----|----------|--------|---------|------|--------|
| BindCrawlerDamageTaken | OnDamagedBy | OwnerASC | HandleCrawlerDamageTaken | Bridge | BROKEN |
| BindCrawlerDamageDealt | OnDealtDamage | OwnerASC | HandleCrawlerDamageDealt | Bridge | BROKEN |

**Impact:**
- Damage tracking broken (HandleCrawlerDamageTaken never fires)
- Ultimate charge accumulation broken (HandleCrawlerDamageDealt never fires)

**Suggested Fix:** Remove both delegate bindings. Replace with:
- Option A: `WaitGameplayEvent` ability task to listen for damage/dealt events
- Option B: Move tracking logic to GameplayEffect execution calculations
- Option C: Use `AbilityTask_WaitGameplayTagAdded` if damage applies a tag

---

### 3. GA_FatherSymbiote

**Errors:**
```
Create Event  : missing a function/event name.
Create Event  : missing a function/event name.
Failed to create property K2Node_CreateDelegate_OutputDelegate_1 from <None> due to a bad or unknown type (Delegate)
Failed to create property K2Node_CreateDelegate_OutputDelegate_2 from <None> due to a bad or unknown type (Delegate)
Create Event Signature Error: No function/event specified.
Create Event Signature Error: No function/event specified.
```

**Delegate Bindings:**
| ID | Delegate | Source | Handler | Path | Status |
|----|----------|--------|---------|------|--------|
| BindSymbioteDamageDealt | OnDealtDamage | OwnerASC | HandleSymbioteDamageDealt | Bridge | BROKEN |
| BindSymbioteHealing | OnHealedBy | OwnerASC | HandleSymbioteHealing | Bridge | BROKEN |

**Impact:**
- Damage tracking broken (HandleSymbioteDamageDealt never fires)
- Healing/regen tracking broken (HandleSymbioteHealing never fires)

**Suggested Fix:** Same as GA_FatherCrawler - remove delegate bindings and use alternative approach.

---

### 4. GA_ProtectiveDome

**Errors:**
```
Create Event  : missing a function/event name.
Failed to create property K2Node_CreateDelegate_OutputDelegate_2 from <None> due to a bad or unknown type (Delegate)
Create Event Signature Error: No function/event specified.
```

**Delegate Bindings:**
| ID | Delegate | Source | Handler | Path | Status |
|----|----------|--------|---------|------|--------|
| BindDomeDamageAbsorption | OnDamagedBy | OwnerASC | HandleDomeDamageAbsorption | Bridge | WORKS |
| BindPlayerDeath | OnDied | OwnerASC | HandlePlayerDeathReset | Regular | BROKEN |

**Impact:** Player death won't reset dome state. If player dies with dome active, state may be corrupted on respawn.

**Suggested Fix:** Remove `BindPlayerDeath` delegate binding. Add `HandleDeath` function override or use respawn event.

---

### 5. GA_StealthField

**Errors:**
```
Create Event  : missing a function/event name.
Create Event  : missing a function/event name.
Failed to create property K2Node_CreateDelegate_OutputDelegate_1 from <None> due to a bad or unknown type (Delegate)
Failed to create property K2Node_CreateDelegate_OutputDelegate_2 from <None> due to a bad or unknown type (Delegate)
Create Event Signature Error: No function/event specified.
Create Event Signature Error: No function/event specified.
```

**Delegate Bindings:**
| ID | Delegate | Source | Handler | Path | Status |
|----|----------|--------|---------|------|--------|
| BindStealthDamageBreak | OnDamagedBy | OwnerASC | HandleStealthDamageBreak | Bridge | BROKEN |
| BindStealthAttackBreak | OnDealtDamage | OwnerASC | HandleStealthAttackBreak | Bridge | BROKEN |

**Impact:**
- Stealth doesn't break when player takes damage
- Stealth doesn't break when player attacks

**Suggested Fix:** Remove both delegate bindings. Replace with:
- `WaitGameplayEvent` ability task to listen for damage taken/dealt
- Or use `WaitGameplayTagAdded` if damage applies a combat tag

---

### 6. GA_FatherEngineer

**Errors:**
```
'ReturnValue' is already a 'NPCActivity Component', you don't need Cast To NPCActivityComponent.
```

**Severity:** LOW (warning only - ability functions correctly)

**Impact:** None - redundant cast works, just inefficient.

**Suggested Fix:** Find and remove the redundant `CastToActivityComp` node in the event graph. The `GetComponentByClass` with `param.ComponentClass: NPCActivityComponent` already returns the correct type.

---

### 7. GoalGenerator_Alert

**Errors:**
```
'ReturnValue' is already a 'NPCActivity Component', you don't need Cast To NPCActivityComponent.
```

**Severity:** LOW (warning only - goal generator functions correctly)

**Impact:** None - redundant cast works, just inefficient.

**Suggested Fix:** Find and remove the redundant `CastToActivityComp` node in the event graph. Same issue as GA_FatherEngineer.

---

## Technical Background

### Why Bridge Path Also Fails

The Track E Native Bridge creates `UDamageEventBridge` as a **generated Blueprint class** (`NewObject<UBlueprintGeneratedClass>()`), not a native C++ class. When the editor loads a Blueprint that references the bridge:

1. Editor calls `CompileBlueprint()` during load
2. `ReconstructNode()` is called on CreateDelegate nodes
3. Bridge class may not be loaded/found yet
4. `SelectedFunctionName` gets cleared

### Contract 10.1 Limitation

Contract 10.1 (skip final compile for delegate-binding abilities) only helps during **generation**. It cannot prevent the editor's compile pass when the user opens the Blueprint.

### Alternative Approaches

1. **WaitGameplayEvent** - Ability task that waits for gameplay events
2. **HandleDeath function override** - Override the NarrativeCharacter death handler
3. **GE Execution Calculations** - Move logic into GameplayEffect executions
4. **Tag-based polling** - Check for damage/combat tags periodically
5. **Native C++ Bridge** - Create actual C++ classes for bridge handlers (significant work)

---

## Implementation Priority

1. **HIGH:** GA_StealthField - Stealth break is core gameplay
2. **HIGH:** GA_FatherArmor, GA_ProtectiveDome - Death handling is safety-critical
3. **MEDIUM:** GA_FatherCrawler, GA_FatherSymbiote - Damage tracking affects progression
4. **LOW:** GA_FatherEngineer, GoalGenerator_Alert - Just warnings, functionality works

---

## Code Change (v7.6)

Warnings now fail generation (stricter quality gate):

```cpp
// v7.6: Also fail on warnings - all compile issues should be fixed
if (!bHasDelegateBindings && (CompileLog.NumErrors > 0 || CompileLog.NumWarnings > 0))
```

This ensures new issues are caught during generation rather than discovered later in the editor.

---

## Next Steps

1. Fix the 2 warning issues (remove redundant casts) - Quick wins
2. Design alternative implementations for delegate bindings
3. Update manifest with new implementations
4. Test at runtime to verify functionality
5. Consider native C++ bridge classes for long-term solution
