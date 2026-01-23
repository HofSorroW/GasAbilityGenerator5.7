# Delegate Binding Extensions Specification v1.1

**Status:** APPROVED AND LOCKED (Claude-GPT Dual Audit Complete)
**Date:** 2026-01-22
**Audit Trail:** Claude Opus 4.5 + GPT-4 adversarial audit
**Scope:** Extensions to v4.21 Delegate Binding Automation

---

## Executive Summary

This specification defines two new delegate binding patterns for the GasAbilityGenerator plugin:

1. **Section 10: External ASC Binding** — Bind to delegates on external actors' AbilitySystemComponents (e.g., target enemy's OnDied)
2. **Section 11: Attribute Change Delegates** — Bind to attribute value changes using AbilityTask pattern

Both patterns were validated through adversarial audit against UE5.7 engine headers and Narrative Pro source.

---

## Section 10: External ASC Binding (Actor → ASC Resolution)

**Scope:** Extension to v4.21 Delegate Binding Automation for binding to delegates on external actors' AbilitySystemComponents.

---

### 10.1 ASC Resolution Contract

**10.1.1** When `source` field references a Blueprint variable of type `AActor` (or subclass), the generator MUST extract the ASC using `UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent`.

**10.1.2** The resolution follows UE5's built-in fallback behavior:
1. FIRST: Check if actor implements `IAbilitySystemInterface` and call `GetAbilitySystemComponent()`
2. FALLBACK: Perform component search for `UAbilitySystemComponent` on the actor

**10.1.3** The generator MUST NOT require actors to implement `IAbilitySystemInterface`. The component search fallback is acceptable.

**10.1.4** After ASC extraction, the generator MUST cast to `UNarrativeAbilitySystemComponent` using the existing cast logic (v4.21.1 pattern).

**10.1.5** Node generation order for Actor source:
```
VariableGet(ActorVar) → GetAbilitySystemComponent(Actor) → Cast to NASC → AddDelegate
```

**10.1.6** Audit note: Component search is non-deterministic if multiple ASCs exist on an actor. This is documented behavior, not a defect.

---

### 10.2 Source Type Detection

**10.2.1** The generator MUST detect source type by inspecting the Blueprint variable's `PinType.PinSubCategoryObject`:
- IF variable class `IsChildOf(AActor::StaticClass())` AND NOT `IsChildOf(UAbilitySystemComponent::StaticClass())` THEN treat as Actor source
- IF variable class `IsChildOf(UAbilitySystemComponent::StaticClass())` THEN treat as ASC source (existing v4.21.1 behavior)

**10.2.2** The manifest schema MAY include an optional `source_type` field for explicit declaration:
- `source_type: Actor` — Force Actor→ASC extraction
- `source_type: ASC` — Treat as direct ASC reference
- WHEN omitted: Auto-detect from variable type

**10.2.3** IF `source_type` is specified AND conflicts with actual variable type, generator MUST emit `[E_SOURCE_TYPE_MISMATCH]` and FAIL generation for that binding.

---

### 10.3 Validity Checking

**10.3.1** Definition of "invalid" for external ASC binding:
- Actor variable is `nullptr`
- Actor is `PendingKill`
- `GetAbilitySystemComponent` returns `nullptr`
- Cast to `UNarrativeAbilitySystemComponent` fails

**10.3.2** The manifest schema MUST support an `optional` field:
- `optional: true` — Binding failure is non-fatal
- `optional: false` (default) — Binding failure is fatal

**10.3.3** WHEN `optional: false` (default):
- Generator MUST create nodes without validity guards
- Runtime failure results in Blueprint error (standard UE behavior)
- Use case: Developer guarantees target validity at bind time

**10.3.4** WHEN `optional: true`:
- Generator MUST insert `IsValid` check node before `AddDelegate`
- IF `IsValid` returns false: Skip `AddDelegate`, continue execution
- Generator MUST emit `[W_OPTIONAL_BINDING_SKIPPED]` log at runtime (via PrintString node with conditional)
- Guard placement: Between ASC extraction and `AddDelegate` node

**10.3.5** The `optional` field MUST NOT affect `RemoveDelegate` path in `EndAbility`. RemoveDelegate MUST always include `IsValid` guard regardless of `optional` setting (defensive cleanup).

---

### 10.4 Activation-Time Availability

**10.4.1** PRECONDITION: The source Actor variable MUST be assigned BEFORE the delegate binding nodes execute in `ActivateAbility`.

**10.4.2** The generator does NOT provide rebind triggers. If the Actor is assigned after `ActivateAbility` completes, the binding will not occur.

**10.4.3** This is a documented limitation, not a defect. Use cases requiring late binding MUST implement manual Blueprint logic.

**10.4.4** The generator SHOULD emit `[W_LATE_BIND_RISK]` warning during generation IF:
- Source is Actor type AND
- No event graph node assigns the variable before delegate binding nodes

**10.4.5** IF late-bind detection is not feasible at generation time, this warning MAY be omitted. The precondition (10.4.1) remains the developer's responsibility.

---

