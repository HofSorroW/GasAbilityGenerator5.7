# WBP_UltimatePanel - Ultimate Abilities HUD Implementation Guide
## Father Companion with Gameplay Ability System
## For Unreal Engine 5.6 + Narrative Pro Plugin v2.2

**Version:** 1.1
**Date:** January 2026
**Engine:** Unreal Engine 5.6
**Plugin:** Narrative Pro v2.2
**Implementation:** Blueprint Only
**Widget Type:** User Widget (UMG)

---

## **DOCUMENT INFORMATION**

| Field | Value |
|-------|-------|
| Document Type | Widget and Component Implementation Guide |
| Widget Name | WBP_UltimatePanel |
| Component Name | UltimateChargeComponent |
| Curve Asset | FC_UltimateThreshold |
| Enum Asset | EFatherUltimate |
| Version | 1.1 |
| Last Updated | January 2026 |

---

## **TABLE OF CONTENTS**

1. PHASE 1: Create Ultimate Enumeration
2. PHASE 2: Create Threshold Float Curve
3. PHASE 3: Create UltimateChargeComponent
4. PHASE 4: Create WBP_UltimateIcon Widget
5. PHASE 5: Create WBP_UltimatePanel Widget
6. PHASE 6: Player Character Integration
7. PHASE 7: HUD Integration

---

## **INTRODUCTION**

### **System Overview**

The Ultimate Panel HUD system displays father ultimate abilities when the shared charge pool reaches threshold. The panel remains hidden during charging to maintain gameplay tension, appearing only when ultimates are ready for activation. Players then select which unlocked ultimate to activate from the arc-layout display.

### **Key Features**

| Feature | Description |
|---------|-------------|
| Hidden By Default | Panel invisible during charging for tension |
| Appears When Ready | Shows only when charge threshold reached |
| Shared Charge Pool | Single charge value for all ultimates |
| Level-Based Threshold | FC_UltimateThreshold curve scales requirement |
| Selection Interface | Player chooses which ultimate to activate |
| 4 Ultimate Icons | Symbiote, Sword, Rifle, Bow positioned in 90-degree arc |
| Glow Animation | Ready state pulsing effect |

### **Panel Visibility States**

| State | Panel Visible | Description |
|-------|---------------|-------------|
| Charging | NO | Hidden - no progress shown |
| Ready | YES | Panel appears with unlocked icons |
| Selection | YES | Player picks ultimate |
| After Use | NO | Panel hides, cooldown begins |
| Cooldown | NO | Hidden - no indication |

### **Threshold Curve Values**

| Player Level | Threshold |
|--------------|-----------|
| 1 | 5000 |
| 10 | 10000 |
| 25 | 20000 |
| 50 | 40000 |
| 100 | 80000 |

### **Charge Efficiency**

| Damage Source | Efficiency |
|---------------|------------|
| Player Damage | 100% |
| Father Damage | 50% |

---

## **QUICK REFERENCE**

### **Assets Created**

| Asset | Type | Location |
|-------|------|----------|
| EFatherUltimate | Enumeration | /Content/FatherCompanion/Data/ |
| FC_UltimateThreshold | CurveFloat | /Content/FatherCompanion/Data/ |
| UltimateChargeComponent | Actor Component | /Content/FatherCompanion/Components/ |
| WBP_UltimateIcon | Widget Blueprint | /Content/FatherCompanion/UI/ |
| WBP_UltimatePanel | Widget Blueprint | /Content/FatherCompanion/UI/ |

### **UltimateChargeComponent Variables**

| Variable | Type | Default | Replicated |
|----------|------|---------|------------|
| CurrentCharge | Float | 0.0 | Yes |
| ThresholdCurve | UCurveFloat | FC_UltimateThreshold | No |
| EquippedUltimate | EFatherUltimate | Symbiote | Yes |
| UnlockedUltimates | TArray EFatherUltimate | [Symbiote] | No |
| FatherDamageEfficiency | Float | 0.5 | No |
| CooldownDuration | Float | 120.0 | No |
| bIsOnCooldown | Boolean | False | Yes |

### **UltimateChargeComponent Functions**

| Function | Return | Purpose |
|----------|--------|---------|
| AddCharge | Void | Adds damage to charge pool |
| IsChargeReady | Boolean | Checks if threshold reached |
| ResetCharge | Void | Sets CurrentCharge to 0 |
| StartCooldown | Void | Begins cooldown timer |
| GetThreshold | Float | Gets level-scaled threshold |
| IsUltimateUnlocked | Boolean | Checks unlock status |

### **Event Dispatchers**

| Event | Purpose | Used by UI |
|-------|---------|------------|
| OnChargeChanged | Notify charge updated | No (hidden) |
| OnUltimateReady | Show panel, set icons ready | Yes |
| OnUltimateUsed | Hide panel | Yes |
| OnCooldownComplete | Re-enable charging | No (hidden) |

### **Placeholder Textures**

| Texture | Purpose |
|---------|---------|
| T_UltimateIcon_Symbiote | Symbiote icon |
| T_UltimateIcon_Sword | Sword icon |
| T_UltimateIcon_Rifle | Rifle icon |
| T_UltimateIcon_Bow | Bow icon |

---

## **PHASE 1: CREATE ULTIMATE ENUMERATION**

### **1) Navigate to Data Folder**

#### 1.1) Open Content Browser
   - 1.1.1) Navigate to /Content/FatherCompanion/
   - 1.1.2) Create folder if needed: Data
   - 1.1.3) Double-click Data folder to open

### **2) Create Enumeration Asset**

