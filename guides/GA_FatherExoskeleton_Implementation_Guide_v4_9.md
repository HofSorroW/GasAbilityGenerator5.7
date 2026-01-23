# GA_FatherExoskeleton Implementation Guide
## VERSION 4.9 - NL-GUARD-IDENTITY L1 Compliant (3-Layer Guards Added)
## For Unreal Engine 5.7 + Narrative Pro Plugin v2.2

**Version:** 4.9
**Date:** January 2026
**Engine:** Unreal Engine 5.7
**Plugin:** Narrative Pro v2.2
**Implementation:** Blueprint Only
**Parent Class:** NarrativeGameplayAbility
**Architecture:** Option B (GE-Based Form Identity) - See Form_State_Architecture_Fix_v4.13.2.md

---

## **DOCUMENT INFORMATION**

| Property | Value |
|----------|-------|
| Ability Name | GA_FatherExoskeleton |
| Ability Type | Form Activation |
| Parent Class | NarrativeGameplayAbility |
| Form | Exoskeleton (Back Attachment) |
| Input | Narrative.Input.Father.FormChange (T key via wheel) |
| Multiplayer | Compatible (Server Only execution, replicated state) |
| Dependencies | BP_FatherCompanion, E_FatherForm enum, GE_ExoskeletonState, GE_FormChangeCooldown |

---

## **AUTOMATION VS MANUAL**

| Feature | Automation Status | Notes |
|---------|-------------------|-------|
| GE_ExoskeletonState definition | ✅ Auto-generated | manifest.yaml gameplay_effects section |
| GA_FatherExoskeleton blueprint | ✅ Auto-generated | manifest.yaml gameplay_abilities section |
| Activation tags config | ✅ Auto-generated | Required/Blocked tags in manifest |
| Transition prelude nodes | ✅ Auto-generated | Remove old state GE, apply new state GE (in manifest) |
| Attachment logic | ⚠️ Manual | AttachActorToComponent nodes |
| Speed/Jump boost logic | ⚠️ Manual | CharacterMovement manipulation |
| VFX spawning | ⚠️ Manual | GameplayCues preferred (Category C roadmap) |
| EndAbility cleanup | ⚠️ Manual | Speed restore, state reset, ability cleanup |

---

## **DOCUMENT PURPOSE**

This guide provides step-by-step instructions for implementing GA_FatherExoskeleton, the ability that transforms the father companion into Exoskeleton form. The father attaches to the player's back and provides movement speed boost and jump height enhancement through direct CharacterMovement component manipulation.

**Key Features:**
- Father attaches to player's back socket (FatherBackSocket)
- Base movement speed boost: +25% increase (SpeedBoostMultiplier: 1.25)
- Base jump height boost: +30% increase (JumpBoostMultiplier: 1.3)
- Direct CharacterMovement manipulation (NO attribute-based speed modification)
- **Form identity via GE_ExoskeletonState** (Infinite-duration GE grants Effect.Father.FormState.Exoskeleton - **NO invulnerability** per INV-1)
- Ephemeral state tag (Father.State.Attached) via Activation Owned Tags
- Mutual exclusivity with other forms via Cancel Abilities With Tag
- **Transition prelude removes prior form state before applying new** (Option B)
- 5-second transition animation with Father.State.Transitioning tag (**NO invulnerability** per INV-1)
- 15-second form change cooldown
- Animation integration for attachment sequence

**Stat Bonuses (via BP_FatherExoskeletonForm EquippableItem):**
- Attack Rating: +10

**Form-Specific Abilities (granted via EquippableItem):**
- GA_ExoskeletonDash (Q Tap)
- GA_ExoskeletonSprint (Q Hold) - provides additional +75% speed and +50% jump
- GA_StealthField (E)

---

## **ARCHITECTURE OVERVIEW**

### **Form Transition Flow (v4.0)**

| Step | Component | Action |
|------|-----------|--------|
| 1 | Player | Opens T wheel, selects Exoskeleton |
| 2 | GA_FatherExoskeleton | Activates, cancels current form ability |
| 3 | GA_FatherExoskeleton | Checks bIsFirstActivation |
| 4 | GA_FatherExoskeleton (False) | Adds Father.State.Transitioning tag |
| 5 | GA_FatherExoskeleton | Spawns NS_FatherFormTransition VFX |
| 6 | GA_FatherExoskeleton | Delays 5 seconds |
| 7 | Previous Form | EndAbility fires (cleanup) |
| 8 | GA_FatherExoskeleton | Detaches father from player |
| 9 | GA_FatherExoskeleton | Repositions father behind player |
| 10 | GA_FatherExoskeleton | Stores original movement values |
| 11 | GA_FatherExoskeleton | Applies +25% speed, +30% jump |
| 12 | GA_FatherExoskeleton | Attaches father to player back |
| 13 | GA_FatherExoskeleton | Sets CurrentForm = Exoskeleton |
| 14 | GA_FatherExoskeleton | Removes Father.State.Transitioning tag |
| 15 | GA_FatherExoskeleton | Applies GE_FormChangeCooldown (15s) |
| 16 | GA_FatherExoskeleton | Sets bIsFirstActivation = False |
| 17 | GA_FatherExoskeleton | Ends ability |

### **Separation of Concerns**

| Effect | Responsibility |
|--------|----------------|
| GE_ExoskeletonState | Form identity (Effect.Father.FormState.Exoskeleton) - **NO invulnerability** per INV-1 |
| Activation Owned Tags | Ephemeral state only (Father.State.Attached) |
| CharacterMovement | Speed boost (+25%), Jump boost (+30%) - applied directly in ability |
| Father.State.Transitioning | Block form change during 5s transition (added via AddLooseGameplayTag - **NO invulnerability** per INV-1) |
| GE_FormChangeCooldown | Apply 15s cooldown after transition |

---

## **PREREQUISITES**

Before implementing GA_FatherExoskeleton, ensure the following are complete:

1. BP_FatherCompanion created with NarrativeNPCCharacter base class
2. Ability System Component initialized automatically (handled by NarrativeNPCCharacter)
3. Player character has Narrative Ability System Component
4. Player skeletal mesh has FatherBackSocket created
5. **GE_ExoskeletonState Gameplay Effect must exist** (Infinite duration, grants Effect.Father.FormState.Exoskeleton - **NO invulnerability** per INV-1)
6. Father.State.Transitioning tag exists (added via AddLooseGameplayTag during transition - **NO invulnerability** per GAS Audit INV-1)
7. GE_FormChangeCooldown created (grants Cooldown.Father.FormChange for 15s)
8. NS_FatherFormTransition Niagara system created
9. Gameplay Tags configured in DefaultGameplayTags.ini
10. NPCDef_FatherCompanion configured with AC_FatherCompanion_Default
11. ReplicateActivationOwnedTags ENABLED in Project Settings

---

## **QUICK REFERENCE**

### **Tag Configuration Summary**

| Tag Type | Tag | Purpose |
|----------|-----|---------|
| Ability Tags | Ability.Father.Exoskeleton | Identifies this ability |
| Activation Owned | Father.State.Attached | Ephemeral attachment state tag |
| Activation Required | Father.State.Alive | Father must be alive |
| Activation Required | Father.State.Recruited | Father must be recruited |
| Activation Blocked | Father.State.Dormant | Blocked when dormant |
| Activation Blocked | Father.State.Transitioning | Blocked during 5s VFX |
| Activation Blocked | Father.State.SymbioteLocked | Blocked during Symbiote 30s |
| Activation Blocked | Cooldown.Father.FormChange | Blocked during 15s cooldown |
| Cooldown Gameplay Effect Class | GE_FormChangeCooldown | 15s cooldown (GAS built-in) |
| Cancel Abilities with Tag | Ability.Father.Crawler | Cancel Crawler form |
| Cancel Abilities with Tag | Ability.Father.Armor | Cancel Armor form |
| Cancel Abilities with Tag | Ability.Father.Engineer | Cancel Engineer form |

