# GA_Backstab & GA_ProtectiveDome Error Audit

**Date:** 2026-01-24 (Original) | 2026-01-25 (Audit + v4.31 Implementation Attempt)
**Status:** BLOCKED - Need detailed logs to verify v4.31 fix effectiveness
**Context:** Debug analysis of event graph generation failures

---

## Executive Summary

**ORIGINAL CLAIM:** Two abilities (GA_ProtectiveDome, GA_Backstab) fail event graph generation due to manifest syntax issues.

**AUDIT RESULT (2026-01-25 00:58):** Fresh generation confirmed Issue 5 (custom function resolution) as the root cause.

**v4.31 IMPLEMENTATION (2026-01-25 ~01:30):** Code changes made to address Issue 5, but **generation still fails**. Detailed logs unavailable due to commandlet output filtering.

| Issue | Original Claim | Source Evidence | Current Status |
|-------|----------------|-----------------|----------------|
| 1. `parameters:` block | NOT supported | ✅ Parser.cpp:4611-4628 | **WORKS** - Source proves conversion |
| 2. Pin `Spec` | Not found | ⏳ Needs fresh logs | PENDING verification |
| 3. Pin `Tag` | Not found | ⏳ Needs fresh logs | PENDING verification |
| 4. Typed CustomEvent | NOT supported | ⚠️ Split case | Delegate: WORKS, Standalone: OPTIONAL |
| 5. Custom function call | Resolution fails | ❌ v4.31 fix applied | **STILL FAILING** - unknown reason |
| 6. Cascade | Depends on #5 | ❌ Depends on #5 | DEPENDENT |

---

## v4.31 Code Changes (2026-01-25)

### Change 1: FunctionResolver - Blueprint FunctionGraph Resolution

**File:** `GasAbilityGeneratorFunctionResolver.h/cpp`

Added Step 5 to resolution cascade:
```cpp
// New method in header (line ~113)
static FResolvedFunction ResolveViaBlueprintFunctionGraph(const FString& FunctionName, class UBlueprint* Blueprint);

// Implementation searches Blueprint->FunctionGraphs
for (UEdGraph* Graph : Blueprint->FunctionGraphs)
{
    if (Graph->GetFName() == FName(*FunctionName))
    {
        // Return with bFound=true, Function may be nullptr if not yet compiled
        // Generator uses SetSelfMember() for this case
    }
}
```

### Change 2: Generation Order - Custom Functions BEFORE Event Graph

**File:** `GasAbilityGeneratorGenerators.cpp` (lines ~2761-2795)

**Before v4.31:**
1. Generate event_graph (calls CheckBackstabCondition)
2. Generate custom_functions (creates CheckBackstabCondition)

**After v4.31:**
1. Generate custom_functions (creates CheckBackstabCondition FunctionGraph)
2. Generate event_graph (should now find CheckBackstabCondition)

### Change 3: CallFunction Node - Self Member Reference

**File:** `GasAbilityGeneratorGenerators.cpp` (lines ~10376-10430)

When `Resolved.bFound == true` but `Resolved.Function == nullptr` (custom BP function):
```cpp
else if (Resolved.bFound && !Function && Blueprint)
{
    CallNode->FunctionReference.SetSelfMember(FunctionName);
    // ... AllocateDefaultPins, etc.
}
```

---

## Current Problem: Logging Gap

**Issue:** Commandlet output (`commandlet_output.log`) only shows filtered summary:
```
[NEW] GA_FatherCrawler
[FAIL] GA_Backstab
ERROR:   Error: Event graph generation failed
```

**Missing:** Detailed `LogGeneration()` calls that would show:
- `CustomFunctions.Num()` for GA_Backstab (is it 0 or >0?)
- Whether FunctionGraph was created
- Actual failure point in event graph generation

**Root Cause:** `LogGeneration()` uses `UE_LOG(LogGasAbilityGenerator, Display, ...)` which is not captured in headless commandlet mode's filtered output.

---

## Source Evidence (Verified)

### D1: `parameters:` Block - ✅ CONFIRMED WORKING

**Source:** `GasAbilityGeneratorParser.cpp` lines 4611-4628

```cpp
// Line 4611-4614: Detects parameters: section
if (Key == TEXT("parameters") && Value.IsEmpty())
{
    bInParameters = true;
    // Don't add "parameters" as a property - we'll add individual params with "param." prefix
}
// Line 4616-4628: Converts nested keys to param.* prefix
else if (bInParameters && CurrentIndent > PropertiesIndent + 2)
{
    CurrentNode.Properties.Add(TEXT("param.") + Key, Value);
}
```

**Verdict:** ✅ NO FIX NEEDED - Parser already converts `parameters:` blocks to `param.*` prefix.

---

## Pending Verification (Need Fresh Detailed Logs)

| Decision | Topic | Status | What's Needed |
|----------|-------|--------|---------------|
| D2 | Pin `Spec` | PENDING | Fresh logs showing connection success/failure |
| D3 | Pin `Tag` | PENDING | Fresh logs showing connection success/failure |
| D5 | Custom function call | **v4.31 APPLIED** | Verify if CustomFunctions.Num() > 0 for GA_Backstab |
| D7 | GA_ProtectiveDome | PENDING | Identify which 1 of 61 connections fails |

