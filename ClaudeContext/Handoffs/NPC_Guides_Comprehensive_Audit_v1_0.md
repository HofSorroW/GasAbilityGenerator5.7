# NPC Guides Comprehensive Audit Report
## Multi-Level Technical, Logical, and Gameplay Validation
## Version 1.0

---

## DOCUMENT INFORMATION

| Field | Value |
|-------|-------|
| Document Type | Audit Report |
| Scope | All NPC Implementation Guides |
| Date | January 2026 |
| Auditor | Claude (multi-level consistency audit) |
| Status | **CRITICAL VIOLATIONS FOUND** |

---

## EXECUTIVE SUMMARY

This audit validates NPC implementation guides against manifest.yaml, tech docs, design docs, LOCKED_CONTRACTS.md, and Father_Companion_GAS_Abilities_Audit.md.

### Critical Finding

| Severity | Issue | Location |
|----------|-------|----------|
| **CRITICAL** | SpawnActor used instead of SpawnNPC for phase transitions | BP_BiomechHost, BP_WardenHusk in manifest.yaml |

**Impact:** Spawned phase-2 entities (BP_BiomechCreature, BP_WardenCore) will be non-functional shells - no abilities, no activities, no faction tags.

---

## GUIDES AUDITED

| Guide | Version | Manifest Entries | Status |
|-------|---------|------------------|--------|
| Biomechanical_Detachment_System_Implementation_Guide | v1.2 | BP_BiomechHost, BP_BiomechCreature | **VIOLATION** |
| Warden_Husk_System_Implementation_Guide | v1.3 | BP_WardenHusk, BP_WardenCore | **VIOLATION** |
| Guard_Formation_Follow_System_Implementation_Guide | v2.5 | None (manual creation) | COMPLIANT |
| Support_Buffer_Healer_NPC_Implementation_Guide | v1.1 | BTS_HealNearbyAllies, BPA_SupportFollow | COMPLIANT |
| Possessed_Exploder_Enemy_Implementation_Guide | v2.2 | BTS_CheckExplosionProximity, BPA_Explode | COMPLIANT |
| Gatherer_Scout_Alert_System_Implementation_Guide | v1.2 | None (manual creation) | COMPLIANT |
| Random_Aggression_Stalker_System_Implementation_Guide | v2.2 | None (manual creation) | COMPLIANT |

---

## CRITICAL VIOLATION: SpawnActor vs SpawnNPC

### The Problem

**Guide Requirement (Biomechanical_Detachment v1.2, line 339-340):**
```
> **CRITICAL: SpawnNPC vs SpawnActor**
> You MUST use `NarrativeCharacterSubsystem->SpawnNPC()`, NOT raw SpawnActor.
```

**Guide Requirement (Warden_Husk v1.3, line 651-660):**
```
> **CRITICAL:** You MUST use `NarrativeCharacterSubsystem.SpawnNPC()` - NOT raw `SpawnActor`.
>
> **Why?** Raw SpawnActor does NOT call `SetNPCDefinition()` on the spawned NPC. Without this:
> - NPCDefinition property is null
> - AbilityConfiguration is never applied (no abilities granted)
> - ActivityConfiguration is never applied (no AI activities)
> - Faction tags are never set
> - NPC is not registered in the subsystem
```

**Manifest Reality (BP_BiomechHost - line 9971-9975):**
```yaml
- id: SpawnCreature
  type: SpawnActor
  properties:
    actor_class_variable: PhaseSpawnClass
  position: [800, 0]
```

**Manifest Reality (BP_WardenHusk - line 8918-8923):**
```yaml
- id: SpawnCore
  type: SpawnActor
  properties:
    actor_class_variable: PhaseSpawnClass
  position: [800, 0]
```

### Root Cause Analysis

The manifest generator has a `SpawnActor` node type but lacks a `SpawnNPC` node type that calls `NarrativeCharacterSubsystem.SpawnNPC()`.