#### 2.1) Create New Enumeration
   - 2.1.1) Right-click in Content Browser
   - 2.1.2) Select Blueprints -> Enumeration
   - 2.1.3) Name: EFatherUltimate
   - 2.1.4) Press Enter
   - 2.1.5) Double-click EFatherUltimate to open

#### 2.2) Add Enumeration Values
   - 2.2.1) Click Add Enumerator button
   - 2.2.2) Name: Symbiote
   - 2.2.3) Click Add Enumerator button
   - 2.2.4) Name: Sword
   - 2.2.5) Click Add Enumerator button
   - 2.2.6) Name: Rifle
   - 2.2.7) Click Add Enumerator button
   - 2.2.8) Name: Bow

### **3) Compile and Save**
   - 3.1) Click Save
   - 3.2) Close Enumeration Editor

---

## **PHASE 2: CREATE THRESHOLD FLOAT CURVE**

### **4) Create Float Curve Asset**

#### 4.1) Create New Curve
   - 4.1.1) Right-click in Data folder
   - 4.1.2) Select Miscellaneous -> Curve
   - 4.1.3) Select CurveFloat
   - 4.1.4) Name: FC_UltimateThreshold
   - 4.1.5) Press Enter
   - 4.1.6) Double-click FC_UltimateThreshold to open

#### 4.2) Add Curve Keys
   - 4.2.1) Right-click in curve graph
   - 4.2.2) Add Key
   - 4.2.3) Set Time: 1, Value: 5000
   - 4.2.4) Right-click in curve graph
   - 4.2.5) Add Key
   - 4.2.6) Set Time: 10, Value: 10000
   - 4.2.7) Right-click in curve graph
   - 4.2.8) Add Key
   - 4.2.9) Set Time: 25, Value: 20000
   - 4.2.10) Right-click in curve graph
   - 4.2.11) Add Key
   - 4.2.12) Set Time: 50, Value: 40000
   - 4.2.13) Right-click in curve graph
   - 4.2.14) Add Key
   - 4.2.15) Set Time: 100, Value: 80000

#### 4.3) Configure Curve Interpolation
   - 4.3.1) Select all keys (Ctrl+A)
   - 4.3.2) Right-click on key
   - 4.3.3) Set Interpolation: Cubic (Auto)

### **5) Save and Close**
   - 5.1) Click Save
   - 5.2) Close Curve Editor

---

## **PHASE 3: CREATE ULTIMATECHARGECOMPONENT**

### **6) Create Component Blueprint**

#### 6.1) Navigate to Components Folder
   - 6.1.1) Navigate to /Content/FatherCompanion/
   - 6.1.2) Create folder if needed: Components
   - 6.1.3) Double-click Components folder

#### 6.2) Create Actor Component
   - 6.2.1) Right-click in Content Browser
   - 6.2.2) Select Blueprint Class
   - 6.2.3) Expand All Classes
   - 6.2.4) Search: ActorComponent
   - 6.2.5) Select ActorComponent
   - 6.2.6) Name: UltimateChargeComponent
   - 6.2.7) Press Enter
   - 6.2.8) Double-click to open

### **7) Create Component Variables**

#### 7.1) Create CurrentCharge Variable
   - 7.1.1) In My Blueprint panel, click + next to Variables
   - 7.1.2) Name: CurrentCharge
   - 7.1.3) Variable Type: Float
   - 7.1.4) Default Value: 0.0
   - 7.1.5) Replication: RepNotify
   - 7.1.6) Category: Charge

#### 7.2) Create ThresholdCurve Variable
   - 7.2.1) Click + next to Variables
   - 7.2.2) Name: ThresholdCurve
   - 7.2.3) Variable Type: CurveFloat (Object Reference)
   - 7.2.4) Instance Editable: Check
   - 7.2.5) Default Value: FC_UltimateThreshold
   - 7.2.6) Category: Settings

#### 7.3) Create EquippedUltimate Variable
   - 7.3.1) Click + next to Variables
   - 7.3.2) Name: EquippedUltimate
   - 7.3.3) Variable Type: EFatherUltimate (Enum)
   - 7.3.4) Default Value: Symbiote
   - 7.3.5) Replication: Replicated
   - 7.3.6) Category: Settings

#### 7.4) Create UnlockedUltimates Variable
   - 7.4.1) Click + next to Variables
   - 7.4.2) Name: UnlockedUltimates
   - 7.4.3) Variable Type: EFatherUltimate (Array)
   - 7.4.4) Instance Editable: Check
   - 7.4.5) Default Value: Add Symbiote to array
   - 7.4.6) Category: Settings

#### 7.5) Create FatherDamageEfficiency Variable
   - 7.5.1) Click + next to Variables
   - 7.5.2) Name: FatherDamageEfficiency
   - 7.5.3) Variable Type: Float
   - 7.5.4) Default Value: 0.5
   - 7.5.5) Instance Editable: Check
   - 7.5.6) Category: Settings

#### 7.6) Create CooldownDuration Variable
   - 7.6.1) Click + next to Variables
   - 7.6.2) Name: CooldownDuration
   - 7.6.3) Variable Type: Float
   - 7.6.4) Default Value: 120.0
   - 7.6.5) Instance Editable: Check
   - 7.6.6) Category: Settings

#### 7.7) Create bIsOnCooldown Variable
   - 7.7.1) Click + next to Variables
   - 7.7.2) Name: bIsOnCooldown
   - 7.7.3) Variable Type: Boolean
   - 7.7.4) Default Value: False
   - 7.7.5) Replication: Replicated
   - 7.7.6) Category: State

#### 7.8) Create CooldownTimerHandle Variable
   - 7.8.1) Click + next to Variables
   - 7.8.2) Name: CooldownTimerHandle
   - 7.8.3) Variable Type: Timer Handle (Struct)
   - 7.8.4) Category: Internal

