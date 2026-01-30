# GA_StealthField Implementation Guide v3.8 - GAS Audit v6.5 Compliant

## Exoskeleton Form - Player + Attached Father Invisibility

### Unreal Engine 5.7 + Narrative Pro v2.2

### Blueprint-Only Implementation

**Contracts:**
- Father_Companion_GAS_Abilities_Audit.md v6.5 - MEDIUM MED-4 RESOLVED
- LOCKED_CONTRACTS.md v4.32 - Contract 14 (INV-INPUT-1) COMPLIANT

> **GAS Audit v6.5 Compliance:** Uses AbilityTaskWaitDelay + IsValid guard (verified v4.15). Input tag uses `Narrative.Input.Ability2` per INV-INPUT-1 (v3.8).

---

## DOCUMENT INFORMATION

| Property | Value |
|----------|-------|
| Document Version | 3.8 |
| Ability Name | GA_StealthField |
| Ability Type | Active Player Ability |
| Parent Class | NarrativeGameplayAbility |
| Form | Exoskeleton (Father Attached to Back) |
| Input | Ability2 (Narrative.Input.Ability2) |

---

## TABLE OF CONTENTS

1. [Document Information](#document-information)
2. [Prerequisites](#prerequisites)
3. [Ability Overview](#ability-overview)
4. [Phase 1: Gameplay Tags Setup](#phase-1-gameplay-tags-setup)
5. [Phase 2: Create Gameplay Effects](#phase-2-create-gameplay-effects)
6. [Phase 3: Create GA_StealthField Ability](#phase-3-create-ga_stealthfield-ability)
7. [Phase 4: Implement Stealth Activation Logic](#phase-4-implement-stealth-activation-logic)
8. [Phase 5: Implement Break Stealth Logic](#phase-5-implement-break-stealth-logic)
9. [Phase 6: AI Perception Integration](#phase-6-ai-perception-integration)
10. [Phase 7: Grant Ability via EquippableItem](#phase-7-grant-ability-via-equippableitem)
11. [Phase 8: Input Configuration](#phase-8-input-configuration)
12. [Quick Reference](#quick-reference)
13. [Changelog](#changelog)

---

## PREREQUISITES

### Required Assets

| Asset | Type | Purpose |
|-------|------|---------|
| BP_FatherCompanion | Blueprint | Father character with ASC |
| EQI_FatherExoskeleton | EquippableItem | Grants Exoskeleton form abilities |
| BP_NarrativeNPCController | Blueprint | NPC AI controller to modify |

### Required Systems

| System | Status |
|--------|--------|
| Gameplay Ability System | Enabled |
| Narrative Pro Plugin v2.2 | Installed |
| Enhanced Input System | Configured |

### Required Tags (Pre-existing)

| Tag | Purpose |
|-----|---------|
| Effect.Father.FormState.Exoskeleton | Form identification |
| Father.State.Attached | Attachment state |
| Father.State.Recruited | Recruitment state |

---

## ABILITY OVERVIEW

| Parameter | Value |
|-----------|-------|
| Duration | 8 seconds |
| Movement Speed Reduction | -20% (0.8x multiplier) |
| Cooldown | 15 seconds |
| Break on Attack | Yes |
| Break on Damage | Yes |
| Form Required | Exoskeleton |
| State Required | Father.State.Attached, Father.State.Recruited |
| Invisibility Targets | Player AND Attached Father |

### Damage Synergy

| Condition | AttackRating Bonus | Source |
|-----------|-------------------|--------|
| Normal Attack | +0 | Base |
| Behind Enemy (Backstab) | +25 | GA_Backstab |
| First Attack from Stealth | +50 | GA_StealthField |
| Stealth + Behind Combined | +75 | Both (additive) |

---

## PHASE 1: GAMEPLAY TAGS SETUP

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Exoskeleton.StealthField | Father stealth field ability - Exoskeleton form |
| Father.State.Stealthed | Father and player are in stealth mode |
| State.Invisible | Character invisible to AI perception |
| Player.State.Stealth | Player is invisible to AI |
| Effect.Father.Stealth | Stealth effect active on character |
| Effect.Father.StealthDamageBonus | +50% damage bonus from stealth attack |
| Effect.Father.StealthSpeedReduction | -20% movement speed during stealth |
| Cooldown.Father.Exoskeleton.StealthField | 15 second stealth ability cooldown |

### **Verify Existing Tags**

| Tag Name | Purpose |
|----------|---------|
| Effect.Father.FormState.Exoskeleton | Form identification (Activation Required) |
| Father.State.Attached | Attachment state (Activation Required) |
| Father.State.Recruited | Recruitment state (Activation Required) |

---

## PHASE 2: CREATE GAMEPLAY EFFECTS

### 7) Create Effects Folder Structure

#### 7.1) Navigate to Father Content
   - 7.1.1) In Content Browser, go to: `/Content/FatherCompanion/`

#### 7.2) Create Stealth Folder
   - 7.2.1) Right-click in Content Browser
   - 7.2.2) Select: **New Folder**
   - 7.2.3) Name: `Stealth`
   - 7.2.4) Press: **Enter**

#### 7.3) Create Effects Subfolder
   - 7.3.1) Double-click: **Stealth** folder to open
   - 7.3.2) Right-click in Content Browser
   - 7.3.3) Select: **New Folder**
   - 7.3.4) Name: `Effects`
   - 7.3.5) Press: **Enter**
   - 7.3.6) Double-click: **Effects** folder to open

### 8) Create GE_StealthInvisibility

#### 8.1) Create Gameplay Effect Asset
   - 8.1.1) Right-click in `/Content/FatherCompanion/Stealth/Effects/`
   - 8.1.2) Select: **Gameplay** -> **Gameplay Effect**
   - 8.1.3) Name: `GE_StealthInvisibility`
   - 8.1.4) Press: **Enter**
   - 8.1.5) Double-click: **GE_StealthInvisibility** to open