### Impact Assessment

| Component | With SpawnActor | With SpawnNPC |
|-----------|-----------------|---------------|
| NPCDefinition | null | Properly set |
| AbilityConfiguration | Not applied | Applied |
| ActivityConfiguration | Not applied | Applied |
| Faction Tags | Not set | Set from NPCDef |
| Subsystem Registration | Not registered | Registered |
| AI Behavior | Non-functional | Functional |

**Severity: CRITICAL** - The spawned phase-2 enemies will not function as NPCs.

### Resolution Options

1. **Add SpawnNPC node type to generator** - Implement a new node type that calls `GetWorld()->GetSubsystem<UNarrativeCharacterSubsystem>()->SpawnNPC()`

2. **Use CallFunction workaround** - Chain CallFunction nodes to replicate the SpawnNPC call sequence (complex, error-prone)

3. **Manual Blueprint fix** - After generation, manually edit the blueprints to use SpawnNPC (breaks regeneration)

---

## LOCKED CONTRACTS COMPLIANCE

### Contract 12: R-AI-1 Activity System Compatibility

> NPCs with ActivityConfiguration must coordinate BT calls with Activity system - can't call RunBehaviorTree directly.

| Guide | Status | Notes |
|-------|--------|-------|
| All Guides | **COMPLIANT** | All use Activity system (BPA_*, Goal_*, GoalGenerator_*) |

### Contract 13: INV-GESPEC-1 MakeOutgoingGameplayEffectSpec Parameter

> MakeOutgoingGameplayEffectSpec MUST use `param.GameplayEffectClass:` syntax.

| Manifest Entry | Line | Status |
|----------------|------|--------|
| BTS_HealNearbyAllies | 9810 | **COMPLIANT** - Uses `param.GameplayEffectClass: /Game/.../GE_SupportHeal` |

### Contract 15: D-DEATH-RESET Player Death Reset System

Not applicable to NPC guides (player-specific).

---

## GAMEPLAY PATTERN ANALYSIS

### Pattern 1: Two-Phase Death Transition

**Used By:** Biomechanical_Detachment, Warden_Husk

| Component | Implementation | Contract Compliance |
|-----------|----------------|---------------------|
| HandleDeath Override | Server authority check -> Spawn new entity -> Call parent | Correct pattern |
| State Tags | Applied during transition (State.Warden.Ejecting, etc.) | Correct |
| Spawn Method | **SpawnActor in manifest** | **VIOLATION** |

**Expected Flow:**
1. Entity dies -> OnDied delegate fires
2. HandleDeath override executes
3. Server authority check
4. **SpawnNPC** (NOT SpawnActor) creates phase-2 entity with full NPC initialization
5. Original entity calls parent HandleDeath

**Locked Status:** Pattern is **NOT explicitly locked** in LOCKED_CONTRACTS.md but follows established Narrative Pro conventions.

### Pattern 2: Follow-and-Protect (Support Buffer)

**Used By:** Support_Buffer_Healer

| Component | Implementation | Status |
|-----------|----------------|--------|
| BPA_SupportFollow | NPCActivity for following | COMPLIANT |
| BTS_HealNearbyAllies | BTService for periodic healing | COMPLIANT |
| GE_SupportHeal | Gameplay Effect for heal | COMPLIANT |
| param.GameplayEffectClass | Used correctly | CONTRACT 13 COMPLIANT |

**Locked Status:** Not locked - general activity pattern.

### Pattern 3: Suicide Attack (Possessed Exploder)

**Used By:** Possessed_Exploder

| Component | Implementation | Status |
|-----------|----------------|--------|
| BPA_Explode | NPCActivity for explosion sequence | COMPLIANT |
| BTS_CheckExplosionProximity | BTService for range check | COMPLIANT |
| GE_ExplosionDamage | Damage effect (SetByCaller) | COMPLIANT |
| Self-destruct | Activity ends with self-destruction | COMPLIANT |

