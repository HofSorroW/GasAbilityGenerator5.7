# GA_Backstab Implementation Guide v1.8

## Passive Damage Bonus Ability - ViewedCharacter Detection

### Unreal Engine 5.7 + Narrative Pro v2.2

### Blueprint-Only Implementation

---

## DOCUMENT INFORMATION

| Property | Value |
|----------|-------|
| Document Version | 1.8 |
| Ability Name | GA_Backstab |
| Ability Type | Passive (Triggered on Attack Hit) |
| Parent Class | NarrativeGameplayAbility |
| Form Availability | All Forms (Crawler, Armor, Exoskeleton, Symbiote, Engineer) |
| Input Required | None (Auto-Trigger) |
| Grant Location | Player Default Abilities array |

---

## TABLE OF CONTENTS

| Section | Description |
|---------|-------------|
| Phase 1 | Gameplay Tags Setup |
| Phase 2 | Create Gameplay Effect |
| Phase 3 | Configure BP_NarrativeNPCController |
| Phase 4 | Create GA_Backstab Ability |
| Phase 5 | Integrate with Attack Ability |
| Phase 6 | Add to Player Default Abilities |

---

## PREREQUISITES

| Requirement | Description |
|-------------|-------------|
| BP_NarrativeNPCController | Blueprint child of ANarrativeNPCController |
| Player Attack Ability | Existing attack ability with hit detection logic |
| Player Blueprint | BP_NarrativePlayer or similar with Default Abilities array |
| AIPerception Component | Configured on NPC Controller with AISense_Sight |

---

## QUICK REFERENCE

### AUTOMATION VS MANUAL (v5.0)

| Feature | Automation Status | Notes |
|---------|-------------------|-------|
| GA_Backstab Blueprint | ✅ Auto-generated | manifest.yaml creates ability with tags/properties |
| GE_BackstabBonus Effect | ✅ Auto-generated | manifest.yaml creates instant +25 AttackRating |
| CheckBackstabCondition Function | ✅ Auto-generated | Dot product math (simplified approximation) |
| ApplyBackstabBonus Function | ✅ Auto-generated | BP_ApplyGameplayEffectToOwner node graph |
| BP_NarrativeNPCController | ⚠️ Manual | Add ViewedCharacter variable (Phase 3, Step 8) |
| Handle Perception Update | ⚠️ Manual | Override + AIPerception logic (Phase 3, Steps 9-10) |
| Player Attack Integration | ⚠️ Manual | Add backstab check to hit processing (Phase 5) |
| Player Default Abilities | ⚠️ Manual | Add GA_Backstab to array (Phase 6) |

**Why ViewedCharacter is Manual:** The generator creates new assets from manifest but cannot modify existing Narrative Pro blueprints (BP_NarrativeNPCController). The ViewedCharacter approach uses AIPerception for correct gameplay (Father distraction, stealth integration).

### Tag Configuration Summary

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Backstab |
| Effect Tag | Effect.Backstab |
| Event Tag | Event.Combat.Backstab |
| Activation Required | None (passive, triggered by attack) |
| InputTag | None (auto-trigger) |

### Variable Summary (GA_Backstab)

| Variable | Type | Purpose |
|----------|------|---------|
| None | - | Functions only, no instance variables |

### Variable Summary (BP_NarrativeNPCController)

| Variable | Type | Purpose |
|----------|------|---------|
| ViewedCharacter | NarrativePlayerCharacter | Stores currently seen player |

### Gameplay Effect Summary

| Effect | Duration | AttackRating Bonus |
|--------|----------|-------------------|
| GE_BackstabBonus | Instant | +25 |

### Ability Configuration Summary

| Property | Value |
|----------|-------|
| Parent Class | NarrativeGameplayAbility |
| Instancing Policy | Instanced Per Actor |
| Replication Policy | Replicate Yes |
| Net Execution Policy | Local Predicted |
| Grant Location | Player Default Abilities array |

### Replication Settings

| Setting | Value |
|---------|-------|
| ViewedCharacter | Server Authority (NPC Controller owned by server) |
| GE_BackstabBonus | Instant (applied and consumed immediately) |

---

## ABILITY OVERVIEW

| Parameter | Value |
|-----------|-------|
| Detection Method | AIPerception ViewedCharacter Query |
| Backstab Condition | Enemy ViewedCharacter != Attacking Player |
| Backstab Bonus | +25% damage (via AttackRating) |
| Decoy Mechanic | Father distracts enemy, enabling player backstab |

---

## DAMAGE STACKING SYSTEM

| Bonus Source | AttackRating | Managed By |
|--------------|--------------|------------|
| Backstab (enemy not viewing) | +25 | GA_Backstab |
| Stealth (first attack from stealth) | +50 | GA_StealthField |
| Combined (stacks additively) | +75 | Independent systems |

