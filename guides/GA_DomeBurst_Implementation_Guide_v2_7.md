# Father Companion - GA_DomeBurst Implementation Guide
## VERSION 2.7 - Blueprint Node Consistency Fixes
## Unreal Engine 5.6 + Narrative Pro Plugin v2.2

---

## **DOCUMENT INFORMATION**

| Field | Value |
|-------|-------|
| Ability Name | GA_DomeBurst |
| Ability Type | Active (Q Key) / Auto (Threshold) |
| Parent Class | NarrativeGameplayAbility |
| Form | Armor |
| Input | Q Key (Manual) or Auto (When Dome Full) |
| Version | 2.7 |
| Last Updated | January 2026 |

---

## **TABLE OF CONTENTS**

1. [Introduction](#introduction)
2. [Prerequisites](#prerequisites)
3. [PHASE 1: Gameplay Tags Setup](#phase-1-gameplay-tags-setup)
4. [PHASE 2: Create Gameplay Effects](#phase-2-create-gameplay-effects)
5. [PHASE 3: Create GA_DomeBurst Ability](#phase-3-create-ga_domeburst-ability)
6. [PHASE 4: Implement Manual Burst Logic](#phase-4-implement-manual-burst-logic)
7. [PHASE 5: Implement Auto-Burst Integration](#phase-5-implement-auto-burst-integration)
8. [PHASE 6: Grant Ability to Player](#phase-6-grant-ability-to-player)
9. [PHASE 7: Input Configuration](#phase-7-input-configuration)
10. [Changelog](#changelog)
11. [Quick Reference](#quick-reference)

---

## **INTRODUCTION**

### **Ability Overview**

GA_DomeBurst is the explosive payoff ability for the Armor form's Protective Dome system. When the father is attached as armor, it absorbs incoming damage and stores it as Dome Energy. GA_DomeBurst triggers in two ways: automatically when the dome reaches maximum energy (500 absorbed), or manually when the player presses Q to release early. The burst deals AOE damage based on current energy and knocks back all nearby enemies.

### **Key Features**

| Feature | Description |
|---------|-------------|
| Dual Activation | Manual (Q key) or Automatic (threshold) |
| Energy-Based Damage | Damage scales with absorbed energy |
| AOE Explosion | 500 unit radius centered on player |
| Knockback Effect | Pushes enemies away from player |
| Cooldown System | 20 seconds after burst before recharge |
| Form Specific | Only available in Armor form |
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
| Base Damage | 150 |
| Damage Scaling | +0.3 per energy (max +150) |
| Max Damage | 300 (at 500 energy) |
| Burst Radius | 500 units |
| Knockback Force | 1000 units |
| Cooldown | 20 seconds |
| Min Energy for Manual | 50 |

### **Damage Formula**

| Energy Level | Calculation | Result |
|--------------|-------------|--------|
| 50 Energy | 150 + (50 x 0.3) | 165 damage |
| 100 Energy | 150 + (100 x 0.3) | 180 damage |
| 250 Energy | 150 + (250 x 0.3) | 225 damage |
| 500 Energy | 150 + (500 x 0.3) | 300 damage (max) |

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
| GA_FatherArmor | Armor form activation ability |
| GA_DomeManager | Dome absorption management ability |
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
| Father.Form.Armor | Armor form active |
| Father.Dome.Active | Dome system running |
| Father.Dome.Charging | Dome absorbing damage |
| Father.State.Recruited | Father recruited by player |

---

## **PHASE 1: GAMEPLAY TAGS SETUP**

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Father.Dome.FullyCharged | Dome at maximum energy, ready for auto-burst |
| Father.Dome.OnCooldown | Dome burst on cooldown, cannot charge |
| Ability.Father.Armor.DomeBurst | Armor dome AOE explosion ability |
| Father.State.Bursting | Dome burst ability executing |
| Effect.Father.DomeBurst | Damage from dome burst explosion |
| Effect.Father.DomeBurstKnockback | Knockback from dome burst |
| Cooldown.Father.Armor.DomeBurst | Cooldown after dome burst |
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
| Duration Magnitude -> Scalable Float Magnitude -> Value | 20.0 |
| Components | Grant Tags to Target Actor |
| Add Tags -> Add to Inherited [0] | Cooldown.Father.Armor.DomeBurst |
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
| Ability Tags | Ability.Father.Armor.DomeBurst |
| Activation Required Tags | Father.Form.Armor, Father.Dome.Active, Father.State.Recruited |
| Activation Owned Tags | Father.State.Bursting |
| Activation Blocked Tags | Cooldown.Father.Armor.DomeBurst, Father.State.Bursting |

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
| BaseDamage | Float | 150.0 | Yes |
| DamageScaling | Float | 0.3 | Yes |
| BurstRadius | Float | 500.0 | Yes |
| KnockbackForce | Float | 1000.0 | Yes |
| MinEnergyForManual | Float | 50.0 | Yes |
| PlayerRef | Actor (Object Reference) | None | No |
| PlayerASC | NarrativeAbilitySystemComponent (Object Reference) | None | No |
| CurrentDomeEnergy | Float | 0.0 | No |
| CalculatedDamage | Float | 0.0 | No |

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

#### 4.2) Store Energy Value
   - 4.2.1) From Branch True execution pin:
      - 4.2.1.1) Add Set CurrentDomeEnergy node
   - 4.2.2) Connect Get Numeric Attribute Current Value pin to CurrentDomeEnergy value pin

### **5) Validate Minimum Energy (Manual Burst)**

#### 5.1) Check Minimum Energy
   - 5.1.1) Add Float >= Float (Greater or Equal) node
   - 5.1.2) Connect:
      - 5.1.2.1) CurrentDomeEnergy getter to first input (A)
      - 5.1.2.2) MinEnergyForManual getter to second input (B)

#### 5.2) Branch on Energy Check
   - 5.2.1) From Set CurrentDomeEnergy execution pin:
      - 5.2.1.1) Add Branch node
   - 5.2.2) Connect comparison Return Value to Condition pin
   - 5.2.3) From Branch False execution pin:
      - 5.2.3.1) Add End Ability node
      - 5.2.3.2) Was Cancelled: Checked (true)

