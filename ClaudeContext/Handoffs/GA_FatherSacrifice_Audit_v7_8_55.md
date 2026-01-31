# GA_FatherSacrifice Audit Handoff v7.8.55

## Session Summary

**Date:** January 2026
**Audit Type:** Claude-GPT Dual Audit
**Scope:** GA_FatherSacrifice comprehensive audit against manifest, guides, LOCKED contracts, Narrative Pro systems

---

## Decisions Made (All LOCKED)

### D-SACRIFICE-TARGET-1 (CRITICAL)
**Issue:** Invulnerability GE applied to Father (Owner) instead of Player (Target)
**Decision:** Change `BP_ApplyGameplayEffectToOwner` → `BP_ApplyGameplayEffectToTarget` with Player ASC
**Status:** Approved for implementation

### D-SAC-1/2/3 (Architecture)
**Issue:** Manifest triggers sacrifice immediately; guide specifies 15% health threshold monitoring
**Decision:** Implement passive health monitoring using `AbilityAsyncWaitAttributeChanged`
**Pattern:** Same as GA_ProtectiveDome (proven working, v7.8.0+)
**Status:** Approved for implementation

### D-SAC-5 (Tag Semantics)
**Issue:** `Father.State.Offline` vs `Father.State.Dormant` used inconsistently
**Decision:** Use `Father.State.Dormant` ONLY. Remove all `Offline` references.
**Rename:** `GE_FatherOffline` → `GE_FatherDormant`
**Status:** Approved for implementation

### D-SAC-7 (Visual)
**Issue:** Guide says attach to chest socket; manifest hides actor
**Decision:** VFX + Hide with placeholder `GC_FatherSacrificeDormant` cue
**Status:** Approved for implementation

### D-SAC-8 (Reactivation)
**Issue:** Guide says Armor form on reactivation; manifest just shows/ends
**Decision:** Activate `GA_FatherArmor` when dormant timer expires
**Status:** Approved for implementation

### D-SAC-9 (Duration Policy)
**Issue:** Guide says infinite duration; manifest uses SetByCaller
**Decision:** Keep SetByCaller 180s (self-cleaning, current manifest)
**Status:** No change needed

---

## Protection Architecture (LOCKED)

### Layer A: 15% Threshold Monitoring (Blueprint)
- **Mechanism:** `AbilityAsyncWaitAttributeChanged` monitors Player Health
- **Trigger:** When `Health / MaxHealth < 0.15`
- **Protection:** Gradual damage, burst chains, multi-hit scenarios
- **Limitation:** Cannot prevent true one-shot kills that skip threshold

### Layer B: NOT IMPLEMENTED (User Decision)
- User explicitly chose NOT to implement C++ pre-damage interception
- User explicitly chose NOT to implement resurrection approach
- Accept partial protection per Layer A only

### Contract: C_SACRIFICE_THRESHOLD_MONITOR
```
Severity: FAIL
Requirement: When health < 15% and Sacrifice available, trigger automatically
Mechanism: AbilityAsyncWaitAttributeChanged + threshold branch
Implementation: GA_FatherSacrifice event graph (Blueprint)
```

---

## CanTriggerSacrifice() Invariant

For sacrifice to trigger, all conditions must be true:
- `NOT Cooldown.Father.Symbiote.Sacrifice`
- `NOT Father.State.Dormant`
- `NOT Narrative.State.IsDead` (Player)
- `Father.State.Recruited = true`
- `Father.State.Alive = true`

**Reference:** Contract C_SACRIFICE_DEAD_GUARD_C (Option C from earlier session)

---

## Files to Modify

### Manifest (manifest.yaml)
1. Remove `Father.State.Offline` tag definition (line ~372)
2. Remove `Father.State.Offline` from GE granted_tags (line ~854)
3. Rename `GE_FatherOffline` → `GE_FatherDormant`
4. Add `GameplayCue.Father.Sacrifice.Dormant` tag
5. Add `GC_FatherSacrificeDormant` placeholder cue
6. Fix invulnerability target (Owner → Target)
7. Add `AbilityAsyncWaitAttributeChanged` health monitoring
8. Add threshold check and sacrifice trigger flow
9. Add `GA_FatherArmor` activation in timer callback

### Guides
1. `GA_FatherSacrifice_Implementation_Guide_v2_8.md` - Replace `Offline` → `Dormant`
2. `Father_Companion_System_Design_Document_v2_8.md` - Replace `Offline` → `Dormant`

### INI Files
1. `ClaudeContext/DefaultGameplayTags_FatherCompanion_v4_0.ini` - Remove `Father.State.Offline`
2. `guides/DefaultGameplayTags_FatherCompanion_v4_1.ini` - Remove `Father.State.Offline`

---

## Implementation Order

### Phase 1: Tag Cleanup (Foundation) ✅ COMPLETED
1. ✅ Remove `Father.State.Offline` from manifest tags section (line 372)
2. ✅ Remove `Father.State.Offline` from GE granted_tags
3. ✅ Rename `GE_FatherOffline` → `GE_FatherDormant`

