# LOCKED_CONTRACTS.md (v4.31)

## Purpose

This document defines **Tier 1 (LOCKED)** contracts for GasAbilityGenerator.
These contracts are data-safety and CI-safety invariants. Refactors are allowed only if invariants remain true.

**Rule:** If a change touches a LOCKED contract or its implementation, it requires an explicit review session and a full regression pass.

---

## Definitions

### Decision Path
Any code path that determines: **create / skip / modify / conflict / fail** (including UI status logic that influences user actions).

### Persist Changes
Any of:
- `SavePackage` (or equivalent editor save APIs)
- `MarkPackageDirty`
- Asset creation/overwrite via factories/AssetTools
- Writing metadata to assets or registry that changes future behavior

### Generated Asset
An asset is "generated" iff generator metadata exists and indicates generated (`bIsGenerated=true`) from either:
- On-asset `UAssetUserData` (`UGeneratorAssetMetadata`)
- Central registry (`UGeneratorMetadataRegistry`)

---

## LOCKED CONTRACT 1 — Metadata Contract (Registry + AssetUserData)

### Invariant
- Metadata **read** must be registry-aware via:
  - `GetMetadataEx()` / `HasMetadataEx()` / `TryGetMetadata()`
- Metadata **write** must store to the correct source:
  - Asset supports `IInterface_AssetUserData` → write `UGeneratorAssetMetadata`
  - Otherwise → write `UGeneratorMetadataRegistry`

### Forbidden
- Using `GetMetadata()` or `GetAssetMetadata()` in decision paths
- Treating "no `UGeneratorAssetMetadata*`" as "manual" without checking registry

### Allowed
- Using `GetMetadata()` for purely informational UI (non-decision) or debugging

---

## LOCKED CONTRACT 2 — Regen/Diff Semantics (InputHash / OutputHash)

### Invariant
- `InputHash` represents the generator input definition state.
- `OutputHash` represents the generated asset's output state used to detect manual edits.
- A "conflict" is determined by the established rules (input changed vs output changed as implemented).

### Forbidden
- "Simplifying" by skipping fields in hash computations to reduce noise
- Changing what `InputHash`/`OutputHash` mean without versioned migration and explicit approval

---

## LOCKED CONTRACT 3 — CONFLICT Gating and Force Semantics

### Invariant
- `EDryRunStatus::Conflicted` (or equivalent) **blocks generation** unless `--force` is explicitly enabled.
- Force behavior must remain consistent with documented meaning.

### Forbidden
- Auto-resolving conflicts
- Converting conflict into warning without `--force`
- Silently proceeding with overwrite in conflict scenarios

---

## LOCKED CONTRACT 4 — Dry-Run Persistence Contract

### Invariant
In dry-run mode, the system **MUST NOT** persist changes:
- No `SavePackage`
- No `MarkPackageDirty`
- No asset creation/overwrite
- No metadata writes that affect future runs

### Allowed
- Loading assets
- Computing hashes
- Producing reports/logs
- Determining what would happen

---

## LOCKED CONTRACT 5 — Disk-Truth Asset Existence Contract

### Invariant
- Existence checks used for **create/skip decisions** must use disk-truth (`DoesAssetExistOnDisk`).
- Callers must pass **long package name**:
  - ✅ `/Game/Folder/AssetName`
  - ❌ `/Game/Folder/AssetName.AssetName`

### Forbidden
- Replacing with AssetRegistry existence checks for create/skip decisions
- Passing object paths where package paths are required

---

## LOCKED CONTRACT 6 — Manifest Whitelist Gate (Anti-Asset-Spam)

### Invariant
- Asset creation/modification must respect the manifest whitelist.
- Assets not in whitelist must not be generated (except explicitly documented exceptions).

### Forbidden
- Disabling whitelist checks to "make generation work"
- Creating assets outside the declared scope of the manifest

---

## LOCKED CONTRACT 7 — 3-Way Merge Contract (XLSX Sync)

### Invariant
- Conflicts in 3-way merge require **explicit user choice**.
- No silent auto-apply except the clearly-defined "unchanged" case.

### Forbidden
- Auto-applying non-trivial merge cases
- Updating `LastSyncedHash` without a successful, user-approved sync

---

## LOCKED CONTRACT 8 — Headless Safety (Policy Boundaries)

### Invariant
- Headless runs must correctly mark/save/report assets requiring editor verification (e.g., `bHeadlessSaved`).
- Any "escape hatch" policy (like Policy B) must produce warnings and be reported.

### Forbidden
- Treating headless save as clean success without reporting
- Suppressing verification requirements in headless mode

