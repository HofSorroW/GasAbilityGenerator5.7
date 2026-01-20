# Father Companion - GA_FatherElectricTrap Implementation Guide
## VERSION 3.2 - Form State Tag Update (INV-1 Compliant)
## Unreal Engine 5.6 + Narrative Pro Plugin v2.2

---

## **DOCUMENT INFORMATION**

| Field | Value |
|-------|-------|
| Ability Name | GA_FatherElectricTrap |
| Ability Type | AI-Controlled Autonomous |
| Parent Class | NarrativeGameplayAbility |
| Trap Parent Class | NarrativeProjectile |
| Form | Engineer (Deployed Turret) |
| Input | None (AI Automatic) |
| Version | 3.1 |

---

## **TABLE OF CONTENTS**

1. [Introduction](#introduction)
2. [Prerequisites](#prerequisites)
3. [PHASE 1: Gameplay Tags Setup](#phase-1-gameplay-tags-setup)
4. [PHASE 2: Create Electric Web Trap Actor](#phase-2-create-electric-web-trap-actor)
5. [PHASE 3: Create Gameplay Effects](#phase-3-create-gameplay-effects)
6. [PHASE 4: Create GA_FatherElectricTrap Ability](#phase-4-create-ga_fatherelectrictrap-ability)
7. [PHASE 5: Implement Trap Deployment Logic](#phase-5-implement-trap-deployment-logic)
8. [PHASE 6: Implement OnTargetData Hit Handling](#phase-6-implement-ontargetdata-hit-handling)
9. [PHASE 7: Add Trap Tracking to BP_FatherCompanion](#phase-7-add-trap-tracking-to-bp_fathercompanion)
10. [PHASE 8: AI Behavior Tree Integration](#phase-8-ai-behavior-tree-integration)
11. [PHASE 9: Grant Ability via NPCDefinition](#phase-9-grant-ability-via-npcdefinition)
12. [Changelog](#changelog)

---

## **INTRODUCTION**

### **Ability Overview**

GA_FatherElectricTrap is an AI-controlled defensive ability used when the father is deployed as an Engineer turret. When enemies approach within close range, the turret automatically deploys homing electric web traps to root and damage them.

### **v2.7 Architecture Changes**

| Component | v2.6 | v2.7 |
|-----------|------|------|
| Trap Parent Class | Actor | NarrativeProjectile |
| Spawn Method | SpawnActorFromClass | AbilityTask_SpawnProjectile |
| Effect Application | In trap actor | In ability (OnTargetData) |
| Owner Reference | Manual OwnerFather variable | Automatic via GetNarrativeCharacter() |
| Hit Callback | OnComponentBeginOverlap only | SetProjectileTargetData -> OnTargetData |
| Trap Responsibility | Hit detection + effect application | Hit detection + target data broadcast |
| Ability Responsibility | Spawn + configure trap | Spawn + receive hit + apply effects |

### **Key Features**

| Feature | Description |
|---------|-------------|
| AI-Controlled | Activates automatically when enemies approach turret |
| Close Range Defense | Triggers when enemies within 500 units of turret |
| Homing Projectile | Trap tracks and homes toward target enemy |
| Crowd Control | Roots enemies for 5 seconds (complete immobilization) |
| Damage Over Time | 15 damage per second for 5 seconds (75 total) |
| Cooldown | 8 seconds between trap deployments |
| Max Active Traps | 2 traps can exist simultaneously |
| Trap Replacement | When at max, oldest trap destroyed before spawning new |
| Form Restriction | Only works in Engineer form when deployed |
| NarrativeProjectile | Uses v2.2 native projectile system |

### **Engineer Form Context**

In Engineer form, the father deploys as a stationary turret with two AI behaviors:

| Behavior | Range | Description |
|----------|-------|-------------|
| GA_TurretShoot | Greater than 500 units | Fires at distant enemies |
| GA_FatherElectricTrap | Less than 500 units | Deploys homing traps for close enemies |

### **Technical Specifications**

| Parameter | Value |
|-----------|-------|
| Activation Range | Less than 500 units from turret |
| Root Duration | 5 seconds (configurable via SetByCaller) |
| Damage Per Tick | 15 (configurable via SetByCaller) |
| Damage Ticks | 5 (once per second) |
| Total Damage | 75 |
| Cooldown | 8 seconds |
| Max Active Traps | 2 |
| Trap Spawn Distance | 200 units from turret toward enemy |
| Trap Lifetime | 8 seconds (regardless of hit) |
| Homing Acceleration | 500 units/second squared |
| Projectile Speed | 800 units/second |

---

## **PREREQUISITES**

### **Required Before This Guide**

| Requirement | Description |
|-------------|-------------|
| BP_FatherCompanion | Father character with NarrativeNPCCharacter parent |
| GA_FatherEngineer | Engineer form activation ability (creates BT_FatherEngineer) |
| NPCDefinition_Father | Father NPCDefinition asset with AbilityConfiguration |
| Effect.Father.FormState.Engineer tag | Form identification tag |
| Father.State.Deployed tag | Turret deployment state tag |
| BT_FatherEngineer | Engineer form behavior tree (created in GA_FatherEngineer guide) |
| BB_FatherEngineer | Engineer form blackboard (created in GA_FatherEngineer guide) |
| AM_FatherThrowWeb | Animation montage with FatherThrowWebRelease notify |
| Narrative Pro v2.2 | Required for NarrativeProjectile and AbilityTask_SpawnProjectile |

---

## **PHASE 1: GAMEPLAY TAGS SETUP**

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Engineer.ElectricTrap | Father turret electric trap - Engineer form AI-controlled |
| Father.State.ThrowingWeb | Father turret is deploying electric web trap |
| Effect.ElectricWeb.Active | Target is affected by electric web trap |
| Cooldown.Father.Engineer.ElectricTrap | Electric trap ability cooldown |
| Data.Damage.ElectricWeb | SetByCaller tag for electric web damage magnitude |
| Data.Duration.ElectricWebRoot | SetByCaller tag for electric web root duration |

### **Verify Existing Tags**

| Tag Name | Purpose |
|----------|---------|
| Effect.Father.FormState.Engineer | Form identification tag |
| Father.State.Deployed | Turret deployment state tag |
| Father.State.Recruited | Recruitment gate |
| Narrative.State.Movement.Lock | Built-in from Narrative Pro - locks movement |
| Narrative.State.IsDead | Built-in from Narrative Pro - blocked state |

---

## **PHASE 2: CREATE ELECTRIC WEB TRAP ACTOR**

### **1) Create Trap Blueprint**

#### 1.1) Navigate to Projectiles Folder
   - 1.1.1) In Content Browser, navigate to `/Content/FatherCompanion/Projectiles/`
   - 1.1.2) If folder does not exist, create it

#### 1.2) Create NarrativeProjectile Child Blueprint
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select **Blueprint Class**
   - 1.2.3) In search box, type: `NarrativeProjectile`
   - 1.2.4) Select **NarrativeProjectile** as parent class
   - 1.2.5) Name: `BP_ElectricWeb`
   - 1.2.6) Double-click to open

### **2) Add Components**

#### 2.1) Add Collision Component
   - 2.1.1) Click **Add** in Components panel
   - 2.1.2) Search: `Sphere Collision`
   - 2.1.3) Select **Sphere Collision**
   - 2.1.4) Name: `TrapCollision`
   - 2.1.5) Drag onto **DefaultSceneRoot** to make it child

#### 2.2) Configure Collision Component
   - 2.2.1) Select **TrapCollision** in Components panel
   - 2.2.2) In Details panel, find **Shape** section
   - 2.2.3) **Sphere Radius**: `50.0`
   - 2.2.4) Find **Collision** section
   - 2.2.5) **Collision Presets**: `Custom`
   - 2.2.6) **Collision Enabled**: `Query Only`
   - 2.2.7) **Object Type**: `WorldDynamic`
   - 2.2.8) Set all responses to **Ignore**
   - 2.2.9) Set **Pawn** response to **Overlap**
   - 2.2.10) Check **Generate Overlap Events**: True