## Section 11: Attribute Change Delegate Binding

**Scope:** New manifest section for binding to attribute value changes via AbilityTask pattern.

---

### 11.1 Manifest Schema

**11.1.1** Attribute bindings MUST use a separate manifest section: `attribute_bindings` (distinct from `delegate_bindings`).

**11.1.2** Required fields per binding:
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | YES | Unique identifier for this binding |
| `attribute_set` | string | YES | Class name of the AttributeSet (e.g., `NarrativeAttributeSet`) |
| `attribute` | string | YES | Property name within the AttributeSet (e.g., `Health`) |
| `source` | string | YES | ASC source: `OwnerASC`, `PlayerASC`, or variable name |
| `handler` | string | YES | Name of CustomEvent to create (no parameters) |

**11.1.3** Optional fields:
| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `trigger_once` | bool | true | Maps to task's `TriggerOnce` parameter |
| `with_tag` | string | empty | Optional source tag filter |
| `without_tag` | string | empty | Optional exclusion tag filter |

**Header proof:** `AbilityTask_WaitAttributeChange.h:50` factory signature shows `bool TriggerOnce=true`.

---

### 11.2 Attribute Identification

**11.2.1** The generator MUST resolve the full `FGameplayAttribute` from `attribute_set` + `attribute` combination.

**11.2.2** Resolution steps:
1. Find `UClass` for `attribute_set` (e.g., `UNarrativeAttributeSet`)
2. Find `FProperty` named `attribute` on that class
3. Construct `FGameplayAttribute` from the property

**11.2.3** IF `attribute_set` class not found: FAIL with `[E_ATTRIBUTE_SET_NOT_FOUND]`

**11.2.4** IF `attribute` property not found on the AttributeSet: FAIL with `[E_ATTRIBUTE_NOT_FOUND]`

**11.2.5** The generator MUST NOT accept unqualified attribute names (e.g., `attribute: Health` without `attribute_set`). This prevents ambiguity between AttributeSets.

---

### 11.3 Binding Model Decision

**11.3.1** The generator MUST use **AbilityTask-based binding** for attribute change delegates.

**11.3.2** Rationale:
- `FOnGameplayAttributeValueChange` is NOT a dynamic multicast delegate
- Unbinding requires `FDelegateHandle` storage
- `FDelegateHandle` is not Blueprint-serializable
- UE5's AbilityTask encapsulates handle management in C++
- This is the only Blueprint-friendly pattern for attribute delegates

**11.3.3** Supported task class (v1):
- `UAbilityTask_WaitAttributeChange` ONLY
- Generator uses the task's `BlueprintCallable` factory function: `WaitForAttributeChange`
- Additional task classes (e.g., `UAbilityTask_WaitAttributeChangeThreshold`, `UAbilityTask_WaitAttributeChangeRatioThreshold`) are OUT OF SCOPE for v1

**11.3.4** Task creation requirements:
- Task MUST be created through the official AbilityTask factory (`WaitForAttributeChange` BlueprintCallable)
- Factory ensures task is registered with the owning ability
- Factory ensures task receives `EndAbility` cleanup via `OnDestroy()`
- Generator MUST NOT instantiate task via `NewObject` or other non-factory paths

**11.3.5** Callback connection:
- `UAbilityTask_WaitAttributeChange` exposes `OnChange` delegate (type: `FWaitAttributeChangeDelegate`)
- **Header proof:** `AbilityTask_WaitAttributeChange.h:31` declares:
  ```cpp
  DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWaitAttributeChangeDelegate);
  ```
- Generator MUST bind handler CustomEvent to the task's `OnChange` delegate pin
- Handler CustomEvent signature: **no parameters** (zero-param delegate)

**11.3.6** Attribute value access pattern:
- The AbilityTask's Blueprint API exposes only an exec signal (`OnChange`) with no payload pins
- Therefore the generator cannot pass `FOnAttributeChangeData` into the handler
- To access current attribute value, handler MUST query ASC directly (e.g., `GetNumericAttribute`)

**11.3.7** Scope clarification:
- Direct delegate chain binding: **NOT SUPPORTED** (requires C++ handle storage)
- AbilityTask pattern: **REQUIRED**
- Non-AbilityTask AsyncTask patterns: **OUT OF SCOPE**

---

### 11.4 CustomEvent Signature

**11.4.1** The generated CustomEvent MUST have **no parameters**.

**11.4.2** Rationale: `FWaitAttributeChangeDelegate` is declared as `DECLARE_DYNAMIC_MULTICAST_DELEGATE()` with zero params.

**11.4.3** Handler logic MUST query attribute value explicitly:
```
CustomEvent (no params) → GetNumericAttribute(ASC, Attribute) → Use value
```

---

### 11.5 GEModData Caveat

**11.5.1** `FOnAttributeChangeData.GEModData` is NOT accessible via AbilityTask Blueprint interface.

**11.5.2** The AbilityTask internally receives `FOnAttributeChangeData` but does NOT expose it to the `OnChange` delegate.

**11.5.3** Handlers requiring GEModData MUST use C++ implementation (out of scope for this generator feature).

