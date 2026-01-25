# Support Buffer Healer NPC Implementation Guide

## VERSION 1.1

## Unreal Engine 5.7 + Narrative Pro v2.2

## Blueprint-Only Implementation

---

## DOCUMENT INFORMATION

| Field | Value |
|-------|-------|
| Document Type | Support NPC Implementation Guide |
| NPC Name | Support Buffer |
| Last Updated | January 2026 |
| Version | 1.1 |

---

## TABLE OF CONTENTS

1. [Introduction](#introduction)
2. [Quick Reference](#quick-reference)
3. [Phase 1: Verify Gameplay Tags](#phase-1-verify-gameplay-tags)
4. [Phase 2: Create Gameplay Effects](#phase-2-create-gameplay-effects)
5. [Phase 3: Create BP_SupportBuffer](#phase-3-create-bp_supportbuffer)
6. [Phase 4: Create BTS_HealNearbyAllies Service](#phase-4-create-bts_healnearbyallies-service)
7. [Phase 5: Create BT_SupportFollow Behavior Tree](#phase-5-create-bt_supportfollow-behavior-tree)
8. [Phase 6: Create BPA_SupportFollow Activity](#phase-6-create-bpa_supportfollow-activity)
9. [Phase 7: Create Ability Configuration](#phase-7-create-ability-configuration)
10. [Phase 8: Create Activity Configuration](#phase-8-create-activity-configuration)
11. [Phase 9: Create NPCDefinition](#phase-9-create-npcdefinition)
12. [Phase 10: Configure Faction Attitudes](#phase-10-configure-faction-attitudes)
13. [Phase 11: World Placement and Goal Assignment](#phase-11-world-placement-and-goal-assignment)
14. [Changelog](#changelog)

---

## INTRODUCTION

### NPC Overview

| Property | Value |
|----------|-------|
| Name | Support Buffer |
| Type | Non-Combat Support NPC |
| Parent Class | NarrativeNPCCharacter |
| Role | Healer and Speed Buffer |
| Combat Style | Passive - does NOT attack |

### Behavior Flow

| State | Trigger | Action |
|-------|---------|--------|
| Follow | Goal_FollowCharacter assigned | Stays close to designated leader |
| Scan | BTS_HealNearbyAllies service tick | Checks for damaged friendlies in range |
| Heal | Damaged friendly found | Applies GE_SupportHeal + Speed Boost |
| Cooldown | After healing | Waits before next heal check |

### Key Mechanics

| Mechanic | Implementation |
|----------|----------------|
| Healing | NarrativeHealExecution with SetByCaller.Heal tag |
| Speed Boost | Direct CharacterMovement modification + Timer restoration |
| Target Detection | Sphere Overlap + GetAttitude (Friendly filter) |
| Health Check | Compare Health to MaxHealth attribute |
| Stay Near Combat | BPA_FollowCharacter with Goal_FollowCharacter |

### Narrative Pro Systems Leveraged

| System | Asset | Purpose |
|--------|-------|---------|
| Healing Calc | NarrativeHealExecution | Proper heal application via Heal meta-attribute |
| Follow System | Goal_FollowCharacter | Stores follow target |
| Follow Activity | BPA_FollowCharacter (parent) | Base follow behavior |
| Blackboard | BB_FollowCharacter | Follow target storage |
| Faction System | ArsenalStatics::GetAttitude | Filter friendly targets |
| Attribute System | Health, MaxHealth | Damage detection |

---

## QUICK REFERENCE

### Gameplay Tags

| Tag | Purpose |
|-----|---------|
| State.NPC.SupportBuffer | NPC identifier |
| State.NPC.Support.Healing | Applied during heal action |
| Faction.Friendly.Support | Faction identifier |
| Effect.Support.Heal | Heal effect tag |
| Effect.Support.SpeedBoost | Speed boost state tag |
| GameplayCue.Support.Heal | Heal VFX/SFX trigger |

### Assets Created

| Asset | Type | Location |
|-------|------|----------|
| BP_SupportBuffer | Blueprint | /Game/NPCs/Support/ |
| BTS_HealNearbyAllies | Blueprint | /Game/AI/Services/ |
| BT_SupportFollow | Behavior Tree | /Game/AI/BehaviorTrees/ |
| BPA_SupportFollow | Blueprint | /Game/AI/Activities/ |
| GE_SupporterAttributes | Gameplay Effect | /Game/NPCs/Support/Effects/ |
| GE_SupportHeal | Gameplay Effect | /Game/NPCs/Support/Effects/ |
| AC_SupportBuffer | Data Asset | /Game/NPCs/Support/Configurations/ |
| AC_SupportBufferBehavior | Data Asset | /Game/NPCs/Support/Configurations/ |
| NPC_SupportBuffer | Data Asset | /Game/NPCs/Support/Definitions/ |

### Existing Narrative Pro Assets Used

| Asset | Type | Purpose |
|-------|------|---------|
| BB_FollowCharacter | Blackboard | FollowTarget, FollowDistance keys |
| Goal_FollowCharacter | Goal Class | Target storage for following |
| NarrativeHealExecution | Execution Calc | Proper heal calculation |
| ArsenalStatics | Function Library | GetAttitude for faction check |
| BPA_Idle | Activity | Default idle fallback |

### Variable Summary (BP_SupportBuffer)

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| HealRadius | Float | 500.0 | Yes |
| HealAmount | Float | 50.0 | Yes |
| HealCooldown | Float | 3.0 | Yes |
| SpeedBoostPercent | Float | 30.0 | Yes |
| SpeedBoostDuration | Float | 5.0 | Yes |
| bCanHeal | Boolean | true | No |
| BoostedActorSpeeds | Map (Actor -> Float) | - | No |

### Variable Summary (BTS_HealNearbyAllies)

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| HealRadius | Float | 500.0 | Yes |
| DamageThreshold | Float | 0.9 | Yes |

---

## PHASE 1: VERIFY GAMEPLAY TAGS

### **1) Open Gameplay Tags Manager**

#### 1.1) Navigate to Project Settings
   - 1.1.1) Edit -> Project Settings
   - 1.1.2) Navigate to Project -> Gameplay Tags

#### 1.2) Open Gameplay Tags Editor
   - 1.2.1) Click Manage Gameplay Tags button

### **2) Required Tags**

| Tag | Purpose |
|-----|---------|
| State.NPC.SupportBuffer | NPC identifier |
| State.NPC.Support.Healing | Healing state |
| Faction.Friendly.Support | Faction identifier |
| Effect.Support.Heal | Heal effect tag |
| Effect.Support.SpeedBoost | Speed boost effect tag |
| GameplayCue.Support.Heal | Heal VFX/SFX |
| SetByCaller.Heal | Heal magnitude tag (may already exist) |

### **3) Compile and Save**

---

## PHASE 2: CREATE GAMEPLAY EFFECTS

### **1) Create GE_SupporterAttributes**

#### 1.1) Navigate to Effects Folder
   - 1.1.1) Content Browser -> /Game/NPCs/Support/Effects/

#### 1.2) Create Gameplay Effect
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Blueprint Class
   - 1.2.3) Search parent: GameplayEffect
   - 1.2.4) Select GameplayEffect
   - 1.2.5) Name: GE_SupporterAttributes
   - 1.2.6) Double-click to open

#### 1.3) Configure Effect Properties

| Property | Value |
|----------|-------|
| Duration Policy | Instant |

#### 1.4) Configure Modifiers

| Index | Attribute | Modifier Op | Magnitude Type | Scalable Float |
|-------|-----------|-------------|----------------|----------------|
| 0 | MaxHealth | Override | Scalable Float | 150.0 |
| 1 | Health | Override | Scalable Float | 150.0 |
| 2 | Armor | Override | Scalable Float | 20.0 |
| 3 | AttackDamage | Override | Scalable Float | 0.0 |

#### 1.5) Save Asset

### **2) Create GE_SupportHeal**

#### 2.1) Create Gameplay Effect
   - 2.1.1) Right-click in Content Browser
   - 2.1.2) Select Blueprint Class
   - 2.1.3) Search parent: GameplayEffect
   - 2.1.4) Select GameplayEffect
   - 2.1.5) Name: GE_SupportHeal
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
| Calculation Class | NarrativeHealExecution |

#### 2.5) Add Calculation Modifier for SetByCaller
   - 2.5.1) Expand Executions component
   - 2.5.2) Find Calculation Modifiers array
   - 2.5.3) Click + to add modifier
   - 2.5.4) Configure modifier:

| Property | Value |
|----------|-------|
| Backing Capture Definition -> Attribute | Heal |
| Modifier Op | Override |
| Modifier Magnitude -> Magnitude Calculation Type | Set By Caller |
| Modifier Magnitude -> Set By Caller Tag | SetByCaller.Heal |

#### 2.6) Add Asset Tag
   - 2.6.1) Find Components array
   - 2.6.2) Click + to add element
   - 2.6.3) Select: Tags This Effect Has (Asset Tags)
   - 2.6.4) Add tag: Effect.Support.Heal

#### 2.7) Add Gameplay Cues Component
   - 2.7.1) Find Components array
   - 2.7.2) Click + to add element
   - 2.7.3) Select: Gameplay Cues (Cues_GameplayEffectComponent)
   - 2.7.4) Expand component
   - 2.7.5) Add to Gameplay Cue Tags: GameplayCue.Support.Heal

#### 2.8) Save Asset

---

## PHASE 3: CREATE BP_SUPPORTBUFFER

### **1) Create Blueprint Class**

#### 1.1) Navigate to NPCs Folder
   - 1.1.1) Content Browser -> /Game/NPCs/Support/

#### 1.2) Create NPC Blueprint
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Blueprint Class
   - 1.2.3) Search parent: NarrativeNPCCharacter
   - 1.2.4) Select NarrativeNPCCharacter
   - 1.2.5) Name: BP_SupportBuffer
   - 1.2.6) Double-click to open

