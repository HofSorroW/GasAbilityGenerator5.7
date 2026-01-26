# Pipeline Gap Analysis Audit v1.0

**Date:** 2026-01-26
**Scope:** Full audit of manifest → parser → pre-validator → generator → logging pipeline
**Status:** RESOLVED - 20+ fixes implemented in v4.40/v4.40.1/v4.40.2, 194/194 assets generate successfully

---

## v4.40.2 Fixes Implemented (2026-01-26)

| # | Issue | Fix | Files Modified |
|---|-------|-----|----------------|
| 13 | SpawnActor Class pin not validated | Added ERROR for unconnected Class pin | `GasAbilityGeneratorGenerators.cpp:9874-9884` |
| 14 | Delay Duration pin not validated | Added check for unconnected Duration pin | `GasAbilityGeneratorGenerators.cpp:9887-9902` |
| 15 | GetArrayItem Array+Index not validated | Added ERROR for unconnected Array/Index pins | `GasAbilityGeneratorGenerators.cpp:9905-9925` |
| 16 | MakeArray Element pins not validated | Added WARNING if no element pins connected | `GasAbilityGeneratorGenerators.cpp:9927-9955` |
| 17 | Sequence Exec input not validated | Added ERROR for unconnected Exec pin | `GasAbilityGeneratorGenerators.cpp:9958-9971` |
| 18 | Asset registry not notified after SavePackage | Added `FAssetRegistryModule::AssetCreated()` | `GasAbilityGeneratorGenerators.cpp:1945` |
| 19 | N2: Connection validation missing | Added `ValidateConnections()` pre-validation | `GasAbilityGeneratorPreValidator.cpp/.h` |
| 20 | Signature auto-fix silent | Added WARNING log when delegate params auto-added | `GasAbilityGeneratorGenerators.cpp:14389-14393` |

**Generation Result:** 194/194 assets generated successfully (0 failures)

---

## v4.40.1 Fixes Implemented (2026-01-26)

| # | Issue | Fix | Files Modified |
|---|-------|-----|----------------|
| 9 | Object VariableSet flagged as error | Allow object-type variables to be cleared (nullptr is valid) | `GasAbilityGeneratorGenerators.cpp:9810-9830` |
| 10 | In-manifest Blueprint pre-validation fails | Defer validation for Blueprints defined in same manifest | `GasAbilityGeneratorPreValidator.cpp:579-618` |
| 11 | Cross-Blueprint function resolution | Added `ResolveViaBlueprintAsset()` for cross-Blueprint calls | `GasAbilityGeneratorFunctionResolver.cpp/.h` |
| 12 | Cross-Blueprint function call handling | Use `SetExternalMember()` for functions on other Blueprints | `GasAbilityGeneratorGenerators.cpp:11016-11050` |

**Generation Result:** 194/194 assets generated successfully (0 failures)

---

## v4.40 Fixes Implemented (2026-01-26)

| # | Issue | Fix | Files Modified |
|---|-------|-----|----------------|
| 1 | Stale log read | Delete logs before commandlet run | `claude_automation.ps1` |
| 2 | VariableSet/Branch not validated | Added `ValuePinErrors` counter with explicit validation | `GasAbilityGeneratorGenerators.cpp:9488-9660` |
| 3 | Metadata registry race condition | Added `FCriticalSection CacheAccessLock` with `FScopeLock` | `GasAbilityGeneratorMetadata.h/.cpp` |
| 4 | Delegate bindings unvalidated | Added `ValidateDelegateBindings()` with known delegate/source checks | `GasAbilityGeneratorGenerators.cpp:9147-9290` |
| 5 | Generation order already correct | Verified AC_ → NPC_ → DBP_ order in manifest | N/A |
| 6 | No MakeLiteral* node types | Added `CreateMakeLiteralBoolNode/Float/Int/String` | `GasAbilityGeneratorGenerators.cpp:11659-11826` |
| 7 | SavePackage return unchecked | Added `SafeSavePackage()` helper, updated critical saves | `GasAbilityGeneratorGenerators.cpp:1922-1943` |
| 8 | Log not flushed before write | Added `GLog->Flush()` before file save | `GasAbilityGeneratorCommandlet.cpp` |

