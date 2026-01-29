# Guard Formation Follow System Implementation Guide

## VERSION 2.7

## Unreal Engine 5.7 + Narrative Pro v2.2

## Blueprint-Only Implementation

---

## TABLE OF CONTENTS

1. [Introduction](#introduction)
2. [Quick Reference](#quick-reference)
3. [Phase 1: Gameplay Tags Setup](#phase-1-gameplay-tags-setup)
4. [Phase 2: Create Formation Blackboard](#phase-2-create-formation-blackboard)
5. [Phase 3: Create Goal_FormationFollow](#phase-3-create-goal_formationfollow)
6. [Phase 4: Create BTS_CalculateFormationPosition Service](#phase-4-create-bts_calculateformationposition-service)
7. [Phase 5: Create BTS_AdjustFormationSpeed Service](#phase-5-create-bts_adjustformationspeed-service)
8. [Phase 6: Create BT_FormationFollow Behavior Tree](#phase-6-create-bt_formationfollow-behavior-tree)
9. [Phase 7: Create BPA_FormationFollow Activity](#phase-7-create-bpa_formationfollow-activity)
10. [Phase 8: Create BPA_Attack_Formation Activity](#phase-8-create-bpa_attack_formation-activity)
11. [Phase 9: Create AC_FormationGuardBehavior ActivityConfiguration](#phase-9-create-ac_guard_formation-activityconfiguration)
12. [Phase 10: Spawn Guards with Formation Goals](#phase-10-spawn-guards-with-formation-goals)
13. [Changelog](#changelog)

---

## INTRODUCTION

### System Overview

The Guard Formation Follow System extends Narrative Pro's native Goal_FollowCharacter to enable multiple NPCs to follow a target in configurable formations. Each guard maintains a unique offset position relative to the target's location and rotation.

### Key Features

| Feature | Description |
|---------|-------------|
| Formation Positions | Guards maintain relative positions around target |
| Dynamic Rotation | Formation rotates with target facing direction |
| Speed Matching | Guards sprint to catch up, walk when close |
| Configurable Offsets | Per-guard position offsets via Vector |
| Save/Load Support | Inherits CharacterDefinition persistence |
| Vehicle Mounting | Optional follow-into-vehicle behavior |
| Leash Attack System | Guards attack nearby enemies without leaving formation |
| Dynamic Leash Radius | Attack range follows formation position as target moves |

### Square Formation Layout

| Guard Index | Local Offset (X, Y, Z) | Position Description |
|-------------|------------------------|----------------------|
| 0 | (200, 200, 0) | Front-Right |
| 1 | (200, -200, 0) | Front-Left |
| 2 | (-200, 200, 0) | Back-Right |
| 3 | (-200, -200, 0) | Back-Left |

### Architecture Overview

| Component | Parent Class | Purpose |
|-----------|--------------|---------|
| Goal_FormationFollow | Goal_FollowCharacter | Stores formation offset per guard |
| BB_FormationFollow | None (new) | Adds TargetLocation vector key |
| BTS_CalculateFormationPosition | BTService_BlueprintBase | Calculates world position each tick |
| BT_FormationFollow | None (new) | Uses TargetLocation for MoveTo |
| BPA_FormationFollow | NPCActivity | Runs formation behavior tree |
| BPA_Attack_Formation | BPA_Attack_Melee | Attack with leash radius check |
| GE_FormationGuardAttributes | GameplayEffect | Guard base stats (150 HP, 20 ATK, 15 Armor) |
| AC_FormationGuard | NPCAbilityConfiguration | Guard abilities + startup_effects |
| AC_FormationGuardBehavior | NPCActivityConfiguration | Guard-specific activity config |

### Leash System Overview

| Condition | Guard Behavior |
|-----------|----------------|
| Enemy within LeashRadius of TargetLocation | Attack normally |
| Enemy outside LeashRadius of TargetLocation | Ignore enemy, stay in formation |
| Target moves, TargetLocation updates | Leash center follows dynamically |
| Target retreats | Guards disengage distant enemies |

---

## QUICK REFERENCE

### Assets to Create

| Asset Name | Type | Location |
|------------|------|----------|
| Goal_FormationFollow | Blueprint Class | Content/AI/Goals/ |
| BB_FormationFollow | Blackboard | Content/AI/Blackboards/ |
| BTS_CalculateFormationPosition | Blueprint Class | Content/AI/Services/ |
| BTS_AdjustFormationSpeed | Blueprint Class | Content/AI/Services/ |
| BT_FormationFollow | Behavior Tree | Content/AI/BehaviorTrees/ |
| BPA_FormationFollow | Blueprint Class | Content/AI/Activities/ |
| BPA_Attack_Formation | Blueprint Class | Content/AI/Activities/ |
| GE_FormationGuardAttributes | Gameplay Effect | Content/Enemies/Formation/Effects/ |
| AC_FormationGuard | Data Asset | Content/AI/Configurations/ |
| AC_FormationGuardBehavior | Data Asset | Content/AI/Configurations/ |

### Leash Radius Reference

| Guard Type | LeashRadius | Behavior |
|------------|-------------|----------|
| Close Protection | 300.0 | Tight formation, minimal straying |
| Standard Guard | 400.0 | Default balanced protection |
| Perimeter Guard | 500.0 | Can engage threats further out |
| Ranged Guard | 200.0 | Nearly stationary, shoots from position |

### Blackboard Keys

| Key Name | Type | Purpose |
|----------|------|---------|
| SelfActor | Object (Actor) | Reference to owning NPC |
| FollowTarget | Object (Actor) | Character to follow |
| FormationOffset | Vector | Local space offset from target |
| TargetLocation | Vector | Calculated world position |

### Formation Offset Values

| Guard Index | FormationOffset Value | Formation Position |
|-------------|----------------------|-------------------|
| 0 | X=200, Y=200, Z=0 | Front-Right |
| 1 | X=200, Y=-200, Z=0 | Front-Left |
| 2 | X=-200, Y=200, Z=0 | Back-Right |
| 3 | X=-200, Y=-200, Z=0 | Back-Left |

---

## PHASE 1: GAMEPLAY TAGS SETUP

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Narrative.State.NPC.Activity.FormationFollow | NPC is following in formation |

---

## PHASE 2: CREATE FORMATION BLACKBOARD

### 1) Create Blackboard Asset

#### 1.1) Navigate to AI Folder
   - 1.1.1) In Content Browser, navigate to Content/AI/Blackboards/
   - 1.1.2) If folder does not exist, right-click and select New Folder
   - 1.1.3) Name folder: Blackboards

#### 1.2) Create New Blackboard
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Artificial Intelligence
   - 1.2.3) Select Blackboard
   - 1.2.4) Name it: BB_FormationFollow

### 2) Open Blackboard Editor

#### 2.1) Open Asset
   - 2.1.1) Double-click BB_FormationFollow to open

### 3) Add Blackboard Keys

#### 3.1) Add SelfActor Key
   - 3.1.1) Click New Key button in Blackboard panel
   - 3.1.2) Select Object from dropdown
   - 3.1.3) Name it: SelfActor
   - 3.1.4) In Details panel, set Base Class to Actor

#### 3.2) Add FollowTarget Key
   - 3.2.1) Click New Key button
   - 3.2.2) Select Object from dropdown
   - 3.2.3) Name it: FollowTarget
   - 3.2.4) In Details panel, set Base Class to Actor

#### 3.3) Add FormationOffset Key
   - 3.3.1) Click New Key button
   - 3.3.2) Select Vector from dropdown
   - 3.3.3) Name it: FormationOffset

#### 3.4) Add TargetLocation Key
   - 3.4.1) Click New Key button
   - 3.4.2) Select Vector from dropdown
   - 3.4.3) Name it: TargetLocation

### 4) Verify Keys

#### 4.1) Confirm All Keys Present
   - 4.1.1) Blackboard should show 4 keys:

| Key Name | Type | Color |
|----------|------|-------|
| SelfActor | Object | Blue |
| FollowTarget | Object | Blue |
| FormationOffset | Vector | Yellow |
| TargetLocation | Vector | Yellow |

### 5) Save Blackboard

#### 5.1) Save Asset
   - 5.1.1) Press Ctrl+S or click Save button
   - 5.1.2) Close Blackboard editor

---

## PHASE 3: CREATE GOAL_FORMATIONFOLLOW

### 1) Create Goal Blueprint

#### 1.1) Navigate to Goals Folder
   - 1.1.1) In Content Browser, navigate to Content/AI/Goals/
   - 1.1.2) If folder does not exist, create it

#### 1.2) Create Child Blueprint
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Blueprint Class
   - 1.2.3) In search field, type: Goal_FollowCharacter
   - 1.2.4) Select Goal_FollowCharacter as parent class
   - 1.2.5) Click Select
   - 1.2.6) Name it: Goal_FormationFollow

### 2) Open Blueprint Editor

#### 2.1) Open Asset
   - 2.1.1) Double-click Goal_FormationFollow to open

