# GasAbilityGenerator TODO Tracking

**Created:** 2026-01-18
**Updated:** 2026-01-21
**Plugin Version:** v4.15.1
**Status:** Consolidated tracking file for all pending tasks

---

## ✅ CRITICAL - GAS Audit Blockers (COMPLETED 2026-01-21)

**Source:** `Handoffs/Father_Companion_GAS_Audit_Locked_Decisions.md` v2.0

| Task | Ability | Fix Applied | Date |
|------|---------|-------------|------|
| CRIT-1 | GA_FatherCrawler | Removed orphan connections, added Event_EndAbility + guards | 2026-01-21 |
| CRIT-2 | GA_FatherExoskeletonDash | Removed dead invuln nodes, rewired flow | 2026-01-21 |
| CRIT-3 | GA_FatherSacrifice | Implemented full sacrifice logic with timer + guards | 2026-01-21 |
| CRIT-4 | GA_CoreLaser | Added minimal event_graph (instant ability pattern) | 2026-01-21 |

---

## ✅ MEDIUM - Race Condition Fixes (COMPLETED 2026-01-21)

| Ability | Fix Applied | Date |
|---------|-------------|------|
| GA_FatherArmor | Guards already present (verified) | 2026-01-21 |
| GA_FatherExoskeleton | Added IsValid + HasMatchingGameplayTag post-delay guards | 2026-01-21 |
| GA_FatherEngineer | Added IsValid + HasMatchingGameplayTag post-delay guards | 2026-01-21 |
| GA_StealthField | Added PlayerRef validity guard after 8s delay | 2026-01-21 |
| GA_FatherRifle | Added FatherRef validity guard in Event_EndAbility | 2026-01-21 |
| GA_FatherSword | Added FatherRef validity guard in Event_EndAbility | 2026-01-21 |

**Reference Implementation:** GA_FatherSymbiote (lines 2348-2400) - 3-layer guard pattern

---

## ✅ Track B - Generator Enhancement (COMPLETED 2026-01-21)

**Source:** `Handoffs/Father_Companion_GAS_Audit_Locked_Decisions.md` v2.0

| Task | Description | Commit |
|------|-------------|--------|
| AbilityTask Support | Added GameplayAbilitiesEditor + GameplayTasksEditor dependencies | `ed27cb5` |
| UK2Node_LatentAbilityCall | Implemented CreateAbilityTaskWaitDelayNode via reflection | `ed27cb5` |
| WaitDelay Node Type | Added `type: AbilityTaskWaitDelay` handler in node switch | `ed27cb5` |
| Replace Delay Nodes | Converted 7 Delay nodes to WaitDelay (5 form + 2 other) | `ed27cb5` |
| Call-site Correction | Fixed AddLooseGameplayTags/RemoveLooseGameplayTags (14 nodes) | `2bb1388` |

**Converted Nodes:**
- GA_FatherCrawler, GA_FatherArmor, GA_FatherExoskeleton, GA_FatherSymbiote, GA_FatherEngineer (TransitionDelay → AbilityTaskWaitDelay 5s)
- GA_FatherExoskeletonDash, GA_StealthField (Delay → AbilityTaskWaitDelay)

**Call-site Fix:** Changed `class: AbilitySystemComponent` → `class: AbilitySystemBlueprintLibrary` and `Target` pin → `Actor` pin for all AddLooseGameplayTags/RemoveLooseGameplayTags nodes (mechanical correctness, no logic change).

---

## ✅ Recently Completed - GAS Audit (v4.14.x)

| Task | Description | Date |
|------|-------------|------|
| FIX-1 | GA_FatherSymbiote Event_EndAbility + 3-layer guards | 2026-01-21 |
| FIX-2 | Timer Callback Guard Pattern (gold standard) | 2026-01-21 |
| INV-1 | Remove invulnerability from manifest (except GA_FatherSacrifice) | 2026-01-21 |

---

## Recently Completed (v4.12.4)

| Task | Location | Date |
|------|----------|------|
| NPC asset creation implementation | `NPCAssetSync.cpp:263` | 2026-01-18 |
| NPC asset creation implementation | `NPCXLSXSyncEngine.cpp:725` | 2026-01-18 |
| Remove v4.11 DEBUG code | `GasAbilityGeneratorGenerators.cpp:16085` | 2026-01-18 |
| POI preservation documentation | `SNPCTableEditor.cpp:2791` | 2026-01-18 |
| Token string building | `QuestTableConverter.cpp:366` | 2026-01-18 |
| Version alignment | `.uplugin`, module header, CLAUDE.md | 2026-01-18 |
| AutoBuildAndTest.ps1 path fixes | `Tools/AutoBuildAndTest.ps1` | 2026-01-18 |
| Full sync-from-assets (Item abilities) | `ItemAssetSync.cpp` | 2026-01-18 |
| Full sync-from-assets (Quest tasks) | `QuestAssetSync.cpp` | 2026-01-18 |

