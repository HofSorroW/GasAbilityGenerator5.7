# Phase 7 Patch-Plan — Father Combat Abilities (NO CODE, Diff‑Gradeable)

**Version:** 1.0B (Audit-Corrected)
**Date:** January 2026
**Revision:** Audit corrections applied per dual-auditor challenge/response cycle
**Scope:** GA_FatherLaserShot, GA_FatherElectricTrap, GA_FatherMark
**Source Inputs:** Phase 6.2 checklist + Phase 7 remediation specs in the Dual-Auditor Handoffs.
**Prime Directive:** No coding, no design changes; contract hierarchy applies (LOCKED > Guide > Manifest).

---

## 0) How to use this patch-plan

This document is a **spec-only** implementation plan. Each patch item is written so an implementer can:
- locate the exact spot in the guide/manifest/BP
- apply a discrete change
- verify acceptance criteria
- mark the item as PASS/FAIL during review

**Important:** This plan deliberately avoids YAML/code blocks. Where "replace node" is stated, it is a *specification of intent*, not a final syntax.

---

## 1) Dependency order (must follow)

### P0 (Blockers)
1. **P0-A** Guide correction — ElectricTrap damage pattern (Contract 24 compliance)
2. **P0-B** Generator enhancement — AbilityTask_SpawnProjectile + delegate outputs + safe wiring order

> **Note (v1.0B):** P0-A.2 (version footer reconciliation) reclassified to P2 — it is documentation hygiene, not a blocker.

### P1 (Ability remediations)
3. **P1-LaserShot** Apply task pattern + delegate handlers + move commit/end into callbacks + add gates
4. **P1-ElectricTrap** Fix TrapClass type + apply task pattern + deployed gate + required mechanics
5. **P1-Mark** Add minimum tag gates + lifecycle/UI/tracking/cleanup systems (extends existing SetByCaller flow)

### P2 (Polish / consistency)
6. Tag naming consistency (cooldown tag hierarchy), guide footer version consistency, minor doc hygiene

---

## 2) P0-A — Guide correction patch (ElectricTrap damage)

### Patch Item P0-A.1 — Remove forbidden SetByCaller damage modifier (Contract 24)

- **What to change**
  - In *GA_FatherElectricTrap implementation guide*, remove the line that defines damage magnitude as **Set By Caller** for a GE that uses **NarrativeDamageExecCalc**.
  - Replace with guidance that damage is sourced from **Father AttackDamage attribute** and mitigated by target Armor.

- **Where (exact target)**
  - Guide file: `GA_FatherElectricTrap_Implementation_Guide_v3_1.md`
  - Section: **2.3** (GE_ElectricWebDamage configuration)
  - Problem statement is already documented in the handoff (Contract 24 contradiction).

- **Why (contract rationale)**
  - Contract 24 (D-DAMAGE-ATTR-1) locks that **NarrativeDamageExecCalc ignores SetByCaller for damage** and uses captured attributes (AttackDamage/Armor).
  - This makes the guide self-contradictory unless corrected.

- **Acceptance criteria**
  1. The guide no longer instructs: "Magnitude Type: Set By Caller" for damage when NarrativeDamageExecCalc is used.
  2. The guide explicitly states: "Damage derived from AttackDamage attribute (Engineer form state sets it), mitigated by target Armor."
  3. The guide's "damage GE" description is compatible with existing locked architecture (no SetByCaller damage tags).

---

### Patch Item P0-A.2 — Guide version footer reconciliation (documentation consistency)

- **What to change**
  - Update the guide's version markers so header / doc-info table / footer are consistent.

- **Where**
  - Same ElectricTrap guide (header says v3.2; doc info says v3.1; footer says v2.8 per audit).

- **Why**
  - Reduces future audit ambiguity; prevents implementers from using wrong version.

- **Acceptance criteria**
  1. Header version, doc-info version, and footer version agree on the same version string.
  2. Change log/note includes reason: "Contract 24 correction; version normalization."

---

