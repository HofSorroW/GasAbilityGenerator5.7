# Father Companion - GA_FatherAttack Implementation Guide

## VERSION 3.5 - Form State Tag Update (INV-1 Compliant)

## Unreal Engine 5.6 + Narrative Pro Plugin v2.2

---

## **DOCUMENT INFORMATION**

| Property | Value |
|----------|-------|
| Ability Name | GA_FatherAttack |
| Ability Type | AI Combat - Melee (Multi-Hit) |
| Parent Class | GA_Melee_Unarmed |
| Form | Crawler (Active only in Crawler form) |
| Input | Narrative.Input.Father.Attack |
| Version | 3.5 |
| Last Updated | December 2025 |
| Engine | Unreal Engine 5.6 |
| Plugin | Narrative Pro v2.2 |
| Implementation | Blueprint Only (Child Blueprint + Override) |
| Multiplayer | Compatible (Server Only) |
| Dependencies | BP_FatherCompanion, GA_Melee_Unarmed, GE_WeaponDamage |

---

## **TABLE OF CONTENTS**

1. [INTRODUCTION](#introduction)
2. [PREREQUISITES](#prerequisites)
3. [PHASE 1: GAMEPLAY TAGS SETUP](#phase-1-gameplay-tags-setup)
4. [PHASE 2: CREATE CHILD ABILITY BLUEPRINT](#phase-2-create-child-ability-blueprint)
5. [PHASE 3: CONFIGURE ABILITY PROPERTIES](#phase-3-configure-ability-properties)
6. [PHASE 4: MULTI-HIT BLUEPRINT LOGIC](#phase-4-multi-hit-blueprint-logic)
7. [PHASE 5: GRANT ABILITY VIA ABILITYCONFIGURATION](#phase-5-grant-ability-via-abilityconfiguration)
8. [PHASE 6: ANIMATION SETUP](#phase-6-animation-setup)
9. [PHASE 7: FATHER SKELETON SOCKETS](#phase-7-father-skeleton-sockets)
10. [CHANGELOG](#changelog)
11. [QUICK REFERENCE](#quick-reference)

---

## **INTRODUCTION**

### **Ability Overview**

GA_FatherAttack is the primary melee combat ability for the Father Companion in Crawler form. This ability inherits from GA_Melee_Unarmed, gaining all melee combat functionality including socket-based sphere trace hit detection, warp-toward-enemy targeting, combo system support, and proper damage flow through NarrativeDamageExecCalc.

Version 3.3 adds multi-hit support using Narrative Pro v2.2's CachedHitActors pattern, allowing the father to damage multiple clustered enemies per swing while preventing double-hits on the same target.

### **Inherited Features from GA_Melee_Unarmed**

| Feature | Description |
|---------|-------------|
| Socket-Based Tracing | Sphere trace from skeleton socket location |
| Warp System | Father lunges toward enemies when attacking |
| Animation Events | Triggered via AnimNotify in attack montage |
| Damage Flow | Automatic via GE_WeaponDamage and NarrativeDamageExecCalc |
| Combo Support | Light/Heavy attack system (using light only for father) |
| Gameplay Cues | Fire cues for hit feedback |

### **v3.3 Multi-Hit Features**

| Feature | Description |
|---------|-------------|
| Multi-Target Trace | Sphere trace can hit multiple enemies |
| CachedHitActors | Prevents hitting same enemy twice per swing |
| Balanced Damage | Lower per-hit damage (8.0) for multi-hit balance |
| Wider Trace | Increased trace radius (75.0) for better coverage |

### **Activation Methods**

| Method | Trigger | Use Case |
|--------|---------|----------|
| AI Behavior Tree | BTTask_TryActivateAbilityByClass | Autonomous attacking (primary) |
| Input Tag | Narrative.Input.Father.Attack | Manual player control (optional) |

### **Form Restriction**

| Current Form | Can Activate | Reason |
|--------------|--------------|--------|
| Crawler | Yes | Required tags present |
| Armor | No | Blocked: Father.State.Attached |
| Exoskeleton | No | Blocked: Father.State.Attached |
| Symbiote | No | Required: Effect.Father.FormState.Crawler |
| Engineer | No | Required: Effect.Father.FormState.Crawler |

### **Recruitment Gate**

This ability requires Father.State.Recruited tag to activate. This prevents wild (unrecruited) fathers from using player-specific attack abilities.

---

## **PREREQUISITES**

### **Required Assets**

| Asset | Type | Status |
|-------|------|--------|
| BP_FatherCompanion | NarrativeNPCCharacter | Must exist |
| GA_Melee_Unarmed | GameplayAbility | Narrative Pro built-in |
| GE_WeaponDamage | GameplayEffect | Narrative Pro built-in |
| AC_FatherCompanion_Default | AbilityConfiguration | Must exist |
| Father Skeleton | Skeleton | Must have attack sockets |

### **Required Skeleton Sockets**

| Socket Name | Location | Purpose |
|-------------|----------|---------|
| father_right | Right fang/leg | Attack trace origin |
| father_left | Left fang/leg | Attack trace origin (alternate) |

### **Required Tags**

| Tag | Purpose | Reference |
|-----|---------|-----------|
| Effect.Father.FormState.Crawler | Form identification | DefaultGameplayTags_FatherCompanion_v3_5.ini |
| Father.State.Recruited | Recruitment gate | DefaultGameplayTags_FatherCompanion_v3_5.ini |
| Father.State.Attached | Blocked state | DefaultGameplayTags_FatherCompanion_v3_5.ini |

---

## **PHASE 1: GAMEPLAY TAGS SETUP**

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Crawler.Attack | Father melee attack ability |
| Narrative.Input.Father.Attack | Input tag for father attack |
| Cooldown.Father.Crawler.Attack | Cooldown tag for father attack |
| Father.State.Attacking | Father is currently attacking |

### **Verify Existing Tags**

| Tag Name | Purpose |
|----------|---------|
| Effect.Father.FormState.Crawler | Crawler form identification |
| Father.State.Attached | Attachment state (blocked) |
| Father.State.Recruited | Recruitment gate |

---

## **PHASE 2: CREATE CHILD ABILITY BLUEPRINT**

### **1) Create Child Blueprint**

#### 1.1) Navigate to Abilities Folder
   - 1.1.1) In Content Browser, navigate to /Content/FatherCompanion/Abilities/Crawler/

#### 1.2) Create Child Blueprint
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Blueprint Class
   - 1.2.3) In All Classes, search: GA_Melee_Unarmed
   - 1.2.4) Select GA_Melee_Unarmed as parent
   - 1.2.5) Name: GA_FatherAttack
   - 1.2.6) Double-click to open

---

## **PHASE 3: CONFIGURE ABILITY PROPERTIES**

### **1) Open Class Defaults**

#### 1.1) Access Properties
   - 1.1.1) Click Class Defaults button in toolbar

### **2) Configure Default Section**

#### 2.1) Socket Names
   - 2.1.1) Attack Right Socket: father_right
   - 2.1.2) Attack Left Socket: father_left

#### 2.2) Animation Sets
   - 2.2.1) Attack Combo Anim Set: (Father attack anim set - see PHASE 6)
   - 2.2.2) Heavy Attack Combo Anim Set: None

### **3) Configure Warping Section**

#### 3.1) Warp Settings (Keep Defaults or Adjust)
   - 3.1.1) Should Warp: Checked
   - 3.1.2) Warp Maintain Dist: 100.0
   - 3.1.3) Min Warp Dist: 20.0
   - 3.1.4) Max Warp Dist: 250.0

