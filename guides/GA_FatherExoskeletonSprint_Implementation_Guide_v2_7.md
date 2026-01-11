# GA_FatherExoskeletonSprint Implementation Guide
## VERSION 2.7 - Simplified Configuration Sections
## Unreal Engine 5.6 + Narrative Pro Plugin v2.2

**Version:** 2.6
**Date:** January 2026
**Form:** Exoskeleton
**Parent Class:** NarrativeGameplayAbility

---

## DOCUMENT PURPOSE

This guide provides step-by-step instructions for implementing GA_FatherExoskeletonSprint, a continuous sprint ability for the Exoskeleton form. While sprinting, the player moves significantly faster with enhanced jump height and pushes away nearby hostile actors without dealing damage.

**Key Features:**
- Exoskeleton form only (requires Father.Form.Exoskeleton tag)
- Continuous sprint while key held (ends on release)
- Movement speed boost: 75% increase
- Jump height boost: 50% increase (stacks with Exoskeleton base boost)
- Enemy push mechanic: Radial knockback every 0.3 seconds
- Faction-based hostile detection via ArsenalStatics::GetAttitude
- Gameplay Cue integration for VFX/SFX
- Direct CharacterMovement manipulation
- Full multiplayer compatibility via local prediction and authority checks

---

## TABLE OF CONTENTS

1. [Prerequisites](#prerequisites)
2. [Phase 1: Gameplay Tags Setup](#phase-1-gameplay-tags-setup)
3. [Phase 2: Gameplay Effects Setup](#phase-2-gameplay-effects-setup)
4. [Phase 3: Gameplay Ability Blueprint Creation](#phase-3-gameplay-ability-blueprint-creation)
5. [Phase 4: Implement Ability Logic](#phase-4-implement-ability-logic)
6. [Phase 5: Gameplay Cue Integration](#phase-5-gameplay-cue-integration)
7. [Phase 6: Father Blueprint Integration](#phase-6-father-blueprint-integration)
8. [Phase 7: Input Configuration](#phase-7-input-configuration)
9. [Phase 8: Final Compilation](#phase-8-final-compilation)
10. [Quick Reference](#quick-reference)
11. [Pending Decisions](#pending-decisions)

---

## PREREQUISITES

Before implementing GA_FatherExoskeletonSprint, ensure the following are complete:

1. GA_FatherExoskeleton implemented and functional
2. BP_FatherCompanion created with NarrativeNPCCharacter base class
3. Player character has NarrativeAbilitySystemComponent
4. Player character has CharacterMovement component
5. Enhanced Input System enabled in project settings
6. DA_FatherInputMapping asset created (NarrativeAbilityInputMapping)
7. NPCDefinition faction system configured (Narrative.Factions.Heroes for player/father)

---

# PHASE 1: GAMEPLAY TAGS SETUP

## 1) Create Required Tags

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Exoskeleton.Sprint | Continuous sprint ability for Exoskeleton form |
| Father.State.Sprinting | Player is actively sprinting with Exoskeleton form |
| Effect.Father.SprintSpeed | Speed boost effect from sprint ability |
| Effect.Father.SprintJump | Jump height boost effect from sprint ability |
| Effect.Father.SprintPush | Enemy push effect marker |
| GameplayCue.Father.Sprint | VFX and SFX for Exoskeleton sprint ability |

## 2) Verify Existing Tags

| Tag Name | Purpose |
|----------|---------|
| Father.Form.Exoskeleton | Required form |
| Father.State.Attached | Required state |
| Father.State.Recruited | Recruitment requirement |

---

# PHASE 2: GAMEPLAY EFFECTS SETUP

## 2) Create GE_SprintPush - Enemy Push Effect Marker

### 2.1) Create the Gameplay Effect Blueprint
2.1.1) Navigate to: /Content/FatherCompanion/Abilities/ExoskeletonSprint/
2.1.2) Create folder structure if needed
2.1.3) Right-click in Content Browser
2.1.4) Select: Blueprint Class
2.1.5) Search for: GameplayEffect
2.1.6) Select: GameplayEffect as parent class
2.1.7) Name: GE_SprintPush
2.1.8) Double-click to open

### 2.2) Configure Effect Duration
2.2.1) Click: Class Defaults button in toolbar
2.2.2) Locate: Duration Policy property
2.2.3) Duration Policy: Select Instant from dropdown

### 2.3) Configure Gameplay Effect Components

#### 2.3.1) Add Grant Tags Component
2.3.1.1) Locate: Components section in Details panel
2.3.1.2) Click: + button next to Components array
2.3.1.3) Search for: Grant Tags to Target Actor
2.3.1.4) Select: Target Gameplay Effect Component_GrantTagsToTargetActor

#### 2.3.2) Configure Granted Tag
2.3.2.1) Expand: Component [0] in Components array
2.3.2.2) Locate: Add to Inherited Tags Container property
2.3.2.3) Click: + button to add tag
2.3.2.4) Tag [0]: Select Effect.Father.SprintPush

### 2.4) Configure Effect Asset Tags