### 3) Add Formation Variables

#### 3.1) Add FormationOffset Variable
   - 3.1.1) In My Blueprint panel, click + button next to Variables
   - 3.1.2) Name it: FormationOffset
   - 3.1.3) In Details panel, set Variable Type to Vector
   - 3.1.4) Check Instance Editable checkbox
   - 3.1.5) Check Expose on Spawn checkbox
   - 3.1.6) Set Category to: Formation
   - 3.1.7) Set Default Value to: X=0, Y=0, Z=0

#### 3.2) Add FormationIndex Variable
   - 3.2.1) Click + button next to Variables
   - 3.2.2) Name it: FormationIndex
   - 3.2.3) In Details panel, set Variable Type to Integer
   - 3.2.4) Check Instance Editable checkbox
   - 3.2.5) Check Expose on Spawn checkbox
   - 3.2.6) Set Category to: Formation
   - 3.2.7) Set Default Value to: 0

### 4) Configure Class Defaults

#### 4.1) Open Class Defaults
   - 4.1.1) Click Class Defaults button in toolbar

#### 4.2) Verify Inherited Properties
   - 4.2.1) Confirm these properties exist from parent:

| Property | Category | Inherited From |
|----------|----------|----------------|
| Target to Follow Asset | Config | Goal_FollowCharacter |
| Should Mount with Target | Config | Goal_FollowCharacter |
| Default Score | NPC Goal | NPCGoalItem |
| Goal Lifetime | NPC Goal | NPCGoalItem |

#### 4.3) Set Default Values
   - 4.3.1) Default Score: 1.0
   - 4.3.2) Goal Lifetime: -1.0

### 5) Compile and Save

#### 5.1) Compile Blueprint
   - 5.1.1) Click Compile button in toolbar
   - 5.1.2) Verify no errors in Compiler Results

#### 5.2) Save Blueprint
   - 5.2.1) Press Ctrl+S or click Save button

---

## PHASE 4: CREATE BTS_CALCULATEFORMATIONPOSITION SERVICE

### 1) Create Service Blueprint

#### 1.1) Navigate to Services Folder
   - 1.1.1) In Content Browser, navigate to Content/AI/Services/
   - 1.1.2) If folder does not exist, create it

#### 1.2) Create New Blueprint
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Blueprint Class
   - 1.2.3) In All Classes section, search for: BTService_BlueprintBase
   - 1.2.4) Select BTService_BlueprintBase as parent
   - 1.2.5) Click Select
   - 1.2.6) Name it: BTS_CalculateFormationPosition

### 2) Open Blueprint Editor

#### 2.1) Open Asset
   - 2.1.1) Double-click BTS_CalculateFormationPosition to open

### 3) Add Service Variables

#### 3.1) Add BBKey_FollowTarget Variable
   - 3.1.1) In My Blueprint panel, click + button next to Variables
   - 3.1.2) Name it: BBKey_FollowTarget
   - 3.1.3) Set Variable Type to Name
   - 3.1.4) Set Default Value to: FollowTarget

#### 3.2) Add BBKey_FormationOffset Variable
   - 3.2.1) Click + button next to Variables
   - 3.2.2) Name it: BBKey_FormationOffset
   - 3.2.3) Set Variable Type to Name
   - 3.2.4) Set Default Value to: FormationOffset

#### 3.3) Add BBKey_TargetLocation Variable
   - 3.3.1) Click + button next to Variables
   - 3.3.2) Name it: BBKey_TargetLocation
   - 3.3.3) Set Variable Type to Name
   - 3.3.4) Set Default Value to: TargetLocation

### 4) Implement Event Receive Tick AI

#### 4.1) Create Event Node
   - 4.1.1) In Event Graph, right-click
   - 4.1.2) Search for: Event Receive Tick AI
   - 4.1.3) Select Event Receive Tick AI

#### 4.2) Get Blackboard Component
   - 4.2.1) From Owner Controller pin, drag and release
   - 4.2.2) Search for: Get Blackboard Component
   - 4.2.3) Select Get Blackboard Component

#### 4.3) Get Follow Target from Blackboard
   - 4.3.1) From Get Blackboard Component Return Value, drag and release
   - 4.3.2) Search for: Get Value as Object
   - 4.3.3) Select Get Value as Object
   - 4.3.4) GET BBKey_FollowTarget variable
   - 4.3.5) Connect to Key Name pin

#### 4.4) Cast to Actor
   - 4.4.1) From Get Value as Object Return Value, drag and release
   - 4.4.2) Search for: Cast To Actor
   - 4.4.3) Select Cast To Actor
   - 4.4.4) Connect execution pin from Event Receive Tick AI to Cast To Actor

#### 4.5) Validate Follow Target
   - 4.5.1) From As Actor pin, drag and release
   - 4.5.2) Search for: Is Valid
   - 4.5.3) Select Is Valid (the one with Exec, Is Valid, Is Not Valid pins)
   - 4.5.4) Connect Cast To Actor execution to Is Valid Exec pin

#### 4.6) Get Follow Target Location
   - 4.6.1) From Is Valid execution pin, continue
   - 4.6.2) From As Actor pin, drag and release
   - 4.6.3) Search for: Get Actor Location
   - 4.6.4) Select GetActorLocation

#### 4.7) Get Follow Target Rotation
   - 4.7.1) From As Actor pin, drag and release
   - 4.7.2) Search for: Get Actor Rotation
   - 4.7.3) Select GetActorRotation

#### 4.8) Get Formation Offset from Blackboard
   - 4.8.1) From Get Blackboard Component Return Value (reuse from earlier)
   - 4.8.2) Drag and release, search for: Get Value as Vector
   - 4.8.3) Select Get Value as Vector
   - 4.8.4) GET BBKey_FormationOffset variable
   - 4.8.5) Connect to Key Name pin

#### 4.9) Rotate Offset by Target Rotation
   - 4.9.1) From Get Actor Rotation Return Value, drag and release
   - 4.9.2) Search for: Rotate Vector
   - 4.9.3) Select RotateVector (Rotator)
   - 4.9.4) Connect Get Value as Vector Return Value to A pin (vector input)

#### 4.10) Calculate World Position
   - 4.10.1) Right-click, search for: Add (Vector + Vector)
   - 4.10.2) Select Vector + Vector node
   - 4.10.3) Connect Get Actor Location Return Value to first input
   - 4.10.4) Connect Rotate Vector Return Value to second input

#### 4.11) Set Target Location on Blackboard
   - 4.11.1) From Get Blackboard Component Return Value, drag and release
   - 4.11.2) Search for: Set Value as Vector
   - 4.11.3) Select Set Value as Vector
   - 4.11.4) Connect execution from Is Valid execution pin (after calculations)
   - 4.11.5) GET BBKey_TargetLocation variable
   - 4.11.6) Connect to Key Name pin
   - 4.11.7) Connect Vector + Vector Return Value to Vector Value pin

### 5) Node Connection Summary

| Step | From Node | From Pin | To Node | To Pin |
|------|-----------|----------|---------|--------|
| 1 | Event Receive Tick AI | Exec | Cast To Actor | Exec |
| 2 | Event Receive Tick AI | Owner Controller | Get Blackboard Component | Target |
| 3 | Get Blackboard Component | Return Value | Get Value as Object | Target |
| 4 | BBKey_FollowTarget | Value | Get Value as Object | Key Name |
| 5 | Get Value as Object | Return Value | Cast To Actor | Object |
| 6 | Cast To Actor | Exec | Is Valid | Exec |
| 7 | Cast To Actor | As Actor | Is Valid | Input Object |
| 8 | Is Valid | Is Valid | Set Value as Vector | Exec |
| 9 | Cast To Actor | As Actor | Get Actor Location | Target |
| 10 | Cast To Actor | As Actor | Get Actor Rotation | Target |
| 11 | Get Blackboard Component | Return Value | Get Value as Vector | Target |
| 12 | BBKey_FormationOffset | Value | Get Value as Vector | Key Name |
| 13 | Get Actor Rotation | Return Value | Rotate Vector | B (Rotator) |
| 14 | Get Value as Vector | Return Value | Rotate Vector | A (Vector) |
| 15 | Get Actor Location | Return Value | Vector + Vector | First Input |
| 16 | Rotate Vector | Return Value | Vector + Vector | Second Input |
| 17 | Get Blackboard Component | Return Value | Set Value as Vector | Target |
| 18 | BBKey_TargetLocation | Value | Set Value as Vector | Key Name |
| 19 | Vector + Vector | Return Value | Set Value as Vector | Vector Value |

### 6) Configure Service Settings

#### 6.1) Open Class Defaults
   - 6.1.1) Click Class Defaults button

#### 6.2) Set Service Properties
   - 6.2.1) In Details panel, find Service category
   - 6.2.2) Set Interval to: 0.5
   - 6.2.3) Set Random Deviation to: 0.1

### 7) Compile and Save

#### 7.1) Compile Blueprint
   - 7.1.1) Click Compile button
   - 7.1.2) Verify no errors

