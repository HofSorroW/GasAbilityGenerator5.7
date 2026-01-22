# Complete Gap Analysis - All 33 Items

**Created:** 2026-01-22
**Updated:** 2026-01-22 (Audit Session)
**Source:** TODO_Tracking.md consolidated list
**Total Items:** 33 (excluding 2 VFX/Niagara)

---

## AUDIT FINDINGS (2026-01-22)

**CRITICAL DISCOVERY:** Initial analysis incorrectly categorized many items as "Narrative Pro limitations" when they are actually **generator bugs**. Properties exist as UPROPERTYs but generator reports "not found".

### 8 Properties Verified to Exist (Generator Bug, NOT NP Limitation)

| # | Property | Class | Header Location | UPROPERTY Verified |
|---|----------|-------|-----------------|-------------------|
| 1 | `PartySpeakerInfo` | UDialogue | Dialogue.h:205 | ✅ YES |
| 2 | `EquipmentAbilities` | UEquippableItem | EquippableItem.h:136 | ✅ YES |
| 3 | `PickupMeshData` | UNarrativeItem | NarrativeItem.h:509 | ✅ YES |
| 4 | `Stats` | UNarrativeItem | NarrativeItem.h:288 | ✅ YES |
| 5 | `ActivitiesToGrant` | UNarrativeItem | NarrativeItem.h:280 | ✅ YES |
| 6 | `CharacterTargets` | UNarrativeEvent | NarrativeEvent.h:169 | ✅ YES |
| 7 | `NPCTargets` | UNarrativeEvent | NarrativeEvent.h:173 | ✅ YES |
| 8 | `TraceData` | URangedWeaponItem | RangedWeaponItem.h:86 | ✅ YES |

### Root Cause Hypotheses (Pending GPT Validation)

1. **Wrong Class Lookup:** Generator may be looking on wrong class (e.g., base class instead of derived)
2. **Parent Class Resolution:** Parent class not resolving correctly before reflection lookup
3. **Module Dependency:** Missing module dependencies preventing property resolution
4. **Class Hierarchy:** Property on parent class not visible to child CDO

### Items Reclassified

| Item | Old Classification | New Classification |
|------|-------------------|-------------------|
| Item 2: PartySpeakerInfo | NP Limitation | **Generator Bug** |
| Item 12: EquipmentAbilities | NP Limitation | **Generator Bug** |
| Item 13: Stats | NP Limitation | **Generator Bug** |
| Item 14: ActivitiesToGrant | NP Limitation | **Generator Bug** |
| Item 15: PickupMeshData | NP Limitation | **Generator Bug** |
| Item 16: TraceData | NP Limitation | **Generator Bug** |
| Item 17: NPCTargets | NP Limitation | **Generator Bug** |
| Item 18: CharacterTargets | NP Limitation | **Generator Bug** |

---

## Summary Breakdown

| Category | Count | Items |
|----------|-------|-------|
| Manual Setup - Dialogue | 3 | Conditions/events, PartySpeakerInfo, DialogueShot |
| Manual Setup - Quest | 6 | BPT_FinishDialogue, FailQuest, Questgiver, XP/Currency/Item rewards |
| Manual Setup - Item/Equipment | 7 | MeshMaterials, Morphs, EquipmentAbilities, Stats, ActivitiesToGrant, PickupMeshData, TraceData |
| Manual Setup - NPC/Character | 3 | NPCTargets, CharacterTargets, NPC Blueprint |
| Manual Setup - AI/Goals | 1 | Goal class resolution |
| Documented Limitations - Property/Type | 5 | Nested dots, MediaTexture, Nested structs, Root structs, MFI |
| Documented Limitations - Node Types | 3 | Event graph nodes, Function override nodes, Custom function nodes |
| Documented Limitations - Token Registry | 2 | Unknown events, Unsupported properties |
| Acceptable Placeholders | 3 | Sound Wave, Level Sequence, Dialogue placeholder rows |
| **TOTAL** | **33** | |

---

## SECTION 1: Manual Setup Items (20 items)

### 1.1 Dialogue System (3 items)

#### Item 1: Dialogue Conditions/Events
- **Location:** `DialogueTableConverter.cpp:40`
- **Function:** `FDialogueTableConverter::ConvertRowsToManifest()`
- **Current Code:**
  ```cpp
  // For now, leave it empty - manual setup or Phase 2
  ```
