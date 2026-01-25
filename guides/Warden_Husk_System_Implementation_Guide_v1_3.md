# Warden Husk System Implementation Guide
## Two-Phase Enemy with Death Transition
## Version 1.3

---

## DOCUMENT INFORMATION

| Field | Value |
|-------|-------|
| Document Type | Implementation Guide |
| System | Warden Husk System |
| Last Updated | January 2026 |
| Unreal Engine | 5.7 |
| Narrative Pro | v2.2 |

---

## TABLE OF CONTENTS

| Phase | Name |
|-------|------|
| PHASE 1 | VERIFY GAMEPLAY TAGS |
| PHASE 2 | CREATE NPCDEFINITION ASSETS |
| PHASE 3 | CREATE CORE PROJECTILE (BP_CoreLaserProjectile) |
| PHASE 4 | CREATE GAMEPLAY EFFECTS |
| PHASE 5 | CREATE BP_WARDENCORE (FLYING PHASE) |
| PHASE 6 | CREATE BP_WARDENHUSK (MELEE PHASE) |
| PHASE 7 | CREATE ABILITY CONFIGURATIONS |
| PHASE 8 | CREATE ACTIVITY CONFIGURATIONS |
| PHASE 9 | CONFIGURE NPCDEFINITIONS |
| PHASE 10 | WORLD PLACEMENT |

---

## INTRODUCTION

### System Overview

| Aspect | Description |
|--------|-------------|
| Concept | Humanoid guard animated by internal control core mounted in chest |
| Phase 1 | Slow, armored, uses heavy melee attacks |
| Transition | When health depletes, chest ruptures, core ejects |
| Phase 2 | Core becomes flying enemy with laser attacks |
| Visual | Husk collapses, glowing core emerges and flies |

### Phase Specifications

| Phase | Entity | Movement | Combat | Durability |
|-------|--------|----------|--------|------------|
| 1 | BP_WardenHusk | Slow walking | Heavy melee | High health, armored |
| 2 | BP_WardenCore | True 3D flying | Laser ranged | Fragile, high damage |

### System Flow

| Step | Action | Result |
|------|--------|--------|
| 1 | Player damages BP_WardenHusk | Health decreases |
| 2 | Husk health reaches zero | OnDied delegate fires |
| 3 | HandleDeath override executes | Ejection sequence begins |
| 4 | Play ejection animation | Visual feedback |
| 5 | Spawn BP_WardenCore | New flying enemy created |
| 6 | Core sets Flying movement mode | 3D flight enabled |
| 7 | Original husk ragdolls | Phase transition complete |
| 8 | Core AI begins laser attacks | Aggressive combat |

### Design Philosophy

| Principle | Implementation |
|-----------|----------------|
| Multi-Phase Threat | Two distinct combat challenges |
| Dependency Pattern | Must destroy core to truly kill enemy |
| Psychological Pattern | Unexpected second phase surprises player |
| Risk/Reward | Core is fragile but deadly if ignored |

---

## QUICK REFERENCE

### Gameplay Tags

| Tag | Purpose |
|-----|---------|
| State.Warden.Husk | Applied to husk entity |
| State.Warden.Core | Applied to flying core |
| State.Warden.Ejecting | During transition phase |
| Faction.Enemy.Warden | Faction identifier |
| Ability.Warden.CoreLaser | Laser attack ability |
| Cooldown.Warden.CoreLaser | Laser cooldown tag |
| Effect.Damage.WardenLaser | Laser damage type |

### Assets Created

| Asset | Type | Location |
|-------|------|----------|
| BP_WardenHusk | Blueprint Class | /Game/Enemies/Warden/ |
| BP_WardenCore | Blueprint Class | /Game/Enemies/Warden/ |
| BP_CoreLaserProjectile | Blueprint Class | /Game/Enemies/Warden/ |
| NPC_WardenHusk | NPCDefinition | /Game/Enemies/Warden/ |
| NPC_WardenCore | NPCDefinition | /Game/Enemies/Warden/ |
| AC_WardenHusk | AbilityConfiguration | /Game/Enemies/Warden/ |
| AC_WardenCore | AbilityConfiguration | /Game/Enemies/Warden/ |
| AC_WardenHuskBehavior | ActivityConfiguration | /Game/Enemies/Warden/ |
| AC_WardenCoreBehavior | ActivityConfiguration | /Game/Enemies/Warden/ |
| GA_CoreLaser | GameplayAbility | /Game/Enemies/Warden/ |
| GE_CoreLaserDamage | GameplayEffect | /Game/Enemies/Warden/ |
| GE_CoreLaserCooldown | GameplayEffect | /Game/Enemies/Warden/ |
| GE_WardenHuskAttributes | GameplayEffect | /Game/Enemies/Warden/ |
| GE_WardenCoreAttributes | GameplayEffect | /Game/Enemies/Warden/ |

> **NAMING CONVENTION:** Narrative Pro uses `NPC_` prefix for NPCDefinition assets (not `NPCDef_`). ActivityConfiguration assets use `AC_*Behavior` suffix to distinguish from AbilityConfiguration which uses plain `AC_` prefix.

### BP_WardenHusk Variables

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| CoreDefinition | NPCDefinition (Object Reference) | NPC_WardenCore | Yes |
| SpawnOffset | Vector | (0, 0, 100) | Yes |
| EjectionMontage | AnimMontage (Object Reference) | None | Yes |
| bSpawnCorpse | Boolean | false | Yes |
| CorpseMesh | Static Mesh (Object Reference) | None | Yes |

