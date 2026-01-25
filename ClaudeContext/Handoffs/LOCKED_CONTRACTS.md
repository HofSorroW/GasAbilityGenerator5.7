# LOCKED_CONTRACTS.md (v4.32)

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

## LOCKED CONTRACT 15 — D-DEATH-RESET Player Death Reset System (v4.33)

### Context

Design Doc Sections 2.2.6 and 2.4.4 require that DomeEnergy and SymbioteCharge reset to 0 on player death. The manifest only bound FatherASC.OnDied, not PlayerASC.OnDied, leaving a gap where player death wouldn't trigger Father system resets.

### Invariants

#### D-DEATH-RESET-1: Reset Location
> Bind resets to **PlayerASC.OnDied** in Father abilities (GA_ProtectiveDome, GA_FatherSymbiote).

**Evidence:** NarrativeAbilitySystemComponent.cpp:639-645 — OnDied.Broadcast() fires on actual death.

#### D-DEATH-RESET-2: Silent Reset Behavior
> On player death, perform silent reset:
> - `BP_FatherCompanion.DomeEnergy = 0`
> - `BP_FatherCompanion.SymbioteCharge = 0`
> - Remove `Father.Dome.FullyCharged` tag (if present)
> - No burst, no VFX/SFX

**Evidence:** Design Doc Sections 2.2.6, 2.4.4.

#### D-DEATH-RESET-3: GA_Death Unchanged
> Father death reset logic belongs in the **OnDied delegate**, not in GA_Death. GA_Death is a Narrative Pro built-in ability responsible for death signaling (e.g., InformKiller). Per Technical Reference Section 56.8, all Father-specific logic must be implemented via the OnDied delegate.

**Evidence:** Tech Ref 56.8 explicit guidance: "Father-specific logic → Put in BP_FatherCompanion OnDied delegate."

#### D-DEATH-RESET-4: OnDied is Correct Signal
> OnDied is the correct signal for actual player death. It fires only when Health reaches 0 and death pipeline completes.

**Evidence:** NarrativeAttributeSetBase.cpp:188-195 — `if (GetHealth() <= 0.f)` triggers OnOutOfHealth.Broadcast(), which leads to OnDied.

### D-SACRIFICE-1: One-Shot Bypass (Related)
> On lethal one-shot damage, Sacrifice triggers (0 < 15% threshold) and applies invulnerability, but cannot prevent death because Health is already 0 and Sacrifice does not restore Health. This is by design.

**Evidence:**
- manifest.yaml:760-765 — GE_SacrificeInvulnerability has no modifiers (no healing)
- UE5.7 GameplayEffect.cpp:3912-3919 — Attribute delegate fires synchronously within SetHealth()

### Forbidden

- Putting Father-specific reset logic in GA_Death
- Binding only to FatherASC.OnDied for player death resets
- Adding burst/VFX on death reset (silent reset required)
- Modifying GA_Death (Narrative Pro built-in)

### Implementation

GA_ProtectiveDome and GA_FatherSymbiote must bind to PlayerASC.OnDied with handler that:
1. Sets DomeEnergy/SymbioteCharge to 0 on Father
2. Removes Father.Dome.FullyCharged tag from Player ASC
3. Does NOT trigger burst ability

### Reference

- Design Doc: Sections 2.2.6, 2.4.4
- Tech Ref: Section 56.8
- Narrative Pro: NarrativeAbilitySystemComponent.cpp:639-645, NarrativeAttributeSetBase.cpp:188-195
- Audit: Claude–GPT dual audit session (2026-01-24)
- Implementation: v4.33

---

## LOCKED CONTRACT 14 — INV-INPUT-1 Input Architecture Invariants (v4.32)

### Context

Narrative Pro uses a tag-based input system where:
1. `UNarrativeAbilityInputMapping` (DataAsset) maps InputAction → InputTag
2. `NarrativePlayerController` binds InputActions to `AbilityInputTagPressed(InputTag)`
3. `NarrativeAbilitySystemComponent` uses `HasTagExact(InputTag)` to find matching abilities
4. Only abilities on the **Player ASC** receive input (no automatic cross-ASC routing)

