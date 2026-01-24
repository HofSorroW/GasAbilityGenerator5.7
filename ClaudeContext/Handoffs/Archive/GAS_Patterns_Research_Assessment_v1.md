# GAS Patterns Research Assessment v1.0
## January 2026

**Purpose:** Deep research and validation of unresearched patterns identified in GAS Audit v6.0 against UE5.7 and Narrative Pro documentation.

**Auditor:** Claude (web research + source validation)
**Context:** Father Companion GAS implementation

---

## TABLE OF CONTENTS

1. [Montage Task Lifecycle](#1-montage-task-lifecycle)
2. [Socket Attachment/Detachment Replication](#2-socket-attachmentdetachment-replication)
3. [Line Trace Deployment Authority](#3-line-trace-deployment-authority)
4. [Movement Component Restore Contract](#4-movement-component-restore-contract)
5. [Effect Handle Storage Pattern](#5-effect-handle-storage-pattern)
6. [MEDIUM Severity Items Assessment](#6-medium-severity-items-assessment)
7. [Recommendations Summary](#7-recommendations-summary)

---

## 1. MONTAGE TASK LIFECYCLE

### Research Sources
- [Epic Games PlayMontageAndWait Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/Ability/Tasks/PlayMontageAndWait)
- [GASDocumentation (tranek)](https://github.com/tranek/GASDocumentation)
- [Epic Forums: PlayMontageAndWait Blend Out Issue](https://forums.unrealengine.com/t/playmontageandwait-blend-out-issue/248499)
- [Quod Soler: 10 Useful GAS Ability Tasks](https://www.quodsoler.com/blog/from-wait-delays-to-play-montage-10-useful-gas-ability-tasks)

### Callback Semantics (VALIDATED)

| Callback | When Fired | Use Case |
|----------|-----------|----------|
| **OnBlendOut** | Montage starts blending out (BEFORE fully complete) | Early cleanup prep, NOT EndAbility |
| **OnCompleted** | Montage fully finished playing | Normal EndAbility path |
| **OnInterrupted** | Another montage overwrites current one | Cancel cleanup, EndAbility |
| **OnCancelled** | Ability or task explicitly cancelled | Cancel cleanup, EndAbility |

### Key Finding: Callback Auto-Unbinding

From Epic documentation:
> "This task unbinds all animation delegates on this Ability Task (except OnCanceled)"

**Implication:** When EndAbility is called:
1. Task is destroyed via `TaskOwnerEnded()`
2. Delegates auto-unbind (except OnCancelled which fires first)
3. `ShouldBroadcastAbilityTaskDelegates()` checks `Ability->IsActive()` before firing

### Correct Pattern

```yaml
# All three termination callbacks should lead to EndAbility
connections:
  - from: [PlayMontage, OnCompleted]
    to: [EndAbility, Exec]
  - from: [PlayMontage, OnCancelled]
    to: [EndAbility, Exec]
  - from: [PlayMontage, OnInterrupted]
    to: [EndAbility, Exec]
  # OnBlendOut should NOT call EndAbility (montage still running)
```

### Assessment

| Criteria | Status |
|----------|--------|
| Pattern validated by Epic docs | YES |
| Already implemented in Father system | PARTIAL (GA_FatherAttack uses montage) |
| Risk if not formalized | MEDIUM (premature EndAbility if OnBlendOut misused) |
| Recommendation | **DOCUMENT ONLY** |

**Rationale:** Standard GAS knowledge. Not Father-specific. Document in Technical Reference but no LOCKED rule needed.

---

## 2. SOCKET ATTACHMENT/DETACHMENT REPLICATION

### Research Sources
- [Epic Games Remote Procedure Calls Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/remote-procedure-calls-in-unreal-engine)
- [Epic Forums: AttachToComponent and Replication](https://forums.unrealengine.com/t/attachtocomponent-and-replication/386002)
- [Cedric Neukirchen: Remote Procedure Calls](https://cedric-neukirchen.net/docs/multiplayer-compendium/remote-procedure-calls/)
- [Unreal Community Wiki: Replication](https://unrealcommunity.wiki/replication-vyrv8r37)

### Replication Model (VALIDATED)

**Server-Authoritative Architecture:**
1. Only actors with `ROLE_Authority` can execute NetMulticast RPCs
2. Clients cannot replicate unless server initiates
3. `AttachToComponent` on replicated actors must be called on server

### RPC Requirements

From Epic documentation:
> "There are requirements for RPCs to be completely functional: They must be called from Actors. The Actor must be replicated. If the RPC is being called from server to be executed on a client, only the client who actually owns that Actor will execute the function."

### Father Attachment Pattern (Current)

| Action | Authority | Method |
|--------|-----------|--------|
| Attach Father to Player | Server | `AttachActorToComponent` → replicates automatically |
| Detach Father from Player | Server | `DetachFromActor` → replicates automatically |
| Visual effects on attachment | All clients | NetMulticast RPC (server-initiated) |

### Assessment

| Criteria | Status |
|----------|--------|
| Pattern validated by Epic docs | YES |
| Already implemented in Father system | YES (server-only abilities) |
| Risk if not formalized | LOW (already using ServerOnly net execution) |
| Recommendation | **NOT NEEDED** |

**Rationale:** Father abilities already use `net_execution_policy: ServerOnly`. Standard UE replication handles attachment. No Father-specific rule needed.

---

## 3. LINE TRACE DEPLOYMENT AUTHORITY

### Research Sources
- [GASDocumentation: Networking](https://ikrima.dev/ue4guide/gameplay-programming/gameplay-ability-system/gas-networking/)
- [Epic Games: FPredictionKey Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/GameplayAbilities/FPredictionKey)
- [The Games Dev: Target Data in Gameplay Abilities](https://www.thegames.dev/?p=242)
- [Epic Games: Understanding GAS](https://dev.epicgames.com/documentation/en-us/unreal-engine/understanding-the-unreal-engine-gameplay-ability-system)

### TargetData Pattern (VALIDATED)

From GASDocumentation:
> "An ability with net execution policy LocalPredicted will run on both the client and the server. If it is necessary for the server to have TargetData... the client must produce the data and send it to the server via RPC."

### Prediction Key System

From Epic:
> "A prediction key on its own is simply a unique ID that is generated in a central place on the client. The client will send his prediction key to the server, and associate predictive actions and side effects with this key."

**Key Limitation:**
> "FPredictionKey always replicates client -> server, but when replicating server -> clients they *only replicate to the client that sent the prediction key."

### Father Engineer Deployment (Current Pattern)

GA_FatherEngineer uses `ServerOnly` execution:
- No client prediction needed
- Server performs line trace
- Server spawns at trace location
- Result replicates to all clients

### Assessment

| Criteria | Status |
|----------|--------|
| Pattern validated by Epic docs | YES |
| Already implemented in Father system | YES (ServerOnly = server does trace) |
| Risk if not formalized | LOW |
| Recommendation | **NOT NEEDED** |

**Rationale:** ServerOnly execution means server owns all targeting decisions. No client TargetData replication needed. Standard pattern.

---

## 4. MOVEMENT COMPONENT RESTORE CONTRACT

### Research Sources
- [GASDocumentation: Gameplay Effects](https://github.com/tranek/GASDocumentation)
- [Epic Games: Gameplay Effects Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-effects-for-the-gameplay-ability-system-in-unreal-engine)
- [Epic Games: Remove Active Gameplay Effect](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/GameplayEffects/RemoveActiveGameplayEffect)
- [Devtricks: The Truth of GAS](https://vorixo.github.io/devtricks/gas/)

### The Problem

GAS has no `MovementSpeed` attribute by default. Character movement modifications require:
- Direct `CharacterMovementComponent.MaxWalkSpeed` manipulation
- Store original → Apply modified → Restore original on end

### Restoration Approaches (VALIDATED)

| Approach | Mechanism | Auto-Cleanup |
|----------|-----------|--------------|
| **GE Duration Policy** | Apply GE with HasDuration, modifier auto-removes | YES |
| **Store/Restore** | Variable stores original, EndAbility restores | Manual |
| **Ability Activation Tags** | Tag-based multiplier in CMC | YES (tag auto-removed) |

### Current Father Pattern

From GAS Audit SPEED/JUMP RESTORATION PATTERN section:
```
1. On activate: Store OriginalMaxWalkSpeed = CharacterMovement.MaxWalkSpeed
2. Apply boost: MaxWalkSpeed = Original * Multiplier
3. On end: Restore MaxWalkSpeed = OriginalMaxWalkSpeed
```

### Known Edge Case (Already Documented)

From audit:
> "If player has other speed modifier active (sprint, slow debuff) when Symbiote activates, 'original' captures the MODIFIED value."

**Assessment:** Low priority - works for normal gameplay.

### Assessment

| Criteria | Status |
|----------|--------|
| Pattern validated by Epic docs | YES |
| Already implemented in Father system | YES (Store/Restore pattern) |
| Risk if not formalized | LOW (edge case documented) |
| Recommendation | **NOT NEEDED** |

**Rationale:** Already documented in GAS Audit v6.0. Edge case acknowledged. No additional rule needed.

---

## 5. EFFECT HANDLE STORAGE PATTERN

### Research Sources
- [Epic Games: Remove Active Gameplay Effect](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/GameplayEffects/RemoveActiveGameplayEffect)
- [GASDocumentation: Gameplay Effects](https://github.com/tranek/GASDocumentation)
- [Quod Soler: Gameplay Effect Components](https://www.quodsoler.com/blog/how-to-use-gameplay-effect-components-in-unreal-engine-5)

### Handle Storage Pattern (VALIDATED)

From Epic documentation:
> "Remove Active Gameplay Effect - removes the specified GameplayEffect by Handle."

**Pattern:**
```cpp
// On Apply
FActiveGameplayEffectHandle EffectHandle = ApplyGameplayEffectToSelf(...);

// On End/Cleanup
RemoveActiveGameplayEffect(EffectHandle);
```

### Prediction Limitation

From GASDocumentation:
> "Cannot predict the removal of GameplayEffects. We can however predict adding GameplayEffects with the inverse effects, effectively removing them. This is not always appropriate or feasible."

**Implication:** For predicted abilities, inverse effects may be needed. For ServerOnly abilities (Father), direct removal works.

### Father Implementation

Current pattern in form abilities:
1. Apply GE_*State (stored implicitly by tag)
2. Remove via `BP_RemoveGameplayEffectFromOwnerWithGrantedTags(Effect.Father.FormState)`

This uses **tag-based removal** rather than handle storage:
- More robust (finds all matching effects)
- Works across ability instances
- No handle variable needed

### Assessment

| Criteria | Status |
|----------|--------|
| Pattern validated by Epic docs | YES |
| Already implemented in Father system | YES (tag-based removal) |
| Risk if not formalized | LOW |
| Recommendation | **DOCUMENT ONLY** |

**Rationale:** Tag-based removal is superior for Father's use case. Document handle storage as alternative pattern for single-effect scenarios.

---

## 6. MEDIUM SEVERITY ITEMS ASSESSMENT

### 6.1 GA_StealthField Guards - VERIFIED CORRECT

**Audit Claim:**
> "Has Delay (8s), no guards - Miscategorized as toggle"

**Manifest Verification (lines 5034-5053):**
```yaml
# v4.15: WaitDelay auto-terminates with ability (Track B GAS Audit)
- id: Delay
  type: AbilityTaskWaitDelay    # ← AUTO-PROTECTED
  position: [2700, 0]
# === POST-DELAY GUARDS (GAS Audit MED-4) ===
- id: IsValid_PlayerRef_Guard
  type: CallFunction
  properties:
    function: IsValid
- id: Branch_PlayerValid_Guard
  type: Branch                   # ← GUARD PRESENT
```

**Findings:**
1. Uses `AbilityTaskWaitDelay` (auto-terminates with ability per VTF-1)
2. Has post-delay guard: `Branch_PlayerValid_Guard` checking `IsValid(PlayerRef)`
3. Has Event_EndAbility handler with effect cleanup

**Status:** ✅ CORRECTLY IMPLEMENTED - No action needed

### 6.2 GA_FatherRifle Cleanup - VERIFIED CORRECT

**Audit Claim:**
> "No guards in Event_EndAbility - Persistent ability"

**Manifest Verification (lines 6369-6401):**
```yaml
# v4.14: Route through guard (GAS Audit MED-5)
- from: [Event_EndAbility, Then]
  to: [Branch_Valid_End, Exec]
- from: [Branch_Valid_End, True]
  to: [ShowFather, Exec]
# Guard data flow (GAS Audit MED-5)
- from: [GetFatherRef_End, FatherRef]
  to: [IsValid_FatherRef_End, Object]
- from: [IsValid_FatherRef_End, ReturnValue]
  to: [Branch_Valid_End, Condition]
```

**Findings:**
1. Has Event_EndAbility handler
2. Has guard: `Branch_Valid_End` checking `IsValid(FatherRef)`
3. Cleanup: ShowFather, EnableCollision, RemoveWeapon

**Status:** ✅ CORRECTLY IMPLEMENTED - No action needed

### 6.3 GA_FatherSword Cleanup - VERIFIED CORRECT

**Audit Claim:**
> "No guards in Event_EndAbility - Persistent ability"

**Manifest Verification (lines 6647-6679):**
```yaml
# v4.14: Route through guard (GAS Audit MED-6)
- from: [Event_EndAbility, Then]
  to: [Branch_Valid_End, Exec]
- from: [Branch_Valid_End, True]
  to: [ShowFather, Exec]
```

**Findings:**
1. Has Event_EndAbility handler (identical pattern to GA_FatherRifle)
2. Has guard: `Branch_Valid_End` checking `IsValid(FatherRef)`
3. Cleanup: ShowFather, EnableCollision, RemoveWeapon

**Status:** ✅ CORRECTLY IMPLEMENTED - No action needed

### MEDIUM Severity Summary

**All MEDIUM severity items verified as correctly implemented:**

| Ability | Claimed Issue | Actual Status |
|---------|--------------|---------------|
| GA_StealthField | "No guards" | Has AbilityTaskWaitDelay + IsValid guard |
| GA_FatherRifle | "No guards in EndAbility" | Has IsValid guard + full cleanup flow |
| GA_FatherSword | "No guards in EndAbility" | Has IsValid guard + full cleanup flow |

**Note:** The audit document's severity matrix may be stale. All MEDIUM items were fixed in v4.14/v4.15 as indicated by the comments.

---

## 7. RECOMMENDATIONS SUMMARY

### New Rules to LOCK

| Pattern | Recommendation | Severity | Rationale |
|---------|---------------|----------|-----------|
| Montage Task Lifecycle | DOCUMENT ONLY | N/A | Standard GAS knowledge, not Father-specific |
| Socket Attachment Replication | NOT NEEDED | N/A | Already covered by ServerOnly execution |
| Line Trace Authority | NOT NEEDED | N/A | Already covered by ServerOnly execution |
| Movement Restore | NOT NEEDED | N/A | Already documented in audit |
| Effect Handle Storage | DOCUMENT ONLY | N/A | Tag-based removal used instead |

### Action Items for MEDIUM Severity

| Ability | Original Claim | Verification Result | Action Required |
|---------|----------------|---------------------|-----------------|
| GA_StealthField | "No guards" | ✅ Has AbilityTaskWaitDelay + guards | NONE |
| GA_FatherRifle | "No guards in EndAbility" | ✅ Has IsValid guard + cleanup | NONE |
| GA_FatherSword | "No guards in EndAbility" | ✅ Has IsValid guard + cleanup | NONE |

**All MEDIUM severity items are already correctly implemented (v4.14/v4.15).**

### Documentation Updates

1. Add Montage callback semantics to Technical Reference (optional)
2. Update GAS Audit severity matrix to reflect MEDIUM items resolved
3. Add Effect Handle storage as alternative pattern reference (optional)

---

## PATTERN VALIDATION SUMMARY

| Pattern | UE5 Validated | Narrative Pro Validated | Father Implemented | Needs Rule |
|---------|--------------|------------------------|-------------------|------------|
| Montage Lifecycle | YES | N/A | PARTIAL | NO (doc only) |
| Socket Attachment | YES | N/A | YES | NO |
| Line Trace Authority | YES | N/A | YES | NO |
| Movement Restore | YES | N/A | YES | NO |
| Effect Handle Storage | YES | N/A | YES (via tags) | NO (doc only) |

---

## CONCLUSION

**No new LOCKED rules required from this research.**

All researched patterns are either:
1. Already implemented correctly in Father system
2. Covered by existing rules (ServerOnly execution, R-CLEANUP-1)
3. Standard GAS knowledge that should be documented but not locked

**MEDIUM Severity Resolution:**
All 3 MEDIUM severity items (GA_StealthField, GA_FatherRifle, GA_FatherSword) were verified and found to be **correctly implemented** with proper guards and cleanup. The audit severity matrix should be updated to reflect these items as RESOLVED.

**Final Status:**
- 6 patterns researched: 0 need new LOCKED rules
- 3 MEDIUM severity items verified: All correctly implemented
- Audit document status: May need severity matrix update to RESOLVED

---

## SOURCES

- [Epic Games: PlayMontageAndWait](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/Ability/Tasks/PlayMontageAndWait)
- [Epic Games: Remote Procedure Calls](https://dev.epicgames.com/documentation/en-us/unreal-engine/remote-procedure-calls-in-unreal-engine)
- [Epic Games: FPredictionKey](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/GameplayAbilities/FPredictionKey)
- [Epic Games: Gameplay Effects](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-effects-for-the-gameplay-ability-system-in-unreal-engine)
- [Epic Games: Remove Active Gameplay Effect](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/GameplayEffects/RemoveActiveGameplayEffect)
- [GASDocumentation (tranek)](https://github.com/tranek/GASDocumentation)
- [Devtricks: The Truth of GAS](https://vorixo.github.io/devtricks/gas/)
- [Quod Soler: GAS Ability Tasks](https://www.quodsoler.com/blog/from-wait-delays-to-play-montage-10-useful-gas-ability-tasks)
- [Epic Forums: AttachToComponent and Replication](https://forums.unrealengine.com/t/attachtocomponent-and-replication/386002)
- [Cedric Neukirchen: Multiplayer Compendium](https://cedric-neukirchen.net/docs/multiplayer-compendium/remote-procedure-calls/)

---

**Document Version:** 1.0
**Date:** January 2026
**Author:** Claude (research + validation)