### BP_WardenCore Variables

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| FlightSpeed | Float | 600.0 | Yes |
| FlightAltitude | Float | 300.0 | Yes |
| LaserDamage | Float | 50.0 | Yes |
| LaserCooldown | Float | 1.5 | Yes |
| LaserRange | Float | 2000.0 | Yes |
| ProjectileClass | Class (NarrativeProjectile) | BP_CoreLaserProjectile | Yes |
| LaserSpawnOffset | Vector | (50, 0, 0) | Yes |

### GA_CoreLaser Variables

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| ProjectileClass | Class (NarrativeProjectile) | BP_CoreLaserProjectile | Yes |
| MaxRange | Float | 2000.0 | Yes |
| LaserSpawnOffset | Vector | (50, 0, 0) | Yes |
| CoreRef | BP_WardenCore (Object Reference) | None | No |
| TargetRef | Character (Object Reference) | None | No |

---

## PHASE 1: VERIFY GAMEPLAY TAGS

### Create Required Tags

| Tag Name | Purpose |
|----------|---------|
| State.Warden.Husk | Applied to husk entity |
| State.Warden.Core | Applied to flying core |
| State.Warden.Ejecting | During transition phase |
| Faction.Enemy.Warden | Faction identifier |
| Ability.Warden.CoreLaser | Laser attack ability |
| Cooldown.Warden.CoreLaser | Laser cooldown tag |
| Effect.Damage.WardenLaser | Laser damage type |

---

## PHASE 2: CREATE NPCDEFINITION ASSETS

### **1) Create Folder Structure**

#### 1.1) Navigate to Content Browser
   - 1.1.1) Content Browser -> /Game/Enemies/
   - 1.1.2) Right-click in empty space
   - 1.1.3) Select: New Folder
   - 1.1.4) Name: Warden
   - 1.1.5) Open Warden folder

### **2) Create NPC_WardenHusk**

#### 2.1) Create NPCDefinition Asset
   - 2.1.1) Right-click in Content Browser
   - 2.1.2) Select: Narrative -> NPC Definition
   - 2.1.3) Name: NPC_WardenHusk

### **3) Create NPC_WardenCore**

#### 3.1) Create NPCDefinition Asset
   - 3.1.1) Right-click in Content Browser
   - 3.1.2) Select: Narrative -> NPC Definition
   - 3.1.3) Name: NPC_WardenCore

---

## PHASE 3: CREATE CORE PROJECTILE (BP_CoreLaserProjectile)

### **4) Create Projectile Blueprint**

#### 4.1) Create NarrativeProjectile Blueprint
   - 4.1.1) Right-click in Content Browser
   - 4.1.2) Select: Blueprint Class
   - 4.1.3) Click All Classes dropdown
   - 4.1.4) Search: NarrativeProjectile
   - 4.1.5) Select: NarrativeProjectile
   - 4.1.6) Click: Select button
   - 4.1.7) Name: BP_CoreLaserProjectile
   - 4.1.8) Double-click BP_CoreLaserProjectile to open

#### 4.2) Configure Actor Replication
   - 4.2.1) Click Class Defaults button
   - 4.2.2) In Details panel, find Replication section
   - 4.2.3) Replicates: Check
   - 4.2.4) Replicate Movement: Check

### **5) Add Projectile Components**

#### 5.1) Add Static Mesh Component
   - 5.1.1) In Components panel, click Add button
   - 5.1.2) Search: Static Mesh
   - 5.1.3) Select: Static Mesh Component
   - 5.1.4) Rename to: LaserMesh
   - 5.1.5) Select LaserMesh in hierarchy
   - 5.1.6) In Details panel:
   - 5.1.6.1) Static Mesh: Select Sphere or custom mesh
   - 5.1.6.2) Scale: (0.2, 0.2, 0.5)
   - 5.1.6.3) Collision Presets: NoCollision

#### 5.2) Create Emissive Material
   - 5.2.1) In Content Browser, right-click
   - 5.2.2) Select: Material
   - 5.2.3) Name: M_CoreLaserGlow
   - 5.2.4) Open material editor
   - 5.2.5) Add Constant3Vector node
   - 5.2.6) Set color: R=0.2, G=0.8, B=1.0 (cyan)
   - 5.2.7) Add Multiply node
   - 5.2.8) Connect color to A input
   - 5.2.9) Add Constant node with value: 75
   - 5.2.10) Connect constant to B input
   - 5.2.11) Connect Multiply output to Emissive Color
   - 5.2.12) Click Apply and Save
   - 5.2.13) Back in BP_CoreLaserProjectile:
   - 5.2.13.1) Select LaserMesh
   - 5.2.13.2) Material: Select M_CoreLaserGlow

#### 5.3) Add Collision Component
   - 5.3.1) In Components panel, click Add
   - 5.3.2) Search: Sphere Collision
   - 5.3.3) Select: Sphere Collision
   - 5.3.4) Rename to: DamageCollision
   - 5.3.5) Drag DamageCollision onto DefaultSceneRoot
   - 5.3.6) Select DamageCollision, in Details panel:
   - 5.3.6.1) Sphere Radius: 25.0
   - 5.3.6.2) Collision Presets: Custom
   - 5.3.6.3) Collision Enabled: Query Only (No Physics)
   - 5.3.6.4) Expand Collision Responses:
   - 5.3.6.4.1) Set all to Ignore
   - 5.3.6.4.2) Set Pawn to Overlap

#### 5.4) Add Projectile Movement Component
   - 5.4.1) Click Add in Components panel
   - 5.4.2) Search: Projectile Movement
   - 5.4.3) Select: Projectile Movement Component
   - 5.4.4) In Details panel:
   - 5.4.4.1) Initial Speed: 6000.0
   - 5.4.4.2) Max Speed: 6000.0
   - 5.4.4.3) Projectile Gravity Scale: 0.0
   - 5.4.4.4) Rotation Follows Velocity: Check
   - 5.4.4.5) Should Bounce: Uncheck

