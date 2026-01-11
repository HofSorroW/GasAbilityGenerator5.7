# Father Companion - WBP_MarkIndicator Widget Implementation Guide
## VERSION 1.0 - Enemy Mark Indicator Above Head (Always Visible Through Walls)
## Unreal Engine 5.6 + Narrative Pro Plugin v2.1

---

## **DOCUMENT INFORMATION**

| Field | Value |
|-------|-------|
| Document Type | Widget Implementation Guide |
| Widget Name | WBP_MarkIndicator |
| Widget Type | User Widget (UMG) |
| Display Method | Widget Component attached to enemy |
| Version | 1.0 |
| Last Updated | December 2025 |
| Estimated Time | 30-45 minutes |

---

## **TABLE OF CONTENTS**

1. Introduction
2. Prerequisites
3. PHASE 1: Create Widget Blueprint
4. PHASE 2: Design Widget Layout
5. PHASE 3: Create Widget Variables
6. PHASE 4: Implement Pulse Animation
7. PHASE 5: Implement Duration Timer
8. Quick Reference
9. Changelog

---

## **INTRODUCTION**

### **Widget Overview**

WBP_MarkIndicator is a simple visual indicator displayed above marked enemies. When GA_FatherMark applies a mark to an enemy, a Widget Component is dynamically added to the enemy actor displaying this widget. The indicator remains visible through walls using Screen space rendering, providing tactical awareness to the player.

### **Key Features**

| Feature | Description |
|---------|-------------|
| Display Position | 100 units above enemy head |
| Visibility | Always visible (Screen space) |
| Through Walls | Yes (Widget Component in Screen mode) |
| Animation | Pulse effect while active |
| Duration | Matches GE_FatherMark (5 seconds) |
| Size | 32x32 pixels |

### **Technical Specifications**

| Parameter | Value |
|-----------|-------|
| Widget Space | Screen |
| Draw Size | 32x32 |
| Relative Location Z | 100 (above head) |
| Render Opacity | 1.0 (pulsing between 0.6-1.0) |
| Mark Duration | 5 seconds |

### **Integration with GA_FatherMark**

| Step | Component | Action |
|------|-----------|--------|
| 1 | GA_FatherMark | Adds Widget Component to enemy |
| 2 | Widget Component | Class set to WBP_MarkIndicator |
| 3 | Widget Component | Space set to Screen |
| 4 | Widget Component | Location set to (0, 0, 100) |
| 5 | GE_FatherMark expires | Widget Component destroyed via HandleMarkedEnemyDeath or timer |

---

## **PREREQUISITES**

### **Required Before This Guide**

| Requirement | Description |
|-------------|-------------|
| GA_FatherMark | Father marking ability implemented |
| BP_FatherCompanion | Father character with marking functions |
| T_MarkIndicator | Texture for mark icon (placeholder) |

### **Placeholder Assets**

| Asset | Type | Description |
|-------|------|-------------|
| T_MarkIndicator | Texture2D | Mark icon (crosshair, target, or father icon) |

---

## **PHASE 1: CREATE WIDGET BLUEPRINT**

### **1) Navigate to UI Folder**

#### 1.1) Open Content Browser
   - 1.1.1) Navigate to /Content/FatherCompanion/
   - 1.1.2) Create folder if needed: UI
   - 1.1.3) Double-click UI folder to open

### **2) Create Widget Blueprint**

#### 2.1) Create New Widget
   - 2.1.1) Right-click in Content Browser
   - 2.1.2) Select User Interface -> Widget Blueprint
   - 2.1.3) Name: WBP_MarkIndicator
   - 2.1.4) Press Enter
   - 2.1.5) Double-click WBP_MarkIndicator to open

---

## **PHASE 2: DESIGN WIDGET LAYOUT**

### **3) Configure Canvas Panel**

#### 3.1) Select Root Canvas
   - 3.1.1) In Hierarchy panel, select Canvas Panel
   - 3.1.2) In Details panel, find Slot section
   - 3.1.3) Size X: 32
   - 3.1.4) Size Y: 32

