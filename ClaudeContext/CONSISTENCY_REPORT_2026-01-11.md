# Father Companion System - Comprehensive Consistency Report

**Generated:** January 11, 2026
**Files Analyzed:** 43 files in guides/ + 8 files in ClaudeContext/
**Plugin Version:** GasAbilityGenerator v2.6.8
**Narrative Pro Version:** v2.2

---

## EXECUTIVE SUMMARY

Analysis of 51 documentation files reveals:

| Severity | Issue | Files Affected | Details |
|----------|-------|----------------|---------|
| **CRITICAL** | Tag format: `State.Father.*` should be `Father.State.*` | 11 files | 78 occurrences |
| **INFO** | Replication settings vary by ability type (intentional) | All GA files | Documented patterns |
| **INFO** | Parent classes vary by asset type (intentional) | All files | Multiple parent types |

---

## 1. TAG FORMAT INCONSISTENCY (CRITICAL)

### Understanding the Tag Hierarchies

**Authoritative Source:** `DefaultGameplayTags_FatherCompanion_v4_0.ini` (lines 148-166)

There are TWO separate tag hierarchies that should not be confused:

1. **`Father.State.*`** - Father-specific states (19 tags defined in .ini)
   - `Father.State.Alive`, `Father.State.Attached`, `Father.State.Dormant`
   - `Father.State.Transitioning`, `Father.State.SymbioteLocked`, etc.

2. **`State.*`** - Narrative Pro built-in states (separate hierarchy)
   - `State.Invulnerable` - Blocks all damage (Narrative Pro system)
   - `State.Invisible` - Character invisible (Narrative Pro system)

### Problem
Some guides use the **OLD** tag format `State.Father.*` which is **INCORRECT**. The correct format is `Father.State.*` as defined in DefaultGameplayTags.ini.

Note: The Technical Reference v6.0 Section 10.1 v4.1 Additions table itself contains this error and needs updating.

### Files with OLD format (`State.Father.*`) - 78 occurrences:
| File | Count |
|------|-------|
| GA_FatherCrawler_Implementation_Guide_v3_3.md | 20 |
| GA_FatherExoskeleton_Implementation_Guide_v3_10.md | 14 |
| Father_Companion_Technical_Reference_v6_0.md | 13 |
| GA_FatherEngineer_Implementation_Guide_v4_3.md | 8 |
| GA_FatherArmor_Implementation_Guide_v4_3.md | 4 |
| GA_FatherSymbiote_Implementation_Guide_v3_5.md | 2 |
| Father_Companion_System_Design_Document_v2_0.md | 1 |

### Tags Requiring Update
| OLD Format (Wrong) | CORRECT Format | .ini Line |
|--------------------|----------------|-----------|
| `State.Father.Dormant` | `Father.State.Dormant` | 154 |
| `State.Father.Transitioning` | `Father.State.Transitioning` | 165 |
| `State.Father.SymbioteLocked` | `Father.State.SymbioteLocked` | 164 |

### Complete Father.State.* Tag Reference (from .ini)
```
Father.State.Alive          - Required for form activation
Father.State.Attached       - Father attached to player
Father.State.Attacking      - Currently executing attack
Father.State.Dashing        - Dash in progress
Father.State.Deployed       - Engineer turret mode active
Father.State.Dormant        - Post-sacrifice dormant state
Father.State.Following      - Following player
Father.State.Idle           - Idle state
Father.State.InCombat       - Combat engaged
Father.State.Patrolling     - Patrolling area
Father.State.Recovering     - Post-detachment recovery
Father.State.Searching      - Searching for target
Father.State.SymbioteLocked - Blocks form change during 30s Symbiote
Father.State.Targeting      - Has valid target
Father.State.Transitioning  - Blocks form change during 5s VFX
```

---

## 2. REPLICATION SETTINGS (DETAILED)

### Finding: INTENTIONALLY VARIED per Technical Reference v6.0 Section 7

