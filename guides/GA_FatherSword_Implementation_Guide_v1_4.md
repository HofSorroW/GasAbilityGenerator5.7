# GA_FatherSword - Sword Form Implementation Guide
## Father Companion with Gameplay Ability System
## For Unreal Engine 5.6 + Narrative Pro Plugin v2.2

**Version:** 1.3
**Date:** January 2026
**Engine:** Unreal Engine 5.6
**Plugin:** Narrative Pro v2.2
**Implementation:** Blueprint Only
**Parent Class:** MeleeWeaponItem

---

## DOCUMENT PURPOSE

This guide provides step-by-step instructions for implementing the Sword Form, where the father companion physically transforms into a single-hand melee weapon wielded by the player. This form leverages Narrative Pro's built-in MeleeWeaponItem system for combat functionality.

**Key Features:**

| Feature | Value |
|---------|-------|
| Weapon Type | Single-Hand Melee Sword |
| Base Class | MeleeWeaponItem |
| Attack Type | Socket-based sphere trace (inherited) |
| Combo System | Light and Heavy attacks |
| Damage Source | AttackDamage attribute via SetByCaller |
| Visual | Father morphs into bio-organic blade |

**Stat Bonuses:**

| Stat | Value | Method |
|------|-------|--------|
| Attack Rating | +75 | EquippableItem ArmorRating |
| Attack Damage | 25.0 | WeaponItem AttackDamage |
| Armor Rating | +25 | EquippableItem ArmorRating |

---

## PREREQUISITES

Before implementing the Sword Form, ensure the following are complete:

| Prerequisite | Type | Status |
|--------------|------|--------|
| BP_FatherCompanion | NarrativeNPCCharacter | Must exist |
| AC_FatherCompanion_Default | AbilityConfiguration | Must exist |
| GE_WeaponDamage | GameplayEffect | Narrative Pro built-in |
| GA_Melee_Unarmed | GameplayAbility | Narrative Pro built-in |
| GE_EquipmentMod | GameplayEffect | Narrative Pro built-in |
| AM_FatherSwordAttack | AnimMontage | Create placeholder |
| Player hand_r socket | Skeleton Socket | Standard hand socket |

---

## QUICK REFERENCE

### Tag Configuration Summary

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Sword |
| Equippable Slot | Narrative.Equipment.Slot.Weapon_Back |
| Form State Tag | Father.Form.Sword |
| State Tag | Father.State.Wielded |

### Multiplayer Settings

| Setting | Value |
|---------|-------|
| Instancing Policy | Instanced Per Actor |
| Replication Policy | Replicate Yes |
| Net Execution Policy | Local Predicted |

### WeaponItem vs EquippableItem

| Aspect | EquippableItem (Other Forms) | MeleeWeaponItem (Sword Form) |
|--------|------------------------------|------------------------------|
| Slot | FatherForm (custom) | Weapon_Back (standard) |
| Combat | Separate attack abilities | Inherited weapon attacks |
| Wielding | Always "equipped" | Wield/Unwield cycle |
| Holstering | N/A | Holsters to back when not in use |

---

## PHASE 1: CONFIGURE GAMEPLAY TAGS

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Sword | Father sword weapon form ability |
| Father.Form.Sword | Father is transformed into sword weapon |
| Father.State.Wielded | Father weapon is actively wielded by player |
| Narrative.Anim.AnimSets.Attacks.FatherSword | Attack animation set parent |
| Narrative.Anim.AnimSets.Attacks.FatherSword.Light | Light attack combo |
| Narrative.Anim.AnimSets.Attacks.FatherSword.Heavy | Heavy attack combo |

### **Verify Existing Tags**

| Tag Name | Purpose |
|----------|---------|
| Narrative.Equipment.Slot.Weapon_Back | Equipment slot for sword holstering |
| Father.State.Recruited | Recruitment state (Activation Required) |

---

## PHASE 2: CREATE FATHER SWORD WEAPON ITEM

