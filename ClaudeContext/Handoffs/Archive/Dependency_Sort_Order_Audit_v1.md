# Dependency Sort Order Audit v1.0

## Audit Metadata

| Field | Value |
|-------|-------|
| Date | 2026-01-23 |
| Auditors | Claude (Opus 4.5), GPT |
| Audit Type | Dual-Agent Consensus Audit |
| Scope | Manifest dependency ordering, generation phase order |
| Outcome | **COMPLETE** - Quick-fix applied, 156/156 assets verified |

---

## Executive Summary

This audit examined the GasAbilityGenerator plugin's asset generation ordering to identify and resolve dependency-related failures. The primary trigger was GA_ProtectiveDome failing to resolve GA_DomeBurst via TSubclassOf because GA_DomeBurst was generated after GA_ProtectiveDome.

**Key Findings:**
1. TopologicalSort exists but is never called - dependency graph is informational only
2. Generation uses manifest order within hardcoded phases
3. Materials phase runs before MaterialFunctions phase (design defect, dormant)
4. Cross-phase dependencies (e.g., BP→GA) are theoretical risk, not evidenced in current manifest

---

## Audit Findings

### Finding 1: TopologicalSort Unused

**Status:** ✅ Proven

**Evidence:**
- `TopologicalSort()` declared at `GasAbilityGeneratorTypes.h:5865`
- Method is NEVER called anywhere in codebase
- Dependency graph built at `GasAbilityGeneratorCommandlet.cpp:2185-2312`
- Generation loops iterate `ManifestData.*` arrays directly (lines 959, 971, 1008, etc.)

**Impact:** Dependency edges are tracked but have no effect on generation order.

**Fix Required:** Code change - consume TopologicalSort result in generation loops.

---

### Finding 2: GA→GA Manifest Order Issue

**Status:** ✅ Proven and Fixed

**Evidence:**
- GA_ProtectiveDome.EndAbility calls `TryActivateAbilityByClass(GA_DomeBurst)`
- GA_DomeBurst was at manifest line ~4100, GA_ProtectiveDome at ~3915
- TSubclassOf resolution failed because GA_DomeBurst not yet in session cache

**Fix Applied:** Reordered GA_DomeBurst before GA_ProtectiveDome in manifest (v4.27).

---

### Finding 3: Materials Before MaterialFunctions (Phase Order Bug)

**Status:** ✅ Proven (Dormant)

**Evidence:**
- Materials generate at Phase 9 (line 1138)
- MaterialFunctions generate at Phase 10 (line 1150)
- `ResolveMaterialFunction()` at line 6643 uses 3-tier resolution:
  - Tier 1: Engine paths - LoadObject
  - Tier 2: Project paths - LoadObject
  - Tier 3: Session cache, then LoadObject
- If Material uses manifest MF, returns nullptr and logs `E_MAT_FUNCTION_NOT_FOUND`

**Current Impact:** None - current Materials don't use manifest-defined MaterialFunctions.

**Future Risk:** Will fail when a Material adds MaterialFunctionCall referencing manifest MF.

**Fix Required:** Code change - swap phase order in commandlet.

---

### Finding 4: BP→GA Cross-Phase Dependency

**Status:** ❌ Not Evidenced (Theoretical Risk)

**Analysis:**
- ActorBlueprints generate at Phase 3 (line 971)
- GameplayAbilities generate at Phase 4 (line 1008)
- If BP had `abilities` or `TSubclassOf<GA>` defaults, would fail

**Evidence Search:** Grep of `actor_blueprints:` section found no GA-class fields. Current BPs are "clean NPCs/services".

**Verdict:** Valid risk class for future, but not present in current manifest.

---

### Finding 5: EI→GA Dependency

**Status:** ✅ Verified (Not Order Issue)

**Evidence:**
- EquippableItems generate at Phase 16 (line 1222)
- GameplayAbilities generate at Phase 4 (line 1008)
- EI runs AFTER GA, so `LoadObject<UClass>` at line 16012 succeeds

**Verdict:** Hard dependency edge for graph completeness, but not an ordering problem.

---

## Current Phase Order (Commandlet.cpp)

