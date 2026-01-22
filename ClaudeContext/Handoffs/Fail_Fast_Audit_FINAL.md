# Fail-Fast Audit - Final Reference Document

**Version:** 1.0 (Consolidated)
**Date:** 2026-01-22
**Status:** COMPLETE (v4.23.1)
**Auditors:** Claude (Opus 4.5), GPT
**Scope:** Generation + Sync Pipelines Only

---

## Executive Summary

The Claude-GPT dual audit achieved full consensus on the fail-fast system design. All phases completed successfully.

**Key Outcomes:**
- 118 silent failure points identified and classified
- 111 Type M (Masking) items converted to HARD FAIL in Phase 2
- 8 generator bugs proven (properties exist in NP but FindPropertyByName fails)
- Phase 3 diagnostic output implemented with full context
- R1/R2/R3 classification framework locked

---

## Phase Completion Status

| Phase | Goal | Status | Version |
|-------|------|--------|---------|
| Phase 1 | Inventory + document all silent failures | COMPLETE | v4.22 |
| Phase 2 | Remove fallbacks so assets truly fail | COMPLETE | v4.23 |
| Phase 3 | Report failing assets with diagnostics | COMPLETE | v4.23.1 |
| Phase 4 | Build fail-proof system based on reasons | PENDING | - |

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
- Editor UX fallbacks remain intact (6 items, Type D)
- Pipeline correctness is enforced (112 items)
- Each item tagged with `Scope=Pipeline` or `Scope=EditorUX`

### 3. R1/R2/R3 Classification Framework (LOCKED)

| Class | Condition | Action |
|-------|-----------|--------|
| **R1** | Manifest explicitly references something -> cannot resolve | **HARD FAIL** |
| **R2** | Manifest omits optional field -> documented default | ALLOWED (no warn) |
| **R3** | Structural invalidity (quests, graphs, etc.) | **HARD FAIL** |

**Key Separator:** "Manifest referenced?" YES/NO

### 4. Fallback Classification (LOCKED)

| Type | Description | Action |
|------|-------------|--------|
| **Type M (Masking)** | Manifest referenced X -> X missing -> generation continued | **ELIMINATE** |
| **Type D (Default)** | Manifest omitted optional X -> uses default | ALLOWED if documented |

### 5. Phase 3 Failure Report Format (LOCKED)

Every hard fail produces structured diagnostic output:

```
[E_ERROR_CODE] AssetName | Subsystem: X | Human message | ClassPath: /Script/Module.UClass | SuperClassPath: /Script/Module.UParent | RequestedProperty: PropertyName
```

**Required Fields for 8 BUG paths:**
- `ClassPath` - `GetClass()->GetPathName()`
- `SuperClassPath` - `GetClass()->GetSuperClass()->GetPathName()`
- `RequestedProperty` - The property name that failed lookup

**Subsystems:** GAS, Item, NPC, Dialogue, Quest, BT, Material, Niagara, Token, General

This enables sorting failures into:
- Bad input data (Excel/YAML)
- Missing dependencies (assets/tags)
- Generator logic bugs

---

## Phase 2 Execution Results

### Summary Statistics
- **Total Failed Assets:** 16
- **Total Error Codes Triggered:** 97

### Error Code Breakdown

| Count | Error Code | Description |
|-------|------------|-------------|
| 55 | E_FUNCTION_NOT_FOUND | Blueprint function not found |
| 30 | E_PIN_POSITION_NOT_FOUND | Layout helper (logged only) |
| 6 | E_MONTAGE_SKELETON_NOT_FOUND | Animation skeleton missing |
| 1 | E_MF_CONNECTION_TARGET_NOT_FOUND | Material function node missing |
| 1 | E_GE_PROPERTY_NOT_FOUND | GE attribute not found |
| 1 | E_GA_CUSTOMFUNCTION_FAILED | Custom function failed |
| 1 | E_COOLDOWNGE_NOT_FOUND | Cooldown GE invalid |
| 1 | E_ABILITYCONFIG_ABILITY_NOT_FOUND | AbilityConfig ability missing |
| 1 | E_ACTIVITYCONFIG_ACTIVITY_NOT_FOUND | ActivityConfig activity missing |

