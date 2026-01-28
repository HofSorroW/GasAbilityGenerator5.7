# GasAbilityGenerator TODO Tracking

**Created:** 2026-01-18
**Updated:** 2026-01-28
**Plugin Version:** v7.8.0
**Status:** Consolidated tracking file for all pending tasks

---

## ⚠️ OBSOLETE - Delegate Binding System (v4.21-v4.22) - REMOVED in v7.7.0

> **Track E was removed in v7.7.0.** All delegate binding features documented below were implemented but then removed due to UE5 Blueprint compilation stage ordering bugs with `const FGameplayEffectSpec&` reference parameters.
>
> **Current approach:** Use GAS AbilityTasks (`WaitAttributeChange`, `WaitGameplayEffectApplied`) instead of delegate bindings.
>
> **Reference:** See `Handoffs/Delegate_Binding_History_FINAL.md` for full history.

<details>
<summary>Historical: Delegate Binding Implementation (v4.21-v4.22) - Click to expand</summary>

### v4.21.2 - Pin Wiring Fixes
- UK2Node_Self uses PN_Self, not PN_ReturnValue
- Cast node type propagation via NotifyPinConnectionListChanged

### v4.22 - Extensions
- External ASC Binding (Actor→ASC Resolution)
- Attribute Change Delegates (AbilityTask Pattern)

### v4.21/v4.21.1 - Core Implementation
- 11 delegate bindings across 5 abilities (all removed in v7.7.0)
- Features: OnDied, OnDamagedBy, OnHealedBy, OnDealtDamage delegates
- Two CreateDelegate nodes per binding pattern

</details>

---

## ✅ Recently Completed - ActorComponentBlueprintGenerator (v4.19)

**Source:** `Handoffs/Implementation_Plans_Audit_v1.md` Section 8

**Features:**
| Feature | Description | Status |
|---------|-------------|--------|
| Parent Class Whitelist | ActorComponent, SceneComponent, PrimitiveComponent, AudioComponent | ✅ |
| Variables | Standard Blueprint variable types | ✅ |
| Event Dispatchers | PC_MCDelegate with DelegateSignatureGraph, parameters | ✅ |
| Functions | Input/output pins, pure flag support | ✅ |
| Tick Configuration | bCanEverTick, bStartWithTickEnabled, TickInterval (CDO AFTER compile) | ✅ |
| Reserved Suffix Guard | Blocks _Implementation, _Validate, _C, __ suffixes | ✅ |
| Contract 10 Compliance | FCompilerResultsLog fail-fast on compile errors | ✅ |

**7-Phase Implementation:**
1. Create Blueprint with parent class (whitelist validated)
2. Add Variables via AddMemberVariable
3. Add Event Dispatchers via DelegateSignatureGraph + MarkFunctionEntryAsEditable
4. Add Functions via AddFunctionGraph + CreateUserDefinedPin
5. Compile with FCompilerResultsLog
6. Configure CDO (tick settings) AFTER compile
7. Save with metadata

**Key Technical Details:**
- `AddFunctionGraph` requires `static_cast<UFunction*>(nullptr)` for template deduction
- Pure functions: `AddExtraFlags(FUNC_BlueprintPure)` BEFORE pin creation
- CDO properties set BEFORE compile are LOST (compile recreates class)

**Commit:** `35e477a` feat(v4.19): ActorComponentBlueprintGenerator [LOCKED-CHANGE-APPROVED]

---

## ✅ Recently Completed - Circular Dependency Detection (v4.17)

**Source:** `Handoffs/Implementation_Plans_Audit_v1.md` Section 3

**Dependency Contract v1 Edges:**
| Edge Type | Source Field | Description |
|-----------|--------------|-------------|
| GA → GE | `CooldownGameplayEffectClass` | Cooldown effect reference |
| BT → BB | `BlackboardAsset` | Behavior tree blackboard |
| Any → Parent | `ParentClass` | Only if manifest-defined asset |

