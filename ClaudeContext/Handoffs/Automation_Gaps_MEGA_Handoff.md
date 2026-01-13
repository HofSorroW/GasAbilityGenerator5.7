# Automation Gaps MEGA Handoff

**Complete Gap Analysis & Implementation Guide** | January 2026 | Ready for Implementation

This document contains ALL identified gaps between Narrative Pro headers and GasAbilityGenerator automation, with full implementation code for each enhancement.

---

## TABLE OF CONTENTS

1. [Executive Summary](#1-executive-summary)
2. [Quest Enhancements](#2-quest-enhancements)
3. [NPC Definition Enhancements](#3-npc-definition-enhancements)
4. [Dialogue Enhancements](#4-dialogue-enhancements)
5. [Item Enhancements](#5-item-enhancements)
6. [Activity Enhancements](#6-activity-enhancements)
7. [Goal Enhancements](#7-goal-enhancements)
8. [Weapon Enhancements](#8-weapon-enhancements)
9. [Cross-Asset Relationship System](#9-cross-asset-relationship-system)
10. [Implementation Order](#10-implementation-order)
11. [Testing Checklist](#11-testing-checklist)

---

## 1. EXECUTIVE SUMMARY

### Current State
GasAbilityGenerator v3.9.3 automates 25 asset types with ~80% property coverage. The remaining 20% consists of:
- Cross-asset relationships (quest-NPC, dialogue-quest)
- Complex nested structures (rewards, loadouts, camera configs)
- Advanced features (fragments, requirements, attachments)

### Gap Categories

| Priority | Category | Gaps | Complexity |
|----------|----------|------|------------|
| **HIGH** | Quest System | Rewards, Questgiver | Medium |
| **HIGH** | NPC System | TradingItemLoadout | Medium |
| **HIGH** | Dialogue System | Quest shortcuts | Low |
| **MEDIUM** | Item System | Usage properties | Low |
| **MEDIUM** | Activity System | ActivityName | Low |
| **LOW** | Weapon System | Attachment configs | High |
| **LOW** | Camera System | Dialogue sequences | High |

### Files to Modify

| File | Purpose |
|------|---------|
| `GasAbilityGeneratorTypes.h` | Add new struct members |
| `GasAbilityGeneratorParser.cpp` | Parse new YAML fields |
| `GasAbilityGeneratorGenerators.cpp` | Set properties via reflection |
| `manifest.yaml` | Add examples |

---

## 2. QUEST ENHANCEMENTS

### 2.1 Quest Rewards

**Header Reference:** `NarrativeArsenal/Public/Tales/Quest.h` (no explicit FQuestReward - rewards handled via events)

**Gap:** Quests have no manifest support for rewards (currency, XP, items granted on completion).

**Solution:** Fire reward events on success state, or create custom reward struct.

#### Struct Addition (GasAbilityGeneratorTypes.h)

```cpp
// Add after FManifestQuestTaskDefinition

USTRUCT()
struct FManifestQuestRewardDefinition
{
    GENERATED_BODY()

    // Currency to grant on quest completion
    UPROPERTY()
    int32 Currency = 0;

    // XP to grant on quest completion
    UPROPERTY()
    int32 XP = 0;

    // Items to grant on completion (asset names)
    UPROPERTY()
    TArray<FString> Items;

    // Item quantities (parallel array)
    UPROPERTY()
    TArray<int32> ItemQuantities;

    FString ComputeHash() const
    {
        FString Hash = FString::Printf(TEXT("Currency:%d|XP:%d"), Currency, XP);
        for (int32 i = 0; i < Items.Num(); i++)
        {
            int32 Qty = ItemQuantities.IsValidIndex(i) ? ItemQuantities[i] : 1;
            Hash += FString::Printf(TEXT("|Item:%s:%d"), *Items[i], Qty);
        }
        return Hash;
    }
};
```

#### Update FManifestQuestDefinition

```cpp
// In FManifestQuestDefinition struct, add:

// The NPC who gives this quest (NPCDef_ name)
UPROPERTY()
FString QuestGiver;

// Rewards granted on quest success
UPROPERTY()
FManifestQuestRewardDefinition Rewards;

// Update ComputeHash to include:
// + QuestGiver + Rewards.ComputeHash()
```

#### Parser Addition (GasAbilityGeneratorParser.cpp)

```cpp
// In ParseQuestDefinition function, add:

// Parse questgiver
if (QuestNode["questgiver"])
{
    QuestDef.QuestGiver = FString(QuestNode["questgiver"].as<std::string>().c_str());
}

// Parse rewards
if (QuestNode["rewards"])
{
    YAML::Node RewardsNode = QuestNode["rewards"];

    if (RewardsNode["currency"])
    {
        QuestDef.Rewards.Currency = RewardsNode["currency"].as<int32>();
    }
    if (RewardsNode["xp"])
    {
        QuestDef.Rewards.XP = RewardsNode["xp"].as<int32>();
    }
    if (RewardsNode["items"])
    {
        for (const auto& ItemNode : RewardsNode["items"])
        {
            if (ItemNode.IsScalar())
            {
                // Simple item name
                QuestDef.Rewards.Items.Add(FString(ItemNode.as<std::string>().c_str()));
                QuestDef.Rewards.ItemQuantities.Add(1);
            }
            else if (ItemNode.IsMap())
            {
                // Item with quantity
                if (ItemNode["item"])
                {
                    QuestDef.Rewards.Items.Add(FString(ItemNode["item"].as<std::string>().c_str()));
                }
                if (ItemNode["quantity"])
                {
                    QuestDef.Rewards.ItemQuantities.Add(ItemNode["quantity"].as<int32>());
                }
                else
                {
                    QuestDef.Rewards.ItemQuantities.Add(1);
                }
            }
        }
    }
}
```

#### Generator Addition (GasAbilityGeneratorGenerators.cpp)

```cpp
// In FQuestGenerator::Generate, after creating success state:

// Add reward events to success state
if (Definition.Rewards.Currency > 0 || Definition.Rewards.XP > 0 || Definition.Rewards.Items.Num() > 0)
{
    // Find success state
    for (UQuestState* State : QuestTemplate->GetStates())
    {
        if (State && State->StateNodeType == EStateNodeType::Success)
        {
            // Create reward events
            // Option 1: Use NE_GrantCurrency, NE_GrantXP, NE_GrantItem events
            // Option 2: Create custom NE_QuestReward event that handles all

            // For currency
            if (Definition.Rewards.Currency > 0)
            {
                // Create and add NE_GrantCurrency event
                UNarrativeEvent* CurrencyEvent = NewObject<UNarrativeEvent>(State);
                // Set properties via reflection...
                State->Events.Add(CurrencyEvent);
            }

            // For XP
            if (Definition.Rewards.XP > 0)
            {
                // Create and add NE_GrantXP event
                // ...
            }

            // For items
            for (int32 i = 0; i < Definition.Rewards.Items.Num(); i++)
            {
                // Create and add NE_GrantItem event
                // ...
            }

            break;
        }
    }
}
```

#### Manifest Example

```yaml
quests:
  - name: Quest_ForgeSupplies
    folder: Quests/Town
    quest_name: "Forge Supplies"
    description: "Gather iron ore for the blacksmith."
    questgiver: NPCDef_Blacksmith    # NEW - links to NPC
    is_tracked: true
    states:
      - id: Start
        type: regular
        description: "Talk to the blacksmith"
      - id: Complete
        type: success
        description: "Quest completed!"
    branches:
      - id: GatherOre
        from_state: Start
        to_state: Complete
        tasks:
          - task_class: BPT_FindItem
            quantity: 10
            properties:
              item_class: IronOre
    rewards:                          # NEW - automatic reward events
      currency: 100
      xp: 50
      items:
        - item: EI_IronSword
          quantity: 1
        - EI_HealthPotion            # Shorthand for quantity 1
```

---

## 3. NPC DEFINITION ENHANCEMENTS

### 3.1 TradingItemLoadout (Shop Inventory)

**Header Reference:** `NarrativeArsenal/Public/AI/NPCDefinition.h:80-81`

```cpp
// From NPCDefinition.h
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC Trading")
TArray<FLootTableRoll> TradingItemLoadout;
```

**FLootTableRoll Reference:** `NarrativeArsenal/Public/Items/InventoryComponent.h`

```cpp
USTRUCT(BlueprintType)
struct NARRATIVEARSENAL_API FLootTableRoll
{
    GENERATED_BODY()

    FLootTableRoll()
    {
        Quantity = 1;
        RollChance = 1.f;
    };

    // The item to spawn
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
    TSubclassOf<class UNarrativeItem> Item;

    // The amount of items to spawn
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
    int32 Quantity;

    // The chance this item will roll (0-1)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
    float RollChance;
};
```

#### Struct Addition (GasAbilityGeneratorTypes.h)

```cpp
// Add new struct for loot table entries

USTRUCT()
struct FManifestLootTableRollDefinition
{
    GENERATED_BODY()

    // Item class name (e.g., "EI_HealthPotion")
    UPROPERTY()
    FString Item;

    // Quantity to spawn
    UPROPERTY()
    int32 Quantity = 1;

    // Roll chance (0.0 - 1.0)
    UPROPERTY()
    float RollChance = 1.0f;

    FString ComputeHash() const
    {
        return FString::Printf(TEXT("%s:%d:%.2f"), *Item, Quantity, RollChance);
    }
};

// In FManifestNPCDefinitionDefinition, add:

// Trading inventory for vendor NPCs
UPROPERTY()
TArray<FManifestLootTableRollDefinition> TradingItemLoadout;

// Default item loadout (items granted on spawn)
UPROPERTY()
TArray<FManifestLootTableRollDefinition> DefaultItemLoadout;
```

#### Parser Addition (GasAbilityGeneratorParser.cpp)

```cpp
// Helper function to parse loot table rolls
TArray<FManifestLootTableRollDefinition> ParseLootTableRolls(const YAML::Node& Node)
{
    TArray<FManifestLootTableRollDefinition> Rolls;

    if (!Node || !Node.IsSequence()) return Rolls;

    for (const auto& RollNode : Node)
    {
        FManifestLootTableRollDefinition Roll;

        if (RollNode.IsScalar())
        {
            // Simple item name
            Roll.Item = FString(RollNode.as<std::string>().c_str());
            Roll.Quantity = 1;
            Roll.RollChance = 1.0f;
        }
        else if (RollNode.IsMap())
        {
            if (RollNode["item"])
            {
                Roll.Item = FString(RollNode["item"].as<std::string>().c_str());
            }
            if (RollNode["quantity"])
            {
                Roll.Quantity = RollNode["quantity"].as<int32>();
            }
            if (RollNode["chance"] || RollNode["roll_chance"])
            {
                YAML::Node ChanceNode = RollNode["chance"] ? RollNode["chance"] : RollNode["roll_chance"];
                Roll.RollChance = ChanceNode.as<float>();
            }
        }

        if (!Roll.Item.IsEmpty())
        {
            Rolls.Add(Roll);
        }
    }

    return Rolls;
}

// In ParseNPCDefinition, add:

if (NPCNode["trading_item_loadout"] || NPCNode["trading_inventory"])
{
    YAML::Node LoadoutNode = NPCNode["trading_item_loadout"] ?
        NPCNode["trading_item_loadout"] : NPCNode["trading_inventory"];
    NPCDef.TradingItemLoadout = ParseLootTableRolls(LoadoutNode);
}

if (NPCNode["default_item_loadout"])
{
    NPCDef.DefaultItemLoadout = ParseLootTableRolls(NPCNode["default_item_loadout"]);
}
```

#### Generator Addition (GasAbilityGeneratorGenerators.cpp)

```cpp
// In FNPCDefinitionGenerator::Generate, add after existing property setting:

// Set TradingItemLoadout
if (Definition.TradingItemLoadout.Num() > 0)
{
    FArrayProperty* TradingLoadoutProp = CastField<FArrayProperty>(
        NPCDefClass->FindPropertyByName(TEXT("TradingItemLoadout")));

    if (TradingLoadoutProp)
    {
        FScriptArrayHelper ArrayHelper(TradingLoadoutProp,
            TradingLoadoutProp->ContainerPtrToValuePtr<void>(NPCDefinition));

        ArrayHelper.EmptyValues();

        for (const FManifestLootTableRollDefinition& RollDef : Definition.TradingItemLoadout)
        {
            // Find item class
            UClass* ItemClass = FindItemClass(RollDef.Item);
            if (!ItemClass) continue;

            // Add to array
            int32 NewIndex = ArrayHelper.AddValue();
            void* RollPtr = ArrayHelper.GetRawPtr(NewIndex);

            // Set Item property (TSubclassOf<UNarrativeItem>)
            FClassProperty* ItemProp = CastField<FClassProperty>(
                TradingLoadoutProp->Inner->FindPropertyByName(TEXT("Item")));
            if (ItemProp)
            {
                ItemProp->SetPropertyValue_InContainer(RollPtr, ItemClass);
            }

            // Set Quantity
            FIntProperty* QtyProp = CastField<FIntProperty>(
                TradingLoadoutProp->Inner->FindPropertyByName(TEXT("Quantity")));
            if (QtyProp)
            {
                QtyProp->SetPropertyValue_InContainer(RollPtr, RollDef.Quantity);
            }

            // Set RollChance
            FFloatProperty* ChanceProp = CastField<FFloatProperty>(
                TradingLoadoutProp->Inner->FindPropertyByName(TEXT("RollChance")));
            if (ChanceProp)
            {
                ChanceProp->SetPropertyValue_InContainer(RollPtr, RollDef.RollChance);
            }
        }
    }
}

// Helper function to find item class
UClass* FindItemClass(const FString& ItemName)
{
    // Search paths for item classes
    TArray<FString> SearchPaths = {
        FString::Printf(TEXT("/Game/%s/Items/%s.%s_C"), *ProjectName, *ItemName, *ItemName),
        FString::Printf(TEXT("/Game/%s/Items/Equipment/%s.%s_C"), *ProjectName, *ItemName, *ItemName),
        FString::Printf(TEXT("/Game/%s/Items/Weapons/%s.%s_C"), *ProjectName, *ItemName, *ItemName),
        FString::Printf(TEXT("/Game/%s/Items/Consumables/%s.%s_C"), *ProjectName, *ItemName, *ItemName),
    };

    for (const FString& Path : SearchPaths)
    {
        UClass* FoundClass = LoadClass<UNarrativeItem>(nullptr, *Path);
        if (FoundClass)
        {
            return FoundClass;
        }
    }

    return nullptr;
}
```

#### Manifest Example

```yaml
npc_definitions:
  - name: NPCDef_Merchant
    folder: NPCs/Definitions
    npc_id: Merchant_01
    npc_name: "Traveling Merchant"
    is_vendor: true
    shop_name: "Exotic Goods"
    buy_item_percentage: 0.5
    sell_item_percentage: 1.5
    trading_currency: 1000

    # NEW - Shop inventory
    trading_item_loadout:
      - item: EI_HealthPotion
        quantity: 10
        chance: 1.0
      - item: EI_ManaPotion
        quantity: 5
        chance: 1.0
      - item: EI_RareAmulet
        quantity: 1
        chance: 0.3          # 30% chance to have in stock
      - EI_Torch              # Shorthand: quantity 1, chance 1.0

    # NEW - Items NPC spawns with
    default_item_loadout:
      - item: EI_MerchantRobes
        quantity: 1
      - item: EI_GoldPouch
        quantity: 1
```

---

## 4. DIALOGUE ENHANCEMENTS

### 4.1 Dialogue-Quest Shortcuts

**Gap:** Starting a quest from dialogue requires manually creating NarrativeEvent with properties. Need shorthand.

#### Struct Addition (GasAbilityGeneratorTypes.h)

```cpp
// In FManifestDialogueNodeDefinition, add:

// Shortcut: Quest to start when this node is selected
UPROPERTY()
FString StartQuest;

// Shortcut: Quest branch to complete when this node is selected
UPROPERTY()
FString CompleteQuestBranch;

// Shortcut: Quest to fail when this node is selected
UPROPERTY()
FString FailQuest;
```

#### Parser Addition (GasAbilityGeneratorParser.cpp)

```cpp
// In ParseDialogueNode, add:

if (NodeYaml["start_quest"])
{
    Node.StartQuest = FString(NodeYaml["start_quest"].as<std::string>().c_str());
}
if (NodeYaml["complete_quest_branch"])
{
    Node.CompleteQuestBranch = FString(NodeYaml["complete_quest_branch"].as<std::string>().c_str());
}
if (NodeYaml["fail_quest"])
{
    Node.FailQuest = FString(NodeYaml["fail_quest"].as<std::string>().c_str());
}
```

#### Generator Addition (GasAbilityGeneratorGenerators.cpp)

```cpp
// In CreateDialogueNode, after creating base node:

// Handle quest shortcuts
if (!NodeDef.StartQuest.IsEmpty())
{
    // Create NE_BeginQuest event
    FManifestDialogueEventDefinition QuestEvent;
    QuestEvent.Type = TEXT("NE_BeginQuest");
    QuestEvent.Runtime = TEXT("Start");
    QuestEvent.Properties.Add(TEXT("QuestClass"), NodeDef.StartQuest);

    // Add to node's events
    NodeDef.Events.Add(QuestEvent);
}

if (!NodeDef.CompleteQuestBranch.IsEmpty())
{
    // Create NE_CompleteQuestBranch event
    FManifestDialogueEventDefinition BranchEvent;
    BranchEvent.Type = TEXT("NE_CompleteQuestBranch");
    BranchEvent.Runtime = TEXT("Start");
    BranchEvent.Properties.Add(TEXT("BranchID"), NodeDef.CompleteQuestBranch);

    NodeDef.Events.Add(BranchEvent);
}

if (!NodeDef.FailQuest.IsEmpty())
{
    // Create NE_FailQuest event
    FManifestDialogueEventDefinition FailEvent;
    FailEvent.Type = TEXT("NE_FailQuest");
    FailEvent.Runtime = TEXT("Start");
    FailEvent.Properties.Add(TEXT("QuestClass"), NodeDef.FailQuest);

    NodeDef.Events.Add(FailEvent);
}
```

#### Manifest Example

```yaml
dialogue_blueprints:
  - name: DBP_BlacksmithQuest
    folder: Dialogues
    dialogue_tree:
      root: greeting
      nodes:
        - id: greeting
          type: npc
          speaker: NPCDef_Blacksmith
          text: "I need help gathering supplies."
          player_replies: [accept, decline]

        - id: accept
          type: player
          text: "I'll help you."
          option_text: "Accept quest"
          start_quest: Quest_ForgeSupplies    # NEW - shortcut!
          npc_replies: [thanks]

        - id: decline
          type: player
          text: "Not interested."
          option_text: "Decline"
          npc_replies: [disappointed]

        - id: return_success
          type: player
          text: "I have your ore."
          complete_quest_branch: GatherOre    # NEW - completes branch
          npc_replies: [reward]
```

### 4.2 Speaker Tag Population

**Header Reference:** `NarrativeArsenal/Public/Tales/Dialogue.h:55-56`

```cpp
// FSpeakerInfo
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speaker Details")
FGameplayTagContainer OwnedTags;
```

**Gap:** Speaker OwnedTags not populated from manifest.

#### Struct Addition (GasAbilityGeneratorTypes.h)

```cpp
// In FManifestDialogueSpeakerDefinition, add:

// Tags granted to speaker during dialogue
UPROPERTY()
TArray<FString> OwnedTags;
```

#### Parser Addition

```cpp
// In speaker parsing:
if (SpeakerNode["owned_tags"])
{
    for (const auto& TagNode : SpeakerNode["owned_tags"])
    {
        Speaker.OwnedTags.Add(FString(TagNode.as<std::string>().c_str()));
    }
}
```

#### Generator Addition

```cpp
// When creating FSpeakerInfo:
for (const FString& TagStr : SpeakerDef.OwnedTags)
{
    FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagStr), false);
    if (Tag.IsValid())
    {
        SpeakerInfo.OwnedTags.AddTag(Tag);
    }
}
```

### 4.3 Camera System (LOW PRIORITY)

**Header Reference:** `NarrativeArsenal/Public/Tales/Dialogue.h:264-274`

```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
TSubclassOf<class UCameraShakeBase> DialogueCameraShake;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
class USoundAttenuation* DialogueSoundAttenuation;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Camera")
class UNarrativeDialogueSequence* DefaultDialogueShot;
```

**Gap:** Camera shake, sound attenuation, and dialogue sequences not in manifest.

**Recommendation:** These are advanced features better suited for editor-based setup. Mark as LOW priority.

---

## 5. ITEM ENHANCEMENTS

### 5.1 NarrativeItem Usage Properties

**Header Reference:** `NarrativeArsenal/Public/Items/NarrativeItem.h:223-247`

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item | Usage")
bool bAddDefaultUseOption;

UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item | Usage")
bool bConsumeOnUse;

UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item | Usage")
bool bUsedWithOtherItem;

UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item | Usage")
FText UseActionText;

UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item | Activation")
bool bCanActivate;

UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item | Activation")
bool bToggleActiveOnUse;
```

#### Struct Addition (GasAbilityGeneratorTypes.h)

```cpp
// In FManifestEquippableItemDefinition (base for all items), add:

// Usage properties
UPROPERTY()
bool bAddDefaultUseOption = true;

UPROPERTY()
bool bConsumeOnUse = false;

UPROPERTY()
bool bUsedWithOtherItem = false;

UPROPERTY()
FString UseActionText;

// Activation properties
UPROPERTY()
bool bCanActivate = false;

UPROPERTY()
bool bToggleActiveOnUse = false;

// Sound
UPROPERTY()
FString UseSound;  // Asset path to USoundBase
```

#### Parser Addition

```cpp
// In ParseEquippableItem:

if (ItemNode["add_default_use_option"])
{
    ItemDef.bAddDefaultUseOption = ItemNode["add_default_use_option"].as<bool>();
}
if (ItemNode["consume_on_use"])
{
    ItemDef.bConsumeOnUse = ItemNode["consume_on_use"].as<bool>();
}
if (ItemNode["used_with_other_item"])
{
    ItemDef.bUsedWithOtherItem = ItemNode["used_with_other_item"].as<bool>();
}
if (ItemNode["use_action_text"])
{
    ItemDef.UseActionText = FString(ItemNode["use_action_text"].as<std::string>().c_str());
}
if (ItemNode["can_activate"])
{
    ItemDef.bCanActivate = ItemNode["can_activate"].as<bool>();
}
if (ItemNode["toggle_active_on_use"])
{
    ItemDef.bToggleActiveOnUse = ItemNode["toggle_active_on_use"].as<bool>();
}
if (ItemNode["use_sound"])
{
    ItemDef.UseSound = FString(ItemNode["use_sound"].as<std::string>().c_str());
}
```

#### Generator Addition

```cpp
// In FEquippableItemGenerator::Generate:

// Set usage properties via reflection
SetBoolProperty(ItemBP, TEXT("bAddDefaultUseOption"), Definition.bAddDefaultUseOption);
SetBoolProperty(ItemBP, TEXT("bConsumeOnUse"), Definition.bConsumeOnUse);
SetBoolProperty(ItemBP, TEXT("bUsedWithOtherItem"), Definition.bUsedWithOtherItem);

if (!Definition.UseActionText.IsEmpty())
{
    SetTextProperty(ItemBP, TEXT("UseActionText"), FText::FromString(Definition.UseActionText));
}

SetBoolProperty(ItemBP, TEXT("bCanActivate"), Definition.bCanActivate);
SetBoolProperty(ItemBP, TEXT("bToggleActiveOnUse"), Definition.bToggleActiveOnUse);

// Set UseSound if specified
if (!Definition.UseSound.IsEmpty())
{
    USoundBase* Sound = LoadObject<USoundBase>(nullptr, *Definition.UseSound);
    if (Sound)
    {
        SetObjectProperty(ItemBP, TEXT("UseSound"), Sound);
    }
}
```

#### Manifest Example

```yaml
equippable_items:
  - name: EI_HealthPotion
    folder: Items/Consumables
    parent_class: NarrativeItem
    display_name: "Health Potion"
    description: "Restores 50 health points."
    stackable: true
    max_stack_size: 10
    base_value: 25
    weight: 0.5

    # NEW - Usage properties
    add_default_use_option: true
    consume_on_use: true
    use_action_text: "Drink"
    use_sound: "/Game/Audio/SFX/S_DrinkPotion"

    # NEW - Activation (for toggleable items)
    can_activate: false
    toggle_active_on_use: false
```

### 5.2 Item Fragments (LOW PRIORITY)

**Header Reference:** `NarrativeArsenal/Public/Items/NarrativeItem.h:73-86`

```cpp
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class NARRATIVEARSENAL_API UNarrativeItemFragment : public UObject
```

**Gap:** Fragments provide composition-based item data (ammo, poison, etc). Complex to automate.

**Recommendation:** LOW priority - fragments are advanced and better suited for BP implementation.

---

## 6. ACTIVITY ENHANCEMENTS

### 6.1 ActivityName Property

**Header Reference:** `NarrativeArsenal/Public/AI/Activities/NarrativeActivityBase.h:117`

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activity")
FText ActivityName;
```

**Gap:** ActivityName exists in manifest struct but not being set by generator.

#### Check Current Struct (GasAbilityGeneratorTypes.h)

```cpp
// FManifestActivityDefinition should already have:
UPROPERTY()
FString ActivityName;

// If not, add it
```

#### Parser Check

```cpp
// Ensure parsing exists:
if (ActivityNode["activity_name"])
{
    ActivityDef.ActivityName = FString(ActivityNode["activity_name"].as<std::string>().c_str());
}
```

#### Generator Addition (MISSING - needs to be added)

```cpp
// In FActivityGenerator::Generate, add:

// Set ActivityName on UNarrativeActivityBase CDO
if (!Definition.ActivityName.IsEmpty())
{
    UNarrativeActivityBase* ActivityCDO = Cast<UNarrativeActivityBase>(
        ActivityBP->GeneratedClass->GetDefaultObject());

    if (ActivityCDO)
    {
        // Use reflection to set FText property
        FTextProperty* NameProp = CastField<FTextProperty>(
            UNarrativeActivityBase::StaticClass()->FindPropertyByName(TEXT("ActivityName")));

        if (NameProp)
        {
            FText NameText = FText::FromString(Definition.ActivityName);
            NameProp->SetPropertyValue_InContainer(ActivityCDO, NameText);
        }
    }
}
```

#### Manifest Example

```yaml
activities:
  - name: BPA_PatrolRoute
    folder: AI/Activities
    parent_class: NarrativeActivityBase
    behavior_tree: BT_Patrol
    activity_name: "Patrol Route"        # NOW ACTUALLY SET
    description: "NPC patrols route"
    owned_tags: [State.Patrolling]
    block_tags: [State.InCombat]
```

---

## 7. GOAL ENHANCEMENTS

### 7.1 Goal Progress Events

**Header Reference:** `NarrativeArsenal/Public/AI/Activities/NPCGoalItem.h:81-87`

```cpp
UPROPERTY(BlueprintAssignable, Category = "NPC Activity")
FOnGoalSignature OnGoalSucceeded;

UPROPERTY(BlueprintAssignable, Category = "NPC Activity")
FOnGoalSignature OnGoalRemoved;
```

**Gap:** Goal delegates are runtime events, not data. Cannot automate.

**Note:** Goals are already well automated (v3.9). The only missing piece is goal execution logic which MUST be Blueprint-implemented.

### 7.2 IntendedTODStartTime

**Header Reference:** `NarrativeArsenal/Public/AI/Activities/NPCGoalItem.h:65-66`

```cpp
UPROPERTY(BlueprintReadWrite, SaveGame, Category = "NPC Goal", meta = (ExposeOnSpawn=true))
float IntendedTODStartTime;
```

**Gap:** This is set by schedules at runtime, not data.

**Recommendation:** No change needed - this is correctly handled by schedule system.

---

## 8. WEAPON ENHANCEMENTS

### 8.1 Weapon Attachment Configs

**Header Reference:** `NarrativeArsenal/Public/Items/WeaponItem.h:159-164`

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item - Weapon | Weapon Visuals")
TMap<FGameplayTag, FWeaponAttachmentConfig> HolsterAttachmentConfigs;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item - Weapon | Weapon Visuals")
TMap<FGameplayTag, FWeaponAttachmentConfig> WieldAttachmentConfigs;
```

**FWeaponAttachmentConfig:**
```cpp
USTRUCT(BlueprintType)
struct FWeaponAttachmentConfig
{
    GENERATED_BODY()

    FName SocketName;
    FTransform Offset;
};
```

**Gap:** Attachment configs define where weapons attach in different slots/hands.

#### Struct Addition (GasAbilityGeneratorTypes.h)

```cpp
USTRUCT()
struct FManifestWeaponAttachmentConfigDefinition
{
    GENERATED_BODY()

    // Gameplay tag for the slot (e.g., "Narrative.Equipment.Slot.Weapon.HipL")
    UPROPERTY()
    FString SlotTag;

    // Socket/bone name to attach to
    UPROPERTY()
    FString SocketName;

    // Optional offset transform
    UPROPERTY()
    FVector LocationOffset = FVector::ZeroVector;

    UPROPERTY()
    FRotator RotationOffset = FRotator::ZeroRotator;

    UPROPERTY()
    FVector ScaleOffset = FVector::OneVector;
};

// In FManifestEquippableItemDefinition, add:

UPROPERTY()
TArray<FManifestWeaponAttachmentConfigDefinition> HolsterAttachments;

UPROPERTY()
TArray<FManifestWeaponAttachmentConfigDefinition> WieldAttachments;
```

#### Parser Addition

```cpp
// Helper to parse attachment configs
TArray<FManifestWeaponAttachmentConfigDefinition> ParseAttachmentConfigs(const YAML::Node& Node)
{
    TArray<FManifestWeaponAttachmentConfigDefinition> Configs;

    if (!Node || !Node.IsSequence()) return Configs;

    for (const auto& ConfigNode : Node)
    {
        FManifestWeaponAttachmentConfigDefinition Config;

        if (ConfigNode["slot"]) Config.SlotTag = FString(ConfigNode["slot"].as<std::string>().c_str());
        if (ConfigNode["socket"]) Config.SocketName = FString(ConfigNode["socket"].as<std::string>().c_str());

        if (ConfigNode["offset"])
        {
            YAML::Node OffsetNode = ConfigNode["offset"];
            if (OffsetNode["location"])
            {
                Config.LocationOffset.X = OffsetNode["location"]["x"].as<float>(0.f);
                Config.LocationOffset.Y = OffsetNode["location"]["y"].as<float>(0.f);
                Config.LocationOffset.Z = OffsetNode["location"]["z"].as<float>(0.f);
            }
            if (OffsetNode["rotation"])
            {
                Config.RotationOffset.Pitch = OffsetNode["rotation"]["pitch"].as<float>(0.f);
                Config.RotationOffset.Yaw = OffsetNode["rotation"]["yaw"].as<float>(0.f);
                Config.RotationOffset.Roll = OffsetNode["rotation"]["roll"].as<float>(0.f);
            }
        }

        Configs.Add(Config);
    }

    return Configs;
}

// In weapon item parsing:
if (ItemNode["holster_attachments"])
{
    ItemDef.HolsterAttachments = ParseAttachmentConfigs(ItemNode["holster_attachments"]);
}
if (ItemNode["wield_attachments"])
{
    ItemDef.WieldAttachments = ParseAttachmentConfigs(ItemNode["wield_attachments"]);
}
```

#### Manifest Example

```yaml
equippable_items:
  - name: EI_Sword
    folder: Items/Weapons
    parent_class: MeleeWeaponItem
    display_name: "Steel Sword"
    equipment_slot: Narrative.Equipment.Slot.Weapon.HipL
    weapon_hand: MainHand
    attack_damage: 25.0

    # NEW - Where to attach when holstered
    holster_attachments:
      - slot: Narrative.Equipment.Slot.Weapon.HipL
        socket: spine_03
        offset:
          location: { x: -15, y: 5, z: 0 }
          rotation: { pitch: 0, yaw: 90, roll: 0 }
      - slot: Narrative.Equipment.Slot.Weapon.Back
        socket: spine_02
        offset:
          location: { x: 0, y: -10, z: 5 }
          rotation: { pitch: 45, yaw: 0, roll: 0 }

    # NEW - Where to attach when wielded
    wield_attachments:
      - slot: Narrative.Equipment.WieldSlot.Mainhand
        socket: hand_r
```

---

## 9. CROSS-ASSET RELATIONSHIP SYSTEM

### 9.1 Quest-NPC Bi-Directional Linking

**Problem:** Quests know their questgiver, but NPCs don't know what quests they give.

**Solution:** During generation, build relationship map and populate both sides.

#### Implementation

```cpp
// In GasAbilityGeneratorCommandlet or Window, after all assets generated:

void BuildCrossAssetRelationships(const FManifestData& Manifest)
{
    // Build Quest -> NPC map
    TMap<FString, TArray<FString>> NPCToQuests;  // NPC name -> quest names

    for (const FManifestQuestDefinition& Quest : Manifest.Quests)
    {
        if (!Quest.QuestGiver.IsEmpty())
        {
            NPCToQuests.FindOrAdd(Quest.QuestGiver).Add(Quest.Name);
        }
    }

    // Now update each NPC's generated asset
    for (const auto& Pair : NPCToQuests)
    {
        FString NPCName = Pair.Key;
        const TArray<FString>& QuestNames = Pair.Value;

        // Find the NPCDefinition asset
        UNPCDefinition* NPCDef = FindNPCDefinition(NPCName);
        if (!NPCDef) continue;

        // NPCDefinition doesn't have a QuestsGiven array in Narrative Pro...
        // But we can add this to the Dialogue instead!

        // Or: Generate a data asset that tracks these relationships
        // Or: Add as metadata for the NPC content browser preview
    }
}
```

**Alternative:** Store in metadata registry for UI/editor consumption.

### 9.2 Dependency-Ordered Generation

**Problem:** NPCDef references AC_, which must exist first.

**Current Solution:** Deferred retry mechanism.

**Enhancement:** Topological sort of generation order.

```cpp
// Generation order (already implemented in commandlet):
1. Tags
2. Enumerations
3. Blackboards
4. GameplayEffects
5. GameplayAbilities
6. Goals
7. Activities
8. ActivityConfigurations
9. AbilityConfigurations
10. Schedules
11. Dialogues
12. TaggedDialogueSets
13. Quests
14. NPCDefinitions  // Last, can reference all above
15. ActorBlueprints
```

---

## 10. IMPLEMENTATION ORDER

### Phase 1: HIGH Priority (Core NPC Pipeline)

| Order | Feature | Complexity | Files Changed |
|-------|---------|------------|---------------|
| 1 | Quest Rewards | Medium | Types, Parser, QuestGen |
| 2 | Quest Questgiver | Low | Types, Parser |
| 3 | TradingItemLoadout | Medium | Types, Parser, NPCDefGen |
| 4 | Dialogue Quest Shortcuts | Low | Types, Parser, DialogueGen |

**Estimated LOC:** ~400 total

### Phase 2: MEDIUM Priority (Quality of Life)

| Order | Feature | Complexity | Files Changed |
|-------|---------|------------|---------------|
| 5 | Activity ActivityName | Low | ActivityGen only |
| 6 | Item Usage Props | Low | Types, Parser, ItemGen |
| 7 | Speaker OwnedTags | Low | Types, Parser, DialogueGen |
| 8 | DefaultItemLoadout | Medium | Same as TradingItemLoadout |

**Estimated LOC:** ~200 total

### Phase 3: LOW Priority (Advanced)

| Order | Feature | Complexity | Files Changed |
|-------|---------|------------|---------------|
| 9 | Weapon Attachments | High | Types, Parser, WeaponGen |
| 10 | Equipment Effect Values | Medium | Types, Parser, ItemGen |
| 11 | Camera/Sequence | High | Types, Parser, DialogueGen |
| 12 | Item Fragments | Very High | New generator |

**Estimated LOC:** ~600 total

---

## 11. TESTING CHECKLIST

### Quest Rewards
- [ ] Parse rewards with currency only
- [ ] Parse rewards with XP only
- [ ] Parse rewards with items (simple)
- [ ] Parse rewards with items (quantity)
- [ ] Parse rewards with all three
- [ ] Verify reward events created on success state
- [ ] Runtime test: complete quest, verify rewards granted

### TradingItemLoadout
- [ ] Parse simple item list
- [ ] Parse items with quantities
- [ ] Parse items with roll chances
- [ ] Verify FLootTableRoll array populated
- [ ] Runtime test: vendor shop shows items

### Dialogue Quest Shortcuts
- [ ] Parse start_quest property
- [ ] Parse complete_quest_branch property
- [ ] Verify NE_BeginQuest event created
- [ ] Runtime test: selecting node starts quest

### Activity ActivityName
- [ ] Parse activity_name
- [ ] Verify ActivityName property set on CDO
- [ ] Runtime test: debug display shows name

### Item Usage Props
- [ ] Parse all usage booleans
- [ ] Parse use_action_text
- [ ] Parse use_sound
- [ ] Verify properties set on item CDO
- [ ] Runtime test: item use behavior correct

---

## MANIFEST EXAMPLE: COMPLETE NPC

```yaml
# Single manifest defining complete NPC with all relationships

npc_definitions:
  - name: NPCDef_Blacksmith
    folder: NPCs/Town
    npc_id: Blacksmith_01
    npc_name: "Garrett the Blacksmith"
    npc_blueprint: BP_Blacksmith

    # Configurations
    ability_configuration: AC_Blacksmith
    activity_configuration: ActConfig_Blacksmith

    # Tags
    default_owned_tags: [State.Invulnerable]
    default_factions: [Narrative.Factions.Friendly, Narrative.Factions.Town]

    # Schedules
    activity_schedules: [Schedule_BlacksmithDay]

    # Dialogue
    auto_create_dialogue: true
    auto_create_tagged_dialogue: true

    # Vendor
    is_vendor: true
    shop_name: "Garrett's Forge"
    trading_currency: 500
    buy_item_percentage: 0.5
    sell_item_percentage: 1.5
    trading_item_loadout:
      - item: EI_IronSword
        quantity: 3
      - item: EI_SteelArmor
        quantity: 2
      - item: EI_RepairKit
        quantity: 10

dialogue_blueprints:
  - name: DBP_BlacksmithDialogue
    folder: NPCs/Town/Dialogues
    free_movement: true
    dialogue_tree:
      root: greeting
      nodes:
        - id: greeting
          type: npc
          speaker: NPCDef_Blacksmith
          text: "Welcome to my forge! Need weapons or armor?"
          player_replies: [shop, quest, leave]

        - id: shop
          type: player
          text: "Show me your wares."
          option_text: "Trade"
          # Opens vendor UI (built-in Narrative Pro behavior)

        - id: quest
          type: player
          text: "Need any help around here?"
          option_text: "Any work?"
          npc_replies: [quest_offer]

        - id: quest_offer
          type: npc
          speaker: NPCDef_Blacksmith
          text: "Actually, I'm running low on iron ore..."
          player_replies: [accept_quest, decline_quest]

        - id: accept_quest
          type: player
          text: "I'll gather some for you."
          option_text: "Accept"
          start_quest: Quest_ForgeSupplies
          npc_replies: [thanks]

quests:
  - name: Quest_ForgeSupplies
    folder: Quests/Town
    quest_name: "Forge Supplies"
    description: "Gather iron ore for Garrett the blacksmith."
    questgiver: NPCDef_Blacksmith
    is_tracked: true
    dialogue: DBP_BlacksmithDialogue
    states:
      - id: Start
        type: regular
        description: "Gather 10 iron ore"
      - id: Return
        type: regular
        description: "Return to Garrett"
      - id: Complete
        type: success
        description: "Quest completed!"
    branches:
      - id: GatherOre
        from_state: Start
        to_state: Return
        tasks:
          - task_class: BPT_FindItem
            quantity: 10
            properties:
              item_class: IronOre
      - id: ReturnToBlacksmith
        from_state: Return
        to_state: Complete
        tasks:
          - task_class: BPT_FinishDialogue
            properties:
              dialogue: DBP_BlacksmithReturn
    rewards:
      currency: 100
      xp: 50
      items:
        - item: EI_IronSword
          quantity: 1

activity_schedules:
  - name: Schedule_BlacksmithDay
    folder: AI/Schedules
    behaviors:
      - start_time: 600
        end_time: 1200
        goal_class: Goal_Work
        location: Forge
      - start_time: 1200
        end_time: 1300
        goal_class: Goal_Eat
        location: Tavern
      - start_time: 1300
        end_time: 1800
        goal_class: Goal_Work
        location: Forge
      - start_time: 2200
        end_time: 600
        goal_class: Goal_Sleep
        location: BlacksmithHome

goal_items:
  - name: Goal_Work
    folder: AI/Goals
    default_score: 75.0
    owned_tags: [State.Working]
```

---

## APPENDIX: HEADER FILE LOCATIONS

| Class | Header Path |
|-------|-------------|
| UNPCDefinition | `NarrativeArsenal/Public/AI/NPCDefinition.h` |
| UQuest | `NarrativeArsenal/Public/Tales/Quest.h` |
| UQuestState | `NarrativeArsenal/Public/Tales/QuestSM.h` |
| UQuestBranch | `NarrativeArsenal/Public/Tales/QuestSM.h` |
| UDialogue | `NarrativeArsenal/Public/Tales/Dialogue.h` |
| FSpeakerInfo | `NarrativeArsenal/Public/Tales/Dialogue.h` |
| UNPCGoalItem | `NarrativeArsenal/Public/AI/Activities/NPCGoalItem.h` |
| UNPCActivity | `NarrativeArsenal/Public/AI/Activities/NPCActivity.h` |
| UNarrativeActivityBase | `NarrativeArsenal/Public/AI/Activities/NarrativeActivityBase.h` |
| UNPCActivitySchedule | `NarrativeArsenal/Public/AI/Activities/NPCActivitySchedule.h` |
| UNarrativeItem | `NarrativeArsenal/Public/Items/NarrativeItem.h` |
| UEquippableItem | `NarrativeArsenal/Public/Items/EquippableItem.h` |
| UWeaponItem | `NarrativeArsenal/Public/Items/WeaponItem.h` |
| FLootTableRoll | `NarrativeArsenal/Public/Items/InventoryComponent.h` |

---

**Document Version:** 1.0
**Created:** January 2026
**Author:** Claude Code Analysis
**Status:** Ready for Implementation
