# Father Companion - GA_FatherEngineer Implementation Guide
## VERSION 4.9 - NL-GUARD-IDENTITY L1 Compliant (3-Layer Guards Updated)
## Unreal Engine 5.7 + Narrative Pro Plugin v2.2

**Version:** 4.9
**Date:** January 2026
**Engine:** Unreal Engine 5.7
**Plugin:** Narrative Pro v2.2
**Implementation:** Blueprint Only
**Parent Class:** NarrativeGameplayAbility
**Architecture:** Option B (GE-Based Form Identity) - See Form_State_Architecture_Fix_v4.13.2.md

---

## **AUTOMATION VS MANUAL**

| Feature | Automation Status | Notes |
|---------|-------------------|-------|
| GE_EngineerState definition | ✅ Auto-generated | manifest.yaml gameplay_effects section |
| GA_FatherEngineer blueprint | ✅ Auto-generated | manifest.yaml gameplay_abilities section |
| Activation tags config | ✅ Auto-generated | Required/Blocked tags in manifest |
| Transition prelude nodes | ✅ Auto-generated | RemovePriorFormState + ApplyEngineerState in event_graph |
| Deployment logic | ✅ Auto-generated | LineTrace, SetActorLocation, Run Behavior Tree in event_graph |
| VFX spawning | ✅ Auto-generated | SpawnSystemAttached nodes in event_graph |
| 3-Layer Guards | ✅ Auto-generated | NL-GUARD-IDENTITY L1 pattern (v5.0) |
| EndAbility cleanup | ✅ Auto-generated | Event_EndAbility with bWasCancelled check |

---

## **DOCUMENT INFORMATION**

| Field | Value |
|-------|-------|
| Ability Name | GA_FatherEngineer |
| Ability Type | Form Activation (Deployment) |
| Parent Class | NarrativeGameplayAbility |
| Form | Engineer (Stationary Turret) |
| Input | T (Form Wheel) |
| Version | 4.4 |
| Last Updated | January 2026 |
| Estimated Time | 2-3 hours |

---

## **TABLE OF CONTENTS**

1. [Introduction](#introduction)
2. [Prerequisites](#prerequisites)
3. [PHASE 1: Gameplay Tags Setup](#phase-1-gameplay-tags-setup)
4. [PHASE 2: Create Gameplay Effects](#phase-2-create-gameplay-effects)
5. [PHASE 3: Create GA_FatherEngineer Ability](#phase-3-create-ga_fatherengineer-ability)
6. [PHASE 4: Implement Deployment Position Logic](#phase-4-implement-deployment-position-logic)
7. [PHASE 5: Implement Turret Setup Logic](#phase-5-implement-turret-setup-logic)
8. [PHASE 5A: Form Switch With Transition Animation](#phase-5a-form-switch-with-transition-animation)
9. [PHASE 5B: EndAbility Cleanup](#phase-5b-endability-cleanup)
10. [PHASE 6: Configure Ability Granting](#phase-6-configure-ability-granting)
11. [PHASE 7: Input Configuration](#phase-7-input-configuration)
12. [Changelog](#changelog)
13. [Quick Reference](#quick-reference)

---

## **INTRODUCTION**

### **Ability Overview**

GA_FatherEngineer is the form activation ability that transforms the father companion into a stationary turret. When activated, the father deploys at the player's aimed location and becomes a fixed defensive platform with autonomous targeting capabilities.

### **Key Features**

- Deployment System: Father teleports/moves to aimed location
- Stationary Mode: Father becomes fixed position turret
- 270 Degree Coverage: Turret can rotate within limited arc
- Health Pool: Turret can be damaged and destroyed
- AI Autonomous: Turret shoots and deploys traps automatically
- Form Transition: Switch to other forms via T wheel (with 5s animation)
- Area Denial: Controls chokepoints and flanks

### **Engineer Form Context**

The Engineer form transforms the father into a tactical asset. Player aims at deployment location, father deploys and becomes stationary, turret automatically engages enemies, player is free to focus on other combat, and turret remains until player switches to another form or turret is destroyed.

### **Form Comparison**

| Form | Father State | Player Benefit |
|------|--------------|----------------|
| Crawler | Following | AI combat support |
| Armor | Chest attached | Damage reduction |
| Exoskeleton | Back attached | Mobility enhancement |
| Symbiote | Full merge | Berserker power |
| **Engineer** | **Deployed turret** | **Area control** |

### **Technical Specifications**

| Parameter | Value |
|-----------|-------|
| Deployment Range | 1500 units max |
| Min Deploy Range | 200 units |
| Detection Range | 1500 units |
| Shoot Range | 500-1500 units |
| Trap Range | Less than 500 units |
| Rotation Arc | 270 degrees |
| Turret Health | 500 HP |
| Form Input | T (Wheel Selection) |

### **Implementation Patterns Used**

| Pattern # | Pattern Name | Purpose |
|-----------|--------------|---------|
| 3 | Tag-Based Activation | Form/state restrictions |
| 5 | Multiplayer Configuration | Net Execution, replication |
| 6 | Designer-Friendly Variables | Instance Editable properties |

---

## **PREREQUISITES**

### **1) Required Assets**

#### 1.1) Father Character
   - 1.1.1) BP_FatherCompanion exists
   - 1.1.2) Parent class: NarrativeNPCCharacter
   - 1.1.3) Has NarrativeAbilitySystemComponent

#### 1.2) Required Effects
   - 1.2.1) GE_TurretMode (stat modifications)
   - 1.2.2) Father.State.Transitioning tag (transition state - **NO invulnerability** per INV-1)
   - 1.2.3) GE_FormChangeCooldown (15s cooldown)

#### 1.3) Form System Foundation
   - 1.3.1) GA_FatherCrawler implemented (default form)
   - 1.3.2) EFatherForm enum created with existing forms
   - 1.3.3) Form wheel UI functional
   - 1.3.4) BP_FatherEngineerForm EquippableItem created per Father_Companion_Forms_Implementation_Guide_v4_0 PHASE 7

#### 1.4) Player Character
   - 1.4.1) Player character with camera for aiming
   - 1.4.2) FatherCompanionRef variable exists

#### 1.5) Narrative Pro v2.2
   - 1.5.1) Plugin installed and enabled
   - 1.5.2) AbilityConfiguration system understood

#### 1.6) Required Project Setting
   - 1.6.1) ReplicateActivationOwnedTags: Enabled
   - 1.6.2) Location: Project Settings -> Gameplay Abilities

#### 1.7) GE_EngineerState Gameplay Effect Must Exist
   - 1.7.1) Duration: Infinite
   - 1.7.2) Grants: `Effect.Father.FormState.Engineer`
   - 1.7.3) Form identity persists via this GE (Option B architecture)

### **2) Required Variables in BP_FatherCompanion**

#### 2.1) Existing Variables (Should Already Exist)

| Variable | Type | Replicated | Purpose |
|----------|------|------------|---------|
| OwnerPlayer | NarrativeCharacter Ref | Yes | Reference to owning player |
| CurrentForm | EFatherForm | RepNotify | Current active form |

#### 2.2) New Variables (Created in This Guide)

| Variable | Type | Replicated | SaveGame | Purpose |
|----------|------|------------|----------|---------|
| IsDeployed | Boolean | Yes | Yes | Turret deployment state |
| DeployedTransform | Transform | Yes | Yes | Turret world position and rotation |
| TurretModeEffectHandle | Active GE Handle | No | No | Effect handle for EndAbility cleanup |

#### 2.3) Father Health (Native Attributes)

Turret health uses father's existing NarrativeAttributeSetBase attributes:

| Attribute | Source | Purpose |
|-----------|--------|---------|
| Health | NarrativeAttributeSetBase | Current turret HP |
| MaxHealth | NarrativeAttributeSetBase | Maximum turret HP |

GE_TurretMode modifies MaxHealth to 500 when deploying as turret.

### **3) Required Gameplay Tags**

#### 3.1) Form State Tags (Activation Owned Tags)

| Tag | Purpose |
|-----|---------|
| Father.Form.Engineer | Form identification (auto-granted) |
| Father.State.Deployed | Deployment state (auto-granted) |

#### 3.2) Effect Asset Tags

| Tag | Purpose |
|-----|---------|
| Effect.Father.TurretMode | Asset tag for GE_TurretMode |

---

## **PHASE 1: GAMEPLAY TAGS SETUP**

### **Verify Existing Tags**

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Engineer | Form activation ability |
| Effect.Father.FormState.Engineer | Form identity tag (granted by GE_EngineerState) |
| Father.State.TurretDeployed | Turret deployment state (Activation Owned Tag) |
| Father.State.TurretActive | Granted by GE_TurretMode effect |
| Effect.Father.TurretMode | Asset tag for GE_TurretMode effect |
| Cooldown.Father.Engineer | Post-recall cooldown |
| Father.State.Alive | Father must be alive (Activation Required) |
| Father.State.Recruited | Father must be recruited (Activation Required) |
| Father.State.Dormant | Blocked after sacrifice |
| Father.State.Transitioning | Blocked during 5s transition |
| Father.State.SymbioteLocked | Blocked during 30s Symbiote |
| Cooldown.Father.FormChange | 15s shared form cooldown |

**Note (v4.4):** `Father.Form.*` tags are **orphan tags** - no persistent GE grants them. Form identity uses `Effect.Father.FormState.*` tags granted by infinite-duration GE_*State effects. Do NOT use Father.Form.* for activation gating.

---

## **PHASE 2: CREATE GAMEPLAY EFFECTS**

### **1) Create Effects Folder**

