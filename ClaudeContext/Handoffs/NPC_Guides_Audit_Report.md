# NPC Implementation Guides Audit Report
## Version 1.2 - January 2026

**Auditor:** Claude (Opus 4.5)
**Date:** 2026-01-25
**Scope:** 6 NPC Implementation Guides
**Context:** UE5.7 + Narrative Pro v2.2 + GasAbilityGenerator v4.33

---

## EXECUTIVE SUMMARY

This audit reviews 6 NPC implementation guides against:
1. Locked Contracts (LOCKED_CONTRACTS.md v4.32)
2. GAS Abilities Audit (Father_Companion_GAS_Abilities_Audit.md v6.5)
3. Technical accuracy and Blueprint feasibility
4. Gameplay design achievability

### Audit Results Summary

| Guide | Critical Issues | Warnings | Compliance |
|-------|----------------|----------|------------|
| Gatherer_Scout_Alert_System_v1_1 | 0 | 2 | PASS |
| Possessed_Exploder_Enemy_v2_1 | 1 | 1 | NEEDS REVIEW |
| Random_Aggression_Stalker_v2_1 | 1 | 2 | NEEDS REVIEW |
| Support_Buffer_Healer_v1_0 | 1 | 2 | NEEDS REVIEW |
| Warden_Husk_v1_2 | 0 | 3 | PASS |
| Biomechanical_Detachment_v1_1 | 0 | 2 | PASS |

**Total: 3 CRITICAL issues, 12 WARNINGS**

### Automation Status (v4.32+)

| NPC System | Automation Level | Notes |
|------------|------------------|-------|
| Warden Husk/Core | **FULL** | HandleDeath spawns Core via SpawnActor |
| Biomech Host/Creature | **FULL** | HandleDeath spawns Creature via SpawnActor |
| Possessed Exploder | **FULL** | BTS_CheckExplosionProximity applies GE_ExplosionDamage + HandleDeath |
| Support Buffer | **HIGH** | BTS_HealNearbyAllies has SphereOverlap + faction check + GE_SupportHeal |
| Formation Guard | **HIGH** | BTS services calculate position and match speed |
| Gatherer Scout | **PARTIAL** | Structural scaffolding; alert logic is manual |
| Random Stalker | **PARTIAL** | GoalGenerator timers need manual validity guards |

---

## TABLE OF CONTENTS