---

## DAMAGE MULTIPLIER TABLE

| Condition | Enemy Sees Player | Player Stealthed | Multiplier |
|-----------|-------------------|------------------|------------|
| Normal Attack | Yes | No | 1.0x (100%) |
| Backstab Only | No | No | 1.25x (125%) |
| Stealth Only | Yes | Yes | 1.5x (150%) |
| Stealth + Backstab | No | Yes | 1.75x (175%) |

---

## TACTICAL SCENARIOS

| Scenario | ViewedCharacter | Backstab Valid |
|----------|-----------------|----------------|
| Enemy sees Player | Player Reference | No |
| Enemy sees nothing | null | Yes |
| Father distracts enemy | null (Father is NPC type) | Yes |
| Player in Stealth | null | Yes |
| Both Player and Father visible | Player Reference | No |

---

## DECOY TACTICAL FLOW

| Step | Action | Result |
|------|--------|--------|
| 1 | Player sends Father (Crawler form) toward enemy | Father is NarrativeNPCCharacter type |
| 2 | Enemy AIPerception detects Father | Handle Perception Update fires |
| 2.1 | Get Sense Class for Stimulus | Returns AISense_Sight |
| 2.2 | Cast to NarrativePlayerCharacter | FAILS (Father is NPC) |
| 2.3 | ViewedCharacter update | Remains NULL or unchanged |
| 3 | Enemy engages Father in combat | Enemy AI focuses on Father threat |
| 3.1 | Player flanks from behind | Player undetected |
| 4 | Player attacks enemy | IsBackstabValid check runs |
| 4.1 | Get Enemy Controller | Returns BP_NarrativeNPCController |
| 4.2 | Get ViewedCharacter | Returns NULL or non-player |
| 4.3 | Compare to attacking player | NOT EQUAL returns TRUE |
| 5 | BACKSTAB VALID | Apply GE_BackstabBonus (+25) |
| 5.1 | If also stealthed | GA_StealthField bonus (+50) stacks automatically |
| 5.2 | Continue | Normal damage calculation proceeds |

---

## PHASE 1: GAMEPLAY TAGS SETUP

### Create Required Tags

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Backstab | Identifies backstab ability |
| Effect.Backstab | Applied when backstab bonus active |
| Event.Combat.Backstab | Fired when backstab executed |

---

## PHASE 2: CREATE GAMEPLAY EFFECT

### 6) Create GE_BackstabBonus

#### 6.1) Navigate to Effects Folder
   - 6.1.1) In Content Browser, navigate to: `/Content/FatherCompanion/Effects/`
   - 6.1.2) If folder does not exist, create it:
   - 6.1.2.1) Right-click in Content Browser
   - 6.1.2.2) Select: **New Folder**
   - 6.1.2.3) Name: `Effects`

#### 6.2) Create Gameplay Effect Asset
   - 6.2.1) Right-click in Effects folder
   - 6.2.2) Select: **Gameplay** -> **Gameplay Effect**
   - 6.2.3) Name: `GE_BackstabBonus`
   - 6.2.4) Press: **Enter** to confirm
   - 6.2.5) Double-click: **GE_BackstabBonus** to open

#### 6.3) Configure Duration Policy
   - 6.3.1) In Details panel, find: **Gameplay Effect** section
   - 6.3.2) Locate: **Duration Policy** property
   - 6.3.3) Set: `Instant`

#### 6.4) Add Attack Rating Modifier

##### 6.4.1) Create Modifier Entry
   - 6.4.1.1) In Details panel, find: **Modifiers** section
   - 6.4.1.2) Click: **+** button next to Modifiers array
   - 6.4.1.3) Element [0] appears in array
   - 6.4.1.4) Expand: Element [0]

##### 6.4.2) Configure Attribute
   - 6.4.2.1) Find: **Attribute** property in Element [0]
   - 6.4.2.2) Click: Attribute dropdown
   - 6.4.2.3) In search field, type: `AttackRating`
   - 6.4.2.4) Select: `NarrativeAttributeSetBase.AttackRating`

##### 6.4.3) Configure Modifier Operation
   - 6.4.3.1) Find: **Modifier Op** property
   - 6.4.3.2) Click: Dropdown
   - 6.4.3.3) Select: `Add`

##### 6.4.4) Configure Modifier Magnitude
   - 6.4.4.1) Find: **Modifier Magnitude** section
   - 6.4.4.2) Expand: **Magnitude Calculation Type**
   - 6.4.4.3) Set: `Scalable Float`
   - 6.4.4.4) Find: **Scalable Float Magnitude** subsection
   - 6.4.4.5) Expand: **Scalable Float Magnitude**
   - 6.4.4.6) Set **Value**: `25.0`

