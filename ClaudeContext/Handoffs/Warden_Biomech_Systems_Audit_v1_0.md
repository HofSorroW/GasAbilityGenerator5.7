# Warden Husk & Biomechanical Detachment Systems Audit
## Version 1.2
## Date: January 2026

---

## DOCUMENT PURPOSE

Comprehensive multi-level audit of Warden Husk and Biomechanical Detachment NPC systems against:
- Implementation Guides (v1.4 and v1.2 respectively)
- Manifest (`manifest.yaml`)
- LOCKED Contracts
- Narrative Pro native patterns
- GAS audit standards

**Scope expanded:** All NPC attribute GEs audited.

---

## EXECUTIVE SUMMARY

| System | Guide Version | Technical Compliance | Gameplay Logic | LOCKED Contracts | Overall |
|--------|---------------|---------------------|----------------|------------------|---------|
| **Warden Husk** | v1.4 | PASS | PASS (3 issues) | PASS | PASS |
| **Biomech Detachment** | v1.2 | PASS | PASS (2 issues) | PASS | PASS |
| **Other NPCs** | - | - | 3 missing attribute GEs | - | NEEDS FIX |

**Total Issues Found: 8**
- CRITICAL: 0
- HIGH: 2 (Biomech missing attribute modifiers)
- MEDIUM: 6 (Warden goal generators, HandleDeath incomplete, other NPC attributes)

---

## PART 1: WARDEN HUSK SYSTEM AUDIT

### 1.1 System Overview

| Aspect | Guide v1.4 | Manifest | Status |
|--------|------------|----------|--------|
| Concept | Humanoid guard with internal control core | Implemented | PASS |
| Phase 1 | Slow, armored, heavy melee (BP_WardenHusk) | Implemented | PASS |
| Phase 2 | Flying, fragile, laser ranged (BP_WardenCore) | Implemented | PASS |
| Parent Class | NarrativeNPCCharacter | Both use NarrativeNPCCharacter | PASS |

### 1.2 Tags Audit

| Tag | Guide | Manifest | Status |
|-----|-------|----------|--------|
| `State.Warden.Husk` | Required | Line 8983 | PRESENT |
| `State.Warden.Core` | Required | Line 8985 | PRESENT |
| `State.Warden.Ejecting` | Required | Line 8987 | PRESENT |
| `Faction.Enemy.Warden` | Required | Line 8991 | PRESENT |
| `Ability.Warden.CoreLaser` | Required | Line 8993 | PRESENT |
| `Cooldown.Warden.CoreLaser` | Required | Line 8995 | PRESENT |
| `Effect.Damage.WardenLaser` | Required | Line 8997 | PRESENT |
| `State.Warden.Attacking` | Enhancement | Line 8989 | PRESENT |

**Tags Verdict:** PASS - All required tags present

### 1.3 NPCDefinition Audit

**NPC_WardenHusk (line 7792):**

| Property | Guide | Manifest | Status |
|----------|-------|----------|--------|
| npc_id | Required | `WardenHusk` | PRESENT |
| npc_name | "Warden Husk" | "Warden Husk" | MATCH |
| npc_blueprint | BP_WardenHusk | BP_WardenHusk | MATCH |
| ability_configuration | AC_WardenHusk | AC_WardenHusk | MATCH |
| activity_configuration | AC_WardenHuskBehavior | AC_WardenHuskBehavior | MATCH |
| default_factions | Faction.Enemy.Warden | Narrative.Factions.Returned | **DECISION #1: Correct guide** |

**NPC_WardenCore (line 7802):**

| Property | Guide | Manifest | Status |
|----------|-------|----------|--------|
| npc_id | Required | `WardenCore` | PRESENT |
| npc_name | "Warden Core" | "Warden Core" | MATCH |
| npc_blueprint | BP_WardenCore | BP_WardenCore | MATCH |
| ability_configuration | AC_WardenCore | AC_WardenCore | MATCH |
| activity_configuration | AC_WardenCoreBehavior | AC_WardenCoreBehavior | MATCH |

