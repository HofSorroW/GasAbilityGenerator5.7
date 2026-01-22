# Fail-Fast Audit Report v2.0

**Created:** 2026-01-22
**Updated:** 2026-01-22 (Phase 1 Complete - Line Number Verification)
**Purpose:** Eliminate all silent failures - convert to hard fails with error codes
**Philosophy:** Assets must be 100% automated or fail loudly - no "manual setup required"

---

## AUDIT VERIFICATION SUMMARY

**Phase 1 Status:** COMPLETE - All line numbers verified against current code
**Code Base:** GasAbilityGeneratorGenerators.cpp (~22,900 lines)

### Verification Method
1. Grep search for `[WARNING]`, `[WARN]`, `[INFO]` patterns
2. Grep search for `fallback`, `manual setup`, `continue.*skip` patterns
3. Grep search for `could not load`, `could not find`, `not found` patterns
4. Manual cross-reference with documented items

### Findings Summary

| Category | v1 Count | v2 Count | Change |
|----------|----------|----------|--------|
| Category A (INFO manual setup) | 12 | 12 | Line numbers corrected |
| Category B (WARN missing class) | 28 | 28 | Line numbers corrected |
| Category C (Reward class) | 3 | 3 | Line numbers corrected |
| Category D (Comment deferred) | 4 | 4 | No change |
| Category S (Secondary warnings) | 32 | 47 | +15 NEW findings |
| Category T (Token registry) | 2 | 2 | No change |
| Category W (Window/Editor) | 6 | 6 | No change |
| **TOTAL** | **87** | **102** | **+15 new** |

---

## Part 1: Category A - [INFO] Manual Setup Fallbacks (12 items)

**Behavior:** Logs "[INFO]" and continues, creating incomplete assets.

| # | File:Line (VERIFIED) | Current Message | Error Code | Severity |
|---|----------------------|-----------------|------------|----------|
| A1 | Generators.cpp:15625 | PartySpeakerInfo property not found | `E_PROPERTY_NOT_FOUND` | FAIL |
| A2 | Generators.cpp:16896 | MeshMaterials - complex struct, logging for manual | `E_MESHMATERIALS_NOT_AUTOMATED` | FAIL |
| A3 | Generators.cpp:16914 | Morphs - complex struct, logging for manual | `E_MORPHS_NOT_AUTOMATED` | FAIL |
| A4 | Generators.cpp:16969 | EquipmentEffectValues - logging values for manual | `E_EQUIPMENT_EFFECTS_NOT_AUTOMATED` | FAIL |
| A5 | Generators.cpp:17025 | EquipmentAbilities property not found | `E_EQUIPMENT_ABILITIES_NOT_FOUND` | FAIL |
| A6 | Generators.cpp:17085 | Stats property not found on UNarrativeItem | `E_STATS_PROPERTY_NOT_FOUND` | FAIL |
| A7 | Generators.cpp:17148 | ActivitiesToGrant property not found | `E_ACTIVITIES_PROPERTY_NOT_FOUND` | FAIL |
| A8 | Generators.cpp:17245 | PickupMeshData property not found | `E_PICKUPMESH_PROPERTY_NOT_FOUND` | FAIL |
| A9 | Generators.cpp:17293 | TraceData property not found | `E_TRACEDATA_PROPERTY_NOT_FOUND` | FAIL |
| A10 | Generators.cpp:18312 | NPCTargets property not found, manual setup | `E_NPCTARGETS_PROPERTY_NOT_FOUND` | FAIL |
| A11 | Generators.cpp:18362 | CharacterTargets - manual setup required | `E_CHARTARGETS_PROPERTY_NOT_FOUND` | FAIL |
| A12 | Generators.cpp:22123 | Questgiver (manual setup) | `E_QUESTGIVER_NOT_AUTOMATED` | FAIL |

**Line Number Corrections from v1:**
- A10: 18254 → 18312 (+58 lines drift)
- A11: 18304 → 18362 (+58 lines drift)
- A12: 22065 → 22123 (+58 lines drift)

---

## Part 2: Category B - [WARN] Missing Class/Asset (28 items)

**Behavior:** Logs "[WARN]" or "[WARNING]" and continues.