#### 6.5) Add Granted Tags Component (UE 5.6)

##### 6.5.1) Add Component
   - 6.5.1.1) In Details panel, find: **Components** section
   - 6.5.1.2) Click: **+** button next to Components array
   - 6.5.1.3) From dropdown, select: **Grant Tags to Target Actor**
   - 6.5.1.4) Component appears in array

##### 6.5.2) Configure Granted Tag
   - 6.5.2.1) Expand: Grant Tags to Target Actor component
   - 6.5.2.2) Find: **Add to Inherited** array
   - 6.5.2.3) Click: **+** button
   - 6.5.2.4) Click: Tag dropdown
   - 6.5.2.5) Select: `Effect.Backstab`

#### 6.6) Add Asset Tags Component (UE 5.6)

##### 6.6.1) Add Component
   - 6.6.1.1) Click: **+** button next to Components array
   - 6.6.1.2) From dropdown, select: **Asset Tags (on GA GE)**
   - 6.6.1.3) Component appears in array

##### 6.6.2) Configure Asset Tag
   - 6.6.2.1) Expand: Asset Tags component
   - 6.6.2.2) Find: **Add to Inherited** array
   - 6.6.2.3) Click: **+** button
   - 6.6.2.4) Click: Tag dropdown
   - 6.6.2.5) Select: `Effect.Backstab`

#### 6.7) Compile and Save
   - 6.7.1) Click: **Compile** button
   - 6.7.2) Click: **Save** button
   - 6.7.3) Close: GE_BackstabBonus editor

---

## PHASE 3: CONFIGURE BP_NARRATIVENPCCONTROLLER

### 7) Open BP_NarrativeNPCController

#### 7.1) Locate Controller Blueprint
   - 7.1.1) In Content Browser, navigate to your AI Controllers folder
   - 7.1.2) Find: **BP_NarrativeNPCController**
   - 7.1.3) Double-click: To open blueprint

### 8) Create ViewedCharacter Variable

#### 8.1) Add Variable
   - 8.1.1) In My Blueprint panel, find: **Variables** section
   - 8.1.2) Click: **+** button to add new variable
   - 8.1.3) Name: `ViewedCharacter`
   - 8.1.4) Press: **Enter** to confirm

#### 8.2) Configure Variable Type
   - 8.2.1) Select: **ViewedCharacter** variable in My Blueprint panel
   - 8.2.2) In Details panel, find: **Variable Type** property
   - 8.2.3) Click: Type dropdown
   - 8.2.4) Search: `NarrativePlayerCharacter`
   - 8.2.5) Select: **Narrative Player Character** (Object Reference)

#### 8.3) Configure Variable Properties
   - 8.3.1) In Details panel, find: **Instance Editable** checkbox
   - 8.3.2) Leave: Unchecked
   - 8.3.3) Find: **Blueprint Read Only** checkbox
   - 8.3.4) Leave: Unchecked
   - 8.3.5) Find: **Category** field
   - 8.3.6) Enter: `Perception`

### 9) Locate Handle Perception Update Function

#### 9.1) Find or Create Function
   - 9.1.1) In My Blueprint panel, find: **Functions** section
   - 9.1.2) Look for: **Handle Perception Update**
   - 9.1.3) If exists: Double-click to open
   - 9.1.4) If not exists: Create by overriding from AIController

#### 9.2) Override Handle Perception Update (If Needed)
   - 9.2.1) In My Blueprint panel, click: **Override** dropdown
   - 9.2.2) Search: `Handle Perception Update`
   - 9.2.3) Select: **Handle Perception Update**
   - 9.2.4) Function graph opens

### 10) Build Perception Update Logic

#### 10.1) Get Updated Actors
   - 10.1.1) From function entry node, note the **Updated Actors** input array
   - 10.1.2) This contains all actors detected by AIPerception

#### 10.2) Create For Each Loop
   - 10.2.1) Drag from: **Updated Actors** pin
   - 10.2.2) Release in empty space
   - 10.2.3) Search: `For Each Loop`
   - 10.2.4) Select: **For Each Loop**
   - 10.2.5) Connect execution from function entry

#### 10.3) Get Actor Stimulus
   - 10.3.1) Drag from: For Each **Array Element** pin
   - 10.3.2) Search: `Get Actors Perception`
   - 10.3.3) Select: **Get Actors Perception** (from AIPerceptionComponent)
   - 10.3.4) Connect: Self to AIPerceptionComponent reference

#### 10.4) Get Last Sensed Stimuli
   - 10.4.1) Drag from: Get Actors Perception **Info** output
   - 10.4.2) Search: `Get Last Sensed Stimuli`
   - 10.4.3) Select: **Get Last Sensed Stimuli**