> **AUDIT NOTE (v4.8 - 2026-01-23):** `Ability.Father.Symbiote` removed from cancel list per C_SYMBIOTE_STRICT_CANCEL contract (LOCKED_CONTRACTS.md Contract 11). Symbiote is an ultimate ability (30s duration) that cannot be cancelled by player-initiated form changes.

**Note (v4.4):** Form identity (`Effect.Father.FormState.Exoskeleton`) is NOT in Activation Owned Tags. It persists via `GE_ExoskeletonState` (Option B architecture). Only ephemeral state tags (`Father.State.Attached`) belong in Activation Owned Tags. `Father.Form.*` tags are **orphan tags** - no persistent GE grants them.

### **Variable Summary**

| Variable | Type | Default | Instance Editable | Purpose |
|----------|------|---------|-------------------|---------|
| FatherRef | Object (BP_FatherCompanion) | None | No | Cached father reference |
| PlayerRef | Object (NarrativePlayerCharacter) | None | No | Cached player reference |
| OriginalWalkSpeed | Float | 0.0 | Yes | Stores player original walk speed |
| OriginalJumpZVelocity | Float | 0.0 | Yes | Stores player original jump velocity |
| SpeedBoostMultiplier | Float | 1.25 | Yes | Walk speed multiplier (25% increase) |
| JumpBoostMultiplier | Float | 1.3 | Yes | Jump height multiplier (30% increase) |
| BackSocketName | Name | FatherBackSocket | Yes | Socket for back attachment |

### **Handle Variables (BP_FatherCompanion)**

| Variable | Type | Purpose |
|----------|------|---------|
| DashAbilityHandle | FGameplayAbilitySpecHandle | GA_FatherExoskeletonDash two-step cleanup |
| SprintAbilityHandle | FGameplayAbilitySpecHandle | GA_FatherExoskeletonSprint two-step cleanup |
| StealthAbilityHandle | FGameplayAbilitySpecHandle | GA_StealthField two-step cleanup |

### **Ability Configuration**

| Property | Value |
|----------|-------|
| Parent Class | NarrativeGameplayAbility |
| Instancing Policy | Instanced Per Actor |
| Replication Policy | Replicate |
| Net Execution Policy | Server Only |
| InputTag | Narrative.Input.Father.FormChange |

### **Node Flow Summary - Initial Spawn (True Path)**

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event ActivateAbility | Entry point |
| 2 | Get Avatar Actor from Actor Info | Get father reference |
| 3 | Cast to BP_FatherCompanion | Type-safe reference |
| 4 | Branch (bIsFirstActivation) | Check if first activation |
| 5 | Store original movement values | Save for restoration |
| 6 | Apply speed/jump boost | Modify CharacterMovement |
| 7 | Attach to player back | Physical attachment |
| 8 | Set CurrentForm = Exoskeleton | Update state |
| 9 | Set bIsFirstActivation = False | Mark complete |
| 10 | End Ability | Clean lifecycle |

### **Node Flow Summary - Form Switch (False Path)**

| Step | Node | Purpose |
|------|------|---------|
| 1 | Branch False | Form switch detected |
| 2 | Add Loose Gameplay Tag | Add Father.State.Transitioning |
| 3 | Spawn System Attached | Spawn NS_FatherFormTransition VFX |
| 4 | Delay 5s | Wait for transition |
| 5 | Detach From Actor | Detach father |
| 6 | Set Actor Location | Position behind player |
| 7 | Store original movement | Save for restoration |
| 8 | Apply speed/jump boost | Modify CharacterMovement |
| 9 | Attach to player back | Physical attachment |
| 10 | Set CurrentForm = Exoskeleton | Update state |
| 11 | Remove Loose Gameplay Tag | Remove Father.State.Transitioning |
| 12 | Apply GE_FormChangeCooldown | Apply 15s cooldown |
| 13 | End Ability | Clean lifecycle |

### **Transition Parameters**

| Parameter | Value |
|-----------|-------|
| VFX Duration | 5 seconds |
| Father Invulnerable | **No** (GAS Audit INV-1 - vulnerable during transition) |
| Cooldown Duration | 15 seconds |
| Spawn Offset | -200 units (behind player) |

---

# PHASE 1: GAMEPLAY TAGS SETUP

## 1) Verify Existing Tags

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Exoskeleton | Father Exoskeleton form ability |
| Effect.Father.FormState.Exoskeleton | Exoskeleton form identity tag (granted by GE_ExoskeletonState) |
| Father.State.Attached | Ephemeral attachment state (Activation Owned Tag) |
| Father.State.Alive | Required for activation |
| Father.State.Recruited | Recruitment requirement |
| Father.State.Dormant | Blocks activation |
| Father.State.Transitioning | Blocks during 5s VFX |
| Father.State.SymbioteLocked | Blocks during Symbiote 30s |
| Cooldown.Father.FormChange | Shared 15s cooldown between form changes |
| Ability.Father.Crawler | Cancel Abilities with Tag |
| Ability.Father.Armor | Cancel Abilities with Tag |
| Ability.Father.Engineer | Cancel Abilities with Tag |

**Note (v4.4):** `Father.Form.*` tags are **orphan tags** - no persistent GE grants them. Form identity uses `Effect.Father.FormState.*` tags granted by infinite-duration GE_*State effects. Do NOT use Father.Form.* for activation gating.

---

# PHASE 2: GAMEPLAY ABILITY BLUEPRINT CREATION

## 2A) Verify GE_ExoskeletonState Exists

### 2A.1) GE_ExoskeletonState Purpose
This Gameplay Effect establishes Exoskeleton form identity using Option B architecture:
- **Duration Policy:** Infinite (persists until explicitly removed by next form's transition prelude)
- **Granted Tags:** Effect.Father.FormState.Exoskeleton
- **Asset Tag:** Effect.Father.FormState.Exoskeleton

**Note (INV-1):** Per GAS Audit decision INV-1, no form state GE grants invulnerability. Only GA_FatherSacrifice grants invulnerability (to the player, for 8 seconds).

### 2A.2) Verify Asset Location
   - 2A.2.1) Navigate to: /Content/FatherCompanion/Effects/FormState/
   - 2A.2.2) Verify GE_ExoskeletonState exists
   - 2A.2.3) If missing, generate from manifest using GasAbilityGenerator

### 2A.3) GE_ExoskeletonState Properties

| Property | Value |
|----------|-------|
| Duration Policy | Infinite |
| Granted Tags | Effect.Father.FormState.Exoskeleton |
| Asset Tag | Effect.Father.FormState.Exoskeleton |

---

## 2) Create GA_FatherExoskeleton Ability

### 2.1) Create the Ability Blueprint

#### 2.1.1) Navigate to Abilities Folder
2.1.1.1) In Content Browser, navigate to: /Content/FatherCompanion/Abilities/
2.1.1.2) Verify folder exists

#### 2.1.2) Create New Gameplay Ability
2.1.2.1) Right-click in Content Browser
2.1.2.2) Select: Blueprint Class
2.1.2.3) In class picker dialog, search for: NarrativeGameplayAbility
2.1.2.4) Select: NarrativeGameplayAbility as parent class
2.1.2.5) Name: GA_FatherExoskeleton
2.1.2.6) Press: Enter to confirm

