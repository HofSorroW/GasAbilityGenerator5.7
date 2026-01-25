# Biomechanical Detachment System Implementation Guide
## Enemy Phase Transition on Death
## Version 1.2

---

## DOCUMENT INFORMATION

| Field | Value |
|-------|-------|
| Document Type | Implementation Guide |
| System | Biomechanical Detachment System |
| Last Updated | January 2026 |
| Unreal Engine | 5.7 |
| Narrative Pro | v2.2 |

---

## TABLE OF CONTENTS

| Phase | Name |
|-------|------|
| PHASE 1 | VERIFY GAMEPLAY TAGS |
| PHASE 2 | CREATE NPCDEFINITION ASSETS |
| PHASE 3 | CREATE BP_BIOMECHCREATURE |
| PHASE 4 | CREATE BP_BIOMECHHOST |
| PHASE 5 | IMPLEMENT DEATH DETACHMENT LOGIC |
| PHASE 6 | CREATE ABILITY CONFIGURATIONS |
| PHASE 7 | CREATE ACTIVITY CONFIGURATIONS |
| PHASE 8 | CONFIGURE NPCDEFINITION PROPERTIES |
| PHASE 9 | CONFIGURE FACTION ATTITUDES |
| PHASE 10 | WORLD PLACEMENT |

---

## INTRODUCTION

### System Overview

| Aspect | Description |
|--------|-------------|
| Concept | Combined human+biomechanical enemy transforms on death |
| Phase 1 | Human host with attached biomechanical enhancements (single mesh) |
| Phase 2 | When health depletes, machine detaches and fights independently |
| Visual | Human corpse remains on ground, creature continues combat |
| Integration | Uses Narrative Pro death handling, NPC spawning, AI systems |

### System Flow

| Step | Action | Result |
|------|--------|--------|
| 1 | Player damages BP_BiomechHost | Health decreases |
| 2 | Health reaches zero | OnDied delegate fires |
| 3 | HandleDeath override executes | Detachment sequence begins |
| 4 | Play detachment montage | Visual feedback |
| 5 | Spawn BP_BiomechCreature | New enemy actor created |
| 6 | Spawn corpse mesh (optional) | Visual continuity |
| 7 | Original host destroyed | Phase transition complete |
| 8 | Creature AI begins combat | GoalGenerator_Attack takes over |

### Existing Narrative Pro Assets Used

| Asset | Type | Purpose |
|-------|------|---------|
| BPA_Patrol | NPCActivity | Host patrol behavior |
| BPA_Attack_Melee | NPCActivity | Combat behavior (both) |
| BPA_Idle | NPCActivity | Default fallback |
| BT_Patrol | BehaviorTree | Patrol logic |
| BT_Attack | BehaviorTree | Attack logic |
| GoalGenerator_Attack | NPCGoalGenerator | Creates combat goals from perception |
| Goal_Attack | NPCGoalItem | Attack goal |
| NarrativeCharacterSubsystem | Subsystem | Spawns creature on death |

---

## QUICK REFERENCE

### Gameplay Tags

| Tag | Purpose |
|-----|---------|
| State.Biomech.Host | Applied to combined host entity |
| State.Biomech.Creature | Applied to detached creature |
| State.Biomech.Detaching | During transition phase |
| Faction.Enemy.Biomech | Faction identifier |

### Assets Created

| Asset | Type | Location |
|-------|------|----------|
| BP_BiomechHost | Blueprint Class | /Game/Enemies/Biomech/ |
| BP_BiomechCreature | Blueprint Class | /Game/Enemies/Biomech/ |
| NPC_BiomechHost | NPCDefinition | /Game/Enemies/Biomech/Definitions/ |
| NPC_BiomechCreature | NPCDefinition | /Game/Enemies/Biomech/Definitions/ |
| AC_BiomechHost | AbilityConfiguration | /Game/Enemies/Biomech/Configurations/ |
| AC_BiomechCreature | AbilityConfiguration | /Game/Enemies/Biomech/Configurations/ |
| AC_BiomechHostBehavior | ActivityConfiguration | /Game/Enemies/Biomech/Configurations/ |
| AC_BiomechCreatureBehavior | ActivityConfiguration | /Game/Enemies/Biomech/Configurations/ |

