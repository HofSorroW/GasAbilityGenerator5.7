# Random Aggression Stalker System Implementation Guide
## Passive NPCs with Stacking Bond, Tagged Dialogue, and Defend Mechanics
## Version 2.3

---

## DOCUMENT INFORMATION

| Field | Value |
|-------|-------|
| Document Type | Implementation Guide |
| System | Random Aggression Stalker System |
| Last Updated | January 2026 |
| Unreal Engine | 5.7 |
| Narrative Pro | v2.2 |

---

## TABLE OF CONTENTS

| Phase | Name |
|-------|------|
| PHASE 1 | VERIFY GAMEPLAY TAGS |
| PHASE 2 | CREATE TAGGEDDIALOGUESET ASSET |
| PHASE 3 | CREATE GOALGENERATOR_RANDOMAGGRESSION |
| PHASE 4 | IMPLEMENT INITIALIZE AND FOLLOW ROLL |
| PHASE 5 | IMPLEMENT TALK ROLL AND STACKING BOND |
| PHASE 6 | IMPLEMENT DEFEND PLAYER MECHANIC |
| PHASE 7 | IMPLEMENT ATTACK ROLL LOGIC |
| PHASE 8 | IMPLEMENT TIMEOUT AND CLEANUP |
| PHASE 9 | CREATE HELPER FUNCTIONS |
| PHASE 10 | CREATE ACTIVITYCONFIGURATION |
| PHASE 11 | CREATE NPCDEFINITION |
| PHASE 12 | WORLD PLACEMENT |

---

## INTRODUCTION

### System Overview

| Aspect | Description |
|--------|-------------|
| Concept | Passive NPCs that randomly follow, bond through dialogue, and may attack or defend |
| Default State | Normal patrol behavior |
| Follow Trigger | Random timer roll starts following player |
| Bonding | Periodic talk rolls increase bond, modifying behavior |
| Defend Trigger | Player takes damage while being followed |
| Attack Trigger | Random roll during follow escalates to combat |
| Timeout | If no attack roll succeeds, stops following and returns to patrol |
| Post-Attack | Stays aggressive permanently |
| Post-Defend | Returns to following player |

### Stacking Bond System

| Talk Count | Follow Duration | Attack Player Chance | Defend Player Chance |
|------------|-----------------|----------------------|----------------------|
| 0 (Silent) | 20s | 25% | 50% |
| 1 | 25s | 20% | 55% |
| 2 | 30s | 15% | 60% |
| 3 | 35s | 10% | 65% |
| 4 | 40s | 5% | 70% |
| 5+ (Max) | 45s | 2.5% | 75% |

### Behavior States

| State | Activity | Score | Trigger to Exit |
|-------|----------|-------|-----------------|
| Patrol | BPA_Patrol | 1.0 | FollowChance roll succeeds |
| Following | BPA_FollowCharacter | 2.0 | Attack/Timeout/Defend trigger |
| Defending | BPA_Attack_Melee | 3.0 | Target killed -> Return to Following |
| Aggressive | BPA_Attack_Melee | 3.0 | Permanent (attacks player) |

### System Flow

| Step | Component | Action |
|------|-----------|--------|
| 1 | GoalGenerator | FollowCheckTimer fires |
| 2 | GoalGenerator | Roll FollowChance, find player in range |
| 3 | GoalGenerator | Create Goal_FollowCharacter, play StartFollowing dialogue |
| 4 | GoalGenerator | Bind to player OnDamaged delegate |
| 5 | BPA_FollowCharacter | NPC follows player |
| 6 | GoalGenerator | TalkCheckTimer fires, roll TalkChance |
| 7a | Talk succeeds | Increment TalkCount, play dialogue, recalculate stats |
| 7b | Talk fails | Continue following silently |
| 8 | Player damaged | Roll DefendChance (based on TalkCount) |
| 9a | Defend succeeds | Attack the attacker, play Defending dialogue |
| 9b | Defend fails | Continue following |
| 10 | GoalGenerator | AttackCheckTimer fires, roll AttackChance (reduced by TalkCount) |
| 11a | Attack succeeds | Attack player, play Attacking dialogue, permanent aggro |
| 11b | Attack fails | Continue following |
| 12 | FollowDuration expires | Play GivingUp dialogue, return to patrol |
| 13 | Defender kills target | Resume following player |

### Design Philosophy

| Principle | Implementation |
|-----------|----------------|
| Unpredictability | Multiple random rolls create tension |
| Emergent Bonding | Talking increases protection, reduces threat |
| Betrayal Possibility | Even bonded NPCs can attack (low chance) |
| Narrative Integration | Tagged dialogue for all outcomes |
| Field Chaos | Multiple NPCs create dynamic ally/threat ecosystem |

---

## QUICK REFERENCE

### Gameplay Tags

| Tag | Purpose |
|-----|---------|
| State.NPC.Returned.Following | Applied during follow state |
| State.NPC.Returned.Bonded | Applied when TalkCount > 0 |
| State.NPC.Returned.Defending | Applied while defending player |
| State.NPC.Returned.Aggressive | Applied after attacking player |
| Narrative.TaggedDialogue.Returned.StartFollowing | When follow begins |
| Narrative.TaggedDialogue.Returned.Following.First | First talk (TalkCount = 1) |
| Narrative.TaggedDialogue.Returned.Following.Bonding | TalkCount 2-3 |
| Narrative.TaggedDialogue.Returned.Following.Deep | TalkCount 4+ |
| Narrative.TaggedDialogue.Returned.Defending | When defending player |
| Narrative.TaggedDialogue.Returned.Attacking | When attacking player |
| Narrative.TaggedDialogue.Returned.GivingUp | When timeout expires |

### Assets Created

| Asset | Type | Location |
|-------|------|----------|
| GoalGenerator_RandomAggression | NPCGoalGenerator | /Game/AI/GoalGenerators/ |
| TaggedDialogueSet_Returned | TaggedDialogueSet | /Game/Dialogue/TaggedSets/ |
| AC_ReturnedStalkerBehavior | ActivityConfiguration | /Game/AI/Configurations/ |
| NPC_ReturnedStalker | NPCDefinition | /Game/NPCs/Returned/ |

### Existing Assets Reused

| Asset | Type | Purpose |
|-------|------|---------|
| Goal_Patrol | NPCGoalItem | Default patrol goal |
| BPA_Patrol | NPCActivity | Default patrol behavior |
| BT_Patrol | BehaviorTree | Patrol logic |
| Goal_FollowCharacter | NPCGoalItem | Follow player goal |
| BPA_FollowCharacter | NPCActivity | Follow behavior |
| BT_FollowCharacter | BehaviorTree | Follow logic |
| Goal_Attack | NPCGoalItem | Attack goal |
| BPA_Attack_Melee | NPCActivity | Melee combat |
| GoalGenerator_Attack | NPCGoalGenerator | Maintains combat state |

### GoalGenerator Variables - Configuration

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| FollowCheckInterval | Float | 8.0 | Yes |
| FollowChance | Float | 0.15 | Yes |
| DetectionRange | Float | 2000.0 | Yes |
| FollowDistance | Float | 400.0 | Yes |
| BaseFollowDuration | Float | 20.0 | Yes |
| FollowDurationPerStack | Float | 5.0 | Yes |
| TalkCheckInterval | Float | 8.0 | Yes |
| TalkChance | Float | 0.3 | Yes |
| MaxTalkStacks | Integer | 5 | Yes |
| AttackCheckInterval | Float | 3.0 | Yes |
| BaseAttackChance | Float | 0.25 | Yes |
| AttackReductionPerStack | Float | 0.05 | Yes |
| MinAttackChance | Float | 0.025 | Yes |
| BaseDefendChance | Float | 0.5 | Yes |
| DefendBonusPerStack | Float | 0.05 | Yes |
| MaxDefendChance | Float | 0.75 | Yes |
| DefendRange | Float | 1500.0 | Yes |

