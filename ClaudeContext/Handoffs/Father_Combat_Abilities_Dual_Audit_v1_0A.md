# Father Combat Abilities — Dual-Auditor Audit Handoff (EXTENDED)

## Version: 1.0A (Audit-Extended)
## Date: January 2026
## Auditors: Claude (Primary) + GPT (Counter-Auditor)
## Scope: GA_FatherLaserShot, GA_FatherElectricTrap, GA_FatherMark
## Status: Phase 6.2 complete; Phase 7 Patch-Plan v1.0B complete; implementation blocked until Erdem authorizes coding.

### Key Decision Logged
**ERDEM DECISION (2026-01-30):** GA_FatherMark works in **Crawler and Engineer forms ONLY**. Form block tags (Armor/Exoskeleton/Symbiote) are NOT restored — this is an approved design decision, not a Contract 25 violation.

---

## 0) PRIME DIRECTIVE (LOCKED)

### Audit Rules in Force

| # | Rule | Description |
|---|------|-------------|
| 1 | NO CODING | Unless Erdem explicitly requests |
| 2 | NO DESIGN CHANGES | Abilities remain as designed in guides/design doc |
| 3 | CONSISTENCY CHECK | Guide ↔ Manifest ↔ Tech Ref ↔ Design Doc |
| 4 | CHALLENGE PROTOCOL | No default acceptance; require proof |
| 5 | RESEARCH ALLOWED | Web research permitted, but engine/plugin headers win |
| 6 | PATTERN IDENTIFICATION | Identify gameplay patterns and whether LOCKED or not |
| 7 | CONTRACT HIERARCHY | LOCKED contract > Guide > Manifest |

---

## 1) WHAT THIS HANDOFF IS FOR

This handoff enables a new auditor or implementer to pick up the work and proceed **without re-reading the entire conversation**.

### Contents

- Final reconciled findings (post-challenge corrections)
- Exact remediation ordering and dependencies
- Contract-grade acceptance checklist
- Risk register (what can crash assets / violate locked rules)
- Open items requiring verification or Erdem decision

---

## 2) KEY LOCKED CONTRACTS INVOLVED

### Contract 25 — C_NEVER_SIMPLIFY_ABILITIES (LOCKED)

**Meaning:** If guide requires a pattern and generator can't express it, the correct action is:
```
STOP → ENHANCE generator → VERIFY
```

**Forbidden:**
- "Close enough" implementations
- Moving logic into projectile actors
- Removing callbacks
- Skipping "hard" steps

**Source:** LOCKED_CONTRACTS.md

---

### Contract 24 — D-DAMAGE-ATTR-1 (LOCKED)

**Meaning:** NarrativeDamageExecCalc uses captured attributes (AttackDamage, Armor).

**Forbidden:** Using SetByCaller to drive damage when NarrativeDamageExecCalc is the execution calculation.

**Source:** LOCKED_CONTRACTS.md

---

### Contract 14 — INV-INPUT-1 (Reference)

**Meaning:** Father/NPC ASC abilities should not use input tags.

**Status:** All three abilities are compliant.

---

## 3) CONSOLIDATED OUTCOME (FINAL, POST-CHALLENGE)

### Summary Table

| Ability | Contract 24 | Contract 25 | Status |
|---------|-------------|-------------|--------|
| GA_FatherLaserShot | ✅ compliant | ❌ fail | Requires remediation (task + delegates + sequencing in GA) |
| GA_FatherElectricTrap | ❌ guide error | ❌ fail | Requires guide fix first, then remediation |
| GA_FatherMark | N/A | ❌ fail | Partial impl exists; missing lifecycle/UI/tracking per guide |

### Root Cause (Systemic)

Generator lacks `AbilityTask_SpawnProjectile` + delegate outputs (`OnTargetData` / `OnDestroyed`) → manifests use `SpawnActor` fallback that cannot express guide sequencing.

---

## 4) ABILITY-BY-ABILITY TRUTH STATE

