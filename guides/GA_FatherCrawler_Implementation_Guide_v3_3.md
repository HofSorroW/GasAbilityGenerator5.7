# Father Companion - GA_FatherCrawler Implementation Guide

## VERSION 3.3 - Tag Format and Reference Updates

## Unreal Engine 5.6 + Narrative Pro Plugin v2.2

---

## **DOCUMENT INFORMATION**

| Property | Value |
|----------|-------|
| **Ability Name** | GA_FatherCrawler |
| **Ability Type** | Form - Default Autonomous Mode |
| **Parent Class** | NarrativeGameplayAbility |
| **Form** | Crawler (Default form) |
| **Input** | T (Form Wheel) or bActivateAbilityOnGranted |
| **Version** | 3.2 |
| **Last Updated** | January 2026 |
| **Engine** | Unreal Engine 5.6 |
| **Plugin** | Narrative Pro v2.2 |
| **Implementation** | Blueprint Only |
| **Multiplayer** | Compatible (Replicated via Activation Owned Tags) |
| **Dependencies** | BP_FatherCompanion, E_FatherForm enum, GE_Invulnerable, GE_FormChangeCooldown |

---

## **TABLE OF CONTENTS**

1. [Introduction](#introduction)
2. [Prerequisites](#prerequisites)
3. [PHASE 1: Gameplay Tags Setup](#phase-1-gameplay-tags-setup)
4. [PHASE 2: Create GA_FatherCrawler Ability](#phase-2-create-ga_fathercrawler-ability)
5. [PHASE 3: Configure Ability Properties](#phase-3-configure-ability-properties)
6. [PHASE 4: Implement Crawler Ability Logic](#phase-4-implement-crawler-ability-logic)
7. [PHASE 4A: Form Switch With Transition Animation](#phase-4a-form-switch-with-transition-animation)
8. [PHASE 4B: EndAbility Cleanup](#phase-4b-endability-cleanup)
9. [PHASE 5: Grant Ability via AbilityConfiguration](#phase-5-grant-ability-via-abilityconfiguration)
10. [Quick Reference](#quick-reference)
11. [Changelog](#changelog)

---

## **INTRODUCTION**

### **Ability Overview**

GA_FatherCrawler is the default form-changing ability that sets the father companion to its autonomous Crawler mode. The Crawler form represents the father's baseline state where it follows the player independently, engages enemies autonomously, and provides support fire without requiring direct player control.

This ability serves as the "neutral" form, allowing the father to operate at medium range while the player maintains full freedom of movement and combat capability. Unlike attached forms (Armor, Exoskeleton, Symbiote), the Crawler form keeps the father as a separate AI-controlled entity.

### **Form State Architecture (v3.0)**

Form state tags are managed by Activation Owned Tags with ReplicateActivationOwnedTags enabled. Form transitions include 5-second VFX animation with father invulnerability and 15-second shared cooldown.

| System | Description |
|--------|-------------|
| **Tag Management** | Activation Owned Tags (auto-granted on activation) |
| **Mutual Exclusivity** | Cancel Abilities with Tag |
| **Form Switching** | Direct T wheel selection |
| **Transition Animation** | 5s Niagara VFX with invulnerability |
| **Cooldown** | 15s shared (Cooldown.Father.FormChange) |

### **Key Features**

| Feature | Description |
|---------|-------------|
| **Form State Tags** | Father.Form.Crawler, Father.State.Detached via Activation Owned Tags |
| **Mutual Exclusivity** | Cancel Abilities with Tag cancels other form abilities |
| **State Management** | Sets CurrentForm and IsAttached variables |
| **AI Activation** | Returns autonomous control to father's Behavior Tree |
| **Auto-Activation** | Uses bActivateAbilityOnGranted for default starting form |
| **Transition Animation** | 5-second Niagara VFX during form change |
| **Invulnerability** | Father invulnerable during 5s transition |
| **Form Cooldown** | 15-second shared cooldown after transition |
| **Multiplayer Support** | Tags replicate via ReplicateActivationOwnedTags |
| **Blueprint Implementation** | Zero C++ code required |

### **Activation Methods**

| Method | Trigger | Use Case |
|--------|---------|----------|
| **bActivateAbilityOnGranted** | Ability granted on spawn | Default starting form (recommended) |
| **T (Form Wheel)** | Player selects Crawler in wheel | Return to Crawler from any other form |

### **Form Transition (From Other Forms)**

| From Form | Action | Result |
|-----------|--------|--------|
| Armor | Select Crawler in T wheel | GA_FatherCrawler activates, cancels GA_FatherArmor |
| Exoskeleton | Select Crawler in T wheel | GA_FatherCrawler activates, cancels GA_FatherExoskeleton |
| Symbiote | Not allowed during 30s lock | State.Father.SymbioteLocked blocks activation |
| Engineer | Select Crawler in T wheel | GA_FatherCrawler activates, cancels GA_FatherEngineer |

### **Crawler Activation Flow (Initial Spawn)**

| Step | Action | System |
|------|--------|--------|
| 1 | Father spawns via NPCSpawner | Narrative Pro Spawning |
| 2 | NPCDefinition loads AbilityConfiguration | AC_FatherCompanion_Default |
| 3 | GA_FatherCrawler granted to father | Ability System Component |
| 4 | bActivateAbilityOnGranted triggers | OnAvatarSet callback |
| 5 | GA_FatherCrawler activates | TryActivateAbility |
| 6 | Activation Owned Tags granted | Father.Form.Crawler, Father.State.Detached |
| 7 | CurrentForm = Crawler, IsAttached = False | Father state variables |
| 8 | End Ability called | Ability lifecycle complete |
| 9 | AI takes control | Behavior Tree activation |

### **Crawler Activation Flow (Form Switch)**

| Step | Action | Duration |
|------|--------|----------|
| 1 | Player selects Crawler in T wheel | - |
| 2 | GAS checks Activation Required/Blocked Tags | Instant |
| 3 | If blocked: nothing happens, no cooldown | - |
| 4 | If allowed: GA_FatherCrawler activates | - |
| 5 | Activation Owned Tags granted | Father.Form.Crawler, Father.State.Detached |
| 6 | Add State.Father.Transitioning tag | - |
| 7 | Apply GE_Invulnerable to father | - |
| 8 | Cancel Abilities with Tag cancels old form | - |
| 9 | Old form EndAbility restores stats | Instant |
| 10 | Spawn Niagara transition VFX | - |
| 11 | Wait for VFX duration | 5s |
| 12 | Detach father from player | - |
| 13 | Set CurrentForm = Crawler, IsAttached = False | - |
| 14 | Remove State.Father.Transitioning tag | - |
| 15 | Remove GE_Invulnerable | - |
| 16 | Apply Cooldown.Father.FormChange (15s) | - |
| 17 | End Ability | - |
| 18 | AI takes control | Behavior Tree activation |

---

## **PREREQUISITES**

### **Required Assets**

| Asset | Type | Purpose |
|-------|------|---------|
| **BP_FatherCompanion** | Blueprint Character | Father companion actor (parent: NarrativeNPCCharacter) |
| **E_FatherForm** | Enum | Father form types (Crawler, Armor, Exoskeleton, Symbiote, Engineer) |
| **NPCDef_FatherCompanion** | NPCDefinition | Father NPC configuration |
| **AC_FatherCompanion_Default** | AbilityConfiguration | Grants baseline abilities |
| **GE_FormChangeCooldown** | GameplayEffect | 15s form change cooldown |

### **Required Variables in BP_FatherCompanion**

| Variable | Type | Default | Replication | Purpose |
|----------|------|---------|-------------|---------|
| **CurrentForm** | E_FatherForm | Crawler | RepNotify | Tracks active form |
| **IsAttached** | Boolean | False | RepNotify | Tracks attachment state |
| **OwnerPlayer** | NarrativeCharacter Reference | None | Replicated | Reference to player |

### **Required Gameplay Tags**

| Tag | Category | Purpose |
|-----|----------|---------|
| Ability.Father.Crawler | Ability | Identifies this ability |
| Father.Form.Crawler | State | Activation Owned Tag |
| Father.State.Detached | State | Activation Owned Tag |
| Father.State.Alive | State | Activation required tag |
| State.Father.Transitioning | State | Activation blocked tag (during 5s VFX) |
| State.Father.SymbioteLocked | State | Activation blocked tag (during 30s Symbiote) |
| Cooldown.Father.FormChange | Cooldown | 15s shared form cooldown |

### **Required Effects**

| Effect | Purpose |
|--------|---------|
| **GE_Invulnerable** | Blocks damage during 5s transition (Narrative Pro built-in) |
| **GE_FormChangeCooldown** | Applies 15s cooldown after transition |

### **Required Project Setting**

| Setting | Value | Location |
|---------|-------|----------|
| **ReplicateActivationOwnedTags** | Enabled | Project Settings -> Gameplay Abilities |

---

## **PHASE 1: GAMEPLAY TAGS SETUP**

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Crawler | Father default crawler form ability - follow and attack mode |
| Father.State.Alive | Father is alive and can change forms |
| State.Father.Transitioning | Father is in form transition animation (5s) |
| State.Father.SymbioteLocked | Form changes blocked during 30s Symbiote duration |
| Cooldown.Father.FormChange | 15s shared cooldown for form changes |

### **Verify Existing Tags**

| Tag Name | Purpose |
|----------|---------|
| Father.Form.Crawler | Father is in Crawler form |
| Father.State.Detached | Father is detached from player |
| Father.State.Recruited | Father must be recruited (Activation Required) |
| State.Father.Dormant | Blocked after sacrifice |

---

## **PHASE 2: CREATE GA_FATHERCRAWLER ABILITY**

### **1) Navigate to Abilities Folder**

#### 1.1) Open Content Browser
   - 1.1.1) Click **Content Browser** tab
   - 1.1.2) Navigate to: `/Content/FatherCompanion/Abilities/`

#### 1.2) Create Folder Structure
   - 1.2.1) If **Abilities** folder missing, create it
   - 1.2.2) Inside **Abilities**, create **Forms** subfolder
   - 1.2.3) Folder path: `/Content/FatherCompanion/Abilities/Forms/`

### **2) Create Ability Blueprint**

#### 2.1) Create from Parent Class
   - 2.1.1) Right-click in **Forms** folder
   - 2.1.2) Select **Blueprint Class**
   - 2.1.3) In **Pick Parent Class** window, expand **All Classes**
   - 2.1.4) Search for: `NarrativeGameplayAbility`
   - 2.1.5) Select **NarrativeGameplayAbility**
   - 2.1.6) Click **Select** button