### Failed Assets List

1. GE_BackstabBonus - E_GE_PROPERTY_NOT_FOUND (AttackSpeed)
2. BP_FatherCompanion - E_FUNCTION_NOT_FOUND (SpawnSystemAttached)
3. GA_FatherCrawler - E_FUNCTION_NOT_FOUND (multiple)
4. GA_FatherArmor - E_FUNCTION_NOT_FOUND (multiple)
5. GA_FatherExoskeleton - E_FUNCTION_NOT_FOUND (multiple)
6. GA_FatherSymbiote - E_FUNCTION_NOT_FOUND (multiple)
7. GA_FatherEngineer - E_FUNCTION_NOT_FOUND (multiple)
8. GA_FatherExoskeletonSprint - E_FUNCTION_NOT_FOUND
9. GA_FatherSacrifice - E_FUNCTION_NOT_FOUND
10. GA_Backstab - E_GA_CUSTOMFUNCTION_FAILED
11. AM_FatherAttack - E_MONTAGE_SKELETON_NOT_FOUND
12. AM_FatherThrowWeb - E_MONTAGE_SKELETON_NOT_FOUND
13. AM_FatherSacrifice - E_MONTAGE_SKELETON_NOT_FOUND
14. AM_FatherReactivate - E_MONTAGE_SKELETON_NOT_FOUND
15. AM_FatherDetach - E_MONTAGE_SKELETON_NOT_FOUND
16. AM_FatherAttach - E_MONTAGE_SKELETON_NOT_FOUND
17. MF_RadialGradient - E_MF_CONNECTION_TARGET_NOT_FOUND
18. AC_FatherCompanion - E_ABILITYCONFIG_ABILITY_NOT_FOUND (cascading)
19. AC_FatherBehavior - E_ACTIVITYCONFIG_ACTIVITY_NOT_FOUND (cascading)

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

### Root Cause Hypotheses (Unproven - Phase 4 Will Investigate)

| Hypothesis | Description |
|------------|-------------|
| H1 | Wrong UClass (fallback class, wrong parent) |
| H2 | Module/load/reflection timing |
| H3 | Name string mismatch vs reflected FName |
| H4 | `#if WITH_EDITORONLY_DATA` guards |
| H5 | Blueprint-generated class layout differs |

### Audit Consensus

- **"Property exists"** - PROVEN
- **"Generator bug"** - AGREED
- **"Root cause"** - UNPROVEN (deferred to Phase 4)

---

## Full Classification (118 Items)

### Column Definitions

| Column | Description |
|--------|-------------|
| **#** | Item identifier |
| **Anchor** | `FunctionName \| "unique log substring"` - stable identifier |
| **Subsystem** | GAS / NPC / Dialogue / Quest / BT / Material / Niagara / Token / Item / General |
| **Type** | **M** = Masking (HARD FAIL), **D** = Default (Allowed if documented) |
| **Scope** | **Pipeline** = Enforced, **EditorUX** = Not enforced |

---

### Category A: [INFO] Manual Setup Fallbacks (12 items)

| # | Anchor | Subsystem | Type | Scope |
|---|--------|-----------|------|-------|
| A1 | `FDialogueBlueprintGenerator::Generate \| "PartySpeakerInfo property not found"` | Dialogue | M | Pipeline |
| A2 | `FEquippableItemGenerator::Generate \| "MeshMaterials - complex struct, logging for manual"` | Item | M | Pipeline |
| A3 | `FEquippableItemGenerator::Generate \| "Morphs - complex struct, logging for manual"` | Item | M | Pipeline |
| A4 | `FEquippableItemGenerator::Generate \| "Logging values for manual setup"` | Item | M | Pipeline |
| A5 | `FEquippableItemGenerator::Generate \| "EquipmentAbilities property not found"` | Item | M | Pipeline |
| A6 | `FEquippableItemGenerator::Generate \| "Stats property not found on UNarrativeItem"` | Item | M | Pipeline |
| A7 | `FEquippableItemGenerator::Generate \| "ActivitiesToGrant property not found"` | Item | M | Pipeline |
| A8 | `FEquippableItemGenerator::Generate \| "PickupMeshData property not found"` | Item | M | Pipeline |
| A9 | `FEquippableItemGenerator::Generate \| "TraceData property not found"` | Item | M | Pipeline |
| A10 | `FNarrativeEventGenerator::Generate \| "NPCTargets - property not found, manual setup"` | NPC | M | Pipeline |
| A11 | `FNarrativeEventGenerator::Generate \| "CharacterTargets - manual setup required"` | NPC | M | Pipeline |
| A12 | `FQuestGenerator::Generate \| "Questgiver (manual setup)"` | Quest | M | Pipeline |