| # | File:Line (VERIFIED) | Current Message | Error Code | Severity |
|---|----------------------|-----------------|------------|----------|
| B1 | Generators.cpp:5282 | Task class not found, using BTTask_Wait | `E_BT_TASK_CLASS_NOT_FOUND` | FAIL |
| B2 | Generators.cpp:10381 | Could not find enum value | `E_ENUM_VALUE_NOT_FOUND` | FAIL |
| B3 | Generators.cpp:10480 | Could not resolve class for TSubclassOf | `E_CLASS_RESOLUTION_FAILED` | FAIL |
| B4 | Generators.cpp:10498 | Could not find input pin on function | `E_FUNCTION_PIN_NOT_FOUND` | FAIL |
| B5 | Generators.cpp:10545 | Could not find function | `E_FUNCTION_NOT_FOUND` | FAIL |
| B6 | Generators.cpp:10632 | VariableGet could not resolve class | `E_VARIABLE_CLASS_NOT_FOUND` | FAIL |
| B7 | Generators.cpp:10710 | VariableSet could not resolve class | `E_VARIABLE_CLASS_NOT_FOUND` | FAIL |
| B8 | Generators.cpp:10752 | PropertyGet could not find class | `E_PROPERTY_CLASS_NOT_FOUND` | FAIL |
| B9 | Generators.cpp:10815 | PropertySet could not find class | `E_PROPERTY_CLASS_NOT_FOUND` | FAIL |
| B10 | Generators.cpp:10903 | BreakStruct could not find struct | `E_STRUCT_NOT_FOUND` | FAIL |
| B11 | Generators.cpp:11174 | DynamicCast could not find target class | `E_CAST_CLASS_NOT_FOUND` | FAIL |
| B12 | Generators.cpp:14120 | Could not find Attribute pin | `E_ATTRIBUTE_PIN_NOT_FOUND` | FAIL |
| B13 | Generators.cpp:14172 | Could not find OnChange pin | `E_ONCHANGE_PIN_NOT_FOUND` | FAIL |
| B14 | Generators.cpp:14800 | Could not find dialogue event class | `E_DIALOGUE_EVENT_CLASS_NOT_FOUND` | FAIL |
| B15 | Generators.cpp:14864 | Could not find dialogue condition class | `E_DIALOGUE_CONDITION_CLASS_NOT_FOUND` | FAIL |
| B16 | Generators.cpp:15529 | Could not find NPCDefinition | `E_NPC_DEFINITION_NOT_FOUND` | FAIL |
| B17 | Generators.cpp:16929 | ClothingMeshData property not found | `E_CLOTHINGMESH_PROPERTY_NOT_FOUND` | FAIL |
| B18 | Generators.cpp:16968 | EquipmentEffectValues property not found | `E_EQUIPMENT_EFFECTS_PROPERTY_NOT_FOUND` | FAIL |
| B19 | Generators.cpp:17019 | Could not resolve EquipmentAbility class | `E_EQUIPMENT_ABILITY_CLASS_NOT_FOUND` | FAIL |
| B20 | Generators.cpp:17142 | Could not resolve Activity class | `E_ACTIVITY_CLASS_NOT_FOUND` | FAIL |
| B21 | Generators.cpp:17175 | Could not load ItemWidgetOverride class | `E_WIDGET_CLASS_NOT_FOUND` | FAIL |
| B22 | Generators.cpp:17704 | Could not resolve ability | `E_ABILITY_CLASS_NOT_FOUND` | FAIL |
| B23 | Generators.cpp:17743 | Could not resolve startup effect | `E_EFFECT_CLASS_NOT_FOUND` | FAIL |
| B24 | Generators.cpp:17772 | Could not resolve default attributes | `E_ATTRIBUTES_NOT_FOUND` | FAIL |
| B25 | Generators.cpp:17947 | Could not resolve activity | `E_ACTIVITY_CLASS_NOT_FOUND` | FAIL |
| B26 | Generators.cpp:17980 | Could not resolve goal generator | `E_GOAL_GENERATOR_NOT_FOUND` | FAIL |
| B27 | Generators.cpp:21451 | Could not resolve goal class - behavior will need manual setup | `E_GOAL_CLASS_NOT_FOUND` | FAIL |
| B28 | Generators.cpp:21798 | Could not resolve dialogue class | `E_DIALOGUE_CLASS_NOT_FOUND` | FAIL |

**Line Number Corrections from v1:**
- B22: 17689 → 17704 (+15 lines)
- B23: 17720 → 17743 (+23 lines)
- B24: 17749 → 17772 (+23 lines)
- B25: 17889 → 17947 (+58 lines)
- B26: 17922 → 17980 (+58 lines)
- B27: 21393 → 21451 (+58 lines)
- B28: 21740 → 21798 (+58 lines)