#### 8.2) Configure Duration Policy
   - 8.2.1) Click: **Class Defaults** button
   - 8.2.2) In Details panel, find: **Duration Policy** property
   - 8.2.3) Set: `Has Duration`
   - 8.2.4) Find: **Duration Magnitude** section
   - 8.2.5) Expand: **Duration Magnitude**
   - 8.2.6) Set **Scalable Float Magnitude** value: `8.0`

#### 8.3) Add Grant Tags Component (UE 5.6)

##### 8.3.1) Add Component
   - 8.3.1.1) In Details panel, find: **Components** section
   - 8.3.1.2) Click: **+** button next to Components array
   - 8.3.1.3) From dropdown, select: **Grant Tags to Target Actor**

##### 8.3.2) Configure Granted Tags
   - 8.3.2.1) Expand: Grant Tags to Target Actor component
   - 8.3.2.2) Find: **Add to Inherited** array
   - 8.3.2.3) Click: **+** button to add first tag
   - 8.3.2.4) Select: `State.Invisible`
   - 8.3.2.5) Click: **+** button to add second tag
   - 8.3.2.6) Select: `Player.State.Stealth`
   - 8.3.2.7) Click: **+** button to add third tag
   - 8.3.2.8) Select: `Effect.Father.Stealth`

#### 8.4) Add Modifier for StealthRating

##### 8.4.1) Create Modifier Entry
   - 8.4.1.1) In Details panel, find: **Modifiers** section
   - 8.4.1.2) Click: **+** button next to Modifiers array
   - 8.4.1.3) Element [0] appears
   - 8.4.1.4) Expand: Element [0]

##### 8.4.2) Configure Attribute
   - 8.4.2.1) Find: **Attribute** property
   - 8.4.2.2) Click: Attribute dropdown
   - 8.4.2.3) Search: `StealthRating`
   - 8.4.2.4) Select: `NarrativeAttributeSetBase.StealthRating`

##### 8.4.3) Configure Modifier Operation
   - 8.4.3.1) Find: **Modifier Op** property
   - 8.4.3.2) Set: `Override`

##### 8.4.4) Configure Modifier Magnitude
   - 8.4.4.1) Find: **Scalable Float Magnitude** section
   - 8.4.4.2) Set **Value**: `100.0`

#### 8.5) Compile and Save
   - 8.5.1) Click: **Compile** button
   - 8.5.2) Click: **Save** button

### 9) Create GE_StealthDamageBonus

#### 9.1) Create Gameplay Effect Asset
   - 9.1.1) Right-click in Effects folder
   - 9.1.2) Select: **Gameplay** -> **Gameplay Effect**
   - 9.1.3) Name: `GE_StealthDamageBonus`
   - 9.1.4) Double-click to open

#### 9.2) Configure Duration Policy
   - 9.2.1) In Details panel, find: **Duration Policy**
   - 9.2.2) Set: `Has Duration`
   - 9.2.3) Set **Duration Magnitude** value: `8.0`

#### 9.3) Add Attack Rating Modifier

##### 9.3.1) Create Modifier Entry
   - 9.3.1.1) Find: **Modifiers** section
   - 9.3.1.2) Click: **+** button
   - 9.3.1.3) Expand: Element [0]

##### 9.3.2) Configure Attribute
   - 9.3.2.1) **Attribute**: `NarrativeAttributeSetBase.AttackRating`
   - 9.3.2.2) **Modifier Op**: `Add`
   - 9.3.2.3) **Scalable Float Value**: `50.0`

#### 9.4) Add Grant Tags Component

##### 9.4.1) Add Component
   - 9.4.1.1) Click: **+** next to Components
   - 9.4.1.2) Select: **Grant Tags to Target Actor**

##### 9.4.2) Configure Tag
   - 9.4.2.1) In **Add to Inherited** array
   - 9.4.2.2) Add tag: `Effect.Father.StealthDamageBonus`

#### 9.5) Compile and Save
   - 9.5.1) Click: **Compile**
   - 9.5.2) Click: **Save**

### 10) Create GE_StealthFieldCooldown

#### 10.1) Create Gameplay Effect Asset
   - 10.1.1) Right-click in Effects folder
   - 10.1.2) Select: **Gameplay** -> **Gameplay Effect**
   - 10.1.3) Name: `GE_StealthFieldCooldown`
   - 10.1.4) Double-click to open

#### 10.2) Configure Duration Policy
   - 10.2.1) Set **Duration Policy**: `Has Duration`
   - 10.2.2) Set **Duration Magnitude** value: `15.0`

#### 10.3) Add Grant Tags Component

##### 10.3.1) Add Component
   - 10.3.1.1) Click: **+** next to Components
   - 10.3.1.2) Select: **Grant Tags to Target Actor**

##### 10.3.2) Configure Tag
   - 10.3.2.1) In **Add to Inherited** array
   - 10.3.2.2) Add tag: `Cooldown.Father.Exoskeleton.StealthField`

#### 10.4) Compile and Save
   - 10.4.1) Click: **Compile**
   - 10.4.2) Click: **Save**

---

## PHASE 3: CREATE GA_STEALTHFIELD ABILITY

### 11) Create Ability Blueprint

#### 11.1) Navigate to Abilities Folder
   - 11.1.1) In Content Browser, go to: `/Content/FatherCompanion/Abilities/`

#### 11.2) Create New Gameplay Ability
   - 11.2.1) Right-click in folder
   - 11.2.2) Select: **Blueprint Class**
   - 11.2.3) In Pick Parent Class, click: **All Classes**
   - 11.2.4) Search: `NarrativeGameplayAbility`
   - 11.2.5) Select: **NarrativeGameplayAbility**
   - 11.2.6) Click: **Select**
   - 11.2.7) Name: `GA_StealthField`
   - 11.2.8) Press: **Enter**