### 4) Create BP_FatherSwordWeapon Blueprint

#### 4.1) Navigate to Weapons Folder

##### 4.1.1) Create Folder Structure
   - 4.1.1.1) In Content Browser: Navigate to Content/FatherCompanion/
   - 4.1.1.2) Right-click in empty space
   - 4.1.1.3) Select New Folder
   - 4.1.1.4) Name folder: Weapons
   - 4.1.1.5) Double-click to open Weapons folder

#### 4.2) Create MeleeWeaponItem Blueprint

##### 4.2.1) Create New Blueprint
   - 4.2.1.1) Right-click in Content Browser
   - 4.2.1.2) Select Blueprint Class
   - 4.2.1.3) In Pick Parent Class dialog
   - 4.2.1.4) Search: MeleeWeaponItem
   - 4.2.1.5) Select MeleeWeaponItem
   - 4.2.1.6) Click Select

##### 4.2.2) Name Blueprint
   - 4.2.2.1) Type: BP_FatherSwordWeapon
   - 4.2.2.2) Press Enter
   - 4.2.2.3) Double-click to open

#### 4.3) Configure Item Properties

##### 4.3.1) Open Class Defaults
   - 4.3.1.1) In Blueprint Editor toolbar
   - 4.3.1.2) Click Class Defaults button

##### 4.3.2) Configure Item Category

###### 4.3.2.1) Display Name
   - 4.3.2.1.1) In Details panel, find Item category
   - 4.3.2.1.2) Locate Display Name property
   - 4.3.2.1.3) Enter: Father Sword

###### 4.3.2.2) Description
   - 4.3.2.2.1) Locate Description property
   - 4.3.2.2.2) Enter: Transform your father companion into a bio-organic blade. Grants enhanced melee combat with light and heavy attack combos.

###### 4.3.2.3) Max Stack Size
   - 4.3.2.3.1) Locate Max Stack Size property
   - 4.3.2.3.2) Enter: 1

###### 4.3.2.4) Weight
   - 4.3.2.4.1) Locate Weight property
   - 4.3.2.4.2) Enter: 0.0

##### 4.3.3) Configure Equippable Properties

###### 4.3.3.1) Equippable Slot
   - 4.3.3.1.1) In Details panel, find Item - Equippable category
   - 4.3.3.1.2) Locate Equippable Slot property
   - 4.3.3.1.3) Click dropdown
   - 4.3.3.1.4) Select: Narrative.Equipment.Slot.Weapon_Back

###### 4.3.3.2) Equipment Mod GE
   - 4.3.3.2.1) Locate Equipment Mod GE property
   - 4.3.3.2.2) Click dropdown
   - 4.3.3.2.3) Select: GE_EquipmentMod

###### 4.3.3.3) Attack Rating
   - 4.3.3.3.1) Locate Attack Rating property
   - 4.3.3.3.2) Enter: 75.0

###### 4.3.3.4) Armor Rating
   - 4.3.3.4.1) Locate Armor Rating property
   - 4.3.3.4.2) Enter: 25.0

###### 4.3.3.5) Stealth Rating
   - 4.3.3.5.1) Locate Stealth Rating property
   - 4.3.3.5.2) Enter: 0.0

#### 4.4) Configure Weapon Properties

##### 4.4.1) Attack Damage
   - 4.4.1.1) In Details panel, find Weapon category
   - 4.4.1.2) Locate Attack Damage property
   - 4.4.1.3) Enter: 25.0

##### 4.4.2) Weapon Visual Attach Bone
   - 4.4.2.1) Locate Weapon Visual Attach Bone property
   - 4.4.2.2) Enter: hand_r

##### 4.4.3) Weapon Visual Holstered Attach Bone
   - 4.4.3.1) Locate Weapon Visual Holstered Attach Bone property
   - 4.4.3.2) Enter: spine_03

#### 4.5) Configure Melee Data

##### 4.5.1) Expand Melee Data
   - 4.5.1.1) In Details panel, find Weapon | Config | Trace category
   - 4.5.1.2) Click arrow to expand Melee Data