### **2) Create Variables**

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| HealRadius | Float | 500.0 | Yes |
| HealAmount | Float | 50.0 | Yes |
| HealCooldown | Float | 3.0 | Yes |
| SpeedBoostPercent | Float | 30.0 | Yes |
| SpeedBoostDuration | Float | 5.0 | Yes |
| bCanHeal | Boolean | true | No |
| BoostedActorSpeeds | Map (Actor -> Float) | - | No |

Note: BoostedActorSpeeds stores original MaxWalkSpeed values for restoration after boost expires.

### **3) Create ResetHealCooldown Function**

#### 3.1) Create Function
   - 3.1.1) In My Blueprint panel, click + next to Functions
   - 3.1.2) Name: ResetHealCooldown

#### 3.2) Implement Reset Logic
   - 3.2.1) Add SET bCanHeal node
   - 3.2.2) bCanHeal: Check (true)

### **4) Create RestoreSpeed Function**

#### 4.1) Create Function
   - 4.1.1) In My Blueprint panel, click + next to Functions
   - 4.1.2) Name: RestoreSpeed

#### 4.2) Add Input Parameter
   - 4.2.1) In Details panel, find Inputs
   - 4.2.2) Click + to add parameter
   - 4.2.3) Name: TargetActor
   - 4.2.4) Type: Actor (Object Reference)

#### 4.3) Check Map Contains Key
   - 4.3.1) From function entry:
      - 4.3.1.1) GET BoostedActorSpeeds variable
   - 4.3.2) From BoostedActorSpeeds:
      - 4.3.2.1) Add Contains node
      - 4.3.2.2) Key: TargetActor
   - 4.3.3) From Contains Return Value:
      - 4.3.3.1) Add Branch node
   - 4.3.4) From Branch False:
      - 4.3.4.1) Add Return Node

#### 4.4) Get Original Speed
   - 4.4.1) From Branch True execution:
      - 4.4.1.1) GET BoostedActorSpeeds variable
   - 4.4.2) From BoostedActorSpeeds:
      - 4.4.2.1) Add Find node
      - 4.4.2.2) Key: TargetActor

#### 4.5) Cast and Restore Speed
   - 4.5.1) From TargetActor:
      - 4.5.1.1) Add Cast To Character node
   - 4.5.2) From Cast successful:
      - 4.5.2.1) Add Get Character Movement node
   - 4.5.3) From Get Character Movement Return Value:
      - 4.5.3.1) Add Set Max Walk Speed node
      - 4.5.3.2) Max Walk Speed: Find result (original speed)