#### 1.1) Navigate to Engineer Folder
   - 1.1.1) In Content Browser, go to `/Content/FatherCompanion/`
   - 1.1.2) Right-click in Content Browser
   - 1.1.3) Select **New Folder**
   - 1.1.4) Name: `Engineer`
   - 1.1.5) Press **Enter**

#### 1.2) Create Effects Subfolder
   - 1.2.1) Double-click **Engineer** folder to open
   - 1.2.2) Right-click in Content Browser
   - 1.2.3) Select **New Folder**
   - 1.2.4) Name: `Effects`
   - 1.2.5) Press **Enter**
   - 1.2.6) Double-click **Effects** folder to open

### **2) Create GE_TurretMode**

#### 2.1) Create Gameplay Effect Asset
   - 2.1.1) Right-click in `/Content/FatherCompanion/Engineer/Effects/`
   - 2.1.2) Select **Gameplay** -> **Gameplay Effect**
   - 2.1.3) Name: `GE_TurretMode`
   - 2.1.4) Double-click to open

#### 2.2) Configure GE_TurretMode Properties

| Property | Value |
|----------|-------|
| Duration Policy | Infinite |

#### 2.3) Configure GE_TurretMode Components

| Component | Configuration |
|-----------|---------------|
| Grant Tags to Target Actor | Father.State.Deployed, Father.State.TurretActive, Effect.Father.TurretMode |
| Tags This Effect Has (Asset Tags) | Effect.Father.TurretMode |

#### 2.4) Compile and Save
   - 2.4.1) Click **Compile** button
   - 2.4.2) Click **Save** button

### **3) Create GE_TurretHealth**

This effect sets the father's MaxHealth and Health to turret values using native NarrativeAttributeSetBase attributes.

#### 3.1) Create Gameplay Effect Asset
   - 3.1.1) Right-click in Effects folder
   - 3.1.2) Select **Gameplay** -> **Gameplay Effect**
   - 3.1.3) Name: `GE_TurretHealth`
   - 3.1.4) Double-click to open

#### 3.2) Configure GE_TurretHealth Properties

| Property | Value |
|----------|-------|
| Duration Policy | Instant |

#### 3.3) Configure GE_TurretHealth Modifiers

| Modifier | Attribute | Modifier Op | Magnitude Type | Value |
|----------|-----------|-------------|----------------|-------|
| [0] | NarrativeAttributeSetBase.MaxHealth | Override | Scalable Float | 500.0 |
| [1] | NarrativeAttributeSetBase.Health | Override | Scalable Float | 500.0 |

#### 3.4) Compile and Save
   - 3.4.1) Click **Compile** button
   - 3.4.2) Click **Save** button

### **3A) Create GE_EngineerState (Option B Form Identity)**

This effect grants the Engineer form identity tag. Applied in the transition prelude when switching TO Engineer form.

#### 3A.1) Create Gameplay Effect Asset
   - 3A.1.1) Right-click in `/Content/FatherCompanion/Engineer/Effects/`
   - 3A.1.2) Select **Gameplay** -> **Gameplay Effect**
   - 3A.1.3) Name: `GE_EngineerState`
   - 3A.1.4) Double-click to open

#### 3A.2) Configure GE_EngineerState Properties

| Property | Value |
|----------|-------|
| Duration Policy | Infinite |

#### 3A.3) Configure GE_EngineerState Components

| Component | Configuration |
|-----------|---------------|
| Grant Tags to Target Actor | Effect.Father.FormState.Engineer |
| Tags This Effect Has (Asset Tags) | Effect.Father.FormState.Engineer |

#### 3A.4) Compile and Save
   - 3A.4.1) Click **Compile** button
   - 3A.4.2) Click **Save** button

**Note (v4.5):** GE_EngineerState grants only Effect.Father.FormState.Engineer. Per GAS Audit INV-1, NO form grants Narrative.State.Invulnerable - invulnerability was removed from the entire Father companion system.

### **4) Create GE_EngineerCooldown**

#### 4.1) Create Gameplay Effect Asset
   - 4.1.1) Right-click in Effects folder
   - 4.1.2) Select **Gameplay** -> **Gameplay Effect**
   - 4.1.3) Name: `GE_EngineerCooldown`
   - 4.1.4) Double-click to open

#### 4.2) Configure GE_EngineerCooldown Properties

| Property | Value |
|----------|-------|
| Duration Policy | Has Duration |
| Magnitude Calculation Type | Scalable Float |
| Scalable Float Value | 5.0 |

#### 4.3) Configure GE_EngineerCooldown Components

| Component | Configuration |
|-----------|---------------|
| Grant Tags to Target Actor | Cooldown.Father.Engineer |

#### 4.4) Compile and Save
   - 4.4.1) Click **Compile** button
   - 4.4.2) Click **Save** button

---

## **PHASE 3: CREATE GA_FATHERENGINEER ABILITY**

### **1) Create Abilities Folder**

#### 1.1) Navigate to Engineer Folder
   - 1.1.1) In Content Browser, go to `/Content/FatherCompanion/Engineer/`

#### 1.2) Create Abilities Subfolder
   - 1.2.1) Right-click in Engineer folder
   - 1.2.2) Select **New Folder**
   - 1.2.3) Name: `Abilities`
   - 1.2.4) Press **Enter**
   - 1.2.5) Double-click **Abilities** folder to open

### **2) Create GA_FatherEngineer Blueprint**

#### 2.1) Create Blueprint Class
   - 2.1.1) Right-click in `/Content/FatherCompanion/Engineer/Abilities/`
   - 2.1.2) Select **Blueprint Class**
   - 2.1.3) In **Pick Parent Class** dialog:
      - 2.1.3.1) Click **All Classes** dropdown at bottom
      - 2.1.3.2) In search bar, type: `NarrativeGameplayAbility`
      - 2.1.3.3) Select **NarrativeGameplayAbility** from list
      - 2.1.3.4) Click **Select** button
   - 2.1.4) Name: `GA_FatherEngineer`
   - 2.1.5) Press **Enter**
   - 2.1.6) Double-click to open

### **3) Configure Class Defaults**

#### 3.1) Open Class Defaults Panel
   - 3.1.1) Click **Class Defaults** button in Blueprint toolbar
   - 3.1.2) Details panel on right shows class properties

### **4) Configure Ability Tags (Class Defaults)**

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Engineer |
| Cancel Abilities with Tag | Ability.Father.Crawler, Ability.Father.Armor, Ability.Father.Exoskeleton |
| Activation Owned Tags | Father.State.TurretDeployed |
| Activation Required Tags | Father.State.Alive, Father.State.Recruited |
| Activation Blocked Tags | Father.State.Dormant, Father.State.Transitioning, Father.State.SymbioteLocked, Cooldown.Father.FormChange |

**Note (v4.4):** Form identity (`Effect.Father.FormState.Engineer`) is NOT in Activation Owned Tags. It persists via `GE_EngineerState` (Option B architecture). Only ephemeral state tags (`Father.State.TurretDeployed`) belong in Activation Owned Tags.

### **5) Configure Replication Settings**

#### 5.1) Set Instancing Policy
   - 5.1.1) In Details panel, find **Ability** category
   - 5.1.2) Find **Instancing Policy** dropdown
   - 5.1.3) Select: `Instanced Per Actor`

#### 5.2) Set Replication Policy
   - 5.2.1) Find **Replication Policy** dropdown
   - 5.2.2) Select: `Replicate`

#### 5.3) Set Net Execution Policy
   - 5.3.1) Find **Net Execution Policy** dropdown
   - 5.3.2) Select: `Server Only`
   - 5.3.3) NPC-owned form ability executes on server, clients see replicated results
   - 5.3.4) Required because GA_FatherEngineer spawns replicated turret actor and applies cross-actor GameplayEffects

#### 5.4) Configure Cooldown (Built-in System)
   - 5.4.1) Find **Cooldown** section in Class Defaults
   - 5.4.2) Locate **Cooldown Gameplay Effect Class** property
   - 5.4.3) Click dropdown and select: `GE_FormChangeCooldown`
   - 5.4.4) GAS automatically blocks activation when cooldown tag is present

#### 5.5) Configure Additional Replication Settings
   - 5.5.1) Find **Replicate Input Directly** checkbox
   - 5.5.2) Ensure: Unchecked
   - 5.5.3) Find **Server Respects Remote Ability Cancellation**
   - 5.5.4) Ensure: Unchecked

### **6) Configure InputTag**

#### 6.1) Set Input Tag for Form Wheel
   - 6.1.1) In Details panel, find **Narrative Ability** category
   - 6.1.2) Find **Input Tag** property
   - 6.1.3) Click dropdown or tag picker
   - 6.1.4) Search: `Narrative.Input.Father.FormChange`
   - 6.1.5) Select the tag

### **7) Compile and Save**

#### 7.1) Save Configuration
   - 7.1.1) Click **Compile** button
   - 7.1.2) Click **Save** button

---

## **PHASE 4: IMPLEMENT DEPLOYMENT POSITION LOGIC**

### **1) Create Ability Variables**

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| bIsFirstActivation | Boolean | True | No |
| MaxDeployRange | Float | 1500.0 | Yes |
| MinDeployRange | Float | 200.0 | Yes |
| TurretMaxHealth | Float | 500.0 | Yes |
| RotationArc | Float | 270.0 | Yes |
| PlayerRef | Actor (Object Reference) | None | No |
| FatherRef | BP_FatherCompanion (Object Reference) | None | No |
| DeployLocation | Vector | (0,0,0) | No |
| DeployRotation | Rotator | (0,0,0) | No |

### **2) Implement ActivateAbility Event**

#### 2.1) Add Event ActivateAbility
   - 2.1.1) Right-click in Event Graph empty space
   - 2.1.2) In context menu search: `Event ActivateAbility`
   - 2.1.3) Select **Event ActivateAbility** node