- **Affected Assets:** All `DBP_*` (DialogueBlueprint) assets with conditions or events on nodes
- **Manifest Section:** `dialogue_blueprints[].dialogue_tree.nodes[].conditions[]` and `.events[]`
- **What's Missing:** Parser doesn't populate conditions/events from table rows to manifest definitions
- **Impact:** Dialogue blueprints are created without node conditions or events - must add manually in editor

#### Item 2: PartySpeakerInfo Property
- **Location:** `GasAbilityGeneratorGenerators.cpp:15625`
- **Function:** `FDialogueBlueprintGenerator::Generate()`
- **Current Code:**
  ```cpp
  LogGeneration(TEXT("  [INFO] PartySpeakerInfo property not found on UDialogue - logging for manual setup:"));
  ```
- **Affected Assets:** All `DBP_*` DialogueBlueprint assets with player party speakers
- **Manifest Section:** `dialogue_blueprints[].speakers[]` (party speakers)
- **Narrative Pro Property:** `UDialogue::PartySpeakerInfo` (TArray<FPlayerSpeakerInfo>)
- **What's Missing:** Property reflection lookup fails - may be version-specific or renamed in Narrative Pro v2.2
- **Impact:** Party speaker info not set on dialogues - must configure manually

#### Item 3: DefaultDialogueShot Sequences
- **Location:** `GasAbilityGeneratorGenerators.cpp:15481`
- **Function:** `FDialogueBlueprintGenerator::Generate()`
- **Current Code:**
  ```cpp
  LogGeneration(TEXT("  NOTE: DefaultDialogueShot with sequence assets requires manual editor configuration"));
  ```
- **Affected Assets:** `DBP_*` assets with `default_dialogue_shot.sequence_assets[]`
- **Manifest Section:** `dialogue_blueprints[].default_dialogue_shot.sequence_assets[]`
- **Narrative Pro Property:** `UDialogue::DefaultDialogueShot` (complex sequence composition)
- **What's Missing:** Single sequence class works; arrays of sequence assets don't
- **Impact:** Complex camera sequences not configured - must set up manually in Dialogue editor

---

### 1.2 Quest System (6 items)

#### Item 4: Quest Dialogue Setup (BPT_FinishDialogue)
- **Location:** `GasAbilityGeneratorGenerators.cpp:15087`
- **Function:** `FDialogueBlueprintGenerator::Generate()` (dialogue node events)
- **Current Code:**
  ```cpp
  // Log info for manual setup via BPT_FinishDialogue task on quest branch
  if (!NodeDef.CompleteQuestBranch.IsEmpty())
  {
      UE_LOG(LogGasAbilityGenerator, Log, TEXT("    [INFO] complete_quest_branch: %s - Use BPT_FinishDialogue task on quest branch"), *NodeDef.CompleteQuestBranch);
  }
  ```
- **Affected Assets:** Dialogue nodes with `complete_quest_branch` field
- **Manifest Section:** `dialogue_blueprints[].dialogue_tree.nodes[].complete_quest_branch`
- **What's Missing:** BPT_FinishDialogue task instantiation and property setting via reflection
- **Impact:** Quest completion triggers not wired to dialogue nodes

#### Item 5: Quest Fail Setup (FailQuest call)
- **Location:** `GasAbilityGeneratorGenerators.cpp:15093`
- **Function:** `FDialogueBlueprintGenerator::Generate()` (dialogue node events)
- **Current Code:**
  ```cpp
  // Log info for manual setup via Blueprint call to Quest->FailQuest()
  if (!NodeDef.FailQuest.IsEmpty())
  {
      UE_LOG(LogGasAbilityGenerator, Log, TEXT("    [INFO] fail_quest: %s - Call Quest->FailQuest() from Blueprint event"), *NodeDef.FailQuest);
  }
  ```
- **Affected Assets:** Dialogue nodes with `fail_quest` field
- **Manifest Section:** `dialogue_blueprints[].dialogue_tree.nodes[].fail_quest`
- **What's Missing:** No NE_FailQuest event exists in Narrative Pro - requires Blueprint node generation
- **Impact:** Quest failure triggers not implemented on dialogue nodes