### 1.4 AbilityConfiguration Audit

**AC_WardenHusk (line 7541):**

| Property | Guide | Manifest | Status |
|----------|-------|----------|--------|
| abilities | GA_Death | `[GA_Death]` | MATCH |
| startup_effects | GE_WardenHuskAttributes | `[GE_WardenHuskAttributes]` | MATCH |

**AC_WardenCore (line 7548):**

| Property | Guide | Manifest | Status |
|----------|-------|----------|--------|
| abilities | GA_CoreLaser | `[GA_CoreLaser, GA_Death]` | MATCH |
| startup_effects | GE_WardenCoreAttributes | `[GE_WardenCoreAttributes]` | MATCH |

### 1.5 ActivityConfiguration Audit

**AC_WardenHuskBehavior (line 7657):**

| Property | Guide | Manifest | Status |
|----------|-------|----------|--------|
| activities | BPA_Attack_Melee, BPA_Wander, BPA_Idle | `[BPA_Attack_Melee]` | **DECISION #2: Update both** |
| goal_generators | GoalGenerator_Attack | **MISSING** | **DECISION #2: Add to both** |

**AC_WardenCoreBehavior (line 7662):**

| Property | Guide | Manifest | Status |
|----------|-------|----------|--------|
| activities | BPA_Attack_Ranged | `[BPA_Attack_Ranged]` | MATCH |
| goal_generators | GoalGenerator_Attack | **MISSING** | **DECISION #3: Add to both** |

### 1.6 GameplayEffect Audit

**GE_WardenHuskAttributes (line 9018):**

| Attribute | Guide | Manifest | Status |
|-----------|-------|----------|--------|
| MaxHealth | 500.0 | 500.0 | MATCH |
| Health | 500.0 | 500.0 | MATCH |
| AttackDamage | 35.0 | 35.0 | MATCH |
| Defense/Armor | 25.0 | 25.0 (Armor) | **DECISION #4: Correct guide to use "Armor"** |

**GE_WardenCoreAttributes (line 9041):**

| Attribute | Guide | Manifest | Status |
|-----------|-------|----------|--------|
| MaxHealth | 75.0 | 75.0 | MATCH |
| Health | 75.0 | 75.0 | MATCH |
| AttackDamage | 50.0 | 50.0 | MATCH |
| Armor | 0.0 | 0.0 | MATCH |

### 1.7 Blueprint Implementation Audit

**BP_WardenHusk Variables (line 9068-9093):**

| Variable | Defined | Used in HandleDeath | Status |
|----------|---------|---------------------|--------|
| PhaseSpawnDefinition | ✅ Yes | ✅ Yes | OK |
| SpawnOffset | ✅ Yes | ❌ NOT USED | **WH-3** |
| EjectionMontage | ✅ Yes | ❌ NOT USED | **WH-3** |
| bSpawnCorpse | ✅ Yes | ❌ NOT USED | **WH-3** |
| CorpseMesh | ✅ Yes | ❌ NOT USED | **WH-3** |

**BP_WardenHusk HandleDeath (line 9103-9157):**

| Requirement | Contract | Status |
|-------------|----------|--------|
| Authority check | R-PHASE-1 | COMPLIANT |
| SpawnNPC (not SpawnActor) | R-SPAWN-1 | COMPLIANT |
| NPCDefinition variable type | R-NPCDEF-1 | COMPLIANT |
| Call parent HandleDeath | R-PHASE-1 | COMPLIANT |
| Play ejection montage | Guide Phase 29 | **MISSING - WH-3** |
| Use spawn offset | Guide | **MISSING - WH-3** |
| Spawn corpse (optional) | Guide | **MISSING - WH-3** |

**Current HandleDeath Flow (Incomplete):**
```
FunctionEntry → HasAuth → AuthBranch
  ├─ True → GetActorTransform → SpawnCore → CallParentDeath
  └─ False → CallParentDeath
```

