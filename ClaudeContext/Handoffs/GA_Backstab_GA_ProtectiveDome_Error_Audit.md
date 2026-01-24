# GA_Backstab & GA_ProtectiveDome Error Audit

**Date:** 2026-01-24 (Original) | 2026-01-25 (Audit + Fresh Generation)
**Status:** GENERATOR FIX REQUIRED - Issue 5 Confirmed
**Context:** Debug analysis of event graph generation failures

---

## Executive Summary

**ORIGINAL CLAIM:** Two abilities (GA_ProtectiveDome, GA_Backstab) fail event graph generation due to manifest syntax issues.

**AUDIT RESULT:** Fresh generation run (2026-01-25 00:58) confirms:
- **4 of 6 original claims were INCORRECT** - generator already handles them
- **Issue 5 (custom function call) is CONFIRMED** - generator cannot resolve same-Blueprint functions
- **Issue 6 (cascade) is CONFIRMED** - direct consequence of Issue 5

| Issue | Original Claim | Audit Verdict | Fresh Gen Evidence |
|-------|----------------|---------------|-------------------|
| 1. `parameters:` block | NOT supported | ✅ **WORKS** | Parser converts automatically |
| 2. Pin `Spec` | Not found | ✅ **WORKS** | Partial matching succeeds |
| 3. Pin `Tag` | Not found | ✅ **WORKS** | 22+ successful uses |
| 4. Typed CustomEvent | NOT supported | ⚠️ **PARTIAL** | Delegate-bound: WORKS |
| 5. Custom function call | Resolution fails | ❌ **CONFIRMED** | `CheckBackstabCondition` not found |
| 6. Cascade | Depends on #5 | ❌ **CONFIRMED** | `ApplyGEToSelf` node not created |

**ACTION REQUIRED:** Generator enhancement needed - add same-Blueprint FunctionGraph resolution to `FGasAbilityGeneratorFunctionResolver`.

---

## Validated Findings

### Issue 1: `parameters:` Block - ✅ WORKS

**Original Claim:** Nested `parameters:` blocks are unsupported; must use `param.` prefix.

**Validation:** Parser v2.8.2 (GasAbilityGeneratorParser.cpp lines 4611-4628) **ALREADY** converts `parameters:` blocks to `param.*` prefix:

```cpp
// Line 4611-4614
if (Key == TEXT("parameters") && Value.IsEmpty())
{
    bInParameters = true;
    // Don't add "parameters" as a property - we'll add individual params with "param." prefix
}
// Line 4616-4628
else if (bInParameters && CurrentIndent > PropertiesIndent + 2)
{
    CurrentNode.Properties.Add(TEXT("param.") + Key, Value);
}
```

**Verdict:** No manifest or generator fix needed. Both syntaxes work.

---

### Issue 2: Pin `Spec` vs `SpecHandle` - ✅ WORKS

**Original Claim:** `FindPinByName` has no alias for `Spec`; should be `SpecHandle`.

**Validation:** Logs prove partial matching succeeds:
```
Connecting: MakeGESpec.ReturnValue [struct] -> ApplyGE.SpecHandle [struct&]
Connected: MakeGESpec.ReturnValue -> ApplyGE.Spec
```

The `E_PIN_POSITION_NOT_FOUND` error for `Spec` is a **LAYOUT WARNING ONLY** (from `GetPinYOffset`), not a connection failure. Per fail_fast_report.txt: "E_PIN_POSITION_NOT_FOUND is a LAYOUT HELPER warning only. Actual connections still work."

**Verdict:** No manifest or generator fix needed. Partial matching handles this.

---

### Issue 3: Pin `Tag` vs `InTag` - ✅ WORKS

**Original Claim:** `MakeGameplayTagContainerFromTag` uses `InTag` but manifest uses `Tag`.

**Validation:** 22+ occurrences of `Tag` pin in manifest for `MakeGameplayTagContainerFromTag`:
- GA_FatherCrawler (lines 1233, 1239, 1251, 1294)
- GA_FatherArmor (lines 1779, 1787, 1793)
- GA_FatherExoskeleton (lines 2379, 2391, 2397)
- GA_FatherSymbiote (lines 3095, 3103, 3109)
- GA_FatherEngineer (lines 3670, 3704, 3710)
- GA_FatherSacrifice (lines 6451, 6491, 6525)

