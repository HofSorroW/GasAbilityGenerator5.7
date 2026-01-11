# Father Companion System Setup Guide

## VERSION 3.1

## Unreal Engine 5.6 + Narrative Pro Plugin v2.2

---

## **DOCUMENT INFORMATION**

| Property | Value |
|----------|-------|
| Document Version | 3.1 |
| Document Type | Consolidated System Setup |
| Target Blueprints | BP_FatherCompanion, Player Blueprint, EquippableItems, NarrativeEvents |
| Parent Class | NarrativeNPCCharacter (Father), NarrativeGameplayAbility (Abilities) |
| Total Forms | 5 (Crawler, Armor, Exoskeleton, Symbiote, Engineer) |
| Total Abilities | 17 |
| Last Updated | January 2026 |
| Estimated Time | 12-16 hours |

---

## **OVERVIEW**

This guide consolidates all father companion system setup into a single comprehensive document. It covers gameplay tag configuration, blueprint creation, variable and function setup, player integration, EquippableItem form creation, and NPCDefinition configuration.

### **Key Architecture**

| Component | Source |
|-----------|--------|
| Ability System Component | Auto-created by NarrativeNPCCharacter constructor |
| NarrativeAttributeSetBase | Auto-created by NarrativeNPCCharacter constructor |
| AI Controller | Auto-assigned via NPCDefinition |
| Ownership Setup | NarrativeEvent via dialogue (NOT BeginPlay) |
| Spawning | NPCSpawner system (NOT SpawnActor) |
| Visual Effects | NS_FatherCompanion Niagara system (see Father_Companion_VFX_Guide) |

### **Ownership Architecture**

| Aspect | Implementation |
|--------|----------------|
| Ownership Trigger | Dialogue recruitment node |
| Ownership Event | NE_SetFatherOwner executes |
| Clear Trigger | Dialogue dismiss node |
| Clear Event | NE_ClearFatherOwner executes |
| Form Unlock | Father.State.Recruited tag granted on recruitment |

### **What NarrativeNPCCharacter Provides**

| Feature | Status | Notes |
|---------|--------|-------|
| AbilitySystemComponent | Automatic | Created in constructor, initialized in BeginPlay |
| NarrativeAttributeSetBase | Automatic | Created in constructor |
| INarrativeSavableActor | Inherited | For save/load support |
| InventoryComponent | Automatic | For EquippableItems |
| Server Replication | Automatic | Mixed replication mode |

### **NPCSpawner System Architecture**

| What NPCSpawner Provides | What It Does Not Provide |
|--------------------------|-------------------------|
| Actor spawning at designated world location | Custom initialization logic (setting OwnerPlayer) |
| Respawn conditions (streaming, distance culling) | Player-specific assignments in multiplayer |
| Save system integration for persistent NPCs | Bidirectional reference setup |
| Goal assignment via OptionalGoal field | N/A |
| Follow behavior through NPCGoal_FollowPlayer | N/A |

### **ASC Initialization Architecture**

| Phase | Action | Source |
|-------|--------|--------|
| Constructor | Creates AbilitySystemComponent, sets replication | C++ |
| Constructor | Sets replication mode to Mixed | Automatic |
| Constructor | Creates AttributeSetBase | Automatic |
| BeginPlay (C++) | Calls InitAbilityActorInfo(this, this) | C++ |
| BeginPlay (Blueprint) | ASC fully initialized before Blueprint BeginPlay | Automatic |

### **Reference Setup Pattern**

| Reference | Direction | Purpose |
|-----------|-----------|---------|
| OwnerPlayer | Father -> Player | Attachment, targeting, buff application |
| OwnerPlayerDefinition | Father -> CD_Asset | Save/Load persistence |
| FatherCompanionRef | Player -> Father | Ability activation, UI queries |

### **Recruitment Flow**

| Step | Actor | Action |
|------|-------|--------|
| 1 | Player | Talks to father (dialogue) |
| 2 | Dialogue | Fires NE_SetFatherOwner event |
| 3 | Father | Sets OwnerPlayer, grants Father.State.Recruited |
| 4 | Father | Form abilities become available |

### **Related Documents**

| Document | Purpose |
|----------|---------|
| Father_Companion_VFX_Guide_v1_0 | Niagara visual effects, materials, emitters |
| Father_Companion_Technical_Reference | C++ sources, patterns, architecture |
| Father_Companion_System_Design_Document | Abilities, forms, parameters |
| DefaultGameplayTags_FatherCompanion | Tag definitions and line numbers |

---

## **SYSTEM ARCHITECTURE**

### **NPCDefinition Structure**

| Component | Asset | Purpose |
|-----------|-------|---------|
| NPCDefinition | NPCDef_FatherCompanion | Main father configuration |
| Ability Configuration | AC_FatherCompanion_Default | Grants baseline abilities on spawn |
| Default Item Loadout | IC_FatherForms or direct items | Provides form EquippableItems |

### **Two-Part Ability System**

| Part | Granted Via | Contents |
|------|-------------|----------|
| Baseline Abilities | NPCDefinition -> Ability Configuration | Form abilities + General abilities (always present) |
| Form-Specific Abilities | EquippableItem -> Abilities Array | Action abilities (conditionally granted when form equipped) |

### **Baseline Abilities (AC_FatherCompanion_Default)**

| Index | Ability | Type | Description |
|-------|---------|------|-------------|
| 0 | GA_FatherCrawler | Form | Default following mode |
| 1 | GA_FatherArmor | Form | Chest attachment |
| 2 | GA_FatherExoskeleton | Form | Back attachment |
| 3 | GA_FatherSymbiote | Form | Full body merge |
| 4 | GA_FatherEngineer | Form | Turret deployment |
| 5 | GA_FatherAttack | AI | Melee attack |
| 6 | GA_FatherLaserShot | AI | Ranged attack |
| 7 | GA_FatherMark | Passive | Enemy marking |
| 8 | GA_FatherSacrifice | Passive | Emergency save |

### **Form-Specific Abilities (via EquippableItem)**

| Form | Abilities Granted | Stat Bonuses |
|------|-------------------|--------------|
| Armor | GA_ProtectiveDome, GA_DomeBurst | Armor Rating: +50 |
| Exoskeleton | GA_ExoskeletonDash, GA_ExoskeletonSprint, GA_StealthField | Attack Rating: +10 |
| Symbiote | GA_ProximityStrike | Attack Rating: +100 |
| Engineer | GA_TurretShoot, GA_FatherElectricTrap | None (turret has own stats) |

### **Why EquippableItem (NOT WeaponItem)**

| Scenario | WeaponItem | EquippableItem |
|----------|------------|----------------|
| Player wields Rifle | Occupies weapon slot | Does not conflict |
| Player equips Father Form | CONFLICT - replaces rifle | Uses FatherForm slot |
| Result | Cannot have both | Both active simultaneously |

---

## **TABLE OF CONTENTS**

1. Configure Gameplay Tags
2. Create E_FatherForm Enum
3. Create BP_FatherCompanion Blueprint
4. Create Variables - Core References
5. Create Variables - Form State
6. Create Variables - Attachment Sockets
7. Create Variables - Movement Storage
8. Create Variables - Form Multipliers
9. Create Variables - Engineer Form
10. Create Variables - Mark System
11. Create Variables - Trap System
12. Create Variables - Effect and Ability Handles
13. Configure Variable Replication
14. Create Functions - Reference Management
15. Create Functions - AI Control
16. Create Functions - Enemy Marking
17. Create Functions - Mark Widget Management
18. Create Functions - EndPlay Safety Net
19. Implement BeginPlay
20. Implement Prepare For Save
21. Implement Initialize
22. Implement OnRep Functions
23. Implement EndPlay
24. Configure SaveGame Variables
25. Override Load Function
26. Create Player Blueprint Variables
27. Create Player Blueprint Components
28. Create Player Skeleton Sockets
29. Create AC_FatherCompanion_Default
30. Create Father Form Equipment Modifier GEs
31. Create Armor Form EquippableItem
32. Create Exoskeleton Form EquippableItem
33. Create Symbiote Form EquippableItem
34. Create Engineer Form EquippableItem
35. Create IC_FatherForms (Optional)
36. Configure NPCDef_FatherCompanion
37. Create Narrative Events
38. Create BT_FatherFollow Behavior Tree
39. Create BPA_FatherFollow Activity
40. Create ActConfig_FatherCompanion
41. Update NPCDefinition with Activity Configuration

---

## **PHASE 1: CONFIGURE GAMEPLAY TAGS**

### **1) Add Required Gameplay Tags**

#### 1.1) Open Project Settings
   - 1.1.1) Click: **Edit** menu
   - 1.1.2) Select: **Project Settings**

#### 1.2) Navigate to Gameplay Tags
   - 1.2.1) In left sidebar: Expand **Project** category
   - 1.2.2) Click: **GameplayTags**

#### 1.3) Locate Gameplay Tag Source
   - 1.3.1) Find: **GameplayTagTableList** section
   - 1.3.2) Click: Arrow to expand array
   - 1.3.3) Look for: **DefaultGameplayTags.ini**

#### 1.4) Add Father Equipment Slot Tag

##### 1.4.1) Add Narrative.Equipment.Slot.FatherForm Tag
   - 1.4.1.1) Click: **+** button next to **GameplayTagList**
   - 1.4.1.2) **Tag:** Enter `Narrative.Equipment.Slot.FatherForm`
   - 1.4.1.3) **DevComment:** Enter `Equipment slot for father companion forms (separate from weapon slots)`

#### 1.5) Add Father Form State Tags

##### 1.5.1) Add Father.Form.Crawler Tag
   - 1.5.1.1) Click: **+** button
   - 1.5.1.2) **Tag:** `Father.Form.Crawler`
   - 1.5.1.3) **DevComment:** `Father is in default crawler/follower mode`

##### 1.5.2) Add Father.Form.Armor Tag
   - 1.5.2.1) Click: **+** button
   - 1.5.2.2) **Tag:** `Father.Form.Armor`
   - 1.5.2.3) **DevComment:** `Father is attached as chest armor providing defense`

##### 1.5.3) Add Father.Form.Exoskeleton Tag
   - 1.5.3.1) Click: **+** button
   - 1.5.3.2) **Tag:** `Father.Form.Exoskeleton`
   - 1.5.3.3) **DevComment:** `Father is attached as back exoskeleton providing speed`

##### 1.5.4) Add Father.Form.Symbiote Tag
   - 1.5.4.1) Click: **+** button
   - 1.5.4.2) **Tag:** `Father.Form.Symbiote`
   - 1.5.4.3) **DevComment:** `Father merged with player as full-body symbiote`

##### 1.5.5) Add Father.Form.Engineer Tag
   - 1.5.5.1) Click: **+** button
   - 1.5.5.2) **Tag:** `Father.Form.Engineer`
   - 1.5.5.3) **DevComment:** `Father deployed as stationary turret`

#### 1.6) Add Father Attachment State Tags

##### 1.6.1) Add Father.State.Attached Tag
   - 1.6.1.1) Click: **+** button
   - 1.6.1.2) **Tag:** `Father.State.Attached`
   - 1.6.1.3) **DevComment:** `Father is currently attached to player`

##### 1.6.2) Add Father.State.Detached Tag
   - 1.6.2.1) Click: **+** button
   - 1.6.2.2) **Tag:** `Father.State.Detached`
   - 1.6.2.3) **DevComment:** `Father is currently detached from player`

##### 1.6.3) Add Father.State.Merged Tag
   - 1.6.3.1) Click: **+** button
   - 1.6.3.2) **Tag:** `Father.State.Merged`
   - 1.6.3.3) **DevComment:** `Father is merged with player (Symbiote form)`

##### 1.6.4) Add Father.State.Deployed Tag
   - 1.6.4.1) Click: **+** button
   - 1.6.4.2) **Tag:** `Father.State.Deployed`
   - 1.6.4.3) **DevComment:** `Father is deployed as turret (Engineer form)`

##### 1.6.5) Add Father.State.Recruited Tag
   - 1.6.5.1) Click: **+** button
   - 1.6.5.2) **Tag:** `Father.State.Recruited`
   - 1.6.5.3) **DevComment:** `Father has been recruited by player - required for form abilities`

#### 1.7) Add Father Ability Tags

##### 1.7.1) Add Ability.Father.Crawler Tag
   - 1.7.1.1) Click: **+** button
   - 1.7.1.2) **Tag:** `Ability.Father.Crawler`
   - 1.7.1.3) **DevComment:** `Father crawler form ability`

##### 1.7.2) Add Ability.Father.Armor Tag
   - 1.7.2.1) Click: **+** button
   - 1.7.2.2) **Tag:** `Ability.Father.Armor`
   - 1.7.2.3) **DevComment:** `Father armor form ability`

##### 1.7.3) Add Ability.Father.Exoskeleton Tag
   - 1.7.3.1) Click: **+** button
   - 1.7.3.2) **Tag:** `Ability.Father.Exoskeleton`
   - 1.7.3.3) **DevComment:** `Father exoskeleton form ability`

##### 1.7.4) Add Ability.Father.Symbiote Tag
   - 1.7.4.1) Click: **+** button
   - 1.7.4.2) **Tag:** `Ability.Father.Symbiote`
   - 1.7.4.3) **DevComment:** `Father symbiote form ability`

##### 1.7.5) Add Ability.Father.Engineer Tag
   - 1.7.5.1) Click: **+** button
   - 1.7.5.2) **Tag:** `Ability.Father.Engineer`
   - 1.7.5.3) **DevComment:** `Father engineer form ability`

#### 1.8) Save and Restart Editor
   - 1.8.1) Click: **Save** button
   - 1.8.2) Close: Project Settings
   - 1.8.3) Restart: Unreal Editor

---

## **PHASE 2: CREATE E_FATHERFORM ENUM**

### **1) Create Enumeration Asset**

#### 1.1) Navigate to Folder
   - 1.1.1) In Content Browser, navigate to `/Content/FatherCompanion/Data/`
   - 1.1.2) If folder does not exist, create folder structure

#### 1.2) Create Enum
   - 1.2.1) Right-click in empty space
   - 1.2.2) Hover over **Blueprints** submenu
   - 1.2.3) Click **Enumeration**
   - 1.2.4) Name: `E_FatherForm`
   - 1.2.5) Press **Enter** to confirm
   - 1.2.6) Double-click to open

### **2) Add Enum Values**

#### 2.1) Add Crawler Value
   - 2.1.1) Click **Add Enumerator** button
   - 2.1.2) Display Name: `Crawler`
   - 2.1.3) Description: `Default following form - autonomous AI mode`

#### 2.2) Add Armor Value
   - 2.2.1) Click **Add Enumerator** button
   - 2.2.2) Display Name: `Armor`
   - 2.2.3) Description: `Chest attachment - defensive bonuses`

#### 2.3) Add Exoskeleton Value
   - 2.3.1) Click **Add Enumerator** button
   - 2.3.2) Display Name: `Exoskeleton`
   - 2.3.3) Description: `Back attachment - mobility bonuses`

#### 2.4) Add Symbiote Value
   - 2.4.1) Click **Add Enumerator** button
   - 2.4.2) Display Name: `Symbiote`
   - 2.4.3) Description: `Full body merge - ultimate berserker form`

#### 2.5) Add Engineer Value
   - 2.5.1) Click **Add Enumerator** button
   - 2.5.2) Display Name: `Engineer`
   - 2.5.3) Description: `Deployed turret - stationary artillery mode`

#### 2.6) Add Offline Value
   - 2.6.1) Click **Add Enumerator** button
   - 2.6.2) Display Name: `Offline`
   - 2.6.3) Description: `Dormant state during sacrifice recovery`

### **3) Save Enum**
   - 3.1) Click **Save** button
   - 3.2) Close editor

---

## **PHASE 3: CREATE BP_FATHERCOMPANION BLUEPRINT**

### **1) Create Blueprint Class**

#### 1.1) Navigate to Folder
   - 1.1.1) In Content Browser, navigate to `/Content/FatherCompanion/Characters/`
   - 1.1.2) If folder does not exist, create folder structure

#### 1.2) Create Blueprint
   - 1.2.1) Right-click in empty space
   - 1.2.2) Select **Blueprint Class**
   - 1.2.3) In **Pick Parent Class** dialog, expand **All Classes**
   - 1.2.4) Search: `NarrativeNPCCharacter`
   - 1.2.5) Select **NarrativeNPCCharacter**
   - 1.2.6) Click **Select** button
   - 1.2.7) Name: `BP_FatherCompanion`
   - 1.2.8) Press **Enter** to confirm
   - 1.2.9) Double-click to open Blueprint Editor

### **2) Configure Capsule Component**

#### 2.1) Select Capsule Component
   - 2.1.1) In Components panel, select **CapsuleComponent** (root)

#### 2.2) Set Capsule Size
   - 2.2.1) In Details panel, find **Shape** category
   - 2.2.2) **Capsule Half Height**: `45.0`
   - 2.2.3) **Capsule Radius**: `30.0`

### **3) Configure Mesh Component**

#### 3.1) Select Mesh Component
   - 3.1.1) In Components panel, click **Mesh (CharacterMesh0)**

#### 3.2) Assign Skeletal Mesh
   - 3.2.1) In Details panel, find **Mesh** category
   - 3.2.2) **Skeletal Mesh**: Select your father mesh asset (e.g., `SK_FatherCompanion`)
   - 3.2.3) **Location Z**: `-90.0` (adjust to match capsule)

### **4) Configure Character Movement**

#### 4.1) Select CharacterMovement Component
   - 4.1.1) In Components panel, click **CharacterMovement**

#### 4.2) Set Movement Properties
   - 4.2.1) In Details panel, find **Character Movement: Walking** category
   - 4.2.2) **Max Walk Speed**: `400.0`
   - 4.2.3) **Max Walk Speed Crouched**: `200.0`
   - 4.2.4) **Rotation Rate Z**: `720.0`

#### 4.3) Set Network Properties
   - 4.3.1) Find **Character Movement (Networking)** category
   - 4.3.2) **Network Smoothing Mode**: `Exponential`

### **5) Compile Initial Blueprint**
   - 5.1) Click **Compile** button
   - 5.2) Click **Save** button

---

## **PHASE 4: CREATE VARIABLES - CORE REFERENCES**

### **1) Create OwnerPlayer Variable**

#### 1.1) Add Variable
   - 1.1.1) In My Blueprint panel, find **Variables** section
   - 1.1.2) Click **+** button next to Variables header
   - 1.1.3) Name: `OwnerPlayer`
   - 1.1.4) Press **Enter** to confirm

#### 1.2) Configure Variable Type
   - 1.2.1) With variable selected in list
   - 1.2.2) In Details panel, find **Variable Type** property
   - 1.2.3) Click dropdown
   - 1.2.4) Search: `NarrativeCharacter`
   - 1.2.5) Select **NarrativeCharacter** -> **Object Reference**

#### 1.3) Configure Variable Properties
   - 1.3.1) **Instance Editable**: Check
   - 1.3.2) **Blueprint Read Only**: Uncheck
   - 1.3.3) **Replication**: Select **Replicated**
   - 1.3.4) **Replication Condition**: Select **Initial Only**
   - 1.3.5) **Category**: `Father|References`
   - 1.3.6) **Tooltip**: `Reference to the player that owns this father companion`