#### 2.2) Get Avatar Actor (Father)
   - 2.2.1) From **Event ActivateAbility** node:
      - 2.2.1.1) Find **Actor Info** pin (structure pin)
      - 2.2.1.2) Drag from Actor Info pin outward
      - 2.2.1.3) In context menu search: `Get Avatar Actor from Actor Info`
      - 2.2.1.4) Add **Get Avatar Actor from Actor Info** node

#### 2.3) Cast to BP_FatherCompanion
   - 2.3.1) From **Event ActivateAbility** execution pin:
      - 2.3.1.1) Drag outward into empty space
      - 2.3.1.2) Search: `Cast To BP_FatherCompanion`
      - 2.3.1.3) Add **Cast To BP_FatherCompanion** node
   - 2.3.2) Connect execution wire from Event to Cast input
   - 2.3.3) For Cast **Object** input:
      - 2.3.3.1) Connect **Return Value** from Get Avatar Actor node

#### 2.4) Handle Cast Failure
   - 2.4.1) From Cast **Cast Failed** execution pin:
      - 2.4.1.1) Drag outward and search: `End Ability`
      - 2.4.1.2) Add **End Ability** node
   - 2.4.2) On End Ability node:
      - 2.4.2.1) Find **Was Cancelled** checkbox
      - 2.4.2.2) Set: Check (true)

#### 2.5) Store Father Reference
   - 2.5.1) From Cast success execution pin:
      - 2.5.1.1) Drag outward and search: `Set FatherRef`
      - 2.5.1.2) Add **Set FatherRef** node
   - 2.5.2) Connect execution
   - 2.5.3) Connect **As BP Father Companion** to FatherRef value

#### 2.6) Get Owner Player
   - 2.6.1) From **As BP Father Companion** pin:
      - 2.6.1.1) Drag outward and search: `Get Owner Player`
      - 2.6.1.2) Add **Get Owner Player** node
      - 2.6.1.3) Connect As BP Father Companion to Target

#### 2.7) Validate Owner Player
   - 2.7.1) From Set FatherRef execution:
      - 2.7.1.1) Drag outward and search: `Is Valid`
      - 2.7.1.2) Add **Is Valid** node (object validation)
   - 2.7.2) Connect Get Owner Player **Return Value** to Is Valid input
   - 2.7.3) From Is Valid **Is Not Valid** pin:
      - 2.7.3.1) Add **End Ability** node
      - 2.7.3.2) **Was Cancelled**: Check (true)

#### 2.8) Store Player Reference
   - 2.8.1) From Is Valid **Is Valid** execution:
      - 2.8.1.1) Drag outward and search: `Set PlayerRef`
      - 2.8.1.2) Add **Set PlayerRef** node
   - 2.8.2) Connect execution
   - 2.8.3) Connect Get Owner Player **Return Value** to PlayerRef value

#### 2.9) Branch on First Activation
   - 2.9.1) From Set PlayerRef execution:
      - 2.9.1.1) Drag outward and search: `Branch`
      - 2.9.1.2) Add **Branch** node
   - 2.9.2) Drag **bIsFirstActivation** variable to graph (GET)
   - 2.9.3) Connect bIsFirstActivation to Branch **Condition** pin
   - 2.9.4) True path: First activation (initial spawn) - continues to Section 3
   - 2.9.5) False path: Form switch (transition animation) - see PHASE 5A

### **3) Set First Activation Flag (True Path - First Activation)**

#### 3.1) Set bIsFirstActivation to False
   - 3.1.1) From Branch **True** execution:
      - 3.1.1.1) Drag outward and search: `Set bIsFirstActivation`
      - 3.1.1.2) Add **Set bIsFirstActivation** node
   - 3.1.2) Set **bIsFirstActivation**: `False` (unchecked)

### **4) Calculate Deployment Position**

#### 4.1) Distance Pre-Check (Performance Optimization)
   - 4.1.1) From Set bIsFirstActivation execution:
      - 4.1.1.1) From FatherRef getter, drag outward
      - 4.1.1.2) Search: `Get Distance To`
      - 4.1.1.3) Add **Get Distance To** node
   - 4.1.2) Configure:
      - 4.1.2.1) **Target**: Connect FatherRef
      - 4.1.2.2) **Other Actor**: Connect PlayerRef
   - 4.1.3) Add **Float < Float** (Less Than) node
   - 4.1.4) Connect:
      - 4.1.4.1) Get Distance To **Return Value** to first input
      - 4.1.4.2) **MinDeployRange** getter to second input
   - 4.1.5) Add **Branch** node
   - 4.1.6) Connect Less Than result to Condition
   - 4.1.7) From Branch **True** (too close):
      - 4.1.7.1) Add **End Ability** node
      - 4.1.7.2) **Was Cancelled**: Check (true)

#### 4.2) Get Player Controller
   - 3.2.1) From Branch **False** execution (distance OK):
      - 3.2.1.1) Drag outward and search: `Get Player Controller`
      - 3.2.1.2) Add **Get Player Controller** node
   - 3.2.2) Set **Player Index**: `0`

#### 3.3) Get Camera Location and Rotation
   - 3.3.1) From Player Controller Return Value:
      - 3.3.1.1) Drag outward and search: `Get Player View Point`
      - 3.3.1.2) Add **Get Player View Point** node

#### 3.4) Calculate Trace End Point
   - 3.4.1) From Get Player View Point **Rotation** output:
      - 3.4.1.1) Drag outward and search: `Get Forward Vector`
      - 3.4.1.2) Add **Get Forward Vector** node
   - 3.4.2) Add **Vector * Float** (Multiply) node
   - 3.4.3) Connect:
      - 3.4.3.1) Forward Vector to first input
      - 3.4.3.2) **MaxDeployRange** getter to float input
   - 3.4.4) Add **Vector + Vector** (Add) node
   - 3.4.5) Connect:
      - 3.4.5.1) View Point **Location** to first input
      - 3.4.5.2) Multiply result to second input

#### 3.5) Perform Line Trace
   - 3.5.1) From Get Player Controller execution (continue chain):
      - 3.5.1.1) Drag outward and search: `Line Trace By Channel`
      - 3.5.1.2) Add **Line Trace By Channel** node
   - 3.5.2) Connect execution
   - 3.5.3) Configure Line Trace:
      - 3.5.3.1) **Start**: Connect View Point **Location**
      - 3.5.3.2) **End**: Connect calculated Trace End Location (Add result)
      - 3.5.3.3) **Trace Channel**: `Visibility`
      - 3.5.3.4) **Trace Complex**: Uncheck
      - 3.5.3.5) **Actors to Ignore**: Create array with PlayerRef and FatherRef
      - 3.5.3.6) **Draw Debug Type**: `None`

#### 3.6) Check Trace Hit
   - 3.6.1) From Line Trace **Return Value** (boolean):
      - 3.5.1.1) Drag outward and add **Branch** node
   - 3.5.2) Connect Line Trace execution to Branch

### **4) Handle Trace Result**

#### 4.1) Extract Hit Location (Hit Success)
   - 4.1.1) From Line Trace **Out Hit** pin:
      - 4.1.1.1) Drag outward and search: `Break Hit Result`
      - 4.1.1.2) Add **Break Hit Result** node

#### 4.2) Set Deploy Location (Hit)
   - 4.2.1) From Branch **True** execution:
      - 4.2.1.1) Drag outward and search: `Set DeployLocation`
      - 4.2.1.2) Add **Set DeployLocation** node
   - 4.2.2) Connect execution
   - 4.2.3) Connect Break Hit Result **Location** to DeployLocation value

#### 4.3) Set Deploy Location (No Hit - Use Max Range)
   - 4.3.1) From Branch **False** execution:
      - 4.3.1.1) Drag outward and search: `Set DeployLocation`
      - 4.3.1.2) Add **Set DeployLocation** node
   - 4.3.2) Connect execution
   - 4.3.3) Connect calculated Trace End Location (Add result) to DeployLocation value

#### 4.4) Merge Execution Paths
   - 4.4.1) Add **Sequence** node after both Set DeployLocation nodes
   - 4.4.2) Connect True path Set DeployLocation execution to Sequence input
   - 4.4.3) Connect False path Set DeployLocation execution to Sequence input
   - 4.4.4) Continue from Sequence **Then 0** pin

### **5) Calculate Deploy Rotation**

#### 5.1) Calculate Rotation Facing Player
   - 5.1.1) From Sequence **Then 0** execution:
      - 5.1.1.1) Drag outward and search: `Find Look at Rotation`
      - 5.1.1.2) Add **Find Look at Rotation** node
   - 5.1.2) Configure:
      - 5.1.2.1) **Start**: Connect **DeployLocation** getter
      - 5.1.2.2) **Target**: Connect **PlayerRef** -> Get Actor Location

#### 5.2) Store Deploy Rotation
   - 5.2.1) From Find Look at Rotation execution:
      - 5.2.1.1) Add **Set DeployRotation** node
   - 5.2.2) Connect execution
   - 5.2.3) Connect Find Look at Rotation **Return Value** to DeployRotation value

### **6) Compile and Save**

#### 6.1) Save Progress
   - 6.1.1) Click **Compile** button
   - 6.1.2) Click **Save** button

---

## **PHASE 5: IMPLEMENT TURRET SETUP LOGIC**

### **1) Play Deployment Animation**