These abilities generate successfully, proving partial matching works for `Tag` → `InTag`.

**Verdict:** No manifest or generator fix needed.

---

### Issue 4: Typed CustomEvent Parameters - ⚠️ PARTIAL

**Original Claim:** Generator creates zero-param CustomEvents only.

**Validation:** Two cases exist:

#### Case A: Delegate-Bound CustomEvents - ✅ WORKS
Log evidence:
```
Created handler event: HandleDomeDamageAbsorption with signature from OnDamagedBy__DelegateSignature
Created delegate bind/unbind nodes for OwnerASC.OnDamagedBy -> HandleDomeDamageAbsorption
```

The delegate binding system extracts typed parameters from the delegate signature. GA_ProtectiveDome uses this path.

#### Case B: Standalone CustomEvents - ❌ NOT PARSED
Parser line 4601 skips lines starting with `-`:
```cpp
else if (bInProperties && !TrimmedLine.StartsWith(TEXT("-")))
```

The array format `parameters: [ - name: X, type: Y ]` is NOT parsed for standalone CustomEvents.

**Verdict:**
- GA_ProtectiveDome: No fix needed (uses delegate binding)
- Standalone typed CustomEvents: Generator enhancement needed IF required in future

---

### Issue 5: Custom Function Call Resolution - ❌ CONFIRMED (Generator Fix Needed)

**Original Claim:** Event graph can't call functions defined in same Blueprint's `custom_functions`.

**Fresh Generation (2026-01-25 00:58):**
```
[final_test.log:17954]
Function 'CheckBackstabCondition' not found via shared resolver.
Relevant functions in KismetMathLibrary:
  MultiplyMultiply_FloatFloat
  MultiplyByPi
  ...
```

The function resolver searches:
1. WellKnown functions (K2_EndAbility, etc.)
2. ExplicitClass (class specified in manifest)
3. ParentChain (NarrativeGameplayAbility hierarchy)
4. LibraryFallback (KismetMathLibrary, etc.)

**Missing:** Same-Blueprint FunctionGraph resolution. The `custom_functions` section creates FunctionGraphs but the CallFunction node generator doesn't search the current Blueprint's FunctionGraphs.

**Verdict:** Generator enhancement required in `FGasAbilityGeneratorFunctionResolver::ResolveFunction()` to search current Blueprint's FunctionGraphs.

---

### Issue 6: Cascade Failure - ❌ CONFIRMED

**Original Claim:** `ApplyGEToSelf` fails as cascade from `CallCheckBackstab` failure.

**Fresh Generation (2026-01-25 00:58):**
```
[final_test.log:18157]
Connection failed: Target node 'ApplyGEToSelf' not found

[final_test.log:18163]
Event graph 'GA_Backstab_EventGraph' summary: Nodes 8/10, Connections 8/15
```

The cascade is:
1. `CallCheckBackstab` node creation fails (Issue 5)
2. `Branch_CanBackstab` has no input connection
3. `ApplyGEToSelf` depends on `Branch_CanBackstab.True`
4. Node creation for `ApplyGEToSelf` fails because execution flow is broken

**Verdict:** Will be automatically fixed when Issue 5 is resolved.

---

## Locked Decisions (Audit Session 2026-01-25)

| Decision ID | Topic | Status | Action |
|-------------|-------|--------|--------|
| D1 | `parameters:` block | ✅ NO FIX | Parser already converts |
| D2 | Pin `Spec` | ✅ NO FIX | Partial matching works |
| D3 | Pin `Tag` | ✅ NO FIX | Partial matching works |
| D4a | Typed CustomEvent (delegate) | ✅ NO FIX | Binder extracts signature |
| D4b | Typed CustomEvent (standalone) | ⏳ OPTIONAL | Enhancement if needed |
| D5 | Custom function call | ❌ **FIX REQUIRED** | Add same-BP FunctionGraph resolution |
| D6 | Cascade | ✅ AUTO-FIXED | Resolves when D5 is fixed |
| D7 | GA_ProtectiveDome connection | ⏳ INVESTIGATE | 1 of 61 connections failing |

---

## Fresh Generation Results (2026-01-25 00:58)