### GoalGenerator Variables - Runtime State

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| CurrentFollowGoal | Goal_FollowCharacter | None | No |
| CurrentDefendGoal | Goal_Attack | None | No |
| FollowStartTime | Float | 0.0 | No |
| TalkCount | Integer | 0 | No |
| bIsFollowing | Boolean | false | No |
| bIsDefending | Boolean | false | No |
| CachedPlayerCharacter | NarrativePlayerCharacter | None | No |

### GoalGenerator Variables - Timer Handles

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| FollowCheckTimerHandle | Timer Handle | None | No |
| TalkCheckTimerHandle | Timer Handle | None | No |
| AttackCheckTimerHandle | Timer Handle | None | No |
| FollowDurationTimerHandle | Timer Handle | None | No |

### GoalGenerator Variables - Tagged Dialogue Tags

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| Tag_StartFollowing | GameplayTag | Narrative.TaggedDialogue.Returned.StartFollowing | Yes |
| Tag_FollowFirst | GameplayTag | Narrative.TaggedDialogue.Returned.Following.First | Yes |
| Tag_FollowBonding | GameplayTag | Narrative.TaggedDialogue.Returned.Following.Bonding | Yes |
| Tag_FollowDeep | GameplayTag | Narrative.TaggedDialogue.Returned.Following.Deep | Yes |
| Tag_Defending | GameplayTag | Narrative.TaggedDialogue.Returned.Defending | Yes |
| Tag_Attacking | GameplayTag | Narrative.TaggedDialogue.Returned.Attacking | Yes |
| Tag_GivingUp | GameplayTag | Narrative.TaggedDialogue.Returned.GivingUp | Yes |

---

## PHASE 1: VERIFY GAMEPLAY TAGS

### State Tags

| Tag | Purpose |
|-----|---------|
| State.NPC.Returned.Following | Applied during follow state |
| State.NPC.Returned.Bonded | Applied when TalkCount > 0 |
| State.NPC.Returned.Defending | Applied while defending player |
| State.NPC.Returned.Aggressive | Applied after attacking player |

### Tagged Dialogue Tags

| Tag | Purpose |
|-----|---------|
| Narrative.TaggedDialogue.Returned.StartFollowing | When follow begins |
| Narrative.TaggedDialogue.Returned.Following.First | First talk |
| Narrative.TaggedDialogue.Returned.Following.Bonding | Bonding talks |
| Narrative.TaggedDialogue.Returned.Following.Deep | Deep bond talks |
| Narrative.TaggedDialogue.Returned.Defending | Defending player |
| Narrative.TaggedDialogue.Returned.Attacking | Attacking player |
| Narrative.TaggedDialogue.Returned.GivingUp | Timeout reached |

---

## PHASE 2: CREATE TAGGEDDIALOGUESET ASSET

### **1) Create Folder Structure**

#### 1.1) Navigate to Content Browser
   - 1.1.1) Content Browser -> /Game/Dialogue/
   - 1.1.2) Create folder: TaggedSets
   - 1.1.3) Open TaggedSets folder

### **2) Create TaggedDialogueSet**

#### 2.1) Create Data Asset
   - 2.1.1) Right-click in Content Browser
   - 2.1.2) Select: Miscellaneous -> Data Asset
   - 2.1.3) Select Class: TaggedDialogueSet
   - 2.1.4) Name: TaggedDialogueSet_Returned
   - 2.1.5) Double-click to open

### **3) Configure Tagged Dialogues Array**

#### 3.1) Add Tagged Dialogue Entries

| Index | Tag | Dialogue | Cooldown | Max Distance |
|-------|-----|----------|----------|--------------|
| 0 | Narrative.TaggedDialogue.Returned.StartFollowing | DBP_Returned_StartFollow | 30.0 | 5000.0 |
| 1 | Narrative.TaggedDialogue.Returned.Following.First | DBP_Returned_First | 30.0 | 5000.0 |
| 2 | Narrative.TaggedDialogue.Returned.Following.Bonding | DBP_Returned_Bonding | 30.0 | 5000.0 |
| 3 | Narrative.TaggedDialogue.Returned.Following.Deep | DBP_Returned_Deep | 30.0 | 5000.0 |
| 4 | Narrative.TaggedDialogue.Returned.Defending | DBP_Returned_Defending | 5.0 | 5000.0 |
| 5 | Narrative.TaggedDialogue.Returned.Attacking | DBP_Returned_Attacking | 5.0 | 5000.0 |
| 6 | Narrative.TaggedDialogue.Returned.GivingUp | DBP_Returned_GivingUp | 30.0 | 5000.0 |

### **4) Save Asset**
   - 4.1) Click Save

---

## PHASE 3: CREATE GOALGENERATOR_RANDOMAGGRESSION

### **5) Create Folder Structure**

#### 5.1) Navigate to Content Browser
   - 5.1.1) Content Browser -> /Game/AI/
   - 5.1.2) Create folder: GoalGenerators
   - 5.1.3) Open GoalGenerators folder

### **6) Create GoalGenerator Blueprint**

#### 6.1) Create NPCGoalGenerator Blueprint
   - 6.1.1) Right-click in Content Browser
   - 6.1.2) Select: Blueprint Class
   - 6.1.3) Click All Classes dropdown
   - 6.1.4) Search: NPCGoalGenerator
   - 6.1.5) Select: NPCGoalGenerator
   - 6.1.6) Click: Select button
   - 6.1.7) Name: GoalGenerator_RandomAggression
   - 6.1.8) Double-click to open

### **7) Create Configuration Variables**

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| FollowCheckInterval | Float | 8.0 | Yes |
| FollowChance | Float | 0.15 | Yes |
| DetectionRange | Float | 2000.0 | Yes |
| FollowDistance | Float | 400.0 | Yes |
| BaseFollowDuration | Float | 20.0 | Yes |
| FollowDurationPerStack | Float | 5.0 | Yes |
| TalkCheckInterval | Float | 8.0 | Yes |
| TalkChance | Float | 0.3 | Yes |
| MaxTalkStacks | Integer | 5 | Yes |
| AttackCheckInterval | Float | 3.0 | Yes |
| BaseAttackChance | Float | 0.25 | Yes |
| AttackReductionPerStack | Float | 0.05 | Yes |
| MinAttackChance | Float | 0.025 | Yes |
| BaseDefendChance | Float | 0.5 | Yes |
| DefendBonusPerStack | Float | 0.05 | Yes |
| MaxDefendChance | Float | 0.75 | Yes |
| DefendRange | Float | 1500.0 | Yes |

### **8) Create Runtime State Variables**

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| CurrentFollowGoal | Goal_FollowCharacter (Object Reference) | None | No |
| CurrentDefendGoal | Goal_Attack (Object Reference) | None | No |
| FollowStartTime | Float | 0.0 | No |
| TalkCount | Integer | 0 | No |
| bIsFollowing | Boolean | false | No |
| bIsDefending | Boolean | false | No |
| CachedPlayerCharacter | NarrativePlayerCharacter (Object Reference) | None | No |

