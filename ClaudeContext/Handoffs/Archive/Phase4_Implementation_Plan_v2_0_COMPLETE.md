# Phase 4 Implementation Plan - Pre-Validation & Dependency Ordering

**Version:** 2.0 (Consolidated)
**Date:** 2026-01-22
**Status:** Phase 4.1 COMPLETE, Phase 4.2 COMPLETE

---

## Executive Summary

Phase 4 implements fail-fast pre-validation and dependency ordering with cascade skip logic. Both phases are now complete.

| Phase | Description | Status | Version |
|-------|-------------|--------|---------|
| Phase 4.1 | Pre-Validation System | ✅ COMPLETE | v4.24.2 |
| Phase 4.1.1 | Class Resolution Fixes | ✅ COMPLETE | v4.24.1 |
| Phase 4.1.2 | U/A Prefix Normalization | ✅ COMPLETE | v4.24.2 |
| Phase 4.2 | Dependency Ordering with Cascade Skip | ✅ COMPLETE | v4.25 |

---

## Phase 4.1: Pre-Validation System (COMPLETE)

### Implementation Summary

Pre-validation runs BEFORE generation, catching errors early with actionable diagnostics.

**Files Created:**
- `GasAbilityGeneratorPreValidator.h` - FPreValidator, FPreValidationCache declarations
- `GasAbilityGeneratorPreValidator.cpp` - Implementation

**Files Modified:**
- `GasAbilityGeneratorTypes.h` - FPreValidationIssue, FPreValidationReport structs
- `GasAbilityGeneratorCommandlet.cpp` - Pre-validation integration

### Validation Rules Implemented

| Rule | Description | Severity |
|------|-------------|----------|
| F1 | Class resolves to UClass | Error |
| F2 | Function exists on UClass | Error |
| A1 | AttributeSet class exists | Error |
| A2 | Attribute exists on AttributeSet | Error |
| C1 | Referenced UClass paths resolve | Error |
| C2 | Parent class exists | Error |
| R1-R5 | Asset references exist (AssetRegistry) | Error |
| T1 | GameplayTag registered | Warning |
| T2 | SetByCaller tag exists | Error |
| K1/K2 | Token validation | Error |

### Phase 4.1.1: Class Resolution Fixes

**Problem:** 77 false negative errors due to class resolution bugs.

**Fixes:**
1. Fixed CoreUObject bug at line 194: `*CorePath` → `*ClassName`
2. Added missing /Script paths: GameplayTags, Niagara, UMG, AIModule
3. Added diagnostic logging showing raw and normalized names

**Result:** Errors reduced 77 → 20

### Phase 4.1.2: U/A Prefix Normalization

**Problem:** UE5 /Script paths use class names WITHOUT U/A prefix (e.g., `UDialogue` → `/Script/NarrativeArsenal.Dialogue`)

**Fix:** `GetScriptClassName()` helper strips U/A prefix only if second char is uppercase (UE naming convention).

```cpp
// UClassName → ClassName (strip - 'C' is uppercase)
// UserWidget → UserWidget (keep - 's' is lowercase)
```

### Phase 4.1 Final: StaticLoadClass for Plugin Modules

**Problem:** NarrativeArsenal classes not found in headless mode (FindObject requires loaded classes).

**Fix:** Changed `FindObject<UClass>` to `StaticLoadClass` for NarrativeArsenal module lookups.

---

## Phase 4.2: Dependency Ordering with Cascade Skip (COMPLETE)

### Implementation Summary

Implements topological sorting for generation order and cascade skip logic when upstream assets fail.

**Files Modified:**
- `GasAbilityGeneratorTypes.h`:
  - Added `EGenerationStatus::SkippedCascaded`
  - Added cascade fields to `FGenerationResult`
  - Added `SkippedCascadedCount`, `CascadeRootFailures` to `FGenerationSummary`
  - Added `FDependencyGraph` class with Kahn's algorithm

- `GasAbilityGeneratorCommandlet.h`:
  - Added `BuildDependencyGraph()`, `CheckUpstreamFailure()`, `RegisterFailure()`
  - Added `FailedAssets`, `CascadeRoots`, `AssetDependencies` tracking maps

- `GasAbilityGeneratorCommandlet.cpp`:
  - Dependency graph built at generation start
  - Cascade check before each asset generation
  - Updated summary statistics with cascade counts
  - RESULT footer updated to schema v2

### Dependency Edges Tracked

| Edge | Source Field |
|------|--------------|
| GA → GE | `CooldownGameplayEffectClass` |
| GA → GA | `ParentClass` (if manifest-defined) |
| BT → BB | `BlackboardAsset` |
| NPC → DBP | `Dialogue` |
| NPC → AC | `AbilityConfiguration`, `ActivityConfiguration` |
| Activity → BT | `BehaviorTree` |

### Cascade Skip Logic

When an asset fails:
1. Failure registered in `FailedAssets` map with error code
2. Before generating dependent assets, `CheckUpstreamFailure()` checks if any dependency failed
3. If failed dependency found, asset marked `SkippedCascaded` with full chain path
4. Cascade skips NOT counted as failures (fix root cause, cascades auto-resolve)

### Summary Output (Schema v2)

```
RESULT: New=X Skipped=X Failed=X Deferred=X Cascaded=X CascadeRoots=X Total=X Headless=true/false
```

---

## Manifest Fixes Applied

| Issue | Fix |
|-------|-----|
| `Multiply_FloatFloat` | Changed to `Multiply_DoubleDouble` (UE5 double precision) |
| `GetComponentByClass` on `NarrativeInventoryComponent` | Changed `class:` to `Actor` |
| `parent_class: DialogueBlueprint` | Changed to `Dialogue` |

---

## Current Pre-Validation Status

**Last Run:** 11 errors, 7 warnings

**Remaining Errors (Missing Assets - Erdem to decide):**

| Code | Count | Issue |
|------|-------|-------|
| C1 | 2 | `BP_FatherRifleWeapon`, `BP_FatherSwordWeapon` not created |
| A2 | 3 | `Armor`, `Damage`, `AttackSpeed` not on UNarrativeAttributeSetBase |
| R1 | 6 | `SK_FatherCompanion` skeleton not imported |

**Warnings (generation proceeds):**
- T1 (7): Tags not registered (will be created during tag generation)

---

## Acceptance Criteria

### Phase 4.1 ✅
- [x] Pre-validation catches errors BEFORE generation
- [x] Errors block generation (return code 1)
- [x] Warnings do not block
- [x] Actionable report format: `[ErrorCode] RuleId | ItemId | Message | YAMLPath`
- [x] Cache stats reported

### Phase 4.2 ✅
- [x] Dependency graph built from manifest
- [x] Kahn's algorithm with lexical tie-breaking (deterministic)
- [x] Cascade skip when upstream fails
- [x] Full chain path in cascade results
- [x] Max depth capped at 16
- [x] Summary includes cascade stats

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-22 | Claude | Initial Phase 4.1 plan |
| 1.1 | 2026-01-22 | Claude | GPT audit R1-R6 changes |
| 2.0 | 2026-01-22 | Claude | Consolidated: Phase 4.1 + 4.1.1 + 4.1.2 + 4.2 complete |
