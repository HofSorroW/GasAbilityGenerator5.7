# Graph Validation Audit v1.0

**Session Date:** 2026-01-21
**Participants:** Claude (Opus 4.5), GPT
**Objective:** Increase logging and pre-generation error detection to catch Blueprint compile errors before asset writing

---

## Problem Statement

**Current State:** Assets report `Status: New - Created successfully` but have Blueprint compilation errors in editor.

**Symptoms:**
- Pin type mismatches silently allowed
- Blueprint "looks generated" but doesn't run
- Compile errors only visible when opening in UE editor
- No abort mechanism when errors detected

**Desired State:**
1. Detect pin type / signature errors BEFORE writing assets
2. Abort generation on mismatch
3. Return detailed failure reason
4. Allow correction and retry
5. Only generate if validation passes

---

## Current Validation Architecture

### What Works (Layers 1-2)

| Layer | Validation | Status |
|-------|------------|--------|
| Manifest Parsing | YAML syntax, node IDs, required properties | Complete |
| Structural Checks | Node existence, connection refs, pin names | Complete |
| Type Compatibility | K2Schema `CanCreateConnection()` | Partial |
| Exec Pin Tracking | Max 1 output per exec pin | Complete |

### What's Missing (Layers 3-5)

| Gap ID | Issue | Impact |
|--------|-------|--------|
| **GAP-1** | Conversion node responses allow type mismatches | Connection made with incompatible types |
| **GAP-2** | No semantic validation of node configuration | Invalid function/class refs not caught |
| **GAP-3** | No post-compilation error detection | Only GE checks `GeneratedClass` |
| **GAP-4** | Conversion nodes not created when K2Schema suggests them | Type mismatches remain unfixed |
| **GAP-5** | No error recovery or cleanup | Assets saved even on compile failure |
| **GAP-6** | No circular connection detection | Infinite loops possible |

---

## Code References

### Current Validation Points

| Location | Lines | Function | What It Does |
|----------|-------|----------|--------------|
| `GasAbilityGeneratorGenerators.cpp` | 7997-8199 | `ValidateEventGraph()` | Structural pre-validation |
| `GasAbilityGeneratorGenerators.cpp` | 10633-10825 | `ConnectPins()` | Pin connection with type check |
| `GasAbilityGeneratorGenerators.cpp` | 10766-10802 | Type check block | K2Schema `CanCreateConnection()` |
| `GasAbilityGeneratorGenerators.cpp` | 2244-2251 | GE compilation check | Only generator with post-compile validation |

### Pin Connection Response Handling (Current)

```cpp
// Line 10770
FPinConnectionResponse Response = K2Schema->CanCreateConnection(FromPin, ToPin);

if (Response.Response == CONNECT_RESPONSE_DISALLOW)
{
    // Hard error - connection rejected
    return false;
}
else if (Response.Response == CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE)
{
    // WARNING ONLY - connection still made with incompatible types!
    LogGeneration(TEXT("Connection requires conversion..."));
}
// Connection proceeds regardless
```

---

## Proposed Solutions

### Track A: Pre-Connection Validation (Prevent Bad Connections)

**Severity:** HIGH
**Effort:** MEDIUM

Add validation layer BEFORE `MakeLinkTo()`:

1. **Strict Type Enforcement**
   - Treat `CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE` as error, not warning
   - Abort connection if types don't match exactly
   - Option: Auto-create conversion nodes where possible

2. **Function Reference Validation**
   - For `CallFunction` nodes: verify function exists on target class
   - For `DynamicCast` nodes: verify target class exists and is valid
   - For `VariableGet/Set`: verify variable exists in Blueprint

3. **Semantic Node Validation**
   - Validate all required pins have connections before compile
   - Detect orphaned data flows (output pins with no consumers)

### Track B: Post-Compilation Verification (Catch Remaining Errors)

**Severity:** CRITICAL
**Effort:** LOW

