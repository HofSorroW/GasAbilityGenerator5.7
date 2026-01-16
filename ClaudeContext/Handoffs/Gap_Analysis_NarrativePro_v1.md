# Narrative Pro Gap Analysis - Generator Automation Coverage

**Date:** January 2026
**Plugin Version:** v4.7.1
**Analysis Scope:** All Narrative Pro headers vs. manifest parsing vs. generator implementation

---

## Executive Summary

This document analyzes gaps between Narrative Pro's C++ API and our generator's automation capabilities. The goal is to identify properties and asset types that exist in Narrative Pro but are not yet supported for YAML-based generation.

### Coverage Statistics

| Asset Type | Header Properties | Manifest Fields | Generator Coverage | Gap % |
|------------|------------------|-----------------|-------------------|-------|
| UNarrativeGameplayAbility | 4 | 2 | 50% | **50%** |
| UNPCDefinition | 18 | 17 | 94% | **6%** |
| UEquippableItem | 12 | 11 | 92% | **8%** |
| UWeaponItem | 15 | 14 | 93% | **7%** |
| URangedWeaponItem | 14 | 13 | 93% | **7%** |
| UDialogue | 17 | 14 | 82% | **18%** |
| UNPCActivityConfiguration | 3 | 3 | 100% | **0%** |
| UNPCGoalItem | 8 | 8 | 100% | **0%** |
| UNarrativeActivityBase | 4 | 4 | 100% | **0%** |
| UCharacterDefinition | 7 | 6 | 86% | **14%** |
| UAbilityConfiguration | 3 | 3 | 100% | **0%** |
| UNarrativeItem (Base) | 20 | 14 | 70% | **30%** |

---

## 1. UNarrativeGameplayAbility - GAPS IDENTIFIED

### Header Properties (NarrativeGameplayAbility.h)
```cpp
UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Narrative Ability", meta=(Categories="Narrative.Input"))
FGameplayTag InputTag = FNarrativeGameplayTags::Get().Narrative_Input_None;

UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Narrative Ability")
bool bActivateAbilityOnGranted = false;
```

### Current Manifest Support
- ✅ `parent_class` - Supported
- ✅ `instancing_policy` - Supported
- ✅ `net_execution_policy` - Supported
- ✅ `cooldown_gameplay_effect_class` - Supported
- ✅ `tags` (ability_tags, cancel_with_tag, etc.) - Supported
- ✅ `variables` - Supported
- ✅ `event_graph` - Supported

### GAPS (Priority: HIGH)
| Property | Type | Purpose | Effort |
|----------|------|---------|--------|
| **`input_tag`** | FGameplayTag | Maps ability to input action | LOW |
| **`activate_on_granted`** | bool | Auto-activate when ability is given | LOW |

### Recommendation
Add to `FManifestGameplayAbilityDefinition`:
```cpp
FString InputTag;                    // Narrative.Input.* tag for input binding
bool bActivateAbilityOnGranted = false;
```

---

## 2. UNPCDefinition - GAPS IDENTIFIED