#### 2.4.1) Add Asset Tags Component
2.4.1.1) Click: + button next to Components array
2.4.1.2) Search for: Tags This Effect Has
2.4.1.3) Select: Target Gameplay Effect Component_AssetTags

#### 2.4.2) Set Asset Tag
2.4.2.1) Expand: Component [1] in Components array
2.4.2.2) Locate: Inherited Tags property
2.4.2.3) Click: + button to add tag
2.4.2.4) Tag [0]: Select Effect.Father.SprintPush

### 2.5) Compile and Save
2.5.1) Click: Compile button
2.5.2) Verify: No errors
2.5.3) Click: Save button

---

# PHASE 3: GAMEPLAY ABILITY BLUEPRINT CREATION

## 3) Create GA_FatherExoskeletonSprint Ability

### 3.1) Create the Ability Blueprint

#### 3.1.1) Navigate to Abilities Folder
3.1.1.1) In Content Browser: /Content/FatherCompanion/Abilities/ExoskeletonSprint/

#### 3.1.2) Create New Gameplay Ability
3.1.2.1) Right-click in Content Browser
3.1.2.2) Select: Blueprint Class
3.1.2.3) Search for: NarrativeGameplayAbility
3.1.2.4) Select: NarrativeGameplayAbility as parent class
3.1.2.5) Name: GA_FatherExoskeletonSprint

#### 3.1.3) Open the Blueprint
3.1.3.1) Double-click: GA_FatherExoskeletonSprint

### 3.2) Configure Ability Class Defaults

#### 3.2.1) Open Class Defaults
3.2.1.1) Click: Class Defaults button in toolbar

#### 3.2.2) Configure Narrative Ability Section

##### 3.2.2.1) Set InputTag Property
3.2.2.1.1) Locate: Narrative Ability section in Details panel
3.2.2.1.2) Find property: InputTag
3.2.2.1.3) Click dropdown arrow
3.2.2.1.4) Navigate to: Narrative -> Input -> Father
3.2.2.1.5) Select: Narrative.Input.Father.Ability2

#### 3.2.3) Configure Ability Tags (Class Defaults)

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Exoskeleton.Sprint |
| Activation Required Tags | Father.Form.Exoskeleton, Father.State.Attached, Father.State.Recruited |
| Activation Owned Tags | Father.State.Sprinting |

#### 3.2.4) Configure Replication Settings

##### 3.2.4.1) Set Net Execution Policy
3.2.4.1.1) Locate: Advanced section in Details panel
3.2.4.1.2) Find property: Net Execution Policy
3.2.4.1.3) Select: Local Predicted from dropdown

##### 3.2.4.2) Set Replication Policy
3.2.4.2.1) Locate: Replication Policy property
3.2.4.2.2) Select: Replicate Yes from dropdown

##### 3.2.4.3) Set Replicate Input Directly
3.2.4.3.1) Locate: Replicate Input Directly checkbox
3.2.4.3.2) Ensure: Unchecked (FALSE)

#### 3.2.5) Configure Instancing Policy
3.2.5.1) Locate: Instancing Policy property
3.2.5.2) Select: Instanced Per Actor from dropdown

### 3.3) Create Ability Variables

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| OriginalMaxWalkSpeed | Float | 0.0 | No |
| OriginalJumpZVelocity | Float | 0.0 | No |
| SpeedMultiplier | Float | 1.75 | Yes |
| JumpMultiplier | Float | 1.5 | Yes |
| PushTimerHandle | Timer Handle | - | No |
| PushInterval | Float | 0.3 | Yes |
| PushRadius | Float | 300.0 | Yes |
| PushForce | Float | 800.0 | Yes |
| SprintCueTag | Gameplay Tag | GameplayCue.Father.Sprint | Yes |
| CachedPlayerRef | Actor (Object Reference) | None | No |
| PlayerASC | NarrativeAbilitySystemComponent (Object Reference) | None | No |

---

# PHASE 4: IMPLEMENT ABILITY LOGIC

## 4) Create Event ActivateAbility Logic

### 4.1) Open Event Graph
4.1.1) In GA_FatherExoskeletonSprint blueprint
4.1.2) Click: Event Graph tab

### 4.2) Add Event ActivateAbility
4.2.1) Right-click in Event Graph
4.2.2) Search: Event ActivateAbility
4.2.3) Select: Event ActivateAbility

### 4.3) Get Owning Player

#### 4.3.1) Get Actor Info
4.3.1.1) From Event ActivateAbility execution:
4.3.1.2) Drag out and search: Get Avatar Actor from Actor Info
4.3.1.3) Add: Get Avatar Actor from Actor Info node

#### 4.3.2) Cast to Player Character
4.3.2.1) From Avatar Actor return:
4.3.2.2) Drag out and search: Cast To
4.3.2.3) Add: Cast To NarrativePlayerCharacter node
4.3.2.4) Connect execution from Event ActivateAbility

#### 4.3.3) Cache Player Reference
4.3.3.1) From Cast success execution:
4.3.3.2) Add: SET CachedPlayerRef node
4.3.3.3) Connect: As Narrative Player Character -> CachedPlayerRef input

### 4.4) Validate CachedPlayerRef

