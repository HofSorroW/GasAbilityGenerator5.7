# Father Companion Technical Reference
## Narrative Pro v2.2 | Unreal Engine 5.7 | Consolidated Reference

---

> **⚠️ GAS AUDIT v6.3 COMPLIANCE (January 2026)**
>
> Per GAS Audit v6.3 (Claude-GPT dual audit 2026-01-24), the following rules apply:
>
> **LOCKED Rules (Must Follow):**
>
> | Rule | Status | Summary |
> |------|--------|---------|
> | **INV-1** | LOCKED | NO invulnerability except GA_FatherSacrifice |
> | **R-TIMER-1** | LOCKED | SetTimer callbacks MUST guard against invalid state |
> | **R-ENUM-1** | LOCKED | Enum is derived view; GE is truth source |
> | **R-AI-1** | LOCKED | NPCs with Activities must coordinate BT calls |
> | **R-CLEANUP-1** | LOCKED | Runtime-granted abilities need removal strategy |
> | **INV-GESPEC-1** | LOCKED | MakeOutgoingGameplayEffectSpec MUST use `param.GameplayEffectClass:` |
>
> **DOC-ONLY Patterns (Best Practices):**
>
> | Pattern | Summary |
> |---------|---------|
> | **P-MOVE-1** | Store/Restore MaxWalkSpeed via CharacterMovement |
> | **P-ATTACH-1** | ServerOnly execution handles attachment authority |
> | **P-EFFECT-1** | Tag-based OR handle-based GE removal both valid |
> | **P-MONTAGE-1** | OnCompleted/Interrupted/Cancelled → EndAbility; OnBlendOut ≠ EndAbility |
> | **P-TARGET-1** | Use Narrative Pro's GenerateTargetDataUsingTrace or simple LineTrace |
>
> **INV-1 Details:** ALL invulnerability has been REMOVED from the Father companion system EXCEPT GA_FatherSacrifice (8-second player invulnerability).
>
> **R-AI-1 Details:** GA_FatherEngineer calls StopCurrentActivity() before RunBehaviorTree() per LOCKED CONTRACT 12.
>
> **INV-GESPEC-1 Details:** All MakeOutgoingGameplayEffectSpec nodes MUST use `param.GameplayEffectClass: GE_*` syntax. The `gameplay_effect_class:` property is NOT processed by the generator. See LOCKED CONTRACT 13 and VTF-9.
>
> See `ClaudeContext/Handoffs/Father_Companion_GAS_Abilities_Audit.md` and `ClaudeContext/Handoffs/LOCKED_CONTRACTS.md` for full details.

---

## DOCUMENT INFORMATION

| Field | Value |
|-------|-------|
| Document Type | Technical Reference |
| Engine Version | Unreal Engine 5.7 |
| Plugin Version | Narrative Pro v2.2 BETA |
| Last Updated | January 2026 |
| Version | 6.7 |
| Last Audit | 2026-01-24 (GAS Audit v6.3 - INV-GESPEC-1) |
| Purpose | Combined reference for C++ locations, Blueprint patterns, system architecture, Narrative Pro NPC systems, NarrativeEvent system, cross-actor ability granting, ability validation, death handling, EndPlay safety, multiplayer authority patterns, NPC Schedule system, Interaction Slot system, Time of Day triggers, Goal/Activity Follow System architecture, v2.2 new systems (Projectile, Melee Multi-Hit, Cover, Fragments, Dual Wield/Offhand), UE 5.6 GE component reference, built-in cooldown system, faction attack chain, HandleDeath parameters, Hostiles array patterns, complete content folder structure, BT task system, BT services (complete documentation), GE_EquipmentModifier pattern, EquippableItem lifecycle, child GE architecture, reference asset analysis, father-to-Narrative alignment |
| Replaces | Father_Companion_Technical_Reference_v6_6.md |

---

## VERSION 6.7 CHANGES

| Change | Details |
|--------|---------|
| **INV-GESPEC-1 Added** | New LOCKED rule: MakeOutgoingGameplayEffectSpec nodes MUST use `param.GameplayEffectClass:` syntax. 16 abilities fixed. |
| **GAS Audit v6.3** | Added VTF-9 finding for silent runtime failure with incorrect manifest syntax. |
| **LOCKED CONTRACT 13** | Added Contract 13 documenting generator param.* prefix requirement for TSubclassOf pins. |

## VERSION 6.6 CHANGES

| Change | Details |
|--------|---------|
| **GAS Audit v6.2 Compliance** | Updated compliance box to reflect v6.2 audit closure. Added DOC-ONLY patterns table (P-MOVE-1, P-ATTACH-1, P-EFFECT-1, P-MONTAGE-1, P-TARGET-1). |
| **Audit Closure** | Claude-GPT dual audit concluded: 5 LOCKED rules, 5 DOC-ONLY patterns. No new LOCKED rules required. All patterns validated. |
| **Research Assessment Archived** | `GAS_Patterns_Research_Assessment_v1.md` consolidated into GAS Audit and archived. |

## VERSION 6.5 CHANGES

| Change | Details |
|--------|---------|
| **Maintenance update** | Minor formatting and reference updates. |

## VERSION 6.4 CHANGES

| Change | Details |
|--------|---------|
| **v5.1 Goal_Attack Backstab Detection** | Section 5.3 ViewedCharacter marked as DEPRECATED/LEGACY. Section 5.5 completely rewritten with Goal_Attack Query approach. Uses Narrative Pro's built-in GoalGenerator_Attack → Goal_Attack → TargetToAttack system. No custom perception binding required. BP_BackstabNPCController no longer needed. |
| **Decoy Tactical Flow Updated** | Section 5.5 includes new decoy flow table showing Father distraction → player backstab via Goal_Attack query. |

## VERSION 6.3 CHANGES

| Change | Details |
|--------|---------|
| **INC-4 Fix (Claude-GPT Audit)** | Section 19.7 transition flow updated: "Delay node" → "AbilityTaskWaitDelay per Track B v4.15". Manifest uses AbilityTaskWaitDelay for all form ability delays. |
| **Authority Note** | This document is explanatory. Runtime behavior and generation are defined by `manifest.yaml`. In case of conflict, the manifest takes precedence. |

## VERSION 6.2 CHANGES

| Change | Details |
|--------|---------|
| **Locked EndAbility Rules** | Section 11.9 updated with Rules 1-4 from January 2026 Claude-GPT dual-agent audit |
| **VTF-7 Reference** | Added CommitCooldown requirement from audit findings |
| **Audit Document Reference** | Points to `ClaudeContext/Handoffs/Father_Companion_GAS_Abilities_Audit.md` |

## VERSION 6.1 CHANGES

| Change | Details |
|--------|---------|
| **Option B Form State Architecture** | Form identity now uses GE-based persistent state (GE_*State) instead of ActivationOwnedTags |
| **Form State Law Section Added** | New canonical section defining form state taxonomy, invariants, and transition sequence |
| **Errata Marked** | §11.9, §19.*, §58.7-8 marked as ERRATA - ActivationOwnedTags pattern replaced |
| **UE Version Updated** | 5.6 → 5.7 |
| **Audit Reference** | See Form_State_Architecture_Audit_v1_0.md for full rationale |

---

## FORM STATE LAW (Option B Architecture)

**Reference:** `ClaudeContext/Handoffs/Form_State_Architecture_Audit_v1_0.md`

### Taxonomy

| Category | Tags | Purpose |
|----------|------|---------|
| **Eligibility Gates** | Father.State.Alive, Father.State.Recruited, Father.State.Dormant | World state checks for activation |
| **Form Identity** | Effect.Father.FormState.* | System-owned form ownership |
| **During-Ability State** | Father.Form.* (ActivationOwnedTags) | Active while ability executes |

### Locked Invariants

| Invariant | Rule |
|-----------|------|
| **Single Active Form State** | Exactly one Effect.Father.FormState.* must exist at runtime |
| **GE Split Rule** | GE_*State = identity only (**NO invulnerability** per INV-1); GE_*Boost = stats only |
| **bWasCancelled Scope** | EndAbility cleanup (speed, detach) only when bWasCancelled=true |
| **Net Execution Policy** | Form abilities are ServerOnly (NPC-owned, cross-actor operations) |

### Transition Sequence (Option B)

```
NEW form ActivateAbility:
  1. Remove ALL prior form state GEs → BP_RemoveGameplayEffectFromOwnerWithGrantedTags(Effect.Father.FormState.*)
  2. Apply GE_[NewForm]State → Grants Effect.Father.FormState.[Form] only (NO invulnerability per INV-1)
  3. Apply GE_[NewForm]Boost → Stats only (if applicable)
  4. Commit cooldown
  5. EndAbility (bWasCancelled=false) → Form is now active, state persists via GE

OLD form EndAbility (bWasCancelled=true):
  - Restore movement speed
  - Detach from player
  - Reset state variables
  - DO NOT remove form state GE here (already removed by NEW form)
```

### GE_*State Definitions

> **INV-1 Update:** Per GAS Audit, NO form state GE grants invulnerability. Only GA_FatherSacrifice provides invulnerability.

| GE | Grants Tags | Invulnerable |
|----|-------------|--------------|
| GE_CrawlerState | Effect.Father.FormState.Crawler | No |
| GE_ArmorState | Effect.Father.FormState.Armor | No (INV-1) |
| GE_ExoskeletonState | Effect.Father.FormState.Exoskeleton | No (INV-1) |
| GE_SymbioteState | Effect.Father.FormState.Symbiote | No (INV-1) |
| GE_EngineerState | Effect.Father.FormState.Engineer | No |

### Default Form at Spawn

AC_FatherCompanion includes `startup_effects: [GE_CrawlerState]` to ensure Father spawns with Crawler form identity.

---

## VERSION 6.0 CHANGES

| Change | Details |
|--------|---------|
| Full Rename | Renamed from Spider to Father throughout entire document |
| All References Updated | Tags, abilities, variables, functions, blueprints renamed |

---

## VERSION 5.14 CHANGES

| Change | Details |
|--------|---------|
| Section 60 Added | Demo Weapon Visual Reference - Dagger, Bow, Rifle, Greatsword configuration patterns |
| Section 60.3.1 Added | Bow Child GE Pattern - GE_EquipmentModifier_Bow demonstrates child GE organization |
| Section 61 Added | NPC Definition Visual Reference - NPC_Seth configuration as father template |
| Section 61.4 Added | ItemLoadout_Seth Data Table Pattern - LootTableRow struct, Item Collections |
| Section 61.5 Added | Father Item Loadout Options - Direct items vs IC_FatherForms vs Data Table |
| Section 62 Added | Activity Configuration Visual Reference - AC_RunAndGun as father template |
| Section 63 Added | Ability Configuration Visual Reference - AC_NPC_Default as father template |
| Section 64 Added | BPA_Attack_Melee Visual Reference - Activity scoring, class defaults |
| Section 65 Added | GoalGenerator_Attack Visual Reference - Complete auto-targeting system |
| Section 66 Added | Team Coordination System - Report Noise Event, NotifyTeammates pattern |
| Visual Confirmation | All patterns verified against Narrative Pro v2.2 demo assets |

---

## VERSION 5.13 CHANGES

| Change | Details |
|--------|---------|
| Tag Format Fix | Corrected State.Father.Alive to Father.State.Alive per DefaultGameplayTags_FatherCompanion_v3_5.ini line 150 |
| Section 7.4 Updated | Form Activation Tags now use correct Father.State.Alive format |
| Section 37.5 Updated | All form ability Activation Required Tags use correct format |
| Related Guide Updates | GA_FatherArmor v4.2, GA_FatherCrawler v3.3, GA_FatherEngineer v4.3, GA_FatherExoskeleton v3.10 |

---

## VERSION 5.12 CHANGES

| Change | Details |
|--------|---------|
| Section 58.7 Updated | Duplicate stat issue RESOLVED - form abilities no longer apply custom stat GEs |
| Section 58.8 Added | Child GE Architecture Decision - no Components, form tags from Activation Owned Tags |
| Section 58.9 Added | HandleEquip/HandleUnequip Lifecycle - C++ source code flow documentation |
| Section 58.10 Added | Dual Entry System - Equipment Effect Values map + Rating properties |
| Section 58.11 Added | Activities to Grant - EquippableItem AI behavior integration |
| Setup Guide v2.2 | Remove Grant Tags Components from child GEs in PHASE 30 |
| GA_FatherSymbiote v3.5 | Remove GE_SymbioteBoost variable and cleanup code |

---

## VERSION 5.11 CHANGES

| Change | Details |
|--------|---------|
| Section 52.3.1-52.3.7 Added | Attack Abilities detail - Melee, Firearms, Magic, Misc, Projectiles, BowArrows, Throwables |
| Section 52 Renumbered | 52.4-52.7 became 52.5-52.8 to accommodate new subsections |
| Section 58 Added | GE_EquipmentModifier Pattern - SetByCaller stat application via EquippableItem |
| Section 58.7 Added | NOTED: Potential duplicate stat issue for Armor and Symbiote forms |
| Section 59 Added | Narrative Pro 2.2 Dual Wield/Offhand System documentation |
| Section 59.5 Added | New 2.2 Attack Abilities summary |
| Section 59.6 Added | Arrow variant pattern for future father projectiles |
| 2.2 New Assets | Documented all offhand abilities (Melee, Firearms, Magic) and arrow variants |

---

## VERSION 5.10 CHANGES

| Change | Details |
|--------|---------|
| Section 55.1 Update | Added BTS_AdjustFollowSpeed to available services list with High relevance |
| Section 55.4-55.5 Added | BTS_ClearAIFocus complete documentation (class defaults, event graph) |
| Section 55.6-55.9 Added | BTS_SetAIFocus complete documentation (class defaults, activation flow, deactivation flow, auto-cleanup) |
| Section 55.10-55.12 Added | BTS_AdjustFollowSpeed complete documentation (class defaults, full event graph, speed matching logic) |
| Section 55.13 Added | Service Dependencies expanded table |
| Section 55.14 Update | Father Service Usage updated with CUSTOM status for following |
| Section 55.15 Update | Father Custom Follow System documentation (BT_FatherFollow removes BTS_SetAIFocus) |
| Father Follow Clarification | Father uses custom BT_FatherFollow without BTS_SetAIFocus (father faces movement direction, not stare at player) |
| Appendix E Update | Father Follow Services marked COMPLETED with custom implementation |
| Appendix E Update | Father ActivityConfiguration marked COMPLETED |
| Appendix E Update | GA_Death Integration marked COMPLETED |
| Related Guide Update | Father_Companion_System_Setup_Guide_v2_1 Phases 37-40 added |

---

## VERSION 5.9 CHANGES

| Change | Details |
|--------|---------|
| Section 54 Added | Behavior Tree Task System - BTTask_ActivateAbilityByClass usage, comparison with custom tasks |
| Section 55 Added | Behavior Tree Services - BTS_FocusAttackTarget, BTS_ClearAIFocus, BTS_SetAIFocus |
| Section 56 Added | Reference Asset Analysis - AC_NPC_Default, AC_Default (ActivityConfiguration), GA_Death pattern |
| Section 57 Added | Father to Narrative Pro Alignment - Complete mapping of father systems to Narrative Pro patterns |
| Appendix A Update | Added BTTask_ActivateAbilityByClass, BTS_FocusAttackTarget patterns |
| Appendix E Update | Added BT task decision, damage system decision |
| Guide Impact | GA_FatherLaserShot, GA_TurretShoot, GA_FatherElectricTrap guides need BTTask update |
| TOC Update | Added sections 54-57 |

---

## VERSION 5.8 CHANGES

| Change | Details |
|--------|---------|
| Section 52 Added | Narrative Pro 2.2 Content Folder Structure - complete plugin content organization |
| Section 53 Added | Narrative Pro 2.2 C++ Module Structure - all modules and key classes |
| Appendix C Update | Additional asset prefixes: BI_ (Blueprint Items), IC_ (Item Collections), DT_ (Data Tables), QBP_ (Quest Blueprints), A_ (Animation Sequences) |
| Appendix D.15 Added | Content Folder Quick Reference table |
| Appendix D.16 Added | C++ Module Quick Reference table |
| Plugin Version | Updated to Narrative Pro v2.2 BETA |
| TOC Update | Added sections 52-53 |

---

## VERSION 5.7 CHANGES

| Change | Details |
|--------|---------|
| Section 7.9 Added | Faction + GoalGenerator Attack Chain - automatic hostile detection flow from AIPerception to Goal_Attack |
| Section 7.10 Added | HandleDeath Parameters - KilledActor is Self (not attacker), accessing attacker via damage context |
| Section 7.11 Added | Hostiles Array vs Faction System - when to use manual Hostiles array, ShouldBeAggressiveTowardsTarget override |
| Appendix C Update | Separated AC_ (AbilityConfiguration) from ActConfig_ (ActivityConfiguration) prefixes |
| Section 21.1 Update | ActivityConfiguration naming convention clarified with ActConfig_ prefix |
| Appendix D.2 Update | Changed AC_FatherCompanion_Activity to ActConfig_FatherCompanion |

---

## VERSION 5.6 CHANGES

| Change | Details |
|--------|---------|
| PATTERN 7 Update | Cooldown System updated to GAS built-in pattern: CooldownGameplayEffectClass + CommitAbilityCooldown |
| Section 19.3 Update | Form Ability Configuration Template: Block Abilities with Tag replaced with Cooldown Gameplay Effect Class |
| Section 19.6 Update | Form Cooldown Implementation: Full built-in cooldown system documentation |
| Section 19.7.2 Update | Form Activation Tag Configuration: Updated for built-in cooldown |
| PATTERN 3 Update | Tag types table: Added Cooldown Gameplay Effect Class, removed Block Abilities with Tag for cooldown |
| Form Flow Updates | Steps 9 and 19 in form transition flows updated to use CommitAbilityCooldown |

---

## VERSION 5.5 CHANGES

| Change | Details |
|--------|---------|
| Section 8.4 Update | Grant Tags Navigation corrected: Use "Add to Inherited" array (not "Combined Tags" which is read-only) |
| Section 8.5 Added | UE 5.6 Gameplay Effect Components Reference table |
| Section 8.6 Corrected | Grant Tags Component Properties: Combined Tags is READ-ONLY, Add to Inherited is PRIMARY field |
| Section 8.7 Added | NarrativeAttributeSetBase Attributes complete list |
| Section 8.8 Added | Modifier Operations reference (Add Base, Multiply, Override, etc.) |
| Section 8.9 Added | Magnitude Calculation Types reference |
| Section 8.10 Added | Narrative Pro Execution Calculations reference |
| UE 5.6 Alignment | All GE component names verified against UE 5.6 editor |

---

## VERSION 5.4 CHANGES

| Change | Details |
|--------|---------|
| Section 1.1 Update | NarrativeCharacter.cpp: CanBeSeenFrom (1181-1218), PerformSightTrace (1220-1246), CalcSightStrength (265), IAISightTargetInterface (214), AddAbility (749-795), GrantAbilities (797-807) |
| Section 1.2 Update | NarrativeNPCCharacter.cpp: ASC Creation (37-43), ASC Init BeginPlay (59-71), InitNewCharacter (178-204) |
| Section 1.3 Update | NarrativeAbilitySystemComponent.cpp: AbilityInputTagPressed (260-316), AbilityInputTagReleased (318-351), ClearAbilitiesWithTag (353-364), FindAbilitiesWithTag (366-377) |
| Section 1.4 Update | NarrativeGameplayAbility.h: InputTag (24-25), bActivateAbilityOnGranted (28-29), OnAvatarSet (33), EndAbility (34) |
| Section 1.5 Update | NarrativeCombatAbility: Added complete .cpp line references for all implementations |
| Section 4.5 Update | Death/Damage Delegates (155-169), HandleOutOfHealth (111-143) |
| Section 7.3 Update | Attack Token System (122-150) |
| Section 7.8 Update | ArsenalStatics.h GetAttitude (80-81) |
| Section 10.3 Update | OnMovementModeChanged (105-161) |
| Section 13.8 Update | Load_Implementation (436-448) |
| C++ Audit | Comprehensive line number verification against Narrative Pro v2.2 source |

---

## VERSION 5.3 CHANGES

| Change | Details |
|--------|---------|
| Plugin Version | Updated from Narrative Pro v2.1 to v2.2 |
| Section 47 | Narrative Pro v2.2 New Systems Overview - File counts, new categories, backward compatibility |
| Section 48 | Projectile System - NarrativeProjectile, AbilityTask_SpawnProjectile, SetProjectileTargetData |
| Section 49 | Melee Multi-Hit System - CachedHitActors pattern, FCombatTraceData, bTraceMulti, debug CVars |
| Section 50 | Item Fragment System - AmmoFragment, PoisonableFragment, fragment composition patterns |
| Section 51 | Cover System - NarrativeRecastNavMesh, NarrativeTileCoverGenerator, cover queries |
| TOC Update | Added sections 47-51 |
| Appendix D Update | Added v2.2 systems quick reference tables |
| Appendix E Update | Added v2.2 migration decisions |

---

## VERSION 5.2 CHANGES

| Change | Details |
|--------|---------|
| Section 46 | Goal/Activity Follow System Architecture - Goal_FollowCharacter internals, SetupBlackboard pattern, NarrativeProSettings BBKey naming, NarrativeCharacterSubsystem lookup, speed matching, teleport binding, sprint threshold, Quest Task vs Goal system |
| TOC Update | Added section 46 |
| Appendix D Update | Added Goal/Activity Follow System Summary table |

---

## VERSION 5.1 CHANGES

| Change | Details |
|--------|---------|
| Section 42 | NPC Schedule System - TriggerSet, TriggerSchedule, NPCActivitySchedule, UScheduledBehavior |
| Section 43 | Interaction Slot System - UInteractionSlotBehavior, FInteractionSlotConfig, slot claiming |
| Section 44 | NarrativeInteractAbility - GA_Interact_Sit pattern, montage sections, prop attachment |
| Section 45 | Time of Day Triggers - BPT_TimeOfDayRange, BPT_Always, WaitTimeRange latent node |
| TOC Update | Added sections 42-45 |
| Appendix A Update | Added NPC Schedule verified patterns |
| Appendix C Update | Added BPT_, TS_, BP_Interactable prefixes |

---

## VERSION 5.0 CHANGES

| Change | Details |
|--------|---------|
| GA_Backstab Location | Moved from father recruitment grant to Player Default Abilities |
| BackstabAbilityHandle | Removed from BP_FatherCompanion (no longer needed) |
| Ability Handle Count | Reduced from 7 to 6 ability spec handles |
| Section 35.5 Update | InitOwnerReference no longer grants GA_Backstab |
| Section 35.8 Update | ClearOwnerReference no longer performs BackstabAbilityHandle cleanup |
| Section 36.12 Update | Removed BackstabAbilityHandle from ability handle list |
| Section 34.16 Update | Removed BackstabAbilityHandle from variable table |
| Design Rationale | Backstab is generic action game mechanic, not father-exclusive |

---

## VERSION 4.8 ADDITIONS

| Addition | Details |
|----------|---------|
| Section 13.5 Update | Added IsDeployed and DeployedTransform to SaveGame requirements |
| Section 13.6 New | Father SaveGame Variables Summary table with complete SaveGame specification |
| Section 34.16 Update | Added SaveGame column to Updated Father Variables Summary table |
| Section Renumbering | Sections 13.6-13.8 renumbered to 13.7-13.10 |

---

## VERSION 4.7 ADDITIONS

| Addition | Details |
|----------|---------|
| Section 39 | EndPlay Safety Net Pattern - Emergency cleanup when father destroyed unexpectedly |
| Section 40 | Cross-Actor Authority Gates - HasAuthority patterns for all cross-actor operations |
| Section 41 | Symbiote Auto-Return Fallback - Fallback logic when Armor activation fails after Symbiote timer |
| Section 7.4 Correction | Form abilities granting to player must use ServerOnly, not LocalPredicted |
| Section 36.14 | Manual Cleanup Pattern - Alternative to missing RemoveActiveEffectsWithSourceObject |
| Narrative Pro Audit | C++ source audit confirmed missing two-step cleanup, EndPlay safety, cross-ASC handle storage |
| NetUpdateFrequency | Narrative Pro already sets 100.0f on PlayerState (no manual setup needed) |
| ASC Replication | NarrativeNPCCharacter uses Mixed mode (Minimal recommended but Mixed works) |

---

## VERSION 4.6 ADDITIONS (SUPERSEDED BY v5.0)

| Addition | Details | Status |
|----------|---------|--------|
| Section 35.5 Update | InitOwnerReference now grants GA_Backstab to player ASC and stores BackstabAbilityHandle | **REMOVED in v5.0** |
| Section 35.8 Update | ClearOwnerReference now performs two-step cleanup for BackstabAbilityHandle before clearing references | **REMOVED in v5.0** |
| Recruitment Ability Grant | GA_Backstab granted on recruitment, removed on dismiss (not via Player AbilityConfiguration) | **CHANGED in v5.0** - Now Player Default Ability |
| Backstab Handle Lifecycle | Grant in InitOwnerReference, cleanup in ClearOwnerReference | **REMOVED in v5.0** |

---

## VERSION 4.5 ADDITIONS

| Addition | Details |
|----------|---------|
| Section 36.1 | Critical Limitation - Cancel Abilities with Tag does NOT work cross-actor |
| Section 36.10-36.13 | Orphaned abilities warning, two-step cleanup pattern, ability handle variables, form EndAbility cleanup flows |
| Section 14.8 Update | Added ability handles (FGameplayAbilitySpecHandle) for granted sub-abilities |
| Section 14.10 Update | Two-step pattern: Cancel Ability then Set Remove Ability On End |
| Ability Handle Variables | DomeAbilityHandle, DomeBurstAbilityHandle, DashAbilityHandle, SprintAbilityHandle, StealthAbilityHandle, ProximityAbilityHandle (6 handles - BackstabAbilityHandle removed in v5.0) |
| Cross-Actor Cleanup Pattern | Cancel Ability (stops execution) then Set Remove Ability On End (removes spec safely) |
| Form EndAbility Flows | Two-step cleanup for GA_FatherArmor, GA_FatherExoskeleton, GA_FatherSymbiote |
| Research Confirmed | ClearAbility alone unsafe for active abilities, SetRemoveAbilityOnEnd alone does not stop execution |

---

## VERSION 4.4 ADDITIONS

| Addition | Details |
|----------|---------|
| Section 35 | NarrativeEvent System - NE_SetFatherOwner, NE_ClearFatherOwner, ExecuteEvent override |
| Section 36 | Cross-Actor Ability Granting - SourceObject pattern, granting to player ASC, handle management |
| Section 37 | Ability Validation Architecture - Tags + CanActivateAbility hybrid, Father.State.Recruited |
| Section 38 | Father Death and Invulnerability - Form-based vulnerability, Narrative.State.Invulnerable |
| Section 14 Expansion | Per-form handle tracking with TMap pattern, CancelAbilityHandle for cross-actor cleanup |
| Section 34 Expansion | CharacterDefinition persistence, PrepareForSave/Initialize events, OwnerPlayerDefinition |
| NarrativeEvent Pattern | ExecuteEvent(Target, Controller, NarrativeComponent) signature |
| Validation Pipeline | Tag blocking before activation, CanActivateAbility for complex logic |
| Handle Management | FFatherFormGrantedHandles struct pattern from Lyra |
| Death Architecture | Father dies only in Crawler/Engineer forms (detached) |

---

## VERSION 4.3 ADDITIONS

| Addition | Details |
|----------|---------|
| Section 32 | Blackboard System - BB_ keys for all Narrative Pro behavior trees |
| Section 33 | Dialogue-Driven NPC Control - Goal assignment via dialogue events |
| Section 34 | Father-Player Reference Architecture - OwnerPlayer vs Goal system responsibilities |
| BB_Attack Analysis | Base blackboard with TargetLocation, AttackTarget, TargetRotation, SelfActor |
| BB_FollowCharacter Analysis | FollowTarget key for Goal Follow Character |
| Dialogue Event: AI Add Goal To NPC | Configuration for dialogue-triggered goal assignment |
| Goal Follow Character | Native goal type with CD_DefaultPlayer integration |
| Father Reference Split | Clarified what OwnerPlayer handles vs what Goal system handles |

---

## VERSION 4.2 ADDITIONS

| Addition | Details |
|----------|---------|
| Section 31 | Gameplay Cue System - class hierarchy, Narrative Pro patterns, camera shake, triggering |
| GC_TakeDamage Analysis | BurstLatent pattern with directional hit reactions, Niagara VFX |
| GC_WeaponFire Analysis | Burst (non-instanced) pattern for weapon effects |
| Camera Shake System | Perlin Noise vs Wave Oscillator patterns, BP_PistolCameraShake, BP_RifleCameraShake |
| GCN Effects Structure | Burst Particles, Sounds, Camera Shake, Decal configuration |
| Father Cue Recommendations | Form transition, attack, status effect cue patterns |

---

## VERSION 4.1 ADDITIONS

| Addition | Details |
|----------|---------|
| Transition Animation | 5s Niagara VFX during form change |
| Father Invulnerability | Narrative.State.Invulnerable during 5s transition |
| Form Activation Tags | Required: Father.State.Alive, Blocked: Transitioning, SymbioteLocked, Dormant |
| Symbiote Lock | Father.State.SymbioteLocked blocks T wheel during 30s |
| Symbiote Auto-Return | Returns to Armor form (not Crawler) after 30s |
| Document References | Updated to v1_6 Design Doc, v3_3 GameplayTags |

---

## VERSION 4.0 ARCHITECTURE CHANGES

| Change | Old (v3.6) | New (v4.0) |
|--------|------------|------------|
| Form Switching | F key detach to Crawler, then T key to new form | T key form wheel direct form-to-form |
| Detach Abilities | GA_FatherDetach, GA_TurretRecall | REMOVED - handled by Cancel Abilities With Tag |
| Activation Owned Tags | "Do not replicate" | REPLICATE (ReplicateActivationOwnedTags enabled in project) |
| Form State GEs | Required for tag replication | Optional (stat modifiers only) |
| Effect Removal | Source-based via GA_FatherDetach | EndAbility in each form ability |
| Form Cooldown | None | 15s shared (Cooldown.Father.FormChange) |
| Symbiote Duration | Unlimited | 30 seconds fixed |
| Exoskeleton Speed | +25% | +50% |
| Exoskeleton Jump | Not specified | +30% |
| Exoskeleton Attack | Not specified | +10 AttackRating |

---

## TABLE OF CONTENTS

1. [NARRATIVE PRO C++ SOURCE LOCATIONS](#section-1-narrative-pro-c-source-locations)
2. [ATTRIBUTE SYSTEM](#section-2-attribute-system)
3. [INPUT TAG SYSTEM](#section-3-input-tag-system)
4. [DAMAGE SYSTEM](#section-4-damage-system)
5. [AI PERCEPTION SYSTEM](#section-5-ai-perception-system)
6. [NPCDEFINITION SYSTEM](#section-6-npcdefinition-system)
7. [MULTIPLAYER CONFIGURATION](#section-7-multiplayer-configuration)
8. [UE 5.6 GAMEPLAY EFFECT NAVIGATION](#section-8-ue-56-gameplay-effect-navigation)
9. [BLUEPRINT IMPLEMENTATION PATTERNS](#section-9-blueprint-implementation-patterns)
10. [TAG SYSTEM REFERENCE](#section-10-tag-system-reference)
11. [ANTI-PATTERNS TO AVOID](#section-11-anti-patterns-to-avoid)
12. [PATTERN SELECTION GUIDE](#section-12-pattern-selection-guide)
13. [SAVE SYSTEM](#section-13-save-system)
14. [GAMEPLAY EFFECT REMOVAL PATTERNS](#section-14-gameplay-effect-removal-patterns)
15. [ABILITY CLEANUP PATTERNS](#section-15-ability-cleanup-patterns)
16. [PERFORMANCE PATTERNS](#section-16-performance-patterns)
17. [MULTIPLAYER SYNC PATTERNS](#section-17-multiplayer-sync-patterns)
18. [NARRATIVE PRO C++ CLEANUP PATTERNS](#section-18-narrative-pro-c-cleanup-patterns-reference)
19. [FORM STATE ARCHITECTURE](#section-19-form-state-architecture)
20. [ABILITYCONFIGURATION SYSTEM](#section-20-abilityconfiguration-system)
21. [ACTIVITYCONFIGURATION SYSTEM](#section-21-activityconfiguration-system)
22. [NPC ACTIVITY SYSTEM (BPA_)](#section-22-npc-activity-system-bpa_)
23. [GOAL SYSTEM](#section-23-goal-system)
24. [ENVIRONMENT QUERY SYSTEM (EQS)](#section-24-environment-query-system-eqs)
25. [COMBAT ABILITY HIERARCHY](#section-25-combat-ability-hierarchy)
26. [GAMEPLAY EFFECT PATTERNS](#section-26-gameplay-effect-patterns)
27. [NARRATIVE PRO ABILITY EXAMPLES](#section-27-narrative-pro-ability-examples)
28. [INPUT MAPPING CONTEXT](#section-28-input-mapping-context)
29. [GAMEPLAY EVENT TAGS](#section-29-gameplay-event-tags)
30. [ABILITY PATTERN SUMMARY](#section-30-ability-pattern-summary)
31. [GAMEPLAY CUE SYSTEM](#section-31-gameplay-cue-system)
32. [BLACKBOARD SYSTEM](#section-32-blackboard-system)
33. [DIALOGUE-DRIVEN NPC CONTROL](#section-33-dialogue-driven-npc-control)
34. [FATHER-PLAYER REFERENCE ARCHITECTURE](#section-34-father-player-reference-architecture)
35. [NARRATIVEEVENT SYSTEM](#section-35-narrativeevent-system)
36. [CROSS-ACTOR ABILITY GRANTING](#section-36-cross-actor-ability-granting)
37. [ABILITY VALIDATION ARCHITECTURE](#section-37-ability-validation-architecture)
38. [FATHER DEATH AND INVULNERABILITY](#section-38-father-death-and-invulnerability)
39. [ENDPLAY SAFETY NET PATTERN](#section-39-endplay-safety-net-pattern)
40. [CROSS-ACTOR AUTHORITY GATES](#section-40-cross-actor-authority-gates)
41. [SYMBIOTE AUTO-RETURN FALLBACK](#section-41-symbiote-auto-return-fallback)
42. [NPC SCHEDULE SYSTEM](#section-42-npc-schedule-system)
43. [INTERACTION SLOT SYSTEM](#section-43-interaction-slot-system)
44. [NARRATIVEINTERACTABILITY SYSTEM](#section-44-narrativeinteractability-system)
45. [TIME OF DAY TRIGGERS](#section-45-time-of-day-triggers)
46. [GOAL/ACTIVITY FOLLOW SYSTEM ARCHITECTURE](#section-46-goalactivity-follow-system-architecture)
47. [NARRATIVE PRO v2.2 NEW SYSTEMS OVERVIEW](#section-47-narrative-pro-v22-new-systems-overview)
48. [PROJECTILE SYSTEM](#section-48-projectile-system)
49. [MELEE MULTI-HIT SYSTEM](#section-49-melee-multi-hit-system)
50. [ITEM FRAGMENT SYSTEM](#section-50-item-fragment-system)
51. [COVER SYSTEM](#section-51-cover-system)
52. [NARRATIVE PRO 2.2 CONTENT FOLDER STRUCTURE](#section-52-narrative-pro-22-content-folder-structure)
53. [NARRATIVE PRO 2.2 C++ MODULE STRUCTURE](#section-53-narrative-pro-22-c-module-structure)
54. [BEHAVIOR TREE TASK SYSTEM](#section-54-behavior-tree-task-system)
55. [BEHAVIOR TREE SERVICES](#section-55-behavior-tree-services)
56. [REFERENCE ASSET ANALYSIS](#section-56-reference-asset-analysis)
57. [FATHER TO NARRATIVE PRO ALIGNMENT](#section-57-father-to-narrative-pro-alignment)

---

## SECTION 1: NARRATIVE PRO C++ SOURCE LOCATIONS

### 1.1) NarrativeCharacter

| Function | File | Lines | Purpose |
|----------|------|-------|---------|
| CanBeSeenFrom() | NarrativeCharacter.cpp | 1181-1218 | IAISightTargetInterface - visibility check |
| PerformSightTrace() | NarrativeCharacter.cpp | 1220-1246 | Actual trace for sight detection |
| CalcSightStrength() | NarrativeCharacter.h | 265 | BlueprintImplementableEvent - override in BP |
| IAISightTargetInterface | NarrativeCharacter.h | 214 | Interface declaration |
| AddAbility() | NarrativeCharacter.cpp | 749-795 | Grant ability with InputTag to DynamicSpecSourceTags |
| GrantAbilities() | NarrativeCharacter.cpp | 797-807 | Batch grant abilities |

### 1.2) NarrativeNPCCharacter

| Function | File | Lines | Purpose |
|----------|------|-------|---------|
| ASC Creation | NarrativeNPCCharacter.cpp | 37-43 | Constructor creates AbilitySystemComponent |
| ASC Init | NarrativeNPCCharacter.cpp | 59-71 | BeginPlay calls InitAbilityActorInfo |
| InitNewCharacter | NarrativeNPCCharacter.cpp | 178-204 | Grants abilities from NPCDefinition |

### 1.3) NarrativeAbilitySystemComponent

| Function | File | Lines | Purpose |
|----------|------|-------|---------|
| AbilityInputTagPressed | NarrativeAbilitySystemComponent.cpp | 260-316 | Activates abilities by InputTag |
| AbilityInputTagReleased | NarrativeAbilitySystemComponent.cpp | 318-351 | Releases abilities by InputTag |
| ClearAbilitiesWithTag | NarrativeAbilitySystemComponent.cpp | 353-364 | Removes abilities with matching tag |
| FindAbilitiesWithTag | NarrativeAbilitySystemComponent.cpp | 366-377 | Finds abilities with matching tag |

### 1.4) NarrativeGameplayAbility

| Property | File | Lines | Purpose |
|----------|------|-------|---------|
| InputTag | NarrativeGameplayAbility.h | 24-25 | Tag-based input binding (Narrative.Input.*) |
| bActivateAbilityOnGranted | NarrativeGameplayAbility.h | 28-29 | Auto-activate on grant (for autonomous abilities) |
| OnAvatarSet | NarrativeGameplayAbility.h | 33 | Override for activate-on-grant logic |
| EndAbility | NarrativeGameplayAbility.h | 34 | Override for cleanup logic |

### 1.5) NarrativeCombatAbility

| Property | File | Lines | Purpose |
|----------|------|-------|---------|
| bRequiresAmmo | NarrativeCombatAbility.h | 170 | Set FALSE for unarmed/father attacks |
| DefaultAttackDamage | NarrativeCombatAbility.h | 175 | Base damage value for ability |
| FCombatTraceData | NarrativeCombatAbility.h | 13-34 | Trace configuration struct |
| bTraceMulti | NarrativeCombatAbility.h | 33 | Multi-hit trace toggle |

| Function | File | Lines | Purpose |
|----------|------|-------|---------|
| GenerateTargetDataUsingTrace | NarrativeCombatAbility.cpp | 116-164 | Auto sphere sweep hit detection |
| GetTargetDataUsingTrace | NarrativeCombatAbility.cpp | 166-207 | Returns target data without finalizing |
| FinalizeTargetData | NarrativeCombatAbility.cpp | 209-238 | Sends target data to server |
| HandleTargetData | NarrativeCombatAbility.cpp | 240-243 | Override for damage application |
| GetAttackDamage | NarrativeCombatAbility.cpp | 420-423 | Override for dynamic damage calculation |
| PerformTrace | NarrativeCombatAbility.cpp | 245-283 | Single trace execution |
| PerformTraceMulti | NarrativeCombatAbility.cpp | 285-322 | Multi-hit trace execution |

### 1.6) Key Reference Files

| File | Contains |
|------|----------|
| NarrativeDamageExecCalc.cpp | Damage formula, invulnerable check |
| NarrativeAttributeSetBase.h | Available attributes list |
| NarrativeGameplayTags.cpp | Built-in tag definitions |
| NPCSpawnComponent.cpp | NPCSpawner initialization |

---

## SECTION 2: ATTRIBUTE SYSTEM

### 2.1) NarrativeAttributeSetBase Attributes

| Attribute | Exists | Type | Notes |
|-----------|--------|------|-------|
| Health | YES | Float | Current health |
| MaxHealth | YES | Float | Maximum health |
| Stamina | YES | Float | Ability resource |
| MaxStamina | YES | Float | Maximum stamina |
| StaminaRegenRate | YES | Float | Passive regen per second |
| AttackDamage | YES | Float | Base damage value |
| AttackRating | YES | Float | Damage multiplier (100 = 2x damage) |
| Armor | YES | Float | Damage reduction (100 = 50% reduction) |
| StealthRating | YES | Float | 100 = invisible, 50 = 50% harder to detect |
| XP | YES | Float | Experience points |
| Damage | YES | Meta | Not replicated, reset after use |
| Heal | YES | Meta | Not replicated, reset after use |
| MovementSpeed | NO | - | Does NOT exist - use CharacterMovement directly |

### 2.2) Movement Speed Handling

MovementSpeed attribute does NOT exist in NarrativeAttributeSetBase.

Correct method:
1. Get CharacterMovement Component from avatar actor
2. Store original Max Walk Speed in variable (for restoration)
3. Set Max Walk Speed directly on component
4. Restore original speed on ability end

### 2.3) Custom Energy Attributes

Custom attributes like DomeEnergy, SymbioteCharge, etc. should be created as proper GAS attributes using the "Gameplay Blueprint Attributes" plugin.

| Method | Recommendation |
|--------|----------------|
| Blueprint Float Variables | NOT recommended for energy systems |
| Gameplay Blueprint Attributes Plugin | RECOMMENDED |

Benefits of proper GAS attributes:

| Feature | Blueprint Float | GAS Attribute |
|---------|-----------------|---------------|
| Gameplay Effect modification | Manual | Automatic |
| Attribute change delegates | Manual | Built-in |
| Multiplayer replication | Manual | Native GAS |
| NarrativeSaveAttribute support | No | Yes |
| ExecCalc integration | No | Yes |
| Aggregator support (temp/perm mods) | No | Yes |

Example attribute sets in Father Companion:

| Attribute Set | Attributes | Used By |
|---------------|------------|---------|
| AS_DomeAttributes | DomeEnergy, MaxDomeEnergy, AbsorptionPercentage | GA_ProtectiveDome |

---

## SECTION 3: INPUT TAG SYSTEM

### 3.1) Input Flow Architecture

| Step | Component | Flow |
|------|-----------|------|
| 1 | Key Press | Physical input |
| 2 | Input Action (IA_) | Unreal input binding |
| 3 | Input Mapping Context (IMC_) | Context-based mapping |
| 4 | NarrativeAbilityInputMapping (DA_) | Tag assignment |
| 5 | Gameplay Tag | Narrative.Input.* |
| 6 | NarrativeASC | Ability lookup |
| 7 | Ability Activation | Matched ability executes |

### 3.2) Father Input Tag Mapping

| Key | Input Action | Input Tag |
|-----|--------------|-----------|
| T | IA_FatherFormWheel | Narrative.Input.Father.FormWheel |
| Q | IA_FatherAbility1 | Narrative.Input.Father.Ability1 |
| E | IA_FatherAbility2 | Narrative.Input.Father.Ability2 |
| R | IA_FatherAbility3 | Narrative.Input.Father.Ability3 |
| V (hold) | IA_FatherOpenWheel | Narrative.Input.Father.OpenWheel |

### 3.3) Input Assets Required

| Asset | Type | Location |
|-------|------|----------|
| IA_FatherFormWheel | Input Action | /Content/Input/Actions/ |
| IA_FatherAbility1-3 | Input Action | /Content/Input/Actions/ |
| IMC_FatherCompanion | Input Mapping Context | /Content/Input/Contexts/ |
| DA_FatherInputMapping | NarrativeAbilityInputMapping | /Content/Father/Data/ |

### 3.4) Input Tag Patterns

Shared Input, Form-Specific Activation:

| Ability | InputTag | ActivationRequired |
|---------|----------|--------------------|
| GA_ElectricTrap | Narrative.Input.Father.Ability1 | Effect.Father.FormState.Engineer |
| GA_ExoskeletonDash | Narrative.Input.Father.Ability1 | Effect.Father.FormState.Exoskeleton |

AI + Manual Trigger:
- GA_FatherElectricTrap: InputTag=Narrative.Input.Father.Ability1
- AI uses TryActivateAbilityByClass, Player uses Q key

Passive Auto-Activation:
- GA_ProtectiveDome: InputTag=None, bActivateAbilityOnGranted=True

### 3.5) How Abilities Get Input Tags (C++ Reference)

NarrativeCharacter.cpp lines 773-777 (within AddAbility function):

| Step | Code Element | Purpose |
|------|--------------|---------|
| 1 | FGameplayAbilitySpec(Ability, Level, INDEX_NONE, SourceObject) | Create spec |
| 2 | AbilitySpec.GetDynamicSpecSourceTags().AddTag(InputTag) | Attach input tag |
| 3 | AbilitySystemComponent->GiveAbility(AbilitySpec) | Grant to ASC |

### 3.6) How Input Activates Abilities (C++ Reference)

NarrativeAbilitySystemComponent.cpp lines 283-311 (within AbilityInputTagPressed function):

| Step | Logic |
|------|-------|
| 1 | Loop through ActivatableAbilities.Items |
| 2 | Check Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag) |
| 3 | If match and not active: TryActivateAbility(Spec.Handle) |
| 4 | If match and active: AbilitySpecInputPressed(Spec) |

---

## SECTION 4: DAMAGE SYSTEM

### 4.1) NarrativeDamageExecCalc Formula

| Calculation | Formula |
|-------------|---------|
| AttackMultiplier | 1.0 + (AttackRating / 100.0) |
| DefenceMultiplier | 1.0 + (Armor / 100.0) |
| FinalDamage | (BaseDamage * AttackMultiplier) / DefenceMultiplier |

### 4.2) Auto-Checks Performed

| Check | Behavior |
|-------|----------|
| Narrative.State.Invulnerable tag | Blocks ALL damage automatically |
| Armor attribute | Reduces damage per formula |
| AttackRating attribute | Multiplies damage per formula |

### 4.3) Damage Gameplay Effect Configuration

| Step | Action |
|------|--------|
| 1 | Create GameplayEffect blueprint (GE_YourDamage) |
| 2 | Set Duration Policy: Instant |
| 3 | Add Executions component (UE 5.6 method) |
| 4 | Set Calculation Class: NarrativeDamageExecCalc |
| 5 | Add Asset Tags component with Effect.Damage tag |

### 4.4) SetByCaller Tags

| Tag | Purpose |
|-----|---------|
| SetByCaller.Damage | Damage effects (GE_WeaponDamage, GE_Damage_SetByCaller) |
| SetByCaller.Heal | Heal effects (GE_Heal_SetByCaller) |
| SetByCaller.Duration | Duration-based effects (GE_ItemGameplayEffect_Duration) |
| SetByCaller.Health | Direct health modification (GE_ItemGameplayEffect) |
| SetByCaller.Stamina | Direct stamina modification (GE_ItemGameplayEffect) |
| SetByCaller.AttackDamage | Attack damage modifier |
| SetByCaller.AttackRating | Attack rating modifier (GE_EquipmentMod) |
| SetByCaller.Armor | Armor modifier (GE_EquipmentMod) |
| SetByCaller.StealthRating | Stealth rating modifier (GE_EquipmentMod) |

### 4.5) Death and Damage Delegates

File: NarrativeAbilitySystemComponent.h (lines 155-169)

| Delegate | Fires When | Use Case |
|----------|------------|----------|
| OnDied | Health reaches zero | Detach father, play death anim, respawn |
| OnDamagedBy | Any damage received | Trigger defensive abilities, UI feedback |
| OnDealtDamage | Damage dealt to target | Combo tracking, kill confirmation |
| OnHealedBy | Any healing received | UI feedback, buff tracking |

Automatic death handling (NarrativeAbilitySystemComponent.cpp lines 111-143):

| Step | Action |
|------|--------|
| 1 | Health reaches zero (server authority) |
| 2 | Check State.IsDead tag not already present |
| 3 | Fire GameplayEvent.Death event |
| 4 | Set bIsDead = true |
| 5 | Broadcast OnDied delegate |

Father death response pattern:

| Step | Action |
|------|--------|
| 1 | Bind to OnDied delegate in BeginPlay |
| 2 | Detach from player (if attached) |
| 3 | Cancel all active abilities |
| 4 | Play death animation |
| 5 | Start respawn timer |
| 6 | Return to Crawler form on respawn |

---

## SECTION 5: AI PERCEPTION SYSTEM

### 5.1) BP_NarrativeNPCController Components

| Component | Type | Purpose |
|-----------|------|---------|
| AIPerception | UAIPerceptionComponent | Detects actors via senses |
| NPCActivity | UNPCActivityComponent | Schedules/activities |
| Interaction | UNPCInteractionComponent | Player interaction |
| PathFollowing | UNarrativePathFollowingComp | Navigation |

### 5.2) AIPerception Senses

| Sense | Class | Detects |
|-------|-------|---------|
| Sight | AISense_Sight | Visual detection |
| Hearing | AISense_Hearing | Sound detection |
| Damage | AISense_Damage | Damage events |

### 5.3) ViewedCharacter Variable (LEGACY - v5.0 and earlier)

> **NOTE (v5.1):** ViewedCharacter approach replaced by Goal_Attack Query. See Section 5.5.

| Property | Value |
|----------|-------|
| Type | NarrativePlayerCharacter (NOT Actor) |
| Location | BP_NarrativeNPCController |
| Purpose | Stores currently seen PLAYER |
| Status | **DEPRECATED** - Goal_Attack approach preferred |

### 5.4) StealthRating Usage

| Value | Effect |
|-------|--------|
| 0 | Normal perception |
| 50 | 50% harder to detect |
| 100 | Fully invisible to AI |

### 5.5) Backstab Detection Logic (v5.1 - Goal_Attack Query)

**v5.1 Update:** Backstab detection now uses Narrative Pro's built-in Goal_Attack system instead of custom ViewedCharacter binding.

#### How Goal_Attack Detection Works

```
AIPerception detects player → GoalGenerator_Attack.OnPerceptionUpdated →
Faction check → If HOSTILE → TryAddAttackGoalFromActor →
Goal_Attack created with TargetToAttack = detected actor
```

#### CheckBackstabCondition Query Flow

| Step | Action | Result |
|------|--------|--------|
| 1 | Get enemy's NPCActivityComponent | Activity system handles goals |
| 2 | Call GetCurrentActivityGoal() | Returns current activity goal |
| 3 | Cast to Goal_Attack | Check if enemy is attacking |
| 4 | Get TargetToAttack property | Who enemy is attacking |
| 5 | Compare: TargetToAttack != Player | Backstab valid if NOT attacking player |

#### Backstab Validity Table

| Enemy State | Goal_Attack | TargetToAttack | Backstab Valid |
|-------------|-------------|----------------|----------------|
| Idle/Patrolling | None | N/A | **YES** |
| Attacking Father | Goal_Attack | Father | **YES** |
| Attacking other NPC | Goal_Attack | Other NPC | **YES** |
| Attacking Player | Goal_Attack | Player | **NO** |

#### Decoy Tactical Flow (v5.1)

| Step | Action | Goal_Attack State |
|------|--------|-------------------|
| 1 | Player sends Father toward enemy | No Goal_Attack yet |
| 2 | Enemy AIPerception detects Father | GoalGenerator_Attack fires |
| 3 | Faction check: Father = Hostile | TryAddAttackGoalFromActor |
| 4 | Goal_Attack created | TargetToAttack = Father |
| 5 | Player flanks from behind | Player not detected |
| 6 | Player attacks enemy | CheckBackstabCondition runs |
| 7 | Query: Goal_Attack.TargetToAttack | Returns Father |
| 8 | Compare: Father != Player | TRUE |
| 9 | **BACKSTAB VALID** | Apply GE_BackstabBonus (+25) |

**Benefits of Goal_Attack Approach:**
- Uses existing Narrative Pro infrastructure
- No custom perception binding required
- No BP_BackstabNPCController modification needed
- Works automatically with faction system

---

## SECTION 6: NPCDEFINITION SYSTEM

### 6.1) AbilityConfiguration (AC_*)

Grants abilities at spawn via AbilitiesToGrant array.
Use for baseline abilities always present.

### 6.2) DefaultItemLoadout

Items added to inventory on spawn.
EquippableItems auto-equip if slot empty (ShouldUseOnAdd).

### 6.3) Automatic Init Flow

| Step | Action |
|------|--------|
| 1 | NPCSpawner calls InitNewCharacter(NPCDefinition) |
| 2 | Grants abilities from AbilityConfiguration |
| 3 | Adds items from DefaultItemLoadout |
| 4 | Zero blueprint code needed |

### 6.4) Inherited Components (NarrativeNPCCharacter)

| Component | Type | Auto-Created |
|-----------|------|--------------|
| AbilitySystemComponent | UNarrativeAbilitySystemComponent | YES |
| AttributeSet | UNarrativeAttributeSetBase | YES |
| InventoryComponent | UNarrativeInventoryComponent | YES |
| EquipmentComponent | UEquipmentComponent | YES |
| TradingInventoryComponent | UNarrativeInventoryComponent | YES |
| NPCInteractableComponent | UNPCInteractable | YES |

Do NOT create these in Blueprint - they already exist.

### 6.5) NPCSpawner Capabilities

| Feature | Provided | Notes |
|---------|----------|-------|
| Actor spawning | YES | At designated world location |
| Respawn conditions | YES | Streaming, distance culling |
| Save system integration | YES | Persistent NPCs across sessions |
| Goal assignment | YES | Via OptionalGoal field |
| Follow behavior | YES | NPCGoal_FollowPlayer |
| Custom initialization | NO | Must implement in Blueprint BeginPlay |
| Player-specific assignments | NO | Manual setup for multiplayer |
| Bidirectional references | NO | Must set OwnerPlayer and FatherCompanionRef manually |

### 6.6) NPCSpawner Init Flow

| Step | Action |
|------|--------|
| 1 | NPCSpawner.SpawnNPC() |
| 2 | Spawn actor at SpawnComponent location |
| 3 | Call InitNewCharacter(NPCDefinition) |
| 4 | Grant abilities from AbilityConfiguration |
| 5 | Add items from DefaultItemLoadout |
| 6 | Assign OptionalGoal (if set) |
| 7 | Actor BeginPlay executes (Blueprint logic here) |

### 6.7) Bidirectional Reference Setup

Required variables for Father-Player communication:

| Variable | Location | Type | Replication |
|----------|----------|------|-------------|
| OwnerPlayer | BP_FatherCompanion | NarrativeCharacter (Object Reference) | Replicated |
| FatherCompanionRef | Player Blueprint | BP_FatherCompanion (Object Reference) | Replicated |

Reference initialization pattern (in Father BeginPlay):

| Step | Action |
|------|--------|
| 1 | Event BeginPlay |
| 2 | Has Authority? (Branch) |
| 3 | TRUE: Delay 0.1s (ensures player fully initialized) |
| 4 | Get Player Controller (Index 0) |
| 5 | Get Controlled Pawn |
| 6 | Cast to Player Class |
| 7 | Set OwnerPlayer (on Self) |
| 8 | Set FatherCompanionRef (on Player, pass Self) |

### 6.8) ASC Initialization Timing

| Event | ASC Status | Safe Operations |
|-------|------------|-----------------|
| NarrativeNPCCharacter Constructor | Created | None (too early) |
| NarrativeNPCCharacter BeginPlay (C++) | InitAbilityActorInfo called | All ASC operations |
| Blueprint BeginPlay | Fully initialized | Grant abilities, apply GE, set tags |

No manual ASC setup required - NarrativeNPCCharacter handles everything before Blueprint BeginPlay executes.

---

## SECTION 7: MULTIPLAYER CONFIGURATION

### 7.1) Standard Settings

| Setting | Value | Reason |
|---------|-------|--------|
| Instancing Policy | Instanced Per Actor | One instance per ASC, best performance |
| Net Execution Policy | Local Predicted | Client predicts, server validates |
| Replication Policy | Replicate Yes | Effects and tags replicate |
| Replicate Input Directly | UNCHECKED | Uses Narrative Pro input system |
| Server Respects Remote Ability Cancellation | FALSE | Server makes final decision |

### 7.2) Authority Checking Pattern

| Step | Action |
|------|--------|
| 1 | Event ActivateAbility |
| 2 | Has Authority (returns Bool) |
| 3 | Branch on result |
| 4 | TRUE (Server): Grant abilities, spawn actors, modify attributes |
| 5 | FALSE (Client): Skip server-only logic |

### 7.3) Attack Token System

File: NarrativeAbilitySystemComponent.h (lines 122-150)

NPC combat coordination via token-based aggression management:

| Property | Type | Purpose |
|----------|------|---------|
| NumAttackTokens | int32 | How many NPCs can attack this target simultaneously |
| AttackPriority | float | Higher value = more likely to receive attack token |
| GrantedAttackTokens | TArray | NPCs currently granted attack tokens |

| Function | Purpose |
|----------|---------|
| TryClaimToken(Claimer) | NPC requests permission to attack |
| ReturnToken(Returner) | NPC finished attacking, releases token |
| ReturnTokenAtIndex(Index) | Return token at specific index |
| ShouldImmediatelyStealToken | Virtual, override for custom steal logic |
| CanStealToken | Virtual, checks if token can be stolen |
| GetNumAttackTokens | Virtual, returns max token count |
| GetAvailableAttackTokens | Returns unclaimed token count |
| GetNumGrantedAttackTokens | Returns claimed token count |

Father companion integration:

| Configuration | Value | Effect |
|---------------|-------|--------|
| NumAttackTokens on Player | 2-3 | Limits simultaneous attackers on player |
| AttackPriority on Father | High | Father prioritized for token allocation |

Token flow:

| Step | Action |
|------|--------|
| 1 | Enemy NPC wants to attack player |
| 2 | TryClaimToken(Self) on player ASC |
| 3 | If token available: Claim granted, begin attack |
| 4 | If no tokens: Wait or find different target |
| 5 | After attack: ReturnToken(Self) to release |

### 7.4) Net Execution Policy by Ability Type

Server Only requires ANY of these conditions:
1. Ability is Passive/Autonomous (no player input)
2. Ability is owned by NPC and grants abilities to another ASC
3. Ability applies GameplayEffects cross-actor

| Ability Type | Owner | Recommended Policy | Reason |
|--------------|-------|-------------------|--------|
| Player Input Actions | Player | Local Predicted | Responsive feel, client prediction improves UX |
| Passive + NPC-Owned | NPC (Father) | Server Only | No input to predict, NPC is server-authoritative |
| Passive + Player-Owned | Player | Local Predicted | Player abilities benefit from prediction |
| AI-Triggered | NPC | Server Only | AI runs on server |
| Form Abilities (grants to player) | Father (NPC) | Server Only | Cross-actor grants require server authority |
| Form Abilities (visual only) | Father (NPC) | Server Initiated | Visual feedback with server start |
| Player-triggered on NPC | Father (NPC) | Server Initiated | Player triggers, server validates |

Policy Behavior:

| Policy | Behavior |
|--------|----------|
| Local Predicted | Client predicts, server confirms, rollback on mismatch |
| Local Only | Client only, no server involvement (cosmetic only) |
| Server Initiated | Server starts, replicates to clients |
| Server Only | Server executes, clients see results via replication |

Father Companion Ability Recommendations:

| Ability | Owner | Type | Recommended Policy | Reason |
|---------|-------|------|--------------------|--------|
| GA_FatherAttack | Father (NPC) | Autonomous combat | Server Only | AI-triggered |
| GA_ProximityStrike | Father (NPC) | Passive AOE | Server Only | NPC passive |
| GA_FatherMark | Father (NPC) | Passive detection | Server Only | NPC passive |
| GA_Backstab | Player | Passive damage bonus | Local Predicted | Player Default Ability |
| GA_FatherCrawler | Father (NPC) | Form (follow mode) | Server Only | NPC form ability |
| GA_FatherArmor | Father (NPC) | Form (grants dome, applies GE) | Server Only | Cross-actor operations |
| GA_FatherExoskeleton | Father (NPC) | Form (grants dash/sprint/stealth) | Server Only | Grants to player ASC |
| GA_FatherSymbiote | Father (NPC) | Form (grants proximity, applies GE) | Server Only | Cross-actor operations |
| GA_FatherEngineer | Father (NPC) | Form (spawns turret) | Server Only | Server spawns actor |
| GA_FatherExoskeletonDash | Player | Action (player input) | Local Predicted | Player owns, input-triggered |
| GA_FatherExoskeletonSprint | Player | Hold-to-Maintain | Local Predicted | Player owns, input-triggered |
| GA_StealthField | Player | Toggle | Local Predicted | Player owns |
| GA_ProtectiveDome | Player | Toggle | Local Predicted | Player owns |
| GA_DomeBurst | Player | Instant | Local Predicted | Player owns |
| GA_FatherLaserShot | Father (NPC) | Autonomous projectile | Server Only | AI-autonomous |

Form Ability Policy Rationale:

| Issue | Wrong Policy | Correct Policy |
|-------|--------------|----------------|
| Client grants ability to player ASC | Local Predicted (fails) | Server Only (works) |
| Client applies GE to player ASC | Local Predicted (fails) | Server Only (works) |
| Ability with no client prediction key | Local Predicted (error) | Server Only (no prediction) |

Trade-off for Server Only: Visual effects may appear slightly delayed on client. Solution: Spawn VFX locally in ability, but grant/GE operations remain server-only.

### 7.5) ASC Replication Mode

| Character Type | Replication Mode | Reason |
|----------------|------------------|--------|
| Player Characters | Mixed | Full ability state for owner, minimal for others |
| AI/NPC Characters (Father) | Minimal | Reduced bandwidth, server handles all logic |
| DO NOT USE | Full | Excessive bandwidth, not needed |

NarrativeNPCCharacter default: Minimal (correct for father)

### 7.6) Deprecated Settings (UE 5.5+)

| Setting | Status | Alternative |
|---------|--------|-------------|
| Replicated properties on GA_ | Deprecated | Use AbilityTasks, GameplayCues |
| NonInstanced abilities | Deprecated | Use Instanced Per Actor |
| Replicate Input Directly | Should be OFF | Use AbilityTasks instead |
| Full ASC Replication Mode | Avoid | Use Mixed (player) or Minimal (NPC) |

### 7.7) Variable Replication Conditions

Replication Condition determines WHEN a replicated variable sends updates to clients. This is separate from Replication Type (Replicated/RepNotify/None).

#### 7.7.1) Available Replication Conditions

| Condition | Behavior | Use Case |
|-----------|----------|----------|
| None | Replicates to all clients whenever value changes | Default - most variables |
| Initial Only | Only replicates once on spawn, never updates | Values set once and never change |
| Owner Only | Only replicates to owning client | Private player data |
| Skip Owner | Replicates to all clients EXCEPT owner | Owner already has authoritative value |
| Simulated Only | Only to clients where actor is simulated proxy | Movement prediction |
| Autonomous Only | Only to autonomous proxy (controlling client) | Player-controlled actors only |
| Simulated Or Physics | Simulated actors OR physics-enabled | Physics replication |
| Initial Or Owner | Initial packet OR to owner | Hybrid use case |
| Custom | Custom logic via DOREPLIFETIME_ACTIVE_OVERRIDE | Advanced C++ only |
| Replay Or Owner | Replay system OR owner | Replay recording |
| Replay Only | Only for replay recording | Demo recording |

#### 7.7.2) BP_FatherCompanion Variable Replication Conditions

Master reference table for all replicated variables in BP_FatherCompanion:

| Variable | Replication Type | Replication Condition | Reason |
|----------|------------------|----------------------|--------|
| OwnerPlayer | Replicated | Initial Only | Set once at spawn, never changes |
| CurrentForm | RepNotify | None | All clients must see form changes |
| IsAttached | RepNotify | None | All clients must see attachment state |
| IsDeployed | Replicated | None | All clients must see turret mode |
| DeployedLocation | Replicated | None | All clients need turret position |
| TurretHealth | RepNotify | None | All clients see health UI |
| AttackTarget | Replicated | None | Target visible to all for AI coordination |

#### 7.7.3) Condition Selection Guidelines

| Scenario | Recommended Condition |
|----------|----------------------|
| NPC/Companion state (form, attachment) | None - all clients observe NPC |
| Reference set once at spawn | Initial Only - bandwidth optimization |
| Player-private inventory | Owner Only - security |
| Combat state all players see | None - shared game state |
| Local-only cosmetic data | Do not replicate |

#### 7.7.4) Why Father Uses "None" for Most Variables

The father companion is a shared NPC that all players observe. Unlike player-owned data:

| Comparison | Player Health | Father Form |
|------------|---------------|-------------|
| Who needs to see? | All players | All players |
| Owner vs others? | Owner needs full, others need minimal | Everyone needs same info |
| Recommended | Owner Only or None | None |

Father state must be consistent across all clients because:
- All players see the father following
- All players see form changes (Armor, Exoskeleton, etc.)
- All players see attachment to the owning player
- Combat effects must appear synchronized

#### 7.7.5) Initial Only for OwnerPlayer

OwnerPlayer reference is set ONCE during BeginPlay initialization and never changes during gameplay. Using Initial Only:
- Reduces network traffic (no updates sent after initial)
- Server sends value once when father spawns
- Value persists on all clients without re-sending

#### 7.7.6) Blueprint Variable Configuration Format

When creating replicated variables in Blueprints, always specify both properties:

| Step | Property | Value |
|------|----------|-------|
| 1 | Replication | Replicated OR RepNotify |
| 2 | Replication Condition | None OR Initial Only OR Owner Only |

Example for CurrentForm variable:

| Property | Value |
|----------|-------|
| Variable Type | EFatherForm (Enum) |
| Replication | RepNotify |
| Replication Condition | None |
| Instance Editable | Unchecked |
| Category | Father State |

#### 7.7.7) Ability-Local Variable Replication

Variables created within GA_ ability blueprints are NOT replicated by default. For runtime state:

| Variable Scope | Replication | Notes |
|----------------|-------------|-------|
| BP_FatherCompanion variables | YES (configure) | Persistent actor state |
| GA_ ability local variables | NO | Temporary activation state |
| GE_ effect modifications | Via GE system | Automatic GAS replication |

Ability variables (like FatherRef, PlayerRef) do NOT need replication - they exist only during ability execution on the authoritative instance.

### 7.8) Faction System for Hostile Detection

Use ArsenalStatics::GetAttitude instead of Character.Enemy tags for hostile detection.

ArsenalStatics.h (lines 80-81):

| Function | Category | Returns |
|----------|----------|---------|
| GetAttitude(TestActor, Target) | BlueprintPure, Teams | ETeamAttitude::Type |

Additional faction functions (ArsenalStatics.h):

| Function | Line | Purpose |
|----------|------|---------|
| IsSameTeam(TestActor, Target) | 77-78 | Check if actors are on same team |
| GetActorFactions(Actor) | 84-85 | Get faction tags from actor |
| AddFactionsToActor(Actor, Factions) | 88-89 | Add faction tags to actor |
| RemoveFactionsFromActor(Actor, Factions) | 92-93 | Remove faction tags from actor |

| Attitude | Value | Meaning |
|----------|-------|---------|
| Friendly | 0 | Same faction |
| Neutral | 1 | No relationship |
| Hostile | 2 | Enemy faction |

Comparison:

| Old Approach | New Approach |
|--------------|--------------|
| Has Matching Gameplay Tag (Character.Enemy) | GetAttitude(Self, Target) == Hostile |

Benefits:
- Integrates with NPCDefinition faction system
- No manual enemy tagging required
- NarrativeDamageExecCalc uses this internally
- Automatic friendly fire prevention

Faction Configuration via NPCDefinition:

| Actor | Faction Tag | Set Via |
|-------|-------------|---------|
| Father | Narrative.Factions.Heroes | NPCDef_FatherCompanion |
| Player | Narrative.Factions.Heroes | Player NPCDefinition |
| Enemies | Narrative.Factions.Bandits | Enemy NPCDefinition |

Blueprint Usage:

| Step | Action |
|------|--------|
| 1 | Get Attitude (ArsenalStatics) |
| 2 | Test Actor: Self (Father or Player) |
| 3 | Target: Potential Enemy Actor |
| 4 | Return: ETeamAttitude::Type |
| 5 | Branch: == ETeamAttitude::Hostile |
| 6 | TRUE: Valid enemy target |
| 7 | FALSE: Skip (friendly or neutral) |

Father Abilities Using Faction System:

| Ability | Detection Type |
|---------|---------------|
| GA_FatherAttack | AI Perception + Faction |
| GA_ProximityStrike | Sphere Overlap + Faction |
| GA_FatherElectricTrap | Collision + Faction |
| GA_FatherLaserShot | AI Target + Faction |
| GA_FatherExoskeletonSprint | Push Detection + Faction |

### 7.9) Faction + GoalGenerator Attack Chain

When GoalGenerator_Attack is added to ActivityConfiguration, it creates automatic hostile detection:

| Step | System | Action |
|------|--------|--------|
| 1 | AIPerception | Detects actor via sight/hearing |
| 2 | GoalGenerator_Attack | OnPerceptionUpdated fires |
| 3 | GoalGenerator_Attack | Calls DoesAttitudeMatchFilter using ArsenalStatics::GetAttitude |
| 4 | GoalGenerator_Attack | If Hostile attitude returned, calls TryAddAttackGoalFromActor |
| 5 | Goal_Attack | Created with target actor reference |
| 6 | BPA_Attack_Melee | Scores against Goal_Attack in activity evaluation |
| 7 | NPC | Attacks target automatically |

Faction Attitudes Configuration:

| Location | Path |
|----------|------|
| Project Settings | Project Settings -> Plugins -> Narrative Pro -> Faction Settings |
| Property | Faction Attitudes (map of faction tag pairs to attitude) |

Default Attitude Mapping:

| Faction A | Faction B | Attitude |
|-----------|-----------|----------|
| Narrative.Factions.Heroes | Narrative.Factions.Bandits | Hostile |
| Narrative.Factions.Heroes | Narrative.Factions.Heroes | Friendly |
| Narrative.Factions.Bandits | Narrative.Factions.Bandits | Friendly |

No Manual Configuration Required:
- NPCDefinition assigns faction via FactionTags property
- GoalGenerator_Attack handles detection automatically
- No need to manually add actors to Hostiles array for standard faction enemies

### 7.10) HandleDeath Parameters

HandleDeath delegate parameters (NarrativeCharacter.cpp:1551):

| Parameter | Type | Value | Description |
|-----------|------|-------|-------------|
| KilledActor | AActor* | Self | The actor that died (NOT the attacker) |
| KilledActorASC | UNarrativeAbilitySystemComponent* | Self ASC | The dead actor's ability system |

Common Misconception:
- KilledActor is often assumed to be the attacker
- KilledActor is actually the dead NPC itself (the owner of HandleDeath)

OnDied Broadcast (NarrativeAbilitySystemComponent.cpp:643):

| Broadcast Call | Parameter 1 | Parameter 2 |
|----------------|-------------|-------------|
| OnDied.Broadcast | GetAvatarActor() | this |

Getting the Actual Attacker:
- Attacker information is NOT passed to HandleDeath
- To get attacker, use damage event context or last damage instigator
- For death-trigger aggro transfer, use faction system instead

Blueprint Override Pattern:

| Step | Action |
|------|--------|
| 1 | Override HandleDeath in BP_FatherCompanion |
| 2 | KilledActor parameter = Self (Father) |
| 3 | Add custom death logic (despawn, respawn timer, etc.) |
| 4 | Call Parent: HandleDeath for default behavior |

### 7.11) Hostiles Array vs Faction System

The Hostiles array (NarrativeNPCCharacter.h:307) is a "hostile override" system:

| System | Primary Use | Automatic |
|--------|-------------|-----------|
| Faction System | Standard faction-based enemies | Yes |
| Hostiles Array | Override for special cases | No (manual) |

Hostiles Array Purpose (from source comment):
- "hostile override" - attack even neutral or friendly actors
- Quest scenarios: friendly NPCs temporarily hostile
- Retaliation: neutral actors who attacked us

When to Use Hostiles Array:

| Scenario | Use Faction? | Use Hostiles? |
|----------|--------------|---------------|
| Standard enemy NPCs | Yes | No |
| Quest betrayal (friendly turns hostile) | No | Yes |
| Neutral attacked player first | No | Yes |
| Temporary aggro during event | No | Yes |

Hostiles Array Access Pattern:

| Step | Blueprint Node |
|------|----------------|
| 1 | Get reference to NarrativeNPCCharacter |
| 2 | Get Hostiles (variable access, not function) |
| 3 | Add Unique (array operation) |
| 4 | Pass actor to add |

ShouldBeAggressiveTowardsTarget Override (NarrativeNPCCharacter.h:317-319):

| Property | Value |
|----------|-------|
| Function | ShouldBeAggressiveTowardsTarget |
| Category | BlueprintNativeEvent |
| Parameter | Target (AActor*) |
| Return | bool |
| Purpose | Custom aggro logic beyond faction/Hostiles |

Use Cases for Override:
- Damage threshold retaliation (attack after X damage)
- Proximity-based aggro for neutrals
- Time-based hostility windows
- Conditional aggro based on quest state

---

## SECTION 8: UE 5.6 GAMEPLAY EFFECT NAVIGATION

### 8.1) Component-Based GE Architecture

In UE 5.6, Gameplay Effects use a component-based system instead of direct property arrays.

### 8.2) Adding Components to GE

| Step | Action |
|------|--------|
| 1 | Open Gameplay Effect blueprint |
| 2 | Click Class Defaults |
| 3 | Find Components section (array) |
| 4 | Click + to add component |
| 5 | Select component type from dropdown |
| 6 | Expand component to configure |

### 8.3) Component Type Names

| Purpose | Component Name in UE 5.6 |
|---------|--------------------------|
| Grant tags to target | Grant Tags to Target Actor |
| Asset tags on effect | Tags This Effect Has (Asset Tags) |
| Tag requirements | Require Tags to Apply/Continue This Effect |
| Executions | Executions |
| Modifiers | Modifiers |

### 8.4) Grant Tags Navigation (UE 5.6)

Old Method (Pre-5.6): Direct "Granted Tags -> Added" array field

New Method (UE 5.6):

| Step | Action |
|------|--------|
| 1 | Navigate to Components section |
| 2 | Click + to add component |
| 3 | Select "Grant Tags to Target Actor" |
| 4 | Expand component |
| 5 | Expand "Add Tags" subsection |
| 6 | In "Add to Inherited" array, click + button |
| 7 | Select or enter tag value |

### 8.5) UE 5.6 Gameplay Effect Components Reference

| Component Name | Purpose |
|----------------|---------|
| Grant Tags to Target Actor | Grants gameplay tags to target |
| Tags This Effect Has (Asset Tags) | Identification tags for effect queries |
| Apply Additional Effects | Chain/trigger other GEs |
| Block Abilities with Tags | Block abilities matching tags |
| Grant Gameplay Abilities | Grant abilities while active |
| Immunity to Other Effects | Prevent specified effects |
| Remove Other Effects | Remove other GEs (mutual exclusivity) |
| Require Tags to Apply/Continue This Effect | Tag requirements |
| Chance To Apply This Effect | Probability-based application |

### 8.6) Grant Tags Component Properties

| Property | Location | Purpose |
|----------|----------|---------|
| Combined Tags | Add Tags -> Combined Tags | READ-ONLY auto-calculated result (Parent + Added - Removed) |
| Add to Inherited | Add Tags -> Add to Inherited | PRIMARY FIELD - Add tags here |
| Remove from Inherited | Add Tags -> Remove from Inherited | Override inherited parent tags |

### 8.7) NarrativeAttributeSetBase Attributes

| Attribute | Purpose |
|-----------|---------|
| NarrativeAttributeSetBase.Health | Current health |
| NarrativeAttributeSetBase.MaxHealth | Maximum health |
| NarrativeAttributeSetBase.Stamina | Current stamina |
| NarrativeAttributeSetBase.MaxStamina | Maximum stamina |
| NarrativeAttributeSetBase.StaminaRegenRate | Stamina regeneration rate |
| NarrativeAttributeSetBase.XP | Experience points |
| NarrativeAttributeSetBase.AttackRating | Attack rating |
| NarrativeAttributeSetBase.AttackDamage | Attack damage |
| NarrativeAttributeSetBase.Armor | Armor/defense value |
| NarrativeAttributeSetBase.StealthRating | Stealth value |
| NarrativeAttributeSetBase.Heal | Heal meta-attribute |
| NarrativeAttributeSetBase.Damage | Damage meta-attribute |
| AbilitySystemComponent.OutgoingDuration | Outgoing effect duration modifier |
| AbilitySystemComponent.IncomingDuration | Incoming effect duration modifier |

MovementSpeed is NOT an attribute - modify via CharacterMovement component directly.

### 8.8) Modifier Operations

| Modifier Op | Description |
|-------------|-------------|
| Add (Base) | Adds to base value before multipliers |
| Multiply (Additive) | Additive multiplier (stacks additively) |
| Divide (Additive) | Additive division |
| Multiply (Compound) | Compound multiplier (multiplies with others) |
| Add (Final) | Adds after all multipliers applied |
| Override | Completely replaces the attribute value |

### 8.9) Magnitude Calculation Types

| Type | Description |
|------|-------------|
| Scalable Float | Fixed float value (can use curve tables) |
| Attribute Based | Value derived from another attribute |
| Custom Calculation Class | Custom C++ calculation class |
| Set by Caller | Value passed at runtime via tag |

### 8.10) Narrative Pro Execution Calculations

| Calculation Class | Purpose |
|-------------------|---------|
| NarrativeDamageExecCalc | Built-in damage calculation |
| NarrativeHealExecution | Built-in heal calculation |

---

## SECTION 9: BLUEPRINT IMPLEMENTATION PATTERNS

### PATTERN 1: Dynamic Damage Calculation

Override GetAttackDamage() to calculate damage from attributes:

| Step | Node |
|------|------|
| 1 | Event: Get Attack Damage (Override from NarrativeCombatAbility) |
| 2 | Get Avatar Actor From Actor Info |
| 3 | Cast To BP_FatherCompanion |
| 4 | Get Ability System Component |
| 5 | Get Numeric Attribute Base (NarrativeAttributeSetBase.AttackDamage) |
| 6 | Return attribute value |

### PATTERN 2: Animation-Driven Timing

| Node | Purpose |
|------|---------|
| Play Montage and Wait (Async Node) | Start animation |
| On Notify Begin | Execute gameplay logic (damage, spawn, effects) |
| On Completed | Apply cooldown, end ability |
| On Interrupted | Cancel pending effects, end ability (cancelled) |

AnimNotify naming convention: [AbilityName][Action]
Examples: FatherAttackDamage, ProximityStrikeDamage, DashImpact

### PATTERN 3: Tag-Based Activation Control

| Tag Type | Purpose | Example |
|----------|---------|---------|
| Ability Tags | Unique identifier | Ability.Father.Crawler.Attack |
| Activation Required Tags | Must have to activate | Effect.Father.FormState.Crawler |
| Activation Blocked Tags | Cannot activate if present | Father.State.Attached |
| Cancel Abilities with Tag | Cancel on activation | Ability.Father.Armor |
| Cooldown Gameplay Effect Class | GAS built-in cooldown | GE_FormChangeCooldown |
| Activation Owned Tags | Granted while active | Father.Form.Exoskeleton |

### PATTERN 4: NarrativeDamageExecCalc Integration

| Step | Action |
|------|--------|
| 1 | Create GE_YourDamage |
| 2 | Duration Policy: Instant |
| 3 | Components -> Executions: NarrativeDamageExecCalc |
| 4 | Components -> Asset Tags: Effect.Damage |
| 5 | In ability: Make Outgoing Spec (GE_YourDamage) |
| 6 | Apply Gameplay Effect Spec to Target |

### PATTERN 5: Timer-Based Periodic Checking

| Event | Action |
|-------|--------|
| Event ActivateAbility | Set Timer by Function Name ("PeriodicCheck", 0.5, Looping: TRUE) |
| Function PeriodicCheck | Check conditions, perform detection, apply effects |
| Event End Ability | Clear Timer by Function Name |

### PATTERN 6: Trace-Based Hit Detection

Requires parent class: NarrativeCombatAbility

| Property | Value |
|----------|-------|
| Trace Distance | 300.0 |
| Trace Radius | 50.0 |
| bTrace Multi | FALSE |

Execute:
1. Generate Target Data Using Trace
2. Override HandleTargetData to process hits

### PATTERN 7: Cooldown System (Built-in)

GAS built-in cooldown system (recommended for form abilities):

| Step | Configuration |
|------|---------------|
| 1 | Create GE_YourAbility_Cooldown |
| 2 | Duration Policy: Has Duration |
| 3 | Duration Magnitude: CooldownDuration |
| 4 | Components -> Grant Tags to Target Actor -> Add to Inherited: Cooldown.Father.YourAbility |
| 5 | Ability Class Defaults -> Cooldown Gameplay Effect Class: GE_YourAbility_Cooldown |
| 6 | Blueprint: Call CommitAbilityCooldown node after ability completes |

Benefits of built-in cooldown:
- GAS automatically blocks activation when cooldown tag present
- GetCooldownTimeRemaining() available for UI
- No manual Block Abilities with Tag configuration needed

Legacy Pattern (manual):

| Step | Configuration |
|------|---------------|
| 1 | Ability Class Defaults -> Block Abilities with Tag: Cooldown.Father.YourAbility |
| 2 | Blueprint: Apply Gameplay Effect to Self (GE_YourAbility_Cooldown) |

### PATTERN 8: Form Restriction System

| Ability | Activation Required Tags | Result |
|---------|--------------------------|--------|
| GA_FatherElectricTrap | Effect.Father.FormState.Engineer | Only activates in Engineer form |
| GA_ExoskeletonDash | Effect.Father.FormState.Exoskeleton | Only activates in Exoskeleton form |
| GA_ProtectiveDome | Effect.Father.FormState.Armor | Only activates in Armor form |

### PATTERN 9: Movement Speed Modification

| Step | Action |
|------|--------|
| 1 | Get Avatar Actor |
| 2 | Get Character Movement Component |
| 3 | Store Original Speed (variable) |
| 4 | Set Max Walk Speed (NewSpeed = Original * Multiplier) |
| 5 | On End Ability: Restore Original Speed (lerp over 0.3-0.5s) |

### PATTERN 10: Invulnerability During Action

| Step | Configuration |
|------|---------------|
| 1 | GE_Invulnerability: Duration Policy: Infinite |
| 2 | Components -> Grant Tags: Narrative.State.Invulnerable |
| 3 | On Activation: Apply GE_Invulnerability |
| 4 | On End: Remove GE_Invulnerability |

### PATTERN 11: AoE Sphere Overlap

| Step | Action |
|------|--------|
| 1 | Get Actor Location (center point) |
| 2 | Sphere Overlap Actors |
| 3 | Sphere Pos: Actor Location |
| 4 | Sphere Radius: AoERadius variable |
| 5 | Object Types: Pawn |
| 6 | Actor Class Filter: NarrativeCharacter |
| 7 | For each actor: Validate and apply effect |

### PATTERN 12: Projectile Spawn

| Step | Action |
|------|--------|
| 1 | Get Mesh -> Get Socket Transform (MuzzleSocket) |
| 2 | Spawn Actor from Class (BP_FatherProjectile) |
| 3 | Projectile handles its own collision and damage |

Projectile Actor:
- Sphere Collision (OverlapAllDynamic)
- Projectile Movement Component
- On Overlap: Apply damage GE

### PATTERN 13: Delegate-Based Attribute Monitoring

More efficient than timer-based polling for health/attribute monitoring.

| Approach | Method | Performance |
|----------|--------|-------------|
| Timer Polling | Check attribute every 0.5s | Higher CPU, potential miss |
| Delegate | OnAttributeChangeDelegate | Event-driven, immediate |

Setup (in ActivateAbility or BeginPlay):

| Step | Action |
|------|--------|
| 1 | Get Ability System Component (target) |
| 2 | Get Gameplay Attribute Value Change Delegate |
| 3 | Attribute: NarrativeAttributeSetBase.Health |
| 4 | Add Dynamic (bind to custom function) |
| 5 | Function: OnHealthChanged |

OnHealthChanged Function:
- Parameters: FOnAttributeChangeData (contains OldValue, NewValue)
- Calculate threshold check
- Execute ability logic when threshold met

Cleanup (in EndAbility):
- Get Gameplay Attribute Value Change Delegate
- Remove Dynamic (unbind function)

Use Cases:

| Ability | Monitors | Threshold |
|---------|----------|-----------|
| GA_FatherSacrifice | Player Health | Below 15% |
| GA_ProtectiveDome | Dome Energy | Reaches 0 or Max |

---

## SECTION 10: TAG SYSTEM REFERENCE

### 10.1) Tag Hierarchy Structure

Ability Tags:

| Parent | Children |
|--------|----------|
| Ability.Father | Ability.Father.Attack, Ability.Father.Crawler, Ability.Father.Armor, Ability.Father.Exoskeleton, Ability.Father.Symbiote, Ability.Father.Engineer |

Form Tags (5 forms):

| Parent | Children |
|--------|----------|
| Father.Form | Father.Form.Crawler, Father.Form.Armor, Father.Form.Exoskeleton, Father.Form.Symbiote, Father.Form.Engineer |

State Tags:

| Parent | Children |
|--------|----------|
| Father.State | Father.State.Attacking, Father.State.Dashing, Father.State.Attached, Father.State.Detached, Father.State.Stealthed, Father.State.TurretActive, Father.State.Deployed, Father.State.Alive, Father.State.Transitioning, Father.State.SymbioteLocked |

State Tags (v4.1 Additions):

| Tag | Purpose |
|-----|---------|
| Father.State.Alive | Required for form activation (applied at spawn, default true) |
| Father.State.Dormant | Blocks form activation after GA_FatherSacrifice |
| Father.State.Transitioning | Blocks form activation during 5s VFX transition |
| Father.State.SymbioteLocked | Blocks form activation during 30s Symbiote duration |

Cooldown Tags:

| Parent | Children |
|--------|----------|
| Cooldown.Father | Cooldown.Father.Attack, Cooldown.Father.FormChange, Cooldown.Father.Dash, Cooldown.Father.ProximityStrike, Cooldown.Father.ElectricTrap, Cooldown.Father.LaserShot, Cooldown.Father.StealthField |

Built-in Narrative Pro States:

| Parent | Children |
|--------|----------|
| Narrative.State | Narrative.State.IsDead, Narrative.State.Stunned, Narrative.State.Invulnerable |

### 10.2) Built-In Narrative Pro Tags

| Tag | Purpose | Used By Father |
|-----|---------|----------------|
| Narrative.State.Invulnerable | Blocks all damage | YES |
| State.InvisibleToEnemies | AI ignores target | YES |
| State.Weapon.Blocking | Blocking state | NO |
| Narrative.State.Movement.Lock | Prevents all movement (immobilize) | YES (Electric Trap) |
| Narrative.State.IsDead | Death state | YES |

### 10.3) Auto-Managed Movement State Tags

File: NarrativeCharacter.cpp (lines 105-161)

Narrative Pro automatically manages movement state tags via OnMovementModeChanged:

| Tag | Auto-Added When | Auto-Removed When |
|-----|-----------------|-------------------|
| State.Movement.Walking | Character is walking | Character stops walking |
| State.Movement.Falling | Character starts falling | Character lands |
| State.Movement.Swimming | Character enters water | Character exits water |
| State.Movement.Climbing | Character starts climbing | Character stops climbing |
| State.Movement.Ragdoll | Character enters ragdoll | Character exits ragdoll |

No manual tag management needed. Query tags directly in abilities:

| Ability | Check |
|---------|-------|
| GA_FatherExoskeletonDash | Has Matching Gameplay Tag: State.Movement.Falling |
| If TRUE | Block activation (no mid-air dash) |
| If FALSE | Allow activation |

### 10.4) Gameplay Event Tags

| Tag | Purpose |
|-----|---------|
| GameplayEvent.Interact | Initiate interact ability |
| GameplayEvent.KilledEnemy | Fires when killing enemy |
| GameplayEvent.Death | Fires on death (server only) |
| GameplayEvent.ToggleWield.On | Wield weapon |
| GameplayEvent.ToggleWield.Off | Unwield weapon |
| GameplayEvent.AttackApex | Attack montage damage point |
| GameplayEvent.EndAttack | Attack finished |

---

## SECTION 11: ANTI-PATTERNS TO AVOID

### 11.1) Hardcoded Damage Values

| Status | Pattern |
|--------|---------|
| WRONG | Apply Damage: 25.0 (hardcoded) |
| CORRECT | Apply Damage: BaseDamage variable |

### 11.2) Using Tick for Periodic Checks

| Status | Pattern |
|--------|---------|
| WRONG | Event Tick: Check for enemies (every frame) |
| CORRECT | Timer (0.5s): Check for enemies (twice per second) |

### 11.3) Direct Health Modification

| Status | Pattern |
|--------|---------|
| WRONG | Set Attribute (Health): Health - Damage (bypasses armor) |
| CORRECT | Apply Gameplay Effect with NarrativeDamageExecCalc |

### 11.4) Collision Components for Character Combat

Context matters for collision vs trace decision:

| Context | Approach | Reason |
|---------|----------|--------|
| Melee attack on character | WRONG: Collision component on character | No hit location/normal data, poor GAS integration |
| Melee attack on character | CORRECT: GenerateTargetDataUsingTrace | Provides hit data, proper GAS flow |
| Projectile actor | CORRECT: Collision component on projectile | Projectile detects its own collisions |
| Placed trap/hazard actor | CORRECT: Collision component on trap | Trap detects enemies entering |
| AoE instant detection | CORRECT: Sphere Overlap Actors function | One-time detection, not persistent component |

### 11.5) Ignoring Multiplayer Authority

| Status | Pattern |
|--------|---------|
| WRONG | BeginPlay: Grant Ability (runs on all clients) |
| CORRECT | BeginPlay -> Has Authority? -> TRUE: Grant Ability (server only) |

### 11.6) Using Apply Gameplay Effect to Target on Actor

| Status | Pattern |
|--------|---------|
| WRONG | Apply Gameplay Effect to Target (expects ASC, receives Actor) |
| CORRECT | Get Ability System Component from target -> Apply Gameplay Effect to Self on ASC |

### 11.7) Manual ASC Creation

| Status | Pattern |
|--------|---------|
| WRONG | Manual ASC creation in Blueprint BeginPlay |
| CORRECT | Inherit from NarrativeNPCCharacter (auto-creates ASC in C++) |

### 11.8) Not Using bActivateAbilityOnGranted

| Status | Pattern |
|--------|---------|
| WRONG | Complex timer systems to start autonomous abilities |
| CORRECT | Set bActivateAbilityOnGranted = true in Class Defaults |

### 11.9) Form Ability Ending Patterns (Locked - January 2026 Audit)

> **LOCKED RULES:** The following EndAbility lifecycle rules were validated during Claude-GPT dual-agent audit (January 2026). See `ClaudeContext/Handoffs/Father_Companion_GAS_Abilities_Audit.md` for full details.

**RULE 1 — Instant Abilities** (GA_FatherAttack, GA_DomeBurst, GA_ProximityStrike, GA_TurretShoot, GA_FatherLaserShot, GA_FatherElectricTrap, GA_Backstab)
- Call `K2_EndAbility` at end of activation flow
- `Event_EndAbility` NOT required
- Pattern: `Event_Activate → Logic → CommitCooldown → K2_EndAbility`

**RULE 2 — Abilities with Delay/Timer** (Form abilities, GA_FatherExoskeletonDash, GA_StealthField)
- MUST have `Event_EndAbility` with `bWasCancelled` check
- MUST clean up persistent state (timers, GE handles)
- MUST prevent post-delay execution after cancellation
- **Preferred:** Use `AbilityTaskWaitDelay` (auto-cancels on EndAbility)
- **Fallback:** Explicit 3-layer guards after Delay node

**RULE 3 — Toggle/Persistent Abilities** (GA_ProtectiveDome, GA_FatherExoskeletonSprint, GA_FatherRifle, GA_FatherSword)
- Do NOT call `K2_EndAbility` on activation
- MUST have `Event_EndAbility` for cleanup
- Stay active until cancelled externally

**RULE 4 — First Activation Path** (Form abilities with `bIsFirstActivation`)
- True path may skip transition-only presentation (VFX, delay)
- True path MUST merge into same stateful setup chain as False path
- True path MUST NOT terminate early at separate EndAbility

**VTF-7 — CommitCooldown Requirement**
- If ability defines `CooldownGameplayEffectClass`, at least one activation path MUST call `CommitAbility()` or `CommitAbilityCooldown()`
- Otherwise cooldown GE won't be applied and tags for ActivationBlocked won't be granted

RECOMMENDED PATTERN: End Ability After Setup Complete

| Step | Action |
|------|--------|
| 1 | ActivateAbility |
| 2 | Activation Owned Tags grant form tag (Father.Form.*) |
| 3 | Apply stat effects (GE_ArmorBoost, etc.) |
| 4 | Set CurrentForm variable |
| 5 | CommitAbilityCooldown (applies form change cooldown) |
| 6 | Ability ends naturally (no explicit End Ability node needed) |

Why this pattern works (v4.0 architecture):
1. Activation Owned Tags REPLICATE (ReplicateActivationOwnedTags enabled)
2. Cancel Abilities With Tag handles form mutual exclusivity
3. Ability can be re-activated (not blocked while running)
4. Clean lifecycle: activate -> apply -> commit cooldown

CRITICAL: EndAbility Event Must Check bWasCancelled

The K2_OnEndAbility event fires in TWO scenarios:
1. **bWasCancelled = false**: Ability ended normally after setup
2. **bWasCancelled = true**: Ability cancelled by another form's Cancel Abilities With Tag

EndAbility cleanup logic (restore speed, remove GEs, detach) must ONLY run when bWasCancelled = true:

| Event EndAbility Flow | Action |
|----------------------|--------|
| 1 | Event K2_OnEndAbility fires |
| 2 | Branch on bWasCancelled output pin |
| 3 | FALSE path → Skip cleanup (ability just finished setup, form is now active) |
| 4 | TRUE path → Do cleanup (form switch in progress, restore player state) |

WRONG PATTERN (causes immediate cleanup after setup):
```
Event EndAbility → Restore Speed → Remove GEs
```

CORRECT PATTERN (cleanup only on form switch):
```
Event EndAbility → Branch (bWasCancelled?)
                      │
                      ├── FALSE → (do nothing, skip cleanup)
                      │
                      └── TRUE → Restore Speed → Remove GEs → Detach
```

ALTERNATIVE PATTERN: Keep Ability Running (Input-Held States Only)

Only use for states tied to input hold duration (sprint-while-holding-Shift).

Limitations of keeping ability running:
- Ability cannot be re-triggered
- Consumes resources (AbilityTasks ticking)
- Risk of phantom tasks on unexpected termination

### 11.10) Reinventing Character/Controller Getters

| Status | Pattern |
|--------|---------|
| WRONG | Custom function to find player controller |
| CORRECT | Use GetOwningNarrativeCharacter() (returns CharacterOwner) |
| CORRECT | Use GetOwningController() (returns controller) |

Built-in functions handle all edge cases.

### 11.11) Assuming Activation Required Tags Are Continuously Checked

Activation Required Tags only checked at activation time, NOT during execution.

WRONG ASSUMPTION:

| Assumption | Reality |
|------------|---------|
| GA_ProximityStrike will auto-end when Symbiote form ends | Ability continues running even after tags removed |

ACTUAL BEHAVIOR:

| Step | Event | GA_ProximityStrike Status |
|------|-------|---------------------------|
| 1 | Symbiote form active | Running (timer dealing damage) |
| 2 | GA_FatherCrawler activates | Form switch initiated |
| 3 | Cancel Abilities with Tag: Ability.Father.Symbiote | GA_FatherSymbiote cancelled |
| 4 | Father.Form.Symbiote tag removed | Tag gone from father |
| 5 | GA_ProximityStrike | STILL RUNNING - tags not re-checked |
| 6 | Timer fires | Damage dealt in wrong form (BUG) |

SOLUTION: Use hierarchical child tags (see 11.12).

### 11.12) Flat Tag Structure for Form-Specific Abilities

WRONG (form-specific abilities orphaned from cancel system):

| Tag | Problem |
|-----|---------|
| Ability.Father.Symbiote | Form tag |
| Ability.Father.ProximityStrike | Separate - not cancelled when form ends |

CORRECT (hierarchical tags enable automatic cancellation):

| Tag | Relationship |
|-----|--------------|
| Ability.Father.Symbiote | Form (parent) |
| Ability.Father.Symbiote.ProximityStrike | Child - cancelled with parent |

GAS Cancel Abilities with Tag uses hierarchical matching - parent tag cancels all children.

| Form Ability | Cancel Tag | Abilities Cancelled |
|--------------|------------|---------------------|
| GA_FatherCrawler | Ability.Father.Symbiote | GA_FatherSymbiote AND GA_ProximityStrike |

HIERARCHICAL TAG PATTERN FOR FORM-SPECIFIC ABILITIES:

| Form | Form Ability Tag | Form-Specific Ability Tags |
|------|------------------|---------------------------|
| Crawler | Ability.Father.Crawler | Ability.Father.Crawler.Attack, Ability.Father.Crawler.LaserShot |
| Armor | Ability.Father.Armor | Ability.Father.Armor.ProtectiveDome, Ability.Father.Armor.DomeBurst |
| Exoskeleton | Ability.Father.Exoskeleton | Ability.Father.Exoskeleton.Dash, Ability.Father.Exoskeleton.Sprint, Ability.Father.Exoskeleton.StealthField |
| Symbiote | Ability.Father.Symbiote | Ability.Father.Symbiote.ProximityStrike |
| Engineer | Ability.Father.Engineer | Ability.Father.Engineer.TurretShoot, Ability.Father.Engineer.ElectricTrap |

Multi-Form Abilities (Keep Flat - No Hierarchical Parent):

| Ability | Tag | Reason |
|---------|-----|--------|
| GA_FatherMark | Ability.Father.Mark | Works in Crawler AND Engineer, transient (ends immediately) |

Benefits:
- No changes to form Cancel Abilities with Tag lists
- Automatic blanket cancel via parent tag matching
- Scales to future form-specific abilities
- Clean hierarchical organization

---

## SECTION 12: PATTERN SELECTION GUIDE

### 12.1) Parent Class Decision

| If Your Ability... | Use Parent Class |
|-------------------|------------------|
| Is melee with socket-based traces | GA_Melee_Unarmed |
| Needs inherited warp-toward-enemy | GA_Melee_Unarmed |
| Needs custom trace logic | NarrativeCombatAbility |
| Is a hitscan weapon | NarrativeCombatAbility |
| Uses GenerateTargetDataUsingTrace | NarrativeCombatAbility |
| Fires projectiles | NarrativeGameplayAbility |
| Is area effect (Sphere Overlap Actors) | NarrativeGameplayAbility |
| Is buff/debuff | NarrativeGameplayAbility |
| Is movement ability | NarrativeGameplayAbility |
| Is utility ability | NarrativeGameplayAbility |
| Uses timer-based periodic checks | Either (depends on other factors) |

### 12.1.1) GA_Melee_Unarmed Inherited Features

GA_Melee_Unarmed provides complete melee combat implementation:

| Feature | Included |
|---------|----------|
| Socket-based sphere tracing | YES |
| Animation event integration (AnimNotify) | YES |
| Warp-toward-enemy targeting | YES |
| Combo system (light/heavy) | YES |
| GE_WeaponDamage integration | YES |
| Blocking tags configured | YES |

Properties to override in child Blueprint:

| Property | Purpose |
|----------|---------|
| Attack Right Socket | Socket name for right attack origin |
| Attack Left Socket | Socket name for left attack origin |
| Attack Combo Anim Set | Animation set tag for attacks |
| Damage Effect Class | GE to apply on hit |
| Default Attack Damage | Base damage value |
| Input Tag | Activation input |
| Should Warp | Enable/disable warp system |

Father Example (GA_FatherAttack):

| Property | Value |
|----------|-------|
| Attack Right Socket | father_right |
| Attack Left Socket | father_left |
| Damage Effect Class | GE_WeaponDamage |
| Default Attack Damage | 10.0 |
| Should Warp | TRUE |

### 12.2) Pattern Selection Matrix

Pattern numbers reference Section 9 (Blueprint Implementation Patterns).

| Ability Type | Use Patterns | Parent Class |
|--------------|--------------|--------------|
| Melee Attack | 1, 2, 3, 4, 5, 6, 8 | NarrativeCombatAbility |
| Ranged Hitscan | 1, 2, 3, 4, 5, 6, 8 | NarrativeCombatAbility |
| Projectile Attack | 2, 3, 5, 6, 12 | NarrativeGameplayAbility |
| AoE Damage | 1, 3, 4, 5, 6, 11 | NarrativeGameplayAbility |
| Passive Detection | 3, 5, 6, 7 | Either |
| Buff/Debuff | 3, 5, 6 | NarrativeGameplayAbility |
| Movement | 3, 5, 6, 9 | NarrativeGameplayAbility |
| Defensive | 3, 5, 6, 10 | NarrativeGameplayAbility |
| Chain Attack | 1, 3, 4, 5, 6 | NarrativeGameplayAbility |

Pattern Key:

| # | Pattern Name | Purpose |
|---|--------------|---------|
| 1 | Dynamic Damage Calculation | GetAttackDamage override |
| 2 | Animation Timing | AnimNotify sync |
| 3 | Tag-Based Activation | Form/state restrictions |
| 4 | NarrativeDamageExecCalc | Proper damage application |
| 5 | Multiplayer Configuration | Net Execution, replication |
| 6 | Designer-Friendly Variables | Instance Editable properties |
| 7 | Timer-Based Checks | Periodic detection |
| 8 | Trace-Based Combat | GenerateTargetDataUsingTrace |
| 9 | Movement Speed Modification | CharacterMovement direct |
| 10 | Invulnerability | Narrative.State.Invulnerable tag |
| 11 | AoE Sphere Overlap | Sphere Overlap Actors |
| 12 | Projectile Spawn | Projectile actor pattern |

### 12.3) Base Class Summary

| Blueprint | Parent Class | Reason |
|-----------|--------------|--------|
| BP_FatherCompanion | NarrativeNPCCharacter | Has ASC, AI, inventory auto-created |
| All Father Abilities | NarrativeGameplayAbility | Tag-based input support |
| Combat Abilities | NarrativeCombatAbility | Trace-based hit detection |

---

## SECTION 13: SAVE SYSTEM

### 13.1) INarrativeSavableActor Interface

NarrativeNPCCharacter implements INarrativeSavableActor interface automatically.

| Function | Purpose | When Called |
|----------|---------|-------------|
| PrepareForSave() | Populate save data before serialization | Before game saves |
| Load() | Restore state from save data | After game loads |
| SetActorGUID() | Assign unique identifier | On spawn |
| GetActorGUID() | Retrieve unique identifier | During save/load |

### 13.2) SaveGame Variable Marking

Variables marked with UPROPERTY(SaveGame) in C++ are automatically serialized.

Blueprint variables require manual SaveGame checkbox:

| Step | Action |
|------|--------|
| 1 | Select variable in My Blueprint panel |
| 2 | In Details panel, expand Advanced section |
| 3 | Enable SaveGame checkbox |

### 13.3) NarrativeNPCCharacter Auto-Saved Properties

| Variable | Type | SaveGame | Source |
|----------|------|----------|--------|
| AICRecord | FNarrativeActorRecord | YES | AI Controller state |
| OwningSettlement | FGameplayTag | YES | Settlement reference |
| SpawnInfo | FNPCSpawnInfo | YES | Spawn data |
| NPCLevel | int32 | YES | NPC level |
| NPCFactions | FGameplayTagContainer | YES | Faction tags |

### 13.4) NarrativeAttributeSetBase Auto-Saved Attributes

Attributes with meta=(NarrativeSaveAttribute) are automatically saved:

| Attribute | NarrativeSaveAttribute | Auto-Saved |
|-----------|------------------------|------------|
| Health | YES | YES |
| MaxHealth | YES | YES |
| Stamina | YES | YES |
| MaxStamina | YES | YES |
| StaminaRegenRate | YES | YES |
| XP | YES | YES |
| Armor | NO | NO |
| AttackRating | NO | NO |
| AttackDamage | NO | NO |
| StealthRating | NO | NO |

### 13.5) What Is NOT Auto-Saved

| Item | Reason | Solution |
|------|--------|----------|
| Blueprint variables | Not marked SaveGame in C++ | Enable SaveGame in Blueprint |
| Active Gameplay Effects | Runtime state only | Reapply via ability activation |
| Activation Owned Tags | Removed when ability ends | Reactivate ability on load |
| CurrentForm (father) | Blueprint variable | Mark SaveGame, override Load() |
| IsAttached (father) | Blueprint variable | Mark SaveGame, override Load() |
| IsDeployed (father) | Blueprint variable | Mark SaveGame, restore turret mode |
| DeployedTransform (father) | Blueprint variable | Mark SaveGame, restore turret position |

### 13.6) Father SaveGame Variables Summary

| Variable | SaveGame | Purpose |
|----------|----------|---------|
| CurrentForm | YES | Restore active form on load |
| IsAttached | YES | Restore attachment state (Armor/Exoskeleton) |
| IsDeployed | YES | Restore Engineer turret deployment state |
| DeployedTransform | YES | Restore turret world position and rotation |
| OwnerPlayerDefinition | NO | Used for lookup only, not persistence |
| Effect Handles (all) | NO | Recreated when abilities activate |
| Ability Handles (all) | NO | Recreated when abilities grant |

### 13.8) Load_Implementation Base Behavior

NarrativeNPCCharacter.cpp lines 436-448:

| Step | Action |
|------|--------|
| 1 | Load_Implementation() |
| 2 | Check if Controller exists |
| 3 | If not: SpawnDefaultController() |
| 4 | Get NarrativeSaveSubsystem |
| 5 | LoadActorFromRecord(Controller, AICRecord) |

Base Load() only restores AI Controller. Custom state requires override.

### 13.9) Father Form Persistence Pattern

BP_FatherCompanion requires Load() override for form restoration:

| Step | Action |
|------|--------|
| 1 | Event Load (override) |
| 2 | Parent: Load (restores AI Controller) |
| 3 | Switch on CurrentForm (now restored from SaveGame) |
| 4 | Crawler: Do nothing (default state) |
| 5 | Armor: TryActivateAbilityByClass(GA_FatherArmor) |
| 6 | Exoskeleton: TryActivateAbilityByClass(GA_FatherExoskeleton) |
| 7 | Symbiote: TryActivateAbilityByClass(GA_FatherSymbiote) |
| 8 | Engineer: TryActivateAbilityByClass(GA_FatherEngineer) |

Cancel Abilities with Tag ensures GA_FatherCrawler cancelled when saved form activates.

### 13.10) Save System Reference

For complete implementation steps, see: BP_FatherCompanion_SaveSystem_Integration_Guide_v1_0.md

---

## SECTION 14: GAMEPLAY EFFECT REMOVAL PATTERNS

### 14.1) v4.0 Architecture - EndAbility Pattern

Form abilities now handle their own cleanup in EndAbility instead of centralized GA_FatherDetach.

| Pattern | Method | When Used |
|---------|--------|-----------|
| EndAbility Cleanup | Each form ability restores state on end | All form abilities |
| Handle-Based | Remove Active Gameplay Effect | Duration abilities with precise control |
| Source-Based | Remove Active Gameplay Effects With Source Tags | Blanket removal (legacy pattern) |

### 14.2) Form Ability EndAbility Pattern

Each form ability that modifies player stats must restore them on end:

| Step | Action |
|------|--------|
| 1 | Event End Ability (override) |
| 2 | Get stored OriginalMaxWalkSpeed variable |
| 3 | Get OwnerPlayer -> Get Character Movement |
| 4 | Timeline lerp (0.3-0.5 seconds) |
| 5 | Set Max Walk Speed from lerp output |
| 6 | Remove any stat GEs applied by this ability |
| 7 | Parent: End Ability |

### 14.3) Why EndAbility Pattern (v4.0)

| Reason | Explanation |
|--------|-------------|
| Direct form-to-form | No intermediate Crawler state |
| Cancel Abilities With Tag | Old form EndAbility fires when new form activates |
| Clean state transfer | Old form restores base, new form applies boost |
| No centralized cleanup | Each ability responsible for own state |

### 14.4) Form Transition Speed Restoration Flow

Example: Armor (slow) to Exoskeleton (fast)

| Step | Actor | Action |
|------|-------|--------|
| 1 | Player | Selects Exoskeleton from form wheel |
| 2 | GA_FatherExoskeleton | Activates |
| 3 | Cancel Abilities With Tag | Cancels GA_FatherArmor |
| 4 | GA_FatherArmor EndAbility | Fires (bWasCancelled = true) |
| 5 | GA_FatherArmor | Lerp restores speed to stored original |
| 6 | GA_FatherExoskeleton | Stores current speed (now base) |
| 7 | GA_FatherExoskeleton | Applies +50% speed boost |
| 8 | Result | Smooth transition from slow to fast |

### 14.5) Handle-Based Removal (Duration Abilities)

Duration abilities still use handle-based removal for precise control:

| Ability | Variable | Purpose |
|---------|----------|---------|
| GA_StealthField | PlayerInvisibilityHandle | Remove stealth on end |
| GA_StealthField | FatherInvisibilityHandle | Remove stealth on end |
| GA_StealthField | DamageBonusHandle | Remove damage bonus |
| GA_ProtectiveDome | DomeEffectHandle | Remove dome on end |
| GA_FatherSacrifice | OfflineEffectHandle | Remove offline state |

### 14.6) Effect Handle Storage Location

| Scenario | Storage Location | Reason |
|----------|------------------|--------|
| Effect applied and removed by same ability | Local ability variable | Single ability manages lifecycle |
| Effect applied by one ability, removed by another | BP_FatherCompanion variable | Cross-ability access required |

### 14.7) Anti-Pattern: Missing EndAbility Cleanup

WRONG (no cleanup):

| Problem | Form ability ends without restoring player speed |
|---------|--------------------------------------------------|
| Result | Player stuck at modified speed permanently |

CORRECT (proper cleanup):

| Step | Action |
|------|--------|
| 1 | Event End Ability fires |
| 2 | Restore OriginalMaxWalkSpeed |
| 3 | Remove applied GEs |
| 4 | Call Parent: End Ability |

### 14.8) Per-Form Handle Tracking Pattern

For multi-form companions, track both effect handles and ability handles:

Effect Handles (FActiveGameplayEffectHandle) - for GEs applied:

| Variable | Type | Purpose |
|----------|------|---------|
| CrawlerStateHandle | ActiveGameplayEffectHandle | GE_CrawlerState removal |
| ArmorStateHandle | ActiveGameplayEffectHandle | GE_ArmorState removal |
| ArmorBoostHandle | ActiveGameplayEffectHandle | GE_ArmorBoost removal (on player) |
| ExoskeletonStateHandle | ActiveGameplayEffectHandle | GE_ExoskeletonState removal |
| ExoskeletonSpeedHandle | ActiveGameplayEffectHandle | GE_ExoskeletonSpeed removal (on player) |
| SymbioteStateHandle | ActiveGameplayEffectHandle | GE_SymbioteState removal |
| SymbioteBoostHandle | ActiveGameplayEffectHandle | GE_SymbioteBoost removal (on player) |
| EngineerStateHandle | ActiveGameplayEffectHandle | GE_EngineerState removal |
| FormCooldownHandle | ActiveGameplayEffectHandle | GE_FormChangeCooldown removal |

Ability Handles (FGameplayAbilitySpecHandle) - for abilities granted to player:

| Variable | Type | Purpose |
|----------|------|---------|
| DomeAbilityHandle | GameplayAbilitySpecHandle | GA_ProtectiveDome removal (on player) |
| DomeBurstAbilityHandle | GameplayAbilitySpecHandle | GA_DomeBurst removal (on player) |
| DashAbilityHandle | GameplayAbilitySpecHandle | GA_FatherExoskeletonDash removal (on player) |
| SprintAbilityHandle | GameplayAbilitySpecHandle | GA_FatherExoskeletonSprint removal (on player) |
| StealthAbilityHandle | GameplayAbilitySpecHandle | GA_StealthField removal (on player) |
| ProximityAbilityHandle | GameplayAbilitySpecHandle | GA_ProximityStrike removal (on player) |

Note: BackstabAbilityHandle removed in v5.0 - GA_Backstab is now a Player Default Ability.

Critical: Cancel Abilities with Tag does NOT work cross-actor. Must use ClearAbility(Handle) on player ASC. |

### 14.9) Cross-Actor Effect Application

When father applies effects to player:

| Step | Action |
|------|--------|
| 1 | Get OwnerPlayer reference |
| 2 | Get OwnerPlayer -> Ability System Component |
| 3 | Make Outgoing Spec (GE class) |
| 4 | Apply Gameplay Effect Spec to Target (player ASC) |
| 5 | Store returned handle in father variable |
| 6 | On EndAbility: Remove using stored handle on player ASC |

### 14.10) Cross-Actor Ability Cleanup Pattern

When father form changes or father dies, cleanup abilities granted to player using two-step pattern:

| Step | Action |
|------|--------|
| 1 | Is Valid (OwnerPlayer) check |
| 2 | Get OwnerPlayer -> Ability System Component |
| 3 | For each stored FGameplayAbilitySpecHandle: |
| 4 | Call Cancel Ability (Handle) on Player ASC |
| 5 | Call Set Remove Ability On End (Handle) on Player ASC |
| 6 | Clear handle variable |

Two-step pattern is required because:

| Issue | Explanation |
|-------|-------------|
| Cancel Abilities with Tag | Only works on same ASC, not cross-actor |
| ClearAbility alone | Does NOT cancel active abilities, removes spec unsafely |
| SetRemoveAbilityOnEnd alone | Does NOT stop active abilities, they continue |

Function behavior comparison:

| Function | Active Ability | Spec Removal |
|----------|----------------|--------------|
| Cancel Ability | Stops immediately | Spec remains |
| Set Remove Ability On End | Continues to natural end | After EndAbility |
| Cancel + SetRemoveOnEnd | Stops immediately | After cleanup |

### 14.11) Cleanup Order for Cross-Actor Systems

When father dies or player disconnects:

| Step | Action | Reason |
|------|--------|--------|
| 1 | Cancel Ability (each handle) | Stop execution, fire cleanup delegates |
| 2 | Set Remove Ability On End (each handle) | Queue spec removal after cleanup |
| 3 | Remove gameplay effects from player | Clear stat modifiers |
| 4 | Null father references | Prevent dangling pointers |
| 5 | Null player references | Bidirectional cleanup |

---

## SECTION 15: ABILITY CLEANUP PATTERNS

### 15.1) Event End Ability vs Event On Ability Ended

| Event Type | When Called | Safe for Cleanup |
|------------|-------------|------------------|
| Event End Ability (Override) | DURING ability termination | YES - references still valid |
| Event On Ability Ended | AFTER ability fully ended | NO - references may be invalidated |

Blueprint guides should use Event End Ability (override) for cleanup, not Event On Ability Ended.

### 15.2) IsEndAbilityValid Check

From NarrativeCombatAbility.cpp, always check before cleanup operations:

| Step | Action |
|------|--------|
| 1 | EndAbility Implementation |
| 2 | Is End Ability Valid? (check) |
| 3 | Branch: TRUE -> Proceed with cleanup |
| 4 | Branch: FALSE -> Skip cleanup (ability already ended) |

### 15.3) Timer Cleanup in EndAbility

| Step | Action |
|------|--------|
| 1 | EndAbility |
| 2 | Clear Timer by Handle (StopTimer variable) |
| 3 | Invalidate Timer Handle |
| 4 | Continue with other cleanup |

### 15.4) Delegate Cleanup in EndAbility

| Step | Action |
|------|--------|
| 1 | EndAbility |
| 2 | Get bound delegate reference |
| 3 | Remove Dynamic (unbind custom function) |
| 4 | Continue with other cleanup |

### 15.5) Form vs Utility Ability Cleanup

| Ability Type | EndAbility Cleanup | Reason |
|--------------|-------------------|--------|
| Form (Armor, Exo, etc.) | Full - restore speed, remove stat GEs | Each ability manages own state |
| Duration (StealthField) | Full - remove effects by handle | Self-managed duration |
| Passive (ProximityStrike) | Full - stop timers, unbind delegates | Continuous operation |

---

## SECTION 16: PERFORMANCE PATTERNS

### 16.1) Sphere Overlap: One-Time vs Continuous

| Use Case | Pattern | Performance |
|----------|---------|-------------|
| One-time detection (DomeBurst) | Sphere Overlap Actors on event | LOW cost (single query) |
| Continuous AoE (ProximityStrike) | Timer + Sphere Overlap every 0.5s | HIGH cost (repeated queries) |

Optimal Continuous AoE Pattern:

| Alternative | Description |
|-------------|-------------|
| Persistent Sphere Collision Component | Better for high-frequency checks |
| On Component Begin Overlap | Add to tracked actors array |
| On Component End Overlap | Remove from tracked actors array |
| Timer | Only applies damage to tracked actors (no physics query) |

### 16.2) Component Reference Caching

| Status | Pattern |
|--------|---------|
| WRONG (every tick) | Get Character Movement -> Set Speed, Get Character Movement -> Get Velocity |
| CORRECT (cached) | ActivateAbility: Get Character Movement -> SET CharMovementRef variable, Timer: GET CharMovementRef (cached) -> Set Speed |

### 16.3) Distance Check Before Expensive Operations

Cheap First pattern:

| Step | Action | Cost |
|------|--------|------|
| 1 | Get Distance To (target) | 1x (vector math) |
| 2 | Branch: Distance < MaxRange | Cheap comparison |
| 3 | TRUE: Proceed with expensive query | Only when needed |
| 4 | FALSE: Skip | No expensive operation |

Cost Comparison:

| Operation | Relative Cost |
|-----------|---------------|
| Get Distance To | 1x |
| Line Trace | 10-50x |
| AI Perception Query | 20-100x |
| Sphere Overlap Actors | 50-200x |

---

## SECTION 17: MULTIPLAYER SYNC PATTERNS

### 17.1) Property Replication Timing Risk

From GAS Best Practices:
"If you add a replicated property to your GA and the server modifies the value, you are not guaranteed that the property value change and ability RPCs arrive in the same order on the client."

Affected Variables:

| Variable | Risk | Affected Abilities |
|----------|------|-------------------|
| OriginalWalkSpeed | Stale value restoration | GA_FatherArmor, GA_FatherExoskeleton |
| CurrentForm | Wrong form on client | BP_FatherCompanion |

### 17.2) WaitNetSync for Critical Operations

| Step | Action |
|------|--------|
| 1 | Client/Server Sync Point |
| 2 | Wait Net Sync (Only Server Wait) |
| 3 | Server waits for client confirmation |
| 4 | Guarantees both sides aligned before continuing |

Use Cases:

| Operation | Needs Sync |
|-----------|------------|
| Movement speed restoration | YES |
| Form transition completion | YES |
| Damage application | NO (Server Only policy) |

### 17.3) Server Only vs Local Predicted Trade-offs

| Policy | VFX Timing | Damage Timing | Use When |
|--------|------------|---------------|----------|
| Local Predicted | Immediate | Confirmed by server | Player input abilities |
| Server Only | Delayed on client | Immediate | Autonomous NPC abilities |

Solution for Server Only VFX delay: Spawn VFX locally in ability, but damage calculation remains server-only.

### 17.4) Misprediction Handling

For Local Predicted abilities, implement graceful handling when server rejects:

| Event | Client Experience |
|-------|-------------------|
| Ability Activation | Client predicts: Apply movement speed boost |
| Server rejects | Client has Activation Blocked Tags |
| Client observes | Speed snaps back (rubber-banding) |

Mitigation:

| Step | Action |
|------|--------|
| 1 | Use On Ability Failed event for feedback |
| 2 | Play "denied" audio cue |
| 3 | Do not show prolonged visual effects until confirmed |

---

## SECTION 18: NARRATIVE PRO C++ CLEANUP PATTERNS (REFERENCE)

### 18.1) AbilityTask_MoveToLocationAndWait.cpp

OnDestroy pattern:

| Step | Action |
|------|--------|
| 1 | Check if PFComp valid |
| 2 | PFComp->OnRequestFinished.Remove(FinishMoveHandle) - Unbind delegate |
| 3 | Super::OnDestroy(AbilityEnding) - ALWAYS call last |

### 18.2) NarrativeNPCController.cpp

EndPlay pattern:

| Step | Action |
|------|--------|
| 1 | Super::EndPlay(EndPlayReason) |
| 2 | Get PathFollowingComponent |
| 3 | PFC->OnRequestFinished.RemoveAll(this) - Clean all bindings |

### 18.3) AsyncAction_BeginDialogueAndWait.cpp

SetReadyToDestroy pattern:

| Step | Action |
|------|--------|
| 1 | Check OwningTalesComponent valid |
| 2 | Remove all delegate bindings from component |
| 3 | Super::SetReadyToDestroy() |

### 18.4) NarrativeCombatAbility.cpp

EndAbility pattern:

| Step | Action |
|------|--------|
| 1 | IsEndAbilityValid(Handle, ActorInfo) - CRITICAL check |
| 2 | Branch on validity |
| 3 | TRUE: Remove target data delegate |
| 4 | ConsumeClientReplicatedTargetData |
| 5 | Super::EndAbility(...) - Call parent LAST |

### 18.5) GameplayTask_MoveToLocationAndRotation.cpp

OnDestroy pattern:

| Step | Action |
|------|--------|
| 1 | Restore player state (remove loose gameplay tags) |
| 2 | Restore input (SetIgnoreMoveInput false) |
| 3 | Super::OnDestroy(AbilityEnding) |
| 4 | Remove delegate AFTER super (component may be destroyed) |

Key observation: Some Narrative Pro code removes delegates AFTER Super::OnDestroy().

---

## SECTION 19: FORM STATE ARCHITECTURE

### 19.1) v4.0 Architecture Overview

| Feature | Implementation |
|---------|----------------|
| Form Tags | Activation Owned Tags (replicates with ReplicateActivationOwnedTags) |
| Form Switching | T key form wheel - direct form-to-form |
| Mutual Exclusivity | Cancel Abilities With Tag |
| Stat Modification | Gameplay Effects (GE_ArmorBoost, GE_ExoskeletonSpeed, etc.) |
| Speed Restoration | EndAbility with Timeline lerp |
| Cooldown | 15s shared (Cooldown.Father.FormChange) |

### 19.2) ReplicateActivationOwnedTags Setting

Project Settings -> Gameplay Ability System:

| Setting | Value | Effect |
|---------|-------|--------|
| Replicate Activation Owned Tags | ENABLED | Activation Owned Tags replicate to clients |

This enables using Activation Owned Tags for form state without Infinite GEs.

### 19.3) Form Ability Configuration Template

All form abilities follow this pattern:

| Property | Value |
|----------|-------|
| Parent Class | NarrativeGameplayAbility |
| Ability Tags | Ability.Father.[Form] |
| Cancel Abilities with Tag | All other form tags (Ability.Father.Crawler, Ability.Father.Armor, etc.) |
| Activation Owned Tags | Father.Form.[Form], Father.State.[Attached/Detached/Deployed] |
| Cooldown Gameplay Effect Class | GE_FormChangeCooldown |
| Net Execution Policy | Server Only |
| Instancing Policy | Instanced Per Actor |

### 19.4) Form Stats Reference

| Form | Speed | Jump | Attack | Other |
|------|-------|------|--------|-------|
| Crawler | 100% (base) | 100% (base) | - | Default AI following |
| Armor | -15% | - | - | +50 Armor |
| Exoskeleton | +50% | +30% | +10 AttackRating | - |
| Symbiote | +50% | +30% | +100 AttackDamage | Infinite Stamina |
| Engineer | - | - | - | Turret mode (no player stats) |

### 19.5) Symbiote Special Case - 30 Second Duration

Symbiote form has fixed 30 second duration with form wheel lock:

| Step | Action |
|------|--------|
| 1 | GA_FatherSymbiote activates |
| 2 | Apply Father.State.Transitioning tag |
| 3 | Cancel old form ability |
| 4 | Spawn transition VFX, wait 5s |
| 5 | Apply stat boosts to player |
| 6 | Remove Father.State.Transitioning tag |
| 7 | CommitAbilityCooldown (applies Cooldown.Father.FormChange 15s) |
| 8 | Add Father.State.SymbioteLocked tag |
| 9 | Start 30 second timer |
| 10 | Ability stays active (does NOT end) |
| 11 | Timer completes after 30 seconds |
| 12 | Remove Father.State.SymbioteLocked tag |
| 13 | Auto-activate GA_FatherArmor |
| 14 | GA_FatherArmor cancels GA_FatherSymbiote |
| 15 | EndAbility restores player stats |

> **INV-1 Note:** GE_Invulnerable was REMOVED per GAS Audit decision INV-1. Transition no longer grants damage immunity.

### 19.5.1) Symbiote Lock Behavior

| Aspect | Behavior |
|--------|----------|
| T Wheel During Symbiote | Opens, but form abilities grayed out |
| Other T Wheel Items | Still usable |
| Auto-Return Destination | Armor form (not Crawler) |
| Early Exit | Not allowed (player cannot exit early) |
| Lock Tag | Father.State.SymbioteLocked |

### 19.6) Form Cooldown Implementation (Built-in System)

Shared 15 second cooldown for all form changes using GAS built-in cooldown:

| Property | Value |
|----------|-------|
| Cooldown Tag | Cooldown.Father.FormChange |
| Duration | 15 seconds |
| Scope | All form abilities share this cooldown |
| Implementation | GE_FormChangeCooldown via CooldownGameplayEffectClass |

GE_FormChangeCooldown Configuration:

| Property | Value |
|----------|-------|
| Duration Policy | Has Duration |
| Duration Magnitude | 15.0 |
| Components -> Grant Tags to Target Actor -> Add to Inherited | Cooldown.Father.FormChange |

Form Ability Configuration (Built-in Cooldown):

| Property | Value |
|----------|-------|
| Cooldown Gameplay Effect Class | GE_FormChangeCooldown |
| Blueprint Node | CommitAbilityCooldown (after form change completes) |
| Block Abilities with Tag | NOT NEEDED - GAS handles automatically |

Built-in Cooldown Benefits:

| Benefit | Description |
|---------|-------------|
| Automatic Blocking | GAS checks cooldown tag before allowing activation |
| UI Support | GetCooldownTimeRemaining() function available |
| Cleaner Config | No Block Abilities with Tag needed |
| Standard Pattern | Follows official GAS documentation |

### 19.7) Direct Form-to-Form Transition Flow

Example: Armor to Exoskeleton

| Step | System | Action |
|------|--------|--------|
| 1 | Player | Holds T key, form wheel opens |
| 2 | Player | Selects Exoskeleton |
| 3 | Form Wheel | Confirms selection, activates GA_FatherExoskeleton |
| 4 | GA_FatherExoskeleton | GAS checks Activation Required/Blocked Tags |
| 5 | GA_FatherExoskeleton | Adds Father.State.Transitioning tag |
| 6 | GA_FatherExoskeleton | Cancel Abilities With Tag cancels GA_FatherArmor |
| 7 | GA_FatherArmor | EndAbility fires (bWasCancelled = true) |
| 8 | GA_FatherArmor | Restores player speed to stored original |
| 9 | GA_FatherArmor | Removes GE_ArmorBoost |
| 10 | GA_FatherExoskeleton | Spawns NS_FatherFormTransition Niagara VFX |
| 11 | GA_FatherExoskeleton | Waits 5 seconds (AbilityTaskWaitDelay per Track B v4.15) |
| 12 | GA_FatherExoskeleton | Father repositions (chest to back) |
| 13 | GA_FatherExoskeleton | Stores current player speed |
| 14 | GA_FatherExoskeleton | Applies GE_ExoskeletonSpeed (+50% speed, +30% jump, +10 attack) |
| 15 | GA_FatherExoskeleton | Sets CurrentForm = Exoskeleton |
| 16 | GA_FatherExoskeleton | Removes Father.State.Transitioning tag |
| 17 | GA_FatherExoskeleton | Calls CommitAbilityCooldown (applies GE_FormChangeCooldown 15s) |
| 18 | GA_FatherExoskeleton | Ends ability |

> **INV-1 Note:** GE_Invulnerable steps were REMOVED per GAS Audit decision INV-1. Transition no longer grants damage immunity.

### 19.7.1) Transition Animation Parameters

| Parameter | Value |
|-----------|-------|
| VFX Duration | 5 seconds |
| Father Invulnerable | **No (INV-1)** |
| Player Movement | Allowed during transition |
| Player Combat | Allowed during transition |
| Cancel Transition | Not allowed once started |
| Cooldown Start | After transition completes |

> **INV-1 Note:** Father is NO LONGER invulnerable during transition per GAS Audit decision INV-1.

### 19.7.2) Form Activation Tag Configuration

All form abilities use these tags:

| Tag Type | Tags |
|----------|------|
| Activation Required | Father.State.Alive, Father.State.Recruited |
| Activation Blocked | Father.State.Dormant, Father.State.Transitioning, Father.State.SymbioteLocked |
| Cooldown Gameplay Effect Class | GE_FormChangeCooldown (grants Cooldown.Father.FormChange) |

### 19.8) Cancel Abilities With Tag Configuration

Each form ability cancels all other forms:

| Form Ability | Cancel Abilities With Tag |
|--------------|---------------------------|
| GA_FatherCrawler | Ability.Father.Armor, Ability.Father.Exoskeleton, Ability.Father.Symbiote, Ability.Father.Engineer |
| GA_FatherArmor | Ability.Father.Crawler, Ability.Father.Exoskeleton, Ability.Father.Symbiote, Ability.Father.Engineer |
| GA_FatherExoskeleton | Ability.Father.Crawler, Ability.Father.Armor, Ability.Father.Symbiote, Ability.Father.Engineer |
| GA_FatherSymbiote | Ability.Father.Crawler, Ability.Father.Armor, Ability.Father.Exoskeleton, Ability.Father.Engineer |
| GA_FatherEngineer | Ability.Father.Crawler, Ability.Father.Armor, Ability.Father.Exoskeleton, Ability.Father.Symbiote |

### 19.9) Stat Effect Configuration

| Form | Effect | Modifiers |
|------|--------|-----------|
| GE_ArmorBoost | Armor +50 | Modifier: Armor, Add, 50.0 |
| GE_ExoskeletonSpeed | Speed +50%, Jump +30%, Attack +10 | Direct CharacterMovement for speed/jump, Modifier: AttackRating for attack |
| GE_SymbioteBoost | Speed +50%, Jump +30%, Attack +100, Infinite Stamina | CharacterMovement + Modifier: AttackDamage + Infinite Stamina |

### 19.10) Form State vs Stat Modification

Form state and stat effects are managed separately:

| Purpose | Mechanism |
|---------|-----------|
| Form state tags | Activation Owned Tags on ability |
| Stat modifications | Gameplay Effects (applied in ActivateAbility, removed in EndAbility) |

Why separate:
- Activation Owned Tags auto-cleanup when ability ends
- Stat GEs need explicit removal for lerped transitions
- Different lifecycles (tags immediate, stats gradual)

### 19.11) Engineer Form Special Handling

Engineer form father becomes turret:

| Step | Action |
|------|--------|
| 1 | GA_FatherEngineer activates |
| 2 | Father detaches from player |
| 3 | Father moves to deployment location |
| 4 | Father enters turret mode (stationary) |
| 5 | Player selects new form from wheel |
| 6 | GA_FatherEngineer EndAbility fires |
| 7 | Father teleports to player (200 units behind) |
| 8 | New form ability activates |

No separate recall ability needed - Cancel Abilities With Tag triggers EndAbility cleanup.

### 19.12) Option B: Form Identity via GE_*State (v6.1)

Form identity persists via Infinite-duration GameplayEffects, NOT Activation Owned Tags alone.

| Form | State GE | Identity Tag |
|------|----------|--------------|
| Crawler | GE_CrawlerState | Effect.Father.FormState.Crawler |
| Armor | GE_ArmorState | Effect.Father.FormState.Armor |
| Exoskeleton | GE_ExoskeletonState | Effect.Father.FormState.Exoskeleton |
| Symbiote | GE_SymbioteState | Effect.Father.FormState.Symbiote |
| Engineer | GE_EngineerState | Effect.Father.FormState.Engineer |

**Invariant:** Exactly one `Effect.Father.FormState.*` tag must exist on the Father ASC at runtime.

**Migration Note:** Option B is the target architecture. Legacy manifests may still reference `Father.Form.*` until cleanup is complete.

### 19.13) Attached Form Invulnerability - REMOVED (INV-1)

> **INV-1 COMPLIANCE (January 2026):** Per GAS Audit decision INV-1, ALL form-based invulnerability has been REMOVED from the Father companion system.

| Form | Attached | Grants Narrative.State.Invulnerable |
|------|----------|-------------------------------------|
| Crawler | No | **No** |
| Armor | Yes | **No (INV-1)** |
| Exoskeleton | Yes | **No (INV-1)** |
| Symbiote | Yes | **No (INV-1)** |
| Engineer | No | **No** |

**Current State:** GE_ArmorState, GE_ExoskeletonState, and GE_SymbioteState do NOT grant `Narrative.State.Invulnerable`. Only GA_FatherSacrifice provides invulnerability (to the player, for 8 seconds).

### 19.14) Form Transition Prelude

Transition logic executes in the **NEW** form's ActivateAbility:

| Step | Action |
|------|--------|
| 1 | Remove any active form state GEs by targeting parent tag `Effect.Father.FormState` |
| 2 | Apply new GE_*State (establishes new identity) |
| 3 | Continue with form-specific setup |

**Why NEW form handles transition:** The old form ability may already be ending/removed when the new form activates. Only the new form can reliably know what state to establish.

### 19.15) ASC Boundary Rule (Cross-ASC Gating)

`activation_required_tags` and `activation_blocked_tags` check the **owning ASC only**.

| Ability Owner | ASC Checked | Can Gate On Father Tags? |
|---------------|-------------|--------------------------|
| Father-owned (GA_FatherCrawler) | Father ASC | Yes |
| Player-owned (GA_ProtectiveDome) | Player ASC | **No** |

**Cross-ASC tag gating is impossible.** Equipment-granted abilities are Player-owned and cannot see Father ASC tags.

**Correct Pattern:** If a player-owned ability needs to depend on Father state, it must be gated by ability existence (equipment grant) or by player-side tags/effects, not Father tags.

### 19.16) Two-Part Ability System

Father abilities are granted via two mechanisms:

| Part | Granted Via | Owner ASC | Contents |
|------|-------------|-----------|----------|
| Baseline Abilities | AC_FatherCompanion_Default | Father | Form abilities + AI abilities (always present) |
| Form-Specific Abilities | EquippableItem abilities_to_grant | Player | Action abilities (conditionally granted when form equipped) |

### 19.17) Baseline Abilities (AC_FatherCompanion_Default)

| Category | Abilities | Trigger |
|----------|-----------|---------|
| Form abilities | GA_FatherCrawler, GA_FatherArmor, GA_FatherExoskeleton, GA_FatherSymbiote, GA_FatherEngineer | T wheel |
| Crawler AI | GA_FatherAttack, GA_FatherLaserShot, GA_FatherMark | AI automatic |
| Engineer AI | GA_TurretShoot, GA_FatherElectricTrap | AI automatic |
| General | GA_FatherSacrifice | Auto trigger |

**Total:** 11 baseline abilities on Father ASC.

### 19.18) Form-Specific Abilities (Equipment-Granted)

| Form | EquippableItem | Abilities Granted | Owner ASC |
|------|----------------|-------------------|-----------|
| Armor | EI_FatherArmorForm | GA_ProtectiveDome, GA_DomeBurst | Player |
| Exoskeleton | EI_FatherExoskeletonForm | GA_ExoskeletonDash, GA_ExoskeletonSprint, GA_StealthField, GA_Backstab | Player |
| Symbiote | EI_FatherSymbioteForm | GA_ProximityStrike | Player |

**Gate Pattern:** Ability existence IS the gate. If form is equipped, abilities are available. If unequipped, abilities are revoked.

**DO NOT** add `activation_required_tags` referencing Father tags to equipment-granted abilities.

### 19.19) Form Activation Mechanism

| Form | Activation Method | Equipment Required? |
|------|-------------------|---------------------|
| Crawler | T wheel -> GAS ability directly | NO |
| Engineer | T wheel -> GAS ability directly | NO |
| Armor | T wheel -> Equip EI_FatherArmorForm | YES |
| Exoskeleton | T wheel -> Equip EI_FatherExoskeletonForm | YES |
| Symbiote | T wheel -> Equip EI_FatherSymbioteForm | YES |
| Rifle | Equip command | YES (weapon) |
| Sword | Equip command | YES (weapon) |

**Key Distinction:**
- Crawler/Engineer: Father-owned AI abilities, activated via GAS
- Armor/Exo/Symbiote: Attach to player, grant player-owned abilities via equipment

### 19.20) Weapons vs Forms

Weapons (Rifle, Sword) are equipment-mode selections, not body-form transitions:

| Weapon | Equipment Item | Form Change? | State GE? |
|--------|----------------|--------------|-----------|
| GA_FatherRifle | EI_FatherRifleForm | No | No |
| GA_FatherSword | EI_FatherSwordForm | No | No |

Weapons live in the FatherForm slot for UI/NP consistency but do not establish body-form identity and must not use GE_*State / Effect.Father.FormState.*.

### 19.21) Orphan Tags (DO NOT USE for Gating)

The following tags are legacy/non-authoritative for identity:

| Tag | Status | Replacement |
|-----|--------|-------------|
| Father.Form.Crawler | Orphan | Effect.Father.FormState.Crawler |
| Father.Form.Armor | Orphan | Effect.Father.FormState.Armor |
| Father.Form.Exoskeleton | Orphan | Effect.Father.FormState.Exoskeleton |
| Father.Form.Symbiote | Orphan | Effect.Father.FormState.Symbiote |
| Father.Form.Engineer | Orphan | Effect.Father.FormState.Engineer |

**Why Orphan:** No GE grants these tags. They were used in legacy Activation Owned Tags pattern but are not persistent.

**Rule:** Do not use `Father.Form.*` for activation_required_tags or activation_blocked_tags. Use `Effect.Father.FormState.*` for Father-owned abilities.

---

## SECTION 20: ABILITYCONFIGURATION SYSTEM

### 20.1) AbilityConfiguration Overview

AbilityConfiguration (AC_*) is a data asset that defines abilities and attributes granted to an NPC at spawn.

Parent Class: AbilityConfiguration.h / AbilityConfiguration.cpp

### 20.2) AC_NPC_Default Configuration

| Section | Property | Purpose |
|---------|----------|---------|
| Default Attributes | Array of Attribute Sets | Grants attribute sets on spawn |
| Startup Effects | Array of Gameplay Effects | Applied immediately on spawn |
| Default Abilities | Array of Gameplay Abilities | Granted on spawn |
| Goal Generators | Array of NPCGoalGenerator | AI goal creation systems |

### 20.3) Default Abilities Array Example (AC_RunAndGun)

| Index | Ability | Purpose |
|-------|---------|---------|
| [0] | GA_Locomotion | Movement system |
| [1] | GA_Sprint | Sprint ability |
| [2] | GA_Dodge | Dodge/roll ability |
| [3] | GA_Jump | Jump ability |
| [4] | GA_Interact | Interaction ability |
| [5] | GA_Death | Death handling |
| [6] | GA_Melee_Unarmed | Unarmed combat |
| [7] | GA_Grenade | Grenade throw |

### 20.4) Father Companion Configuration

| Asset | Contents |
|-------|----------|
| AC_FatherCompanion_Default | GA_FatherCrawler, GA_FatherArmor, GA_FatherExoskeleton, GA_FatherSymbiote, GA_FatherEngineer, GA_FatherAttack, GA_FatherLaserShot, GA_FatherMark, GA_TurretShoot, GA_FatherElectricTrap, GA_FatherSacrifice |

All form abilities granted at spawn. Form wheel controls which one activates.

### 20.5) Goal Generators Array

| Property | Value |
|----------|-------|
| Goal Generators | Array of NPCGoalGenerator blueprints |
| Example | GoalGenerator_Attack |

---

## SECTION 21: ACTIVITYCONFIGURATION SYSTEM

### 21.1) ActivityConfiguration Overview

ActivityConfiguration (uses ActConfig_ prefix) defines AI behaviors and activities for NPCs.

Naming Convention:

| Asset Type | Prefix | Example |
|------------|--------|---------|
| AbilityConfiguration | AC_ | AC_FatherCompanion_Default |
| ActivityConfiguration | ActConfig_ | ActConfig_FatherCompanion |

Parent Class: NPCActivityConfiguration.h / NPCActivityConfiguration.cpp

### 21.2) AC_RunAndGun Activity Configuration

| Property | Value |
|----------|-------|
| Request Interval | 0.5 |
| Default Activities | Array of BPA_ activities |
| Goal Generators | GoalGenerator_Attack |

### 21.3) Default Activities Array

| Index | Activity | Purpose |
|-------|----------|---------|
| [0] | BPA_Attack_Melee | Close combat |
| [1] | BPA_Attack_RangedCrouching_Strafe | Ranged with strafe |
| [2] | BPA_Attack_RangedStand_Stationary | Stationary ranged |
| [3] | BPA_Attack_Grenade | Grenade throw |
| [4] | BPA_Idle | Idle behavior |
| [5] | BPA_Wander | Wandering |
| [6] | BPA_Flee | Fleeing behavior |

### 21.4) Dual Configuration Architecture

NPCDefinition references BOTH configurations:

| Configuration Type | Asset Name | Contains |
|-------------------|------------|----------|
| AbilityConfiguration | AC_FatherCompanion_Default | Abilities, Attributes |
| ActivityConfiguration | ActConfig_FatherCompanion | Behaviors, Goal Generators |

---

## SECTION 22: NPC ACTIVITY SYSTEM (BPA_)

### 22.1) NPC Activity Overview

BPA_ (Blueprint Activity) files define specific AI behaviors that score and execute based on goals.

Parent Class: NPCActivity.h / NPCActivity.cpp

### 22.2) Activity Hierarchy

| Level | Class | Purpose |
|-------|-------|---------|
| 1 | NPCActivity (C++) | Base activity class |
| 2 | BPA_Attack (Blueprint) | Attack behavior base |
| 3 | BPA_Attack_Melee (Blueprint) | Melee-specific attack |

### 22.3) BPA_Attack_Melee Configuration

| Section | Property | Value |
|---------|----------|-------|
| NPC Activity | Get Owner Character | Returns owning NPC |
| NPC Activity | Get Owner Controller | Returns NPC controller |
| NPC Activity | Is Running | Activity execution state |

### 22.4) Key Activity Functions

| Function | Purpose |
|----------|---------|
| SetupBlackboard | Initializes blackboard values for behavior tree |
| GetWeaponToAttackWith | Selects weapon for attack |
| ScoreGoalItem | Calculates activity priority score |
| K2_RunActivity | Executes the activity (Blueprint override) |
| K2_StopActivity | Stops activity execution |

### 22.5) ScoreGoalItem Pattern

| Step | Action |
|------|--------|
| 1 | Receive Goal_Attack item |
| 2 | Check weapon availability |
| 3 | Check distance to target |
| 4 | Calculate base score |
| 5 | Apply modifiers (health, ammo, etc.) |
| 6 | Return final score |

Higher score = higher priority for this activity.

### 22.6) Activity Variables

| Variable | Type | Purpose |
|----------|------|---------|
| CurrentGoalItem | NPCGoalItem | Current goal being processed |
| OwnerCharacter | NarrativeNPCCharacter | Cached owner reference |
| OwnerController | NarrativeNPCController | Cached controller reference |
| IsRunning | Boolean | Activity execution state |

---

## SECTION 23: GOAL SYSTEM

### 23.1) Goal Generator Overview

GoalGenerator creates and manages AI goals based on perception data.

Parent Class: NPCGoalGenerator.h / NPCGoalGenerator.cpp

### 23.2) GoalGenerator_Attack Configuration

| Property | Value |
|----------|-------|
| Query Template | EQS_Actor_SensedAttackTarget |
| Run Mode | All Matching |
| Query Run Interval | 0.5 |
| Attack Goal Class | Goal_Attack |
| Attack Goal Base Score | 3.0 |

### 23.3) Attack Affiliation Map

| AI Sense | Detect Enemies | Detect Neutrals | Detect Friendlies |
|----------|----------------|-----------------|-------------------|
| AISense_Damage | Checked | Checked | Unchecked |
| AISense_Sight | Checked | Unchecked | Unchecked |
| AISense_Hearing | Checked | Unchecked | Unchecked |

### 23.4) GoalGenerator Functions

| Function | Purpose |
|----------|---------|
| Event InitializeGoalGenerator | Setup on spawn |
| TryAddAttackGoalFromActor | Create goal from detected actor |
| DoesAttitudeMatchFilter | Check faction relationship |
| OnStartSenseActor | Handle new perception |
| OnStopSenseActor | Handle lost perception |
| OnPerceptionUpdated_Event | Process perception changes |
| RefreshPerceivedActors | Update all tracked actors |

### 23.5) Goal_Attack Configuration

Parent Class: NPCGoalItem.h / NPCGoalItem.cpp

| Property | Type | Purpose |
|----------|------|---------|
| TargetActor | Actor Reference | Enemy to attack |
| TargetLastKnownLocation | Vector | Last seen position |
| TargetLastSeenTime | Float | Timestamp of last sight |
| IsTargetVisible | Boolean | Current visibility |
| GoalScore | Float | Priority score |

### 23.6) Goal Check Functions

| Function | Returns | Purpose |
|----------|---------|---------|
| IsTargetAlive | Boolean | Check target health > 0 |
| IsTargetInRange | Boolean | Check distance to target |
| CanSeeTarget | Boolean | Check line of sight |
| GetTimeSinceLastSeen | Float | Staleness of target info |

### 23.7) Complete Attack Flow

| Step | System | Action |
|------|--------|--------|
| 1 | AIPerception | Detects enemy actor |
| 2 | GoalGenerator_Attack | OnPerceptionUpdated fires |
| 3 | GoalGenerator_Attack | TryAddAttackGoalFromActor creates Goal_Attack |
| 4 | Goal_Attack | Stores target actor, location, visibility |
| 5 | ActivityConfiguration | Evaluates BPA_ activities against goal |
| 6 | BPA_Attack_Melee | ScoreGoalItem returns priority score |
| 7 | NPC Controller | Selects highest scoring activity |
| 8 | BPA_Attack_Melee | K2_RunActivity executes behavior tree |
| 9 | Behavior Tree | Runs attack sequence |

---

## SECTION 24: ENVIRONMENT QUERY SYSTEM (EQS)

### 24.1) EQS Overview

EQS queries find and score actors/locations for AI decision making.

### 24.2) Available EQS Templates

| EQS Name | Type | Purpose |
|----------|------|---------|
| EQS_Actor_SensedAttackTarget | Actor Query | Find actors from perception |
| EQS_Move_LookBusy | Location Query | Find idle positions |
| EQS_Move_MeleeAttack | Location Query | Find melee approach positions |
| EQS_Move_RangedStrafe_GetClose | Location Query | Find close strafe positions |
| EQS_Move_RangedStrafe_StayBack | Location Query | Find distant strafe positions |
| EQS_Move_ToAttackTarget | Location Query | Find path to target |

### 24.3) EQS_Actor_SensedAttackTarget Configuration

| Section | Test | Purpose |
|---------|------|---------|
| Generator | Actors Of Class | Find NarrativeCharacter actors |
| Test 1 | EnvQueryTest_Team | Filter by faction |
| Test 2 | Gameplay Tags | Filter by state tags |
| Test 3 | Distance | Score by distance |
| Test 4 | Dot Product | Score by facing direction |
| Test 5 | EnvQueryTest_AttackTokens | Check token availability |
| Test 6 | EnvQueryTest_AttackPriority | Priority scoring |

### 24.4) EQS Test Details

| Test | Scoring |
|------|---------|
| Team | Filter Only (Hostile = Pass) |
| Distance | Prefer closer targets |
| Dot Product | Prefer targets in front |
| Attack Tokens | Filter targets with available tokens |
| Attack Priority | Higher priority = higher score |

### 24.5) EQS Context Providers

| Context | Class | Provides |
|---------|-------|----------|
| EQSContext_AttackTarget | Blueprint | Current attack target actor |
| EQSContext_Querier | Built-in | The querying NPC |

### 24.6) Father Companion EQS Usage

| Father Behavior | EQS to Use |
|-----------------|------------|
| Find attack targets | EQS_Actor_SensedAttackTarget |
| Melee positioning | EQS_Move_MeleeAttack |
| Laser positioning | EQS_Move_RangedStrafe_StayBack |
| Navigate to enemy | EQS_Move_ToAttackTarget |

---

## SECTION 25: COMBAT ABILITY HIERARCHY

### 25.1) Complete Ability Hierarchy

| Level | Class | Type | Purpose |
|-------|-------|------|---------|
| 1 | UGameplayAbility | C++ | Engine base |
| 2 | NarrativeGameplayAbility | C++ | Narrative Pro base |
| 3 | NarrativeCombatAbility | C++ | Combat with trace support |
| 4 | GA_CombatAbilityBase | Blueprint | Combat configuration |
| 5 | GA_Attack_ComboBase | Blueprint | Combo system |
| 6 | GA_Attack_Combo_Melee | Blueprint | Melee combos |
| 7 | GA_Melee_Unarmed | Blueprint | Unarmed attacks |

### 25.2) GA_CombatAbilityBase Properties

| Section | Property | Value |
|---------|----------|-------|
| Target Data Handling | Damage Effect Class | GE_WeaponDamage |
| Cues | Fire Cue Tag No Impact | GameplayCue.Weapon.Fire.NoImpact |
| Cues | Fire Cue Tag | GameplayCue.Weapon.Fire |
| Narrative Ability | Requires Ammo | Checked |
| Narrative Ability | Input Tag | None |
| Damage | Default Attack Damage | 12.5 |
| Advanced | Replication Policy | Do Not Replicate |
| Advanced | Instancing Policy | Instanced Per Actor |
| Advanced | Net Execution Policy | Local Predicted |

### 25.3) GA_Attack_Combo_Melee Features

| Feature | Implementation |
|---------|----------------|
| Combo System | Selected Combo, Combo Idx, Combo Expire Time |
| Warping | ShouldWarp, WarpLocation, MinWarpDist, MaxWarpDist |
| Execution | IsPlayingExecution, ExecutionTarget, Should Execute? |
| Backstab | IsBackstabbing detection |
| Animation | Attack Combo Anim Set, Heavy Attack Combo Anim Set |

### 25.4) GA_Melee_Unarmed Configuration

| Property | Value |
|----------|-------|
| Attack Right Socket | hand_r |
| Attack Left Socket | hand_l |
| Attack Combo Anim Set | Narrative.Anim.AnimSets.Attacks.Unarmed.Light |
| Heavy Attack Combo Anim Set | Narrative.Anim.AnimSets.Attacks.Unarmed.Heavy |
| Should Warp | Checked |
| Warp Maintain Dist | 100.0 |
| Min Warp Dist | 20.0 |
| Max Warp Dist | 250.0 |
| Damage Effect Class | GE_WeaponDamage_Heavy |
| Default Attack Damage | 13.2 |
| Input Tag | Narrative.Input.Attack |

### 25.5) Firearm Ability Hierarchy

| Level | Class | Purpose |
|-------|-------|---------|
| 1 | NarrativeGameplayAbility | Base |
| 2 | GA_Attack_Firearm_Base | Firearm configuration |
| 3 | GA_Firearm_Rifle | Rifle implementation |
| 3 | GA_Attack_Firearm_Projectile | Projectile weapons |

### 25.6) GA_Attack_Firearm_Base Properties

| Property | Value |
|----------|-------|
| Is Automatic | Unchecked (base) |
| Rate Of Fire | 0.2 |
| Burst Amount | -1 |
| Trace Distance | 10000.0 |
| Trace Radius | 0.0 |
| Requires Ammo | Checked |
| Damage Effect Class | GE_WeaponDamage |
| Default Attack Damage | 12.5 |

### 25.7) GA_Attack_Firearm_Projectile Properties

| Property | Value |
|----------|-------|
| Projectile Class | BP_WeaponProjectile_Launcher |
| Input Tag | Narrative.Input.Attack |
| Damage Effect Class | GE_WeaponDamage |
| Default Attack Damage | 12.5 |

### 25.8) Father Parent Class Selection

| Father Ability | Recommended Parent | Reason |
|----------------|-------------------|--------|
| GA_FatherAttack | GA_Melee_Unarmed | Inherits warp, combo, socket trace |
| GA_FatherLaserShot | NarrativeGameplayAbility | Custom projectile spawn |
| Other abilities | NarrativeGameplayAbility | Standard ability pattern |

---

## SECTION 26: GAMEPLAY EFFECT PATTERNS

### 26.1) GE_WeaponDamage Configuration

| Property | Value |
|----------|-------|
| Duration Policy | Instant |
| Executions | NarrativeDamageExecCalc |
| Calculation Modifiers | AttackDamage (Source, Snapshotted) |
| Magnitude Calculation | Set By Caller |
| Set By Caller Tag | SetByCaller.Damage |
| Gameplay Cue Tags | GameplayCue.TakeDamage |

### 26.2) GE_WeaponDamage_Heavy

Parent: GE_WeaponDamage

| Property | Override Value |
|----------|----------------|
| Gameplay Cue Tags | GameplayCue.TakeDamage.Heavy |

### 26.3) GE_EquipmentMod Configuration

| Property | Value |
|----------|-------|
| Duration Policy | Infinite |
| Modifiers | Multiple SetByCaller modifiers |

| Modifier | Attribute | Operation |
|----------|-----------|-----------|
| 1 | Armor | Add |
| 2 | AttackRating | Add |
| 3 | StealthRating | Add |

### 26.4) GE_Invulnerable Configuration (Reference Only)

> **INV-1 Note:** This configuration is documented for reference. Per GAS Audit decision INV-1, Father does NOT use GE_Invulnerable except in GA_FatherSacrifice.

| Property | Value |
|----------|-------|
| Duration Policy | Infinite |
| Components | Grant Tags to Target Actor |
| Granted Tag | Narrative.State.Invulnerable |

Usage Pattern (I-Frames) - **NOT used by Father per INV-1**:

| Step | Description |
|------|-------------|
| 1 | Apply GE_Invulnerable at dodge/roll start |
| 2 | NarrativeDamageExecCalc checks for Narrative.State.Invulnerable tag |
| 3 | If tag present, damage is blocked |
| 4 | Remove GE_Invulnerable after i-frame window ends |

**Father Exception:** Only GA_FatherSacrifice uses invulnerability (GE_SacrificeInvulnerability) to protect the player for 8 seconds.

Father Abilities Using Invulnerability:

| Ability | Usage |
|---------|-------|
| GA_FatherExoskeletonDash | Brief i-frames during dash |
| GA_FatherSacrifice | Invulnerable during sacrifice animation |

### 26.5) GE_Damage_SetByCaller Configuration

| Property | Value |
|----------|-------|
| Duration Policy | Instant |
| Executions | NarrativeDamageExecCalc |
| Magnitude Calculation | Set By Caller |
| Set By Caller Tag | SetByCaller.Damage |

Blueprint Usage:

| Step | Node |
|------|------|
| 1 | Make Outgoing Gameplay Effect Spec (GE_Damage_SetByCaller) |
| 2 | Assign Tag Set By Caller Magnitude (SetByCaller.Damage, DamageValue) |
| 3 | Apply Gameplay Effect Spec to Target |

### 26.6) GE_Heal_SetByCaller Configuration

| Property | Value |
|----------|-------|
| Duration Policy | Instant |
| Executions | NarrativeHealExecution |
| Magnitude Calculation | Set By Caller |
| Set By Caller Tag | SetByCaller.Heal |

Blueprint Usage:

| Step | Node |
|------|------|
| 1 | Make Outgoing Gameplay Effect Spec (GE_Heal_SetByCaller) |
| 2 | Assign Tag Set By Caller Magnitude (SetByCaller.Heal, HealAmount) |
| 3 | Apply Gameplay Effect Spec to Target |

### 26.7) Father Damage Effect Selection

| Father Ability | Recommended GE | Reason |
|----------------|---------------|--------|
| GA_FatherAttack | GE_WeaponDamage | Standard damage |
| GA_FatherLaserShot | GE_WeaponDamage | Projectile damage |
| GA_ProximityStrike | Custom (SetByCaller) | Variable damage |
| GA_FatherElectricTrap | Custom (DOT) | Damage over time |

---

## SECTION 27: NARRATIVE PRO ABILITY EXAMPLES

### 27.1) GA_Sprint (Hold-to-Maintain Pattern)

| Property | Value |
|----------|-------|
| Input Tag | Narrative.Input.Sprint |
| Activation Owned Tags | Narrative.State.Sprinting |
| Net Execution Policy | Local Predicted |

| Pattern Element | Implementation |
|-----------------|----------------|
| Input Hold | Repeat Action AbilityTask |
| State While Active | Activation Owned Tags |
| End Condition | Input Release |

### 27.2) GA_Dodge (Instant with Cost)

| Property | Value |
|----------|-------|
| Input Tag | Narrative.Input.Dodge |
| Cost GE | GE_DodgeCost |
| Uses Motion Warping | Yes |
| Net Execution Policy | Local Predicted |

| Pattern Element | Implementation |
|-----------------|----------------|
| Resource Cost | Cost Gameplay Effect Class |
| Movement | Play Montage + Motion Warping |
| I-Frames | Apply GE with Narrative.State.Invulnerable |

### 27.3) GA_Death (Event-Triggered)

| Property | Value |
|----------|-------|
| Activation via Event | GameplayEvent.Death |
| Activation Owned Tags | Narrative.State.IsDead |
| Net Execution Policy | Server Only |

| Pattern Element | Implementation |
|-----------------|----------------|
| Trigger | Gameplay Event (not input) |
| Server Authority | Server Only policy |
| State Persistence | Activation Owned Tags until respawn |

---

## SECTION 28: INPUT MAPPING CONTEXT

### 28.1) IMC_Default Input Mappings

| Index | Input Action | Key (Keyboard) | Key (Gamepad) |
|-------|--------------|----------------|---------------|
| [0] | IA_Move | WASD | Left Stick |
| [1] | IA_Look | Mouse XY | Right Stick |
| [2] | IA_Attack | Left Mouse | Right Trigger |
| [3] | IA_Aim | Right Mouse | Left Trigger |
| [4] | IA_Sprint | Left Shift | Left Stick Click |
| [5] | IA_Crouch | Left Ctrl | B |
| [6] | IA_Jump | Space | A |
| [7] | IA_Dodge | C | B (double) |
| [8] | IA_Interact | E | X |
| [9] | IA_Reload | R | X (hold) |
| [10] | IA_ToggleWield | Tab | Y |
| [11] | IA_HeavyAttack | Middle Mouse | Right Bumper |
| [12] | IA_Ability1 | Q | Left Bumper |
| [13] | IA_Ability2 | F | D-Pad Down |
| [14] | IA_Ability3 | G | D-Pad Right |

### 28.2) Input Action to Input Tag Mapping

| Input Action | Input Tag |
|--------------|-----------|
| IA_Attack | Narrative.Input.Attack |
| IA_Aim | Narrative.Input.Aim |
| IA_Sprint | Narrative.Input.Sprint |
| IA_Jump | Narrative.Input.Jump |
| IA_Dodge | Narrative.Input.Dodge |
| IA_Interact | Narrative.Input.Interact |
| IA_ToggleWield | Narrative.Input.ToggleWield |
| IA_Ability1 | Narrative.Input.Ability1 |
| IA_Ability2 | Narrative.Input.Ability2 |
| IA_Ability3 | Narrative.Input.Ability3 |

### 28.3) Father Companion Input

| Father Input | Recommended Key | Input Tag |
|--------------|-----------------|-----------|
| Form Wheel | T (hold) | Narrative.Input.Father.FormWheel |
| Ability 1 | Q | Narrative.Input.Father.Ability1 |
| Ability 2 | E | Narrative.Input.Father.Ability2 |
| Ability 3 | R | Narrative.Input.Father.Ability3 |

---

## SECTION 29: GAMEPLAY EVENT TAGS

### 29.1) System Events

| Event Tag | Trigger | Use Case |
|-----------|---------|----------|
| GameplayEvent.Death | Health reaches 0 | GA_Death activation |
| GameplayEvent.KilledEnemy | Target dies from damage | Kill tracking |
| GameplayEvent.Interact | Interact input | GA_Interact activation |
| GameplayEvent.ToggleWield.On | Wield weapon | Weapon equip |
| GameplayEvent.ToggleWield.Off | Unwield weapon | Weapon unequip |

### 29.2) Animation Notify Events

| Event Tag | Animation Point | Purpose |
|-----------|-----------------|---------|
| GameplayEvent.AttackApex | Attack montage damage frame | Apply damage |
| GameplayEvent.EndAttack | Attack montage completion | End ability |
| GameplayEvent.Execution | Execution start | Cinematic kill |

### 29.3) Father-Specific Events

| Event Tag | Purpose |
|-----------|---------|
| GameplayEvent.Father.FormChange | Form transition started |
| GameplayEvent.Father.Sacrifice | Sacrifice activation |

---

## SECTION 30: ABILITY PATTERN SUMMARY

### 30.1) Pattern Categories

| Category | Examples | Key Characteristic |
|----------|----------|-------------------|
| Input-Triggered | GA_Attack, GA_Dodge | InputTag activation |
| Event-Triggered | GA_Death | GameplayEvent activation |
| Passive | GA_ProximityStrike | bActivateAbilityOnGranted |
| AI-Autonomous | GA_FatherAttack | AI behavior tree activation |

### 30.2) Net Execution Policy by Pattern

| Pattern | Recommended Policy | Reason |
|---------|-------------------|--------|
| Player Input | Local Predicted | Responsive feel |
| AI Autonomous | Server Only | Server authority |
| Passive NPC | Server Only | No client prediction needed |
| Form Change | Local Predicted | Player-triggered |

### 30.3) Common Activation Blocked Tags

| Tag | Blocks |
|-----|--------|
| Narrative.State.IsDead | All abilities except GA_Death |
| Narrative.State.Busy | Most combat abilities |
| Narrative.State.Movement.Climbing | Ground-based abilities |
| Narrative.State.Movement.Falling | Some movement abilities |

### 30.4) Father Ability to Pattern Mapping

| Father Ability | Pattern | Reference Ability |
|----------------|---------|-------------------|
| GA_FatherAttack | AI Autonomous | GA_Melee_Unarmed |
| GA_FatherExoskeletonSprint | Hold-to-Maintain | GA_Sprint |
| GA_FatherExoskeletonDash | Instant with Cost | GA_Dodge |
| GA_FatherDeath | Event-Triggered | GA_Death |
| GA_ProximityStrike | Passive | Custom |

---

## SECTION 31: GAMEPLAY CUE SYSTEM

### 31.1) Class Hierarchy

| Class | Base | Instanced | Latent Actions | Use Case |
|-------|------|-----------|----------------|----------|
| UGameplayCueNotify_Static | UObject | NO (CDO) | NO | Override HandleGameplayCue for simple logic |
| UGameplayCueNotify_Burst | Static | NO (CDO) | NO | Data-driven VFX/SFX without custom logic |
| AGameplayCueNotify_BurstLatent | Actor | YES | YES | One-off effects needing timelines/delays/montages |
| AGameplayCueNotify_Actor | Actor | YES | YES | Base for custom instanced cues |
| AGameplayCueNotify_Looping | Actor | YES | YES | Persistent effects (auras, DOTs, status) |

Class Selection Guide:

| Scenario | Recommended Class |
|----------|-------------------|
| Simple hit impact VFX | Burst |
| Weapon muzzle flash | Burst |
| Directional hit reaction + montage | BurstLatent |
| Damage numbers popup | BurstLatent |
| Burning/Poison DOT visual | Looping |
| Shield aura | Looping |
| Form transition VFX | BurstLatent |

### 31.2) Narrative Pro Cue Examples

#### 31.2.1) GC_TakeDamage (BurstLatent)

| Property | Value |
|----------|-------|
| Parent Class | GameplayCueNotify_BurstLatent |
| Gameplay Cue Tag | GameplayCue.TakeDamage |
| Auto Destroy on Remove | TRUE |
| Num Preallocated Instances | 3 |

Custom Variables:

| Variable | Type | Value |
|----------|------|-------|
| StumbleLeftTag | GameplayTag | Narrative.Anim.AnimSets.Flinch.Left |
| StumbleBackTag | GameplayTag | Narrative.Anim.AnimSets.Flinch.Back |
| StumbleRightTag | GameplayTag | Narrative.Anim.AnimSets.Flinch.Right |
| StumbleForwardTag | GameplayTag | Narrative.Anim.AnimSets.Flinch.Forward |

GCN Effects Configuration:

| Effect Type | Value |
|-------------|-------|
| Burst Particles | 1 element - NS_ImpactBlood (Niagara System) |
| Burst Sounds | 0 elements |
| Burst Camera Shake | None |
| Burst Decal | None |

Event On Burst Flow:

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event On Burst | Entry point with Target, Parameters, Spawn Results |
| 2 | Parent: On Burst | Calls parent implementation first |
| 3 | Cast To NarrativeCharacter | Type-safe access to target |
| 4 | Get Hit Result | Extracts FHitResult from Parameters |
| 5 | Break Hit Result | Decomposes Location, Impact Point, Normal |
| 6 | Normalize (Location - Impact Point) | Calculate hit direction vector |
| 7 | Select Hit Montage | Custom function selecting montage by direction |
| 8 | Is Alive | Validates target alive before animation |
| 9 | Is Locally Viewed | Checks if local player pawn |
| 10 | Play Anim Montage (Multicast Reliable) | Network-synced animation playback |

Reliable Multicast Pattern:

| Property | Value |
|----------|-------|
| Custom Event Name | Play Anim Montage |
| Replicates | Multicast |
| Reliable | TRUE |
| Inputs | Target (Character), AnimMontage (Anim Montage) |

#### 31.2.2) GC_TakeDamage_Stumble (Inheritance Pattern)

| Property | Value |
|----------|-------|
| Parent Class | GC_TakeDamage (NOT GameplayCueNotify_BurstLatent) |
| Gameplay Cue Tag | GameplayCue.TakeDamage.Stumble |
| Custom Variables | None (inherits parent) |
| Custom Functions | None (inherits parent) |

Tag Variant Pattern:

| Trigger Source | Cue Tag |
|----------------|---------|
| GE_LightDamage | GameplayCue.TakeDamage |
| GE_HeavyDamage | GameplayCue.TakeDamage.Stumble |

#### 31.2.3) GC_WeaponFire (Burst)

| Property | Value |
|----------|-------|
| Parent Class | GameplayCueNotify_Burst |
| Gameplay Cue Tag | GameplayCue.Weapon.Fire |
| Is Override | FALSE |
| Can Do Latent Actions | NO (CDO only) |

### 31.3) Camera Shake System

#### 31.3.1) Pattern Types

| Pattern | Class | Use Case |
|---------|-------|----------|
| Perlin Noise | PerlinNoiseCameraShakePattern | Organic continuous effects (handheld, breathing) |
| Wave Oscillator | WaveOscillatorCameraShakePattern | Mechanical one-shot impacts (recoil, explosions) |

#### 31.3.2) Narrative Pro Camera Shake Assets

BP_HandheldCameraShake:

| Property | Value |
|----------|-------|
| Single Instance | TRUE |
| Root Shake Pattern | Perlin Noise Camera Shake Pattern |
| Location Amplitude Multiplier | 1.0 |
| Location Frequency Multiplier | 1.0 |
| Rotation Amplitude Multiplier | 0.6 |
| Rotation Frequency Multiplier | 0.3 |
| FOV Amplitude | 0.0 |
| Duration | 0.0 (infinite/looping) |
| Blend In Time | 0.2 |
| Blend Out Time | 0.2 |

BP_PistolCameraShake / BP_RifleCameraShake:

| Property | Value |
|----------|-------|
| Single Instance | FALSE |
| Root Shake Pattern | Wave Oscillator Camera Shake Pattern |
| Location X Amplitude | 2.0 |
| Location X Frequency | 1.0 |
| Location X Initial Offset Type | Zero |
| Location Y/Z | 0.0 (disabled) |
| Rotation | All 0.0 (disabled) |
| FOV Amplitude | 1.0 |
| FOV Frequency | 1.0 |
| FOV Initial Offset Type | Zero |
| Duration | 0.1 |
| Blend In/Out Time | 0.0 |

#### 31.3.3) Single Instance Setting

| Value | Behavior |
|-------|----------|
| TRUE | Only one instance plays at a time (prevents stacking) |
| FALSE | Multiple can stack (rapid fire stacks recoil) |

### 31.4) GCN Effects Configuration

#### 31.4.1) Default Spawn Condition

| Property | Purpose |
|----------|---------|
| Locally Controlled Source | Instigator Actor or Target Actor |
| Locally Controlled Policy | Always, Only Local, Only Not Local |
| Chance to Play | 0.0 - 1.0 spawn probability |
| Allowed Surface Types | Physical material filter (include) |
| Rejected Surface Types | Physical material filter (exclude) |

#### 31.4.2) Default Placement Info

| Property | Purpose |
|----------|---------|
| Socket Name | Attachment socket (or None) |
| Attach Policy | Do Not Attach, Attach to Owner |
| Attachment Rule | Keep World, Snap to Target |
| Scale Override | Enable/disable with Vector3 |

#### 31.4.3) Burst Effects Arrays

| Array | Content Type |
|-------|--------------|
| Burst Particles | Niagara System references |
| Burst Sounds | Sound Cue / Sound Wave references |
| Burst Camera Shake | CameraShakeBase reference + Play Space |
| Burst Camera Lens Effect | Post-process material |
| Burst Force Feedback | Controller vibration pattern |
| Burst Decal | Material + Size + Fade settings |

### 31.5) Gameplay Cue Properties

| Property | Type | Purpose |
|----------|------|---------|
| Gameplay Cue Tag | GameplayTag | Must start with "GameplayCue." |
| Is Override | Boolean | TRUE prevents parent tag cues from firing |
| Auto Attach to Owner | Boolean | Attach spawned actor to target |
| Unique Instance Per Instigator | Boolean | Separate instance per damage source |
| Unique Instance Per Source Object | Boolean | Separate instance per weapon/item |
| Allow Multiple On Burst Events | Boolean | Allow multiple simultaneous triggers |
| Num Preallocated Instances | Integer | Object pool size (performance) |
| Auto Destroy on Remove | Boolean | Cleanup when cue removed |
| Auto Destroy Delay | Float | Seconds before destruction |

### 31.6) Triggering Methods

#### 31.6.1) From GameplayEffect

| Location | Property |
|----------|----------|
| GE Blueprint | Display -> Gameplay Cue Tags array |

Effect Duration to Event Mapping:

| Duration Policy | Events Triggered |
|-----------------|------------------|
| Instant | OnBurst / Execute only |
| Has Duration | OnActive -> WhileActive -> OnRemove |
| Infinite | OnActive -> WhileActive (until manual remove) |

#### 31.6.2) From Ability (Blueprint)

| Node | Purpose |
|------|---------|
| Execute Gameplay Cue | One-time burst |
| Add Gameplay Cue to Owner | Start persistent cue |
| Remove Gameplay Cue from Owner | End persistent cue |
| Execute Gameplay Cue Local | Local-only (no network) |

#### 31.6.3) GameplayCueParameters

| Property | Content |
|----------|---------|
| NormalizedMagnitude | 0-1 for scaling effects |
| RawMagnitude | Actual damage value |
| Location | World position |
| Normal | Surface normal |
| PhysicalMaterial | Surface type for material-based effects |
| EffectContext | Full context including hit results |

### 31.7) Replication Behavior

| Behavior | Description |
|----------|-------------|
| Default | Unreliable multicast RPCs |
| Bandwidth | Efficient but not guaranteed delivery |
| Limit | Maximum 2 identical cue RPCs per net update |
| Server | Cues never triggered on dedicated server by default |

ASC Replication Mode Impact:

| Mode | Cue Behavior |
|------|--------------|
| Full | Tags and Cues replicate to all clients |
| Mixed | Tags and Cues replicate to all clients |
| Minimal | Tags and Cues replicate (no GE data) |

Reliable Workaround Pattern:

| Step | Implementation |
|------|----------------|
| 1 | Create Custom Event in BurstLatent cue |
| 2 | Set Replicates: Multicast |
| 3 | Set Reliable: TRUE |
| 4 | Call from OnBurst for guaranteed delivery |

### 31.8) GameplayCueManager Configuration

DefaultGame.ini paths (required for performance):

| Setting | Purpose |
|---------|---------|
| +GameplayCueNotifyPaths=/Game/Effects/GameplayCues | Scan directory for cues |
| +GameplayCueNotifyPaths=/Game/Abilities/Cues | Additional scan directory |

Without configuration, manager scans entire /Game directory (slow on large projects).

### 31.9) Father Companion Cue Recommendations

| Cue | Parent Class | Trigger | Purpose |
|-----|--------------|---------|---------|
| GC_FatherFormTransition | BurstLatent | Form ability activation | 5s Niagara VFX during form change |
| GC_FatherTakeDamage | BurstLatent | Father receives damage | Hit reaction, flash effect |
| GC_FatherAttack | Burst | GA_FatherAttack hit | Impact VFX at hit location |
| GC_ProximityStrike | Looping | Symbiote proximity damage | Pulsing damage aura VFX |
| GC_ElectricTrap | Looping | Trap active | Electric field particles |
| GC_TurretFire | Burst | GA_TurretShoot | Muzzle flash, tracer |
| GC_FatherDeath | BurstLatent | Father dies | Death VFX, dissolve effect |

Father Cue Tag Hierarchy:

| Tag | Purpose |
|-----|---------|
| GameplayCue.Father.TakeDamage | Base father damage cue |
| GameplayCue.Father.FormTransition | Form change VFX |
| GameplayCue.Father.Attack.Hit | Attack impact |
| GameplayCue.Father.ProximityStrike | Symbiote aura pulse |
| GameplayCue.Father.ElectricTrap | Engineer trap field |
| GameplayCue.Father.TurretFire | Engineer turret muzzle |
| GameplayCue.Father.Death | Death effect |

Configuration Pattern for Father Cues:

| Setting | Recommended Value |
|---------|-------------------|
| Locally Controlled Policy | Always (all clients see effects) |
| Chance to Play | 1.0 (guaranteed) |
| Attach Policy | Do Not Attach (world-space) |
| Auto Destroy on Remove | TRUE |
| Num Preallocated Instances | 3 |
| Allow Multiple On Burst Events | TRUE |

---

## SECTION 32: BLACKBOARD SYSTEM

### 32.1) Blackboard Overview

Narrative Pro uses Blackboard assets to store runtime data for Behavior Trees. Each BB_ asset defines keys that activities and behavior trees read/write.

### 32.2) BB_Attack (Base Combat Blackboard)

| Key | Type | Color | Purpose |
|-----|------|-------|---------|
| TargetLocation | Vector | Orange | World position of attack target |
| AttackTarget | Object (Actor) | Blue | Reference to enemy actor |
| TargetRotation | Rotator | Yellow | Facing direction toward target |
| SelfActor | Object (Actor) | Blue | Reference to owning NPC |

### 32.3) BB_Attack_Cover (Inherits BB_Attack)

| Key | Type | Color | Purpose |
|-----|------|-------|---------|
| (Inherited) | - | - | All keys from BB_Attack |
| Cover | Object | Blue | Cover actor to use during combat |

### 32.4) BB_Attack_Grenade (Inherits BB_Attack)

| Key | Type | Color | Purpose |
|-----|------|-------|---------|
| (Inherited) | - | - | All keys from BB_Attack |

Uses same keys as BB_Attack for grenade throwing behavior.

### 32.5) BB_Attack_Strafe (Inherits BB_Attack)

| Key | Type | Color | Purpose |
|-----|------|-------|---------|
| (Inherited) | - | - | All keys from BB_Attack |
| AttackRangeMin | Float | Blue | Minimum distance to maintain from target |
| AttackRangeMax | Float | Blue | Maximum distance to maintain from target |

### 32.6) BB_Flee

| Key | Type | Color | Purpose |
|-----|------|-------|---------|
| SelfActor | Object (Actor) | Blue | Reference to owning NPC |
| FleeTarget | Object (Actor) | Blue | Actor to flee from |
| TargetLocation | Vector | Orange | Destination to flee toward |
| TargetRotation | Rotator | Yellow | Facing direction |

### 32.7) BB_FollowCharacter

| Key | Type | Color | Purpose |
|-----|------|-------|---------|
| SelfActor | Object (Actor) | Blue | Reference to owning NPC |
| FollowTarget | Object (Actor) | Blue | Character to follow |

This blackboard is used by Goal Follow Character. The FollowTarget key is set automatically when the goal is assigned.

### 32.8) BB_GoToLocation

| Key | Type | Color | Purpose |
|-----|------|-------|---------|
| SelfActor | Object (Actor) | Blue | Reference to owning NPC |
| TargetLocation | Vector | Orange | Destination position |
| WithinPlayerDistance | Float | Orange | Maximum distance from player |
| TargetRotation | Rotator | Yellow | Facing direction at destination |
| Delay | Float | Blue | Wait time at destination |

### 32.9) BB_Interact (Inherits BB_GoToLocation)

| Key | Type | Color | Purpose |
|-----|------|-------|---------|
| (Inherited) | - | - | All keys from BB_GoToLocation |
| InteractableActor | Object (Actor) | Blue | Actor to interact with |
| InteractableComponent | Object (Component) | Blue | Specific component to interact with |
| ClaimedSlotIdx | Integer | Blue | Interaction slot index |

### 32.10) BB_ReturnToSpawn

| Key | Type | Color | Purpose |
|-----|------|-------|---------|
| SelfActor | Object (Actor) | Blue | Reference to owning NPC |
| PlayerPawn | Object (Actor) | Blue | Reference to player |
| TargetLocation | Vector | Orange | Spawn location to return to |
| Initialized | Boolean | Green | Whether behavior has been initialized |
| TargetRotation | Rotator | Yellow | Facing direction at spawn |
| SpawnLoaded | Boolean | Red | Whether spawn data has loaded |

### 32.11) BB_DriveToDestination

| Key | Type | Color | Purpose |
|-----|------|-------|---------|
| SelfActor | Object (Actor) | Blue | Reference to owning NPC |
| TargetLocation | Vector | Orange | Destination position |
| WithinPlayerDistance | Float | Orange | Maximum distance from player |
| TargetRotation | Rotator | Yellow | Facing direction |
| Delay | Float | Blue | Wait time |
| InteractableActor | Object (Actor) | Blue | Vehicle actor |
| InteractableComponent | Object (Component) | Blue | Vehicle component |
| ClaimedSlotIdx | Integer | Blue | Vehicle seat index |
| InVehicle? | Boolean | Green | Whether NPC is in vehicle |
| VehicleDriveDestination | Vector | Orange | Final drive destination |
| WithinWalkingRange | Boolean | Blue | Whether destination is walkable |
| PassengersInVehicle? | Boolean | Green | Whether passengers are present |

### 32.12) Blackboard Inheritance Pattern

| Parent | Children |
|--------|----------|
| BB_Attack | BB_Attack_Cover, BB_Attack_Grenade, BB_Attack_Strafe |
| BB_GoToLocation | BB_Interact, BB_DriveToDestination |

### 32.13) Father Blackboard Selection

| Father Behavior | Recommended Blackboard |
|-----------------|------------------------|
| Crawler Following | BB_FollowCharacter |
| Crawler Combat | BB_Attack |
| Engineer Turret | BB_Attack_Strafe (stationary with range) |
| Return After Detach | BB_GoToLocation |

---

## SECTION 33: DIALOGUE-DRIVEN NPC CONTROL

### 33.1) Dialogue Blueprint Overview

Narrative Pro allows NPC control through dialogue events. When player talks to NPC, dialogue nodes can trigger AI goal assignments.

### 33.2) Dialogue Asset Structure

| Asset Type | Prefix | Purpose |
|------------|--------|---------|
| DialogueBlueprint | DBP_ | Dialogue flow graph |
| DialogueAsset | DA_ | Dialogue data |
| CharacterDefinition | CD_ | Character identity for dialogue |

### 33.3) AI: Add Goal To NPC Event

Dialogue nodes can contain events that assign goals to NPCs.

| Property | Location | Purpose |
|----------|----------|---------|
| Events | Dialogue Node Details | Array of events to fire |
| Event Type | Event dropdown | Select "AI: Add Goal To NPC" |

### 33.4) AI: Add Goal To NPC Configuration

| Section | Property | Value | Purpose |
|---------|----------|-------|---------|
| Event | Refire on Load | Checked | Restores goal after save/load |
| Event | Event Runtime | Start | When event fires |
| Event | Event Filter | Only NPCs | Who receives event |
| Event | NPCTargets | 0 Array element | Empty = apply to dialogue owner |
| Parties | Party Event Policy | Party | Which party receives |
| Events and Conditions | Conditions | 0 Array element | Optional conditions |
| NPC Activity | Goal to Add | Goal Follow Character | Goal type |
| Config | Target to Follow | CD_DefaultPlayer | CharacterDefinition reference |
| Config | Should Mount | Checked/Unchecked | Vehicle mounting option |
| NPC Goal | Remove on Success | Unchecked | Keep goal active |
| NPC Goal | Default Score | 1.0 | Goal priority |
| NPC Goal | Save Goal | Unchecked | Persistence option |
| NPC Goal | Goal Lifetime | -1.0 | Infinite duration (-1) |

### 33.5) Goal Follow Character

Native Narrative Pro goal for NPC following behavior.

| Property | Type | Purpose |
|----------|------|---------|
| Target to Follow | CharacterDefinition | Who to follow (CD_DefaultPlayer) |
| Should Mount | Boolean | Whether to mount vehicles |
| Goal Lifetime | Float | -1 for infinite |

### 33.6) CharacterDefinition Reference

| CD_ Asset | Purpose |
|-----------|---------|
| CD_DefaultPlayer | Player character definition |
| CD_FatherCompanion | Father companion definition |

When Goal Follow Character uses CD_DefaultPlayer, the NPC follows the player using native AI behavior.

### 33.7) Dialogue Flow for Father Following

| Step | Node Type | Content |
|------|-----------|---------|
| 1 | NPC Line | "Hi there, I am spidey!" |
| 2 | Player Reply | "Hi Spidey! Wanna Team Up?" |
| 3 | NPC Line + Event | "Sure, that will be great!" + AI: Add Goal To NPC |
| 4 | Player Reply | "Let's Go" |

### 33.8) Goal System vs Manual Reference

| System | Controls | How |
|--------|----------|-----|
| Goal Follow Character | AI following behavior | Blackboard FollowTarget, pathfinding, distance |
| OwnerPlayer Variable | Direct actor reference | Socket attachment, effect application, ability granting |

Both systems can coexist - dialogue triggers following behavior while OwnerPlayer enables gameplay mechanics.

### 33.9) Dialogue Event Timing

| Event Runtime | When Fires |
|---------------|------------|
| Start | When dialogue line starts |
| End | When dialogue line ends |
| Select | When player selects option |

### 33.10) Save/Load Considerations

| Property | Setting | Effect |
|----------|---------|--------|
| Refire on Load | Checked | Goal restored after save/load |
| Save Goal | Unchecked | Goal not saved independently |
| Goal Lifetime | -1.0 | Goal persists until removed |

---

## SECTION 34: FATHER-PLAYER REFERENCE ARCHITECTURE

### 34.1) Two Reference Systems

The father companion uses two parallel systems for player interaction:

| System | Purpose | Mechanism |
|--------|---------|-----------|
| Goal System | AI Behavior | Dialogue-driven Goal Follow Character |
| OwnerPlayer Variable | Direct Reference | Blueprint variable on father |

### 34.2) Goal System Responsibilities

| Behavior | Controlled By |
|----------|---------------|
| Following player in Crawler form | Goal Follow Character |
| Pathfinding to player | Native AI navigation |
| Maintaining follow distance | BB_FollowCharacter configuration |
| Obstacle avoidance | Native AI system |
| Speed matching | Native AI movement |

### 34.3) OwnerPlayer Variable Responsibilities

| Feature | Requires OwnerPlayer |
|---------|----------------------|
| Socket attachment (Armor/Exoskeleton) | Get player mesh, attach to socket |
| Effect application to player | Get player ASC, apply GE |
| Ability granting to player | Get player ASC, grant ability |
| Dome positioning | Get player location |
| Symbiote merge visual | Get player mesh reference |
| Bidirectional reference setup | Player needs FatherCompanionRef |

### 34.4) InitOwnerReference Event Graph

Father's BeginPlay or custom event establishes bidirectional references:

| Step | Node | Purpose |
|------|------|---------|
| 1 | InitOwnerReference (Custom Event, Executes On Server) | Server authority |
| 2 | Get Player Controller (Index 0) | Get local player controller |
| 3 | Get Controlled Pawn | Get player character |
| 4 | Is Valid | Validate pawn exists |
| 5 | Cast To BP_NarrativePlayer | Cast to player class |
| 6 | SET OwnerPlayer | Father stores player reference |
| 7 | SET FatherCompanionRef (Target: OwnerPlayer) | Player stores father reference |

### 34.5) Multiplayer Considerations

| Aspect | Solution |
|--------|----------|
| Player Index 0 | Only works for host - use spawning player controller instead |
| Server Authority | InitOwnerReference runs on server |
| Reference Replication | OwnerPlayer variable must be Replicated |
| Bidirectional Setup | Both father and player need replicated references |

### 34.6) Father Variables Summary

| Variable | Type | Replicated | Purpose |
|----------|------|------------|---------|
| OwnerPlayer | BP_NarrativePlayer Ref | Yes | Direct player reference for abilities |
| CurrentForm | E_FatherForm | Yes | Track current form state |
| IsAttached | Boolean | Yes | Attached vs detached state |
| Effect Handles | ActiveGameplayEffectHandle | No | For GE removal on form change |
| MarkedEnemies | Array (Actor) | Yes | GA_FatherMark tracking |

### 34.7) Variables NOT Needed (Goal System Handles)

| Not Needed | Reason |
|------------|--------|
| FollowTarget (for AI) | Goal Follow Character uses BB_FollowCharacter |
| FollowDistance | Goal configuration handles |
| IsFollowing | Goal system tracks |
| Custom follow behavior tree | Native Goal Follow Character |

### 34.8) Form-Specific Reference Usage

| Form | Uses OwnerPlayer For |
|------|----------------------|
| Crawler | Marking targets near player |
| Armor | Attach to FatherChestSocket, apply GE_ArmorBoost to player |
| Exoskeleton | Attach to FatherBackSocket, apply GE_ExoskeletonSpeed to player |
| Symbiote | Apply GE_SymbioteBoost to player, merge visual |
| Engineer | N/A (turret mode, no player attachment) |

### 34.9) Ability Application Pattern

Form abilities apply effects TO the player, not the father:

| Step | Action |
|------|--------|
| 1 | GA_FatherArmor activates on father's ASC |
| 2 | Get OwnerPlayer reference |
| 3 | Get OwnerPlayer's AbilitySystemComponent |
| 4 | Apply GE_ArmorBoost to OwnerPlayer's ASC |
| 5 | Store handle for later removal |

### 34.10) Reference Setup Timing

| Event | What Happens |
|-------|--------------|
| Father Spawned (NPCSpawner) | Father exists in world |
| Dialogue Triggered | Player talks to father |
| Goal Assigned | Father starts following (AI behavior) |
| InitOwnerReference Called | Bidirectional references established |
| Form Abilities Usable | Can now attach, apply effects |

### 34.11) Architecture Diagram (Text)

| Layer | System | Data Flow |
|-------|--------|-----------|
| AI Behavior | Goal Follow Character | Dialogue -> Goal -> BB_FollowCharacter -> AI Movement |
| Direct Reference | OwnerPlayer | Father -> OwnerPlayer -> Player ASC -> Effects/Abilities |

### 34.12) CharacterDefinition Persistence Pattern

Goal Follow Character uses CharacterDefinition for save/load persistence:

| Variable | Type | Purpose |
|----------|------|---------|
| TargetToFollowAsset | Character Definition | Save/Load persistence reference |
| TargetToFollow | Narrative Character | Runtime actor reference |

Father should mirror this pattern:

| Variable | Type | Purpose |
|----------|------|---------|
| OwnerPlayerDefinition | Character Definition | Save/Load persistence reference |
| OwnerPlayer | Narrative Character | Runtime actor reference |

### 34.13) PrepareForSave Event

Father implements PrepareForSave to store CharacterDefinition before save:

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event PrepareForSave | Called before game saves |
| 2 | Is Valid (OwnerPlayer) | Check player reference exists |
| 3 | Get Character Definition (Target: OwnerPlayer) | Get CD_ asset |
| 4 | SET OwnerPlayerDefinition | Store for persistence |

### 34.14) Initialize Event

Father implements Initialize to restore references after load:

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event Initialize | Called after game loads |
| 2 | Is Valid (OwnerPlayerDefinition) | Check saved definition exists |
| 3 | Get Narrative Character Subsystem | Access character lookup |
| 4 | Find Character (OwnerPlayerDefinition) | Get actor from CD_ |
| 5 | Is Valid (result) | Validate found actor |
| 6 | SET OwnerPlayer | Restore runtime reference |
| 7 | Cast To BP_NarrativePlayer | Access player class |
| 8 | SET FatherCompanionRef (Target: Player) | Restore bidirectional |
| 9 | Apply Gameplay Effect to Self (GE_GrantRecruitedTag) | Restore Father.State.Recruited tag |

Step 9 is critical - without reapplying GE_GrantRecruitedTag, form abilities will fail activation because Father.State.Recruited tag won't exist after load.

### 34.15) Complete Save/Load Flow

| Phase | Event | Action | Result |
|-------|-------|--------|--------|
| Save | PrepareForSave | Store OwnerPlayerDefinition | Ownership persisted |
| Save | Auto | CurrentForm, IsAttached saved | Form state persisted |
| Load | Initialize | Find Character from CD_ | OwnerPlayer restored |
| Load | Initialize | Set FatherCompanionRef | Bidirectional reference |
| Load | Initialize | Apply GE_GrantRecruitedTag | Tag requirement restored |
| Load | Load Override | Switch on CurrentForm | Form ability activated |
| Load | Form Ability | Attach, apply stat GE | Player buffs restored |

### 34.16) Updated Father Variables Summary

| Variable | Type | Replicated | Rep Condition | SaveGame | Purpose |
|----------|------|------------|---------------|----------|---------|
| OwnerPlayer | NarrativeCharacter Ref | Yes | Initial Only | No | Runtime player reference |
| OwnerPlayerDefinition | Character Definition | No | N/A | No | Save/Load persistence |
| CurrentForm | E_FatherForm | Yes (RepNotify) | None | Yes | Form state |
| IsAttached | Boolean | Yes (RepNotify) | None | Yes | Attachment state |
| IsDeployed | Boolean | Yes | None | Yes | Turret mode state |
| DeployedTransform | Transform | Yes | None | Yes | Engineer turret position |
| ChestSocketName | Name | No | N/A | No | Armor attachment socket |
| BackSocketName | Name | No | N/A | No | Exoskeleton attachment socket |
| MarkedEnemies | Array (Actor) | Yes | None | No | GA_FatherMark tracking |
| OriginalWalkSpeed | Float | No | N/A | No | Speed restoration |
| OriginalJumpVelocity | Float | No | N/A | No | Jump restoration |
| CrawlerStateHandle | ActiveGameplayEffectHandle | No | N/A | No | Effect removal |
| ArmorStateHandle | ActiveGameplayEffectHandle | No | N/A | No | Effect removal |
| ArmorBoostHandle | ActiveGameplayEffectHandle | No | N/A | No | Effect removal |
| ExoskeletonStateHandle | ActiveGameplayEffectHandle | No | N/A | No | Effect removal |
| ExoskeletonSpeedHandle | ActiveGameplayEffectHandle | No | N/A | No | Effect removal |
| SymbioteStateHandle | ActiveGameplayEffectHandle | No | N/A | No | Effect removal |
| SymbioteBoostHandle | ActiveGameplayEffectHandle | No | N/A | No | Effect removal |
| EngineerStateHandle | ActiveGameplayEffectHandle | No | N/A | No | Effect removal |
| FormCooldownHandle | ActiveGameplayEffectHandle | No | N/A | No | Cooldown removal |
| InvulnerableHandle | ActiveGameplayEffectHandle | No | N/A | No | Invulnerability removal |
| bIsFirstActivation | Boolean | No | N/A | No | Spawn vs form switch |

---

## SECTION 35: NARRATIVEEVENT SYSTEM

### 35.1) NarrativeEvent Overview

NarrativeEvent is a Blueprint class for creating reusable events that can be triggered from dialogues, quests, or other systems.

| Property | Location |
|----------|----------|
| Parent Class | NarrativeEvent (NarrativeArsenal module) |
| Asset Prefix | NE_ |
| Usage | Dialogue Events array, Quest Events |

### 35.2) ExecuteEvent Function Signature

Override ExecuteEvent to implement custom event logic:

| Parameter | Type | Purpose |
|-----------|------|---------|
| Target | APawn* | The pawn receiving the event (NPC in dialogue) |
| Controller | APlayerController* | The player controller triggering the event |
| NarrativeComponent | UTalesComponent* | The component managing dialogue/quest |

### 35.3) NE_SetFatherOwner Configuration

| Property | Value | Purpose |
|----------|-------|---------|
| Event Filter | Only NPCs | Fires only on NPC targets |
| Refire on Load | Unchecked | Persistence via save system instead |
| Event Runtime | Start | Fires when event begins |

### 35.4) NE_SetFatherOwner ExecuteEvent Implementation

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event ExecuteEvent | Override function entry |
| 2 | Cast To BP_FatherCompanion (Target) | Validate father |
| 3 | From Controller: Get Controlled Pawn | Get player character |
| 4 | Cast To BP_NarrativePlayer | Validate player type |
| 5 | Call InitOwnerReference (Target: Father, Input: Player) | Set references |

### 35.5) InitOwnerReference Function (Father)

| Step | Node | Purpose |
|------|------|---------|
| 1 | Function Entry (Input: NewOwner - NarrativeCharacter) | Public function |
| 2 | Is Valid (NewOwner) | Validate input |
| 3 | SET OwnerPlayer (Value: NewOwner) | Store reference on father |
| 4 | Cast To BP_NarrativePlayer | Access player class |
| 5 | SET FatherCompanionRef (Target: Player, Value: Self) | Bidirectional reference |
| 6 | Get Ability System Component (Self) | Access father ASC |
| 7 | Apply Gameplay Effect to Self (GE_GrantRecruitedTag) | Grant Father.State.Recruited |

Note: GA_Backstab grant removed in v5.0 - Backstab is now a Player Default Ability.

### 35.6) NE_ClearFatherOwner Configuration

| Property | Value | Purpose |
|----------|-------|---------|
| Event Filter | Only NPCs | Fires only on NPC targets |
| Refire on Load | Unchecked | N/A for clear operation |
| Event Runtime | Start | Fires when event begins |

### 35.7) NE_ClearFatherOwner ExecuteEvent Implementation

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event ExecuteEvent | Override function entry |
| 2 | Cast To BP_FatherCompanion (Target) | Validate father |
| 3 | Call ClearOwnerReference (Target: Father) | Clear references |

### 35.8) ClearOwnerReference Function (Father)

| Step | Node | Purpose |
|------|------|---------|
| 1 | Function Entry | Public function |
| 2 | Is Valid (OwnerPlayer) | Check reference exists |
| 3 | Cast To BP_NarrativePlayer | Access player class |
| 4 | SET FatherCompanionRef (Target: Player, Value: NULL) | Clear player reference |
| 5 | Remove stat boost GEs from player | Clear effects |
| 6 | SET OwnerPlayer (NULL) | Clear father reference |
| 7 | SET CurrentForm (Crawler) | Reset to default form |
| 8 | SET IsAttached (False) | Reset attachment state |
| 9 | Get Ability System Component (Self) | Access father ASC |
| 10 | Remove Loose Gameplay Tag (Father.State.Recruited) | Remove recruited tag |

Note: BackstabAbilityHandle cleanup removed in v5.0 - GA_Backstab is now a Player Default Ability.

### 35.9) Dialogue Integration

| Dialogue Node | Events |
|---------------|--------|
| Recruitment Node | AI: Add Goal To NPC, NE_SetFatherOwner |
| Dismiss Node | NE_ClearFatherOwner, AI: Remove All Goals |

### 35.10) Father Lifecycle States

| State | OwnerPlayer | Goal | Forms Enabled |
|-------|-------------|------|---------------|
| Wild | NULL | None | No |
| Recruited | Valid | Active | Yes |
| Dismissed | NULL | None | No (returns to Wild) |

---

## SECTION 36: CROSS-ACTOR ABILITY GRANTING

### 36.1) Critical Limitation - Tag Cancellation Does NOT Work Cross-Actor

Cancel Abilities with Tag only operates on the SAME ASC. Father form changes CANNOT cancel abilities on player ASC via tags.

| Method | Works Cross-Actor |
|--------|-------------------|
| Cancel Abilities with Tag | NO - same ASC only |
| ClearAbility(Handle) on Player ASC | YES |
| Remove Active Gameplay Effect(Handle) on Player ASC | YES |

Source code confirmation - CancelAbilities iterates only own ASC:

| Line | Code |
|------|------|
| 1 | for (FGameplayAbilitySpec and Spec : ActivatableAbilities.Items) |
| 2 | // Only iterates abilities on THIS component |

This means form abilities MUST manually cleanup abilities they granted to player in EndAbility.

### 36.2) SourceObject Pattern

When granting abilities from father to player, father becomes the SourceObject:

| Concept | Explanation |
|---------|-------------|
| SourceObject | The actor that granted the ability (father) |
| Owner | The ASC that owns the ability spec (player) |
| Avatar | The actor executing the ability (player) |

### 36.3) Granting Abilities to Player

| Step | Action |
|------|--------|
| 1 | Get OwnerPlayer -> Ability System Component |
| 2 | Create FGameplayAbilitySpec |
| 3 | Set SourceObject = Self (father) |
| 4 | Give Ability (spec) on player ASC |
| 5 | Store returned FGameplayAbilitySpecHandle |

### 36.4) Server Authority for Granting

| Rule | Implementation |
|------|----------------|
| Grant only on server | Has Authority check before granting |
| Replication automatic | GiveAbility replicates spec to client |
| Never grant on client | Client grants do not replicate |

### 36.5) Net Execution Policy for Companion Abilities

| Policy | When to Use |
|--------|-------------|
| ServerInitiated | Player needs visual feedback from companion ability |
| ServerOnly | Pure buff/debuff, no player visuals needed |
| NOT LocalPredicted | Companion-granted abilities should not predict |

Why not LocalPredicted:

| Issue | Explanation |
|-------|-------------|
| Prediction key source | Prediction requires client-initiated key |
| Companion abilities | Server initiates, no client prediction key |
| Desync risk | Client cannot predict server-initiated abilities |

### 36.6) Handle Storage Strategies

| Strategy | When to Use | Implementation |
|----------|-------------|----------------|
| Local ability variable | Single ability manages effect | Store handle in GA_ variable |
| Father variable | Cross-ability cleanup needed | Store handle in BP_FatherCompanion |
| TMap per form | Multiple forms with different effects | TMap<EFatherForm, HandleArray> |

### 36.7) FFatherFormGrantedHandles Pattern (Lyra-inspired)

| Field | Type | Purpose |
|-------|------|---------|
| AbilityHandles | Array (FGameplayAbilitySpecHandle) | Granted ability tracking |
| EffectHandles | Array (FActiveGameplayEffectHandle) | Applied effect tracking |

TakeFromAbilitySystem method:

| Step | Action |
|------|--------|
| 1 | For each AbilityHandle: Clear Ability |
| 2 | For each EffectHandle: Remove Active Gameplay Effect |
| 3 | Reset arrays |

### 36.8) Weak Pointer Validation

Father reference in abilities should be validated:

| Location | Validation |
|----------|------------|
| CanActivateAbility | Check SourceObject Is Valid |
| ActivateAbility | Cache weak pointer, validate before use |
| AbilityTask callbacks | Re-validate before executing logic |

### 36.9) Ability Cleanup on Father Destruction

BeginDestroy pattern for emergency cleanup:

| Step | Action |
|------|--------|
| 1 | Get all ASCs that received abilities from this father |
| 2 | Find specs where SourceObject == Self |
| 3 | Cancel active abilities |
| 4 | Clear ability specs |

### 36.10) Orphaned Abilities Warning

GAS does NOT automatically remove abilities when their SourceObject is destroyed:

| Issue | Consequence |
|-------|-------------|
| SourceObject becomes NULL | Ability persists on player ASC |
| No auto-cleanup | Orphaned ability remains indefinitely |
| Potential crash | Ability code accessing NULL SourceObject |
| Logic errors | Abilities require source for costs/validation |

SourceObject is stored as TWeakObjectPtr - becomes invalid on destroy but ability spec remains.

### 36.11) Ability Removal Functions Comparison

| Function | Active Ability Behavior | Spec Removal |
|----------|------------------------|--------------|
| ClearAbility(Handle) | Does NOT cancel - removes spec unsafely | Immediate |
| CancelAbility(Handle) | Stops immediately, fires OnCancelled | Spec REMAINS |
| SetRemoveAbilityOnEnd(Handle) | Continues until natural end | After EndAbility |

Correct two-step pattern for form EndAbility cleanup:

| Step | Function | Purpose |
|------|----------|---------|
| 1 | Cancel Ability (Handle) | Stop execution, fire cleanup delegates, stop montages |
| 2 | Set Remove Ability On End (Handle) | Safely remove spec after cleanup completes |

Effects of Cancel Ability:

| Aspect | Behavior |
|--------|----------|
| Montages | Stop immediately |
| GameplayCues | End immediately |
| AbilityTasks | ExternalCancel() called |
| EndAbility | Called with bWasCancelled = true |
| Cooldowns | Already committed cooldowns persist |

SetRemoveAbilityOnEnd alone is insufficient - ability continues until natural end.

### 36.12) Ability Handle Variables for Granted Sub-Abilities

Variables needed on BP_FatherCompanion (separate from effect handles):

| Variable | Type | Stores Handle For |
|----------|------|-------------------|
| DomeAbilityHandle | GameplayAbilitySpecHandle | GA_ProtectiveDome on player |
| DomeBurstAbilityHandle | GameplayAbilitySpecHandle | GA_DomeBurst on player |
| DashAbilityHandle | GameplayAbilitySpecHandle | GA_FatherExoskeletonDash on player |
| SprintAbilityHandle | GameplayAbilitySpecHandle | GA_FatherExoskeletonSprint on player |
| StealthAbilityHandle | GameplayAbilitySpecHandle | GA_StealthField on player |
| ProximityAbilityHandle | GameplayAbilitySpecHandle | GA_ProximityStrike on player |

Note: BackstabAbilityHandle removed in v5.0 - GA_Backstab is now a Player Default Ability.

These are ABILITY handles (FGameplayAbilitySpecHandle), not EFFECT handles (FActiveGameplayEffectHandle).

### 36.13) Form EndAbility Cleanup Flow

**CRITICAL: All cleanup must be gated by bWasCancelled check (see Section 11.9)**

Each form ability's Event EndAbility must:
1. Check bWasCancelled output pin
2. Only run cleanup when bWasCancelled = TRUE (form switch)
3. Skip cleanup when bWasCancelled = FALSE (normal ability end after setup)

GA_FatherArmor EndAbility:

| Step | Node | Target | Handle Variable |
|------|------|--------|-----------------|
| 1 | Event K2_OnEndAbility | - | bWasCancelled output |
| 2 | Branch (bWasCancelled) | - | - |
| 3 | (FALSE path) | Skip cleanup | - |
| 4 | (TRUE path) Is Valid (OwnerPlayer) | - | - |
| 5 | Get Ability System Component | Player | - |
| 6 | Cancel Ability | Player ASC | DomeAbilityHandle |
| 7 | Set Remove Ability On End | Player ASC | DomeAbilityHandle |
| 8 | Cancel Ability | Player ASC | DomeBurstAbilityHandle |
| 9 | Set Remove Ability On End | Player ASC | DomeBurstAbilityHandle |
| 10 | Restore Player Speed | Player CharMov | OriginalWalkSpeed |
| 11 | Detach Father from Player | Father | - |
| 12 | Call Parent EndAbility | - | - |

GA_FatherExoskeleton EndAbility:

| Step | Node | Target | Handle Variable |
|------|------|--------|-----------------|
| 1 | Event K2_OnEndAbility | - | bWasCancelled output |
| 2 | Branch (bWasCancelled) | - | - |
| 3 | (FALSE path) | Skip cleanup | - |
| 4 | (TRUE path) Is Valid (OwnerPlayer) | - | - |
| 5 | Get Ability System Component | Player | - |
| 6 | Cancel Ability | Player ASC | DashAbilityHandle |
| 7 | Set Remove Ability On End | Player ASC | DashAbilityHandle |
| 8 | Cancel Ability | Player ASC | SprintAbilityHandle |
| 9 | Set Remove Ability On End | Player ASC | SprintAbilityHandle |
| 10 | Cancel Ability | Player ASC | StealthAbilityHandle |
| 11 | Set Remove Ability On End | Player ASC | StealthAbilityHandle |
| 12 | Restore Player Speed | Player CharMov | OriginalWalkSpeed |
| 13 | Detach Father from Player | Father | - |
| 14 | Call Parent EndAbility | - | - |

GA_FatherSymbiote EndAbility:

| Step | Node | Target | Handle Variable |
|------|------|--------|-----------------|
| 1 | Event K2_OnEndAbility | - | bWasCancelled output |
| 2 | Branch (bWasCancelled) | - | - |
| 3 | (FALSE path) | Skip cleanup | - |
| 4 | (TRUE path) Is Valid (OwnerPlayer) | - | - |
| 5 | Get Ability System Component | Player | - |
| 6 | Cancel Ability | Player ASC | ProximityAbilityHandle |
| 7 | Set Remove Ability On End | Player ASC | ProximityAbilityHandle |
| 8 | Restore Player Speed | Player CharMov | OriginalWalkSpeed |
| 9 | Detach Father from Player | Father | - |
| 10 | Call Parent EndAbility | - | - |

### 36.14) Manual Cleanup Pattern (RemoveActiveEffectsWithSourceObject Alternative)

Narrative Pro does NOT implement RemoveActiveEffectsWithSourceObject. Manual cleanup required using stored handles.

Missing C++ function:

| Function | Availability | Purpose |
|----------|--------------|---------|
| RemoveActiveEffectsWithSourceObject(Actor) | NOT in Narrative Pro | Remove all GEs from target where SourceObject matches |

Manual alternative - iterate stored handles:

| Step | Action |
|------|--------|
| 1 | Store each applied GE handle when applying |
| 2 | On cleanup, iterate stored handles |
| 3 | Remove Active Gameplay Effect for each valid handle |

BP_FatherCompanion handle arrays for manual cleanup:

| Array | Type | Contents |
|-------|------|----------|
| PlayerEffectHandles | Array of ActiveGameplayEffectHandle | GEs applied to player |
| FatherEffectHandles | Array of ActiveGameplayEffectHandle | GEs applied to father |
| PlayerAbilityHandles | Array of GameplayAbilitySpecHandle | Abilities granted to player |

Emergency cleanup function (call from EndPlay):

| Step | Action |
|------|--------|
| 1 | For Each PlayerEffectHandle |
| 2 | Get Player ASC (with validity check) |
| 3 | Remove Active Gameplay Effect (handle) |
| 4 | For Each PlayerAbilityHandle |
| 5 | Get Player ASC (with validity check) |
| 6 | Cancel Ability (handle) |
| 7 | Set Remove Ability On End (handle) |
| 8 | Clear all arrays |

---

## SECTION 37: ABILITY VALIDATION ARCHITECTURE

### 37.1) GAS Activation Pipeline

| Order | Check | Timing |
|-------|-------|--------|
| 1 | DoesAbilitySatisfyTagRequirements | Before activation |
| 2 | CheckCost | Before activation |
| 3 | CheckCooldown | Before activation |
| 4 | CanActivateAbility (custom) | Before activation |
| 5 | ActivateAbility | Activation begins |
| 6 | CommitAbility | Optional during activation |

Blueprint guards in ActivateAbility run AFTER step 5 - ability already started.

### 37.2) Validation Method Comparison

| Method | Timing | Safety | Performance |
|--------|--------|--------|-------------|
| Activation Required/Blocked Tags | Before activation | Safe | Fast (native C++) |
| CanActivateAbility override | Before activation | Safe | Good |
| Blueprint guard in ActivateAbility | After activation | Risky | Slow |

### 37.3) Why Tag-Based Blocking is Preferred

| Benefit | Explanation |
|---------|-------------|
| Prediction safe | Evaluated before prediction begins |
| No EndAbility needed | Ability never activates |
| Automatic feedback | Failure tags available for UI |
| Native performance | C++ tag matching |

### 37.4) Father.State.Recruited Tag

Primary gate for form abilities:

| Configuration | Value |
|---------------|-------|
| Tag | Father.State.Recruited |
| Granted by | NE_SetFatherOwner (via GE_GrantRecruitedTag) |
| Removed by | NE_ClearFatherOwner |
| Required by | All form abilities (Activation Required Tags) |

### 37.5) Form Ability Activation Required Tags

| Ability | Required Tags |
|---------|---------------|
| GA_FatherArmor | Father.State.Recruited |
| GA_FatherExoskeleton | Father.State.Recruited |
| GA_FatherSymbiote | Father.State.Recruited |
| GA_FatherEngineer | Father.State.Recruited |

### 37.6) CanActivateAbility Override Pattern

For complex validation beyond tags:

| Step | Node | Purpose |
|------|------|---------|
| 1 | Override CanActivateAbility | Function override |
| 2 | Parent: CanActivateAbility | Call parent first |
| 3 | Get Avatar Actor From Actor Info | Get father |
| 4 | Cast To BP_FatherCompanion | Access variables |
| 5 | GET OwnerPlayer | Get player reference |
| 6 | Is Valid | Check not NULL |
| 7 | AND (Parent result, IsValid result) | Combine checks |
| 8 | Return Boolean | TRUE = can activate |

### 37.7) When to Use Each Validation Layer

| Layer | Use For | Examples |
|-------|---------|----------|
| Tags | State requirements | Recruited, Alive, not Dead |
| Tags | Mutual exclusion | Form cancellation |
| Tags | Equipment prereqs | Must have weapon equipped |
| CanActivateAbility | Reference validation | OwnerPlayer Is Valid |
| CanActivateAbility | Complex conditions | Charge threshold met |
| CanActivateAbility | Dynamic state | Target in range |

### 37.8) Validation Failure Handling

| Method | Failure Behavior |
|--------|------------------|
| Tag-blocked | Ability never activates, OnAbilityFailed fires |
| CanActivateAbility returns false | Ability never activates |
| Blueprint guard fails | MUST call EndAbility or ability stays "running" |

Epic warning: "Failing to end the ability properly will result in the Gameplay Ability System believing that the Ability is still running."

### 37.9) Narrative Pro Validation Pattern

Narrative Pro uses hybrid approach (from source code):

| Class | Validation Method |
|-------|-------------------|
| NarrativeGameplayAbility | Tags only (base class) |
| NarrativeCombatAbility | CanActivateAbility override + CheckCost |
| NarrativeInteractAbility | CanActivateAbility override |

### 37.10) Recommended Father Validation Architecture

| Layer | Implementation |
|-------|----------------|
| Tags | Father.State.Recruited in Activation Required |
| Tags | Narrative.State.IsDead in Activation Blocked |
| Tags | Cooldown.Father.FormChange in Activation Blocked |
| CanActivateAbility | Validate OwnerPlayer Is Valid |

---

## SECTION 38: FATHER DEATH AND VULNERABILITY

> **INV-1 COMPLIANCE (January 2026):** Per GAS Audit decision INV-1, ALL form-based invulnerability has been REMOVED. Father is vulnerable in ALL forms.

### 38.1) Form-Based Vulnerability - UPDATED (INV-1)

| Form | Vulnerability | Reason |
|------|---------------|--------|
| Crawler | **Vulnerable** | Detached, can be attacked |
| Armor | **Vulnerable (INV-1)** | Attached, but NO invulnerability |
| Exoskeleton | **Vulnerable (INV-1)** | Attached, but NO invulnerability |
| Symbiote | **Vulnerable (INV-1)** | Attached, but NO invulnerability |
| Engineer | **Vulnerable** | Deployed as turret, can be attacked |

### 38.2) Narrative.State.Invulnerable Tag (Reference)

Built-in Narrative Pro tag for damage immunity:

| Property | Value |
|----------|-------|
| Tag | Narrative.State.Invulnerable |
| Location | NarrativeGameplayTags.cpp |
| Effect | PreGameplayEffectExecute returns false, damage = 0 |

> **INV-1 Note:** Father's GE_*State effects do NOT grant this tag. Only GA_FatherSacrifice uses invulnerability.

### 38.3) Invulnerability Implementation - UPDATED (INV-1)

GE State effects NO LONGER grant invulnerability:

| GE | Grants Invulnerable |
|----|---------------------|
| GE_CrawlerState | **No** |
| GE_ArmorState | **No (INV-1)** |
| GE_ExoskeletonState | **No (INV-1)** |
| GE_SymbioteState | **No (INV-1)** |
| GE_EngineerState | **No** |

**Only Exception:** GA_FatherSacrifice grants GE_SacrificeInvulnerability to the PLAYER for 8 seconds.

### 38.4) Death Handling Architecture

NarrativeAbilitySystemComponent death flow (HandleOutOfHealth):

| Step | Action |
|------|--------|
| 1 | Health reaches zero (server authority) |
| 2 | Check State.IsDead tag not already present |
| 3 | Fire GameplayEvent.Death |
| 4 | Set bIsDead = true |
| 5 | Broadcast OnDied delegate |

### 38.5) OnFatherDied Event Handler

| Step | Action | Reason |
|------|--------|--------|
| 1 | Check IsAttached | **Can be true (INV-1 removed invulnerability)** |
| 2 | Cancel all active abilities | Stop running logic |
| 3 | Clear or Keep OwnerPlayer | KEEP - ownership persists through death |
| 4 | SET CurrentForm = Crawler | Reset to default |
| 5 | Play death animation/ragdoll | Visual feedback |
| 6 | NPCSpawner handles respawn | System manages respawn timing |

> **INV-1 Note:** Father can now die while attached. Death handling must account for detaching from player.

### 38.6) Death in Each Form

| Form | Can Die | On Death |
|------|---------|----------|
| Crawler | Yes | Standard death flow |
| Armor | No | Invulnerable tag blocks damage |
| Exoskeleton | No | Invulnerable tag blocks damage |
| Symbiote | No | Invulnerable tag blocks damage |
| Engineer | Yes | Turret can be destroyed |

### 38.7) Respawn Considerations

| Aspect | Decision |
|--------|----------|
| Ownership | Preserved (OwnerPlayer remains valid) |
| Re-recruitment | Not required (father remembers owner) |
| Form on respawn | Crawler (default) |
| Goal system | Goal Follow Character resumes |

### 38.8) Death Delegate Binding

| Step | Action |
|------|--------|
| 1 | BeginPlay: Get Ability System Component |
| 2 | Bind Event to OnDied |
| 3 | Create OnFatherDied custom event |
| 4 | OnFatherDied handles cleanup |

### 38.9) Deferred Implementation Note

Full death/respawn implementation deferred until Narrative Pro v2.2 tether feature. v2.2 will decouple NPCs from spawn points, allowing father to tether to player when far from original spawn.

---

## SECTION 39: ENDPLAY SAFETY NET PATTERN

### 39.1) Why EndPlay Safety Net is Critical

| Scenario | Without Safety Net | With Safety Net |
|----------|-------------------|-----------------|
| Network disconnect | Orphaned abilities persist on player | All cross-actor grants cleaned up |
| Level streaming | Father destroyed mid-form | Player abilities properly removed |
| Game shutdown | Undefined behavior | Clean termination |
| Hot reload (editor) | Corrupt state | Proper cleanup |

### 39.2) EndPlay Cleanup Flow

| Step | Action | Target |
|------|--------|--------|
| 1 | Event EndPlay override | Father |
| 2 | Is Valid (OwnerPlayer) | Check reference |
| 3 | Branch on validity | - |
| 4 | TRUE: Get Player ASC | Player |
| 5 | For each stored ability handle | See 39.3 |
| 6 | Two-step cleanup pattern | Player ASC |
| 7 | For each stored effect handle | See 39.4 |
| 8 | Remove Active Gameplay Effect | Player ASC |
| 9 | Clear father ability handles | Father arrays |
| 10 | Clear father effect handles | Father arrays |
| 11 | Set OwnerPlayer = None | Father variable |
| 12 | Call Parent EndPlay | Engine |

### 39.3) Ability Handle Cleanup in EndPlay

| Handle Variable | Two-Step Cleanup |
|-----------------|------------------|
| DomeAbilityHandle | Cancel + SetRemoveOnEnd |
| DomeBurstAbilityHandle | Cancel + SetRemoveOnEnd |
| DashAbilityHandle | Cancel + SetRemoveOnEnd |
| SprintAbilityHandle | Cancel + SetRemoveOnEnd |
| StealthAbilityHandle | Cancel + SetRemoveOnEnd |
| ProximityAbilityHandle | Cancel + SetRemoveOnEnd |

Note: BackstabAbilityHandle removed in v5.0 - GA_Backstab is now a Player Default Ability.

### 39.4) Effect Handle Cleanup in EndPlay

| Handle Variable | Target ASC |
|-----------------|------------|
| ArmorBoostHandle | Player |
| ExoskeletonSpeedHandle | Player |
| SymbioteBoostHandle | Player |
| CrawlerStateHandle | Father |
| ArmorStateHandle | Father |
| ExoskeletonStateHandle | Father |
| SymbioteStateHandle | Father |
| EngineerStateHandle | Father |

### 39.5) EndPlay vs Destroyed vs BeginDestroy

| Event | When Called | Safe for ASC Operations |
|-------|-------------|------------------------|
| EndPlay | Actor removed from world | YES - all systems valid |
| Destroyed | Actor marked for destruction | PARTIAL - some systems may be invalid |
| BeginDestroy | GC collecting actor | NO - too late |

Always use EndPlay for cleanup, never BeginDestroy.

### 39.6) Narrative Pro EndPlay Gap

Narrative Pro does NOT implement EndPlay ASC cleanup in NarrativeCharacter:

| Class | EndPlay Content |
|-------|-----------------|
| NarrativeCharacter.cpp | Only destroys CharVisual, NO ASC cleanup |
| NarrativeNPCController.cpp | Only removes PathFollowing delegate, NO ASC cleanup |

Father companion MUST implement its own EndPlay cleanup.

### 39.7) EndPlay Safety Net Blueprint Implementation

BP_FatherCompanion Event Graph:

| Step | Node |
|------|------|
| 1 | Add Event End Play (override) |
| 2 | From End Play Reason -> Has Authority |
| 3 | Branch on Has Authority |
| 4 | TRUE (Server): Execute cleanup sequence |
| 5 | Call custom function CleanupCrossActorGrants |
| 6 | Parent: End Play (call at end) |

CleanupCrossActorGrants function:

| Step | Node |
|------|------|
| 1 | Is Valid (OwnerPlayer) |
| 2 | FALSE: Return (nothing to clean) |
| 3 | TRUE: Get Player ASC |
| 4 | Is Valid (Player ASC) |
| 5 | FALSE: Return |
| 6 | TRUE: For each ability handle |
| 7 | Cancel Ability (handle) on Player ASC |
| 8 | Set Remove Ability On End (handle) on Player ASC |
| 9 | For each effect handle |
| 10 | Remove Active Gameplay Effect (handle) on appropriate ASC |

---

## SECTION 40: CROSS-ACTOR AUTHORITY GATES

### 40.1) Why HasAuthority Gates are Required

| Operation | Without Authority Check | With Authority Check |
|-----------|------------------------|---------------------|
| GiveAbility to player | Client grants, server unaware | Only server grants, replicates |
| Apply GE to player | Client applies, no replication | Server applies, properly replicated |
| ClearAbility from player | Client clears, server retains | Server clears, synchronized |
| Spawn actor | Client spawns, not replicated | Server spawns, replicated |

### 40.2) Narrative Pro Authority Patterns (Reference)

Narrative Pro uses authority checks consistently:

| Location | Pattern |
|----------|---------|
| EquippableItem.cpp:62 | `if (HasAuthority())` before GE application |
| EquippableItem.cpp:90 | `if (HasAuthority())` before GE removal |
| NarrativeCharacter.cpp:695 | `if (IsValid(Ability) && HasAuthority() && AbilitySystemComponent)` |
| NarrativeCharacter.cpp:745 | `if (GetLocalRole() != ROLE_Authority)` early return |

### 40.3) Safe Ability Grant Pattern (Father to Player)

| Step | Node | Purpose |
|------|------|---------|
| 1 | Has Authority | Check server |
| 2 | Branch -> FALSE: Return | Skip on client |
| 3 | TRUE: Get OwnerPlayer | Get target |
| 4 | Is Valid (OwnerPlayer) | Validate reference |
| 5 | Get Ability System Component (Player) | Get target ASC |
| 6 | Make Gameplay Ability Spec | Create spec |
| 7 | Spec -> Set Source Object = Self | Track granter |
| 8 | Give Ability (spec) on Player ASC | Grant ability |
| 9 | Store returned handle | For cleanup |

### 40.4) Safe Ability Remove Pattern (Father from Player)

| Step | Node | Purpose |
|------|------|---------|
| 1 | Has Authority | Check server |
| 2 | Branch -> FALSE: Return | Skip on client |
| 3 | TRUE: Get OwnerPlayer | Get target |
| 4 | Is Valid (OwnerPlayer) | Validate reference |
| 5 | Get Ability System Component (Player) | Get target ASC |
| 6 | Is Valid (stored handle) | Check handle valid |
| 7 | Cancel Ability (handle) on Player ASC | Stop execution |
| 8 | Set Remove Ability On End (handle) on Player ASC | Queue removal |
| 9 | Clear stored handle | Prevent reuse |

### 40.5) Safe GE Application Pattern (Father to Player)

| Step | Node | Purpose |
|------|------|---------|
| 1 | Has Authority | Check server |
| 2 | Branch -> FALSE: Return | Skip on client |
| 3 | TRUE: Get OwnerPlayer | Get target |
| 4 | Is Valid (OwnerPlayer) | Validate reference |
| 5 | Get Ability System Component (Player) | Get target ASC |
| 6 | Make Effect Context (on Player ASC) | Create context |
| 7 | Add Source Object (Self) to context | Track source |
| 8 | Make Outgoing Spec (GE class, context) | Create spec |
| 9 | Apply Gameplay Effect Spec to Self (on Player ASC) | Apply effect |
| 10 | Store returned handle | For cleanup |

### 40.6) Safe GE Removal Pattern (Father from Player)

| Step | Node | Purpose |
|------|------|---------|
| 1 | Has Authority | Check server |
| 2 | Branch -> FALSE: Return | Skip on client |
| 3 | TRUE: Get OwnerPlayer | Get target |
| 4 | Is Valid (OwnerPlayer) | Validate reference |
| 5 | Get Ability System Component (Player) | Get target ASC |
| 6 | Is Valid (stored handle) | Check handle valid |
| 7 | Remove Active Gameplay Effect (handle) on Player ASC | Remove effect |
| 8 | Clear stored handle | Prevent reuse |

### 40.7) Authority Gate Placement in Form Abilities

| Section | Authority Gate |
|---------|---------------|
| ActivateAbility start | Before any cross-actor operations |
| Before GiveAbility | Gate each grant individually |
| Before Apply GE | Gate each application individually |
| EndAbility cleanup | Gate entire cleanup section |

Recommended: Single authority gate at ActivateAbility start for form abilities.

### 40.8) Client-Side Operations (No Authority Needed)

| Operation | Authority Required |
|-----------|-------------------|
| Play local VFX | NO |
| Play local sound | NO |
| Update local UI | NO |
| Read variables | NO |
| Read tags | NO |
| Activate ability on OWN ASC | NO (prediction handles) |

### 40.9) Server-Side Operations (Authority Required)

| Operation | Authority Required |
|-----------|-------------------|
| GiveAbility to other ASC | YES |
| ClearAbility from other ASC | YES |
| Apply GE to other ASC | YES |
| Remove GE from other ASC | YES |
| Spawn replicated actor | YES |
| Modify replicated variable | YES |
| Set replicated tag | YES |

---

## SECTION 41: SYMBIOTE AUTO-RETURN FALLBACK

### 41.1) Symbiote Timer Expiration Flow

| Step | Action |
|------|--------|
| 1 | GA_FatherSymbiote 30-second timer expires |
| 2 | Timer callback fires |
| 3 | Attempt return to Armor form |
| 4 | If Armor fails, fallback to Crawler |
| 5 | If Crawler fails, force manual restoration |

### 41.2) Fallback Priority

| Priority | Target Form | Condition |
|----------|-------------|-----------|
| 1 | Armor | Attached to player (default return) |
| 2 | Crawler | If Armor blocked/fails |
| 3 | Manual restoration | If all activation fails |

### 41.3) Armor Activation Failure Reasons

| Reason | Tag/Condition |
|--------|---------------|
| Cooldown active | Cooldown.Father.FormChange |
| Player dead | Narrative.State.IsDead on player |
| Player invalid | OwnerPlayer reference NULL |
| Father stunned | Narrative.State.Stunned |
| Father dead | Narrative.State.IsDead |

### 41.4) Fallback Implementation Flow

GA_FatherSymbiote timer callback:

| Step | Node |
|------|------|
| 1 | Timer fires (30 seconds elapsed) |
| 2 | Try Activate Ability By Class (GA_FatherArmor) |
| 3 | Branch on return value (activated successfully) |
| 4 | TRUE: Armor activated, done |
| 5 | FALSE: Try Activate Ability By Class (GA_FatherCrawler) |
| 6 | Branch on return value |
| 7 | TRUE: Crawler activated (fallback worked) |
| 8 | FALSE: Execute manual restoration |

### 41.5) Manual Restoration (Last Resort)

If both Armor and Crawler activation fail:

| Step | Action |
|------|--------|
| 1 | Remove GE_SymbioteBoost from player |
| 2 | Remove GE_SymbioteState from father |
| 3 | Cancel GA_ProximityStrike on player ASC (two-step) |
| 4 | Restore player speed to stored original |
| 5 | Clear Symbiote form tags manually |
| 6 | Set CurrentForm = Crawler |
| 7 | Set IsAttached = false |
| 8 | Detach father from player |
| 9 | Log warning for investigation |
| 10 | End Ability (GA_FatherSymbiote) |

### 41.6) Why Fallback is Necessary

| Issue | Without Fallback | With Fallback |
|-------|------------------|---------------|
| Cooldown blocks Armor | Father stuck in Symbiote | Returns to Crawler |
| Player dies during Symbiote | Undefined state | Clean return to Crawler |
| Network desync | Ability state corruption | Manual state restoration |

### 41.7) Fallback Logging

| Log Level | Message |
|-----------|---------|
| Warning | "Symbiote auto-return: Armor activation failed, falling back to Crawler" |
| Error | "Symbiote auto-return: All activation failed, executing manual restoration" |

---

## SECTION 42: NPC SCHEDULE SYSTEM

### 42.1) Schedule System Overview

Narrative Pro provides a hierarchical schedule system for time-based NPC behaviors (daily routines, work schedules, patrol shifts).

| Component | Purpose |
|-----------|---------|
| UTriggerSchedule | DataAsset containing multiple TriggerSets |
| UTriggerSet | DataAsset containing multiple UNarrativeTrigger instances |
| UNarrativeTrigger | Base class for trigger types (Time of Day Range, Always, etc.) |
| UNarrativeEvent | Actions executed when triggers fire (AI: Add Goal To NPC, etc.) |
| UNPCActivitySchedule | DataAsset containing scheduled behaviors with time ranges |
| UScheduledBehavior | Base class for time-based behaviors |

### 42.2) C++ Source Locations

| Class | Header File | Purpose |
|-------|-------------|---------|
| UTriggerSchedule | TriggerSchedule.h | Schedule container |
| UTriggerSet | TriggerSet.h | Trigger container |
| UNarrativeTrigger | NarrativeTrigger.h | Base trigger class |
| UNarrativeEvent | NarrativeEvent.h | Event base class |
| UScheduledBehavior | NarrativeGameState.h | Time-based behavior base |
| UScheduledBehavior_NPC | NPCActivitySchedule.h | NPC-specific scheduled behavior |
| UScheduledBehavior_AddNPCGoal | NPCActivitySchedule.h | Adds goal at start time, removes at end |
| UNPCActivitySchedule | NPCActivitySchedule.h | DataAsset for NPC schedules |

### 42.3) TriggerSchedule Structure

| Property | Type | Purpose |
|----------|------|---------|
| TriggerSets | TArray<TSoftObjectPtr<UTriggerSet>> | References to TriggerSet assets |

### 42.4) TriggerSet Structure

| Property | Type | Purpose |
|----------|------|---------|
| Triggers | TArray<UNarrativeTrigger*> | Instanced trigger objects |

### 42.5) NarrativeTrigger Base Class

| Property | Type | Purpose |
|----------|------|---------|
| TriggerEvents | TArray<UNarrativeEvent*> | Events fired when trigger activates |
| OwnerCharacter | ANarrativeCharacter* | Character owning this trigger |
| bIsActive | bool | Current activation state |

| Function | Purpose |
|----------|---------|
| Initialize() | Called when trigger is set up |
| Activate() | Fires all TriggerEvents with Start runtime |
| Deactivate() | Fires all TriggerEvents with End runtime |
| IsActive() | Returns current state |
| GetDescription() | For debug/UI display |

### 42.6) Available Trigger Types

| Blueprint Trigger | Parent Class | Purpose |
|-------------------|--------------|---------|
| BPT_TimeOfDayRange | UNarrativeTrigger | Fires within time range |
| BPT_Always | UNarrativeTrigger | Fires immediately on initialize |

### 42.7) BPT_TimeOfDayRange Configuration

| Property | Type | Purpose |
|----------|------|---------|
| TimeStart | Float | Start of active range (0.0 - 2400.0) |
| TimeEnd | Float | End of active range (0.0 - 2400.0) |
| TimeRange | FTimeOfDayRange | Struct containing range data |

| Custom Event | Purpose |
|--------------|---------|
| Wait Time | Sets up WaitTimeRange latent node |
| Time Range Start | Fires when time enters range -> calls Activate |
| Time Range End | Fires when time exits range -> calls Deactivate |

### 42.8) BPT_TimeOfDayRange Initialize Flow

| Step | Action |
|------|--------|
| 1 | Event Initialize fires |
| 2 | Break Time Of Day Range -> extracts TimeMin/TimeMax |
| 3 | Get Time Of Day -> Is Time in Range check |
| 4 | If current time IS in range -> immediately calls Activate |
| 5 | Then calls Wait Time to set up future triggers |

### 42.9) WaitTimeRange Latent Node

| Input | Type | Purpose |
|-------|------|---------|
| Time Start | Float | Range start time |
| Time End | Float | Range end time |

| Output Pin | Type | Purpose |
|------------|------|---------|
| On Time Range Start | Exec | Fires when time enters range |
| On Time Range End | Exec | Fires when time exits range |
| Event Time | Float | Current time of day |
| Time at Fire | Float | Exact time trigger fired |
| Time Passed Delta | Float | Time since last check |
| Fired from Advance Time | Bool | True if manual time advance |

### 42.10) BPT_Always Configuration

Simplest trigger - immediately activates on initialize.

| Event | Action |
|-------|--------|
| Event Initialize | -> Activate (self) |

Use Cases:

| Use Case | Example |
|----------|---------|
| Permanent behaviors | NPC always has idle activity |
| Default state | Guard always patrols until interrupted |
| Persistent goals | Companion always follows player |
| Spawn-time setup | Grant abilities/tags on NPC spawn |

### 42.11) Time of Day System

ANarrativeGameState manages game time:

| Property | Type | Purpose |
|----------|------|---------|
| TimeOfDay | float | Current time (0-2400) |
| AccumulatedTime | float | Total time passed (2400 = 1 day) |

| Function | Purpose |
|----------|---------|
| GetTimeOfDayEventDelegate(float Time) | Returns delegate for specific time |
| AdvanceTimeOfDay(float Amount) | Manually advances time |
| AdvanceToTimeOfDay(float DesiredTime) | Jumps to specific time |

### 42.12) Time of Day Settings

UNarrativeTimeOfDaySettings (DeveloperSettings):

| Setting | Purpose |
|---------|---------|
| bDynamicTimeOfDay | Auto-advance time |
| DefaultTimeOfDay | Starting time |
| DayLengthMinutes | Real-time to game-time (day) |
| NightLengthMinutes | Real-time to game-time (night) |
| SunriseTime | Day/night boundary |
| SunsetTime | Day/night boundary |

### 42.13) NarrativeEvent Types

| Event Type | Purpose |
|------------|---------|
| AI: Add Goal To NPC | Adds goal to NPCActivityComponent |
| AI: Remove Goal From NPC | Removes goal from NPC |
| Set Gameplay Tag | Adds/removes gameplay tag |
| Spawn Actor | Spawns actor at location |
| Custom Event (Blueprint) | User-defined event logic |

### 42.14) AI: Add Goal To NPC Event Configuration

| Property | Value | Purpose |
|----------|-------|---------|
| Refire on Load | Checked | Restores goal after save/load if within time range |
| Event Runtime | Start | Fires at range start time |
| Event Filter | Only NPCs | Excludes players |
| NPCTargets | Empty | Empty = applies to owning NPC |
| Goal to Add | Goal Interact | Type of goal to add |

### 42.15) NPCActivitySchedule System

Alternative to TriggerSet for NPC-specific scheduling:

| Property | Type | Purpose |
|----------|------|---------|
| Activities | TArray<UScheduledBehavior_NPC*> | Instanced scheduled behaviors |

### 42.16) UScheduledBehavior Properties

| Property | Type | Purpose |
|----------|------|---------|
| StartTime | float | When behavior activates (0-2400) |
| EndTime | float | When behavior deactivates (0-2400) |
| bDisabled | bool | Skip this behavior |

| Function | Purpose |
|----------|---------|
| BindBehavior() | Hooks into GameState time events |
| HandleStarted() | Virtual - called at StartTime |
| HandleEnded() | Virtual - called at EndTime |

### 42.17) UScheduledBehavior_AddNPCGoal

| Property | Type | Purpose |
|----------|------|---------|
| ActiveGoal | UNPCGoalItem* | Currently active goal (runtime) |
| ScoreOverride | float | Override goal scoring |
| bReselect | bool | Force activity reselection |

| Function | Purpose |
|----------|---------|
| ProvideGoal() | Virtual - returns goal instance |
| HandleStarted() | Creates and adds goal |
| HandleEnded() | Removes goal |

### 42.18) Goal Catchup System

For late game loads (e.g., load at 10 PM when goal started at 8 AM):

| Property | Purpose |
|----------|---------|
| IntendedTODStartTime | When goal should have started |
| TODCreationTime | When goal actually created |

| Usage | Behavior |
|-------|----------|
| Load within schedule range | Catchup time calculated |
| BPA activity | Can teleport NPC directly to slot |
| Fresh goal check | Goal->GetGoalAgeSeconds() < 0.5f |

### 42.19) NPCDefinition Integration

| Property | Type | Purpose |
|----------|------|---------|
| ActivitySchedules | TArray<TSoftObjectPtr<UNPCActivitySchedule>> | Schedule assets |
| DefaultTriggerSets | TArray<TSoftObjectPtr<UTriggerSet>> | Trigger assets |

| Load Function | Purpose |
|---------------|---------|
| OnCharacterDefinitionDataLoaded() | Loads schedules |
| ApplyActivitySchedules() | Calls AddActivitySchedule() |
| ApplyTriggerSets() | Calls AddTrigger() |

### 42.20) Example: Blacksmith Work Schedule

| Time | Goal | Behavior |
|------|------|----------|
| 8:01 AM | Goal_Interact added | Walk to anvil |
| 8:01 AM - 4:00 PM | Goal active | Work at anvil (SeatIdle loop) |
| 4:00 PM | Goal removed | Activity reselection |

TriggerSet Configuration:

| Property | Value |
|----------|-------|
| Trigger Type | Time of Day Range |
| Time Start | 801.0 |
| Time End | 1600.0 |
| Event Type | AI: Add Goal To NPC |
| Goal to Add | Goal Interact |
| Actor to Interact With | BP_InteractableSeat_Anvil |

---

## SECTION 43: INTERACTION SLOT SYSTEM

### 43.1) Interaction Slot Overview

Narrative Pro provides a slot-based interaction system for NPC workstations, seats, and other interactables.

| Component | Purpose |
|-----------|---------|
| UNarrativeInteractableComponent | Component on interactable actors |
| FInteractionSlotConfig | Configuration for each slot |
| UInteractionSlotBehavior | Defines behavior when slot used |
| EInteractionSlotStatus | Slot state (Free, Targeted, Occupied) |
| FInteractionSlotClaimHandle | Reference to claimed slot |

### 43.2) C++ Source Locations

| Class | Header File | Purpose |
|-------|-------------|---------|
| UNarrativeInteractableComponent | InteractableComponent.h | Interactable component |
| FInteractionSlotConfig | InteractableComponent.h | Slot configuration struct |
| UInteractionSlotBehavior | InteractableComponent.h | Slot behavior base class |
| FInteractionSlotClaimHandle | InteractableComponent.h | Handle for claimed slots |
| FActiveInteractionSlot | InteractableComponent.h | Runtime slot state |

### 43.3) FInteractionSlotConfig Structure

| Property | Type | Purpose |
|----------|------|---------|
| SlotTag | FGameplayTag | Optional tag identifier |
| DebugColor | FColor | Visualization color in editor |
| LinkedSlots | TArray<int32> | Indices that share status |
| SlotTransform | FTransform | World position for navigation/warp |
| SlotInteractBehavior | UInteractionSlotBehavior* | Behavior when slot used |

### 43.4) UInteractionSlotBehavior Properties

| Property | Type | Purpose |
|----------|------|---------|
| SlotInteractBehavior | TSubclassOf<UNarrativeInteractAbility> | Ability to activate |
| FinishInteractText | FText | Prompt text for exiting |
| bIsStealableByDefault | bool | Can others steal slot |
| AllowedInteractors | TSet<UNarrativeInteractionComponent*> | Whitelist |

| Function | Purpose |
|----------|---------|
| IsStealable() | Virtual - check if slot can be stolen |
| IsUsable() | Virtual - check if slot is usable |

### 43.5) EInteractionSlotStatus Values

| Status | Value | Meaning |
|--------|-------|---------|
| ISS_Free | 0 | Available |
| ISS_Targeted | 1 | Reserved by approaching NPC |
| ISS_Occupied | 2 | In use |

### 43.6) FActiveInteractionSlot Runtime State

| Property | Type | Purpose |
|----------|------|---------|
| SlotStatus | EInteractionSlotStatus | Current status |
| SlotUser | UNarrativeInteractionComponent* | Component using slot |

### 43.7) FInteractionSlotClaimHandle

| Property | Type | Purpose |
|----------|------|---------|
| HandleIndex | int32 | Slot array index |
| HandleOwner | TWeakObjectPtr<UNarrativeInteractableComponent> | Owning component |

| Function | Purpose |
|----------|---------|
| IsValidHandle() | Check if handle still valid |

### 43.8) Available Slot Behavior Types

| Blueprint | Purpose |
|-----------|---------|
| BP_SlotBehavior_Sit | Sitting interactions (chairs, workstations) |
| BP_SlotBehavior_Mount | Mounting vehicles/horses |
| BP_SlotBehavior_Lever | Pull/push lever interactions |
| BP_SlotBehavior_AnimatedInteractable | Generic animated interactions |

### 43.9) BP_SlotBehavior_Sit Configuration

| Property | Value | Purpose |
|----------|-------|---------|
| Slot Interact Behavior | GA_Interact_Sit | Ability to run |
| Finish Interact Text | "Get Up" | Exit prompt |
| Character Get Up and Sit Down Montage | A_Interact_Chair_Montage | Animation |
| Sit Start Montage Section Name | SeatEntry | Entry animation section |
| Sit Exit Montage Section Name | SeatExit | Exit animation section |
| Sit Montage Warp Target Name | SitWarp | Motion warp target |
| Sit Loop Montage Section Name | SeatIdle | Looping animation section |
| Sit Offset | Transform | Final seated position offset |
| Tags to Apply Whilst Seated | Narrative.State.Movement.Lock | Tags granted |
| Menu to Open when Seated | None | Optional UI menu |

### 43.10) Interaction Props System

BP_SlotBehavior_Sit supports prop attachment:

| Property | Type | Purpose |
|----------|------|---------|
| InteractionProps | TArray<FSInteractProp> | Props to spawn/attach |

FSInteractProp Structure:

| Property | Type | Purpose |
|----------|------|---------|
| PropMesh | UStaticMesh* | Mesh to spawn |
| AttachBone | FName | Skeleton bone to attach |
| AttachOffset | FTransform | Offset from bone |

Example (Blacksmith Anvil):

| Index | PropMesh | AttachBone | Purpose |
|-------|----------|------------|---------|
| 0 | SM_Prop_HammerBlacksmith | hand_r | Hammer in right hand |
| 1 | SM_Prop_Sword | hand_l | Sword in left hand |

### 43.11) NPC Interaction Workflow

| Step | System | Action |
|------|--------|--------|
| 1 | Goal_Interact | Contains target interactable |
| 2 | BPA_Interact | Evaluates goal, scores activity |
| 3 | Behavior Tree | Runs BT_Interact |
| 4 | BT Task | Move to SlotTransform position |
| 5 | BT Task | TargetInteractionSlot() - claim as Targeted |
| 6 | NarrativeInteractAbility | Activates, claims as Occupied |
| 7 | Montage | Plays SeatEntry, warps, loops SeatIdle |
| 8 | Goal removed | SeatExit plays, slot released |

### 43.12) NPCInteractionComponent Functions

| Function | Purpose |
|----------|---------|
| TargetBestInteractionSlot() | Finds best available slot |
| TargetInteractionSlot() | Claims specific slot as Targeted |
| OnTargetSlotTaken | Delegate for slot theft |
| bFindNewSlotIfSlotTaken | Auto-retarget if stolen |

### 43.13) Slot Visualization in Editor

| Color | Meaning |
|-------|---------|
| Green circle | SlotTransform position |
| Blue circle | NPC facing direction after seated |
| Arrow | Forward direction indicator |

### 43.14) Creating Custom Interactable Actors

| Step | Action |
|------|--------|
| 1 | Create Blueprint inheriting from Actor |
| 2 | Add Scene Component (root) |
| 3 | Add Static Mesh Component (visual) |
| 4 | Add NarrativeInteractableComponent |
| 5 | Configure Interaction Slots array |
| 6 | Set SlotTransform for each slot |
| 7 | Assign SlotInteractBehavior |

### 43.15) Interactable Actor Replication Settings

| Setting | Value | Purpose |
|---------|-------|---------|
| Net Load on Client | Checked | Clients load from level |
| Component Replicates | Checked | Slot status replicates |
| Net Dormancy | Awake | Active replication |
| Net Update Frequency | 100.0 | Update rate |

---

## SECTION 44: NARRATIVEINTERACTABILITY SYSTEM

### 44.1) NarrativeInteractAbility Overview

Base class for interaction abilities that work with the slot system.

| Parent Class | Header File |
|--------------|-------------|
| NarrativeGameplayAbility | NarrativeInteractAbility.h |

### 44.2) GA_Interact_Sit Properties

| Property | Type | Value |
|----------|------|-------|
| Walk Tag | FGameplayTag | Narrative.State.Movement.Walking |
| Move to Location Task | AbilityTask | None (set at runtime) |
| Sit Behavior | BP Slot Behavior Sit | Populated from slot config |
| NPCInteract Goal | Goal Interact | Populated at runtime |
| Input Tag | FGameplayTag | Narrative.Input.Attack |

### 44.3) GA_Interact_Sit Activation Owned Tags

| Tag | Purpose |
|-----|---------|
| Camera.FirstPerson.DisableFirstPersonRendering | Disable first-person mesh |
| Camera.FirstPerson.Follow3PHeadLocation | Camera follows head |
| Camera.FirstPerson.Follow3PHeadRotation | Camera rotates with head |
| Narrative.State.Busy | General busy state |
| Narrative.State.Interacting | Interacting state |
| Narrative.State.Movement.Walking | Walking movement |
| Narrative.State.Weapon.ForceHolster | Force weapon holster |

### 44.4) GA_Interact_Sit Functions

| Function | Purpose |
|----------|---------|
| Grab Sit Data | Gets sit behavior from slot |
| Attach SM to Character | Attaches prop meshes |
| SpawnProps | Spawns prop actors |
| DestroyProps | Cleans up prop actors |
| ShouldSkipEntry | Check if skip entry animation |

### 44.5) GA_Interact_Sit Variables

| Variable | Type | Purpose |
|----------|------|---------|
| Seat | BP Interactable Seat | Target interactable |
| SitBehavior | BP Slot Behavior Sit | Slot behavior reference |
| NPCInteractGoal | Goal Interact | Current goal |
| CreatedMeshActors | Static Mesh Actor | Spawned prop actors |
| MoveToLocationTask | Ability Task | Movement task |

### 44.6) GA_Interact_Sit Advanced Settings

| Setting | Value | Purpose |
|---------|-------|---------|
| Replication Policy | Do Not Replicate | Client-side ability |
| Instancing Policy | Instanced Per Actor | One per character |
| Net Execution Policy | Server Initiated | Server starts |
| Net Security Policy | Client Or Server | Both can execute |

### 44.7) Montage Section Flow

| Section | When Played | Purpose |
|---------|-------------|---------|
| SeatEntry | On ability activation | Approach and sit |
| SitWarp | During SeatEntry | Motion warp target |
| SeatIdle | After SeatEntry completes | Looping work animation |
| SeatExit | On ability end | Stand up and leave |

### 44.8) Motion Warp Integration

GA_Interact_Sit uses SitWarp motion warp target:

| Property | Purpose |
|----------|---------|
| Sit Montage Warp Target Name | Name referenced in animation |
| Sit Offset | Final transform after warp |

Flow:

| Step | Action |
|------|--------|
| 1 | NPC walks to SlotTransform |
| 2 | SeatEntry montage plays |
| 3 | Motion warp moves NPC to Sit Offset |
| 4 | NPC rotates to face workstation |
| 5 | SeatIdle loops |

### 44.9) Tags Applied During Interaction

| Tag | Source | Duration |
|-----|--------|----------|
| Activation Owned Tags | GA_Interact_Sit | While ability active |
| Tags to Apply Whilst Seated | BP_SlotBehavior_Sit | While seated |

### 44.10) Prop Lifecycle

| Event | Action |
|-------|--------|
| Ability Activates | SpawnProps() creates mesh actors |
| Props Created | Attach SM to Character() attaches to bones |
| Ability Ends | DestroyProps() removes mesh actors |

---

## SECTION 45: TIME OF DAY TRIGGERS

### 45.1) Custom Trigger Development

Create new trigger types by inheriting from UNarrativeTrigger (C++) or BPT_ Blueprint class.

### 45.2) Trigger Blueprint Pattern

| Component | Implementation |
|-----------|----------------|
| Event Initialize | Setup detection/bindings |
| Custom Logic | Your condition checking |
| Activate() | Call when condition met |
| Deactivate() | Call when condition lost |

### 45.3) Potential Custom Triggers

| Trigger Name | Fires When | Use Case |
|--------------|------------|----------|
| BPT_PlayerProximity | Player within radius | Shopkeeper greets player |
| BPT_PlayerSeen | AI Perception sees player | NPC waves at player |
| BPT_PlayerOverlapBox | Player enters volume | Area-specific reactions |
| BPT_PlayerLookingAt | Player focuses on NPC | Acknowledge being watched |
| BPT_TagCondition | NPC has specific tag | State-dependent behavior |
| BPT_QuestState | Quest reaches state | Quest-driven schedules |

### 45.4) BPT_PlayerProximity Implementation Pattern

| Event | Action |
|-------|--------|
| Event Initialize | Set Timer by Function Name ("CheckProximity", 0.5, Looping) |
| CheckProximity | Get Distance To Player |
| If Distance < Radius | Activate() if not already active |
| If Distance > Radius | Deactivate() if currently active |

### 45.5) Combined Trigger Pattern

| Trigger | Condition | Use Case |
|---------|-----------|----------|
| BPT_TimeAndProximity | Time range AND player nearby | Blacksmith only greets during work hours |
| BPT_TagAndProximity | NPC has tag AND player nearby | Only friendly NPCs react |

### 45.6) TriggerSet vs ActivitySchedule Comparison

| Feature | TriggerSet | ActivitySchedule |
|---------|------------|------------------|
| Primary Use | General-purpose triggers | NPC daily routines |
| Container | UTriggerSet DataAsset | UNPCActivitySchedule DataAsset |
| Triggers | Multiple UNarrativeTrigger | Multiple UScheduledBehavior_NPC |
| Events | Any NarrativeEvent | Goal-based behaviors |
| Integration | ApplyTriggerSets() | AddActivitySchedule() |
| Flexibility | High (any event type) | Focused (activity system) |

### 45.7) Scheduled Goals vs Manual Goals

| Type | bSaveGoal | IntendedTODStartTime | Persistence |
|------|-----------|----------------------|-------------|
| Scheduled | false | Set for catchup | Re-added by schedule on load |
| Manual | true | -1 (no catchup) | Saved to disk |

### 45.8) Time Range Behavior for Day Wraparound

UArsenalStatics::IsTimeInRange handles day wraparound:

| Scenario | Start | End | Current | Result |
|----------|-------|-----|---------|--------|
| Normal range | 800 | 1600 | 1200 | In range |
| Normal range | 800 | 1600 | 2000 | Not in range |
| Overnight | 2200 | 600 | 2300 | In range |
| Overnight | 2200 | 600 | 100 | In range |
| Overnight | 2200 | 600 | 1200 | Not in range |

### 45.9) bFiredFromAdvancedTime Parameter

| Value | Meaning |
|-------|---------|
| false | Natural time progression fired trigger |
| true | Manual AdvanceTimeOfDay() call fired trigger |

Use Case: Skip transition animations when time manually advanced (debug/testing).

### 45.10) Father Companion Schedule Integration

Potential father schedule behaviors:

| Time | Behavior | Goal |
|------|----------|------|
| Night (2200-0600) | Dormant mode | Reduce activity |
| Day (0600-2200) | Active mode | Normal following |
| Combat detected | Alert mode | Increased aggression |

Implementation via NPCDefinition->ActivitySchedules with UScheduledBehavior_AddNPCGoal.

---

## SECTION 46: GOAL/ACTIVITY FOLLOW SYSTEM ARCHITECTURE

### 46.1) System Overview

Narrative Pro's follow system consists of interconnected Goal, Activity, Blackboard, and Behavior Tree components that work together to enable NPCs to follow characters.

| Component | Asset | Purpose |
|-----------|-------|---------|
| Goal | Goal_FollowCharacter | Stores target and follow parameters |
| Activity | BPA_FollowCharacter | Manages behavior tree and blackboard setup |
| Blackboard | BB_FollowCharacter | Runtime data for behavior tree |
| Behavior Tree | BT_FollowCharacter | Executes following logic |
| Services | BTS_SetAIFocus, BTS_AdjustFollowSpeed | Look-at and speed adjustment |

### 46.2) Goal_FollowCharacter Variables

| Variable | Type | Purpose |
|----------|------|---------|
| TargetToFollowAsset | Character Definition | For save/load persistence |
| ShouldMountWithTarget | Boolean | Whether to mount vehicles with target |
| FollowDistance | Float | How close to stay (default 200) |
| TargetToFollow | Narrative Character | Runtime actor reference (instance editable) |

### 46.3) Goal_FollowCharacter Class Defaults

| Property | Value |
|----------|-------|
| Follow Distance | 200.0 |
| Should Mount with Target | Checked |
| Goal Lifetime | -1.0 (infinite) |
| Default Score | 1.0 |

### 46.4) Goal_FollowCharacter Persistence Pattern

Event PrepareForSave:

| Step | Action |
|------|--------|
| 1 | Get TargetToFollow actor |
| 2 | Get Character Definition from target |
| 3 | SET TargetToFollowAsset (for save persistence) |

Event Initialize:

| Step | Action |
|------|--------|
| 1 | Is Valid (TargetToFollowAsset) |
| 2 | Get NarrativeCharacterSubsystem |
| 3 | Find Character (by CharacterDefinition) |
| 4 | SET TargetToFollow to found actor |

### 46.5) BPA_FollowCharacter Variables

| Variable | Type | Purpose |
|----------|------|---------|
| FollowGoal | Goal Follow Character | Cached goal reference |
| Interact SubGoal | Goal Interact | For vehicle mounting |
| BBKey_FollowDistance | Name | Blackboard key name |

### 46.6) BPA_FollowCharacter Class Defaults

| Property | Value |
|----------|-------|
| BBKey Follow Distance | FollowDistance |
| Activity Name | Follow Player |
| Owned Tags | Narrative.State.NPC.Activity.Following |
| Behaviour Tree | BT_FollowCharacter |
| Supported Goal Type | Goal_FollowCharacter |
| Is Interruptable | Unchecked |

### 46.7) SetupBlackboard Function Pattern

BPA_FollowCharacter.SetupBlackboard writes goal data to blackboard:

| Step | Action |
|------|--------|
| 1 | Cast ActivityGoal to Goal_FollowCharacter |
| 2 | SET FollowGoal variable |
| 3 | GET Target to Follow from goal |
| 4 | Is Valid check on Target to Follow |
| 5 | If Not Valid: Use NarrativeCharacterSubsystem -> Find Character |
| 6 | Get Narrative Pro Settings -> BBKey Follow Target |
| 7 | Set Value as Object (BB, BBKey Follow Target, Target) |
| 8 | Set Value as Float (BB, BBKey Follow Distance, FollowDistance) |
| 9 | Return true |

### 46.8) NarrativeProSettings BBKey Naming

Blackboard key names are centralized in NarrativeProSettings:

| Setting | Default Value | Used By |
|---------|---------------|---------|
| BBKey Follow Target | FollowTarget | BPA_FollowCharacter, BTS_AdjustFollowSpeed |
| BBKey Follow Distance | FollowDistance | BPA_FollowCharacter, Move To task |
| BBKey Target Location | TargetLocation | BT_GoToLocation, Movement tasks |
| BBKey Target Rotation | TargetRotation | BT_GoToLocation |
| BBKey Player Pawn | PlayerPawn | BPT_FollowNPCToLocation |

Access pattern: Get Narrative Pro Settings -> BBKey [Name]

### 46.9) BB_FollowCharacter Keys

| Key | Type | Purpose |
|-----|------|---------|
| SelfActor | Object (Actor) | Reference to owning NPC |
| FollowTarget | Object (Actor) | Character to follow |
| FollowDistance | Float | Acceptance radius for MoveTo |

### 46.10) BT_FollowCharacter Structure

| Level | Node Type | Node | Services/Decorators |
|-------|-----------|------|---------------------|
| 0 | Root | ROOT | BB_FollowCharacter |
| 1 | Composite | Selector | Blackboard Condition (FollowTarget Is Set), Gameplay Tag Condition (NOT Falling) |
| 2 | Composite | Sequence | BTS_SetAIFocus, BTS_AdjustFollowSpeed |
| 3 | Task | Move To | Blackboard Key: FollowTarget |

### 46.11) Move To Task Configuration

| Property | Value | Notes |
|----------|-------|-------|
| Blackboard Key | FollowTarget | Actor reference |
| Acceptable Radius | 173.58 (default) | Overridden by FollowDistance key |
| Track Moving Goal | Checked | Follows moving target |
| Reach Test Includes Agent Radius | Checked | - |
| Reach Test Includes Goal Radius | Checked | - |
| Require Navigable End Location | Checked | - |
| Project Goal Location | Checked | - |

### 46.12) BTS_SetAIFocus Configuration

| Property | Value |
|----------|-------|
| Focus Target | FollowTarget |
| Interval | 0.5 |
| Random Deviation | 0.1 |

### 46.13) BTS_AdjustFollowSpeed Logic

| Step | Action |
|------|--------|
| 1 | Event Receive Tick AI |
| 2 | Cast Controlled Pawn to NarrativeCharacter |
| 3 | Get Narrative Character Movement component |
| 4 | Get Owners Blackboard -> Get Value as Object (BBKey Follow Target) |
| 5 | Cast to Actor -> Get Distance To (self vs follow target) |
| 6 | Branch: Distance > 575.0 |
| 7 | True: Start Sprinting |
| 8 | False: Stop Sprinting |

Sprint Threshold: 575 units

### 46.14) Event K2_RunActivity Pattern

BPA_FollowCharacter.K2_RunActivity:

| Step | Action |
|------|--------|
| 1 | Cast ActivityGoal to Goal_FollowCharacter |
| 2 | SET FollowGoal variable |
| 3 | GET Target to Follow from goal |
| 4 | Get Narrative Character Movement (both NPC and target) |
| 5 | Set Mirror Component - NPC mirrors target movement |
| 6 | Bind Event to On Teleported - NPC teleports with target |

### 46.15) Speed Matching Pattern

Set Mirror Component mirrors target movement speed:

| Property | Source |
|----------|--------|
| Target | NPC Narrative Character Movement |
| Mirror CMC | Target Narrative Character Movement |

Effect: NPC walks when target walks, runs when target runs.

### 46.16) Teleport Binding Pattern

Bind Event to On Teleported ensures NPC stays with target:

| Event | Action |
|-------|--------|
| Target Teleports | NPC teleports to nearby position |
| OnTeleported_Event | Custom event bound to target teleport |

### 46.17) Vehicle Mounting Behavior

When ShouldMountWithTarget is true:

| Trigger | Action |
|---------|--------|
| Target mounts vehicle | OnPossessedPawnChanged_Event fires |
| Find Mountable Passenger Seat | Searches for available seat |
| Construct Goal Interact | Score 99999 (highest priority) |
| Add Goal | NPC mounts vehicle |
| Target dismounts | Remove Interact Sub Goal |

### 46.18) Event K2_EndActivity Cleanup

| Step | Action |
|------|--------|
| 1 | Set Mirror Component to None |
| 2 | Unbind Event from On Teleported |
| 3 | Unbind Event from On Possessed Pawn Changed |
| 4 | Remove Interact Sub Goal if valid |

### 46.19) NarrativeCharacterSubsystem Functions

| Function | Parameters | Returns | Purpose |
|----------|------------|---------|---------|
| Find Character | CharacterDefinition | NarrativeCharacter | Get actor from definition |
| Find NPC | NPCDefinition / NPCData | NarrativeNPCCharacter | Get NPC from definition |
| Get All Characters | None | Array | All spawned characters |

### 46.20) Quest Task vs Goal System Comparison

| Aspect | Goal System | Quest Task System |
|--------|-------------|-------------------|
| Parent Class | NPCGoalItem | QuestTask |
| Lifespan | Persistent until removed | Quest-bound, temporary |
| Save/Load | Automatic via goal persistence | Quest state persistence |
| Target | Dynamic (actor reference) | Location or actor |
| Completion | Ongoing behavior | Completes quest objective |
| Use Case | Bodyguards, companions | "Go to X" objectives |

### 46.21) BPT_FollowNPCToLocation Direct BT Injection

Quest tasks can inject behavior trees directly without Goal system:

| Step | Action |
|------|--------|
| 1 | Get Relevant NPCs (array) |
| 2 | For Each: Update NPC Behavior |
| 3 | Find NPC via NarrativeCharacterSubsystem |
| 4 | Get AIController from NPC |
| 5 | Run Behavior Tree (BT_MoveToDes) directly |
| 6 | Get Blackboard from controller |
| 7 | Set Value as Vector (BBKey Target Location) |
| 8 | Set Value as Rotator (BBKey Target Rotation) |
| 9 | Set Value as Object (BBKey Player Pawn) |

### 46.22) Formation Follow Extension Points

To extend follow system with formation offsets:

| New Component | Parent | Adds |
|---------------|--------|------|
| Goal_FormationFollow | Goal_FollowCharacter | FormationOffset (Vector) |
| BB_FormationFollow | New blackboard | TargetLocation (Vector) key |
| BTS_CalculateFormationPosition | BTService_BlueprintBase | World position calculation |
| BT_FormationFollow | New tree | Uses TargetLocation for MoveTo |
| BPA_FormationFollow | NPCActivity | SetupBlackboard with offset |

Formation Position Calculation:

| Step | Formula |
|------|---------|
| 1 | Get FollowTarget Location |
| 2 | Get FollowTarget Rotation |
| 3 | Rotate FormationOffset by TargetRotation |
| 4 | TargetLocation = FollowTarget.Location + RotatedOffset |

### 46.23) Asset Locations

| Asset | Path |
|-------|------|
| Goal_FollowCharacter | /Content/NarrativePro/AI/Follow/ |
| BPA_FollowCharacter | /Content/NarrativePro/AI/Follow/ |
| BB_FollowCharacter | /Content/NarrativePro/AI/Follow/ |
| BT_FollowCharacter | /Content/NarrativePro/AI/Follow/ |
| BTS_SetAIFocus | /Content/NarrativePro/AI/Follow/ |
| BTS_AdjustFollowSpeed | /Content/NarrativePro/AI/Follow/ |
| BPC_HasFollowGoalFor | /Content/NarrativePro/AI/Follow/ |
| BPE_RemoveFollowGoal | /Content/NarrativePro/AI/Follow/ |
| BPT_FollowNPCToLocation | /Content/NarrativePro/AI/Follow/ |

---

## SECTION 47: NARRATIVE PRO v2.2 NEW SYSTEMS OVERVIEW

### 47.1) Version Comparison

| Metric | v2.1 | v2.2 | Change |
|--------|------|------|--------|
| Header Files | 330 | 369 | +39 |
| Source Files | 321 | 353 | +32 |
| Total Files | 651 | 722 | +71 |
| Total Lines | 60,685 | 86,965 | +26,280 (+43.3%) |

### 47.2) New File Categories

| Category | New Files | New Lines | Key Components |
|----------|-----------|-----------|----------------|
| Traffic/Road System | 18 | 1,802 | TrafficLightSubsystem, QuestRoadControls |
| Mass AI/Collision | 19 | 1,161 | IncomingCollisionProcessor, MassVehicleSpawner |
| Cover System | 11 | 1,093 | NarrativeRecastNavMesh, NarrativeTileCoverGenerator |
| Actor/Spawn/Mount | 8 | 463 | MountComponent, ActorSpawnComponent |
| Item Fragments | 4 | 113 | AmmoFragment, PoisonableFragment |
| Ability/Projectile | 5 | 259 | NarrativeProjectile, AbilityTask_SpawnProjectile |
| Other | 11 | 688 | NarrativeNavigationSystem, GameplayDebuggerCategory_NChar |

### 47.3) Major File Line Changes

| File | v2.1 | v2.2 | Change | Notes |
|------|------|------|--------|-------|
| WeaponItem.cpp | 397 | 773 | +376 (+94.7%) | Attachment system expansion |
| ArsenalStatics.cpp | 1,276 | 1,906 | +630 (+49.4%) | New utility functions |
| EquippableItem.cpp | 255 | 504 | +249 (+97.6%) | Fragment integration |
| NarrativeCharacter.cpp | 1,642 | 1,804 | +162 (+9.9%) | Melee/mount additions |
| WeaponItem.h | 217 | 352 | +135 (+62.2%) | New properties |

### 47.4) Backward Compatibility

All v2.1 systems remain fully functional:

| System | Status | Migration Notes |
|--------|--------|-----------------|
| NarrativeNPCCharacter | Compatible | No changes required |
| NarrativeGameplayAbility | Compatible | No changes required |
| EquippableItem | Compatible | Fragment system is additive |
| AbilityConfiguration | Compatible | No changes required |
| NPCDefinition | Compatible | No changes required |

### 47.5) C++ Source Locations (New in v2.2)

| System | Header | Source |
|--------|--------|--------|
| NarrativeProjectile | NarrativeProjectile.h | NarrativeProjectile.cpp |
| AbilityTask_SpawnProjectile | AbilityTask_SpawnProjectile.h | (Header only) |
| AmmoFragment | AmmoFragment.h | AmmoFragment.cpp |
| PoisonableFragment | PoisonableFragment.h | PoisonableFragment.cpp |
| NarrativeRecastNavMesh | NarrativeRecastNavMesh.h | NarrativeRecastNavMesh.cpp |
| NarrativeTileCoverGenerator | NarrativeTileCoverGenerator.h | NarrativeTileCoverGenerator.cpp |
| CoverTypes | CoverTypes.h | CoverTypes.cpp |
| MountComponent | MountComponent.h | MountComponent.cpp |
| NarrativeGASStatics | NarrativeGASStatics.h | NarrativeGASStatics.cpp |

---

## SECTION 48: PROJECTILE SYSTEM

### 48.1) System Overview

Narrative Pro v2.2 introduces a native projectile system with automatic ability task integration and target data handling.

| Component | Purpose |
|-----------|---------|
| NarrativeProjectile | Base projectile actor with owner tracking |
| AbilityTask_SpawnProjectile | Native ability task for projectile spawning |
| SetProjectileTargetData() | Hit data broadcast to ability |

### 48.2) NarrativeProjectile Class

Location: NarrativeProjectile.h/cpp

| Property/Function | Type | Purpose |
|-------------------|------|---------|
| OwningCharacter | NarrativeCharacter* | Cached owner reference |
| GetNarrativeCharacter() | Function | Returns owning character |
| SetProjectileTargetData() | Function | Broadcasts hit data to spawning ability task |

### 48.3) AbilityTask_SpawnProjectile

Location: AbilityTask_SpawnProjectile.h

| Property | Type | Purpose |
|----------|------|---------|
| ProjectileClass | TSubclassOf | Class to spawn |
| SpawnTransform | FTransform | Spawn location/rotation |
| OnTargetData | Delegate | Fires when projectile hits |
| OnDestroyed | Delegate | Fires when projectile destroyed without hit |

### 48.4) AbilityTask_SpawnProjectile Usage Pattern

| Step | Action |
|------|--------|
| 1 | Create AbilityTask_SpawnProjectile node |
| 2 | Set ProjectileClass to NarrativeProjectile child |
| 3 | Set SpawnTransform from socket/muzzle |
| 4 | Bind OnTargetData -> Apply damage |
| 5 | Bind OnDestroyed -> Handle miss |
| 6 | Call ReadyForActivation |

### 48.5) Projectile Hit Flow

| Step | Component | Action |
|------|-----------|--------|
| 1 | NarrativeProjectile | Detects collision |
| 2 | NarrativeProjectile | Calls SetProjectileTargetData() |
| 3 | AbilityTask_SpawnProjectile | Receives target data |
| 4 | AbilityTask_SpawnProjectile | Broadcasts OnTargetData delegate |
| 5 | Spawning Ability | Receives target data, applies damage |

### 48.6) Migration from Custom Projectiles

| v2.1 Pattern | v2.2 Alternative | Benefit |
|--------------|------------------|---------|
| Custom Actor + SpawnActorFromClass | NarrativeProjectile child | Built-in owner tracking |
| Manual hit detection | SetProjectileTargetData() | Automatic ability integration |
| Custom target data | AbilityTask delegates | Standardized flow |

### 48.7) Father Projectile Integration

For GA_FatherLaserShot and GA_TurretShoot:

| Property | Value |
|----------|-------|
| Parent for Projectile | NarrativeProjectile |
| Spawn Task | AbilityTask_SpawnProjectile |
| Hit Handling | OnTargetData -> Apply GE_FatherLaserDamage |

---

## SECTION 49: MELEE MULTI-HIT SYSTEM

### 49.1) System Overview

Narrative Pro v2.2 enhances the melee combat system with multi-target support and animation-cached hit detection.

| Feature | Description | Location |
|---------|-------------|----------|
| Multi-Target Trace | Hit multiple enemies per swing | NarrativeCombatAbility |
| CachedHitActors | Prevent double-hits per swing | WeaponVisual |
| Animation Caching | Frame-accurate hits at any FPS | WeaponVisual |

### 49.2) FCombatTraceData Structure

Location: NarrativeCombatAbility.h

| Property | Type | Default | Purpose |
|----------|------|---------|---------|
| TraceDistance | float | 500.0 | Max trace range |
| TraceRadius | float | 0.0 | Sphere sweep radius (0 = line trace) |
| bTraceMulti | bool | false | Enable multi-target hits |

### 49.3) Multi-Hit Configuration

To enable multi-hit in combat abilities:

| Property | Single-Hit | Multi-Hit |
|----------|------------|-----------|
| bTraceMulti | false | true |
| TraceRadius | 0-50 | 50-100 |
| Damage | Full | Reduced (balance) |

### 49.4) CachedHitActors Pattern

Prevents hitting the same actor multiple times per swing:

| Step | Action |
|------|--------|
| 1 | Create CachedHitActors variable (Array of Actor) |
| 2 | In HandleTargetData, check if actor in array |
| 3 | If not in array, apply damage and add to array |
| 4 | If in array, skip (already hit this swing) |
| 5 | In EndAbility, clear the array |

### 49.5) HandleTargetData Override Pattern

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event Handle Target Data | Receive multi-hit data |
| 2 | Get Num (Target Data) | Get hit count |
| 3 | For Loop (0 to Num-1) | Iterate hits |
| 4 | Get Hit Result From Target Data | Extract hit |
| 5 | Break Hit Result -> Hit Actor | Get actor |
| 6 | Contains (CachedHitActors, Actor) | Check cache |
| 7 | Branch | If not cached, continue |
| 8 | Add (CachedHitActors, Actor) | Add to cache |
| 9 | Call Parent Handle Target Data | Apply damage |

### 49.6) WeaponVisual Animation Caching (Reference)

Location: WeaponVisual.h/cpp

| Function | Purpose |
|----------|---------|
| CacheAnimationTransform() | Pre-sample weapon positions |
| PerformCollisionCheck() | Sweep between cached positions |
| CleanupAttackData() | Reset for next attack |
| CacheCollisionData() | Build collision shapes |

Note: Animation caching requires WeaponVisual actor - not applicable to father (uses socket-based trace instead).

### 49.7) FWeaponCollisionData Structure

Location: WeaponAnimPose.h

| Property | Type | Purpose |
|----------|------|---------|
| RelativeLocation | FVector | Position relative to bone |
| RelativeRotation | FQuat | Rotation relative to bone |
| LastLocation | FVector | Previous frame position |
| Shape | FCollisionShape | Capsule collision |

### 49.8) Debug Console Variables

| CVar | Values | Purpose |
|------|--------|---------|
| n.gas.DrawMeleeTraces | 0=Off, 1=Player, 2=All | Visualize melee traces |
| n.gas.DrawMeleeAimAssist | 0=Off, 1=Player | Visualize warp targeting |
| n.gas.DrawTraces | 0=Off, 1=On | General combat traces |
| n.gas.TransformProvider.Debug | 0=Off, 1=On | Transform provider debug |

### 49.9) Father Multi-Hit Configuration (GA_FatherAttack v3.3)

| Property | v3.2 Value | v3.3 Value | Change |
|----------|------------|------------|--------|
| TraceDistance | inherited | 200.0 | Father range |
| TraceRadius | 65.0 | 75.0 | Wider sweep |
| bTraceMulti | false | true | Multi-target |
| Default Attack Damage | 10.0 | 8.0 | Balanced for multi-hit |
| CachedHitActors | N/A | Array of Actor | New variable |

---

## SECTION 50: ITEM FRAGMENT SYSTEM

### 50.1) System Overview

Narrative Pro v2.2 introduces item fragments - composable data objects that can be added to any NarrativeItem to extend functionality without creating new item classes.

| Fragment | Purpose | Location |
|----------|---------|----------|
| AmmoFragment | Custom ammo properties | AmmoFragment.h/cpp |
| PoisonableFragment | Poison application | PoisonableFragment.h/cpp |

### 50.2) Fragment Base Class

Location: NarrativeItem.h

| Class | Parent | Purpose |
|-------|--------|---------|
| UNarrativeItemFragment | UObject | Base fragment class |

Fragment Access:

| Function | Return | Purpose |
|----------|--------|---------|
| GetFragment(Class) | Fragment or nullptr | Get single fragment |
| GetFragments(Class) | Array of Fragments | Get all of type |

### 50.3) AmmoFragment

Location: AmmoFragment.h/cpp

| Property | Type | Purpose |
|----------|------|---------|
| AmmoDamageOverride | float | Override default ammo damage |
| DamageEffect | TSubclassOf of GE | Custom damage GameplayEffect |
| ProjectileClass | TSubclassOf of Projectile | Custom projectile actor |
| bOverrideTraceData | bool | Use custom trace settings |
| TraceData | FCombatTraceData | Custom trace configuration |

### 50.4) AmmoFragment Usage Pattern

| Step | Action |
|------|--------|
| 1 | Create AmmoItem child blueprint |
| 2 | In Details, add Fragment (AmmoFragment) |
| 3 | Configure AmmoDamageOverride, ProjectileClass, etc. |
| 4 | In weapon ability, get AmmoFragment from ammo item |
| 5 | Use fragment properties for damage/projectile |

### 50.5) PoisonableFragment

Location: PoisonableFragment.h/cpp

| Property/Function | Type | Purpose |
|-------------------|------|---------|
| AppliedPoison | TSubclassOf of GE | Currently applied poison |
| SetPoison(GE) | Function | Apply poison effect |
| ConsumePoison() | Function | Use and remove poison |
| CanBePoisonedBy(Poison) | Function | Validation check |

### 50.6) PoisonableFragment Usage Pattern

| Step | Action |
|------|--------|
| 1 | Add PoisonableFragment to weapon/ammo item |
| 2 | Call SetPoison(GE_PoisonEffect) to apply poison |
| 3 | In attack ability, get PoisonableFragment |
| 4 | Call ConsumePoison() to get and clear poison |
| 5 | Apply returned poison GE to target |

### 50.7) Fragment Composition Example

A single item can have multiple fragments:

| Item | Fragments | Combined Effect |
|------|-----------|-----------------|
| Poisoned Fire Arrow | AmmoFragment + PoisonableFragment | Custom projectile + poison on hit |
| Enchanted Sword | PoisonableFragment | Melee with poison application |

### 50.8) Father Fragment Integration (Future)

Potential father weapon mod system using fragments:

| Father Mod | Fragment | Effect |
|------------|----------|--------|
| Venom Fangs | PoisonableFragment | Father attack applies poison |
| Acid Spit | AmmoFragment | Custom projectile for acid attack |
| Electric Fangs | Custom ElectricFragment | Chain damage on hit |

---

## SECTION 51: COVER SYSTEM

### 51.1) System Overview

Narrative Pro v2.2 adds a tactical cover system with navmesh integration and async cover generation.

| Component | Purpose | Location |
|-----------|---------|----------|
| NarrativeRecastNavMesh | Extended NavMesh with cover | NarrativeRecastNavMesh.h/cpp |
| NarrativeTileCoverGenerator | Async cover processor | NarrativeTileCoverGenerator.h/cpp |
| CoverTypes | Data structures | CoverTypes.h/cpp |

### 51.2) Cover Query Functions

Location: NarrativeRecastNavMesh/NarrativeNavigationSystem

| Function | Purpose |
|----------|---------|
| FindAllCoverInRadiusToPoint() | Get all cover positions within radius |
| FindNearestCoverToPoint() | Get closest cover to location |
| TestPointForCoverType() | Check if point provides cover |

### 51.3) Cover Configuration

| Parameter | Default | Purpose |
|-----------|---------|---------|
| CoverSpacing | 150.0 | Distance between cover points |
| HalfHeight | 85.0 | Crouch cover height |
| PeekOverHeight | 150.0 | Stand-to-peek height |

### 51.4) FCoverChain Structure

Location: CoverTypes.h

| Property | Type | Purpose |
|----------|------|---------|
| Links | TArray of FCoverLink | Cover positions in chain |
| NavNodeRef | NavNodeRef | NavMesh reference |

### 51.5) FCoverLink Structure

| Property | Type | Purpose |
|----------|------|---------|
| Location | FVector | Cover position |
| Normal | FVector | Cover facing direction |
| CoverType | ECoverType | Low/High/Peek |

### 51.6) EQS Integration

Cover system integrates with Environment Query System:

| Query | Purpose |
|-------|---------|
| EQS_FindCoverFromEnemy | Find cover protecting from threat |
| EQS_FindFlankingCover | Find cover for flanking maneuver |

### 51.7) Blackboard Keys for Cover

| Key | Type | Used By |
|-----|------|---------|
| Cover | Object | BB_Attack_Cover |
| CoverLocation | Vector | Movement tasks |
| CoverNormal | Vector | Orientation |

### 51.8) Father Cover Usage (Reference)

Father companion could use cover system for:

| Behavior | Cover Usage |
|----------|-------------|
| Engineer Turret Placement | FindAllCoverInRadiusToPoint for strategic positions |
| Crawler Combat | TestPointForCoverType for ambush positions |
| Electric Trap | Cover points as trap locations |

---

## SECTION 52: NARRATIVE PRO 2.2 CONTENT FOLDER STRUCTURE

### 52.1) Plugin Content Root

Location: Plugins/Narrative Pro/Pro/

| Folder | Purpose |
|--------|---------|
| Core/ | Production-ready base assets |
| Demo/ | Example implementations |
| Editor/ | Editor utilities |

### 52.2) Core Folder Structure

| Path | Contents |
|------|----------|
| Core/Abilities/ | Configurations, Cues, GameplayAbilities, GameplayEffects |
| Core/AI/ | Activities, BP, Configs, EQS, Mass, Services, Tasks |
| Core/Audio/ | Attenuations, Blueprints, Classes, Data, Foley, MetaSounds, Mix, Waves |
| Core/BP/ | FastTravel, Framework, Interaction, NarrativeSky, Navigator, SaveSystem, Vehicles, WeaponFX, WeaponVisuals, World |
| Core/Character/ | Biped, BP |
| Core/CharCreator/ | Character creation assets |
| Core/Data/ | Curves, Input, Physics, SkillTrees |
| Core/Inventory/ | Items (Book, Clothing, Weapons, etc.) |
| Core/Maps/ | MainMenu |
| Core/Tales/ | Blueprint, Conditions, Events, Requirements, Shots, Tasks, Triggers |
| Core/UI/ | Audio, Blueprint, ControllerData, Fonts, Materials, Menus, Style, Textures, Widgets |
| Core/VFX/ | Blood, Decals, Grenade, Impacts, MuzzleFlash, Noise, Projectiles, Sword_Trail, VehicleDamage |
| Core/Weapons/ | Ammo, Attachments, Bow, Grenade, Materials, Pistol, Rifle, Shield, Sword, Wand |

### 52.3) Abilities Folder Detail

| Subfolder | Contents | Naming |
|-----------|----------|--------|
| Configurations/ | AC_NPC_Default, AC_Player_RPG, AC_Player_Shooter, AC_Vehicle_Default | AC_* |
| Cues/ | Bursts/, OverlayEffect/, TakeDamage/ | GC_* |
| GameplayAbilities/ | Attacks/, Interacts/, Movement/, Weapons/, GA_Death | GA_* |
| GameplayEffects/ | Attributes/, Equipment/, GE_Damage_SetByCaller, GE_Heal_SetByCaller, etc. | GE_* |

### 52.3.1) Attack Abilities - Melee

| Asset | Type | Notes |
|-------|------|-------|
| GA_Attack_Combo_Melee | Blueprint Class | Main hand combo |
| GA_Attack_Combo_Melee_Offhand | Blueprint Class | NEW 2.2 - Offhand combo (dual wield) |
| GA_Melee_Punch_Unarmed | Blueprint Class | Unarmed melee (father parent) |
| GA_Weapon_Bash | Blueprint Class | Weapon bash attack |

### 52.3.2) Attack Abilities - Firearms

| Asset | Type | Notes |
|-------|------|-------|
| GA_Attack_Firearm_Base | Blueprint Class | Base class for firearms |
| GA_Attack_Firearm_Proj_Launcher | Blueprint Class | Projectile launcher |
| GA_Attack_Firearm_Projectile | Blueprint Class | Projectile-based weapons |
| GA_Attack_Firearm_Trace | Blueprint Class | Hitscan/trace weapons |
| GA_Firearm_Pistol | Blueprint Class | Pistol implementation |
| GA_Firearm_Pistol_R | Blueprint Class | NEW 2.2 - Right hand pistol (dual wield) |
| GA_Firearm_Rifle | Blueprint Class | Rifle implementation |

### 52.3.3) Attack Abilities - Magic

| Asset | Type | Notes |
|-------|------|-------|
| GA_Attack_Magic_Beam | Blueprint Class | Beam/channeled magic |
| GA_Attack_Magic_Beam_Offhand | Blueprint Class | NEW 2.2 - Offhand beam (dual cast) |
| GA_Attack_Magic_Proj | Blueprint Class | Projectile magic |
| GA_Attack_Magic_Proj_Offhand | Blueprint Class | NEW 2.2 - Offhand projectile (dual cast) |

### 52.3.4) Attack Abilities - Misc

| Asset | Type | Purpose |
|-------|------|---------|
| GA_Attack_Magic_Fall | Blueprint Class | Fall damage/ground slam |
| GA_Attack_Magic_NPC_Control | Blueprint Class | Mind control/possess NPC |
| GA_Attack_Magic_Ragdoller | Blueprint Class | Force push/ragdoll effect |

### 52.3.5) Attack Abilities - Projectiles

| Asset | Type | Purpose |
|-------|------|---------|
| BP_Weapon_Projectile_Base | Blueprint Class | Base projectile class |
| BP_Weapon_Projectile_Grenade | Blueprint Class | Grenade projectile |
| BP_Weapon_Projectile_HeliMissile | Blueprint Class | Homing missile |
| BP_Weapon_Projectile_Launcher | Blueprint Class | Generic launcher projectile |
| BP_Weapon_Projectile_Magic_Fireball | Blueprint Class | Magic fireball |
| CF_Fireball | Float Curve | Fireball damage curve |
| CF_Grenade_Damage | Float Curve | Grenade damage falloff |
| WBP_Grenade_Icon | Widget Blueprint | Grenade UI indicator |

### 52.3.6) Attack Abilities - BowArrows

| Asset | Type | Notes |
|-------|------|-------|
| BP_Weapon_Projectile_Bow_Arrow | Blueprint Class | Standard arrow |
| BP_Weapon_Projectile_Bow_Arrow_Explosive | Blueprint Class | NEW 2.2 - Explosive arrow |
| BP_Weapon_Projectile_Dark_Arrow | Blueprint Class | NEW 2.2 - Dark/magic arrow |
| BP_Weapon_Projectile_Iron_Arrow | Blueprint Class | NEW 2.2 - Heavy/armor-piercing arrow |

### 52.3.7) Attack Abilities - Throwables

| Asset | Type | Purpose |
|-------|------|---------|
| GA_Attack_ThrowGrenade | Blueprint Class | Grenade throw ability |

### 52.5) AI Folder Detail

| Subfolder | Contents |
|-----------|----------|
| Activities/ | Attacks, DriveTo, Flee, FlyTo, FollowCharacter, GoToLocation, Idle, Interact, Patrol, ReturnToSpawn, Tasks |
| BP/ | BP_NarrativeNPC, BP_NarrativeNPCController |
| Configs/ | AC_CrowdNPC, AC_Default, AC_Marksman, AC_Melee, AC_Melee_Ranger, AC_Pacifist, AC_RunAndGun |
| EQS/ | EQSContext_PlayerPawn |
| Mass/ | Pedestrians, Vehicles |
| Services/ | BTS_ApplyGameplayTags, BTS_ClearAIFocus, BTS_EquipAndWieldWeapon, BTS_ExecuteOnce, BTS_FocusAttackTarget, BTS_SetAIFocus |
| Tasks/ | BTS_PlayTaggedDialogue, BTT_SetBehaviorTree, BTTask_ActivateAbilityByClass, BTTask_ActivateAbilityByInput, BTTask_RotateToGoal |

### 52.6) Framework Blueprints

| Asset | Type | Purpose |
|-------|------|---------|
| BP_NarrativeCameraManager | Blueprint Class | Camera management |
| BP_NarrativeController_MainMenu | Blueprint Class | Main menu controller |
| BP_NarrativeGameInstance | Blueprint Class | Game instance |
| BP_NarrativeGameMode | Blueprint Class | Base game mode |
| BP_NarrativeGameMode_MainMenu | Blueprint Class | Main menu game mode |
| BP_NarrativeGameState | Blueprint Class | Game state |
| BP_NarrativePlayerController | Blueprint Class | Player controller |
| BP_NarrativePlayerState | Blueprint Class | Player state |

### 52.7) Demo Folder Structure

| Path | Contents |
|------|----------|
| Demo/BP/ | Combat, Framework, Interaction, Inventory, TimeOfDay, Demo buttons |
| Demo/Character/Definitions/ | Bandit, BanditLeader, Becky, Felix, Luca, Marco, Marvin, Nirvana, Player, Seth, Spud |
| Demo/Cinematics/ | BlindBeggar, IntroSequence, PlayerRobbery, Preacher, Robbery, TestAnims |
| Demo/Items/Examples/ | Items (Clothing, EffectItems, Misc, Weapons), LootTables |
| Demo/Quests/ | BP, LearningToDrive, OpenWorld, SecretMerchant, QBP_DemoQuestBase |

### 52.8) Key Reference Assets for Father

| Asset | Location | Father Usage |
|-------|----------|--------------|
| AC_NPC_Default | Core/Abilities/Configurations | Template for AC_FatherCompanion |
| GA_Death | Core/Abilities/GameplayAbilities | Death handling reference |
| GE_Damage_SetByCaller | Core/Abilities/GameplayEffects | SetByCaller damage pattern |
| BTTask_ActivateAbilityByClass | Core/AI/Tasks | AI-triggered abilities |
| BTTask_ActivateAbilityByInput | Core/AI/Tasks | Tag-based ability activation |
| BPA_FollowCharacter | Core/AI/Activities/FollowCharacter | Follow player activity |
| BP_NarrativeNPC | Core/AI/BP | Base NPC blueprint |
| BP_NarrativeNPCController | Core/AI/BP | Base NPC controller |

---

## SECTION 53: NARRATIVE PRO 2.2 C++ MODULE STRUCTURE

### 53.1) Module Overview

| Module | Purpose | Father Relevance |
|--------|---------|------------------|
| NarrativeArsenal | Core gameplay, GAS, AI, Items | Critical |
| NarrativeArsenalEditor | Asset factories, editor tools | NPCDefinition creation |
| NarrativeArsenalPreEditor | Pre-editor utilities | Low |
| NarrativeCommonUI | UI framework | Father UI widgets |
| NarrativeDialogueEditor | Dialogue editor | Father barks |
| NarrativeQuestEditor | Quest editor | Quest integration |
| NarrativeSaveSystem | Save/load system | Father persistence |

### 53.2) NarrativeArsenal Public Folders

| Folder | Key Classes |
|--------|-------------|
| AI/ | NPCDefinition, NarrativeNPCController, EnvQueryTest_Team, EnvQueryTest_AttackPriority |
| AI/Activities/ | NPCActivity, NPCActivityComponent, NPCActivityConfiguration, NPCGoal, NPCGoalGenerator |
| AI/BehaviorTree/Tasks/ | BTTask_LongMove |
| AI/Navigation/ | NarrativeNavigationSystem, NarrativeRecastNavMesh |
| Camera/ | NarrativeCameraComponent, NarrativeCameraMode |
| Character/ | CharacterDefinition, PlayerDefinition, NarrativeCharacterMovement, NarrativeCharacterVisual |
| Components/ | EquipmentComponent, InventoryComponent, MountComponent, PartyComponent, SkillTreeComponent |
| GAS/ | Core GAS classes (see 53.3) |
| GAS/AbilityTasks/ | AbilityTask_MoveToLocationAndWait, AbilityTask_RotateActor, AbilityTask_SpawnProjectile |
| Items/ | NarrativeItem, EquippableItem, WeaponItem, AmmoItem, AmmoFragment, PoisonableFragment |
| Spawners/ | NPCSpawner, NPCSpawnComponent, SpawnerBase |
| Weapons/ | WeaponVisual, FirearmWeaponVisual, MeleeWeaponItem, RangedWeaponItem |

### 53.3) GAS Folder Classes

| Class | Purpose | Father Usage |
|-------|---------|--------------|
| AbilityConfiguration | Data asset for ability granting | AC_FatherCompanion |
| NarrativeAbilityInputMapping | Input tag to ability mapping | Tag-based input |
| NarrativeAbilitySystemComponent | Custom ASC | Father ASC |
| NarrativeAnimSet | Animation set for abilities | Father animations |
| NarrativeASCActor | Base actor with ASC | - |
| NarrativeAttributeSetBase | Base attributes (Health, Stamina, etc.) | Father attributes |
| NarrativeCharacterAttributeSet | Character-specific attributes | - |
| NarrativeCombatAbility | Base combat ability | GA_FatherAttack parent |
| NarrativeDamageExecCalc | Damage calculation | Father damage |
| NarrativeGameplayAbility | Base ability class | All father abilities |
| NarrativeGASStatics | GAS utility functions | Utility functions |
| NarrativeHealExecution | Heal calculation | Father healing |
| NarrativeInteractAbility | Interaction ability | - |

### 53.4) Custom Ability Tasks

| Task | Purpose | Father Usage |
|------|---------|--------------|
| AbilityTask_MoveToLocationAndWait | Move to location and wait | Father movement abilities |
| AbilityTask_RotateActor | Rotate actor over time | Father targeting |
| AbilityTask_SpawnProjectile | Spawn projectile | GA_FatherLaserShot, GA_TurretShoot |

### 53.5) Save System Classes

| Class | Purpose | Father Usage |
|-------|---------|--------------|
| NarrativeSavableActor | Base savable actor | Father save support |
| NarrativeSavableComponent | Savable component | Alternative save approach |
| NarrativeSave | Save data class | Father state |
| NarrativeStableActor | Persistent actor | Cross-level persistence |
| SaveSystemStatics | Utility functions | Save operations |

### 53.6) Editor Module Classes

| Class | Purpose |
|-------|---------|
| NPCDefinitionFactory | Creates NPCDefinition assets |
| PlayerDefinitionFactory | Creates PlayerDefinition assets |
| GameplayEffectItemBlueprintFactory | Creates GE item blueprints |
| WeaponItemBlueprintFactory | Creates weapon item blueprints |

### 53.7) UI Module Classes

| Class | Purpose |
|-------|---------|
| NarrativeActivatableWidget | Base activatable widget |
| NarrativeCommonHUD | Common HUD base |
| NarrativeCommonUIFunctionLibrary | UI utility functions |
| NarrativeCommonUISubsystem | UI subsystem |

### 53.8) Key C++ File Locations

| Class | Header | Source |
|-------|--------|--------|
| NarrativeGameplayAbility | NarrativeArsenal/Public/GAS/ | NarrativeArsenal/Private/GAS/ |
| NarrativeCombatAbility | NarrativeArsenal/Public/GAS/ | NarrativeArsenal/Private/GAS/ |
| NPCDefinition | NarrativeArsenal/Public/AI/ | NarrativeArsenal/Private/AI/ |
| AbilityConfiguration | NarrativeArsenal/Public/GAS/ | NarrativeArsenal/Private/GAS/ |
| NarrativeNPCController | NarrativeArsenal/Public/AI/ | NarrativeArsenal/Private/AI/ |
| NarrativeProjectile | NarrativeArsenal/Public/Weapons/ | NarrativeArsenal/Private/Weapons/ |

---

## SECTION 54: BEHAVIOR TREE TASK SYSTEM

### 54.1) Overview

Narrative Pro provides built-in BT Tasks for AI ability activation. These should be used instead of creating custom tasks per ability.

### 54.2) BTTask_ActivateAbilityByClass

| Property | Description |
|----------|-------------|
| Location | Plugins/Narrative Pro/Pro/Core/AI/Tasks/ |
| Type | Blueprint Task (BTTask_BlueprintBase child) |
| Purpose | Activate any GameplayAbility by class reference |

### 54.3) Class Defaults Configuration

| Property | Value | Purpose |
|----------|-------|---------|
| Ability to Activate | None (dropdown) | Select ability class to trigger |
| Custom Description | "Activate Ability" | Node display name in BT |
| Tick Interval | -1.0 | No ticking (instant execution) |

### 54.4) Event Graph Flow

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event Receive Execute AI | Entry point |
| 2 | Controlled Pawn | Get AI-controlled pawn |
| 3 | Get Ability System Component | Get ASC from pawn (generic, no cast needed) |
| 4 | Try Activate Ability by Class | Target: ASC, Ability: variable, Allow Remote: checked |
| 5 | Finish Execute | Success: Return Value from activation |

### 54.5) Comparison: Built-in vs Custom Tasks

| Aspect | BTTask_ActivateAbilityByClass | Custom Tasks |
|--------|------------------------------|--------------|
| Task Count | ONE reusable task | Multiple (one per ability) |
| Casting | NO cast needed | Cast to BP_FatherCompanion |
| Configuration | Select class in BT node dropdown | Hardcoded tag per task |
| Maintenance | None (plugin provided) | Must maintain Blueprint tasks |
| Node Count | 5 nodes | 8+ nodes |

### 54.6) Father Usage Pattern

| BT Node Instance | Ability to Activate Parameter |
|------------------|-------------------------------|
| BTTask_ActivateAbilityByClass | GA_FatherAttack |
| BTTask_ActivateAbilityByClass | GA_FatherLaserShot |
| BTTask_ActivateAbilityByClass | GA_TurretShoot |
| BTTask_ActivateAbilityByClass | GA_FatherElectricTrap |

### 54.7) BTTask_ActivateAbilityByInput

| Property | Description |
|----------|-------------|
| Location | Plugins/Narrative Pro/Pro/Core/AI/Tasks/ |
| Type | Blueprint Task |
| Purpose | Activate ability by InputTag (tag-based) |

### 54.8) Guides Requiring Update

| Guide | Current Custom Task | Update Required |
|-------|---------------------|-----------------|
| GA_FatherLaserShot_Implementation_Guide | BTTask_ActivateLaserShot | Replace with BTTask_ActivateAbilityByClass |
| GA_TurretShoot_Implementation_Guide | BTTask_ActivateFatherAbility | Replace with BTTask_ActivateAbilityByClass |
| GA_FatherElectricTrap_Implementation_Guide | BTTask_ActivateAbility | Update to BTTask_ActivateAbilityByClass |

---

## SECTION 55: BEHAVIOR TREE SERVICES

### 55.1) Available Services

| Service | Purpose | Father Relevance |
|---------|---------|------------------|
| BTS_ApplyGameplayTags | Apply tags during BT execution | Low |
| BTS_ClearAIFocus | Clear AI focus target | Medium |
| BTS_EquipAndWieldWeapon | Equip weapon during combat | None (natural attacks) |
| BTS_ExecuteOnce | Run logic once per BT activation | Low |
| BTS_FocusAttackTarget | Keep AI focused on attack target | High |
| BTS_SetAIFocus | Set AI focus to specific target | High (via BT_FollowCharacter) |
| BTS_AdjustFollowSpeed | Match movement speed to target | High (via BT_FollowCharacter) |

### 55.2) BTS_FocusAttackTarget

| Property | Value |
|----------|-------|
| Location | Plugins/Narrative Pro/Pro/Core/AI/Services/ |
| Type | Blueprint Service (BTService_BlueprintBase) |
| Interval | 0.1 seconds |
| Random Deviation | 0.1 |

### 55.3) BTS_FocusAttackTarget Event Graph

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event Receive Tick AI | Service ticks every 0.1s |
| 2 | Cast To NarrativeNPCController | Get NPC controller |
| 3 | NPCActivityComponent -> Get Current Activity Goal | Get active goal |
| 4 | Cast To Goal_Attack | Check if in attack goal |
| 5 | Target -> Last Sight Stimulus | Get attack target stimulus |
| 6 | Break AIStimulus | Extract Stimulus Location |
| 7 | Get AIController -> SetFocalPoint | Set AI focus to target location |

### 55.4) BTS_ClearAIFocus

| Property | Value |
|----------|-------|
| Location | Plugins/Narrative Pro/Pro/Core/AI/Services/ |
| Type | Blueprint Service (BTService_BlueprintBase) |
| Node Name | Clear Focus |
| Interval | 0.5 |
| Random Deviation | 0.1 |

### 55.5) BTS_ClearAIFocus Event Graph

| Step | Node | Connection |
|------|------|------------|
| 1 | Event Receive Activation AI | Entry point (fires once on activation) |
| 2 | Get AIController | Controlled Actor: from Controlled Pawn |
| 3 | ClearFocus | Target: AIController Return Value |

### 55.6) BTS_SetAIFocus

| Property | Value |
|----------|-------|
| Location | Plugins/Narrative Pro/Pro/Core/AI/Services/ |
| Type | Blueprint Service (BTService_BlueprintBase) |
| Focus Target | None (Blackboard Key dropdown) |
| Interval | 0.5 |
| Random Deviation | 0.1 |

### 55.7) BTS_SetAIFocus Event Graph - Activation

| Step | Node | Connection |
|------|------|------------|
| 1 | Event Receive Activation AI | Entry point |
| 2 | Get AIController | Controlled Actor: from Controlled Pawn |
| 3 | Get Blackboard Value as Actor | Key: Focus Target variable |
| 4 | SetFocus | Target: AIController, New Focus: Blackboard actor |

### 55.8) BTS_SetAIFocus Event Graph - Deactivation

| Step | Node | Connection |
|------|------|------------|
| 1 | Event Receive Deactivation AI | Entry point (fires when leaving scope) |
| 2 | Get AIController | Controlled Actor: from Controlled Pawn |
| 3 | ClearFocus | Target: AIController |

### 55.9) BTS_SetAIFocus Auto-Cleanup

| Trigger | Action |
|---------|--------|
| Service Activates | SetFocus to blackboard actor |
| Service Deactivates | ClearFocus automatically |

### 55.10) BTS_AdjustFollowSpeed

| Property | Value |
|----------|-------|
| Location | Plugins/Narrative Pro/Pro/Core/AI/Services/ |
| Type | Blueprint Service (BTService_BlueprintBase) |
| Walk Tag | Narrative.State.Movement.Walking |
| Interval | 0.5 |
| Random Deviation | 0.1 |

### 55.11) BTS_AdjustFollowSpeed Event Graph

| Step | Node | Connection |
|------|------|------------|
| 1 | Event Receive Tick AI | Entry point (ticks every 0.5s) |
| 2 | Cast To NarrativeCharacter | Object: Controlled Pawn |
| 3 | Get Narrative Character Movement | Target: As Narrative Character |
| 4 | SET Movement Component | Stores reference for later use |
| 5 | Get Narrative Pro Settings | Gets centralized settings |
| 6 | BBKey Follow Target | Property from settings |
| 7 | Get Owners Blackboard | Gets blackboard component |
| 8 | Get Value as Object | Key Name: BBKey Follow Target |
| 9 | Cast To Actor | Converts blackboard value to actor |
| 10 | Get Distance To | Target: NPC (self), Other Actor: Follow Target |
| 11 | > 575.0 | Compare distance to threshold |
| 12 | Branch | Condition: distance > 575 |
| 13a | True -> Start Sprinting | Target: Movement Component |
| 13b | False -> Stop Sprinting | Target: Movement Component |

### 55.12) BTS_AdjustFollowSpeed Speed Matching Logic

| Distance | Action | Result |
|----------|--------|--------|
| > 575 units | Start Sprinting | NPC runs to catch up |
| <= 575 units | Stop Sprinting | NPC walks normally |

### 55.13) Service Dependencies

| Service | Requires |
|---------|----------|
| BTS_FocusAttackTarget | Goal_Attack active (from GoalGenerator_Attack) |
| BTS_SetAIFocus | Blackboard key with target actor |
| BTS_ClearAIFocus | None |
| BTS_AdjustFollowSpeed | Blackboard key with follow target actor |

### 55.14) Father Service Usage

| Form/Activity | Service | Status |
|---------------|---------|--------|
| Following Player | BTS_AdjustFollowSpeed only | CUSTOM via BT_FatherFollow |
| Crawler Combat | BTS_FocusAttackTarget | Available for combat BT |
| Engineer Combat | BTS_FocusAttackTarget | Available for combat BT |
| Form Switch | None needed | Father has no visible eyes |

### 55.15) Father Custom Follow System

Father uses custom BT_FatherFollow instead of native BT_FollowCharacter:

| Service | BT_FollowCharacter | BT_FatherFollow | Reason |
|---------|-------------------|-----------------|--------|
| BTS_SetAIFocus | Included | REMOVED | Father should not stare at player |
| BTS_AdjustFollowSpeed | Included | KEPT | Father needs speed matching |

Father Follow Behavior:

| Aspect | Before (Native) | After (Custom) |
|--------|-----------------|----------------|
| Look Direction | Stares at player | Faces movement direction |
| Speed Matching | Sprint > 575 units | Sprint > 575 units |
| Blackboard | BB_FollowCharacter | BB_FollowCharacter (reused) |
| Goal | Goal_FollowCharacter | Goal_FollowCharacter (reused) |

Custom Assets Created:

| Asset | Type | Purpose |
|-------|------|---------|
| BT_FatherFollow | Behavior Tree | Follow without BTS_SetAIFocus |
| BPA_FatherFollow | Activity | Uses custom BT |
| ActConfig_FatherCompanion | ActivityConfiguration | Father-specific behaviors |

Reference: Father_Companion_System_Setup_Guide_v2_1.md Phases 37-40

---

## SECTION 56: REFERENCE ASSET ANALYSIS

### 56.1) AC_NPC_Default (AbilityConfiguration)

| Property | Value |
|----------|-------|
| Location | Plugins/Narrative Pro/Pro/Core/Abilities/Configurations/ |
| Default Attributes | GE_DefaultNPCAttributes |
| Startup Effects | 0 (empty) |
| Default Abilities | 6 elements |

### 56.2) AC_NPC_Default Granted Abilities

| Index | Ability | Purpose |
|-------|---------|---------|
| 0 | GA_Melee_Punch_Unarmed | Basic melee attack |
| 1 | GA_Melee_Block | Blocking |
| 2 | GA_Dodge | Evasion |
| 3 | GA_Weapon_Reload | Weapon reload |
| 4 | GA_Weapon_Wield | Weapon equip |
| 5 | GA_Death | Death handling |

### 56.3) AC_Default (ActivityConfiguration)

| Property | Value |
|----------|-------|
| Location | Plugins/Narrative Pro/Pro/Core/AI/Configs/ |
| Rescore Interval | 0.5 seconds |
| Default Activities | 9 elements |
| Goal Generators | 1 element (GoalGenerator_Attack) |

### 56.4) AC_Default Activities

| Index | Activity | Purpose |
|-------|----------|---------|
| 0 | BPA_Attack_Melee | Melee combat |
| 1 | BPA_Attack_Investigate | Investigate sounds/sights |
| 2 | BPA_Attack_Grenade | Grenade throwing |
| 3 | BPA_FollowCharacter | Following |
| 4 | BPA_Patrol | Patrol routes |
| 5 | BPA_Interact | Interact with objects |
| 6 | BPA_Idle | Idle behavior |
| 7 | BPA_MoveToDestination | Go to location |
| 8 | BPA_ReturnToSpawn | Return to spawn |

### 56.5) GA_Death Pattern Analysis

| Property | Value |
|----------|-------|
| InputTag | None |
| Activate Ability on Granted | Unchecked |
| Asset Tags | Abilities.Death |
| Activation Owned Tags | Narrative.State.Weapon.BlockFiring |
| Replication Policy | Do Not Replicate |
| Instancing Policy | Instanced Per Actor |
| Net Execution Policy | Server Only |
| Trigger Tag | GameplayEvent.Death |
| Trigger Source | Gameplay Event |

### 56.6) GA_Death Event Flow

| Step | Action |
|------|--------|
| 1 | Event ActivateAbilityFromEvent (triggered by GameplayEvent.Death) |
| 2 | Parent: ActivateAbilityFromEvent |
| 3 | InformKiller function |

### 56.7) GA_Death InformKiller Function

| Step | Node | Purpose |
|------|------|---------|
| 1 | Break Gameplay Event Data | Get Instigator, Target |
| 2 | Make Gameplay Event Data | Swap roles |
| 3 | Send Gameplay Event to Actor | Send to Instigator |
| 4 | Event Tag | GameplayEvent.KilledEnemy |

### 56.8) Father Death Recommendation

| Decision | Rationale |
|----------|-----------|
| Use GA_Death directly | Built-in, handles InformKiller |
| Add to AC_FatherCompanion_Default | Same pattern as AC_NPC_Default |
| Father-specific logic | Put in BP_FatherCompanion OnDied delegate |

### 56.9) Attack Activities Folder Structure

| Folder | Activity | Blackboard | Behavior Tree |
|--------|----------|------------|---------------|
| MeleeAttack | BPA_Attack_Melee | BB_Attack | BT_Attack_Melee |
| MeleeAttack | BPA_Attack_Melee_Primary | BB_Attack | BT_Attack_Melee |
| ShootAndStrafe | BPA_Attack_Ranged_Strafe | BB_Attack_Strafe | BT_Attack_Ranged |
| ShootFromCover | BPA_Attack_Ranged_Cover | BB_Attack_Cover | BT_Attack_Ranged_Cover |
| Investigate | BPA_Attack_Investigate | - | BT_Attack_Investigate |
| ThrowGrenade | BPA_Attack_Grenade | BB_Attack_Grenade | BT_Attack_Grenade |

### 56.10) SetByCaller Damage Analysis

| Pattern | Armor Applied | Use Case |
|---------|---------------|----------|
| GE_Damage_SetByCaller (Modifier) | NO - bypassed | True damage, environmental |
| NarrativeDamageExecCalc (Execution) | YES - formula | Combat damage with armor |

### 56.11) Father Damage Decision

| Decision | Rationale |
|----------|-----------|
| Keep NarrativeDamageExecCalc | Father damage should respect armor |
| Do not use GE_Damage_SetByCaller | Would bypass enemy defenses |

---

## SECTION 57: FATHER TO NARRATIVE PRO ALIGNMENT

### 57.1) Ability Configuration Mapping

| Father Ability | Matches AC_NPC_Default? | Notes |
|----------------|------------------------|-------|
| GA_FatherCrawler | No | Father-specific form |
| GA_FatherArmor | No | Father-specific form |
| GA_FatherExoskeleton | No | Father-specific form |
| GA_FatherSymbiote | No | Father-specific form |
| GA_FatherEngineer | No | Father-specific form |
| GA_FatherAttack | Similar to GA_Melee_Punch_Unarmed | Melee combat |
| GA_FatherLaserShot | No equivalent | Ranged combat |
| GA_FatherMark | No equivalent | Passive marking |
| GA_FatherSacrifice | No equivalent | Emergency save |
| GA_Death | YES - Same as AC_NPC_Default | ADD THIS |

### 57.2) Activity Configuration Mapping

| Father Activity | Narrative Pro Activity | Status |
|-----------------|----------------------|--------|
| Melee Attack | BPA_Attack_Melee | Mapped |
| Ranged Attack (Laser) | BPA_Attack_Ranged_Strafe | Mapped |
| Turret Shoot | BPA_Attack_Ranged | Mapped |
| Electric Trap | Discuss Later | Guide complete |
| Follow Player | BPA_FollowCharacter | Mapped (via dialogue/goals) |
| Idle | BPA_Idle | Mapped |
| Move to Deploy | BPA_MoveToDestination | Mapped |

### 57.3) Damage System Alignment

| Father Pattern | Narrative Pro Pattern | Status |
|----------------|----------------------|--------|
| NarrativeDamageExecCalc | NarrativeDamageExecCalc | ALIGNED |
| SetByCaller for magnitude | SetByCaller tags | ALIGNED |
| Custom GE per ability | Custom GE | Correct (need armor calc) |

### 57.4) Projectile System Alignment

| Father Component | Narrative Pro Component | Status |
|------------------|------------------------|--------|
| BP_ElectricWeb | NarrativeProjectile | ALIGNED |
| BP_TurretProjectile | NarrativeProjectile | ALIGNED |
| BP_LaserProjectile | NarrativeProjectile | ALIGNED |
| AbilityTask_SpawnProjectile | AbilityTask_SpawnProjectile | ALIGNED |
| SetProjectileTargetData | SetProjectileTargetData | ALIGNED |
| OnTargetData delegate | OnTargetData delegate | ALIGNED |

### 57.5) BT Task Alignment

| Father | Narrative Pro | Status |
|--------|---------------|--------|
| Custom BTTask per ability | BTTask_ActivateAbilityByClass | UPDATE REQUIRED |
| Tag-based activation | BTTask_ActivateAbilityByInput | Available |

### 57.6) BT Service Alignment

| Father Needs | Narrative Pro Service | Status |
|--------------|----------------------|--------|
| Attack focus | BTS_FocusAttackTarget | Available |
| Clear focus | BTS_ClearAIFocus | Available |
| Set focus | BTS_SetAIFocus | Available |

### 57.7) Proposed AC_FatherCompanion_Default

| Index | Ability | Notes |
|-------|---------|-------|
| 0 | GA_FatherCrawler | Form |
| 1 | GA_FatherArmor | Form |
| 2 | GA_FatherExoskeleton | Form |
| 3 | GA_FatherSymbiote | Form |
| 4 | GA_FatherEngineer | Form |
| 5 | GA_FatherAttack | Combat |
| 6 | GA_FatherLaserShot | Combat |
| 7 | GA_FatherMark | Passive |
| 8 | GA_FatherSacrifice | Passive |
| 9 | GA_Death | Death handling (ADD) |

### 57.8) Proposed ActConfig_FatherCompanion

| Index | Activity | Purpose |
|-------|----------|---------|
| 0 | BPA_Attack_Melee | Crawler melee |
| 1 | BPA_Attack_Ranged_Strafe | Laser shot (mobile) |
| 2 | BPA_Attack_Ranged | Turret shoot (stationary) |
| 3 | BPA_FollowCharacter | Follow player |
| 4 | BPA_Idle | Default idle |
| 5 | BPA_MoveToDestination | Engineer deployment |

### 57.9) Goal Generator Configuration

| Generator | Purpose |
|-----------|---------|
| GoalGenerator_Attack | Auto-target hostiles (same as AC_Default) |

### 57.10) Alignment Summary Score

| Category | Status |
|----------|--------|
| Ability Configuration | 9/10 aligned (add GA_Death) |
| Activity Configuration | 6/7 mapped |
| Damage System | FULLY ALIGNED |
| Projectile System | FULLY ALIGNED |
| BT Tasks | UPDATE REQUIRED (use built-in) |
| BT Services | Available (implement) |

---

## SECTION 58: GE_EQUIPMENTMODIFIER PATTERN

### 58.1) Overview

GE_EquipmentModifier is Narrative Pro's built-in GameplayEffect for applying stat bonuses from EquippableItems. It uses SetByCaller magnitudes to dynamically set stat values.

### 58.2) GE_EquipmentModifier Configuration

| Property | Value |
|----------|-------|
| Duration Policy | Infinite |
| Period | 0.0 |
| Components | 0 Array elements |
| Modifiers | 3 Array elements |
| Stacking Type | None |

### 58.3) Modifier Configuration

| Index | Attribute | Modifier Op | Magnitude Type | SetByCaller Tag |
|-------|-----------|-------------|----------------|-----------------|
| [0] | NarrativeAttributeSetBase.Armor | Add (Base) | Set by Caller | SetByCaller.Armor |
| [1] | NarrativeAttributeSetBase.AttackRating | Add (Base) | Set by Caller | SetByCaller.AttackRating |
| [2] | NarrativeAttributeSetBase.StealthRating | Add (Base) | Set by Caller | SetByCaller.StealthRating |

### 58.4) EquippableItem Integration

EquippableItem passes stat values to GE_EquipmentModifier via properties:

| EquippableItem Property | Maps To | SetByCaller Tag |
|-------------------------|---------|-----------------|
| Armor Rating | Modifier [0] | SetByCaller.Armor |
| Attack Rating | Modifier [1] | SetByCaller.AttackRating |
| Stealth Rating | Modifier [2] | SetByCaller.StealthRating |

### 58.5) Father Form Stat Mapping

| Form EquippableItem | Armor | AttackRating | StealthRating |
|---------------------|-------|--------------|---------------|
| BP_FatherArmorForm | 50.0 | 0.0 | 0.0 |
| BP_FatherExoskeletonForm | 0.0 | 10.0 | 0.0 |
| BP_FatherSymbioteForm | 0.0 | 100.0 | 0.0 |
| BP_FatherEngineerForm | 0.0 | 0.0 | 0.0 |

### 58.6) What GE_EquipmentModifier Does NOT Support

| Stat | Handling Method |
|------|-----------------|
| Movement Speed | CharacterMovement component direct modification |
| Jump Height | CharacterMovement component direct modification |
| Stamina/StaminaRegen | Custom GE required (e.g., GE_SymbioteBoost) |

### 58.7) Child GE vs Base GE Pattern (RESOLVED)

| Form | Equipment Effect GE | Components | Form Tags From |
|------|---------------------|------------|----------------|
| Armor | GE_EquipmentModifier_FatherArmor | 0 elements | GA Activation Owned Tags |
| Exoskeleton | GE_EquipmentModifier_FatherExoskeleton | 0 elements | GA Activation Owned Tags |
| Symbiote | GE_EquipmentModifier_FatherSymbiote | 0 elements | GA Activation Owned Tags |
| Engineer | GE_EquipmentModifier_FatherEngineer | 0 elements | GA Activation Owned Tags |

Child GEs created for future expandability but do NOT grant tags via Components to prevent duplicate tag granting with GA Activation Owned Tags.

### 58.8) Child GE Architecture Decision

| Design Question | Decision | Rationale |
|-----------------|----------|-----------|
| Use base or child GEs? | Child GEs | Future expandability |
| Grant form tags via Components? | NO | Prevents duplicate with Activation Owned Tags |
| Where do form tags come from? | GA Activation Owned Tags | Applied on ability activate, removed on ability end |
| Symbiote special handling | StaminaRegenRate Override 10000 | Infinite stamina not supported by base GE |

### 58.9) HandleEquip/HandleUnequip Lifecycle

Source: EquippableItem.cpp

| Method | Line | Called When | What It Does |
|--------|------|-------------|--------------|
| EquipItem() | 98-126 | Item equipped via UI/inventory | Validates slot, calls HandleEquip |
| HandleEquip_Implementation() | 73-81 | After EquipItem | GrantAbilities + ApplyEquipmentAttributes |
| UnequipItem() | 128-156 | Item unequipped | Validates, calls HandleUnequip |
| HandleUnequip_Implementation() | 83-91 | After UnequipItem | RemoveAbilities + RemoveEquipmentAttributes |

### 58.9.1) HandleEquip Flow

| Step | Code Location | Action |
|------|---------------|--------|
| 1 | Line 75 | CharacterOwner = GetOwningNarrativeCharacter() |
| 2 | Line 77 | GrantAbilities(EquipmentAbilities) |
| 3 | Line 79 | ApplyEquipmentAttributes() |

### 58.9.2) ApplyEquipmentAttributes Flow

| Step | Code Location | Action |
|------|---------------|--------|
| 1 | Line 241 | Create GE spec from EquipmentEffect class |
| 2 | Line 245-255 | ModifyEquipmentEffectSpec() sets SetByCaller magnitudes |
| 3 | Line 257 | Apply GE to owner ASC |
| 4 | Line 259 | Store FActiveGameplayEffectHandle |

### 58.9.3) HandleUnequip Flow

| Step | Code Location | Action |
|------|---------------|--------|
| 1 | Line 85 | RemoveAbilities(AbilityHandles) |
| 2 | Line 87 | RemoveEquipmentAttributes() via stored handle |

### 58.10) Dual Entry System

EquippableItem stat values appear in TWO places that must match:

| Location | Purpose | Example |
|----------|---------|---------|
| Equipment Effect Values (Map) | Passed to GE via SetByCaller | SetByCaller.Armor: 50.0 |
| Individual Rating Properties | UI display and tooltips | Armor Rating: 50.0 |

### 58.10.1) Equipment Effect Values Map

| Key (Gameplay Tag) | Value (Float) | Effect |
|--------------------|---------------|--------|
| SetByCaller.Armor | 50.0 | +50 Armor via GE modifier |
| SetByCaller.AttackRating | 100.0 | +100 Attack via GE modifier |
| SetByCaller.StealthRating | 40.0 | +40 Stealth via GE modifier |

### 58.10.2) Rating Properties

| Property | Type | Purpose |
|----------|------|---------|
| Armor Rating | Float | UI Stats display, tooltip generation |
| Attack Rating | Float | UI Stats display, tooltip generation |
| Stealth Rating | Float | UI Stats display, tooltip generation |

### 58.11) Activities to Grant

EquippableItems can grant AI Activities when equipped via the Activities to Grant array.

| Property | Location | Purpose |
|----------|----------|---------|
| Activities to Grant | Item - Equippable category | Array of activities granted on equip |

### 58.11.1) Father Form Activity Mapping

| Form EquippableItem | Activity to Grant | Purpose |
|---------------------|-------------------|---------|
| BP_FatherCrawlerForm | BPA_FollowCharacter | Follow behavior when in Crawler |
| BP_FatherArmorForm | None | Attached to player |
| BP_FatherExoskeletonForm | None | Attached to player |
| BP_FatherSymbioteForm | None | Merged with player |
| BP_FatherEngineerForm | None | Stationary turret mode |

---

## SECTION 59: NARRATIVE PRO 2.2 DUAL WIELD/OFFHAND SYSTEM

### 59.1) Overview

Narrative Pro 2.2 introduces offhand ability variants enabling dual wield combat across melee, firearms, and magic.

### 59.2) Dual Wield Pattern Summary

| Category | Main Hand | Offhand | Dual Wield Support |
|----------|-----------|---------|-------------------|
| Melee Combo | GA_Attack_Combo_Melee | GA_Attack_Combo_Melee_Offhand | YES |
| Firearms (Pistol) | GA_Firearm_Pistol | GA_Firearm_Pistol_R | YES |
| Magic Beam | GA_Attack_Magic_Beam | GA_Attack_Magic_Beam_Offhand | YES |
| Magic Projectile | GA_Attack_Magic_Proj | GA_Attack_Magic_Proj_Offhand | YES |

### 59.3) Asset Locations

| Category | Path |
|----------|------|
| Melee | Core/Abilities/GameplayAbilities/Attacks/Melee/ |
| Firearms | Core/Abilities/GameplayAbilities/Attacks/Firearms/ |
| Magic | Core/Abilities/GameplayAbilities/Attacks/Magic/ |

### 59.4) Father Dual Wield Relevance

| Consideration | Assessment |
|---------------|------------|
| Current father attacks | Single attack (GA_FatherAttack) |
| Dual wield potential | Could attack with two limbs simultaneously |
| Priority | LOW - Father uses natural attacks, not weapons |

### 59.5) New 2.2 Attack Abilities Summary

| Folder | New Assets |
|--------|------------|
| Melee/ | GA_Attack_Combo_Melee_Offhand |
| Firearms/ | GA_Firearm_Pistol_R |
| Magic/ | GA_Attack_Magic_Beam_Offhand, GA_Attack_Magic_Proj_Offhand |
| Misc/ | GA_Attack_Magic_Fall, GA_Attack_Magic_NPC_Control, GA_Attack_Magic_Ragdoller |
| BowArrows/ | BP_Weapon_Projectile_Bow_Arrow_Explosive, BP_Weapon_Projectile_Dark_Arrow, BP_Weapon_Projectile_Iron_Arrow |

### 59.6) Arrow Variant Pattern (Future Father Projectiles)

| Arrow Type | Father Equivalent Idea |
|------------|------------------------|
| Standard Arrow | BP_FatherWeb_Projectile (basic) |
| Explosive Arrow | BP_FatherWeb_Explosive (AoE damage) |
| Dark Arrow | BP_FatherWeb_Poison (DoT effect) |
| Iron Arrow | BP_FatherWeb_Heavy (armor piercing) |

---

## SECTION 60: DEMO WEAPON VISUAL REFERENCE

### 60.1) Overview

Visual analysis of Narrative Pro demo weapons confirms EquippableItem configuration patterns for father form implementation.

### 60.2) Demo Dagger (MeleeWeaponItem)

| Property | Value |
|----------|-------|
| Parent Class | MeleeWeaponItem |
| Weapon Hand | Dual Wieldable |
| Attack Damage | 30.0 |
| Heavy Attack Damage Multiplier | 1.6 |
| Equipment Effect | GE_EquipmentModifier |
| Weapon Abilities | GA_Attack_Melee_Sword_1H, GA_Melee_Block |
| Fragments | Poisonable Fragment |

### 60.3) Demo Bow (RangedWeaponItem)

| Property | Value |
|----------|-------|
| Parent Class | RangedWeaponItem |
| Equipment Effect | **GE_EquipmentModifier_Bow** (child GE) |
| Ammo | BI_ArrowBase |
| Visual | BP_DemoBowVisual |
| Crosshair | WBP_Crosshair_Default |

### 60.3.1) Bow Child GE Pattern

Bow demonstrates child GE pattern with **GE_EquipmentModifier_Bow**:

| Benefit | Description |
|---------|-------------|
| Organization | Separate GE asset per item type |
| Expandability | Can add bow-specific modifiers later |
| Debugging | Easier to trace effects to source |

### 60.4) Demo Rifle (RangedWeaponItem)

| Property | Value |
|----------|-------|
| Parent Class | Ranged Weapon Item |
| Automatic Fire | Checked |
| Rate Of Fire | 0.12 |
| Trace Distance | 15000.0 |
| Attack Damage | 22.0 |
| Required Ammo | Ammo_Rifle |
| Clip Size | 30 |
| Equipment Mod GE | GE_EquipmentMod... |
| Abilities | GA_Firearm_Rifle, GA_Weapon_Aim, GA_Weapon_Bash |

### 60.5) Demo Greatsword (MeleeWeaponItem)

| Property | Value |
|----------|-------|
| Parent Class | MeleeWeaponItem |
| Weapon Hand | Two Handed |
| Attack Damage | 30.0 |
| Equipment Effect | GE_EquipmentModifier (base) |
| Weapon Abilities | GA_Attack_Melee_Sword_2H, GA_Melee_Block |
| Fragments | Poisonable Fragment |

### 60.5.1) Holster/Wield Attachment Pattern

| Config Type | Slot Tag | Socket Name |
|-------------|----------|-------------|
| Holster | Narrative.Equipment.Slot.Weapon.BackA | Socket_BackA |
| Holster | Narrative.Equipment.Slot.Weapon.BackB | Socket_BackB |
| Wield | Narrative.Equipment.WieldSlot.Mainhand | weapon_r |

### 60.6) Equipment Effect Values Map (All Weapons)

| Key (Gameplay Tag) | Type | Purpose |
|--------------------|------|---------|
| SetByCaller.Armor | Float | Armor stat via GE modifier |
| SetByCaller.AttackRating | Float | Attack stat via GE modifier |
| SetByCaller.StealthRating | Float | Stealth stat via GE modifier |

### 60.7) Father Form Mapping from Demo Weapons

| Father Form | Equipment Effect | Based On |
|-------------|------------------|----------|
| BP_FatherArmorForm | GE_EquipmentModifier_FatherArmor | Bow child GE pattern |
| BP_FatherExoskeletonForm | GE_EquipmentModifier_FatherExoskeleton | Bow child GE pattern |
| BP_FatherSymbioteForm | GE_EquipmentModifier_FatherSymbiote | Bow child GE pattern |
| BP_FatherEngineerForm | GE_EquipmentModifier_FatherEngineer | Bow child GE pattern |

---

## SECTION 61: NPC DEFINITION VISUAL REFERENCE

### 61.1) NPC_Seth Configuration

| Property | Value | Father Equivalent |
|----------|-------|-------------------|
| NPCID | Seth | FatherCompanion |
| NPCName | Seth Taiga | Father Companion |
| NPCClass Path | BP_NarrativeNPC | BP_FatherCompanion |
| Dialogue | DBP_Seth | DBP_FatherCompanion |
| Tagged Dialogue Set | Seth_TaggedDialogue | FatherCompanion_TaggedDialogue |

### 61.2) NPC_Seth AI Configuration

| Property | Value | Father Equivalent |
|----------|-------|-------------------|
| Activity Configuration | AC_RunAndGun | ActConfig_FatherCompanion |
| Ability Configuration | AC_NPC_Default | AC_FatherCompanion_Default |

### 61.3) NPC_Seth Character/Faction

| Property | Value | Father Equivalent |
|----------|-------|-------------------|
| Default Owned Tags | Narrative.State.Invulnerable | Father.State.Alive |
| Default Factions | Narrative.Factions.Heroes | Narrative.Factions.Heroes |
| Attack Priority | 1.0 | 1.0 |

### 61.4) ItemLoadout_Seth (Data Table Pattern)

| Property | Value |
|----------|-------|
| Row Struct | **LootTableRow** |
| Items to Grant | 0 Array element |
| Item Collections to Grant | 2 Array elements |
| Index [0] | IC_RifleWithAmmo |
| Index [1] | IC_ExampleArmorSet |
| Chance | 1.0 |

### 61.4.1) LootTableRow Structure

| Field | Type | Purpose |
|-------|------|---------|
| Items to Grant | Array (NarrativeItem) | Individual items |
| Item Collections to Grant | Array (ItemCollection) | Item groups |
| Sub Tables to Roll | Array (DataTable) | Nested loot tables |
| Chance | Float | Probability (1.0 = 100%) |

### 61.5) Father Item Loadout Options

| Option | Method | Complexity |
|--------|--------|------------|
| A | Items to Grant array in NPCDef | Simple |
| B | IC_FatherForms Item Collection | Cleaner |
| C | DT_FatherLoadout Data Table | Most flexible |

### 61.5.1) Recommended: Option B (Item Collection)

| IC_FatherForms Contents |
|------------------------|
| BP_FatherArmorForm |
| BP_FatherExoskeletonForm |
| BP_FatherSymbioteForm |
| BP_FatherEngineerForm |

Note: BP_FatherCrawlerForm excluded - Crawler is default form without EquippableItem.

---

## SECTION 62: ACTIVITY CONFIGURATION VISUAL REFERENCE

### 62.1) AC_RunAndGun Configuration

| Property | Value |
|----------|-------|
| Rescore Interval | 0.5 |
| Default Activities | 11 Array elements |
| Goal Generators | 1 Array element |

### 62.2) AC_RunAndGun Activities

| Index | Activity | Father Usage |
|-------|----------|--------------|
| [0] | BPA_Attack_Melee | YES |
| [1] | BPA_Attack_Ranged_Strafe | YES |
| [2] | BPA_Attack_Investigate | Optional |
| [3] | BPA_Attack_Grenade | NO |
| [4] | BPA_FollowCharacter | YES (custom) |
| [5] | BPA_Patrol | NO |
| [6] | BPA_Interact | Optional |
| [7] | BPA_Idle | YES |
| [8] | BPA_MoveToDestination | YES |
| [9] | BPA_ReturnToSpawn | Optional |
| [10] | BPA_DriveToDestination | NO |

### 62.3) AC_RunAndGun Goal Generators

| Index | Generator |
|-------|-----------|
| [0] | GoalGenerator_Attack |

### 62.4) Available ActivityConfigurations

| Asset | Purpose | Father Relevance |
|-------|---------|------------------|
| AC_CrowdNPC | Crowd/civilian | NO |
| AC_Default | Base NPC | Reference |
| AC_Marksman | Sniper/ranged | NO |
| AC_Melee | Melee-only | Partial |
| AC_Melee_Ranger | Mixed combat | Reference |
| AC_Pacifist | Non-combat | NO |
| AC_Pacifist_NoSpawn | Non-combat | NO |
| AC_RunAndGun | Full combat | **Best template** |

---

## SECTION 63: ABILITY CONFIGURATION VISUAL REFERENCE

### 63.1) AC_NPC_Default Configuration

| Property | Value |
|----------|-------|
| Default Attributes | GE_DefaultNPCAttributes |
| Startup Effects | 0 Array element |
| Default Abilities | 6 Array elements |

### 63.2) AC_NPC_Default Abilities

| Index | Ability | Father Equivalent |
|-------|---------|-------------------|
| [0] | GA_Melee_Punch_Unarmed | GA_FatherAttack |
| [1] | GA_Melee_Block | NO |
| [2] | GA_Dodge | Optional |
| [3] | GA_Weapon_Reload | NO |
| [4] | GA_Weapon_Wield | NO |
| [5] | **GA_Death** | **YES - Required** |

### 63.3) Available AbilityConfigurations

| Asset | Purpose |
|-------|---------|
| AC_NPC_Default | Base NPC abilities |
| AC_Player_RPG | Player RPG |
| AC_Player_Shooter | Player shooter |
| AC_Vehicle_Default | Vehicle abilities |

### 63.4) AC_FatherCompanion_Default Template

| Property | Value |
|----------|-------|
| Default Attributes | GE_DefaultNPCAttributes |
| Startup Effects | 0 Array element |
| Default Abilities | 10 Array elements |

### 63.5) AC_FatherCompanion_Default Abilities

| Index | Ability | Purpose |
|-------|---------|---------|
| [0] | GA_FatherCrawler | Form - follow |
| [1] | GA_FatherArmor | Form - defensive |
| [2] | GA_FatherExoskeleton | Form - mobility |
| [3] | GA_FatherSymbiote | Form - offensive |
| [4] | GA_FatherEngineer | Form - turret |
| [5] | GA_FatherAttack | Combat - melee |
| [6] | GA_FatherLaserShot | Combat - ranged |
| [7] | GA_FatherMark | Passive - detection |
| [8] | GA_FatherSacrifice | Passive - emergency |
| [9] | GA_Death | Death handling |

---

## SECTION 64: BPA_ATTACK_MELEE VISUAL REFERENCE

### 64.1) Class Defaults - Config Attack

| Property | Value |
|----------|-------|
| Weapon Types | 1 Array element: MeleeWeaponItem |
| Attack Tagged Dialogue | Narrative.TaggedDialogue.BeginAttacking |

### 64.2) Class Defaults - Activity Properties

| Property | Value |
|----------|-------|
| Activity Name | Melee Attack |
| Owned Tags | Narrative.State.NPC.Activity.Attacking, Narrative.State.NPC.IsBusy |
| Block Tags | Narrative.State.NPC.DisableAggro |
| Require Tags | Empty |

### 64.3) Class Defaults - NPC Activity

| Property | Value |
|----------|-------|
| Behaviour Tree | BT_Attack_Melee |
| Supported Goal Type | Goal_Attack |
| Is Interruptable | Unchecked |
| Save Activity | Unchecked |

### 64.4) ScoreGoalItem Function

| Step | Node | Purpose |
|------|------|---------|
| 1 | Cast To Goal_Attack | Verify goal type |
| 2 | Has Been Alert for Time (0.0) | Check if target is alerted |
| 3 | Has Been Hidden for Time (2.0) | Target hidden > 2 seconds? |
| 4 | Has Been Unreachable for Time (2.0) | Target unreachable > 2 seconds? |
| 5 | Branch (Hidden) | If hidden -> Return 0.0 |
| 6 | Branch (Unreachable) | If unreachable -> Return 0.0 |
| 7 | Return 0.2 | Fallback if cannot reach |
| 8 | Parent: ScoreGoalItem | Get base score |
| 9 | MAX(0.1, 0.0) + Parent Score | Favor melee slightly over ranged |

### 64.5) Scoring Return Values

| Condition | Return Value | Meaning |
|-----------|--------------|---------|
| Target hidden > 2s | 0.0 | Do not use melee |
| Target unreachable > 2s | 0.0 | Do not use melee |
| Cannot reach but recent | 0.2 | Low priority |
| Can reach target | Parent + 0.1 | Favor melee slightly |

### 64.6) Father Activity Mapping

| Father Property | Value |
|-----------------|-------|
| Weapon Types | None (father uses natural attacks) |
| Behaviour Tree | BT_Attack_Melee or custom BT_FatherMelee |
| Supported Goal Type | Goal_Attack |
| Owned Tags | Narrative.State.NPC.Activity.Attacking |
| Block Tags | Narrative.State.NPC.DisableAggro |

---

## SECTION 65: GOALGENERATOR_ATTACK VISUAL REFERENCE

### 65.1) Class Defaults - Attack Goal Generator Settings

| Property | Value |
|----------|-------|
| Query Template | EQS_Actor_SensedAttackTarget |
| Run Mode | All Matching |
| Query Run Interval | 0.5 |
| Attack Goal Class | Goal_Attack |
| Attack Goal Base Score | 3.0 |

### 65.2) Attack Affiliation Map

| Sense Type | Detect Enemies | Detect Neutrals | Detect Friendlies |
|------------|----------------|-----------------|-------------------|
| AISense_Damage | Checked | Checked | Unchecked |
| AISense_Sight | Checked | Unchecked | Unchecked |
| AISense_Hearing | Checked | Checked | Checked |

### 65.3) Affiliation Map Behavior

| Sense | Behavior |
|-------|----------|
| Damage | Attack enemies AND neutrals who damage us |
| Sight | Only attack enemies we see |
| Hearing | Investigate all sounds (enemies, neutrals, friendlies) |

### 65.4) Functions Overview

| Function | Purpose |
|----------|---------|
| Try Add Attack Goal From Actor | Creates attack goal for detected actor |
| DoesAttitudeMatchFilter | Checks faction relationship against filter |
| OnStartSenseActor | AIPerception detected new actor |
| OnStopSenseActor | AIPerception lost actor |
| OnPerceptionUpdated_Event | Routes to Start/Stop handlers |
| OnFactionsUpdated | Faction relationship changed |
| RefreshPerceivedActors | Re-evaluate all perceived actors |

### 65.5) Event InitializeGoalGenerator Flow

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event InitializeGoalGenerator | Entry point |
| 2 | Get Game State | Get NarrativeGameState |
| 3 | Cast To NarrativeGameState | Access faction system |
| 4 | Create Event (OnFactionsUpdated) | Faction change handler |
| 5 | Bind Event to On Faction Attitude Changed | Listen for faction changes |
| 6 | Cast To BP_NarrativeNPCController | Get NPC controller |
| 7 | Create Event (OnPerceptionUpdated_Event) | Perception handler |
| 8 | Bind Event to On Perception Updated | Listen for perception changes |
| 9 | Refresh Perceived Actors | Initial evaluation |

### 65.6) OnPerceptionUpdated_Event Flow

| Step | Node | Purpose |
|------|------|---------|
| 1 | OnPerceptionUpdated_Event | Entry - receives Actor and Stimulus |
| 2 | Break AIStimulus | Extract stimulus properties |
| 3 | Branch (Successfully Sensed) | Actor sensed or lost? |
| 4a | On Start Sense Actor (True) | Actor detected - create attack goal |
| 4b | On Stop Sense Actor (False) | Actor lost - remove attack goal |

### 65.7) OnStartSenseActor Flow

| Step | Node | Purpose |
|------|------|---------|
| 1 | OnStartSenseActor | Entry - Actor detected |
| 2 | Get Sense Class for Stimulus | Determine which sense |
| 3 | SET Sense Class | Store sense type |
| 4 | Attack Affiliation Map | Get faction filter |
| 5 | FIND | Look up sense in map |
| 6 | Get Goal by Key | Check if goal exists |
| 7 | Branch (Out Succeeded) | Goal exists? |
| 8 | Try Add Attack Goal from Actor | Create new attack goal |

### 65.8) DoesAttitudeMatchFilter Logic

| Check | Attitude | Filter Property | Result |
|-------|----------|-----------------|--------|
| 1 | == Friendly | Detect Friendlies | AND -> matches |
| 2 | == Neutral | Detect Neutrals | AND -> matches |
| 3 | == Hostile | Detect Enemies | AND -> matches |
| Special | Friendly + NotifyTeammates tag | - | matches (teammate alert) |

### 65.9) Try Add Attack Goal From Actor Flow

| Phase | Steps | Purpose |
|-------|-------|---------|
| Validation | 1-6 | Check sense config, cast to NarrativeCharacter |
| Faction Check | 7-14 | Get Attitude, verify target is valid enemy |
| Duplicate Check | 15-17 | Skip if goal already exists for actor |
| Goal Creation | 18-21 | Construct Goal_Attack with score |
| Registration | 22-25 | Add Goal Item to activity component |
| Team Alert | 26-27 | Report Noise Event, return |

### 65.10) Construct Goal Attack Parameters

| Parameter | Value/Source |
|-----------|--------------|
| Class | Attack Goal Class variable |
| Outer | self |
| Target to Attack | Attack Target |
| Last Sight Stimulus | Stimulus |
| Owned Tags | Empty |
| Remove on Succeeded | Unchecked |
| Default Score | Attack Goal Base Score + Score |
| Save Goal | Unchecked |
| Intended TODStart Time | -1.0 |
| Goal Lifetime | -1.0 |

### 65.11) OnFactionsUpdated Flow

| Step | Node | Purpose |
|------|------|---------|
| 1 | OnFactionsUpdated | Entry - Faction relationship changed |
| 2 | Refresh Perceived Actors | Re-evaluate all actors |

### 65.12) RefreshPerceivedActors Flow

| Step | Node | Purpose |
|------|------|---------|
| 1 | RefreshPerceivedActors | Entry point |
| 2 | Get AIPerception Component | Get perception from Owner Controller |
| 3 | Get Currently Perceived Actors | Get all sensed actors (AISense_Sight) |
| 4 | For Each Loop | Iterate through perceived actors |
| 5 | Make AIStimulus (fake) | Create stimulus for re-evaluation |
| 6 | Try Add Attack Goal from Actor | Re-evaluate as potential target |

### 65.13) Father Integration

| Father Config | Value |
|---------------|-------|
| GoalGenerator | GoalGenerator_Attack (in ActConfig_FatherCompanion) |
| Faction | Narrative.Factions.Heroes |
| Result | Auto-attacks enemies detected via AIPerception |

---

## SECTION 66: TEAM COORDINATION SYSTEM

### 66.1) Report Noise Event Pattern

Used in Try Add Attack Goal From Actor to alert nearby friendlies:

| Parameter | Value | Purpose |
|-----------|-------|---------|
| Noise Location | Attack Target location | Where enemy is |
| Loudness | 9999.0 | Maximum detection range |
| Instigator | Attack target | Who to attack |
| Max Range | Notify Teammates To Fight Range | From Combat Settings |
| Tag | NotifyTeammates | Special tag for teammate alert |

### 66.2) NotifyTeammates Special Case

In DoesAttitudeMatchFilter, friendlies with AISense_Hearing detect the noise event. The NotifyTeammates tag triggers special handling:

| Condition | Check | Result |
|-----------|-------|--------|
| Attitude == Friendly | Yes | Normally filtered out |
| Stimulus Tag == NotifyTeammates | Yes | Bypass filter, create attack goal |
| AND result | Both true | Friendly joins fight |

### 66.3) Alert Time Functions

| Function | Parameters | Purpose |
|----------|------------|---------|
| Has Been Alert for Time | Target, Required Time | Check if NPC alerted to target |
| Has Been Hidden for Time | Target, Required Time | Target hidden for duration? |
| Has Been Unreachable for Time | Target, Required Time | Target unreachable for duration? |

### 66.4) Combat Settings Integration

| Property | Source | Purpose |
|----------|--------|---------|
| Notify Teammates To Fight Range | Get Combat Settings | Alert propagation radius |

### 66.5) Future NPC Applications

| NPC Type | Feature Usage |
|----------|---------------|
| Father Companion | Report enemy locations to player |
| Bandit Patrol | Call reinforcements when attacked |
| Guard Squad | Coordinate defense when intruder detected |
| Boss Encounter | Summon adds when alerted |
| Faction War | One member alerts entire patrol |

### 66.6) Implementation Pattern for Custom NPCs

| Step | Action |
|------|--------|
| 1 | Ensure NPC has AISense_Hearing configured |
| 2 | Set Detect Friendlies = true for AISense_Hearing in Attack Affiliation Map |
| 3 | GoalGenerator_Attack handles NotifyTeammates automatically |
| 4 | Nearby friendlies will join combat when one detects enemy |

### 66.7) Father Team Coordination

Father can alert player about enemies using Report Noise Event:

| Scenario | Implementation |
|----------|----------------|
| Father detects enemy | GoalGenerator_Attack creates Goal_Attack |
| Father attacks | Report Noise Event with NotifyTeammates |
| Player receives alert | Player AIPerception hears NotifyTeammates noise |
| Result | Player knows enemy location |

---

## APPENDIX A: VERIFIED WORKING PATTERNS

| Pattern | Verified In |
|---------|-------------|
| GA_Melee_Unarmed parent class | GA_FatherAttack v3.0 |
| ~~Narrative.State.Invulnerable i-frames~~ | ~~GA_FatherExoskeletonDash v3.0~~ **(REMOVED per INV-1)** |
| StealthRating + State.Invisible | GA_StealthField v3.0 |
| Component-based GE in UE 5.6 | All v2.0+ guides |
| CharacterMovement speed | Sprint v2.2, Dash v3.8, Stealth v3.0 |
| NarrativeDamageExecCalc | GA_FatherAttack, GA_Dash, GA_ElectricTrap v2.4 |
| InputTag property | All ability guides |
| GE_EquipmentModifier SetByCaller | Section 58 (EquippableItem stat application) |
| Child GE without Components | Section 58.8 (Form tags from Activation Owned Tags) |
| HandleEquip/HandleUnequip lifecycle | Section 58.9 (EquippableItem.cpp flow) |
| Dual Entry System | Section 58.10 (Map + Properties must match) |
| Activities to Grant | Section 58.11 (AI behaviors on equip) |
| Dual Wield Offhand Pattern | Section 59 (2.2 offhand abilities) |
| Arrow Variant Pattern | Section 52.3.6, 59.6 (BowArrows folder) |
| NPCSpawner for spawn management | Father spawn guide v2.0 |
| ArsenalStatics::GetAttitude | GA_FatherLaserShot v3.1, GA_FatherExoskeletonSprint v2.2 |
| Delegate-based attribute monitoring | GA_ProtectiveDome v1.6, GA_FatherSacrifice v2.0 |
| Activation Owned Tags replication | v4.0 architecture (ReplicateActivationOwnedTags enabled) |
| Cancel Abilities With Tag mutual exclusivity | All form abilities v4.0 |
| EndAbility speed restoration | GA_FatherArmor, GA_FatherExoskeleton v4.0 |
| Direct form-to-form transitions | v4.0 architecture |
| 15s shared form cooldown | All form abilities v4.0 |
| AbilityConfiguration for ability granting | Section 20 (AC_NPC_Default pattern) |
| ActivityConfiguration for AI behaviors | Section 21 (AC_RunAndGun pattern) |
| BPA_ activity scoring pattern | Section 22 (ScoreGoalItem) |
| Goal system for AI targeting | Section 23 (GoalGenerator_Attack, Goal_Attack) |
| EQS for target selection | Section 24 (EQS_Actor_SensedAttackTarget) |
| Combat ability hierarchy | Section 25 (GA_CombatAbilityBase -> GA_Attack_Combo_Melee) |
| GE_WeaponDamage_Heavy inheritance | Section 26 (parent: GE_WeaponDamage) |
| Hold-to-Maintain pattern | Section 27.1 (GA_Sprint) |
| Instant with Cost pattern | Section 27.2 (GA_Dodge) |
| Event-Triggered pattern | Section 27.3 (GA_Death) |
| Child GE per EquippableItem | Section 60.3.1 (GE_EquipmentModifier_Bow) |
| LootTableRow Data Table | Section 61.4 (ItemLoadout_Seth) |
| Item Collection loadout | Section 61.5 (IC_FatherForms recommendation) |
| AC_RunAndGun template | Section 62 (ActConfig_FatherCompanion base) |
| AC_NPC_Default template | Section 63 (AC_FatherCompanion_Default base) |
| GoalGenerator_Attack auto-targeting | Section 62.3 (Faction-based hostile detection) |
| Narrative.Factions.Heroes | Section 61.3 (Father faction assignment) |
| BPA_Attack_Melee scoring | Section 64.4 (ScoreGoalItem function) |
| GoalGenerator_Attack initialization | Section 65.5 (Event bindings) |
| Attack Affiliation Map | Section 65.2 (Per-sense faction filtering) |
| DoesAttitudeMatchFilter | Section 65.8 (Faction check logic) |
| Try Add Attack Goal From Actor | Section 65.9 (Complete goal creation flow) |
| Report Noise Event | Section 66.1 (NotifyTeammates pattern) |
| Team Coordination | Section 66 (Alert propagation system) |
| Variable Replication Conditions | Section 7.7 (BP_FatherCompanion variables) |
| GameplayCueNotify_BurstLatent | Section 31.2.1 (GC_TakeDamage) |
| Cue Inheritance Pattern | Section 31.2.2 (GC_TakeDamage_Stumble extends GC_TakeDamage) |
| Reliable Multicast in Cues | Section 31.2.1 (Play Anim Montage custom event) |
| Perlin Noise Camera Shake | Section 31.3.2 (BP_HandheldCameraShake) |
| Wave Oscillator Camera Shake | Section 31.3.2 (BP_PistolCameraShake, BP_RifleCameraShake) |
| GCN Effects Configuration | Section 31.4 (Burst Particles, Sounds, Decals) |
| BB_FollowCharacter for NPC following | Section 32.7 (Goal Follow Character) |
| Dialogue-driven goal assignment | Section 33.4 (AI: Add Goal To NPC) |
| Dual reference architecture | Section 34 (Goal System + OwnerPlayer) |
| NarrativeEvent for ownership | Section 35 (NE_SetFatherOwner, NE_ClearFatherOwner) |
| SourceObject for ability tracking | Section 36 (Father grants to player ASC) |
| Tag + CanActivateAbility hybrid | Section 37 (Narrative Pro pattern) |
| Father.State.Recruited gating | Section 37 (Form ability requirement) |
| Form-based invulnerability | Section 38 (Narrative.State.Invulnerable) |
| CharacterDefinition persistence | Section 34.12-34.14 (PrepareForSave/Initialize) |
| Per-form handle tracking | Section 14.8 (Handle variables per form) |
| CancelAbilityHandle cleanup | Section 14.10 (Cross-actor ability removal) |
| EndPlay Safety Net | Section 39 (Emergency cleanup on father destruction) |
| HasAuthority cross-actor gates | Section 40 (Authority checks for GiveAbility/ApplyGE) |
| Symbiote fallback pattern | Section 41 (Armor -> Crawler -> Manual) |
| ServerOnly for form abilities | Section 7.4 (Cross-actor grants require server authority) |
| Manual handle-based cleanup | Section 36.14 (Alternative to RemoveActiveEffectsWithSourceObject) |
| BPT_TimeOfDayRange trigger | Section 45.7 (Time-based schedule activation) |
| BPT_Always trigger | Section 42.10 (Immediate permanent activation) |
| UTriggerSchedule hierarchy | Section 42.3 (TriggerSchedule -> TriggerSet -> Trigger) |
| UInteractionSlotBehavior | Section 43.4 (Slot behavior configuration) |
| BP_SlotBehavior_Sit | Section 43.9 (Sitting/workstation interactions) |
| GA_Interact_Sit | Section 44.2-44.6 (Interaction ability with props) |
| WaitTimeRange latent node | Section 42.9 (Time range detection) |
| Goal catchup system | Section 42.18 (Late game load handling) |
| Interaction Props attachment | Section 43.10 (Prop spawn and bone attachment) |
| NPCActivitySchedule | Section 42.15 (NPC-specific scheduled behaviors) |
| BTTask_ActivateAbilityByClass | Section 54 (Built-in task for AI ability activation) |
| BTS_FocusAttackTarget | Section 55 (Attack target focus during combat) |
| BTS_ClearAIFocus | Section 55.4-55.5 (Clear focus on activation, one-shot) |
| BTS_SetAIFocus | Section 55.6-55.9 (Set focus from BB key, auto-clear on deactivation) |
| BTS_AdjustFollowSpeed | Section 55.10-55.12 (Sprint if > 575 units, walk if <= 575) |
| BT_FollowCharacter Built-in Services | Section 55.15 (Native has BTS_SetAIFocus + BTS_AdjustFollowSpeed) |
| BT_FatherFollow Custom Pattern | Section 55.15 (Removes BTS_SetAIFocus, keeps BTS_AdjustFollowSpeed) |
| ActConfig_FatherCompanion | Section 55.15 (Father-specific ActivityConfiguration) |
| AC_NPC_Default pattern | Section 56.1-56.2 (6 abilities including GA_Death) |
| AC_Default ActivityConfiguration | Section 56.3-56.4 (9 activities, GoalGenerator_Attack) |
| GA_Death for NPC death | Section 56.5-56.8 (Event-triggered death handling) |
| SetByCaller bypasses armor | Section 56.10 (Modifier vs Execution distinction) |

---

## APPENDIX B: PROJECT REFERENCE DOCUMENTS

| Document | Purpose |
|----------|---------|
| Father_Companion_System_Design_Document_v1_6.md | System overview, 18 abilities across 5 forms |
| Father_Companion_Guide_Format_Reference_v2_2.md | Documentation formatting standards |
| DefaultGameplayTags_FatherCompanion_v3_4.ini | All gameplay tags defined |
| Father_Companion_System_Setup_Guide_v1_1.md | Father character setup, variables, functions |
| GE_FormStateAndCooldown_System_Implementation_Guide_v1_2.md | Form effects and cooldown system |

---

## APPENDIX C: NARRATIVE PRO ASSET NAMING CONVENTIONS

| Prefix | Type | Example |
|--------|------|---------|
| GA_ | Gameplay Ability | GA_Sprint, GA_FatherAttack |
| GE_ | Gameplay Effect | GE_WeaponDamage, GE_ArmorBoost |
| GC_ | Gameplay Cue | GC_TakeDamage, GC_WeaponFire |
| AC_ | AbilityConfiguration | AC_RunAndGun, AC_FatherCompanion_Default |
| ActConfig_ | ActivityConfiguration | ActConfig_FatherCompanion, ActConfig_Guard |
| BPA_ | Blueprint Activity | BPA_Attack_Melee, BPA_Idle |
| BB_ | Blackboard | BB_Attack, BB_FollowCharacter |
| BT_ | Behavior Tree | BT_Attack, BT_Follow |
| BTS_ | BT Service | BTS_ApplyGameplayTags, BTS_SetAIFocus |
| BTT_ | BT Task (Blueprint) | BTT_SetBehaviorTree |
| BTTask_ | BT Task (C++) | BTTask_ActivateAbilityByClass |
| EQS_ | Environment Query | EQS_Actor_SensedAttackTarget |
| EQSContext_ | EQS Context | EQSContext_PlayerPawn |
| NPCDef_ | NPC Definition | NPCDef_FatherCompanion |
| Goal_ | Goal Item | Goal_Attack, Goal_FollowCharacter |
| GoalGenerator_ | Goal Generator | GoalGenerator_Attack |
| DBP_ | Dialogue Blueprint | DBP_Father_FatherCompanion |
| CD_ | Character Definition | CD_DefaultPlayer, CD_FatherCompanion |
| NE_ | Narrative Event | NE_SetFatherOwner, NE_ClearFatherOwner |
| BP_ | Blueprint Class | BP_NarrativeNPC, BP_PistolCameraShake |
| BPT_ | Blueprint Trigger | BPT_TimeOfDayRange, BPT_Always |
| TS_ | Trigger Set | TS_BlacksmithSchedule, TS_GuardPatrol |
| BP_Interactable | Interactable Actor | BP_InteractableSeat_Anvil |
| BP_SlotBehavior_ | Slot Behavior | BP_SlotBehavior_Sit |
| BI_ | Blueprint Item | BI_NarrativeBook |
| BI_C_ | Clothing Item | BI_C_Example_Boots |
| IC_ | Item Collection | IC_ExampleArmorSet |
| DT_ | Data Table | DT_Armor, DT_Weapons |
| QBP_ | Quest Blueprint | QBP_DemoQuestBase |
| A_ | Animation Sequence | A_NarrativeDemo |
| AM_ | Animation Montage | AM_FatherAttack |
| SM_ | Static Mesh | SM_NarrativeSkySphere |
| M_ | Material | M_NarrativeSkySphere |
| T_ | Texture | T_Stars |

---

## APPENDIX D: QUICK REFERENCE TABLES

### D.1) Form State Summary

| Form | Activation Owned Tags | State Tags | Stat Effect |
|------|----------------------|------------|-------------|
| Crawler | Father.Form.Crawler | Father.State.Detached | None |
| Armor | Father.Form.Armor | Father.State.Attached | GE_ArmorBoost (+50 Armor, -15% Speed) |
| Exoskeleton | Father.Form.Exoskeleton | Father.State.Attached | GE_ExoskeletonSpeed (+50% Speed, +30% Jump, +10 Attack) |
| Symbiote | Father.Form.Symbiote | Father.State.Attached | GE_SymbioteBoost (+50% Speed, +30% Jump, +100 Attack, Infinite Stamina) |
| Engineer | Father.Form.Engineer | Father.State.Deployed | None (turret mode) |

### D.2) NPC Configuration Architecture

| Configuration | Asset | Contents |
|---------------|-------|----------|
| NPCDefinition | NPCDef_FatherCompanion | References both configurations below |
| AbilityConfiguration | AC_FatherCompanion_Default | Default Attributes, Startup Effects, Default Abilities |
| ActivityConfiguration | ActConfig_FatherCompanion | Request Interval, Default Activities, Goal Generators |

### D.3) AI System Component Summary

| Component | Purpose | Location |
|-----------|---------|----------|
| AbilityConfiguration | Grants abilities | NPCDefinition |
| ActivityConfiguration | Defines behaviors | NPCDefinition |
| GoalGenerator | Creates goals from perception | ActivityConfiguration |
| Goal | Tracks target state | Created by GoalGenerator |
| BPA_ Activity | Evaluates and executes | ActivityConfiguration |
| Behavior Tree | Runs AI logic | BPA_ Activity |
| Blackboard | Stores runtime data | Behavior Tree |
| EQS | Finds targets/positions | GoalGenerator, BT Tasks |

### D.4) Ability Parent Class Selection

| Ability Type | Parent Class | Key Features |
|--------------|--------------|--------------|
| Melee Combat | GA_Melee_Unarmed | Warp, combo, socket trace |
| Ranged Hitscan | GA_Attack_Firearm_Base | Trace, automatic fire |
| Ranged Projectile | GA_Attack_Firearm_Projectile | Projectile spawn |
| Utility/Buff | NarrativeGameplayAbility | Standard ability |

### D.5) Blackboard Selection by Behavior

| Behavior | Blackboard | Key Variables |
|----------|------------|---------------|
| Combat | BB_Attack | AttackTarget, TargetLocation |
| Combat with Range | BB_Attack_Strafe | AttackRangeMin, AttackRangeMax |
| Cover Combat | BB_Attack_Cover | Cover |
| Following | BB_FollowCharacter | FollowTarget |
| Navigation | BB_GoToLocation | TargetLocation, Delay |
| Interaction | BB_Interact | InteractableActor |
| Fleeing | BB_Flee | FleeTarget |

### D.6) Father Reference Split

| What | System | Variable/Asset |
|------|--------|----------------|
| AI Following | Goal System | Goal Follow Character, BB_FollowCharacter |
| Socket Attachment | Direct Reference | OwnerPlayer variable |
| Effect Application | Direct Reference | OwnerPlayer -> ASC |
| Form State | Gameplay Tags | Activation Owned Tags |
| Combat Target | Goal System | Goal_Attack, BB_Attack |

### D.7) NPC Schedule System Summary

| Component | Asset Type | Purpose |
|-----------|------------|---------|
| TriggerSchedule | DataAsset | Contains TriggerSets |
| TriggerSet | DataAsset | Contains Triggers |
| BPT_TimeOfDayRange | Blueprint | Time-based activation |
| BPT_Always | Blueprint | Immediate activation |
| NPCActivitySchedule | DataAsset | NPC-specific schedules |
| UScheduledBehavior_AddNPCGoal | C++ Class | Adds/removes goals by time |

### D.8) Interaction Slot System Summary

| Component | Purpose |
|-----------|---------|
| NarrativeInteractableComponent | Added to interactable actors |
| FInteractionSlotConfig | Per-slot configuration |
| UInteractionSlotBehavior | Defines slot behavior (Sit, Mount, etc.) |
| GA_Interact_Sit | Ability for sitting interactions |
| SlotTransform | Navigation target position |
| SlotInteractBehavior | References ability to activate |

### D.9) Time Format Reference

| Value | Time | Notes |
|-------|------|-------|
| 0.0 | Midnight | Start of day |
| 600.0 | 6:00 AM | Sunrise default |
| 800.0 | 8:00 AM | Work start example |
| 1200.0 | Noon | Midday |
| 1600.0 | 4:00 PM | Work end example |
| 1800.0 | 6:00 PM | Sunset default |
| 2200.0 | 10:00 PM | Night start |
| 2400.0 | Midnight | End of day |

### D.10) Goal/Activity Follow System Summary

| Component | Type | Key Properties |
|-----------|------|----------------|
| Goal_FollowCharacter | Goal | TargetToFollow, FollowDistance, ShouldMountWithTarget |
| BPA_FollowCharacter | Activity | SetupBlackboard, K2_RunActivity, K2_EndActivity |
| BB_FollowCharacter | Blackboard | SelfActor, FollowTarget, FollowDistance |
| BT_FollowCharacter | Behavior Tree | MoveTo task with FollowTarget |
| BTS_SetAIFocus | Service | Look at FollowTarget |
| BTS_AdjustFollowSpeed | Service | Sprint if > 575 units |

Follow System Data Flow:

| Step | Component | Action |
|------|-----------|--------|
| 1 | Goal_FollowCharacter | Stores TargetToFollow actor reference |
| 2 | BPA_FollowCharacter.SetupBlackboard | Writes to BB_FollowCharacter keys |
| 3 | BT_FollowCharacter | MoveTo task reads FollowTarget |
| 4 | BTS_AdjustFollowSpeed | Adjusts sprint based on distance |
| 5 | BTS_SetAIFocus | NPC looks at target |

Speed Matching:

| Distance | Behavior |
|----------|----------|
| > 575 units | Start Sprinting |
| <= 575 units | Stop Sprinting |

### D.11) Narrative Pro v2.2 New Systems Summary

| System | Key Components | Father Relevance |
|--------|----------------|------------------|
| Projectile | NarrativeProjectile, AbilityTask_SpawnProjectile | GA_FatherLaserShot, GA_TurretShoot |
| Melee Multi-Hit | CachedHitActors, bTraceMulti | GA_FatherAttack v3.3 |
| Item Fragments | AmmoFragment, PoisonableFragment | Future weapon mods |
| Cover System | NarrativeRecastNavMesh, cover queries | Turret placement |
| Traffic/Road | TrafficLightSubsystem | Not applicable |
| Mass AI | IncomingCollisionProcessor | Not applicable |

### D.12) Projectile System Quick Reference

| Component | Purpose | Key Function |
|-----------|---------|--------------|
| NarrativeProjectile | Base projectile actor | SetProjectileTargetData() |
| AbilityTask_SpawnProjectile | Spawn task | OnTargetData delegate |
| GetNarrativeCharacter() | Owner access | Returns spawning character |

Projectile Hit Flow:

| Step | Action |
|------|--------|
| 1 | Projectile hits target |
| 2 | SetProjectileTargetData() called |
| 3 | AbilityTask broadcasts OnTargetData |
| 4 | Ability applies damage GE |

### D.13) Melee Multi-Hit Quick Reference

| Property | Single-Target | Multi-Target |
|----------|---------------|--------------|
| bTraceMulti | false | true |
| TraceRadius | 0-50 | 50-100 |
| CachedHitActors | N/A | Required |
| Damage | Full | Reduced |

Debug CVars:

| CVar | Purpose |
|------|---------|
| n.gas.DrawMeleeTraces | Visualize traces |
| n.gas.DrawMeleeAimAssist | Visualize warp |

### D.14) Item Fragment Quick Reference

| Fragment | Key Properties | Use Case |
|----------|----------------|----------|
| AmmoFragment | AmmoDamageOverride, ProjectileClass | Custom ammo types |
| PoisonableFragment | AppliedPoison, SetPoison(), ConsumePoison() | Poisoned weapons |

Fragment Access:

| Function | Return |
|----------|--------|
| GetFragment(Class) | Single fragment or null |
| GetFragments(Class) | Array of fragments |

### D.15) Content Folder Quick Reference

| Folder | Key Contents | Father Usage |
|--------|--------------|--------------|
| Core/Abilities/Configurations | AC_NPC_Default, AC_Player_RPG | AC_FatherCompanion template |
| Core/Abilities/GameplayAbilities | GA_Death, Attacks/, Movement/ | Ability patterns |
| Core/Abilities/GameplayEffects | GE_Damage_SetByCaller, GE_Invulnerable | Effect patterns |
| Core/AI/Activities | FollowCharacter, Attacks, Idle | Father behaviors |
| Core/AI/BP | BP_NarrativeNPC, BP_NarrativeNPCController | Father base classes |
| Core/AI/Tasks | BTTask_ActivateAbilityByClass | AI ability triggering |
| Demo/Character/Definitions | Bandit, Player folders | NPCDefinition structure |

### D.16) C++ Module Quick Reference

| Module | Key Classes | Father Usage |
|--------|-------------|--------------|
| NarrativeArsenal/GAS | NarrativeGameplayAbility, NarrativeCombatAbility | All abilities |
| NarrativeArsenal/GAS | AbilityConfiguration, NarrativeAttributeSetBase | Father config |
| NarrativeArsenal/GAS | NarrativeDamageExecCalc, NarrativeHealExecution | Damage/heal |
| NarrativeArsenal/GAS/AbilityTasks | AbilityTask_SpawnProjectile | GA_FatherLaserShot |
| NarrativeArsenal/AI | NPCDefinition, NarrativeNPCController | Father definition |
| NarrativeArsenal/AI/Activities | NPCActivity, NPCGoal, NPCActivityConfiguration | Father behaviors |
| NarrativeSaveSystem | NarrativeSavableActor, SaveSystemStatics | Father persistence |

---

## APPENDIX E: RESOLVED TECHNICAL DECISIONS

| Item | Status | Resolution |
|------|--------|------------|
| Form State Architecture | COMPLETED | Activation Owned Tags replicate (v4.0) |
| Symbiote Effect Pattern | COMPLETED | Father applies to Player |
| GetAttitude Pattern Migration | COMPLETED | Faction system in all applicable guides |
| Net Execution Policy Updates | COMPLETED | Server Only applied to AI-autonomous abilities |
| Hierarchical Tag Migration | COMPLETED | Form-specific abilities use parent.child tags |
| F Key Detach Removal | COMPLETED | Direct form-to-form via T key wheel only (v4.0) |
| Form Cooldown | COMPLETED | 15s shared cooldown (Cooldown.Father.FormChange) |
| Symbiote Duration | COMPLETED | 30 seconds fixed timer |
| Exoskeleton Stats | COMPLETED | +50% Speed, +30% Jump, +10 AttackRating |
| EndAbility Cleanup Pattern | COMPLETED | Each form ability restores state on end |
| AI Following Architecture | COMPLETED | Goal Follow Character via dialogue (v4.3) |
| OwnerPlayer Scope | COMPLETED | Direct reference for abilities only (v4.3) |
| Blackboard Documentation | COMPLETED | All BB_ keys documented (v4.3) |
| NarrativeEvent Ownership | COMPLETED | NE_SetFatherOwner/NE_ClearFatherOwner pattern (v4.4) |
| Cross-Actor Granting | COMPLETED | SourceObject pattern, grant to player ASC (v4.4) |
| Ability Validation | COMPLETED | Tag + CanActivateAbility hybrid (v4.4) |
| Father.State.Recruited | COMPLETED | Primary gate for form abilities (v4.4) |
| Form-Based Invulnerability | **REMOVED (INV-1)** | ~~Narrative.State.Invulnerable for attached forms~~ Removed per GAS Audit |
| CharacterDefinition Persistence | COMPLETED | PrepareForSave/Initialize pattern (v4.4) |
| Handle Management | COMPLETED | Per-form handle variables (v4.4) |
| Cross-Actor Tag Cancellation | COMPLETED | Cancel Abilities with Tag is same-ASC only (v4.5) |
| Ability Handle Variables | COMPLETED | FGameplayAbilitySpecHandle for granted sub-abilities (v4.5) |
| Two-Step Cleanup Pattern | COMPLETED | Cancel Ability + Set Remove Ability On End for cross-actor (v4.5) |
| GA_Backstab Location | UPDATED | Moved to Player Default Abilities (v5.0) |
| EndPlay Safety Net | COMPLETED | Emergency cleanup for all cross-actor grants in EndPlay (v4.7) |
| HasAuthority Gates | COMPLETED | All cross-actor operations wrapped in authority checks (v4.7) |
| Form Ability Net Policy | COMPLETED | ServerOnly for form abilities granting to player ASC (v4.7) |
| Symbiote Auto-Return Fallback | COMPLETED | Armor -> Crawler -> Manual restoration (v4.7) |
| Manual Cleanup Pattern | COMPLETED | Handle-based alternative to RemoveActiveEffectsWithSourceObject (v4.7) |
| Narrative Pro Audit | COMPLETED | C++ source audit identified gaps, documented workarounds (v4.7) |
| Death Architecture | DEFERRED | Waiting for Narrative Pro v2.2 tether feature |
| v2.2 Plugin Migration | COMPLETED | Updated from v2.1 to v2.2, all systems compatible (v5.3) |
| Projectile System | COMPLETED | NarrativeProjectile + AbilityTask_SpawnProjectile for GA_FatherLaserShot, GA_TurretShoot (v5.3) |
| Melee Multi-Hit | COMPLETED | CachedHitActors pattern for GA_FatherAttack v3.3 (v5.3) |
| Item Fragment System | DOCUMENTED | AmmoFragment, PoisonableFragment for future father mods (v5.3) |
| Cover System | DOCUMENTED | NarrativeRecastNavMesh queries for turret placement (v5.3) |
| C++ Line Number Audit | COMPLETED | Comprehensive verification against v2.2 source code (v5.4) |
| BT Task System | COMPLETED | Use BTTask_ActivateAbilityByClass instead of custom tasks (v5.9) |
| BT Services | COMPLETED | BTS_FocusAttackTarget, BTS_ClearAIFocus, BTS_SetAIFocus, BTS_AdjustFollowSpeed fully documented (v5.10) |
| GA_Death Integration | COMPLETED | Add GA_Death to AC_FatherCompanion_Default (v5.10) |
| SetByCaller vs ExecCalc | COMPLETED | Keep NarrativeDamageExecCalc for father (armor respected) (v5.9) |
| Father Activity Mapping | COMPLETED | Mapped to BPA_Attack_Melee, BPA_Attack_Ranged_Strafe, etc. (v5.9) |
| Guide Updates Required | COMPLETED | GA_FatherLaserShot v3.7, GA_TurretShoot v2.9, GA_FatherElectricTrap v3.1 updated with BTTask_ActivateAbilityByClass (v5.10) |
| Father Follow Services | COMPLETED | Custom BT_FatherFollow created without BTS_SetAIFocus, BTS_AdjustFollowSpeed retained (v5.10) |
| Father ActivityConfiguration | COMPLETED | ActConfig_FatherCompanion created with BPA_FatherFollow, BPA_Attack_Melee, BPA_Idle (v5.10) |
| Child GE Architecture | COMPLETED | Child GEs without Components, form tags from Activation Owned Tags (v5.12) |
| Setup Guide v2.2 | COMPLETED | Remove Grant Tags Components from PHASE 30 child GEs (v5.12) |
| GA_FatherSymbiote v3.5 | COMPLETED | Remove GE_SymbioteBoost variable and cleanup code (v5.12) |
| Demo Weapon Visual Verification | COMPLETED | Dagger, Bow, Rifle, Greatsword patterns documented (v5.14) |
| Child GE Per Item Pattern | VERIFIED | GE_EquipmentModifier_Bow confirms child GE organization (v5.14) |
| LootTableRow Data Table | VERIFIED | ItemLoadout_Seth shows Item Collection loadout pattern (v5.14) |
| NPC_Seth Configuration | VERIFIED | NPCDefinition template for father confirmed (v5.14) |
| AC_RunAndGun Template | VERIFIED | ActivityConfiguration with 11 activities + GoalGenerator_Attack (v5.14) |
| AC_NPC_Default Template | VERIFIED | AbilityConfiguration with GA_Death confirmed (v5.14) |
| Narrative.Factions.Heroes | VERIFIED | Father faction assignment for auto-hostile detection (v5.14) |
| BPA_Attack_Melee Scoring | VERIFIED | ScoreGoalItem function with Hidden/Unreachable checks (v5.14) |
| GoalGenerator_Attack System | VERIFIED | Complete auto-targeting with faction filtering (v5.14) |
| Attack Affiliation Map | VERIFIED | Per-sense faction detection configuration (v5.14) |
| Report Noise Event | VERIFIED | NotifyTeammates pattern for team coordination (v5.14) |
| Team Coordination System | DOCUMENTED | Future NPC applications documented (v5.14) |

---

**Document Version**: 6.0
**Status**: Production Reference
**Total Sections**: 66
**Total Appendices**: 5
**New in v5.14**: Demo Weapon Visual Reference (60), NPC Definition Visual Reference (61), Activity Configuration Visual Reference (62), Ability Configuration Visual Reference (63), BPA_Attack_Melee Visual Reference (64), GoalGenerator_Attack Visual Reference (65), Team Coordination System (66)
**Verified Assets v5.14**: Weapon_DemoDagger, Weapon_DemoBow, Weapon_DemoRifle, Weapon_DemoGreatsword, NPC_Seth, ItemLoadout_Seth, AC_RunAndGun, AC_NPC_Default, BPA_Attack_Melee, GoalGenerator_Attack
**New in v5.13**: Tag format fix (Father.State.Alive)
**New in v5.12**: Child GE Architecture Decision (58.8), HandleEquip/HandleUnequip Lifecycle (58.9), Dual Entry System (58.10), Activities to Grant (58.11)
**Updated Sections v5.12**: Section 58.7 (duplicate stat issue RESOLVED)
**New in v5.11**: Complete BT Services documentation (55.4-55.15), BTS_ClearAIFocus, BTS_SetAIFocus, BTS_AdjustFollowSpeed event graphs, father follow services clarification
**Updated Sections v5.11**: Section 55 expanded from 5 subsections to 15 subsections
**New in v5.10**: BT Task System (54), BT Services (55), Reference Asset Analysis (56), Father to Narrative Pro Alignment (57)
**Updated Sections v5.10**: Appendix A (BTTask, BTS patterns), Appendix E (BT decisions, damage system, guide updates)
**New in v5.8**: Content folder structure (52), C++ module structure (53), expanded naming conventions
**Updated Sections v5.8**: Appendix C (BTS_, BTT_, BTTask_, EQSContext_, BI_, IC_, DT_, QBP_, A_, AM_, SM_, M_, T_ prefixes), Appendix D.15-D.16 (quick references)
**New in v5.7**: Faction attack chain (7.9), HandleDeath parameters (7.10), Hostiles array patterns (7.11), ActConfig_ naming convention
**Updated Sections v5.7**: 21.1 (ActivityConfiguration naming), Appendix C (prefix separation), Appendix D.2 (ActConfig_ prefix)
**New in v5.6**: Built-in cooldown system (CooldownGameplayEffectClass + CommitAbilityCooldown)
**Updated Sections v5.6**: PATTERN 3, PATTERN 7, 19.3, 19.6, 19.7.2, Form transition flows
**New in v5.5**: UE 5.6 GE component navigation, NarrativeAttributeSetBase attributes, Modifier operations
**Updated Sections v5.5**: 8.4-8.10 (Grant Tags navigation, component properties)
**New in v5.4**: C++ line number audit against Narrative Pro v2.2 source code
**New Sections v5.3**: 47 (v2.2 Overview), 48 (Projectile System), 49 (Melee Multi-Hit), 50 (Fragment System), 51 (Cover System)
**New Sections v5.2**: 46 (Goal/Activity Follow System Architecture)
**New Sections v5.1**: 42 (NPC Schedule System), 43 (Interaction Slot System), 44 (NarrativeInteractAbility), 45 (Time of Day Triggers)
**New Sections v4.7**: 39 (EndPlay Safety Net), 40 (Cross-Actor Authority Gates), 41 (Symbiote Fallback)
**New Sections v4.4**: 35 (NarrativeEvent), 36 (Cross-Actor Granting), 37 (Validation), 38 (Death/Invulnerability)
**Consolidates**: Father_Companion_Technical_Reference_v5_14.md
