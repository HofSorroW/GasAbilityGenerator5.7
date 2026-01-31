# Father Companion - GA_FatherMark Implementation Guide
## VERSION 2.0 - Manifest Alignment (Joint Audit Compliant)
## Unreal Engine 5.7 + Narrative Pro Plugin v2.2

---

## **DOCUMENT INFORMATION**

| Field | Value |
|-------|-------|
| Ability Name | GA_FatherMark |
| Ability Type | Passive (Automatic) |
| Parent Class | NarrativeGameplayAbility |
| Forms | Crawler, Engineer |
| Input | None (Triggers on enemy engagement) |
| Version | 2.0 |
| Engine | Unreal Engine 5.7 |
| Plugin | Narrative Pro v2.2 |

---

## **TABLE OF CONTENTS**

1. [Introduction](#introduction)
2. [Prerequisites](#prerequisites)
3. [PHASE 1: Gameplay Tags Setup](#phase-1-gameplay-tags-setup)
4. [PHASE 2: Create Gameplay Effects](#phase-2-create-gameplay-effects)
5. [PHASE 3: Create GA_FatherMark Ability](#phase-3-create-ga_fathermark-ability)
6. [PHASE 4: Implement Marking Logic](#phase-4-implement-marking-logic)
7. [PHASE 5: Implement Mark Limit System](#phase-5-implement-mark-limit-system)
8. [PHASE 6: BP_FatherCompanion Setup](#phase-6-bp_fathercompanion-setup)
9. [PHASE 7: Grant Ability via AbilityConfiguration](#phase-7-grant-ability-via-abilityconfiguration)
10. [PHASE 8: Integration with Combat Abilities](#phase-8-integration-with-combat-abilities)
11. [Changelog](#changelog)
12. [Quick Reference](#quick-reference)

---

## **INTRODUCTION**

### **Ability Overview**

GA_FatherMark is a passive ability that automatically marks enemies when the father engages them. Marked enemies have a visual indicator above their head and take increased damage from all sources. This ability functions in both Crawler form (following) and Engineer form (turret), enhancing tactical awareness and combat effectiveness.

### **Key Features**

| Feature | Description |
|---------|-------------|
| Automatic Activation | Triggers when father targets enemy |
| Mark Indicator | Widget icon visible above enemy head (even through walls) |
| Damage Amplification | ~10% more damage to marked targets (via Armor reduction) |
| Duration Based | Marks expire after 5 seconds |
| Stack Limit | Maximum 3 enemies marked simultaneously |
| Multi-Form Support | Works in Crawler and Engineer forms |
| Death Cleanup | Marks automatically removed when enemy dies |

### **Technical Specifications**

| Parameter | Value |
|-----------|-------|
| Trigger | Father targets/attacks enemy |
| Mark Duration | 5 seconds |
| Damage Bonus | ~10% (Armor -10 modifier) |
| Max Concurrent Marks | 3 |
| Mark Indicator | WBP_MarkIndicator widget (always visible) |
| Forms Active | Crawler, Engineer |

### **Combat Integration**

| Father Action | Mark Trigger |
|---------------|--------------|
| GA_FatherAttack (melee) | On hit |
| GA_FatherLaserShot | On hit |
| GA_TurretShoot | On hit |
| GA_FatherElectricTrap | On trap activation |

---

## **PREREQUISITES**

### **Required Before This Guide**

| Requirement | Description |
|-------------|-------------|
| BP_FatherCompanion | Father with NarrativeNPCCharacter parent |
| GA_FatherAttack | Crawler melee attack ability |
| GA_FatherLaserShot | Crawler ranged attack ability |
| GA_TurretShoot | Engineer turret attack ability |
| AC_FatherCompanion_Default | AbilityConfiguration data asset |
| WBP_MarkIndicator | Mark indicator widget |

### **Variables Required in BP_FatherCompanion**

| Variable | Type | Purpose |
|----------|------|---------|
| CurrentForm | Enum (EFatherForm) | Active form check |
| MarkedEnemies | Array (Actor Ref) | Track marked enemies |
| MaxMarks | Integer | Limit (3) |

---

## **PHASE 1: GAMEPLAY TAGS SETUP**

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Mark | Father passive enemy marking ability |
| Father.State.Marking | Father is marking an enemy (Activation Owned) |
| Character.Marked | Enemy is marked by father |
| Effect.Father.Mark | Father mark gameplay effect |
| Data.Mark.Duration | SetByCaller duration (default 5.0) |
| Data.Mark.ArmorReduction | SetByCaller armor reduction (default -10.0) |
| Data.Father.Marked | Data tag for marked enemy identification |

### **Verify Existing Tags**

| Tag Name | Purpose |
|----------|---------|
| Father.State.Recruited | Must be recruited (Activation Required) |
| Narrative.State.IsDead | Block when dead (Activation Blocked) |
| Effect.Father.FormState.Armor | Block in Armor form (Activation Blocked) |
| Effect.Father.FormState.Exoskeleton | Block in Exoskeleton form (Activation Blocked) |
| Effect.Father.FormState.Symbiote | Block in Symbiote form (Activation Blocked) |

---

## **PHASE 2: CREATE GAMEPLAY EFFECT**

### **1) Navigate to Effects Folder**

#### 1.1) Open or Create Effects Folder
   - 1.1.1) In Content Browser, go to `/Content/FatherCompanion/Effects/Utility/`
   - 1.1.2) Create folder structure if missing

### **2) Create GE_MarkEffect (Single Combined GE)**

> **Note:** Per manifest.yaml authority, mark uses a SINGLE gameplay effect (GE_MarkEffect) with SetByCaller pattern for duration and armor reduction. This replaces the previous dual-GE approach.

#### 2.1) Create Gameplay Effect Asset
   - 2.1.1) Right-click in Utility folder
   - 2.1.2) Select **Gameplay** -> **Gameplay Effect**
   - 2.1.3) Name: `GE_MarkEffect`
   - 2.1.4) Press **Enter**
   - 2.1.5) Double-click to open

#### 2.2) Configure Duration Policy (SetByCaller)
   - 2.2.1) Click **Class Defaults** button
   - 2.2.2) In Details panel, find **Gameplay Effect** section
   - 2.2.3) Set **Duration Policy**: `Has Duration`
   - 2.2.4) Find **Duration Magnitude** section
   - 2.2.5) Set **Magnitude Calculation Type**: `Set By Caller`
   - 2.2.6) Set **Set By Caller Magnitude** -> **Data Tag**: `Data.Mark.Duration`