### **4) Add Mark Icon Image**

#### 4.1) Add Image Widget
   - 4.1.1) From Palette panel, drag Image onto Canvas Panel
   - 4.1.2) In Hierarchy, rename to: MarkIcon
   - 4.1.3) Select MarkIcon in Hierarchy

#### 4.2) Configure Image Properties
   - 4.2.1) In Details panel, find Slot section
   - 4.2.2) Anchors: Center (0.5, 0.5)
   - 4.2.3) Position X: 0
   - 4.2.4) Position Y: 0
   - 4.2.5) Size X: 32
   - 4.2.6) Size Y: 32
   - 4.2.7) Alignment X: 0.5
   - 4.2.8) Alignment Y: 0.5

#### 4.3) Set Image Brush
   - 4.3.1) In Details panel, find Appearance section
   - 4.3.2) Expand Brush property
   - 4.3.3) Image: T_MarkIndicator (placeholder texture)
   - 4.3.4) Tint: R=1.0, G=0.2, B=0.2, A=1.0 (red tint for visibility)
   - 4.3.5) Draw As: Image

### **5) Add Background Glow (Optional)**

#### 5.1) Add Background Image
   - 5.1.1) From Palette, drag Image onto Canvas Panel
   - 5.1.2) In Hierarchy, rename to: GlowBackground
   - 5.1.3) In Hierarchy, drag GlowBackground above MarkIcon (renders behind)

#### 5.2) Configure Glow Properties
   - 5.2.1) Select GlowBackground in Hierarchy
   - 5.2.2) Anchors: Center (0.5, 0.5)
   - 5.2.3) Position X: 0
   - 5.2.4) Position Y: 0
   - 5.2.5) Size X: 40
   - 5.2.6) Size Y: 40
   - 5.2.7) Alignment X: 0.5
   - 5.2.8) Alignment Y: 0.5

#### 5.3) Set Glow Brush
   - 5.3.1) Image: Engine default radial gradient or solid color
   - 5.3.2) Tint: R=1.0, G=0.0, B=0.0, A=0.3 (semi-transparent red)

---

## **PHASE 3: CREATE WIDGET VARIABLES**

### **6) Switch to Graph Tab**

#### 6.1) Open Event Graph
   - 6.1.1) Click Graph tab at top of Widget Blueprint editor

### **7) Create Variables**

#### 7.1) Add MarkDuration Variable
   - 7.1.1) In My Blueprint panel, find Variables section
   - 7.1.2) Click + button next to Variables
   - 7.1.3) Name: MarkDuration
   - 7.1.4) Variable Type: Float
   - 7.1.5) Instance Editable: Checked
   - 7.1.6) Default Value: 5.0
   - 7.1.7) Category: Mark Settings
   - 7.1.8) Tooltip: Duration in seconds before mark expires

#### 7.2) Add RemainingTime Variable
   - 7.2.1) Click + button next to Variables
   - 7.2.2) Name: RemainingTime
   - 7.2.3) Variable Type: Float
   - 7.2.4) Instance Editable: Unchecked
   - 7.2.5) Default Value: 5.0
   - 7.2.6) Category: State
   - 7.2.7) Tooltip: Current remaining mark duration

#### 7.3) Add PulseTimerHandle Variable
   - 7.3.1) Click + button next to Variables
   - 7.3.2) Name: PulseTimerHandle
   - 7.3.3) Variable Type: Timer Handle (Struct)
   - 7.3.4) Category: Internal
   - 7.3.5) Tooltip: Handle for pulse animation timer

#### 7.4) Add bPulsingUp Variable
   - 7.4.1) Click + button next to Variables
   - 7.4.2) Name: bPulsingUp
   - 7.4.3) Variable Type: Boolean
   - 7.4.4) Default Value: True (Checked)
   - 7.4.5) Category: Internal
   - 7.4.6) Tooltip: Pulse direction flag

