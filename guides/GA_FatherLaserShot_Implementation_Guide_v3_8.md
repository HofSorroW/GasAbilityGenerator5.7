# GA_FatherLaserShot - Implementation Guide

## VERSION 3.8 - Form State Tag Update (INV-1 Compliant) | Unreal Engine 5.7 | Narrative Pro Plugin v2.2

---

## DOCUMENT INFORMATION

| Field | Value |
|-------|-------|
| Ability Name | GA_FatherLaserShot |
| Form | Crawler |
| Type | AI Combat (Autonomous) |
| Parent Class | NarrativeGameplayAbility |
| Projectile Parent | NarrativeProjectile |
| Input | None (AI-controlled) |
| Damage System | NarrativeDamageExecCalc |
| Version | 3.8 |

---

## TABLE OF CONTENTS

| Section | Description |
|---------|-------------|
| Introduction | Overview, key features, system flow |
| Prerequisites | Required assets before implementation |
| Phase 1 | Gameplay Tags Setup |
| Phase 2 | Laser Projectile Actor (NarrativeProjectile) |
| Phase 3 | Gameplay Effects |
| Phase 4 | Laser Shot Ability (AbilityTask_SpawnProjectile) |
| Phase 5 | AI Behavior Tree Integration |
| Phase 6 | Father Ability Integration |
| Changelog | Version history |
| Quick Reference | Summary tables |

---

## INTRODUCTION

### Overview

GA_FatherLaserShot is an autonomous AI-controlled ranged attack that allows the father companion to shoot laser projectiles at enemies when in Crawler form. The father AI automatically triggers this ability based on enemy proximity and line-of-sight. On hit, the projectile marks enemies via GA_FatherMark integration, enabling wall visibility and damage bonus effects.

### Key Features

| Feature | Description |
|---------|-------------|
| Autonomous Activation | Father AI triggers based on enemy detection |
| Form Restriction | Only activates in Crawler form when detached |
| NarrativeProjectile System | Uses v2.2 native projectile base class |
| AbilityTask_SpawnProjectile | Native task handles spawn and target data |
| Damage Integration | Uses NarrativeDamageExecCalc for proper armor scaling |
| Enemy Marking | Triggers GA_FatherMark on hit for +10% damage bonus |
| Cooldown Control | Tag-based cooldown prevents ability spam |
| Multiplayer Compatible | Full server-client replication |

### Technical Specifications

| Parameter | Value |
|-----------|-------|
| Projectile Speed | 5000 units/second |
| Damage Source | Father AttackDamage attribute (via NarrativeDamageExecCalc) |
| Effective Range | 1500 units |
| Cooldown Duration | 3 seconds |
| Projectile Lifespan | 3 seconds |

### System Flow

| Step | Action |
|------|--------|
| 1 | AI Behavior Tree detects enemy in range |
| 2 | BT Task calls TryActivateAbilitiesByTag |
| 3 | GA_FatherLaserShot activates |
| 4 | AbilityTask_SpawnProjectile spawns BP_LaserProjectile |
| 5 | Projectile travels toward target |
| 6 | On collision, projectile calls SetProjectileTargetData |
| 7 | OnTargetData delegate fires in ability |
| 8 | Ability applies GE_LaserDamage and marks enemy |
| 9 | GE_LaserCooldown applied to father |
| 10 | Ability ends |

---

## PREREQUISITES

### Required Assets

| Asset | Status |
|-------|--------|
| BP_FatherCompanion | Must exist (NarrativeNPCCharacter child) |
| BT_FatherCompanion | Must exist (Behavior Tree) |
| BB_FatherCompanion | Must exist (Blackboard with TargetEnemy key) |
| GA_FatherMark | Must exist (for enemy marking integration) |
| MarkEnemy function | Must exist in BP_FatherCompanion |
| AC_FatherCompanion_Default | Must exist (AbilityConfiguration data asset) |
| NPCDef_FatherCompanion | Must exist (NPCDefinition asset) |

### Required Knowledge

| Topic | Reference |
|-------|-----------|
| Blueprint basics | Unreal Engine documentation |
| Gameplay Ability System | Narrative Pro documentation |
| Behavior Trees | AI system documentation |

---

## PHASE 1: GAMEPLAY TAGS SETUP

### Create Required Tags

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Crawler.LaserShot | Autonomous laser projectile attack in Crawler form |
| Cooldown.Father.Crawler.LaserShot | Cooldown for laser shot ability |
| Father.State.ShootingLaser | Father is actively shooting laser |
| Effect.Damage.Laser | Damage type from laser projectile |