**Remaining Gaps:** ~23 non-critical gaps remain (see below for details) - none block generation

---

## Executive Summary

This audit identified **47 distinct gaps** across the GasAbilityGenerator pipeline, organized into 4 severity levels:

| Severity | Count | Fixed | Remaining | Description |
|----------|-------|-------|-----------|-------------|
| **CRITICAL** | 6 | 6 | 0 | Silent data loss, corruption, or security issues |
| **HIGH** | 14 | 12 | 2 | Build failures not caught, runtime errors, wrong behavior |
| **MEDIUM** | 18 | 6 | 12 | Validation gaps catchable by dryrun but not pre-validation |
| **LOW** | 9 | 0 | 9 | Edge cases, cleanup issues, documentation gaps |

**v4.40.2 Update:** 20+ fixes implemented. All generation-blocking issues resolved. 194/194 assets generate successfully.
**Remaining:** 23 gaps (2 HIGH, 12 MEDIUM, 9 LOW) - mostly edge cases and pre-validation expansions for non-blocking scenarios.

---

## Part 1: Node Creation Validation Gaps

### Pattern: "If VariableSet has no value pin validation, what else is missing?"

| Node Type | Required Input | Validated? | Failure Mode | Severity |
|-----------|----------------|------------|--------------|----------|
| **VariableSet** | Value pin | **YES (v4.40)** | Now reports `ValuePinErrors++` | ~~**CRITICAL**~~ FIXED |
| **PropertySet** | Value pin | **YES (v4.40)** | Now included in ValuePin check | ~~**CRITICAL**~~ FIXED |
| **Branch** | Condition pin | **YES (v4.40)** | Now reports `ValuePinErrors++` | ~~**HIGH**~~ FIXED |
| **DynamicCast** | Object pin | **YES (v4.40)** | Now reports `ValuePinErrors++` | ~~**MEDIUM**~~ FIXED |
| **ForEachLoop** | Array pin | ERROR detected | Correctly caught | OK |
| **SpawnActor** | Class pin | **YES (v4.40.2)** | Now reports `ValuePinErrors++` | ~~**MEDIUM**~~ FIXED |
| **Sequence** | Exec input | **YES (v4.40.2)** | Now reports `ValuePinErrors++` | ~~**HIGH**~~ FIXED |
| **Delay** | Duration pin | **YES (v4.40.2)** | Now logs warning for unconnected | ~~**MEDIUM**~~ FIXED |
| **GetArrayItem** | Array + Index | **YES (v4.40.2)** | Now reports `ValuePinErrors++` | ~~**HIGH**~~ FIXED |
| **MakeArray** | Element pins | **YES (v4.40.2)** | Now logs warning if no elements | ~~**MEDIUM**~~ FIXED |

### Root Cause Analysis

**Location:** `GasAbilityGeneratorGenerators.cpp` lines 9467-9620 (Post-generation validation)

The post-validation filtering logic only checks:
- Exec pins (SKIPPED - v2.5.2 pure node bypass)
- Self pins (SKIPPED)
- Pins with default values (SKIPPED)
- **Only "object" or "struct" category pins are warned about**

**Gap:** Float, Int, Bool, Name, Text pin categories slip through without validation.

```cpp
// Lines 9583-9591: Current filtering
if (PinCategory == TEXT("object") || PinCategory == TEXT("struct") ||
    PinNameStr.Contains(TEXT("Parent")) || PinNameStr.Contains(TEXT("Socket")) ||
    PinNameStr.Contains(TEXT("Object")))
{
    // WARN: unconnected object/struct
}
// MISSING: No check for "real", "int", "bool", "name", "text" categories
```

### Affected Manifest Patterns

Any manifest with these patterns can create invalid blueprints:

```yaml
# DANGEROUS: VariableSet without value connection
nodes:
  - id: SetHealth
    type: VariableSet
    properties:
      variable_name: CurrentHealth
connections:
  - from: [Event_BeginPlay, Then]
    to: [SetHealth, Exec]
  # MISSING: Connection to SetHealth.Value pin
  # Result: CurrentHealth set to 0.0 instead of intended value
```