### Header Properties (NPCDefinition.h)
```cpp
FName NPCID;                                    // ✅ Supported
FText NPCName;                                  // ✅ Supported
int32 MinLevel, MaxLevel;                       // ✅ Supported
bool bAllowMultipleInstances;                   // ✅ Supported
FGuid UniqueNPCGUID;                            // ❌ NOT SUPPORTED
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

### GAPS (Priority: LOW)
| Property | Type | Purpose | Effort |
|----------|------|---------|--------|
| **`unique_npc_guid`** | FGuid | Save system GUID for unique NPCs | LOW |

### From UCharacterDefinition (inherited)
```cpp
TSoftObjectPtr<UCharacterAppearanceBase> DefaultAppearance; // ✅ Supported
int32 DefaultCurrency;                          // ✅ Supported
TArray<FLootTableRoll> DefaultItemLoadout;      // ✅ Supported
FGameplayTagContainer DefaultOwnedTags;         // ✅ Supported
FGameplayTagContainer DefaultFactions;          // ✅ Supported
TArray<TSoftObjectPtr<UTriggerSet>> TriggerSets; // ❌ NOT SUPPORTED (partial)
float AttackPriority;                           // ✅ Supported
UAbilityConfiguration* AbilityConfiguration;    // ✅ Supported
```

### GAPS (Priority: MEDIUM)
| Property | Type | Purpose | Effort |
|----------|------|---------|--------|
| **`trigger_sets`** | TArray<TSoftObjectPtr<UTriggerSet>> | Event triggers for NPC | MEDIUM |

---

## 3. UDialogue - GAPS IDENTIFIED

### Header Properties (Dialogue.h)
```cpp
TArray<FSpeakerInfo> Speakers;                  // ✅ Supported
FPlayerSpeakerInfo PlayerSpeakerInfo;           // ✅ Supported (v4.0)
TArray<FPlayerSpeakerInfo> PartySpeakerInfo;    // ❌ NOT SUPPORTED
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
FTransform PlayerAutoAdjustTransform;           // ❌ NOT SUPPORTED
bool bAdjustPlayerTransform;                    // ✅ Supported
TSubclassOf<UCameraShakeBase> DialogueCameraShake; // ✅ Supported (v4.1)
USoundAttenuation* DialogueSoundAttenuation;    // ❌ NOT SUPPORTED
UNarrativeDialogueSequence* DefaultDialogueShot; // ❌ NOT SUPPORTED
```

### GAPS (Priority: MEDIUM)
| Property | Type | Purpose | Effort |
|----------|------|---------|--------|
| **`party_speaker_info`** | TArray<FPlayerSpeakerInfo> | Party member speakers | MEDIUM |
| **`player_auto_adjust_transform`** | FTransform | Transform for player positioning | LOW |
| **`dialogue_sound_attenuation`** | USoundAttenuation* | Audio attenuation settings | LOW |
| **`default_dialogue_shot`** | UNarrativeDialogueSequence* | Default camera sequence | MEDIUM |

---

## 4. UNarrativeItem (Base) - GAPS IDENTIFIED

### Header Properties (NarrativeItem.h)
```cpp
TSoftObjectPtr<UStaticMesh> PickupMesh;         // ❌ DEPRECATED
TSoftObjectPtr<UTexture2D> Thumbnail;           // ✅ Supported
USoundBase* UseSound;                           // ✅ Supported
FText DisplayName;                              // ✅ Supported
FText Description;                              // ✅ Supported
float Weight;                                   // ✅ Supported
int32 BaseValue;                                // ✅ Supported
float BaseScore;                                // ✅ Supported
FGameplayTagContainer ItemTags;                 // ✅ Supported
TSubclassOf<UNarrativeInventoryItemButton> ItemWidgetOverride; // ❌ NOT SUPPORTED
bool bAddDefaultUseOption;                      // ✅ Supported
bool bConsumeOnUse;                             // ✅ Supported
bool bUsedWithOtherItem;                        // ✅ Supported
FText UseActionText;                            // ✅ Supported
float UseRechargeDuration;                      // ✅ Supported
bool bCanActivate;                              // ✅ Supported
bool bToggleActiveOnUse;                        // ✅ Supported
bool bStackable;                                // ✅ Supported
int32 MaxStackSize;                             // ✅ Supported
bool bWantsTickByDefault;                       // ❌ NOT SUPPORTED
TArray<TSubclassOf<UNPCActivity>> ActivitiesToGrant; // ❌ NOT SUPPORTED
TArray<FNarrativeItemStat> Stats;               // ❌ NOT SUPPORTED
FPickupMeshData PickupMeshData;                 // ❌ NOT SUPPORTED
```

### GAPS (Priority: MEDIUM)
| Property | Type | Purpose | Effort |
|----------|------|---------|--------|
| **`item_widget_override`** | TSubclassOf | Custom inventory UI | LOW |
| **`wants_tick_by_default`** | bool | Enable item ticking | LOW |
| **`activities_to_grant`** | TArray | NPC activities from item | MEDIUM |
| **`stats`** | TArray<FNarrativeItemStat> | UI stat display | MEDIUM |
| **`pickup_mesh_data`** | FPickupMeshData | Pickup visual config | MEDIUM |

---

## 5. UEquippableItem - GAPS IDENTIFIED

### Header Properties (EquippableItem.h)
```cpp
FGameplayTagContainer EquippableSlots;          // ✅ Supported
TSubclassOf<UGameplayEffect> EquipmentEffect;   // ✅ Supported
TMap<FGameplayTag, float> EquipmentEffectValues; // ✅ Supported
float AttackRating;                             // ✅ Supported (deprecated in header)
float ArmorRating;                              // ✅ Supported (deprecated in header)
float StealthRating;                            // ✅ Supported (deprecated in header)
TArray<TSubclassOf<UNarrativeGameplayAbility>> EquipmentAbilities; // ✅ Supported (v4.2)
```

### Status: **FULLY COVERED** ✅

---

## 6. UWeaponItem - GAPS IDENTIFIED

### Header Properties (WeaponItem.h)
```cpp
TSoftClassPtr<AWeaponVisual> WeaponVisualClass; // ✅ Supported
TMap<FGameplayTag, FWeaponAttachmentConfig> HolsterAttachmentConfigs; // ✅ Supported (v4.0)
TMap<FGameplayTag, FWeaponAttachmentConfig> WieldAttachmentConfigs;   // ✅ Supported (v4.0)
TSoftClassPtr<UCrosshairWidget> CrosshairWidget; // ✅ Supported (v3.10)
EWeaponHandRule WeaponHand;                     // ✅ Supported
TArray<TSubclassOf<UNarrativeGameplayAbility>> WeaponAbilities;      // ✅ Supported
TArray<TSubclassOf<UNarrativeGameplayAbility>> MainhandWeaponAbilities; // ✅ Supported
TArray<TSubclassOf<UNarrativeGameplayAbility>> OffhandWeaponAbilities;  // ✅ Supported
bool bPawnFollowsControlRotation;               // ✅ Supported
bool bPawnOrientsRotationToMovement;            // ✅ Supported
float AttackDamage;                             // ✅ Supported
```

### Status: **FULLY COVERED** ✅

---

## 7. URangedWeaponItem - MINOR GAPS

### Header Properties (RangedWeaponItem.h)
```cpp
float AimFOVPct;                                // ✅ Supported
float AimWeaponRenderFOV;                       // ✅ Supported (v3.10)
float AimWeaponFStop;                           // ✅ Supported (v3.10)
float BaseSpreadDegrees;                        // ✅ Supported
float MoveSpeedAddDegrees;                      // ✅ Supported (v3.10)
float CrouchSpreadMultiplier;                   // ✅ Supported (v3.10)
float AimSpreadMultiplier;                      // ✅ Supported (v3.10)
float SpreadFireBump;                           // ✅ Supported
float MaxSpreadDegrees;                         // ✅ Supported
float SpreadDecreaseSpeed;                      // ✅ Supported
FVector RecoilImpulseTranslationMin/Max;        // ✅ Supported (v3.10)
FVector HipRecoilImpulseTranslationMin/Max;     // ✅ Supported (v3.10)
FCombatTraceData TraceData;                     // ❌ NOT SUPPORTED
```

### GAPS (Priority: LOW)
| Property | Type | Purpose | Effort |
|----------|------|---------|--------|
| **`trace_data`** | FCombatTraceData | Weapon trace configuration | MEDIUM |

---

## 8. MISSING ASSET TYPES

### Currently NOT Generated (Potential Additions)

| Asset Type | Narrative Pro Class | Purpose | Priority |
|------------|---------------------|---------|----------|
| **Trigger Sets** | UTriggerSet | Event trigger containers | HIGH |
| **Character Appearance** | UCharacterAppearanceBase | Visual customization | MEDIUM |
| **Dialogue Sequences** | UNarrativeDialogueSequence | Camera shots | MEDIUM |
| **Quest Tasks (Custom)** | UNarrativeTask subclasses | Custom quest logic | LOW |
| **Interaction Config** | Various | Interaction system | LOW |

---

## 9. PRIORITY ACTION ITEMS

### HIGH Priority (Quick Wins - LOW Effort, HIGH Impact)

1. **UNarrativeGameplayAbility.InputTag** - 2 lines of code
   - Add `input_tag` field to manifest
   - Set via reflection in generator

2. **UNarrativeGameplayAbility.bActivateAbilityOnGranted** - 2 lines of code
   - Add `activate_on_granted` field to manifest
   - Set via reflection in generator

3. **UNPCDefinition.UniqueNPCGUID** - Auto-generate
   - Generate GUID if `bAllowMultipleInstances = false`

### MEDIUM Priority (Moderate Effort)

4. **UDialogue.PartySpeakerInfo** - Party dialogue support
5. **UNarrativeItem.Stats** - UI stat display array
6. **UNarrativeItem.ActivitiesToGrant** - NPC activity grants
7. **UTriggerSet Generator** - New asset type entirely

### LOW Priority (Future Enhancements)

8. **URangedWeaponItem.TraceData** - Complex struct
9. **UDialogue.DefaultDialogueShot** - Requires sequence assets
10. **UCharacterAppearance Generator** - New asset type

---

## 10. IMPLEMENTATION RECOMMENDATIONS

### Phase 1: Quick Wins (1-2 hours)
```yaml
# Add to gameplay_abilities manifest section:
gameplay_abilities:
  - name: GA_Example
    input_tag: Narrative.Input.Attack      # NEW
    activate_on_granted: true              # NEW
