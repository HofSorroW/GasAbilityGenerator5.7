# GA_FatherRifle - Rifle Form Implementation Guide
## Father Companion with Gameplay Ability System
## For Unreal Engine 5.6 + Narrative Pro Plugin v2.2

**Version:** 1.5 - Form State Tag Update (INV-1 Compliant)
**Date:** January 2026
**Engine:** Unreal Engine 5.6
**Plugin:** Narrative Pro v2.2
**Implementation:** Blueprint Only
**Parent Class:** RangedWeaponItem

---

## DOCUMENT PURPOSE

This guide provides step-by-step instructions for implementing the Rifle Form, where the father companion physically transforms into a ranged firearm weapon wielded by the player. This form leverages Narrative Pro's built-in RangedWeaponItem system for hitscan combat, aim-down-sights, spread mechanics, and recoil.

**Key Features:**

| Feature | Value |
|---------|-------|
| Weapon Type | Two-Hand Ranged Rifle |
| Base Class | RangedWeaponItem |
| Attack Type | Hitscan trace (inherited) |
| Fire Mode | Semi-Automatic |
| ADS Support | Aim-Down-Sights with FOV zoom |
| Ammo | Unlimited (father energy) |
| Visual | Father morphs into bio-organic rifle |

**Stat Bonuses:**

| Stat | Value | Method |
|------|-------|--------|
| Attack Rating | +50 | EquippableItem AttackRating |
| Attack Damage | 35.0 | WeaponItem AttackDamage |
| Armor Rating | 0.0 | EquippableItem ArmorRating |

---

## PREREQUISITES

Before implementing the Rifle Form, ensure the following are complete:

| Prerequisite | Type | Status |
|--------------|------|--------|
| BP_FatherCompanion | NarrativeNPCCharacter | Must exist |
| AC_FatherCompanion_Default | AbilityConfiguration | Must exist |
| GE_WeaponDamage | GameplayEffect | Narrative Pro built-in |
| GA_Attack_Firearm_Base | GameplayAbility | Narrative Pro built-in |
| GA_Weapon_Aim | GameplayAbility | Narrative Pro built-in |
| GE_EquipmentMod | GameplayEffect | Narrative Pro built-in |
| Player hand_r socket | Skeleton Socket | Standard hand socket |
| Player spine_03 socket | Skeleton Socket | Holster socket |

---

## QUICK REFERENCE

### Tag Configuration Summary

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Rifle |
| Equippable Slot | Narrative.Equipment.Slot.Weapon_Back |
| Form State Tag | Effect.Father.FormState.Rifle |
| State Tag | Father.State.Wielded |

### Multiplayer Settings

| Setting | Value |
|---------|-------|
| Instancing Policy | Instanced Per Actor |
| Replication Policy | Replicate Yes |
| Net Execution Policy | Local Predicted |

### Firearm Properties

| Property | Value |
|----------|-------|
| bAutomaticFire | FALSE (semi-auto) |
| Rate Of Fire | 0.25 seconds |
| Trace Distance | 15000.0 |
| Base Spread Degrees | 0.5 |
| Max Spread Degrees | 8.0 |
| AimFOVPct | 0.65 (35% zoom) |
| Requires Ammo | FALSE (unlimited) |

---

## PHASE 1: CONFIGURE GAMEPLAY TAGS

### Create Required Tags

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Rifle | Father rifle weapon form ability |
| Effect.Father.FormState.Rifle | Father is transformed into rifle weapon |
| Ability.Father.Rifle.Fire | Father rifle fire ability |
| Ability.Father.Rifle.Aim | Father rifle aim-down-sights ability |
| Ability.Father.Rifle.Detach | Detach rifle and restore father |
| Father.State.Wielded | State tag indicating weapon wielded |

### Verify Existing Tags

| Tag Name | Purpose |
|----------|---------|
| Narrative.Equipment.Slot.Weapon_Back | Equippable slot for weapon |
| Narrative.Input.AltAttack | Input for aim-down-sights |
| Narrative.Input.Father.Detach | Input for detach ability |

---

## PHASE 2: CREATE FATHER RIFLE WEAPON ITEM

### 4) Create BP_FatherRifleWeapon Blueprint

#### 4.1) Navigate to Weapons Folder

##### 4.1.1) Navigate
   - 4.1.1.1) In Content Browser: Navigate to Content/FatherCompanion/Weapons/
   - 4.1.1.2) Create folder if not exists

