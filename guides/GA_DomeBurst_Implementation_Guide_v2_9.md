# Father Companion - GA_DomeBurst Implementation Guide
## VERSION 2.9 - Multi-Document Audit Corrections
## Unreal Engine 5.7 + Narrative Pro Plugin v2.2

---

## **AUTHORITY NOTE**

This guide follows values defined in **manifest.yaml** (single source of truth). If any discrepancy exists between this guide and manifest.yaml, the manifest takes precedence.

**Key references:**
- `manifest.yaml` lines 4061-4100: GA_DomeBurst definition
- `manifest.yaml` lines 626-631: GE_DomeBurstCooldown definition

---

## **DOCUMENT INFORMATION**

| Field | Value |
|-------|-------|
| Ability Name | GA_DomeBurst |
| Ability Type | Active (Q Key) / Auto (Threshold) |
| Parent Class | NarrativeGameplayAbility |
| Form | Armor |
| Input | Q Key (Manual) or Auto (When Dome Full) |
| Version | 2.9 |
| Last Updated | January 2026 |

---

## **AUDIT STATUS**

| Field | Value |
|-------|-------|
| Last Audit Date | 2026-01-23 |
| Audit Type | Claude-GPT Dual Audit |
| Manifest Alignment | Verified |
| INC Items Fixed | INC-A through INC-J (10 items) |

---

## **TABLE OF CONTENTS**

1. [Introduction](#introduction)
2. [Prerequisites](#prerequisites)
3. [PHASE 1: Gameplay Tags Setup](#phase-1-gameplay-tags-setup)
4. [PHASE 2: Create Gameplay Effects](#phase-2-create-gameplay-effects)
5. [PHASE 3: Create GA_DomeBurst Ability](#phase-3-create-ga_domeburst-ability)
6. [PHASE 4: Implement Manual Burst Logic](#phase-4-implement-manual-burst-logic)
7. [PHASE 5: Implement Auto-Burst Integration](#phase-5-implement-auto-burst-integration)
8. [PHASE 6: Ability Granting via Equipment](#phase-6-ability-granting-via-equipment)
9. [PHASE 7: Input Configuration](#phase-7-input-configuration)
10. [Changelog](#changelog)
11. [Quick Reference](#quick-reference)

---

## **INTRODUCTION**

### **Ability Overview**

GA_DomeBurst is the explosive payoff ability for the Armor form's Protective Dome system. When the father is attached as armor, it absorbs incoming damage and stores it as Dome Energy. GA_DomeBurst triggers in two ways: automatically when the dome reaches maximum energy (500 absorbed), or manually when the player presses Q to release early. The burst deals flat AOE damage and knocks back all nearby enemies.

### **Key Features**

| Feature | Description |
|---------|-------------|
| Dual Activation | Manual (Q key) or Automatic (threshold) |
| Flat Damage | 75 damage to all enemies in radius |
| AOE Explosion | 500 unit radius centered on player |
| Knockback Effect | Pushes enemies away from player |
| Cooldown System | 12 seconds after burst before recharge |
| Form Specific | Only available in Armor form (via equipment granting) |
| NarrativeDamageExecCalc | Proper damage pipeline with armor/attack rating |

### **Activation Modes**

| Mode | Trigger | Energy Required |
|------|---------|-----------------|
| Manual | Q Key Press | Any amount (min 50) |
| Automatic | Threshold Reached | 500 (maximum) |

### **Dome System Flow**

| Step | Description |
|------|-------------|
| 1 | Armor Form Active |
| 2 | GA_DomeManager Running |
| 3 | Player Takes Damage |
| 4 | 30% Absorbed -> Dome Energy |
| 5a | If Energy < 500: Continue Charging |
| 5b | If Energy = 500: Auto-Burst (GA_DomeBurst) |
| 6 | If Player Presses Q with 50+ Energy: Manual Burst |

### **Technical Specifications**

| Parameter | Value |
|-----------|-------|
| Burst Damage | 75 (flat) |
| Burst Radius | 500 units |
| Knockback Force | 1000 units |
| Cooldown | 12 seconds |
| Min Energy for Manual | 50 |

### **Damage Pipeline**

GA_DomeBurst uses NarrativeDamageExecCalc for proper damage application:

| Feature | Behavior |
|---------|----------|
| AttackRating | Player's AttackRating multiplies burst damage |
| Armor | Enemy armor reduces damage received |
| State.Invulnerable | Automatically blocks damage to invulnerable targets |
| Friendly Fire | Respects combat settings for friendly fire |

---

## **PREREQUISITES**

### **Required Before This Guide**

| Requirement | Description |
|-------------|-------------|
| BP_FatherCompanion | Father with NarrativeNPCCharacter parent |
| EI_FatherArmorForm | Armor form equipment item (grants GA_DomeBurst) |
| GA_ProtectiveDome | Dome activation ability (grants Father.Dome.Active) |
| AS_DomeAttributes | Dome attributes (DomeEnergy, MaxDomeEnergy) |
| Player ASC | Player has NarrativeAbilitySystemComponent |

### **Required Attributes (via Gameplay Blueprint Attributes Plugin)**

| Attribute | Type | Default | Purpose |
|-----------|------|---------|---------|
| DomeEnergy | Float | 0 | Current absorbed energy |
| MaxDomeEnergy | Float | 500 | Maximum energy capacity |

### **Existing Tags Required**

| Tag | Purpose |
|-----|---------|
| Father.Dome.Active | Dome system running (on Player ASC, granted by GA_ProtectiveDome) |

**Note:** Form identity validation is handled by equipment granting - GA_DomeBurst is only granted when EI_FatherArmorForm is equipped.

---

## **PHASE 1: GAMEPLAY TAGS SETUP**

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Father.Dome.FullyCharged | Dome at maximum energy, ready for auto-burst |
| Father.Dome.OnCooldown | Dome burst on cooldown, cannot charge |
| Ability.Father.DomeBurst | Dome AOE explosion ability |
| Father.State.Attacking | Burst ability executing (owned tag during activation) |
| Effect.Father.DomeBurst | Damage from dome burst explosion |
| Effect.Father.DomeBurstKnockback | Knockback from dome burst |
| Cooldown.Father.DomeBurst | Cooldown after dome burst |
| Data.Damage.DomeBurst | SetByCaller tag for dome burst damage |

---

## **PHASE 2: CREATE GAMEPLAY EFFECTS**

### **1) Navigate to Effects Folder**

#### 1.1) Open Effects Folder
   - 1.1.1) In Content Browser, navigate to `/Content/FatherCompanion/ProtectiveDome/Effects/`
   - 1.1.2) Create the folder structure if needed

### **2) Create GE_DomeBurstDamage**

#### 2.1) Create Gameplay Effect Asset
   - 2.1.1) Right-click in Effects folder
   - 2.1.2) Select Gameplay -> Gameplay Effect
   - 2.1.3) Name: `GE_DomeBurstDamage`
   - 2.1.4) Double-click to open

#### 2.2) Configure GE_DomeBurstDamage Properties

| Property | Value |
|----------|-------|
| Duration Policy | Instant |
| Components | Executions |
| Calculation Class | NarrativeDamageExecCalc |
| Calculation Modifiers [0] | |
| - Backing Capture Definition -> Attribute to Capture | AttackDamage |
| - Backing Capture Definition -> Attribute Source | Source |
| - Modifier Op | Override |
| - Modifier Magnitude -> Magnitude Calculation Type | Set By Caller |
| - Set By Caller Magnitude -> Data Tag | Data.Damage.DomeBurst |
| Asset Tags -> Add to Inherited [0] | Effect.Father.DomeBurst |

#### 2.3) Compile and Save

### **3) Create GE_DomeBurstCooldown**

#### 3.1) Create Gameplay Effect Asset
   - 3.1.1) Right-click in Effects folder
   - 3.1.2) Select Gameplay -> Gameplay Effect
   - 3.1.3) Name: `GE_DomeBurstCooldown`
   - 3.1.4) Double-click to open

#### 3.2) Configure GE_DomeBurstCooldown Properties

| Property | Value |
|----------|-------|
| Duration Policy | Has Duration |
| Duration Magnitude -> Magnitude Calculation Type | Scalable Float |
| Duration Magnitude -> Scalable Float Magnitude -> Value | 12.0 |
| Components | Grant Tags to Target Actor |
| Add Tags -> Add to Inherited [0] | Cooldown.Father.DomeBurst |
| Add Tags -> Add to Inherited [1] | Father.Dome.OnCooldown |

#### 3.3) Compile and Save

### **4) Create GE_DomeEnergyReset**

#### 4.1) Create Gameplay Effect Asset
   - 4.1.1) Right-click in Effects folder
   - 4.1.2) Select Gameplay -> Gameplay Effect
   - 4.1.3) Name: `GE_DomeEnergyReset`
   - 4.1.4) Double-click to open