Add compilation check to ALL generators:

```cpp
// After CompileBlueprint()
FKismetEditorUtilities::CompileBlueprint(Blueprint);

// NEW: Check compilation result
if (Blueprint->Status == BS_Error)
{
    // Collect compile errors
    TArray<FString> CompileErrors;
    for (const FEdGraphCompilerMessage& Msg : Blueprint->Message_Log->Messages)
    {
        if (Msg.Severity == ECompilerMessageSeverity::Error)
        {
            CompileErrors.Add(Msg.Message);
        }
    }

    // Abort and return failure
    return FGenerationResult(
        AssetName,
        EGenerationStatus::Failed,
        FString::Printf(TEXT("Blueprint compilation failed: %s"), *FString::Join(CompileErrors, TEXT("; "))),
        Category,
        AssetPath
    );
}
```

### Track C: Dry-Run Compilation (Validate Before Save)

**Severity:** HIGH
**Effort:** HIGH

1. Create Blueprint in memory
2. Generate all nodes and connections
3. Compile WITHOUT saving
4. If compile succeeds → save package
5. If compile fails → discard, return errors

### Track D: Connection Graph Analysis (Static Analysis)

**Severity:** MEDIUM
**Effort:** MEDIUM

1. **Cycle Detection:** DFS/BFS to detect circular exec flows
2. **Data Flow Analysis:** Ensure all required inputs have sources
3. **Dead Code Detection:** Warn on unreachable nodes
4. **Type Flow Analysis:** Trace types through the graph

---

## Decision Log

| ID | Decision | Rationale | Status |
|----|----------|-----------|--------|
| D-001 | `MAKE_WITH_CONVERSION_NODE` = hard fail | UE5 KismetCompiler.cpp:848-854 treats this as compile error; generator should fail early | **LOCKED** |
| D-002 | All 4 post-validation patterns → diagnostic-only | ASC Target/Actor Pin are context-dependent; Array Input empty not universally valid; Multiple Exec doesn't exist in manifest | **LOCKED** |
| D-003 | Fail-fast per asset, continue to next | Dependency ordering via PHASES + Deferred system handles cascading; P6b classifies failure types | **LOCKED** |
| D-004 | `FCompilerResultsLog` for ALL Blueprint generators | Erdem confirmed: no exceptions, all current and future generators | **LOCKED** |
| D-005 | Failures reflected in `UGenerationReport` system | Erdem confirmed: `EGenerationStatus::Failed` + error details | **LOCKED** |
| D-006 | FWidgetBlueprintGenerator needs `CompileBlueprint()` added | WBP generator (lines 3101-4050) has NO compile call - saves without compiling | **LOCKED** |
| D-007 | P6b uses existing tokens: `[DEFER]`, `[FAIL]`, `[UNRESOLVED]`, etc. | Align with existing code at lines 670, 675-677, 1377, 1392, 1407 | **LOCKED** |
| D-008 | ANY `CompileBlueprint()` with `NumErrors > 0` = fail immediately | Intermediate compile errors leave GeneratedClass stale; fail-fast prevents cascading failures | **LOCKED** |
| D-009 | No checkpoint saves - only save complete, validated Blueprints | Checkpoint saves recreate "looks generated but broken" failure mode | **LOCKED** |
| D-010 | FGameplayAbilityGenerator requires restructure | GA has checkpoint save at line 2671 before Category C nodes; must move to single save at end | **LOCKED** |
| D-011 | **INVARIANT:** Each Blueprint generator may call `SavePackage` at most once, and only after passing unified validation gates (P1/P3/P4) | Prevents future reintroduction of checkpoint save patterns | **LOCKED** |

---

## Agreed Policies (Claude-GPT Audit)

### P1: GenerateEventGraph Failure Conditions
`GenerateEventGraph()` returns `false` if:
- `NodesFailed > 0` (node creation failed)
- `ConnectionsFailed > 0` (connection failed)
- `TotalErrors > 0` (post-validation errors)