**Critical Discovery (Claude-GPT dual audit 2026-01-24):** Manifest used `Narrative.Input.Father.Ability1` but Narrative Pro's default mapping broadcasts `Narrative.Input.Ability1`. The `HasTagExact` check caused silent activation failure.

### Invariant INV-INPUT-ASC-1

> `input_tag` is valid **only** for abilities owned by the **Player ASC**.

**Reason:** Player input flows through `NarrativePlayerController` → Player ASC. Father's ASC never receives direct input events.

**Abilities on Father ASC** (e.g., `GA_FatherElectricTrap`) must use alternative activation:
- Relay from Player ability (`TryActivateAbilityByClass` on Father ASC)
- AI/Goal-driven activation
- Event-driven activation

### Invariant INV-INPUT-1

> All Player-input-triggered abilities MUST:
> 1. Be owned by Player ASC
> 2. Use Narrative Pro's built-in InputTags: `Narrative.Input.Ability1`, `Narrative.Input.Ability2`, `Narrative.Input.Ability3`
> 3. Be gated exclusively via `activation_required_tags` (NOT custom InputTag namespaces)

**Reason:** Narrative Pro ships with `DA_DefaultAbilityInputs` mapping Q→Ability1, E→Ability2, F→Ability3. Using custom namespaces (e.g., `Narrative.Input.Father.*`) creates tag mismatch with `HasTagExact`.

### Correct Pattern (Multiple Q Abilities)

```yaml
# All Q abilities share Narrative.Input.Ability1, differentiated by form tags
- name: GA_DomeBurst
  input_tag: Narrative.Input.Ability1
  activation_required_tags:
    - Father.Dome.Active
    - Father.Dome.FullyCharged

- name: GA_ProximityStrike
  input_tag: Narrative.Input.Ability1
  activation_required_tags:
    - Effect.Father.FormState.Symbiote
```

### Forbidden

- Using custom InputTag namespaces (e.g., `Narrative.Input.Father.*`) without custom `UNarrativeAbilityInputMapping`
- Setting `input_tag` on abilities owned by non-Player ASCs (e.g., Father ASC)
- Creating custom InputActions (e.g., `IA_FatherDomeBurst`) without adding them to the active InputMapping

### Validated Corrections (IMPLEMENTED v4.32)

| Ability | Owner ASC | input_tag | Status |
|---------|-----------|-----------|--------|
| GA_DomeBurst | Player | `Narrative.Input.Ability1` | ✅ Q, Armor-gated |
| GA_StealthField | Player | `Narrative.Input.Ability2` | ✅ E, Exoskeleton-gated |
| GA_FatherExoskeletonSprint | Player | `Narrative.Input.Ability1` | ✅ Q, Exoskeleton-gated |
| GA_ProximityStrike | Player | `Narrative.Input.Ability1` | ✅ Q, Symbiote-gated |
| GA_FatherElectricTrap | Father | **None** | ✅ Father ASC - requires relay |

### ⚠️ FUTURE ABILITIES NOTE (Erdem)

> **NOTE:** If new player-input-driven abilities are added in the future, they MUST follow INV-INPUT-1 (use `Narrative.Input.Ability{1|2|3}` + `activation_required_tags` gating). Review input mapping strategy before extension.

### Reference

- Narrative Pro: `NarrativePlayerController.cpp:39` (default mapping load), `NarrativePlayerController.cpp:160-165` (input binding loop)
- Narrative Pro: `NarrativeAbilitySystemComponent.cpp:285` (`HasTagExact` check)
- Narrative Pro: `NarrativeCharacter.cpp:791` (InputTag added to DynamicSpecSourceTags at grant)
- Audit: Claude-GPT dual audit session (2026-01-24)
- Implementation: v4.32

---

## LOCKED CONTRACT 16 — R-SPAWN-1 SpawnNPC-Only for NPC Spawning (v4.34)

### Context

NPCs in Narrative Pro require proper initialization via `NarrativeCharacterSubsystem.SpawnNPC()`. Raw `SpawnActor` bypasses critical initialization:
- `SetNPCDefinition()` not called → No abilities, activities, or faction tags
- Subsystem registration skipped → NPC invisible to AI systems
- AbilityConfiguration/ActivityConfiguration not applied → No GAS setup