#### 2.2) Name the Blueprint
   - 2.2.1) Name: `GA_FatherCrawler`
   - 2.2.2) Press **Enter** to confirm

### **3) Open GA_FatherCrawler**

#### 3.1) Open Blueprint Editor
   - 3.1.1) Double-click **GA_FatherCrawler** asset

---

## **PHASE 3: CONFIGURE ABILITY PROPERTIES**

### **1) Access Class Defaults**

#### 1.1) Open Class Defaults Panel
   - 1.1.1) In Blueprint toolbar, click **Class Defaults** button
   - 1.1.2) Details panel shows class default properties

### **2) Configure Narrative Ability Settings**

#### 2.1) Set Auto-Activation
   - 2.1.1) In Details panel, find **Narrative Ability** category
   - 2.1.2) Locate **bActivateAbilityOnGranted** property
   - 2.1.3) Check the checkbox: **Enabled**

#### 2.2) Set Input Tag
   - 2.2.1) Locate **Input Tag** property
   - 2.2.2) Leave as default (None/empty)

### **3) Configure Ability Tags**

#### 3.1) Locate Ability Tags Section
   - 3.1.1) In Details panel, find **Tags** category
   - 3.1.2) Locate **Ability Tags** property

#### 3.2) Add Ability Tag
   - 3.2.1) Expand **Ability Tags**
   - 3.2.2) Find **Gameplay Tag Container** section
   - 3.2.3) Click **+** next to **Gameplay Tags** array
   - 3.2.4) Click dropdown on new element
   - 3.2.5) Search for: `Ability.Father.Crawler`
   - 3.2.6) Select tag from list

