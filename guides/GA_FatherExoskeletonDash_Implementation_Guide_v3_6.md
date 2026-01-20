# GA_FatherExoskeletonDash Implementation Guide
## Version 3.8 - INV-1 Compliance (Invulnerability Removed)

---

## **DOCUMENT INFORMATION**

| Property | Value |
|----------|-------|
| Ability Name | GA_FatherExoskeletonDash |
| Ability Type | Action - Movement Enhancement |
| Parent Class | NarrativeGameplayAbility |
| Form | Exoskeleton (active only when attached) |
| Input | Double-press directional keys (W/A/S/D) |
| Version | 3.8 |
| Engine | Unreal Engine 5.7 |
| Plugin | Narrative Pro v2.2 |

---

## **TABLE OF CONTENTS**

- [INTRODUCTION](#introduction)
- [PREREQUISITES](#prerequisites)
- [PHASE 1: GAMEPLAY TAGS SETUP](#phase-1-gameplay-tags-setup)
- [PHASE 2: GAMEPLAY EFFECTS CREATION](#phase-2-gameplay-effects-creation)
- [PHASE 3: DOUBLE-PRESS INPUT COMPONENT](#phase-3-double-press-input-component)
- [PHASE 4: DASH GAMEPLAY ABILITY](#phase-4-dash-gameplay-ability)
- [PHASE 5: INTEGRATION](#phase-5-integration)
- [PHASE 6: INPUT CONFIGURATION](#phase-6-input-configuration)
- [QUICK REFERENCE](#quick-reference)

---

## **INTRODUCTION**

### **Ability Overview**

GA_FatherExoskeletonDash is a high-mobility action ability exclusive to the Exoskeleton form. Players activate dashes by double-tapping directional movement keys (W/A/S/D), launching themselves in the corresponding direction at high speed. During the dash, the player deals damage to enemies they pass through, with knockback applied radially from the player position.

> **INV-1 Note:** Per GAS Audit decision INV-1, dash invulnerability was REMOVED. The dash no longer grants damage immunity - only GA_FatherSacrifice provides invulnerability (to the player for 8 seconds).

### **Key Features**

| Feature | Description |
|---------|-------------|
| Double-Press Input | Detection on W/A/S/D keys for directional control |
| Four-Direction Dash | Forward, Backward, Left, Right |
| Dash Duration | 0.5 seconds |
| Enemy Damage | Deals damage to enemies passed through |
| Knockback | Radial impulse from player position |
| Cooldown | Prevents spam activation |
| Multiplayer | Local Predicted execution |

### **Activation Flow**

| Step | Action | Result |
|------|--------|--------|
| 1 | Player in Exoskeleton form | Form requirement met |
| 2 | Player double-taps W key | Input component detects double-press |
| 3 | Component calls TriggerDash(Forward) | Sets PendingDashDirection variable |
| 4 | GA_FatherExoskeletonDash activates | Checks Required Tags and Cooldown |
| 5 | Set CharacterMovement speed | Boosts Max Walk Speed |
| 6 | Launch Character | Applies velocity in dash direction |
| 7 | Detect nearby enemies | Sphere overlap for Character.Enemy tag |
| 8 | Apply GE_DashDamage | Deals damage per enemy |
| 9 | Apply knockback | Radial impulse from player |
| 10 | Wait for duration | Dash completes |
| 11 | Restore original speed | Reset Max Walk Speed |
| 12 | Apply GE_DashCooldown | Cooldown starts |
| 13 | End ability | Cleanup |

### **Form Restriction**

| Form | Can Use Dash | Reason |
|------|--------------|--------|
| Crawler | No | Not attached to player |
| Armor | No | Different form abilities |
| Exoskeleton | Yes | Ability exclusive to this form |
| Symbiote | No | Different form abilities |
| Engineer | No | Different form abilities |

---

## **PREREQUISITES**

### **Required Assets**

| Requirement | Description |
|-------------|-------------|
| BP_FatherCompanion | Father companion character (NarrativeNPCCharacter base) |
| GA_FatherExoskeleton | Exoskeleton form ability (must be implemented) |
| EQI_FatherExoskeleton | Exoskeleton EquippableItem for ability granting |
| Player Character | Character with NarrativeAbilitySystemComponent |

### **Required Tags**

| Tag | Purpose |
|-----|---------|
| Ability.Father.Exoskeleton.Dash | Identifies this ability |
| Father.State.Dashing | Active dash state indicator |
| Effect.Father.FormState.Exoskeleton | Form requirement |
| Father.State.Attached | Attachment requirement |
| Father.State.Recruited | Recruitment requirement |
| Character.Enemy | Enemy identification |
| Cooldown.Father.Exoskeleton.Dash | Cooldown tracking |

---

## **PHASE 1: GAMEPLAY TAGS SETUP**

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Exoskeleton.Dash | Exoskeleton dash ability |
| Father.State.Dashing | Father is currently dashing |
| Cooldown.Father.Exoskeleton.Dash | Cooldown tag for dash ability |

### **Verify Existing Tags**

| Tag Name | Purpose |
|----------|---------|
| Effect.Father.FormState.Exoskeleton | Form requirement (Activation Required) |
| Father.State.Attached | Attachment requirement (Activation Required) |
| Father.State.Recruited | Recruitment requirement (Activation Required) |
| Character.Enemy | Enemy identification for damage detection |

---

## **PHASE 2: GAMEPLAY EFFECTS CREATION**

> **INV-1 Note:** GE_DashInvulnerability was REMOVED per GAS Audit decision INV-1. Dash no longer grants damage immunity.

### **1) Create GE_DashDamage**

#### 2.1) Create GameplayEffect Blueprint
   - 2.1.1) Right-click in Content Browser -> Blueprint Class -> GameplayEffect
   - 2.1.2) Name: `GE_DashDamage`
   - 2.1.3) Save in: `/Content/FatherCompanion/Abilities/ExoskeletonDash/Effects/`

#### 2.2) Configure GE_DashDamage Properties

| Property | Value |
|----------|-------|
| Duration Policy | Instant |

#### 2.3) Configure GE_DashDamage Modifiers

| Modifier | Attribute | Modifier Op | Magnitude Type | Value |
|----------|-----------|-------------|----------------|-------|
| [0] | Damage (meta-attribute) | Add | Scalable Float | 15.0 |

#### 2.4) Configure GE_DashDamage Executions

| Property | Value |
|----------|-------|
| Calculation Class | NarrativeDamageExecCalc |

#### 2.5) Compile and Save

### **3) Create GE_DashCooldown**

#### 3.1) Create GameplayEffect Blueprint
   - 3.1.1) Right-click in Content Browser -> Blueprint Class -> GameplayEffect
   - 3.1.2) Name: `GE_DashCooldown`
   - 3.1.3) Save in: `/Content/FatherCompanion/Abilities/ExoskeletonDash/Effects/`

#### 3.2) Configure GE_DashCooldown Properties

| Property | Value |
|----------|-------|
| Duration Policy | Has Duration |
| Magnitude Calculation Type | Scalable Float |
| Scalable Float Value | 3.0 |

#### 3.3) Configure GE_DashCooldown Components

| Component | Configuration |
|-----------|---------------|
| Grant Tags to Target Actor | Cooldown.Father.Exoskeleton.Dash |

#### 3.4) Compile and Save

---

## **PHASE 3: DOUBLE-PRESS INPUT COMPONENT**

### **1) Create Component Blueprint**

#### 1.1) Create ActorComponent
   - 1.1.1) Right-click in Content Browser
   - 1.1.2) Select Blueprint Class
   - 1.1.3) Search for: `ActorComponent`
   - 1.1.4) Select ActorComponent
   - 1.1.5) Name: `BP_DoublePressInputComponent`
   - 1.1.6) Save in: `/Content/FatherCompanion/Components/`