> **NAMING CONVENTION:** Narrative Pro uses `NPC_` prefix for NPCDefinition assets (not `NPCDef_`). ActivityConfiguration assets use `AC_*Behavior` suffix to distinguish from AbilityConfiguration which uses plain `AC_` prefix.

### Variable Summary (BP_BiomechHost)

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| CreatureDefinition | NPCDefinition (Object Reference) | NPC_BiomechCreature | Yes |
| CorpseMesh | Static Mesh (Object Reference) | None | Yes |
| SpawnOffset | Vector | (0, 0, 50) | Yes |
| bSpawnCorpse | Boolean | True | Yes |
| DetachmentMontage | Anim Montage (Object Reference) | None | Yes |

---

## **PHASE 1: VERIFY GAMEPLAY TAGS**

### **1) Create Required Tags**

| Tag | Purpose |
|-----|---------|
| State.Biomech.Host | Applied to host entity |
| State.Biomech.Creature | Applied to creature entity |
| State.Biomech.Detaching | During transition |
| Faction.Enemy.Biomech | Faction identifier |

---

## **PHASE 2: CREATE NPCDEFINITION ASSETS**

### **1) Create Folder Structure**

#### 1.1) Navigate to Content Browser
- 1.1.1) Content Browser -> /Game/Enemies/
- 1.1.2) Right-click in empty space
- 1.1.3) Select: New Folder
- 1.1.4) Name: Biomech
- 1.1.5) Open Biomech folder
- 1.1.6) Create subfolder: Definitions
- 1.1.7) Create subfolder: Configurations

### **2) Create NPC_BiomechCreature**

#### 2.1) Create NPCDefinition Asset
- 2.1.1) Content Browser -> /Game/Enemies/Biomech/Definitions/
- 2.1.2) Right-click in Content Browser
- 2.1.3) Select: Narrative -> NPC Definition
- 2.1.4) Name: NPC_BiomechCreature
- 2.1.5) Save asset

### **3) Create NPC_BiomechHost**

#### 3.1) Create NPCDefinition Asset
- 3.1.1) Right-click in Content Browser
- 3.1.2) Select: Narrative -> NPC Definition
- 3.1.3) Name: NPC_BiomechHost
- 3.1.4) Save asset

---

## **PHASE 3: CREATE BP_BIOMECHCREATURE**

### **1) Create Blueprint Class**

#### 1.1) Create Asset
- 1.1.1) Content Browser -> /Game/Enemies/Biomech/
- 1.1.2) Right-click in Content Browser
- 1.1.3) Select: Blueprint Class
- 1.1.4) Search parent: NarrativeNPCCharacter
- 1.1.5) Select: NarrativeNPCCharacter
- 1.1.6) Name: BP_BiomechCreature
- 1.1.7) Double-click to open

### **2) Configure Class Defaults**

#### 2.1) Open Class Defaults
- 2.1.1) Click Class Defaults button

#### 2.2) Configure Replication
- 2.2.1) Replicates: Check
- 2.2.2) Replicate Movement: Check

#### 2.3) Configure Character
- 2.3.1) Select Mesh component in Components panel
- 2.3.2) Skeletal Mesh: Select creature mesh asset

#### 2.4) Configure Capsule
- 2.4.1) Select CapsuleComponent in Components panel
- 2.4.2) Capsule Half Height: Adjust for creature size
- 2.4.3) Capsule Radius: Adjust for creature width

### **3) Compile and Save**
- 3.1) Click Compile
- 3.2) Click Save

---

## **PHASE 4: CREATE BP_BIOMECHHOST**

### **1) Create Blueprint Class**

#### 1.1) Create Asset
- 1.1.1) Content Browser -> /Game/Enemies/Biomech/
- 1.1.2) Right-click in Content Browser
- 1.1.3) Select: Blueprint Class
- 1.1.4) Search parent: NarrativeNPCCharacter
- 1.1.5) Select: NarrativeNPCCharacter
- 1.1.6) Name: BP_BiomechHost
- 1.1.7) Double-click to open

### **2) Configure Class Defaults**

#### 2.1) Open Class Defaults
- 2.1.1) Click Class Defaults button

