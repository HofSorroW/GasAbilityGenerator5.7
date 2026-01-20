# Father Companion GAS Audit - Locked Decisions
## Version 2.0 - January 2026

**Purpose:** This document consolidates all validated findings and locked decisions from the dual-agent audit (Claude-GPT) conducted January 2026. These decisions are LOCKED and should not be debated again.

**Audit Context:** UE5.7 + Narrative Pro v2.2 + GasAbilityGenerator v4.14.x

**v2.0 Updates:** Added EndAbility lifecycle rules, complete severity matrix, orphan connection findings, and Track A/B implementation strategy from dual-agent verification session.

---

## LOCKED CONSTRAINTS (Implementation Boundaries)

### LC-1: No Manual Blueprint Edits
**Status:** LOCKED
**Source:** User requirement (audit session)

After GasAbilityGenerator generates assets, NO manual Blueprint edits are allowed. All fixes must go through:
1. Manifest.yaml updates
2. Generator enhancement (if new node types needed)
3. Re-generation via commandlet

**Rationale:** Ensures reproducibility, enables CI/CD, prevents configuration drift.

---

### LC-2: No UE Source Modification
**Status:** LOCKED
**Source:** User requirement (audit session)

Cannot modify:
- Unreal Engine source code
- Narrative Pro plugin C++ code
- Any engine-level headers

**Rationale:** Project uses stock engine and plugin binaries.

---

### LC-3: No C++ GameplayAbility Implementation
**Status:** LOCKED
**Source:** User requirement (audit session)

All GameplayAbility logic must be Blueprint-only, generated via manifest.yaml. No custom C++ ability classes.

**Allowed:**
- Enhance GasAbilityGenerator C++ (node generators)
- Use existing Blueprint nodes from UE/Narrative Pro

---

### LC-4: Process Lock (Research -> Audit -> Decide -> Implement)
**Status:** LOCKED
**Source:** Audit methodology

All changes must follow:
1. **Research** - Investigate UE5.7 source, validate claims
2. **Audit** - Cross-reference against docs, challenge assumptions
3. **Decide** - Make architectural decision with sources
4. **Implement** - Update manifest/generator, regenerate

No jumping directly to implementation without research validation.

---

## INVULNERABILITY DECISION

### INV-1: Remove ALL Invulnerability Except GA_FatherSacrifice
**Status:** LOCKED
**Source:** User decision (audit session)
**Date:** January 2026

**Remove:**
- GE_TransitionInvulnerability (5s during form transitions)
- GE_DashInvulnerability (dash i-frames)
- GE_Invulnerable
- Narrative.State.Invulnerable from all GE_*State effects
- State.Invulnerable tag references in Father system

**KEEP:**
- GA_FatherSacrifice: 8-second PLAYER invulnerability (not Father)
- This is the only intentional invulnerability in the design

**Rationale:** Invulnerability was NEVER intended in the Father design - it was an oversight discovered during audit. Form transitions should be vulnerable.

---

## ENDABILITY LIFECYCLE RULES (CANONICAL)

### Rule 1 — Instant Abilities
**Status:** LOCKED
**Examples:** GA_FatherAttack, GA_DomeBurst, GA_ProximityStrike, GA_TurretShoot, GA_Backstab, GA_FatherElectricTrap, GA_FatherLaserShot

**Requirements:**
- ✅ Call `K2_EndAbility` at end of activation flow
- ❌ `Event_EndAbility` NOT required
- ❌ No delays, timers, or persistent state

**Pattern:**
```
Event_Activate → Logic → CommitCooldown → K2_EndAbility
```

---

### Rule 2 — Abilities with Delay or Timer
**Status:** LOCKED
**Examples:** All form abilities (GA_FatherCrawler, GA_FatherArmor, GA_FatherExoskeleton, GA_FatherSymbiote, GA_FatherEngineer), GA_FatherExoskeletonDash, GA_StealthField

**Requirements:**
- ✅ MUST have `Event_EndAbility` with `bWasCancelled` check
- ✅ MUST clean up persistent state (timers, GE handles)
- ✅ MUST prevent post-delay execution after cancellation