### Verify Existing Tags

| Tag Name | Purpose |
|----------|---------|
| Effect.Father.FormState.Crawler | Must be in Crawler form (Activation Required) |
| Father.State.Detached | Must not be attached (Activation Required) |
| Father.State.Recruited | Must be recruited (Activation Required) |

---

## PHASE 2: LASER PROJECTILE ACTOR (NARRATIVEPROJECTILE)

### 3) Create Projectile Blueprint

#### 3.1) Create Folder Structure
   - 3.1.1) Navigate to /Content/FatherCompanion/ in Content Browser
   - 3.1.2) Right-click in empty space
   - 3.1.3) Select New Folder
   - 3.1.4) Name folder: LaserAbility
   - 3.1.5) Open LaserAbility folder

#### 3.2) Create NarrativeProjectile Blueprint
   - 3.2.1) Right-click in empty space
   - 3.2.2) Select Blueprint Class
   - 3.2.3) Click All Classes dropdown
   - 3.2.4) Search: NarrativeProjectile
   - 3.2.5) Select NarrativeProjectile
   - 3.2.6) Click Select button
   - 3.2.7) Name: BP_LaserProjectile
   - 3.2.8) Double-click BP_LaserProjectile to open

#### 3.3) Configure Actor Replication
   - 3.3.1) Click Class Defaults button
   - 3.3.2) In Details panel, find Replication section
   - 3.3.3) Replicates: Check (enable)
   - 3.3.4) Replicate Movement: Check (enable)

### 4) Add Projectile Components

#### 4.1) Add Static Mesh Component
   - 4.1.1) In Components panel, click Add button
   - 4.1.2) Search: Static Mesh
   - 4.1.3) Select Static Mesh Component
   - 4.1.4) Rename to: LaserMesh
   - 4.1.5) Select LaserMesh in hierarchy
   - 4.1.6) In Details panel:
   - 4.1.6.1) Static Mesh: Select elongated shape (Cylinder or custom)
   - 4.1.6.2) Scale: X=0.1, Y=0.1, Z=0.5
   - 4.1.6.3) Collision Presets: NoCollision

#### 4.2) Create Emissive Material
   - 4.2.1) In Content Browser, right-click
   - 4.2.2) Select Material
   - 4.2.3) Name: M_LaserGlow
   - 4.2.4) Open material editor
   - 4.2.5) Add Constant3Vector node
   - 4.2.6) Set color: R=1.0, G=0.1, B=0.1
   - 4.2.7) Add Multiply node
   - 4.2.8) Connect color to A input
   - 4.2.9) Add Constant node with value: 50
   - 4.2.10) Connect constant to B input
   - 4.2.11) Connect Multiply output to Emissive Color
   - 4.2.12) Click Apply and Save
   - 4.2.13) Back in BP_LaserProjectile:
   - 4.2.13.1) Select LaserMesh
   - 4.2.13.2) Material: Select M_LaserGlow

#### 4.3) Add Collision Component
   - 4.3.1) In Components panel, click Add
   - 4.3.2) Search: Sphere Collision
   - 4.3.3) Select Sphere Collision
   - 4.3.4) Rename to: DamageCollision
   - 4.3.5) Drag DamageCollision onto DefaultSceneRoot
   - 4.3.6) Select DamageCollision, in Details panel:
   - 4.3.6.1) Sphere Radius: 30.0
   - 4.3.6.2) Collision Presets: Custom
   - 4.3.6.3) Collision Enabled: Query Only (No Physics)
   - 4.3.6.4) Expand Collision Responses:
   - 4.3.6.4.a) Set all to Ignore
   - 4.3.6.4.b) Set Pawn to Overlap

#### 4.4) Add Projectile Movement Component
   - 4.4.1) Click Add in Components panel
   - 4.4.2) Search: Projectile Movement
   - 4.4.3) Select Projectile Movement Component
   - 4.4.4) In Details panel:
   - 4.4.4.1) Initial Speed: 5000.0
   - 4.4.4.2) Max Speed: 5000.0
   - 4.4.4.3) Projectile Gravity Scale: 0.0
   - 4.4.4.4) Rotation Follows Velocity: Check
   - 4.4.4.5) Should Bounce: Uncheck

### 5) Create Projectile Variables

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| HasHitTarget | Boolean | false | No |

### 6) Implement Projectile BeginPlay

#### 6.1) Set Lifespan
   - 6.1.1) Click Event Graph tab
   - 6.1.2) Find Event BeginPlay node
   - 6.1.3) From Event BeginPlay:
   - 6.1.3.1) Add Set Life Span node
   - 6.1.3.2) In Life Span: 3.0

