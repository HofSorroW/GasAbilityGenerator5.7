# GA_Backstab Implementation Guide v1.9

## Passive Damage Bonus Ability - Goal_Attack Query Approach

### Unreal Engine 5.7 + Narrative Pro v2.2

### Blueprint-Only Implementation

---

## DOCUMENT INFORMATION

| Property | Value |
|----------|-------|
| Document Version | 1.9 |
| Ability Name | GA_Backstab |
| Ability Type | Passive (Triggered on Attack Hit) |
| Parent Class | NarrativeGameplayAbility |
| Form Availability | All Forms (Crawler, Armor, Exoskeleton, Symbiote, Engineer) |
| Input Required | None (Auto-Trigger) |
| Grant Location | Player Default Abilities array |
| Detection Method | **Goal_Attack Query** (v5.1) |

---

## VERSION 1.9 CHANGES (Goal_Attack Approach)

| Change | Details |
|--------|---------|
| **Detection Method** | ViewedCharacter → Goal_Attack Query |
| **Phase 3 REMOVED** | No longer need to modify BP_NarrativeNPCController |
| **CheckBackstabCondition** | Now queries NPCActivityComponent → Goal_Attack → TargetToAttack |
| **Full Automation** | No manual perception binding required |
| **Manifest Updated** | GA_Backstab custom_functions use Goal_Attack query |

---

## TABLE OF CONTENTS

| Section | Description |
|---------|-------------|
| Phase 1 | Gameplay Tags Setup |
| Phase 2 | Create Gameplay Effect |
| Phase 3 | **REMOVED** - No NPC Controller changes needed |
| Phase 4 | Create GA_Backstab Ability |
| Phase 5 | Integrate with Attack Ability |
| Phase 6 | Add to Player Default Abilities |

---

## PREREQUISITES

| Requirement | Description |
|-------------|-------------|
| GoalGenerator_Attack | NPCs must have GoalGenerator_Attack in their ActivityConfiguration |
| Player Attack Ability | Existing attack ability with hit detection logic |
| Player Blueprint | BP_NarrativePlayer or similar with Default Abilities array |
| Faction Configuration | NPCs configured with hostile factions (automatic via Narrative Pro) |

**No longer required:**
- ~~BP_NarrativeNPCController modification~~
- ~~ViewedCharacter variable~~
- ~~AIPerception binding~~

---

## QUICK REFERENCE

### AUTOMATION VS MANUAL (v5.1 - Goal_Attack Approach)

| Feature | Automation Status | Manual Steps |
|---------|-------------------|--------------|
| GA_Backstab Blueprint | ✅ Auto-generated | Phase 4 (if manual) |
| GE_BackstabBonus Effect | ✅ Auto-generated | Phase 2 (if manual) |
| CheckBackstabCondition Function | ✅ Auto-generated | Phase 4.14 (if manual) |
| ApplyBackstabBonus Function | ✅ Auto-generated | Phase 4.15 (if manual) |
| NPC Controller Changes | ✅ NOT NEEDED | N/A |
| Player Attack Integration | ⚠️ Manual | Phase 5 |
| Player Default Abilities | ⚠️ Manual | Phase 6 |

**Why Goal_Attack Approach is Fully Automated:**
Narrative Pro's GoalGenerator_Attack automatically creates Goal_Attack when NPCs detect hostile actors via AIPerception. The CheckBackstabCondition function simply queries this existing goal system - no custom perception binding needed.

### How Goal_Attack Detection Works

```
AIPerception detects player
       ↓
GoalGenerator_Attack.OnPerceptionUpdated fires
       ↓
DoesAttitudeMatchFilter (faction check)
       ↓
If HOSTILE → TryAddAttackGoalFromActor
       ↓
Goal_Attack created with TargetToAttack = Player
       ↓
CheckBackstabCondition queries: Does enemy have Goal_Attack targeting me?
       ↓
If NO Goal_Attack OR target != player → BACKSTAB VALID
```

