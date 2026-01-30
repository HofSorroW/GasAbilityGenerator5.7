# GA_FatherSacrifice Implementation Guide
## VERSION 2.6 - GAS Audit Compliant (ONLY Ability With Invulnerability)
## Emergency Save Passive Ability
## Unreal Engine 5.7 + Narrative Pro v2.2
## Blueprint-Only Implementation

---

## DOCUMENT INFORMATION

| Property | Value |
|----------|-------|
| Document Version | 2.6 |
| Ability Name | GA_FatherSacrifice |
| Ability Type | Passive (Automatic Trigger) |
| Parent Class | NarrativeGameplayAbility |
| Form Availability | All Forms (Baseline) |
| Input Required | None (Auto-Trigger on Grant) |

---

## TABLE OF CONTENTS

1. Ability Overview
2. Prerequisites
3. Changelog
4. Phase 1: Gameplay Tags Setup
5. Phase 2: EFatherForm Enum Update
6. Phase 3: Create Gameplay Ability Blueprint
7. Phase 4: Configure Ability Properties
8. Phase 5: Create Variables
9. Phase 6: Health Monitoring System
10. Phase 7: Execute Sacrifice Function
11. Phase 8: Invulnerability Application
12. Phase 9: Reactivate Father Function
13. Phase 10: Implement EndAbility Event
14. Phase 11: Gameplay Effects
15. Phase 12: Integration

---

## ABILITY OVERVIEW

| Parameter | Value |
|-----------|-------|
| Trigger Condition | Player Health drops below 15% of MaxHealth |
| Protection | 10 seconds of complete invulnerability |
| Dormant Duration | 180 seconds (fixed, hidden from player) |
| Recovery Form | Armor |
| Cooldown | Duration of dormant state |
| Form During Dormant | Offline (enum value) |

---

## INVULNERABILITY ARCHITECTURE

| Aspect | Implementation |
|--------|----------------|
| Protection Method | State.Invulnerable tag via GE_SacrificeInvulnerability |
| Duration | 10 seconds (Has Duration effect) |
| Damage Blocking | NarrativeDamageExecCalc checks for State.Invulnerable tag |
| No Absorption Math | All damage blocked completely - no heal-back needed |
| Blueprint Compatible | Tag-based system works entirely in Blueprint |

---

## SACRIFICE FLOW

| Phase | Description |
|-------|-------------|
| Phase 1: Trigger | Player Health < 15%, health monitoring delegate fires |
| Phase 2: Sacrifice | Cancel all forms, attach to chest, apply 8s invulnerability, father enters DORMANT |
| Phase 3: Dormant | Father dark/lifeless on chest, Father.State.Offline tag active, 180 seconds |
| Phase 4: Reactivation | Father awakens, Armor form activates |

---

## PREREQUISITES

| Requirement | Description |
|-------------|-------------|
| BP_FatherCompanion | Father companion blueprint (NarrativeNPCCharacter child) |
| Father_Companion_System_Setup_Guide_v1_3.md | Father setup with OwnerPlayer reference |
| GA_FatherArmor | Armor form ability for reactivation |
| Player Setup | Player has NarrativeAbilitySystemComponent with Health attribute |
| Socket | FatherChestSocket on player mesh |
| EFatherForm | Enum with Offline value (index 5) |

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 2.6 | January 2026 | **Locked Decisions Reference:** Added Father_Companion_GAS_Abilities_Audit.md reference. This guide is the ONLY ability with invulnerability per INV-1 decision. GE_SacrificeInvulnerability (8s to PLAYER) is the only kept invulnerability effect. Updated UE version to 5.7. Updated Technical Reference to v6.2. |
| 2.5 | January 2026 | Updated Narrative Pro version reference from v2.1 to v2.2. |
| 2.4 | January 2026 | Simplified documentation: Tag configuration (PHASE 4 Section 10) converted to single Property/Tags table. |
| 2.3 | January 2026 | Simplified PHASE 1: Replaced 6 detailed subsections with consolidated tag tables. Removed step-by-step Project Settings navigation. Tags now listed in Create Required Tags and Verify Existing Tags tables. Reduced PHASE 1 from 56 lines to 18 lines. |
| 2.2 | December 2025 | Technical Reference v4.5 compliance: Added Father.State.Recruited to Activation Required Tags (new section 10.3A). Added OwnerPlayer validation in ExecuteSacrifice function before accessing player ASC. Added PHASE 10: Implement EndAbility Event with timer cleanup, effect removal using OfflineEffectHandle, health delegate unbinding. Updated Prerequisites to reference Setup Guide v1_3. Updated Related Documents to v4.5/v3.5/v1.3. |
| 2.1 | December 2025 | Replaced shield system with 8-second invulnerability. Removed PHASE 3 (BP_FatherCompanion Shield System), PHASE 10 (Shield Decay), PHASE 11 (Shield Absorption). State.Invulnerable tag blocks all damage via NarrativeDamageExecCalc. Simplified from 13 phases to 11 phases. Attack tokens noted as future C++ enhancement. |
| 2.0 | November 2025 | Replaced Shield attribute with Blueprint Shield variable system. Added shield decay and OnDamagedBy heal-back pattern. |
| 1.0 | November 2025 | Initial implementation with Shield attribute (non-existent) |

---

## **PHASE 1: GAMEPLAY TAGS SETUP**

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Sacrifice | Father emergency sacrifice passive ability |
| Father.State.Offline | Father is in dormant/offline state after sacrifice |
| Father.State.Sacrificing | Father is currently performing sacrifice sequence |
| Cooldown.Father.Sacrifice | Sacrifice ability on cooldown - father dormant |

### **Verify Existing Tags**

