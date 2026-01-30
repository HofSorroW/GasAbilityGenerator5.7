# Master Audit Coverage List
## Version 1.0
## Date: 2026-01-29
## Compiled By: Claude (Opus 4.5)

---

## PURPOSE

This document consolidates ALL audit findings, LOCKED rules, and verified patterns from the complete audit trail. Use this as the single reference for "what has been covered so far."

---

## EXECUTIVE SUMMARY

| Category | Count | Status |
|----------|-------|--------|
| LOCKED Contracts | 25 | ALL ENFORCED |
| Validated Technical Findings (VTF) | 10 | ALL LOCKED |
| EndAbility Lifecycle Rules | 4 | ALL LOCKED |
| DOC-ONLY Patterns | 5 | ALL DOCUMENTED |
| NPC Systems Audited | 7 | ALL COMPLETE |
| NPC Decision Points | 19 | ALL RESOLVED |
| Exec-to-Pure Patterns | 22 | ALL VERIFIED |
| Manifest Audit Phases | 6 | ALL CLOSED |
| Generator Code Versions | v7.8.31 | CURRENT |

---

## PART 1: LOCKED CONTRACTS (25 Total)

### Data Safety Contracts (1-9)

| Contract | Name | Status |
|----------|------|--------|
| 1 | Metadata Contract (Registry + AssetUserData) | LOCKED |
| 2 | Regen/Diff Semantics (InputHash/OutputHash) | LOCKED |
| 3 | CONFLICT Gating and Force Semantics | LOCKED |
| 4 | Dry-Run Persistence Contract | LOCKED |
| 5 | Disk-Truth Asset Existence Contract | LOCKED |
| 6 | Manifest Whitelist Gate (Anti-Asset-Spam) | LOCKED |
| 7 | 3-Way Merge Contract (XLSX Sync) | LOCKED |
| 8 | Headless Safety (Policy Boundaries) | LOCKED |
| 9 | Reporting Schema (CI Interface) | LOCKED |

### Blueprint & Compilation Contracts (10, 22-23)

| Contract | Name | Status |
|----------|------|--------|
| 10 | Blueprint Compile Gate (v4.16 Graph Validation) | LOCKED |
| 22 | C_PIN_CONNECTION_FAILURE_GATE (C1) | LOCKED |
| 23 | C_SKELETON_SYNC_BEFORE_CREATEDELEGATE (C2) | LOCKED |

### Father Companion Specific Contracts (11, 15, 25)

| Contract | Name | Status |
|----------|------|--------|
| 11 | C_SYMBIOTE_STRICT_CANCEL | LOCKED |
| 15 | D-DEATH-RESET Player Death Reset System | LOCKED |
| 25 | C_NEVER_SIMPLIFY_ABILITIES | LOCKED |

### NPC & AI Contracts (12, 16-21)

| Contract | Name | Status |
|----------|------|--------|
| 12 | R-AI-1 Activity System Compatibility | LOCKED |
| 16 | R-SPAWN-1 SpawnNPC-Only for NPC Spawning | LOCKED |
| 17 | R-PHASE-1 Two-Phase Death Transition Pattern | LOCKED |
| 18 | R-DELEGATE-1 Delegate Binding CustomEvent Signature | LOCKED |
| 19 | R-NPCDEF-1 NPCDefinition Variable Type | LOCKED |
| 20 | P-BB-KEY-2 NarrativeProSettings BB Key Access | LOCKED |
| 21 | R-INPUTTAG-1 NPC Combat Ability InputTag Requirement | LOCKED |

### GAS & Manifest Contracts (13-14, 24)

| Contract | Name | Status |
|----------|------|--------|
| 13 | INV-GESPEC-1 MakeOutgoingGameplayEffectSpec Parameter | LOCKED |
| 14 | INV-INPUT-1 Input Architecture Invariants | LOCKED |
| 24 | D-DAMAGE-ATTR-1 Attribute-Based Damage System | LOCKED |

---

## PART 2: VALIDATED TECHNICAL FINDINGS (VTF-1 to VTF-10)

### VTF-1: Delay vs WaitDelay (AbilityTask)
- **Status:** VALIDATED
- **Finding:** Raw `Delay` does NOT terminate on EndAbility; `WaitDelay (AbilityTask)` has 3-layer protection (TaskOwnerEnded, Delegate Auto-Unbinding, ShouldBroadcastAbilityTaskDelegates)
- **Action:** All form abilities converted to WaitDelay

