# Father Companion - GA_TurretShoot Implementation Guide
## VERSION 3.1 - Manifest Alignment
## Unreal Engine 5.7 + Narrative Pro Plugin v2.2

---

## DOCUMENT INFORMATION

| Field | Value |
|-------|-------|
| Ability Name | GA_TurretShoot |
| Ability Type | AI Combat (Ranged Attack) |
| Parent Class | NarrativeGameplayAbility |
| Projectile Parent | NarrativeProjectile |
| Spawn Task | AbilityTask_SpawnProjectile |
| Form | Engineer (Stationary Turret) |
| Input | None (AI Autonomous) |
| Version | 3.1 |
| Last Updated | January 2026 |

---

## TABLE OF CONTENTS

| Section | Description |
|---------|-------------|
| Introduction | Overview, key features, combat flow |
| Prerequisites | Required assets before implementation |
| Quick Reference | Summary tables |
| Phase 1 | Gameplay Tags Setup |
| Phase 2 | Create Projectile Actor (NarrativeProjectile) |
| Phase 3 | Create Gameplay Effects |
| Phase 4 | Create GA_TurretShoot Ability |
| Phase 5 | Implement Shooting Logic (AbilityTask_SpawnProjectile) |
| Phase 6 | Grant Ability to Father |
| Phase 7 | AI Behavior Tree Integration |
| Changelog | Version history |

---

## INTRODUCTION

### Ability Overview

GA_TurretShoot is an AI-controlled ranged attack ability for the Engineer form. When the father is deployed as a turret, it automatically detects and shoots enemies within its engagement range. The turret fires energy projectiles at a consistent rate, dealing damage to hostile targets.

### Key Features

| Feature | Description |
|---------|-------------|
| AI Autonomous | No player input required |
| Ranged Attack | Engages enemies at 500-1500 unit range |
| NarrativeProjectile System | Uses v2.2 native projectile base class |
| AbilityTask_SpawnProjectile | Native task handles spawn and target data |
| Rotation Tracking | Turret rotates to face targets within arc |
| Fire Rate Control | Consistent shots per second |
| Target Prioritization | Engages closest or highest threat |
| Arc Limitation | Only fires within 270 degree rotation arc |

### Engineer Form Combat Flow

| Step | Condition | Action |
|------|-----------|--------|
| 1 | Enemy detected | Turret detects enemy within 1500 unit range |
| 2 | Distance check | Measure distance from turret to enemy |
| 3a | FAR (500-1500 units) | Activate GA_TurretShoot (this ability) |
| 3b | CLOSE (less than 500 units) | Activate GA_FatherElectricTrap |

### Technical Specifications

| Parameter | Value |
|-----------|-------|
| Damage Per Shot | 15 |
| Fire Rate | 2 shots per second |
| Projectile Speed | 3000 units/second |
| Min Engage Range | 500 units |
| Max Engage Range | 1500 units |
| Rotation Arc | 270 degrees |
| Projectile Type | Energy bolt |

### Engagement Priority

| Priority | Target Type |
|----------|-------------|
| 1 | Marked enemies (GA_FatherMark) |
| 2 | Closest enemy in range |
| 3 | Lowest health enemy |

---

## PREREQUISITES

### Required Before This Guide

| Requirement | Description |
|-------------|-------------|
| BP_FatherCompanion | Father with NarrativeNPCCharacter parent |
| GA_FatherEngineer | Engineer form activation ability |
| Effect.Father.FormState.Engineer tag | Form identification |
| Father.State.Deployed tag | Turret deployment state |
| Engineer AI Controller | AI controller for turret behavior |

### Variables Required in BP_FatherCompanion

| Variable | Type | Purpose |
|----------|------|---------|
| IsDeployed | Boolean | Turret deployment state |
| CurrentTarget | Actor Reference | Current attack target |
| DeployedTransform | Transform | Turret world position and rotation |
| TurretRotationArc | Float | 270 degree rotation limit |

---

## QUICK REFERENCE

### Tag Configuration Summary

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Engineer.TurretShoot |
| Activation Required | Effect.Father.FormState.Engineer, Father.State.Deployed, Father.State.Recruited |
| Activation Owned | Father.State.Shooting |
| Activation Blocked | Cooldown.Father.Engineer.TurretShoot |
| InputTag | None (AI-controlled) |

### Variable Summary

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| ProjectileClass | Class Ref (NarrativeProjectile) | BP_TurretProjectile | Projectile to spawn |
| DamagePerShot | Float | 15.0 | Damage per projectile |
| ProjectileSpeed | Float | 3000.0 | Projectile velocity |
| MuzzleSocketName | Name | MuzzleSocket | Socket for projectile spawn |
| FatherRef | Actor Ref | None | Father reference |
| AttackTarget | Actor Ref | None | Current target (AI-set) |
| ShootMontage | Anim Montage | None | Optional shoot animation |

### Gameplay Effect Summary

| Effect | Duration | Purpose |
|--------|----------|---------|
| GE_TurretProjectileDamage | Instant | 15 damage per hit via NarrativeDamageExecCalc |
| GE_TurretShootCooldown | 0.5 seconds | Fire rate limiter |

### Projectile Parameters

| Parameter | Value |
|-----------|-------|
| Parent Class | NarrativeProjectile |
| Damage | 15 |
| Speed | 3000 units/second |
| Gravity | 0 (no drop) |
| Collision Radius | 15 units |
| Lifetime | 3 seconds |