| Tag Name | Purpose |
|----------|---------|
| State.Invulnerable | Target is invulnerable to all damage (Narrative Pro built-in) |
| Father.State.Recruited | Must be recruited (Activation Required) |
| Narrative.State.IsDead | Block when dead (Activation Blocked) |

---

## **PHASE 2: EFATHERFORM ENUM UPDATE**

### **7) Open EFatherForm Enum**

#### 7.1) Locate Enum Asset
   - 7.1.1) In Content Browser, navigate to: `/Content/FatherCompanion/Enums/`
   - 7.1.2) Locate **EFatherForm** asset
   - 7.1.3) Double-click to open Enum Editor

### **8) Add Offline Enum Value**

#### 8.1) Add New Enumerator
   - 8.1.1) Click **Add Enumerator** button in toolbar
   - 8.1.2) **Name**: `Offline`
   - 8.1.3) **Display Name**: `Offline`
   - 8.1.4) **Description**: `Father is dormant after sacrifice`

#### 8.2) Verify Enum Values
   - 8.2.1) Crawler (index 0)
   - 8.2.2) Armor (index 1)
   - 8.2.3) Exoskeleton (index 2)
   - 8.2.4) Symbiote (index 3)
   - 8.2.5) Engineer (index 4)
   - 8.2.6) Offline (index 5)

#### 8.3) Save Enum
   - 8.3.1) Click **Compile**
   - 8.3.2) Click **Save**

---

## **PHASE 3: CREATE GAMEPLAY ABILITY BLUEPRINT**

### **9) Create GA_FatherSacrifice Blueprint**

#### 9.1) Create Blueprint Asset
   - 9.1.1) In Content Browser, navigate to: `/Content/FatherCompanion/Abilities/`
   - 9.1.2) Right-click in empty space
   - 9.1.3) Select **Blueprint Class**
   - 9.1.4) In Pick Parent Class window, expand **All Classes**
   - 9.1.5) Search for: `NarrativeGameplayAbility`
   - 9.1.6) Select **NarrativeGameplayAbility**
   - 9.1.7) Click **Select**
   - 9.1.8) Name: `GA_FatherSacrifice`
   - 9.1.9) Double-click to open Blueprint Editor

---

## **PHASE 4: CONFIGURE ABILITY PROPERTIES**

### **10) Configure Class Defaults**

#### 10.1) Open Class Defaults
   - 10.1.1) Click **Class Defaults** button in toolbar

#### 10.2) Configure Ability Tags

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Sacrifice |
| Activation Owned Tags | Father.State.Sacrificing |
| Activation Required Tags | Father.State.Alive, Father.State.Recruited |
| Activation Blocked Tags | Cooldown.Father.Sacrifice, Father.State.Offline, Narrative.State.IsDead |
| Cancel Abilities with Tag | Ability.Father.Crawler, Ability.Father.Armor, Ability.Father.Exoskeleton, Ability.Father.Symbiote, Ability.Father.Engineer |

#### 10.3) Configure Instancing Policy
   - 10.6.1) Find **Ability** category
   - 10.6.2) Locate **Instancing Policy**
   - 10.6.3) Set to: `Instanced Per Actor`

#### 10.7) Configure Replication
   - 10.7.1) Find **Net Execution Policy**
   - 10.7.2) Set to: `Server Initiated`
   - 10.7.3) Find **Replication Policy**
   - 10.7.4) Set to: `Replicate Yes`

#### 10.8) Configure Auto-Activation
   - 10.8.1) Find **Narrative Ability** category
   - 10.8.2) Locate **bActivateAbilityOnGranted**
   - 10.8.3) Set to: Checked (true)
   - 10.8.4) This enables automatic activation when granted

#### 10.9) Clear Input Tag
   - 10.9.1) Find **Input Tag** property
   - 10.9.2) Set to: `None` or leave empty
   - 10.9.3) This ability has no manual input

---

## **PHASE 5: CREATE VARIABLES**

### **11) Create Ability Variables**

#### 11.1) Create PlayerRef Variable
   - 11.1.1) In My Blueprint panel, find **Variables** section
   - 11.1.2) Click **+** button next to Variables
   - 11.1.3) Name: `PlayerRef`
   - 11.1.4) Click on variable to select
   - 11.1.5) In Details panel, find **Variable Type**
   - 11.1.6) Set type to: `Actor` -> `Object Reference`
   - 11.1.7) Click **Compile**
   - 11.1.8) Set **Instance Editable**: Unchecked
   - 11.1.9) Set **Category**: `Runtime`

#### 11.2) Create FatherRef Variable
   - 11.2.1) Click **+** next to Variables
   - 11.2.2) Name: `FatherRef`
   - 11.2.3) Set **Variable Type**: `BP_FatherCompanion` -> `Object Reference`
   - 11.2.4) Click **Compile**
   - 11.2.5) Set **Instance Editable**: Unchecked
   - 11.2.6) Set **Category**: `Runtime`

#### 11.3) Create PlayerASC Variable
   - 11.3.1) Click **+** next to Variables
   - 11.3.2) Name: `PlayerASC`
   - 11.3.3) Set **Variable Type**: Search `Narrative Ability System Component`
   - 11.3.4) Select: `Narrative Ability System Component` -> `Object Reference`
   - 11.3.5) Click **Compile**
   - 11.3.6) Set **Instance Editable**: Unchecked
   - 11.3.7) Set **Category**: `Runtime`

#### 11.4) Create HealthThreshold Variable
   - 11.4.1) Click **+** next to Variables
   - 11.4.2) Name: `HealthThreshold`
   - 11.4.3) Set **Variable Type**: `Float`
   - 11.4.4) Click **Compile**
   - 11.4.5) Set **Instance Editable**: Checked
   - 11.4.6) Set **Default Value**: `0.15` (15%)
   - 11.4.7) Set **Category**: `Configuration`

