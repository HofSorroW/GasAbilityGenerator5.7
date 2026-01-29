# Manifest Comprehensive Audit
## Cross-Reference: manifest.yaml vs All NPC Implementation Guides
## Version 1.0

---

## DOCUMENT INFORMATION

| Field | Value |
|-------|-------|
| Document Type | Comprehensive Audit |
| Reference File | ClaudeContext/manifest.yaml |
| Guides Audited | 7 NPC Implementation Guides |
| Audit Date | January 2026 |
| Manifest Version | 3.0.3 |
| Manifest Lines | 15,717 |

---

## EXECUTIVE SUMMARY

### Overall Compliance Score

| Category | Score | Status |
|----------|-------|--------|
| NPCDefinitions | 11/11 | PASS |
| AbilityConfigurations | 11/11 | PASS |
| ActivityConfigurations | 11/11 | PASS |
| GameplayEffects (Attributes) | 11/11 | PASS |
| BehaviorTrees | 5/5 | PASS |
| Activities | 4/4 | PASS |
| Goals | 2/2 | PASS |
| GoalGenerators | 2/2 | PASS |
| Attribute Values | 9/9 | PASS |

### Findings Summary

| ID | Severity | Area | Finding | Status |
|----|----------|------|---------|--------|
| MAUD-01 | MEDIUM | AC_GathererScout | Missing attributes GE - Scout needs Health to be killable | **FIXED** |
| MAUD-02 | PASS | All Attribute Values | All 10 NPCs match guide specifications | VERIFIED |
| MAUD-03 | PASS | Faction Consistency | All NPCs use Narrative.Factions.Returned | VERIFIED |

### Remediation Applied (v1.1)

**MAUD-01 Fix:** Added `GE_GathererScoutAttributes` (75/75/0/0) and updated `AC_GathererScout` with `startup_effects`. Scout is non-combatant but needs Health to be killable for loot.

---

## MANIFEST INVENTORY

### Total Assets by Type

| Asset Type | Count | Section Lines |
|------------|-------|---------------|
| Gameplay Tags | 202 | 76-423 |
| Input Actions | 7 | 427-468 |
| Input Mapping Contexts | 1 | 490-509 |
| Enumerations | 3 | 514-541 |
| Float Curves | 1 | 545-568 |
| Gameplay Effects | 30+ | 589-887, 9098-9143, etc. |
| Gameplay Abilities | 18 | 891-7228 |
| Equippable Items | 9 | 7233-7368 |
| Blackboards | 2+ | 7372-7429, 9839-9854 |
| Behavior Trees | 5 | 7433-7510, 9855-9887, 11235-11272, 12113-12150 |
| Activities | 4 | 7515-7520, 9888-10045, 11273-11293, 12151-12170 |
| Ability Configurations | 11 | 7524-7640 |
| Activity Configurations | 11 | 7644-7769 |
| NPC Definitions | 11 | 7773-7920 |
| Character Definitions | 1 | 7924-7929 |
| Item Collections | 2 | 7933-7959 |
| Narrative Events | 2 | 7963-7973 |
| Actor Blueprints | 10+ | 7994-8133, 9144-9354, etc. |
| Widget Blueprints | 3 | 8137-8298 |
| Niagara Systems | 7 | 8362-8555 |
| Material Functions | 3 | 8624-8783 |
| Materials | 4 | 8788-8898 |
| Goal Items | 2 | 12775-13145, 13147-13195 |
| Goal Generators | 2 | 12811-13145, 13925-15565 |
| Tagged Dialogue Sets | 1 | 15571-15717 |

---

## NPCDefinition AUDIT

### Comparison Matrix

