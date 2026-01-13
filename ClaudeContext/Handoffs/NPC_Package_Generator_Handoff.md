# NPC Package Generator Handoff

**Single-Manifest NPC Pipeline** | January 2026 | Design Document

This document describes the NPC Package concept - a new manifest section that enables defining a complete NPC with all related assets in one place.

---

## TABLE OF CONTENTS

1. [Vision](#1-vision)
2. [Current State vs Target State](#2-current-vs-target)
3. [NPC Package Schema](#3-npc-package-schema)
4. [Asset Expansion Logic](#4-asset-expansion-logic)
5. [Implementation Architecture](#5-implementation-architecture)
6. [Cross-Reference System](#6-cross-reference-system)
7. [Three Interface Modes](#7-three-interface-modes)
8. [Implementation Code](#8-implementation-code)

---

## 1. VISION

### The Problem
Currently, creating a complete NPC requires defining 6-10 separate manifest entries:
- NPCDefinition
- AbilityConfiguration
- ActivityConfiguration
- Dialogue
- TaggedDialogueSet
- Quest(s)
- Schedule(s)
- Goal(s)

These must be coordinated manually with correct cross-references.

### The Solution
A single `npc_packages` section that:
1. Defines all NPC data in one place
2. Auto-generates all related assets
3. Auto-populates cross-references
4. Supports both inline definitions and external references

### The Three Interfaces

1. **Batch File (manifest.yaml)** - Define in YAML, run commandlet
2. **Editor UI** - "New NPC Package" wizard in Unreal
3. **Claude Prompt** - "Create a blacksmith NPC with a quest" → YAML → commandlet

---

## 2. CURRENT VS TARGET

### Current Manifest Structure

```yaml
# Must define each asset separately and coordinate references

ability_configurations:
  - name: AC_Blacksmith
    abilities: [GA_Craft]

activity_configurations:
  - name: ActConfig_Blacksmith
    activities: [BPA_Work, BPA_Idle]

goal_items:
  - name: Goal_Work
    default_score: 75.0

activity_schedules:
  - name: Schedule_Blacksmith
    behaviors:
      - start_time: 600
        goal_class: Goal_Work

dialogue_blueprints:
  - name: DBP_Blacksmith
    dialogue_tree:
      root: greeting
      # ... 50+ lines of dialogue definition

quests:
  - name: Quest_ForgeSupplies
    # ... 30+ lines of quest definition

npc_definitions:
  - name: NPCDef_Blacksmith
    ability_configuration: AC_Blacksmith      # Manual reference
    activity_configuration: ActConfig_Blacksmith
    activity_schedules: [Schedule_Blacksmith]
    # ...
```

### Target: NPC Package Structure

```yaml
# Single entry generates ALL assets with auto cross-references

npc_packages:
  - name: Blacksmith
    folder: NPCs/Town

    # === IDENTITY ===
    npc_id: Blacksmith_01
    npc_name: "Garrett the Blacksmith"
    npc_blueprint: BP_Blacksmith

    # === ABILITIES (generates AC_Blacksmith) ===
    abilities: [GA_Craft, GA_Repair]
    startup_effects: [GE_NPCStats]

    # === ACTIVITIES (generates ActConfig_Blacksmith + Goal_ assets) ===
    activities: [BPA_Work, BPA_Eat, BPA_Sleep, BPA_Idle]
    goals:
      work:
        score: 75
        owned_tags: [State.Working]
      eat:
        score: 50
      sleep:
        score: 100

    # === SCHEDULE (generates Schedule_Blacksmith) ===
    schedule:
      - time: [600, 1200]
        goal: work
        location: Forge
      - time: [1200, 1300]
        goal: eat
        location: Tavern
      - time: [2200, 600]
        goal: sleep
        location: Home

    # === DIALOGUE (generates DBP_Blacksmith) ===
    dialogue:
      greeting: "Welcome to my forge!"
      nodes:
        - id: shop
          text: "Show me your wares"
          action: open_vendor
        - id: quest
          text: "Need any help?"
          start_quest: forge_supplies

    # === QUESTS (generates Quest_Blacksmith_ForgeSupplies) ===
    quests:
      forge_supplies:
        name: "Forge Supplies"
        description: "Gather iron ore"
        objectives:
          - find: IronOre
            quantity: 10
        rewards:
          currency: 100
          items: [EI_IronSword]

    # === VENDOR (populates TradingItemLoadout) ===
    vendor:
      shop_name: "Garrett's Forge"
      inventory:
        - EI_IronSword: 3
        - EI_SteelArmor: 2

    # === TAGS & FACTIONS ===
    tags: [State.Invulnerable]
    factions: [Friendly, Town]
```

### Generated Assets

From the single entry above, the generator creates:

| Asset | Name | Type |
|-------|------|------|
| 1 | `NPCDef_Blacksmith` | UNPCDefinition |
| 2 | `AC_Blacksmith` | UNarrativeAbilityConfiguration |
| 3 | `ActConfig_Blacksmith` | UNPCActivityConfiguration |
| 4 | `Goal_Blacksmith_Work` | UNPCGoalItem Blueprint |
| 5 | `Goal_Blacksmith_Eat` | UNPCGoalItem Blueprint |
| 6 | `Goal_Blacksmith_Sleep` | UNPCGoalItem Blueprint |
| 7 | `Schedule_Blacksmith` | UNPCActivitySchedule |
| 8 | `DBP_Blacksmith` | UDialogueBlueprint |
| 9 | `Blacksmith_TaggedDialogue` | UTaggedDialogueSet |
| 10 | `Quest_Blacksmith_ForgeSupplies` | UQuestBlueprint |

---

## 3. NPC PACKAGE SCHEMA

### Full Schema Definition

```cpp
// GasAbilityGeneratorTypes.h

USTRUCT()
struct FManifestNPCPackageGoalDefinition
{
    GENERATED_BODY()

    UPROPERTY()
    FString Id;

    UPROPERTY()
    float Score = 50.0f;

    UPROPERTY()
    TArray<FString> OwnedTags;

    UPROPERTY()
    TArray<FString> BlockTags;

    UPROPERTY()
    TArray<FString> RequireTags;

    UPROPERTY()
    float Lifetime = -1.0f;  // -1 = never expires

    UPROPERTY()
    bool bRemoveOnSucceeded = false;
};

USTRUCT()
struct FManifestNPCPackageScheduleEntry
{
    GENERATED_BODY()

    UPROPERTY()
    int32 StartTime = 0;  // 0-2400

    UPROPERTY()
    int32 EndTime = 0;

    UPROPERTY()
    FString Goal;  // Reference to goal id

    UPROPERTY()
    FString Location;

    UPROPERTY()
    float ScoreOverride = -1.0f;  // -1 = use goal default
};

USTRUCT()
struct FManifestNPCPackageDialogueNode
{
    GENERATED_BODY()

    UPROPERTY()
    FString Id;

    UPROPERTY()
    FString Type = TEXT("npc");  // npc or player

    UPROPERTY()
    FString Text;

    UPROPERTY()
    FString OptionText;  // For player nodes

    UPROPERTY()
    TArray<FString> Replies;  // Child node IDs

    UPROPERTY()
    FString Action;  // open_vendor, start_quest, etc

    UPROPERTY()
    FString StartQuest;

    UPROPERTY()
    FString CompleteQuestBranch;
};

USTRUCT()
struct FManifestNPCPackageQuestObjective
{
    GENERATED_BODY()

    UPROPERTY()
    FString Find;  // Item class to find

    UPROPERTY()
    FString Kill;  // Enemy type to kill

    UPROPERTY()
    FString TalkTo;  // NPC to talk to

    UPROPERTY()
    FString GoTo;  // Location to reach

    UPROPERTY()
    int32 Quantity = 1;

    UPROPERTY()
    bool bOptional = false;
};

USTRUCT()
struct FManifestNPCPackageQuestReward
{
    GENERATED_BODY()

    UPROPERTY()
    int32 Currency = 0;

    UPROPERTY()
    int32 XP = 0;

    UPROPERTY()
    TArray<FString> Items;
};

USTRUCT()
struct FManifestNPCPackageQuestDefinition
{
    GENERATED_BODY()

    UPROPERTY()
    FString Id;

    UPROPERTY()
    FString Name;

    UPROPERTY()
    FString Description;

    UPROPERTY()
    TArray<FManifestNPCPackageQuestObjective> Objectives;

    UPROPERTY()
    FManifestNPCPackageQuestReward Rewards;

    UPROPERTY()
    bool bTracked = true;
};

USTRUCT()
struct FManifestNPCPackageVendorItem
{
    GENERATED_BODY()

    UPROPERTY()
    FString Item;

    UPROPERTY()
    int32 Quantity = 1;

    UPROPERTY()
    float Chance = 1.0f;
};

USTRUCT()
struct FManifestNPCPackageVendorConfig
{
    GENERATED_BODY()

    UPROPERTY()
    bool bEnabled = false;

    UPROPERTY()
    FString ShopName;

    UPROPERTY()
    int32 Currency = 500;

    UPROPERTY()
    float BuyPercentage = 0.5f;

    UPROPERTY()
    float SellPercentage = 1.5f;

    UPROPERTY()
    TArray<FManifestNPCPackageVendorItem> Inventory;
};

USTRUCT()
struct FManifestNPCPackageDefinition
{
    GENERATED_BODY()

    // === IDENTITY ===
    UPROPERTY()
    FString Name;  // Base name (e.g., "Blacksmith")

    UPROPERTY()
    FString Folder;

    UPROPERTY()
    FString NPCId;

    UPROPERTY()
    FString NPCName;

    UPROPERTY()
    FString NPCBlueprint;  // BP_ class path

    // === ABILITY SYSTEM ===
    UPROPERTY()
    TArray<FString> Abilities;  // GA_ classes

    UPROPERTY()
    TArray<FString> StartupEffects;  // GE_ classes

    UPROPERTY()
    FString DefaultAttributes;  // GE_ class

    // === ACTIVITY SYSTEM ===
    UPROPERTY()
    TArray<FString> Activities;  // BPA_ classes

    UPROPERTY()
    TArray<FString> GoalGenerators;

    UPROPERTY()
    TMap<FString, FManifestNPCPackageGoalDefinition> Goals;

    // === SCHEDULE ===
    UPROPERTY()
    TArray<FManifestNPCPackageScheduleEntry> Schedule;

    // === DIALOGUE ===
    UPROPERTY()
    FString GreetingText;

    UPROPERTY()
    TArray<FManifestNPCPackageDialogueNode> DialogueNodes;

    UPROPERTY()
    bool bAutoCreateTaggedDialogue = true;

    // === QUESTS ===
    UPROPERTY()
    TMap<FString, FManifestNPCPackageQuestDefinition> Quests;

    // === VENDOR ===
    UPROPERTY()
    FManifestNPCPackageVendorConfig Vendor;

    // === TAGS & FACTIONS ===
    UPROPERTY()
    TArray<FString> DefaultTags;

    UPROPERTY()
    TArray<FString> Factions;

    // === MISC ===
    UPROPERTY()
    int32 MinLevel = 1;

    UPROPERTY()
    int32 MaxLevel = 10;

    UPROPERTY()
    float AttackPriority = 0.5f;

    UPROPERTY()
    bool bAllowMultipleInstances = false;

    // === COMPUTED ===
    FString GetAssetPrefix() const { return Name; }
    FString GetNPCDefName() const { return FString::Printf(TEXT("NPCDef_%s"), *Name); }
    FString GetACName() const { return FString::Printf(TEXT("AC_%s"), *Name); }
    FString GetActConfigName() const { return FString::Printf(TEXT("ActConfig_%s"), *Name); }
    FString GetScheduleName() const { return FString::Printf(TEXT("Schedule_%s"), *Name); }
    FString GetDialogueName() const { return FString::Printf(TEXT("DBP_%s"), *Name); }
    FString GetTaggedDialogueName() const { return FString::Printf(TEXT("%s_TaggedDialogue"), *Name); }
    FString GetGoalName(const FString& GoalId) const
    {
        return FString::Printf(TEXT("Goal_%s_%s"), *Name, *GoalId);
    }
    FString GetQuestName(const FString& QuestId) const
    {
        return FString::Printf(TEXT("Quest_%s_%s"), *Name, *QuestId);
    }
};
```

---

## 4. ASSET EXPANSION LOGIC

### Expansion Pipeline

```cpp
// FNPCPackageGenerator::ExpandPackage

TArray<FManifestAssetDefinition> FNPCPackageGenerator::ExpandPackage(
    const FManifestNPCPackageDefinition& Package)
{
    TArray<FManifestAssetDefinition> ExpandedAssets;

    // 1. Expand Goals
    for (const auto& GoalPair : Package.Goals)
    {
        FManifestGoalItemDefinition GoalDef = ExpandGoal(Package, GoalPair.Key, GoalPair.Value);
        ExpandedAssets.Add(MakeAsset(GoalDef));
    }

    // 2. Expand Schedule (depends on goals)
    if (Package.Schedule.Num() > 0)
    {
        FManifestActivityScheduleDefinition ScheduleDef = ExpandSchedule(Package);
        ExpandedAssets.Add(MakeAsset(ScheduleDef));
    }

    // 3. Expand AbilityConfiguration
    if (Package.Abilities.Num() > 0)
    {
        FManifestAbilityConfigurationDefinition ACDef = ExpandAbilityConfig(Package);
        ExpandedAssets.Add(MakeAsset(ACDef));
    }

    // 4. Expand ActivityConfiguration
    if (Package.Activities.Num() > 0)
    {
        FManifestActivityConfigurationDefinition ActConfigDef = ExpandActivityConfig(Package);
        ExpandedAssets.Add(MakeAsset(ActConfigDef));
    }

    // 5. Expand Dialogue
    if (Package.DialogueNodes.Num() > 0 || !Package.GreetingText.IsEmpty())
    {
        FManifestDialogueBlueprintDefinition DialogueDef = ExpandDialogue(Package);
        ExpandedAssets.Add(MakeAsset(DialogueDef));
    }

    // 6. Expand Quests
    for (const auto& QuestPair : Package.Quests)
    {
        FManifestQuestDefinition QuestDef = ExpandQuest(Package, QuestPair.Key, QuestPair.Value);
        ExpandedAssets.Add(MakeAsset(QuestDef));
    }

    // 7. Expand NPC Definition (last - references all above)
    FManifestNPCDefinitionDefinition NPCDef = ExpandNPCDefinition(Package);
    ExpandedAssets.Add(MakeAsset(NPCDef));

    return ExpandedAssets;
}
```

### Goal Expansion

```cpp
FManifestGoalItemDefinition FNPCPackageGenerator::ExpandGoal(
    const FManifestNPCPackageDefinition& Package,
    const FString& GoalId,
    const FManifestNPCPackageGoalDefinition& GoalPkg)
{
    FManifestGoalItemDefinition GoalDef;
    GoalDef.Name = Package.GetGoalName(GoalId);
    GoalDef.Folder = Package.Folder + TEXT("/Goals");
    GoalDef.ParentClass = TEXT("NPCGoalItem");
    GoalDef.DefaultScore = GoalPkg.Score;
    GoalDef.GoalLifetime = GoalPkg.Lifetime;
    GoalDef.bRemoveOnSucceeded = GoalPkg.bRemoveOnSucceeded;
    GoalDef.OwnedTags = GoalPkg.OwnedTags;
    GoalDef.BlockTags = GoalPkg.BlockTags;
    GoalDef.RequireTags = GoalPkg.RequireTags;
    return GoalDef;
}
```

### Schedule Expansion

```cpp
FManifestActivityScheduleDefinition FNPCPackageGenerator::ExpandSchedule(
    const FManifestNPCPackageDefinition& Package)
{
    FManifestActivityScheduleDefinition ScheduleDef;
    ScheduleDef.Name = Package.GetScheduleName();
    ScheduleDef.Folder = Package.Folder + TEXT("/Schedules");

    for (const FManifestNPCPackageScheduleEntry& Entry : Package.Schedule)
    {
        FManifestScheduledBehaviorDefinition Behavior;
        Behavior.StartTime = Entry.StartTime;
        Behavior.EndTime = Entry.EndTime;
        Behavior.GoalClass = Package.GetGoalName(Entry.Goal);  // Auto-reference
        Behavior.Location = Entry.Location;
        Behavior.ScoreOverride = Entry.ScoreOverride;
        ScheduleDef.Behaviors.Add(Behavior);
    }

    return ScheduleDef;
}
```

### Quest Expansion

```cpp
FManifestQuestDefinition FNPCPackageGenerator::ExpandQuest(
    const FManifestNPCPackageDefinition& Package,
    const FString& QuestId,
    const FManifestNPCPackageQuestDefinition& QuestPkg)
{
    FManifestQuestDefinition QuestDef;
    QuestDef.Name = Package.GetQuestName(QuestId);
    QuestDef.Folder = Package.Folder + TEXT("/Quests");
    QuestDef.QuestName = QuestPkg.Name;
    QuestDef.QuestDescription = QuestPkg.Description;
    QuestDef.bTracked = QuestPkg.bTracked;
    QuestDef.QuestGiver = Package.GetNPCDefName();  // Auto-reference!
    QuestDef.Dialogue = Package.GetDialogueName();  // Auto-reference!

    // Create states from objectives
    FManifestQuestStateDefinition StartState;
    StartState.Id = TEXT("Start");
    StartState.Type = TEXT("regular");
    StartState.Description = QuestPkg.Objectives.Num() > 0 ?
        QuestPkg.Objectives[0].GetDescription() : TEXT("Begin quest");
    QuestDef.States.Add(StartState);

    // ... expand objectives to states/branches/tasks

    // Add success state
    FManifestQuestStateDefinition SuccessState;
    SuccessState.Id = TEXT("Complete");
    SuccessState.Type = TEXT("success");
    SuccessState.Description = TEXT("Quest completed!");
    QuestDef.States.Add(SuccessState);

    // Set rewards
    QuestDef.Rewards.Currency = QuestPkg.Rewards.Currency;
    QuestDef.Rewards.XP = QuestPkg.Rewards.XP;
    QuestDef.Rewards.Items = QuestPkg.Rewards.Items;

    return QuestDef;
}
```

### NPC Definition Expansion

```cpp
FManifestNPCDefinitionDefinition FNPCPackageGenerator::ExpandNPCDefinition(
    const FManifestNPCPackageDefinition& Package)
{
    FManifestNPCDefinitionDefinition NPCDef;
    NPCDef.Name = Package.GetNPCDefName();
    NPCDef.Folder = Package.Folder;
    NPCDef.NPCId = Package.NPCId;
    NPCDef.NPCName = Package.NPCName;
    NPCDef.NPCClassPath = Package.NPCBlueprint;

    // Auto-wire configurations
    if (Package.Abilities.Num() > 0)
    {
        NPCDef.AbilityConfiguration = Package.GetACName();
    }
    if (Package.Activities.Num() > 0)
    {
        NPCDef.ActivityConfiguration = Package.GetActConfigName();
    }
    if (Package.Schedule.Num() > 0)
    {
        NPCDef.ActivitySchedules.Add(Package.GetScheduleName());
    }
    if (Package.DialogueNodes.Num() > 0)
    {
        NPCDef.Dialogue = Package.GetDialogueName();
    }
    if (Package.bAutoCreateTaggedDialogue)
    {
        NPCDef.TaggedDialogueSet = Package.GetTaggedDialogueName();
    }

    // Vendor config
    NPCDef.bIsVendor = Package.Vendor.bEnabled;
    if (NPCDef.bIsVendor)
    {
        NPCDef.ShopFriendlyName = Package.Vendor.ShopName;
        NPCDef.TradingCurrency = Package.Vendor.Currency;
        NPCDef.BuyItemPercentage = Package.Vendor.BuyPercentage;
        NPCDef.SellItemPercentage = Package.Vendor.SellPercentage;

        for (const auto& VendorItem : Package.Vendor.Inventory)
        {
            FManifestLootTableRollDefinition Roll;
            Roll.Item = VendorItem.Item;
            Roll.Quantity = VendorItem.Quantity;
            Roll.RollChance = VendorItem.Chance;
            NPCDef.TradingItemLoadout.Add(Roll);
        }
    }

    // Tags and factions
    NPCDef.DefaultOwnedTags = Package.DefaultTags;
    NPCDef.DefaultFactions = Package.Factions;

    // Misc
    NPCDef.MinLevel = Package.MinLevel;
    NPCDef.MaxLevel = Package.MaxLevel;
    NPCDef.AttackPriority = Package.AttackPriority;
    NPCDef.bAllowMultipleInstances = Package.bAllowMultipleInstances;

    return NPCDef;
}
```

---

## 5. IMPLEMENTATION ARCHITECTURE

### Generator Integration

```cpp
// In commandlet/window:

void GenerateFromManifest(const FManifestData& Manifest)
{
    // First, expand all NPC packages into individual assets
    TArray<FExpandedAsset> ExpandedAssets;

    for (const FManifestNPCPackageDefinition& Package : Manifest.NPCPackages)
    {
        TArray<FManifestAssetDefinition> PackageAssets =
            FNPCPackageGenerator::ExpandPackage(Package);

        for (const auto& Asset : PackageAssets)
        {
            ExpandedAssets.Add(Asset);
        }
    }

    // Merge expanded assets with directly-defined assets
    // (Deduplication: directly-defined takes precedence)
    FManifestData MergedManifest = MergeManifests(Manifest, ExpandedAssets);

    // Continue with normal generation
    GenerateTags(MergedManifest);
    GenerateEnumerations(MergedManifest);
    // ... etc
}
```

### Deduplication Logic

```cpp
// If user defines both in package AND directly, direct definition wins

FManifestData MergeManifests(
    const FManifestData& Direct,
    const TArray<FExpandedAsset>& Expanded)
{
    FManifestData Merged = Direct;

    // For each expanded asset, only add if not already defined
    for (const FExpandedAsset& Asset : Expanded)
    {
        switch (Asset.Type)
        {
            case EAssetType::Goal:
            {
                bool bExists = Direct.Goals.ContainsByPredicate(
                    [&](const FManifestGoalItemDefinition& G) { return G.Name == Asset.Goal.Name; });
                if (!bExists)
                {
                    Merged.Goals.Add(Asset.Goal);
                }
                break;
            }
            // ... other types
        }
    }

    return Merged;
}
```

---

## 6. CROSS-REFERENCE SYSTEM

### Relationship Tracking

```cpp
// Build relationship map during expansion

struct FNPCRelationships
{
    // NPC -> Quests given by this NPC
    TMap<FString, TArray<FString>> NPCToQuests;

    // Quest -> NPC who gives it
    TMap<FString, FString> QuestToNPC;

    // NPC -> Dialogue
    TMap<FString, FString> NPCToDialogue;

    // Schedule -> Goals used
    TMap<FString, TArray<FString>> ScheduleToGoals;
};

FNPCRelationships BuildRelationships(const FManifestData& Manifest)
{
    FNPCRelationships Rels;

    // From packages
    for (const auto& Package : Manifest.NPCPackages)
    {
        FString NPCName = Package.GetNPCDefName();

        for (const auto& QuestPair : Package.Quests)
        {
            FString QuestName = Package.GetQuestName(QuestPair.Key);
            Rels.NPCToQuests.FindOrAdd(NPCName).Add(QuestName);
            Rels.QuestToNPC.Add(QuestName, NPCName);
        }

        Rels.NPCToDialogue.Add(NPCName, Package.GetDialogueName());

        FString ScheduleName = Package.GetScheduleName();
        for (const auto& Entry : Package.Schedule)
        {
            Rels.ScheduleToGoals.FindOrAdd(ScheduleName).Add(
                Package.GetGoalName(Entry.Goal));
        }
    }

    // From direct definitions
    for (const auto& Quest : Manifest.Quests)
    {
        if (!Quest.QuestGiver.IsEmpty())
        {
            Rels.NPCToQuests.FindOrAdd(Quest.QuestGiver).Add(Quest.Name);
            Rels.QuestToNPC.Add(Quest.Name, Quest.QuestGiver);
        }
    }

    return Rels;
}
```

---

## 7. THREE INTERFACE MODES

### Mode 1: Batch File (manifest.yaml)

User writes YAML, runs commandlet:

```bash
UnrealEditor-Cmd.exe ... -run=GasAbilityGenerator -manifest="manifest.yaml"
```

### Mode 2: Editor UI

New button in GasAbilityGenerator window: **"New NPC Package"**

Opens wizard dialog with fields:
- NPC Name, ID
- Abilities (multi-select)
- Activities (multi-select)
- Schedule editor (time range + goal)
- Dialogue tree builder
- Quest builder
- Vendor config

Generates YAML section and appends to manifest, then runs generation.

### Mode 3: Claude Prompt

User describes NPC in natural language:

```
"Create a blacksmith NPC named Garrett who:
- Works at the forge from 6am to 6pm
- Eats lunch at noon
- Sells weapons and armor
- Has a quest to gather iron ore (reward: sword + 100 gold)"
```

Claude generates YAML:

```yaml
npc_packages:
  - name: Blacksmith
    npc_id: Blacksmith_01
    npc_name: "Garrett"
    # ... complete definition
```

User runs commandlet or Claude executes directly.

---

## 8. IMPLEMENTATION CODE

### Parser Addition

```cpp
// GasAbilityGeneratorParser.cpp

void FGasAbilityGeneratorParser::ParseNPCPackages(const YAML::Node& Root)
{
    if (!Root["npc_packages"]) return;

    for (const auto& PackageNode : Root["npc_packages"])
    {
        FManifestNPCPackageDefinition Package;

        // Identity
        if (PackageNode["name"])
            Package.Name = FString(PackageNode["name"].as<std::string>().c_str());
        if (PackageNode["folder"])
            Package.Folder = FString(PackageNode["folder"].as<std::string>().c_str());
        if (PackageNode["npc_id"])
            Package.NPCId = FString(PackageNode["npc_id"].as<std::string>().c_str());
        if (PackageNode["npc_name"])
            Package.NPCName = FString(PackageNode["npc_name"].as<std::string>().c_str());
        if (PackageNode["npc_blueprint"])
            Package.NPCBlueprint = FString(PackageNode["npc_blueprint"].as<std::string>().c_str());

        // Abilities
        if (PackageNode["abilities"])
        {
            for (const auto& AbilityNode : PackageNode["abilities"])
            {
                Package.Abilities.Add(FString(AbilityNode.as<std::string>().c_str()));
            }
        }

        // Activities
        if (PackageNode["activities"])
        {
            for (const auto& ActivityNode : PackageNode["activities"])
            {
                Package.Activities.Add(FString(ActivityNode.as<std::string>().c_str()));
            }
        }

        // Goals
        if (PackageNode["goals"])
        {
            for (YAML::const_iterator it = PackageNode["goals"].begin();
                 it != PackageNode["goals"].end(); ++it)
            {
                FString GoalId = FString(it->first.as<std::string>().c_str());
                FManifestNPCPackageGoalDefinition GoalDef;
                GoalDef.Id = GoalId;

                YAML::Node GoalNode = it->second;
                if (GoalNode["score"]) GoalDef.Score = GoalNode["score"].as<float>();
                if (GoalNode["owned_tags"])
                {
                    for (const auto& Tag : GoalNode["owned_tags"])
                    {
                        GoalDef.OwnedTags.Add(FString(Tag.as<std::string>().c_str()));
                    }
                }
                // ... other goal properties

                Package.Goals.Add(GoalId, GoalDef);
            }
        }

        // Schedule
        if (PackageNode["schedule"])
        {
            for (const auto& EntryNode : PackageNode["schedule"])
            {
                FManifestNPCPackageScheduleEntry Entry;

                if (EntryNode["time"] && EntryNode["time"].IsSequence())
                {
                    Entry.StartTime = EntryNode["time"][0].as<int32>();
                    Entry.EndTime = EntryNode["time"][1].as<int32>();
                }
                if (EntryNode["goal"])
                    Entry.Goal = FString(EntryNode["goal"].as<std::string>().c_str());
                if (EntryNode["location"])
                    Entry.Location = FString(EntryNode["location"].as<std::string>().c_str());

                Package.Schedule.Add(Entry);
            }
        }

        // Dialogue
        if (PackageNode["dialogue"])
        {
            YAML::Node DialogueNode = PackageNode["dialogue"];

            if (DialogueNode["greeting"])
                Package.GreetingText = FString(DialogueNode["greeting"].as<std::string>().c_str());

            if (DialogueNode["nodes"])
            {
                for (const auto& NodeDef : DialogueNode["nodes"])
                {
                    FManifestNPCPackageDialogueNode Node;
                    if (NodeDef["id"])
                        Node.Id = FString(NodeDef["id"].as<std::string>().c_str());
                    if (NodeDef["text"])
                        Node.Text = FString(NodeDef["text"].as<std::string>().c_str());
                    if (NodeDef["start_quest"])
                        Node.StartQuest = FString(NodeDef["start_quest"].as<std::string>().c_str());
                    // ... other properties

                    Package.DialogueNodes.Add(Node);
                }
            }
        }

        // Quests
        if (PackageNode["quests"])
        {
            for (YAML::const_iterator it = PackageNode["quests"].begin();
                 it != PackageNode["quests"].end(); ++it)
            {
                FString QuestId = FString(it->first.as<std::string>().c_str());
                FManifestNPCPackageQuestDefinition QuestDef;
                QuestDef.Id = QuestId;

                YAML::Node QuestNode = it->second;
                if (QuestNode["name"])
                    QuestDef.Name = FString(QuestNode["name"].as<std::string>().c_str());
                if (QuestNode["description"])
                    QuestDef.Description = FString(QuestNode["description"].as<std::string>().c_str());

                // Objectives
                if (QuestNode["objectives"])
                {
                    for (const auto& ObjNode : QuestNode["objectives"])
                    {
                        FManifestNPCPackageQuestObjective Obj;
                        if (ObjNode["find"])
                            Obj.Find = FString(ObjNode["find"].as<std::string>().c_str());
                        if (ObjNode["quantity"])
                            Obj.Quantity = ObjNode["quantity"].as<int32>();
                        QuestDef.Objectives.Add(Obj);
                    }
                }

                // Rewards
                if (QuestNode["rewards"])
                {
                    YAML::Node RewardsNode = QuestNode["rewards"];
                    if (RewardsNode["currency"])
                        QuestDef.Rewards.Currency = RewardsNode["currency"].as<int32>();
                    if (RewardsNode["xp"])
                        QuestDef.Rewards.XP = RewardsNode["xp"].as<int32>();
                    if (RewardsNode["items"])
                    {
                        for (const auto& ItemNode : RewardsNode["items"])
                        {
                            QuestDef.Rewards.Items.Add(
                                FString(ItemNode.as<std::string>().c_str()));
                        }
                    }
                }

                Package.Quests.Add(QuestId, QuestDef);
            }
        }

        // Vendor
        if (PackageNode["vendor"])
        {
            YAML::Node VendorNode = PackageNode["vendor"];
            Package.Vendor.bEnabled = true;

            if (VendorNode["shop_name"])
                Package.Vendor.ShopName = FString(VendorNode["shop_name"].as<std::string>().c_str());
            if (VendorNode["currency"])
                Package.Vendor.Currency = VendorNode["currency"].as<int32>();
            if (VendorNode["inventory"])
            {
                for (const auto& ItemNode : VendorNode["inventory"])
                {
                    FManifestNPCPackageVendorItem Item;
                    // Parse item:quantity or {item, quantity, chance}
                    // ...
                    Package.Vendor.Inventory.Add(Item);
                }
            }
        }

        // Tags and factions
        if (PackageNode["tags"])
        {
            for (const auto& Tag : PackageNode["tags"])
            {
                Package.DefaultTags.Add(FString(Tag.as<std::string>().c_str()));
            }
        }
        if (PackageNode["factions"])
        {
            for (const auto& Faction : PackageNode["factions"])
            {
                Package.Factions.Add(FString(Faction.as<std::string>().c_str()));
            }
        }

        ManifestData.NPCPackages.Add(Package);
    }
}
```

---

## SUMMARY

The NPC Package generator transforms this:

**Before (10 separate definitions, ~200 lines):**
```yaml
ability_configurations: [...]
activity_configurations: [...]
goal_items: [...]
activity_schedules: [...]
dialogue_blueprints: [...]
quests: [...]
npc_definitions: [...]
```

**After (1 definition, ~50 lines):**
```yaml
npc_packages:
  - name: Blacksmith
    # Complete NPC definition
```

**Implementation Effort:**
- Structs: ~200 LOC
- Parser: ~300 LOC
- Expander: ~400 LOC
- Integration: ~100 LOC
- **Total: ~1000 LOC**

---

**Document Version:** 1.0
**Created:** January 2026
**Status:** Design Complete - Ready for Implementation