#### 11.5) Create InvulnerabilityDuration Variable
   - 11.5.1) Click **+** next to Variables
   - 11.5.2) Name: `InvulnerabilityDuration`
   - 11.5.3) Set **Variable Type**: `Float`
   - 11.5.4) Click **Compile**
   - 11.5.5) Set **Instance Editable**: Checked
   - 11.5.6) Set **Default Value**: `10.0` (10 seconds)
   - 11.5.7) Set **Category**: `Configuration`

#### 11.6) Create DormantDuration Variable
   - 11.6.1) Click **+** next to Variables
   - 11.6.2) Name: `DormantDuration`
   - 11.6.3) Set **Variable Type**: `Float`
   - 11.6.4) Click **Compile**
   - 11.6.5) Set **Instance Editable**: Checked
   - 11.6.6) Set **Default Value**: `180.0` (180 seconds fixed)
   - 11.6.7) Set **Category**: `Configuration`

#### 11.7) Create OfflineEffectHandle Variable
   - 11.8.1) Click **+** next to Variables
   - 11.8.2) Name: `OfflineEffectHandle`
   - 11.8.3) Set **Variable Type**: Search `Active Gameplay Effect Handle`
   - 11.8.4) Select: `Active Gameplay Effect Handle` (structure type)
   - 11.8.5) Click **Compile**
   - 11.8.6) Set **Instance Editable**: Unchecked
   - 11.8.7) Set **Category**: `Runtime`

#### 11.9) Create DormantTimerHandle Variable
   - 11.9.1) Click **+** next to Variables
   - 11.9.2) Name: `DormantTimerHandle`
   - 11.9.3) Set **Variable Type**: `Timer Handle`
   - 11.9.4) Click **Compile**
   - 11.9.5) Set **Instance Editable**: Unchecked
   - 11.9.6) Set **Category**: `Runtime`

#### 11.10) Create IsMonitoring Variable
   - 11.10.1) Click **+** next to Variables
   - 11.10.2) Name: `IsMonitoring`
   - 11.10.3) Set **Variable Type**: `Boolean`
   - 11.10.4) Click **Compile**
   - 11.10.5) Set **Instance Editable**: Unchecked
   - 11.10.6) Set **Default Value**: Unchecked (false)
   - 11.10.7) Set **Category**: `Runtime`

#### 11.11) Create HealthChangeDelegateHandle Variable
   - 11.11.1) Click **+** next to Variables
   - 11.11.2) Name: `HealthChangeDelegateHandle`
   - 11.11.3) Set **Variable Type**: Search `Delegate Handle`
   - 11.11.4) Select: `FDelegateHandle` (structure type)
   - 11.11.5) Click **Compile**
   - 11.11.6) Set **Instance Editable**: Unchecked
   - 11.11.7) Set **Category**: `Runtime`

### **12) Variable Summary Table**

| Variable | Type | Instance Editable | Default | Category |
|----------|------|-------------------|---------|----------|
| PlayerRef | Actor Ref | Unchecked | None | Runtime |
| FatherRef | BP_FatherCompanion Ref | Unchecked | None | Runtime |
| PlayerASC | NarrativeAbilitySystemComponent Ref | Unchecked | None | Runtime |
| HealthThreshold | Float | Checked | 0.15 | Configuration |
| InvulnerabilityDuration | Float | Checked | 10.0 | Configuration |
| DormantDuration | Float | Checked | 180.0 | Configuration |
| OfflineEffectHandle | Active GE Handle | Unchecked | None | Runtime |
| DormantTimerHandle | Timer Handle | Unchecked | None | Runtime |
| IsMonitoring | Boolean | Unchecked | false | Runtime |
| HealthChangeDelegateHandle | FDelegateHandle | Unchecked | None | Runtime |

---

## **PHASE 6: HEALTH MONITORING SYSTEM**

### **13) Create ActivateAbility Event**

#### 13.1) Add Event Node
   - 13.1.1) Open **Event Graph**
   - 13.1.2) Right-click in empty space
   - 13.1.3) Search: `Event ActivateAbility`
   - 13.1.4) Select **Event ActivateAbility**

### **14) Get Avatar Actor (Father)**

#### 14.1) Get Avatar from Actor Info
   - 14.1.1) From **Event ActivateAbility** node:
      - 14.1.1.1) Find **Actor Info** pin (structure pin)
      - 14.1.1.2) Drag from Actor Info pin outward
      - 14.1.1.3) Search: `Get Avatar Actor from Actor Info`
      - 14.1.1.4) Add **Get Avatar Actor from Actor Info** node

### **15) Cast to BP_FatherCompanion**

#### 15.1) Add Cast Node
   - 15.1.1) From **Event ActivateAbility** execution pin:
      - 15.1.1.1) Drag outward into empty space
      - 15.1.1.2) Search: `Cast To BP_FatherCompanion`
      - 15.1.1.3) Add **Cast To BP_FatherCompanion** node
   - 15.1.2) Connect **Return Value** from Get Avatar Actor to **Object** input

#### 15.2) Handle Cast Failure
   - 15.2.1) From Cast **Cast Failed** execution pin:
      - 15.2.1.1) Drag outward and search: `End Ability`
      - 15.2.1.2) Add **End Ability** node
   - 15.2.2) On End Ability node:
      - 15.2.2.1) Find **Was Cancelled** checkbox
      - 15.2.2.2) Set: Checked (true)

### **16) Store Father Reference**

#### 16.1) Set FatherRef Variable
   - 16.1.1) From Cast success execution pin:
      - 16.1.1.1) Drag outward and search: `Set FatherRef`
      - 16.1.1.2) Add **Set FatherRef** node
   - 16.1.2) Connect **As BP Father Companion** to FatherRef value input