| NPC | Guide | Manifest | npc_id | ability_config | activity_config | factions |
|-----|-------|----------|--------|----------------|-----------------|----------|
| NPC_GathererScout | Gatherer v1.3 | Line 7787 | GathererScout | AC_GathererScout | AC_GathererScoutBehavior | Returned |
| NPC_Reinforcement | Gatherer v1.3 | Line 7801 | Reinforcement | AC_Reinforcement | AC_RunAndGun | Returned |
| NPC_ReturnedStalker | Stalker v2.4 | Line 7816 | ReturnedStalker | AC_ReturnedStalker | AC_ReturnedStalkerBehavior | Returned |
| NPC_SupportBuffer | Buffer v1.4 | Line 7833 | SupportBuffer | AC_SupportBuffer | AC_SupportBufferBehavior | Returned |
| NPC_FormationGuard | Guard v2.7 | Line 7848 | FormationGuard | AC_FormationGuard | AC_FormationGuardBehavior | Returned |
| NPC_PossessedExploder | Exploder v2.3 | Line 7861 | PossessedExploder | AC_PossessedExploder | AC_PossessedExploderBehavior | Returned |
| NPC_WardenHusk | Warden v1.5 | Line 7874 | WardenHusk | AC_WardenHusk | AC_WardenHuskBehavior | Returned |
| NPC_WardenCore | Warden v1.5 | Line 7884 | WardenCore | AC_WardenCore | AC_WardenCoreBehavior | Returned |
| NPC_BiomechHost | Biomech v1.3 | Line 7897 | BiomechHost | AC_BiomechHost | AC_BiomechHostBehavior | Returned |
| NPC_BiomechCreature | Biomech v1.3 | Line 7909 | BiomechCreature | AC_BiomechCreature | AC_BiomechCreatureBehavior | Returned |
| NPC_FatherCompanion | (Father) | Line 7774 | - | AC_FatherCompanion | AC_FatherBehavior | FatherCompanion, Player |

**Status:** ALL NPCDefinitions present and correctly configured.

---

## AbilityConfiguration AUDIT

### Comparison Matrix

| Config | Guide Reference | Manifest Line | startup_effects | abilities |
|--------|-----------------|---------------|-----------------|-----------|
| AC_GathererScout | Gatherer v1.3 | 7552 | GE_GathererScoutAttributes (v1.1 fix) | [] |
| AC_Reinforcement | Gatherer v1.3 | 7557 | GE_ReinforcementAttributes | GA_Death |
| AC_ReturnedStalker | Stalker v2.4 | 7571 | GE_ReturnedStalkerAttributes | GA_Death |
| AC_SupportBuffer | Buffer v1.4 | 7581 | GE_SupporterAttributes | GA_Death |
| AC_FormationGuard | Guard v2.7 | 7592 | GE_FormationGuardAttributes | GA_Death |
| AC_PossessedExploder | Exploder v2.3 | 7602 | GE_ExploderAttributes | GA_Death |
| AC_WardenHusk | Warden v1.5 | 7612 | GE_WardenHuskAttributes | GA_Death |
| AC_WardenCore | Warden v1.5 | 7620 | GE_WardenCoreAttributes | GA_CoreLaser, GA_Death |
| AC_BiomechHost | Biomech v1.3 | 7630 | GE_BiomechHostAttributes | GA_Death |
| AC_BiomechCreature | Biomech v1.3 | 7637 | GE_BiomechCreatureAttributes | GA_Death |
| AC_FatherCompanion | Father | 7526 | GE_CrawlerState | 12 abilities |

**Status:** ALL AbilityConfigurations match guide specifications.

---

## ActivityConfiguration AUDIT

### Comparison Matrix

| Config | Guide Reference | Manifest Line | Activities | GoalGenerators |
|--------|-----------------|---------------|------------|----------------|
| AC_GathererScoutBehavior | Gatherer v1.3 | 7667 | BPA_Alert, BPA_Flee, BPA_Interact, BPA_Patrol | GoalGenerator_Alert |
| AC_ReturnedStalkerBehavior | Stalker v2.4 | 7686 | BPA_Patrol, BPA_FollowCharacter, BPA_Attack_Melee | GoalGenerator_RandomAggression, GoalGenerator_Attack |
| AC_SupportBufferBehavior | Buffer v1.4 | 7697 | BPA_SupportFollow, BPA_Idle | None (per guide) |
| AC_FormationGuardBehavior | Guard v2.7 | 7707 | BPA_FormationFollow, BPA_Attack_Formation, BPA_Idle | GoalGenerator_Attack |
| AC_PossessedExploderBehavior | Exploder v2.3 | 7720 | BPA_Explode, BPA_Idle | GoalGenerator_Attack |
| AC_WardenHuskBehavior | Warden v1.5 | 7731 | BPA_Attack_Melee, BPA_Patrol, BPA_Idle | GoalGenerator_Attack |
| AC_WardenCoreBehavior | Warden v1.5 | 7742 | BPA_Attack_Ranged, GoalGenerator_Attack | GoalGenerator_Attack |
| AC_BiomechHostBehavior | Biomech v1.3 | 7752 | BPA_Patrol, BPA_Attack_Melee, BPA_Idle | GoalGenerator_Attack |
| AC_BiomechCreatureBehavior | Biomech v1.3 | 7761 | BPA_Attack_Melee, BPA_Idle | GoalGenerator_Attack |
| AC_FatherBehavior | Father | 7647 | 6 activities | GoalGenerator_Attack |