### **6) Calculate Burst Damage**

#### 6.1) Calculate Scaled Damage
   - 6.1.1) Add Float * Float (Multiply) node
   - 6.1.2) Connect:
      - 6.1.2.1) CurrentDomeEnergy getter to first input (A)
      - 6.1.2.2) DamageScaling getter to second input (B)

#### 6.2) Add Base Damage
   - 6.2.1) Add Float + Float (Add) node
   - 6.2.2) Connect:
      - 6.2.2.1) BaseDamage getter to first input (A)
      - 6.2.2.2) Multiply Return Value to second input (B)

#### 6.3) Store Calculated Damage
   - 6.3.1) From Branch True execution pin:
      - 6.3.1.1) Add Set CalculatedDamage node
   - 6.3.2) Connect Add Return Value to CalculatedDamage value pin

### **7) Find Enemies in Radius**

#### 7.1) Get Player Location
   - 7.1.1) From PlayerRef getter:
      - 7.1.1.1) Drag outward and search: `Get Actor Location`
      - 7.1.1.2) Add Get Actor Location node

#### 7.2) Sphere Overlap for Actors
   - 7.2.1) From Set CalculatedDamage execution pin:
      - 7.2.1.1) Drag outward and search: `Sphere Overlap Actors`
      - 7.2.1.2) Add Sphere Overlap Actors node
   - 7.2.2) Connect execution pin
   - 7.2.3) Configure:
      - 7.2.3.1) **World Context Object**: Right-click on pin, select **Get a Reference to Self**
      - 7.2.3.2) **Sphere Pos**: Connect Get Actor Location Return Value
      - 7.2.3.3) **Sphere Radius**: Connect BurstRadius getter
      - 7.2.3.4) **Object Types**: Add `Pawn`
      - 7.2.3.5) **Actors to Ignore**: Create Make Array with PlayerRef

### **8) Apply Damage to Each Enemy**

#### 8.1) Create ForEach Loop
   - 8.1.1) From Sphere Overlap Actors execution pin:
      - 8.1.1.1) Drag outward and search: `For Each Loop`
      - 8.1.1.2) Add For Each Loop node
   - 8.1.2) Connect Out Actors pin to ForEach Array pin

