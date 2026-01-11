# Father Companion VFX Guide
## Floating Energy Visual System
## Version 1.0

---

## DOCUMENT INFORMATION

| Field | Value |
|-------|-------|
| Document Type | VFX Implementation Guide |
| System | Father Companion Visual Effects |
| Created | January 2026 |
| Engine Version | Unreal Engine 5.6 |
| Plugin Dependencies | Narrative Pro v2.2 |
| Integrates With | BP_FatherCompanion (NarrativeNPCCharacter) |

---

## TABLE OF CONTENTS

| Phase | Title |
|-------|-------|
| PHASE 1 | OVERVIEW AND INTEGRATION |
| PHASE 2 | CREATE MATERIALS |
| PHASE 3 | CREATE NIAGARA EMITTERS |
| PHASE 4 | CREATE NIAGARA SYSTEM |
| PHASE 5 | ADD VFX TO BP_FATHERCOMPANION |
| PHASE 6 | IMPLEMENT STATE-BASED VISUALS |
| PHASE 7 | CREATE MATERIAL FUNCTIONS |

---

## INTRODUCTION

### System Overview

| Aspect | Description |
|--------|-------------|
| Purpose | Visual representation of father's consciousness as floating energy |
| Integration | Attaches to BP_FatherCompanion (NarrativeNPCCharacter) |
| Visual Concept | Layered energy sphere with particles and aura |
| Form Adaptation | Visuals change based on CurrentForm state |

### Relationship to BP_FatherCompanion

| Component | Source |
|-----------|--------|
| BP_FatherCompanion | Created via Father_Companion_System_Setup_Guide (NarrativeNPCCharacter) |
| NS_FatherCompanion | Created in this guide (Niagara System) |
| Integration | Niagara component added to BP_FatherCompanion |

### Visual Layer Structure

| Layer | Size | Purpose |
|-------|------|---------|
| Inner Core | 15-20cm | Bright pulsing center - the soul |
| Energy Shell | 25-30cm | Translucent sphere with Fresnel edge |
| Particle Field | 50cm radius | Orbiting energy wisps |
| Ambient Aura | 60cm radius | Soft environmental glow |
| Movement Trail | Variable | Ribbon trail during movement |

### Form Visual Variations

| Form | Visual Adaptation |
|------|-------------------|
| Crawler | Full orb visible, follows player |
| Armor | Disperses into player chest, particles visible at attachment |
| Exoskeleton | Disperses into player back, energy trail on limbs |
| Symbiote | Full body energy aura surrounding player |
| Engineer | Orb transforms into turret configuration |

---

## QUICK REFERENCE

### Assets Created

| Asset | Type | Location |
|-------|------|----------|
| M_FatherCore | Material | /Content/FatherCompanion/VFX/Materials/ |
| M_FatherShell | Material | /Content/FatherCompanion/VFX/Materials/ |
| M_FatherParticle | Material | /Content/FatherCompanion/VFX/Materials/ |
| MF_EnergyPulse | Material Function | /Content/FatherCompanion/VFX/Materials/ |
| MF_FresnelGlow | Material Function | /Content/FatherCompanion/VFX/Materials/ |
| MF_RadialGradient | Material Function | /Content/FatherCompanion/VFX/Materials/ |
| NE_FatherCore | Niagara Emitter | /Content/FatherCompanion/VFX/ |
| NE_FatherShell | Niagara Emitter | /Content/FatherCompanion/VFX/ |
| NE_FatherParticles | Niagara Emitter | /Content/FatherCompanion/VFX/ |
| NE_FatherAura | Niagara Emitter | /Content/FatherCompanion/VFX/ |
| NE_FatherTrail | Niagara Emitter | /Content/FatherCompanion/VFX/ |
| NS_FatherCompanion | Niagara System | /Content/FatherCompanion/VFX/ |

### Material Parameter Summary

| Material | Parameter | Type | Default |
|----------|-----------|------|---------|
| M_FatherCore | CoreColor | Vector | (1.0, 0.8, 0.4) |
| M_FatherCore | EmissiveIntensity | Scalar | 100.0 |
| M_FatherCore | PulseIntensity | Scalar | 0.3 |
| M_FatherShell | EdgeColor | Vector | (0.4, 0.6, 1.0) |
| M_FatherShell | ShellIntensity | Scalar | 20.0 |
| M_FatherShell | FresnelPower | Scalar | 3.0 |
| M_FatherShell | BaseOpacity | Scalar | 0.3 |
| M_FatherParticle | ParticleColor | Vector | (1.0, 0.9, 0.5) |
| M_FatherParticle | ParticleIntensity | Scalar | 30.0 |

### Niagara User Parameters

| Parameter | Type | Default | Purpose |
|-----------|------|---------|---------|
| CoreScale | Float | 1.0 | Scale of inner core mesh |
| ShellScale | Float | 1.0 | Scale of outer shell mesh |
| ParticleSpawnRate | Float | 15.0 | Orbiting particles per second |
| AlertLevel | Float | 0.0 | 0-1 alert state intensity |
| EmotionColor | LinearColor | (1.0, 0.8, 0.4, 1.0) | Emotion-based color tint |