#### 10.5) Check Sight Sense
   - 10.5.1) Drag from: Get Last Sensed Stimuli **Return Value**
   - 10.5.2) Add: **For Each Loop** for stimuli array
   - 10.5.3) From Array Element (Stimulus):
   - 10.5.4) Add: **Break FAIStimulus** node
   - 10.5.5) From **Type** output:
   - 10.5.6) Add: **Equal (Class)** node
   - 10.5.7) Compare to: `AISense_Sight`

#### 10.6) Branch on Sight Detection
   - 10.6.1) Add: **Branch** node
   - 10.6.2) Connect: Equal result to Condition
   - 10.6.3) Connect execution from stimuli For Each Loop Body

#### 10.7) Check Successfully Sensed
   - 10.7.1) From Branch **True** execution:
   - 10.7.2) From **Break FAIStimulus**:
   - 10.7.3) Get: **Was Successfully Sensed** output
   - 10.7.4) Add: **Branch** node
   - 10.7.5) Connect: Was Successfully Sensed to Condition

#### 10.8) Cast to NarrativePlayerCharacter
   - 10.8.1) From sensed check Branch **True** execution:
   - 10.8.2) Add: **Cast To NarrativePlayerCharacter** node
   - 10.8.3) Connect: Original Array Element (from Updated Actors loop) to Object input

#### 10.9) Set ViewedCharacter on Success
   - 10.9.1) From Cast success execution:
   - 10.9.2) Add: **Set Viewed Character** node
   - 10.9.3) Connect: **As Narrative Player Character** to Viewed Character input

#### 10.10) Clear ViewedCharacter on Lost Sight
   - 10.10.1) From sensed check Branch **False** execution:
   - 10.10.2) Add: **Cast To NarrativePlayerCharacter** node
   - 10.10.3) Connect: Array Element to Object input
   - 10.10.4) From Cast success:
   - 10.10.5) Get: **ViewedCharacter** variable
   - 10.10.6) Add: **Equal (Object)** node
   - 10.10.7) Compare: ViewedCharacter to Cast result
   - 10.10.8) Add: **Branch** node
   - 10.10.9) From Branch True:
   - 10.10.10) Add: **Set Viewed Character** node
   - 10.10.11) Leave Viewed Character input empty (clears to None)

### 11) Compile and Save Controller

#### 11.1) Finalize
   - 11.1.1) Click: **Compile** button
   - 11.1.2) Verify: No compilation errors
   - 11.1.3) Click: **Save** button

---

## PHASE 4: CREATE GA_BACKSTAB ABILITY

### 12) Create Ability Blueprint

#### 12.1) Navigate to Abilities Folder
   - 12.1.1) In Content Browser, navigate to: `/Content/FatherCompanion/Abilities/`

#### 12.2) Create New Gameplay Ability
   - 12.2.1) Right-click in Abilities folder
   - 12.2.2) Select: **Blueprint Class**
   - 12.2.3) In Pick Parent Class window, click: **All Classes**
   - 12.2.4) In search field, type: `NarrativeGameplayAbility`
   - 12.2.5) Select: **NarrativeGameplayAbility**
   - 12.2.6) Click: **Select** button
   - 12.2.7) Name: `GA_Backstab`
   - 12.2.8) Press: **Enter** to confirm

#### 12.3) Open Ability Blueprint
   - 12.3.1) Double-click: **GA_Backstab** to open

### 13) Configure Ability Properties

#### 13.1) Open Class Defaults
   - 13.1.1) Click: **Class Defaults** button in toolbar
   - 13.1.2) Details panel shows ability properties

#### 13.2) Configure Ability Tags

##### 13.2.1) Set Ability Tags
   - 13.2.1.1) In Details panel, find: **Tags** section
   - 13.2.1.2) Find: **Ability Tags** property
   - 13.2.1.3) Click: **+** button to add element
   - 13.2.1.4) Click: Tag dropdown
   - 13.2.1.5) Select: `Ability.Father.Backstab`

#### 13.3) Configure Replication

##### 13.3.1) Set Instancing Policy
   - 13.3.1.1) Find: **Instancing Policy** property
   - 13.3.1.2) Set: `Instanced Per Actor`

##### 13.3.2) Set Net Execution Policy
   - 13.3.2.1) Find: **Net Execution Policy** property
   - 13.3.2.2) Set: `Local Predicted`

##### 13.3.3) Set Replication Policy
   - 13.3.3.1) Find: **Replication Policy** property
   - 13.3.3.2) Set: `Replicate Yes`

### 14) Create CheckBackstabCondition Function