#### 8.2) Filter Enemies Only
   - 8.2.1) From ForEach Loop Body execution pin:
      - 8.2.1.1) From Array Element pin, drag outward
      - 8.2.1.2) Search: `Get Ability System Component`
      - 8.2.1.3) Add Get Ability System Component node
   - 8.2.2) From enemy ASC Return Value:
      - 8.2.2.1) Drag outward and search: `Has Matching Gameplay Tag`
      - 8.2.2.2) Add Has Matching Gameplay Tag node
   - 8.2.3) Configure:
      - 8.2.3.1) Tag to Check: `Character.Enemy`

#### 8.3) Branch on Enemy Check
   - 8.3.1) Add Branch node
   - 8.3.2) Connect Has Matching Gameplay Tag Return Value to Condition pin
   - 8.3.3) From Branch False execution pin: Leave unconnected (continues to next iteration)

#### 8.4) Create Damage Effect Spec
   - 8.4.1) From Branch True execution pin:
      - 8.4.1.1) Drag from PlayerASC variable
      - 8.4.1.2) Search: `Make Outgoing Spec`
      - 8.4.1.3) Add Make Outgoing Spec node
   - 8.4.2) Configure:
      - 8.4.2.1) Gameplay Effect Class: `GE_DomeBurstDamage`
      - 8.4.2.2) Level: `1.0`

#### 8.5) Assign Set By Caller Magnitude
   - 8.5.1) From Make Outgoing Spec Return Value pin:
      - 8.5.1.1) Drag outward and search: `Assign Tag Set By Caller Magnitude`
      - 8.5.1.2) Add Assign Tag Set By Caller Magnitude node
   - 8.5.2) Configure:
      - 8.5.2.1) Data Tag: `Data.Damage.DomeBurst`
      - 8.5.2.2) Magnitude: Connect CalculatedDamage getter

#### 8.6) Apply Effect to Enemy
   - 8.6.1) From Assign Tag Set By Caller Magnitude execution pin:
      - 8.6.1.1) Drag outward and search: `Apply Gameplay Effect Spec to Target`
      - 8.6.1.2) Add Apply Gameplay Effect Spec to Target node
   - 8.6.2) Connect:
      - 8.6.2.1) Execution from Assign Tag node
      - 8.6.2.2) **Target** pin: Connect enemy ASC (from Get Ability System Component in step 8.2.1)
      - 8.6.2.3) **Spec Handle** pin: Connect Assign Tag Return Value

### **9) Apply Knockback**

#### 9.1) Calculate Knockback Direction
   - 9.1.1) From Array Element (enemy):
      - 9.1.1.1) Drag outward and search: `Get Actor Location`
      - 9.1.1.2) Add Get Actor Location node
   - 9.1.2) Add Vector - Vector (Subtract) node
   - 9.1.3) Connect:
      - 9.1.3.1) Enemy Location to A pin
      - 9.1.3.2) Player Location to B pin

#### 9.2) Normalize Direction
   - 9.2.1) From Subtract Return Value:
      - 9.2.1.1) Drag outward and search: `Normalize`
      - 9.2.1.2) Add Normalize node

#### 9.3) Scale by Knockback Force
   - 9.3.1) Add Vector * Float (Multiply) node
   - 9.3.2) Connect:
      - 9.3.2.1) Normalize Return Value to Vector pin
      - 9.3.2.2) KnockbackForce getter to Float pin

#### 9.4) Cast to Character (Required)
   - 9.4.1) From Array Element:
      - 9.4.1.1) Drag outward and search: `Cast To Character`
      - 9.4.1.2) Add Cast To Character node
   - 9.4.2) Array Element is AActor* but Launch Character requires ACharacter*

#### 9.5) Launch Character
   - 9.5.1) From Apply Gameplay Effect Spec execution pin:
      - 9.5.1.1) Connect to Cast To Character execution input
   - 9.5.2) From Cast success execution pin:
      - 9.5.2.1) From **As Character** pin, drag outward
      - 9.5.2.2) Search: `Launch Character`
      - 9.5.2.3) Add Launch Character node
   - 9.5.3) Configure:
      - 9.5.3.1) **Target**: Connect **As Character** pin from Cast node
      - 9.5.3.2) **Launch Velocity**: Connect scaled knockback vector (Multiply Return Value)
      - 9.5.3.3) **XY Override**: Unchecked
      - 9.5.3.4) **Z Override**: Unchecked