**Expected HandleDeath Flow (per BP_BiomechHost reference pattern):**
```
FunctionEntry → HasAuth → AuthBranch
  ├─ True → CheckMontageValid → MontageBranch
  │           ├─ True → PlayMontage → GetLocation → AddOffset → MakeTransform → SpawnCore → CheckSpawnCorpse → ...
  │           └─ False → GetLocation → AddOffset → MakeTransform → SpawnCore → CheckSpawnCorpse → ...
  └─ False → CallParentDeath
```

**BP_WardenCore BeginPlay (line 9199-9230):**

| Requirement | Guide | Status |
|-------------|-------|--------|
| Set Flying mode | SetMovementMode: Flying | MATCH |

### 1.8 GA_CoreLaser Ability Audit

| Requirement | Contract | Status |
|-------------|----------|--------|
| input_tag | R-INPUTTAG-1 | COMPLIANT (`Narrative.Input.Attack`) |
| parent_class | Guide | MATCH (`NarrativeGameplayAbility`) |
| instancing_policy | Guide | MATCH (`InstancedPerActor`) |
| net_execution_policy | Guide | MATCH (`ServerOnly`) |
| cooldown_effect | Guide | MATCH (`GE_CoreLaserCooldown`) |
| MakeOutgoingGameplayEffectSpec | INV-GESPEC-1 | COMPLIANT |
| Blackboard key access | P-BB-KEY-2 | COMPLIANT |

---

## PART 2: BIOMECHANICAL DETACHMENT SYSTEM AUDIT

### 2.1 System Overview

| Aspect | Guide v1.2 | Manifest | Status |
|--------|------------|----------|--------|
| Concept | Human host with biomechanical enhancements | Implemented | PASS |
| Phase 1 | Combined entity patrols (BP_BiomechHost) | Implemented | PASS |
| Phase 2 | Machine detaches, fast melee (BP_BiomechCreature) | Implemented | PASS |
| Parent Class | NarrativeNPCCharacter | Both use NarrativeNPCCharacter | PASS |

### 2.2 Tags Audit

| Tag | Guide | Manifest | Status |
|-----|-------|----------|--------|
| `State.Biomech.Host` | Required | Line 11815 | PRESENT |
| `State.Biomech.Creature` | Required | Line 11817 | PRESENT |
| `State.Biomech.Detaching` | Required | Line 11819 | PRESENT |
| `Faction.Enemy.Biomech` | Required | Line 11821 | PRESENT |

**Tags Verdict:** PASS - All required tags present

### 2.3 NPCDefinition Audit

**NPC_BiomechHost (line 7815):**

| Property | Guide | Manifest | Status |
|----------|-------|----------|--------|
| npc_id | Required | `BiomechHost` | PRESENT |
| npc_name | "Biomech Host" | "Biomech Host" | MATCH |
| npc_blueprint | BP_BiomechHost | BP_BiomechHost | MATCH |
| ability_configuration | AC_BiomechHost | AC_BiomechHost | MATCH |
| activity_configuration | AC_BiomechHostBehavior | AC_BiomechHostBehavior | MATCH |
| default_factions | - | Narrative.Factions.Returned | OK |
| default_owned_tags | Faction.Enemy.Biomech | `[Faction.Enemy.Biomech]` | **DECISION #5: Change to Narrative.Factions.Returned** |

**NPC_BiomechCreature (line 7827):**

| Property | Guide | Manifest | Status |
|----------|-------|----------|--------|
| npc_id | Required | `BiomechCreature` | PRESENT |
| npc_name | "Biomech Creature" | "Biomech Creature" | MATCH |
| npc_blueprint | BP_BiomechCreature | BP_BiomechCreature | MATCH |
| ability_configuration | AC_BiomechCreature | AC_BiomechCreature | MATCH |
| activity_configuration | AC_BiomechCreatureBehavior | AC_BiomechCreatureBehavior | MATCH |
| default_owned_tags | Faction.Enemy.Biomech | `[Faction.Enemy.Biomech]` | **DECISION #5: Change to Narrative.Factions.Returned** |