### P2: Caller Propagation
All callers of `GenerateEventGraph()` must:
- Check return value
- NOT save package on `false`
- Return `EGenerationStatus::Failed` with error details

### P3: Compile Verification
All Blueprint-generating generators must:
```cpp
FCompilerResultsLog CompileLog;
FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &CompileLog);

if (CompileLog.NumErrors > 0)
{
    // Collect error messages
    TArray<FString> ErrorMessages;
    for (const auto& Msg : CompileLog.Messages)
    {
        if (Msg->Severity == EMessageSeverity::Error)
        {
            ErrorMessages.Add(Msg->ToText().ToString());
        }
    }
    // Return failure - do NOT save
    return FGenerationResult(..., EGenerationStatus::Failed,
        FString::Printf(TEXT("Compile failed: %s"), *FString::Join(ErrorMessages, TEXT("; "))));
}
```

### P4: Save Gating
`UPackage::SavePackage()` only called if:
- `GenerateEventGraph()` returned `true` (if applicable)
- `CompileLog.NumErrors == 0`

### P5: Type Mismatch Hard Fail
`CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE` treated as hard fail with diagnostic:
```
Pin type mismatch: [{FromNode}].{FromPin} ({FromType}) → [{ToNode}].{ToPin} ({ToType})
Connection requires conversion node (UE response: MAKE_WITH_CONVERSION_NODE)
Generator does not auto-create conversions. Fix manifest or add explicit conversion node.
Aborting asset: {AssetName}
```

### P6: Asset Independence
- Fail-fast per asset, continue to next asset
- Dependency ordering via PHASES (Enums → Effects → Blueprints → Abilities)
- Deferred Asset System handles missing dependencies with retry

### P6b: Failure Classification
| Type | Meaning | Log Token |
|------|---------|-----------|
| Primary | Asset's own graph/compile issues | `[FAIL]` |
| Blocked | Dependency not found after retries | `[UNRESOLVED]` |

**Note:** "Blocked" is a conceptual label; the actual log token is `[UNRESOLVED]` (aligns with existing code at line 1407).

### P7: Post-Validation (Diagnostic Only)
All 4 patterns detected but NOT auto-fixed:

| Pattern | Detection | Suggested Fix (logged) |
|---------|-----------|------------------------|
| Multiple Exec | `Pin->LinkedTo.Num() > 1` on exec output | "Use Sequence node" |
| ASC Target | Unconnected Target pin expecting ASC | "Connect GetAbilitySystemComponent" |
| Actor Pin | Unconnected Actor pin (self is not Actor) | "Connect GetAvatarActor or GetOwningActor" |
| Array Input | Unconnected array-type input pin | "Connect MakeArray node" |

---

## Implementation Scope (Erdem Confirmed)

**Applies to:** ALL 32+ current generators AND all future generators

**Blueprint Generators requiring P3 (compile check):**
- FGameplayEffectGenerator (already has partial check)
- FGameplayAbilityGenerator
- FActorBlueprintGenerator
- FWidgetBlueprintGenerator
- FDialogueBlueprintGenerator
- FEquippableItemGenerator
- FActivityGenerator
- FNarrativeEventGenerator
- FGameplayCueGenerator
- FGoalItemGenerator
- FQuestGenerator
- FAnimationNotifyGenerator

**Report Integration:** All failures must populate `UGenerationReport` with:
- `EGenerationStatus::Failed`
- Error code(s)
- Context path
- Actionable message
- Suggested fix

---

## Implementation Priority Matrix

| Track | Impact | Effort | Priority | Recommendation |
|-------|--------|--------|----------|----------------|
| **B** | Catches ALL compile errors | LOW | **P0** | Do first - immediate win |
| **A** | Prevents type mismatches | MEDIUM | **P1** | Do second - stops common issues |
| **C** | Perfect validation | HIGH | **P2** | Future - gold standard |
| **D** | Static analysis | MEDIUM | **P3** | Nice-to-have |