**Note:** A1, A5-A11 are the 8 PROVEN GENERATOR BUGS.

---

### Category B: [WARN] Missing Class/Asset (28 items)

| # | Anchor | Subsystem | Type | Scope |
|---|--------|-----------|------|-------|
| B1 | `FBehaviorTreeGenerator::Generate \| "Task class not found: %s, using BTTask_Wait"` | BT | M | Pipeline |
| B2 | `CreateCallFunctionNode \| "Could not find enum value"` | General | M | Pipeline |
| B3 | `CreateCallFunctionNode \| "Could not resolve class '%s' for TSubclassOf"` | General | M | Pipeline |
| B4 | `CreateCallFunctionNode \| "Could not find input pin '%s' on function"` | General | M | Pipeline |
| B5 | `CreateCallFunctionNode \| "Could not find a function named"` | General | M | Pipeline |
| B6 | `CreateVariableGetNode \| "Could not resolve class for target '%s', falling back to self"` | General | M | Pipeline |
| B7 | `CreateVariableSetNode \| "Could not resolve class for target '%s', falling back to self"` | General | M | Pipeline |
| B8 | `CreatePropertyGetNode \| "Could not find class '%s'"` | General | M | Pipeline |
| B9 | `CreatePropertySetNode \| "Could not find class '%s'"` | General | M | Pipeline |
| B10 | `CreateBreakStructNode \| "Could not find struct '%s'"` | General | M | Pipeline |
| B11 | `CreateDynamicCastNode \| "could not find target class '%s'"` | General | M | Pipeline |
| B12 | `CreateAbilityTaskWaitAttributeChangeNode \| "Could not find Attribute pin"` | GAS | M | Pipeline |
| B13 | `CreateAbilityTaskWaitAttributeChangeNode \| "Could not find OnChange pin"` | GAS | M | Pipeline |
| B14 | `FDialogueBlueprintGenerator \| "Could not find dialogue event class"` | Dialogue | M | Pipeline |
| B15 | `FDialogueBlueprintGenerator \| "Could not find dialogue condition class"` | Dialogue | M | Pipeline |
| B16 | `FDialogueBlueprintGenerator \| "Could not find NPCDefinition"` | Dialogue | M | Pipeline |
| B17 | `FEquippableItemGenerator \| "ClothingMeshData property not found"` | Item | M | Pipeline |
| B18 | `FEquippableItemGenerator \| "EquipmentEffectValues property not found"` | Item | M | Pipeline |
| B19 | `FEquippableItemGenerator \| "Could not resolve EquipmentAbility class"` | Item | M | Pipeline |
| B20 | `FEquippableItemGenerator \| "Could not resolve Activity class"` | Item | M | Pipeline |
| B21 | `FEquippableItemGenerator \| "Could not load ItemWidgetOverride class"` | Item | M | Pipeline |
| B22 | `FAbilityConfigurationGenerator \| "Could not resolve ability"` | GAS | M | Pipeline |
| B23 | `FAbilityConfigurationGenerator \| "Could not resolve startup effect"` | GAS | M | Pipeline |
| B24 | `FAbilityConfigurationGenerator \| "Could not resolve default attributes"` | GAS | M | Pipeline |
| B25 | `FActivityConfigurationGenerator \| "Could not resolve activity"` | GAS | M | Pipeline |
| B26 | `FActivityConfigurationGenerator \| "Could not resolve goal generator"` | GAS | M | Pipeline |
| B27 | `FActivityScheduleGenerator \| "Could not resolve goal class - behavior will need manual setup"` | NPC | M | Pipeline |
| B28 | `FNPCDefinitionGenerator \| "Could not resolve dialogue class"` | NPC | M | Pipeline |