### **17) Get Owner Player**

#### 17.1) Get Owner Player from Father
   - 17.1.1) From **As BP Father Companion** output pin:
      - 17.1.1.1) Drag outward and search: `Get Owner Player`
      - 17.1.1.2) Add **Get Owner Player** node

#### 17.2) Store Player Reference
   - 17.2.1) From **Set FatherRef** execution pin:
      - 17.2.1.1) Drag outward and search: `Set PlayerRef`
      - 17.2.1.2) Add **Set PlayerRef** node
   - 17.2.2) Connect **Get Owner Player** Return Value to PlayerRef value input

### **18) Validate Player Reference**

#### 18.1) Add Branch for Validation
   - 18.1.1) From **Set PlayerRef** execution pin:
      - 18.1.1.1) Drag outward and search: `Branch`
      - 18.1.1.2) Add **Branch** node

#### 18.2) Create Validation Condition
   - 18.2.1) Drag **PlayerRef** variable getter into graph
   - 18.2.2) From PlayerRef:
      - 18.2.2.1) Drag outward and search: `Is Valid`
      - 18.2.2.2) Add **Is Valid** node
   - 18.2.3) Connect **Is Valid** Return Value to Branch **Condition**

#### 18.3) Handle Invalid Reference
   - 18.3.1) From Branch **False** pin:
      - 18.3.1.1) Add **End Ability** node
      - 18.3.1.2) Set **Was Cancelled**: Checked (true)

### **19) Get Player ASC**

#### 19.1) Get Ability System Component
   - 19.1.1) From Branch **True** pin:
      - 19.1.1.1) Drag outward and search: `Get Ability System Component`
      - 19.1.1.2) Add **Get Ability System Component** node
   - 19.1.2) Connect **PlayerRef** to Actor input

#### 19.2) Cast to NarrativeAbilitySystemComponent
   - 19.2.1) From Get Ability System Component execution pin:
      - 19.2.1.1) Drag outward and search: `Cast To NarrativeAbilitySystemComponent`
      - 19.2.1.2) Add **Cast To NarrativeAbilitySystemComponent** node
   - 19.2.2) Connect Return Value from Get ASC to **Object** input

#### 19.3) Store PlayerASC Reference
   - 19.3.1) From Cast success execution pin:
      - 19.3.1.1) Drag outward and search: `Set PlayerASC`
      - 19.3.1.2) Add **Set PlayerASC** node
   - 19.3.2) Connect **As Narrative Ability System Component** to value input

### **20) Bind Health Change Delegate**

#### 20.1) Get Health Attribute Delegate
   - 20.1.1) From **Set PlayerASC** execution pin:
      - 20.1.1.1) Drag outward and search: `Get Gameplay Attribute Value Change Delegate`
      - 20.1.1.2) Add **Get Gameplay Attribute Value Change Delegate** node
   - 20.1.2) Connect **PlayerASC** variable to Target input
   - 20.1.3) For **Attribute** parameter:
      - 20.1.3.1) Click dropdown
      - 20.1.3.2) Expand **NarrativeAttributeSetBase**
      - 20.1.3.3) Select: `Health`

#### 20.2) Bind Event to Delegate
   - 20.2.1) From **Get Gameplay Attribute Value Change Delegate** Return Value:
      - 20.2.1.1) Drag outward and search: `Bind Event`
      - 20.2.1.2) Add **Bind Event to On Gameplay Attribute Value Change** node
   - 20.2.2) From Event (red) pin:
      - 20.2.2.1) Drag outward and search: `Create Event`
      - 20.2.2.2) Select **Create Matching Function**
      - 20.2.2.3) Name function: `OnHealthChanged`
      - 20.2.2.4) Function created and bound to delegate

#### 20.3) Store Delegate Handle
   - 20.3.1) From **Bind Event** Return Value:
      - 20.3.1.1) Add **Set HealthChangeDelegateHandle** node
      - 20.3.1.2) Connect Bind Event return to handle input

### **21) Set Monitoring Flag and Commit**

#### 21.1) Set IsMonitoring Flag
   - 21.1.1) From Set HealthChangeDelegateHandle execution pin:
      - 21.1.1.1) Drag outward and search: `Set IsMonitoring`
      - 21.1.1.2) Add **Set IsMonitoring** node
   - 21.1.2) Set value: Checked (true)

#### 21.2) Commit Ability
   - 21.2.1) From **Set IsMonitoring** execution pin:
      - 21.2.1.1) Drag outward and search: `Commit Ability`
      - 21.2.1.2) Add **Commit Ability** node

### **22) Implement OnHealthChanged Function**

#### 22.1) Open OnHealthChanged Function
   - 22.1.1) In My Blueprint panel, find **Functions** section
   - 22.1.2) Double-click **OnHealthChanged** function

#### 22.2) Check if Monitoring Active
   - 22.2.1) From function entry node:
      - 22.2.1.1) Drag **IsMonitoring** variable getter into graph
   - 22.2.2) Add **Branch** node
   - 22.2.3) Connect **IsMonitoring** to Condition

#### 22.3) Get Current Health Values
   - 22.3.1) From Branch **True** pin:
      - 22.3.1.1) Drag **PlayerASC** variable getter into graph
   - 22.3.2) From PlayerASC:
      - 22.3.2.1) Drag outward and search: `Get Numeric Attribute`
      - 22.3.2.2) Add **Get Numeric Attribute** node
      - 22.3.2.3) Set Attribute: `Health`
   - 22.3.3) Add another **Get Numeric Attribute** node
      - 22.3.3.1) Set Attribute: `MaxHealth`

#### 22.4) Calculate Health Percentage
   - 22.4.1) Add **Float / Float** node
   - 22.4.2) Connect Health value to **A** input
   - 22.4.3) Connect MaxHealth value to **B** input