### **2) Create OwnerPlayerDefinition Variable**

#### 2.1) Add Variable
   - 2.1.1) Click **+** button next to Variables
   - 2.1.2) Name: `OwnerPlayerDefinition`

#### 2.2) Configure Variable Type
   - 2.2.1) **Variable Type**: Search `Character Definition`
   - 2.2.2) Select **Character Definition** -> **Object Reference**

#### 2.3) Configure Variable Properties
   - 2.3.1) **Instance Editable**: Uncheck
   - 2.3.2) **Replication**: None
   - 2.3.3) **Category**: `Father|References`
   - 2.3.4) **Tooltip**: `Character Definition asset for OwnerPlayer - used for save/load persistence`

---

## **PHASE 5: CREATE VARIABLES - FORM STATE**

### **1) Create CurrentForm Variable**

#### 1.1) Add Variable
   - 1.1.1) Click **+** button
   - 1.1.2) Name: `CurrentForm`

#### 1.2) Configure Variable Type
   - 1.2.1) **Variable Type**: Search `E_FatherForm`
   - 1.2.2) Select **E_FatherForm** (enum created in Phase 2)

#### 1.3) Configure Variable Properties
   - 1.3.1) **Instance Editable**: Uncheck
   - 1.3.2) **Replication**: Select **RepNotify**
   - 1.3.3) **Replication Condition**: Select **None**
   - 1.3.4) **Category**: `Father|State`
   - 1.3.5) **Default Value**: `Crawler`
   - 1.3.6) **Tooltip**: `Current form the father is in`

### **2) Create IsAttached Variable**

#### 2.1) Add Variable
   - 2.1.1) Click **+** button
   - 2.1.2) Name: `IsAttached`

#### 2.2) Configure Variable
   - 2.2.1) **Variable Type**: `Boolean`
   - 2.2.2) **Instance Editable**: Uncheck
   - 2.2.3) **Replication**: Select **RepNotify**
   - 2.2.4) **Replication Condition**: Select **None**
   - 2.2.5) **Category**: `Father|State`
   - 2.2.6) **Default Value**: Unchecked (false)
   - 2.2.7) **Tooltip**: `Whether father is attached to player`

### **3) Create IsDeployed Variable**

#### 3.1) Add Variable
   - 3.1.1) Click **+** button
   - 3.1.2) Name: `IsDeployed`

#### 3.2) Configure Variable
   - 3.2.1) **Variable Type**: `Boolean`
   - 3.2.2) **Instance Editable**: Uncheck
   - 3.2.3) **Replication**: Select **Replicated**
   - 3.2.4) **Replication Condition**: Select **None**
   - 3.2.5) **Category**: `Father|State`
   - 3.2.6) **Default Value**: Unchecked (false)
   - 3.2.7) **Tooltip**: `Whether father is deployed as turret in Engineer form`

---

## **PHASE 6: CREATE VARIABLES - ATTACHMENT SOCKETS**

### **1) Create ChestSocketName Variable**

#### 1.1) Add Variable
   - 1.1.1) Click **+** button
   - 1.1.2) Name: `ChestSocketName`

#### 1.2) Configure Variable Type
   - 1.2.1) **Variable Type**: `Name`

#### 1.3) Configure Variable Properties
   - 1.3.1) **Instance Editable**: Check
   - 1.3.2) **Replication**: None
   - 1.3.3) **Category**: `Father|Attachment`
   - 1.3.4) **Default Value**: `FatherChestSocket`
   - 1.3.5) **Tooltip**: `Socket name on player skeleton for Armor form chest attachment`

### **2) Create BackSocketName Variable**

#### 2.1) Add Variable
   - 2.1.1) Click **+** button
   - 2.1.2) Name: `BackSocketName`

#### 2.2) Configure Variable
   - 2.2.1) **Variable Type**: `Name`
   - 2.2.2) **Instance Editable**: Check
   - 2.2.3) **Replication**: None
   - 2.2.4) **Category**: `Father|Attachment`
   - 2.2.5) **Default Value**: `FatherBackSocket`
   - 2.2.6) **Tooltip**: `Socket name on player skeleton for Exoskeleton form back attachment`

---

## **PHASE 7: CREATE VARIABLES - MOVEMENT STORAGE**

### **1) Create OriginalMaxWalkSpeed Variable**

#### 1.1) Add Variable
   - 1.1.1) Click **+** button
   - 1.1.2) Name: `OriginalMaxWalkSpeed`

#### 1.2) Configure Variable
   - 1.2.1) **Variable Type**: `Float`
   - 1.2.2) **Instance Editable**: Uncheck
   - 1.2.3) **Replication**: None
   - 1.2.4) **Category**: `Father|Movement`
   - 1.2.5) **Default Value**: `0.0`
   - 1.2.6) **Tooltip**: `Stores player original walk speed before form modification`

### **2) Create OriginalJumpZVelocity Variable**

#### 2.1) Add Variable
   - 2.1.1) Click **+** button
   - 2.1.2) Name: `OriginalJumpZVelocity`

#### 2.2) Configure Variable
   - 2.2.1) **Variable Type**: `Float`
   - 2.2.2) **Instance Editable**: Uncheck
   - 2.2.3) **Replication**: None
   - 2.2.4) **Category**: `Father|Movement`
   - 2.2.5) **Default Value**: `0.0`
   - 2.2.6) **Tooltip**: `Stores player original jump velocity before form modification`

---

## **PHASE 8: CREATE VARIABLES - FORM MULTIPLIERS**

### **1) Create ArmorSpeedMultiplier Variable**

#### 1.1) Add Variable
   - 1.1.1) Click **+** button
   - 1.1.2) Name: `ArmorSpeedMultiplier`

#### 1.2) Configure Variable
   - 1.2.1) **Variable Type**: `Float`
   - 1.2.2) **Instance Editable**: Check
   - 1.2.3) **Replication**: None
   - 1.2.4) **Category**: `Father|Armor`
   - 1.2.5) **Default Value**: `0.85`
   - 1.2.6) **Tooltip**: `Movement speed multiplier when in Armor form (0.85 = 15% reduction)`

### **2) Create SpeedBoostMultiplier Variable**

#### 2.1) Add Variable
   - 2.1.1) Click **+** button
   - 2.1.2) Name: `SpeedBoostMultiplier`

#### 2.2) Configure Variable
   - 2.2.1) **Variable Type**: `Float`
   - 2.2.2) **Instance Editable**: Check
   - 2.2.3) **Replication**: None
   - 2.2.4) **Category**: `Father|Exoskeleton`
   - 2.2.5) **Default Value**: `1.25`
   - 2.2.6) **Tooltip**: `Movement speed multiplier when in Exoskeleton form (1.25 = 25% boost)`

### **3) Create JumpBoostMultiplier Variable**

#### 3.1) Add Variable
   - 3.1.1) Click **+** button
   - 3.1.2) Name: `JumpBoostMultiplier`

#### 3.2) Configure Variable
   - 3.2.1) **Variable Type**: `Float`
   - 3.2.2) **Instance Editable**: Check
   - 3.2.3) **Replication**: None
   - 3.2.4) **Category**: `Father|Exoskeleton`
   - 3.2.5) **Default Value**: `1.2`
   - 3.2.6) **Tooltip**: `Jump height multiplier when in Exoskeleton form (1.2 = 20% boost)`

---

## **PHASE 9: CREATE VARIABLES - ENGINEER FORM**

### **1) Create DeployedTransform Variable**

#### 1.1) Add Variable
   - 1.1.1) Click **+** button
   - 1.1.2) Name: `DeployedTransform`

#### 1.2) Configure Variable
   - 1.2.1) **Variable Type**: Search `Transform`
   - 1.2.2) Select **Transform** (Structure)
   - 1.2.3) **Instance Editable**: Uncheck
   - 1.2.4) **Replication**: Select **Replicated**
   - 1.2.5) **Replication Condition**: Select **None**
   - 1.2.6) **Category**: `Father|Engineer`
   - 1.2.7) **Tooltip**: `World transform where father turret is deployed (position + rotation)`

### **2) Create TurretModeEffectHandle Variable**

#### 2.1) Add Variable
   - 2.1.1) Click **+** button
   - 2.1.2) Name: `TurretModeEffectHandle`

#### 2.2) Configure Variable
   - 2.2.1) **Variable Type**: Search `Active Gameplay Effect Handle`
   - 2.2.2) Select **Active Gameplay Effect Handle** (Structure)
   - 2.2.3) **Instance Editable**: Uncheck
   - 2.2.4) **Replication**: None
   - 2.2.5) **Category**: `Father|Engineer`
   - 2.2.6) **Tooltip**: `Handle for turret mode effect removal on form change`

---

## **PHASE 10: CREATE VARIABLES - MARK SYSTEM**

### **1) Create MarkedEnemies Variable**

#### 1.1) Add Variable
   - 1.1.1) Click **+** button
   - 1.1.2) Name: `MarkedEnemies`

#### 1.2) Configure Variable Type
   - 1.2.1) In Details panel, click **Variable Type** dropdown
   - 1.2.2) Select **Actor** -> **Object Reference**
   - 1.2.3) Click array icon next to type to convert to array
   - 1.2.4) Type shows as **Actor Object Reference (Array)**

#### 1.3) Configure Variable Properties
   - 1.3.1) **Instance Editable**: Uncheck
   - 1.3.2) **Replication**: Select **Replicated**
   - 1.3.3) **Replication Condition**: Select **None**
   - 1.3.4) **Category**: `Father|Combat`
   - 1.3.5) **Tooltip**: `Array of enemies currently marked by GA_FatherMark`

### **2) Create MaxMarks Variable**

#### 2.1) Add Variable
   - 2.1.1) Click **+** button
   - 2.1.2) Name: `MaxMarks`

#### 2.2) Configure Variable
   - 2.2.1) **Variable Type**: `Integer`
   - 2.2.2) **Instance Editable**: Check
   - 2.2.3) **Replication**: None
   - 2.2.4) **Category**: `Father|Combat`
   - 2.2.5) **Default Value**: `3`
   - 2.2.6) **Tooltip**: `Maximum number of concurrent enemy marks`

### **3) Create PendingMarkTarget Variable**

#### 3.1) Add Variable
   - 3.1.1) Click **+** button
   - 3.1.2) Name: `PendingMarkTarget`

#### 3.2) Configure Variable
   - 3.2.1) **Variable Type**: `Actor` -> **Object Reference**
   - 3.2.2) **Instance Editable**: Uncheck
   - 3.2.3) **Replication**: None
   - 3.2.4) **Category**: `Father|Combat`
   - 3.2.5) **Tooltip**: `Target queued for marking on next attack`

### **4) Create MarkWidgetMap Variable**

#### 4.1) Add Variable
   - 4.1.1) Click **+** button
   - 4.1.2) Name: `MarkWidgetMap`

#### 4.2) Configure Variable Type
   - 4.2.1) **Variable Type**: Click dropdown
   - 4.2.2) Select **Map** type
   - 4.2.3) Key Type: `Actor` -> **Object Reference**
   - 4.2.4) Value Type: `Widget Component` -> **Object Reference**

#### 4.3) Configure Variable Properties
   - 4.3.1) **Instance Editable**: Uncheck
   - 4.3.2) **Replication**: None
   - 4.3.3) **Category**: `Father|Combat`
   - 4.3.4) **Tooltip**: `Maps enemies to their mark indicator widgets`

---

## **PHASE 11: CREATE VARIABLES - TRAP SYSTEM**

### **1) Create ActiveTraps Variable**

#### 1.1) Add Variable
   - 1.1.1) Click **+** button
   - 1.1.2) Name: `ActiveTraps`

#### 1.2) Configure Variable Type
   - 1.2.1) **Variable Type**: `Actor` -> **Object Reference** -> Convert to Array
   - 1.2.2) Type shows as **Actor Object Reference (Array)**

#### 1.3) Configure Variable Properties
   - 1.3.1) **Instance Editable**: Uncheck
   - 1.3.2) **Replication**: None
   - 1.3.3) **Category**: `Father|Combat`
   - 1.3.4) **Tooltip**: `Array of active electric traps (max 2)`

### **2) Create MaxActiveTraps Variable**

#### 2.1) Add Variable
   - 2.1.1) Click **+** button
   - 2.1.2) Name: `MaxActiveTraps`

#### 2.2) Configure Variable
   - 2.2.1) **Variable Type**: `Integer`
   - 2.2.2) **Instance Editable**: Check
   - 2.2.3) **Replication**: None
   - 2.2.4) **Category**: `Father|Combat`
   - 2.2.5) **Default Value**: `2`
   - 2.2.6) **Tooltip**: `Maximum number of electric traps that can exist simultaneously`

---

## **PHASE 12: CREATE VARIABLES - EFFECT AND ABILITY HANDLES**

### **1) Create CrawlerStateHandle Variable**

#### 1.1) Add Variable
   - 1.1.1) Click **+** button next to Variables
   - 1.1.2) Name: `CrawlerStateHandle`

#### 1.2) Configure Variable
   - 1.2.1) **Variable Type**: Search `Active Gameplay Effect Handle`
   - 1.2.2) Select **Active Gameplay Effect Handle** (Structure)
   - 1.2.3) **Instance Editable**: Uncheck
   - 1.2.4) **Replication**: None
   - 1.2.5) **Category**: `Father|Handles`
   - 1.2.6) **Tooltip**: `Handle for GE_CrawlerState removal`

### **2) Create ArmorStateHandle Variable**

#### 2.1) Add Variable
   - 2.1.1) Click **+** button
   - 2.1.2) Name: `ArmorStateHandle`

#### 2.2) Configure Variable
   - 2.2.1) **Variable Type**: `Active Gameplay Effect Handle` (Structure)
   - 2.2.2) **Replication**: None
   - 2.2.3) **Category**: `Father|Handles`
   - 2.2.4) **Tooltip**: `Handle for GE_ArmorState removal`

### **3) Create ArmorBoostHandle Variable**

#### 3.1) Add Variable
   - 3.1.1) Click **+** button
   - 3.1.2) Name: `ArmorBoostHandle`

#### 3.2) Configure Variable
   - 3.2.1) **Variable Type**: `Active Gameplay Effect Handle` (Structure)
   - 3.2.2) **Replication**: None
   - 3.2.3) **Category**: `Father|Handles`
   - 3.2.4) **Tooltip**: `Handle for GE_ArmorBoost removal from player`

### **4) Create ExoskeletonStateHandle Variable**

#### 4.1) Add Variable
   - 4.1.1) Click **+** button
   - 4.1.2) Name: `ExoskeletonStateHandle`

#### 4.2) Configure Variable
   - 4.2.1) **Variable Type**: `Active Gameplay Effect Handle` (Structure)
   - 4.2.2) **Replication**: None
   - 4.2.3) **Category**: `Father|Handles`
   - 4.2.4) **Tooltip**: `Handle for GE_ExoskeletonState removal`

### **5) Create ExoskeletonSpeedHandle Variable**

#### 5.1) Add Variable
   - 5.1.1) Click **+** button
   - 5.1.2) Name: `ExoskeletonSpeedHandle`

#### 5.2) Configure Variable
   - 5.2.1) **Variable Type**: `Active Gameplay Effect Handle` (Structure)
   - 5.2.2) **Replication**: None
   - 5.2.3) **Category**: `Father|Handles`
   - 5.2.4) **Tooltip**: `Handle for GE_ExoskeletonSpeed removal from player`

### **6) Create SymbioteStateHandle Variable**

#### 6.1) Add Variable
   - 6.1.1) Click **+** button
   - 6.1.2) Name: `SymbioteStateHandle`

#### 6.2) Configure Variable
   - 6.2.1) **Variable Type**: `Active Gameplay Effect Handle` (Structure)
   - 6.2.2) **Replication**: None
   - 6.2.3) **Category**: `Father|Handles`
   - 6.2.4) **Tooltip**: `Handle for GE_SymbioteState removal`

### **7) Create SymbioteBoostHandle Variable**

#### 7.1) Add Variable
   - 7.1.1) Click **+** button
   - 7.1.2) Name: `SymbioteBoostHandle`

#### 7.2) Configure Variable
   - 7.2.1) **Variable Type**: `Active Gameplay Effect Handle` (Structure)
   - 7.2.2) **Replication**: None
   - 7.2.3) **Category**: `Father|Handles`
   - 7.2.4) **Tooltip**: `Handle for GE_SymbioteBoost removal from player`

### **8) Create EngineerStateHandle Variable**

#### 8.1) Add Variable
   - 8.1.1) Click **+** button
   - 8.1.2) Name: `EngineerStateHandle`

#### 8.2) Configure Variable
   - 8.2.1) **Variable Type**: `Active Gameplay Effect Handle` (Structure)
   - 8.2.2) **Replication**: None
   - 8.2.3) **Category**: `Father|Handles`
   - 8.2.4) **Tooltip**: `Handle for GE_EngineerState removal`

### **9) Create FormCooldownHandle Variable**

#### 9.1) Add Variable
   - 9.1.1) Click **+** button
   - 9.1.2) Name: `FormCooldownHandle`

#### 9.2) Configure Variable
   - 9.2.1) **Variable Type**: `Active Gameplay Effect Handle` (Structure)
   - 9.2.2) **Replication**: None
   - 9.2.3) **Category**: `Father|Handles`
   - 9.2.4) **Tooltip**: `Handle for GE_FormChangeCooldown removal`

### **10) Create InvulnerableHandle Variable**

#### 10.1) Add Variable
   - 10.1.1) Click **+** button
   - 10.1.2) Name: `InvulnerableHandle`

#### 10.2) Configure Variable
   - 10.2.1) **Variable Type**: `Active Gameplay Effect Handle` (Structure)
   - 10.2.2) **Replication**: None
   - 10.2.3) **Category**: `Father|Handles`
   - 10.2.4) **Tooltip**: `Handle for form transition invulnerability removal`

### **11) Create DomeAbilityHandle Variable**

#### 11.1) Add Variable
   - 11.1.1) Click **+** button
   - 11.1.2) Name: `DomeAbilityHandle`

#### 11.2) Configure Variable
   - 11.2.1) **Variable Type**: Search `Gameplay Ability Spec Handle`
   - 11.2.2) Select **Gameplay Ability Spec Handle** (Structure)
   - 11.2.3) **Replication**: None
   - 11.2.4) **Category**: `Father|Handles`
   - 11.2.5) **Tooltip**: `Handle for GA_ProtectiveDome granted to player - for two-step cleanup`

### **12) Create DomeBurstAbilityHandle Variable**

#### 12.1) Add Variable
   - 12.1.1) Click **+** button
   - 12.1.2) Name: `DomeBurstAbilityHandle`

#### 12.2) Configure Variable
   - 12.2.1) **Variable Type**: `Gameplay Ability Spec Handle` (Structure)
   - 12.2.2) **Replication**: None
   - 12.2.3) **Category**: `Father|Handles`
   - 12.2.4) **Tooltip**: `Handle for GA_DomeBurst granted to player - for two-step cleanup`

### **13) Create DashAbilityHandle Variable**

#### 13.1) Add Variable
   - 13.1.1) Click **+** button
   - 13.1.2) Name: `DashAbilityHandle`