```yaml
# DANGEROUS: Branch without condition
nodes:
  - id: CheckAlive
    type: Branch
connections:
  - from: [Event_Tick, Then]
    to: [CheckAlive, Exec]
  # MISSING: Connection to CheckAlive.Condition pin
  # Result: Blueprint compile error at runtime, not caught by generator
```

---

## Part 2: Pre-Validation Coverage Gaps

### Pattern: "If delegate_bindings have no pre-validation, what else is missing?"

**Current Coverage:** ~20% of manifest schema is pre-validated

| Feature | Pre-Validated | Gap Details |
|---------|---------------|-------------|
| **Delegate Bindings** | **NO** | Zero validation - delegate existence, handler signature, source variable |
| **Event Graph Nodes** | PARTIAL | Only CallFunction class/function checked; Branch, Delay, etc. NOT checked |
| **Event Graph Connections** | **NO** | Pin existence, type compatibility NOT checked |
| **Widget Tree** | **NO** | Widget types, properties, slot settings, children hierarchy |
| **Dialogue Tree** | **NO** | Node references, cyclic dependencies, speaker NPCs, events |
| **Quest States/Branches** | **NO** | State references, reachability, task classes, rewards |
| **Behavior Tree Nodes** | **NO** | Task/service classes, decorators, blackboard keys |
| **Material Expressions** | **NO** | Expression types, pin names, internal connections |
| **Niagara Parameters** | **NO** | Parameter names, types, value ranges, emitter existence |
| **NPC/Character Refs** | **NO** | AC_*, DBP_*, IC_*, Schedule_* asset references |

### Validation Rule Coverage

| Rule ID | Category | Status | Notes |
|---------|----------|--------|-------|
| C1 | Class path resolution | COVERED | 11 module search patterns |
| C2 | Parent class existence | COVERED | |
| F1 | Function on parent/library | COVERED | FunctionResolver parity (v4.31) |
| F2 | Function on explicit class | COVERED | CallFunction nodes only |
| A1 | AttributeSet class | COVERED | |
| A2 | Attribute on AttributeSet | COVERED | |
| R1-R3 | Asset references | COVERED | AssetRegistry only |
| T1 | Normal tags | COVERED | WARNING only |
| T2 | SetByCaller tags | COVERED | ERROR |
| K1-K2 | Tokens | **MISSING** | Placeholder, not implemented |
| D1 | Delegate existence | COVERED | ValidateDelegateBindings (v4.40) |
| D2 | Handler signature match | PARTIAL | Auto-fix with WARNING (v4.40.2) |
| N1 | Node required inputs | **MISSING** | Not pre-validated (generation-time only) |
| N2 | Connection node/pin existence | **COVERED (v4.40.2)** | ValidateConnections checks node IDs and pin names |

---

## Part 3: Delegate Binding Gaps

### Current Implementation Issues

**Location:** `GasAbilityGeneratorGenerators.cpp` lines 10480-10623 (mid-graph binding)
**Location:** `GasAbilityGeneratorGenerators.cpp` lines 13655-14600 (GA delegate binding)

| Issue | Severity | Details |
|-------|----------|---------|
| No pre-validation | ~~**HIGH**~~ FIXED | ValidateDelegateBindings checks delegate/source (v4.40) |
| Handler event order dependency | **HIGH** | CustomEvent must exist in graph BEFORE binding node |
| Signature auto-fix masks errors | ~~**MEDIUM**~~ FIXED | Now logs WARNING when parameters auto-added (v4.40.2) |
| Source variable not validated | ~~**MEDIUM**~~ FIXED | ValidateDelegateBindings checks source exists (v4.40) |
| ASC function assumed | **MEDIUM** | `GetAbilitySystemComponentFromActorInfo` not null-checked |

### OnGoalFailed vs OnGoalRemoved Issue

**From Session Context:** The manifest incorrectly uses `OnGoalSucceeded` only, leaving failure cases unhandled.

**Correct Pattern from Narrative Pro:**
- `OnGoalSucceeded` - Called when goal completes successfully
- `OnGoalRemoved` - Called when goal is removed for ANY reason (success, failure, timeout, cleanup)

**Impact:** NPCs using only `OnGoalSucceeded` binding will get stuck if goal fails.