#### 4.2) Configure GE_DomeEnergyReset Properties

| Property | Value |
|----------|-------|
| Duration Policy | Instant |
| Components | Modifiers |
| Modifiers [0] -> Attribute | AS_DomeAttributes.DomeEnergy |
| Modifiers [0] -> Modifier Op | Override |
| Modifiers [0] -> Modifier Magnitude -> Magnitude Calculation Type | Scalable Float |
| Modifiers [0] -> Modifier Magnitude -> Scalable Float Magnitude -> Value | 0.0 |

#### 4.3) Compile and Save

---

## **PHASE 3: CREATE GA_DOMEBURST ABILITY**

### **1) Navigate to Abilities Folder**

#### 1.1) Open Abilities Folder
   - 1.1.1) In Content Browser, navigate to `/Content/FatherCompanion/ProtectiveDome/Abilities/`

### **2) Create GA_DomeBurst Blueprint**

#### 2.1) Create Blueprint Class
   - 2.1.1) Right-click in Abilities folder
   - 2.1.2) Select Blueprint Class
   - 2.1.3) In Pick Parent Class dialog:
      - 2.1.3.1) Click All Classes dropdown
      - 2.1.3.2) Search: `NarrativeGameplayAbility`
      - 2.1.3.3) Select NarrativeGameplayAbility
      - 2.1.3.4) Click Select
   - 2.1.4) Name: `GA_DomeBurst`
   - 2.1.5) Press Enter
   - 2.1.6) Double-click to open

### **3) Configure Class Defaults**

#### 3.1) Open Class Defaults Panel
   - 3.1.1) Click Class Defaults button in toolbar