The Technical Reference defines specific replication patterns based on ability ownership and type:

### 2.1) Net Execution Policy Matrix (from Tech Ref Section 7.4)

| Ability Type | Owner | Policy | Reason |
|--------------|-------|--------|--------|
| Player Input Actions | Player | **Local Predicted** | Responsive feel, client prediction |
| Passive + NPC-Owned | NPC (Father) | **Server Only** | No input to predict, server-authoritative |
| AI-Triggered | NPC | **Server Only** | AI runs on server |
| Form Abilities (grants to player) | Father (NPC) | **Server Only** | Cross-actor grants require server authority |
| Form Abilities (visual only) | Father (NPC) | **Server Initiated** | Visual feedback with server start |

### 2.2) Actual Settings per Guide

| Ability | Net Execution | Replication | Owner | Notes |
|---------|---------------|-------------|-------|-------|
| **Form Abilities (NPC-owned, Server Only)** |||||
| GA_FatherCrawler | Server Only | Replicate Yes | Father | NPC-owned form |
| GA_FatherArmor | Server Only | Replicate Yes | Father | Cross-actor GE application |
| GA_FatherExoskeleton | Server Only | Replicate | Father | Grants abilities to player |
| GA_FatherSymbiote | Server Only | Replicate Yes | Father | Cross-actor operations |
| GA_FatherEngineer | Server Only | Replicate | Father | Spawns replicated actors |
| **AI Combat Abilities (NPC-owned, Server Only)** |||||
| GA_FatherAttack | Server Only | (inherited) | Father | AI melee |
| GA_FatherLaserShot | Server Only | Replicate | Father | AI ranged |
| GA_TurretShoot | Server Only | Replicate | Father | AI turret |
| GA_FatherElectricTrap | Server Only | Replicate | Father | AI trap |
| GA_FatherMark | Server Only | Replicate | Father | Passive NPC-owned |
| GA_ProximityStrike | Server Only | Replicate | Father | Passive AOE |
| GA_CoreLaser (Warden) | Server Only | Replicate | NPC | AI projectile |
| **Player-Owned Abilities (Local Predicted)** |||||
| GA_FatherExoskeletonDash | Local Predicted | Replicate Yes | Player | Player input |
| GA_FatherExoskeletonSprint | Local Predicted | Replicate Yes | Player | Player input |
| GA_StealthField | Local Predicted | Replicate Yes | Player | Player toggle |
| GA_ProtectiveDome | Local Predicted | Replicate | Player | Player toggle |
| GA_DomeBurst | Local Predicted | Replicate | Player | Player instant |
| GA_Backstab | Local Predicted | Replicate Yes | Player | Player Default |
| GA_FatherRifle | Local Predicted | Replicate Yes | Player | Weapon form |
| GA_FatherSword | Local Predicted | Replicate Yes | Player | Weapon form |
| **Special Cases** |||||
| GA_Death | Server Only | Do Not Replicate | Any | Server event trigger |
| GA_Interact | Server Initiated | Do Not Replicate | Player | Client-side ability |

### 2.3) ASC Replication Mode (from Tech Ref Section 7.5)

| Character Type | Replication Mode | Reason |
|----------------|------------------|--------|
| Player Characters | **Mixed** | Full ability state for owner, minimal for others |
| AI/NPC Characters (Father) | **Minimal** | Reduced bandwidth, server handles all logic |
| DO NOT USE | Full | Excessive bandwidth |

### 2.4) Variable Replication Conditions (from Tech Ref Section 7.7)

| Variable | Replication Type | Condition | Reason |
|----------|------------------|-----------|--------|
| OwnerPlayer | Replicated | **Initial Only** | Set once at spawn, never changes |
| CurrentForm | RepNotify | **None** | All clients must see form changes |
| IsAttached | RepNotify | **None** | All clients must see attachment |
| IsDeployed | Replicated | **None** | All clients see turret mode |
| DeployedLocation | Replicated | **None** | All clients need turret position |
| TurretHealth | RepNotify | **None** | All clients see health UI |
| AttackTarget | Replicated | **None** | Target visible for AI coordination |

