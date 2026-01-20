# GA_FatherArmor - Armor Form Ability Implementation Guide
## VERSION 4.6 - GAS Audit Compliant (INV-1)
## For Unreal Engine 5.7 + Narrative Pro Plugin v2.2

**Version:** 4.6
**Date:** January 2026
**Engine:** Unreal Engine 5.7
**Plugin:** Narrative Pro v2.2
**Implementation:** Blueprint Only
**Parent Class:** NarrativeGameplayAbility
**Architecture:** Option B (GE-Based Form Identity) - See Form_State_Architecture_Audit_v1_0.md

---

## **AUTOMATION VS MANUAL**

| Feature | Automation Status | Notes |
|---------|-------------------|-------|
| GE_ArmorState definition | ✅ Auto-generated | manifest.yaml gameplay_effects section |
| GA_FatherArmor blueprint | ✅ Auto-generated | manifest.yaml gameplay_abilities section |
| Activation tags config | ✅ Auto-generated | Required/Blocked tags in manifest |
| Transition prelude nodes | ⚠️ Manual | Remove old state GE, apply new state GE |
| Attachment logic | ⚠️ Manual | AttachActorToComponent nodes |
| VFX spawning | ⚠️ Manual | GameplayCues preferred (Category C roadmap) |
| EndAbility cleanup | ⚠️ Manual | Speed restore, state reset |

---

## **DOCUMENT PURPOSE**

This guide provides step-by-step instructions for implementing GA_FatherArmor, the ability that transforms the father companion into Armor form. The father attaches to the player's chest. Stat bonuses are handled by the EquippableItem system via GE_EquipmentModifier_FatherArmor.

**Key Features:**
- Father attaches to player's chest socket
- Armor boost: +50 armor (via EquippableItem and GE_EquipmentModifier_FatherArmor)
- Movement penalty: -15% speed while attached
- Automatic stacking with existing armor via GAS
- **Form identity via GE_ArmorState** (Infinite-duration GE grants Effect.Father.FormState.Armor - **NO invulnerability** per INV-1)
- Mutual exclusivity via Cancel Abilities With Tag
- Animation-driven attachment sequence
- **Transition prelude removes prior form state before applying new** (Option B)
- 5-second transition animation with VFX
- 15-second shared form cooldown
- Full multiplayer compatibility via server authority

---

## **PREREQUISITES**

Before implementing GA_FatherArmor, ensure the following are complete:

1. BP_FatherCompanion must be created and configured
2. NarrativeNPCCharacter base class must be set for father
3. Ability System Component must be initialized automatically
4. Player character must have Narrative Ability System Component
5. NarrativeAttributeSetBase must be assigned to player
6. Enhanced Input System must be enabled in project settings
7. Narrative Pro v2.2 plugin must be installed and enabled
8. E_FatherForm enum must include: Crawler, Armor, Exoskeleton, Symbiote, Engineer
9. **GE_ArmorState Gameplay Effect must exist** (Infinite duration, grants Effect.Father.FormState.Armor - **NO invulnerability** per INV-1)
10. **Father.State.Transitioning tag** must exist (added via AddLooseGameplayTag during transition - NO invulnerability per GAS Audit INV-1)
11. GE_FormChangeCooldown Gameplay Effect must exist (15s duration, grants Cooldown.Father.FormChange)
13. NS_FatherFormTransition Niagara System must be created (5s transition VFX)
14. AC_FatherCompanion_Default must be configured (see Father_Companion_System_Setup_Guide_v2_3.md Phase 24)
15. FatherChestSocket must exist on player skeletal mesh (see Father_Companion_System_Setup_Guide_v2_3.md Phase 23)
16. Father input system must be configured (see Father_Companion_System_Setup_Guide_v2_3.md or use existing Narrative Pro input)

---

## **PHASE 1: GAMEPLAY TAGS SETUP**

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Armor | Father armor form ability - chest attachment with defense boost |
| Father.State.Attached | Father is attached to player |
| Effect.Father.ArmorBoost | Father armor form defense boost effect |
| Effect.Father.FormState.Armor | Armor form identity tag (granted by GE_ArmorState) |
| Father.Buff.Armor | Player has armor buff from father active |

### **Verify Existing Tags**

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Crawler | Default following form ability |
| Ability.Father.Exoskeleton | Back attachment form ability |
| Ability.Father.Symbiote | Full body merge form ability |
| Ability.Father.Engineer | Turret deployment form ability |

**Note (v4.5):** `Father.Form.*` tags are **orphan tags** - no persistent GE grants them. Form identity uses `Effect.Father.FormState.*` tags granted by infinite-duration GE_*State effects. Do NOT use Father.Form.* for activation gating.

---

## **PHASE 2: CREATE GA_FATHERARMOR ABILITY BLUEPRINT**

### **1) Navigate to Abilities Folder**

#### 1.1) Open Content Browser
   - 1.1.1) Locate **Content Browser** panel at bottom of editor
   - 1.1.2) If not visible, open via **Window > Content Browser 1** menu
   - 1.1.3) Content Browser shows project file hierarchy

#### 1.2) Create Folder Structure
   - 1.2.1) In Content Browser, navigate to `/Content/` root folder
   - 1.2.2) Look for `FatherCompanion` folder
   - 1.2.3) If `FatherCompanion` folder missing:
   - 1.2.3.1) Right-click in Content folder empty space
   - 1.2.3.2) Select **New Folder** from context menu
   - 1.2.3.3) Type folder name: `FatherCompanion`
   - 1.2.3.4) Press **Enter** to confirm
   - 1.2.4) Double-click `FatherCompanion` folder to open
   - 1.2.5) Create `GAS` subfolder inside FatherCompanion
   - 1.2.6) Inside GAS folder, create `Abilities` subfolder
   - 1.2.7) Final path: `/Content/FatherCompanion/GAS/Abilities/`

### **2) Create Ability Blueprint**

#### 2.1) Initiate Blueprint Creation
   - 2.1.1) In `/Content/FatherCompanion/GAS/Abilities/` folder
   - 2.1.2) Right-click in empty space
   - 2.1.3) Context menu appears
   - 2.1.4) Select **Blueprint Class** option
   - 2.1.5) **Pick Parent Class** window opens

#### 2.2) Select Parent Class

##### 2.2.1) Open Full Class List
   - 2.2.1.1) In Pick Parent Class window, **Common Classes** section shows
   - 2.2.1.2) NarrativeGameplayAbility not in common list
   - 2.2.1.3) Click **All Classes** button at bottom of window
   - 2.2.1.4) Window expands to show complete class list
   - 2.2.1.5) Search box appears at top of class list

##### 2.2.2) Find NarrativeGameplayAbility Class
   - 2.2.2.1) Click in search box at top
   - 2.2.2.2) Type: `NarrativeGameplayAbility`
   - 2.2.2.3) Class list filters to show matching results
   - 2.2.2.4) **NarrativeGameplayAbility** appears in filtered list
   - 2.2.2.5) Click on **NarrativeGameplayAbility** to select
   - 2.2.2.6) Class highlights with blue selection
   - 2.2.2.7) Click **Select** button at bottom right

##### 2.2.3) Name the Blueprint
   - 2.2.3.1) New blueprint asset created in folder
   - 2.2.3.2) Asset name field automatically selected
   - 2.2.3.3) Type blueprint name: `GA_FatherArmor`
   - 2.2.3.4) Press **Enter** to confirm name
   - 2.2.3.5) Blueprint asset shows GA_ prefix in icon

### **3) Open Ability Blueprint**

#### 3.1) Open Blueprint Editor
   - 3.1.1) Double-click on `GA_FatherArmor` blueprint asset
   - 3.1.2) Blueprint editor opens in new tab
   - 3.1.3) Tab shows blueprint name: GA_FatherArmor
   - 3.1.4) **Event Graph** tab active by default

#### 3.2) Verify Blueprint Setup
   - 3.2.1) In toolbar, verify blueprint name shows: GA_FatherArmor
   - 3.2.2) Below toolbar, parent class shows: NarrativeGameplayAbility
   - 3.2.3) **My Blueprint** panel visible on left side
   - 3.2.4) **Details** panel visible on right side
   - 3.2.5) Graph canvas in center area

