# GA_FatherCrawler - Crawler Form Ability Implementation Guide
## VERSION 4.5 - GAS Audit Compliant (INV-1)
## For Unreal Engine 5.7 + Narrative Pro Plugin v2.2

**Version:** 4.5
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
| GE_CrawlerState definition | ✅ Auto-generated | manifest.yaml gameplay_effects section |
| GA_FatherCrawler blueprint | ✅ Auto-generated | manifest.yaml gameplay_abilities section |
| Activation tags config | ✅ Auto-generated | Required/Blocked tags in manifest |
| Transition prelude nodes | ⚠️ Manual | Remove old state GE, apply new state GE |
| Detach logic | ⚠️ Manual | DetachFromActor nodes |
| VFX spawning | ⚠️ Manual | GameplayCues preferred (Category C roadmap) |
| EndAbility cleanup | ⚠️ Manual | State reset, position update |

---

## **DOCUMENT PURPOSE**

This guide provides step-by-step instructions for implementing GA_FatherCrawler, the default form ability that sets the father companion to autonomous Crawler mode. The Crawler form represents the father's baseline state where it follows the player independently, engages enemies autonomously, and provides support fire without requiring direct player control.

**Key Features:**
- Father operates as autonomous AI companion
- Detached from player (free movement)
- Default form via bActivateAbilityOnGranted
- **Form identity via GE_CrawlerState** (Infinite-duration GE grants Effect.Father.FormState.Crawler)
- Mutual exclusivity via Cancel Abilities With Tag
- **Transition prelude removes prior form state before applying new** (Option B)
- 5-second transition animation with VFX (when switching TO Crawler)
- 15-second shared form cooldown
- Full multiplayer compatibility via server authority

---

## **PREREQUISITES**

Before implementing GA_FatherCrawler, ensure the following are complete:

1. BP_FatherCompanion must be created and configured
2. NarrativeNPCCharacter base class must be set for father
3. Ability System Component must be initialized automatically
4. Enhanced Input System must be enabled in project settings
5. Narrative Pro v2.2 plugin must be installed and enabled
6. E_FatherForm enum must include: Crawler, Armor, Exoskeleton, Symbiote, Engineer
7. **GE_CrawlerState Gameplay Effect must exist** (Infinite duration, grants Effect.Father.FormState.Crawler)
8. **Father.State.Transitioning tag** must exist (added via AddLooseGameplayTag during transition - NO invulnerability per GAS Audit INV-1)
9. GE_FormChangeCooldown Gameplay Effect must exist (15s duration, grants Cooldown.Father.FormChange)
10. NS_FatherFormTransition Niagara System must be created (5s transition VFX)
11. AC_FatherCompanion_Default must be configured with startup_effects including GE_CrawlerState

---

## **PHASE 1: GAMEPLAY TAGS SETUP**

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Crawler | Father default crawler form ability - follow and attack mode |
| Effect.Father.FormState.Crawler | Crawler form identity tag (granted by GE_CrawlerState) |
| Father.State.Alive | Father is alive and can change forms |
| Father.State.Transitioning | Father is in form transition animation (5s) |
| Father.State.SymbioteLocked | Form changes blocked during 30s Symbiote duration |
| Cooldown.Father.FormChange | 15s shared cooldown for form changes |

### **Verify Existing Tags**

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Armor | Armor form ability |
| Ability.Father.Exoskeleton | Back attachment form ability |
| Ability.Father.Symbiote | Full body merge form ability |
| Ability.Father.Engineer | Turret deployment form ability |

**Note (v4.4):** `Father.Form.*` tags are **orphan tags** - no persistent GE grants them. Form identity uses `Effect.Father.FormState.*` tags granted by infinite-duration GE_*State effects. Do NOT use Father.Form.* for activation gating.

---

