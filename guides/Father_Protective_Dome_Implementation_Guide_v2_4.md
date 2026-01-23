# Father Companion - Protective Dome Ability Implementation Guide
## VERSION 2.4 - Full-Only Burst Requirement (Decisions 22-24)

**Document Purpose**: Complete step-by-step guide for implementing the Protective Dome ability for the Father companion in Armor form using GAS attributes created via Gameplay Blueprint Attributes plugin.

**System Overview**: When the father is in Armor form, the player accumulates Dome Energy from incoming damage using an **Energy-Only model** - the player takes full incoming damage (no reduction), while 30% of post-mitigation damage is converted to Dome Energy stored on the Player ASC. When the dome reaches maximum energy (500), the `Father.Dome.FullyCharged` tag is granted, enabling the Q key to trigger a burst dealing 75 flat damage to all enemies within 500 units. The burst can also trigger automatically on form exit (T wheel) when fully charged. After bursting, the ability enters a 12-second cooldown.

---

## **DOCUMENT INFORMATION**

| Field | Value |
|-------|-------|
| Ability Name | GA_ProtectiveDome (passive) + GA_DomeBurst (active/auto) |
| Parent Class | NarrativeGameplayAbility |
| Form | Armor |
| Input | Automatic (passive) / Q Key (manual burst) |
| Version | 2.4 |
| Last Updated | January 2026 |

---

## **TABLE OF CONTENTS**

1. [PHASE 1: Setup and Prerequisites](#phase-1-setup-and-prerequisites)
2. [PHASE 2: Gameplay Tags Setup](#phase-2-gameplay-tags-setup)
3. [PHASE 3: Create Dome Attributes](#phase-3-create-dome-attributes)
4. [PHASE 4: Gameplay Effects - Damage Absorption](#phase-4-gameplay-effects---damage-absorption)
5. [PHASE 5: Gameplay Effects - Dome Burst](#phase-5-gameplay-effects---dome-burst)
6. [PHASE 6: Protective Dome Ability](#phase-6-protective-dome-ability)
7. [PHASE 7: Dome Burst Ability](#phase-7-dome-burst-ability)
8. [PHASE 8: Player Integration](#phase-8-player-integration)
9. [PHASE 9: Visual Effects Setup](#phase-9-visual-effects-setup)
10. [PHASE 10: Input Configuration](#phase-10-input-configuration)

---

## **PHASE 1: SETUP AND PREREQUISITES**

### **1) Verify Prerequisites**

#### **1.1) Check Gameplay Blueprint Attributes Plugin**
1.1.1) Main menu -> Edit -> Plugins
1.1.2) Search: Gameplay Blueprint Attributes
1.1.3) Verify: Plugin is enabled
1.1.4) Close Plugins window

#### **1.2) Check Father Companion Setup**
1.2.1) Verify BP_FatherCompanion exists
1.2.2) Parent class: NarrativeNPCCharacter
1.2.3) Has NarrativeAbilitySystemComponent

#### **1.3) Check Armor Form Ability**
1.3.1) Verify GA_FatherArmor implemented and working
1.3.2) Father can attach to player chest
1.3.3) Armor form activates correctly
1.3.4) GE_ArmorBoost applies properly

#### **1.4) Check Player Setup**
1.4.1) Player character has NarrativeAbilitySystemComponent
1.4.2) Attribute Set assigned: NarrativeAttributeSetBase
1.4.3) InitAbilityActorInfo called in BeginPlay

#### **1.5) Verify Unreal Engine Version**
1.5.1) Unreal Engine 5.6 or later
1.5.2) Narrative Pro Plugin v2.2 enabled

### **2) Create Folder Structure**

#### **2.1) Navigate to Content Browser**
2.1.1) Open Content Browser
2.1.2) Navigate to /Content/FatherCompanion/

#### **2.2) Create ProtectiveDome Folder**
2.2.1) Right-click in FatherCompanion folder
2.2.2) Select: New Folder
2.2.3) Name: ProtectiveDome
2.2.4) Press Enter

#### **2.3) Create Subfolders**
2.3.1) Inside ProtectiveDome, create folders:
   - 2.3.1.1) Attributes - For AS_DomeAttributes
   - 2.3.1.2) Abilities - For GA_ProtectiveDome and GA_DomeBurst
   - 2.3.1.3) Effects - For Gameplay Effects
   - 2.3.1.4) VFX - For visual effects (optional)

---

## **PHASE 2: GAMEPLAY TAGS SETUP**

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Father.Dome.Active | Dome is currently active and absorbing damage |
| Father.Dome.Charging | Dome is accumulating energy from absorbed damage |
| Father.Dome.FullyCharged | Dome has reached maximum energy and is ready to burst |
| Father.Dome.OnCooldown | Dome ability is on cooldown after burst |
| Ability.Father.Armor.ProtectiveDome | Main dome management ability that handles absorption and charging |
| Ability.Father.Armor.DomeBurst | Ability that triggers the AoE explosion when dome is fully charged |
| Father.State.Bursting | Dome burst ability is currently executing |
| Effect.Father.DomeAbsorption | Effect that reduces incoming player damage and stores it |
| Effect.Father.DomeEnergy | Effect that modifies dome energy attribute |
| Effect.Father.DomeBurst | Damage effect applied to enemies in burst radius |
| Cooldown.Father.DomeBurst | Cooldown applied after dome bursts |
| Effect.Father.DomeBurstKnockback | Knockback effect from dome burst explosion |
| Data.Dome.Damage | Data tag for passing damage values via SetByCaller |
| Data.Damage.DomeBurst | Damage type identifier for dome burst |

### **Verify Existing Tags**

| Tag Name | Purpose |
|----------|---------|
| Effect.Father.FormState.Armor | Form identity tag (granted by GE_ArmorState, used for Activation Required) |
| Father.State.Attached | Attachment requirement (Activation Required) |
| Father.State.Recruited | Recruitment requirement (Activation Required) |

---

## **PHASE 3: CREATE DOME ATTRIBUTES**

### **10) Open Gameplay Blueprint Attributes Plugin**

#### **10.1) Access Plugin Interface**
10.1.1) Main menu: Tools -> Gameplay Blueprint Attributes

### **11) Create New Attribute Set**

#### **11.1) Start Attribute Set Creation**
11.1.1) In plugin window, click: Create New Attribute Set button
11.1.2) Or: File -> New Attribute Set

#### **11.2) Configure Attribute Set**
11.2.1) **Attribute Set Name**: AS_DomeAttributes
11.2.2) **Parent Class**: Select AttributeSet from dropdown
11.2.3) Alternate: NarrativeAttributeSetBase if available in dropdown
11.2.4) **Save Location**: Browse to /Content/FatherCompanion/ProtectiveDome/Attributes/
11.2.5) Click: Create button

### **12) Add DomeEnergy Attribute**

#### **12.1) Create New Attribute**
12.1.1) Click: Add Attribute button
12.1.2) Or: Right-click in attribute list -> Add Attribute

#### **12.2) Configure DomeEnergy Properties**
12.2.1) **Attribute Name**: DomeEnergy
12.2.2) **Variable Type**: Float
12.2.3) **Default Value**: 0.0
12.2.4) **Category**: Dome (optional organization)
12.2.5) **Tooltip**: Current dome energy stored from absorbed damage (0-500)

#### **12.3) Configure DomeEnergy Replication**
12.3.1) Find: Replication section in attribute properties
12.3.2) **Replication Mode**: Select RepNotify from dropdown

#### **12.4) Add Meta Tags**
12.4.1) Find: Meta Tags or Meta Data section
12.4.2) Click: Add Meta Tag button
12.4.3) **Meta Tag Name**: NarrativeSaveAttribute

#### **12.5) Configure Clamping**
12.5.1) Find: Min Value field
12.5.2) Set: 0.0 (energy cannot be negative)
12.5.3) Find: Max Value field
12.5.4) Leave empty or set 500.0

### **13) Add MaxDomeEnergy Attribute**

#### **13.1) Create Attribute**
13.1.1) Click: Add Attribute button

#### **13.2) Configure MaxDomeEnergy Properties**
13.2.1) **Attribute Name**: MaxDomeEnergy
13.2.2) **Variable Type**: Float
13.2.3) **Default Value**: 500.0
13.2.4) **Category**: Dome
13.2.5) **Tooltip**: Maximum dome energy capacity

#### **13.3) Configure Replication**
13.3.1) **Replication Mode**: Replicated

#### **13.4) Add Meta Tags**
13.4.1) Click: Add Meta Tag
13.4.2) **Meta Tag Name**: NarrativeSaveAttribute