#### Item 6: Questgiver Linking
- **Location:** `GasAbilityGeneratorGenerators.cpp:22062-22066`
- **Function:** `FQuestGenerator::Generate()`
- **Current Code:**
  ```cpp
  // v3.9.6: Log questgiver for manual setup
  if (!Definition.Questgiver.IsEmpty())
  {
      LogGeneration(FString::Printf(TEXT("  Questgiver (manual setup): %s"), *Definition.Questgiver));
  }
  ```
- **Affected Assets:** All `Quest_*` assets with `questgiver` field
- **Manifest Section:** `quests[].questgiver`
- **What's Missing:** NPC → Dialogue → Quest linking chain not automated
- **Impact:** Questgiver is documentation-only; actual NPC dialogue must manually have start_quest event

#### Item 7: XP Reward Class (NE_GiveXP)
- **Location:** `GasAbilityGeneratorGenerators.cpp:22129`
- **Function:** `FQuestGenerator::Generate()` (rewards section)
- **Current Code:**
  ```cpp
  LogGeneration(FString::Printf(TEXT("  [WARN] Could not load NE_GiveXP class - XP reward requires manual setup")));
  ```
- **Affected Assets:** Quests with `rewards.xp` > 0
- **Manifest Section:** `quests[].rewards.xp`
- **Searched Paths:**
  - `/NarrativePro/Pro/Core/Tales/Events/NE_GiveXP.NE_GiveXP_C`
  - `/NarrativePro/Tales/Events/NE_GiveXP.NE_GiveXP_C`
  - `/NarrativePro/Events/NE_GiveXP.NE_GiveXP_C`
  - `/Game/NarrativePro/Events/NE_GiveXP.NE_GiveXP_C`
- **What's Missing:** NE_GiveXP class not found at any search path
- **Impact:** XP rewards not added to success state events

#### Item 8: Currency Reward Class (BPE_AddCurrency)
- **Location:** `GasAbilityGeneratorGenerators.cpp:22164`
- **Function:** `FQuestGenerator::Generate()` (rewards section)
- **Current Code:**
  ```cpp
  LogGeneration(FString::Printf(TEXT("  [WARN] Could not load BPE_AddCurrency class - Currency reward requires manual setup")));
  ```
- **Affected Assets:** Quests with `rewards.currency` > 0
- **Manifest Section:** `quests[].rewards.currency`
- **Searched Paths:** Similar to NE_GiveXP
- **What's Missing:** BPE_AddCurrency class not found at any search path
- **Impact:** Currency rewards not added to success state events

#### Item 9: Item Reward Class (BPE_AddItemToInventory)
- **Location:** `GasAbilityGeneratorGenerators.cpp:22229`
- **Function:** `FQuestGenerator::Generate()` (rewards section)
- **Current Code:**
  ```cpp
  LogGeneration(FString::Printf(TEXT("  [WARN] Could not load BPE_AddItemToInventory class - Item reward requires manual setup")));
  ```
- **Affected Assets:** Quests with `rewards.items[]` array
- **Manifest Section:** `quests[].rewards.items[]`
- **Searched Paths:** Similar to NE_GiveXP
- **What's Missing:** BPE_AddItemToInventory class not found at any search path
- **Impact:** Item rewards not added to success state events

---

### 1.3 Item/Equipment System (7 items)

#### Item 10: MeshMaterials (Complex Struct)
- **Location:** `GasAbilityGeneratorGenerators.cpp:16896`
- **Function:** `FEquippableItemGenerator::Generate()`
- **Current Code:**
  ```cpp
  LogGeneration(FString::Printf(TEXT("    [INFO] MeshMaterials (%d entries) - complex struct, logging for manual setup:"), Definition.ClothingMesh.Materials.Num()));
  ```
- **Affected Assets:** `EI_*` EquippableItem assets with `clothing_mesh.materials[]`
- **Manifest Section:** `equippable_items[].clothing_mesh.materials[]`
- **Narrative Pro Struct:** `FCreatorMeshMaterial` containing:
  - `Material: TSoftObjectPtr<UMaterialInterface>`
  - `VectorParams: TArray<FCreatorMeshMaterialParam_Vector>` (with FGameplayTag)
  - `ScalarParams: TArray<FCreatorMeshMaterialParam_Scalar>` (with FGameplayTag)