### **4) Configure Activation Required Tags**

#### 4.1) Locate Activation Required Tags
   - 4.1.1) In **Tags** category, find **Activation Required Tags**
   - 4.1.2) Click **+** next to array

#### 4.2) Add Father.State.Alive
   - 4.2.1) Click dropdown on new element
   - 4.2.2) Search for: `Father.State.Alive`
   - 4.2.3) Select tag from list

#### 4.3) Add Father.State.Recruited
   - 4.3.1) Click **+** next to array to add second element
   - 4.3.2) Click dropdown on new element
   - 4.3.3) Search for: `Father.State.Recruited`
   - 4.3.4) Select tag from list
   - 4.3.5) Gates form abilities to recruited fathers only

### **5) Configure Activation Blocked Tags**

#### 5.1) Locate Activation Blocked Tags
   - 5.1.1) In **Tags** category, find **Activation Blocked Tags**
   - 5.1.2) Click **+** next to array 3 times (for 3 tags)

#### 5.2) Add Blocked Tags
   - 5.2.1) First element: `State.Father.Dormant`
   - 5.2.2) Second element: `State.Father.Transitioning`
   - 5.2.3) Third element: `State.Father.SymbioteLocked`

### **6) Configure Activation Owned Tags**

#### 6.1) Locate Activation Owned Tags
   - 6.1.1) In **Tags** category, find **Activation Owned Tags**
   - 6.1.2) Click **+** to add element [0]
   - 6.1.3) Select: `Father.Form.Crawler`
   - 6.1.4) Click **+** to add element [1]
   - 6.1.5) Select: `Father.State.Detached`
   - 6.1.6) Tags auto-granted on activation, auto-removed on EndAbility