### **14) Add AbsorptionPercentage Attribute**

#### **14.1) Create Attribute**
14.1.1) Click: Add Attribute button

#### **14.2) Configure AbsorptionPercentage Properties**
14.2.1) **Attribute Name**: AbsorptionPercentage
14.2.2) **Variable Type**: Float
14.2.3) **Default Value**: 0.3 (represents 30%)
14.2.4) **Category**: Dome
14.2.5) **Tooltip**: Percentage of damage absorbed by dome (0.3 = 30%)

#### **14.3) Configure Replication**
14.3.1) **Replication Mode**: Replicated

#### **14.4) Configure Clamping**
14.4.1) **Min Value**: 0.0
14.4.2) **Max Value**: 1.0 (100% max)

### **15) Implement OnRep_DomeEnergy Logic**

#### **15.1) Locate OnRep Function**
15.1.1) In attribute set, find: DomeEnergy attribute
15.1.2) Expand attribute details
15.1.3) Find: OnRep Function or Replication Callback section
15.1.4) Button or link to edit callback logic

#### **15.2) Open OnRep Graph**
15.2.1) Click: Edit OnRep Function or similar button
15.2.2) Event node present: OnRep_DomeEnergy(Old Value)

#### **15.3) Add Fully Charged Check**
15.3.1) From event execution pin:
   - 15.3.1.1) Add: Branch node

15.3.2) For Branch Condition:
   - 15.3.2.1) Add: >= (Greater Than or Equal) node
   - 15.3.2.2) Top input: GET DomeEnergy (current value)
   - 15.3.2.3) Bottom input: GET MaxDomeEnergy
   - 15.3.2.4) Connect result to Branch Condition

15.3.3) From Branch True execution:
   - 15.3.3.1) Get owner actor of this attribute set
   - 15.3.3.2) Add: Get Owner Actor or Get Owning Actor node
   - 15.3.3.3) From owner:
      - 15.3.3.3.a) Add: Add Loose Gameplay Tag node
      - 15.3.3.3.b) Tag to Add: Father.Dome.FullyCharged

15.3.4) From Branch False execution:
   - 15.3.4.1) Get owner actor
   - 15.3.4.2) Add: Remove Loose Gameplay Tag node
   - 15.3.4.3) Tag to Remove: Father.Dome.FullyCharged

### **16) Implement PreAttributeChange (Optional)**

#### **16.1) Locate PreAttributeChange Function**
16.1.1) In attribute set properties
16.1.2) Find: Advanced or Overrides section
16.1.3) Find: PreAttributeChange function

#### **16.2) Add Clamping Logic**
16.2.1) Override PreAttributeChange function
16.2.2) Check which attribute is changing
16.2.3) If DomeEnergy: Clamp between 0 and MaxDomeEnergy
16.2.4) If AbsorptionPercentage: Clamp between 0 and 1

### **17) Save and Compile Attribute Set**

#### **17.1) Save Attribute Set**
17.1.1) Click: Save button in plugin window
17.1.2) Or: File -> Save Attribute Set

#### **17.2) Verify Compilation**
17.2.1) Check for compilation errors in Output Log

#### **17.3) Close Plugin Window**
17.3.1) Click: Close or X button
17.3.2) Attribute set now available in Content Browser

### **18) Add Attribute Set to Player**

#### **18.1) Open Player Character Blueprint**
18.1.1) Navigate to player character blueprint
18.1.2) Example: /Content/Characters/BP_NarrativePlayer
18.1.3) Double-click to open

#### **18.2) Locate Ability System Component**
18.2.1) In Components panel, find: NarrativeAbilitySystemComponent
18.2.2) Select it
18.2.3) Details panel shows ASC properties

#### **18.3) Add Dome Attribute Set**
18.3.1) In Details panel, find: Granted Attributes array
18.3.2) Or: Default Attribute Sets or similar property
18.3.3) Click: + (Plus) button to add element
18.3.4) In dropdown, search: AS_DomeAttributes
18.3.5) Select: AS_DomeAttributes

#### **18.4) Compile and Save**
18.4.1) Click: Compile button
18.4.2) Click: Save button

---

## **PHASE 4: GAMEPLAY EFFECTS - DAMAGE ABSORPTION**

### **19) Create GE_DomeAbsorption Effect**

#### **19.1) Create the Gameplay Effect**
19.1.1) Navigate to: /Content/FatherCompanion/ProtectiveDome/Effects/
19.1.2) Right-click in Content Browser
19.1.3) Select: Blueprint Class
19.1.4) In Pick Parent Class window:
   - 19.1.4.1) Search: GameplayEffect
   - 19.1.4.2) Select: GameplayEffect
   - 19.1.4.3) Click: Select button
19.1.5) Name: GE_DomeAbsorption
19.1.6) Press Enter
19.1.7) Double-click to open

#### **19.2) Configure Duration Policy**
19.2.1) Click: Class Defaults button (top toolbar)
19.2.2) In Details panel, find: Duration Policy dropdown
19.2.3) Select: Infinite

#### **19.3) Add Granted Tags**

##### **19.3.1) Add Target Tags Component**
19.3.1.1) In Details panel, find: Components section
19.3.1.2) Click: + (Plus) button next to Components
19.3.1.3) In dropdown menu, search: Grant Tags
19.3.1.4) Select: Grant Tags to Target Actor

##### **19.3.2) Configure Granted Tags**
19.3.2.1) Click on newly added component in Components list
19.3.2.2) Find: Add Tags section
19.3.2.3) Under Add Tags, find: Add to Inherited field
19.3.2.4) Click: + (Plus) button next to Add to Inherited
19.3.2.5) Search: Father.Dome.Active
19.3.2.6) Select tag
19.3.2.7) Click: + (Plus) button again
19.3.2.8) Search: Father.Dome.Charging
19.3.2.9) Select tag

#### **19.4) Add Asset Tags**

##### **19.4.1) Add Asset Tags Component**
19.4.1.1) In Components section, click: + button
19.4.1.2) Search: Asset Tags
19.4.1.3) Select: Tags This Effect Has (Asset Tags)

##### **19.4.2) Configure Asset Tags**
19.4.2.1) Click on Asset Tags component
19.4.2.2) Find: Asset Tags section
19.4.2.3) Under Asset Tags, find: Add to Inherited
19.4.2.4) Click: + button
19.4.2.5) Search: Effect.Father.DomeAbsorption
19.4.2.6) Select tag

#### **19.5) Compile and Save**
19.5.1) Click: Compile button
19.5.2) Click: Save button

### **20) Create GE_DomeEnergyIncrease Effect**

#### **20.1) Create the Gameplay Effect**
20.1.1) Right-click in Effects folder
20.1.2) Blueprint Class -> GameplayEffect
20.1.3) Name: GE_DomeEnergyIncrease
20.1.4) Double-click to open

#### **20.2) Configure Duration Policy**
20.2.1) Class Defaults button
20.2.2) Duration Policy: Instant

#### **20.3) Add Dome Energy Modifier**

##### **20.3.1) Add Modifier to Array**
20.3.1.1) Find: Modifiers array in Details panel
20.3.1.2) Click: + (Plus) button
20.3.1.3) Expand: Element [0]

##### **20.3.2) Configure Attribute**
20.3.2.1) Find: Attribute dropdown
20.3.2.2) Search: DomeEnergy
20.3.2.3) Select: AS_DomeAttributes.DomeEnergy

##### **20.3.3) Configure Modifier Operation**
20.3.3.1) Find: Modifier Op dropdown
20.3.3.2) Select: Add

##### **20.3.4) Configure Magnitude**
20.3.4.1) Find: Modifier Magnitude section
20.3.4.2) Expand it
20.3.4.3) **Magnitude Calculation Type**: Set By Caller
20.3.4.4) Find: Set By Caller Magnitude subsection
20.3.4.5) **Data Tag**: Data.Dome.Damage

#### **20.4) Add Asset Tags**
20.4.1) Components -> +
20.4.2) Add: Tags This Effect Has (Asset Tags)
20.4.3) Asset Tags -> Add to Inherited -> +
20.4.4) Add tag: Effect.Father.DomeEnergy

#### **20.5) Compile and Save**
20.5.1) Click: Compile
20.5.2) Click: Save

### **21) Create GE_DomeInitialize Effect**

#### **21.1) Create the Gameplay Effect**
21.1.1) Right-click in Effects folder
21.1.2) Blueprint Class -> GameplayEffect
21.1.3) Name: GE_DomeInitialize
21.1.4) Double-click to open