### Tag Configuration Summary

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Backstab |
| Effect Tag | Effect.Backstab |
| Event Tag | Event.Combat.Backstab |
| Activation Required | None (passive, triggered by attack) |
| InputTag | None (auto-trigger) |

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

---

## ABILITY OVERVIEW

| Parameter | Value |
|-----------|-------|
| Detection Method | Goal_Attack Query via NPCActivityComponent |
| Backstab Condition | Enemy has no Goal_Attack targeting the player |
| Backstab Bonus | +25% damage (via AttackRating) |
| Decoy Mechanic | Father distracts enemy (enemy's Goal_Attack targets Father, not player) |

---

## DAMAGE STACKING SYSTEM

| Bonus Source | AttackRating | Managed By |
|--------------|--------------|------------|
| Backstab (enemy not targeting player) | +25 | GA_Backstab |
| Stealth (first attack from stealth) | +50 | GA_StealthField |
| Combined (stacks additively) | +75 | Independent systems |

---

## DAMAGE MULTIPLIER TABLE

| Condition | Enemy Goal_Attack Targeting | Player Stealthed | Multiplier |
|-----------|---------------------------|------------------|------------|
| Normal Attack | Player | No | 1.0x (100%) |
| Backstab Only | Not Player (or none) | No | 1.25x (125%) |
| Stealth Only | Player | Yes | 1.5x (150%) |
| Stealth + Backstab | Not Player (or none) | Yes | 1.75x (175%) |

---

## TACTICAL SCENARIOS (Goal_Attack Approach)

| Scenario | Enemy's Goal_Attack | Backstab Valid |
|----------|---------------------|----------------|
| Enemy attacking Player | Target = Player | No |
| Enemy has no combat goal | No Goal_Attack | Yes |
| Father distracts enemy | Target = Father | Yes |
| Player in Stealth | No Goal_Attack (not detected) | Yes |
| Enemy attacking other NPC | Target = Other NPC | Yes |

---

## DECOY TACTICAL FLOW (v5.1)

| Step | Action | Goal_Attack State |
|------|--------|-------------------|
| 1 | Player sends Father (Crawler form) toward enemy | No Goal_Attack yet |
| 2 | Enemy AIPerception detects Father | GoalGenerator_Attack fires |
| 3 | Faction check: Father = Hostile | TryAddAttackGoalFromActor creates Goal_Attack |
| 4 | Goal_Attack.TargetToAttack = Father | Enemy now attacking Father |
| 5 | Player flanks from behind | Player not detected OR has no Goal_Attack |
| 6 | Player attacks enemy | CheckBackstabCondition runs |
| 7 | Query: Enemy.Goal_Attack.TargetToAttack | Returns Father (not Player) |
| 8 | Compare: Father != Player | TRUE |
| 9 | BACKSTAB VALID | Apply GE_BackstabBonus (+25) |

---

## PHASE 1: GAMEPLAY TAGS SETUP

### Create Required Tags

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Backstab | Identifies backstab ability |
| Effect.Backstab | Applied when backstab bonus active |
| Event.Combat.Backstab | Fired when backstab executed |

**Note:** If using manifest.yaml automation, these tags are auto-generated. For manual creation, follow steps below.

### 1) Open Project Settings

#### 1.1) Access Settings
   - 1.1.1) In Unreal Editor menu bar, click: **Edit**
   - 1.1.2) Select: **Project Settings**
   - 1.1.3) Project Settings window opens

#### 1.2) Navigate to Gameplay Tags
   - 1.2.1) In left panel, find: **Project** section
   - 1.2.2) Click: **GameplayTags**
   - 1.2.3) GameplayTags settings appear in right panel

### 2) Add Ability Tag

#### 2.1) Create Ability.Father.Backstab
   - 2.1.1) In **Gameplay Tag List** section, click: **Add New Gameplay Tag**
   - 2.1.2) In **Name** field, enter: `Ability.Father.Backstab`
   - 2.1.3) In **Comment** field, enter: `Identifies backstab passive ability`
   - 2.1.4) Click: **Add New Tag**

### 3) Add Effect Tag