---

## **PHASE 3: CONFIGURE ABILITY PROPERTIES**

### **1) Access Class Defaults**

#### 1.1) Open Class Defaults Panel
   - 1.1.1) In Blueprint editor toolbar, locate **Class Defaults** button
   - 1.1.2) Button shows gear icon with text "Class Defaults"
   - 1.1.3) Click **Class Defaults** button
   - 1.1.4) Details panel updates to show class default properties
   - 1.1.5) Properties organized into categories

### **2) Configure Instancing Policy**

#### 2.1) Set Instancing Policy
   - 2.1.1) In Details panel, find **Ability** category
   - 2.1.2) Click arrow to expand category if collapsed
   - 2.1.3) Locate **Instancing Policy** dropdown property
   - 2.1.4) Click dropdown arrow
   - 2.1.5) Dropdown shows options:
   - 2.1.5.a) Non Instanced
   - 2.1.5.b) Instanced Per Actor
   - 2.1.5.c) Instanced Per Execution
   - 2.1.6) Select **Instanced Per Actor** option
   - 2.1.7) This ensures one ability instance per father

### **3) Configure Replication Policy**

#### 3.1) Set Replication Policy
   - 3.1.1) In Details panel, under **Ability** category
   - 3.1.2) Find **Replication Policy** dropdown property
   - 3.1.3) Click dropdown arrow
   - 3.1.4) Dropdown shows options:
   - 3.1.4.a) Replicate No
   - 3.1.4.b) Replicate Yes
   - 3.1.5) Select **Replicate Yes** option
   - 3.1.6) This enables multiplayer synchronization

### **4) Configure Net Execution Policy**

#### 4.1) Set Net Execution Policy
   - 4.1.1) In Details panel, under **Ability** category
   - 4.1.2) Find **Net Execution Policy** dropdown property
   - 4.1.3) Click dropdown arrow
   - 4.1.4) Dropdown shows options:
   - 4.1.4.a) Local Predicted
   - 4.1.4.b) Local Only
   - 4.1.4.c) Server Initiated
   - 4.1.4.d) Server Only
   - 4.1.5) Select **Server Only** option
   - 4.1.6) NPC-owned form ability executes on server, clients see replicated results
   - 4.1.7) Required because GA_FatherArmor performs cross-actor attachment to player mesh

### **4A) Configure Cooldown (Built-in System)**

#### 4A.1) Set Cooldown Gameplay Effect Class
   - 4A.1.1) In Details panel, find **Cooldown** section
   - 4A.1.2) Locate **Cooldown Gameplay Effect Class** property
   - 4A.1.3) Click dropdown and select: `GE_FormChangeCooldown`
   - 4A.1.4) GAS automatically blocks activation when cooldown tag is present

### **5) Configure Ability Tags (Class Defaults)**

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Armor |
| Cancel Abilities with Tag | Ability.Father.Crawler, Ability.Father.Exoskeleton, Ability.Father.Symbiote, Ability.Father.Engineer |
| Activation Owned Tags | Father.State.Attached |
| Activation Required Tags | Father.State.Alive, Father.State.Recruited |
| Activation Blocked Tags | Father.State.Dormant, Father.State.Transitioning, Father.State.SymbioteLocked, Cooldown.Father.FormChange |

**Note (v4.5):** Father.Form.Armor removed from tags - form identity persists via GE_ArmorState which grants `Effect.Father.FormState.Armor` (Option B architecture). See Form_State_Architecture_Audit_v1_0.md.

### **6) Configure Input Tag**

#### 6.1) Set Input Tag Property
   - 6.1.1) In Details panel, find **Narrative Ability** category
   - 6.1.2) Click arrow to expand category if collapsed
   - 6.1.3) Locate **Input Tag** property
   - 6.1.4) Property shows gameplay tag selector
   - 6.1.5) Click dropdown arrow next to Input Tag
   - 6.1.6) **Select Gameplay Tag** window opens
   - 6.1.7) Navigate to: `Narrative > Input > Father > FormChange`
   - 6.1.8) Select `Narrative.Input.Father.FormChange` tag
   - 6.1.9) Click **Select** button
   - 6.1.10) Tag appears in Input Tag property

### **7) Compile and Save**
   - 7.1) Click **Compile** button in toolbar
   - 7.2) Wait for compilation to complete
   - 7.3) Verify no errors in message log
   - 7.4) Click **Save** button in toolbar
   - 7.5) Blueprint properties now configured
---

## **PHASE 4: CREATE ABILITY VARIABLES**

### **Variable Definitions**

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| OriginalMaxWalkSpeed | Float | 0.0 | No |
| SpeedPenaltyMultiplier | Float | 0.85 | Yes |
| ChestSocketName | Name | FatherChestSocket | Yes |
| bIsFirstActivation | Boolean | True | No |

### **Compile Blueprint**
   - Click **Compile** button in toolbar
   - Verify no errors
   - Variables now available for use in graph

---

## **PHASE 5: IMPLEMENT ABILITY LOGIC**

### **1) Navigate to Event Graph**

#### 1.1) Switch to Event Graph
   - 1.1.1) At top of graph canvas, locate tab bar
   - 1.1.2) Find **Event Graph** tab
   - 1.1.3) Click **Event Graph** tab to switch view
   - 1.1.4) Graph canvas shows empty grid

### **2) Add Event ActivateAbility Node**

#### 2.1) Create Activation Event

##### 2.1.1) Add Event Node
   - 2.1.1.1) Right-click on empty graph canvas
   - 2.1.1.2) Context menu appears showing available nodes
   - 2.1.1.3) In search box at top, type: `Event ActivateAbility`
   - 2.1.1.4) List filters to show matching events
   - 2.1.1.5) Click on **Event ActivateAbility** option
   - 2.1.1.6) Red event node appears on graph at cursor location

##### 2.1.2) Position Event Node
   - 2.1.2.1) Click and drag event node to position at left side of graph
   - 2.1.2.2) Place near top-left corner for logical flow
   - 2.1.2.3) Release mouse to drop node in position

##### 2.1.3) Verify Event Node Structure
   - 2.1.3.1) Node shows red header with event name
   - 2.1.3.2) White execution pin on right side of node
   - 2.1.3.3) No input pins (this is event trigger)

### **3) Get Father Actor Reference**

#### 3.1) Add Get Avatar Actor From Actor Info Node

##### 3.1.1) Create Get Avatar Node
   - 3.1.1.1) From Event ActivateAbility white execution pin
   - 3.1.1.2) Drag connection wire to right
   - 3.1.1.3) Release mouse to open context menu
   - 3.1.1.4) In search box, type: `Get Avatar Actor From Actor Info`
   - 3.1.1.5) Click on node in results list
   - 3.1.1.6) Blue pure function node appears

##### 3.1.2) Verify Node Placement
   - 3.1.2.1) Node positioned to right of Event node
   - 3.1.2.2) Has blue output pin labeled **Return Value**
   - 3.1.2.3) No input or execution pins (pure function provides data only)
   - 3.1.2.4) Connection wire automatically disappears (pure function provides data only)

#### 3.2) Cast to BP_FatherCompanion

##### 3.2.1) Add Cast Node
   - 3.2.1.1) From Event ActivateAbility execution pin (white pin)
   - 3.2.1.2) Drag connection wire to right
   - 3.2.1.3) Release to open context menu
   - 3.2.1.4) In search box, type: `Cast To BP_FatherCompanion`
   - 3.2.1.5) Click on cast node in results
   - 3.2.1.6) Blue cast node appears with execution pins

##### 3.2.2) Connect Object Pin
   - 3.2.2.1) Locate **Object** input pin on Cast node (blue pin)
   - 3.2.2.2) From Get Avatar Actor node **Return Value** pin
   - 3.2.2.3) Drag wire to Cast node **Object** pin
   - 3.2.2.4) Release to connect
   - 3.2.2.5) Wire appears connecting data pins

##### 3.2.3) Verify Cast Node Structure
   - 3.2.3.1) White execution input pin on left
   - 3.2.3.2) Blue **Object** input pin (connected to Get Avatar)
   - 3.2.3.3) White execution output pin on top right (success path)
   - 3.2.3.4) White execution output pin on bottom right (failure path)
   - 3.2.3.5) Blue **As BP Father Companion** output pin (cast result)