### Combat Parameters

| Parameter | Value |
|-----------|-------|
| Min Engage Range | 500 units |
| Max Engage Range | 1500 units |
| Fire Rate | 2 shots/second |
| DPS (if all hit) | 50 |

### Behavior Tree Logic

| Branch | Decorators | Task |
|--------|------------|------|
| Shoot Sequence (Far Range) | AttackTarget Is Set, Distance > 500, Distance <= 1500, ShootOnCooldown Is Not Set, Rotation Arc Valid | Activate GA_TurretShoot |
| Trap Sequence (Close Range) | AttackTarget Is Set, Distance < 500, TrapOnCooldown Is Not Set | Activate GA_FatherElectricTrap |

### Engineer Form Abilities Status

| Ability | Type | Status |
|---------|------|--------|
| GA_FatherEngineer | Form | Complete |
| GA_TurretShoot | AI | Complete |
| GA_FatherElectricTrap | AI | v2.6 Complete |
| GA_FatherMark | Passive | Pending |

### Replication Settings

| Property | Value |
|----------|-------|
| Instancing Policy | Instanced Per Actor |
| Replication Policy | Replicate |
| Net Execution Policy | Server Only |

### v2.2 Feature Integration

| Feature | Usage |
|---------|-------|
| NarrativeProjectile | Base class for BP_TurretProjectile |
| AbilityTask_SpawnProjectile | Spawns projectile with target data handling |
| OnProjectileTargetData | Delegate fires when projectile hits |
| SetProjectileTargetData | Called by projectile on collision |
| GetNarrativeCharacter | Returns owning character |

### Related Documents

| Document | Purpose |
|----------|---------|
| Father_Companion_Technical_Reference_v5.3 | Cross-actor patterns, tag architecture |
| GA_FatherEngineer_Implementation_Guide | Engineer form activation |
| GA_FatherElectricTrap_Implementation_Guide | Close-range turret defense |
| DefaultGameplayTags_FatherCompanion_v3_5 | Tag definitions and line numbers |

---

## PHASE 1: GAMEPLAY TAGS SETUP

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Engineer.TurretShoot | Father turret ranged attack ability |
| Father.State.Shooting | Father turret is actively firing |
| Father.State.Targeting | Father turret is tracking a target |
| Effect.Father.TurretDamage | Target hit by turret projectile |
| Data.Damage.Turret | Damage type identifier for turret shots |
| Cooldown.Father.Engineer.TurretShoot | Fire rate cooldown between shots |

### **Verify Existing Tags**

| Tag Name | Purpose |
|----------|---------|
| Effect.Father.FormState.Engineer | Form identification (Activation Required) |
| Father.State.Deployed | Turret deployment state (Activation Required) |
| Father.State.Recruited | Recruitment state (Activation Required) |

---

## PHASE 2: CREATE PROJECTILE ACTOR (NARRATIVEPROJECTILE)

### 1) Create Projectiles Folder

#### 1.1) Navigate to Engineer Folder
   - 1.1.1) In Content Browser, go to /Content/FatherCompanion/Engineer/

#### 1.2) Create Projectiles Subfolder
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select New Folder
   - 1.2.3) Name: Projectiles
   - 1.2.4) Double-click Projectiles folder to open

### 2) Create BP_TurretProjectile Actor

#### 2.1) Create NarrativeProjectile Blueprint
   - 2.1.1) Right-click in Projectiles folder
   - 2.1.2) Select Blueprint Class
   - 2.1.3) In Pick Parent Class dialog:
   - 2.1.3.1) Click All Classes dropdown
   - 2.1.3.2) Search: NarrativeProjectile
   - 2.1.3.3) Select NarrativeProjectile
   - 2.1.3.4) Click Select button
   - 2.1.4) Name: BP_TurretProjectile
   - 2.1.5) Double-click to open

### 3) Add Components

#### 3.1) Add Root Scene Component
   - 3.1.1) In Components panel, click Add
   - 3.1.2) Search: Scene
   - 3.1.3) Select Scene Component
   - 3.1.4) Rename to: Root
   - 3.1.5) Drag onto DefaultSceneRoot to replace it

#### 3.2) Add Static Mesh Component
   - 3.2.1) Click Add in Components panel
   - 3.2.2) Search: Static Mesh
   - 3.2.3) Select Static Mesh Component
   - 3.2.4) Rename to: ProjectileMesh
   - 3.2.5) In Details panel:
   - 3.2.5.1) Find Static Mesh property
   - 3.2.5.2) Select a sphere or elongated mesh (placeholder)
   - 3.2.5.3) Set Scale: (0.1, 0.1, 0.3) for bullet shape

#### 3.3) Add Particle System Component
   - 3.3.1) Click Add in Components panel
   - 3.3.2) Search: Niagara
   - 3.3.3) Select Niagara Particle System Component
   - 3.3.4) Rename to: TrailEffect
   - 3.3.5) In Details panel:
   - 3.3.5.1) Find Niagara System Asset property
   - 3.3.5.2) Select energy/laser trail effect (placeholder)