#### 3.1) Create Effect.Backstab
   - 3.1.1) Click: **Add New Gameplay Tag**
   - 3.1.2) In **Name** field, enter: `Effect.Backstab`
   - 3.1.3) In **Comment** field, enter: `Applied when backstab bonus is active`
   - 3.1.4) Click: **Add New Tag**

### 4) Add Event Tag

#### 4.1) Create Event.Combat.Backstab
   - 4.1.1) Click: **Add New Gameplay Tag**
   - 4.1.2) In **Name** field, enter: `Event.Combat.Backstab`
   - 4.1.3) In **Comment** field, enter: `Fired when backstab is executed`
   - 4.1.4) Click: **Add New Tag**

### 5) Save Project Settings
   - 5.1) Close Project Settings window
   - 5.2) Tags are automatically saved to DefaultGameplayTags.ini

---

## PHASE 2: CREATE GAMEPLAY EFFECT

**Note:** GE_BackstabBonus is auto-generated from manifest.yaml. For manual creation, follow steps below.

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

#### 6.5) Add Granted Tags Component (UE 5.6+)

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

#### 6.6) Add Asset Tags Component (UE 5.6+)

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

## PHASE 3: REMOVED (v5.1)

**This phase is no longer needed with the Goal_Attack approach.**

The previous ViewedCharacter approach required:
- ~~Adding ViewedCharacter variable to BP_NarrativeNPCController~~
- ~~Binding OnPerceptionUpdated delegate manually~~
- ~~Implementing sight detection logic~~

With Goal_Attack, all of this is handled automatically by Narrative Pro's existing GoalGenerator_Attack system.

---

## PHASE 4: CREATE GA_BACKSTAB ABILITY

**Note:** GA_Backstab is auto-generated from manifest.yaml. For manual creation, follow steps below.

### Manifest Definition (for automation)

```yaml
- name: GA_Backstab
  folder: Abilities/Actions
  parent_class: NarrativeGameplayAbility
  instancing_policy: InstancedPerActor
  net_execution_policy: LocalPredicted
  tags:
    ability_tags:
      - Ability.Father.Backstab
    activation_owned_tags:
      - State.BackstabReady
  custom_functions:
    - function_name: CheckBackstabCondition
      pure: false
      inputs:
        - name: EnemyActor
          type: Actor
      outputs:
        - name: CanBackstab
          type: Boolean
    - function_name: ApplyBackstabBonus
      pure: false
```

### 12) Create Ability Blueprint

#### 12.1) Navigate to Abilities Folder
   - 12.1.1) In Content Browser, navigate to: `/Content/FatherCompanion/Abilities/`
   - 12.1.2) If folder does not exist, create it

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

### 14) Create CheckBackstabCondition Function (Goal_Attack Query)

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

#### 14.4) Get Player Reference

##### 14.4.1) Add Get Avatar Actor Node
   - 14.4.1.1) Right-click in empty graph space
   - 14.4.1.2) Search: `Get Avatar Actor from Actor Info`
   - 14.4.1.3) Select: **Get Avatar Actor from Actor Info**
   - 14.4.1.4) Position at left side of graph

#### 14.5) Get NPCActivityComponent from Enemy

##### 14.5.1) Add Get Component by Class Node
   - 14.5.1.1) Drag from: **EnemyActor** input pin
   - 14.5.1.2) Release in empty space
   - 14.5.1.3) Search: `Get Component by Class`
   - 14.5.1.4) Select: **Get Component by Class**
   - 14.5.1.5) In **Component Class** dropdown, select: `NPCActivityComponent`

#### 14.6) Check Component Validity

##### 14.6.1) Add IsValid Node
   - 14.6.1.1) Drag from: **Get Component by Class** Return Value
   - 14.6.1.2) Search: `Is Valid`
   - 14.6.1.3) Select: **Is Valid** (from KismetSystemLibrary)