##### 4.5.2) Configure Trace Data

###### 4.5.2.1) Trace Distance
   - 4.5.2.1.1) Locate Trace Data -> Trace Distance
   - 4.5.2.1.2) Enter: 150.0

###### 4.5.2.2) Trace Radius
   - 4.5.2.2.1) Locate Trace Data -> Trace Radius
   - 4.5.2.2.2) Enter: 35.0

###### 4.5.2.3) Trace Multi
   - 4.5.2.3.1) Locate Trace Data -> bTraceMulti
   - 4.5.2.3.2) Check: TRUE

##### 4.5.3) Configure Attack Combos

###### 4.5.3.1) Attack Combos (Light)
   - 4.5.3.1.1) Locate Attack Combos array
   - 4.5.3.1.2) Click + to add element
   - 4.5.3.1.3) Select or create AttackComboAnimSet with tag:
   - 4.5.3.1.4) Narrative.Anim.AnimSets.Attacks.FatherSword.Light

###### 4.5.3.2) Heavy Attack Combos
   - 4.5.3.2.1) Locate Heavy Attack Combos array
   - 4.5.3.2.2) Click + to add element
   - 4.5.3.2.3) Select or create AttackComboAnimSet with tag:
   - 4.5.3.2.4) Narrative.Anim.AnimSets.Attacks.FatherSword.Heavy

#### 4.6) Configure Abilities Array

##### 4.6.1) Find Abilities Property
   - 4.6.1.1) In Details panel, find Abilities category
   - 4.6.1.2) Locate Abilities array

##### 4.6.2) Add Form Ability
   - 4.6.2.1) Click + to add element [0]
   - 4.6.2.2) Select: GA_FatherSword (create in Phase 4)

#### 4.7) Compile and Save

##### 4.7.1) Compile Blueprint
   - 4.7.1.1) Click Compile in toolbar
   - 4.7.1.2) Verify no errors in Compiler Results

##### 4.7.2) Save Blueprint
   - 4.7.2.1) Click Save in toolbar
   - 4.7.2.2) Close Blueprint Editor

---

## PHASE 3: CREATE FATHER SWORD WEAPON VISUAL

### 5) Create BP_FatherSwordVisual Blueprint

#### 5.1) Navigate to Visuals Folder

##### 5.1.1) Create Folder
   - 5.1.1.1) In Content/FatherCompanion/Weapons/
   - 5.1.1.2) Right-click -> New Folder
   - 5.1.1.3) Name: Visuals

#### 5.2) Create WeaponVisual Blueprint

##### 5.2.1) Create New Blueprint
   - 5.2.1.1) Right-click in Visuals folder
   - 5.2.1.2) Select Blueprint Class
   - 5.2.1.3) Search: WeaponVisual
   - 5.2.1.4) Select WeaponVisual
   - 5.2.1.5) Click Select
   - 5.2.1.6) Name: BP_FatherSwordVisual

#### 5.3) Configure Visual Components

##### 5.3.1) Open Blueprint
   - 5.3.1.1) Double-click BP_FatherSwordVisual
   - 5.3.1.2) Navigate to Viewport tab

##### 5.3.2) Configure Skeletal Mesh
   - 5.3.2.1) Select WeaponMesh component in Components panel
   - 5.3.2.2) In Details panel, find Mesh category
   - 5.3.2.3) Set Skeletal Mesh: (use placeholder sword mesh or father sword mesh)

##### 5.3.3) Configure Transform
   - 5.3.3.1) Location: X=0, Y=0, Z=0
   - 5.3.3.2) Rotation: X=0, Y=0, Z=0
   - 5.3.3.3) Scale: X=1, Y=1, Z=1

#### 5.4) Compile and Save

##### 5.4.1) Compile and Save
   - 5.4.1.1) Click Compile
   - 5.4.1.2) Click Save
   - 5.4.1.3) Close Blueprint Editor

### 6) Link Visual to Weapon Item

