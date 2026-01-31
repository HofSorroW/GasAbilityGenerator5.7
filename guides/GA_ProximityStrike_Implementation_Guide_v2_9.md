# Father Companion - GA_ProximityStrike Implementation Guide
## VERSION 2.12 - Variable Naming Alignment, Defense-in-Depth Tags
## Unreal Engine 5.7 + Narrative Pro Plugin v2.2

---

## **DOCUMENT INFORMATION**

| Field | Value |
|-------|-------|
| Ability Name | GA_ProximityStrike |
| Ability Type | Passive AOE Damage |
| Parent Class | NarrativeGameplayAbility |
| Form | Symbiote (Full Body Merge) |
| Input | Passive (activated by GA_FatherSymbiote) |
| Version | 2.12 |
| Granting Method | EquippableItem (BP_FatherSymbioteForm) |
| Activation Method | Player Q input (INV-INPUT-1 compliant) |

---

## **TABLE OF CONTENTS**

1. [Introduction](#introduction)
2. [Prerequisites](#prerequisites)
3. [PHASE 1: Gameplay Tags Setup](#phase-1-gameplay-tags-setup)
4. [PHASE 2: Create Gameplay Effects](#phase-2-create-gameplay-effects)
5. [PHASE 3: Create GA_ProximityStrike Ability](#phase-3-create-ga_proximitystrike-ability)
6. [PHASE 4: Implement Proximity Detection Logic](#phase-4-implement-proximity-detection-logic)
7. [PHASE 5: Implement Damage Application Logic](#phase-5-implement-damage-application-logic)
8. [PHASE 6: Implement Cleanup Logic](#phase-6-implement-cleanup-logic)
9. [PHASE 7: Integration with BP_FatherSymbioteForm](#phase-7-integration-with-bp_fathersymbioteform)
10. [PHASE 8: Update GA_FatherSymbiote for Activation](#phase-8-update-ga_fathersymbiote-for-activation)
11. [Changelog](#changelog)

---

## **INTRODUCTION**

### **Ability Overview**

GA_ProximityStrike is a passive AOE damage ability that automatically damages all enemies within range while the player is in Symbiote form. The ability pulses damage at regular intervals, creating a devastating aura around the merged player-father entity.

### **Key Features**

- Passive Activation: Explicitly activated by GA_FatherSymbiote after form tags applied
- AOE Damage: Damages all enemies within 400 unit radius
- Rapid Ticks: Deals damage every 0.5 seconds
- Unlimited Targets: No cap on simultaneous targets
- Form Restriction: Only active during Symbiote form
- Contract 24 Compliant: Uses NarrativeDamageExecCalc with captured AttackDamage attribute (NOT SetByCaller)
- Hierarchical Tag: Uses Ability.Father.Symbiote.ProximityStrike for blanket cancel support
- Recruited Gate: Requires Father.State.Recruited for activation

### **Symbiote Form Context**

In Symbiote form, the father fully merges with the player body, creating a berserker state:
- Requires 100% charge to activate (validated in GA_FatherSymbiote)
- Limited duration (30 seconds)
- All abilities enhance aggressive playstyle
- Proximity Strike is the core damage engine

### **Technical Specifications**

| Parameter | Value |
|-----------|-------|
| Damage Per Tick | 50 (base, before scaling) |
| Tick Rate | 0.5 seconds |
| Damage Radius | 400 units |
| Max Targets | Unlimited |
| Form Required | Symbiote |
| State Required | Merged, Recruited |
| Form Duration | 30 seconds |

### **Damage Flow (Contract 24 Compliant)**

| Step | Component | Action |
|------|-----------|--------|
| 1 | GA_FatherSymbiote | Activates GA_ProximityStrike after merge |
| 2 | GA_ProximityStrike | Timer triggers DealProximityDamage |
| 3 | Sphere Overlap | Detects enemies in radius |
| 4 | Make Outgoing Spec | Creates GE spec (source: Father ASC) |
| 5 | GE_ProximityDamage | Applied to each target |
| 6 | NarrativeDamageExecCalc | Captures AttackDamage from Father (source) |
| 7 | ExecCalc | Applies AttackRating multiplier |
| 8 | ExecCalc | Applies target Armor reduction |
| 9 | ExecCalc | Checks Narrative.State.Invulnerable |
| 10 | Attribute System | Modifies target Health |

**Contract 24 Note:** Damage comes from Father's captured AttackDamage attribute, NOT SetByCaller. Father's AttackDamage should be set to 50 via GE_FatherSymbioteStats initialization effect.

### **Activation Flow**

| Step | Action | Result |
|------|--------|--------|
| 1 | Player equips BP_FatherSymbioteForm | Parent:HandleEquip grants GA_ProximityStrike |
| 2 | bActivateAbilityOnGranted = false | No auto-activation attempt |
| 3 | BP_FatherSymbioteForm activates GA_FatherSymbiote | Form ability starts |
| 4 | GA_FatherSymbiote applies Activation Owned Tags | Effect.Father.FormState.Symbiote + Father.State.Merged present |
| 5 | GA_FatherSymbiote calls TryActivateAbilityByClass | GA_ProximityStrike activation requested |
| 6 | GA_ProximityStrike validates Activation Required Tags | Tags now present - activation succeeds |
| 7 | ProximityStrike timer starts | AOE damage begins |

### **Deactivation Flow (Blanket Cancel)**

| Step | Action | Result |
|------|--------|--------|
| 1 | Player switches form or Symbiote ends | New form ability activates |
| 2 | New form has Cancel Abilities with Tag | Includes Ability.Father.Symbiote |
| 3 | GA_ProximityStrike tag is Ability.Father.Symbiote.ProximityStrike | Child of cancelled parent tag |
| 4 | GAS hierarchical tag matching | GA_ProximityStrike cancelled automatically |
| 5 | Event OnEndAbility fires | Timer cleared, references cleared, cleanup complete |

---

## **PREREQUISITES**

### **Required Before This Guide**

| Requirement | Description | Reference |
|-------------|-------------|-----------|
| BP_FatherCompanion | Father character with NarrativeNPCCharacter parent | Father_Companion_System_Setup_Guide_v1_3 |
| GA_FatherSymbiote | Symbiote form activation ability | GA_FatherSymbiote_Implementation_Guide_v2_9 |
| BP_FatherSymbioteForm | EquippableItem for Symbiote form (PENDING) | Father_Companion_Forms_Implementation_Guide_v4_0, PHASE 6 |
| Effect.Father.FormState.Symbiote tag | Form identification tag | DefaultGameplayTags_FatherCompanion_v3_5.ini |
| Father.State.Merged tag | Full body merge state tag | DefaultGameplayTags_FatherCompanion_v3_5.ini |
| Father.State.Recruited tag | Father recruited by player | DefaultGameplayTags_FatherCompanion_v3_5.ini |
| Player Character | Player with NarrativeAbilitySystemComponent | Narrative Pro default |

### **Pending Prerequisites**

BP_FatherSymbioteForm (EquippableItem) must be created following Father_Companion_Forms_Implementation_Guide_v4_0, PHASE 6 before PHASE 7 of this guide can be completed.

---

## **PHASE 1: GAMEPLAY TAGS SETUP**

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Symbiote.ProximityStrike | Father proximity AOE damage - Symbiote form passive |
| Father.State.ProximityActive | Proximity strike aura is active |
| Effect.Father.ProximityDamage | Target is being damaged by proximity aura |

**Note:** Data.Damage.ProximityStrike tag removed per Contract 24 - SetByCaller forbidden for damage with NarrativeDamageExecCalc.

### **Verify Existing Tags**

| Tag Name | Purpose |
|----------|---------|
| Effect.Father.FormState.Symbiote | Father is in Symbiote/Berserker form |
| Father.State.Merged | Father fully merged with player body |
| Father.State.Recruited | Father recruited by player |

---

## **PHASE 2: CREATE GAMEPLAY EFFECTS**

### **1) Create Effects Folder**

#### 1.1) Navigate to Symbiote Folder
   - 1.1.1) In Content Browser, go to `/Content/FatherCompanion/`
   - 1.1.2) If **Symbiote** folder missing:
      - 1.1.2.1) Right-click in Content Browser
      - 1.1.2.2) Select **New Folder**
      - 1.1.2.3) Name: `Symbiote`
      - 1.1.2.4) Press **Enter**

#### 1.2) Create Effects Subfolder
   - 1.2.1) Double-click **Symbiote** folder to open
   - 1.2.2) Right-click in Content Browser
   - 1.2.3) Select **New Folder**
   - 1.2.4) Name: `Effects`
   - 1.2.5) Press **Enter**
   - 1.2.6) Double-click **Effects** folder to open