### **6) Create Projectile Variables**

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| HasHitTarget | Boolean | false | No |

### **7) Implement Projectile BeginPlay**

#### 7.1) Set Lifespan
   - 7.1.1) Click Event Graph tab
   - 7.1.2) Find Event BeginPlay node
   - 7.1.3) From Event BeginPlay:
   - 7.1.3.1) Add Set Life Span node
   - 7.1.3.2) In Life Span: 4.0

### **8) Implement Overlap Detection with SetProjectileTargetData**

#### 8.1) Create OnComponentBeginOverlap Event
   - 8.1.1) Select DamageCollision in Components panel
   - 8.1.2) In Details panel, scroll to Events section
   - 8.1.3) Click + next to On Component Begin Overlap

#### 8.2) Prevent Multiple Hits
   - 8.2.1) From On Component Begin Overlap:
   - 8.2.1.1) Add Branch node
   - 8.2.2) From HasHitTarget variable:
   - 8.2.2.1) Add NOT Boolean node
   - 8.2.2.2) Connect HasHitTarget to NOT input
   - 8.2.3) Connect NOT output to Branch Condition

#### 8.3) Validate Hit Target
   - 8.3.1) From Branch True pin:
   - 8.3.1.1) Add Cast To Character node
   - 8.3.1.2) Connect Other Actor to Object input
   - 8.3.2) From Cast successful execution:
   - 8.3.2.1) Add Get Narrative Character node
   - 8.3.2.2) Target: Self (inherited from NarrativeProjectile)
   - 8.3.3) From Get Narrative Character Return Value:
   - 8.3.3.1) Add Not Equal (Object) node
   - 8.3.3.2) Connect As Character (from cast) to B input

#### 8.4) Set Hit Flag and Call Target Data
   - 8.4.1) From Not Equal True result:
   - 8.4.1.1) Add Branch node
   - 8.4.2) From Branch True:
   - 8.4.2.1) Add Set HasHitTarget node
   - 8.4.2.2) HasHitTarget: Check (true)
   - 8.4.3) From Set HasHitTarget:
   - 8.4.3.1) Add Set Projectile Target Data node
   - 8.4.3.2) Target: Self
   - 8.4.3.3) Hit Result: Connect Hit Result from overlap event
   - 8.4.3.4) Target Actor: Connect As Character from cast

#### 8.5) Destroy Projectile
   - 8.5.1) From Set Projectile Target Data:
   - 8.5.1.1) Add Destroy Actor node
   - 8.5.1.2) Target: Self

### **9) Compile and Save Projectile**
   - 9.1) Click Compile
   - 9.2) Click Save

---

## PHASE 4: CREATE GAMEPLAY EFFECTS

### **10) Create GE_CoreLaserDamage**

#### 10.1) Create GameplayEffect Asset
   - 10.1.1) Right-click in Content Browser
   - 10.1.2) Select: Gameplay -> Gameplay Effect
   - 10.1.3) Name: GE_CoreLaserDamage
   - 10.1.4) Double-click to open

#### 10.2) Configure Effect Properties

| Property | Value |
|----------|-------|
| Duration Policy | Instant |

#### 10.3) Add Executions Component
   - 10.3.1) In Details panel, find Components section
   - 10.3.2) Click + to add component
   - 10.3.3) Select: Gameplay Effect Executions
   - 10.3.4) Expand Execution Definitions array
   - 10.3.5) Click + to add element
   - 10.3.6) Calculation Class: NarrativeDamageExecCalc

#### 10.4) Configure SetByCaller Magnitude
   - 10.4.1) In Execution Definitions [0]:
   - 10.4.2) Expand: Passed In Tags
   - 10.4.3) Click + to add element
   - 10.4.4) Set: SetByCaller.Damage

#### 10.5) Add Asset Tags Component
   - 10.5.1) Click + to add component
   - 10.5.2) Select: Asset Tags Gameplay Effect Component
   - 10.5.3) Combined Asset Tags: Effect.Damage.WardenLaser

#### 10.6) Compile and Save

### **11) Create GE_CoreLaserCooldown**

#### 11.1) Create GameplayEffect Asset
   - 11.1.1) Right-click in Content Browser
   - 11.1.2) Select: Gameplay -> Gameplay Effect
   - 11.1.3) Name: GE_CoreLaserCooldown
   - 11.1.4) Double-click to open

#### 11.2) Configure Effect Properties

| Property | Value |
|----------|-------|
| Duration Policy | Has Duration |
| Duration Magnitude -> Magnitude Calculation Type | Scalable Float |
| Duration Magnitude -> Scalable Float Magnitude | 1.5 |

#### 11.3) Add Granted Tags Component
   - 11.3.1) Click + to add component
   - 11.3.2) Select: Granted Tags Gameplay Effect Component
   - 11.3.3) Grant Tags On Apply: Cooldown.Warden.CoreLaser

#### 11.4) Compile and Save

### **12) Create GE_WardenHuskAttributes**

#### 12.1) Create GameplayEffect Asset
   - 12.1.1) Right-click in Content Browser
   - 12.1.2) Select: Gameplay -> Gameplay Effect
   - 12.1.3) Name: GE_WardenHuskAttributes
   - 12.1.4) Double-click to open

#### 12.2) Configure Effect Properties

| Property | Value |
|----------|-------|
| Duration Policy | Instant |

#### 12.3) Add Modifiers