#### 6.1) Open BP_FatherSwordWeapon

##### 6.1.1) Navigate and Open
   - 6.1.1.1) In Content Browser, navigate to Content/FatherCompanion/Weapons/
   - 6.1.1.2) Double-click BP_FatherSwordWeapon

#### 6.2) Configure Weapon Visual Class

##### 6.2.1) Set Visual Class
   - 6.2.1.1) Click Class Defaults
   - 6.2.1.2) In Details, find Weapon Visual category
   - 6.2.1.3) Locate Weapon Visual Class property
   - 6.2.1.4) Click dropdown
   - 6.2.1.5) Select: BP_FatherSwordVisual

#### 6.3) Compile and Save

##### 6.3.1) Save Changes
   - 6.3.1.1) Click Compile
   - 6.3.1.2) Click Save

---

## PHASE 4: CREATE FORM STATE GAMEPLAY EFFECT

### 7) Create GE_SwordState Gameplay Effect

#### 7.1) Navigate to Effects Folder

##### 7.1.1) Create Folder if Needed
   - 7.1.1.1) In Content/FatherCompanion/
   - 7.1.1.2) Create folder: Effects (if not exists)

#### 7.2) Create Gameplay Effect Blueprint

##### 7.2.1) Create New Blueprint
   - 7.2.1.1) Right-click in Effects folder
   - 7.2.1.2) Select Blueprint Class
   - 7.2.1.3) Search: GameplayEffect
   - 7.2.1.4) Select GameplayEffect
   - 7.2.1.5) Click Select
   - 7.2.1.6) Name: GE_SwordState

#### 7.3) Configure GE_SwordState Properties

| Property | Value |
|----------|-------|
| Duration Policy | Infinite |
| Components | Grant Tags to Target Actor |
| Granted Tags [0] | Father.Form.Sword |
| Granted Tags [1] | Father.State.Wielded |
| Components | Remove Other Gameplay Effect on Application |
| Remove GE Query -> Any Tags Match | Father.Form |
| Components | Asset Tags |
| Asset Tags [0] | Effect.Father.FormState |
| Asset Tags [1] | Effect.Father.SwordState |

#### 7.4) Compile and Save

---

## PHASE 5: CREATE GA_FATHERSWORD ABILITY

### 8) Create GA_FatherSword Gameplay Ability

#### 8.1) Navigate to Abilities Folder

##### 8.1.1) Create Folder if Needed
   - 8.1.1.1) In Content/FatherCompanion/
   - 8.1.1.2) Create folder: Abilities (if not exists)

#### 8.2) Create Ability Blueprint

##### 8.2.1) Create New Blueprint
   - 8.2.1.1) Right-click in Abilities folder
   - 8.2.1.2) Select Blueprint Class
   - 8.2.1.3) Search: NarrativeGameplayAbility
   - 8.2.1.4) Select NarrativeGameplayAbility
   - 8.2.1.5) Click Select
   - 8.2.1.6) Name: GA_FatherSword

#### 8.3) Configure Ability Properties

##### 8.3.1) Open Blueprint and Class Defaults
   - 8.3.1.1) Double-click GA_FatherSword
   - 8.3.1.2) Click Class Defaults

##### 8.3.2) Configure Tags Section

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Sword |
| Activation Blocked Tags | Father.Form.Sword |

##### 8.3.3) Configure Advanced Section

###### 8.3.3.1) Instancing Policy
   - 8.3.3.1.1) In Details, find Advanced category
   - 8.3.3.1.2) Locate Instancing Policy
   - 8.3.3.1.3) Select: Instanced Per Actor

###### 8.3.3.2) Replication Policy
   - 8.3.3.2.1) Locate Replication Policy
   - 8.3.3.2.2) Select: Replicate Yes

###### 8.3.3.3) Net Execution Policy
   - 8.3.3.3.1) Locate Net Execution Policy
   - 8.3.3.3.2) Select: Local Predicted