#### 7.2) Save Blueprint
   - 7.2.1) Press Ctrl+S

---

## PHASE 5: CREATE BTS_ADJUSTFORMATIONSPEED SERVICE

### 1) Create Service Blueprint

#### 1.1) Navigate to Services Folder
   - 1.1.1) In Content Browser, navigate to Content/AI/Services/

#### 1.2) Create New Blueprint
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Blueprint Class
   - 1.2.3) In All Classes section, search for: BTService_BlueprintBase
   - 1.2.4) Select BTService_BlueprintBase as parent
   - 1.2.5) Click Select
   - 1.2.6) Name it: BTS_AdjustFormationSpeed

### 2) Open Blueprint Editor

#### 2.1) Open Asset
   - 2.1.1) Double-click BTS_AdjustFormationSpeed to open

### 3) Add Service Variables

#### 3.1) Add BBKey_TargetLocation Variable
   - 3.1.1) In My Blueprint panel, click + button next to Variables
   - 3.1.2) Name it: BBKey_TargetLocation
   - 3.1.3) Set Variable Type to Name
   - 3.1.4) Set Default Value to: TargetLocation

#### 3.2) Add BBKey_FollowTarget Variable
   - 3.2.1) Click + button next to Variables
   - 3.2.2) Name it: BBKey_FollowTarget
   - 3.2.3) Set Variable Type to Name
   - 3.2.4) Set Default Value to: FollowTarget

#### 3.3) Add SprintThreshold Variable
   - 3.3.1) Click + button next to Variables
   - 3.3.2) Name it: SprintThreshold
   - 3.3.3) Set Variable Type to Float
   - 3.3.4) Set Default Value to: 300.0
   - 3.3.5) Check Instance Editable checkbox
   - 3.3.6) Set Category to: Formation Settings

#### 3.4) Add SprintMultiplier Variable
   - 3.4.1) Click + button next to Variables
   - 3.4.2) Name it: SprintMultiplier
   - 3.4.3) Set Variable Type to Float
   - 3.4.4) Set Default Value to: 1.2
   - 3.4.5) Check Instance Editable checkbox
   - 3.4.6) Set Category to: Formation Settings

### 4) Implement Event Receive Tick AI

#### 4.1) Create Event Node
   - 4.1.1) In Event Graph, right-click
   - 4.1.2) Search for: Event Receive Tick AI
   - 4.1.3) Select Event Receive Tick AI

#### 4.2) Get Blackboard Component
   - 4.2.1) From Owner Controller pin, drag and release
   - 4.2.2) Search for: Get Blackboard Component
   - 4.2.3) Select Get Blackboard Component

#### 4.3) Get Guard Pawn and Location
   - 4.3.1) From Owner Controller pin, drag and release
   - 4.3.2) Search for: Get Controlled Pawn
   - 4.3.3) Select Get Controlled Pawn
   - 4.3.4) From Get Controlled Pawn Return Value, drag and release
   - 4.3.5) Search for: Get Actor Location
   - 4.3.6) Select GetActorLocation

#### 4.4) Get Target Location from Blackboard
   - 4.4.1) From Get Blackboard Component Return Value, drag and release
   - 4.4.2) Search for: Get Value as Vector
   - 4.4.3) Select Get Value as Vector
   - 4.4.4) GET BBKey_TargetLocation variable
   - 4.4.5) Connect to Key Name pin

#### 4.5) Calculate Distance to Formation Position
   - 4.5.1) Right-click, search for: Distance (Vector)
   - 4.5.2) Select Vector Distance
   - 4.5.3) Connect GetActorLocation Return Value to V1 pin
   - 4.5.4) Connect Get Value as Vector Return Value to V2 pin

#### 4.6) Compare Distance to Threshold
   - 4.6.1) From Vector Distance Return Value, drag and release
   - 4.6.2) Search for: > (float)
   - 4.6.3) Select float > float
   - 4.6.4) GET SprintThreshold variable
   - 4.6.5) Connect to second input pin

#### 4.7) Cast Guard to NarrativeNPCCharacter
   - 4.7.1) From Get Controlled Pawn Return Value, drag and release
   - 4.7.2) Search for: Cast To NarrativeNPCCharacter
   - 4.7.3) Select Cast To NarrativeNPCCharacter
   - 4.7.4) Connect Event Receive Tick AI execution to Cast Exec pin

#### 4.8) Get Guard Movement Component
   - 4.8.1) From As Narrative NPC Character pin, drag and release
   - 4.8.2) Search for: GetCharacterMovement
   - 4.8.3) Select GetCharacterMovement

#### 4.9) Branch on Distance Check
   - 4.9.1) Right-click, search for: Branch
   - 4.9.2) Select Branch
   - 4.9.3) Connect Cast To NarrativeNPCCharacter execution out to Branch Exec pin
   - 4.9.4) Connect float > float Return Value to Condition pin

#### 4.10) Get VIP from Blackboard
   - 4.10.1) From Get Blackboard Component Return Value, drag and release
   - 4.10.2) Search for: Get Value as Object
   - 4.10.3) Select Get Value as Object
   - 4.10.4) GET BBKey_FollowTarget variable
   - 4.10.5) Connect to Key Name pin

#### 4.11) Cast VIP to Character
   - 4.11.1) From Get Value as Object Return Value, drag and release
   - 4.11.2) Search for: Cast To Character
   - 4.11.3) Select Cast To Character

#### 4.12) Get VIP Movement Speed
   - 4.12.1) From As Character pin (VIP cast), drag and release
   - 4.12.2) Search for: GetCharacterMovement
   - 4.12.3) Select GetCharacterMovement
   - 4.12.4) From Return Value, drag and release
   - 4.12.5) Search for: Get Max Walk Speed
   - 4.12.6) Select Get Max Walk Speed

#### 4.13) Calculate Sprint Speed
   - 4.13.1) From Get Max Walk Speed Return Value (VIP), drag and release
   - 4.13.2) Search for: Multiply (float)
   - 4.13.3) Select float * float
   - 4.13.4) GET SprintMultiplier variable
   - 4.13.5) Connect to second input pin

#### 4.14) Set Sprint Speed (True Branch)
   - 4.14.1) From Branch True execution pin, continue
   - 4.14.2) From GetCharacterMovement Return Value (Guard), drag and release
   - 4.14.3) Search for: Set Max Walk Speed
   - 4.14.4) Select Set Max Walk Speed
   - 4.14.5) Connect float * float Return Value to Max Walk Speed pin

#### 4.15) Set Walk Speed (False Branch)
   - 4.15.1) From Branch False execution pin, continue
   - 4.15.2) From GetCharacterMovement Return Value (Guard), drag and release
   - 4.15.3) Search for: Set Max Walk Speed
   - 4.15.4) Select Set Max Walk Speed
   - 4.15.5) Connect Get Max Walk Speed Return Value (VIP) to Max Walk Speed pin

### 5) Node Connection Summary

| Step | From Node | From Pin | To Node | To Pin |
|------|-----------|----------|---------|--------|
| 1 | Event Receive Tick AI | Exec | Cast To NarrativeNPCCharacter (Guard) | Exec |
| 2 | Cast To NarrativeNPCCharacter (Guard) | Exec | Branch | Exec |
| 3 | Event Receive Tick AI | Owner Controller | Get Blackboard Component | Target |
| 4 | Event Receive Tick AI | Owner Controller | Get Controlled Pawn | Target |
| 5 | Get Controlled Pawn | Return Value | GetActorLocation | Target |
| 6 | Get Controlled Pawn | Return Value | Cast To NarrativeNPCCharacter (Guard) | Object |
| 7 | As Narrative NPC Character (Guard) | Value | GetCharacterMovement (Guard) | Target |
| 8 | Get Blackboard Component | Return Value | Get Value as Vector | Target |
| 9 | Get Blackboard Component | Return Value | Get Value as Object | Target |
| 10 | BBKey_TargetLocation | Value | Get Value as Vector | Key Name |
| 11 | BBKey_FollowTarget | Value | Get Value as Object | Key Name |
| 12 | GetActorLocation | Return Value | Vector Distance | V1 |
| 13 | Get Value as Vector | Return Value | Vector Distance | V2 |
| 14 | Vector Distance | Return Value | float > float | First Input |
| 15 | SprintThreshold | Value | float > float | Second Input |
| 16 | float > float | Return Value | Branch | Condition |
| 17 | Get Value as Object | Return Value | Cast To Character (VIP) | Object |
| 18 | As Character (VIP) | Value | GetCharacterMovement (VIP) | Target |
| 19 | GetCharacterMovement (VIP) | Return Value | Get Max Walk Speed | Target |
| 20 | Get Max Walk Speed | Return Value | float * float | First Input |
| 21 | SprintMultiplier | Value | float * float | Second Input |
| 22 | Branch | True | Set Max Walk Speed (Sprint) | Exec |
| 23 | Branch | False | Set Max Walk Speed (Walk) | Exec |
| 24 | GetCharacterMovement (Guard) | Return Value | Set Max Walk Speed (Sprint) | Target |
| 25 | GetCharacterMovement (Guard) | Return Value | Set Max Walk Speed (Walk) | Target |
| 26 | float * float | Return Value | Set Max Walk Speed (Sprint) | Max Walk Speed |
| 27 | Get Max Walk Speed | Return Value | Set Max Walk Speed (Walk) | Max Walk Speed |