---

### Category C: [WARN] Reward Class Not Found (3 items)

| # | Anchor | Subsystem | Type | Scope |
|---|--------|-----------|------|-------|
| C1 | `FQuestGenerator \| "Could not load NE_GiveXP class - XP reward requires manual"` | Quest | M | Pipeline |
| C2 | `FQuestGenerator \| "Could not load BPE_AddCurrency class - Currency reward requires manual"` | Quest | M | Pipeline |
| C3 | `FQuestGenerator \| "Could not load BPE_AddItemToInventory class - Item reward requires manual"` | Quest | M | Pipeline |

---

### Category D: Comment-Only Deferred (4 items)

| # | Anchor | Subsystem | Type | Scope |
|---|--------|-----------|------|-------|
| D1 | `FDialogueTableConverter \| "For now, leave it empty - manual setup"` | Dialogue | M | Pipeline |
| D2 | `FDialogueBlueprintGenerator \| "Log info for manual setup via BPT_FinishDialogue"` | Dialogue | M | Pipeline |
| D3 | `FDialogueBlueprintGenerator \| "Log info for manual setup via Blueprint call to FailQuest"` | Quest | M | Pipeline |
| D4 | `FDialogueBlueprintGenerator \| "DefaultDialogueShot with sequence assets requires manual"` | Dialogue | M | Pipeline |

---

### Category S: Secondary Warnings (51 items)

#### S1-S28: Original Items

| # | Anchor | Subsystem | Type | Scope |
|---|--------|-----------|------|-------|
| S1 | `FActorBlueprintGenerator \| "Component class not found"` | General | M | Pipeline |
| S2 | `FAnimationMontageGenerator \| "Could not load skeleton - creating empty montage"` | General | M | Pipeline |
| S3 | `FDialogueBlueprintGenerator \| "Could not load dialogue audio"` | Dialogue | M | Pipeline |
| S4 | `FDialogueBlueprintGenerator \| "Could not find NPC node for reply"` | Dialogue | M | Pipeline |
| S5 | `FDialogueBlueprintGenerator \| "Could not find Player node for reply"` | Dialogue | M | Pipeline |
| S6 | `FDialogueBlueprintGenerator \| "Could not load camera shake class"` | Dialogue | M | Pipeline |
| S7 | `FDialogueBlueprintGenerator \| "Could not load sound attenuation"` | Dialogue | M | Pipeline |
| S8 | `FDialogueBlueprintGenerator \| "Could not load dialogue sequence class"` | Dialogue | M | Pipeline |
| S9 | `FGameplayCueGenerator \| "Could not load condition class"` | GAS | M | Pipeline |
| S10 | `FGameplayCueGenerator \| "Could not find Conditions array property"` | GAS | M | Pipeline |
| S11 | `FGameplayCueGenerator \| "Could not load ParticleSystem"` | Niagara | M | Pipeline |
| S12 | `FGameplayCueGenerator \| "Could not load Sound"` | GAS | M | Pipeline |
| S13 | `FGameplayCueGenerator \| "Could not load CameraShake class"` | GAS | M | Pipeline |
| S14 | `FNPCDefinitionGenerator \| "ActivitySchedules property not found"` | NPC | M | Pipeline |
| S15 | `FNPCDefinitionGenerator \| "Trigger class not found"` | NPC | M | Pipeline |
| S16 | `FNPCDefinitionGenerator \| "Failed to create trigger instance"` | NPC | M | Pipeline |
| S17 | `FNPCDefinitionGenerator \| "Event class not found"` | NPC | M | Pipeline |
| S18 | `FNPCDefinitionGenerator \| "Failed to create event instance"` | NPC | M | Pipeline |
| S19 | `FNPCDefinitionGenerator \| "Event property not found"` | NPC | M | Pipeline |
| S20 | `FGoalItemGenerator \| "Could not find parent class, using UNPCGoalItem"` | NPC | M | Pipeline |
| S21 | `FQuestGenerator \| "Could not load requirement class"` | Quest | M | Pipeline |
| S22 | `FQuestGenerator \| "Could not find QuestRequirements array property"` | Quest | M | Pipeline |
| S23 | `FQuestGenerator \| "Could not resolve task class"` | Quest | M | Pipeline |
| S24 | `FPOIGenerator \| "Failed to load POI icon"` | NPC | M | Pipeline |
| S25 | `FNPCSpawnerGenerator \| "Failed to create NPC spawn component"` | NPC | M | Pipeline |
| S26 | `FNPCSpawnerGenerator \| "Could not resolve POI location, using origin"` | NPC | M | Pipeline |
| S27 | `FNPCSpawnerGenerator \| "Could not load NPCDefinition"` | NPC | M | Pipeline |
| S28 | `FNPCSpawnerGenerator \| "Could not load goal class"` | NPC | M | Pipeline |

