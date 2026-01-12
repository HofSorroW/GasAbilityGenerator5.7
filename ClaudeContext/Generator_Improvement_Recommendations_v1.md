# GasAbilityGenerator Improvement Recommendations

**Date:** January 2026
**Current Version:** v3.0
**Purpose:** Analysis of generator automation gaps and improvement paths

---

## Executive Summary

After comprehensive assessment of all 25 asset generators and online/local research, this document identifies specific improvements to increase automation levels for generators currently at MINIMAL or MEDIUM automation.

---

## Current Automation Levels

| Level | Generators | Status |
|-------|------------|--------|
| **FULL** | Blackboard, Enumeration, InputAction, InputMappingContext, FloatCurve, ItemCollection, Tag | No changes needed |
| **HIGH** | GameplayAbility, GameplayEffect, WidgetBlueprint, ActorBlueprint, EquippableItem, NPCDefinition, CharacterDefinition | Minor improvements possible |
| **MEDIUM-HIGH** | Activity, NarrativeEvent, NiagaraSystem | Improvements identified |
| **MEDIUM** | Material, MaterialFunction, AnimationMontage, AnimationNotify, DialogueBlueprint | Significant improvements possible |
| **MINIMAL** | BehaviorTree, AbilityConfiguration, ActivityConfiguration | Major improvements identified |

---

## Priority 1: MINIMAL Automation Generators

### 1. BehaviorTree Generator (BT_)

**Current State:** Creates empty UBehaviorTree container with BlackboardAsset reference only.

**Gap Analysis:**
- `RootNode` (UBTCompositeNode) is not populated
- `Children` array in composite nodes is empty
- Services, Decorators not attached
- No task nodes created

**Research Findings:**

From `BTCompositeNode.h`:
```cpp
// Key structures for programmatic population
struct FBTCompositeChild {
    TObjectPtr<UBTCompositeNode> ChildComposite = nullptr;
    TObjectPtr<UBTTaskNode> ChildTask = nullptr;
    TArray<TObjectPtr<UBTDecorator>> Decorators;
    TArray<FBTDecoratorLogic> DecoratorOps;
};

class UBTCompositeNode : public UBTNode {
    TArray<FBTCompositeChild> Children;  // Child nodes
    TArray<TObjectPtr<UBTService>> Services;  // Service nodes
};
```

**Implementation Path:**

1. Add manifest structure:
```yaml
behavior_trees:
  - name: BT_FatherFollow
    blackboard: BB_Father
    root_composite: Selector  # or Sequence
    nodes:
      - id: Root
        type: Selector
        children:
          - id: AttackSequence
            type: Sequence
            services:
              - class: BTService_BlueprintBase
                subclass: BTS_UpdateTarget
            decorators:
              - class: BTDecorator_Blackboard
                key: TargetEnemy
                operation: IsSet
            children:
              - id: MoveToTarget
                type: BTTask_MoveTo
                blackboard_key: TargetEnemy
              - id: AttackTarget
                type: BTTask_BlueprintBase
                subclass: BTTask_Attack
```

2. Generator implementation:
```cpp
// Create root composite
UBTComposite_Selector* RootSelector = NewObject<UBTComposite_Selector>(BehaviorTree);
BehaviorTree->RootNode = RootSelector;

// Add children
FBTCompositeChild Child;
Child.ChildTask = NewObject<UBTTask_MoveTo>(RootSelector);
// Configure task properties via reflection
RootSelector->Children.Add(Child);

// Add services
UBTService_BlueprintBase* Service = NewObject<UBTService_BlueprintBase>(RootSelector);
RootSelector->Services.Add(Service);
```

**Estimated Impact:** MINIMAL -> HIGH (80% automation increase)

---

### 2. AbilityConfiguration Generator (AC_)

**Current State:** Creates DataAsset with empty TArray<TSubclassOf<UGameplayAbility>> arrays.

**Gap Analysis:**
- `DefaultAbilities` array not populated
- `StartupEffects` array not populated
- `DefaultAttributes` not set