#### 4.4.1) Add IsValid Check
4.4.1.1) From SET CachedPlayerRef execution:
4.4.1.2) Add: Is Valid node
4.4.1.3) Drag from CachedPlayerRef variable
4.4.1.4) Connect CachedPlayerRef to Is Valid input

#### 4.4.2) Add Branch
4.4.2.1) From Is Valid Return Value:
4.4.2.2) Add: Branch node
4.4.2.3) Connect Is Valid to Condition

#### 4.4.3) Handle Invalid Player
4.4.3.1) From Branch False execution:
4.4.3.2) Add: End Ability node
4.4.3.3) Was Cancelled: Checked

### 4.5) Get and Store PlayerASC

#### 4.5.1) Get Ability System Component
4.5.1.1) From Branch True execution:
4.5.1.2) Drag from CachedPlayerRef variable
4.5.1.3) Add: Get Ability System Component node

#### 4.5.2) Cast to NarrativeAbilitySystemComponent
4.5.2.1) From ASC Return Value:
4.5.2.2) Add: Cast To NarrativeAbilitySystemComponent node
4.5.2.3) Connect execution

#### 4.5.3) Store PlayerASC
4.5.3.1) From Cast Success execution:
4.5.3.2) Add: SET PlayerASC node
4.5.3.3) Connect As NarrativeAbilitySystemComponent to value

#### 4.5.4) Handle Cast Failure
4.5.4.1) From Cast Failure execution:
4.5.4.2) Add: End Ability node
4.5.4.3) Was Cancelled: Checked

### 4.6) Validate PlayerASC

#### 4.6.1) Add IsValid Check
4.6.1.1) From SET PlayerASC execution:
4.6.1.2) Add: Is Valid node
4.6.1.3) Drag from PlayerASC variable
4.6.1.4) Connect PlayerASC to Is Valid input

#### 4.6.2) Add Branch
4.6.2.1) From Is Valid Return Value:
4.6.2.2) Add: Branch node
4.6.2.3) Connect Is Valid to Condition

#### 4.6.3) Handle Invalid ASC
4.6.3.1) From Branch False execution:
4.6.3.2) Add: End Ability node
4.6.3.3) Was Cancelled: Checked

### 4.7) Store Original Movement Values

#### 4.7.1) Get Character Movement Component
4.7.1.1) From Branch True execution:
4.7.1.2) Drag from CachedPlayerRef variable
4.7.1.3) Add: Get Character Movement node

#### 4.7.2) Get Original Walk Speed
4.7.2.1) From Character Movement:
4.7.2.2) Drag out and search: Get Max Walk Speed
4.7.2.3) Add: Get Max Walk Speed node

#### 4.7.3) Store Original Walk Speed
4.7.3.1) Add: SET OriginalMaxWalkSpeed node
4.7.3.2) Connect: Max Walk Speed value -> OriginalMaxWalkSpeed input
4.7.3.3) Connect execution

#### 4.7.4) Get Original Jump Velocity
4.7.4.1) From Character Movement:
4.7.4.2) Drag out and search: Get Jump Z Velocity
4.7.4.3) Add: Get Jump Z Velocity node

#### 4.7.5) Store Original Jump Velocity
4.7.5.1) From SET OriginalMaxWalkSpeed execution:
4.7.5.2) Add: SET OriginalJumpZVelocity node
4.7.5.3) Connect: Jump Z Velocity value -> OriginalJumpZVelocity input

### 4.8) Apply Sprint Speed Boost

#### 4.8.1) Calculate New Speed
4.8.1.1) Add: Float * Float node
4.8.1.2) Connect: OriginalMaxWalkSpeed (GET) -> first input
4.8.1.3) Connect: SpeedMultiplier (GET) -> second input

#### 4.8.2) Set New Walk Speed
4.8.2.1) From SET OriginalJumpZVelocity execution:
4.8.2.2) From Character Movement reference:
4.8.2.3) Drag out and search: Set Max Walk Speed
4.8.2.4) Add: Set Max Walk Speed node
4.8.2.5) Connect: Multiply result -> Max Walk Speed input

### 4.9) Apply Jump Height Boost

#### 4.9.1) Calculate New Jump Velocity
4.9.1.1) Add: Float * Float node
4.9.1.2) Connect: OriginalJumpZVelocity (GET) -> first input
4.9.1.3) Connect: JumpMultiplier (GET) -> second input

#### 4.9.2) Set New Jump Velocity
4.9.2.1) From Set Max Walk Speed execution:
4.9.2.2) From Character Movement reference:
4.9.2.3) Drag out and search: Set Jump Z Velocity
4.9.2.4) Add: Set Jump Z Velocity node
4.9.2.5) Connect: Multiply result -> Jump Z Velocity input

### 4.10) Start Push Timer

#### 4.10.1) Create Push Timer
4.10.1.1) From Set Jump Z Velocity execution:
4.10.1.2) Add: Set Timer by Function Name node
4.10.1.3) Function Name: ExecutePushCheck (string literal)
4.10.1.4) Time: Connect PushInterval variable (GET)
4.10.1.5) Looping: Check TRUE