#### 4.6) Remove from Map
   - 4.6.1) From Set Max Walk Speed execution:
      - 4.6.1.1) GET BoostedActorSpeeds variable
   - 4.6.2) From BoostedActorSpeeds:
      - 4.6.2.1) Add Remove node
      - 4.6.2.2) Key: TargetActor

### **5) Create ApplyHealToTarget Function**

#### 5.1) Create Function
   - 5.1.1) In My Blueprint panel, click + next to Functions
   - 5.1.2) Name: ApplyHealToTarget

#### 5.2) Add Input Parameter
   - 5.2.1) In Details panel, find Inputs
   - 5.2.2) Click + to add parameter
   - 5.2.3) Name: TargetCharacter
   - 5.2.4) Type: Narrative Character (Object Reference)

#### 5.3) Add Authority Check
   - 5.3.1) Add Has Authority node
   - 5.3.2) From Has Authority Return Value:
      - 5.3.2.1) Add Branch node
   - 5.3.3) From Branch False:
      - 5.3.3.1) Add Return Node

#### 5.4) Check Cooldown
   - 5.4.1) From Branch True execution:
      - 5.4.1.1) Add GET bCanHeal node
   - 5.4.2) From bCanHeal:
      - 5.4.2.1) Add Branch node
   - 5.4.3) From Branch False:
      - 5.4.3.1) Add Return Node

#### 5.5) Set Cooldown
   - 5.5.1) From Branch True execution:
      - 5.5.1.1) Add SET bCanHeal node
      - 5.5.1.2) bCanHeal: Uncheck (false)
   - 5.5.2) From SET bCanHeal execution:
      - 5.5.2.1) Add Set Timer by Function Name node
      - 5.5.2.2) Object: Self
      - 5.5.2.3) Function Name: ResetHealCooldown
      - 5.5.2.4) Time: GET HealCooldown
      - 5.5.2.5) Looping: Uncheck (false)

#### 5.6) Get Target ASC
   - 5.6.1) From Set Timer execution:
      - 5.6.1.1) Add Get Ability System Component node
      - 5.6.1.2) Target: TargetCharacter
   - 5.6.2) From Get ASC Return Value:
      - 5.6.2.1) Add Is Valid node
   - 5.6.3) From Is Valid Is Not Valid pin:
      - 5.6.3.1) Add Return Node

#### 5.7) Get Source ASC
   - 5.7.1) From Is Valid Is Valid pin:
      - 5.7.1.1) Add Get Ability System Component node
      - 5.7.1.2) Target: Self

#### 5.8) Apply Heal Effect
   - 5.8.1) From Source ASC:
      - 5.8.1.1) Add Make Outgoing Spec node
      - 5.8.1.2) Gameplay Effect Class: GE_SupportHeal
      - 5.8.1.3) Level: 1.0
   - 5.8.2) From Make Outgoing Spec Return Value:
      - 5.8.2.1) Add Assign Tag Set By Caller Magnitude node
      - 5.8.2.2) Spec Handle: Outgoing Spec result
      - 5.8.2.3) Data Tag: SetByCaller.Heal
      - 5.8.2.4) Magnitude: GET HealAmount
   - 5.8.3) From Assign Tag execution:
      - 5.8.3.1) Add Apply Gameplay Effect Spec to Self node
      - 5.8.3.2) Target: Target ASC (TargetCharacter's ASC)
      - 5.8.3.3) Spec Handle: Assigned spec

#### 5.9) Get Current Speed
   - 5.9.1) From Apply GE execution:
      - 5.9.1.1) Add Get Character Movement node
      - 5.9.1.2) Target: TargetCharacter
   - 5.9.2) From Get Character Movement Return Value:
      - 5.9.2.1) Add Get Max Walk Speed node

#### 5.10) Store Original Speed (if not already boosted)
   - 5.10.1) GET BoostedActorSpeeds variable
   - 5.10.2) From BoostedActorSpeeds:
      - 5.10.2.1) Add Contains node
      - 5.10.2.2) Key: TargetCharacter
   - 5.10.3) From Contains Return Value:
      - 5.10.3.1) Add Branch node
   - 5.10.4) From Branch False (not already boosted):
      - 5.10.4.1) GET BoostedActorSpeeds variable
      - 5.10.4.2) Add Add node
      - 5.10.4.3) Key: TargetCharacter
      - 5.10.4.4) Value: Get Max Walk Speed result