### **4) Configure Target Data Handling Section**

#### 4.1) Damage Effect
   - 4.1.1) Damage Effect Class: GE_WeaponDamage

### **5) Configure Damage Section**

#### 5.1) Attack Damage (Reduced for Multi-Hit Balance)
   - 5.1.1) Default Attack Damage: 8.0

### **6) Configure Combat Trace Data Section**

#### 6.1) Trace Settings for Multi-Hit
   - 6.1.1) Trace Distance: 200.0
   - 6.1.2) Trace Radius: 75.0
   - 6.1.3) Trace Multi: Checked

### **7) Configure Narrative Ability Section**

#### 7.1) Ammo and Debug
   - 7.1.1) Requires Ammo: Unchecked
   - 7.1.2) Draw Debug Traces: Unchecked (or Checked for testing)

#### 7.2) Input Tag
   - 7.2.1) Input Tag: Narrative.Input.Father.Attack

### **8) Configure Tags Section**

| Property | Tags |
|----------|------|
| Asset Tags | Ability.Father.Crawler.Attack |
| Activation Owned Tags | Ability.Father.Crawler.Attack, Father.State.Attacking |
| Activation Required Tags | Effect.Father.FormState.Crawler, Father.State.Recruited |
| Activation Blocked Tags | Narrative.State.Busy, Narrative.State.IsDead, Narrative.State.Movement.Climbing, Narrative.State.Movement.Falling, Narrative.State.OnMount, Narrative.State.Weapon.BlockFiring, Father.State.Attached |

