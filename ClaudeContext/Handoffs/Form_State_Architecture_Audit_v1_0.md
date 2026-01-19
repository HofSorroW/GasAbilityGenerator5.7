# Form State Architecture Audit - Complete Handover Document

**Version:** 1.0
**Date:** January 2026
**Status:** LOCKED
**Audit Type:** Claude + ChatGPT Cross-Validation

---

## Executive Summary

This document records the complete audit of the Father Companion form state architecture. The audit discovered a critical architectural flaw in the v4.0 Technical Reference that would cause form state to persist for 0 frames. The flaw has been corrected and a new architecture (Option B - GE-based persistent state) has been locked.

**Critical Finding:** The Technical Reference v6.0 contained 14+ sections describing an invalid pattern where `ActivationOwnedTags` were used for persistent form state. This pattern fails because GAS removes ActivationOwnedTags when an ability ends, regardless of replication settings.

**Resolution:** Option B architecture locked - form state persists via Infinite-duration GameplayEffects that grant form identity tags.

---

## Table of Contents

1. [Background and Context](#1-background-and-context)
2. [The v4.0 Architecture Flaw](#2-the-v40-architecture-flaw)
3. [Option B: Locked Architecture](#3-option-b-locked-architecture)
4. [Locked Rules and Invariants](#4-locked-rules-and-invariants)
5. [Audit Findings by Section](#5-audit-findings-by-section)
6. [Errata List](#6-errata-list)
7. [Missing Manifest Definitions](#7-missing-manifest-definitions)
8. [Automation Gap Roadmap](#8-automation-gap-roadmap)
9. [Evidence Package](#9-evidence-package)
10. [Implementation Checklist](#10-implementation-checklist)

---

## 1. Background and Context

### 1.1 System Overview

The Father Companion is a transformable spider companion with 5 forms:
- **Crawler** - Default, follows player, melee/laser attacks
- **Armor** - Attaches to chest, protective dome, damage absorption
- **Exoskeleton** - Attaches to back, speed boost, dash, stealth
- **Symbiote** - Merges with player, stat boosts, proximity damage
- **Engineer** - Turret mode, electric traps, auto-targeting

### 1.2 Form State Requirements

Each form requires:
1. **Identity Tag** - `Effect.Father.FormState.[Form]` - Identifies which form is active
2. **Stat Modifiers** - `GE_[Form]Boost` - Grants stat bonuses
3. **Invulnerability** (attached forms) - `Narrative.State.Invulnerable` tag
4. **Sub-Abilities** - Form-specific abilities granted to player

### 1.3 Audit Scope

- **Primary Document:** Father_Companion_Technical_Reference_v6_0.md
- **Secondary Documents:** manifest.yaml, ability guides, DefaultGameplayTags.ini
- **Validation Method:** Cross-validation between Claude and ChatGPT
- **Rules:** No coding until "GO", validate against UE5/GAS semantics, validate against web sources

---

## 2. The v4.0 Architecture Flaw

### 2.1 What v4.0 Claimed

The Technical Reference v4.0 architecture (carried into v6.0) stated:

> "RECOMMENDED PATTERN: End Ability After Setup Complete"
> "Activation Owned Tags grant form tag (Father.Form.*)"
> "Why this pattern works: Activation Owned Tags REPLICATE (ReplicateActivationOwnedTags enabled)"

**Source:** §11.9, Lines 1738-1794

### 2.2 Why This Is Wrong

**GAS Semantic Truth (verified against Epic UE5.7 documentation):**

| Property | Behavior |
|----------|----------|
| ActivationOwnedTags | Tags granted while ability EXECUTES |
| On EndAbility | Tags are REMOVED regardless of replication |
| ReplicateActivationOwnedTags | Replicates tag state to clients, does NOT prevent removal |

**The Failure Mode:**

```
1. GA_FatherArmor activates
2. ActivationOwnedTags grants Father.Form.Armor
3. Ability "ends after setup" (per §11.9 pattern)
4. EndAbility fires
5. GAS removes Father.Form.Armor ← FORM STATE LOST
6. Father has no form identity (0-frame state)
```

### 2.3 Historical Context

The v4.0 decision was based on a misunderstanding of `ReplicateActivationOwnedTags`:
- v3.6 architecture used GE-based state (correct)
- v4.0 "simplified" to ActivationOwnedTags (incorrect)
- The change was documented but the semantic flaw was not caught

### 2.4 Evidence of the Flaw

**§11.9 (Lines 1751-1755):**
```markdown
Why this pattern works (v4.0 architecture):
1. Activation Owned Tags REPLICATE (ReplicateActivationOwnedTags enabled)
```

**Epic Documentation (verified via web):**
> "Activation Owned Tags are granted to the owning actor when the ability is active and removed when the ability ends."

These two statements are incompatible. Replication does not prevent removal.

---

## 3. Option B: Locked Architecture

### 3.1 Architecture Decision

Three options were evaluated:

| Option | Description | Decision |
|--------|-------------|----------|
| A | Keep GA running (stance pattern) | Valid but not chosen |
| B | GE-based persistent form state | **LOCKED** |
| C | Loose tags via AddLooseGameplayTags | Rejected (NP-misaligned, replication-risky) |

**Option B was locked because:**
- Aligns with Narrative Pro patterns
- GEs persist across ability lifecycle
- Automatic replication via ASC
- Save/load compatible (ASC persists active effects)
- Regen-safe (deterministic state)

### 3.2 Option B Components

| Component | Purpose | Persistence |
|-----------|---------|-------------|
| `GE_[Form]State` | Grants form identity tag + invulnerability | Infinite duration |
| `GE_[Form]Boost` | Grants stat modifiers | Infinite duration |
| Form ability | Setup, apply GEs, then ends | Transient |

### 3.3 Form Transition Sequence (LOCKED)

```
NEW FORM ACTIVATEABILITY (Transition Prelude):
├─ Step 1: BP_RemoveGameplayEffectFromOwnerWithGrantedTags(Effect.Father.FormState.*)
├─ Step 2: Apply GE_[NewForm]State (grants Effect.Father.FormState.[NewForm])
├─ Step 3: Apply GE_[NewForm]Boost (stats)
├─ Step 4: Commit cooldown
├─ Step 5: Grant sub-abilities to player (if applicable)
├─ Step 6: Attach/detach as needed
└─ Step 7: Ability ends normally

OLD FORM ENDABILITY (bWasCancelled=true only):
├─ Step 1: Check bWasCancelled == true (form switch)
├─ Step 2: Cancel sub-abilities on player (two-step: Cancel + SetRemoveOnEnd)
├─ Step 3: Remove stat GEs from player (GE_[OldForm]Boost)
├─ Step 4: Restore player speed/stats
├─ Step 5: Detach if attached
└─ Step 6: Call Parent EndAbility
❌ FORBIDDEN: Never remove GE_[Form]State in EndAbility
```

### 3.4 Why EndAbility Must NOT Remove Form State

If the OLD form removes its GE_[Form]State in EndAbility:
1. Form state is removed
2. NEW form hasn't applied its state yet
3. Brief window with NO form state (0-frame problem recreated)
4. Race conditions in multiplayer

The NEW form is responsible for removing the PREVIOUS form's state GE as its first action.

---

## 4. Locked Rules and Invariants

### 4.1 Single Active Form State Invariant

> **LOCKED:** "Form switch must remove all prior `Effect.Father.FormState.*` GameplayEffects before applying the new one; no overlap unless a dedicated transition mechanism is explicitly documented."

**Clarifications:**
- Removal scope: Parent tag family `Effect.Father.FormState.*`
- Removal timing: Transition prelude (NEW form's ActivateAbility)
- Overlap: NOT allowed
- Rationale: Regen safety, correctness, single-source-of-truth identity

### 4.2 GE Split Rule

> **LOCKED:** GE naming convention must separate identity from stats.

| GE Type | Contains | Example |
|---------|----------|---------|
| `GE_[Form]State` | Form identity tag + invulnerability (if attached) | GE_ArmorState |
| `GE_[Form]Boost` | Stat modifiers only | GE_ArmorBoost |

**Never mix identity and stats in the same GE.**

### 4.3 Removal Targeting Rule

> **LOCKED:** Always remove previous state by granted tag family; handles are optional.

**Preferred Method:**
```
BP_RemoveGameplayEffectFromOwnerWithGrantedTags(Effect.Father.FormState.*)
```

**Why:**
- Single call removes any form state
- No need to track which form was previously active
- Regen-safe (deterministic)

### 4.4 Transition Prelude Rule

> **LOCKED:** Form switch must follow this sequence:
> 1. Remove previous state (by tag)
> 2. Apply new GE_[Form]State
> 3. Apply GE_[Form]Boost
> 4. Commit cooldown

### 4.5 Activation Tag Requirements

All form abilities must have:

| Tag Type | Tags |
|----------|------|
| Activation Required | `Father.State.Alive`, `Father.State.Recruited` |
| Activation Blocked | `Father.State.Dormant`, `Father.State.Transitioning`, `Cooldown.Father.FormChange` |

---

## 5. Audit Findings by Section

### 5.1 Five-Bucket Classification System

| Bucket | Description | Count |
|--------|-------------|-------|
| A | Architectural Contradictions (ERRATA) | 12 |
| B | Implicit Assumptions (Clarify) | 12 |
| C | Obsolete/Transitional Language (Deprecate) | 8 |
| D | Missing Rules (Add) | 15 |
| E | Automation Gaps | 16 |

### 5.2 Bucket A: Architectural Contradictions (ERRATA)

| # | Section | Issue | Required Action |
|---|---------|-------|-----------------|
| A1 | §11.9 (Lines 1738-1794) | "End Ability After Setup" with ActivationOwnedTags | ERRATA: Replace with GE-based pattern |
| A2 | Lines 365-366 | "ActivationOwnedTags REPLICATE" implies persistence | ERRATA: Clarify tags removed on EndAbility |
| A3 | Lines 1751-1755 | "Why this pattern works" rationale | DELETE or replace |
| A4 | Lines 359-372 | "Form State GEs: Optional" | ERRATA: GEs are REQUIRED |
| A5 | §19.1 (Lines 2476-2486) | v4.0 Architecture Overview | ERRATA: Replace with Option B |
| A6 | §19.2 (Lines 2487-2495) | Replication explanation | DELETE or rewrite |
| A7 | §19.3 (Lines 2497-2510) | Form Ability Configuration Template | Update for GE-based tags |
| A8 | §19.10 (Lines 2659-2672) | "Form state tags: Activation Owned Tags" | ERRATA: Replace with GEs |
| A9 | §30.2 (Line 3315) | "Form Change: Local Predicted" | ERRATA: Change to Server Only |
| A10 | §36.13 | Cleanup flows missing transition prelude | Add prelude documentation |
| A11 | §58.7 (Lines 7004-7013) | "Form Tags From: GA Activation Owned Tags" | ERRATA: Replace with GE_*State |
| A12 | §58.8 (Lines 7015-7023) | Architecture decision section | Update tag source |

### 5.3 Bucket B: Implicit Assumptions (Clarify)

| # | Section | Issue | Required Action |
|---|---------|-------|-----------------|
| B1 | §11.9 | "Apply stat effects" doesn't mention state GEs | Clarify separation |
| B2 | §11.9 | bWasCancelled pattern scope | Clarify: for non-tag cleanup only |
| B3 | §7.4 | Assumes form abilities work with "end after setup" | Note: pattern is errata |
| B4 | §13.5 | Notes ActivationOwnedTags removed on end | Add: GE-based state persists |
| B5 | §14.8 | Lists StateHandle variables | Clarify: requires GE_*State to exist |
| B6 | §19.5 | Symbiote "stays active" | Clarify: correct stance pattern |
| B7 | §34.16 | Lists *StateHandle variables | Clarify: assumes GE_*State exist |
| B8 | §37.5 | Missing activation tags | Add: Father.State.Alive, Father.State.Dormant |
| B9 | §38.3 | GE_*State invulnerability table | Clarify: requires GE creation |
| B10 | §39.4 | EndPlay cleanup lists *StateHandle | Clarify: assumes GE_*State exist |
| B11 | §57.7 | AC_FatherCompanion_Default missing StartupEffects | Add: GE_CrawlerState |
| B12 | §58.5 | Father Form Stat Mapping | Clarify: stat vs state separation |

### 5.4 Bucket C: Obsolete/Transitional Language (Deprecate)

| # | Section | Issue | Required Action |
|---|---------|-------|-----------------|
| C1 | Line 2 | "Unreal Engine 5.6" | Update to 5.7 |
| C2 | Line 385 | "UE 5.6 GAMEPLAY EFFECT NAVIGATION" | Update to 5.7 |
| C3 | Lines 1286-1398 | Multiple "5.6" references | Bulk replace |
| C4 | §14.1 | "v4.0 Architecture - EndAbility Pattern" | Rename to "Cleanup Pattern" |
| C5 | §19.1 title | "v4.0 Architecture Overview" | Add deprecation notice |
| C6 | §27.1 | GA_Sprint ActivationOwnedTags pattern | Clarify: input-held only |
| C7 | §54.8 | Custom BTTask references | Update to use NP built-in |
| C8 | §55.15 | Custom BT_FatherFollow | Mark as intentional design |

### 5.5 Bucket D: Missing Rules (Add)

| # | Section | Gap | Priority |
|---|---------|-----|----------|
| D1 | §10.1 | Missing FormState tag hierarchy | P2 |
| D2 | §19 | **GE_*State effect definitions** | **P1 - BLOCKER** |
| D3 | §11.9 | Missing GE removal pattern | P2 |
| D4 | §14 | Missing StateHandle variables | P3 |
| D5 | §19 | Missing GE_*State application step | P1 |
| D6 | §19.7 | Missing GE_*State removal for previous form | P1 |
| D7 | §14.8 | GE_*State effect definitions | **P1 - BLOCKER** |
| D8 | §20.4 | Missing GE_CrawlerState in StartupEffects | **P1 - BLOCKER** |
| D9 | §36.13 | Missing transition prelude step | P2 |
| D10 | §37.5 | Missing activation tag requirements | P2 |
| D11 | §38.3 | GE_*State definitions for invulnerability | **P1 - BLOCKER** |
| D12 | §31.9 | GameplayCue assets not in manifest | P3 |
| D13 | §57.7 | StartupEffects missing GE_CrawlerState | **P1 - BLOCKER** |
| D14 | §58.9 | HandleEquip integration clarification | P3 |
| D15 | §63.5 | Missing Rifle/Sword mode abilities | P3 |

### 5.6 Bucket E: Automation Gaps

| # | Description | Category | Notes |
|---|-------------|----------|-------|
| E1 | Niagara spawn | C (Enhancement) | Generator limitation, not engine |
| E2 | OnAttributeChangeDelegate binding | C (Major Feature) | Needs delegate IR |
| E3 | OnDied delegate binding | C (Major Feature) | Needs delegate IR |
| E4 | Attack Token configuration | B | Manual config |
| E5 | Load() override | C (Conditional) | Depends on NP persistence |
| E6 | Two-step cross-actor cleanup | B | Documented pattern |
| E7 | Symbiote 30-second timer | B | Ability-specific |
| E8 | Niagara VFX in form transition | C (Enhancement) | GameplayCues preferred |
| E9 | GC_TakeDamage implementation | B | Too specific |
| E10 | Camera Shake assets | C (Enhancement) | No factory currently |
| E11 | Dialogue event generation | C | Could add events section |
| E12 | NarrativeEvent ExecuteEvent graph | C | Could add execute_event_graph |
| E13 | AbilityTask_SpawnProjectile | C | Could add ability_task section |
| E14 | BTTask_ActivateAbilityByClass config | C | Could add bt_task_config |
| E15 | GoalGenerator_Attack config | B | Uses NP built-in |
| E16 | NPCActivitySchedule extensions | C | Could extend |

---

## 6. Errata List

### 6.1 Sections Requiring Complete Rewrite

| Section | Lines | Current Content | Required Change |
|---------|-------|-----------------|-----------------|
| §11.9 | 1738-1794 | "End Ability After Setup" pattern | Rewrite for GE-based form state |
| §19.1 | 2476-2486 | v4.0 Architecture (ActivationOwnedTags) | Rewrite for Option B |
| §19.2 | 2487-2495 | Replication explanation | DELETE or rewrite |
| §19.3 | 2497-2510 | Form Ability Configuration Template | Update for GE-based tags |
| §19.10 | 2659-2672 | Form State vs Stat Modification | Update form state source |
| §58.7 | 7004-7013 | Child GE vs Base GE Pattern | Update tag source |
| §58.8 | 7015-7023 | Child GE Architecture Decision | Update tag source |

### 6.2 Sections Requiring Minor Updates

| Section | Issue | Fix |
|---------|-------|-----|
| Multiple | "UE 5.6" | Change to "UE 5.7" |
| §30.2 | Form Change "Local Predicted" | Change to "Server Only" |
| §36.13 | Missing prelude context | Add reference to transition prelude |
| §37.5 | Missing tags | Add Father.State.Alive, Dormant |
| §63.4 | StartupEffects empty | Add GE_CrawlerState |

---

## 7. Missing Manifest Definitions

### 7.1 GE_*State Effects (CRITICAL - P1)

These GameplayEffects must be added to manifest.yaml:

```yaml
gameplay_effects:
  # Form State Effects - Grant identity tags + invulnerability
  - name: GE_CrawlerState
    folder: Effects/FormState
    duration_policy: Infinite
    granted_tags:
      - Effect.Father.FormState.Crawler
    # No invulnerability - Crawler is vulnerable

  - name: GE_ArmorState
    folder: Effects/FormState
    duration_policy: Infinite
    granted_tags:
      - Effect.Father.FormState.Armor
      - Narrative.State.Invulnerable

  - name: GE_ExoskeletonState
    folder: Effects/FormState
    duration_policy: Infinite
    granted_tags:
      - Effect.Father.FormState.Exoskeleton
      - Narrative.State.Invulnerable

  - name: GE_SymbioteState
    folder: Effects/FormState
    duration_policy: Infinite
    granted_tags:
      - Effect.Father.FormState.Symbiote
      - Narrative.State.Invulnerable

  - name: GE_EngineerState
    folder: Effects/FormState
    duration_policy: Infinite
    granted_tags:
      - Effect.Father.FormState.Engineer
    # No invulnerability - Engineer turret is vulnerable
```

### 7.2 StartupEffects Update (CRITICAL - P1)

AC_FatherCompanion_Default must include:

```yaml
ability_configurations:
  - name: AC_FatherCompanion_Default
    folder: AbilityConfigs
    startup_effects:
      - GE_CrawlerState  # Default form state at spawn
    default_attributes: GE_DefaultNPCAttributes
    abilities:
      - GA_FatherCrawler
      - GA_FatherArmor
      - GA_FatherExoskeleton
      - GA_FatherSymbiote
      - GA_FatherEngineer
      - GA_FatherAttack
      - GA_FatherLaserShot
      - GA_FatherMark
      - GA_FatherSacrifice
      - GA_Death
```

### 7.3 Activation Tag Updates (P2)

All form abilities need updated tags:

```yaml
# Example for GA_FatherArmor
- name: GA_FatherArmor
  activation_required_tags:
    - Father.State.Alive
    - Father.State.Recruited
  activation_blocked_tags:
    - Father.State.Dormant
    - Father.State.Transitioning
    - Father.State.SymbioteLocked
    - Cooldown.Father.FormChange
```

---

## 8. Automation Gap Roadmap

### 8.1 Category Classification

| Category | Description | Action |
|----------|-------------|--------|
| A | Never automate | Manual by design |
| B | Not worth complexity | Documented workaround |
| C | Should add to generator | Roadmap item |
| D | Blocked by engine | Cannot automate |

### 8.2 Category C Roadmap (Prioritized)

| Priority | Item | Description | Effort |
|----------|------|-------------|--------|
| **P1** | GE_*State definitions | Add to manifest, generator supports | Low |
| **P1** | StartupEffects | Manifest change only | Low |
| **P1** | Activation tags | Manifest change only | Low |
| P2 | Delegate binding IR | New manifest section + codegen | High |
| P2 | Load override | New event_load section | Medium |
| P2 | Dialogue events | Add events section | Medium |
| P3 | AbilityTask nodes | Add ability_task section | Medium |
| P3 | BTTask config | Add bt_task_config section | Medium |
| P3 | GameplayCue wiring | Trigger automation | Medium |
| P3 | Niagara spawn | Add module deps (optional) | High |

### 8.3 Generator Capability Verification

| Capability | Status | Evidence |
|------------|--------|----------|
| GE creation | ✅ Supported | FGameplayEffectGenerator |
| GrantedTags | ✅ Supported | GasAbilityGeneratorTypes.h:694 |
| Infinite duration | ✅ Supported | Parser handles duration_policy |
| Startup effects | ✅ Supported | AbilityConfiguration generator |
| Tag removal by granted | ✅ Supported | WellKnownFunctions table |

---

## 9. Evidence Package

### 9.1 §36.13 - Assumes ActivationOwnedTags

**Lines 4332-4339:**
```markdown
### 36.13) Form EndAbility Cleanup Flow

**CRITICAL: All cleanup must be gated by bWasCancelled check (see Section 11.9)**

Each form ability's Event EndAbility must:
1. Check bWasCancelled output pin
2. Only run cleanup when bWasCancelled = TRUE (form switch)
3. Skip cleanup when bWasCancelled = FALSE (normal ability end after setup)
```

**Analysis:** References §11.9 (errata). "Normal ability end after setup" implies form state persists after EndAbility via ActivationOwnedTags.

### 9.2 §63.4 - Missing StartupEffects

**Lines 7391-7397:**
```markdown
### 63.4) AC_FatherCompanion_Default Template

| Property | Value |
|----------|-------|
| Default Attributes | GE_DefaultNPCAttributes |
| Startup Effects | 0 Array element |
| Default Abilities | 10 Array elements |
```

**Analysis:** StartupEffects is empty. Must contain GE_CrawlerState for default form at spawn.

### 9.3 §38.3 - Invulnerability via Tags

**Lines 4564-4575:**
```markdown
### 38.3) Invulnerability Implementation

GE State effects grant invulnerability:

| GE | Grants Invulnerable |
|----|---------------------|
| GE_CrawlerState | No |
| GE_ArmorState | Yes (Narrative.State.Invulnerable) |
| GE_ExoskeletonState | Yes (Narrative.State.Invulnerable) |
| GE_SymbioteState | Yes (Narrative.State.Invulnerable) |
| GE_EngineerState | No |
```

**Analysis:** Confirms invulnerability is tag-based. GE_*State effects expected but not defined in manifest.

### 9.4 §30.2 vs §7.4 - Net Execution Contradiction

**§30.2 (Line 3315):**
```markdown
| Form Change | Local Predicted | Player-triggered |
```

**§7.4 (Line 942):**
```markdown
| Form Abilities (grants to player) | Father (NPC) | Server Only | Cross-actor grants require server authority |
```

**Analysis:** Direct contradiction. §7.4 is correct - form abilities perform cross-actor operations.

---

## 10. Implementation Checklist

### 10.1 Phase 1: Manifest Changes (P1 Blockers)

- [ ] Add GE_CrawlerState to manifest
- [ ] Add GE_ArmorState to manifest
- [ ] Add GE_ExoskeletonState to manifest
- [ ] Add GE_SymbioteState to manifest
- [ ] Add GE_EngineerState to manifest
- [ ] Add GE_CrawlerState to AC_FatherCompanion_Default StartupEffects
- [ ] Update form ability activation_required_tags (add Father.State.Alive)
- [ ] Update form ability activation_blocked_tags (add Father.State.Dormant)

### 10.2 Phase 2: Ability Guide Updates

- [ ] Update GA_FatherArmor guide with transition prelude
- [ ] Update GA_FatherExoskeleton guide
- [ ] Update GA_FatherSymbiote guide
- [ ] Update GA_FatherEngineer guide
- [ ] Update GA_FatherCrawler guide

### 10.3 Phase 3: Technical Reference Updates

- [ ] Rewrite §11.9 for GE-based pattern
- [ ] Rewrite §19.1-19.3, §19.10 for Option B
- [ ] Update §30.2 (ServerOnly for form change)
- [ ] Update §36.13 with transition prelude context
- [ ] Update §37.5 with complete tag list
- [ ] Update §58.7-58.8 for GE-based tags
- [ ] Add "Form State Law" canonical rules section
- [ ] Update version references (5.6 → 5.7)

### 10.4 Phase 4: Verification

- [ ] Run manifest generation
- [ ] Verify GE_*State assets created
- [ ] Verify GrantedTags applied correctly
- [ ] Test form transition sequence
- [ ] Verify single form state invariant

---

## Appendix A: Glossary

| Term | Definition |
|------|------------|
| ActivationOwnedTags | Tags granted while ability executes, removed on EndAbility |
| GE | GameplayEffect |
| ASC | AbilitySystemComponent |
| Transition Prelude | First steps of NEW form's ActivateAbility (remove old state, apply new) |
| Form State GE | GE_*State - grants form identity tag |
| Form Stat GE | GE_*Boost - grants stat modifiers |
| Option B | GE-based persistent form state architecture (LOCKED) |

## Appendix B: Tag Reference

| Tag | Purpose | Source |
|-----|---------|--------|
| Effect.Father.FormState.Crawler | Crawler form identity | GE_CrawlerState |
| Effect.Father.FormState.Armor | Armor form identity | GE_ArmorState |
| Effect.Father.FormState.Exoskeleton | Exoskeleton form identity | GE_ExoskeletonState |
| Effect.Father.FormState.Symbiote | Symbiote form identity | GE_SymbioteState |
| Effect.Father.FormState.Engineer | Engineer form identity | GE_EngineerState |
| Narrative.State.Invulnerable | Blocks all damage | GE_*State (attached forms) |
| Father.State.Alive | Form activation prereq | Eligibility gate |
| Father.State.Dormant | Blocks form activation | Eligibility gate |

---

**Document Status:** LOCKED
**Last Updated:** January 2026
**Validated By:** Claude + ChatGPT cross-validation audit