**Research Findings:**

From Narrative Pro headers:
```cpp
// AbilityConfiguration.h
class UAbilityConfiguration : public UDataAsset {
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;
    TArray<TSubclassOf<UGameplayEffect>> StartupEffects;
    TSubclassOf<UAttributeSet> DefaultAttributes;
};
```

TSubclassOf arrays can be populated using asset references:

```cpp
// Load class by path
UClass* AbilityClass = LoadClass<UGameplayAbility>(
    nullptr,
    TEXT("/Game/FatherCompanion/Abilities/GA_FatherAttack.GA_FatherAttack_C")
);
if (AbilityClass) {
    AbilityConfig->DefaultAbilities.Add(AbilityClass);
}
```

**Implementation Path:**

1. Add manifest structure:
```yaml
ability_configurations:
  - name: AC_FatherCrawler
    folder: AbilityConfigs
    default_abilities:
      - GA_FatherAttack
      - GA_FatherLaserShot
      - GA_FatherMark
    startup_effects:
      - GE_FatherCrawlerStats
    default_attributes: AS_FatherBase
```

2. Generator implementation:
```cpp
// After generating GA assets, resolve references
for (const FString& AbilityName : Definition.DefaultAbilities)
{
    FString AssetPath = FString::Printf(TEXT("/Game/%s/Abilities/%s.%s_C"),
        *ProjectName, *AbilityName, *AbilityName);

    UClass* AbilityClass = LoadClass<UGameplayAbility>(nullptr, *AssetPath);
    if (AbilityClass)
    {
        AbilityConfig->DefaultAbilities.Add(AbilityClass);
    }
    else
    {
        // Defer for later resolution
        DeferredAbilityRefs.Add({AbilityConfig, AbilityName});
    }
}
```

**Estimated Impact:** MINIMAL -> FULL (100% automation)

---

### 3. ActivityConfiguration Generator (ActConfig_)

**Current State:** Creates empty DataAsset.

**Gap Analysis:**
- `DefaultActivities` (TArray<TSubclassOf<UNPCActivity>>) not populated
- `GoalGenerators` array not populated
- `RescoreInterval` can be set from manifest

**Implementation Path:** Same pattern as AbilityConfiguration.

**Estimated Impact:** MINIMAL -> FULL (100% automation)

---

## Priority 2: MEDIUM Automation Generators

### 4. AnimationMontage Generator (AM_)

**Current State:** Creates UAnimMontage but cannot link to source AnimSequence.

**Gap Analysis:**
- `SlotAnimTracks[0].AnimTrack.AnimSegments` empty
- No source animation linked
- Slot name not configurable

**Research Findings:**

From `AnimMontage.h`:
```cpp
struct FSlotAnimationTrack {
    FName SlotName;
    FAnimTrack AnimTrack;  // Contains AnimSegments array
};

// From AnimTrack.h
struct FAnimSegment {
    void SetAnimReference(UAnimSequenceBase* InAnim, bool bInitializeTimeRange);
};
```

Example from forum/blogs:
```cpp
UAnimMontage* Montage = NewObject<UAnimMontage>();
Montage->SetSkeleton(SourceAnimSequence->GetSkeleton());

FSlotAnimationTrack& Track = Montage->SlotAnimTracks[0];
Track.SlotName = FName("DefaultSlot");

FAnimSegment NewSegment;
NewSegment.SetAnimReference(SourceAnimSequence, true);
Track.AnimTrack.AnimSegments.Add(NewSegment);

Montage->SetCompositeLength(SourceAnimSequence->GetPlayLength());
```

**Implementation Path:**

1. Add manifest structure:
```yaml
animation_montages:
  - name: AM_FatherAttack
    folder: Animations/Montages
    skeleton: /Game/Characters/Father/SK_Father
    slot_name: DefaultSlot
    source_animation: A_Father_Attack_01
    # OR multiple sections:
    sections:
      - name: WindUp
        animation: A_Father_Attack_WindUp
        start_time: 0.0
      - name: Strike
        animation: A_Father_Attack_Strike
        start_time: 0.5
```

