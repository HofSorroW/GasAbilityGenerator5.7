# NPC & Ability Master Audit Reference
## Version 1.1
## Date: January 2026
## Compiled from: 12 Audit Documents

---

## TABLE OF CONTENTS

1. [Executive Summary](#part-1-executive-summary)
2. [Locked Contracts (25+)](#part-2-locked-contracts)
3. [Validated Technical Findings (VTF-1 to VTF-10)](#part-3-validated-technical-findings)
4. [EndAbility Lifecycle Rules](#part-4-endability-lifecycle-rules)
5. [NPC Systems Audit Summary](#part-5-npc-systems-audit-summary)
6. [Father Companion Abilities Audit Summary](#part-6-father-companion-abilities-audit-summary)
7. [Generator Enhancements History](#part-7-generator-enhancements-history)
8. [Future Audit Checklist](#part-8-future-audit-checklist)
9. [Document Cross-Reference](#part-9-document-cross-reference)

---

## PART 1: EXECUTIVE SUMMARY

### Audit Coverage Statistics

| Category | Count | Status |
|----------|-------|--------|
| LOCKED Contracts | 29 | ALL ENFORCED |
| Validated Technical Findings (VTF) | 10 | ALL LOCKED |
| EndAbility Lifecycle Rules | 4 | ALL LOCKED |
| DOC-ONLY Patterns | 5 | ALL DOCUMENTED |
| NPC Systems Audited | 7 | ALL COMPLETE |
| Father Abilities Audited | 20 | ALL COMPLETE |
| Decision Points Resolved | 40+ | ALL RESOLVED |
| Generator Versions | v7.8.55 | CURRENT |

### NPC Systems Status

| System | Guide Version | Status |
|--------|---------------|--------|
| Guard Formation Follow | v2.7 | PASS |
| Possessed Exploder | v2.3 | PASS |
| Biomechanical Detachment | v1.3 | PASS |
| Gatherer Scout Alert | v1.3 | PASS |
| Warden Husk | v1.5 | PASS |
| Random Aggression Stalker | v2.4 | PASS |
| Support Buffer Healer | v1.4 | PASS |

### Father Abilities Status

| Ability | Rule | Status |
|---------|------|--------|
| GA_FatherCrawler | Rule 2 | COMPLIANT |
| GA_FatherArmor | Rule 2, Rule 4 | COMPLIANT |
| GA_FatherExoskeleton | Rule 2, Rule 4 | COMPLIANT |
| GA_FatherSymbiote | Rule 2, Rule 4 | COMPLIANT |
| GA_FatherEngineer | Rule 2, Rule 4 | COMPLIANT |
| GA_FatherAttack | Rule 1 | COMPLIANT |
| GA_FatherLaserShot | Rule 1 | COMPLIANT |
| GA_ProtectiveDome | Rule 3 | COMPLIANT |
| GA_DomeBurst | Rule 1 | COMPLIANT |
| GA_StealthField | Rule 2 | COMPLIANT |
| GA_FatherExoskeletonDash | Rule 2 | COMPLIANT |
| GA_FatherExoskeletonSprint | Rule 3 | COMPLIANT |
| GA_ProximityStrike | Rule 2 | COMPLIANT |
| GA_FatherSacrifice | Special (INV-1) | COMPLIANT |
| GA_TurretShoot | Rule 1 | COMPLIANT |
| GA_FatherElectricTrap | Rule 1 | COMPLIANT |
| GA_CoreLaser | Rule 1 | COMPLIANT |
| GA_Backstab | Rule 1 | COMPLIANT |
| GA_FatherRifle | Rule 3 | COMPLIANT |
| GA_FatherSword | Rule 3 | COMPLIANT |
| GA_FatherMark | Rule 1 | COMPLIANT |

---

## PART 2: LOCKED CONTRACTS

### Data Safety Contracts (1-9)

| Contract | Name | Description |
|----------|------|-------------|
| 1 | Metadata Contract | Registry + AssetUserData dual storage |
| 2 | Regen/Diff Semantics | InputHash/OutputHash tracking |
| 3 | CONFLICT Gating | Force flag required for modified assets |
| 4 | Dry-Run Persistence | No disk writes during dry-run |
| 5 | Disk-Truth Asset Existence | Check disk, not memory |
| 6 | Manifest Whitelist Gate | Only generate manifest-listed assets |
| 7 | 3-Way Merge Contract | XLSX sync with hash tracking |
| 8 | Headless Safety | Policy boundaries for commandlet |
| 9 | Reporting Schema | CI-compatible JSON output |

### Blueprint & Compilation Contracts (10, 22-23)

| Contract | Name | Description |
|----------|------|-------------|
| 10 | Blueprint Compile Gate | v4.16 graph validation before save |
| 22 | C_PIN_CONNECTION_FAILURE_GATE | Fail on pin connection errors |
| 23 | C_SKELETON_SYNC_BEFORE_CREATEDELEGATE | Sync skeleton before delegate binding |

### Father Companion Specific Contracts (11, 15, 25)

| Contract | Name | Description |
|----------|------|-------------|
| 11 | C_SYMBIOTE_STRICT_CANCEL | Only Sacrifice can cancel Symbiote |
| 15 | D-DEATH-RESET | Player death reset system |
| 25 | C_NEVER_SIMPLIFY_ABILITIES | STOP->ENHANCE->VERIFY, never simplify |

### NPC & AI Contracts (12, 16-21)

| Contract | Name | Description |
|----------|------|-------------|
| 12 | R-AI-1 | Activity system compatibility |
| 16 | R-SPAWN-1 | SpawnNPC-only for NPC spawning |
| 17 | R-PHASE-1 | Two-phase death transition pattern |
| 18 | R-DELEGATE-1 | Delegate binding CustomEvent signature |
| 19 | R-NPCDEF-1 | NPCDefinition variable type |
| 20 | P-BB-KEY-2 | NarrativeProSettings BB key access |
| 21 | R-INPUTTAG-1 | NPC combat ability InputTag requirement |

### GAS & Manifest Contracts (13-14, 24, 27)

| Contract | Name | Description |
|----------|------|-------------|
| 13 | INV-GESPEC-1 | MakeOutgoingGameplayEffectSpec parameter |
| 14 | INV-INPUT-1 | Input architecture invariants |
| 24 | D-DAMAGE-ATTR-1 | Attribute-based damage system (no SetByCaller with ExecCalc) |
| 27 | C_COOLDOWN_TAG_HIERARCHY | Cooldown tag format: `Cooldown.Father.{Form}.{Ability}` |
| 28 | C_SACRIFICE_THRESHOLD_MONITOR | Health < 15% auto-triggers sacrifice via AbilityAsyncWaitAttributeChanged |
| 29 | C_SACRIFICE_DEAD_GUARD_C | Sacrifice requires NOT Dead, Dormant, Cooldown + Recruited + Alive |

### Implementation Constraints (LC-1 to LC-4)

| ID | Rule | Description |
|----|------|-------------|
| LC-1 | No Manual Blueprint Edits | All via manifest + generator |
| LC-2 | No UE Source Modification | Stock engine/plugin binaries |
| LC-3 | No C++ GameplayAbility | Blueprint-only via generator |
| LC-4 | Process Lock | Research -> Audit -> Decide -> Implement |

---

## PART 3: VALIDATED TECHNICAL FINDINGS

### VTF-1: Delay vs WaitDelay (AbilityTask)
**Status:** VALIDATED

| Node Type | Behavior | Protection Layers |
|-----------|----------|-------------------|
| Delay | Does NOT terminate when ability ends | 0 |
| WaitDelay (AbilityTask) | Auto-terminates on EndAbility | 3 layers |

**WaitDelay Protection:**
1. TaskOwnerEnded() - Task destroyed when ability ends
2. Delegate Auto-Unbinding - UObject destruction cleanup
3. ShouldBroadcastAbilityTaskDelegates() - Checks IsActive()

### VTF-2: ActivationOwnedTags Auto-Removal
**Status:** VALIDATED

ActivationOwnedTags automatically removed when EndAbility() is called (GameplayAbility.cpp:870).

### VTF-3: Activation Required Tags NOT Continuously Checked
**Status:** VALIDATED

Only checked at activation time, not during execution. Use `Cancel Abilities with Tag` for cascade-cancellation.

### VTF-4: K2_OnEndAbility Blueprint Event
**Status:** VALIDATED

`bWasCancelled` parameter distinguishes normal end vs cancellation.

### VTF-5: No IsActive() Blueprint Node
**Status:** VALIDATED

`IsActive()` is C++ only. Use ActivationOwnedTags as proxy.

### VTF-6: Generator AbilityTask Support
**Status:** IMPLEMENTED (v4.15)

New manifest node type: `type: AbilityTaskWaitDelay`

### VTF-7: CommitCooldown Requires Explicit Call
**Status:** LOCKED

If cooldown_class defined, must call CommitAbility() or CommitAbilityCooldown().

### VTF-8: SetByCaller Requires Matching GE Modifier
**Status:** LOCKED

Only works if GE has modifiers configured to read those tags.

### VTF-9: MakeOutgoingGameplayEffectSpec Requires param.GameplayEffectClass
**Status:** LOCKED

Must use `param.GameplayEffectClass:` syntax. `gameplay_effect_class:` is NOT processed.

### VTF-10: Input System Architecture
**Status:** LOCKED

**INV-INPUT-1:** Player ASC abilities must use `Narrative.Input.Ability{1|2|3}` built-in tags.

---

## PART 4: ENDABILITY LIFECYCLE RULES

### Rule 1 - Instant Abilities

**Pattern:** `Event_Activate -> Logic -> CommitCooldown -> K2_EndAbility`
**Event_EndAbility:** NOT required

**Examples:** GA_FatherAttack, GA_DomeBurst, GA_ProximityStrike, GA_TurretShoot, GA_Backstab, GA_FatherElectricTrap, GA_FatherLaserShot, GA_CoreLaser

### Rule 2 - Abilities with Delay or Timer

**Requirements:**
- MUST have `Event_EndAbility` with `bWasCancelled` check
- MUST clean up persistent state
- MUST prevent post-delay execution after cancellation

**Guard Pattern (3-layer NL-GUARD-IDENTITY L1):**
1. IsValid(FatherRef) - Actor exists
2. Father.State.Transitioning - Ability not cancelled
3. Effect.Father.FormState - Form identity intact

**Examples:** All form abilities, GA_FatherExoskeletonDash, GA_StealthField

### Rule 3 - Toggle/Persistent Abilities

**Requirements:**
- Do NOT call K2_EndAbility on activation
- MUST have Event_EndAbility for cleanup
- Stay active until cancelled externally

**Examples:** GA_ProtectiveDome, GA_FatherExoskeletonSprint, GA_FatherRifle, GA_FatherSword

### Rule 4 - First Activation Path

**Status:** LOCKED

For form abilities using `bIsFirstActivation`, True path must merge into same stateful setup chain as False path before ending.

**Affected:** GA_FatherArmor, GA_FatherExoskeleton, GA_FatherSymbiote, GA_FatherEngineer

---

## PART 5: NPC SYSTEMS AUDIT SUMMARY

### Blanket Rules Established

| Rule | Scope | Description |
|------|-------|-------------|
| GoalGenerator_Attack | All combat NPCs | Any combat NPC ActivityConfig missing it gets added |
| Narrative.Factions.Returned | ALL NPCs | All NPCs use this faction - no exceptions |

### NPC Attribute Matrix (Final)

| NPC | HP | ATK | Armor | Role |
|-----|----|-----|-------|------|
| Warden Husk | 500 | 35 | 25 | Slow armored boss |
| Warden Core | 75 | 50 | 0 | Fragile flying ranged |
| Exploder | 50 | 0 | 0 | Suicide bomber |
| Supporter | 150 | 0 | 20 | Support healer |
| BiomechHost | 250 | 28 | 12 | Armored phase 1 |
| BiomechCreature | 85 | 38 | 0 | Fast aggressive phase 2 |
| Reinforcement | 120 | 25 | 5 | Standard ranged |
| ReturnedStalker | 100 | 30 | 0 | Glass cannon |
| FormationGuard | 150 | 20 | 15 | Tanky protector |

### System-Specific Decisions

#### Guard Formation Follow (GF-1 to GF-3)
- Faction: Keep Narrative.Factions.Returned
- Attributes: GE_FormationGuardAttributes (150/20/15)
- Positions: Keep hardcoded

#### Possessed Exploder (PE-1 to PE-4)
- Faction: Updated guide to Returned
- Activities: Added BPA_Idle
- GoalGenerator_Attack: Added
- Blueprint functions: Already in manifest custom_functions

#### Biomechanical Detachment (BM-1 to BM-2)
- Faction tags: Keep in default_owned_tags
- Host attributes: 250/28/12
- Creature attributes: 85/38/0

#### Gatherer Scout Alert (GS-1 to GS-2)
- Reinforcement attributes: 120/25/5
- AIPerception: Keep in Controller (v7.8.22)

#### Warden Husk (WH-1 to WH-4)
- Faction: Updated guide to Returned
- Activities: Added BPA_Patrol, BPA_Idle
- GoalGenerator_Attack: Added to both configs
- HandleDeath: SpawnOffset + EjectionMontage added

#### Random Aggression Stalker (RS-1 to RS-3)
- Faction: Narrative.Factions.Returned (blanket rule)
- Attributes: 100/30/0
- GoalGenerator_RandomAggression: Full 23-node implementation

#### Support Buffer Healer (SB-1 to SB-2)
- Faction: Narrative.Factions.Returned (blanket rule)
- BTS_HealNearbyAllies: Complete in manifest

---

## PART 6: FATHER COMPANION ABILITIES AUDIT SUMMARY

### GA_StealthField & GA_ProximityStrike (v7.8.51)

| Issue | Resolution |
|-------|------------|
| ProximityStrike execution | Rewritten to 30s looping timer (60 ticks) |
| ProximityStrike SetByCaller | Removed per Contract 24 |
| StealthField speed | Added -20% reduction |
| StealthField Father invisibility | Added FatherRef + FatherStealthHandle |
| StealthField ability tag | Changed to hierarchical |
| ProximityStrike knockback | Removed per design decision |

### GA_FatherSacrifice & GA_Backstab (v7.8.49 → v7.8.55c)

| Issue | Resolution |
|-------|------------|
| Sacrifice Net Policy | ServerInitiated (visual feedback) |
| Sacrifice Cooldown Tag | Hierarchical `Cooldown.Father.Symbiote.Sacrifice` |
| Backstab Type | Active (Input-Triggered) via Narrative.Input.Ability3 |

### GA_FatherSacrifice Comprehensive Audit (v7.8.55c)

**Audit Type:** Claude-GPT Dual Audit

#### Locked Decisions

| Decision | Issue | Resolution |
|----------|-------|------------|
| D-SACRIFICE-TARGET-1 | Invulnerability GE applied to Father instead of Player | Changed `BP_ApplyGameplayEffectToOwner` → `BP_ApplyGameplayEffectToTarget` with Player ASC |
| D-SAC-1/2/3 | Manifest triggers immediately; guide specifies 15% threshold | Implement `AbilityAsyncWaitAttributeChanged` monitoring (same as GA_ProtectiveDome) |
| D-SAC-5 | `Father.State.Offline` vs `Father.State.Dormant` inconsistent | Use `Father.State.Dormant` ONLY. Renamed `GE_FatherOffline` → `GE_FatherDormant` |
| D-SAC-7 | Guide says chest socket; manifest hides actor | VFX + Hide with placeholder `GC_FatherSacrificeDormant` cue |
| D-SAC-8 | Guide says Armor on reactivation; manifest just shows/ends | Activate `GA_FatherArmor` when dormant timer expires |
| D-SAC-9 | Guide says infinite; manifest uses SetByCaller | Keep SetByCaller 180s (self-cleaning) |

#### Protection Architecture (LOCKED)

**Layer A: 15% Threshold Monitoring (Blueprint)**
- Mechanism: `AbilityAsyncWaitAttributeChanged` monitors Player Health
- Trigger: When `Health / MaxHealth < 0.15`
- Protection: Gradual damage, burst chains, multi-hit scenarios
- Limitation: Cannot prevent true one-shot kills that skip threshold

**Layer B: NOT IMPLEMENTED (User Decision)**
- User explicitly rejected C++ pre-damage interception
- User explicitly rejected resurrection approach
- Accept partial protection per Layer A only

#### CanTriggerSacrifice() Invariant

All conditions must be true:
- `NOT Cooldown.Father.Symbiote.Sacrifice`
- `NOT Father.State.Dormant`
- `NOT Narrative.State.IsDead` (Player)
- `Father.State.Recruited = true`
- `Father.State.Alive = true`

#### Implementation Completed (All 6 Phases)

1. **Tag Cleanup:** Removed `Father.State.Offline`, renamed GE
2. **New Assets:** Added `GameplayCue.Father.Sacrifice.Dormant` tag and cue
3. **Critical Bug Fix:** Changed Owner → Target for invulnerability
4. **Health Monitoring:** Added threshold check with 15% branch
5. **Event Graph Complete:** Added dormant cue, `GA_FatherArmor` activation
6. **Documentation:** Updated guides and INI files

### Combat Abilities (GA_FatherLaserShot, GA_FatherElectricTrap, GA_FatherMark)

| Issue | Status |
|-------|--------|
| AbilityTask_SpawnProjectile | Requires generator enhancement |
| OnTargetData/OnDestroyed delegates | Requires generator enhancement |
| Mark form gating | ERDEM DECISION: Crawler + Engineer ONLY |

### Special Rules

#### INV-1: Invulnerability
- KEEP: GA_FatherSacrifice 10s PLAYER invulnerability only
- REMOVE: All other invulnerability (GE_TransitionInvulnerability, GE_DashInvulnerability)

#### Contract 24 Compliance

| Policy | Abilities |
|--------|-----------|
| SetByCaller FORBIDDEN | Any GE using NarrativeDamageExecCalc |
| SetByCaller ALLOWED | Duration, armor modifiers, stack counts |

---

## PART 7: GENERATOR ENHANCEMENTS HISTORY

### v7.8.14 - Gatherer Scout Support
- ConstructObjectFromClass node (UK2Node_GenericCreateObject)
- MakeLiteralByte node
- {Root}/AI/Goals/ and {Root}/AI/GoalGenerators/ search paths

### v7.8.15 - Function Override Support
- MODIFY status asset deletion fix
- FunctionResult node creation
- Return node type handling

### v7.8.17 - NPC Systems Priority 1
- BPA_FormationFollow SetupBlackboard
- BPA_Attack_Formation ScoreGoalItem
- BPA_Alert Goal_Flee/Goal_MoveToDestination

### v7.8.19 - Delegate Binding
- UK2Node_AddDelegate support
- GoalGenerator_Alert generates successfully

### v7.8.22 - AIPerception Generator
- Full `ai_perception` manifest support
- Sight/Hearing/Damage sense types

### v7.8.29 - Pure Node Handling
- Clean exec-to-pure connection handling
- EConnectResult enum for logging

### v7.8.50-51 - Ability Fixes
- ProximityStrike looping timer pattern
- StealthField speed reduction + Father invisibility

### v7.8.52 - Gameplay Cues
- Full GCN Effects support
- Item TMap support (holster_attachment_configs)

### v7.8.54 - Current
- BPC_ Blueprint Condition generator
- Schedule bReselect
- Component bAutoActivate

---

## PART 8: FUTURE AUDIT CHECKLIST

### Pre-Audit Preparation

- [ ] Identify audit scope (NPC system / Ability / Generator feature)
- [ ] Gather relevant documents:
  - [ ] Implementation Guide (primary source)
  - [ ] manifest.yaml sections
  - [ ] LOCKED_CONTRACTS.md
  - [ ] Technical Reference v6.8
  - [ ] Design Document v2.8

### Contract Compliance Checklist

- [ ] Contract 14 (INV-INPUT-1): Input tags use Narrative.Input.Ability{1|2|3}
- [ ] Contract 24 (D-DAMAGE-ATTR-1): No SetByCaller with NarrativeDamageExecCalc
- [ ] Contract 25 (C_NEVER_SIMPLIFY): Guide matches manifest exactly
- [ ] Contract 27 (C_COOLDOWN_HIERARCHY): Hierarchical cooldown tags

### NPC System Checklist

- [ ] NPCDefinition complete (npc_id, npc_name, npc_blueprint, configs)
- [ ] AbilityConfiguration with startup_effects
- [ ] ActivityConfiguration with activities + goal_generators
- [ ] GoalGenerator_Attack present for combat NPCs
- [ ] Faction: Narrative.Factions.Returned
- [ ] Attribute GE with HP/ATK/Armor modifiers

### Ability Checklist

- [ ] Parent class correct (NarrativeGameplayAbility)
- [ ] Net execution policy matches pattern (ServerOnly/LocalPredicted/ServerInitiated)
- [ ] EndAbility rule compliance (Rule 1/2/3/4)
- [ ] Cooldown tag hierarchical format
- [ ] Input tag (if player ability) uses built-in tags
- [ ] Guard pattern after delays (if Rule 2)

### Post-Audit Verification

- [ ] Build plugin successfully
- [ ] Delete Content folder if code changed
- [ ] Run generation cycle
- [ ] Check logs for 0 errors
- [ ] Verify connection counts match expected

---

## PART 9: DOCUMENT CROSS-REFERENCE

### Source Documents

| Document | Location | Content |
|----------|----------|---------|
| Father_Companion_GAS_Abilities_Audit.md | ClaudeContext/Handoffs/ | VTF findings, lifecycle rules, severity matrix |
| Master_Audit_Coverage_v1_0.md | ClaudeContext/Handoffs/ | Contracts summary, coverage statistics |
| NPC_Systems_Comprehensive_Audit_v1_0.md | ClaudeContext/Handoffs/ | All 7 NPC systems, 19 decision points |
| NPC_Lifecycle_Comprehensive_Audit_v1_0.md | ClaudeContext/Handoffs/ | Technical reference alignment |
| Warden_Biomech_Systems_Audit_v1_0.md | ClaudeContext/Handoffs/ | Phase transition patterns |
| NPC_Systems_Audit_Fixes_v7_8_17.md | ClaudeContext/Handoffs/ | Generator fixes history |
| Gatherer_Scout_System_Audit_Fixes_v1_0.md | ClaudeContext/Handoffs/ | Fix patterns |
| StealthField_ProximityStrike_Audit_Handoff_v1_0.md | ClaudeContext/Handoffs/ | Exoskeleton/Symbiote abilities |
| GA_Sacrifice_Backstab_Audit_v1_0.md | ClaudeContext/Handoffs/ | Special abilities |
| Father_Combat_Abilities_Dual_Audit_v1_0.md | ClaudeContext/Handoffs/ | Combat abilities |
| Father_Combat_Abilities_Dual_Audit_v1_0A.md | ClaudeContext/Handoffs/ | Extended combat audit |
| GA_FatherSacrifice_Audit_v7_8_55.md | ClaudeContext/Handoffs/ | Sacrifice 15% threshold, dormant state (CONSOLIDATED) |

### Related Reference Documents

| Document | Location | Content |
|----------|----------|---------|
| LOCKED_CONTRACTS.md | ClaudeContext/Handoffs/ | All contract definitions |
| Father_Companion_Technical_Reference_v6_8.md | ClaudeContext/ | Technical patterns |
| Father_Companion_System_Design_Document_v2_8.md | ClaudeContext/ | Design intent |
| CLAUDE.md | Plugin root | Generator usage |

---

## VERSION HISTORY

| Version | Date | Changes |
|---------|------|---------|
| 1.1 | 2026-01-31 | Consolidated GA_FatherSacrifice_Audit_v7_8_55.md, added Contracts 28-29 |
| 1.0 | 2026-01-31 | Initial compilation from 11 audit documents |

---

**END OF MASTER AUDIT REFERENCE**

**Status:** All audits complete. All contracts enforced. All systems compliant.