##### 14.6.2) Add NOT Node for No-Component Case
   - 14.6.2.1) Drag from: **IsValid** Return Value
   - 14.6.2.2) Search: `NOT Boolean`
   - 14.6.2.3) Select: **NOT Boolean**
   - 14.6.2.4) This gives TRUE when enemy has no NPCActivityComponent

#### 14.7) Get Current Activity Goal

##### 14.7.1) Add GetCurrentActivityGoal Node
   - 14.7.1.1) Drag from: **Get Component by Class** Return Value
   - 14.7.1.2) Search: `Get Current Activity Goal`
   - 14.7.1.3) Select: **Get Current Activity Goal**

#### 14.8) Cast to Goal_Attack

##### 14.8.1) Add Cast Node
   - 14.8.1.1) Drag from: **Get Current Activity Goal** Return Value
   - 14.8.1.2) Search: `Cast to Goal_Attack`
   - 14.8.1.3) Select: **Cast To Goal_Attack**

#### 14.9) Get TargetToAttack Property

##### 14.9.1) Access Target Property
   - 14.9.1.1) Drag from: **As Goal Attack** output pin
   - 14.9.1.2) Search: `Target to Attack`
   - 14.9.1.3) Select: **Get Target to Attack**

#### 14.10) Compare Target to Player

##### 14.10.1) Add Not Equal Node
   - 14.10.1.1) Drag from: **Target to Attack** Return Value
   - 14.10.1.2) Search: `Not Equal (Object)`
   - 14.10.1.3) Select: **Not Equal (Object)**
   - 14.10.1.4) Connect: **Get Avatar Actor** Return Value to second input (B pin)

#### 14.11) Handle Cast Failure (No Goal_Attack)

##### 14.11.1) Check if Goal is Valid
   - 14.11.1.1) Drag from: **As Goal Attack** output pin
   - 14.11.1.2) Search: `Is Valid`
   - 14.11.1.3) Select: **Is Valid**

##### 14.11.2) Add Select Node
   - 14.11.2.1) Right-click in empty space
   - 14.11.2.2) Search: `Select`
   - 14.11.2.3) Select: **Select** (Boolean version)
   - 14.11.2.4) Connect: **Is Valid** (Goal) Return Value to **Pick A** input
   - 14.11.2.5) Connect: **Not Equal** Return Value to **A** input
   - 14.11.2.6) Set **B** input to: `True` (checked)
   - 14.11.2.7) Result: If valid goal, use NotEqual result; else return True

#### 14.12) Combine with No-Component Case

##### 14.12.1) Add OR Node
   - 14.12.1.1) Right-click in empty space
   - 14.12.1.2) Search: `OR Boolean`
   - 14.12.1.3) Select: **OR Boolean**
   - 14.12.1.4) Connect: **NOT Boolean** (from step 14.6.2) to first input
   - 14.12.1.5) Connect: **Select** Return Value to second input

#### 14.13) Add Return Node

##### 14.13.1) Connect to Output
   - 14.13.1.1) Drag from: **OR Boolean** Return Value
   - 14.13.1.2) Search: `Return Node`
   - 14.13.1.3) Or drag to the **CanBackstab** output on the Return Node
   - 14.13.1.4) Connect: **OR Boolean** result to **CanBackstab** output

### Logic Summary

| Condition | Result |
|-----------|--------|
| Enemy has no NPCActivityComponent | TRUE (can backstab) |
| Enemy has no Goal_Attack | TRUE (can backstab) |
| Enemy's Goal_Attack targets other | TRUE (can backstab) |
| Enemy's Goal_Attack targets player | FALSE (no backstab) |

### Node Graph Flow