### 7) Implement Overlap Detection with SetProjectileTargetData

#### 7.1) Create OnComponentBeginOverlap Event
   - 7.1.1) Select DamageCollision in Components panel
   - 7.1.2) In Details panel, scroll to Events section
   - 7.1.3) Click + next to On Component Begin Overlap
   - 7.1.4) Event node created in Event Graph

#### 7.2) Prevent Multiple Hits
   - 7.2.1) From On Component Begin Overlap:
   - 7.2.1.1) Add Branch node
   - 7.2.2) From HasHitTarget variable:
   - 7.2.2.1) Add NOT Boolean node
   - 7.2.2.2) Connect HasHitTarget to NOT input
   - 7.2.3) Connect NOT output to Branch Condition

#### 7.3) Validate Hit Target
   - 7.3.1) From Branch True pin:
   - 7.3.1.1) Add Cast To Character node
   - 7.3.1.2) Connect Other Actor to Object input
   - 7.3.2) From Cast successful execution:
   - 7.3.2.1) Add Get Narrative Character node
   - 7.3.2.2) Target: Self (inherited from NarrativeProjectile)
   - 7.3.3) From Get Narrative Character Return Value:
   - 7.3.3.1) Add Is Valid node
   - 7.3.4) From Is Valid execution True pin:
   - 7.3.4.1) Add Does Not Equal node
   - 7.3.4.2) Connect As Character to first input
   - 7.3.4.3) Connect Get Narrative Character to second input
   - 7.3.5) From Does Not Equal:
   - 7.3.5.1) Add Branch node
   - 7.3.5.2) Connect comparison result to Condition

#### 7.4) Set Hit Flag and Create Target Data
   - 7.4.1) From Branch True (not owner):
   - 7.4.1.1) Add Set node for HasHitTarget
   - 7.4.1.2) Value: true (checked)
   - 7.4.2) From Set HasHitTarget:
   - 7.4.2.1) Add Make Target Data Handle from Actor node
   - 7.4.2.2) Target Actor: Connect As Character (hit target)

#### 7.5) Broadcast Target Data via NarrativeProjectile System
   - 7.5.1) From Make Target Data Handle:
   - 7.5.1.1) Add Set Projectile Target Data node
   - 7.5.1.2) Target: Self
   - 7.5.1.3) Target Handle: Connect from Make Target Data Handle

#### 7.6) Destroy Projectile
   - 7.6.1) From Set Projectile Target Data execution:
   - 7.6.1.1) Add Destroy Actor node
   - 7.6.1.2) Target: Self

#### 7.7) Compile and Save
   - 7.7.1) Click Compile button
   - 7.7.2) Click Save button

---

## PHASE 3: GAMEPLAY EFFECTS

### 8) Create Damage Effect

#### 8.1) Create GE_LaserDamage Blueprint
   - 8.1.1) Navigate to /Content/FatherCompanion/LaserAbility/
   - 8.1.2) Right-click in Content Browser
   - 8.1.3) Select Blueprint Class
   - 8.1.4) Expand All Classes section
   - 8.1.5) Search: GameplayEffect
   - 8.1.6) Select GameplayEffect
   - 8.1.7) Click Select button
   - 8.1.8) Name: GE_LaserDamage
   - 8.1.9) Double-click to open

#### 8.2) Configure GE_LaserDamage Properties

| Property | Value |
|----------|-------|
| Duration Policy | Instant |
| Components | Executions |
| Execution Definitions [0] -> Calculation Class | NarrativeDamageExecCalc |
| Asset Tags -> Add to Inherited [0] | Effect.Damage.Laser |

#### 8.3) Compile and Save

### 9) Create Cooldown Effect

#### 9.1) Create GE_LaserCooldown Blueprint
   - 9.1.1) Right-click in Content Browser
   - 9.1.2) Select Blueprint Class
   - 9.1.3) Expand All Classes section
   - 9.1.4) Search: GameplayEffect
   - 9.1.5) Select GameplayEffect
   - 9.1.6) Click Select button
   - 9.1.7) Name: GE_LaserCooldown
   - 9.1.8) Double-click to open

#### 9.2) Configure GE_LaserCooldown Properties

| Property | Value |
|----------|-------|
| Duration Policy | Has Duration |
| Duration Magnitude -> Scalable Float Magnitude | 3.0 |
| Components | Granted Tags |
| Granted Tags -> Add to Inherited [0] | Cooldown.Father.Crawler.LaserShot |