#### 3.4) Add Sphere Collision Component
   - 3.4.1) Click Add in Components panel
   - 3.4.2) Search: Sphere Collision
   - 3.4.3) Select Sphere Collision
   - 3.4.4) Rename to: DamageCollision
   - 3.4.5) In Details panel:
   - 3.4.5.1) Set Sphere Radius: 15.0
   - 3.4.5.2) Find Collision Presets dropdown
   - 3.4.5.3) Select: OverlapAllDynamic
   - 3.4.5.4) Enable Generate Overlap Events: Checked

#### 3.5) Add Projectile Movement Component
   - 3.5.1) Click Add in Components panel
   - 3.5.2) Search: Projectile Movement
   - 3.5.3) Select Projectile Movement Component
   - 3.5.4) In Details panel:
   - 3.5.4.1) Initial Speed: 3000.0
   - 3.5.4.2) Max Speed: 3000.0
   - 3.5.4.3) Projectile Gravity Scale: 0.0
   - 3.5.4.4) Rotation Follows Velocity: Checked

### 4) Create Projectile Variables

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| HasHitTarget | Boolean | false | No |
| DamageAmount | Float | 25.0 | Yes |

### 5) Configure Replication

#### 5.1) Open Class Defaults
   - 5.1.1) Click Class Defaults button in toolbar

#### 5.2) Set Replication Properties
   - 5.2.1) Find Replication section
   - 5.2.2) Replicates: Checked
   - 5.2.3) Replicate Movement: Checked
   - 5.2.4) Net Load on Client: Checked

### 6) Configure Lifespan

#### 6.1) Set Initial Life Span
   - 6.1.1) In Class Defaults, find Actor section
   - 6.1.2) Initial Life Span: 3.0

### 7) Create Overlap Event Logic with SetProjectileTargetData

#### 7.1) Add Overlap Event
   - 7.1.1) Select DamageCollision component
   - 7.1.2) In Details panel, scroll to Events section
   - 7.1.3) Click + next to On Component Begin Overlap

#### 7.2) Implement Authority Check
   - 7.2.1) From On Component Begin Overlap event:
   - 7.2.1.1) Add Has Authority node
   - 7.2.2) From Has Authority:
   - 7.2.2.1) Add Branch node
   - 7.2.2.2) Connect return to Condition

#### 7.3) Prevent Multiple Hits
   - 7.3.1) From Branch True (Has Authority):
   - 7.3.1.1) From HasHitTarget variable getter
   - 7.3.1.2) Add NOT Boolean node
   - 7.3.1.3) Add Branch node
   - 7.3.1.4) Connect NOT output to Condition

#### 7.4) Validate Not Self-Hit
   - 7.4.1) From Branch True (not already hit):
   - 7.4.1.1) Add Get Narrative Character node
   - 7.4.1.2) Target: Self
   - 7.4.2) From Get Narrative Character:
   - 7.4.2.1) Add Not Equal node
   - 7.4.2.2) Connect Other Actor to first input
   - 7.4.2.3) Connect Get Narrative Character to second input
   - 7.4.3) From Not Equal:
   - 7.4.3.1) Add Branch node
   - 7.4.3.2) Connect Boolean result to Condition

#### 7.5) Set Hit Flag and Create Target Data
   - 7.5.1) From Branch True (not self):
   - 7.5.1.1) Add Set HasHitTarget node
   - 7.5.1.2) Value: true (checked)
   - 7.5.2) From Set HasHitTarget:
   - 7.5.2.1) Add Cast To NarrativeCharacter node
   - 7.5.2.2) Connect Other Actor to Object input
   - 7.5.3) From Cast success:
   - 7.5.3.1) Add Make Target Data Handle from Actor node
   - 7.5.3.2) Target Actor: Connect As Narrative Character

#### 7.6) Broadcast Target Data via NarrativeProjectile System
   - 7.6.1) From Make Target Data Handle:
   - 7.6.1.1) Add Set Projectile Target Data node
   - 7.6.1.2) Target: Self
   - 7.6.1.3) Target Handle: Connect from Make Target Data Handle

#### 7.7) Destroy Projectile
   - 7.7.1) From Set Projectile Target Data execution:
   - 7.7.1.1) Add Destroy Actor node
   - 7.7.1.2) Target: Self

#### 7.8) Client-Side Visual Destruction
   - 7.8.1) From Branch False (not authority - client):
   - 7.8.1.1) Add Destroy Actor node
   - 7.8.1.2) Target: Self

### 8) Compile and Save

#### 8.1) Save Projectile Blueprint
   - 8.1.1) Click Compile button
   - 8.1.2) Click Save button

---

## PHASE 3: CREATE GAMEPLAY EFFECTS

### 1) Create Effects Folder

#### 1.1) Navigate to Engineer Folder
   - 1.1.1) In Content Browser, go to /Content/FatherCompanion/Engineer/

#### 1.2) Create Effects Subfolder
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select New Folder
   - 1.2.3) Name: Effects
   - 1.2.4) Double-click Effects folder to open

### 2) Create GE_TurretProjectileDamage

#### 2.1) Create GameplayEffect Blueprint
   - 2.1.1) Right-click in Effects folder
   - 2.1.2) Select Blueprint Class
   - 2.1.3) Search parent: GameplayEffect
   - 2.1.4) Select GameplayEffect
   - 2.1.5) Name: GE_TurretProjectileDamage
   - 2.1.6) Double-click to open

#### 2.2) Configure GE_TurretProjectileDamage Properties