### **2) Create Component Variables**

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| DoublePressTimeWindow | Float | 0.3 | Yes |
| LastPressTime | Float | 0.0 | No |
| CurrentKey | Name | None | No |

### **3) Create DetectDoublePress Function**

#### 3.1) Create Function
   - 3.1.1) In My Blueprint panel, click + Function
   - 3.1.2) Function Name: `DetectDoublePress`

#### 3.2) Add Function Input
   - 3.2.1) Select DetectDoublePress function
   - 3.2.2) In Details panel, find Inputs section
   - 3.2.3) Click + to add input
   - 3.2.4) Input Name: `KeyName`
   - 3.2.5) Input Type: Name

#### 3.3) Add Function Output
   - 3.3.1) Find Outputs section
   - 3.3.2) Click + to add output
   - 3.3.3) Output Name: `IsDoublePress`
   - 3.3.4) Output Type: Boolean

#### 3.4) Implement Detection Logic
   - 3.4.1) Add Get Game Time in Seconds node
   - 3.4.2) Add Get LastPressTime node
   - 3.4.3) Add Subtract (Float - Float) node
   - 3.4.4) Connect Game Time to A pin
   - 3.4.5) Connect LastPressTime to B pin
   - 3.4.6) Add Get DoublePressTimeWindow node
   - 3.4.7) Add Less or Equal (Float <= Float) node
   - 3.4.8) Connect Subtract result to A pin
   - 3.4.9) Connect TimeWindow to B pin
   - 3.4.10) Add Get CurrentKey node
   - 3.4.11) Add Equal (Name == Name) node
   - 3.4.12) Connect CurrentKey to A pin
   - 3.4.13) Connect KeyName input to B pin
   - 3.4.14) Add AND (Boolean) node
   - 3.4.15) Connect Less or Equal output to first input
   - 3.4.16) Connect Equal output to second input
   - 3.4.17) Connect AND output to IsDoublePress return

