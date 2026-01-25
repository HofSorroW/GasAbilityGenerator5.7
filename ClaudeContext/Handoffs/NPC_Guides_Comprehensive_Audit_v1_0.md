# NPC Guides Comprehensive Audit Report
## Multi-Level Technical, Logical, and Gameplay Validation
## Version 1.2

---

## DOCUMENT INFORMATION

| Field | Value |
|-------|-------|
| Document Type | Audit Report |
| Scope | All NPC Implementation Guides |
| Date | January 2026 |
| Auditor | Claude (multi-level consistency audit) |
| Status | **RESOLVED** (v4.34) |

---

## EXECUTIVE SUMMARY

This audit validates NPC implementation guides against manifest.yaml, tech docs, design docs, LOCKED_CONTRACTS.md, and Father_Companion_GAS_Abilities_Audit.md.

### Resolution Status (v4.34)

| Severity | Issue | Resolution |
|----------|-------|------------|
| ~~CRITICAL~~ | ~~SpawnActor used instead of SpawnNPC~~ | **FIXED** - SpawnNPC node type added, manifest updated |

**v4.34 Changes:**
- Added `SpawnNPC` node type to generator (calls `NarrativeCharacterSubsystem.SpawnNPC()`)
- Updated BP_BiomechHost and BP_WardenHusk manifests to use SpawnNPC
- All 187 manifest assets now generate successfully

---

## GUIDES AUDITED

> **POLICY:** No manual creation allowed. All guides must have full manifest automation.

| Guide | Version | Manifest Entries | Status |
|-------|---------|------------------|--------|
| Biomechanical_Detachment_System_Implementation_Guide | v1.2 | BP_BiomechHost, BP_BiomechCreature | **COMPLIANT** (v4.34) |
| Warden_Husk_System_Implementation_Guide | v1.3 | BP_WardenHusk, BP_WardenCore | **COMPLIANT** (v4.34) |
| Guard_Formation_Follow_System_Implementation_Guide | v2.5 | BTS_CalculateFormationPosition, BTS_AdjustFormationSpeed, Goal_FormationFollow | **COMPLIANT** |
| Support_Buffer_Healer_NPC_Implementation_Guide | v1.1 | BTS_HealNearbyAllies, BPA_SupportFollow | COMPLIANT |
| Possessed_Exploder_Enemy_Implementation_Guide | v2.2 | BTS_CheckExplosionProximity, BPA_Explode | COMPLIANT |
| Gatherer_Scout_Alert_System_Implementation_Guide | v1.2 | GoalGenerator_Alert, BPA_Alert, NPC_GathererScout | **COMPLIANT** |
| Random_Aggression_Stalker_System_Implementation_Guide | v2.2 | GoalGenerator_RandomAggression, NPC_ReturnedStalker, TaggedDialogueSet_Returned | **COMPLIANT** |

---

## RESOLVED: SpawnActor → SpawnNPC (v4.34)

### The Problem (Now Fixed)

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

### v4.34 Resolution

**Generator Enhancement:**
- Added `CreateSpawnNPCNode` function to `GasAbilityGeneratorGenerators.cpp`
- Creates `GetSubsystem<UNarrativeCharacterSubsystem>` + `SpawnNPC` nodes
- Auto-wires subsystem output to SpawnNPC target
- Exposes `NPCData` and `Transform` pins for manifest connections

**Updated Manifest (BP_BiomechHost):**
```yaml
variables:
  # NPCDefinition for spawning BiomechCreature on death via SpawnNPC
  - name: PhaseSpawnDefinition
    type: Object
    class: NPCDefinition
    instance_editable: true

# In function_overrides.HandleDeath:
- id: SpawnCreature
  type: SpawnNPC
  properties:
    npc_definition_variable: PhaseSpawnDefinition
  position: [800, 0]

connections:
  - from: [GetSpawnTransform, ReturnValue]
    to: [SpawnCreature, Transform]
  - from: [SpawnCreature, ReturnValue]
    to: [IsValid_Spawned, Object]
```

**Updated Manifest (BP_WardenHusk):**
```yaml
variables:
  # NPCDefinition for spawning WardenCore on death via SpawnNPC
  - name: PhaseSpawnDefinition
    type: Object
    class: NPCDefinition
    instance_editable: true

# In function_overrides.HandleDeath:
- id: SpawnCore
  type: SpawnNPC
  properties:
    npc_definition_variable: PhaseSpawnDefinition
  position: [800, 0]

connections:
  - from: [GetSpawnTransform, ReturnValue]
    to: [SpawnCore, Transform]
  - from: [SpawnCore, ReturnValue]
    to: [IsValid_Spawned, Object]
```