### 6) Configure Service Settings

#### 6.1) Open Class Defaults
   - 6.1.1) Click Class Defaults button

#### 6.2) Set Service Properties
   - 6.2.1) In Details panel, find Service category
   - 6.2.2) Set Interval to: 1.0
   - 6.2.3) Set Random Deviation to: 0.2

### 7) Compile and Save

#### 7.1) Compile Blueprint
   - 7.1.1) Click Compile button
   - 7.1.2) Verify no errors

#### 7.2) Save Blueprint
   - 7.2.1) Press Ctrl+S

---

## PHASE 6: CREATE BT_FORMATIONFOLLOW BEHAVIOR TREE

### 1) Create Behavior Tree Asset

#### 1.1) Navigate to BehaviorTrees Folder
   - 1.1.1) In Content Browser, navigate to Content/AI/BehaviorTrees/
   - 1.1.2) If folder does not exist, create it

#### 1.2) Create New Behavior Tree
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Artificial Intelligence
   - 1.2.3) Select Behavior Tree
   - 1.2.4) Name it: BT_FormationFollow

### 2) Configure Behavior Tree

#### 2.1) Open Behavior Tree
   - 2.1.1) Double-click BT_FormationFollow to open

#### 2.2) Assign Blackboard
   - 2.2.1) Click on ROOT node
   - 2.2.2) In Details panel, find Behavior Tree section
   - 2.2.3) Set Blackboard Asset to: BB_FormationFollow

### 3) Build Tree Structure

#### 3.1) Add Selector Node
   - 3.1.1) Drag from ROOT node downward
   - 3.1.2) Select Composites category
   - 3.1.3) Select Selector

#### 3.2) Add Blackboard Decorator to Selector
   - 3.2.1) Right-click on Selector node
   - 3.2.2) Select Add Decorator
   - 3.2.3) Search for: Blackboard Based Condition
   - 3.2.4) Select Blackboard Based Condition
   - 3.2.5) In Details panel:
   - 3.2.5.1) Set Key Query to: Is Set
   - 3.2.5.2) Set Blackboard Key to: FollowTarget
   - 3.2.5.3) Set Notify Observer to: On Result Change

#### 3.3) Add Gameplay Tag Decorator to Selector
   - 3.3.1) Right-click on Selector node
   - 3.3.2) Select Add Decorator
   - 3.3.3) Search for: Gameplay Tag Condition
   - 3.3.4) Select Gameplay Tag Condition
   - 3.3.5) In Details panel:
   - 3.3.5.1) Set Actor to Check to: SelfActor
   - 3.3.5.2) Set Tags to Match to: Any
   - 3.3.5.3) Add tag: Narrative.State.Movement.Falling
   - 3.3.5.4) Check Inverse Condition checkbox

#### 3.4) Add Sequence Node
   - 3.4.1) Drag from Selector node downward
   - 3.4.2) Select Composites category
   - 3.4.3) Select Sequence

#### 3.5) Add BTS_CalculateFormationPosition Service
   - 3.5.1) Right-click on Sequence node
   - 3.5.2) Select Add Service
   - 3.5.3) Search for: BTS_CalculateFormationPosition
   - 3.5.4) Select BTS_CalculateFormationPosition

#### 3.6) Add BTS_AdjustFormationSpeed Service
   - 3.6.1) Right-click on Sequence node
   - 3.6.2) Select Add Service
   - 3.6.3) Search for: BTS_AdjustFormationSpeed
   - 3.6.4) Select BTS_AdjustFormationSpeed

#### 3.7) Add Move To Task
   - 3.7.1) Drag from Sequence node downward
   - 3.7.2) Select Tasks category
   - 3.7.3) Select Move To
   - 3.7.4) Click on Move To task
   - 3.7.5) In Details panel:
   - 3.7.5.1) Set Blackboard Key to: TargetLocation
   - 3.7.5.2) Set Acceptable Radius to: 50.0
   - 3.7.5.3) Check Allow Strafe checkbox: Unchecked
   - 3.7.5.4) Check Reach Test Includes Agent Radius: Checked
   - 3.7.5.5) Check Reach Test Includes Goal Radius: Unchecked
   - 3.7.5.6) Check Track Moving Goal: Checked
   - 3.7.5.7) Check Require Navigable End Location: Checked
   - 3.7.5.8) Check Project Goal Location: Checked

### 4) Final Tree Structure

| Level | Node Type | Node Name | Services/Decorators |
|-------|-----------|-----------|---------------------|
| 0 | Root | ROOT | BB_FormationFollow |
| 1 | Composite | Selector | BB Condition, Tag Condition |
| 2 | Composite | Sequence | BTS_CalculateFormationPosition, BTS_AdjustFormationSpeed |
| 3 | Task | Move To | TargetLocation key |

### 5) Save Behavior Tree

#### 5.1) Save Asset
   - 5.1.1) Press Ctrl+S or click Save button

---

## PHASE 7: CREATE BPA_FORMATIONFOLLOW ACTIVITY

### 1) Create Activity Blueprint

#### 1.1) Navigate to Activities Folder
   - 1.1.1) In Content Browser, navigate to Content/AI/Activities/
   - 1.1.2) If folder does not exist, create it

#### 1.2) Create New Blueprint
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Blueprint Class
   - 1.2.3) In All Classes section, search for: NPCActivity
   - 1.2.4) Select NPCActivity as parent
   - 1.2.5) Click Select
   - 1.2.6) Name it: BPA_FormationFollow

### 2) Open Blueprint Editor

#### 2.1) Open Asset
   - 2.1.1) Double-click BPA_FormationFollow to open

### 3) Add Activity Variables

#### 3.1) Add FormationGoal Variable
   - 3.1.1) In My Blueprint panel, click + button next to Variables
   - 3.1.2) Name it: FormationGoal
   - 3.1.3) Set Variable Type to: Goal_FormationFollow (Object Reference)
   - 3.1.4) Set Category to: Formation

#### 3.2) Add BBKey_FollowTarget Variable
   - 3.2.1) Click + button next to Variables
   - 3.2.2) Name it: BBKey_FollowTarget
   - 3.2.3) Set Variable Type to: Name
   - 3.2.4) Set Default Value to: FollowTarget

#### 3.3) Add BBKey_FormationOffset Variable
   - 3.3.1) Click + button next to Variables
   - 3.3.2) Name it: BBKey_FormationOffset
   - 3.3.3) Set Variable Type to: Name
   - 3.3.4) Set Default Value to: FormationOffset

### 4) Configure Class Defaults

#### 4.1) Open Class Defaults
   - 4.1.1) Click Class Defaults button in toolbar

#### 4.2) Set Activity Properties
   - 4.2.1) In Details panel:
   - 4.2.2) Set Activity Name to: Formation Follow
   - 4.2.3) In Owned Tags, add: Narrative.State.NPC.Activity.FormationFollow
   - 4.2.4) Leave Block Tags empty
   - 4.2.5) Leave Require Tags empty

#### 4.3) Set NPC Activity Properties
   - 4.3.1) Set Behaviour Tree to: BT_FormationFollow
   - 4.3.2) Set Supported Goal Type to: Goal_FormationFollow
   - 4.3.3) Set Is Interruptable to: Unchecked
   - 4.3.4) Set Save Activity to: Unchecked

### 5) Override SetupBlackboard Function

#### 5.1) Create Override
   - 5.1.1) In My Blueprint panel, find Functions section
   - 5.1.2) Click Override dropdown
   - 5.1.3) Select SetupBlackboard

#### 5.2) Cast Activity Goal
   - 5.2.1) From BB input pin, note this is the Blackboard Component
   - 5.2.2) Drag from Activity Goal variable (inherited)
   - 5.2.3) From Activity Goal, drag and release
   - 5.2.4) Search for: Cast To Goal_FormationFollow
   - 5.2.5) Select Cast To Goal_FormationFollow
   - 5.2.6) Connect SetupBlackboard execution to Cast node

#### 5.3) Store Formation Goal
   - 5.3.1) From As Goal Formation Follow pin, drag and release
   - 5.3.2) Search for: SET
   - 5.3.3) Select SET FormationGoal
   - 5.3.4) Connect Cast execution to SET node

#### 5.4) Get Target to Follow
   - 5.4.1) From FormationGoal, drag and release
   - 5.4.2) Search for: Get Target to Follow
   - 5.4.3) Select Get Target to Follow (inherited from Goal_FollowCharacter)