#### 4.2) Create RangedWeaponItem Blueprint

##### 4.2.1) Create New Blueprint
   - 4.2.1.1) Right-click in Content Browser
   - 4.2.1.2) Select Blueprint Class
   - 4.2.1.3) In Pick Parent Class dialog
   - 4.2.1.4) Search: RangedWeaponItem
   - 4.2.1.5) Select RangedWeaponItem
   - 4.2.1.6) Click Select

##### 4.2.2) Name Blueprint
   - 4.2.2.1) Type: BP_FatherRifleWeapon
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
   - 4.3.2.1.3) Enter: Father Rifle

###### 4.3.2.2) Description
   - 4.3.2.2.1) Locate Description property
   - 4.3.2.2.2) Enter: Transform your father companion into a bio-organic rifle. Fires high-energy projectiles with precision accuracy. Supports aim-down-sights for improved targeting.

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
   - 4.3.3.3.2) Enter: 50.0

###### 4.3.3.4) Armor Rating
   - 4.3.3.4.1) Locate Armor Rating property
   - 4.3.3.4.2) Enter: 0.0

###### 4.3.3.5) Stealth Rating
   - 4.3.3.5.1) Locate Stealth Rating property
   - 4.3.3.5.2) Enter: 0.0

#### 4.4) Configure Weapon Properties

##### 4.4.1) Attack Damage
   - 4.4.1.1) In Details panel, find Weapon category
   - 4.4.1.2) Locate Attack Damage property
   - 4.4.1.3) Enter: 35.0

##### 4.4.2) Weapon Visual Attach Bone
   - 4.4.2.1) Locate Weapon Visual Attach Bone property
   - 4.4.2.2) Enter: hand_r

##### 4.4.3) Weapon Visual Holstered Attach Bone
   - 4.4.3.1) Locate Weapon Visual Holstered Attach Bone property
   - 4.4.3.2) Enter: spine_03

##### 4.4.4) Required Ammo
   - 4.4.4.1) Locate Required Ammo property
   - 4.4.4.2) Leave: None (unlimited ammo)

##### 4.4.5) Clip Size
   - 4.4.5.1) Locate Clip Size property
   - 4.4.5.2) Enter: 999

#### 4.5) Configure Ranged Weapon Properties

##### 4.5.1) Find Ranged Weapon Category
   - 4.5.1.1) In Details, find Weapon | Config | Ranged Weapon category

##### 4.5.2) Configure Fire Mode

###### 4.5.2.1) Automatic Fire
   - 4.5.2.1.1) Locate bAutomaticFire property
   - 4.5.2.1.2) Uncheck: FALSE (semi-automatic)

###### 4.5.2.2) Rate Of Fire
   - 4.5.2.2.1) Locate Rate Of Fire property
   - 4.5.2.2.2) Enter: 0.25

##### 4.5.3) Configure Aim Settings

###### 4.5.3.1) Aim FOV Pct
   - 4.5.3.1.1) Locate Aim FOV Pct property
   - 4.5.3.1.2) Enter: 0.65

###### 4.5.3.2) Aim Weapon Render FOV
   - 4.5.3.2.1) Locate Aim Weapon Render FOV property
   - 4.5.3.2.2) Enter: 35.0

###### 4.5.3.3) Aim Weapon FStop
   - 4.5.3.3.1) Locate Aim Weapon FStop property
   - 4.5.3.3.2) Enter: 80.0

##### 4.5.4) Configure Spread

###### 4.5.4.1) Base Spread Degrees
   - 4.5.4.1.1) Locate Base Spread Degrees property
   - 4.5.4.1.2) Enter: 0.5

###### 4.5.4.2) Move Speed Add Degrees
   - 4.5.4.2.1) Locate Move Speed Add Degrees property
   - 4.5.4.2.2) Enter: 1.5

###### 4.5.4.3) Crouch Spread Multiplier
   - 4.5.4.3.1) Locate Crouch Spread Multiplier property
   - 4.5.4.3.2) Enter: 0.5

###### 4.5.4.4) Spread Fire Bump
   - 4.5.4.4.1) Locate Spread Fire Bump property
   - 4.5.4.4.2) Enter: 2.0

###### 4.5.4.5) Max Spread Degrees
   - 4.5.4.5.1) Locate Max Spread Degrees property
   - 4.5.4.5.2) Enter: 8.0