**11.5.4** Attribute identity is implicit from task creation: `WaitForAttributeChange(..., Attribute, ...)`.

---

### 11.6 Lifecycle and Cleanup

**11.6.1** AbilityTask manages its own lifecycle when created correctly:
- Task created via `WaitForAttributeChange` factory in `ActivateAbility` event chain
- Task registered with owning ability automatically
- Task destroyed when ability ends (any path: complete, cancel, interrupt)
- Delegate unbind handled internally by task's `OnDestroy()`

**11.6.2** No explicit unbind nodes required in `EndAbility`.

**11.6.3** Ownership guarantee:
- Task MUST be created through official AbilityTask factory (BlueprintCallable)
- This ensures registration with ability's task system
- This ensures cleanup on `EndAbility`

**11.6.4** WHEN ability ends before attribute change fires:
- Task is destroyed cleanly via ability's task cleanup
- Delegate automatically unbound in task's `OnDestroy()`
- No memory leak or dangling delegate

**11.6.5** WHEN task created outside factory (error case):
- Generator MUST NOT produce this pattern
- If detected at generation time: FAIL with `[E_TASK_FACTORY_REQUIRED]`

---

## Section 12: Error Codes

| Code | Severity | Description |
|------|----------|-------------|
| `E_SOURCE_TYPE_MISMATCH` | FAIL | Explicit `source_type` conflicts with variable type |
| `E_ATTRIBUTE_SET_NOT_FOUND` | FAIL | AttributeSet class not found |
| `E_ATTRIBUTE_NOT_FOUND` | FAIL | Attribute property not found on AttributeSet |
| `E_TASK_FACTORY_REQUIRED` | FAIL | AbilityTask created outside official factory |
| `W_OPTIONAL_BINDING_SKIPPED` | WARN | Optional binding skipped at runtime (invalid target) |
| `W_LATE_BIND_RISK` | WARN | Actor source may not be assigned at bind time |

---

## Audit Trail

### Participants
- **Claude Opus 4.5** — Primary drafter, counter-auditor
- **GPT-4** — Adversarial auditor

### Key Audit Findings Resolved
1. `FDelegateHandle` is not Blueprint-serializable → AbilityTask pattern required for attribute delegates
2. `FOnAttributeChangeData` does NOT contain `Attribute` field (GPT retracted claim)
3. `FWaitAttributeChangeDelegate` is zero-param delegate (header proof: line 31)
4. `TriggerOnce` default is `true` (header proof: line 50)
5. UE5 ASC resolution has built-in fallback (IAbilitySystemInterface → component search)

### Version History
- v1.0: Initial draft (2026-01-22)
- v1.1: Resolved proof-blockers, AbilityTask pattern finalized (2026-01-22)

---

## Implementation Notes

### Section 10 Implementation Location
- `GasAbilityGeneratorGenerators.cpp` — Extend `GenerateDelegateBindingNodes()` (lines 12957+)
- Add Actor type detection before existing variable resolution
- Insert `GetAbilitySystemComponent` node when Actor source detected

### Section 11 Implementation Location
- `GasAbilityGeneratorGenerators.cpp` — New function `GenerateAttributeBindingNodes()`
- `GasAbilityGeneratorTypes.h` — New struct `FManifestAttributeBindingDefinition`
- `GasAbilityGeneratorParser.cpp` — Parse `attribute_bindings` section

### Dependencies
- `GameplayAbilitiesEditor` module (already added in v4.15)
- `UK2Node_LatentAbilityCall` for AbilityTask node creation

### Implementation Lessons Learned (v4.21.2)

Critical implementation details discovered during core delegate binding fixes:

**1. UK2Node_Self Pin Name**
- `UK2Node_Self` uses `PN_Self` for its output pin, NOT `PN_ReturnValue`
- Source: `K2Node_Self.cpp:59` — `CreatePin(..., UEdGraphSchema_K2::PN_Self)`
- Symptom if wrong: `FindPin(PN_ReturnValue)` returns null, Self pin wiring silently skipped

**2. Pin Type Propagation**
- `MakeLinkTo()` does NOT automatically trigger `NotifyPinConnectionListChanged()`
- For `UK2Node_DynamicCast`, must call `CastNode->NotifyPinConnectionListChanged(CastSourcePin)` after wiring
- Without this, compiler validation fails with "Object type undetermined" because Cast node doesn't resolve connected pin types

**3. Exec Reachability for Cast Nodes**
- Impure nodes (like `UK2Node_DynamicCast`) must be in exec flow or `PruneIsolatedNodes()` removes them
- For delegate bindings with separate ActivateAbility/EndAbility paths, create SEPARATE Cast node instances
- Each Cast node must be wired into its respective exec chain before AddDelegate/RemoveDelegate

**4. ASC Resolution Function**
- For abilities, use `UGameplayAbility::GetAbilitySystemComponentFromActorInfo()` (member function, needs Self pin)
- NOT `UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor)` (static, needs Actor input)
- The member function is simpler as it doesn't require resolving an Actor reference first
