# Possessed Exploder Enemy Implementation Guide

## VERSION 2.1

## Unreal Engine 5.6 + Narrative Pro v2.2

## Blueprint-Only Implementation

---

## DOCUMENT INFORMATION

| Field | Value |
|-------|-------|
| Document Type | Enemy NPC Implementation Guide |
| Enemy Name | Possessed Exploder |
| Last Updated | January 2026 |
| Version | 2.1 |

---

## TABLE OF CONTENTS

1. [Introduction](#introduction)
2. [Quick Reference](#quick-reference)
3. [Phase 1: Verify Gameplay Tags](#phase-1-verify-gameplay-tags)
4. [Phase 2: Create Gameplay Effects](#phase-2-create-gameplay-effects)
5. [Phase 3: Create BP_PossessedExploder](#phase-3-create-bp_possessedexploder)
6. [Phase 4: Create BTS_CheckExplosionProximity Service](#phase-4-create-bts_checkexplosionproximity-service)
7. [Phase 5: Create BT_Explode Behavior Tree](#phase-5-create-bt_explode-behavior-tree)
8. [Phase 6: Create BPA_Explode Activity](#phase-6-create-bpa_explode-activity)
9. [Phase 7: Create Ability Configuration](#phase-7-create-ability-configuration)
10. [Phase 8: Create Activity Configuration](#phase-8-create-activity-configuration)
11. [Phase 9: Create NPCDefinition](#phase-9-create-npcdefinition)
12. [Phase 10: Configure Faction Attitudes](#phase-10-configure-faction-attitudes)
13. [Phase 11: World Placement](#phase-11-world-placement)
14. [Changelog](#changelog)

---

## INTRODUCTION

### Enemy Overview

| Property | Value |
|----------|-------|
| Name | Possessed Exploder |
| Type | Suicidal Hovering Enemy |
| Parent Class | NarrativeNPCCharacter |
| Movement Mode | Flying |
| Combat Style | Kamikaze - pursues and detonates |

### Behavior Flow

| State | Trigger | Action |
|-------|---------|--------|
| Idle | Default | Hovers in place with ambient glow |
| Alert | Player detected via AIPerception | Plays alert VFX, begins pursuit |
| Pursuit | Goal_Attack assigned | Flies toward player |
| Explosion | Within ExplosionRadius | Detonates, deals radial damage, destroys self |

### Narrative Pro Systems Leveraged

| System | Asset | Purpose |
|--------|-------|---------|
| Blackboard | BB_Attack (existing) | Stores AttackTarget, TargetLocation |
| Goal | Goal_Attack (existing) | Stores target actor reference |
| Goal Generator | GoalGenerator_Attack (existing) | Creates attack goals from perception |
| Damage System | NarrativeDamageExecCalc | Calculates explosion damage |
| Faction System | Faction attitudes | Determines hostile relationships |

---

## QUICK REFERENCE

### Gameplay Tags

| Tag | Purpose |
|-----|---------|
| State.Enemy.PossessedExploder | NPC identifier |
| State.Enemy.Exploder.Alert | Applied during alert state |
| Faction.Enemy.Possessed | Faction identifier |
| Effect.Enemy.ExplosionDamage | Damage effect tag |
| GameplayCue.Enemy.Exploder.Alert | Alert VFX/SFX trigger |
| GameplayCue.Enemy.Exploder.Explosion | Explosion VFX/SFX trigger |

### Assets Created

| Asset | Type | Location |
|-------|------|----------|
| BP_PossessedExploder | Blueprint | /Game/Enemies/Possessed/ |
| BTS_CheckExplosionProximity | Blueprint | /Game/AI/Services/ |
| BT_Explode | Behavior Tree | /Game/AI/BehaviorTrees/ |
| BPA_Explode | Blueprint | /Game/AI/Activities/ |
| GE_ExploderAttributes | Gameplay Effect | /Game/Enemies/Possessed/Effects/ |
| GE_ExplosionDamage | Gameplay Effect | /Game/Enemies/Possessed/Effects/ |
| AC_PossessedExploder | Data Asset | /Game/Enemies/Possessed/Configurations/ |
| ActConfig_PossessedExploder | Data Asset | /Game/Enemies/Possessed/Configurations/ |
| NPCDef_PossessedExploder | Data Asset | /Game/Enemies/Possessed/Definitions/ |

### Existing Narrative Pro Assets Used

| Asset | Type | Purpose |
|-------|------|---------|
| BB_Attack | Blackboard | AttackTarget, TargetLocation keys |
| Goal_Attack | Goal Class | Target storage |
| GoalGenerator_Attack | Goal Generator | Perception-based target detection |
| NarrativeDamageExecCalc | Execution | Damage calculation |
| BPA_Idle | Activity | Default idle behavior |

### Variable Summary (BP_PossessedExploder)

| Variable | Type | Default | Replicated | Instance Editable |
|----------|------|---------|------------|-------------------|
| ExplosionRadius | Float | 300.0 | No | Yes |
| ExplosionDamage | Float | 150.0 | No | Yes |
| bHasExploded | Boolean | false | No | No |
| bIsAlerted | Boolean | false | No | No |

---

## PHASE 1: VERIFY GAMEPLAY TAGS

### **1) Open Gameplay Tags Manager**

#### 1.1) Navigate to Project Settings
   - 1.1.1) Edit -> Project Settings
   - 1.1.2) Navigate to Project -> Gameplay Tags

#### 1.2) Open Gameplay Tags Editor
   - 1.2.1) Click Manage Gameplay Tags button

### **2) Required Tags

| Tag | Line (if using DefaultGameplayTags.ini) |
|-----|----------------------------------------|
| State.Enemy.PossessedExploder | Add new |
| State.Enemy.Exploder.Alert | Add new |
| Faction.Enemy.Possessed | Add new |
| Effect.Enemy.ExplosionDamage | Add new |
| GameplayCue.Enemy.Exploder.Alert | Add new |
| GameplayCue.Enemy.Exploder.Explosion | Add new |

### **3) Compile and Save

---

## PHASE 2: CREATE GAMEPLAY EFFECTS

### **1) Create GE_ExploderAttributes

#### 1.1) Navigate to Effects Folder
   - 1.1.1) Content Browser -> /Game/Enemies/Possessed/Effects/

#### 1.2) Create Gameplay Effect
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Blueprint Class
   - 1.2.3) Search parent: GameplayEffect
   - 1.2.4) Select GameplayEffect
   - 1.2.5) Name: GE_ExploderAttributes
   - 1.2.6) Double-click to open

#### 1.3) Configure Effect Properties

| Property | Value |
|----------|-------|
| Duration Policy | Instant |

#### 1.4) Configure Modifiers

| Index | Attribute | Modifier Op | Magnitude Type | Scalable Float |
|-------|-----------|-------------|----------------|----------------|
| 0 | MaxHealth | Override | Scalable Float | 50.0 |
| 1 | Health | Override | Scalable Float | 50.0 |
| 2 | Defense | Override | Scalable Float | 0.0 |

#### 1.5) Save Asset

### **2) Create GE_ExplosionDamage

#### 2.1) Create Gameplay Effect
   - 2.1.1) Right-click in Content Browser
   - 2.1.2) Select Blueprint Class
   - 2.1.3) Search parent: GameplayEffect
   - 2.1.4) Select GameplayEffect
   - 2.1.5) Name: GE_ExplosionDamage
   - 2.1.6) Double-click to open