#### 2.3) Add Static Mesh Component
   - 2.3.1) Click **Add** in Components panel
   - 2.3.2) Search: `Static Mesh`
   - 2.3.3) Select **Static Mesh**
   - 2.3.4) Name: `TrapMesh`
   - 2.3.5) Drag onto **DefaultSceneRoot** to make it child
   - 2.3.6) In Details, set **Static Mesh** to placeholder sphere or web mesh

#### 2.4) Add Projectile Movement Component
   - 2.4.1) Click **Add** in Components panel
   - 2.4.2) Search: `Projectile Movement`
   - 2.4.3) Select **Projectile Movement**
   - 2.4.4) Name: `ProjectileMovement`

#### 2.5) Configure Projectile Movement
   - 2.5.1) Select **ProjectileMovement** in Components panel
   - 2.5.2) In Details panel, find **Projectile** section
   - 2.5.3) **Initial Speed**: `800.0`
   - 2.5.4) **Max Speed**: `800.0`
   - 2.5.5) **Projectile Gravity Scale**: `0.0`
   - 2.5.6) Find **Homing** section
   - 2.5.7) **Is Homing Projectile**: Unchecked (enabled at runtime)
   - 2.5.8) **Homing Acceleration Magnitude**: `500.0`

### **3) Create Variables**

#### 3.1) Create Target Actor Variable
   - 3.1.1) In My Blueprint panel, click **+** next to Variables
   - 3.1.2) **Variable Name**: `TargetActor`
   - 3.1.3) **Variable Type**: `Actor Reference`
   - 3.1.4) **Instance Editable**: Checked
   - 3.1.5) **Default Value**: `None`

#### 3.2) Create Trap Lifetime Variable
   - 3.2.1) Click **+** next to Variables
   - 3.2.2) **Variable Name**: `TrapLifetime`
   - 3.2.3) **Variable Type**: `Float`
   - 3.2.4) **Instance Editable**: Checked
   - 3.2.5) **Default Value**: `8.0`

#### 3.3) Create Has Hit Target Variable
   - 3.3.1) Click **+** next to Variables
   - 3.3.2) **Variable Name**: `HasHitTarget`
   - 3.3.3) **Variable Type**: `Boolean`
   - 3.3.4) **Instance Editable**: Unchecked
   - 3.3.5) **Default Value**: `false`

### **4) Implement BeginPlay - Lifetime and Homing Setup**

#### 4.1) Open Event Graph
   - 4.1.1) Click **Event Graph** tab

#### 4.2) Find BeginPlay Event
   - 4.2.1) Locate **Event BeginPlay** node
   - 4.2.2) If not present, right-click and add it

#### 4.3) Set Lifespan
   - 4.3.1) From **Event BeginPlay** execution:
      - 4.3.1.1) Drag outward and search: `Set Life Span`
      - 4.3.1.2) Add **Set Life Span** node
   - 4.3.2) Connect execution from BeginPlay
   - 4.3.3) Drag **TrapLifetime** variable getter onto graph
   - 4.3.4) Connect **TrapLifetime** to **In Lifespan** pin

#### 4.4) Enable Homing
   - 4.4.1) From **Set Life Span** execution:
      - 4.4.1.1) Drag outward and search: `Is Valid`
      - 4.4.1.2) Add **Is Valid** node
   - 4.4.2) Drag **TargetActor** variable getter onto graph
   - 4.4.3) Connect **TargetActor** to Is Valid **Input Object** pin
   - 4.4.4) From Is Valid **Is Valid** execution:
      - 4.4.4.1) Drag **ProjectileMovement** component reference onto graph
      - 4.4.4.2) From ProjectileMovement, search: `Set Is Homing Projectile`
      - 4.4.4.3) Add **Set Is Homing Projectile** node
   - 4.4.5) Connect execution from Is Valid
   - 4.4.6) Check **Is Homing Projectile** checkbox (sets to true)

#### 4.5) Set Homing Target Component
   - 4.5.1) From **Set Is Homing Projectile** execution:
      - 4.5.1.1) From ProjectileMovement reference, search: `Set Homing Target Component`
      - 4.5.1.2) Add **Set Homing Target Component** node
   - 4.5.2) Connect execution
   - 4.5.3) From **TargetActor** variable:
      - 4.5.3.1) Drag outward and search: `Get Root Component`
      - 4.5.3.2) Add **Get Root Component** node
   - 4.5.4) Connect **Get Root Component** Return Value to **New Homing Target Component** pin

### **5) Implement EndPlay - Trap Cleanup**

#### 5.1) Add EndPlay Event
   - 5.1.1) Right-click in Event Graph
   - 5.1.2) Search: `Event EndPlay`
   - 5.1.3) Add **Event EndPlay** node

#### 5.2) Get Owner Character via NarrativeProjectile
   - 5.2.1) From **Event EndPlay** execution:
      - 5.2.1.1) Right-click and search: `Get Narrative Character`
      - 5.2.1.2) Add **Get Narrative Character** node (inherited from NarrativeProjectile)
   - 5.2.2) From Return Value:
      - 5.2.2.1) Drag and search: `Is Valid`
      - 5.2.2.2) Add **Is Valid** node

#### 5.3) Cast to BP_FatherCompanion
   - 5.3.1) From Is Valid **Is Valid** execution:
      - 5.3.1.1) Drag outward and search: `Cast to BP_FatherCompanion`
      - 5.3.1.2) Add **Cast to BP_FatherCompanion** node
   - 5.3.2) Connect **Get Narrative Character** Return Value to **Object** input