###### 4.5.4.6) Spread Decrease Speed
   - 4.5.4.6.1) Locate Spread Decrease Speed property
   - 4.5.4.6.2) Enter: 15.0

##### 4.5.5) Configure Recoil

###### 4.5.5.1) Recoil Impulse Translation Min
   - 4.5.5.1.1) Locate Recoil Impulse Translation Min
   - 4.5.5.1.2) X: -0.1, Y: -3.0, Z: -0.1

###### 4.5.5.2) Recoil Impulse Translation Max
   - 4.5.5.2.1) Locate Recoil Impulse Translation Max
   - 4.5.5.2.2) X: 0.1, Y: -2.0, Z: 0.1

###### 4.5.5.3) Hip Recoil Impulse Translation Min
   - 4.5.5.3.1) Locate Hip Recoil Impulse Translation Min
   - 4.5.5.3.2) X: -0.2, Y: -5.0, Z: -0.2

###### 4.5.5.4) Hip Recoil Impulse Translation Max
   - 4.5.5.4.1) Locate Hip Recoil Impulse Translation Max
   - 4.5.5.4.2) X: 0.2, Y: -3.0, Z: 0.2

#### 4.6) Configure Trace Data

##### 4.6.1) Find Trace Category
   - 4.6.1.1) In Details, find Weapon | Config | Trace category
   - 4.6.1.2) Expand Trace Data section

##### 4.6.2) Configure Trace Properties

###### 4.6.2.1) Trace Distance
   - 4.6.2.1.1) Locate Trace Distance property
   - 4.6.2.1.2) Enter: 15000.0

###### 4.6.2.2) Trace Radius
   - 4.6.2.2.1) Locate Trace Radius property
   - 4.6.2.2.2) Enter: 0.0

###### 4.6.2.3) Trace Multi
   - 4.6.2.3.1) Locate bTraceMulti property
   - 4.6.2.3.2) Uncheck: FALSE (single target)

#### 4.7) Configure Abilities Array

##### 4.7.1) Find Abilities Property
   - 4.7.1.1) In Details panel, find Abilities category
   - 4.7.1.2) Locate Abilities array

##### 4.7.2) Add Abilities
   - 4.7.2.1) Click + three times to create elements [0], [1], [2]
   - 4.7.2.2) Element [0]: GA_FatherRifle
   - 4.7.2.3) Element [1]: GA_FatherRifleFire
   - 4.7.2.4) Element [2]: GA_FatherRifleAim

#### 4.8) Compile and Save

##### 4.8.1) Compile Blueprint
   - 4.8.1.1) Click Compile in toolbar
   - 4.8.1.2) Verify no errors in Compiler Results

##### 4.8.2) Save Blueprint
   - 4.8.2.1) Click Save in toolbar

---

## PHASE 3: CREATE FATHER RIFLE WEAPON VISUAL

### 5) Create BP_FatherRifleVisual Blueprint

#### 5.1) Navigate to Visuals Folder

##### 5.1.1) Navigate
   - 5.1.1.1) In Content/FatherCompanion/Weapons/Visuals/
   - 5.1.1.2) Create folder if not exists

#### 5.2) Create FirearmWeaponVisual Blueprint

##### 5.2.1) Create New Blueprint
   - 5.2.1.1) Right-click in Visuals folder
   - 5.2.1.2) Select Blueprint Class
   - 5.2.1.3) Search: FirearmWeaponVisual
   - 5.2.1.4) Select FirearmWeaponVisual
   - 5.2.1.5) Click Select
   - 5.2.1.6) Name: BP_FatherRifleVisual

#### 5.3) Configure Visual Components

##### 5.3.1) Open Blueprint
   - 5.3.1.1) Double-click BP_FatherRifleVisual
   - 5.3.1.2) Navigate to Viewport tab

##### 5.3.2) Configure Skeletal Mesh
   - 5.3.2.1) Select WeaponMesh component in Components panel
   - 5.3.2.2) In Details panel, find Mesh category
   - 5.3.2.3) Set Skeletal Mesh: (use placeholder rifle mesh or father rifle mesh)

##### 5.3.3) Configure Transform
   - 5.3.3.1) Location: X=0, Y=0, Z=0
   - 5.3.3.2) Rotation: X=0, Y=0, Z=0
   - 5.3.3.3) Scale: X=1, Y=1, Z=1