| Property | Value |
|----------|-------|
| Duration Policy | Instant |
| Components | Executions |
| Execution Definitions [0] -> Calculation Class | NarrativeDamageExecCalc |
| Asset Tags -> Add to Inherited [0] | Effect.Father.TurretDamage |

#### 2.3) Compile and Save

### 3) Create GE_TurretShootCooldown

#### 3.1) Create GameplayEffect Blueprint
   - 3.1.1) Right-click in Effects folder
   - 3.1.2) Select Blueprint Class
   - 3.1.3) Search parent: GameplayEffect
   - 3.1.4) Select GameplayEffect
   - 3.1.5) Name: GE_TurretShootCooldown
   - 3.1.6) Double-click to open

#### 3.2) Configure GE_TurretShootCooldown Properties

| Property | Value |
|----------|-------|
| Duration Policy | Has Duration |
| Duration Magnitude -> Scalable Float Magnitude | 0.5 |
| Components | Granted Tags |
| Granted Tags -> Add to Inherited [0] | Cooldown.Father.Engineer.TurretShoot |

#### 3.3) Compile and Save

---

## PHASE 4: CREATE GA_TURRETSHOOT ABILITY

### 1) Create Ability Blueprint

#### 1.1) Navigate to Abilities Folder
   - 1.1.1) In Content Browser, go to /Content/FatherCompanion/Engineer/Abilities/

#### 1.2) Create NarrativeGameplayAbility Blueprint
   - 1.2.1) Right-click in folder
   - 1.2.2) Select Blueprint Class
   - 1.2.3) Search parent: NarrativeGameplayAbility
   - 1.2.4) Select NarrativeGameplayAbility
   - 1.2.5) Name: GA_TurretShoot
   - 1.2.6) Double-click to open

### 2) Configure Ability Tags

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Engineer.TurretShoot |
| Activation Required Tags | Effect.Father.FormState.Engineer, Father.State.Deployed, Father.State.Recruited |
| Activation Blocked Tags | Cooldown.Father.Engineer.TurretShoot |
| Activation Owned Tags | Father.State.Shooting |

### 4) Configure Replication

#### 4.1) Set Replication Properties
   - 4.1.1) Find Replication section
   - 4.1.2) Replication Policy: Replicate
   - 4.1.3) Net Execution Policy: Server Only
   - 4.1.4) Instancing Policy: Instanced Per Actor

### 5) Create Ability Variables

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| ProjectileClass | Class Reference (NarrativeProjectile) | BP_TurretProjectile | Yes |
| DamagePerShot | Float | 15.0 | Yes |
| ProjectileSpeed | Float | 3000.0 | Yes |
| MuzzleSocketName | Name | MuzzleSocket | Yes |
| FatherRef | BP_FatherCompanion (Object Reference) | None | No |
| AttackTarget | Actor (Object Reference) | None | No |
| ShootMontage | Anim Montage (Object Reference) | None | Yes |

---

## PHASE 5: IMPLEMENT SHOOTING LOGIC (ABILITYTASK_SPAWNPROJECTILE)

### 1) Add Event ActivateAbility

#### 1.1) Create Event Node
   - 1.1.1) Right-click in Event Graph
   - 1.1.2) Search: Event ActivateAbility
   - 1.1.3) Select Event ActivateAbility

### 2) Get Father Reference

#### 2.1) Get Avatar Actor
   - 2.1.1) From Event ActivateAbility:
   - 2.1.1.1) Add Get Avatar Actor From Actor Info node
   - 2.1.2) From Return Value:
   - 2.1.2.1) Add Cast To BP_FatherCompanion node
   - 2.1.2.2) Connect Return Value to Object input
   - 2.1.3) From As BP Father Companion:
   - 2.1.3.1) Add Set FatherRef node
   - 2.1.3.2) Connect As BP Father Companion to input

### 3) Get Target from Father

#### 3.1) Get Attack Target
   - 3.1.1) From Set FatherRef execution:
   - 3.1.1.1) From FatherRef getter:
   - 3.1.1.2) Add Get AttackTarget node (father variable)
   - 3.1.2) From Get AttackTarget Return Value:
   - 3.1.2.1) Add Set AttackTarget node (local variable)
   - 3.1.2.2) Connect to AttackTarget input

#### 3.2) Validate Target
   - 3.2.1) From Set AttackTarget execution:
   - 3.2.1.1) From AttackTarget getter:
   - 3.2.1.2) Add Is Valid node
   - 3.2.2) From Is Valid:
   - 3.2.2.1) Add Branch node
   - 3.2.2.2) Connect Is Valid return to Condition

### 4) Play Shoot Animation (Optional)

#### 4.1) Check Montage Valid
   - 4.1.1) From Branch True execution:
   - 4.1.1.1) Add ShootMontage getter
   - 4.1.1.2) Add Is Valid node
   - 4.1.1.3) Add Branch node

#### 4.2) Play Montage Path (If Montage Exists)
   - 4.2.1) From Branch True (montage valid):
   - 4.2.1.1) Add Play Montage and Wait for Event node
   - 4.2.1.2) Montage to Play: Connect ShootMontage getter
   - 4.2.1.3) Event Tags: Leave empty
   - 4.2.2) From On Completed or On Notify Begin:
   - 4.2.2.1) Continue to muzzle position calculation