**Discovery:** BP_WardenHusk and BP_BiomechHost phase transitions originally used `SpawnActor`, causing spawned NPCs (WardenCore, BiomechCreature) to be non-functional.

### Invariant

1. All NPC spawning in manifests **MUST** use `type: SpawnNPC` node (NOT `SpawnActor`)
2. `SpawnNPC` requires `npc_definition_variable` property referencing an `NPCDefinition` variable
3. The NPCDefinition variable must be typed as `Object` with `class: NPCDefinition`

### Correct Pattern

```yaml
variables:
  - name: PhaseSpawnDefinition
    type: Object
    class: NPCDefinition

event_graph:
  nodes:
    - id: SpawnCore
      type: SpawnNPC
      properties:
        npc_definition_variable: PhaseSpawnDefinition
```

### Forbidden

- Using `type: SpawnActor` for any NPC spawning
- Using `TSubclassOf` for NPCDefinition variables (use `Object` with `class: NPCDefinition`)
- Spawning NPCs without NPCDefinition reference

### Reference

- Manifest: `manifest.yaml` — BP_WardenHusk (lines 8921-8925), BP_BiomechHost (lines 9973-9977)
- Narrative Pro: `NarrativeCharacterSubsystem.h` — `SpawnNPC()` function
- Audit: Claude–GPT dual audit session (2026-01-25)
- Implementation: v4.34

---

## LOCKED CONTRACT 17 — R-PHASE-1 Two-Phase Death Transition Pattern (v4.34)

### Context

Multi-phase enemies (Warden Husk → Warden Core, Biomech Host → Biomech Creature) require proper death transition handling. The transition must:
1. Detect death via delegate binding
2. Check authority before spawning
3. Spawn replacement NPC via SpawnNPC
4. Complete parent death handling

### Invariant

1. Phase transition blueprints **MUST** bind to `OnDied` delegate from ASC
2. Death handler **MUST** check `HasAuthority` before spawning (multiplayer safety)
3. Spawn **MUST** use `SpawnNPC` (per R-SPAWN-1)
4. Handler **MUST** call parent death handling (`CallParent` or equivalent)

### Correct Pattern

```yaml
delegate_bindings:
  - source_variable: ASC
    delegate: OnDied
    handler: HandleDeath

event_graph:
  nodes:
    - id: Event_HandleDeath
      type: CustomEvent
      properties:
        event_name: HandleDeath
        parameters:
          - name: KilledActor
            type: Actor
          - name: KilledActorASC
            type: NarrativeAbilitySystemComponent
    - id: HasAuthority
      type: CallFunction
      properties:
        function: HasAuthority
        class: Actor
    - id: AuthBranch
      type: Branch
    - id: SpawnReplacement
      type: SpawnNPC
      properties:
        npc_definition_variable: PhaseSpawnDefinition
    - id: CallParent
      type: CallFunction
      properties:
        function: HandleDeath
        call_parent: true
```

### Forbidden

- Spawning replacement NPC without authority check
- Using SpawnActor for phase transition spawning
- Skipping parent death handling after spawn
- Binding to incorrect delegate (must be OnDied from ASC)

### Reference

- Manifest: `manifest.yaml` — BP_WardenHusk, BP_BiomechHost event graphs
- Narrative Pro: `NarrativeAbilitySystemComponent.h` — OnDied delegate signature
- Audit: Claude–GPT dual audit session (2026-01-25)
- Implementation: v4.34

---

## LOCKED CONTRACT 18 — R-DELEGATE-1 Delegate Binding CustomEvent Signature (v4.34)

### Context

The generator's `delegate_bindings` system auto-creates CustomEvent nodes for handlers. When manifest also defines explicit CustomEvent nodes for the same handler (to support event_graph connections), the CustomEvent **MUST** include parameters matching the delegate signature exactly.

**Discovery:** GA_ProtectiveDome failed compilation with "duplicate function names" when explicit CustomEvents had wrong/missing parameters vs delegate signature.

### Invariant