#### 5.4) Remove Self from Tracking Array
   - 5.4.1) From Cast **As BP Father Companion** pin:
      - 5.4.1.1) Drag outward and search: `Get ActiveElectricTraps`
      - 5.4.1.2) Add **Get ActiveElectricTraps** node (variable getter)
   - 5.4.2) From ActiveElectricTraps array:
      - 5.4.2.1) Drag outward and search: `Remove`
      - 5.4.2.2) Add **Remove** node (Array Remove Item)
   - 5.4.3) Connect execution from Cast success pin
   - 5.4.4) Get reference to **Self**:
      - 5.4.4.1) Right-click and search: `Get a reference to self`
      - 5.4.4.2) Add **Get a reference to self** node
   - 5.4.5) Connect **Self** reference to Remove **Item** pin

### **6) Implement Overlap - Faction Check and Target Data Broadcast**

#### 6.1) Add Overlap Event
   - 6.1.1) Select **TrapCollision** in Components panel
   - 6.1.2) In Details panel, scroll to **Events** section
   - 6.1.3) Click **+** next to **On Component Begin Overlap**

#### 6.2) Check Already Hit
   - 6.2.1) From **On Component Begin Overlap** execution:
      - 6.2.1.1) Drag **HasHitTarget** variable getter onto graph
      - 6.2.1.2) Drag outward and add **Branch** node
   - 6.2.2) Connect **HasHitTarget** to Branch **Condition**
   - 6.2.3) From Branch **True** (already hit):
      - 6.2.3.1) Leave disconnected (do nothing)

#### 6.3) Get Owner Character
   - 6.3.1) From Branch **False** execution:
      - 6.3.1.1) Right-click and search: `Get Narrative Character`
      - 6.3.1.2) Add **Get Narrative Character** node

#### 6.4) Check Not Self (Owner)
   - 6.4.1) From **Get Narrative Character** Return Value:
      - 6.4.1.1) Add **Not Equal (Object)** node
   - 6.4.2) Connect **Other Actor** from Overlap event to first input
   - 6.4.3) Connect **Get Narrative Character** Return Value to second input
   - 6.4.4) Add **Branch** node
   - 6.4.5) Connect Not Equal result to Branch **Condition**
   - 6.4.6) Connect execution

#### 6.5) Check Faction - Hostile Only
   - 6.5.1) From Branch **True** (not owner) execution:
      - 6.5.1.1) Right-click and search: `Get Attitude`
      - 6.5.1.2) Add **Get Attitude** node (from ArsenalStatics)
   - 6.5.2) Connect **Get Narrative Character** Return Value to **Test Actor** pin
   - 6.5.3) Connect **Other Actor** from Overlap event to **Target** pin
   - 6.5.4) From Get Attitude Return Value:
      - 6.5.4.1) Drag outward and add **Equal (Enum)** node
   - 6.5.5) Set second enum input to **Hostile**
   - 6.5.6) Add **Branch** node
   - 6.5.7) Connect Equal result to Branch **Condition**
   - 6.5.8) Connect execution

#### 6.6) Set Hit Flag
   - 6.6.1) From Branch **True** (is hostile) execution:
      - 6.6.1.1) Drag outward and search: `Set HasHitTarget`
      - 6.6.1.2) Add **Set HasHitTarget** node
   - 6.6.2) Check the checkbox (sets to true)

#### 6.7) Create Hit Result for Target Data
   - 6.7.1) From **Set HasHitTarget** execution:
      - 6.7.1.1) Right-click and search: `Make Hit Result`
      - 6.7.1.2) Add **Make Hit Result** node
   - 6.7.2) Configure Hit Result:
      - 6.7.2.1) **Hit Actor**: Connect **Other Actor** from Overlap event
      - 6.7.2.2) **Location**: From **Other Actor**, add **Get Actor Location** and connect

#### 6.8) Create Target Data Handle from Hit Result
   - 6.8.1) From **Make Hit Result** execution:
      - 6.8.1.1) Right-click and search: `Ability Target Data from Hit Result`
      - 6.8.1.2) Add **Ability Target Data from Hit Result** node
   - 6.8.2) Connect **Make Hit Result** Return Value to **Hit Result** input

#### 6.9) Broadcast Target Data to Ability Task
   - 6.9.1) From **Ability Target Data from Hit Result** execution:
      - 6.9.1.1) Right-click and search: `Set Projectile Target Data`
      - 6.9.1.2) Add **Set Projectile Target Data** node (inherited from NarrativeProjectile)
   - 6.9.2) Connect **Ability Target Data from Hit Result** Return Value to **Target Handle** input
   - 6.9.3) Connect execution

#### 6.10) Stop Projectile Movement
   - 6.10.1) From **Set Projectile Target Data** execution:
      - 6.10.1.1) Drag **ProjectileMovement** component onto graph
      - 6.10.1.2) From ProjectileMovement, search: `Stop Movement Immediately`
      - 6.10.1.3) Add **Stop Movement Immediately** node
   - 6.10.2) Connect execution

#### 6.11) Disable Homing
   - 6.11.1) From **Stop Movement Immediately** execution:
      - 6.11.1.1) From ProjectileMovement, search: `Set Is Homing Projectile`
      - 6.11.1.2) Add **Set Is Homing Projectile** node
   - 6.11.2) Leave checkbox unchecked (sets to false)

### **7) Compile and Save**

#### 7.1) Compile Blueprint
   - 7.1.1) Click **Compile** button in toolbar

#### 7.2) Save Blueprint
   - 7.2.1) Click **Save** button in toolbar

---

## **PHASE 3: CREATE GAMEPLAY EFFECTS**

### **1) Create GE_ElectricWebRoot**

#### 1.1) Create Effect Blueprint
   - 1.1.1) In Content Browser, navigate to `/Content/FatherCompanion/GAS/Effects/`
   - 1.1.2) Right-click -> **Blueprint Class** -> **GameplayEffect**
   - 1.1.3) Name: `GE_ElectricWebRoot`
   - 1.1.4) Double-click to open

#### 1.2) Configure GE_ElectricWebRoot Properties

| Property | Value |
|----------|-------|
| Duration Policy | Has Duration |
| Duration Magnitude Type | Set By Caller |
| Set By Caller Tag | Data.Duration.ElectricWebRoot |

#### 1.3) Configure GE_ElectricWebRoot Components

| Component | Configuration |
|-----------|---------------|
| Grant Tags to Target Actor | Narrative.State.Movement.Lock, Effect.ElectricWeb.Active |
| Tags This Effect Has (Asset Tags) | Effect.ElectricWeb.Root |

#### 1.4) Compile and Save

### **2) Create GE_ElectricWebDamage**