**How to prevent post-delay execution:**
- **Preferred (Track B):** WaitDelay AbilityTask (auto-cancels)
- **Temporary (Track A):** Explicit guards after Delay node

**Guard Pattern (3-layer):**
```
After Delay:
  → IsValid(ActorRef) → Branch
    True → CurrentForm == ExpectedForm → Branch
      True → HasMatchingGameplayTag(ExpectedTag) → Branch
        True → [Execute post-delay logic]
        False → Return (ability already ended)
```

**Violation = race condition + corrupted state**

---

### Rule 3 — Toggle / Persistent Abilities
**Status:** LOCKED
**Examples:** GA_ProtectiveDome, GA_FatherExoskeletonSprint, GA_FatherRifle, GA_FatherSword

**Requirements:**
- ❌ Do NOT call `K2_EndAbility` on activation
- ✅ MUST have `Event_EndAbility` for cleanup
- ✅ Stay active until cancelled externally

**Pattern:**
```
Event_Activate → Setup state → [ability stays active]

Event_EndAbility → Cleanup state (remove GE, restore values)
```

---

## SEVERITY MATRIX (LOCKED)

### 🔴 CRITICAL — Must Fix Before Build

| Ability | Issue | Lines |
|---------|-------|-------|
| GA_FatherExoskeletonDash | Dead nodes referencing removed GE_DashInvulnerability (MakeInvulnSpec, ApplyInvuln) | 3947-4036 |
| GA_FatherSacrifice | Incomplete flow - ends at SetIsMonitoring, missing invuln/dormant/EndAbility logic | 4900-4989 |
| GA_CoreLaser | No event_graph defined | 7068-7078 |
| GA_FatherCrawler | No Event_EndAbility + ORPHAN CONNECTIONS referencing non-existent nodes | 785-1084 |

### 🟡 MEDIUM — Race Conditions

| Ability | Issue | Notes |
|---------|-------|-------|
| GA_FatherArmor | Guard (Branch_Valid) executes AFTER GE operations | Ineffective placement |
| GA_FatherExoskeleton | No post-delay guards | Has Event_EndAbility |
| GA_FatherEngineer | No post-delay guards | Has Event_EndAbility |
| GA_StealthField | Has Delay (8s), no guards | Miscategorized as toggle |
| GA_FatherRifle | No guards in Event_EndAbility | Persistent ability |
| GA_FatherSword | No guards in Event_EndAbility | Persistent ability |

### 🟢 CORRECT — Gold Standard

| Ability | Pattern | Notes |
|---------|---------|-------|
| GA_FatherSymbiote | 3-layer guards + Event_EndAbility + bWasCancelled | Reference implementation |
| GA_FatherAttack | Instant (Rule 1) | No Event_EndAbility needed |
| GA_TurretShoot | Instant (Rule 1) | No Event_EndAbility needed |
| GA_ProximityStrike | Instant (Rule 1) | No Event_EndAbility needed |
| GA_DomeBurst | Instant (Rule 1) | No Event_EndAbility needed |
| GA_ProtectiveDome | Toggle (Rule 3) | Event_EndAbility for cleanup |
| GA_FatherExoskeletonSprint | Toggle (Rule 3) | Event_EndAbility for cleanup |

---

## CRITICAL DEFECT DETAILS

### CRIT-1: GA_FatherExoskeletonDash Dead Nodes
**Status:** CRITICAL BLOCKER
**Location:** manifest.yaml lines 3947-4036

The ability references removed GE_DashInvulnerability via:
- Node `MakeInvulnSpec` (line 3947)
- Node `ApplyInvuln` (line 3953)
- Connections at lines 4008-4011, 4034-4036

**Fix Required:**
1. Remove MakeInvulnSpec and ApplyInvuln nodes
2. Rewire: `SetPlayerRef → LaunchCharacter → Delay → CommitCooldown → EndAbility`

---

### CRIT-2: GA_FatherSacrifice Incomplete Flow
**Status:** CRITICAL BLOCKER
**Location:** manifest.yaml lines 4900-4989