---

## Hypotheses for Continued Failure

### Hypothesis A: Parser Not Extracting custom_functions

**Theory:** The `custom_functions:` section in GA_Backstab manifest isn't being parsed, so `Definition.CustomFunctions.Num() == 0`.

**Evidence needed:** Log showing `CustomFunctions.Num()` value.

**Parser code location:** `GasAbilityGeneratorParser.cpp` lines 1473-1482 (inside `bInEventGraph` block).

### Hypothesis B: FunctionGraph Created But Not Found

**Theory:** FunctionGraph is created but resolver doesn't find it due to timing or naming mismatch.

**Evidence needed:** Log showing `Blueprint->FunctionGraphs.Num()` after custom function generation.

### Hypothesis C: Different Failure Point

**Theory:** The failure is elsewhere in the event graph (not CheckBackstabCondition).

**Evidence needed:** Full event graph generation log with node-by-node status.

---

## Fresh Generation Results (2026-01-25 ~01:30, POST v4.31)

### GA_Backstab - STILL FAILS
```
[FAIL] GA_Backstab
ERROR:   Error: Event graph generation failed
```
No detailed logs available to determine failure point.

### GA_ProtectiveDome - STILL FAILS
```
[FAIL] GA_ProtectiveDome
ERROR:   Error: Event graph generation failed
```
No detailed logs available to determine failure point.

---

## Recommended Next Steps

1. **Enable detailed logging** - Either:
   - Run in editor mode (not headless) to get full UE_LOG output
   - Modify commandlet to capture LogGasAbilityGenerator category
   - Add `printf`/`FPlatformMisc::LowLevelOutputDebugString` calls

2. **Verify parser is extracting custom_functions** - Check if `CustomFunctions.Num() > 0` for GA_Backstab

3. **Verify FunctionGraph is created** - Check `Blueprint->FunctionGraphs` after custom function generation

4. **Identify exact failure point** - Which node/connection fails in event graph generation

---

## Locked Decisions (Updated 2026-01-25)

| Decision ID | Topic | Status | Evidence |
|-------------|-------|--------|----------|
| D1 | `parameters:` block | ✅ **LOCKED - NO FIX** | Source code proves parser converts |
| D2 | Pin `Spec` | ⏳ PENDING | Needs fresh logs |
| D3 | Pin `Tag` | ⏳ PENDING | Needs fresh logs |
| D4a | Typed CustomEvent (delegate) | ✅ **LOCKED - NO FIX** | Delegate binder extracts signature |
| D4b | Typed CustomEvent (standalone) | ⏳ OPTIONAL | Enhancement if needed in future |
| D5 | Custom function call | ⚠️ **v4.31 APPLIED** | Code changed, still failing, need logs |
| D6 | Cascade | ⏳ DEPENDENT | Resolves when D5 works |
| D7 | GA_ProtectiveDome connection | ⏳ PENDING | Need to identify failing connection |

---

## Audit Participants

- **Claude (Opus 4.5):** Code implementation, source analysis
- **GPT:** Cross-validation, challenge assumptions
- **Erdem:** Approval authority

---

## Reference Files

| File | Purpose |
|------|---------|
| `GasAbilityGeneratorParser.cpp:4611-4628` | `parameters:` block conversion (D1 evidence) |
| `GasAbilityGeneratorParser.cpp:1473-1482` | `custom_functions:` parsing (D5 investigation) |
| `GasAbilityGeneratorFunctionResolver.cpp:348-396` | v4.31 BlueprintFunctionGraph resolution |
| `GasAbilityGeneratorGenerators.cpp:2761-2795` | v4.31 generation order change |
| `GasAbilityGeneratorGenerators.cpp:10376-10430` | v4.31 CallFunction self-member handling |

---

## Appendix: Original Log Evidence (2026-01-25 00:58, PRE v4.31)

### GA_Backstab Function Resolution Failure
```
[final_test.log:17947-17955]
Generating event graph 'GA_Backstab_EventGraph' in Blueprint GA_Backstab
...
Created node: Branch_ValidTarget (Branch)
Function 'CheckBackstabCondition' not found via shared resolver.
```

### GA_Backstab Summary
```
[final_test.log:18163-18166]
Event graph 'GA_Backstab_EventGraph' summary: Nodes 8/10, Connections 8/15
[FAIL] GenerateEventGraph failed for GA_Backstab_EventGraph: Nodes=2 Connections=7 PostValidation=0
```

### GA_ProtectiveDome Summary
```
[final_test.log:15680-15687]
Event graph 'GA_ProtectiveDome_EventGraph' summary: Nodes 47/47, Connections 60/61
VALIDATION: 3 potentially unconnected input pin(s) detected
[FAIL] GenerateEventGraph failed for GA_ProtectiveDome_EventGraph: Nodes=0 Connections=1 PostValidation=0
```
