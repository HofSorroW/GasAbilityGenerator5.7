# Father Companion - Symbiote Ultimate Ability Implementation Guide
## VERSION 4.9 - C_SYMBIOTE_STRICT_CANCEL Contract (LOCKED)
## For Unreal Engine 5.7 + Narrative Pro Plugin v2.2

**Version:** 4.9
**Date:** January 2026
**Last Audit:** 2026-01-23 (Claude-GPT dual audit - C_SYMBIOTE_STRICT_CANCEL contract)
**Engine:** Unreal Engine 5.7
**Plugin:** Narrative Pro v2.2
**Implementation:** Blueprint Only
**Parent Class:** NarrativeGameplayAbility
**Architecture:** Option B (GE-Based Form Identity) - See Form_State_Architecture_Fix_v4.13.2.md
**Contract:** LOCKED_CONTRACTS.md Contract 11 - C_SYMBIOTE_STRICT_CANCEL

---

## **AUTHORITY NOTE**

This document is explanatory. Runtime behavior and generation are defined by `manifest.yaml`. In case of conflict, the manifest takes precedence.

---

## **AUDIT STATUS**

| Field | Value |
|-------|-------|
| Status | VERIFIED |
| Last Audit Date | 2026-01-23 |
| Audit Scope | Design / Guide / Manifest Consistency |
| Verified Against | manifest.yaml, GAS Audit Locked Decisions v4.1 |
| Auditors | Claude-GPT dual audit |
| Notes | INC-2 (version mismatch), INC-3 (cooldown clarity), INC-4 (Delay wording) resolved |

---

**Document Purpose**: Complete step-by-step guide for implementing the ultimate Symbiote ability for the father companion system using Narrative Pro v2.2 and Unreal Engine 5.7. This ability transforms the father into a full-body symbiote covering the player, providing massive stat boosts for 30 seconds after accumulating damage equal to the level-scaled threshold.

**System Overview**: The Symbiote ability is a powerful ultimate that requires charging through combat. The player and father together must deal damage equal to the threshold (scaled by player level via FC_UltimateThreshold curve) to charge the ability. Player damage counts 100%, father damage counts 50%. Once charged, activating the ability via Form Wheel (Z key) covers the player in a full-body father symbiote for 30 seconds, granting +50% movement speed, +30% jump height, +100 Attack Rating, infinite stamina, and a proximity aura that damages nearby enemies (40 damage per 0.5s within 350 units). During the 30-second duration, the player cannot switch forms (Z wheel locked via Father.State.SymbioteLocked tag). After duration expires, the father auto-returns to Armor form. After use, the ability enters a 120-second cooldown. Charge resets on player death.

---

## **DOCUMENT INFORMATION**

| Field | Value |
|-------|-------|
| Ability Name | GA_FatherSymbiote |
| Ability Type | Ultimate Form Ability |
| Parent Class | NarrativeGameplayAbility |
| Form | Symbiote |
| Input | Z Key (Form Wheel) when charged |
| Charge Component | UltimateChargeComponent |
| Threshold Scaling | FC_UltimateThreshold (CurveFloat) |
| Version | 4.8 |
| Last Updated | January 2026 |
| Dependencies | GE_SymbioteState, GE_FormChangeCooldown, GE_SymbioteLock |

---

## **AUTOMATION VS MANUAL**

| Feature | Automation Status | Notes |
|---------|-------------------|-------|
| GE_SymbioteState definition | ✅ Auto-generated | manifest.yaml gameplay_effects section |
| GA_FatherSymbiote blueprint | ✅ Auto-generated | manifest.yaml gameplay_abilities section |
| Activation tags config | ✅ Auto-generated | Required/Blocked tags in manifest |
| Transition prelude nodes | ✅ Auto-generated | Remove old state GE, apply new state GE (in manifest) |
| Merge/Hide logic | ⚠️ Manual | SetActorHiddenInGame, collision disable |
| VFX spawning | ⚠️ Manual | GameplayCues preferred (Category C roadmap) |
| Duration timer | ⚠️ Manual | 30s timer with EndSymbiote callback |
| EndAbility cleanup | ⚠️ Manual | Speed restore, state reset, return to Armor |

---

## **PREREQUISITES**

### **Required Guides (Complete Before This Guide)**

| Guide | Purpose |
|-------|---------|
| WBP_UltimatePanel_Implementation_Guide_v1_0.md | Ultimate charge HUD system |
| GE_FormChangeCooldown_Implementation_Guide_v1_0.md | Form change cooldown effect |
| Father_Companion_System_Setup_Guide_v2_0.md | Father character setup |

### **Required Assets**

| Asset | Purpose |
|-------|---------|
| UltimateChargeComponent | Shared charge component (created in WBP_UltimatePanel guide) |
| FC_UltimateThreshold | Float curve for level-based threshold scaling |
| BP_FatherCompanion | Father character with NarrativeNPCCharacter parent |
| GA_ProximityStrike | Proximity damage ability granted to player |
| ReplicateActivationOwnedTags | ENABLED in Project Settings |

---

## **TABLE OF CONTENTS**

1. PHASE 1: Gameplay Tags Setup
2. PHASE 2: Create Gameplay Effects
3. PHASE 3: Create GA_FatherSymbiote Ability
4. PHASE 3A: Form Switch with Transition Animation
5. PHASE 4: Implement EndSymbiote Function
6. PHASE 5: Implement EndAbility Event
7. PHASE 6: Player Character Integration
8. PHASE 7: Input Configuration
9. Quick Reference
10. Changelog

---

## **PHASE 1: GAMEPLAY TAGS SETUP**

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Ability.Father.Symbiote | Father Symbiote ultimate ability |
| Effect.Father.FormState.Symbiote | Symbiote form identity tag (granted by GE_SymbioteState) |
| Symbiote.State.Merged | Father completely merged with player body (Activation Owned Tag) |
| Symbiote.Charge.Ready | Ultimate charge is full, ability can activate |
| Cooldown.Father.Symbiote | Reserved for Symbiote-specific cooldown (see Cooldown Model below) |
| Father.State.SymbioteLocked | Form changes blocked during 30s Symbiote duration |

### **Cooldown Model (INC-3 Clarification)**

Symbiote has a dual-cooldown system:

| Cooldown Type | Mechanism | Duration | Tag |
|---------------|-----------|----------|-----|
| Form Change Cooldown | GE_FormChangeCooldown (GAS) | 15s | Cooldown.Father.FormChange |
| Post-Duration Cooldown | UltimateChargeComponent::StartCooldown() (non-GAS) | 120s | N/A (component-based) |

**Note:** The `Cooldown.Father.Symbiote` tag is reserved but not currently applied by GE_FormChangeCooldown. The 120s cooldown is managed entirely by the UltimateChargeComponent, not the GAS cooldown system. This is intentional - the charge-based ultimate uses component state rather than GAS tags.

### **Verify Existing Tags**

| Tag Name | Purpose |
|----------|---------|
| Father.State.Alive | Required for activation |
| Father.State.Recruited | Recruitment state (Activation Required) |
| Father.State.Transitioning | Blocks during 5s VFX |
| Father.State.Dormant | Blocks activation |

**Note (v4.4):** `Father.Form.*` tags are **orphan tags** - no persistent GE grants them. Form identity uses `Effect.Father.FormState.*` tags granted by infinite-duration GE_*State effects. Do NOT use Father.Form.* for activation gating.