#### 2.1) Create Effect Blueprint
   - 2.1.1) Right-click in Content Browser -> **Blueprint Class** -> **GameplayEffect**
   - 2.1.2) Name: `GE_ElectricWebDamage`
   - 2.1.3) Double-click to open

#### 2.2) Configure GE_ElectricWebDamage Properties

| Property | Value |
|----------|-------|
| Duration Policy | Has Duration |
| Duration Magnitude (Scalable Float) | 5.0 |
| Period (Scalable Float) | 1.0 |
| Execute Periodic Effect on Application | Checked |

#### 2.3) Configure GE_ElectricWebDamage Components

| Component | Configuration |
|-----------|---------------|
| Custom Execution Calculation | Calculation Class: NarrativeDamageExecCalc |
| Calculation Modifiers [0] | Modifier Op: Override, Magnitude Type: Set By Caller, Tag: Data.Damage.ElectricWeb |
| Tags This Effect Has (Asset Tags) | Effect.ElectricWeb.Damage |

#### 2.4) Compile and Save

### **3) Create GE_ElectricTrapCooldown**

#### 3.1) Create Effect Blueprint
   - 3.1.1) Right-click in Content Browser -> **Blueprint Class** -> **GameplayEffect**
   - 3.1.2) Name: `GE_ElectricTrapCooldown`
   - 3.1.3) Double-click to open

#### 3.2) Configure GE_ElectricTrapCooldown Properties

| Property | Value |
|----------|-------|
| Duration Policy | Has Duration |
| Duration Magnitude (Scalable Float) | 8.0 |

#### 3.3) Configure GE_ElectricTrapCooldown Components

| Component | Configuration |
|-----------|---------------|
| Grant Tags to Target Actor | Cooldown.Father.Engineer.ElectricTrap |

#### 3.4) Compile and Save

---

## **PHASE 4: CREATE GA_FATHERELECTRICTRAP ABILITY**

### **1) Create Ability Blueprint**

#### 1.1) Navigate to Abilities Folder
   - 1.1.1) In Content Browser, navigate to `/Content/FatherCompanion/GAS/Abilities/`

#### 1.2) Create Ability Blueprint
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select **Blueprint Class**
   - 1.2.3) Search parent: `NarrativeGameplayAbility`
   - 1.2.4) Select **NarrativeGameplayAbility**
   - 1.2.5) Name: `GA_FatherElectricTrap`
   - 1.2.6) Double-click to open

### **2) Configure Class Defaults**

#### 2.1) Configure Ability Tags

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Engineer.ElectricTrap |
| Activation Required Tags | Effect.Father.FormState.Engineer, Father.State.Deployed, Father.State.Recruited |
| Activation Blocked Tags | Cooldown.Father.Engineer.ElectricTrap, Narrative.State.IsDead |
| Activation Owned Tags | Father.State.ThrowingWeb |

### **3) Configure Replication**

#### 3.1) Set Instancing Policy
   - 3.1.1) Find **Instancing** section
   - 3.1.2) **Instancing Policy**: `Instanced Per Actor`

#### 3.2) Set Net Execution Policy
   - 3.2.1) Find **Net Execution Policy**
   - 3.2.2) Set to: `Server Only`

#### 3.3) Set Replication Policy
   - 3.3.1) Find **Replication Policy**
   - 3.3.2) Set to: `Replicate`

#### 3.4) Configure Additional Net Settings
   - 3.4.1) **Replicate Input Directly**: Unchecked
   - 3.4.2) **Server Respects Remote Ability Cancellation**: Unchecked

### **4) Create Variables

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| ProjectileClass | Class Reference (NarrativeProjectile) | BP_ElectricWeb | Yes |
| DamagePerTick | Float | 15.0 | Yes |
| RootDuration | Float | 5.0 | Yes |
| TrapSpawnDistance | Float | 200.0 | Yes |
| TargetEnemy | Actor Reference | None | Yes |
| SpawnedProjectileTask | AbilityTask_SpawnProjectile (Object Reference) | None | No |

### **5) Compile and Save**

#### 5.1) Compile Blueprint
   - 5.1.1) Click **Compile** button

#### 5.2) Save Blueprint
   - 5.2.1) Click **Save** button

---

## **PHASE 5: IMPLEMENT TRAP DEPLOYMENT LOGIC**

### **1) Open Event Graph**

#### 1.1) Access Event Graph
   - 1.1.1) In GA_FatherElectricTrap, click **Event Graph** tab

### **2) Get Avatar Actor Reference**

#### 2.1) Find Activate Ability Event
   - 2.1.1) Locate **Event ActivateAbility** node
   - 2.1.2) If not present, right-click and add it

#### 2.2) Get Avatar Actor
   - 2.2.1) From **Event ActivateAbility** execution:
      - 2.2.1.1) Drag outward and search: `Get Avatar Actor from Actor Info`
      - 2.2.1.2) Add **Get Avatar Actor from Actor Info** node
   - 2.2.2) Connect **Actor Info** from Event to this node

#### 2.3) Cast to BP_FatherCompanion
   - 2.3.1) From **Get Avatar Actor** Return Value:
      - 2.3.1.1) Drag outward and search: `Cast to BP_FatherCompanion`
      - 2.3.1.2) Add **Cast to BP_FatherCompanion** node
   - 2.3.2) Connect execution from Get Avatar Actor

### **3) Check Authority**

#### 3.1) Add Authority Check
   - 3.1.1) From Cast success execution:
      - 3.1.1.1) Drag from **As BP Father Companion** pin
      - 3.1.1.2) Search: `Has Authority`
      - 3.1.1.3) Add **Has Authority** node
   - 3.1.2) Add **Branch** node
   - 3.1.3) Connect **Has Authority** Return Value to Branch **Condition**
   - 3.1.4) Connect execution from Cast to Branch

#### 3.2) Handle Non-Authority
   - 3.2.1) From Branch **False** (client):
      - 3.2.1.1) Leave disconnected (server handles spawning)

### **4) Check Max Traps and Handle Replacement**

#### 4.1) Get Trap Tracking Array
   - 4.1.1) From Branch **True** (has authority) execution:
      - 4.1.1.1) From **As BP Father Companion** pin
      - 4.1.1.2) Drag and search: `Get ActiveElectricTraps`
      - 4.1.1.3) Add variable getter node

#### 4.2) Get Array Length
   - 4.2.1) From **ActiveElectricTraps** array:
      - 4.2.1.1) Drag outward and search: `Length`
      - 4.2.1.2) Add **Length** node

#### 4.3) Get Max Traps
   - 4.3.1) From **As BP Father Companion** pin:
      - 4.3.1.1) Drag and search: `Get MaxActiveTraps`
      - 4.3.1.2) Add variable getter node

