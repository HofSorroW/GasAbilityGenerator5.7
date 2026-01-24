# GA_Backstab & GA_ProtectiveDome Error Audit

**Date:** 2026-01-24
**Status:** Research Complete - No Code Changes Made
**Context:** Debug analysis of event graph generation failures

---

## Executive Summary

Two abilities (GA_ProtectiveDome, GA_Backstab) fail event graph generation due to manifest syntax issues that don't match the generator's implemented capabilities. This report identifies root causes using the locked specifications and existing documentation.

**Generation Status:** 154/156 assets succeed. Only these 2 abilities fail.

---

## GA_ProtectiveDome Failures

### Failure 1: `parameters:` Block Syntax Not Supported

**Manifest (line 4571-4573):**
```yaml
- id: TryActivateBurst
  type: CallFunction
  properties:
    function: TryActivateAbilityByClass
    class: AbilitySystemComponent
    parameters:                          # ← WRONG SYNTAX
      InAbilityToActivate: GA_DomeBurst
      bAllowRemoteActivation: false
```

**Root Cause:** Per `GasAbilityGeneratorGenerators.cpp:10343`, the generator only supports the `param.` prefix pattern, not nested `parameters:` blocks.

**Correct Syntax (per LOCKED CONTRACT 13 - INV-GESPEC-1):**
```yaml
properties:
  function: TryActivateAbilityByClass
  class: AbilitySystemComponent
  param.InAbilityToActivate: GA_DomeBurst
  param.bAllowRemoteActivation: false
```

---

### Failure 2: Pin Name `Spec` Not Found on `ApplyGameplayEffectSpecToSelf`

**Manifest (line 4458):**
```yaml
- from: [SetByCaller, ReturnValue]
  to: [ApplyGE, Spec]                    # ← Wrong pin name
```

**Root Cause:** The `FindPinByName` function (lines 11522+) has no alias for `Spec`. The actual UE5 pin name for `BP_ApplyGameplayEffectSpecToSelf` is `SpecHandle`.

**Fix Required:** Change `Spec` → `SpecHandle` in connections.

---

### Failure 3: Pin Name `Tag` Not Found on `MakeGameplayTagContainerFromTag`

**Manifest (line 4588):**
```yaml
- id: MakeTagContainer_Charged
  type: CallFunction
  properties:
    function: MakeGameplayTagContainerFromTag
    class: BlueprintGameplayTagLibrary
```

**Root Cause:** The connections reference pin `Tag` but `BlueprintGameplayTagLibrary::MakeGameplayTagContainerFromTag` uses pin name `InTag`.

**Fix Required:** Add pin alias `Tag` → `InTag` OR change manifest connections to use `InTag`.

---

### Failure 4: CustomEvent with Typed Parameters

**Manifest (lines 4609-4619):**
```yaml
- id: Event_HandleDomeDamageAbsorption
  type: CustomEvent
  properties:
    event_name: HandleDomeDamageAbsorption
    parameters:                          # ← Typed parameters
      - name: DamageCauserASC
        type: AbilitySystemComponent
      - name: Damage
        type: Float
      - name: Spec
        type: GameplayEffectSpec
```

**Relevant Spec:** Delegate_Binding_Extensions_Spec_v1_1.md, Section 11.4.1:
> "The generated CustomEvent MUST have **no parameters**."

**Root Cause Analysis:** This spec applies specifically to **attribute change delegate** handlers (Section 11). The `OnDamagedBy` delegate is a different delegate type with its own signature (`FOnDamagedByEvent`).

The actual issue is that **CustomEvent typed parameters generation is not implemented** in the generator. The generator creates zero-param CustomEvents only.

---

## GA_Backstab Failures

### Failure 1: Custom Function Call Not Resolved

**Manifest (lines 5427-5431):**
```yaml
- id: CallCheckBackstab
  type: CallFunction
  properties:
    function: CheckBackstabCondition
    target_self: true
```