#### 11.3) Open Ability Blueprint
   - 11.3.1) Double-click: **GA_StealthField** to open

### 12) Configure Ability Properties

#### 12.1) Open Class Defaults
   - 12.1.1) Click: **Class Defaults** button in toolbar

#### 12.2) Configure Ability Tags

##### 12.2.1) Set Ability Tags

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Exoskeleton.StealthField |
| Activation Required Tags | Effect.Father.FormState.Exoskeleton, Father.State.Attached, Father.State.Recruited |
| Activation Blocked Tags | Cooldown.Father.Exoskeleton.StealthField, Father.State.Stealthed |
| Activation Owned Tags | Father.State.Stealthed |
| Cancel Abilities with Tag | State.Attacking |

#### 12.3) Configure Replication Settings

##### 12.4.1) Set Instancing Policy
   - 12.4.1.1) Find: **Instancing Policy** property
   - 12.4.1.2) Set: `Instanced Per Actor`

##### 12.4.2) Set Net Execution Policy
   - 12.4.2.1) Find: **Net Execution Policy** property
   - 12.4.2.2) Set: `Local Predicted`

##### 12.4.3) Set Replication Policy
   - 12.4.3.1) Find: **Replication Policy** property
   - 12.4.3.2) Set: `Replicate Yes`

#### 12.5) Configure Input Tag
   - 12.5.1) Find: **InputTag** property
   - 12.5.2) Set: `Narrative.Input.Ability2`

### 13) Create Variables

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| PlayerRef | NarrativePlayerCharacter (Object Reference) | None | No |
| FatherRef | BP_FatherCompanion (Object Reference) | None | No |
| PlayerASC | NarrativeAbilitySystemComponent (Object Reference) | None | No |
| PlayerInvisibilityHandle | Active Gameplay Effect Handle (Structure) | None | No |
| FatherInvisibilityHandle | Active Gameplay Effect Handle (Structure) | None | No |
| DamageBonusHandle | Active Gameplay Effect Handle (Structure) | None | No |
| OriginalWalkSpeed | Float | 0.0 | No |
| StealthDuration | Float | 8.0 | Yes |
| SpeedReductionMultiplier | Float | 0.8 | Yes |

### 14) Compile Variables
   - 14.1) Click: **Compile** button

---

## PHASE 4: IMPLEMENT STEALTH ACTIVATION LOGIC

### 15) Create ActivateAbility Event

#### 15.1) Add Event Node
   - 15.1.1) In Event Graph, right-click
   - 15.1.2) Search: `Event Activate Ability`
   - 15.1.3) Select: **Event Activate Ability**

### 16) Get Player and Father References

#### 16.1) Get Avatar Actor (Father)
   - 16.1.1) Drag from: Event Activate Ability execution pin
   - 16.1.2) Search: `Get Avatar Actor from Actor Info`
   - 16.1.3) Select: **Get Avatar Actor from Actor Info**

#### 16.2) Cast to Father
   - 16.2.1) Drag from: Get Avatar Actor Return Value
   - 16.2.2) Search: `Cast to BP_FatherCompanion`
   - 16.2.3) Select: **Cast To BP_FatherCompanion**
   - 16.2.4) Connect execution wire

#### 16.3) Store Father Reference
   - 16.3.1) Drag from: **As BP Father Companion** pin
   - 16.3.2) Search: `Set Father Ref`
   - 16.3.3) Select: **Set FatherRef**
   - 16.3.4) Connect execution from Cast success

#### 16.4) Get Owner Player from Father
   - 16.4.1) Drag from: **As BP Father Companion** pin
   - 16.4.2) Search: `Get Owner Player`
   - 16.4.3) Select: **Get Owner Player**

#### 16.5) Cast to Player Character
   - 16.5.1) Drag from: Get Owner Player Return Value
   - 16.5.2) Search: `Cast to NarrativePlayerCharacter`
   - 16.5.3) Select: **Cast To NarrativePlayerCharacter**

#### 16.6) Store Player Reference
   - 16.6.1) Drag from: **As Narrative Player Character** pin
   - 16.6.2) Search: `Set Player Ref`
   - 16.6.3) Select: **Set PlayerRef**
   - 16.6.4) Connect execution from Cast success

#### 16.7) Validate Player Reference
   - 16.7.1) From Set PlayerRef execution:
   - 16.7.1.1) Add: **Is Valid** node
   - 16.7.1.2) Drag from PlayerRef variable
   - 16.7.1.3) Connect PlayerRef to Is Valid input

#### 16.8) Branch on Validity
   - 16.8.1) From Is Valid Return Value:
   - 16.8.1.1) Add: **Branch** node
   - 16.8.1.2) Connect Is Valid to Condition

#### 16.9) Handle Invalid (End Ability)
   - 16.9.1) From Branch **False** execution:
   - 16.9.2) Add: **End Ability** node
   - 16.9.3) Was Cancelled: Checked

### 17) Get and Store PlayerASC

#### 17.1) Get Ability System Component
   - 17.1.1) From Branch True execution:
   - 17.1.1.1) Drag from PlayerRef variable
   - 17.1.1.2) Add: **Get Ability System Component** node

#### 17.2) Cast to NarrativeAbilitySystemComponent
   - 17.2.1) From ASC Return Value:
   - 17.2.1.1) Add: **Cast To NarrativeAbilitySystemComponent** node
   - 17.2.1.2) Connect execution

#### 17.3) Store PlayerASC
   - 17.3.1) From Cast Success execution:
   - 17.3.1.1) Add: **Set PlayerASC** node
   - 17.3.1.2) Connect As NarrativeAbilitySystemComponent to value