### VTF-2: ActivationOwnedTags Auto-Removal
- **Status:** VALIDATED
- **Finding:** ActivationOwnedTags automatically removed when EndAbility() is called (GameplayAbility.cpp:870)
- **Implication:** Tags in ActivationOwnedTags can be used as guards

### VTF-3: Activation Required Tags NOT Continuously Checked
- **Status:** VALIDATED
- **Finding:** Only checked at activation time, not during execution
- **Solution:** Use `Cancel Abilities with Tag` for cascade-cancellation

### VTF-4: K2_OnEndAbility Blueprint Event
- **Status:** VALIDATED
- **Finding:** `BlueprintImplementableEvent`, `bWasCancelled` parameter distinguishes normal end vs cancellation
- **Usage:** Event OnEndAbility in Blueprint

### VTF-5: No IsActive() Blueprint Node
- **Status:** VALIDATED
- **Finding:** `IsActive()` is C++ only (no UFUNCTION macro)
- **Workaround:** Use ActivationOwnedTags as proxy

### VTF-6: Generator AbilityTask Support
- **Status:** IMPLEMENTED (v4.15)
- **Finding:** Generator now supports `UK2Node_LatentAbilityCall`
- **Action:** New manifest node type: `type: AbilityTaskWaitDelay`

### VTF-7: CommitCooldown Requires Explicit Call
- **Status:** LOCKED
- **Finding:** If cooldown_class defined, at least one path must call CommitAbility() or CommitAbilityCooldown()
- **Enforcement:** Manifest lint

### VTF-8: SetByCaller Requires Matching GE Modifier
- **Status:** LOCKED
- **Finding:** SetByCaller tags only work if GE has modifiers configured to read those tags
- **Evidence:** EquippableItem.cpp:278-294 shows only Armor/AttackRating/StealthRating supported

### VTF-9: MakeOutgoingGameplayEffectSpec Requires param.GameplayEffectClass
- **Status:** LOCKED
- **Finding:** `gameplay_effect_class:` property is NOT processed; must use `param.GameplayEffectClass:`
- **Action:** 16 abilities fixed for silent runtime failure

### VTF-10: Input System Architecture
- **Status:** LOCKED
- **Finding:** `Narrative.Input.Father.*` namespace causes HasTagExact mismatch; must use `Narrative.Input.Ability1/2/3`
- **Invariants:** INV-INPUT-ASC-1, INV-INPUT-1
- **Action:** GA_DomeBurst, GA_StealthField, GA_FatherExoskeletonSprint, GA_ProximityStrike corrected

---

## PART 3: ENDABILITY LIFECYCLE RULES (Rule 1-4)

### Rule 1: Instant Abilities
- **Pattern:** `Event_Activate → Logic → CommitCooldown → K2_EndAbility`
- **Event_EndAbility:** NOT required
- **Examples:** GA_FatherAttack, GA_DomeBurst, GA_ProximityStrike, GA_TurretShoot, GA_Backstab, GA_FatherElectricTrap, GA_FatherLaserShot

### Rule 2: Abilities with Delay or Timer
- **Pattern:** MUST have `Event_EndAbility` with `bWasCancelled` check
- **Requirements:** Clean up persistent state, prevent post-delay execution after cancellation
- **Guard Pattern (3-layer):** IsValid → Father.State.Transitioning → Effect.Father.FormState
- **Examples:** All form abilities, GA_FatherExoskeletonDash, GA_StealthField

### Rule 3: Toggle/Persistent Abilities
- **Pattern:** Do NOT call K2_EndAbility on activation; MUST have Event_EndAbility for cleanup
- **Examples:** GA_ProtectiveDome, GA_FatherExoskeletonSprint, GA_FatherRifle, GA_FatherSword

### Rule 4: First Activation Path
- **Pattern:** True path may skip transition VFX, but MUST merge into same stateful setup chain as False path
- **Enforcement:** Manifest lint - True branch must reach SetupJoin node
- **Affected:** GA_FatherArmor, GA_FatherExoskeleton, GA_FatherSymbiote, GA_FatherEngineer

---

## PART 4: DOC-ONLY PATTERNS (No LOCKED Rules Required)

### P-MOVE-1: Movement Speed Restore Pattern
- **Status:** DOC-ONLY (already implemented correctly)
- **Pattern:** Store OriginalMaxWalkSpeed → Apply boost → Restore on end
- **Edge Case:** If other modifier active during activation, original captures modified value (low priority)
- **Affected:** GA_FatherSymbiote, GA_FatherArmor

### P-ATTACH-1: Socket Attachment/Detachment
- **Status:** NOT NEEDED (covered by architecture)
- **Reason:** ServerOnly execution + UE replication handles client sync