| Attribute | Modifier Op | Magnitude Type | Magnitude |
|-----------|-------------|----------------|-----------|
| MaxHealth | Override | Scalable Float | 500.0 |
| Health | Override | Scalable Float | 500.0 |
| AttackDamage | Override | Scalable Float | 35.0 |
| Defense | Override | Scalable Float | 25.0 |

#### 12.4) Compile and Save

### **13) Create GE_WardenCoreAttributes**

#### 13.1) Create GameplayEffect Asset
   - 13.1.1) Right-click in Content Browser
   - 13.1.2) Select: Gameplay -> Gameplay Effect
   - 13.1.3) Name: GE_WardenCoreAttributes
   - 13.1.4) Double-click to open

#### 13.2) Configure Effect Properties

| Property | Value |
|----------|-------|
| Duration Policy | Instant |

#### 13.3) Add Modifiers

| Attribute | Modifier Op | Magnitude Type | Magnitude |
|-----------|-------------|----------------|-----------|
| MaxHealth | Override | Scalable Float | 75.0 |
| Health | Override | Scalable Float | 75.0 |
| AttackDamage | Override | Scalable Float | 50.0 |
| Defense | Override | Scalable Float | 0.0 |

#### 13.4) Compile and Save

---

## PHASE 5: CREATE BP_WARDENCORE (FLYING PHASE)

### **14) Create Core Blueprint**

#### 14.1) Create NarrativeNPCCharacter Blueprint
   - 14.1.1) Right-click in Content Browser
   - 14.1.2) Select: Blueprint Class
   - 14.1.3) Click All Classes dropdown
   - 14.1.4) Search: NarrativeNPCCharacter
   - 14.1.5) Select: NarrativeNPCCharacter
   - 14.1.6) Click: Select button
   - 14.1.7) Name: BP_WardenCore
   - 14.1.8) Double-click to open

### **15) Configure Actor Replication**

#### 15.1) Set Replication Settings
   - 15.1.1) Click Class Defaults button
   - 15.1.2) In Details panel, find Replication section
   - 15.1.3) Replicates: Check
   - 15.1.4) Replicate Movement: Check

### **16) Configure Character Movement for Flying**

#### 16.1) Select CharacterMovement Component
   - 16.1.1) In Components panel, select CharacterMovement
   - 16.1.2) In Details panel:
   - 16.1.2.1) Default Land Movement Mode: Flying
   - 16.1.2.2) Max Fly Speed: 600.0
   - 16.1.2.3) Braking Deceleration Flying: 1000.0
   - 16.1.2.4) Nav Agent Radius: 50.0
   - 16.1.2.5) Nav Agent Height: 100.0

### **17) Create Core Variables**

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| FlightSpeed | Float | 600.0 | Yes |
| FlightAltitude | Float | 300.0 | Yes |
| LaserDamage | Float | 50.0 | Yes |
| LaserCooldown | Float | 1.5 | Yes |
| LaserRange | Float | 2000.0 | Yes |
| ProjectileClass | Class (NarrativeProjectile) | BP_CoreLaserProjectile | Yes |
| LaserSpawnOffset | Vector | (50, 0, 0) | Yes |

### **18) Configure Visual Components**

#### 18.1) Select Mesh Component
   - 18.1.1) In Components panel, select Mesh component
   - 18.1.2) In Details panel:
   - 18.1.2.1) Skeletal Mesh: Select core mesh or placeholder sphere

#### 18.2) Add Point Light Component
   - 18.2.1) In Components panel, click Add
   - 18.2.2) Search: Point Light
   - 18.2.3) Select: Point Light
   - 18.2.4) In Details panel:
   - 18.2.4.1) Light Color: Cyan (R=0.2, G=0.8, B=1.0)
   - 18.2.4.2) Intensity: 5000.0
   - 18.2.4.3) Attenuation Radius: 200.0

#### 18.3) Adjust Capsule Size
   - 18.3.1) Select Capsule Component
   - 18.3.2) In Details panel:
   - 18.3.2.1) Capsule Radius: 40.0
   - 18.3.2.2) Capsule Half Height: 50.0

### **19) Implement BeginPlay to Ensure Flying Mode**

#### 19.1) Override BeginPlay
   - 19.1.1) Click Event Graph tab
   - 19.1.2) Find Event BeginPlay node

#### 19.2) Set Flying Movement Mode
   - 19.2.1) From Event BeginPlay:
   - 19.2.1.1) Add Get Character Movement node
   - 19.2.2) From Get Character Movement Return Value:
   - 19.2.2.1) Add Set Movement Mode node
   - 19.2.2.2) New Movement Mode: Flying
   - 19.2.3) From Set Movement Mode:
   - 19.2.3.1) Add Call to Parent Function node

### **20) Compile and Save Core Blueprint**
   - 20.1) Click Compile
   - 20.2) Click Save

---

## PHASE 6: CREATE BP_WARDENHUSK (MELEE PHASE)

### **21) Create Husk Blueprint**

#### 21.1) Create NarrativeNPCCharacter Blueprint
   - 21.1.1) Right-click in Content Browser
   - 21.1.2) Select: Blueprint Class
   - 21.1.3) Click All Classes dropdown
   - 21.1.4) Search: NarrativeNPCCharacter
   - 21.1.5) Select: NarrativeNPCCharacter
   - 21.1.6) Click: Select button
   - 21.1.7) Name: BP_WardenHusk
   - 21.1.8) Double-click to open

### **22) Configure Actor Replication**

#### 22.1) Set Replication Settings
   - 22.1.1) Click Class Defaults button
   - 22.1.2) In Details panel, find Replication section
   - 22.1.3) Replicates: Check
   - 22.1.4) Replicate Movement: Check

