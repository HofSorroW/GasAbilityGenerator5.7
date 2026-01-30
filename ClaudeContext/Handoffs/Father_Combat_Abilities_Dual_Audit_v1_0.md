# Father Combat Abilities - Dual Auditor Audit Report

## VERSION 1.0 - Complete Audit Handoff
## Date: January 2026
## Auditors: Claude (Primary) + GPT (Counter-Auditor)

---

## DOCUMENT INFORMATION

| Field | Value |
|-------|-------|
| Document Type | Audit Handoff |
| Scope | GA_FatherLaserShot, GA_FatherElectricTrap, GA_FatherMark |
| Audit Method | Dual-Auditor (Claude-GPT) with Challenge Protocol |
| Primary Reference | LOCKED_CONTRACTS.md v7.8.45 |
| Status | Phase 6.2 Complete, Phase 7 Ready |

---

## TABLE OF CONTENTS

1. [Executive Summary](#executive-summary)
2. [Audit Rules and Methodology](#audit-rules-and-methodology)
3. [Contract Reference](#contract-reference)
4. [Ability-by-Ability Findings](#ability-by-ability-findings)
5. [Phase 6.2 Verification Checklist](#phase-62-verification-checklist)
6. [Guide Corrections Required](#guide-corrections-required)
7. [Generator Enhancement Specifications](#generator-enhancement-specifications)
8. [Remediation Priority Matrix](#remediation-priority-matrix)
9. [Evidence Citations](#evidence-citations)
10. [Decision Log](#decision-log)
11. [Open Items](#open-items)

---

## EXECUTIVE SUMMARY

### Audit Scope

Three Father Companion combat abilities were audited for consistency across:
- Implementation Guides
- manifest.yaml
- Technical Reference Document
- Design Document
- GAS Abilities Audit
- LOCKED_CONTRACTS.md

### High-Level Findings

| Ability | Contract 24 | Contract 25 | Overall Status |
|---------|-------------|-------------|----------------|
| GA_FatherLaserShot | ✅ COMPLIANT | ❌ FAIL (6 items) | REQUIRES REMEDIATION |
| GA_FatherElectricTrap | ❌ GUIDE ERROR | ❌ FAIL (7 items) | REQUIRES GUIDE FIX + REMEDIATION |
| GA_FatherMark | N/A | ❌ FAIL (5 items) | REQUIRES REMEDIATION |

### Root Cause

The generator lacks support for `AbilityTask_SpawnProjectile` with delegate outputs (`OnTargetData`, `OnDestroyed`). Current fallback to `SpawnActor` node type cannot express guide-required hit detection and callback sequencing patterns.

### Remediation Path

Per Contract 25: **Enhance generator, do not simplify abilities.**

---

## AUDIT RULES AND METHODOLOGY

### Governing Rules (Established at Session Start)

| # | Rule | Description |
|---|------|-------------|
| 1 | NO CODING | No code changes unless explicitly requested by Erdem |
| 2 | NO DESIGN CHANGES | Abilities stay as designed; if guide says "apply stealth", it stays |
| 3 | CONSISTENCY CHECK | Cross-reference guides ↔ manifest ↔ tech doc ↔ design doc |
| 4 | CHALLENGE FINDINGS | No accepting defaults; ask for proof with evidence |
| 5 | RESEARCH PERMITTED | Web search and file research allowed where needed |
| 6 | IDENTIFY PATTERNS | Note which gameplay patterns are used and which are NOT locked |
| 7 | CONTRACT HIERARCHY | Locked Contract > Guide Specification > Manifest Implementation |

### Dual-Auditor Protocol

- **Claude (Primary):** Performs initial analysis, produces findings
- **GPT (Counter-Auditor):** Challenges findings, requests evidence, identifies misclassifications
- **Resolution:** Evidence-based corrections applied; both auditors must cite manifest/guide line numbers

### Phase Progression

| Phase | Description | Status |
|-------|-------------|--------|
| Phase 0 | Establish audit framework and rules | ✅ Complete |
| Phase 1 | Read LOCKED_CONTRACTS.md | ✅ Complete |
| Phase 2 | Read implementation guides | ✅ Complete |
| Phase 3 | Read manifest entries | ✅ Complete |
| Phase 4 | Cross-reference consistency | ✅ Complete |
| Phase 5 | Contract compliance assessment | ✅ Complete |
| Phase 6 | Node-level manifest audit | ✅ Complete |
| Phase 6.1 | Pin-by-pin checklist | ✅ Complete |
| Phase 6.2 | Diff-friendly verification table | ✅ Complete |
| Phase 7 | Remediation specifications | 🔲 Ready to Start |

---

## CONTRACT REFERENCE

### Contract 24: D-DAMAGE-ATTR-1 (LOCKED)

**Source:** LOCKED_CONTRACTS.md

**Rule:**
> NarrativeDamageExecCalc uses captured attributes (AttackDamage, Armor). SetByCaller tags: NOT used for damage values.

**Implication:** Any GE using `NarrativeDamageExecCalc` must derive damage from the `AttackDamage` attribute, not from SetByCaller magnitude tags.

**Affected Ability:** GA_FatherElectricTrap guide specifies SetByCaller for damage, which VIOLATES this contract.

---

### Contract 25: C_NEVER_SIMPLIFY_ABILITIES (LOCKED)

**Source:** LOCKED_CONTRACTS.md

**Rule:**
> Abilities MUST match their implementation guides exactly. If a generator limitation prevents implementing a feature:
> 1. STOP - Do not simplify or work around the limitation
> 2. ENHANCE - Add generator support for the required pattern/node type
> 3. VERIFY - Ensure the ability matches the guide's specified gameplay mechanics

**Forbidden Actions:**
- Replacing damage-scaled values with fixed amounts
- Removing attribute tracking because FGameplayAttribute is complex
- Skipping validation steps to avoid adding new node types
- Using "close enough" alternatives that change gameplay behavior

**Implication:** When guide ≠ manifest, the manifest is WRONG and must be corrected. The generator must be enhanced to support the guide's requirements.

---

### Contract 14: INV-INPUT-1 (Reference)

**Rule:** Input tags are only valid for Player ASC abilities.

**Implication:** Father abilities (NPC-owned) should not have input tags. All three audited abilities correctly have no input tags.

---

## ABILITY-BY-ABILITY FINDINGS

### GA_FatherLaserShot

#### Document Versions
| Document | Version |
|----------|---------|
| Implementation Guide | v3.7 (header says v3.8) |
| Manifest Entry | Lines 3883-4021 |

#### Guide Specification Summary

| Feature | Specification |
|---------|---------------|
| Parent Class | NarrativeGameplayAbility |
| Projectile Class | NarrativeProjectile (BP_LaserProjectile) |
| Spawn Method | AbilityTask_SpawnProjectile |
| Hit Detection | OnTargetData delegate from task |
| Miss Handling | OnDestroyed delegate from task |
| Post-Hit Action | Call MarkEnemy on FatherRef |
| Damage System | NarrativeDamageExecCalc (attribute-based) |
| AI Activation | BTTask_ActivateAbilityByClass |

#### Manifest Implementation Summary

| Feature | Implementation |
|---------|----------------|
| Parent Class | NarrativeGameplayAbility ✅ |
| ProjectileClass Type | Class<NarrativeProjectile> ✅ |
| Spawn Method | SpawnActor ❌ |
| Hit Detection | None (ability ends after spawn) ❌ |
| Miss Handling | None ❌ |
| Post-Hit Action | None (no callback path) ❌ |
| Damage Application | Relocated to projectile ❌ |

#### Contract 25 Violations

| # | Violation | Evidence |
|---|-----------|----------|
| 1 | Wrong spawn method | `SpawnProjectile: type: SpawnActor` (manifest:3970) |
| 2 | Missing OnTargetData handler | No delegate nodes in graph |
| 3 | Missing OnDestroyed handler | No delegate nodes in graph |
| 4 | Missing MarkEnemy call | No hit callback path exists |
| 5 | Logic relocation | Damage/mark logic moved out of GA |
| 6 | Missing activation tags | Only Crawler form; missing Recruited/Detached |

#### Contract 24 Status: ✅ COMPLIANT

Guide correctly specifies NarrativeDamageExecCalc without SetByCaller for damage. The GE uses captured AttackDamage attribute.

---

### GA_FatherElectricTrap

#### Document Versions
| Document | Version | Notes |
|----------|---------|-------|
| Implementation Guide | v3.1 (header says v3.2, footer says v2.8) | Version inconsistency |
| Manifest Entry | Lines 4168-4279 |

#### Guide Specification Summary

| Feature | Specification |
|---------|---------------|
| Parent Class | NarrativeGameplayAbility |
| Trap Class | NarrativeProjectile (BP_ElectricWeb) |
| Spawn Method | AbilityTask_SpawnProjectile |
| Hit Detection | OnTargetData delegate from task |
| Effects Applied | GE_ElectricWebRoot + GE_ElectricWebDamage |
| Animation | AM_FatherThrowWeb with notify |
| Max Traps | 2 with FIFO replacement |
| Damage System | NarrativeDamageExecCalc + SetByCaller ❌ GUIDE ERROR |

#### Manifest Implementation Summary

| Feature | Implementation |
|---------|----------------|
| Parent Class | NarrativeGameplayAbility ✅ |
| TrapClass Type | Class<Actor> ❌ WRONG |
| Spawn Method | SpawnActor ❌ |
| Hit Detection | None ❌ |
| Effects Applied | None ❌ |
| Animation | None ❌ |
| Max Traps | None ❌ |

#### Contract 25 Violations

| # | Violation | Evidence |
|---|-----------|----------|
| 1 | Wrong TrapClass type | `TrapClass: class: Actor` (manifest:4190) |
| 2 | Wrong spawn method | `SpawnTrap: type: SpawnActor` (manifest:4238) |
| 3 | Missing OnTargetData handler | No delegate nodes |
| 4 | Missing GE application | No effect nodes |
| 5 | Missing animation montage | No montage nodes |
| 6 | Missing max trap system | No tracking/FIFO logic |
| 7 | Missing deployed gate | TurretShoot has it (manifest:4039), ElectricTrap doesn't |

#### Contract 24 Status: ❌ GUIDE ERROR

**Guide Section 2.3 (GE_ElectricWebDamage) specifies:**
```
| Custom Execution Calculation | Calculation Class: NarrativeDamageExecCalc |
| Calculation Modifiers [0] | Modifier Op: Override, Magnitude Type: Set By Caller, Tag: Data.Damage.ElectricWeb |
```

This **directly violates Contract 24**. The guide must be corrected.

#### Internal Consistency Issue

| Ability | activation_required_tags | Evidence |
|---------|-------------------------|----------|
| GA_TurretShoot | `Engineer` + `Father.State.TurretDeployed` | manifest:4038-4039 |
| GA_FatherElectricTrap | `Engineer` only | manifest:4180-4181 |

Both are Engineer combat abilities. The gating pattern exists but wasn't applied consistently.

---

### GA_FatherMark

#### Document Versions
| Document | Version |
|----------|---------|
| Implementation Guide | v1.9 |
| Manifest Entry | Lines 6096-6253 |

#### Guide Specification Summary

| Feature | Specification |
|---------|---------------|
| Parent Class | NarrativeGameplayAbility |
| Trigger | Called by combat abilities via BP_FatherCompanion.MarkEnemy() |
| Target Source | PendingMarkTarget variable in BP_FatherCompanion |
| Effects | GE_FatherMark + GE_MarkDamageBonus (2 GEs) |
| Mark Limit | Max 3 concurrent marks with FIFO |
| Widget | WBP_MarkIndicator spawned above enemy head |
| Death Cleanup | OnDied delegate binding for automatic cleanup |
| Form Restriction | Blocked in Armor/Exoskeleton/Symbiote forms |

#### Manifest Implementation Summary

| Feature | Implementation |
|---------|----------------|
| Parent Class | NarrativeGameplayAbility ✅ |
| Target Source | TargetEnemy variable (partial) ⚠️ |
| GE Application | Single GE_MarkEffect with SetByCaller ⚠️ |
| SetByCaller Duration | Data.Mark.Duration ✅ |
| SetByCaller Armor | Data.Mark.ArmorReduction ✅ |
| Mark Limit | None ❌ |
| Widget | None ❌ |
| Death Cleanup | None ❌ |
| Form Restriction | Only Transitioning blocked ❌ |

#### Key Insight: Partial Implementation Exists

The manifest has a **working spec-based SetByCaller flow** (manifest:6153-6251):
```
MakeGESpec(GE_MarkEffect) → SetDurationByCaller → SetArmorByCaller → ApplyGE → EndAbility
```

This is NOT a stub. Remediation extends this flow, not replaces it.

#### Contract 25 Violations

| # | Violation | Evidence |
|---|-----------|----------|
| 1 | Missing mark limit system | No MarkedEnemies array, no MaxMarks, no FIFO |
| 2 | Missing widget spawn | No WBP_MarkIndicator nodes |
| 3 | Missing death cleanup | No OnDied delegate binding |
| 4 | Missing Father.State.Recruited gate | Comment says "removed" (manifest:6106) |
| 5 | Missing form block tags | Only Transitioning blocked (manifest:6108-6109) |

#### Manifest Comment Analysis (manifest:6106-6107)

```yaml
# activation_required_tags removed - Mark works in both Crawler and Engineer forms
# Called internally by attack abilities (GA_FatherAttack, GA_FatherLaserShot, GA_TurretShoot, GA_FatherElectricTrap)
```

**Interpretation:** Mark is triggered internally by other abilities, so form gates were intentionally removed. However, `Father.State.Recruited` (recruitment gate) and `Narrative.State.IsDead` (death block) are still guide-required.

---

## PHASE 6.2 VERIFICATION CHECKLIST

### Format
```
[#] [Ability] [Guide Requirement] → [Manifest Evidence] → [Status] → [Insertion Point]
```

### GA_FatherLaserShot

| # | Requirement | Manifest Evidence | Status | Insertion Point |
|---|-------------|-------------------|--------|-----------------|
| 1 | AbilityTask_SpawnProjectile | `SpawnProjectile: type: SpawnActor` (3970) | ❌ WRONG | Replace node, rewire exec |
| 2 | OnTargetData delegate handler | None exists | ❌ MISSING | After task, before CommitCooldown |
| 3 | MarkEnemy call on hit | No hit callback path | ❌ MISSING | Inside OnTargetData handler |
| 4 | OnDestroyed handler | None exists | ❌ MISSING | Task OnDestroyed delegate |
| 5 | Father.State.Recruited gate | Not in required tags | ⚠️ VERIFY | tags.activation_required_tags |
| 6 | Father.State.Detached gate | Not in required tags | ⚠️ VERIFY | tags.activation_required_tags |

### GA_FatherElectricTrap

| # | Requirement | Manifest Evidence | Status | Insertion Point |
|---|-------------|-------------------|--------|-----------------|
| 7 | TrapClass: NarrativeProjectile | `class: Actor` (4190) | ❌ WRONG | variables.TrapClass.class |
| 8 | AbilityTask_SpawnProjectile | `SpawnTrap: type: SpawnActor` (4238) | ❌ WRONG | Replace node, rewire exec |
| 9 | Father.State.Deployed gate | Not present | ❌ INCONSISTENT | tags.activation_required_tags |
| 10 | OnTargetData handler | None exists | ❌ MISSING | After task creation |
| 11 | GE_ElectricWebRoot application | None exists | ❌ MISSING | Inside OnTargetData handler |
| 12 | GE_ElectricWebDamage application | None exists | ❌ MISSING | Inside OnTargetData handler |
| 13 | Animation montage | None exists | ❌ MISSING | Before spawn sequence |
| 14 | Max trap tracking | None exists | ❌ MISSING | Before spawn, with FIFO logic |
| 15 | Attribute-based damage (C24) | Guide says SetByCaller | ❌ GUIDE ERROR | GE_ElectricWebDamage |

### GA_FatherMark

| # | Requirement | Manifest Evidence | Status | Insertion Point |
|---|-------------|-------------------|--------|-----------------|
| 16 | Father.State.Recruited gate | Removed per comment (6106) | ❌ MISSING | tags.activation_required_tags |
| 17 | Narrative.State.IsDead block | Only Transitioning (6108) | ❌ MISSING | tags.activation_blocked_tags |
| 18 | Form state block tags | Not present | ❌ MISSING | tags.activation_blocked_tags |
| 19 | MarkedEnemies array | Not in GA (BP var) | ❌ MISSING | BP_FatherCompanion |
| 20 | MaxMarks variable (3) | Not in GA (BP var) | ❌ MISSING | BP_FatherCompanion |
| 21 | FIFO mark removal | No removal logic | ❌ MISSING | Event graph branches |
| 22 | WBP_MarkIndicator spawn | No widget nodes | ❌ MISSING | After GE apply |
| 23 | MarkWidgetMap variable | Not present | ❌ MISSING | BP_FatherCompanion |
| 24 | OnDied delegate cleanup | No delegate binding | ❌ MISSING | Event graph / BP function |

---

## GUIDE CORRECTIONS REQUIRED

### GA_FatherElectricTrap_Implementation_Guide_v3_1.md

#### Contract 24 Violation Fix

**Location:** Section 2.3 (GE_ElectricWebDamage Configuration)

**Current (WRONG):**
```markdown
| Custom Execution Calculation | Calculation Class: NarrativeDamageExecCalc |
| Calculation Modifiers [0] | Modifier Op: Override, Magnitude Type: Set By Caller, Tag: Data.Damage.ElectricWeb |
```

**Corrected:**
```markdown
| Custom Execution Calculation | Calculation Class: NarrativeDamageExecCalc |
| Note | Damage derived from Father's AttackDamage attribute (Contract 24 compliant) |
```

**Rationale:** Contract 24 explicitly forbids SetByCaller magnitudes for damage when using NarrativeDamageExecCalc. The execution calculation captures AttackDamage from the source and Armor from the target automatically.

#### Version Footer Reconciliation

| Location | Current | Should Be |
|----------|---------|-----------|
| Header (line 2) | VERSION 3.2 | VERSION 3.2 |
| Document Info table | Version 3.1 | Version 3.2 |
| Footer (line 1213) | v2.8 | v3.2 |

---

## GENERATOR ENHANCEMENT SPECIFICATIONS

### P0: AbilityTask_SpawnProjectile Support

#### Requirement

Add first-class support for `AbilityTask_SpawnProjectile` node type with delegate output handling.

#### Technical Specification

**New Node Type:**
```yaml
- id: SpawnLaserTask
  type: AbilityTask_SpawnProjectile
  properties:
    task_name: LaserProjectileTask
    projectile_class_variable: ProjectileClass
    spawn_transform_node: MakeSpawnTransform
  delegates:
    on_target_data: HandleHit      # Custom event ID
    on_destroyed: HandleMiss       # Custom event ID
```

**Delegate Output Wiring:**

The generator must create:
1. The AbilityTask factory call node
2. CustomEvent nodes for each delegate
3. Connections from task delegate pins to CustomEvent exec pins

**Engine Reference:**
- Class: `UAbilityTask_SpawnProjectile` (NarrativeArsenal)
- Location: `Plugins/NarrativePro22B57/Source/NarrativeArsenal/Public/AbilityTask_SpawnProjectile.h`
- Delegates: `OnTargetData`, `OnDestroyed` (FProjectileTargetDataDelegate)

#### Acceptance Criteria

1. Generator can parse `type: AbilityTask_SpawnProjectile` nodes
2. Generator creates proper UK2Node_CallFunction for task factory
3. Generator creates UK2Node_CustomEvent for each delegate
4. Generator wires delegate output pins to custom event exec pins
5. Generated Blueprint compiles without errors
6. Delegate events fire correctly at runtime

---

### P1: Delegate Binding for Death Cleanup

#### Requirement

Support binding to ASC OnDied delegate for cleanup patterns (used by GA_FatherMark).

#### Technical Specification

```yaml
- id: BindDeathCleanup
  type: BindDelegate
  properties:
    delegate_source: EnemyASC
    delegate_name: OnDied
    handler_event: OnMarkedEnemyDied
```

**Contracts 22-23 Reference:** Delegate binding must follow schema-safe patterns to avoid crash-inducing mismatches.

---

## REMEDIATION PRIORITY MATRIX

### Priority Definitions

| Priority | Definition |
|----------|------------|
| P0 | Blocks other work; must complete first |
| P1 | Core functionality; complete after P0 |
| P2 | Polish/consistency; complete after P1 |

### Ordered Work Items

| Priority | Item | Blocker For | Effort Est. |
|----------|------|-------------|-------------|
| **P0** | Generator: AbilityTask_SpawnProjectile | LaserShot, ElectricTrap manifest fixes | High |
| **P0** | Guide Fix: ElectricTrap Contract 24 | ElectricTrap manifest fix | Low |
| **P1** | Manifest: LaserShot task pattern | None | Medium |
| **P1** | Manifest: ElectricTrap class + task | None | Medium |
| **P1** | Manifest: Mark lifecycle systems | None | Medium |
| **P2** | Manifest: LaserShot activation tags | None | Low |
| **P2** | Manifest: ElectricTrap deployed gate | None | Low |
| **P2** | Manifest: Mark form block tags | None | Low |

### Dependency Graph

```
[P0: Generator Enhancement]
         │
         ├──────────────────┐
         ▼                  ▼
[P1: LaserShot Fix]   [P1: ElectricTrap Fix] ◄── [P0: Guide Fix]
         │                  │
         ▼                  ▼
[P2: LaserShot Tags]  [P2: ElectricTrap Tags]

[P1: Mark Lifecycle] (independent of above)
         │
         ▼
[P2: Mark Tags]
```

---

## EVIDENCE CITATIONS

### Manifest Line References

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

### Guide Section References

| Guide | Section | Content |
|-------|---------|---------|
| GA_FatherLaserShot v3.7 | Phase 4, Section 15 | AbilityTask_SpawnProjectile usage |
| GA_FatherLaserShot v3.7 | Phase 4, Section 16 | OnTargetData handling + MarkEnemy |
| GA_FatherElectricTrap v3.1 | Phase 3, Section 2.3 | GE_ElectricWebDamage (Contract 24 issue) |
| GA_FatherElectricTrap v3.1 | Phase 5, Section 8 | AbilityTask_SpawnProjectile usage |
| GA_FatherMark v1.9 | Phase 5 | Mark limit system |
| GA_FatherMark v1.9 | Phase 5, Section 4 | Widget spawning |
| GA_FatherMark v1.9 | Phase 5, Section 5 | OnDied delegate cleanup |

### Contract References

| Contract | ID | Location |
|----------|----|---------|
| Damage Attributes | D-DAMAGE-ATTR-1 | LOCKED_CONTRACTS.md, Contract 24 |
| Never Simplify | C_NEVER_SIMPLIFY_ABILITIES | LOCKED_CONTRACTS.md, Contract 25 |
| Input Tags | INV-INPUT-1 | LOCKED_CONTRACTS.md, Contract 14 |

---

## DECISION LOG

| # | Decision | Rationale | Date |
|---|----------|-----------|------|
| 1 | ElectricTrap damage: Use attribute-based (AttackDamage) | Contract 24 compliance; no SetByCaller for damage with NarrativeDamageExecCalc | 2026-01-30 |
| 2 | LaserShot ProjectileClass reclassified as CORRECT | Evidence shows `class: NarrativeProjectile` (manifest:3905) | 2026-01-30 |
| 3 | Mark partial implementation acknowledged | Existing SetByCaller flow to be extended, not replaced | 2026-01-30 |
| 4 | Mark form gates intentionally removed | Comment indicates internal triggering; but Recruited/IsDead still required | 2026-01-30 |

---

## OPEN ITEMS

### Requires Erdem Decision

| # | Item | Options | Default |
|---|------|---------|---------|
| 1 | Mark form blocking | A) Restore guide's form blocks, B) Keep removed (internal triggering) | B (current) |
| 2 | LaserShot Recruited/Detached gates | A) Add per guide, B) Verify if needed for this ability | A (add) |

### Requires Further Investigation

| # | Item | Reason |
|---|------|--------|
| 1 | BP_FatherCompanion manifest section | Need to verify MarkedEnemies/MaxMarks/MarkWidgetMap variable definitions |
| 2 | MarkEnemy function | Need to verify BP_FatherCompanion has this function defined |
| 3 | WBP_MarkIndicator | Need to verify widget blueprint exists |

### Deferred to Phase 7

| # | Item |
|---|------|
| 1 | Generator enhancement implementation specification |
| 2 | Manifest correction YAML snippets |
| 3 | Test verification checklist |

---

## APPENDIX A: MARKENNY CALL CHAIN

**Guide-specified flow:**

```
Hit Event (OnTargetData)
    │
    ▼
GA_FatherLaserShot calls FatherRef.MarkEnemy(HitActor)
    │
    ▼
BP_FatherCompanion.MarkEnemy() sets PendingMarkTarget = Enemy
    │
    ▼
BP_FatherCompanion.MarkEnemy() calls ASC.TryActivateAbilityByClass(GA_FatherMark)
    │
    ▼
GA_FatherMark activates, reads PendingMarkTarget, applies effects
```

**Current manifest state:** No hit callback path exists in GA_FatherLaserShot, so MarkEnemy cannot be called from the ability. If the projectile calls MarkEnemy directly, that's logic relocation (Contract 25 violation). If nothing calls it, the feature is lost.

**Conclusion:** AbilityTask_SpawnProjectile + OnTargetData is P0 MANDATORY, not optional.

---

## APPENDIX B: INTERNAL CONSISTENCY COMPARISON

### Engineer Combat Abilities Tag Gating

| Ability | Form Tag | Deployed Tag | Cooldown Tag |
|---------|----------|--------------|--------------|
| GA_TurretShoot | Effect.Father.FormState.Engineer | Father.State.TurretDeployed | Cooldown.Father.TurretShoot |
| GA_FatherElectricTrap | Effect.Father.FormState.Engineer | **MISSING** | Cooldown.Father.ElectricTrap |

**Observation:** Both abilities are Engineer combat behaviors. TurretShoot requires deployed state; ElectricTrap does not. This is an internal inconsistency within the manifest itself.

---

## APPENDIX C: CORRECTED CONTRACT 25 CLASSIFICATION

### Original vs Corrected Findings

| Ability | Original Finding | GPT Challenge | Corrected Finding |
|---------|-----------------|---------------|-------------------|
| LaserShot | "ProjectileClass type mismatch" | Evidence shows NarrativeProjectile | ✅ Type is correct; spawn method is wrong |
| LaserShot | "Damage not implemented" | Damage relocated to projectile | ⚠️ Logic relocation (Contract 25 issue) |
| Mark | "Stub implementation" | Has SetByCaller flow | ✅ Partial implementation exists |
| Mark | "Missing form blocks" | Comment says intentionally removed | ⚠️ Recruited/IsDead still required |

---

**END OF AUDIT HANDOFF**

**Document Version:** 1.0
**Status:** Phase 6.2 Complete, Ready for Phase 7
**Next Action:** Await Erdem direction on P0 items or proceed to Phase 7 specifications