### **7) Configure Cancel Abilities with Tag**

#### 7.1) Locate Cancel Abilities with Tag
   - 7.1.1) In **Tags** category, find **Cancel Abilities with Tag**
   - 7.1.2) Click **+** next to array 4 times (for 4 tags)

#### 7.2) Add Cancel Tags (All Other Form Abilities)
   - 7.2.1) First element: `Ability.Father.Armor`
   - 7.2.2) Second element: `Ability.Father.Exoskeleton`
   - 7.2.3) Third element: `Ability.Father.Symbiote`
   - 7.2.4) Fourth element: `Ability.Father.Engineer`

### **8) Configure Ability Settings**

#### 8.1) Set Instancing Policy
   - 8.1.1) Find **Ability** category
   - 8.1.2) Locate **Instancing Policy**
   - 8.1.3) Set to: `Instanced Per Actor`

#### 8.2) Configure Replication
   - 8.2.1) Find **Replication Policy**
   - 8.2.2) Set to: `Replicate Yes`
   - 8.2.3) Find **Net Execution Policy**
   - 8.2.4) Set to: `Server Only`
   - 8.2.5) NPC-owned abilities execute on server, clients see replicated results

#### 8.3) Configure Cooldown (Built-in System)
   - 8.3.1) Find **Cooldown** section in Class Defaults
   - 8.3.2) Locate **Cooldown Gameplay Effect Class** property
   - 8.3.3) Click dropdown and select: `GE_FormChangeCooldown`
   - 8.3.4) GAS automatically blocks activation when cooldown tag is present

### **9) Compile and Save**

#### 9.1) Compile Blueprint
   - 9.1.1) Click **Compile** button in toolbar

#### 9.2) Save Blueprint
   - 9.2.1) Click **Save** button

---

## **PHASE 4: IMPLEMENT CRAWLER ABILITY LOGIC**

### **1) Navigate to Event Graph**

#### 1.1) Switch to Event Graph
   - 1.1.1) Click **Event Graph** tab in Blueprint Editor

### **2) Create Event ActivateAbility**

#### 2.1) Add Event Node
   - 2.1.1) Right-click in Event Graph
   - 2.1.2) Search for: `Event ActivateAbility`
   - 2.1.3) Select **Event ActivateAbility** from list

### **3) Get Father Actor Reference**

#### 3.1) Add Get Avatar Actor Node
   - 3.1.1) Drag from **Event ActivateAbility** execution pin
   - 3.1.2) Search: `Get Avatar Actor From Actor Info`
   - 3.1.3) Select node from list

#### 3.2) Connect Actor Info
   - 3.2.1) Drag from **Event ActivateAbility** -> **Actor Info** pin
   - 3.2.2) Connect to **Get Avatar Actor From Actor Info** -> **Actor Info** input

#### 3.3) Add Cast to BP_FatherCompanion
   - 3.3.1) Drag from **Get Avatar Actor** -> **Return Value** pin
   - 3.3.2) Search: `Cast to BP_FatherCompanion`
   - 3.3.3) Select cast node
   - 3.3.4) Connect execution: **Get Avatar Actor** exec -> **Cast** exec in

### **4) Set Father Form to Crawler**

#### 4.1) Add Set Current Form Node
   - 4.1.1) Drag from **Cast** success execution pin
   - 4.1.2) Search: `Set Current Form`
   - 4.1.3) Select setter node

#### 4.2) Connect Target
   - 4.2.1) Drag from **Cast** -> **As BP Father Companion** pin
   - 4.2.2) Connect to **Set Current Form** -> **Target** input

#### 4.3) Set Form Value
   - 4.3.1) In **Current Form** dropdown, select: `Crawler`

### **5) Set IsAttached to False**

#### 6.1) Add Set Is Attached Node
   - 6.1.1) Drag from **Set Current Form** execution pin
   - 6.1.2) Search: `Set Is Attached`
   - 6.1.3) Select setter node

#### 6.2) Connect Target
   - 6.2.1) Drag from **Cast** -> **As BP Father Companion** pin
   - 6.2.2) Connect to **Set Is Attached** -> **Target** input

#### 6.3) Set Attached Value
   - 6.3.1) Uncheck **Is Attached** checkbox (set to **false**)

### **7) End Ability**

#### 7.1) Add End Ability Node
   - 7.1.1) Drag from **Set Is Attached** execution pin
   - 7.1.2) Search: `End Ability`
   - 7.1.3) Select node

#### 7.2) Configure End Ability
   - 7.2.1) **Was Cancelled**: Leave unchecked (false)

### **8) Verify Node Chain**