### 2.4 AbilityConfiguration Audit

**AC_BiomechHost (line 7559):**

| Property | Guide | Manifest | Status |
|----------|-------|----------|--------|
| abilities | GA_Death | `[GA_Death]` | MATCH |
| startup_effects | GE_BiomechHostAttributes | `[GE_BiomechHostAttributes]` | MATCH |

**AC_BiomechCreature (line 7566):**

| Property | Guide | Manifest | Status |
|----------|-------|----------|--------|
| abilities | GA_Death | `[GA_Death]` | MATCH |
| startup_effects | GE_BiomechCreatureAttributes | `[GE_BiomechCreatureAttributes]` | MATCH |

### 2.5 ActivityConfiguration Audit

**AC_BiomechHostBehavior (line 7671):**

| Property | Guide | Manifest | Status |
|----------|-------|----------|--------|
| activities | BPA_Patrol, BPA_Attack_Melee, BPA_Idle | `[BPA_Patrol, BPA_Attack_Melee, BPA_Idle]` | EXACT MATCH |
| goal_generators | GoalGenerator_Attack | `[GoalGenerator_Attack]` | EXACT MATCH |

**AC_BiomechCreatureBehavior (line 7680):**

| Property | Guide | Manifest | Status |
|----------|-------|----------|--------|
| activities | BPA_Attack_Melee, BPA_Idle | `[BPA_Attack_Melee, BPA_Idle]` | EXACT MATCH |
| goal_generators | GoalGenerator_Attack | `[GoalGenerator_Attack]` | EXACT MATCH |

### 2.6 GameplayEffect Audit

**GE_BiomechHostAttributes (line 11825):**

| Property | Guide | Manifest | Status |
|----------|-------|----------|--------|
| duration_policy | Instant | Instant | MATCH |
| modifiers | Custom attributes | **MISSING** | **DECISION #6: Add 250 HP, 28 ATK, 12 Armor** |

**GE_BiomechCreatureAttributes (line 11829):**

| Property | Guide | Manifest | Status |
|----------|-------|----------|--------|
| duration_policy | Instant | Instant | MATCH |
| modifiers | Custom attributes | **MISSING** | **DECISION #7: Add 85 HP, 38 ATK, 0 Armor** |

### 2.7 Blueprint Implementation Audit

**BP_BiomechHost HandleDeath (line 11875-12186):**

| Requirement | Contract | Status |
|-------------|----------|--------|
| Authority check | R-PHASE-1 | COMPLIANT |
| Add detaching tag | Guide Phase 5.3 | COMPLIANT |
| Play montage | Guide Phase 5.4 | COMPLIANT |
| Calculate spawn transform | Guide Phase 5.6-5.7 | COMPLIANT |
| SpawnNPC (not SpawnActor) | R-SPAWN-1 | COMPLIANT |
| NPCDefinition variable type | R-NPCDEF-1 | COMPLIANT |
| Spawn corpse | Guide Phase 5.8 | COMPLIANT |
| Call parent HandleDeath | R-PHASE-1 | COMPLIANT |

**BP_BiomechCreature BeginPlay (line 12200-12273):**

| Requirement | Status |
|-------------|--------|
| Speed boost (1.5x) | ENHANCEMENT - **DECISION #8: Add to guide** |

---

## PART 3: ALL NPC ATTRIBUTE GE AUDIT

### 3.1 Complete NPC Attribute Status