## 3) P0-B — Generator enhancement patch (AbilityTask_SpawnProjectile pattern)

> Goal: enable guide-required sequencing inside GAs using Narrative Pro's `UAbilityTask_SpawnProjectile`, including delegate handlers **OnTargetData** and **OnDestroyed**, while preserving the LOCKED "delegate wiring safety" invariant.

### Patch Item P0-B.1 — Add new manifest node type: AbilityTask_SpawnProjectile

- **What to change**
  - Extend generator's manifest schema to support a new node type representing the Narrative Pro task factory call:
    - Task factory function: **SpawnProjectile**
    - Inputs: OwningAbility (self), TaskInstanceName, Class (NarrativeProjectile subclass), ProjectileSpawnTransform
    - Outputs: Task object reference + exec continuation
    - Delegates: OnTargetData → handler event; OnDestroyed → handler event

- **Where**
  - Generator's "node type registry / parser" that currently recognizes SpawnActor nodes.

- **Why**
  - Contract 25 requires matching the guide-required task/delegate sequencing; SpawnActor fallback cannot express "OnTargetData → apply damage → MarkEnemy → end".

- **Acceptance criteria**
  1. Manifest can express AbilityTask_SpawnProjectile as a first-class node without "SpawnActor fallback".
  2. The generator emits a task factory call with correct pin types (Class must be NarrativeProjectile subclass).
  3. The task node provides delegate binding points for OnTargetData and OnDestroyed.

---

### Patch Item P0-B.2 — Delegate handler generation for task delegates (signature-correct)

- **What to change**
  - Generator must be able to emit handler CustomEvents for:
    - OnTargetData
    - OnDestroyed
  - Each handler must have parameters matching **FProjectileTargetDataDelegate** signature.

- **Where**
  - Generator's "CustomEvent creation" logic.

- **Why**
  - Locked delegate contracts require exact signature matching; mismatches can crash or refuse to compile.

- **Acceptance criteria**
  1. Handler CustomEvents exist with the correct parameter list/type.
  2. The handler nodes are placed deterministically and are referenced by the delegate binding step.
  3. Blueprint compiles (no pin type errors, no missing parameter errors).

---

### Patch Item P0-B.3 — Enforce safe wiring order (skeleton-sync-before-delegate invariant)

- **What to change**
  - Generator must enforce this sequence whenever it creates delegate bindings:
    1) Create CustomEvents
    2) Force skeleton sync / compile boundary as required by your locked workflow
    3) Create Delegate / Bind wiring
    4) Connect exec flow continuation

- **Where**
  - Generator's "emit delegates" pipeline and any "post-graph" finalize step.

- **Why**
  - Prior sessions established that wrong ordering can create crash-on-open assets or invalid skeleton pin resolution.

- **Acceptance criteria**
  1. Delegate binding is never emitted before its handler CustomEvent exists in skeleton.
  2. Generated assets open without crash and compile cleanly.
  3. Runtime: OnTargetData and OnDestroyed fire and reach the correct handler graphs.

- **Notes / Investigation hook**
  - This ties to Open Investigation OI-1: confirm if this invariant is already implemented; if yes, scope reduces to "task support only".

---

### Patch Item P0-B.4 — Reusable "task + delegates" pattern library entry (non-optional)

- **What to change**
  - Add a reusable internal "pattern template" for task-spawn-projectile sequences so multiple GAs can use it without bespoke logic.

- **Where**
  - Generator's pattern library (or equivalent abstraction).

- **Why**
  - Prevents regression: future projectile abilities shouldn't regress to SpawnActor fallback.

- **Acceptance criteria**
  1. Pattern can be invoked for both LaserShot and ElectricTrap.
  2. Pattern supports per-ability handler names and downstream chain insertion.

---

## 4) P1-LaserShot — Patch items (manifest/graph + tags)

### Context (truth state)
LaserShot currently spawns a NarrativeProjectile via SpawnActor and ends immediately; there is no ability-owned hit/miss callback; MarkEnemy cannot be called from GA.

---