#### 3.5) Update Tracking Variables
   - 3.5.1) Add Set LastPressTime node
   - 3.5.2) Connect Get Game Time in Seconds to value
   - 3.5.3) Add Set CurrentKey node
   - 3.5.4) Connect KeyName input to value

#### 3.6) Compile and Save
   - 3.6.1) Click Compile
   - 3.6.2) Click Save

### **4) Create TriggerDash Function**

#### 4.1) Create Function
   - 4.1.1) Click + Function
   - 4.1.2) Function Name: `TriggerDash`

#### 4.2) Add Function Input
   - 4.2.1) Select TriggerDash function
   - 4.2.2) In Details panel, find Inputs section
   - 4.2.3) Click + to add input
   - 4.2.4) Input Name: `DashDirection`
   - 4.2.5) Input Type: Vector

#### 4.3) Implement Trigger Logic
   - 4.3.1) Add Get Owner node
   - 4.3.2) Add Cast To NarrativePlayerCharacter node
   - 4.3.3) Connect Owner to Object pin
   - 4.3.4) From Cast success execution pin:
   - 4.3.4.1) Add Set PendingDashDirection node
   - 4.3.4.2) Connect DashDirection input to value
   - 4.3.5) From Set execution pin:
   - 4.3.5.1) Add Get Ability System Component node
   - 4.3.5.2) Target: As Narrative Player Character
   - 4.3.6) From ASC Return Value:
   - 4.3.6.1) Add Try Activate Ability By Class node
   - 4.3.6.2) Ability Class: GA_FatherExoskeletonDash

#### 4.4) Compile and Save
   - 4.4.1) Click Compile
   - 4.4.2) Click Save

---

## **PHASE 4: DASH GAMEPLAY ABILITY**

### **1) Create Ability Blueprint**

#### 1.1) Create NarrativeGameplayAbility
   - 1.1.1) Right-click in Content Browser
   - 1.1.2) Select Blueprint Class
   - 1.1.3) Expand All Classes
   - 1.1.4) Search for: `NarrativeGameplayAbility`
   - 1.1.5) Select NarrativeGameplayAbility
   - 1.1.6) Name: `GA_FatherExoskeletonDash`
   - 1.1.7) Save in: `/Content/FatherCompanion/Abilities/ExoskeletonDash/`

### **2) Configure Ability Properties**

#### 2.1) Open Class Defaults
   - 2.1.1) Double-click to open
   - 2.1.2) Click Class Defaults button

#### 2.2) Set Ability Tags
   - 2.2.1) Find Ability Tags section
   - 2.2.2) Expand Asset Tags
   - 2.2.3) Click + button
   - 2.2.4) Select: `Ability.Father.Exoskeleton.Dash`

#### 2.3) Set Block Abilities with Tag
   - 2.3.1) Find Block Abilities with Tag section
   - 2.3.2) Click + button
   - 2.3.3) Select: `Father.State.Dashing`
   - 2.3.4) Click + button
   - 2.3.5) Select: `Cooldown.Father.Exoskeleton.Dash`