### **9) Configure Replication Settings**

#### 9.1) Set Net Execution Policy
   - 9.1.1) In Details panel, find Advanced section
   - 9.1.2) Find Net Execution Policy dropdown
   - 9.1.3) Select: Server Only

### **10) Compile and Save**

#### 10.1) Save Blueprint
   - 10.1.1) Click Compile
   - 10.1.2) Click Save

---

## **PHASE 4: MULTI-HIT BLUEPRINT LOGIC**

### **1) Create CachedHitActors Variable**

#### 1.1) Open Variables Panel
   - 1.1.1) In GA_FatherAttack blueprint, click My Blueprint tab
   - 1.1.2) In Variables section, click + button

#### 1.2) Configure Variable
   - 1.2.1) Name: CachedHitActors
   - 1.2.2) Variable Type: Actor
   - 1.2.3) Click array icon to make it Array of Actor
   - 1.2.4) Click Compile

### **2) Override Handle Target Data Event**

#### 2.1) Open Event Graph
   - 2.1.1) Click Event Graph tab

#### 2.2) Add Override Event
   - 2.2.1) Right-click in Event Graph
   - 2.2.2) Search: Handle Target Data
   - 2.2.3) Select Handle Target Data under Add Event -> Narrative Combat Ability

### **3) Implement Multi-Hit Filter Logic**

#### 3.1) Get Target Data Length
   - 3.1.1) From Handle Target Data event
   - 3.1.2) Drag from Target Data pin
   - 3.1.3) Add Get Num node

#### 3.2) Create ForLoop
   - 3.2.1) From Handle Target Data execution pin
   - 3.2.2) Add For Loop node
   - 3.2.3) First Index: 0
   - 3.2.4) Connect Get Num Return Value to Last Index

#### 3.3) Get Hit Result from Target Data
   - 3.3.1) From For Loop -> Loop Body execution pin
   - 3.3.2) Drag from Target Data parameter
   - 3.3.3) Add Get Hit Result From Target Data node
   - 3.3.4) Connect For Loop Index to Index parameter

#### 3.4) Get Hit Actor
   - 3.4.1) From Get Hit Result From Target Data
   - 3.4.2) Drag from Hit Result output
   - 3.4.3) Add Break Hit Result node
   - 3.4.4) From Hit Actor pin, drag and release

#### 3.5) Check If Already Hit
   - 3.5.1) Add Is Valid node
   - 3.5.2) Connect Hit Actor to Is Valid input
   - 3.5.3) From Is Valid -> Is Valid execution pin
   - 3.5.4) Drag from CachedHitActors variable (Get)
   - 3.5.5) Add Contains node
   - 3.5.6) Connect Hit Actor to Contains Item input

#### 3.6) Branch on Contains Result
   - 3.6.1) Add Branch node
   - 3.6.2) Connect Contains Return Value to Condition
   - 3.6.3) True path: Skip (already hit this actor)
   - 3.6.4) False path: Continue to apply damage

### **4) Apply Damage to New Targets**

#### 4.1) Add Actor to Cache
   - 4.1.1) From Branch -> False execution pin
   - 4.1.2) Drag from CachedHitActors variable (Get)
   - 4.1.3) Add Add node (array operation)
   - 4.1.4) Connect Hit Actor to Add input

#### 4.2) Create Single Target Data Handle
   - 4.2.1) From Add execution pin
   - 4.2.2) Add Make Target Data Handle From Hit Result node
   - 4.2.3) Connect Hit Result from Break Hit Result to input