2. Requires source animations to exist (external dependency)

**Estimated Impact:** MEDIUM -> HIGH (depends on source animation availability)

---

### 5. Material/MaterialFunction Generators (M_, MF_)

**Current State:** Creates material with expression nodes but limited connections.

**Gap Analysis:**
- Expression graph connections are manual
- Complex node networks not fully supported
- MaterialFunction outputs not connected

**Improvement Path:**
- Add expression_connections to manifest
- Implement connection logic between expression nodes
- Support MaterialFunctionCall nodes

**Estimated Impact:** MEDIUM -> MEDIUM-HIGH

---

## Priority 3: Future Integration - Narrative Pro Assets

### 6. DialogueBlueprint Generator (DBP_)

**Current Understanding from Headers:**

`UDialogue` class structure:
```cpp
// Key members
TArray<FSpeakerInfo> Speakers;           // NPC speakers
FPlayerSpeakerInfo PlayerSpeakerInfo;    // Player speaker
UDialogueNode_NPC* RootDialogue;         // Entry point
TArray<UDialogueNode_NPC*> NPCReplies;   // All NPC nodes
TArray<UDialogueNode_Player*> PlayerReplies;  // All player nodes
```

`UDialogueNode` structure:
```cpp
FDialogueLine Line;  // Contains Text, DialogueSound, DialogueMontage
TArray<UDialogueNode_NPC*> NPCReplies;      // Child NPC responses
TArray<UDialogueNode_Player*> PlayerReplies; // Child player responses
```

**Potential Implementation:**

```yaml
dialogue_blueprints:
  - name: DBP_FatherGreeting
    folder: Dialogues
    speakers:
      - id: Father
        npc_definition: NPCDef_Father
        node_color: [0.1, 0.3, 0.8]
    player_speaker:
      selecting_reply_shot: null
    dialogue_tree:
      - id: Greeting
        type: NPC
        speaker: Father
        line:
          text: "Hello, I am Father."
          sound: /Game/Audio/Father/VO_Greeting
        replies:
          - id: Response1
            type: Player
            line:
              text: "Nice to meet you."
            next: Farewell
          - id: Response2
            type: Player
            line:
              text: "What are you?"
            next: Explanation
      - id: Farewell
        type: NPC
        speaker: Father
        line:
          text: "Goodbye."
```

**Complexity:** HIGH - Dialogue node graph is complex with branching.

---

### 7. Quest Generator (Q_) - NEW

**Understanding from Headers:**

`UQuest` class structure:
```cpp
UQuestState* QuestStartState;
TArray<UQuestState*> States;
TArray<UQuestBranch*> Branches;
```

`UQuestState` structure:
```cpp
TArray<UQuestBranch*> Branches;
EStateNodeType StateNodeType;  // Regular, Success, Failure
FText Description;
```

`UQuestBranch` structure:
```cpp
TArray<UNarrativeTask*> QuestTasks;  // Tasks to complete
UQuestState* DestinationState;        // Where branch leads
```

**Potential Implementation:**

```yaml
quests:
  - name: Q_RescueFather
    folder: Quests
    quest_name: "Rescue Father"
    quest_description: "Find and rescue the Father companion."
    states:
      - id: Start
        type: Regular
        description: "Find the Father in the cave."
        branches:
          - id: FindFather
            tasks:
              - type: GoToLocation
                location: CaveEntrance
            destination: FoundFather
      - id: FoundFather
        type: Regular
        description: "Escort Father to safety."
        branches:
          - id: EscortFather
            tasks:
              - type: EscortNPC
                npc: NPCDef_Father
                destination: SafeZone
            destination: Success
      - id: Success
        type: Success
        description: "Father has been rescued!"
```

---

### 8. NarrativeEvent Generator (NE_) - Enhancement