##### 5.3.4) Configure ADS Socket
   - 5.3.4.1) Click Class Defaults
   - 5.3.4.2) In Details, find Character Visual category
   - 5.3.4.3) Locate ADS Sight Socket Name property
   - 5.3.4.4) Enter: ADS_Socket

#### 5.4) Compile and Save

##### 5.4.1) Compile and Save
   - 5.4.1.1) Click Compile
   - 5.4.1.2) Click Save
   - 5.4.1.3) Close Blueprint Editor

### 6) Link Visual to Weapon Item

#### 6.1) Open BP_FatherRifleWeapon

##### 6.1.1) Navigate and Open
   - 6.1.1.1) In Content Browser, navigate to Content/FatherCompanion/Weapons/
   - 6.1.1.2) Double-click BP_FatherRifleWeapon

#### 6.2) Configure Weapon Visual Class

##### 6.2.1) Set Visual Class
   - 6.2.1.1) Click Class Defaults
   - 6.2.1.2) In Details, find Weapon Visual category
   - 6.2.1.3) Locate Weapon Visual Class property
   - 6.2.1.4) Click dropdown
   - 6.2.1.5) Select: BP_FatherRifleVisual

#### 6.3) Compile and Save

##### 6.3.1) Save Changes
   - 6.3.1.1) Click Compile
   - 6.3.1.2) Click Save

---

## PHASE 4: CREATE FORM STATE GAMEPLAY EFFECT

### 7) Create GE_RifleState Gameplay Effect

#### 7.1) Navigate to Effects Folder

##### 7.1.1) Navigate
   - 7.1.1.1) In Content/FatherCompanion/Effects/
   - 7.1.1.2) Create folder if not exists

#### 7.2) Create Gameplay Effect Blueprint

##### 7.2.1) Create New Blueprint
   - 7.2.1.1) Right-click in Effects folder
   - 7.2.1.2) Select Blueprint Class
   - 7.2.1.3) Search: GameplayEffect
   - 7.2.1.4) Select GameplayEffect
   - 7.2.1.5) Click Select
   - 7.2.1.6) Name: GE_RifleState

#### 7.3) Configure GE_RifleState Properties

| Property | Value |
|----------|-------|
| Duration Policy | Infinite |
| Components | Grant Tags to Target Actor |
| Granted Tags [0] | Effect.Father.FormState.Rifle |
| Granted Tags [1] | Father.State.Wielded |
| Components | Remove Other Gameplay Effect on Application |
| Remove GE Query -> Any Tags Match | Father.Form |
| Components | Asset Tags |
| Asset Tags [0] | Effect.Father.FormState |
| Asset Tags [1] | Effect.Father.RifleState |

#### 7.4) Compile and Save

---

## PHASE 5: CREATE GA_FATHERRIFLE ABILITY

### 8) Create GA_FatherRifle Gameplay Ability

#### 8.1) Navigate to Abilities Folder

##### 8.1.1) Navigate
   - 8.1.1.1) In Content/FatherCompanion/Abilities/
   - 8.1.1.2) Create folder if not exists

#### 8.2) Create Ability Blueprint

##### 8.2.1) Create New Blueprint
   - 8.2.1.1) Right-click in Abilities folder
   - 8.2.1.2) Select Blueprint Class
   - 8.2.1.3) Search: NarrativeGameplayAbility
   - 8.2.1.4) Select NarrativeGameplayAbility
   - 8.2.1.5) Click Select
   - 8.2.1.6) Name: GA_FatherRifle

#### 8.3) Configure Ability Properties

##### 8.3.1) Open Blueprint and Class Defaults
   - 8.3.1.1) Double-click GA_FatherRifle
   - 8.3.1.2) Click Class Defaults

##### 8.3.2) Configure Tags Section

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Rifle |
| Activation Blocked Tags | Effect.Father.FormState.Rifle |

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
| RifleItemReference | BP_FatherRifleWeapon (Object Reference) | None | No |

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

##### 8.5.10) Apply GE_RifleState
   - 8.5.10.1) From Set PlayerReference execution pin
   - 8.5.10.2) Add Apply Gameplay Effect to Self node
   - 8.5.10.3) Target: Connect Father's ASC
   - 8.5.10.4) Gameplay Effect Class: GE_RifleState
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
   - 8.5.13.6) Item Class: BP_FatherRifleWeapon
   - 8.5.13.7) Quantity: 1