### **4) Branch: Check If First Activation**

#### 4.1) Add Branch Node

##### 4.1.1) Create Branch Node
   - 4.1.1.1) From Cast node success execution pin (white pin on top right)
   - 4.1.1.2) Drag connection wire to right
   - 4.1.1.3) Release to open context menu
   - 4.1.1.4) In search box, type: `Branch`
   - 4.1.1.5) Select **Flow Control > Branch** node
   - 4.1.1.6) Branch node appears with True/False execution pins

##### 4.1.2) Connect Condition
   - 4.1.2.1) Drag **bIsFirstActivation** variable into graph (GET)
   - 4.1.2.2) Connect **bIsFirstActivation** output to Branch **Condition** pin

#### 4.2) Configure Branch Paths
   - 4.2.1) **True** path: Continues to Section 5 (Initial Activation - runs once on spawn)
   - 4.2.2) **False** path: Continues to PHASE 5A (Form Switch with Transition Animation)

### **5) Get Owner Player Reference**

#### 5.1) Add Get Owner Player Node

##### 5.1.1) Create Get Owner Node
   - 5.1.1.1) From Branch node **True** execution pin
   - 5.1.1.2) Drag connection wire to right
   - 5.1.1.3) Release to open context menu
   - 5.1.1.4) In search box, type: `Get Owner Player`
   - 5.1.1.5) Ensure node shows **Target: BP_FatherCompanion** context
   - 5.1.1.6) Click on **Get Owner Player** node in results
   - 5.1.1.7) Blue function node appears

##### 5.1.2) Connect Target Pin
   - 5.1.2.1) Locate **Target** input pin on Get Owner Player node
   - 5.1.2.2) From Cast node **As BP Father Companion** output pin (blue)
   - 5.1.2.3) Drag wire to Get Owner Player **Target** pin
   - 5.1.2.4) Release to connect
   - 5.1.2.5) Wire connects cast output to function target

##### 5.1.3) Verify Get Owner Player Structure
   - 5.1.3.1) White execution input pin on left (connected to Branch True)
   - 5.1.3.2) Blue **Target** input pin (connected to cast result)
   - 5.1.3.3) White execution output pin on right
   - 5.1.3.4) Blue **Return Value** output pin (owner player actor)

#### 5.2) Add Is Valid Check

##### 5.2.1) Create Is Valid Node
   - 5.2.1.1) From Get Owner Player execution output pin
   - 5.2.1.2) Drag connection wire to right
   - 5.2.1.3) Release to open context menu
   - 5.2.1.4) In search box, type: `Is Valid`
   - 5.2.1.5) Select **Utilities > Flow Control > Is Valid**
   - 5.2.1.6) Macro node appears with multiple pins

##### 5.2.2) Connect Input Object Pin
   - 5.2.2.1) Locate **Input Object** pin on Is Valid node (blue pin)
   - 5.2.2.2) From Get Owner Player **Return Value** pin (blue)
   - 5.2.2.3) Drag wire to Is Valid **Input Object** pin
   - 5.2.2.4) Release to connect

##### 5.2.3) Verify Is Valid Structure
   - 5.2.3.1) White execution input pin on left
   - 5.2.3.2) Blue **Input Object** pin (connected to owner player)
   - 5.2.3.3) White **Is Valid** execution output pin (valid path)
   - 5.2.3.4) White **Is Not Valid** execution output pin (invalid path)

### **6) State Tags Via Activation Owned Tags**

#### 5.1) State Tags Automatically Applied
   - 5.1.1) State tag (Father.State.Attached) is in Activation Owned Tags
   - 5.1.2) Tag automatically granted when ability activates
   - 5.1.3) Tag automatically removed when ability ends
   - 5.1.4) Multiplayer replication enabled via ReplicateActivationOwnedTags setting
   - 5.1.5) Stat bonuses handled by BP_FatherArmorForm EquippableItem via GE_EquipmentModifier_FatherArmor
   - 5.1.6) **Form identity** persists via GE_ArmorState (grants Effect.Father.FormState.Armor) - NOT Activation Owned Tags

### **7) Store Original Movement Speed**

#### 6.1) Get Character Movement Component

##### 6.1.1) Create Get Character Movement Node
   - 6.1.1.1) From Is Valid **Is Valid** execution output pin
   - 6.1.1.2) Drag connection wire to right
   - 6.1.1.3) Release to open context menu
   - 6.1.1.4) In search box, type: `Get Character Movement`
   - 6.1.1.5) Click on **Get Character Movement** node in results
   - 6.1.1.6) Blue function node appears

##### 6.1.2) Connect Target Pin
   - 6.1.2.1) Locate **Target** input pin on Get Character Movement node
   - 6.1.2.2) From Get Owner Player **Return Value** pin
   - 6.1.2.3) Drag wire to Get Character Movement **Target** pin
   - 6.1.2.4) Release to connect

##### 6.1.3) Verify Node Structure
   - 6.1.3.1) White execution input pin on left
   - 6.1.3.2) Blue **Target** input pin (connected to owner player)
   - 6.1.3.3) White execution output pin on right
   - 6.1.3.4) Blue **Return Value** output pin (Character Movement Component)

#### 6.2) Get Current Max Walk Speed

##### 6.2.1) Create Get Max Walk Speed Node
   - 6.2.1.1) From Get Character Movement **Return Value** pin (blue)
   - 6.2.1.2) Drag wire to empty space on right
   - 6.2.1.3) Release to open context menu
   - 6.2.1.4) In search box, type: `Get Max Walk Speed`
   - 6.2.1.5) Click on **Get Max Walk Speed** pure function
   - 6.2.1.6) Blue pure function node appears

##### 6.2.2) Verify Node Connection
   - 6.2.2.1) Node has **Target** pin auto-connected to Character Movement
   - 6.2.2.2) **Return Value** pin outputs float (current max walk speed)

#### 6.3) Store Original Speed in Variable

##### 6.3.1) Add Set Variable Node
   - 6.3.1.1) In **My Blueprint** panel, find **OriginalMaxWalkSpeed** variable
   - 6.3.1.2) Hold **Ctrl** key and drag variable to graph
   - 6.3.1.3) Setter node appears (SET OriginalMaxWalkSpeed)
   - 6.3.1.4) Position to right of Get Character Movement

##### 6.3.2) Connect Setter Node
   - 6.3.2.1) From Get Character Movement execution pin
   - 6.3.2.2) Drag wire to SET OriginalMaxWalkSpeed execution input
   - 6.3.2.3) Release to connect execution flow
   - 6.3.2.4) From Get Max Walk Speed **Return Value** pin
   - 6.3.2.5) Drag wire to SET OriginalMaxWalkSpeed input pin
   - 6.3.2.6) Release to connect data flow

##### 6.3.3) Verify Setter Node Structure
   - 6.3.3.1) White execution input pin (connected to Get Character Movement)
   - 6.3.3.2) Green **OriginalMaxWalkSpeed** input pin (connected to Get Max Walk Speed)
   - 6.3.3.3) White execution output pin on right
   - 6.3.3.4) Green output pin (outputs stored value)

### **8) Apply Movement Speed Penalty**

#### 7.1) Calculate Reduced Speed

##### 7.1.1) Get Original Speed Variable
   - 7.1.1.1) In **My Blueprint** panel, find **OriginalMaxWalkSpeed** variable
   - 7.1.1.2) Drag variable to graph (without Ctrl for getter)
   - 7.1.1.3) Getter node appears (GET OriginalMaxWalkSpeed)
   - 7.1.1.4) Position below SET node

##### 7.1.2) Create Multiply Node
   - 7.1.2.1) From GET OriginalMaxWalkSpeed output pin (green)
   - 7.1.2.2) Drag wire to empty space on right
   - 7.1.2.3) Release to open context menu
   - 7.1.2.4) In search box, type: `* float`
   - 7.1.2.5) Select **float * float** (Multiply)
   - 7.1.2.6) Multiply node appears
   - 7.1.2.7) First input auto-connected to OriginalMaxWalkSpeed