- **What's Missing:** Nested array population with gameplay tag validation
- **Impact:** Clothing materials not configured - must set up in editor

#### Item 11: Morphs (Complex Struct)
- **Location:** `GasAbilityGeneratorGenerators.cpp:16914`
- **Function:** `FEquippableItemGenerator::Generate()`
- **Current Code:**
  ```cpp
  LogGeneration(FString::Printf(TEXT("    [INFO] Morphs (%d entries) - complex struct, logging for manual setup:"), Definition.ClothingMesh.Morphs.Num()));
  ```
- **Affected Assets:** `EI_*` EquippableItem assets with `clothing_mesh.morphs[]`
- **Manifest Section:** `equippable_items[].clothing_mesh.morphs[]`
- **Narrative Pro Struct:** `FCreatorMeshMorph` containing:
  - `ScalarTag: FGameplayTag`
  - `MorphNames: TArray<FName>`
- **What's Missing:** Morph target validation against skeletal mesh
- **Impact:** Clothing morphs not configured - must set up in editor

#### Item 12: EquipmentAbilities
- **Location:** `GasAbilityGeneratorGenerators.cpp:17025`
- **Function:** `FEquippableItemGenerator::Generate()`
- **Current Code:**
  ```cpp
  LogGeneration(TEXT("  [INFO] EquipmentAbilities property not found - logging for manual setup:"));
  ```
- **Affected Assets:** `EI_*` EquippableItem assets with `equipment_abilities[]`
- **Manifest Section:** `equippable_items[].equipment_abilities[]`
- **Narrative Pro Property:** `UEquippableItem::EquipmentAbilities` (TArray<TSubclassOf<UNarrativeGameplayAbility>>)
- **What's Missing:** Property not found via reflection - may be renamed or on different class
- **Impact:** Equipment abilities not granted when item equipped

#### Item 13: Stats Property
- **Location:** `GasAbilityGeneratorGenerators.cpp:17085`
- **Function:** `FEquippableItemGenerator::Generate()`
- **Current Code:**
  ```cpp
  LogGeneration(TEXT("  [INFO] Stats property not found on UNarrativeItem - logging for manual setup:"));
  ```
- **Affected Assets:** All `EI_*` EquippableItem assets with `stats[]`
- **Manifest Section:** `equippable_items[].stats[]`
- **Narrative Pro Property:** `UNarrativeItem::Stats` (TArray<FNarrativeItemStat>)
- **Manifest Struct:** `{ stat_name, stat_value, stat_icon }`
- **NP Struct:** `{ StatDisplayName (FText), StringVariable, StatTooltip (FText) }`
- **What's Missing:** Property not found + struct mismatch between manifest and Narrative Pro
- **Impact:** Item stats not displayed in UI

#### Item 14: ActivitiesToGrant
- **Location:** `GasAbilityGeneratorGenerators.cpp:17148`
- **Function:** `FEquippableItemGenerator::Generate()`
- **Current Code:**
  ```cpp
  LogGeneration(TEXT("  [INFO] ActivitiesToGrant property not found - logging for manual setup:"));
  ```
- **Affected Assets:** `EI_*` EquippableItem assets with `activities_to_grant[]`
- **Manifest Section:** `equippable_items[].activities_to_grant[]`
- **Narrative Pro Property:** `UNarrativeItem::ActivitiesToGrant` (TArray<TSubclassOf<UNPCActivity>>)
- **What's Missing:** Property not found via reflection
- **Impact:** Activities not granted when item used/equipped

#### Item 15: PickupMeshData
- **Location:** `GasAbilityGeneratorGenerators.cpp:17245`
- **Function:** `FEquippableItemGenerator::Generate()`
- **Current Code:**
  ```cpp
  LogGeneration(TEXT("  [INFO] PickupMeshData property not found - logging for manual setup:"));
  ```
- **Affected Assets:** `EI_*` EquippableItem assets with `pickup_mesh_data`
- **Manifest Section:** `equippable_items[].pickup_mesh_data`
- **Narrative Pro Struct:** `FPickupMeshData` containing:
  - `PickupMesh: TSoftObjectPtr<UStaticMesh>`
  - `PickupMeshMaterials: TArray<TSoftObjectPtr<UMaterialInterface>>`