#### 17.4) Handle Cast Failure
   - 17.4.1) From Cast Failure execution:
   - 17.4.1.1) Add: **End Ability** node
   - 17.4.1.2) Was Cancelled: Checked

#### 17.5) Validate PlayerASC
   - 17.5.1) From Set PlayerASC execution:
   - 17.5.1.1) Add: **Is Valid** node
   - 17.5.1.2) Drag from PlayerASC variable
   - 17.5.1.3) Connect PlayerASC to Is Valid input

#### 17.6) Branch on ASC Validity
   - 17.6.1) From Is Valid Return Value:
   - 17.6.1.1) Add: **Branch** node
   - 17.6.1.2) Connect Is Valid to Condition

#### 17.7) Handle Invalid ASC
   - 17.7.1) From Branch False execution:
   - 17.7.1.1) Add: **End Ability** node
   - 17.7.1.2) Was Cancelled: Checked

### 18) Store Original Walk Speed

#### 18.1) Get Character Movement
   - 18.1.1) From Branch **True** execution:
   - 18.1.2) Drag from: PlayerRef variable
   - 18.1.3) Search: `Get Character Movement`
   - 18.1.4) Select: **Get Character Movement**

#### 18.2) Get Current Max Walk Speed
   - 18.2.1) Drag from: Character Movement Return Value
   - 18.2.2) Search: `Get Max Walk Speed`
   - 18.2.3) Select: **Get Max Walk Speed**

#### 18.3) Store Original Speed
   - 18.3.1) Add: **Set OriginalWalkSpeed** node
   - 18.3.2) Connect: Get Max Walk Speed return to OriginalWalkSpeed input
   - 18.3.3) Connect execution

### 19) Apply Speed Reduction

#### 19.1) Calculate Reduced Speed
   - 19.1.1) Drag from: OriginalWalkSpeed getter
   - 19.1.2) Add: **Multiply (Float)** node
   - 19.1.3) Connect: OriginalWalkSpeed to first input (A)
   - 19.1.4) Connect: SpeedReductionMultiplier getter to second input (B)

#### 19.2) Set Reduced Walk Speed
   - 19.2.1) From Set OriginalWalkSpeed execution:
   - 19.2.2) Drag from: Character Movement reference
   - 19.2.3) Search: `Set Max Walk Speed`
   - 19.2.4) Select: **Set Max Walk Speed**
   - 19.2.5) Connect execution
   - 19.2.6) Connect: Multiply result to Max Walk Speed input

### 20) Apply Invisibility to Player

#### 20.1) Apply Player Invisibility Effect
   - 20.1.1) From Set Max Walk Speed execution:
   - 20.1.2) Drag from: PlayerASC variable
   - 20.1.3) Search: `Apply Gameplay Effect to Self`
   - 20.1.4) Select: **Apply Gameplay Effect to Self**
   - 20.1.5) Connect execution
   - 20.1.6) **Gameplay Effect Class**: Select `GE_StealthInvisibility`
   - 20.1.7) **Level**: `1`

#### 20.2) Store Player Effect Handle
   - 20.2.1) Drag from: Apply Gameplay Effect **Return Value** pin
   - 20.2.2) Add: **Set PlayerInvisibilityHandle** node
   - 20.2.3) Connect execution from Apply Gameplay Effect

### 21) Apply Invisibility to Attached Father

#### 21.1) Get Father's ASC
   - 21.1.1) Drag from: FatherRef getter
   - 21.1.2) Search: `Get Ability System Component`
   - 21.1.3) Select: **Get Ability System Component**

#### 21.2) Apply Father Invisibility Effect
   - 21.2.1) From Set PlayerInvisibilityHandle execution:
   - 21.2.2) Drag from: Father ASC
   - 21.2.3) Search: `Apply Gameplay Effect to Self`
   - 21.2.4) Select: **Apply Gameplay Effect to Self**
   - 21.2.5) Connect execution
   - 21.2.6) **Gameplay Effect Class**: Select `GE_StealthInvisibility`
   - 21.2.7) **Level**: `1`

#### 21.3) Store Father Effect Handle
   - 21.3.1) Drag from: Apply Gameplay Effect **Return Value** pin
   - 21.3.2) Add: **Set FatherInvisibilityHandle** node
   - 21.3.3) Connect execution from Apply Gameplay Effect

### 22) Apply Damage Bonus to Player

#### 22.1) Apply Damage Bonus Effect
   - 22.1.1) From Set FatherInvisibilityHandle execution:
   - 22.1.2) Drag from: PlayerASC variable
   - 22.1.3) Add: **Apply Gameplay Effect to Self** node
   - 22.1.4) Connect execution
   - 22.1.5) **Gameplay Effect Class**: Select `GE_StealthDamageBonus`
   - 22.1.6) **Level**: `1`

#### 22.2) Store Damage Bonus Handle
   - 22.2.1) Drag from: Apply Gameplay Effect Return Value
   - 22.2.2) Add: **Set DamageBonusHandle** node
   - 22.2.3) Connect execution

### 23) Clear ViewedCharacter from NPCs

#### 23.1) Get All NPC Controllers
   - 23.1.1) From Set DamageBonusHandle execution:
   - 23.1.2) Right-click, search: `Get All Actors of Class`
   - 23.1.3) Select: **Get All Actors of Class**
   - 23.1.4) **Actor Class**: Select `BP_NarrativeNPCController`

#### 23.2) Create For Each Loop
   - 23.2.1) Drag from: Get All Actors **Out Actors** pin
   - 23.2.2) Add: **For Each Loop** node
   - 23.2.3) Connect execution from Set DamageBonusHandle

#### 23.3) Cast to NPC Controller
   - 23.3.1) Drag from: For Each **Array Element** pin
   - 23.3.2) Add: **Cast To BP_NarrativeNPCController**
   - 23.3.3) Connect execution from Loop Body