### **9) Create Timer Handle Variables**

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| FollowCheckTimerHandle | Timer Handle | None | No |
| TalkCheckTimerHandle | Timer Handle | None | No |
| AttackCheckTimerHandle | Timer Handle | None | No |
| FollowDurationTimerHandle | Timer Handle | None | No |

### **10) Create Tagged Dialogue Tag Variables**

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| Tag_StartFollowing | GameplayTag | Narrative.TaggedDialogue.Returned.StartFollowing | Yes |
| Tag_FollowFirst | GameplayTag | Narrative.TaggedDialogue.Returned.Following.First | Yes |
| Tag_FollowBonding | GameplayTag | Narrative.TaggedDialogue.Returned.Following.Bonding | Yes |
| Tag_FollowDeep | GameplayTag | Narrative.TaggedDialogue.Returned.Following.Deep | Yes |
| Tag_Defending | GameplayTag | Narrative.TaggedDialogue.Returned.Defending | Yes |
| Tag_Attacking | GameplayTag | Narrative.TaggedDialogue.Returned.Attacking | Yes |
| Tag_GivingUp | GameplayTag | Narrative.TaggedDialogue.Returned.GivingUp | Yes |

### **11) Compile and Save**
   - 11.1) Click Compile
   - 11.2) Click Save

---

## PHASE 4: IMPLEMENT INITIALIZE AND FOLLOW ROLL

### **12) Override InitializeGoalGenerator**

#### 12.1) Find InitializeGoalGenerator
   - 12.1.1) In My Blueprint panel, find Functions section
   - 12.1.2) Click Override dropdown
   - 12.1.3) Select: Initialize Goal Generator

#### 12.2) Start Follow Check Timer
   - 12.2.1) From Initialize Goal Generator entry node:
   - 12.2.1.1) Add Set Timer by Event node
   - 12.2.2) Configure timer:
   - 12.2.2.1) Time: Connect FollowCheckInterval variable
   - 12.2.2.2) Looping: Check (true)
   - 12.2.3) From Return Value:
   - 12.2.3.1) Add SET FollowCheckTimerHandle node

#### 12.3) Create Follow Check Event
   - 12.3.1) From Set Timer by Event Red delegate pin:
   - 12.3.1.1) Drag and release in empty space
   - 12.3.1.2) Select: Add Custom Event
   - 12.3.1.3) Name: OnFollowCheckTimer

### **13) Implement OnFollowCheckTimer**

#### 13.1) Check Not Already Following
   - 13.1.1) From OnFollowCheckTimer event:
   - 13.1.1.1) Drag bIsFollowing variable getter
   - 13.1.1.2) Add NOT Boolean node
   - 13.1.1.3) Connect bIsFollowing to NOT input
   - 13.1.2) Add Branch node
   - 13.1.3) Connect NOT result to Condition

### **14) Roll Follow Chance**

#### 14.1) Generate Random Roll
   - 14.1.1) From Branch True pin:
   - 14.1.1.1) Add Random Float in Range node
   - 14.1.1.2) Min: 0.0
   - 14.1.1.3) Max: 1.0

#### 14.2) Compare to Follow Chance
   - 14.2.1) Add Less Than or Equal (Float) node
   - 14.2.2) Connect:
   - 14.2.2.1) A: Random Float Return Value
   - 14.2.2.2) B: FollowChance variable
   - 14.2.3) Add Branch node
   - 14.2.4) Connect comparison result to Condition

### **15) Find Player Character**

#### 15.1) Get Player Character
   - 15.1.1) From Branch True pin:
   - 15.1.1.1) Add Get Player Character node
   - 15.1.1.2) Player Index: 0
   - 15.1.2) Add Cast to NarrativePlayerCharacter node
   - 15.1.3) Connect Get Player Character Return Value to Object

#### 15.2) Validate Cast
   - 15.2.1) From Cast failed execution:
   - 15.2.1.1) Leave unconnected (exits function)

### **16) Check Player in Range**

#### 16.1) Get Distance to Player
   - 16.1.1) From Cast successful execution:
   - 16.1.1.1) Drag from OwnerController variable (inherited)
   - 16.1.2) From OwnerController:
   - 16.1.2.1) Add Get Controlled Pawn node
   - 16.1.3) From Controlled Pawn:
   - 16.1.3.1) Add Get Distance To node
   - 16.1.3.2) Other Actor: Connect As NarrativePlayerCharacter

#### 16.2) Compare to Detection Range
   - 16.2.1) Add Less Than or Equal (Float) node
   - 16.2.2) Connect:
   - 16.2.2.1) A: Get Distance To Return Value
   - 16.2.2.2) B: DetectionRange variable
   - 16.2.3) Add Branch node
   - 16.2.4) Connect comparison result to Condition

### **17) Cache Player Reference**

#### 17.1) Store Player Reference
   - 17.1.1) From Branch True pin:
   - 17.1.1.1) Add SET CachedPlayerCharacter node
   - 17.1.1.2) Connect As NarrativePlayerCharacter to value

### **18) Create Goal_FollowCharacter**

#### 18.1) Construct Follow Goal
   - 18.1.1) From SET CachedPlayerCharacter execution:
   - 18.1.1.1) Add Construct Object from Class node
   - 18.1.1.2) Class: Goal_FollowCharacter
   - 18.1.1.3) Outer: Self

#### 18.2) Configure Follow Goal
   - 18.2.1) From Construct Object Return Value:
   - 18.2.1.1) Add SET node for Target to Follow
   - 18.2.1.2) Connect CachedPlayerCharacter to Target to Follow
   - 18.2.2) From SET execution:
   - 18.2.2.1) Add SET node for Follow Distance (on goal)
   - 18.2.2.2) Connect FollowDistance variable to value

#### 18.3) Store Goal Reference
   - 18.3.1) From SET Follow Distance execution:
   - 18.3.1.1) Add SET CurrentFollowGoal node
   - 18.3.1.2) Connect Constructed Goal to value

### **19) Add Goal to Activity Component**

#### 19.1) Call AddGoalItem
   - 19.1.1) From SET CurrentFollowGoal execution:
   - 19.1.1.1) Add Add Goal Item node (inherited from NPCGoalGenerator)
   - 19.1.2) Connect:
   - 19.1.2.1) Goal: CurrentFollowGoal
   - 19.1.2.2) Trigger Reselect: Check (true)

### **20) Initialize Following State**

#### 20.1) Set bIsFollowing
   - 20.1.1) From Add Goal Item execution:
   - 20.1.1.1) Add SET bIsFollowing node
   - 20.1.1.2) bIsFollowing: Check (true)

#### 20.2) Reset TalkCount
   - 20.2.1) From SET bIsFollowing execution:
   - 20.2.1.1) Add SET TalkCount node
   - 20.2.1.2) TalkCount: 0

#### 20.3) Record Start Time
   - 20.3.1) From SET TalkCount execution:
   - 20.3.1.1) Add Get Game Time in Seconds node
   - 20.3.2) From Return Value:
   - 20.3.2.1) Add SET FollowStartTime node

### **21) Play Start Following Dialogue**

#### 21.1) Get NPC Character
   - 21.1.1) From SET FollowStartTime execution:
   - 21.1.1.1) From OwnerController, Add Get Controlled Pawn node
   - 21.1.2) Add Cast to NarrativeNPCCharacter node

#### 21.2) Play Tagged Dialogue
   - 21.2.1) From Cast successful execution:
   - 21.2.1.1) Add Play Tagged Dialogue node
   - 21.2.2) Connect:
   - 21.2.2.1) Target: As NarrativeNPCCharacter
   - 21.2.2.2) Tag: Tag_StartFollowing variable
   - 21.2.2.3) Dialogue Instigator: CachedPlayerCharacter

