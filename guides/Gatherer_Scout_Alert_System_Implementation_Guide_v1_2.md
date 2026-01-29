# Gatherer Scout Alert System Implementation Guide
## Passive Enemy with Reinforcement Summoning
## Version 1.2

---

## DOCUMENT INFORMATION

| Field | Value |
|-------|-------|
| Document Type | Implementation Guide |
| System | Gatherer Scout Alert System |
| Last Updated | January 2026 |
| Unreal Engine | 5.7 |
| Narrative Pro | v2.2 |

---

## TABLE OF CONTENTS

| Phase | Name |
|-------|------|
| PHASE 1 | VERIFY GAMEPLAY TAGS |
| PHASE 2 | CREATE GOAL_ALERT |
| PHASE 3 | CREATE GOALGENERATOR_ALERT |
| PHASE 4 | CREATE BPA_ALERT |
| PHASE 5 | CREATE BPA_GATHER (OPTIONAL) |
| PHASE 6 | CREATE ACTIVITYCONFIGURATION |
| PHASE 7 | CREATE NPCDEFINITIONS |
| PHASE 8 | WORLD PLACEMENT |

---

## INTRODUCTION

### System Overview

| Aspect | Description |
|--------|-------------|
| Concept | Passive enemy NPCs that gather resources and alert reinforcements when spotting hostiles |
| Behavior | Wander/gather until detecting hostile faction actor via AIPerception |
| Alert | Play signal animation, spawn reinforcements at alert location |
| Post-Alert | Gatherer flees using existing BPA_Flee system |
| Risk/Reward | Gatherers have lootable inventory but alerting brings reinforcements |

### System Flow

| Step | Actor | Action |
|------|-------|--------|
| 1 | BP_GathererScout | Wanders or gathers (default behavior) |
| 2 | AIPerception | Detects hostile faction actor (small sight radius) |
| 3 | GoalGenerator_Alert | Creates Goal_Alert from perception |
| 4 | BPA_Alert | Scores highest (5.0), runs alert behavior |
| 5 | BPA_Alert | Plays signal animation |
| 6 | BPA_Alert | Spawns reinforcements at own location |
| 7 | BPA_Alert | Adds Goal_MoveToDestination to each reinforcement |
| 8 | BPA_Alert | Adds Goal_Flee to self |
| 9 | BPA_Alert | Removes Goal_Alert, ends activity |
| 10 | BP_GathererScout | BPA_Flee takes over automatically |
| 11 | Reinforcements | BPA_MoveToDestination runs (travel to alert) |
| 12 | Reinforcements | GoalGenerator_Attack detects enemies on arrival |
| 13 | Reinforcements | Combat activities engage |

### Architecture Overview

| Component | Parent Class | Purpose |
|-----------|--------------|---------|
| BP_GathererScout | NarrativeNPCCharacter | Passive enemy with perception |
| Goal_Alert | NPCGoalItem | Stores alert location and spotted target |
| GoalGenerator_Alert | NPCGoalGenerator | Creates alert goals from perception |
| BPA_Alert | NPCActivity | Signal animation + spawn reinforcements |
| BPA_Interact | NPCActivity | Optional gathering behavior (flavor) |
| AC_GathererScoutBehavior | ActivityConfiguration | Activities for gatherer |
| NPC_GathererScout | NPCDefinition | Gatherer character definition |
| NPC_Reinforcement | NPCDefinition | Reinforcement character definition |

### Activity Scoring

| Activity | Base Score | Condition | Result |
|----------|------------|-----------|--------|
| BPA_Alert | 5.0 | Goal_Alert exists | Runs alert sequence |
| BPA_Flee | 3.0 | Goal_Flee exists | Runs away (existing system) |
| BPA_Interact | 2.0 | Near resource point | Gathering animation |
| BPA_Patrol | 1.0 | Default fallback | Random patrol/wandering |

### Existing Narrative Pro Assets Used