### Phase 2: Add New Assets ✅ COMPLETED
4. ✅ Add `GameplayCue.Father.Sacrifice.Dormant` tag (after line 305)
5. ✅ Add `GC_FatherSacrificeDormant` placeholder cue (gameplay_cues section)

### Phase 3: Fix Critical Bug ✅ COMPLETED
6. ✅ Change `BP_ApplyGameplayEffectToOwner` → `BP_ApplyGameplayEffectToTarget` with PlayerASC

### Phase 4: Implement Monitoring ✅ COMPLETED
7. ✅ Add `AbilityAsyncWaitAttributeChanged` for Health monitoring (WaitHealthChange node)
8. ✅ Add threshold check (Health/MaxHealth < 0.15) with DivideHealth, LessThanThreshold nodes
9. ✅ Wire sacrifice trigger on threshold breach (Branch_ThresholdCheck → SetIsMonitoring_False → ...)

### Phase 5: Complete Event Graph ✅ COMPLETED
10. ✅ Add cue trigger before hide (ExecuteGameplayCueDormant → GameplayCue.Father.Sacrifice.Dormant)
11. ✅ Add GA_FatherArmor activation in timer callback (TryActivateArmor → GA_FatherArmor)

### Phase 6: Documentation ✅ COMPLETED
12. ✅ Update guides - Offline → Dormant (GA_FatherSacrifice v2.9, System Design, Plugin Spec)
13. ✅ Update INI files (ClaudeContext/v4_0.ini, guides/v4_1.ini)

---

## LOCKED Contracts Applicable

| Contract | Relevance |
|----------|-----------|
| Contract 25 (C_NEVER_SIMPLIFY_ABILITIES) | Must match guide exactly |
| Contract 24 (D-DAMAGE-ATTR-1) | N/A - not a damage ability |
| Contract 27 (C_COOLDOWN_TAG_HIERARCHY) | Uses `Cooldown.Father.Symbiote.Sacrifice` |
| Contract 11 (C_SYMBIOTE_STRICT_CANCEL) | Sacrifice MAY cancel Symbiote (emergency override) |

---

## Evidence References

### AbilityAsyncWaitAttributeChanged Pattern
**[MANIFEST]** GA_ProtectiveDome lines 5168-5174:
```yaml
- id: WaitHealthChange
  type: AbilityAsyncWaitAttributeChanged
  properties:
    attribute_set: NarrativeAttributeSetBase
    attribute: Health
    trigger_once: false
```

### Death Pipeline Analysis
**[ENGINE]** NarrativeAbilitySystemComponent.cpp:111-143
- `HandleOutOfHealth()` does NOT re-check health
- Always sets `bIsDead = true` if not already dead-tagged
- OnOutOfHealth last-chance save NOT viable

### Invulnerability Check
**[ENGINE]** NarrativeAttributeSetBase.cpp:46
- `PreGameplayEffectExecute` checks `Narrative.State.Invulnerable`
- Blocks damage before application

---

## Session Notes

1. User rejected C++ modifications to Narrative Pro due to maintenance burden
2. User accepted partial protection (Layer A only) for 15% threshold
3. True one-shot protection requires C++ pre-damage interception (NOT implemented)
4. Resurrection approach was analyzed but rejected (child actors destroyed, bIsDead not re-checkable)

---

## Version

| Version | Date | Changes |
|---------|------|---------|
| v7.8.55c | January 2026 | GENERATION FIX - Changed PlayerASC to base ASC type, used NarrativeCharacter.GetHealth/GetMaxHealth instead of MakeAttribute, fixed Changed delegate name |
| v7.8.55b | January 2026 | ALL PHASES COMPLETED - manifest updated, guides aligned, INI files cleaned |
| v7.8.55 | January 2026 | Initial audit handoff - all decisions locked |

---

## Implementation Summary

All 6 phases completed:

1. **Tag Cleanup:** Removed `Father.State.Offline`, renamed `GE_FatherOffline` → `GE_FatherDormant`
2. **New Assets:** Added `GameplayCue.Father.Sacrifice.Dormant` tag and `GC_FatherSacrificeDormant` cue
3. **Critical Bug Fix:** Changed `BP_ApplyGameplayEffectToOwner` → `BP_ApplyGameplayEffectToTarget` with PlayerASC
4. **Health Monitoring:** Added `AbilityAsyncWaitAttributeChanged` with 15% threshold check
5. **Event Graph Complete:** Added dormant cue trigger before hide, `GA_FatherArmor` activation in timer callback
6. **Documentation:** Updated all guides and INI files

**Files Modified:**
- `ClaudeContext/manifest.yaml` - GA_FatherSacrifice event graph rewritten
- `guides/GA_FatherSacrifice_Implementation_Guide_v2_8.md` → v2.9
- `guides/Father_Companion_System_Design_Document_v2_8.md`
- `guides/Father_Ability_Generator_Plugin_v7_8_2_Specification.md`
- `ClaudeContext/DefaultGameplayTags_FatherCompanion_v4_0.ini`
- `guides/DefaultGameplayTags_FatherCompanion_v4_1.ini`