#### 4.10.2) Store Timer Handle
4.10.2.1) From Set Timer by Function Name -> Return Value:
4.10.2.2) Add: SET PushTimerHandle node
4.10.2.3) Connect execution

### 4.11) Setup Input Release Binding

#### 4.11.1) Add Wait Input Release Task
4.11.1.1) From SET PushTimerHandle execution:
4.11.1.2) Right-click and search: Wait Input Release
4.11.1.3) Add: Ability Task Wait Input Release node

#### 4.11.2) Connect On Release Event
4.11.2.1) From On Release execution pin:
4.11.2.2) This will connect to EndSprint logic (created in next section)

## 5) Create EndSprint Custom Event

### 5.1) Create Custom Event
5.1.1) Right-click in Event Graph
5.1.2) Search: Custom Event
5.1.3) Add: Custom Event node
5.1.4) Name: EndSprint

### 5.2) Connect Input Release to EndSprint
5.2.1) From Wait Input Release -> On Release:
5.2.2) Connect execution to: EndSprint event

### 5.3) Clear Push Timer

#### 5.3.1) Clear Timer by Handle
5.3.1.1) From EndSprint execution:
5.3.1.2) Add: Clear Timer by Handle node
5.3.1.3) Handle input: Connect PushTimerHandle variable (GET)

### 5.4) Restore Original Movement Values

#### 5.4.1) Validate CachedPlayerRef
5.4.1.1) From Clear Timer execution:
5.4.1.2) Add: Is Valid node
5.4.1.3) Drag from CachedPlayerRef variable
5.4.1.4) Connect CachedPlayerRef to Is Valid input

#### 5.4.2) Add Branch
5.4.2.1) From Is Valid Return Value:
5.4.2.2) Add: Branch node
5.4.2.3) Connect Is Valid to Condition

#### 5.4.3) Get Character Movement
5.4.3.1) From Branch True execution:
5.4.3.2) Drag from CachedPlayerRef variable
5.4.3.3) Add: Get Character Movement node

#### 5.4.4) Restore Walk Speed
5.4.4.1) From Branch True execution:
5.4.4.2) From Character Movement:
5.4.4.3) Add: Set Max Walk Speed node
5.4.4.4) Connect: OriginalMaxWalkSpeed (GET) -> Max Walk Speed input

#### 5.4.5) Restore Jump Velocity
5.4.5.1) From Set Max Walk Speed execution:
5.4.5.2) From Character Movement:
5.4.5.3) Add: Set Jump Z Velocity node
5.4.5.4) Connect: OriginalJumpZVelocity (GET) -> Jump Z Velocity input

### 5.5) Clear References and End Ability

#### 5.5.1) Clear CachedPlayerRef
5.5.1.1) From Set Jump Z Velocity execution (or Branch False):
5.5.1.2) Add: SET CachedPlayerRef node
5.5.1.3) Value: None (leave disconnected)

#### 5.5.2) Clear PlayerASC
5.5.2.1) From SET CachedPlayerRef execution:
5.5.2.2) Add: SET PlayerASC node
5.5.2.3) Value: None (leave disconnected)

#### 5.5.3) End Ability Node
5.5.3.1) From SET PlayerASC execution:
5.5.3.2) Add: End Ability node
5.5.3.3) Was Cancelled: FALSE

## 6) Create ExecutePushCheck Function

### 6.1) Create Custom Event for Timer
6.1.1) Right-click in Event Graph
6.1.2) Search: Custom Event
6.1.3) Add: Custom Event node
6.1.4) Name: ExecutePushCheck

### 6.2) Check Authority

#### 6.2.1) Add Authority Check
6.2.1.1) From ExecutePushCheck execution:
6.2.1.2) Add: Has Authority node
6.2.1.3) From Return Value:
6.2.1.4) Add: Branch node
6.2.1.5) Connect: Has Authority result -> Branch Condition

#### 6.2.2) Authority Branch
6.2.2.1) TRUE path: Continue to push logic
6.2.2.2) FALSE path: No connection (clients skip push logic)

### 6.3) Get Player Reference

#### 6.3.1) Get Cached Player
6.3.1.1) From Branch TRUE execution:
6.3.1.2) Add: GET CachedPlayerRef node

#### 6.3.2) Validate Player Reference
6.3.2.1) From CachedPlayerRef:
6.3.2.2) Add: Is Valid node
6.3.2.3) Add: Branch node
6.3.2.4) Connect: Is Valid result -> Branch Condition

### 6.4) Get Player Location

#### 6.4.1) Get Actor Location
6.4.1.1) From Branch TRUE execution:
6.4.1.2) From CachedPlayerRef:
6.4.1.3) Drag out and search: Get Actor Location
6.4.1.4) Add: Get Actor Location node

### 6.5) Find Nearby Actors

#### 6.5.1) Sphere Overlap Actors
6.5.1.1) From Get Actor Location execution:
6.5.1.2) Add: Sphere Overlap Actors node
6.5.1.3) Sphere Pos: Connect Player Location
6.5.1.4) Sphere Radius: Connect PushRadius variable (GET)
6.5.1.5) Object Types: Add Pawn
6.5.1.6) Actors to Ignore: Create Make Array with CachedPlayerRef