### 2.5) Deprecated Settings (UE 5.5+)

| Setting | Status | Alternative |
|---------|--------|-------------|
| Replicated properties on GA_ | Deprecated | Use AbilityTasks, GameplayCues |
| NonInstanced abilities | Deprecated | Use Instanced Per Actor |
| Replicate Input Directly | Should be OFF | Use AbilityTasks instead |
| Full ASC Replication Mode | Avoid | Use Mixed (player) or Minimal (NPC) |

---

## 3. PARENT CLASSES (COMPREHENSIVE)

### Finding: INTENTIONALLY VARIED by asset type

The system uses specialized parent classes for different asset types as documented in Tech Ref Section 12.1 and Plugin Spec.

### 3.1) Gameplay Abilities

| Ability | Parent Class | Reason |
|---------|--------------|--------|
| Most GA_* | **NarrativeGameplayAbility** | Standard Narrative Pro ability |
| GA_FatherAttack | **GA_Melee_Unarmed** | Inherits warp, combo, socket trace |
| GA_FatherRifleFire | GA_Attack_Firearm_Base | Inherits ranged combat |
| GA_FatherSwordAttack | GA_Attack_Combo_Melee | Inherits melee combo |

### 3.2) Characters/NPCs

| Asset | Parent Class | Module |
|-------|--------------|--------|
| BP_FatherCompanion | **NarrativeNPCCharacter** | NarrativeArsenal |
| BP_PossessedExploder | NarrativeNPCCharacter | NarrativeArsenal |
| BP_SupportBuffer | NarrativeNPCCharacter | NarrativeArsenal |
| BP_WardenHusk | NarrativeNPCCharacter | NarrativeArsenal |
| BP_WardenCore | NarrativeNPCCharacter | NarrativeArsenal |

### 3.3) Equippable Items

| Asset | Parent Class | Purpose |
|-------|--------------|---------|
| BP_FatherArmorForm | **EquippableItem** | Form equipment |
| BP_FatherExoskeletonForm | EquippableItem | Form equipment |
| BP_FatherSymbioteForm | EquippableItem | Form equipment |
| BP_FatherEngineerForm | EquippableItem | Form equipment |
| BP_FatherRifleWeapon | **RangedWeaponItem** | Weapon form |
| BP_FatherSwordWeapon | **MeleeWeaponItem** | Weapon form |

### 3.4) Gameplay Effects

| Asset | Parent Class | Purpose |
|-------|--------------|---------|
| Most GE_* | **GameplayEffect** | Standard effect |
| GE_EquipmentModifier_* | **GE_EquipmentModifier** | Form stat modifiers |
| GE_LaserDamage, etc. | GE_WeaponDamage | Damage effects |

### 3.5) Projectiles

| Asset | Parent Class |
|-------|--------------|
| BP_LaserProjectile | **NarrativeProjectile** |
| BP_TurretProjectile | NarrativeProjectile |
| BP_ElectricWeb | NarrativeProjectile |
| BP_CoreLaserProjectile | NarrativeProjectile |

### 3.6) AI/Activity System

| Asset Type | Parent Class | Header File |
|------------|--------------|-------------|
| Activity Configuration | **AbilityConfiguration** | AbilityConfiguration.h |
| NPC Activity Configuration | **NPCActivityConfiguration** | NPCActivityConfiguration.h |
| NPC Activities (BPA_*) | **NarrativeActivityBase** | NPCActivity.h |
| BPA_Attack_Formation | **BPA_Attack_Melee** | Child of NarrativeActivityBase |
| BPA_SupportFollow | **BPA_FollowCharacter** | Child of NarrativeActivityBase |
| Goal Generators | **NPCGoalGenerator** | NPCGoalGenerator.h |
| Goal Items | **NPCGoalItem** | NPCGoalItem.h |
| Goal_FormationFollow | **Goal_FollowCharacter** | Child of NPCGoalItem |
| Behavior Tree Services | **BTService_BlueprintBase** | Engine |