## **PHASE 2: CREATE GA_FATHERCRAWLER ABILITY BLUEPRINT**

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
   - 1.2.5) Inside FatherCompanion, create `Abilities` subfolder
   - 1.2.6) Inside Abilities, create `Forms` subfolder
   - 1.2.7) Final path: `/Content/FatherCompanion/Abilities/Forms/`

### **2) Create Ability Blueprint**

#### 2.1) Initiate Blueprint Creation
   - 2.1.1) In `/Content/FatherCompanion/Abilities/Forms/` folder
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
   - 2.2.3.3) Type blueprint name: `GA_FatherCrawler`
   - 2.2.3.4) Press **Enter** to confirm name
   - 2.2.3.5) Blueprint asset shows GA_ prefix in icon

### **3) Open Ability Blueprint**

#### 3.1) Open Blueprint Editor
   - 3.1.1) Double-click on `GA_FatherCrawler` blueprint asset
   - 3.1.2) Blueprint editor opens in new tab
   - 3.1.3) Tab shows blueprint name: GA_FatherCrawler
   - 3.1.4) **Event Graph** tab active by default

#### 3.2) Verify Blueprint Setup
   - 3.2.1) In toolbar, verify blueprint name shows: GA_FatherCrawler
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

### **2) Configure Narrative Ability Settings**

#### 2.1) Set Auto-Activation
   - 2.1.1) In Details panel, find **Narrative Ability** category
   - 2.1.2) Locate **bActivateAbilityOnGranted** property
   - 2.1.3) Check the checkbox: **Enabled**
   - 2.1.4) This makes Crawler the default form on spawn

#### 2.2) Set Input Tag
   - 2.2.1) Locate **Input Tag** property
   - 2.2.2) Leave as default (None/empty)
   - 2.2.3) Form wheel handles activation, no direct input binding

### **3) Configure Instancing Policy**

#### 3.1) Set Instancing Policy
   - 3.1.1) In Details panel, find **Ability** category
   - 3.1.2) Click arrow to expand category if collapsed
   - 3.1.3) Locate **Instancing Policy** dropdown property
   - 3.1.4) Click dropdown arrow
   - 3.1.5) Select **Instanced Per Actor** option
   - 3.1.6) This ensures one ability instance per father

### **4) Configure Replication Policy**

#### 4.1) Set Replication Policy
   - 4.1.1) In Details panel, under **Ability** category
   - 4.1.2) Find **Replication Policy** dropdown property
   - 4.1.3) Click dropdown arrow
   - 4.1.4) Select **Replicate Yes** option
   - 4.1.5) This enables multiplayer synchronization

### **5) Configure Net Execution Policy**

#### 5.1) Set Net Execution Policy
   - 5.1.1) In Details panel, under **Ability** category
   - 5.1.2) Find **Net Execution Policy** dropdown property
   - 5.1.3) Click dropdown arrow
   - 5.1.4) Select **Server Only** option
   - 5.1.5) NPC-owned form ability executes on server, clients see replicated results

### **6) Configure Cooldown (Built-in System)**

#### 6.1) Set Cooldown Gameplay Effect Class
   - 6.1.1) In Details panel, find **Cooldown** section
   - 6.1.2) Locate **Cooldown Gameplay Effect Class** property
   - 6.1.3) Click dropdown and select: `GE_FormChangeCooldown`
   - 6.1.4) GAS automatically blocks activation when cooldown tag is present

### **7) Configure Ability Tags (Class Defaults)**

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Crawler |
| Cancel Abilities with Tag | Ability.Father.Armor, Ability.Father.Exoskeleton, Ability.Father.Symbiote, Ability.Father.Engineer |
| Activation Required Tags | Father.State.Alive, Father.State.Recruited |
| Activation Blocked Tags | Father.State.Dormant, Father.State.Transitioning, Father.State.SymbioteLocked, Cooldown.Father.FormChange |

**Note (v4.4):** Activation Owned Tags are NOT used for form identity. Form identity persists via GE_CrawlerState which grants `Effect.Father.FormState.Crawler` (Option B architecture). See Form_State_Architecture_Audit_v1_0.md.