**Locked Status:** Not locked - general activity pattern.

### Pattern 4: Formation Follow (Guard)

**Used By:** Guard_Formation_Follow

| Component | Implementation | Status |
|-----------|----------------|--------|
| Goal_FormationFollow | Custom goal for formation slots | Manual creation guide |
| BT Services | Formation position calculation | Manual creation guide |
| Activity System | Uses standard Narrative Pro activities | COMPLIANT |

**Locked Status:** Not locked - no manifest entries to validate.

### Pattern 5: Alert-and-Reinforce (Gatherer Scout)

**Used By:** Gatherer_Scout

| Component | Implementation | Status |
|-----------|----------------|--------|
| GoalGenerator_Alert | Creates alert goals from perception | Manual creation guide |
| BPA_Alert | Signal animation + spawn reinforcements | Manual creation guide |
| SpawnNPC | Correctly documented in guide | COMPLIANT |

**Locked Status:** Not locked - manual creation guide, follows correct SpawnNPC pattern.

### Pattern 6: Random Aggression Bond (Stalker)

**Used By:** Random_Aggression_Stalker

| Component | Implementation | Status |
|-----------|----------------|--------|
| GoalGenerator_RandomAggression | Stacking bond system with timers | Manual creation guide |
| Tagged Dialogue | Integration with TaggedDialogueSet | COMPLIANT |
| Defend Mechanic | Player damage binding | COMPLIANT |
| Timer System | Multiple independent timers | **See R-TIMER-1 analysis** |

**Timer Pattern (R-TIMER-1 Analysis):**
The guide uses Set Timer by Event with looping enabled - this is acceptable for BTService-like periodic checks but requires proper cleanup in StopFollowing function. The guide correctly implements:
- FollowCheckTimerHandle cleanup
- TalkCheckTimerHandle cleanup
- AttackCheckTimerHandle cleanup
- FollowDurationTimerHandle cleanup

**Locked Status:** Not locked - manual creation guide, timer cleanup is properly documented.

---

## GAS ABILITIES AUDIT CROSS-REFERENCE

### VTF-1 to VTF-10 (Validated Technical Findings)

| Finding | Relevant to NPC Guides | Status |
|---------|------------------------|--------|
| VTF-1: EndAbility Requirement | Abilities must call EndAbility | N/A - NPC guides don't define GA_* details |
| VTF-2: Timer Cleanup | Clear timers before EndAbility | Stalker guide implements correctly |
| VTF-3: Active Effect Cleanup | Remove active effects | N/A |
| VTF-4: Server Authority | Use HasAuthority for server-only ops | All guides check authority correctly |
| VTF-5-10: Various | Father-specific | N/A |

### LC-1 to LC-4 (Locked Constraints)

| Constraint | Compliance |
|------------|------------|
| LC-1: No manual BP edits after generation | All guides designed for clean generation |
| LC-2: No UE source modifications | COMPLIANT |
| LC-3: No C++ abilities | COMPLIANT - Blueprint only |
| LC-4: Process lock | N/A - documentation only |

---

## TECHNICAL VALIDATION MATRIX

### Parent Class Usage

| Guide | Parent Class | Expected | Status |
|-------|--------------|----------|--------|
| Biomech | NarrativeNPCCharacter | NarrativeNPCCharacter | COMPLIANT |
| Warden | NarrativeNPCCharacter | NarrativeNPCCharacter | COMPLIANT |
| Guard | NarrativeNPCCharacter | NarrativeNPCCharacter | COMPLIANT |
| Support | NarrativeNPCCharacter | NarrativeNPCCharacter | COMPLIANT |
| Exploder | NarrativeNPCCharacter | NarrativeNPCCharacter | COMPLIANT |
| Gatherer | NarrativeNPCCharacter | NarrativeNPCCharacter | COMPLIANT |
| Stalker | NarrativeNPCCharacter | NarrativeNPCCharacter | COMPLIANT |