---

## **PHASE 1: OVERVIEW AND INTEGRATION**

### **1) Prerequisites**

#### 1.1) Required Assets
   - 1.1.1) BP_FatherCompanion blueprint must exist (from System Setup Guide)
   - 1.1.2) BP_FatherCompanion must inherit from NarrativeNPCCharacter
   - 1.1.3) E_FatherForm enumeration must exist

#### 1.2) Folder Structure
   - 1.2.1) Create folder: `/Content/FatherCompanion/VFX/`
   - 1.2.2) Create subfolder: `/Content/FatherCompanion/VFX/Materials/`
   - 1.2.3) Create subfolder: `/Content/FatherCompanion/VFX/Emitters/`

### **2) Integration Architecture**

#### 2.1) Component Hierarchy

| Parent | Child | Purpose |
|--------|-------|---------|
| BP_FatherCompanion | FatherVFX (NiagaraComponent) | Main visual system |
| BP_FatherCompanion | AmbientGlow (PointLightComponent) | Dynamic lighting |

#### 2.2) Data Flow

| Source | Data | Target |
|--------|------|--------|
| BP_FatherCompanion.CurrentForm | Form state | NS_FatherCompanion parameters |
| BP_FatherCompanion.IsAttached | Attachment state | VFX visibility |
| BP_FatherCompanion.AlertLevel | Alert 0-1 | Particle intensity |

---

## **PHASE 2: CREATE MATERIALS**

### **1) Create M_FatherCore Material**

#### 1.1) Create Material Asset
   - 1.1.1) In Content Browser, navigate to `/Content/FatherCompanion/VFX/Materials/`
   - 1.1.2) Right-click in empty space
   - 1.1.3) Select Material
   - 1.1.4) Name: `M_FatherCore`
   - 1.1.5) Double-click to open Material Editor

#### 1.2) Configure Material Properties
   - 1.2.1) In Details panel, find Material section
   - 1.2.2) Blend Mode: Additive
   - 1.2.3) Shading Model: Unlit
   - 1.2.4) Two Sided: Checked

#### 1.3) Create Time-Based Pulse

##### 1.3.1) Add Time Node
   - 1.3.1.1) Right-click in graph
   - 1.3.1.2) Search: `Time`
   - 1.3.1.3) Select Time node

##### 1.3.2) Add Multiply for Speed
   - 1.3.2.1) Right-click in graph
   - 1.3.2.2) Search: `Multiply`
   - 1.3.2.3) Select Multiply node
   - 1.3.2.4) Connect Time output to Multiply A input
   - 1.3.2.5) Set B value: `2.0`

##### 1.3.3) Add Sine Node
   - 1.3.3.1) Right-click in graph
   - 1.3.3.2) Search: `Sine`
   - 1.3.3.3) Select Sine node
   - 1.3.3.4) Connect Multiply output to Sine input

##### 1.3.4) Remap Sine to 0-1
   - 1.3.4.1) Add Multiply node
   - 1.3.4.2) Connect Sine output to A
   - 1.3.4.3) Set B: `0.5`
   - 1.3.4.4) Add Add node
   - 1.3.4.5) Connect Multiply output to A
   - 1.3.4.6) Set B: `0.5`

#### 1.4) Create Color Parameter

##### 1.4.1) Add Vector Parameter
   - 1.4.1.1) Right-click in graph
   - 1.4.1.2) Search: `VectorParameter`
   - 1.4.1.3) Select Vector Parameter node
   - 1.4.1.4) In Details panel, Parameter Name: `CoreColor`
   - 1.4.1.5) Default Value: R=1.0, G=0.8, B=0.4

#### 1.5) Create Intensity Parameter

##### 1.5.1) Add Scalar Parameter
   - 1.5.1.1) Right-click in graph
   - 1.5.1.2) Search: `ScalarParameter`
   - 1.5.1.3) Select Scalar Parameter node
   - 1.5.1.4) Parameter Name: `EmissiveIntensity`
   - 1.5.1.5) Default Value: `100.0`

#### 1.6) Create Pulse Intensity Parameter

##### 1.6.1) Add Scalar Parameter
   - 1.6.1.1) Add another Scalar Parameter
   - 1.6.1.2) Parameter Name: `PulseIntensity`
   - 1.6.1.3) Default Value: `0.3`

#### 1.7) Combine Pulse with Intensity

##### 1.7.1) Calculate Pulse Amount
   - 1.7.1.1) Add Multiply node
   - 1.7.1.2) Connect remapped Sine (0-1) to A
   - 1.7.1.3) Connect PulseIntensity parameter to B

##### 1.7.2) Add Base Intensity
   - 1.7.2.1) Add Subtract node
   - 1.7.2.2) Set A: `1.0`
   - 1.7.2.3) Connect PulseIntensity to B

##### 1.7.3) Combine Base and Pulse
   - 1.7.3.1) Add Add node
   - 1.7.3.2) Connect base amount to A
   - 1.7.3.3) Connect pulse amount to B

##### 1.7.4) Apply Emissive Intensity
   - 1.7.4.1) Add Multiply node
   - 1.7.4.2) Connect combined pulse to A
   - 1.7.4.3) Connect EmissiveIntensity parameter to B

