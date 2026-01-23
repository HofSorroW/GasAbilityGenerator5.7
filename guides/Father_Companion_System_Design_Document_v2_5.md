# Father Companion System - Design Document

---

## INTELLECTUAL PROPERTY NOTICE

| Field | Information |
|-------|-------------|
| **Owner** | Erdem Halacoglu |
| **Contact** | erdemhalacoglu@gmail.com |
| **Status** | CONFIDENTIAL - All Rights Reserved |

### Rights Statement

This document and all concepts, designs, mechanics, and ideas contained within are the exclusive intellectual property of **Erdem Halacoglu**.

### Restrictions

| Restriction | Description |
|-------------|-------------|
| **No Sharing** | This document may not be shared with any third party |
| **No Reproduction** | No part of this design may be reproduced without explicit written permission |
| **No Derivative Works** | No derivative works may be created based on these concepts |
| **No Commercial Use** | No commercial use by any party other than the owner |
| **No Suggestion** | These concepts may not be suggested or referenced to other users or company staff |

### Covered Content

All content within this document is protected, including but not limited to:

- Father Companion system architecture
- Form designs (Crawler, Armor, Exoskeleton, Symbiote, Engineer)
- Ability mechanics and parameters
- Sacrifice and dormant state mechanics
- Visual design language
- Control schemes and HUD designs
- Gameplay tag structures
- All future additions and modifications

### Acknowledgment

Any party with access to this document acknowledges the intellectual property rights of Erdem Halacoglu and agrees to maintain strict confidentiality.

---

## Document Information

| Field | Value |
|-------|-------|
| Version | 2.5 |
| Engine | Unreal Engine 5.7 |
| Plugin | Narrative Pro v2.2 |
| Implementation | Blueprint Only |
| Multiplayer | Compatible |
| Owner | Erdem Halacoglu |

---

## 1) SYSTEM OVERVIEW

### 1.1) Core Concept

The Father Companion is a mechanical/energy hybrid entity that bonds with the player, providing different combat capabilities through five distinct forms. Each form represents a unique playstyle archetype, allowing players to adapt to different combat situations.

### 1.2) Father Identity

| Aspect | Description |
|--------|-------------|
| Nature | Mechanical construct with energy core |
| Intelligence | Semi-autonomous AI |
| Bond | Symbiotic relationship with player |
| Visual | Metallic plates connected by energy conduits |
| Personality | Loyal, protective, sacrificial |

### 1.3) Design Philosophy

| Principle | Implementation |
|-----------|----------------|
| Minimal Buttons | Maximum 3 ability inputs per form (Q, E) |
| Clear Identity | Each form serves distinct purpose |
| Meaningful Choice | Form selection matters tactically |
| Emotional Bond | Sacrifice mechanic creates attachment |
| Flexibility | No form is mandatory for progression |

### 1.4) Form Overview

| Form | Archetype | Attachment | Player Focus | Stat Bonus |
|------|-----------|------------|--------------|------------|
| Crawler | Support | Following | Gunplay | None |
| Armor | Tank | Chest | Survival | +50 Armor, -15% Speed |
| Exoskeleton | Assassin | Back/Spine | Mobility | +50% Speed, +30% Jump, +10 Attack |
| Symbiote | Berserker | Full Body | Aggression | +50% Speed, +30% Jump, +100 Attack, Infinite Stamina |
| Engineer | Tactician | Deployed | Positioning | None (turret mode) |

---

## 2) FORMS

### 2.1) CRAWLER (Mobile Support)

#### 2.1.1) Identity

| Aspect | Description |
|--------|-------------|
| Archetype | Support / Summoner |
| Fantasy | "My father companion fights alongside me" |
| Attachment | None - Father follows player autonomously |
| Player Input | None during combat |
| Best For | Gunplay focus, exploration, mobile combat |

#### 2.1.2) Behavior

| State | Father Action |
|-------|---------------|
| Combat (Ranged) | Fires laser at distant enemies |
| Combat (Melee) | Attacks nearby enemies directly |
| No Combat | Follows player at set distance |
| Player Sprinting | Matches player speed |

#### 2.1.3) Abilities

| Ability | Type | Trigger |
|---------|------|---------|
| GA_FatherCrawler | Form Activation | T (Wheel) |
| GA_FatherAttack | AI Combat | Automatic (melee range) |
| GA_FatherLaserShot | AI Combat | Automatic (ranged) |
| GA_FatherMark | Passive | On enemy engagement |

#### 2.1.4) Enemy Marking

| Parameter | Value |
|-----------|-------|
| Trigger | Father targets enemy |
| Effect | Enemy visible through walls |
| Duration | 5 seconds |
| Bonus | +10% damage to marked targets |
| Max Active | 3 marks |

#### 2.1.5) GA_FatherAttack Parameters

| Parameter | Value |
|-----------|-------|
| Base Damage | 10.0 |
| Parent Class | GA_Melee_Unarmed |
| Attack Sockets | father_right, father_left |
| Warp to Target | Yes |
| Damage Effect | GE_WeaponDamage |
| Damage System | NarrativeDamageExecCalc |

#### 2.1.6) Use Cases

| Scenario | Effectiveness |
|----------|---------------|
| Exploring unknown area | 10/10 |
| Pushing through enemies | 8/10 |
| Boss fight (mobile) | 8/10 |
| Defending position | 4/10 |
| Stealth approach | 4/10 |

---

### 2.2) ARMOR (Tank)

#### 2.2.1) Identity

| Aspect | Description |
|--------|-------------|
| Archetype | Tank / Paladin |
| Fantasy | "I am the immovable wall" |
| Attachment | Chest socket - forms protective carapace |
| Player Input | Minimal (optional Q for burst) |
| Best For | Holding position, absorbing damage, survival |

#### 2.2.2) Visual Design