#### S29-S47: Additional Discoveries

| # | Anchor | Subsystem | Type | Scope |
|---|--------|-----------|------|-------|
| S29 | `FGameplayAbilityGenerator \| "CooldownGE not found"` | GAS | M | Pipeline |
| S30 | `GetPinTypeFromString \| "Class '%s' not found, using AActor as fallback"` | General | M | Pipeline |
| S31 | `FGameplayEffectGenerator \| "Property '%s' not found on class"` | GAS | M | Pipeline |
| S32 | `FGameplayEffectGenerator \| "Attribute set class '%s' not found"` | GAS | M | Pipeline |
| S33 | `FGameplayEffectGenerator \| "SetByCaller tag '%s' not found"` | GAS | M | Pipeline |
| S34 | `FGameplayEffectGenerator \| "Tag '%s' not found in GameplayTags"` | GAS | M | Pipeline |
| S35 | `FGameplayAbilityGenerator \| "Failed to generate custom function"` | GAS | M | Pipeline |
| S36 | `FActorBlueprintGenerator \| "Failed to generate function override"` | General | M | Pipeline |
| S37 | `FBlackboardGenerator \| "Parent blackboard '%s' not found"` | BT | M | Pipeline |
| S38 | `FBehaviorTreeGenerator \| "Blackboard not found"` | BT | M | Pipeline |
| S39 | `FBehaviorTreeGenerator \| "Parent node '%s' not found or not composite"` | BT | M | Pipeline |
| S40 | `FBehaviorTreeGenerator \| "Child node '%s' not found for parent"` | BT | M | Pipeline |
| S41 | `FBehaviorTreeGenerator \| "Failed to create UBehaviorTreeGraph"` | BT | M | Pipeline |
| S42 | `FMaterialGenerator \| "MaterialFunctionCall '%s' -> function NOT FOUND"` | Material | M | Pipeline |
| S43 | `FMaterialFunctionGenerator \| "MF connection source '%s' not found"` | Material | M | Pipeline |
| S44 | `FMaterialFunctionGenerator \| "MF connection target '%s' not found"` | Material | M | Pipeline |
| S45 | `FMaterialInstanceGenerator \| "Texture not found for param"` | Material | M | Pipeline |
| S46 | `CreateForEachLoopNode \| "ForEachLoop macro not found"` | General | M | Pipeline |
| S47 | `FQuestGenerator \| "Destination state not found"` | Quest | M | Pipeline |

#### S48-S51: Quest State Machine (R3 - Structural Invalidity)

| # | Anchor | Subsystem | Type | Scope |
|---|--------|-----------|------|-------|
| S48 | `FQuestGenerator \| "Quest has no valid start state"` | Quest | M | Pipeline |
| S49 | `FQuestGenerator \| "Duplicate state ID"` | Quest | M | Pipeline |
| S50 | `FQuestGenerator \| "Duplicate branch ID"` | Quest | M | Pipeline |
| S51 | `FPOIGenerator \| "POI tag not registered"` | NPC | M | Pipeline |

---

### Category F: Fallback Patterns (12 items)