##### 7.1.3) Get Penalty Multiplier Variable
   - 7.1.3.1) In **My Blueprint** panel, find **SpeedPenaltyMultiplier** variable
   - 7.1.3.2) Drag variable to graph (getter)
   - 7.1.3.3) Position near multiply node

##### 7.1.4) Connect Multiplier
   - 7.1.4.1) From GET SpeedPenaltyMultiplier output pin
   - 7.1.4.2) Drag wire to multiply node second input pin
   - 7.1.4.3) Release to connect
   - 7.1.4.4) Multiply node now calculates: OriginalMaxWalkSpeed * 0.85

#### 7.2) Set New Walk Speed

##### 7.2.1) Create Set Max Walk Speed Node
   - 7.2.1.1) From SET OriginalMaxWalkSpeed execution output pin
   - 7.2.1.2) Drag connection wire to right
   - 7.2.1.3) Release to open context menu
   - 7.2.1.4) In search box, type: `Set Max Walk Speed`
   - 7.2.1.5) Click on **Set Max Walk Speed** function
   - 7.2.1.6) Blue function node appears

##### 7.2.2) Connect Target Pin
   - 7.2.2.1) From Get Character Movement **Return Value** pin
   - 7.2.2.2) Drag wire to Set Max Walk Speed **Target** pin
   - 7.2.2.3) Release to connect

##### 7.2.3) Connect New Speed Value
   - 7.2.3.1) From multiply node output pin (result)
   - 7.2.3.2) Drag wire to Set Max Walk Speed **Max Walk Speed** input pin
   - 7.2.3.3) Release to connect
   - 7.2.3.4) New speed value now connected (85% of original)

##### 7.2.4) Verify Set Max Walk Speed Structure
   - 7.2.4.1) White execution input pin (connected to SET OriginalMaxWalkSpeed)
   - 7.2.4.2) Blue **Target** pin (connected to Character Movement)
   - 7.2.4.3) Green **Max Walk Speed** pin (connected to multiply result)
   - 7.2.4.4) White execution output pin on right

### **9) Play Jump Animation**

#### 8.1) Add Play Montage Node

##### 8.1.1) Create Play Montage Node
   - 8.1.1.1) From Set Max Walk Speed execution output pin
   - 8.1.1.2) Drag connection wire to right
   - 8.1.1.3) Release to open context menu
   - 8.1.1.4) In search box, type: `Play Montage and Wait`
   - 8.1.1.5) Select **Play Montage and Wait** task node
   - 8.1.1.6) Async task node appears with multiple output pins

##### 8.1.2) Configure Montage Property
   - 8.1.2.1) In Play Montage node, find **Montage to Play** dropdown
   - 8.1.2.2) Click dropdown arrow
   - 8.1.2.3) Select `AM_FatherArmor` from list
   - 8.1.2.4) If montage not yet created:
   - 8.1.2.4.a) Leave as None for now
   - 8.1.2.4.b) Can be set later when animation ready

##### 8.1.3) Verify Play Montage Structure
   - 8.1.3.1) White execution input pin
   - 8.1.3.2) **Montage to Play** property (set to your montage)
   - 8.1.3.3) AbilityTask automatically uses ability owner (father) as montage target
   - 8.1.3.4) Multiple white execution output pins:
   - 8.1.3.4.a) **On Completed** pin (montage finished)
   - 8.1.3.4.b) **On Blend Out** pin
   - 8.1.3.4.c) **On Interrupted** pin
   - 8.1.3.4.d) **On Notify Begin** pin
   - 8.1.3.4.e) **On Notify End** pin

### **10) Attach Father to Player**

#### 9.1) Add Attach Actor To Component Node

##### 9.1.1) Create Attach Node
   - 9.1.1.1) From Play Montage **On Completed** execution pin
   - 9.1.1.2) Drag connection wire to right
   - 9.1.1.3) Release to open context menu
   - 9.1.1.4) In search box, type: `Attach Actor To Component`
   - 9.1.1.5) Click on node in results list
   - 9.1.1.6) Blue function node appears

##### 9.1.2) Connect Target Pin
   - 9.1.2.1) Locate **Target** input pin on Attach node
   - 9.1.2.2) From Cast node **As BP Father Companion** output pin
   - 9.1.2.3) Drag wire to Attach node **Target** pin
   - 9.1.2.4) Release to connect

#### 9.2) Get Player Mesh Component

##### 9.2.1) Add Get Mesh Node
   - 9.2.1.1) From Get Owner Player **Return Value** pin (blue data pin)
   - 9.2.1.2) Drag wire to empty space on right
   - 9.2.1.3) Release to open context menu
   - 9.2.1.4) In search box, type: `Get Mesh`
   - 9.2.1.5) Select **Get Mesh** function
   - 9.2.1.6) Pure function node appears (no execution pins)

##### 9.2.2) Connect Mesh to Parent Pin
   - 9.2.2.1) From Get Mesh **Return Value** pin (blue)
   - 9.2.2.2) Drag wire to Attach node **Parent** input pin
   - 9.2.2.3) Release to connect
   - 9.2.2.4) Wire connects mesh component to attachment parent

#### 9.3) Configure Socket Name

##### 9.3.1) Get Socket Name Variable
   - 9.3.1.1) In **My Blueprint** panel, find **ChestSocketName** variable
   - 9.3.1.2) Drag variable to graph (getter)
   - 9.3.1.3) Position near Attach node

##### 9.3.2) Connect Socket Name
   - 9.3.2.1) From GET ChestSocketName output pin
   - 9.3.2.2) Drag wire to Attach node **Socket Name** input pin
   - 9.3.2.3) Release to connect

#### 9.4) Configure Attachment Rules

##### 9.4.1) Set Location Rule
   - 9.4.1.1) On Attach node, find **Location Rule** dropdown
   - 9.4.1.2) Click dropdown arrow
   - 9.4.1.3) Select **Snap to Target** option
   - 9.4.1.4) This makes father snap to socket position

##### 9.4.2) Set Rotation Rule
   - 9.4.2.1) Find **Rotation Rule** dropdown
   - 9.4.2.2) Click dropdown arrow
   - 9.4.2.3) Select **Snap to Target** option
   - 9.4.2.4) This makes father match socket rotation

##### 9.4.3) Set Scale Rule
   - 9.4.3.1) Find **Scale Rule** dropdown
   - 9.4.3.2) Click dropdown arrow
   - 9.4.3.3) Select **Keep World** option
   - 9.4.3.4) This preserves father's original scale

##### 9.4.4) Configure Weld Simulated Bodies
   - 9.4.4.1) Find **Weld Simulated Bodies** checkbox
   - 9.4.4.2) Leave unchecked (default)

### **11) Update Father State Variables**

#### 10.1) Add Set Current Form Node

##### 10.1.1) Create Setter Node
   - 10.1.1.1) From Attach Actor execution output pin
   - 10.1.1.2) Drag connection wire to right
   - 10.1.1.3) Release to open context menu
   - 10.1.1.4) In search box, type: `Set Current Form`
   - 10.1.1.5) Ensure context shows BP_FatherCompanion
   - 10.1.1.6) Click on **Set Current Form** setter node
   - 10.1.1.7) Blue function node appears

##### 10.1.2) Connect Target Pin
   - 10.1.2.1) Locate **Target** input pin on Set Current Form node
   - 10.1.2.2) From Cast node **As BP Father Companion** output pin
   - 10.1.2.3) Drag wire to setter **Target** pin
   - 10.1.2.4) Release to connect

##### 10.1.3) Set Form Value
   - 10.1.3.1) On Set Current Form node, find **Current Form** dropdown
   - 10.1.3.2) Click dropdown arrow
   - 10.1.3.3) Dropdown shows enum values:
   - 10.1.3.3.a) Crawler
   - 10.1.3.3.b) Armor
   - 10.1.3.3.c) Exoskeleton
   - 10.1.3.3.d) Symbiote
   - 10.1.3.3.e) Engineer
   - 10.1.3.4) Select **Armor** option
   - 10.1.3.5) Dropdown closes with value set

#### 10.2) Add Set Is Attached Node

