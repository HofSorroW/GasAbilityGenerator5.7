# Father Companion GAS Audit - Locked Decisions
## Version 1.0 - January 2026

**Purpose:** This document consolidates all validated findings and locked decisions from the dual-agent audit (Claude-GPT) conducted January 2026. These decisions are LOCKED and should not be debated again.

**Audit Context:** UE5.7 + Narrative Pro v2.2 + GasAbilityGenerator v4.13.x

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

## VALIDATED TECHNICAL FINDINGS

### VTF-1: Delay vs Wait Delay (AbilityTask)
**Status:** VALIDATED
**Sources:**
- UE5.7 AbilityTask_WaitDelay.cpp (lines 37-49)
- UE5.7 AbilityTask.cpp (lines 197-206)
- UE5.7 GameplayTask.cpp (lines 53-67)

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

**Decision:** For abilities using timer patterns (Symbiote 30s), either:
- A) Enhance generator to support AbilityTask nodes (preferred long-term)
- B) Use Delay + defensive guards in callback (immediate workaround)

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

### VTF-6: Generator AbilityTask Limitation
**Status:** VALIDATED
**Source:** GasAbilityGenerator Build.cs analysis

Current generator modules:
- Has: GameplayAbilities, GameplayTasks, BlueprintGraph, Kismet, KismetCompiler
- Missing: GameplayTasksEditor, GameplayAbilitiesEditor

AbilityTask nodes use `UK2Node_LatentAbilityCall` (in GameplayAbilitiesEditor module), not standard `UK2Node_CallFunction`.

**Impact:** Cannot currently generate Wait Delay AbilityTask nodes via manifest.

**Options:**
- A) Add GameplayAbilitiesEditor dependency, implement UK2Node_LatentAbilityCall support
- B) Use standard Delay + defensive guards (immediate)

---

## REQUIRED FIXES

### FIX-1: GA_FatherSymbiote Missing Event_EndAbility
**Status:** NEEDS IMPLEMENTATION
**Priority:** HIGH

GA_FatherSymbiote manifest does NOT have Event_EndAbility handler. If ability is cancelled (e.g., by another form's `Cancel Abilities with Tag`), cleanup doesn't run.

**Required:**
1. Add `Event_EndAbility` node to manifest event_graph
2. Clear timer via `Clear and Invalidate Timer by Handle`
3. Restore movement speed if stored
4. Call GA_FatherArmor activation (return to default form)

**Reference:** GA_FatherArmor guide PHASE 7 (has Event_EndAbility with proper cleanup)

---

### FIX-2: Timer Callback Missing Guards
**Status:** NEEDS IMPLEMENTATION
**Priority:** HIGH

Current manifest (lines 2299-2316):
```yaml
# === TIMER EXPIRED FLOW ===
- from: [CustomEvent_TimerExpired, Then]
  to: [ShowFather, Exec]  # Direct execution - NO GUARDS!
```

**Problem:** If timer fires after ability ended (edge case), callback executes without checking validity.

**Required Guards (in order):**
1. `IsValid(FatherRef)` - Father still exists
2. `CurrentForm == Symbiote` - Still in Symbiote form (enum check)
3. `HasMatchingGameplayTag(Symbiote.State.Merged)` - Ability still active (tag proxy)

**Pattern:**
```
CustomEvent_TimerExpired -> IsValid(FatherRef) -> Branch
  True -> GetCurrentForm -> Equal(Symbiote) -> Branch
    True -> HasMatchingGameplayTag(Symbiote.State.Merged) -> Branch
      True -> [Actual cleanup logic]
      False -> Return (ability already ended)
    False -> Return (form changed)
  False -> Return (father destroyed)
```

---

### FIX-3: Guide Invulnerability References
**Status:** NEEDS IMPLEMENTATION
**Priority:** MEDIUM

GA_FatherSymbiote_Implementation_Guide_v4_4.md has invulnerability references that must be removed:

| Line | Content | Action |
|------|---------|--------|
| 32 | `GE_TransitionInvulnerability` in Dependencies | REMOVE |
| 121 | `Narrative.State.Invulnerable` in GE_SymbioteState | REMOVE |
| 124 | Comment about attached forms granting invulnerability | REMOVE |
| 137 | `Granted Tags [1]` Narrative.State.Invulnerable | REMOVE |
| 392-418 | Apply GE_Invulnerable during transition | REMOVE section |
| 768-769 | GE_TransitionInvulnerability table entry | REMOVE |
| 838 | GE_SymbioteState grants Narrative.State.Invulnerable | UPDATE |
| 841 | GE_TransitionInvulnerability table entry | REMOVE |

Similar cleanup needed in:
- GA_FatherArmor_Implementation_Guide_v4_4.md
- GA_FatherExoskeleton_Implementation_Guide_v3_10.md
- Father_Companion_Technical_Reference_v6_0.md

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

## DOCUMENT CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-20 | Initial creation from dual-agent audit session |

---

**END OF LOCKED DECISIONS DOCUMENT**