#### 2.2) Configure Effect Properties

| Property | Value |
|----------|-------|
| Duration Policy | Instant |

#### 2.3) Add Executions Component
   - 2.3.1) Find Components array
   - 2.3.2) Click + to add element
   - 2.3.3) Select: Executions (GameplayEffectExecutions_GameplayEffectComponent)

#### 2.4) Configure Execution

| Property | Value |
|----------|-------|
| Calculation Class | NarrativeDamageExecCalc |

#### 2.5) Add Asset Tag
   - 2.5.1) Find Asset Tag property
   - 2.5.2) Add tag: Effect.Enemy.ExplosionDamage

#### 2.6) Save Asset

---

## PHASE 3: CREATE BP_POSSESSEDEXPLODER

### **1) Create Blueprint Class

#### 1.1) Navigate to Enemies Folder
   - 1.1.1) Content Browser -> /Game/Enemies/Possessed/

#### 1.2) Create NPC Blueprint
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Blueprint Class
   - 1.2.3) Search parent: NarrativeNPCCharacter
   - 1.2.4) Select NarrativeNPCCharacter
   - 1.2.5) Name: BP_PossessedExploder
   - 1.2.6) Double-click to open

### **2) Configure Character Movement

#### 2.1) Select CharacterMovement Component
   - 2.1.1) In Components panel, select CharacterMovement