---

## LOCKED CONTRACT 9 — Reporting Schema (CI Interface)

### Invariant
- Report item fields and `RESULT` footer format are **stable**.
- Exit code meanings are stable:
  - `0` = success
  - non-zero = failure (as defined)

### Forbidden
- Changing `RESULT` footer keys/format without coordinated CI update
- Removing required report fields or altering meanings

---

## LOCKED CONTRACT 10 — Blueprint Compile Gate (v4.16 Graph Validation)

### Invariant
- Every Blueprint-generating generator must call `CompileBlueprint()` with `FCompilerResultsLog`
- If `FCompilerResultsLog.NumErrors > 0`, the asset **MUST NOT** be saved
- Each Blueprint generator may call `SavePackage` **at most once**, and only after passing validation gates

### Forbidden
- Saving a Blueprint without compile validation
- Checkpoint saves (multiple `SavePackage` calls per asset)
- Ignoring compile errors and proceeding to save
- Treating `CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE` as warning (must be hard fail)

### Allowed
- Multiple `CompileBlueprint()` calls if intermediate compiles are needed for CDO access
- Collecting all errors before failing (fail after enumeration, not on first error)

### Reference
- Audit: `ClaudeContext/Handoffs/Graph_Validation_Audit_v1.md`
- Implementation: `ClaudeContext/Handoffs/Graph_Validation_Implementation_v4.16.md`
- Decisions: D-006, D-008, D-009, D-010, D-011

---

## P1.3 — Startup Effects Validation (v4.16.2 Locked)

### Context

AbilityConfiguration assets with Father form abilities (GA_FatherCrawler, GA_FatherArmor, etc.) require a default form state GE in `startup_effects`. Missing form state causes runtime failures (abilities check `Effect.Father.FormState.*` tags).

### Validation (Implemented at `FAbilityConfigurationGenerator::ValidateStartupEffects`)

**Error Codes:**

| Code | Severity | Meaning |
|------|----------|---------|
| `E_AC_MISSING_FORM_STATE` | FAIL | Configuration has form abilities but no `GE_*State` in startup_effects |
| `E_AC_STARTUP_EFFECT_NOT_FOUND` | FAIL | A startup effect class could not be resolved via `LoadClass<UGameplayEffect>` |

**Message Format (single-line, pipe-delimited):**
```
E_AC_MISSING_FORM_STATE | AC_FatherCrawler | Ability configuration with form abilities must have a default GE_*State in startup_effects | Add GE_CrawlerState or equivalent
```

### Abort Strategy

1. Collect **all** errors for the asset (do not fail-fast on first error)
2. After validation pass, if any FAIL-severity errors exist:
   - Skip asset creation entirely
   - Report all collected errors
   - Mark overall run as failed (non-zero exit)

### CI Gate

AbilityConfiguration generation with FAIL errors must cause commandlet to return non-zero exit code.

### Reference

- Implementation: `GasAbilityGeneratorGenerators.cpp :: FAbilityConfigurationGenerator::ValidateStartupEffects(...)`
- Audit: `Implementation_Plans_Audit_v1.md` Section P1.3

---

## TEMPORARY EXCEPTION 1 — Rule #9 Father Validation in Core (v4.16)

### Background
Rule #9 normally forbids project-specific logic in core generator code.

**Exception (temporary, intentional):**
The plugin currently contains Father-project-specific validation checks (e.g. `GA_Father*` ability names and `Father.State.*` tags).
This exception exists to preserve **gameplay-critical validation behavior** while stability and correctness are prioritized.

This exception has been **dual-audited and contractually locked**.

### Applies to these exact code locations (enforcement scope)
- `GasAbilityGeneratorGenerators.cpp :: FGameplayAbilityGenerator::ValidateFormAbility(...)`
- `GasAbilityGeneratorGenerators.cpp :: FAbilityConfigurationGenerator::ValidateStartupEffects(...)`

(Line numbers are intentionally omitted to prevent drift; symbol anchors are authoritative.)

### Constraints (must follow)
1. **No new project-specific validations** may be added to core generator or parser code unless Erdem explicitly approves the change.
2. **Existing Father validations must remain diagnostic-only**:
   - Structured ERROR / WARNING reporting is permitted
   - No new generation behavior may be introduced beyond already-locked abort/skip gates
3. **Father-specific identifiers must remain isolated** to the two symbol-anchored locations above.
   Duplication or spread to other generators/files is forbidden.

### Grep Guard (conceptual)
Any new occurrences of `GA_Father` or `Father.State.` outside the approved symbol anchors require explicit approval from Erdem.