#### 7.5) Add CurrentOpacity Variable
   - 7.5.1) Click + button next to Variables
   - 7.5.2) Name: CurrentOpacity
   - 7.5.3) Variable Type: Float
   - 7.5.4) Default Value: 1.0
   - 7.5.5) Category: Internal
   - 7.5.6) Tooltip: Current opacity for pulse effect

---

## **PHASE 4: IMPLEMENT PULSE ANIMATION**

### **8) Create PulseEffect Function**

#### 8.1) Add New Function
   - 8.1.1) In My Blueprint panel, find Functions section
   - 8.1.2) Click + button next to Functions
   - 8.1.3) Name: PulseEffect
   - 8.1.4) Double-click to open function graph

#### 8.2) Implement Pulse Logic

##### 8.2.1) Check Pulse Direction
   - 8.2.1.1) From function entry execution:
   - 8.2.1.2) Drag GET bPulsingUp into graph
   - 8.2.1.3) Add Branch node
   - 8.2.1.4) Connect bPulsingUp to Condition

##### 8.2.2) Pulsing Up Path (True)
   - 8.2.2.1) From Branch True execution:
   - 8.2.2.2) Drag GET CurrentOpacity into graph
   - 8.2.2.3) Add Float + Float node
   - 8.2.2.4) A: CurrentOpacity
   - 8.2.2.5) B: 0.05 (opacity increment)
   - 8.2.2.6) Add SET CurrentOpacity node
   - 8.2.2.7) Connect addition result to value

##### 8.2.3) Check Upper Bound
   - 8.2.3.1) From SET CurrentOpacity:
   - 8.2.3.2) Add >= (Float >= Float) node
   - 8.2.3.3) A: GET CurrentOpacity
   - 8.2.3.4) B: 1.0
   - 8.2.3.5) Add Branch node
   - 8.2.3.6) Connect >= result to Condition

##### 8.2.4) Reverse Direction at Upper Bound
   - 8.2.4.1) From Branch True:
   - 8.2.4.2) Add SET bPulsingUp node
   - 8.2.4.3) bPulsingUp: Unchecked (false)

##### 8.2.5) Pulsing Down Path (False from 8.2.1)
   - 8.2.5.1) From Branch False (step 8.2.1.3) execution:
   - 8.2.5.2) Drag GET CurrentOpacity into graph
   - 8.2.5.3) Add Float - Float node
   - 8.2.5.4) A: CurrentOpacity
   - 8.2.5.5) B: 0.05 (opacity decrement)
   - 8.2.5.6) Add SET CurrentOpacity node
   - 8.2.5.7) Connect subtraction result to value

##### 8.2.6) Check Lower Bound
   - 8.2.6.1) From SET CurrentOpacity:
   - 8.2.6.2) Add <= (Float <= Float) node
   - 8.2.6.3) A: GET CurrentOpacity
   - 8.2.6.4) B: 0.6 (minimum opacity)
   - 8.2.6.5) Add Branch node
   - 8.2.6.6) Connect <= result to Condition

##### 8.2.7) Reverse Direction at Lower Bound
   - 8.2.7.1) From Branch True:
   - 8.2.7.2) Add SET bPulsingUp node
   - 8.2.7.3) bPulsingUp: Checked (true)

##### 8.2.8) Apply Opacity to Widget
   - 8.2.8.1) After all SET bPulsingUp paths merge:
   - 8.2.8.2) Add Set Render Opacity node
   - 8.2.8.3) Target: Self
   - 8.2.8.4) In Opacity: GET CurrentOpacity

### **9) Start Pulse Timer on Construct**

#### 9.1) Find Event Construct
   - 9.1.1) In Event Graph, locate Event Construct node
   - 9.1.2) If not present, right-click and add Event Construct

#### 9.2) Initialize Remaining Time
   - 9.2.1) From Event Construct execution:
   - 9.2.2) Add SET RemainingTime node
   - 9.2.3) Value: GET MarkDuration

#### 9.3) Start Pulse Timer
   - 9.3.1) From SET RemainingTime execution:
   - 9.3.2) Add Set Timer by Function Name node
   - 9.3.3) Function Name: PulseEffect
   - 9.3.4) Time: 0.05 (20 times per second)
   - 9.3.5) Looping: Checked
   - 9.3.6) Return Value: SET PulseTimerHandle

