# Spec DataAsset UX Implementation Handoff

**Native UE Workflow for Asset Generation** | January 2026 | Ready for Implementation

This document contains the complete implementation plan for replacing YAML-centric workflow with Spec DataAssets + Content Browser right-click generation.

---

## TABLE OF CONTENTS

1. [Executive Summary](#1-executive-summary)
2. [Architecture Overview](#2-architecture-overview)
3. [Spec DataAsset Definitions](#3-spec-dataasset-definitions)
4. [Content Browser Integration](#4-content-browser-integration)
5. [Generation Flow](#5-generation-flow)
6. [Template Inheritance](#6-template-inheritance)
7. [YAML Migration](#7-yaml-migration)
8. [Implementation Order](#8-implementation-order)
9. [File Changes Summary](#9-file-changes-summary)

---

## 1. EXECUTIVE SUMMARY

### Problem
Current workflow requires:
- Learning YAML syntax
- Manual reference coordination
- External file management
- Custom wizard UI development

### Solution
Spec DataAssets + Content Browser right-click:
- Edit specs in native Details panel (no custom UI)
- TSubclassOf fields auto-populate dropdowns
- Right-click → Generate/Validate/Preview
- Template inheritance via ParentTemplate field

### Benefits
| Aspect | Benefit |
|--------|---------|
| UI Development | Zero - uses Details panel |
| Dropdowns | Free - TSubclassOf asset pickers |
| Validation | Free - UPROPERTY meta tags |
| Undo/Redo | Free - FScopedTransaction |
| Batch Support | Commandlet scans spec assets |

---

## 2. ARCHITECTURE OVERVIEW

### Before (YAML-centric)
```
┌──────────────┐     ┌─────────────┐     ┌─────────────┐
│ manifest.yaml│ ──► │   Parser    │ ──► │ FManifest*  │ ──► Generators
└──────────────┘     └─────────────┘     └─────────────┘
```

### After (Asset-centric)
```
┌──────────────┐                         ┌─────────────┐
│ *_Spec.uasset│ ────────────────────► │ FManifest*  │ ──► Generators
└──────────────┘   ToManifestDefinition  └─────────────┘
        │
        ▼ (optional export)
┌──────────────┐
│ manifest.yaml│  ← For CI/versioning
└──────────────┘
```

### Key Insight
Generators remain unchanged. Only the input source changes from YAML to DataAssets.

---

## 3. SPEC DATAASSET DEFINITIONS

### 3.1 New Header File: GasAbilityGeneratorSpecs.h

Create: `Source/GasAbilityGenerator/Public/GasAbilityGeneratorSpecs.h`

```cpp
// GasAbilityGeneratorSpecs.h
// v4.0: Spec DataAssets for native UE workflow
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GasAbilityGeneratorTypes.h"
#include "GasAbilityGeneratorSpecs.generated.h"

// Forward declarations
class UGameplayAbility;
class UGameplayEffect;
class UNPCActivity;
class UNPCGoalItem;
class ANarrativeNPCCharacter;
class UNarrativeItem;

//=============================================================================
// Supporting Structs
//=============================================================================

USTRUCT(BlueprintType)
struct FSpecScheduleEntry
{
    GENERATED_BODY()

    /** Start time (0-2400, where 100 = 1 hour) */
    UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 2400))
    int32 StartTime = 600;

    /** End time (0-2400, where 100 = 1 hour) */
    UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 2400))
    int32 EndTime = 1800;

    /** Goal to pursue during this time */
    UPROPERTY(EditAnywhere)
    TSubclassOf<UNPCGoalItem> GoalClass;

    /** Optional location tag */
    UPROPERTY(EditAnywhere)
    FString Location;

    /** Score override (-1 = use goal default) */
    UPROPERTY(EditAnywhere, meta = (ClampMin = -1.0))
    float ScoreOverride = -1.0f;

    /** Trigger activity reselection */
    UPROPERTY(EditAnywhere)
    bool bReselect = true;
};

USTRUCT(BlueprintType)
struct FSpecVendorItem
{
    GENERATED_BODY()

    /** Item class to sell */
    UPROPERTY(EditAnywhere)
    TSubclassOf<UNarrativeItem> Item;

    /** Quantity in stock */
    UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
    int32 Quantity = 1;

    /** Chance to have in stock (0.0 - 1.0) */
    UPROPERTY(EditAnywhere, meta = (ClampMin = 0.0, ClampMax = 1.0))
    float Chance = 1.0f;
};

USTRUCT(BlueprintType)
struct FSpecGoalDefinition
{
    GENERATED_BODY()

    /** Goal identifier (used for naming: Goal_{NPCName}_{Id}) */
    UPROPERTY(EditAnywhere)
    FString Id;

    /** Default priority score */
    UPROPERTY(EditAnywhere, meta = (ClampMin = 0.0))
    float DefaultScore = 50.0f;

    /** Goal lifetime in seconds (-1 = never expires) */
    UPROPERTY(EditAnywhere)
    float Lifetime = -1.0f;

    /** Remove goal when succeeded */
    UPROPERTY(EditAnywhere)
    bool bRemoveOnSucceeded = false;

    /** Tags granted while pursuing goal */
    UPROPERTY(EditAnywhere)
    FGameplayTagContainer OwnedTags;

    /** Tags that block this goal */
    UPROPERTY(EditAnywhere)
    FGameplayTagContainer BlockTags;

    /** Tags required to pursue goal */
    UPROPERTY(EditAnywhere)
    FGameplayTagContainer RequireTags;
};

USTRUCT(BlueprintType)
struct FSpecDialogueNode
{
    GENERATED_BODY()

    /** Node identifier */
    UPROPERTY(EditAnywhere)
    FString Id;

    /** Node type: npc or player */
    UPROPERTY(EditAnywhere)
    FString Type = TEXT("npc");

    /** Dialogue text */
    UPROPERTY(EditAnywhere, meta = (MultiLine = true))
    FText Text;

    /** Option text (for player choices) */
    UPROPERTY(EditAnywhere)
    FText OptionText;

    /** Child node IDs */
    UPROPERTY(EditAnywhere)
    TArray<FString> Replies;

    /** Quest to start when selected */
    UPROPERTY(EditAnywhere)
    FString StartQuest;

    /** Quest branch to complete */
    UPROPERTY(EditAnywhere)
    FString CompleteQuestBranch;
};

USTRUCT(BlueprintType)
struct FSpecQuestObjective
{
    GENERATED_BODY()

    /** Item to find (BPT_FindItem) */
    UPROPERTY(EditAnywhere)
    TSubclassOf<UNarrativeItem> FindItem;

    /** Quantity required */
    UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
    int32 Quantity = 1;

    /** Location to reach (BPT_Move) */
    UPROPERTY(EditAnywhere)
    FString GoToLocation;

    /** NPC to talk to (BPT_FinishDialogue) */
    UPROPERTY(EditAnywhere)
    FString TalkToNPC;

    /** Is this objective optional */
    UPROPERTY(EditAnywhere)
    bool bOptional = false;
};

USTRUCT(BlueprintType)
struct FSpecQuestReward
{
    GENERATED_BODY()

    /** Currency to grant */
    UPROPERTY(EditAnywhere, meta = (ClampMin = 0))
    int32 Currency = 0;

    /** XP to grant */
    UPROPERTY(EditAnywhere, meta = (ClampMin = 0))
    int32 XP = 0;

    /** Items to grant */
    UPROPERTY(EditAnywhere)
    TArray<TSubclassOf<UNarrativeItem>> Items;
};

USTRUCT(BlueprintType)
struct FSpecQuestDefinition
{
    GENERATED_BODY()

    /** Quest identifier (used for naming: Quest_{NPCName}_{Id}) */
    UPROPERTY(EditAnywhere)
    FString Id;

    /** Quest display name */
    UPROPERTY(EditAnywhere)
    FText QuestName;

    /** Quest description */
    UPROPERTY(EditAnywhere, meta = (MultiLine = true))
    FText Description;

    /** Quest objectives */
    UPROPERTY(EditAnywhere)
    TArray<FSpecQuestObjective> Objectives;

    /** Quest rewards */
    UPROPERTY(EditAnywhere)
    FSpecQuestReward Rewards;

    /** Show in quest tracker */
    UPROPERTY(EditAnywhere)
    bool bTracked = true;
};

//=============================================================================
// NPC Package Spec
//=============================================================================

/**
 * UNPCPackageSpec - Defines a complete NPC with all related assets
 *
 * Usage:
 * 1. Create in Content Browser: Right-click → Create → GasAbilityGenerator → NPC Spec
 * 2. Configure in Details panel
 * 3. Right-click → Generate Assets
 *
 * Generates: NPCDef_, AC_, ActConfig_, Goal_, Schedule_, DBP_, Quest_ assets
 */
UCLASS(BlueprintType)
class GASABILITYGENERATOR_API UNPCPackageSpec : public UDataAsset
{
    GENERATED_BODY()

public:
    //=========================================================================
    // Template Inheritance
    //=========================================================================

    /** Parent template to inherit from (all fields merged) */
    UPROPERTY(EditAnywhere, Category = "Template")
    TSoftObjectPtr<UNPCPackageSpec> ParentTemplate;

    //=========================================================================
    // Identity
    //=========================================================================

    /** Base name for generated assets (e.g., "Blacksmith" → NPCDef_Blacksmith) */
    UPROPERTY(EditAnywhere, Category = "Identity")
    FString BaseName;

    /** Output folder under /Game/{ProjectName}/ */
    UPROPERTY(EditAnywhere, Category = "Identity")
    FString OutputFolder = TEXT("NPCs");

    /** Unique NPC identifier */
    UPROPERTY(EditAnywhere, Category = "Identity")
    FString NPCId;

    /** Display name shown in-game */
    UPROPERTY(EditAnywhere, Category = "Identity")
    FText NPCDisplayName;

    /** NPC Blueprint class */
    UPROPERTY(EditAnywhere, Category = "Identity")
    TSoftClassPtr<ANarrativeNPCCharacter> NPCBlueprint;

    //=========================================================================
    // Abilities
    //=========================================================================

    /** Abilities granted to this NPC (populates AC_{BaseName}) */
    UPROPERTY(EditAnywhere, Category = "Abilities")
    TArray<TSubclassOf<UGameplayAbility>> Abilities;

    /** Effects applied at spawn */
    UPROPERTY(EditAnywhere, Category = "Abilities")
    TArray<TSubclassOf<UGameplayEffect>> StartupEffects;

    /** Default attribute set */
    UPROPERTY(EditAnywhere, Category = "Abilities")
    TSubclassOf<UGameplayEffect> DefaultAttributes;

    //=========================================================================
    // Activities
    //=========================================================================

    /** Activities available to this NPC (populates ActConfig_{BaseName}) */
    UPROPERTY(EditAnywhere, Category = "Activities")
    TArray<TSubclassOf<UNPCActivity>> Activities;

    /** Goal generators */
    UPROPERTY(EditAnywhere, Category = "Activities")
    TArray<TSubclassOf<UObject>> GoalGenerators;

    /** Custom goals (generates Goal_{BaseName}_{Id} assets) */
    UPROPERTY(EditAnywhere, Category = "Activities")
    TArray<FSpecGoalDefinition> Goals;

    //=========================================================================
    // Schedule
    //=========================================================================

    /** Daily schedule (generates Schedule_{BaseName}) */
    UPROPERTY(EditAnywhere, Category = "Schedule")
    TArray<FSpecScheduleEntry> Schedule;

    //=========================================================================
    // Dialogue
    //=========================================================================

    /** Auto-create dialogue blueprint */
    UPROPERTY(EditAnywhere, Category = "Dialogue")
    bool bAutoCreateDialogue = true;

    /** Auto-create tagged dialogue set */
    UPROPERTY(EditAnywhere, Category = "Dialogue")
    bool bAutoCreateTaggedDialogue = true;

    /** Greeting text (root dialogue node) */
    UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (MultiLine = true))
    FText GreetingText;

    /** Dialogue nodes (generates DBP_{BaseName}) */
    UPROPERTY(EditAnywhere, Category = "Dialogue")
    TArray<FSpecDialogueNode> DialogueNodes;

    //=========================================================================
    // Quests
    //=========================================================================

    /** Quests given by this NPC (generates Quest_{BaseName}_{Id}) */
    UPROPERTY(EditAnywhere, Category = "Quests")
    TArray<FSpecQuestDefinition> Quests;

    //=========================================================================
    // Vendor
    //=========================================================================

    /** Is this NPC a vendor */
    UPROPERTY(EditAnywhere, Category = "Vendor")
    bool bIsVendor = false;

    /** Shop display name */
    UPROPERTY(EditAnywhere, Category = "Vendor", meta = (EditCondition = "bIsVendor"))
    FString ShopName;

    /** Starting currency */
    UPROPERTY(EditAnywhere, Category = "Vendor", meta = (EditCondition = "bIsVendor", ClampMin = 0))
    int32 TradingCurrency = 500;

    /** Buy price multiplier (0.5 = pays 50% of value) */
    UPROPERTY(EditAnywhere, Category = "Vendor", meta = (EditCondition = "bIsVendor", ClampMin = 0.0, ClampMax = 2.0))
    float BuyItemPercentage = 0.5f;

    /** Sell price multiplier (1.5 = charges 150% of value) */
    UPROPERTY(EditAnywhere, Category = "Vendor", meta = (EditCondition = "bIsVendor", ClampMin = 0.0, ClampMax = 5.0))
    float SellItemPercentage = 1.5f;

    /** Shop inventory */
    UPROPERTY(EditAnywhere, Category = "Vendor", meta = (EditCondition = "bIsVendor"))
    TArray<FSpecVendorItem> ShopInventory;

    //=========================================================================
    // Tags & Factions
    //=========================================================================

    /** Default owned tags */
    UPROPERTY(EditAnywhere, Category = "Tags")
    FGameplayTagContainer DefaultOwnedTags;

    /** Faction tags */
    UPROPERTY(EditAnywhere, Category = "Tags")
    FGameplayTagContainer DefaultFactions;

    //=========================================================================
    // Misc
    //=========================================================================

    /** Minimum level */
    UPROPERTY(EditAnywhere, Category = "Misc", meta = (ClampMin = 1))
    int32 MinLevel = 1;

    /** Maximum level */
    UPROPERTY(EditAnywhere, Category = "Misc", meta = (ClampMin = 1))
    int32 MaxLevel = 10;

    /** Attack priority (0 = low, 1 = high) */
    UPROPERTY(EditAnywhere, Category = "Misc", meta = (ClampMin = 0.0, ClampMax = 1.0))
    float AttackPriority = 0.5f;

    /** Allow multiple instances in world */
    UPROPERTY(EditAnywhere, Category = "Misc")
    bool bAllowMultipleInstances = false;

    //=========================================================================
    // Generation Tracking (Read-Only)
    //=========================================================================

    /** Assets generated from this spec */
    UPROPERTY(VisibleAnywhere, Category = "Generated", AdvancedDisplay)
    TArray<FSoftObjectPath> GeneratedAssets;

    /** Last generation timestamp */
    UPROPERTY(VisibleAnywhere, Category = "Generated", AdvancedDisplay)
    FDateTime LastGeneratedTime;

    //=========================================================================
    // Methods
    //=========================================================================

    /** Convert to manifest definition for generators */
    FManifestNPCPackageDefinition ToManifestDefinition() const;

    /** Merge parent template fields into this spec */
    void MergeWithParent(const UNPCPackageSpec* Parent);

    /** Get generated asset names */
    FString GetNPCDefName() const { return FString::Printf(TEXT("NPCDef_%s"), *BaseName); }
    FString GetACName() const { return FString::Printf(TEXT("AC_%s"), *BaseName); }
    FString GetActConfigName() const { return FString::Printf(TEXT("ActConfig_%s"), *BaseName); }
    FString GetScheduleName() const { return FString::Printf(TEXT("Schedule_%s"), *BaseName); }
    FString GetDialogueName() const { return FString::Printf(TEXT("DBP_%s"), *BaseName); }
    FString GetTaggedDialogueName() const { return FString::Printf(TEXT("%s_TaggedDialogue"), *BaseName); }
    FString GetGoalName(const FString& GoalId) const { return FString::Printf(TEXT("Goal_%s_%s"), *BaseName, *GoalId); }
    FString GetQuestName(const FString& QuestId) const { return FString::Printf(TEXT("Quest_%s_%s"), *BaseName, *QuestId); }
};

//=============================================================================
// Quest Spec (Standalone)
//=============================================================================

/**
 * UQuestSpec - Defines a standalone quest
 */
UCLASS(BlueprintType)
class GASABILITYGENERATOR_API UQuestSpec : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Template")
    TSoftObjectPtr<UQuestSpec> ParentTemplate;

    UPROPERTY(EditAnywhere, Category = "Identity")
    FString QuestName;

    UPROPERTY(EditAnywhere, Category = "Identity")
    FString OutputFolder = TEXT("Quests");

    UPROPERTY(EditAnywhere, Category = "Quest")
    FText DisplayName;

    UPROPERTY(EditAnywhere, Category = "Quest", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditAnywhere, Category = "Quest")
    TSoftObjectPtr<UDataAsset> QuestGiver;  // NPCDefinition

    UPROPERTY(EditAnywhere, Category = "Quest")
    bool bTracked = true;

    UPROPERTY(EditAnywhere, Category = "Objectives")
    TArray<FSpecQuestObjective> Objectives;

    UPROPERTY(EditAnywhere, Category = "Rewards")
    FSpecQuestReward Rewards;

    UPROPERTY(VisibleAnywhere, Category = "Generated", AdvancedDisplay)
    TArray<FSoftObjectPath> GeneratedAssets;

    FManifestQuestDefinition ToManifestDefinition() const;
};

//=============================================================================
// Item Spec
//=============================================================================

/**
 * UItemSpec - Defines an equippable item
 */
UCLASS(BlueprintType)
class GASABILITYGENERATOR_API UItemSpec : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Template")
    TSoftObjectPtr<UItemSpec> ParentTemplate;

    UPROPERTY(EditAnywhere, Category = "Identity")
    FString ItemName;

    UPROPERTY(EditAnywhere, Category = "Identity")
    FString OutputFolder = TEXT("Items");

    UPROPERTY(EditAnywhere, Category = "Identity")
    TSubclassOf<UObject> ParentClass;  // EquippableItem, WeaponItem, etc.

    UPROPERTY(EditAnywhere, Category = "Item")
    FText DisplayName;

    UPROPERTY(EditAnywhere, Category = "Item", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditAnywhere, Category = "Item")
    FGameplayTag EquipmentSlot;

    UPROPERTY(EditAnywhere, Category = "Stats")
    float AttackRating = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Stats")
    float ArmorRating = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Stats")
    float Weight = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Stats")
    int32 BaseValue = 100;

    UPROPERTY(EditAnywhere, Category = "Stacking")
    bool bStackable = false;

    UPROPERTY(EditAnywhere, Category = "Stacking", meta = (EditCondition = "bStackable"))
    int32 MaxStackSize = 1;

    UPROPERTY(EditAnywhere, Category = "Tags")
    FGameplayTagContainer ItemTags;

    UPROPERTY(EditAnywhere, Category = "Abilities")
    TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrant;

    UPROPERTY(VisibleAnywhere, Category = "Generated", AdvancedDisplay)
    TArray<FSoftObjectPath> GeneratedAssets;

    FManifestEquippableItemDefinition ToManifestDefinition() const;
};
```

### 3.2 Implementation File: GasAbilityGeneratorSpecs.cpp

Create: `Source/GasAbilityGenerator/Private/GasAbilityGeneratorSpecs.cpp`

```cpp
// GasAbilityGeneratorSpecs.cpp

#include "GasAbilityGeneratorSpecs.h"

//=============================================================================
// UNPCPackageSpec
//=============================================================================

FManifestNPCPackageDefinition UNPCPackageSpec::ToManifestDefinition() const
{
    FManifestNPCPackageDefinition Def;

    // Identity
    Def.Name = BaseName;
    Def.Folder = OutputFolder;
    Def.NPCId = NPCId;
    Def.NPCName = NPCDisplayName.ToString();
    Def.NPCBlueprint = NPCBlueprint.ToString();

    // Abilities - convert TSubclassOf to asset names
    for (const TSubclassOf<UGameplayAbility>& AbilityClass : Abilities)
    {
        if (AbilityClass)
        {
            FString ClassName = AbilityClass->GetName();
            ClassName.RemoveFromEnd(TEXT("_C"));
            Def.Abilities.Add(ClassName);
        }
    }

    for (const TSubclassOf<UGameplayEffect>& EffectClass : StartupEffects)
    {
        if (EffectClass)
        {
            FString ClassName = EffectClass->GetName();
            ClassName.RemoveFromEnd(TEXT("_C"));
            Def.StartupEffects.Add(ClassName);
        }
    }

    if (DefaultAttributes)
    {
        FString ClassName = DefaultAttributes->GetName();
        ClassName.RemoveFromEnd(TEXT("_C"));
        Def.DefaultAttributes = ClassName;
    }

    // Activities
    for (const TSubclassOf<UNPCActivity>& ActivityClass : Activities)
    {
        if (ActivityClass)
        {
            FString ClassName = ActivityClass->GetName();
            ClassName.RemoveFromEnd(TEXT("_C"));
            Def.Activities.Add(ClassName);
        }
    }

    // Goals
    for (const FSpecGoalDefinition& GoalSpec : Goals)
    {
        FManifestNPCPackageGoalDefinition GoalDef;
        GoalDef.Id = GoalSpec.Id;
        GoalDef.Score = GoalSpec.DefaultScore;
        GoalDef.Lifetime = GoalSpec.Lifetime;
        GoalDef.bRemoveOnSucceeded = GoalSpec.bRemoveOnSucceeded;

        // Convert tag containers to string arrays
        for (const FGameplayTag& Tag : GoalSpec.OwnedTags)
        {
            GoalDef.OwnedTags.Add(Tag.ToString());
        }
        for (const FGameplayTag& Tag : GoalSpec.BlockTags)
        {
            GoalDef.BlockTags.Add(Tag.ToString());
        }
        for (const FGameplayTag& Tag : GoalSpec.RequireTags)
        {
            GoalDef.RequireTags.Add(Tag.ToString());
        }

        Def.Goals.Add(GoalSpec.Id, GoalDef);
    }

    // Schedule
    for (const FSpecScheduleEntry& Entry : Schedule)
    {
        FManifestNPCPackageScheduleEntry SchedEntry;
        SchedEntry.StartTime = Entry.StartTime;
        SchedEntry.EndTime = Entry.EndTime;
        SchedEntry.Location = Entry.Location;
        SchedEntry.ScoreOverride = Entry.ScoreOverride;

        if (Entry.GoalClass)
        {
            FString ClassName = Entry.GoalClass->GetName();
            ClassName.RemoveFromEnd(TEXT("_C"));
            SchedEntry.Goal = ClassName;
        }

        Def.Schedule.Add(SchedEntry);
    }

    // Dialogue
    Def.bAutoCreateDialogue = bAutoCreateDialogue;
    Def.bAutoCreateTaggedDialogue = bAutoCreateTaggedDialogue;
    Def.GreetingText = GreetingText.ToString();

    for (const FSpecDialogueNode& Node : DialogueNodes)
    {
        FManifestNPCPackageDialogueNode DialogueNode;
        DialogueNode.Id = Node.Id;
        DialogueNode.Type = Node.Type;
        DialogueNode.Text = Node.Text.ToString();
        DialogueNode.OptionText = Node.OptionText.ToString();
        DialogueNode.Replies = Node.Replies;
        DialogueNode.StartQuest = Node.StartQuest;
        DialogueNode.CompleteQuestBranch = Node.CompleteQuestBranch;
        Def.DialogueNodes.Add(DialogueNode);
    }

    // Quests
    for (const FSpecQuestDefinition& Quest : Quests)
    {
        FManifestNPCPackageQuestDefinition QuestDef;
        QuestDef.Id = Quest.Id;
        QuestDef.Name = Quest.QuestName.ToString();
        QuestDef.Description = Quest.Description.ToString();
        QuestDef.bTracked = Quest.bTracked;

        for (const FSpecQuestObjective& Obj : Quest.Objectives)
        {
            FManifestNPCPackageQuestObjective ObjDef;
            if (Obj.FindItem)
            {
                ObjDef.Find = Obj.FindItem->GetName();
            }
            ObjDef.Quantity = Obj.Quantity;
            ObjDef.GoTo = Obj.GoToLocation;
            ObjDef.TalkTo = Obj.TalkToNPC;
            ObjDef.bOptional = Obj.bOptional;
            QuestDef.Objectives.Add(ObjDef);
        }

        QuestDef.Rewards.Currency = Quest.Rewards.Currency;
        QuestDef.Rewards.XP = Quest.Rewards.XP;
        for (const TSubclassOf<UNarrativeItem>& ItemClass : Quest.Rewards.Items)
        {
            if (ItemClass)
            {
                QuestDef.Rewards.Items.Add(ItemClass->GetName());
            }
        }

        Def.Quests.Add(Quest.Id, QuestDef);
    }

    // Vendor
    Def.Vendor.bEnabled = bIsVendor;
    Def.Vendor.ShopName = ShopName;
    Def.Vendor.Currency = TradingCurrency;
    Def.Vendor.BuyPercentage = BuyItemPercentage;
    Def.Vendor.SellPercentage = SellItemPercentage;

    for (const FSpecVendorItem& VendorItem : ShopInventory)
    {
        FManifestNPCPackageVendorItem Item;
        if (VendorItem.Item)
        {
            Item.Item = VendorItem.Item->GetName();
        }
        Item.Quantity = VendorItem.Quantity;
        Item.Chance = VendorItem.Chance;
        Def.Vendor.Inventory.Add(Item);
    }

    // Tags
    for (const FGameplayTag& Tag : DefaultOwnedTags)
    {
        Def.DefaultTags.Add(Tag.ToString());
    }
    for (const FGameplayTag& Tag : DefaultFactions)
    {
        Def.Factions.Add(Tag.ToString());
    }

    // Misc
    Def.MinLevel = MinLevel;
    Def.MaxLevel = MaxLevel;
    Def.AttackPriority = AttackPriority;
    Def.bAllowMultipleInstances = bAllowMultipleInstances;

    return Def;
}

void UNPCPackageSpec::MergeWithParent(const UNPCPackageSpec* Parent)
{
    if (!Parent) return;

    // Merge arrays (append parent, don't override)
    if (Abilities.Num() == 0) Abilities = Parent->Abilities;
    if (StartupEffects.Num() == 0) StartupEffects = Parent->StartupEffects;
    if (Activities.Num() == 0) Activities = Parent->Activities;
    if (Goals.Num() == 0) Goals = Parent->Goals;
    if (Schedule.Num() == 0) Schedule = Parent->Schedule;

    // Merge single values (only if not set)
    if (!DefaultAttributes) DefaultAttributes = Parent->DefaultAttributes;
    if (NPCBlueprint.IsNull()) NPCBlueprint = Parent->NPCBlueprint;

    // Merge vendor config
    if (!bIsVendor && Parent->bIsVendor)
    {
        bIsVendor = Parent->bIsVendor;
        ShopName = Parent->ShopName;
        TradingCurrency = Parent->TradingCurrency;
        BuyItemPercentage = Parent->BuyItemPercentage;
        SellItemPercentage = Parent->SellItemPercentage;
        ShopInventory = Parent->ShopInventory;
    }

    // Merge tags (combine)
    DefaultOwnedTags.AppendTags(Parent->DefaultOwnedTags);
    DefaultFactions.AppendTags(Parent->DefaultFactions);

    // Use parent values for unset fields
    if (MinLevel == 1 && Parent->MinLevel != 1) MinLevel = Parent->MinLevel;
    if (MaxLevel == 10 && Parent->MaxLevel != 10) MaxLevel = Parent->MaxLevel;
    if (FMath::IsNearlyEqual(AttackPriority, 0.5f)) AttackPriority = Parent->AttackPriority;
}
```

---

## 4. CONTENT BROWSER INTEGRATION

### 4.1 Asset Factory: GasAbilityGeneratorFactories.h

Create: `Source/GasAbilityGenerator/Public/GasAbilityGeneratorFactories.h`

```cpp
// GasAbilityGeneratorFactories.h

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "GasAbilityGeneratorFactories.generated.h"

/**
 * Factory for creating NPC Package Spec assets
 */
UCLASS()
class GASABILITYGENERATOR_API UNPCPackageSpecFactory : public UFactory
{
    GENERATED_BODY()

public:
    UNPCPackageSpecFactory();

    virtual UObject* FactoryCreateNew(
        UClass* Class,
        UObject* InParent,
        FName Name,
        EObjectFlags Flags,
        UObject* Context,
        FFeedbackContext* Warn) override;

    virtual bool ShouldShowInNewMenu() const override { return true; }
    virtual FText GetDisplayName() const override;
    virtual uint32 GetMenuCategories() const override;
};

/**
 * Factory for creating Quest Spec assets
 */
UCLASS()
class GASABILITYGENERATOR_API UQuestSpecFactory : public UFactory
{
    GENERATED_BODY()

public:
    UQuestSpecFactory();

    virtual UObject* FactoryCreateNew(
        UClass* Class,
        UObject* InParent,
        FName Name,
        EObjectFlags Flags,
        UObject* Context,
        FFeedbackContext* Warn) override;

    virtual bool ShouldShowInNewMenu() const override { return true; }
    virtual FText GetDisplayName() const override;
    virtual uint32 GetMenuCategories() const override;
};

/**
 * Factory for creating Item Spec assets
 */
UCLASS()
class GASABILITYGENERATOR_API UItemSpecFactory : public UFactory
{
    GENERATED_BODY()

public:
    UItemSpecFactory();

    virtual UObject* FactoryCreateNew(
        UClass* Class,
        UObject* InParent,
        FName Name,
        EObjectFlags Flags,
        UObject* Context,
        FFeedbackContext* Warn) override;

    virtual bool ShouldShowInNewMenu() const override { return true; }
    virtual FText GetDisplayName() const override;
    virtual uint32 GetMenuCategories() const override;
};
```

### 4.2 Asset Factory Implementation

Create: `Source/GasAbilityGenerator/Private/GasAbilityGeneratorFactories.cpp`

```cpp
// GasAbilityGeneratorFactories.cpp

#include "GasAbilityGeneratorFactories.h"
#include "GasAbilityGeneratorSpecs.h"
#include "AssetToolsModule.h"

#define LOCTEXT_NAMESPACE "GasAbilityGenerator"

//=============================================================================
// NPC Package Spec Factory
//=============================================================================

UNPCPackageSpecFactory::UNPCPackageSpecFactory()
{
    SupportedClass = UNPCPackageSpec::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UNPCPackageSpecFactory::FactoryCreateNew(
    UClass* Class,
    UObject* InParent,
    FName Name,
    EObjectFlags Flags,
    UObject* Context,
    FFeedbackContext* Warn)
{
    return NewObject<UNPCPackageSpec>(InParent, Class, Name, Flags);
}

FText UNPCPackageSpecFactory::GetDisplayName() const
{
    return LOCTEXT("NPCPackageSpec", "NPC Package Spec");
}

uint32 UNPCPackageSpecFactory::GetMenuCategories() const
{
    // Custom category for GasAbilityGenerator
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    return AssetTools.FindAdvancedAssetCategory(FName("GasAbilityGenerator"));
}

//=============================================================================
// Quest Spec Factory
//=============================================================================

UQuestSpecFactory::UQuestSpecFactory()
{
    SupportedClass = UQuestSpec::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UQuestSpecFactory::FactoryCreateNew(
    UClass* Class,
    UObject* InParent,
    FName Name,
    EObjectFlags Flags,
    UObject* Context,
    FFeedbackContext* Warn)
{
    return NewObject<UQuestSpec>(InParent, Class, Name, Flags);
}

FText UQuestSpecFactory::GetDisplayName() const
{
    return LOCTEXT("QuestSpec", "Quest Spec");
}

uint32 UQuestSpecFactory::GetMenuCategories() const
{
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    return AssetTools.FindAdvancedAssetCategory(FName("GasAbilityGenerator"));
}

//=============================================================================
// Item Spec Factory
//=============================================================================

UItemSpecFactory::UItemSpecFactory()
{
    SupportedClass = UItemSpec::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UItemSpecFactory::FactoryCreateNew(
    UClass* Class,
    UObject* InParent,
    FName Name,
    EObjectFlags Flags,
    UObject* Context,
    FFeedbackContext* Warn)
{
    return NewObject<UItemSpec>(InParent, Class, Name, Flags);
}

FText UItemSpecFactory::GetDisplayName() const
{
    return LOCTEXT("ItemSpec", "Item Spec");
}

uint32 UItemSpecFactory::GetMenuCategories() const
{
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    return AssetTools.FindAdvancedAssetCategory(FName("GasAbilityGenerator"));
}

#undef LOCTEXT_NAMESPACE
```

### 4.3 Context Menu Registration

Add to: `Source/GasAbilityGenerator/Private/GasAbilityGeneratorModule.cpp`

```cpp
// Add includes
#include "GasAbilityGeneratorSpecs.h"
#include "ContentBrowserModule.h"
#include "ToolMenus.h"
#include "AssetToolsModule.h"

void FGasAbilityGeneratorModule::StartupModule()
{
    // ... existing code ...

    // Register asset category
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    GasAbilityGeneratorCategory = AssetTools.RegisterAdvancedAssetCategory(
        FName("GasAbilityGenerator"),
        LOCTEXT("GasAbilityGenerator", "GasAbilityGenerator"));

    // Register context menu extensions
    RegisterContentBrowserMenuExtensions();
}

void FGasAbilityGeneratorModule::RegisterContentBrowserMenuExtensions()
{
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
    {
        // NPC Package Spec context menu
        UToolMenu* NPCSpecMenu = UToolMenus::Get()->ExtendMenu(
            "ContentBrowser.AssetContextMenu.NPCPackageSpec");

        FToolMenuSection& NPCSection = NPCSpecMenu->FindOrAddSection("GasAbilityGenerator");
        NPCSection.Label = LOCTEXT("GasAbilityGenerator", "GasAbilityGenerator");

        NPCSection.AddMenuEntry(
            "GenerateFromNPCSpec",
            LOCTEXT("GenerateNPC", "Generate NPC Assets"),
            LOCTEXT("GenerateNPCTooltip", "Generate all assets defined in this NPC spec"),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"),
            FUIAction(FExecuteAction::CreateStatic(&FGasAbilityGeneratorModule::GenerateFromSelectedNPCSpecs))
        );

        NPCSection.AddMenuEntry(
            "ValidateNPCSpec",
            LOCTEXT("ValidateNPC", "Validate Spec"),
            LOCTEXT("ValidateNPCTooltip", "Check spec for errors without generating"),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Check"),
            FUIAction(FExecuteAction::CreateStatic(&FGasAbilityGeneratorModule::ValidateSelectedNPCSpecs))
        );

        NPCSection.AddMenuEntry(
            "PreviewNPCSpec",
            LOCTEXT("PreviewNPC", "Preview Generation Plan"),
            LOCTEXT("PreviewNPCTooltip", "Show what assets would be created or modified"),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Info"),
            FUIAction(FExecuteAction::CreateStatic(&FGasAbilityGeneratorModule::PreviewSelectedNPCSpecs))
        );

        // Quest Spec context menu
        UToolMenu* QuestSpecMenu = UToolMenus::Get()->ExtendMenu(
            "ContentBrowser.AssetContextMenu.QuestSpec");

        FToolMenuSection& QuestSection = QuestSpecMenu->FindOrAddSection("GasAbilityGenerator");
        QuestSection.Label = LOCTEXT("GasAbilityGenerator", "GasAbilityGenerator");

        QuestSection.AddMenuEntry(
            "GenerateFromQuestSpec",
            LOCTEXT("GenerateQuest", "Generate Quest"),
            LOCTEXT("GenerateQuestTooltip", "Generate quest blueprint from this spec"),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"),
            FUIAction(FExecuteAction::CreateStatic(&FGasAbilityGeneratorModule::GenerateFromSelectedQuestSpecs))
        );

        // Item Spec context menu
        UToolMenu* ItemSpecMenu = UToolMenus::Get()->ExtendMenu(
            "ContentBrowser.AssetContextMenu.ItemSpec");

        FToolMenuSection& ItemSection = ItemSpecMenu->FindOrAddSection("GasAbilityGenerator");
        ItemSection.Label = LOCTEXT("GasAbilityGenerator", "GasAbilityGenerator");

        ItemSection.AddMenuEntry(
            "GenerateFromItemSpec",
            LOCTEXT("GenerateItem", "Generate Item"),
            LOCTEXT("GenerateItemTooltip", "Generate item asset from this spec"),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"),
            FUIAction(FExecuteAction::CreateStatic(&FGasAbilityGeneratorModule::GenerateFromSelectedItemSpecs))
        );
    }));
}
```

---

## 5. GENERATION FLOW

### 5.1 Generation Functions

Add to: `Source/GasAbilityGenerator/Private/GasAbilityGeneratorModule.cpp`

```cpp
#include "ScopedTransaction.h"
#include "GasAbilityGeneratorGenerators.h"

void FGasAbilityGeneratorModule::GenerateFromSelectedNPCSpecs()
{
    // Get selected NPC specs from Content Browser
    TArray<FAssetData> SelectedAssets;
    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    TArray<UNPCPackageSpec*> Specs;
    for (const FAssetData& AssetData : SelectedAssets)
    {
        if (UNPCPackageSpec* Spec = Cast<UNPCPackageSpec>(AssetData.GetAsset()))
        {
            Specs.Add(Spec);
        }
    }

    if (Specs.Num() == 0)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("NoSpecsSelected", "No NPC Package Specs selected."));
        return;
    }

    // Wrap in transaction for undo support
    FScopedTransaction Transaction(LOCTEXT("GenerateFromNPCSpecs", "Generate from NPC Specs"));

    int32 SuccessCount = 0;
    int32 FailCount = 0;
    TArray<FString> GeneratedAssetPaths;

    for (UNPCPackageSpec* Spec : Specs)
    {
        // Merge with parent template if set
        UNPCPackageSpec* MergedSpec = DuplicateObject(Spec, GetTransientPackage());
        if (Spec->ParentTemplate.IsValid())
        {
            UNPCPackageSpec* Parent = Spec->ParentTemplate.LoadSynchronous();
            if (Parent)
            {
                MergedSpec->MergeWithParent(Parent);
            }
        }

        // Convert to manifest definition
        FManifestNPCPackageDefinition ManifestDef = MergedSpec->ToManifestDefinition();

        // Expand package to individual assets
        TArray<FExpandedAsset> ExpandedAssets = FNPCPackageGenerator::ExpandPackage(ManifestDef);

        // Generate each asset
        bool bSuccess = true;
        for (const FExpandedAsset& Asset : ExpandedAssets)
        {
            FGenerationResult Result = GenerateAsset(Asset);
            if (Result.bSuccess)
            {
                GeneratedAssetPaths.Add(Result.AssetPath);
            }
            else
            {
                bSuccess = false;
                UE_LOG(LogGasAbilityGenerator, Error, TEXT("Failed to generate: %s - %s"),
                    *Asset.Name, *Result.ErrorMessage);
            }
        }

        // Update spec with generated asset tracking
        if (bSuccess)
        {
            Spec->GeneratedAssets.Empty();
            for (const FString& Path : GeneratedAssetPaths)
            {
                Spec->GeneratedAssets.Add(FSoftObjectPath(Path));
            }
            Spec->LastGeneratedTime = FDateTime::Now();
            Spec->MarkPackageDirty();
            SuccessCount++;
        }
        else
        {
            FailCount++;
        }
    }

    // Show results dialog
    FString ResultMessage = FString::Printf(
        TEXT("Generation complete.\n\nSuccess: %d\nFailed: %d\n\nGenerated %d assets."),
        SuccessCount, FailCount, GeneratedAssetPaths.Num());

    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ResultMessage));

    // Optionally open generated assets
    if (GeneratedAssetPaths.Num() > 0)
    {
        // Sync Content Browser to generated folder
        TArray<FString> FolderPaths;
        FolderPaths.Add(FPaths::GetPath(GeneratedAssetPaths[0]));
        ContentBrowserModule.Get().SyncBrowserToFolders(FolderPaths);
    }
}

void FGasAbilityGeneratorModule::ValidateSelectedNPCSpecs()
{
    TArray<FAssetData> SelectedAssets;
    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    TArray<FString> ValidationErrors;
    TArray<FString> ValidationWarnings;

    for (const FAssetData& AssetData : SelectedAssets)
    {
        if (UNPCPackageSpec* Spec = Cast<UNPCPackageSpec>(AssetData.GetAsset()))
        {
            // Validate required fields
            if (Spec->BaseName.IsEmpty())
            {
                ValidationErrors.Add(FString::Printf(TEXT("%s: BaseName is required"),
                    *AssetData.AssetName.ToString()));
            }
            if (Spec->NPCId.IsEmpty())
            {
                ValidationErrors.Add(FString::Printf(TEXT("%s: NPCId is required"),
                    *AssetData.AssetName.ToString()));
            }

            // Validate schedule times
            for (int32 i = 0; i < Spec->Schedule.Num(); i++)
            {
                const FSpecScheduleEntry& Entry = Spec->Schedule[i];
                if (!Entry.GoalClass)
                {
                    ValidationWarnings.Add(FString::Printf(TEXT("%s: Schedule[%d] has no goal class"),
                        *AssetData.AssetName.ToString(), i));
                }
            }

            // Validate vendor config
            if (Spec->bIsVendor && Spec->ShopName.IsEmpty())
            {
                ValidationWarnings.Add(FString::Printf(TEXT("%s: Vendor enabled but ShopName is empty"),
                    *AssetData.AssetName.ToString()));
            }
        }
    }

    // Show validation results
    FString ResultMessage;
    if (ValidationErrors.Num() == 0 && ValidationWarnings.Num() == 0)
    {
        ResultMessage = TEXT("Validation passed! No errors or warnings.");
    }
    else
    {
        if (ValidationErrors.Num() > 0)
        {
            ResultMessage += TEXT("ERRORS:\n");
            for (const FString& Error : ValidationErrors)
            {
                ResultMessage += FString::Printf(TEXT("  - %s\n"), *Error);
            }
        }
        if (ValidationWarnings.Num() > 0)
        {
            ResultMessage += TEXT("\nWARNINGS:\n");
            for (const FString& Warning : ValidationWarnings)
            {
                ResultMessage += FString::Printf(TEXT("  - %s\n"), *Warning);
            }
        }
    }

    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ResultMessage));
}

void FGasAbilityGeneratorModule::PreviewSelectedNPCSpecs()
{
    TArray<FAssetData> SelectedAssets;
    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

    FString PreviewMessage = TEXT("Generation Plan:\n\n");

    for (const FAssetData& AssetData : SelectedAssets)
    {
        if (UNPCPackageSpec* Spec = Cast<UNPCPackageSpec>(AssetData.GetAsset()))
        {
            PreviewMessage += FString::Printf(TEXT("=== %s ===\n"), *Spec->BaseName);

            // List what would be generated
            PreviewMessage += FString::Printf(TEXT("  NPCDefinition: %s\n"), *Spec->GetNPCDefName());

            if (Spec->Abilities.Num() > 0)
            {
                PreviewMessage += FString::Printf(TEXT("  AbilityConfig: %s\n"), *Spec->GetACName());
            }

            if (Spec->Activities.Num() > 0)
            {
                PreviewMessage += FString::Printf(TEXT("  ActivityConfig: %s\n"), *Spec->GetActConfigName());
            }

            for (const FSpecGoalDefinition& Goal : Spec->Goals)
            {
                PreviewMessage += FString::Printf(TEXT("  Goal: %s\n"), *Spec->GetGoalName(Goal.Id));
            }

            if (Spec->Schedule.Num() > 0)
            {
                PreviewMessage += FString::Printf(TEXT("  Schedule: %s\n"), *Spec->GetScheduleName());
            }

            if (Spec->bAutoCreateDialogue)
            {
                PreviewMessage += FString::Printf(TEXT("  Dialogue: %s\n"), *Spec->GetDialogueName());
            }

            if (Spec->bAutoCreateTaggedDialogue)
            {
                PreviewMessage += FString::Printf(TEXT("  TaggedDialogue: %s\n"), *Spec->GetTaggedDialogueName());
            }

            for (const FSpecQuestDefinition& Quest : Spec->Quests)
            {
                PreviewMessage += FString::Printf(TEXT("  Quest: %s\n"), *Spec->GetQuestName(Quest.Id));
            }

            PreviewMessage += TEXT("\n");
        }
    }

    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(PreviewMessage));
}
```

---

## 6. TEMPLATE INHERITANCE

### Usage Example

```
Content Browser:
├── Specs/Templates/
│   └── NPC_Template_Merchant.uasset    ← Base merchant template
│       bIsVendor = true
│       Schedule = [Work 6-18]
│       Factions = [Friendly, Town]
│
└── Specs/NPCs/
    ├── NPC_Blacksmith_Spec.uasset      ← Inherits from Merchant
    │   ParentTemplate = NPC_Template_Merchant
    │   BaseName = "Blacksmith"          ← Override
    │   Abilities = [GA_Craft]           ← Override
    │   ShopInventory = [weapons]        ← Override
    │
    └── NPC_Alchemist_Spec.uasset       ← Also inherits from Merchant
        ParentTemplate = NPC_Template_Merchant
        BaseName = "Alchemist"
        Abilities = [GA_Brew]
        ShopInventory = [potions]
```

### Merge Rules

| Field Type | Merge Behavior |
|------------|----------------|
| Arrays (empty) | Use parent's array |
| Arrays (non-empty) | Use child's array (override) |
| Single values (default) | Use parent's value |
| Single values (set) | Use child's value (override) |
| Tag containers | Combine (append parent to child) |
| Booleans | Child overrides parent |

---

## 7. YAML MIGRATION

### 7.1 Import Existing Manifest

```cpp
// GasAbilityGeneratorMigration.cpp

void ImportManifestToSpecs(const FString& ManifestPath, const FString& OutputFolder)
{
    // Parse existing manifest
    FManifestData Manifest;
    FGasAbilityGeneratorParser::Parse(ManifestPath, Manifest);

    // Convert NPC packages to specs
    for (const FManifestNPCPackageDefinition& NPCPkg : Manifest.NPCPackages)
    {
        UNPCPackageSpec* Spec = NewObject<UNPCPackageSpec>();

        // Copy fields
        Spec->BaseName = NPCPkg.Name;
        Spec->OutputFolder = NPCPkg.Folder;
        Spec->NPCId = NPCPkg.NPCId;
        Spec->NPCDisplayName = FText::FromString(NPCPkg.NPCName);
        // ... convert all fields

        // Save as asset
        FString AssetPath = FString::Printf(TEXT("%s/NPC_%s_Spec"),
            *OutputFolder, *NPCPkg.Name);
        SaveSpecAsset(Spec, AssetPath);
    }

    // Convert standalone quests
    for (const FManifestQuestDefinition& QuestDef : Manifest.Quests)
    {
        UQuestSpec* Spec = NewObject<UQuestSpec>();
        // ... convert fields
        SaveSpecAsset(Spec, FString::Printf(TEXT("%s/Quest_%s_Spec"),
            *OutputFolder, *QuestDef.Name));
    }
}
```

### 7.2 Export Specs to YAML

```cpp
void ExportSpecsToManifest(const FString& SpecsFolder, const FString& OutputPath)
{
    FManifestData Manifest;

    // Find all spec assets
    TArray<FAssetData> SpecAssets;
    FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    AssetRegistry.Get().GetAssetsByPath(FName(*SpecsFolder), SpecAssets, true);

    for (const FAssetData& AssetData : SpecAssets)
    {
        if (UNPCPackageSpec* NPCSpec = Cast<UNPCPackageSpec>(AssetData.GetAsset()))
        {
            Manifest.NPCPackages.Add(NPCSpec->ToManifestDefinition());
        }
        else if (UQuestSpec* QuestSpec = Cast<UQuestSpec>(AssetData.GetAsset()))
        {
            Manifest.Quests.Add(QuestSpec->ToManifestDefinition());
        }
    }

    // Write YAML
    FGasAbilityGeneratorParser::WriteYAML(Manifest, OutputPath);
}
```

---

## 8. IMPLEMENTATION ORDER

### Phase 1: Core Specs (2-3 days)

| Task | Files | LOC |
|------|-------|-----|
| Create UNPCPackageSpec | GasAbilityGeneratorSpecs.h/cpp | ~400 |
| Create factories | GasAbilityGeneratorFactories.h/cpp | ~150 |
| Register asset category | GasAbilityGeneratorModule.cpp | ~20 |
| ToManifestDefinition() | GasAbilityGeneratorSpecs.cpp | ~200 |
| MergeWithParent() | GasAbilityGeneratorSpecs.cpp | ~50 |

### Phase 2: Context Menu (1 day)

| Task | Files | LOC |
|------|-------|-----|
| Register context menus | GasAbilityGeneratorModule.cpp | ~100 |
| GenerateFromSelectedNPCSpecs | GasAbilityGeneratorModule.cpp | ~100 |
| ValidateSelectedNPCSpecs | GasAbilityGeneratorModule.cpp | ~80 |
| PreviewSelectedNPCSpecs | GasAbilityGeneratorModule.cpp | ~60 |

### Phase 3: Additional Specs (1 day)

| Task | Files | LOC |
|------|-------|-----|
| UQuestSpec | GasAbilityGeneratorSpecs.h/cpp | ~150 |
| UItemSpec | GasAbilityGeneratorSpecs.h/cpp | ~150 |
| Context menus for Quest/Item | GasAbilityGeneratorModule.cpp | ~100 |

### Phase 4: Migration Tools (Optional)

| Task | Files | LOC |
|------|-------|-----|
| ImportManifestToSpecs | GasAbilityGeneratorMigration.cpp | ~200 |
| ExportSpecsToManifest | GasAbilityGeneratorMigration.cpp | ~100 |

**Total Estimated: ~1800 LOC, 4-5 days**

---

## 9. FILE CHANGES SUMMARY

### New Files

| File | Purpose |
|------|---------|
| `Public/GasAbilityGeneratorSpecs.h` | Spec DataAsset class definitions |
| `Private/GasAbilityGeneratorSpecs.cpp` | Spec implementation |
| `Public/GasAbilityGeneratorFactories.h` | Asset factories for Content Browser |
| `Private/GasAbilityGeneratorFactories.cpp` | Factory implementation |
| `Private/GasAbilityGeneratorMigration.cpp` | YAML import/export (optional) |

### Modified Files

| File | Changes |
|------|---------|
| `GasAbilityGeneratorModule.cpp` | Register asset category, context menus |
| `GasAbilityGenerator.Build.cs` | No changes needed (existing deps sufficient) |

### Dependencies (Already in Build.cs)

- `UnrealEd` - Factory, AssetTools
- `AssetTools` - RegisterAdvancedAssetCategory
- `ContentBrowser` - GetSelectedAssets
- `Slate`, `SlateCore` - UI
- `GameplayTags` - FGameplayTagContainer
- `GameplayAbilities` - TSubclassOf<UGameplayAbility>

---

## APPENDIX: UPROPERTY Meta Tags Reference

| Meta Tag | Effect |
|----------|--------|
| `EditCondition = "bIsVendor"` | Field only editable when condition true |
| `ClampMin = 0, ClampMax = 2400` | Value clamped in Details panel |
| `MultiLine = true` | Text field shows multi-line editor |
| `AdvancedDisplay` | Field hidden in "Simple" view |
| `AllowAbstract = false` | TSubclassOf excludes abstract classes |

---

**Document Version:** 1.0
**Created:** January 2026
**Status:** Ready for Implementation
**Estimated Effort:** 4-5 days
**Prerequisite:** NPC_Package_Generator_Handoff.md structs (FManifestNPCPackage*)