### **2) Create GE_ProximityDamage**

#### 2.1) Create Gameplay Effect Asset
   - 2.1.1) Right-click in `/Content/FatherCompanion/Symbiote/Effects/`
   - 2.1.2) Select **Gameplay** -> **Gameplay Effect**
   - 2.1.3) Name: `GE_ProximityDamage`
   - 2.1.4) Press **Enter**
   - 2.1.5) Double-click to open

#### 2.2) Configure Duration Policy
   - 2.2.1) Click **Class Defaults** button in toolbar
   - 2.2.2) In Details panel, find **Gameplay Effect** section
   - 2.2.3) Find **Duration Policy** dropdown
   - 2.2.4) Select: `Instant`

#### 2.3) Add Executions Component
   - 2.3.1) In Details panel, find **Components** section
   - 2.3.2) Click **+ (Plus)** button to add component
   - 2.3.3) Search: `Executions`
   - 2.3.4) Select: **ExecutionsGameplayEffectComponent**

#### 2.4) Configure Damage Execution (Contract 24 Compliant)
   - 2.4.1) Click Executions component to expand
   - 2.4.2) Find **Execution Definitions** array
   - 2.4.3) Click **+ (Plus)** to add element [0]
   - 2.4.4) Expand element [0]
   - 2.4.5) Find **Calculation Class** property
   - 2.4.6) Click dropdown next to Calculation Class
   - 2.4.7) Search: `NarrativeDamageExecCalc`
   - 2.4.8) Select: **UNarrativeDamageExecCalc**

**Contract 24 Note:** Do NOT add SetByCaller modifiers for damage. NarrativeDamageExecCalc automatically captures AttackDamage from the source (Father). Father's AttackDamage attribute (set via GE_FatherSymbioteStats) determines damage output.

#### 2.5) Add Asset Tags Component
   - 2.5.1) Click **+ (Plus)** in Components section
   - 2.5.2) Search: `Asset Tags`
   - 2.5.3) Select: **AssetTagsGameplayEffectComponent**

#### 2.6) Configure Asset Tags
   - 2.6.1) Click component to expand
   - 2.6.2) Find **Added** section
   - 2.6.3) Click **+ (Plus)**
   - 2.6.4) Add tag: `Effect.Father.ProximityDamage`