##### 10.2.1) Create Is Attached Setter
   - 10.2.1.1) From Set Current Form execution output pin
   - 10.2.1.2) Drag connection wire to right
   - 10.2.1.3) Release to open context menu
   - 10.2.1.4) In search box, type: `Set Is Attached`
   - 10.2.1.5) Ensure context shows BP_FatherCompanion
   - 10.2.1.6) Click on **Set Is Attached** setter node
   - 10.2.1.7) Blue function node appears

##### 10.2.2) Connect Target Pin
   - 10.2.2.1) Locate **Target** input pin on Set Is Attached node
   - 10.2.2.2) From Cast node **As BP Father Companion** output pin
   - 10.2.2.3) Drag wire to setter **Target** pin
   - 10.2.2.4) Release to connect

##### 10.2.3) Set Attached Value
   - 10.2.3.1) On Set Is Attached node, find **Is Attached** checkbox
   - 10.2.3.2) Click checkbox to check it
   - 10.2.3.3) Checkbox shows checkmark (true)

### **12) Stat Bonuses and Abilities (Handled by EquippableItem)**

Stat bonuses (+50 Armor) and ability granting (GA_ProtectiveDome, GA_DomeBurst) are handled automatically by BP_FatherArmorForm EquippableItem.

| Component | Handled By |
|-----------|------------|
| +50 Armor | GE_EquipmentModifier_FatherArmor (via SetByCaller.Armor) |
| GA_ProtectiveDome | EquippableItem Abilities array |
| GA_DomeBurst | EquippableItem Abilities array |

GA_FatherArmor only handles physical attachment and movement penalty.

### **13) End Ability**

#### 13.1) Set First Activation Flag to False

##### 13.1.1) Add Set Variable Node
   - 13.1.1.1) From Set Is Attached execution output pin
   - 13.1.1.2) Drag connection wire to right
   - 13.1.1.3) Release to open context menu
   - 13.1.1.4) In search box, type: `Set bIsFirstActivation`
   - 13.1.1.5) Select **Set bIsFirstActivation** node
   - 13.1.1.6) Variable setter node appears

##### 13.1.2) Configure Value
   - 13.1.2.1) On Set bIsFirstActivation node, find checkbox
   - 13.1.2.2) Leave checkbox unchecked (false)
   - 13.1.2.3) Next activation will use Form Switch path (PHASE 5A)

#### 13.2) Add End Ability Node

##### 13.2.1) Create End Ability Node
   - 13.2.1.1) From Set bIsFirstActivation execution output pin
   - 13.2.1.2) Drag connection wire to right
   - 13.2.1.3) Release to open context menu
   - 13.2.1.4) In search box, type: `End Ability`
   - 13.2.1.5) Click on **End Ability** node in results
   - 13.2.1.6) Function node appears

##### 13.2.2) Configure Was Cancelled
   - 13.2.2.1) On End Ability node, find **Was Cancelled** checkbox
   - 13.2.2.2) Leave checkbox unchecked (false)
   - 13.2.2.3) This indicates successful ability completion

### **14) Handle Animation Interrupted**

#### 14.1) Add End Ability for Interruption

##### 14.1.1) Create Second End Ability Node
   - 14.1.1.1) From Play Montage **On Interrupted** execution pin
   - 14.1.1.2) Drag connection wire to right
   - 14.1.1.3) Release to open context menu
   - 14.1.1.4) In search box, type: `End Ability`
   - 14.1.1.5) Click on **End Ability** node
   - 14.1.1.6) Second End Ability node appears

##### 14.1.2) Configure Cancelled State
   - 14.1.2.1) On this End Ability node, find **Was Cancelled** checkbox
   - 14.1.2.2) Click checkbox to check it (true)
   - 14.1.2.3) This indicates ability was interrupted/cancelled

### **15) Compile and Save Blueprint**
   - 15.1) Click **Compile** button in toolbar
   - 15.2) Wait for compilation to complete
   - 15.3) Check bottom panel for any errors or warnings
   - 15.4) If errors appear, review node connections
   - 15.5) Click **Save** button in toolbar
   - 15.6) Blueprint now has complete ability logic implemented

---

## **PHASE 5A: FORM SWITCH WITH TRANSITION ANIMATION (Option B)**

This section covers the extended logic when switching TO Armor from another form via the T wheel. The initial spawn flow (PHASE 5) uses bActivateAbilityOnGranted and skips transition animation.

**CRITICAL (Option B):** The transition prelude MUST:
1. Remove ALL prior form state GEs (Single Active Form State Invariant)
2. Apply GE_ArmorState (grants Effect.Father.FormState.Armor - **NO invulnerability** per INV-1)

### **1) TRANSITION PRELUDE: Remove Prior Form State (Option B)**

#### 1.1) Get Father Ability System Component
   - 1.1.1) From Branch node **False** execution pin
   - 1.1.2) Drag from **As BP Father Companion**
   - 1.1.3) Search: `Get Ability System Component`
   - 1.1.4) Select node

#### 1.2) Remove All Prior Form State GEs
   - 1.2.1) Drag from **Get Ability System Component** -> **Return Value**
   - 1.2.2) Search: `BP_RemoveGameplayEffectFromOwnerWithGrantedTags`
   - 1.2.3) Select node
   - 1.2.4) Connect Father ASC to **Target**
   - 1.2.5) In **Tags** field, add: `Effect.Father.FormState` (parent tag removes ALL form states)
   - 1.2.6) **Purpose:** Enforces Single Active Form State Invariant - exactly one form state at a time

### **2) TRANSITION PRELUDE: Apply New Form State (Option B)**

#### 2.1) Apply GE_ArmorState
   - 2.1.1) Drag from **BP_RemoveGameplayEffectFromOwnerWithGrantedTags** execution pin
   - 2.1.2) Search: `Apply Gameplay Effect to Self`
   - 2.1.3) Select node
   - 2.1.4) Connect Father ASC to **Target**
   - 2.1.5) Set **Gameplay Effect Class**: `GE_ArmorState`
   - 2.1.6) **Result:** Father now has Effect.Father.FormState.Armor tag

### **3) Add Transitioning Tag**

> **v4.5 (GAS Audit INV-1):** Form transitions NO LONGER grant invulnerability.
> Father is vulnerable during the 5s transition. Use AddLooseGameplayTag for the transitioning state.

#### 3.1) Add Loose Gameplay Tag (Father.State.Transitioning)
   - 3.1.1) Drag from **Apply GE_ArmorState** execution pin
   - 3.1.2) Search: `Add Loose Gameplay Tag`
   - 3.1.3) Select node
   - 3.1.4) Connect Father ASC to **Target**
   - 3.1.5) Set **Tag**: `Father.State.Transitioning`
   - 3.1.6) **Note:** This blocks form switch during VFX (no invulnerability)

### **4) Spawn Transition VFX**

#### 4.1) Add Spawn System Attached Node
   - 4.1.1) Drag from **Add Loose Gameplay Tag** execution pin
   - 4.1.2) Search: `Spawn System Attached`
   - 4.1.3) Select node

#### 3.2) Configure VFX Parameters
   - 3.2.1) **System Template**: Select `NS_FatherFormTransition`
   - 3.2.2) **Attach to Component**: Drag from **As BP Father Companion** -> Get **Mesh**
   - 3.2.3) **Attach Point Name**: `root`
   - 3.2.4) **Location Type**: `Snap to Target`

### **4) Wait 5 Seconds**

#### 4.1) Add Delay Node
   - 4.1.1) Drag from **Spawn System Attached** execution pin
   - 4.1.2) Search: `Delay`
   - 4.1.3) Select node
   - 4.1.4) Set **Duration**: `5.0`

### **5) Get Owner Player Reference**

#### 5.1) Get Owner Player
   - 5.1.1) From **Delay** -> **Completed** pin
   - 5.1.2) Drag from **As BP Father Companion**
   - 5.1.3) Search: `Get Owner Player`
   - 5.1.4) Select node

### **6) Store Original Movement Speed**

#### 6.1) Get Character Movement Component
   - 6.1.1) From **Get Owner Player** -> **Return Value**
   - 6.1.2) Search: `Get Character Movement`
   - 6.1.3) Select node