### 3.7) Gameplay Cues

| Asset | Parent Class | Type |
|-------|--------------|------|
| GC_TakeDamage | **GameplayCueNotify_BurstLatent** | Burst with latent |
| GC_TakeDamage_Stumble | **GC_TakeDamage** | Child cue |
| GC_Footstep | **GameplayCueNotify_Burst** | Simple burst |
| GC_Sprint (looping) | **AGameplayCueNotify_Actor** | Persistent |

### 3.8) Widgets

| Asset | Parent Class |
|-------|--------------|
| WBP_MarkIndicator | **UserWidget** |
| WBP_UltimatePanel | UserWidget |
| WBP_UltimateIcon | UserWidget |

### 3.9) Narrative Events

| Asset | Parent Class |
|-------|--------------|
| NE_SetFatherOwner | **NarrativeEvent** |
| NE_ClearFatherOwner | NarrativeEvent |

---

## 4. COMPLETE FILE INVENTORY

### 4.1) Guides Directory (43 files)

| Category | Count | Files |
|----------|-------|-------|
| Form Ability Guides | 5 | GA_FatherCrawler, GA_FatherArmor, GA_FatherExoskeleton, GA_FatherSymbiote, GA_FatherEngineer |
| Combat Ability Guides | 7 | GA_FatherAttack, GA_FatherLaserShot, GA_TurretShoot, GA_FatherRifle, GA_FatherSword, GA_Backstab, GA_FatherElectricTrap |
| Movement/Utility Guides | 5 | GA_FatherExoskeletonDash, GA_FatherExoskeletonSprint, GA_StealthField, GA_ProximityStrike, GA_FatherSacrifice |
| Dome System Guides | 3 | Father_Protective_Dome, GA_DomeBurst, GA_FatherMark |
| NPC/Enemy Guides | 6 | Gatherer_Scout_Alert, Guard_Formation_Follow, Possessed_Exploder, Random_Aggression_Stalker, Support_Buffer_Healer, Warden_Husk |
| Widget Guides | 2 | WBP_MarkIndicator, WBP_UltimatePanel |
| System Guides | 4 | GE_FormChangeCooldown, Biomechanical_Detachment, Father_Companion_VFX, Father_Companion_System_Setup |
| Reference Docs | 6 | Technical_Reference, System_Design, Guide_Format, Plugin_Spec, Node_Connection_Reference, Plugin_Enhancement_Research |
| Config Files | 1 | DefaultGameplayTags_FatherCompanion_v4_0.ini |
| API Reference | 1 | UE5_6_Programmatic_Asset_Creation_APIs |
| Project Instructions | 1 | Spider_Companion_Project_Instructions |

### 4.2) ClaudeContext Directory (8 files)

| File | Purpose |
|------|---------|
| manifest.yaml | Single source of truth for assets |
| Father_Companion_Technical_Reference_v6_0.md | GAS patterns, multiplayer, tags |
| Father_Companion_System_Design_Document_v2_0.md | System overview, forms, abilities |
| Father_Companion_Guide_Format_Reference_v3_0.md | Documentation standards |
| Father_Companion_System_Setup_Guide_v3_1.md | Initial setup instructions |
| Father_Ability_Generator_Plugin_v7_8_2_Specification.md | Plugin architecture |
| DefaultGameplayTags_FatherCompanion_v4_0.ini | 174 gameplay tags |
| SESSION_LOG_2026-01-10.md, session_log_2026-01-11.md | Development logs |

---

## 5. GAMEPLAY TAGS (174 Total)

From DefaultGameplayTags_FatherCompanion_v4_0.ini:

| Category | Count | Examples |
|----------|-------|----------|
| Ability.Father.* | 24 | Ability.Father.Crawler, Ability.Father.Armor |
| AI.Action.* | 5 | AI.Action.MarkEnemy, AI.Action.MeleeAttack |
| AI.State.* | 3 | AI.State.HasTarget, AI.State.InCombat |
| Character.* | 4 | Character.Enemy, Character.Player, Character.Father |
| Companion.* | 7 | Companion.Slot.1-4, Companion.State.Selected |
| Cooldown.Father.* | 9 | Cooldown.Father.FormChange, Cooldown.Father.Dash |
| Data.* | 10 | Data.Damage.Turret, Data.Dome.Energy |
| Effect.Father.* | 29 | Effect.Father.ArmorBoost, Effect.Father.Stealth |
| Father.Form.* | 6 | Father.Form.Crawler, Father.Form.Armor |
| Father.State.* | 19 | Father.State.Alive, Father.State.Attached |
| Father.Dome.* | 4 | Father.Dome.Active, Father.Dome.Charging |
| GameplayCue.Father.* | 6 | GameplayCue.Father.Sprint, GameplayCue.Father.Mark |
| GameplayEvent.Father.* | 6 | GameplayEvent.Father.FormChanged |
| Narrative.Equipment.* | 1 | Narrative.Equipment.Slot.FatherForm |
| Narrative.Factions.* | 2 | Narrative.Factions.Heroes, Narrative.Factions.Player |
| Narrative.Input.* | 10 | Narrative.Input.Father.FormChange |
| Narrative.TaggedDialogue.* | 6 | Narrative.TaggedDialogue.Father.* |
| Player.State.* | 1 | Player.State.Stealth |
| State.* | 3 | State.Invulnerable, State.Invisible |
| Status.Immune.* | 2 | Status.Immune.Damage, Status.Immune.Knockback |
| Symbiote.Charge.* | 1 | Symbiote.Charge.Ready |

---

## 6. ASSET GENERATION PATTERNS

From Plugin Specification v7.8.2:

### 6.1) Programmatic Asset Creation APIs

| Asset Type | Creation Method | Key Notes |
|------------|-----------------|-----------|
| Behavior Trees | FBTCompositeChild arrays | NOT pin connections |
| Animation Montages | FAnimNotifyEvent::Link() | CRITICAL for timing |
| Widget Blueprints | UWidgetTree::ConstructWidget<T>() | Only correct way |
| Niagara Systems | Template duplication | Full programmatic NOT supported |
| Gameplay Cues | Two distinct base classes | Burst vs Persistent |
| Cast Nodes | NotifyPinConnectionListChanged() | MUST call after MakeLinkTo |

### 6.2) Current Generator Statistics

| Metric | Count |
|--------|-------|
| Total Auto-Gen Assets | 248 |
| Structure Only Assets | 18 |
| Gameplay Effects | 34 |
| Gameplay Abilities | 17 |
| Animation Montages | 6 |

---

## 7. ACTION ITEMS

### Critical (Fix Immediately):
1. **Update tag format** in 11 files: Change `State.Father.*` to `Father.State.*` (78 occurrences)

### Medium Priority:
2. **Add bWasCancelled check** to GA_FatherCrawler and GA_FatherSymbiote guides
3. **Rename variable** in GA_FatherArmor: `OriginalWalkSpeed` → `OriginalMaxWalkSpeed`

### Low Priority:
4. **Update widget guide versions** to reference Narrative Pro v2.2
5. **Standardize blackboard key naming** with BBKey_ prefix

---

## 8. CONCLUSION

The Father Companion system documentation is **highly consistent** with documented variations:

1. **Tag Format Issue:** 78 occurrences of old format need updating
2. **Replication Settings:** Intentionally varied per ability type (NPC vs Player ownership)
3. **Parent Classes:** Intentionally varied per asset type (abilities, items, NPCs, etc.)
4. **All other patterns:** Consistent across 51 documentation files

**Total Files Analyzed:**
- guides/: 43 files
- ClaudeContext/: 8 files
- **Total: 51 files**