#### 2.2) Set Movement Properties
   - 2.2.1) In Details panel, find Character Movement: Walking section
   - 2.2.2) Max Walk Speed: 400.0
   - 2.2.3) Find Character Movement (General Settings) section
   - 2.2.4) Default Land Movement Mode: Flying
   - 2.2.5) Find Character Movement: Flying section
   - 2.2.6) Max Fly Speed: 400.0

#### 2.3) Configure Nav Agent
   - 2.3.1) Find Nav Agent Props section
   - 2.3.2) Nav Agent Radius: 50.0
   - 2.3.3) Nav Agent Height: 100.0

### **3) Add Visual Components

#### 3.1) Add Point Light for Glow
   - 3.1.1) In Components panel, click Add
   - 3.1.2) Search: Point Light
   - 3.1.3) Select Point Light
   - 3.1.4) Rename to: PossessedGlow
   - 3.1.5) Attach to Mesh component

#### 3.2) Configure Light Properties
   - 3.2.1) Light Color: R=1.0, G=0.2, B=0.1 (Red-Orange)
   - 3.2.2) Intensity: 3000.0
   - 3.2.3) Attenuation Radius: 150.0

### **4) Create Variables

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| ExplosionRadius | Float | 300.0 | Yes |
| ExplosionDamage | Float | 150.0 | Yes |
| bHasExploded | Boolean | false | No |
| bIsAlerted | Boolean | false | No |

### **5) Create TriggerAlert Function

#### 5.1) Create Function
   - 5.1.1) In My Blueprint panel, click + next to Functions
   - 5.1.2) Name: TriggerAlert

#### 5.2) Implement Alert Logic
   - 5.2.1) Add GET bIsAlerted node
   - 5.2.2) From bIsAlerted:
   - 5.2.2.1) Add Branch node
   - 5.2.2.2) Connect bIsAlerted to Condition
   - 5.2.3) From Branch False execution:
   - 5.2.3.1) Add SET bIsAlerted node
   - 5.2.3.2) bIsAlerted: Check (true)
   - 5.2.4) From SET bIsAlerted execution:
   - 5.2.4.1) Add Get Ability System Component node
   - 5.2.4.2) Target: Self
   - 5.2.5) From Get ASC Return Value:
   - 5.2.5.1) Add Execute Gameplay Cue node
   - 5.2.5.2) Gameplay Cue Tag: GameplayCue.Enemy.Exploder.Alert
   - 5.2.6) From Execute Gameplay Cue execution:
   - 5.2.6.1) Add Add Loose Gameplay Tag node
   - 5.2.6.2) Target: Ability System Component
   - 5.2.6.3) Gameplay Tag: State.Enemy.Exploder.Alert

### **6) Create CheckExplosionProximity Function

#### 6.1) Create Function
   - 6.1.1) In My Blueprint panel, click + next to Functions
   - 6.1.2) Name: CheckExplosionProximity

#### 6.2) Add Input Parameter
   - 6.2.1) In Details panel, find Inputs
   - 6.2.2) Click + to add parameter
   - 6.2.3) Name: TargetActor
   - 6.2.4) Type: Actor (Object Reference)

#### 6.3) Add Output Parameter
   - 6.3.1) Click + to add output
   - 6.3.2) Name: bShouldExplode
   - 6.3.3) Type: Boolean