#### 2.7) Compile and Save
   - 2.7.1) Click **Compile** button
   - 2.7.2) Click **Save** button

---

## **PHASE 3: CREATE GA_PROXIMITYSTRIKE ABILITY**

### **1) Create Abilities Folder**

#### 1.1) Navigate to Symbiote Folder
   - 1.1.1) In Content Browser, go to `/Content/FatherCompanion/Symbiote/`

#### 1.2) Create Abilities Subfolder
   - 1.2.1) If **Abilities** folder missing:
      - 1.2.1.1) Right-click in Symbiote folder
      - 1.2.1.2) Select **New Folder**
      - 1.2.1.3) Name: `Abilities`
      - 1.2.1.4) Press **Enter**
   - 1.2.2) Double-click **Abilities** folder to open

### **2) Create GA_ProximityStrike Blueprint**

#### 2.1) Create Blueprint Class
   - 2.1.1) Right-click in `/Content/FatherCompanion/Symbiote/Abilities/`
   - 2.1.2) Select **Blueprint Class**
   - 2.1.3) In **Pick Parent Class** dialog:
      - 2.1.3.1) Click **All Classes** dropdown at bottom
      - 2.1.3.2) In search bar, type: `NarrativeGameplayAbility`
      - 2.1.3.3) Select **NarrativeGameplayAbility** from list
      - 2.1.3.4) Click **Select** button
   - 2.1.4) Name: `GA_ProximityStrike`
   - 2.1.5) Press **Enter**
   - 2.1.6) Double-click to open

### **3) Configure Class Defaults**

#### 3.1) Open Class Defaults Panel
   - 3.1.1) Click **Class Defaults** button in Blueprint toolbar

### **4) Configure Ability Tags**

#### 4.1) Set Ability Tags (Hierarchical Tag)
   - 4.1.1) In Details panel, find **Tags** category
   - 4.1.2) Find **Ability Tags** property
   - 4.1.3) Click arrow to expand
   - 4.1.4) Under **Gameplay Tags**, click **+ (Plus)**
   - 4.1.5) In tag picker, search: `Ability.Father.Symbiote.ProximityStrike`
   - 4.1.6) Select the tag

#### 4.2) Configure Activation Required Tags
   - 4.2.1) Find **Activation Required Tags** property
   - 4.2.2) Expand property
   - 4.2.3) Under **Gameplay Tags**, click **+ (Plus)**
   - 4.2.4) Add tag: `Effect.Father.FormState.Symbiote`
   - 4.2.5) Click **+ (Plus)** again
   - 4.2.6) Add tag: `Father.State.Merged`
   - 4.2.7) Click **+ (Plus)** again
   - 4.2.8) Add tag: `Father.State.Recruited`

#### 4.3) Configure Activation Owned Tags
   - 4.3.1) Find **Activation Owned Tags** property
   - 4.3.2) Expand property
   - 4.3.3) Click **+ (Plus)**
   - 4.3.4) Add tag: `Father.State.ProximityActive`

#### 4.4) Configure Activation Blocked Tags
   - 4.4.1) Find **Activation Blocked Tags** property
   - 4.4.2) Expand property
   - 4.4.3) Click **+ (Plus)**
   - 4.4.4) Add tag: `Father.State.ProximityActive`

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

#### 5.4) Configure Additional Replication Settings
   - 5.4.1) Find **Replicate Input Directly** checkbox
   - 5.4.2) Ensure: Unchecked
   - 5.4.3) Find **Server Respects Remote Ability Cancellation**
   - 5.4.4) Ensure: Unchecked

### **6) Configure Narrative Ability Settings**

#### 6.1) Set InputTag to None
   - 6.1.1) In Details panel, find **Narrative Ability** category
   - 6.1.2) Find **Input Tag** property
   - 6.1.3) Ensure InputTag is empty (no tag)

#### 6.2) Disable Auto-Activation on Grant
   - 6.2.1) Find **bActivateAbilityOnGranted** property
   - 6.2.2) Uncheck the checkbox (set to false)

### **7) Compile and Save**

#### 7.1) Save Configuration
   - 7.1.1) Click **Compile** button
   - 7.1.2) Click **Save** button

---

## **PHASE 4: IMPLEMENT PROXIMITY DETECTION LOGIC**

### **1) Create Ability Variables**

#### 1.1) Open Event Graph
   - 1.1.1) In GA_ProximityStrike Blueprint, click **Event Graph** tab

#### 1.2) Create TickRate Variable
   - 1.2.1) Click **+ (Plus)** next to Variables
   - 1.2.2) Name: `TickRate`
   - 1.2.3) **Variable Type**: `Float`
   - 1.2.4) **Instance Editable**: Check
   - 1.2.5) Click **Compile**
   - 1.2.6) Set **Default Value**: `0.5`
   - 1.2.7) Set **Category**: `Proximity Config`

#### 1.3) Create ProximityRadius Variable
   - 1.3.1) Click **+ (Plus)** next to Variables
   - 1.3.2) Name: `ProximityRadius`
   - 1.3.3) **Variable Type**: `Float`
   - 1.3.4) **Instance Editable**: Check
   - 1.3.5) Click **Compile**
   - 1.3.6) Set **Default Value**: `400.0`
   - 1.3.7) Set **Category**: `Proximity Config`

#### 1.4) Create PlayerRef Variable
   - 1.4.1) Click **+ (Plus)** next to Variables
   - 1.4.2) Name: `PlayerRef`
   - 1.4.3) **Variable Type**: `Actor` -> `Object Reference`
   - 1.4.4) Click **Compile**
   - 1.4.5) Set **Category**: `Runtime`

