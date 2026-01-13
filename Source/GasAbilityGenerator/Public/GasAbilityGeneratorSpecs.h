// GasAbilityGeneratorSpecs.h
// v4.0: Spec DataAssets for native UE workflow
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
//
// Core Invariants:
// - "Specs are canonical. Everything else is derived."
// - "Generators never mutate Specs."
// - "Paths > Names, Plans > Actions, One Canonical Source"

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GasAbilityGeneratorSpecs.generated.h"

// Forward declarations
class UGameplayAbility;
class UGameplayEffect;

//=============================================================================
// Schema Version - For future migrations
//=============================================================================

#define SPEC_SCHEMA_VERSION 1

//=============================================================================
// Supporting Structs
//=============================================================================

/**
 * Schedule entry for NPC daily routines
 * Time format: 0-2400 where 100 = 1 hour (e.g., 600 = 6:00 AM)
 */
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

	/** Goal class to pursue during this time (soft reference for rename safety) */
	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "/Script/NarrativeArsenal.NPCGoalItem"))
	FSoftClassPath GoalClass;

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

/**
 * Vendor item entry for shop inventory
 */
USTRUCT(BlueprintType)
struct FSpecVendorItem
{
	GENERATED_BODY()

	/** Item class to sell (soft reference for rename safety) */
	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "/Script/NarrativeArsenal.NarrativeItem"))
	FSoftClassPath Item;

	/** Quantity in stock */
	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	int32 Quantity = 1;

	/** Chance to have in stock (0.0 - 1.0) */
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0.0, ClampMax = 1.0))
	float Chance = 1.0f;
};

/**
 * Goal definition for NPC AI objectives
 */
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

/**
 * Dialogue node for conversation trees
 */
USTRUCT(BlueprintType)
struct FSpecDialogueNode
{
	GENERATED_BODY()

	/** Node identifier */
	UPROPERTY(EditAnywhere)
	FString Id;

	/** Node type: npc or player */
	UPROPERTY(EditAnywhere, meta = (GetOptions = "GetDialogueNodeTypes"))
	FString Type = TEXT("npc");

	/** Dialogue text */
	UPROPERTY(EditAnywhere, meta = (MultiLine = true))
	FText Text;

	/** Option text (for player choices) */
	UPROPERTY(EditAnywhere)
	FText OptionText;

	/** Child node IDs (replies/responses) */
	UPROPERTY(EditAnywhere)
	TArray<FString> Replies;

	/** Quest to start when this node is selected */
	UPROPERTY(EditAnywhere)
	FString StartQuest;

	/** Quest branch to complete when this node is selected */
	UPROPERTY(EditAnywhere)
	FString CompleteQuestBranch;
};

/**
 * Quest objective definition
 */
USTRUCT(BlueprintType)
struct FSpecQuestObjective
{
	GENERATED_BODY()

	/** Item to find (for BPT_FindItem task) */
	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "/Script/NarrativeArsenal.NarrativeItem"))
	FSoftClassPath FindItem;

	/** Quantity required */
	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	int32 Quantity = 1;

	/** Location to reach (for BPT_Move task) */
	UPROPERTY(EditAnywhere)
	FString GoToLocation;

	/** NPC to talk to (for BPT_FinishDialogue task) */
	UPROPERTY(EditAnywhere)
	FString TalkToNPC;

	/** Is this objective optional */
	UPROPERTY(EditAnywhere)
	bool bOptional = false;
};

/**
 * Quest reward definition
 */
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

	/** Items to grant (soft references for rename safety) */
	UPROPERTY(EditAnywhere)
	TArray<FSoftClassPath> Items;
};

/**
 * Quest definition for NPC-given quests
 */
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
 * 1. Create in Content Browser: Right-click -> Miscellaneous -> NPC Package Spec
 * 2. Configure in Details panel (dropdowns auto-populate from project assets)
 * 3. Right-click asset -> GasAbilityGenerator -> Generate NPC Assets
 *
 * Generates: NPCDef_, AC_, ActConfig_, Goal_, Schedule_, DBP_, Quest_ assets
 *
 * Core Invariant: This spec is canonical. Generated assets are derived.
 */
UCLASS(BlueprintType)
class GASABILITYGENERATOR_API UNPCPackageSpec : public UDataAsset
{
	GENERATED_BODY()

public:
	//=========================================================================
	// Schema Version
	//=========================================================================

	/** Schema version for migration support */
	UPROPERTY(VisibleAnywhere, Category = "Schema")
	int32 SpecSchemaVersion = SPEC_SCHEMA_VERSION;

