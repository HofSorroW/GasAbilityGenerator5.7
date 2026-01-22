# Phase 4 Specification - LOCKED

**Version:** 1.0
**Date:** 2026-01-22
**Status:** LOCKED (Claude-GPT Dual Audit Consensus)
**Auditors:** Claude (Opus 4.5), GPT

---

## Executive Summary

Phase 4 implements fail-proof automation through pre-validation and dependency ordering. This specification is normative and implementation-binding.

**Priority Order (LOCKED):**
1. Pre-validation (parse-time semantic checks)
2. Dependency ordering (topological sort + cascade handling)
3. 8 BUG root cause investigation (deferred until reproducible)

---

## 1. Pre-Validation System

### 1.1 Overview

Pre-validation runs **before** generation, inside the UE editor process. It performs reflection-based semantic checks and produces a complete "fix list" of all issues before any assets are touched.

**Key Differences from Generation-Time Checking:**

| Aspect | Current (Generation-Time) | Phase 4 (Pre-Validation) |
|--------|---------------------------|--------------------------|
| When | During asset generation | Before generation starts |
| Scope | Per-asset, stops at first fail | All assets, all checks batched |
| Output | Cascading failures | Complete fix list |
| Source mapping | Limited | Full manifest location (file, row, YAML path) |

### 1.2 Pre-Validation Rules

#### Function Validation

| Rule | Description | Severity |
|------|-------------|----------|
| **F1** | Verify `class` resolves to a valid UClass | Error |
| **F2** | Verify `function` exists on the resolved UClass | Error |

**Scope:** L1 (existence) only. Signature checking (L2/L3) is explicitly deferred.

| Level | Check | Status |
|-------|-------|--------|
| L1 | Function exists on class | **REQUIRED** |
| L2 | Param count matches | DEFERRED |
| L3 | Param types match | OUT OF SCOPE |

**Rationale:** L1 eliminates the dominant failure class (55× E_FUNCTION_NOT_FOUND in Phase 2). L2/L3 add complexity due to Blueprint coercion rules.

#### Attribute Validation

| Rule | Description | Severity |
|------|-------------|----------|
| **A1** | Resolve AttributeSet class for property reference | Error |
| **A2** | Verify property exists on AttributeSet via reflection | Error |

**AttributeSet Resolution Order:**
1. Use manifest-specified `attributeSetClass` if present
2. Else use project default: `UNarrativeAttributeSetBase`
3. If property not found → Error
4. If ambiguity arises → Error

**Explicitly NOT Done:**
- No global scan of all UAttributeSet subclasses
- No hardcoded attribute name lists

#### Class Validation

| Rule | Description | Severity |
|------|-------------|----------|
| **C1** | Verify all referenced UClass paths resolve | Error |
| **C2** | Verify parent class exists for Blueprint generation | Error |

#### Asset Reference Validation

| Rule | Description | Severity |
|------|-------------|----------|
| **R1** | Verify skeleton assets exist (for montages) | Error |
| **R2** | Verify texture assets exist (for materials) | Error |
| **R3** | Verify MaterialFunction assets exist | Error |
| **R4** | Verify referenced NPCDefinitions exist | Error |
| **R5** | Verify SoftObjectPath references resolve in AssetRegistry | Error |

#### Tag Validation

| Rule | Description | Severity |
|------|-------------|----------|
| **T1** | Verify GameplayTags are registered | Warning |
| **T2** | Verify SetByCaller tags exist | Error |

**Note:** T1 is Warning (generation can proceed), T2 is Error (GE will fail without it).

#### Token Validation

| Rule | Description | Severity |
|------|-------------|----------|
| **K1** | Verify token types are supported | Error |
| **K2** | Verify token property references are valid | Error |

### 1.3 Blocking Policy

| Severity | Blocks Generation? | Behavior |
|----------|-------------------|----------|
| **Error** | YES | Generation does not start |
| **Warning** | NO | Logged, generation proceeds with caution |
| **Info** | NO | Informational only |

**Examples:**
- Function not found → Error → BLOCK
- Class not found → Error → BLOCK
- Missing skeleton asset → Error → BLOCK
- Tag not registered → Warning → PROCEED
- Suggestion/hint → Info → LOG ONLY

### 1.4 Pre-Validation Output Format

```
[PRE-VAL] {Severity} | {RuleCode} | {Message}
  Manifest: {FilePath}:{Line}:{Column}
  YAML Path: {path.to.field}
  Resolution: {attempted resolution details}
```