1. If manifest defines both `delegate_bindings` entry AND explicit `CustomEvent` node for same handler name:
   - CustomEvent parameters **MUST** match delegate signature exactly
   - Parameter names and types must match delegate's `FProperty` definitions
2. Generator will merge/validate, but manifest should be explicit to avoid timing issues

### Delegate Signatures (Narrative Pro)

| Delegate | Parameters |
|----------|------------|
| `OnDamagedBy` | `DamagerCauserASC: NarrativeAbilitySystemComponent`, `Damage: Float`, `Spec: GameplayEffectSpec` |
| `OnDied` | `KilledActor: Actor`, `KilledActorASC: NarrativeAbilitySystemComponent` |

### Correct Pattern

```yaml
delegate_bindings:
  - source_variable: ASC
    delegate: OnDamagedBy
    handler: HandleDamage

event_graph:
  nodes:
    - id: Event_HandleDamage
      type: CustomEvent
      properties:
        event_name: HandleDamage
        parameters:
          - name: DamagerCauserASC
            type: NarrativeAbilitySystemComponent
          - name: Damage
            type: Float
          - name: Spec
            type: GameplayEffectSpec
```

### Forbidden

- Defining CustomEvent with missing parameters (connections will fail to find pins)
- Defining CustomEvent with wrong parameter names (signature mismatch)
- Defining CustomEvent with wrong parameter types

### Reference

- Manifest: `manifest.yaml` — GA_ProtectiveDome (lines 4614-4626, 4727-4739)
- Generator: `GasAbilityGeneratorGenerators.cpp` — `GenerateDelegateBindingNodes()`
- Narrative Pro: `NarrativeAbilitySystemComponent.h` — delegate signatures
- Audit: Claude–GPT dual audit session (2026-01-25)
- Implementation: v4.34

---

## LOCKED CONTRACT 19 — R-NPCDEF-1 NPCDefinition Variable Type (v4.34)

### Context

NPCDefinition references in blueprints must use the correct variable type for proper SpawnNPC integration. `TSubclassOf<UNPCDefinition>` is **incorrect** — NPCDefinition is a DataAsset, not a class.

### Invariant

1. Variables storing NPCDefinition references **MUST** use:
   - `type: Object`
   - `class: NPCDefinition`
2. This produces a `TObjectPtr<UNPCDefinition>` variable in Blueprint
3. SpawnNPC node expects this type for `npc_definition_variable`

### Correct Pattern

```yaml
variables:
  - name: PhaseSpawnDefinition
    type: Object
    class: NPCDefinition
```

### Forbidden

- Using `type: TSubclassOf` for NPCDefinition (wrong — it's a DataAsset, not a class)
- Using `type: Class` for NPCDefinition
- Omitting `class: NPCDefinition` specification

### Reference

- Manifest: `manifest.yaml` — BP_WardenHusk, BP_BiomechHost variables
- Narrative Pro: `NPCDefinition.h` — inherits from UDataAsset
- Audit: Claude–GPT dual audit session (2026-01-25)
- Implementation: v4.34

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
| v4.32 | 2026-01-24 | Added Contract 14 — INV-INPUT-1 Input Architecture Invariants (Claude–GPT dual audit): input_tag valid only for Player ASC abilities; must use Narrative Pro built-in tags (Narrative.Input.Ability1/2/3); custom namespaces cause HasTagExact mismatch. Pending Erdem review for final ability-to-input mapping. |
| v4.33 | 2026-01-24 | Added Contract 15 — D-DEATH-RESET Player Death Reset System (Claude–GPT dual audit): PlayerASC.OnDied bindings for DomeEnergy/SymbioteCharge reset; silent reset (no burst/VFX); GA_Death unchanged (OnDied delegate for custom logic); D-SACRIFICE-1 one-shot bypass documented. |
| v4.34 | 2026-01-25 | Added Contracts 16-19 from NPC Guides audit (Claude–GPT dual audit): R-SPAWN-1 SpawnNPC-only for NPC spawning; R-PHASE-1 Two-phase death transition pattern; R-DELEGATE-1 Delegate binding CustomEvent signature matching; R-NPCDEF-1 NPCDefinition variable type (Object not TSubclassOf). |