#### 23.4) Get ViewedCharacter
   - 23.4.1) Drag from: **As BP Narrative NPC Controller** pin
   - 23.4.2) Search: `Get Viewed Character`
   - 23.4.3) Select: **Get Viewed Character**

#### 23.5) Compare to Player
   - 23.5.1) Drag from: Get Viewed Character Return Value
   - 23.5.2) Add: **Equal (Object)** node
   - 23.5.3) Connect: PlayerRef getter to second input (B)

#### 23.6) Branch on Match
   - 23.6.1) From Cast success execution:
   - 23.6.2) Add: **Branch** node
   - 23.6.3) Connect: Equal result to Condition

#### 23.7) Clear ViewedCharacter
   - 23.7.1) From Branch **True** execution:
   - 23.7.2) Drag from: **As BP Narrative NPC Controller** pin
   - 23.7.3) Search: `Set Viewed Character`
   - 23.7.4) Select: **Set Viewed Character**
   - 23.7.5) Leave **Viewed Character** input empty (None)
   - 23.7.6) Connect execution

### 24) Start Stealth Duration Timer

#### 24.1) Add Delay Node
   - 24.1.1) From For Each **Completed** execution:
   - 24.1.2) Add: **Delay** node
   - 24.1.3) Connect: StealthDuration getter to Duration input

#### 24.2) Call End Stealth on Completion
   - 24.2.1) From Delay **Completed** execution:
   - 24.2.2) Drag and search: `End Stealth`
   - 24.2.3) Select: **End Stealth** (created in Phase 5)
   - 24.2.4) **Was Cancelled**: Unchecked (false)

### 25) Compile Activation Logic
   - 25.1) Click: **Compile** button

---

## PHASE 5: IMPLEMENT BREAK STEALTH LOGIC

### 26) Create EndStealth Function

#### 26.1) Add Function
   - 26.1.1) In My Blueprint panel, click: **+** next to Functions
   - 26.1.2) Name: `EndStealth`
   - 26.1.3) Press: **Enter**

#### 26.2) Add Input Parameter
   - 26.2.1) Select: Function entry node
   - 26.2.2) In Details panel, find: **Inputs** section
   - 26.2.3) Click: **+** to add input
   - 26.2.4) Name: `WasCancelled`
   - 26.2.5) Type: `Boolean`

### 27) Restore Player Walk Speed

#### 27.1) Validate Player
   - 27.1.1) From function entry execution:
   - 27.1.2) Drag from PlayerRef variable
   - 27.1.3) Add: **Is Valid** node
   - 27.1.4) Connect PlayerRef to input

#### 27.2) Branch on Validity
   - 27.2.1) Add: **Branch** node
   - 27.2.2) Connect: Is Valid return to Condition
   - 27.2.3) Connect execution from function entry

#### 27.3) Get Character Movement
   - 27.3.1) From Branch True execution:
   - 27.3.2) Drag from PlayerRef variable
   - 27.3.3) Add: **Get Character Movement** node

#### 27.4) Restore Original Speed
   - 27.4.1) From Branch **True** execution:
   - 27.4.2) Drag from: Character Movement
   - 27.4.3) Add: **Set Max Walk Speed** node
   - 27.4.4) Connect execution
   - 27.4.5) Connect: OriginalWalkSpeed getter to Max Walk Speed input

### 28) Remove Player Invisibility Effect

#### 28.1) Remove Player Invisibility
   - 28.1.1) From Set Max Walk Speed execution:
   - 28.1.2) Drag from: PlayerASC variable
   - 28.1.3) Search: `Remove Active Gameplay Effect by Source Effect`
   - 28.1.4) Select: **Remove Active Gameplay Effect by Source Effect**
   - 28.1.5) Connect execution
   - 28.1.6) Connect: PlayerInvisibilityHandle getter to Handle input
   - 28.1.7) **Stacks to Remove**: `-1`

### 29) Remove Father Invisibility Effect

#### 29.1) Get Father ASC
   - 29.1.1) Drag from: FatherRef getter
   - 29.1.2) Add: **Get Ability System Component** node

#### 29.2) Validate Father
   - 29.2.1) Add: **Is Valid** node
   - 29.2.2) Connect: FatherRef getter to input

#### 29.3) Branch on Father Validity
   - 29.3.1) From Remove Player Effect execution:
   - 29.3.2) Add: **Branch** node
   - 29.3.3) Connect: Is Valid return to Condition

#### 29.4) Remove Father Invisibility
   - 29.4.1) From Branch **True** execution:
   - 29.4.2) Drag from: Father ASC
   - 29.4.3) Add: **Remove Active Gameplay Effect by Source Effect** node
   - 29.4.4) Connect execution
   - 29.4.5) Connect: FatherInvisibilityHandle getter to Handle input
   - 29.4.6) **Stacks to Remove**: `-1`

### 30) Remove Damage Bonus Effect

#### 30.1) Remove Damage Bonus
   - 30.1.1) From Remove Father Effect execution (or Branch False):
   - 30.1.2) Drag from: PlayerASC variable
   - 30.1.3) Add: **Remove Active Gameplay Effect by Source Effect** node
   - 30.1.4) Connect execution
   - 30.1.5) Connect: DamageBonusHandle getter to Handle input
   - 30.1.6) **Stacks to Remove**: `-1`

### 31) Apply Cooldown Effect

#### 31.1) Apply Cooldown
   - 31.1.1) From Remove Damage Bonus execution:
   - 31.1.2) Add: **Apply Gameplay Effect to Owner** node
   - 31.1.3) Connect execution
   - 31.1.4) **Gameplay Effect Class**: Select `GE_StealthFieldCooldown`
   - 31.1.5) **Level**: `1`

### 32) Clear References and End Ability