#### 13.2) Configure Variable
   - 13.2.1) **Variable Type**: `Gameplay Ability Spec Handle` (Structure)
   - 13.2.2) **Replication**: None
   - 13.2.3) **Category**: `Father|Handles`
   - 13.2.4) **Tooltip**: `Handle for GA_FatherExoskeletonDash granted to player - for two-step cleanup`

### **14) Create SprintAbilityHandle Variable**

#### 14.1) Add Variable
   - 14.1.1) Click **+** button
   - 14.1.2) Name: `SprintAbilityHandle`

#### 14.2) Configure Variable
   - 14.2.1) **Variable Type**: `Gameplay Ability Spec Handle` (Structure)
   - 14.2.2) **Replication**: None
   - 14.2.3) **Category**: `Father|Handles`
   - 14.2.4) **Tooltip**: `Handle for GA_FatherExoskeletonSprint granted to player - for two-step cleanup`

### **15) Create StealthAbilityHandle Variable**

#### 15.1) Add Variable
   - 15.1.1) Click **+** button
   - 15.1.2) Name: `StealthAbilityHandle`

#### 15.2) Configure Variable
   - 15.2.1) **Variable Type**: `Gameplay Ability Spec Handle` (Structure)
   - 15.2.2) **Replication**: None
   - 15.2.3) **Category**: `Father|Handles`
   - 15.2.4) **Tooltip**: `Handle for GA_StealthField granted to player - for two-step cleanup`

### **16) Create ProximityAbilityHandle Variable**

#### 16.1) Add Variable
   - 16.1.1) Click **+** button
   - 16.1.2) Name: `ProximityAbilityHandle`

#### 16.2) Configure Variable
   - 16.2.1) **Variable Type**: `Gameplay Ability Spec Handle` (Structure)
   - 16.2.2) **Replication**: None
   - 16.2.3) **Category**: `Father|Handles`
   - 16.2.4) **Tooltip**: `Handle for GA_ProximityStrike granted to player - for two-step cleanup`

### **17) Create bIsFirstActivation Variable**

#### 17.1) Add Variable
   - 17.1.1) Click **+** button
   - 17.1.2) Name: `bIsFirstActivation`

#### 17.2) Configure Variable
   - 17.2.1) **Variable Type**: `Boolean`
   - 17.2.2) **Instance Editable**: Uncheck
   - 17.2.3) **Replication**: None
   - 17.2.4) **Category**: `Father|State`
   - 17.2.5) **Default Value**: Checked (true)
   - 17.2.6) **Tooltip**: `Tracks if this is first form activation vs form switch`

---

## **PHASE 13: CONFIGURE VARIABLE REPLICATION**

### **Variable Replication Summary**

| Variable | Replication Type | Replication Condition | Purpose |
|----------|------------------|----------------------|---------|
| OwnerPlayer | Replicated | Initial Only | Set once at recruitment, never changes |
| OwnerPlayerDefinition | None | N/A | Local save/load persistence |
| CurrentForm | RepNotify | None | All clients see form changes |
| IsAttached | RepNotify | None | All clients see attachment |
| IsDeployed | Replicated | None | All clients see turret mode |
| DeployedTransform | Replicated | None | All clients need turret position/rotation |
| MarkedEnemies | Replicated | None | All clients see mark indicators |
| Effect Handles (all) | None | N/A | Server-only handle management |
| Ability Handles (all) | None | N/A | Server-only handle management |

### **1) Compile to Generate OnRep Functions**

#### 1.1) Compile Blueprint
   - 1.1.1) Click **Compile** button
   - 1.1.2) Wait for compilation to complete
   - 1.1.3) RepNotify variables now have OnRep functions generated

---

## **PHASE 14: CREATE FUNCTIONS - REFERENCE MANAGEMENT**

### **1) Create GetOwnerPlayer Function**

#### 1.1) Add Function
   - 1.1.1) In My Blueprint panel, find **Functions** section
   - 1.1.2) Click **+** button next to Functions header
   - 1.1.3) Name: `GetOwnerPlayer`
   - 1.1.4) Double-click to open function graph

#### 1.2) Configure Return Value
   - 1.2.1) Select function in My Blueprint panel
   - 1.2.2) In Details panel, find **Outputs** section
   - 1.2.3) Click **+** (New Parameter) button
   - 1.2.4) **Parameter Name**: `ReturnValue`
   - 1.2.5) **Parameter Type**: `NarrativeCharacter` -> **Object Reference**

#### 1.3) Implement Function Logic
   - 1.3.1) In function graph, Entry node exists
   - 1.3.2) Drag **OwnerPlayer** variable into graph (GET)
   - 1.3.3) Connect OwnerPlayer output pin to Return Node -> ReturnValue pin

#### 1.4) Configure Function Properties
   - 1.4.1) **Pure**: Check
   - 1.4.2) **Category**: `Father|References`

### **2) Create InitOwnerReference Function**

#### 2.1) Add Function
   - 2.1.1) Click **+** button next to Functions
   - 2.1.2) Name: `InitOwnerReference`
   - 2.1.3) Double-click to open function graph

#### 2.2) Configure Input Parameter
   - 2.2.1) In Details panel, find **Inputs** section
   - 2.2.2) Click **+** to add parameter
   - 2.2.3) **Parameter Name**: `NewOwner`
   - 2.2.4) **Parameter Type**: `NarrativeCharacter` -> **Object Reference**

#### 2.3) Implement Function Logic

##### 2.3.1) Validate Input
   - 2.3.1.1) From Entry execution pin:
   - 2.3.1.2) Add **Is Valid** node
   - 2.3.1.3) Connect NewOwner to Input Object

##### 2.3.2) Add Branch
   - 2.3.2.1) Add **Branch** node
   - 2.3.2.2) Connect Is Valid return to Condition

##### 2.3.3) Set OwnerPlayer
   - 2.3.3.1) From Branch True:
   - 2.3.3.2) Add **Set OwnerPlayer** node
   - 2.3.3.3) Connect NewOwner to value input

##### 2.3.4) Cast to Player
   - 2.3.4.1) From Set OwnerPlayer execution:
   - 2.3.4.2) Add **Cast To BP_NarrativePlayer** node
   - 2.3.4.3) Connect NewOwner to Object input

##### 2.3.5) Set FatherCompanionRef on Player
   - 2.3.5.1) From Cast success:
   - 2.3.5.2) Add **Set FatherCompanionRef** node
   - 2.3.5.3) Target: Connect As BP_NarrativePlayer
   - 2.3.5.4) Value: Add **Self** reference

##### 2.3.6) Apply Recruited Tag
   - 2.3.6.1) From Set FatherCompanionRef execution:
   - 2.3.6.2) Get **Self** reference
   - 2.3.6.3) From Self, search: `Get Ability System Component`
   - 2.3.6.4) Add **Get Ability System Component** node
   - 2.3.6.5) From ASC Return Value, drag outward
   - 2.3.6.6) Search: `Apply Gameplay Effect to Self`
   - 2.3.6.7) Add **Apply Gameplay Effect to Self** node
   - 2.3.6.8) Connect ASC to **Target**
   - 2.3.6.9) **Gameplay Effect Class**: GE_GrantRecruitedTag

#### 2.4) Configure Function Properties
   - 2.4.1) **Category**: `Father|References`
   - 2.4.2) **Description**: `Establishes bidirectional ownership references and grants Father.State.Recruited tag`

### **3) Create ClearOwnerReference Function**

#### 3.1) Add Function
   - 3.1.1) Click **+** button next to Functions
   - 3.1.2) Name: `ClearOwnerReference`
   - 3.1.3) Double-click to open function graph

#### 3.2) Implement Function Logic

##### 3.2.1) Validate OwnerPlayer
   - 3.2.1.1) From Entry execution pin:
   - 3.2.1.2) Drag **OwnerPlayer** variable into graph (GET)
   - 3.2.1.3) Add **Is Valid** node
   - 3.2.1.4) Connect OwnerPlayer to Input Object

##### 3.2.2) Cast to Player
   - 3.2.2.1) From Is Valid execution (Is Valid path):
   - 3.2.2.2) Add **Cast To BP_NarrativePlayer** node
   - 3.2.2.3) Connect OwnerPlayer to Object input

##### 3.2.3) Clear FatherCompanionRef on Player
   - 3.2.3.1) From Cast success:
   - 3.2.3.2) Add **Set FatherCompanionRef** node
   - 3.2.3.3) Target: Connect As BP_NarrativePlayer
   - 3.2.3.4) Value: Leave empty (clears to NULL)

##### 3.2.4) Remove Stat Boost Effects from Player
   - 3.2.4.1) From Set FatherCompanionRef execution:
   - 3.2.4.2) Get stored handles (ArmorBoostHandle, ExoskeletonSpeedHandle, SymbioteBoostHandle)
   - 3.2.4.3) For each valid handle: Add **Remove Active Gameplay Effect** node
   - 3.2.4.4) Target: Player's Ability System Component

##### 3.2.5) Clear OwnerPlayer
   - 3.2.5.1) After effect removal:
   - 3.2.5.2) Add **Set OwnerPlayer** node
   - 3.2.5.3) Value: Leave empty (clears to NULL)

##### 3.2.6) Reset Form State
   - 3.2.6.1) Add **Set CurrentForm** node
   - 3.2.6.2) Value: `Crawler`
   - 3.2.6.3) Add **Set IsAttached** node
   - 3.2.6.4) Value: Unchecked (false)

##### 3.2.7) Remove Recruited Tag
   - 3.2.7.1) From Set IsAttached execution:
   - 3.2.7.2) Get **Self** reference
   - 3.2.7.3) Add **Get Ability System Component** node (Target: Self)
   - 3.2.7.4) Add **Remove Loose Gameplay Tag** node
   - 3.2.7.5) **Tag to Remove**: Father.State.Recruited

#### 3.3) Configure Function Properties
   - 3.3.1) **Category**: `Father|References`
   - 3.3.2) **Description**: `Clears ownership references, removes player effects, and removes Father.State.Recruited`

---

## **PHASE 15: CREATE FUNCTIONS - AI CONTROL**

### **1) Create DisableTurretAI Function**

#### 1.1) Add Function
   - 1.1.1) Click **+** next to Functions
   - 1.1.2) Name: `DisableTurretAI`
   - 1.1.3) Double-click to open function graph

#### 1.2) Implement Function Logic

##### 1.2.1) Get AI Controller
   - 1.2.1.1) Right-click in graph
   - 1.2.1.2) Search: `Get Controller`
   - 1.2.1.3) Add **Get Controller** node

##### 1.2.2) Cast to AIController
   - 1.2.2.1) From Get Controller Return Value:
   - 1.2.2.2) Add **Cast To AIController** node
   - 1.2.2.3) Connect execution from Entry to Cast node

##### 1.2.3) Stop Behavior Tree
   - 1.2.3.1) From Cast success pin (As AI Controller):
   - 1.2.3.2) Add **Stop Logic** node
   - 1.2.3.3) Connect Cast success execution to Stop Logic
   - 1.2.3.4) **Reason**: `Turret Recall`

#### 1.3) Configure Function Properties
   - 1.3.1) **Category**: `Father|AI`
   - 1.3.2) **Description**: `Stops behavior tree when recalling turret`

---

## **PHASE 16: CREATE FUNCTIONS - ENEMY MARKING**

### **1) Create AddToMarkedEnemies Function**

#### 1.1) Add Function
   - 1.1.1) Click **+** next to Functions
   - 1.1.2) Name: `AddToMarkedEnemies`
   - 1.1.3) Double-click to open

#### 1.2) Configure Input Parameter
   - 1.2.1) In Details panel, **Inputs** section
   - 1.2.2) Click **+** to add parameter
   - 1.2.3) **Parameter Name**: `Enemy`
   - 1.2.4) **Parameter Type**: `Actor` -> **Object Reference**

#### 1.3) Implement Function Logic
   - 1.3.1) Drag **MarkedEnemies** variable into graph (GET)
   - 1.3.2) Add **Add Unique** node
   - 1.3.3) Connect MarkedEnemies to Target Array
   - 1.3.4) Connect Enemy parameter to Item to Add
   - 1.3.5) Connect execution from Entry to Add Unique

#### 1.4) Configure Function Properties
   - 1.4.1) **Category**: `Father|Combat`

### **2) Create RemoveFromMarkedEnemies Function**

#### 2.1) Add Function
   - 2.1.1) Click **+** next to Functions
   - 2.1.2) Name: `RemoveFromMarkedEnemies`
   - 2.1.3) Double-click to open

#### 2.2) Configure Input Parameter
   - 2.2.1) **Parameter Name**: `Enemy`
   - 2.2.2) **Parameter Type**: `Actor` -> **Object Reference**

#### 2.3) Implement Function Logic
   - 2.3.1) Add **Remove** node (Array operation)
   - 2.3.2) Drag **MarkedEnemies** into graph (GET)
   - 2.3.3) Connect MarkedEnemies to Target Array
   - 2.3.4) Connect Enemy parameter to Item to Remove
   - 2.3.5) Connect execution from Entry to Remove

#### 2.4) Configure Function Properties
   - 2.4.1) **Category**: `Father|Combat`

### **3) Create MarkEnemy Function**

#### 3.1) Add Function
   - 3.1.1) Click **+** next to Functions
   - 3.1.2) Name: `MarkEnemy`
   - 3.1.3) Double-click to open

#### 3.2) Configure Input Parameter
   - 3.2.1) **Parameter Name**: `EnemyToMark`
   - 3.2.2) **Parameter Type**: `Actor` -> **Object Reference**

#### 3.3) Implement Function Logic

##### 3.3.1) Validate Enemy
   - 3.3.1.1) From Entry execution pin:
   - 3.3.1.2) Add **Is Valid** node
   - 3.3.1.3) Connect EnemyToMark to Input Object

##### 3.3.2) Check If Already Marked
   - 3.3.2.1) Drag **MarkedEnemies** into graph (GET)
   - 3.3.2.2) Add **Contains** node
   - 3.3.2.3) Connect MarkedEnemies to Array pin
   - 3.3.2.4) Connect EnemyToMark to Item pin

##### 3.3.3) Add Combined Branch
   - 3.3.3.1) Add **Branch** node after Entry
   - 3.3.3.2) Create **AND Boolean** node
   - 3.3.3.3) Connect Is Valid return to AND input A
   - 3.3.3.4) Add **NOT Boolean** node
   - 3.3.3.5) Connect Contains return to NOT input
   - 3.3.3.6) Connect NOT return to AND input B
   - 3.3.3.7) Connect AND return to Branch Condition

##### 3.3.4) Call AddToMarkedEnemies
   - 3.3.4.1) From Branch True:
   - 3.3.4.2) Add **AddToMarkedEnemies** function call
   - 3.3.4.3) Enemy: Connect EnemyToMark

##### 3.3.5) Apply Mark Effect
   - 3.3.5.1) From AddToMarkedEnemies execution:
   - 3.3.5.2) From EnemyToMark:
   - 3.3.5.3) Add **Cast To NarrativeCharacter** node
   - 3.3.5.4) From Cast success:
   - 3.3.5.5) Add **Get Ability System Component** node
   - 3.3.5.6) Add **Apply Gameplay Effect to Self** node
   - 3.3.5.7) **Gameplay Effect Class**: `GE_MarkedByFather`
   - 3.3.5.8) **Level**: `1.0`

#### 3.4) Configure Function Properties
   - 3.4.1) **Category**: `Father|Combat`
   - 3.4.2) **Description**: `Marks an enemy for bonus damage`

---

## **PHASE 17: CREATE FUNCTIONS - MARK WIDGET MANAGEMENT**

### **1) Create StoreMarkWidget Function**

#### 1.1) Add Function
   - 1.1.1) Click **+** next to Functions
   - 1.1.2) Name: `StoreMarkWidget`
   - 1.1.3) Double-click to open

#### 1.2) Configure Input Parameters
   - 1.2.1) Add first parameter:
   - 1.2.1.1) **Name**: `Enemy`
   - 1.2.1.2) **Type**: `Actor` -> **Object Reference**
   - 1.2.2) Add second parameter:
   - 1.2.2.1) **Name**: `Widget`
   - 1.2.2.2) **Type**: `Widget Component` -> **Object Reference**

#### 1.3) Implement Function Logic
   - 1.3.1) From Entry execution:
   - 1.3.2) Drag **MarkWidgetMap** into graph (GET)
   - 1.3.3) Add **Add** node (Map operation)
   - 1.3.4) Connect MarkWidgetMap to Target
   - 1.3.5) Connect Enemy to Key
   - 1.3.6) Connect Widget to Value

#### 1.4) Configure Function Properties
   - 1.4.1) **Category**: `Father|Combat`
   - 1.4.2) **Description**: `Stores widget reference in map for cleanup`

### **2) Create DestroyMarkWidgetForEnemy Function**

#### 2.1) Add Function
   - 2.1.1) Click **+** next to Functions
   - 2.1.2) Name: `DestroyMarkWidgetForEnemy`
   - 2.1.3) Double-click to open

#### 2.2) Configure Input Parameter
   - 2.2.1) **Parameter Name**: `Enemy`
   - 2.2.2) **Parameter Type**: `Actor` -> **Object Reference**

#### 2.3) Implement Function Logic

##### 2.3.1) Find Widget in Map
   - 2.3.1.1) From Entry execution:
   - 2.3.1.2) Drag **MarkWidgetMap** into graph (GET)
   - 2.3.1.3) Add **Find** node (Map operation)
   - 2.3.1.4) Connect Enemy to Key

##### 2.3.2) Validate Widget Found
   - 2.3.2.1) From Find Return Value:
   - 2.3.2.2) Add **Is Valid** node
   - 2.3.2.3) Add **Branch** node
   - 2.3.2.4) Connect Is Valid return to Condition

##### 2.3.3) Destroy Widget
   - 2.3.3.1) From Branch True:
   - 2.3.3.2) Add **Destroy Component** node
   - 2.3.3.3) Target: Connect found widget

##### 2.3.4) Remove from Map
   - 2.3.4.1) From Destroy Component execution:
   - 2.3.4.2) Drag **MarkWidgetMap** into graph (GET)
   - 2.3.4.3) Add **Remove** node (Map operation)
   - 2.3.4.4) Connect Enemy to Key

#### 2.4) Configure Function Properties
   - 2.4.1) **Category**: `Father|Combat`
   - 2.4.2) **Description**: `Destroys mark widget and removes from map`

### **3) Create HandleMarkedEnemyDeath Function**

#### 3.1) Add Function
   - 3.1.1) Click **+** next to Functions
   - 3.1.2) Name: `HandleMarkedEnemyDeath`
   - 3.1.3) Double-click to open

#### 3.2) Configure Input Parameter
   - 3.2.1) **Parameter Name**: `Enemy`
   - 3.2.2) **Parameter Type**: `Actor` -> **Object Reference**

#### 3.3) Implement Function Logic
   - 3.3.1) From Entry execution:
   - 3.3.2) Add **DestroyMarkWidgetForEnemy** function call
   - 3.3.3) Enemy: Connect Enemy parameter
   - 3.3.4) From DestroyMarkWidgetForEnemy execution:
   - 3.3.5) Add **RemoveFromMarkedEnemies** function call
   - 3.3.6) Enemy: Connect Enemy parameter