#### **21.2) Configure Duration**
21.2.1) Class Defaults button
21.2.2) Duration Policy: Instant

#### **21.3) Add MaxDomeEnergy Modifier**

##### **21.3.1) Add Modifier**
21.3.1.1) Modifiers array -> + button
21.3.1.2) Expand Element [0]

##### **21.3.2) Configure Modifier**
21.3.2.1) Attribute: AS_DomeAttributes.MaxDomeEnergy
21.3.2.2) Modifier Op: Override
21.3.2.3) Magnitude Calculation Type: Scalable Float
21.3.2.4) Value: 500.0

#### **21.4) Add AbsorptionPercentage Modifier**

##### **21.4.1) Add Second Modifier**
21.4.1.1) Modifiers array -> + button
21.4.1.2) Expand Element [1]

##### **21.4.2) Configure Modifier**
21.4.2.1) Attribute: AS_DomeAttributes.AbsorptionPercentage
21.4.2.2) Modifier Op: Override
21.4.2.3) Magnitude Calculation Type: Scalable Float
21.4.2.4) Value: 0.3

#### **21.5) Compile and Save**
21.5.1) Click: Compile
21.5.2) Click: Save

---

## **PHASE 5: GAMEPLAY EFFECTS - DOME BURST**

### **22) Create GE_DomeBurstDamage Effect**

#### **22.1) Create the Gameplay Effect**
22.1.1) Right-click in Effects folder
22.1.2) Blueprint Class -> GameplayEffect
22.1.3) Name: GE_DomeBurstDamage
22.1.4) Double-click to open

#### **22.2) Configure Duration**
22.2.1) Class Defaults button
22.2.2) Duration Policy: Instant

#### **22.3) Add Damage Execution**

##### **22.3.1) Add Execution**
22.3.1.1) Find: Executions array
22.3.1.2) Click: + button
22.3.1.3) Expand Element [0]

##### **22.3.2) Configure Execution Class**
22.3.2.1) Calculation Class: NarrativeDamageExecCalc

##### **22.3.3) Add Calculation Modifier**
22.3.3.1) In execution element, find: Calculation Modifiers array
22.3.3.2) Click: + button
22.3.3.3) Expand Element [0]

##### **22.3.4) Configure Backing Attribute**
22.3.4.1) Backing Attribute section -> Expand
22.3.4.2) Attribute: NarrativeAttributeSetBase.AttackDamage
22.3.4.3) Attribute Calculation Type: Attribute Base Value
22.3.4.4) Attribute Source: Source

##### **22.3.5) Configure Magnitude**
22.3.5.1) Magnitude Calculation Type: Set By Caller
22.3.5.2) Set By Caller Data Tag: Data.Damage.DomeBurst

#### **22.4) Add Asset Tags**
22.4.1) Components -> +
22.4.2) Add: Tags This Effect Has (Asset Tags)
22.4.3) Asset Tags -> Add to Inherited -> +
22.4.4) Add tag: Effect.Father.DomeBurst

#### **22.5) Compile and Save**
22.5.1) Click: Compile
22.5.2) Click: Save

### **23) Create GE_DomeBurstCooldown Effect**

#### **23.1) Create the Gameplay Effect**
23.1.1) Right-click in Effects folder
23.1.2) Blueprint Class -> GameplayEffect
23.1.3) Name: GE_DomeBurstCooldown
23.1.4) Double-click to open

#### **23.2) Configure Duration**
23.2.1) Class Defaults button
23.2.2) Duration Policy: Has Duration
23.2.3) Duration Magnitude -> Scalable Float -> Value: 12.0

#### **23.3) Add Granted Tags**
23.3.1) Components -> +
23.3.2) Add: Grant Tags to Target Actor
23.3.3) Add Tags -> Add to Inherited -> +
23.3.4) Add tag: Father.Dome.OnCooldown

#### **23.4) Add Asset Tags**
23.4.1) Components -> +
23.4.2) Add: Tags This Effect Has (Asset Tags)
23.4.3) Asset Tags -> Add to Inherited -> +
23.4.4) Add tag: Cooldown.Father.DomeBurst

#### **23.5) Compile and Save**
23.5.1) Click: Compile
23.5.2) Click: Save

### **24) Create GE_DomeEnergyReset Effect**

#### **24.1) Create the Gameplay Effect**
24.1.1) Right-click in Effects folder
24.1.2) Blueprint Class -> GameplayEffect
24.1.3) Name: GE_DomeEnergyReset
24.1.4) Double-click to open

#### **24.2) Configure Duration**
24.2.1) Class Defaults button
24.2.2) Duration Policy: Instant

#### **24.3) Add Energy Reset Modifier**
24.3.1) Modifiers array -> +
24.3.2) Expand Element [0]
24.3.3) Attribute: AS_DomeAttributes.DomeEnergy
24.3.4) Modifier Op: Override
24.3.5) Magnitude Calculation Type: Scalable Float
24.3.6) Value: 0.0

#### **24.4) Compile and Save**
24.4.1) Click: Compile
24.4.2) Click: Save

---

## **PHASE 6: PROTECTIVE DOME ABILITY**

### **25) Create GA_ProtectiveDome Ability**

#### **25.1) Create the Ability Blueprint**
25.1.1) Navigate to: /Content/FatherCompanion/ProtectiveDome/Abilities/
25.1.2) Right-click
25.1.3) Blueprint Class
25.1.4) Parent: NarrativeGameplayAbility
25.1.5) Name: GA_ProtectiveDome
25.1.6) Double-click to open

#### **25.2) Configure Class Defaults**
25.2.1) Click: Class Defaults button

##### **25.2.2) Ability Tags**
25.2.2.1) Ability Tags -> +
25.2.2.2) Add: Ability.Father.Armor.ProtectiveDome

##### **25.2.3) Activation Required Tags**
25.2.3.1) Activation Required Tags -> +
25.2.3.2) Element [0]: Effect.Father.FormState.Armor
25.2.3.3) Click: +
25.2.3.4) Element [1]: Father.State.Attached
25.2.3.5) Click: +
25.2.3.6) Element [2]: Father.State.Recruited

##### **25.2.4) Activation Blocked Tags**
25.2.4.1) Activation Blocked Tags -> +
25.2.4.2) Add: Father.Dome.OnCooldown

##### **25.2.5) Activation Owned Tags**
25.2.5.1) Activation Owned Tags -> +
25.2.5.2) Add: Ability.Father.Armor.ProtectiveDome.Active

##### **25.2.6) Network Settings**
25.2.6.1) Net Execution Policy: Local Predicted
25.2.6.2) Instancing Policy: Instanced Per Actor
25.2.6.3) Replication Policy: Replicate

#### **25.3) Create Ability Variables**

##### **25.3.1) PlayerRef Variable**
25.3.1.1) Variables -> +
25.3.1.2) Name: PlayerRef
25.3.1.3) Type: Actor -> Object Reference

##### **25.3.2) FatherRef Variable**
25.3.2.1) Variables -> +
25.3.2.2) Name: FatherRef
25.3.2.3) Type: BP_FatherCompanion -> Object Reference

##### **25.3.3) AbsorptionHandle Variable**
25.3.3.1) Variables -> +
25.3.3.2) Name: AbsorptionEffectHandle
25.3.3.3) Type: FActiveGameplayEffectHandle

##### **25.3.4) PlayerASC Variable**
25.3.4.1) Variables -> +
25.3.4.2) Name: PlayerASC
25.3.4.3) Type: NarrativeAbilitySystemComponent -> Object Reference

##### **25.3.5) IsMonitoring Variable**
25.3.5.1) Variables -> +
25.3.5.2) Name: IsMonitoring
25.3.5.3) Type: Boolean
25.3.5.4) Default Value: false

##### **25.3.6) HealthChangeDelegateHandle Variable**
25.3.6.1) Variables -> +
25.3.6.2) Name: HealthChangeDelegateHandle
25.3.6.3) Type: FDelegateHandle

#### **25.4) Implement ActivateAbility Event**

##### **25.4.1) Add Event ActivateAbility**
25.4.1.1) Event Graph
25.4.1.2) Right-click -> Event ActivateAbility

##### **25.4.2) Get Avatar Actor**
25.4.2.1) From event execution:
   - 25.4.2.1.1) Add: Get Avatar Actor From Actor Info node

##### **25.4.3) Cast to Player Character**
25.4.3.1) From Avatar Actor Return Value:
   - 25.4.3.1.1) Add: Cast To your player class

##### **25.4.4) Store Player Reference**
25.4.4.1) From successful cast:
   - 25.4.4.1.1) Add: Set PlayerRef node