```
[FunctionEntry: EnemyActor]
         ↓
[GetComponentByClass] → NPCActivityComponent
         ↓
[IsValid] → [NOT] → ─────────────────────────────┐
         ↓                                        │
[GetCurrentActivityGoal]                          │
         ↓                                        │
[Cast to Goal_Attack]                             │
         ↓ (As Goal Attack)                       │
[IsValid] ─────────────────────────────┐          │
         ↓                              ↓          │
[GetProperty:                     [Select] ←──────┤
 TargetToAttack]                       ↑          │
         ↓                              │          │
[GetAvatarActor] ──→ [NotEqual] ───────┘          │
                                       ↓          │
                                 [OR Boolean] ←───┘
                                       ↓
                              [FunctionResult: CanBackstab]
```

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
   - 15.2.1.3) Select: **BP Apply Gameplay Effect to Owner**
   - 15.2.1.4) In **Gameplay Effect Class** dropdown: Select `GE_BackstabBonus`
   - 15.2.1.5) Set **Gameplay Effect Level**: `1`

##### 15.2.2) Add Return
   - 15.2.2.1) From: Apply Gameplay Effect execution output
   - 15.2.2.2) Drag and add: **Return Node**

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
   - 17.1.2) Find: Your primary melee attack ability (e.g., GA_Attack_Melee)
   - 17.1.3) Double-click: To open ability blueprint

### 18) Add Backstab Check to Hit Detection

#### 18.1) Locate Hit Processing Logic
   - 18.1.1) In Event Graph, find where attack hit is processed
   - 18.1.2) This is typically after trace hit or overlap detection
   - 18.1.3) Look for nodes like: **Break Hit Result**, **Get Hit Actor**, or damage application
   - 18.1.4) Identify the Hit Actor reference point

#### 18.2) Create Local Variable for GA_Backstab Reference

##### 18.2.1) Add Variable
   - 18.2.1.1) In My Blueprint panel, find: **Variables** section
   - 18.2.1.2) Click: **+** button
   - 18.2.1.3) Name: `BackstabAbilityRef`
   - 18.2.1.4) Set Type: `GA_Backstab` (Object Reference)

#### 18.3) Get Backstab Ability Reference

##### 18.3.1) Get ASC
   - 18.3.1.1) At start of hit processing logic
   - 18.3.1.2) Right-click in empty space
   - 18.3.1.3) Search: `Get Ability System Component from Actor Info`
   - 18.3.1.4) Select: **Get Ability System Component from Actor Info**

##### 18.3.2) Get Activatable Abilities
   - 18.3.2.1) Drag from: ASC Return Value
   - 18.3.2.2) Search: `Get Activatable Abilities`
   - 18.3.2.3) Select: **Get Activatable Abilities**

##### 18.3.3) Find Backstab Ability
   - 18.3.3.1) Drag from: **Activatable Abilities** array output
   - 18.3.3.2) Search: `For Each Loop`
   - 18.3.3.3) Select: **For Each Loop**
   - 18.3.3.4) From **Array Element** pin:
   - 18.3.3.5) Search: `Get Class`
   - 18.3.3.6) Select: **Get Class**
   - 18.3.3.7) Add: **Equal (Class)** node
   - 18.3.3.8) In second input, select: `GA_Backstab`
   - 18.3.3.9) Add: **Branch** node connected to Equal result
   - 18.3.3.10) From **True** execution:
   - 18.3.3.11) Add: **Cast To GA_Backstab** node
   - 18.3.3.12) Connect: **Array Element** to Cast Object input
   - 18.3.3.13) From Cast success, add: **Set BackstabAbilityRef** node
   - 18.3.3.14) Connect: **As GA Backstab** to variable input
   - 18.3.3.15) After loop, add: **Break** node to exit once found

#### 18.4) Add Backstab Check

##### 18.4.1) Check Condition Before Damage
   - 18.4.1.1) After: Hit actor is validated as enemy
   - 18.4.1.2) Before: Damage effect is applied
   - 18.4.1.3) Drag from: **BackstabAbilityRef** variable
   - 18.4.1.4) Search: `Check Backstab Condition`
   - 18.4.1.5) Select: **Check Backstab Condition**
   - 18.4.1.6) Connect: Hit Actor to **EnemyActor** input

##### 18.4.2) Branch on Result
   - 18.4.2.1) Drag from: **Check Backstab Condition** execution output
   - 18.4.2.2) Add: **Branch** node
   - 18.4.2.3) Connect: **CanBackstab** output to Branch **Condition**