#### 4.3) Call Parent Handle Target Data
   - 4.3.1) Right-click in Event Graph
   - 4.3.2) Search: Parent Handle Target Data
   - 4.3.3) Select Add call to parent function Handle Target Data
   - 4.3.4) Connect execution from Make Target Data Handle
   - 4.3.5) Connect Target Data Handle output to Target Data input
   - 4.3.6) Connect Application Tag from original event to Application Tag input

### **5) Override End Ability Event**

#### 5.1) Add End Ability Override
   - 5.1.1) Right-click in Event Graph
   - 5.1.2) Search: End Ability
   - 5.1.3) Select Event End Ability under Add Event -> Gameplay Ability

#### 5.2) Clear Cache Before Ending
   - 5.2.1) From Event End Ability execution pin
   - 5.2.2) Drag from CachedHitActors variable (Get)
   - 5.2.3) Add Clear node (array operation)

#### 5.3) Call Parent End Ability
   - 5.3.1) From Clear execution pin
   - 5.3.2) Right-click and search: Parent End Ability
   - 5.3.3) Add call to parent function End Ability
   - 5.3.4) Connect Handle, Actor Info, Activation Info from event
   - 5.3.5) Connect Replicate End Ability and Was Cancelled from event

### **6) Compile and Save**

#### 6.1) Save Blueprint
   - 6.1.1) Click Compile
   - 6.1.2) Verify no errors
   - 6.1.3) Click Save

---

## **PHASE 5: GRANT ABILITY VIA ABILITYCONFIGURATION**

### **1) Open AbilityConfiguration Asset**

#### 1.1) Navigate to Asset
   - 1.1.1) In Content Browser, locate AC_FatherCompanion_Default
   - 1.1.2) Double-click to open

### **2) Add GA_FatherAttack to Abilities Array**

#### 2.1) Find Abilities Array
   - 2.1.1) In Details panel, find Abilities section
   - 2.1.2) Click + to add new element

#### 2.2) Configure Element
   - 2.2.1) Click dropdown on new element
   - 2.2.2) Search: GA_FatherAttack
   - 2.2.3) Select GA_FatherAttack

### **3) Save Configuration**

#### 3.1) Save Asset
   - 3.1.1) Click Save

---

## **PHASE 6: ANIMATION SETUP**

### **1) Create Father Attack Animation Set**

#### 1.1) Create NarrativeAnimSet Asset
   - 1.1.1) Right-click in Content Browser
   - 1.1.2) Select Miscellaneous -> Data Asset
   - 1.1.3) Select NarrativeAnimSet as class
   - 1.1.4) Name: NAS_FatherAttack

#### 1.2) Configure Animation Set
   - 1.2.1) Double-click to open
   - 1.2.2) Add attack montages to array

### **2) Create Attack Montage**

#### 2.1) Create Montage
   - 2.1.1) Right-click father attack animation
   - 2.1.2) Select Create -> Create Anim Montage
   - 2.1.3) Name: AM_FatherAttack

#### 2.2) Add Animation Notify
   - 2.2.1) Open AM_FatherAttack
   - 2.2.2) In Notifies track, right-click at strike frame
   - 2.2.3) Add Notify -> Play Gameplay Event Tag
   - 2.2.4) Set Event Tag: GameplayEvent.Attack.Activate.UnarmedRight

### **3) Link Animation Set to Ability**

#### 3.1) Open GA_FatherAttack
   - 3.1.1) Click Class Defaults

#### 3.2) Set Animation Set
   - 3.2.1) Attack Combo Anim Set: NAS_FatherAttack

### **4) Save All**

#### 4.1) Save Assets
   - 4.1.1) Click File -> Save All

---

## **PHASE 7: FATHER SKELETON SOCKETS**

### **1) Open Father Skeleton**

#### 1.1) Navigate to Skeleton
   - 1.1.1) In Content Browser, locate father skeleton asset
   - 1.1.2) Double-click to open

### **2) Add Attack Sockets**

#### 2.1) Add Right Socket
   - 2.1.1) In Skeleton Tree, right-click on appropriate bone (head/fang/leg)
   - 2.1.2) Select Add Socket
   - 2.1.3) Name: father_right