#### 2.4) Set Activation Required Tags
   - 2.4.1) Find Activation Required Tags section
   - 2.4.2) Click + button
   - 2.4.3) Element [0]: Select `Effect.Father.FormState.Exoskeleton`
   - 2.4.4) Click + button
   - 2.4.5) Element [1]: Select `Father.State.Attached`
   - 2.4.6) Click + button
   - 2.4.7) Element [2]: Select `Father.State.Recruited`

#### 2.5) Set Activation Owned Tags
   - 2.5.1) Find Activation Owned Tags section
   - 2.5.2) Click + button
   - 2.5.3) Select: `Father.State.Dashing`

#### 2.6) Configure Narrative Ability Settings
   - 2.6.1) Find Narrative Ability category
   - 2.6.2) Set bActivateAbilityOnGranted: Unchecked

#### 2.7) Configure Multiplayer Settings
   - 2.7.1) Find Instancing Policy dropdown
   - 2.7.2) Select: Instanced Per Actor
   - 2.7.3) Find Replication Policy dropdown
   - 2.7.4) Select: Replicate Yes
   - 2.7.5) Find Net Execution Policy dropdown
   - 2.7.6) Select: Local Predicted

### **3) Create Ability Variables**

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| DashSpeed | Float | 3000.0 | Yes |
| OriginalMaxWalkSpeed | Float | 600.0 | No |
| DashDuration | Float | 0.5 | Yes |
| DashForce | Float | 2000.0 | Yes |
| DamageRadius | Float | 200.0 | Yes |
| KnockbackForce | Float | 1000.0 | Yes |
| PlayerRef | NarrativeCharacter (Object Reference) | None | No |
| PlayerASC | NarrativeAbilitySystemComponent (Object Reference) | None | No |

### **4) Implement Dash Logic**

#### 4.1) Create Event ActivateAbility
   - 4.1.1) Click Event Graph tab
   - 4.1.2) Right-click in Event Graph
   - 4.1.3) Search for: `Event ActivateAbility`
   - 4.1.4) Select Event ActivateAbility

#### 4.2) Get Player Reference
   - 4.2.1) From Event ActivateAbility execution pin:
   - 4.2.1.1) Add GetOwningNarrativeCharacter node
   - 4.2.2) From Return Value:
   - 4.2.2.1) Add Set PlayerRef node
   - 4.2.2.2) Connect Return Value to value

#### 4.3) Validate PlayerRef
   - 4.3.1) From Set PlayerRef execution pin:
   - 4.3.1.1) Add IsValid node
   - 4.3.1.2) Connect PlayerRef to Input Object
   - 4.3.2) From IsValid Return Value:
   - 4.3.2.1) Add Branch node
   - 4.3.2.2) Connect IsValid to Condition
   - 4.3.3) From Branch False execution:
   - 4.3.3.1) Add End Ability node
   - 4.3.3.2) Was Cancelled: Checked

#### 4.4) Get and Store PlayerASC
   - 4.4.1) From Branch True execution:
   - 4.4.1.1) Drag from PlayerRef variable
   - 4.4.1.2) Add Get Ability System Component node
   - 4.4.2) From ASC Return Value:
   - 4.4.2.1) Add Cast To NarrativeAbilitySystemComponent node
   - 4.4.3) From Cast Success execution:
   - 4.4.3.1) Add Set PlayerASC node
   - 4.4.3.2) Connect As NarrativeAbilitySystemComponent to value
   - 4.4.4) From Cast Failure execution:
   - 4.4.4.1) Add End Ability node
   - 4.4.4.2) Was Cancelled: Checked

#### 4.5) Validate PlayerASC
   - 4.5.1) From Set PlayerASC execution:
   - 4.5.1.1) Add Is Valid node
   - 4.5.1.2) Connect PlayerASC to Input Object
   - 4.5.2) From Is Valid Return Value:
   - 4.5.2.1) Add Branch node
   - 4.5.2.2) Connect Is Valid to Condition
   - 4.5.3) From Branch False execution:
   - 4.5.3.1) Add End Ability node
   - 4.5.3.2) Was Cancelled: Checked