#### 4.4) Compare Count to Max
   - 4.4.1) Add **Greater Than or Equal (Integer)** node
   - 4.4.2) Connect **Length** Return Value to first input
   - 4.4.3) Connect **MaxActiveTraps** to second input
   - 4.4.4) Add **Branch** node
   - 4.4.5) Connect comparison result to Branch **Condition**
   - 4.4.6) Connect execution from authority Branch True

#### 4.5) Destroy Oldest Trap (At Max)
   - 4.5.1) From Branch **True** (at max capacity) execution:
      - 4.5.1.1) From **ActiveElectricTraps** array
      - 4.5.1.2) Drag and search: `Get`
      - 4.5.1.3) Add **Get (a copy)** node
   - 4.5.2) **Index**: `0` (oldest trap)
   - 4.5.3) From Get Return Value:
      - 4.5.3.1) Drag outward and search: `Destroy Actor`
      - 4.5.3.2) Add **Destroy Actor** node
   - 4.5.4) Connect execution from Branch True

### **5) Play Throw Animation with Notify**

#### 5.1) Add Play Montage and Wait Task
   - 5.1.1) From **Destroy Actor** execution:
      - 5.1.1.1) Right-click and search: `Play Montage and Wait for Event`
      - 5.1.1.2) Add **Play Montage and Wait for Event** ability task node
   - 5.1.2) From Branch **False** (under max capacity) execution:
      - 5.1.2.1) Also connect to the same **Play Montage and Wait for Event** node

#### 5.2) Configure Montage Task
   - 5.2.1) **Montage to Play**: Select `AM_FatherThrowWeb`
   - 5.2.2) **Event Tags**: Click **+** and add: `Ability.Father.Engineer.ElectricTrap`
   - 5.2.3) Leave other settings default

### **6) Handle Animation Notify - Spawn Trap**

#### 6.1) Connect to On Event Received Pin
   - 6.1.1) On **Play Montage and Wait for Event** node, locate the **On Event Received** pin
   - 6.1.2) This pin fires when the AnimNotify (FatherThrowWebRelease) triggers during the montage

#### 6.2) Validate Target Enemy
   - 6.2.1) From **On Event Received** execution:
      - 6.2.1.1) Drag **TargetEnemy** variable getter onto graph
      - 6.2.1.2) Add **Is Valid** node
   - 6.2.2) Connect **TargetEnemy** to Is Valid input
   - 6.2.3) Add **Branch** node
   - 6.2.4) Connect Is Valid **Return Value** to Branch **Condition**
   - 6.2.5) Connect execution from **On Event Received** to Branch

#### 6.3) Handle Invalid Target
   - 6.3.1) From Branch **False** (no valid target):
      - 6.3.1.1) Add **Cancel Ability** node
      - 6.3.1.2) Connect execution from Branch False

### **7) Calculate Spawn Transform**

#### 7.1) Get Father Location (Data Node)
   - 7.1.1) From **As BP Father Companion** reference (saved from earlier cast):
      - 7.1.1.1) Drag and search: `Get Actor Location`
      - 7.1.1.2) Add **Get Actor Location** node

#### 7.2) Get Enemy Location (Data Node)
   - 7.2.1) From **TargetEnemy** variable:
      - 7.2.1.1) Drag and search: `Get Actor Location`
      - 7.2.1.2) Add **Get Actor Location** node

#### 7.3) Calculate Direction (Data Node)
   - 7.3.1) Right-click and search: `Get Unit Direction (Vector)`
   - 7.3.2) Add **Get Unit Direction (Vector)** node
   - 7.3.3) Connect Father Location to **From** input
   - 7.3.4) Connect Enemy Location to **To** input

#### 7.4) Calculate Spawn Position (Data Nodes)
   - 7.4.1) Drag **TrapSpawnDistance** variable getter onto graph
   - 7.4.2) Add **Multiply (Vector * Float)** node
   - 7.4.3) Connect **Get Unit Direction** Return Value to Vector input
   - 7.4.4) Connect **TrapSpawnDistance** to Float input
   - 7.4.5) Add **Add (Vector + Vector)** node
   - 7.4.6) Connect Father Location to first input
   - 7.4.7) Connect Multiply result to second input

#### 7.5) Create Spawn Transform
   - 7.5.1) Right-click and search: `Make Transform`
   - 7.5.2) Add **Make Transform** node
   - 7.5.3) Configure Make Transform:
      - 7.5.3.1) **Location**: Connect the Add (Vector + Vector) result
      - 7.5.3.2) **Rotation**: Leave default (0,0,0)
      - 7.5.3.3) **Scale**: Leave default (1,1,1)

### **8) Spawn Trap Using AbilityTask_SpawnProjectile**

#### 8.1) Add Spawn Projectile Task
   - 8.1.1) From Branch **True** (valid target) execution:
      - 8.1.1.1) Right-click and search: `Spawn Projectile`
      - 8.1.1.2) Add **Spawn Projectile** ability task node (AbilityTask_SpawnProjectile)
   - 8.1.2) Connect execution from Branch True

#### 8.2) Configure Spawn Projectile Task
   - 8.2.1) **Task Instance Name**: `SpawnElectricWeb`
   - 8.2.2) **Class**: Connect **ProjectileClass** variable
   - 8.2.3) **Projectile Spawn Transform**: Connect **Make Transform** Return Value

#### 8.3) Store Task Reference
   - 8.3.1) From **Spawn Projectile** Return Value:
      - 8.3.1.1) Drag and search: `Set SpawnedProjectileTask`
      - 8.3.1.2) Add **Set SpawnedProjectileTask** node
   - 8.3.2) Connect execution

#### 8.4) Activate Task
   - 8.4.1) From **Set SpawnedProjectileTask** execution:
      - 8.4.1.1) From **SpawnedProjectileTask** variable
      - 8.4.1.2) Drag and search: `Ready for Activation`
      - 8.4.1.3) Add **Ready for Activation** node
   - 8.4.2) Connect execution

### **9) Configure Spawned Trap**

#### 9.1) Get Spawned Projectile from Task
   - 9.1.1) From **SpawnedProjectileTask** variable:
      - 9.1.1.1) Drag and search: `Get Projectile`
      - 9.1.1.2) Add getter node (or access Projectile property)

#### 9.2) Cast to BP_ElectricWeb
   - 9.2.1) From **Ready for Activation** execution:
      - 9.2.1.1) Add **Cast to BP_ElectricWeb** node
   - 9.2.2) Connect Projectile reference to **Object** input

#### 9.3) Set Target Actor (for Homing)
   - 9.3.1) From Cast success execution:
      - 9.3.1.1) From **As BP Electric Web** pin
      - 9.3.1.2) Add **Set TargetActor** node
   - 9.3.2) Connect **TargetEnemy** variable to TargetActor value

### **10) Add Trap to Tracking Array**

#### 10.1) Get Tracking Array
   - 10.1.1) From **Set TargetActor** execution:
      - 10.1.1.1) From **As BP Father Companion** reference
      - 10.1.1.2) Drag and search: `Get ActiveElectricTraps`
      - 10.1.1.3) Add variable getter