### **4) Configure Ability Tags**

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.DomeBurst |
| Activation Required Tags | Father.Dome.Active |
| Activation Owned Tags | Father.State.Attacking |
| Activation Blocked Tags | Cooldown.Father.DomeBurst |

### **5) Configure Replication Settings**

#### 5.1) Set Instancing Policy
   - 5.1.1) In Details panel, find Ability category
   - 5.1.2) Find Instancing Policy dropdown
   - 5.1.3) Select: `Instanced Per Actor`

#### 5.2) Set Replication Policy
   - 5.2.1) Find Replication Policy dropdown
   - 5.2.2) Select: `Replicate`

#### 5.3) Set Net Execution Policy
   - 5.3.1) Find Net Execution Policy dropdown
   - 5.3.2) Select: `Local Predicted`

### **6) Configure InputTag**

#### 6.1) Set Input Tag for Q Key
   - 6.1.1) In Details panel, find Narrative Ability category
   - 6.1.2) Find Input Tag property
   - 6.1.3) Click dropdown or tag picker
   - 6.1.4) Search: `Narrative.Input.Father.Ability1`
   - 6.1.5) Select the tag

### **7) Compile and Save**

#### 7.1) Save Configuration
   - 7.1.1) Click Compile button
   - 7.1.2) Click Save button

---

## **PHASE 4: IMPLEMENT MANUAL BURST LOGIC**

### **1) Create Ability Variables**

#### 1.1) Open Event Graph
   - 1.1.1) In GA_DomeBurst Blueprint, click Event Graph tab

#### 1.2) Create Variables

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| BurstDamage | Float | 75.0 | Yes |
| BurstRadius | Float | 500.0 | Yes |
| KnockbackForce | Float | 1000.0 | Yes |
| MinEnergyForManual | Float | 50.0 | Yes |
| PlayerRef | Actor (Object Reference) | None | No |
| PlayerASC | NarrativeAbilitySystemComponent (Object Reference) | None | No |

### **2) Implement ActivateAbility Event**

#### 2.1) Add Event ActivateAbility
   - 2.1.1) Right-click in Event Graph
   - 2.1.2) Search: `Event ActivateAbility`
   - 2.1.3) Add Event ActivateAbility node

#### 2.2) Get Avatar Actor (Player)
   - 2.2.1) From Event ActivateAbility node:
      - 2.2.1.1) Find Actor Info pin
      - 2.2.1.2) Drag outward and search: `Get Avatar Actor from Actor Info`
      - 2.2.1.3) Add Get Avatar Actor from Actor Info node

#### 2.3) Store Player Reference
   - 2.3.1) From Event ActivateAbility execution pin:
      - 2.3.1.1) Add Set PlayerRef node
   - 2.3.2) Connect Return Value from Get Avatar Actor to PlayerRef value pin

#### 2.4) Validate Player Reference
   - 2.4.1) From Set PlayerRef execution pin:
      - 2.4.1.1) Drag from PlayerRef output pin
      - 2.4.1.2) Add Is Valid node
   - 2.4.2) From Is Valid node:
      - 2.4.2.1) Add Branch node
      - 2.4.2.2) Connect Is Valid Return Value to Condition pin
   - 2.4.3) From Branch False execution pin:
      - 2.4.3.1) Add End Ability node
      - 2.4.3.2) Was Cancelled: Checked (true)

### **3) Get and Validate Player ASC**

#### 3.1) Get Player Ability System Component
   - 3.1.1) From Branch True execution pin:
      - 3.1.1.1) Drag from PlayerRef variable
      - 3.1.1.2) Search: `Get Ability System Component`
      - 3.1.1.3) Add Get Ability System Component node

#### 3.2) Cast to NarrativeAbilitySystemComponent
   - 3.2.1) From Get Ability System Component Return Value:
      - 3.2.1.1) Drag outward and search: `Cast To NarrativeAbilitySystemComponent`
      - 3.2.1.2) Add Cast To NarrativeAbilitySystemComponent node
   - 3.2.2) Connect execution from Branch True pin

#### 3.3) Store PlayerASC
   - 3.3.1) From Cast succeeded execution pin:
      - 3.3.1.1) Add Set PlayerASC node
   - 3.3.2) Connect As Narrative Ability System Component to PlayerASC value pin

#### 3.4) Handle Cast Failure
   - 3.4.1) From Cast failed execution pin:
      - 3.4.1.1) Add End Ability node
      - 3.4.1.2) Was Cancelled: Checked (true)

#### 3.5) Validate PlayerASC
   - 3.5.1) From Set PlayerASC execution pin:
      - 3.5.1.1) Drag from PlayerASC output pin
      - 3.5.1.2) Add Is Valid node
   - 3.5.2) From Is Valid node:
      - 3.5.2.1) Add Branch node
      - 3.5.2.2) Connect Is Valid Return Value to Condition pin
   - 3.5.3) From Branch False execution pin:
      - 3.5.3.1) Add End Ability node
      - 3.5.3.2) Was Cancelled: Checked (true)

### **4) Get Current Dome Energy**

#### 4.1) Get Dome Energy Attribute
   - 4.1.1) From Branch True execution pin (PlayerASC validation):
      - 4.1.1.1) Drag from PlayerASC variable
      - 4.1.1.2) Search: `Get Numeric Attribute`
      - 4.1.1.3) Add Get Numeric Attribute node
   - 4.1.2) Configure:
      - 4.1.2.1) Attribute: `AS_DomeAttributes.DomeEnergy`