#### 4.3) No Montage Path (Skip Animation)
   - 4.3.1) From Branch False (no montage):
   - 4.3.1.1) Continue directly to muzzle position calculation

### 5) Get Muzzle Position from Socket

#### 5.1) Get Mesh Component
   - 5.1.1) From FatherRef getter:
   - 5.1.1.1) Add Get Mesh node

#### 5.2) Get Socket Transform
   - 5.2.1) From Get Mesh Return Value:
   - 5.2.1.1) Add Get Socket Transform node
   - 5.2.1.2) In Socket Name: Connect MuzzleSocketName getter
   - 5.2.1.3) Transform Space: World

#### 5.3) Break Transform
   - 5.3.1) From Get Socket Transform Return Value:
   - 5.3.1.1) Add Break Transform node
   - 5.3.2) Location output is Muzzle Position

### 6) Calculate Direction to Target

#### 6.1) Get Target Location
   - 6.1.1) From AttackTarget getter:
   - 6.1.1.1) Add Get Actor Location node

#### 6.2) Calculate Direction
   - 6.2.1) Add Subtract (Vector) node:
   - 6.2.1.1) Connect Target Location to A input
   - 6.2.1.2) Connect Muzzle Position (from Break Transform) to B input
   - 6.2.2) From Subtract Return Value:
   - 6.2.2.1) Add Normalize node
   - 6.2.3) From Normalize Return Value:
   - 6.2.3.1) Add Make Rot from X node
   - 6.2.3.2) Connect normalized vector to X input

### 7) Spawn Projectile Using AbilityTask

#### 7.1) Create Spawn Transform
   - 7.1.1) Add Make Transform node
   - 7.1.2) Connect:
   - 7.1.2.1) Location: Muzzle Position (from Break Transform Location)
   - 7.1.2.2) Rotation: Make Rot from X result
   - 7.1.2.3) Scale: (1.0, 1.0, 1.0) default

#### 7.2) Spawn Projectile with AbilityTask
   - 7.2.1) Right-click in graph
   - 7.2.2) Search: Spawn Projectile
   - 7.2.3) Select Spawn Projectile (AbilityTask_SpawnProjectile)
   - 7.2.4) Configure node:
   - 7.2.4.1) Task Instance Name: TurretProjectileTask
   - 7.2.4.2) Class: Connect ProjectileClass variable
   - 7.2.4.3) Projectile Spawn Transform: Connect Make Transform return

### 8) Handle Target Data from Projectile Hit

#### 8.1) Bind to OnTargetData Delegate
   - 8.1.1) From Spawn Projectile On Target Data execution pin:
   - 8.1.1.1) This fires when projectile calls SetProjectileTargetData

#### 8.2) Get Hit Actor from Target Data
   - 8.2.1) From On Target Data:
   - 8.2.1.1) Drag from Data pin
   - 8.2.1.2) Add Get Actors from Target Data node
   - 8.2.2) From Get Actors Return Value:
   - 8.2.2.1) Add Get (Copy) node with index 0
   - 8.2.3) From Get (Copy) Return Value:
   - 8.2.3.1) Add Cast To NarrativeCharacter node

#### 8.3) Apply Damage Effect to Hit Target
   - 8.3.1) From Cast successful execution:
   - 8.3.1.1) Add Get Ability System Component node
   - 8.3.1.2) Target: Connect As Narrative Character
   - 8.3.2) From Get ASC Return Value:
   - 8.3.2.1) Add Is Valid node
   - 8.3.3) From Is Valid Is Valid pin:
   - 8.3.3.1) Add Make Outgoing Spec node
   - 8.3.3.2) Target: Get Ability System Component from FatherRef
   - 8.3.3.3) Gameplay Effect Class: GE_TurretProjectileDamage
   - 8.3.3.4) Level: 1.0
   - 8.3.4) From Make Outgoing Spec:
   - 8.3.4.1) Add Assign Tag Set By Caller Magnitude node
   - 8.3.4.2) Data Tag: Data.Damage.Turret
   - 8.3.4.3) Magnitude: Connect DamagePerShot getter
   - 8.3.5) From Assign Tag Set By Caller:
   - 8.3.5.1) Add Apply Gameplay Effect Spec to Target node
   - 8.3.5.2) Spec Handle: Connect from Assign node output
   - 8.3.5.3) Target: Connect target ASC

### 9) Handle Projectile Destroyed (No Hit)

#### 9.1) Bind to OnDestroyed Delegate
   - 9.1.1) From Spawn Projectile On Destroyed execution pin:
   - 9.1.1.1) This fires when projectile is destroyed without hitting target

#### 9.2) Connect to Cooldown
   - 9.2.1) From On Destroyed:
   - 9.2.1.1) Connect to Apply Cooldown section (shared)

### 10) Apply Cooldown and End

#### 10.1) Apply Shoot Cooldown
   - 10.1.1) After Apply GE Spec execution (and from On Destroyed path):
   - 10.1.1.1) Add Apply Gameplay Effect to Owner node
   - 10.1.2) Configure:
   - 10.1.2.1) Gameplay Effect Class: GE_TurretShootCooldown
   - 10.1.2.2) Level: 1.0

#### 10.2) Commit Ability
   - 10.2.1) From Apply GE execution:
   - 10.2.1.1) Add Commit Ability node