**Example:**
```
[PRE-VAL] ERROR | F2 | Function 'SpawnSystemAttached' not found on class 'NiagaraFunctionLibrary'
  Manifest: ClaudeContext/manifest.yaml:1247:12
  YAML Path: actor_blueprints[0].event_graph.nodes[5].properties.function
  Resolution: UNiagaraFunctionLibrary loaded, FindFunctionByName returned nullptr
```

### 1.5 Caching Strategy

- Cache per unique (Class, FunctionName) pair
- Cache per unique (AttributeSet, PropertyName) pair
- Cache invalidated on manifest reload
- Single validation pass before generation session

---

## 2. Dependency Ordering System

### 2.1 Overview

Extend the existing dependency graph (v4.17 circular detection) to:
1. Include external asset dependencies
2. Enable topological sort for generation order
3. Support cascade skip logic

### 2.2 Dependency Edge Types

#### Existing Edges (from v4.17)

| From | To | Edge Type |
|------|-----|-----------|
| GA | GE | cooldown_effect |
| GA | Montage | animation_montage |
| BT | BB | blackboard |
| AC (Ability) | GA | abilities array |
| AC (Ability) | GE | startup_effects |
| AC (Activity) | Activity | activities array |

#### New External Edges (Phase 4)

| From | To | Edge Type |
|------|-----|-----------|
| Montage | Skeleton | skeleton_asset |
| Material Instance | Texture | texture_asset |
| Material | MaterialFunction | material_function |
| Dialogue BP | NPCDefinition | speaker_npc |
| Dialogue BP | NarrativeEvent | dialogue_event |
| Quest | NPCDefinition | questgiver_npc |
| Quest | Item | reward_item |
| NPC Spawner | NPCDefinition | spawned_npc |
| GE | AttributeSet | attribute_reference |
| GA | GameplayTag | ability_tag |
| Any | UClass | class_reference |

### 2.3 Generation Order

1. Build complete dependency graph (generated + external)
2. Validate no cycles (existing v4.17 check)
3. Topologically sort generated assets
4. Generate in sorted order (dependencies first)

**Determinism:** Same manifest always produces same generation order.

### 2.4 External Dependency Handling

External dependencies (assets not generated by manifest) are validated but not generated:

| External Type | Validation | On Missing |
|---------------|------------|------------|
| Skeleton | AssetRegistry lookup | Error (block) |
| Texture | AssetRegistry lookup | Error (block) |
| MaterialFunction | AssetRegistry lookup | Error (block) |
| UClass (native) | FindObject/LoadClass | Error (block) |
| GameplayTag | Tag registry lookup | Warning (proceed) |

---

## 3. Cascade Handling

### 3.1 Failure Categories

| Category | Definition | Counted In |
|----------|------------|------------|
| **FailedRoot** | Direct failure, not caused by upstream | Failure Total |
| **SkippedCascaded** | Skipped because upstream failed | Skip Total (separate) |
| **Succeeded** | Generated successfully | Success Total |

**Key Rule:** Cascaded skips are NOT counted as failures. This prevents inflated failure totals.

### 3.2 Cascade Error Code

**Code:** `E_CASCADE_SKIPPED`

**Payload:**
```
UpstreamAsset: {asset that failed}
UpstreamReasonCode: {original error code}
ChainDepth: {number of hops}
Chain: {A -> B -> C} (capped)
```

### 3.3 Chain Reporting

- Store full chain internally
- Print capped chain (max 3 hops)
- Use "(+N more)" suffix if longer

**Example Output:**
```
[E_CASCADE_SKIPPED] AC_FatherCompanion | Skipped due to upstream failure
  Chain: GA_FatherAttack (E_FUNCTION_NOT_FOUND) -> AC_FatherCompanion
  ChainDepth: 1
```

**Longer Chain Example:**
```
[E_CASCADE_SKIPPED] Widget_FatherHUD | Skipped due to upstream failure
  Chain: GE_Stats (E_GE_PROPERTY_NOT_FOUND) -> GA_FatherCrawler -> AC_FatherCompanion (+1 more)
  ChainDepth: 4
```

### 3.4 Cascade Logic

```
For each asset in topological order:
  If any dependency in FailedRoot or SkippedCascaded:
    Mark asset as SkippedCascaded
    Record chain from failed root
    Continue to next asset
  Else:
    Attempt generation
    If fails: Mark as FailedRoot
    If succeeds: Mark as Succeeded
```

---

## 4. Summary Statistics Format

Post-generation report should show:

```
=== Generation Summary ===
Pre-Validation: {N} errors, {M} warnings
  - Errors blocked generation: {YES/NO}

Generation Results:
  Succeeded:        {count}
  FailedRoot:       {count}
  SkippedCascaded:  {count}

Root Failures by Code:
  E_FUNCTION_NOT_FOUND:     {count}
  E_CLASS_NOT_FOUND:        {count}
  ...

Cascade Impact:
  Root failures:            {count}
  Total assets affected:    {root + cascaded}
  Cascade amplification:    {cascaded / root}x
```

---

## 5. Error Code Taxonomy (Phase 4 Additions)

### Pre-Validation Codes

| Code | Severity | Description |
|------|----------|-------------|
| E_PREVAL_CLASS_NOT_FOUND | Error | Referenced UClass doesn't exist |
| E_PREVAL_FUNCTION_NOT_FOUND | Error | Function doesn't exist on class |
| E_PREVAL_ATTRIBUTE_NOT_FOUND | Error | Attribute doesn't exist on AttributeSet |
| E_PREVAL_ASSET_NOT_FOUND | Error | Referenced asset not in AssetRegistry |
| E_PREVAL_TAG_NOT_REGISTERED | Warning | GameplayTag not registered |
| E_PREVAL_TOKEN_UNSUPPORTED | Error | Token type not supported |

### Cascade Codes

| Code | Severity | Description |
|------|----------|-------------|
| E_CASCADE_SKIPPED | Skip | Asset skipped due to upstream failure |

---

## 6. Manifest Extensions

### 6.1 Optional attributeSetClass Field

```yaml
gameplay_effects:
  - name: GE_CustomStats
    folder: Effects
    modifiers:
      - attribute: CustomAttribute
        attributeSetClass: UMyCustomAttributeSet  # Optional override
        modifier_op: Add
        magnitude: 10.0
```

If omitted, defaults to `UNarrativeAttributeSetBase`.

---

## 7. Deferred Items

### 7.1 Function Signature Checking (L2/L3)

- **Status:** Explicitly deferred
- **Trigger:** If L1 proves insufficient based on future failure data
- **Scope:** Param count (L2), param types (L3)

### 7.2 8 BUG Root Cause Investigation (H1-H5)

- **Status:** Deferred until reproducible
- **Trigger:** When 8 BUG error codes appear in generation logs
- **Approach:** Use Phase 3 context (ClassPath, SuperClassPath, RequestedProperty) to test hypotheses

| Hypothesis | Description |
|------------|-------------|
| H1 | Wrong UClass (fallback class, wrong parent) |
| H2 | Module/load/reflection timing |
| H3 | Name string mismatch vs reflected FName |
| H4 | `#if WITH_EDITORONLY_DATA` guards |
| H5 | Blueprint-generated class layout differs |

### 7.3 JSON Output

- **Status:** Optional, deferred
- **Purpose:** CI/CD integration, machine-readable reports
- **Trigger:** When pipeline automation requires structured output

---

## 8. Audit Trail

### Phase 4 Audit Record

| Round | Topic | Outcome |
|-------|-------|---------|
| 1 | Priority order | Locked: Pre-val → Deps → H1-H5 |
| 2 | Function validation scope | Locked: L1 only |
| 3 | AttributeSet resolution | Locked: Option C (default + override) |
| 4 | Blocking policy | Locked: Error blocks, Warning proceeds |
| 5 | Cascade handling | Locked: Separate bucket + E_CASCADE_SKIPPED |
| 6 | Dependency graph | Locked: Extend existing with external edges |

### Phase 4.1.1 Audit Record (2026-01-22)

**Issue Identified:** 77 pre-validation errors for valid engine classes (false negatives)

**Root Cause Analysis (Claude-GPT Dual Audit):**

| Finding | Description | Impact |
|---------|-------------|--------|
| **Bug at line 194** | Copy-paste error: `*CorePath` instead of `*ClassName` | CoreUObject path never resolves correctly |
| **Missing /Script paths** | GameplayTags, Niagara, UMG, AIModule not tried | Valid engine classes reported as not found |

**Affected Classes:**
- `UBlueprintGameplayTagLibrary` → `/Script/GameplayTags.BlueprintGameplayTagLibrary`
- `UNiagaraFunctionLibrary` → `/Script/Niagara.NiagaraFunctionLibrary`
- `UUserWidget` → `/Script/UMG.UserWidget`
- `UBTService_BlueprintBase` → `/Script/AIModule.BTService_BlueprintBase`

**Resolution (Locked Scope):**
1. Fix CoreUObject bug: `*CorePath` → `*ClassName`
2. Add `/Script/GameplayTags.<ClassName>` path attempt
3. Add `/Script/Niagara.<ClassName>` path attempt
4. Add `/Script/UMG.<ClassName>` path attempt
5. Add `/Script/AIModule.<ClassName>` path attempt
6. Add diagnostic logging showing attempted paths on failure