#### 2.2) Add Left Socket
   - 2.2.1) In Skeleton Tree, right-click on appropriate bone
   - 2.2.2) Select Add Socket
   - 2.2.3) Name: father_left

### **3) Position Sockets**

#### 3.1) Adjust Socket Transforms
   - 3.1.1) Select father_right socket
   - 3.1.2) In Details panel, adjust Relative Location to attack point
   - 3.1.3) Repeat for father_left

### **4) Save Skeleton**

#### 4.1) Save Asset
   - 4.1.1) Click Save

---

## **CHANGELOG**

### **Version 3.5 - January 2026**

| Change | Description |
|--------|-------------|
| Tag Configuration | PHASE 3 Section 8 - converted to single Property/Tags table |

---

### **Version 3.4 - January 2026**

| Change | Description |
|--------|-------------|
| Tag Creation | Simplified PHASE 1 - replaced detailed step-by-step instructions with simple tag list tables |

### **Version 3.3 - December 2025**

| Change | Description |
|--------|-------------|
| Multi-Hit Support | Added PHASE 4 for multi-hit Blueprint logic |
| CachedHitActors | New variable to prevent double-hits per swing |
| Handle Target Data Override | Filters hits through cache before applying damage |
| End Ability Override | Clears cache when ability ends |
| Trace Multi | Enabled multi-target trace in Combat Trace Data |
| Trace Radius | Increased from 65.0 to 75.0 for wider coverage |
| Trace Distance | Set to 200.0 for appropriate father range |
| Default Attack Damage | Reduced from 10.0 to 8.0 for multi-hit balance |
| Plugin Version | Updated from v2.1 to v2.2 |
| Phase Renumbering | Phases 4-6 became 5-7, new Phase 4 added |

### **Version 3.2 - December 2025**

| Change | Description |
|--------|-------------|
| Father.State.Recruited | Added to Activation Required Tags (2 elements total) |
| Prerequisites | Added Required Tags table with tag references |
| Introduction | Added Recruitment Gate section explaining tag purpose |
| Verify Tags | Added Father.State.Recruited to Section 3.1 |
| Related Documents | Added reference table |

### **Version 3.1 - December 2025**

| Change | Description |
|--------|-------------|
| Hierarchical tag | Changed Ability.Father.Attack to Ability.Father.Crawler.Attack |
| Cooldown tag | Changed Cooldown.Father.Attack to Cooldown.Father.Crawler.Attack |
| Net Execution Policy | Changed from Local Predicted to Server Only |
| Replication section | Added Section 8: Configure Replication Settings |

### **Version 3.0 - November 28, 2025**