| Element | Description |
|---------|-------------|
| Mechanical Plates | Heavy overlapping plates on chest/shoulders |
| Energy Shield | Translucent barrier surrounding player |
| Father Core | Visible on chest center, eyes glowing |
| Color Accent | Blue/White (protection energy) |

#### 2.2.3) Defense Parameters

| Parameter | Value |
|-----------|-------|
| Armor Bonus | +50 (via GE_ArmorBoost) |
| Movement Penalty | -15% speed |

#### 2.2.4) Abilities

| Ability | Type | Input |
|---------|------|-------|
| GA_FatherArmor | Form Activation | T (Wheel) |
| GA_ProtectiveDome | Passive | Automatic (absorbs damage) |
| GA_DomeBurst | Active/Auto | Q (Tap) or auto at threshold |

#### 2.2.5) Dome Energy System (Locked January 2026)

The Protective Dome uses an Energy-Only damage model. The player takes full incoming damage (no absorption reduction), while 30% of post-mitigation damage is converted to Dome Energy stored in a Player ASC attribute.

| Parameter | Value | Notes |
|-----------|-------|-------|
| Energy Location | Player ASC (AS_DomeAttributes) | Armor form = player+father merged |
| Damage Model | Energy-Only | Player takes full damage |
| Energy Source | OnDamagedBy.Damage | Post-mitigation damage amount |
| Conversion Ratio | 0.30 (30%) | 30% of damage → energy |
| MaxDomeEnergy | 500 | Maximum energy capacity |
| Clamp at Max | Yes | Energy capped at 500 |

#### 2.2.6) Dome Burst Mechanic (Locked January 2026)

| Parameter | Value | Notes |
|-----------|-------|-------|
| Manual Release | Q key when FULL (500) only | No partial release |
| Auto-burst | DISABLED | No automatic burst at threshold |
| Pre-release | DISABLED | Cannot release before full |
| Damage | 75 (flat) | Does not scale with energy |
| Radius | 500 units | AOE centered on player |
| Knockback | 1000 units | Pushes enemies away |
| Cooldown | 12 seconds | After burst before recharge |
| Cooldown Blocks Burst | Yes | Both manual AND form exit |
| Cooldown Blocks Charging | No | Energy accumulates during cooldown |
| Queue After Cooldown | No | No queued burst |
| Reset on Burst | Energy → 0 | Full reset after burst |
| Reset on Death | Energy → 0 (no burst) | Silent reset |

#### 2.2.7) Form Exit Burst (Locked January 2026)

When the player switches forms via T wheel while Dome Energy is full:
- If charged (500) AND not on cooldown → burst fires automatically
- If on cooldown → no burst, energy reset to 0
- Prevents "punishing" accidental form changes
- Enables tactical option: build charge → switch forms → burst fires

#### 2.2.8) FullyCharged Tag Behavior

| State | Tag Granted | Burst Available |
|-------|-------------|-----------------|
| Energy < 500 | No | No |
| Energy = 500 | Father.Dome.FullyCharged | Yes (if not on cooldown) |
| On Cooldown | (removed) | No |
| After Burst | (removed) | No |

#### 2.2.9) Reset Ownership

GA_ProtectiveDome EndAbility handles all dome cleanup:
- Resets DomeEnergy to 0 (if not already burst)
- Removes Father.Dome.FullyCharged tag
- Unbinds OnDamagedBy delegate
- Removes GE_DomeAbsorption effect

#### 2.2.10) Form Exit Burst Implementation (Decisions 22-24)

When switching forms via T wheel while dome is fully charged:

| Step | Action |
|------|--------|
| 1 | Player selects new form from T wheel |
| 2 | EI_FatherArmorForm.HandleUnequip override fires |
| 3 | Check: Father.Dome.FullyCharged AND NOT Cooldown.Father.DomeBurst |
| 4 | If conditions met: TryActivateAbilityByClass(GA_DomeBurst) |
| 5 | Parent HandleUnequip called (removes abilities) |
| 6 | New form equipment equipped |

**Implementation Notes:**
- Decision 22: TryActivateAbilityByClass is BlueprintCallable on UAbilitySystemComponent
- Decision 23: HandleUnequip is BlueprintNativeEvent, override runs BEFORE parent removes abilities
- Decision 24: Father.Dome.FullyCharged tag gates GA_DomeBurst activation

**Blocked Cases:**
- Cooldown active → no burst, energy reset to 0
- Not fully charged → no burst, energy reset to 0

**Generator Requirement:**
- FManifestFunctionOverrideDefinition support for EquippableItems (BLOCKED until generator extension)

#### 2.2.11) Use Cases

| Scenario | Effectiveness |
|----------|---------------|
| Defending objective | 10/10 |
| Tanking boss attacks | 10/10 |
| Holding chokepoint | 8/10 |
| Mobile combat | 4/10 |
| Chasing enemies | 2/10 |

---

### 2.3) EXOSKELETON (Assassin)

#### 2.3.1) Identity

| Aspect | Description |
|--------|-------------|
| Archetype | Assassin / Rogue |
| Fantasy | "Strike from shadows, untouchable" |
| Attachment | Back/Spine - enhances mobility |
| Player Input | High (skill-based abilities) |
| Best For | Hit-and-run, flanking, precision strikes |

#### 2.3.2) Visual Design

| Element | Description |
|---------|-------------|
| Mechanical Plates | Light plates at joints (spine, knees, elbows) |
| Energy Conduits | Glowing lines connecting plates |
| Coverage | Minimal (speed over protection) |
| Color Accent | Orange/Yellow (power energy) |
| Animation | Energy pulses when moving |

#### 2.3.3) Stat Parameters