#### 8.1) Check Execution Flow

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event ActivateAbility | Entry point |
| 2 | Get Avatar Actor From Actor Info | Get actor reference |
| 3 | Cast to BP_FatherCompanion | Type-safe father reference |
| 4 | Set Current Form (Crawler) | Update form state variable |
| 5 | Set Is Attached (false) | Update attachment state variable |
| 6 | End Ability | Complete ability lifecycle |

#### 8.2) Check Data Connections

| Source | Target |
|--------|--------|
| Event ActivateAbility -> Actor Info | Get Avatar Actor -> Actor Info input |
| Get Avatar Actor -> Return Value | Cast -> Object input |
| Cast -> As BP Father Companion | Set Current Form -> Target |
| Cast -> As BP Father Companion | Set Is Attached -> Target |

### **9) Compile and Save**

#### 9.1) Compile Blueprint
   - 9.1.1) Click **Compile** button

#### 9.2) Save Blueprint
   - 9.2.1) Click **Save** button

---

## **PHASE 4A: FORM SWITCH WITH TRANSITION ANIMATION**

This section covers the extended logic when switching TO Crawler from another form via the T wheel. The initial spawn flow (Phase 4) uses bActivateAbilityOnGranted and skips transition animation.

### **1) Branch: Check If First Activation**

#### 1.1) Create Is First Activation Variable
   - 1.1.1) In My Blueprint panel, click **Variables** -> **+**
   - 1.1.2) Name: `bIsFirstActivation`
   - 1.1.3) Type: **Boolean**
   - 1.1.4) Default Value: **True**
   - 1.1.5) Instance Editable: **Unchecked**

#### 1.2) Add Branch After Cast
   - 1.2.1) After **Cast to BP_FatherCompanion** success
   - 1.2.2) Add **Branch** node
   - 1.2.3) Connect **bIsFirstActivation** to **Condition**

#### 1.3) True Path (First Activation - Spawn)
   - 1.3.1) Connect True pin to **Set Current Form** flow
   - 1.3.2) Add **Set bIsFirstActivation** node after End Ability
   - 1.3.3) Set to **False**

#### 1.4) False Path (Form Switch - Transition)
   - 1.4.1) Connect False pin to new Transition section (below)

### **2) Add Transitioning State Tag**

#### 2.1) Get Owning Actor Tag Component
   - 2.1.1) From False branch execution
   - 2.1.2) Drag from **As BP Father Companion**
   - 2.1.3) Search: `Get Ability System Component`
   - 2.1.4) Select node

#### 2.2) Add Loose Gameplay Tag
   - 2.2.1) Drag from **Get Ability System Component** -> **Return Value**
   - 2.2.2) Search: `Add Loose Gameplay Tag`
   - 2.2.3) Select node
   - 2.2.4) In **Gameplay Tag** field, set: `State.Father.Transitioning`

### **3) Apply Invulnerability**

#### 3.1) Add Apply Gameplay Effect to Self
   - 3.1.1) Drag from **Add Loose Gameplay Tag** execution pin
   - 3.1.2) Search: `Apply Gameplay Effect to Self`
   - 3.1.3) Select node
   - 3.1.4) Connect **Get Ability System Component** -> **Return Value** to **Target**
   - 3.1.5) Set **Gameplay Effect Class**: `GE_Invulnerable`

### **4) Spawn Transition VFX**

#### 4.1) Add Spawn System Attached Node
   - 4.1.1) Drag from **Apply GE_Invulnerable** execution pin
   - 4.1.2) Search: `Spawn System Attached`
   - 4.1.3) Select node

#### 4.2) Configure VFX Parameters
   - 4.2.1) **System Template**: Select `NS_FatherFormTransition` (your Niagara system)
   - 4.2.2) **Attach to Component**: Drag from **As BP Father Companion** -> Get **Mesh**
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
   - 6.1.4) Connect **As BP Father Companion** to **Target**

#### 6.2) Configure Detach Parameters
   - 6.2.1) **Location Rule**: `Keep World`
   - 6.2.2) **Rotation Rule**: `Keep World`
   - 6.2.3) **Scale Rule**: `Keep World`

### **7) Move Father Behind Player**

#### 7.1) Get Owner Player
   - 7.1.1) Drag from **As BP Father Companion**
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
   - 7.3.3) Connect **As BP Father Companion** to **Target**
   - 7.3.4) Connect calculated position to **New Location**

### **8) Set Form Variables**

#### 8.1) Set Current Form
   - 8.1.1) Drag from **Set Actor Location** execution pin
   - 8.1.2) Add **Set Current Form** node
   - 8.1.3) Connect **As BP Father Companion** to **Target**
   - 8.1.4) Set **Current Form**: `Crawler`