#### 1.8) Create Final Emissive Output

##### 1.8.1) Multiply Color by Intensity
   - 1.8.1.1) Add Multiply node
   - 1.8.1.2) Connect CoreColor parameter to A
   - 1.8.1.3) Connect final intensity to B
   - 1.8.1.4) Connect result to Emissive Color input on material node

#### 1.9) Add Fresnel for Edge Glow

##### 1.9.1) Add Fresnel Node
   - 1.9.1.1) Right-click in graph
   - 1.9.1.2) Search: `Fresnel`
   - 1.9.1.3) Select Fresnel node
   - 1.9.1.4) Exponent In: `3.0`

##### 1.9.2) Add to Final Color
   - 1.9.2.1) Add Add node
   - 1.9.2.2) Connect current Emissive Color chain to A
   - 1.9.2.3) Add Multiply node for Fresnel
   - 1.9.2.4) Connect Fresnel to Multiply A
   - 1.9.2.5) Connect CoreColor to Multiply B
   - 1.9.2.6) Add another Multiply
   - 1.9.2.7) Connect previous Multiply to A
   - 1.9.2.8) Set B: `50.0`
   - 1.9.2.9) Connect to Add B input
   - 1.9.2.10) Connect Add result to Emissive Color

#### 1.10) Apply and Save
   - 1.10.1) Click Apply button
   - 1.10.2) Click Save button

### **2) Create M_FatherShell Material**

#### 2.1) Create Material Asset
   - 2.1.1) Right-click in Materials folder
   - 2.1.2) Select Material
   - 2.1.3) Name: `M_FatherShell`
   - 2.1.4) Double-click to open

#### 2.2) Configure Material Properties
   - 2.2.1) Blend Mode: Translucent
   - 2.2.2) Shading Model: Unlit
   - 2.2.3) Two Sided: Checked

#### 2.3) Create Fresnel Effect

##### 2.3.1) Add Fresnel Node
   - 2.3.1.1) Right-click, search: `Fresnel`
   - 2.3.1.2) Add Fresnel node

##### 2.3.2) Add Fresnel Power Parameter
   - 2.3.2.1) Add Scalar Parameter
   - 2.3.2.2) Parameter Name: `FresnelPower`
   - 2.3.2.3) Default Value: `3.0`
   - 2.3.2.4) Connect to Fresnel Exponent In

#### 2.4) Create Edge Color Parameter

##### 2.4.1) Add Vector Parameter
   - 2.4.1.1) Add Vector Parameter node
   - 2.4.1.2) Parameter Name: `EdgeColor`
   - 2.4.1.3) Default Value: R=0.4, G=0.6, B=1.0

#### 2.5) Create Emissive Intensity

##### 2.5.1) Add Scalar Parameter
   - 2.5.1.1) Add Scalar Parameter
   - 2.5.1.2) Parameter Name: `ShellIntensity`
   - 2.5.1.3) Default Value: `20.0`

##### 2.5.2) Multiply Color by Fresnel and Intensity
   - 2.5.2.1) Add Multiply node
   - 2.5.2.2) Connect EdgeColor to A
   - 2.5.2.3) Connect Fresnel to B
   - 2.5.2.4) Add another Multiply node
   - 2.5.2.5) Connect previous result to A
   - 2.5.2.6) Connect ShellIntensity to B
   - 2.5.2.7) Connect to Emissive Color

#### 2.6) Create Opacity from Fresnel

##### 2.6.1) Add Opacity Parameter
   - 2.6.1.1) Add Scalar Parameter
   - 2.6.1.2) Parameter Name: `BaseOpacity`
   - 2.6.1.3) Default Value: `0.3`

##### 2.6.2) Combine with Fresnel
   - 2.6.2.1) Add Multiply node
   - 2.6.2.2) Connect Fresnel to A
   - 2.6.2.3) Connect BaseOpacity to B
   - 2.6.2.4) Connect to Opacity input

#### 2.7) Apply and Save
   - 2.7.1) Click Apply button
   - 2.7.2) Click Save button

### **3) Create M_FatherParticle Material**

#### 3.1) Create Material Asset
   - 3.1.1) Right-click in Materials folder
   - 3.1.2) Select Material
   - 3.1.3) Name: `M_FatherParticle`
   - 3.1.4) Double-click to open

#### 3.2) Configure Material Properties
   - 3.2.1) Blend Mode: Additive
   - 3.2.2) Shading Model: Unlit
   - 3.2.3) Two Sided: Checked

#### 3.3) Create Radial Gradient

##### 3.3.1) Add Radial Gradient
   - 3.3.1.1) Right-click, search: `RadialGradientExponential`
   - 3.3.1.2) Add RadialGradientExponential node
   - 3.3.1.3) Radius: `0.5`
   - 3.3.1.4) Density: `2.0`

#### 3.4) Create Color Parameter

##### 3.4.1) Add Vector Parameter
   - 3.4.1.1) Add Vector Parameter
   - 3.4.1.2) Parameter Name: `ParticleColor`
   - 3.4.1.3) Default Value: R=1.0, G=0.9, B=0.5