### **8) Create Event Dispatchers**

#### 8.1) Create OnChargeChanged Dispatcher
   - 8.1.1) In My Blueprint, click + next to Event Dispatchers
   - 8.1.2) Name: OnChargeChanged
   - 8.1.3) Click on OnChargeChanged
   - 8.1.4) In Details, add input: NewCharge (Float)
   - 8.1.5) Add input: Threshold (Float)

#### 8.2) Create OnUltimateReady Dispatcher
   - 8.2.1) Click + next to Event Dispatchers
   - 8.2.2) Name: OnUltimateReady
   - 8.2.3) Add input: UltimateType (EFatherUltimate)

#### 8.3) Create OnUltimateUsed Dispatcher
   - 8.3.1) Click + next to Event Dispatchers
   - 8.3.2) Name: OnUltimateUsed
   - 8.3.3) Add input: UltimateType (EFatherUltimate)

#### 8.4) Create OnCooldownComplete Dispatcher
   - 8.4.1) Click + next to Event Dispatchers
   - 8.4.2) Name: OnCooldownComplete

### **9) Implement OnRep_CurrentCharge**

#### 9.1) Locate Rep Notify Function
   - 9.1.1) In My Blueprint -> Functions
   - 9.1.2) Find: OnRep_CurrentCharge (auto-generated)
   - 9.1.3) Double-click to open

#### 9.2) Implement Notification
   - 9.2.1) From function entry:
   - 9.2.1.1) Add Call OnChargeChanged node
   - 9.2.1.2) New Charge: GET CurrentCharge
   - 9.2.1.3) Threshold: Call GetThreshold function

### **10) Create GetThreshold Function**

#### 10.1) Add New Function
   - 10.1.1) In My Blueprint -> Functions -> Click +
   - 10.1.2) Name: GetThreshold
   - 10.1.3) Add output: ReturnValue (Float)
   - 10.1.4) Set Pure: Check

#### 10.2) Implement Threshold Lookup
   - 10.2.1) From function entry:
   - 10.2.1.1) Add Is Valid node
   - 10.2.1.2) Input Object: GET ThresholdCurve
   - 10.2.2) From Is Valid -> Is Valid:
   - 10.2.2.1) Add Get Owning Actor node
   - 10.2.2.2) Add Cast to Character node
   - 10.2.2.3) Target: Owning Actor return value
   - 10.2.3) From Cast successful:
   - 10.2.3.1) Add Get Ability System Component node
   - 10.2.3.2) From ASC -> Add Get Numeric Attribute node
   - 10.2.3.3) Attribute: Level (or your level attribute)
   - 10.2.4) From Get Numeric Attribute:
   - 10.2.4.1) Drag GET ThresholdCurve
   - 10.2.4.2) Add Get Float Value node
   - 10.2.4.3) In Time: Level value
   - 10.2.5) Return: Float Value result
   - 10.2.6) From Is Valid -> Is Not Valid:
   - 10.2.6.1) Return: 5000.0 (fallback)

### **11) Create AddCharge Function**

#### 11.1) Add New Function
   - 11.1.1) In My Blueprint -> Functions -> Click +
   - 11.1.2) Name: AddCharge
   - 11.1.3) Add input: DamageDealt (Float)
   - 11.1.4) Add input: bIsFatherDamage (Boolean)

#### 11.2) Implement Charge Addition
   - 11.2.1) From function entry:
   - 11.2.1.1) Add Branch node
   - 11.2.1.2) Condition: GET bIsOnCooldown
   - 11.2.2) From Branch False:
   - 11.2.2.1) Add Select Float node
   - 11.2.2.2) Index: bIsFatherDamage
   - 11.2.2.3) A (False): 1.0
   - 11.2.2.4) B (True): GET FatherDamageEfficiency
   - 11.2.3) From Select:
   - 11.2.3.1) Add Multiply (Float) node
   - 11.2.3.2) A: DamageDealt
   - 11.2.3.3) B: Select result
   - 11.2.4) From Multiply:
   - 11.2.4.1) Add Add (Float) node
   - 11.2.4.2) A: GET CurrentCharge
   - 11.2.4.3) B: Multiply result
   - 11.2.5) From Add:
   - 11.2.5.1) Add SET CurrentCharge node
   - 11.2.5.2) Value: Add result
   - 11.2.6) From SET CurrentCharge:
   - 11.2.6.1) Add Call IsChargeReady node
   - 11.2.6.2) Add Branch node
   - 11.2.6.3) Condition: IsChargeReady result
   - 11.2.7) From Branch True:
   - 11.2.7.1) Add Call OnUltimateReady node
   - 11.2.7.2) Ultimate Type: GET EquippedUltimate

### **12) Create IsChargeReady Function**

#### 12.1) Add New Function
   - 12.1.1) In My Blueprint -> Functions -> Click +
   - 12.1.2) Name: IsChargeReady
   - 12.1.3) Add output: ReturnValue (Boolean)
   - 12.1.4) Set Pure: Check

#### 12.2) Implement Ready Check
   - 12.2.1) From function entry:
   - 12.2.1.1) Add And (Boolean) node
   - 12.2.2) First condition:
   - 12.2.2.1) Add >= (Float) node
   - 12.2.2.2) A: GET CurrentCharge
   - 12.2.2.3) B: Call GetThreshold
   - 12.2.3) Second condition:
   - 12.2.3.1) Add NOT node
   - 12.2.3.2) Input: GET bIsOnCooldown
   - 12.2.4) Connect both to And node
   - 12.2.5) Return: And result

### **13) Create ResetCharge Function**

