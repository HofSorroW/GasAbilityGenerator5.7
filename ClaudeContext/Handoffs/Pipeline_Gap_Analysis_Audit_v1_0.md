# Pipeline Gap Analysis Audit v1.0

**Date:** 2026-01-26
**Scope:** Full audit of manifest → parser → pre-validator → generator → logging pipeline
**Status:** RESOLVED - 12 critical/high fixes implemented in v4.40/v4.40.1, 194/194 assets generate successfully

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

**Remaining Gaps:** ~35 gaps still need addressing (see below for details)

---

## Executive Summary

This audit identified **47 distinct gaps** across the GasAbilityGenerator pipeline, organized into 4 severity levels:

| Severity | Count | Fixed | Remaining | Description |
|----------|-------|-------|-----------|-------------|
| **CRITICAL** | 6 | 5 | 1 | Silent data loss, corruption, or security issues |
| **HIGH** | 14 | 7 | 7 | Build failures not caught, runtime errors, wrong behavior |
| **MEDIUM** | 18 | 0 | 18 | Validation gaps catchable by dryrun but not pre-validation |
| **LOW** | 9 | 0 | 9 | Edge cases, cleanup issues, documentation gaps |

**v4.40.1 Update:** All blocking generation errors resolved. 194/194 assets generate successfully.

---

## Part 1: Node Creation Validation Gaps

### Pattern: "If VariableSet has no value pin validation, what else is missing?"

| Node Type | Required Input | Validated? | Failure Mode | Severity |
|-----------|----------------|------------|--------------|----------|
| **VariableSet** | Value pin | **YES (v4.40)** | Now reports `ValuePinErrors++` | ~~**CRITICAL**~~ FIXED |
| **PropertySet** | Value pin | **YES (v4.40)** | Now included in ValuePin check | ~~**CRITICAL**~~ FIXED |
| **Branch** | Condition pin | **YES (v4.40)** | Now reports `ValuePinErrors++` | ~~**HIGH**~~ FIXED |
| **DynamicCast** | Object pin | WARN only | Runtime nullptr, WARN logged but proceeds | **MEDIUM** |
| **ForEachLoop** | Array pin | ERROR detected | Correctly caught | OK |
| **SpawnActor** | Class pin | WARN only | Runtime nullptr | **MEDIUM** |
| **Sequence** | Exec input | **NO** | Silent - sequence never fires | **HIGH** |
| **Delay** | Duration pin | **NO** | Uses default (0) - immediate execution | **MEDIUM** |
| **GetArrayItem** | Array + Index | **NO** | Runtime array bounds error | **HIGH** |
| **MakeArray** | Element pins | **NO** | Creates empty array silently | **MEDIUM** |

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
| F1 | Function on parent/library | **MISSING** | Not pre-validated |
| F2 | Function on explicit class | COVERED | CallFunction nodes only |
| A1 | AttributeSet class | COVERED | |
| A2 | Attribute on AttributeSet | COVERED | |
| R1-R3 | Asset references | COVERED | AssetRegistry only |
| T1 | Normal tags | COVERED | WARNING only |
| T2 | SetByCaller tags | COVERED | ERROR |
| K1-K2 | Tokens | **MISSING** | Placeholder, not implemented |
| D1 | Delegate existence | **MISSING** | Not implemented |
| D2 | Handler signature match | **MISSING** | Not implemented |
| N1 | Node required inputs | **MISSING** | Not implemented |
| N2 | Connection type compatibility | **MISSING** | Not implemented |

---

## Part 3: Delegate Binding Gaps

### Current Implementation Issues

**Location:** `GasAbilityGeneratorGenerators.cpp` lines 10480-10623 (mid-graph binding)
**Location:** `GasAbilityGeneratorGenerators.cpp` lines 13655-14600 (GA delegate binding)