### **8) Compile and Save**
   - 8.1) Click **Compile** button in toolbar
   - 8.2) Wait for compilation to complete
   - 8.3) Verify no errors in message log
   - 8.4) Click **Save** button in toolbar
   - 8.5) Blueprint properties now configured

---

## **PHASE 4: CREATE ABILITY VARIABLES**

### **Variable Definitions**

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| bIsFirstActivation | Boolean | True | No |
| FatherRef | BP_FatherCompanion Reference | None | No |

### **Compile Blueprint**
   - Click **Compile** button in toolbar
   - Verify no errors
   - Variables now available for use in graph

---

## **PHASE 5: IMPLEMENT ABILITY LOGIC (Initial Spawn)**

This phase covers the initial spawn flow where bActivateAbilityOnGranted triggers the ability. The form switch path (PHASE 5A) handles transitions from other forms.

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

### **3) Get Father Actor Reference**

#### 3.1) Add Get Avatar Actor From Actor Info Node

##### 3.1.1) Create Get Avatar Node
   - 3.1.1.1) Right-click in graph area to right of Event node
   - 3.1.1.2) In search box, type: `Get Avatar Actor From Actor Info`
   - 3.1.1.3) Click on node in results list
   - 3.1.1.4) Blue pure function node appears

##### 3.1.2) Verify Node Placement
   - 3.1.2.1) Node positioned to right of Event node
   - 3.1.2.2) Has blue output pin labeled **Return Value**
   - 3.1.2.3) No execution pins (pure function provides data only)

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

### **4) Store Father Reference**

#### 4.1) Set FatherRef Variable

##### 4.1.1) Create Setter Node
   - 4.1.1.1) From Cast node success execution pin
   - 4.1.1.2) Drag connection wire to right
   - 4.1.1.3) Release to open context menu
   - 4.1.1.4) In search box, type: `Set FatherRef`
   - 4.1.1.5) Select **Set FatherRef** node

##### 4.1.2) Connect Value
   - 4.1.2.1) From Cast node **As BP Father Companion** output pin
   - 4.1.2.2) Drag wire to Set FatherRef input pin
   - 4.1.2.3) Release to connect

### **5) Branch: Check If First Activation**

#### 5.1) Add Branch Node

##### 5.1.1) Create Branch Node
   - 5.1.1.1) From Set FatherRef execution output pin
   - 5.1.1.2) Drag connection wire to right
   - 5.1.1.3) Release to open context menu
   - 5.1.1.4) In search box, type: `Branch`
   - 5.1.1.5) Select **Flow Control > Branch** node
   - 5.1.1.6) Branch node appears with True/False execution pins

##### 5.1.2) Connect Condition
   - 5.1.2.1) Drag **bIsFirstActivation** variable into graph (GET)
   - 5.1.2.2) Connect **bIsFirstActivation** output to Branch **Condition** pin

#### 5.2) Configure Branch Paths
   - 5.2.1) **True** path: Initial Spawn (continues below)
   - 5.2.2) **False** path: Form Switch with Transition (PHASE 5A)

### **6) Update Father State Variables (True Path - Initial Spawn)**

#### 6.1) Set Current Form

##### 6.1.1) Create Setter Node
   - 6.1.1.1) From Branch **True** execution pin
   - 6.1.1.2) Drag connection wire to right
   - 6.1.1.3) Release and search: `Set Current Form`
   - 6.1.1.4) Add node

##### 6.1.2) Connect Target
   - 6.1.2.1) From **FatherRef** variable (GET)
   - 6.1.2.2) Connect to Set Current Form **Target** pin

##### 6.1.3) Set Form Value
   - 6.1.3.1) In **Current Form** dropdown, select: `Crawler`

#### 6.2) Set Is Attached

##### 6.2.1) Create Setter Node
   - 6.2.1.1) From Set Current Form execution pin
   - 6.2.1.2) Search: `Set Is Attached`
   - 6.2.1.3) Add node