#### 5.5) Validate Target
   - 5.5.1) From Get Target to Follow Return Value, drag and release
   - 5.5.2) Search for: Is Valid
   - 5.5.3) Select Is Valid (the one with Exec, Is Valid, Is Not Valid pins)
   - 5.5.4) Connect SET FormationGoal execution to Is Valid Exec pin

#### 5.6) Create Find Character Data Chain (Pure Nodes)
   - 5.6.1) Right-click in empty graph space
   - 5.6.2) Search for: Get Subsystem
   - 5.6.3) Select Get Subsystem (Narrative Character Subsystem)
   - 5.6.4) From Return Value, drag and release
   - 5.6.5) Search for: Find Character
   - 5.6.6) Select Find Character
   - 5.6.7) From FormationGoal, drag and release
   - 5.6.8) Search for: Get Target to Follow Asset
   - 5.6.9) Select Get Target to Follow Asset
   - 5.6.10) Connect Return Value to Find Character Character Definition pin

#### 5.7) Is Valid Path - Set FollowTarget with Target to Follow
   - 5.7.1) From BB input pin, drag and release
   - 5.7.2) Search for: Set Value as Object
   - 5.7.3) Select Set Value as Object
   - 5.7.4) Connect Is Valid execution pin to Set Value as Object exec pin
   - 5.7.5) BB is already connected to Target pin
   - 5.7.6) GET BBKey_FollowTarget variable
   - 5.7.7) Connect to Key Name pin
   - 5.7.8) From FormationGoal, drag and release
   - 5.7.9) Search for: Get Target to Follow
   - 5.7.10) Connect Return Value to Object Value pin

#### 5.8) Is Not Valid Path - Set FollowTarget with Find Character Result
   - 5.8.1) From BB input pin, drag and release
   - 5.8.2) Search for: Set Value as Object
   - 5.8.3) Select Set Value as Object
   - 5.8.4) Connect Is Not Valid execution pin to Set Value as Object exec pin
   - 5.8.5) BB is already connected to Target pin
   - 5.8.6) GET BBKey_FollowTarget variable
   - 5.8.7) Connect to Key Name pin
   - 5.8.8) Connect Find Character Return Value to Object Value pin

#### 5.9) Is Valid Path - Set FormationOffset on Blackboard
   - 5.9.1) From BB input pin, drag and release
   - 5.9.2) Search for: Set Value as Vector
   - 5.9.3) Select Set Value as Vector
   - 5.9.4) Connect first Set Value as Object (Valid path) exec output to this Set Value as Vector exec pin
   - 5.9.5) BB is already connected to Target pin
   - 5.9.6) GET BBKey_FormationOffset variable
   - 5.9.7) Connect to Key Name pin
   - 5.9.8) From FormationGoal, drag and release
   - 5.9.9) Search for: Get Formation Offset
   - 5.9.10) Connect Return Value to Vector Value pin

#### 5.10) Is Not Valid Path - Set FormationOffset on Blackboard
   - 5.10.1) From BB input pin, drag and release
   - 5.10.2) Search for: Set Value as Vector
   - 5.10.3) Select Set Value as Vector
   - 5.10.4) Connect second Set Value as Object (Invalid path) exec output to this Set Value as Vector exec pin
   - 5.10.5) BB is already connected to Target pin
   - 5.10.6) GET BBKey_FormationOffset variable
   - 5.10.7) Connect to Key Name pin
   - 5.10.8) Connect same Formation Offset Return Value to Vector Value pin

#### 5.11) Return True
   - 5.11.1) Connect first Set Value as Vector (Valid path) exec output to Return Node exec pin
   - 5.11.2) Connect second Set Value as Vector (Invalid path) exec output to Return Node exec pin
   - 5.11.3) Set Return Value to: True (checked)

### 6) SetupBlackboard Node Connection Summary

| Step | From Node | From Pin | To Node | To Pin |
|------|-----------|----------|---------|--------|
| 1 | SetupBlackboard | Exec | Cast To Goal_FormationFollow | Exec |
| 2 | Activity Goal | Value | Cast To Goal_FormationFollow | Object |
| 3 | Cast To Goal_FormationFollow | Exec | SET FormationGoal | Exec |
| 4 | As Goal Formation Follow | Value | SET FormationGoal | Value |
| 5 | SET FormationGoal | Exec | Is Valid | Exec |
| 6 | FormationGoal | Target to Follow | Is Valid | Input Object |
| 7 | Narrative Character Subsystem | Return Value | Find Character | Target |
| 8 | Get Target to Follow Asset | Return Value | Find Character | Character Definition |
| 9 | Is Valid | Is Valid | Set Value as Object (Valid) | Exec |
| 10 | BB | Value | Set Value as Object (Valid) | Target |
| 11 | BBKey_FollowTarget | Value | Set Value as Object (Valid) | Key Name |
| 12 | Get Target to Follow | Return Value | Set Value as Object (Valid) | Object Value |
| 13 | Is Valid | Is Not Valid | Set Value as Object (Invalid) | Exec |
| 14 | BB | Value | Set Value as Object (Invalid) | Target |
| 15 | BBKey_FollowTarget | Value | Set Value as Object (Invalid) | Key Name |
| 16 | Find Character | Return Value | Set Value as Object (Invalid) | Object Value |
| 17 | Set Value as Object (Valid) | Exec | Set Value as Vector (Valid) | Exec |
| 18 | BB | Value | Set Value as Vector (Valid) | Target |
| 19 | BBKey_FormationOffset | Value | Set Value as Vector (Valid) | Key Name |
| 20 | Get Formation Offset | Return Value | Set Value as Vector (Valid) | Vector Value |
| 21 | Set Value as Object (Invalid) | Exec | Set Value as Vector (Invalid) | Exec |
| 22 | BB | Value | Set Value as Vector (Invalid) | Target |
| 23 | BBKey_FormationOffset | Value | Set Value as Vector (Invalid) | Key Name |
| 24 | Get Formation Offset | Return Value | Set Value as Vector (Invalid) | Vector Value |
| 25 | Set Value as Vector (Valid) | Exec | Return Node | Exec |
| 26 | Set Value as Vector (Invalid) | Exec | Return Node | Exec |

### 7) Compile and Save

#### 7.1) Compile Blueprint
   - 7.1.1) Click Compile button
   - 7.1.2) Verify no errors

#### 7.2) Save Blueprint
   - 7.2.1) Press Ctrl+S

---

## PHASE 8: CREATE BPA_ATTACK_FORMATION ACTIVITY

### 1) Create Activity Blueprint

#### 1.1) Navigate to Activities Folder
   - 1.1.1) In Content Browser, navigate to Content/AI/Activities/

#### 1.2) Create Child Blueprint
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Blueprint Class
   - 1.2.3) In search field, type: BPA_Attack_Melee
   - 1.2.4) Select BPA_Attack_Melee as parent class
   - 1.2.5) Click Select
   - 1.2.6) Name it: BPA_Attack_Formation

### 2) Open Blueprint Editor

#### 2.1) Open Asset
   - 2.1.1) Double-click BPA_Attack_Formation to open

### 3) Add Leash Variables

#### 3.1) Add LeashRadius Variable
   - 3.1.1) In My Blueprint panel, click + button next to Variables
   - 3.1.2) Name it: LeashRadius
   - 3.1.3) In Details panel, set Variable Type to Float
   - 3.1.4) Check Instance Editable checkbox
   - 3.1.5) Set Category to: Formation
   - 3.1.6) Set Default Value to: 400.0

#### 3.2) Add BBKey_TargetLocation Variable
   - 3.2.1) Click + button next to Variables
   - 3.2.2) Name it: BBKey_TargetLocation
   - 3.2.3) In Details panel, set Variable Type to Name
   - 3.2.4) Set Category to: Blackboard
   - 3.2.5) Set Default Value to: TargetLocation

### 4) Override ScoreGoalItem Function

#### 4.1) Create Function Override
   - 4.1.1) In My Blueprint panel, find Functions section
   - 4.1.2) Click Override dropdown
   - 4.1.3) Select Score Goal Item

### 5) Implement Leash Check Logic

#### 5.1) Get Attack Target Location
   - 5.1.1) From Goal Item input pin, drag and release
   - 5.1.2) Search for: Cast to Goal_Attack
   - 5.1.3) Select Cast To Goal_Attack
   - 5.1.4) From As Goal Attack, drag and release
   - 5.1.5) Search for: Get Target Actor
   - 5.1.6) From Target Actor Return Value, drag and release
   - 5.1.7) Search for: Get Actor Location
   - 5.1.8) Select Get Actor Location

#### 5.2) Get Formation Position from Blackboard
   - 5.2.1) Right-click, search for: Get Owner Controller
   - 5.2.2) From Return Value, drag and release
   - 5.2.3) Search for: Get Blackboard Component
   - 5.2.4) From Return Value, drag and release
   - 5.2.5) Search for: Get Value as Vector
   - 5.2.6) Select Get Value as Vector
   - 5.2.7) GET BBKey_TargetLocation variable
   - 5.2.8) Connect to Key Name pin

