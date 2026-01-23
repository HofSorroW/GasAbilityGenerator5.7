# GA_Backstab Implementation Guide v1.9

## Passive Damage Bonus Ability - Goal_Attack Query Approach

### Unreal Engine 5.7 + Narrative Pro v2.2

### Blueprint-Only Implementation

---

## DOCUMENT INFORMATION

| Property | Value |
|----------|-------|
| Document Version | 1.9 |
| Ability Name | GA_Backstab |
| Ability Type | Passive (Triggered on Attack Hit) |
| Parent Class | NarrativeGameplayAbility |
| Form Availability | All Forms (Crawler, Armor, Exoskeleton, Symbiote, Engineer) |
| Input Required | None (Auto-Trigger) |
| Grant Location | Player Default Abilities array |
| Detection Method | **Goal_Attack Query** (v5.1) |

---

## VERSION 1.9 CHANGES (Goal_Attack Approach)

| Change | Details |
|--------|---------|
| **Detection Method** | ViewedCharacter → Goal_Attack Query |
| **Phase 3 REMOVED** | No longer need to modify BP_NarrativeNPCController |
| **CheckBackstabCondition** | Now queries NPCActivityComponent → Goal_Attack → TargetToAttack |
| **Full Automation** | No manual perception binding required |
| **Manifest Updated** | GA_Backstab custom_functions use Goal_Attack query |

---

## TABLE OF CONTENTS

| Section | Description |
|---------|-------------|
| Phase 1 | Gameplay Tags Setup |
| Phase 2 | Create Gameplay Effect |
| Phase 3 | **REMOVED** - No NPC Controller changes needed |
| Phase 4 | Create GA_Backstab Ability (Auto-Generated) |
| Phase 5 | Integrate with Attack Ability |
| Phase 6 | Add to Player Default Abilities |

---

## PREREQUISITES

| Requirement | Description |
|-------------|-------------|
| GoalGenerator_Attack | NPCs must have GoalGenerator_Attack in their ActivityConfiguration |
| Player Attack Ability | Existing attack ability with hit detection logic |
| Player Blueprint | BP_NarrativePlayer or similar with Default Abilities array |
| Faction Configuration | NPCs configured with hostile factions (automatic via Narrative Pro) |

**No longer required:**
- ~~BP_NarrativeNPCController modification~~
- ~~ViewedCharacter variable~~
- ~~AIPerception binding~~

---

## QUICK REFERENCE

### AUTOMATION VS MANUAL (v5.1 - Goal_Attack Approach)

| Feature | Automation Status | Notes |
|---------|-------------------|-------|
| GA_Backstab Blueprint | ✅ Auto-generated | manifest.yaml creates ability with tags/properties |
| GE_BackstabBonus Effect | ✅ Auto-generated | manifest.yaml creates instant +25 AttackRating |
| CheckBackstabCondition Function | ✅ Auto-generated | Goal_Attack query via NPCActivityComponent |
| ApplyBackstabBonus Function | ✅ Auto-generated | BP_ApplyGameplayEffectToOwner node graph |
| NPC Controller Changes | ✅ NOT NEEDED | Goal_Attack approach uses existing Narrative Pro system |
| Player Attack Integration | ⚠️ Manual | Add backstab check to hit processing (Phase 5) |
| Player Default Abilities | ⚠️ Manual | Add GA_Backstab to array (Phase 6) |

**Why Goal_Attack Approach is Fully Automated:**
Narrative Pro's GoalGenerator_Attack automatically creates Goal_Attack when NPCs detect hostile actors via AIPerception. The CheckBackstabCondition function simply queries this existing goal system - no custom perception binding needed.

### How Goal_Attack Detection Works

```
AIPerception detects player
       ↓
GoalGenerator_Attack.OnPerceptionUpdated fires
       ↓
DoesAttitudeMatchFilter (faction check)
       ↓
If HOSTILE → TryAddAttackGoalFromActor
       ↓
Goal_Attack created with TargetToAttack = Player
       ↓
CheckBackstabCondition queries: Does enemy have Goal_Attack targeting me?
       ↓
If NO Goal_Attack OR target != player → BACKSTAB VALID
```

### Tag Configuration Summary

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Backstab |
| Effect Tag | Effect.Backstab |
| Event Tag | Event.Combat.Backstab |
| Activation Required | None (passive, triggered by attack) |
| InputTag | None (auto-trigger) |

