# Fail-Fast Audit v2.1 - Anchored Classification

**Created:** 2026-01-22
**Status:** SINGLE SOURCE OF TRUTH FOR PHASE 2
**Scope:** Generation + Sync Pipelines Only (Option 1)
**Audit:** Claude-GPT Dual Audit COMPLETE

---

## Column Definitions

| Column | Description |
|--------|-------------|
| **#** | Item identifier |
| **Anchor** | `FunctionName \| "unique log substring"` - stable identifier |
| **Subsystem** | GAS / NPC / Dialogue / Quest / BT / Material / Niagara / Token / Item / General |
| **Type** | **M** = Masking (HARD FAIL), **D** = Default (Allowed if documented) |
| **Scope** | **Pipeline** = Enforced, **EditorUX** = Not enforced |
| **ManifestRef** | YES = manifest explicitly references, NO = optional/omitted |

---

## Classification Rules (LOCKED)

| Class | Condition | Type | Action |
|-------|-----------|------|--------|
| R1 | Manifest references X → X unresolvable | M | HARD FAIL |
| R2 | Manifest omits optional X → uses default | D | ALLOWED |
| R3 | Structural invalidity | M | HARD FAIL |

---

## Category A: [INFO] Manual Setup Fallbacks (12 items)

| # | Anchor | Subsystem | Type | Scope | ManifestRef |
|---|--------|-----------|------|-------|-------------|
| A1 | `FDialogueBlueprintGenerator::Generate \| "PartySpeakerInfo property not found"` | Dialogue | M | Pipeline | YES |
| A2 | `FEquippableItemGenerator::Generate \| "MeshMaterials - complex struct, logging for manual"` | Item | M | Pipeline | YES |
| A3 | `FEquippableItemGenerator::Generate \| "Morphs - complex struct, logging for manual"` | Item | M | Pipeline | YES |
| A4 | `FEquippableItemGenerator::Generate \| "Logging values for manual setup"` | Item | M | Pipeline | YES |
| A5 | `FEquippableItemGenerator::Generate \| "EquipmentAbilities property not found"` | Item | M | Pipeline | YES |
| A6 | `FEquippableItemGenerator::Generate \| "Stats property not found on UNarrativeItem"` | Item | M | Pipeline | YES |
| A7 | `FEquippableItemGenerator::Generate \| "ActivitiesToGrant property not found"` | Item | M | Pipeline | YES |
| A8 | `FEquippableItemGenerator::Generate \| "PickupMeshData property not found"` | Item | M | Pipeline | YES |
| A9 | `FEquippableItemGenerator::Generate \| "TraceData property not found"` | Item | M | Pipeline | YES |
| A10 | `FNarrativeEventGenerator::Generate \| "NPCTargets - property not found, manual setup"` | NPC | M | Pipeline | YES |
| A11 | `FNarrativeEventGenerator::Generate \| "CharacterTargets - manual setup required"` | NPC | M | Pipeline | YES |
| A12 | `FQuestGenerator::Generate \| "Questgiver (manual setup)"` | Quest | M | Pipeline | YES |

**Note:** A1, A5-A11 are the 8 PROVEN GENERATOR BUGS - properties exist in NP source.

---

## Category B: [WARN] Missing Class/Asset (28 items)