### **10) Spawn Burst Visual Effect**

#### 10.1) Spawn Niagara System
   - 10.1.1) From ForEach Loop Completed execution pin:
      - 10.1.1.1) Drag outward and search: `Spawn System at Location`
      - 10.1.1.2) Add Spawn System at Location node
   - 10.1.2) Connect execution pin
   - 10.1.3) Configure:
      - 10.1.3.1) System Template: Select `NS_DomeBurst`
      - 10.1.3.2) Location: Connect Player Location (Get Actor Location Return Value)
      - 10.1.3.3) Auto Destroy: Checked

### **11) Reset Dome Energy**

#### 11.1) Apply Energy Reset Effect
   - 11.1.1) From Spawn System at Location execution pin:
      - 11.1.1.1) Drag from PlayerASC variable
      - 11.1.1.2) Search: `Apply Gameplay Effect to Self`
      - 11.1.1.3) Add Apply Gameplay Effect to Self node
   - 11.1.2) Connect execution pin
   - 11.1.3) Configure:
      - 11.1.3.1) Gameplay Effect Class: `GE_DomeEnergyReset`
      - 11.1.3.2) Level: `1.0`

### **12) Apply Cooldown**

#### 12.1) Apply Burst Cooldown Effect
   - 12.1.1) From Apply GE (Reset) execution pin:
      - 12.1.1.1) Drag from PlayerASC variable
      - 12.1.1.2) Search: `Apply Gameplay Effect to Self`
      - 12.1.1.3) Add Apply Gameplay Effect to Self node
   - 12.1.2) Connect execution pin
   - 12.1.3) Configure:
      - 12.1.3.1) Gameplay Effect Class: `GE_DomeBurstCooldown`
      - 12.1.3.2) Level: `1.0`

### **13) End Ability**

#### 13.1) Commit Ability
   - 13.1.1) From Apply GE (Cooldown) execution pin:
      - 13.1.1.1) Drag outward and search: `Commit Ability`
      - 13.1.1.2) Add Commit Ability node
   - 13.1.2) Connect execution pin

#### 13.2) Clear References
   - 13.2.1) From Commit Ability execution pin:
      - 13.2.1.1) Add Set PlayerRef node
      - 13.2.1.2) Leave value pin disconnected (clears to None)
   - 13.2.2) From Set PlayerRef execution pin:
      - 13.2.2.1) Add Set PlayerASC node
      - 13.2.2.2) Leave value pin disconnected (clears to None)

#### 13.3) End Ability
   - 13.3.1) From Set PlayerASC execution pin:
      - 13.3.1.1) Drag outward and search: `End Ability`
      - 13.3.1.2) Add End Ability node
   - 13.3.2) Connect execution pin
   - 13.3.3) Was Cancelled: Unchecked (false)

### **14) Implement Event OnEndAbility**

#### 14.1) Add Event OnEndAbility
   - 14.1.1) Right-click in Event Graph (separate area from main logic)
   - 14.1.2) Search: `Event OnEndAbility`
   - 14.1.3) Add Event OnEndAbility node

#### 14.2) Validate PlayerASC for Cleanup
   - 14.2.1) From Event OnEndAbility execution pin:
      - 14.2.1.1) Drag from PlayerASC variable
      - 14.2.1.2) Add Is Valid node
   - 14.2.2) From Is Valid node:
      - 14.2.2.1) Add Branch node
      - 14.2.2.2) Connect Is Valid Return Value to Condition pin
   - 14.2.3) From Branch False execution pin:
      - 14.2.3.1) Leave unconnected (nothing to clean up)

#### 14.3) Clear References on Early Cancel
   - 14.3.1) From Branch True execution pin:
      - 14.3.1.1) Add Set PlayerRef node
      - 14.3.1.2) Leave value pin disconnected (clears to None)
   - 14.3.2) From Set PlayerRef execution pin:
      - 14.3.2.1) Add Set PlayerASC node
      - 14.3.2.2) Leave value pin disconnected (clears to None)

### **15) Compile and Save**

#### 15.1) Save All Changes
   - 15.1.1) Click Compile button
   - 15.1.2) Click Save button