| Issue | Severity | Details |
|-------|----------|---------|
| No pre-validation | **HIGH** | Delegate existence checked at generation time only |
| Handler event order dependency | **HIGH** | CustomEvent must exist in graph BEFORE binding node |
| Signature auto-fix masks errors | **MEDIUM** | Parameters added silently, no mismatch warning |
| Source variable not validated | **MEDIUM** | If source is unknown variable, silently skipped |
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
| Stale output log | **CRITICAL** | `claude_automation.ps1` | `commandlet_output.log` not deleted before run; old logs read as current |
| No flush before file write | **HIGH** | `GasAbilityGeneratorCommandlet.cpp:731` | LogMessages written without `GLog->Flush()` |
| Race condition on UE log | **HIGH** | `claude_automation.ps1:298-305` | Fallback log read while commandlet may still be writing |
| Three unsynchronized logs | **MEDIUM** | Tools/Logs/ | output.log, stdout.log, full.log have different content |
| No timestamps in output log | **MEDIUM** | All log messages | Cannot determine if logs are fresh |

### File Operation Race Conditions

| Issue | Severity | Location | Details |
|-------|----------|----------|---------|
| Metadata registry not thread-safe | **CRITICAL** | `GasAbilityGeneratorMetadata.cpp:17,121-128` | Static cache with no mutex |
| SavePackage no verify | **HIGH** | 70+ locations in Generators.cpp | Return value not checked |
| XLSX write without lock | **MEDIUM** | `NPCXLSXWriter.cpp:81-91` | File can corrupt if opened in Excel |
| Registry save deferred | **MEDIUM** | `GasAbilityGeneratorCommandlet.cpp:1802` | Save at end, failure silent |
| Asset apply not transactional | **MEDIUM** | `NPCXLSXSyncEngine.cpp:648-831` | Partial application without rollback |
| Asset registry not notified | **MEDIUM** | All SavePackage calls | Assets not visible until next scan |

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

### CRITICAL (Fix Immediately)

1. **Delete stale logs before generation**
   - Location: `claude_automation.ps1` cycle action
   - Fix: Add `Remove-Item $OutputLog -ErrorAction SilentlyContinue` before commandlet run

2. **Add mutex to metadata registry**
   - Location: `GasAbilityGeneratorMetadata.cpp`
   - Fix: Add `FCriticalSection` for all cache access

3. **Validate VariableSet/PropertySet Value pins**
   - Location: `GasAbilityGeneratorGenerators.cpp` post-validation
   - Fix: Add explicit check for these node types

### HIGH (Fix This Sprint)

4. **Pre-validate delegate bindings**
   - Location: `GasAbilityGeneratorPreValidator.cpp`
   - Fix: Add `ValidateDelegates()` pass checking delegate existence on ASC class

5. **Validate Branch Condition pin**
   - Location: Post-validation loop
   - Fix: Add `UK2Node_IfThenElse` check for unconnected Condition

6. **Check SavePackage return values**
   - Location: All 70+ SavePackage calls
   - Fix: Add `if (!UPackage::SavePackage(...)) { OutErrors.Add(...); }`

7. **Fix generation order**
   - Location: `GasAbilityGeneratorCommandlet.cpp`
   - Fix: Move AC_* before NPC_*, verify NPC_* before DBP_*

### MEDIUM (Fix Next Sprint)

8. **Add MakeLiteralBool/Float/Int node types**
9. **Pre-validate event graph connections**
10. **Add file flush after log write**
11. **Make XLSX ApplyToAssets transactional**
12. **Notify asset registry after SavePackage**

---

## Part 8: Testing Checklist

After implementing fixes, verify:

- [ ] `commandlet_output.log` timestamp updates on each run
- [ ] VariableSet without Value connection logs ERROR (not silent)
- [ ] Branch without Condition connection logs ERROR
- [ ] Delegate binding to non-existent delegate logs ERROR at pre-validation
- [ ] NPCDefinitions generate AFTER AC_* configurations
- [ ] Dialogue speakers find NPC definitions
- [ ] Metadata registry survives concurrent access
- [ ] SavePackage failures increment error count

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

**Document Version:** 1.0
**Author:** Claude Code Audit
**Next Review:** After fixes implemented