#### 8.2) Set Is Attached
   - 8.2.1) Drag from **Set Current Form** execution pin
   - 8.2.2) Add **Set Is Attached** node
   - 8.2.3) Connect **As BP Father Companion** to **Target**
   - 8.2.4) Set **Is Attached**: `False` (unchecked)

### **9) Remove Transitioning State**

#### 9.1) Remove Loose Gameplay Tag
   - 9.1.1) Drag from **Set Is Attached** execution pin
   - 9.1.2) Search: `Remove Loose Gameplay Tag`
   - 9.1.3) Select node
   - 9.1.4) Connect ASC to **Target**
   - 9.1.5) Set **Gameplay Tag**: `State.Father.Transitioning`

### **10) Remove Invulnerability**

#### 10.1) Remove Active Gameplay Effect by Source
   - 10.1.1) Drag from **Remove Loose Gameplay Tag** execution pin
   - 10.1.2) Search: `Remove Active Gameplay Effects with Granted Tags`
   - 10.1.3) Select node
   - 10.1.4) Connect ASC to **Target**
   - 10.1.5) Set **Tags**: `State.Invulnerable`

### **11) Apply Form Cooldown**

#### 11.1) Commit Ability Cooldown
   - 11.1.1) Drag from **Remove Invulnerability** execution pin
   - 11.1.2) Search: `Commit Ability Cooldown`
   - 11.1.3) Select **Commit Ability Cooldown** node
   - 11.1.4) No parameters needed - uses CooldownGameplayEffectClass automatically

### **12) End Ability**

#### 12.1) Add End Ability Node
   - 12.1.1) Drag from **Commit Ability Cooldown** execution pin
   - 12.1.2) Search: `End Ability`
   - 12.1.3) Select node
   - 12.1.4) **Was Cancelled**: `False` (unchecked)

### **13) Compile and Save**

#### 14.1) Compile Blueprint
   - 14.1.1) Click **Compile** button

#### 14.2) Save Blueprint
   - 14.2.1) Click **Save** button

---

## **PHASE 4B: ENDABILITY CLEANUP**

Form tags (Father.Form.Crawler, Father.State.Detached) are automatically removed when ability ends via Activation Owned Tags.

### **1) Add Event OnEndAbility**

#### 1.1) Create Event Node
   - 1.1.1) Right-click in Event Graph
   - 1.1.2) Search: `Event OnEndAbility`
   - 1.1.3) Select node

### **2) Cleanup Summary**

#### 2.1) Automatic Cleanup
   - 2.1.1) Activation Owned Tags auto-removed when ability ends
   - 2.1.2) No manual effect removal needed
   - 2.1.3) Crawler form has no stat effects to remove

### **3) Compile and Save**

#### 3.1) Compile Blueprint
   - 3.1.1) Click **Compile** button

#### 3.2) Save Blueprint
   - 3.2.1) Click **Save** button

---

## **PHASE 5: GRANT ABILITY VIA ABILITYCONFIGURATION**

### **1) Ability Granting Architecture**

GA_FatherCrawler is a baseline ability granted via the Narrative Pro AbilityConfiguration system:

| Component | Asset | Purpose |
|-----------|-------|---------|
| NPCDefinition | NPCDef_FatherCompanion | Father NPC configuration |
| AbilityConfiguration | AC_FatherCompanion_Default | Grants baseline abilities on spawn |

### **2) Add GA_FatherCrawler to AbilityConfiguration**

#### 2.1) Open AC_FatherCompanion_Default
   - 2.1.1) In Content Browser, locate **AC_FatherCompanion_Default**
   - 2.1.2) Double-click to open Data Asset Editor

#### 2.2) Add to Default Abilities Array
   - 2.2.1) Find **Default Abilities** array
   - 2.2.2) Click **+** to add new element
   - 2.2.3) Click dropdown on new element
   - 2.2.4) Search for: `GA_FatherCrawler`
   - 2.2.5) Select **GA_FatherCrawler**

### **3) Verify NPCDefinition References AbilityConfiguration**

#### 3.1) Check NPCDef_FatherCompanion
   - 3.1.1) Open **NPCDef_FatherCompanion**
   - 3.1.2) Verify **Ability Configuration** property references **AC_FatherCompanion_Default**

### **4) Auto-Activation on Spawn**

#### 4.1) Activation Flow

| Step | Action |
|------|--------|
| 1 | NPCSpawner spawns father |
| 2 | NPCDefinition loads AbilityConfiguration |
| 3 | GA_FatherCrawler granted to father ASC |
| 4 | bActivateAbilityOnGranted = true triggers OnAvatarSet |
| 5 | OnAvatarSet calls TryActivateAbility automatically |
| 6 | Activation Owned Tags granted (Father.Form.Crawler, Father.State.Detached) |
| 7 | Father starts in Crawler form |