##### 8.5.14) Store Rifle Reference
   - 8.5.14.1) From Give Item Return Value
   - 8.5.14.2) Cast to BP_FatherRifleWeapon
   - 8.5.14.3) Set RifleItemReference

##### 8.5.15) Equip Weapon
   - 8.5.15.1) From RifleItemReference
   - 8.5.15.2) Drag wire to right
   - 8.5.15.3) Search: Equip Item
   - 8.5.15.4) Connect execution

##### 8.5.16) Wield Weapon
   - 8.5.16.1) From Equip Item execution pin
   - 8.5.16.2) From PlayerReference
   - 8.5.16.3) Search: Wield Weapon
   - 8.5.16.4) Connect player to Target
   - 8.5.16.5) Weapon: RifleItemReference

##### 8.5.17) Play Transform Animation
   - 8.5.17.1) From Wield Weapon execution pin
   - 8.5.17.2) Add Play Montage node
   - 8.5.17.3) Target: PlayerReference -> Get Mesh
   - 8.5.17.4) Montage to Play: AM_FatherRifleTransform

##### 8.5.18) End Ability
   - 8.5.18.1) From Play Montage -> On Completed pin
   - 8.5.18.2) Add End Ability node
   - 8.5.18.3) Was Cancelled: FALSE

#### 8.6) Compile and Save

##### 8.6.1) Compile Blueprint
   - 8.6.1.1) Click Compile
   - 8.6.1.2) Verify no errors

##### 8.6.2) Save Blueprint
   - 8.6.2.1) Click Save
   - 8.6.2.2) Close Blueprint Editor

---

## PHASE 6: CREATE RIFLE FIRE ABILITY

### 9) Create GA_FatherRifleFire Ability

#### 9.1) Create Ability Blueprint

##### 9.1.1) Create New Blueprint
   - 9.1.1.1) In Content/FatherCompanion/Abilities/
   - 9.1.1.2) Right-click -> Blueprint Class
   - 9.1.1.3) Search parent: GA_Attack_Firearm_Base
   - 9.1.1.4) Select GA_Attack_Firearm_Base
   - 9.1.1.5) Name: GA_FatherRifleFire

#### 9.2) Configure Ability Properties

##### 9.2.1) Open Class Defaults
   - 9.2.1.1) Double-click GA_FatherRifleFire
   - 9.2.1.2) Click Class Defaults

##### 9.2.2) Configure Tags

###### 9.2.2.1) Ability Tags
   - 9.2.2.1.1) Add: Ability.Father.Rifle.Fire

###### 9.2.2.2) Activation Required Tags
   - 9.2.2.2.1) Add: Effect.Father.FormState.Rifle

##### 9.2.3) Configure Input Tag
   - 9.2.3.1) Input Tag: Narrative.Input.Attack

##### 9.2.4) Configure Firearm Properties

###### 9.2.4.1) Is Automatic
   - 9.2.4.1.1) Locate Is Automatic property
   - 9.2.4.1.2) Uncheck: FALSE

###### 9.2.4.2) Rate Of Fire
   - 9.2.4.2.1) Locate Rate Of Fire property
   - 9.2.4.2.2) Enter: 0.25

###### 9.2.4.3) Burst Amount
   - 9.2.4.3.1) Locate Burst Amount property
   - 9.2.4.3.2) Enter: -1 (no burst limit)

##### 9.2.5) Configure Trace

###### 9.2.5.1) Trace Distance
   - 9.2.5.1.1) Locate Trace Distance property
   - 9.2.5.1.2) Enter: 15000.0

###### 9.2.5.2) Trace Radius
   - 9.2.5.2.1) Locate Trace Radius property
   - 9.2.5.2.2) Enter: 0.0

##### 9.2.6) Configure Damage

###### 9.2.6.1) Damage Effect Class
   - 9.2.6.1.1) Locate Damage Effect Class property
   - 9.2.6.1.2) Select: GE_WeaponDamage

###### 9.2.6.2) Default Attack Damage
   - 9.2.6.2.1) Locate Default Attack Damage property
   - 9.2.6.2.2) Enter: 35.0

##### 9.2.7) Configure Ammo

###### 9.2.7.1) Requires Ammo
   - 9.2.7.1.1) Locate Requires Ammo property
   - 9.2.7.1.2) Uncheck: FALSE (unlimited ammo)