### P-EFFECT-1: Effect Removal Strategy
- **Status:** DOC-ONLY (both approaches valid)
- **Options:** Tag-based removal (Father pattern) OR Handle-based removal

### P-MONTAGE-1: Montage Task Lifecycle
- **Status:** DOC-ONLY (standard GAS knowledge)
- **Callbacks:** OnCompleted, OnInterrupted, OnCancelled, OnBlendOut
- **Key Rule:** OnBlendOut should NOT call EndAbility

### P-TARGET-1: Targeting/Tracing
- **Status:** DOC-ONLY
- **Pattern:** Narrative Pro uses own system (GenerateTargetDataUsingTrace in NarrativeCombatAbility)

---

## PART 5: NPC SYSTEMS AUDIT (7 Systems, 19 Decision Points)

### System Status Summary

| System | Guide Version | Technical | Gameplay | LOCKED | Status |
|--------|---------------|-----------|----------|--------|--------|
| Guard Formation Follow | v2.7 | PASS | PASS | PASS | COMPLETE |
| Possessed Exploder | v2.3 | PASS | PASS | PASS | COMPLETE |
| Biomechanical Detachment | v1.3 | PASS | PASS | PASS | COMPLETE |
| Gatherer Scout Alert | v1.3 | PASS | PASS | PASS | COMPLETE |
| Warden Husk | v1.5 | PASS | PASS | PASS | COMPLETE |
| Random Aggression Stalker | v2.4 | PASS | PASS | PASS | COMPLETE |
| Support Buffer Healer | v1.3 | PASS | PASS | PASS | COMPLETE |

### HIGH Priority Decisions (All Implemented)

| ID | System | Decision | Status |
|----|--------|----------|--------|
| PE-3 | Possessed Exploder | Added GoalGenerator_Attack | DONE |
| BM-2 | Biomech | Added modifiers (Host: 250/28/12, Creature: 85/38/0) | DONE |
| WH-3 | Warden | Added GoalGenerator_Attack to both ActivityConfigs | DONE |
| RS-3 | Stalker | Full GoalGenerator_RandomAggression (23 nodes, 60+ connections) | DONE |

### MEDIUM Priority Decisions (All Implemented)

| ID | System | Decision | Status |
|----|--------|----------|--------|
| GF-2 | Guard Formation | Created GE_FormationGuardAttributes (150/20/15) | DONE |
| PE-2 | Possessed Exploder | Added BPA_Idle to activities | DONE |
| PE-4 | Possessed Exploder | Blueprint functions in custom_functions | ALREADY COMPLETE |
| GS-1 | Gatherer Scout | Created GE_ReinforcementAttributes (120/25/5) | DONE |
| WH-2 | Warden | Added BPA_Patrol, BPA_Idle | DONE |
| WH-4 | Warden | HandleDeath SpawnOffset + EjectionMontage | DONE |
| RS-2 | Stalker | Created GE_ReturnedStalkerAttributes (100/30/0) | DONE |

### LOW Priority Decisions (All Resolved)

| ID | System | Decision | Status |
|----|--------|----------|--------|
| GF-1, GF-3 | Guard Formation | Keep Narrative.Factions.Returned, keep hardcoded positions | RESOLVED |
| PE-1 | Possessed Exploder | Updated guide to Narrative.Factions.Returned | DONE |
| BM-1 | Biomech | Keep in default_owned_tags | RESOLVED |
| WH-1 | Warden | Updated guide | DONE |
| RS-1 | Stalker | Blanket rule: all NPCs use Narrative.Factions.Returned | RESOLVED |
| SB-1, SB-2 | Support Buffer | Faction resolved, BTS_HealNearbyAllies complete in manifest | RESOLVED |

### Blanket Rules Established

| Rule | Scope | Description |
|------|-------|-------------|
| GoalGenerator_Attack | All combat NPCs | Any combat NPC missing GoalGenerator_Attack gets it added |
| Narrative.Factions.Returned | ALL NPCs | All NPCs use Narrative.Factions.Returned - no exceptions |

---

## PART 6: EXEC-TO-PURE AUDIT (22 Patterns Verified)

### Summary

| Category | Count |
|----------|-------|
| Truly impure (valid exec) | 4 |
| Auto-pure (const + output) | 16 |
| Explicit pure (BlueprintPure) | 2 |
| **Total** | **22** |

### Key Discovery: UHT Auto-Pure Rule