---

## **QUICK REFERENCE**

### **1. Tag Configuration Summary**

| Tag Type | Tag Name | Purpose |
|----------|----------|---------|
| **Ability Tag** | Ability.Father.Crawler | Identifies this ability |
| **Activation Owned** | Father.Form.Crawler | Form state tag |
| **Activation Owned** | Father.State.Detached | Attachment state tag |
| **Activation Required** | Father.State.Alive | Father must be alive |
| **Activation Required** | Father.State.Recruited | Father must be recruited |
| **Activation Blocked** | State.Father.Dormant | Blocked after sacrifice |
| **Activation Blocked** | State.Father.Transitioning | Blocked during 5s transition |
| **Activation Blocked** | State.Father.SymbioteLocked | Blocked during 30s Symbiote |
| **Cancel Tag** | Ability.Father.Armor | Cancels Armor form |
| **Cancel Tag** | Ability.Father.Exoskeleton | Cancels Exoskeleton form |
| **Cancel Tag** | Ability.Father.Symbiote | Cancels Symbiote form |
| **Cancel Tag** | Ability.Father.Engineer | Cancels Engineer form |
| **Cooldown Tag** | Cooldown.Father.FormChange | 15s shared cooldown |

### **2. Ability Configuration Summary**

| Property | Value |
|----------|-------|
| **Parent Class** | NarrativeGameplayAbility |
| **bActivateAbilityOnGranted** | true |
| **Instancing Policy** | Instanced Per Actor |
| **Replication Policy** | Replicate Yes |
| **Net Execution Policy** | Server Only |
| **Input Tag** | None (empty) |
| **Ability Tags** | Ability.Father.Crawler |
| **Activation Required Tags** | Father.State.Alive, Father.State.Recruited |
| **Activation Blocked Tags** | State.Father.Dormant, State.Father.Transitioning, State.Father.SymbioteLocked |
| **Activation Owned Tags** | Father.Form.Crawler, Father.State.Detached |
| **Cancel Abilities with Tag** | Ability.Father.Armor, Ability.Father.Exoskeleton, Ability.Father.Symbiote, Ability.Father.Engineer |

### **3. Variable Configuration**

| Variable | Type | Set To | Purpose |
|----------|------|--------|---------|
| **CurrentForm** | E_FatherForm | Crawler | Tracks active father form |
| **IsAttached** | Boolean | False | Indicates detachment state |
| **bIsFirstActivation** | Boolean | True (then False) | Distinguishes spawn vs form switch |

### **4. Blueprint Node Flow Summary (Initial Spawn)**

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event ActivateAbility | Entry point |
| 2 | Get Avatar Actor From Actor Info | Get actor reference |
| 3 | Cast to BP_FatherCompanion | Type-safe father reference |
| 4 | Branch (bIsFirstActivation) | Check if first activation |
| 5 | Set Current Form (Crawler) | Update form state variable |
| 6 | Set Is Attached (false) | Update attachment state variable |
| 7 | Set bIsFirstActivation (false) | Mark as no longer first activation |
| 8 | End Ability | Complete ability lifecycle |

### **5. Blueprint Node Flow Summary (Form Switch)**

| Step | Node | Purpose |
|------|------|---------|
| 1-4 | Same as Initial Spawn | Get references, check branch |
| 5 | Add Loose Gameplay Tag | Add State.Father.Transitioning |
| 6 | Apply GE_Invulnerable | Father invulnerable during transition |
| 7 | Spawn System Attached | Spawn NS_FatherFormTransition VFX |
| 8 | Delay (5.0s) | Wait for VFX duration |
| 9 | Detach From Actor | Detach father from player |
| 10 | Set Actor Location | Move father behind player |
| 11 | Set Current Form (Crawler) | Update form variable |
| 12 | Set Is Attached (false) | Update attachment variable |
| 13 | Remove Loose Gameplay Tag | Remove State.Father.Transitioning |
| 14 | Remove GE with Tags | Remove GE_Invulnerable |
| 15 | Apply GE_FormChangeCooldown | Apply 15s cooldown |
| 16 | End Ability | Complete ability lifecycle |

### **5A. EndAbility Cleanup Flow**

| Step | Node | Purpose |
|------|------|---------|
| 1 | Event OnEndAbility | Fires when ability ends |
| 2 | Form Tags Auto-Removed | Activation Owned Tags cleanup (automatic) |

### **6. Form State Architecture**

| Component | Description |
|-----------|-------------|
| **Activation Owned Tags** | Father.Form.Crawler, Father.State.Detached (auto-granted) |
| **Replication** | Tags replicate via ReplicateActivationOwnedTags setting |
| **Cleanup** | Tags auto-removed when ability ends |