- **What's Missing:** Property not found on class
- **Impact:** Items don't have world pickup mesh configured

#### Item 16: TraceData (RangedWeaponItem)
- **Location:** `GasAbilityGeneratorGenerators.cpp:17293`
- **Function:** `FEquippableItemGenerator::Generate()`
- **Current Code:**
  ```cpp
  LogGeneration(TEXT("  [INFO] TraceData property not found on class (not a RangedWeaponItem?) - logging for manual setup:"));
  ```
- **Affected Assets:** `EI_*` weapon items with `trace_data`
- **Manifest Section:** `equippable_items[].trace_data`
- **Narrative Pro Struct:** `FCombatTraceData` containing:
  - `TraceDistance: float`
  - `TraceRadius: float`
  - `bTraceMulti: bool`
- **What's Missing:** Property not found - may require RangedWeaponItem parent class
- **Impact:** Ranged weapon trace configuration not set

---

### 1.4 NPC/Character System (3 items)

#### Item 17: NPCTargets Array
- **Location:** `GasAbilityGeneratorGenerators.cpp:18254`
- **Function:** `FNarrativeEventGenerator::Generate()`
- **Current Code:**
  ```cpp
  LogGeneration(FString::Printf(TEXT("  Event '%s' NPCTargets (%d) - property not found, manual setup required: %s"),
      *Definition.Name, Definition.NPCTargets.Num(), *FString::Join(Definition.NPCTargets, TEXT(", "))));
  ```
- **Affected Assets:** `NE_*` NarrativeEvent assets with `npc_targets[]`
- **Manifest Section:** `narrative_events[].npc_targets[]`
- **Narrative Pro Property:** `UNarrativeEvent::NPCTargets` (TArray<TObjectPtr<UNPCDefinition>>)
- **What's Missing:** Property not found via reflection (fallback case)
- **Impact:** Events don't target specific NPCs

#### Item 18: CharacterTargets Array
- **Location:** `GasAbilityGeneratorGenerators.cpp:18304`
- **Function:** `FNarrativeEventGenerator::Generate()`
- **Current Code:**
  ```cpp
  LogGeneration(FString::Printf(TEXT("  Event '%s' CharacterTargets (%d) - manual setup required: %s"),
      *Definition.Name, Definition.CharacterTargets.Num(), *FString::Join(Definition.CharacterTargets, TEXT(", "))));
  ```
- **Affected Assets:** `NE_*` NarrativeEvent assets with `character_targets[]`
- **Manifest Section:** `narrative_events[].character_targets[]`
- **Narrative Pro Property:** `UNarrativeEvent::CharacterTargets` (TArray<TObjectPtr<UCharacterDefinition>>)
- **What's Missing:** Property not found via reflection
- **Impact:** Events don't target specific characters

#### Item 19: NPC Blueprint Assignment
- **Location:** `NPCTableValidator.cpp:173`
- **Function:** `FNPCTableValidator::ValidateRow()`
- **Current Code:**
  ```cpp
  if (Row.Blueprint.IsNull())
  {
      Issues.Add(FNPCValidationIssue(
          ENPCValidationSeverity::Warning,
          DisplayName,
          TEXT("Blueprint"),
          TEXT("No NPC Blueprint assigned - NPC will need manual setup")
      ));
  }
  ```
- **Affected Assets:** All `NPC_*` NPCDefinition assets created via NPC Table
- **What's Missing:** Auto-creation or auto-linking of BP_NPC_{Name} blueprints
- **Impact:** NPCs have no visual representation until blueprint manually assigned

---

### 1.5 AI/Goals System (1 item)

#### Item 20: Goal Class Resolution
- **Location:** `GasAbilityGeneratorGenerators.cpp:21393`
- **Function:** `FActivityScheduleGenerator::Generate()`
- **Current Code:**
  ```cpp
  LogGeneration(FString::Printf(TEXT("  [WARNING] Could not resolve goal class: %s - behavior will need manual setup"), *Behavior.GoalClass));
  ```