#### 9.3) Compile and Save

---

## PHASE 4: LASER SHOT ABILITY (ABILITYTASK_SPAWNPROJECTILE)

### 10) Create Ability Blueprint

#### 10.1) Create GA_FatherLaserShot Blueprint
   - 10.1.1) Navigate to /Content/FatherCompanion/LaserAbility/
   - 10.1.2) Right-click in Content Browser
   - 10.1.3) Select Blueprint Class
   - 10.1.4) Expand All Classes section
   - 10.1.5) Search: NarrativeGameplayAbility
   - 10.1.6) Select NarrativeGameplayAbility
   - 10.1.7) Click Select button
   - 10.1.8) Name: GA_FatherLaserShot
   - 10.1.9) Double-click to open

### 11) Configure Ability Tags

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Crawler.LaserShot |
| Activation Required Tags | Effect.Father.FormState.Crawler, Father.State.Detached, Father.State.Recruited |
| Activation Blocked Tags | Cooldown.Father.Crawler.LaserShot |
| Activation Owned Tags | Father.State.ShootingLaser |

### 12) Configure Replication Settings

#### 12.1) Set Replication Policy
   - 12.1.1) Scroll to Replication section
   - 12.1.2) Replication Policy: Replicate

#### 12.2) Set Net Execution Policy
   - 12.2.1) Net Execution Policy: Server Only

#### 12.3) Set Instancing Policy
   - 12.3.1) Scroll to Instancing section
   - 12.3.2) Instancing Policy: Instanced Per Actor

#### 12.4) Disable Input Replication
   - 12.4.1) Scroll to Advanced section
   - 12.4.2) Replicate Input Directly: Uncheck

### 13) Create Ability Variables

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| ProjectileClass | Class (NarrativeProjectile) | BP_LaserProjectile | Yes |
| MaxRange | Float | 1500.0 | Yes |
| LaserSpawnOffset | Vector | (80.0, 0.0, 30.0) | Yes |
| FatherRef | BP_FatherCompanion (Object Reference) | None | No |
| TargetRef | Character (Object Reference) | None | No |

### 14) Implement Ability Activation Logic

#### 14.1) Create Event ActivateAbility
   - 14.1.1) Click Event Graph tab
   - 14.1.2) Right-click in empty graph space
   - 14.1.3) Search: ActivateAbility
   - 14.1.4) Select Event ActivateAbility

#### 14.2) Get Father Reference
   - 14.2.1) From Event ActivateAbility:
   - 14.2.1.1) Add Get Avatar Actor From Actor Info node
   - 14.2.2) From Get Avatar Return Value:
   - 14.2.2.1) Add Cast To BP_FatherCompanion node
   - 14.2.3) Connect Event ActivateAbility execution to Cast input pin
   - 14.2.4) From Cast successful execution:
   - 14.2.4.1) Add Set FatherRef node
   - 14.2.4.2) Connect As BP Father Companion to input

#### 14.3) Handle Cast Failure
   - 14.3.1) From Cast Cast Failed pin:
   - 14.3.1.1) Add End Ability node
   - 14.3.1.2) Was Cancelled: Uncheck

#### 14.4) Get Target from AI Blackboard
   - 14.4.1) From Set FatherRef execution:
   - 14.4.1.1) From FatherRef output pin
   - 14.4.1.2) Add Get Controller node
   - 14.4.2) From Get Controller Return Value:
   - 14.4.2.1) Add Cast To AIController node
   - 14.4.3) From Cast As AIController pin:
   - 14.4.3.1) Add Get Blackboard Component node
   - 14.4.4) From Get Blackboard Component Return Value:
   - 14.4.4.1) Add Get Value as Object node
   - 14.4.5) On Get Value node:
   - 14.4.5.1) Key Name: TargetEnemy
   - 14.4.6) From Get Value Return Value:
   - 14.4.6.1) Add Cast To Character node

#### 14.5) Validate Target
   - 14.5.1) From Cast To Character Cast Failed pin:
   - 14.5.1.1) Add End Ability node
   - 14.5.1.2) Was Cancelled: Uncheck
   - 14.5.2) From Cast successful execution:
   - 14.5.2.1) Add Set TargetRef node
   - 14.5.2.2) Connect As Character to input
   - 14.5.3) From Set TargetRef:
   - 14.5.3.1) Add Is Valid node
   - 14.5.3.2) Connect TargetRef to Input Object
   - 14.5.4) From Is Valid:
   - 14.5.4.1) Add Branch node
   - 14.5.4.2) Condition: Connect Is Valid return