---

## **PHASE 2: CREATE GAMEPLAY EFFECTS**

### **6A) Verify GE_SymbioteState Exists (Option B Form Identity)**

#### 6A.1) GE_SymbioteState Purpose
This Gameplay Effect establishes Symbiote form identity using Option B architecture:
- **Duration Policy:** Infinite (persists until explicitly removed by next form's transition prelude)
- **Granted Tags:** Effect.Father.FormState.Symbiote
- **Asset Tag:** Effect.Father.FormState.Symbiote

> **AUDIT NOTE (v4.5):** Invulnerability was removed per GAS audit. Father is NOT invulnerable during Symbiote form - this was an unintended oversight. See `Father_Companion_GAS_Audit_Locked_Decisions.md` INV-1.

#### 6A.2) Verify Asset Location
   - 6A.2.1) Navigate to: /Content/FatherCompanion/Effects/FormState/
   - 6A.2.2) Verify GE_SymbioteState exists
   - 6A.2.3) If missing, generate from manifest using GasAbilityGenerator

#### 6A.3) GE_SymbioteState Properties

| Property | Value |
|----------|-------|
| Duration Policy | Infinite |
| Granted Tags [0] | Effect.Father.FormState.Symbiote |
| Asset Tag | Effect.Father.FormState.Symbiote |

---

### **7) Stat Bonuses (Handled by EquippableItem)**

Attack Rating (+100) and Infinite Stamina are handled automatically by BP_FatherSymbioteForm EquippableItem via GE_EquipmentModifier_FatherSymbiote.

| Stat | Value | Handled By |
|------|-------|------------|
| Attack Rating | +100 | GE_EquipmentModifier_FatherSymbiote (inherited SetByCaller.AttackRating) |
| Stamina Regen | Override 10000 | GE_EquipmentModifier_FatherSymbiote (custom modifier [3]) |

GA_FatherSymbiote does NOT need to apply stat modifiers. See Father_Companion_System_Setup_Guide PHASE 30 and PHASE 33 for GE and EquippableItem configuration.

### **8) Create GE_SymbioteLock Effect**

#### 8.1) Create Gameplay Effect
   - 8.1.1) Right-click in Effects folder
   - 8.1.2) Select Blueprint Class
   - 8.1.3) Select GameplayEffect as parent
   - 8.1.4) Name: GE_SymbioteLock
   - 8.1.5) Double-click to open

#### 8.2) Configure GE_SymbioteLock Properties

| Property | Value |
|----------|-------|
| Duration Policy | Has Duration |
| Duration Magnitude -> Scalable Float Magnitude | 30.0 |
| Components | Grant Tags to Target Actor |
| Add Tags -> Add to Inherited [0] | Father.State.SymbioteLocked |

#### 8.3) Compile and Save

---

## **PHASE 2B: GE_SYMBIOTE_DURATION SETBYCALLER EFFECT (C_SYMBIOTE_STRICT_CANCEL)**

> **AUDIT NOTE (v4.9 - 2026-01-23):** This section implements LOCKED_CONTRACTS.md Contract 11 - C_SYMBIOTE_STRICT_CANCEL. GE_SymbioteDuration is applied at activation START using SetByCaller pattern, granting `Father.State.SymbioteLocked` for 30 seconds. This blocks all player-initiated form/weapon abilities during Symbiote.

### **8A) GE_SymbioteDuration Purpose**

GE_SymbioteDuration establishes the 30-second form lock using SetByCaller duration:
- **Duration Policy:** Has Duration (SetByCaller)
- **SetByCaller Tag:** `Data.Symbiote.Duration`
- **Granted Tags:** `Father.State.SymbioteLocked`

### **8B) GE_SymbioteDuration Properties**

| Property | Value |
|----------|-------|
| Duration Policy | Has Duration |
| Duration Magnitude | SetByCaller (Data.Symbiote.Duration) |
| Granted Tags [0] | Father.State.SymbioteLocked |

### **8C) SetByCaller Application Flow (Event Graph)**

The SetByCaller chain is applied at the START of Event_Activate, BEFORE any other Symbiote logic:

| Step | Node | Purpose |
|------|------|---------|
| 1 | MakeOutgoingGameplayEffectSpec | Create spec for GE_SymbioteDuration |
| 2 | MakeLiteralGameplayTag | Create Data.Symbiote.Duration tag |
| 3 | AssignTagSetByCallerMagnitude | Set duration to 30.0 seconds |
| 4 | K2_ApplyGameplayEffectSpecToOwner | Apply spec to self (ability owner) |

### **8D) Defense-in-Depth Strategy**

| Layer | Mechanism | Purpose |
|-------|-----------|---------|
| Layer 1 | `Father.State.SymbioteLocked` in activation_blocked_tags | Blocks ability activation |
| Layer 2 | Symbiote NOT in cancel_abilities_with_tag | Abilities won't forcibly cancel Symbiote |
| Layer 3 | GE_SymbioteDuration auto-expires | Lock tag automatically removed after 30s |

**Exception:** GA_FatherSacrifice MAY cancel Symbiote (emergency auto-trigger when player HP < 15%).

---

## **PHASE 3: CREATE GA_FATHERSYMBIOTE ABILITY**

### **9) Create Ability Blueprint**

#### 9.1) Navigate to Abilities Folder
   - 9.1.1) Content Browser: /Content/FatherCompanion/Symbiote/Abilities/
   - 9.1.2) Create folder if needed

#### 9.2) Create Gameplay Ability
   - 9.2.1) Right-click in Content Browser
   - 9.2.2) Select Blueprint Class
   - 9.2.3) Search: NarrativeGameplayAbility
   - 9.2.4) Select NarrativeGameplayAbility as parent
   - 9.2.5) Name: GA_FatherSymbiote
   - 9.2.6) Double-click to open

### **10) Configure Ability Properties**

#### 10.1) Open Class Defaults
   - 10.1.1) Click Class Defaults button in toolbar

#### 10.2) Configure Ability Tags

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Symbiote |
| Cancel Abilities with Tag | Ability.Father.Crawler, Ability.Father.Armor, Ability.Father.Exoskeleton, Ability.Father.Engineer |
| Activation Required Tags | Father.State.Alive, Father.State.Recruited, Symbiote.Charge.Ready |
| Activation Blocked Tags | Father.State.Dormant, Father.State.Transitioning, Father.State.SymbioteLocked, Cooldown.Father.FormChange |
| Activation Owned Tags | Symbiote.State.Merged |

**Note (v4.4):** Form identity (`Effect.Father.FormState.Symbiote`) is NOT in Activation Owned Tags. It persists via `GE_SymbioteState` (Option B architecture). Only ephemeral state tags (`Symbiote.State.Merged`) belong in Activation Owned Tags.

#### 10.3) Configure Cooldown (Built-in System)
   - 10.3.1) Find: **Cooldown** section in Class Defaults
   - 10.3.2) Locate: **Cooldown Gameplay Effect Class** property
   - 10.3.3) Click dropdown and select: `GE_FormChangeCooldown`

#### 10.4) Set Net Execution Policy
   - 10.4.1) Find: Net Execution Policy dropdown
   - 10.4.2) Select: Server Only