#### 9.3) Compile and Save

##### 9.3.1) Save
   - 9.3.1.1) Click Compile
   - 9.3.1.2) Click Save

---

## PHASE 7: CREATE RIFLE AIM ABILITY

### 10) Create GA_FatherRifleAim Ability

#### 10.1) Create Ability Blueprint

##### 10.1.1) Create New Blueprint
   - 10.1.1.1) In Content/FatherCompanion/Abilities/
   - 10.1.1.2) Right-click -> Blueprint Class
   - 10.1.1.3) Search parent: GA_Weapon_Aim
   - 10.1.1.4) Select GA_Weapon_Aim
   - 10.1.1.5) Name: GA_FatherRifleAim

#### 10.2) Configure Ability Properties

##### 10.2.1) Open Class Defaults
   - 10.2.1.1) Double-click GA_FatherRifleAim
   - 10.2.1.2) Click Class Defaults

##### 10.2.2) Configure Tags

###### 10.2.2.1) Ability Tags
   - 10.2.2.1.1) Add: Ability.Father.Rifle.Aim

###### 10.2.2.2) Activation Required Tags
   - 10.2.2.2.1) Add: Effect.Father.FormState.Rifle

##### 10.2.3) Configure Input Tag
   - 10.2.3.1) Input Tag: Narrative.Input.AltAttack

#### 10.3) Compile and Save

##### 10.3.1) Save
   - 10.3.1.1) Click Compile
   - 10.3.1.2) Click Save

---

## PHASE 8: CREATE DETACH LOGIC

### 11) Create GA_FatherRifleDetach Ability

#### 11.1) Create Ability Blueprint

##### 11.1.1) Create New Blueprint
   - 11.1.1.1) In Content/FatherCompanion/Abilities/
   - 11.1.1.2) Right-click -> Blueprint Class
   - 11.1.1.3) Parent: NarrativeGameplayAbility
   - 11.1.1.4) Name: GA_FatherRifleDetach

#### 11.2) Configure Properties

##### 11.2.1) Tags
   - 11.2.1.1) Ability Tags: Ability.Father.Rifle.Detach
   - 11.2.1.2) Activation Required Tags: Effect.Father.FormState.Rifle

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
   - 11.3.3.4) Remove Item (BP_FatherRifleWeapon instance)

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
   - 11.3.9.1) Play Montage: AM_FatherRifleDetach

##### 11.3.10) End Ability
   - 11.3.10.1) From On Completed pin
   - 11.3.10.2) End Ability

#### 11.4) Compile and Save

##### 11.4.1) Save
   - 11.4.1.1) Click Compile
   - 11.4.1.2) Click Save

---

## PHASE 9: UPDATE WEAPON ITEM WITH ALL ABILITIES

### 12) Final Weapon Configuration

#### 12.1) Open BP_FatherRifleWeapon

##### 12.1.1) Navigate and Open
   - 12.1.1.1) In Content Browser -> Content/FatherCompanion/Weapons/
   - 12.1.1.2) Double-click BP_FatherRifleWeapon

#### 12.2) Verify Abilities Array

##### 12.2.1) Check Abilities
   - 12.2.1.1) Click Class Defaults
   - 12.2.1.2) Find Abilities array
   - 12.2.1.3) Verify elements:

| Index | Ability |
|-------|---------|
| [0] | GA_FatherRifle |
| [1] | GA_FatherRifleFire |
| [2] | GA_FatherRifleAim |
| [3] | GA_FatherRifleDetach |

##### 12.2.2) Add Detach If Missing
   - 12.2.2.1) Click + to add element [3]
   - 12.2.2.2) Select: GA_FatherRifleDetach

#### 12.3) Compile and Save

##### 12.3.1) Save
   - 12.3.1.1) Click Compile
   - 12.3.1.2) Click Save

---

## PHASE 10: UPDATE ABILITYCONFIGURATION

### 13) Add Rifle Ability to Baseline

#### 13.1) Open AC_FatherCompanion_Default

##### 13.1.1) Navigate and Open
   - 13.1.1.1) In Content Browser -> Content/FatherCompanion/Configurations/
   - 13.1.1.2) Double-click AC_FatherCompanion_Default

#### 13.2) Add GA_FatherRifle to Default Abilities

##### 13.2.1) Add New Element
   - 13.2.1.1) Find Default Abilities array
   - 13.2.1.2) Click + to add element
   - 13.2.1.3) Select: GA_FatherRifle