#### 3.5) Create Intensity Parameter

##### 3.5.1) Add Scalar Parameter
   - 3.5.1.1) Add Scalar Parameter
   - 3.5.1.2) Parameter Name: `ParticleIntensity`
   - 3.5.1.3) Default Value: `30.0`

#### 3.6) Combine for Emissive Output

##### 3.6.1) Multiply All Together
   - 3.6.1.1) Add Multiply node
   - 3.6.1.2) Connect ParticleColor to A
   - 3.6.1.3) Connect RadialGradient to B
   - 3.6.1.4) Add another Multiply
   - 3.6.1.5) Connect previous result to A
   - 3.6.1.6) Connect ParticleIntensity to B
   - 3.6.1.7) Connect to Emissive Color

#### 3.7) Use Gradient for Opacity

##### 3.7.1) Connect Gradient to Opacity
   - 3.7.1.1) Connect RadialGradientExponential output to Opacity input

#### 3.8) Enable Particle Color Support

##### 3.8.1) Add Particle Color Node
   - 3.8.1.1) Right-click, search: `ParticleColor`
   - 3.8.1.2) Add Particle Color node
   - 3.8.1.3) Insert Multiply before final emissive
   - 3.8.1.4) Connect Particle Color RGB to new Multiply A
   - 3.8.1.5) Connect existing color chain to B
   - 3.8.1.6) Connect result to Emissive Color

#### 3.9) Apply and Save
   - 3.9.1) Click Apply button
   - 3.9.2) Click Save button

---

## **PHASE 3: CREATE NIAGARA EMITTERS**

### **1) Create NE_FatherCore Emitter**

#### 1.1) Create Emitter Asset
   - 1.1.1) In Content Browser, navigate to `/Content/FatherCompanion/VFX/Emitters/`
   - 1.1.2) Right-click in empty space
   - 1.1.3) Select FX -> Niagara Emitter
   - 1.1.4) Select New emitter from a template
   - 1.1.5) Choose Simple Sprite Burst
   - 1.1.6) Click OK
   - 1.1.7) Name: `NE_FatherCore`
   - 1.1.8) Double-click to open Niagara Editor

#### 1.2) Configure Emitter Properties
   - 1.2.1) In Emitter Properties section
   - 1.2.2) Sim Target: CPUSim
   - 1.2.3) Local Space: Checked

#### 1.3) Configure Emitter Spawn

##### 1.3.1) Remove Spawn Burst Instantaneous
   - 1.3.1.1) In Emitter Spawn section
   - 1.3.1.2) Right-click Spawn Burst Instantaneous
   - 1.3.1.3) Select Delete

##### 1.3.2) Add Spawn Rate
   - 1.3.2.1) Click + in Emitter Spawn
   - 1.3.2.2) Search: `Spawn Rate`
   - 1.3.2.3) Select Spawn Rate
   - 1.3.2.4) Spawn Rate: `1.0`

#### 1.4) Configure Particle Spawn

##### 1.4.1) Set Lifetime
   - 1.4.1.1) In Particle Spawn section
   - 1.4.1.2) Find Initialize Particle module
   - 1.4.1.3) Lifetime Mode: Direct Set
   - 1.4.1.4) Lifetime: `999999.0`

##### 1.4.2) Set Sprite Size
   - 1.4.2.1) In Initialize Particle
   - 1.4.2.2) Sprite Size Mode: Uniform
   - 1.4.2.3) Uniform Sprite Size: `20.0`

##### 1.4.3) Set Color
   - 1.4.3.1) In Initialize Particle
   - 1.4.3.2) Color Mode: Direct Set
   - 1.4.3.3) Color: R=1.0, G=0.8, B=0.4, A=1.0

#### 1.5) Configure Render

##### 1.5.1) Change to Mesh Renderer
   - 1.5.1.1) In Render section
   - 1.5.1.2) Right-click Sprite Renderer
   - 1.5.1.3) Select Delete
   - 1.5.1.4) Click + in Render
   - 1.5.1.5) Search: `Mesh Renderer`
   - 1.5.1.6) Select Mesh Renderer

##### 1.5.2) Configure Mesh Renderer
   - 1.5.2.1) In Mesh Renderer properties
   - 1.5.2.2) Particle Mesh: Select sphere mesh (Engine content)
   - 1.5.2.3) Override Material: Checked
   - 1.5.2.4) Override Materials 0: Select M_FatherCore

#### 1.6) Add Scale Animation

##### 1.6.1) Add Scale Mesh Size Module
   - 1.6.1.1) Click + in Particle Update
   - 1.6.1.2) Search: `Scale Mesh Size`
   - 1.6.1.3) Select Scale Mesh Size
   - 1.6.1.4) Scale Factor: Make Parameter
   - 1.6.1.5) Link to User Parameter: CoreScale

#### 1.7) Apply and Save
   - 1.7.1) Click Apply
   - 1.7.2) Click Save

### **2) Create NE_FatherShell Emitter**