| # | Anchor | Subsystem | Type | Scope | ManifestRef |
|---|--------|-----------|------|-------|-------------|
| B1 | `FBehaviorTreeGenerator::Generate \| "Task class not found: %s, using BTTask_Wait"` | BT | M | Pipeline | YES |
| B2 | `CreateCallFunctionNode \| "Could not find enum value"` | General | M | Pipeline | YES |
| B3 | `CreateCallFunctionNode \| "Could not resolve class '%s' for TSubclassOf"` | General | M | Pipeline | YES |
| B4 | `CreateCallFunctionNode \| "Could not find input pin '%s' on function"` | General | M | Pipeline | YES |
| B5 | `CreateCallFunctionNode \| "Could not find a function named"` | General | M | Pipeline | YES |
| B6 | `CreateVariableGetNode \| "Could not resolve class for target '%s', falling back to self"` | General | M | Pipeline | YES |
| B7 | `CreateVariableSetNode \| "Could not resolve class for target '%s', falling back to self"` | General | M | Pipeline | YES |
| B8 | `CreatePropertyGetNode \| "Could not find class '%s'"` | General | M | Pipeline | YES |
| B9 | `CreatePropertySetNode \| "Could not find class '%s'"` | General | M | Pipeline | YES |
| B10 | `CreateBreakStructNode \| "Could not find struct '%s'"` | General | M | Pipeline | YES |
| B11 | `CreateDynamicCastNode \| "could not find target class '%s'"` | General | M | Pipeline | YES |
| B12 | `CreateAbilityTaskWaitAttributeChangeNode \| "Could not find Attribute pin"` | GAS | M | Pipeline | YES |
| B13 | `CreateAbilityTaskWaitAttributeChangeNode \| "Could not find OnChange pin"` | GAS | M | Pipeline | YES |
| B14 | `FDialogueBlueprintGenerator \| "Could not find dialogue event class"` | Dialogue | M | Pipeline | YES |
| B15 | `FDialogueBlueprintGenerator \| "Could not find dialogue condition class"` | Dialogue | M | Pipeline | YES |
| B16 | `FDialogueBlueprintGenerator \| "Could not find NPCDefinition"` | Dialogue | M | Pipeline | YES |
| B17 | `FEquippableItemGenerator \| "ClothingMeshData property not found"` | Item | M | Pipeline | YES |
| B18 | `FEquippableItemGenerator \| "EquipmentEffectValues property not found"` | Item | M | Pipeline | YES |
| B19 | `FEquippableItemGenerator \| "Could not resolve EquipmentAbility class"` | Item | M | Pipeline | YES |
| B20 | `FEquippableItemGenerator \| "Could not resolve Activity class"` | Item | M | Pipeline | YES |
| B21 | `FEquippableItemGenerator \| "Could not load ItemWidgetOverride class"` | Item | M | Pipeline | YES |
| B22 | `FAbilityConfigurationGenerator \| "Could not resolve ability"` | GAS | M | Pipeline | YES |
| B23 | `FAbilityConfigurationGenerator \| "Could not resolve startup effect"` | GAS | M | Pipeline | YES |
| B24 | `FAbilityConfigurationGenerator \| "Could not resolve default attributes"` | GAS | M | Pipeline | YES |
| B25 | `FActivityConfigurationGenerator \| "Could not resolve activity"` | GAS | M | Pipeline | YES |
| B26 | `FActivityConfigurationGenerator \| "Could not resolve goal generator"` | GAS | M | Pipeline | YES |
| B27 | `FActivityScheduleGenerator \| "Could not resolve goal class - behavior will need manual setup"` | NPC | M | Pipeline | YES |
| B28 | `FNPCDefinitionGenerator \| "Could not resolve dialogue class"` | NPC | M | Pipeline | YES |

---

## Category C: [WARN] Reward Class Not Found (3 items)

| # | Anchor | Subsystem | Type | Scope | ManifestRef |
|---|--------|-----------|------|-------|-------------|
| C1 | `FQuestGenerator \| "Could not load NE_GiveXP class - XP reward requires manual"` | Quest | M | Pipeline | YES |
| C2 | `FQuestGenerator \| "Could not load BPE_AddCurrency class - Currency reward requires manual"` | Quest | M | Pipeline | YES |
| C3 | `FQuestGenerator \| "Could not load BPE_AddItemToInventory class - Item reward requires manual"` | Quest | M | Pipeline | YES |

---

## Category D: Comment-Only Deferred (4 items)

| # | Anchor | Subsystem | Type | Scope | ManifestRef |
|---|--------|-----------|------|-------|-------------|
| D1 | `FDialogueTableConverter \| "For now, leave it empty - manual setup"` | Dialogue | M | Pipeline | YES |
| D2 | `FDialogueBlueprintGenerator \| "Log info for manual setup via BPT_FinishDialogue"` | Dialogue | M | Pipeline | YES |
| D3 | `FDialogueBlueprintGenerator \| "Log info for manual setup via Blueprint call to FailQuest"` | Quest | M | Pipeline | YES |
| D4 | `FDialogueBlueprintGenerator \| "DefaultDialogueShot with sequence assets requires manual"` | Dialogue | M | Pipeline | YES |

---

## Category S: Secondary Warnings (51 items)

### S1-S28: Original Items