#### 1.1) Add Play Montage Node
   - 1.1.1) From Set DeployRotation execution:
      - 1.1.1.1) From **FatherRef** getter, drag outward
      - 1.1.1.2) Search: `Play Anim Montage`
      - 1.1.1.3) Add **Play Anim Montage** node
   - 1.1.2) Connect execution
   - 1.1.3) Configure:
      - 1.1.3.1) **Target**: Connect FatherRef
      - 1.1.3.2) **New Anim Montage**: Select `AM_FatherDeploy` (placeholder - create later)
      - 1.1.3.3) **In Play Rate**: `1.0`

### **2) Teleport Father to Deploy Location**

#### 2.1) Set Actor Location and Rotation
   - 2.1.1) From Play Anim Montage execution (or Set DeployRotation if skipping animation):
      - 2.1.1.1) From **FatherRef** getter, drag outward
      - 2.1.1.2) Search: `Set Actor Location and Rotation`
      - 2.1.1.3) Add **Set Actor Location and Rotation** node
   - 2.1.2) Connect execution
   - 2.1.3) Configure:
      - 2.1.3.1) **Target**: Connect FatherRef
      - 2.1.3.2) **New Location**: Connect **DeployLocation** getter
      - 2.1.3.3) **New Rotation**: Connect **DeployRotation** getter
      - 2.1.3.4) **Sweep**: Uncheck
      - 2.1.3.5) **Teleport**: Check

### **3) Disable Father Movement**

#### 3.1) Get Character Movement Component
   - 3.1.1) From **FatherRef** getter:
      - 3.1.1.1) Drag outward and search: `Get Character Movement`
      - 3.1.1.2) Add **Get Character Movement** node

#### 3.2) Disable Movement
   - 3.2.1) From Set Actor Location execution:
      - 3.2.1.1) From Character Movement Return Value, drag outward
      - 3.2.1.2) Search: `Disable Movement`
      - 3.2.1.3) Add **Disable Movement** node
   - 3.2.2) Connect execution

### **4) Set Father Form State**

#### 4.1) Set Current Form
   - 4.1.1) From Disable Movement execution:
      - 4.1.1.1) From FatherRef, drag outward
      - 4.1.1.2) Search: `Set Current Form`
      - 4.1.1.3) Add **Set Current Form** node
   - 4.1.2) Connect execution
   - 4.1.3) Set **Current Form**: `Engineer` (from EFatherForm enum)

#### 4.2) Set Is Deployed Flag
   - 4.2.1) From Set Current Form execution:
      - 4.2.1.1) From FatherRef, drag outward
      - 4.2.1.2) Search: `Set Is Deployed`
      - 4.2.1.3) Add **Set Is Deployed** node
   - 4.2.2) Connect execution
   - 4.2.3) Set **Is Deployed**: Check (true)

#### 4.3) Store Deployed Location
   - 4.3.1) From Set Is Deployed execution:
      - 4.3.1.1) From FatherRef, drag outward
      - 4.3.1.2) Search: `Set Deployed Location`
      - 4.3.1.3) Add **Set Deployed Location** node
   - 4.3.2) Connect execution
   - 4.3.3) Connect **DeployLocation** getter to value

### **5) Initialize Turret Health**

#### 5.1) Set Turret Health
   - 5.1.1) From Set Deployed Location execution:
      - 5.1.1.1) From FatherRef, drag outward
      - 5.1.1.2) Search: `Set Turret Health`
      - 5.1.1.3) Add **Set Turret Health** node
   - 5.1.2) Connect execution
   - 5.1.3) Connect **TurretMaxHealth** getter to value

#### 5.2) Set Max Turret Health
   - 5.2.1) From Set Turret Health execution:
      - 5.2.1.1) From FatherRef, drag outward
      - 5.2.1.2) Search: `Set Max Turret Health`
      - 5.2.1.3) Add **Set Max Turret Health** node
   - 5.2.2) Connect execution
   - 5.2.3) Connect **TurretMaxHealth** getter to value

### **6) Apply Turret Mode Effect**

#### 6.1) Get Father ASC
   - 6.1.1) From **FatherRef** getter:
      - 6.1.1.1) Drag outward and search: `Get Ability System Component`
      - 6.1.1.2) Add **Get Ability System Component** node

#### 6.2) Apply GE_TurretMode
   - 6.2.1) From Set Max Turret Health execution:
      - 6.2.1.1) From Father ASC Return Value, drag outward
      - 6.2.1.2) Search: `Apply Gameplay Effect to Self`
      - 6.2.1.3) Add **Apply Gameplay Effect to Self** node
   - 6.2.2) Connect execution
   - 6.2.3) Configure:
      - 6.2.3.1) **Gameplay Effect Class**: `GE_TurretMode`
      - 6.2.3.2) **Level**: `1.0`

#### 6.3) Store Effect Handle in BP_FatherCompanion
   - 6.3.1) From Apply GE execution:
      - 6.3.1.1) From **FatherRef** getter, drag outward
      - 6.3.1.2) Search: `Set Turret Mode Effect Handle`
      - 6.3.1.3) Add **Set Turret Mode Effect Handle** node
   - 6.3.2) Connect execution
   - 6.3.3) Connect Apply GE **Return Value** to TurretModeEffectHandle value
   - 6.3.4) Handle stored in BP_FatherCompanion for EndAbility cleanup when form changes

### **7) Initialize Turret AI**

#### 7.1) Run Behavior Tree
   - 7.1.1) From Set TurretModeEffectHandle execution:
      - 7.1.1.1) From FatherRef, drag outward
      - 7.1.1.2) Search: `Get Controller`
      - 7.1.1.3) Add **Get Controller** node
   - 7.1.2) From Get Controller Return Value:
      - 7.1.2.1) Drag outward and search: `Cast To AIController`
      - 7.1.2.2) Add **Cast To AIController** node
   - 7.1.3) From Cast success:
      - 7.1.3.1) Drag outward and search: `Run Behavior Tree`
      - 7.1.3.2) Add **Run Behavior Tree** node
   - 7.1.4) Configure:
      - 7.1.4.1) **BTAsset**: Select `BT_FatherEngineer` (created in GA_TurretShoot guide)

### **8) Set State Variables**

#### 8.1) Set Current Form
   - 8.1.1) From Run Behavior Tree execution:
      - 8.1.1.1) From FatherRef, drag outward
      - 8.1.1.2) Search: `Set Current Form`
      - 8.1.1.3) Add **Set Current Form** node
   - 8.1.2) Connect execution
   - 8.1.3) **Current Form**: Select `Engineer`

#### 8.2) Set Is Deployed
   - 8.2.1) From Set Current Form execution:
      - 8.2.1.1) From FatherRef, drag outward
      - 8.2.1.2) Search: `Set Is Deployed`
      - 8.2.1.3) Add **Set Is Deployed** node
   - 8.2.2) Connect execution
   - 8.2.3) **Is Deployed**: Check (true)

### **9) Mark First Activation Complete**

#### 9.1) Set bIsFirstActivation to False
   - 9.1.1) From Set Is Deployed execution:
      - 9.1.1.1) Drag outward and search: `Set bIsFirstActivation`
      - 9.1.1.2) Add **Set bIsFirstActivation** node
   - 9.1.2) Connect execution
   - 9.1.3) **bIsFirstActivation**: Uncheck (false)
   - 9.1.4) Marks ability as having been activated before

### **10) End Ability (Initial Spawn)**

#### 10.1) Call End Ability
   - 10.1.1) From Set bIsFirstActivation execution:
      - 10.1.1.1) Drag outward and search: `End Ability`
      - 10.1.1.2) Add **End Ability** node
   - 10.1.2) Connect execution
   - 10.1.3) **Was Cancelled**: Uncheck (false) - normal completion
   - 10.1.4) Form tags persist via Activation Owned Tags (ability stays active)

### **11) Compile and Save**

#### 11.1) Save All Changes
   - 11.1.1) Click **Compile** button
   - 11.1.2) Click **Save** button

---

## **PHASE 5A: FORM SWITCH WITH TRANSITION ANIMATION (FALSE PATH)**

This phase handles switching TO Engineer form from another form via the T wheel. The bIsFirstActivation Branch False path routes here.

**CRITICAL (Option B):** The transition prelude MUST:
1. Remove ALL prior form state GEs (Single Active Form State Invariant)
2. Apply GE_EngineerState (grants Effect.Father.FormState.Engineer)

### **1) TRANSITION PRELUDE: Remove Prior Form State (Option B)**

#### 1.1) Get Father Ability System Component
   - 1.1.1) From Branch **False** execution pin (Section 2.9)
   - 1.1.2) Drag from **FatherRef** (or **As BP Father Companion**)
   - 1.1.3) Search: `Get Ability System Component`
   - 1.1.4) Add node

#### 1.2) Remove All Prior Form State GEs
   - 1.2.1) Drag from **Get Ability System Component** execution pin
   - 1.2.2) Search: `BP_RemoveGameplayEffectFromOwnerWithGrantedTags`
   - 1.2.3) Add node
   - 1.2.4) Connect Father ASC to **Target**
   - 1.2.5) In **Tags** field, add: `Effect.Father.FormState` (parent tag removes ALL form states)
   - 1.2.6) **Purpose:** Enforces Single Active Form State Invariant - exactly one form state at a time

### **2) TRANSITION PRELUDE: Apply New Form State (Option B)**

#### 2.1) Apply GE_EngineerState
   - 2.1.1) Drag from **BP_RemoveGameplayEffectFromOwnerWithGrantedTags** execution pin
   - 2.1.2) Search: `Apply Gameplay Effect to Self`
   - 2.1.3) Add node
   - 2.1.4) Connect Father ASC to **Target**
   - 2.1.5) Set **Gameplay Effect Class**: `GE_EngineerState`
   - 2.1.6) **Result:** Father now has Effect.Father.FormState.Engineer tag

### **3) Add Transitioning Tag**