### **7. Mutual Exclusivity via Cancel Abilities with Tag**

| When GA_FatherCrawler Activates | Effect |
|--------------------------------|--------|
| GA_FatherArmor active | Cancelled (has Ability.Father.Armor) |
| GA_FatherExoskeleton active | Cancelled (has Ability.Father.Exoskeleton) |
| GA_FatherSymbiote active | Cancelled (has Ability.Father.Symbiote) |
| GA_FatherEngineer active | Cancelled (has Ability.Father.Engineer) |

### **8. Narrative Pro Integration Points**

| System | Integration Method | Details |
|--------|-------------------|---------|
| **AbilityConfiguration** | AC_FatherCompanion_Default | Grants GA_FatherCrawler on spawn |
| **NPCDefinition** | NPCDef_FatherCompanion | References AbilityConfiguration |
| **Auto-Activation** | bActivateAbilityOnGranted | Narrative Pro feature for default form |
| **Form State** | Activation Owned Tags | Replicated form tags |

### **9. Multiplayer Configuration**

| Setting | Value | Purpose |
|---------|-------|---------|
| **Replication Policy** | Replicate Yes | Ability replicates |
| **Net Execution Policy** | Server Only | NPC ability executes on server |
| **ReplicateActivationOwnedTags** | Enabled | Tags replicate to all clients |
| **Variable Replication** | CurrentForm (RepNotify), IsAttached (RepNotify) | State variables replicate |

### **10. Responsibility Split**

| Component | Responsibility |
|-----------|----------------|
| **GA_FatherCrawler** | Handle transition animation, apply invulnerability, set variables, apply cooldown, call End Ability |
| **Activation Owned Tags** | Grant Father.Form.Crawler + Father.State.Detached tags |
| **GE_Invulnerable** | Block damage during 5s transition |
| **GE_FormChangeCooldown** | Apply 15s cooldown after transition |
| **Cancel Abilities with Tag** | Cancel other active form abilities |

### **11. Transition Parameters**

| Parameter | Value |
|-----------|-------|
| **VFX Duration** | 5 seconds |
| **Form Cooldown** | 15 seconds |
| **Father Position After Detach** | 200 units behind player |
| **Father Invulnerable During Transition** | Yes |
| **Player Movement During Transition** | Allowed |

### **12. Save System Integration**

For form persistence across save/load, see **BP_FatherCompanion_SaveSystem_Integration_Guide_v1_0.md**.

### **13. Related Documents**

| Document | Purpose |
|----------|---------|
| Father_Companion_Technical_Reference_v6_0.md | Cross-actor patterns, tag architecture, Net Execution Policy |
| Father_Companion_System_Design_Document_v2_0.md | System overview, form transition flow |
| Father_Companion_System_Setup_Guide_v3_0.md | Variable definition, EndPlay cleanup |
| GE_FormChangeCooldown_Implementation_Guide_v1_0.md | Form cooldown effect |
| DefaultGameplayTags_FatherCompanion_v4_0.ini | Complete tag definitions including Father.State.Recruited |

---

## **CHANGELOG**

| Version | Date | Changes |
|---------|------|---------|
| 3.3 | January 2026 | Renamed from Spider to Father throughout entire document. All tags, abilities, references updated. Fixed tag format: State.Father.Alive changed to Father.State.Alive per DefaultGameplayTags. Updated Related Documents to Technical Reference v6.0, Design Document v2.0, and Setup Guide v3.0. Fixed curly quotes to straight ASCII. |
| 3.2 | January 2026 | Simplified PHASE 1 tag creation - replaced detailed step-by-step instructions with simple tag list tables. |
| 3.1 | January 2026 | Built-in cooldown system: Added CooldownGameplayEffectClass property, replaced Apply GE with CommitAbilityCooldown node. GAS auto-blocks during cooldown. |
| 3.0 | January 2026 | Form tags via Activation Owned Tags. ReplicateActivationOwnedTags required. Simplified ability logic and EndAbility cleanup. |
| 2.9 | December 2025 | Net Execution Policy: Server Only for NPC-owned abilities. |
| 2.8 | December 2025 | Added Father.State.Recruited to Activation Required Tags. |
| 2.7 | December 2025 | Added CrawlerStateHandle storage and EndAbility cleanup. |
| 2.6 | December 2025 | Added form transition animation, invulnerability, and cooldown. |
| 2.5 | November 2025 | Initial GE_CrawlerState architecture. |

---

**END OF GUIDE**

**GA_FatherCrawler VERSION 3.3 - Complete**

**Compatible with: Unreal Engine 5.6 + Narrative Pro v2.2**

**Blueprint-Only Implementation - Full Multiplayer Support via Activation Owned Tags**