#### 14.1) Add Function
   - 14.1.1) In My Blueprint panel, find: **Functions** section
   - 14.1.2) Click: **+** button next to Functions
   - 14.1.3) Name: `CheckBackstabCondition`
   - 14.1.4) Press: **Enter** to confirm
   - 14.1.5) Function graph opens

#### 14.2) Configure Input Parameter

##### 14.2.1) Add Input
   - 14.2.1.1) Select: Purple function entry node
   - 14.2.1.2) In Details panel, find: **Inputs** section
   - 14.2.1.3) Click: **+** button
   - 14.2.1.4) Set Name: `EnemyActor`
   - 14.2.1.5) Set Type: `Actor` (Object Reference)

#### 14.3) Configure Output Parameter

##### 14.3.1) Add Output
   - 14.3.1.1) In Details panel, find: **Outputs** section
   - 14.3.1.2) Click: **+** button
   - 14.3.1.3) Set Name: `CanBackstab`
   - 14.3.1.4) Set Type: `Boolean`

#### 14.4) Build Function Logic - Get Controller

##### 14.4.1) Get Enemy Controller
   - 14.4.1.1) Drag from: **EnemyActor** input pin
   - 14.4.1.2) Release in empty graph space
   - 14.4.1.3) In context menu, search: `Get Controller`
   - 14.4.1.4) Select: **Get Controller**

#### 14.5) Build Function Logic - Cast to NPC Controller

##### 14.5.1) Add Cast Node
   - 14.5.1.1) Drag from: **Get Controller** Return Value pin
   - 14.5.1.2) Release in empty space
   - 14.5.1.3) Search: `Cast to BP_NarrativeNPCController`
   - 14.5.1.4) Select: **Cast To BP_NarrativeNPCController**
   - 14.5.1.5) Connect execution wire from function entry to Cast node

#### 14.6) Build Function Logic - Get ViewedCharacter

##### 14.6.1) Access ViewedCharacter Variable
   - 14.6.1.1) Drag from: **As BP Narrative NPC Controller** output pin
   - 14.6.1.2) Release in empty space
   - 14.6.1.3) Search: `Viewed Character`
   - 14.6.1.4) Select: **Get Viewed Character**

#### 14.7) Build Function Logic - Get Attacking Player

##### 14.7.1) Get Owning Actor
   - 14.7.1.1) Right-click in empty graph space
   - 14.7.1.2) Search: `Get Owning Actor from Actor Info`
   - 14.7.1.3) Select: **Get Owning Actor from Actor Info**

#### 14.8) Build Function Logic - Compare Characters

##### 14.8.1) Add Not Equal Node
   - 14.8.1.1) Drag from: **Get Viewed Character** Return Value pin
   - 14.8.1.2) Release in empty space
   - 14.8.1.3) Search: `Not Equal (Object)`
   - 14.8.1.4) Select: **Not Equal (Object)** node
   - 14.8.1.5) Connect: **Get Owning Actor** Return Value to second input (B pin)

#### 14.9) Build Function Logic - Cast Success Return

##### 14.9.1) Add Return Node for Success
   - 14.9.1.1) Drag from: Cast node success execution pin
   - 14.9.1.2) Add: **Return Node**
   - 14.9.1.3) Connect: **Not Equal** Boolean output to **CanBackstab** input pin

#### 14.10) Build Function Logic - Cast Failure Return

##### 14.10.1) Add Return Node for Failure
   - 14.10.1.1) Drag from: Cast node **Cast Failed** execution pin
   - 14.10.1.2) Add: **Return Node**
   - 14.10.1.3) Set: **CanBackstab** output to `False` (unchecked)

### 15) Create ApplyBackstabBonus Function

#### 15.1) Add Function
   - 15.1.1) In My Blueprint panel, click: **+** next to Functions
   - 15.1.2) Name: `ApplyBackstabBonus`
   - 15.1.3) Press: **Enter** to confirm
   - 15.1.4) Function graph opens

#### 15.2) Build Function Logic - Apply Effect

##### 15.2.1) Apply Backstab Effect
   - 15.2.1.1) Drag from: Function entry execution pin
   - 15.2.1.2) Search: `Apply Gameplay Effect to Owner`
   - 15.2.1.3) Select: **Apply Gameplay Effect to Owner**
   - 15.2.1.4) **Gameplay Effect Class**: Select `GE_BackstabBonus`
   - 15.2.1.5) **Level**: `1`

##### 15.2.2) Add Return
   - 15.2.2.1) From: Apply Gameplay Effect execution output
   - 15.2.2.2) Add: **Return Node**

### 16) Compile and Save GA_Backstab