#### 6.2) Get Current Walk Speed
   - 6.2.1) From **Get Character Movement** -> **Return Value**
   - 6.2.2) Search: `Get Max Walk Speed`
   - 6.2.3) Select node

#### 6.3) Store in Variable
   - 6.3.1) Drag **OriginalMaxWalkSpeed** variable into graph (SET)
   - 6.3.2) Connect **Get Max Walk Speed** -> **Return Value** to **OriginalMaxWalkSpeed**

### **7) Apply Movement Speed Penalty**

#### 7.1) Calculate Reduced Speed
   - 7.1.1) Add **Multiply (Float * Float)** node
   - 7.1.2) Connect **OriginalMaxWalkSpeed** to first input
   - 7.1.3) Connect **SpeedPenaltyMultiplier** variable to second input (0.85)

#### 7.2) Set New Walk Speed
   - 7.2.1) From **Get Character Movement** -> **Return Value**
   - 7.2.2) Search: `Set Max Walk Speed`
   - 7.2.3) Connect multiply result to **Max Walk Speed** input

### **8) Attach Father to Player**

#### 8.1) Add Attach Actor To Component Node
   - 8.1.1) From **Set Max Walk Speed** execution pin
   - 8.1.2) Search: `Attach Actor To Component`
   - 8.1.3) Select node
   - 8.1.4) Connect **As BP Father Companion** to **Target**

#### 8.2) Configure Attachment
   - 8.2.1) From **Get Owner Player** -> **Return Value** -> **Get Mesh**
   - 8.2.2) Connect to **Parent** input
   - 8.2.3) Connect **ChestSocketName** variable to **Socket Name**
   - 8.2.4) **Location Rule**: `Snap to Target`
   - 8.2.5) **Rotation Rule**: `Snap to Target`
   - 8.2.6) **Scale Rule**: `Keep World`

### **9) Set Form Variables**

#### 9.1) Set Current Form
   - 9.1.1) Drag from **Attach Actor To Component** execution pin
   - 9.1.2) Add **Set Current Form** node
   - 9.1.3) Connect **As BP Father Companion** to **Target**
   - 9.1.4) Set **Current Form**: `Armor`

#### 9.2) Set Is Attached
   - 9.2.1) Drag from **Set Current Form** execution pin
   - 9.2.2) Add **Set Is Attached** node
   - 9.2.3) Connect **As BP Father Companion** to **Target**
   - 9.2.4) Set **Is Attached**: `True` (checked)

### **10) Stat Bonuses (Handled by EquippableItem)**

Stat bonuses (+50 Armor) are handled automatically by BP_FatherArmorForm EquippableItem via GE_EquipmentModifier_FatherArmor. GA_FatherArmor only handles attachment and movement.

### **11) Remove Transitioning State**

#### 11.1) Remove Loose Gameplay Tag
   - 11.1.1) Drag from **Set Is Attached** execution pin
   - 11.1.2) Search: `Remove Loose Gameplay Tag`
   - 11.1.3) Select node
   - 11.1.4) Connect Father ASC to **Target**
   - 11.1.5) Set **Gameplay Tag**: `Father.State.Transitioning`

### **12) Apply Form Cooldown**

> **v4.5 (GAS Audit INV-1):** No GE removal needed - Father.State.Transitioning tag was removed in Step 11 via RemoveLooseGameplayTag.

#### 12.1) Commit Ability Cooldown
   - 12.1.1) Drag from **Remove Loose Gameplay Tag** execution pin
   - 12.1.2) Search: `Commit Ability Cooldown`
   - 12.1.3) Select **Commit Ability Cooldown** node
   - 12.1.4) No parameters needed - uses CooldownGameplayEffectClass automatically

### **13) End Ability**

#### 13.1) Add End Ability Node
   - 13.1.1) Drag from **Commit Ability Cooldown** execution pin
   - 13.1.2) Search: `End Ability`
   - 13.1.3) Select node
   - 13.1.4) **Was Cancelled**: `False` (unchecked)

### **14) Compile and Save**

#### 14.1) Compile Blueprint
   - 14.1.1) Click **Compile** button

#### 14.2) Save Blueprint
   - 14.2.1) Click **Save** button

---

## **PHASE 6: STAT BONUSES (HANDLED BY EQUIPPABLEITEM)**

Armor stat bonus (+50 Armor) is handled automatically by the EquippableItem system:

| Component | Responsibility |
|-----------|----------------|
| BP_FatherArmorForm | EquippableItem with Armor Rating: 50.0 |
| GE_EquipmentModifier_FatherArmor | Child GE that applies SetByCaller.Armor value |
| HandleEquip | Parent method applies stats when form equipped |

GA_FatherArmor does NOT need to apply stat modifiers. The EquippableItem's HandleEquip method:
1. Calls Parent: HandleEquip which applies GE_EquipmentModifier_FatherArmor
2. GE_EquipmentModifier_FatherArmor receives Armor Rating (50.0) via SetByCaller.Armor
3. Stats are automatically removed when HandleUnequip is called

See Father_Companion_System_Setup_Guide PHASE 30 and PHASE 31 for GE and EquippableItem configuration.

---

## **PHASE 7: IMPLEMENT ENDABILITY EVENT FOR SPEED RESTORATION**

### **1) Open GA_FatherArmor Blueprint**

#### 1.1) Navigate to Ability
   - 1.1.1) In Content Browser, go to `/Content/FatherCompanion/GAS/Abilities/`
   - 1.1.2) Double-click **GA_FatherArmor** to open

### **2) Add Event EndAbility Node**

#### 2.1) Create EndAbility Event

##### 2.1.1) Add Event Node
   - 2.1.1.1) Right-click in Event Graph empty space (below existing logic)
   - 2.1.1.2) In search box, type: `Event EndAbility`
   - 2.1.1.3) Click on **Event EndAbility** in results
   - 2.1.1.4) Red event node appears on graph

##### 2.1.2) Position Node
   - 2.1.2.1) Drag node to position below activation logic
   - 2.1.2.2) Leave space for cleanup nodes to the right

##### 2.1.3) Verify Event Structure
   - 2.1.3.1) Node shows red header: Event EndAbility
   - 2.1.3.2) White execution output pin on right
   - 2.1.3.3) Boolean **bWasCancelled** output pin

### **2A) CRITICAL: Check bWasCancelled Before Cleanup (v2.5.5)**

**IMPORTANT:** The EndAbility event fires in TWO scenarios:
1. **bWasCancelled = false**: After activation flow calls EndAbility (form is now active)
2. **bWasCancelled = true**: When cancelled by another form's Cancel Abilities With Tag

Cleanup should ONLY run when bWasCancelled = true (form switch in progress).

#### 2A.1) Add Branch on bWasCancelled

##### 2A.1.1) Create Branch Node
   - 2A.1.1.1) From Event EndAbility execution pin
   - 2A.1.1.2) Drag wire to right and release
   - 2A.1.1.3) Search: `Branch`
   - 2A.1.1.4) Select **Branch** node

##### 2A.1.2) Connect Condition
   - 2A.1.2.1) From Event EndAbility **bWasCancelled** pin (boolean output)
   - 2A.1.2.2) Drag wire to Branch **Condition** input pin
   - 2A.1.2.3) Release to connect

#### 2A.2) Configure Branch Paths

##### 2A.2.1) False Path (Normal End - Skip Cleanup)
   - 2A.2.1.1) From Branch **False** execution pin
   - 2A.2.1.2) No connection needed - execution ends (form is now active, keep state)

##### 2A.2.2) True Path (Cancelled - Do Cleanup)
   - 2A.2.2.1) From Branch **True** execution pin
   - 2A.2.2.2) Continue to Section 3 (Get References for Cleanup)

### **3) Get References for Cleanup**

#### 3.1) Add Get Avatar Actor Node

##### 3.1.1) Create Node
   - 3.1.1.1) From Event EndAbility execution pin
   - 3.1.1.2) Drag wire to right
   - 3.1.1.3) Release to open context menu
   - 3.1.1.4) Search: `Get Avatar Actor From Actor Info`
   - 3.1.1.5) Click to add node