| # | Anchor | Subsystem | Type | Scope |
|---|--------|-----------|------|-------|
| F1 | `GetPinTypeFromString \| "using AActor as fallback"` | General | M | Pipeline |
| F2 | `FWidgetBlueprintGenerator \| "Fallback to UUserWidget"` | General | M | Pipeline |
| F3 | `FBehaviorTreeGenerator \| "using BTTask_Wait"` | BT | M | Pipeline |
| F4 | `CreateCallFunctionNode \| "Still create the node with fallback"` | General | M | Pipeline |
| F5 | `CreateVariableGetNode \| "falling back to self"` | General | M | Pipeline |
| F6 | `CreateVariableSetNode \| "falling back to self"` | General | M | Pipeline |
| F7 | `GetPinPosition \| "Pin not found, return middle of node as fallback"` | General | M | Pipeline |
| F8 | `FAnimationMontageGenerator \| "creating empty montage"` | General | M | Pipeline |
| F9 | `FEquippableItemGenerator \| "Fallback to UEquippableItem"` | Item | M | Pipeline |
| F10 | `FGameplayCueGenerator \| "Default fallback - simple sprite burst"` | GAS | D | Pipeline |
| F11 | `FGoalItemGenerator \| "using UNPCGoalItem"` | NPC | M | Pipeline |
| F12 | `FNPCSpawnerGenerator \| "using origin"` | NPC | M | Pipeline |

**Note:** F10 is Type D (Default) - manifest did not specify cue effects, uses default sprite burst.

---

### Category T: Token Registry (2 items)

| # | Anchor | Subsystem | Type | Scope |
|---|--------|-----------|------|-------|
| T1 | `SerializeEvent \| "UNSUPPORTED(%s)"` | Token | M | Pipeline |
| T2 | `SetPropertyFromParam \| "Unsupported property type"` | Token | M | Pipeline |

---

### Category W: Window/Editor (6 items - NOT ENFORCED)

| # | Anchor | Subsystem | Type | Scope |
|---|--------|-----------|------|-------|
| W1 | `SGasAbilityGeneratorWindow \| "Sound Wave - Import audio manually"` | General | D | EditorUX |
| W2 | `SGasAbilityGeneratorWindow \| "AC_NPC_Default not found"` | NPC | D | EditorUX |
| W3 | `SGasAbilityGeneratorWindow \| "DefaultItemLoadout property not found"` | NPC | D | EditorUX |
| W4 | `SGasAbilityGeneratorWindow \| "AC_RunAndGun not found"` | NPC | D | EditorUX |
| W5 | `SGasAbilityGeneratorWindow \| "Apperance_Manny not found"` | NPC | D | EditorUX |
| W6 | `SGasAbilityGeneratorWindow \| "BP_NarrativeNPCCharacter not found"` | NPC | D | EditorUX |

**Note:** W1-W6 are Scope=EditorUX, Type=D - NOT enforced under Option 1.

---

## Summary Statistics

| Category | Total | Type M | Type D | Scope Pipeline | Scope EditorUX |
|----------|-------|--------|--------|----------------|----------------|
| A (INFO) | 12 | 12 | 0 | 12 | 0 |
| B (WARN class) | 28 | 28 | 0 | 28 | 0 |
| C (Reward) | 3 | 3 | 0 | 3 | 0 |
| D (Comment) | 4 | 4 | 0 | 4 | 0 |
| S (Secondary) | 51 | 51 | 0 | 51 | 0 |
| F (Fallback) | 12 | 11 | 1 | 12 | 0 |
| T (Token) | 2 | 2 | 0 | 2 | 0 |
| W (Window) | 6 | 0 | 6 | 0 | 6 |
| **TOTAL** | **118** | **111** | **7** | **112** | **6** |

---

## Audit Trail

| Round | Claude Position | GPT Position | Outcome |
|-------|-----------------|--------------|---------|
| 1 | v2 doc with 114 items, line corrections | v1.1 doc, challenged completeness | Merged findings |
| 2 | Proved 8 BUG items with full NP source | Accepted proof, challenged root cause | Root cause deferred |
| 3 | Accepted R1/R2/R3, Phase 3 diagnostics | Accepted, locked framework | Full consensus |
| 4 | Confirmed all locked items | Final confirmation | AUDIT COMPLETE |