| NPC | AbilityConfig | startup_effects | GE Exists | Has Modifiers | Status |
|-----|---------------|-----------------|-----------|---------------|--------|
| NPC_FatherCompanion | AC_FatherCompanion | GE_CrawlerState | Yes | Yes | OK |
| NPC_GathererScout | AC_GathererScout | None | - | - | OK (non-combatant) |
| NPC_Reinforcement | AC_Reinforcement | **None** | No | - | **DECISION #9** |
| NPC_ReturnedStalker | AC_ReturnedStalker | **None** | No | - | **DECISION #10** |
| NPC_SupportBuffer | AC_SupportBuffer | GE_SupporterAttributes | Yes | Yes (150 HP, 20 Armor) | OK |
| NPC_FormationGuard | AC_FormationGuard | **None** | No | - | **DECISION #11** |
| NPC_PossessedExploder | AC_PossessedExploder | GE_ExploderAttributes | Yes | Yes (50 HP, 0 Armor) | OK |
| NPC_WardenHusk | AC_WardenHusk | GE_WardenHuskAttributes | Yes | Yes (500 HP, 35 ATK, 25 Armor) | OK |
| NPC_WardenCore | AC_WardenCore | GE_WardenCoreAttributes | Yes | Yes (75 HP, 50 ATK, 0 Armor) | OK |
| NPC_BiomechHost | AC_BiomechHost | GE_BiomechHostAttributes | Yes | **EMPTY** | **DECISION #6** |
| NPC_BiomechCreature | AC_BiomechCreature | GE_BiomechCreatureAttributes | Yes | **EMPTY** | **DECISION #7** |

### 3.2 Reference: Existing NPC Attribute Values

| NPC | HP | ATK | Armor | Notes |
|-----|----|-----|-------|-------|
| Warden Husk | 500 | 35 | 25 | Slow, armored melee boss |
| Warden Core | 75 | 50 | 0 | Fragile flying ranged |
| Exploder | 50 | - | 0 | Suicide bomber |
| Supporter | 150 | - | 20 | Support buffer |

---

## PART 4: LOCKED CONTRACTS COMPLIANCE

| Contract | Warden Husk | Biomech Detachment |
|----------|-------------|-------------------|
| R-SPAWN-1 (SpawnNPC-only) | COMPLIANT | COMPLIANT |
| R-PHASE-1 (Death Transition) | COMPLIANT | COMPLIANT |
| R-NPCDEF-1 (Variable Type) | COMPLIANT | COMPLIANT |
| R-INPUTTAG-1 (NPC Combat) | COMPLIANT | N/A (uses NP defaults) |
| P-BB-KEY-2 (BB Key Access) | COMPLIANT | N/A (uses NP defaults) |
| INV-GESPEC-1 (MakeOutgoing) | COMPLIANT | N/A (uses NP defaults) |
| Contract 25 (No Simplify) | COMPLIANT | COMPLIANT |

**Note on Biomech N/A Contracts:**
- Biomech system uses Narrative Pro default melee attacks (no custom GA_Attack ability)
- GoalGenerator_Attack → BPA_Attack_Melee → NP's built-in melee attack system
- No custom projectile or blackboard key usage = these contracts don't apply

---

## PART 5: ALL ISSUES FOUND

| ID | Severity | System | Category | Issue | Decision |
|----|----------|--------|----------|-------|----------|
| WH-1 | MEDIUM | Warden | ActivityConfig | AC_WardenHuskBehavior missing activities & goal_generators | **DECIDED: Update both** |
| WH-2 | MEDIUM | Warden | ActivityConfig | AC_WardenCoreBehavior missing goal_generators | **DECIDED: Update both** |
| WH-3 | MEDIUM | Warden | Blueprint | BP_WardenHusk HandleDeath doesn't use EjectionMontage, SpawnOffset, bSpawnCorpse, CorpseMesh | **DECIDED: Fix manifest & guide** |
| BM-1 | HIGH | Biomech | GameplayEffect | GE_BiomechHostAttributes has no modifiers | **DECIDED: Add 250 HP, 28 ATK, 12 Armor** |
| BM-2 | HIGH | Biomech | GameplayEffect | GE_BiomechCreatureAttributes has no modifiers | **DECIDED: Add 85 HP, 38 ATK, 0 Armor** |
| NEW-1 | MEDIUM | Reinforcement | GameplayEffect | No startup_effects, no GE | **DECIDED: 120 HP, 25 ATK, 5 Armor** |
| NEW-2 | MEDIUM | ReturnedStalker | GameplayEffect | No startup_effects, no GE | **DECIDED: 100 HP, 30 ATK, 0 Armor** |
| NEW-3 | MEDIUM | FormationGuard | GameplayEffect | No startup_effects, no GE | **DECIDED: 150 HP, 20 ATK, 15 Armor** |