#### 32.1) Clear PlayerRef
   - 32.1.1) From Apply Cooldown execution:
   - 32.1.2) Add: **Set PlayerRef** node
   - 32.1.3) Value: None (leave disconnected)

#### 32.2) Clear FatherRef
   - 32.2.1) From Set PlayerRef execution:
   - 32.2.2) Add: **Set FatherRef** node
   - 32.2.3) Value: None (leave disconnected)

#### 32.3) Clear PlayerASC
   - 32.3.1) From Set FatherRef execution:
   - 32.3.2) Add: **Set PlayerASC** node
   - 32.3.3) Value: None (leave disconnected)

#### 32.4) End Ability Node
   - 32.4.1) From Set PlayerASC execution:
   - 32.4.2) Add: **End Ability** node
   - 32.4.3) Connect execution

### 33) Handle Branch False Path

#### 33.1) Connect False to End
   - 33.1.1) From initial validity Branch **False**:
   - 33.1.2) Connect to: End Ability node

### 34) Compile EndStealth Function
   - 34.1) Click: **Compile** button

### 35) Create Break Stealth Event Handlers

#### 35.1) Return to Event Graph
   - 35.1.1) Click: **Event Graph** tab

#### 35.2) Add Gameplay Event for Attack

##### 35.2.1) Add Event Node
   - 35.2.1.1) Right-click in Event Graph
   - 35.2.1.2) Search: `Event On Gameplay Event`
   - 35.2.1.3) Select: **Event On Gameplay Event**

##### 35.2.2) Configure Event Tag
   - 35.2.2.1) In Details panel of event node
   - 35.2.2.2) Find: **Event Tag** property
   - 35.2.2.3) Set: `State.Attacking`

##### 35.2.3) Call EndStealth
   - 35.2.3.1) From Event execution:
   - 35.2.3.2) Add: **End Stealth** function call
   - 35.2.3.3) **Was Cancelled**: Checked (true)

#### 35.3) Add Gameplay Event for Damage

##### 35.3.1) Add Second Event Node
   - 35.3.1.1) Right-click in Event Graph
   - 35.3.1.2) Add another: **Event On Gameplay Event**

##### 35.3.2) Configure Event Tag
   - 35.3.2.1) **Event Tag**: `State.TakingDamage`

##### 35.3.3) Call EndStealth
   - 35.3.3.1) From Event execution:
   - 35.3.3.2) Add: **End Stealth** function call
   - 35.3.3.3) **Was Cancelled**: Checked (true)

### 36) Implement Event OnEndAbility

#### 36.1) Add Event OnEndAbility
   - 36.1.1) Right-click in Event Graph
   - 36.1.2) Search: `Event OnEndAbility`
   - 36.1.3) Select: **Event OnEndAbility**

#### 36.2) Validate PlayerASC for Cleanup
   - 36.2.1) From Event OnEndAbility execution:
   - 36.2.1.1) Drag from PlayerASC variable
   - 36.2.1.2) Add: **Is Valid** node
   - 36.2.1.3) Connect PlayerASC to Is Valid input

#### 36.3) Add Branch
   - 36.3.1) From Is Valid Return Value:
   - 36.3.1.1) Add: **Branch** node
   - 36.3.1.2) Connect Is Valid to Condition

#### 36.4) Clear References on Early Cancel
   - 36.4.1) From Branch True execution:
   - 36.4.1.1) Add: **Set PlayerRef** node
   - 36.4.1.2) Value: None (leave disconnected)
   - 36.4.2) From Set PlayerRef execution:
   - 36.4.2.1) Add: **Set FatherRef** node
   - 36.4.2.2) Value: None (leave disconnected)
   - 36.4.3) From Set FatherRef execution:
   - 36.4.3.1) Add: **Set PlayerASC** node
   - 36.4.3.2) Value: None (leave disconnected)

### 37) Compile and Save GA_StealthField

#### 37.1) Finalize
   - 37.1.1) Click: **Compile** button
   - 37.1.2) Click: **Save** button

---

## PHASE 6: AI PERCEPTION INTEGRATION

### 38) Open BP_NarrativeNPCController

#### 38.1) Navigate to Controller Blueprint
   - 38.1.1) In Content Browser, locate: `BP_NarrativeNPCController`
   - 38.1.2) Double-click to open

### 39) Locate Handle Perception Update Function

#### 39.1) Find Function in My Blueprint
   - 39.1.1) In My Blueprint panel, find: **Functions** section
   - 39.1.2) Expand: **Perception** category
   - 39.1.3) Double-click: **Handle Perception Update**

### 40) Modify Perception Flow for Invisibility Check

#### 40.1) Locate Cast to NarrativePlayerCharacter
   - 40.1.1) In Handle Perception Update graph
   - 40.1.2) Find: **Cast To NarrativePlayerCharacter** node
   - 40.1.3) This is after the AISense_Sight check

#### 40.2) Add Invisibility Check After Cast

##### 40.2.1) Get Player's ASC
   - 40.2.1.1) From Cast **As Narrative Player Character** pin:
   - 40.2.1.2) Drag and search: `Get Ability System Component`
   - 40.2.1.3) Add: **Get Ability System Component** node

##### 40.2.2) Check for Invisible Tag
   - 40.2.2.1) Drag from: ASC Return Value
   - 40.2.2.2) Search: `Has Matching Gameplay Tag`
   - 40.2.2.3) Add: **Has Matching Gameplay Tag** node
   - 40.2.2.4) **Tag to Check**: `State.Invisible`

##### 40.2.3) Add Branch Node
   - 40.2.3.1) Disconnect: Current execution wire from Cast success
   - 40.2.3.2) From Cast success execution:
   - 40.2.3.3) Add: **Branch** node
   - 40.2.3.4) Connect: Has Matching Gameplay Tag return to Condition