**Status:** ALL ActivityConfigurations match guide specifications.

---

## GameplayEffect ATTRIBUTE AUDIT

### Attribute Value Comparison

| GE Asset | Guide | MaxHealth | Health | AttackDamage | Armor | Status |
|----------|-------|-----------|--------|--------------|-------|--------|
| GE_GathererScoutAttributes | Gatherer v1.3 | 75 | 75 | 0 | 0 | ADDED (v1.1) |
| GE_WardenHuskAttributes | Warden v1.5 | 500 | 500 | 35 | 25 | MATCH |
| GE_WardenCoreAttributes | Warden v1.5 | 75 | 75 | 50 | 0 | MATCH |
| GE_BiomechHostAttributes | Biomech v1.3 | 250 | 250 | 28 | 12 | MATCH |
| GE_BiomechCreatureAttributes | Biomech v1.3 | 85 | 85 | 38 | 0 | MATCH |
| GE_ReinforcementAttributes | Gatherer v1.3 | 120 | 120 | 25 | 5 | MATCH |
| GE_ReturnedStalkerAttributes | Stalker v2.4 | 100 | 100 | 30 | 0 | MATCH |
| GE_FormationGuardAttributes | Guard v2.7 | 150 | 150 | 20 | 15 | MATCH |
| GE_ExploderAttributes | Exploder v2.3 | 50 | 50 | - | 0 | MATCH |
| GE_SupporterAttributes | Buffer v1.4 | 150 | 150 | 0 | 20 | MATCH |

**Status:** ALL attribute values match guide specifications exactly (10 NPCs verified).

---

## BehaviorTree AUDIT

### Comparison Matrix

| BT Asset | Guide | Manifest Line | Blackboard | Services | Tasks |
|----------|-------|---------------|------------|----------|-------|
| BT_FormationFollow | Guard v2.7 | 9856 | BB_FormationFollow | BTS_CalculateFormationPosition, BTS_AdjustFormationSpeed | BTTask_MoveTo |
| BT_Explode | Exploder v2.3 | 11237 | BB_Attack | BTS_CheckExplosionProximity | BTTask_MoveTo |
| BT_SupportFollow | Buffer v1.4 | 12117 | BB_FollowCharacter | BTS_HealNearbyAllies, BTS_SetAIFocus, BTS_AdjustFollowSpeed | BTTask_MoveTo |
| BT_FatherFollow | Father | 7433 | BB_FatherCompanion | - | BTTask_MoveTo |
| BT_FatherEngineer | Father | 7476 | BB_FatherEngineer | - | Wait |

**Status:** ALL BehaviorTrees present with correct configurations.

---

## Activity AUDIT

### Comparison Matrix

| Activity | Guide | Manifest Line | Parent Class | BehaviorTree | SupportedGoalType |
|----------|-------|---------------|--------------|--------------|-------------------|
| BPA_FormationFollow | Guard v2.7 | 9891 | NPCActivity | BT_FormationFollow | Goal_FormationFollow |
| BPA_Explode | Exploder v2.3 | 11273 | NPCActivity | BT_Explode | Goal_Attack |
| BPA_SupportFollow | Buffer v1.4 | 12151 | BPA_FollowCharacter | BT_SupportFollow | Goal_FollowCharacter |
| BPA_Alert | Gatherer v1.3 | 13168 | NPCActivity | BT_MoveToDestination | Goal_Alert |

**Status:** ALL custom Activities present with correct configurations.

---

## Goal/GoalGenerator AUDIT

### Goals

| Goal | Guide | Manifest Line | Variables | Functions |
|------|-------|---------------|-----------|-----------|
| Goal_Alert | Gatherer v1.3 | 12777 | AlertLocation, SpottedTarget | Full event graph |
| Goal_FormationFollow | Guard v2.7 | 13150 | FormationOffset, FormationIndex | Inherited |

### GoalGenerators