#### 22.5) Check Against Threshold
   - 22.5.1) Add **Float < Float** node
   - 22.5.2) Connect division result to **A** input
   - 22.5.3) Drag **HealthThreshold** variable getter into graph
   - 22.5.4) Connect HealthThreshold to **B** input

#### 22.6) Execute Sacrifice if Below Threshold
   - 22.6.1) Add **Branch** node
   - 22.6.2) Connect < result to **Condition**
   - 22.6.3) From Branch **True** pin:
      - 22.6.3.1) Drag outward and search: `Execute Sacrifice`
      - 22.6.3.2) Add **Execute Sacrifice** function call

---

## **PHASE 7: EXECUTE SACRIFICE FUNCTION**

### **23) Create ExecuteSacrifice Function**

#### 23.1) Create Function
   - 23.1.1) In My Blueprint panel, find **Functions** section
   - 23.1.2) Click **+** button next to Functions
   - 23.1.3) Name: `ExecuteSacrifice`
   - 23.1.4) Double-click to open function graph

### **24) Stop Health Monitoring**

#### 24.1) Set IsMonitoring False
   - 24.1.1) From function entry execution pin:
      - 24.1.1.1) Drag outward and search: `Set IsMonitoring`
      - 24.1.1.2) Add **Set IsMonitoring** node
   - 24.1.2) Set value: Unchecked (false)

### **24A) Validate Player Reference**

#### 24A.1) Check PlayerRef Is Valid
   - 24A.1.1) From **Set IsMonitoring** execution pin:
      - 24A.1.1.1) Drag **PlayerRef** variable getter into graph
      - 24A.1.1.2) Add **Is Valid** node
      - 24A.1.1.3) Add **Branch** node
      - 24A.1.1.4) Connect Is Valid to Branch Condition
   - 24A.1.2) From Branch **False** pin:
      - 24A.1.2.1) Add **Return Node** (early exit)

### **25) Cancel Current Forms**

#### 25.1) Get Father ASC
   - 25.1.1) From Branch **True** execution pin:
      - 25.1.1.1) Drag **FatherRef** variable getter into graph
   - 25.1.2) From FatherRef:
      - 25.1.2.1) Drag outward and search: `Get Ability System Component`
      - 25.1.2.2) Add **Get Ability System Component** node

#### 25.2) Cancel Form Abilities
   - 25.2.1) From Get ASC execution pin:
      - 25.2.1.1) Drag outward and search: `Cancel Abilities`
      - 25.2.1.2) Add **Cancel Abilities** node
   - 25.2.2) For **With Tags** parameter:
      - 25.2.2.1) Click dropdown arrow
      - 25.2.2.2) Select **Make GameplayTagContainer**
      - 25.2.2.3) Add tags:
         - 25.2.2.3.a) `Ability.Father.Crawler`
         - 25.2.2.3.b) `Ability.Father.Armor`
         - 25.2.2.3.c) `Ability.Father.Exoskeleton`
         - 25.2.2.3.d) `Ability.Father.Symbiote`
         - 25.2.2.3.e) `Ability.Father.Engineer`

### **26) Play Sacrifice Animation**

#### 26.1) Play Montage
   - 26.1.1) From **Cancel Abilities** execution pin:
      - 26.1.1.1) Drag outward and search: `Play Anim Montage`
      - 26.1.1.2) Add **Play Anim Montage** node
   - 26.1.2) Connect **FatherRef** to Target input
   - 26.1.3) Set **Montage to Play**: `AM_FatherSacrifice` (placeholder)

### **27) Attach Father to Player Chest**

#### 27.1) Get Player Mesh
   - 27.1.1) From **Play Anim Montage** execution pin:
      - 27.1.1.1) Drag **PlayerRef** variable getter into graph
   - 27.1.2) From PlayerRef:
      - 27.1.2.1) Drag outward and search: `Get Mesh`
      - 27.1.2.2) Add **Get Mesh** node

#### 27.2) Attach Actor to Component
   - 27.2.1) Drag outward and search: `Attach Actor To Component`
   - 27.2.2) Add **Attach Actor To Component** node
   - 27.2.3) Connect **FatherRef** to Target input
   - 27.2.4) Connect **Get Mesh** Return Value to Parent input
   - 27.2.5) Set **Socket Name**: `FatherChestSocket`
   - 27.2.6) Set **Location Rule**: `Snap to Target`
   - 27.2.7) Set **Rotation Rule**: `Snap to Target`
   - 27.2.8) Set **Scale Rule**: `Keep World`

### **28) Set Father Form to Offline**

#### 28.1) Set Current Form
   - 28.1.1) From **Attach Actor To Component** execution pin:
      - 28.1.1.1) Drag **FatherRef** variable getter into graph
   - 28.1.2) From FatherRef:
      - 28.1.2.1) Drag outward and search: `Set Current Form`
      - 28.1.2.2) Add **Set Current Form** node
   - 28.1.3) Set **Current Form**: `Offline`

### **29) Set Father Attached State**

#### 29.1) Set Is Attached
   - 29.1.1) From **Set Current Form** execution pin:
      - 29.1.1.1) From FatherRef, add **Set Is Attached** node
   - 29.1.2) Set **Is Attached**: Checked (true)

### **30) Apply Invulnerability to Player**

#### 30.1) Validate PlayerASC
   - 30.1.1) From **Set Is Attached** execution pin:
      - 30.1.1.1) Drag **PlayerASC** variable getter into graph
      - 30.1.1.2) Add **Is Valid** node
      - 30.1.1.3) Add **Branch** node
      - 30.1.1.4) Connect Is Valid to Branch Condition