### Deferred Generalization
When scheduled, these validations will be migrated to a manifest-driven opt-in validation profile.
All hard-coded Father identifiers will then be removed from core generator code.

### Status
**LOCKED** until Erdem changes it.

---

## LOCKED CONTRACT 11 — C_SYMBIOTE_STRICT_CANCEL (v4.28.2)

### Context

Symbiote is an **ultimate ability** with 30-second duration. During Symbiote, the player should NOT be able to cancel it via any player-initiated ability (form changes, weapon forms).

**Exception:** GA_FatherSacrifice is an auto-trigger emergency save ability. It CAN cancel Symbiote when the player would otherwise die.

### Invariant

1. **GE_SymbioteDuration** must be applied at the START of GA_FatherSymbiote activation using SetByCaller pattern with `Data.Symbiote.Duration` tag
2. **GE_SymbioteDuration** grants `Father.State.SymbioteLocked` tag for the duration
3. All player-initiated abilities that could cancel Symbiote must have `Father.State.SymbioteLocked` in their `activation_blocked_tags`:
   - GA_FatherCrawler ✓
   - GA_FatherArmor ✓
   - GA_FatherExoskeleton ✓
   - GA_FatherEngineer ✓
   - GA_FatherRifle ✓
   - GA_FatherSword ✓
4. `Ability.Father.Symbiote` must NOT be in `cancel_abilities_with_tag` for the above abilities
5. **GA_FatherSacrifice** MAY retain `Ability.Father.Symbiote` in its cancel list (emergency override)

### Defense-in-Depth Strategy

- **Layer 1 (Blocking):** `Father.State.SymbioteLocked` in blocked_tags prevents ability activation
- **Layer 2 (No Cancel):** Removing Symbiote from cancel lists ensures even if blocking fails, abilities won't forcibly cancel Symbiote
- **Layer 3 (Duration Enforcement):** GE_SymbioteDuration uses `HasDuration` policy with SetByCaller, ensuring the lock automatically expires

### Forbidden

- Adding `Ability.Father.Symbiote` to any player-initiated ability's cancel_abilities_with_tag (except GA_FatherSacrifice)
- Removing `Father.State.SymbioteLocked` from any form/weapon ability's activation_blocked_tags
- Removing GE_SymbioteDuration application from GA_FatherSymbiote activation flow

### Reference

- Manifest: `manifest.yaml` — GA_FatherSymbiote event_graph, GE_SymbioteDuration definition
- Audit: Claude–GPT dual audit session (2026-01-23)
- Implementation: v4.28.2

---

## LOCKED CONTRACT 12 — R-AI-1 Activity System Compatibility (v4.30)

### Context

NPCs using Narrative Pro's Activity system (via `activity_configuration` in NPCDefinition) have their Behavior Trees managed by `NPCActivityComponent`. A GameplayAbility that directly calls `RunBehaviorTree` on the AIController bypasses this system, causing state conflicts.

**Father Companion Case:** `NPC_FatherCompanion` has `activity_configuration: AC_FatherBehavior`, meaning Father uses the Activity system for normal AI behaviors (follow, attack, etc.).

### Invariant

1. If an NPC has `activity_configuration` set, GameplayAbilities on that NPC **MUST NOT** call `RunBehaviorTree` directly without coordinating with the Activity system
2. Acceptable coordination strategies:
   - **Option A:** Create a dedicated Activity (e.g., `BPA_FatherEngineer`) and request it via `NPCActivityComponent`
   - **Option B:** Call `StopCurrentActivity()` before `RunBehaviorTree`, and trigger activity reselection on EndAbility
3. GA_FatherEngineer specifically uses **Option B** (implemented v4.30)

### Forbidden

- Calling `RunBehaviorTree` directly on AIController without stopping current activity first
- Leaving the Activity system in a desync state when ability ends
- Creating competing BT drivers (activity BT + ability BT running simultaneously)

### Implementation (GA_FatherEngineer)

```yaml
# Before RunBehaviorTree:
- id: GetActivityComponent
  type: CallFunction
  properties:
    function: GetComponentByClass
    class: Actor
  # Returns NPCActivityComponent

- id: StopCurrentActivity
  type: CallFunction
  properties:
    function: StopCurrentActivity
    class: NPCActivityComponent

# Then RunBehaviorTree as before
```

### Reference

- Manifest: `manifest.yaml` — GA_FatherEngineer event_graph, NPC_FatherCompanion definition
- Narrative Pro: `NPCActivityComponent.h:131` — `StopCurrentActivity()` is BlueprintCallable
- Audit: Claude–GPT dual audit session (2026-01-24)
- Implementation: v4.30