---

## **PHASE 5: IMPLEMENT AUTO-BURST INTEGRATION**

### **1) Open GA_DomeManager**

#### 1.1) Navigate to Dome Manager
   - 1.1.1) In Content Browser, navigate to `/Content/FatherCompanion/ProtectiveDome/Abilities/`
   - 1.1.2) Double-click GA_DomeManager to open

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
      - 4.2.3.2) Add tag: `Ability.Father.Armor.DomeBurst`

### **5) Compile and Save**

#### 5.1) Save All Changes
   - 5.1.1) Click Compile button
   - 5.1.2) Click Save button

---

## **PHASE 6: GRANT ABILITY TO PLAYER**

### **1) Open GA_FatherArmor**

#### 1.1) Navigate to Armor Ability
   - 1.1.1) In Content Browser, navigate to `/Content/FatherCompanion/Armor/Abilities/`
   - 1.1.2) Double-click GA_FatherArmor to open

### **2) Find Armor Activation Section**

#### 2.1) Locate Attachment Complete Logic
   - 2.1.1) In Event Graph, find after father attaches to player
   - 2.1.2) Where armor boost effects are applied

### **3) Grant Dome Burst Ability**

#### 3.1) Get Player ASC
   - 3.1.1) From Player reference:
      - 3.1.1.1) Drag outward and search: `Get Ability System Component`
      - 3.1.1.2) Add Get Ability System Component node

#### 3.2) Give Ability
   - 3.2.1) After other armor effects applied:
      - 3.2.1.1) From Player ASC, drag outward
      - 3.2.1.2) Search: `Give Ability`
      - 3.2.1.3) Add Give Ability node
   - 3.2.2) Connect execution pin
   - 3.2.3) Configure:
      - 3.2.3.1) Ability Class: `GA_DomeBurst`
      - 3.2.3.2) Level: `1`
      - 3.2.3.3) Input ID: `-1`

#### 3.3) Store Ability Handle
   - 3.3.1) From Give Ability Return Value:
      - 3.3.1.1) Add Set DomeBurstHandle node
      - 3.3.1.2) Connect Return Value to DomeBurstHandle value pin

### **4) Compile and Save**

#### 4.1) Save All Changes
   - 4.1.1) Click Compile button
   - 4.1.2) Click Save button

### **5) Ability Cleanup Note**

When player switches forms via T wheel, GA_FatherArmor is cancelled by Cancel Abilities With Tag. GA_FatherArmor's EndAbility uses stored DomeBurstHandle to properly remove GA_DomeBurst before clearing references.

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

**Damage Pipeline Benefits (v2.0):**

| Feature | Behavior |
|---------|----------|
| AttackRating | Player's AttackRating multiplies burst damage |
| Armor | Enemy armor reduces damage received |
| State.Invulnerable | Automatically blocks damage to invulnerable targets |
| Friendly Fire | Respects NarrativeCombatDeveloperSettings |
| Damage Notifications | Proper damage dealt/received callbacks |

**Removed from v1.0:**

| Item | Reason |
|------|--------|
| DomeBurstHandle variable | Source-based cleanup handles automatically |
| Cancel Ability Handle node | Not needed with source-based removal |
| Clear Ability node | Not needed with source-based removal |
| Health attribute direct modification | Replaced with NarrativeDamageExecCalc |

---

### **VERSION 1.0 - Initial Implementation**

**Release Date:** 2025

**Implementation Details:**

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
| Ability Tags | `Ability.Father.Armor.DomeBurst` |
| Activation Required | `Father.Form.Armor`, `Father.Dome.Active`, `Father.State.Recruited` |
| Activation Owned | `Father.State.Bursting` |
| Activation Blocked | `Cooldown.Father.Armor.DomeBurst`, `Father.State.Bursting` |
| InputTag | `Narrative.Input.Father.Ability1` |

### **Variable Summary**