#### 2.3) Add Armor Modifier (SetByCaller)
   - 2.3.1) In Components section, click **+ (Plus)**
   - 2.3.2) Search: `Modifiers`
   - 2.3.3) Select: **Modifiers**
   - 2.3.4) Click Modifiers component to expand
   - 2.3.5) Find **Modifiers** array
   - 2.3.6) Click **+ (Plus)** to add modifier
   - 2.3.7) Expand the modifier:
      - 2.3.7.1) **Attribute**: Select `NarrativeAttributeSetBase.Armor`
      - 2.3.7.2) **Modifier Op**: Select `Add`
      - 2.3.7.3) **Modifier Magnitude** -> **Magnitude Calculation Type**: `Set By Caller`
      - 2.3.7.4) Set **Set By Caller Magnitude** -> **Data Tag**: `Data.Mark.ArmorReduction`

> **Armor Math Note:** The damage increase varies by target armor: maximum ~10% benefit on low-armor targets (10 armor → 0); reduced benefit on high-armor targets (100 armor → 90 = ~5.3%); no benefit if target armor is already 0 due to engine clamping.

#### 2.4) Add Grant Tags Component
   - 2.4.1) Click **+ (Plus)** in Components
   - 2.4.2) Search: `Grant Tags`
   - 2.4.3) Select: **Grant Tags to Target Actor**

#### 2.5) Configure Granted Tags
   - 2.5.1) Click component to expand
   - 2.5.2) Find **Add Tags** -> **Add to Inherited**
   - 2.5.3) Click **+ (Plus)**
   - 2.5.4) Add tag: `Character.Marked`

#### 2.6) Add Asset Tags Component
   - 2.6.1) Click **+ (Plus)** in Components
   - 2.6.2) Search: `Asset Tags`
   - 2.6.3) Select: **Tags This Effect Has (Asset Tags)**

#### 2.7) Configure Asset Tags
   - 2.7.1) Click component to expand
   - 2.7.2) Find **Add to Inherited**
   - 2.7.3) Click **+ (Plus)**
   - 2.7.4) Add tag: `Effect.Father.Mark`

#### 2.8) Add Stacking Component
   - 2.8.1) Click **+ (Plus)** in Components
   - 2.8.2) Search: `Stacking`
   - 2.8.3) Select: **Stacking**

#### 2.9) Configure Stacking (Refresh on Re-Apply)
   - 2.9.1) Click component to expand
   - 2.9.2) Set **Stacking Type**: `Aggregate by Target`
   - 2.9.3) Set **Stack Limit Count**: `1`
   - 2.9.4) Set **Stack Duration Refresh Policy**: `Refresh on Successful Application`
   - 2.9.5) Set **Stack Period Reset Policy**: `Reset on Successful Application`

#### 2.10) Compile and Save
   - 2.10.1) Click **Compile** button
   - 2.10.2) Click **Save** button

---

## **PHASE 3: CREATE GA_FATHERMARK ABILITY**

### **1) Navigate to Abilities Folder**

#### 1.1) Open Abilities Folder
   - 1.1.1) In Content Browser, go to `/Content/FatherCompanion/General/Abilities/`
   - 1.1.2) Create folder structure if missing

### **2) Create GA_FatherMark Blueprint**

#### 2.1) Create Blueprint Class
   - 2.1.1) Right-click in Abilities folder
   - 2.1.2) Select **Blueprint Class**
   - 2.1.3) In **Pick Parent Class** dialog:
      - 2.1.3.1) Click **All Classes** dropdown
      - 2.1.3.2) Search: `NarrativeGameplayAbility`
      - 2.1.3.3) Select **NarrativeGameplayAbility**
      - 2.1.3.4) Click **Select**
   - 2.1.4) Name: `GA_FatherMark`
   - 2.1.5) Press **Enter**
   - 2.1.6) Double-click to open

### **3) Configure Class Defaults**

#### 3.1) Open Class Defaults Panel
   - 3.1.1) Click **Class Defaults** button in toolbar

### **4) Configure Ability Tags**

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Mark |
| Activation Required Tags | Father.State.Recruited |
| Activation Blocked Tags | Father.State.Transitioning, Narrative.State.IsDead, Effect.Father.FormState.Armor, Effect.Father.FormState.Exoskeleton, Effect.Father.FormState.Symbiote |
| Activation Owned Tags | Father.State.Marking |

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

### **6) Configure InputTag (None - Passive)**

#### 6.1) Set InputTag to None
   - 6.1.1) In Details panel, find **Narrative Ability** category
   - 6.1.2) Find **Input Tag** property
   - 6.1.3) Set to: `None` (or leave empty)

### **7) Compile and Save**

#### 7.1) Save Configuration
   - 7.1.1) Click **Compile** button
   - 7.1.2) Click **Save** button

---

## **PHASE 4: IMPLEMENT MARKING LOGIC**

### **1) Create Ability Variables**

#### 1.1) Open Event Graph
   - 1.1.1) In GA_FatherMark Blueprint, click **Event Graph** tab

#### 1.2) Create FatherRef Variable
   - 1.2.1) In My Blueprint, click **+ (Plus)** next to Variables
   - 1.2.2) Name: `FatherRef`
   - 1.2.3) **Variable Type**: `BP_FatherCompanion` -> `Object Reference`
   - 1.2.4) Set **Category**: `Runtime`