#### 10.3) End Ability
   - 10.3.1) From Commit Ability execution:
   - 10.3.1.1) Add End Ability node
   - 10.3.2) Was Cancelled: Unchecked (false)

### 11) Handle Invalid Target

#### 11.1) From Branch False
   - 11.1.1) From Branch False execution:
   - 11.1.1.1) Add End Ability node
   - 11.1.2) Was Cancelled: Checked (true)

### 12) Compile and Save

#### 12.1) Save Ability
   - 12.1.1) Click Compile button
   - 12.1.2) Click Save button

---

## PHASE 6: GRANT ABILITY TO FATHER

### 1) Open BP_FatherCompanion

#### 1.1) Navigate to Father Blueprint
   - 1.1.1) In Content Browser, go to /Content/FatherCompanion/Characters/
   - 1.1.2) Double-click BP_FatherCompanion to open

### 2) Add to Default Abilities Array

#### 2.1) Open Class Defaults
   - 2.1.1) Click Class Defaults button

#### 2.2) Find Default Abilities Property
   - 2.2.1) In Details panel, search: Default Abilities
   - 2.2.2) Click arrow to expand array

#### 2.3) Add New Ability Entry
   - 2.3.1) Click + button
   - 2.3.2) Expand new element

#### 2.4) Configure Ability Entry
   - 2.4.1) Ability Class: Select GA_TurretShoot
   - 2.4.2) Level: 1
   - 2.4.3) Input ID: -1 (AI-controlled, no input)

### 3) Add AttackTarget Variable

| Variable | Type | Default | Instance Editable | Replication |
|----------|------|---------|-------------------|-------------|
| AttackTarget | Actor (Object Reference) | None | No | Replicated |

### 4) Compile and Save

#### 4.1) Compile Blueprint
   - 4.1.1) Click Compile button

#### 4.2) Save Blueprint
   - 4.2.1) Click Save button

---

## PHASE 7: AI BEHAVIOR TREE INTEGRATION

### 1) Create Engineer Blackboard

#### 1.1) Navigate to AI Folder
   - 1.1.1) In Content Browser, go to /Content/FatherCompanion/AI/

#### 1.2) Create Blackboard Asset
   - 1.2.1) Right-click in folder
   - 1.2.2) Select Artificial Intelligence then Blackboard
   - 1.2.3) Name: BB_FatherEngineer
   - 1.2.4) Double-click to open

### 2) Configure Blackboard Keys

#### 2.1) Add AttackTarget Key
   - 2.1.1) Click New Key button
   - 2.1.2) Select Object
   - 2.1.3) Name: AttackTarget
   - 2.1.4) Base Class: Actor

#### 2.2) Add DistanceToTarget Key
   - 2.2.1) Click New Key button
   - 2.2.2) Select Float
   - 2.2.3) Name: DistanceToTarget

#### 2.3) Add ShootOnCooldown Key
   - 2.3.1) Click New Key button
   - 2.3.2) Select Bool
   - 2.3.3) Name: ShootOnCooldown

#### 2.4) Add TrapOnCooldown Key
   - 2.4.1) Click New Key button
   - 2.4.2) Select Bool
   - 2.4.3) Name: TrapOnCooldown

### 3) Create Engineer Behavior Tree

#### 3.1) Create BT Asset
   - 3.1.1) Right-click in AI folder
   - 3.1.2) Select Artificial Intelligence then Behavior Tree
   - 3.1.3) Name: BT_FatherEngineer
   - 3.1.4) Double-click to open

#### 3.2) Set Blackboard
   - 3.2.1) In Details panel, find Blackboard Asset
   - 3.2.2) Select: BB_FatherEngineer

### 4) Create BTDecorator_CheckRotationArc

#### 4.1) Create Decorator Blueprint
   - 4.1.1) Right-click in AI folder
   - 4.1.2) Select Blueprint Class
   - 4.1.3) Search parent: BTDecorator_BlueprintBase
   - 4.1.4) Select BTDecorator_BlueprintBase
   - 4.1.5) Name: BTDecorator_CheckRotationArc
   - 4.1.6) Double-click to open

#### 4.2) Create Decorator Variables

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| MaxArcAngle | Float | 135.0 | Yes |
| TargetKey | Blackboard Key Selector | None | Yes |

#### 4.3) Implement Check Condition
   - 4.3.1) Add Event Perform Condition Check AI
   - 4.3.2) Get controlled pawn and cast to BP_FatherCompanion
   - 4.3.3) Get deployed transform rotation (forward vector)
   - 4.3.4) Get target location from blackboard
   - 4.3.5) Calculate angle between deployed forward and target direction
   - 4.3.6) Return true if angle less than MaxArcAngle

#### 4.4) Compile and Save
   - 4.4.1) Click Compile button
   - 4.4.2) Click Save button

### 5) Build Behavior Tree Structure

#### 5.1) Add Root Selector
   - 5.1.1) Right-click on ROOT node
   - 5.1.2) Select Add Composite then Selector

#### 5.2) Add Shoot Sequence (Left Branch - Higher Priority)
   - 5.2.1) Right-click on Selector
   - 5.2.2) Select Add Composite then Sequence