| Parameter | Value |
|-----------|-------|
| Base Speed Bonus | +50% movement speed |
| Sprint Speed | +75% movement speed (during sprint) |
| Base Jump Bonus | +30% jump height |
| Sprint Jump Bonus | +50% jump height (during sprint) |
| Attack Rating Bonus | +10 AttackRating |
| Dash Distance | 800 units |
| Dash Cooldown | 3 seconds |
| Dash I-Frames | No (removed per GAS Audit INV-1) |

#### 2.3.4) Abilities

| Ability | Type | Input |
|---------|------|-------|
| GA_FatherExoskeleton | Form Activation | T (Wheel) |
| GA_FatherExoskeletonDash | Active | Q (Tap) |
| GA_FatherExoskeletonSprint | Active | Q (Hold) |
| GA_StealthField | Active | E |

#### 2.3.5) Stealth Mechanic

| Parameter | Value |
|-----------|-------|
| Duration | 10 seconds |
| Cooldown | 15 seconds |
| Speed Penalty | -20% while stealthed |
| Break Conditions | Attack, take damage, timer, form change |
| First Hit Bonus | +50% damage |

#### 2.3.6) Gameplay Loop

| Step | Description |
|------|-------------|
| 1 | Spot enemy group |
| 2 | Activate Stealth (E) |
| 3 | Sprint to flank position (Q Hold) |
| 4 | Position behind priority target |
| 5 | Attack -> Stealth breaks -> Backstab triggers |
| 6 | Dash away (Q Tap) |
| 7 | Repeat or escape |

#### 2.3.7) Use Cases

| Scenario | Effectiveness |
|----------|---------------|
| Flanking enemies | 10/10 |
| Escaping danger | 10/10 |
| Precision strikes | 10/10 |
| Melee combat | 8/10 |
| Sustained tanking | 2/10 |

---

### 2.4) SYMBIOTE (Berserker)

#### 2.4.1) Identity

| Aspect | Description |
|--------|-------------|
| Archetype | Berserker / Warlock |
| Fantasy | "Unleash primal fury, become unstoppable" |
| Attachment | Full body merge - father fuses with player |
| Availability | Requires charge (100%), limited duration |
| Best For | Overwhelming odds, boss fights, aggressive play |

#### 2.4.2) Visual Design

| Element | Description |
|---------|-------------|
| Coverage | Full body transformation |
| Mechanical | Plates flow across entire body |
| Energy | Veins of energy throughout |
| Color Accent | Purple/Black with energy veins |
| Eyes | Player eyes glow with energy |

#### 2.4.3) Charge Mechanic

| Source | Charge Gained |
|--------|---------------|
| Deal damage | +3% per hit |
| Take damage | +8% per hit received |
| Kill enemy | +15% per kill |
| Time in combat | +2% per second |

#### 2.4.4) Form Parameters

| Parameter | Value |
|-----------|-------|
| Activation Requirement | 100% charge |
| Duration | 30 seconds |
| Post-Form Cooldown | 120 seconds (before recharge) |
| Charge Reset | On activation and player death |

#### 2.4.5) Stat Boosts

| Attribute | Boost | Method |
|-----------|-------|--------|
| Movement Speed | +50% | CharacterMovement direct |
| Jump Height | +30% | CharacterMovement direct |
| Attack Rating | +100 | GE_SymbioteBoost modifier |
| Stamina Regen Rate | +10000 (Infinite) | GE_SymbioteBoost modifier |

#### 2.4.6) Abilities

| Ability | Type | Input |
|---------|------|-------|
| GA_FatherSymbiote | Form Activation | T (Wheel) when charged |
| GA_ProximityStrike | Passive | Auto (enemies nearby) |

#### 2.4.7) Proximity Strike Parameters

| Parameter | Value |
|-----------|-------|
| Damage | 40 per tick |
| Radius | 350 units |
| Tick Rate | Every 0.5 seconds |
| Max Targets | Unlimited |

#### 2.4.8) Gameplay Loop

| Step | Description |
|------|-------------|
| 1 | Build charge through combat (any form) |
| 2 | Reach 100% charge |
| 3 | Identify large enemy group or boss fight |
| 4 | Activate Symbiote (T) |
| 5 | Dive into enemies |
| 6 | Proximity Strike damages all nearby |
| 7 | Use movement boosts to stay in optimal range |
| 8 | Duration expires -> Auto-return to Armor form |

#### 2.4.9) Use Cases

| Scenario | Effectiveness |
|----------|---------------|
| Overwhelming enemies | 10/10 |
| Boss damage phase | 10/10 |
| Crowd clearing | 10/10 |
| Sustained play | 4/10 - Limited duration |
| Stealth approach | 0/10 - Too aggressive |

---

### 2.5) ENGINEER (Stationary Defense)

#### 2.5.1) Identity

| Aspect | Description |
|--------|-------------|
| Archetype | Tactician / Artillery |
| Fantasy | "This position is locked down" |
| Attachment | None - Father deploys as stationary turret |
| Player Input | Deploy only (form selection) |
| Best For | Defending positions, watching flanks, area denial |

#### 2.5.2) Deployment

| Aspect | Description |
|--------|-------------|
| Activation | T (Wheel) -> Father deploys at aimed location |
| Position | Fixed until form change |
| Rotation | 270 degrees coverage arc |
| Health | Father has health pool as turret |

#### 2.5.3) AI Behavior

| Step | Description |
|------|-------------|
| 1 | Enemy Detected: Engineer turret detects enemy within range |
| 2 | Check Distance: Measure distance to enemy |
| 3a | If FAR (>500 units): SHOOT with turret weapon |
| 3b | If CLOSE (<500 units): Deploy TRAP between self and enemy, then SHOOT |

#### 2.5.4) AI Parameters

| Parameter | Value |
|-----------|-------|
| Detection Range | 1500 units |
| Shoot Range | 500-1500 units |
| Trap Range | <500 units |
| Trap Cooldown | 8 seconds |
| Max Active Traps | 2 |
| Rotation Arc | 270 degrees |

#### 2.5.5) Abilities

