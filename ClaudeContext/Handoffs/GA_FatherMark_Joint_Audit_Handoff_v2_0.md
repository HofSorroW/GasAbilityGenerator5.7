# GA_FatherMark Joint Audit Handoff v2.0
## Claude + GPT Cross-Examination - FINAL
## Date: January 2026
## Status: AUDIT COMPLETE - IMPLEMENTATION AUTHORIZED

---

## Executive Summary

Joint audit between Claude (Opus 4.5) and GPT completed. Both auditors validated findings against UE5.7, GAS documentation, and Narrative Pro source code.

**Final Decision (Erdem):** Implement FULL gameplay/design logic per guide. Enhance manifest and generators as needed.

---

## Audit Session Results

### Participants
| Auditor | Model | Role |
|---------|-------|------|
| Claude | Opus 4.5 | Primary auditor, source code access |
| GPT | GPT-4 | Cross-examiner, contract analysis |

### Research Sources Validated
- [GAS Documentation (tranek)](https://github.com/tranek/GASDocumentation)
- [Epic GAS Documentation (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-abilities-in-unreal-engine)
- Narrative Pro Source Code (NarrativeAbilitySystemComponent.cpp)
- NarrativeDamageExecCalc.cpp (damage formula verification)

---

## Contract Proposals - Joint Verdict

### Research Conclusion (Both Auditors Agree)

| Proposal | Lockable? | Reason |
|----------|-----------|--------|
| C_MARK_MAILBOX_SAFETY | ⚠️ Not needed | Current manifest already correct (copy-on-activate) |
| C_MARK_MAX_CONCURRENCY_3 | ❌ No | Logic not implemented in manifest |
| C_MARK_REFRESH_SEMANTICS | ❌ No | No runtime behavior exists |
| C_MARK_UI_WALL_VISIBLE | ❌ No | Presentation-layer, non-deterministic |

### Mailbox Pattern Analysis

**GPT's Finding (validated by Claude):**
> "Clearing after read is NOT required. Immediate copy to local is the correct invariant."

**Evidence:**
- GAS ability activation is synchronous before latent nodes
- Blueprint nodes before first async execute atomically
- Narrative Pro uses similar patterns (PendingMusicSet, PendingUpdateList, etc.)

**Current Manifest Status:** CORRECT - copies PendingMarkTarget to TargetEnemy immediately on activation.

### Concurrency/Refresh/UI Analysis

**Joint Conclusion:** These are design-intent features described in guide but NOT implemented in manifest. Cannot lock contracts for unimplemented behavior.

**Decision:** Implement full logic per guide rather than lock incomplete patterns.

---

## Confirmed Discrepancies (Guide vs Manifest)

### Must Fix (Both Auditors Agree)

| # | Issue | Guide v1.9 | Manifest | Fix |
|---|-------|------------|----------|-----|
| 1 | GE Structure | 2 GEs | 1 GE (GE_MarkEffect) | Update guide to single GE |
| 2 | Tag Name | `Enemy.State.Marked` | `Character.Marked` | Update guide |
| 3 | Blocked Tag | Missing | `Father.State.Transitioning` | Add to guide |
| 4 | Variables | Hardcoded | SetByCaller pattern | Update guide |
| 5 | Armor Math | "~10%" flat | Conditional (~10% for 10-armor) | Clarify in docs |

### Must Implement (Manifest Enhancement)

| # | Feature | Guide v1.9 | Manifest | Action |
|---|---------|------------|----------|--------|
| 6 | Mark Count Check | Detailed | Variables exist, not used | **IMPLEMENT** |
| 7 | Oldest Mark Rotation | Detailed | Missing | **IMPLEMENT** |
| 8 | MarkedEnemies Array | Detailed | Variable exists, not used | **IMPLEMENT** |
| 9 | Widget Spawning | Detailed | Missing | **IMPLEMENT** |
| 10 | OnDied Cleanup | Detailed | Missing | **IMPLEMENT** |
| 11 | Widget Refresh | Detailed | Missing | **IMPLEMENT** |

---

## Implementation Plan

### Phase 1: Guide Corrections (Documentation)

Update `GA_FatherMark_Implementation_Guide_v1_9.md` → `v2_0.md`:

1. Replace `GE_FatherMark` + `GE_MarkDamageBonus` → `GE_MarkEffect`
2. Replace `Enemy.State.Marked` → `Character.Marked` (all occurrences)
3. Add `Father.State.Transitioning` to Activation Blocked Tags
4. Add SetByCaller variables (MarkDuration, MarkArmorReduction)
5. Update armor math description to clarify conditional nature
6. Update Related Documents versions

### Phase 2: Manifest Enhancement (Event Graph)

Enhance `manifest.yaml` GA_FatherMark event graph to include:

1. **Mark Count Check**
   - Get MarkedEnemies array from BP_FatherCompanion
   - Get Length, compare to MaxMarks
   - Branch: at limit → rotation logic, under limit → apply mark

2. **Oldest Mark Rotation**
   - Get first element of MarkedEnemies (index 0)
   - Validate (IsValid check)
   - Remove GE from oldest enemy ASC
   - Destroy widget for oldest enemy
   - Remove from MarkedEnemies array

3. **Apply Mark Flow**
   - Apply GE_MarkEffect to enemy ASC (existing)
   - Add enemy to MarkedEnemies array
   - Spawn WBP_MarkIndicator widget component
   - Store in MarkWidgetMap
   - Bind OnDied delegate for cleanup

4. **Refresh Path (Already Marked)**
   - Check if target already in MarkedEnemies
   - If yes: re-apply GE (duration refresh via stacking)
   - Refresh widget timer
   - Skip array add and widget spawn

5. **Cleanup Binding**
   - Bind to enemy ASC OnDied delegate
   - On death: destroy widget, remove from array/map

### Phase 3: Generator Enhancement (If Needed)

Check if generator supports:
- Array operations (Get, Length, Add, Remove)
- Map operations (Find, Add, Remove)
- Widget Component spawning (AddComponentByClass)
- Delegate binding (BindEvent)
- Screen Space widget configuration

If missing, enhance generator.

---

## Reference Patterns

### Narrative Pro Token System (Implementation Model)

From NarrativeAbilitySystemComponent.cpp:

| Function | Lines | Mark System Equivalent |
|----------|-------|------------------------|
| `TryClaimToken()` | 389-452 | Check limit + add mark |
| `ReturnToken()` | 467-484 | Remove mark on cleanup |
| `ShouldImmediatelyStealToken()` | 487-490 | Validate null/dead entries |
| `CanStealToken()` | 492-532 | Rotation logic |

### Widget Visibility

| Mode | Behavior | Use |
|------|----------|-----|
| Screen Space | Always on top (through walls) | **Mark indicator** ✅ |
| World Space | Can be occluded | N/A |

---

## Armor Math Clarification

### Formula (NarrativeDamageExecCalc.cpp)

```cpp
// Line 80: Armor clamped to >= 0
Armor = FMath::Max<float>(Armor, 0.0f);

// Line 94: Defense multiplier
const float DefenceMultiplier = (1.f + Armor / 100.f);

// Line 107: Final damage
float FinalDamage = ((Damage * AttackMultiplier) / DefenceMultiplier) * MaterialMultiplier;
```

### Actual Damage Increase from -10 Armor

| Target Armor | After -10 | Damage Increase |
|--------------|-----------|-----------------|
| 0 | 0 (clamped) | **0%** |
| 10 | 0 | **~10%** ✓ |
| 50 | 40 | **~7.1%** |
| 100 | 90 | **~5.3%** |

### Recommended Documentation Text

> "Applies Data.Mark.ArmorReduction (default -10). The damage increase varies by target armor: maximum ~10% benefit on low-armor targets; reduced benefit on high-armor targets; no benefit if target armor is already 0 due to engine clamping."

---

## Files to Modify

### Documentation
| File | Action |
|------|--------|
| `guides/GA_FatherMark_Implementation_Guide_v1_9.md` | Rename to v2_0, apply fixes |

### Manifest
| File | Action |
|------|--------|
| `ClaudeContext/manifest.yaml` | Enhance GA_FatherMark event graph |

### Generator (If Needed)
| File | Action |
|------|--------|
| `Source/GasAbilityGenerator/Private/GasAbilityGeneratorGenerators.cpp` | Add node types if missing |

---

## Validation Checklist (Post-Implementation)

### Guide v2.0
- [ ] Uses single `GE_MarkEffect` (not two GEs)
- [ ] Uses `Character.Marked` tag throughout
- [ ] Activation Blocked Tags includes `Father.State.Transitioning`
- [ ] Variables include MarkDuration, MarkArmorReduction with SetByCaller
- [ ] Armor math clarified as conditional
- [ ] Version updated to 2.0 with changelog

### Manifest GA_FatherMark
- [ ] Mark count check (MarkedEnemies.Length vs MaxMarks)
- [ ] Oldest mark rotation (validate, remove GE, destroy widget, remove from array)
- [ ] Add to MarkedEnemies array after apply
- [ ] Widget spawning (Screen Space)
- [ ] Store in MarkWidgetMap
- [ ] OnDied delegate binding for cleanup
- [ ] Refresh path (re-apply GE, refresh widget, skip spawn)

### Generator
- [ ] Supports array Get/Length/Add/Remove
- [ ] Supports map Find/Add/Remove
- [ ] Supports AddComponentByClass for WidgetComponent
- [ ] Supports BindEvent for delegates
- [ ] Supports SetWidgetSpace configuration

---

## Session Commits

| Commit | Hash | Description |
|--------|------|-------------|
| GA_DomeBurst fixes | `541a5f9` | Contract 24/24A + Contract 27 compliance |
| GA_FatherLaserShot fixes | `4d8c444` | Tag hierarchy, cooldown, version |
| Initial audit | `61ffa11` | First findings |
| Joint audit v1.0 | `9d39ce5` | Claude + GPT consensus |
| Research docs | `9b7a6ed` | Contract proposals investigation |
| Joint audit v2.0 | (pending) | Final + implementation plan |

---

## Authorization

**Erdem Decision:** Implement full gameplay/design logic per guide.

**Scope:**
- Fix guide documentation (5 items)
- Enhance manifest event graph (6 features)
- Enhance generator if needed

**Status:** AUTHORIZED TO PROCEED

---

**END OF JOINT AUDIT HANDOFF v2.0**

**Auditors:** Claude Opus 4.5, GPT-4
**Date:** January 2026
**Status:** AUDIT COMPLETE - IMPLEMENTATION AUTHORIZED