### Impact Assessment (Now Resolved)

| Component | ~~With SpawnActor~~ | With SpawnNPC (v4.34) |
|-----------|---------------------|----------------------|
| NPCDefinition | ~~null~~ | **Properly set** |
| AbilityConfiguration | ~~Not applied~~ | **Applied** |
| ActivityConfiguration | ~~Not applied~~ | **Applied** |
| Faction Tags | ~~Not set~~ | **Set from NPCDef** |
| Subsystem Registration | ~~Not registered~~ | **Registered** |
| AI Behavior | ~~Non-functional~~ | **Functional** |

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
| Spawn Method | **SpawnNPC in manifest (v4.34)** | **COMPLIANT** |

**Expected Flow:**
1. Entity dies -> OnDied delegate fires
2. HandleDeath override executes
3. Server authority check
4. **SpawnNPC** creates phase-2 entity with full NPC initialization
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
| Goal_FormationFollow | Custom goal for formation slots | **Manifest automated** |
| BTS_CalculateFormationPosition | Formation position calculation service | **Manifest automated** |
| BTS_AdjustFormationSpeed | Speed adjustment service | **Manifest automated** |
| Activity System | Uses standard Narrative Pro activities | COMPLIANT |

**Locked Status:** Not locked.

### Pattern 5: Alert-and-Reinforce (Gatherer Scout)

**Used By:** Gatherer_Scout

| Component | Implementation | Status |
|-----------|----------------|--------|
| GoalGenerator_Alert | Creates alert goals from perception | **Manifest automated** |
| BPA_Alert | Signal animation + spawn reinforcements | **Manifest automated** |
| NPC_GathererScout | NPCDefinition | **Manifest automated** |
| SpawnNPC | Correctly documented in guide | COMPLIANT |

**Locked Status:** Not locked.

### Pattern 6: Random Aggression Bond (Stalker)

**Used By:** Random_Aggression_Stalker

| Component | Implementation | Status |
|-----------|----------------|--------|
| GoalGenerator_RandomAggression | Stacking bond system with timers | **Manifest automated** |
| NPC_ReturnedStalker | NPCDefinition | **Manifest automated** |
| TaggedDialogueSet_Returned | Contextual barks | **Manifest automated** |
| Tagged Dialogue | Integration with TaggedDialogueSet | COMPLIANT |
| Defend Mechanic | Player damage binding | COMPLIANT |
| Timer System | Multiple independent timers | **See R-TIMER-1 analysis** |

**Timer Pattern (R-TIMER-1 Analysis):**
The guide uses Set Timer by Event with looping enabled - this is acceptable for BTService-like periodic checks but requires proper cleanup in StopFollowing function. The guide correctly implements:
- FollowCheckTimerHandle cleanup
- TalkCheckTimerHandle cleanup
- AttackCheckTimerHandle cleanup
- FollowDurationTimerHandle cleanup

**Locked Status:** Not locked - timer cleanup is properly documented.

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
| Guard | Yes (BPA_FollowCharacter, BPA_Attack_Melee) | Yes (AC_FormationGuardBehavior) | Yes (Goal_FormationFollow) |
| Support | Yes (BPA_SupportFollow) | Yes (AC_SupportBufferBehavior) | Yes (GoalGenerator_Attack) |
| Exploder | Yes (BPA_Explode, BPA_Attack_Melee) | Yes (AC_PossessedExploderBehavior) | Yes (GoalGenerator_Attack) |
| Gatherer | Yes (BPA_Alert, BPA_Wander, BPA_Flee) | Yes (AC_GathererScoutBehavior) | Yes (GoalGenerator_Alert) |
| Stalker | Yes (BPA_Patrol, BPA_FollowCharacter, BPA_Attack_Melee) | Yes (AC_ReturnedStalkerBehavior) | Yes (GoalGenerator_RandomAggression) |

### Naming Convention Compliance (Narrative Pro v2.2)