Current flow ends at `SetIsMonitoring` with no actual sacrifice logic.

**Missing (per design):**
1. Apply GE_SacrificeInvulnerability to PLAYER (8 seconds)
2. Hide Father actor
3. Add Father.State.Dormant tag
4. Start dormant timer
5. Timer callback: Unhide Father, remove dormant tag
6. Call K2_EndAbility

---

### CRIT-3: GA_CoreLaser No Event Graph
**Status:** CRITICAL BLOCKER
**Location:** manifest.yaml lines 7068-7078

GA_CoreLaser has only tags definition, no event_graph section. Cannot activate.

**Fix Required:** Either:
- A) Implement minimal activation graph (instant attack pattern)
- B) Remove from manifest if unused

---

### CRIT-4: GA_FatherCrawler Orphan Connections
**Status:** CRITICAL BLOCKER (NEW in v2.0)
**Location:** manifest.yaml lines 1079-1083

Connections reference **non-existent nodes**:
```yaml
- from: [MakeLiteralTag_Transition, ReturnValue]
  to: [MakeTagContainer_Transition, Tag]
- from: [MakeTagContainer_Transition, ReturnValue]
  to: [RemoveTransitionInvuln, WithGrantedTags]
```

Grep confirms these nodes do NOT exist anywhere in manifest:
- `id: MakeLiteralTag_Transition` - **NO MATCHES**
- `id: MakeTagContainer_Transition` - **NO MATCHES**
- `id: RemoveTransitionInvuln` - **NO MATCHES**

**Fix Required:**
1. Remove orphan connections (lines 1079-1083)
2. Add Event_EndAbility handler
3. Add post-delay guards (following Symbiote pattern)

---

## VALIDATED TECHNICAL FINDINGS