**Implementation Details:**
- **Algorithm:** Tarjan's SCC (Strongly Connected Components)
- **Graph Key Format:** `Type:Name` composite key (e.g., `GameplayAbility:GA_FatherAttack`)
- **Cycle Criteria:** `SCC.Num() > 1` OR `(SCC.Num() == 1 && HasSelfEdge)`
- **Status for Cycle Members:** `[FAIL]` with `[E_CIRCULAR_DEPENDENCY]` error code
- **Non-cycle Assets:** Continue generation normally

**Commit:** `fa5fd34` feat(v4.17): Circular Dependency Detection (Dependency Contract v1)

---

## ✅ Recently Completed - Contract 10 Blueprint Compile Gate (v4.16)

**Source:** `Handoffs/Graph_Validation_Audit_v1.md`, `LOCKED_CONTRACTS.md` Contract 10

| Generator | File:Line | Status |
|-----------|-----------|--------|
| FGameplayEffectGenerator | `GasAbilityGeneratorGenerators.cpp:2249` | ✅ FCompilerResultsLog |
| FGameplayAbilityGenerator | `GasAbilityGeneratorGenerators.cpp:2739` | ✅ FCompilerResultsLog |
| FActorBlueprintGenerator | `GasAbilityGeneratorGenerators.cpp:3152` | ✅ FCompilerResultsLog |
| FWidgetBlueprintGenerator | `GasAbilityGeneratorGenerators.cpp:4145` | ✅ FCompilerResultsLog |
| FDialogueBlueprintGenerator | `GasAbilityGeneratorGenerators.cpp:12301` | ✅ FCompilerResultsLog |
| FEquippableItemGenerator | `GasAbilityGeneratorGenerators.cpp:14908` | ✅ FCompilerResultsLog |
| FActivityGenerator | `GasAbilityGeneratorGenerators.cpp:15042` | ✅ FCompilerResultsLog |
| FNarrativeEventGenerator | `GasAbilityGeneratorGenerators.cpp:15687` | ✅ FCompilerResultsLog |
| FNarrativeEventGenerator (final) | `GasAbilityGeneratorGenerators.cpp:16050` | ✅ FCompilerResultsLog |
| FGameplayCueGenerator | `GasAbilityGeneratorGenerators.cpp:16174` | ✅ FCompilerResultsLog |
| FGoalItemGenerator | `GasAbilityGeneratorGenerators.cpp:19186` | ✅ FCompilerResultsLog |
| FQuestGenerator | `GasAbilityGeneratorGenerators.cpp:19838` | ✅ FCompilerResultsLog |

**Contract 10 Invariants (All Satisfied):**
- Every Blueprint generator calls `CompileBlueprint()` with `FCompilerResultsLog`
- If `NumErrors > 0`, asset is NOT saved
- Each Blueprint generator calls `SavePackage` at most once
- `CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE` handled as hard fail

**Archived:** `Graph_Validation_Implementation_v4.16.md` → `Archive/`

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

## ✅ Recently Completed (v4.12.4)

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

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| Hash collision detection | Add detection/warning for hash collisions in metadata system | MEDIUM | ✅ COMPLETE (v4.16.1) |