Note: Only store original speed if not already in map (prevents overwriting original with boosted value)

#### 5.11) Calculate Speed Boost Value
   - 5.11.1) From Get Max Walk Speed Return Value:
      - 5.11.1.1) Add Multiply (Float * Float) node
      - 5.11.1.2) A: Max Walk Speed
      - 5.11.1.3) B: GET SpeedBoostPercent
   - 5.11.2) From Multiply Return Value:
      - 5.11.2.1) Add Divide (Float / Float) node
      - 5.11.2.2) A: Multiply result
      - 5.11.2.3) B: 100.0

#### 5.12) Calculate New Speed
   - 5.12.1) From Divide result:
      - 5.12.1.1) Add Add (Float + Float) node
      - 5.12.1.2) A: Get Max Walk Speed (current speed)
      - 5.12.1.3) B: Divide result (boost amount)

#### 5.13) Apply Speed Boost Directly
   - 5.13.1) From Get Character Movement Return Value:
      - 5.13.1.1) Add Set Max Walk Speed node
      - 5.13.1.2) Max Walk Speed: Add result (boosted speed)

Note: MovementSpeed is NOT an attribute - must modify CharacterMovement component directly per tech reference.

#### 5.14) Set Restore Timer
   - 5.14.1) From Set Max Walk Speed execution:
      - 5.14.1.1) Add Set Timer by Event node
      - 5.14.1.2) Time: GET SpeedBoostDuration
      - 5.14.1.3) Looping: Uncheck (false)
   - 5.14.2) Create Custom Event:
      - 5.14.2.1) Name: OnSpeedBoostExpired
      - 5.14.2.2) Add input: ExpiredActor (Actor)
   - 5.14.3) Bind OnSpeedBoostExpired to timer delegate
      - 5.14.3.1) Pass TargetCharacter as ExpiredActor

#### 5.15) Create OnSpeedBoostExpired Event
   - 5.15.1) In Event Graph, create Custom Event: OnSpeedBoostExpired
   - 5.15.2) Add input parameter: ExpiredActor (Actor)
   - 5.15.3) From OnSpeedBoostExpired:
      - 5.15.3.1) Add RestoreSpeed node
      - 5.15.3.2) TargetActor: ExpiredActor

### **6) Compile and Save**

---

## PHASE 4: CREATE BTS_HEALNEARBYALLIES SERVICE

### **1) Create Service Blueprint**

#### 1.1) Navigate to Services Folder
   - 1.1.1) Content Browser -> /Game/AI/Services/

#### 1.2) Create Blueprint
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Blueprint Class
   - 1.2.3) Search parent: BTService_BlueprintBase
   - 1.2.4) Select BTService_BlueprintBase
   - 1.2.5) Name: BTS_HealNearbyAllies
   - 1.2.6) Double-click to open

### **2) Create Variables**

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| HealRadius | Float | 500.0 | Yes |
| DamageThreshold | Float | 0.9 | Yes |

### **3) Implement Receive Tick AI**

#### 3.1) Add Event Node
   - 3.1.1) Right-click in Event Graph
   - 3.1.2) Search: Receive Tick AI
   - 3.1.3) Select Event Receive Tick AI

#### 3.2) Get Controlled Pawn
   - 3.2.1) From Event Receive Tick AI execution:
      - 3.2.1.1) Add Get Controlled Pawn node
      - 3.2.1.2) Target: Owner Controller

#### 3.3) Cast to BP_SupportBuffer
   - 3.3.1) From Get Controlled Pawn Return Value:
      - 3.3.1.1) Add Cast To BP_SupportBuffer node
   - 3.3.2) From Cast failed:
      - 3.3.2.1) Do nothing (return)

#### 3.4) Get Support Location
   - 3.4.1) From Cast successful execution:
      - 3.4.1.1) Add Get Actor Location node
      - 3.4.1.2) Target: As BP Support Buffer

#### 3.5) Sphere Overlap for Pawns
   - 3.5.1) From Get Actor Location execution:
      - 3.5.1.1) Add Sphere Overlap Actors node
   - 3.5.2) Configure Sphere Overlap:
      - 3.5.2.1) Sphere Pos: Actor Location result
      - 3.5.2.2) Sphere Radius: GET HealRadius
      - 3.5.2.3) Object Types: Click array, add Pawn
      - 3.5.2.4) Actors to Ignore: Add As BP Support Buffer (Self)