### VTF-1: Delay vs Wait Delay (AbilityTask)
**Status:** VALIDATED
**Sources:**
- [UE5 Gameplay Ability Tasks Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-tasks-in-unreal-engine)
- [GAS Documentation (tranek)](https://github.com/tranek/GASDocumentation)

| Node Type | Behavior | Protection Layers |
|-----------|----------|-------------------|
| **Delay** | Does NOT terminate when ability ends | 0 (continues running) |
| **Wait Delay (AbilityTask)** | Auto-terminates on EndAbility | 3 layers |

**Wait Delay 3-Layer Protection:**
1. **TaskOwnerEnded()** - Task destroyed when owning ability ends
2. **Delegate Auto-Unbinding** - UObject destruction cleans up delegates
3. **ShouldBroadcastAbilityTaskDelegates()** - Checks `Ability->IsActive()` before firing

**Quote from UE5 docs:**
> "AbilityTasks... will be automatically ended on UGameplayAbility::EndAbility"

---

### VTF-2: UK2Node_LatentAbilityCall
**Status:** VALIDATED
**Source:** [GAS Documentation](https://github.com/tranek/GASDocumentation)

**Quote:**
> "We have code in K2Node_LatentAbilityCall to make using these in blueprints streamlined."

AbilityTask Blueprint nodes use `UK2Node_LatentAbilityCall` (in GameplayAbilitiesEditor module), not standard `UK2Node_CallFunction`.

**Current Generator Limitation:**
- Missing: GameplayAbilitiesEditor dependency
- Cannot generate WaitDelay AbilityTask nodes

---

### VTF-3: ActivationOwnedTags Auto-Removal
**Status:** VALIDATED
**Source:** UE5.7 GameplayAbility.cpp (line 870)

ActivationOwnedTags are automatically removed when EndAbility() is called.

**Implication:** Tags like `Symbiote.State.Merged` can be used as guards - if ability ended, tag is gone.

---

### VTF-4: Activation Required Tags NOT Continuously Checked
**Status:** VALIDATED
**Source:** UE5.7 GameplayAbility.cpp

Activation Required Tags are ONLY checked at activation time, NOT re-checked during execution.

**Solution:** Use `Cancel Abilities with Tag` on form abilities to cascade-cancel dependent abilities.

---

### VTF-5: No IsActive() Blueprint Node
**Status:** VALIDATED
**Source:** UE5.7 GameplayAbility.h (line 235)

`IsActive()` is NOT exposed to Blueprint (no UFUNCTION macro).

**Workaround:** Use ActivationOwnedTags as proxy via `HasMatchingGameplayTag()`.

---

## IMPLEMENTATION STRATEGY (LOCKED)

### Track A — Immediate Manifest Fixes
**Status:** APPROVED
**Timeline:** Now

Fix all CRITICAL blockers via manifest.yaml updates:
1. GA_FatherCrawler - Remove orphan connections, add Event_EndAbility + guards
2. GA_FatherExoskeletonDash - Remove dead invuln nodes, rewire flow
3. GA_FatherSacrifice - Implement full sacrifice logic
4. GA_CoreLaser - Implement or remove

Then fix MEDIUM issues:
5. Add guards to GA_FatherArmor (move before GE ops)
6. Add guards to GA_FatherExoskeleton, GA_FatherEngineer
7. Add guards to GA_StealthField
8. Add guards to GA_FatherRifle, GA_FatherSword

**This is defensive, not ideal.**

---

### Track B — Generator Enhancement (Future)
**Status:** APPROVED FOR FUTURE
**Timeline:** After Track A complete

Enhance GasAbilityGenerator to support AbilityTask nodes:
1. Add GameplayAbilitiesEditor dependency
2. Implement `UK2Node_LatentAbilityCall` node generation
3. Add new manifest node type: `type: AbilityTaskWaitDelay`
4. Replace all Delay nodes with WaitDelay in form abilities

**This restores true GAS lifecycle safety.**

---

## FORM STATE ARCHITECTURE (Option B - VALIDATED)

### Architecture Summary
**Status:** VALIDATED and IMPLEMENTED

| Component | Description |
|-----------|-------------|
| GE_*State | Infinite-duration GE grants form identity tag |
| ActivationOwnedTags | Ephemeral state (e.g., `Symbiote.State.Merged`) |
| Transition Prelude | Remove old GE_*State via parent tag, apply new GE_*State |

**Form Identity Tags:**
- `Effect.Father.FormState.Crawler`
- `Effect.Father.FormState.Armor`
- `Effect.Father.FormState.Exoskeleton`
- `Effect.Father.FormState.Symbiote`
- `Effect.Father.FormState.Engineer`

**Removal Pattern:**
```yaml
# Remove ALL prior form states (parent tag match)
- BP_RemoveGameplayEffectFromOwnerWithGrantedTags
  Tags: Effect.Father.FormState  # Parent removes all children
```

---

## GA_FatherSymbiote — GOLD STANDARD REFERENCE

### 3-Layer Guard Pattern (lines 2348-2400)

```yaml
# Guard 1: Validate FatherRef
- id: Branch_FatherValid
  type: Branch
  # Condition: IsValid(FatherRef)

# Guard 2: Check CurrentForm == Symbiote
- id: Branch_FormCheck
  type: Branch
  # Condition: CurrentForm == E_FatherForm::Symbiote

# Guard 3: Check Symbiote.State.Merged tag
- id: Branch_MergedTag
  type: Branch
  # Condition: HasMatchingGameplayTag(Symbiote.State.Merged)
```

### Event_EndAbility Handler (lines 2435-2490)

```yaml
- id: Event_EndAbility
  type: Event
  properties:
    event_name: K2_OnEndAbility

- id: Branch_WasCancelled
  type: Branch
  # Condition: bWasCancelled
  # True → Cleanup (clear timer, show Father, activate Armor)
  # False → Normal end (already cleaned up)
```

**All form abilities should follow this pattern.**

---

## DOCUMENT CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-20 | Initial creation from dual-agent audit session |
| 2.0 | 2026-01-21 | Added EndAbility rules, complete severity matrix, orphan connections finding, Track A/B strategy, GA_StealthField reclassification |

---

**END OF LOCKED DECISIONS DOCUMENT**