#### 5.3) Calculate Distance
   - 5.3.1) From Get Actor Location Return Value (enemy location), drag and release
   - 5.3.2) Search for: Vector - Vector (Subtract)
   - 5.3.3) Connect Get Value as Vector Return Value to second input
   - 5.3.4) From Subtract result, drag and release
   - 5.3.5) Search for: Vector Length
   - 5.3.6) Select Vector Length

#### 5.4) Check Leash Radius
   - 5.4.1) From Vector Length Return Value, drag and release
   - 5.4.2) Search for: Less Than or Equal (Float)
   - 5.4.3) GET LeashRadius variable
   - 5.4.4) Connect to second input of Less Than or Equal
   - 5.4.5) Add Branch node
   - 5.4.6) Connect Less Than or Equal result to Branch Condition

#### 5.5) Return Score Based on Distance
   - 5.5.1) From Branch True pin:
   - 5.5.2) Right-click, search for: Call to Parent Function
   - 5.5.3) Select Parent: Score Goal Item
   - 5.5.4) Connect Goal Item input to Parent function Goal Item pin
   - 5.5.5) Add Return Node
   - 5.5.6) Connect Parent Score Goal Item Return Value to Return Node
   - 5.5.7) From Branch False pin:
   - 5.5.8) Add Return Node
   - 5.5.9) Set Return Value to: 0.0

### 6) ScoreGoalItem Node Connection Summary

| Step | From Node | From Pin | To Node | To Pin |
|------|-----------|----------|---------|--------|
| 1 | Score Goal Item | Exec | Cast to Goal_Attack | Exec |
| 2 | Goal Item | Value | Cast to Goal_Attack | Object |
| 3 | As Goal Attack | Value | Get Target Actor | Target |
| 4 | Get Target Actor | Return Value | Get Actor Location | Target |
| 5 | Get Owner Controller | Return Value | Get Blackboard Component | Target |
| 6 | Get Blackboard Component | Return Value | Get Value as Vector | Target |
| 7 | BBKey_TargetLocation | Value | Get Value as Vector | Key Name |
| 8 | Get Actor Location | Return Value | Subtract | A |
| 9 | Get Value as Vector | Return Value | Subtract | B |
| 10 | Subtract | Return Value | Vector Length | Target |
| 11 | Vector Length | Return Value | Less Than or Equal | A |
| 12 | LeashRadius | Value | Less Than or Equal | B |
| 13 | Less Than or Equal | Return Value | Branch | Condition |
| 14 | Cast to Goal_Attack | Exec | Branch | Exec |
| 15 | Branch | True | Parent: Score Goal Item | Exec |
| 16 | Goal Item | Value | Parent: Score Goal Item | Goal Item |
| 17 | Parent: Score Goal Item | Return Value | Return Node (True) | Return Value |
| 18 | Branch | False | Return Node (False) | Exec |
| 19 | Return Node (False) | Return Value | 0.0 | Value |

### 7) Configure Class Defaults

#### 7.1) Click Class Defaults

#### 7.2) Set Activity Name
   - 7.2.1) In Details panel, find Activity Name
   - 7.2.2) Set to: Formation Attack

### 8) Compile and Save

#### 8.1) Compile Blueprint
   - 8.1.1) Click Compile button
   - 8.1.2) Verify no errors

#### 8.2) Save Blueprint
   - 8.2.1) Press Ctrl+S

---

## PHASE 9: CREATE AC_GUARD_FORMATION ACTIVITYCONFIGURATION

### 1) Create ActivityConfiguration Asset

#### 1.1) Navigate to Configurations Folder
   - 1.1.1) In Content Browser, navigate to Content/AI/Configurations/
   - 1.1.2) If folder does not exist, right-click and create it

#### 1.2) Create New Data Asset
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Miscellaneous
   - 1.2.3) Select Data Asset
   - 1.2.4) In class picker, search for: NPCActivityConfiguration
   - 1.2.5) Select NPCActivityConfiguration
   - 1.2.6) Name it: AC_FormationGuardBehavior

### 2) Open Data Asset Editor

#### 2.1) Open Asset
   - 2.1.1) Double-click AC_FormationGuardBehavior to open

### 3) Configure Activity Settings

#### 3.1) Set Request Interval
   - 3.1.1) In Details panel, find Request Interval
   - 3.1.2) Set to: 0.5

### 4) Configure Default Activities Array

#### 4.1) Add Formation Follow Activity
   - 4.1.1) Find Default Activities array
   - 4.1.2) Click + button to add element
   - 4.1.3) Set element [0] to: BPA_FormationFollow

#### 4.2) Add Formation Attack Activity
   - 4.2.1) Click + button to add element
   - 4.2.2) Set element [1] to: BPA_Attack_Formation

#### 4.3) Add Idle Activity (Optional)
   - 4.3.1) Click + button to add element
   - 4.3.2) Set element [2] to: BPA_Idle

### 5) Configure Goal Generators

#### 5.1) Add Attack Goal Generator
   - 5.1.1) Find Goal Generators array
   - 5.1.2) Click + button to add element
   - 5.1.3) Set element [0] to: GoalGenerator_Attack

### 6) Final Default Activities Array

| Index | Activity | Purpose |
|-------|----------|---------|
| [0] | BPA_FormationFollow | Default formation following |
| [1] | BPA_Attack_Formation | Leashed attack behavior |
| [2] | BPA_Idle | Fallback idle |

### 7) Goal Generators Array

| Index | Generator | Purpose |
|-------|-----------|---------|
| [0] | GoalGenerator_Attack | Creates attack goals from perception |

### 8) Save Data Asset

#### 8.1) Save Asset
   - 8.1.1) Press Ctrl+S

---

## PHASE 10: SPAWN GUARDS WITH FORMATION GOALS

### 1) Configure Guard NPCDefinition

#### 1.1) Open or Create NPCDefinition
   - 1.1.1) Navigate to your Guard NPCDefinition asset
   - 1.1.2) Double-click to open

#### 1.2) Set Activity Configuration
   - 1.2.1) In NPCDefinition, find Activity Configuration property
   - 1.2.2) Set to: AC_FormationGuardBehavior

### 2) Create Formation Goal Assignment Function

#### 2.1) Open Target Character Blueprint
   - 2.1.1) Open the character that guards will follow (e.g., player or VIP)
   - 2.1.2) Add function to assign formation goals

#### 2.2) Create AssignFormationGuards Function
   - 2.2.1) In My Blueprint panel, click + next to Functions
   - 2.2.2) Name it: AssignFormationGuards
   - 2.2.3) Add Input parameter: Guards (Array of NarrativeNPCCharacter)

#### 2.3) Implement Formation Goal Assignment

##### 2.3.1) Add ForEachLoop
   - 2.3.1.1) Right-click, search for: For Each Loop
   - 2.3.1.2) Connect Guards array to Array pin

##### 2.3.2) Get Guard Controller
   - 2.3.2.1) From Array Element, drag and release
   - 2.3.2.2) Search for: Get Controller
   - 2.3.2.3) Cast to NarrativeNPCController

##### 2.3.3) Get Activity Component
   - 2.3.3.1) From As Narrative NPC Controller, drag and release
   - 2.3.3.2) Search for: Get Activity Component
   - 2.3.3.3) Select Get Activity Component

##### 2.3.4) Calculate Formation Offset
   - 2.3.4.1) From For Each Loop Array Index, create Select node
   - 2.3.4.2) Add 4 vector options:

| Index | Vector Value | Position |
|-------|--------------|----------|
| 0 | X=200, Y=200, Z=0 | Front-Right |
| 1 | X=200, Y=-200, Z=0 | Front-Left |
| 2 | X=-200, Y=200, Z=0 | Back-Right |
| 3 | X=-200, Y=-200, Z=0 | Back-Left |

##### 2.3.5) Construct Formation Goal
   - 2.3.5.1) Right-click, search for: Construct Object from Class
   - 2.3.5.2) Set Class to: Goal_FormationFollow
   - 2.3.5.3) Set Outer to: Self (the function owner)
   - 2.3.5.4) Expose spawn parameters by clicking arrow on node

##### 2.3.6) Set Goal Properties
   - 2.3.6.1) Set Target to Follow to: Self (the character being followed)
   - 2.3.6.2) Connect Select Return Value to Formation Offset pin
   - 2.3.6.3) Connect Array Index to Formation Index pin
   - 2.3.6.4) Set Goal Lifetime to: -1.0
   - 2.3.6.5) Set Default Score to: 1.0

##### 2.3.7) Add Goal to Guard
   - 2.3.7.1) From Get Activity Component Return Value, drag and release
   - 2.3.7.2) Search for: Add Goal
   - 2.3.7.3) Select Add Goal
   - 2.3.7.4) Connect Construct Object Return Value to New Goal pin
   - 2.3.7.5) Set Trigger Reselect to: True (checked)

### 3) Usage Example - Spawning 4 Guards

#### 3.1) Spawn Guards at Locations
   - 3.1.1) Use NPCSpawner or manual spawn
   - 3.1.2) Spawn 4 guards near the target character
   - 3.1.3) Collect spawned guard references into array