---

## Audit Questions - RESOLVED

| # | Question | Answer | Policy |
|---|----------|--------|--------|
| 1 | Should compilation errors abort immediately or collect all errors first? | Collect all errors in `FCompilerResultsLog`, then abort | P3 |
| 2 | Should `MAKE_WITH_CONVERSION_NODE` hard fail, auto-create, or warn? | **(a) Hard fail** - no auto-create in v1 | P5 |
| 3 | When compilation fails, what happens to the asset? | **(a) Don't save** - asset not written to disk | P4 |
| 4 | If validation fails, abort run or continue? | **(b) Skip failed asset, continue others** | P6 |
| 5 | What error codes should we define? | See table below | P7 |

### Error Codes (P7 Post-Validation)

| Code | Pattern | Suggested Fix |
|------|---------|---------------|
| `E_MULTIPLE_EXEC` | Multiple connections from exec output | Use Sequence node |
| `E_ASC_TARGET_MISSING` | Unconnected ASC Target pin | Connect GetAbilitySystemComponent |
| `E_ACTOR_PIN_MISSING` | Unconnected Actor pin | Connect GetAvatarActor/GetOwningActor |
| `E_ARRAY_INPUT_MISSING` | Unconnected array input | Connect MakeArray node |
| `E_TYPE_MISMATCH` | MAKE_WITH_CONVERSION_NODE response | Fix manifest types or add conversion |
| `E_COMPILE_FAILED` | Blueprint compilation error | See compile log for details |
| `E_NODE_CREATION_FAILED` | Node couldn't be created | Check node type/properties in manifest |
| `E_CONNECTION_FAILED` | Connection couldn't be made | Check pin names and types |

---

## Session Notes

### Claude Analysis Summary

The exploration identified that the plugin has robust structural validation but lacks:
1. **Post-compilation checking** (only GE does this)
2. **Strict type enforcement** (conversion responses treated as warnings)
3. **Semantic validation** (function/class reference checking)
4. **Error recovery** (assets saved regardless of compile status)

**Recommended Priority:** Track B first (low effort, high impact), then Track A.

### GPT Contributions

1. **Validated gap analysis** - Confirmed GAP-1 through GAP-5; challenged GAP-6 (circular connections) as not source of current failures
2. **Phase model** - Proposed fail-fast at each phase (node creation → connection → post-validation → compile)
3. **Auto-fix scope reduction** - Challenged 4 auto-fix patterns; accepted diagnostic-only for all after evidence review
4. **P6b classification** - Proposed Primary vs Blocked failure distinction for cleaner reporting
5. **P5 diagnostic clarity** - Flagged need for actionable error message on conversion node failures

### Evidence Found During Audit

| Claim | Evidence Location | Claude | GPT |
|-------|-------------------|--------|-----|
| `MAKE_WITH_CONVERSION_NODE` = compile error | KismetCompiler.cpp:848-854 | ✅ | ⚠️ Supporting evidence only; D-001 is project policy regardless |
| Only 1/16 compile calls validate result | GasAbilityGeneratorGenerators.cpp | ✅ | Unverified (plugin code) |
| `GenerateEventGraph()` returns true on errors | Line 8753 | ✅ | Unverified (plugin code) |
| Empty array not universally valid | manifest.yaml:4060-4072 (ObjectTypes vs ActorsToIgnore) | ✅ | ✅ Logic accepted |
| Multiple Exec pattern doesn't exist | Manifest search: 0 instances | ✅ | ✅ Logic accepted |
| Dependency ordering exists | PHASE comments + Deferred system | ✅ | ✅ Logic accepted |
| FWidgetBlueprintGenerator has NO `CompileBlueprint()` | Lines 4029-4049: MarkDirty → Save, no compile | ✅ | Unverified (plugin code) |
| Status tokens: DEFER, FAIL, UNRESOLVED | Commandlet.cpp lines 670, 675-677, 1377, 1392, 1407 | ✅ | Unverified (plugin code) |
| GA checkpoint save pattern | Lines 2671 (first save), 2722 (second save conditional) | ✅ | Unverified (plugin code) |
| GA GenerateEventGraph failure = warning only | Lines 2626-2629: logs warning, continues execution | ✅ | Unverified (plugin code) |
| GA Category C added AFTER first save | Lines 2676-2710 (after 2671 save) | ✅ | Unverified (plugin code) |
| Other generators have single save | Grep: ActorBP, AnimNotify, DialogueBP, etc. all single save | ✅ | Unverified (plugin code) |
| v4.14 removed redundant recompiles elsewhere | Lines 14670-14671, 14908-14909 comments | ✅ | Unverified (plugin code) |