| Asset | Type | Purpose |
|-------|------|---------|
| Goal_MoveToDestination | Goal | Reinforcement travel destination |
| BPA_MoveToDestination | Activity | Reinforcement travel behavior |
| BT_MoveToDestination | Behavior Tree | Reinforcement navigation |
| BB_GoToLocation | Blackboard | Reinforcement travel keys |
| Goal_Flee | Goal | Gatherer escape goal |
| GoalGenerator_Flee | Goal Generator | Creates flee goals (optional) |
| BPA_Flee | Activity | Gatherer escape behavior |
| BT_Flee | Behavior Tree | Flee navigation |
| BB_Flee | Blackboard | Flee target keys |
| EQS_Flee | Environment Query | Find escape location |
| BPA_Patrol | Activity | Default patrol/wandering behavior |
| GoalGenerator_Attack | Goal Generator | Reinforcement combat detection |

---

## QUICK REFERENCE

### Gameplay Tags

| Tag | Purpose |
|-----|---------|
| State.NPC.Alert.Signaling | Applied during alert animation |
| Faction.Enemy.Gatherer | Gatherer faction identifier |
| Faction.Enemy.Reinforcement | Reinforcement faction identifier |
| Goal.Alert | Goal type identifier |

### Assets Created

| Asset | Type | Location |
|-------|------|----------|
| BP_GathererScout | Blueprint Class | /Game/Enemies/Gatherer/ |
| Goal_Alert | Blueprint Class | /Game/AI/Goals/ |
| GoalGenerator_Alert | Blueprint Class | /Game/AI/GoalGenerators/ |
| BPA_Alert | Blueprint Class | /Game/AI/Activities/ |
| BPA_Interact | Blueprint Class | /Game/AI/Activities/ |
| AC_GathererScoutBehavior | ActivityConfiguration | /Game/AI/Configurations/ |
| NPC_GathererScout | NPCDefinition | /Game/Enemies/Gatherer/Definitions/ |
| NPC_Reinforcement | NPCDefinition | /Game/Enemies/Gatherer/Definitions/ |

### Variable Summary (Goal_Alert)

| Variable | Type | Default | SaveGame | Purpose |
|----------|------|---------|----------|---------|
| AlertLocation | Vector | (0,0,0) | Yes | Location where hostile was spotted |
| SpottedTarget | Actor (Object Reference) | None | No | Actor that triggered alert |

### Variable Summary (BPA_Alert)

| Variable | Type | Default | Instance Editable | Purpose |
|----------|------|---------|-------------------|---------|
| ReinforcementDefinition | NPCDefinition (Object Reference) | None | Yes | NPC to spawn as reinforcement |
| ReinforcementCount | Integer | 2 | Yes | Number of reinforcements to spawn |
| SpawnRadius | Float | 100.0 | Yes | Radius around gatherer to spawn |
| SignalMontage | Anim Montage (Object Reference) | None | Yes | Animation to play during alert |

---

## **PHASE 1: VERIFY GAMEPLAY TAGS**

### **1) Open Project Settings**

#### 1.1) Navigate to Gameplay Tags
- 1.1.1) Edit -> Project Settings
- 1.1.2) Project Settings -> Project -> Gameplay Tags
- 1.1.3) Click Gameplay Tag List

#### 1.2) Required Tags
- 1.2.1) State.NPC.Alert.Signaling
- 1.2.2) Faction.Enemy.Gatherer
- 1.2.3) Faction.Enemy.Reinforcement
- 1.2.4) Goal.Alert

---

## **PHASE 2: CREATE GOAL_ALERT**

### **1) Create Blueprint Class**

#### 1.1) Create Asset
- 1.1.1) Content Browser -> /Game/AI/Goals/
- 1.1.2) Right-click in Content Browser
- 1.1.3) Select: Blueprint Class
- 1.1.4) Search parent: NPCGoalItem
- 1.1.5) Select: NPCGoalItem
- 1.1.6) Name: Goal_Alert
- 1.1.7) Double-click to open

### **2) Create Variables**