### **22) Bind to Player Damage Event**

#### 22.1) Bind Event
   - 22.1.1) From Play Tagged Dialogue execution:
   - 22.1.1.1) Add Bind Event to On Damage Received node
   - 22.1.2) Connect:
   - 22.1.2.1) Target: CachedPlayerCharacter
   - 22.1.3) From Red delegate pin:
   - 22.1.3.1) Drag and select: Add Custom Event
   - 22.1.3.2) Name: OnPlayerDamaged
   - 22.1.4) Add parameters to match FOnDamagedBy delegate signature:
   - 22.1.4.1) DamagerCauserASC: NarrativeAbilitySystemComponent Reference
   - 22.1.4.2) Damage: Float
   - 22.1.4.3) Spec: GameplayEffectSpec Reference

### **23) Start Talk Check Timer**

#### 23.1) Set Talk Timer
   - 23.1.1) From Bind Event execution:
   - 23.1.1.1) Add Set Timer by Event node
   - 23.1.2) Configure:
   - 23.1.2.1) Time: Connect TalkCheckInterval variable
   - 23.1.2.2) Looping: Check (true)
   - 23.1.3) From Return Value:
   - 23.1.3.1) Add SET TalkCheckTimerHandle node

#### 23.2) Create Talk Check Event
   - 23.2.1) From Red delegate pin:
   - 23.2.1.1) Drag and select: Add Custom Event
   - 23.2.1.2) Name: OnTalkCheckTimer

### **24) Start Attack Check Timer**

#### 24.1) Set Attack Timer
   - 24.1.1) From SET TalkCheckTimerHandle execution:
   - 24.1.1.1) Add Set Timer by Event node
   - 24.1.2) Configure:
   - 24.1.2.1) Time: Connect AttackCheckInterval variable
   - 24.1.2.2) Looping: Check (true)
   - 24.1.3) From Return Value:
   - 24.1.3.1) Add SET AttackCheckTimerHandle node

#### 24.2) Create Attack Check Event
   - 24.2.1) From Red delegate pin:
   - 24.2.1.1) Drag and select: Add Custom Event
   - 24.2.1.2) Name: OnAttackCheckTimer

### **25) Start Follow Duration Timer**

#### 25.1) Calculate Initial Duration
   - 25.1.1) From SET AttackCheckTimerHandle execution:
   - 25.1.1.1) Drag BaseFollowDuration variable getter

#### 25.2) Set Duration Timer
   - 25.2.1) Add Set Timer by Event node
   - 25.2.2) Configure:
   - 25.2.2.1) Time: Connect BaseFollowDuration
   - 25.2.2.2) Looping: Uncheck (false)
   - 25.2.3) From Return Value:
   - 25.2.3.1) Add SET FollowDurationTimerHandle node

#### 25.3) Create Duration Timeout Event
   - 25.3.1) From Red delegate pin:
   - 25.3.1.1) Drag and select: Add Custom Event
   - 25.3.1.2) Name: OnFollowDurationExpired

### **26) Compile and Save**
   - 26.1) Click Compile
   - 26.2) Click Save

---

## PHASE 5: IMPLEMENT TALK ROLL AND STACKING BOND

### **27) Implement OnTalkCheckTimer**

#### 27.1) Verify Still Following
   - 27.1.1) From OnTalkCheckTimer event:
   - 27.1.1.1) Drag bIsFollowing variable getter
   - 27.1.2) Add Branch node
   - 27.1.3) Connect bIsFollowing to Condition

### **28) Check Max Stacks Not Reached**

#### 28.1) Compare Talk Count
   - 28.1.1) From Branch True pin:
   - 28.1.1.1) Add Less Than (Integer) node
   - 28.1.2) Connect:
   - 28.1.2.1) A: TalkCount variable
   - 28.1.2.2) B: MaxTalkStacks variable
   - 28.1.3) Add Branch node
   - 28.1.4) Connect comparison result to Condition

### **29) Roll Talk Chance**

#### 29.1) Generate Random Roll
   - 29.1.1) From Branch True pin:
   - 29.1.1.1) Add Random Float in Range node
   - 29.1.1.2) Min: 0.0
   - 29.1.1.3) Max: 1.0

#### 29.2) Compare to Talk Chance
   - 29.2.1) Add Less Than or Equal (Float) node
   - 29.2.2) Connect:
   - 29.2.2.1) A: Random Float Return Value
   - 29.2.2.2) B: TalkChance variable
   - 29.2.3) Add Branch node
   - 29.2.4) Connect comparison result to Condition

### **30) Increment Talk Count**

#### 30.1) Increment Counter
   - 30.1.1) From Branch True pin (talk roll succeeded):
   - 30.1.1.1) Drag TalkCount variable getter
   - 30.1.1.2) Add Increment Int node
   - 30.1.2) From Increment Int:
   - 30.1.2.1) Add SET TalkCount node

### **31) Select Dialogue Tag Based on Talk Count**

#### 31.1) Create Select Node
   - 31.1.1) From SET TalkCount execution:
   - 31.1.1.1) Add Select node (GameplayTag type)
   - 31.1.2) Configure options:
   - 31.1.2.1) Option 0: Tag_FollowFirst (for TalkCount = 1)
   - 31.1.2.2) Option 1: Tag_FollowBonding (for TalkCount = 2)
   - 31.1.2.3) Option 2: Tag_FollowBonding (for TalkCount = 3)
   - 31.1.2.4) Default: Tag_FollowDeep (for TalkCount >= 4)

#### 31.2) Calculate Select Index
   - 31.2.1) Add Subtract (Integer) node
   - 31.2.2) Connect:
   - 31.2.2.1) A: TalkCount
   - 31.2.2.2) B: 1 (constant)
   - 31.2.3) Add Clamp (Integer) node
   - 31.2.4) Connect:
   - 31.2.4.1) Value: Subtract result
   - 31.2.4.2) Min: 0
   - 31.2.4.3) Max: 3
   - 31.2.5) Connect Clamp result to Select Index

### **32) Play Talk Dialogue**

#### 32.1) Get NPC Character
   - 32.1.1) From Select execution flow:
   - 32.1.1.1) From OwnerController, Add Get Controlled Pawn node
   - 32.1.2) Add Cast to NarrativeNPCCharacter node

#### 32.2) Play Tagged Dialogue
   - 32.2.1) From Cast successful execution:
   - 32.2.1.1) Add Play Tagged Dialogue node
   - 32.2.2) Connect:
   - 32.2.2.1) Target: As NarrativeNPCCharacter
   - 32.2.2.2) Tag: Select Return Value
   - 32.2.2.3) Dialogue Instigator: CachedPlayerCharacter

### **33) Extend Follow Duration**

#### 33.1) Calculate New Duration
   - 33.1.1) From Play Tagged Dialogue execution:
   - 33.1.1.1) Add Call Function: GetCurrentFollowDuration (created in Phase 9)

#### 33.2) Get Remaining Time
   - 33.2.1) Add Get Timer Remaining Time by Handle node
   - 33.2.2) Connect: FollowDurationTimerHandle

#### 33.3) Calculate Extended Time
   - 33.3.1) Add Add (Float) node
   - 33.3.2) Connect:
   - 33.3.2.1) A: Remaining Time
   - 33.3.2.2) B: FollowDurationPerStack variable

#### 33.4) Clamp to Max Duration
   - 33.4.1) Add Min (Float) node
   - 33.4.2) Connect:
   - 33.4.2.1) A: Extended Time
   - 33.4.2.2) B: GetCurrentFollowDuration Return Value

