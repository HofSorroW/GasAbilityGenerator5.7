# GasAbilityGenerator TODO Tracking

**Created:** 2026-01-18
**Updated:** 2026-01-22
**Plugin Version:** v4.22
**Status:** Consolidated tracking file for all pending tasks

---

## ✅ Recently Completed - Delegate Binding Extensions (v4.22)

**Source:** `Handoffs/Delegate_Binding_Extensions_Spec_v1_1.md`
**Audit:** Claude-GPT dual audit PASSED (2026-01-22)

### Section 10: External ASC Binding (Actor→ASC Resolution)

| Feature | Description | Status |
|---------|-------------|--------|
| Actor Type Detection | Detect source variable type via `PinType.PinSubCategoryObject` | ✅ |
| ASC Extraction | Insert `GetAbilitySystemComponent` node for Actor sources | ✅ |
| NASC Cast | Auto-cast to `UNarrativeAbilitySystemComponent` after extraction | ✅ |
| Source Type Validation | Error if explicit `source_type` conflicts with actual type | ✅ |

**Node Generation Order:**
```
VariableGet(ActorVar) → GetAbilitySystemComponent(Actor) → Cast to NASC → AddDelegate
```

### Section 11: Attribute Change Delegates (AbilityTask Pattern)

| Feature | Description | Status |
|---------|-------------|--------|
| AbilityTask Creation | `UK2Node_LatentAbilityCall` for `UAbilityTask_WaitAttributeChange` | ✅ |
| Zero-Param Handler | CustomEvent with no parameters per `FWaitAttributeChangeDelegate` | ✅ |
| Attribute Resolution | `AttributeSet` + `Attribute` → `FGameplayAttribute` | ✅ |
| Automatic Lifecycle | Task manages unbind via `OnDestroy()` - no EndAbility nodes needed | ✅ |
| TriggerOnce Support | Default `true` per factory signature | ✅ |
| Tag Filters | Optional `with_tag` and `without_tag` support | ✅ |

**Error Codes:**
| Code | Severity | Description |
|------|----------|-------------|
| `E_ATTRIBUTE_SET_NOT_FOUND` | FAIL | AttributeSet class not found |
| `E_ATTRIBUTE_NOT_FOUND` | FAIL | Attribute property not found on AttributeSet |
| `E_TASK_FACTORY_REQUIRED` | FAIL | AbilityTask created outside official factory |

**Key Technical Detail:**
- `FDelegateHandle` is NOT Blueprint-serializable - AbilityTask pattern required
- `FWaitAttributeChangeDelegate` is zero-param: `DECLARE_DYNAMIC_MULTICAST_DELEGATE()`

**Files Modified:**
- `GasAbilityGeneratorTypes.h` - Added `FManifestAttributeBindingDefinition` struct
- `GasAbilityGeneratorParser.cpp` - Added `attribute_bindings:` section parsing
- `GasAbilityGeneratorGenerators.cpp` - Added `GenerateAttributeBindingNodes()` function
- `GasAbilityGeneratorGenerators.h` - Added function declaration

---

## ✅ Recently Completed - Delegate Binding Variable Source (v4.21.1)

**Source:** `Handoffs/Implementation_Plans_Audit_v1.md` Section 9 (P2.1) - Test Case #5
**Acceptance Criteria:** "Source resolution works for OwnerASC, PlayerASC, and variables"

**Implementation:**
- Creates UK2Node_VariableGet for Blueprint variable source
- Searches Blueprint.NewVariables for source name
- Auto-casts to UNarrativeAbilitySystemComponent if variable type is base ASC
- Error codes: E_DELEGATE_SOURCE_INVALID (not found), E_DELEGATE_VARIABLE_PIN (pin error)

**Completes v4.21 locked design - all 10 test cases now covered.**

---

## ✅ Recently Completed - Delegate Binding Automation (v4.21)

**Source:** `Handoffs/Implementation_Plans_Audit_v1.md` Section 9 (P2.1)
**Audit:** Claude-GPT dual audit PASSED (2026-01-21)