#### 2.1.3) Open the Blueprint
2.1.3.1) Double-click: GA_FatherExoskeleton in Content Browser

---

# PHASE 3: ABILITY PROPERTIES CONFIGURATION

## 3) Configure Ability Class Defaults

### 3.1) Open Class Defaults
3.1.1) Click: Class Defaults button in toolbar
3.1.2) Details panel shows all ability properties

### 3.2) Configure Ability Tags Section

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Exoskeleton |
| Activation Owned Tags | Father.State.Attached |
| Cancel Abilities with Tag | Ability.Father.Crawler, Ability.Father.Armor, Ability.Father.Engineer |
| Activation Required Tags | Father.State.Alive, Father.State.Recruited |
| Activation Blocked Tags | Father.State.Dormant, Father.State.Transitioning, Father.State.SymbioteLocked, Cooldown.Father.FormChange |

**Note (v4.4):** Form identity (`Effect.Father.FormState.Exoskeleton`) is NOT in Activation Owned Tags. It persists via `GE_ExoskeletonState` (Option B architecture). Only ephemeral state tags (`Father.State.Attached`) belong in Activation Owned Tags.

### 3.3) Configure Instancing and Replication

#### 3.3.1) Set Instancing Policy
3.3.1.1) Locate: Instancing Policy property
3.3.1.2) Select: Instanced Per Actor from dropdown

#### 3.3.2) Set Replication Policy
3.3.2.1) Locate: Replication Policy property
3.3.2.2) Select: Replicate from dropdown

#### 3.3.3) Set Net Execution Policy
3.3.3.1) Locate: Net Execution Policy property
3.3.3.2) Select: Server Only from dropdown
3.3.3.3) NPC-owned form ability executes on server, clients see replicated results
3.3.3.4) Required because GA_FatherExoskeleton grants GA_FatherExoskeletonDash, GA_FatherExoskeletonSprint, and GA_StealthField to player ASC cross-actor

#### 3.3.4) Set Cooldown Gameplay Effect Class (Built-in System)
3.3.4.1) Find **Cooldown** section in Class Defaults
3.3.4.2) Locate **Cooldown Gameplay Effect Class** property
3.3.4.3) Click dropdown and select: GE_FormChangeCooldown
3.3.4.4) GAS automatically blocks activation when cooldown tag is present

#### 3.3.5) Set Replicate Input Directly
3.3.5.1) Locate: Replicate Input Directly checkbox
3.3.5.2) Ensure: Unchecked (FALSE)

### 3.4) Configure Input Tag

#### 3.4.1) Set InputTag Property
3.4.1.1) Locate: InputTag property in Details panel
3.4.1.2) Click: Dropdown to open tag picker
3.4.1.3) Select: Narrative.Input.Father.FormChange

---

# PHASE 4: CREATE ABILITY VARIABLES

## 4) Create Required Variables

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| bIsFirstActivation | Boolean | True | No |
| OriginalMaxWalkSpeed | Float | 0.0 | No |
| OriginalJumpZVelocity | Float | 0.0 | No |
| SpeedBoostMultiplier | Float | 1.5 | Yes |
| JumpBoostMultiplier | Float | 1.3 | Yes |
| BackSocketName | Name | FatherBackSocket | Yes |

---

# PHASE 5: IMPLEMENT ACTIVATE ABILITY EVENT

## 5) Setup Event ActivateAbility

### 5.1) Create Event Node

#### 5.1.1) Add Event ActivateAbility
5.1.1.1) Open Event Graph tab
5.1.1.2) Right-click in empty space
5.1.1.3) Search: Event ActivateAbility
5.1.1.4) Select: Event ActivateAbility
5.1.1.5) Red event node appears

### 5.2) Get Avatar Actor

#### 5.2.1) Add Get Avatar Actor From Actor Info
5.2.1.1) From Event ActivateAbility Handle pin
5.2.1.2) Drag to empty space
5.2.1.3) Search: Get Avatar Actor from Actor Info
5.2.1.4) Select node

### 5.3) Cast to Father

#### 5.3.1) Add Cast Node
5.3.1.1) From Event ActivateAbility execution pin
5.3.1.2) Drag to right
5.3.1.3) Search: Cast To BP_FatherCompanion
5.3.1.4) Select node

#### 5.3.2) Connect Object Pin
5.3.2.1) From Get Avatar Actor Return Value
5.3.2.2) Connect to Cast Object input pin

### 5.4) Branch on First Activation

#### 5.4.1) Add Branch Node
5.4.1.1) From Cast successful execution pin
5.4.1.2) Drag to right
5.4.1.3) Search: Branch
5.4.1.4) Select node

#### 5.4.2) Connect Condition
5.4.2.1) Drag bIsFirstActivation variable to graph (GET)
5.4.2.2) Connect to Branch Condition pin

---

# PHASE 6: IMPLEMENT INITIAL SPAWN FLOW (TRUE PATH)

## 6) True Path - First Activation

### 6.1) Get Owner Player

#### 6.1.1) Add Get Owner Player Node
6.1.1.1) From Branch True execution pin
6.1.1.2) Drag to right
6.1.1.3) From As BP Father Companion
6.1.1.4) Search: Get Owner Player
6.1.1.5) Select node

### 6.2) Validate Owner

#### 6.2.1) Add Is Valid Check
6.2.1.1) From Get Owner Player execution pin
6.2.1.2) Drag to right
6.2.1.3) Search: Is Valid
6.2.1.4) Select node

#### 6.2.2) Connect Input
6.2.2.1) From Get Owner Player Return Value
6.2.2.2) Connect to Is Valid Input Object pin

### 6.3) Get Character Movement

#### 6.3.1) Add Get Character Movement Node
6.3.1.1) From Is Valid -> Is Valid execution pin
6.3.1.2) Drag to right
6.3.1.3) Search: Get Character Movement
6.3.1.4) Select node

#### 6.3.2) Connect Target
6.3.2.1) From Get Owner Player Return Value
6.3.2.2) Connect to Get Character Movement Target pin

### 6.4) Store Original Walk Speed

#### 6.6.1) Get Current Max Walk Speed
6.6.1.1) From Get Character Movement Return Value
6.6.1.2) Drag to empty space
6.6.1.3) Search: Get Max Walk Speed
6.6.1.4) Select node

#### 6.6.2) Set Original Walk Speed Variable
6.6.2.1) From Get Character Movement execution pin
6.6.2.2) Drag to right
6.6.2.3) Search: SET OriginalMaxWalkSpeed
6.6.2.4) Select node
6.6.2.5) Connect Get Max Walk Speed Return Value to input

### 6.7) Store Original Jump Velocity

#### 6.7.1) Get Current Jump Z Velocity
6.7.1.1) From Get Character Movement Return Value
6.7.1.2) Drag to empty space
6.7.1.3) Search: Get Jump Z Velocity
6.7.1.4) Select node

#### 6.7.2) Set Original Jump Variable
6.7.2.1) From SET OriginalMaxWalkSpeed execution pin
6.7.2.2) Drag to right
6.7.2.3) Search: SET OriginalJumpZVelocity
6.7.2.4) Select node
6.7.2.5) Connect Get Jump Z Velocity Return Value to input

### 6.8) Apply Speed Boost

#### 6.8.1) Calculate New Walk Speed
6.8.1.1) Drag GET OriginalMaxWalkSpeed to graph
6.8.1.2) Drag from output, search: * (multiply float)
6.8.1.3) Add Float * Float node
6.8.1.4) Drag GET SpeedBoostMultiplier to graph
6.8.1.5) Connect SpeedBoostMultiplier to second multiply input