25.4.4.2) Connect cast output to input

##### **25.4.5) Get Father Reference**
25.4.5.1) From Set PlayerRef execution:
   - 25.4.5.1.1) Drag from PlayerRef output
   - 25.4.5.1.2) Add: Get Father Companion Ref node

##### **25.4.6) Store Father Reference**
25.4.6.1) From Get Father execution:
   - 25.4.6.1.1) Add: Set FatherRef node

##### **25.4.7) Validate References**
25.4.7.1) From Set FatherRef:
   - 25.4.7.1.1) Add: Branch node
25.4.7.2) Condition: PlayerRef AND FatherRef both valid
25.4.7.3) False: End Ability (Was Cancelled: Checked)

##### **25.4.8) Get Player ASC**
25.4.8.1) From Branch True:
   - 25.4.8.1.1) Drag from PlayerRef
   - 25.4.8.1.2) Add: Get Ability System Component

##### **25.4.9) Cast to NarrativeAbilitySystemComponent**
25.4.9.1) From Get ASC Return Value:
   - 25.4.9.1.1) Add: Cast To NarrativeAbilitySystemComponent
25.4.9.2) From successful cast:
   - 25.4.9.2.1) Add: Set PlayerASC node
   - 25.4.9.2.2) Connect As Narrative Ability System Component to value input

##### **25.4.10) Initialize Dome Attributes**
25.4.10.1) From Set PlayerASC execution:
   - 25.4.10.1.1) Drag from PlayerASC variable
   - 25.4.10.1.2) Add: Make Outgoing Spec node
   - 25.4.10.1.3) Gameplay Effect Class: GE_DomeInitialize
   - 25.4.10.1.4) Level: 1.0

25.4.10.2) Apply initialization:
   - 25.4.10.2.1) Drag from PlayerASC
   - 25.4.10.2.2) Add: Apply Gameplay Effect Spec To Self
   - 25.4.10.2.3) Spec Handle: Connect Make Outgoing Spec output

##### **25.4.11) Apply Absorption Effect**
25.4.11.1) From Apply GE Spec execution:
   - 25.4.11.1.1) Drag from PlayerASC
   - 25.4.11.1.2) Add: Make Outgoing Spec node
   - 25.4.11.1.3) Gameplay Effect Class: GE_DomeAbsorption
   - 25.4.11.1.4) Level: 1.0

25.4.11.2) Apply absorption:
   - 25.4.11.2.1) Drag from PlayerASC
   - 25.4.11.2.2) Add: Apply Gameplay Effect Spec To Self
   - 25.4.11.2.3) Spec Handle: Connect output

25.4.11.3) Store handle:
   - 25.4.11.3.1) From Apply GE Spec:
   - 25.4.11.3.2) Add: Set AbsorptionEffectHandle
   - 25.4.11.3.3) Connect Return Value

##### **25.4.12) Bind Health Change Delegate**
25.4.12.1) From Set AbsorptionEffectHandle execution:
   - 25.4.12.1.1) Drag from PlayerASC variable
   - 25.4.12.1.2) Add: Get Gameplay Attribute Value Change Delegate node
25.4.12.2) Configure Attribute parameter:
   - 25.4.12.2.1) Click dropdown
   - 25.4.12.2.2) Expand: NarrativeAttributeSetBase
   - 25.4.12.2.3) Select: Health

##### **25.4.13) Create Event Binding**
25.4.13.1) From Get Gameplay Attribute Value Change Delegate Return Value:
   - 25.4.13.1.1) Drag outward
   - 25.4.13.1.2) Search: Bind Event
   - 25.4.13.1.3) Add: Bind Event to On Gameplay Attribute Value Change node
25.4.13.2) From Event (red) pin:
   - 25.4.13.2.1) Drag outward
   - 25.4.13.2.2) Search: Create Event
   - 25.4.13.2.3) Select: Create Matching Function
   - 25.4.13.2.4) Name function: OnHealthChanged

##### **25.4.14) Store Delegate Handle**
25.4.14.1) From Bind Event execution:
   - 25.4.14.1.1) Add: Set HealthChangeDelegateHandle node
   - 25.4.14.1.2) Connect Bind Event Return Value to input

##### **25.4.15) Set Monitoring Flag**
25.4.15.1) From Set HealthChangeDelegateHandle:
   - 25.4.15.1.1) Add: Set IsMonitoring node
   - 25.4.15.1.2) Value: Checked (true)

##### **25.4.16) Commit Ability**
25.4.16.1) From Set IsMonitoring:
   - 25.4.16.1.1) Add: Commit Ability node

#### **25.5) Implement OnHealthChanged Function**

##### **25.5.1) Open OnHealthChanged Function**
25.5.1.1) In My Blueprint panel, find: Functions section
25.5.1.2) Double-click: OnHealthChanged function
25.5.1.3) Function graph opens with FOnAttributeChangeData parameter

##### **25.5.2) Check if Monitoring Active**
25.5.2.1) From function entry node:
   - 25.5.2.1.1) Drag IsMonitoring variable getter into graph
25.5.2.2) Add: Branch node
25.5.2.3) Connect IsMonitoring to Condition
25.5.2.4) False path: Return (do nothing)

##### **25.5.3) Extract Old and New Values**
25.5.3.1) From Branch True:
   - 25.5.3.1.1) From function input parameter (FOnAttributeChangeData):
   - 25.5.3.1.2) Add: Break FOnAttributeChangeData node
   - 25.5.3.1.3) This exposes: OldValue, NewValue

##### **25.5.4) Check if Damage Occurred**
25.5.4.1) Add: < (Less Than) node
   - 25.5.4.1.1) A: NewValue
   - 25.5.4.1.2) B: OldValue
25.5.4.2) Add: Branch node
25.5.4.3) Condition: NewValue < OldValue (damage occurred)
25.5.4.4) False path: Return (health increased or unchanged)

##### **25.5.5) Calculate Damage Amount**
25.5.5.1) From Branch True:
   - 25.5.5.1.1) Add: - (Subtract) node
   - 25.5.5.1.2) OldValue - NewValue = Damage taken

##### **25.5.6) Calculate Absorbed Damage**
25.5.6.1) Get absorption percentage:
   - 25.5.6.1.1) Drag PlayerASC variable
   - 25.5.6.1.2) Add: Get Numeric Attribute node
   - 25.5.6.1.3) Attribute: AS_DomeAttributes.AbsorptionPercentage

25.5.6.2) Calculate absorbed:
   - 25.5.6.2.1) Add: * (Multiply) node
   - 25.5.6.2.2) Damage * AbsorptionPercentage

##### **25.5.7) Add to Dome Energy**
25.5.7.1) From multiply result:
   - 25.5.7.1.1) Drag from PlayerASC
   - 25.5.7.1.2) Add: Make Outgoing Spec node
   - 25.5.7.1.3) Gameplay Effect Class: GE_DomeEnergyIncrease
   - 25.5.7.1.4) Level: 1.0

25.5.7.2) Set absorbed amount:
   - 25.5.7.2.1) From Make Outgoing Spec Return Value:
   - 25.5.7.2.2) Add: Assign Tag Set By Caller Magnitude node
   - 25.5.7.2.3) Data Tag: Data.Dome.Damage
   - 25.5.7.2.4) Magnitude: Absorbed damage value

25.5.7.3) Apply effect:
   - 25.5.7.3.1) Drag from PlayerASC
   - 25.5.7.3.2) Add: Apply Gameplay Effect Spec To Self
   - 25.5.7.3.3) Spec Handle: Connect output

##### **25.5.8) Check for Auto-Burst**
25.5.8.1) From Apply GE execution:
   - 25.5.8.1.1) Drag from PlayerASC
   - 25.5.8.1.2) Add: Get Numeric Attribute node
   - 25.5.8.1.3) Attribute: AS_DomeAttributes.DomeEnergy
25.5.8.2) Get max energy:
   - 25.5.8.2.1) Drag from PlayerASC
   - 25.5.8.2.2) Add: Get Numeric Attribute node
   - 25.5.8.2.3) Attribute: AS_DomeAttributes.MaxDomeEnergy
25.5.8.3) Add: >= comparison node
25.5.8.4) Add: Branch node

25.5.8.5) From Branch True (energy full):
   - 25.5.8.5.1) Drag from PlayerASC
   - 25.5.8.5.2) Add: Try Activate Ability by Class node
   - 25.5.8.5.3) Ability: GA_DomeBurst
   - 25.5.8.5.4) Allow Remote Activation: Checked