- **Affected Assets:** `Schedule_*` ActivitySchedule assets with behaviors referencing goals
- **Manifest Section:** `activity_schedules[].behaviors[].goal_class`
- **Searched Paths:** 11 hardcoded paths in NarrativePro and project
- **What's Missing:** Goal class not found at any search path
- **Impact:** Scheduled behaviors skip goal assignment - NPC doesn't pursue goal

---

## SECTION 2: Documented Limitations (10 items)

### 2.1 Property/Type Limitations (5 items)

#### Item 21: Nested Dots in Property Paths
- **Location:** `GasAbilityGeneratorGenerators.cpp:3667` (approx line 3753)
- **Function:** `ConfigureWidgetProperties()` lambda
- **Limitation:** Only 1-level dotted paths supported (e.g., `Font.Size`)
- **Rejected:** Multi-level paths like `Font.Typeface.Name`
- **Affected Assets:** Widget blueprints with deeply nested property paths
- **Manifest Section:** `widget_blueprints[].widget_tree.widgets[].properties`
- **Error Code:** `W_NESTED_TOO_DEEP`

#### Item 22: MediaTexture Type
- **Location:** `GasAbilityGeneratorGenerators.cpp:3724` (approx line 3805)
- **Function:** `ConfigureWidgetProperties()` lambda
- **Limitation:** MediaTexture assets blocked for `Brush.ResourceObject`
- **Reason:** No MediaAssets module dependency
- **Affected Assets:** Widget blueprints using video textures
- **Error Code:** `W_TEXTURE_TYPE_BLOCKED`

#### Item 23: Nested Struct Types
- **Location:** `GasAbilityGeneratorGenerators.cpp:3881` (approx line 3913)
- **Function:** `ConfigureWidgetProperties()` lambda
- **Limitation:** Only 3 struct types supported: `FLinearColor`, `FVector2D`, `FSlateColor`
- **Rejected:** All other struct types (FTransform, FRotator, custom structs)
- **Affected Assets:** Widget blueprints with complex nested struct properties
- **Error Code:** `W_UNSUPPORTED_TYPE`

#### Item 24: Root-Level Struct Types
- **Location:** `GasAbilityGeneratorGenerators.cpp:3992` (approx line 4024)
- **Function:** `ConfigureWidgetProperties()` lambda
- **Limitation:** Same as nested - only 3 struct types supported at root level
- **Affected Assets:** Widget blueprints with direct struct properties
- **Error Code:** `W_UNSUPPORTED_TYPE`

#### Item 25: UMaterialFunctionInstance
- **Location:** `GasAbilityGeneratorGenerators.cpp:6401` (approx line 6892)
- **Function:** Material expression validation
- **Limitation:** Material Function Instances blocked entirely
- **Reason:** Cannot inline instance parameters into expression graph
- **Affected Assets:** Materials referencing `*_Inst` function variants
- **Error Code:** `E_FUNCTION_INSTANCE_BLOCKED`

---

### 2.2 Unsupported Node Types (3 items)

#### Item 26: Unknown Event Graph Node Types
- **Location:** `GasAbilityGeneratorGenerators.cpp:8200` (approx line 8929)
- **Function:** `GenerateEventGraph()`
- **Supported Types (19):** Event, CustomEvent, CallFunction, Branch, VariableGet, VariableSet, Sequence, Delay, AbilityTaskWaitDelay, PrintString, DynamicCast, ForEachLoop, SpawnActor, PropertyGet, PropertySet, BreakStruct, MakeArray, GetArrayItem, Self
- **Unsupported:** IsValid, DoOnce, MultiGate, FlipFlop, Timeline, Reroute, Comment, etc.
- **Affected Assets:** All Blueprint assets using unsupported node types in event_graph

#### Item 27: Unknown Function Override Types
- **Location:** `GasAbilityGeneratorGenerators.cpp:9081` (approx line 9539)
- **Function:** `GenerateFunctionOverride()`
- **Supported Types (8):** CallParent, CallFunction, Branch, VariableGet, VariableSet, Sequence, PrintString, Self
- **Missing vs Event Graph:** DynamicCast, ForEachLoop, SpawnActor, PropertyGet/Set, BreakStruct, MakeArray, GetArrayItem, Delay
- **Affected Assets:** Blueprints with function overrides using missing node types