#### 6.8.2) Set Max Walk Speed
6.8.2.1) From SET OriginalJumpZVelocity execution pin
6.8.2.2) Drag to right
6.8.2.3) Search: SET Max Walk Speed
6.8.2.4) Select node
6.8.2.5) Target: Connect Get Character Movement Return Value
6.8.2.6) Max Walk Speed: Connect multiply output

### 6.9) Apply Jump Boost

#### 6.9.1) Calculate New Jump Velocity
6.9.1.1) Drag GET OriginalJumpZVelocity to graph
6.9.1.2) Drag from output, search: * (multiply float)
6.9.1.3) Add Float * Float node
6.9.1.4) Drag GET JumpBoostMultiplier to graph
6.9.1.5) Connect JumpBoostMultiplier to second multiply input

#### 6.9.2) Set Jump Z Velocity
6.9.2.1) From SET Max Walk Speed execution pin
6.9.2.2) Drag to right
6.9.2.3) Search: SET Jump Z Velocity
6.9.2.4) Select node
6.9.2.5) Target: Connect Get Character Movement Return Value
6.9.2.6) Jump Z Velocity: Connect multiply output

### 6.10) Attach Father to Player Back

#### 6.10.1) Get Player Mesh
6.10.1.1) From Get Owner Player Return Value
6.10.1.2) Drag to empty space
6.10.1.3) Search: Get Mesh
6.10.1.4) Select node

#### 6.10.2) Add Attach Actor To Component
6.10.2.1) From SET Jump Z Velocity execution pin
6.10.2.2) Drag to right
6.10.2.3) Search: Attach Actor To Component
6.10.2.4) Select node

#### 6.10.3) Configure Attachment
6.10.3.1) Target: Connect As BP Father Companion
6.10.3.2) Parent: Connect Get Mesh Return Value
6.10.3.3) Socket Name: Drag GET BackSocketName, connect to pin
6.10.3.4) Location Rule: Snap to Target
6.10.3.5) Rotation Rule: Snap to Target
6.10.3.6) Scale Rule: Keep World

### 6.11) Play Attachment Animation

#### 6.11.1) Add Play Montage Node
6.11.1.1) From Attach Actor To Component execution pin
6.11.1.2) Drag to right
6.11.1.3) Search: Play Montage
6.11.1.4) Select node

#### 6.11.2) Configure Play Montage
6.11.2.1) Target: Connect As BP Father Companion
6.11.2.2) Montage to Play: AM_FatherExoskeletonAttach
6.11.2.3) Play Rate: 1.0

### 6.12) Update Father State

#### 6.12.1) Set Current Form
6.12.1.1) From Play Montage On Completed execution pin
6.12.1.2) Drag to right
6.12.1.3) Search: SET Current Form
6.12.1.4) Select node
6.12.1.5) Target: Connect As BP Father Companion
6.12.1.6) Current Form: Exoskeleton

#### 6.12.2) Set Is Attached
6.12.2.1) From SET Current Form execution pin
6.12.2.2) Drag to right
6.12.2.3) Search: SET Is Attached
6.12.2.4) Select node
6.12.2.5) Target: Connect As BP Father Companion
6.12.2.6) Is Attached: Checked (True)

### 6.13) Mark First Activation Complete

#### 6.13.1) Set bIsFirstActivation to False
6.13.1.1) From SET Is Attached execution pin
6.13.1.2) Drag to right
6.13.1.3) Search: SET bIsFirstActivation
6.13.1.4) Select node
6.13.1.5) bIsFirstActivation: Unchecked (False)

### 6.13A) Grant Form-Specific Abilities to Player

#### 6.13A.1) Get Player Ability System Component
6.13A.1.1) From SET bIsFirstActivation execution pin
6.13A.1.2) From Get Owner Player Return Value
6.13A.1.3) Search: Get Ability System Component
6.13A.1.4) Select node
6.13A.1.5) Connect Get Owner Player Return Value to Target pin

#### 6.13A.2) Grant GA_FatherExoskeletonDash
6.13A.2.1) From SET bIsFirstActivation execution pin
6.13A.2.2) Drag to right
6.13A.2.3) Search: Give Ability
6.13A.2.4) Select node
6.13A.2.5) Connect Player ASC Return Value to Target pin
6.13A.2.6) Ability Class: GA_FatherExoskeletonDash

#### 6.13A.3) Store DashAbilityHandle
6.13A.3.1) From Give Ability execution pin
6.13A.3.2) Search: Set Dash Ability Handle
6.13A.3.3) Select node
6.13A.3.4) Connect Give Ability Return Value to Dash Ability Handle input
6.13A.3.5) Connect As BP Father Companion to Target pin

#### 6.13A.4) Grant GA_FatherExoskeletonSprint
6.13A.4.1) From Set Dash Ability Handle execution pin
6.13A.4.2) Drag to right
6.13A.4.3) Search: Give Ability
6.13A.4.4) Select node
6.13A.4.5) Connect Player ASC Return Value to Target pin
6.13A.4.6) Ability Class: GA_FatherExoskeletonSprint

#### 6.13A.5) Store SprintAbilityHandle
6.13A.5.1) From Give Ability execution pin
6.13A.5.2) Search: Set Sprint Ability Handle
6.13A.5.3) Select node
6.13A.5.4) Connect Give Ability Return Value to Sprint Ability Handle input
6.13A.5.5) Connect As BP Father Companion to Target pin

#### 6.13A.6) Grant GA_StealthField
6.13A.6.1) From Set Sprint Ability Handle execution pin
6.13A.6.2) Drag to right
6.13A.6.3) Search: Give Ability
6.13A.6.4) Select node
6.13A.6.5) Connect Player ASC Return Value to Target pin
6.13A.6.6) Ability Class: GA_StealthField

#### 6.13A.7) Store StealthAbilityHandle
6.13A.7.1) From Give Ability execution pin
6.13A.7.2) Search: Set Stealth Ability Handle
6.13A.7.3) Select node
6.13A.7.4) Connect Give Ability Return Value to Stealth Ability Handle input
6.13A.7.5) Connect As BP Father Companion to Target pin

### 6.14) End Ability

#### 6.14.1) Add End Ability Node
6.14.1.1) From Set Stealth Ability Handle execution pin
6.14.1.2) Drag to right
6.14.1.3) Search: End Ability
6.14.1.4) Select node
6.14.1.5) Was Cancelled: Unchecked (False)

---

# PHASE 7: IMPLEMENT FORM SWITCH FLOW (FALSE PATH)

## 7) False Path - Form Switch with Transition Animation

### 7.1) Get Father ASC (Form Switch)

#### 7.1.1) Create Get ASC Node
7.1.1.1) From Branch False execution pin
7.1.1.2) Drag to right
7.1.1.3) Search: Get Ability System Component
7.1.1.4) Select node
7.1.1.5) Connect As BP Father Companion to Target

### 7.2) Add Transitioning State Tag

#### 7.2.1) Add Loose Gameplay Tag
7.2.1.1) From Get ASC execution pin
7.2.1.2) Drag to right
7.2.1.3) Search: Add Loose Gameplay Tag
7.2.1.4) Select node
7.2.1.5) Connect ASC Return Value to Target
7.2.1.6) Gameplay Tag: Father.State.Transitioning

### 7.3) Add Transitioning Tag

> **v4.5 (GAS Audit INV-1):** Form transitions NO LONGER grant invulnerability.
> Father is vulnerable during the 5s transition. Use AddLooseGameplayTag for the transitioning state.