#### 1.3) Create TargetEnemy Variable
   - 1.3.1) Click **+ (Plus)** next to Variables
   - 1.3.2) Name: `TargetEnemy`
   - 1.3.3) **Variable Type**: `Actor` -> `Object Reference`
   - 1.3.4) Set **Category**: `Runtime`

#### 1.4) Create MarkDuration Variable (SetByCaller)
   - 1.4.1) Click **+ (Plus)** next to Variables
   - 1.4.2) Name: `MarkDuration`
   - 1.4.3) **Variable Type**: `Float`
   - 1.4.4) Click **Compile**
   - 1.4.5) Set **Default Value**: `5.0`
   - 1.4.6) Enable **Instance Editable**: Check
   - 1.4.7) Set **Category**: `Config`

#### 1.5) Create MarkArmorReduction Variable (SetByCaller)
   - 1.5.1) Click **+ (Plus)** next to Variables
   - 1.5.2) Name: `MarkArmorReduction`
   - 1.5.3) **Variable Type**: `Float`
   - 1.5.4) Click **Compile**
   - 1.5.5) Set **Default Value**: `-10.0`
   - 1.5.6) Enable **Instance Editable**: Check
   - 1.5.7) Set **Category**: `Config`

### **2) Implement ActivateAbility Event**

#### 2.1) Add Event ActivateAbility
   - 2.1.1) Right-click in Event Graph
   - 2.1.2) Search: `Event ActivateAbility`
   - 2.1.3) Add **Event ActivateAbility** node

#### 2.2) Get Avatar Actor (Father)
   - 2.2.1) From **Event ActivateAbility** node:
      - 2.2.1.1) Find **Actor Info** pin
      - 2.2.1.2) Drag outward and search: `Get Avatar Actor from Actor Info`
      - 2.2.1.3) Add **Get Avatar Actor from Actor Info** node

#### 2.3) Cast to BP_FatherCompanion
   - 2.3.1) From Event execution pin:
      - 2.3.1.1) Drag outward and search: `Cast To BP_FatherCompanion`
      - 2.3.1.2) Add **Cast To BP_FatherCompanion** node
   - 2.3.2) Connect:
      - 2.3.2.1) Get Avatar Actor Return Value -> Cast Object input

#### 2.4) Store Father Reference
   - 2.4.1) From **As BP Father Companion** pin:
      - 2.4.1.1) Drag outward and search: `Set Father Ref`
      - 2.4.1.2) Add **Set FatherRef** node
   - 2.4.2) Connect execution from Cast node

#### 2.5) Get Target from PendingMarkTarget
   - 2.5.1) From FatherRef:
      - 2.5.1.1) Drag outward and search: `Get Pending Mark Target`
      - 2.5.1.2) Add **Get PendingMarkTarget** node
   - 2.5.2) From Return Value:
      - 2.5.2.1) Drag outward and search: `Set Target Enemy`
      - 2.5.2.2) Add **Set TargetEnemy** node
   - 2.5.3) Connect execution from Set FatherRef

### **3) Validate Target**

#### 3.1) Check Target Valid
   - 3.1.1) From TargetEnemy getter:
      - 3.1.1.1) Drag outward and search: `Is Valid`
      - 3.1.1.2) Add **Is Valid** node
   - 3.1.2) From Set TargetEnemy execution:
      - 3.1.2.1) Add **Branch** node
   - 3.1.3) Connect Is Valid return to Branch Condition
   - 3.1.4) From Branch **False**:
      - 3.1.4.1) Add **End Ability** node
      - 3.1.4.2) **Was Cancelled**: Check (true)

### **4) Check if Already Marked**

#### 4.1) Get Target ASC
   - 4.1.1) From Branch **True** execution:
      - 4.1.1.1) From TargetEnemy getter, drag outward
      - 4.1.1.2) Search: `Get Ability System Component`
      - 4.1.1.3) Add **Get Ability System Component** node

#### 4.2) Check for Marked Tag
   - 4.2.1) From Target ASC:
      - 4.2.1.1) Drag outward and search: `Has Gameplay Tag`
      - 4.2.1.2) Add **Has Gameplay Tag** node
   - 4.2.2) Configure:
      - 4.2.2.1) **Tag to Check**: `Character.Marked`

#### 4.3) Branch on Already Marked
   - 4.3.1) From Get ASC execution:
      - 4.3.1.1) Add **Branch** node
   - 4.3.2) Connect Has Gameplay Tag return to Condition
   - 4.3.3) From Branch **True** (already marked):
      - 4.3.3.1) Connect to refresh logic (re-apply GE to refresh duration)
   - 4.3.4) From Branch **False** (not marked):
      - 4.3.4.1) Connect to new mark logic (Phase 5)

### **5) Compile and Save Progress**

#### 5.1) Save Work
   - 5.1.1) Click **Compile** button
   - 5.1.2) Click **Save** button

---

## **PHASE 5: IMPLEMENT MARK LIMIT SYSTEM**

### **1) Check Mark Count (For New Marks)**

#### 1.1) Get Marked Enemies Array
   - 1.1.1) From **FatherRef** getter:
      - 1.1.1.1) Drag outward and search: `Get Marked Enemies`
      - 1.1.1.2) Add **Get Marked Enemies** node (array from BP_FatherCompanion)

#### 1.2) Get Array Length
   - 1.2.1) From Marked Enemies Return Value:
      - 1.2.1.1) Drag outward and search: `Length`
      - 1.2.1.2) Add **Length** node

#### 1.3) Compare to Max
   - 1.3.1) From FatherRef:
      - 1.3.1.1) Drag outward and search: `Get Max Marks`
      - 1.3.1.2) Add **Get Max Marks** node
   - 1.3.2) Add **Integer >= Integer** (Greater or Equal) node
   - 1.3.3) Connect:
      - 1.3.3.1) Array Length to first input
      - 1.3.3.2) Max Marks to second input

