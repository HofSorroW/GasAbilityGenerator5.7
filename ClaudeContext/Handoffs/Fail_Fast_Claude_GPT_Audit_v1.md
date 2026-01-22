# Fail-Fast System - Claude-GPT Dual Audit Record

**Date:** 2026-01-22
**Participants:** Claude (Opus 4.5), GPT
**Status:** AUDIT COMPLETE - CONSENSUS REACHED
**Scope Decision:** Option 1 - Generation + Sync Pipelines Only

---

## Audit Rules (Enforced Throughout)

1. Both sides are auditors - challenge all claims
2. Demand proof - no acceptance by default
3. Validate against UE, Narrative Pro, and plugin code
4. Make research and web search when needed
5. No coding until Erdem approves

---

## Executive Summary

The Claude-GPT dual audit achieved full consensus on the fail-fast system design. No unresolved technical disagreements remain.

**Key Outcomes:**
- 114 silent failure points identified (expanded from original 87)
- 8 generator bugs proven (properties exist in NP but FindPropertyByName fails)
- R1/R2/R3 classification framework locked
- Phase 3 failure report format locked
- Scope: Generation + sync pipelines only (Option 1)

---

## Phase Plan (Erdem-Approved)

| Phase | Goal | Status |
|-------|------|--------|
| Phase 1 | Inventory + document all silent failures | ✅ COMPLETE |
| Phase 2 | Remove fallbacks so assets truly fail | ⏳ PENDING |
| Phase 3 | Report failing assets with diagnostics | ⏳ PENDING |
| Phase 4 | Build fail-proof system based on reasons | ⏳ PENDING |

---

## Locked Decisions

### 1. Terminology Definitions

| Term | Definition |
|------|------------|
| **Silent Fail** | Detects missing/invalid input, logs [INFO]/[WARN], continues producing incomplete asset |
| **Suppressed Error** | Converts error to default value/placeholder so downstream doesn't see failure |
| **Bypassed Fail** | Detects fatal condition but falls back to different behavior |

### 2. Scope Decision (LOCKED)

**Option 1 Selected:** Fail-fast applies to generation + sync pipelines only

**Rationale:** Directly solves "assets created but broken" without destabilizing editor tooling.

**Implications:**
- Editor UX fallbacks remain intact
- Pipeline correctness is enforced
- Each item tagged with `Scope=Pipeline` or `Scope=EditorUX`

### 3. R1/R2/R3 Classification Framework (LOCKED)

| Class | Condition | Action |
|-------|-----------|--------|
| **R1** | Manifest explicitly references something → cannot resolve | **HARD FAIL** |
| **R2** | Manifest omits optional field → documented default | ALLOWED (no warn) |
| **R3** | Structural invalidity (quests, graphs, etc.) | **HARD FAIL** |

**Key Separator:** "Manifest referenced?" YES/NO

### 4. Fallback Classification (LOCKED)

| Type | Description | Action |
|------|-------------|--------|
| **Type M (Masking)** | Manifest referenced X → X missing → generation continued | **ELIMINATE** |
| **Type D (Default)** | Manifest omitted optional X → uses default | ALLOWED if documented |

### 5. Phase 3 Failure Report Format (LOCKED)

Every hard fail must produce:

```
Asset path: /Game/FatherCompanion/Abilities/GA_Example
Subsystem: GAS | NPC | Dialogue | Quest | BT | Material | Niagara | Token
Reason code: E_EQUIPMENT_ABILITIES_NOT_FOUND
Human message: EquipmentAbilities property not found on class
Context payload:
  - GetClass()->GetPathName(): /Script/NarrativeArsenal.EquippableItem
  - GetClass()->GetSuperClass()->GetPathName(): /Script/NarrativeArsenal.NarrativeItem
  - PropertyNameRequested: EquipmentAbilities
```

This enables sorting failures into:
- Bad input data (Excel/YAML)
- Missing dependencies (assets/tags)
- Generator logic bugs

---

## 8 Generator Bugs - PROVEN

### Evidence

Properties exist in NarrativePro source but `FindPropertyByName()` fails at runtime.

| Property | Header | Line | Declaration |
|----------|--------|------|-------------|
| PartySpeakerInfo | Dialogue.h | 205 | `TArray<FPlayerSpeakerInfo> PartySpeakerInfo;` |
| EquipmentAbilities | EquippableItem.h | 136 | `TArray<TSubclassOf<class UNarrativeGameplayAbility>> EquipmentAbilities;` |
| Stats | NarrativeItem.h | 288 | `TArray<FNarrativeItemStat> Stats;` |
| ActivitiesToGrant | NarrativeItem.h | 280 | `TArray<TSubclassOf<class UNPCActivity>> ActivitiesToGrant;` |
| PickupMeshData | NarrativeItem.h | 509 | `FPickupMeshData PickupMeshData;` |
| TraceData | RangedWeaponItem.h | 86 | `FCombatTraceData TraceData;` |
| NPCTargets | NarrativeEvent.h | 173 | `TArray<TObjectPtr<class UNPCDefinition>> NPCTargets;` |
| CharacterTargets | NarrativeEvent.h | 169 | `TArray<TObjectPtr<class UCharacterDefinition>> CharacterTargets;` |