#### 5.3) Add Shoot Decorators
   - 5.3.1) Right-click on Shoot Sequence
   - 5.3.2) Select Add Decorator then Blackboard
   - 5.3.3) Configure:
   - 5.3.3.1) Blackboard Key: AttackTarget
   - 5.3.3.2) Key Query: Is Set
   - 5.3.3.3) Observer Aborts: Both
   - 5.3.4) Right-click on Shoot Sequence again
   - 5.3.5) Select Add Decorator then Blackboard
   - 5.3.6) Configure (Minimum Range):
   - 5.3.6.1) Blackboard Key: DistanceToTarget
   - 5.3.6.2) Key Query: Is Greater Than
   - 5.3.6.3) Float Value: 500.0
   - 5.3.7) Right-click on Shoot Sequence again
   - 5.3.8) Select Add Decorator then Blackboard
   - 5.3.9) Configure (Maximum Range):
   - 5.3.9.1) Blackboard Key: DistanceToTarget
   - 5.3.9.2) Key Query: Is Less Than Or Equal To
   - 5.3.9.3) Float Value: 1500.0
   - 5.3.10) Right-click on Shoot Sequence again
   - 5.3.11) Select Add Decorator then Blackboard
   - 5.3.12) Configure (Cooldown Check):
   - 5.3.12.1) Blackboard Key: ShootOnCooldown
   - 5.3.12.2) Key Query: Is Not Set
   - 5.3.13) Right-click on Shoot Sequence again
   - 5.3.14) Select Add Decorator then BTDecorator_CheckRotationArc
   - 5.3.15) Configure (Rotation Arc):
   - 5.3.15.1) Max Arc Angle: 135.0
   - 5.3.15.2) Target Key: Select AttackTarget

#### 5.4) Add Shoot Task
   - 5.4.1) Right-click on Shoot Sequence
   - 5.4.2) Select Add Task then BTTask_ActivateAbilityByClass
   - 5.4.3) Select the task node in Behavior Tree
   - 5.4.4) In Details panel, set Ability to Activate: GA_TurretShoot

#### 5.5) Add Trap Sequence (Right Branch - Lower Priority)
   - 5.5.1) Right-click on Selector
   - 5.5.2) Select Add Composite then Sequence

#### 5.6) Add Trap Decorators
   - 5.6.1) Right-click on Trap Sequence
   - 5.6.2) Add Decorator then Blackboard
   - 5.6.2.1) Blackboard Key: AttackTarget
   - 5.6.2.2) Key Query: Is Set
   - 5.6.3) Add Decorator then Blackboard
   - 5.6.3.1) Blackboard Key: DistanceToTarget
   - 5.6.3.2) Key Query: Is Less Than
   - 5.6.3.3) Float Value: 500.0
   - 5.6.4) Add Decorator then Blackboard
   - 5.6.4.1) Blackboard Key: TrapOnCooldown
   - 5.6.4.2) Key Query: Is Not Set

#### 5.7) Add Trap Task
   - 5.7.1) Right-click on Trap Sequence
   - 5.7.2) Select Add Task then BTTask_ActivateAbilityByClass
   - 5.7.3) Select the task node in Behavior Tree
   - 5.7.4) In Details panel, set Ability to Activate: GA_FatherElectricTrap

### 6) BTTask_ActivateAbilityByClass Reference

Narrative Pro provides BTTask_ActivateAbilityByClass - a built-in reusable task for AI ability activation. No custom task creation needed.

#### 6.1) Task Location
   - 6.1.1) BTTask_ActivateAbilityByClass is located in: Plugins/Narrative Pro/Pro/Core/AI/Tasks/

#### 6.2) Task Configuration

| Property | Value | Purpose |
|----------|-------|---------|
| Ability to Activate | (Select ability class) | Dropdown to select ability |
| Custom Description | (Optional) | Display name in BT |

### 7) Save Behavior Tree

#### 7.1) Save All AI Assets
   - 7.1.1) Save BB_FatherEngineer
   - 7.1.2) Save BT_FatherEngineer
   - 7.1.3) Save BTDecorator_CheckRotationArc

---

## CHANGELOG

### VERSION 3.1 - Manifest Alignment

**Release Date:** January 2026

| Change | Description |
|--------|-------------|
| BTTask Update | Replaced custom BTTask_ActivateFatherAbility with Narrative Pro built-in BTTask_ActivateAbilityByClass |
| Section 5.4 | Updated Shoot Task to use BTTask_ActivateAbilityByClass with Ability to Activate dropdown |
| Section 5.7 | Updated Trap Task to use BTTask_ActivateAbilityByClass |
| Section 6 | Replaced custom task creation with BTTask_ActivateAbilityByClass reference |
| Section 7.1 | Removed BTTask_ActivateFatherAbility from Save All AI Assets |

---

### VERSION 2.8 - Documentation Simplification

**Release Date:** January 2026

| Change | Description |
|--------|-------------|
| AttackTarget Variable | PHASE 6 Section 3 - converted to table format |
| Decorator Variables | PHASE 7 Section 4.2 - converted to table format |
| Task Variables | PHASE 7 Section 6.2 - converted to table format |

---

### VERSION 2.7 - Simplified Documentation

**Release Date:** January 2026

| Change Type | Description |
|-------------|-------------|
| Tag Configuration | PHASE 4 Sections 2-3 - converted to single Property/Tags table |
| Variable Creation | PHASE 2 Section 4, PHASE 4 Section 5 - converted to Variable/Type/Default tables |
| GameplayEffect Config | PHASE 3 Sections 2-3 - converted to Property/Value tables |
| Line Reduction | Reduced documentation size while preserving all information |