| Phase | Asset Type | Line | Notes |
|-------|------------|------|-------|
| 1 | Enumerations | 908 | No deps |
| 1 | FloatCurves | 923 | No deps |
| 1 | InputActions | 935 | No deps |
| 1 | InputMappingContexts | 947 | No deps |
| 2 | GameplayEffects | 959 | Base |
| 3 | ActorBlueprints | 971 | Before GA |
| 4 | GameplayAbilities | 1008 | After BP |
| 5 | WidgetBlueprints | 1057 | |
| 6 | ComponentBlueprints | 1094 | |
| 7 | Blackboards | 1114 | |
| 8 | BehaviorTrees | 1126 | |
| 9 | Materials | 1138 | ⚠️ Before MF (bug) |
| 10 | MaterialFunctions | 1150 | ⚠️ Should be before M |
| 11 | MaterialInstances | 1162 | |
| 12 | TaggedDialogueSets | 1174 | |
| 13 | AnimationMontages | 1186 | |
| 14 | AnimationNotifies | 1198 | |
| 15 | DialogueBlueprints | 1210 | |
| 16 | EquippableItems | 1222 | After GA ✅ |

---

## Current Dependency Edges Tracked (v4.27)

| Edge Type | Status | Code Line |
|-----------|--------|-----------|
| GA → GE (cooldown) | ✅ Tracked | 2228 |
| BT → BB | ✅ Tracked | 2234 |
| AC → GA (abilities) | ✅ Tracked | 2243 |
| AC → GE (startup_effects) | ✅ Tracked | 2246 |
| AC → GE (default_attributes) | ✅ Tracked | 2249 |
| DialogueBP → NPC | ✅ Tracked | 2257 |
| Quest → NPC | ✅ Tracked | 2264 |
| GA → GA (TryActivateAbilityByClass) | ✅ Tracked | 2282 |

### Missing Edges (Future Work)

| Edge Type | Priority | Reason |
|-----------|----------|--------|
| BP → BP (parent_class) | P0 | Parent must exist first |
| Material → MF | P1 | Expression refs |
| MI → Material | P1 | Parent ref |
| ActivityConfig → Activity | P1 | TSubclassOf |
| Schedule → Goal | P2 | Class ref |

---

## Approved Quick-Fix Scope

### P0 (Must Do)

| Section | Action | Reason |
|---------|--------|--------|
| `actor_blueprints` | Parent BP before child BP | parent_class inheritance |
| `gameplay_abilities` | Dependency-first ordering | TSubclassOf refs |

### P1 (Should Do)

| Section | Action | Reason |
|---------|--------|--------|
| `ability_configurations` | After referenced GAs/GEs | TSubclassOf resolution |
| `activity_configurations` | After referenced Activities | TSubclassOf resolution |

### P1 Hygiene (No Runtime Effect Until Code Fix)

| Section | Action | Note |
|---------|--------|------|
| `material_functions` | Move before `materials` | Hygiene - phase bug dominates |
| `material_instances` | Parent MI before child MI | Hygiene - phase bug dominates |

---

## Code Fixes Required (Backlog)

| Issue | Fix | Priority |
|-------|-----|----------|
| TopologicalSort unused | Consume topo order in generation loops | P0 |
| M before MF phases | Swap phase order in commandlet | P1 |
| Missing edges | Add BP→BP, Material→MF, etc. | P1 |

---

## Audit Verdicts

| Finding | Status | Fix Type |
|---------|--------|----------|
| GA→GA manifest order | ✅ Proven issue | Quick-fix (manifest) |
| BP→BP parent order | ✅ Proven risk | Quick-fix (manifest) |
| M before MF phases | ✅ Proven bug (dormant) | Code fix required |
| TopologicalSort unused | ✅ Proven gap | Code fix required |
| BP→GA cross-phase | ❌ Not evidenced | Theoretical risk |
| EI→GA order | ✅ Verified OK | No fix needed |

---

## Implementation Completed

### Quick-Fix Applied (2026-01-23)

| Section | Action | Result |
|---------|--------|--------|
| `material_functions` | Moved before `materials` section | ✅ Done (manifest line 7292) |
| `gameplay_abilities` | GA_DomeBurst already before GA_ProtectiveDome | ✅ Already correct (v4.27) |
| `actor_blueprints` | All use external parent classes | ✅ No changes needed |
| `ability_configurations` | Already after referenced GAs/GEs | ✅ No changes needed |
| `activity_configurations` | Already after referenced Activities | ✅ No changes needed |

### Verification

```
Generation Result: 156/156 assets
- New: 156
- Skipped: 0
- Failed: 0
- Deferred: 0

VERIFICATION PASSED: All whitelist assets processed, counts match, no duplicates
```

### Commit

```
4c7f60c docs(v4.27): Dependency Sort Order Audit and manifest hygiene
```

---

## References

- `GasAbilityGeneratorCommandlet.cpp` - Phase order, dependency graph
- `GasAbilityGeneratorTypes.h:5865` - TopologicalSort declaration
- `GasAbilityGeneratorGenerators.cpp:6643` - ResolveMaterialFunction
- `GasAbilityGeneratorGenerators.cpp:16012` - EI abilities_to_grant LoadObject

---

**END OF AUDIT DOCUMENT**