##### 6.2.2) Connect and Configure
   - 6.2.2.1) Connect **FatherRef** to **Target**
   - 6.2.2.2) Uncheck **Is Attached** checkbox (set to **false**)

### **7) Set First Activation Flag to False**

#### 7.1) Add Set bIsFirstActivation Node
   - 7.1.1) From Set Is Attached execution pin
   - 7.1.2) Search: `Set bIsFirstActivation`
   - 7.1.3) Add node
   - 7.1.4) Leave checkbox unchecked (false)
   - 7.1.5) Next activation will use Form Switch path (PHASE 5A)

### **8) End Ability (Initial Spawn)**

#### 8.1) Add End Ability Node
   - 8.1.1) From Set bIsFirstActivation execution pin
   - 8.1.2) Search: `End Ability`
   - 8.1.3) Add node
   - 8.1.4) **Was Cancelled**: Leave unchecked (false)

### **9) Compile and Save**
   - 9.1) Click **Compile** button
   - 9.2) Verify no errors
   - 9.3) Click **Save** button

---

## **PHASE 5A: FORM SWITCH WITH TRANSITION ANIMATION (Option B)**

This section covers the extended logic when switching TO Crawler from another form via the T wheel. The initial spawn flow (PHASE 5) uses bActivateAbilityOnGranted and skips transition animation.

**CRITICAL (Option B):** The transition prelude MUST:
1. Remove ALL prior form state GEs (Single Active Form State Invariant)
2. Apply GE_CrawlerState (grants Effect.Father.FormState.Crawler)

### **1) TRANSITION PRELUDE: Remove Prior Form State (Option B)**

#### 1.1) Get Father Ability System Component
   - 1.1.1) From Branch node **False** execution pin
   - 1.1.2) From **FatherRef** variable
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

#### 2.1) Apply GE_CrawlerState
   - 2.1.1) Drag from **BP_RemoveGameplayEffectFromOwnerWithGrantedTags** execution pin
   - 2.1.2) Search: `Apply Gameplay Effect to Self`
   - 2.1.3) Select node
   - 2.1.4) Connect Father ASC to **Target**
   - 2.1.5) Set **Gameplay Effect Class**: `GE_CrawlerState`
   - 2.1.6) **Result:** Father now has Effect.Father.FormState.Crawler tag

### **3) Add Transitioning Tag**

> **v4.5 (GAS Audit INV-1):** Form transitions NO LONGER grant invulnerability.
> Father is vulnerable during the 5s transition. Use AddLooseGameplayTag for the transitioning state.

#### 3.1) Add Loose Gameplay Tag (Father.State.Transitioning)
   - 3.1.1) Drag from **Apply GE_CrawlerState** execution pin
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

#### 4.2) Configure VFX Parameters
   - 4.2.1) **System Template**: Select `NS_FatherFormTransition`
   - 4.2.2) **Attach to Component**: From **FatherRef** -> Get **Mesh**
   - 4.2.3) **Attach Point Name**: `root`
   - 4.2.4) **Location Type**: `Snap to Target`

### **5) Wait 5 Seconds**

#### 5.1) Add Delay Node
   - 5.1.1) Drag from **Spawn System Attached** execution pin
   - 5.1.2) Search: `Delay`
   - 5.1.3) Select node
   - 5.1.4) Set **Duration**: `5.0`

### **6) Detach Father From Player**

#### 6.1) Add Detach From Actor Node
   - 6.1.1) Drag from **Delay** -> **Completed** pin
   - 6.1.2) Search: `Detach From Actor`
   - 6.1.3) Select node
   - 6.1.4) Connect **FatherRef** to **Target**

#### 6.2) Configure Detach Parameters
   - 6.2.1) **Location Rule**: `Keep World`
   - 6.2.2) **Rotation Rule**: `Keep World`
   - 6.2.3) **Scale Rule**: `Keep World`

### **7) Move Father Behind Player**