---

### VERSION 2.6 - Simplified Tag Creation

**Release Date:** January 2026

| Change Type | Description |
|-------------|-------------|
| PHASE 1 Simplified | Replaced 7 detailed subsections with consolidated tag tables |
| Tag Creation Format | Tags now listed in Create Required Tags and Verify Existing Tags tables |
| Line Reduction | Reduced PHASE 1 from 63 lines to 21 lines |

---

### VERSION 2.5 - Narrative Pro v2.2 Integration

**Release Date:** December 2025

| Change Type | Description |
|-------------|-------------|
| Projectile Parent | Changed BP_TurretProjectile parent from Actor to NarrativeProjectile |
| Spawn System | Replaced SpawnActorFromClass with AbilityTask_SpawnProjectile |
| Target Data | Updated hit detection to use SetProjectileTargetData |
| Damage Application | Moved from projectile to ability via OnTargetData delegate |
| Variable Removal | Removed OwnerFather from projectile (NarrativeProjectile handles via GetNarrativeCharacter) |
| v2.2 Features | Added v2.2 Feature Integration table to Quick Reference |

---

### VERSION 2.4 - Variable Name Correction

**Release Date:** December 2025

| Change Type | Description |
|-------------|-------------|
| Variable Rename | Changed DeployedLocation to DeployedTransform in Variables Required table |
| Variable Type | Updated from Vector to Transform to include rotation |
| Consistency | Aligned with GA_FatherEngineer v3.5 and Setup Guide v1.6 |

---

### VERSION 2.3 - Technical Reference v4.5 Alignment

**Release Date:** December 2025

| Change Type | Description |
|-------------|-------------|
| Recruitment Gate | Added Father.State.Recruited to Activation Required Tags |
| Tag Configuration | Ability requires: Engineer form, Deployed state, Recruited state |
| Quick Reference | Added Related Documents table |
| Section 3.3 | Updated to include third required tag |

---

### VERSION 2.2 - v4.0 Architecture Update

**Release Date:** December 2025

| Change Type | Description |
|-------------|-------------|
| Abilities Table | Removed GA_TurretRecall from Engineer Form Abilities Status |
| Documentation | GA_TurretRecall ability removed from v4.0 architecture |

---

### VERSION 2.1 - Hierarchical Tags and Net Execution Policy

**Release Date:** December 2025

**Technical Corrections:**
- Changed Ability.Father.TurretShoot to Ability.Father.Engineer.TurretShoot
- Changed Cooldown.Father.TurretShoot to Cooldown.Father.Engineer.TurretShoot
- Changed Net Execution Policy from Local Predicted to Server Only
- Reason: AI-autonomous ability owned by NPC requires server authority

### VERSION 2.0 - Format, System, and Narrative Pro Alignment

**Release Date:** 2025

**Format Corrections:**
- Removed all UTF-8 special characters (arrows, box-drawing, degree symbols)
- Converted code block diagrams to markdown tables
- Moved Quick Reference section to after Introduction
- Removed conditional "if exists" logic throughout
- Removed obvious UI feedback statements
- Standardized document structure order

**System Corrections:**
- Added NarrativeDamageExecCalc to GE_TurretProjectileDamage
- Added UE 5.6 component-based navigation for Gameplay Effects
- Added proper instigator tracking to projectile damage
- Added replication settings to BP_TurretProjectile
- Updated Engineer Form Abilities Status table

**Narrative Pro Alignment:**
- Renamed TargetEnemy to AttackTarget (matches BBKey_AttackTarget convention)
- Added socket-based muzzle position (MuzzleSocketName) instead of offset calculation
- Added optional animation montage support (ShootMontage variable)
- Added Owner parameter to Spawn Actor for proper instigator chain

**Behavior Tree Improvements:**
- Added BTDecorator_CheckRotationArc for 270-degree arc validation
- Added maximum range decorator (DistanceToTarget <= 1500)
- Updated all blackboard key references to use AttackTarget

**Assets Created:**
- BP_TurretProjectile (NarrativeProjectile with replication)
- GE_TurretProjectileDamage (25 instant damage via NarrativeDamageExecCalc)
- GE_TurretShootCooldown (0.5 second fire rate)
- GA_TurretShoot (ability blueprint)
- BB_FatherEngineer (blackboard with AttackTarget key)
- BT_FatherEngineer (behavior tree with arc/range validation)
- BTDecorator_CheckRotationArc (custom rotation arc decorator)
- BTTask_ActivateFatherAbility (custom BT task)

**Tag Configuration:**
- Ability Tags: Ability.Father.Engineer.TurretShoot
- Activation Required: Effect.Father.FormState.Engineer, Father.State.Deployed
- Activation Owned: Father.State.Shooting
- Activation Blocked: Cooldown.Father.Engineer.TurretShoot
- InputTag: None (AI-controlled)

---

### VERSION 1.0 - Initial Implementation

**Release Date:** 2025

- Initial implementation guide created

---

**END OF GA_TURRETSHOOT IMPLEMENTATION GUIDE v2.5**

**Engineer Form - AI Ranged Attack**

**Unreal Engine 5.7 + Narrative Pro v2.2**

**Blueprint-Only Implementation**