**Fix Required:**
```yaml
# WRONG (current):
delegate_bindings:
  - delegate: OnGoalSucceeded
    handler: HandleGoalComplete

# CORRECT:
delegate_bindings:
  - delegate: OnGoalRemoved  # Fires for ALL termination scenarios
    handler: HandleGoalComplete
```

---

## Part 4: Logging and File Operation Gaps

### Logging Issues

| Issue | Severity | Location | Details |
|-------|----------|----------|---------|
| Stale output log | ~~**CRITICAL**~~ FIXED | `claude_automation.ps1` | Now deletes log before run (v4.40) |
| No flush before file write | ~~**HIGH**~~ FIXED | `GasAbilityGeneratorCommandlet.cpp:734` | Added `GLog->Flush()` (v4.40) |
| Race condition on UE log | **HIGH** | `claude_automation.ps1:298-305` | Fallback log read while commandlet may still be writing |
| Three unsynchronized logs | **MEDIUM** | Tools/Logs/ | output.log, stdout.log, full.log have different content |
| No timestamps in output log | ~~**MEDIUM**~~ FIXED | `GasAbilityGeneratorCommandlet.cpp:737` | Added timestamp header (v4.40) |

### File Operation Race Conditions

| Issue | Severity | Location | Details |
|-------|----------|----------|---------|
| Metadata registry not thread-safe | ~~**CRITICAL**~~ FIXED | `GasAbilityGeneratorMetadata.cpp` | Added FCriticalSection (v4.40) |
| SavePackage no verify | ~~**HIGH**~~ FIXED | `SafeSavePackage()` helper | Now checks return value (v4.40) |
| XLSX write without lock | **MEDIUM** | `NPCXLSXWriter.cpp:81-91` | File can corrupt if opened in Excel |
| Registry save deferred | **MEDIUM** | `GasAbilityGeneratorCommandlet.cpp:1802` | Save at end, failure silent |
| Asset apply not transactional | **MEDIUM** | `NPCXLSXSyncEngine.cpp:648-831` | Partial application without rollback |
| Asset registry not notified | ~~**MEDIUM**~~ FIXED | `SafeSavePackage()` | Added FAssetRegistryModule::AssetCreated (v4.40.2) |

### Evidence of Stale Log Issue

From Tools/Logs/ directory:
```
commandlet_output.log     11K   Jan 26 00:39  <- STALE (12+ hours old)
commandlet_stdout.log    171K   Jan 26 12:13  <- Fresh (latest run)
```

The timestamp mismatch proves `commandlet_output.log` wasn't regenerated on recent runs.

---

## Part 5: Generation Order Dependencies

### Current Issue: NPCDefinitions Before AC_*

**Problem:** NPCDefinitions reference `ability_configuration` and `activity_configuration` (AC_* assets), but these are generated AFTER NPCDefinitions in the commandlet.

**Result:** NPCDefinitions are deferred, then dialogue speakers can't find them.

**Evidence from logs:**
```
[DEFER] NPC_FatherCompanion (waiting for AC_FatherCompanion)
[DEFER] NPC_ReturnedStalker (waiting for AC_ReturnedStalker)
...
[NEW] DBP_Returned_StartFollow  <- Dialogue generated before NPC resolved
E_SPEAKER_NPCDEFINITION_NOT_FOUND: NPC_ReturnedStalker
```

**Correct Order:**
1. Tags
2. Enumerations
3. **AbilityConfigurations (AC_*)**
4. **ActivityConfigurations (AC_*)**
5. **NPCDefinitions (NPC_*)**
6. DialogueBlueprints (DBP_*)

---

## Part 6: Hardcoded Values and Dead Code

### VariableSet Without Value Input Issue

**From Session Context:** The manifest attempts to set `bCanHeal = true` using a VariableSet node without connecting a value source.

**Analysis of Options:**

| Option | Viability | Issue |
|--------|-----------|-------|
| Connect Exec only, no Value | **BROKEN** | VariableSet executes with unconnected pin = 0/false |
| Branch with hardcoded True | **BROKEN** | Branch also needs Condition input |
| Create boolean literal node | **MISSING** | Generator has no `MakeLiteralBool` node type |

**Current Gap:** No way to create a literal boolean/float/int value in event graphs from manifest.

