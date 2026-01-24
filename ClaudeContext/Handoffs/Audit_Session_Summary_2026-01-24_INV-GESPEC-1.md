# Audit Session Summary: INV-GESPEC-1 (MakeOutgoingGameplayEffectSpec)
## Date: 2026-01-24 | Auditors: Claude + GPT

---

## Executive Summary

**Finding:** 16 GameplayAbilities had broken `MakeOutgoingGameplayEffectSpec` nodes due to incorrect manifest syntax. The generator only processes `param.*` prefix properties for class pin assignment, but manifest used `gameplay_effect_class:` which was silently ignored.

**Impact:** Silent runtime failure - abilities appeared to generate correctly but GE applications did nothing at runtime.

**Resolution:** All 16 abilities fixed, invariant codified as INV-GESPEC-1/VTF-9/Contract 13.

---

## Discovery Chain

1. **Initial Investigation:** GA_StealthField audit revealed GE_StealthActive was not being applied
2. **Root Cause:** `gameplay_effect_class:` property ignored by generator
3. **Evidence Gathering:**
   - Generator code analysis (GasAbilityGeneratorGenerators.cpp:10335)
   - UE5.7 schema verification (EdGraphSchema_K2.h:394)
   - Pin dump test with intentional wrong name
4. **Scope Expansion:** Grep found 16 affected abilities

---

## Technical Details

### Root Cause

```cpp
// GasAbilityGeneratorGenerators.cpp:10335
if (PropPair.Key.StartsWith(TEXT("param.")))  // Only param.* processed
{
    FString PinName = PropPair.Key.Mid(6);
    // ...
    ParamPin->DefaultObject = ResolvedClass;  // Sets TSubclassOf<> pin
}
```

### Failure Mode

```
gameplay_effect_class: GE_StealthActive  →  Ignored by generator
                                         →  Pin left as nullptr
                                         →  MakeOutgoingSpec returns invalid handle
                                         →  ApplyGE silently no-ops
```

### Correct Syntax

```yaml
param.GameplayEffectClass: GE_StealthActive  # Pin name must match C++ parameter exactly
```

---

## Fixed Abilities (16)

| Ability | GE Applied |
|---------|------------|
| GA_FatherCrawler | GE_CrawlerState |
| GA_FatherArmor | GE_ArmorState |
| GA_FatherExoskeleton | GE_ExoskeletonState |
| GA_FatherSymbiote | GE_SymbioteState |
| GA_FatherEngineer | GE_EngineerState |
| GA_FatherRifle | GE_CrawlerState |
| GA_FatherSword | GE_CrawlerState |
| GA_FatherAttack | GE_FatherDamage |
| GA_FatherLaserShot | GE_LaserDamage |
| GA_TurretShoot | GE_FatherDamage |
| GA_CoreLaser | GE_CoreLaserDamage |
| GA_DomeBurst | GE_DomeBurstDamage |
| GA_ProtectiveDome | GE_DomeAbsorption |
| GA_StealthField | GE_StealthActive |
| GA_ProximityStrike | GE_ProximityDamage |
| GA_FatherMark | GE_MarkEffect |

---

## Additional Fixes (Same Session)

### GA_Backstab (CheckBackstabCondition)

**Issue:** `SelectObject` returns Object, but `BooleanOR` expects bool

**Fix:** Replaced with proper boolean OR chain:
- Added `NotValidGoal` node (NOT of IsValidGoal)
- Added `OrGoalCheck` node (OR of NotValidGoal, NotEqualPlayer)
- Logic: `CanBackstab = NoComponent OR (NoGoal OR TargetNotPlayer)`

### GA_FatherEngineer (R-AI-1 Activity System)

**Issue:** `GetComponentByClass` returns `ActorComponent`, `StopCurrentActivity` expects `NPCActivityComponent`

**Fix:** Added `CastToActivityComponent` dynamic cast between them

---

## Documentation Updates

| Document | Version | Changes |
|----------|---------|---------|
| Father_Companion_GAS_Abilities_Audit.md | v6.2 → v6.3 | Added VTF-9 |
| LOCKED_CONTRACTS.md | v4.30 → v4.31 | Added Contract 13 (INV-GESPEC-1) |
| Father_Companion_Technical_Reference | v6.6 → v6.7 | Added INV-GESPEC-1 to LOCKED rules |

---

## Verification Evidence

**GA_StealthField (verify_fix.log:16023):**
```
Set class parameter 'GameplayEffectClass' = 'GE_StealthActive_C' (TSubclassOf<GameplayEffect>)
```

**Generation Result:**
```
New=157 Skipped=0 Failed=0 Deferred=0
```

---

## Commits

| Hash | Description |
|------|-------------|
| `97cd164` | docs(v6.7): INV-GESPEC-1 documentation |
| `b5bbd74` | fix(manifest): GA_Backstab and GA_FatherEngineer generation errors |
| `4986ce7` | docs(Contract 13): Refine wording for precision |

---

## Audit Rules Followed

1. ✅ Both sides challenged assumptions (GPT challenged "Pattern B works" claim)
2. ✅ All claims verified with UE/Narrative Pro/plugin code
3. ✅ No coding without explicit approval
4. ✅ Invariant codified to prevent regression
5. ✅ Post-fix evidence artifact captured

---

## Key Learnings

1. **Silent failures are the worst failures** - The bug produced no errors, warnings, or visible symptoms during generation
2. **Pin type matters** - TSubclassOf<> pins use `DefaultObject`, not `DefaultValue`
3. **Pin names must match exactly** - Blueprint pin name = C++ parameter name (`GameplayEffectClass`)
4. **Codify invariants** - Without VTF-9/Contract 13, this bug could silently regress

---

## References

- Generator: `GasAbilityGeneratorGenerators.cpp:10335-10495`
- UE5.7: `EdGraphSchema_K2.h:394`
- Function: `GameplayAbility.h:226` (MakeOutgoingGameplayEffectSpec signature)