#### 30.2) Apply Invulnerability Effect
   - 30.2.1) From Branch **True** execution pin:
      - 30.2.1.1) From PlayerASC:
      - 30.2.1.2) Drag outward and search: `Apply Gameplay Effect to Self`
      - 30.2.1.3) Add **Apply Gameplay Effect to Self** node
   - 30.2.2) Set **Gameplay Effect Class**: `GE_SacrificeInvulnerability`

### **31) Apply Offline Effect to Father**

#### 31.1) Get Father ASC
   - 31.1.1) From **Apply GE to Self** execution pin (or Branch False):
      - 31.1.1.1) Drag **FatherRef** variable getter into graph
   - 31.1.2) From FatherRef:
      - 31.1.2.1) Add **Get Ability System Component** node

#### 31.2) Apply Effect to Father
   - 31.2.1) From Father ASC:
      - 31.2.1.1) Drag outward and search: `Apply Gameplay Effect to Self`
      - 31.2.1.2) Add **Apply Gameplay Effect to Self** node
   - 31.2.2) Set **Gameplay Effect Class**: `GE_FatherOffline`

#### 31.3) Store Effect Handle
   - 31.3.1) From Apply GE **Return Value**:
      - 31.3.1.1) Drag outward and search: `Set OfflineEffectHandle`
      - 31.3.1.2) Add **Set OfflineEffectHandle** node
   - 31.3.2) Connect Return Value to handle input

### **32) Start Dormant Timer**

#### 32.1) Get Dormant Duration
   - 32.1.1) Drag **DormantDuration** variable getter into graph

#### 32.2) Set Timer for Reactivation
   - 32.2.1) From **Set OfflineEffectHandle** execution pin:
      - 32.2.1.1) Drag outward and search: `Set Timer by Function Name`
      - 32.2.1.2) Add **Set Timer by Function Name** node
   - 32.2.2) Configure timer:
      - 32.2.2.1) **Function Name**: `ReactivateFather`
      - 32.2.2.2) **Time**: Connect **DormantDuration** variable (180 seconds fixed)
      - 32.2.2.3) **Looping**: Unchecked

#### 32.3) Store Timer Handle
   - 32.3.1) From Set Timer **Return Value**:
      - 32.3.1.1) Drag outward and search: `Set DormantTimerHandle`
      - 32.3.1.2) Add **Set DormantTimerHandle** node

---

## **PHASE 8: INVULNERABILITY APPLICATION**

### **34) Invulnerability Effect Summary**

| Property | Value |
|----------|-------|
| Effect Name | GE_SacrificeInvulnerability |
| Target | Player ASC |
| Duration Policy | Has Duration |
| Duration | 10 seconds (InvulnerabilityDuration) |
| Tag Granted | State.Invulnerable |
| Damage Blocking | NarrativeDamageExecCalc checks for State.Invulnerable tag |

---

## **PHASE 9: REACTIVATE FATHER FUNCTION**

### **35) Create ReactivateFather Function**

#### 35.1) Create Function
   - 35.1.1) In My Blueprint panel, find **Functions** section
   - 35.1.2) Click **+** button next to Functions
   - 35.1.3) Name: `ReactivateFather`
   - 35.1.4) Double-click to open function graph

### **35A) Validate FatherRef**

#### 35A.1) Check FatherRef Is Valid
   - 35A.1.1) From function entry execution pin:
      - 35A.1.1.1) Drag **FatherRef** variable getter into graph
      - 35A.1.1.2) Add **Is Valid** node
      - 35A.1.1.3) Add **Branch** node
      - 35A.1.1.4) Connect Is Valid to Branch Condition
   - 35A.1.2) From Branch **False** pin:
      - 35A.1.2.1) Add **Return Node** (early exit)

### **36) Remove Offline Effect**

#### 36.1) Get Father ASC
   - 36.1.1) From Branch **True** execution pin:
      - 36.1.1.1) From FatherRef, add **Get Ability System Component** node

#### 36.2) Validate OfflineEffectHandle
   - 36.2.1) Drag **OfflineEffectHandle** variable getter into graph
   - 36.2.2) Add **Is Valid** node
   - 36.2.3) Add **Branch** node

#### 36.3) Remove Active Gameplay Effect
   - 36.3.1) From Branch **True**:
      - 36.3.1.1) From Father ASC, add **Remove Active Gameplay Effect** node
      - 36.3.1.2) Connect **OfflineEffectHandle** to **Handle** input
      - 36.3.1.3) **Stacks To Remove**: `-1`

### **37) Play Reactivation Animation**

#### 37.1) Play Montage
   - 37.1.1) From **Remove Active Gameplay Effect** execution pin (or Branch False):
      - 37.1.1.1) Drag outward and search: `Play Anim Montage`
      - 37.1.1.2) Add **Play Anim Montage** node
   - 37.1.2) Connect **FatherRef** to Target input
   - 37.1.3) Set **Montage to Play**: `AM_FatherReactivate` (placeholder)

### **38) Activate Armor Form**

#### 38.1) Get Father ASC
   - 38.1.1) From **Play Anim Montage** execution pin:
      - 38.1.1.1) From FatherRef, add **Get Ability System Component** node

#### 38.2) Try Activate Ability
   - 38.2.1) From Father ASC:
      - 38.2.1.1) Drag outward and search: `Try Activate Ability by Class`
      - 38.2.1.2) Add **Try Activate Ability by Class** node
   - 38.2.2) Set **Ability Class**: `GA_FatherArmor`

### **39) Restart Health Monitoring**

#### 39.1) Set IsMonitoring Flag
   - 39.1.1) From **Try Activate Ability** execution pin:
      - 39.1.1.1) Drag outward and search: `Set IsMonitoring`
      - 39.1.1.2) Add **Set IsMonitoring** node
   - 39.1.2) Set value: Checked (true)

---