#### 3.2) Cast to Father

##### 3.2.1) Add Cast Node
   - 3.2.1.1) From Get Avatar Actor execution, drag wire right
   - 3.2.1.2) Release and search: `Cast To BP_FatherCompanion`
   - 3.2.1.3) Add cast node
   - 3.2.1.4) Connect Get Avatar **Return Value** to Cast **Object** pin

#### 3.3) Get Owner Player

##### 3.3.1) Add Get Owner Node
   - 3.3.1.1) From Cast success execution pin, drag right
   - 3.3.1.2) Search: `Get Owner Player`
   - 3.3.1.3) Add node
   - 3.3.1.4) Connect **As BP Father Companion** to **Target** pin

#### 3.4) Validate Owner

##### 3.4.1) Add Is Valid Check
   - 3.4.1.1) From Get Owner Player execution, drag right
   - 3.4.1.2) Search: `Is Valid`
   - 3.4.1.3) Add Is Valid macro node
   - 3.4.1.4) Connect Get Owner Player **Return Value** to **Input Object** pin

### **4) Restore Original Movement Speed**

#### 4.1) Get Character Movement Component

##### 4.1.1) Create Node
   - 4.1.1.1) From Is Valid **Is Valid** execution pin (valid path)
   - 4.1.1.2) Drag wire to right
   - 4.1.1.3) Release and search: `Get Character Movement`
   - 4.1.1.4) Add node
   - 4.1.1.5) Connect Get Owner Player **Return Value** to **Target** pin

##### 4.1.2) Verify Connection
   - 4.1.2.1) Execution wire connected from Is Valid
   - 4.1.2.2) Target connected to Owner Player
   - 4.1.2.3) Return Value outputs Character Movement Component

#### 4.2) Validate Original Speed Before Restoration

##### 4.2.1) Get Original Speed Variable
   - 4.2.1.1) In My Blueprint panel, find **OriginalMaxWalkSpeed** variable
   - 4.2.1.2) Drag to graph (getter node)
   - 4.2.1.3) Position to right of Get Character Movement node

##### 4.2.2) Add Greater Than Comparison
   - 4.2.2.1) From GET OriginalMaxWalkSpeed output pin
   - 4.2.2.2) Drag wire to empty space
   - 4.2.2.3) Release and search: `> float`
   - 4.2.2.4) Select **float > float** (Greater Than) node
   - 4.2.2.5) First input auto-connected to OriginalMaxWalkSpeed
   - 4.2.2.6) In second input pin, type: `0.0`

##### 4.2.3) Add Branch Node
   - 4.2.3.1) From Get Character Movement execution pin
   - 4.2.3.2) Drag wire to right
   - 4.2.3.3) Release and search: `Branch`
   - 4.2.3.4) Add Branch node
   - 4.2.3.5) Connect Greater Than **Return Value** to Branch **Condition** pin

#### 4.3) Set Max Walk Speed to Original

##### 4.3.1) Create Set Node
   - 4.3.1.1) From Branch **True** execution pin
   - 4.3.1.2) Drag wire to right
   - 4.3.1.3) Release and search: `Set Max Walk Speed`
   - 4.3.1.4) Add node
   - 4.3.1.5) Connect Get Character Movement **Return Value** to **Target** pin

##### 4.3.2) Connect Original Speed
   - 4.3.2.1) From GET OriginalMaxWalkSpeed output pin
   - 4.3.2.2) Drag wire to Set Max Walk Speed **Max Walk Speed** input pin
   - 4.3.2.3) Release to connect
   - 4.3.2.4) Player speed restored only if OriginalMaxWalkSpeed was stored (> 0)

### **5) State Tags Auto-Removed by Ability Lifecycle**

#### 5.1) Automatic Cleanup
   - 5.1.1) Father.State.Attached tag automatically removed when ability ends (Activation Owned Tag)
   - 5.1.2) Form identity tag (Effect.Father.FormState.Armor) removed by GE_ArmorState removal in graph

### **6) Stat and Ability Cleanup (Handled by EquippableItem)**

Stat removal and ability cleanup are handled automatically by BP_FatherArmorForm EquippableItem.

| Cleanup | Handled By |
|---------|------------|
| -50 Armor | HandleUnequip removes GE_EquipmentModifier_FatherArmor |
| GA_ProtectiveDome | HandleUnequip removes via Abilities array |
| GA_DomeBurst | HandleUnequip removes via Abilities array |

GA_FatherArmor EndAbility only handles movement restoration and state reset.

### **7) Reset State Variables**

#### 7.1) Set Is Attached to False
   - 7.1.1) From Set Max Walk Speed execution (or Branch False if speed not stored):
      - 7.1.1.1) From **As BP Father Companion**, search: `Set Is Attached`
      - 7.1.1.2) Add **Set Is Attached** node
   - 7.1.2) **Is Attached**: Uncheck (false)

### **8) Compile and Save**
   - 8.1) Click **Compile** button
   - 8.2) Verify no errors
   - 8.3) Click **Save** button

### **9) Verify EndAbility Cleanup Flow**

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event EndAbility | Fires when ability ends |
| 2 | **Branch (bWasCancelled)** | **CRITICAL: Only cleanup when cancelled (v2.5.5)** |
| 3 | FALSE path | Skip cleanup - form is now active |
| 4 | TRUE path → Continue | Form switch in progress - do cleanup |
| 5 | Get Avatar Actor From Actor Info | Get father reference |
| 6 | Cast to BP_FatherCompanion | Type-safe reference |
| 7 | Get Owner Player | Get player reference |
| 8 | Is Valid (Owner) | Validate player exists |
| 9 | Get Character Movement | Access movement component |
| 10 | Set Max Walk Speed | Restore original speed |
| 11 | Form Tags Auto-Removed | Activation Owned Tags cleanup (automatic) |
| 12 | Stat/Ability Cleanup | Handled by EquippableItem HandleUnequip |
| 13 | Set Is Attached (false) | Reset attachment state |

---

## **QUICK REFERENCE**

### **Tag Configuration Summary**

| Property | Tags |
|----------|------|
| Ability Tags | `Ability.Father.Armor` |
| Cancel Abilities with Tag | Ability.Father.Crawler, Exoskeleton, Symbiote, Engineer |
| Activation Owned Tags | Father.State.Attached |
| Activation Required Tags | Father.State.Alive, Father.State.Recruited |
| Activation Blocked Tags | Father.State.Dormant, Father.State.Transitioning, Father.State.SymbioteLocked, Cooldown.Father.FormChange |
| Input Tag | `Narrative.Input.Father.FormChange` |

### **Form State Architecture (Option B)**

| Component | Description |
|-----------|-------------|
| Activation Owned Tags | Father.State.Attached (auto-granted on activation, ephemeral) |
| Form Identity | GE_ArmorState grants Effect.Father.FormState.Armor (persists via infinite GE) |
| ReplicateActivationOwnedTags | ENABLED in Project Settings (enables multiplayer replication) |
| Transitioning Tag | AddLooseGameplayTag(Father.State.Transitioning) during 5s transition (**NO invulnerability** per INV-1) |
| GE_FormChangeCooldown | 15s cooldown after form change |
| Mutual Exclusivity | Cancel Abilities With Tag cancels other form abilities |

### **Variable Summary**

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| OriginalMaxWalkSpeed | Float | 0.0 | Stores player speed for restoration |
| SpeedPenaltyMultiplier | Float | 0.85 | Movement penalty multiplier (15% reduction) |
| ChestSocketName | Name | FatherChestSocket | Attachment socket on player skeleton |
| bIsFirstActivation | Boolean | True | Tracks initial spawn vs form switch |

### **Stat and Ability Handling**

| Component | Handled By | Notes |
|-----------|------------|-------|
| +50 Armor | BP_FatherArmorForm EquippableItem | Via GE_EquipmentModifier_FatherArmor |
| GA_ProtectiveDome | BP_FatherArmorForm Abilities array | Auto-granted on HandleEquip |
| GA_DomeBurst | BP_FatherArmorForm Abilities array | Auto-granted on HandleEquip |
| Movement Penalty | GA_FatherArmor directly | -15% via CharacterMovement |