	//=========================================================================
	// Template Inheritance
	//=========================================================================

	/** Parent template to inherit from (all fields merged) */
	UPROPERTY(EditAnywhere, Category = "Template")
	TSoftObjectPtr<UNPCPackageSpec> ParentTemplate;

	//=========================================================================
	// Identity
	//=========================================================================

	/** Base name for generated assets (e.g., "Blacksmith" -> NPCDef_Blacksmith) */
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

	/** NPC Blueprint class (soft reference for rename safety) */
	UPROPERTY(EditAnywhere, Category = "Identity", meta = (AllowedClasses = "/Script/NarrativeArsenal.NarrativeNPCCharacter"))
	FSoftClassPath NPCBlueprint;

	//=========================================================================
	// Abilities
	//=========================================================================

	/** Abilities granted to this NPC (populates AC_{BaseName}) */
	UPROPERTY(EditAnywhere, Category = "Abilities")
	TArray<FSoftClassPath> Abilities;

	/** Effects applied at spawn */
	UPROPERTY(EditAnywhere, Category = "Abilities")
	TArray<FSoftClassPath> StartupEffects;

	/** Default attribute set */
	UPROPERTY(EditAnywhere, Category = "Abilities")
	FSoftClassPath DefaultAttributes;

	//=========================================================================
	// Activities
	//=========================================================================

	/** Activities available to this NPC (populates ActConfig_{BaseName}) */
	UPROPERTY(EditAnywhere, Category = "Activities")
	TArray<FSoftClassPath> Activities;

	/** Goal generators */
	UPROPERTY(EditAnywhere, Category = "Activities")
	TArray<FSoftClassPath> GoalGenerators;

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

	/** Validate spec before generation */
	bool Validate(TArray<FString>& OutErrors) const;

	/** Merge parent template fields into this spec (returns merged copy) */
	UNPCPackageSpec* GetMergedSpec() const;

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
 *
 * For quests not tied to a specific NPC, or for complex multi-stage quests.
 */
UCLASS(BlueprintType)
class GASABILITYGENERATOR_API UQuestSpec : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Schema version for migration support */
	UPROPERTY(VisibleAnywhere, Category = "Schema")
	int32 SpecSchemaVersion = SPEC_SCHEMA_VERSION;

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

	/** Quest giver NPC definition (soft reference) */
	UPROPERTY(EditAnywhere, Category = "Quest")
	FSoftObjectPath QuestGiver;

	UPROPERTY(EditAnywhere, Category = "Quest")
	bool bTracked = true;

	UPROPERTY(EditAnywhere, Category = "Objectives")
	TArray<FSpecQuestObjective> Objectives;

	UPROPERTY(EditAnywhere, Category = "Rewards")
	FSpecQuestReward Rewards;

	UPROPERTY(VisibleAnywhere, Category = "Generated", AdvancedDisplay)
	TArray<FSoftObjectPath> GeneratedAssets;

	UPROPERTY(VisibleAnywhere, Category = "Generated", AdvancedDisplay)
	FDateTime LastGeneratedTime;

	/** Validate spec before generation */
	bool Validate(TArray<FString>& OutErrors) const;
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
	/** Schema version for migration support */
	UPROPERTY(VisibleAnywhere, Category = "Schema")
	int32 SpecSchemaVersion = SPEC_SCHEMA_VERSION;

	UPROPERTY(EditAnywhere, Category = "Template")
	TSoftObjectPtr<UItemSpec> ParentTemplate;

	UPROPERTY(EditAnywhere, Category = "Identity")
	FString ItemName;

	UPROPERTY(EditAnywhere, Category = "Identity")
	FString OutputFolder = TEXT("Items");

	/** Parent class (EquippableItem, WeaponItem, RangedWeaponItem, etc.) */
	UPROPERTY(EditAnywhere, Category = "Identity")
	FSoftClassPath ParentClass;

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

	/** Abilities granted when equipped */
	UPROPERTY(EditAnywhere, Category = "Abilities")
	TArray<FSoftClassPath> AbilitiesToGrant;

	UPROPERTY(VisibleAnywhere, Category = "Generated", AdvancedDisplay)
	TArray<FSoftObjectPath> GeneratedAssets;

	UPROPERTY(VisibleAnywhere, Category = "Generated", AdvancedDisplay)
	FDateTime LastGeneratedTime;

	/** Validate spec before generation */
	bool Validate(TArray<FString>& OutErrors) const;
};