**Features:**
| Feature | Description | Status |
|---------|-------------|--------|
| Narrative Pro Delegates | OnDied, OnDamagedBy, OnHealedBy, OnDealtDamage | ✅ |
| Two CreateDelegate Nodes | Separate nodes for Activate and End paths | ✅ |
| PN_Self Wiring | CreateDelegate→Self, Add/RemoveDelegate→SourceASC | ✅ |
| Cast to NarrativeASC | Mandatory for OwnerASC/PlayerASC keywords | ✅ |
| Custom Event Creation | Auto-created with delegate signature parameters | ✅ |
| Parser Id Field | `id:` field for delegate binding identification | ✅ |
| Error Codes | E_DELEGATE_NOT_FOUND, E_DELEGATE_SIGNATURE_MISMATCH (FAIL) | ✅ |
| Variable Source Resolution | UK2Node_VariableGet for custom variables | ✅ (v4.21.1) |

**Architecture Decision (Audit-Approved):**
- Two CreateDelegate nodes per binding (not shared across exec paths)
- RemoveDelegate unbinds by (Object, FunctionName) tuple match
- No guards modified - replaced incomplete stub with full implementation

**Commit:** `4fcab76` feat(v4.21): Delegate Binding Automation [LOCKED-CHANGE-APPROVED]

**Manifest Usage (v4.21.1) - 11 Delegate Bindings:**
| Ability | Delegate | Source | Handler | Use Case |
|---------|----------|--------|---------|----------|
| GA_FatherCrawler | OnDamagedBy | OwnerASC | HandleCrawlerDamageTaken | Damage tracking/UI |
| GA_FatherCrawler | OnDealtDamage | OwnerASC | HandleCrawlerDamageDealt | Ultimate charge |
| GA_FatherArmor | OnDamagedBy | OwnerASC | HandleArmorDamageReceived | Damage feedback |
| GA_FatherArmor | OnDied | FatherASC (var) | HandleFatherDied | Death cleanup |
| GA_FatherSymbiote | OnDealtDamage | OwnerASC | HandleSymbioteDamageDealt | Proximity damage |
| GA_FatherSymbiote | OnHealedBy | OwnerASC | HandleSymbioteHealing | Regen tracking |
| GA_ProtectiveDome | OnDamagedBy | OwnerASC | HandleDomeDamageAbsorption | Dome absorption |
| GA_StealthField | OnDamagedBy | OwnerASC | HandleStealthDamageBreak | Break on hit |
| GA_StealthField | OnDealtDamage | OwnerASC | HandleStealthAttackBreak | Break on attack |

**Delegate Coverage:**
- All 4 Narrative Pro delegates used: OnDied, OnDamagedBy, OnHealedBy, OnDealtDamage
- OwnerASC keyword: 10 bindings
- Variable source: 1 binding (FatherASC)
- Player-owned abilities: GA_ProtectiveDome, GA_StealthField (3 bindings)
- Father-owned abilities: GA_FatherCrawler, GA_FatherArmor, GA_FatherSymbiote (8 bindings)

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
| P1.3 Startup Effects Validation | Detect missing default form state in ability configurations | LOW | Pending audit |

**P1.2 - COMPLETED (v4.18, commit `8687f88`):**
- Father-specific scope (intentional per Rule #9)
- Tag-based form extraction: `Ability.Father.{Form}` → Form name
- Log-only `[W_TRANSITION_INVALID]` warnings at parse time
- Detected 4 expected warnings for Symbiote's `Symbiote.Charge.Ready` requirement

**P1.3 Validation Rules:**
1. If `ability_configurations` contains form abilities (GA_Father*), at least one must have `startup_effects` with a GE_*State
2. Warn if no default form state is configured

**Implementation:**
- P1.2: Post-parse `ValidateFormTransitions()` in `GasAbilityGeneratorParser.cpp`
- P1.3: Add validation in AbilityConfiguration generator (pending audit)

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

---

## 🔧 Known Issues - Node Position Persistence (v4.20.x)

**Status:** ROOT CAUSE IDENTIFIED - FIX APPROVED (2026-01-22)

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

**Files to Modify:**
- `GasAbilityGeneratorGenerators.cpp` - Add post-layout clamp for override events

**Implementation Status:** AWAITING APPROVAL

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