#### 1.5) Create FatherRef Variable
   - 1.5.1) Click **+ (Plus)** next to Variables
   - 1.5.2) Name: `FatherRef`
   - 1.5.3) **Variable Type**: `Actor` -> `Object Reference`
   - 1.5.4) Click **Compile**
   - 1.5.5) Set **Category**: `Runtime`

#### 1.6) Create FatherASC Variable
   - 1.6.1) Click **+ (Plus)** next to Variables
   - 1.6.2) Name: `FatherASC`
   - 1.6.3) **Variable Type**: Search `AbilitySystemComponent`
      - 1.6.3.1) Select: `AbilitySystemComponent` -> `Object Reference`
   - 1.6.4) Click **Compile**
   - 1.6.5) Set **Category**: `Runtime`

   **Contract 24 Note:** Father's ASC is used as the damage source for MakeOutgoingSpec - NarrativeDamageExecCalc captures AttackDamage from Father.

#### 1.7) Create DamageTimerHandle Variable
   - 1.7.1) Click **+ (Plus)** next to Variables
   - 1.7.2) Name: `DamageTimerHandle`
   - 1.7.3) **Variable Type**: Search `Timer Handle`
      - 1.7.3.1) Select: `Timer Handle` (structure type)
   - 1.7.4) Click **Compile**
   - 1.7.5) Set **Category**: `Runtime`

#### 1.8) Create IsActive Variable
   - 1.8.1) Click **+ (Plus)** next to Variables
   - 1.8.2) Name: `IsActive`
   - 1.8.3) **Variable Type**: `Boolean`
   - 1.8.4) Click **Compile**
   - 1.8.5) Set **Default Value**: Unchecked (false)
   - 1.8.6) Set **Category**: `Runtime`

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
      - 2.4.2.2) Set: Checked (true)

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

#### 2.7) Store Player Reference
   - 2.7.1) From Set FatherRef execution:
      - 2.7.1.1) Drag outward and search: `Set PlayerRef`
      - 2.7.1.2) Add **Set PlayerRef** node
   - 2.7.2) Connect execution
   - 2.7.3) Connect Get Owner Player **Return Value** to PlayerRef value

#### 2.8) Validate Player Reference
   - 2.8.1) From Set PlayerRef execution:
      - 2.8.1.1) Drag **PlayerRef** variable getter into graph
      - 2.8.1.2) Add **Is Valid** node
      - 2.8.1.3) Connect PlayerRef to Is Valid input
   - 2.8.2) From Is Valid output:
      - 2.8.2.1) Add **Branch** node
      - 2.8.2.2) Connect Is Valid **Return Value** to Branch **Condition**
   - 2.8.3) From Branch **False** execution:
      - 2.8.3.1) Add **End Ability** node
      - 2.8.3.2) **Was Cancelled**: Checked (true)

#### 2.9) Get Player ASC
   - 2.9.1) From Branch **True** execution:
      - 2.9.1.1) Drag **PlayerRef** variable getter into graph
      - 2.9.1.2) From PlayerRef, add **Get Ability System Component** node

#### 2.10) Cast to NarrativeAbilitySystemComponent
   - 2.10.1) From Get Ability System Component **Return Value**:
      - 2.10.1.1) Drag outward and search: `Cast To NarrativeAbilitySystemComponent`
      - 2.10.1.2) Add **Cast To NarrativeAbilitySystemComponent** node
   - 2.10.2) Connect execution from Branch True

#### 2.11) Handle ASC Cast Failure
   - 2.11.1) From Cast **Cast Failed** execution pin:
      - 2.11.1.1) Add **End Ability** node
      - 2.11.1.2) **Was Cancelled**: Checked (true)

#### 2.12) Store FatherASC
   - 2.12.1) From Cast success execution:
      - 2.12.1.1) Drag outward and search: `Set FatherASC`
      - 2.12.1.2) Add **Set FatherASC** node
   - 2.12.2) Connect **GetAbilitySystemComponent** Return Value to FatherASC value

   **Contract 24 Note:** Father's ASC is stored as the damage source for MakeOutgoingSpec.

#### 2.13) Validate FatherASC
   - 2.13.1) From Set FatherASC execution:
      - 2.13.1.1) Drag **FatherASC** variable getter into graph
      - 2.13.1.2) Add **Is Valid** node
      - 2.13.1.3) Add **Branch** node
      - 2.13.1.4) Connect Is Valid Return Value to Branch Condition
   - 2.13.2) From Branch **False** execution:
      - 2.13.2.1) Add **End Ability** node
      - 2.13.2.2) **Was Cancelled**: Checked (true)

### **3) Start Damage Timer**

#### 3.1) Set IsActive Flag
   - 3.1.1) From Section 2.13 Branch **True** execution:
      - 3.1.1.1) Drag outward and search: `Set IsActive`
      - 3.1.1.2) Add **Set IsActive** node
   - 3.1.2) Connect execution
   - 3.1.3) Check **Is Active** (true)

#### 3.2) Create Timer Function
   - 3.2.1) From Set IsActive execution:
      - 3.2.1.1) Drag outward and search: `Set Timer by Function Name`
      - 3.2.1.2) Add **Set Timer by Function Name** node
   - 3.2.2) Connect execution
   - 3.2.3) Configure Timer node:
      - 3.2.3.1) **Object**: Connect **Self** reference
      - 3.2.3.2) **Function Name**: Type `DealProximityDamage`
      - 3.2.3.3) **Time**: Connect **TickRate** getter
      - 3.2.3.4) **Looping**: Checked (true)