> **v4.5 (GAS Audit INV-1):** Form transitions NO LONGER grant invulnerability.
> Father is vulnerable during the 5s transition. Use AddLooseGameplayTag for the transitioning state.

#### 3.1) Add Loose Gameplay Tag (Father.State.Transitioning)
   - 3.1.1) Drag from **Apply GE_EngineerState** execution pin
   - 3.1.2) Search: `Add Loose Gameplay Tag`
   - 3.1.3) Add node
   - 3.1.4) Connect Father ASC to **Target**
   - 3.1.5) Set **Tag**: `Father.State.Transitioning`
   - 3.1.6) **Note:** This blocks form switch during VFX (no invulnerability)

### **4) Spawn Transition VFX**

#### 4.1) Add Spawn System Attached
   - 4.1.1) From Add Loose Gameplay Tag execution:
      - 4.1.1.1) Drag outward and search: `Spawn System Attached`
      - 4.1.1.2) Add **Spawn System Attached** node

#### 4.2) Configure VFX
   - 4.2.1) **System Template**: `NS_FatherFormTransition`
   - 4.2.2) For **Attach to Component**:
      - 4.2.2.1) From FatherRef, drag and add **Get Mesh** node
      - 4.2.2.2) Connect Get Mesh Return Value to Attach to Component
   - 4.2.3) **Attach Point Name**: `root`
   - 4.2.4) **Location Type**: `Snap to Target`

### **5) Wait 5 Seconds**

#### 5.1) Add Delay Node
   - 5.1.1) From Spawn System Attached execution:
      - 5.1.1.1) Drag outward and search: `Delay`
      - 5.1.1.2) Add **Delay** node
   - 5.1.2) **Duration**: `5.0`

### **6) POST-DELAY 3-LAYER GUARDS (NL-GUARD-IDENTITY L1)**

> **GAS Audit Compliance (v5.0):** These guards execute IMMEDIATELY after the Delay callback returns.
> They validate that the ability context remains valid before any state-modifying operations.
> Pattern locked as NL-GUARD-IDENTITY L1 per Father_Companion_GAS_Abilities_Audit.md v5.0.
>
> **CRITICAL:** Tags/effects in ASC are the canonical state (GAS truth source), not external enums.

#### 6.1) Guard 1: Validate FatherRef

##### 6.1.1) Add Is Valid Node
   - 6.1.1.1) From **Delay** -> **Completed** execution pin
   - 6.1.1.2) Drag wire to right and release
   - 6.1.1.3) Search: `Is Valid`
   - 6.1.1.4) Select **Utilities > Is Valid** macro node
   - 6.1.1.5) Position to the right of Delay node

##### 6.1.2) Connect Input Object
   - 6.1.2.1) From **FatherRef** variable (stored in Section 2.5)
   - 6.1.2.2) Drag wire to **Is Valid** -> **Input Object** pin
   - 6.1.2.3) Release to connect

##### 6.1.3) Add Branch Node
   - 6.1.3.1) Drag from **Is Valid** -> **Return Value** execution pin
   - 6.1.3.2) Search: `Branch`
   - 6.1.3.3) Select **Branch** node
   - 6.1.3.4) Connect **Is Valid** -> **Return Value** to **Branch** -> **Condition**

##### 6.1.4) Handle Invalid Path
   - 6.1.4.1) From **Branch** -> **False** execution pin
   - 6.1.4.2) Leave unconnected or optionally add **Return Node**
   - 6.1.4.3) **Purpose:** Father destroyed during transition - abort silently

#### 6.2) Guard 2: Check Transitioning Phase Tag (Father.State.Transitioning)

> **Phase Check:** Validates we're still in a valid transition period. This tag was added
> at the start of the transition and will be removed when transition completes.

##### 6.2.1) Get Father Ability System Component
   - 6.2.1.1) From **Branch** -> **True** execution pin (valid path)
   - 6.2.1.2) Drag from **FatherRef**
   - 6.2.1.3) Search: `Get Ability System Component`
   - 6.2.1.4) Select node

##### 6.2.2) Add Has Matching Gameplay Tag Check
   - 6.2.2.1) From **Get Ability System Component** -> **Return Value**
   - 6.2.2.2) Drag wire to empty space
   - 6.2.2.3) Search: `Has Matching Gameplay Tag`
   - 6.2.2.4) Select **Has Matching Gameplay Tag** function

##### 6.2.3) Configure Tag to Check
   - 6.2.3.1) On **Has Matching Gameplay Tag** node, find **Tag to Check** parameter
   - 6.2.3.2) Click dropdown and navigate to: `Father > State > Transitioning`
   - 6.2.3.3) Select **Father.State.Transitioning** tag
   - 6.2.3.4) **Rationale:** Phase check - if tag missing, transition was interrupted

##### 6.2.4) Add Branch Node
   - 6.2.4.1) Drag wire from execution flow
   - 6.2.4.2) Search: `Branch`
   - 6.2.4.3) Select **Branch** node
   - 6.2.4.4) Connect **Has Matching Gameplay Tag** -> **Return Value** to **Branch** -> **Condition**

##### 6.2.5) Handle Phase Interrupted
   - 6.2.5.1) From **Branch** -> **False** execution pin
   - 6.2.5.2) Leave unconnected or optionally add **Return Node**
   - 6.2.5.3) **Purpose:** Transition was interrupted (tag removed externally) - abort

#### 6.3) Guard 3: Check Form Identity Tag (Effect.Father.FormState)

> **Identity Check (NL-GUARD-IDENTITY L1):** Validates form identity GE is still active.
> Uses PARENT tag `Effect.Father.FormState` (not form-specific) per locked decision.
> Parent tag match confirms ANY form state GE is present - indicates valid ability context.

##### 6.3.1) Add Has Matching Gameplay Tag Check
   - 6.3.1.1) From **Guard 2 Branch** -> **True** execution pin (phase check passed)
   - 6.3.1.2) From **Get Ability System Component** -> **Return Value** (reuse from Guard 2)
   - 6.3.1.3) Drag wire to empty space
   - 6.3.1.4) Search: `Has Matching Gameplay Tag`
   - 6.3.1.5) Select **Has Matching Gameplay Tag** function

##### 6.3.2) Configure Tag to Check
   - 6.3.2.1) On **Has Matching Gameplay Tag** node, find **Tag to Check** parameter
   - 6.3.2.2) Click dropdown and navigate to: `Effect > Father > FormState`
   - 6.3.2.3) Select **Effect.Father.FormState** tag (PARENT tag, not form-specific)
   - 6.3.2.4) **Rationale:** Identity check - parent tag confirms form state GE is active

##### 6.3.3) Add Branch Node
   - 6.3.3.1) Drag wire from execution flow
   - 6.3.3.2) Search: `Branch`
   - 6.3.3.3) Select **Branch** node
   - 6.3.3.4) Connect **Has Matching Gameplay Tag** -> **Return Value** to **Branch** -> **Condition**

##### 6.3.4) Handle Identity Missing
   - 6.3.4.1) From **Branch** -> **False** execution pin
   - 6.3.4.2) Leave unconnected or optionally add **Return Node**
   - 6.3.4.3) **Purpose:** Form identity GE was removed - ability context invalid, abort

##### 6.3.5) Continue to State Operations
   - 6.3.5.1) From **Branch** -> **True** execution pin
   - 6.3.5.2) Continue to Section 7 (Detach Father From Player)
   - 6.3.5.3) **All guards passed - safe to modify state**

##### 6.3.3) Configure Tag to Check
   - 6.3.3.1) On **Has Matching Gameplay Tag** node, find **Tag to Check** parameter
   - 6.3.3.2) Click dropdown and navigate to: `Father > State > TurretDeployed`
   - 6.3.3.3) Select **Father.State.TurretDeployed** tag
   - 6.3.3.4) **Rationale:** This is the Activation Owned Tag - if present, ability is still active

##### 6.3.4) Add Branch Node
   - 6.3.4.1) From **Get Ability System Component** execution pin
   - 6.3.4.2) Drag wire to right and release
   - 6.3.4.3) Search: `Branch`
   - 6.3.4.4) Select **Branch** node
   - 6.3.4.5) Connect **Has Matching Gameplay Tag** -> **Return Value** to **Branch** -> **Condition**

##### 6.3.5) Handle Ability No Longer Active
   - 6.3.5.1) From **Branch** -> **False** execution pin
   - 6.3.5.2) Leave unconnected or optionally add **Return Node**
   - 6.3.5.3) **Purpose:** Ability was cancelled during transition - abort remaining setup

##### 6.3.6) Continue to State Operations
   - 6.3.6.1) From **Branch** -> **True** execution pin
   - 6.3.6.2) Continue to Section 7 (Detach Father From Previous Form)
   - 6.3.6.3) **All guards passed - safe to modify state**

### **7) Detach Father From Previous Form**

#### 7.1) Add Detach From Actor
   - 7.1.1) From **Guard 3 Branch** -> **True** execution (all guards passed):
      - 7.1.1.1) Drag outward and search: `Detach From Actor`
      - 7.1.1.2) Add **Detach From Actor** node
   - 7.1.2) Connect **FatherRef** to Target

#### 7.2) Configure Detach Rules
   - 7.2.1) **Location Rule**: `Keep World`
   - 7.2.2) **Rotation Rule**: `Keep World`
   - 7.2.3) **Scale Rule**: `Keep World`

### **8) Execute Deployment Logic**

#### 8.1) Merge to Main Deployment Flow
   - 8.1.1) From Detach From Actor execution:
   - 8.1.2) Connect to same deployment flow as initial spawn (PHASE 4 Section 4 onward)
   - 8.1.3) Calculate deploy position, move father, apply turret mode, etc.
   - 8.1.4) Deployment logic is identical for both initial spawn and form switch