#### 2.2) Configure Replication
- 2.2.1) Replicates: Check
- 2.2.2) Replicate Movement: Check

#### 2.3) Configure Character
- 2.3.1) Select Mesh component in Components panel
- 2.3.2) Skeletal Mesh: Select combined host mesh asset

#### 2.4) Configure Capsule
- 2.4.1) Select CapsuleComponent in Components panel
- 2.4.2) Capsule Half Height: Adjust for host size
- 2.4.3) Capsule Radius: Adjust for host width

### **3) Create Variables**

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| CreatureDefinition | NPCDefinition (Object Reference) | None | Yes |
| CorpseMesh | Static Mesh (Object Reference) | None | Yes |
| SpawnOffset | Vector | (0, 0, 50) | Yes |
| bSpawnCorpse | Boolean | True | Yes |
| DetachmentMontage | Anim Montage (Object Reference) | None | Yes |

### **4) Compile and Save**
- 4.1) Click Compile
- 4.2) Click Save

---

## **PHASE 5: IMPLEMENT DEATH DETACHMENT LOGIC**

### **1) Override HandleDeath Function**

#### 1.1) Create Override
- 1.1.1) Open BP_BiomechHost
- 1.1.2) My Blueprint panel -> Functions -> Override
- 1.1.3) Search: Handle Death
- 1.1.4) Click to override

### **2) Add Authority Check**

#### 2.1) Check Server Authority
- 2.1.1) From Handle Death event:
- 2.1.1.1) Add Has Authority node
- 2.1.2) From Has Authority Return Value:
- 2.1.2.1) Add Branch node
- 2.1.2.2) Connect Return Value to Condition
- 2.1.3) From Branch False:
- 2.1.3.1) Add Call to Parent Function node (HandleDeath)
- 2.1.3.2) Connect execution to Return node

### **3) Add Detaching State Tag**

#### 3.1) Apply State Tag
- 3.1.1) From Branch True:
- 3.1.1.1) Add Get Ability System Component node
- 3.1.1.2) Target: Self
- 3.1.2) From Get ASC Return Value:
- 3.1.2.1) Add Add Loose Gameplay Tag node
- 3.1.2.2) Tag: State.Biomech.Detaching

### **4) Play Detachment Animation**

#### 4.1) Check Montage Valid
- 4.1.1) From Add Loose Gameplay Tag:
- 4.1.1.1) Drag DetachmentMontage variable getter
- 4.1.1.2) Add Is Valid node
- 4.1.2) From Is Valid -> Is Valid pin:
- 4.1.2.1) Add Branch node

#### 4.2) Play Montage
- 4.2.1) From Branch True:
- 4.2.1.1) Add Play Anim Montage node
- 4.2.1.2) Target: Self
- 4.2.1.3) Montage to Play: DetachmentMontage variable
- 4.2.2) From Branch False:
- 4.2.2.1) Route to spawn creature logic (skip animation)

### **5) Get NarrativeCharacterSubsystem**

#### 5.1) Get Subsystem Reference
- 5.1.1) From Play Anim Montage (or Branch False):
- 5.1.1.1) Add Get NarrativeCharacterSubsystem node
- 5.1.2) From Return Value:
- 5.1.2.1) Promote to local variable
- 5.1.2.2) Name: CharacterSubsystem

### **6) Calculate Spawn Transform**

#### 6.1) Get Host Location
- 6.1.1) Add Get Actor Location node
- 6.1.2) Target: Self

#### 6.2) Apply Spawn Offset
- 6.2.1) Add Add (Vector + Vector) node
- 6.2.2) Connect:
- 6.2.2.1) A: Get Actor Location Return Value
- 6.2.2.2) B: SpawnOffset variable

#### 6.3) Get Host Rotation
- 6.3.1) Add Get Actor Rotation node
- 6.3.2) Target: Self

#### 6.4) Make Spawn Transform
- 6.4.1) Add Make Transform node
- 6.4.2) Connect:
- 6.4.2.1) Location: Vector addition result
- 6.4.2.2) Rotation: Get Actor Rotation Return Value
- 6.4.2.3) Scale: Leave default (1, 1, 1)

### **7) Spawn Creature**