---

## Medium Priority (Next Quarter)

### Refactoring

| Task | Description | Complexity |
|------|-------------|------------|
| Split Generators.cpp | Split `GasAbilityGeneratorGenerators.cpp` (18,528 LOC) into per-generator files | HIGH |

**Details:**
- Current file is 18,528 lines - exceeds maintainability threshold
- Suggested structure:
  ```
  Source/GasAbilityGenerator/Private/Generators/
  ├── EnumerationGenerator.cpp
  ├── InputActionGenerator.cpp
  ├── GameplayEffectGenerator.cpp
  ├── GameplayAbilityGenerator.cpp
  ├── MaterialGenerator.cpp
  ├── NiagaraSystemGenerator.cpp
  ├── ... (one file per generator)
  └── GeneratorHelpers.cpp
  ```

### Metadata System

| Task | Description | Complexity |
|------|-------------|------------|
| Hash collision detection | Add detection/warning for hash collisions in metadata system | MEDIUM |

**Details:**
- Currently hash collisions are undetected
- Add collision tracking to `UGeneratorMetadataRegistry`
- Log warning when different inputs produce same hash

### Material Validation

| Task | Description | Complexity |
|------|-------------|------------|
| Circular reference detection | Detect circular expression references in material validation | MEDIUM |

**Details:**
- Current 6-guardrail system doesn't detect circular refs
- Add graph traversal in Pass 2 of validation
- Error code: `E_CIRCULAR_REFERENCE`

---

## Low Priority (Future)

### Performance

| Task | Description | Complexity |
|------|-------------|------------|
| Test large XLSX files | Test XLSX sync with >1000 rows | LOW |
| Parallelize generators | Parallelize generator execution for performance | HIGH |
| Batch validation | Add batch validation mode for large manifests | MEDIUM |

### Known Edge Cases (Untested/Unsupported)

| Area | Status | Notes |
|------|--------|-------|
| XLSX: Simultaneous external + YAML changes | UNTESTED | May cause sync conflicts |
| XLSX: Cell merge handling | UNSUPPORTED | Document as limitation |
| XLSX: Large files (>1000 rows) | UNTESTED | Performance unknown |
| Material: Circular expression refs | UNDETECTED | See task above |
| Metadata: Hash collisions | UNDETECTED | See task above |

---

## Test Framework (Proposed)

**Status:** Not yet implemented - Requires evaluation of ROI

### Phase 1: Validator Tests (Week 1)
- Test directory: `Source/GasAbilityGenerator/Tests/`
- Base class: `FGasAbilityGeneratorTestBase`
- Target: 10 validator methods across 4 validators

### Phase 2: Generator Smoke Tests (Week 2-3)
- Basic generation tests for all 36 generators
- Verify: Creation, metadata, no errors

### Phase 3: Sync System Tests (Week 4)
- 4 sync engines (NPC, Dialogue, Quest, Item)
- 14 sync status cases per engine

### Phase 4: Integration Tests (Week 5)
- End-to-end manifest → asset flows
- XLSX round-trip tests

---

## Acceptable Placeholders (By Design)

These are intentionally not implemented:

| File | Description | Reason |
|------|-------------|--------|
| `GasAbilityGeneratorWindow.cpp:1630` | Sound Wave placeholder | Audio assets require manual import |
| `GasAbilityGeneratorWindow.cpp:1633` | Level Sequence placeholder | Requires Sequencer editor |
| `SDialogueTableEditor.cpp:3415-3429` | Placeholder rows for empty dialogues | Editor UX requirement |

---

## Document History

| Date | Change |
|------|--------|
| 2026-01-18 | Created consolidated TODO file from System_Audit_v4.12.4.md and Architecture_Reference.md |
| 2026-01-18 | Completed 8 HIGH/MEDIUM priority tasks |
| 2026-01-21 | Added GAS Audit sections: CRITICAL blockers, MEDIUM race conditions, Track B future, Completed v4.14.x |
| 2026-01-21 | Track B COMPLETED: AbilityTaskWaitDelay support (7 nodes), call-site correction (14 nodes) → v4.15.1 |