#### 10.2) Add Spawned Trap to Array
   - 10.2.1) From **ActiveElectricTraps** array:
      - 10.2.1.1) Drag outward and search: `Add`
      - 10.2.1.2) Add **Add** node (Array Add)
   - 10.2.2) Connect execution from Set TargetActor
   - 10.2.3) From Projectile reference (the spawned trap):
      - 10.2.3.1) Connect to Add **Item** pin

---

## **PHASE 6: IMPLEMENT ONTARGETDATA HIT HANDLING**

### **1) Bind to OnTargetData Delegate**

#### 1.1) Connect to OnTargetData Pin
   - 1.1.1) On **Spawn Projectile** ability task node, locate the **On Target Data** delegate pin
   - 1.1.2) This fires when trap calls SetProjectileTargetData() on hit

### **2) Extract Hit Information**

#### 2.1) Get Hit Result from Target Data
   - 2.1.1) From **On Target Data** execution:
      - 2.1.1.1) Right-click and search: `Get Hit Result from Target Data`
      - 2.1.1.2) Add **Get Hit Result from Target Data** node
   - 2.1.2) Connect **Data** parameter from On Target Data to **Target Data** input
   - 2.1.3) **Index**: `0`

#### 2.2) Break Hit Result
   - 2.2.1) From **Get Hit Result from Target Data** Return Value:
      - 2.2.1.1) Drag and search: `Break Hit Result`
      - 2.2.1.2) Add **Break Hit Result** node

#### 2.3) Get Hit Actor
   - 2.3.1) From **Break Hit Result** node, find **Hit Actor** pin
   - 2.3.2) This is the enemy that was hit by the trap

### **3) Validate Hit Actor**

#### 3.1) Check Hit Actor Valid
   - 3.1.1) From **On Target Data** execution:
      - 3.1.1.1) Add **Is Valid** node
   - 3.1.2) Connect **Hit Actor** to Is Valid input
   - 3.1.3) Add **Branch** node
   - 3.1.4) Connect Is Valid result to Branch Condition
   - 3.1.5) Connect execution

### **4) Get ASC References**

#### 4.1) Get Target (Enemy) ASC
   - 4.1.1) From Branch **True** (valid hit) execution:
      - 4.1.1.1) From **Hit Actor**
      - 4.1.1.2) Drag and search: `Get Ability System Component`
      - 4.1.1.3) Add **Get Ability System Component** node

#### 4.2) Validate Target Has ASC
   - 4.2.1) Add **Is Valid** node
   - 4.2.2) Connect Target ASC to Is Valid input
   - 4.2.3) Add **Branch** node
   - 4.2.4) Connect execution

#### 4.3) Get Owner (Father) ASC
   - 4.3.1) From Branch **True** (target has ASC) execution:
      - 4.3.1.1) Right-click and search: `Get Ability System Component from Actor Info`
      - 4.3.1.2) Add **Get Ability System Component from Actor Info** node
   - 4.3.2) Connect **Actor Info** from Event ActivateAbility

### **5) Apply Root Effect to Target**

#### 5.1) Create Root Effect Spec
   - 5.1.1) From Get Owner ASC:
      - 5.1.1.1) Drag and search: `Make Outgoing Spec`
      - 5.1.1.2) Add **Make Outgoing Spec** node
   - 5.1.2) Connect Owner Father ASC to **Target** pin
   - 5.1.3) **Gameplay Effect Class**: `GE_ElectricWebRoot`
   - 5.1.4) **Level**: `1.0`

#### 5.2) Assign Root Duration via SetByCaller
   - 5.2.1) From **Make Outgoing Spec** execution:
      - 5.2.1.1) Drag outward and search: `Assign Tag Set By Caller Magnitude`
      - 5.2.1.2) Add **Assign Tag Set By Caller Magnitude** node
   - 5.2.2) Connect **Spec Handle** from Make Outgoing Spec
   - 5.2.3) **Data Tag**: Click dropdown, select `Data.Duration.ElectricWebRoot`
   - 5.2.4) Drag **RootDuration** variable getter onto graph
   - 5.2.5) Connect **RootDuration** to **Magnitude** pin

#### 5.3) Apply Root Effect to Target
   - 5.3.1) From **Assign Tag Set By Caller Magnitude** execution:
      - 5.3.1.1) From Target ASC (enemy)
      - 5.3.1.2) Drag and search: `Apply Gameplay Effect Spec to Self`
      - 5.3.1.3) Add **Apply Gameplay Effect Spec to Self** node
   - 5.3.2) Connect execution
   - 5.3.3) Connect Root **Spec Handle**

### **6) Apply Damage Effect to Target**

#### 6.1) Create Damage Effect Spec
   - 6.1.1) From Apply Root Effect execution:
      - 6.1.1.1) From Owner Father ASC
      - 6.1.1.2) Add **Make Outgoing Spec** node
   - 6.1.2) **Gameplay Effect Class**: `GE_ElectricWebDamage`
   - 6.1.3) **Level**: `1.0`

#### 6.2) Assign Damage Magnitude via SetByCaller
   - 6.2.1) From **Make Outgoing Spec** (Damage) execution:
      - 6.2.1.1) Add **Assign Tag Set By Caller Magnitude** node
   - 6.2.2) Connect Damage **Spec Handle**
   - 6.2.3) **Data Tag**: `Data.Damage.ElectricWeb`
   - 6.2.4) Drag **DamagePerTick** variable getter
   - 6.2.5) Connect **DamagePerTick** to **Magnitude** pin

#### 6.3) Apply Damage Effect to Target
   - 6.3.1) From **Assign Tag Set By Caller Magnitude** (Damage) execution:
      - 6.3.1.1) From Target ASC
      - 6.3.1.2) Add **Apply Gameplay Effect Spec to Self** node
   - 6.3.2) Connect execution
   - 6.3.3) Connect Damage **Spec Handle**

### **7) Handle OnDestroyed Delegate**

#### 7.1) Connect to OnDestroyed Pin
   - 7.1.1) On **Spawn Projectile** ability task node, locate the **On Destroyed** delegate pin
   - 7.1.2) This fires when trap is destroyed without hitting a target

#### 7.2) No Action Required
   - 7.2.1) From **On Destroyed** execution:
      - 7.2.1.1) Leave disconnected (trap expired, no effects to apply)

### **8) Handle Animation Complete - Apply Cooldown**

#### 8.1) Connect to On Completed Pin
   - 8.1.1) On **Play Montage and Wait for Event** node, locate **On Completed** pin
   - 8.1.2) From **On Completed** execution:
      - 8.1.2.1) Drag outward and search: `Apply Gameplay Effect to Owner`
      - 8.1.2.2) Add **Apply Gameplay Effect to Owner** node