#### 1.4) Branch on Limit
   - 1.4.1) From Branch **False** (not already marked) execution:
      - 1.4.1.1) Add **Branch** node
   - 1.4.2) Connect comparison result to Condition
   - 1.4.3) From Branch **True** (at limit):
      - 1.4.3.1) Connect to remove oldest mark logic
   - 1.4.4) From Branch **False** (under limit):
      - 1.4.4.1) Connect directly to apply mark logic

### **2) Remove Oldest Mark (When At Limit)**

#### 2.1) Get First Element
   - 2.1.1) From Branch True execution (at limit):
      - 2.1.1.1) From Marked Enemies array, drag outward
      - 2.1.1.2) Search: `Get (a copy)`
      - 2.1.1.3) Add **Get (a copy)** node
   - 2.1.2) Set **Index**: `0` (oldest mark)

#### 2.2) Validate Oldest Enemy
   - 2.2.1) From Get Return Value:
      - 2.2.1.1) Drag outward and search: `Is Valid`
      - 2.2.1.2) Add **Is Valid** node
   - 2.2.2) Add **Branch** node
   - 2.2.3) Connect Is Valid return to Condition
   - 2.2.4) From Branch **False** (invalid reference):
      - 2.2.4.1) Connect to RemoveFromMarkedEnemies to clean array
      - 2.2.4.2) Loop back to check limit again

#### 2.3) Remove Mark Effect from Oldest
   - 2.3.1) From Branch **True** (valid enemy):
      - 2.3.1.1) From oldest enemy, drag outward
      - 2.3.1.2) Search: `Get Ability System Component`
      - 2.3.1.3) Add **Get Ability System Component** node

#### 2.4) Remove GE_MarkEffect from Oldest
   - 2.4.1) From Get ASC execution:
      - 2.4.1.1) From oldest enemy ASC, drag outward
      - 2.4.1.2) Search: `Remove Active Gameplay Effects With Granted Tags`
      - 2.4.1.3) Add **Remove Active Gameplay Effects With Granted Tags** node
   - 2.4.2) Connect execution
   - 2.4.3) Configure:
      - 2.4.3.1) Create **Gameplay Tag Container**
      - 2.4.3.2) Add tag: `Character.Marked`

#### 2.5) Destroy Oldest Mark Widget
   - 2.5.1) From Remove GE execution:
      - 2.5.1.1) From FatherRef, drag outward
      - 2.5.1.2) Search: `Destroy Mark Widget For Enemy`
      - 2.5.1.3) Add **DestroyMarkWidgetForEnemy** node (custom function)
   - 2.5.2) Connect execution
   - 2.5.3) Connect oldest enemy to Enemy input

#### 2.6) Remove from Array
   - 2.6.1) From Destroy Widget execution:
      - 2.6.1.1) From FatherRef, drag outward
      - 2.6.1.2) Search: `Remove from Marked Enemies`
      - 2.6.1.3) Add **Remove from Marked Enemies** node (custom function)
   - 2.6.2) Connect execution
   - 2.6.3) Connect oldest enemy to item to remove

### **3) Apply Mark to New Target**

#### 3.1) Create Apply Mark Sequence
   - 3.1.1) Add **Sequence** node after limit check
   - 3.1.2) Connect both paths (under limit, after remove oldest) to Sequence

#### 3.2) Get Target ASC
   - 3.2.1) From **TargetEnemy** getter:
      - 3.2.1.1) Drag outward and search: `Get Ability System Component`
      - 3.2.1.2) Add **Get Ability System Component** node

#### 3.3) Make Outgoing GE Spec for GE_MarkEffect
   - 3.3.1) From Sequence **Then 0** execution:
      - 3.3.1.1) Drag outward and search: `Make Outgoing Gameplay Effect Spec`
      - 3.3.1.2) Add **Make Outgoing Gameplay Effect Spec** node
   - 3.3.2) Connect execution
   - 3.3.3) Configure:
      - 3.3.3.1) **Gameplay Effect Class**: `GE_MarkEffect`
      - 3.3.3.2) **Level**: `1.0`

#### 3.4) Set Duration via SetByCaller
   - 3.4.1) From Make Outgoing GE Spec execution:
      - 3.4.1.1) From Return Value (Spec Handle), drag outward
      - 3.4.1.2) Search: `Assign Tag Set By Caller Magnitude`
      - 3.4.1.3) Add **Assign Tag Set By Caller Magnitude** node
   - 3.4.2) Connect execution
   - 3.4.3) Configure:
      - 3.4.3.1) **Data Tag**: `Data.Mark.Duration`
      - 3.4.3.2) **Magnitude**: Connect to **MarkDuration** variable getter

#### 3.5) Set Armor Reduction via SetByCaller
   - 3.5.1) From Assign Duration execution:
      - 3.5.1.1) From Spec Handle, drag outward
      - 3.5.1.2) Add another **Assign Tag Set By Caller Magnitude** node
   - 3.5.2) Connect execution
   - 3.5.3) Configure:
      - 3.5.3.1) **Data Tag**: `Data.Mark.ArmorReduction`
      - 3.5.3.2) **Magnitude**: Connect to **MarkArmorReduction** variable getter

#### 3.6) Apply GE_MarkEffect to Target
   - 3.6.1) From Assign Armor execution:
      - 3.6.1.1) From Target ASC, drag outward
      - 3.6.1.2) Search: `Apply Gameplay Effect Spec to Self`
      - 3.6.1.3) Add **Apply Gameplay Effect Spec to Self** node
   - 3.6.2) Connect execution
   - 3.6.3) Connect Spec Handle to Spec input

#### 3.5) Add to Marked Array
   - 3.5.1) From Sequence **Then 1** execution:
      - 3.5.1.1) From FatherRef, drag outward
      - 3.5.1.2) Search: `Add to Marked Enemies`
      - 3.5.1.3) Add **Add to Marked Enemies** node (custom function)
   - 3.5.2) Connect execution
   - 3.5.3) Connect TargetEnemy to item to add