#### 13.1) Add New Function
   - 13.1.1) In My Blueprint -> Functions -> Click +
   - 13.1.2) Name: ResetCharge

#### 13.2) Implement Reset
   - 13.2.1) From function entry:
   - 13.2.1.1) Add SET CurrentCharge node
   - 13.2.1.2) Value: 0.0
   - 13.2.2) From SET CurrentCharge:
   - 13.2.2.1) Add Call OnUltimateUsed node
   - 13.2.2.2) Ultimate Type: GET EquippedUltimate

### **14) Create StartCooldown Function**

#### 14.1) Add New Function
   - 14.1.1) In My Blueprint -> Functions -> Click +
   - 14.1.2) Name: StartCooldown

#### 14.2) Implement Cooldown Timer
   - 14.2.1) From function entry:
   - 14.2.1.1) Add SET bIsOnCooldown node
   - 14.2.1.2) Value: True (Check)
   - 14.2.2) From SET bIsOnCooldown:
   - 14.2.2.1) Add Set Timer by Function Name node
   - 14.2.2.2) Function Name: OnCooldownEnd
   - 14.2.2.3) Time: GET CooldownDuration
   - 14.2.2.4) Looping: Uncheck
   - 14.2.3) From Set Timer:
   - 14.2.3.1) Add SET CooldownTimerHandle node
   - 14.2.3.2) Value: Timer Handle return

### **15) Create OnCooldownEnd Function**

#### 15.1) Add New Function
   - 15.1.1) In My Blueprint -> Functions -> Click +
   - 15.1.2) Name: OnCooldownEnd

#### 15.2) Implement Cooldown End
   - 15.2.1) From function entry:
   - 15.2.1.1) Add SET bIsOnCooldown node
   - 15.2.1.2) Value: False (Uncheck)
   - 15.2.2) From SET bIsOnCooldown:
   - 15.2.2.1) Add Call OnCooldownComplete node

### **16) Create IsUltimateUnlocked Function**

#### 16.1) Add New Function
   - 16.1.1) In My Blueprint -> Functions -> Click +
   - 16.1.2) Name: IsUltimateUnlocked
   - 16.1.3) Add input: UltimateType (EFatherUltimate)
   - 16.1.4) Add output: ReturnValue (Boolean)
   - 16.1.5) Set Pure: Check

#### 16.2) Implement Unlock Check
   - 16.2.1) From function entry:
   - 16.2.1.1) Drag GET UnlockedUltimates
   - 16.2.1.2) Add Contains node
   - 16.2.1.3) Item: UltimateType input
   - 16.2.2) Return: Contains result

### **17) Compile and Save Component**
   - 17.1) Click Compile
   - 17.2) Click Save

---

## **PHASE 4: CREATE WBP_ULTIMATEICON WIDGET**

### **18) Create Icon Widget Blueprint**

#### 18.1) Navigate to UI Folder
   - 18.1.1) Navigate to /Content/FatherCompanion/
   - 18.1.2) Create folder if needed: UI
   - 18.1.3) Double-click UI folder

#### 18.2) Create Widget
   - 18.2.1) Right-click in Content Browser
   - 18.2.2) Select User Interface -> Widget Blueprint
   - 18.2.3) Name: WBP_UltimateIcon
   - 18.2.4) Press Enter
   - 18.2.5) Double-click to open

### **19) Design Icon Layout**

#### 19.1) Configure Canvas Panel
   - 19.1.1) Select Canvas Panel in Hierarchy
   - 19.1.2) In Details -> Slot
   - 19.1.3) Size X: 64
   - 19.1.4) Size Y: 64

#### 19.2) Add Icon Image
   - 19.2.1) From Palette, drag Image onto Canvas Panel
   - 19.2.2) Rename to: IconImage
   - 19.2.3) Anchors: Center (0.5, 0.5)
   - 19.2.4) Position X: 0
   - 19.2.5) Position Y: 0
   - 19.2.6) Size X: 48
   - 19.2.7) Size Y: 48
   - 19.2.8) Alignment X: 0.5
   - 19.2.9) Alignment Y: 0.5

#### 19.3) Add Glow Border Image
   - 19.3.1) From Palette, drag Image onto Canvas Panel
   - 19.3.2) Rename to: GlowBorder
   - 19.3.3) In Hierarchy, drag GlowBorder above IconImage
   - 19.3.4) Anchors: Center (0.5, 0.5)
   - 19.3.5) Position X: 0
   - 19.3.6) Position Y: 0
   - 19.3.7) Size X: 64
   - 19.3.8) Size Y: 64
   - 19.3.9) Alignment X: 0.5
   - 19.3.10) Alignment Y: 0.5
   - 19.3.11) Tint: R=1.0, G=0.8, B=0.2, A=0.0

### **20) Create Icon Variables**

#### 20.1) Switch to Graph Tab
   - 20.1.1) Click Graph tab

#### 20.2) Create UltimateType Variable
   - 20.2.1) In My Blueprint -> Variables -> Click +
   - 20.2.2) Name: UltimateType
   - 20.2.3) Variable Type: EFatherUltimate
   - 20.2.4) Instance Editable: Check
   - 20.2.5) Expose on Spawn: Check

#### 20.3) Create IconState Variable
   - 20.3.1) Click + next to Variables
   - 20.3.2) Name: IconState
   - 20.3.3) Variable Type: Integer
   - 20.3.4) Default Value: 0 (Normal)
   - 20.3.5) Note: 0=Normal, 1=Ready (with glow)

#### 20.4) Create IconTexture Variable
   - 20.4.1) Click + next to Variables
   - 20.4.2) Name: IconTexture
   - 20.4.3) Variable Type: Texture2D (Object Reference)
   - 20.4.4) Instance Editable: Check
   - 20.4.5) Expose on Spawn: Check