#### 2.1) AlertLocation Variable
- 2.1.1) Variables panel -> Click + button
- 2.1.2) Name: AlertLocation
- 2.1.3) Type: Vector
- 2.1.4) Default Value: (0, 0, 0)

#### 2.2) SpottedTarget Variable
- 2.2.1) Variables panel -> Click + button
- 2.2.2) Name: SpottedTarget
- 2.2.3) Type: Actor (Object Reference)
- 2.2.4) Default Value: None

### **3) Configure Class Defaults**

#### 3.1) Open Class Defaults
- 3.1.1) Click Class Defaults button

#### 3.2) Set Goal Properties
- 3.2.1) Default Score: 5.0

### **4) Override GetGoalKey Function**

#### 4.1) Create Override
- 4.1.1) My Blueprint panel -> Functions -> Override
- 4.1.2) Search: Get Goal Key
- 4.1.3) Click to override

#### 4.2) Return Spotted Target as Key
- 4.2.1) From Get Goal Key event:
- 4.2.1.1) Drag SpottedTarget variable getter
- 4.2.1.2) Add Return Node
- 4.2.1.3) Connect SpottedTarget to Return Value

### **5) Compile and Save**
- 5.1) Click Compile
- 5.2) Click Save

---

## **PHASE 3: CREATE GOALGENERATOR_ALERT**

### **1) Create Blueprint Class**

#### 1.1) Create Asset
- 1.1.1) Content Browser -> /Game/AI/GoalGenerators/
- 1.1.2) Right-click in Content Browser
- 1.1.3) Select: Blueprint Class
- 1.1.4) Search parent: NPCGoalGenerator
- 1.1.5) Select: NPCGoalGenerator
- 1.1.6) Name: GoalGenerator_Alert
- 1.1.7) Double-click to open

### **2) Create Variables**

#### 2.1) AlertGoalClass Variable
- 2.1.1) Variables panel -> Click + button
- 2.1.2) Name: AlertGoalClass
- 2.1.3) Type: Class Reference (select Goal_Alert as default)
- 2.1.4) Default Value: Goal_Alert

#### 2.2) AlertGoalBaseScore Variable
- 2.2.1) Variables panel -> Click + button
- 2.2.2) Name: AlertGoalBaseScore
- 2.2.3) Type: Float
- 2.2.4) Default Value: 5.0

### **3) Override Event Initialize Goal Generator**

#### 3.1) Create Override
- 3.1.1) My Blueprint panel -> Functions -> Override
- 3.1.2) Search: Event Initialize Goal Generator
- 3.1.3) Click to override

#### 3.2) Bind to Perception Updated
- 3.2.1) From Event Initialize Goal Generator:
- 3.2.1.1) Add Get Owner Controller node
- 3.2.2) From Get Owner Controller Return Value:
- 3.2.2.1) Add Get AI Perception Component node
- 3.2.3) From Get AI Perception Component Return Value:
- 3.2.3.1) Add Is Valid node
- 3.2.4) From Is Valid -> Is Valid pin:
- 3.2.4.1) Add Branch node
- 3.2.5) From Branch True:
- 3.2.5.1) From AIPerception component, drag and search: Bind Event to On Perception Updated
- 3.2.5.2) Create custom event from delegate pin
- 3.2.5.3) Name event: OnPerceptionUpdated

### **4) Implement OnPerceptionUpdated Event**

#### 4.1) Create Perception Handler
- 4.1.1) From OnPerceptionUpdated event (has UpdatedActors array parameter):
- 4.1.1.1) Add For Each Loop node
- 4.1.1.2) Connect UpdatedActors to Array input

#### 4.2) Check if Actor is Hostile
- 4.2.1) From For Each Loop Array Element:
- 4.2.1.1) Add Get Attitude node (ArsenalStatics)
- 4.2.1.2) Test Actor: Get Owner Character (from GoalGenerator)
- 4.2.1.3) Target: Array Element
- 4.2.2) From Get Attitude Return Value:
- 4.2.2.1) Add Equal (Enum) node
- 4.2.2.2) Compare to: ETeamAttitude::Hostile
- 4.2.3) Add Branch node:
- 4.2.3.1) Connect comparison result to Condition