#### 10.5) Set Instancing Policy
   - 10.5.1) Find: Instancing Policy dropdown
   - 10.5.2) Select: Instanced Per Actor

#### 10.6) Set Input Tag
   - 10.6.1) Find: Input Tag property
   - 10.6.2) Click: Tag dropdown
   - 10.6.3) Search: Narrative.Input.Father.FormChange
   - 10.6.4) Select: Narrative.Input.Father.FormChange

### **11) Create Ability Variables**

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| OriginalMaxWalkSpeed | Float | 0.0 | No |
| OriginalJumpZVelocity | Float | 0.0 | No |
| DurationTimerHandle | TimerHandle | None | No |
| MovementSpeedMultiplier | Float | 1.5 | Yes |
| JumpHeightMultiplier | Float | 1.3 | Yes |
| AbilityDuration | Float | 30.0 | Yes |
| bIsFirstActivation | Boolean | True | No |
| ChargeComponentRef | UltimateChargeComponent (Object Reference) | None | No |
| FatherRef | BP_FatherCompanion (Object Reference) | None | No |
| OwnerCharRef | Character (Object Reference) | None | No |
| SymbioteLockHandle | Active Gameplay Effect Handle | None | No |
| ProximityAbilityHandle | Gameplay Ability Spec Handle | None | No |

### **12) Implement CanActivateAbility Check**

#### 12.1) Override CanActivateAbility
   - 12.1.1) My Blueprint -> Functions: Find Overrides
   - 12.1.2) Click: Override dropdown
   - 12.1.3) Search: Can Activate Ability
   - 12.1.4) Select: CanActivateAbility function

#### 12.2) Open Function Graph
   - 12.2.1) Double-click: CanActivateAbility function
   - 12.2.2) Entry node: Has Handle, Actor Info, Trigger Data inputs
   - 12.2.3) Must return: Boolean

#### 12.3) Call Parent CanActivateAbility
   - 12.3.1) From function entry:
      - 12.3.1.1) Right-click: Search Parent: Can Activate Ability
      - 12.3.1.2) Add: Parent: CanActivateAbility node
      - 12.3.1.3) Connect: Entry inputs to parent inputs

#### 12.4) Check Charge Component Exists
   - 12.4.1) From parent call execution:
      - 12.4.1.1) Search: Get Owner Actor
      - 12.4.1.2) Add: Get Owner Actor node (from Actor Info)

##### 12.4.2) Get Component
   - 12.4.2.1) From Owner Actor:
   - 12.4.2.2) Search: Get Component By Class
   - 12.4.2.3) Add: Get Component By Class node
   - 12.4.2.4) Component Class: UltimateChargeComponent

##### 12.4.3) Validate Component
   - 12.4.3.1) From Get Component return:
   - 12.4.3.2) Search: Is Valid
   - 12.4.3.3) Add: Is Valid node
   - 12.4.3.4) Input Object: Component return

#### 12.5) Check Charge Ready
   - 12.5.1) From Is Valid -> Is Valid execution:
      - 12.5.1.1) Promote component to local variable: ChargeComp
   - 12.5.2) From ChargeComp:
      - 12.5.2.1) Search: Is Charge Ready
      - 12.5.2.2) Add: IsChargeReady function call
      - 12.5.2.3) Returns Boolean indicating charge >= threshold

#### 12.6) Check Cooldown Status
   - 12.6.1) From ChargeComp:
      - 12.6.1.1) Search: Get b In Cooldown
      - 12.6.1.2) Add: GET bInCooldown node
      - 12.6.1.3) Add: NOT Boolean node
      - 12.6.1.4) Connect: bInCooldown -> NOT input

#### 12.7) Combine Checks
   - 12.7.1) Add: AND Boolean node
   - 12.7.2) A input: Parent result
   - 12.7.3) B input: Component Is Valid
   - 12.7.4) Add: AND Boolean node
   - 12.7.5) A input: Previous AND result
   - 12.7.6) B input: IsChargeReady result
   - 12.7.7) Add: AND Boolean node
   - 12.7.8) A input: Previous AND result
   - 12.7.9) B input: NOT in cooldown

#### 12.8) Return Result
   - 12.8.1) Add: Return Node
   - 12.8.2) Connect: Final AND result -> Return Value

### **13) Implement ActivateAbility Event**

#### 13.1) Switch to Event Graph
   - 13.1.1) Click: Event Graph tab

#### 13.2) Add Event ActivateAbility
   - 13.2.1) Right-click: In graph
   - 13.2.2) Search: Event Activate Ability
   - 13.2.3) Select: Event ActivateAbility

#### 13.3) Check Server Authority
   - 13.3.1) From event execution:
      - 13.3.1.1) Search: Has Authority
      - 13.3.1.2) Add: Has Authority node
      - 13.3.1.3) Add: Branch node
      - 13.3.1.4) Condition: Has Authority result
      - 13.3.1.5) From False: Connect to End Ability
      - 13.3.1.6) Was Cancelled: True

#### 13.4) Get Owner Character
   - 13.4.1) From Branch True execution:
      - 13.4.1.1) Search: Get Owner Actor
      - 13.4.1.2) Add: Get Owner Actor node
   - 13.4.2) From Owner Actor:
      - 13.4.2.1) Search: Cast To Character
      - 13.4.2.2) Add: Cast To Character node
      - 13.4.2.3) Connect: Owner -> Cast Object
   - 13.4.3) From Cast success:
      - 13.4.3.1) Add: SET OwnerCharRef node
      - 13.4.3.2) Connect cast output to value

#### 13.5) Get Charge Component Reference
   - 13.5.1) From SET OwnerCharRef execution:
      - 13.5.1.1) From OwnerCharRef, search: Get Component By Class
      - 13.5.1.2) Add: Get Component By Class node
      - 13.5.1.3) Component Class: UltimateChargeComponent
   - 13.5.2) Add: SET ChargeComponentRef node
   - 13.5.3) Connect component output to value

#### 13.6) Get Father Reference
   - 13.6.1) From SET ChargeComponentRef execution:
      - 13.6.1.1) From OwnerCharRef, search: Get Father Companion Ref
      - 13.6.1.2) Add: GET FatherCompanionRef node
   - 13.6.2) From FatherCompanionRef:
      - 13.6.2.1) Add: Cast To BP_FatherCompanion node
   - 13.6.3) From Cast success:
      - 13.6.3.1) Add: SET FatherRef node

#### 13.7) Branch: Check If First Activation
   - 13.7.1) From SET FatherRef execution:
      - 13.7.1.1) Drag bIsFirstActivation variable into graph (GET)
      - 13.7.1.2) Add: Branch node
      - 13.7.1.3) Connect bIsFirstActivation output -> Branch Condition pin
   - 13.7.2) True path: Continues to Section 14 (Initial Spawn)
   - 13.7.3) False path: Continues to PHASE 3A (Form Switch with Transition)

### **14) Initial Spawn Path (First Activation)**

#### 14.1) Set bIsFirstActivation to False
   - 14.1.1) From Branch True execution:
      - 14.1.1.1) Add: SET bIsFirstActivation node
      - 14.1.1.2) Value: Unchecked (false)

#### 14.2) Continue to Main Symbiote Logic
   - 14.2.1) From SET bIsFirstActivation:
      - 14.2.1.1) Connect to Section 16 (Main Symbiote Logic)

---