#### 7.1) Get Owner Player
   - 7.1.1) From **FatherRef**
   - 7.1.2) Search: `Get Owner Player`
   - 7.1.3) Select node

#### 7.2) Calculate Spawn Position
   - 7.2.1) From **Get Owner Player** -> **Return Value**
   - 7.2.2) Add **Get Actor Location** node
   - 7.2.3) Add **Get Actor Forward Vector** node
   - 7.2.4) Add **Multiply (Vector * Float)** node
   - 7.2.5) Set Float: `-200.0` (negative for behind player)
   - 7.2.6) Add **Add (Vector + Vector)** node
   - 7.2.7) Connect: Location + (Forward * -200)

#### 7.3) Set Actor Location
   - 7.3.1) After **Detach From Actor** execution
   - 7.3.2) Add **Set Actor Location** node
   - 7.3.3) Connect **FatherRef** to **Target**
   - 7.3.4) Connect calculated position to **New Location**

### **8) Set Form Variables**

#### 8.1) Set Current Form
   - 8.1.1) Drag from **Set Actor Location** execution pin
   - 8.1.2) Add **Set Current Form** node
   - 8.1.3) Connect **FatherRef** to **Target**
   - 8.1.4) Set **Current Form**: `Crawler`

#### 8.2) Set Is Attached
   - 8.2.1) Drag from **Set Current Form** execution pin
   - 8.2.2) Add **Set Is Attached** node
   - 8.2.3) Connect **FatherRef** to **Target**
   - 8.2.4) Set **Is Attached**: `False` (unchecked)

### **9) Remove Transitioning State**

#### 9.1) Remove Loose Gameplay Tag
   - 9.1.1) Drag from **Set Is Attached** execution pin
   - 9.1.2) Search: `Remove Loose Gameplay Tag`
   - 9.1.3) Select node
   - 9.1.4) Connect Father ASC to **Target**
   - 9.1.5) Set **Gameplay Tag**: `Father.State.Transitioning`

### **10) Apply Form Cooldown**

> **v4.5 (GAS Audit INV-1):** No GE removal needed - Father.State.Transitioning tag was removed in Step 9 via RemoveLooseGameplayTag.

#### 10.1) Commit Ability Cooldown
   - 10.1.1) Drag from **Remove Loose Gameplay Tag** execution pin
   - 10.1.2) Search: `Commit Ability Cooldown`
   - 10.1.3) Select **Commit Ability Cooldown** node
   - 10.1.4) No parameters needed - uses CooldownGameplayEffectClass automatically

### **11) End Ability**

#### 11.1) Add End Ability Node
   - 11.1.1) Drag from **Commit Ability Cooldown** execution pin
   - 11.1.2) Search: `End Ability`
   - 11.1.3) Select node
   - 11.1.4) **Was Cancelled**: `False` (unchecked)

### **12) Compile and Save**

#### 12.1) Compile Blueprint
   - 12.1.1) Click **Compile** button

#### 12.2) Save Blueprint
   - 12.2.1) Click **Save** button

---

## **PHASE 6: IMPLEMENT ENDABILITY EVENT**

Crawler form has minimal EndAbility cleanup since:
- Form identity tag (Effect.Father.FormState.Crawler) is removed by the new form's transition prelude
- No stat modifiers to restore
- No abilities granted to remove

### **1) Add Event EndAbility Node**

#### 1.1) Create EndAbility Event
   - 1.1.1) Right-click in Event Graph empty space (below existing logic)
   - 1.1.2) In search box, type: `Event EndAbility`
   - 1.1.3) Click on **Event EndAbility** in results
   - 1.1.4) Red event node appears on graph

### **2) Check bWasCancelled**

#### 2.1) Add Branch on bWasCancelled
   - 2.1.1) From Event EndAbility execution pin
   - 2.1.2) Search: `Branch`
   - 2.1.3) Add Branch node
   - 2.1.4) Connect Event EndAbility **bWasCancelled** to Branch **Condition**