##### 8.3.4) Configure Input Tag
   - 8.3.4.1) In Details, find Narrative Ability category
   - 8.3.4.2) Locate Input Tag
   - 8.3.4.3) Set: Narrative.Input.Father.FormChange

#### 8.4) Create Variables

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| FatherReference | BP_FatherCompanion (Object Reference) | None | No |
| PlayerReference | NarrativePlayerCharacter (Object Reference) | None | No |

#### 8.5) Implement Activation Logic

##### 8.5.1) Open Event Graph
   - 8.5.1.1) Click Event Graph tab

##### 8.5.2) Create Event ActivateAbility
   - 8.5.2.1) Right-click in Event Graph
   - 8.5.2.2) Search: Event ActivateAbility
   - 8.5.2.3) Select Event ActivateAbility

##### 8.5.3) Get Avatar Actor
   - 8.5.3.1) From Event ActivateAbility execution pin
   - 8.5.3.2) Drag wire to right, release
   - 8.5.3.3) Search: Get Avatar Actor From Actor Info
   - 8.5.3.4) Select Get Avatar Actor from Actor Info

##### 8.5.4) Cast to BP_FatherCompanion
   - 8.5.4.1) From Return Value pin
   - 8.5.4.2) Drag wire to right
   - 8.5.4.3) Search: Cast To BP_FatherCompanion
   - 8.5.4.4) Connect execution from Get Avatar Actor

##### 8.5.5) Store Father Reference
   - 8.5.5.1) From As BP Father Companion pin
   - 8.5.5.2) Drag wire to right
   - 8.5.5.3) Search: Set FatherReference
   - 8.5.5.4) Connect As BP Father Companion to input

##### 8.5.6) Get Owner Player
   - 8.5.6.1) From As BP Father Companion pin
   - 8.5.6.2) Drag wire to right (new connection)
   - 8.5.6.3) Search: Get Owner Player
   - 8.5.6.4) Creates pure function node

##### 8.5.7) Validate Owner Player
   - 8.5.7.1) From Get Owner Player Return Value
   - 8.5.7.2) Drag wire to right
   - 8.5.7.3) Search: Is Valid
   - 8.5.7.4) Connect Return Value to Input Object

##### 8.5.8) Store Player Reference
   - 8.5.8.1) From Is Valid -> Is Valid execution pin
   - 8.5.8.2) Add Set PlayerReference node
   - 8.5.8.3) Connect Get Owner Player Return Value to input

##### 8.5.9) Get Father ASC
   - 8.5.9.1) From FatherReference variable
   - 8.5.9.2) Drag wire to right
   - 8.5.9.3) Search: Get Ability System Component
   - 8.5.9.4) Returns father's ASC

##### 8.5.10) Apply GE_SwordState
   - 8.5.10.1) From Set PlayerReference execution pin
   - 8.5.10.2) Add Apply Gameplay Effect to Self node
   - 8.5.10.3) Target: Connect Father's ASC
   - 8.5.10.4) Gameplay Effect Class: GE_SwordState
   - 8.5.10.5) Level: 1

##### 8.5.11) Hide Father Actor
   - 8.5.11.1) From Apply GE execution pin
   - 8.5.11.2) Add Set Actor Hidden In Game node
   - 8.5.11.3) Target: FatherReference
   - 8.5.11.4) New Hidden: TRUE

##### 8.5.12) Disable Father Collision
   - 8.5.12.1) From Set Actor Hidden execution
   - 8.5.12.2) Add Set Actor Enable Collision node
   - 8.5.12.3) Target: FatherReference
   - 8.5.12.4) New Actor Enable Collision: FALSE

##### 8.5.13) Give Weapon to Player
   - 8.5.13.1) From PlayerReference variable
   - 8.5.13.2) Drag wire -> Get Inventory Component
   - 8.5.13.3) From Disable Collision execution
   - 8.5.13.4) Add Give Item node
   - 8.5.13.5) Target: Player's Inventory Component
   - 8.5.13.6) Item Class: BP_FatherSwordWeapon
   - 8.5.13.7) Quantity: 1