##### 40.2.4) Configure Branch Paths
   - 40.2.4.1) From Branch **True** (Player IS invisible):
   - 40.2.4.2) Do NOT connect to SetViewedCharacter
   - 40.2.4.3) Leave True path disconnected (skip perception)
   - 40.2.4.4) From Branch **False** (Player is visible):
   - 40.2.4.5) Connect to: Original SetViewedCharacter logic

### 41) Compile and Save NPC Controller

#### 41.1) Finalize
   - 41.1.1) Click: **Compile** button
   - 41.1.2) Click: **Save** button

---

## PHASE 7: GRANT ABILITY VIA EQUIPPABLEITEM

### 42) Open EQI_FatherExoskeleton

#### 42.1) Navigate to Equipment Blueprint
   - 42.1.1) In Content Browser, go to: `/Content/FatherCompanion/Equipment/`
   - 42.1.2) Double-click: **EQI_FatherExoskeleton** to open

### 43) Add GA_StealthField to Abilities Array

#### 43.1) Open Class Defaults
   - 43.1.1) Click: **Class Defaults** button

#### 43.2) Find Abilities Array
   - 43.2.1) In Details panel, search: `Abilities`
   - 43.2.2) Expand: **Abilities** array

#### 43.3) Add New Entry
   - 43.3.1) Click: **+** to add element
   - 43.3.2) Select: `GA_StealthField`

### 44) Compile and Save Equipment Blueprint

#### 44.1) Finalize
   - 44.1.1) Click: **Compile** button
   - 44.1.2) Click: **Save** button

---

## PHASE 8: INPUT CONFIGURATION

### 45) Verify Input Tag Exists

#### 45.1) Check Gameplay Tags
   - 45.1.1) Open: Project Settings -> Gameplay Tags
   - 45.1.2) Verify tag exists: `Narrative.Input.Ability2`
   - 45.1.3) If missing, create it:
      - 45.1.3.1) **Tag Name**: `Narrative.Input.Ability2`
      - 45.1.3.2) **Comment**: `Father Ability2 input binding`

### 46) Configure Narrative Input Settings

#### 46.1) Open Project Settings
   - 46.1.1) Navigate to: **Edit** -> **Project Settings**
   - 46.1.2) In left panel, find: **Narrative Input Settings**

#### 46.2) Find Ability Input Mappings
   - 46.2.1) Locate: **Ability Input Mappings** array

#### 46.3) Add or Verify Mapping
   - 46.3.1) Find entry for Ability2 input action
   - 46.3.2) Verify **Input Tag**: `Narrative.Input.Ability2`
   - 46.3.3) If missing, add new mapping:
      - 46.3.3.1) Click: **+** to add element
      - 46.3.3.2) **Input Action**: Your Ability2 action
      - 46.3.3.3) **Input Tag**: `Narrative.Input.Ability2`

### 47) Save Input Configuration

#### 47.1) Finalize
   - 47.1.1) Click: **Save** in Project Settings
   - 47.1.2) Close: Project Settings window

---

## QUICK REFERENCE

### Tag Configuration Summary

| Tag Type | Tag Name | Purpose |
|----------|----------|---------|
| Ability Tag | Ability.Father.Exoskeleton.StealthField | Identifies this ability |
| Activation Required | Effect.Father.FormState.Exoskeleton | Must be in Exoskeleton form |
| Activation Required | Father.State.Attached | Must be attached to player |
| Activation Required | Father.State.Recruited | Must be recruited |
| Activation Blocked | Cooldown.Father.Exoskeleton.StealthField | Cannot activate during cooldown |
| Activation Blocked | Father.State.Stealthed | Cannot activate while already stealthed |
| Activation Owned | Father.State.Stealthed | Granted while ability active |
| Cancel Tag | State.Attacking | Cancels attack abilities |
| Input Tag | Narrative.Input.Ability2 | Ability2 input binding |

### Gameplay Effect Summary

| Effect | Duration | Purpose |
|--------|----------|---------|
| GE_StealthInvisibility | 8 seconds | Grants invisibility tags + StealthRating 100 |
| GE_StealthDamageBonus | 8 seconds | +50 AttackRating |
| GE_StealthFieldCooldown | 15 seconds | Prevents reactivation |

### Variable Summary

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| PlayerRef | NarrativePlayerCharacter | None | No |
| FatherRef | BP_FatherCompanion | None | No |
| PlayerASC | NarrativeAbilitySystemComponent | None | No |
| PlayerInvisibilityHandle | Active GE Handle | None | No |
| FatherInvisibilityHandle | Active GE Handle | None | No |
| DamageBonusHandle | Active GE Handle | None | No |
| OriginalWalkSpeed | Float | 0.0 | No |
| StealthDuration | Float | 8.0 | Yes |
| SpeedReductionMultiplier | Float | 0.8 | Yes |

### Multiplayer Configuration

| Setting | Value |
|---------|-------|
| Instancing Policy | Instanced Per Actor |
| Net Execution Policy | Local Predicted |
| Replication Policy | Replicate Yes |

### Stealth Activation Flow

| Step | Action | Target |
|------|--------|--------|
| 1 | Validate PlayerRef | - |
| 2 | Get and validate PlayerASC | - |
| 3 | Store original walk speed | Player |
| 4 | Apply speed reduction (0.8x) | Player CharacterMovement |
| 5 | Apply GE_StealthInvisibility | Player ASC |
| 6 | Apply GE_StealthInvisibility | Father ASC |
| 7 | Apply GE_StealthDamageBonus | Player ASC |
| 8 | Clear ViewedCharacter | All NPC Controllers |
| 9 | Start duration timer | 8 seconds |

### Stealth End Flow