| Generator | Guide | Manifest Line | Key Features |
|-----------|-------|---------------|--------------|
| GoalGenerator_Alert | Gatherer v1.3 | 12813 | OnPerceptionUpdated, Creates Goal_Alert |
| GoalGenerator_RandomAggression | Stalker v2.4 | 13929 | Bond system, 7 tagged dialogue triggers, follow/attack/defend states |

**Status:** ALL Goals and GoalGenerators present with full implementations.

---

## COMPLIANCE MATRIX

| Requirement | Gatherer | Stalker | Buffer | Guard | Exploder | Warden | Biomech |
|-------------|----------|---------|--------|-------|----------|--------|---------|
| NPCDefinition(s) | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| AbilityConfiguration | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| ActivityConfiguration | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| Attributes GE | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| BehaviorTree | N/A | N/A | PASS | PASS | PASS | N/A | N/A |
| Custom Activity | PASS | N/A | PASS | PASS | PASS | N/A | N/A |
| Custom Goal | PASS | N/A | N/A | PASS | N/A | N/A | N/A |
| GoalGenerator | PASS | PASS | N/A | N/A | N/A | N/A | N/A |
| Faction Consistency | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| Attribute Values | PASS | PASS | PASS | PASS | PASS | PASS | PASS |

**Legend:**
- PASS: Manifest entry matches guide specification
- N/A: Guide does not require this asset type (uses existing Narrative Pro assets)

---

## DETAILED FINDINGS

### MAUD-01: AC_GathererScout Missing Attributes GE (FIXED)

**Location:** manifest.yaml:7552

**Original Issue:**
```yaml
- name: AC_GathererScout
  folder: Configs
  abilities: []  # Non-combatant
  # MISSING: startup_effects with attributes GE
```

**Problem:** Even non-combatant NPCs need Health/MaxHealth attributes so they can be damaged and killed (for loot drops). Without attributes, the Scout would be unkillable.

**Fix Applied (v7.8.32):**
1. Added `GE_GathererScoutAttributes` with values: 75/75/0/0 (fragile, non-combatant profile)
2. Updated `AC_GathererScout` with `startup_effects: [GE_GathererScoutAttributes]`
3. Updated guide to include Scout attributes documentation

**Status:** **FIXED** - Scout now has proper Health attributes.

---

### MAUD-02: All Attribute Values Match Guides (PASS)

**Verification:** All 9 NPC attribute GEs were manually verified against their respective implementation guides:

| NPC Profile | Stat Profile |
|-------------|--------------|
| Warden Husk | Slow, armored, high health melee (500/500/35/25) |
| Warden Core | Fragile, high damage flying (75/75/50/0) |
| Biomech Host | Slow, armored phase 1 (250/250/28/12) |
| Biomech Creature | Fast, aggressive phase 2 (85/85/38/0) |
| Reinforcement | Moderate combat (120/120/25/5) |
| Returned Stalker | Glass cannon (100/100/30/0) |
| Formation Guard | Balanced defensive (150/150/20/15) |
| Possessed Exploder | Suicide bomber, low health (50/50/-/0) |
| Support Buffer | Healer, no attack (150/150/0/20) |

**Status:** VERIFIED - All values match guide specifications exactly.

---

### MAUD-03: Faction Consistency (PASS)

**Observation:** All NPC systems consistently use `Narrative.Factions.Returned` as their faction.

**Exception:** NPC_FatherCompanion uses `Narrative.Factions.FatherCompanion` and `Narrative.Factions.Player` as the companion is player-allied.

**Status:** VERIFIED - Faction assignments are correct and consistent.

---

## RECOMMENDATIONS

### Maintenance

1. **Version tracking:** Continue including version comments (e.g., `v7.8.23: GF-2 fix`) for manifest changes
2. **Guide alignment:** Keep manifest organized by system (GUARD FORMATION, POSSESSED EXPLODER, etc.)
3. **Cross-reference:** Manifest section comments reference guide phases and sections

### Future Audits

1. Run manifest audit after each guide version update
2. Verify attribute values when balance changes occur
3. Check ActivityConfiguration activity lists when new activities added

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | January 2026 | Initial comprehensive audit - all systems verified |
| 1.1 | January 2026 | MAUD-01 fix: Added GE_GathererScoutAttributes (75/75/0/0) and updated AC_GathererScout |

---

**END OF AUDIT v1.1**