**Explicitly Excluded:** Module pre-loading (not required if /Script paths are correct)

### Phase 4.1.2 Audit Record (2026-01-22)

**Issue Identified:** 3 A1 AttributeSet errors for valid `UNarrativeAttributeSetBase` class

**Root Cause Analysis (Claude-GPT Dual Audit):**

| Finding | Description | Impact |
|---------|-------------|--------|
| **U/A prefix in /Script paths** | `/Script/NarrativeArsenal.UNarrativeAttributeSetBase` fails | Should be `NarrativeAttributeSetBase` (no prefix) |
| **UE5 reflection naming** | C++ uses `UClassName`, /Script paths use `ClassName` | Prefix must be stripped for /Script lookups |

**Proof:** NarrativeAttributeSetBase.h declares `class NARRATIVEARSENAL_API UNarrativeAttributeSetBase` but UE5 stores it at `/Script/NarrativeArsenal.NarrativeAttributeSetBase`.

**Resolution (Locked Scope):**
1. Strip U/A prefix when building /Script module paths
2. Keep original name for "as-is" attempt and LoadClass fallback
3. Safety constraints:
   - Skip if already `/Script/` or `/Game/` qualified
   - Skip if contains `.` (already module-qualified)
   - Skip if name is 1 char long
   - Only strip U and A prefixes (not F, E, etc.)
4. Update diagnostic logging to show both raw and normalized attempts

**Explicitly Excluded:** Stripping other prefixes (F, E, T, etc.)

### Evidence Base

This specification is derived from:
- **Phase 2 Results:** 97 error codes, 55 E_FUNCTION_NOT_FOUND
- **Phase 3 Diagnostics:** ClassPath, SuperClassPath, RequestedProperty
- **Fail_Fast_Audit_FINAL.md:** 118 classified items

---

## 9. Implementation Phases

### Phase 4.1: Pre-Validation System ✓ (v4.24)
- Implement reflection-based checks (F1, F2, A1, A2, C1, C2, R1-R5, T1, T2, K1, K2)
- Implement caching
- Implement blocking logic
- Implement report format

### Phase 4.1.1: Class Resolution Fix ✓ (v4.24.1)
- Fix CoreUObject bug (`*CorePath` → `*ClassName`)
- Add missing /Script paths (GameplayTags, Niagara, UMG, AIModule)
- Add diagnostic logging for failed resolution
- Eliminates 77 false negative errors

### Phase 4.1.2: U/A Prefix Normalization (v4.24.2)
- Strip U/A prefix when building /Script module paths
- UE5 reflection stores classes without prefix in /Script paths
- Safety constraints: skip if qualified, skip if contains `.`, skip if 1 char
- Keep original name for "as-is" and LoadClass fallback
- Fixes 3 remaining A1 AttributeSet false negatives

### Phase 4.2: Dependency Ordering
- Extend dependency graph with external edges
- Implement topological sort for generation
- Implement cascade skip logic
- Implement cascade reporting

### Phase 4.3: 8 BUG Investigation (When Triggered)
- Reproduce BUG failures
- Test H1-H5 hypotheses
- Fix root causes

---

## 10. Success Criteria

Phase 4 is complete when:

1. **Pre-validation catches 100% of resolvable errors before generation**
   - All E_FUNCTION_NOT_FOUND preventable at parse time
   - All E_CLASS_NOT_FOUND preventable at parse time
   - All E_ASSET_NOT_FOUND preventable at parse time

2. **Cascade failures are non-duplicative**
   - Root failure count = actual bugs
   - Cascade count = impact measurement
   - No repeated errors for same root cause

3. **Generation order is deterministic**
   - Same manifest = same order
   - Dependencies always generated before dependents

4. **Reports are actionable**
   - Every error includes manifest location
   - Every error includes resolution attempt details
   - Fix list is complete (not partial)

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.2 | 2026-01-22 | Claude+GPT | Phase 4.1.2 audit findings: U/A prefix normalization scope |
| 1.1 | 2026-01-22 | Claude+GPT | Phase 4.1.1 audit findings: class resolution fix scope |
| 1.0 | 2026-01-22 | Claude+GPT | Initial locked specification |

---

## Related Documents

- `Fail_Fast_Audit_FINAL.md` - Phase 1-3 audit record, 118 items
- `LOCKED_CONTRACTS.md` - Existing locked contracts
- `CLAUDE.md` - Plugin documentation