##### 8.5.14) Equip Weapon
   - 8.5.14.1) From Give Item Return Value (item reference)
   - 8.5.14.2) Drag wire to right
   - 8.5.14.3) Search: Equip Item
   - 8.5.14.4) From Give Item execution pin
   - 8.5.14.5) Connect execution to Equip Item

##### 8.5.15) Wield Weapon
   - 8.5.15.1) From Equip Item execution pin
   - 8.5.15.2) From PlayerReference
   - 8.5.15.3) Search: Wield Weapon
   - 8.5.15.4) Connect player to Target
   - 8.5.15.5) Weapon: Cast Give Item Return Value to WeaponItem

##### 8.5.16) Play Transform Animation
   - 8.5.16.1) From Wield Weapon execution pin
   - 8.5.16.2) Add Play Montage node
   - 8.5.16.3) Target: PlayerReference -> Get Mesh
   - 8.5.16.4) Montage to Play: AM_FatherSwordTransform

##### 8.5.17) End Ability
   - 8.5.17.1) From Play Montage -> On Completed pin
   - 8.5.17.2) Add End Ability node
   - 8.5.17.3) Was Cancelled: FALSE

#### 8.6) Compile and Save

##### 8.6.1) Compile Blueprint
   - 8.6.1.1) Click Compile
   - 8.6.1.2) Verify no errors

##### 8.6.2) Save Blueprint
   - 8.6.2.1) Click Save
   - 8.6.2.2) Close Blueprint Editor

---

## PHASE 6: CREATE SWORD ATTACK ABILITY

### 9) Create GA_FatherSwordAttack Ability

#### 9.1) Create Ability Blueprint

##### 9.1.1) Create New Blueprint
   - 9.1.1.1) In Content/FatherCompanion/Abilities/
   - 9.1.1.2) Right-click -> Blueprint Class
   - 9.1.1.3) Search parent: GA_Attack_Combo_Melee
   - 9.1.1.4) Select GA_Attack_Combo_Melee
   - 9.1.1.5) Name: GA_FatherSwordAttack

#### 9.2) Configure Ability Properties

##### 9.2.1) Open Class Defaults
   - 9.2.1.1) Double-click GA_FatherSwordAttack
   - 9.2.1.2) Click Class Defaults

##### 9.2.2) Configure Tags

###### 9.2.2.1) Ability Tags
   - 9.2.2.1.1) Add: Ability.Father.Sword.Attack

###### 9.2.2.2) Activation Required Tags
   - 9.2.2.2.1) Add: Father.Form.Sword

##### 9.2.3) Configure Input Tag
   - 9.2.3.1) Input Tag: Narrative.Input.Attack

##### 9.2.4) Configure Damage
   - 9.2.4.1) Damage Effect Class: GE_WeaponDamage
   - 9.2.4.2) Default Attack Damage: 25.0

##### 9.2.5) Configure Combo Anims
   - 9.2.5.1) Attack Combo Anim Set: Narrative.Anim.AnimSets.Attacks.FatherSword.Light
   - 9.2.5.2) Heavy Attack Combo Anim Set: Narrative.Anim.AnimSets.Attacks.FatherSword.Heavy

##### 9.2.6) Configure Warp
   - 9.2.6.1) Should Warp: TRUE
   - 9.2.6.2) Warp Maintain Dist: 80.0
   - 9.2.6.3) Min Warp Dist: 20.0
   - 9.2.6.4) Max Warp Dist: 200.0

#### 9.3) Compile and Save

##### 9.3.1) Save
   - 9.3.1.1) Click Compile
   - 9.3.1.2) Click Save

---

## PHASE 7: UPDATE WEAPON ITEM WITH ATTACK ABILITY

### 10) Add Attack Ability to Weapon

#### 10.1) Open BP_FatherSwordWeapon