#### 3.3) Store Timer Handle
   - 3.3.1) From Set Timer **Return Value** (Timer Handle):
      - 3.3.1.1) Drag outward and search: `Set DamageTimerHandle`
      - 3.3.1.2) Add **Set DamageTimerHandle** node
   - 3.3.2) Connect Set Timer execution to Set DamageTimerHandle
   - 3.3.3) Connect Timer Return Value to DamageTimerHandle value

### **4) Compile and Save**

#### 4.1) Save Progress
   - 4.1.1) Click **Compile** button
   - 4.1.2) Click **Save** button

---

## **PHASE 5: IMPLEMENT DAMAGE APPLICATION LOGIC**

### **1) Create DealProximityDamage Function**

#### 1.1) Create New Function
   - 1.1.1) In **My Blueprint** panel, find **Functions** section
   - 1.1.2) Click **+ (Plus)** next to Functions
   - 1.1.3) Name: `DealProximityDamage`
   - 1.1.4) Press **Enter**

### **2) Check If Still Active**

#### 2.1) Add Branch for Active Check
   - 2.1.1) From function entry execution pin:
      - 2.1.1.1) Drag outward and search: `Branch`
      - 2.1.1.2) Add **Branch** node
   - 2.1.2) Drag **IsActive** getter onto graph
   - 2.1.3) Connect IsActive to Branch **Condition**
   - 2.1.4) From Branch **False**: Leave disconnected (abort if not active)

### **3) Validate Player Reference**

#### 3.1) Check Player Valid
   - 3.1.1) From Branch **True** execution:
      - 3.1.1.1) Drag **PlayerRef** getter onto graph
      - 3.1.1.2) Add **Is Valid** node
      - 3.1.1.3) Add **Branch** node
   - 3.1.2) Connect PlayerRef to Is Valid input
   - 3.1.3) Connect Is Valid Return Value to Branch Condition
   - 3.1.4) From Branch **False**: Leave disconnected

### **4) Get Player Location**

#### 4.1) Get Actor Location
   - 4.1.1) From Section 3.1 Branch **True** execution:
      - 4.1.1.1) Drag **PlayerRef** getter onto graph
      - 4.1.1.2) From PlayerRef, add **Get Actor Location** node

### **5) Find Enemies in Range**

#### 5.1) Create Sphere Overlap
   - 5.1.1) From Get Actor Location node (continue execution flow):
      - 5.1.1.1) Drag outward and search: `Sphere Overlap Actors`
      - 5.1.1.2) Add **Sphere Overlap Actors** node
   - 5.1.2) Connect execution
   - 5.1.3) Configure Sphere Overlap:
      - 5.1.3.1) **World Context Object**: Right-click on pin, select **Get a Reference to Self**
      - 5.1.3.2) **Sphere Pos**: Connect Get Actor Location **Return Value**
      - 5.1.3.3) **Sphere Radius**: Connect **ProximityRadius** getter
      - 5.1.3.4) **Object Types**: Click to add:
         - 5.1.3.4.1) Add `Pawn`
      - 5.1.3.5) **Actor Class Filter**: Select `Character`
      - 5.1.3.6) **Actors to Ignore**: Create array with:
         - 5.1.3.6.1) **PlayerRef** (ignore self)
         - 5.1.3.6.2) **FatherRef** (ignore father)

### **6) Loop Through Detected Actors**

#### 6.1) Add For Each Loop
   - 6.1.1) From Sphere Overlap execution:
      - 6.1.1.1) Drag outward and search: `For Each Loop`
      - 6.1.1.2) Add **For Each Loop** node
   - 6.1.2) Connect execution
   - 6.1.3) Connect Sphere Overlap **Out Actors** array to For Each **Array** input

### **7) Get Source ASC for Effect Application**

#### 7.1) Get Father ASC (Source)
   - 7.1.1) Drag **FatherRef** getter onto graph
   - 7.1.2) From FatherRef:
      - 7.1.2.1) Drag outward and search: `Get Ability System Component`
      - 7.1.2.2) Add **Get Ability System Component** node

### **8) Apply Damage with SetByCaller**

#### 8.1) Get Target ASC
   - 8.1.1) From For Each **Array Element** pin:
      - 8.1.1.1) Drag outward and search: `Get Ability System Component`
      - 8.1.1.2) Add **Get Ability System Component** node (Target)

#### 8.2) Validate Target Has ASC
   - 8.2.1) From For Each **Loop Body** execution:
      - 8.2.1.1) Add **Is Valid** node
      - 8.2.1.2) Add **Branch** node
   - 8.2.2) Connect Target ASC Return Value to Is Valid input
   - 8.2.3) Connect Is Valid Return Value to Branch Condition
   - 8.2.4) From Branch **False**: Leave disconnected (skip actor)

#### 8.3) Make Outgoing Gameplay Effect Spec (Contract 24 Compliant)
   - 8.3.1) From Branch **True** execution:
      - 8.3.1.1) Drag outward and search: `Make Outgoing Spec`
      - 8.3.1.2) Add **Make Outgoing Spec** node
   - 8.3.2) Connect execution
   - 8.3.3) Configure:
      - 8.3.3.1) **Target**: Connect Father ASC (Source from step 7.1)
      - 8.3.3.2) **Gameplay Effect Class**: Select `GE_ProximityDamage`
      - 8.3.3.3) **Level**: `1.0`