UnrealHeaderTool automatically adds `FUNC_BlueprintPure` to functions meeting ALL criteria:
1. `BlueprintCallable` specifier
2. `const` member function
3. Has output (return value or out parameters)

### Verified Pure Functions

| Function | Type | Evidence |
|----------|------|----------|
| `RandomFloat` | Explicit-pure | `UFUNCTION(BlueprintPure, ...)` |
| `Less_DoubleDouble` | Explicit-pure | `UFUNCTION(BlueprintPure, ...)` |
| `MakeTransform` | Explicit-pure | `UFUNCTION(BlueprintPure, ...)` |
| `K2_GetActorLocation` | Auto-pure | `const`, returns `FVector` |
| `MakeOutgoingGameplayEffectSpec` | Auto-pure | `const`, returns handle |
| `HasAuthority` | Auto-pure | `const`, returns `bool` |

### Verified Impure Functions (Valid Exec)

| Function | Evidence |
|----------|----------|
| `K2_CommitAbilityCooldown` | NOT const |
| `K2_EndAbility` | NOT const, void return |
| `SetValueAsVector` | NOT const, void return |
| `VariableSet` | Inherently has exec pins |

### Generator Behavior

- Generator bypasses exec connections to pure nodes (logs as "Skipped (pure bypass)")
- v7.8.31: `EConnectResult` enum for accurate logging (Connected/SkippedPure/Failed)

---

## PART 7: MANIFEST AUDIT (6 Phases Closed)

### Phase Summary

| Phase | Scope | Status | P0 Issues |
|-------|-------|--------|-----------|
| A | Narrative Pro Class Alignment | CLOSED | 0 |
| B | Node/Pin Validity | CLOSED | 0 |
| B2 | BP-Exposure Safety | CLOSED | 0 |
| C | Connection/Flow Validation | CLOSED | 0 |
| D | Tag Consistency | CLOSED | 0 (reclassified) |
| E | Asset Dependencies | CLOSED | 0 |

### Generation Result: 194/194 Assets (0 Failures)

### Key Findings

**Phase A:** All major systems use Narrative Pro classes (578 references analyzed)

**Phase B:** All 16 node types valid (964 nodes total)

**Phase B2:** All 113 functions verified BP-exposed

**Phase C:** No dead ends or orphaned nodes (1251 connections)

**Phase D:** Tag issues reclassified after NP investigation
- `Narrative.State.Invulnerable` - CORRECT (NP C++ registers via AddTag)
- `Narrative.Factions.Enemy/Friendly` - ARCHITECTURE (NP uses specific factions)
- `Data.Damage.Laser` - UNNECESSARY (NarrativeDamageExecCalc uses attributes)

**Phase E:** 100% dependency coverage

---

## PART 8: ADDITIONAL LOCKED RULES

### Locked Constraints (LC-1 to LC-4)

| ID | Rule | Status |
|----|------|--------|
| LC-1 | No Manual Blueprint Edits | LOCKED |
| LC-2 | No UE Source Modification | LOCKED |
| LC-3 | No C++ GameplayAbility Implementation | LOCKED |
| LC-4 | Process Lock (Research → Audit → Decide → Implement) | LOCKED |

### INV-1: Remove ALL Invulnerability Except GA_FatherSacrifice

- **Status:** LOCKED
- **Keep:** GA_FatherSacrifice 8-second PLAYER invulnerability only
- **Remove:** GE_TransitionInvulnerability, GE_DashInvulnerability, all State.Invulnerable from GE_*State

### New Locked Rules (v6.0)

| ID | Rule | Severity |
|----|------|----------|
| R-TIMER-1 | Timer Callback Safety | HIGH |
| R-ENUM-1 | GE-First Causality | MEDIUM |
| R-AI-1 | Activity System Compatibility | HIGH |
| R-CLEANUP-1 | Granted Ability Cleanup (Scoped) | MEDIUM |

### NL-GUARD-IDENTITY (L1): Post-Delay Identity Guard

- **Status:** LOCKED
- **3-Layer Pattern:**
  1. IsValid(FatherRef) - Actor exists
  2. Father.State.Transitioning - Ability not cancelled
  3. Effect.Father.FormState - Form identity intact

---

## PART 9: GENERATOR ENHANCEMENTS APPLIED

### v7.8.14 - Gatherer Scout Support
- ConstructObjectFromClass node support (UK2Node_GenericCreateObject)
- MakeLiteralByte node support
- {Root}/AI/Goals/ and {Root}/AI/GoalGenerators/ search paths

### v7.8.15 - Function Override Support
- MODIFY status asset deletion fix
- FunctionResult node creation
- Return node type handling