### GA_Backstab - FAILS (2 node failures, 7 connection failures)
```
[final_test.log:17954] Function 'CheckBackstabCondition' not found via shared resolver
[final_test.log:18157] Connection failed: Target node 'ApplyGEToSelf' not found
[final_test.log:18163] Nodes 8/10, Connections 8/15
[final_test.log:18164] FAIL: Nodes=2 Connections=7 PostValidation=0
```

### GA_ProtectiveDome - FAILS (0 node failures, 1 connection failure)
```
[final_test.log:15680] Nodes 47/47, Connections 60/61
[final_test.log:15682] FAIL: Nodes=0 Connections=1 PostValidation=0
```

All 47 nodes created, but 1 connection failed. Need to identify which connection.

---

## Next Steps

1. ~~Wait for build to complete~~ ✅ DONE
2. ~~Run fresh generation~~ ✅ DONE (2026-01-25 00:58)
3. ~~Analyze NEW logs~~ ✅ DONE - Issue 5 confirmed
4. **Implement D5 fix:** Add same-Blueprint FunctionGraph resolution to `FGasAbilityGeneratorFunctionResolver`
5. **Investigate D7:** Identify which connection fails in GA_ProtectiveDome

---

## Audit Participants

- **Claude (Opus 4.5):** Code validation, log analysis
- **GPT:** Cross-validation, challenge assumptions
- **Erdem:** Approval authority

---

## Reference Documents

- `LOCKED_CONTRACTS.md` - Contract 13 (INV-GESPEC-1)
- `GasAbilityGeneratorParser.cpp:4611-4628` - `parameters:` block conversion
- `GasAbilityGeneratorGenerators.cpp:11529-11699` - `FindPinByName` partial matching
- `GasAbilityGeneratorFunctionResolver.cpp` - Function resolution logic

---

## Appendix: Log Evidence

### Spec Pin Connection Success
```
[commandlet_output_v2.log:1140]
Connecting: MakeGESpec.ReturnValue [struct] -> ApplyGE.SpecHandle [struct&]
Connected: MakeGESpec.ReturnValue -> ApplyGE.Spec
```

### Delegate Binding Signature Extraction
```
[pin_test2.log:14255]
Created handler event: HandleDomeDamageAbsorption with signature from OnDamagedBy__DelegateSignature
[pin_test2.log:14282]
Created delegate bind/unbind nodes for OwnerASC.OnDamagedBy -> HandleDomeDamageAbsorption
```

### Layout Warning (Not Connection Failure)
```
[fail_fast_report.txt:106-107]
E_PIN_POSITION_NOT_FOUND (30 errors) is a LAYOUT HELPER warning only.
Actual connections still work.
```

### Fresh Generation: GA_Backstab Function Resolution Failure
```
[final_test.log:17947-17955]
Generating event graph 'GA_Backstab_EventGraph' in Blueprint GA_Backstab
...
Created node: Branch_ValidTarget (Branch)
Function 'CheckBackstabCondition' not found via shared resolver.
Relevant functions in KismetMathLibrary:
  MultiplyMultiply_FloatFloat
  MultiplyByPi
```

### Fresh Generation: GA_Backstab Cascade Node Failure
```
[final_test.log:18093-18097]
Node: None (K2Node_CallFunction)
  Ptr: 0x00000228907D6C00
  GUID: 00000000000000000000000000000000   ← Invalid GUID = failed node
  Pos: X=0, Y=0
  Path: .../GA_Backstab.GA_Backstab:EventGraph.K2Node_CallFunction_1
```

### Fresh Generation: GA_Backstab Summary
```
[final_test.log:18163-18166]
Event graph 'GA_Backstab_EventGraph' summary: Nodes 8/10, Connections 8/15
[FAIL] GenerateEventGraph failed for GA_Backstab_EventGraph: Nodes=2 Connections=7 PostValidation=0
[FAIL] GA_Backstab: Event graph generation failed
```

### Fresh Generation: GA_ProtectiveDome Summary
```
[final_test.log:15680-15687]
Event graph 'GA_ProtectiveDome_EventGraph' summary: Nodes 47/47, Connections 60/61
VALIDATION: 3 potentially unconnected input pin(s) detected
[FAIL] GenerateEventGraph failed for GA_ProtectiveDome_EventGraph: Nodes=0 Connections=1 PostValidation=0
[FAIL] GA_ProtectiveDome: Event graph generation failed
```