#### 33.5) Clear Old Timer
   - 33.5.1) Add Clear and Invalidate Timer by Handle node
   - 33.5.2) Connect: FollowDurationTimerHandle

#### 33.6) Set New Timer
   - 33.6.1) From Clear Timer execution:
   - 33.6.1.1) Add Set Timer by Event node
   - 33.6.2) Configure:
   - 33.6.2.1) Time: Min result (clamped extended time)
   - 33.6.2.2) Looping: Uncheck (false)
   - 33.6.3) Connect delegate to existing OnFollowDurationExpired event
   - 33.6.4) From Return Value:
   - 33.6.4.1) Add SET FollowDurationTimerHandle node

### **34) Compile and Save**
   - 34.1) Click Compile
   - 34.2) Click Save

---

## PHASE 6: IMPLEMENT DEFEND PLAYER MECHANIC

### **35) Implement OnPlayerDamaged**

#### 35.1) Verify Still Following
   - 35.1.1) From OnPlayerDamaged event:
   - 35.1.1.1) Drag bIsFollowing variable getter
   - 35.1.2) Add Branch node
   - 35.1.3) Connect bIsFollowing to Condition

#### 35.2) Check Not Already Defending
   - 35.2.1) From Branch True pin:
   - 35.2.1.1) Drag bIsDefending variable getter
   - 35.2.1.2) Add NOT Boolean node
   - 35.2.2) Add Branch node
   - 35.2.3) Connect NOT result to Condition

### **36) Extract Attacker Actor from ASC**

#### 36.1) Get Attacker Actor
   - 36.1.1) From Branch True pin:
   - 36.1.1.1) Drag DamagerCauserASC parameter
   - 36.1.1.2) Add Get Owner node (from ActorComponent)
   - 36.1.2) Store result as AttackerActor (local variable or wire)

#### 36.2) Check Attacker Valid
   - 36.2.1) From Get Owner Return Value:
   - 36.2.1.1) Add Is Valid node
   - 36.2.2) Add Branch node
   - 36.2.3) Connect Is Valid result to Condition

#### 36.3) Check Attacker Not Self
   - 36.3.1) From Branch True pin:
   - 36.3.1.1) Add Equal (Object) node
   - 36.3.2) Connect:
   - 36.3.2.1) A: AttackerActor (from Get Owner)
   - 36.3.2.2) B: From OwnerController -> Get Controlled Pawn
   - 36.3.3) Add NOT Boolean node
   - 36.3.4) Add Branch node
   - 36.3.5) Connect NOT result to Condition

### **37) Check Attacker in Range**

#### 37.1) Get Distance to Attacker
   - 37.1.1) From Branch True pin:
   - 37.1.1.1) From OwnerController -> Get Controlled Pawn
   - 37.1.2) Add Get Distance To node
   - 37.1.3) Connect:
   - 37.1.3.1) Other Actor: AttackerActor (from Get Owner)

#### 37.2) Compare to Defend Range
   - 37.2.1) Add Less Than or Equal (Float) node
   - 37.2.2) Connect:
   - 37.2.2.1) A: Get Distance To Return Value
   - 37.2.2.2) B: DefendRange variable
   - 37.2.3) Add Branch node
   - 37.2.4) Connect comparison result to Condition

### **38) Roll Defend Chance**

#### 38.1) Get Current Defend Chance
   - 38.1.1) From Branch True pin:
   - 38.1.1.1) Add Call Function: GetCurrentDefendChance (created in Phase 9)

#### 38.2) Generate Random Roll
   - 38.2.1) Add Random Float in Range node
   - 38.2.2) Min: 0.0
   - 38.2.3) Max: 1.0

#### 38.3) Compare to Defend Chance
   - 38.3.1) Add Less Than or Equal (Float) node
   - 38.3.2) Connect:
   - 38.3.2.1) A: Random Float Return Value
   - 38.3.2.2) B: GetCurrentDefendChance Return Value
   - 38.3.3) Add Branch node
   - 38.3.4) Connect comparison result to Condition

### **39) Create Defend Goal**

#### 39.1) Construct Attack Goal
   - 39.1.1) From Branch True pin (defend roll succeeded):
   - 39.1.1.1) Add Construct Object from Class node
   - 39.1.1.2) Class: Goal_Attack
   - 39.1.1.3) Outer: Self

#### 39.2) Configure Attack Goal
   - 39.2.1) From Construct Object Return Value:
   - 39.2.1.1) Cast AttackerActor to NarrativeCharacter
   - 39.2.1.2) Add SET node for TargetToAttack property
   - 39.2.1.3) Connect Cast result (As NarrativeCharacter) to TargetToAttack

#### 39.3) Store Defend Goal Reference
   - 39.3.1) From SET Target execution:
   - 39.3.1.1) Add SET CurrentDefendGoal node
   - 39.3.1.2) Connect Constructed Goal to value

### **40) Add Defend Goal**

#### 40.1) Call AddGoalItem
   - 40.1.1) From SET CurrentDefendGoal execution:
   - 40.1.1.1) Add Add Goal Item node
   - 40.1.2) Connect:
   - 40.1.2.1) Goal: CurrentDefendGoal
   - 40.1.2.2) Trigger Reselect: Check (true)

### **41) Set Defending State**

#### 41.1) Set bIsDefending
   - 41.1.1) From Add Goal Item execution:
   - 41.1.1.1) Add SET bIsDefending node
   - 41.1.1.2) bIsDefending: Check (true)

### **42) Play Defending Dialogue**

#### 42.1) Get NPC Character
   - 42.1.1) From SET bIsDefending execution:
   - 42.1.1.1) From OwnerController, Add Get Controlled Pawn node
   - 42.1.2) Add Cast to NarrativeNPCCharacter node

#### 42.2) Play Tagged Dialogue
   - 42.2.1) From Cast successful execution:
   - 42.2.1.1) Add Play Tagged Dialogue node
   - 42.2.2) Connect:
   - 42.2.2.1) Target: As NarrativeNPCCharacter
   - 42.2.2.2) Tag: Tag_Defending variable
   - 42.2.2.3) Dialogue Instigator: CachedPlayerCharacter

### **43) Bind to Defend Goal Completion**

#### 43.1) Bind to OnGoalRemoved (NOT OnGoalSucceeded)
   - 43.1.1) From Play Tagged Dialogue execution:
   - 43.1.1.1) Add Bind Event to On Goal Removed node
   - 43.1.2) Connect:
   - 43.1.2.1) Target: CurrentDefendGoal
   - 43.1.3) From Red delegate pin:
   - 43.1.3.1) Drag and select: Add Custom Event
   - 43.1.3.2) Name: OnDefendGoalCompleted
   - 43.1.3.3) Add parameters to match FOnGoalSignature:
   - 43.1.3.3.1) Activity: NPCActivity Reference
   - 43.1.3.3.2) Goal: NPCGoalItem Reference

> **IMPORTANT:** Use OnGoalRemoved, NOT OnGoalSucceeded. OnGoalRemoved fires in ALL
> termination cases (success, failure, timeout, cleanup). OnGoalFailed does NOT exist
> in Narrative Pro. Using OnGoalSucceeded alone causes stuck defending state when
> target escapes or goal times out.

### **44) Implement OnDefendGoalCompleted**

#### 44.1) Clear Defending State
   - 44.1.1) From OnDefendGoalCompleted event:
   - 44.1.1.1) Add SET bIsDefending node
   - 44.1.1.2) bIsDefending: Uncheck (false)