| Change | Description |
|--------|-------------|
| New parent class | Changed from NarrativeCombatAbility to GA_Melee_Unarmed |
| Inherited logic | All melee trace logic inherited from parent |
| Warp system | Father warps toward enemies using parent's warp system |
| Simplified guide | Reduced from 8 phases to 6 phases |
| No custom logic | No Blueprint Event Graph implementation needed |
| Damage Effect | Uses existing GE_WeaponDamage instead of custom GE |
| Socket-based trace | Uses animation event with socket names |
| Attack damage | Set to 10.0 (lower than player's 13.2) |
| State tag added | Father.State.Attacking for state tracking per Technical Reference |
| Cooldown tag added | Cooldown.Father.Crawler.Attack per tag hierarchy standard |

---

## **QUICK REFERENCE**

### **1. Property Override Summary**

| Property | Parent Value | Father Override (v3.3) |
|----------|--------------|------------------------|
| Attack Right Socket | hand_r | father_right |
| Attack Left Socket | hand_l | father_left |
| Attack Combo Anim Set | Unarmed.Light | NAS_FatherAttack |
| Heavy Attack Combo Anim Set | Unarmed.Heavy | None |
| Damage Effect Class | GE_WeaponDamage_Heavy | GE_WeaponDamage |
| Default Attack Damage | 13.2 | 8.0 |
| Trace Distance | 500.0 | 200.0 |
| Trace Radius | 65.0 | 75.0 |
| Trace Multi | false | true |
| Input Tag | Narrative.Input.Attack | Narrative.Input.Father.Attack |
| AssetTags | Abilities.Attacks.MeleeAttack | Ability.Father.Crawler.Attack |
| Activation Owned Tags | Abilities.Attacks.MeleeAttack | Ability.Father.Crawler.Attack, Father.State.Attacking |
| Activation Required Tags | Empty | Effect.Father.FormState.Crawler, Father.State.Recruited |
| Activation Blocked Tags | (existing list) | (existing) + Father.State.Attached |

### **2. Multi-Hit Configuration (v3.3 New)**

| Property | Value | Purpose |
|----------|-------|---------|
| CachedHitActors | Array of Actor | Tracks hit actors per swing |
| Trace Multi | true | Enable multi-target detection |
| Trace Radius | 75.0 | Wider sweep area |
| Trace Distance | 200.0 | Father melee range |
| Default Attack Damage | 8.0 | Balanced for multi-hit |

### **3. Inherited from Parent (No Changes)**

| Property | Value |
|----------|-------|
| Should Warp | Checked |
| Warp Maintain Dist | 100.0 |
| Min Warp Dist | 20.0 |
| Max Warp Dist | 250.0 |
| Requires Ammo | Unchecked |
| Sphere Trace Logic | Event AnimationEvent_Activate |
| Finalize Target Data | Automatic |

### **4. Tag Configuration**

| Tag Type | Tag Name |
|----------|----------|
| Ability Tag | Ability.Father.Crawler.Attack |
| Input Tag | Narrative.Input.Father.Attack |
| State Tag | Father.State.Attacking |
| Cooldown Tag | Cooldown.Father.Crawler.Attack |
| Activation Required | Effect.Father.FormState.Crawler, Father.State.Recruited |
| Activation Blocked | Father.State.Attached |

### **5. Assets Created**

| Asset | Type | Location |
|-------|------|----------|
| GA_FatherAttack | Child of GA_Melee_Unarmed | /FatherCompanion/Abilities/Crawler/ |
| NAS_FatherAttack | NarrativeAnimSet | /FatherCompanion/Animations/ |
| AM_FatherAttack | AnimMontage | /FatherCompanion/Animations/ |

### **6. Skeleton Sockets Required**

| Socket Name | Parent Bone | Purpose |
|-------------|-------------|---------|
| father_right | Head/Fang/Leg | Attack trace origin |
| father_left | Head/Fang/Leg | Alternate trace origin |

### **7. Multi-Hit Attack Flow (v3.3)**

| Step | System | Action |
|------|--------|--------|
| 1 | Ability System | GA_FatherAttack activates |
| 2 | Warp System | Father warps toward enemy |
| 3 | Animation | Attack montage plays |
| 4 | AnimNotify | Triggers GameplayEvent.Attack.Activate.UnarmedRight |
| 5 | GA_Melee_Unarmed | Event AnimationEvent_Activate fires |
| 6 | Parent Logic | Gets socket location (father_right or father_left) |
| 7 | Parent Logic | Sphere trace multi from socket |
| 8 | Parent Logic | Multiple hits detected, creates target data array |
| 9 | Handle Target Data | Override receives multi-hit target data |
| 10 | For Each Hit | Loop through all hit results |
| 11 | Cache Check | Check if actor in CachedHitActors |
| 12 | If Not Cached | Add to cache, call parent with single hit |
| 13 | Parent Logic | Finalize Target Data for each new hit |
| 14 | GE_WeaponDamage | Applied to each unique target |
| 15 | NarrativeDamageExecCalc | Calculates final damage per target |
| 16 | End Ability | Clear CachedHitActors array |

### **8. Related Documents**

| Document | Version | Purpose |
|----------|---------|---------|
| Father_Companion_Technical_Reference | v5.2 | Cross-actor patterns, cleanup architecture |
| DefaultGameplayTags_FatherCompanion | v3.5 | Tag definitions |
| Father_Companion_System_Setup_Guide | v2.0 | BP_FatherCompanion setup |
| GA_FatherCrawler_Implementation_Guide | v2.9 | Form ability that enables Attack |
| NarrativePro_v2_2_Melee_Analysis | v1.0 | v2.2 melee system analysis |

---

**END OF GUIDE**