#### 8.2) Configure Cooldown Application
   - 8.2.1) **Gameplay Effect Class**: `GE_ElectricTrapCooldown`
   - 8.2.2) **Level**: `1.0`

#### 8.3) End Ability
   - 8.3.1) From **Apply Gameplay Effect to Owner** execution:
      - 8.3.1.1) Drag outward and search: `End Ability`
      - 8.3.1.2) Add **End Ability** node
   - 8.3.2) Leave **Was Cancelled** unchecked

### **9) Handle Animation Interrupted**

#### 9.1) Connect to On Interrupted Pin
   - 9.1.1) On **Play Montage and Wait for Event** node, locate **On Interrupted** pin
   - 9.1.2) From **On Interrupted** execution:
      - 9.1.2.1) Add **End Ability** node
   - 9.1.3) Check **Was Cancelled** checkbox

### **10) Compile and Save**

#### 10.1) Compile Ability
   - 10.1.1) Click **Compile** button

#### 10.2) Save Ability
   - 10.2.1) Click **Save** button

---

## **PHASE 7: ADD TRAP TRACKING TO BP_FATHERCOMPANION**

### **1) Open BP_FatherCompanion**

#### 1.1) Navigate to Blueprint
   - 1.1.1) In Content Browser, navigate to `/Content/FatherCompanion/Characters/`
   - 1.1.2) Double-click **BP_FatherCompanion** to open

### **2) Create Tracking Variables**

#### 2.1) Create Active Traps Array
   - 2.1.1) In My Blueprint panel, click **+** next to Variables
   - 2.1.2) **Variable Name**: `ActiveElectricTraps`
   - 2.1.3) **Variable Type**: `Array` of `Actor Reference`
   - 2.1.4) **Instance Editable**: Unchecked
   - 2.1.5) **Blueprint Read Only**: Unchecked
   - 2.1.6) **Default Value**: Empty array

#### 2.2) Create Max Traps Variable
   - 2.2.1) Click **+** next to Variables
   - 2.2.2) **Variable Name**: `MaxActiveTraps`
   - 2.2.3) **Variable Type**: `Integer`
   - 2.2.4) **Instance Editable**: Checked
   - 2.2.5) **Default Value**: `2`

### **3) Compile and Save**

#### 3.1) Compile Blueprint
   - 3.1.1) Click **Compile** button

#### 3.2) Save Blueprint
   - 3.2.1) Click **Save** button

---

## **PHASE 8: AI BEHAVIOR TREE INTEGRATION**

### **1) Open BT_FatherEngineer**

#### 1.1) Navigate to Behavior Tree
   - 1.1.1) In Content Browser, navigate to `/Content/FatherCompanion/AI/`
   - 1.1.2) Double-click **BT_FatherEngineer** to open

### **2) Add Electric Trap Decision Branch**

#### 2.1) Create Distance Check
   - 2.1.1) The behavior tree should check enemy distance
   - 2.1.2) If distance less than 500 units: Use GA_FatherElectricTrap
   - 2.1.3) If distance greater than 500 units: Use GA_TurretShoot

#### 2.2) Configure BTTask for Electric Trap
   - 2.2.1) Add **BTTask_ActivateAbilityByClass** task node (Narrative Pro built-in)
   - 2.2.2) Select the task node in Behavior Tree
   - 2.2.3) In Details panel, set **Ability to Activate**: `GA_FatherElectricTrap`

#### 2.3) BTTask_ActivateAbilityByClass Reference

| Property | Value |
|----------|-------|
| Location | Plugins/Narrative Pro/Pro/Core/AI/Tasks/ |
| Type | Built-in reusable task |
| Configuration | Select ability class from dropdown |

### **3) Save Behavior Tree**

#### 3.1) Save Asset
   - 3.1.1) Click **Save** button

---

## **PHASE 9: GRANT ABILITY VIA NPCDEFINITION**

### **1) Open NPCDefinition_Father**

#### 1.1) Navigate to NPCDefinition
   - 1.1.1) In Content Browser, navigate to `/Content/FatherCompanion/Data/`
   - 1.1.2) Double-click **NPCDefinition_Father** to open

### **2) Add Electric Trap to AbilityConfiguration**

#### 2.1) Find AbilityConfiguration Section
   - 2.1.1) In Details panel, find **Ability Configuration** section
   - 2.1.2) Locate **Granted Abilities** array

#### 2.2) Add Electric Trap Ability
   - 2.2.1) Click **+** to add new entry
   - 2.2.2) **Ability Class**: `GA_FatherElectricTrap`
   - 2.2.3) **Level**: `1`

### **3) Save NPCDefinition**

#### 3.1) Save Asset
   - 3.1.1) Click **Save** button

---

## **CHANGELOG**

### **VERSION 3.1 - BTTask_ActivateAbilityByClass Integration**

**Release Date:** January 2026

**Changes from v3.0:**

1. **BTTask Update**
   - PHASE 8 Section 2.2 - Updated to use Narrative Pro built-in BTTask_ActivateAbilityByClass
   - Replaced BTTask_ActivateAbility with BTTask_ActivateAbilityByClass
   - Added Section 2.3 - BTTask_ActivateAbilityByClass Reference table

---

### **VERSION 3.0 - Documentation Simplification**

**Release Date:** January 2026

**Changes from v2.9:**

1. **Tag Configuration Simplified**
   - PHASE 4 Section 2 - converted to single Property/Tags table
   
2. **Variable Creation Simplified**
   - PHASE 4 Section 4 - converted to Variable/Type/Default table

---

### **VERSION 2.9 - Simplified Configuration Sections**

**Release Date:** January 2026

**Changes from v2.8:**

1. **GameplayEffect Configuration Simplified**
   - PHASE 3 GE configs (GE_ElectricWebRoot, GE_ElectricWebDamage, GE_ElectricTrapCooldown) reduced from 121 to 60 lines
   - Duration, Component, and Modifier settings now use table format

---

### **VERSION 2.8 - Simplified Tag Creation**

**Release Date:** January 2026

**Changes from v2.7:**

1. **Tag Creation Simplified**
   - Replaced detailed step-by-step tag creation instructions with simple tag list tables
   - Separated "Create Required Tags" and "Verify Existing Tags" sections

---

### **VERSION 2.7 - NarrativeProjectile Integration**

**Release Date:** December 2025

**Changes from v2.6:**

1. **Parent Class Migration**
   - Changed BP_ElectricWeb parent from Actor to NarrativeProjectile
   - Enables automatic owner character tracking via GetNarrativeCharacter()
   - Integrates with AbilityTask_SpawnProjectile system

2. **Spawn Method Change**
   - Replaced SpawnActorFromClass with AbilityTask_SpawnProjectile
   - Provides OnTargetData and OnDestroyed delegate callbacks
   - Better integration with GAS ability lifecycle