#### 44.2) Clear Defend Goal Reference
   - 44.2.1) From SET bIsDefending execution:
   - 44.2.1.1) Add SET CurrentDefendGoal node
   - 44.2.1.2) Leave value empty (None)

### **45) Compile and Save**
   - 45.1) Click Compile
   - 45.2) Click Save

---

## PHASE 7: IMPLEMENT ATTACK ROLL LOGIC

### **46) Implement OnAttackCheckTimer**

#### 46.1) Verify Still Following
   - 46.1.1) From OnAttackCheckTimer event:
   - 46.1.1.1) Drag bIsFollowing variable getter
   - 46.1.2) Add Branch node
   - 46.1.3) Connect bIsFollowing to Condition

#### 46.2) Check Not Defending
   - 46.2.1) From Branch True pin:
   - 46.2.1.1) Drag bIsDefending variable getter
   - 46.2.1.2) Add NOT Boolean node
   - 46.2.2) Add Branch node
   - 46.2.3) Connect NOT result to Condition

### **47) Roll Attack Chance**

#### 47.1) Get Current Attack Chance
   - 47.1.1) From Branch True pin:
   - 47.1.1.1) Add Call Function: GetCurrentAttackChance (created in Phase 9)

#### 47.2) Generate Random Roll
   - 47.2.1) Add Random Float in Range node
   - 47.2.2) Min: 0.0
   - 47.2.3) Max: 1.0

#### 47.3) Compare to Attack Chance
   - 47.3.1) Add Less Than or Equal (Float) node
   - 47.3.2) Connect:
   - 47.3.2.1) A: Random Float Return Value
   - 47.3.2.2) B: GetCurrentAttackChance Return Value
   - 47.3.3) Add Branch node
   - 47.3.4) Connect comparison result to Condition

### **48) Create Attack Goal Against Player**

#### 48.1) Construct Attack Goal
   - 48.1.1) From Branch True pin (attack roll succeeded):
   - 48.1.1.1) Add Construct Object from Class node
   - 48.1.1.2) Class: Goal_Attack
   - 48.1.1.3) Outer: Self

#### 48.2) Configure Attack Goal
   - 48.2.1) From Construct Object Return Value:
   - 48.2.1.1) Add SET node for Target Actor
   - 48.2.1.2) Connect CachedPlayerCharacter to target

### **49) Play Attacking Dialogue**

#### 49.1) Get NPC Character
   - 49.1.1) From SET Target execution:
   - 49.1.1.1) From OwnerController, Add Get Controlled Pawn node
   - 49.1.2) Add Cast to NarrativeNPCCharacter node

#### 49.2) Play Tagged Dialogue
   - 49.2.1) From Cast successful execution:
   - 49.2.1.1) Add Play Tagged Dialogue node
   - 49.2.2) Connect:
   - 49.2.2.1) Target: As NarrativeNPCCharacter
   - 49.2.2.2) Tag: Tag_Attacking variable
   - 49.2.2.3) Dialogue Instigator: CachedPlayerCharacter

### **50) Add Attack Goal and Clean Up**

#### 50.1) Call AddGoalItem
   - 50.1.1) From Play Tagged Dialogue execution:
   - 50.1.1.1) Add Add Goal Item node
   - 50.1.2) Connect:
   - 50.1.2.1) Goal: Constructed Goal_Attack
   - 50.1.2.2) Trigger Reselect: Check (true)

#### 50.2) Call BecomeAggressive (NOT StopFollowing)
   - 50.2.1) From Add Goal Item execution:
   - 50.2.1.1) Add Call Function: BecomeAggressive (created in Phase 8)

> **IMPORTANT:** Use BecomeAggressive, NOT StopFollowing. The attack path leads to
> PERMANENT aggression - the NPC should stay hostile. StopFollowing returns the NPC
> to patrol mode, which is wrong for attack behavior.

### **51) Compile and Save**
   - 51.1) Click Compile
   - 51.2) Click Save

---

## PHASE 8: IMPLEMENT TIMEOUT AND CLEANUP

### **52) Implement OnFollowDurationExpired**

#### 52.1) Play Giving Up Dialogue
   - 52.1.1) From OnFollowDurationExpired event:
   - 52.1.1.1) From OwnerController, Add Get Controlled Pawn node
   - 52.1.2) Add Cast to NarrativeNPCCharacter node

#### 52.2) Play Tagged Dialogue
   - 52.2.1) From Cast successful execution:
   - 52.2.1.1) Add Play Tagged Dialogue node
   - 52.2.2) Connect:
   - 52.2.2.1) Target: As NarrativeNPCCharacter
   - 52.2.2.2) Tag: Tag_GivingUp variable
   - 52.2.2.3) Dialogue Instigator: CachedPlayerCharacter

#### 52.3) Call StopFollowing
   - 52.3.1) From Play Tagged Dialogue execution:
   - 52.3.1.1) Add Call Function: StopFollowing

### **53) Create StopFollowing Function**

#### 53.1) Add Function
   - 53.1.1) In My Blueprint panel, click + next to Functions
   - 53.1.2) Name: StopFollowing
   - 53.1.3) Press Enter

### **54) Implement StopFollowing - Clear Timers**

#### 54.1) Clear Talk Timer
   - 54.1.1) From function entry node:
   - 54.1.1.1) Add Clear and Invalidate Timer by Handle node
   - 54.1.1.2) Connect TalkCheckTimerHandle variable

#### 54.2) Clear Attack Timer
   - 54.2.1) From Clear Timer execution:
   - 54.2.1.1) Add Clear and Invalidate Timer by Handle node
   - 54.2.1.2) Connect AttackCheckTimerHandle variable

#### 54.3) Clear Duration Timer
   - 54.3.1) From Clear Timer execution:
   - 54.3.1.1) Add Clear and Invalidate Timer by Handle node
   - 54.3.1.2) Connect FollowDurationTimerHandle variable

### **55) Implement StopFollowing - Unbind Player Damage**

#### 55.1) Check Player Valid
   - 55.1.1) From Clear Timer execution:
   - 55.1.1.1) Drag CachedPlayerCharacter variable getter
   - 55.1.1.2) Add Is Valid node
   - 55.1.2) Add Branch node
   - 55.1.3) Connect Is Valid result to Condition

#### 55.2) Unbind Event
   - 55.2.1) From Branch True pin:
   - 55.2.1.1) Add Unbind Event from On Damage Received node
   - 55.2.2) Connect:
   - 55.2.2.1) Target: CachedPlayerCharacter

### **56) Implement StopFollowing - Remove Follow Goal**

#### 56.1) Check Goal Valid
   - 56.1.1) From Branch execution (True or False merge):
   - 56.1.1.1) Drag CurrentFollowGoal variable getter
   - 56.1.1.2) Add Is Valid node
   - 56.1.2) Add Branch node
   - 56.1.3) Connect Is Valid result to Condition

#### 56.2) Remove Goal
   - 56.2.1) From Branch True pin:
   - 56.2.1.1) Add Remove Goal Item node (inherited)
   - 56.2.1.2) Connect CurrentFollowGoal to Goal input

### **57) Implement StopFollowing - Reset State**

#### 57.1) Clear Goal Reference
   - 57.1.1) From Remove Goal Item execution (or Branch False merge):
   - 57.1.1.1) Add SET CurrentFollowGoal node
   - 57.1.1.2) Leave value empty (None)