#### 6.4) Implement Proximity Check
   - 6.4.1) Add Is Valid node
   - 6.4.1.1) Input Object: TargetActor
   - 6.4.2) From Is Valid Is Not Valid pin:
   - 6.4.2.1) Add Return Node
   - 6.4.2.2) bShouldExplode: Uncheck (false)
   - 6.4.3) From Is Valid Is Valid pin:
   - 6.4.3.1) Add Get Distance To node
   - 6.4.3.2) Target: Self
   - 6.4.3.3) Other Actor: TargetActor
   - 6.4.4) From Get Distance To Return Value:
   - 6.4.4.1) Add Less Equal (Float <= Float) node
   - 6.4.4.2) A: Distance result
   - 6.4.4.3) B: GET ExplosionRadius
   - 6.4.5) From Less Equal Return Value:
   - 6.4.5.1) Add Return Node
   - 6.4.5.2) Connect comparison result to bShouldExplode

### **7) Create TriggerExplosion Function

#### 7.1) Create Function
   - 7.1.1) In My Blueprint panel, click + next to Functions
   - 7.1.2) Name: TriggerExplosion

#### 7.2) Add Authority Check
   - 7.2.1) Add Has Authority node
   - 7.2.2) From Has Authority Return Value:
   - 7.2.2.1) Add Branch node
   - 7.2.2.2) Connect to Condition
   - 7.2.3) From Branch False:
   - 7.2.3.1) Add Return Node (server-only execution)

#### 7.3) Add Explosion Guard
   - 7.3.1) From Branch True execution:
   - 7.3.1.1) Add GET bHasExploded node
   - 7.3.2) From bHasExploded:
   - 7.3.2.1) Add Branch node
   - 7.3.2.2) Connect to Condition
   - 7.3.3) From Branch True:
   - 7.3.3.1) Add Return Node (prevent double explosion)
   - 7.3.4) From Branch False execution:
   - 7.3.4.1) Add SET bHasExploded node
   - 7.3.4.2) bHasExploded: Check (true)

#### 7.4) Spawn Explosion VFX
   - 7.4.1) From SET bHasExploded execution:
   - 7.4.1.1) Add Get Actor Location node
   - 7.4.1.2) Target: Self
   - 7.4.2) From Get Actor Location execution:
   - 7.4.2.1) Add Spawn System at Location node
   - 7.4.2.2) System Template: NS_Explosion (placeholder)
   - 7.4.2.3) Location: Actor Location result

#### 7.5) Execute Explosion Cue
   - 7.5.1) From Spawn System execution:
   - 7.5.1.1) Add Get Ability System Component node
   - 7.5.1.2) Target: Self
   - 7.5.2) From Get ASC Return Value:
   - 7.5.2.1) Add Execute Gameplay Cue node
   - 7.5.2.2) Gameplay Cue Tag: GameplayCue.Enemy.Exploder.Explosion

#### 7.6) Find Targets in Radius
   - 7.6.1) From Execute Gameplay Cue execution:
   - 7.6.1.1) Add Sphere Overlap Actors node
   - 7.6.2) Configure Sphere Overlap:
   - 7.6.2.1) Sphere Pos: Self Actor Location
   - 7.6.2.2) Sphere Radius: GET ExplosionRadius
   - 7.6.2.3) Object Types: Click array, add Pawn
   - 7.6.2.4) Actors to Ignore: Add Self

#### 7.7) Filter and Damage Each Target
   - 7.7.1) From Sphere Overlap Out Actors:
   - 7.7.1.1) Add For Each Loop node
   - 7.7.2) From For Each Loop Body:
   - 7.7.2.1) Add Cast To NarrativeCharacter node
   - 7.7.2.2) Object: Array Element
   - 7.7.3) From Cast successful execution:
   - 7.7.3.1) Add Get Attitude (ArsenalStatics) node
   - 7.7.3.2) Source: Self
   - 7.7.3.3) Target: As Narrative Character
   - 7.7.4) From Get Attitude Return Value:
   - 7.7.4.1) Add Equal (Enum) node
   - 7.7.4.2) Compare to: Hostile
   - 7.7.5) From Equal Return Value:
   - 7.7.5.1) Add Branch node
   - 7.7.6) From Branch True execution:
   - 7.7.6.1) Continue to damage application