#### 3.4) Configure Function Properties
   - 3.4.1) **Category**: `Father|Combat`
   - 3.4.2) **Description**: `Cleans up mark data when enemy dies`

---

## **PHASE 18: CREATE FUNCTIONS - ENDPLAY SAFETY NET**

### **1) Create CleanupCrossActorGrants Function**

#### 1.1) Add Function
   - 1.1.1) Click **+** next to Functions
   - 1.1.2) Name: `CleanupCrossActorGrants`
   - 1.1.3) Double-click to open function graph

#### 1.2) Implement Function Logic

##### 1.2.1) Validate OwnerPlayer
   - 1.2.1.1) From Entry execution pin:
   - 1.2.1.2) Drag **OwnerPlayer** variable into graph (GET)
   - 1.2.1.3) Add **Is Valid** node
   - 1.2.1.4) Connect OwnerPlayer to Input Object

##### 1.2.2) Add Branch
   - 1.2.2.1) Add **Branch** node
   - 1.2.2.2) Connect Is Valid return to Condition

##### 1.2.3) Get Player ASC
   - 1.2.3.1) From Branch True:
   - 1.2.3.2) From OwnerPlayer, drag outward
   - 1.2.3.3) Add **Get Ability System Component** node
   - 1.2.3.4) Promote **Return Value** to local variable: `PlayerASC`

##### 1.2.4) Validate Player ASC
   - 1.2.4.1) From Get Ability System Component execution:
   - 1.2.4.2) Add **Is Valid** node
   - 1.2.4.3) Connect PlayerASC to Input Object
   - 1.2.4.4) Add **Branch** node
   - 1.2.4.5) Connect Is Valid return to Condition

##### 1.2.5) Cleanup DomeAbilityHandle
   - 1.2.5.1) From Branch True:
   - 1.2.5.2) Drag **DomeAbilityHandle** variable into graph (GET)
   - 1.2.5.3) From DomeAbilityHandle, add **Is Valid** node
   - 1.2.5.4) Add **Branch** node
   - 1.2.5.5) From Branch True:
   - 1.2.5.6) Add **Cancel Ability** node (Target: PlayerASC, Handle: DomeAbilityHandle)
   - 1.2.5.7) Add **Set Remove Ability On End** node (Target: PlayerASC, Handle: DomeAbilityHandle)

##### 1.2.6) Cleanup DomeBurstAbilityHandle
   - 1.2.6.1) Repeat pattern from 1.2.5 for DomeBurstAbilityHandle

##### 1.2.7) Cleanup DashAbilityHandle
   - 1.2.7.1) Repeat pattern for DashAbilityHandle

##### 1.2.8) Cleanup SprintAbilityHandle
   - 1.2.8.1) Repeat pattern for SprintAbilityHandle

##### 1.2.9) Cleanup StealthAbilityHandle
   - 1.2.9.1) Repeat pattern for StealthAbilityHandle

##### 1.2.10) Cleanup ProximityAbilityHandle
   - 1.2.10.1) Repeat pattern for ProximityAbilityHandle

##### 1.2.11) Remove Player Effect Handles
   - 1.2.11.1) After all ability handle cleanup:
   - 1.2.11.2) Drag **ArmorBoostHandle** variable into graph (GET)
   - 1.2.11.3) Add **Is Valid (Active Gameplay Effect Handle)** node
   - 1.2.11.4) Add **Branch** node
   - 1.2.11.5) From Branch True:
   - 1.2.11.6) Add **Remove Active Gameplay Effect** node
   - 1.2.11.7) Target: Connect PlayerASC
   - 1.2.11.8) Handle: Connect ArmorBoostHandle

##### 1.2.12) Remove ExoskeletonSpeedHandle
   - 1.2.12.1) Repeat effect removal pattern for ExoskeletonSpeedHandle

##### 1.2.13) Remove SymbioteBoostHandle
   - 1.2.13.1) Repeat effect removal pattern for SymbioteBoostHandle

##### 1.2.14) Clear Player Reference on Player
   - 1.2.14.1) After all effect removal:
   - 1.2.14.2) From OwnerPlayer, add **Cast To BP_NarrativePlayer** node
   - 1.2.15.3) From Cast success:
   - 1.2.15.4) Add **Set FatherCompanionRef** node
   - 1.2.15.5) Target: As BP_NarrativePlayer
   - 1.2.15.6) Value: Leave empty (clears to NULL)

#### 1.3) Configure Function Properties
   - 1.3.1) **Category**: `Father|Cleanup`
   - 1.3.2) **Description**: `Emergency cleanup of all abilities and effects granted to player - called from EndPlay`

### **2) Cleanup Pattern Summary**

| Handle Type | Cleanup Method | Target ASC |
|-------------|---------------|------------|
| Gameplay Ability Spec Handle | Cancel Ability + Set Remove Ability On End | Player |
| Active Gameplay Effect Handle | Remove Active Gameplay Effect | Player |

### **3) All Handles Cleaned by CleanupCrossActorGrants**

| Ability Handles | Effect Handles |
|-----------------|----------------|
| DomeAbilityHandle | ArmorBoostHandle |
| DomeBurstAbilityHandle | ExoskeletonSpeedHandle |
| DashAbilityHandle | SymbioteBoostHandle |
| SprintAbilityHandle | |
| StealthAbilityHandle | |
| ProximityAbilityHandle | |

---

## **PHASE 19: IMPLEMENT BEGINPLAY**

### **1) Open Event Graph**

#### 1.1) Navigate to Event Graph
   - 1.1.1) In graph tabs at top, click **Event Graph**

### **2) Implement BeginPlay Logic**

#### 2.1) Event BeginPlay Already Exists
   - 2.1.1) Locate **Event BeginPlay** node (auto-created)

#### 2.2) Add Basic Initialization
   - 2.2.1) From Event BeginPlay execution:
   - 2.2.2) Add any visual initialization (mesh setup, VFX) as needed
   - 2.2.3) Ownership setup is handled by NarrativeEvents (NE_SetFatherOwner)

---

## **PHASE 20: IMPLEMENT PREPARE FOR SAVE**

### **1) Override PrepareForSave**

#### 1.1) Add Override
   - 1.1.1) In My Blueprint panel, find **Functions** section
   - 1.1.2) Click **Override** dropdown
   - 1.1.3) Find **PrepareForSave**
   - 1.1.4) Click to select
   - 1.1.5) Double-click to open function graph

### **2) Implement PrepareForSave Logic**

#### 2.1) Call Parent
   - 2.1.1) From Entry execution:
   - 2.1.2) Add **Parent: PrepareForSave** node

#### 2.2) Check OwnerPlayer Valid
   - 2.2.1) From Parent execution:
   - 2.2.2) Drag **OwnerPlayer** variable into graph (GET)
   - 2.2.3) Add **Is Valid** node
   - 2.2.4) Connect OwnerPlayer to Input Object
   - 2.2.5) Add **Branch** node
   - 2.2.6) Connect Is Valid return to Condition

#### 2.3) Store Character Definition
   - 2.3.1) From Branch True:
   - 2.3.2) Add **Get Character Definition** node
   - 2.3.3) Target: Connect OwnerPlayer
   - 2.3.4) From Return Value:
   - 2.3.5) Add **Set OwnerPlayerDefinition** node
   - 2.3.6) Connect Character Definition to value

---

## **PHASE 21: IMPLEMENT INITIALIZE**

### **1) Override Initialize**

#### 1.1) Add Override
   - 1.1.1) In My Blueprint panel, find **Functions** section
   - 1.1.2) Click **Override** dropdown
   - 1.1.3) Find **Initialize**
   - 1.1.4) Click to select
   - 1.1.5) Double-click to open function graph

### **2) Implement Initialize Logic**

#### 2.1) Call Parent
   - 2.1.1) From Entry execution:
   - 2.1.2) Add **Parent: Initialize** node

#### 2.2) Check OwnerPlayerDefinition Valid
   - 2.2.1) From Parent execution:
   - 2.2.2) Drag **OwnerPlayerDefinition** variable into graph (GET)
   - 2.2.3) Add **Is Valid** node
   - 2.2.4) Connect OwnerPlayerDefinition to Input Object
   - 2.2.5) Add **Branch** node
   - 2.2.6) Connect Is Valid return to Condition

#### 2.3) Find Character from Definition
   - 2.3.1) From Branch True:
   - 2.3.2) Add **Get Narrative Character Subsystem** node
   - 2.3.3) From Return Value:
   - 2.3.4) Add **Find Character** node
   - 2.3.5) Connect OwnerPlayerDefinition to Character Definition input

#### 2.4) Validate Found Character
   - 2.4.1) From Find Character Return Value:
   - 2.4.2) Add **Is Valid** node
   - 2.4.3) Add **Branch** node
   - 2.4.4) Connect Is Valid return to Condition

#### 2.5) Restore References
   - 2.5.1) From Branch True:
   - 2.5.2) Add **Set OwnerPlayer** node
   - 2.5.3) Connect Find Character result to value
   - 2.5.4) From execution:
   - 2.5.5) Add **Cast To BP_NarrativePlayer** node
   - 2.5.6) Connect Find Character result to Object input
   - 2.5.7) From Cast success:
   - 2.5.8) Add **Set FatherCompanionRef** node
   - 2.5.9) Target: Connect As BP_NarrativePlayer
   - 2.5.10) Value: Add **Self** reference

#### 2.6) Reapply Recruited Tag
   - 2.6.1) From Set FatherCompanionRef execution:
   - 2.6.2) Add **Apply Gameplay Effect to Self** node
   - 2.6.3) **Gameplay Effect Class**: GE_GrantRecruitedTag
   - 2.6.4) This restores Father.State.Recruited tag, enabling form abilities

---

## **PHASE 22: IMPLEMENT ONREP FUNCTIONS**

### **1) OnRep_CurrentForm**

#### 1.1) Open OnRep Function
   - 1.1.1) In My Blueprint panel, find **Functions** section
   - 1.1.2) Locate **OnRep_CurrentForm** (auto-generated)
   - 1.1.3) Double-click to open

#### 1.2) Add Visual Feedback
   - 1.2.1) Add custom logic for form change visual/audio feedback

### **2) OnRep_IsAttached**

#### 2.1) Open OnRep Function
   - 2.1.1) Double-click **OnRep_IsAttached**

#### 2.2) Add Visual Feedback
   - 2.2.1) Add custom logic for attachment state visual updates

---

## **PHASE 23: IMPLEMENT ENDPLAY**

### **1) Override Event EndPlay**

#### 1.1) Open Event Graph
   - 1.1.1) In BP_FatherCompanion, click **Event Graph** tab

#### 1.2) Add Event EndPlay Override
   - 1.2.1) Right-click in empty area of Event Graph
   - 1.2.2) Search: `Event End Play`
   - 1.2.3) Select **Add Event** -> **Event End Play**

### **2) Implement EndPlay Safety Net**

#### 2.1) Check Server Authority
   - 2.1.1) From Event EndPlay execution pin:
   - 2.1.2) Add **Has Authority** node
   - 2.1.3) Add **Branch** node
   - 2.1.4) Connect Has Authority return to Condition

#### 2.2) Call Cleanup Function
   - 2.2.1) From Branch True:
   - 2.2.2) Add **CleanupCrossActorGrants** function call
   - 2.2.3) Target: Self

#### 2.3) Remove Father State Effects
   - 2.3.1) From CleanupCrossActorGrants execution:
   - 2.3.2) Add **Get Ability System Component** node (Target: Self)
   - 2.3.3) Promote to local variable: `FatherASC`

#### 2.4) Remove CrawlerStateHandle
   - 2.4.1) Drag **CrawlerStateHandle** into graph (GET)
   - 2.4.2) Add **Is Valid (Active Gameplay Effect Handle)** node
   - 2.4.3) Add **Branch** node
   - 2.4.4) From Branch True:
   - 2.4.5) Add **Remove Active Gameplay Effect** node
   - 2.4.6) Target: FatherASC
   - 2.4.7) Handle: CrawlerStateHandle

#### 2.5) Remove Other State Handles
   - 2.5.1) Repeat pattern for ArmorStateHandle
   - 2.5.2) Repeat pattern for ExoskeletonStateHandle
   - 2.5.3) Repeat pattern for SymbioteStateHandle
   - 2.5.4) Repeat pattern for EngineerStateHandle
   - 2.5.5) Repeat pattern for FormCooldownHandle
   - 2.5.6) Repeat pattern for InvulnerableHandle

#### 2.6) Clear OwnerPlayer Reference
   - 2.6.1) After all effect removal:
   - 2.6.2) Add **Set OwnerPlayer** node
   - 2.6.3) Value: Leave empty (clears to NULL)

### **3) EndPlay Summary**

| Step | Action | Purpose |
|------|--------|---------|
| 1 | Has Authority check | Only server performs cleanup |
| 2 | CleanupCrossActorGrants | Removes player abilities and effects |
| 3 | Remove father state effects | Cleans up father ASC |
| 4 | Clear OwnerPlayer | Prevents dangling references |

---

## **PHASE 24: CONFIGURE SAVEGAME VARIABLES**

### **1) Configure CurrentForm SaveGame**

#### 1.1) Select Variable
   - 1.1.1) In My Blueprint panel, click **CurrentForm** variable

#### 1.2) Enable SaveGame
   - 1.2.1) In Details panel, expand **Advanced** section
   - 1.2.2) Find **SaveGame** property
   - 1.2.3) Check the checkbox: **Enabled**

### **2) Configure IsAttached SaveGame**

#### 2.1) Select Variable
   - 2.1.1) Click **IsAttached** variable

#### 2.2) Enable SaveGame
   - 2.2.1) In Details panel, expand **Advanced** section
   - 2.2.2) Check **SaveGame**: **Enabled**

### **3) Configure IsDeployed SaveGame**

#### 3.1) Select Variable
   - 3.1.1) Click **IsDeployed** variable

#### 3.2) Enable SaveGame
   - 3.2.1) In Details panel, expand **Advanced** section
   - 3.2.2) Check **SaveGame**: **Enabled**

### **4) Configure DeployedTransform SaveGame**

#### 4.1) Select Variable
   - 4.1.1) Click **DeployedTransform** variable

#### 4.2) Enable SaveGame
   - 4.2.1) In Details panel, expand **Advanced** section
   - 4.2.2) Check **SaveGame**: **Enabled**

### **5) Configure OwnerPlayerDefinition SaveGame**

#### 5.1) Select Variable
   - 5.1.1) Click **OwnerPlayerDefinition** variable

#### 5.2) Enable SaveGame
   - 5.2.1) In Details panel, expand **Advanced** section
   - 5.2.2) Check **SaveGame**: **Enabled**

### **6) Compile**
   - 6.1) Click **Compile** button
   - 6.2) Click **Save** button

---

## **PHASE 25: OVERRIDE LOAD FUNCTION**

### **1) Add Load Function Override**

#### 1.1) Open Functions Panel
   - 1.1.1) In My Blueprint panel, find **Functions** section
   - 1.1.2) Click **Override** dropdown

#### 1.2) Select Load Function
   - 1.2.1) In dropdown list, find **Load** (from INarrativeSavableActor)
   - 1.2.2) Click to select
   - 1.2.3) Function added to Functions list

#### 1.3) Open Load Function Graph
   - 1.3.1) Double-click **Load** function

### **2) Implement Load Logic**

#### 2.1) Call Parent Load
   - 2.1.1) **Event Load** node already exists
   - 2.1.2) Drag from execution pin
   - 2.1.3) Add **Parent: Load** node
   - 2.1.4) Connect execution

#### 2.2) Add Switch on E_FatherForm
   - 2.2.1) Drag from **Parent: Load** execution pin
   - 2.2.2) Search: `Switch on E_FatherForm`
   - 2.2.3) Add **Switch on E_FatherForm** node
   - 2.2.4) Selection: Drag **CurrentForm** variable (GET) and connect

#### 2.3) Handle Each Form Case

##### 2.3.1) Crawler Case
   - 2.3.1.1) From Crawler execution pin: Leave empty (bActivateAbilityOnGranted handles default)

##### 2.3.2) Armor Case
   - 2.3.2.1) From Armor execution pin:
   - 2.3.2.2) Add **Get Ability System Component** node
   - 2.3.2.3) Add **Try Activate Ability By Class** node
   - 2.3.2.4) **Ability Class**: `GA_FatherArmor`

##### 2.3.3) Exoskeleton Case
   - 2.3.3.1) From Exoskeleton execution pin:
   - 2.3.3.2) Add **Get Ability System Component** node
   - 2.3.3.3) Add **Try Activate Ability By Class** node
   - 2.3.3.4) **Ability Class**: `GA_FatherExoskeleton`

##### 2.3.4) Symbiote Case
   - 2.3.4.1) From Symbiote execution pin:
   - 2.3.4.2) Add **Get Ability System Component** node
   - 2.3.4.3) Add **Try Activate Ability By Class** node
   - 2.3.4.4) **Ability Class**: `GA_FatherSymbiote`

##### 2.3.5) Engineer Case
   - 2.3.5.1) From Engineer execution pin:
   - 2.3.5.2) Add **Get Ability System Component** node
   - 2.3.5.3) Add **Try Activate Ability By Class** node
   - 2.3.5.4) **Ability Class**: `GA_FatherEngineer`

##### 2.3.6) Offline Case
   - 2.3.6.1) From Offline execution pin:
   - 2.3.6.2) Handle sacrifice recovery state restoration

---

## **PHASE 26: CREATE PLAYER BLUEPRINT VARIABLES**

### **1) Open Player Blueprint**

#### 1.1) Navigate to Player Blueprint
   - 1.1.1) In Content Browser, locate your player blueprint
   - 1.1.2) Example: `BP_NarrativePlayer`
   - 1.1.3) Double-click to open

### **2) Create FatherCompanionRef Variable**

#### 2.1) Add Variable
   - 2.1.1) In My Blueprint panel, click **+** next to Variables
   - 2.1.2) Name: `FatherCompanionRef`

#### 2.2) Configure Variable
   - 2.2.1) **Variable Type**: `BP_FatherCompanion` -> **Object Reference**
   - 2.2.2) **Instance Editable**: Uncheck
   - 2.2.3) **Replication**: Select **Replicated**
   - 2.2.4) **Category**: `Father|References`
   - 2.2.5) **Private**: Uncheck (abilities need access)
   - 2.2.6) **Tooltip**: `Reference to player's father companion`

### **3) Create GetFatherCompanion Function**

#### 3.1) Add Function
   - 3.1.1) Click **+** next to Functions
   - 3.1.2) Name: `GetFatherCompanion`

#### 3.2) Configure Return Value
   - 3.2.1) In Details panel, **Outputs** section
   - 3.2.2) Add parameter
   - 3.2.3) **Name**: `Father`
   - 3.2.4) **Type**: `BP_FatherCompanion` -> **Object Reference**

#### 3.3) Implement Function
   - 3.3.1) Drag **FatherCompanionRef** variable (GET) into graph
   - 3.3.2) Connect to Return Node -> Father pin

#### 3.4) Configure Function
   - 3.4.1) **Pure**: Check
   - 3.4.2) **Category**: `Father|References`

### **4) Compile and Save Player Blueprint**
   - 4.1) Click **Compile** button
   - 4.2) Click **Save** button

---