#### 16.1) Finalize
   - 16.1.1) Click: **Compile** button
   - 16.1.2) Verify: No compilation errors in Compiler Results
   - 16.1.3) Click: **Save** button
   - 16.1.4) Close: GA_Backstab blueprint

---

## PHASE 5: INTEGRATE WITH ATTACK ABILITY

### 17) Open Player Attack Ability

#### 17.1) Locate Attack Ability
   - 17.1.1) In Content Browser, navigate to your attack abilities folder
   - 17.1.2) Find: Your primary melee attack ability
   - 17.1.3) Double-click: To open ability blueprint

### 18) Add Backstab Check to Hit Detection

#### 18.1) Locate Hit Processing Logic
   - 18.1.1) In Event Graph, find where attack hit is processed
   - 18.1.2) This is typically after trace hit or overlap detection
   - 18.1.3) Identify the Hit Actor reference point

#### 18.2) Create Local Variable for GA_Backstab Reference

##### 18.2.1) Add Variable
   - 18.2.1.1) In My Blueprint panel, find: **Variables** section
   - 18.2.1.2) Click: **+** button
   - 18.2.1.3) Name: `BackstabAbilityRef`
   - 18.2.1.4) Type: `GA_Backstab` (Object Reference)

#### 18.3) Get Backstab Ability Reference

##### 18.3.1) Get ASC
   - 18.3.1.1) At start of hit processing logic
   - 18.3.1.2) Add: **Get Ability System Component from Actor Info** node

##### 18.3.2) Get Activatable Abilities
   - 18.3.2.1) Drag from: ASC Return Value
   - 18.3.2.2) Search: `Get Activatable Abilities`
   - 18.3.2.3) Select: **Get Activatable Abilities**

##### 18.3.3) Find Backstab Ability
   - 18.3.3.1) Drag from: Activatable Abilities array
   - 18.3.3.2) Add: **For Each Loop**
   - 18.3.3.3) From Array Element:
   - 18.3.3.4) Add: **Get Class** node
   - 18.3.3.5) Add: **Equal (Class)** node
   - 18.3.3.6) Compare to: `GA_Backstab`
   - 18.3.3.7) Branch on result
   - 18.3.3.8) If True: Cast to GA_Backstab and store reference

#### 18.4) Alternative - Direct Function Call Approach

##### 18.4.1) Inline Backstab Check
   - 18.4.1.1) Instead of referencing GA_Backstab
   - 18.4.1.2) Copy the CheckBackstabCondition logic directly into attack ability
   - 18.4.1.3) This is simpler for single-attack-ability projects

#### 18.5) Add Backstab Check

##### 18.5.1) Check Condition Before Damage
   - 18.5.1.1) After: Hit actor is validated as enemy
   - 18.5.1.2) Before: Damage effect is applied
   - 18.5.1.3) From BackstabAbilityRef variable:
   - 18.5.1.4) Drag and search: `Check Backstab Condition`
   - 18.5.1.5) Connect: Hit Actor to **EnemyActor** input

##### 18.5.2) Branch on Result
   - 18.5.2.1) Drag from: CheckBackstabCondition execution output
   - 18.5.2.2) Add: **Branch** node
   - 18.5.2.3) Connect: **CanBackstab** output to Branch **Condition**

##### 18.5.3) Apply Backstab Bonus (True Branch)
   - 18.5.3.1) From Branch **True** execution:
   - 18.5.3.2) Call: **Apply Backstab Bonus** function on BackstabAbilityRef

##### 18.5.4) Continue to Damage (Both Branches)
   - 18.5.4.1) From ApplyBackstabBonus output:
   - 18.5.4.2) Connect to: Damage application logic
   - 18.5.4.3) From Branch **False** execution:
   - 18.5.4.4) Connect directly to: Damage application logic

### 19) Add Visual and Audio Feedback

#### 19.1) Backstab Hit Effects

##### 19.1.1) Add Particle Effect
   - 19.1.1.1) After: ApplyBackstabBonus call
   - 19.1.1.2) Before: Damage application
   - 19.1.1.3) Add: **Spawn Emitter at Location** node
   - 19.1.1.4) **Emitter Template**: Your critical hit VFX
   - 19.1.1.5) **Location**: Hit location from trace result

##### 19.1.2) Add Sound Effect
   - 19.1.2.1) Add: **Play Sound at Location** node
   - 19.1.2.2) **Sound**: Your backstab/critical hit sound cue
   - 19.1.2.3) **Location**: Hit location

### 20) Compile and Save Attack Ability

#### 20.1) Finalize
   - 20.1.1) Click: **Compile** button
   - 20.1.2) Verify: No compilation errors
   - 20.1.3) Click: **Save** button
   - 20.1.4) Close: Attack ability blueprint

---