3. **Effect Application Moved to Ability**
   - Removed effect application logic from BP_ElectricWeb
   - Trap now only detects hit and broadcasts target data
   - GA_FatherElectricTrap handles all effect application in OnTargetData

4. **Simplified Trap Variables**
   - Removed from trap: OwnerFather, DamageAmount, RootDuration
   - Trap retains: TargetActor (homing), TrapLifetime, HasHitTarget
   - Damage/duration parameters stay in ability

5. **New PHASE 6**
   - Added dedicated phase for OnTargetData hit handling
   - Centralized effect application in ability
   - Proper ASC reference management

6. **Owner Reference Pattern**
   - Replaced manual OwnerFather variable with GetNarrativeCharacter()
   - NarrativeProjectile automatically tracks owner via GetOwner()
   - Cleaner architecture aligned with Narrative Pro patterns

7. **Plugin Version Update**
   - Updated from Narrative Pro v2.1 to v2.2
   - Leverages new NarrativeProjectile and AbilityTask_SpawnProjectile

---

### **VERSION 2.6 - Execution Flow Corrections**

**Release Date:** December 2025

**Changes from v2.5:**

1. **Spawn Actor Connection Fix**
   - Corrected execution flow from Branch True directly to Spawn Actor from Class
   - Removed incorrect intermediate node reference

2. **Pin Name Corrections**
   - Added explicit pin name references throughout Phase 5

3. **Execution Flow Clarification**
   - Clarified that calculation nodes are pure/data nodes without execution pins
   - Separated data node setup from execution flow

4. **Explicit Execution Connections**
   - Added explicit connection instructions for Branch to execution nodes

---

## **QUICK REFERENCE**

### **Architecture Comparison**

| Component | v2.6 | v2.7 |
|-----------|------|------|
| Trap Parent | Actor | NarrativeProjectile |
| Spawn Method | SpawnActorFromClass | AbilityTask_SpawnProjectile |
| Effect Location | In trap | In ability |
| Owner Access | OwnerFather variable | GetNarrativeCharacter() |
| Hit Callback | None | OnTargetData delegate |

### **Tag Configuration Summary**

| Property | Tags |
|----------|------|
| Ability Tags | `Ability.Father.Engineer.ElectricTrap` |
| Activation Required | `Effect.Father.FormState.Engineer`, `Father.State.Deployed`, `Father.State.Recruited` |
| Activation Owned | `Father.State.ThrowingWeb` |
| Activation Blocked | `Cooldown.Father.Engineer.ElectricTrap`, `Narrative.State.IsDead` |
| InputTag | None (AI-controlled) |

### **SetByCaller Tags Summary**

| Tag | Purpose | Default Value |
|-----|---------|---------------|
| `Data.Damage.ElectricWeb` | Damage per tick magnitude | 15.0 |
| `Data.Duration.ElectricWebRoot` | Root effect duration | 5.0 |

### **Ability Variable Summary**

| Variable | Type | Instance Editable | Default | Purpose |
|----------|------|-------------------|---------|---------|
| ProjectileClass | Class Ref (NarrativeProjectile) | Yes | BP_ElectricWeb | Trap actor to spawn |
| DamagePerTick | Float | Yes | 15.0 | Damage per second (SetByCaller) |
| RootDuration | Float | Yes | 5.0 | Root duration in seconds (SetByCaller) |
| TrapSpawnDistance | Float | Yes | 200.0 | Distance from turret to spawn |
| TargetEnemy | Actor Ref | Yes | None | AI-provided target |
| SpawnedProjectileTask | AbilityTask_SpawnProjectile | No | None | Task reference for cleanup |

### **Trap Actor Variable Summary**

| Variable | Type | Instance Editable | Default | Purpose |
|----------|------|-------------------|---------|---------|
| TargetActor | Actor Ref | Yes | None | Homing target reference |
| TrapLifetime | Float | Yes | 8.0 | How long trap exists |
| HasHitTarget | Boolean | No | false | Prevents multiple hits |

### **BP_FatherCompanion Variable Summary**

| Variable | Type | Instance Editable | Default | Purpose |
|----------|------|-------------------|---------|---------|
| ActiveElectricTraps | Array of Actor Ref | No | Empty | Tracks spawned traps |
| MaxActiveTraps | Integer | Yes | 2 | Max simultaneous traps |

### **Gameplay Effect Summary**

| Effect | Duration | Purpose |
|--------|----------|---------|
| GE_ElectricWebRoot | SetByCaller (default 5s) | Immobilizes target via Narrative.State.Movement.Lock |
| GE_ElectricWebDamage | 5 seconds | 15 damage/second via NarrativeDamageExecCalc |
| GE_ElectricTrapCooldown | 8 seconds | Ability cooldown |

### **Homing Configuration**

| Parameter | Value |
|-----------|-------|
| Initial Speed | 800 units/second |
| Max Speed | 800 units/second |
| Homing Acceleration | 500 units/second squared |
| Gravity Scale | 0 (no gravity) |
| Homing Target | Enemy Root Component |

### **Replication Summary**

| Property | Value |
|----------|-------|
| Instancing Policy | Instanced Per Actor |
| Replication Policy | Replicate |
| Net Execution Policy | Server Only |
| Replicate Input Directly | FALSE |
| Server Respects Remote Ability Cancellation | FALSE |

### **Data Flow (v2.7)**

| Step | Component | Action |
|------|-----------|--------|
| 1 | GA_FatherElectricTrap | AbilityTask_SpawnProjectile spawns trap |
| 2 | BP_ElectricWeb | Trap homes toward enemy |
| 3 | BP_ElectricWeb | OnOverlap detects hit, validates faction |
| 4 | BP_ElectricWeb | SetProjectileTargetData() broadcasts hit |
| 5 | GA_FatherElectricTrap | OnTargetData receives hit data |
| 6 | GA_FatherElectricTrap | Apply GE_ElectricWebRoot to enemy |
| 7 | GA_FatherElectricTrap | Apply GE_ElectricWebDamage to enemy |

### **Related Documents**

| Document | Version | Purpose |
|----------|---------|---------|
| Father_Companion_Technical_Reference | v5.3 | Projectile system, v2.2 systems |
| DefaultGameplayTags_FatherCompanion | v3.5 | Tag definitions |
| Father_Companion_System_Setup_Guide | v1.3 | BP_FatherCompanion setup |
| GA_FatherEngineer_Implementation_Guide | v3.6 | Form ability that enables ElectricTrap |

---

**END OF GA_FATHERELECTRICTRAP IMPLEMENTATION GUIDE v2.8**

**Engineer Form - NarrativeProjectile Integration**

**Unreal Engine 5.6 + Narrative Pro v2.2**

**Blueprint-Only Implementation**