#### 57.2) Clear Player Reference
   - 57.2.1) From SET CurrentFollowGoal execution:
   - 57.2.1.1) Add SET CachedPlayerCharacter node
   - 57.2.1.2) Leave value empty (None)

#### 57.3) Reset Following State
   - 57.3.1) From SET CachedPlayerCharacter execution:
   - 57.3.1.1) Add SET bIsFollowing node
   - 57.3.1.2) bIsFollowing: Uncheck (false)

#### 57.4) Reset Defending State
   - 57.4.1) From SET bIsFollowing execution:
   - 57.4.1.1) Add SET bIsDefending node
   - 57.4.1.2) bIsDefending: Uncheck (false)

#### 57.5) Reset Talk Count
   - 57.5.1) From SET bIsDefending execution:
   - 57.5.1.1) Add SET TalkCount node
   - 57.5.1.2) TalkCount: 0

#### 57.6) Remove State Tags
   - 57.6.1) From SET TalkCount execution:
   - 57.6.1.1) From OwnerController, Add Get Controlled Pawn node
   - 57.6.2) Add Get Ability System Component node
   - 57.6.3) Add Remove Loose Gameplay Tag node
   - 57.6.3.1) Tag: State.NPC.Returned.Following
   - 57.6.4) Add Remove Loose Gameplay Tag node
   - 57.6.4.1) Tag: State.NPC.Returned.Bonded
   - 57.6.5) Add Remove Loose Gameplay Tag node
   - 57.6.5.1) Tag: State.NPC.Returned.Defending

### **58) Create BecomeAggressive Function**

> **Purpose:** Cleanup for ATTACK path - leads to PERMANENT aggression. Unlike
> StopFollowing, this does NOT return to patrol and does NOT reset TalkCount.

#### 58.1) Add Function
   - 58.1.1) In My Blueprint panel, click + next to Functions
   - 58.1.2) Name: BecomeAggressive
   - 58.1.3) Press Enter

#### 58.2) Clear Timers (Same as StopFollowing)
   - 58.2.1) From function entry node:
   - 58.2.1.1) Add Clear and Invalidate Timer by Handle node for TalkCheckTimerHandle
   - 58.2.1.2) Add Clear and Invalidate Timer by Handle node for AttackCheckTimerHandle
   - 58.2.1.3) Add Clear and Invalidate Timer by Handle node for FollowDurationTimerHandle

#### 58.3) Unbind Player Damage (Same as StopFollowing)
   - 58.3.1) Check CachedPlayerCharacter validity
   - 58.3.2) If valid, Unbind Event from On Damage Received

#### 58.4) Remove Follow Goal Only (Keep Defend Goal)
   - 58.4.1) Check CurrentFollowGoal validity
   - 58.4.2) If valid, Remove Goal Item for CurrentFollowGoal
   - 58.4.3) Clear CurrentFollowGoal reference
   - 58.4.4) Clear CachedPlayerCharacter reference
   - 58.4.5) NOTE: Do NOT clear CurrentDefendGoal - may still be active

#### 58.5) Update State (Partial Reset)
   - 58.5.1) SET bIsFollowing = false
   - 58.5.2) NOTE: Do NOT reset bIsDefending (defend may still be active)
   - 58.5.3) NOTE: Do NOT reset TalkCount (irrelevant when aggressive)

#### 58.6) Update State Tags
   - 58.6.1) From OwnerController, Get Controlled Pawn -> Get Ability System Component
   - 58.6.2) Add Remove Loose Gameplay Tag: State.NPC.Returned.Following
   - 58.6.3) Add Remove Loose Gameplay Tag: State.NPC.Returned.Bonded
   - 58.6.4) NOTE: Do NOT remove Defending tag (let OnDefendGoalCompleted handle it)
   - 58.6.5) NOTE: Aggressive tag is added in OnAttackCheckTimer BEFORE calling this

### **59) Compile and Save**
   - 59.1) Click Compile
   - 59.2) Click Save

---

## PHASE 9: CREATE HELPER FUNCTIONS

### **59) Create GetCurrentFollowDuration Function**

#### 59.1) Add Function
   - 59.1.1) In My Blueprint panel, click + next to Functions
   - 59.1.2) Name: GetCurrentFollowDuration
   - 59.1.3) Set Return Type: Float

#### 59.2) Implement Calculation
   - 59.2.1) From function entry node:
   - 59.2.1.1) Add Multiply (Float) node
   - 59.2.2) Connect:
   - 59.2.2.1) A: TalkCount variable (converted to float)
   - 59.2.2.2) B: FollowDurationPerStack variable
   - 59.2.3) Add Add (Float) node
   - 59.2.4) Connect:
   - 59.2.4.1) A: BaseFollowDuration variable
   - 59.2.4.2) B: Multiply result
   - 59.2.5) Connect Add result to Return Node

### **60) Create GetCurrentAttackChance Function**

#### 60.1) Add Function
   - 60.1.1) In My Blueprint panel, click + next to Functions
   - 60.1.2) Name: GetCurrentAttackChance
   - 60.1.3) Set Return Type: Float

#### 60.2) Implement Calculation
   - 60.2.1) From function entry node:
   - 60.2.1.1) Add Multiply (Float) node
   - 60.2.2) Connect:
   - 60.2.2.1) A: TalkCount variable (converted to float)
   - 60.2.2.2) B: AttackReductionPerStack variable
   - 60.2.3) Add Subtract (Float) node
   - 60.2.4) Connect:
   - 60.2.4.1) A: BaseAttackChance variable
   - 60.2.4.2) B: Multiply result
   - 60.2.5) Add Max (Float) node
   - 60.2.6) Connect:
   - 60.2.6.1) A: Subtract result
   - 60.2.6.2) B: MinAttackChance variable
   - 60.2.7) Connect Max result to Return Node

### **61) Create GetCurrentDefendChance Function**

#### 61.1) Add Function
   - 61.1.1) In My Blueprint panel, click + next to Functions
   - 61.1.2) Name: GetCurrentDefendChance
   - 61.1.3) Set Return Type: Float

#### 61.2) Implement Calculation
   - 61.2.1) From function entry node:
   - 61.2.1.1) Add Multiply (Float) node
   - 61.2.2) Connect:
   - 61.2.2.1) A: TalkCount variable (converted to float)
   - 61.2.2.2) B: DefendBonusPerStack variable
   - 61.2.3) Add Add (Float) node
   - 61.2.4) Connect:
   - 61.2.4.1) A: BaseDefendChance variable
   - 61.2.4.2) B: Multiply result
   - 61.2.5) Add Min (Float) node
   - 61.2.6) Connect:
   - 61.2.6.1) A: Add result
   - 61.2.6.2) B: MaxDefendChance variable
   - 61.2.7) Connect Min result to Return Node

### **62) Compile and Save**
   - 62.1) Click Compile
   - 62.2) Click Save

---

## PHASE 10: CREATE ACTIVITYCONFIGURATION

### **63) Create Configuration Asset**

#### 63.1) Navigate to Configurations Folder
   - 63.1.1) Content Browser -> /Game/AI/Configurations/
   - 63.1.2) If folder does not exist, create it

#### 63.2) Create ActivityConfiguration
   - 63.2.1) Right-click in Content Browser
   - 63.2.2) Select: Narrative -> NPC Activity Configuration
   - 63.2.3) Name: AC_ReturnedStalkerBehavior
   - 63.2.4) Double-click to open

### **64) Configure Default Activities**

#### 64.1) Add Activities Array Elements
   - 64.1.1) In Details panel, expand Default Activities
   - 64.1.2) Click + to add elements:

| Index | Activity |
|-------|----------|
| 0 | BPA_Patrol |
| 1 | BPA_FollowCharacter |
| 2 | BPA_Attack_Melee |
| 3 | BPA_Idle |

### **65) Configure Goal Generators**

#### 65.1) Add Goal Generators Array Elements
   - 65.1.1) Expand Goal Generators array
   - 65.1.2) Click + to add elements:

| Index | Generator |
|-------|-----------|
| 0 | GoalGenerator_RandomAggression |
| 1 | GoalGenerator_Attack |

### **66) Configure Rescore Interval**

#### 66.1) Set Interval
   - 66.1.1) Find Rescore Interval property
   - 66.1.2) Set to: 0.5

### **67) Save Configuration**
   - 67.1) Click Save

---

## PHASE 11: CREATE NPCDEFINITION

### **68) Create NPC Folder**

#### 68.1) Navigate to NPCs Folder
   - 68.1.1) Content Browser -> /Game/NPCs/
   - 68.1.2) Create folder: Returned
   - 68.1.3) Open Returned folder

### **69) Create NPCDefinition Asset**

#### 69.1) Create Definition
   - 69.1.1) Right-click in Content Browser
   - 69.1.2) Select: Narrative -> NPC Definition
   - 69.1.3) Name: NPC_ReturnedStalker
   - 69.1.4) Double-click to open

### **70) Configure Basic Properties**

#### 70.1) Set NPC Identity
   - 70.1.1) NPC ID: ReturnedStalker
   - 70.1.2) NPC Name: Returned
   - 70.1.3) NPC Class Path: Select NarrativeNPCCharacter (or custom BP)

### **71) Configure Activity Configuration**

#### 71.1) Set Configuration Reference
   - 71.1.1) Find Activity Configuration property
   - 71.1.2) Select: AC_ReturnedStalkerBehavior

### **72) Configure Tagged Dialogue Set**

#### 72.1) Set Dialogue Set Reference
   - 72.1.1) Find Tagged Dialogue Set property
   - 72.1.2) Select: TaggedDialogueSet_Returned

### **73) Configure Faction**

#### 73.1) Set Faction Tags
   - 73.1.1) Expand Factions array
   - 73.1.2) Add element: Faction.Neutral.Returned

### **74) Save NPCDefinition**
   - 74.1) Click Save

---

## PHASE 12: WORLD PLACEMENT

### **75) Configure Faction Attitudes**

#### 75.1) Open Faction Settings
   - 75.1.1) Edit -> Project Settings
   - 75.1.2) Navigate to: Narrative -> Faction Settings

#### 75.2) Set Faction Attitude
   - 75.2.1) Find Faction Attitudes array
   - 75.2.2) Add entry for Faction.Neutral.Returned
   - 75.2.3) Set attitude toward Narrative.Factions.Heroes: Neutral

### **76) Setup Patrol Points**

#### 76.1) Configure Patrol Area
   - 76.1.1) Place patrol point actors in level for patrol routes
   - 76.1.2) Configure patrol route per spawner if needed

### **77) Place NPCs in Level**

#### 77.1) Drag NPCDefinition to Level
   - 77.1.1) In Content Browser, find NPC_ReturnedStalker
   - 77.1.2) Drag and drop into level viewport
   - 77.1.3) NPCSpawner actor created automatically

#### 77.2) Configure Spawner
   - 77.2.1) Select NPCSpawner in level
   - 77.2.2) In Details panel:
   - 77.2.2.1) Spawn On Begin Play: Check (for field population)
   - 77.2.2.2) Configure patrol points if needed

### **78) Create Field of Distorted Souls**

#### 78.1) Populate Field with Multiple Returned
   - 78.1.1) Place 20-30 NPCSpawner actors in area
   - 78.1.2) Each uses NPC_ReturnedStalker
   - 78.1.3) Vary patrol routes for natural movement

#### 78.2) Optional Per-Instance Tuning
   - 78.2.1) Select individual spawners
   - 78.2.2) Override GoalGenerator_RandomAggression variables:
   - 78.2.2.1) FollowChance: Vary 0.1-0.2 for different aggression
   - 78.2.2.2) TalkChance: Vary 0.2-0.4 for different sociability
   - 78.2.2.3) AttackChance: Vary 0.15-0.35 for threat level

---

## BEHAVIOR SUMMARY

### State Transition Table

| From State | Trigger | To State | Dialogue Tag |
|------------|---------|----------|--------------|
| Patrol | FollowChance succeeds | Following | StartFollowing |
| Following | TalkChance succeeds | Following (bonded) | First/Bonding/Deep |
| Following | DefendChance succeeds | Defending | Defending |
| Following | AttackChance succeeds | Aggressive | Attacking |
| Following | Duration expires | Patrol | GivingUp |
| Defending | Target killed | Following | (none) |
| Aggressive | - | Aggressive | (permanent) |

### Timer Summary

| Timer | Interval | Looping | Purpose |
|-------|----------|---------|---------|
| FollowCheckTimer | FollowCheckInterval (8s) | Yes | Roll to start following |
| TalkCheckTimer | TalkCheckInterval (8s) | Yes | Roll to talk during follow |
| AttackCheckTimer | AttackCheckInterval (3s) | Yes | Roll to attack during follow |
| FollowDurationTimer | Calculated | No | Timeout to stop following |

### Probability Examples

With default values and TalkCount = 0:
- Follow Duration: 20s
- Attack Chance per roll: 25%
- Defend Chance: 50%

With TalkCount = 3:
- Follow Duration: 35s (+15s)
- Attack Chance per roll: 10% (-15%)
- Defend Chance: 65% (+15%)

With TalkCount = 5 (max):
- Follow Duration: 45s (+25s)
- Attack Chance per roll: 2.5% (minimum)
- Defend Chance: 75% (maximum)

### Field Chaos Scenarios

| Scenario | Description |
|----------|-------------|
| Multiple followers | Several Returned follow player simultaneously |
| Bonded bodyguard | Deep bond NPC defends against other Returned attacks |
| Chain reaction | Defender attacks another Returned, creating NPC vs NPC combat |
| Betrayal | Even bonded NPC can attack (2.5% chance at max bond) |
| Mass timeout | Multiple followers give up and disperse |
| Progressive bonding | Same NPC talks multiple times, becoming protector |

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 2.3 | January 2026 | **AUDIT FIXES:** (PATCH-B) Fixed OnPlayerDamaged delegate signature to match FOnDamagedBy(ASC, Float, Spec). (PATCH-D) Added BecomeAggressive function for attack path - separates permanent aggression from timeout cleanup. (PATCH-E) Changed defend goal binding from OnGoalSucceeded to OnGoalRemoved - fires in ALL termination cases. Added state tag removal to StopFollowing. Added GetOwner pattern to extract attacker actor from ASC. |
| 2.2 | January 2026 | Updated to Narrative Pro v2.2 naming conventions: ActConfig_ → AC_*Behavior suffix (AC_ReturnedStalkerBehavior), NPCDef_ → NPC_* prefix (NPC_ReturnedStalker). |
| 2.1 | January 2026 | Fixed ActivityConfiguration type name in Assets Created table for consistency. |
| 2.0 | January 2026 | Added stacking bond system with TalkCount. Added periodic talk rolls. Added defend player mechanic with player damage binding. Added return to following after defending. Added dynamic stat calculations. Integrated TaggedDialogueSet for all outcomes. Renamed from Stalker to Returned. |
| 1.0 | January 2026 | Initial implementation with basic follow/attack mechanics |

---

**END OF GUIDE**