### Gameplay Effect Summary

| Effect | Duration | AttackRating Bonus |
|--------|----------|-------------------|
| GE_BackstabBonus | Instant | +25 |

### Ability Configuration Summary

| Property | Value |
|----------|-------|
| Parent Class | NarrativeGameplayAbility |
| Instancing Policy | Instanced Per Actor |
| Replication Policy | Replicate Yes |
| Net Execution Policy | Local Predicted |
| Grant Location | Player Default Abilities array |

---

## ABILITY OVERVIEW

| Parameter | Value |
|-----------|-------|
| Detection Method | Goal_Attack Query via NPCActivityComponent |
| Backstab Condition | Enemy has no Goal_Attack targeting the player |
| Backstab Bonus | +25% damage (via AttackRating) |
| Decoy Mechanic | Father distracts enemy (enemy's Goal_Attack targets Father, not player) |

---

## DAMAGE STACKING SYSTEM

| Bonus Source | AttackRating | Managed By |
|--------------|--------------|------------|
| Backstab (enemy not targeting player) | +25 | GA_Backstab |
| Stealth (first attack from stealth) | +50 | GA_StealthField |
| Combined (stacks additively) | +75 | Independent systems |

---

## DAMAGE MULTIPLIER TABLE

| Condition | Enemy Goal_Attack Targeting | Player Stealthed | Multiplier |
|-----------|---------------------------|------------------|------------|
| Normal Attack | Player | No | 1.0x (100%) |
| Backstab Only | Not Player (or none) | No | 1.25x (125%) |
| Stealth Only | Player | Yes | 1.5x (150%) |
| Stealth + Backstab | Not Player (or none) | Yes | 1.75x (175%) |

---

## TACTICAL SCENARIOS (Goal_Attack Approach)

| Scenario | Enemy's Goal_Attack | Backstab Valid |
|----------|---------------------|----------------|
| Enemy attacking Player | Target = Player | No |
| Enemy has no combat goal | No Goal_Attack | Yes |
| Father distracts enemy | Target = Father | Yes |
| Player in Stealth | No Goal_Attack (not detected) | Yes |
| Enemy attacking other NPC | Target = Other NPC | Yes |

---

## DECOY TACTICAL FLOW (v5.1)

| Step | Action | Goal_Attack State |
|------|--------|-------------------|
| 1 | Player sends Father (Crawler form) toward enemy | No Goal_Attack yet |
| 2 | Enemy AIPerception detects Father | GoalGenerator_Attack fires |
| 3 | Faction check: Father = Hostile | TryAddAttackGoalFromActor creates Goal_Attack |
| 4 | Goal_Attack.TargetToAttack = Father | Enemy now attacking Father |
| 5 | Player flanks from behind | Player not detected OR has no Goal_Attack |
| 6 | Player attacks enemy | CheckBackstabCondition runs |
| 7 | Query: Enemy.Goal_Attack.TargetToAttack | Returns Father (not Player) |
| 8 | Compare: Father != Player | TRUE |
| 9 | BACKSTAB VALID | Apply GE_BackstabBonus (+25) |

---

## PHASE 1: GAMEPLAY TAGS SETUP

### Create Required Tags

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Backstab | Identifies backstab ability |
| Effect.Backstab | Applied when backstab bonus active |
| Event.Combat.Backstab | Fired when backstab executed |

**Note:** These tags are defined in manifest.yaml and auto-generated.

---

## PHASE 2: CREATE GAMEPLAY EFFECT

**Note:** GE_BackstabBonus is auto-generated from manifest.yaml. This section is for reference only.

### GE_BackstabBonus Configuration

| Property | Value |
|----------|-------|
| Duration Policy | Instant |
| Modifier Attribute | NarrativeAttributeSetBase.AttackRating |
| Modifier Op | Add |
| Magnitude | 25.0 |
| Granted Tags | Effect.Backstab |

---

## PHASE 3: REMOVED (v5.1)

**This phase is no longer needed with the Goal_Attack approach.**

The previous ViewedCharacter approach required:
- ~~Adding ViewedCharacter variable to BP_NarrativeNPCController~~
- ~~Binding OnPerceptionUpdated delegate manually~~
- ~~Implementing sight detection logic~~

With Goal_Attack, all of this is handled automatically by Narrative Pro's existing GoalGenerator_Attack system.

---

## PHASE 4: GA_BACKSTAB ABILITY (AUTO-GENERATED)

**Note:** GA_Backstab is auto-generated from manifest.yaml. This section describes the generated Blueprint.

### Manifest Definition (manifest.yaml)

```yaml
- name: GA_Backstab
  folder: Abilities/Actions
  parent_class: NarrativeGameplayAbility
  instancing_policy: InstancedPerActor
  net_execution_policy: LocalPredicted
  tags:
    ability_tags:
      - Ability.Father.Backstab
    activation_owned_tags:
      - State.BackstabReady
  custom_functions:
    - function_name: CheckBackstabCondition
      pure: false
      inputs:
        - name: EnemyActor
          type: Actor
      outputs:
        - name: CanBackstab
          type: Boolean
      # Node graph queries Goal_Attack system
    - function_name: ApplyBackstabBonus
      pure: false
      # Applies GE_BackstabBonus to owner
```

### CheckBackstabCondition Function (Goal_Attack Query)

The generated function queries the enemy's goal system:

| Step | Node | Purpose |
|------|------|---------|
| 1 | GetAvatarActorFromActorInfo | Get player (self) for comparison |
| 2 | GetComponentByClass(EnemyActor, NPCActivityComponent) | Get enemy's activity component |
| 3 | IsValid(Component) | Check if enemy has activity system |
| 4 | GetCurrentActivityGoal | Get enemy's current goal |
| 5 | Cast to Goal_Attack | Check if enemy is in attack mode |
| 6 | GetProperty(TargetToAttack) | Get who enemy is attacking |
| 7 | NotEqual(Target, Player) | Compare target to player |
| 8 | OR logic | No component OR no goal OR target != player = CAN BACKSTAB |

### Node Graph Flow

```
[FunctionEntry: EnemyActor]
         ↓
[GetComponentByClass] → NPCActivityComponent
         ↓
[IsValid] → [NOT] → ─────────────────────────────┐
         ↓                                        │
[GetCurrentActivityGoal]                          │
         ↓                                        │
[Cast to Goal_Attack]                             │
         ↓ (success)        ↓ (fail)              │
[GetProperty:          [LiteralTrue] ────────────┤
 TargetToAttack]              ↓                   │
         ↓              [Select] ←───────────────┤
[GetAvatarActor]              ↓                   │
         ↓              [OR] ←────────────────────┘
[NotEqual] ──────────────────→↓
                        [FunctionResult: CanBackstab]
```

### Return Value Logic

| Condition | Return |
|-----------|--------|
| Enemy has no NPCActivityComponent | TRUE (can backstab) |
| Enemy has no current Goal_Attack | TRUE (can backstab) |
| Enemy's Goal_Attack targets someone else | TRUE (can backstab) |
| Enemy's Goal_Attack targets player | FALSE (no backstab) |

### ApplyBackstabBonus Function

| Step | Node | Purpose |
|------|------|---------|
| 1 | BP_ApplyGameplayEffectToOwner | Apply GE_BackstabBonus to player |
| | GameplayEffectClass: GE_BackstabBonus | +25 AttackRating |
| | Level: 1 | Standard level |

---

## PHASE 5: INTEGRATE WITH ATTACK ABILITY

### 17) Open Player Attack Ability

#### 17.1) Locate Attack Ability
   - In Content Browser, navigate to your attack abilities folder
   - Find your primary melee attack ability
   - Double-click to open ability blueprint

### 18) Add Backstab Check to Hit Detection

#### 18.1) Locate Hit Processing Logic
   - In Event Graph, find where attack hit is processed
   - This is typically after trace hit or overlap detection
   - Identify the Hit Actor reference point

#### 18.2) Get Backstab Ability Reference

##### 18.2.1) Get ASC
   - Add: **Get Ability System Component from Actor Info** node

##### 18.2.2) Get Activatable Abilities
   - From ASC Return Value, search: `Get Activatable Abilities`

##### 18.2.3) Find Backstab Ability
   - From Activatable Abilities array, add **For Each Loop**
   - From Array Element, add **Get Class** node
   - Add **Equal (Class)** node, compare to `GA_Backstab`
   - Branch on result
   - If True: Cast to GA_Backstab and store reference

#### 18.3) Call Backstab Check

##### 18.3.1) Check Condition
   - From BackstabAbilityRef, call **CheckBackstabCondition**
   - Connect: Hit Actor to **EnemyActor** input

##### 18.3.2) Branch on Result
   - Add **Branch** node
   - Connect: **CanBackstab** output to Branch **Condition**

##### 18.3.3) Apply Backstab Bonus (True Branch)
   - From Branch **True** execution
   - Call **ApplyBackstabBonus** function

##### 18.3.4) Continue to Damage (Both Branches)
   - Connect both branches to damage application logic

---

## PHASE 6: ADD TO PLAYER DEFAULT ABILITIES

### 21) Configure Player Blueprint

#### 21.1) Open Player Blueprint
   - In Content Browser: Navigate to player blueprint folder
   - Double-click: `BP_NarrativePlayer` (or your player blueprint)
   - Click: **Class Defaults** button

#### 21.2) Find Default Abilities Array
   - In Details panel: Search `abilit`
   - Locate: **Narrative** -> **Abilities** category
   - Find: **Default Abilities** array

#### 21.3) Add GA_Backstab
   - Click: **+** button to add element
   - Dropdown: Select **GA_Backstab**

#### 21.4) Save
   - Click: **Compile** button
   - Click: **Save** button

---

## COMPLETE TAG REFERENCE

| Tag | Purpose |
|-----|---------|
| `Ability.Father.Backstab` | Identifies backstab ability |
| `Effect.Backstab` | Applied when backstab bonus active |
| `Event.Combat.Backstab` | Fired when backstab executed |

---

## GAMEPLAY EFFECT REFERENCE

| Effect | AttackRating Bonus | Condition |
|--------|-------------------|-----------|
| `GE_BackstabBonus` | +25 | Enemy Goal_Attack.TargetToAttack != Player |
| `GE_StealthDamageBonus` | +50 | Player stealthed (managed by GA_StealthField) |

---

## INTEGRATION SUMMARY

| Component | Modification Required |
|-----------|----------------------|
| BP_NarrativeNPCController | **NONE** (v5.1 - uses existing goal system) |
| Player Blueprint | Add GA_Backstab to Default Abilities array |
| NPCs | Must have GoalGenerator_Attack in ActivityConfiguration |
| Player Attack Ability | Add backstab check logic |
| GA_StealthField | None (stealth bonus stacks independently) |

---

## RELATED DOCUMENTS

| Document | Purpose |
|----------|---------|
| Father_Companion_Technical_Reference_v6_4.md | Section 5.5: Backstab Detection Logic (Goal_Attack Query) |
| Father_Companion_Technical_Reference_v6_4.md | Section 7.9: Faction + GoalGenerator Attack Chain |
| Father_Companion_Technical_Reference_v6_4.md | Section 23: Goal System (Goal_Attack) |
| Father_Companion_System_Design_Document_v2_6.md | Section 3.1: GA_Backstab overview |
| GA_StealthField_Implementation_Guide_v3_2.md | Stealth damage bonus that stacks with backstab |
| manifest.yaml | GA_Backstab definition with Goal_Attack query nodes |

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 1.9 | January 2026 | **v5.1 Goal_Attack Approach:** Replaced ViewedCharacter detection with Goal_Attack query. REMOVED Phase 3 (no NPC controller changes needed). CheckBackstabCondition now queries NPCActivityComponent → GetCurrentActivityGoal → Cast to Goal_Attack → TargetToAttack. Full automation - no manual perception binding required. Updated all tables and flow diagrams. |
| 1.8 | January 2026 | AUTOMATION VS MANUAL Table Added: Clarified what is auto-generated vs manual. |
| 1.7 | January 2026 | Locked Decisions Reference: Decision 4 (GA_Backstab is UNIVERSAL player ability). |
| 1.6 | January 2026 | Updated Narrative Pro version to v2.2. |
| 1.5 | January 2026 | Simplified PHASE 1 tag creation. |
| 1.4 | December 2025 | Changed Grant Location to Player Default Abilities array. |
| 1.3 | December 2025 | Changed Grant Location to InitOwnerReference function. |
| 1.2 | December 2025 | Added Quick Reference section. |
| 1.1 | November 2025 | Made guide fully standalone. |
| 1.0 | November 2025 | Initial implementation. |

---

**END OF GA_BACKSTAB IMPLEMENTATION GUIDE v1.9**

**Passive Damage Bonus - Goal_Attack Query Approach**

**Unreal Engine 5.7 + Narrative Pro v2.2**

**Locked Decision: Decision 4 - Universal player ability (not Exoskeleton-only)**

**v5.1: Fully Automated via Narrative Pro Goal System**