## **PHASE 27: CREATE PLAYER BLUEPRINT COMPONENTS**

### **1) Add SymbioteChargeComponent**

#### 1.1) Open Player Blueprint
   - 1.1.1) If not already open, open player blueprint

#### 1.2) Add Component
   - 1.2.1) In Components panel, click **Add** button
   - 1.2.2) Search: `Symbiote Charge Component`
   - 1.2.3) Select **SymbioteChargeComponent**
   - 1.2.4) Component added to hierarchy

#### 1.3) Configure Component Properties
   - 1.3.1) Select SymbioteChargeComponent in hierarchy
   - 1.3.2) In Details panel:
   - 1.3.2.1) **Charge Threshold**: `5000.0`
   - 1.3.2.2) **Father Damage Efficiency**: `0.5`
   - 1.3.2.3) **Cooldown Duration**: `120.0`

### **2) Add Attribute Sets to Player ASC**

#### 2.1) Select Ability System Component
   - 2.1.1) In Components panel, find **NarrativeAbilitySystemComponent**
   - 2.1.2) Click to select

#### 2.2) Add AS_DomeAttributes
   - 2.2.1) In Details panel, find **Granted Attributes** array
   - 2.2.2) Click **+** button to add element
   - 2.2.3) Click dropdown on new element
   - 2.2.4) Search: `AS_DomeAttributes`
   - 2.2.5) Select **AS_DomeAttributes**

### **3) Compile and Save**
   - 3.1) Click **Compile** button
   - 3.2) Click **Save** button

---

## **PHASE 28: CREATE PLAYER SKELETON SOCKETS**

### **1) Open Player Skeletal Mesh**

#### 1.1) Locate Skeletal Mesh
   - 1.1.1) In Content Browser, navigate to player character folder
   - 1.1.2) Find skeletal mesh asset (e.g., `SK_PlayerBody`)
   - 1.1.3) Right-click on skeletal mesh
   - 1.1.4) Select **Edit Skeleton**

### **2) Create FatherChestSocket**

#### 2.1) Locate Spine Bone
   - 2.1.1) In Skeleton Tree panel, search: `spine`
   - 2.1.2) Locate `spine_03` bone (upper chest area)
   - 2.1.3) Right-click on bone

#### 2.2) Add Socket
   - 2.2.1) Select **Add Socket**
   - 2.2.2) Socket created as child of bone

#### 2.3) Configure Socket
   - 2.3.1) Right-click new socket
   - 2.3.2) Select **Rename Socket**
   - 2.3.3) Name: `FatherChestSocket`
   - 2.3.4) Press **Enter**

#### 2.4) Position Socket
   - 2.4.1) Select socket in tree
   - 2.4.2) In Details panel, find **Transform** section
   - 2.4.3) **Relative Location**: X=15.0, Y=0.0, Z=0.0
   - 2.4.4) **Relative Rotation**: Roll=0.0, Pitch=0.0, Yaw=0.0

### **3) Create FatherBackSocket**

#### 3.1) Add Second Socket
   - 3.1.1) Right-click on `spine_03` bone
   - 3.1.2) Select **Add Socket**

#### 3.2) Configure Socket
   - 3.2.1) Rename to: `FatherBackSocket`

#### 3.3) Position Socket
   - 3.3.1) **Relative Location**: X=-25.0, Y=0.0, Z=10.0
   - 3.3.2) **Relative Rotation**: Roll=0.0, Pitch=0.0, Yaw=180.0

### **Socket Notes for Forms**

| Form | Socket Required | Notes |
|------|-----------------|-------|
| Crawler | None | Father follows player autonomously |
| Armor | FatherChestSocket | Attached to chest |
| Exoskeleton | FatherBackSocket | Attached to back |
| Symbiote | None | Full body merge - visual effect only |
| Engineer | None | Father deploys as separate turret actor |

### **4) Save Skeleton**
   - 4.1) Click **Save** button
   - 4.2) Close Skeleton Editor

---

## **PHASE 29: CREATE AC_FATHERCOMPANION_DEFAULT**

### **1) Create Data Asset**

#### 1.1) Navigate to Folder
   - 1.1.1) In Content Browser: Navigate to `Content/FatherCompanion/Configurations/`
   - 1.1.2) If folder does not exist: Create folder structure
   - 1.1.3) Right-click: In empty space
   - 1.1.4) Hover: **Miscellaneous**
   - 1.1.5) Click: **Data Asset**

#### 1.2) Select Class
   - 1.2.1) In dialog: Search `AbilityConfiguration`
   - 1.2.2) Select: **AbilityConfiguration**
   - 1.2.3) Click: **Select**

#### 1.3) Name Asset
   - 1.3.1) Type: `AC_FatherCompanion_Default`
   - 1.3.2) Press: **Enter**

### **2) Configure Baseline Abilities Array**

#### 2.1) Open Configuration
   - 2.1.1) Double-click: `AC_FatherCompanion_Default`

#### 2.2) Add Form-Changing Abilities

##### 2.2.1) Add GA_FatherCrawler
   - 2.2.1.1) Find: **Default Abilities** array
   - 2.2.1.2) Click: **+** button
   - 2.2.1.3) Element [0] appears
   - 2.2.1.4) Click: Dropdown on Element [0]
   - 2.2.1.5) Search: `GA_FatherCrawler`
   - 2.2.1.6) Select: **GA_FatherCrawler**

##### 2.2.2) Add GA_FatherArmor
   - 2.2.2.1) Click: **+** button
   - 2.2.2.2) Element [1] appears
   - 2.2.2.3) Dropdown: Select **GA_FatherArmor**

##### 2.2.3) Add GA_FatherExoskeleton
   - 2.2.3.1) Click: **+** button
   - 2.2.3.2) Element [2] appears
   - 2.2.3.3) Dropdown: Select **GA_FatherExoskeleton**

##### 2.2.4) Add GA_FatherSymbiote
   - 2.2.4.1) Click: **+** button
   - 2.2.4.2) Element [3] appears
   - 2.2.4.3) Dropdown: Select **GA_FatherSymbiote**

##### 2.2.5) Add GA_FatherEngineer
   - 2.2.5.1) Click: **+** button
   - 2.2.5.2) Element [4] appears
   - 2.2.5.3) Dropdown: Select **GA_FatherEngineer**

#### 2.3) Add General/Support Abilities

##### 2.3.1) Add GA_FatherAttack
   - 2.3.1.1) Click: **+** button
   - 2.3.1.2) Element [5] appears
   - 2.3.1.3) Dropdown: Select **GA_FatherAttack**

##### 2.3.2) Add GA_FatherLaserShot
   - 2.3.2.1) Click: **+** button
   - 2.3.2.2) Element [6] appears
   - 2.3.2.3) Dropdown: Select **GA_FatherLaserShot**

##### 2.3.3) Add GA_FatherMark
   - 2.3.3.1) Click: **+** button
   - 2.3.3.2) Element [7] appears
   - 2.3.3.3) Dropdown: Select **GA_FatherMark**

##### 2.3.4) Add GA_FatherSacrifice
   - 2.3.4.1) Click: **+** button
   - 2.3.4.2) Element [8] appears
   - 2.3.4.3) Dropdown: Select **GA_FatherSacrifice**

#### 2.4) Verify Configuration
   - 2.4.1) **Default Abilities** array should contain 9 abilities:

| Index | Ability |
|-------|---------|
| [0] | GA_FatherCrawler |
| [1] | GA_FatherArmor |
| [2] | GA_FatherExoskeleton |
| [3] | GA_FatherSymbiote |
| [4] | GA_FatherEngineer |
| [5] | GA_FatherAttack |
| [6] | GA_FatherLaserShot |
| [7] | GA_FatherMark |
| [8] | GA_FatherSacrifice |

### **3) Save Configuration**
   - 3.1) Click: **Save** button
   - 3.2) Close: Data Asset Editor

---

## **PHASE 30: CREATE FATHER FORM EQUIPMENT MODIFIER GEs**

This phase creates child blueprints of GE_EquipmentModifier for each father form. Child GEs allow form-specific modifiers and future expandability. Form state tags are granted via GA Activation Owned Tags in form abilities, not via GE Components.

### **1) Create GE_EquipmentModifier_FatherArmor**

#### 1.1) Navigate to GAS Folder
   - 1.1.1) In Content Browser: Navigate to `Content/FatherCompanion/GAS/`
   - 1.1.2) Right-click: In empty space
   - 1.1.3) Click: **Blueprint Class**

#### 1.2) Select Parent Class
   - 1.2.1) Click: **All Classes** dropdown
   - 1.2.2) Search: `GE_EquipmentModifier`
   - 1.2.3) Select: **GE_EquipmentModifier**
   - 1.2.4) Click: **Select** button

#### 1.3) Name Blueprint
   - 1.3.1) Type: `GE_EquipmentModifier_FatherArmor`
   - 1.3.2) Press: **Enter**

#### 1.4) Configure Child GE
   - 1.4.1) Double-click: `GE_EquipmentModifier_FatherArmor`
   - 1.4.2) Click: **Class Defaults**
   - 1.4.3) Verify: Components array is **0 elements** (inherited, do not add)
   - 1.4.4) Verify: Modifiers array shows 3 inherited elements (Armor, AttackRating, StealthRating)

#### 1.5) Save
   - 1.5.1) Click: **Compile**
   - 1.5.2) Click: **Save**

### **2) Create GE_EquipmentModifier_FatherExoskeleton**

#### 2.1) Create Blueprint
   - 2.1.1) Right-click in GAS folder
   - 2.1.2) Click: **Blueprint Class**
   - 2.1.3) Parent: **GE_EquipmentModifier**
   - 2.1.4) Name: `GE_EquipmentModifier_FatherExoskeleton`

#### 2.2) Configure Child GE
   - 2.2.1) Open blueprint, click **Class Defaults**
   - 2.2.2) Verify: Components array is **0 elements** (inherited, do not add)
   - 2.2.3) Verify: Modifiers array shows 3 inherited elements
   - 2.2.4) Compile and Save

### **3) Create GE_EquipmentModifier_FatherSymbiote**

#### 3.1) Create Blueprint
   - 3.1.1) Right-click in GAS folder
   - 3.1.2) Click: **Blueprint Class**
   - 3.1.3) Parent: **GE_EquipmentModifier**
   - 3.1.4) Name: `GE_EquipmentModifier_FatherSymbiote`

#### 3.2) Configure Child GE
   - 3.2.1) Open blueprint, click **Class Defaults**
   - 3.2.2) Verify: Components array is **0 elements** (inherited, do not add)

#### 3.3) Add Stamina Modifier (Symbiote-Specific)
   - 3.3.1) Find: **Modifiers** array (inherited, shows 3 elements)
   - 3.3.2) Click: **+** button to add Index [3]
   - 3.3.3) Expand Index [3]

| Property | Value |
|----------|-------|
| Attribute | NarrativeAttributeSetBase.StaminaRegenRate |
| Modifier Op | Override |
| Magnitude Calculation Type | Scalable Float |
| Scalable Float Magnitude | 10000.0 |

   - 3.3.4) Compile and Save

### **4) Create GE_EquipmentModifier_FatherEngineer**

#### 4.1) Create Blueprint
   - 4.1.1) Right-click in GAS folder
   - 4.1.2) Click: **Blueprint Class**
   - 4.1.3) Parent: **GE_EquipmentModifier**
   - 4.1.4) Name: `GE_EquipmentModifier_FatherEngineer`

#### 4.2) Configure Child GE
   - 4.2.1) Open blueprint, click **Class Defaults**
   - 4.2.2) Verify: Components array is **0 elements** (inherited, do not add)
   - 4.2.3) Verify: Modifiers array shows 3 inherited elements
   - 4.2.4) Compile and Save

### **5) Verify All Child GEs Created**

| Child GE | Parent | Components | Extra Modifiers |
|----------|--------|------------|-----------------|
| GE_EquipmentModifier_FatherArmor | GE_EquipmentModifier | 0 elements | None |
| GE_EquipmentModifier_FatherExoskeleton | GE_EquipmentModifier | 0 elements | None |
| GE_EquipmentModifier_FatherSymbiote | GE_EquipmentModifier | 0 elements | StaminaRegenRate Override 10000 |
| GE_EquipmentModifier_FatherEngineer | GE_EquipmentModifier | 0 elements | None |

Note: Form tags (Father.Form.Armor, etc.) are granted via GA Activation Owned Tags in form abilities, not via GE Components.

---

## **PHASE 31: CREATE ARMOR FORM EQUIPPABLEITEM**

### **1) Create Blueprint Class**

#### 1.1) Navigate to Items Folder
   - 1.1.1) In Content Browser: Navigate to `Content/FatherCompanion/Items/`
   - 1.1.2) If folder does not exist: Create folder structure
   - 1.1.3) Right-click: In empty space
   - 1.1.4) Click: **Blueprint Class**

#### 1.2) Select Parent Class
   - 1.2.1) In Pick Parent Class dialog: Search `EquippableItem`
   - 1.2.2) Select: **EquippableItem** (NOT EquippableItem_Clothing)
   - 1.2.3) Click: **Select** button

#### 1.3) Name Blueprint
   - 1.3.1) Type: `BP_FatherArmorForm`
   - 1.3.2) Press: **Enter**

### **2) Configure Armor Form Properties**

#### 2.1) Open Blueprint
   - 2.1.1) Double-click: `BP_FatherArmorForm`

#### 2.2) Open Class Defaults
   - 2.2.1) In toolbar: Click **Class Defaults** button

#### 2.3) Configure Item Properties
   - 2.3.1) In Details: Find **Item** category
   - 2.3.2) Expand: **Item** category

##### 2.3.1.a) Display Name
   - 2.3.1.a.1) Locate: **Display Name** property
   - 2.3.1.a.2) Enter: `Father Armor Form`

##### 2.3.1.b) Description
   - 2.3.1.b.1) Locate: **Description** property
   - 2.3.1.b.2) Enter: `Transform father into protective chest armor. Greatly increases defense and grants protective dome ability.`

##### 2.3.1.c) Use Action Text
   - 2.3.1.c.1) Locate: **Use Action Text** property
   - 2.3.1.c.2) Enter: `Equip Armor Form`

##### 2.3.1.d) Max Stack Size
   - 2.3.1.d.1) Locate: **Max Stack Size** property
   - 2.3.1.d.2) Enter: `1`

##### 2.3.1.e) Weight
   - 2.3.1.e.1) Locate: **Weight** property
   - 2.3.1.e.2) Enter: `0.0`

#### 2.4) Configure Equippable Properties
   - 2.4.1) In Details: Find **Item - Equippable** category
   - 2.4.2) Expand: **Item - Equippable** category

##### 2.4.1.a) Equippable Slot
   - 2.4.1.a.1) Locate: **Equippable Slot** property
   - 2.4.1.a.2) Click: Dropdown button
   - 2.4.1.a.3) Search: `FatherForm`
   - 2.4.1.a.4) Select: `Narrative.Equipment.Slot.FatherForm`

##### 2.4.1.b) Equipment Mod GE
   - 2.4.1.b.1) Locate: **Equipment Mod GE** property
   - 2.4.1.b.2) Click: Dropdown button
   - 2.4.1.b.3) Search: `GE_EquipmentModifier_FatherArmor`
   - 2.4.1.b.4) Select: **GE_EquipmentModifier_FatherArmor**

#### 2.5) Configure Stat Bonuses
   - 2.5.1) Still in **Item - Equippable** category

##### 2.5.1.a) Attack Rating
   - 2.5.1.a.1) Locate: **Attack Rating** property
   - 2.5.1.a.2) Enter: `0.0`

##### 2.5.1.b) Armor Rating
   - 2.5.1.b.1) Locate: **Armor Rating** property
   - 2.5.1.b.2) Enter: `50.0`

##### 2.5.1.c) Stealth Rating
   - 2.5.1.c.1) Locate: **Stealth Rating** property
   - 2.5.1.c.2) Enter: `0.0`

#### 2.6) Configure Abilities Array
   - 2.6.1) In Details: Find **Abilities** property
   - 2.6.2) Click: Arrow to expand **Abilities** array

##### 2.6.1.a) Add GA_ProtectiveDome
   - 2.6.1.a.1) Click: **+** button next to **Abilities**
   - 2.6.1.a.2) Element [0] appears
   - 2.6.1.a.3) Click: Dropdown on Element [0]
   - 2.6.1.a.4) Search: `GA_ProtectiveDome`
   - 2.6.1.a.5) Select: **GA_ProtectiveDome**

##### 2.6.1.b) Add GA_DomeBurst
   - 2.6.1.b.1) Click: **+** button next to **Abilities**
   - 2.6.1.b.2) Element [1] appears
   - 2.6.1.b.3) Click: Dropdown on Element [1]
   - 2.6.1.b.4) Search: `GA_DomeBurst`
   - 2.6.1.b.5) Select: **GA_DomeBurst**

#### 2.7) Verify Abilities Configuration
   - 2.7.1) **Abilities** array should show:

| Element | Ability |
|---------|---------|
| [0] | GA_ProtectiveDome |
| [1] | GA_DomeBurst |

### **3) Implement Custom Attachment Logic**

#### 3.1) Switch to Event Graph
   - 3.1.1) In Blueprint Editor: Click **Event Graph** tab

#### 3.2) Override HandleEquip Event
   - 3.2.1) Right-click: In graph
   - 3.2.2) Search: `HandleEquip`
   - 3.2.3) Select: **Event HandleEquip**

#### 3.3) Call Parent HandleEquip First
   - 3.3.1) From **Event HandleEquip** execution pin:
   - 3.3.1.a) Drag from execution pin
   - 3.3.1.b) Search: `Parent: HandleEquip`
   - 3.3.1.c) Select: **Parent: HandleEquip**
   - 3.3.2) Parent grants abilities from Abilities array automatically

#### 3.4) Get Player Character
   - 3.4.1) From **Parent: HandleEquip** execution pin:
   - 3.4.1.a) Drag from execution pin
   - 3.4.1.b) Search: `Get Owning Narrative Character`
   - 3.4.1.c) Select: **Get Owning Narrative Character**

#### 3.5) Get Father Reference
   - 3.5.1) From **Get Owning Narrative Character** return value:
   - 3.5.1.a) Drag from return pin
   - 3.5.1.b) Search: `Get Father Companion`
   - 3.5.1.c) Select: **Get Father Companion**

#### 3.6) Validate Father Reference
   - 3.6.1) From **Get Father Companion** return value:
   - 3.6.1.a) Drag from pin
   - 3.6.1.b) Search: `Is Valid`
   - 3.6.1.c) Select: **Is Valid**

#### 3.7) Add Branch Node
   - 3.7.1) From **Parent: HandleEquip** execution pin:
   - 3.7.1.a) Add **Branch** node
   - 3.7.1.b) Connect **Is Valid** return to **Condition** input

#### 3.8) Get Father's Ability System Component
   - 3.8.1) From **Branch** -> **True** execution pin:
   - 3.8.1.a) Drag from execution pin
   - 3.8.2) From **Get Father Companion** return value:
   - 3.8.2.a) Drag from pin
   - 3.8.2.b) Search: `Get Ability System Component`
   - 3.8.2.c) Select: **Get Ability System Component**

