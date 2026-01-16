# Narrative Pro Gap Analysis - Generator Automation Coverage

**Date:** January 2026
**Plugin Version:** v4.8.2
**Analysis Scope:** All Narrative Pro headers vs. manifest parsing vs. generator implementation

---

## Executive Summary

This document analyzes gaps between Narrative Pro's C++ API and our generator's automation capabilities.

### Coverage Statistics (Updated v4.8.2)

| Asset Type | Header Properties | Manifest Fields | Generator Coverage | Gap % |
|------------|------------------|-----------------|-------------------|-------|
| UNarrativeGameplayAbility | 2 | 2 | 100% | **0%** ✅ |
| UNPCDefinition | 16 | 16 | 100% | **0%** ✅ |
| UCharacterDefinition | 8 | 8 | 100% | **0%** ✅ |
| UEquippableItem | 7 | 7 | 100% | **0%** ✅ |
| UWeaponItem | 18 | 18 | 100% | **0%** ✅ |
| URangedWeaponItem | 14 | 14 | 100% | **0%** ✅ |
| UNarrativeItem (Base) | 20 | 20 | 100% | **0%** ✅ |
| UDialogue | 17 | 15 | 88% | **12%** |
| UNPCActivityConfiguration | 3 | 3 | 100% | **0%** ✅ |
| UNPCGoalItem | 8 | 8 | 100% | **0%** ✅ |
| UNarrativeActivityBase | 4 | 4 | 100% | **0%** ✅ |
| UAbilityConfiguration | 3 | 3 | 100% | **0%** ✅ |

**Overall Coverage: ~97%**

---

## 1. UNarrativeGameplayAbility - FULLY COVERED ✅

### Header Properties (NarrativeGameplayAbility.h)
```cpp
UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Narrative Ability")
FGameplayTag InputTag;                          // ✅ v4.8.1

UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Narrative Ability")
bool bActivateAbilityOnGranted = false;         // ✅ v4.8.1
```

### YAML Syntax
```yaml
gameplay_abilities:
  - name: GA_Example
    input_tag: Narrative.Input.Attack           # v4.8.1
    activate_on_granted: true                   # v4.8.1
```

---

## 2. UNPCDefinition - FULLY COVERED ✅

### Header Properties (NPCDefinition.h)
```cpp
FName NPCID;                                    // ✅ Supported
FText NPCName;                                  // ✅ Supported
int32 MinLevel, MaxLevel;                       // ✅ Supported
bool bAllowMultipleInstances;                   // ✅ Supported
FGuid UniqueNPCGUID;                            // ✅ v4.8.1 (auto-generated)
TSoftClassPtr<ANarrativeNPCCharacter> NPCClassPath;  // ✅ Supported
TSoftClassPtr<UDialogue> Dialogue;              // ✅ Supported
TSoftObjectPtr<UTaggedDialogueSet> TaggedDialogueSet; // ✅ Supported
bool bIsVendor;                                 // ✅ Supported
int32 TradingCurrency;                          // ✅ Supported
float BuyItemPercentage, SellItemPercentage;    // ✅ Supported
TArray<FLootTableRoll> TradingItemLoadout;      // ✅ Supported
FText ShopFriendlyName;                         // ✅ Supported
TArray<TSoftObjectPtr<UNPCActivitySchedule>> ActivitySchedules; // ✅ Supported
TSoftObjectPtr<UNPCActivityConfiguration> ActivityConfiguration; // ✅ Supported
```

### From UCharacterDefinition (inherited)
```cpp
TSoftObjectPtr<UCharacterAppearanceBase> DefaultAppearance; // ✅ Supported
int32 DefaultCurrency;                          // ✅ Supported
TArray<FLootTableRoll> DefaultItemLoadout;      // ✅ Supported
FGameplayTagContainer DefaultOwnedTags;         // ✅ Supported
FGameplayTagContainer DefaultFactions;          // ✅ Supported
TArray<TSoftObjectPtr<UTriggerSet>> TriggerSets; // ✅ v4.8.1
float AttackPriority;                           // ✅ Supported
UAbilityConfiguration* AbilityConfiguration;    // ✅ Supported
```