#### 7.8) Apply Damage Effect
   - 7.8.1) From Branch True:
   - 7.8.1.1) Add Get Ability System Component node
   - 7.8.1.2) Target: As Narrative Character (the target)
   - 7.8.2) From Target ASC:
   - 7.8.2.1) Add Is Valid node
   - 7.8.3) From Is Valid Is Valid pin:
   - 7.8.3.1) Add Get Ability System Component node
   - 7.8.3.2) Target: Self (the exploder - source)
   - 7.8.4) From Source ASC:
   - 7.8.4.1) Add Make Outgoing Spec node
   - 7.8.4.2) Gameplay Effect Class: GE_ExplosionDamage
   - 7.8.4.3) Level: 1.0
   - 7.8.5) From Make Outgoing Spec Return Value:
   - 7.8.5.1) Add Assign Tag Set By Caller Magnitude node
   - 7.8.5.2) Spec Handle: Outgoing Spec result
   - 7.8.5.3) Data Tag: SetByCaller.Damage
   - 7.8.5.4) Magnitude: GET ExplosionDamage
   - 7.8.6) From Assign Tag execution:
   - 7.8.6.1) Add Apply Gameplay Effect Spec to Self node
   - 7.8.6.2) Target: Target ASC (the character being damaged)
   - 7.8.6.3) Spec Handle: Assigned spec

#### 7.9) Destroy Self After Delay
   - 7.9.1) From For Each Loop Completed:
   - 7.9.1.1) Add Delay node
   - 7.9.1.2) Duration: 0.1
   - 7.9.2) From Delay execution:
   - 7.9.2.1) Add Destroy Actor node
   - 7.9.2.2) Target: Self

### **8) Configure BeginPlay for Flying Mode

#### 8.1) Open Event Graph
   - 8.1.1) Click Event Graph tab

#### 8.2) Implement BeginPlay
   - 8.2.1) Add Event BeginPlay node
   - 8.2.2) From Event BeginPlay execution:
   - 8.2.2.1) Add Get Character Movement node
   - 8.2.3) From Get Character Movement Return Value:
   - 8.2.3.1) Add Set Movement Mode node
   - 8.2.3.2) New Movement Mode: Flying
   - 8.2.4) From Set Movement Mode execution:
   - 8.2.4.1) Add Call Parent BeginPlay node

### **9) Compile and Save

---

## PHASE 4: CREATE BTS_CHECKEXPLOSIONPROXIMITY SERVICE

### **1) Create Service Blueprint

#### 1.1) Navigate to Services Folder
   - 1.1.1) Content Browser -> /Game/AI/Services/

#### 1.2) Create Blueprint
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Blueprint Class
   - 1.2.3) Search parent: BTService_BlueprintBase
   - 1.2.4) Select BTService_BlueprintBase
   - 1.2.5) Name: BTS_CheckExplosionProximity
   - 1.2.6) Double-click to open

### **2) Create Variables

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| BBKey_AttackTarget | Name | AttackTarget | Yes |

### **3) Implement Receive Tick AI

#### 3.1) Add Event Node
   - 3.1.1) Right-click in Event Graph
   - 3.1.2) Search: Receive Tick AI
   - 3.1.3) Select Event Receive Tick AI

#### 3.2) Get Attack Target from Blackboard
   - 3.2.1) From Event Receive Tick AI execution:
   - 3.2.1.1) Add Get Blackboard Component node
   - 3.2.1.2) Target: Owner Controller
   - 3.2.2) From Get Blackboard Component Return Value:
   - 3.2.2.1) Add Get Value as Object node
   - 3.2.2.2) Key Name: GET BBKey_AttackTarget

#### 3.3) Cast and Check Proximity
   - 3.3.1) From Get Value as Object Return Value:
   - 3.3.1.1) Add Cast To Actor node
   - 3.3.2) From Cast successful execution:
   - 3.3.2.1) Add Get Controlled Pawn node
   - 3.3.2.2) Target: Owner Controller
   - 3.3.3) From Get Controlled Pawn Return Value:
   - 3.3.3.1) Add Cast To BP_PossessedExploder node
   - 3.3.4) From Cast successful execution:
   - 3.3.4.1) Add Check Explosion Proximity node
   - 3.3.4.2) Target: As BP Possessed Exploder
   - 3.3.4.3) Target Actor: As Actor (from first cast)