#### 20.5) Create PulseTimerHandle Variable
   - 20.5.1) Click + next to Variables
   - 20.5.2) Name: PulseTimerHandle
   - 20.5.3) Variable Type: Timer Handle

#### 20.6) Create bPulsingUp Variable
   - 20.6.1) Click + next to Variables
   - 20.6.2) Name: bPulsingUp
   - 20.6.3) Variable Type: Boolean
   - 20.6.4) Default Value: True

#### 20.7) Create CurrentGlowAlpha Variable
   - 20.7.1) Click + next to Variables
   - 20.7.2) Name: CurrentGlowAlpha
   - 20.7.3) Variable Type: Float
   - 20.7.4) Default Value: 0.0

### **21) Create SetIconState Function**

#### 21.1) Add New Function
   - 21.1.1) In My Blueprint -> Functions -> Click +
   - 21.1.2) Name: SetIconState
   - 21.1.3) Add input: NewState (Integer)

#### 21.2) Implement State Logic
   - 21.2.1) From function entry:
   - 21.2.1.1) Add SET IconState node
   - 21.2.1.2) Value: NewState input
   - 21.2.2) From SET IconState:
   - 21.2.2.1) Add Switch on Int node
   - 21.2.2.2) Selection: NewState
   - 21.2.3) From case 0 (Normal):
   - 21.2.3.1) Get IconImage reference
   - 21.2.3.2) Add Set Color and Opacity node
   - 21.2.3.3) In Color and Opacity: R=1.0, G=1.0, B=1.0, A=1.0
   - 21.2.3.4) Call StopPulseAnimation
   - 21.2.4) From case 1 (Ready):
   - 21.2.4.1) Get IconImage reference
   - 21.2.4.2) Add Set Color and Opacity node
   - 21.2.4.3) In Color and Opacity: R=1.0, G=1.0, B=1.0, A=1.0
   - 21.2.4.4) Call StartPulseAnimation

### **22) Create StartPulseAnimation Function**

#### 22.1) Add New Function
   - 22.1.1) In My Blueprint -> Functions -> Click +
   - 22.1.2) Name: StartPulseAnimation

#### 22.2) Implement Pulse Start
   - 22.2.1) From function entry:
   - 22.2.1.1) Add Set Timer by Function Name node
   - 22.2.1.2) Function Name: PulseEffect
   - 22.2.1.3) Time: 0.05
   - 22.2.1.4) Looping: Check
   - 22.2.2) From Set Timer:
   - 22.2.2.1) Add SET PulseTimerHandle node
   - 22.2.2.2) Value: Timer Handle return

### **23) Create StopPulseAnimation Function**

#### 23.1) Add New Function
   - 23.1.1) In My Blueprint -> Functions -> Click +
   - 23.1.2) Name: StopPulseAnimation

#### 23.2) Implement Pulse Stop
   - 23.2.1) From function entry:
   - 23.2.1.1) Add Clear Timer by Handle node
   - 23.2.1.2) Handle: GET PulseTimerHandle
   - 23.2.2) From Clear Timer:
   - 23.2.2.1) Get GlowBorder reference
   - 23.2.2.2) Add Set Render Opacity node
   - 23.2.2.3) In Opacity: 0.0

### **24) Create PulseEffect Function**

#### 24.1) Add New Function
   - 24.1.1) In My Blueprint -> Functions -> Click +
   - 24.1.2) Name: PulseEffect

#### 24.2) Implement Pulse Logic
   - 24.2.1) From function entry:
   - 24.2.1.1) Add Branch node
   - 24.2.1.2) Condition: GET bPulsingUp
   - 24.2.2) From Branch True (pulsing up):
   - 24.2.2.1) Add Add (Float) node
   - 24.2.2.2) A: GET CurrentGlowAlpha
   - 24.2.2.3) B: 0.05
   - 24.2.2.4) Add SET CurrentGlowAlpha
   - 24.2.2.5) Value: Add result
   - 24.2.3) Check upper bound:
   - 24.2.3.1) Add >= (Float) node
   - 24.2.3.2) A: CurrentGlowAlpha
   - 24.2.3.3) B: 0.8
   - 24.2.3.4) Add Branch node
   - 24.2.4) From upper bound True:
   - 24.2.4.1) Add SET bPulsingUp
   - 24.2.4.2) Value: False
   - 24.2.5) From Branch False (pulsing down):
   - 24.2.5.1) Add Subtract (Float) node
   - 24.2.5.2) A: GET CurrentGlowAlpha
   - 24.2.5.3) B: 0.05
   - 24.2.5.4) Add SET CurrentGlowAlpha
   - 24.2.5.5) Value: Subtract result
   - 24.2.6) Check lower bound:
   - 24.2.6.1) Add <= (Float) node
   - 24.2.6.2) A: CurrentGlowAlpha
   - 24.2.6.3) B: 0.2
   - 24.2.6.4) Add Branch node
   - 24.2.7) From lower bound True:
   - 24.2.7.1) Add SET bPulsingUp
   - 24.2.7.2) Value: True
   - 24.2.8) Apply glow opacity:
   - 24.2.8.1) Get GlowBorder reference
   - 24.2.8.2) Add Set Render Opacity node
   - 24.2.8.3) In Opacity: GET CurrentGlowAlpha

### **25) Implement Event Construct**

#### 25.1) Setup Icon on Construct
   - 25.1.1) In Event Graph, add Event Construct
   - 25.1.2) From Event Construct:
   - 25.1.2.1) Get IconImage reference
   - 25.1.2.2) Add Set Brush from Texture node
   - 25.1.2.3) Texture: GET IconTexture
   - 25.1.3) From Set Brush:
   - 25.1.3.1) Call SetIconState
   - 25.1.3.2) New State: 0 (Normal)