#### 4.3) Check if Already Has Alert Goal for This Target
- 4.3.1) From Branch True:
- 4.3.1.1) Add Get Activity Component node
- 4.3.1.2) Target: Get Owner Character
- 4.3.2) From Get Activity Component Return Value:
- 4.3.2.1) Add Get Goal By Key node
- 4.3.2.2) Goal Type: Goal_Alert
- 4.3.2.3) Key: Array Element (the spotted actor)
- 4.3.3) From Out Succeeded:
- 4.3.3.1) Add Branch node
- 4.3.3.2) Condition: NOT Out Succeeded (add NOT Boolean node)

#### 4.4) Create and Add Goal_Alert
- 4.4.1) From Branch True (goal does NOT exist):
- 4.4.1.1) Add Construct Object from Class node
- 4.4.1.2) Class: AlertGoalClass variable
- 4.4.1.3) Outer: Get Activity Component
- 4.4.2) From Construct Object Return Value:
- 4.4.2.1) Add Cast To Goal_Alert node
- 4.4.3) From successful cast:
- 4.4.3.1) SET SpottedTarget on goal: Array Element
- 4.4.3.2) SET AlertLocation on goal: Get Actor Location of Owner Character
- 4.4.3.3) SET Goal Score: AlertGoalBaseScore variable
- 4.4.4) From SET Goal Score:
- 4.4.4.1) Add Add Goal node (on Activity Component)
- 4.4.4.2) New Goal: The constructed Goal_Alert
- 4.4.4.3) bTriggerReselect: Check (true)

### **5) Compile and Save**
- 5.1) Click Compile
- 5.2) Click Save

---

## **PHASE 4: CREATE BPA_ALERT**

### **1) Create Blueprint Class**

#### 1.1) Create Asset
- 1.1.1) Content Browser -> /Game/AI/Activities/
- 1.1.2) Right-click in Content Browser
- 1.1.3) Select: Blueprint Class
- 1.1.4) Search parent: NPCActivity
- 1.1.5) Select: NPCActivity
- 1.1.6) Name: BPA_Alert
- 1.1.7) Double-click to open

### **2) Create Variables**

#### 2.1) ReinforcementDefinition Variable
- 2.1.1) Variables panel -> Click + button
- 2.1.2) Name: ReinforcementDefinition
- 2.1.3) Type: NPCDefinition (Object Reference)
- 2.1.4) Default Value: None
- 2.1.5) Instance Editable: Check
- 2.1.6) Expose on Spawn: Check

#### 2.2) ReinforcementCount Variable
- 2.2.1) Variables panel -> Click + button
- 2.2.2) Name: ReinforcementCount
- 2.2.3) Type: Integer
- 2.2.4) Default Value: 2
- 2.2.5) Instance Editable: Check

#### 2.3) SpawnRadius Variable
- 2.3.1) Variables panel -> Click + button
- 2.3.2) Name: SpawnRadius
- 2.3.3) Type: Float
- 2.3.4) Default Value: 100.0
- 2.3.5) Instance Editable: Check

#### 2.4) SignalMontage Variable
- 2.4.1) Variables panel -> Click + button
- 2.4.2) Name: SignalMontage
- 2.4.3) Type: Anim Montage (Object Reference)
- 2.4.4) Default Value: None
- 2.4.5) Instance Editable: Check

#### 2.5) CachedAlertGoal Variable
- 2.5.1) Variables panel -> Click + button
- 2.5.2) Name: CachedAlertGoal
- 2.5.3) Type: Goal_Alert (Object Reference)
- 2.5.4) Default Value: None

### **3) Configure Class Defaults**

#### 3.1) Open Class Defaults
- 3.1.1) Click Class Defaults button