#### 3.9) Activate Father Armor Form Ability
   - 3.9.1) From **Get Ability System Component** return value:
   - 3.9.1.a) Drag from pin
   - 3.9.1.b) Search: `Try Activate Ability By Class`
   - 3.9.1.c) Select: **Try Activate Ability By Class**

##### 3.9.1.a) Configure Activation Node Inputs
   - 3.9.1.a.1) **Target:** Connect from **Get Ability System Component** return value
   - 3.9.1.a.2) **Ability To Activate:** Click dropdown
   - 3.9.1.a.3) Search: `GA_FatherArmor`
   - 3.9.1.a.4) Select: **GA_FatherArmor**

#### 3.10) Override HandleUnequip Event
   - 3.10.1) Right-click: In empty graph space
   - 3.10.2) Search: `HandleUnequip`
   - 3.10.3) Select: **Event HandleUnequip**

#### 3.11) Activate Crawler Form on Unequip
   - 3.11.1) From **Event HandleUnequip** execution pin:
   - 3.11.1.a) Get father reference (same pattern as HandleEquip)
   - 3.11.1.b) Get ASC from father
   - 3.11.1.c) Add: **Try Activate Ability By Class**
   - 3.11.1.d) **Ability To Activate:** `GA_FatherCrawler`

#### 3.12) Call Parent HandleUnequip
   - 3.12.1) From **Try Activate Ability By Class** execution pin:
   - 3.12.1.a) Drag from execution pin
   - 3.12.1.b) Search: `Parent: HandleUnequip`
   - 3.12.1.c) Select: **Parent: HandleUnequip**

### **4) Compile and Save**
   - 4.1) Click: **Compile** button in toolbar
   - 4.2) Click: **Save** button
   - 4.3) Close: Blueprint Editor

---

## **PHASE 32: CREATE EXOSKELETON FORM EQUIPPABLEITEM**

### **1) Create Blueprint Class**

#### 1.1) Navigate to Items Folder
   - 1.1.1) In Content Browser: Navigate to `Content/FatherCompanion/Items/`
   - 1.1.2) Right-click: In empty space
   - 1.1.3) Click: **Blueprint Class**

#### 1.2) Select Parent Class
   - 1.2.1) Search: `EquippableItem`
   - 1.2.2) Select: **EquippableItem** (NOT EquippableItem_Clothing)
   - 1.2.3) Click: **Select**

#### 1.3) Name Blueprint
   - 1.3.1) Type: `BP_FatherExoskeletonForm`
   - 1.3.2) Press: **Enter**

### **2) Configure Exoskeleton Form Properties**

#### 2.1) Open Blueprint and Class Defaults
   - 2.1.1) Double-click: `BP_FatherExoskeletonForm`
   - 2.1.2) Click: **Class Defaults** button

#### 2.2) Configure Item Properties
   - 2.2.1) **Display Name:** `Father Exoskeleton Form`
   - 2.2.2) **Description:** `Transform father into back exoskeleton. Grants enhanced speed, dash ability, sprint boost, and stealth field.`
   - 2.2.3) **Use Action Text:** `Equip Exoskeleton Form`
   - 2.2.4) **Max Stack Size:** `1`
   - 2.2.5) **Weight:** `0.0`

#### 2.3) Configure Equippable Properties
   - 2.3.1) **Equippable Slot:** `Narrative.Equipment.Slot.FatherForm`
   - 2.3.2) **Equipment Mod GE:** `GE_EquipmentModifier_FatherExoskeleton`

#### 2.4) Configure Stat Bonuses
   - 2.4.1) **Attack Rating:** `10.0`
   - 2.4.2) **Armor Rating:** `0.0`
   - 2.4.3) **Stealth Rating:** `0.0`

#### 2.5) Configure Abilities Array
   - 2.5.1) Click: **+** button three times

| Element | Ability |
|---------|---------|
| [0] | GA_ExoskeletonDash |
| [1] | GA_ExoskeletonSprint |
| [2] | GA_StealthField |

### **3) Implement Custom Attachment Logic**

#### 3.1) Event Graph - HandleEquip
   - 3.1.1) Add: **Event HandleEquip**
   - 3.1.2) Call: **Parent: HandleEquip** (grants abilities)
   - 3.1.3) Get player -> Get father -> Validate -> Get ASC
   - 3.1.4) Activate: **GA_FatherExoskeleton** ability

#### 3.2) Event Graph - HandleUnequip
   - 3.2.1) Add: **Event HandleUnequip**
   - 3.2.2) Get father -> Validate -> Get ASC
   - 3.2.3) Activate: **GA_FatherCrawler** ability
   - 3.2.4) Call: **Parent: HandleUnequip** (removes abilities)

### **4) Compile and Save**
   - 4.1) Click: **Compile**
   - 4.2) Click: **Save**
   - 4.3) Close: Blueprint Editor

---

## **PHASE 33: CREATE SYMBIOTE FORM EQUIPPABLEITEM**

### **1) Create Blueprint Class**

#### 1.1) Navigate to Items Folder
   - 1.1.1) In Content Browser: Navigate to `Content/FatherCompanion/Items/`
   - 1.1.2) Right-click: In empty space
   - 1.1.3) Click: **Blueprint Class**

#### 1.2) Select Parent Class
   - 1.2.1) Search: `EquippableItem`
   - 1.2.2) Select: **EquippableItem** (NOT EquippableItem_Clothing)
   - 1.2.3) Click: **Select**

#### 1.3) Name Blueprint
   - 1.3.1) Type: `BP_FatherSymbioteForm`
   - 1.3.2) Press: **Enter**

### **2) Configure Symbiote Form Properties**

#### 2.1) Open Blueprint and Class Defaults
   - 2.1.1) Double-click: `BP_FatherSymbioteForm`
   - 2.1.2) Click: **Class Defaults** button

#### 2.2) Configure Item Properties
   - 2.2.1) **Display Name:** `Father Symbiote Form`
   - 2.2.2) **Description:** `Merge with father for ultimate power. Requires full charge. Grants massive damage boost and proximity aura. Limited duration.`
   - 2.2.3) **Use Action Text:** `Activate Symbiote`
   - 2.2.4) **Max Stack Size:** `1`
   - 2.2.5) **Weight:** `0.0`

#### 2.3) Configure Equippable Properties
   - 2.3.1) **Equippable Slot:** `Narrative.Equipment.Slot.FatherForm`
   - 2.3.2) **Equipment Mod GE:** `GE_EquipmentModifier_FatherSymbiote`

#### 2.4) Configure Stat Bonuses
   - 2.4.1) **Attack Rating:** `100.0`
   - 2.4.2) **Armor Rating:** `0.0`
   - 2.4.3) **Stealth Rating:** `0.0`

#### 2.5) Configure Abilities Array
   - 2.5.1) Click: **+** button once

| Element | Ability |
|---------|---------|
| [0] | GA_ProximityStrike |

### **3) Implement Custom Attachment Logic**

#### 3.1) Event Graph - HandleEquip
   - 3.1.1) Add: **Event HandleEquip**
   - 3.1.2) Call: **Parent: HandleEquip** (grants abilities)
   - 3.1.3) Get player -> Get father -> Validate -> Get ASC
   - 3.1.4) Activate: **GA_FatherSymbiote** ability

#### 3.2) Event Graph - HandleUnequip
   - 3.2.1) Add: **Event HandleUnequip**
   - 3.2.2) Get father -> Validate -> Get ASC
   - 3.2.3) Activate: **GA_FatherCrawler** ability
   - 3.2.4) Call: **Parent: HandleUnequip** (removes abilities)

### **4) Compile and Save**
   - 4.1) Click: **Compile**
   - 4.2) Click: **Save**
   - 4.3) Close: Blueprint Editor

---

## **PHASE 34: CREATE ENGINEER FORM EQUIPPABLEITEM**

### **1) Create Blueprint Class**

#### 1.1) Navigate to Items Folder
   - 1.1.1) In Content Browser: Navigate to `Content/FatherCompanion/Items/`
   - 1.1.2) Right-click: In empty space
   - 1.1.3) Click: **Blueprint Class**

#### 1.2) Select Parent Class
   - 1.2.1) Search: `EquippableItem`
   - 1.2.2) Select: **EquippableItem** (NOT EquippableItem_Clothing)
   - 1.2.3) Click: **Select**

#### 1.3) Name Blueprint
   - 1.3.1) Type: `BP_FatherEngineerForm`
   - 1.3.2) Press: **Enter**

### **2) Configure Engineer Form Properties**

#### 2.1) Open Blueprint and Class Defaults
   - 2.1.1) Double-click: `BP_FatherEngineerForm`
   - 2.1.2) Click: **Class Defaults** button

#### 2.2) Configure Item Properties
   - 2.2.1) **Display Name:** `Father Engineer Form`
   - 2.2.2) **Description:** `Deploy father as stationary turret. Provides autonomous defense with laser attacks and electric traps. Use recall to retrieve.`
   - 2.2.3) **Use Action Text:** `Deploy Turret`
   - 2.2.4) **Max Stack Size:** `1`
   - 2.2.5) **Weight:** `0.0`

#### 2.3) Configure Equippable Properties
   - 2.3.1) **Equippable Slot:** `Narrative.Equipment.Slot.FatherForm`
   - 2.3.2) **Equipment Mod GE:** `GE_EquipmentModifier_FatherEngineer`

#### 2.4) Configure Stat Bonuses
   - 2.4.1) **Attack Rating:** `0.0`
   - 2.4.2) **Armor Rating:** `0.0`
   - 2.4.3) **Stealth Rating:** `0.0`

#### 2.5) Configure Abilities Array
   - 2.5.1) Click: **+** button two times

| Element | Ability |
|---------|---------|
| [0] | GA_TurretShoot |
| [1] | GA_FatherElectricTrap |

### **3) Implement Custom Deployment Logic**

#### 3.1) Event Graph - HandleEquip
   - 3.1.1) Add: **Event HandleEquip**
   - 3.1.2) Call: **Parent: HandleEquip** (grants abilities)
   - 3.1.3) Get player -> Get father -> Validate -> Get ASC
   - 3.1.4) Activate: **GA_FatherEngineer** ability

#### 3.2) Event Graph - HandleUnequip
   - 3.2.1) Add: **Event HandleUnequip**
   - 3.2.2) Get father -> Validate -> Get ASC
   - 3.2.3) Activate: **GA_FatherCrawler** ability
   - 3.2.4) Call: **Parent: HandleUnequip** (removes abilities)

### **4) Compile and Save**
   - 4.1) Click: **Compile**
   - 4.2) Click: **Save**
   - 4.3) Close: Blueprint Editor

---

## **PHASE 35: CREATE IC_FATHERFORMS (OPTIONAL)**

### **1) Understanding Item Collections**

| Use Case | Recommended Approach |
|----------|---------------------|
| Forms unlocked together as set | Use Item Collection |
| Forms unlocked individually | Add items directly to NPCDefinition |

### **2) Create Item Collection**

#### 2.1) Create Data Asset
   - 2.1.1) Right-click: In `Content/FatherCompanion/Items/`
   - 2.1.2) Select: **Miscellaneous** -> **Data Asset**
   - 2.1.3) Select: **ItemCollection**
   - 2.1.4) Name: `IC_FatherForms`

#### 2.2) Configure Collection
   - 2.2.1) Open: `IC_FatherForms`

##### 2.2.1.a) Add Armor Form
   - 2.2.1.a.1) **Items** array: Click **+**
   - 2.2.1.a.2) **Item:** `BP_FatherArmorForm`
   - 2.2.1.a.3) **Quantity:** `1`
   - 2.2.1.a.4) **Probability:** `1.0`

##### 2.2.1.b) Add Exoskeleton Form
   - 2.2.1.b.1) **Items** array: Click **+**
   - 2.2.1.b.2) **Item:** `BP_FatherExoskeletonForm`
   - 2.2.1.b.3) **Quantity:** `1`
   - 2.2.1.b.4) **Probability:** `1.0`

##### 2.2.1.c) Add Symbiote Form
   - 2.2.1.c.1) **Items** array: Click **+**
   - 2.2.1.c.2) **Item:** `BP_FatherSymbioteForm`
   - 2.2.1.c.3) **Quantity:** `1`
   - 2.2.1.c.4) **Probability:** `1.0`

##### 2.2.1.d) Add Engineer Form
   - 2.2.1.d.1) **Items** array: Click **+**
   - 2.2.1.d.2) **Item:** `BP_FatherEngineerForm`
   - 2.2.1.d.3) **Quantity:** `1`
   - 2.2.1.d.4) **Probability:** `1.0`

#### 2.3) Save Collection
   - 2.3.1) Click: **Save**
   - 2.3.2) Close: Data Asset Editor

---

## **PHASE 36: CONFIGURE NPCDEFINITION**

### **1) Open NPCDefinition**

#### 1.1) Locate Definition Asset
   - 1.1.1) In Content Browser: Navigate to `Content/FatherCompanion/`
   - 1.1.2) Look for: `NPCDef_FatherCompanion`
   - 1.1.3) If asset does not exist: Create new NPCDefinition
   - 1.1.4) Double-click: `NPCDef_FatherCompanion`

### **2) Assign Ability Configuration**

#### 2.1) Locate Abilities Section
   - 2.1.1) In Details panel: Find **Abilities** category
   - 2.1.2) Click: Arrow to expand category

#### 2.2) Set Ability Configuration
   - 2.2.1) Locate: **Ability Configuration** property
   - 2.2.2) Click: Dropdown button
   - 2.2.3) Search: `AC_FatherCompanion_Default`
   - 2.2.4) Select: **AC_FatherCompanion_Default**
   - 2.2.5) This grants all 11 baseline abilities on spawn

### **3) Configure Default Item Loadout**

#### 3.1) Locate Inventory Section
   - 3.1.1) In Details panel: Find **Inventory** category
   - 3.1.2) Click: Arrow to expand category

#### 3.2) Find Default Item Loadout Array
   - 3.2.1) Locate: **Default Item Loadout** property
   - 3.2.2) Click: Arrow to expand array

### **4) Add Father Forms to Loadout**

#### 4.1) Option A - Using Item Collection

##### 4.1.1) Add Collection Entry
   - 4.1.1.1) Click: **+** button next to **Default Item Loadout**
   - 4.1.1.2) Element [0] appears
   - 4.1.1.3) Expand: Element [0]

##### 4.1.2) Configure Loot Roll
   - 4.1.2.1) Find: **Item Collections to Grant** array in element
   - 4.1.2.2) Click: **+** button
   - 4.1.2.3) Click: Dropdown on element
   - 4.1.2.4) Search: `IC_FatherForms`
   - 4.1.2.5) Select: **IC_FatherForms**
   - 4.1.2.6) This grants all 4 forms together

#### 4.2) Option B - Direct Items

##### 4.2.1) Add Armor Form Entry
   - 4.2.1.1) Click: **+** button next to **Default Item Loadout**
   - 4.2.1.2) Element [0] appears
   - 4.2.1.3) Expand: Element [0]
   - 4.2.1.4) Find: **Items to Grant** array
   - 4.2.1.5) Click: **+** button in **Items to Grant**
   - 4.2.1.6) **Item Class:** `BP_FatherArmorForm`
   - 4.2.1.7) **Quantity:** `1`
   - 4.2.1.8) **Probability:** `1.0`

##### 4.2.2) Add Exoskeleton Form Entry
   - 4.2.2.1) Click: **+** button next to **Default Item Loadout**
   - 4.2.2.2) Element [1] appears
   - 4.2.2.3) **Item Class:** `BP_FatherExoskeletonForm`
   - 4.2.2.4) **Quantity:** `1`
   - 4.2.2.5) **Probability:** `1.0`

##### 4.2.3) Add Symbiote Form Entry
   - 4.2.3.1) Click: **+** button next to **Default Item Loadout**
   - 4.2.3.2) Element [2] appears
   - 4.2.3.3) **Item Class:** `BP_FatherSymbioteForm`
   - 4.2.3.4) **Quantity:** `1`
   - 4.2.3.5) **Probability:** `1.0`

##### 4.2.4) Add Engineer Form Entry
   - 4.2.4.1) Click: **+** button next to **Default Item Loadout**
   - 4.2.4.2) Element [3] appears
   - 4.2.4.3) **Item Class:** `BP_FatherEngineerForm`
   - 4.2.4.4) **Quantity:** `1`
   - 4.2.4.5) **Probability:** `1.0`

#### 4.3) Verify Configuration
   - 4.3.1) **Default Item Loadout** should contain either:

| Option | Contents |
|--------|----------|
| Option A | 1 entry with IC_FatherForms |
| Option B | 4 entries with individual form items |

### **5) Configure Other NPCDefinition Settings**

#### 5.1) Character Definition
   - 5.1.1) Locate: **Character Definition** property
   - 5.1.2) Assign: Father character definition if needed

#### 5.2) Character Mesh
   - 5.2.1) Locate: **Character Mesh** property
   - 5.2.2) Assign: Father skeletal mesh

### **6) Save NPCDefinition**
   - 6.1) Click: **Save** button in toolbar
   - 6.2) Close: NPCDefinition Editor

---

## **PHASE 37: CREATE NARRATIVE EVENTS**

### **1) Create NE_SetFatherOwner Event**

#### 1.1) Navigate to Folder
   - 1.1.1) In Content Browser: Navigate to `Content/FatherCompanion/Events/`
   - 1.1.2) If folder does not exist: Create folder

#### 1.2) Create Event Blueprint
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select: **Blueprint Class**
   - 1.2.3) In Pick Parent Class: Expand **All Classes**
   - 1.2.4) Search: `NarrativeEvent`
   - 1.2.5) Select: **NarrativeEvent**
   - 1.2.6) Click: **Select**
   - 1.2.7) Name: `NE_SetFatherOwner`
   - 1.2.8) Double-click to open

#### 1.3) Configure Event Properties
   - 1.3.1) Click: **Class Defaults** button
   - 1.3.2) In Details panel:
   - 1.3.3) **Event Filter**: Only NPCs
   - 1.3.4) **Refire on Load**: Unchecked
   - 1.3.5) **Event Runtime**: Start

#### 1.4) Override ExecuteEvent
   - 1.4.1) In My Blueprint panel: Find **Functions**
   - 1.4.2) Click: **Override** dropdown
   - 1.4.3) Select: **Execute Event**
   - 1.4.4) Double-click to open function graph

#### 1.5) Implement ExecuteEvent Logic

##### 1.5.1) Cast Target to Father
   - 1.5.1.1) From Entry node:
   - 1.5.1.2) Drag **Target** parameter into graph
   - 1.5.1.3) Add **Cast To BP_FatherCompanion** node
   - 1.5.1.4) Connect Target to Object input

##### 1.5.2) Get Player from Controller
   - 1.5.2.1) Drag **Controller** parameter into graph
   - 1.5.2.2) Add **Get Controlled Pawn** node
   - 1.5.2.3) Connect Controller to Target

##### 1.5.3) Cast to Player
   - 1.5.3.1) From Get Controlled Pawn Return Value:
   - 1.5.3.2) Add **Cast To BP_NarrativePlayer** node

##### 1.5.4) Call InitOwnerReference
   - 1.5.4.1) From both casts successful (use Sequence if needed):
   - 1.5.4.2) Add **InitOwnerReference** node
   - 1.5.4.3) Target: Connect As BP_FatherCompanion
   - 1.5.4.4) NewOwner: Connect As BP_NarrativePlayer