### **26) Compile and Save Widget**
   - 26.1) Click Compile
   - 26.2) Click Save

---

## **PHASE 5: CREATE WBP_ULTIMATEPANEL WIDGET**

### **27) Create Panel Widget Blueprint**

#### 27.1) Create Widget
   - 27.1.1) In UI folder, right-click
   - 27.1.2) Select User Interface -> Widget Blueprint
   - 27.1.3) Name: WBP_UltimatePanel
   - 27.1.4) Press Enter
   - 27.1.5) Double-click to open

### **28) Design Panel Layout**

#### 28.1) Configure Canvas Panel
   - 28.1.1) Select Canvas Panel in Hierarchy
   - 28.1.2) In Details -> Slot
   - 28.1.3) Size X: 256
   - 28.1.4) Size Y: 256

#### 28.2) Add Icon Container
   - 28.2.1) From Palette, drag Canvas Panel onto root
   - 28.2.2) Rename to: IconContainer
   - 28.2.3) Anchors: Bottom Left
   - 28.2.4) Position X: 0
   - 28.2.5) Position Y: 0
   - 28.2.6) Size X: 256
   - 28.2.7) Size Y: 256

### **29) Create Panel Variables**

#### 29.1) Switch to Graph Tab
   - 29.1.1) Click Graph tab

#### 29.2) Create Icon Reference Variables
   - 29.2.1) Create: SymbioteIcon (WBP_UltimateIcon Reference)
   - 29.2.2) Create: SwordIcon (WBP_UltimateIcon Reference)
   - 29.2.3) Create: RifleIcon (WBP_UltimateIcon Reference)
   - 29.2.4) Create: BowIcon (WBP_UltimateIcon Reference)

#### 29.3) Create ChargeComponentRef Variable
   - 29.3.1) Click + next to Variables
   - 29.3.2) Name: ChargeComponentRef
   - 29.3.3) Variable Type: UltimateChargeComponent (Object Reference)

#### 29.4) Create Icon Texture Variables
   - 29.4.1) Create: SymbioteTexture (Texture2D Reference)
   - 29.4.1.1) Default: T_UltimateIcon_Symbiote
   - 29.4.2) Create: SwordTexture (Texture2D Reference)
   - 29.4.2.1) Default: T_UltimateIcon_Sword
   - 29.4.3) Create: RifleTexture (Texture2D Reference)
   - 29.4.3.1) Default: T_UltimateIcon_Rifle
   - 29.4.4) Create: BowTexture (Texture2D Reference)
   - 29.4.4.1) Default: T_UltimateIcon_Bow

### **30) Calculate Arc Positions**

Arc positions for 4 icons in 90-degree span (bottom-left corner):

| Icon | Angle | X Position | Y Position |
|------|-------|------------|------------|
| Symbiote | 0 degrees | 32 | 224 |
| Sword | 30 degrees | 80 | 176 |
| Rifle | 60 degrees | 128 | 128 |
| Bow | 90 degrees | 176 | 80 |

### **31) Implement Event Construct**

#### 31.1) Get Charge Component
   - 31.1.1) Add Event Construct
   - 31.1.2) From Event Construct:
   - 31.1.2.1) Add Get Owning Player Pawn node
   - 31.1.3) From Owning Player Pawn:
   - 31.1.3.1) Add Get Component by Class node
   - 31.1.3.2) Component Class: UltimateChargeComponent
   - 31.1.4) From Get Component:
   - 31.1.4.1) Add SET ChargeComponentRef node

#### 31.2) Set Panel Hidden by Default
   - 31.2.1) From SET ChargeComponentRef:
   - 31.2.1.1) Add Set Visibility node
   - 31.2.1.2) Target: Self
   - 31.2.1.3) In Visibility: Collapsed

#### 31.3) Bind to OnUltimateReady Event
   - 31.3.1) From Set Visibility:
   - 31.3.1.1) Get ChargeComponentRef
   - 31.3.1.2) Drag OnUltimateReady dispatcher
   - 31.3.1.3) Add Bind Event to OnUltimateReady node
   - 31.3.2) Create Custom Event: HandleUltimateReady
   - 31.3.2.1) Add input: UltimateType (EFatherUltimate)

#### 31.4) Bind to OnUltimateUsed Event
   - 31.4.1) From Bind OnUltimateReady:
   - 31.4.1.1) Get ChargeComponentRef
   - 31.4.1.2) Drag OnUltimateUsed dispatcher
   - 31.4.1.3) Add Bind Event to OnUltimateUsed node
   - 31.4.2) Create Custom Event: HandleUltimateUsed
   - 31.4.2.1) Add input: UltimateType (EFatherUltimate)

### **32) Implement HandleUltimateReady Event**

#### 32.1) Clear Previous Icons
   - 32.1.1) From HandleUltimateReady event:
   - 32.1.1.1) Get IconContainer reference
   - 32.1.1.2) Add Clear Children node

#### 32.2) Create Icons for Unlocked Ultimates Only
   - 32.2.1) From Clear Children:
   - 32.2.1.1) Add For Each Loop node
   - 32.2.1.2) Array: Create array of all EFatherUltimate values (Symbiote, Sword, Rifle, Bow)
   - 32.2.2) Inside loop body:
   - 32.2.2.1) Get ChargeComponentRef
   - 32.2.2.2) Call IsUltimateUnlocked
   - 32.2.2.3) Input: Array Element (current ultimate)
   - 32.2.3) From IsUltimateUnlocked:
   - 32.2.3.1) Add Branch node
   - 32.2.3.2) Condition: Return Value