**Contract 24 Note:** Do NOT add SetByCaller assignment. NarrativeDamageExecCalc captures AttackDamage from Father (source ASC). Damage is determined by Father's AttackDamage attribute value.

#### 8.4) Apply Gameplay Effect Spec to Target
   - 8.4.1) From Make Outgoing Spec execution:
      - 8.4.1.1) Drag outward and search: `Apply Gameplay Effect Spec to Target`
      - 8.4.1.2) Add **Apply Gameplay Effect Spec to Target** node
   - 8.4.2) Connect execution
   - 8.4.3) Configure:
      - 8.4.3.1) **Target**: Connect Target ASC (from step 8.1)
      - 8.4.3.2) **Spec Handle**: Connect Make Outgoing Spec **Return Value**

### **9) Compile and Save**

#### 9.1) Save Function
   - 9.1.1) Click **Compile** button
   - 9.1.2) Click **Save** button

---

## **PHASE 6: IMPLEMENT CLEANUP LOGIC**

### **1) Return to Event Graph**

#### 1.1) Open Event Graph
   - 1.1.1) Click **Event Graph** tab

### **2) Add Event OnEndAbility**

#### 2.1) Add Event Node
   - 2.1.1) Right-click in Event Graph
   - 2.1.2) Search: `Event On End Ability`
   - 2.1.3) Add **Event OnEndAbility** node

### **3) Set IsActive to False**

#### 3.1) Clear Active Flag
   - 3.1.1) From Event OnEndAbility execution:
      - 3.1.1.1) Drag outward and search: `Set IsActive`
      - 3.1.1.2) Add **Set IsActive** node
   - 3.1.2) Connect execution
   - 3.1.3) Uncheck **Is Active** (false)

### **4) Clear Timer**

#### 4.1) Clear and Invalidate Timer
   - 4.1.1) From Set IsActive execution:
      - 4.1.1.1) Drag outward and search: `Clear and Invalidate Timer by Handle`
      - 4.1.1.2) Add **Clear and Invalidate Timer by Handle** node
   - 4.1.2) Connect execution
   - 4.1.3) Connect **DamageTimerHandle** getter to Handle input

### **5) Clear References**

#### 5.1) Clear PlayerRef
   - 5.1.1) From Clear Timer execution:
      - 5.1.1.1) Drag outward and search: `Set PlayerRef`
      - 5.1.1.2) Add **Set PlayerRef** node
   - 5.1.2) Connect execution
   - 5.1.3) Leave value as **None** (default null)

#### 5.2) Clear FatherRef
   - 5.2.1) From Set PlayerRef execution:
      - 5.2.1.1) Drag outward and search: `Set FatherRef`
      - 5.2.1.2) Add **Set FatherRef** node
   - 5.2.2) Connect execution
   - 5.2.3) Leave value as **None** (default null)

#### 5.3) Clear FatherASC
   - 5.3.1) From Set FatherRef execution:
      - 5.3.1.1) Drag outward and search: `Set FatherASC`
      - 5.3.1.2) Add **Set FatherASC** node
   - 5.3.2) Connect execution
   - 5.3.3) Leave value as **None** (default null)

### **6) Compile and Save**

#### 6.1) Save All Changes
   - 6.1.1) Click **Compile** button
   - 6.1.2) Click **Save** button

---

## **PHASE 7: INTEGRATION WITH BP_FATHERSYMBIOTEFORM**

### **1) Open Symbiote EquippableItem**

#### 1.1) Navigate to EquippableItem
   - 1.1.1) In Content Browser, go to `/Content/FatherCompanion/Forms/`
   - 1.1.2) Locate **BP_FatherSymbioteForm**
   - 1.1.3) Double-click to open

### **2) Add GA_ProximityStrike to Abilities Array**

#### 2.1) Open Class Defaults
   - 2.1.1) Click **Class Defaults** button in toolbar

#### 2.2) Find Abilities Property
   - 2.2.1) In Details panel, find **Abilities** category
   - 2.2.2) Locate **Abilities** array property
   - 2.2.3) Click arrow to expand array

#### 2.3) Add New Ability Entry
   - 2.3.1) Click **+ (Plus)** button next to array name
   - 2.3.2) New element appears in array
   - 2.3.3) Expand the new element

#### 2.4) Configure Ability Entry
   - 2.4.1) Find **Ability Class** dropdown
   - 2.4.2) Click dropdown
   - 2.4.3) Search: `GA_ProximityStrike`
   - 2.4.4) Select: **GA_ProximityStrike**

### **3) Compile and Save**

#### 3.1) Compile Blueprint
   - 3.1.1) Click **Compile** button in toolbar

#### 3.2) Save Blueprint
   - 3.2.1) Click **Save** button in toolbar

---

## **PHASE 8: UPDATE GA_FATHERSYMBIOTE FOR ACTIVATION**

### **1) Open GA_FatherSymbiote**

#### 1.1) Navigate to Ability
   - 1.1.1) In Content Browser, go to `/Content/FatherCompanion/Symbiote/Abilities/`
   - 1.1.2) Locate **GA_FatherSymbiote**
   - 1.1.3) Double-click to open

### **2) Add ProximityStrike Activation**

#### 2.1) Find End of Merge Logic
   - 2.1.1) In Event Graph, locate the last node after merge completes
   - 2.1.2) This is after all state variables set and effects applied