#### 4.2) Promote to Local Variable
   - 4.2.1) From Get Numeric Attribute Current Value pin:
      - 4.2.1.1) Right-click and select Promote to Local Variable
      - 4.2.1.2) Name: `CurrentDomeEnergy`

### **5) Validate Minimum Energy (Manual Burst)**

#### 5.1) Check Minimum Energy
   - 5.1.1) Add Float >= Float (Greater or Equal) node
   - 5.1.2) Connect:
      - 5.1.2.1) CurrentDomeEnergy getter to first input (A)
      - 5.1.2.2) MinEnergyForManual getter to second input (B)

#### 5.2) Branch on Energy Check
   - 5.2.1) From Get Numeric Attribute execution pin:
      - 5.2.1.1) Add Branch node
   - 5.2.2) Connect comparison Return Value to Condition pin
   - 5.2.3) From Branch False execution pin:
      - 5.2.3.1) Add End Ability node
      - 5.2.3.2) Was Cancelled: Checked (true)

### **6) Find Enemies in Radius**

#### 6.1) Get Player Location
   - 6.1.1) From PlayerRef getter:
      - 6.1.1.1) Drag outward and search: `Get Actor Location`
      - 6.1.1.2) Add Get Actor Location node

#### 6.2) Sphere Overlap for Actors
   - 6.2.1) From Branch True execution pin:
      - 6.2.1.1) Drag outward and search: `Sphere Overlap Actors`
      - 6.2.1.2) Add Sphere Overlap Actors node
   - 6.2.2) Connect execution pin
   - 6.2.3) Configure:
      - 6.2.3.1) **World Context Object**: Right-click on pin, select **Get a Reference to Self**
      - 6.2.3.2) **Sphere Pos**: Connect Get Actor Location Return Value
      - 6.2.3.3) **Sphere Radius**: Connect BurstRadius getter
      - 6.2.3.4) **Object Types**: Add `Pawn`
      - 6.2.3.5) **Actors to Ignore**: Create Make Array with PlayerRef

### **7) Apply Damage to Each Enemy**

#### 7.1) Create ForEach Loop
   - 7.1.1) From Sphere Overlap Actors execution pin:
      - 7.1.1.1) Drag outward and search: `For Each Loop`
      - 7.1.1.2) Add For Each Loop node
   - 7.1.2) Connect Out Actors pin to ForEach Array pin

#### 7.2) Filter Enemies Only
   - 7.2.1) From ForEach Loop Body execution pin:
      - 7.2.1.1) From Array Element pin, drag outward
      - 7.2.1.2) Search: `Get Ability System Component`
      - 7.2.1.3) Add Get Ability System Component node
   - 7.2.2) From enemy ASC Return Value:
      - 7.2.2.1) Drag outward and search: `Has Matching Gameplay Tag`
      - 7.2.2.2) Add Has Matching Gameplay Tag node
   - 7.2.3) Configure:
      - 7.2.3.1) Tag to Check: `Character.Enemy`

#### 7.3) Branch on Enemy Check
   - 7.3.1) Add Branch node
   - 7.3.2) Connect Has Matching Gameplay Tag Return Value to Condition pin
   - 7.3.3) From Branch False execution pin: Leave unconnected (continues to next iteration)

#### 7.4) Create Damage Effect Spec
   - 7.4.1) From Branch True execution pin:
      - 7.4.1.1) Drag from PlayerASC variable
      - 7.4.1.2) Search: `Make Outgoing Spec`
      - 7.4.1.3) Add Make Outgoing Spec node
   - 7.4.2) Configure:
      - 7.4.2.1) Gameplay Effect Class: `GE_DomeBurstDamage`
      - 7.4.2.2) Level: `1.0`

#### 7.5) Assign Set By Caller Magnitude
   - 7.5.1) From Make Outgoing Spec Return Value pin:
      - 7.5.1.1) Drag outward and search: `Assign Tag Set By Caller Magnitude`
      - 7.5.1.2) Add Assign Tag Set By Caller Magnitude node
   - 7.5.2) Configure:
      - 7.5.2.1) Data Tag: `Data.Damage.DomeBurst`
      - 7.5.2.2) Magnitude: Connect BurstDamage getter (flat 75)

#### 7.6) Apply Effect to Enemy
   - 7.6.1) From Assign Tag Set By Caller Magnitude execution pin:
      - 7.6.1.1) Drag outward and search: `Apply Gameplay Effect Spec to Target`
      - 7.6.1.2) Add Apply Gameplay Effect Spec to Target node
   - 7.6.2) Connect:
      - 7.6.2.1) Execution from Assign Tag node
      - 7.6.2.2) **Target** pin: Connect enemy ASC (from Get Ability System Component in step 7.2.1)
      - 7.6.2.3) **Spec Handle** pin: Connect Assign Tag Return Value

### **8) Apply Knockback**

#### 8.1) Calculate Knockback Direction
   - 8.1.1) From Array Element (enemy):
      - 8.1.1.1) Drag outward and search: `Get Actor Location`
      - 8.1.1.2) Add Get Actor Location node
   - 8.1.2) Add Vector - Vector (Subtract) node
   - 8.1.3) Connect:
      - 8.1.3.1) Enemy Location to A pin
      - 8.1.3.2) Player Location to B pin