### 4.1 GA_FatherLaserShot — Final Audit Stance

#### Guide Requires

| Feature | Specification |
|---------|---------------|
| Spawn Method | AbilityTask_SpawnProjectile |
| Hit Handling | OnTargetData delegate |
| Miss Handling | OnDestroyed delegate |
| Damage | Applied in GA sequencing |
| Post-Damage | Call MarkEnemy (in GA) |
| AI Activation | BTTask_ActivateAbilityByClass |

#### Manifest Currently Does

| Feature | Implementation |
|---------|----------------|
| Spawn | NarrativeProjectile via SpawnActor |
| After Spawn | Commits cooldown and ends immediately |
| Delegates | None |
| Hit Callback | None |
| MarkEnemy | Not called in GA |
| Damage/Mark Logic | Relocated to projectile or absent |

**Verdict:** Contract 25 FAIL (logic relocation or feature loss)

#### Why SpawnActor is NOT Equivalent

Even if the projectile applies damage on hit, the guide requires **ability-owned sequencing** ("after damage, MarkEnemy"). Contract 25 forbids relocating that chain into projectile-only logic.

---

### 4.2 GA_FatherElectricTrap — Final Audit Stance

#### Manifest Mismatches (Worse Than LaserShot)

| Issue | Current State |
|-------|---------------|
| TrapClass | Typed as `Actor` (wrong type) |
| Spawn | Uses SpawnActor |
| OnTargetData | Missing |
| GE Application | Missing |
| Animation Montage | Missing |
| Max Traps FIFO | Missing |
| Deployed Gate | Missing (TurretShoot has it; ElectricTrap doesn't) |

#### Contract 24 Guide Contradiction

Guide section describes "NarrativeDamageExecCalc + SetByCaller damage" which is **illegal under Contract 24**.

**Decision Accepted:** Fix guide to attribute-based damage (AttackDamage).

#### Remediation Order (LOCKED)

```
1. Guide correction (Contract 24)
         ↓
2. Generator enhancement (task + delegates)
         ↓
3. Manifest remediation (type fix + task pattern + sequencing + deployed gate)
```

---

### 4.3 GA_FatherMark — Final Audit Stance

#### Critical Correction (Post-Challenge)

Mark is **NOT a stub**. It already has a working spec-based SetByCaller flow:

```
MakeGESpec(GE_MarkEffect)
    ↓
Set duration by caller (Data.Mark.Duration)
    ↓
Set armor reduction by caller (Data.Mark.ArmorReduction)
    ↓
Apply and EndAbility
```

#### What's Missing (Contract 25)

| Missing Feature | Guide Requirement |
|-----------------|-------------------|
| Recruited gate | Activation required tag |
| Death-block tag | Activation blocked tag |
| Mark limit system | MaxMarks=3 + FIFO |
| UI widget | WBP_MarkIndicator spawn/despawn |
| Death cleanup | OnDied or approved alternative |
| Tracking storage | MarkedEnemies, MarkWidgetMap in BP |

#### Special Note on Form Gating

Manifest comment says: "activation_required_tags removed; Mark works in both Crawler/Engineer."

**Audit Conclusion:** Form gating removal may be intentional, but `Father.State.Recruited` + death-block remain guide-required unless Erdem explicitly approves design change.

---

## 5) PHASE 6.2 VERIFICATION ITEMS (CANONICAL LIST)

These checklist items define "done" for remediation and post-implementation verification. They are contract-grade acceptance gates.

### GA_FatherLaserShot

| # | Requirement | Status | Insertion Point |
|---|-------------|--------|-----------------|
| 1 | AbilityTask_SpawnProjectile | ❌ WRONG | Replace SpawnProjectile node |
| 2 | OnTargetData handler | ❌ MISSING | After task, before CommitCooldown |
| 3 | MarkEnemy call on hit | ❌ MISSING | Inside OnTargetData handler |
| 4 | OnDestroyed handler | ❌ MISSING | Task OnDestroyed delegate |
| 5 | Father.State.Recruited gate | ⚠️ VERIFY | tags.activation_required_tags |
| 6 | Father.State.Detached gate | ⚠️ VERIFY | tags.activation_required_tags |

### GA_FatherElectricTrap

| # | Requirement | Status | Insertion Point |
|---|-------------|--------|-----------------|
| 7 | TrapClass: NarrativeProjectile | ❌ WRONG | variables.TrapClass.class |
| 8 | AbilityTask_SpawnProjectile | ❌ WRONG | Replace SpawnTrap node |
| 9 | Father.State.Deployed gate | ❌ INCONSISTENT | tags.activation_required_tags |
| 10 | OnTargetData handler | ❌ MISSING | After task creation |
| 11 | GE_ElectricWebRoot application | ❌ MISSING | Inside OnTargetData handler |
| 12 | GE_ElectricWebDamage application | ❌ MISSING | Inside OnTargetData handler |
| 13 | Animation montage | ❌ MISSING | Before spawn sequence |
| 14 | Max trap tracking | ❌ MISSING | Before spawn, FIFO logic |
| 15 | Attribute-based damage (C24) | ❌ GUIDE ERROR | GE_ElectricWebDamage |

### GA_FatherMark

| # | Requirement | Status | Insertion Point |
|---|-------------|--------|-----------------|
| 16 | Father.State.Recruited gate | ❌ MISSING | tags.activation_required_tags |
| 17 | Narrative.State.IsDead block | ❌ MISSING | tags.activation_blocked_tags |
| 18 | Form state block tags | ❌ MISSING | tags.activation_blocked_tags |
| 19 | MarkedEnemies array | ❌ MISSING | BP_FatherCompanion |
| 20 | MaxMarks variable (3) | ❌ MISSING | BP_FatherCompanion |
| 21 | FIFO mark removal | ❌ MISSING | Event graph branches |
| 22 | WBP_MarkIndicator spawn | ❌ MISSING | After GE apply |
| 23 | MarkWidgetMap variable | ❌ MISSING | BP_FatherCompanion |
| 24 | OnDied delegate cleanup | ❌ MISSING | Event graph / BP function |

---

## 6) PHASE 7 REMEDIATION PROGRAM

### P0-A: Guide Correction (ElectricTrap Contract 24 Fix)

**Required Change:** Remove SetByCaller damage modifier from guide if NarrativeDamageExecCalc is used.

**New Text:**
> Damage is captured via AttackDamage attribute (Engineer form state sets it), mitigated by target Armor.

**Acceptance:** Guide no longer instructs a forbidden pattern.

---

### P0-B: Generator Support for AbilityTask_SpawnProjectile + Delegates

This is the **main unblocker** for LaserShot + ElectricTrap.

#### Generator Must Express

| Capability | Description |
|------------|-------------|
| Task Factory Call | Create the AbilityTask factory call node |
| Exec Chain | Wire exec chain continuation |
| OnTargetData Binding | Bind to handler CustomEvent |
| OnDestroyed Binding | Bind to handler CustomEvent |

#### Locked Safety Invariant

> CustomEvent must exist in the blueprint skeleton **before** delegate binding is emitted, or assets may crash / fail compilation.

This is consistent with prior delegate-binding incident history.

#### Acceptance Criteria

| # | Criterion |
|---|-----------|
| 1 | Generated Blueprint compiles |
| 2 | Delegates fire at runtime (OnTargetData/OnDestroyed) |
| 3 | No crash-on-open / skeleton mismatch failures |

---

### P1: LaserShot Manifest Remediation (After P0 Generator)

**Goal:** Restore guide sequencing in GA.

| Step | Action |
|------|--------|
| 1 | Replace SpawnActor spawn step with AbilityTask task node |
| 2 | Move CommitCooldown/EndAbility into callback paths |
| 3 | Implement OnTargetData chain: apply damage, call MarkEnemy |
| 4 | Implement OnDestroyed chain: miss cleanup and end |

---

### P1: ElectricTrap Manifest Remediation (After P0 Guide + P0 Generator)

**Goal:** Correct type + restore guide mechanics.

| Step | Action |
|------|--------|
| 1 | TrapClass becomes NarrativeProjectile subclass type |
| 2 | Spawn via AbilityTask task |
| 3 | Apply effects in OnTargetData chain |
| 4 | Add montage if guide requires |
| 5 | Add max-traps tracking FIFO |
| 6 | Add deployed gate consistent with Engineer patterns |

---

### P1: Mark Remediation (Parallelizable)

**Goal:** Extend existing SetByCaller flow with guide-required lifecycle systems.

| Step | Action |
|------|--------|
| 1 | Add minimum tag gates (Recruited) + death-block tag |
| 2 | Implement mark limit FIFO |
| 3 | Implement widget spawn/despawn |
| 4 | Implement cleanup on death |

---

## 7) OPEN TECHNICAL INVESTIGATION ITEMS

### OI-1: Skeleton-Sync-Before-Delegate Already Implemented?

**Question:** Does the generator already enforce CustomEvent-before-delegate ordering?

**If Yes:** P0-B scope is smaller.
**If No:** Sub-requirement of P0-B; safety-critical.

**Audit Stance:** Do not assume—verify from generator code path once Erdem approves coding.

---

### OI-2: Mark Death Cleanup Pattern

**Context:** If direct delegate binding is deprecated/removed, need alternative pattern.

**Candidates:**

| Pattern | Description |
|---------|-------------|
| AbilityTask-style | Wait for death / target-data |
| Event-driven | Via approved NarrativePro death event API |
| Polling | Timer-based validity check (last resort) |

**Audit Rule:** Do not select pattern without evidence from NarrativePro headers or existing working assets.

---

### OI-3: BP_FatherCompanion Capabilities

**Must Verify:**

| Item | Purpose |
|------|---------|
| MarkEnemy function | Entry point for marking system |
| PendingMarkTarget | Data path for target passing |
| MarkedEnemies array | Storage location per guide |
| MarkWidgetMap | Widget tracking per guide |

---

## 8) RISK REGISTER

| Risk | Impact | Mitigation |
|------|--------|------------|
| Delegate binding order / skeleton mismatch | Crash-on-open or broken blueprints | Enforce skeleton-sync-before-delegate |
| Wrong class type pins (Actor vs NarrativeProjectile) | Pin type errors / invalid node calls | Verify class types before task wiring |
| Logic relocation (projectile performs GA responsibilities) | Contract 25 violation even if gameplay "works" | Keep sequencing in GA per guide |
| Contract 24 drift (SetByCaller damage with NarrativeDamageExecCalc) | Locked violation | Fix guide first; verify GE definitions |

---

## 9) EXIT CRITERIA

Phase 7 is complete **only when ALL of these are true**:

| # | Criterion | Verification |
|---|-----------|--------------|
| 1 | Contract 24 | No GE using NarrativeDamageExecCalc uses SetByCaller for damage |
| 2 | Contract 25 | Each GA matches guide sequencing; no "projectile does it all" substitutions |
| 3 | Generator Capability | Task + delegate pattern is first-class and reusable |
| 4 | Internal Consistency | Engineer deployed gating is consistent across Engineer combat abilities |

---

## 10) NEXT STEPS (AUDIT-COMPLIANT)

### Recommended Next Deliverable

**Phase 7 Patch-Plan (NO CODE)** that is "diff-gradeable" but stays at spec level:

| Element | Description |
|---------|-------------|
| Node replacements | "Replace node X with node Y" |
| Handler chains | "Insert handler chain after node Z" |
| Tag additions | "Add these tags at the GA header" |
| BP variables | "Add these BP variables + responsibilities" |
| Acceptance tests | "Define acceptance tests for each change" |

This avoids implementing YAML/code prematurely while giving implementers a precise plan.

---

## APPENDIX A: MARKENEMY CALL CHAIN

**Guide-Specified Flow:**

```
Hit Event (OnTargetData)
         ↓
GA_FatherLaserShot calls FatherRef.MarkEnemy(HitActor)
         ↓
BP_FatherCompanion.MarkEnemy() sets PendingMarkTarget = Enemy
         ↓
BP_FatherCompanion.MarkEnemy() calls ASC.TryActivateAbilityByClass(GA_FatherMark)
         ↓
GA_FatherMark activates, reads PendingMarkTarget, applies effects
```

**Current State:** No hit callback path exists in GA_FatherLaserShot, so MarkEnemy cannot be called from the ability.

**Conclusion:** AbilityTask_SpawnProjectile + OnTargetData is **P0 MANDATORY**.

---

## APPENDIX B: INTERNAL CONSISTENCY COMPARISON

### Engineer Combat Abilities Tag Gating

| Ability | Form Tag | Deployed Tag | Cooldown Tag |
|---------|----------|--------------|--------------|
| GA_TurretShoot | Effect.Father.FormState.Engineer | Father.State.TurretDeployed | Cooldown.Father.TurretShoot |
| GA_FatherElectricTrap | Effect.Father.FormState.Engineer | **MISSING** | Cooldown.Father.ElectricTrap |

**Observation:** Both are Engineer combat behaviors. Pattern exists but wasn't applied consistently.

---

## APPENDIX C: CORRECTED CONTRACT 25 CLASSIFICATION

### Original vs Corrected Findings

| Ability | Original Finding | GPT Challenge | Corrected Finding |
|---------|-----------------|---------------|-------------------|
| LaserShot | "ProjectileClass type mismatch" | Evidence shows NarrativeProjectile | ✅ Type correct; spawn method wrong |
| LaserShot | "Damage not implemented" | Damage relocated to projectile | ⚠️ Logic relocation (C25 issue) |
| Mark | "Stub implementation" | Has SetByCaller flow | ✅ Partial implementation exists |
| Mark | "Missing form blocks" | Comment says intentionally removed | ⚠️ Recruited/IsDead still required |

---

## APPENDIX D: MANIFEST LINE REFERENCES

| Item | Lines | Content |
|------|-------|---------|
| GA_FatherLaserShot definition | 3883-4021 | Full ability entry |
| LaserShot ProjectileClass | 3903-3906 | `class: NarrativeProjectile` |
| LaserShot SpawnProjectile node | 3969-3973 | `type: SpawnActor` |
| GA_FatherElectricTrap definition | 4168-4279 | Full ability entry |
| ElectricTrap TrapClass | 4188-4191 | `class: Actor` |
| ElectricTrap SpawnTrap node | 4238-4242 | `type: SpawnActor` |
| ElectricTrap required tags | 4180-4181 | Only Engineer form |
| GA_TurretShoot deployed gate | 4038-4039 | `Father.State.TurretDeployed` |
| GA_FatherMark definition | 6096-6253 | Full ability entry |
| Mark activation comment | 6106-6107 | "removed - Mark works in both forms" |
| Mark blocked tags | 6108-6109 | Only `Father.State.Transitioning` |
| Mark SetByCaller flow | 6153-6251 | Spec + duration + armor + apply |

---

## APPENDIX E: GUIDE VERSION REFERENCES

| Guide | File | Current Version | Issues |
|-------|------|-----------------|--------|
| GA_FatherLaserShot | v3_7.md | 3.7 (header says 3.8) | Minor version mismatch |
| GA_FatherElectricTrap | v3_1.md | 3.1 (header 3.2, footer 2.8) | Multiple version mismatches |
| GA_FatherMark | v1_9.md | 1.9 | Consistent |

---

**END OF EXTENDED AUDIT HANDOFF**

**Document:** Father_Combat_Abilities_Dual_Audit_v1_0A.md
**Status:** Phase 6.2 Complete, Phase 7 Specified
**Next Action:** Await Erdem authorization for Phase 7 Patch-Plan or coding