#### 7.3.1) Add Loose Gameplay Tag (Father.State.Transitioning)
7.3.1.1) From Apply GE_ExoskeletonState execution pin
7.3.1.2) Drag to right
7.3.1.3) Search: Add Loose Gameplay Tag
7.3.1.4) Select node
7.3.1.5) Connect ASC Return Value to Target
7.3.1.6) Tag: Father.State.Transitioning (blocks form switch during VFX - no invulnerability)

### 7.4) Spawn Transition VFX

#### 7.4.1) Add Spawn System Attached
7.4.1.1) From Add Loose Gameplay Tag execution pin
7.4.1.2) Drag to right
7.4.1.3) Search: Spawn System Attached
7.4.1.4) Select node

#### 7.4.2) Configure VFX
7.4.2.1) System Template: NS_FatherFormTransition
7.4.2.2) Attach to Component: From As BP Father Companion -> Get Mesh
7.4.2.3) Attach Point Name: root
7.4.2.4) Location Type: Snap to Target

### 7.5) Wait 5 Seconds

#### 7.5.1) Add Delay Node
7.5.1.1) From Spawn System Attached execution pin
7.5.1.2) Drag to right
7.5.1.3) Search: Delay
7.5.1.4) Select node
7.5.1.5) Duration: 5.0

### 7.5G) POST-DELAY 3-LAYER GUARDS (NL-GUARD-IDENTITY L1)

> **GAS Audit Compliance (v5.0):** These guards execute IMMEDIATELY after the Delay callback returns.
> They validate that the ability context remains valid before any state-modifying operations.
> Pattern locked as NL-GUARD-IDENTITY L1 per Father_Companion_GAS_Abilities_Audit.md v5.0.
>
> **CRITICAL:** Tags/effects in ASC are the canonical state (GAS truth source), not external enums.

#### 7.5G.1) Guard 1: Validate FatherRef
7.5G.1.1) From **Delay** -> **Completed** execution pin
7.5G.1.2) Drag wire to right and release
7.5G.1.3) Search: `Is Valid`
7.5G.1.4) Select **Utilities > Is Valid** macro node
7.5G.1.5) Connect **FatherRef** variable to **Input Object** pin
7.5G.1.6) Add **Branch** node, connect **Return Value** to **Condition**
7.5G.1.7) From **False** path: Leave unconnected (father destroyed - abort silently)

#### 7.5G.2) Guard 2: Check Transitioning Phase Tag
7.5G.2.1) From **Branch** -> **True** execution pin
7.5G.2.2) From **FatherRef**, Get Ability System Component
7.5G.2.3) Add **Has Matching Gameplay Tag** node
7.5G.2.4) Tag: `Father.State.Transitioning` (phase check)
7.5G.2.5) Add **Branch** node, connect result to **Condition**
7.5G.2.6) From **False** path: Transition interrupted - abort

#### 7.5G.3) Guard 3: Check Form Identity Tag
7.5G.3.1) From **Guard 2 Branch** -> **True** execution pin
7.5G.3.2) Reuse Father ASC from Guard 2
7.5G.3.3) Add **Has Matching Gameplay Tag** node
7.5G.3.4) Tag: `Effect.Father.FormState` (PARENT tag - NL-GUARD-IDENTITY L1)
7.5G.3.5) Add **Branch** node, connect result to **Condition**
7.5G.3.6) From **False** path: Form identity removed - abort
7.5G.3.7) From **True** path: Continue to Section 7.5A (all guards passed)

### 7.5A) Remove Prior Form State (TRANSITION PRELUDE - Option B)

**Purpose:** Before applying GE_ExoskeletonState, remove any existing form state GE. This ensures the Single Active Form State Invariant - exactly one Effect.Father.FormState.* tag at runtime.

#### 7.5A.1) Add BP_RemoveGameplayEffectFromOwnerWithGrantedTags
7.5A.1.1) From Delay Completed execution pin
7.5A.1.2) Drag to right
7.5A.1.3) Search: BP_RemoveGameplayEffectFromOwnerWithGrantedTags
7.5A.1.4) Select node
7.5A.1.5) Connect Father ASC Return Value to Target
7.5A.1.6) Tags: Create tag container with `Effect.Father.FormState` (parent tag - removes ANY prior form state GE)

#### 7.5A.2) Create Tag Container for Parent Tag
7.5A.2.1) Add Make Literal Gameplay Tag node
7.5A.2.2) Tag Value: Effect.Father.FormState
7.5A.2.3) Add Make Gameplay Tag Container From Tag node
7.5A.2.4) Connect Make Literal Tag output to input
7.5A.2.5) Connect container output to BP_RemoveGameplayEffectFromOwnerWithGrantedTags Tags pin

### 7.5B) Apply GE_ExoskeletonState (TRANSITION PRELUDE - Option B)

**Purpose:** Apply the new form state GE to establish Exoskeleton form identity.

#### 7.5B.1) Add Apply Gameplay Effect to Self
7.5B.1.1) From BP_RemoveGameplayEffectFromOwnerWithGrantedTags execution pin
7.5B.1.2) Drag to right
7.5B.1.3) Search: Apply Gameplay Effect to Self
7.5B.1.4) Select node
7.5B.1.5) Connect Father ASC Return Value to Target
7.5B.1.6) Gameplay Effect Class: GE_ExoskeletonState

**Note (INV-1):** GE_ExoskeletonState grants `Effect.Father.FormState.Exoskeleton` only (infinite duration). Per GAS Audit INV-1, no form state GE grants invulnerability.

### 7.6) Detach Father From Previous Form

#### 7.6.1) Add Detach From Actor
7.6.1.1) From Apply GE_ExoskeletonState execution pin
7.6.1.2) Drag to right
7.6.1.3) Search: Detach From Actor
7.6.1.4) Select node
7.6.1.5) Target: Connect As BP Father Companion

#### 7.6.2) Configure Detach Rules
7.6.2.1) Location Rule: Keep World
7.6.2.2) Rotation Rule: Keep World
7.6.2.3) Scale Rule: Keep World

### 7.7) Reposition Father Behind Player

#### 7.7.1) Get Owner Player (Form Switch)
7.7.1.1) From As BP Father Companion
7.7.1.2) Drag to empty space
7.7.1.3) Search: Get Owner Player
7.7.1.4) Select node

#### 7.7.2) Calculate Spawn Position
7.7.2.1) From Get Owner Player Return Value
7.7.2.2) Add Get Actor Location node
7.7.2.3) Add Get Actor Forward Vector node
7.7.2.4) Add Multiply (Vector * Float) node
7.7.2.5) Float value: -200.0 (negative for behind)
7.7.2.6) Add Vector + Vector node
7.7.2.7) Connect: Location + (Forward * -200)

#### 7.7.3) Set Actor Location
7.7.3.1) From Detach From Actor execution pin
7.7.3.2) Drag to right
7.7.3.3) Search: Set Actor Location
7.7.3.4) Select node
7.7.3.5) Target: Connect As BP Father Companion
7.7.3.6) New Location: Connect calculated position

### 7.8) Store and Apply Movement Boosts

#### 7.8.1) Get Character Movement (Form Switch)
7.8.1.1) From Set Actor Location execution pin
7.8.1.2) Drag to right
7.8.1.3) Search: Get Character Movement
7.8.1.4) Select node
7.8.1.5) Target: Connect Get Owner Player Return Value

#### 7.8.2) Store Original Speed
7.8.2.1) From Get Character Movement execution pin
7.8.2.2) Follow same pattern as Phase 6.4 - 6.5
7.8.2.3) Store OriginalMaxWalkSpeed and OriginalJumpZVelocity