| Ability | Type | Trigger |
|---------|------|---------|
| GA_FatherEngineer | Form (Deploy) | T (Wheel) |
| GA_TurretShoot | AI | Automatic (far enemies) |
| GA_FatherElectricTrap | AI | Automatic (close enemies) |
| GA_FatherMark | Passive | On enemy detection |

#### 2.5.6) Electric Trap Behavior

| Step | Description |
|------|-------------|
| 1 | Enemy moves within 500 units of Engineer turret |
| 2 | Engineer detects enemy proximity |
| 3 | Engineer deploys electric trap between itself and enemy |
| 4 | Enemy walks into trap |
| 5 | Trap applies damage plus root effect (Narrative.State.Movement.Lock) |
| 6 | Engineer shoots rooted enemy with turret weapon |

#### 2.5.7) Use Cases

| Scenario | Effectiveness |
|----------|---------------|
| Defending objective | 10/10 |
| Watching flank/rear | 10/10 |
| Chokepoint control | 10/10 |
| Wave defense | 10/10 |
| Mobile combat | 2/10 |
| Boss fight (mobile) | 2/10 |

---

## 3) GENERAL ABILITIES

### 3.1) GA_Backstab (Player Default Ability)

GA_Backstab is a Player Default Ability, not granted by the father. Backstab is a standard action game mechanic available to all players.

| Aspect | Description |
|--------|-------------|
| Type | Passive (always active) |
| Trigger | Player attacks enemy from behind |
| Detection | AIPerception ViewedCharacter Query (enemy not viewing player) |
| Grant Location | Player Default Abilities array |
| Father Relationship | Father can distract enemies, enabling backstab opportunities |

#### 3.1.1) Damage Scaling

| Condition | AttackRating Bonus |
|-----------|-------------------|
| Any form, from behind | +25 AttackRating |
| Exoskeleton Stealth active | +50 AttackRating |
| Stealth + from behind | +75 AttackRating (stacked additively) |

---

### 3.2) GA_FatherSacrifice

| Aspect | Description |
|--------|-------------|
| Type | Passive (automatic trigger) |
| Trigger | Player HP drops below 15% |
| Available In | All forms |
| Result | Father sacrifices energy, enters dormant state |

#### 3.2.1) Sacrifice Flow

| Phase | Description |
|-------|-------------|
| **PHASE 1: TRIGGER** | Triggered when player HP drops below 15%. Current form breaks immediately, father moves to chest (Armor position), energy surge begins. |
| **PHASE 2: SACRIFICE** | Father channels all remaining energy to grant player 8 seconds of invulnerability. State.Invulnerable tag blocks all damage. Father enters DORMANT state. Attack tokens increase from 3 to 6. |
| **PHASE 3: DORMANT STATE** | Father remains attached to chest appearing dark and lifeless. All lights OFF, energy lines DARK, eyes UNLIT. HUD completely HIDDEN. Duration: 150-210 seconds (variable). Player fights alone without father abilities. |
| **PHASE 4: REACTIVATION** | Subtle eye flicker signals reactivation. Energy lines power up with wave effect, eyes illuminate. Full Armor form activates, HUD reappears. Attack tokens normalize from 6 back to 3. Reactivation sound effect plays. |

#### 3.2.2) Dormant State Parameters

| Parameter | Value |
|-----------|-------|
| Invulnerability Duration | 8 seconds |
| Dormant Duration | 150-210 seconds (random) |
| Countdown Visible | No (hidden from player) |
| HUD Visible | No (completely removed) |
| Attack Tokens | Increased to 5-6 |
| Father Position | Chest (Armor position) |
| Father Visual | Dark, lifeless, unlit |
| Recovery Form | Armor |

---

## 4) VISUAL DESIGN LANGUAGE

### 4.1) Core Visual Elements

| Element | Present In | Purpose |
|---------|------------|---------|
| Mechanical Plates | Armor, Exoskeleton, Symbiote | Physical structure |
| Energy Conduits | All attached forms | Power flow visualization |
| Father Eyes | All forms | Life/status indicator |
| Core Glow | All forms | Energy level indicator |

### 4.2) Color Language

| Color | Meaning | Used In |
|-------|---------|---------|
| Blue/White | Protection/Shield energy | Armor |
| Orange/Yellow | Power/Speed energy | Exoskeleton |
| Purple/Black | Symbiote energy | Symbiote |
| Green | Targeting/Marking | Crawler, Engineer |
| Red | Warning/Low energy | All forms |
| Dark/Off | Offline/Dormant | Shutdown state |

### 4.3) Form Visual Summary

| Form | Father Position | Visual Style |
|------|-----------------|--------------|
| Crawler | Following nearby | Mobile father unit |
| Armor | Chest | Heavy plates + Energy shield |
| Exoskeleton | Back/Spine | Light plates + Energy conduits |
| Symbiote | Full body | Complete merge + Energy veins |
| Engineer | Deployed | Turret mode |
| Dormant | Chest | Dark plates, no energy |

---

## 5) CONTROL SCHEME

### 5.1) Form Selection (T Key)

| Action | Result |
|--------|--------|
| T (Hold) | Open form wheel |
| T + Direction | Select form |
| T (Release) | Confirm selection |
| T (Tap) | Quick cycle to next form |

### 5.2) Form Wheel Layout

Form wheel displays 5 forms arranged radially: Armor (top), Symbiote (left), Engineer (right), Exoskeleton (bottom-left), Crawler (bottom-right). Player holds T key and moves mouse/stick in direction of desired form, then releases T to confirm selection.

### 5.3) Per-Form Input Mapping

| Form | Q (Tap) | Q (Hold) | E |
|------|---------|----------|---|
| Crawler | - | - | - |
| Armor | Dome Burst | - | - |
| Exoskeleton | Dash | Sprint | Stealth |
| Symbiote | - | - | - |
| Engineer | - | - | - |