## **PHASE 3A: FORM SWITCH WITH TRANSITION ANIMATION**

### **15) Form Switch Path (Not First Activation)**

#### 15.1) Add Transitioning Tag
   - 15.1.1) From Branch False execution (step 13.7.3):
      - 15.1.1.1) From FatherRef, Get Ability System Component
   - 15.1.2) From Father ASC:
      - 15.1.2.1) Search: Add Loose Gameplay Tag
      - 15.1.2.2) Add: Add Loose Gameplay Tag node
      - 15.1.2.3) Tag: Father.State.Transitioning

> **AUDIT NOTE (v4.5):** Invulnerability during transition was REMOVED per GAS audit. Father is vulnerable during the 5-second transition VFX. This is intentional - invulnerability was never part of the design. See `Father_Companion_GAS_Audit_Locked_Decisions.md` INV-1.

#### 15.2) Spawn Transition VFX
   - 15.2.1) From Add Loose Tag execution:
      - 15.2.1.1) From FatherRef, Get Actor Location
   - 15.2.2) Add: Spawn System at Location node
      - 15.2.2.1) System Template: NS_FatherFormTransition
      - 15.2.2.2) Location: Father Actor Location
      - 15.2.2.3) Auto Destroy: Checked

#### 15.3) Wait 5 Seconds for Transition
   - 15.3.1) From Spawn System execution:
      - 15.3.1.1) Add: AbilityTaskWaitDelay node (auto-terminates with ability per Track B v4.15)
      - 15.3.1.2) Duration: 5.0

#### 15.4) Remove Transitioning Tag
   - 15.4.1) From Delay Completed:
      - 15.4.1.1) From Father ASC, Remove Loose Gameplay Tag
      - 15.4.1.2) Tag: Father.State.Transitioning

#### 15.5) Commit Form Change Cooldown
   - 15.5.1) From Remove Tag execution:
      - 15.5.1.1) Search: `Commit Ability Cooldown`
      - 15.5.1.2) Select **Commit Ability Cooldown** node
      - 15.5.1.3) No parameters needed - uses CooldownGameplayEffectClass automatically

#### 15.6) Continue to Main Symbiote Logic
   - 15.6.1) From Commit Ability Cooldown execution:
      - 15.6.1.1) Connect to Section 16 (Main Symbiote Logic)

---

### **16) Main Symbiote Logic (Both Paths Merge)**

#### 16.1) Store Original Movement Speed
   - 16.1.1) From merge point (both paths connect here):
      - 16.1.1.1) From OwnerCharRef, Get Character Movement
   - 16.1.2) From Character Movement:
      - 16.1.2.1) GET Max Walk Speed
   - 16.1.3) Add: SET OriginalMaxWalkSpeed node
   - 16.1.4) Connect Max Walk Speed to value

#### 16.2) Store Original Jump Velocity
   - 16.2.1) From SET OriginalMaxWalkSpeed execution:
      - 16.2.1.1) From Character Movement, GET Jump Z Velocity
   - 16.2.2) Add: SET OriginalJumpZVelocity node
   - 16.2.3) Connect Jump Z Velocity to value

#### 16.3) Apply Movement Speed Boost
   - 16.3.1) From SET OriginalJumpZVelocity execution:
      - 16.3.1.1) GET OriginalMaxWalkSpeed
      - 16.3.1.2) GET MovementSpeedMultiplier
      - 16.3.1.3) Add: Multiply (Float * Float) node
   - 16.3.2) From Character Movement:
      - 16.3.2.1) SET Max Walk Speed
      - 16.3.2.2) Connect multiply result to value

#### 16.4) Apply Jump Height Boost
   - 16.4.1) From SET Max Walk Speed execution:
      - 16.4.1.1) GET OriginalJumpZVelocity
      - 16.4.1.2) GET JumpHeightMultiplier
      - 16.4.1.3) Add: Multiply (Float * Float) node
   - 16.4.2) From Character Movement:
      - 16.4.2.1) SET Jump Z Velocity
      - 16.4.2.2) Connect multiply result to value

#### 16.5) Stat Bonuses (Handled by EquippableItem)

Stat bonuses (+100 Attack, Infinite Stamina) are handled automatically by BP_FatherSymbioteForm EquippableItem via GE_EquipmentModifier_FatherSymbiote. GA_FatherSymbiote only handles movement modifications and ability grants.

#### 16.6) Apply GE_SymbioteLock to Father
   - 16.6.1) From SET Jump Z Velocity execution:
      - 16.6.1.1) From FatherRef, Get Ability System Component
   - 16.6.2) From Father ASC:
      - 16.6.2.1) Apply Gameplay Effect to Self
      - 16.6.2.2) Gameplay Effect Class: GE_SymbioteLock
      - 16.6.2.3) Level: 1.0

#### 16.6A) Store SymbioteLockHandle
   - 16.6A.1) From Apply GE_SymbioteLock:
      - 16.6A.1.1) Add: SET SymbioteLockHandle node
      - 16.6A.1.2) Connect Apply GE Return Value to handle input

#### 16.7) Ability Grants (Handled by EquippableItem)

GA_ProximityStrike is handled automatically by BP_FatherSymbioteForm EquippableItem Abilities array. GA_FatherSymbiote does not need to manually grant abilities.

#### 16.8) Activate GA_ProximityStrike
   - 16.8.1) From SET SymbioteLockHandle execution:
      - 16.8.1.1) From OwnerCharRef, Get Ability System Component
   - 16.8.2) From Player ASC:
      - 16.8.2.1) Search: Try Activate Ability By Class
      - 16.8.2.2) Ability Class: GA_ProximityStrike
      - 16.7B.1.2) Handle: GET ProximityAbilityHandle
      - 16.7B.1.3) Allow Remote Activation: Checked

#### 16.8) Update Current Form Variable
   - 16.8.1) From Try Activate execution:
      - 16.8.1.1) From FatherRef, SET CurrentForm
      - 16.8.1.2) Current Form: Symbiote

#### 16.9) Start Duration Timer
   - 16.9.1) From SET CurrentForm execution:
      - 16.9.1.1) Add: Set Timer by Function Name node
      - 16.9.1.2) Function Name: EndSymbiote
      - 16.9.1.3) Time: GET AbilityDuration
      - 16.9.1.4) Looping: Unchecked
   - 16.9.2) Add: SET DurationTimerHandle node
   - 16.9.3) Connect timer return to handle

#### 16.10) Reset Charge After Activation
   - 16.10.1) From SET DurationTimerHandle execution:
      - 16.10.1.1) From ChargeComponentRef, Call ResetCharge function

#### 16.11) End Activation (Ability Stays Active)
   - 16.11.1) From ResetCharge execution:
      - 16.11.1.1) Execution ends here (DO NOT call End Ability)
      - 16.11.1.2) Ability remains active until timer expires

---

## **PHASE 4: IMPLEMENT ENDSYMBIOTE FUNCTION (TIMER CALLBACK)**

> **AUDIT NOTE (v4.5):** This function is called by the 30-second timer. It MUST have defensive guards to handle edge cases where the timer fires after the ability has already ended (e.g., death, force-cancel). See `Father_Companion_GAS_Audit_Locked_Decisions.md` FIX-2.

### **17) Create EndSymbiote Function**

#### 17.1) Add New Function
   - 17.1.1) My Blueprint -> Functions: Click +
   - 17.1.2) Rename: EndSymbiote