### **23) Configure Character Movement for Slow Walking**

#### 23.1) Select CharacterMovement Component
   - 23.1.1) In Components panel, select CharacterMovement
   - 23.1.2) In Details panel:
   - 23.1.2.1) Max Walk Speed: 200.0
   - 23.1.2.2) Max Acceleration: 500.0

### **24) Create Husk Variables**

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| CoreDefinition | NPCDefinition (Object Reference) | NPC_WardenCore | Yes |
| SpawnOffset | Vector | (0, 0, 100) | Yes |
| EjectionMontage | AnimMontage (Object Reference) | None | Yes |
| bSpawnCorpse | Boolean | false | Yes |
| CorpseMesh | Static Mesh (Object Reference) | None | Yes |

### **25) Override HandleDeath Function**

#### 25.1) Find HandleDeath
   - 25.1.1) In My Blueprint panel, find Functions section
   - 25.1.2) Expand Overrides dropdown
   - 25.1.3) Click Handle Death
   - 25.1.4) Function graph opens

### **26) Implement Death Transition Logic**

#### 26.1) Check Server Authority
   - 26.1.1) From Handle Death entry node:
   - 26.1.1.1) Add Has Authority node
   - 26.1.2) From Has Authority Return Value:
   - 26.1.2.1) Add Branch node
   - 26.1.2.2) Connect Return Value to Condition

### **27) Get NarrativeCharacterSubsystem**

#### 27.1) Get Subsystem Reference
   - 27.1.1) From Branch True pin:
   - 27.1.1.1) Add Get NarrativeCharacterSubsystem node
   - 27.1.2) From Return Value:
   - 27.1.2.1) Add Is Valid node
   - 27.1.3) From Is Valid -> Is Valid pin:
   - 27.1.3.1) Add Branch node

### **28) Validate Core Definition**

#### 28.1) Check Definition Exists
   - 28.1.1) From Branch True pin:
   - 28.1.1.1) Drag CoreDefinition variable getter
   - 28.1.1.2) Add Is Valid node
   - 28.1.1.3) Connect CoreDefinition to Object input
   - 28.1.2) From Is Valid -> Is Valid pin:
   - 28.1.2.1) Add Branch node

### **29) Play Ejection Animation (Optional)**

#### 29.1) Check Montage Valid
   - 29.1.1) From Branch True pin (definition valid):
   - 29.1.1.1) Drag EjectionMontage variable getter
   - 29.1.1.2) Add Is Valid node
   - 29.1.2) From Is Valid -> Is Valid pin:
   - 29.1.2.1) Add Branch node

#### 29.2) Play Montage
   - 29.2.1) From Branch True pin:
   - 29.2.1.1) Add Play Montage node
   - 29.2.1.2) Montage to Play: Connect EjectionMontage variable
   - 29.2.1.3) Target: Self
   - 29.2.2) From Branch False pin:
   - 29.2.2.1) Route to spawn logic (bypasses animation)

### **30) Calculate Spawn Transform**

#### 30.1) Get Husk Location
   - 30.1.1) From Play Montage On Completed (or Branch False):
   - 30.1.1.1) Add Get Actor Location node
   - 30.1.1.2) Target: Self

#### 30.2) Add Spawn Offset
   - 30.2.1) Drag SpawnOffset variable getter
   - 30.2.2) Add Add (Vector + Vector) node
   - 30.2.3) Connect:
   - 30.2.3.1) A: Get Actor Location Return Value
   - 30.2.3.2) B: SpawnOffset

#### 30.3) Get Husk Rotation
   - 30.3.1) Add Get Actor Rotation node
   - 30.3.2) Target: Self

#### 30.4) Make Spawn Transform
   - 30.4.1) Add Make Transform node
   - 30.4.2) Connect:
   - 30.4.2.1) Location: Vector addition result
   - 30.4.2.2) Rotation: Get Actor Rotation Return Value
   - 30.4.2.3) Scale: Leave default (1, 1, 1)

### **31) Spawn Core Using NarrativeCharacterSubsystem**

> **CRITICAL:** You MUST use `NarrativeCharacterSubsystem.SpawnNPC()` - NOT raw `SpawnActor`.
>
> **Why?** Raw SpawnActor does NOT call `SetNPCDefinition()` on the spawned NPC. Without this:
> - NPCDefinition property is null
> - AbilityConfiguration is never applied (no abilities granted)
> - ActivityConfiguration is never applied (no AI activities)
> - Faction tags are never set
> - NPC is not registered in the subsystem
>
> The spawned Core would be a non-functional shell. SpawnNPC handles all initialization automatically.

#### 31.1) Call SpawnNPC
   - 31.1.1) From the World Subsystem result:
   - 31.1.1.1) Add Spawn NPC node
   - 31.1.2) Connect:
   - 31.1.2.1) Target: NarrativeCharacterSubsystem reference
   - 31.1.2.2) NPCData: CoreDefinition variable
   - 31.1.2.3) Transform: Make Transform result

#### 31.2) Store Spawned Core Reference
   - 31.2.1) From Spawn NPC Return Value:
   - 31.2.1.1) Right-click -> Promote to Local Variable
   - 31.2.1.2) Name: SpawnedCore

### **32) Spawn Corpse Mesh (Optional)**

#### 32.1) Check bSpawnCorpse Flag
   - 32.1.1) Drag bSpawnCorpse variable getter
   - 32.1.2) Add Branch node
   - 32.1.3) Connect bSpawnCorpse to Condition