**Note:** "Unverified (plugin code)" means GPT could not independently verify line numbers in this chat session. Claude read the files locally. Policies remain valid regardless.

### Erdem Decisions

1. ✅ Policies apply to ALL 32+ generators, no exceptions
2. ✅ P3 (FCompilerResultsLog) applies to ALL Blueprint generators
3. ✅ Failures reflected in UGenerationReport system
4. ✅ All current AND future generators must comply

---

## Acceptance Criteria (GO Checkpoint)

Implementation is complete when ALL of the following are satisfied:

### AC-1: Blueprint Compile Gate
Any Blueprint generator that compiles with `NumErrors > 0`:
- ❌ Does NOT call `SavePackage()`
- ✅ Increments Failed count
- ✅ Emits actionable diagnostics into `UGenerationReport`

### AC-2: GenerateEventGraph Gate
Any generator using `GenerateEventGraph()`:
- If `NodesFailed > 0` OR `ConnectionsFailed > 0` OR `TotalErrors > 0`:
  - Returns `false`
  - Aborts save (P1/P2/P4)

### AC-3: GA Generator Restructure
FGameplayAbilityGenerator:
- ❌ No checkpoint save
- ✅ Single save at end only
- ✅ Compile validation gates save (D-010)

### AC-4: WBP Generator Fix
FWidgetBlueprintGenerator:
- ✅ Must call `CompileBlueprint()` before save
- ✅ Must validate compile result (D-006)

**Success metric:** If AC-1 through AC-4 are satisfied, the "generated-but-broken" class of failures should disappear.

---

## Cross-References

- **Contract Definition:** `ClaudeContext/Handoffs/LOCKED_CONTRACTS.md` - Contract 10 (Blueprint Compile Gate)
- **Implementation Mapping:** `ClaudeContext/Handoffs/LOCKED_SYSTEMS.md` - Contract 10
- **Implementation Guide:** `ClaudeContext/Handoffs/Graph_Validation_Implementation_v4.16.md`

---

## Version History

- v1.0 (2026-01-21): Initial audit document created from codebase exploration
- v1.1 (2026-01-21): Added agreed policies P1-P7, P6b; locked decisions D-001 through D-005; documented implementation scope
- v1.2 (2026-01-21): Extended audit - added D-006 (WBP missing compile), D-007 (status tokens), D-008 (any compile error = fail), D-009 (no checkpoint saves), D-010 (GA restructure), D-011 (single-save invariant); added 7 new evidence items
- v1.3 (2026-01-21): Final GPT review - updated Evidence table with verification status (Claude vs GPT); tightened P6b to use `[UNRESOLVED]` token; added Acceptance Criteria (AC-1 through AC-4); **AUDIT COMPLETE - READY FOR GO**
- v1.4 (2026-01-21): Added cross-references to LOCKED_CONTRACTS.md (Contract 10), LOCKED_SYSTEMS.md, and implementation guide