#### 14.6) Handle Invalid Target
   - 14.6.1) From Branch False pin:
   - 14.6.1.1) Add End Ability node
   - 14.6.1.2) Was Cancelled: Uncheck

#### 14.7) Check Range
   - 14.7.1) From Branch True pin:
   - 14.7.1.1) Add Get Actor Location node
   - 14.7.1.2) Target: Connect FatherRef
   - 14.7.2) Add another Get Actor Location node:
   - 14.7.2.1) Target: Connect TargetRef
   - 14.7.3) Add Distance (Vector) node:
   - 14.7.3.1) V1: Connect father location
   - 14.7.3.2) V2: Connect target location
   - 14.7.4) Add Less Than or Equal node:
   - 14.7.4.1) A: Connect Distance return
   - 14.7.4.2) B: Drag from MaxRange variable
   - 14.7.5) Add Branch node:
   - 14.7.5.1) Condition: Connect comparison result

#### 14.8) Handle Out of Range
   - 14.8.1) From Branch False pin (out of range):
   - 14.8.1.1) Add End Ability node
   - 14.8.1.2) Was Cancelled: Uncheck

### 15) Spawn Projectile Using AbilityTask_SpawnProjectile

#### 15.1) Calculate Spawn Location
   - 15.1.1) From Branch True pin (in range):
   - 15.1.1.1) Add Get Actor Location node
   - 15.1.1.2) Target: Connect FatherRef
   - 15.1.2) Add Get Actor Rotation node:
   - 15.1.2.1) Target: Connect FatherRef
   - 15.1.3) Drag from LaserSpawnOffset variable:
   - 15.1.3.1) Add Rotate Vector node
   - 15.1.3.2) A: Connect LaserSpawnOffset
   - 15.1.3.3) B: Connect father rotation
   - 15.1.4) Add Vector + Vector node:
   - 15.1.4.1) A: Connect father location
   - 15.1.4.2) B: Connect rotated offset

#### 15.2) Calculate Spawn Rotation
   - 15.2.1) Add Get Actor Location node:
   - 15.2.1.1) Target: Connect TargetRef
   - 15.2.2) Add Find Look at Rotation node:
   - 15.2.2.1) Start: Connect sum from 15.1.4
   - 15.2.2.2) Target: Connect target location

#### 15.3) Create Spawn Transform
   - 15.3.1) Add Make Transform node:
   - 15.3.1.1) Location: Connect sum from 15.1.4
   - 15.3.1.2) Rotation: Connect Find Look at Rotation return
   - 15.3.1.3) Scale: X=1.0, Y=1.0, Z=1.0

#### 15.4) Spawn Projectile with AbilityTask
   - 15.4.1) Right-click in graph
   - 15.4.2) Search: Spawn Projectile
   - 15.4.3) Select Spawn Projectile (AbilityTask_SpawnProjectile)
   - 15.4.4) Configure node:
   - 15.4.4.1) Task Instance Name: LaserProjectileTask
   - 15.4.4.2) Class: Connect ProjectileClass variable
   - 15.4.4.3) Projectile Spawn Transform: Connect Make Transform return

### 16) Handle Target Data from Projectile Hit

#### 16.1) Bind to OnTargetData Delegate
   - 16.1.1) From Spawn Projectile On Target Data execution pin:
   - 16.1.1.1) This fires when projectile calls SetProjectileTargetData

#### 16.2) Get Hit Actor from Target Data
   - 16.2.1) From On Target Data:
   - 16.2.1.1) Drag from Data pin
   - 16.2.1.2) Add Get Actors from Target Data node
   - 16.2.2) From Get Actors Return Value:
   - 16.2.2.1) Add Get (Copy) node with index 0
   - 16.2.3) From Get (Copy) Return Value:
   - 16.2.3.1) Add Cast To Character node

#### 16.3) Apply Damage Effect to Hit Target
   - 16.3.1) From Cast successful execution:
   - 16.3.1.1) Add Get Ability System Component node
   - 16.3.1.2) Target: Connect As Character
   - 16.3.2) From Get ASC Return Value:
   - 16.3.2.1) Add Is Valid node
   - 16.3.3) From Is Valid Is Valid pin:
   - 16.3.3.1) Add Make Outgoing Spec node
   - 16.3.3.2) Target: Get Ability System Component from FatherRef
   - 16.3.3.3) Gameplay Effect Class: Select GE_LaserDamage
   - 16.3.3.4) Level: 1.0
   - 16.3.4) From Make Outgoing Spec:
   - 16.3.4.1) Add Apply Gameplay Effect Spec to Target node
   - 16.3.4.2) Spec Handle: Connect from Make Outgoing Spec
   - 16.3.4.3) Target: Connect target ASC from step 16.3.1