#### 3.6) Loop Through Overlapped Actors
   - 3.6.1) From Sphere Overlap Out Actors:
      - 3.6.1.1) Add For Each Loop node

#### 3.7) Cast to NarrativeCharacter
   - 3.7.1) From For Each Loop Body:
      - 3.7.1.1) Add Cast To NarrativeCharacter node
      - 3.7.1.2) Object: Array Element

#### 3.8) Check if Friendly
   - 3.8.1) From Cast successful execution:
      - 3.8.1.1) Add Get Attitude (ArsenalStatics) node
      - 3.8.1.2) Source: As BP Support Buffer
      - 3.8.1.3) Target: As Narrative Character
   - 3.8.2) From Get Attitude Return Value:
      - 3.8.2.1) Add Equal (Enum) node
      - 3.8.2.2) Compare to: Friendly
   - 3.8.3) From Equal Return Value:
      - 3.8.3.1) Add Branch node
   - 3.8.4) From Branch False:
      - 3.8.4.1) Do nothing (continue loop)

#### 3.9) Get Health Attributes
   - 3.9.1) From Branch True execution:
      - 3.9.1.1) Add Get Ability System Component node
      - 3.9.1.2) Target: As Narrative Character
   - 3.9.2) From Get ASC Return Value:
      - 3.9.2.1) Add Get Numeric Attribute node
      - 3.9.2.2) Attribute: NarrativeAttributeSetBase.Health
   - 3.9.3) From Get ASC Return Value:
      - 3.9.3.1) Add Get Numeric Attribute node
      - 3.9.3.2) Attribute: NarrativeAttributeSetBase.MaxHealth

#### 3.10) Calculate Health Ratio
   - 3.10.1) From Health value:
      - 3.10.1.1) Add Divide (Float / Float) node
      - 3.10.1.2) A: Health value
      - 3.10.1.3) B: MaxHealth value

#### 3.11) Check if Damaged
   - 3.11.1) From Divide Return Value:
      - 3.11.1.1) Add Less (Float < Float) node
      - 3.11.1.2) A: Health ratio (Divide result)
      - 3.11.1.3) B: GET DamageThreshold
   - 3.11.2) From Less Return Value:
      - 3.11.2.1) Add Branch node

#### 3.12) Apply Heal
   - 3.12.1) From Branch True execution:
      - 3.12.1.1) Add Apply Heal To Target node
      - 3.12.1.2) Target: As BP Support Buffer
      - 3.12.1.3) Target Character: As Narrative Character
   - 3.12.2) From Apply Heal To Target execution:
      - 3.12.2.1) Add Break node

Note: Break exits the loop after healing one target per tick

### **4) Configure Service Settings**

#### 4.1) Click Class Defaults

#### 4.2) Set Service Properties
   - 4.2.1) Interval: 0.5
   - 4.2.2) Random Deviation: 0.1

### **5) Compile and Save**

---

## PHASE 5: CREATE BT_SUPPORTFOLLOW BEHAVIOR TREE

### **1) Create Behavior Tree**

#### 1.1) Navigate to BehaviorTrees Folder
   - 1.1.1) Content Browser -> /Game/AI/BehaviorTrees/

#### 1.2) Create Behavior Tree
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Artificial Intelligence -> Behavior Tree
   - 1.2.3) Name: BT_SupportFollow
   - 1.2.4) Double-click to open

### **2) Set Blackboard**

#### 2.1) Configure Blackboard Asset
   - 2.1.1) In Details panel, find Blackboard Asset
   - 2.1.2) Select: BB_FollowCharacter

Note: Reusing existing Narrative Pro blackboard with FollowTarget and FollowDistance keys

### **3) Create Tree Structure**

#### 3.1) Add Selector to Root
   - 3.1.1) Drag from Root node
   - 3.1.2) Add Selector node

#### 3.2) Add Sequence to Selector
   - 3.2.1) Drag from Selector
   - 3.2.2) Add Sequence node