#### 1.6) Save Event
   - 1.6.1) Click: **Compile**
   - 1.6.2) Click: **Save**

### **2) Create NE_ClearFatherOwner Event**

#### 2.1) Create Event Blueprint
   - 2.1.1) Right-click in Content Browser (in Events folder)
   - 2.1.2) Select: **Blueprint Class**
   - 2.1.3) Pick Parent Class: **NarrativeEvent**
   - 2.1.4) Name: `NE_ClearFatherOwner`
   - 2.1.5) Double-click to open

#### 2.2) Configure Event Properties
   - 2.2.1) Click: **Class Defaults** button
   - 2.2.2) **Event Filter**: Only NPCs
   - 2.2.3) **Refire on Load**: Unchecked
   - 2.2.4) **Event Runtime**: Start

#### 2.3) Override ExecuteEvent
   - 2.3.1) In My Blueprint panel: Click **Override** -> **Execute Event**
   - 2.3.2) Double-click to open

#### 2.4) Implement ExecuteEvent Logic

##### 2.4.1) Cast Target to Father
   - 2.4.1.1) From Entry node:
   - 2.4.1.2) Drag **Target** parameter into graph
   - 2.4.1.3) Add **Cast To BP_FatherCompanion** node
   - 2.4.1.4) Connect Target to Object input

##### 2.4.2) Call ClearOwnerReference
   - 2.4.2.1) From Cast success:
   - 2.4.2.2) Add **ClearOwnerReference** node
   - 2.4.2.3) Target: Connect As BP_FatherCompanion

#### 2.5) Save Event
   - 2.5.1) Click: **Compile**
   - 2.5.2) Click: **Save**

### **3) Create GE_GrantRecruitedTag Effect**

#### 3.1) Create Gameplay Effect
   - 3.1.1) Right-click in Content Browser: `Content/FatherCompanion/Effects/`
   - 3.1.2) Select: **Blueprint Class**
   - 3.1.3) Parent Class: **GameplayEffect**
   - 3.1.4) Name: `GE_GrantRecruitedTag`
   - 3.1.5) Double-click to open

#### 3.2) Configure Effect
   - 3.2.1) Click: **Class Defaults**
   - 3.2.2) **Duration Policy**: Infinite
   - 3.2.3) In **Components** array: Add **Target Tags Gameplay Effect Component**
   - 3.2.4) Expand component:
   - 3.2.5) **Grant Tags to Target Actor**: Add `Father.State.Recruited`

#### 3.3) Save Effect
   - 3.3.1) Click: **Save**

---

## **PHASE 38: CREATE BT_FATHERFOLLOW BEHAVIOR TREE**

### **1) Create Behavior Tree Asset**

#### 1.1) Navigate to AI Folder
   - 1.1.1) In Content Browser: Navigate to `Content/FatherCompanion/AI/`
   - 1.1.2) If folder does not exist: Create folder

#### 1.2) Create Behavior Tree
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select: **Artificial Intelligence**
   - 1.2.3) Select: **Behavior Tree**
   - 1.2.4) Name: `BT_FatherFollow`
   - 1.2.5) Double-click to open

### **2) Configure Root Node**

#### 2.1) Set Blackboard Asset
   - 2.1.1) Click: ROOT node
   - 2.1.2) In Details panel: Find **Blackboard Asset** property
   - 2.1.3) Click: Dropdown
   - 2.1.4) Search: `BB_FollowCharacter`
   - 2.1.5) Select: **BB_FollowCharacter**

### **3) Build Tree Structure**

#### 3.1) Add Selector Node
   - 3.1.1) Drag from ROOT node downward
   - 3.1.2) Select: **Composites** category
   - 3.1.3) Select: **Selector**

#### 3.2) Add Blackboard Decorator to Selector
   - 3.2.1) Right-click on Selector node
   - 3.2.2) Select: **Add Decorator**
   - 3.2.3) Search: `Blackboard Based Condition`
   - 3.2.4) Select: **Blackboard Based Condition**
   - 3.2.5) In Details panel:
   - 3.2.5.1) **Key Query**: Is Set
   - 3.2.5.2) **Blackboard Key**: FollowTarget
   - 3.2.5.3) **Notify Observer**: On Result Change

#### 3.3) Add Gameplay Tag Decorator to Selector
   - 3.3.1) Right-click on Selector node
   - 3.3.2) Select: **Add Decorator**
   - 3.3.3) Search: `Gameplay Tag Condition`
   - 3.3.4) Select: **Gameplay Tag Condition**
   - 3.3.5) In Details panel:
   - 3.3.5.1) **Actor to Check**: SelfActor
   - 3.3.5.2) **Tags to Match**: Any
   - 3.3.5.3) Add tag: `Narrative.State.Movement.Falling`
   - 3.3.5.4) **Inverse Condition**: Checked

#### 3.4) Add Sequence Node
   - 3.4.1) Drag from Selector node downward
   - 3.4.2) Select: **Composites** category
   - 3.4.3) Select: **Sequence**

#### 3.5) Add BTS_AdjustFollowSpeed Service to Sequence
   - 3.5.1) Right-click on Sequence node
   - 3.5.2) Select: **Add Service**
   - 3.5.3) Search: `BTS_AdjustFollowSpeed`
   - 3.5.4) Select: **BTS_AdjustFollowSpeed**

#### 3.6) Add Move To Task
   - 3.6.1) Drag from Sequence node downward
   - 3.6.2) Select: **Tasks** category
   - 3.6.3) Select: **Move To**
   - 3.6.4) Click on Move To task
   - 3.6.5) In Details panel:
   - 3.6.5.1) **Blackboard Key**: FollowTarget
   - 3.6.5.2) **Acceptable Radius**: 173.58
   - 3.6.5.3) **Track Moving Goal**: Checked
   - 3.6.5.4) **Reach Test Includes Agent Radius**: Checked
   - 3.6.5.5) **Reach Test Includes Goal Radius**: Checked
   - 3.6.5.6) **Require Navigable End Location**: Checked
   - 3.6.5.7) **Project Goal Location**: Checked

### **4) Final Tree Structure**

| Level | Node Type | Node Name | Services/Decorators |
|-------|-----------|-----------|---------------------|
| 0 | Root | ROOT | BB_FollowCharacter |
| 1 | Composite | Selector | BB Condition (FollowTarget Is Set), Tag Condition (NOT Falling) |
| 2 | Composite | Sequence | BTS_AdjustFollowSpeed |
| 3 | Task | Move To | FollowTarget key |

### **5) Save Behavior Tree**
   - 5.1) Click: **Save** button

---

## **PHASE 39: CREATE BPA_FATHERFOLLOW ACTIVITY**

### **1) Create Activity Blueprint**

#### 1.1) Navigate to AI Folder
   - 1.1.1) In Content Browser: Navigate to `Content/FatherCompanion/AI/`

#### 1.2) Create Activity Blueprint
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select: **Blueprint Class**
   - 1.2.3) In Pick Parent Class: Expand **All Classes**
   - 1.2.4) Search: `BPA_FollowCharacter`
   - 1.2.5) Select: **BPA_FollowCharacter**
   - 1.2.6) Click: **Select**
   - 1.2.7) Name: `BPA_FatherFollow`

### **2) Configure Class Defaults**

#### 2.1) Open Class Defaults
   - 2.1.1) Double-click **BPA_FatherFollow** to open
   - 2.1.2) Click: **Class Defaults** button

#### 2.2) Set Activity Properties

| Property | Value |
|----------|-------|
| Activity Name | Father Follow |
| Behaviour Tree | BT_FatherFollow |
| Supported Goal Type | Goal_FollowCharacter |
| Is Interruptable | Unchecked |

#### 2.3) Configure Owned Tags
   - 2.3.1) Find: **Owned Tags** property
   - 2.3.2) Clear existing tags
   - 2.3.3) Add: `Narrative.State.NPC.Activity.Following`
   - 2.3.4) Add: `Father.State.Following`

### **3) Save Activity**
   - 3.1) Click: **Compile**
   - 3.2) Click: **Save**

---

## **PHASE 40: CREATE ACTCONFIG_FATHERCOMPANION**

### **1) Create ActivityConfiguration Asset**

#### 1.1) Navigate to Configurations Folder
   - 1.1.1) In Content Browser: Navigate to `Content/FatherCompanion/Configurations/`

#### 1.2) Create ActivityConfiguration
   - 1.2.1) Right-click in Content Browser
   - 1.2.2) Select: **Narrative** (or Miscellaneous -> Data Asset)
   - 1.2.3) Select: **NPC Activity Configuration**
   - 1.2.4) Name: `ActConfig_FatherCompanion`
   - 1.2.5) Double-click to open

### **2) Configure Request Interval**

#### 2.1) Set Request Interval
   - 2.1.1) In Details panel: Find **Request Interval** property
   - 2.1.2) Set value: `0.5`

### **3) Configure Default Activities**

#### 3.1) Add Activities to Array
   - 3.1.1) Find: **Default Activities** array
   - 3.1.2) Click: **+** button to add elements

#### 3.2) Activity Configuration

| Index | Activity | Purpose |
|-------|----------|---------|
| 0 | BPA_FatherFollow | Father following player (custom, no AI focus stare) |
| 1 | BPA_Attack_Melee | Crawler melee combat |
| 2 | BPA_Idle | Default idle behavior |

### **4) Configure Goal Generators**

#### 4.1) Add Goal Generator
   - 4.1.1) Find: **Goal Generators** array
   - 4.1.2) Click: **+** button
   - 4.1.3) Click: Dropdown on element
   - 4.1.4) Search: `GoalGenerator_Attack`
   - 4.1.5) Select: **GoalGenerator_Attack**

### **5) Save ActivityConfiguration**
   - 5.1) Click: **Save** button

---

## **PHASE 41: UPDATE NPCDEFINITION WITH ACTIVITY CONFIGURATION**

### **1) Open NPCDefinition**

#### 1.1) Locate Definition Asset
   - 1.1.1) In Content Browser: Navigate to `Content/FatherCompanion/`
   - 1.1.2) Double-click: `NPCDef_FatherCompanion`

### **2) Assign Activity Configuration**

#### 2.1) Locate AI Section
   - 2.1.1) In Details panel: Find **AI** category
   - 2.1.2) Click: Arrow to expand category

#### 2.2) Set Activity Configuration
   - 2.2.1) Locate: **Activity Configuration** property
   - 2.2.2) Click: Dropdown button
   - 2.2.3) Search: `ActConfig_FatherCompanion`
   - 2.2.4) Select: **ActConfig_FatherCompanion**

### **3) Verify Dual Configuration**

| Configuration Type | Asset | Purpose |
|-------------------|-------|---------|
| Ability Configuration | AC_FatherCompanion_Default | Abilities, Attributes |
| Activity Configuration | ActConfig_FatherCompanion | Behaviors, Goal Generators |

### **4) Save NPCDefinition**
   - 4.1) Click: **Save** button

---

## **PHASE 42: ADD VFX COMPONENTS (OPTIONAL)**

### **1) Add Niagara Component**

#### 1.1) Open BP_FatherCompanion
   - 1.1.1) In Content Browser: Navigate to `/Content/FatherCompanion/Characters/`
   - 1.1.2) Double-click: `BP_FatherCompanion`

#### 1.2) Add Niagara Component
   - 1.2.1) In Components panel: Click **Add** button
   - 1.2.2) Search: `Niagara`
   - 1.2.3) Select: **Niagara Particle System Component**
   - 1.2.4) Rename to: `FatherVFX`

#### 1.3) Configure Niagara Component
   - 1.3.1) With FatherVFX selected in Components panel
   - 1.3.2) In Details panel, find **Niagara** category
   - 1.3.3) **Niagara System Asset**: Select `NS_FatherCompanion` (from VFX Guide)
   - 1.3.4) **Auto Activate**: Check
   - 1.3.5) Find **Transform** category
   - 1.3.6) **Location X**: `0.0`
   - 1.3.7) **Location Y**: `0.0`
   - 1.3.8) **Location Z**: `90.0`

#### 1.4) Parent to Mesh
   - 1.4.1) In Components panel: Drag FatherVFX onto Mesh component
   - 1.4.2) FatherVFX now parented to Mesh

### **2) Add Point Light Component**

#### 2.1) Add Component
   - 2.1.1) In Components panel: Click **Add** button
   - 2.1.2) Search: `Point Light`
   - 2.1.3) Select: **Point Light**
   - 2.1.4) Rename to: `AmbientGlow`

#### 2.2) Configure Light Properties
   - 2.2.1) With AmbientGlow selected
   - 2.2.2) In Details panel, find **Light** category
   - 2.2.3) **Intensity**: `500.0`
   - 2.2.4) **Light Color**: Click color picker
   - 2.2.5) Set RGB: `R=255, G=200, B=100`
   - 2.2.6) **Attenuation Radius**: `150.0`
   - 2.2.7) **Source Radius**: `15.0`

#### 2.3) Configure Transform
   - 2.3.1) Find **Transform** category
   - 2.3.2) **Location X**: `0.0`
   - 2.3.3) **Location Y**: `0.0`
   - 2.3.4) **Location Z**: `90.0`

#### 2.4) Configure Mobility
   - 2.4.1) Find **Transform** category
   - 2.4.2) **Mobility**: `Movable`

#### 2.5) Parent to Mesh
   - 2.5.1) In Components panel: Drag AmbientGlow onto Mesh component

### **3) Create VFX Variables**

#### 3.1) Create CorePulseSpeed Variable
   - 3.1.1) In My Blueprint panel: Click **+** next to Variables
   - 3.1.2) Name: `CorePulseSpeed`
   - 3.1.3) **Variable Type**: `Float`
   - 3.1.4) **Instance Editable**: Check
   - 3.1.5) **Category**: `Father|VFX`
   - 3.1.6) **Default Value**: `2.0`
   - 3.1.7) **Tooltip**: `Speed of core energy pulsing`

#### 3.2) Create CorePulseIntensity Variable
   - 3.2.1) Click **+** next to Variables
   - 3.2.2) Name: `CorePulseIntensity`
   - 3.2.3) **Variable Type**: `Float`
   - 3.2.4) **Instance Editable**: Check
   - 3.2.5) **Category**: `Father|VFX`
   - 3.2.6) **Default Value**: `0.3`
   - 3.2.7) **Tooltip**: `Intensity variation of core pulse`

#### 3.3) Create ParticleOrbitSpeed Variable
   - 3.3.1) Click **+** next to Variables
   - 3.3.2) Name: `ParticleOrbitSpeed`
   - 3.3.3) **Variable Type**: `Float`
   - 3.3.4) **Instance Editable**: Check
   - 3.3.5) **Category**: `Father|VFX`
   - 3.3.6) **Default Value**: `30.0`
   - 3.3.7) **Tooltip**: `Rotation speed of orbiting particles`

#### 3.4) Create BaseEmissiveIntensity Variable
   - 3.4.1) Click **+** next to Variables
   - 3.4.2) Name: `BaseEmissiveIntensity`
   - 3.4.3) **Variable Type**: `Float`
   - 3.4.4) **Instance Editable**: Check
   - 3.4.5) **Category**: `Father|VFX`
   - 3.4.6) **Default Value**: `100.0`
   - 3.4.7) **Tooltip**: `Base emissive intensity for materials`

#### 3.5) Create CoreColor Variable
   - 3.5.1) Click **+** next to Variables
   - 3.5.2) Name: `CoreColor`
   - 3.5.3) **Variable Type**: `Linear Color`
   - 3.5.4) **Instance Editable**: Check
   - 3.5.5) **Category**: `Father|VFX`
   - 3.5.6) **Default Value**: `R=1.0, G=0.8, B=0.4, A=1.0`
   - 3.5.7) **Tooltip**: `Color of energy core`

#### 3.6) Create ShellColor Variable
   - 3.6.1) Click **+** next to Variables
   - 3.6.2) Name: `ShellColor`
   - 3.6.3) **Variable Type**: `Linear Color`
   - 3.6.4) **Instance Editable**: Check
   - 3.6.5) **Category**: `Father|VFX`
   - 3.6.6) **Default Value**: `R=0.4, G=0.6, B=1.0, A=1.0`
   - 3.6.7) **Tooltip**: `Color of outer shell`

### **4) Create UpdateVFXForForm Function**

#### 4.1) Add Function
   - 4.1.1) In My Blueprint panel: Click **+** next to Functions
   - 4.1.2) Name: `UpdateVFXForForm`

#### 4.2) Add Input Parameter
   - 4.2.1) In Details panel, find **Inputs** section
   - 4.2.2) Click **+** to add parameter
   - 4.2.3) Name: `NewForm`
   - 4.2.4) Type: `E_FatherForm`

#### 4.3) Implement Switch Logic
   - 4.3.1) From function entry node:
   - 4.3.2) Drag from execution pin
   - 4.3.3) Search: `Switch on E_FatherForm`
   - 4.3.4) Add **Switch on E_FatherForm** node
   - 4.3.5) Connect NewForm to Selection input

#### 4.4) Handle Crawler Case
   - 4.4.1) From Crawler execution pin:
   - 4.4.2) Add **Get FatherVFX** component reference
   - 4.4.3) Add **Set Visibility** node
   - 4.4.4) **New Visibility**: Check (true)
   - 4.4.5) From Set Visibility execution:
   - 4.4.6) Add **Set Niagara Variable (Float)** node
   - 4.4.7) **Component**: Connect FatherVFX
   - 4.4.8) **Variable Name**: `User.CoreScale`
   - 4.4.9) **Value**: `1.0`

#### 4.5) Handle Armor Case
   - 4.5.1) From Armor execution pin:
   - 4.5.2) Add **Get FatherVFX** reference
   - 4.5.3) Add **Set Visibility** node
   - 4.5.4) **New Visibility**: Uncheck (false)

#### 4.6) Handle Exoskeleton Case
   - 4.6.1) From Exoskeleton execution pin:
   - 4.6.2) Add **Get FatherVFX** reference
   - 4.6.3) Add **Set Visibility** node
   - 4.6.4) **New Visibility**: Uncheck (false)

#### 4.7) Handle Symbiote Case
   - 4.7.1) From Symbiote execution pin:
   - 4.7.2) Add **Get FatherVFX** reference
   - 4.7.3) Add **Set Visibility** node
   - 4.7.4) **New Visibility**: Uncheck (false)

#### 4.8) Handle Engineer Case
   - 4.8.1) From Engineer execution pin:
   - 4.8.2) Add **Get FatherVFX** reference
   - 4.8.3) Add **Set Visibility** node
   - 4.8.4) **New Visibility**: Check (true)
   - 4.8.5) From Set Visibility execution:
   - 4.8.6) Add **Set Niagara Variable (Float)** node
   - 4.8.7) **Component**: Connect FatherVFX
   - 4.8.8) **Variable Name**: `User.CoreScale`
   - 4.8.9) **Value**: `0.7`

#### 4.9) Handle Offline Case
   - 4.9.1) From Offline execution pin:
   - 4.9.2) Add **Get FatherVFX** reference
   - 4.9.3) Add **Set Visibility** node
   - 4.9.4) **New Visibility**: Uncheck (false)