#### 3.4) Trigger Explosion if Close
   - 3.4.1) From Check Explosion Proximity bShouldExplode:
   - 3.4.1.1) Add Branch node
   - 3.4.2) From Branch True execution:
   - 3.4.2.1) Add Trigger Explosion node
   - 3.4.2.2) Target: As BP Possessed Exploder

### **4) Configure Service Settings

#### 4.1) Click Class Defaults

#### 4.2) Set Service Properties
   - 4.2.1) Interval: 0.1
   - 4.2.2) Random Deviation: 0.0

### **5) Compile and Save

---

## PHASE 5: CREATE BT_EXPLODE BEHAVIOR TREE

### **1) Create Behavior Tree Asset

#### 1.1) Navigate to BehaviorTrees Folder
   - 1.1.1) Content Browser -> /Game/AI/BehaviorTrees/

#### 1.2) Create Behavior Tree
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Artificial Intelligence
   - 1.2.3) Select Behavior Tree
   - 1.2.4) Name: BT_Explode

### **2) Configure Behavior Tree

#### 2.1) Open Behavior Tree
   - 2.1.1) Double-click BT_Explode to open

#### 2.2) Assign Blackboard
   - 2.2.1) Click on ROOT node
   - 2.2.2) In Details panel, find Blackboard Asset
   - 2.2.3) Set to: BB_Attack

### **3) Build Tree Structure

#### 3.1) Add Selector Node
   - 3.1.1) Drag from ROOT node downward
   - 3.1.2) Select Composites
   - 3.1.3) Select Selector

#### 3.2) Add Blackboard Decorator to Selector
   - 3.2.1) Right-click on Selector node
   - 3.2.2) Select Add Decorator
   - 3.2.3) Select Blackboard Based Condition
   - 3.2.4) In Details panel:
   - 3.2.4.1) Key Query: Is Set
   - 3.2.4.2) Blackboard Key: AttackTarget
   - 3.2.4.3) Notify Observer: On Result Change

#### 3.3) Add Sequence Node
   - 3.3.1) Drag from Selector node downward
   - 3.3.2) Select Composites
   - 3.3.3) Select Sequence

#### 3.4) Add BTS_CheckExplosionProximity Service
   - 3.4.1) Right-click on Sequence node
   - 3.4.2) Select Add Service
   - 3.4.3) Search: BTS_CheckExplosionProximity
   - 3.4.4) Select BTS_CheckExplosionProximity

#### 3.5) Add Move To Task
   - 3.5.1) Drag from Sequence node downward
   - 3.5.2) Select Tasks
   - 3.5.3) Select Move To
   - 3.5.4) Click on Move To task
   - 3.5.5) In Details panel:
   - 3.5.5.1) Blackboard Key: AttackTarget
   - 3.5.5.2) Acceptable Radius: 50.0
   - 3.5.5.3) Allow Strafe: Uncheck
   - 3.5.5.4) Reach Test Includes Agent Radius: Check
   - 3.5.5.5) Track Moving Goal: Check

### **4) Final Tree Structure

| Level | Node Type | Node | Decorators/Services |
|-------|-----------|------|---------------------|
| 0 | Root | ROOT | BB_Attack |
| 1 | Composite | Selector | BB Condition (AttackTarget Is Set) |
| 2 | Composite | Sequence | BTS_CheckExplosionProximity |
| 3 | Task | Move To | AttackTarget key |

### **5) Save Behavior Tree

---

## PHASE 6: CREATE BPA_EXPLODE ACTIVITY

### **1) Create Activity Blueprint

#### 1.1) Navigate to Activities Folder
   - 1.1.1) Content Browser -> /Game/AI/Activities/

#### 1.2) Create Blueprint
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Blueprint Class
   - 1.2.3) Search parent: NPCActivity
   - 1.2.4) Select NPCActivity
   - 1.2.5) Name: BPA_Explode
   - 1.2.6) Double-click to open

### **2) Configure Class Defaults

#### 2.1) Click Class Defaults

#### 2.2) Set Activity Properties
   - 2.2.1) Activity Name: Explode
   - 2.2.2) Behaviour Tree: BT_Explode
   - 2.2.3) Supported Goal Type: Goal_Attack
   - 2.2.4) Is Interruptable: Uncheck

### **3) Override SetupBlackboard Function