##### 10.1.1) Navigate and Open
   - 10.1.1.1) In Content Browser -> Content/FatherCompanion/Weapons/
   - 10.1.1.2) Double-click BP_FatherSwordWeapon

#### 10.2) Update Abilities Array

##### 10.2.1) Add Attack Ability
   - 10.2.1.1) Click Class Defaults
   - 10.2.1.2) Find Abilities array
   - 10.2.1.3) Click + to add element [1]
   - 10.2.1.4) Select: GA_FatherSwordAttack

##### 10.2.2) Verify Abilities
   - 10.2.2.1) Element [0]: GA_FatherSword
   - 10.2.2.2) Element [1]: GA_FatherSwordAttack

#### 10.3) Compile and Save

##### 10.3.1) Save
   - 10.3.1.1) Click Compile
   - 10.3.1.2) Click Save

---

## PHASE 8: CREATE DETACH LOGIC

### 11) Create GA_FatherSwordDetach Ability

#### 11.1) Create Ability Blueprint

##### 11.1.1) Create New Blueprint
   - 11.1.1.1) In Content/FatherCompanion/Abilities/
   - 11.1.1.2) Right-click -> Blueprint Class
   - 11.1.1.3) Parent: NarrativeGameplayAbility
   - 11.1.1.4) Name: GA_FatherSwordDetach

#### 11.2) Configure Properties

##### 11.2.1) Tags
   - 11.2.1.1) Ability Tags: Ability.Father.Sword.Detach
   - 11.2.1.2) Activation Required Tags: Father.Form.Sword

##### 11.2.2) Input Tag
   - 11.2.2.1) Input Tag: Narrative.Input.Father.Detach

#### 11.3) Implement Detach Logic

##### 11.3.1) Event ActivateAbility
   - 11.3.1.1) Get Avatar Actor from Actor Info
   - 11.3.1.2) Cast To NarrativePlayerCharacter

##### 11.3.2) Get Father Reference
   - 11.3.2.1) From player, get father companion reference
   - 11.3.2.2) Store in local variable

##### 11.3.3) Unwield and Unequip Weapon
   - 11.3.3.1) Get player's current weapon
   - 11.3.3.2) Unwield Weapon
   - 11.3.3.3) From player Get Inventory Component
   - 11.3.3.4) Remove Item (BP_FatherSwordWeapon instance)

##### 11.3.4) Show Father Actor
   - 11.3.4.1) Set Actor Hidden In Game: FALSE
   - 11.3.4.2) Target: Father Reference

##### 11.3.5) Enable Father Collision
   - 11.3.5.1) Set Actor Enable Collision: TRUE

##### 11.3.6) Calculate Spawn Position
   - 11.3.6.1) Get player location
   - 11.3.6.2) Get player forward vector
   - 11.3.6.3) Multiply forward by -200
   - 11.3.6.4) Add to player location

##### 11.3.7) Set Father Location
   - 11.3.7.1) Set Actor Location node
   - 11.3.7.2) Target: Father Reference
   - 11.3.7.3) New Location: Calculated spawn position

##### 11.3.8) Apply GE_CrawlerState
   - 11.3.8.1) Get Father ASC
   - 11.3.8.2) Apply Gameplay Effect to Self
   - 11.3.8.3) Effect: GE_CrawlerState

##### 11.3.9) Play Animation
   - 11.3.9.1) Play Montage: AM_FatherSwordDetach

##### 11.3.10) End Ability
   - 11.3.10.1) From On Completed pin
   - 11.3.10.2) End Ability

#### 11.4) Compile and Save

##### 11.4.1) Save
   - 11.4.1.1) Click Compile
   - 11.4.1.2) Click Save

---

## PHASE 9: UPDATE ABILITYCONFIGURATION

### 12) Add Sword Ability to Baseline

#### 12.1) Open AC_FatherCompanion_Default

##### 12.1.1) Navigate and Open
   - 12.1.1.1) In Content Browser -> Content/FatherCompanion/Configurations/
   - 12.1.1.2) Double-click AC_FatherCompanion_Default