#### 7.8.3) Apply Speed and Jump Boosts
7.8.3.1) Follow same pattern as Phase 6.6 - 6.7
7.8.3.2) Apply SpeedBoostMultiplier (1.25) and JumpBoostMultiplier (1.3)

### 7.9) Attach Father to Player Back (Form Switch)

#### 7.9.1) Get Player Mesh
7.9.1.1) From Get Owner Player Return Value
7.9.1.2) Add Get Mesh node

#### 7.9.2) Attach Actor To Component
7.9.2.1) From last SET execution pin
7.9.2.2) Add Attach Actor To Component node
7.9.2.3) Configure same as Phase 6.8.3

### 7.10) Update Father State (Form Switch)

#### 7.10.1) Set Current Form
7.10.1.1) From Attach Actor To Component execution pin
7.10.1.2) Add SET Current Form node
7.10.1.3) Target: As BP Father Companion
7.10.1.4) Current Form: Exoskeleton

#### 7.10.2) Set Is Attached
7.10.2.1) From SET Current Form execution pin
7.10.2.2) Add SET Is Attached node
7.10.2.3) Is Attached: True

### 7.11) Remove Transitioning State

#### 7.12.1) Remove Loose Gameplay Tag
7.12.1.1) From SET Is Attached execution pin
7.12.1.2) Drag to right
7.12.1.3) Search: Remove Loose Gameplay Tag
7.12.1.4) Select node
7.12.1.5) Connect ASC to Target
7.12.1.6) Gameplay Tag: Father.State.Transitioning

### 7.13) Apply Form Cooldown

> **v4.5 (GAS Audit INV-1):** No GE removal needed - Father.State.Transitioning tag was removed in Step 7.12 via RemoveLooseGameplayTag.
> Invulnerability was removed from the Father companion system per GAS Audit decision INV-1.

#### 7.13.1) Commit Ability Cooldown
7.13.1.1) From Remove Loose Gameplay Tag execution pin
7.13.1.2) Drag to right
7.13.1.3) Search: Commit Ability Cooldown
7.13.1.4) Select Commit Ability Cooldown node
7.13.1.5) No parameters needed - uses CooldownGameplayEffectClass automatically

### 7.14A) Grant Form-Specific Abilities to Player (Form Switch)

#### 7.14A.1) Get Player Ability System Component
7.14A.1.1) From Commit Ability Cooldown execution pin
7.14A.1.2) From Get Owner Player Return Value
7.14A.1.3) Search: Get Ability System Component
7.14A.1.4) Select node
7.14A.1.5) Connect Get Owner Player Return Value to Target pin

#### 7.14A.2) Grant GA_FatherExoskeletonDash
7.14A.2.1) From Commit Ability Cooldown execution pin
7.14A.2.2) Drag to right
7.14A.2.3) Search: Give Ability
7.14A.2.4) Select node
7.14A.2.5) Connect Player ASC Return Value to Target pin
7.14A.2.6) Ability Class: GA_FatherExoskeletonDash

#### 7.14A.3) Store DashAbilityHandle
7.14A.3.1) From Give Ability execution pin
7.14A.3.2) Search: Set Dash Ability Handle
7.14A.3.3) Select node
7.14A.3.4) Connect Give Ability Return Value to Dash Ability Handle input
7.14A.3.5) Connect As BP Father Companion to Target pin

#### 7.14A.4) Grant GA_FatherExoskeletonSprint
7.14A.4.1) From Set Dash Ability Handle execution pin
7.14A.4.2) Drag to right
7.14A.4.3) Search: Give Ability
7.14A.4.4) Select node
7.14A.4.5) Connect Player ASC Return Value to Target pin
7.14A.4.6) Ability Class: GA_FatherExoskeletonSprint

#### 7.14A.5) Store SprintAbilityHandle
7.14A.5.1) From Give Ability execution pin
7.14A.5.2) Search: Set Sprint Ability Handle
7.14A.5.3) Select node
7.14A.5.4) Connect Give Ability Return Value to Sprint Ability Handle input
7.14A.5.5) Connect As BP Father Companion to Target pin

#### 7.14A.6) Grant GA_StealthField
7.14A.6.1) From Set Sprint Ability Handle execution pin
7.14A.6.2) Drag to right
7.14A.6.3) Search: Give Ability
7.14A.6.4) Select node
7.14A.6.5) Connect Player ASC Return Value to Target pin
7.14A.6.6) Ability Class: GA_StealthField

#### 7.14A.7) Store StealthAbilityHandle
7.14A.7.1) From Give Ability execution pin
7.14A.7.2) Search: Set Stealth Ability Handle
7.14A.7.3) Select node
7.14A.7.4) Connect Give Ability Return Value to Stealth Ability Handle input
7.14A.7.5) Connect As BP Father Companion to Target pin

### 7.15) End Ability (Form Switch)

#### 7.15.1) Add End Ability Node
7.15.1.1) From Set Stealth Ability Handle execution pin
7.15.1.2) Drag to right
7.15.1.3) Search: End Ability
7.15.1.4) Select node
7.15.1.5) Was Cancelled: Unchecked (False)

---

# PHASE 8: IMPLEMENT ENDABILITY EVENT

## 8) Speed Restoration on Form Change

### 8.1) Add Event EndAbility

#### 8.1.1) Create Event Node
8.1.1.1) Right-click in Event Graph
8.1.1.2) Search: Event EndAbility
8.1.1.3) Select node
8.1.1.4) Position below main flow

### 8.1A) CRITICAL: Check bWasCancelled Before Cleanup (v2.5.5)

**IMPORTANT:** The EndAbility event fires in TWO scenarios:
1. **bWasCancelled = false**: After activation flow calls EndAbility (form is now active)
2. **bWasCancelled = true**: When cancelled by another form's Cancel Abilities With Tag

Cleanup should ONLY run when bWasCancelled = true (form switch in progress).

#### 8.1A.1) Add Branch on bWasCancelled
8.1A.1.1) From Event EndAbility execution pin
8.1A.1.2) Drag wire to right and release
8.1A.1.3) Search: Branch
8.1A.1.4) Select Branch node

#### 8.1A.2) Connect Condition
8.1A.2.1) From Event EndAbility **bWasCancelled** pin (boolean output)
8.1A.2.2) Drag wire to Branch **Condition** input pin
8.1A.2.3) Release to connect

#### 8.1A.3) Configure Branch Paths
8.1A.3.1) FALSE path: No connection needed - execution ends (form is now active, keep state)
8.1A.3.2) TRUE path: Continue to Section 8.2 (Get References for Cleanup)

### 8.2) Get References for Cleanup

#### 8.2.1) Add Get Avatar Actor Node
8.2.1.1) From Event EndAbility **Handle** data pin
8.2.1.2) Drag to empty space
8.2.1.3) Search: Get Avatar Actor from Actor Info
8.2.1.4) Select node

#### 8.2.2) Cast to Father
8.2.2.1) From **Branch TRUE** execution pin (from Section 8.1A.3.2)
8.2.2.2) Add Cast To BP_FatherCompanion node
8.2.2.3) Connect Get Avatar Actor Return Value to Object

#### 8.2.3) Get Owner Player
8.2.3.1) From As BP Father Companion
8.2.3.2) Add Get Owner Player node

#### 8.2.4) Validate Owner
8.2.4.1) From Cast successful execution
8.2.4.2) Add Is Valid node
8.2.4.3) Connect Get Owner Player Return Value to Input Object

### 8.3) Restore Original Movement Speed