---

## Part 3: Category C - [WARN] Reward Class Not Found (3 items)

| # | File:Line (VERIFIED) | Current Message | Error Code | Severity |
|---|----------------------|-----------------|------------|----------|
| C1 | Generators.cpp:22187 | Could not load NE_GiveXP class - XP reward requires manual | `E_XP_REWARD_CLASS_NOT_FOUND` | FAIL |
| C2 | Generators.cpp:22222 | Could not load BPE_AddCurrency class - Currency reward requires manual | `E_CURRENCY_REWARD_CLASS_NOT_FOUND` | FAIL |
| C3 | Generators.cpp:22287 | Could not load BPE_AddItemToInventory class - Item reward requires manual | `E_ITEM_REWARD_CLASS_NOT_FOUND` | FAIL |

**Line Number Corrections from v1:**
- C1: 22129 → 22187 (+58 lines)
- C2: 22164 → 22222 (+58 lines)
- C3: 22229 → 22287 (+58 lines)

---

## Part 4: Category D - Comment-Only Deferred (4 items)

| # | File:Line | Current Comment | Error Code | Severity |
|---|-----------|-----------------|------------|----------|
| D1 | DialogueTableConverter.cpp:40 | For now, leave it empty - manual setup or Phase 2 | `E_DIALOGUE_SPEAKER_NOT_AUTOMATED` | FAIL |
| D2 | Generators.cpp:15090 | Log info for manual setup via BPT_FinishDialogue | `E_QUEST_TASK_NOT_AUTOMATED` | FAIL |
| D3 | Generators.cpp:15096 | Log info for manual setup via Blueprint call to FailQuest | `E_QUEST_FAIL_NOT_AUTOMATED` | FAIL |
| D4 | Generators.cpp:15480 | DefaultDialogueShot with sequence assets requires manual | `E_DIALOGUE_SHOT_SEQUENCE_NOT_AUTOMATED` | FAIL |

---

## Part 5: Category S - Secondary Warnings (47 items - WAS 32)

**NEW FINDINGS from Phase 1 verification (+15 items):**

### Original Items (with line corrections)

| # | File:Line (VERIFIED) | Current Message | Recommendation |
|---|----------------------|-----------------|----------------|
| S1 | Generators.cpp:3114 | Component class not found | Convert to FAIL |
| S2 | Generators.cpp:14452 | Could not load skeleton - creating empty montage | Convert to FAIL |
| S3 | Generators.cpp:15011 | Could not load dialogue audio | Convert to FAIL |
| S4 | Generators.cpp:15142 | Could not find NPC node for reply | Convert to FAIL |
| S5 | Generators.cpp:15155 | Could not find Player node for reply | Convert to FAIL |
| S6 | Generators.cpp:15390 | Could not load camera shake class | Convert to FAIL |
| S7 | Generators.cpp:15416 | Could not load sound attenuation | Convert to FAIL |
| S8 | Generators.cpp:15475 | Could not load dialogue sequence class | Convert to FAIL |
| S9 | Generators.cpp:18493 | Could not load condition class | Convert to FAIL |
| S10 | Generators.cpp:18499 | Could not find Conditions array property | Convert to FAIL |
| S11 | Generators.cpp:18723 | Could not load ParticleSystem | Convert to FAIL |
| S12 | Generators.cpp:18764 | Could not load Sound | Convert to FAIL |
| S13 | Generators.cpp:18796 | Could not load CameraShake class | Convert to FAIL |
| S14 | Generators.cpp:19378 | ActivitySchedules property not found | Convert to FAIL |
| S15 | Generators.cpp:19858 | Trigger class not found | Convert to FAIL |
| S16 | Generators.cpp:19866 | Failed to create trigger instance | Convert to FAIL |
| S17 | Generators.cpp:19924 | Event class not found | Convert to FAIL |
| S18 | Generators.cpp:19932 | Failed to create event instance | Convert to FAIL |
| S19 | Generators.cpp:19956 | Event property not found | Convert to FAIL |
| S20 | Generators.cpp:21552 | Could not find parent class, using UNPCGoalItem | Convert to FAIL |
| S21 | Generators.cpp:21939 | Could not load requirement class | Convert to FAIL |
| S22 | Generators.cpp:21945 | Could not find QuestRequirements array property | Convert to FAIL |
| S23 | Generators.cpp:22034 | Could not resolve task class | Convert to FAIL |
| S24 | Generators.cpp:22531 | Failed to load POI icon | Convert to FAIL |
| S25 | Generators.cpp:22677 | Failed to create NPC spawn component | Convert to FAIL |
| S26 | Generators.cpp:22713 | Could not resolve POI location, using origin | Convert to FAIL |
| S27 | Generators.cpp:22758 | Could not load NPCDefinition (spawner) | Convert to FAIL |
| S28 | Generators.cpp:22833 | Could not load goal class (spawner) | Convert to FAIL |