#### 6.5.2) Check for Valid Results
6.5.2.1) From Sphere Overlap -> Out Actors:
6.5.2.2) Add: Length node
6.5.2.3) Add: > (Greater Than) node
6.5.2.4) Compare Length with: 0
6.5.2.5) Add: Branch node
6.5.2.6) Connect: Greater Than result -> Branch Condition

### 6.6) Process Each Overlapped Actor

#### 6.6.1) ForEach Loop
6.6.1.1) From Branch True execution:
6.6.1.2) Add: For Each Loop node
6.6.1.3) Array input: Connect Out Actors from Sphere Overlap

#### 6.6.2) Check if Target is Hostile
6.6.2.1) From ForEach Array Element:
6.6.2.2) Right-click and search: Get Attitude
6.6.2.3) Add: Get Attitude node (from ArsenalStatics)
6.6.2.4) Test Actor input: Connect CachedPlayerRef (GET)
6.6.2.5) Target input: Connect Array Element (overlapped actor)

#### 6.6.3) Compare Attitude to Hostile
6.6.3.1) From Get Attitude Return Value:
6.6.3.2) Add: Equal (Enum) node
6.6.3.3) First input: Connect Get Attitude return value
6.6.3.4) Second input: Select ETeamAttitude::Hostile from dropdown

#### 6.6.4) Branch on Hostile Check
6.6.4.1) From ForEach Loop Body execution:
6.6.4.2) Add: Branch node
6.6.4.3) Connect: Equal result -> Branch Condition
6.6.4.4) TRUE path: Continue to push logic
6.6.4.5) FALSE path: No connection (skip non-hostile actors)

### 6.7) Calculate Push Direction

#### 6.7.1) Get Enemy Location
6.7.1.1) From Branch True execution:
6.7.1.2) From ForEach Array Element:
6.7.1.3) Add: Get Actor Location node

#### 6.7.2) Calculate Direction Vector
6.7.2.1) From enemy location output:
6.7.2.2) Add: Vector - Vector node
6.7.2.3) Connect: Enemy Location -> first input (A)
6.7.2.4) Connect: Player Location (from section 6.4) -> second input (B)

#### 6.7.3) Normalize Direction
6.7.3.1) From subtract result:
6.7.3.2) Add: Normalize (Vector) node

#### 6.7.4) Scale by Push Force
6.7.4.1) From Normalize output:
6.7.4.2) Add: Vector * Float node
6.7.4.3) Connect: Normalized vector -> Vector input
6.7.4.4) Connect: PushForce variable (GET) -> Float input

### 6.8) Apply Push to Enemy

#### 6.8.1) Cast to Character
6.8.1.1) From Branch True execution (after direction calculation):
6.8.1.2) From ForEach Array Element:
6.8.1.3) Add: Cast To Character node

#### 6.8.2) Launch Character
6.8.2.1) From Cast success execution:
6.8.2.2) From As Character output:
6.8.2.3) Add: Launch Character node
6.8.2.4) Launch Velocity input: Connect scaled force vector
6.8.2.5) XY Override: Check TRUE
6.8.2.6) Z Override: Uncheck FALSE

#### 6.8.3) Get Enemy ASC
6.8.3.1) From As Character output:
6.8.3.2) Add: Get Ability System Component node

#### 6.8.4) Apply Push Effect Tag
6.8.4.1) From Launch Character execution:
6.8.4.2) Add: Apply Gameplay Effect to Target node
6.8.4.3) Target: Connect enemy ASC
6.8.4.4) Gameplay Effect Class: Select GE_SprintPush
6.8.4.5) Level: 1.0

---

# PHASE 5: GAMEPLAY CUE INTEGRATION

## 7) Add Gameplay Cue Triggering

### 7.1) Add Gameplay Cue on Sprint Start

#### 7.1.1) Locate Activation Point
7.1.1.1) In Event ActivateAbility logic
7.1.1.2) Find: After SET PushTimerHandle node
7.1.1.3) Before: Wait Input Release node

#### 7.1.2) Add Gameplay Cue Node
7.1.2.1) Right-click after SET PushTimerHandle
7.1.2.2) Search: Add Gameplay Cue to Owner
7.1.2.3) Add: Add Gameplay Cue to Owner node

#### 7.1.3) Configure Add Gameplay Cue
7.1.3.1) Gameplay Cue Tag input: Connect SprintCueTag variable (GET)
7.1.3.2) Connect execution: SET PushTimerHandle -> Add Gameplay Cue to Owner
7.1.3.3) Connect execution: Add Gameplay Cue to Owner -> Wait Input Release

### 7.2) Remove Gameplay Cue on Sprint End

#### 7.2.1) Locate End Point
7.2.1.1) In EndSprint custom event logic
7.2.1.2) Find: After Clear Timer by Handle node
7.2.1.3) Before: Is Valid (CachedPlayerRef) node

#### 7.2.2) Add Remove Gameplay Cue Node
7.2.2.1) Right-click after Clear Timer by Handle
7.2.2.2) Search: Remove Gameplay Cue from Owner
7.2.2.3) Add: Remove Gameplay Cue from Owner node