#### 3.1) Create Override
   - 3.1.1) In My Blueprint panel, find Functions
   - 3.1.2) Click Override dropdown
   - 3.1.3) Select Setup Blackboard

#### 3.2) Implement Setup Logic
   - 3.2.1) From Activity Goal input:
   - 3.2.1.1) Add Cast To Goal_Attack node
   - 3.2.2) From Cast successful execution:
   - 3.2.2.1) Add Get Target Actor node
   - 3.2.2.2) Target: As Goal Attack
   - 3.2.3) From Get Target Actor Return Value:
   - 3.2.3.1) Add Is Valid node
   - 3.2.4) From Is Valid Is Not Valid pin:
   - 3.2.4.1) Add Return Node
   - 3.2.4.2) Return Value: Uncheck (false)
   - 3.2.5) From Is Valid Is Valid pin:
   - 3.2.5.1) Add Set Value as Object node
   - 3.2.5.2) Target: BB (Blackboard parameter)
   - 3.2.5.3) Key Name: AttackTarget
   - 3.2.5.4) Object Value: Target Actor
   - 3.2.6) From Set Value as Object execution:
   - 3.2.6.1) Add Get Actor Location node
   - 3.2.6.2) Target: Target Actor
   - 3.2.7) From Get Actor Location execution:
   - 3.2.7.1) Add Set Value as Vector node
   - 3.2.7.2) Target: BB
   - 3.2.7.3) Key Name: TargetLocation
   - 3.2.7.4) Vector Value: Actor Location

#### 3.3) Trigger Alert on NPC
   - 3.3.1) From Set Value as Vector execution:
   - 3.3.1.1) Add Get Owner Character node
   - 3.3.2) From Get Owner Character Return Value:
   - 3.3.2.1) Add Cast To BP_PossessedExploder node
   - 3.3.3) From Cast successful execution:
   - 3.3.3.1) Add Trigger Alert node
   - 3.3.3.2) Target: As BP Possessed Exploder
   - 3.3.4) From Trigger Alert execution:
   - 3.3.4.1) Add Return Node
   - 3.3.4.2) Return Value: Check (true)

### **4) Override ScoreGoalItem Function

#### 4.1) Create Override
   - 4.1.1) In My Blueprint panel, find Functions
   - 4.1.2) Click Override dropdown
   - 4.1.3) Select Score Goal Item

#### 4.2) Implement Scoring
   - 4.2.1) From Goal Item input:
   - 4.2.1.1) Add Cast To Goal_Attack node
   - 4.2.2) From Cast successful execution:
   - 4.2.2.1) Add Return Node
   - 4.2.2.2) Return Value: 10.0 (high priority - exploder should pursue immediately)
   - 4.2.3) From Cast failed execution:
   - 4.2.3.1) Add Return Node
   - 4.2.3.2) Return Value: 0.0

### **5) Compile and Save

---

## PHASE 7: CREATE ABILITY CONFIGURATION

### **1) Create AbilityConfiguration Asset

#### 1.1) Navigate to Configurations Folder
   - 1.1.1) Content Browser -> /Game/Enemies/Possessed/Configurations/

#### 1.2) Create Data Asset
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Narrative
   - 1.2.3) Select Ability Configuration
   - 1.2.4) Name: AC_PossessedExploder
   - 1.2.5) Double-click to open

### **2) Configure Properties

| Property | Value |
|----------|-------|
| Default Attributes | GE_ExploderAttributes |

### **3) Configure AI Perception

#### 3.1) Expand AI Perception
   - 3.1.1) Find AI Perception section

#### 3.2) Add Sight Sense
   - 3.2.1) Click + on Senses Config
   - 3.2.2) Select AISense_Sight
   - 3.2.3) Sight Radius: 1500.0
   - 3.2.4) Lose Sight Radius: 2000.0
   - 3.2.5) Peripheral Vision Half Angle Degrees: 90.0

#### 3.3) Add Hearing Sense
   - 3.3.1) Click + on Senses Config
   - 3.3.2) Select AISense_Hearing
   - 3.3.3) Hearing Range: 1000.0

### **4) Save Asset

---

## PHASE 8: CREATE ACTIVITY CONFIGURATION