### **4) Spawn Mark Indicator Widget**

#### 4.1) Create Widget Component
   - 4.1.1) From Add to Marked execution:
      - 4.1.1.1) From TargetEnemy, drag outward
      - 4.1.1.2) Search: `Add Widget Component`
      - 4.1.1.3) Add **Add Widget Component** node (using Add Component by Class)
   - 4.1.2) Connect execution

#### 4.2) Configure Widget Component
   - 4.2.1) From Add Widget Component Return Value:
      - 4.2.1.1) Drag outward and search: `Set Widget Class`
      - 4.2.1.2) Add **Set Widget Class** node
   - 4.2.2) Connect execution
   - 4.2.3) Configure:
      - 4.2.3.1) **New Widget Class**: `WBP_MarkIndicator`

#### 4.3) Set Widget Space
   - 4.3.1) From Set Widget Class execution:
      - 4.3.1.1) From widget component, drag outward
      - 4.3.1.2) Search: `Set Widget Space`
      - 4.3.1.3) Add **Set Widget Space** node
   - 4.3.2) Connect execution
   - 4.3.3) Configure:
      - 4.3.3.1) **New Space**: `Screen`

#### 4.4) Set Draw Size
   - 4.4.1) From Set Widget Space execution:
      - 4.4.1.1) From widget component, drag outward
      - 4.4.1.2) Search: `Set Draw Size`
      - 4.4.1.3) Add **Set Draw Size** node
   - 4.4.2) Connect execution
   - 4.4.3) Configure:
      - 4.4.3.1) **Size X**: `32`
      - 4.4.3.2) **Size Y**: `32`

#### 4.5) Set Relative Location (Above Head)
   - 4.5.1) From Set Draw Size execution:
      - 4.5.1.1) From widget component, drag outward
      - 4.5.1.2) Search: `Set Relative Location`
      - 4.5.1.3) Add **Set Relative Location** node
   - 4.5.2) Connect execution
   - 4.5.3) Configure:
      - 4.5.3.1) **New Location X**: `0`
      - 4.5.3.2) **New Location Y**: `0`
      - 4.5.3.3) **New Location Z**: `100` (above head)

#### 4.6) Store Widget Reference in Father
   - 4.6.1) From Set Relative Location execution:
      - 4.6.1.1) From FatherRef, drag outward
      - 4.6.1.2) Search: `Store Mark Widget`
      - 4.6.1.3) Add **StoreMarkWidget** node (custom function)
   - 4.6.2) Connect execution
   - 4.6.3) Connect:
      - 4.6.3.1) TargetEnemy to Enemy input
      - 4.6.3.2) Widget Component Return Value to Widget input

### **5) Bind to Enemy Death Event**

#### 5.1) Get Enemy ASC for Delegate
   - 5.1.1) From Store Mark Widget execution:
      - 5.1.1.1) From TargetEnemy, drag outward
      - 5.1.1.2) Search: `Get Ability System Component`
      - 5.1.1.3) Add **Get Ability System Component** node
   - 5.1.2) Connect execution

#### 5.2) Bind to OnDied Delegate
   - 5.2.1) From Target ASC:
      - 5.2.1.1) Drag outward and search: `Bind Event to On Died`
      - 5.2.1.2) Add **Bind Event to OnDied** node
   - 5.2.2) Connect execution
   - 5.2.3) From **Event** pin:
      - 5.2.3.1) Drag outward and select **Create Event**
      - 5.2.3.2) Select **Create a matching function**
      - 5.2.3.3) Name function: `OnMarkedEnemyDied`

#### 5.3) Implement OnMarkedEnemyDied Function
   - 5.3.1) In the new function graph:
      - 5.3.1.1) From FatherRef getter, drag outward
      - 5.3.1.2) Search: `Handle Marked Enemy Death`
      - 5.3.1.3) Add **HandleMarkedEnemyDeath** node (custom function in BP_FatherCompanion)
   - 5.3.2) Connect execution
   - 5.3.3) Connect the enemy parameter from delegate to Enemy input

### **6) End Ability**

#### 6.1) Commit Ability
   - 6.1.1) From Bind Event execution:
      - 6.1.1.1) Drag outward and search: `Commit Ability`
      - 6.1.1.2) Add **Commit Ability** node
   - 6.1.2) Connect execution

#### 6.2) End Ability
   - 6.2.1) From Commit Ability execution:
      - 6.2.1.1) Drag outward and search: `End Ability`
      - 6.2.1.2) Add **End Ability** node
   - 6.2.2) Connect execution
   - 6.2.3) **Was Cancelled**: Uncheck (false)

### **7) Handle Refresh Path (Already Marked)**

#### 7.1) Refresh GE Duration
   - 7.1.1) From Branch **True** (already marked) earlier:
      - 7.1.1.1) GE_MarkEffect has **Refresh on Successful Application** stacking policy
      - 7.1.1.2) Re-apply GE_MarkEffect (same Make Spec + SetByCaller + Apply logic as Section 3.3-3.6)
      - 7.1.1.3) This single apply refreshes both tag and armor reduction

#### 7.2) Refresh Widget Timer
   - 7.2.1) From FatherRef:
      - 7.2.1.1) Drag outward and search: `Get Mark Widget Map`
      - 7.2.1.2) Add **Get MarkWidgetMap** node
   - 7.2.2) From MarkWidgetMap Return Value:
      - 7.2.2.1) Drag outward and search: `Find`
      - 7.2.2.2) Add **Find** node
      - 7.2.2.3) Key: Connect TargetEnemy
   - 7.2.3) From Find Return Value (Widget Component):
      - 7.2.3.1) Drag outward and search: `Get Widget`
      - 7.2.3.2) Add **Get Widget** node
   - 7.2.4) From Get Widget Return Value:
      - 7.2.4.1) Drag outward and search: `Cast To WBP_MarkIndicator`
      - 7.2.4.2) Add **Cast To WBP_MarkIndicator** node
   - 7.2.5) From Cast success (As WBP Mark Indicator):
      - 7.2.5.1) Drag outward and search: `Refresh Mark`
      - 7.2.5.2) Add **Refresh Mark** node
   - 7.2.6) Connect execution from Apply GE_MarkDamageBonus