#### 13.3) Save

##### 13.3.1) Save Configuration
   - 13.3.1.1) Click Save

---

## INTEGRATION SUMMARY

### Asset Creation Summary

| Asset | Type | Location |
|-------|------|----------|
| BP_FatherRifleWeapon | RangedWeaponItem | Content/FatherCompanion/Weapons/ |
| BP_FatherRifleVisual | FirearmWeaponVisual | Content/FatherCompanion/Weapons/Visuals/ |
| GE_RifleState | GameplayEffect | Content/FatherCompanion/Effects/ |
| GA_FatherRifle | NarrativeGameplayAbility | Content/FatherCompanion/Abilities/ |
| GA_FatherRifleFire | GA_Attack_Firearm_Base | Content/FatherCompanion/Abilities/ |
| GA_FatherRifleAim | GA_Weapon_Aim | Content/FatherCompanion/Abilities/ |
| GA_FatherRifleDetach | NarrativeGameplayAbility | Content/FatherCompanion/Abilities/ |

### Tag Summary

| Tag | Purpose |
|-----|---------|
| Ability.Father.Rifle | Form ability identifier |
| Ability.Father.Rifle.Fire | Fire ability identifier |
| Ability.Father.Rifle.Aim | Aim ability identifier |
| Ability.Father.Rifle.Detach | Detach ability identifier |
| Effect.Father.FormState.Rifle | Form state tag (granted by GE) |
| Father.State.Wielded | State tag indicating weapon wielded |

### Ability Flow

| Step | Action | Result |
|------|--------|--------|
| 1 | Player activates GA_FatherRifle | Form change initiated |
| 2 | Apply GE_RifleState to father | Effect.Father.FormState.Rifle granted |
| 3 | Hide father actor | Father visually disappears |
| 4 | Give BP_FatherRifleWeapon to player | Weapon in inventory |
| 5 | Equip and Wield weapon | Player holds rifle |
| 6 | Fire ability available | LMB shoots |
| 7 | Aim ability available | RMB aims down sights |
| 8 | Player activates GA_FatherRifleDetach | Rifle removed, father restored |

### Inherited Firearm Features

| Feature | Source | Description |
|---------|--------|-------------|
| Hitscan Trace | RangedWeaponItem | Instant hit detection |
| Spread System | RangedWeaponItem | Dynamic accuracy |
| Recoil | RangedWeaponItem | Camera kick on fire |
| ADS | GA_Weapon_Aim | FOV zoom, reduced spread |
| Damage | GE_WeaponDamage | SetByCaller damage application |
| Gameplay Cues | NarrativeCombatAbility | Fire and impact effects |

---

## RELATED DOCUMENTS

| Document | Purpose |
|----------|---------|
| Father_Companion_Technical_Reference_v4_5.md | EquippableItem mechanics, WeaponItem integration |
| Father_Companion_System_Design_Document_v1_6.md | Section 10: Weapon form specifications |
| GA_FatherSword_Implementation_Guide_v1_1.md | Companion melee weapon form |
| RangedWeaponItem.h | Narrative Pro C++ source for firearm base class |
| DefaultGameplayTags_FatherCompanion_v3_5.ini | Tag definitions |

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 1.4 | January 2026 | Updated Narrative Pro version reference from v2.1 to v2.2. |
| 1.3 | January 2026 | Simplified documentation: GE configuration (Section 7.3), tag configuration (Section 8.3.2), and variable creation (Section 8.4) converted to markdown tables. |
| 1.2 | January 2026 | Simplified PHASE 1 tag creation - replaced detailed step-by-step instructions with simple tag list tables. |
| 1.1 | December 2025 | Added Related Documents table. Updated to Technical Reference v4.5 standards. |
| 1.0 | December 2025 | Initial implementation guide. Rifle form using RangedWeaponItem. Inherits from Narrative Pro GA_Attack_Firearm_Base for combat. GE_RifleState for form state management. Includes GA_Weapon_Aim for ADS. Complete wield/unwield integration. Unlimited ammo (father energy). Detach ability to restore father. |

---

**END OF IMPLEMENTATION GUIDE**

**GA_FatherRifle VERSION 1.4 - Complete**

**Blueprint-Only Implementation for Unreal Engine 5.6 + Narrative Pro Plugin v2.2**