---

## PART 6: ERDEM'S DECISIONS LOG (FINAL)

| # | Item | Decision | Update Target |
|---|------|----------|---------------|
| 1 | Faction Deviation (Warden) | Use `Narrative.Factions.Returned` | Guide only |
| 2 | WH-1: AC_WardenHuskBehavior | Use BPA_Patrol (not BPA_Wander), add BPA_Idle, add GoalGenerator_Attack | Both guide & manifest |
| 3 | WH-2: AC_WardenCoreBehavior | Add GoalGenerator_Attack | Both guide & manifest |
| 4 | Attribute naming | Use "Armor" (not "Defense") | Guide only |
| 5 | Biomech Faction Tags | Change `default_owned_tags` from `Faction.Enemy.Biomech` to `Narrative.Factions.Returned` | Both guide & manifest |
| 6 | BM-1: GE_BiomechHostAttributes | Add modifiers: 250 HP, 28 ATK, 12 Armor | Both guide & manifest |
| 7 | BM-2: GE_BiomechCreatureAttributes | Add modifiers: 85 HP, 38 ATK, 0 Armor | Both guide & manifest |
| 8 | BP_BiomechCreature speed boost | Add 1.5x CreatureSpeedMultiplier to BeginPlay | Guide only |
| 9 | NEW-1: NPC_Reinforcement attributes | Create GE_ReinforcementAttributes: 120 HP, 25 ATK, 5 Armor | Manifest only |
| 10 | NEW-2: NPC_ReturnedStalker attributes | Create GE_ReturnedStalkerAttributes: 100 HP, 30 ATK, 0 Armor | Manifest only |
| 11 | NEW-3: NPC_FormationGuard attributes | Create GE_FormationGuardAttributes: 150 HP, 20 ATK, 15 Armor | Manifest only |
| 12 | Biomech N/A Contracts | Keep NP defaults - no custom attack abilities needed | No change |
| 13 | WH-3: BP_WardenHusk HandleDeath | Add full ejection montage, spawn offset, corpse logic (match BP_BiomechHost pattern) | Both guide & manifest |

---

## PART 7: DETAILED FIX SPECIFICATIONS

### Fix WH-1 & WH-2: Update Warden ActivityConfigs

**Manifest changes:**
```yaml
- name: AC_WardenHuskBehavior
  folder: Configs
  activities:
    - BPA_Attack_Melee
    - BPA_Patrol
    - BPA_Idle
  goal_generators:
    - GoalGenerator_Attack

- name: AC_WardenCoreBehavior
  folder: Configs
  activities:
    - BPA_Attack_Ranged
  goal_generators:
    - GoalGenerator_Attack
```

### Fix WH-3: BP_WardenHusk HandleDeath Full Implementation

**Variables already defined (OK):**
- PhaseSpawnDefinition (Object:NPCDefinition) - for NPC_WardenCore
- SpawnOffset (Vector) - default (0, 0, 100) for core ejection
- EjectionMontage (AnimMontage) - ejection animation
- bSpawnCorpse (Boolean) - optional corpse mesh
- CorpseMesh (StaticMesh) - husk shell mesh