### **9) Remove Transitioning State**

#### 9.1) Remove Loose Gameplay Tag
   - 9.1.1) After Set Is Deployed execution (before End Ability):
      - 9.1.1.1) Drag outward and search: `Remove Loose Gameplay Tag`
      - 9.1.1.2) Add **Remove Loose Gameplay Tag** node
   - 9.1.2) Connect ASC to Target
   - 9.1.3) **Gameplay Tag**: `Father.State.Transitioning`

### **10) Apply Form Cooldown**

> **v4.5 (GAS Audit INV-1):** No GE removal needed - Father.State.Transitioning tag was removed in Step 9 via RemoveLooseGameplayTag.

#### 10.1) Commit Ability Cooldown
   - 10.1.1) From Remove Loose Gameplay Tag execution:
      - 10.1.1.1) Drag outward and search: `Commit Ability Cooldown`
      - 10.1.1.2) Select **Commit Ability Cooldown** node
   - 10.1.2) No parameters needed - uses CooldownGameplayEffectClass automatically

### **11) End Ability (Form Switch)**

#### 11.1) Call End Ability
   - 11.1.1) From Commit Ability Cooldown execution:
      - 11.1.1.1) Drag outward and search: `End Ability`
      - 11.1.1.2) Add **End Ability** node
   - 11.1.2) **Was Cancelled**: Uncheck (false)

---

## **PHASE 5B: ENDABILITY CLEANUP**

State tag (`Father.State.TurretDeployed`) is automatically removed when ability ends via Activation Owned Tags. Form identity tag (`Effect.Father.FormState.Engineer`) persists via GE_EngineerState until the next form's transition prelude removes it. This section handles GE_TurretMode cleanup.

### **1) Add Event OnEndAbility**

#### 1.1) Create Event Node
   - 1.1.1) Right-click in Event Graph
   - 1.1.2) Search: `Event OnEndAbility`
   - 1.1.3) Select node

### **1A) CRITICAL: Check bWasCancelled Before Cleanup (v2.5.5)**

**IMPORTANT:** The EndAbility event fires in TWO scenarios:
1. **bWasCancelled = false**: After activation flow calls EndAbility (form is now active)
2. **bWasCancelled = true**: When cancelled by another form's Cancel Abilities With Tag

Cleanup should ONLY run when bWasCancelled = true (form switch in progress).

#### 1A.1) Add Branch on bWasCancelled
   - 1A.1.1) From Event OnEndAbility execution pin
   - 1A.1.2) Drag wire to right and release
   - 1A.1.3) Search: `Branch`
   - 1A.1.4) Select **Branch** node

#### 1A.2) Connect Condition
   - 1A.2.1) From Event OnEndAbility **bWasCancelled** pin (boolean output)
   - 1A.2.2) Drag wire to Branch **Condition** input pin
   - 1A.2.3) Release to connect

#### 1A.3) Configure Branch Paths
   - 1A.3.1) FALSE path: No connection needed - execution ends (form is now active, keep state)
   - 1A.3.2) TRUE path: Continue to Section 2 (Get Father Reference)

### **2) Get Father Reference**

#### 2.1) Add Get Avatar Actor From Actor Info
   - 2.1.1) From **Branch TRUE** execution pin (from Section 1A.3.2)
   - 2.1.2) Drag to empty space
   - 2.1.3) Search: `Get Avatar Actor From Actor Info`
   - 2.1.4) Select node
   - 2.1.5) Connect **Actor Info** from Event OnEndAbility to input

#### 2.2) Add Cast to BP_FatherCompanion
   - 2.2.1) Drag from **Get Avatar Actor** -> **Return Value**
   - 2.2.2) Search: `Cast To BP_FatherCompanion`
   - 2.2.3) Select node

### **3) Remove TurretModeEffectHandle**

#### 3.1) Get TurretModeEffectHandle
   - 3.1.1) Drag from **Cast** -> **As BP Father Companion** pin
   - 3.1.2) Search: `Get Turret Mode Effect Handle`
   - 3.1.3) Select getter node

#### 3.2) Validate TurretModeEffectHandle
   - 3.2.1) Drag from **Get Turret Mode Effect Handle** -> **Return Value**
   - 3.2.2) Search: `Is Valid`
   - 3.2.3) Select **Is Valid (Active Gameplay Effect Handle)** function

#### 3.3) Add Branch Node
   - 3.3.1) Drag from **Cast** success execution pin
   - 3.3.2) Search: `Branch`
   - 3.3.3) Select node
   - 3.3.4) Connect **Is Valid** -> **Return Value** to **Condition**

#### 3.4) Remove GE_TurretMode Using Handle
   - 3.4.1) Drag from **Branch** -> **True** execution pin
   - 3.4.2) From **As BP Father Companion**, search: `Get Ability System Component`
   - 3.4.3) Add **Get Ability System Component** node
   - 3.4.4) Drag from Get ASC execution:
      - 3.4.4.1) Search: `Remove Active Gameplay Effect`
      - 3.4.4.2) Add **Remove Active Gameplay Effect** node
   - 3.4.5) Connect **Get Ability System Component** -> **Return Value** to **Target**
   - 3.4.6) Connect **Get Turret Mode Effect Handle** -> **Return Value** to **Handle** input
   - 3.4.7) **Stacks To Remove**: `-1` (remove all stacks)

### **4) Reset State Variables**

#### 4.1) Set Is Deployed to False
   - 4.1.1) From Branch (both True and False paths):
      - 4.1.1.1) Add **Sequence** node to merge paths
   - 4.1.2) From Sequence Then 0:
      - 4.1.2.1) From **As BP Father Companion**, search: `Set Is Deployed`
      - 4.1.2.2) Add **Set Is Deployed** node
   - 4.1.3) **Is Deployed**: Uncheck (false)

### **5) Verify EndAbility Cleanup Flow**

#### 5.1) Check Execution Flow

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event OnEndAbility | Fires when ability ends |
| 2 | **Branch (bWasCancelled)** | **CRITICAL: Only cleanup when cancelled (v2.5.5)** |
| 3 | FALSE path | Skip cleanup - form is now active |
| 4 | TRUE path → Continue | Form switch in progress - do cleanup |
| 5 | Get Avatar Actor From Actor Info | Get father reference |
| 6 | Cast to BP_FatherCompanion | Type-safe reference |
| 7 | Get Turret Mode Effect Handle | Retrieve TurretMode handle |
| 8 | Is Valid (TurretMode) | Check handle validity |
| 9 | Branch | Only proceed if valid |
| 10 | Remove Active Gameplay Effect | Remove GE_TurretMode using handle |
| 11 | Set Is Deployed (false) | Reset deployment state |
| 12 | Form Tags Auto-Removed | Activation Owned Tags cleanup (automatic) |

### **7) Compile and Save**

#### 7.1) Compile Blueprint
   - 7.1.1) Click **Compile** button

#### 7.2) Save Blueprint
   - 7.2.1) Click **Save** button

---

## **PHASE 6: CONFIGURE ABILITY GRANTING**

### **1) Open BP_FatherCompanion**

#### 1.1) Navigate to Father Blueprint
   - 1.1.1) In Content Browser, go to `/Content/FatherCompanion/Characters/`
   - 1.1.2) Locate **BP_FatherCompanion**
   - 1.1.3) Double-click to open

### **2) Add Engineer Form Variables**

#### 2.1) Create IsDeployed Variable
   - 2.1.1) In My Blueprint, click **+ (Plus)** next to Variables
   - 2.1.2) Name: `IsDeployed`
   - 2.1.3) **Variable Type**: `Boolean`
   - 2.1.4) Set **Default Value**: Uncheck (false)
   - 2.1.5) Enable **Replication**: Replicated
   - 2.1.6) Enable **SaveGame**: Check (in Advanced section)

#### 2.2) Create DeployedTransform Variable
   - 2.2.1) Click **+ (Plus)** next to Variables
   - 2.2.2) Name: `DeployedTransform`
   - 2.2.3) **Variable Type**: `Transform`
   - 2.2.4) Enable **Replication**: Replicated
   - 2.2.5) Enable **SaveGame**: Check (in Advanced section)

#### 2.3) Create TurretModeEffectHandle Variable
   - 2.3.1) Click **+ (Plus)** next to Variables
   - 2.3.2) Name: `TurretModeEffectHandle`
   - 2.3.3) **Variable Type**: Search `Active Gameplay Effect Handle`
   - 2.3.4) Select: `Active Gameplay Effect Handle` (structure type)
   - 2.3.5) Variable stored here for EndAbility cleanup when switching forms

### **3) Update EFatherForm Enum**

#### 3.1) Open EFatherForm Enum
   - 3.1.1) In Content Browser, locate EFatherForm enumeration asset
   - 3.1.2) Double-click to open

#### 3.2) Add Engineer Entry
   - 3.2.1) Click **Add Enumerator** button
   - 3.2.2) Name: `Engineer`
   - 3.2.3) Description: `Stationary turret form`
   - 3.2.4) Save enum

### **4) Add to AbilityConfiguration**

#### 4.1) Open AC_FatherCompanion_Default
   - 4.1.1) In Content Browser, navigate to `/Content/FatherCompanion/Configurations/`
   - 4.1.2) Locate **AC_FatherCompanion_Default**
   - 4.1.3) Double-click to open

#### 4.2) Add GA_FatherEngineer to Default Abilities
   - 4.2.1) Find **Default Abilities** array
   - 4.2.2) Click **+ (Plus)** button
   - 4.2.3) New element appears
   - 4.2.4) Click dropdown on new element
   - 4.2.5) Search: `GA_FatherEngineer`
   - 4.2.6) Select: **GA_FatherEngineer**