#### 3.2) Call AssignFormationGuards
   - 3.2.1) After spawning, call AssignFormationGuards
   - 3.2.2) Pass the array of 4 guard references
   - 3.2.3) Guards will automatically assume formation positions

### 4) Alternative: Dialogue-Based Goal Assignment

#### 4.1) Create Dialogue Event
   - 4.1.1) In Dialogue Blueprint, add NPC dialogue node
   - 4.1.2) Add Event: AI: Add Goal To NPC
   - 4.1.3) Set Goal to Add to: Goal_FormationFollow

#### 4.2) Configure Goal Properties in Dialogue
   - 4.2.1) Set Target to Follow Asset to: CD_DefaultPlayer (or target CharacterDefinition)
   - 4.2.2) Set Formation Offset per guard
   - 4.2.3) Set Formation Index per guard
   - 4.2.4) Set Goal Lifetime to: -1.0

---

## CHANGELOG

### VERSION 2.7 - Attribute GE and AbilityConfiguration (GF-2)

**Release Date:** January 2026

| Change | Description |
|--------|-------------|
| GF-2 | Added GE_FormationGuardAttributes (150 HP, 20 ATK, 15 Armor) for guard attribute initialization |
| GF-2 | Added AC_FormationGuard AbilityConfiguration with startup_effects referencing the attribute GE |
| Audit | Per NPC_Systems_Comprehensive_Audit_v1_0.md decision GF-2 (MEDIUM priority) |
| Faction | Guards use `Narrative.Factions.Returned` consistent with other NPCs |

---

### VERSION 2.6 - Audit Compliance Update

**Release Date:** January 2026

| Change | Description |
|--------|-------------|
| INC-1 Fix | Renamed `AC_GuardFormationBehavior` → `AC_FormationGuardBehavior` to match manifest `AC_{NPCName}Behavior` pattern |
| Audit | Claude-GPT dual audit verified naming conventions against manifest and Narrative Pro standards |

---

### VERSION 2.5 - Naming Convention Update

**Release Date:** January 2026

| Change | Description |
|--------|-------------|
| Naming Conventions | Updated to Narrative Pro v2.2: AC_Guard_Formation → AC_GuardFormationBehavior (AC_*Behavior suffix pattern). |

---

### VERSION 2.4 - Reference Updates

**Release Date:** January 2026

| Change | Description |
|--------|-------------|
| Narrative Pro | Updated version reference from v2.1 to v2.2 |

---

### VERSION 2.3 - Simplified Tag Creation

**Release Date:** January 2026

| Change | Description |
|--------|-------------|
| PHASE 1 Simplified | Replaced 3 detailed subsections with consolidated tag table |
| Tag Creation Format | Tags now listed in Create Required Tags table |
| Line Reduction | Reduced PHASE 1 from 25 lines to 7 lines |

---

### VERSION 2.2 - SetupBlackboard Node Creation and Dual Vector Paths

**Release Date:** December 2025

**Corrections Applied:**

| Correction # | Issue | Fix |
|--------------|-------|-----|
| C-013 | Steps 5.7, 5.8 said drag from exec pin to create Set Value as Object | Create from BB pin, then connect exec separately |
| C-014 | Step 5.9.4 said connect both exec paths to same Set Value as Vector | Use two Set Value as Vector nodes (one per path), both connect to Return Node |

**Phase 7 Changes - BPA_FormationFollow SetupBlackboard:**

Node Creation Method Fixed:

| Old (Wrong) | New (Correct) |
|-------------|---------------|
| From Is Valid exec, drag to create Set Value as Object | From BB pin, create Set Value as Object, then connect Is Valid exec |
| From Is Not Valid exec, drag to create Set Value as Object | From BB pin, create Set Value as Object, then connect Is Not Valid exec |

Dual Set Value as Vector Nodes:

| Path | Nodes |
|------|-------|
| Is Valid | Set Value as Object (Valid) -> Set Value as Vector (Valid) -> Return |
| Is Not Valid | Set Value as Object (Invalid) -> Set Value as Vector (Invalid) -> Return |

Data Connections (Both Set Value as Vector nodes):

| Pin | Connection |
|-----|------------|
| Target | BB (Blackboard Component) |
| Key Name | BBKey_FormationOffset |
| Vector Value | Get Formation Offset Return Value |

**Node Connection Summary Updated:**

| Before | After |
|--------|-------|
| 22 connections | 26 connections |
| 1 Set Value as Vector | 2 Set Value as Vector nodes |

---

### VERSION 2.1 - Service Intervals and SetupBlackboard Logic Fix

**Release Date:** December 2025

**Corrections Applied:**

| Correction # | Issue | Fix |
|--------------|-------|-----|
| C-008 | BTS_CalculateFormationPosition interval too frequent (0.1/0.05) | Changed to Interval: 0.5, Deviation: 0.1 |
| C-009 | BTS_AdjustFormationSpeed interval too frequent (0.2/0.05) | Changed to Interval: 1.0, Deviation: 0.2 |
| C-010 | Step 5.6 implied exec connection to pure nodes | Clarified Get Subsystem and Find Character are pure (no exec pins) |
| C-011 | Step 5.6.7 said "Target to Follow" | Changed to "Get Target to Follow Asset" |
| C-012 | Step 5.7 used local variable (confusing) | Removed local variable, use two Set Value as Object nodes instead |

**Phase 4 Changes - BTS_CalculateFormationPosition:**

| Property | Before | After |
|----------|--------|-------|
| Interval | 0.1 | 0.5 |
| Random Deviation | 0.05 | 0.1 |

**Phase 5 Changes - BTS_AdjustFormationSpeed:**

| Property | Before | After |
|----------|--------|-------|
| Interval | 0.2 | 1.0 |
| Random Deviation | 0.05 | 0.2 |

**Phase 7 Changes - BPA_FormationFollow SetupBlackboard:**

Steps 5.6 through 5.10 rewritten:

| Old Approach | New Approach |
|--------------|--------------|
| Local variable TargetToSet | Removed |
| Single Set Value as Object | Two Set Value as Object nodes (one per path) |
| Implied exec to pure nodes | Pure nodes created separately, exec flows to SET nodes |

---

### VERSION 2.0 - Remove FollowDistance (Hardcoded Acceptable Radius)

**Release Date:** December 2025

**Corrections Applied:**

| Correction # | Issue | Fix |
|--------------|-------|-----|
| C-006 | FollowDistance listed as inherited from Goal_FollowCharacter | Does not exist in parent - removed entirely |
| C-007 | FollowDistance blackboard key not needed | Use hardcoded Acceptable Radius on Move To task |

**Phase 2 Changes - BB_FormationFollow:**

| Before | After |
|--------|-------|
| 5 blackboard keys | 4 blackboard keys |
| FollowDistance key (Float) | Removed |

**Phase 3 Changes - Goal_FormationFollow:**

Inherited Properties Table Updated:

| Removed | Reason |
|---------|--------|
| Follow Distance | Does not exist in Goal_FollowCharacter parent |

Default Values Updated:

| Removed | Reason |
|---------|--------|
| Follow Distance: 100.0 | Not inherited, not needed |

**Phase 6 Changes - BT_FormationFollow:**

Move To Task Configuration:

| Property | Value | Source |
|----------|-------|--------|
| Acceptable Radius | 50.0 | Hardcoded on task |

**Phase 7 Changes - BPA_FormationFollow:**

Variables Removed:

| Variable | Reason |
|----------|--------|
| BBKey_FollowDistance | No longer writing to blackboard |

SetupBlackboard Steps Removed:

| Step | Content | Reason |
|------|---------|--------|
| Old 5.9 | Set FollowDistance on Blackboard | Key no longer exists |

**Rationale:**

BT_FollowCharacter (Narrative Pro default) uses hardcoded Acceptable Radius (250.0) on Move To task, not a blackboard key. Our system follows the same pattern with 50.0 for tighter formation positioning.

---

### VERSION 1.9 - Movement Component and Execution Flow Fixes

**Release Date:** December 2025

**Corrections Applied:**

| Correction # | Issue | Fix |
|--------------|-------|-----|
| C-004 | Get Narrative Character Movement only works for NarrativeCharacter (player) | Use GetCharacterMovement (standard UE function works on any Character) |
| C-005 | Cast nodes need execution pins connected | Added explicit execution wiring: Event Tick -> Cast -> Branch |

**Phase 5 Step Reorder:**

| Old Step | New Step | Content |
|----------|----------|---------|
| 4.7 | 4.7 | Cast Guard to NarrativeNPCCharacter (now includes exec connection) |
| 4.11 | 4.8 | Get Guard Movement Component (now uses GetCharacterMovement) |
| 4.7 | 4.9 | Branch on Distance Check (exec from Cast, not Event) |
| 4.8 | 4.10 | Get VIP from Blackboard |
| 4.9 | 4.11 | Cast VIP to Character (not NarrativeNPCCharacter) |
| 4.10 | 4.12 | Get VIP Movement Speed (uses GetCharacterMovement) |