#### 32.2) Check CorpseMesh Valid
   - 32.2.1) From Branch True pin:
   - 32.2.1.1) Drag CorpseMesh variable getter
   - 32.2.1.2) Add Is Valid node
   - 32.2.2) From Is Valid -> Is Valid pin:
   - 32.2.2.1) Add Branch node

#### 32.3) Spawn Static Mesh Actor
   - 32.3.1) From Branch True pin:
   - 32.3.1.1) Add Spawn Actor from Class node
   - 32.3.1.2) Class: StaticMeshActor
   - 32.3.1.3) Spawn Transform: Get Actor Transform (from self)
   - 32.3.1.4) Collision Handling Override: Always Spawn

#### 32.4) Configure Corpse Mesh
   - 32.4.1) From Spawn Actor Return Value:
   - 32.4.1.1) Add Cast To StaticMeshActor node
   - 32.4.2) From successful cast:
   - 32.4.2.1) Add Get Static Mesh Component node
   - 32.4.2.2) Target: As Static Mesh Actor
   - 32.4.3) From Get Static Mesh Component Return Value:
   - 32.4.3.1) Add Set Static Mesh node
   - 32.4.3.2) New Mesh: CorpseMesh variable

#### 33.5) Enable Corpse Physics
   - 33.5.1) From Set Static Mesh execution:
   - 33.5.1.1) Add Set Simulate Physics node
   - 33.5.1.2) Target: Static Mesh Component
   - 33.5.1.3) Simulate: Check (true)

### **33) Call Parent HandleDeath**

#### 33.1) Execute Parent Implementation
   - 33.1.1) From final execution pin:
   - 33.1.1.1) Add Call to Parent Function node

### **34) Compile and Save Husk Blueprint**
   - 34.1) Click Compile
   - 34.2) Click Save

---

## PHASE 7: CREATE ABILITY CONFIGURATIONS

### **36) Create AC_WardenHusk**

#### 36.1) Create AbilityConfiguration Asset
   - 36.1.1) Right-click in Content Browser
   - 36.1.2) Select: Narrative -> Ability Configuration
   - 36.1.3) Name: AC_WardenHusk
   - 36.1.4) Double-click to open

#### 36.2) Configure Default Attributes
   - 36.2.1) In Details panel, find Default Attributes
   - 36.2.2) Set: GE_WardenHuskAttributes

#### 36.3) Configure Abilities to Grant
   - 36.3.1) Expand: Default Abilities
   - 36.3.2) Add standard NPC abilities as needed:
   - 36.3.2.1) GA_Death (if exists)

#### 36.4) Save Asset

### **37) Create AC_WardenCore**

#### 37.1) Create AbilityConfiguration Asset
   - 37.1.1) Right-click in Content Browser
   - 37.1.2) Select: Narrative -> Ability Configuration
   - 37.1.3) Name: AC_WardenCore
   - 37.1.4) Double-click to open

#### 37.2) Configure Default Attributes
   - 37.2.1) In Details panel, find Default Attributes
   - 37.2.2) Set: GE_WardenCoreAttributes

#### 37.3) Configure Abilities to Grant
   - 37.3.1) Expand: Default Abilities
   - 37.3.2) Add: GA_CoreLaser (created in Phase 8)

#### 37.4) Save Asset

---

## PHASE 8: CREATE ACTIVITY CONFIGURATIONS

> **NOTE:** ActivityConfiguration assets use `AC_*Behavior` suffix to distinguish from AbilityConfiguration which uses plain `AC_` prefix. Both asset types share the `AC_` prefix but serve different purposes.

### **38) Create AC_WardenHuskBehavior**

#### 38.1) Create ActivityConfiguration Asset
   - 38.1.1) Right-click in Content Browser
   - 38.1.2) Select: Narrative -> NPC Activity Configuration
   - 38.1.3) Name: AC_WardenHuskBehavior
   - 38.1.4) Double-click to open

#### 38.2) Configure Default Activities
   - 38.2.1) Expand: Default Activities
   - 38.2.2) Add existing melee activities:
   - 38.2.2.1) BPA_Attack_Melee
   - 38.2.2.2) BPA_Wander
   - 38.2.2.3) BPA_Idle

#### 38.3) Configure Goal Generators
   - 38.3.1) Expand: Goal Generators
   - 38.3.2) Add: GoalGenerator_Attack

#### 38.4) Save Asset

### **39) Create AC_WardenCoreBehavior**

#### 39.1) Create ActivityConfiguration Asset
   - 39.1.1) Right-click in Content Browser
   - 39.1.2) Select: Narrative -> NPC Activity Configuration
   - 39.1.3) Name: AC_WardenCoreBehavior
   - 39.1.4) Double-click to open

#### 39.2) Configure Default Activities
   - 39.2.1) Expand: Default Activities
   - 39.2.2) Add ranged activities:
   - 39.2.2.1) BPA_Attack_RangedStand_Stationary (or similar)
   - 39.2.2.2) BPA_Wander
   - 39.2.2.3) BPA_Idle

#### 39.3) Configure Goal Generators
   - 39.3.1) Expand: Goal Generators
   - 39.3.2) Add: GoalGenerator_Attack

#### 39.4) Save Asset

---

## PHASE 9: CONFIGURE NPCDEFINITIONS

### **40) Configure NPC_WardenHusk**

#### 40.1) Open NPCDefinition
   - 40.1.1) Double-click NPC_WardenHusk

#### 40.2) Configure Basic Properties
   - 40.2.1) NPC Name: Warden Husk
   - 40.2.2) NPC Class Path: Select BP_WardenHusk

#### 40.3) Configure Configurations
   - 40.3.1) Ability Configuration: AC_WardenHusk
   - 40.3.2) Activity Configuration: AC_WardenHuskBehavior