#### 17.2) CRITICAL: Timer Callback Guards (v4.5)

> **Why Guards Are Needed:** The timer callback (EndSymbiote) can fire AFTER the ability has already ended due to:
> - Player death
> - Force-cancellation by another ability
> - Game state changes
>
> Without guards, the callback would execute invalid cleanup logic on a non-existent ability state.

##### 17.2.1) Guard 1: Validate FatherRef
   - 17.2.1.1) From function entry:
   - 17.2.1.2) Drag FatherRef variable (GET)
   - 17.2.1.3) Add: Is Valid node
   - 17.2.1.4) Add: Branch node
   - 17.2.1.5) From False: Return immediately (father destroyed)

##### 17.2.2) Guard 2: Check Current Form Is Still Symbiote
   - 17.2.2.1) From Branch True:
   - 17.2.2.2) From FatherRef, GET CurrentForm
   - 17.2.2.3) Add: Equal (Enum) node
   - 17.2.2.4) Compare with: E_FatherForm::Symbiote
   - 17.2.2.5) Add: Branch node
   - 17.2.2.6) From False: Return immediately (form already changed)

##### 17.2.3) Guard 3: Check Ability Still Active via Tag Proxy
   - 17.2.3.1) From Branch True:
   - 17.2.3.2) From FatherRef, Get Ability System Component
   - 17.2.3.3) Search: Has Matching Gameplay Tag
   - 17.2.3.4) Tag: Symbiote.State.Merged (ActivationOwnedTag - removed when ability ends)
   - 17.2.3.5) Add: Branch node
   - 17.2.3.6) From False: Return immediately (ability already ended)

> **Note:** `IsActive()` is NOT exposed to Blueprint (C++ only). We use `Symbiote.State.Merged` tag as a proxy - this tag is in ActivationOwnedTags and automatically removed when the ability ends.

#### 17.3) Validate OwnerCharRef (After All Guards Pass)
   - 17.3.1) From Guard 3 Branch True:
      - 17.2.1.1) Drag OwnerCharRef variable into graph (GET)
      - 17.2.1.2) Add: Is Valid node
      - 17.2.1.3) Connect OwnerCharRef to Is Valid input
   - 17.2.2) Add: Branch node
      - 17.2.2.1) Connect Is Valid result to Condition
   - 17.2.3) From False: Skip to End Ability (step 17.12)

#### 17.3) Restore Movement Speed
   - 17.3.1) From Branch True execution:
      - 17.3.1.1) From OwnerCharRef, Get Character Movement
   - 17.3.2) From Character Movement:
      - 17.3.2.1) SET Max Walk Speed
      - 17.3.2.2) Value: GET OriginalMaxWalkSpeed

#### 17.4) Restore Jump Velocity
   - 17.4.1) From SET Max Walk Speed execution:
      - 17.4.1.1) From Character Movement, SET Jump Z Velocity
      - 17.4.1.2) Value: GET OriginalJumpZVelocity

Note: Stat bonuses (Attack Rating, Stamina Regen) are handled by BP_FatherSymbioteForm EquippableItem via GE_EquipmentModifier_FatherSymbiote. No manual GE removal needed.

#### 17.5) Cleanup GA_ProximityStrike with Two-Step Pattern
   - 17.5.1) From SET Jump Z Velocity execution:
      - 17.5.1.1) Validate ProximityAbilityHandle:
      - 17.5.1.2) Drag ProximityAbilityHandle variable (GET)
      - 17.5.1.3) Add: Is Valid node
      - 17.5.1.4) Add: Branch node
   - 17.5.2) From Branch True - Step 1 Cancel:
      - 17.5.2.1) From OwnerCharRef, Get Ability System Component
      - 17.5.2.2) From Player ASC, search: Cancel Ability
      - 17.5.2.3) Add: Cancel Ability node
      - 17.5.2.4) Ability Handle: GET ProximityAbilityHandle
   - 17.5.3) From Cancel Ability - Step 2 Set Remove:
      - 17.5.3.1) From Player ASC, search: Set Remove Ability On End
      - 17.5.3.2) Add: Set Remove Ability On End node
      - 17.5.3.3) Ability Handle: GET ProximityAbilityHandle

#### 17.6) Activate GA_FatherArmor (Return to Armor Form)
   - 17.6.1) From Set Remove Ability On End execution (or Branch False):
      - 17.6.1.1) Validate FatherRef:
      - 17.6.1.2) Drag FatherRef variable (GET)
      - 17.6.1.3) Add: Is Valid node
      - 17.6.1.4) Add: Branch node
   - 17.6.2) From Branch True:
      - 17.6.2.1) From FatherRef, Get Ability System Component
   - 17.6.3) From Father ASC:
      - 17.6.3.1) Search: Try Activate Ability by Class
      - 17.6.3.2) Ability Class: GA_FatherArmor
      - 17.6.3.3) Allow Remote Activation: True

#### 17.7) Remove GE_SymbioteLock Using Handle
   - 17.7.1) From Try Activate Ability execution:
      - 17.7.1.1) Validate SymbioteLockHandle:
      - 17.7.1.2) Drag SymbioteLockHandle variable (GET)
      - 17.7.1.3) Add: Is Valid node
      - 17.7.1.4) Add: Branch node
   - 17.7.2) From Branch True:
      - 17.7.2.1) From Father ASC, Remove Active Gameplay Effect
      - 17.7.2.2) Handle: GET SymbioteLockHandle
      - 17.7.2.3) Stacks To Remove: -1

#### 17.8) Update Father Form Variable
   - 17.8.1) From Remove GE_SymbioteLock execution (or Branch False):
      - 17.8.1.1) From FatherRef, SET CurrentForm
      - 17.8.1.2) Current Form: Armor

#### 17.9) Start Charge Cooldown
   - 17.9.1) From SET CurrentForm execution:
      - 17.9.1.1) From ChargeComponentRef, Call StartCooldown function

#### 17.10) Call End Ability
   - 17.10.1) From StartCooldown execution (or from step 17.2.3):
      - 17.10.1.1) Add: End Ability node
      - 17.10.1.2) Was Cancelled: Unchecked (false)

---

## **PHASE 5: IMPLEMENT ENDABILITY EVENT**

> **AUDIT NOTE (v4.5):** This event MUST include a `bWasCancelled` check. Cleanup should ONLY run when cancelled (form switch in progress). Normal end (after EndSymbiote calls EndAbility) should skip redundant cleanup. Pattern aligned with GA_FatherArmor guide PHASE 7. See `Father_Companion_GAS_Audit_Locked_Decisions.md` FIX-1.

### **18) Override EndAbility Event**

#### 18.1) Add Event On End Ability
   - 18.1.1) In Event Graph, right-click
   - 18.1.2) Search: Event On End Ability
   - 18.1.3) Add: Event OnEndAbility node
   - 18.1.4) Node has `bWasCancelled` boolean output pin

#### 18.2) CRITICAL: Check bWasCancelled (v4.5)

> **Why This Check Is Needed:** The EndAbility event fires in TWO scenarios:
> 1. `bWasCancelled = false`: After EndSymbiote calls EndAbility (cleanup already done)
> 2. `bWasCancelled = true`: When cancelled by another form's Cancel Abilities With Tag
>
> Cleanup should ONLY run when bWasCancelled = true (form switch in progress).