#### 3.2) Set Activity Properties
- 3.2.1) Supported Goal Type: Goal_Alert
- 3.2.2) Is Interruptable: Uncheck (alert must complete)

### **4) Override K2_RunActivity**

#### 4.1) Create Override
- 4.1.1) My Blueprint panel -> Functions -> Override
- 4.1.2) Search: K2 Run Activity
- 4.1.3) Click to override

#### 4.2) Cache Alert Goal
- 4.2.1) From K2 Run Activity event:
- 4.2.1.1) Drag from Activity Goal pin
- 4.2.1.2) Add Cast To Goal_Alert node
- 4.2.2) From successful cast:
- 4.2.2.1) SET CachedAlertGoal: As Goal Alert

#### 4.3) Add Signaling State Tag
- 4.3.1) From SET CachedAlertGoal:
- 4.3.1.1) Add Get Owner Character node
- 4.3.2) From Get Owner Character Return Value:
- 4.3.2.1) Add Get Ability System Component node
- 4.3.3) From Get ASC Return Value:
- 4.3.3.1) Add Add Loose Gameplay Tag node
- 4.3.3.2) Tag: State.NPC.Alert.Signaling

#### 4.4) Play Signal Animation
- 4.4.1) From Add Loose Gameplay Tag:
- 4.4.1.1) Add Get Owner Character node
- 4.4.2) From Get Owner Character Return Value:
- 4.4.2.1) Add Play Anim Montage node
- 4.4.2.2) Montage to Play: SignalMontage variable
- 4.4.3) From Play Anim Montage Return Value (duration):
- 4.4.3.1) Add Delay node
- 4.4.3.2) Duration: Connect montage duration (or use fixed 2.0 seconds)

#### 4.5) Spawn Reinforcements Loop
- 4.5.1) From Delay Completed:
- 4.5.1.1) Add For Loop node
- 4.5.1.2) First Index: 0
- 4.5.1.3) Last Index: ReinforcementCount - 1 (add Subtract Integer node)

#### 4.6) Validate Reinforcement Definition
- 4.6.1) From For Loop Body:
- 4.6.1.1) Drag ReinforcementDefinition variable getter
- 4.6.1.2) Add Is Valid node
- 4.6.2) From Is Valid -> Is Valid pin:
- 4.6.2.1) Add Branch node

#### 4.7) Calculate Spawn Location
- 4.7.1) From Branch True:
- 4.7.1.1) Add Get Owner Character node
- 4.7.2) From Get Owner Character:
- 4.7.2.1) Add Get Actor Location node
- 4.7.3) Add Random Unit Vector node
- 4.7.4) Add Multiply (Vector * Float) node:
- 4.7.4.1) Vector: Random Unit Vector result
- 4.7.4.2) Float: SpawnRadius variable
- 4.7.5) Add Vector + Vector node:
- 4.7.5.1) A: Owner Character location
- 4.7.5.2) B: Multiplied random offset
- 4.7.6) Add Make Transform node:
- 4.7.6.1) Location: Vector addition result
- 4.7.6.2) Rotation: Get Owner Character rotation
- 4.7.6.3) Scale: (1, 1, 1) default

#### 4.8) Spawn Reinforcement via Subsystem
- 4.8.1) Add Get NarrativeCharacterSubsystem node
- 4.8.2) From Subsystem Return Value:
- 4.8.2.1) Add Spawn NPC node
- 4.8.2.2) NPCData: ReinforcementDefinition variable
- 4.8.2.3) Transform: Make Transform result
- 4.8.3) From Spawn NPC Return Value:
- 4.8.3.1) Promote to local variable: SpawnedReinforcement