---

## 6) HUD DESIGN

### 6.1) Father Status Panel (Bottom Left)

| Form | Display |
|------|---------|
| Crawler | [Father] CRAWLER + Mark indicator |
| Armor | [Shield] ARMOR + Shield bar |
| Exoskeleton | [Speed] EXOSKELETON + Stealth cooldown |
| Symbiote | [Merge] SYMBIOTE + Duration timer |
| Engineer | [Turret] ENGINEER + Turret health |

### 6.2) Symbiote Charge Bar

Symbiote charge bar displays when not in Symbiote form showing current charge percentage (e.g., 80%). At 100% charge, bar displays "READY" text with pulsing/glowing visual effect to indicate ultimate ability is available for activation.

### 6.3) Enemy Mark Indicator

Marked enemies display three visual indicators: icon above their head, outline visible through walls for tracking, and damage bonus indicator showing +10% bonus damage.

---

## 7) GAMEPLAY TAGS

### 7.1) Form Tags

Father.Form.Crawler
Father.Form.Armor
Father.Form.Exoskeleton
Father.Form.Symbiote
Father.Form.Engineer

### 7.2) State Tags

Father.State.Attached
Father.State.Detached
Father.State.Following
Father.State.Deployed
Father.State.Merged
Father.State.Offline
Father.State.Stealthed
Father.State.Alive
Father.State.Transitioning
Father.State.SymbioteLocked
Father.State.Recruited

### 7.3) Ability Tags

Ability.Father.Crawler
Ability.Father.Armor
Ability.Father.Exoskeleton
Ability.Father.Symbiote
Ability.Father.Engineer
Ability.Father.Attack
Ability.Father.LaserShot
Ability.Father.Mark
Ability.Father.ProtectiveDome
Ability.Father.DomeBurst
Ability.Father.Dash
Ability.Father.Sprint
Ability.Father.StealthField
Ability.Father.Backstab
Ability.Father.ProximityStrike
Ability.Father.TurretShoot
Ability.Father.ElectricTrap
Ability.Father.Sacrifice

### 7.4) Effect Tags

Effect.Father.Mark
Effect.Father.Shield
Effect.Father.Stealth
Effect.Father.ProximityDamage

### 7.5) Cooldown Tags

Cooldown.Father.Dash
Cooldown.Father.Stealth
Cooldown.Father.DomeBurst
Cooldown.Father.ElectricTrap
Cooldown.Father.Symbiote
Cooldown.Father.Sacrifice
Cooldown.Father.FormChange

### 7.6) Input Tags

Narrative.Input.Father.FormChange
Narrative.Input.Father.Ability1      (Q Tap)
Narrative.Input.Father.Ability2      (Q Hold)
Narrative.Input.Father.Ability3      (E)

---

## 8) ABILITY SUMMARY

### 8.1) Complete Ability List

| # | Ability | Form | Type | Input |
|---|---------|------|------|-------|
| 1 | GA_FatherCrawler | Crawler | Form | T |
| 2 | GA_FatherAttack | Crawler | AI | Auto |
| 3 | GA_FatherLaserShot | Crawler | AI | Auto |
| 4 | GA_FatherMark | Crawler/Engineer | Passive | Auto |
| 5 | GA_FatherArmor | Armor | Form | T |
| 6 | GA_ProtectiveDome | Armor | Passive | Auto |
| 7 | GA_DomeBurst | Armor | Active | Q |
| 8 | GA_FatherExoskeleton | Exoskeleton | Form | T |
| 9 | GA_FatherExoskeletonDash | Exoskeleton | Active | Q (Tap) |
| 10 | GA_FatherExoskeletonSprint | Exoskeleton | Active | Q (Hold) |
| 11 | GA_StealthField | Exoskeleton | Active | E |
| 12 | GA_FatherSymbiote | Symbiote | Form | T |
| 13 | GA_ProximityStrike | Symbiote | Passive | Auto |
| 14 | GA_FatherEngineer | Engineer | Form | T |
| 15 | GA_TurretShoot | Engineer | AI | Auto |
| 16 | GA_FatherElectricTrap | Engineer | AI | Auto |
| 17 | GA_FatherSacrifice | General | Passive | Auto |

**Total: 17 Father Abilities**

**Note**: GA_Backstab is a Player Default Ability (not father-granted). See Section 3.1 for details.

### 8.2) Abilities by Form

| Form | Count | Abilities |
|------|-------|-----------|
| Crawler | 4 | Crawler, Attack, LaserShot, Mark |
| Armor | 3 | Armor, ProtectiveDome, DomeBurst |
| Exoskeleton | 4 | Exoskeleton, Dash, Sprint, StealthField |
| Symbiote | 2 | Symbiote, ProximityStrike |
| Engineer | 4 | Engineer, TurretShoot, FatherElectricTrap, Mark |
| General | 1 | Sacrifice |
| Player Default | 1 | Backstab |

---

## 9) BALANCE PARAMETERS

### 9.1) Form Comparison Matrix

| Aspect | Crawler | Armor | Exoskeleton | Symbiote | Engineer |
|--------|---------|-------|-------------|----------|----------|
| Damage Output | 6/10 | 4/10 | 8/10 | 10/10 | 6/10 |
| Survivability | 4/10 | 10/10 | 6/10 | 8/10 | 4/10 |
| Mobility | 6/10 | 4/10 | 10/10 | 6/10 | 2/10 |
| Area Control | 4/10 | 6/10 | 2/10 | 8/10 | 10/10 |
| Player Input | 0/10 | 2/10 | 10/10 | 6/10 | 0/10 |

### 9.2) Attack Token Configuration

| State | Attack Tokens | Notes |
|-------|---------------|-------|
| Crawler | 3 | Father draws some aggro |
| Armor | 3 | Player tanks |
| Exoskeleton | 3 | Player evades |
| Symbiote | 4 | Berserker handles crowds |
| Engineer | 3 | Turret handles some |
| Father Offline | 5-6 | No companion support |