##### 18.2.1) Add Branch on bWasCancelled
   - 18.2.1.1) From Event OnEndAbility execution pin
   - 18.2.1.2) Drag wire and search: Branch
   - 18.2.1.3) Add Branch node

##### 18.2.2) Connect bWasCancelled
   - 18.2.2.1) From Event OnEndAbility `bWasCancelled` pin (boolean)
   - 18.2.2.2) Drag to Branch `Condition` pin

##### 18.2.3) Configure Branch Paths
   - 18.2.3.1) **FALSE path**: Skip cleanup - form is now active (EndSymbiote already handled it)
   - 18.2.3.2) **TRUE path**: Continue to cleanup (form switch in progress)

#### 18.3) Clear Duration Timer (From TRUE path)
   - 18.3.1) From Branch True execution:
      - 18.3.1.1) Add: Clear and Invalidate Timer by Handle node
      - 18.3.1.2) Handle: GET DurationTimerHandle
      - 18.3.1.3) This prevents the timer from firing after ability is cancelled

#### 18.4) Validate FatherRef for Cleanup
   - 18.4.1) From Clear Timer execution:
      - 18.4.1.1) Drag FatherRef variable (GET)
      - 18.4.1.2) Add: Is Valid node
      - 18.4.1.3) Add: Branch node

#### 18.5) Form Tags Auto-Removed
   - 18.5.1) Ephemeral state tags (Symbiote.State.Merged) auto-removed by Activation Owned Tags cleanup
   - 18.5.2) Form identity (Effect.Father.FormState.Symbiote) persists until next form's transition prelude removes it

#### 18.6) Validate OwnerCharRef for Player Cleanup
   - 18.6.1) From Branch True (FatherRef valid):
      - 18.6.1.1) Drag OwnerCharRef variable (GET)
      - 18.6.1.2) Add: Is Valid node
      - 18.6.1.3) Add: Branch node

Note: Stat bonuses (Attack Rating, Stamina Regen) are handled by BP_FatherSymbioteForm EquippableItem via GE_EquipmentModifier_FatherSymbiote. No manual GE removal needed in EndAbility.

#### 18.7) Restore Movement Speed (Cancellation Cleanup)
   - 18.7.1) From Branch True (OwnerCharRef valid):
      - 18.7.1.1) From OwnerCharRef, Get Character Movement
   - 18.7.2) Validate OriginalMaxWalkSpeed > 0:
      - 18.7.2.1) GET OriginalMaxWalkSpeed
      - 18.7.2.2) Add: Greater Than (float > float) with 0.0
      - 18.7.2.3) Add: Branch node
   - 18.7.3) From Branch True:
      - 18.7.3.1) SET Max Walk Speed = GET OriginalMaxWalkSpeed
   - 18.7.4) From SET Max Walk Speed:
      - 18.7.4.1) GET OriginalJumpZVelocity
      - 18.7.4.2) Add: Greater Than with 0.0
      - 18.7.4.3) Add: Branch node
   - 18.7.5) From Branch True:
      - 18.7.5.1) SET Jump Z Velocity = GET OriginalJumpZVelocity

#### 18.8) Cleanup GA_ProximityStrike if Not Already Cleaned
   - 18.8.1) From speed restoration (or Branch False if speed not stored):
      - 18.8.1.1) From OwnerCharRef, Get Ability System Component
      - 18.8.1.2) Validate ProximityAbilityHandle
      - 18.8.1.3) Add: Is Valid node
      - 18.8.1.4) Add: Branch node
   - 18.8.2) From inner Branch True:
      - 18.8.2.1) From Player ASC, Cancel Ability
      - 18.8.2.2) Handle: GET ProximityAbilityHandle
   - 18.8.3) From Cancel Ability:
      - 18.8.3.1) Set Remove Ability On End
      - 18.8.3.2) Handle: GET ProximityAbilityHandle

#### 18.9) Activate GA_FatherArmor (Return to Armor Form on Cancellation)
   - 18.9.1) From ProximityStrike cleanup (or Branch False):
      - 18.9.1.1) Validate FatherRef
      - 18.9.1.2) From FatherRef, Get Ability System Component
   - 18.9.2) From Father ASC:
      - 18.9.2.1) Search: Try Activate Ability by Class
      - 18.9.2.2) Ability Class: GA_FatherArmor
      - 18.9.2.3) Allow Remote Activation: True

### **19) Compile and Save Ability**

#### 19.1) Finalize Ability
   - 19.1.1) Click: Compile button
   - 19.1.2) Click: Save button

---

## **PHASE 6: PLAYER CHARACTER INTEGRATION**

### **20) Verify UltimateChargeComponent on Player**

#### 20.1) Open Player Character Blueprint
   - 20.1.1) Content Browser: Navigate to player blueprint
   - 20.1.2) Example: /Content/Characters/BP_NarrativePlayer
   - 20.1.3) Double-click to open

#### 20.2) Verify Component Exists
   - 20.2.1) Components panel: Find UltimateChargeComponent
   - 20.2.2) If missing: Follow WBP_UltimatePanel_Implementation_Guide PHASE 3

#### 20.3) Configure Component for Symbiote
   - 20.3.1) Select: UltimateChargeComponent in hierarchy
   - 20.3.2) Details panel: Verify ThresholdCurve = FC_UltimateThreshold
   - 20.3.3) Verify: Father Damage Efficiency = 0.5
   - 20.3.4) Verify: Cooldown Duration = 120.0
   - 20.3.5) Verify: Equipped Ultimate = Symbiote

#### 20.4) Compile and Save
   - 20.4.1) Compile button
   - 20.4.2) Save button

### **21) Grant Symbiote Ability to Player**

#### 21.1) Locate Ability Configuration
   - 21.1.1) Find player's CharacterDefinition or AbilityConfiguration
   - 21.1.2) Example: /Content/Characters/DA_PlayerAbilityConfig

#### 21.2) Add to Default Abilities
   - 21.2.1) Open: Ability Configuration asset
   - 21.2.2) Find: Default Abilities array
   - 21.2.3) Click: + button
   - 21.2.4) Select: GA_FatherSymbiote

#### 21.3) Save Configuration
   - 21.3.1) Click: Save button

---

## **PHASE 7: INPUT CONFIGURATION**

### **22) Verify Form Change Input Action Exists**

#### 22.1) Navigate to Input Folder
   - 22.1.1) Content Browser: Navigate to input assets folder
   - 22.1.2) Example: /Content/Input/

#### 22.2) Locate Form Change Input Action
   - 22.2.1) Find: IA_FatherFormChange
   - 22.2.2) If exists: Proceed to step 23
   - 22.2.3) If not exists: Create per GA_FatherCrawler guide

### **23) Verify Input Mapping Context**

#### 23.1) Open Input Mapping Context
   - 23.1.1) Locate: Player's Input Mapping Context
   - 23.1.2) Example: /Content/Input/IMC_Default
   - 23.1.3) Double-click to open

#### 23.2) Verify Form Change Mapping
   - 23.2.1) Find: IA_FatherFormChange in Mappings array
   - 23.2.2) Verify Key: Z (Quick Use Wheel)
   - 23.2.3) If missing: Add mapping per GA_FatherCrawler guide

#### 23.3) Save Mapping Context
   - 23.3.1) Click: Save button