#### 7.2.3) Configure Remove Gameplay Cue
7.2.3.1) Gameplay Cue Tag input: Connect SprintCueTag variable (GET)
7.2.3.2) Connect execution: Clear Timer by Handle -> Remove Gameplay Cue from Owner
7.2.3.3) Connect execution: Remove Gameplay Cue from Owner -> Is Valid

### 7.3) Create GCN_FatherSprint Blueprint
7.3.1) Follow separate guide: GCN_FatherSprint_Implementation_Guide_v1_0.md
7.3.2) This creates the GameplayCueNotify_Looping blueprint for VFX/SFX

---

# PHASE 6: IMPLEMENT EVENT ONENDABILITY

## 8) Create Event OnEndAbility Handler

### 8.1) Add Event OnEndAbility
8.1.1) Right-click in Event Graph
8.1.2) Search: Event OnEndAbility
8.1.3) Select: Event OnEndAbility

### 8.2) Validate PlayerASC for Cleanup

#### 8.2.1) Add IsValid Check
8.2.1.1) From Event OnEndAbility execution:
8.2.1.2) Drag from PlayerASC variable
8.2.1.3) Add: Is Valid node
8.2.1.4) Connect PlayerASC to Is Valid input

#### 8.2.2) Add Branch
8.2.2.1) From Is Valid Return Value:
8.2.2.2) Add: Branch node
8.2.2.3) Connect Is Valid to Condition

### 8.3) Clear References on Early Cancel

#### 8.3.1) Clear CachedPlayerRef
8.3.1.1) From Branch True execution:
8.3.1.2) Add: SET CachedPlayerRef node
8.3.1.3) Value: None (leave disconnected)

#### 8.3.2) Clear PlayerASC
8.3.2.1) From SET CachedPlayerRef execution:
8.3.2.2) Add: SET PlayerASC node
8.3.2.3) Value: None (leave disconnected)

---

# PHASE 7: FATHER BLUEPRINT INTEGRATION

## 9) Grant Sprint Ability When Exoskeleton Activates

### 9.1) Open Father Companion Blueprint
9.1.1) Navigate to: /Content/FatherCompanion/
9.1.2) Double-click: BP_FatherCompanion to open

### 9.2) Locate GA_FatherExoskeleton

#### 9.2.1) Find Exoskeleton Ability Event
9.2.1.1) In Event Graph, search for: GA_FatherExoskeleton
9.2.1.2) Or locate: Event where Exoskeleton form is activated

### 9.3) Grant Sprint Ability to Player

#### 9.3.1) Get Player ASC
9.3.1.1) After Exoskeleton attachment logic:
9.3.1.2) From Owner Player reference:
9.3.1.3) Drag and search: Get Ability System Component
9.3.1.4) Add: Get Ability System Component node

#### 9.3.2) Give Ability
9.3.2.1) From player's ASC:
9.3.2.2) Drag out and search: Give Ability
9.3.2.3) Add: Give Ability node
9.3.2.4) Ability input: Select GA_FatherExoskeletonSprint class
9.3.2.5) Level input: 1
9.3.2.6) Connect execution from Exoskeleton activation

#### 9.3.3) Store Ability Handle
9.3.3.1) From Give Ability -> Return Value:
9.3.3.2) Right-click -> Promote to Variable
9.3.3.3) Variable Name: SprintAbilityHandle
9.3.3.4) Variable Type: FGameplayAbilitySpecHandle (auto-set)

### 9.4) Remove Sprint When Form Changes

#### 9.4.1) Find Form Change Event
9.4.1.1) Locate: Event where father detaches or changes form

#### 9.4.2) Get Player ASC
9.4.2.1) From form change event execution:
9.4.2.2) From Owner Player reference:
9.4.2.3) Drag and search: Get Ability System Component
9.4.2.4) Add: Get Ability System Component node

#### 9.4.3) Cancel Ability by Handle
9.4.3.1) From player's ASC:
9.4.3.2) Drag out and search: Cancel Ability
9.4.3.3) Add: Cancel Ability node
9.4.3.4) Handle input: Connect SprintAbilityHandle variable (GET)
9.4.3.5) Connect execution

#### 9.4.4) Set Remove Ability On End
9.4.4.1) From Cancel Ability execution:
9.4.4.2) Drag out and search: Set Remove Ability On End
9.4.4.3) Add: Set Remove Ability On End node
9.4.4.4) Handle input: Connect SprintAbilityHandle variable (GET)
9.4.4.5) Connect execution

### 9.5) Compile and Save Father Blueprint
9.5.1) Click: Compile button
9.5.2) Verify: No errors
9.5.3) Click: Save button

---

# PHASE 8: INPUT CONFIGURATION

## 10) Verify Input Mapping Configuration

### 10.1) Open Narrative Input Mapping Data Asset
10.1.1) Navigate to: /Content/FatherCompanion/Data/ (or project input folder)
10.1.2) Locate: DA_FatherInputMapping (NarrativeAbilityInputMapping asset)
10.1.3) Double-click to open