#### 2.1) Create Emitter Asset
   - 2.1.1) Right-click in Emitters folder
   - 2.1.2) Select FX -> Niagara Emitter
   - 2.1.3) Select New emitter from a template
   - 2.1.4) Choose Simple Sprite Burst
   - 2.1.5) Name: `NE_FatherShell`
   - 2.1.6) Double-click to open

#### 2.2) Configure Similar to Core
   - 2.2.1) Local Space: Checked
   - 2.2.2) Delete Spawn Burst, add Spawn Rate: `1.0`
   - 2.2.3) Lifetime: `999999.0`

#### 2.3) Configure Mesh Renderer
   - 2.3.1) Delete Sprite Renderer
   - 2.3.2) Add Mesh Renderer
   - 2.3.3) Particle Mesh: Sphere
   - 2.3.4) Override Material: M_FatherShell

#### 2.4) Set Larger Size
   - 2.4.1) In Initialize Particle
   - 2.4.2) Mesh Scale: (1.5, 1.5, 1.5)

#### 2.5) Apply and Save

### **3) Create NE_FatherParticles Emitter**

#### 3.1) Create Emitter Asset
   - 3.1.1) Right-click in Emitters folder
   - 3.1.2) Select FX -> Niagara Emitter
   - 3.1.3) Choose Simple Sprite Burst template
   - 3.1.4) Name: `NE_FatherParticles`
   - 3.1.5) Double-click to open

#### 3.2) Configure Emitter Properties
   - 3.2.1) Local Space: Checked

#### 3.3) Configure Spawn

##### 3.3.1) Set Spawn Rate
   - 3.3.1.1) Delete Spawn Burst Instantaneous
   - 3.3.1.2) Add Spawn Rate module
   - 3.3.1.3) Spawn Rate: `15.0`

#### 3.4) Configure Particle Spawn

##### 3.4.1) Set Lifetime
   - 3.4.1.1) Lifetime: `3.0` seconds

##### 3.4.2) Set Initial Position
   - 3.4.2.1) Click + in Particle Spawn
   - 3.4.2.2) Search: `Shape Location`
   - 3.4.2.3) Select Shape Location
   - 3.4.2.4) Shape Primitive: Sphere
   - 3.4.2.5) Sphere Radius: `25.0`
   - 3.4.2.6) Surface Only: Checked

##### 3.4.3) Set Sprite Size
   - 3.4.3.1) Sprite Size Mode: Random Uniform
   - 3.4.3.2) Sprite Size Min: `3.0`
   - 3.4.3.3) Sprite Size Max: `8.0`

##### 3.4.4) Set Initial Color
   - 3.4.4.1) Color: R=1.0, G=0.9, B=0.5, A=1.0

#### 3.5) Configure Particle Update

##### 3.5.1) Add Orbit Velocity
   - 3.5.1.1) Click + in Particle Update
   - 3.5.1.2) Search: `Point Attraction Force`
   - 3.5.1.3) Select Point Attraction Force
   - 3.5.1.4) Attraction Strength: `100.0`
   - 3.5.1.5) Attraction Radius: `50.0`
   - 3.5.1.6) Kill Radius: `5.0`

##### 3.5.2) Add Curl Noise
   - 3.5.2.1) Click + in Particle Update
   - 3.5.2.2) Search: `Curl Noise Force`
   - 3.5.2.3) Select Curl Noise Force
   - 3.5.2.4) Noise Strength: `50.0`
   - 3.5.2.5) Noise Frequency: `0.5`

##### 3.5.3) Add Scale Color Over Life
   - 3.5.3.1) Click + in Particle Update
   - 3.5.3.2) Search: `Scale Color`
   - 3.5.3.3) Select Scale Color
   - 3.5.3.4) Scale Alpha: Create curve
   - 3.5.3.5) Curve: 0->1, 0.2->1, 0.8->1, 1->0

#### 3.6) Configure Sprite Renderer
   - 3.6.1) Keep Sprite Renderer
   - 3.6.2) Material: M_FatherParticle
   - 3.6.3) Alignment: Velocity Aligned

#### 3.7) Apply and Save

### **4) Create NE_FatherAura Emitter**

#### 4.1) Create Emitter Asset
   - 4.1.1) Right-click in Emitters folder
   - 4.1.2) Select FX -> Niagara Emitter
   - 4.1.3) Choose Simple Sprite Burst template
   - 4.1.4) Name: `NE_FatherAura`
   - 4.1.5) Double-click to open

#### 4.2) Configure for Ambient Glow
   - 4.2.1) Local Space: Checked
   - 4.2.2) Delete Spawn Burst, add Spawn Rate: `1.0`
   - 4.2.3) Lifetime: `999999.0`
   - 4.2.4) Sprite Size: `120.0`
   - 4.2.5) Color: R=1.0, G=0.8, B=0.4, A=0.2

#### 4.3) Configure Sprite Renderer
   - 4.3.1) Material: M_FatherParticle
   - 4.3.2) Sort Order: -1

#### 4.4) Apply and Save

### **5) Create NE_FatherTrail Emitter**

#### 5.1) Create Emitter Asset
   - 5.1.1) Right-click in Emitters folder
   - 5.1.2) Select FX -> Niagara Emitter
   - 5.1.3) Choose Simple Ribbon template or Empty
   - 5.1.4) Name: `NE_FatherTrail`
   - 5.1.5) Double-click to open