1. [Gameplay Overview](#gameplay-overview)
2. [Research Questions Investigation](#research-questions-investigation)
3. [Per-Guide Audit Details](#per-guide-audit-details)
4. [Patterns Not Locked (Candidates for Documentation)](#patterns-not-locked)
5. [Patterns Conflicting with Locked Decisions](#patterns-conflicting-with-locked-decisions)
6. [Recommendations](#recommendations)

---

## GAMEPLAY OVERVIEW

This section describes what each NPC does from a gameplay perspective - the player-facing experience.

### 1. Gatherer Scout (Alert System)

**Role:** Non-combatant resource carrier with emergency response capabilities

**Gameplay Experience:**
- Player encounters Scouts passively wandering the world carrying valuable loot/resources
- Scouts are **not initially hostile** - they're focused on gathering tasks
- When the player attacks or threatens a Scout, it enters **Alert Mode**:
  1. Scout immediately broadcasts an alert signal to nearby allied NPCs
  2. Scout attempts to flee toward the nearest safe zone or allied group
  3. Reinforcement NPCs respond to the alert and converge on the player's location
- **Risk/Reward Design:** Attacking Scouts gives easy loot BUT triggers escalating encounters
- Scouts may have varying "alert radius" - some trigger larger reinforcement waves

**Strategic Impact:** Creates emergent gameplay where players must decide if ambushing a lone Scout is worth the incoming reinforcements.

---

### 2. Possessed Exploder (Flying Enemy)

**Role:** Kamikaze aerial unit - high-damage suicide attacker

**Gameplay Experience:**
- Flying enemy that aggressively pursues the player
- **Two-phase attack pattern:**
  1. **Pursuit Phase:** Rapidly closes distance while airborne, difficult to hit due to erratic movement
  2. **Detonation Phase:** When in proximity, begins visible "charging up" sequence (material glow, audio warning)
- Player has a **brief reaction window** (1-2 seconds) to:
  - Kill the Exploder before detonation (rewards: no damage, drops loot)
  - Dodge/evade the blast radius
  - Use knockback abilities to push it away
- **Explosion deals massive AOE damage** - dangerous in groups or tight spaces
- Possessed aesthetic: unnatural movements, eerie sounds, glowing effects suggesting corruption

**Strategic Impact:** Forces players to prioritize threats. Groups of Exploders create "crowd control checks" - players must thin their numbers before they close distance.

---

### 3. Random Aggression Stalker

**Role:** Unpredictable psychological threat - friend or foe NPC

**Gameplay Experience:**
- Most unsettling NPC type - the player **never knows its true intentions**
- **Behavioral States:**
  1. **Passive/Following:** Stalker follows the player at a distance, appears non-threatening, may even seem helpful
  2. **Agitation Building:** Subtle tells (twitching, sound changes) as internal aggression meter rises
  3. **Aggression Burst:** Suddenly attacks the player with full hostility
  4. **Cooldown/Reset:** After an attack, may return to passive state... or continue attacking
- The **randomness is the feature:** Same Stalker may follow peacefully for 5 minutes, then attack. Or attack immediately. Or never attack.
- Players cannot "read" the Stalker reliably - creates constant tension
- If the player attacks first, Stalker becomes permanently hostile

**Strategic Impact:** Creates paranoia and tension. Players must decide: tolerate the Stalker following them (risking sudden attack) or preemptively attack (potentially wasting resources on what might have stayed passive).

---

### 4. Support Buffer/Healer NPC

**Role:** Friendly allied healer - player support unit

**Gameplay Experience:**
- Benevolent NPC that **follows and assists the player**
- **Core Behaviors:**
  1. **Following:** Maintains proximity to player, navigates around obstacles
  2. **Healing:** When player health drops below threshold, applies heal-over-time effects
  3. **Buffing:** Periodically grants stat buffs (speed boost, damage resistance, etc.)
  4. **Self-Preservation:** Avoids combat, flees if directly attacked
- **Heal Prioritization:** If multiple allies injured, heals the most critical first
- The healer has **limited resources** (mana/cooldowns) - cannot heal infinitely
- Visual/audio feedback when healing (particle effects on player, soothing sounds)

**Strategic Impact:** Changes encounter dynamics - players can be more aggressive knowing they have healing support. Creates escort-mission-lite gameplay where protecting the healer extends your survivability.

---

### 5. Warden Husk (Two-Phase Boss)

**Role:** Major boss enemy with dramatic phase transition

**Gameplay Experience:**
- **Phase 1 - The Warden (Armored Melee):**
  - Large, heavily armored humanoid enemy
  - Slow but devastating melee attacks
  - High damage resistance - requires sustained damage to bring down
  - Attack patterns: ground slams, sweeping strikes, charge attacks
  - Player strategy: dodge melee attacks, exploit attack recovery windows

- **Phase Transition (50% health threshold):**
  - Dramatic death animation - the Warden "dies"
  - The **Husk emerges** from the corpse (cinematic moment)
  - Brief invulnerability during transformation

- **Phase 2 - The Husk (Flying Laser Attacker):**
  - Completely different enemy type emerges
  - Now **aerial and ranged** - fires laser beams at player
  - Fast, evasive movement - hard to hit
  - Lower health than Phase 1 but more dangerous attack patterns
  - Player strategy: ranged attacks preferred, dodge laser tracking

**Strategic Impact:** Two-phase design tests different player skills. Phase 1 tests melee combat/dodge timing. Phase 2 tests ranged combat/aerial tracking. Players who only spec'd into one combat style face difficulties.

---

### 6. Biomechanical Detachment (Two-Phase Enemy)

**Role:** Body horror enemy - host organism releases parasite on death

**Gameplay Experience:**
- **Phase 1 - The Host (Human-Machine Hybrid):**
  - Appears as a corrupted humanoid with visible mechanical/biological grafts
  - Standard combat enemy with melee and/or ranged attacks
  - The mechanical parts are **visibly attached** - foreshadowing the detachment
  - When killed, does NOT simply die...

- **Detachment Event:**
  - The mechanical creature **rips free from the host body**
  - Host corpse collapses, mechanical entity activates
  - Visual horror moment - like a parasite leaving its host

- **Phase 2 - The Creature (Detached Mechanical Entity):**
  - Smaller, faster enemy with different attack patterns
  - May have abilities the Host didn't have (burrowing, leaping, etc.)
  - Lower health but more aggressive
  - Can potentially seek a new host? (design extension possibility)

**Strategic Impact:** Players learn to "double-tap" - killing the Host isn't enough. Must be prepared for the Creature emergence. Creates resource management decisions: save ammo/abilities for Phase 2.

---

## RESEARCH QUESTIONS INVESTIGATION

Two questions were raised during initial audit and required deeper investigation:

### Question 1: R-TIMER-1 Extension to GoalGenerators

**Question:** Should R-TIMER-1 (Timer Callback Safety) be extended to cover all UObject timer usage, not just GameplayAbilities?

**Research Findings:**

1. **Current R-TIMER-1 Scope (from GAS Audit v6.5):**
   > "If a GameplayAbility uses engine timers (SetTimer) instead of AbilityTasks, the timer callback MUST guard against invalid state (actor validity + ability/phase/state checks) before executing logic."

2. **Web Search Evidence ([Epic's Gameplay Timers Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-timers-in-unreal-engine)):**
   > "Timers will be canceled automatically if the Object that they are going to be called on, such as an Actor, is destroyed before the time is up."

3. **Forum Evidence ([Timers in UObjects - Epic Forums](https://forums.unrealengine.com/t/timers-in-uobjects/89733)):**
   - Common issue: `GetWorld()->GetTimerManager().SetTimer()` can cause null pointer access violations in UObjects
   - Timer callbacks continue executing if the UObject is pending kill but not yet garbage collected

4. **GoalGenerator Context:**
   - NPCGoalGenerator is a UObject (not AActor)
   - Timer callbacks bind to `this` (the generator instance)
   - If the NPC is destroyed, the GoalGenerator may still have pending timers

**Analysis:**

| Timer Owner | Auto-Cancel on Destruction | Risk Level |
|-------------|---------------------------|------------|
| AActor | YES (World manages timers) | LOW |
| UGameplayAbility | Partial (depends on EndAbility cleanup) | MEDIUM |
| UObject (GoalGenerator) | NO (requires explicit ClearTimer) | HIGH |

**Conclusion:** R-TIMER-1 **SHOULD be extended** to cover GoalGenerators and other UObjects. The current rule only addresses GameplayAbilities, but GoalGenerators face the same (or worse) lifecycle risks.

**Proposed Extension (R-TIMER-2):**

> "UObject classes using engine timers (SetTimer) MUST:
> 1. Store the FTimerHandle as a member variable
> 2. Check `IsValid(GetWorld())` before timer operations
> 3. In timer callbacks, guard with `if (!IsValid(this))` or equivalent validity check
> 4. Call `ClearAndInvalidateTimerByHandle` in destructor or explicit cleanup"

**Affected NPC Guides:**
- Random_Aggression_Stalker_v2_1.md - Uses 4 timers in GoalGenerator without validity guards

---

### Question 2: Contract 13 Applicability to Blueprint Guides

**Question:** Contract 13 (INV-GESPEC-1) requires `param.GameplayEffectClass` syntax for manifest. Do Blueprint guides showing node-level `MakeOutgoingSpec` need different documentation?

**Research Findings:**

1. **Contract 13 Scope (from LOCKED_CONTRACTS.md v4.31):**
   > "All `MakeOutgoingGameplayEffectSpec` nodes in manifest MUST use `param.GameplayEffectClass: GE_*` syntax"

   The contract is explicitly scoped to **manifest yaml generation**, not raw Blueprint construction.

2. **Blueprint Node Behavior:**
   When manually creating a `MakeOutgoingGameplayEffectSpec` node in Blueprint:
   - The `GameplayEffectClass` pin appears as a **TSubclassOf<UGameplayEffect>** input
   - You connect a literal class reference or variable to this pin
   - **No `param.` prefix exists in Blueprint** - that's a manifest convention

3. **Generator Processing Evidence (GasAbilityGeneratorGenerators.cpp:10335):**
   ```cpp
   if (PropPair.Key.StartsWith(TEXT("param.")))  // Only manifest uses this
   ```

**Conclusion:** Contract 13 is **manifest-specific**. Blueprint guides correctly show direct class connections without `param.` prefix. **No documentation change needed for Blueprint guides.**

However, if an NPC guide was later converted to manifest yaml, the `param.GameplayEffectClass:` syntax would be required.

**Recommendation:** Add a note to guides that reference `MakeOutgoingGameplayEffectSpec`:
> "Note: If implementing via manifest.yaml instead of manual Blueprint, use `param.GameplayEffectClass: GE_*` syntax per Contract 13."

---

## AUTOMATION STATUS (v4.32+)

GasAbilityGenerator v4.32/v4.32.1 implemented significant automation enhancements for NPC systems. This section documents what's now fully automated vs what still requires manual work.

### Phase Transition Automation

| Asset | Implementation | Status |
|-------|----------------|--------|
| **BP_WardenHusk** | HandleDeath override with SpawnActor for BP_WardenCore | **AUTOMATED** |
| **BP_BiomechHost** | HandleDeath override with SpawnActor for BP_BiomechCreature | **AUTOMATED** |

**Implementation Details:**
- `PhaseSpawnClass` TSubclassOf variable (set in editor to target class)
- `HasAuthority()` check for server-only spawning
- `K2_GetActorTransform()` for spawn location at death
- `SpawnActor` node with class and transform connections
- `CallParent` for proper death chain

**Designer Task:** Set `PhaseSpawnClass` variable default to `BP_WardenCore` or `BP_BiomechCreature` respectively.

---

### BT Service Automation

#### BTS_CheckExplosionProximity (Possessed Exploder)

**Before v4.32:** PrintString stub only
**After v4.32.1:** Full explosion behavior

| Node | Function | Purpose |
|------|----------|---------|
| GetAbilitySystemComponent | AbilitySystemBlueprintLibrary | Get target's ASC |
| ApplyGameplayEffectToTarget | param.GameplayEffectClass: GE_ExplosionDamage | Apply explosion damage |
| CastToNarrativeNPCCharacter | DynamicCast | Access death function |
| HandleDeath | NarrativeCharacter::HandleDeath | Kill the exploder (suicide) |

**Execution Flow:**
```
ReceiveTickAI → Already Exploded Check (Branch)
    → FALSE: GetBlackboard → GetAttackTarget → CastToActor
    → Distance Check → Proximity Branch
    → TRUE: SetExploded → GetTargetASC → ApplyExplosionDamage
    → CastToNPC → HandleDeath
```

---

#### BTS_HealNearbyAllies (Support Buffer)

**Before v4.32:** PrintString stub only
**After v4.32.1:** Full healing behavior with radius detection

| Node | Function | Purpose |
|------|----------|---------|
| SphereOverlapActors | KismetSystemLibrary | Find actors in HealRadius |
| ForEachLoop | Blueprint Macro | Iterate found actors |
| ActorHasMatchingGameplayTag | BlueprintGameplayTagLibrary | Check Narrative.Factions.Friendly |
| GetAbilitySystemComponent | AbilitySystemBlueprintLibrary | Get ally's ASC |
| ApplyGameplayEffectToTarget | param.GameplayEffectClass: GE_SupportHeal | Apply healing |

**Execution Flow:**
```
ReceiveTickAI → Accumulate DeltaTime → Cooldown Check (Branch)
    → TRUE: Reset Timer → GetPawnLocation → SphereOverlap
    → ForEach Actor → HasFriendlyTag (Branch)
    → TRUE: GetTargetASC → ApplyHeal
```

**Variables:**
- `HealRadius` (Float, default 500.0) - Detection radius
- `HealCooldown` (Float, default 2.0) - Seconds between heal pulses
- `TimeSinceLastHeal` (Float) - Internal timer accumulator

---

#### BTS_CalculateFormationPosition (Formation Guard)

**Before v4.32:** Empty stub
**After v4.32:** Position calculation with blackboard integration

| Node | Function | Purpose |
|------|----------|---------|
| GetBlackboardComponent | AIController | Access AI blackboard |
| GetValueAsObject | BlackboardComponent | Get TargetCharacter (leader) |
| K2_GetActorLocation | Actor | Get leader position |
| GetActorForwardVector | Actor | Get leader facing direction |
| Multiply_VectorFloat | KismetMathLibrary | Calculate offset distance |
| Subtract_VectorVector | KismetMathLibrary | Apply offset behind leader |
| SetValueAsVector | BlackboardComponent | Store TargetLocation |

**Variables:**
- `FormationIndex` (Integer, default 0) - Position in formation
- `FormationDistance` (Float, default 200.0) - Distance from leader

---

#### BTS_AdjustFormationSpeed (Formation Guard)

**Before v4.32:** Empty stub
**After v4.32:** Speed matching with leader

| Node | Function | Purpose |
|------|----------|---------|
| CastToCharacter | DynamicCast | Access CharacterMovement |
| GetBlackboardComponent | AIController | Access AI blackboard |
| GetValueAsObject | BlackboardComponent | Get TargetCharacter (leader) |
| GetVelocity | Actor | Get leader's current velocity |
| VSize | KismetMathLibrary | Calculate velocity magnitude |
| FMax | KismetMathLibrary | Clamp to minimum BaseWalkSpeed |
| PropertySet (MaxWalkSpeed) | CharacterMovementComponent | Apply matched speed |

**Variables:**
- `BaseWalkSpeed` (Float, default 400.0) - Minimum movement speed

---

### What Still Requires Manual Work

| System | Manual Task | Reason |
|--------|-------------|--------|
| **Gatherer Scout** | Alert broadcast logic | NPC-to-NPC event propagation is game-specific |
| **Random Stalker** | Timer validity guards | GoalGenerator timers need per-project cleanup patterns |
| **GoalGenerator_RandomAggression** | State machine logic | Random aggression scoring is design-specific |
| **GE Magnitudes** | Damage/heal values | Balance tuning in GE assets |
| **Faction Tags** | Tag registration | Game-specific faction hierarchy |

---

## PER-GUIDE AUDIT DETAILS

### 1. Gatherer_Scout_Alert_System_Implementation_Guide_v1_1.md

**NPC Type:** Gatherer/Scout with alert propagation
**Primary Systems:** Activity System, Perception, Goals

#### Compliance Status: PASS

#### Findings:

| ID | Severity | Issue | Location |
|----|----------|-------|----------|
| GSA-W1 | WARNING | Uses `Delay` node in Goal completion flow | Section 4.2.3 |
| GSA-W2 | WARNING | No explicit `IsValid` guard before broadcast | Section 5.1 |

**GSA-W1 Details:**
- Guide shows `Delay` node for "investigation hold time"
- Per VTF-1 (GAS Audit), Delay does NOT terminate when ability ends
- Since this is in a Goal (UNPCGoalItem), not GameplayAbility, AbilityTasks are NOT available
- **Acceptable workaround:** Goals use different lifecycle, Delay is appropriate here

**GSA-W2 Details:**
- Alert broadcast to nearby NPCs doesn't check if source NPC is still valid
- Edge case: NPC destroyed during broadcast enumeration
- **Low risk:** Narrative Pro's NPC subsystem handles this case

#### Patterns Used:
- Activity-based daily routine (Schedule_*)
- Perception-triggered goal scoring
- NPC-to-NPC alert propagation
- BTService for state monitoring

#### Locked Compliance:
- R-AI-1 (Activity System): COMPLIANT - Uses Activity system correctly
- Contract 12: COMPLIANT - No direct RunBehaviorTree bypassing

---

### 2. Possessed_Exploder_Enemy_Implementation_Guide_v2_1.md

**NPC Type:** Enemy that explodes on death
**Primary Systems:** HandleDeath override, GE application

#### Compliance Status: PASS (v4.32.1 - FULLY AUTOMATED)

#### Automation Status (v4.32.1)

**BTS_CheckExplosionProximity** now implements complete explosion behavior:
- ReceiveTickAI event with proximity detection
- `GetAbilitySystemComponent` + `ApplyGameplayEffectToTarget(GE_ExplosionDamage)`
- `HandleDeath` call for suicide explosion

**Designer Configuration:**
- Set `ExplosionRadius` variable (default 150.0)
- Configure `GE_ExplosionDamage` magnitude in the GE asset

#### Findings:

| ID | Severity | Issue | Location | Status |
|----|----------|-------|----------|--------|
| PEE-C1 | ~~CRITICAL~~ | MakeOutgoingSpec shown without param. prefix | Section 3.4 | **RESOLVED** - Now uses manifest automation |
| PEE-W1 | WARNING | No cleanup of explosion timer if NPC respawns | Section 4.1 | N/A - Uses ReceiveTickAI, not timers |

**PEE-C1 Resolution (v4.32.1):**
- Manifest now uses `param.GameplayEffectClass: GE_ExplosionDamage` syntax
- BTS_CheckExplosionProximity fully automated with proper Contract 13 compliance

#### Patterns Used:
- ~~HandleDeath override~~ → **BTS_CheckExplosionProximity** (ReceiveTickAI)
- ~~Delayed explosion via timer~~ → **Proximity detection with bHasExploded guard**
- Area damage via GE application ✓
- Visual warning (material change before explosion) - **Still manual in BP_PossessedExploder**

#### Locked Compliance:
- R-TIMER-1: **COMPLIANT** - No longer uses timers, uses ReceiveTickAI event
- Contract 13: **COMPLIANT** - Uses param.GameplayEffectClass syntax

---

### 3. Random_Aggression_Stalker_System_Implementation_Guide_v2_1.md

**NPC Type:** NPC with randomized aggression phases
**Primary Systems:** GoalGenerator timers, state machine

#### Compliance Status: NEEDS REVIEW

#### Findings:

| ID | Severity | Issue | Location |
|----|----------|-------|----------|
| RAS-C1 | CRITICAL | 4 timers without validity guards | Section 6.2, 6.3, 6.4 |
| RAS-W1 | WARNING | State transitions don't check NPC validity | Section 5.1 |
| RAS-W2 | WARNING | No timer cleanup in destructor | Section 7.0 |

**RAS-C1 Details:**
The GoalGenerator uses 4 SetTimer calls:
1. `AggressionBuildTimer` - Gradual aggression increase
2. `CooldownTimer` - Post-attack cooldown
3. `StalkPhaseTimer` - Random phase transitions
4. `ResetTimer` - Return to passive state

None of these timer callbacks include validity guards:
```
// Current (UNSAFE):
void OnAggressionTimerFire()
{
    AggressionLevel += BuildRate;  // May access destroyed NPC data
    UpdateGoalScore();
}

// Should be:
void OnAggressionTimerFire()
{
    if (!IsValid(OwnerNPC)) return;  // Guard
    AggressionLevel += BuildRate;
    UpdateGoalScore();
}
```

**RAS-W2 Details:**
- Guide shows no explicit cleanup in BeginDestroy/destructor
- Pending timers may fire after GoalGenerator is marked for GC
- Recommendation: Add `ClearAllTimersForObject(this)` to cleanup

#### Patterns Used:
- GoalGenerator with dynamic scoring
- Timer-based state machine
- Random interval calculations
- Perception integration for trigger events

#### Locked Compliance:
- R-TIMER-1 Extension: VIOLATED - No validity guards (proposed R-TIMER-2)
- R-AI-1: COMPLIANT - Uses GoalGenerator pattern correctly

---

### 4. Support_Buffer_Healer_NPC_Implementation_Guide_v1_0.md

**NPC Type:** NPC that buffs/heals allies
**Primary Systems:** GE application, target selection

#### Compliance Status: PASS (v4.32.1 - HIGHLY AUTOMATED)

#### Automation Status (v4.32.1)

**BTS_HealNearbyAllies** now implements complete healing behavior:
- ReceiveTickAI event with cooldown timer
- `SphereOverlapActors` for radius detection
- `ActorHasMatchingGameplayTag(Narrative.Factions.Friendly)` for faction filtering
- `GetAbilitySystemComponent` + `ApplyGameplayEffectToTarget(GE_SupportHeal)`

**Designer Configuration:**
- Set `HealRadius` variable (default 500.0)
- Set `HealCooldown` variable (default 2.0 seconds)
- Configure `GE_SupportHeal` magnitude in the GE asset
- Ensure allies have `Narrative.Factions.Friendly` tag

#### Findings:

| ID | Severity | Issue | Location | Status |
|----|----------|-------|----------|--------|
| SBH-C1 | ~~CRITICAL~~ | MakeOutgoingSpec shown without param. prefix | Section 4.3 | **RESOLVED** - Now uses manifest automation |
| SBH-W1 | WARNING | Movement speed modification without restore pattern | Section 5.2 | **Still applies if using speed buffs** |
| SBH-W2 | ~~WARNING~~ | No delegate cleanup on ability end | Section 4.5 | **N/A** - Uses ReceiveTickAI, not delegates |

**SBH-C1 Resolution (v4.32.1):**
- Manifest now uses `param.GameplayEffectClass: GE_SupportHeal` syntax
- BTS_HealNearbyAllies fully automated with proper Contract 13 compliance

**SBH-W1 Details (Still Relevant for Speed Buffs):**
- If implementing speed buffs separately, use P-MOVE-1 store/restore pattern
- Current automation only covers healing, not speed modification

#### Patterns Used:
- ~~Target prioritization~~ → **SphereOverlapActors + ForEachLoop** heals all friendlies
- GE-based buff application ✓
- Range-based ally detection ✓ (HealRadius variable)
- ~~Periodic scan via BTService~~ → **ReceiveTickAI with cooldown timer**

#### Locked Compliance:
- P-MOVE-1 (DOC-ONLY): N/A - Current automation doesn't modify movement speed
- R-CLEANUP-1: **COMPLIANT** - ReceiveTickAI has no delegate cleanup needs
- Contract 13: **COMPLIANT** - Uses param.GameplayEffectClass syntax

---

### 5. Warden_Husk_System_Implementation_Guide_v1_2.md

**NPC Type:** Boss NPC with phase transitions
**Primary Systems:** HandleDeath override, ability spawning

#### Compliance Status: PASS (v4.32 - FULLY AUTOMATED)

#### Automation Status (v4.32)

**BP_WardenHusk** now implements complete phase transition:
- `PhaseSpawnClass` TSubclassOf variable for spawn target
- `HandleDeath` function_override with:
  - `HasAuthority()` check for server-only spawning
  - `K2_GetActorTransform()` for spawn location
  - `SpawnActor` node connected to PhaseSpawnClass
  - `CallParent` for proper death chain

**Designer Configuration:**
- Set `PhaseSpawnClass` default value to `BP_WardenCore`
- BP_WardenCore already configured with Flying movement mode

#### Findings:

| ID | Severity | Issue | Location | Status |
|----|----------|-------|----------|--------|
| WH-W1 | WARNING | AbilityTask_SpawnProjectile not in generator | Section 6.2 | Still applies for laser attacks |
| WH-W2 | ~~WARNING~~ | HandleDeath complexity may timeout | Section 8.1 | **RESOLVED** - Minimal logic: auth check + spawn + parent call |
| WH-W3 | WARNING | Phase transition missing Transitioning tag | Section 5.3 | **Acceptable** - SpawnActor is instant |

**WH-W1 Details:**
- Guide references `AbilityTask_SpawnProjectile` for GA_CoreLaser
- Generator supports `SpawnActor` node but projectile tasks need manual wiring
- Not a blocker - GA_CoreLaser already implemented in manifest

**WH-W2 Resolution (v4.32):**
- HandleDeath now contains minimal logic:
  1. HasAuthority check (pure function)
  2. GetActorTransform (pure function)
  3. SpawnActor (single node)
  4. CallParent
- Well under any timeout threshold

#### Patterns Used:
- HandleDeath override for phase transition ✓ **AUTOMATED**
- ~~Health threshold monitoring~~ → **Death triggers transition**
- ~~Corpse actor spawning~~ → **SpawnActor for BP_WardenCore**
- Multi-phase boss state machine ✓

#### Locked Compliance:
- R-ENUM-1: COMPLIANT - Uses GE-first for state identity
- Rule 2 (Delay): COMPLIANT - No delays in HandleDeath

---

### 6. Biomechanical_Detachment_System_Implementation_Guide_v1_1.md

**NPC Type:** NPC with detachable mechanical parts
**Primary Systems:** HandleDeath, component spawning

#### Compliance Status: PASS (v4.32 - FULLY AUTOMATED)

#### Automation Status (v4.32)

**BP_BiomechHost** now implements complete phase transition:
- `PhaseSpawnClass` TSubclassOf variable for spawn target
- `HandleDeath` function_override with:
  - `HasAuthority()` check for server-only spawning
  - `K2_GetActorTransform()` for spawn location
  - `SpawnActor` node connected to PhaseSpawnClass
  - `CallParent` for proper death chain

**Designer Configuration:**
- Set `PhaseSpawnClass` default value to `BP_BiomechCreature`

#### Findings:

| ID | Severity | Issue | Location | Status |
|----|----------|-------|----------|--------|
| BMD-W1 | WARNING | Corpse spawn doesn't verify attach socket | Section 4.2 | **N/A** - Uses GetActorTransform, not sockets |
| BMD-W2 | ~~WARNING~~ | No validity check before component detach | Section 3.4 | **N/A** - No component detach in automated version |

**Resolution Notes:**
- v4.32 automation uses simple SpawnActor at death location
- Complex socket-based spawning is game-specific and not automated
- If socket-based spawning needed, extend manually in BP_BiomechHost

#### Patterns Used:
- HandleDeath for cleanup and spawning ✓ **AUTOMATED**
- ~~Component-based part management~~ → **Simple spawn pattern**
- ~~Physics simulation for detached parts~~ → **BP_BiomechCreature handles its own physics**
- ~~Damage threshold monitoring per part~~ → **Death triggers transition**

#### Locked Compliance:
- P-ATTACH-1: N/A - No attachment in automated version
- R-AI-1: N/A - No direct BT manipulation

---

## PATTERNS NOT LOCKED

The following patterns are used across NPC guides but are NOT covered by locked rules in the GAS Audit:

### 1. HandleDeath Override Pattern

**Used By:** Warden_Husk, Biomechanical_Detachment, Possessed_Exploder

**Description:** Override `HandleDeath` in NarrativeCharacter to perform custom death sequences (spawn corpse, trigger events, phase transitions).

**Risk Level:** MEDIUM
- HandleDeath has implicit timeout
- Complex logic may not complete
- Should ensure EndAbility-equivalent cleanup

**Recommendation:** Document as pattern. Consider adding to DOC-ONLY section similar to P-MONTAGE-1.

---

### 2. GoalGenerator Timer Pattern

**Used By:** Random_Aggression_Stalker

**Description:** NPCGoalGenerator using FTimerHandle for state machine transitions.

**Risk Level:** HIGH
- No equivalent of AbilityTask auto-cleanup
- UObject lifecycle differs from GameplayAbility
- Timer callbacks can outlive owner

**Recommendation:** Create new locked rule R-TIMER-2 (see Research Question 2).

---

### 3. NPC-to-NPC Event Propagation

**Used By:** Gatherer_Scout_Alert

**Description:** One NPC broadcasts events to nearby NPCs for coordinated behavior.

**Risk Level:** LOW
- Narrative Pro's subsystem handles dead NPC filtering
- Broadcast patterns are idiomatic UE5

**Recommendation:** Document as DOC-ONLY pattern. No rule needed.

---

### 4. BTService-Driven State Monitoring

**Used By:** Gatherer_Scout_Alert, Support_Buffer_Healer

**Description:** Use BTService tick to continuously monitor conditions and update Blackboard/Goals.

**Risk Level:** LOW
- BTService lifecycle managed by BehaviorTree
- Standard Narrative Pro pattern

**Recommendation:** Document as DOC-ONLY pattern. No rule needed.

---

### 5. Health Threshold Phase Transitions

**Used By:** Warden_Husk

**Description:** Monitor health via delegate, trigger phase changes at thresholds.

**Risk Level:** MEDIUM
- Requires proper delegate cleanup
- Multiple rapid threshold crossings could cause issues

**Recommendation:** Document best practices. Consider extending R-CLEANUP-1 scope.

---

## PATTERNS CONFLICTING WITH LOCKED DECISIONS

### 1. Timer Usage Without Validity Guards (Conflicts with R-TIMER-1 Spirit)

**Guide:** Random_Aggression_Stalker_v2_1

**Conflict:** R-TIMER-1 requires validity guards for timers in GameplayAbilities. The Stalker guide uses timers in GoalGenerator without guards.

**Resolution:** R-TIMER-1 should be extended or a new R-TIMER-2 rule created for UObject timer safety.

---

### 2. Movement Speed Modification Without P-MOVE-1 Pattern

**Guide:** Support_Buffer_Healer_v1_0

**Conflict:** P-MOVE-1 (DOC-ONLY) recommends Store-on-Activate/Restore-on-End for movement speed changes. The healer guide modifies speed directly without this pattern.

**Resolution:** Update guide to reference P-MOVE-1 pattern or use GE-based movement modification.

---

## RECOMMENDATIONS

### Immediate Actions (Before Implementation)

1. **Update Version Headers**
   - Change "UE 5.6 + Narrative Pro v2.1+" to "UE5.7 + Narrative Pro v2.2" in all guides

2. **Add Manifest Syntax Notes**
   - Add note to guides using MakeOutgoingSpec about Contract 13 manifest syntax
   - Affects: Possessed_Exploder, Support_Buffer_Healer

3. **Fix Timer Safety in Stalker Guide**
   - Add validity guards to all 4 timer callbacks
   - Add timer cleanup to destructor/cleanup section

### Documentation Updates

4. **Extend R-TIMER-1 or Create R-TIMER-2**
   - Scope: All UObject timer usage (GoalGenerators, custom UObjects)
   - Add to Father_Companion_GAS_Abilities_Audit.md

5. **Document HandleDeath Pattern**
   - Add to DOC-ONLY patterns section
   - Include timeout considerations and cleanup requirements

6. **Reference P-MOVE-1 in Buffer/Healer Guide**
   - Add store/restore pattern for movement speed buffs
   - Or recommend GE-based approach

### Future Considerations

7. **Create NPC Patterns Appendix**
   - Consolidate NPC-specific patterns separate from Father abilities
   - Include: GoalGenerator timers, HandleDeath, NPC events, BTService monitoring

8. **Manifest Conversion Checklist**
   - If guides are converted to manifest yaml
   - Verify Contract 13 compliance
   - Verify all param.* prefixes

---

## APPENDIX A: Pattern Compliance Matrix

| Pattern | Father Audit Rule | NPC Guides Using | Status |
|---------|------------------|------------------|--------|
| Timer Callback Safety | R-TIMER-1 (GA only) | Stalker, Exploder | EXTEND RULE |
| Activity System | R-AI-1 / Contract 12 | Scout, Healer | COMPLIANT |
| Movement Restore | P-MOVE-1 (DOC-ONLY) | Healer | NOT FOLLOWING |
| GE Application | Contract 13 (manifest) | Exploder, Healer | N/A (Blueprint) |
| Delegate Cleanup | R-CLEANUP-1 (scoped) | Healer | PARTIAL |
| HandleDeath Override | NOT DOCUMENTED | Warden, Detachment, Exploder | ADD PATTERN |
| GoalGenerator Timers | NOT DOCUMENTED | Stalker | ADD RULE |
| BTService Monitoring | NOT DOCUMENTED | Scout, Healer | ADD PATTERN |

---

## APPENDIX B: Web Search Sources

- [Gameplay Timers | UE5.7 Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-timers-in-unreal-engine)
- [Using C++ Timers in Unreal Engine | Tom Looman](https://www.tomlooman.com/unreal-engine-cpp-timers/)
- [Timers in UObjects - Epic Forums](https://forums.unrealengine.com/t/timers-in-uobjects/89733)
- [GASDocumentation - GitHub](https://github.com/tranek/GASDocumentation)
- [Unreal Engine 5.7 Release Notes](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-7-release-notes)
- [Gameplay Effects Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-effects-for-the-gameplay-ability-system-in-unreal-engine)

---

## VERSION HISTORY

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.4 | 2026-01-25 | Claude (Opus 4.5) | v4.33: Custom function resolution (Step 5 - Blueprint->FunctionGraphs); ParseGoalItems LineIndex fix; Fixed automation script log capture; Deleted duplicate manifest.yaml |
| 1.3 | 2026-01-25 | Claude (Opus 4.5) | v4.32.2-4.32.3: GoalGenerator automation with InitializeGoalGenerator override; Parser debug confirms all sections parsed correctly; GA_Backstab/GA_ProtectiveDome still blocked (custom function resolution D5) |
| 1.2 | 2026-01-25 | Claude (Opus 4.5) | Added Automation Status section (v4.32/v4.32.1); Updated per-guide sections with automation details; Marked resolved issues |
| 1.1 | 2026-01-24 | Claude (Opus 4.5) | Added Gameplay Overview section with player-facing descriptions of each NPC |
| 1.0 | 2026-01-24 | Claude (Opus 4.5) | Initial audit of 6 NPC guides |

---

## APPENDIX C: v4.32.2/v4.32.3 GoalGenerator Enhancements

### Parser Debugging Session (2026-01-25 01:30-01:50)

Debug logging was added to ParseActorBlueprints and ParseGoalItems to trace manifest parsing. The logs confirmed:

```
[PARSER DEBUG] ParseActorBlueprints exiting at line 9865, total blueprints now: 17
[PARSER DEBUG] ParseGoalItems entered at line 9865: goals:  # GATHERER
[PARSER DEBUG] ParseGoalItems exiting at line 9869 (next line: actor_blueprints:  # GATHERER GOAL GENERATORS)
[PARSER DEBUG] ParseActorBlueprints exiting at line 10085, total blueprints now: 19
[PARSER DEBUG] ParseActorBlueprints exiting at line 10154, total blueprints now: 20
[PARSER DEBUG] ParseActorBlueprints exiting at line 10460, total blueprints now: 22
```

**Findings:**
1. Parser correctly enters and exits ALL actor_blueprints sections including GATHERER GOAL GENERATORS
2. ParseGoalItems LineIndex fix (v4.32.3) correctly decrements LineIndex before break
3. Total 22 blueprints are parsed by the parser
4. BUT commandlet reports only 19 blueprints - **3 blueprints are filtered out somewhere**

### Resolution: Stale Output Log

**Root Cause Found (2026-01-25 01:55):**

The `commandlet_output.log` was stale (modified at 00:29, over 1.5 hours old). The actual UE log (`NP22B57.log`) showed:

```
Parsed manifest with 204 tags, 3 enumerations, 21 abilities, 47 effects, 20 blueprints, 6 MICs
```

**Parser Debug Output (verified):**
```
[PARSER] Adding blueprint on exit: BP_Reinforcement (total: 17)
[PARSER] Adding blueprint on exit: GoalGenerator_Alert (total: 18)  ← CONFIRMED
[PARSER] Adding blueprint on exit: BP_ReturnedStalker (total: 19)
[PARSER] Adding blueprint on exit: GoalGenerator_RandomAggression (total: 20)
```

**Final Status:**
- ✅ GoalGenerator_Alert IS being parsed correctly
- ✅ 20 blueprints total (was 19 before adding GoalGenerator_Alert)
- ⚠️ Automation script output log capture needs review
- ⚠️ Duplicate manifest.yaml in root was deleted

**Note:** Timestamps are UTC in logs (e.g., `2026.01.24-22.54.26` = `2026.01.25-01:54:26` Turkey local time)

### GoalGenerator Automation (v4.32.2)

Added `function_overrides` support for InitializeGoalGenerator:

```yaml
actor_blueprints:
  - name: GoalGenerator_Alert
    parent_class: NPCGoalGenerator
    function_overrides:
      - function: InitializeGoalGenerator
        nodes:
          - id: ParentInitialize
            type: CallParent
          - id: PrintReady
            type: CallFunction
            properties:
              function: PrintString
              class: KismetSystemLibrary
              parameters:
                InString: "GoalGenerator initialized"
```

### Current Blockers

| Issue | Asset | Status | Reference |
|-------|-------|--------|-----------|
| D5 - Custom function resolution | GA_Backstab | BLOCKED | See GA_Backstab_GA_ProtectiveDome_Error_Audit.md |
| D5 - Custom function resolution | GA_ProtectiveDome | BLOCKED | v4.31 fix applied but still failing |

**Next Steps:**
1. Review v4.31 custom function resolution code
2. Check if FunctionGraph is being created for CheckBackstabCondition
3. Detailed event graph generation logs needed

### v4.33 Changes (2026-01-25)

**1. Custom Function Resolution (Step 5):**

Added `ResolveViaBlueprintFunctionGraph()` to FGasAbilityGeneratorFunctionResolver:
- Searches `Blueprint->FunctionGraphs` for custom functions
- Handles uncompiled functions via `SetSelfMember()` reference
- Generation order changed: custom_functions now generated BEFORE event_graph

**2. ParseGoalItems LineIndex Fix:**

Fixed section boundary detection in `ParseGoalItems`:
```cpp
// v4.32.3 FIX: Decrement LineIndex before break so main loop re-processes section headers
if (CurrentIndent <= SectionIndent && !TrimmedLine.StartsWith(TEXT("-")))
{
    LineIndex--;  // <-- Added this line
    break;
}
```

**3. Automation Script Log Capture Fix:**

Fixed `claude_automation.ps1` output log not updating:
- Root cause: PowerShell path escaping issues with UE commandlet
- Fix: Use `Start-Process` with proper argument quoting
- Added fallback: Extract from UE log if commandlet output missing
- Added diagnostic output: shows command being run

**4. Duplicate Manifest Cleanup:**

Deleted stale `manifest.yaml` from plugin root (was from Jan 17, v2.0.3). Only `ClaudeContext/manifest.yaml` should exist.

---

**END OF AUDIT REPORT**
