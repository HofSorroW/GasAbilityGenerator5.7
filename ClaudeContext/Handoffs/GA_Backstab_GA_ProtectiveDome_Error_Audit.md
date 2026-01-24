# GA_Backstab & GA_ProtectiveDome Error Audit

**Date:** 2026-01-24 (Original) | 2026-01-25 (Audit Session Update)
**Status:** AUDIT COMPLETE - Claims Validated Against Code & Logs
**Context:** Debug analysis of event graph generation failures

---

## Executive Summary

**ORIGINAL CLAIM:** Two abilities (GA_ProtectiveDome, GA_Backstab) fail event graph generation due to manifest syntax issues.

**AUDIT RESULT:** After validation with code and logs, **4 of 6 original claims were INCORRECT**. The generator already handles most cases correctly.

| Issue | Original Claim | Audit Verdict | Evidence |
|-------|----------------|---------------|----------|
| 1. `parameters:` block | NOT supported | ✅ **WORKS** | Parser v2.8.2 lines 4611-4628 |
| 2. Pin `Spec` | Not found | ✅ **WORKS** | Log: `Connected: MakeGESpec.ReturnValue -> ApplyGE.Spec` |
| 3. Pin `Tag` | Not found | ✅ **WORKS** | 22+ successful uses in manifest |
| 4. Typed CustomEvent | NOT supported | ⚠️ **PARTIAL** | Delegate-bound: WORKS. Standalone: NOT PARSED |
| 5. Custom function call | Resolution fails | ⏳ **STALE DATA** | Logs from outdated manifest |
| 6. Cascade | Depends on #5 | ⏳ **STALE DATA** | Consequence of #5 |

**ACTION REQUIRED:** Fresh generation run needed to verify current state. Logs are from 2026-01-22/24, manifest updated 2026-01-25.

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

### Issue 5: Custom Function Call Resolution - ⏳ STALE DATA

**Original Claim:** Event graph can't call functions defined in same Blueprint's `custom_functions`.

**Validation:** The logs referenced are OUTDATED:
- fail_fast_report.txt (2026-01-22): Claimed `Less_FloatFloat` not found
- pin_test2.log (2026-01-24): Showed `SelectResult` type mismatch

**Current Manifest (2026-01-25):**
- No `Less_FloatFloat` or deprecated function names
- No `SelectResult` node in `CheckBackstabCondition`
- Manifest commits show fixes applied:
  - `72bd1b7 fix(v6.9): GA_Backstab + GA_FatherEngineer generation failures`
  - `b5bbd74 fix(manifest): GA_Backstab and GA_FatherEngineer generation errors`

**Verdict:** Cannot validate until fresh generation run. The underlying concern about same-BP function resolution may still be valid but is not testable with stale logs.

---

### Issue 6: Cascade Failure - ⏳ STALE DATA

**Original Claim:** `ApplyGEToSelf` fails as cascade from `CallCheckBackstab` failure.

**Verdict:** Consequence of Issue 5. Will be resolved if Issue 5 passes, or will manifest with new error details if Issue 5 still fails.

---

## Locked Decisions (Audit Session 2026-01-25)

| Decision ID | Topic | Status | Action |
|-------------|-------|--------|--------|
| D1 | `parameters:` block | ✅ NO FIX | Parser already converts |
| D2 | Pin `Spec` | ✅ NO FIX | Partial matching works |
| D3 | Pin `Tag` | ✅ NO FIX | Partial matching works |
| D4a | Typed CustomEvent (delegate) | ✅ NO FIX | Binder extracts signature |
| D4b | Typed CustomEvent (standalone) | ⏳ OPTIONAL | Enhancement if needed |
| D5 | Custom function call | ⏳ PENDING | Fresh generation required |
| D6 | Cascade | ⏳ PENDING | Depends on D5 |

---

## Next Steps

1. **Wait for build to complete**
2. **Run fresh generation** with `-force` flag on GA_Backstab and GA_ProtectiveDome
3. **Analyze NEW logs** to determine actual current state
4. **Update this document** with fresh findings

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