## PHASE 6: ADD TO PLAYER DEFAULT ABILITIES

### 21) Configure Player Blueprint

GA_Backstab is a generic action game mechanic available to all players. It is granted via the Player Default Abilities array, not tied to father recruitment.

#### 21.1) Open Player Blueprint
   - 21.1.1) In Content Browser: Navigate to player blueprint folder
   - 21.1.2) Double-click: `BP_NarrativePlayer` (or your player blueprint)
   - 21.1.3) Click: **Class Defaults** button

#### 21.2) Find Default Abilities Array
   - 21.2.1) In Details panel: Search `abilit`
   - 21.2.2) Locate: **Narrative** -> **Abilities** category
   - 21.2.3) Find: **Default Abilities** array

#### 21.3) Add GA_Backstab
   - 21.3.1) Click: **+** button to add element
   - 21.3.2) New element appears at end of array
   - 21.3.3) Dropdown: Select **GA_Backstab**

#### 21.4) Verify Array
   - 21.4.1) GA_Backstab should now be in Default Abilities
   - 21.4.2) Position in array does not matter

### 22) Save Player Blueprint

#### 22.1) Finalize
   - 22.1.1) Click: **Compile** button
   - 22.1.2) Verify: No compilation errors
   - 22.1.3) Click: **Save** button

### 23) Design Rationale

| Aspect | Player Default | Father Recruitment |
|--------|----------------|-------------------|
| Availability | Always | Only with father |
| Player Expectation | Backstab is standard action game mechanic | Tied to companion |
| Implementation | Simple (no handles) | Complex (grant/cleanup) |
| Future Enhancement | Father can boost backstab damage | Father grants ability |

Backstab is available to all players as a baseline mechanic. Father companion can enhance backstab effectiveness via GE_BackstabBonus when recruited (future enhancement).

---

## ALTERNATIVE: INLINE INTEGRATION

This section provides an alternative implementation where backstab logic is integrated directly into your attack ability rather than as a separate ability.

### 23) Create Standalone Backstab Check Function

#### 23.1) Open Attack Ability
   - 23.1.1) Open your primary attack ability blueprint

#### 23.2) Create IsBackstabValid Function

##### 23.2.1) Add Function
   - 23.2.1.1) In My Blueprint panel, click: **+** next to Functions
   - 23.2.1.2) Name: `IsBackstabValid`
   - 23.2.1.3) Press: **Enter**

##### 23.2.2) Configure Parameters
   - 23.2.2.1) Input: `HitEnemy` (Actor Reference)
   - 23.2.2.2) Output: `IsValid` (Boolean)

##### 23.2.3) Build Logic
   - 23.2.3.1) From HitEnemy: Add **Get Controller** node
   - 23.2.3.2) From Return Value: Add **Cast To BP_NarrativeNPCController** node
   - 23.2.3.3) From As BP Narrative NPC Controller: Add **Get Viewed Character** node
   - 23.2.3.4) Right-click: Add **Get Avatar Actor From Actor Info** node
   - 23.2.3.5) Add: **Not Equal (Object)** node
   - 23.2.3.6) Connect: Get Viewed Character to A input
   - 23.2.3.7) Connect: Get Avatar Actor to B input
   - 23.2.3.8) Add: **Return Node**
   - 23.2.3.9) Connect: Not Equal result to IsValid output

#### 23.3) Create ApplyBackstabEffect Function

##### 23.3.1) Add Function
   - 23.3.1.1) Name: `ApplyBackstabEffect`

##### 23.3.2) Build Logic
   - 23.3.2.1) Add: **Get Ability System Component from Actor Info** node
   - 23.3.2.2) From ASC: Add **Apply Gameplay Effect to Self** node
   - 23.3.2.3) **Gameplay Effect Class**: `GE_BackstabBonus`
   - 23.3.2.4) **Level**: `1`

### 24) Integrate Into Hit Processing

#### 24.1) After Hit Validation
   - 24.1.1) Call: IsBackstabValid(HitActor)
   - 24.1.2) Branch on result
   - 24.1.3) If True: Call ApplyBackstabEffect
   - 24.1.4) Continue to damage application

---

## COMPLETE TAG REFERENCE

| Tag | Purpose |
|-----|---------|
| `Ability.Father.Backstab` | Identifies backstab ability |
| `Effect.Backstab` | Applied when backstab bonus active |
| `Event.Combat.Backstab` | Fired when backstab executed |

---

## GAMEPLAY EFFECT REFERENCE

| Effect | AttackRating Bonus | Condition |
|--------|-------------------|-----------|
| `GE_BackstabBonus` | +25 | Enemy ViewedCharacter != Player |
| `GE_StealthDamageBonus` | +50 | Player stealthed (managed by GA_StealthField) |