#### 16.4) Mark Enemy After Damage
   - 16.4.1) From Apply Gameplay Effect Spec to Target execution:
   - 16.4.1.1) From FatherRef variable
   - 16.4.1.2) Add Mark Enemy node
   - 16.4.1.3) Target: Connect FatherRef
   - 16.4.1.4) Enemy input: Connect As Character (hit target)

### 17) Handle Projectile Destroyed (No Hit)

#### 17.1) Bind to OnDestroyed Delegate
   - 17.1.1) From Spawn Projectile On Destroyed execution pin:
   - 17.1.1.1) This fires when projectile is destroyed without hitting target

#### 17.2) End Ability on Destroyed
   - 17.2.1) From On Destroyed:
   - 17.2.1.1) Connect to Apply Cooldown section (shared)

### 18) Apply Cooldown and End Ability

#### 18.1) Apply Cooldown Effect
   - 18.1.1) After Mark Enemy execution (and from On Destroyed path):
   - 18.1.1.1) Add Apply Gameplay Effect to Owner node
   - 18.1.1.2) Gameplay Effect Class: Select GE_LaserCooldown

#### 18.2) End Ability
   - 18.2.1) From Apply Gameplay Effect to Owner:
   - 18.2.1.1) Add End Ability node
   - 18.2.1.2) Was Cancelled: Uncheck

#### 18.3) Compile and Save
   - 18.3.1) Click Compile button
   - 18.3.2) Click Save button

---

## PHASE 5: AI BEHAVIOR TREE INTEGRATION

### 19) Configure BTTask_ActivateAbilityByClass

Narrative Pro provides BTTask_ActivateAbilityByClass - a built-in reusable task for AI ability activation. No custom task creation needed.

#### 19.1) Locate Built-in Task
   - 19.1.1) BTTask_ActivateAbilityByClass is located in: Plugins/Narrative Pro/Pro/Core/AI/Tasks/
   - 19.1.2) This task can activate any GameplayAbility by class reference

#### 19.2) Task Configuration Properties

| Property | Value | Purpose |
|----------|-------|---------|
| Ability to Activate | GA_FatherLaserShot | Select from dropdown |
| Custom Description | Activate Laser Shot | Display name in BT |
| Tick Interval | -1.0 | No ticking needed |

### 20) Update Behavior Tree

#### 20.1) Open Father Behavior Tree
   - 20.1.1) Navigate to /Content/FatherCompanion/AI/
   - 20.1.2) Find and open: BT_FatherCompanion

#### 20.2) Locate Attack Selection Area
   - 20.2.1) Find existing attack logic in tree
   - 20.2.2) Add Laser Attack Sequence BEFORE Melee Attack

#### 20.3) Create Laser Shot Sequence

##### 20.3.1) Add New Sequence Node
   - 20.3.1.1) Right-click on Attack Choice Selector
   - 20.3.1.2) Select Add Composite then Sequence
   - 20.3.1.3) Drag new Sequence to be FIRST child
   - 20.3.1.4) Right-click then Rename to Laser Shot Attack

##### 20.3.2) Add Blackboard Decorator
   - 20.3.2.1) Right-click on Laser Shot Attack Sequence
   - 20.3.2.2) Select Add Decorator then Blackboard
   - 20.3.2.3) Select decorator, in Details panel:
   - 20.3.2.3.a) Observer Aborts: Both
   - 20.3.2.3.b) Blackboard Key: Select TargetEnemy
   - 20.3.2.3.c) Key Query: Is Set
   - 20.3.2.3.d) Node Name: Has Target Enemy

##### 20.3.3) Add Laser Shot Task
   - 20.3.3.1) Right-click on Laser Shot Attack Sequence
   - 20.3.3.2) Select Add Task
   - 20.3.3.3) Search: BTTask_ActivateAbilityByClass
   - 20.3.3.4) Click BTTask_ActivateAbilityByClass from list
   - 20.3.3.5) Select the task node in Behavior Tree
   - 20.3.3.6) In Details panel, set Ability to Activate: GA_FatherLaserShot

##### 20.3.4) Add Wait Task
   - 20.3.4.1) Right-click on Laser Shot Attack Sequence again
   - 20.3.4.2) Select Add Task then Wait
   - 20.3.4.3) Select Wait task, in Details panel:
   - 20.3.4.3.a) Wait Time: 0.1