### 9.3) Cooldown Summary

| Ability | Cooldown |
|---------|----------|
| Dash | 3 seconds |
| Stealth | 15 seconds |
| Dome Burst | 12 seconds |
| Electric Trap | 8 seconds |
| Symbiote (post-form) | 120 seconds |
| Sacrifice | 150-210 seconds |
| Form Change | 15 seconds |

---

## 10) IMPLEMENTATION ARCHITECTURE

### 10.1) Effect Cleanup Pattern

| Ability Type | Cleanup Method | Handler |
|--------------|----------------|---------|
| All Form Abilities | EndAbility restores stats | Each form ability |
| Duration Abilities | Timer-based end | Ability internal timer |

### 10.2) Form Ability Lifecycle

| Property | Configuration | Reason |
|----------|---------------|--------|
| Commit Ability Node | REMOVE | No cost on form abilities |
| End Ability Node | After transition completes | Cleanup stats and effects |
| Cancel Abilities With Tag | All other form tags | Mutual exclusion |

### 10.3) Cancel Abilities Configuration

> **AUDIT NOTE (v2.5 - 2026-01-23):** C_SYMBIOTE_STRICT_CANCEL contract implemented. Symbiote is an ultimate ability (30s duration) that cannot be cancelled by player-initiated form changes. `Ability.Father.Symbiote` removed from all cancel lists except GA_FatherSacrifice (emergency override). See LOCKED_CONTRACTS.md Contract 11.

| Form Ability | Cancel Abilities with Tag |
|--------------|--------------------------|
| GA_FatherCrawler | Armor, Exoskeleton, Engineer |
| GA_FatherArmor | Crawler, Exoskeleton, Engineer |
| GA_FatherExoskeleton | Crawler, Armor, Engineer |
| GA_FatherSymbiote | Crawler, Armor, Exoskeleton, Engineer |
| GA_FatherEngineer | Crawler, Armor, Exoskeleton |
| GA_FatherRifle | Crawler, Armor, Exoskeleton, Engineer, Sword |
| GA_FatherSword | Crawler, Armor, Exoskeleton, Engineer, Rifle |

**Note:** Symbiote is NOT in cancel lists (except GA_FatherSacrifice). During the 30-second Symbiote duration, `Father.State.SymbioteLocked` blocks activation of all other form/weapon abilities.

### 10.4) Movement Speed Modifications

| Method | Usage |
|--------|-------|
| CharacterMovement Component Direct | All movement speed changes |
| Attribute-Based | NOT USED (NarrativeAttributeSetBase lacks MovementSpeed) |

### 10.5) Damage System

| Ability Type | Damage Method |
|--------------|---------------|
| All Father Attacks | NarrativeDamageExecCalc |
| ProximityStrike | NarrativeDamageExecCalc with SetByCaller |
| ElectricTrap | NarrativeDamageExecCalc |
| TurretShoot | NarrativeDamageExecCalc |
| DomeBurst | NarrativeDamageExecCalc |

### 10.6) Form Transition System

#### 10.6.1) Transition Flow

| Step | Action | Duration |
|------|--------|----------|
| 1 | Player selects new form via T wheel | - |
| 2 | GAS checks Activation Required/Blocked Tags | Instant |
| 3 | If blocked: nothing happens, no cooldown | - |
| 4 | If allowed: New form ability activates | - |
| 5 | Add Father.State.Transitioning tag (via AddLooseGameplayTag) | - |
| 6 | Cancel Abilities With Tag cancels old form | - |
| 7 | Old form EndAbility restores stats (instant) | - |
| 8 | Spawn Niagara transition VFX | - |
| 9 | Wait for VFX duration | 5s |
| 10 | Reposition father to new socket/location | - |
| 11 | Apply new form stat effects | - |
| 12 | Set CurrentForm variable | - |
| 13 | Remove Father.State.Transitioning tag (via RemoveLooseGameplayTag) | - |
| 14 | Apply Cooldown.Father.FormChange (15s) | - |
| 15 | End ability (or stay active for Symbiote) | - |

#### 10.6.2) Transition Parameters

| Parameter | Value |
|-----------|-------|
| VFX Duration | 5 seconds |
| Father Invulnerable During Transition | No (removed per GAS Audit INV-1) |
| Player Movement During Transition | Allowed |
| Player Combat During Transition | Allowed |
| Cancel Transition | Not allowed |
| Form Cooldown | 15 seconds (after transition completes) |

#### 10.6.3) Tag Configuration (All Form Abilities)

| Property | Tags |
|----------|------|
| Activation Required Tags | Father.State.Alive, Father.State.Recruited |
| Activation Blocked Tags | Father.State.Dormant, Father.State.Transitioning, Father.State.SymbioteLocked |
| Cooldown Tags | Cooldown.Father.FormChange |

#### 10.6.4) Symbiote Special Handling (C_SYMBIOTE_STRICT_CANCEL v4.28.2)

> **AUDIT NOTE (v2.5 - 2026-01-23):** GE_SymbioteDuration with SetByCaller pattern implemented at activation START. This grants `Father.State.SymbioteLocked` for 30 seconds, blocking all player-initiated form/weapon abilities. GA_FatherSacrifice is the ONLY ability that can cancel Symbiote (emergency override). See LOCKED_CONTRACTS.md Contract 11.

| Step | Action |
|------|--------|
| 1-5 | Same as normal transition (validation, cancel old form) |
| 6 | **Apply GE_SymbioteDuration via SetByCaller** (grants `Father.State.SymbioteLocked` for 30s) |
| 7-16 | Continue normal transition (VFX, repositioning, stat effects) |
| 17 | Ability stays active (does NOT end) |
| 18 | Start 30 second duration timer (synced with GE_SymbioteDuration) |
| 19 | Timer completes |
| 20 | GE_SymbioteDuration expires automatically (removes `Father.State.SymbioteLocked`) |
| 21 | Auto-activate GA_FatherArmor |
| 22 | Normal Armor transition begins |