#### 8.3.1) Get Character Movement
8.3.1.1) From Is Valid -> Is Valid execution pin
8.3.1.2) Add Get Character Movement node
8.3.1.3) Target: Connect Get Owner Player Return Value

#### 8.3.2) Validate Original Speed
8.3.2.1) Drag GET OriginalMaxWalkSpeed to graph
8.3.2.2) Add > (greater than) node
8.3.2.3) Compare OriginalMaxWalkSpeed > 0.0
8.3.2.4) Add Branch node
8.3.2.5) Connect comparison to Condition

#### 8.3.3) Restore Walk Speed
8.3.3.1) From Branch True execution pin
8.3.3.2) Add SET Max Walk Speed node
8.3.3.3) Target: Get Character Movement Return Value
8.3.3.4) Max Walk Speed: Connect GET OriginalMaxWalkSpeed

#### 8.3.4) Restore Jump Velocity
8.3.4.1) From SET Max Walk Speed execution pin
8.3.4.2) Add SET Jump Z Velocity node
8.3.4.3) Target: Get Character Movement Return Value
8.3.4.4) Jump Z Velocity: Connect GET OriginalJumpZVelocity

### 8.4) Form Tags Auto-Removed

#### 8.4.1) Automatic Cleanup Note
8.4.1.1) Form tags (Father.Form.Exoskeleton, Father.State.Attached) auto-removed when ability ends
8.4.1.2) Activation Owned Tags cleanup handled by GAS lifecycle
8.4.1.3) Continue to ability cleanup

### 8.5) Two-Step Cleanup for GA_FatherExoskeletonDash

#### 8.5.1) Get DashAbilityHandle
8.5.1.1) From SET Jump Z Velocity execution (or via Sequence after Branch merge):
8.5.1.2) From As BP Father Companion, search: Get Dash Ability Handle
8.5.1.3) Add getter node

#### 8.5.2) Validate DashAbilityHandle
8.5.2.1) From Get Dash Ability Handle Return Value
8.5.2.2) Search: Is Valid
8.5.2.3) Add Is Valid (Gameplay Ability Spec Handle) node

#### 8.5.3) Add Branch for Handle Validation
8.5.3.1) Add Branch node
8.5.3.2) Connect Is Valid Return Value to Condition

#### 8.5.4) Step 1: Cancel Dash Ability
8.5.4.1) From Branch True execution:
8.5.4.1.1) From Get Owner Player Return Value, search: Get Ability System Component
8.5.4.1.2) Add Get Ability System Component node (player ASC)
8.5.4.2) From Player ASC:
8.5.4.2.1) Search: Cancel Ability
8.5.4.2.2) Add Cancel Ability node
8.5.4.3) Connect Player ASC Return Value to Target
8.5.4.4) Connect Get Dash Ability Handle Return Value to Ability Handle

#### 8.5.5) Step 2: Set Remove Ability On End
8.5.5.1) From Cancel Ability execution:
8.5.5.2) Search: Set Remove Ability On End
8.5.5.3) Add Set Remove Ability On End node
8.5.5.4) Connect Player ASC Return Value to Target
8.5.5.5) Connect Get Dash Ability Handle Return Value to Ability Handle

### 8.6) Two-Step Cleanup for GA_FatherExoskeletonSprint

#### 8.6.1) Get SprintAbilityHandle
8.6.1.1) From Set Remove Ability On End (Dash) execution (or Branch False):
8.6.1.2) From As BP Father Companion, search: Get Sprint Ability Handle
8.6.1.3) Add getter node

#### 8.6.2) Validate SprintAbilityHandle
8.6.2.1) From Get Sprint Ability Handle Return Value
8.6.2.2) Search: Is Valid
8.6.2.3) Add Is Valid (Gameplay Ability Spec Handle) node

#### 8.6.3) Add Branch for Handle Validation
8.6.3.1) Add Branch node
8.6.3.2) Connect Is Valid Return Value to Condition

#### 8.6.4) Step 1: Cancel Sprint Ability
8.6.4.1) From Branch True execution:
8.6.4.2) Search: Cancel Ability
8.6.4.3) Add Cancel Ability node
8.6.4.4) Connect Player ASC Return Value to Target
8.6.4.5) Connect Get Sprint Ability Handle Return Value to Ability Handle

#### 8.6.5) Step 2: Set Remove Ability On End
8.6.5.1) From Cancel Ability execution:
8.6.5.2) Search: Set Remove Ability On End
8.6.5.3) Add Set Remove Ability On End node
8.6.5.4) Connect Player ASC Return Value to Target
8.6.5.5) Connect Get Sprint Ability Handle Return Value to Ability Handle

### 8.7) Two-Step Cleanup for GA_StealthField

#### 8.7.1) Get StealthAbilityHandle
8.7.1.1) From Set Remove Ability On End (Sprint) execution (or Branch False):
8.7.1.2) From As BP Father Companion, search: Get Stealth Ability Handle
8.7.1.3) Add getter node

#### 8.7.2) Validate StealthAbilityHandle
8.7.2.1) From Get Stealth Ability Handle Return Value
8.7.2.2) Search: Is Valid
8.7.2.3) Add Is Valid (Gameplay Ability Spec Handle) node

#### 8.7.3) Add Branch for Handle Validation
8.7.3.1) Add Branch node
8.7.3.2) Connect Is Valid Return Value to Condition

#### 8.7.4) Step 1: Cancel Stealth Ability
8.7.4.1) From Branch True execution:
8.7.4.2) Search: Cancel Ability
8.7.4.3) Add Cancel Ability node
8.7.4.4) Connect Player ASC Return Value to Target
8.7.4.5) Connect Get Stealth Ability Handle Return Value to Ability Handle

#### 8.7.5) Step 2: Set Remove Ability On End
8.7.5.1) From Cancel Ability execution:
8.7.5.2) Search: Set Remove Ability On End
8.7.5.3) Add Set Remove Ability On End node
8.7.5.4) Connect Player ASC Return Value to Target
8.7.5.5) Connect Get Stealth Ability Handle Return Value to Ability Handle

### 8.8) Reset State Variables

#### 8.8.1) Set Is Attached to False
8.8.1.1) From Set Remove Ability On End (Stealth) execution (or Branch False):
8.8.1.2) From As BP Father Companion, search: Set Is Attached
8.8.1.3) Add Set Is Attached node
8.8.1.4) Is Attached: Unchecked (false)

### 8.9) Verify EndAbility Cleanup Flow

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event EndAbility | Fires when ability ends |
| 2 | Get Avatar Actor From Actor Info | Get father reference |
| 3 | Cast to BP_FatherCompanion | Type-safe reference |
| 4 | Get Owner Player | Get player reference |
| 5 | Is Valid (Owner) | Validate player exists |
| 6 | Get Character Movement | Access movement component |
| 7 | Set Max Walk Speed | Restore original speed |
| 8 | Set Jump Z Velocity | Restore original jump |
| 9 | Form Tags Auto-Removed | Activation Owned Tags cleanup (automatic) |
| 10-12 | Dash cleanup | Cancel then Remove |
| 13-15 | Sprint cleanup | Cancel then Remove |
| 16-18 | Stealth cleanup | Cancel then Remove |
| 19 | Set Is Attached (false) | Reset attachment state |

---

# PHASE 9: COMPILE AND SAVE

## 9) Finalize the Ability

### 9.1) Compile GA_FatherExoskeleton
9.1.1) Click: Compile button in toolbar
9.1.2) Verify: No errors in Compiler Results panel

### 9.2) Save GA_FatherExoskeleton
9.2.1) Click: Save button in toolbar

---

## **INTEGRATION NOTES**

### Ability Granting