### 10.2) Verify Sprint Input Entry

#### 10.2.1) Check Input Abilities Array
10.2.1.1) In Details panel, find: Input Abilities array
10.2.1.2) Look for entry with Input Tag: Narrative.Input.Father.Ability2

#### 10.2.2) Add Entry if Missing
10.2.2.1) If entry missing, click: + button to add element
10.2.2.2) Input Action: Select IA_FatherAbility2 (or equivalent)
10.2.2.3) Input Tag: Select Narrative.Input.Father.Ability2

### 10.3) Verify Input Mapping Context

#### 10.3.1) Open Input Mapping Context
10.3.1.1) Navigate to: /Content/Input/ (or project input folder)
10.3.1.2) Open: IMC_Father (or player's Input Mapping Context)

#### 10.3.2) Verify Key Binding
10.3.2.1) Find mapping for: IA_FatherAbility2
10.3.2.2) Verify key is bound (default: E key or project-specific)

### 10.4) Save Input Configuration
10.4.1) Save: DA_FatherInputMapping
10.4.2) Save: IMC_Father (if modified)

---

# PHASE 9: FINAL COMPILATION

## 11) Compile and Save All Assets

### 11.1) Compile All Modified Blueprints
11.1.1) Open and compile: GA_FatherExoskeletonSprint
11.1.2) Open and compile: GE_SprintPush
11.1.3) Open and compile: GCN_FatherSprint
11.1.4) Open and compile: BP_FatherCompanion
11.1.5) Verify: No errors in any blueprint

### 11.2) Save All Assets
11.2.1) Press: Ctrl+Shift+S (Save All)
11.2.2) Verify: All assets saved successfully

---

## QUICK REFERENCE

### Variable Summary

| Variable | Type | Editable | Default |
|----------|------|----------|---------|
| OriginalMaxWalkSpeed | Float | FALSE | 0.0 |
| OriginalJumpZVelocity | Float | FALSE | 0.0 |
| SpeedMultiplier | Float | TRUE | 1.75 |
| JumpMultiplier | Float | TRUE | 1.5 |
| PushTimerHandle | Timer Handle | FALSE | - |
| PushInterval | Float | TRUE | 0.3 |
| PushRadius | Float | TRUE | 300.0 |
| PushForce | Float | TRUE | 800.0 |
| SprintCueTag | Gameplay Tag | TRUE | GameplayCue.Father.Sprint |
| CachedPlayerRef | Actor (Object Reference) | FALSE | - |
| PlayerASC | NarrativeAbilitySystemComponent | FALSE | - |

### Tag Summary

| Tag | Purpose |
|-----|---------|
| Ability.Father.Exoskeleton.Sprint | Ability identifier |
| Father.State.Sprinting | Active sprint state |
| Father.Form.Exoskeleton | Required form |
| Father.State.Attached | Required state |
| Father.State.Recruited | Recruitment requirement |
| Effect.Father.SprintPush | Push effect marker |
| GameplayCue.Father.Sprint | VFX/SFX cue trigger |

### Class Defaults Summary

| Property | Value |
|----------|-------|
| Parent Class | NarrativeGameplayAbility |
| InputTag | Narrative.Input.Father.Ability2 |
| Instancing Policy | Instanced Per Actor |
| Net Execution Policy | Local Predicted |
| Replication Policy | Replicate Yes |
| Replicate Input Directly | FALSE |

### Assets Created

| Asset | Type | Location |
|-------|------|----------|
| GA_FatherExoskeletonSprint | Gameplay Ability | /Content/FatherCompanion/Abilities/ExoskeletonSprint/ |
| GE_SprintPush | Gameplay Effect | /Content/FatherCompanion/Abilities/ExoskeletonSprint/ |
| GCN_FatherSprint | GameplayCueNotify_Looping | /Content/FatherCompanion/GameplayCues/ |

### EndAbility Cleanup Flow

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event OnEndAbility | Triggered on any ability end |
| 2 | Is Valid (PlayerASC) | Validate reference exists |
| 3 | Set CachedPlayerRef (None) | Clear player reference |
| 4 | Set PlayerASC (None) | Clear ASC reference |

### Ability Handle Management

| Variable | Type | Stores Handle For |
|----------|------|-------------------|
| SprintAbilityHandle | FGameplayAbilitySpecHandle | GA_FatherExoskeletonSprint on player |

### Two-Step Cleanup Pattern

| Step | Function | Purpose |
|------|----------|---------|
| 1 | Cancel Ability (Handle) | Stop execution, fire cleanup |
| 2 | Set Remove Ability On End (Handle) | Safely remove spec after cleanup |

### Related Documents

| Document | Version | Purpose |
|----------|---------|---------|
| Father_Companion_Technical_Reference | v4.5 | Cross-actor patterns, validation |
| Father_Companion_System_Setup_Guide | v1.3 | BP_FatherCompanion setup |
| GA_FatherExoskeleton_Implementation_Guide | v3.3 | Parent form ability |
| GCN_FatherSprint_Implementation_Guide | v1.0 | VFX/SFX implementation |
| DefaultGameplayTags_FatherCompanion | v3.5 | Tag definitions |