#### 4.6) Store Original Speed
   - 4.6.1) From Branch True execution:
   - 4.6.1.1) Add Get PlayerRef node
   - 4.6.1.2) Add Get Character Movement node
   - 4.6.1.3) Connect PlayerRef to Target
   - 4.6.2) From Character Movement Return Value:
   - 4.6.2.1) Add Get Max Walk Speed node
   - 4.6.3) From Max Walk Speed Return Value:
   - 4.6.3.1) Add Set OriginalMaxWalkSpeed node
   - 4.6.3.2) Connect Max Walk Speed to value

#### 4.7) Apply Speed Boost
   - 4.7.1) From Set OriginalMaxWalkSpeed execution pin:
   - 4.7.1.1) Add Get Character Movement node
   - 4.7.1.2) Connect PlayerRef to Target
   - 4.7.2) From Character Movement:
   - 4.7.2.1) Add Set Max Walk Speed node
   - 4.7.2.2) Connect Character Movement to Target
   - 4.7.3) Add Get DashSpeed node
   - 4.7.4) Connect DashSpeed to Max Walk Speed input

#### 4.8) Apply Invulnerability Effect
   - 4.8.1) From Set Max Walk Speed execution pin:
   - 4.8.1.1) Drag from PlayerASC variable
   - 4.8.1.2) Add Apply Gameplay Effect to Self node
   - 4.8.1.3) Connect PlayerASC to Target
   - 4.8.1.4) Gameplay Effect Class: GE_DashInvulnerability

#### 4.9) Calculate Dash Direction
   - 4.9.1) From Apply GE execution pin:
   - 4.9.1.1) Add Get PendingDashDirection node (from PlayerRef)
   - 4.9.2) From PlayerRef:
   - 4.9.2.1) Add Get Control Rotation node
   - 4.9.3) From Control Rotation:
   - 4.9.3.1) Add Get Forward Vector node
   - 4.9.3.2) Add Get Right Vector node
   - 4.9.4) Calculate final direction:
   - 4.9.4.1) Add Break Vector node on PendingDashDirection
   - 4.9.4.2) Add Multiply (Vector * Float) node for Forward
   - 4.9.4.3) Connect Forward Vector to first input
   - 4.9.4.4) Connect X component to second input
   - 4.9.4.5) Add Multiply (Vector * Float) node for Right
   - 4.9.4.6) Connect Right Vector to first input
   - 4.9.4.7) Connect Y component to second input
   - 4.9.4.8) Add Add (Vector + Vector) node
   - 4.9.4.9) Connect both multiply results

#### 4.10) Launch Character
   - 4.10.1) From Add Vector:
   - 4.10.1.1) Add Multiply (Vector * Float) node
   - 4.10.1.2) Connect direction to first input
   - 4.10.1.3) Connect DashForce variable to second input
   - 4.10.2) Add Launch Character node
   - 4.10.2.1) Connect PlayerRef to Target
   - 4.10.2.2) Connect multiplied vector to Launch Velocity
   - 4.10.2.3) XY Override: Checked
   - 4.10.2.4) Z Override: Unchecked

#### 4.11) Detect and Damage Enemies
   - 4.11.1) From Launch Character execution pin:
   - 4.11.1.1) Add Get Actor Location node
   - 4.11.1.2) Connect PlayerRef to Target
   - 4.11.2) Add Sphere Overlap Actors node
   - 4.11.2.1) Connect Actor Location to Sphere Pos
   - 4.11.2.2) Connect DamageRadius to Sphere Radius
   - 4.11.2.3) Object Types: Pawn
   - 4.11.3) From Out Actors:
   - 4.11.3.1) Add For Each Loop node
   - 4.11.3.2) Connect Out Actors to Array

#### 4.12) Filter and Damage Enemies
   - 4.12.1) In For Each Loop body:
   - 4.12.1.1) Add Get Ability System Component node
   - 4.12.1.2) Connect Array Element to Actor
   - 4.12.2) From ASC Return Value:
   - 4.12.2.1) Add Has Matching Gameplay Tag node
   - 4.12.2.2) Tag to Match: `Character.Enemy`
   - 4.12.3) From Has Tag Return Value:
   - 4.12.3.1) Add Branch node
   - 4.12.3.2) Connect Has Tag to Condition
   - 4.12.4) From Branch True:
   - 4.12.4.1) Add Apply Gameplay Effect to Target node
   - 4.12.4.2) Target: Connect enemy ASC
   - 4.12.4.3) Gameplay Effect Class: GE_DashDamage