#### 5.2) Configure Spawn Rate
   - 5.2.1) Add Spawn Rate: `30.0`

#### 5.3) Configure Particle Spawn
   - 5.3.1) Lifetime: `0.5` seconds
   - 5.3.2) Ribbon Width: `10.0`
   - 5.3.3) Color: R=1.0, G=0.8, B=0.4, A=1.0

#### 5.4) Configure Ribbon Renderer
   - 5.4.1) Delete Sprite Renderer if present
   - 5.4.2) Add Ribbon Renderer
   - 5.4.3) Material: M_FatherParticle
   - 5.4.4) Facing Mode: Screen

#### 5.5) Add Width Scale Over Life
   - 5.5.1) Add Scale Ribbon Width module
   - 5.5.2) Scale curve: 1->0 over lifetime

#### 5.6) Apply and Save

---

## **PHASE 4: CREATE NIAGARA SYSTEM**

### **1) Create NS_FatherCompanion System**

#### 1.1) Create System Asset
   - 1.1.1) In Content Browser, navigate to `/Content/FatherCompanion/VFX/`
   - 1.1.2) Right-click in empty space
   - 1.1.3) Select FX -> Niagara System
   - 1.1.4) Select New system from selected emitter(s)
   - 1.1.5) Check all father emitters (Core, Shell, Particles, Aura, Trail)
   - 1.1.6) Click OK
   - 1.1.7) Name: `NS_FatherCompanion`
   - 1.1.8) Double-click to open

#### 1.2) Configure System Properties
   - 1.2.1) In System Settings
   - 1.2.2) Warmup Time: `0.0`
   - 1.2.3) Fixed Bounds: Checked
   - 1.2.4) Fixed Bounds Min: (-100, -100, -100)
   - 1.2.5) Fixed Bounds Max: (100, 100, 100)

### **2) Add User Parameters**

#### 2.1) Create CoreScale Parameter
   - 2.1.1) In User Parameters section, click +
   - 2.1.2) Select Float
   - 2.1.3) Name: `CoreScale`
   - 2.1.4) Default Value: `1.0`

#### 2.2) Create ShellScale Parameter
   - 2.2.1) Click + in User Parameters
   - 2.2.2) Select Float
   - 2.2.3) Name: `ShellScale`
   - 2.2.4) Default Value: `1.0`

#### 2.3) Create ParticleSpawnRate Parameter
   - 2.3.1) Click + in User Parameters
   - 2.3.2) Select Float
   - 2.3.3) Name: `ParticleSpawnRate`
   - 2.3.4) Default Value: `15.0`

#### 2.4) Create AlertLevel Parameter
   - 2.4.1) Click + in User Parameters
   - 2.4.2) Select Float
   - 2.4.3) Name: `AlertLevel`
   - 2.4.4) Default Value: `0.0`

#### 2.5) Create EmotionColor Parameter
   - 2.5.1) Click + in User Parameters
   - 2.5.2) Select Linear Color
   - 2.5.3) Name: `EmotionColor`
   - 2.5.4) Default Value: R=1.0, G=0.8, B=0.4, A=1.0

### **3) Link Parameters to Emitters**

#### 3.1) Link CoreScale
   - 3.1.1) Select NE_FatherCore emitter in system
   - 3.1.2) Find Scale Mesh Size module
   - 3.1.3) Link Scale Factor to User.CoreScale

#### 3.2) Link ShellScale
   - 3.2.1) Select NE_FatherShell emitter
   - 3.2.2) Link mesh scale to User.ShellScale

#### 3.3) Link ParticleSpawnRate
   - 3.3.1) Select NE_FatherParticles emitter
   - 3.3.2) Find Spawn Rate module
   - 3.3.3) Link Spawn Rate to User.ParticleSpawnRate

### **4) Save System**
   - 4.1) Click Apply
   - 4.2) Click Save

---

## **PHASE 5: ADD VFX TO BP_FATHERCOMPANION**

### **1) Open BP_FatherCompanion**

#### 1.1) Locate Blueprint
   - 1.1.1) Navigate to `/Content/FatherCompanion/Characters/`
   - 1.1.2) Double-click BP_FatherCompanion to open

### **2) Add Niagara Component**

#### 2.1) Add Component
   - 2.1.1) In Components panel, click Add
   - 2.1.2) Search: `Niagara`
   - 2.1.3) Select Niagara Particle System Component
   - 2.1.4) Rename to: `FatherVFX`

#### 2.2) Configure Component
   - 2.2.1) In Details panel, find Niagara section
   - 2.2.2) Niagara System Asset: Select NS_FatherCompanion
   - 2.2.3) Auto Activate: Checked

#### 2.3) Set Transform
   - 2.3.1) In Transform section
   - 2.3.2) Location: (0, 0, 90)
   - 2.3.3) Attach to Mesh component

### **3) Add Point Light Component**

#### 3.1) Add Component
   - 3.1.1) Click Add in Components panel
   - 3.1.2) Search: `Point Light`
   - 3.1.3) Select Point Light Component
   - 3.1.4) Rename to: `AmbientGlow`