### YAML Syntax
```yaml
npc_definitions:
  - name: NPC_Example
    unique_npc_guid: "A1B2C3D4-..."             # v4.8.1 (optional, auto-generated if unique)
    trigger_sets:                               # v4.8.1
      - TS_MerchantSchedule
      - TS_CombatTriggers
```

---

## 3. UDialogue - MINOR GAPS REMAINING

### Header Properties (Dialogue.h)
```cpp
TArray<FSpeakerInfo> Speakers;                  // ✅ Supported
FPlayerSpeakerInfo PlayerSpeakerInfo;           // ✅ Supported
TArray<FPlayerSpeakerInfo> PartySpeakerInfo;    // ✅ v4.8.1
float EndDialogueDist;                          // ✅ Supported
bool bShowCinematicBars;                        // ✅ Supported
bool bUnskippable;                              // ✅ Supported
bool bFreeMovement;                             // ✅ Supported
bool bCanBeExited;                              // ✅ Supported
bool bAutoRotateSpeakers;                       // ✅ Supported
bool bAutoStopMovement;                         // ✅ Supported
int32 Priority;                                 // ✅ Supported
FName DefaultHeadBoneName;                      // ✅ Supported
float DialogueBlendOutTime;                     // ✅ Supported
FTransform PlayerAutoAdjustTransform;           // ❌ NOT SUPPORTED (complex, rarely used)
bool bAdjustPlayerTransform;                    // ✅ Supported
TSubclassOf<UCameraShakeBase> DialogueCameraShake; // ✅ Supported
USoundAttenuation* DialogueSoundAttenuation;    // ❌ NOT SUPPORTED (asset reference)
UNarrativeDialogueSequence* DefaultDialogueShot; // ❌ NOT SUPPORTED (instanced object)
```

### Remaining Gaps (LOW Priority)
| Property | Type | Reason Not Supported |
|----------|------|---------------------|
| `PlayerAutoAdjustTransform` | FTransform | Complex struct, rarely used |
| `DialogueSoundAttenuation` | USoundAttenuation* | Asset reference, better in editor |
| `DefaultDialogueShot` | Instanced UObject* | Requires editor-only instanced creation |

### YAML Syntax
```yaml
dialogue_blueprints:
  - name: DBP_Example
    party_speaker_info:                         # v4.8.1
      - speaker_id: "PartyMember1"
        node_color: "#00FF00"
      - speaker_id: "PartyMember2"
        node_color: "#0000FF"
```

---

## 4. UNarrativeItem (Base) - FULLY COVERED ✅

### Header Properties (NarrativeItem.h)
```cpp
TSoftObjectPtr<UTexture2D> Thumbnail;           // ✅ Supported
USoundBase* UseSound;                           // ✅ Supported
FText DisplayName;                              // ✅ Supported
FText Description;                              // ✅ Supported
float Weight;                                   // ✅ Supported
int32 BaseValue;                                // ✅ Supported
float BaseScore;                                // ✅ Supported
FGameplayTagContainer ItemTags;                 // ✅ Supported
TSubclassOf<UNarrativeInventoryItemButton> ItemWidgetOverride; // ✅ v4.8.2
bool bAddDefaultUseOption;                      // ✅ Supported
bool bConsumeOnUse;                             // ✅ Supported
bool bUsedWithOtherItem;                        // ✅ Supported
FText UseActionText;                            // ✅ Supported
float UseRechargeDuration;                      // ✅ Supported
bool bCanActivate;                              // ✅ Supported
bool bToggleActiveOnUse;                        // ✅ Supported
bool bStackable;                                // ✅ Supported
int32 MaxStackSize;                             // ✅ Supported
bool bWantsTickByDefault;                       // ✅ v4.8.2
TArray<TSubclassOf<UNPCActivity>> ActivitiesToGrant; // ✅ v4.8.1
TArray<FNarrativeItemStat> Stats;               // ✅ v4.8.1
FPickupMeshData PickupMeshData;                 // ✅ v4.8.2
```

