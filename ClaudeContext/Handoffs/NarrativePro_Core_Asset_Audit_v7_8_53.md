# Narrative Pro Core Asset Audit v7.8.53

Comprehensive audit of all 4,823 assets in `/NarrativePro/Pro/Core/` with C++ header analysis and generator gap identification.

**Last Updated:** January 31, 2026
**Status:** ✓ All HIGH PRIORITY verification complete

---

## Table of Contents

1. [Full Folder Structure](#full-folder-structure)
2. [Asset Type Summary](#asset-type-summary)
3. [AI System Assets](#ai-system-assets)
4. [Abilities System Assets](#abilities-system-assets)
5. [Tales System Assets](#tales-system-assets)
6. [Inventory System Assets](#inventory-system-assets)
7. [UI/Widget System](#ui-widget-system)
8. [VFX System](#vfx-system)
9. [Character System](#character-system)
10. [Generator Gap Analysis](#generator-gap-analysis)
11. [Implementation Priority](#implementation-priority)

---

## Full Folder Structure

**Path:** `C:\Unreal Projects\NP22B57\Plugins\NarrativePro22B57\Content\Pro\Core\`

| Folder | Asset Count | Purpose |
|--------|-------------|---------|
| **Abilities/** | 111 | GameplayAbilities, GameplayEffects, Cues, Configurations |
| **AI/** | 143 | Activities, BehaviorTrees, Goals, Services, EQS, Mass AI |
| **Audio/** | 466 | Sound files, Music, Foley, Master Sounds |
| **BP/** | 237 | Actor Blueprints, Misc systems, Materials |
| **Character/** | 1,700+ | Animations, Skeletons, Materials, Character assets |
| **CharCreator/** | 124 | Character creator UI and data |
| **Data/** | 58 | Input Actions, Perks, Profiles |
| **Inventory/** | 53 | Items, Books, Item system |
| **Maps/** | 2 | Levels (MainMenu) |
| **Tales/** | 91 | Dialogues, Events, Tasks, Triggers, Shots |
| **UI/** | 759 | Widgets, Text Styles, Buttons, Controls |
| **VFX/** | 303 | Niagara Systems, Materials, Textures, Effects |
| **Weapons/** | 193 | Weapon models, animations, FX |

---

## Asset Type Summary

### Generator-Supported Asset Types

| Prefix | Count | Type | Header File | Generator Status |
|--------|-------|------|-------------|------------------|
| GA_ | 61 | Gameplay Abilities | NarrativeGameplayAbility.h | **v7.8.53 Complete** |
| GE_ | 33 | Gameplay Effects | GameplayEffect.h | **v7.8.53 Complete** |
| GC_ | 10 | Gameplay Cues | GameplayCueNotify_*.h | **v7.8.53 Complete** |
| AC_ | 13 | Ability/Activity Configs | AbilityConfiguration.h | **v7.8.53 Complete** |
| BPA_ | 17 | NPC Activities | NPCActivity.h | **v7.8.53 Complete** |
| Goal_ | 8 | Goal Items | NPCGoalItem.h | **v7.8.53 Complete** |
| BB_ | 11 | Blackboards | Blackboard.h | **v7.8.53 Complete** |
| BT_ | 15 | Behavior Trees | BehaviorTree.h | **v7.8.53 Complete** |
| NPC_ | 8 | NPC Definitions | NPCDefinition.h | **v7.8.53 Complete** |
| NE_ | 12 | Narrative Events | NarrativeEvent.h | **v7.8.53 Complete** |
| IA_ | 27 | Input Actions | InputAction.h | **v7.8.53 Complete** |
| IMC_ | 1 | Input Mapping Contexts | InputMappingContext.h | **v7.8.53 Complete** |
| BP_ | 90 | Actor Blueprints | Various | **v7.8.53 Complete** |
| WBP_ | 96 | Widget Blueprints | UserWidget | **v7.8.53 Complete** |

### Reference-Only Asset Types (Not Generated)

| Prefix | Count | Category | Reason |
|--------|-------|----------|--------|
| BTS_ | 16 | AI | Abstract base classes |
| BTTask_ | 8 | AI | Abstract base classes |
| BTT_ | 7 | AI | Abstract base classes |
| BTD_ | 5 | AI | Abstract base classes |
| GoalGenerator_ | 3 | AI | Specialized generators |
| BPT_ | 22 | Tales | Quest task blueprints |
| DS_ | 14 | Tales | Dialogue shots |
| Shot_ | 11 | Tales | Camera shots |
| BPC_ | 9 | Tales | Conditions |
| BPE_ | 5 | Tales | Events |
| EQS_ | 10 | AI | Environment Queries |
| M_ | 1,100+ | Art | Materials |
| MI_ | 128+ | Art | Material Instances |
| T_ | 900+ | Art | Textures |
| A_ | 152 | Art | Animations |
| AM_ | 122 | Art | Animation Montages |
| SK_ | 38 | Art | Skeletal Meshes |
| SM_ | 47 | Art | Static Meshes |
| NS_ | 25 | VFX | Niagara Systems |
| S_ | 215+ | Audio | Sound Effects |

---

## AI System Assets

### Activities (BPA_)

**Location:** `/Pro/Core/AI/Activities/`

**Assets Found:**
```
BPA_Attack.uasset
BPA_Attack_Ranged.uasset
BPA_Attack_Investigate.uasset
BPA_Attack_Melee.uasset
BPA_Attack_Melee_Primary.uasset
BPA_Attack_Ranged_Strafe.uasset
BPA_Attack_Ranged_Cover.uasset
BPA_Attack_Grenade.uasset
BPA_DriveToDestination.uasset
BPA_Flee.uasset
BPA_FlyToDestination.uasset
BPA_FollowCharacter.uasset
BPA_MoveToDestination.uasset
BPA_Idle.uasset
BPA_Interact.uasset
BPA_Patrol.uasset
BPA_ReturnToSpawn.uasset
```

**C++ Header: NPCActivity.h**
```cpp
UCLASS(Abstract, Blueprintable, EditInlineNew, AutoExpandCategories = ("Default"))
class NARRATIVEARSENAL_API UNPCActivity : public UNarrativeActivityBase
{
    // From UNarrativeActivityBase:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FText ActivityName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTagContainer OwnedTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTagContainer BlockTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTagContainer RequireTags;

    // UNPCActivity specific:
    UPROPERTY(EditDefaultsOnly)
    TObjectPtr<UBehaviorTree> BehaviourTree;

    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UNPCGoalItem> SupportedGoalType;

    UPROPERTY(EditDefaultsOnly)
    bool bIsInterruptable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bSaveActivity;
};
```

**Generator Coverage:** COMPLETE (v7.8.52)
- All base properties supported
- Enhanced properties: AttackTaggedDialogue, WeaponTypes, FollowGoal, InteractSubGoal, BBKeyFollowDistance

---

### Goals (Goal_)

**Location:** `/Pro/Core/AI/Activities/*/`

**Assets Found:**
```
Goal_Attack.uasset
Goal_Flee.uasset
Goal_DriveToDestination.uasset
Goal_FollowCharacter.uasset
Goal_MoveToDestination.uasset
Goal_Idle.uasset
Goal_Interact.uasset
Goal_Patrol.uasset
```

**C++ Header: NPCGoalItem.h**
```cpp
UCLASS(Abstract, Blueprintable, EditInlineNew, AutoExpandCategories = ("Default"))
class NARRATIVEARSENAL_API UNPCGoalItem : public UObject
{
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn=true))
    FGameplayTagContainer OwnedTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTagContainer BlockTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTagContainer RequireTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (ExposeOnSpawn=true))
    bool bRemoveOnSucceeded;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (ExposeOnSpawn=true))
    float DefaultScore;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (ExposeOnSpawn=true))
    bool bSaveGoal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta = (ExposeOnSpawn=true))
    float GoalLifetime;
};
```

**Generator Coverage:** PARTIAL
| Property | Manifest Field | Generator Sets | Status |
|----------|---------------|----------------|--------|
| OwnedTags | owned_tags | Yes | OK |
| BlockTags | block_tags | Yes | OK |
| RequireTags | require_tags | Yes | OK |
| bRemoveOnSucceeded | remove_on_succeeded | Need to verify | CHECK |
| DefaultScore | default_score | Yes | OK |
| bSaveGoal | save_goal | Need to verify | CHECK |
| GoalLifetime | goal_lifetime | Need to verify | CHECK |

**GAP:** Need to verify generator sets bRemoveOnSucceeded, bSaveGoal, GoalLifetime on CDO.

---

### Goal Generators (GoalGenerator_)

**Location:** `/Pro/Core/AI/Activities/*/Goals/`

**Assets Found:**
```
GoalGenerator_Attack.uasset
GoalGenerator_Flee.uasset
GoalGenerator_Interact.uasset
GoalGeneratorBase_Stimulus.uasset
```

**C++ Header: NPCGoalGenerator.h**
```cpp
UCLASS(Abstract, Blueprintable, EditInlineNew, AutoExpandCategories = ("Default"))
class NARRATIVEARSENAL_API UNPCGoalGenerator : public UObject
{
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bSaveGoalGenerator;
};
```

**Generator Coverage:** COMPLETE
- Simple class with only bSaveGoalGenerator property
- Variables and EventGraph supported via v7.8.52

---

### BT Services (BTS_)

**Location:** `/Pro/Core/AI/Services/`

**Assets Found:**
```
BTS_ApplyGameplayTags.uasset
BTS_ClearAIFocus.uasset
BTS_EquipAndWieldWeapon.uasset
BTS_ExecuteOnce.uasset
BTS_FocusAttackTarget.uasset
BTS_SetAIFocus.uasset
```

**C++ Header: BTService_BlueprintBase (Engine)**
```cpp
UCLASS(Abstract, Blueprintable)
class AIMODULE_API UBTService_BlueprintBase : public UBTService
{
    // From UBTAuxiliaryNode:
    UPROPERTY(EditAnywhere)
    FString NodeName;

    // From UBTService:
    UPROPERTY(EditAnywhere, meta=(ClampMin="0.0"))
    float Interval;

    UPROPERTY(EditAnywhere, meta=(ClampMin="0.0"))
    float RandomDeviation;

    UPROPERTY(EditAnywhere)
    bool bCallTickOnSearchStart;

    UPROPERTY(EditAnywhere)
    bool bRestartTimerOnEachActivation;
};
```

**Generator Coverage:** COMPLETE (v7.8.52)
- All CDO properties supported
- Variables and EventGraph supported

---

### BT Tasks (BTTask_)

**Location:** `/Pro/Core/AI/Tasks/`

**Assets Found:**
```
BTTask_ActivateAbilityByClass.uasset
BTTask_ActivateAbilityByInput.uasset
BTTask_RotateToGoal.uasset
BTT_SetBehaviorTree.uasset
BTS_PlayTaggedDialogue.uasset  (misplaced?)
```

**C++ Header: BTTask_BlueprintBase (Engine)**
```cpp
UCLASS(Abstract, Blueprintable)
class AIMODULE_API UBTTask_BlueprintBase : public UBTTaskNode
{
    // From UBTAuxiliaryNode:
    UPROPERTY(EditAnywhere)
    FString NodeName;

    // From UBTTaskNode:
    UPROPERTY(EditAnywhere)
    bool bIgnoreRestartSelf;
};
```

**Generator Coverage:** COMPLETE (v7.8.52)
- All CDO properties supported
- Variables and EventGraph supported

---

### EQS Contexts

**Location:** `/Pro/Core/AI/EQS/`

**Assets Found:**
```
EQSContext_PlayerPawn.uasset
```

**Status:** EQS Contexts are not currently generated by GasAbilityGenerator.

---

## Abilities System Assets

### Gameplay Abilities (GA_)

**Location:** `/Pro/Core/Abilities/GameplayAbilities/`

**Assets Found (43 total):**
```
# Attacks
GA_Attack_Bow.uasset
GA_Attack_ComboBase.uasset
GA_Attack_Firearm_Base.uasset
GA_Attack_Firearm_Projectile.uasset
GA_Attack_Firearm_Proj_Launcher.uasset
GA_Attack_Firearm_Trace.uasset
GA_Firearm_Pistol.uasset
GA_Firearm_Pistol_Offhand.uasset
GA_Firearm_Pistol_R.uasset
GA_Firearm_Rifle.uasset
GA_CombatAbilityBase.uasset
GA_Attack_Magic_Beam.uasset
GA_Attack_Magic_Beam_Offhand.uasset
GA_Attack_Magic_Proj.uasset
GA_Attack_Magic_Projectile.uasset
GA_Attack_Magic_Proj_Offhand.uasset
GA_Attack_Combo_Melee.uasset
GA_Attack_Combo_Melee_Offhand.uasset
GA_Melee_Punch_Unarmed.uasset
GA_Melee_Unarmed.uasset
GA_Weapon_Bash.uasset
GA_Attack_Magic_Fall.uasset
GA_Attack_Magic_NPCControl.uasset
GA_Attack_Magic_Ragdoller.uasset
GA_Attack_ThrowGrenade.uasset

# Movement
GA_Cover.uasset
GA_Crouch.uasset
GA_Dodge.uasset
GA_Jump.uasset
GA_LockOn.uasset
GA_Sprint.uasset

# Weapons
GA_Melee_Block.uasset
GA_Melee_Block_Unarmed.uasset
GA_Weapon_Aim.uasset
GA_Weapon_Reload.uasset
GA_Weapon_Wield.uasset

# Interacts
GA_Interact_AnimatedInteractable.uasset
GA_Interact_Mount.uasset
GA_Mount_Vehicle.uasset
GA_Interact_Sit.uasset

# Other
GA_Death.uasset
```

**Generator Coverage:** COMPLETE (v7.8.52)
- Full NarrativeGameplayAbility property support
- Ability triggers, cues, damage, firearm config, motion warping, execution config

---

### Gameplay Effects (GE_)

**Location:** `/Pro/Core/Abilities/GameplayEffects/`

**Assets Found (34 total):**
```
# Attributes
GE_DefaultNPCAttributes.uasset
GE_DefaultPlayerAttributes.uasset
GE_DefaultVehicleAttributes.uasset

# Equipment
GE_EquipmentModifier.uasset
GE_EquipmentModifier_Bow.uasset
GE_EquipmentModifier_Shield.uasset

# Combat
GE_Aiming.uasset
GE_BlockCooldown.uasset
GE_BlockCost.uasset
GE_Blocking.uasset
GE_DamageVolume.uasset
GE_Damage_SetByCaller.uasset
GE_DodgeCost.uasset

# Status
GE_CrouchStealth.uasset
GE_DynamicTag.uasset
GE_ForceWalk.uasset
GE_GiveXP.uasset
GE_Heal_SetByCaller.uasset
...
```

**Generator Coverage:** COMPLETE (v7.8.52)
- Execution calculations (NarrativeDamageExecCalc, NarrativeHealExecution)
- Attribute-based magnitude with coefficients
- Per-modifier tag requirements
- Conditional gameplay effects
- Periodic inhibition policy

---

### Gameplay Cues (GC_)

**Location:** `/Pro/Core/Abilities/Cues/`

**Assets Found (18 total):**
```
# Bursts
GC_Burst_Unarmed.uasset
GC_GenericBurst.uasset
GC_WeaponBurst.uasset

# Overlay Effects
GC_OverlayEffect.uasset
GC_OverlayEffect_Invisible.uasset
GC_OverlayEffect_Invulnerable.uasset
GC_OverlayEffect_Poison.uasset

# Take Damage
GC_TakeDamage.uasset
GC_TakeDamage_Block.uasset
GC_TakeDamage_Heavy.uasset
```

**Generator Coverage:** COMPLETE (v7.8.52)
- All parent class types (Burst, BurstLatent, Actor)
- Full GCN Effects system
- Spawn condition and placement overrides

---

### Ability Configurations (AC_)

**Location:** `/Pro/Core/Abilities/Configurations/`

**Assets Found:**
```
AC_NPC_Default.uasset
AC_Player_Default.uasset
AC_Player_RPG.uasset
AC_Player_Shooter.uasset
AC_Vehicle_Default.uasset
```

**Generator Coverage:** COMPLETE

---

### Projectiles (BP_WeaponProjectile_)

**Location:** `/Pro/Core/Abilities/GameplayAbilities/Attacks/Projectiles/`

**Assets Found:**
```
BP_WeaponProjectile_Base.uasset
BP_WeaponProjectile_BowArrow.uasset
BP_WeaponProjectile_Grenade.uasset
BP_WeaponProjectile_HeliMissile.uasset
BP_WeaponProjectile_Launcher.uasset
BP_WeaponProjectile_MagicFireball.uasset
BP_WeaponProjectile_BowArrow_Explosive.uasset
BP_WeaponProjectile_DarkArrow.uasset
BP_WeaponProjectile_IronArrow.uasset
```

**Status:** Actor Blueprints with specific parent class. Generated via `actor_blueprints:` section.

---

## Tales System Assets

### Blueprint Triggers (BPT_)

**Location:** `/Pro/Core/Tales/Tasks/` and `/Pro/Core/Tales/Triggers/`

**Assets Found (22 total):**
```
# Tasks
BPT_AddGoalAndWait.uasset
BPT_ClearSpawn.uasset
BPT_CompleteDataTask.uasset
BPT_CompleteQuest.uasset
BPT_EquipItem.uasset
BPT_FindItem.uasset
BPT_FinishDialogue.uasset
BPT_HolsterWeapon.uasset
BPT_InteractWithObject.uasset
BPT_KillEnemy.uasset
BPT_Move.uasset
BPT_PlayDialogueNode.uasset
BPT_TakePickup.uasset
BPT_UnequipItem.uasset
BPT_UseItem.uasset
BPT_WaitGameplayEvent.uasset
BPT_WaitGameplayTagAdded.uasset
BPT_FollowNPCToLocation.uasset
BPT_GoToLocation.uasset
BPT_PlayCutsceneAndWait.uasset

# Triggers
BPT_Always.uasset
BPT_TimeOfDayRange.uasset
```

**C++ Header: QuestTask.h**
```cpp
UCLASS(Abstract, Blueprintable, EditInlineNew, AutoExpandCategories = ("Default"))
class NARRATIVEARSENAL_API UNarrativeTask : public UObject
{
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin=1))
    int32 RequiredQuantity;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DescriptionOverride;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bOptional;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bHidden;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float TickInterval;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FTaskNavigationMarker MarkerSettings;
};

USTRUCT(BlueprintType)
struct FTaskNavigationMarker
{
    bool bAddNavigationMarker;
    bool bDrawBreadcrumbs;
    TSubclassOf<UMapMarker> MarkerClass;
    UTexture2D* NavigationMarkerIcon;
    FLinearColor MarkerColor;
    FGameplayTagContainer MarkerDomains;
    FText MarkerDisplayText;
    FText MarkerSubtitleText;
    FVector MarkerLocation;
};
```

**Generator Coverage:** COMPLETE (v7.8.53)
- All UNarrativeTask properties
- Full FTaskNavigationMarker struct support
- GoToLocation properties (GoalLoc, DistanceTolerance, bInvert)
- TimeOfDayRange properties (TimeStart, TimeEnd)
- FollowNPCToLocation properties (NPCsToFollow, GoalRotation)

---

### Narrative Events (NE_)

**Location:** `/Pro/Core/Tales/Events/`

**Assets Found (12 Tales Events):**
```
NE_AddFactions.uasset
NE_AddGameplayTags.uasset
NE_AddSaveCheckpoint.uasset
NE_BeginDialogue.uasset
NE_BeginQuest.uasset
NE_GiveXP.uasset
NE_PrintString.uasset
NE_RemoveFactions.uasset
NE_RemoveGameplayTags.uasset
NE_RestartQuest.uasset
NE_SetFactionAttitude.uasset
NE_ShowNotification.uasset
```

**Generator Coverage:** COMPLETE (v7.8.52)
- Variables support
- EventGraph support
- Override functions (ExecuteEvent, OnActivate, OnDeactivate)
- Conditions (19 types)

---

## Inventory System Assets

### Items (BI_)

**Location:** `/Pro/Core/Inventory/Items/`

**Assets Found:**
```
BI_NarrativeBook.uasset
BI_NarrativeUserManual.uasset
```

**Status:** Book items are specialized UNarrativeItem subclasses. Generated via `equippable_items:` section with appropriate parent_class.

---

## Generator Gap Analysis

### Verified Complete (No Gaps)

| Asset Type | Generator | Status |
|------------|-----------|--------|
| BPA_ Activities | FActivityGenerator | v7.8.52 Complete |
| BTS_ Services | FBTServiceGenerator | v7.8.52 Complete |
| BTTask_ Tasks | FBTTaskGenerator | v7.8.52 Complete |
| GA_ Abilities | FGameplayAbilityGenerator | v7.8.52 Complete |
| GE_ Effects | FGameplayEffectGenerator | v7.8.52 Complete |
| GC_ Cues | FGameplayCueGenerator | v7.8.52 Complete |
| AC_ Configs | FAbilityConfigurationGenerator | Complete |
| BPT_ Triggers | FBlueprintTriggerGenerator | v7.8.53 Complete |
| NE_ Events | FNarrativeEventGenerator | v7.8.52 Complete |

### Verified Complete (Continued)

| Asset Type | Generator | Status | Verified Properties |
|------------|-----------|--------|---------------------|
| Goal_ Items | FGoalItemGenerator | v7.8.53 Complete | DefaultScore, GoalLifetime, bRemoveOnSucceeded, bSaveGoal, OwnedTags, BlockTags, RequireTags |
| GoalGenerator_ | FGoalGeneratorGenerator | v7.8.53 Complete | bSaveGoalGenerator |

### Not Currently Generated

| Asset Type | Reason | Priority |
|------------|--------|----------|
| EQS Contexts | Specialized C++ class | LOW |
| Projectiles | Generic ActorBP with parent class | N/A (use actor_blueprints) |
| Book Items | Specialized item type | LOW |

---

## Implementation Priority

### HIGH PRIORITY (v7.8.53) ✓ COMPLETED

1. **Goal Item Generator Verification** ✓
   - Read FGoalItemGenerator::Generate (lines 28614-28862)
   - Verified: DefaultScore, GoalLifetime, bRemoveOnSucceeded, bSaveGoal all set on CDO
   - Verified: OwnedTags, BlockTags, RequireTags tag containers populated

2. **Goal Generator Generator Verification** ✓
   - Read FGoalGeneratorGenerator::Generate (lines 28897-29030)
   - Verified: bSaveGoalGenerator set on CDO (line 29026-29028)

### MEDIUM PRIORITY (Future)

1. **EQS Context Generator**
   - New generator for EQS_* assets
   - Requires understanding of UEnvQueryContext hierarchy

2. **Weapon Projectile Templates**
   - Not a new generator, but documentation for using actor_blueprints
   - With AWeaponProjectile as parent class

### LOW PRIORITY

1. **Book/Interactive Item Support**
   - Specialized item types
   - Complex mesh/animation requirements

---

## Completion Status

### Completed (January 31, 2026)
1. ✓ Verified Goal Item Generator CDO property setting - All properties set correctly
2. ✓ Verified Goal Generator CDO property setting - bSaveGoalGenerator set correctly
3. ✓ Item Generator enhancements (plan zazzy-seeking-lake.md) already implemented:
   - FManifestItemStatDefinition updated with StatDisplayName, StringVariable, StatTooltip
   - FManifestWeaponAttachmentConfigEntry added with TMap support
   - Parser updated for holster_attachment_configs, wield_attachment_configs
   - Generator creates TMap<FGameplayTag, FWeaponAttachmentConfig> correctly
4. ✓ Build verified - Plugin compiles successfully
5. ✓ Generation verified - All 205 assets pass (0 failures)
6. ✓ Fixed GA_ProximityStrike manifest - Use MakeOutgoingSpec + Target pin for ASC function calls

### Key Insights
- When calling ASC functions from a GameplayAbility, use `MakeOutgoingSpec` (not `MakeOutgoingGameplayEffectSpec`)
- Connect the ASC variable to the `Target` pin (not `self`) since `self` refers to the Blueprint (GA)
- 13 asset types are fully generator-supported
- 4,823 total assets in Narrative Pro Core folder
- All art/audio assets are reference-only (not generated)

---

**Document Version:** v7.8.53
**Created:** January 31, 2026
**Author:** Claude (Anthropic)