#### 3.2) Configure Light Properties
   - 3.2.1) Intensity: `500.0`
   - 3.2.2) Light Color: R=255, G=200, B=100
   - 3.2.3) Attenuation Radius: `150.0`
   - 3.2.4) Source Radius: `15.0`
   - 3.2.5) Mobility: Movable

#### 3.3) Set Transform
   - 3.3.1) Location: (0, 0, 90)
   - 3.3.2) Attach to Mesh component

### **4) Create VFX Control Variables**

#### Variable Table

| Variable | Type | Default | Instance Editable | Category |
|----------|------|---------|-------------------|----------|
| CorePulseSpeed | Float | 2.0 | Yes | Father VFX |
| CorePulseIntensity | Float | 0.3 | Yes | Father VFX |
| ParticleOrbitSpeed | Float | 30.0 | Yes | Father VFX |
| BaseEmissiveIntensity | Float | 100.0 | Yes | Father VFX |
| CoreColor | LinearColor | (1.0, 0.8, 0.4, 1.0) | Yes | Father VFX |
| ShellColor | LinearColor | (0.4, 0.6, 1.0, 1.0) | Yes | Father VFX |

### **5) Compile and Save**
   - 5.1) Click Compile button
   - 5.2) Click Save button

---

## **PHASE 6: IMPLEMENT STATE-BASED VISUALS**

### **1) Create UpdateVFXForForm Function**

#### 1.1) Create Function
   - 1.1.1) In My Blueprint panel, click + next to Functions
   - 1.1.2) Name: `UpdateVFXForForm`

#### 1.2) Add Form Input
   - 1.2.1) Select function in graph
   - 1.2.2) In Details panel, find Inputs section
   - 1.2.3) Click + to add input
   - 1.2.4) Name: `NewForm`
   - 1.2.5) Type: E_FatherForm

### **2) Implement Form Switch Logic**

#### 2.1) Add Switch on E_FatherForm
   - 2.1.1) Right-click in function graph
   - 2.1.2) Search: `Switch on E_FatherForm`
   - 2.1.3) Add node
   - 2.1.4) Connect NewForm input to Selection

#### 2.2) Crawler Form (Default Orb)
   - 2.2.1) From Crawler execution pin:
   - 2.2.2) Add Get FatherVFX node (component reference)
   - 2.2.3) Add Set Visibility node
   - 2.2.3.1) New Visibility: True
   - 2.2.4) Add Set Niagara Variable (Float) node
   - 2.2.4.1) Variable Name: `User.CoreScale`
   - 2.2.4.2) Value: `1.0`

#### 2.3) Armor Form (Chest Dispersal)
   - 2.3.1) From Armor execution pin:
   - 2.3.2) Add Set Visibility node
   - 2.3.2.1) New Visibility: False (main orb hidden)
   - 2.3.3) Trigger chest attachment particle effect

#### 2.4) Exoskeleton Form (Back Energy)
   - 2.4.1) From Exoskeleton execution pin:
   - 2.4.2) Add Set Visibility node
   - 2.4.2.1) New Visibility: False (main orb hidden)
   - 2.4.3) Trigger back energy trail effect

#### 2.5) Symbiote Form (Full Body Aura)
   - 2.5.1) From Symbiote execution pin:
   - 2.5.2) Add Set Visibility node
   - 2.5.2.1) New Visibility: False (main orb hidden)
   - 2.5.3) Apply full body energy aura to player

#### 2.6) Engineer Form (Turret Configuration)
   - 2.6.1) From Engineer execution pin:
   - 2.6.2) Add Set Niagara Variable (Float) node
   - 2.6.2.1) Variable Name: `User.CoreScale`
   - 2.6.2.2) Value: `0.7` (smaller turret form)

### **3) Call from OnRep_CurrentForm**

#### 3.1) Open OnRep_CurrentForm Function
   - 3.1.1) In My Blueprint panel, find OnRep_CurrentForm
   - 3.1.2) Double-click to open

#### 3.2) Add Function Call
   - 3.2.1) After existing logic
   - 3.2.2) Add UpdateVFXForForm node
   - 3.2.3) Connect CurrentForm variable to NewForm input

### **4) Compile and Save**
   - 4.1) Click Compile
   - 4.2) Click Save

---

## **PHASE 7: CREATE MATERIAL FUNCTIONS**

### **1) Create MF_EnergyPulse Material Function**

#### 1.1) Create Asset
   - 1.1.1) Right-click in Materials folder
   - 1.1.2) Select Materials and Textures -> Material Function
   - 1.1.3) Name: `MF_EnergyPulse`

#### 1.2) Add Function Inputs

| Input Name | Type | Preview Value |
|------------|------|---------------|
| Time | Scalar | 0.0 |
| PulseSpeed | Scalar | 2.0 |
| PulseIntensity | Scalar | 0.3 |

#### 1.3) Implement Pulse

##### 1.3.1) Create Sine Wave
   - 1.3.1.1) Add Multiply node (Time * PulseSpeed)
   - 1.3.1.2) Add Sine node
   - 1.3.1.3) Add remap chain (Sine * 0.5 + 0.5)