#### 4.9) Add Goal_MoveToDestination to Reinforcement
- 4.9.1) From SpawnedReinforcement:
- 4.9.1.1) Add Is Valid node
- 4.9.2) From Is Valid -> Is Valid pin:
- 4.9.2.1) Add Branch node
- 4.9.3) From Branch True:
- 4.9.3.1) Add Get Activity Component node
- 4.9.3.2) Target: SpawnedReinforcement
- 4.9.4) From Get Activity Component:
- 4.9.4.1) Add Construct Object from Class node
- 4.9.4.2) Class: Goal_MoveToDestination
- 4.9.4.3) Outer: Activity Component
- 4.9.5) From Construct Object Return Value:
- 4.9.5.1) Add Cast To Goal_MoveToDestination
- 4.9.6) From successful cast:
- 4.9.6.1) SET Target Location: CachedAlertGoal -> AlertLocation
- 4.9.6.2) SET Goal Score: 3.0
- 4.9.7) From SET Goal Score:
- 4.9.7.1) Add Add Goal node
- 4.9.7.2) Target: Reinforcement's Activity Component
- 4.9.7.3) New Goal: The constructed Goal_MoveToDestination
- 4.9.7.4) bTriggerReselect: Check (true)

### **5) Cleanup After Loop - Add Goal_Flee to Gatherer**

#### 5.1) After Loop Completes
- 5.1.1) From For Loop Completed:
- 5.1.1.1) Add Get Activity Component node
- 5.1.1.2) Target: Get Owner Character (the gatherer)

#### 5.2) Construct Goal_Flee
- 5.2.1) Add Construct Object from Class node:
- 5.2.1.1) Class: Goal_Flee
- 5.2.1.2) Outer: Activity Component
- 5.2.2) From Construct Object Return Value:
- 5.2.2.1) Add Cast To Goal_Flee
- 5.2.3) From successful cast:
- 5.2.3.1) SET Flee Target: CachedAlertGoal -> SpottedTarget
- 5.2.3.2) SET Goal Score: 3.0

#### 5.3) Add Goal_Flee to Gatherer
- 5.3.1) From SET Goal Score:
- 5.3.1.1) Add Add Goal node
- 5.3.1.2) Target: Gatherer's Activity Component
- 5.3.1.3) New Goal: The constructed Goal_Flee
- 5.3.1.4) bTriggerReselect: Uncheck (will reselect when alert removed)

#### 5.4) Remove Goal_Alert
- 5.4.1) From Add Goal:
- 5.4.1.1) Add Remove Goal node
- 5.4.1.2) Target: Gatherer's Activity Component
- 5.4.1.3) Goal To Remove: CachedAlertGoal

#### 5.5) Remove Signaling State Tag
- 5.5.1) From Remove Goal:
- 5.5.1.1) Add Get Owner Character -> Get ASC
- 5.5.1.2) Add Remove Loose Gameplay Tag node
- 5.5.1.3) Tag: State.NPC.Alert.Signaling

### **6) Compile and Save**
- 6.1) Click Compile
- 6.2) Click Save

---

## **PHASE 5: CREATE BPA_GATHER (OPTIONAL)**

### **1) Create Blueprint Class**

#### 1.1) Create Asset
- 1.1.1) Content Browser -> /Game/AI/Activities/
- 1.1.2) Right-click in Content Browser
- 1.1.3) Select: Blueprint Class
- 1.1.4) Search parent: NPCActivity
- 1.1.5) Select: NPCActivity
- 1.1.6) Name: BPA_Interact
- 1.1.7) Double-click to open

### **2) Configure Class Defaults**

#### 2.1) Open Class Defaults
- 2.1.1) Click Class Defaults button

#### 2.2) Set Activity Properties
- 2.2.1) Supported Goal Type: None (or create Goal_Gather if needed)
- 2.2.2) Is Interruptable: Check

### **3) Override ScoreActivity**

#### 3.1) Create Override
- 3.1.1) My Blueprint panel -> Functions -> Override
- 3.1.2) Search: Score Activity
- 3.1.3) Click to override

#### 3.2) Return Base Score
- 3.2.1) Add Return Node
- 3.2.2) Return Value: 2.0 (constant)

### **4) Override K2_RunActivity**

#### 4.1) Create Override
- 4.1.1) Override K2 Run Activity