**GE_SymbioteDuration SetByCaller Flow:**
1. `MakeOutgoingGameplayEffectSpec(GE_SymbioteDuration)`
2. `AssignTagSetByCallerMagnitude(SpecHandle, Data.Symbiote.Duration, 30.0)`
3. `K2_ApplyGameplayEffectSpecToOwner(SpecHandle)`

**Defense-in-Depth Strategy:**
- **Layer 1 (Blocking):** `Father.State.SymbioteLocked` in activation_blocked_tags blocks ability activation
- **Layer 2 (No Cancel):** Symbiote NOT in cancel_abilities_with_tag ensures abilities won't forcibly cancel it
- **Layer 3 (Duration):** GE_SymbioteDuration auto-expires, removing the lock tag

---

## 11) REMOVED CONTENT

### 11.1) Shield Form (Removed)

| Factor | Explanation |
|--------|-------------|
| Redundancy | Narrative Pro has built-in shield slot |
| Form clarity | Shield overlapped with Armor (both defensive) |
| Player equipment | Shield abilities fit better as player gear |

### 11.2) Removed Abilities

| Ability | Reason |
|---------|--------|
| GA_FatherShield | Shield form removed |
| GA_ShieldThrow | Moved to player shield equipment |
| GA_ShieldLightning | Moved to player shield equipment |
| GA_FatherDetach | Direct form-to-form switching via T wheel |
| GA_TurretRecall | Cancel Abilities With Tag handles cleanup |

---

## 12) LOCKED DECISIONS REFERENCE (January 2026 Audit)

The following decisions were locked during Claude-GPT dual-agent audit (January 2026). These are FINAL and should not be reopened for debate.

### 12.1) Implementation Constraints

| ID | Constraint | Rationale |
|----|-----------|-----------|
| LC-1 | No Manual Blueprint Edits | All changes via manifest.yaml + regeneration |
| LC-2 | No UE Source Modification | Project uses stock engine binaries |
| LC-3 | No C++ GameplayAbility | Blueprint-only, generated via manifest |
| LC-4 | Process Lock | Research → Audit → Decide → Implement |

### 12.2) Invulnerability Decision (INV-1)

**REMOVED:** All Father invulnerability (transitions, dash i-frames, GE_Invulnerable)

**KEPT:** GA_FatherSacrifice 8-second PLAYER invulnerability only

**Rationale:** Invulnerability was never intended in Father design - discovered as oversight during audit.

### 12.3) EndAbility Lifecycle Rules

| Rule | Abilities | Pattern |
|------|-----------|---------|
| Rule 1: Instant | GA_FatherAttack, GA_DomeBurst, GA_ProximityStrike, GA_TurretShoot, GA_FatherLaserShot, GA_FatherElectricTrap | Event_Activate → Logic → K2_EndAbility |
| Rule 2: Delay/Timer | Form abilities, GA_FatherExoskeletonDash, GA_StealthField | MUST have Event_EndAbility + guards |
| Rule 3: Toggle | GA_ProtectiveDome, GA_FatherExoskeletonSprint, GA_FatherRifle, GA_FatherSword | Stay active until cancelled, Event_EndAbility for cleanup |
| Rule 4: First Activation | Form abilities | True path MUST merge into same setup chain as False path |

### 12.4) Key Technical Findings

| ID | Finding | Impact |
|----|---------|--------|
| VTF-1 | Delay node does NOT terminate on EndAbility | Use AbilityTaskWaitDelay instead |
| VTF-2 | ActivationOwnedTags auto-removed on EndAbility | Can use as guard proxy |
| VTF-3 | Activation Required Tags NOT continuously checked | Use Cancel Abilities With Tag for cascade |
| VTF-5 | No IsActive() Blueprint node | Use tag guards instead |
| VTF-7 | CommitCooldown requires explicit call | Cooldown GE not applied otherwise |
| VTF-8 | SetByCaller requires matching GE modifier | Ensure GE has modifier reading tag |

### 12.5) Orphan Removals

| Asset | Reason |
|-------|--------|
| GE_ArmorBoost | No references in abilities or equipment |
| GE_SymbioteBoost | No references in abilities or equipment |
| GE_TransitionInvulnerability | INV-1 removal |
| GE_DashInvulnerability | INV-1 removal |

**Full Audit Document:** `ClaudeContext/Handoffs/Father_Companion_GAS_Abilities_Audit.md`

---

## 13) DOCUMENT HISTORY