#### 8.2) Normalize Direction
   - 8.2.1) From Subtract Return Value:
      - 8.2.1.1) Drag outward and search: `Normalize`
      - 8.2.1.2) Add Normalize node

#### 8.3) Scale by Knockback Force
   - 8.3.1) Add Vector * Float (Multiply) node
   - 8.3.2) Connect:
      - 8.3.2.1) Normalize Return Value to Vector pin
      - 8.3.2.2) KnockbackForce getter to Float pin

#### 8.4) Cast to Character (Required)
   - 8.4.1) From Array Element:
      - 8.4.1.1) Drag outward and search: `Cast To Character`
      - 8.4.1.2) Add Cast To Character node
   - 8.4.2) Array Element is AActor* but Launch Character requires ACharacter*

#### 8.5) Launch Character
   - 8.5.1) From Apply Gameplay Effect Spec execution pin:
      - 8.5.1.1) Connect to Cast To Character execution input
   - 8.5.2) From Cast success execution pin:
      - 8.5.2.1) From **As Character** pin, drag outward
      - 8.5.2.2) Search: `Launch Character`
      - 8.5.2.3) Add Launch Character node
   - 8.5.3) Configure:
      - 8.5.3.1) **Target**: Connect **As Character** pin from Cast node
      - 8.5.3.2) **Launch Velocity**: Connect scaled knockback vector (Multiply Return Value)
      - 8.5.3.3) **XY Override**: Unchecked
      - 8.5.3.4) **Z Override**: Unchecked

### **9) Spawn Burst Visual Effect**

#### 9.1) Spawn Niagara System
   - 9.1.1) From ForEach Loop Completed execution pin:
      - 9.1.1.1) Drag outward and search: `Spawn System at Location`
      - 9.1.1.2) Add Spawn System at Location node
   - 9.1.2) Connect execution pin
   - 9.1.3) Configure:
      - 9.1.3.1) System Template: Select `NS_DomeBurst`
      - 9.1.3.2) Location: Connect Player Location (Get Actor Location Return Value)
      - 9.1.3.3) Auto Destroy: Checked

### **10) Reset Dome Energy**

#### 10.1) Apply Energy Reset Effect
   - 10.1.1) From Spawn System at Location execution pin:
      - 10.1.1.1) Drag from PlayerASC variable
      - 10.1.1.2) Search: `Apply Gameplay Effect to Self`
      - 10.1.1.3) Add Apply Gameplay Effect to Self node
   - 10.1.2) Connect execution pin
   - 10.1.3) Configure:
      - 10.1.3.1) Gameplay Effect Class: `GE_DomeEnergyReset`
      - 10.1.3.2) Level: `1.0`

### **11) Apply Cooldown**

#### 11.1) Apply Burst Cooldown Effect
   - 11.1.1) From Apply GE (Reset) execution pin:
      - 11.1.1.1) Drag from PlayerASC variable
      - 11.1.1.2) Search: `Apply Gameplay Effect to Self`
      - 11.1.1.3) Add Apply Gameplay Effect to Self node
   - 11.1.2) Connect execution pin
   - 11.1.3) Configure:
      - 11.1.3.1) Gameplay Effect Class: `GE_DomeBurstCooldown`
      - 11.1.3.2) Level: `1.0`

### **12) End Ability**

#### 12.1) Commit Ability
   - 12.1.1) From Apply GE (Cooldown) execution pin:
      - 12.1.1.1) Drag outward and search: `Commit Ability`
      - 12.1.1.2) Add Commit Ability node
   - 12.1.2) Connect execution pin

#### 12.2) Clear References
   - 12.2.1) From Commit Ability execution pin:
      - 12.2.1.1) Add Set PlayerRef node
      - 12.2.1.2) Leave value pin disconnected (clears to None)
   - 12.2.2) From Set PlayerRef execution pin:
      - 12.2.2.1) Add Set PlayerASC node
      - 12.2.2.2) Leave value pin disconnected (clears to None)

#### 12.3) End Ability
   - 12.3.1) From Set PlayerASC execution pin:
      - 12.3.1.1) Drag outward and search: `End Ability`
      - 12.3.1.2) Add End Ability node
   - 12.3.2) Connect execution pin
   - 12.3.3) Was Cancelled: Unchecked (false)

### **13) Compile and Save**

#### 13.1) Save All Changes
   - 13.1.1) Click Compile button
   - 13.1.2) Click Save button

**Note:** Per GAS Audit Rule 1 (Instant Abilities), Event OnEndAbility is optional for instant abilities. The reference clearing in step 12.2 handles cleanup for the normal execution path. If you want defensive cleanup for edge cases where the ability is externally cancelled, you may optionally add Event OnEndAbility with the same reference-clearing pattern.

---

## **PHASE 5: IMPLEMENT AUTO-BURST INTEGRATION**

### **1) Open GA_ProtectiveDome or GA_DomeManager**

#### 1.1) Navigate to Dome Ability
   - 1.1.1) In Content Browser, navigate to `/Content/FatherCompanion/ProtectiveDome/Abilities/`
   - 1.1.2) Double-click GA_ProtectiveDome (or GA_DomeManager) to open

### **2) Find Energy Monitoring Section**

#### 2.1) Locate Energy Update Logic
   - 2.1.1) In Event Graph, find where DomeEnergy is updated
   - 2.1.2) This should be after damage absorption calculation