### **Transition Animation Parameters**

| Parameter | Value |
|-----------|-------|
| VFX Duration | 5 seconds |
| Father Invulnerable | **No** (GAS Audit INV-1 - vulnerable during transition) |
| Form Cooldown | 15 seconds |
| VFX System | NS_FatherFormTransition |

### **Multiplayer Settings**

| Setting | Value | Purpose |
|---------|-------|---------|
| Instancing Policy | Instanced Per Actor | One instance per ASC |
| Replication Policy | Replicate Yes | Effects and tags replicate |
| Net Execution Policy | Server Only | NPC-owned form ability granting to player ASC |
| ReplicateActivationOwnedTags | ENABLED | Form tags replicate to clients |

### **Stat Modifications**

| Stat | Value | Method |
|------|-------|--------|
| Armor | +50 | BP_FatherArmorForm EquippableItem via GE_EquipmentModifier_FatherArmor |
| Movement Speed | -15% | CharacterMovement component direct (GA_FatherArmor) |

### **Effect Removal Architecture**

| Approach | Method | Used By |
|----------|--------|---------|
| Activation Owned Tags | Auto-removed when ability ends | State tag (Father.State.Attached) |
| GE_ArmorState Removal | Explicitly removed in EndAbility graph | Form identity (Effect.Father.FormState.Armor) |
| EquippableItem HandleUnequip | Automatic removal via parent method | Stats and abilities |
| Speed Restoration | Set Max Walk Speed to original value | CharacterMovement component |

### **Abilities Granted via EquippableItem**

| Ability | Granted By | Cleanup Method |
|---------|------------|----------------|
| GA_ProtectiveDome | BP_FatherArmorForm Abilities array | HandleUnequip auto-removes |
| GA_DomeBurst | BP_FatherArmorForm Abilities array | HandleUnequip auto-removes |

### **Ability Granting Architecture**

| Method | Asset | Purpose |
|--------|-------|---------|
| AbilityConfiguration | AC_FatherCompanion_Default | Grants all baseline abilities at spawn |
| NPCDefinition | NPCDef_FatherCompanion | References AbilityConfiguration |

### **Form System (5 Forms)**

| Form | Socket | Primary Benefit |
|------|--------|-----------------|
| Crawler | None (following) | Default combat companion |
| Armor | FatherChestSocket | +50 armor, -15% speed |
| Exoskeleton | FatherBackSocket | +50% speed boost |
| Symbiote | Full body merge | Berserker mode |
| Engineer | Deployed turret | Stationary firepower |

---

## **RELATED GUIDES**

### **Prerequisite Guides**

| Guide | Purpose |
|-------|---------|
| GE_FormChangeCooldown_Implementation_Guide_v1_0.md | Form change cooldown effect |
| BP_FatherCompanion_Setup_Guide_v2_3.md | Father companion blueprint configuration |
| Project Settings | ReplicateActivationOwnedTags must be ENABLED |

### **Armor Form Ability Guides**

| Guide | Purpose |
|-------|---------|
| Father_Protective_Dome_Implementation_Guide_v1_9.md | Dome system setup including attributes and damage absorption |
| GA_DomeBurst_Implementation_Guide_v2_3.md | Manual/auto burst ability triggered by Q key or energy threshold |

### **Reference Documents**

| Document | Relevant Sections |
|----------|-------------------|
| Father_Companion_Technical_Reference_v5_12.md | Section 7.4: Net Execution Policy, Section 14.8: Effect Handle Storage, Section 36.12: Two-Step Cleanup, Section 37: Recruited Tag Gating |
| Father_Companion_System_Setup_Guide_v2_3.md | Phase 12: Effect and Ability Handles |
| DefaultGameplayTags_FatherCompanion_v3_5.ini | Complete tag list including Father.State.Recruited |

### **Implementation Order**

| Step | Guide | Description |
|------|-------|-------------|
| 1 | Project Settings | Enable ReplicateActivationOwnedTags |
| 2 | GE_FormChangeCooldown | Create cooldown effect |
| 3 | GA_FatherArmor (this guide) | Core armor form attachment and stat bonuses |
| 4 | Father_Protective_Dome | Dome attributes, absorption effects, GA_ProtectiveDome |
| 5 | GA_DomeBurst | Burst ability with AOE damage and knockback |

### **Dome System Summary**

| Component | Description |
|-----------|-------------|
| GA_ProtectiveDome | Passive ability granted to player when armor form activates |
| GA_DomeBurst | Active ability (Q key) or auto-trigger when dome fully charged |
| AS_DomeAttributes | DomeEnergy and MaxDomeEnergy attributes on player |
| GE_DomeAbsorption | 30% damage absorption effect |
| GE_DomeBurstDamage | AOE damage effect (150-300 based on energy) |

---

**END OF GA_FATHERARMOR IMPLEMENTATION GUIDE VERSION 4.6**

**Blueprint-Only Implementation for Unreal Engine 5.7 + Narrative Pro v2.2**

---

## **CHANGELOG**

| Version | Changes |
|---------|---------|
| 4.6 | **INV-1 Compliance:** Fixed changelog entry - GE_ArmorState grants only Effect.Father.FormState.Armor (no Narrative.State.Invulnerable). Only GA_FatherSacrifice grants invulnerability (to player for 8s). |
| 4.4 | **Option B Form State Architecture:** Replaced ActivationOwnedTags form identity with GE-based persistent state (GE_ArmorState). Added transition prelude to PHASE 5A: remove prior form state GE via BP_RemoveGameplayEffectFromOwnerWithGrantedTags, then apply GE_ArmorState. GE_ArmorState grants Effect.Father.FormState.Armor (Infinite duration). Removed ReplicateActivationOwnedTags prerequisite. Added Automation vs Manual table. Updated UE version to 5.7. See Form_State_Architecture_Audit_v1_0.md for architecture rationale. |
| 4.3 | Blueprint Node Consistency Fixes: Removed incorrect Target pin documentation from Play Montage and Wait (AbilityTask nodes use owning ability avatar automatically). Fixed End Ability terminal node issue - Set bIsFirstActivation now executes BEFORE End Ability since End Ability has no output execution pin. |
| 4.2 | Fixed tag format: State.Father.Alive changed to Father.State.Alive per DefaultGameplayTags. Updated Related Documents to Technical Reference v5.12 and Setup Guide v2.3. Fixed curly quotes to straight ASCII. |
| 4.1 | Removed GE_ArmorBoost and ability handle cleanup - stats and abilities now handled by BP_FatherArmorForm EquippableItem via GE_EquipmentModifier_FatherArmor. GA_FatherArmor only handles physical attachment and movement penalty. Removed ArmorBoostHandle, DomeAbilityHandle, DomeBurstAbilityHandle from requirements. Simplified EndAbility to only restore movement speed and reset state. Updated all references from GE_ArmorBoost to EquippableItem pattern. |
| 4.0 | Simplified configuration sections: Tag Config (Class Defaults) reduced from 180 to 8 lines using table format. Variable Creation reduced from 106 to 13 lines using table format. GE_ArmorBoost config reduced from 190 to 15 lines using table format. Total reduction: ~450 lines. |
| 3.9 | Simplified PHASE 1 tag creation - replaced detailed step-by-step instructions with simple tag list tables. |
| 3.8 | Built-in cooldown system: Added CooldownGameplayEffectClass property, replaced Apply GE with CommitAbilityCooldown node. GAS auto-blocks during cooldown. |
| 3.7 | Form tags via Activation Owned Tags. ReplicateActivationOwnedTags required. Simplified ability logic and EndAbility cleanup. |
| 3.6 | Added GA_ProtectiveDome and GA_DomeBurst granting. Two-step cleanup for granted abilities. |
| 3.5 | Net Execution Policy: Server Only for cross-actor operations. |
| 3.4 | Added Father.State.Recruited gating. Handle-based effect removal. |
| 3.3 | Consolidated prerequisites. Fixed Exoskeleton speed reference. |
| 3.2 | Added form transition animation and 15s cooldown. |
| 3.1 | Cancel Abilities with Tag for mutual exclusivity. Self-cleaning EndAbility. |
| 3.0 | Initial implementation. |