#### 7.1) Call SpawnNPC
- 7.1.1) From CharacterSubsystem local variable:
- 7.1.1.1) Add Spawn NPC node
- 7.1.2) Connect:
- 7.1.2.1) Target: CharacterSubsystem
- 7.1.2.2) NPCData: CreatureDefinition variable
- 7.1.2.3) Transform: Make Transform result

#### 7.2) Store Spawned Reference
- 7.2.1) From Spawn NPC Return Value:
- 7.2.1.1) Promote to local variable
- 7.2.1.2) Name: SpawnedCreature

> **CRITICAL: SpawnNPC vs SpawnActor**
> You MUST use `NarrativeCharacterSubsystem->SpawnNPC()`, NOT raw `SpawnActor`. Raw SpawnActor bypasses NPC initialization because `NarrativeCharacterSubsystem.OnActorSpawned` is COMMENTED OUT in Narrative Pro v2.2 (line 327-332). SpawnNPC_Internal calls `SetNPCDefinition()` which triggers the full initialization chain: `OnRep_NPCDefinition()` → `OnDefinitionSet()` → applies AbilityConfiguration, ActivityConfiguration, faction tags, etc.

### **8) Spawn Corpse Mesh (Optional)**

#### 8.1) Check bSpawnCorpse Flag
- 8.1.1) From Spawn NPC execution:
- 8.1.1.1) Drag bSpawnCorpse variable getter
- 8.1.1.2) Add Branch node

#### 8.2) Validate Corpse Mesh
- 8.2.1) From Branch True:
- 8.2.1.1) Drag CorpseMesh variable getter
- 8.2.1.2) Add Is Valid node
- 8.2.2) From Is Valid -> Is Valid pin:
- 8.2.2.1) Add Branch node

#### 8.3) Spawn Static Mesh Actor
- 8.3.1) From Branch True:
- 8.3.1.1) Add Spawn Actor from Class node
- 8.3.1.2) Class: StaticMeshActor
- 8.3.1.3) Spawn Transform: Get Actor Transform (Self)
- 8.3.1.4) Collision Handling Override: Always Spawn

#### 8.4) Configure Corpse
- 8.4.1) From Spawn Actor Return Value:
- 8.4.1.1) Add Cast To StaticMeshActor node
- 8.4.2) From successful cast:
- 8.4.2.1) Add Get Static Mesh Component node
- 8.4.2.2) Target: As Static Mesh Actor
- 8.4.3) From Get Static Mesh Component:
- 8.4.3.1) Add Set Static Mesh node
- 8.4.3.2) New Mesh: CorpseMesh variable
- 8.4.4) From Set Static Mesh:
- 8.4.4.1) Add Set Simulate Physics node
- 8.4.4.2) Target: Static Mesh Component
- 8.4.4.3) Simulate: Check

### **9) Call Parent HandleDeath**

#### 9.1) Execute Parent Implementation
- 9.1.1) From final execution (all branches converge):
- 9.1.1.1) Add Call to Parent Function node
- 9.1.1.2) This calls NarrativeNPCCharacter::HandleDeath

### **10) Compile and Save**
- 10.1) Click Compile
- 10.2) Click Save

---

## **PHASE 6: CREATE ABILITY CONFIGURATIONS**

### **1) Create AC_BiomechCreature**

#### 1.1) Create Asset
- 1.1.1) Content Browser -> /Game/Enemies/Biomech/Configurations/
- 1.1.2) Right-click in Content Browser
- 1.1.3) Select: Narrative -> Ability Configuration
- 1.1.4) Name: AC_BiomechCreature
- 1.1.5) Double-click to open

#### 1.2) Configure Default Attributes
- 1.2.1) Default Attributes: GE_DefaultAttributes (or custom)

#### 1.3) Save Asset

### **2) Create AC_BiomechHost**

#### 2.1) Create Asset
- 2.1.1) Right-click in Content Browser
- 2.1.2) Select: Narrative -> Ability Configuration
- 2.1.3) Name: AC_BiomechHost
- 2.1.4) Double-click to open

#### 2.2) Configure Default Attributes
- 2.2.1) Default Attributes: GE_DefaultAttributes (or custom)

#### 2.3) Save Asset

---

## **PHASE 7: CREATE ACTIVITY CONFIGURATIONS**