---

## DAMAGE STACKING EXPLANATION

| Scenario | Effects Applied | Total Bonus |
|----------|-----------------|-------------|
| Normal attack (enemy sees player) | None | +0 |
| Backstab only (not stealthed) | GE_BackstabBonus | +25 |
| Stealth only (enemy sees player) | GE_StealthDamageBonus | +50 |
| Stealth + Backstab | Both effects stack | +75 |

---

## MULTIPLAYER REPLICATION

| Setting | Value | Purpose |
|---------|-------|---------|
| Instancing Policy | Instanced Per Actor | Each actor has own instance |
| Net Execution Policy | Local Predicted | Client predicts, server validates |
| Replication Policy | Replicate Yes | Effect syncs to all clients |
| GE Duration | Instant | Applied and consumed immediately |
| ViewedCharacter | Server Authority | NPC Controller owned by server |

---

## INTEGRATION SUMMARY

| Component | Modification Required |
|-----------|----------------------|
| BP_NarrativeNPCController | Add ViewedCharacter variable, modify Handle Perception Update |
| Player Blueprint | Add GA_Backstab to Default Abilities array |
| AIPerception Component | None (uses existing sight sense) |
| Player Attack Ability | Add backstab check logic |
| GA_StealthField | None (stealth bonus stacks independently) |

---

## RELATED DOCUMENTS

| Document | Purpose |
|----------|---------|
| Father_Companion_System_Setup_Guide_v2_3.md | Phase 29: AbilityConfiguration without GA_Backstab |
| GA_StealthField_Implementation_Guide_v3_2.md | Stealth damage bonus that stacks with backstab |
| Father_Companion_System_Design_Document_v1_8.md | Section 3.1: Backstab mechanic overview |
| DefaultGameplayTags_FatherCompanion_v3_5.ini | Tag definitions |

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 1.8 | January 2026 | **AUTOMATION VS MANUAL Table Added:** Clarified what is auto-generated (GA_Backstab, GE_BackstabBonus, custom functions) vs manual (BP_NarrativeNPCController ViewedCharacter, Handle Perception Update). Explained why ViewedCharacter requires manual integration (generator cannot modify existing Narrative Pro blueprints). |
| 1.7 | January 2026 | **Locked Decisions Reference:** Added Father_Companion_GAS_Abilities_Audit.md reference. This guide complies with Decision 4 (GA_Backstab is UNIVERSAL player ability, not Exoskeleton-only). Grant Location confirmed as Player Default Abilities array. Updated UE version to 5.7. |
| 1.6 | January 2026 | Updated Narrative Pro version to v2.2. Updated Related Documents to Design Document v1.8 and Setup Guide v2.3. |
| 1.5 | January 2026 | Simplified PHASE 1 tag creation - replaced detailed step-by-step instructions with simple tag list table. |
| 1.4 | December 2025 | Changed Grant Location from InitOwnerReference to Player Default Abilities array. Backstab is now a generic player ability, not father-exclusive. Removed BackstabAbilityHandle variable requirement. Removed Phase 6 grant/cleanup architecture. Removed Phase 7 (renumbered to Alternative section). Simplified Prerequisites. Updated Integration Summary. Updated Related Documents to reference Setup Guide v2.0. |
| 1.3 | December 2025 | Changed Grant Location from Player AbilityConfiguration to InitOwnerReference function. GA_Backstab now granted on father recruitment via NE_SetFatherOwner dialogue event. Added BackstabAbilityHandle for two-step cleanup (Cancel Ability + Set Remove Ability On End) in ClearOwnerReference. Updated Prerequisites to include BP_FatherCompanion and BackstabAbilityHandle variable. Replaced PHASE 6 with recruitment-based architecture. Updated Integration Summary. Updated Related Documents to reference Technical Reference v4.6 and Setup Guide v1.4. |
| 1.2 | December 2025 | Added Quick Reference section with tag, variable, effect, and ability configuration summaries. Added Related Documents table. Updated document version. |
| 1.1 | November 2025 | Made guide fully standalone - added Phase 3 with ViewedCharacter variable creation and Handle Perception Update setup; Modular damage system - backstab always applies +25, stealth stacks independently; Added TOC, Prerequisites, Changelog; Fixed UTF-8 characters; Converted flow diagrams to tables |
| 1.0 | November 2025 | Initial implementation with combo effect approach |

---

**END OF GA_BACKSTAB IMPLEMENTATION GUIDE v1.8**

**Passive Damage Bonus - ViewedCharacter Detection**

**Unreal Engine 5.7 + Narrative Pro v2.2**

**Locked Decision: Decision 4 - Universal player ability (not Exoskeleton-only)**

**Blueprint-Only Implementation**