#### 32.3) Create Icon for Unlocked Ultimate
   - 32.3.1) From Branch True (unlocked):
   - 32.3.1.1) Add Create Widget node
   - 32.3.1.2) Class: WBP_UltimateIcon
   - 32.3.1.3) Ultimate Type: Array Element
   - 32.3.1.4) Icon Texture: Call GetTextureForUltimate(Array Element)
   - 32.3.2) From Create Widget:
   - 32.3.2.1) Call StoreIconReference (to store in correct variable)
   - 32.3.3) From Store:
   - 32.3.3.1) Get IconContainer
   - 32.3.3.2) Add Add Child to Canvas node
   - 32.3.3.3) Content: Created widget
   - 32.3.4) From Add Child:
   - 32.3.4.1) Call SetPositionForUltimate(Array Element, Created widget)

#### 32.4) Set All Icons to Ready State
   - 32.4.1) From For Each Loop Completed:
   - 32.4.1.1) For each stored icon reference (SymbioteIcon, SwordIcon, etc):
   - 32.4.1.2) Add Is Valid node
   - 32.4.1.3) If valid: Call SetIconState(1) (Ready with glow)

#### 32.5) Show Panel
   - 32.5.1) From setting icon states:
   - 32.5.1.1) Add Set Visibility node
   - 32.5.1.2) Target: Self
   - 32.5.1.3) In Visibility: Visible

### **33) Implement HandleUltimateUsed Event**

#### 33.1) Hide Panel
   - 33.1.1) From HandleUltimateUsed event:
   - 33.1.1.1) Add Set Visibility node
   - 33.1.1.2) Target: Self
   - 33.1.1.3) In Visibility: Collapsed

#### 33.2) Stop All Pulse Animations
   - 33.2.1) From Set Visibility:
   - 33.2.1.1) For each icon (SymbioteIcon, SwordIcon, RifleIcon, BowIcon):
   - 33.2.1.2) Add Is Valid node
   - 33.2.1.3) If valid: Call SetIconState(0) (Normal, stops pulse)

### **34) Create GetTextureForUltimate Function**

#### 34.1) Add New Function
   - 34.1.1) In My Blueprint -> Functions -> Click +
   - 34.1.2) Name: GetTextureForUltimate
   - 34.1.3) Add input: UltimateType (EFatherUltimate)
   - 34.1.4) Add output: Texture (Texture2D Reference)

#### 34.2) Implement Switch Logic
   - 34.2.1) From function entry:
   - 34.2.1.1) Add Switch on EFatherUltimate node
   - 34.2.1.2) Selection: UltimateType
   - 34.2.2) From Symbiote case: Return GET SymbioteTexture
   - 34.2.3) From Sword case: Return GET SwordTexture
   - 34.2.4) From Rifle case: Return GET RifleTexture
   - 34.2.5) From Bow case: Return GET BowTexture

### **35) Create GetIconForUltimate Function**

#### 35.1) Add New Function
   - 35.1.1) In My Blueprint -> Functions -> Click +
   - 35.1.2) Name: GetIconForUltimate
   - 35.1.3) Add input: UltimateType (EFatherUltimate)
   - 35.1.4) Add output: Icon (WBP_UltimateIcon Reference)

#### 35.2) Implement Switch Logic
   - 35.2.1) From function entry:
   - 35.2.1.1) Add Switch on EFatherUltimate node
   - 35.2.1.2) Selection: UltimateType input
   - 35.2.2) From Symbiote case: Return GET SymbioteIcon
   - 35.2.3) From Sword case: Return GET SwordIcon
   - 35.2.4) From Rifle case: Return GET RifleIcon
   - 35.2.5) From Bow case: Return GET BowIcon

### **36) Create SetPositionForUltimate Function**

#### 36.1) Add New Function
   - 36.1.1) In My Blueprint -> Functions -> Click +
   - 36.1.2) Name: SetPositionForUltimate
   - 36.1.3) Add input: UltimateType (EFatherUltimate)
   - 36.1.4) Add input: IconWidget (WBP_UltimateIcon Reference)

#### 36.2) Implement Position Logic
   - 36.2.1) From function entry:
   - 36.2.1.1) Add Switch on EFatherUltimate node
   - 36.2.1.2) Selection: UltimateType
   - 36.2.2) From Symbiote case:
   - 36.2.2.1) Set slot Position X: 32, Y: 224
   - 36.2.3) From Sword case:
   - 36.2.3.1) Set slot Position X: 80, Y: 176
   - 36.2.4) From Rifle case:
   - 36.2.4.1) Set slot Position X: 128, Y: 128
   - 36.2.5) From Bow case:
   - 36.2.5.1) Set slot Position X: 176, Y: 80

### **37) Create StoreIconReference Function**

#### 37.1) Add New Function
   - 37.1.1) In My Blueprint -> Functions -> Click +
   - 37.1.2) Name: StoreIconReference
   - 37.1.3) Add input: UltimateType (EFatherUltimate)
   - 37.1.4) Add input: IconWidget (WBP_UltimateIcon Reference)

#### 37.2) Implement Storage Logic
   - 37.2.1) From function entry:
   - 37.2.1.1) Add Switch on EFatherUltimate node
   - 37.2.1.2) Selection: UltimateType
   - 37.2.2) From Symbiote case: SET SymbioteIcon = IconWidget
   - 37.2.3) From Sword case: SET SwordIcon = IconWidget
   - 37.2.4) From Rifle case: SET RifleIcon = IconWidget
   - 37.2.5) From Bow case: SET BowIcon = IconWidget

### **38) Add Icon Click Handling**