#### 2.2) Configure Branch Paths
   - 2.2.1) **False path**: Skip cleanup - form is now active (ability ran to EndAbility call)
   - 2.2.2) **True path**: Form switch in progress - another form cancelled this one

### **3) Cleanup (True Path - Cancelled)**

Form state cleanup is handled by the new form's transition prelude:
- New form removes Effect.Father.FormState.* via BP_RemoveGameplayEffectFromOwnerWithGrantedTags
- New form applies its own GE_*State

No additional cleanup required for Crawler form.

### **4) Compile and Save**
   - 4.1) Click **Compile** button
   - 4.2) Verify no errors
   - 4.3) Click **Save** button

---

## **PHASE 7: GRANT ABILITY VIA ABILITYCONFIGURATION**

### **1) Ability Granting Architecture**

GA_FatherCrawler is a baseline ability granted via the Narrative Pro AbilityConfiguration system:

| Component | Asset | Purpose |
|-----------|-------|---------|
| NPCDefinition | NPC_FatherCompanion | Father NPC configuration |
| AbilityConfiguration | AC_FatherCompanion | Grants baseline abilities on spawn |
| StartupEffects | GE_CrawlerState | Default form state at spawn (Option B) |

### **2) Verify AC_FatherCompanion Configuration**

#### 2.1) Check Abilities Array
   - 2.1.1) Open **AC_FatherCompanion** Data Asset
   - 2.1.2) Verify **GA_FatherCrawler** is in abilities array

#### 2.2) Check StartupEffects Array
   - 2.2.1) Verify **GE_CrawlerState** is in startup_effects array
   - 2.2.2) This applies default form state at spawn

### **3) Auto-Activation on Spawn**

| Step | Action |
|------|--------|
| 1 | NPCSpawner spawns father |
| 2 | NPCDefinition loads AbilityConfiguration |
| 3 | StartupEffects apply GE_CrawlerState (form identity) |
| 4 | GA_FatherCrawler granted to father ASC |
| 5 | bActivateAbilityOnGranted = true triggers OnAvatarSet |
| 6 | OnAvatarSet calls TryActivateAbility automatically |
| 7 | Ability sets CurrentForm = Crawler, IsAttached = False |
| 8 | Father starts in Crawler form |

---

## **QUICK REFERENCE**

### **Tag Configuration Summary**

| Property | Tags |
|----------|------|
| Ability Tags | `Ability.Father.Crawler` |
| Cancel Abilities with Tag | Ability.Father.Armor, Exoskeleton, Symbiote, Engineer |
| Activation Required Tags | Father.State.Alive, Father.State.Recruited |
| Activation Blocked Tags | Father.State.Dormant, Father.State.Transitioning, Father.State.SymbioteLocked, Cooldown.Father.FormChange |

### **Form State Architecture (Option B)**

| Component | Description |
|-----------|-------------|
| Form Identity | GE_CrawlerState grants Effect.Father.FormState.Crawler (persists via infinite GE) |
| Transitioning Tag | AddLooseGameplayTag(Father.State.Transitioning) during 5s transition (**NO invulnerability** per INV-1) |
| GE_FormChangeCooldown | 15s cooldown after form change |
| Mutual Exclusivity | Cancel Abilities With Tag cancels other form abilities |
| Transition Prelude | Remove Effect.Father.FormState.* -> Apply GE_CrawlerState |

### **Variable Summary**

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| bIsFirstActivation | Boolean | True | Tracks initial spawn vs form switch |
| FatherRef | BP_FatherCompanion | None | Cached father reference |

### **Transition Animation Parameters**

| Parameter | Value |
|-----------|-------|
| VFX Duration | 5 seconds |
| Father Invulnerable | **No** (GAS Audit INV-1 - vulnerable during transition) |
| Form Cooldown | 15 seconds |
| VFX System | NS_FatherFormTransition |
| Spawn Position | 200 units behind player |

### **Multiplayer Settings**