### Root Cause Hypotheses (Unproven - Phase 3 Will Capture)

| Hypothesis | Description |
|------------|-------------|
| H1 | Wrong UClass (fallback class, wrong parent) |
| H2 | Module/load/reflection timing |
| H3 | Name string mismatch vs reflected FName |
| H4 | `#if WITH_EDITORONLY_DATA` guards |
| H5 | Blueprint-generated class layout differs |

### Audit Consensus

- ✅ "Property exists" - PROVEN
- ✅ "Generator bug" - AGREED
- ❌ "Root cause" - UNPROVEN (deferred to Phase 3 runtime diagnostics)

---

## Quest Warnings → Hard Fails (LOCKED)

| Item | Line | Issue | Decision |
|------|------|-------|----------|
| S48 | 22346 | Quest has no valid start state | HARD FAIL |
| S49 | 22356 | Duplicate state ID | HARD FAIL |
| S50 | 22374 | Duplicate branch ID | HARD FAIL |
| S51 | 22512 | POI tag not registered | HARD FAIL |

**Rationale:** Structurally invalid state machine = runtime crashes, softlocks, nondeterministic behavior.

---

## v2.1 Documentation Requirements (Pre-Phase 2 Gate)

| Requirement | Format |
|-------------|--------|
| Anchor column | `FunctionName \| "Unique log substring"` |
| Manifest referenced? | YES / NO |
| Fallback type | M (Masking) or D (Default) |
| Scope | Pipeline / EditorUX |
| Subsystem | GAS / NPC / Dialogue / Quest / BT / Material / Niagara / Token |
| NP header proof | Declaration excerpts for 8 BUG items |

---

## Item Counts

| Category | Count |
|----------|-------|
| Category A (INFO manual setup) | 12 |
| Category B (WARN missing class) | 28 |
| Category C (Reward class) | 3 |
| Category D (Comment deferred) | 4 |
| Category S (Secondary warnings) | 51 |
| Category F (Fallback patterns) | 12 |
| Category T (Token registry) | 2 |
| Category W (Window/Editor) | 6 |
| **TOTAL** | **118** |

*Note: Some items overlap categories. Unique items for conversion: ~114*

---

## Audit Trail

| Round | Claude Position | GPT Position | Outcome |
|-------|-----------------|--------------|---------|
| 1 | v2 doc with 114 items, line corrections | v1.1 doc, challenged completeness | Merged findings |
| 2 | Proved 8 BUG items with full NP source | Accepted proof, challenged root cause | Root cause deferred |
| 3 | Accepted R1/R2/R3, Phase 3 diagnostics | Accepted, locked framework | Full consensus |
| 4 | Confirmed all locked items | Final confirmation | AUDIT COMPLETE |

---

## GPT Challenges Accepted by Claude

1. **Line number drift** - Multiple insertions, not single. Use anchors instead.
2. **Root cause hypothesis** - Withdrawn as unproven. Phase 3 will capture.
3. **Failure masking vs defaults** - Must distinguish before removal.
4. **Phase 3 must capture class identity** - Required for root cause diagnosis.

---

## Claude Challenges Accepted by GPT

1. **8 BUG items proven** - Full NP source grep accepted as evidence.
2. **Quest warnings = hard fails** - Structural invalidity cannot be warning-only.

---

## Next Steps

1. ✅ Scope decided: Option 1 (generation + sync pipelines)
2. ⏳ Create v2.1 document with anchors and classifications
3. ⏳ Phase 2: Remove failure-masking fallbacks
4. ⏳ Phase 3: Implement diagnostic failure reporting
5. ⏳ Phase 4: Build fail-proof system

---

## Related Documents

- `Fail_Fast_Audit_v1.md` - Original audit document
- `Fail_Fast_Audit_v2.md` - Expanded with line corrections and new findings
- `LOCKED_CONTRACTS.md` - Existing locked contracts (reference)
- `TODO_Tracking.md` - Task tracking

---

## Document History

| Version | Date | Changes |
|---------|------|---------|
| v1.0 | 2026-01-22 | Initial creation after Claude-GPT dual audit consensus |