#### 4.2) Play Gather Animation
- 4.2.1) From K2 Run Activity:
- 4.2.1.1) Add Get Owner Character node
- 4.2.2) From Get Owner Character:
- 4.2.2.1) Add Play Anim Montage node
- 4.2.2.2) Montage to Play: AM_Gather (placeholder)

### **5) Compile and Save**
- 5.1) Click Compile
- 5.2) Click Save

---

## **PHASE 6: CREATE ACTIVITYCONFIGURATION**

### **1) Create AC_GathererScoutBehavior**

#### 1.1) Create Asset
- 1.1.1) Content Browser -> /Game/AI/Configurations/
- 1.1.2) Right-click in Content Browser
- 1.1.3) Select: Narrative -> NPC Activity Configuration
- 1.1.4) Name: AC_GathererScoutBehavior
- 1.1.5) Double-click to open

#### 1.2) Configure Default Activities
- 1.2.1) Expand: Default Activities
- 1.2.2) Add elements:
- 1.2.2.1) [0] BPA_Alert
- 1.2.2.2) [1] BPA_Flee (existing Narrative Pro asset)
- 1.2.2.3) [2] BPA_Interact (optional)
- 1.2.2.4) [3] BPA_Patrol (existing Narrative Pro asset - default patrol/wandering)

#### 1.3) Configure Goal Generators
- 1.3.1) Expand: Goal Generators
- 1.3.2) Add elements:
- 1.3.2.1) [0] GoalGenerator_Alert

#### 1.4) Save Asset
- 1.4.1) Click Save

---

## **PHASE 7: CREATE NPCDEFINITIONS**

### **1) Create NPC_GathererScout**

#### 1.1) Create Asset
- 1.1.1) Content Browser -> /Game/Enemies/Gatherer/Definitions/
- 1.1.2) Right-click in Content Browser
- 1.1.3) Select: Narrative -> NPC Definition
- 1.1.4) Name: NPC_GathererScout
- 1.1.5) Double-click to open

#### 1.2) Set NPC Class Path
- 1.2.1) NPC Class Path: NarrativeNPCCharacter (or custom BP_GathererScout)

#### 1.3) Set Activity Configuration
- 1.3.1) Activity Configuration: AC_GathererScoutBehavior

#### 1.4) Configure Identity
- 1.4.1) NPC Name: "Gatherer Scout"
- 1.4.2) NPCID: GathererScout
- 1.4.3) Allow Multiple Instances: Check

#### 1.5) Configure Default Tags
- 1.5.1) Default Owned Tags -> Add:
- 1.5.1.1) Faction.Enemy.Gatherer

#### 1.6) Configure Default Item Loadout (Lootable Inventory)
- 1.6.1) Default Item Loadout -> Add entries for resources
- 1.6.2) Example: Scrap Metal, Components, Food, etc.

#### 1.7) Configure AI Perception (via Ability Configuration)
- 1.7.1) Create or reference AbilityConfiguration with perception settings
- 1.7.2) Sight Radius: 800.0 (small detection radius)
- 1.7.3) Lose Sight Radius: 1000.0

#### 1.8) Save Asset
- 1.8.1) Click Save

### **2) Create NPC_Reinforcement**

#### 2.1) Create Asset
- 2.1.1) Right-click in Content Browser
- 2.1.2) Select: Narrative -> NPC Definition
- 2.1.3) Name: NPC_Reinforcement
- 2.1.4) Double-click to open

#### 2.2) Set NPC Class Path
- 2.2.1) NPC Class Path: NarrativeNPCCharacter (or custom combat NPC)

#### 2.3) Set Activity Configuration
- 2.3.1) Activity Configuration: AC_RunAndGun (existing combat config)

#### 2.4) Configure Identity
- 2.4.1) NPC Name: "Reinforcement"
- 2.4.2) NPCID: Reinforcement
- 2.4.3) Allow Multiple Instances: Check

#### 2.5) Configure Default Tags
- 2.5.1) Default Owned Tags -> Add:
- 2.5.1.1) Faction.Enemy.Reinforcement