#### **25.6) Implement EndAbility Event**

##### **25.6.1) Add Event EndAbility**
25.6.1.1) Right-click -> Event EndAbility

##### **25.6.2) Set Monitoring Flag False**
25.6.2.1) From event:
   - 25.6.2.1.1) Add: Set IsMonitoring node
   - 25.6.2.1.2) Value: Unchecked (false)

##### **25.6.3) Validate PlayerASC**
25.6.3.1) From Set IsMonitoring:
   - 25.6.3.1.1) Add: Is Valid node
   - 25.6.3.1.2) Input: GET PlayerASC
25.6.3.2) Add: Branch node
25.6.3.3) Connect Is Valid to Condition
25.6.3.4) False path: Skip to section 25.6.6 (Clear References)

##### **25.6.4) Unbind Health Delegate Using Handle**
25.6.4.1) From Branch True:
   - 25.6.4.1.1) Drag from PlayerASC
   - 25.6.4.1.2) Add: Get Gameplay Attribute Value Change Delegate node
   - 25.6.4.1.3) Attribute: NarrativeAttributeSetBase.Health
25.6.4.2) From delegate:
   - 25.6.4.2.1) Add: Unbind Event from On Gameplay Attribute Value Change node
   - 25.6.4.2.2) Handle input: GET HealthChangeDelegateHandle

##### **25.6.5) Remove Absorption Effect Using Handle**
25.6.5.1) From Unbind Event:
   - 25.6.5.1.1) Add: Is Valid node
   - 25.6.5.1.2) Input: GET AbsorptionEffectHandle
25.6.5.2) Add: Branch node
25.6.5.3) From Branch True:
   - 25.6.5.3.1) Drag from PlayerASC
   - 25.6.5.3.2) Add: Remove Active Gameplay Effect node
   - 25.6.5.3.3) Active Handle: GET AbsorptionEffectHandle

##### **25.6.6) Clear References**
25.6.6.1) Set PlayerRef: None
25.6.6.2) Set FatherRef: None
25.6.6.3) Set PlayerASC: None

#### **25.7) Implement Event OnEndAbility**

##### **25.7.1) Add Event OnEndAbility**
25.7.1.1) Event Graph
25.7.1.2) Right-click -> Add Event -> Event OnEndAbility

##### **25.7.2) Validate PlayerASC for Cleanup**
25.7.2.1) From event:
   - 25.7.2.1.1) Add: Is Valid node
   - 25.7.2.1.2) Input: GET PlayerASC
25.7.2.2) Add: Branch node
25.7.2.3) False path: Skip cleanup (already cleaned or invalid)

##### **25.7.3) Check AbsorptionEffectHandle Valid**
25.7.3.1) From Branch True:
   - 25.7.3.1.1) Add: Is Valid node
   - 25.7.3.1.2) Input: GET AbsorptionEffectHandle
25.7.3.2) Add: Branch node

##### **25.7.4) Remove Absorption Effect if Still Active**
25.7.4.1) From Branch True:
   - 25.7.4.1.1) Drag from PlayerASC
   - 25.7.4.1.2) Add: Remove Active Gameplay Effect node
   - 25.7.4.1.3) Active Handle: GET AbsorptionEffectHandle

##### **25.7.5) Clear AbsorptionEffectHandle**
25.7.5.1) Add: Set AbsorptionEffectHandle node
25.7.5.2) Leave input disconnected (sets to invalid/null)

#### **25.8) Compile and Save**
25.8.1) Click: Compile
25.8.2) Verify: No errors
25.8.3) Click: Save

---

## **PHASE 7: DOME BURST ABILITY**

### **26) Create GA_DomeBurst Ability**

#### **26.1) Create the Ability Blueprint**
26.1.1) Navigate to Abilities folder
26.1.2) Right-click -> Blueprint Class
26.1.3) Parent: NarrativeGameplayAbility
26.1.4) Name: GA_DomeBurst
26.1.5) Double-click to open

#### **26.2) Configure Class Defaults**
26.2.1) Class Defaults button

##### **26.2.2) Ability Tags**
26.2.2.1) Ability Tags -> +
26.2.2.2) Add: Ability.Father.DomeBurst

##### **26.2.3) Activation Required Tags**
26.2.3.1) Activation Required Tags -> +
26.2.3.2) Element [0]: Father.Dome.Active
26.2.3.3) Click: +
26.2.3.4) Element [1]: Father.Dome.FullyCharged

**Note:** Equipment granting (EI_FatherArmorForm) ensures ability only available in Armor form. FullyCharged tag gates activation to FULL energy only.

##### **26.2.4) Activation Owned Tags**
26.2.4.1) Activation Owned Tags -> +
26.2.4.2) Add: Father.State.Attacking

##### **26.2.5) Activation Blocked Tags**
26.2.5.1) Activation Blocked Tags -> +
26.2.5.2) Add: Cooldown.Father.DomeBurst

##### **26.2.6) Cooldown Effect**
26.2.6.1) Cooldown Gameplay Effect Class: GE_DomeBurstCooldown

##### **26.2.7) InputTag Configuration**
26.2.7.1) Find: InputTag property
26.2.7.2) Set: Narrative.Input.Father.Ability1

##### **26.2.8) Network Settings**
26.2.8.1) Net Execution Policy: Local Predicted
26.2.8.2) Instancing Policy: Instanced Per Actor
26.2.8.3) Replication Policy: Replicate

#### **26.3) Create Ability Variables**

##### **26.3.1) BurstDamage Variable**
26.3.1.1) Variables -> +
26.3.1.2) Name: BurstDamage
26.3.1.3) Type: Float
26.3.1.4) Default: 75.0
26.3.1.5) Instance Editable: Checked

**Note:** Damage is flat 75, does not scale with energy. BaseDamage and DamageScaling variables removed in v2.4.

##### **26.3.2) BurstRadius Variable**
26.3.2.1) Variables -> +
26.3.2.2) Name: BurstRadius
26.3.2.3) Type: Float
26.3.2.4) Default: 500.0
26.3.2.5) Instance Editable: Checked

##### **26.3.3) KnockbackForce Variable**
26.3.3.1) Variables -> +
26.3.3.2) Name: KnockbackForce
26.3.3.3) Type: Float
26.3.3.4) Default: 1000.0
26.3.3.5) Instance Editable: Checked

**Note:** MinEnergyForManual removed in v2.4 - burst only available when FULL (Father.Dome.FullyCharged tag gates activation).

##### **26.3.4) PlayerRef Variable**
26.3.4.1) Variables -> +
26.3.4.2) Name: PlayerRef
26.3.4.3) Type: Actor -> Object Reference

##### **26.3.5) PlayerASC Variable**
26.3.5.1) Variables -> +
26.3.5.2) Name: PlayerASC
26.3.5.3) Type: NarrativeAbilitySystemComponent -> Object Reference

#### **26.4) Implement ActivateAbility Event**

##### **26.4.1) Add Event ActivateAbility**
26.4.1.1) Event Graph
26.4.1.2) Right-click -> Event ActivateAbility

##### **26.4.2) Get Player Reference**
26.4.2.1) From event:
   - 26.4.2.1.1) Add: Get Avatar Actor From Actor Info
26.4.2.2) Cast to player character
26.4.2.3) Store in PlayerRef variable

##### **26.4.2A) Validate PlayerRef**
26.4.2A.1) From Set PlayerRef:
   - 26.4.2A.1.1) Add: Is Valid node
   - 26.4.2A.1.2) Input: GET PlayerRef
26.4.2A.2) Add: Branch node
26.4.2A.3) False path: End Ability (Was Cancelled: Checked)

##### **26.4.2B) Get and Store PlayerASC**
26.4.2B.1) From Branch True:
   - 26.4.2B.1.1) Drag from PlayerRef
   - 26.4.2B.1.2) Add: Get Ability System Component
26.4.2B.2) Cast to NarrativeAbilitySystemComponent
26.4.2B.3) Add: Set PlayerASC node

##### **26.4.2C) Validate PlayerASC**
26.4.2C.1) From Set PlayerASC:
   - 26.4.2C.1.1) Add: Is Valid node
   - 26.4.2C.1.2) Input: GET PlayerASC
26.4.2C.2) Add: Branch node
26.4.2C.3) False path: End Ability (Was Cancelled: Checked)

##### **26.4.3) Get Current Dome Energy**
26.4.3.1) From Branch True:
   - 26.4.3.1.1) Drag from PlayerASC
   - 26.4.3.1.2) Add: Get Numeric Attribute node
   - 26.4.3.1.3) Attribute: AS_DomeAttributes.DomeEnergy
   - 26.4.3.1.4) Store in CurrentDomeEnergy variable