#### 4.3) Save Configuration
   - 4.3.1) Click **Save** button

### **5) Compile and Save Father**

#### 5.1) Compile Blueprint
   - 5.1.1) Click **Compile** button

#### 5.2) Save Blueprint
   - 5.2.1) Click **Save** button

---

## **PHASE 7: INPUT CONFIGURATION**

### **1) Verify Form Wheel Integration**

#### 1.1) Open Input Mapping Asset
   - 1.1.1) Navigate to your Input Mapping Context asset
   - 1.1.2) Usually located in `/Content/Input/` or `/Content/Settings/`
   - 1.1.3) Double-click to open

#### 1.2) Verify Form Change Input Action
   - 1.2.1) Ensure **IA_FatherFormChange** input action exists
   - 1.2.2) Should be bound to **T** key
   - 1.2.3) This triggers form wheel display

### **2) Configure Form Wheel UI**

#### 2.1) Open Form Wheel Widget
   - 2.1.1) Navigate to your form wheel widget blueprint
   - 2.1.2) Usually: `/Content/FatherCompanion/UI/WBP_FormWheel`

#### 2.2) Add Engineer Form Option
   - 2.2.1) In widget designer, locate form selection area
   - 2.2.2) Add new form option for Engineer
   - 2.2.3) Configure:
      - 2.2.3.1) **Form Name**: `Engineer`
      - 2.2.3.2) **Form Icon**: Turret/defense icon
      - 2.2.3.3) **Form Description**: `Deploy as stationary turret`
      - 2.2.3.4) **Ability Class**: `GA_FatherEngineer`

### **3) Configure Input Tag**

#### 3.1) Open DA_NarrativeInputSettings
   - 3.1.1) Navigate to `/Content/Data/` or Narrative Pro settings folder
   - 3.1.2) Locate **DA_NarrativeInputSettings** or equivalent

#### 3.2) Verify FormChange Tag Mapping
   - 3.2.1) Ensure `Narrative.Input.Father.FormChange` is mapped
   - 3.2.2) Should link to T key / form wheel system

### **4) Save All Assets**

#### 4.1) Save Input Configuration
   - 4.1.1) Save all modified input assets
   - 4.1.2) Save widget blueprints
   - 4.1.3) Save data assets

---

## **CHANGELOG**

| Version | Date | Changes |
|---------|------|---------|
| 4.9 | January 2026 | **NL-GUARD-IDENTITY L1 (Claude-GPT Audit v5.0 - 2026-01-24):** Updated 3-layer guards to use LOCKED L1 pattern: (1) IsValid(FatherRef), (2) HasMatchingGameplayTag(Father.State.Transitioning) - phase check, (3) HasMatchingGameplayTag(Effect.Father.FormState) - identity check with PARENT tag. Removed enum-based Guard 2, now uses tag-based phase/identity checks per GAS truth source principle. |
| 4.8 | January 2026 | **3-Layer Guards (Claude-GPT Audit - 2026-01-24):** Added PHASE 5A Section 6 (POST-DELAY 3-LAYER GUARDS) implementing gold standard pattern from GA_FatherSymbiote. Guards execute after Delay callback: (1) IsValid(FatherRef), (2) CurrentForm == Engineer, (3) HasMatchingGameplayTag(Father.State.TurretDeployed). Resolves MEDIUM severity audit finding "No post-delay guards". Renumbered subsequent sections 7-11. |
| 4.7 | January 2026 | **C_SYMBIOTE_STRICT_CANCEL Contract (Claude-GPT Audit - 2026-01-23):** Removed `Ability.Father.Symbiote` from cancel_abilities_with_tag. Symbiote is an ultimate ability (30s duration) that cannot be cancelled by player-initiated form changes. Defense-in-depth: Layer 1 blocks via `Father.State.SymbioteLocked` in activation_blocked_tags, Layer 2 ensures no cancel path exists. See LOCKED_CONTRACTS.md Contract 11. |
| 4.6 | January 2026 | **Locked Decisions Reference:** Added Father_Companion_GAS_Abilities_Audit.md reference. This guide complies with: INV-1 (no transition invulnerability), Rule 4 (First Activation path merges into setup chain). Updated Technical Reference to v6.2. |
| 4.5 | January 2026 | **GAS Audit INV-1 Compliance:** REMOVED GE_TransitionInvulnerability - transitions no longer grant invulnerability. Form transitions now use AddLooseGameplayTag(Father.State.Transitioning) directly without damage immunity. Updated Quick Reference and Gameplay Effect Summary. Only GA_FatherSacrifice grants invulnerability in the entire Father system. |
| 4.4 | January 2026 | **Option B Form State Architecture:** Complete rewrite for GE-based form identity. Added GE_EngineerState (Infinite, grants Effect.Father.FormState.Engineer). Added transition prelude to PHASE 5A: remove prior form state GE via BP_RemoveGameplayEffectFromOwnerWithGrantedTags(Effect.Father.FormState parent tag), then apply GE_EngineerState. Updated Activation Owned Tags from `Father.Form.Engineer, Father.State.Deployed` to `Father.State.TurretDeployed` (removed orphan tags). ~~Fixed invulnerability removal from tag-based to class-based~~ (SUPERSEDED by INV-1 - invulnerability removed entirely). Updated UE version to 5.7. Added Automation vs Manual table. Added Option B architecture section to Quick Reference. See Form_State_Architecture_Fix_v4.13.2.md for architecture rationale. |
| 4.3 | January 2026 | Fixed tag format: State.Father.Alive changed to Father.State.Alive per DefaultGameplayTags. Updated Related Documents to Technical Reference v5.12, Design Document v1.8, and Setup Guide v2.3. Fixed curly quotes to straight ASCII. |
| 4.2 | January 2026 | Simplified documentation: Variable creation (PHASE 4 Section 1) converted to markdown table - reduced from 76 lines to 14 lines. |
| 4.1 | January 2026 | Simplified configuration sections: Tag Config (Class Defaults) reduced from 51 to 8 lines. GE configs (TurretMode, TurretHealth, EngineerCooldown) reduced from 98 to 35 lines using table format. Total reduction: ~100 lines. |
| 4.0 | January 2026 | Simplified PHASE 1 tag creation - replaced detailed step-by-step instructions with simple tag list table. |
| 3.9 | January 2026 | Built-in cooldown system: Added CooldownGameplayEffectClass property, replaced Apply GE with CommitAbilityCooldown node. Removed Block Abilities with Tag for cooldown (GAS auto-blocks). |
| 3.8 | January 2026 | Form tags via Activation Owned Tags. ReplicateActivationOwnedTags required. Removed GE_EngineerState and EngineerStateHandle. Simplified EndAbility cleanup. |
| 3.7 | January 2026 | Added Activation Owned Tags section to Class Defaults. |
| 3.6 | December 2025 | Native attribute integration for turret health. |
| 3.5 | December 2025 | SaveGame configuration and Net Execution Policy: Server Only. |
| 3.3 | December 2025 | Added Father.State.Recruited, effect handle storage, PHASE 5B EndAbility cleanup. |
| 3.2 | December 2025 | Complete form transition system with Cancel Abilities with Tag and cooldowns. |
| 3.1 | December 2025 | Removed GA_TurretRecall, direct T wheel form switching only. |
| 3.0 | November 2025 | Initial GE_EngineerState architecture. | |
   - Ability stays active to maintain Activation Owned Tags
   - Per Design Doc v1.3 Section 3.4.2 Form Ability Node Pattern
   - GA_FatherEngineer remains active until cancelled by another form

#### **Clarifications Added:**

2. **TurretModeEffectHandle Purpose**
   - Added note explaining handle stored for EndAbility cleanup
   - Engineer is exception to source-based removal pattern (Tech Ref v1.7 Section 15.3)
   - Handle-based removal required because effects on father, not player

3. **Engineer Ability Architecture (Corrected v4.4)**
   - All Engineer abilities are baseline abilities on Father ASC
   - GA_FatherEngineer, GA_TurretShoot, GA_FatherElectricTrap, GA_FatherMark all in AC_FatherCompanion_Default
   - Engineer does NOT use EquippableItem - form activated via GAS directly from T wheel

#### **Technical Reference Alignment:**

- Aligned with Father_Companion_Technical_Reference_v5_12.md
- Confirmed effect removal pattern per Section 15
- Confirmed form ability lifecycle per Design Doc v1.3 Section 3.4.2

---

### **VERSION 2.0 - November 2025**

#### **Major Changes:**

1. **Fixed UTF-8 Characters**
   - Replaced all special arrow characters with ASCII "->"
   - Replaced degree symbols with "degrees" text
   - Replaced checkbox symbols with "Check" / "Uncheck" text

2. **Updated Ability Granting Architecture**
   - Changed from Default Abilities array to AbilityConfiguration asset
   - PHASE 6 now references AC_FatherCompanion_Default
   - Aligns with Father_Companion_Forms_Implementation_Guide_v4_0

3. **Added Instance Editable Properties**
   - MaxDeployRange now Instance Editable for designer tuning
   - MinDeployRange now Instance Editable
   - TurretMaxHealth now Instance Editable
   - RotationArc now Instance Editable
   - Follows Pattern 6 (Designer-Friendly Variables)

4. **Added Deployment Animation**
   - Added Play Anim Montage node with AM_FatherDeploy placeholder
   - Professional polish for form transitions

5. **Improved Turret AI Initialization**
   - Changed from custom EnableTurretAI function to direct Run Behavior Tree
   - Uses Get Controller -> Cast To AIController -> Run Behavior Tree pattern
   - References BT_FatherEngineer behavior tree