### **24) Verify Narrative Input Mapping**

#### 24.1) Open Narrative Input Configuration
   - 24.1.1) Locate: NarrativeAbilityInputMapping asset
   - 24.1.2) Example: /Content/NarrativePro/Input/DA_NarrativeInputMapping

#### 24.2) Verify Form Change Mapping
   - 24.2.1) Find: Ability Input Mappings array
   - 24.2.2) Verify entry exists:
      - 24.2.2.1) Input Action: IA_FatherFormChange
      - 24.2.2.2) Input Tag: Narrative.Input.Father.FormChange

#### 24.3) Save Configuration
   - 24.3.1) Click: Save button

---

## **QUICK REFERENCE**

### **Tag Configuration Summary**

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.Symbiote |
| Cancel Abilities with Tag | Ability.Father.Crawler, Armor, Exoskeleton, Engineer |
| Activation Required Tags | Father.State.Alive, Father.State.Recruited, Symbiote.Charge.Ready |
| Activation Blocked Tags | Father.State.Dormant, Father.State.Transitioning, Father.State.SymbioteLocked, Cooldown.Father.FormChange |
| Cooldown Gameplay Effect Class | GE_FormChangeCooldown |
| Activation Owned Tags | Symbiote.State.Merged |
| InputTag | Narrative.Input.Father.FormChange |

> **AUDIT NOTE (v4.9 - C_SYMBIOTE_STRICT_CANCEL):** Symbiote is NOT in any other form's cancel_abilities_with_tag. This is intentional - Symbiote is an ultimate ability (30s duration) that cannot be cancelled by player-initiated form changes. Only GA_FatherSacrifice (emergency override) can cancel Symbiote. The `Father.State.SymbioteLocked` tag blocks other abilities from activating during the 30s duration. See LOCKED_CONTRACTS.md Contract 11.

**Note (v4.4):** Form identity (`Effect.Father.FormState.Symbiote`) is NOT in Activation Owned Tags. It persists via `GE_SymbioteState` (Option B architecture). Only ephemeral state tags (`Symbiote.State.Merged`) belong in Activation Owned Tags. `Father.Form.*` tags are **orphan tags**.

### **Form State Architecture (Option B)**

| Component | Description |
|-----------|-------------|
| Activation Owned Tags | `Symbiote.State.Merged` (ephemeral, auto-granted on activation) |
| Form Identity | GE_SymbioteState grants `Effect.Father.FormState.Symbiote` (persists via infinite GE) |
| GE_FormChangeCooldown | 15s cooldown after form change |
| GE_SymbioteLock | 30s duration lock preventing form switching |
| Replication | Tags replicate via ReplicateActivationOwnedTags |
| Cleanup (State Tags) | Activation Owned Tags auto-removed when ability ends |
| Cleanup (Form Identity) | GE_SymbioteState removed by next form's transition prelude |

> **AUDIT NOTE (v4.5):** Invulnerability was REMOVED. Father is NOT invulnerable during Symbiote form or during transitions. See `Father_Companion_GAS_Audit_Locked_Decisions.md` INV-1.

### **Variable Summary**

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| OriginalMaxWalkSpeed | Float | 0.0 | Stores player speed for restoration |
| OriginalJumpZVelocity | Float | 0.0 | Stores jump velocity for restoration |
| DurationTimerHandle | TimerHandle | None | 30 second duration timer |
| MovementSpeedMultiplier | Float | 1.5 | Movement boost (150%) |
| JumpHeightMultiplier | Float | 1.3 | Jump boost (130%) |
| AbilityDuration | Float | 30.0 | Symbiote duration in seconds |
| bIsFirstActivation | Boolean | True | Tracks initial spawn vs form switch |
| ChargeComponentRef | UltimateChargeComponent | None | Charge component reference |
| FatherRef | BP_FatherCompanion | None | Father reference |
| OwnerCharRef | Character | None | Owner character reference |

### **Handle Variables**

| Variable | Type | Purpose |
|----------|------|---------|
| SymbioteLockHandle | ActiveGameplayEffectHandle | GE_SymbioteLock removal on father |
| ProximityAbilityHandle | GameplayAbilitySpecHandle | GA_ProximityStrike cleanup on player |

### **Stat Modifications**

| Stat | Value | Method |
|------|-------|--------|
| Movement Speed | +50% | CharacterMovement component direct |
| Jump Height | +30% | CharacterMovement component direct |
| Attack Rating | +100 | BP_FatherSymbioteForm EquippableItem (SetByCaller.AttackRating) |
| Stamina Regen | Infinite | GE_EquipmentModifier_FatherSymbiote modifier [3] (Override 10000) |

### **Transition Parameters**

| Parameter | Value |
|-----------|-------|
| VFX Duration | 5 seconds |
| Father Invulnerable | **NO** (v4.5 audit - invulnerability removed) |
| Form Cooldown | 15 seconds |
| VFX System | NS_FatherFormTransition |

### **Symbiote Parameters**

| Parameter | Value |
|-----------|-------|
| Duration | 30 seconds |
| Post-Duration Return | Armor form |
| Post-Duration Cooldown | 120 seconds |
| Charge Reset | On activation and player death |
| Proximity Strike | 40 damage per 0.5s in 350 units |

### **Multiplayer Settings**

| Setting | Value | Purpose |
|---------|-------|---------|
| Instancing Policy | Instanced Per Actor | One instance per ASC |
| Replication Policy | Replicate Yes | Effects and tags replicate |
| Net Execution Policy | Server Only | NPC-owned form ability granting to player ASC |

### **Gameplay Effect Summary**

| Effect | Duration | Purpose |
|--------|----------|---------|
| GE_SymbioteState | Infinite | Form identity (Effect.Father.FormState.Symbiote only) |
| GE_EquipmentModifier_FatherSymbiote | Infinite (via EquippableItem) | +100 Attack, Infinite Stamina |
| GE_SymbioteLock | 30 seconds | Grants Father.State.SymbioteLocked |
| GE_FormChangeCooldown | 15 seconds | Shared cooldown between form changes |

> **AUDIT NOTE (v4.5):** GE_TransitionInvulnerability was REMOVED. Father is vulnerable during 5s transition. See `Father_Companion_GAS_Audit_Locked_Decisions.md` INV-1.

### **UltimateChargeComponent Integration**

| Property | Value |
|----------|-------|
| Component Name | UltimateChargeComponent |
| Threshold Source | FC_UltimateThreshold (CurveFloat) |
| Player Damage | 100% efficiency |
| Father Damage | 50% efficiency |
| Check Function | IsChargeReady() |
| Reset Function | ResetCharge() |
| Cooldown Function | StartCooldown() |

### **Granted Abilities**

| Ability | Granted To | Handle Variable | Cleanup Method |
|---------|------------|-----------------|----------------|
| GA_ProximityStrike | Player ASC | ProximityAbilityHandle | Two-Step (Cancel + SetRemoveOnEnd) |

### **Effect Removal Architecture**

| Effect | Target ASC | Removal Method |
|--------|------------|----------------|
| State Tags (Symbiote.State.Merged) | Father | Activation Owned Tags (automatic) |
| Form Identity (GE_SymbioteState) | Father | Next form's transition prelude |
| Stat Bonuses | Player | EquippableItem HandleUnequip (automatic) |
| GE_SymbioteLock | Father | Remove Active GE (SymbioteLockHandle) |
| Father.State.Transitioning | Father | RemoveLooseGameplayTag (INV-1: no GE removal needed) |