| Step | Action | Target |
|------|--------|--------|
| 1 | Restore original walk speed | Player CharacterMovement |
| 2 | Remove invisibility effect | Player ASC |
| 3 | Remove invisibility effect | Father ASC |
| 4 | Remove damage bonus effect | Player ASC |
| 5 | Apply cooldown effect | Owner ASC |
| 6 | Clear all references | - |
| 7 | End ability | - |

### EndAbility Cleanup Flow

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event OnEndAbility | Triggered on any ability end |
| 2 | Is Valid (PlayerASC) | Validate reference exists |
| 3 | Set PlayerRef (None) | Clear player reference |
| 4 | Set FatherRef (None) | Clear father reference |
| 5 | Set PlayerASC (None) | Clear ASC reference |

### AI Perception Integration

| Check Point | Action |
|-------------|--------|
| After AISense_Sight detection | Cast to NarrativePlayerCharacter |
| After successful cast | Get player ASC |
| Check State.Invisible tag | Branch on result |
| If invisible (True) | Skip SetViewedCharacter |
| If visible (False) | Continue to SetViewedCharacter |

### Related Documents

| Document | Version | Purpose |
|----------|---------|---------|
| Father_Companion_Technical_Reference | v4.5 | Cross-actor patterns, validation |
| Father_Companion_System_Setup_Guide | v1.3 | BP_FatherCompanion setup |
| GA_FatherExoskeleton_Implementation_Guide | v3.4 | Parent form ability |
| DefaultGameplayTags_FatherCompanion | v3.5 | Tag definitions |

---

## CHANGELOG

### Version 3.8 - January 2026

| Change | Description |
|--------|-------------|
| Duration Alignment | Changed StealthDuration from 10s to 8s to match manifest.yaml |
| Input Tag Alignment | Changed input_tag from Narrative.Input.Father.Ability3 to Narrative.Input.Ability2 per INV-INPUT-1 compliance |
| GE Duration Update | GE_StealthInvisibility and GE_StealthDamageBonus durations updated to 8s |
| GAS Audit | Updated reference from v6.1 to v6.5 |

### Version 3.6 - January 2026

| Change | Description |
|--------|-------------|
| Locked Decisions Reference | Added Father_Companion_GAS_Abilities_Audit.md reference. This guide complies with Rule 2 (Event_EndAbility required for 8s delay). UE version updated to 5.7. |

### Version 3.5 - January 2026

| Change | Description |
|--------|-------------|
| Narrative Pro | Updated version reference from v2.1 to v2.2 |
| Documentation | Aligned with Technical Reference v5.13 and Setup Guide v2.3 |

### Version 3.4 - January 2026

| Change | Description |
|--------|-------------|
| Tag Configuration | PHASE 3 Section 12.2 - converted to single Property/Tags table |
| Variable Creation | PHASE 3 Section 13 - converted to Variable/Type/Default table |

### Version 3.3 - January 2026

| Change | Description |
|--------|-------------|
| PHASE 1 Simplified | Replaced 6 detailed subsections with consolidated tag tables |
| Tag Creation Format | Tags now listed in Create Required Tags and Verify Existing Tags tables |
| Line Reduction | Reduced PHASE 1 from 74 lines to 24 lines |

### Version 3.2 - December 2025

| Change | Description |
|--------|-------------|
| Activation Required Tags | Added Father.State.Recruited (Element [2]) |
| PlayerASC Variable | Added NarrativeAbilitySystemComponent reference |
| PlayerRef Validation | Added Branch early exit on invalid PlayerRef |
| PlayerASC Validation | Added Cast + Is Valid + Branch for ASC validation |
| Reference Clearing | Added Set PlayerRef/FatherRef/PlayerASC to None before EndAbility |
| Event OnEndAbility | Added Section 36 for early cancellation cleanup |
| Quick Reference | Added EndAbility Cleanup Flow table |
| Related Documents | Updated to v4.5/v1.3/v3.5 versions |

### Version 3.1 - December 2025

| Change | Description |
|--------|-------------|
| Ability Tag | Changed Ability.Father.StealthField to Ability.Father.Exoskeleton.StealthField |
| Cooldown Tag | Changed Cooldown.Father.StealthField to Cooldown.Father.Exoskeleton.StealthField |
| Tag Hierarchy | Enables automatic cancellation when Exoskeleton form ends |

### Version 3.0 - Current Release

| Change | Description |
|--------|-------------|
| Input Tag Correction | Changed from Ability2 to Ability3 per Design Document |
| Added TABLE OF CONTENTS | Standard section added |
| Added PREREQUISITES | Standalone section with required assets |
| Granting Method Changed | Now grants via EQI_FatherExoskeleton instead of BP_FatherCompanion |
| Variable Types Updated | PlayerRef now NarrativePlayerCharacter, FatherRef now BP_FatherCompanion |
| UTF-8 Characters Removed | All arrows converted to ASCII (->) |
| Code Block Diagrams Removed | Converted flow diagrams to markdown tables |
| Section Count Reduced | From 22 sections to 13 sections |
| Instance Editable Added | StealthDuration and SpeedReductionMultiplier now designer-friendly |
| Player Cast Added | Added explicit cast to NarrativePlayerCharacter for type safety |

### Version 2.1 - Previous Release

| Change | Description |
|--------|-------------|
| Attached Father Invisibility | Father now becomes invisible with player |
| AI Perception Integration | Handle Perception Update invisibility check |
| ViewedCharacter Clearing | NPCs forget player when stealth activates |
| Dual Effect Tracking | Separate handles for player and father effects |
| Break Stealth Updates | Removes invisibility from both characters |
| Backstab Synergy | Documented connection to GA_Backstab |

### Version 2.0 - Initial Dual Invisibility

| Change | Description |
|--------|-------------|
| Initial implementation | Player-only stealth with speed reduction |
| Cooldown system | 15 second cooldown via gameplay effect |
| Break conditions | Attack and damage break stealth |