### Patch Item P1-LS.1 — Replace SpawnActor with AbilityTask_SpawnProjectile

- **What to change**
  - Replace the existing spawn step with the task-based spawn.

- **Where**
  - GA_FatherLaserShot manifest entry (lines referenced in handoff: ability entry 3883–4021; spawn node around 3969–3973).

- **Why**
  - Required by guide and Contract 25; enables OnTargetData/OnDestroyed.

- **Acceptance criteria**
  1. Spawn uses task pattern, not SpawnActor.
  2. Task's Class input points to the existing ProjectileClass variable (already correct type: NarrativeProjectile).

---

### Patch Item P1-LS.2 — Add OnTargetData handler chain (ability-owned sequencing)

- **What to change**
  - Add a handler chain that runs when OnTargetData fires:
    - Extract target/hit actor from TargetData
    - Apply damage effect chain (as per guide and your locked damage architecture)
    - Call FatherRef.MarkEnemy(hit actor)
    - End ability (success path)

- **Where**
  - Inside GA_FatherLaserShot event graph as the OnTargetData CustomEvent chain.

- **Why**
  - Contract 25 forbids relocating "after hit → mark enemy" into projectile-only logic.

- **Acceptance criteria**
  1. MarkEnemy is executed from GA handler chain, not projectile actor.
  2. Damage application is executed in GA sequencing (unless guide explicitly delegates damage to projectile—which it does not).
  3. Ability ends after handler completes (not immediately after spawn).

---

### Patch Item P1-LS.3 — Add OnDestroyed handler chain (miss/cleanup path)

- **What to change**
  - Add handler for OnDestroyed delegate:
    - Miss handling / cleanup per guide
    - End ability (failure path)

- **Where**
  - OnDestroyed CustomEvent chain in GA_FatherLaserShot.

- **Why**
  - Required by guide; provides deterministic end path if projectile never produces target data.

- **Acceptance criteria**
  1. Ability always ends via either OnTargetData or OnDestroyed.
  2. No "dangling" ability instances remain active after projectile expires.

---

### Patch Item P1-LS.4 — Move CommitCooldown / EndAbility out of immediate spawn chain

- **What to change**
  - Remove "CommitCooldown → EndAbility" from the immediate spawn exec chain.
  - Reinsert commit/end in:
    - success path (OnTargetData)
    - failure path (OnDestroyed)

- **Where**
  - GA_FatherLaserShot exec chain after spawn.

- **Why**
  - If ability ends immediately, delegates cannot be handled; sequencing collapses.