| Setting | Value | Purpose |
|---------|-------|---------|
| Instancing Policy | Instanced Per Actor | One instance per ASC |
| Replication Policy | Replicate Yes | Effects and tags replicate |
| Net Execution Policy | Server Only | NPC-owned form ability |
| bActivateAbilityOnGranted | True | Default form on spawn |

### **Effect Removal Architecture**

| Approach | Method | Used By |
|----------|--------|---------|
| Transition Prelude | BP_RemoveGameplayEffectFromOwnerWithGrantedTags | Remove prior form state |
| GE_CrawlerState | Applied after removal | New form identity |
| EndAbility | Minimal cleanup | Form identity removed by new form's prelude |

### **Ability Granting Architecture**

| Method | Asset | Purpose |
|--------|-------|---------|
| AbilityConfiguration | AC_FatherCompanion | Grants all baseline abilities at spawn |
| NPCDefinition | NPC_FatherCompanion | References AbilityConfiguration |
| StartupEffects | GE_CrawlerState | Default form state (Option B) |

### **Form System (5 Forms)**

| Form | Mode | Primary Benefit |
|------|------|-----------------|
| Crawler | Detached (following) | Default combat companion |
| Armor | Attached (chest) | +50 armor, -15% speed |
| Exoskeleton | Attached (back) | +50% speed boost |
| Symbiote | Merged (full body) | Berserker mode |
| Engineer | Deployed (turret) | Stationary firepower |

---

## **RELATED GUIDES**

### **Prerequisite Guides**

| Guide | Purpose |
|-------|---------|
| GE_FormChangeCooldown_Implementation_Guide_v1_0.md | Form change cooldown effect |
| BP_FatherCompanion_Setup_Guide_v2_3.md | Father companion blueprint configuration |
| Form_State_Architecture_Audit_v1_0.md | Option B architecture rationale |

### **Reference Documents**

| Document | Relevant Sections |
|----------|-------------------|
| Father_Companion_Technical_Reference_v6_0.md | Section 7.4: Net Execution Policy, Form State Architecture |
| Father_Companion_System_Setup_Guide_v3_0.md | Variable definition, EndPlay cleanup |
| DefaultGameplayTags_FatherCompanion_v4_0.ini | Complete tag list including Father.State.Recruited |

---

**END OF GA_FATHERCRAWLER IMPLEMENTATION GUIDE VERSION 4.4**

**Blueprint-Only Implementation for Unreal Engine 5.7 + Narrative Pro v2.2**

---

## **CHANGELOG**

| Version | Changes |
|---------|---------|
| 4.4 | **Option B Form State Architecture:** Complete rewrite from v3.4. Replaced ActivationOwnedTags form identity with GE-based persistent state (GE_CrawlerState). Added transition prelude to PHASE 5A: remove prior form state GE via BP_RemoveGameplayEffectFromOwnerWithGrantedTags, then apply GE_CrawlerState. GE_CrawlerState grants Effect.Father.FormState.Crawler (Infinite duration). Removed Father.Form.Crawler and Father.State.Detached from Activation Owned Tags (orphan tags). Updated UE version to 5.7. Added Automation vs Manual table. Added FatherRef variable for cached reference. Simplified EndAbility (form identity removed by new form's prelude). Added StartupEffects documentation for spawn flow. |
| 3.4 | Pure function connection fix - exec connections to pure functions handled by bypass. |
| 3.3 | Renamed from Spider to Father throughout entire document. All tags, abilities, references updated. |
| 3.2 | Simplified PHASE 1 tag creation - replaced detailed step-by-step instructions with simple tag list tables. |
| 3.1 | Built-in cooldown system: Added CooldownGameplayEffectClass property. |
| 3.0 | Form tags via Activation Owned Tags. ReplicateActivationOwnedTags required. |
| 2.9 | Net Execution Policy: Server Only for NPC-owned abilities. |
| 2.8 | Added Father.State.Recruited to Activation Required Tags. |