#### 2.2) Add Try Activate Ability By Class Node
   - 2.2.1) From last merge logic execution:
      - 2.2.1.1) Drag outward and search: `Try Activate Ability By Class`
      - 2.2.1.2) Add **Try Activate Ability By Class** node
   - 2.2.2) Connect execution

#### 2.3) Configure Activation
   - 2.3.1) Find **Ability Class** dropdown
   - 2.3.2) Click dropdown
   - 2.3.3) Search: `GA_ProximityStrike`
   - 2.3.4) Select: **GA_ProximityStrike**
   - 2.3.5) **Allow Remote Activation**: Unchecked (false)

### **3) Compile and Save**

#### 3.1) Compile Blueprint
   - 3.1.1) Click **Compile** button

#### 3.2) Save Blueprint
   - 3.2.1) Click **Save** button

---

## **CHANGELOG**

### **VERSION 2.12 - Variable Naming Alignment**

**Release Date:** January 2026

| Change Type | Description |
|-------------|-------------|
| DamagePerTick → ProximityDamage | Aligned guide variable name with manifest (more descriptive) |
| DamageRadius → ProximityRadius | Aligned guide variable name with manifest (consistent Proximity prefix) |
| PlayerASC → FatherASC | Corrected ASC reference - Father is damage source per Contract 24 |
| activation_required_tags | Added to manifest per defense-in-depth (3 tags: Symbiote, Merged, Recruited) |
| Variable Summary | Updated all variable names to match manifest |
| EndAbility Cleanup Flow | Updated FatherASC reference |

---

### **VERSION 2.11 - Knockback Removal**

**Release Date:** January 2026

| Change Type | Description |
|-------------|-------------|
| Knockback Removed | Removed knockback feature per Erdem decision (Design Doc v1.3 already documented this) |
| PHASE 5 Section 9 | Removed "Apply Knockback to Each Enemy" section entirely |
| KnockbackForce Variable | Removed from Variables section and Variable Summary |
| Input Method | Changed from "Q Key (Narrative.Input.Ability1)" to "Passive (activated by GA_FatherSymbiote)" |
| Variable Numbering | Fixed incorrect numbering in PHASE 4 (1.5.x through 1.9.x) |

---

### **VERSION 2.10 - Contract 24/27 Compliance**

**Release Date:** January 2026

| Change Type | Description |
|-------------|-------------|
| Contract 24 | Removed SetByCaller damage pattern - FORBIDDEN with NarrativeDamageExecCalc |
| Damage Source | Changed from SetByCaller (Data.Damage.ProximityStrike) to captured AttackDamage attribute |
| ProximityRadius | Updated default from 350.0 to 400.0 (matching tech spec) |
| Tag Removal | Removed Data.Damage.ProximityStrike tag (SetByCaller forbidden) |
| GE_ProximityDamage | Simplified - no Calculation Modifiers, uses default attribute capture |
| PHASE 5 | Removed Section 8.4 (Assign SetByCaller) - no longer needed |
| Documentation | Added Contract 24 notes throughout guide |

---

### **VERSION 2.7 - Blueprint Node Consistency Fixes**

**Release Date:** January 2026

| Change Type | Description |
|-------------|-------------|
| Sphere Overlap | Added World Context Object connection (required pin) |
| Cast to Character | Made Cast to Character explicitly required before Launch Character with proper execution flow (Array Element is AActor*, Launch Character requires ACharacter*) |

---

### **VERSION 2.6 - Reference Updates**

**Release Date:** January 2026

| Change Type | Description |
|-------------|-------------|
| Narrative Pro | Updated version reference from v2.1 to v2.2 |

---

### **VERSION 2.5 - Tag Creation Simplification**

**Release Date:** January 2026

| Change Type | Description |
|-------------|-------------|
| Tag Creation | Simplified PHASE 1 - replaced detailed step-by-step instructions with simple tag list table |

---

### **VERSION 2.3 - Technical Reference v4.5 Alignment**

**Release Date:** December 2025

| Change Type | Description |
|-------------|-------------|
| Father.State.Recruited | Added to Activation Required Tags (3 elements total: Symbiote, Merged, Recruited) |
| FatherASC Variable | Added AbilitySystemComponent reference for Contract 24 damage source (PHASE 4, Section 1.7) |
| PlayerRef Validation | Added Branch node with early exit pattern after storing reference (PHASE 4, Section 2.8) |
| FatherASC Validation | Added Get ASC -> Store -> Validate pattern (PHASE 4, Sections 2.9-2.13) |
| Reference Clearing | Added Set PlayerRef/FatherRef/FatherASC to None in OnEndAbility (PHASE 6, Section 5) |
| Event OnEndAbility | Replaced Event On Ability Ended with Event OnEndAbility for consistency (PHASE 6, Section 2) |
| Form Duration | Updated from 15 to 30 seconds per system design document |
| Quick Reference | Updated with Handle Variables table, EndAbility Cleanup Flow, Related Documents |
| Related Documents | Updated version references (v4.5, v3.5, v1.3) |

---

### **VERSION 2.2 - Activation Method and SetByCaller Pattern**

**Release Date:** November 2025

| Change Type | Description |
|-------------|-------------|
| Ability Tag Structure | Changed from Ability.Father.ProximityStrike to Ability.Father.Symbiote.ProximityStrike |
| Activation Method | Changed from bActivateAbilityOnGranted = true to false with explicit activation |
| Net Execution Policy | Changed from Local Predicted to Server Only |
| Damage Pattern | Changed from Scalable Float to SetByCaller with Data.Damage.ProximityStrike tag |
| PHASE 8 Added | Documents TryActivateAbilityByClass integration in GA_FatherSymbiote |
| Activation/Deactivation Flows | Added tables documenting complete lifecycle |
| SetByCaller Data Tag | Changed from Data.Damage.Proximity to Data.Damage.ProximityStrike |