#### 20.4) Save Behavior Tree
   - 20.4.1) Click Save button

---

## PHASE 6: FATHER ABILITY INTEGRATION

### 21) Configure Ability in AbilityConfiguration

#### 21.1) Open Father Ability Configuration
   - 21.1.1) Navigate to /Content/FatherCompanion/Data/
   - 21.1.2) Find and open: AC_FatherCompanion_Default
   - 21.1.3) Data asset editor opens

#### 21.2) Verify Laser Ability Entry
   - 21.2.1) Find Default Abilities array
   - 21.2.2) GA_FatherLaserShot should be at Element [7]
   - 21.2.3) If missing:
   - 21.2.3.1) Click + button to add new element
   - 21.2.3.2) Click dropdown on new element
   - 21.2.3.3) Search: GA_FatherLaserShot
   - 21.2.3.4) Select GA_FatherLaserShot from list

#### 21.3) Save Configuration
   - 21.3.1) Click Save button
   - 21.3.2) Close data asset editor

### 22) Verify NPCDefinition Reference

#### 22.1) Open Father NPCDefinition
   - 22.1.1) Navigate to /Content/FatherCompanion/Data/
   - 22.1.2) Find and open: NPCDef_FatherCompanion

#### 22.2) Verify Ability Configuration Reference
   - 22.2.1) In Details panel, find Ability Configuration property
   - 22.2.2) Verify value is: AC_FatherCompanion_Default
   - 22.2.3) If None: Select AC_FatherCompanion_Default from dropdown

#### 22.3) Save NPCDefinition
   - 22.3.1) Click Save button
   - 22.3.2) Close data asset editor

### 23) Configure Father Faction

#### 23.1) Open Father NPCDefinition
   - 23.1.1) Navigate to /Content/FatherCompanion/Data/
   - 23.1.2) Find and open: NPCDef_FatherCompanion

#### 23.2) Set Faction Tag
   - 23.2.1) In Details panel, find Factions property
   - 23.2.2) Expand Factions array
   - 23.2.3) Click + to add element
   - 23.2.4) Select: Narrative.Factions.Heroes
   - 23.2.5) This ensures father is friendly to player and hostile to enemies

#### 23.3) Save NPCDefinition
   - 23.3.1) Click Save button
   - 23.3.2) Close data asset editor

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 3.7 | January 2026 | Replaced custom BTTask_ActivateLaserShot with Narrative Pro built-in BTTask_ActivateAbilityByClass. Removed Section 19 custom task creation. Updated Section 20.3.3 task reference. Updated Blueprints Created and AI Integration tables. |
| 3.6 | January 2026 | Simplified documentation: Tag configuration (Section 11), variable creation (Sections 5, 13), and GameplayEffect configuration (Sections 8-9) converted to markdown tables. |
| 3.5 | January 2026 | Simplified PHASE 1 tag creation - replaced detailed step-by-step instructions with simple tag list tables. |
| 3.4 | December 2025 | Updated to Narrative Pro v2.2. Changed BP_LaserProjectile parent from Actor to NarrativeProjectile. Replaced SpawnActorFromClass with AbilityTask_SpawnProjectile. Updated projectile hit detection to use SetProjectileTargetData. Moved damage application from projectile to ability via OnTargetData delegate. Removed OwnerFather variable (NarrativeProjectile handles via GetNarrativeCharacter). |
| 3.3 | December 2025 | Added Father.State.Recruited to Activation Required Tags (3 elements total). Updated Tags Summary. Added Related Documents table. |
| 1.0 | 2025 | Initial implementation guide |
| 1.2 | 2025 | Added technical specifications |
| 2.0 | 2025 | Major reformat to standard structure. Fixed damage system to use NarrativeDamageExecCalc. Updated GE components to UE 5.6 format. |
| 3.0 | 2025 | Added GA_FatherMark integration for enemy marking on hit. Updated ability granting to use NPCDefinition/AbilityConfiguration system. Updated prerequisites with GA_FatherMark dependency. Added Integration Requirements to Quick Reference. |
| 3.1 | 2025 | Removed DamageAmount variable (damage now sourced from father AttackDamage attribute via NarrativeDamageExecCalc). Updated Blackboard access to use controller pattern. Added faction configuration (Narrative.Factions.Heroes). Resolved all pending clarification items. |
| 3.2 | December 2025 | Hierarchical tags: Changed Ability.Father.LaserShot to Ability.Father.Crawler.LaserShot, Cooldown.Father.LaserShot to Cooldown.Father.Crawler.LaserShot. Changed Net Execution Policy from Local Predicted to Server Only. |