##### 18.4.3) Apply Backstab Bonus (True Branch)
   - 18.4.3.1) From Branch **True** execution:
   - 18.4.3.2) Drag from: **BackstabAbilityRef** variable
   - 18.4.3.3) Search: `Apply Backstab Bonus`
   - 18.4.3.4) Select: **Apply Backstab Bonus**

##### 18.4.4) Continue to Damage (Both Branches)
   - 18.4.4.1) From **Apply Backstab Bonus** output:
   - 18.4.4.2) Connect to: Damage application logic
   - 18.4.4.3) From Branch **False** execution:
   - 18.4.4.4) Connect directly to: Damage application logic

### 19) Add Visual and Audio Feedback (Optional)

#### 19.1) Backstab Hit Effects

##### 19.1.1) Add Particle Effect
   - 19.1.1.1) After: **Apply Backstab Bonus** call
   - 19.1.1.2) Before: Damage application
   - 19.1.1.3) Right-click, search: `Spawn Emitter at Location`
   - 19.1.1.4) Select: **Spawn Emitter at Location**
   - 19.1.1.5) **Emitter Template**: Your critical hit VFX
   - 19.1.1.6) **Location**: Hit location from trace result

##### 19.1.2) Add Sound Effect
   - 19.1.2.1) Right-click, search: `Play Sound at Location`
   - 19.1.2.2) Select: **Play Sound at Location**
   - 19.1.2.3) **Sound**: Your backstab/critical hit sound cue
   - 19.1.2.4) **Location**: Hit location

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
   - 21.1.2) Find: `BP_NarrativePlayer` (or your player blueprint)
   - 21.1.3) Double-click: To open blueprint
   - 21.1.4) Click: **Class Defaults** button

#### 21.2) Find Default Abilities Array
   - 21.2.1) In Details panel: Search `abilit`
   - 21.2.2) Locate: **Narrative** -> **Abilities** category
   - 21.2.3) Find: **Default Abilities** array

#### 21.3) Add GA_Backstab
   - 21.3.1) Click: **+** button to add element
   - 21.3.2) New element appears at end of array
   - 21.3.3) Click: Dropdown
   - 21.3.4) Search: `GA_Backstab`
   - 21.3.5) Select: **GA_Backstab**

#### 21.4) Verify Array
   - 21.4.1) GA_Backstab should now be in Default Abilities
   - 21.4.2) Position in array does not matter

### 22) Save Player Blueprint

#### 22.1) Finalize
   - 22.1.1) Click: **Compile** button
   - 22.1.2) Verify: No compilation errors
   - 22.1.3) Click: **Save** button
   - 22.1.4) Close: Player blueprint

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

##### 23.2.3) Build Logic (Goal_Attack Query)
   - 23.2.3.1) From **HitEnemy**: Add **Get Component by Class** (NPCActivityComponent)
   - 23.2.3.2) Add: **Is Valid** check on component
   - 23.2.3.3) Add: **NOT Boolean** for no-component case
   - 23.2.3.4) From component: Add **Get Current Activity Goal**
   - 23.2.3.5) Add: **Cast To Goal_Attack**
   - 23.2.3.6) From cast result: Add **Is Valid** check
   - 23.2.3.7) From cast result: Add **Get Target to Attack**
   - 23.2.3.8) Add: **Get Avatar Actor from Actor Info** (player reference)
   - 23.2.3.9) Add: **Not Equal (Object)** comparing target to player
   - 23.2.3.10) Add: **Select** node (valid goal -> NotEqual result, invalid -> True)
   - 23.2.3.11) Add: **OR Boolean** combining no-component and select result
   - 23.2.3.12) Add: **Return Node** with OR result to **IsValid** output

#### 23.3) Create ApplyBackstabEffect Function

##### 23.3.1) Add Function
   - 23.3.1.1) Name: `ApplyBackstabEffect`