#### 4.13) Apply Knockback to Enemies
   - 4.13.1) From Apply GE to Target execution pin:
   - 4.13.1.1) Add Get Actor Location node (player)
   - 4.13.1.2) Connect PlayerRef to Target
   - 4.13.2) Add Get Actor Location node (enemy)
   - 4.13.2.1) Connect Array Element to Target
   - 4.13.3) Add Subtract (Vector - Vector) node
   - 4.13.3.1) Connect enemy location to A
   - 4.13.3.2) Connect player location to B
   - 4.13.4) Add Normalize node
   - 4.13.4.1) Connect Subtract result to input
   - 4.13.5) Add Multiply (Vector * Float) node
   - 4.13.5.1) Connect Normalize result to first input
   - 4.13.5.2) Connect KnockbackForce to second input
   - 4.13.6) Add Cast To Character node
   - 4.13.6.1) Connect Array Element to Object
   - 4.13.7) From Cast success:
   - 4.13.7.1) Add Launch Character node
   - 4.13.7.2) Connect As Character to Target
   - 4.13.7.3) Connect knockback vector to Launch Velocity

#### 4.14) Wait for Dash Duration
   - 4.14.1) From For Each Loop Completed pin:
   - 4.14.1.1) Add Delay node
   - 4.14.1.2) Connect DashDuration variable to Duration

#### 4.15) Restore Original Speed
   - 4.15.1) From Delay Completed execution pin:
   - 4.15.1.1) Add Get Character Movement node
   - 4.15.1.2) Connect PlayerRef to Target
   - 4.15.2) From Character Movement:
   - 4.15.2.1) Add Set Max Walk Speed node
   - 4.15.2.2) Connect OriginalMaxWalkSpeed to Max Walk Speed

#### 4.16) Apply Cooldown
   - 4.16.1) From Set Max Walk Speed execution pin:
   - 4.16.1.1) Drag from PlayerASC variable
   - 4.16.1.2) Add Apply Gameplay Effect to Self node
   - 4.16.1.3) Connect PlayerASC to Target
   - 4.16.1.4) Gameplay Effect Class: GE_DashCooldown

#### 4.17) Clear References and End Ability
   - 4.17.1) From Apply Cooldown execution pin:
   - 4.17.1.1) Add Set PlayerRef node
   - 4.17.1.2) Value: None (leave disconnected)
   - 4.17.2) From Set PlayerRef execution:
   - 4.17.2.1) Add Set PlayerASC node
   - 4.17.2.2) Value: None (leave disconnected)
   - 4.17.3) From Set PlayerASC execution:
   - 4.17.3.1) Add End Ability node
   - 4.17.3.2) Was Cancelled: Unchecked

### **5) Implement Event OnEndAbility**

#### 5.1) Add Event OnEndAbility
   - 5.1.1) Right-click in Event Graph
   - 5.1.2) Search for: `Event OnEndAbility`
   - 5.1.3) Select Event OnEndAbility

#### 5.2) Validate PlayerASC for Cleanup
   - 5.2.1) From Event OnEndAbility execution:
   - 5.2.1.1) Add Is Valid node
   - 5.2.1.2) Drag from PlayerASC variable
   - 5.2.1.3) Connect PlayerASC to Input Object
   - 5.2.2) From Is Valid Return Value:
   - 5.2.2.1) Add Branch node
   - 5.2.2.2) Connect Is Valid to Condition

#### 5.3) Clear References on Early Cancel
   - 5.3.1) From Branch True execution:
   - 5.3.1.1) Add Set PlayerRef node
   - 5.3.1.2) Value: None (leave disconnected)
   - 5.3.2) From Set PlayerRef execution:
   - 5.3.2.1) Add Set PlayerASC node
   - 5.3.2.2) Value: None (leave disconnected)

### **6) Compile and Save**
   - 6.1) Click Compile
   - 6.2) Click Save

---

## **PHASE 5: INTEGRATION**

### **1) Add Component to Player Character**

#### 1.1) Open Player Character Blueprint
   - 1.1.1) Navigate to player character blueprint
   - 1.1.2) Double-click to open

#### 1.2) Add Double-Press Component
   - 1.2.1) In Components panel, click Add
   - 1.2.2) Search for: `BP_DoublePressInputComponent`
   - 1.2.3) Select BP_DoublePressInputComponent
   - 1.2.4) Rename to: `DoublePressInput`