##### 1.3.2) Scale by Intensity
   - 1.3.2.1) Add Multiply node (remapped * PulseIntensity)
   - 1.3.2.2) Add base: 1 - PulseIntensity
   - 1.3.2.3) Add result: base + scaled
   - 1.3.2.4) Add Function Output: PulseValue

### **2) Create MF_FresnelGlow Material Function**

#### 2.1) Create Asset
   - 2.1.1) Name: `MF_FresnelGlow`

#### 2.2) Add Function Inputs

| Input Name | Type | Preview Value |
|------------|------|---------------|
| Color | Vector3 | (1, 0.8, 0.4) |
| Power | Scalar | 3.0 |
| Intensity | Scalar | 50.0 |

#### 2.3) Implement Glow

##### 2.3.1) Create Fresnel
   - 2.3.1.1) Add Fresnel node
   - 2.3.1.2) Connect Power to Exponent In
   - 2.3.1.3) Add Multiply (Fresnel * Color)
   - 2.3.1.4) Add Multiply (result * Intensity)
   - 2.3.1.5) Add Function Output: EmissiveColor
   - 2.3.1.6) Add Function Output: FresnelMask (raw Fresnel)

### **3) Create MF_RadialGradient Material Function**

#### 3.1) Create Asset
   - 3.1.1) Name: `MF_RadialGradient`

#### 3.2) Add Function Inputs

| Input Name | Type | Preview Value |
|------------|------|---------------|
| Radius | Scalar | 0.5 |
| Hardness | Scalar | 2.0 |
| Center | Vector2 | (0.5, 0.5) |

#### 3.3) Implement Gradient

##### 3.3.1) Calculate Distance from Center
   - 3.3.1.1) Add Texture Coordinate node
   - 3.3.1.2) Add Subtract node (TexCoord - Center)
   - 3.3.1.3) Add Length node

##### 3.3.2) Create Gradient Falloff
   - 3.3.2.1) Add Divide node (Distance / Radius)
   - 3.3.2.2) Add Power node (result ^ Hardness)
   - 3.3.2.3) Add OneMinus node
   - 3.3.2.4) Add Saturate node
   - 3.3.2.5) Add Function Output: Gradient
   - 3.3.2.6) Connect Saturate result

### **4) Update Materials to Use Functions**

#### 4.1) Update M_FatherCore

##### 4.1.1) Open M_FatherCore Material

##### 4.1.2) Replace Pulse Logic
   - 4.1.2.1) Delete existing Time/Sine/Remap nodes
   - 4.1.2.2) Add Material Function node
   - 4.1.2.3) Select MF_EnergyPulse
   - 4.1.2.4) Connect Time node to Time input
   - 4.1.2.5) Connect CorePulseSpeed parameter to PulseSpeed
   - 4.1.2.6) Connect CorePulseIntensity parameter to PulseIntensity
   - 4.1.2.7) Use PulseValue output in emissive calculation

##### 4.1.3) Replace Fresnel Logic
   - 4.1.3.1) Add MF_FresnelGlow function
   - 4.1.3.2) Connect parameters
   - 4.1.3.3) Use EmissiveColor output

#### 4.2) Update M_FatherShell

##### 4.2.1) Open M_FatherShell Material

##### 4.2.2) Use Fresnel Function
   - 4.2.2.1) Add MF_FresnelGlow function
   - 4.2.2.2) Connect EdgeColor parameter
   - 4.2.2.3) Use FresnelMask for opacity

#### 4.3) Update M_FatherParticle

##### 4.3.1) Open M_FatherParticle Material

##### 4.3.2) Use Gradient Function
   - 4.3.2.1) Add MF_RadialGradient function
   - 4.3.2.2) Connect to opacity
   - 4.3.2.3) Multiply with particle color

### **5) Compile and Save All Materials**
   - 5.1) Apply each material
   - 5.2) Save all assets

---

## ASSETS NOT SUPPORTED BY GENERATOR PLUGIN

The following assets must be created manually as they are not supported by the Blueprint Generator Plugin:

### Manual Creation Required

| Asset Type | Assets | Reason |
|------------|--------|--------|
| Materials | M_FatherCore, M_FatherShell, M_FatherParticle | Plugin does not support Material generation |
| Material Functions | MF_EnergyPulse, MF_FresnelGlow, MF_RadialGradient | Plugin does not support Material Function generation |
| Niagara Emitters | NE_FatherCore, NE_FatherShell, NE_FatherParticles, NE_FatherAura, NE_FatherTrail | Plugin does not support Niagara generation |
| Niagara System | NS_FatherCompanion | Plugin does not support Niagara generation |

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | January 2026 | Initial VFX guide created. Extracted and reorganized VFX content from BP_FatherCompanion_Implementation_Guide_v1_0. Changed integration approach from standalone Actor to NiagaraComponent on BP_FatherCompanion (NarrativeNPCCharacter). Removed audio sections. Updated plugin version to v2.2. Added form-based visual state system. Added material function library. Clarified relationship with Father_Companion_System_Setup_Guide. |

---

**END OF FATHER COMPANION VFX GUIDE**