| Variable | Type | Default | Category | Instance Editable | Purpose |
|----------|------|---------|----------|-------------------|---------|
| BaseDamage | Float | 150.0 | Burst Config | Yes | Minimum burst damage |
| DamageScaling | Float | 0.3 | Burst Config | Yes | Damage per energy point |
| BurstRadius | Float | 500.0 | Burst Config | Yes | AOE explosion radius |
| KnockbackForce | Float | 1000.0 | Burst Config | Yes | Push force on enemies |
| MinEnergyForManual | Float | 50.0 | Burst Config | Yes | Min energy for Q press |
| PlayerRef | Actor Reference | None | Runtime | No | Player reference |
| PlayerASC | NarrativeAbilitySystemComponent Reference | None | Runtime | No | Player ASC for validation |
| CurrentDomeEnergy | Float | 0.0 | Runtime | No | Current energy snapshot |
| CalculatedDamage | Float | 0.0 | Runtime | No | Final damage value |

### **Gameplay Effect Summary**

| Effect | Duration | Purpose |
|--------|----------|---------|
| GE_DomeBurstDamage | Instant | Damage via NarrativeDamageExecCalc |
| GE_DomeBurstCooldown | 20 seconds | Grants cooldown tags |
| GE_DomeEnergyReset | Instant | Resets DomeEnergy to 0 |

### **Damage Calculation Table**

| Energy | Base Calculation | After AttackRating/Armor |
|--------|------------------|--------------------------|
| 50 | 150 + (50 x 0.3) = 165 | Modified by NarrativeDamageExecCalc |
| 100 | 150 + (100 x 0.3) = 180 | Modified by NarrativeDamageExecCalc |
| 250 | 150 + (250 x 0.3) = 225 | Modified by NarrativeDamageExecCalc |
| 500 | 150 + (500 x 0.3) = 300 | Modified by NarrativeDamageExecCalc |

### **NarrativeDamageExecCalc Formula**

| Step | Formula |
|------|---------|
| Attack Multiplier | 1.0 + (AttackRating / 100.0) |
| Defence Multiplier | 1.0 + (Armor / 100.0) |
| Final Damage | (BaseDamage x AttackMultiplier) / DefenceMultiplier |

### **Activation Modes**

| Mode | Trigger | Energy Required |
|------|---------|-----------------|
| Manual | Q Key | 50+ |
| Auto | Threshold | 500 (max) |

### **Burst Flow Summary**

| Step | Action |
|------|--------|
| 1 | Q Key Pressed OR Auto-Trigger |
| 2 | Validate Armor Form + Dome Active + Recruited |
| 3 | Check Minimum Energy (50 for manual) |
| 4 | Calculate Damage: 150 + (Energy x 0.3) |
| 5 | Sphere Overlap (500 radius) |
| 6 | For Each Enemy with Character.Enemy tag |
| 7 | Apply GE_DomeBurstDamage via NarrativeDamageExecCalc |
| 8 | Launch Character (1000 unit knockback) |
| 9 | Spawn NS_DomeBurst VFX |
| 10 | Apply GE_DomeEnergyReset |
| 11 | Apply GE_DomeBurstCooldown (20s) |
| 12 | Clear References (PlayerRef, PlayerASC) |
| 13 | End Ability |

### **EndAbility Cleanup Flow**

| Step | Action |
|------|--------|
| 1 | Event OnEndAbility triggered |
| 2 | Validate PlayerASC (early exit if invalid) |
| 3 | Clear PlayerRef (Set to None) |
| 4 | Clear PlayerASC (Set to None) |

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
| GA_ProtectiveDome | Passive | Existing (via GA_DomeManager) |
| GA_DomeManager | Passive | Existing |
| GA_DomeBurst | Active/Auto | Complete |

### **Related Documents**

| Document | Version | Relevance |
|----------|---------|-----------|
| Father_Companion_Technical_Reference | v4.5 | Cross-actor patterns, EndAbility cleanup |
| Father_Companion_System_Setup_Guide | v1.3 | BP_FatherCompanion setup |
| GA_FatherArmor_Implementation_Guide | v3.4 | Handle storage, EndAbility cleanup |
| Father_Protective_Dome_Implementation_Guide | v1.9 | Dome system integration |
| DefaultGameplayTags_FatherCompanion | v3.5 | Tag definitions |

---

**END OF GA_DOMEBURST IMPLEMENTATION GUIDE v2.7**

**Armor Form - Active/Auto AOE Explosion**

**Unreal Engine 5.6 + Narrative Pro v2.2**

**Blueprint-Only Implementation**