#### 40.4) Configure Faction
   - 40.4.1) Expand: Factions array
   - 40.4.2) Add: Faction.Enemy.Warden

#### 40.5) Save Asset

### **41) Configure NPC_WardenCore**

#### 41.1) Open NPCDefinition
   - 41.1.1) Double-click NPC_WardenCore

#### 41.2) Configure Basic Properties
   - 41.2.1) NPC Name: Warden Core
   - 41.2.2) NPC Class Path: Select BP_WardenCore

#### 41.3) Configure Configurations
   - 41.3.1) Ability Configuration: AC_WardenCore
   - 41.3.2) Activity Configuration: AC_WardenCoreBehavior

#### 41.4) Configure Faction
   - 41.4.1) Expand: Factions array
   - 41.4.2) Add: Faction.Enemy.Warden

#### 41.5) Save Asset

---

## PHASE 10: WORLD PLACEMENT

### **42) Configure Faction Attitudes**

#### 42.1) Open Faction Settings
   - 42.1.1) Edit -> Project Settings
   - 42.1.2) Navigate to: Narrative -> Faction Settings

#### 42.2) Add Warden Faction
   - 42.2.1) Find Faction Attitudes array
   - 42.2.2) Add entry for Faction.Enemy.Warden
   - 42.2.3) Set attitude toward Narrative.Factions.Heroes: Hostile

### **43) Place Husk in Level**

#### 43.1) Drag NPCDefinition to Level
   - 43.1.1) In Content Browser, find NPC_WardenHusk
   - 43.1.2) Drag and drop into level viewport
   - 43.1.3) NPCSpawner actor created automatically

#### 43.2) Configure Spawner (Optional)
   - 43.2.1) Select NPCSpawner in level
   - 43.2.2) In Details panel:
   - 43.2.2.1) Spawn On Begin Play: Check (for testing)
   - 43.2.2.2) Respawn: Configure as needed

### **44) Verify Core Definition Reference**

#### 44.1) Open BP_WardenHusk Defaults
   - 44.1.1) In Content Browser, find BP_WardenHusk
   - 44.1.2) Right-click -> Edit Defaults

#### 44.2) Verify CoreDefinition
   - 44.2.1) Find CoreDefinition variable
   - 44.2.2) Ensure set to: NPC_WardenCore

---

## OPTIONAL: CREATE GA_CORELASER ABILITY

### **45) Create Laser Ability Blueprint**

#### 45.1) Create NarrativeGameplayAbility Blueprint
   - 45.1.1) Right-click in Content Browser
   - 45.1.2) Select: Blueprint Class
   - 45.1.3) Expand All Classes section
   - 45.1.4) Search: NarrativeGameplayAbility
   - 45.1.5) Select: NarrativeGameplayAbility
   - 45.1.6) Click: Select button
   - 45.1.7) Name: GA_CoreLaser
   - 45.1.8) Double-click to open

### **46) Configure Ability Tags**

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Warden.CoreLaser |
| Activation Required Tags | State.Warden.Core |
| Activation Blocked Tags | Cooldown.Warden.CoreLaser |

### **47) Configure Replication Settings**

#### 47.1) Set Replication Properties
   - 47.1.1) Click Class Defaults button
   - 47.1.2) Replication Policy: Replicate
   - 47.1.3) Net Execution Policy: Server Only
   - 47.1.4) Instancing Policy: Instanced Per Actor
   - 47.1.5) Replicate Input Directly: Uncheck

### **48) Create Ability Variables**

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| ProjectileClass | Class (NarrativeProjectile) | BP_CoreLaserProjectile | Yes |
| MaxRange | Float | 2000.0 | Yes |
| LaserSpawnOffset | Vector | (50, 0, 0) | Yes |
| CoreRef | BP_WardenCore (Object Reference) | None | No |
| TargetRef | Character (Object Reference) | None | No |

### **49) Implement Ability Activation**

#### 49.1) Create Event ActivateAbility
   - 49.1.1) Click Event Graph tab
   - 49.1.2) Right-click in empty graph space
   - 49.1.3) Search: ActivateAbility
   - 49.1.4) Select: Event ActivateAbility

#### 49.2) Get Core Reference
   - 49.2.1) From Event ActivateAbility:
   - 49.2.1.1) Add Get Avatar Actor From Actor Info node
   - 49.2.2) From Get Avatar Return Value:
   - 49.2.2.1) Add Cast To BP_WardenCore node
   - 49.2.3) From successful cast:
   - 49.2.3.1) Set CoreRef variable

#### 49.3) Get Target from Blackboard
   - 49.3.1) From CoreRef:
   - 49.3.1.1) Add Get Controller node
   - 49.3.2) From Get Controller Return Value:
   - 49.3.2.1) Add Cast To AIController node
   - 49.3.3) From AIController:
   - 49.3.3.1) Add Get Blackboard Component node
   - 49.3.4) From Blackboard Component:
   - 49.3.4.1) Add Get Value as Object node
   - 49.3.4.2) Key Name: TargetEnemy
   - 49.3.5) From Return Value:
   - 49.3.5.1) Add Cast To Character node
   - 49.3.6) From successful cast:
   - 49.3.6.1) Set TargetRef variable

#### 49.4) Validate Target
   - 49.4.1) From TargetRef:
   - 49.4.1.1) Add Is Valid node
   - 49.4.2) From Is Valid -> Is Valid pin:
   - 49.4.2.1) Add Branch node
   - 49.4.3) From Branch False:
   - 49.4.3.1) Add End Ability node
   - 49.4.3.2) Cancelled: Check (true)