**Required Fix:** Add `MakeLiteralBool`, `MakeLiteralFloat`, `MakeLiteralInt` node types to the generator.

```yaml
# NEEDED but not supported:
nodes:
  - id: TrueValue
    type: MakeLiteralBool
    properties:
      value: "true"
  - id: SetCanHeal
    type: VariableSet
    properties:
      variable_name: bCanHeal
connections:
  - from: [TrueValue, ReturnValue]
    to: [SetCanHeal, Value]  # Now has proper value source
```

---

## Part 7: Recommended Fixes by Priority

### CRITICAL (Fix Immediately) - ALL FIXED

1. ~~**Delete stale logs before generation**~~ ✅ FIXED v4.40
2. ~~**Add mutex to metadata registry**~~ ✅ FIXED v4.40
3. ~~**Validate VariableSet/PropertySet Value pins**~~ ✅ FIXED v4.40

### HIGH (Fix This Sprint) - MOSTLY FIXED

4. ~~**Pre-validate delegate bindings**~~ ✅ FIXED v4.40
5. ~~**Validate Branch Condition pin**~~ ✅ FIXED v4.40
6. ~~**Check SavePackage return values**~~ ✅ FIXED v4.40 (SafeSavePackage)
7. ~~**Fix generation order**~~ ✅ Already correct (verified v4.40)
8. **Handler event order dependency** - Still open (edge case)

### MEDIUM (Fix Next Sprint) - PARTIALLY FIXED

9. ~~**Add MakeLiteralBool/Float/Int node types**~~ ✅ FIXED v4.40
10. ~~**Pre-validate event graph connections**~~ ✅ FIXED v4.40.2 (ValidateConnections)
11. ~~**Add file flush after log write**~~ ✅ FIXED v4.40
12. **Make XLSX ApplyToAssets transactional** - Still open
13. ~~**Notify asset registry after SavePackage**~~ ✅ FIXED v4.40.2

### REMAINING (Lower Priority)

- XLSX write without lock
- Registry save deferred
- Race condition on UE log fallback
- Three unsynchronized logs
- ASC function null-check in generated Blueprints

---

## Part 8: Testing Checklist

After implementing fixes, verify:

- [x] `commandlet_output.log` timestamp updates on each run ✅ v4.40
- [x] VariableSet without Value connection logs ERROR (not silent) ✅ v4.40
- [x] Branch without Condition connection logs ERROR ✅ v4.40
- [x] Delegate binding to non-existent delegate logs ERROR at pre-validation ✅ v4.40
- [x] NPCDefinitions generate AFTER AC_* configurations ✅ Verified
- [x] Dialogue speakers find NPC definitions ✅ 194/194 success
- [x] Metadata registry survives concurrent access ✅ v4.40 (FCriticalSection)
- [x] SavePackage failures increment error count ✅ v4.40 (SafeSavePackage)
- [x] SpawnActor/DynamicCast/Sequence/etc. validation ✅ v4.40/v4.40.2
- [x] Asset registry notified after save ✅ v4.40.2
- [x] Connection node IDs validated at pre-validation ✅ v4.40.2

**Final Result:** 194/194 assets generate successfully with 0 errors, 0 warnings in pre-validation.

---

## Appendix: Affected File Locations

| File | Lines | Issues |
|------|-------|--------|
| `GasAbilityGeneratorGenerators.cpp` | 9467-9620, 10480-10623, 11074-11152, 13655-14600 | Node validation, delegate binding |
| `GasAbilityGeneratorPreValidator.cpp` | All | Missing delegate, node, connection validation |
| `GasAbilityGeneratorCommandlet.cpp` | 731, 1802 | Log save, registry save |
| `GasAbilityGeneratorMetadata.cpp` | 17, 121-128, 214-238 | Thread-unsafe cache |
| `claude_automation.ps1` | 189-207, 261-279, 298-305 | Stale log, race condition |
| `NPCXLSXSyncEngine.cpp` | 648-831 | Non-transactional apply |
| All XLSX writers | 59-91 | No file lock check |

---

**Document Version:** 1.1
**Author:** Claude Code Audit
**Last Updated:** 2026-01-26 (v4.40.2 fixes documented)
**Status:** 24 of 47 gaps fixed (51%), all generation-blocking issues resolved