### **EndSymbiote Cleanup Flow**

| Step | Action |
|------|--------|
| 1 | Validate OwnerCharRef |
| 2 | Restore Max Walk Speed to original |
| 3 | Restore Jump Z Velocity to original |
| 4 | Cancel Ability (ProximityAbilityHandle) |
| 5 | Set Remove Ability On End (ProximityAbilityHandle) |
| 6 | Try Activate GA_FatherArmor (return to Armor form) |
| 7 | Remove GE_SymbioteLock using SymbioteLockHandle |
| 8 | SET CurrentForm to Armor |
| 9 | Call StartCooldown on ChargeComponentRef |
| 10 | Call End Ability |

Note: Stat bonuses (Attack Rating, Stamina) are removed automatically by EquippableItem HandleUnequip when form changes.

### **Related Documents**

| Document | Purpose |
|----------|---------|
| WBP_UltimatePanel_Implementation_Guide_v1_0.md | Ultimate HUD system, UltimateChargeComponent |
| GE_FormChangeCooldown_Implementation_Guide_v1_0.md | Form change cooldown effect |
| GA_ProximityStrike_Implementation_Guide_v2_3.md | Proximity aura ability |
| Father_Companion_System_Setup_Guide_v2_0.md | Father character setup |
| Father_Companion_Technical_Reference_v6_2.md | Section 7.4: Net Execution Policy, Section 14.8: Effect Handle Storage, Section 36.12: Two-Step Cleanup, Section 37: Recruited Tag Gating |
| Father_Companion_GAS_Abilities_Audit.md | **LOCKED DECISIONS** - INV-1, VTF-7, Rule 1-4, Design Decisions 1A-4 |
| DefaultGameplayTags_FatherCompanion_v3_5.ini | All gameplay tags including Father.State.Recruited |

---

## **CHANGELOG**

| Version | Date | Changes |
|---------|------|---------|
| 4.9 | January 2026 | **C_SYMBIOTE_STRICT_CANCEL Contract (LOCKED):** Added PHASE 2B documenting GE_SymbioteDuration SetByCaller pattern - applied at activation START to grant `Father.State.SymbioteLocked` for 30s. Updated Tag Configuration Summary with audit note explaining why Symbiote is NOT in other forms' cancel lists. Added Defense-in-Depth strategy (3 layers: blocking, no-cancel, duration enforcement). Contract reference: LOCKED_CONTRACTS.md Contract 11. Claude-GPT dual audit 2026-01-23. |
| 4.8 | January 2026 | **Claude-GPT Audit Fixes:** (1) INC-2: Fixed DOCUMENT INFORMATION table version from 4.4 to 4.8. (2) INC-3: Added Cooldown Model clarification explaining dual cooldown system (15s GE_FormChangeCooldown vs 120s UltimateChargeComponent). (3) INC-4: Changed "Delay node" to "AbilityTaskWaitDelay" per Track B v4.15. Added Authority Note and Audit Status sections per documentation rules D-1/D-2. |
| 4.7 | January 2026 | **Locked Decisions Reference:** Added Father_Companion_GAS_Abilities_Audit.md to Related Documents. This guide now references all locked decisions: INV-1 (no invulnerability), VTF-7 (CommitCooldown required), Decision 1A (stats via EI), Decision 1B (StaminaRegenRate via child GE), Decision 3 (GE_SymbioteBoost removed). Updated Technical Reference to v6.2. |
| 4.6 | January 2026 | Version alignment with manifest v3.0.3. No functional changes. |
| 4.5 | January 2026 | **GAS Audit Fixes:** (1) REMOVED all invulnerability - Father NOT invulnerable during Symbiote or transitions per INV-1 audit decision. Removed GE_TransitionInvulnerability, Narrative.State.Invulnerable from GE_SymbioteState. (2) Added PHASE 4 timer callback guards (FatherRef validity, CurrentForm check, Symbiote.State.Merged tag proxy) per FIX-2. (3) Added PHASE 5 bWasCancelled check - cleanup only runs when cancelled, not on normal EndSymbiote flow per FIX-1. (4) Added movement speed/jump restoration to EndAbility for cancellation cleanup. (5) Added GA_FatherArmor activation on cancellation. See `Father_Companion_GAS_Audit_Locked_Decisions.md`. |
| 4.4 | January 2026 | **Option B Form State Architecture:** GE_SymbioteState for form identity (grants Effect.Father.FormState.Symbiote), transition prelude documented. Fixed Activation Owned Tags to `Symbiote.State.Merged` only (per manifest). Added `Symbiote.Charge.Ready` to Activation Required Tags. Updated to UE 5.7. Added AUTOMATION VS MANUAL table. Added orphan tag notes. |
| 3.5 | January 2026 | EquippableItem stat integration: Removed SymbioteBoostHandle variable and all GE_SymbioteBoost removal code. Stat bonuses (Attack Rating +100, Stamina Regen infinite) now handled by BP_FatherSymbioteForm EquippableItem via GE_EquipmentModifier_FatherSymbiote. Removed Section 17.5 (Remove GE_SymbioteBoost) and Section 18.6 (Remove GE_SymbioteBoost in EndAbility). Renumbered remaining sections. Updated Quick Reference tables. Technical Reference v5.12 Section 58.8 documents this architecture. |
| 3.4 | January 2026 | Simplified documentation: Tag configuration (Section 10), variable creation (Section 11), and GameplayEffect configuration (Sections 7-8) converted to markdown tables. |
| 3.3 | January 2026 | Simplified PHASE 1: Replaced 6 detailed subsections with consolidated tag tables. Tags now listed in Create Required Tags and Verify Existing Tags tables. Reduced PHASE 1 from 54 lines to 18 lines. |
| 3.2 | January 2026 | Built-in cooldown system: Added CooldownGameplayEffectClass property, replaced Apply GE with CommitAbilityCooldown node. Removed Block Abilities with Tag for cooldown. Fixed GE tag property to use Add to Inherited. |
| 3.1 | January 2026 | Form tags via Activation Owned Tags. ReplicateActivationOwnedTags required. Simplified ability logic. EndSymbiote returns to Armor via GA_FatherArmor activation. |
| 3.0 | December 2025 | Net Execution Policy: Server Only for cross-actor operations. |
| 2.9 | December 2025 | Added Father.State.Recruited gating. Handle-based effect removal. |
| 2.8 | December 2025 | Replaced SymbioteChargeComponent with UltimateChargeComponent. |
| 2.7 | December 2025 | Block Abilities with Tag for cooldown integration. |
| 2.6 | December 2025 | Added form switch with transition animation. |
| 2.5 | December 2025 | Duration 30s. Auto-return to Armor. SymbioteLocked tag. |

---

**END OF GA_FATHERSYMBIOTE IMPLEMENTATION GUIDE VERSION 4.9**

**Blueprint-Only Implementation for Unreal Engine 5.7 + Narrative Pro Plugin v2.2**
**Architecture: Option B (GE-Based Form Identity)**
**Contract: LOCKED_CONTRACTS.md Contract 11 - C_SYMBIOTE_STRICT_CANCEL**
**GAS Audit Status: COMPLIANT (January 2026)**