**Custom Function Definition (lines 5505-5512):**
```yaml
custom_functions:
  - function_name: CheckBackstabCondition
    pure: false
    inputs:
      - name: EnemyActor
        type: Actor
    outputs:
      - name: CanBackstab
        type: Boolean
```

**Root Cause:** The event_graph tries to call `CheckBackstabCondition` defined in `custom_functions:`. Per the Father_Companion_GAS_Abilities_Audit.md, custom_functions ARE implemented, but:

1. **Order of operations issue:** Custom functions must be created BEFORE the event graph can call them
2. **Function resolution:** The generator's `FindFunction` logic may not check the same Blueprint's custom functions during event graph generation

The generator creates custom functions via `AddFunctionGraph()` but the event graph generator uses `FindField<UFunction>()` which looks for C++/native functions or already-compiled BP functions.

---

### Failure 2: `ApplyGEToSelf` Node Not Created

**Manifest (lines 5445-5449):**
```yaml
- id: ApplyGEToSelf
  type: CallFunction
  properties:
    function: BP_ApplyGameplayEffectSpecToOwner
    target_self: true
```

**Root Cause:** This failure is a **cascade effect**. When `CallCheckBackstab` fails to generate, subsequent nodes in the exec chain also fail to connect properly, causing the entire subgraph to be invalid.

The function `BP_ApplyGameplayEffectSpecToOwner` exists and should resolve correctly - the issue is earlier in the chain.

---

## Summary of Root Causes

| Issue | Ability | Root Cause | Type |
|-------|---------|------------|------|
| `parameters:` block | GA_ProtectiveDome | Syntax not supported - use `param.` prefix | Manifest Syntax |
| Pin `Spec` | GA_ProtectiveDome | Should be `SpecHandle` | Pin Name |
| Pin `Tag` | GA_ProtectiveDome | Should be `InTag` | Pin Name |
| Typed CustomEvent | GA_ProtectiveDome | Generator creates zero-param events only | Unsupported Feature |
| Custom function call | GA_Backstab | Event graph can't call functions defined in same BP | Order/Resolution Issue |
| ApplyGEToSelf | GA_Backstab | Cascade from CallCheckBackstab failure | Cascade |

---

## Recommendations (No Code Changes Made)

### GA_ProtectiveDome

Requires manifest corrections:
1. Change `parameters:` block to `param.` prefix syntax
2. Change pin name `Spec` → `SpecHandle`
3. Change pin name `Tag` → `InTag`
4. Typed CustomEvent parameters require generator enhancement (not currently supported)

### GA_Backstab

The custom function architecture works for creating functions, but calling them from event_graph requires the function to exist before the event graph generates. Options:
1. **Two-pass generation** - Generate functions first, compile, then generate event graph
2. **Inline logic** - Restructure event graph to use inline nodes instead of calling a custom function
3. **Generator enhancement** - Add same-Blueprint function resolution during event graph generation

---

## Reference Documents

- `LOCKED_CONTRACTS.md` - Contract 13 (INV-GESPEC-1) defines `param.` prefix syntax
- `Delegate_Binding_Extensions_Spec_v1_1.md` - Section 11.4.1 on CustomEvent signatures
- `Father_Companion_GAS_Abilities_Audit.md` - Documents custom_functions implementation status
- `GasAbilityGeneratorGenerators.cpp:10343` - `param.` prefix handling
- `GasAbilityGeneratorGenerators.cpp:11522` - `FindPinByName` with aliases

---

## Appendix: Pre-Validation Fixes Already Applied

The following math function name errors were fixed prior to this audit:

| Wrong | Correct |
|-------|---------|
| `Multiply_FloatFloat` | `Multiply_DoubleDouble` |
| `Add_FloatFloat` | `Add_DoubleDouble` |
| `GreaterEqual_FloatFloat` | `GreaterEqual_DoubleDouble` |
| `MakeLiteralFloat` | `MakeLiteralDouble` |

These fixes are in the current manifest and pre-validation now passes.