#### 1.3) Create PendingDashDirection Variable

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| PendingDashDirection | Vector | (0, 0, 0) | No |

#### 1.4) Compile and Save
   - 1.4.1) Click Compile
   - 1.4.2) Click Save

### **2) Configure EQI_FatherExoskeleton**

#### 2.1) Open EquippableItem Blueprint
   - 2.1.1) Navigate to: `/Content/FatherCompanion/Items/`
   - 2.1.2) Open EQI_FatherExoskeleton

#### 2.2) Add Ability to Abilities Array
   - 2.2.1) Click Class Defaults
   - 2.2.2) Find Abilities array
   - 2.2.3) Verify GA_FatherExoskeletonDash is in array
   - 2.2.4) If not present, click + and add GA_FatherExoskeletonDash

#### 2.3) Compile and Save
   - 2.3.1) Click Compile
   - 2.3.2) Click Save

---

## **PHASE 6: INPUT CONFIGURATION**

### **1) Setup Input Detection in Player Character**

#### 1.1) Open Player Character Event Graph
   - 1.1.1) Navigate to player character blueprint
   - 1.1.2) Click Event Graph tab

### **2) Add Forward Input Detection (W)**

#### 2.1) Create Input Detection
   - 2.1.1) Add Input Action node for forward movement (IA_MoveForward or equivalent)
   - 2.1.2) From Triggered execution pin:
   - 2.1.2.1) Add Get DoublePressInput component reference
   - 2.1.2.2) Add DetectDoublePress function call
   - 2.1.2.3) KeyName: `Forward`
   - 2.1.3) From IsDoublePress Return Value:
   - 2.1.3.1) Add Branch node
   - 2.1.4) From Branch True:
   - 2.1.4.1) Add TriggerDash function call
   - 2.1.4.2) DashDirection: (1, 0, 0)

### **3) Add Backward Input Detection (S)**

#### 3.1) Create Input Detection
   - 3.1.1) Add Input Action node for backward movement
   - 3.1.2) Repeat pattern from 2.1
   - 3.1.3) KeyName: `Backward`
   - 3.1.4) DashDirection: (-1, 0, 0)

### **4) Add Left Input Detection (A)**

#### 4.1) Create Input Detection
   - 4.1.1) Add Input Action node for left movement
   - 4.1.2) Repeat pattern from 2.1
   - 4.1.3) KeyName: `Left`
   - 4.1.4) DashDirection: (0, -1, 0)

### **5) Add Right Input Detection (D)**

#### 5.1) Create Input Detection
   - 5.1.1) Add Input Action node for right movement
   - 5.1.2) Repeat pattern from 2.1
   - 5.1.3) KeyName: `Right`
   - 5.1.4) DashDirection: (0, 1, 0)

### **6) Compile and Save**
   - 6.1) Click Compile
   - 6.2) Click Save

---

## **QUICK REFERENCE**

### **Tag Configuration Summary**

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Exoskeleton.Dash |
| Block Abilities with Tag | Father.State.Dashing, Cooldown.Father.Exoskeleton.Dash |
| Activation Required Tags | Effect.Father.FormState.Exoskeleton, Father.State.Attached, Father.State.Recruited |
| Activation Owned Tags | Father.State.Dashing |

### **Multiplayer Configuration**

| Setting | Value |
|---------|-------|
| Instancing Policy | Instanced Per Actor |
| Replication Policy | Replicate Yes |
| Net Execution Policy | Local Predicted |
| bActivateAbilityOnGranted | False |

### **Gameplay Effects Summary**

| Effect | Duration | Purpose |
|--------|----------|---------|
| GE_DashInvulnerability | 0.5s | Grants State.Invulnerable |
| GE_DashDamage | Instant | 15 base damage via NarrativeDamageExecCalc |
| GE_DashCooldown | 3.0s | Grants Cooldown.Father.Exoskeleton.Dash |

### **Variable Summary**

| Variable | Type | Instance Editable | Default |
|----------|------|-------------------|---------|
| DashSpeed | Float | Yes | 3000.0 |
| OriginalMaxWalkSpeed | Float | No | 600.0 |
| DashDuration | Float | Yes | 0.5 |
| DashForce | Float | Yes | 2000.0 |
| DamageRadius | Float | Yes | 200.0 |
| KnockbackForce | Float | Yes | 1000.0 |
| PlayerRef | NarrativeCharacter | No | None |
| PlayerASC | NarrativeAbilitySystemComponent | No | None |