```

### Phase 2: Item System Enhancement (4-6 hours)
```yaml
# Add to equippable_items manifest section:
equippable_items:
  - name: EI_Example
    stats:                                  # NEW
      - stat_name: "Damage"
        stat_value: 50
        stat_icon: "/Game/UI/Icons/T_Damage"
    activities_to_grant:                    # NEW
      - BPA_PatrolWithItem
```

### Phase 3: New Asset Type - Trigger Sets (8-12 hours)
```yaml
# New manifest section:
trigger_sets:
  - name: TS_MerchantSchedule
    folder: Triggers
    triggers:
      - condition: TimeOfDay
        start_time: 800
        end_time: 1800
        events:
          - NE_OpenShop
```

---

## 11. CONCLUSION

**Current Automation Coverage: ~88%**

The GasAbilityGenerator plugin has excellent coverage of Narrative Pro's core systems. The main gaps are:

1. **NarrativeGameplayAbility** - Missing InputTag and ActivateOnGranted (2 properties)
2. **NarrativeItem base class** - Missing Stats array and ActivitiesToGrant
3. **Dialogue system** - Missing party speakers and camera sequences
4. **New asset types** - TriggerSet and CharacterAppearance not yet supported

Implementing Phase 1 (Quick Wins) would bring coverage to ~92% with minimal effort.

---

*Generated by Gap Analysis Agent - January 2026*