### **3) Add Auto-Burst Check**

#### 3.1) Get Max Dome Energy
   - 3.1.1) From Player ASC:
      - 3.1.1.1) Drag outward and search: `Get Numeric Attribute`
      - 3.1.1.2) Add Get Numeric Attribute node
   - 3.1.2) Configure:
      - 3.1.2.1) Attribute: `AS_DomeAttributes.MaxDomeEnergy`

#### 3.2) Compare Current to Max
   - 3.2.1) Add Float >= Float (Greater or Equal) node
   - 3.2.2) Connect:
      - 3.2.2.1) Current DomeEnergy to first input (A)
      - 3.2.2.2) MaxDomeEnergy Return Value to second input (B)

#### 3.3) Branch on Full Energy
   - 3.3.1) After energy update execution:
      - 3.3.1.1) Add Branch node
   - 3.3.2) Connect comparison Return Value to Condition pin
   - 3.3.3) From Branch False execution pin: Continue normal operation

### **4) Trigger Auto-Burst**

#### 4.1) Grant FullyCharged Tag
   - 4.1.1) From Branch True execution pin:
      - 4.1.1.1) From Player ASC, drag outward
      - 4.1.1.2) Search: `Add Loose Gameplay Tag`
      - 4.1.1.3) Add Add Loose Gameplay Tag node
   - 4.1.2) Configure:
      - 4.1.2.1) Gameplay Tag: `Father.Dome.FullyCharged`

#### 4.2) Try Activate Dome Burst
   - 4.2.1) From Add Loose Gameplay Tag execution pin:
      - 4.2.1.1) From Player ASC, drag outward
      - 4.2.1.2) Search: `Try Activate Ability by Tag`
      - 4.2.1.3) Add Try Activate Ability by Tag node
   - 4.2.2) Connect execution pin
   - 4.2.3) Configure:
      - 4.2.3.1) Create Gameplay Tag Container
      - 4.2.3.2) Add tag: `Ability.Father.DomeBurst`

### **5) Compile and Save**

#### 5.1) Save All Changes
   - 5.1.1) Click Compile button
   - 5.1.2) Click Save button

---

## **PHASE 6: ABILITY GRANTING VIA EQUIPMENT**

### **Ability Granting Architecture**

GA_DomeBurst is granted to the player via the equipment system when EI_FatherArmorForm is equipped. This ensures the ability is only available when the Armor form is active.

### **1) EI_FatherArmorForm Configuration**

The EI_FatherArmorForm EquippableItem is configured in manifest.yaml with:

```yaml
equippable_items:
  - name: EI_FatherArmorForm
    abilities_to_grant:
      - GA_FatherArmor
      - GA_ProtectiveDome
      - GA_DomeBurst      # Granted when Armor form equipped
```

### **2) Form Switch Cleanup**

When the player switches forms via the T wheel, the equipment system automatically:
1. Unequips EI_FatherArmorForm
2. Removes granted abilities (including GA_DomeBurst)
3. Equips the new form's equipment item

This automatic cleanup replaces the need for manual handle tracking in GA_FatherArmor's EndAbility.

### **3) Manual Setup (If Not Using Generator)**

If creating assets manually without the generator:

#### 3.1) Open EI_FatherArmorForm
   - 3.1.1) In Content Browser, navigate to `/Content/FatherCompanion/Items/`
   - 3.1.2) Double-click EI_FatherArmorForm to open

#### 3.2) Add Ability to Grant
   - 3.2.1) In Details panel, find Item Properties category
   - 3.2.2) Find Abilities to Grant array
   - 3.2.3) Add entry: `GA_DomeBurst`
   - 3.2.4) Save asset

---

## **PHASE 7: INPUT CONFIGURATION**

### **1) Create Input Action**

#### 1.1) Navigate to Input Folder
   - 1.1.1) In Content Browser, navigate to `/Content/Input/Actions/`

#### 1.2) Create IA_FatherAbility1
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Input -> Input Action
   - 1.2.3) Name: `IA_FatherAbility1`
   - 1.2.4) Press Enter

#### 1.3) Configure Input Action
   - 1.3.1) Double-click to open IA_FatherAbility1
   - 1.3.2) Set Value Type: `Digital (Bool)`
   - 1.3.3) Set Triggers: Add `Pressed` trigger
   - 1.3.4) Save asset

### **2) Add to Input Mapping Context**

#### 2.1) Open Input Mapping Context
   - 2.1.1) Locate your Input Mapping Context asset (IMC_FatherCompanion)
   - 2.1.2) Double-click to open

#### 2.2) Add Q Key Mapping
   - 2.2.1) Add mapping for `IA_FatherAbility1`
   - 2.2.2) Set Key: Q
   - 2.2.3) Save IMC

### **3) Configure Narrative Input Tag Binding**

#### 3.1) Open Narrative Input Settings
   - 3.1.1) Navigate to Narrative Pro settings data asset (DA_FatherInputMapping)

#### 3.2) Add Input Tag Binding
   - 3.2.1) Find Input Tag Bindings array
   - 3.2.2) Add entry:
      - 3.2.2.1) Input Action: `IA_FatherAbility1`
      - 3.2.2.2) Gameplay Tag: `Narrative.Input.Father.Ability1`
   - 3.2.3) Save data asset