##### **26.4.4) Validate Minimum Energy**
26.4.4.1) Add: >= comparison node
   - 26.4.4.1.1) A: CurrentDomeEnergy
   - 26.4.4.1.2) B: MinEnergyForManual
26.4.4.2) Add: Branch node
26.4.4.3) False path: End Ability (Was Cancelled: Checked)

##### **26.4.5) Calculate Burst Damage**
26.4.5.1) From Branch True:
   - 26.4.5.1.1) Add: * (Multiply) node
   - 26.4.5.1.2) CurrentDomeEnergy * DamageScaling
26.4.5.2) Add: + (Add) node
   - 26.4.5.2.1) BaseDamage + (multiply result)
26.4.5.3) Store result in CalculatedDamage

##### **26.4.6) Get Burst Location**
26.4.6.1) From player cast:
   - 26.4.6.1.1) Add: Get Actor Location node

##### **26.4.7) Find Enemies in Radius**
26.4.7.1) From Get Location:
   - 26.4.7.1.1) Add: Sphere Overlap Actors node
   - 26.4.7.1.2) Sphere Pos: Player location
   - 26.4.7.1.3) Sphere Radius: GET BurstRadius
   - 26.4.7.1.4) Object Types: Pawn
   - 26.4.7.1.5) Actor Class Filter: NarrativeNPCCharacter

##### **26.4.8) Iterate Through Hit Actors**
26.4.8.1) From Out Actors:
   - 26.4.8.1.1) Add: ForEach Loop node

##### **26.4.9) Apply Damage to Each Enemy**
26.4.9.1) From Loop Body:
   - 26.4.9.1.1) Array Element -> Get ASC
   - 26.4.9.1.2) Check valid (Branch)
   - 26.4.9.1.3) From Branch True:
      - 26.4.9.1.3.a) Add: Make Outgoing Spec node
      - 26.4.9.1.3.b) Gameplay Effect Class: GE_DomeBurstDamage
      - 26.4.9.1.3.c) Level: 1.0
   - 26.4.9.1.4) Add: Assign Tag Set By Caller Magnitude node
      - 26.4.9.1.4.a) Data Tag: Data.Damage.DomeBurst
      - 26.4.9.1.4.b) Magnitude: GET CalculatedDamage
   - 26.4.9.1.5) Apply effect to enemy ASC

##### **26.4.10) Apply Knockback**
26.4.10.1) Calculate direction:
   - 26.4.10.1.1) Enemy Location - Player Location
   - 26.4.10.1.2) Add: Normalize node
   - 26.4.10.1.3) Add: * (Multiply) node
   - 26.4.10.1.4) Multiply by KnockbackForce

26.4.10.2) Launch enemy:
   - 26.4.10.2.1) Add: Launch Character node
   - 26.4.10.2.2) Launch Velocity: Calculated vector
   - 26.4.10.2.3) XY Override: Checked
   - 26.4.10.2.4) Z Override: Checked

##### **26.4.11) After Loop Completes**
26.4.11.1) Reset dome energy:
   - 26.4.11.1.1) Player ASC -> Apply GE_DomeEnergyReset

26.4.11.2) Remove fully charged tag:
   - 26.4.11.2.1) Get father reference from player
   - 26.4.11.2.2) Father -> Remove Loose Tag Father.Dome.FullyCharged

##### **26.4.12) Spawn Burst VFX**
26.4.12.1) Add: Spawn Emitter at Location (optional)
26.4.12.2) Location: Player location

##### **26.4.13) Commit and End**
26.4.13.1) Add: Commit Ability
26.4.13.2) Add: End Ability

#### **26.5) Compile and Save**
26.5.1) Click: Compile
26.5.2) Click: Save

---

## **PHASE 8: PLAYER INTEGRATION**

### **27) Modify GA_FatherArmor**

#### **27.1) Open GA_FatherArmor**
27.1.1) Navigate to father abilities
27.1.2) Double-click: GA_FatherArmor

#### **27.2) Locate Armor Activation Logic**
27.2.1) Find where GE_ArmorBoost applied
27.2.2) Find where form set to Armor

#### **27.3) Grant Protective Dome to Player**

##### **27.3.1) After Armor Boost Applied**
27.3.1.1) Get OwnerPlayer reference
27.3.1.2) Get Player ASC
27.3.1.3) Add: Give Ability node
27.3.1.4) Ability: GA_ProtectiveDome
27.3.1.5) Level: 1
27.3.1.6) Input ID: -1

##### **27.3.2) Store ProtectiveDomeHandle**
27.3.2.1) From Give Ability Return Value:
   - 27.3.2.1.1) Add: Set ProtectiveDomeHandle node

##### **27.3.3) Activate Protective Dome**
27.3.3.1) From Set Handle execution:
   - 27.3.3.1.1) Player ASC -> Try Activate Ability by Class
   - 27.3.3.1.2) Ability: GA_ProtectiveDome
   - 27.3.3.1.3) Allow Remote Activation: Checked

##### **27.3.4) Give Dome Burst Ability**
27.3.4.1) From Try Activate execution:
   - 27.3.4.1.1) Player ASC -> Give Ability
   - 27.3.4.1.2) Ability: GA_DomeBurst
   - 27.3.4.1.3) Level: 1
   - 27.3.4.1.4) Input ID: -1

##### **27.3.5) Store DomeBurstHandle**
27.3.5.1) From Give Ability Return Value:
   - 27.3.5.1.1) Add: Set DomeBurstHandle node

#### **27.4) Compile and Save**
27.4.1) Click: Compile
27.4.2) Click: Save

### **28) Form Cancellation Integration**

#### **28.1) Automatic Cleanup via EndAbility**
28.1.1) GA_FatherArmor EndAbility removes all armor-related effects
28.1.2) EndAbility triggers when Cancel Abilities with Tag fires during form switch
28.1.3) Includes: GE_DomeAbsorption, GE_ArmorBoost, and any other armor effects
28.1.4) Two-step cleanup for granted abilities (Cancel Ability then Set Remove Ability On End)

#### **28.2) Ability Cancellation via Tags**
28.2.1) Form abilities use Cancel Abilities with Tag in Class Defaults:
   - 28.2.1.1) GA_FatherCrawler cancels: Ability.Father.Armor, Ability.Father.Exoskeleton, Ability.Father.Symbiote, Ability.Father.Engineer
   - 28.2.1.2) GA_FatherArmor cancels: Ability.Father.Crawler, Ability.Father.Exoskeleton, Ability.Father.Symbiote, Ability.Father.Engineer
28.2.2) When GA_FatherArmor is cancelled, its EndAbility event fires
28.2.3) GA_ProtectiveDome EndAbility cleans up its own timer and references

#### **28.3) Handle-Based Cleanup**
28.3.1) GA_FatherArmor EndAbility uses stored handles for cleanup
28.3.2) ProtectiveDomeHandle and DomeBurstHandle enable proper ability removal
28.3.3) Two-step cleanup pattern: Cancel Ability first, then Set Remove Ability On End

---

## **PHASE 9: VISUAL EFFECTS SETUP**

### **29) Create Dome Shield VFX (Optional)**

#### **29.1) Create Niagara System**
29.1.1) Navigate to VFX folder
29.1.2) Right-click -> FX -> Niagara System
29.1.3) Template: Empty or Sphere
29.1.4) Name: NS_DomeShield

#### **29.2) Configure Shield Appearance**
29.2.1) Particle emitter: Spherical
29.2.2) Radius: Match burst radius (500 units)
29.2.3) Material: Translucent energy
29.2.4) Color: Blue/cyan
29.2.5) Opacity: 0.2-0.3

#### **29.3) Save System**
29.3.1) Click: Save

### **30) Create Dome Burst VFX (Optional)**

#### **30.1) Create Niagara System**
30.1.1) Right-click in VFX folder
30.1.2) FX -> Niagara System
30.1.3) Template: Explosion
30.1.4) Name: NS_DomeBurst

#### **30.2) Configure Burst Effect**
30.2.1) Explosion radius: 500 units
30.2.2) Bright energy burst
30.2.3) Shockwave
30.2.4) Duration: 1-2 seconds

#### **30.3) Save**
30.3.1) Click: Save

### **31) Add VFX to Abilities (Optional)**

#### **31.1) Add Shield VFX to GA_ProtectiveDome**
31.1.1) Open GA_ProtectiveDome
31.1.2) Create variable: ShieldVFXComponent
31.1.3) In ActivateAbility: Spawn NS_DomeShield attached to player
31.1.4) In EndAbility: Destroy VFX component