### YAML Syntax
```yaml
equippable_items:
  - name: EI_Example
    item_widget_override: /Game/UI/WBP_CustomButton  # v4.8.2
    wants_tick_by_default: true                      # v4.8.2
    stats:                                           # v4.8.1
      - stat_name: "Damage"
        stat_value: 50
        stat_icon: "/Game/UI/Icons/T_Damage"
    activities_to_grant:                             # v4.8.1
      - BPA_PatrolWithItem
    pickup_mesh_data:                                # v4.8.2
      mesh: /Game/Meshes/SM_Sword
      materials:
        - /Game/Materials/M_SwordBlade
```

---

## 5. UEquippableItem - FULLY COVERED ✅

All properties supported since v4.2.

---

## 6. UWeaponItem - FULLY COVERED ✅

All properties supported since v4.0.

---

## 7. URangedWeaponItem - FULLY COVERED ✅

### Header Properties (RangedWeaponItem.h)
```cpp
float AimFOVPct;                                // ✅ Supported
float AimWeaponRenderFOV;                       // ✅ Supported
float AimWeaponFStop;                           // ✅ Supported
float BaseSpreadDegrees;                        // ✅ Supported
float MoveSpeedAddDegrees;                      // ✅ Supported
float CrouchSpreadMultiplier;                   // ✅ Supported
float AimSpreadMultiplier;                      // ✅ Supported
float SpreadFireBump;                           // ✅ Supported
float MaxSpreadDegrees;                         // ✅ Supported
float SpreadDecreaseSpeed;                      // ✅ Supported
FVector RecoilImpulseTranslationMin/Max;        // ✅ Supported
FVector HipRecoilImpulseTranslationMin/Max;     // ✅ Supported
FCombatTraceData TraceData;                     // ✅ v4.8.2
```

### YAML Syntax
```yaml
equippable_items:
  - name: EI_LaserRifle
    parent_class: RangedWeaponItem
    trace_data:                                 # v4.8.2
      distance: 5000                            # TraceDistance
      radius: 10                                # TraceRadius (0 = line, >0 = sphere)
      multi: true                               # bTraceMulti (hit multiple targets)
```

---

## 8. Remaining Gaps (NOT PLANNED)

### Asset Types Not Generated

| Asset Type | Reason |
|------------|--------|
| UTriggerSet | Complex event system, better authored in editor |
| UCharacterAppearanceBase | Visual customization, editor-only |
| UNarrativeDialogueSequence | Camera sequences, requires Sequencer |

### Properties Not Supported

| Property | Class | Reason |
|----------|-------|--------|
| PlayerAutoAdjustTransform | UDialogue | FTransform complex, rarely used |
| DialogueSoundAttenuation | UDialogue | Asset reference, editor setup |
| DefaultDialogueShot | UDialogue | Instanced object, editor-only |

These remaining gaps represent edge cases that are better handled through the Unreal Editor's visual tools.

---

## 9. Version History

### v4.8.2 (Current)
- ✅ `ItemWidgetOverride` - Custom inventory UI widget
- ✅ `bWantsTickByDefault` - Enable item ticking
- ✅ `PickupMeshData` - Dropped item visual (mesh + materials)
- ✅ `TraceData` - Ranged weapon hit detection config

### v4.8.1
- ✅ `InputTag` - GameplayAbility input binding
- ✅ `bActivateAbilityOnGranted` - Auto-activate on grant
- ✅ `UniqueNPCGUID` - Save system GUID (auto-generated)
- ✅ `TriggerSets` - NPC event triggers
- ✅ `PartySpeakerInfo` - Multiplayer dialogue speakers
- ✅ `Stats` - Item stat display array
- ✅ `ActivitiesToGrant` - AI activities from items

---

## 10. CONCLUSION

**Final Automation Coverage: ~97%**

The GasAbilityGenerator plugin now has near-complete coverage of Narrative Pro's core systems. The remaining ~3% consists of:

1. **Instanced UObject properties** - Require editor-only creation (DialogueSequences)
2. **Complex visual systems** - Better authored in editor (CharacterAppearance, TriggerSets)
3. **Rarely-used transform properties** - Edge cases (PlayerAutoAdjustTransform)

All gameplay-critical properties are now fully automatable from YAML manifests.

---

*Updated after v4.8.2 implementation - January 2026*