#### 3.3) Attach Heal Service to Sequence
   - 3.3.1) Right-click on Sequence node
   - 3.3.2) Select Add Service -> BTS_HealNearbyAllies
   - 3.3.3) Configure service:
      - 3.3.3.1) Heal Radius: 500.0
      - 3.3.3.2) Damage Threshold: 0.9

#### 3.4) Add Focus Service to Sequence
   - 3.4.1) Right-click on Sequence node
   - 3.4.2) Select Add Service -> BTS_SetAIFocus
   - 3.4.3) Configure:
      - 3.4.3.1) Focus Target Key: FollowTarget

#### 3.5) Add Speed Matching Service
   - 3.5.1) Right-click on Sequence node
   - 3.5.2) Select Add Service -> BTS_AdjustFollowSpeed

Note: Reusing existing Narrative Pro services for follow behavior

#### 3.6) Add Move To Task
   - 3.6.1) Drag from Sequence
   - 3.6.2) Add Move To node
   - 3.6.3) Configure:
      - 3.6.3.1) Blackboard Key: FollowTarget
      - 3.6.3.2) Acceptable Radius: 200.0

### **4) Save Behavior Tree**

---

## PHASE 6: CREATE BPA_SUPPORTFOLLOW ACTIVITY

### **1) Create Activity Blueprint**

#### 1.1) Navigate to Activities Folder
   - 1.1.1) Content Browser -> /Game/AI/Activities/

#### 1.2) Create Blueprint
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Blueprint Class
   - 1.2.3) Search parent: BPA_FollowCharacter
   - 1.2.4) Select BPA_FollowCharacter
   - 1.2.5) Name: BPA_SupportFollow
   - 1.2.6) Double-click to open

Note: Inheriting from BPA_FollowCharacter provides SetupBlackboard functionality

### **2) Override Behaviour Tree**

#### 2.1) Click Class Defaults

#### 2.2) Set Behaviour Tree
   - 2.2.1) Find Behaviour Tree property
   - 2.2.2) Select: BT_SupportFollow

### **3) Compile and Save**

---

## PHASE 7: CREATE ABILITY CONFIGURATION

### **1) Create Data Asset**

#### 1.1) Navigate to Configurations Folder
   - 1.1.1) Content Browser -> /Game/NPCs/Support/Configurations/

#### 1.2) Create Data Asset
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Miscellaneous -> Data Asset
   - 1.2.3) Select: AbilityConfiguration
   - 1.2.4) Name: AC_SupportBuffer
   - 1.2.5) Double-click to open

### **2) Configure Attribute Effect**

#### 2.1) Find Attribute Gameplay Effect
   - 2.1.1) Set to: GE_SupporterAttributes

### **3) Configure AI Perception**

#### 3.1) Find Sight Configuration
   - 3.1.1) Sight Radius: 1500.0
   - 3.1.2) Lose Sight Radius: 2000.0
   - 3.1.3) Peripheral Vision Half Angle: 90.0

### **4) Save Data Asset**

---

## PHASE 8: CREATE ACTIVITY CONFIGURATION

### **1) Create Data Asset**

#### 1.1) Navigate to Configurations Folder
   - 1.1.1) Content Browser -> /Game/NPCs/Support/Configurations/

#### 1.2) Create Data Asset
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Miscellaneous -> Data Asset
   - 1.2.3) Select: ActivityConfiguration
   - 1.2.4) Name: AC_SupportBufferBehavior
   - 1.2.5) Double-click to open

### **2) Configure Activities**

#### 2.1) Add Activities
   - 2.1.1) Find Activities array
   - 2.1.2) Click + to add element
   - 2.1.3) Element 0: BPA_SupportFollow
   - 2.1.4) Click + to add element
   - 2.1.5) Element 1: BPA_Idle

Note: NO attack activities - this is a non-combat support NPC

### **3) Configure Goal Generators**

#### 3.1) Leave Empty
   - 3.1.1) Do NOT add GoalGenerator_Attack

Note: Support NPC does not attack, so no attack goal generator

### **4) Save Data Asset**

---

## PHASE 9: CREATE NPCDEFINITION

### **1) Create Data Asset**

#### 1.1) Navigate to Definitions Folder
   - 1.1.1) Content Browser -> /Game/NPCs/Support/Definitions/