#### 38.1) Add OnClicked Event to WBP_UltimateIcon
   - 38.1.1) Open WBP_UltimateIcon blueprint
   - 38.1.2) Create Event Dispatcher: OnIconClicked
   - 38.1.2.1) Add input: UltimateType (EFatherUltimate)

#### 38.2) Add Button Functionality to Icon
   - 38.2.1) In WBP_UltimateIcon Designer:
   - 38.2.1.1) Wrap IconImage in a Button widget
   - 38.2.1.2) Or set IconImage -> Behavior -> Is Enabled: true
   - 38.2.2) In Event Graph:
   - 38.2.2.1) Add On Clicked event for button/image
   - 38.2.2.2) From On Clicked:
   - 38.2.2.3) Call OnIconClicked dispatcher
   - 38.2.2.4) Ultimate Type: GET UltimateType

#### 38.3) Bind Icon Clicks in WBP_UltimatePanel
   - 38.3.1) After creating each icon in HandleUltimateReady:
   - 38.3.1.1) From created widget
   - 38.3.1.2) Drag OnIconClicked dispatcher
   - 38.3.1.3) Add Bind Event to OnIconClicked
   - 38.3.2) Create Custom Event: HandleIconClicked
   - 38.3.2.1) Add input: SelectedUltimate (EFatherUltimate)

#### 38.4) Implement HandleIconClicked
   - 38.4.1) From HandleIconClicked event:
   - 38.4.1.1) Get ChargeComponentRef
   - 38.4.1.2) SET EquippedUltimate = SelectedUltimate
   - 38.4.2) From SET EquippedUltimate:
   - 38.4.2.1) Call ActivateSelectedUltimate (player function)

### **39) Compile and Save Panel**
   - 39.1) Click Compile
   - 39.2) Click Save

---

## **PHASE 6: PLAYER CHARACTER INTEGRATION**

### **40) Add UltimateChargeComponent to Player**

#### 40.1) Open Player Character Blueprint
   - 40.1.1) Navigate to player character location
   - 40.1.2) Example: /Content/Characters/BP_NarrativePlayer
   - 40.1.3) Double-click to open

#### 40.2) Add Component
   - 40.2.1) In Components panel, click Add
   - 40.2.2) Search: UltimateChargeComponent
   - 40.2.3) Select UltimateChargeComponent

#### 40.3) Configure Component
   - 40.3.1) Select UltimateChargeComponent in Components
   - 40.3.2) In Details -> Settings:
   - 40.3.2.1) ThresholdCurve: FC_UltimateThreshold
   - 40.3.2.2) EquippedUltimate: Symbiote
   - 40.3.2.3) UnlockedUltimates: Add Symbiote
   - 40.3.2.4) FatherDamageEfficiency: 0.5
   - 40.3.2.5) CooldownDuration: 120.0

### **41) Compile and Save Player**
   - 41.1) Click Compile
   - 41.2) Click Save

---

## **PHASE 7: HUD INTEGRATION**

### **42) Open Main HUD Widget**

#### 42.1) Navigate to HUD
   - 42.1.1) Navigate to your main HUD widget blueprint
   - 42.1.2) Double-click to open

### **43) Add Ultimate Panel to HUD**

#### 43.1) Switch to Designer Tab
   - 43.1.1) Click Designer tab

#### 43.2) Add WBP_UltimatePanel
   - 43.2.1) In Palette, expand User Created
   - 43.2.2) Find WBP_UltimatePanel
   - 43.2.3) Drag onto Canvas Panel

#### 43.3) Configure Position
   - 43.3.1) Select WBP_UltimatePanel in Hierarchy
   - 43.3.2) In Details -> Slot (Canvas Panel Slot):
   - 43.3.2.1) Anchors: Bottom Left
   - 43.3.2.2) Position X: 20
   - 43.3.2.3) Position Y: -276
   - 43.3.2.4) Size X: 256
   - 43.3.2.5) Size Y: 256
   - 43.3.2.6) Alignment X: 0.0
   - 43.3.2.7) Alignment Y: 1.0

#### 43.4) Set Initial Visibility
   - 43.4.1) In Details -> Behavior:
   - 43.4.1.1) Visibility: Collapsed
   - 43.4.2) Panel will show itself via OnUltimateReady binding

### **44) Compile and Save HUD**
   - 44.1) Click Compile
   - 44.2) Click Save

---

## **CHANGELOG**

| Version | Date | Changes |
|---------|------|---------|
| 1.1 | January 2026 | Changed panel visibility behavior: hidden by default, appears only when charge threshold reached, hides after use. Removed Locked icon state (only Normal and Ready states remain). Added OnUltimateUsed binding to hide panel. Added icon click handling for ultimate selection. Updated icon state values (0=Normal, 1=Ready). Added helper functions: GetTextureForUltimate, SetPositionForUltimate, StoreIconReference. Panel dynamically creates icons for unlocked ultimates only. |
| 1.0 | December 2025 | Initial implementation guide. EFatherUltimate enumeration with 4 values (Symbiote, Sword, Rifle, Bow). FC_UltimateThreshold float curve for level-based threshold scaling. UltimateChargeComponent with shared charge pool, damage efficiency, and cooldown. WBP_UltimateIcon with 3 states (Locked, Unlocked, Ready) and pulse animation. WBP_UltimatePanel with arc layout positioning 4 icons in 90-degree span. Player character integration. HUD integration with bottom-left positioning. |

---

**END OF WBP_ULTIMATEPANEL IMPLEMENTATION GUIDE VERSION 1.1**

**Blueprint-Only Implementation for Unreal Engine 5.6 + Narrative Pro Plugin v2.2**
