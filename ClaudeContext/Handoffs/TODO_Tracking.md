# GasAbilityGenerator TODO Tracking

**Created:** 2026-01-18
**Updated:** 2026-01-21
**Plugin Version:** v4.16
**Status:** Consolidated tracking file for all pending tasks

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

## HIGH Priority - Delegate Binding Automation

**Source:** `Generator_Roadmap_CategoryC_v1_0.md` (archived) - P2.1

### Delegate Binding IR + Codegen

**Goal:** Automate delegate binding for OnAttributeChange, OnDied, OnDamagedBy, etc.

**Current Limitation:** Generator cannot create delegate binding nodes in Blueprint

**Proposed Manifest Syntax:**
```yaml
gameplay_abilities:
  - name: GA_FatherArmor
    delegate_bindings:
      - delegate: OnDied
        source: OwnerASC
        handler: HandleOwnerDied  # Custom Event name
      - delegate: OnAttributeChanged
        source: PlayerASC
        attribute: Health
        handler: HandleHealthChanged
```

**Required Implementation:**
1. Add `FManifestDelegateBinding` struct to `GasAbilityGeneratorTypes.h`
2. Add parser block for `delegate_bindings:` section in `GasAbilityGeneratorParser.cpp`
3. Create Custom Event nodes for handlers in event graph generation
4. Create BindToDelegate / AddDynamic nodes in ActivateAbility event
5. Create Unbind nodes in EndAbility event
6. Handle delegate signature matching (attribute, damage info, etc.)

**Complexity:** HIGH - Requires new node type generation, delegate signature handling

**Value:** HIGH - Would eliminate most manual Blueprint work for reactive abilities

**Files to Modify:**
- `GasAbilityGeneratorTypes.h` - Add FManifestDelegateBinding
- `GasAbilityGeneratorParser.cpp` - Parse delegate_bindings section
- `GasAbilityGeneratorGenerators.cpp` - Generate delegate bind/unbind nodes

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

**Source:** `Generator_Roadmap_CategoryC_v1_0.md` (archived)

| Task | Description | Complexity |
|------|-------------|------------|
| P1.2 Transition Prelude Validation | Detect manifest errors where form abilities don't follow Option B pattern | LOW |
| P1.3 Startup Effects Validation | Detect missing default form state in ability configurations | LOW |

**P1.2 Validation Rules:**
1. Every form ability must have `cancel_abilities_with_tag` for other forms
2. Every form ability must have `activation_required_tags` including `Father.State.Alive`, `Father.State.Recruited`
3. Every form ability must have `activation_blocked_tags` including `Father.State.Dormant`, `Father.State.Transitioning`

**P1.3 Validation Rules:**
1. If `ability_configurations` contains form abilities (GA_Father*), at least one must have `startup_effects` with a GE_*State
2. Warn if no default form state is configured

**Implementation:**
- Add post-parse validation pass in `GasAbilityGeneratorParser.cpp`
- Emit warnings for missing tags (optionally errors in strict mode)
- Add validation in AbilityConfiguration generator

### FormState Preset Schema (From Roadmap P1.1)

**Source:** `Generator_Roadmap_CategoryC_v1_0.md` (archived)

**Goal:** Reduce boilerplate for form state GE definitions

**Proposed Manifest Syntax:**
```yaml
form_state_effects:
  - form: Armor
    invulnerable: true  # Attached form
  - form: Crawler
    invulnerable: false  # Independent form
```

**Generator Expansion:**
```
form_state_effects[i] → GE_{form}State
  - folder: Effects/FormState
  - duration_policy: Infinite
  - granted_tags: [Effect.Father.FormState.{form}]
  - asset_tag: Effect.Father.FormState.{form}
  - if invulnerable: granted_tags.append(State.Invulnerable)
```

**Implementation:**
1. Add `FManifestFormStateEffectDefinition` struct
2. Add parser block for `form_state_effects:`
3. Expand to GE definitions before generation pass

**Complexity:** LOW
**Value:** MEDIUM - Reduces manifest boilerplate

---

## Low Priority (Future)

### Performance

| Task | Description | Complexity |
|------|-------------|------------|
| Test large XLSX files | Test XLSX sync with >1000 rows | LOW |
| Parallelize generators | Parallelize generator execution for performance | HIGH |
| Batch validation | Add batch validation mode for large manifests | MEDIUM |

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