## **PHASE 10: IMPLEMENT ENDABILITY EVENT**

### **40) Add Event OnEndAbility**

#### 40.1) Add Event Node
   - 40.1.1) In Event Graph, right-click
   - 40.1.2) Search: `Event On End Ability`
   - 40.1.3) Add **Event OnEndAbility** node

### **41) Clear Dormant Timer**

#### 41.1) Clear Timer by Handle
   - 41.1.1) From **Event OnEndAbility** execution pin:
      - 41.1.1.1) Add **Clear Timer by Handle** node
      - 41.1.1.2) Connect **DormantTimerHandle** variable to **Handle** input

### **42) Validate FatherRef for Cleanup**

#### 42.1) Check FatherRef
   - 42.1.1) From **Clear Timer** execution pin:
      - 42.1.1.1) Drag **FatherRef** variable getter into graph
      - 42.1.1.2) Add **Is Valid** node
      - 42.1.1.3) Add **Branch** node

### **43) Remove GE_FatherOffline if Not Already Removed**

#### 43.1) Validate OfflineEffectHandle
   - 43.1.1) From Branch **True** execution pin:
      - 43.1.1.1) Drag **OfflineEffectHandle** variable getter into graph
      - 43.1.1.2) Add **Is Valid** node
      - 43.1.1.3) Add **Branch** node

#### 43.2) Remove Effect
   - 43.2.1) From inner Branch **True** execution:
      - 43.2.1.1) From FatherRef, add **Get Ability System Component** node
      - 43.2.1.2) Add **Remove Active Gameplay Effect** node
      - 43.2.1.3) Handle: **OfflineEffectHandle**
      - 43.2.1.4) Stacks To Remove: `-1`

### **44) Unbind Health Delegate**

#### 44.1) Validate PlayerASC
   - 44.1.1) From Remove Effect execution (or inner Branch False):
      - 44.1.1.1) Drag **PlayerASC** variable getter into graph
      - 44.1.1.2) Add **Is Valid** node
      - 44.1.1.3) Add **Branch** node

#### 44.2) Get Health Attribute Delegate
   - 44.2.1) From Branch **True** execution:
      - 44.2.1.1) From PlayerASC, add **Get Gameplay Attribute Value Change Delegate** node
      - 44.2.1.2) Attribute: `Health`

#### 44.3) Remove Delegate Binding
   - 44.3.1) From Get Delegate:
      - 44.3.1.1) Add **Remove** node (unbind delegate)
      - 44.3.1.2) Connect **HealthChangeDelegateHandle** to handle input

### **45) EndAbility Cleanup Flow Summary**

| Step | Action |
|------|--------|
| 1 | Event OnEndAbility fires |
| 2 | Clear Timer by Handle (DormantTimerHandle) |
| 3 | Validate FatherRef |
| 4 | Validate OfflineEffectHandle |
| 5 | Remove GE_FatherOffline using OfflineEffectHandle |
| 6 | Validate PlayerASC |
| 7 | Unbind Health Change Delegate using HealthChangeDelegateHandle |

---

## **PHASE 11: GAMEPLAY EFFECTS**

### **46) Create GE_SacrificeInvulnerability**

#### 46.1) Create Gameplay Effect Asset
   - 46.1.1) In Content Browser, navigate to: `/Content/FatherCompanion/Effects/`
   - 46.1.2) Right-click in empty space
   - 46.1.3) Select **Gameplay** -> **Gameplay Effect**
   - 46.1.4) Name: `GE_SacrificeInvulnerability`
   - 46.1.5) Double-click to open

#### 46.2) Configure Duration Policy
   - 46.2.1) In Details panel, find **Duration Policy**
   - 46.2.2) Set to: `Has Duration`
   - 46.2.3) Find **Duration Magnitude** section
   - 46.2.4) Expand **Scalable Float Magnitude**
   - 46.2.5) Set **Value**: `10.0` (10 seconds)

#### 46.3) Add Grant Tags Component
   - 46.3.1) Find **Components** section in Details panel
   - 46.3.2) Click **+** next to Components array
   - 46.3.3) Select: **Grant Tags to Target Actor (on GE)**
   - 46.3.4) Expand component
   - 46.3.5) Find **Add to Inherited** array
   - 46.3.6) Click **+** to add element
   - 46.3.7) Select tag: `State.Invulnerable`

#### 46.4) Compile and Save
   - 46.4.1) Click **Compile**
   - 46.4.2) Click **Save**

### **47) Create GE_FatherOffline**

#### 47.1) Create Gameplay Effect Asset
   - 47.1.1) In Content Browser, navigate to: `/Content/FatherCompanion/Effects/`
   - 47.1.2) Right-click in empty space
   - 47.1.3) Select **Gameplay** -> **Gameplay Effect**
   - 47.1.4) Name: `GE_FatherOffline`
   - 47.1.5) Double-click to open

#### 47.2) Configure Duration Policy
   - 47.2.1) In Details panel, find **Duration Policy**
   - 47.2.2) Set to: `Infinite`

#### 47.3) Add Grant Tags Component
   - 47.3.1) Find **Components** section in Details panel
   - 47.3.2) Click **+** next to Components array
   - 47.3.3) Select: **Grant Tags to Target Actor (on GE)**
   - 47.3.4) Expand component
   - 47.3.5) Find **Add to Inherited** array
   - 47.3.6) Click **+** to add element
   - 47.3.7) Select tag: `Father.State.Offline`
   - 47.3.8) Click **+** to add second element
   - 47.3.9) Select tag: `Cooldown.Father.Sacrifice`

#### 47.4) Compile and Save
   - 47.4.1) Click **Compile**
   - 47.4.2) Click **Save**

---

## **PHASE 12: INTEGRATION**

### **48) Add to Ability Configuration**