#### 7.3) End Refresh Path
   - 7.3.1) Skip the array add (already in array)
   - 7.3.2) Skip widget spawn (already has widget)
   - 7.3.3) Connect to Commit Ability and End Ability

### **8) Compile and Save**

#### 8.1) Save All Changes
   - 8.1.1) Click **Compile** button
   - 8.1.2) Click **Save** button

---

## **PHASE 6: BP_FATHERCOMPANION SETUP**

### **1) Open BP_FatherCompanion**

#### 1.1) Navigate to Father Blueprint
   - 1.1.1) In Content Browser, go to `/Content/FatherCompanion/Characters/`
   - 1.1.2) Double-click **BP_FatherCompanion** to open

### **2) Add Mark Tracking Variables**

#### 2.1) Create MarkedEnemies Array Variable
   - 2.1.1) In My Blueprint, click **+ (Plus)** next to Variables
   - 2.1.2) Name: `MarkedEnemies`
   - 2.1.3) **Variable Type**: `Actor` -> `Object Reference`
   - 2.1.4) Click array icon to make it an **Array**
   - 2.1.5) Enable **Replication**: Replicated
   - 2.1.6) Set **Category**: `Mark System`

#### 2.2) Create MaxMarks Variable
   - 2.2.1) Click **+ (Plus)** next to Variables
   - 2.2.2) Name: `MaxMarks`
   - 2.2.3) **Variable Type**: `Integer`
   - 2.2.4) Click **Compile**
   - 2.2.5) Set **Default Value**: `3`
   - 2.2.6) Set **Category**: `Mark System`

#### 2.3) Create PendingMarkTarget Variable
   - 2.3.1) Click **+ (Plus)** next to Variables
   - 2.3.2) Name: `PendingMarkTarget`
   - 2.3.3) **Variable Type**: `Actor` -> `Object Reference`
   - 2.3.4) Set **Category**: `Mark System`

#### 2.4) Create MarkWidgetMap Variable
   - 2.4.1) Click **+ (Plus)** next to Variables
   - 2.4.2) Name: `MarkWidgetMap`
   - 2.4.3) **Variable Type**: `Actor` -> `Object Reference` (Key)
   - 2.4.4) Click **Map** type selector
   - 2.4.5) Set Value Type: `Widget Component` -> `Object Reference`
   - 2.4.6) Set **Category**: `Mark System`

### **3) Create Helper Functions**

#### 3.1) Create AddToMarkedEnemies Function
   - 3.1.1) In My Blueprint, click **+ (Plus)** next to Functions
   - 3.1.2) Name: `AddToMarkedEnemies`
   - 3.1.3) Press **Enter**
   - 3.1.4) Add Input:
      - 3.1.4.1) Name: `Enemy`
      - 3.1.4.2) Type: `Actor` -> `Object Reference`
   - 3.1.5) In function graph:
      - 3.1.5.1) From function input, add **Add Unique** node
      - 3.1.5.2) Connect to MarkedEnemies array
      - 3.1.5.3) Connect Enemy input to element

#### 3.2) Create RemoveFromMarkedEnemies Function
   - 3.2.1) Click **+ (Plus)** next to Functions
   - 3.2.2) Name: `RemoveFromMarkedEnemies`
   - 3.2.3) Add Input:
      - 3.2.3.1) Name: `Enemy`
      - 3.2.3.2) Type: `Actor` -> `Object Reference`
   - 3.2.4) In function graph:
      - 3.2.4.1) From function input, add **Remove Item** node
      - 3.2.4.2) Connect to MarkedEnemies array
      - 3.2.4.3) Connect Enemy input to item

#### 3.3) Create MarkEnemy Function (Convenience)
   - 3.3.1) Click **+ (Plus)** next to Functions
   - 3.3.2) Name: `MarkEnemy`
   - 3.3.3) Add Input:
      - 3.3.3.1) Name: `Enemy`
      - 3.3.3.2) Type: `Actor` -> `Object Reference`
   - 3.3.4) In function graph:
      - 3.3.4.1) Add **Set PendingMarkTarget** node
      - 3.3.4.2) Connect Enemy input to value
      - 3.3.4.3) From execution, add **Get Ability System Component** (self)
      - 3.3.4.4) From ASC, add **Try Activate Ability by Class** node
      - 3.3.4.5) **Ability Class**: `GA_FatherMark`

#### 3.4) Create StoreMarkWidget Function
   - 3.4.1) Click **+ (Plus)** next to Functions
   - 3.4.2) Name: `StoreMarkWidget`
   - 3.4.3) Add Inputs:
      - 3.4.3.1) Name: `Enemy`, Type: `Actor` -> `Object Reference`
      - 3.4.3.2) Name: `Widget`, Type: `Widget Component` -> `Object Reference`
   - 3.4.4) In function graph:
      - 3.4.4.1) From function input, add **Add** node for MarkWidgetMap
      - 3.4.4.2) Connect Enemy to Key
      - 3.4.4.3) Connect Widget to Value

#### 3.5) Create DestroyMarkWidgetForEnemy Function
   - 3.5.1) Click **+ (Plus)** next to Functions
   - 3.5.2) Name: `DestroyMarkWidgetForEnemy`
   - 3.5.3) Add Input:
      - 3.5.3.1) Name: `Enemy`, Type: `Actor` -> `Object Reference`
   - 3.5.4) In function graph:
      - 3.5.4.1) From function input, add **Find** node for MarkWidgetMap
      - 3.5.4.2) Connect Enemy to Key
      - 3.5.4.3) From Return Value, add **Is Valid** node
      - 3.5.4.4) Add **Branch** node with Is Valid result
      - 3.5.4.5) From Branch True:
         - 3.5.4.5.1) Add **Destroy Component** node
         - 3.5.4.5.2) Connect found widget to Target
      - 3.5.4.6) From Destroy Component execution:
         - 3.5.4.6.1) Add **Remove** node for MarkWidgetMap
         - 3.5.4.6.2) Connect Enemy to Key