---

## LOCKED CONTRACT 13 — INV-GESPEC-1 MakeOutgoingGameplayEffectSpec Parameter (v4.31)

### Context

`MakeOutgoingGameplayEffectSpec` is a UGameplayAbility member function that creates a GE spec for later application. The first parameter is `TSubclassOf<UGameplayEffect> GameplayEffectClass`. Without this parameter set correctly, the function returns an invalid spec handle and `ApplyGameplayEffectSpecToTarget` silently does nothing.

### Invariant

1. All `MakeOutgoingGameplayEffectSpec` nodes in manifest **MUST** use `param.GameplayEffectClass: GE_*` syntax
2. For class pin default assignment in this generator path, `param.*` keys drive the assignment; non-`param.*` keys like `gameplay_effect_class:` are ignored by design
3. Using `gameplay_effect_class:` (without `param.` prefix) results in the property being ignored

### Technical Evidence

**Generator Processing (GasAbilityGeneratorGenerators.cpp:10335):**
```cpp
if (PropPair.Key.StartsWith(TEXT("param.")))  // Only param.* prefix processed
{
    FString PinName = PropPair.Key.Mid(6);  // Extract pin name after "param."
    // ...
    ParamPin->DefaultObject = ResolvedClass;  // Sets TSubclassOf<> pin
}
```

**UE5.7 Pin Schema (EdGraphSchema_K2.h:394):**
```cpp
static UE_API const FName PC_Class;  // DefaultValue string should always be empty, use DefaultObject.
```

**Pin Name Verification (from generation log):**
```
Available pins:
  - self (object)
  - GameplayEffectClass (class)  <-- Pin name matches C++ parameter exactly
  - Level (real)
```

### Forbidden

- Using `gameplay_effect_class:` property (not processed by generator)
- Using any property name other than `param.GameplayEffectClass` for this pin
- Omitting `param.GameplayEffectClass` from MakeOutgoingGameplayEffectSpec nodes

### Correct Pattern

```yaml
- id: MakeSpec
  type: CallFunction
  properties:
    function: MakeOutgoingGameplayEffectSpec
    class: UGameplayAbility
    param.GameplayEffectClass: GE_StealthActive  # CORRECT
```

### Failure Mode

Silent runtime failure: Null GameplayEffectClass → Invalid FGameplayEffectSpecHandle → ApplyGameplayEffectSpecToTarget no-op → Effect never applied, no error logged.

### Reference

- Audit: `ClaudeContext/Handoffs/Father_Companion_GAS_Abilities_Audit.md` (VTF-9)
- Generator: `GasAbilityGeneratorGenerators.cpp:10335-10495`
- UE5.7: `EdGraphSchema_K2.h:394`, `GameplayAbility.h:226`
- Implementation: v4.31 (16 abilities fixed)

---

## Enforcement

### Code Review Rule
Any change that touches a LOCKED implementation must:
1. Reference this document and the specific contract IDs
2. Include a regression checklist confirming invariants remain true

### Mechanical Lock (choose at least one)
- CI hash gate for this file + critical locked headers
- "Locked/ requires explicit approval" rule
- Minimal automation tests for Contracts 1, 4, 5, 9

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| v4.12.5 | 2026-01-18 | Initial creation after GPT validation session |
| v4.16 | 2026-01-21 | Added Contract 10 — Blueprint Compile Gate (GPT audit) |
| v4.16.1 | 2026-01-21 | Added Temporary Exception 1 — Rule #9 Father Validation (dual-agent audit) |
| v4.16.2 | 2026-01-22 | Locked P1.3 Startup Effects Validation (Claude–GPT dual audit): severity definitions, error codes, abort strategy, CI gate, symbol anchors |
| v4.28.2 | 2026-01-23 | Added Contract 11 — C_SYMBIOTE_STRICT_CANCEL (Claude–GPT dual audit): Symbiote ultimate cannot be cancelled by player-initiated abilities |
| v4.30 | 2026-01-24 | Added Contract 12 — R-AI-1 Activity System Compatibility (Claude–GPT dual audit): NPCs with ActivityConfiguration must coordinate BT calls with Activity system. GA_FatherEngineer fixed to call StopCurrentActivity before RunBehaviorTree. |
| v4.31 | 2026-01-24 | Added Contract 13 — INV-GESPEC-1 MakeOutgoingGameplayEffectSpec Parameter (Claude–GPT dual audit): All MakeOutgoingGameplayEffectSpec nodes MUST use `param.GameplayEffectClass:` syntax. 16 abilities fixed for silent runtime failure. |