### **Input Direction Vectors**

| Key | Direction | Vector |
|-----|-----------|--------|
| W | Forward | (1, 0, 0) |
| S | Backward | (-1, 0, 0) |
| A | Left | (0, -1, 0) |
| D | Right | (0, 1, 0) |

### **EquippableItem Integration**

| Asset | Contains GA_FatherExoskeletonDash |
|-------|-----------------------------------|
| EQI_FatherExoskeleton | Yes |
| EQI_FatherArmor | No |
| EQI_FatherSymbiote | No |
| EQI_FatherEngineer | No |

### **Narrative Pro Integration**

| System | Method |
|--------|--------|
| Damage Immunity | State.Invulnerable tag via GE |
| Damage Dealing | NarrativeDamageExecCalc |
| Movement Speed | CharacterMovement component direct |
| Form Restriction | Activation Required Tags |
| Ability Granting | EquippableItem Abilities array |

### **EndAbility Cleanup Flow**

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event OnEndAbility | Triggered on any ability end |
| 2 | Is Valid (PlayerASC) | Validate reference exists |
| 3 | Set PlayerRef (None) | Clear player reference |
| 4 | Set PlayerASC (None) | Clear ASC reference |

### **Related Documents**

| Document | Version | Purpose |
|----------|---------|---------|
| Father_Companion_Technical_Reference | v4.5 | Cross-actor patterns, validation architecture |
| Father_Companion_System_Setup_Guide | v1.3 | BP_FatherCompanion setup, variable definitions |
| GA_FatherExoskeleton_Implementation_Guide | v3.3 | Parent form ability |
| DefaultGameplayTags_FatherCompanion | v3.5 | Tag definitions and line numbers |

---

## CHANGELOG

### Version 3.6 - January 2026

| Change | Description |
|--------|-------------|
| Narrative Pro | Updated version reference from v2.1 to v2.2 |

### Version 3.5 - January 2026

| Change | Description |
|--------|-------------|
| Component Variables | PHASE 2 Section 2 - converted to Variable/Type/Default table |
| Player Variable | PHASE 5 Section 1.3 - converted to table format |

### Version 3.4 - January 2026

| Change | Description |
|--------|-------------|
| PHASE 2 Simplified | GE configs (Invulnerability, Damage, Cooldown) reduced from 103 to 55 lines using table format |
| Variable Creation Simplified | Section 3 reduced from 54 to 11 lines using table format |
| Total Line Reduction | ~90 lines saved |

### Version 3.3 - January 2026

| Change | Description |
|--------|-------------|
| PHASE 1 Simplified | Replaced 6 detailed subsections with consolidated tag tables |
| Tag Creation Format | Tags now listed in Create Required Tags and Verify Existing Tags tables |
| Line Reduction | Reduced PHASE 1 from 47 lines to 19 lines |

### Version 3.2 - December 2025

| Change | Description |
|--------|-------------|
| Activation Required Tags | Added Father.State.Recruited (Element [2]) |
| PlayerASC Variable | Added NarrativeAbilitySystemComponent reference for validation |
| PlayerRef Validation | Added Branch early exit on invalid PlayerRef |
| PlayerASC Validation | Added Cast + Is Valid + Branch for ASC validation |
| Effect Application | Updated to use validated PlayerASC variable |
| Reference Clearing | Added Set PlayerRef/PlayerASC to None before EndAbility |
| Event OnEndAbility | Added cleanup handler for early cancellation |
| Quick Reference | Added EndAbility Cleanup Flow table |
| Related Documents | Updated to v4.5/v1.3/v3.5 versions |

### Version 3.1 - December 2025

| Change | Description |
|--------|-------------|
| Ability Tag | Changed Ability.Father.ExoskeletonDash to Ability.Father.Exoskeleton.Dash |
| Cooldown Tag | Changed Cooldown.Father.ExoskeletonDash to Cooldown.Father.Exoskeleton.Dash |
| Tag Hierarchy | Enables automatic cancellation when Exoskeleton form ends |

### Version 3.0 - November 2025

| Change | Description |
|--------|-------------|
| Initial Release | Complete implementation guide with faction system |

---

**END OF GUIDE**