---

## QUICK REFERENCE

### Tags Summary

| Tag | Category | Purpose |
|-----|----------|---------|
| Ability.Father.Crawler.LaserShot | Ability | Identifies this ability |
| Cooldown.Father.Crawler.LaserShot | Cooldown | Blocks activation during cooldown |
| Father.State.ShootingLaser | State | Active while ability executes |
| Effect.Damage.Laser | Effect | Identifies damage type |
| Effect.Father.FormState.Crawler | Required | Must be in Crawler form |
| Father.State.Detached | Required | Must not be attached |
| Father.State.Recruited | Required | Must be recruited |

### Ability Configuration

| Property | Value |
|----------|-------|
| Parent Class | NarrativeGameplayAbility |
| Projectile Parent | NarrativeProjectile |
| Spawn Task | AbilityTask_SpawnProjectile |
| Instancing Policy | Instanced Per Actor |
| Replication Policy | Replicate |
| Net Execution Policy | Server Only |
| Replicate Input Directly | FALSE |
| InputTag | None |
| bActivateAbilityOnGranted | FALSE |

### Variables Summary

| Variable | Type | Default | Editable |
|----------|------|---------|----------|
| ProjectileClass | Class (NarrativeProjectile) | BP_LaserProjectile | Yes |
| MaxRange | Float | 1500.0 | Yes |
| LaserSpawnOffset | Vector | (80, 0, 30) | Yes |
| FatherRef | BP_FatherCompanion | None | No |
| TargetRef | Character | None | No |

### Gameplay Effects

| Effect | Duration | Purpose |
|--------|----------|---------|
| GE_LaserDamage | Instant | Applies damage via NarrativeDamageExecCalc |
| GE_LaserCooldown | 3.0 seconds | Grants Cooldown.Father.Crawler.LaserShot tag |

### Projectile Configuration

| Property | Value |
|----------|-------|
| Parent Class | NarrativeProjectile |
| Initial Speed | 5000.0 |
| Max Speed | 5000.0 |
| Gravity Scale | 0.0 |
| Lifespan | 3.0 seconds |
| Collision Radius | 30.0 |

### Blueprints Created

| Blueprint | Type | Location |
|-----------|------|----------|
| GA_FatherLaserShot | GameplayAbility | /Content/FatherCompanion/LaserAbility/ |
| BP_LaserProjectile | NarrativeProjectile | /Content/FatherCompanion/LaserAbility/ |
| GE_LaserDamage | GameplayEffect | /Content/FatherCompanion/LaserAbility/ |
| GE_LaserCooldown | GameplayEffect | /Content/FatherCompanion/LaserAbility/ |
| M_LaserGlow | Material | /Content/FatherCompanion/LaserAbility/ |

### AI Integration

| Component | Configuration |
|-----------|---------------|
| Behavior Tree | BT_FatherCompanion |
| Task Node | BTTask_ActivateAbilityByClass (Narrative Pro built-in) |
| Ability to Activate | GA_FatherLaserShot |
| Blackboard Key | TargetEnemy |
| Sequence Priority | Before Melee Attack |

### Integration Requirements

| Requirement | Location | Purpose |
|-------------|----------|---------|
| MarkEnemy function | BP_FatherCompanion | Called after projectile damage |
| GA_FatherMark ability | AC_FatherCompanion_Default | Handles marking logic |
| MarkedEnemies array | BP_FatherCompanion | Tracks marked enemies |
| Narrative.Factions.Heroes | NPCDef_FatherCompanion | Enables friendly fire prevention via NarrativeDamageExecCalc |

### v2.2 Feature Integration

| Feature | Usage |
|---------|-------|
| NarrativeProjectile | Base class for BP_LaserProjectile |
| AbilityTask_SpawnProjectile | Spawns projectile with target data handling |
| OnProjectileTargetData | Delegate fires when projectile hits |
| SetProjectileTargetData | Called by projectile on collision |
| GetNarrativeCharacter | Returns owning character (replaces OwnerFather) |

### Related Documents

| Document | Version | Purpose |
|----------|---------|---------|
| Father_Companion_Technical_Reference | v5.9 | BTTask_ActivateAbilityByClass, cross-actor patterns |
| DefaultGameplayTags_FatherCompanion | v3.5 | Tag definitions |
| Father_Companion_System_Setup_Guide | v2.0 | BP_FatherCompanion setup |
| GA_FatherCrawler_Implementation_Guide | v2.9 | Form ability that enables LaserShot |

---

**END OF IMPLEMENTATION GUIDE**