---

## **PHASE 5: IMPLEMENT DURATION TIMER**

### **10) Create Countdown Timer**

#### 10.1) Start Countdown Timer
   - 10.1.1) From SET PulseTimerHandle execution:
   - 10.1.2) Add Set Timer by Function Name node
   - 10.1.3) Function Name: OnMarkExpired
   - 10.1.4) Time: GET MarkDuration
   - 10.1.5) Looping: Unchecked

### **11) Create OnMarkExpired Function**

#### 11.1) Add New Function
   - 11.1.1) In My Blueprint panel, find Functions section
   - 11.1.2) Click + button next to Functions
   - 11.1.3) Name: OnMarkExpired
   - 11.1.4) Double-click to open function graph

#### 11.2) Implement Expiration Logic
   - 11.2.1) From function entry execution:
   - 11.2.2) Add Clear Timer by Handle node
   - 11.2.3) Handle: GET PulseTimerHandle

### **12) Create RefreshMark Function**

#### 12.1) Add New Function
   - 12.1.1) In My Blueprint panel, find Functions section
   - 12.1.2) Click + button next to Functions
   - 12.1.3) Name: RefreshMark
   - 12.1.4) Double-click to open function graph

#### 12.2) Implement Refresh Logic
   - 12.2.1) From function entry execution:
   - 12.2.2) Add SET RemainingTime node
   - 12.2.3) Value: GET MarkDuration

### **13) Compile and Save Widget**

#### 13.1) Finalize Widget
   - 13.1.1) Click Compile button
   - 13.1.2) Click Save button

---

## **QUICK REFERENCE**

### **Widget Configuration Summary**

| Property | Value |
|----------|-------|
| Widget Name | WBP_MarkIndicator |
| Canvas Size | 32x32 |
| Icon Size | 32x32 |
| Icon Tint | Red (1.0, 0.2, 0.2, 1.0) |
| Glow Size | 40x40 |
| Glow Tint | Semi-transparent red (1.0, 0.0, 0.0, 0.3) |

### **Variable Summary**

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| MarkDuration | Float | 5.0 | Total mark duration |
| RemainingTime | Float | 5.0 | Current remaining time |
| PulseTimerHandle | Timer Handle | None | Pulse animation timer |
| bPulsingUp | Boolean | True | Pulse direction |
| CurrentOpacity | Float | 1.0 | Current opacity value |

### **Pulse Animation Parameters**

| Parameter | Value |
|-----------|-------|
| Min Opacity | 0.6 |
| Max Opacity | 1.0 |
| Opacity Step | 0.05 |
| Pulse Rate | 20 Hz (0.05s interval) |
| Full Cycle | ~0.8 seconds |

### **Widget Component Configuration (Set by GA_FatherMark)**

| Property | Value |
|----------|-------|
| Widget Class | WBP_MarkIndicator |
| Widget Space | Screen |
| Draw Size | (32, 32) |
| Relative Location | (0, 0, 100) |

### **Integration Points**

| Component | Responsibility |
|-----------|----------------|
| GA_FatherMark | Creates Widget Component, sets class and location |
| WBP_MarkIndicator | Visual display, pulse animation |
| BP_FatherCompanion | Stores Widget Component reference in MarkWidgetMap |
| GE_FatherMark | Duration determines when mark expires |

### **Placeholder Textures Required**

| Texture | Purpose |
|---------|---------|
| T_MarkIndicator | Mark icon (crosshair, target, or father icon) |

---

## **CHANGELOG**

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | December 2025 | Initial implementation guide. Widget Blueprint for mark indicator above enemy head. Screen space rendering for always-visible-through-walls display. Pulse animation for visual feedback. Duration timer integration with GA_FatherMark. RefreshMark function for mark refresh on re-hit. |

---

**END OF IMPLEMENTATION GUIDE**

**Blueprint-Only Implementation for Unreal Engine 5.6 + Narrative Pro Plugin v2.1**