6. **Added Implementation Patterns Section**
   - Documents patterns 3, 5, 6 usage per Technical Reference

7. **Added Quick Reference Section**
   - Complete tag configuration summary
   - Variable summary with Instance Editable column
   - Gameplay effect summary
   - Engineer form abilities list

8. **Removed Activation Owned Tag: Father.State.Deployed**
   - Tag granted via GE_TurretMode instead
   - Cleaner architecture - effect manages state tags

9. **Improved Execution Path Merging**
   - Explicit Sequence node instructions for merging True/False branches
   - Clear Then pin connection instructions

#### **Minor Changes:**

- Updated version number from 1.0 to 2.0
- Added Estimated Time field to Document Information
- Improved Prerequisites section with granular format
- Removed conditional "if exists" logic per instructions
- Added Min Deploy Range to Technical Specifications
- Fixed formatting consistency throughout

#### **Compatibility Notes:**

- Fully compatible with Unreal Engine 5.6
- Requires Narrative Pro Plugin v2.2
- Requires AC_FatherCompanion_Default AbilityConfiguration
- BT_FatherEngineer behavior tree created in GA_TurretShoot guide
- AM_FatherDeploy animation montage is placeholder (create later)

---

### **VERSION 1.0 - Previous Release**

#### **Features:**

- Basic Engineer form activation ability
- Deployment system using line trace
- Turret mode with 270 degree rotation arc
- 500 HP turret health pool
- GE_TurretMode for state tags
- Form wheel integration

#### **Known Issues (Fixed in v2.0):**

- UTF-8 special characters caused display issues
- Default Abilities array approach not aligned with Forms guide
- Config variables not Instance Editable
- Missing deployment animation
- EnableTurretAI function not defined
- Missing Quick Reference section

---

## **QUICK REFERENCE**

### **1. Tag Configuration Summary**

| Property | Tags |
|----------|------|
| Ability Tags | `Ability.Father.Engineer` |
| Cancel Abilities with Tag | `Ability.Father.Crawler`, `Ability.Father.Armor`, `Ability.Father.Exoskeleton` |
| Activation Owned Tags | `Father.State.TurretDeployed` |

> **AUDIT NOTE (v4.7 - 2026-01-23):** `Ability.Father.Symbiote` removed from cancel list per C_SYMBIOTE_STRICT_CANCEL contract (LOCKED_CONTRACTS.md Contract 11). Symbiote is an ultimate ability (30s duration) that cannot be cancelled by player-initiated form changes. Defense-in-depth: Layer 1 blocks via `Father.State.SymbioteLocked` in activation_blocked_tags, Layer 2 ensures no cancel path exists.
| Activation Required | `Father.State.Alive`, `Father.State.Recruited` |
| Activation Blocked | `Father.State.Dormant`, `Father.State.Transitioning`, `Father.State.SymbioteLocked`, `Cooldown.Father.FormChange` |
| Cooldown Gameplay Effect Class | `GE_FormChangeCooldown` |
| InputTag | `Narrative.Input.Father.FormChange` |

### **1.1 Form State Architecture (Option B)**

| Component | Description |
|-----------|-------------|
| Activation Owned Tags | `Father.State.TurretDeployed` (ephemeral, auto-granted on activation) |
| Form Identity | GE_EngineerState grants `Effect.Father.FormState.Engineer` (persists via infinite GE) |
| Transitioning Tag | AddLooseGameplayTag(Father.State.Transitioning) during 5s transition (**NO invulnerability** per INV-1) |
| GE_FormChangeCooldown | 15s cooldown after form change |
| Mutual Exclusivity | Cancel Abilities With Tag cancels other form abilities |
| Transition Prelude | Remove `Effect.Father.FormState.*` → Apply `GE_EngineerState` |

**Note (v4.4):** `Father.Form.*` tags are **orphan tags** - no persistent GE grants them. Form identity uses `Effect.Father.FormState.*` tags. Do NOT use Father.Form.* for activation gating.

### **1.2 Effect Asset Tags**

| Tag | Purpose |
|-----|---------|
| `Effect.Father.FormState.Engineer` | Form identity (granted by GE_EngineerState) |
| `Effect.Father.TurretMode` | Turret mode effect identifier |
| `Cooldown.Father.Engineer` | Post-recall cooldown |

### **2. Ability Variables Summary**

| Variable | Type | Instance Editable | Default | Purpose |
|----------|------|-------------------|---------|---------|
| bIsFirstActivation | Boolean | No | True | Distinguishes spawn vs form switch |
| MaxDeployRange | Float | Yes | 1500.0 | Maximum deployment distance |
| MinDeployRange | Float | Yes | 200.0 | Minimum deployment distance |
| TurretMaxHealth | Float | Yes | 500.0 | Turret HP pool |
| RotationArc | Float | Yes | 270.0 | Turret rotation coverage |
| PlayerRef | Actor Ref | No | None | Owning player reference |
| FatherRef | BP_FatherCompanion Ref | No | None | Father reference |
| DeployLocation | Vector | No | (0,0,0) | Calculated deploy position |
| DeployRotation | Rotator | No | (0,0,0) | Calculated deploy rotation |

### **3. Gameplay Effect Summary**

| Effect | Duration | Purpose |
|--------|----------|---------|
| GE_EngineerState | Infinite | Form identity (grants Effect.Father.FormState.Engineer) |
| GE_TurretMode | Infinite | Grants turret behavior state tags |
| GE_FormChangeCooldown | 15 seconds | Shared cooldown between form changes |
| GE_EngineerCooldown | 5 seconds | Post-recall cooldown |

> **INV-1 Note:** GE_TransitionInvulnerability was REMOVED per GAS Audit decision INV-1. Transitions use AddLooseGameplayTag(Father.State.Transitioning) only - no damage immunity.

### **4. Turret Parameters**

| Parameter | Value |
|-----------|-------|
| Max Deploy Range | 1500 units |
| Min Deploy Range | 200 units |
| Turret Health | 500 HP |
| Rotation Arc | 270 degrees |
| Detection Range | 1500 units |
| Shoot Range | 500-1500 units |
| Trap Range | Less than 500 units |

### **5. Engineer Form Abilities**

| Ability | Type | Trigger | Granted Via |
|---------|------|---------|-------------|
| GA_FatherEngineer | Form | T (Wheel) | AC_FatherCompanion_Default |
| GA_TurretShoot | AI | Auto (far enemies) | AC_FatherCompanion_Default |
| GA_FatherElectricTrap | AI | Auto (close enemies) | AC_FatherCompanion_Default |
| GA_FatherMark | Passive | On enemy detection | AC_FatherCompanion_Default |

**Note (v4.4):** All Engineer abilities are baseline abilities on Father ASC. Engineer does NOT use EquippableItem for ability granting - form is activated via GAS directly from T wheel.

### **6. BP_FatherCompanion Variables Required**

| Variable | Type | Replicated | SaveGame | Purpose |
|----------|------|------------|----------|---------|
| IsDeployed | Boolean | Yes | Yes | Turret deployment state |
| DeployedTransform | Transform | Yes | Yes | Turret world position and rotation |
| TurretModeEffectHandle | Active GE Handle | No | No | GE_TurretMode handle for EndAbility cleanup |

### **6.1 Turret Health (Native Attributes)**

Turret health uses father's existing NarrativeAttributeSetBase attributes:

| Attribute | Modified By | Value |
|-----------|-------------|-------|
| MaxHealth | GE_TurretHealth | 500.0 |
| Health | GE_TurretHealth | 500.0 |

### **7. Replication Settings**

| Setting | Value | Purpose |
|---------|-------|---------|
| Instancing Policy | Instanced Per Actor | One instance per ASC |
| Replication Policy | Replicate | Effects and tags replicate |
| Net Execution Policy | Server Only | NPC-owned form ability spawning replicated actors |
| ReplicateActivationOwnedTags | Enabled (Project Setting) | Form tags replicate to all clients |
| Replicate Input Directly | Unchecked | Use AbilityTasks instead |
| Server Respects Remote Ability Cancellation | Unchecked | Server authoritative |

### **8. Form State Architecture (Option B)**

| Component | Description |
|-----------|-------------|
| **Activation Owned Tags** | `Father.State.TurretDeployed` (ephemeral, auto-granted) |
| **Form Identity** | GE_EngineerState grants `Effect.Father.FormState.Engineer` (persists via infinite GE) |
| **Transition Prelude** | Remove `Effect.Father.FormState.*` parent tag → Apply `GE_EngineerState` |
| **Replication** | Tags replicate via ReplicateActivationOwnedTags setting |
| **Cleanup (State Tags)** | Activation Owned Tags auto-removed when ability ends |
| **Cleanup (Form Identity)** | GE_EngineerState removed by next form's transition prelude |

### **9. Related Documents**

| Document | Purpose |
|----------|---------|
| GE_FormChangeCooldown_Implementation_Guide_v1_0.md | Form cooldown effect |
| Father_Companion_System_Design_Document_v1_8.md | Section 13.6: Form Transition System |
| Father_Companion_Technical_Reference_v5_12.md | Net Execution Policy, Effect Handle Storage, EndAbility Cleanup |
| Father_Companion_System_Setup_Guide_v2_3.md | Variable definition, SaveGame configuration |
| DefaultGameplayTags_FatherCompanion_v3_5.ini | All gameplay tags including Father.State.Recruited |

---

**END OF GA_FATHERENGINEER IMPLEMENTATION GUIDE v4.4**

**Engineer Form - Stationary Turret Deployment**

**Unreal Engine 5.7 + Narrative Pro v2.2**

**Blueprint-Only Implementation - Option B Form State Architecture (GE-Based Form Identity)**