| Guide | NPCDefinition Prefix | ActivityConfiguration Suffix | Status |
|-------|---------------------|------------------------------|--------|
| Biomech | NPC_BiomechHost, NPC_BiomechCreature | AC_BiomechHostBehavior, AC_BiomechCreatureBehavior | COMPLIANT |
| Warden | NPC_WardenHusk, NPC_WardenCore | AC_WardenHuskBehavior, AC_WardenCoreBehavior | COMPLIANT |
| Guard | NPC_FormationGuard | AC_FormationGuardBehavior | COMPLIANT |
| Support | NPC_SupportBuffer | AC_SupportBufferBehavior | COMPLIANT |
| Exploder | NPC_PossessedExploder | AC_PossessedExploderBehavior | COMPLIANT |
| Gatherer | NPC_GathererScout | AC_GathererScoutBehavior | COMPLIANT |
| Stalker | NPC_ReturnedStalker | AC_ReturnedStalkerBehavior | COMPLIANT |

---

## ACTION ITEMS

### Priority 1: CRITICAL - **RESOLVED**

| Item | Description | Status |
|------|-------------|--------|
| ~~SPAWN-001~~ | ~~Add SpawnNPC node type to generator~~ | **RESOLVED (v4.34)** |

### Priority 2: AUTOMATION GAPS - **RESOLVED**

> **POLICY:** All NPC systems must be fully automatable via manifest.yaml. Manual Blueprint creation is NOT acceptable.

| Item | Guide | Status |
|------|-------|--------|
| ~~AUTO-001~~ | ~~Guard_Formation_Follow~~ | **RESOLVED** - BTS_CalculateFormationPosition, BTS_AdjustFormationSpeed, Goal_FormationFollow in manifest |
| ~~AUTO-002~~ | ~~Gatherer_Scout~~ | **RESOLVED** - GoalGenerator_Alert, BPA_Alert, NPC_GathererScout in manifest |
| ~~AUTO-003~~ | ~~Random_Aggression_Stalker~~ | **RESOLVED** - GoalGenerator_RandomAggression, NPC_ReturnedStalker, TaggedDialogueSet_Returned in manifest |

### Priority 3: Documentation

| Item | Description | Status |
|------|-------------|--------|
| DOC-001 | Add SpawnActor vs SpawnNPC warning to Generator Implementation Reference | PENDING |
| DOC-002 | Lock the Two-Phase Death Transition pattern in LOCKED_CONTRACTS.md | PENDING |

---

## SUGGESTED LOCKS (v4.34 Session Findings)

Based on the v4.34 implementation session, the following patterns should be considered for addition to `LOCKED_CONTRACTS.md`:

### LOCK-SUGGEST-1: SpawnNPC for NPC Spawning (R-SPAWN-1)

**Pattern:** All NPC spawning in manifest blueprints MUST use `SpawnNPC` node type, NOT `SpawnActor`.

**Rationale:**
Raw `SpawnActor` does NOT call `SetNPCDefinition()` on the spawned NPC. Without this:
- NPCDefinition property is null
- AbilityConfiguration is never applied (no abilities granted)
- ActivityConfiguration is never applied (no AI activities)
- Faction tags are never set
- NPC is not registered in the NarrativeCharacterSubsystem

**Correct Pattern:**
```yaml
variables:
  - name: SpawnDefinition
    type: Object           # NOT TSubclassOf
    class: NPCDefinition   # NPCDefinition instance reference

nodes:
  - id: SpawnNPC
    type: SpawnNPC
    properties:
      npc_definition_variable: SpawnDefinition
```

**Wrong Pattern (LOCKED - DO NOT USE):**
```yaml
variables:
  - name: SpawnClass
    type: Class
    class: NarrativeNPCCharacter

nodes:
  - id: SpawnActor
    type: SpawnActor
    properties:
      actor_class_variable: SpawnClass
```

**Severity:** CRITICAL - Spawned NPCs will be non-functional shells.

---

### LOCK-SUGGEST-2: Two-Phase Death Transition (R-PHASE-1)

**Pattern:** Multi-phase enemies (Warden Husk → Core, Biomech Host → Creature) MUST:
1. Override `HandleDeath` function
2. Check `HasAuthority()` before spawning
3. Use `SpawnNPC` (not SpawnActor) for phase-2 entity
4. Call parent `HandleDeath` after spawn

**Correct Pattern:**
```yaml
function_overrides:
  - name: HandleDeath
    nodes:
      - id: AuthCheck
        type: CallFunction
        properties:
          function: HasAuthority
      - id: Branch_Auth
        type: Branch
      - id: SpawnPhase2
        type: SpawnNPC
        properties:
          npc_definition_variable: PhaseSpawnDefinition
      - id: CallParent
        type: CallParent
    connections:
      - from: [AuthCheck, ReturnValue]
        to: [Branch_Auth, Condition]
      - from: [Branch_Auth, True]
        to: [SpawnPhase2, Exec]
      - from: [SpawnPhase2, Exec]
        to: [CallParent, Exec]
```