##### 23.3.2) Build Logic
   - 23.3.2.1) Add: **Get Ability System Component from Actor Info** node
   - 23.3.2.2) From ASC: Add **Apply Gameplay Effect to Self** node
   - 23.3.2.3) **Gameplay Effect Class**: `GE_BackstabBonus`
   - 23.3.2.4) **Level**: `1`
   - 23.3.2.5) Add: **Return Node**

### 24) Integrate Into Hit Processing

#### 24.1) After Hit Validation
   - 24.1.1) Call: **IsBackstabValid**(HitActor)
   - 24.1.2) Add: **Branch** on result
   - 24.1.3) If True: Call **ApplyBackstabEffect**
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
| `GE_BackstabBonus` | +25 | Enemy Goal_Attack.TargetToAttack != Player |
| `GE_StealthDamageBonus` | +50 | Player stealthed (managed by GA_StealthField) |

---

## MULTIPLAYER REPLICATION

| Setting | Value | Purpose |
|---------|-------|---------|
| Instancing Policy | Instanced Per Actor | Each actor has own instance |
| Net Execution Policy | Local Predicted | Client predicts, server validates |
| Replication Policy | Replicate Yes | Effect syncs to all clients |
| GE Duration | Instant | Applied and consumed immediately |
| Goal_Attack | Server Authority | Goal system managed by server |

---

## INTEGRATION SUMMARY

| Component | Modification Required |
|-----------|----------------------|
| BP_NarrativeNPCController | **NONE** (v5.1 - uses existing goal system) |
| Player Blueprint | Add GA_Backstab to Default Abilities array |
| NPCs | Must have GoalGenerator_Attack in ActivityConfiguration |
| Player Attack Ability | Add backstab check logic |
| GA_StealthField | None (stealth bonus stacks independently) |

---

## RELATED DOCUMENTS

| Document | Purpose |
|----------|---------|
| Father_Companion_Technical_Reference_v6_4.md | Section 5.5: Backstab Detection Logic (Goal_Attack Query) |
| Father_Companion_Technical_Reference_v6_4.md | Section 7.9: Faction + GoalGenerator Attack Chain |
| Father_Companion_Technical_Reference_v6_4.md | Section 23: Goal System (Goal_Attack) |
| Father_Companion_System_Design_Document_v2_6.md | Section 3.1: GA_Backstab overview |
| GA_StealthField_Implementation_Guide_v3_2.md | Stealth damage bonus that stacks with backstab |
| manifest.yaml | GA_Backstab definition with Goal_Attack query nodes |

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 1.9 | January 2026 | **v5.1 Goal_Attack Approach:** Replaced ViewedCharacter detection with Goal_Attack query. REMOVED Phase 3 (no NPC controller changes needed). CheckBackstabCondition now queries NPCActivityComponent → GetCurrentActivityGoal → Cast to Goal_Attack → TargetToAttack. Full automation - no manual perception binding required. **Added detailed step-by-step manual creation instructions** for all phases. Updated all tables and flow diagrams. |
| 1.8 | January 2026 | AUTOMATION VS MANUAL Table Added: Clarified what is auto-generated vs manual. |
| 1.7 | January 2026 | Locked Decisions Reference: Decision 4 (GA_Backstab is UNIVERSAL player ability). |
| 1.6 | January 2026 | Updated Narrative Pro version to v2.2. |
| 1.5 | January 2026 | Simplified PHASE 1 tag creation. |
| 1.4 | December 2025 | Changed Grant Location to Player Default Abilities array. |
| 1.3 | December 2025 | Changed Grant Location to InitOwnerReference function. |
| 1.2 | December 2025 | Added Quick Reference section. |
| 1.1 | November 2025 | Made guide fully standalone. |
| 1.0 | November 2025 | Initial implementation. |

---

**END OF GA_BACKSTAB IMPLEMENTATION GUIDE v1.9**

**Passive Damage Bonus - Goal_Attack Query Approach**

**Unreal Engine 5.7 + Narrative Pro v2.2**

**Locked Decision: Decision 4 - Universal player ability (not Exoskeleton-only)**

**v5.1: Fully Automated via Narrative Pro Goal System OR Manual Step-by-Step Creation**