**HandleDeath needs to match BP_BiomechHost pattern:**
1. Authority check ✓ (already implemented)
2. Add State.Warden.Ejecting tag (MISSING)
3. Check EjectionMontage valid → Play if valid (MISSING)
4. Get actor location + SpawnOffset → Make transform (MISSING - currently uses GetTransform only)
5. SpawnNPC ✓ (already implemented)
6. Check bSpawnCorpse → Spawn corpse mesh (MISSING)
7. Call parent ✓ (already implemented)

### Fix BM-1: Add Biomech Host Attributes

```yaml
- name: GE_BiomechHostAttributes
  folder: Enemies/Biomech/Effects
  duration_policy: Instant
  modifiers:
    - attribute: NarrativeAttributeSetBase.MaxHealth
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 250.0
    - attribute: NarrativeAttributeSetBase.Health
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 250.0
    - attribute: NarrativeAttributeSetBase.AttackDamage
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 28.0
    - attribute: NarrativeAttributeSetBase.Armor
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 12.0
```

### Fix BM-2: Add Biomech Creature Attributes

```yaml
- name: GE_BiomechCreatureAttributes
  folder: Enemies/Biomech/Effects
  duration_policy: Instant
  modifiers:
    - attribute: NarrativeAttributeSetBase.MaxHealth
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 85.0
    - attribute: NarrativeAttributeSetBase.Health
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 85.0
    - attribute: NarrativeAttributeSetBase.AttackDamage
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 38.0
    - attribute: NarrativeAttributeSetBase.Armor
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 0.0
```

### Fix NEW-1/2/3: Create Missing NPC Attribute GEs

```yaml
# NEW-1: Reinforcement - standard ranged combatant
- name: GE_ReinforcementAttributes
  folder: Enemies/Gatherer/Effects
  duration_policy: Instant
  modifiers:
    - attribute: NarrativeAttributeSetBase.MaxHealth
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 120.0
    - attribute: NarrativeAttributeSetBase.Health
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 120.0
    - attribute: NarrativeAttributeSetBase.AttackDamage
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 25.0
    - attribute: NarrativeAttributeSetBase.Armor
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 5.0

# NEW-2: Returned Stalker - glass cannon assassin
- name: GE_ReturnedStalkerAttributes
  folder: Enemies/Returned/Effects
  duration_policy: Instant
  modifiers:
    - attribute: NarrativeAttributeSetBase.MaxHealth
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 100.0
    - attribute: NarrativeAttributeSetBase.Health
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 100.0
    - attribute: NarrativeAttributeSetBase.AttackDamage
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 30.0
    - attribute: NarrativeAttributeSetBase.Armor
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 0.0

# NEW-3: Formation Guard - tanky protector
- name: GE_FormationGuardAttributes
  folder: Enemies/Formation/Effects
  duration_policy: Instant
  modifiers:
    - attribute: NarrativeAttributeSetBase.MaxHealth
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 150.0
    - attribute: NarrativeAttributeSetBase.Health
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 150.0
    - attribute: NarrativeAttributeSetBase.AttackDamage
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 20.0
    - attribute: NarrativeAttributeSetBase.Armor
      modifier_op: Override
      magnitude_type: ScalableFloat
      magnitude_value: 15.0
```

---

## PART 8: IMPLEMENTATION STATUS

| Issue ID | Status | Implemented By | Date |
|----------|--------|----------------|------|
| WH-1 | PENDING | - | - |
| WH-2 | PENDING | - | - |
| WH-3 | PENDING | - | - |
| BM-1 | PENDING | - | - |
| BM-2 | PENDING | - | - |
| NEW-1 | PENDING | - | - |
| NEW-2 | PENDING | - | - |
| NEW-3 | PENDING | - | - |

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | January 2026 | Initial audit completed |
| 1.1 | January 2026 | Added Erdem's decisions (1-8), expanded scope to all NPC attribute GEs, found 3 additional NPCs missing attributes (NEW-1/2/3) |
| 1.2 | January 2026 | Added WH-3 (BP_WardenHusk HandleDeath incomplete), finalized all 13 decisions, added detailed fix specifications |

---

**END OF HANDOFF**