### **5) Call UpdateVFXForForm from OnRep_CurrentForm**

#### 5.1) Open OnRep_CurrentForm
   - 5.1.1) In My Blueprint panel: Double-click **OnRep_CurrentForm**

#### 5.2) Add Function Call
   - 5.2.1) After any existing logic:
   - 5.2.2) Add **UpdateVFXForForm** node
   - 5.2.3) Connect **CurrentForm** variable to **NewForm** input

### **6) Compile and Save**
   - 6.1) Click: **Compile** button
   - 6.2) Click: **Save** button

---

## **SYSTEM FLOW SUMMARY**

### **Spawn Flow**

| Step | Action | Result |
|------|--------|--------|
| 1 | NPCSpawner spawns father | NPCDef_FatherCompanion loaded |
| 2 | Ability Configuration applied | 9 baseline abilities granted |
| 3 | Activity Configuration applied | BPA_FatherFollow, BPA_Attack_Melee, BPA_Idle behaviors available |
| 4 | Default Item Loadout applied | 4 form items added to inventory |
| 5 | Father in Wild state | OwnerPlayer = NULL, forms disabled |

### **Recruitment Flow**

| Step | Action | Result |
|------|--------|--------|
| 1 | Player talks to father | Dialogue triggered |
| 2 | Dialogue recruitment node | Fires NE_SetFatherOwner, AI: Add Goal To NPC |
| 3 | InitOwnerReference executes | OwnerPlayer set, FatherCompanionRef set |
| 4 | Father.State.Recruited granted | Form abilities unlocked |
| 5 | GA_FatherCrawler activates | Father starts following player |

### **Form Equip Flow**

| Step | Action | Result |
|------|--------|--------|
| 1 | Player equips BP_FatherArmorForm | EquipmentComponent->EquipItem executes |
| 2 | HandleEquip calls Parent first | GA_ProtectiveDome, GA_DomeBurst granted to player |
| 3 | HandleEquip activates GA_FatherArmor | Father transforms, attaches to chest |
| 4 | GE_EquipmentMod applies | Player gains +50 Armor Rating |

### **Form Unequip Flow**

| Step | Action | Result |
|------|--------|--------|
| 1 | Player unequips form | HandleUnequip executes |
| 2 | HandleUnequip activates GA_FatherCrawler | Father detaches, returns to following |
| 3 | HandleUnequip calls Parent | Form-specific abilities removed |
| 4 | GE_EquipmentMod removed | Stat bonuses return to base |

### **Form Cancellation System**

Each form ability cancels all other form abilities via Cancel Abilities with Tag:

| When Activated | Cancels |
|----------------|---------|
| GA_FatherCrawler | Armor, Exoskeleton, Symbiote, Engineer |
| GA_FatherArmor | Crawler, Exoskeleton, Symbiote, Engineer |
| GA_FatherExoskeleton | Crawler, Armor, Symbiote, Engineer |
| GA_FatherSymbiote | Crawler, Armor, Exoskeleton, Engineer |
| GA_FatherEngineer | Crawler, Armor, Exoskeleton, Symbiote |

---

## **QUICK REFERENCE**

### **Variable Summary (36 Variables + 6 Optional VFX)**

| Variable | Type | Replication | Rep Condition | Category |
|----------|------|-------------|---------------|----------|
| OwnerPlayer | NarrativeCharacter Ref | Replicated | Initial Only | References |
| OwnerPlayerDefinition | Character Definition | None | N/A | References |
| CurrentForm | E_FatherForm | RepNotify | None | State |
| IsAttached | Boolean | RepNotify | None | State |
| IsDeployed | Boolean | Replicated | None | State |
| bIsFirstActivation | Boolean | None | N/A | State |
| ChestSocketName | Name | None | N/A | Attachment |
| BackSocketName | Name | None | N/A | Attachment |
| OriginalMaxWalkSpeed | Float | None | N/A | Movement |
| OriginalJumpZVelocity | Float | None | N/A | Movement |
| ArmorSpeedMultiplier | Float | None | N/A | Armor |
| SpeedBoostMultiplier | Float | None | N/A | Exoskeleton |
| JumpBoostMultiplier | Float | None | N/A | Exoskeleton |
| DeployedTransform | Transform | Replicated | None | Engineer |
| TurretModeEffectHandle | ActiveGEHandle | None | N/A | Engineer |
| MarkedEnemies | Actor Ref Array | Replicated | None | Combat |
| MaxMarks | Integer | None | N/A | Combat |
| PendingMarkTarget | Actor Ref | None | N/A | Combat |
| MarkWidgetMap | Map (Actor->Widget) | None | N/A | Combat |
| ActiveTraps | Actor Ref Array | None | N/A | Combat |
| MaxActiveTraps | Integer | None | N/A | Combat |
| CrawlerStateHandle | ActiveGEHandle | None | N/A | Handles |
| ArmorStateHandle | ActiveGEHandle | None | N/A | Handles |
| ArmorBoostHandle | ActiveGEHandle | None | N/A | Handles |
| ExoskeletonStateHandle | ActiveGEHandle | None | N/A | Handles |
| ExoskeletonSpeedHandle | ActiveGEHandle | None | N/A | Handles |
| SymbioteStateHandle | ActiveGEHandle | None | N/A | Handles |
| SymbioteBoostHandle | ActiveGEHandle | None | N/A | Handles |
| EngineerStateHandle | ActiveGEHandle | None | N/A | Handles |
| FormCooldownHandle | ActiveGEHandle | None | N/A | Handles |
| InvulnerableHandle | ActiveGEHandle | None | N/A | Handles |
| DomeAbilityHandle | AbilitySpecHandle | None | N/A | Handles |
| DomeBurstAbilityHandle | AbilitySpecHandle | None | N/A | Handles |
| DashAbilityHandle | AbilitySpecHandle | None | N/A | Handles |
| SprintAbilityHandle | AbilitySpecHandle | None | N/A | Handles |
| StealthAbilityHandle | AbilitySpecHandle | None | N/A | Handles |
| ProximityAbilityHandle | AbilitySpecHandle | None | N/A | Handles |
| CorePulseSpeed | Float | None | N/A | VFX (Optional) |
| CorePulseIntensity | Float | None | N/A | VFX (Optional) |
| ParticleOrbitSpeed | Float | None | N/A | VFX (Optional) |
| BaseEmissiveIntensity | Float | None | N/A | VFX (Optional) |
| CoreColor | LinearColor | None | N/A | VFX (Optional) |
| ShellColor | LinearColor | None | N/A | VFX (Optional) |

### **Function Summary (11 Functions + 1 Optional VFX)**

| Function | Return Type | Category |
|----------|-------------|----------|
| GetOwnerPlayer | NarrativeCharacter | References |
| InitOwnerReference | None | References |
| ClearOwnerReference | None | References |
| CleanupCrossActorGrants | None | Cleanup |
| DisableTurretAI | None | AI |
| AddToMarkedEnemies | None | Combat |
| RemoveFromMarkedEnemies | None | Combat |
| MarkEnemy | None | Combat |
| StoreMarkWidget | None | Combat |
| DestroyMarkWidgetForEnemy | None | Combat |
| HandleMarkedEnemyDeath | None | Combat |
| UpdateVFXForForm | None | VFX (Optional) |

### **SaveGame Variables**

| Variable | SaveGame | Purpose |
|----------|----------|---------|
| CurrentForm | Enabled | Restore form state on load |
| IsAttached | Enabled | Restore attachment state |
| IsDeployed | Enabled | Restore turret mode state |
| DeployedTransform | Enabled | Restore turret position |
| OwnerPlayerDefinition | Enabled | Restore player ownership via Initialize lookup |

### **Form Summary**

| Form | Attachment | Socket | Special Notes |
|------|------------|--------|---------------|
| Crawler | None | N/A | Default, autonomous following |
| Armor | Chest | FatherChestSocket | Defensive bonuses |
| Exoskeleton | Back | FatherBackSocket | Mobility bonuses |
| Symbiote | Full Body | None | Visual effect, timed duration |
| Engineer | Deployed | None | Separate turret actor |

### **Ability Distribution**

| Category | Count | Granted Via |
|----------|-------|-------------|
| Baseline (always present) | 9 | AC_FatherCompanion_Default |
| Armor-specific | 2 | BP_FatherArmorForm Abilities array |
| Exoskeleton-specific | 3 | BP_FatherExoskeletonForm Abilities array |
| Symbiote-specific | 1 | BP_FatherSymbioteForm Abilities array |
| Engineer-specific | 2 | BP_FatherEngineerForm Abilities array |
| **Total System** | **17** | Combined |

### **Stat Bonuses by Form**

| Form | Attack Rating | Armor Rating | Stealth Rating |
|------|---------------|--------------|----------------|
| Crawler | 0 | 0 | 0 |
| Armor | 0 | +50 | 0 |
| Exoskeleton | +10 | 0 | 0 |
| Symbiote | +100 | 0 | 0 |
| Engineer | 0 | 0 | 0 |

### **Player Socket Summary**

| Socket | Parent Bone | Location | Purpose |
|--------|-------------|----------|---------|
| FatherChestSocket | spine_03 | X=15, Y=0, Z=0 | Armor form attachment |
| FatherBackSocket | spine_03 | X=-25, Y=0, Z=10 | Exoskeleton form attachment |

### **Required Player Variables**

| Variable | Type | Replication |
|----------|------|-------------|
| FatherCompanionRef | BP_FatherCompanion Ref | Replicated |

### **Required Player Functions**

| Function | Return Type | Pure |
|----------|-------------|------|
| GetFatherCompanion | BP_FatherCompanion Ref | Yes |

### **Required Player Components**

| Component | Purpose |
|-----------|---------|
| SymbioteChargeComponent | Tracks damage for Symbiote charge |

### **Required Player Attribute Sets**

| Attribute Set | Purpose |
|---------------|---------|
| AS_DomeAttributes | Dome energy for Protective Dome ability |

### **Assets Created**

| Asset | Type | Location |
|-------|------|----------|
| E_FatherForm | Enumeration | /Content/FatherCompanion/Data/ |
| BP_FatherCompanion | Blueprint | /Content/FatherCompanion/Characters/ |
| AC_FatherCompanion_Default | AbilityConfiguration | /Content/FatherCompanion/Configurations/ |
| ActConfig_FatherCompanion | ActivityConfiguration | /Content/FatherCompanion/Configurations/ |
| BT_FatherFollow | Behavior Tree | /Content/FatherCompanion/AI/ |
| BPA_FatherFollow | Activity Blueprint | /Content/FatherCompanion/AI/ |
| GE_EquipmentModifier_FatherArmor | GameplayEffect | /Content/FatherCompanion/GAS/ |
| GE_EquipmentModifier_FatherExoskeleton | GameplayEffect | /Content/FatherCompanion/GAS/ |
| GE_EquipmentModifier_FatherSymbiote | GameplayEffect | /Content/FatherCompanion/GAS/ |
| GE_EquipmentModifier_FatherEngineer | GameplayEffect | /Content/FatherCompanion/GAS/ |
| BP_FatherArmorForm | EquippableItem | /Content/FatherCompanion/Items/ |
| BP_FatherExoskeletonForm | EquippableItem | /Content/FatherCompanion/Items/ |
| BP_FatherSymbioteForm | EquippableItem | /Content/FatherCompanion/Items/ |
| BP_FatherEngineerForm | EquippableItem | /Content/FatherCompanion/Items/ |
| IC_FatherForms | ItemCollection | /Content/FatherCompanion/Items/ |
| NPCDef_FatherCompanion | NPCDefinition | /Content/FatherCompanion/ |
| NE_SetFatherOwner | NarrativeEvent | /Content/FatherCompanion/Events/ |
| NE_ClearFatherOwner | NarrativeEvent | /Content/FatherCompanion/Events/ |
| GE_GrantRecruitedTag | GameplayEffect | /Content/FatherCompanion/Effects/ |

---

## **CHANGELOG**

| Version | Date | Changes |
|---------|------|---------|
| 3.1 | January 2026 | Added PHASE 42: VFX Components (Optional) for Niagara particle system and ambient light integration. Added 6 VFX variables (CorePulseSpeed, CorePulseIntensity, ParticleOrbitSpeed, BaseEmissiveIntensity, CoreColor, ShellColor). Added UpdateVFXForForm function. Added Related Documents section linking to Father_Companion_VFX_Guide_v1_0. Added Visual Effects row to Key Architecture table. Variable count updated to 36 + 6 optional. Function count updated to 11 + 1 optional. Total phases increased from 41 to 42. |
| 3.0 | January 2026 | Renamed from Spider to Father throughout entire document. All tags, abilities, variables, functions, blueprints, assets renamed. |
| 2.3 | January 2026 | Removed Grant Tags Components from all child Equipment Modifier GEs. Form tags (Father.Form.Armor, etc.) are now granted exclusively via GA Activation Owned Tags in form abilities, preventing duplicate tag granting. PHASE 30 updated: All child GEs now have 0 Components. Verification table updated to show Components column instead of Granted Tag column. Technical Reference v5.12 Section 58.8 documents this architecture decision. |
| 2.2 | January 2026 | Added child Equipment Modifier GEs for each father form. PHASE 30: Create Father Form Equipment Modifier GEs (GE_EquipmentModifier_FatherArmor, GE_EquipmentModifier_FatherExoskeleton, GE_EquipmentModifier_FatherSymbiote with Stamina override, GE_EquipmentModifier_FatherEngineer). Child GEs inherit from GE_EquipmentModifier and add Grant Tags component for form state. Updated EquippableItem phases to use form-specific child GEs instead of base GE_EquipmentModifier. Symbiote child GE includes StaminaRegenRate Override (10000) for infinite stamina - stats no longer applied by ability. Renumbered PHASES 30-40 to 31-41. Total phases increased from 40 to 41. Added 4 new assets. |
| 2.1 | January 2026 | Added Father Follow System. PHASE 37: Create BT_FatherFollow behavior tree (based on BT_FollowCharacter, removes BTS_SetAIFocus so father faces movement direction instead of staring at player). PHASE 38: Create BPA_FatherFollow activity (child of BPA_FollowCharacter, uses custom BT). PHASE 39: Create ActConfig_FatherCompanion (ActivityConfiguration with BPA_FatherFollow, BPA_Attack_Melee, BPA_Idle, GoalGenerator_Attack). PHASE 40: Update NPCDefinition with Activity Configuration. Father now has dual configuration (AC_ for abilities, ActConfig_ for behaviors). BTS_AdjustFollowSpeed retained for speed matching. Updated Spawn Flow. Added 3 new assets (BT_FatherFollow, BPA_FatherFollow, ActConfig_FatherCompanion). Total phases increased from 36 to 40. |
| 2.0 | December 2025 | Moved GA_Backstab to Player Default Abilities - backstab is a generic action game mechanic, not father-exclusive. Removed BackstabAbilityHandle variable. Removed Backstab grant from InitOwnerReference. Removed Backstab cleanup from ClearOwnerReference and CleanupCrossActorGrants. Updated ability handle count from 7 to 6. Updated baseline ability count from 10 to 9. Updated total system abilities from 18 to 17. |
| 1.9 | December 2025 | Removed TurretHealth and MaxTurretHealth from Phase 9 - father already has Health/MaxHealth attributes via NarrativeAttributeSetBase. Turret health now managed via GE_TurretHealth effect that modifies native attributes. Reduces variable count and eliminates redundant state. |
| 1.8 | December 2025 | Added OwnerPlayerDefinition to Phase 24 SaveGame configuration. Without SaveGame enabled, Initialize cannot restore player ownership after load. Updated Quick Reference SaveGame Variables table to include 5 persistent variables with Purpose column. NarrativeSaveSubsystem automatically serializes all SaveGame-marked variables via ArIsSaveGame archive flag. |
| 1.7 | December 2025 | Added TurretHealth and MaxTurretHealth variables to Phase 9 (Engineer Form). TurretHealth is replicated for client health display. MaxTurretHealth is Instance Editable for designer tuning. Updated Quick Reference variable table. Aligns with GA_FatherEngineer v3.5 requirements. |
| 1.6 | December 2025 | Added IsDeployed and DeployedTransform to Phase 24 SaveGame configuration. Updated Quick Reference SaveGame Variables table to include all 4 persistent state variables. Engineer form turret state now persists across save/load. |
| 1.5 | December 2025 | Added EndPlay Safety Net pattern per Technical Reference v4.7 Section 39. Added CleanupCrossActorGrants function (Phase 18) for emergency cleanup of all cross-actor abilities and effects. Added Phase 23 Implement EndPlay with authority-gated cleanup sequence. Renumbered phases 19-36 (was 18-34). Function count increased from 10 to 11. Ensures proper cleanup on father destruction, network disconnect, or level streaming. |
| 1.4 | December 2025 | Updated InitOwnerReference to grant GA_Backstab to player ASC and store BackstabAbilityHandle. Updated ClearOwnerReference to perform two-step cleanup (Cancel Ability + Set Remove Ability On End) for BackstabAbilityHandle before clearing references. GA_Backstab now granted on recruitment and removed on dismiss instead of via Player AbilityConfiguration. Per Technical Reference v4.6 Section 35.5-35.8. |
| 1.3 | December 2025 | Added 7 ability handle variables for cross-actor cleanup (DomeAbilityHandle, DomeBurstAbilityHandle, DashAbilityHandle, SprintAbilityHandle, StealthAbilityHandle, ProximityAbilityHandle, BackstabAbilityHandle). Per Technical Reference v4.5 Section 36.12, form abilities grant sub-abilities to player ASC and must store handles for two-step cleanup (Cancel Ability then Set Remove Ability On End). Renamed Phase 12 to include ability handles. Updated variable count from 29 to 36. |
| 1.2 | December 2025 | Major architecture update. Ownership now via NarrativeEvents (NE_SetFatherOwner, NE_ClearFatherOwner) instead of BeginPlay. Added Father.State.Recruited tag for form ability gating. Added OwnerPlayerDefinition for save/load persistence. Added effect handle variables (10 handles). Replaced DeployedLocation with DeployedTransform. Removed FatherRef (use GetAvatarActorFromActorInfo). Removed AttackTarget (use BB_Attack blackboard). Added InitOwnerReference and ClearOwnerReference functions. Added PrepareForSave and Initialize overrides. Updated to 34 phases. |
| 1.1 | December 2025 | Removed GA_FatherDetach and GA_TurretRecall (v1.6 architecture change). Updated ability counts from 21 to 18. Updated baseline abilities from 11 to 10. Updated Engineer form abilities from 3 to 2. Form switching now direct via T wheel only. |
| 1.0 | December 2025 | Initial consolidated release. Merged BP_FatherCompanion_Setup_Guide_v2_5.md and Father_Companion_Forms_Implementation_Guide_v4_0.md. Removed AS_SymbioteAttributes from player components (shield system not used per Design Doc v1.5). Updated Symbiote stat bonuses to reflect offensive-only focus. Consolidated player skeleton socket sections. Added complete NPCDefinition workflow. 30 phases covering full system setup. |

---

**END OF FATHER COMPANION SYSTEM SETUP GUIDE VERSION 3.1**

**Unreal Engine 5.6 + Narrative Pro Plugin v2.2**

**Blueprint-Only Implementation**