| Version | Date | Changes |
|---------|------|---------|
| 2.5 | January 2026 | **C_SYMBIOTE_STRICT_CANCEL Contract (LOCKED_CONTRACTS.md Contract 11):** Section 10.3 Cancel Abilities Configuration updated - removed `Ability.Father.Symbiote` from all form/weapon cancel lists (exception: GA_FatherSacrifice). Added GA_FatherRifle and GA_FatherSword to cancel table. Section 10.6.4 Symbiote Special Handling rewritten - GE_SymbioteDuration with SetByCaller pattern applied at activation START, grants `Father.State.SymbioteLocked` for 30s. Defense-in-depth strategy documented (3 layers: blocking, no-cancel, duration enforcement). Claude-GPT dual audit 2026-01-23. |
| 2.3 | January 2026 | **Dome System Locked Decisions (21 items)**: Sections 2.2.5-2.2.9 rewritten with Claude-GPT dual-audit locked decisions. Energy-Only damage model (player takes full damage), energy on Player ASC (AS_DomeAttributes), manual release only at full (500), no auto-burst, no pre-release, flat 75 damage, 12s cooldown, form exit burst behavior, FullyCharged tag semantics, GA_ProtectiveDome reset ownership. Section 2.2.6 Use Cases renumbered to 2.2.10. Cooldown Summary updated: Dome Burst 10s→12s. |
| 2.2 | January 2026 | **Added Section 12: Locked Decisions Reference** - Consolidated all locked decisions from Claude-GPT dual-agent audit (January 2026). Includes LC-1 to LC-4 implementation constraints, INV-1 invulnerability removal, EndAbility lifecycle rules (Rules 1-4), and VTF technical findings. |
| 2.1 | January 2026 | **GAS Audit INV-1 Compliance:** Removed all unintended invulnerability per dual-agent audit decision. Section 2.3.3: Exoskeleton Dash I-Frames changed from "Yes" to "No (removed per GAS Audit INV-1)". Section 10.6.1: Removed GE_Invulnerable steps (old steps 6, 15), updated to use AddLooseGameplayTag/RemoveLooseGameplayTag for Father.State.Transitioning. Section 10.6.2: Father Invulnerable During Transition changed from "Yes" to "No (removed per GAS Audit INV-1)". **KEPT:** GA_FatherSacrifice 8-second PLAYER invulnerability (intentional design). |
| 2.0 | January 2026 | Renamed from Spider to Father throughout entire document. All tags, abilities, references updated. |
| 1.9 | January 2026 | Updated Narrative Pro version from v2.1 to v2.2. Fixed ability names: GA_ExoskeletonDash to GA_FatherExoskeletonDash, GA_ExoskeletonSprint to GA_FatherExoskeletonSprint (matching implementation guides). Fixed tag format: Father.State.* changed to Father.State.* per DefaultGameplayTags_FatherCompanion_v4_0.ini (affected Alive, Dormant, Transitioning, SymbioteLocked tags in Sections 10.6.1, 10.6.3, 10.6.4). |
| 1.8 | January 2026 | Major compaction and corrections. Fixed Armor defense to +50 Armor (was incorrectly showing 30% reduction). Added Father.State.Recruited to Section 7.2 and 10.6.3. Fixed duplicate section numbering 2.1.5 (now 2.1.5 and 2.1.6). Standardized Symbiote post-form cooldown to 120 seconds. Removed Implementation Phases section (work in progress). Removed Future Considerations section. Removed Pending Decisions section. Added Form Change to Cooldown Summary. Simplified Visual Design Language and HUD sections. Added stat boost column to Form Overview table. |
| 1.7 | December 2025 | Moved GA_Backstab from father ability to Player Default Ability. Backstab is a generic action game mechanic available to all players, not dependent on father recruitment. Updated Section 3.1 with Player Default grant location. Updated Section 8.1 ability table (17 father abilities + 1 player ability). Updated Section 8.2 with "Moved to Player" status. Updated Section 8.3 with "Player Default" category. Father still enables backstab opportunities by distracting enemies. |
| 2.4 | January 2026 | Dome System Decisions 22-24: Form exit burst via EI_FatherArmorForm.HandleUnequip override calling TryActivateAbilityByClass(GA_DomeBurst). Father.Dome.FullyCharged tag gates burst activation (FULL only, no partial release). Energy-Only damage model documented. Added section 2.2.10 Form Exit Burst Implementation. |
| 1.6 | December 2025 | Major architecture change: Removed F key detach system entirely. GA_FatherDetach and GA_TurretRecall abilities removed (21->18 abilities). Form switching now direct via T wheel only. Added Form Transition System with 5s Niagara VFX, father invulnerability during transition, 15s shared cooldown. Updated Symbiote duration from 15s to 30s, locked during duration, auto-returns to Armor form. Added new state tags (Alive, Transitioning, SymbioteLocked). |
| 1.5 | 2025 | Removed Shield +200 from Symbiote form stat boosts. AS_SymbioteAttributes not needed - shield system removed due to Blueprint limitation. Symbiote form now focuses purely on offensive stat boosts. |
| 1.4 | 2025 | Changed GA_FatherSacrifice from shield-based protection to 8-second invulnerability. Removed shield system due to Blueprint architectural limitations. State.Invulnerable tag now blocks all damage during sacrifice. |
| 1.3 | 2025 | Applied user-confirmed decisions from implementation guides. Changed Symbiote Duration 22s to 15s. Changed ProximityStrike damage 50 to 40, radius 400 to 350, removed knockback. Changed Backstab detection to AIPerception ViewedCharacter Query. Changed Backstab damage to AttackRating modifiers (+25/+50/+75). Changed Exoskeleton stat from 10% damage reduction to +10 AttackRating. Changed ElectricTrap effect to root (Movement.Lock). Added GA_FatherAttack parameters. Added Implementation Architecture section. |
| 1.2 | 2025 | Removed cancelled Symbiote abilities (GA_TendrilLash, GA_Consume, GA_SymbioteLifesteal). Simplified Symbiote form to 2 abilities. Updated total ability count from 23 to 21. |
| 1.1 | 2025 | Corrected ability names (kept original names: GA_StealthField, GA_FatherElectricTrap) |
| 1.0 | 2025 | Initial design document |

---

## AUDIT STAMP

| Field | Value |
|-------|-------|
| Audit Date | 2026-01-23 |
| Audit Type | Claude-GPT Dual Audit |
| Decisions Locked | 25 (Dome Energy System + C_SYMBIOTE_STRICT_CANCEL) |
| Key Decisions | D22: Form exit via TryActivateAbilityByClass |
| | D23: EI_FatherArmorForm.HandleUnequip override |
| | D24: Father.Dome.FullyCharged activation_required_tag |
| | **Contract 11: C_SYMBIOTE_STRICT_CANCEL** |
| Manifest Aligned | Yes |
| Contract Reference | LOCKED_CONTRACTS.md v4.28.2 |

---

# END OF DOCUMENT