#### 3.6) Create HandleMarkedEnemyDeath Function
   - 3.6.1) Click **+ (Plus)** next to Functions
   - 3.6.2) Name: `HandleMarkedEnemyDeath`
   - 3.6.3) Add Input:
      - 3.6.3.1) Name: `Enemy`, Type: `Actor` -> `Object Reference`
   - 3.6.4) In function graph:
      - 3.6.4.1) Call **DestroyMarkWidgetForEnemy** with Enemy
      - 3.6.4.2) Call **RemoveFromMarkedEnemies** with Enemy

### **4) Compile and Save**

#### 4.1) Save Blueprint
   - 4.1.1) Click **Compile** button
   - 4.1.2) Click **Save** button

---

## **PHASE 7: GRANT ABILITY VIA ABILITYCONFIGURATION**

### **1) Open AbilityConfiguration Asset**

#### 1.1) Navigate to Configuration Asset
   - 1.1.1) In Content Browser, go to `/Content/FatherCompanion/Data/`
   - 1.1.2) Double-click **AC_FatherCompanion_Default** to open

### **2) Add GA_FatherMark to Default Abilities**

#### 2.1) Find Default Abilities Array
   - 2.1.1) In Details panel, find **Default Abilities** array

#### 2.2) Add New Entry
   - 2.2.1) Click **+ (Plus)** button to add element
   - 2.2.2) Click dropdown on new element
   - 2.2.3) Search: `GA_FatherMark`
   - 2.2.4) Select **GA_FatherMark**

### **3) Save Configuration**

#### 3.1) Save Asset
   - 3.1.1) Click **Save** button

---

## **PHASE 8: INTEGRATION WITH COMBAT ABILITIES**

### **1) Update GA_FatherAttack**

#### 1.1) Open GA_FatherAttack
   - 1.1.1) Navigate to `/Content/FatherCompanion/Crawler/Abilities/`
   - 1.1.2) Double-click **GA_FatherAttack** to open

#### 1.2) Find Damage Application Section
   - 1.2.1) In Event Graph, locate where damage is applied to enemy

#### 1.3) Add Mark Call After Damage
   - 1.3.1) After Apply Damage execution:
      - 1.3.1.1) From FatherRef, drag outward
      - 1.3.1.2) Search: `Mark Enemy`
      - 1.3.1.3) Add **Mark Enemy** node
   - 1.3.2) Connect execution
   - 1.3.3) Connect damaged enemy actor to Enemy input

#### 1.4) Compile and Save
   - 1.4.1) Click **Compile** button
   - 1.4.2) Click **Save** button

### **2) Update GA_FatherLaserShot**

#### 2.1) Open GA_FatherLaserShot
   - 2.1.1) Navigate to `/Content/FatherCompanion/Crawler/Abilities/`
   - 2.1.2) Double-click **GA_FatherLaserShot** to open

#### 2.2) Find Hit Detection Section
   - 2.2.1) Locate where projectile hit is processed

#### 2.3) Add Mark Call After Hit
   - 2.3.1) After damage application:
      - 2.3.1.1) From FatherRef, add **Mark Enemy** node
   - 2.3.2) Connect execution
   - 2.3.3) Connect hit actor to Enemy input

#### 2.4) Compile and Save
   - 2.4.1) Click **Compile** button
   - 2.4.2) Click **Save** button

### **3) Update GA_TurretShoot**

#### 3.1) Open GA_TurretShoot
   - 3.1.1) Navigate to `/Content/FatherCompanion/Engineer/Abilities/`
   - 3.1.2) Double-click **GA_TurretShoot** to open

#### 3.2) Find Damage Application Section
   - 3.2.1) Locate where turret damage is applied

#### 3.3) Add Mark Call After Damage
   - 3.3.1) After damage application:
      - 3.3.1.1) From FatherRef (or turret owner reference), add **Mark Enemy** node
   - 3.3.2) Connect execution
   - 3.3.3) Connect damaged enemy to Enemy input

#### 3.4) Compile and Save
   - 3.4.1) Click **Compile** button
   - 3.4.2) Click **Save** button

### **4) Update GA_FatherElectricTrap**

#### 4.1) Open GA_FatherElectricTrap
   - 4.1.1) Navigate to `/Content/FatherCompanion/Engineer/Abilities/`
   - 4.1.2) Double-click **GA_FatherElectricTrap** to open

#### 4.2) Find Trap Trigger Section
   - 4.2.1) Locate where trap damage is applied to enemies

#### 4.3) Add Mark Call After Trigger
   - 4.3.1) After damage application:
      - 4.3.1.1) From FatherRef (or trap owner reference), add **Mark Enemy** node
   - 4.3.2) Connect execution
   - 4.3.3) Connect affected enemy to Enemy input

#### 4.4) Compile and Save
   - 4.4.1) Click **Compile** button
   - 4.4.2) Click **Save** button

---

## **CHANGELOG**