### CSV Parser Phase 2 Features

**Source:** `GasAbilityGeneratorDialogueCSVParser.h:20-21`

| Task | Description | Location |
|------|-------------|----------|
| NC_* Conditions Parsing | Parse dialogue node conditions from CSV | `GasAbilityGeneratorDialogueCSVParser.h:20` |
| NE_* Events Parsing | Parse dialogue node events from CSV | `GasAbilityGeneratorDialogueCSVParser.h:21` |

**Current State:** Fields exist in `FDialogueCSVRow` struct but are not processed

**Implementation:**
1. Parse `Conditions` column for NC_* references
2. Parse `Events` column for NE_* references
3. Convert to dialogue node condition/event arrays
4. Wire up to generated dialogue Blueprint nodes

**Complexity:** MEDIUM
**Value:** LOW - Manual setup works fine for complex conditions/events

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
| Circular dependencies | UNTESTED | May infinite loop | Commandlet deferred handling |

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

## Manual Setup Items (By Design)

These require manual editor configuration - automation is not feasible or not worth the complexity:

### Dialogue System

| Item | Location | Reason |
|------|----------|--------|
| Dialogue conditions/events | `DialogueTableConverter.cpp:40` | Complex nested structures |
| PartySpeakerInfo property | `GasAbilityGeneratorGenerators.cpp:13230` | Property not on base UDialogue |
| DefaultDialogueShot sequences | `GasAbilityGeneratorGenerators.cpp:13086` | Requires manual editor config |

### Quest System

| Item | Location | Reason |
|------|----------|--------|
| Quest dialogue setup (BPT_FinishDialogue) | `GasAbilityGeneratorGenerators.cpp:12692` | Complex task configuration |
| Quest fail setup (FailQuest call) | `GasAbilityGeneratorGenerators.cpp:12698` | Blueprint call required |
| Questgiver linking | `GasAbilityGeneratorGenerators.cpp:19667` | NPC dialogue → Quest pattern |
| XP reward class (NE_GiveXP) | `GasAbilityGeneratorGenerators.cpp:19731` | Class resolution optional |
| Currency reward class (BPE_AddCurrency) | `GasAbilityGeneratorGenerators.cpp:19766` | Class resolution optional |
| Item reward class (BPE_AddItemToInventory) | `GasAbilityGeneratorGenerators.cpp:19831` | Class resolution optional |

### Item/Equipment System

| Item | Location | Reason |
|------|----------|--------|
| MeshMaterials (complex struct) | `GasAbilityGeneratorGenerators.cpp:14498` | Nested array of structs |
| Morphs (complex struct) | `GasAbilityGeneratorGenerators.cpp:14516` | Nested array of structs |
| EquipmentAbilities | `GasAbilityGeneratorGenerators.cpp:14627` | Property not found on base |
| Stats property | `GasAbilityGeneratorGenerators.cpp:14687` | Property not on UNarrativeItem |
| ActivitiesToGrant | `GasAbilityGeneratorGenerators.cpp:14750` | Property not found |
| PickupMeshData | `GasAbilityGeneratorGenerators.cpp:14847` | Property not found |
| TraceData (RangedWeaponItem) | `GasAbilityGeneratorGenerators.cpp:14895` | Class-specific property |

### NPC/Character System

| Item | Location | Reason |
|------|----------|--------|
| NPCTargets array | `GasAbilityGeneratorGenerators.cpp:15856` | Property not found fallback |
| CharacterTargets array | `GasAbilityGeneratorGenerators.cpp:15906` | Property not found fallback |
| NPC Blueprint assignment | `NPCTableValidator.cpp:173` | Requires asset selection |

### VFX/Niagara System

| Item | Location | Reason |
|------|----------|--------|
| GameplayCue requested values | `GasAbilityGeneratorGenerators.cpp:17302` | VFX configuration manual |
| Per-quality scalability | `GasAbilityGeneratorGenerators.cpp:18054` | Requires manual verification |

### AI/Goals System

| Item | Location | Reason |
|------|----------|--------|
| Goal class resolution failure | `GasAbilityGeneratorGenerators.cpp:18995` | Class path not found |

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