### **1) Create ActivityConfiguration Asset

#### 1.1) Create Data Asset
   - 1.1.1) Right-click in Configurations folder
   - 1.1.2) Select Narrative
   - 1.1.3) Select NPC Activity Configuration
   - 1.1.4) Name: ActConfig_PossessedExploder
   - 1.1.5) Double-click to open

### **2) Configure Default Activities

| Index | Activity | Purpose |
|-------|----------|---------|
| 0 | BPA_Explode | Pursuit and explosion behavior |
| 1 | BPA_Idle | Default idle hovering |

### **3) Configure Goal Generators

| Index | Generator | Purpose |
|-------|-----------|---------|
| 0 | GoalGenerator_Attack | Creates Goal_Attack from perception |

### **4) Set Rescore Interval
   - 4.1) Rescore Interval: 0.5

### **5) Save Asset

---

## PHASE 9: CREATE NPCDEFINITION

### **1) Create NPCDefinition Asset

#### 1.1) Navigate to Definitions Folder
   - 1.1.1) Content Browser -> /Game/Enemies/Possessed/Definitions/

#### 1.2) Create Data Asset
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Narrative
   - 1.2.3) Select NPC Definition
   - 1.2.4) Name: NPCDef_PossessedExploder
   - 1.2.5) Double-click to open

### **2) Configure NPC Properties

| Property | Value |
|----------|-------|
| NPC Name | Possessed Exploder |
| NPC Class Path | BP_PossessedExploder |
| Ability Configuration | AC_PossessedExploder |
| Activity Configuration | ActConfig_PossessedExploder |

### **3) Configure Factions

#### 3.1) Find Factions Array
   - 3.1.1) Click + to add element
   - 3.1.2) Add tag: Faction.Enemy.Possessed

### **4) Configure Default Owned Tags

#### 4.1) Find Default Owned Tags Array
   - 4.1.1) Click + to add element
   - 4.1.2) Add tag: State.Enemy.PossessedExploder

### **5) Save Asset

---

## PHASE 10: CONFIGURE FACTION ATTITUDES

### **1) Open Faction Settings

#### 1.1) Navigate to Settings
   - 1.1.1) Edit -> Project Settings
   - 1.1.2) Navigate to Game -> Arsenal Settings
   - 1.1.3) Find Faction Attitudes section

### **2) Add Hostile Relationship

#### 2.1) Configure Attitude Entry
   - 2.1.1) Click + to add entry
   - 2.1.2) Faction A: Faction.Enemy.Possessed
   - 2.1.3) Faction B: Narrative.Factions.Heroes
   - 2.1.4) Attitude: Hostile

### **3) Save Settings

---

## PHASE 11: WORLD PLACEMENT

### **1) Place NPCDefinition in Level

#### 1.1) Drag and Drop
   - 1.1.1) In Content Browser, find NPCDef_PossessedExploder
   - 1.1.2) Drag into level viewport
   - 1.1.3) NPCSpawner actor is automatically created

### **2) Configure Spawner

#### 2.1) Select NPCSpawner in Level
   - 2.1.1) Click on spawner in viewport

#### 2.2) Set Spawner Properties
   - 2.2.1) Auto Activate: Check (true)
   - 2.2.2) Spawn Interval: 0.0 (single spawn)
   - 2.2.3) Max Spawns: 1

### **3) Position Spawner
   - 3.1) Move to desired patrol location
   - 3.2) Adjust height for floating hover position

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 2.1 | January 2026 | Fixed SetByCaller tag: Changed Data.Damage to SetByCaller.Damage per Narrative Pro standard (NarrativeGameplayTags.cpp). |
| 2.0 | January 2026 | Streamlined to use existing Narrative Pro systems: BB_Attack blackboard, Goal_Attack goal, GoalGenerator_Attack generator. Reduced custom assets from 9 to 6. Simplified BT structure. |
| 1.0 | January 2026 | Initial implementation with custom blackboard and activity. |

---

**END OF POSSESSED EXPLODER ENEMY IMPLEMENTATION GUIDE v2.1**

**Unreal Engine 5.6 + Narrative Pro v2.2**

**Blueprint-Only Implementation**