| Version | Date | Changes |
|---------|------|---------|
| 2.0 | January 2026 | **Manifest Alignment (Joint Audit Compliant):** (1) Replaced dual-GE pattern (GE_FatherMark + GE_MarkDamageBonus) with single GE_MarkEffect per manifest authority. (2) Changed tag from `Enemy.State.Marked` to `Character.Marked` per tag registry. (3) Added `Father.State.Transitioning` to Activation Blocked Tags. (4) Added SetByCaller variables (MarkDuration, MarkArmorReduction) with Data.Mark.* tags. (5) Clarified armor math as conditional (~10% only for low-armor targets). (6) Updated PHASE 2 to use SetByCaller pattern. (7) Removed MarkWidgetRef variable, replaced with SetByCaller config variables. |
| 1.9 | January 2026 | Form State Tag Update (INV-1 Compliant): Changed Activation Blocked Tags from `Father.Form.*` (orphan tags) to `Effect.Father.FormState.*` (granted by form state GEs). Updated UE version from 5.6 to 5.7. |
| 1.8 | January 2026 | Updated Narrative Pro version reference from v2.1 to v2.2. |
| 1.7 | January 2026 | Simplified documentation: Tag configuration (PHASE 3 Section 4) converted to single Property/Tags table. |
| 1.6 | January 2026 | Added RefreshMark() call in refresh path (Section 7.2) to sync widget timer with GE duration on re-hit. Removed PENDING DOCUMENTS section (WBP_MarkIndicator now complete). Updated Prerequisites table. |
| 1.5 | January 2026 | Simplified PHASE 1 tag creation - replaced detailed step-by-step instructions with simple tag list tables. |
| 1.4 | December 2025 | Added Father.State.Recruited to Activation Required Tags. Updated Quick Reference tag configuration. Added Related Documents table. |
| 1.0 | 2025 | Initial implementation |
| 1.1 | 2025 | Format fixes (UTF-8 to ASCII). Removed ASCII flow diagrams. Changed ability granting from Default Abilities Array to AbilityConfiguration system. Added form restriction via Activation Blocked Tags instead of Blueprint logic. Removed duplicate variables from ability. Added explicit InputTag configuration. Added PendingMarkTarget variable pattern. Reorganized phases for clarity. |
| 1.2 | 2025 | Changed Net Execution Policy from Local Predicted to Server Only per Technical Reference v1.7 guidance for passive NPC-owned abilities. |
| 1.3 | 2025 | Changed damage bonus from IncomingDamageMultiplier attribute to Armor -10 modifier (works with NarrativeAttributeSetBase, ~10% more damage taken). Replaced wall visibility (Custom Depth/Post Process) with Widget Component mark indicator (WBP_MarkIndicator) above enemy head, always visible through walls. Added OnDied delegate binding for proactive mark cleanup when enemy dies. Added MarkWidgetMap variable and widget management functions. Added validity check for oldest mark reference. Updated 5 forms in Activation Blocked Tags (Armor, Exoskeleton, Symbiote - allows Crawler and Engineer). |

---

## **QUICK REFERENCE**

### **Tag Configuration Summary**

| Property | Tags |
|----------|------|
| Ability Tags | `Ability.Father.Mark` |
| Activation Required | `Father.State.Recruited` |
| Activation Owned | `Father.State.Marking` |
| Activation Blocked | `Father.State.Transitioning`, `Narrative.State.IsDead`, `Effect.Father.FormState.Armor`, `Effect.Father.FormState.Exoskeleton`, `Effect.Father.FormState.Symbiote` |
| InputTag | None (passive) |

### **Replication Settings**

| Property | Value |
|----------|-------|
| Instancing Policy | Instanced Per Actor |
| Replication Policy | Replicate |
| Net Execution Policy | Server Only |

### **Gameplay Effect Summary**

| Effect | Duration | Purpose |
|--------|----------|---------|
| GE_MarkEffect | SetByCaller (default 5s) | Combined: Grants `Character.Marked` tag + Armor reduction |

| SetByCaller Tag | Default | Purpose |
|-----------------|---------|---------|
| Data.Mark.Duration | 5.0 | Effect duration in seconds |
| Data.Mark.ArmorReduction | -10.0 | Armor modifier (variable benefit based on target armor) |

### **Mark Parameters**

| Parameter | Value |
|-----------|-------|
| Duration | 5 seconds |
| Damage Bonus | ~10% (Armor -10) |
| Max Concurrent | 3 marks |
| Mark Indicator | WBP_MarkIndicator widget (always visible) |
| Refresh on Hit | Yes |
| Forms Active | Crawler, Engineer |

### **Combat Integration Summary**

| Ability | Mark Trigger |
|---------|--------------|
| GA_FatherAttack | On melee hit |
| GA_FatherLaserShot | On projectile hit |
| GA_TurretShoot | On turret hit |
| GA_FatherElectricTrap | On trap trigger |

### **BP_FatherCompanion Variables Required**

| Variable | Type | Replicated | Purpose |
|----------|------|------------|---------|
| MarkedEnemies | Array (Actor Ref) | Yes | Track marked enemies |
| MaxMarks | Integer | No | Limit (default 3) |
| PendingMarkTarget | Actor Ref | No | Target for pending mark |
| MarkWidgetMap | Map (Actor -> Widget Component) | No | Track widgets per enemy |

### **BP_FatherCompanion Functions Required**

| Function | Purpose |
|----------|---------|
| AddToMarkedEnemies | Add enemy to tracking array |
| RemoveFromMarkedEnemies | Remove enemy from array |
| MarkEnemy | Convenience function to trigger marking |
| StoreMarkWidget | Store widget reference in map |
| DestroyMarkWidgetForEnemy | Destroy widget and remove from map |
| HandleMarkedEnemyDeath | Clean up mark on enemy death |

### **Related Documents**

| Document | Version | Purpose |
|----------|---------|---------|
| Father_Companion_Technical_Reference | v4.5 | Cross-actor patterns, cleanup architecture |
| DefaultGameplayTags_FatherCompanion | v3.5 | Tag definitions |
| Father_Companion_System_Setup_Guide | v1.3 | BP_FatherCompanion setup |
| WBP_MarkIndicator_Implementation_Guide | v1.0 | Mark indicator widget |

---

**END OF GA_FATHERMARK IMPLEMENTATION GUIDE v2.0**

**Crawler/Engineer Form - Passive Enemy Marking**

**Unreal Engine 5.7 + Narrative Pro v2.2**

**Blueprint-Only Implementation**