#### **31.2) Burst VFX Already in GA_DomeBurst**
31.2.1) Step 26.4.12 already spawns burst VFX
31.2.2) Assign NS_DomeBurst to that spawn node

---

## **PHASE 10: INPUT CONFIGURATION**

### **32) Verify Input Action Exists**

#### **32.1) Navigate to Input Folder**
32.1.1) In Content Browser, go to /Content/Input/ or your input assets location

#### **32.2) Verify IA_FatherAbility1 Exists**
32.2.1) Look for IA_FatherAbility1 Input Action asset
32.2.2) This should be the Q key action

#### **32.3) Create Input Action (if needed)**
32.3.1) Right-click in Content Browser
32.3.2) Select: Input -> Input Action
32.3.3) Name: IA_FatherAbility1
32.3.4) Press Enter

#### **32.4) Configure Input Action**
32.4.1) Double-click to open IA_FatherAbility1
32.4.2) Set **Value Type**: Digital (Bool)
32.4.3) Set **Triggers**: Add Pressed trigger
32.4.4) Save asset

### **33) Add to Input Mapping Context**

#### **33.1) Open Input Mapping Context**
33.1.1) Locate your Input Mapping Context asset (IMC)
33.1.2) Double-click to open

#### **33.2) Add Q Key Mapping**
33.2.1) Find or add mapping for IA_FatherAbility1
33.2.2) Set Key: Q
33.2.3) Save IMC

### **34) Configure Narrative Input Tag Binding**

#### **34.1) Open Narrative Input Settings**
34.1.1) Navigate to Narrative Pro settings data asset

#### **34.2) Add Input Tag Binding**
34.2.1) Find Input Tag Bindings array
34.2.2) Add entry:
   - 34.2.2.1) **Input Action**: IA_FatherAbility1
   - 34.2.2.2) **Gameplay Tag**: Narrative.Input.Father.Ability1
34.2.3) Save data asset

### **35) Save All Assets**

#### **35.1) Save Configuration**
35.1.1) Save all modified input assets
35.1.2) Save Narrative settings

---

## **VARIABLE SUMMARY TABLES**

### **GA_ProtectiveDome Variables**

| Variable | Type | Default | Instance Editable | Replicated |
|----------|------|---------|-------------------|------------|
| PlayerRef | Actor Reference | None | No | No |
| FatherRef | BP_FatherCompanion Reference | None | No | No |
| AbsorptionEffectHandle | FActiveGameplayEffectHandle | None | No | No |
| PlayerASC | NarrativeAbilitySystemComponent Reference | None | No | No |
| IsMonitoring | Boolean | false | No | No |
| HealthChangeDelegateHandle | FDelegateHandle | None | No | No |

### **GA_DomeBurst Variables**

| Variable | Type | Default | Instance Editable | Replicated |
|----------|------|---------|-------------------|------------|
| BurstDamage | Float | 75.0 | Yes | No |
| BurstRadius | Float | 500.0 | Yes | No |
| KnockbackForce | Float | 1000.0 | Yes | No |
| PlayerRef | Actor Reference | None | No | No |
| PlayerASC | NarrativeAbilitySystemComponent Reference | None | No | No |

### **Damage Reference (v2.4)**

| Parameter | Value | Notes |
|-----------|-------|-------|
| Damage | 75 (flat) | Does not scale with energy |
| Trigger | FULL only | Father.Dome.FullyCharged required |
| Energy Required | 500 | No partial release |

---

## **TAG CONFIGURATION SUMMARY**

### **GA_ProtectiveDome Tags**

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Armor.ProtectiveDome |
| Activation Required | Effect.Father.FormState.Armor, Father.State.Attached, Father.State.Recruited |
| Activation Blocked | Father.Dome.OnCooldown |
| Activation Owned | Ability.Father.Armor.ProtectiveDome.Active |

### **GA_DomeBurst Tags**

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.DomeBurst |
| Activation Required | Father.Dome.Active, Father.Dome.FullyCharged |
| Activation Owned | Father.State.Attacking |
| Activation Blocked | Cooldown.Father.DomeBurst |
| InputTag | Narrative.Input.Father.Ability1 |

---

## **GAMEPLAY EFFECT SUMMARY**

| Effect | Duration | Purpose |
|--------|----------|---------|
| GE_DomeAbsorption | Infinite | Grants active/charging tags |
| GE_DomeEnergyIncrease | Instant | Adds energy via SetByCaller |
| GE_DomeInitialize | Instant | Sets initial attribute values |
| GE_DomeBurstDamage | Instant | Damage via SetByCaller |
| GE_DomeBurstCooldown | 12 seconds | Grants cooldown tags |
| GE_DomeEnergyReset | Instant | Resets DomeEnergy to 0 |

---

## **QUICK REFERENCE**

### **Handle Variables**

| Ability | Handle Variable | Type | Purpose |
|---------|-----------------|------|---------|
| GA_ProtectiveDome | AbsorptionEffectHandle | FActiveGameplayEffectHandle | GE_DomeAbsorption removal |
| GA_ProtectiveDome | HealthChangeDelegateHandle | FDelegateHandle | Delegate unbinding |
| GA_FatherArmor | ProtectiveDomeHandle | FGameplayAbilitySpecHandle | GA_ProtectiveDome cleanup |
| GA_FatherArmor | DomeBurstHandle | FGameplayAbilitySpecHandle | GA_DomeBurst cleanup |

### **EndAbility Cleanup Flow**

| Step | Action | Target |
|------|--------|--------|
| 1 | Set IsMonitoring = false | Local flag |
| 2 | Validate PlayerASC | Early exit if invalid |
| 3 | Unbind Health Delegate | Using HealthChangeDelegateHandle |
| 4 | Validate AbsorptionEffectHandle | Check before removal |
| 5 | Remove GE_DomeAbsorption | Using AbsorptionEffectHandle |
| 6 | Clear references | PlayerRef, FatherRef, PlayerASC |

### **Related Documents**

| Document | Version | Purpose |
|----------|---------|---------|
| Father_Companion_Technical_Reference | v4.5 | Handle patterns, recruited tag gating |
| Father_Companion_System_Setup_Guide | v1.3 | BP_FatherCompanion setup |
| DefaultGameplayTags_FatherCompanion | v3.5 | Complete tag list |
| GA_FatherArmor_Implementation_Guide | v3.4 | Form ability with dome integration |
| GA_DomeBurst_Implementation_Guide | v2.10 | Separate burst ability guide |

---

## **INTEGRATION NOTES**

### **Effect Cleanup Architecture**

Form abilities use Cancel Abilities with Tag for mutual exclusion. When a new form activates:
- Cancel Abilities with Tag cancels the previous form ability
- The cancelled ability's EndAbility removes all effects it applied
- Uses handle-based removal for GE_DomeAbsorption via AbsorptionEffectHandle
- Uses handle-based unbinding for health delegate via HealthChangeDelegateHandle
- Provides consistent cleanup across all forms (Crawler, Armor, Exoskeleton, Symbiote, Engineer)

### **Ability Granting Pattern (v2.4)**

Dome abilities are granted via EI_FatherArmorForm equipment:
- EI_FatherArmorForm.abilities_to_grant includes GA_ProtectiveDome and GA_DomeBurst
- Equipment system handles automatic grant/remove on equip/unequip
- No manual handle tracking in GA_FatherArmor required

### **Form Exit Burst (Decisions 22-24)**

When player switches forms via T wheel with full dome energy:
1. Player selects new form from T wheel
2. EI_FatherArmorForm.HandleUnequip override fires
3. If Father.Dome.FullyCharged AND NOT Cooldown.Father.DomeBurst:
   - TryActivateAbilityByClass(GA_DomeBurst) triggers burst
4. Parent HandleUnequip called (removes abilities including GA_DomeBurst)
5. New form's equipment item is equipped

**Note:** Burst during form exit is blocked if on cooldown. Energy resets to 0 regardless.

### **Form Cancellation Chain (Legacy)**

Legacy pattern for reference - now handled via equipment system:
1. New form ability activates (e.g., GA_FatherCrawler)
2. Equipment system unequips EI_FatherArmorForm
3. HandleUnequip fires form exit burst if charged
4. Abilities removed by equipment system
5. GA_ProtectiveDome EndAbility fires, cleaning up dome state

### **Damage Detection Method**