#### 2.6) Configure Ability Configuration
- 2.6.1) Ability Configuration: Reference combat abilities
- 2.6.2) Should include GoalGenerator_Attack for combat detection

#### 2.7) Save Asset
- 2.7.1) Click Save

### **3) Update BPA_Alert Default Values**

#### 3.1) Open BPA_Alert
- 3.1.1) Double-click BPA_Alert to open

#### 3.2) Set Default Reinforcement Definition
- 3.2.1) Click Class Defaults
- 3.2.2) ReinforcementDefinition: NPC_Reinforcement
- 3.2.3) ReinforcementCount: 2

#### 3.3) Save
- 3.3.1) Click Save

---

## **PHASE 8: WORLD PLACEMENT**

### **1) Configure Faction Attitudes**

#### 1.1) Open NarrativeGameState or Project Settings
- 1.1.1) Configure faction relationships

#### 1.2) Set Faction Attitudes
- 1.2.1) Faction.Enemy.Gatherer -> Hostile to -> Narrative.Factions.Heroes
- 1.2.2) Faction.Enemy.Reinforcement -> Hostile to -> Narrative.Factions.Heroes
- 1.2.3) Faction.Enemy.Gatherer -> Friendly to -> Faction.Enemy.Reinforcement

### **2) Create NPCSpawner for Gatherer**

#### 2.1) Drag NPCDefinition into World
- 2.1.1) Content Browser -> NPC_GathererScout
- 2.1.2) Drag into level viewport
- 2.1.3) NPCSpawner automatically created

#### 2.2) Position Spawner
- 2.2.1) Move to desired patrol/gather area
- 2.2.2) Adjust rotation as needed

### **3) Configure Variable Reinforcement Count (Optional)**

#### 3.1) Per-Location Reinforcement Scaling
- 3.1.1) Select NPCSpawner in World Outliner
- 3.1.2) In BPA_Alert instance settings (via ActivityConfiguration override):
- 3.1.2.1) ReinforcementCount: Adjust per location
- 3.1.2.2) Low-threat areas: 1-2 reinforcements
- 3.1.2.3) High-threat areas: 3-5 reinforcements

---

## SYSTEM BEHAVIOR SUMMARY

### Gatherer Scout Behavior States

| State | Trigger | Activity | Next State |
|-------|---------|----------|------------|
| Idle | Default | BPA_Patrol | Patrol |
| Patrol | No goals | BPA_Patrol | Idle or Interact |
| Interact | Near interactable | BPA_Interact | Patrol |
| Alert | Spots hostile | BPA_Alert | Flee |
| Flee | Goal_Flee added | BPA_Flee | Despawn or Patrol |

### Reinforcement Behavior States

| State | Trigger | Activity | Next State |
|-------|---------|----------|------------|
| Spawned | BPA_Alert spawns | - | Travel |
| Travel | Goal_MoveToDestination | BPA_MoveToDestination | Combat |
| Combat | Arrives + detects enemy | BPA_Attack_* | Combat or Idle |

### Multiplayer Considerations

| Aspect | Implementation |
|--------|----------------|
| Perception | Server authority |
| Goal Creation | Server only |
| NPC Spawning | Server only via subsystem |
| Activity Selection | Server authority |
| Animation Replication | Automatic via montage replication |

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 1.2 | January 2026 | Updated to Narrative Pro v2.2 naming conventions: ActConfig_ → AC_*Behavior suffix (AC_GathererScoutBehavior), NPCDef_ → NPC_* prefix (NPC_GathererScout, NPC_Reinforcement). |
| 1.1 | January 2026 | Fixed ActivityConfiguration naming from AC_ prefix to ActConfig_ prefix for consistency with Narrative Pro conventions. Fixed ActivityConfiguration type name from NPCActivityConfiguration to ActivityConfiguration. Replaced smart quotes with ASCII equivalents. |
| 1.0 | January 2026 | Initial implementation guide |

---

**END OF GUIDE**