### **50) Calculate Spawn Location**

#### 50.1) Get Core Location
   - 50.1.1) From Branch True:
   - 50.1.1.1) Add Get Actor Location node
   - 50.1.1.2) Target: CoreRef

#### 50.2) Get Core Forward Vector
   - 50.2.1) Add Get Actor Forward Vector node
   - 50.2.2) Target: CoreRef

#### 50.3) Apply Spawn Offset
   - 50.3.1) Add Multiply (Vector * Float) node
   - 50.3.2) Connect:
   - 50.3.2.1) Vector: Get Actor Forward Vector Return Value
   - 50.3.2.2) Float: LaserSpawnOffset X value (50.0)
   - 50.3.3) Add Add (Vector + Vector) node
   - 50.3.4) Connect:
   - 50.3.4.1) A: Get Actor Location Return Value
   - 50.3.4.2) B: Multiply result

#### 50.4) Calculate Rotation to Target
   - 50.4.1) Add Find Look at Rotation node
   - 50.4.2) Connect:
   - 50.4.2.1) Start: Spawn location (Add result)
   - 50.4.2.2) Target: Get Actor Location of TargetRef

#### 50.5) Make Spawn Transform
   - 50.5.1) Add Make Transform node
   - 50.5.2) Connect:
   - 50.5.2.1) Location: Spawn location result
   - 50.5.2.2) Rotation: Find Look at Rotation result

### **51) Spawn Projectile Using AbilityTask**

#### 51.1) Add Spawn Projectile Task
   - 51.1.1) From Make Transform:
   - 51.1.1.1) Search: AbilityTask_SpawnProjectile
   - 51.1.1.2) Add Spawn Projectile and Wait node
   - 51.1.2) Connect:
   - 51.1.2.1) Projectile Class: ProjectileClass variable
   - 51.1.2.2) Spawn Transform: Make Transform result

#### 51.2) Bind Target Data Event
   - 51.2.1) From Spawn Projectile task:
   - 51.2.1.1) Drag from On Target Data pin
   - 51.2.1.2) Add Custom Event
   - 51.2.1.3) Name: OnProjectileHit

### **52) Handle Projectile Hit**

> **LOCKED CONTRACT 13 (INV-GESPEC-1):** When implementing this in manifest YAML, the MakeOutgoingGameplayEffectSpec node MUST use `param.GameplayEffectClass: GE_CoreLaserDamage` syntax. Properties without the `param.` prefix are ignored by the generator, causing silent runtime failure.

#### 52.1) Apply Damage Effect
   - 52.1.1) From OnProjectileHit:
   - 52.1.1.1) Add Make Outgoing Gameplay Effect Spec node
   - 52.1.1.2) Gameplay Effect Class: GE_CoreLaserDamage
   - 52.1.2) From Spec Handle:
   - 52.1.2.1) Add Assign Tag Set By Caller Magnitude node
   - 52.1.2.2) Data Tag: SetByCaller.Damage
   - 52.1.2.3) Magnitude: CoreRef -> LaserDamage variable

#### 52.2) Get Target ASC
   - 52.2.1) From Target Data Hit Result:
   - 52.2.1.1) Get hit actor
   - 52.2.1.2) Add Get Ability System Component node

#### 52.3) Apply Effect to Target
   - 52.3.1) Add Apply Gameplay Effect Spec to Target node
   - 52.3.2) Connect:
   - 52.3.2.1) Target: Target ASC
   - 52.3.2.2) Spec Handle: From Assign Tag Set By Caller

### **53) Apply Cooldown**

#### 53.1) Apply Cooldown Effect
   - 53.1.1) From Apply Gameplay Effect:
   - 53.1.1.1) Add Apply Gameplay Effect to Owner node
   - 53.1.1.2) Gameplay Effect Class: GE_CoreLaserCooldown

### **54) End Ability**

#### 54.1) Commit and End
   - 54.1.1) From Apply Cooldown:
   - 54.1.1.1) Add Commit Ability node
   - 54.1.2) From Commit Ability:
   - 54.1.2.1) Add End Ability node
   - 54.1.2.2) Cancelled: Uncheck (false)

### **55) Compile and Save Ability**
   - 55.1) Click Compile
   - 55.2) Click Save

### **56) Add Ability to AC_WardenCore**

#### 56.1) Open AC_WardenCore
   - 56.1.1) Double-click AC_WardenCore

#### 56.2) Add GA_CoreLaser
   - 56.2.1) Expand: Default Abilities
   - 56.2.2) Click + to add element
   - 56.2.3) Select: GA_CoreLaser

#### 56.3) Save Asset

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 1.3 | January 2026 | **Naming Convention Alignment (Claude-GPT Audit):** Updated all asset names to match Narrative Pro conventions. NPCDef_* → NPC_* (per Narrative Pro Content folder standard). ActConfig_* → AC_*Behavior (to distinguish from AbilityConfiguration AC_*). Added critical note about SpawnNPC vs raw SpawnActor pattern. Updated UE version to 5.7, Narrative Pro to v2.2. |
| 1.2 | January 2026 | Fixed SetByCaller tag: Changed Data.Damage to SetByCaller.Damage in GE_CoreLaserDamage config (Section 10.4) and GA_CoreLaser ability (Section 52.1) per Narrative Pro standard (NarrativeGameplayTags.cpp). |
| 1.1 | January 2026 | Simplified Has Authority pattern. Removed manual aggro transfer (faction system handles automatically via GoalGenerator_Attack). Verified ActivityConfiguration, GoalGenerator, and Default Activities are complete. |
| 1.0 | January 2026 | Initial implementation guide |

---

**END OF GUIDE**