### NEW Items (Found in Phase 1 Verification)

| # | File:Line | Current Message | Recommendation |
|---|-----------|-----------------|----------------|
| S29 | Generators.cpp:1568 | CooldownGE not found (searched multiple paths) | Convert to FAIL |
| S30 | Generators.cpp:1925 | Class not found, using AActor as fallback | Convert to FAIL |
| S31 | Generators.cpp:2370 | Property not found on GE class | Convert to FAIL |
| S32 | Generators.cpp:2375 | Attribute set class not found | Convert to FAIL |
| S33 | Generators.cpp:2411 | SetByCaller tag not found | Convert to FAIL |
| S34 | Generators.cpp:2448 | Tag not found in GameplayTags | Convert to FAIL |
| S35 | Generators.cpp:2742 | Failed to generate custom function | Convert to FAIL |
| S36 | Generators.cpp:3229 | Failed to generate function override | Convert to FAIL |
| S37 | Generators.cpp:4762 | Parent blackboard not found | Convert to FAIL |
| S38 | Generators.cpp:5023 | Blackboard not found (session cache) | Convert to FAIL |
| S39 | Generators.cpp:5468 | Parent node not found (BT) | Convert to FAIL |
| S40 | Generators.cpp:5489 | Child node not found (BT) | Convert to FAIL |
| S41 | Generators.cpp:5587 | Failed to create UBehaviorTreeGraph | Convert to FAIL |
| S42 | Generators.cpp:6167 | MaterialFunctionCall function NOT FOUND | Convert to FAIL |
| S43 | Generators.cpp:8155 | MF connection source not found | Convert to FAIL |
| S44 | Generators.cpp:8160 | MF connection target not found | Convert to FAIL |
| S45 | Generators.cpp:8481 | Texture not found for param | Convert to FAIL |
| S46 | Generators.cpp:11231 | ForEachLoop macro not found | Convert to FAIL |
| S47 | Generators.cpp:22087 | Destination state not found (Quest) | Convert to FAIL |

### Additional Validation Warnings (Quest State Machine)

| # | File:Line | Current Message | Recommendation |
|---|-----------|-----------------|----------------|
| S48 | Generators.cpp:22346 | Quest has no valid start state | Convert to FAIL |
| S49 | Generators.cpp:22356 | Duplicate state ID | Convert to FAIL |
| S50 | Generators.cpp:22374 | Duplicate branch ID | Convert to FAIL |
| S51 | Generators.cpp:22512 | POI tag not registered | Convert to FAIL |

---

## Part 6: Fallback Patterns to Remove

These are "fallback" patterns that silently use defaults instead of failing:

| # | File:Line | Fallback Behavior | Required Change |
|---|-----------|-------------------|-----------------|
| F1 | Generators.cpp:1925 | AActor fallback if class not found | Remove fallback, fail instead |
| F2 | Generators.cpp:3330-3331 | UUserWidget fallback | Remove fallback, fail instead |
| F3 | Generators.cpp:5282 | BTTask_Wait fallback | Remove fallback, fail instead |
| F4 | Generators.cpp:10550 | Create node with fallback (custom function) | Remove fallback, fail instead |
| F5 | Generators.cpp:10625-10632 | VariableGet fallback to self | Remove fallback, fail instead |
| F6 | Generators.cpp:10703-10710 | VariableSet fallback to self | Remove fallback, fail instead |
| F7 | Generators.cpp:11882 | Pin not found, return middle of node | Remove fallback, fail instead |
| F8 | Generators.cpp:14452 | Create empty montage if skeleton missing | Remove fallback, fail instead |
| F9 | Generators.cpp:15848 | UEquippableItem fallback if parent not found | Remove fallback, fail instead |
| F10 | Generators.cpp:20364 | Default sprite burst | Remove fallback, fail instead |
| F11 | Generators.cpp:21552 | UNPCGoalItem fallback | Remove fallback, fail instead |
| F12 | Generators.cpp:22713 | Origin fallback for POI location | Remove fallback, fail instead |

