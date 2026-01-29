# Exec-to-Pure Connection Audit v1.0

**Date:** 2026-01-29
**Auditors:** Claude (Opus 4.5), GPT
**Scope:** Verify all exec-to-pure patterns in manifest.yaml and generator behavior

---

## Executive Summary

Comprehensive audit of exec-to-pure connection patterns in the manifest, validating the "clean pattern" fix for GoalGenerator_RandomAggression and verifying all 22 potential exec-to-pure patterns.

**Verdict:** All patterns are safe. Generator bypass system correctly handles pure function exec connections.

---

## Key Discovery: UHT Auto-Pure Rule

**Source:** [Epic Forums](https://forums.unrealengine.com/t/can-a-function-be-const-but-not-pure/329862)

UnrealHeaderTool (UHT) automatically adds `FUNC_BlueprintPure` to functions that meet ALL criteria:
1. `BlueprintCallable` specifier
2. `const` member function
3. Has output (return value or out parameters)

**Code location:** `HeaderParser.cpp`
```cpp
// "If its a const BlueprintCallable function with some sort of output, mark it as BlueprintPure as well"
FuncInfo.FunctionFlags |= FUNC_BlueprintPure;
```

**Generator detection:** `UK2Node_CallFunction::bDefaultsToPureFunc` is set from `FUNC_BlueprintPure` (K2Node_CallFunction.cpp:1133)

---

## Verified Functions

### Explicit BlueprintPure (UE5.7 Source Verified)

| Function | Header | Line | Evidence |
|----------|--------|------|----------|
| `RandomFloat` | KismetMathLibrary.h | 661 | `UFUNCTION(BlueprintPure, ...)` |
| `Less_DoubleDouble` | KismetMathLibrary.h | 562 | `UFUNCTION(BlueprintPure, ...)` |
| `MakeTransform` | KismetMathLibrary.h | 3931 | `UFUNCTION(BlueprintPure, ...)` |

### Auto-Pure (const + BlueprintCallable + output)

| Function | Header | Evidence |
|----------|--------|----------|
| `K2_GetActorLocation` | Actor.h:1570 | `BlueprintCallable`, `const`, returns `FVector` |
| `MakeOutgoingGameplayEffectSpec` | GameplayAbility.h:226 | `BlueprintCallable`, `const`, returns `FGameplayEffectSpecHandle` |
| `HasAuthority` | Actor.h:1941 | `BlueprintCallable`, `const`, returns `bool` |

### Truly Impure (Valid Exec Pins)

| Function | Header | Evidence |
|----------|--------|----------|
| `K2_CommitAbilityCooldown` | GameplayAbility.h:341 | `BlueprintCallable`, NOT const |
| `K2_EndAbility` | GameplayAbility.h:613 | `BlueprintCallable`, NOT const, void return |
| `SetValueAsVector` | BlackboardComponent.h:157 | `BlueprintCallable`, NOT const, void return |
| `VariableSet` | Node type | N/A - inherently has exec pins |

---

## All 22 Exec-to-Pure Patterns Classified

| Line | Node ID | Function | Pure Type | Status |
|------|---------|----------|-----------|--------|
| 3034 | MakeSpec_SymbioteDuration | MakeOutgoingGameplayEffectSpec | Auto-pure | BYPASS |
| 3627 | GetFatherLocation | K2_GetActorLocation | Auto-pure | BYPASS |
| 3651 | GetFatherLocation | K2_GetActorLocation | Auto-pure | BYPASS |
| 4446 | MakeGESpec | MakeOutgoingGameplayEffectSpec | Auto-pure | BYPASS |
| 4868 | MakeGESpec | MakeOutgoingGameplayEffectSpec | Auto-pure | BYPASS |
| 5513 | MakeGESpec | MakeOutgoingGameplayEffectSpec | Auto-pure | BYPASS |
| 5568 | CommitCooldown_Break | K2_CommitAbilityCooldown | Impure | VALID |
| 5570 | EndAbility_Break | K2_EndAbility | Impure | VALID |
| 5692 | MakeGESpec | MakeOutgoingGameplayEffectSpec | Auto-pure | BYPASS |
| 6048 | MakeGESpec | MakeOutgoingGameplayEffectSpec | Auto-pure | BYPASS |
| 6216 | MakeGESpec | MakeOutgoingGameplayEffectSpec | Auto-pure | BYPASS |
| 9405 | GetActorLocation | K2_GetActorLocation | Auto-pure | BYPASS |
| 9407 | GetActorLocation | K2_GetActorLocation | Auto-pure | BYPASS |
| 9419 | MakeSpawnTransform | MakeTransform | Explicit-pure | BYPASS |
| 10426 | SetTargetLocation | SetValueAsVector | Impure | VALID |
| 11585 | MakeSpec | MakeOutgoingGameplayEffectSpec | Auto-pure | BYPASS |
| 12039 | GetPawnLocation | K2_GetActorLocation | Auto-pure | BYPASS |
| 12534 | GetActorLocation | K2_GetActorLocation | Auto-pure | BYPASS |
| 12536 | GetActorLocation | K2_GetActorLocation | Auto-pure | BYPASS |
| 12548 | MakeSpawnTransform | MakeTransform | Explicit-pure | BYPASS |
| 13608 | GetOwnerLocation | K2_GetActorLocation | Auto-pure | BYPASS |
| 13662 | SetMoveTargetLocation | VariableSet | Impure | VALID |

### Summary

| Category | Count |
|----------|-------|
| Truly impure (valid exec) | 4 |
| Auto-pure (const + output) | 16 |
| Explicit pure (BlueprintPure) | 2 |
| **Total** | **22** |

---

## GoalGenerator_RandomAggression Fix Verification

### Clean Pattern Applied (v7.8.29)

**Before (invalid):**
```yaml
- from: [BranchNotFollowing, False]
  to: [RandomFollowRoll, Exec]  # Invalid - RandomFloat is pure, no Exec pin
- from: [RandomFollowRoll, Then]
  to: [BranchFollowRoll, Exec]  # Invalid - no Then pin
```

**After (correct):**
```yaml
- from: [BranchNotFollowing, False]
  to: [BranchFollowRoll, Exec]  # Direct exec to impure node
# Data flow through pure nodes
- from: [RandomFollowRoll, ReturnValue]
  to: [LessFollowChance, A]
- from: [LessFollowChance, ReturnValue]
  to: [BranchFollowRoll, Condition]
```

### Risk Assessment

| Risk | Status | Evidence |
|------|--------|----------|
| Random fanout | SAFE | Each RandomFloat.ReturnValue used exactly once |
| Missing dataflow | SAFE | All A, B, Condition pins connected |
| Pure evaluation | CORRECT | Branch evaluates pure chain when Condition needed |

---

## Generator Pure Bypass System

### Detection (GasAbilityGeneratorGenerators.cpp:13939-13949)

```cpp
bool bFromNodeIsPure = FromNode->IsNodePure();
bool bToNodeIsPure = ToNode->IsNodePure();

if (bFromNodeIsPure || bToNodeIsPure)
{
    LogGeneration("SKIP exec on pure node: ...");
    return true; // Handled, not failure
}
```

### UK2Node::IsNodePure() Implementation

For `UK2Node_CallFunction` (K2Node_CallFunction.cpp:858-860):
```cpp
bool UK2Node_CallFunction::IsNodePure() const
{
    return !AreExecPinsVisible();
}
```

`AreExecPinsVisible()` returns false when `bDefaultsToPureFunc == true` (set from `FUNC_BlueprintPure`).

---

## Issue Fixed: Logging Accuracy (v7.8.31)

### Problem

`ConnectPins()` returned `true` for both "connected" and "skipped pure", causing:
- "Connected:" logged for skipped connections
- `ConnectionsCreated++` incremented incorrectly
- Misleading "49/49 connections" when some were skipped

### Solution

Changed `ConnectPins()` to return enum distinguishing:
- `EConnectResult::Connected` - Actual connection made
- `EConnectResult::SkippedPure` - Exec-to-pure bypassed
- `EConnectResult::Failed` - Connection error

Logs now correctly show:
- "Connected:" for actual connections
- "Skipped (pure bypass):" for exec-to-pure
- "FAILED:" for errors

---

## References

- **UHT Auto-Pure:** [Epic Forums](https://forums.unrealengine.com/t/can-a-function-be-const-but-not-pure/329862)
- **Blueprint Pure Functions:** [raharudev guide](https://raharuu.github.io/unreal/blueprint-pure-functions-complicated/)
- **UE5.7 Source:** KismetMathLibrary.h, Actor.h, GameplayAbility.h, K2Node_CallFunction.cpp
- **Generator Source:** GasAbilityGeneratorGenerators.cpp

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| v1.0 | 2026-01-29 | Initial audit - 22 patterns verified, UHT auto-pure rule documented |
| v1.1 | 2026-01-29 | Generator logging fix implemented (v7.8.31) - EConnectResult enum, accurate counters |