#### 12.2) Add GA_FatherSword to Default Abilities

##### 12.2.1) Add New Element
   - 12.2.1.1) Find Default Abilities array
   - 12.2.1.2) Click + to add element
   - 12.2.1.3) Select: GA_FatherSword

#### 12.3) Save

##### 12.3.1) Save Configuration
   - 12.3.1.1) Click Save

---

## INTEGRATION SUMMARY

### Asset Creation Summary

| Asset | Type | Location |
|-------|------|----------|
| BP_FatherSwordWeapon | MeleeWeaponItem | Content/FatherCompanion/Weapons/ |
| BP_FatherSwordVisual | WeaponVisual | Content/FatherCompanion/Weapons/Visuals/ |
| GE_SwordState | GameplayEffect | Content/FatherCompanion/Effects/ |
| GA_FatherSword | NarrativeGameplayAbility | Content/FatherCompanion/Abilities/ |
| GA_FatherSwordAttack | GA_Attack_Combo_Melee | Content/FatherCompanion/Abilities/ |
| GA_FatherSwordDetach | NarrativeGameplayAbility | Content/FatherCompanion/Abilities/ |

### Tag Summary

| Tag | Purpose |
|-----|---------|
| Ability.Father.Sword | Form ability identifier |
| Ability.Father.Sword.Attack | Attack ability identifier |
| Ability.Father.Sword.Detach | Detach ability identifier |
| Father.Form.Sword | Form state tag (granted by GE) |
| Father.State.Wielded | State tag indicating weapon wielded |
| Narrative.Anim.AnimSets.Attacks.FatherSword.Light | Light attack animation set |
| Narrative.Anim.AnimSets.Attacks.FatherSword.Heavy | Heavy attack animation set |

### Ability Flow

| Step | Action | Result |
|------|--------|--------|
| 1 | Player activates GA_FatherSword | Form change initiated |
| 2 | Apply GE_SwordState to father | Father.Form.Sword granted |
| 3 | Hide father actor | Father visually disappears |
| 4 | Give BP_FatherSwordWeapon to player | Weapon in inventory |
| 5 | Equip and Wield weapon | Player holds sword |
| 6 | Attack ability available | Light/Heavy melee attacks |
| 7 | Player activates GA_FatherSwordDetach | Sword removed, father restored |

---

## RELATED DOCUMENTS

| Document | Purpose |
|----------|---------|
| Father_Companion_Technical_Reference_v4_5.md | EquippableItem mechanics, WeaponItem integration |
| Father_Companion_System_Design_Document_v1_6.md | Section 10: Weapon form specifications |
| GA_FatherRifle_Implementation_Guide_v1_1.md | Companion ranged weapon form |
| MeleeWeaponItem.h | Narrative Pro C++ source for melee base class |
| DefaultGameplayTags_FatherCompanion_v3_5.ini | Tag definitions |

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 1.4 | January 2026 | Updated Narrative Pro version reference from v2.1 to v2.2. |
| 1.3 | January 2026 | Simplified documentation: GE configuration (Section 7.3), tag configuration (Section 8.3.2), and variable creation (Section 8.4) converted to markdown tables. |
| 1.2 | January 2026 | Simplified PHASE 1: Replaced detailed tag navigation with consolidated tag tables. Tags now listed in Create Required Tags and Verify Existing Tags tables. Reduced PHASE 1 from 75 lines to 21 lines. |
| 1.1 | December 2025 | Added Related Documents table. Updated to Technical Reference v4.5 standards. |
| 1.0 | December 2025 | Initial implementation guide. Sword form using MeleeWeaponItem. Inherits from Narrative Pro GA_Attack_Combo_Melee for combat. GE_SwordState for form state management. Complete wield/unwield integration. Detach ability to restore father. |

---

**END OF IMPLEMENTATION GUIDE**

**GA_FatherSword VERSION 1.4 - Complete**

**Blueprint-Only Implementation for Unreal Engine 5.6 + Narrative Pro Plugin v2.2**