---

## Part 7: Token Registry Fallbacks

| # | File:Line | Current Behavior | Error Code |
|---|-----------|------------------|------------|
| T1 | DialogueTokenRegistry.cpp:1178-1179 | Returns UNSUPPORTED(ClassName) | `E_TOKEN_EVENT_TYPE_UNKNOWN` |
| T2 | DialogueTokenRegistry.cpp:1534 | Returns error string | `E_TOKEN_PROPERTY_TYPE_UNSUPPORTED` |

---

## Part 8: Window/Editor Fallbacks

| # | File:Line | Current Message | Recommendation |
|---|-----------|-----------------|----------------|
| W1 | Window.cpp:1676 | Sound Wave - Import audio manually | Convert to FAIL or remove feature |
| W2 | Window.cpp:1806 | AC_NPC_Default not found | Convert to FAIL |
| W3 | Window.cpp:1863 | DefaultItemLoadout property not found | Convert to FAIL |
| W4 | Window.cpp:1916 | AC_RunAndGun not found | Convert to FAIL |
| W5 | Window.cpp:1936 | Apperance_Manny not found | Convert to FAIL |
| W6 | Window.cpp:1956 | BP_NarrativeNPCCharacter not found | Convert to FAIL |
| W7 | NPCTableValidator.cpp:173 | No NPC Blueprint assigned | Keep as WARNING (validation) |

---

## AUDIT DISCOVERY: Generator Bugs (8 items)

**CRITICAL:** These 8 properties EXIST in Narrative Pro but the generator's reflection lookup fails. These are BUGS, not missing automation.

| Item | Property | NP Class | Header:Line | Status |
|------|----------|----------|-------------|--------|
| A1 | `PartySpeakerInfo` | UDialogue | Dialogue.h:205 | **BUG** |
| A5 | `EquipmentAbilities` | UEquippableItem | EquippableItem.h:136 | **BUG** |
| A6 | `Stats` | UNarrativeItem | NarrativeItem.h:288 | **BUG** |
| A7 | `ActivitiesToGrant` | UNarrativeItem | NarrativeItem.h:280 | **BUG** |
| A8 | `PickupMeshData` | UNarrativeItem | NarrativeItem.h:509 | **BUG** |
| A9 | `TraceData` | URangedWeaponItem | RangedWeaponItem.h:86 | **BUG** |
| A10 | `NPCTargets` | UNarrativeEvent | NarrativeEvent.h:173 | **BUG** |
| A11 | `CharacterTargets` | UNarrativeEvent | NarrativeEvent.h:169 | **BUG** |

### Hypothesized Root Causes

1. **Wrong Class:** `CDO->GetClass()` returns different class than expected
2. **Parent Class Resolution:** `FindParentClass()` not resolving to correct NP class
3. **Module Load Order:** NP modules not loaded when reflection runs
4. **Inheritance Chain:** Property on parent not visible via `FindPropertyByName()`

---

## Conversion Priority Matrix

| Priority | Category | Count | Description |
|----------|----------|-------|-------------|
| P0 | A + B + C + D | 47 | Primary silent failures |
| P1 | S (original) | 28 | Secondary warnings |
| P1.5 | S (new) | 19 | Newly discovered warnings |
| P2 | F | 12 | Fallback patterns |
| P3 | T | 2 | Token registry |
| P4 | W | 6 | Window/Editor (excl W7) |

**Total Items to Convert: 114**

---

## Error Code Registry (Complete)