**Severity:** HIGH - Incorrect phase transitions break enemy progression.

---

### LOCK-SUGGEST-3: Delegate Binding CustomEvent Signature (R-DELEGATE-1)

**Pattern:** When using `delegate_bindings`, the corresponding CustomEvent in `event_graph` MUST have parameters matching the delegate signature EXACTLY.

**Rationale:**
- Event graph connections are made BEFORE delegate_bindings runs
- Connections reference pin names that must exist on the CustomEvent
- Parameter names must match delegate signature (e.g., `DamagerCauserASC`, `Damage`, `Spec` for OnDamagedBy)

**Delegate Signatures (Narrative Pro v2.2):**
| Delegate | Parameters |
|----------|------------|
| OnDamagedBy | `DamagerCauserASC: NarrativeAbilitySystemComponent`, `Damage: Float`, `Spec: GameplayEffectSpec` |
| OnDealtDamage | `DamagedASC: NarrativeAbilitySystemComponent`, `Damage: Float`, `Spec: GameplayEffectSpec` |
| OnDied | `KilledActor: Actor`, `KilledActorASC: NarrativeAbilitySystemComponent` |
| OnHealedBy | `Healer: NarrativeAbilitySystemComponent`, `Amount: Float`, `Spec: GameplayEffectSpec` |

**Correct Pattern:**
```yaml
delegate_bindings:
  - id: BindDamage
    delegate: OnDamagedBy
    source: OwnerASC
    handler: HandleDamage

event_graph:
  nodes:
    - id: Event_HandleDamage
      type: CustomEvent
      properties:
        event_name: HandleDamage
        parameters:
          - name: DamagerCauserASC       # MUST match delegate
            type: NarrativeAbilitySystemComponent
          - name: Damage                 # MUST match delegate
            type: Float
          - name: Spec                   # MUST match delegate
            type: GameplayEffectSpec
```

**Severity:** HIGH - Mismatched signatures cause "Signature Error" compile failures.

---

### LOCK-SUGGEST-4: NPCDefinition Variable Type (R-NPCDEF-1)

**Pattern:** Variables storing NPCDefinition references for SpawnNPC MUST use `type: Object` with `class: NPCDefinition`, NOT `type: Class`.

**Rationale:**
- `SpawnNPC()` takes `UNPCDefinition*` (object instance), not `TSubclassOf<>`
- Using `type: Class` creates a class reference pin, not an object reference
- The SpawnNPC node expects `NPCData` pin to receive an object

**Correct:**
```yaml
variables:
  - name: PhaseSpawnDefinition
    type: Object
    class: NPCDefinition
    instance_editable: true
```

**Wrong:**
```yaml
variables:
  - name: PhaseSpawnClass
    type: Class
    class: NarrativeNPCCharacter
```

**Severity:** MEDIUM - Wrong type causes pin connection failures.

---

## CONCLUSION

All NPC guides are technically sound, follow Narrative Pro conventions, and are fully automated via manifest.yaml.

### Issues Summary (v4.34)

| Issue Type | Count | Resolution |
|------------|-------|------------|
| ~~SpawnActor Violation~~ | ~~2~~ | **RESOLVED** - SpawnNPC node added |
| ~~Missing Automation~~ | ~~3~~ | **RESOLVED** - All guides have manifest entries |
| **Fully Compliant** | **7** | All guides passing |

### v4.34 Resolution Summary

1. **SpawnNPC Node Type:** Added to generator - creates `GetSubsystem<UNarrativeCharacterSubsystem>` + `SpawnNPC()` call chain with proper wiring.

2. **Manifest Updates:**
   - BP_BiomechHost: Variable changed to NPCDefinition, uses SpawnNPC node
   - BP_WardenHusk: Variable changed to NPCDefinition, uses SpawnNPC node

3. **Generation Result:** 187/187 assets passing (0 failures)

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 1.2 | January 2026 | **v4.34 Resolution:** SpawnNPC node type added to generator. BP_BiomechHost and BP_WardenHusk fixed. All 7 guides now COMPLIANT. Updated action items to RESOLVED. |
| 1.1 | January 2026 | Added NO MANUAL CREATION policy. Marked Guard, Gatherer, Stalker as NEEDS AUTOMATION. Added Priority 2 action items for automation gaps. |
| 1.0 | January 2026 | Initial comprehensive audit |

---

**END OF AUDIT REPORT**