| # | Anchor | Subsystem | Type | Scope | ManifestRef |
|---|--------|-----------|------|-------|-------------|
| S1 | `FActorBlueprintGenerator \| "Component class not found"` | General | M | Pipeline | YES |
| S2 | `FAnimationMontageGenerator \| "Could not load skeleton - creating empty montage"` | General | M | Pipeline | YES |
| S3 | `FDialogueBlueprintGenerator \| "Could not load dialogue audio"` | Dialogue | M | Pipeline | YES |
| S4 | `FDialogueBlueprintGenerator \| "Could not find NPC node for reply"` | Dialogue | M | Pipeline | YES |
| S5 | `FDialogueBlueprintGenerator \| "Could not find Player node for reply"` | Dialogue | M | Pipeline | YES |
| S6 | `FDialogueBlueprintGenerator \| "Could not load camera shake class"` | Dialogue | M | Pipeline | YES |
| S7 | `FDialogueBlueprintGenerator \| "Could not load sound attenuation"` | Dialogue | M | Pipeline | YES |
| S8 | `FDialogueBlueprintGenerator \| "Could not load dialogue sequence class"` | Dialogue | M | Pipeline | YES |
| S9 | `FGameplayCueGenerator \| "Could not load condition class"` | GAS | M | Pipeline | YES |
| S10 | `FGameplayCueGenerator \| "Could not find Conditions array property"` | GAS | M | Pipeline | YES |
| S11 | `FGameplayCueGenerator \| "Could not load ParticleSystem"` | Niagara | M | Pipeline | YES |
| S12 | `FGameplayCueGenerator \| "Could not load Sound"` | GAS | M | Pipeline | YES |
| S13 | `FGameplayCueGenerator \| "Could not load CameraShake class"` | GAS | M | Pipeline | YES |
| S14 | `FNPCDefinitionGenerator \| "ActivitySchedules property not found"` | NPC | M | Pipeline | YES |
| S15 | `FNPCDefinitionGenerator \| "Trigger class not found"` | NPC | M | Pipeline | YES |
| S16 | `FNPCDefinitionGenerator \| "Failed to create trigger instance"` | NPC | M | Pipeline | YES |
| S17 | `FNPCDefinitionGenerator \| "Event class not found"` | NPC | M | Pipeline | YES |
| S18 | `FNPCDefinitionGenerator \| "Failed to create event instance"` | NPC | M | Pipeline | YES |
| S19 | `FNPCDefinitionGenerator \| "Event property not found"` | NPC | M | Pipeline | YES |
| S20 | `FGoalItemGenerator \| "Could not find parent class, using UNPCGoalItem"` | NPC | M | Pipeline | YES |
| S21 | `FQuestGenerator \| "Could not load requirement class"` | Quest | M | Pipeline | YES |
| S22 | `FQuestGenerator \| "Could not find QuestRequirements array property"` | Quest | M | Pipeline | YES |
| S23 | `FQuestGenerator \| "Could not resolve task class"` | Quest | M | Pipeline | YES |
| S24 | `FPOIGenerator \| "Failed to load POI icon"` | NPC | M | Pipeline | YES |
| S25 | `FNPCSpawnerGenerator \| "Failed to create NPC spawn component"` | NPC | M | Pipeline | YES |
| S26 | `FNPCSpawnerGenerator \| "Could not resolve POI location, using origin"` | NPC | M | Pipeline | YES |
| S27 | `FNPCSpawnerGenerator \| "Could not load NPCDefinition"` | NPC | M | Pipeline | YES |
| S28 | `FNPCSpawnerGenerator \| "Could not load goal class"` | NPC | M | Pipeline | YES |

### S29-S47: New Discoveries

| # | Anchor | Subsystem | Type | Scope | ManifestRef |
|---|--------|-----------|------|-------|-------------|
| S29 | `FGameplayAbilityGenerator \| "CooldownGE not found"` | GAS | M | Pipeline | YES |
| S30 | `GetPinTypeFromString \| "Class '%s' not found, using AActor as fallback"` | General | M | Pipeline | YES |
| S31 | `FGameplayEffectGenerator \| "Property '%s' not found on class"` | GAS | M | Pipeline | YES |
| S32 | `FGameplayEffectGenerator \| "Attribute set class '%s' not found"` | GAS | M | Pipeline | YES |
| S33 | `FGameplayEffectGenerator \| "SetByCaller tag '%s' not found"` | GAS | M | Pipeline | YES |
| S34 | `FGameplayEffectGenerator \| "Tag '%s' not found in GameplayTags"` | GAS | M | Pipeline | YES |
| S35 | `FGameplayAbilityGenerator \| "Failed to generate custom function"` | GAS | M | Pipeline | YES |
| S36 | `FActorBlueprintGenerator \| "Failed to generate function override"` | General | M | Pipeline | YES |
| S37 | `FBlackboardGenerator \| "Parent blackboard '%s' not found"` | BT | M | Pipeline | YES |
| S38 | `FBehaviorTreeGenerator \| "Blackboard not found"` | BT | M | Pipeline | YES |
| S39 | `FBehaviorTreeGenerator \| "Parent node '%s' not found or not composite"` | BT | M | Pipeline | YES |
| S40 | `FBehaviorTreeGenerator \| "Child node '%s' not found for parent"` | BT | M | Pipeline | YES |
| S41 | `FBehaviorTreeGenerator \| "Failed to create UBehaviorTreeGraph"` | BT | M | Pipeline | YES |
| S42 | `FMaterialGenerator \| "MaterialFunctionCall '%s' -> function NOT FOUND"` | Material | M | Pipeline | YES |
| S43 | `FMaterialFunctionGenerator \| "MF connection source '%s' not found"` | Material | M | Pipeline | YES |
| S44 | `FMaterialFunctionGenerator \| "MF connection target '%s' not found"` | Material | M | Pipeline | YES |
| S45 | `FMaterialInstanceGenerator \| "Texture not found for param"` | Material | M | Pipeline | YES |
| S46 | `CreateForEachLoopNode \| "ForEachLoop macro not found"` | General | M | Pipeline | YES |
| S47 | `FQuestGenerator \| "Destination state not found"` | Quest | M | Pipeline | YES |