### Faction System Reference

| Actor | Faction | Attitude Check Result |
|-------|---------|----------------------|
| Player | Narrative.Factions.Heroes | - |
| Father | Narrative.Factions.Heroes | Friendly |
| Enemies | Narrative.Factions.Bandits (etc.) | Hostile |
| Civilians | Narrative.Factions.Civilians | Neutral |

---

## PENDING DECISIONS

The following items require future discussion and may result in guide updates:

### Input Key Mapping

| Item | Current State | Notes |
|------|---------------|-------|
| InputTag | Narrative.Input.Father.Ability2 | Placeholder value |
| Physical Key | TBD | Requires input mapping discussion |
| Decision Status | DEFERRED | Will be addressed in input configuration session |

### Jump Boost Stacking Behavior

| Item | Current State | Notes |
|------|---------------|-------|
| Exoskeleton Base Jump Boost | 30% (JumpBoostMultiplier = 1.3) | From GA_FatherExoskeleton |
| Sprint Jump Boost | 50% (JumpMultiplier = 1.5) | Multiplies current value |
| Combined Effect | 95% total boost | 600 base -> 780 (Exo) -> 1170 (Sprint) |
| Behavior | STACKS (multiplies current) | Not replacing base value |
| Decision Status | DEFERRED | Confirm if stacking is intended |

### Design Document Update

| Item | Current State | Notes |
|------|---------------|-------|
| Document | Father_Companion_System_Design_Document_v1_3.md | Needs Sprint Push parameters |
| Missing Content | Push mechanic parameters | PushInterval, PushRadius, PushForce |
| Decision Status | PENDING | Requires document update |

---

## CHANGELOG

### Version 2.7 (January 2026)
- Updated Narrative Pro version reference from v2.1 to v2.2

### Version 2.6 (January 2026)
- Simplified Tag Configuration: Section 3.2.3 reduced from 21 to 6 lines using table format
- Simplified Variable Creation: Section 3.3 reduced from 55 to 14 lines using table format
- Total reduction: ~60 lines

### Version 2.5 (January 2026)
- Simplified PHASE 1 tag creation - replaced detailed step-by-step instructions with simple tag list tables

### Version 2.4 (December 2025)
- Added Father.State.Recruited to Activation Required Tags (Element [2])
- Added PlayerASC variable (NarrativeAbilitySystemComponent reference)
- Added CachedPlayerRef validation with Branch early exit after SET
- Added PlayerASC validation with Cast + Is Valid + Branch
- Added reference clearing before End Ability in EndSprint
- Added PHASE 6: Event OnEndAbility for early cancellation cleanup
- Updated SprintAbilityHandle storage with two-step cleanup pattern (Cancel + SetRemoveAbilityOnEnd)
- Added EndAbility Cleanup Flow table to Quick Reference
- Added Ability Handle Management table to Quick Reference
- Added Two-Step Cleanup Pattern table to Quick Reference
- Updated Related Documents to v4.5/v1.3/v3.5 versions

### Version 2.3 (December 2025)
- Changed Ability.Father.ExoskeletonSprint to Ability.Father.Exoskeleton.Sprint
- Changed Cooldown.Father.ExoskeletonSprint to Cooldown.Father.Exoskeleton.Sprint
- Hierarchical tags enable automatic cancellation when Exoskeleton form ends

### Version 2.2 (November 2025)
- Replaced Character.Enemy tag check with ArsenalStatics::GetAttitude faction system
- Added Has Authority check before push logic for multiplayer safety
- Added CachedPlayerRef variable for efficient player reference in timer
- Updated Prerequisites to reference faction system instead of Character.Enemy
- Removed Character.Enemy from Tag Summary
- Added Faction System Reference table to Quick Reference
- Updated Section 6.2 with authority check
- Updated Section 6.6.2-6.6.4 with GetAttitude hostile detection
- Clarified Jump Boost stacking behavior in Pending Decisions

### Version 2.1 (November 2025)
- Added Phase 5: Gameplay Cue Integration
- Added GameplayCue.Father.Sprint tag to Phase 1
- Added SprintCueTag variable to Phase 3
- Added Add/Remove Gameplay Cue nodes for VFX/SFX
- Added reference to GCN_FatherSprint_Implementation_Guide_v1_0.md
- Added GCN_FatherSprint to assets list
- Added Pending Decisions section
- Updated Table of Contents
- Renumbered phases (Father Integration now Phase 6, Input now Phase 7, Compilation now Phase 8)

### Version 2.0 (November 2025)
- Replaced manual input system with Narrative Pro InputTag property
- Removed GE_SprintState (using Activation Owned Tags instead)
- Fixed UTF-8 character encoding throughout document
- Removed forbidden content (completion summary, next steps, emojis)
- Added Table of Contents
- Added Quick Reference section
- Consolidated compile/save phase
- Removed excessive explanatory notes
- Added Replicate Input Directly = FALSE setting
- Standardized formatting per Guide Format Reference

### Version 1.2 (November 2025)
- Initial implementation guide

---

**END OF IMPLEMENTATION GUIDE**