```cpp
// Add to GasAbilityGeneratorTypes.h

// Property Resolution Errors
#define E_PROPERTY_NOT_FOUND "E_PROPERTY_NOT_FOUND"
#define E_STATS_PROPERTY_NOT_FOUND "E_STATS_PROPERTY_NOT_FOUND"
#define E_ACTIVITIES_PROPERTY_NOT_FOUND "E_ACTIVITIES_PROPERTY_NOT_FOUND"
#define E_PICKUPMESH_PROPERTY_NOT_FOUND "E_PICKUPMESH_PROPERTY_NOT_FOUND"
#define E_TRACEDATA_PROPERTY_NOT_FOUND "E_TRACEDATA_PROPERTY_NOT_FOUND"
#define E_NPCTARGETS_PROPERTY_NOT_FOUND "E_NPCTARGETS_PROPERTY_NOT_FOUND"
#define E_CHARTARGETS_PROPERTY_NOT_FOUND "E_CHARTARGETS_PROPERTY_NOT_FOUND"
#define E_CLOTHINGMESH_PROPERTY_NOT_FOUND "E_CLOTHINGMESH_PROPERTY_NOT_FOUND"
#define E_EQUIPMENT_ABILITIES_NOT_FOUND "E_EQUIPMENT_ABILITIES_NOT_FOUND"
#define E_EQUIPMENT_EFFECTS_PROPERTY_NOT_FOUND "E_EQUIPMENT_EFFECTS_PROPERTY_NOT_FOUND"

// Class Resolution Errors
#define E_CLASS_RESOLUTION_FAILED "E_CLASS_RESOLUTION_FAILED"
#define E_ABILITY_CLASS_NOT_FOUND "E_ABILITY_CLASS_NOT_FOUND"
#define E_ACTIVITY_CLASS_NOT_FOUND "E_ACTIVITY_CLASS_NOT_FOUND"
#define E_EFFECT_CLASS_NOT_FOUND "E_EFFECT_CLASS_NOT_FOUND"
#define E_GOAL_CLASS_NOT_FOUND "E_GOAL_CLASS_NOT_FOUND"
#define E_GOAL_GENERATOR_NOT_FOUND "E_GOAL_GENERATOR_NOT_FOUND"
#define E_DIALOGUE_CLASS_NOT_FOUND "E_DIALOGUE_CLASS_NOT_FOUND"
#define E_NPC_DEFINITION_NOT_FOUND "E_NPC_DEFINITION_NOT_FOUND"
#define E_WIDGET_CLASS_NOT_FOUND "E_WIDGET_CLASS_NOT_FOUND"
#define E_EQUIPMENT_ABILITY_CLASS_NOT_FOUND "E_EQUIPMENT_ABILITY_CLASS_NOT_FOUND"

// Blueprint Node Errors
#define E_FUNCTION_NOT_FOUND "E_FUNCTION_NOT_FOUND"
#define E_FUNCTION_PIN_NOT_FOUND "E_FUNCTION_PIN_NOT_FOUND"
#define E_ENUM_VALUE_NOT_FOUND "E_ENUM_VALUE_NOT_FOUND"
#define E_STRUCT_NOT_FOUND "E_STRUCT_NOT_FOUND"
#define E_CAST_CLASS_NOT_FOUND "E_CAST_CLASS_NOT_FOUND"
#define E_VARIABLE_CLASS_NOT_FOUND "E_VARIABLE_CLASS_NOT_FOUND"
#define E_PROPERTY_CLASS_NOT_FOUND "E_PROPERTY_CLASS_NOT_FOUND"
#define E_ATTRIBUTE_PIN_NOT_FOUND "E_ATTRIBUTE_PIN_NOT_FOUND"
#define E_ONCHANGE_PIN_NOT_FOUND "E_ONCHANGE_PIN_NOT_FOUND"
#define E_BT_TASK_CLASS_NOT_FOUND "E_BT_TASK_CLASS_NOT_FOUND"

// Dialogue/Quest Errors
#define E_DIALOGUE_EVENT_CLASS_NOT_FOUND "E_DIALOGUE_EVENT_CLASS_NOT_FOUND"
#define E_DIALOGUE_CONDITION_CLASS_NOT_FOUND "E_DIALOGUE_CONDITION_CLASS_NOT_FOUND"
#define E_DIALOGUE_SPEAKER_NOT_AUTOMATED "E_DIALOGUE_SPEAKER_NOT_AUTOMATED"
#define E_DIALOGUE_SHOT_SEQUENCE_NOT_AUTOMATED "E_DIALOGUE_SHOT_SEQUENCE_NOT_AUTOMATED"
#define E_QUEST_TASK_NOT_AUTOMATED "E_QUEST_TASK_NOT_AUTOMATED"
#define E_QUEST_FAIL_NOT_AUTOMATED "E_QUEST_FAIL_NOT_AUTOMATED"
#define E_QUESTGIVER_NOT_AUTOMATED "E_QUESTGIVER_NOT_AUTOMATED"

// Reward Errors
#define E_XP_REWARD_CLASS_NOT_FOUND "E_XP_REWARD_CLASS_NOT_FOUND"
#define E_CURRENCY_REWARD_CLASS_NOT_FOUND "E_CURRENCY_REWARD_CLASS_NOT_FOUND"
#define E_ITEM_REWARD_CLASS_NOT_FOUND "E_ITEM_REWARD_CLASS_NOT_FOUND"

// Complex Struct Errors
#define E_MESHMATERIALS_NOT_AUTOMATED "E_MESHMATERIALS_NOT_AUTOMATED"
#define E_MORPHS_NOT_AUTOMATED "E_MORPHS_NOT_AUTOMATED"
#define E_EQUIPMENT_EFFECTS_NOT_AUTOMATED "E_EQUIPMENT_EFFECTS_NOT_AUTOMATED"

// Token Registry Errors
#define E_TOKEN_EVENT_TYPE_UNKNOWN "E_TOKEN_EVENT_TYPE_UNKNOWN"
#define E_TOKEN_PROPERTY_TYPE_UNSUPPORTED "E_TOKEN_PROPERTY_TYPE_UNSUPPORTED"

// NEW - Behavior Tree Errors
#define E_BLACKBOARD_NOT_FOUND "E_BLACKBOARD_NOT_FOUND"
#define E_BT_PARENT_NODE_NOT_FOUND "E_BT_PARENT_NODE_NOT_FOUND"
#define E_BT_CHILD_NODE_NOT_FOUND "E_BT_CHILD_NODE_NOT_FOUND"
#define E_BT_GRAPH_CREATE_FAILED "E_BT_GRAPH_CREATE_FAILED"

// NEW - Material Errors
#define E_MF_FUNCTION_NOT_FOUND "E_MF_FUNCTION_NOT_FOUND"
#define E_MF_CONNECTION_SOURCE_NOT_FOUND "E_MF_CONNECTION_SOURCE_NOT_FOUND"
#define E_MF_CONNECTION_TARGET_NOT_FOUND "E_MF_CONNECTION_TARGET_NOT_FOUND"
#define E_TEXTURE_NOT_FOUND "E_TEXTURE_NOT_FOUND"

// NEW - GE/Tag Errors
#define E_COOLDOWN_GE_NOT_FOUND "E_COOLDOWN_GE_NOT_FOUND"
#define E_ATTRIBUTE_SET_NOT_FOUND "E_ATTRIBUTE_SET_NOT_FOUND"
#define E_SETBYCALLER_TAG_NOT_FOUND "E_SETBYCALLER_TAG_NOT_FOUND"
#define E_GAMEPLAY_TAG_NOT_FOUND "E_GAMEPLAY_TAG_NOT_FOUND"

// NEW - Quest State Machine Errors
#define E_QUEST_NO_START_STATE "E_QUEST_NO_START_STATE"
#define E_QUEST_DUPLICATE_STATE_ID "E_QUEST_DUPLICATE_STATE_ID"
#define E_QUEST_DUPLICATE_BRANCH_ID "E_QUEST_DUPLICATE_BRANCH_ID"
#define E_QUEST_DESTINATION_STATE_NOT_FOUND "E_QUEST_DESTINATION_STATE_NOT_FOUND"
```

---

## Document History

| Date | Version | Change |
|------|---------|--------|
| 2026-01-22 | 1.0 | Initial comprehensive audit - 87 total silent failures |
| 2026-01-22 | 2.0 | Phase 1 Complete - Line number verification, +27 new items (114 total) |

---

## GPT Audit Challenge Points

The following items require GPT verification:

1. **Line Number Drift Pattern:** Most items drifted by +58 lines. Was there a ~58 line addition to the file? Need to verify when this happened.

2. **Generator Bug Hypothesis:** 8 items marked as "BUG" need verification that the properties actually exist on Narrative Pro classes. GPT should validate against NarrativePro headers.

3. **Fallback Pattern Classification:** Items F1-F12 use "fallback" terminology. Are these truly silent failures or intentional graceful degradation? Need design intent verification.

4. **New Discovery S29-S51:** These 23 items were NOT in v1 document. GPT should verify these are legitimate silent failures that should become hard fails.

5. **Quest State Machine Warnings (S48-S51):** These are validation warnings. Should they be hard fails that block quest generation, or are they acceptable validation-only?