#### Item 28: Unknown Custom Function Types
- **Location:** `GasAbilityGeneratorGenerators.cpp:9320` (approx line 9784)
- **Function:** `GenerateCustomFunction()`
- **Supported Types (8):** CallFunction, Branch, VariableGet, VariableSet, Sequence, PrintString, Self, DynamicCast
- **Missing vs Event Graph:** Event, CustomEvent, ForEachLoop, SpawnActor, PropertyGet/Set, BreakStruct, MakeArray, GetArrayItem, Delay
- **Affected Assets:** Blueprints with custom functions using missing node types

---

### 2.3 Token Registry Limitations (2 items)

#### Item 29: Unknown Event Types
- **Location:** `DialogueTokenRegistry.cpp:1178-1179`
- **Function:** `FDialogueTokenRegistry::SerializeEvent()`
- **Current Behavior:** Returns `UNSUPPORTED(ClassName)` for unknown events
- **Supported Events (19):** NE_BeginQuest, NE_GiveItem, NE_RemoveItem, NE_AddTag, NE_RemoveTag, NE_GiveXP, NE_AddCurrency, NE_BeginDialogue, NE_RestartQuest, NE_StartTrading, NE_AddSaveCheckpoint, NE_ShowNotification, NE_PrintString, NE_RemoveFollowGoal, NE_AddFaction, NE_RemoveFaction, NE_SetFactionAttitude, NE_AddGoal, NE_FailQuest
- **Affected Assets:** Dialogues with custom/unknown event types

#### Item 30: Unsupported Property Types
- **Location:** `DialogueTokenRegistry.cpp:1534`
- **Function:** `FDialogueTokenRegistry::SetPropertyFromParam()`
- **Supported Types (9):** FIntProperty, FFloatProperty, FBoolProperty, FStrProperty, FNameProperty, FSoftObjectProperty, FSoftClassProperty, FObjectProperty, FStructProperty (only FGameplayTagContainer)
- **Unsupported:** FVector, FRotator, FLinearColor, TArray<>, TMap<>, FText, custom structs
- **Affected Assets:** Dialogue events/conditions with unsupported property types

---

## SECTION 3: Acceptable Placeholders (3 items)

#### Item 31: Sound Wave Placeholder
- **Location:** `GasAbilityGeneratorWindow.cpp:1630`
- **Function:** NPC Definition creation from Window UI
- **Current Code:**
  ```cpp
  AppendLog(FString::Printf(TEXT("[INFO] %s_Voice (USoundWave) - Import audio manually"), *NPCName));
  ```
- **Reason:** Audio assets require external import from audio files
- **Impact:** NPCs don't have voice audio assigned
- **Recommendation:** Remove feature or add audio file import support

#### Item 32: Level Sequence Placeholder
- **Location:** `GasAbilityGeneratorWindow.cpp:1633`
- **Function:** NPC Definition creation from Window UI
- **Current Code:** Similar placeholder for level sequences
- **Reason:** Level sequences require Sequencer editor
- **Impact:** NPCs don't have associated cinematics
- **Recommendation:** Remove feature or add sequence generation

#### Item 33: Placeholder Rows for Empty Dialogues
- **Location:** `SDialogueTableEditor.cpp:3415-3429`
- **Function:** Dialogue table editor display
- **Reason:** Editor UX - shows placeholder rows for empty dialogue trees
- **Impact:** None (editor only, not generation)
- **Recommendation:** Keep as-is (intentional UX feature)

---

## Summary by Generator

| Generator | Silent Failures | Items |
|-----------|-----------------|-------|
| FDialogueBlueprintGenerator | 4 | Items 1-5 |
| FQuestGenerator | 4 | Items 6-9 |
| FEquippableItemGenerator | 7 | Items 10-16 |
| FNarrativeEventGenerator | 2 | Items 17-18 |
| FActivityScheduleGenerator | 1 | Item 20 |
| Widget property system | 4 | Items 21-24 |
| Material validation | 1 | Item 25 |
| Event graph generation | 3 | Items 26-28 |
| Token registry | 2 | Items 29-30 |
| Editor/Window | 3 | Items 31-33 |

---

## Action Required

**All 30 generator items (excluding 3 placeholders) must be converted from silent failures to hard fails with error codes.**

See `Fail_Fast_Audit_v1.md` for the specific error codes and conversion patterns.