GA_FatherExoskeleton is granted automatically via the NPCDefinition system:
- NPCDef_FatherCompanion references AC_FatherCompanion_Default
- AC_FatherCompanion_Default includes GA_FatherExoskeleton at index [2]

### Form State Architecture (Option B)

| Component | Description |
|-----------|-------------|
| Activation Owned Tags | `Father.State.Attached` (ephemeral, auto-granted on activation) |
| Form Identity | GE_ExoskeletonState grants `Effect.Father.FormState.Exoskeleton` (persists via infinite GE) |
| Transitioning Tag | AddLooseGameplayTag(Father.State.Transitioning) during 5s transition |
| Replication | Tags replicate via ReplicateActivationOwnedTags setting |
| Cleanup (State Tags) | Activation Owned Tags auto-removed when ability ends |
| Cleanup (Form Identity) | GE_ExoskeletonState removed by next form's transition prelude |

### Transition Prelude (Option B)

When switching TO Exoskeleton form, the transition prelude executes:
1. `BP_RemoveGameplayEffectFromOwnerWithGrantedTags(Effect.Father.FormState)` - removes any prior form state
2. `ApplyGameplayEffectToSelf(GE_ExoskeletonState)` - applies new form identity

This ensures the Single Active Form State Invariant - exactly one `Effect.Father.FormState.*` tag at runtime.

### Form-Specific Abilities

The following abilities are granted to the player ASC when Exoskeleton form activates (not via EquippableItem):
- GA_FatherExoskeletonDash (stored in DashAbilityHandle)
- GA_FatherExoskeletonSprint (stored in SprintAbilityHandle)
- GA_StealthField (stored in StealthAbilityHandle)

All three abilities use two-step cleanup in EndAbility: Cancel Ability then Set Remove Ability On End.

### Stat Bonuses

Stat bonuses are handled by BP_FatherExoskeletonForm EquippableItem via GE_EquipmentModifier_FatherExoskeleton:
- Attack Rating: +10

### Speed Restoration and Cleanup

EndAbility performs comprehensive cleanup when another form ability cancels this ability:
1. CharacterMovement values restored to original values stored in ability variables
2. State tag (`Father.State.Attached`) auto-removed by Activation Owned Tags
3. Form identity tag (`Effect.Father.FormState.Exoskeleton`) persists via GE_ExoskeletonState until next form's transition prelude removes it
4. All three granted abilities (Dash, Sprint, Stealth) cleaned up via two-step process
5. Is Attached variable reset to false

---

## **RELATED DOCUMENTS**

| Document | Purpose |
|----------|---------|
| GE_FormChangeCooldown_Implementation_Guide_v1_0.md | Form change cooldown (prerequisite) |
| Father_Companion_Technical_Reference_v5_12.md | Section 7.4: Net Execution Policy, Section 14.8: Effect Handle Storage, Section 36.12: Two-Step Cleanup, Section 37: Recruited Tag Gating |
| Father_Companion_System_Setup_Guide_v2_3.md | Phase 12: Effect and Ability Handles |
| DefaultGameplayTags_FatherCompanion_v3_5.ini | All gameplay tags including Father.State.Recruited |
| GA_FatherExoskeletonDash_Implementation_Guide_v3_2.md | Dash ability (granted to player) |
| GA_FatherExoskeletonSprint_Implementation_Guide_v2_4.md | Sprint ability (granted to player) |
| GA_StealthField_Implementation_Guide_v3_2.md | Stealth ability (granted to player) |

---

## **CHANGELOG**

| Version | Date | Changes |
|---------|------|---------|
| 4.9 | January 2026 | **NL-GUARD-IDENTITY L1 (Claude-GPT Audit v5.0 - 2026-01-24):** Added Section 7.5G (POST-DELAY 3-LAYER GUARDS) implementing LOCKED L1 pattern: (1) IsValid(FatherRef), (2) HasMatchingGameplayTag(Father.State.Transitioning) - phase check, (3) HasMatchingGameplayTag(Effect.Father.FormState) - identity check with PARENT tag. Guards execute immediately after 5s Delay callback before any state-modifying operations. |
| 4.8 | January 2026 | **C_SYMBIOTE_STRICT_CANCEL Contract (Claude-GPT Audit - 2026-01-23):** Removed `Ability.Father.Symbiote` from cancel_abilities_with_tag. Symbiote is an ultimate ability (30s duration) that cannot be cancelled by player-initiated form changes. Defense-in-depth: Layer 1 blocks via `Father.State.SymbioteLocked` in activation_blocked_tags, Layer 2 ensures no cancel path exists. See LOCKED_CONTRACTS.md Contract 11. |
| 4.7 | January 2026 | **Locked Decisions Reference:** Added Father_Companion_GAS_Abilities_Audit.md reference. This guide complies with: INV-1 (no invulnerability), Rule 4 (First Activation path merges into setup chain). Updated Technical Reference to v6.2. |
| 4.6 | January 2026 | **INV-1 Compliance:** Removed all invulnerability references per GAS Audit decision INV-1. GE_ExoskeletonState now grants only Effect.Father.FormState.Exoskeleton (no Narrative.State.Invulnerable). Removed GE_Invulnerable steps from Architecture Overview and Node Flow Summary. Only GA_FatherSacrifice grants invulnerability (to player for 8s). |
| 4.4 | January 2026 | **Option B Form State Architecture:** GE_ExoskeletonState for form identity (grants Effect.Father.FormState.Exoskeleton), transition prelude (RemovePriorFormState + ApplyExoskeletonState). Fixed Activation Owned Tags to `Father.State.Attached` only. Updated to UE 5.7. Added AUTOMATION VS MANUAL table. Added orphan tag notes. Fixed SpeedBoostMultiplier to 1.25 per manifest. |
| 3.10 | January 2026 | Fixed tag format: State.Father.Alive changed to Father.State.Alive per DefaultGameplayTags. Updated Related Documents to Technical Reference v5.12 and Setup Guide v2.3. Fixed curly quotes to straight ASCII. |
| 3.9 | January 2026 | Fixed truncated GE name in Quick Reference: GE_EquipmentMod corrected to GE_EquipmentModifier_FatherExoskeleton. |
| 3.8 | January 2026 | Simplified documentation: Tag configuration (PHASE 3) and variable creation (PHASE 4) converted to single markdown tables. |
| 3.7 | January 2026 | Simplified PHASE 1 tag creation - replaced detailed step-by-step instructions with simple tag list table. |
| 3.6 | January 2026 | Built-in cooldown system: Added CooldownGameplayEffectClass property, replaced Apply GE with CommitAbilityCooldown node. Removed Block Abilities with Tag for cooldown (GAS auto-blocks). |
| 3.5 | January 2026 | Form tags via Activation Owned Tags. ReplicateActivationOwnedTags required. Simplified ability logic and EndAbility cleanup. |
| 3.4 | December 2025 | Net Execution Policy: Server Only for cross-actor operations. |
| 3.3 | December 2025 | Added Father.State.Recruited gating. Handle-based effect removal. |
| 3.2 | December 2025 | Added form switch flow with 5s transition. Fixed JumpBoostMultiplier to 1.3. |
| 3.1 | December 2025 | Cancel Abilities with Tag. Self-cleaning EndAbility. |
| 3.0 | November 2025 | Initial GE_ExoskeletonState architecture. |

---

**END OF GA_FATHEREXOSKELETON IMPLEMENTATION GUIDE v4.6**

**Blueprint-Only Implementation for Unreal Engine 5.7 + Narrative Pro Plugin v2.2**
**Architecture: Option B (GE-Based Form Identity)**