### Activity System Usage

| Guide | Uses NPCActivity | Uses ActivityConfiguration | Uses Goal System |
|-------|------------------|---------------------------|------------------|
| Biomech | Yes (BPA_Patrol, BPA_Attack_Melee) | Yes (AC_BiomechHostBehavior, AC_BiomechCreatureBehavior) | Yes (GoalGenerator_Attack) |
| Warden | Yes (BPA_Attack_Melee, BPA_Wander) | Yes (AC_WardenHuskBehavior, AC_WardenCoreBehavior) | Yes (GoalGenerator_Attack) |
| Guard | Yes (BPA_FollowCharacter, BPA_Attack_Melee) | Yes (AC_GuardBehavior) | Yes (Goal_FormationFollow) |
| Support | Yes (BPA_SupportFollow) | Yes (AC_SupportBufferBehavior) | Yes (GoalGenerator_Attack) |
| Exploder | Yes (BPA_Explode, BPA_Attack_Melee) | Yes (AC_ExploderBehavior) | Yes (GoalGenerator_Attack) |
| Gatherer | Yes (BPA_Alert, BPA_Wander, BPA_Flee) | Yes (AC_GathererScoutBehavior) | Yes (GoalGenerator_Alert) |
| Stalker | Yes (BPA_Patrol, BPA_FollowCharacter, BPA_Attack_Melee) | Yes (AC_ReturnedStalkerBehavior) | Yes (GoalGenerator_RandomAggression) |

### Naming Convention Compliance (Narrative Pro v2.2)

| Guide | NPCDefinition Prefix | ActivityConfiguration Suffix | Status |
|-------|---------------------|------------------------------|--------|
| Biomech | NPC_BiomechHost, NPC_BiomechCreature | AC_BiomechHostBehavior, AC_BiomechCreatureBehavior | COMPLIANT |
| Warden | NPC_WardenHusk, NPC_WardenCore | AC_WardenHuskBehavior, AC_WardenCoreBehavior | COMPLIANT |
| Guard | NPC_Guard | AC_GuardBehavior | COMPLIANT |
| Support | NPC_SupportBuffer | AC_SupportBufferBehavior | COMPLIANT |
| Exploder | NPC_PossessedExploder | AC_ExploderBehavior | COMPLIANT |
| Gatherer | NPC_GathererScout | AC_GathererScoutBehavior | COMPLIANT |
| Stalker | NPC_ReturnedStalker | AC_ReturnedStalkerBehavior | COMPLIANT |

---

## ACTION ITEMS

### Priority 1: CRITICAL

| Item | Description | Owner |
|------|-------------|-------|
| **SPAWN-001** | Add SpawnNPC node type to generator OR fix BP_BiomechHost/BP_WardenHusk manifests | Generator Team |

### Priority 2: Documentation

| Item | Description | Owner |
|------|-------------|-------|
| **DOC-001** | Add SpawnActor vs SpawnNPC warning to Generator Implementation Reference | Documentation |
| **DOC-002** | Lock the Two-Phase Death Transition pattern in LOCKED_CONTRACTS.md | Documentation |

---

## CONCLUSION

All NPC guides are technically sound and follow Narrative Pro conventions. The naming convention updates (NPC_*, AC_*Behavior) are complete across all 7 guides.

**One critical violation exists:** The manifest.yaml uses `SpawnActor` for phase transitions where `SpawnNPC` is required. This will cause generated phase-2 entities (BP_BiomechCreature, BP_WardenCore) to be non-functional.

**Recommendation:** Do not regenerate Biomech or Warden blueprints until the SpawnNPC issue is resolved. If already generated, manually edit the blueprints to use `NarrativeCharacterSubsystem.SpawnNPC()`.

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | January 2026 | Initial comprehensive audit |

---

**END OF AUDIT REPORT**