### **1) Create AC_BiomechCreatureBehavior**

#### 1.1) Create Asset
- 1.1.1) Content Browser -> /Game/Enemies/Biomech/Configurations/
- 1.1.2) Right-click in Content Browser
- 1.1.3) Select: Narrative -> NPC Activity Configuration
- 1.1.4) Name: AC_BiomechCreatureBehavior
- 1.1.5) Double-click to open

#### 1.2) Configure Default Activities
- 1.2.1) Expand: Default Activities
- 1.2.2) Click + to add elements:

| Index | Activity | Purpose |
|-------|----------|---------|
| 0 | BPA_Attack_Melee | Combat behavior |
| 1 | BPA_Idle | Default fallback |

#### 1.3) Configure Goal Generators
- 1.3.1) Expand: Goal Generators
- 1.3.2) Click + to add elements:

| Index | Generator | Purpose |
|-------|-----------|---------|
| 0 | GoalGenerator_Attack | Creates combat goals from perception |

#### 1.4) Save Asset

### **2) Create AC_BiomechHostBehavior**

#### 2.1) Create Asset
- 2.1.1) Right-click in Content Browser
- 2.1.2) Select: Narrative -> NPC Activity Configuration
- 2.1.3) Name: AC_BiomechHostBehavior
- 2.1.4) Double-click to open

#### 2.2) Configure Default Activities
- 2.2.1) Expand: Default Activities
- 2.2.2) Click + to add elements:

| Index | Activity | Purpose |
|-------|----------|---------|
| 0 | BPA_Patrol | Patrol behavior |
| 1 | BPA_Attack_Melee | Combat behavior |
| 2 | BPA_Idle | Default fallback |

#### 2.3) Configure Goal Generators
- 2.3.1) Expand: Goal Generators
- 2.3.2) Click + to add elements:

| Index | Generator | Purpose |
|-------|-----------|---------|
| 0 | GoalGenerator_Attack | Creates combat goals from perception |

#### 2.4) Save Asset

---

## **PHASE 8: CONFIGURE NPCDEFINITION PROPERTIES**

### **1) Configure NPC_BiomechCreature**

#### 1.1) Open Asset
- 1.1.1) Double-click NPC_BiomechCreature

#### 1.2) Set NPC Class Path
- 1.2.1) NPC Class Path: Select BP_BiomechCreature

#### 1.3) Set Configurations
- 1.3.1) Ability Configuration: AC_BiomechCreature
- 1.3.2) Activity Configuration: AC_BiomechCreatureBehavior

#### 1.4) Configure Identity
- 1.4.1) NPC Name: Biomech Creature
- 1.4.2) NPCID: BiomechCreature
- 1.4.3) Allow Multiple Instances: Check

#### 1.5) Configure Default Tags
- 1.5.1) Default Owned Tags -> Add:
- 1.5.1.1) Faction.Enemy.Biomech

#### 1.6) Save Asset

### **2) Configure NPC_BiomechHost**

#### 2.1) Open Asset
- 2.1.1) Double-click NPC_BiomechHost

#### 2.2) Set NPC Class Path
- 2.2.1) NPC Class Path: Select BP_BiomechHost

#### 2.3) Set Configurations
- 2.3.1) Ability Configuration: AC_BiomechHost
- 2.3.2) Activity Configuration: AC_BiomechHostBehavior

#### 2.4) Configure Identity
- 2.4.1) NPC Name: Biomech Host
- 2.4.2) NPCID: BiomechHost
- 2.4.3) Allow Multiple Instances: Check

#### 2.5) Configure Default Tags
- 2.5.1) Default Owned Tags -> Add:
- 2.5.1.1) Faction.Enemy.Biomech

#### 2.6) Save Asset

### **3) Update BP_BiomechHost Default Value**

#### 3.1) Open BP_BiomechHost
- 3.1.1) Double-click BP_BiomechHost

#### 3.2) Set CreatureDefinition Default
- 3.2.1) Click Class Defaults
- 3.2.2) CreatureDefinition: NPC_BiomechCreature

#### 3.3) Save Asset

---

## **PHASE 9: CONFIGURE FACTION ATTITUDES**

### **1) Open Faction Settings**