### v7.8.17 - Priority 1 Audit Fixes
- BPA_FormationFollow SetupBlackboard
- BPA_Attack_Formation ScoreGoalItem
- BPA_Alert Goal_Flee/Goal_MoveToDestination

### v7.8.19 - AddDelegate Node Type
- UK2Node_AddDelegate support for delegate binding

### v7.8.22 - AIPerception Generator
- Full `ai_perception` manifest support for controller Blueprints

### v7.8.29 - GoalGenerator_RandomAggression
- Clean pattern fix for exec-to-pure connections

### v7.8.31 - Logging Accuracy
- EConnectResult enum (Connected/SkippedPure/Failed)
- Accurate connection counters

---

## PART 10: CROSS-REFERENCE MATRIX

### Father Abilities Audited

| Ability | Rule | Status |
|---------|------|--------|
| GA_FatherCrawler | Rule 2 | COMPLIANT |
| GA_FatherArmor | Rule 2, Rule 4 | COMPLIANT |
| GA_FatherExoskeleton | Rule 2, Rule 4 | COMPLIANT |
| GA_FatherSymbiote | Rule 2, Rule 4, VTF-7 | COMPLIANT |
| GA_FatherEngineer | Rule 2, Rule 4, R-AI-1 | COMPLIANT |
| GA_FatherAttack | Rule 1 | COMPLIANT |
| GA_FatherLaserShot | Rule 1 | COMPLIANT |
| GA_ProtectiveDome | Rule 3, C25 | COMPLIANT |
| GA_DomeBurst | Rule 1, VTF-10 | COMPLIANT |
| GA_StealthField | Rule 2, VTF-10 | COMPLIANT |
| GA_FatherExoskeletonDash | Rule 2 | COMPLIANT |
| GA_FatherExoskeletonSprint | Rule 3, VTF-10 | COMPLIANT |
| GA_ProximityStrike | Rule 2, VTF-10 | COMPLIANT (v7.8.50: Changed to looping timer) |
| GA_FatherSacrifice | Special (INV-1) | COMPLIANT |
| GA_TurretShoot | Rule 1 | COMPLIANT |
| GA_FatherElectricTrap | Rule 1 | COMPLIANT |
| GA_CoreLaser | Rule 1, R-INPUTTAG-1 | COMPLIANT |
| GA_Backstab | Rule 1 | COMPLIANT |
| GA_FatherRifle | Rule 3 | COMPLIANT |
| GA_FatherSword | Rule 3 | COMPLIANT |

### NPC Attribute GEs Verified

| NPC | GE | HP/ATK/Armor | Status |
|-----|----|--------------| -------|
| Father Companion | GE_CrawlerState | Per form | OK |
| Warden Husk | GE_WardenHuskAttributes | 500/35/25 | OK |
| Warden Core | GE_WardenCoreAttributes | 75/50/0 | OK |
| Biomech Host | GE_BiomechHostAttributes | 250/28/12 | FIXED |
| Biomech Creature | GE_BiomechCreatureAttributes | 85/38/0 | FIXED |
| Possessed Exploder | GE_ExploderAttributes | 50/0/0 | OK |
| Support Buffer | GE_SupporterAttributes | 150/0/20 | OK |
| Formation Guard | GE_FormationGuardAttributes | 150/20/15 | FIXED |
| Reinforcement | GE_ReinforcementAttributes | 120/25/5 | FIXED |
| Returned Stalker | GE_ReturnedStalkerAttributes | 100/30/0 | FIXED |
| Gatherer Scout | None (non-combatant) | - | OK |

---

## PART 11: AUDIT CLOSURE CHECKLIST

| Area | Status |
|------|--------|
| Father GAS Abilities | AUDIT COMPLETE (v6.5) |
| NPC Systems | AUDIT COMPLETE (v1.5) |
| Manifest Validation | AUDIT COMPLETE (v1.0) |
| Exec-to-Pure Patterns | AUDIT COMPLETE (v1.1) |
| Warden/Biomech Deep Dive | AUDIT COMPLETE (v1.2) |
| Gatherer Scout Fixes | AUDIT COMPLETE (v1.0) |
| All LOCKED Contracts | 25 ENFORCED |
| All VTF Findings | 10 LOCKED |
| All Lifecycle Rules | 4 LOCKED |

---

## VERSION HISTORY

| Version | Date | Changes |
|---------|------|---------|
| v1.0 | 2026-01-29 | Initial master coverage list compiled from all audit documents |

---

**END OF MASTER AUDIT COVERAGE LIST**