- **Acceptance criteria**
  1. No EndAbility occurs before either delegate fires.
  2. Cooldown commit occurs at the correct point per guide (typically on activation or on successful resolution—follow your guide's exact rule).

---

### Patch Item P1-LS.5 — Add required activation tags (Recruited + Detached gates)

- **What to change**
  - Ensure activation_required_tags include:
    - Father.State.Recruited
    - Father.State.Detached (as per guide)
    - plus existing form tags required by design (Crawler, etc.)

- **Where**
  - GA_FatherLaserShot tags.activation_required_tags list.

- **Why**
  - Phase 6.2 shows missing gates; internal gating consistency.

- **Acceptance criteria**
  1. Tags match guide-required tags exactly (no missing gates).
  2. No input tags are introduced (Contract 14).

---

## 5) P1-ElectricTrap — Patch items (guide-fixed damage + type + task + gates)

### Context (truth state)
ElectricTrap is worse than LaserShot: TrapClass is typed as Actor, spawn is SpawnActor, no delegates, no effects, no montage, no max-traps FIFO, and missing deployed gating that exists elsewhere (TurretShoot).

---

### Patch Item P1-ET.1 — Fix TrapClass variable type to NarrativeProjectile subclass

- **What to change**
  - Change TrapClass variable definition from:
    - Class<Actor>
  - to:
    - Class<NarrativeProjectile> (or the exact ANarrativeProjectile subclass type required by the task)

- **Where**
  - GA_FatherElectricTrap variables.TrapClass definition (manifest line reference around 4188–4191 in handoff).

- **Why**
  - AbilityTask_SpawnProjectile requires TSubclassOf<ANarrativeProjectile>; Actor is incompatible.

- **Acceptance criteria**
  1. TrapClass is typed as NarrativeProjectile subclass type.
  2. Task node can accept TrapClass without pin type errors.

---

### Patch Item P1-ET.2 — Replace SpawnActor with AbilityTask_SpawnProjectile

- **What to change**
  - Replace SpawnTrap SpawnActor node with task node.

- **Where**
  - GA_FatherElectricTrap spawn node around 4238–4242.

- **Why**
  - Enables OnTargetData/OnDestroyed and restores guide sequencing.

- **Acceptance criteria**
  1. Task-based spawn is in place.
  2. Delegates are bound to handler events.

---

### Patch Item P1-ET.3 — Add OnTargetData handler chain to apply effects

- **What to change**
  - OnTargetData handler must:
    - Validate target actor
    - Apply GE_ElectricWebRoot (root/CC) as specified
    - Apply GE_ElectricWebDamage (damage)
    - Update any trap tracking/FIFO if required by guide (see P1-ET.5)

- **Where**
  - OnTargetData CustomEvent chain for ElectricTrap GA.

- **Why**
  - Effects are currently missing entirely; this is the core gameplay mechanic.

- **Acceptance criteria**
  1. Both GEs are applied in handler chain (exact order per guide).
  2. Damage is attribute-based (Contract 24 compliant post-guide fix).
  3. Ability end/commit handled after effect application.

---

### Patch Item P1-ET.4 — Add montage / notify sequence (MANDATORY)

> **v1.0B Correction:** Conditional removed. Guide Phase 5 Section 5 explicitly requires AM_FatherThrowWeb with notify-gated spawn timing. This is mandatory per Contract 25.

- **What to change**
  - Add the montage play step (AM_FatherThrowWeb) with the required notify (FatherThrowWebRelease) gating the spawn timing.
  - Spawn MUST occur at notify timing, NOT immediately on activation.

- **Where**
  - Immediate activation chain, before task spawn begins; notify event leads into spawn.

- **Why**
  - Guide Phase 5 Section 5 explicitly requires animation-driven timing; Contract 25 forbids skipping.

- **Acceptance criteria**
  1. Montage plays on activation.
  2. Spawn occurs ONLY at notify timing (FatherThrowWebRelease), not immediately.
  3. Ability does not prematurely end before montage/notify path completes.

---

### Patch Item P1-ET.5 — Implement "max 2 traps FIFO replacement" system

- **What to change**
  - Implement tracking of active traps (max 2):
    - If below 2, spawn new
    - If already 2, remove oldest then spawn new
  - Storage location must match guide (likely BP_FatherCompanion) unless guide says GA stores it.

- **Where**
  - Either:
    - BP_FatherCompanion variables + helper functions (preferred if guide defines it there), or
    - GA variables + logic (only if guide defines GA-local ownership)

- **Why**
  - Core mechanic; currently absent.

- **Acceptance criteria**
  1. At runtime, only 2 traps remain active.
  2. Spawning a third replaces the oldest.
  3. Removal is authoritative and does not leak references.

---

### Patch Item P1-ET.6 — Add deployed gating consistent with internal pattern

- **What to change**
  - Add required tag gate for deployed/turret-deployed state:
    - Align with the internal precedent: GA_TurretShoot requires Father.State.TurretDeployed.
  - Also add Father.State.Recruited as universal gate (if guide requires; generally consistent).

- **Where**
  - GA_FatherElectricTrap tags.activation_required_tags list.

- **Why**
  - Internal consistency finding: similar Engineer combat abilities already gate on deployed state.

- **Acceptance criteria**
  1. ElectricTrap cannot activate unless deployed tag is present.
  2. Tag list matches guide (and internal consistency) exactly.

---

## 6) P1-Mark — Patch items (tags + BP lifecycle/UI/tracking + cleanup)

### Context (truth state)
Mark GA already has a working spec + SetByCaller duration and armor reduction. Missing: recruited gate, death-block, tracking (MarkedEnemies/MaxMarks FIFO), UI widget spawn/despawn, and death cleanup.

> **ERDEM DECISION (2026-01-30):** GA_FatherMark works in **Crawler and Engineer forms ONLY**. The manifest's removal of form block tags (Armor/Exoskeleton/Symbiote) is an **approved design decision**. Form blocking is handled implicitly because Mark is only callable from combat abilities that already have form gates. Do NOT restore guide-specified form block tags.

---

### Patch Item P1-MK.1 — Restore Father.State.Recruited activation gate (minimum)

- **What to change**
  - Add activation_required_tags: Father.State.Recruited back to GA_FatherMark.

- **Where**
  - GA_FatherMark tags.activation_required_tags (currently removed with a comment).

- **Why**
  - Guide requires recruitment gate; removing it is a design change unless explicitly approved.

- **Acceptance criteria**
  1. Mark cannot activate unless Father.State.Recruited is present.
  2. Form gating remains as currently intended (Crawler+Engineer only) per Erdem decision — no form block tags restored.

---

### Patch Item P1-MK.2 — Add death-block tag (Narrative.State.IsDead) + keep Transitioning block

- **What to change**
  - Add activation_blocked_tags:
    - Narrative.State.IsDead
  - Keep existing Father.State.Transitioning block.

- **Where**
  - GA_FatherMark tags.activation_blocked_tags.

- **Why**
  - Guide requires death-block; prevents invalid activations and cleanup edge cases.

- **Acceptance criteria**
  1. Mark cannot activate if Father is dead.
  2. Existing transitioning block remains.

---

### Patch Item P1-MK.3 — Add BP variables for tracking (if guide locates ownership in BP)

- **What to change**
  - Add to BP_FatherCompanion (or the actual owner defined by guide):
    - MarkedEnemies (array of actors)
    - MaxMarks (int, default 3)
    - MarkWidgetMap (map: Actor → WidgetRef) OR equivalent structure

- **Where**
  - BP_FatherCompanion variable section (verify existence per OI-3).

- **Why**
  - Guide states tracking is BP-owned; GA is invoked as a worker.

- **Acceptance criteria**
  1. Variables exist with correct types and defaults.
  2. Variables are replicated or authoritative as required by your project rules (follow existing patterns).

---

### Patch Item P1-MK.4 — FIFO enforcement when adding a new mark

- **What to change**
  - When a new mark is applied:
    - If MarkedEnemies length >= MaxMarks, remove oldest mark:
      - remove GE(s) from oldest target
      - destroy/cleanup widget
      - remove from MarkedEnemies and MarkWidgetMap
    - Then add new target to MarkedEnemies, spawn widget, apply GE spec (existing flow)

- **Where**
  - Either inside BP_FatherCompanion.MarkEnemy pipeline *before* activating GA_FatherMark, or inside GA_FatherMark activation logic *if* guide defines it there.

- **Why**
  - Core mechanic; missing today.

- **Acceptance criteria**
  1. Maximum of 3 marked enemies maintained.
  2. Oldest is removed deterministically.
  3. No orphan widgets or stale map entries.

---

### Patch Item P1-MK.5 — Widget spawn/despawn system (WBP_MarkIndicator)

- **What to change**
  - On mark apply:
    - Spawn WBP_MarkIndicator
    - Attach to enemy head (or specified socket)
    - Track widget ref in MarkWidgetMap
  - On mark removal (FIFO or death):
    - Remove widget and clear map entry

- **Where**
  - BP_FatherCompanion or GA_FatherMark, per guide ownership.

- **Why**
  - Guide requires visible mark indicator; missing currently.

- **Acceptance criteria**
  1. Widget appears above newly marked target.
  2. Widget is removed when mark expires/removed/death cleanup triggers.
  3. No duplicate widgets for same target.

---

### Patch Item P1-MK.6 — Death cleanup mechanism (evidence-gated choice)

- **What to change**
  - Implement cleanup when marked enemy dies:
    - remove from MarkedEnemies
    - remove widget
    - remove related mark effects if needed

- **Where**
  - Depends on the supported pattern (must be chosen with evidence):
    - Direct ASC OnDied delegate binding (if supported and safe)
    - AbilityTask / async wait pattern for death
    - Existing NarrativePro event API (if present)

- **Why**
  - Guide mandates death cleanup; prevents stale marks/widgets.

- **Acceptance criteria**
  1. When enemy dies, mark state is cleaned immediately and reliably.
  2. No delegate binding signature mismatches or crash-on-open regressions.
  3. Pattern selection is backed by plugin header evidence or existing working assets (ties to OI-2).

---

## 7) Cross-cutting tag patches (consistency)

### Patch Item X-TAG.1 — Standardize cooldown tag hierarchy (optional P2, unless Tech Ref locks it)

- **What to change**
  - Align activation_blocked_tags with the tag granted by the cooldown GE:
    - If guide/tech ref says Cooldown.Father.Engineer.ElectricTrap, use that consistently across GE + GA blocked tags.

- **Where**
  - ElectricTrap cooldown tag definitions (GA blocked tags, GE granted tags).

- **Why**
  - Prevent "cooldown doesn't block" bugs; tag naming consistency.

- **Acceptance criteria**
  1. Cooldown GE grants the exact tag the GA blocks on.
  2. Tag hierarchy matches your tech reference conventions.

---

## 8) Acceptance Tests (contract-grade)

> These tests are written as **verification requirements**. They are intentionally tool-agnostic (PIE, logs, unit-ish harness, etc.).

### Global tests (apply to all changes)

**AT-G1 — Blueprint safety**
- All changed Blueprints open without crash.
- All changed Blueprints compile with zero pin type errors.

**AT-G2 — Contract 24 compliance**
- No GameplayEffect that uses NarrativeDamageExecCalc relies on SetByCaller to drive damage magnitude.

**AT-G3 — Contract 25 compliance**
- For each ability, the sequence of gameplay events matches the guide:
  - no "projectile-only logic relocation" for guide-specified GA sequencing steps

---

### LaserShot tests

**AT-LS1 — Hit path sequencing**
- On projectile hit:
  - OnTargetData fires
  - damage is applied
  - MarkEnemy is called from GA (observable via log/trace or gameplay effect on target)
  - ability ends only after handler completion

**AT-LS2 — Miss path sequencing**
- On projectile expire/miss:
  - OnDestroyed fires
  - ability ends deterministically
  - no mark is applied

---

### ElectricTrap tests

**AT-ET1 — Type correctness**
- TrapClass accepts NarrativeProjectile subclass only; no Actor class types pass validation.

**AT-ET2 — Deployed gating**
- Ability cannot activate unless deployed tag is present (and recruited gate if required).

**AT-ET3 — OnTargetData applies effects**
- On target acquisition:
  - root/CC GE applies
  - damage GE applies (attribute-based)
  - ability ends after effect application

**AT-ET4 — Max traps FIFO**
- Spawning >2 traps replaces the oldest. Never >2 active traps.

**AT-ET5 — Montage/notify timing (v1.0B addition)**
- Trap spawn occurs ONLY when FatherThrowWebRelease notify fires.
- Trap does NOT spawn immediately on activation.
- Montage interruption prevents spawn (ability cancels cleanly).

---

### Mark tests

**AT-MK1 — Recruitment + death-block gates**
- Mark cannot activate when Father.State.Recruited missing.
- Mark cannot activate when Narrative.State.IsDead present.

**AT-MK2 — Max marks FIFO**
- Marking a 4th enemy removes the oldest mark, including its widget and tracking entries.

**AT-MK3 — Death cleanup**
- When a marked enemy dies, it is removed from tracking and widget disappears (no stale entries).

---

## 9) Open Investigations (must be resolved with evidence)

### OI-1 — Does generator already enforce skeleton-sync-before-delegate?
- **Action:** Verify in generator code path once coding is allowed.
- **Outcome:** If YES, P0-B.3 is confirmation-only; if NO, it is mandatory.

### OI-2 — What is the approved death cleanup pattern in NarrativePro (current version)?

> **v1.0B Clarification:** Guide Phase 5 Section 5.2 explicitly requires "Bind Event to OnDied" — this pattern IS guide-required, not optional. The investigation is about **safety verification**, not whether to implement it.

- **Action:** Confirm via plugin headers or existing working assets that OnDied delegate binding is safe under current LOCKED delegate contracts.
- **Outcome:** If OnDied binding is safe, use it. If not, identify equivalent Narrative Pro pattern (task-based wait, event API) that achieves the same cleanup behavior.

### OI-3 — BP_FatherCompanion: confirm MarkEnemy pipeline + storage variables
- **Action:** Verify BP has MarkEnemy, PendingMarkTarget, and the intended ownership for MarkedEnemies/MaxMarks/MarkWidgetMap.
- **Outcome:** Update patch items P1-MK.3–P1-MK.6 to match *actual* BP structure, without design change.

---

## 10) Patch application checklist (quick tracker)

- [ ] P0-A.1 ElectricTrap guide: remove SetByCaller damage for NarrativeDamageExecCalc
- [ ] P2-A.2 ElectricTrap guide: version footer normalized *(reclassified from P0 in v1.0B)*
- [ ] P0-B.1 Generator: AbilityTask_SpawnProjectile node support
- [ ] P0-B.2 Generator: handler CustomEvents with correct signatures
- [ ] P0-B.3 Generator: safe wiring order enforced
- [ ] P0-B.4 Generator: reusable pattern template created
- [ ] P1-LS.1 LaserShot: spawn node replaced with task
- [ ] P1-LS.2 LaserShot: OnTargetData chain (damage + MarkEnemy + end)
- [ ] P1-LS.3 LaserShot: OnDestroyed chain (end)
- [ ] P1-LS.4 LaserShot: move commit/end into callbacks
- [ ] P1-LS.5 LaserShot: add required activation tags
- [ ] P1-ET.1 ElectricTrap: TrapClass type fixed to NarrativeProjectile
- [ ] P1-ET.2 ElectricTrap: spawn replaced with task
- [ ] P1-ET.3 ElectricTrap: OnTargetData applies required GEs
- [ ] P1-ET.4 ElectricTrap: montage/notify sequencing (MANDATORY per v1.0B)
- [ ] P1-ET.5 ElectricTrap: max 2 traps FIFO implemented
- [ ] P1-ET.6 ElectricTrap: deployed + recruited gating added
- [ ] P1-MK.1 Mark: restore recruited gate (form blocks NOT restored per Erdem decision)
- [ ] P1-MK.2 Mark: add IsDead blocked tag
- [ ] P1-MK.3 Mark: add BP tracking variables
- [ ] P1-MK.4 Mark: FIFO enforcement
- [ ] P1-MK.5 Mark: widget spawn/despawn
- [ ] P1-MK.6 Mark: death cleanup pattern implemented (evidence-backed)
- [ ] AT-ET5 ElectricTrap: montage/notify timing verified *(added in v1.0B)*
- [ ] X-TAG.1 (optional) cooldown tag hierarchy standardized
- [ ] AT-* All acceptance tests pass

---

---

## v1.0B Revision Summary

| Item | Change | Rationale |
|------|--------|-----------|
| P0-A.2 | Reclassified to P2 | Documentation hygiene, not a blocker |
| P1-ET.4 | Conditional removed | Guide Phase 5.5 explicitly requires montage |
| AT-ET5 | Added | Verify montage/notify timing (prevents silent regression) |
| OI-2 | Clarified | Guide-required, investigation is safety not necessity |
| P1-MK | Form blocks NOT restored | Erdem decision: Mark works in Crawler+Engineer only |

**End of Phase 7 Patch-Plan (v1.0B)**