**Execution Flow (Corrected):**

| Order | From | To |
|-------|------|-----|
| 1 | Event Receive Tick AI (Exec) | Cast To NarrativeNPCCharacter (Exec) |
| 2 | Cast To NarrativeNPCCharacter (Exec) | Branch (Exec) |
| 3 | Branch (True) | Set Max Walk Speed (Sprint) |
| 4 | Branch (False) | Set Max Walk Speed (Walk) |

**Node Changes:**

| Old Node | New Node | Reason |
|----------|----------|--------|
| Get Narrative Character Movement | GetCharacterMovement | Works on any Character, not just player |
| Cast To NarrativeNPCCharacter (VIP) | Cast To Character (VIP) | VIP may be player (NarrativeCharacter) or NPC |

---

### VERSION 1.8 - Dynamic Speed Matching and Instance Editable Variables

**Release Date:** December 2025

**Corrections Applied:**

| Correction # | Issue | Fix |
|--------------|-------|-----|
| C-001 | Get Movement Component (Pawn) has no Set Max Walk Speed | Added Cast step before getting movement component |
| C-002 | Generic Cast to Character | Use Cast To NarrativeNPCCharacter and Get Narrative Character Movement |
| C-003 | Hardcoded speed values (600, 300) | Get VIP speed dynamically and match it |

**Phase 5 Rewrite - BTS_AdjustFormationSpeed:**

New Variables Added:

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| BBKey_FollowTarget | Name | FollowTarget | Read VIP from blackboard |
| SprintMultiplier | Float | 1.2 | Multiply VIP speed for catch-up |

Variables Made Instance Editable:

| Variable | Default | Category |
|----------|---------|----------|
| SprintThreshold | 300.0 | Formation Settings |
| SprintMultiplier | 1.2 | Formation Settings |

**Speed Logic (Updated):**

| Condition | Guard Speed |
|-----------|-------------|
| Distance > SprintThreshold | VIP MaxWalkSpeed * SprintMultiplier |
| Distance < SprintThreshold | VIP MaxWalkSpeed (exact match) |

**Node Chain (Updated):**

| Old (Wrong) | New (Correct) |
|-------------|---------------|
| Get Controlled Pawn | Get Controlled Pawn |
| Get Character Movement | Cast To NarrativeNPCCharacter |
| Hardcoded 600.0 | Get Narrative Character Movement |
| Hardcoded 300.0 | Get Max Walk Speed (from VIP) |

**Step Count Change:**
- Previous: 4.1 to 4.10 (10 steps)
- Current: 4.1 to 4.15 (15 steps)

---

### VERSION 1.7 - Custom Formation Speed Service

**Release Date:** December 2025

**Change Rationale:**
- BTS_AdjustFollowSpeed checks distance to FollowTarget (VIP)
- Formation guards are beside VIP, not behind
- Distance to VIP stays constant even when out of formation position
- Need to check distance to TargetLocation (formation position) instead

**New Component:**
- BTS_AdjustFormationSpeed (Phase 5)

**Speed Logic:**

| Distance to TargetLocation | Action |
|---------------------------|--------|
| Greater than 300 units | Sprint (600 speed) |
| Less than 300 units | Walk (300 speed) |

**Relation to Leash System:**

| System | Reference Point | Purpose |
|--------|-----------------|---------|
| BTS_AdjustFormationSpeed | TargetLocation | Movement speed to reach position |
| BPA_Attack_Formation | TargetLocation | Attack range from position |

**Phase Changes:**
- Added Phase 5: Create BTS_AdjustFormationSpeed Service
- Renumbered Phase 5-9 to Phase 6-10
- Updated BT_FormationFollow to use BTS_AdjustFormationSpeed
- Added BTS_AdjustFormationSpeed to Assets to Create table

---

### VERSION 1.6 - Remove BTS_SetAIFocus from Formation Tree

**Release Date:** December 2025

**Change Rationale:**
- Bodyguards should scan perimeter, not stare at VIP
- AI Perception system handles threat detection and focus
- Guards naturally face movement direction

**Phase 5 Changes:**
- Removed section 3.2 (Add SetAIFocus Service to Selector)
- Renumbered sections 3.3-3.8 to 3.2-3.7
- Updated Final Tree Structure table

**Guard Focus Behavior:**

| Before (Wrong) | After (Correct) |
|----------------|-----------------|
| Always looks at VIP | Looks at movement direction |
| Faces inward | Faces outward (perimeter scan) |
| Cannot see threats | AI Perception detects threats |

---

### VERSION 1.5 - Pure Function Execution Flow Correction

**Release Date:** December 2025

**Corrections Applied:**
- Get Value as Object is a PURE function with no execution pins
- Execution flow now correctly bypasses pure function nodes
- Event Receive Tick AI execution connects directly to Cast To Actor

**Phase 4 Fixes (BTS_CalculateFormationPosition):**
- Step 4.3: Removed incorrect execution connection to Get Value as Object
- Step 4.4.4: Execution now connects from Event Receive Tick AI to Cast To Actor
- Node Connection Summary table updated with correct execution flow
- Reduced table from 20 rows to 19 rows (removed duplicate execution entry)

**Execution Flow (Corrected):**

| Order | From | To |
|-------|------|-----|
| 1 | Event Receive Tick AI (Exec) | Cast To Actor (Exec) |
| 2 | Cast To Actor (Exec) | Is Valid (Exec) |
| 3 | Is Valid (Is Valid pin) | Set Value as Vector (Exec) |

---

### VERSION 1.4 - Debug Section Removal

**Release Date:** December 2025

**Removed Content:**
- Removed GetDebugString override section from Phase 3
- Debug functionality is optional and not required for system operation
- Per project instructions: no testing, debugging, or troubleshooting sections

**Phase 3 Changes:**
- Section 5 (GetDebugString) removed entirely
- Section 6 (Compile and Save) renumbered to Section 5

---

### VERSION 1.3 - Is Valid Macro Correction

**Release Date:** December 2025

**Corrections Applied:**
- Removed unnecessary Branch nodes after Is Valid macro
- Is Valid macro has built-in Is Valid and Is Not Valid execution pins
- No separate Branch node needed when using Is Valid macro

**Phase 4 Fixes (BTS_CalculateFormationPosition):**
- Step 4.5: Removed Branch node, use Is Valid execution pins directly
- Step 4.6: Changed "From Branch True" to "From Is Valid execution pin"

**Phase 6 Fixes (BPA_FormationFollow SetupBlackboard):**
- Step 5.5: Removed Branch node, use Is Valid macro execution pins
- Step 5.6: Changed "From Branch False" to "From Is Not Valid execution pin"
- Step 5.7: Changed "From Branch True" to "From Is Valid execution pin"
- Node Connection Summary table updated to remove Branch references

---

### VERSION 1.1 - Leash Attack System

**Release Date:** December 2025

**New Components:**
- BPA_Attack_Formation (child of BPA_Attack_Melee with leash check)
- AC_FormationGuardBehavior (dedicated ActivityConfiguration for guards)

**Leash System Features:**
- ScoreGoalItem override checks enemy distance from TargetLocation
- LeashRadius variable (default 400.0 units)
- Dynamic leash center follows formation position
- Guards attack nearby enemies without leaving formation
- Guards disengage when target retreats (leash center moves)

**Guard Behavior:**
- Enemy within LeashRadius: Normal attack scoring
- Enemy outside LeashRadius: Score returns 0 (ignore enemy)
- Formation position updates every 0.1s via BTS_CalculateFormationPosition
- Attack eligibility re-evaluated as target moves

**Phase Updates:**
- Phase 7: Create BPA_Attack_Formation Activity (NEW)
- Phase 8: Create AC_FormationGuardBehavior ActivityConfiguration (NEW)
- Phase 9: Spawn Guards with Formation Goals (renamed from Phase 7)

---

### VERSION 1.0 - Initial Implementation

**Release Date:** December 2025

**System Created:**
- Goal_FormationFollow (child of Goal_FollowCharacter)
- BB_FormationFollow (5 keys including TargetLocation vector)
- BTS_CalculateFormationPosition (calculates world position from offset)
- BT_FormationFollow (uses TargetLocation for MoveTo)
- BPA_FormationFollow (activity with SetupBlackboard override)

**Formation Support:**
- Square formation with 4 guard positions
- Configurable offset per guard via FormationOffset vector
- Dynamic rotation following target facing direction
- Speed matching via BTS_AdjustFollowSpeed integration

**Narrative Pro Integration:**
- Inherits from Goal_FollowCharacter for persistence
- Uses NPCActivity patterns for activity management
- Compatible with NPCActivityComponent goal system
- Supports CharacterDefinition-based target resolution

---

**END OF GUARD FORMATION FOLLOW SYSTEM IMPLEMENTATION GUIDE v2.5**

**Unreal Engine 5.7 + Narrative Pro v2.2**

**Blueprint-Only Implementation**