#### 48.1) Open AC_FatherCompanion_Default
   - 48.1.1) In Content Browser, navigate to: `/Content/FatherCompanion/Configurations/`
   - 48.1.2) Locate **AC_FatherCompanion_Default** asset
   - 48.1.3) Double-click to open

#### 48.2) Add Sacrifice Ability
   - 48.2.1) In Details panel, find **Abilities** array
   - 48.2.2) Click **+** to add element
   - 48.2.3) Click dropdown on new element
   - 48.2.4) Select: `GA_FatherSacrifice`

#### 48.3) Save Configuration
   - 48.3.1) Click **Save**

### **49) Compile and Save All**

#### 49.1) Save GA_FatherSacrifice
   - 49.1.1) Open **GA_FatherSacrifice** blueprint
   - 49.1.2) Click **Compile** button
   - 49.1.3) Verify no compilation errors
   - 49.1.4) Click **Save** button

#### 49.2) Save All Assets
   - 49.2.1) In top menu, click **File**
   - 49.2.2) Select **Save All**

---

## QUICK REFERENCE

### Tag Configuration Summary

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Sacrifice |
| Activation Required Tags | Father.State.Alive, Father.State.Recruited |
| Activation Owned Tags | Father.State.Sacrificing |
| Activation Blocked Tags | Cooldown.Father.Sacrifice, Father.State.Offline, Narrative.State.IsDead |
| Cancel Abilities with Tag | Ability.Father.Crawler, Armor, Exoskeleton, Symbiote, Engineer |

### Variable Summary Table

| Variable | Type | Instance Editable | Default | Category |
|----------|------|-------------------|---------|----------|
| PlayerRef | Actor Ref | Unchecked | None | Runtime |
| FatherRef | BP_FatherCompanion Ref | Unchecked | None | Runtime |
| PlayerASC | NarrativeASC Ref | Unchecked | None | Runtime |
| HealthThreshold | Float | Checked | 0.15 | Configuration |
| InvulnerabilityDuration | Float | Checked | 10.0 | Configuration |
| DormantDuration | Float | Checked | 180.0 | Configuration |
| OfflineEffectHandle | Active GE Handle | Unchecked | None | Runtime |
| DormantTimerHandle | Timer Handle | Unchecked | None | Runtime |
| IsMonitoring | Boolean | Unchecked | false | Runtime |
| HealthChangeDelegateHandle | FDelegateHandle | Unchecked | None | Runtime |

### Handle Variables

| Variable | Type | Purpose |
|----------|------|---------|
| OfflineEffectHandle | ActiveGameplayEffectHandle | GE_FatherOffline removal on father |
| DormantTimerHandle | TimerHandle | Dormant timer cleanup |
| HealthChangeDelegateHandle | FDelegateHandle | Health delegate unbinding |

### Gameplay Tags Summary

| Tag | Purpose |
|-----|---------|
| Ability.Father.Sacrifice | Ability identification tag |
| Father.State.Alive | Required for activation |
| Father.State.Recruited | Required for activation |
| Father.State.Offline | Father is dormant after sacrifice |
| Father.State.Sacrificing | Sacrifice sequence in progress |
| Cooldown.Father.Sacrifice | Prevents re-triggering during dormant |
| State.Invulnerable | Player immune to all damage |

### Gameplay Effects Summary

| Effect | Target | Duration | Purpose |
|--------|--------|----------|---------|
| GE_SacrificeInvulnerability | Player | 10 seconds | Grants State.Invulnerable for damage immunity |
| GE_FatherOffline | Father | Infinite | Grants Father.State.Offline and Cooldown tags |

### Required Assets Summary

| Asset | Type | Purpose |
|-------|------|---------|
| GA_FatherSacrifice | Blueprint (NarrativeGameplayAbility) | Main ability blueprint |
| GE_SacrificeInvulnerability | Gameplay Effect | 8-second invulnerability |
| GE_FatherOffline | Gameplay Effect | Father dormant state tags |
| AM_FatherSacrifice | Animation Montage | Sacrifice animation (placeholder) |
| AM_FatherReactivate | Animation Montage | Reactivation animation (placeholder) |
| EFatherForm | Enumeration | Must include Offline value |
| AC_FatherCompanion_Default | AbilityConfiguration | Ability granting |

### EndAbility Cleanup Flow

| Step | Action |
|------|--------|
| 1 | Event OnEndAbility fires |
| 2 | Clear Timer by Handle (DormantTimerHandle) |
| 3 | Validate FatherRef |
| 4 | Validate OfflineEffectHandle |
| 5 | Remove GE_FatherOffline using OfflineEffectHandle |
| 6 | Validate PlayerASC |
| 7 | Unbind Health Change Delegate |

### Related Documents

| Document | Purpose |
|----------|---------|
| Father_Companion_System_Setup_Guide_v1_3.md | Father setup with OwnerPlayer reference |
| Father_Companion_Technical_Reference_v4_5.md | Section 14.8: Effect Handle Storage, Section 15: Cleanup Patterns, Section 37: Recruited Tag Gating |
| DefaultGameplayTags_FatherCompanion_v3_5.ini | All gameplay tags including Father.State.Recruited |
| GA_FatherArmor_Implementation_Guide_v3_4.md | Armor form for reactivation |

### Future Enhancements

| Feature | Requirement |
|---------|-------------|
| Attack Token Modification | Requires C++ modification to NarrativeAbilitySystemComponent.GetNumAttackTokens() or Narrative Pro plugin update |
| Visual Dormant State | VFX/material changes for dormant father appearance |

---

## END OF GA_FATHERSACRIFICE IMPLEMENTATION GUIDE VERSION 2.5

Blueprint-Only Implementation for Unreal Engine 5.6 + Narrative Pro Plugin v2.2