### **4) Save All Assets**

#### 4.1) Save Configuration
   - 4.1.1) Save all modified input assets
   - 4.1.2) Save Narrative settings

---

## **CHANGELOG**

### **VERSION 2.9 - Multi-Document Audit Corrections**

**Release Date:** January 2026

| Change Type | Description |
|-------------|-------------|
| INC-A | Changed ability tag from `Ability.Father.Armor.DomeBurst` to `Ability.Father.DomeBurst` (manifest alignment) |
| INC-B | Changed activation owned tag from `Father.State.Bursting` to `Father.State.Attacking` (manifest alignment) |
| INC-C | Simplified activation required tags to only `Father.Dome.Active` (manifest alignment) |
| INC-D | Changed cooldown tag from `Cooldown.Father.Armor.DomeBurst` to `Cooldown.Father.DomeBurst` (manifest alignment) |
| INC-E | Changed damage from 150+scaling to flat 75 (manifest alignment, Erdem decision) |
| INC-F | Changed cooldown from 20s to 12s (manifest alignment, Erdem decision) |
| INC-G | Added note about optional Event_OnEndAbility per Rule 1 (instant abilities) |
| INC-H | Updated engine version from 5.6 to 5.7 |
| INC-I | Updated version number from 2.8 to 2.9 |
| INC-J | Updated PHASE 6 to reference EI_FatherArmorForm equipment granting instead of GA_FatherArmor |
| Authority Note | Added manifest.yaml as authoritative source |
| Audit Status | Added Claude-GPT dual audit metadata |
| Variables | Removed BaseDamage, DamageScaling, CurrentDomeEnergy, CalculatedDamage; added flat BurstDamage |

---

### **VERSION 2.8 - Form State Tag Update (INV-1 Compliant)**

**Release Date:** January 2026

| Change Type | Description |
|-------------|-------------|
| Form State Tags | Changed `Father.Form.Armor` to `Effect.Father.FormState.Armor` in all Activation Required Tags |
| INV-1 Compliance | Aligned with Father_Companion_GAS_Audit_Locked_Decisions.md form state architecture |
| Tag Architecture | `Father.Form.*` tags are orphan tags; form identity now uses GE-granted `Effect.Father.FormState.*` tags |

---

### **VERSION 2.7 - Blueprint Node Consistency Fixes**

**Release Date:** January 2026

| Change Type | Description |
|-------------|-------------|
| Sphere Overlap | Added World Context Object connection (required pin) |
| Apply GE Spec | Changed "Apply Gameplay Effect Spec to Self" to "Apply Gameplay Effect Spec to Target" for enemy damage application |
| Cast to Character | Made Cast to Character explicitly required before Launch Character (Array Element is AActor*, Launch Character requires ACharacter*) |

---

### **VERSION 2.6 - Reference Updates**

**Release Date:** January 2026

| Change Type | Description |
|-------------|-------------|
| Narrative Pro | Updated version reference from v2.1 to v2.2 |
| Documentation | Ready for Technical Reference v5.13 and Setup Guide v2.3 alignment |

---

### **VERSION 2.5 - Simplified Documentation**

**Release Date:** January 2026

| Change Type | Description |
|-------------|-------------|
| Tag Configuration | PHASE 3 Section 4 - converted to single Property/Tags table |
| Variable Creation | PHASE 4 Section 1 - converted to single Variable/Type/Default table |
| GameplayEffect Config | PHASE 2 - converted step-by-step to Property/Value tables for all 3 GEs |
| Line Reduction | Reduced documentation size while preserving all information |

---

### **VERSION 2.4 - Tag Creation Simplification**

**Release Date:** January 2026

| Change Type | Description |
|-------------|-------------|
| Tag Creation | Simplified PHASE 1 - replaced detailed step-by-step instructions with simple tag list table |

---

### **VERSION 2.3 - v4.5 Architecture Update**

**Release Date:** December 2025

| Change Type | Description |
|-------------|-------------|
| Recruited Tag | Added Father.State.Recruited to Activation Required Tags (PHASE 3, Section 4.2) |
| PlayerASC Variable | Added PlayerASC variable for validation (PHASE 4, Section 1.8) |
| PlayerRef Validation | Enhanced with Branch early exit pattern (PHASE 4, Section 2.4) |
| PlayerASC Validation | Added Cast, Store, Validate pattern (PHASE 4, Section 3) |
| Reference Clearing | Added Set PlayerRef/PlayerASC to None before End Ability (PHASE 4, Section 13.2) |
| Event OnEndAbility | Added for early cancellation cleanup (PHASE 4, Section 14) |
| Handle Storage | Updated PHASE 6 with DomeBurstHandle storage (Section 3.3) |
| Quick Reference | Updated Variable Summary and Tag Configuration tables |

---

### **VERSION 2.2 - v4.0 Architecture Update**

**Release Date:** December 2025

| Change Type | Description |
|-------------|-------------|
| Cleanup Pattern | Updated to EndAbility pattern (GA_FatherDetach removed from system) |
| Documentation | PHASE 6 Section 5 cleanup note updated |

---

### **VERSION 2.1 - Hierarchical Tags**

**Release Date:** December 2025

| Change Type | Description |
|-------------|-------------|
| Ability Tag | Changed Ability.Father.DomeBurst to Ability.Father.Armor.DomeBurst |
| Cooldown Tag | Changed Cooldown.Father.DomeBurst to Cooldown.Father.Armor.DomeBurst |
| Tag Hierarchy | Enables automatic cancellation when Armor form ends |