**Details (COMPLETED v4.16.1):**
- ✅ Added `CheckHashCollision()` and `ClearCollisionMap()` to `UGeneratorMetadataRegistry`
- ✅ Collision map cleared at start of each generation session (commandlet and editor window)
- ✅ Logs `HASH COLLISION DETECTED: Hash N used by both 'path1' and 'path2'` when detected
- ✅ Location: `Locked/GasAbilityGeneratorMetadata.h`, `Locked/GasAbilityGeneratorMetadata.cpp`
- ✅ Additive change only - no guards removed or modified (per Rule #6)

### Case-Duplicate Validation (COMPLETED v4.16.1)

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| Case-duplicate warning | Detect manifest names differing only by case | LOW | ✅ COMPLETE |

**Details:**
- ✅ GPT Audit Concern B (MEDIUM) - case-sensitive hashing could cause user confusion
- ✅ Implemented `ValidateCaseDuplicates()` in parser with global scope
- ✅ Machine-parseable warning format: `[W_CASE_DUPLICATE] 'X' (Type) matches 'Y' (case-insensitive)`
- ✅ Location: `GasAbilityGeneratorParser.cpp` (not locked)
- ✅ Commits: `066cdc9` (validation function), `03b7929` (warning prefix)

### Material Validation

| Task | Description | Complexity |
|------|-------------|------------|
| Circular reference detection | Detect circular expression references in material validation | MEDIUM |

**Details:**
- Current 6-guardrail system doesn't detect circular refs
- Add graph traversal in Pass 2 of validation
- Error code: `E_CIRCULAR_REFERENCE`
- Location: `GasAbilityGeneratorGenerators.cpp` material validation section

### FormState Package Validation (From Roadmap P1)

**Source:** `Generator_Roadmap_CategoryC_v1_0.md` (archived), `Implementation_Plans_Audit_v1.md`

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| P1.2 Transition Validation | Lint form-transition state machine at parse time | LOW | ✅ COMPLETE (v4.18) |
| P1.3 Startup Effects Validation | Detect missing default form state in ability configurations | LOW | ✅ COMPLETE (v4.16.2) |

**P1.2 - COMPLETED (v4.18, commit `8687f88`):**
- Father-specific scope (intentional per Rule #9)
- Tag-based form extraction: `Ability.Father.{Form}` → Form name
- Log-only `[W_TRANSITION_INVALID]` warnings at parse time
- Detected 4 expected warnings for Symbiote's `Symbiote.Charge.Ready` requirement

**P1.3 - COMPLETED (v4.16.2, Claude-GPT dual audit 2026-01-22):**
- Father-specific scope (intentional per Rule #9)
- Error codes: `E_AC_MISSING_FORM_STATE`, `E_AC_STARTUP_EFFECT_NOT_FOUND` (FAIL severity)
- Abort strategy: Collect all errors, skip asset creation, fail run
- Message format: Single-line pipe-delimited
- Locked in `LOCKED_CONTRACTS.md` as P1.3 contract

**Implementation:**
- P1.2: Post-parse `ValidateFormTransitions()` in `GasAbilityGeneratorParser.cpp`
- P1.3: `FAbilityConfigurationGenerator::ValidateStartupEffects()` in `GasAbilityGeneratorGenerators.cpp`

### ✅ FormState Preset Schema (P1.1) - COMPLETE

**Source:** `Generator_Roadmap_CategoryC_v1_0.md` (archived)
**Status:** ✅ IMPLEMENTED (code in v4.13, manifest updated 2026-01-23)

**Goal:** Reduce boilerplate for form state GE definitions

**Manifest Syntax (Now in Use):**
```yaml
form_state_effects:
  - form: Crawler
    invulnerable: false
  - form: Armor
    invulnerable: false    # v4.14.2: Removed per GAS Audit INV-1
  - form: Exoskeleton
    invulnerable: false
  - form: Symbiote
    invulnerable: false
  - form: Engineer
    invulnerable: false
```

**Auto-Expansion:**
Each entry expands to `GE_{Form}State` with:
- `folder: Effects/FormState`
- `duration_policy: Infinite`
- `granted_tags: [Effect.Father.FormState.{Form}]`
- If `invulnerable: true`: adds `Narrative.State.Invulnerable` tag

**Implementation (Already Complete):**
- ✅ `FManifestFormStateEffectDefinition` struct in `GasAbilityGeneratorTypes.h:890`
- ✅ `ParseFormStateEffects()` parser in `GasAbilityGeneratorParser.cpp:11869`
- ✅ Expansion logic in `GasAbilityGeneratorCommandlet.cpp:448` and `GasAbilityGeneratorWindow.cpp:551`
- ✅ Manifest updated to use compact syntax (5 forms, 35 lines → 12 lines)

**Result:** 156/156 assets generated successfully

---

## Low Priority (Future)

### Performance

| Task | Description | Complexity | Status |
|------|-------------|------------|--------|
| Test large XLSX files | Test XLSX sync with >1000 rows | LOW | |
| Parallelize generators | Parallelize generator execution for performance | HIGH | |
| ~~Batch validation~~ | ~~Add batch validation mode for large manifests~~ | ~~MEDIUM~~ | ✅ COMPLETE (v4.24) |

**Batch Validation - Audit Closure (2026-01-23):**
- **Implementation:** PreValidator system + `-dryrun` flag
- **Audit:** Claude-GPT dual audit confirmed feature fully implemented
- **Evidence:** PreValidator validates entire manifest before generation; `-dryrun` provides CI-safe execution with no side effects
- **Original spec:** "for large manifests" - satisfied by batch error collection and aggregated reporting

### Save System Integration (From Roadmap P2.2)

**Source:** `Generator_Roadmap_CategoryC_v1_0.md` (archived)

**Goal:** Support attribute/state restoration on game load

**Conditional:** Only needed if Narrative Pro's `NarrativeSavableComponent` doesn't persist ASC state

**Proposed Manifest Syntax:**
```yaml
gameplay_abilities:
  - name: GA_FatherArmor
    on_load:
      nodes:
        - id: RestoreFormState
          type: CallFunction
          properties:
            function: BP_ApplyGameplayEffectToOwner
            target_self: true
```

**Implementation:**
1. Add `on_load:` event_graph section (separate from main graph)
2. Hook into NarrativeSavableComponent.Load_Implementation
3. Research NP save system persistence behavior first

**Complexity:** MEDIUM
**Status:** RESEARCH NEEDED - Verify NP save system behavior

### VFX Automation (From Roadmap P3)

**Source:** `Generator_Roadmap_CategoryC_v1_0.md` (archived)

| Task | Description | Complexity |
|------|-------------|------------|
| P3.1 Niagara Spawning Support | Generate SpawnSystemAttached nodes with lifecycle management | MEDIUM |
| P3.2 GameplayCue Auto-Wiring | Generate GameplayCue assets and wire ExecuteGameplayCue nodes | MEDIUM |

**P3.1 Proposed Syntax:**
```yaml
vfx_spawns:
  - id: FormTransitionVFX
    niagara_system: NS_FormTransition
    attach_to: Owner
    socket: spine_01
```

**P3.2 Proposed Syntax:**
```yaml
gameplay_cues:
  - name: GC_FatherFormTransition
    tag: GameplayCue.Father.FormTransition
    parent_class: GameplayCueNotify_Burst

gameplay_abilities:
  - name: GA_FatherArmor
    cues:
      - trigger: OnActivate
        cue_tag: GameplayCue.Father.FormTransition
```

**Note:** GameplayCues can be created manually with low friction. These are nice-to-have.

---

## ✅ Resolved - Node Position Persistence (v4.20.x → v4.22)

**Status:** ✅ FIXED AND VERIFIED (2026-01-22)

**Problem:**
Event graph nodes in generated Blueprints cluster on the left side (X=0) instead of being spread out horizontally according to the layered graph layout algorithm.

**Attempted Fixes (v4.20 - v4.20.11):**

| Version | Approach | Result |
|---------|----------|--------|
| v4.20.9 | Pin-level positioning ("spider legs") - position based on connecting pin Y offset | No change |
| v4.20.10 | Wave-based processing with PIN_SPREAD_MULTIPLIER | No change |
| v4.20.10 | Added `Modify()` calls on graph and nodes before setting positions | No change |
| v4.20.11 | Added `NotifyGraphChanged()` and `MarkBlueprintAsModified()` | No change |
| v4.20.11 | Static position storage + `ReapplyNodePositions()` after `CompileBlueprint` | No change |

### **Claude-GPT Dual Audit (2026-01-22)**

**Phase 1: Diagnostic Logging (5-point pipeline trace)**

| Logging Point | Position Status | Node Ptrs | Graph Ptr |
|---------------|-----------------|-----------|-----------|
| POINT_1: AfterAutoLayout | Values set | Stable | Stable |
| POINT_2: BeforeCompile | Same values | Stable | Stable |
| POINT_3: AfterCompile | Same values | Stable | Stable |
| POINT_4: AfterReapply | Same values | Stable | Stable |
| POINT_5: BeforeSave | Same values | Stable | Stable |

**Finding:** Positions ARE being set and preserved through save. The generator is not at fault.

**Phase 2: Visual Inspection (Editor screenshots)**

| Blueprint | Compile Status | Node Positions |
|-----------|---------------|----------------|
| GA_FatherEngineer | ✅ Compiles | Clustered LEFT (X=0 area) |
| GA_FatherCrawler | ❌ Errors | Clustered LEFT (X=0 area) |

**Finding:** Both clean and failing BPs show left clustering. Issue is NOT related to compile errors.

**Phase 3: Clipboard Export Analysis (Erdem's evidence)**

| Node | Before Nudge | After Nudge |
|------|--------------|-------------|
| K2_ActivateAbility | No NodePosX, No NodePosY | NodePosX=-160, NodePosY=16 ✅ |
| K2_OnEndAbility | NodePosY=336 only, No NodePosX | NodePosX=32, NodePosY=336 ✅ |
| Branch nodes | NodePosX/Y always present | N/A |

**Finding:** Override event nodes with NodePosX=0 don't serialize the X position (UE default value omission).

### **ROOT CAUSE CONFIRMED**

> **Override event nodes (UK2Node_Event with bOverrideFunction=True) have NodePosX=0, which UE treats as default and omits from serialization.**

- NodePosX=0 is the default value → UE doesn't serialize it → position lost on reload
- NodePosY works when non-zero (OnEndAbility Y=704 serializes correctly)
- Manual nudge makes NodePosX non-zero → then it serializes and persists
- Non-event nodes (Branch, CallFunction, etc.) have non-zero X → serialize correctly

**Evidence Chain:**
1. Diagnostic logs: Event ActivateAbility at X=0, Y=0
2. Diagnostic logs: Event OnEndAbility at X=0, Y=704
3. Clipboard: Untouched events missing NodePosX line
4. Clipboard: Nudged events have NodePosX line and persist

### **APPROVED FIX SPECIFICATION**

**Location:** Post-layout finalization, after any refresh/reconstruct, before `SavePackage`

**Logic:**
```cpp
For each node in EventGraph(s):
    If node is UK2Node_Event AND bOverrideFunction == true:
        If NodePosX == 0:
            NodePosX = 16  // One grid unit, ensures serialization
        // Do NOT modify NodePosY (works when non-zero)
        // Do NOT clamp negative NodePosX (valid authored position)
```

**Constraints:**
- Do NOT change layout algorithm or spacing rules
- Do NOT add editor hooks
- Do NOT touch NodePosY (proven to work)
- Do NOT clamp negative X values (valid from manual nudge)
- Fix must be generation-time and deterministic

**Acceptance Test:**
1. Generate BP via headless commandlet
2. Open in editor WITHOUT touching nodes
3. Copy override event nodes (K2_ActivateAbility, K2_OnEndAbility)
4. **PASS:** Clipboard contains `NodePosX=<non-zero>`
5. **FAIL:** Clipboard missing `NodePosX` line

**Files Modified:**
- `GasAbilityGeneratorGenerators.cpp:2834-2852` - Added post-layout clamp for override events

**Implementation Status:** ✅ IMPLEMENTED AND VERIFIED

**Verification (2026-01-22):**
- Clipboard test on GA_FatherEngineer: `NodePosX=16` present ✅
- All 21 generated abilities show `[POSITION_FIX]` log entries ✅
- Override events now serialize with non-default X position ✅

---

## Known Edge Cases (Untested/Unsupported)

### XLSX Sync System

| Area | Status | Notes | Location |
|------|--------|-------|----------|
| Simultaneous external + YAML changes | UNTESTED | May cause sync conflicts | All XLSX engines |
| Cell merge handling | UNSUPPORTED | Document as limitation | OpenXLSX limitation |
| Large files (>1000 rows) | UNTESTED | Performance unknown | All XLSX readers |

### Validation Gaps

| Area | Status | Notes | Location |
|------|--------|-------|----------|
| Material: Circular expression refs | UNDETECTED | See Medium Priority task | `GasAbilityGeneratorGenerators.cpp` |
| Metadata: Hash collisions | ✅ DETECTED (v4.16.1) | Logs warning when collision found | `Locked/GasAbilityGeneratorMetadata.cpp` |

### Deferred Asset Resolution

| Area | Status | Notes | Location |
|------|--------|-------|----------|
| Missing dependency retry | IMPLEMENTED | Returns Deferred to retry later | `GasAbilityGeneratorGenerators.cpp:2650` |
| Circular dependencies | ✅ DETECTED (v4.17) | Tarjan SCC algorithm, [E_CIRCULAR_DEPENDENCY] | `GasAbilityGeneratorCommandlet.cpp:30-220` |

---

## Documented Limitations (Not Supported)

These are intentional limitations documented in code:

### Property/Type Limitations

| Limitation | Location | Reason |
|------------|----------|--------|
| Nested dots in property paths | `GasAbilityGeneratorGenerators.cpp:3667` | Parser complexity |
| MediaTexture type | `GasAbilityGeneratorGenerators.cpp:3724` | No MediaAssets dependency |
| Nested struct types | `GasAbilityGeneratorGenerators.cpp:3881` | Serialization complexity |
| Root-level struct types | `GasAbilityGeneratorGenerators.cpp:3992` | Same as above |
| UMaterialFunctionInstance | `GasAbilityGeneratorGenerators.cpp:6401` | Use UMaterialFunction instead |

### Unsupported Node Types

| Type | Location | Alternative |
|------|----------|-------------|
| Unknown event graph node types | `GasAbilityGeneratorGenerators.cpp:8200` | Use supported types |
| Unknown function override types | `GasAbilityGeneratorGenerators.cpp:9081` | Use supported types |
| Unknown custom function types | `GasAbilityGeneratorGenerators.cpp:9320` | Use supported types |

### Token Registry Limitations

| Limitation | Location | Fallback |
|------------|----------|----------|
| Unknown event types | `DialogueTokenRegistry.cpp:1178-1179` | Returns `UNSUPPORTED(ClassName)` |
| Unsupported property types | `DialogueTokenRegistry.cpp:1534` | Error message returned |

---

## ✅ Automation Gap Closure (v7.8.1 COMPLETE)

**Updated:** 2026-01-28
**Status:** Audit revealed most features were already implemented in v4.30+. Only NPC auto-link added.

### Audit Summary

| Item | Listed Status | Actual Status | Action |
|------|---------------|---------------|--------|
| EquipmentAbilities | Needs deferred pass | ✅ **IMPLEMENTED** (v4.30, line 19873-19959) | Session cache + defer working |
| ActivitiesToGrant | Needs deferred pass | ✅ **IMPLEMENTED** (v4.30) | Session cache + defer working |
| MeshMaterials | Needs tag validation | ✅ **IMPLEMENTED** (v4.30, lines 19484-19634) | FCreatorMeshMaterial fully automated |
| Morphs | Needs mesh validation | ✅ **IMPLEMENTED** (v4.30, lines 19636-19699) | FCreatorMeshMorph fully automated |
| Stats property | Breaking change needed | ✅ **IMPLEMENTED** (v4.8, lines 19842-19904) | FNarrativeItemStat struct populated |
| Dialogue conditions/events | Phase 2 placeholder | ✅ **IMPLEMENTED** | CreateDialogueConditionFromDefinition/EventFromDefinition |
| DefaultDialogueShot | Array not populated | ✅ **IMPLEMENTED** (v4.30, lines 17897-18027) | SequenceAssets array automated |
| BPT_FinishDialogue tasks | Complex lifecycle | ✅ **IMPLEMENTED** (lines 25567-25603) | Quest task creation automated |
| Quest fail setup | No NE_FailQuest | ❌ **CANNOT AUTOMATE** | UQuest has no FailQuest property |
| Questgiver linking | Implicit relationship | ❌ **CANNOT AUTOMATE** | UQuest has no Questgiver property |
| NPC Blueprint auto-link | No auto-assignment | ✅ **IMPLEMENTED** (v7.8.1) | Convention-based lookup added |

### v7.8.1 Implementation: NPC Blueprint Auto-Link

When `npc_blueprint:` is not specified in manifest, the generator now auto-discovers Blueprint by convention:

**Convention Paths (tried in order):**
1. `{ProjectRoot}/NPCs/BP_{NPCBaseName}.BP_{NPCBaseName}`
2. `{ProjectRoot}/NPCs/BP_NPC_{NPCBaseName}.BP_NPC_{NPCBaseName}`
3. `{ProjectRoot}/Characters/BP_{NPCBaseName}.BP_{NPCBaseName}`
4. `{ProjectRoot}/Characters/NPCs/BP_{NPCBaseName}.BP_{NPCBaseName}`

**NPCBaseName** is derived by stripping `NPC_` prefix (e.g., `NPC_Blacksmith` → `Blacksmith`).

**Location:** `GasAbilityGeneratorGenerators.cpp:22815-22842`

### Items That Cannot Be Automated (By Design)

| Item | Reason |
|------|--------|
| Quest fail setup | `UQuest` has no `FailQuest` property - fail states are handled via state machine |
| Questgiver linking | `UQuest` has no `Questgiver` property - questgiver is the NPC whose dialogue starts the quest |

**Note:** Quest fail and questgiver are implicit relationships, not UQuest properties. The `questgiver:` manifest field is for documentation only.

### Already Automated (Reference)

| Item | Generator Location | Status |
|------|-------------------|--------|
| Quest XP (NE_GiveXP) | Lines 22098-22130 | ✅ Complete (4 NP paths) |
| Quest Currency (BPE_AddCurrency) | Lines 22133-22165 | ✅ Complete |
| Quest Items (BPE_AddItemToInventory) | Lines 22168-22231 | ✅ Complete |
| NPCTargets | Lines 18182-18256 | ✅ Complete (FSoftObjectProperty + FObjectProperty) |
| CharacterTargets | Lines 18259-18307 | ✅ Complete (FSoftObjectProperty) |
| Goal class resolution | Lines 21350-21395 | ✅ Complete (11 search paths) |
| PickupMeshData | Lines 17191-17252 | ✅ Complete |
| TraceData | Lines 17254-17298 | ✅ Complete |

### VFX/Niagara (Manual by Design)

| Item | Location | Reason |
|------|----------|--------|
| GameplayCue requested values | `GasAbilityGeneratorGenerators.cpp:17302` | VFX configuration manual |
| Per-quality scalability | `GasAbilityGeneratorGenerators.cpp:18054` | Requires manual verification |

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
| 2026-01-21 | **v4.16 Consolidation:** Merged Generator_Roadmap_CategoryC_v1_0.md (P1/P2/P3 features) |
| 2026-01-21 | **v4.16 Consolidation:** Added Contract 10 completion record (12 BP generators with FCompilerResultsLog) |
| 2026-01-21 | **v4.16 Consolidation:** Added code-level findings: Manual Setup Items, Documented Limitations, Edge Cases |
| 2026-01-21 | **v4.16 Consolidation:** Archived Graph_Validation_Implementation_v4.16.md (work complete) |
| 2026-01-21 | **v4.16 Consolidation:** Archived Generator_Roadmap_CategoryC_v1_0.md (merged into this file) |
| 2026-01-21 | **v4.16.1:** Hash collision detection complete - `CheckHashCollision()` and `ClearCollisionMap()` added to metadata registry |
| 2026-01-21 | **v4.16.1:** Case-duplicate validation warning (Concern B) - `ValidateCaseDuplicates()` warns on case-only name differences |
| 2026-01-21 | **v4.17:** Circular Dependency Detection complete - Tarjan SCC algorithm detects GA→GE, BT→BB, Any→Parent cycles |
| 2026-01-21 | **v4.18:** P1.2 Form Transition Validation complete - Tag-based form extraction, `[W_TRANSITION_INVALID]` warnings |
| 2026-01-21 | **v4.19:** ActorComponentBlueprintGenerator complete - Variables, Event Dispatchers, Functions, Tick config |
| 2026-01-21 | **v4.20.11:** Node position persistence issue - attempted fixes did not resolve (see Known Issues) |
| 2026-01-21 | **v4.21.1:** Removed stale "HIGH Priority - Delegate Binding Automation" section - feature completed in v4.21/v4.21.1 |
| 2026-01-21 | **v4.21.1:** Manifest updated with 11 delegate bindings across 5 abilities (all 4 delegate types covered) |
| 2026-01-21 | **v4.21.1:** Added: GA_FatherCrawler (2), GA_StealthField (2) - based on Technical Reference §4.5, §38.8 guidance |
| 2026-01-22 | **v4.22:** Section 10 External ASC Binding - Actor→ASC extraction via `GetAbilitySystemComponent` |
| 2026-01-22 | **v4.22:** Section 11 Attribute Change Delegates - AbilityTask pattern for `UAbilityTask_WaitAttributeChange` |
| 2026-01-22 | **v4.22:** Claude-GPT dual audit approved - `Delegate_Binding_Extensions_Spec_v1_1.md` locked |
| 2026-01-22 | **v4.21.2:** Delegate binding pin wiring fixes - UK2Node_Self uses PN_Self, CastNode needs NotifyPinConnectionListChanged |
| 2026-01-22 | **v4.21.2:** All 5 abilities with delegate_bindings now compile (GA_FatherCrawler, GA_FatherArmor, GA_FatherSymbiote, GA_ProtectiveDome, GA_StealthField) |
| 2026-01-22 | **v4.16.2:** P1.3 Startup Effects Validation - Claude-GPT dual audit LOCKED, error codes E_AC_MISSING_FORM_STATE/E_AC_STARTUP_EFFECT_NOT_FOUND |
| 2026-01-22 | **v4.16.2:** Search path fix - Added `Effects/FormState/` and `Effects/Cooldowns/` to startup effect search paths |
| 2026-01-22 | **Fail-Fast Audit Phase 1:** Line number verification complete - 87→114 items (+27 new findings) |
| 2026-01-22 | **Fail-Fast Audit Phase 1:** Created `Fail_Fast_Audit_v2.md` with verified line numbers and new patterns |
| 2026-01-22 | **Fail-Fast Audit:** Claude-GPT dual audit COMPLETE - consensus reached on R1/R2/R3 framework |
| 2026-01-22 | **Fail-Fast Audit:** Scope decision: Option 1 (generation + sync pipelines only) |
| 2026-01-22 | **Fail-Fast Audit:** 8 generator bugs PROVEN (properties exist in NP, FindPropertyByName fails) |
| 2026-01-22 | **Fail-Fast Audit:** Created `Fail_Fast_Claude_GPT_Audit_v1.md` - locked decisions and framework |
| 2026-01-22 | **Fail-Fast Audit v2.1:** Created anchored classification document - 118 items, 111 Type M for Phase 2 |
| 2026-01-22 | **Fail-Fast Phase 2 START:** Converting 111 Type M Pipeline items to hard fails |
| 2026-01-23 | **P1.1 FormState Preset Schema:** Manifest updated to use compact `form_state_effects:` syntax (35→12 lines) |
| 2026-01-23 | **Batch Validation Audit:** Claude-GPT dual audit confirmed FULLY IMPLEMENTED (v4.24 PreValidator + `-dryrun`) |
| 2026-01-28 | **Consolidation:** Merged `Automation_Gap_Closure_Spec_v1.md` into "Automation Gap Closure" section |
| 2026-01-28 | **Cleanup:** Marked Delegate Binding sections (v4.21-v4.22) as OBSOLETE - Track E removed in v7.7.0 |
| 2026-01-28 | **v7.8.1 Automation Gap Audit:** Found most items already implemented (v4.30+). Added NPC Blueprint auto-link by convention. |