---

### **VERSION 2.1 - Damage System Correction**

**Release Date:** November 2025

| Change Type | Description |
|-------------|-------------|
| Damage System | Fixed to use NarrativeDamageExecCalc |
| Cancelled Ability References | Removed |
| Granting Method | Updated to EquippableItem |
| bActivateAbilityOnGranted | Added configuration |
| Instance Editable | Added to config variables |
| UTF-8 Cleanup | Replaced special characters with ASCII equivalents |
| GE_ProximityKnockback | Removed |
| Damage Flow | Added table |

---

### **VERSION 2.0 - Symbiote Form Implementation**

**Release Date:** 2025

| Change Type | Description |
|-------------|-------------|
| Initial Implementation | Created for Symbiote form (full body merge berserker state) |
| Passive Activation | Enabled |
| AOE Damage | Every 0.5 seconds |
| Damage Per Tick | 40 (base) |
| Damage Radius | 400 units |
| Max Targets | Unlimited |

---

## **QUICK REFERENCE**

### **Tag Configuration Summary**

| Property | Tags |
|----------|------|
| Ability Tags | `Ability.Father.Symbiote.ProximityStrike` |
| Activation Required | `Effect.Father.FormState.Symbiote`, `Father.State.Merged`, `Father.State.Recruited` |
| Activation Owned | `Father.State.ProximityActive` |
| Activation Blocked | `Father.State.ProximityActive` |
| InputTag | None (passive) |

### **Variable Summary**

| Variable | Type | Category | Instance Editable | Default | Purpose |
|----------|------|----------|-------------------|---------|---------|
| TickRate | Float | Proximity Config | Yes | 0.5 | Seconds between damage ticks |
| ProximityRadius | Float | Proximity Config | Yes | 400.0 | AOE radius in units |
| PlayerRef | Actor Ref | Runtime | No | None | Merged player reference |
| FatherRef | Actor Ref | Runtime | No | None | Father companion reference |
| FatherASC | AbilitySystemComponent Ref | Runtime | No | None | Father ASC for damage source (Contract 24) |
| DamageTimerHandle | Timer Handle | Runtime | No | None | Looping timer reference |
| IsActive | Boolean | Runtime | No | false | Ability active state |

### **Gameplay Effect Summary (Contract 24 Compliant)**

| Effect | Type | Execution Class | Damage Source |
|--------|------|-----------------|---------------|
| GE_ProximityDamage | Instant | NarrativeDamageExecCalc | Father's AttackDamage (captured) |

**Note:** SetByCaller forbidden per Contract 24. Damage from captured AttackDamage attribute.

### **Damage Calculation (Contract 24 Compliant)**

| Parameter | Value |
|-----------|-------|
| Base Damage Per Tick | 50 (from Father's AttackDamage attribute) |
| Tick Rate | 0.5 seconds |
| Ticks Per Second | 2 |
| Base DPS (per enemy) | 100 |
| Total Duration | 30 seconds |
| Total Ticks | 60 |
| Total Base Damage (per enemy) | 3,000 |
| Actual Damage | (AttackDamage * AttackMultiplier) / DefenseMultiplier |

**Contract 24 Note:** Damage comes from Father's AttackDamage attribute (set to 50 via GE_FatherSymbioteStats), NOT SetByCaller.

### **Replication Summary**

| Property | Value |
|----------|-------|
| Instancing Policy | Instanced Per Actor |
| Replication Policy | Replicate |
| Net Execution Policy | Server Only |
| bActivateAbilityOnGranted | false |

### **Blanket Cancel Integration**

| Form Ability | Cancel Abilities with Tag | Cancels GA_ProximityStrike? |
|--------------|---------------------------|----------------------------|
| GA_FatherCrawler | Ability.Father.Symbiote | Yes (hierarchical match) |
| GA_FatherArmor | Ability.Father.Symbiote | Yes (hierarchical match) |
| GA_FatherExoskeleton | Ability.Father.Symbiote | Yes (hierarchical match) |
| GA_FatherEngineer | Ability.Father.Symbiote | Yes (hierarchical match) |

### **EndAbility Cleanup Flow**

| Step | Action |
|------|--------|
| 1 | Event OnEndAbility fires |
| 2 | Set IsActive to false |
| 3 | Clear and Invalidate Timer by Handle (DamageTimerHandle) |
| 4 | Set PlayerRef to None |
| 5 | Set FatherRef to None |
| 6 | Set FatherASC to None |

### **Related Documents**

| Document | Version | Purpose |
|----------|---------|---------|
| Father_Companion_Technical_Reference | v4.5 | Cross-actor patterns, cleanup architecture |
| DefaultGameplayTags_FatherCompanion | v3.5 | Tag definitions |
| Father_Companion_System_Setup_Guide | v1.3 | BP_FatherCompanion setup |
| GA_FatherSymbiote_Implementation_Guide | v2.9 | Form ability that activates ProximityStrike |

---

**END OF GA_PROXIMITYSTRIKE IMPLEMENTATION GUIDE v2.12**

**Symbiote Form - Berserker AOE Damage Aura**

**Unreal Engine 5.7 + Narrative Pro v2.2**

**Blueprint-Only Implementation - Contract 24/27 Compliant**