#### 1.1) Navigate to Settings
- 1.1.1) Edit -> Project Settings
- 1.1.2) Navigate to: Narrative -> Faction Settings

### **2) Add Biomech Faction**

#### 2.1) Configure Attitude
- 2.1.1) Find Faction Attitudes array
- 2.1.2) Add entry for Faction.Enemy.Biomech
- 2.1.3) Set attitude toward Narrative.Factions.Heroes: Hostile

---

## **PHASE 10: WORLD PLACEMENT**

### **1) Place Host in Level**

#### 1.1) Drag NPCDefinition into World
- 1.1.1) Content Browser -> NPC_BiomechHost
- 1.1.2) Drag into level viewport
- 1.1.3) NPCSpawner automatically created

#### 1.2) Position Spawner
- 1.2.1) Move to desired spawn location
- 1.2.2) Adjust rotation as needed

### **2) Configure Spawner Instance**

#### 2.1) Select NPCSpawner
- 2.1.1) Select in World Outliner

#### 2.2) Configure NPC Spawn Component
- 2.2.1) Details panel -> NPC Spawn Component
- 2.2.2) Spawn On Begin Play: Check (for testing)

#### 2.3) Override Instance Properties (Optional)
- 2.3.1) CreatureDefinition: NPC_BiomechCreature (default)
- 2.3.2) CorpseMesh: Select corpse static mesh
- 2.3.3) SpawnOffset: Adjust if needed
- 2.3.4) bSpawnCorpse: True/False as needed

---

## SYSTEM ARCHITECTURE SUMMARY

### Death Sequence Flow

| Step | Location | Action |
|------|----------|--------|
| 1 | NarrativeAbilitySystemComponent | Health reaches 0, fires OnDied |
| 2 | BP_BiomechHost.HandleDeath | Override receives death event |
| 3 | BP_BiomechHost.HandleDeath | Checks Has Authority |
| 4 | BP_BiomechHost.HandleDeath | Adds State.Biomech.Detaching tag |
| 5 | BP_BiomechHost.HandleDeath | Plays detachment montage |
| 6 | BP_BiomechHost.HandleDeath | Spawns BP_BiomechCreature via subsystem |
| 7 | BP_BiomechHost.HandleDeath | Spawns corpse mesh (optional) |
| 8 | BP_BiomechHost.HandleDeath | Calls parent HandleDeath |
| 9 | BP_BiomechCreature | GoalGenerator_Attack detects player via faction system |
| 10 | BP_BiomechCreature | BPA_Attack_Melee runs combat |

### Activity Scoring

| Entity | Activity | Score | Condition |
|--------|----------|-------|-----------|
| Host | BPA_Attack_Melee | 3.0 | Goal_Attack exists |
| Host | BPA_Patrol | 1.0 | Default behavior |
| Host | BPA_Idle | 0.5 | Fallback |
| Creature | BPA_Attack_Melee | 3.0 | Goal_Attack exists (via faction detection) |
| Creature | BPA_Idle | 0.5 | Fallback |

### Multiplayer Considerations

| Aspect | Implementation |
|--------|----------------|
| Death Detection | Server authority (Has Authority check) |
| Creature Spawning | Server only via SpawnNPC |
| Creature Replication | Automatic via NarrativeNPCCharacter |
| Corpse Spawning | Server only, replicates automatically |
| Tag Replication | Handled by ASC |
| Faction Detection | Automatic via GoalGenerator_Attack |

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 1.2 | January 2026 | **Naming convention fix:** NPCDef_ → NPC_ prefix for NPCDefinition assets, ActConfig_ → AC_*Behavior suffix for ActivityConfiguration assets. Added critical note about SpawnNPC vs SpawnActor (SpawnNPC is required for NPC initialization). Updated UE 5.6 → 5.7, Narrative Pro v2.1 → v2.2. |
| 1.1 | January 2026 | Added PHASE 7 for ActivityConfiguration creation. Added GoalGenerator_Attack references. Added existing Narrative Pro asset documentation. Added faction configuration phase. Added DetachmentMontage variable for animation. Simplified Has Authority pattern. Removed manual aggro transfer (faction system handles automatically via GoalGenerator_Attack). |
| 1.0 | January 2026 | Initial implementation guide |

---

**END OF GUIDE**
