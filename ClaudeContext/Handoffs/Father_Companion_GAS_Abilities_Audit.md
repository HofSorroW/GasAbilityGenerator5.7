# Father Companion GAS & Abilities Audit - Locked Decisions
## Version 5.0 - January 2026

**Purpose:** This document consolidates all validated findings and locked decisions from dual-agent audits (Claude-GPT) conducted January 2026. These decisions are LOCKED and should not be debated again.

**Audit Context:** UE5.7 + Narrative Pro v2.2 + GasAbilityGenerator v4.29

**v5.0 Updates:** Added NOT LOCKED items audit (2026-01-24). Locked NL-GUARD-IDENTITY (L1). Validated bActivateAbilityOnGranted, AbilityTasks, EquippableItem patterns. Updated guard pattern documentation.

**v4.0 Updates:** ALL MANIFEST CHANGES COMPLETE. All 4 critical defects resolved. All First Activation paths fixed. All orphan effects removed.

**v3.0 Updates:** Merged Abilities_Audit_v1.md into this document. Added Rule 4 (First Activation), VTF-7 (CommitCooldown), VTF-8 (SetByCaller). Added Design Decisions section (1A-4). Added orphan GE removals.

---

## TABLE OF CONTENTS

1. [Locked Constraints (LC-1 to LC-4)](#locked-constraints-implementation-boundaries)
2. [Invulnerability Decision (INV-1)](#invulnerability-decision)
3. [EndAbility Lifecycle Rules (Rule 1-4)](#endability-lifecycle-rules-canonical)
4. [Validated Technical Findings (VTF-1 to VTF-8)](#validated-technical-findings)
5. [NOT LOCKED Items Audit (v5.0)](#not-locked-items-audit-v50)
6. [Severity Matrix](#severity-matrix-locked)
7. [Critical Defect Details](#critical-defect-details)
8. [Design Decisions (Abilities Audit)](#design-decisions-abilities-audit-v10)
9. [Implementation Strategy](#implementation-strategy-locked)
10. [Gold Standard Reference](#ga_fathersymbiote--gold-standard-reference)

---

## LOCKED CONSTRAINTS (Implementation Boundaries)

### LC-1: No Manual Blueprint Edits
**Status:** LOCKED
**Source:** User requirement (audit session)

After GasAbilityGenerator generates assets, NO manual Blueprint edits are allowed. All fixes must go through:
1. Manifest.yaml updates
2. Generator enhancement (if new node types needed)
3. Re-generation via commandlet

**Rationale:** Ensures reproducibility, enables CI/CD, prevents configuration drift.

---

### LC-2: No UE Source Modification
**Status:** LOCKED
**Source:** User requirement (audit session)

Cannot modify:
- Unreal Engine source code
- Narrative Pro plugin C++ code
- Any engine-level headers

**Rationale:** Project uses stock engine and plugin binaries.

---

### LC-3: No C++ GameplayAbility Implementation
**Status:** LOCKED
**Source:** User requirement (audit session)

All GameplayAbility logic must be Blueprint-only, generated via manifest.yaml. No custom C++ ability classes.

**Allowed:**
- Enhance GasAbilityGenerator C++ (node generators)
- Use existing Blueprint nodes from UE/Narrative Pro

---

### LC-4: Process Lock (Research -> Audit -> Decide -> Implement)
**Status:** LOCKED
**Source:** Audit methodology

All changes must follow:
1. **Research** - Investigate UE5.7 source, validate claims
2. **Audit** - Cross-reference against docs, challenge assumptions
3. **Decide** - Make architectural decision with sources
4. **Implement** - Update manifest/generator, regenerate

No jumping directly to implementation without research validation.

---

## INVULNERABILITY DECISION

### INV-1: Remove ALL Invulnerability Except GA_FatherSacrifice
**Status:** LOCKED
**Source:** User decision (audit session)
**Date:** January 2026

**Remove:**
- GE_TransitionInvulnerability (5s during form transitions)
- GE_DashInvulnerability (dash i-frames)
- GE_Invulnerable
- Narrative.State.Invulnerable from all GE_*State effects
- State.Invulnerable tag references in Father system

**KEEP:**
- GA_FatherSacrifice: 8-second PLAYER invulnerability (not Father)
- This is the only intentional invulnerability in the design

**Rationale:** Invulnerability was NEVER intended in the Father design - it was an oversight discovered during audit. Form transitions should be vulnerable.

**Affected Files:**
- manifest.yaml - Remove invulnerability GE applications
- GE_ArmorState, GE_ExoskeletonState, GE_SymbioteState - Remove Narrative.State.Invulnerable from GrantedTags
- All form ability guides - Remove invulnerability documentation

---

## ENDABILITY LIFECYCLE RULES (CANONICAL)

### Rule 1 — Instant Abilities
**Status:** LOCKED
**Examples:** GA_FatherAttack, GA_DomeBurst, GA_ProximityStrike, GA_TurretShoot, GA_Backstab, GA_FatherElectricTrap, GA_FatherLaserShot

**Requirements:**
- Call `K2_EndAbility` at end of activation flow
- `Event_EndAbility` NOT required
- No delays, timers, or persistent state

**Pattern:**
```
Event_Activate → Logic → CommitCooldown → K2_EndAbility
```

---

### Rule 2 — Abilities with Delay or Timer
**Status:** LOCKED
**Examples:** All form abilities (GA_FatherCrawler, GA_FatherArmor, GA_FatherExoskeleton, GA_FatherSymbiote, GA_FatherEngineer), GA_FatherExoskeletonDash, GA_StealthField

**Requirements:**
- MUST have `Event_EndAbility` with `bWasCancelled` check
- MUST clean up persistent state (timers, GE handles)
- MUST prevent post-delay execution after cancellation

**How to prevent post-delay execution:**
- **Preferred (Track B):** WaitDelay AbilityTask (auto-cancels)
- **Temporary (Track A):** Explicit guards after Delay node

**Guard Pattern (3-layer) - Updated v5.0:**
```
After Delay:
  → IsValid(ActorRef) → Branch
    True → HasMatchingGameplayTag(Father.State.Transitioning) → Branch
      True → HasMatchingGameplayTag(Effect.Father.FormState) → Branch
        True → [Execute post-delay logic]
        False → Return (identity removed)
```

**Guard Semantics (NL-GUARD-IDENTITY L1):**
| Layer | Check | Detects |
|-------|-------|---------|
| 1 | IsValid(FatherRef) | Actor destroyed |
| 2 | Father.State.Transitioning | Ability cancelled |
| 3 | Effect.Father.FormState | External identity removal |

**Note:** Previous pattern used `CurrentForm == ExpectedForm` (enum check). This was replaced with GAS tag check per audit finding that tags are the GAS truth source, not external enums.

**Violation = race condition + corrupted state**

---

### Rule 3 — Toggle / Persistent Abilities
**Status:** LOCKED
**Examples:** GA_ProtectiveDome, GA_FatherExoskeletonSprint, GA_FatherRifle, GA_FatherSword

**Requirements:**
- Do NOT call `K2_EndAbility` on activation
- MUST have `Event_EndAbility` for cleanup
- Stay active until cancelled externally

**Pattern:**
```
Event_Activate → Setup state → [ability stays active]

Event_EndAbility → Cleanup state (remove GE, restore values)
```

---

### Rule 4 — First Activation Path (NEW v3.0)
**Status:** LOCKED
**Source:** Claude-GPT dual audit (2026-01-22)
**Severity:** Error

For form abilities using `bIsFirstActivation`, the True path may skip transition-only presentation (VFX, delay), but **must merge into the same stateful setup chain as the False path** before ending the ability.

**Rationale:** Guides define First Activation as a real setup path that continues into main logic, not an early-terminate path. If the manifest's True branch ends at `EndAbility_First`, that's a guide-contradicting wiring omission.

**Enforcement:** Manifest lint - True branch must reach SetupJoin node

**Affected Abilities:**
- GA_FatherArmor (lines 1667-1674)
- GA_FatherExoskeleton (lines 2238-2245)
- GA_FatherSymbiote (lines 2829-2836)
- GA_FatherEngineer (lines 3302-3309)

---

## VALIDATED TECHNICAL FINDINGS

### VTF-1: Delay vs Wait Delay (AbilityTask)
**Status:** VALIDATED
**Sources:**
- UE5.7 AbilityTask_WaitDelay.cpp (lines 37-49)
- UE5.7 AbilityTask.cpp (lines 197-206)
- UE5.7 GameplayTask.cpp (lines 53-67)
- [UE5 Gameplay Ability Tasks Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-tasks-in-unreal-engine)
- [GAS Documentation (tranek)](https://github.com/tranek/GASDocumentation)

| Node Type | Behavior | Protection Layers |
|-----------|----------|-------------------|
| **Delay** | Does NOT terminate when ability ends | 0 (continues running) |
| **Wait Delay (AbilityTask)** | Auto-terminates on EndAbility | 3 layers |

**Wait Delay 3-Layer Protection:**
1. **TaskOwnerEnded()** - Task destroyed when owning ability ends
2. **Delegate Auto-Unbinding** - UObject destruction cleans up delegates
3. **ShouldBroadcastAbilityTaskDelegates()** - Checks `Ability->IsActive()` before firing

```cpp
// AbilityTask.cpp:197-206
bool UAbilityTask::ShouldBroadcastAbilityTaskDelegates() const
{
    bool ShouldBroadcast = (Ability && Ability->IsActive());
    return ShouldBroadcast;
}
```

**Quote from UE5 docs:**
> "AbilityTasks... will be automatically ended on UGameplayAbility::EndAbility"

---

### VTF-2: ActivationOwnedTags Auto-Removal
**Status:** VALIDATED
**Source:** UE5.7 GameplayAbility.cpp (line 870)

ActivationOwnedTags are automatically removed when EndAbility() is called:

```cpp
// GameplayAbility.cpp:870
AbilitySystemComponent->RemoveLooseGameplayTags(ActivationOwnedTags, ...);
```

**Implication:** Tags like `Symbiote.State.Merged` (in ActivationOwnedTags) can be used as guards - if ability ended, tag is gone.

---

### VTF-3: Activation Required Tags NOT Continuously Checked
**Status:** VALIDATED
**Source:** UE5.7 GameplayAbility.cpp, Technical Reference v6.0 Section 11.11

Activation Required Tags are ONLY checked at activation time. They are NOT re-checked during ability execution.

**Problem:** GA_ProximityStrike has `Effect.Father.FormState.Symbiote` as Activation Required Tag. If Symbiote form ends mid-execution, GA_ProximityStrike continues running.

**Solution:** Use `Cancel Abilities with Tag` on form abilities to cascade-cancel dependent abilities.

---

### VTF-4: K2_OnEndAbility Blueprint Event
**Status:** VALIDATED
**Source:** UE5.7 GameplayAbility.h (line 621)

```cpp
// GameplayAbility.h:621
UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnEndAbility",
          meta = (ScriptName = "OnEndAbility"))
void K2_OnEndAbility(bool bWasCancelled);
```

- Blueprint-accessible via `Event OnEndAbility`
- `bWasCancelled` parameter distinguishes normal end vs cancellation
- Called AFTER ability cleanup (ActivationOwnedTags removed)

---

### VTF-5: No IsActive() Blueprint Node
**Status:** VALIDATED
**Source:** UE5.7 GameplayAbility.h (line 235)

```cpp
// GameplayAbility.h:235
bool IsActive() const; // No UFUNCTION macro = C++ only
```

`IsActive()` is NOT exposed to Blueprint. Cannot use "If Ability Is Not Active" guard pattern.

**Workaround:** Use ActivationOwnedTags as proxy - check `HasMatchingGameplayTag(Symbiote.State.Merged)` instead.

---

### VTF-6: Generator AbilityTask Support (IMPLEMENTED v4.15)
**Status:** VALIDATED → IMPLEMENTED
**Source:** GasAbilityGenerator Build.cs

**v4.15 Update:** Generator now supports AbilityTask nodes:
- Added GameplayAbilitiesEditor, GameplayTasksEditor dependencies
- Implemented `UK2Node_LatentAbilityCall` node generation
- New manifest node type: `type: AbilityTaskWaitDelay`
- 7 Delay nodes converted to WaitDelay in form abilities

---

### VTF-7: CommitCooldown Requires Explicit Call (NEW v3.0)
**Status:** LOCKED
**Source:** Claude-GPT dual audit (2026-01-22)
**Severity:** Error

If an ability defines `CooldownGameplayEffectClass`, at least one activation path must call `CommitAbility()` or `CommitAbilityCooldown()`; otherwise the cooldown GE won't be applied and tags used for ActivationBlocked won't be granted.

**Sources:**
- Epic docs for commit_ability_cooldown
- GAS Documentation (tranek)
- Common GAS references and sample projects

**Note:** Claude's original rule "must call K2_CommitAbilityCooldown" was too narrow. GPT correctly pointed out that cooldown can be applied by either:
- `CommitAbility()` (cost + cooldown)
- `CommitAbilityCooldown()` / `K2_CommitAbilityCooldown()` (cooldown only)

**Enforcement:** Manifest lint - cooldown_class requires commit node on at least one path

**Affected Abilities:**
- GA_FatherSymbiote - Missing CommitCooldown on ALL paths (both True and False)

---

### VTF-8: SetByCaller Requires Matching GE Modifier (NEW v3.0)
**Status:** LOCKED
**Source:** Claude-GPT dual audit (2026-01-22)
**Severity:** Error

SetByCaller tags only affect attributes when the target GE includes modifiers configured to read those tags; otherwise the values are ignored for attribute modification purposes.

**Sources:**
- Epic's Blueprint API for assigning SetByCaller magnitude
- GAS community references
- UEquippableItem::ModifyEquipmentEffectSpec analysis

**Evidence from Narrative Pro:**
```cpp
// EquippableItem.cpp:278-294
void UEquippableItem::ModifyEquipmentEffectSpec(FGameplayEffectSpec* Spec)
{
    Spec->SetSetByCallerMagnitude(SetByCaller_Armor, ArmorRating);
    Spec->SetSetByCallerMagnitude(SetByCaller_AttackRating, AttackRating);
    Spec->SetSetByCallerMagnitude(SetByCaller_StealthRating, StealthRating);
    // NO SetByCaller.StaminaRegenRate - not built-in
}
```

**Implication:** Adding `SetByCaller.StaminaRegenRate` to an item would be useless unless the GE has a modifier that reads that tag.

**Enforcement:** Design doc reference - ensure GE has matching modifier before using SetByCaller

---

## NOT LOCKED ITEMS AUDIT (v5.0)

### Audit Context
**Date:** 2026-01-24
**Auditors:** Claude + GPT (dual-agent)
**Method:** Challenge-based audit with evidence requirements

### NL-AOG-1: bActivateAbilityOnGranted
**Status:** VALIDATED (Narrative Pro Native)
**Source:** NarrativeGameplayAbility.h:29, NarrativeGameplayAbility.cpp:19-26

Narrative Pro explicitly supports auto-activation on grant:
```cpp
// NarrativeGameplayAbility.h:29
UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Narrative Ability")
bool bActivateAbilityOnGranted = false;

// NarrativeGameplayAbility.cpp:19-26
void UNarrativeGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnAvatarSet(ActorInfo, Spec);
    if (bActivateAbilityOnGranted)
    {
        ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);
    }
}
```

**Rule Text:**
> "Auto-activation-on-grant is permitted only for (a) setup abilities that end without waiting, or (b) abilities that follow the persistent-ability guard/cancel rules."

---

### NL-TASK-1: AbilityTasks for Async Waits
**Status:** ALREADY IMPLEMENTED
**Source:** manifest.yaml (all form abilities)

All form abilities use `type: AbilityTaskWaitDelay` instead of raw Delay nodes:
- GA_FatherCrawler: TransitionDelay (line 962)
- GA_FatherArmor: TransitionDelay (line 1422)
- GA_FatherExoskeleton: TransitionDelay (line 1973)
- GA_FatherSymbiote: TransitionDelay (line 2592)
- GA_FatherEngineer: TransitionDelay (line 3191)

**Evidence:** Comments in manifest: "v4.15: WaitDelay auto-terminates with ability (Track B GAS Audit)"

---

### NL-EQUIP-1: EquippableItem GE Delivery
**Status:** VALIDATED (Narrative Pro Native)
**Source:** EquippableItem.h:107, EquippableItem.cpp:35-41, 73-81

Narrative Pro equipment system uses GE-based stat delivery:
```cpp
// EquippableItem.h:107
TSubclassOf<UGameplayEffect> EquipmentEffect;

// EquippableItem.cpp:73-81
void UEquippableItem::HandleEquip_Implementation()
{
    if (ANarrativeCharacter* CharacterOwner = GetOwningNarrativeCharacter())
    {
        AbilityHandles = CharacterOwner->GrantAbilities(EquipmentAbilities, this);
        ApplyEquipmentAttributes();  // Applies EquipmentEffect GE
    }
}
```

**Rule Text:**
> "All long-lived stat changes must be delivered via GameplayEffects applied by Narrative inventory/equipment, not by transient ability graph logic."

---

### NL-GUARD-IDENTITY (L1): Post-Delay Identity Guard
**Status:** LOCKED
**Source:** Claude-GPT dual audit (2026-01-24)
**Consensus:** Both auditors agreed

**Rule Text:**
> "After async wait in form transition completion, validate:
> (1) actor valid (IsValid check),
> (2) Father.State.Transitioning present (phase/liveness check),
> (3) Effect.Father.FormState tag hierarchy present (identity check)."

**Evidence:**

| Guard Layer | Purpose | Failure Mode Detected |
|-------------|---------|----------------------|
| IsValid(FatherRef) | Actor exists | Father destroyed during delay |
| Father.State.Transitioning | Ability not cancelled | EndAbility removed tag |
| Effect.Father.FormState | Form identity intact | External GE removal |

**Implementation Notes:**
- Guards execute BEFORE form state swap (old identity still present)
- Check parent tag `Effect.Father.FormState` (not specific form)
- All form abilities blocked by `Father.State.Transitioning` in activation_blocked_tags

**Current Manifest Status:**
- Guards 1, 2 & 3: ✅ FULLY IMPLEMENTED (manifest.yaml v5.0)
- All 5 form abilities have complete 3-layer guard pattern

---

### NL-GUARD-IDENTITY (L2): Prior Form Tag Check
**Status:** DEFERRED
**Source:** Claude-GPT dual audit (2026-01-24)
**Reason:** Architecturally unreachable edge case

**What it would check:** `Effect.Father.FormState.<ExpectedPriorForm>` (specific form, not parent)

**Why deferred:**
1. Requires capturing prior form identity at activation time
2. Edge case (external mid-transition form manipulation) is blocked by `Father.State.Transitioning`
3. Implementation cost exceeds defense value in current architecture

**Future consideration:** If architecture changes to allow external identity mutation mid-transition, revisit L2.

---

### Form State Architecture (Option B) - REVALIDATED
**Status:** VALIDATED
**Source:** GasAbilityGeneratorTypes.h:907-919

Form identity delivered via GE_*State effects that grant identity tags:
```cpp
// GasAbilityGeneratorTypes.h:913
GE.GrantedTags.Add(FString::Printf(TEXT("Effect.Father.FormState.%s"), *Form));
```

**Identity Tags (GAS Truth Source):**
| Form | GE | Identity Tag |
|------|-----|--------------|
| Crawler | GE_CrawlerState | Effect.Father.FormState.Crawler |
| Armor | GE_ArmorState | Effect.Father.FormState.Armor |
| Exoskeleton | GE_ExoskeletonState | Effect.Father.FormState.Exoskeleton |
| Symbiote | GE_SymbioteState | Effect.Father.FormState.Symbiote |
| Engineer | GE_EngineerState | Effect.Father.FormState.Engineer |

**Transition Flow:**
1. Add `Father.State.Transitioning` (blocks other form abilities)
2. Wait 5s (AbilityTaskWaitDelay)
3. Guards validate state
4. `BP_RemoveGameplayEffectFromOwnerWithGrantedTags(Effect.Father.FormState)` - removes prior
5. `BP_ApplyGameplayEffectToOwner(GE_*State)` - applies new
6. Remove `Father.State.Transitioning`

---

## SEVERITY MATRIX (ALL CRITICAL RESOLVED)

### CRITICAL — ✅ ALL RESOLVED (v4.26)

| Ability | Issue | Status | Resolution |
|---------|-------|--------|------------|
| GA_FatherExoskeletonDash | Dead nodes referencing removed GE_DashInvulnerability | ✅ Fixed | Lines 4400, 4416 |
| GA_FatherSacrifice | Incomplete flow - ends at SetIsMonitoring | ✅ Fixed | Lines 5377-5689 |
| GA_CoreLaser | No event_graph defined | ✅ Fixed | Lines 7854-7877 |
| GA_FatherCrawler | No Event_EndAbility + ORPHAN CONNECTIONS | ✅ Fixed | Lines 1052-1157 |
| GA_FatherArmor | First Activation terminates early (Rule 4) | ✅ Fixed | Lines 1663-1672 |
| GA_FatherExoskeleton | First Activation terminates early (Rule 4) | ✅ Fixed | Lines 2235-2244 |
| GA_FatherSymbiote | First Activation terminates early (Rule 4) + Missing CommitCooldown (VTF-7) | ✅ Fixed | Lines 2834-2843, 2872-2874 |
| GA_FatherEngineer | First Activation terminates early (Rule 4) | ✅ Fixed | Lines 3311-3320 |

### MEDIUM — Race Conditions

| Ability | Issue | Notes |
|---------|-------|-------|
| GA_FatherArmor | Guard (Branch_Valid) executes AFTER GE operations | Ineffective placement |
| GA_FatherExoskeleton | No post-delay guards | Has Event_EndAbility |
| GA_FatherEngineer | No post-delay guards | Has Event_EndAbility |
| GA_StealthField | Has Delay (8s), no guards | Miscategorized as toggle |
| GA_FatherRifle | No guards in Event_EndAbility | Persistent ability |
| GA_FatherSword | No guards in Event_EndAbility | Persistent ability |

### CORRECT — Gold Standard

| Ability | Pattern | Notes |
|---------|---------|-------|
| GA_FatherSymbiote | 3-layer guards + Event_EndAbility + bWasCancelled | Reference implementation (except CommitCooldown) |
| GA_FatherAttack | Instant (Rule 1) | No Event_EndAbility needed |
| GA_TurretShoot | Instant (Rule 1) | No Event_EndAbility needed |
| GA_ProximityStrike | Instant (Rule 1) | No Event_EndAbility needed |
| GA_DomeBurst | Instant (Rule 1) | No Event_EndAbility needed |
| GA_ProtectiveDome | Toggle (Rule 3) | Event_EndAbility for cleanup |
| GA_FatherExoskeletonSprint | Toggle (Rule 3) | Event_EndAbility for cleanup |

---

## CRITICAL DEFECT DETAILS (ALL RESOLVED v4.26)

### CRIT-1: GA_FatherExoskeletonDash Dead Nodes
**Status:** ✅ RESOLVED (v4.14)
**Location:** manifest.yaml lines 4400, 4416

**Original Issue:** Ability referenced removed GE_DashInvulnerability nodes.

**Resolution:**
- Removed MakeInvulnSpec and ApplyInvuln nodes
- Rewired: `SetPlayerRef → LaunchCharacter → Delay → CommitCooldown → EndAbility`
- Line 4400 comment: "v4.14: Rewired to skip dead invuln nodes (GAS Audit CRIT-2)"
- Line 4416 comment: "v4.14: Removed GetPlayerASC, MakeInvulnSpec, ApplyInvuln data flow (dead nodes)"

---

### CRIT-2: GA_FatherSacrifice Incomplete Flow
**Status:** ✅ RESOLVED (v4.26)
**Location:** manifest.yaml lines 5377-5689

**Original Issue:** Flow ended at `SetIsMonitoring` with no sacrifice logic.

**Resolution:** Full sacrifice logic implemented:
1. Apply GE_SacrificeInvulnerability to PLAYER (lines 5607)
2. Hide Father actor (line 5609)
3. Disable collision (line 5611)
4. Add Father.State.Dormant tag (line 5613)
5. Start dormant timer (line 5615)
6. Timer callback with guards (lines 5646-5669)
7. Event_EndAbility cleanup (lines 5682-5689)

---

### CRIT-3: GA_CoreLaser No Event Graph
**Status:** ✅ RESOLVED (v4.14)
**Location:** manifest.yaml lines 7854-7877

**Original Issue:** No event_graph section defined.

**Resolution:** Implemented minimal instant ability pattern:
```yaml
Event_Activate → CommitCooldown → EndAbility
```
- Line 7842 comment: "v4.14: Added minimal event_graph per GAS Audit CRIT-4 (instant ability pattern)"

---

### CRIT-4: GA_FatherCrawler Orphan Connections
**Status:** ✅ RESOLVED (v4.26)
**Location:** manifest.yaml lines 1052-1139

**Original Issue:** Connections referenced non-existent nodes.

**Resolution:**
1. Removed orphan connections
2. Added post-delay guards (lines 1052-1090)
3. Added Event_EndAbility handler (lines 1091-1139)
4. Fixed First Activation path to merge into setup chain (lines 1148-1157)

---

## DESIGN DECISIONS (Abilities Audit v1.0)

### Decision 1A: Symbiote Stats Delivery Method
**Status:** LOCKED
**Source:** Erdem approval (2026-01-22)

**Question:** How should Symbiote form stats (+100 AttackRating, Infinite Stamina) be delivered?

**Options:**
- Direct GE (GE_SymbioteBoost)
- EquippableItem system (EI_FatherSymbioteForm)

**Decision:** EquippableItem system

**Rationale:**
- EI_FatherSymbioteForm already wired with `equipment_modifier_ge: GE_EquipmentModifier_FatherSymbiote`
- SetByCaller pattern already working for AttackRating
- Consistent with other forms (Armor, Exoskeleton)

---

### Decision 1B: StaminaRegenRate Implementation
**Status:** LOCKED
**Source:** Erdem approval (2026-01-22)

**Question:** How to add StaminaRegenRate to Symbiote form?

**Background:**
- `GE_EquipmentModifier` is Narrative Pro built-in at `/NarrativePro/Pro/Core/Abilities/GameplayEffects/`
- Built-in supports: Armor, AttackRating, StealthRating via SetByCaller
- **No built-in SetByCaller.StaminaRegenRate tag exists**
- Attribute `NarrativeAttributeSetBase.StaminaRegenRate` EXISTS (verified in header)

**Options:**
| Option | Approach | Complexity |
|--------|----------|------------|
| A | Add hardcoded modifier to `GE_EquipmentModifier_FatherSymbiote` | Simple - Symbiote-specific |
| B | Create custom SetByCaller tag + modifier in parent | Complex - for shared use |

**Decision:** Option A - Direct modifier on child GE

**Implementation:**
```yaml
gameplay_effects:
  - name: GE_EquipmentModifier_FatherSymbiote
    folder: Effects/Forms
    parent_class: GE_EquipmentModifier
    duration_policy: Infinite
    modifiers:
      - attribute: NarrativeAttributeSetBase.StaminaRegenRate
        modifier_op: Override
        magnitude_type: ScalableFloat
        magnitude_value: 10000.0
```

---

### Decision 2: GE_ArmorBoost Removal
**Status:** LOCKED
**Source:** Erdem approval (2026-01-22)

**Question:** Can GE_ArmorBoost be removed?

**Analysis:**
| Stat | Design Doc | Implementation |
|------|------------|----------------|
| +50 Armor | Required | EI_FatherArmorForm via SetByCaller |
| -15% Speed | Required | GA_FatherArmor event graph (CharacterMovement) |

**Evidence:**
- GE_ArmorBoost has 0 references in manifest (orphan)
- Armor stat handled by EquippableItem
- Speed reduction must use CharacterMovement (no GAS attribute for MovementSpeed)
- GA_FatherArmor already has `SpeedPenaltyMultiplier = 0.85` and full event graph logic

**Decision:** Remove GE_ArmorBoost from manifest

---

### Decision 3: GE_SymbioteBoost Removal
**Status:** LOCKED
**Source:** Erdem approval (2026-01-22)

**Question:** Can GE_SymbioteBoost be removed?

**Analysis:**
| Stat | Design Doc | GE_SymbioteBoost (Current) | Correct Implementation |
|------|------------|---------------------------|------------------------|
| +100 AttackRating | Required | Has `Damage: 25.0` (wrong attr, wrong value) | EI_FatherSymbioteForm SetByCaller |
| Infinite Stamina | Required | Has `AttackSpeed` (DNE) | GE_EquipmentModifier_FatherSymbiote (Decision 1B) |

**Evidence:**
- GE_SymbioteBoost has 0 references in manifest (orphan)
- Modifiers are misconfigured (wrong attributes, wrong values)
- EquippableItem system handles stats correctly

**Decision:** Remove GE_SymbioteBoost from manifest

---

### Decision 4: GA_Backstab Scope
**Status:** LOCKED
**Source:** Erdem approval (2026-01-22)

**Question:** Should GA_Backstab be Exoskeleton-only or universal?

**3-Layer Audit:**
| Source | Grant Location | Form Availability |
|--------|---------------|-------------------|
| Design Doc (3.1) | Player Default Abilities array | All players (not tied to Father) |
| Guide (v1.6) | Player Default Abilities array | All Forms |
| Manifest | EI_FatherExoskeletonForm | Exoskeleton-only |

**Erdem Clarification:** GA_Backstab is a universal player ability, always available (not tied to Father attachment). Internal conditions (sight check, enemy unaware) still apply.

**Decision:** Universal - Remove from EI_FatherExoskeletonForm, player adds to Default Abilities manually

**Implementation:**
1. Remove `GA_Backstab` from `EI_FatherExoskeletonForm.abilities_to_grant` (line 6290)
2. Update comment at line 4795 (remove "granted via EI_FatherExoskeletonForm")
3. Update comments at lines 4809-4810 (remove cross-ASC gate reference)
4. Erdem manually adds to player's Default Abilities array

---

## ORPHAN GE REMOVAL SUMMARY

| Asset | Lines | Reason | Status |
|-------|-------|--------|--------|
| GE_ArmorBoost | 600-608 | 0 refs, stats handled by EI + BP | Approved for removal |
| GE_SymbioteBoost | 617-631 | 0 refs, misconfigured, stats handled by EI + child GE | Approved for removal |

---

## IMPLEMENTATION STRATEGY (LOCKED)

### Track A — Immediate Manifest Fixes
**Status:** ✅ COMPLETE (v4.26)
**Timeline:** Done

All CRITICAL blockers fixed via manifest.yaml updates:
1. ✅ GA_FatherCrawler - Removed orphan connections, added Event_EndAbility + guards (lines 1052-1157)
2. ✅ GA_FatherExoskeletonDash - Removed dead invuln nodes, rewired flow (lines 4400, 4416)
3. ✅ GA_FatherSacrifice - Implemented full sacrifice logic (lines 5377-5689)
4. ✅ GA_CoreLaser - Implemented minimal event_graph (lines 7854-7877)
5. ✅ GA_FatherCrawler, GA_FatherArmor, GA_FatherExoskeleton, GA_FatherSymbiote, GA_FatherEngineer - Fixed First Activation paths (Rule 4)
6. ✅ GA_FatherSymbiote - Added CommitCooldown (lines 2872-2874)

MEDIUM issues (guards) also addressed in form abilities with post-delay guards.

---

### Track B — Generator Enhancement (IMPLEMENTED v4.15)
**Status:** COMPLETE
**Timeline:** Done

Enhanced GasAbilityGenerator to support AbilityTask nodes:
1. Added GameplayAbilitiesEditor dependency
2. Implemented `UK2Node_LatentAbilityCall` node generation
3. Added new manifest node type: `type: AbilityTaskWaitDelay`
4. Replaced all Delay nodes with WaitDelay in form abilities

**This restores true GAS lifecycle safety.**

---

## MANIFEST CHANGES COMPLETED (v4.26)

**Status:** ALL COMPLETE
**Implemented:** January 2026

### Additions (DONE)

| File | Change | Decision | Status |
|------|--------|----------|--------|
| `GE_EquipmentModifier_FatherSymbiote` | Add StaminaRegenRate modifier (Override, 10000.0) | 1B | ✅ Lines 599-603 |

### Removals (DONE)

| Asset | Reason | Decision | Status |
|-------|--------|----------|--------|
| `GE_ArmorBoost` | Orphan (0 refs), stats handled by EI + BP | 2 | ✅ Line 620 comment |
| `GE_SymbioteBoost` | Orphan (0 refs), misconfigured, stats handled by EI + child GE | 3 | ✅ Line 627 comment |

### Modifications (DONE)

| File | Change | Decision | Status |
|------|--------|----------|--------|
| `EI_FatherExoskeletonForm` | Remove `GA_Backstab` from `abilities_to_grant` | 4 | ✅ Line 6302 |
| `GA_Backstab` comments | Update grant location comments | 4 | ✅ Lines 4806-4810 |
| `GA_FatherCrawler` | Rewire First Activation True path to merge with setup chain | Rule 4 | ✅ Lines 1148-1157 |
| `GA_FatherArmor` | Rewire First Activation True path to merge with setup chain | Rule 4 | ✅ Lines 1663-1672 |
| `GA_FatherExoskeleton` | Rewire First Activation True path to merge with setup chain | Rule 4 | ✅ Lines 2235-2244 |
| `GA_FatherSymbiote` | Rewire First Activation True path + Add CommitCooldown | Rule 4 + VTF-7 | ✅ Lines 2834-2843, 2872-2874 |
| `GA_FatherEngineer` | Rewire First Activation True path to merge with setup chain | Rule 4 | ✅ Lines 3311-3320 |

---

## GA_FatherSymbiote — GOLD STANDARD REFERENCE

### 3-Layer Guard Pattern (NL-GUARD-IDENTITY L1)

```yaml
# Guard 1: Validate FatherRef
- id: Branch_FatherValid_Guard
  type: Branch
  # Condition: IsValid(FatherRef)

# Guard 2: Check Father.State.Transitioning (phase/liveness)
- id: Branch_TransitioningTag_Guard
  type: Branch
  # Condition: HasMatchingGameplayTag(Father.State.Transitioning)

# Guard 3: Check Effect.Father.FormState (identity) - ✅ AUTOMATED
- id: Branch_FormState_Guard
  type: Branch
  # Condition: HasMatchingGameplayTag(Effect.Father.FormState)
```

**Note (v5.0):** Previous pattern used `CurrentForm == Symbiote` enum check. Updated to use GAS tag checks per NL-GUARD-IDENTITY audit finding. Enum checks are outside GAS truth source and can desync.

### Event_EndAbility Handler (lines 2435-2490)

```yaml
- id: Event_EndAbility
  type: Event
  properties:
    event_name: K2_OnEndAbility

- id: Branch_WasCancelled
  type: Branch
  # Condition: bWasCancelled
  # True → Cleanup (clear timer, show Father, activate Armor)
  # False → Normal end (already cleaned up)
```

**All form abilities should follow this pattern.**

---

## FORM STATE ARCHITECTURE (Option B - VALIDATED)

### Architecture Summary
**Status:** VALIDATED and IMPLEMENTED

| Component | Description |
|-----------|-------------|
| GE_*State | Infinite-duration GE grants form identity tag |
| ActivationOwnedTags | Ephemeral state (e.g., `Symbiote.State.Merged`) |
| Transition Prelude | Remove old GE_*State via parent tag, apply new GE_*State |

**Form Identity Tags:**
- `Effect.Father.FormState.Crawler`
- `Effect.Father.FormState.Armor`
- `Effect.Father.FormState.Exoskeleton`
- `Effect.Father.FormState.Symbiote`
- `Effect.Father.FormState.Engineer`

**Removal Pattern:**
```yaml
# Remove ALL prior form states (parent tag match)
- BP_RemoveGameplayEffectFromOwnerWithGrantedTags
  Tags: Effect.Father.FormState  # Parent removes all children
```

---

## TAG HIERARCHY MATCHING (VALIDATED)

### How Tag Matching Works
**Source:** UE5.7 GameplayTagContainer.cpp

| Container Has | Check For | Result |
|---------------|-----------|--------|
| Parent.Child (child) | Parent (parent) | TRUE |
| Parent (exact) | Parent (exact) | TRUE |
| Parent (only) | Parent.Child (child) | FALSE |

`HasMatchingGameplayTag()` expands to include parents of asset tags.
`HasTagExact()` does NOT expand - requires exact match.

---

## SPEED/JUMP RESTORATION PATTERN

### Pattern: Store-on-Activate, Restore-on-End
**Status:** VALIDATED (Low Priority Concern)

Current pattern:
1. On activate: Store `OriginalMaxWalkSpeed = CharacterMovement.MaxWalkSpeed`
2. Apply boost: `MaxWalkSpeed = Original * Multiplier`
3. On end: Restore `MaxWalkSpeed = OriginalMaxWalkSpeed`

**Edge Case:** If player has other speed modifier active (sprint, slow debuff) when Symbiote activates, "original" captures the MODIFIED value.

**Assessment:** Low priority. In practice:
- Sprint is typically cancelled when entering combat/ability
- Debuffs are temporary and would expire
- The pattern works for normal gameplay

**Future Improvement (Optional):** Store baseline speed once on character spawn, not during ability activation.

---

## KNOWN LIMITATIONS (v4.27)

### ~~LIM-1: Automatic Dome Burst on Form Exit~~ RESOLVED
**Status:** ✅ IMPLEMENTED (v4.27)
**Affected:** GA_ProtectiveDome, GA_DomeBurst
**Resolved:** 2026-01-23

**Implementation (Decisions 22-24):** When armor form is unequipped and dome is fully charged, GA_ProtectiveDome's EndAbility automatically triggers GA_DomeBurst.

**Solution:**
1. GA_DomeBurst reordered before GA_ProtectiveDome in manifest (TSubclassOf session cache ordering)
2. GA_ProtectiveDome EndAbility now calls `TryActivateAbilityByClass(GA_DomeBurst)`
3. GA_DomeBurst has `activation_required_tags: Father.Dome.FullyCharged` - only fires if dome is charged
4. EndAbility also clears `Father.Dome.FullyCharged` tag and removes dome absorption GE

**Note:** HandleUnequip approach abandoned (GetEquipmentComponent is C++ only). EndAbility on GA_ProtectiveDome achieves the same result.

---

## CROSS-REFERENCES

| Document | Version | Relevance |
|----------|---------|-----------|
| Father_Companion_System_Design_Document | v2.1 | Form stats, GA_Backstab scope |
| GA_Backstab_Implementation_Guide | v1.6 | Grant location, detection logic |
| GA_FatherArmor_Implementation_Guide | v4.6 | Speed reduction, attachment |
| GA_FatherSymbiote_Implementation_Guide | v4.6 | Stat delivery, duration |
| EquippableItem.cpp | - | SetByCaller system |
| NarrativeAttributeSetBase.h | - | StaminaRegenRate attribute |

---

## DOCUMENT CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-20 | Initial creation from dual-agent audit session |
| 2.0 | 2026-01-21 | Added EndAbility rules (Rule 1/2/3), complete severity matrix, orphan connections finding (CRIT-4), GA_StealthField reclassification, Track A/B strategy, GA_FatherSymbiote gold standard reference, web search sources for VTF-1/VTF-6, updated FIX-1/FIX-2 status |
| 3.0 | 2026-01-22 | Merged Abilities_Audit_v1.md. Added Rule 4 (First Activation path). Added VTF-7 (CommitCooldown explicit call). Added VTF-8 (SetByCaller requires matching modifier). Added Design Decisions 1A-4 from abilities audit. Added orphan GE removal decisions (GE_ArmorBoost, GE_SymbioteBoost). Updated severity matrix with Rule 4 violations. Updated Track B status to COMPLETE (v4.15). |
| 4.0 | 2026-01-23 | **ALL MANIFEST CHANGES COMPLETE.** Verified all 4 critical defects resolved (CRIT-1 to CRIT-4). Verified all First Activation paths fixed (5 form abilities). Verified CommitCooldown added to GA_FatherSymbiote. Verified orphan effects removed. Updated severity matrix to show all resolved. Document now serves as historical reference for audit decisions. |
| 4.1 | 2026-01-23 | **DUAL-AGENT AUDIT VERIFICATION.** Claude-GPT audit verified all document claims against manifest.yaml. All content correct (line numbers stale but non-blocking). Audit grade: PASS. Options deferred: Hard Freeze, Enforcement Layer, External Snapshot - none currently needed. |
| 4.2 | 2026-01-23 | Added **KNOWN LIMITATIONS** section (v4.27). Documented LIM-1: Automatic dome burst on form exit cannot be automated due to C++-only functions and TSubclassOf parameter limitations. |
| 5.0 | 2026-01-24 | **NOT LOCKED ITEMS AUDIT.** Claude-GPT dual audit of 4 NOT LOCKED items. Locked NL-GUARD-IDENTITY (L1) - 3-layer guard with GAS tag checks. Validated bActivateAbilityOnGranted (Narrative Pro native). Validated AbilityTasks (already implemented). Validated EquippableItem GE delivery. Deferred NL-GUARD-IDENTITY (L2) - prior form tag check not needed in current architecture. Updated Rule 2 guard pattern to use tag checks instead of enum. Updated gold standard reference. |

---

**END OF LOCKED DECISIONS DOCUMENT**

**STATUS: FULLY AUTOMATED** - All audit findings implemented and verified. NL-GUARD-IDENTITY (L1) locked and fully automated in manifest.yaml v5.0. All 5 form abilities have complete 3-layer guard patterns. All manual implementation items converted to ✅ Auto-generated.