**Note:** v2.9 reverts these tags back to flat hierarchy per manifest.yaml alignment.

### **VERSION 2.0 - Major Update**

**Release Date:** November 2025

**Changes from v1.0:**

| Change Type | Description |
|-------------|-------------|
| Damage System | Changed from SetByCaller Health modifier to NarrativeDamageExecCalc |
| Knockback Force | Changed from 1500 to 1000 units |
| Ability Granting | Removed handle-based cleanup, uses source-based removal |
| UTF-8 Cleanup | Replaced all special characters with ASCII equivalents |
| Code Blocks | Converted flowchart code blocks to markdown tables |
| Instance Editable | Added Instance Editable flag to designer-friendly variables |
| Formatting | Standardized per Father_Companion_Guide_Format_Reference.md |

---

### **VERSION 1.0 - Initial Implementation**

**Release Date:** 2025

| Feature | Value |
|---------|-------|
| Dome burst AOE explosion | Created |
| Dual activation | Manual (Q) and Auto (threshold) |
| Energy-based damage | 150 base + 0.3 per energy |
| Maximum damage | 300 at full 500 energy |
| Burst radius | 500 units |
| Knockback force | 1500 units |
| Cooldown duration | 20 seconds |
| Min manual energy | 50 |

---

## **QUICK REFERENCE**

### **Tag Configuration Summary**

| Property | Tags |
|----------|------|
| Ability Tags | `Ability.Father.DomeBurst` |
| Activation Required | `Father.Dome.Active` |
| Activation Owned | `Father.State.Attacking` |
| Activation Blocked | `Cooldown.Father.DomeBurst` |
| InputTag | `Narrative.Input.Father.Ability1` |

### **Variable Summary**

| Variable | Type | Default | Instance Editable | Purpose |
|----------|------|---------|-------------------|---------|
| BurstDamage | Float | 75.0 | Yes | Flat burst damage |
| BurstRadius | Float | 500.0 | Yes | AOE explosion radius |
| KnockbackForce | Float | 1000.0 | Yes | Push force on enemies |
| MinEnergyForManual | Float | 50.0 | Yes | Min energy for Q press |
| PlayerRef | Actor Reference | None | No | Player reference |
| PlayerASC | NarrativeAbilitySystemComponent Reference | None | No | Player ASC for validation |

### **Gameplay Effect Summary**

| Effect | Duration | Purpose |
|--------|----------|---------|
| GE_DomeBurstDamage | Instant | Damage via NarrativeDamageExecCalc |
| GE_DomeBurstCooldown | 12 seconds | Grants cooldown tags |
| GE_DomeEnergyReset | Instant | Resets DomeEnergy to 0 |

### **NarrativeDamageExecCalc Formula**

| Step | Formula |
|------|---------|
| Attack Multiplier | 1.0 + (AttackRating / 100.0) |
| Defence Multiplier | 1.0 + (Armor / 100.0) |
| Final Damage | (BurstDamage x AttackMultiplier) / DefenceMultiplier |

### **Activation Modes**

| Mode | Trigger | Energy Required |
|------|---------|-----------------|
| Manual | Q Key | 50+ |
| Auto | Threshold | 500 (max) |

### **Burst Flow Summary**

| Step | Action |
|------|--------|
| 1 | Q Key Pressed OR Auto-Trigger |
| 2 | Validate Dome Active tag |
| 3 | Check Minimum Energy (50 for manual) |
| 4 | Sphere Overlap (500 radius) |
| 5 | For Each Enemy with Character.Enemy tag |
| 6 | Apply GE_DomeBurstDamage (75 flat damage) via NarrativeDamageExecCalc |
| 7 | Launch Character (1000 unit knockback) |
| 8 | Spawn NS_DomeBurst VFX |
| 9 | Apply GE_DomeEnergyReset |
| 10 | Apply GE_DomeBurstCooldown (12s) |
| 11 | Clear References (PlayerRef, PlayerASC) |
| 12 | End Ability |

### **Replication Settings**

| Setting | Value |
|---------|-------|
| Instancing Policy | Instanced Per Actor |
| Replication Policy | Replicate |
| Net Execution Policy | Local Predicted |

### **Armor Form Abilities**

| Ability | Type | Status |
|---------|------|--------|
| GA_FatherArmor | Form | Existing |
| GA_ProtectiveDome | Passive | Existing |
| GA_DomeBurst | Active/Auto | Complete |

### **Related Documents**

| Document | Version | Relevance |
|----------|---------|-----------|
| manifest.yaml | - | Authoritative source for all values |
| Father_Companion_Technical_Reference | v6.3 | Cross-actor patterns, EndAbility cleanup |
| Father_Companion_System_Setup_Guide | v1.3 | BP_FatherCompanion setup |
| GA_FatherArmor_Implementation_Guide | v4.8 | Form activation |
| DefaultGameplayTags_FatherCompanion | v4.0 | Tag definitions |

---

**END OF GA_DOMEBURST IMPLEMENTATION GUIDE v2.9**

**Armor Form - Active/Auto AOE Explosion**

**Unreal Engine 5.7 + Narrative Pro v2.2**

**Blueprint-Only Implementation**