### GPT Challenges Accepted by Claude

1. **Line number drift** - Multiple insertions, not single. Use anchors instead.
2. **Root cause hypothesis** - Withdrawn as unproven. Phase 3 will capture.
3. **Failure masking vs defaults** - Must distinguish before removal.
4. **Phase 3 must capture class identity** - Required for root cause diagnosis.

### Claude Challenges Accepted by GPT

1. **8 BUG items proven** - Full NP source grep accepted as evidence.
2. **Quest warnings = hard fails** - Structural invalidity cannot be warning-only.

---

## NP Header Proof (8 Generator Bugs)

### A1: PartySpeakerInfo
```cpp
// NarrativePro22B57/Source/NarrativeArsenal/Public/Tales/Dialogue.h:205
TArray<FPlayerSpeakerInfo> PartySpeakerInfo;
```

### A5: EquipmentAbilities
```cpp
// NarrativePro22B57/Source/NarrativeArsenal/Public/Items/EquippableItem.h:136
TArray<TSubclassOf<class UNarrativeGameplayAbility>> EquipmentAbilities;
```

### A6: Stats
```cpp
// NarrativePro22B57/Source/NarrativeArsenal/Public/Items/NarrativeItem.h:288
TArray<FNarrativeItemStat> Stats;
```

### A7: ActivitiesToGrant
```cpp
// NarrativePro22B57/Source/NarrativeArsenal/Public/Items/NarrativeItem.h:280
TArray<TSubclassOf<class UNPCActivity>> ActivitiesToGrant;
```

### A8: PickupMeshData
```cpp
// NarrativePro22B57/Source/NarrativeArsenal/Public/Items/NarrativeItem.h:509
FPickupMeshData PickupMeshData;
```

### A9: TraceData
```cpp
// NarrativePro22B57/Source/NarrativeArsenal/Public/Items/RangedWeaponItem.h:86
FCombatTraceData TraceData;
```

### A10: NPCTargets
```cpp
// NarrativePro22B57/Source/NarrativeArsenal/Public/Tales/NarrativeEvent.h:173
TArray<TObjectPtr<class UNPCDefinition>> NPCTargets;
```

### A11: CharacterTargets
```cpp
// NarrativePro22B57/Source/NarrativeArsenal/Public/Tales/NarrativeEvent.h:169
TArray<TObjectPtr<class UCharacterDefinition>> CharacterTargets;
```

---

## Failure Bucketing Capability

### Bucket 1: Bad Input Data
- Invalid attribute (AttackSpeed)
- Wrong function names
- Missing class context

### Bucket 2: Missing Dependencies
- SK_FatherCompanion skeleton
- Material function node 'dist'

### Bucket 3: Generator Logic Bugs
- None triggered in this run
- Infrastructure ready when they occur

### Bucket 4: Cascading Failures
- AC_FatherCompanion (depends on GA_FatherAttack)
- AC_FatherBehavior (depends on upstream activities)

---

## Next Steps (Phase 4)

Phase 4 goals:
1. Investigate root cause of 8 BUG properties (H1-H5 hypotheses)
2. Build fail-proof automation based on failure reasons
3. Consider structured JSON output for CI/CD integration

---

## Document History

| Version | Date | Changes |
|---------|------|---------|
| v1.0 | 2026-01-22 | Initial audit - 87 items |
| v2.0 | 2026-01-22 | Expanded to 114 items |
| v2.1 | 2026-01-22 | Anchored format, Type/Scope classification |
| FINAL | 2026-01-22 | Consolidated from 7 documents, Phase 2+3 complete |

---

## Superseded Documents (Archived)

The following documents were consolidated into this file and can be deleted:
- `Fail_Fast_Audit_v1.md` - Initial audit
- `Fail_Fast_Audit_v2.md` - Expanded audit
- `Fail_Fast_Audit_v2_1.md` - Anchored classification
- `Fail_Fast_Claude_GPT_Audit_v1.md` - Audit rules and phase plan
- `Phase3_Audit_Consensus.md` - Phase 3 completion
- `Phase2_Errors.csv` - Error data export
- `Phase2_Fail_Fast_Report.txt` - Detailed report