This guide uses delegate-based damage detection via Get Gameplay Attribute Value Change Delegate bound to the Health attribute. This approach:
- Fires only when Health attribute actually changes (zero overhead when idle)
- Provides OldValue and NewValue directly (no manual tracking needed)
- Is the standard GAS pattern for attribute monitoring
- Matches the pattern established in GA_FatherSacrifice
- Uses HealthChangeDelegateHandle for proper cleanup

### **Consistency with Other Forms**

This pattern matches the v4.5 architecture where:
- Form abilities use Cancel Abilities with Tag for mutual exclusion
- Father.State.Recruited required in Activation Required Tags
- Effect handles stored immediately after application
- Handle-based cleanup in EndAbility and Event OnEndAbility
- Each form ability's EndAbility is responsible for cleaning up its own effects

---

## **OPEN ITEMS FOR FUTURE DECISION**

The following items require architectural decisions that may affect multiple guides. They are documented here for future resolution.

### **Item 1: GA_FatherArmor Handle Variable (RESOLVED)**

**Issue**: GA_FatherArmor_Implementation_Guide_v2.0 creates ArmorEffectHandle variable to store the GE_ArmorBoost effect handle.

**Resolution**: v4.5 architecture uses handle-based cleanup for all effects. GA_FatherArmor stores handles for ArmorStateHandle, ArmorBoostHandle, ProtectiveDomeHandle, and DomeBurstHandle.

---

### **Item 2: Ability Granting Method (RESOLVED)**

**Issue**: Two patterns exist in the codebase for granting form-specific abilities.

**Resolution**: v4.5 architecture uses Give Ability with handle storage. Handles enable two-step cleanup (Cancel Ability then Set Remove Ability On End).

---

### **Item 3: GA_DomeBurst Activation Required Tags**

**Issue**: Should GA_DomeBurst require Father.Dome.FullyCharged (auto-burst only) or Father.Form.Armor + Father.Dome.Active (manual + auto with code validation)?

**Current Implementation**: Uses Father.Form.Armor + Father.Dome.Active + Father.State.Recruited with MinEnergyForManual check in code.

**Options**:
- A) Keep current (allows manual burst with minimum energy check)
- B) Change to Father.Dome.FullyCharged only (auto-burst only, no manual)

---

### **Item 4: DomeEnergy Attribute Location**

**Issue**: DomeEnergy attribute is on Player ASC (AS_DomeAttributes added to player). Alternative would be Father ASC.

**Current Implementation**: Player ASC (makes sense since dome protects player and absorbs player damage).

**Options**:
- A) Keep on Player ASC (current)
- B) Move to Father ASC (would require cross-ASC attribute access)

---

### **Item 5: GA_ProtectiveDome Activation Owned Tag**

**Issue**: Current implementation uses Ability.Father.Armor.ProtectiveDome.Active as Activation Owned Tag. GE_DomeAbsorption separately grants Father.Dome.Active.

**Options**:
- A) Keep ability-specific tag (Ability.Father.Armor.ProtectiveDome.Active)
- B) Use system-wide state tag (Father.Dome.Active)
- C) Use both tags

---

## CHANGELOG

### Version 2.4 - January 2026 (Decisions 22-24)

| Change | Description |
|--------|-------------|
| Decision 22 | Form exit burst via TryActivateAbilityByClass(GA_DomeBurst) |
| Decision 23 | EI_FatherArmorForm.HandleUnequip override triggers form exit burst |
| Decision 24 | Father.Dome.FullyCharged added to GA_DomeBurst activation_required_tags |
| Damage Model | Energy-Only model - player takes full damage, 30% converts to energy |
| Manual Release | FULL (500) only, no minimum 50 partial release |
| Auto-burst | DISABLED - no automatic threshold burst |
| Burst Damage | Changed from 150+scaling to flat 75 |
| Cooldown | Changed from 20s to 12s |
| Knockback | Changed from 1500 to 1000 units |
| GA_DomeBurst Tags | Updated to flat hierarchy per manifest.yaml |
| Variables | Removed BaseDamage, DamageScaling, MinEnergyForManual, CurrentDomeEnergy, CalculatedDamage |
| Equipment Granting | Abilities now granted via EI_FatherArmorForm, not GA_FatherArmor |
| Form Exit Burst | New section documenting HandleUnequip burst trigger |

---

### Version 2.3 - January 2026 (INV-1 Compliant)

| Change | Description |
|--------|-------------|
| Form State Tags | Changed `Father.Form.Armor` to `Effect.Father.FormState.Armor` in all Activation Required Tags |
| INV-1 Compliance | Aligned with Father_Companion_GAS_Audit_Locked_Decisions.md form state architecture |
| Tag Architecture | `Father.Form.*` tags are orphan tags; form identity now uses GE-granted `Effect.Father.FormState.*` tags |

---

### Version 2.2 - January 2026

| Change | Description |
|--------|-------------|
| Narrative Pro | Updated version reference from v2.1 to v2.2 |
| Documentation | Aligned with Technical Reference v5.13 and Setup Guide v2.3 |

---

### Version 2.1 - January 2026

| Change | Description |
|--------|-------------|
| PHASE 2 Simplified | Replaced 7 detailed subsections (sections 3-9) with consolidated tag tables |
| Tag Creation Format | Tags now listed in Create Required Tags and Verify Existing Tags tables |
| Line Reduction | Reduced PHASE 2 from 111 lines to 27 lines |

---

### Version 2.0 - January 2026

| Change | Description |
|--------|-------------|
| Tag Consistency | Changed GA_DomeBurst tag from Ability.Father.DomeBurst (flat) to Ability.Father.Armor.DomeBurst (hierarchical) |
| Tech Reference | Updated to align with Technical Reference v5.4 Section 11.12 hierarchical tag pattern |
| Form Cancellation | Hierarchical tag enables automatic cancellation when Armor form ends via Cancel Abilities with Tag |

---

### Version 1.9 - December 2025

| Change | Description |
|--------|-------------|
| Section 25.2.3 | Added Father.State.Recruited to GA_ProtectiveDome Activation Required Tags (3 elements total) |
| Section 25.3.6 | Added HealthChangeDelegateHandle variable (FDelegateHandle) |
| Section 25.4.14 | Store HealthChangeDelegateHandle after binding delegate |
| Section 25.6 | Enhanced EndAbility with handle-based delegate unbinding and effect removal |
| Section 25.7 | NEW - Event OnEndAbility for early cancellation cleanup |
| Section 26.2.3 | Added Father.State.Recruited to GA_DomeBurst Activation Required Tags (3 elements total) |
| Section 26.3.9 | Added PlayerASC variable for validation |
| Section 26.4.2A-C | Added PlayerRef and PlayerASC validation before operations |
| Section 27.3 | Updated to include handle storage for granted abilities |
| Section 28 | Updated to describe handle-based cleanup pattern |
| Variable Summary | Added HealthChangeDelegateHandle to GA_ProtectiveDome, PlayerASC to GA_DomeBurst |
| Tag Summary | Updated Activation Required to include Father.State.Recruited |
| Quick Reference | NEW - Added Handle Variables table and EndAbility Cleanup Flow |
| Integration Notes | Updated to describe handle-based cleanup architecture |
| Item 1 and 2 | Marked as resolved with v4.5 architecture |
| Related Documents | Updated to reference v4.5/v3.5/v1.3 versions |

---

### Version 1.8 - December 2025

| Change | Description |
|--------|-------------|
| Section 28 | Renamed from "GA_FatherDetach Integration" to "Form Cancellation Integration" |
| Effect Cleanup | Updated to EndAbility pattern (GA_FatherDetach removed from system) |
| Form Cancellation | Updated chain to describe T wheel direct form switching |
| Consistency | Updated to reference v4.0 architecture |
| Item 1 | Marked as resolved |

---

### Version 1.7 - December 2025

| Change | Description |
|--------|-------------|
| Ability Tag | Changed Ability.Father.ProtectiveDome to Ability.Father.Armor.ProtectiveDome |
| Cooldown Tag | Changed Cooldown.Father.ProtectiveDome to Cooldown.Father.Armor.ProtectiveDome |
| Tag Hierarchy | Enables automatic cancellation when Armor form ends |

### Version 1.6 - November 2025

| Change | Description |
|--------|-------------|
| Initial Release | Complete implementation with Gameplay Blueprint Attributes plugin |

---

**END OF IMPLEMENTATION GUIDE**

**VERSION 2.4 - Full-Only Burst Requirement (Decisions 22-24)**

**Compatible with Unreal Engine 5.7 + Narrative Pro v2.2**

**Blueprint-Only Implementation - Full Multiplayer Support**