**Current State:** Creates Blueprint subclass of UNarrativeEvent.

**Gap Analysis:**
- Event parameters not set
- ExecuteEvent logic not implemented

**From Headers:**
```cpp
class UNarrativeEvent : public UObject {
    bool bRefireOnLoad;
    EEventRuntime EventRuntime;  // Start, End, Both
    EEventFilter EventFilter;    // Anyone, OnlyNPCs, OnlyPlayers
    TArray<UNarrativeCondition*> Conditions;

    virtual void ExecuteEvent(...);  // Blueprint overridable
};
```

**Enhancement Path:**
- Set properties via reflection
- Consider event_graph support for ExecuteEvent implementation

---

## Implementation Roadmap

### Phase 1: Quick Wins (Low Effort, High Impact)
1. **AbilityConfiguration** - Add TSubclassOf resolution from asset names
2. **ActivityConfiguration** - Same pattern as above
3. **BehaviorTree** - Add root composite and basic structure

### Phase 2: Medium Effort
4. **AnimationMontage** - Add slot track and segment creation
5. **BehaviorTree Tasks/Services** - Extend to full node tree

### Phase 3: High Effort (Future)
6. **DialogueBlueprint** - Full dialogue tree generation
7. **Quest Generator** - New generator for quest state machines
8. **NarrativeEvent Enhancement** - Event logic generation

---

## Technical Notes

### TSubclassOf Resolution Pattern

For all generators that need to reference other generated assets:

```cpp
// Deferred resolution system
struct FDeferredClassRef {
    UObject* TargetAsset;
    FName PropertyName;
    FString ReferencedAssetName;
};

TArray<FDeferredClassRef> DeferredRefs;

// After all assets generated, resolve references
void ResolveDeferredReferences()
{
    for (const auto& Ref : DeferredRefs)
    {
        FString AssetPath = FindAssetPath(Ref.ReferencedAssetName);
        UClass* Class = LoadClass<UObject>(nullptr, *AssetPath);
        if (Class)
        {
            SetPropertyValue(Ref.TargetAsset, Ref.PropertyName, Class);
        }
    }
}
```

### Manifest Dependency Order

Current order handles most dependencies:
1. Tags
2. Enumerations
3. Blackboards
4. FloatCurves
5. InputActions/Contexts
6. Materials/Functions
7. GameplayEffects
8. GameplayAbilities
9. Actor/WidgetBlueprints
10. **NEW: AbilityConfiguration (after GA)**
11. **NEW: ActivityConfiguration (after Activities)**
12. BehaviorTrees
13. NPCDefinitions

---

## Sources

**Web Research:**
- [Epic Developer Forums - BTTaskNode](https://forums.unrealengine.com/t/how-to-get-a-bttasknode-working-with-an-editor-based-behavior-tree/448028)
- [Unreal Property System (Reflection)](https://www.unrealengine.com/en-US/blog/unreal-property-system-reflection)
- [Creating Montage in C++](https://forums.unrealengine.com/t/how-to-create-a-montage-and-play-it-in-c/1281309)
- [UAnimMontage::AddSlot Documentation](https://docs.unrealengine.com/4.26/en-US/API/Runtime/Engine/Animation/UAnimMontage/AddSlot/)
- [Narrative Tools Documentation](https://docs.narrativetools.io/)

**Local Headers:**
- `UE_5.7/Engine/Source/Runtime/AIModule/Classes/BehaviorTree/BTCompositeNode.h`
- `UE_5.7/Engine/Source/Runtime/Engine/Classes/Animation/AnimMontage.h`
- `NarrativePro22B57/Source/NarrativeArsenal/Public/Tales/Dialogue.h`
- `NarrativePro22B57/Source/NarrativeArsenal/Public/Tales/Quest.h`
- `NarrativePro22B57/Source/NarrativeArsenal/Public/Tales/QuestSM.h`
- `NarrativePro22B57/Source/NarrativeArsenal/Public/Tales/NarrativeEvent.h`