### S48-S51: Quest State Machine (R3 - Structural Invalidity)

| # | Anchor | Subsystem | Type | Scope | ManifestRef |
|---|--------|-----------|------|-------|-------------|
| S48 | `FQuestGenerator \| "Quest has no valid start state"` | Quest | M | Pipeline | YES |
| S49 | `FQuestGenerator \| "Duplicate state ID"` | Quest | M | Pipeline | YES |
| S50 | `FQuestGenerator \| "Duplicate branch ID"` | Quest | M | Pipeline | YES |
| S51 | `FPOIGenerator \| "POI tag not registered"` | NPC | M | Pipeline | YES |

---

## Category F: Fallback Patterns (12 items)

| # | Anchor | Subsystem | Type | Scope | ManifestRef |
|---|--------|-----------|------|-------|-------------|
| F1 | `GetPinTypeFromString \| "using AActor as fallback"` | General | M | Pipeline | YES |
| F2 | `FWidgetBlueprintGenerator \| "Fallback to UUserWidget"` | General | M | Pipeline | YES |
| F3 | `FBehaviorTreeGenerator \| "using BTTask_Wait"` | BT | M | Pipeline | YES |
| F4 | `CreateCallFunctionNode \| "Still create the node with fallback"` | General | M | Pipeline | YES |
| F5 | `CreateVariableGetNode \| "falling back to self"` | General | M | Pipeline | YES |
| F6 | `CreateVariableSetNode \| "falling back to self"` | General | M | Pipeline | YES |
| F7 | `GetPinPosition \| "Pin not found, return middle of node as fallback"` | General | M | Pipeline | YES |
| F8 | `FAnimationMontageGenerator \| "creating empty montage"` | General | M | Pipeline | YES |
| F9 | `FEquippableItemGenerator \| "Fallback to UEquippableItem"` | Item | M | Pipeline | YES |
| F10 | `FGameplayCueGenerator \| "Default fallback - simple sprite burst"` | GAS | D | Pipeline | NO |
| F11 | `FGoalItemGenerator \| "using UNPCGoalItem"` | NPC | M | Pipeline | YES |
| F12 | `FNPCSpawnerGenerator \| "using origin"` | NPC | M | Pipeline | YES |

**Note:** F10 is Type D (Default) - manifest did not specify cue effects, uses default sprite burst.

---

## Category T: Token Registry (2 items)

| # | Anchor | Subsystem | Type | Scope | ManifestRef |
|---|--------|-----------|------|-------|-------------|
| T1 | `SerializeEvent \| "UNSUPPORTED(%s)"` | Token | M | Pipeline | YES |
| T2 | `SetPropertyFromParam \| "Unsupported property type"` | Token | M | Pipeline | YES |

---

## Category W: Window/Editor (6 items)

| # | Anchor | Subsystem | Type | Scope | ManifestRef |
|---|--------|-----------|------|-------|-------------|
| W1 | `SGasAbilityGeneratorWindow \| "Sound Wave - Import audio manually"` | General | D | EditorUX | NO |
| W2 | `SGasAbilityGeneratorWindow \| "AC_NPC_Default not found"` | NPC | D | EditorUX | NO |
| W3 | `SGasAbilityGeneratorWindow \| "DefaultItemLoadout property not found"` | NPC | D | EditorUX | NO |
| W4 | `SGasAbilityGeneratorWindow \| "AC_RunAndGun not found"` | NPC | D | EditorUX | NO |
| W5 | `SGasAbilityGeneratorWindow \| "Apperance_Manny not found"` | NPC | D | EditorUX | NO |
| W6 | `SGasAbilityGeneratorWindow \| "BP_NarrativeNPCCharacter not found"` | NPC | D | EditorUX | NO |

**Note:** W1-W6 are Scope=EditorUX, Type=D - NOT enforced under Option 1.

---

## 8 Generator Bugs - NP Header Proof

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

**Phase 2 Scope:** 111 Type M items in Pipeline scope will be converted to HARD FAIL.

---

## Document History

| Version | Date | Changes |
|---------|------|---------|
| v1.0 | 2026-01-22 | Initial audit - 87 items |
| v2.0 | 2026-01-22 | Expanded to 114 items, line corrections |
| v2.1 | 2026-01-22 | Anchored format, Type/Scope classification, NP proof excerpts |