#### 1.2) Create Data Asset
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select Miscellaneous -> Data Asset
   - 1.2.3) Select: NPCDefinition
   - 1.2.4) Name: NPC_SupportBuffer
   - 1.2.5) Double-click to open

### **2) Configure Basic Properties**

#### 2.1) Set NPC Class
   - 2.1.1) Find NPC Class property
   - 2.1.2) Select: BP_SupportBuffer

#### 2.2) Set Display Name
   - 2.2.1) Find Display Name property
   - 2.2.2) Enter: Support Buffer

### **3) Configure Ability Configuration**

#### 3.1) Set Ability Configuration
   - 3.1.1) Find Ability Configuration property
   - 3.1.2) Select: AC_SupportBuffer

### **4) Configure Activity Configuration**

#### 4.1) Set Activity Configuration
   - 4.1.1) Find Activity Configuration property
   - 4.1.2) Select: AC_SupportBufferBehavior

### **5) Configure Faction**

#### 5.1) Set Faction Tags
   - 5.1.1) Find Faction Tags property
   - 5.1.2) Add tag: Faction.Friendly.Support

### **6) Save Data Asset**

---

## PHASE 10: CONFIGURE FACTION ATTITUDES

### **1) Open Faction Settings**

#### 1.1) Navigate to Project Settings
   - 1.1.1) Edit -> Project Settings
   - 1.1.2) Navigate to Game -> Narrative Combat

### **2) Configure Faction Attitudes**

#### 2.1) Add Faction Relationship
   - 2.1.1) Find Faction Attitudes array
   - 2.1.2) Add entry for Faction.Friendly.Support

#### 2.2) Set Friendly Factions
   - 2.2.1) Faction.Friendly.Support is Friendly to:
      - 2.2.1.1) Narrative.Factions.Heroes
      - 2.2.1.2) Faction.Friendly.Support

#### 2.3) Set Hostile Factions
   - 2.3.1) Faction.Friendly.Support is Hostile to:
      - 2.3.1.1) Narrative.Factions.Bandits
      - 2.3.1.2) Any enemy faction tags

### **3) Save Settings**

---

## PHASE 11: WORLD PLACEMENT AND GOAL ASSIGNMENT

### **1) Place via NPCSpawner**

#### 1.1) Add NPCSpawner to Level
   - 1.1.1) Place NPCSpawner actor in level
   - 1.1.2) Configure NPCSpawner:
      - 1.1.2.1) NPC Definition: NPC_SupportBuffer
      - 1.1.2.2) Spawn on Begin Play: Check (true)

### **2) Assign Follow Goal**

#### 2.1) Via Dialogue or Blueprint
   - 2.1.1) Get reference to BP_SupportBuffer
   - 2.1.2) Add Add Goal (NarrativeCharacter) node
   - 2.1.3) Goal Class: Goal_FollowCharacter
   - 2.1.4) Priority: 1
   - 2.1.5) From Return Value (Goal):
      - 2.1.5.1) Cast To Goal_FollowCharacter
   - 2.1.6) From Cast successful:
      - 2.1.6.1) Add Set Target To Follow node
      - 2.1.6.2) Target: Player character reference

### **3) Alternative: Quest Task Assignment**

#### 3.1) Via Quest Graph
   - 3.1.1) Use Quest Action to assign Goal_FollowCharacter
   - 3.1.2) Reference NPC via NPC Definition

---

## CHANGELOG

### Version 1.1 - January 2026

| Change | Description |
|--------|-------------|
| Naming Conventions | Updated to Narrative Pro v2.2: ActConfig_ → AC_*Behavior suffix (AC_SupportBufferBehavior), NPCDef_ → NPC_* prefix (NPC_SupportBuffer). |

### Version 1.0 - January 2026

| Change | Description |
|--------|-------------|
| Initial Release | Complete Support Buffer Healer NPC implementation |
| Healing System | Uses NarrativeHealExecution with SetByCaller.Heal |
| Speed Boost | Direct CharacterMovement modification (MaxWalkSpeed is NOT an attribute) |
| Faction Filter | Uses ArsenalStatics::GetAttitude for friendly detection |
| Follow System | Inherits from BPA_FollowCharacter, reuses BB_FollowCharacter |
| Non-Combat | ActConfig has NO attack activities or GoalGenerator_Attack |

---

**END OF GUIDE**
