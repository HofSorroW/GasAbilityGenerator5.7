# Dialogue-Quest Integration Handoff

**Terminal 2 Research Output** | January 2026

This document covers the Event and Condition systems in Narrative Pro, plus how Dialogues and Quests integrate with each other.

---

## Executive Summary

Narrative Pro uses **Events** to trigger game actions and **Conditions** to control visibility/availability of dialogue options and quest branches. Both are attached to nodes via arrays defined in `UNarrativeNodeBase`.

**Key Integration Points:**
1. Dialogue nodes can start quests via `NE_BeginQuest` event
2. Dialogue visibility can depend on quest state via `NC_IsQuestInProgress` condition
3. Quests can reference dialogues via `QuestDialogue` property
4. Both systems share the same Event/Condition architecture

---

## UNarrativeNodeBase (Shared Base Class)

All dialogue and quest nodes inherit from this base class which defines the Event/Condition attachment points.

```cpp
// Location: NarrativeArsenal/Public/Tales/NarrativeNodeBase.h

UCLASS()
class UNarrativeNodeBase : public UObject
{
    // Conditions that must pass for this node to be visible/available
    // NOTE: Currently only dialogues use conditions, quests ignore them
    UPROPERTY(EditAnywhere, Instanced, Category = "Events & Conditions")
    TArray<class UNarrativeCondition*> Conditions;

    // Events that fire when this node is reached
    // Supported by BOTH quests and dialogues
    UPROPERTY(EditAnywhere, Instanced, Category = "Events & Conditions")
    TArray<class UNarrativeEvent*> Events;

    // Execute all events based on runtime timing
    void ProcessEvents(APawn* Pawn, APlayerController* Controller, UTalesComponent* NarrativeComponent, EEventRuntime Runtime);

    // Check if all conditions pass
    bool AreConditionsMet(APawn* Pawn, APlayerController* Controller, UTalesComponent* NarrativeComponent);
};
```

---

## NarrativeEvent System

### Base Class
```cpp
// Location: NarrativeArsenal/Public/Tales/NarrativeEvent.h

UCLASS(Abstract, Blueprintable, EditInlineNew, AutoExpandCategories = ("Default"))
class UNarrativeEvent : public UObject
{
    // QUESTS ONLY: Should this event fire again when loading a save?
    // Set false for rewards (XP, items) that are already saved
    UPROPERTY(EditAnywhere, Category = "Event")
    bool bRefireOnLoad;

    // When to execute: Start, End, or Both
    UPROPERTY(EditAnywhere, Category = "Event")
    EEventRuntime EventRuntime;

    // Who can receive this event: Anyone, OnlyNPCs, OnlyPlayers
    UPROPERTY(EditAnywhere, Category = "Event")
    EEventFilter EventFilter;

    // How to handle party dialogues: Party, AllPartyMembers, PartyLeader
    UPROPERTY(EditAnywhere, Category = "Parties")
    EPartyEventPolicy PartyEventPolicy;

    // Optional conditions that must pass for event to fire
    UPROPERTY(EditAnywhere, Instanced, Category = "Events & Conditions")
    TArray<class UNarrativeCondition*> Conditions;

    // Target selection based on EventFilter
    UPROPERTY(EditAnywhere, Category = "Event", meta = (EditCondition = "EventFilter == EEventFilter::EF_Anyone"))
    TArray<UCharacterDefinition*> CharacterTargets;

    UPROPERTY(EditAnywhere, Category = "Event", meta = (EditCondition = "EventFilter == EEventFilter::EF_OnlyNPCs"))
    TArray<UNPCDefinition*> NPCTargets;

    UPROPERTY(EditAnywhere, Category = "Event", meta = (EditCondition = "EventFilter == EEventFilter::EF_OnlyPlayers"))
    TArray<UPlayerDefinition*> PlayerTargets;

    // Override to implement custom behavior
    UFUNCTION(BlueprintNativeEvent)
    void ExecuteEvent(APawn* Target, APlayerController* Controller, UTalesComponent* NarrativeComponent);

    // Text shown on node in editor graph
    UFUNCTION(BlueprintNativeEvent)
    FString GetGraphDisplayText();

    // Hint text shown after dialogue option (e.g., "[Begin Quest]")
    UFUNCTION(BlueprintNativeEvent)
    FText GetHintText();
};
```

### Event Runtime Timing
```cpp
UENUM(BlueprintType)
enum class EEventRuntime : uint8
{
    Start,  // Dialogue: line starts | Quest State: entered | Quest Branch: activated
    End,    // Dialogue: line ends   | Quest State: exited  | Quest Branch: completed
    Both    // Execute at both start and end (fires twice)
};
```

### Event Filter
```cpp
UENUM(BlueprintType)
enum class EEventFilter : uint8
{
    EF_Anyone UMETA(DisplayName="Anyone"),
    EF_OnlyNPCs UMETA(DisplayName="Only NPCs"),
    EF_OnlyPlayers UMETA(DisplayName="Only Players")
};
```

### Party Event Policy
```cpp
UENUM()
enum class EPartyEventPolicy
{
    Party,            // Run on party's TalesComponent
    AllPartyMembers,  // Run on every member (e.g., all get reward)
    PartyLeader       // Run only on leader
};
```

---

## Built-in NarrativeEvent Types

Located in: `NarrativePro/Content/Pro/Core/Tales/Events/`

| Asset | Class | Purpose | Properties |
|-------|-------|---------|------------|
| `NE_BeginQuest` | Blueprint | Start a quest | QuestClass: TSubclassOf<UQuest> |
| `NE_RestartQuest` | Blueprint | Restart quest | QuestClass, StartFromID |
| `NE_BeginDialogue` | Blueprint | Start dialogue | DialogueClass: TSubclassOf<UDialogue> |
| `NE_GiveXP` | Blueprint | Award XP | XPAmount: int32 |
| `NE_AddGameplayTags` | Blueprint | Add tags | Tags: FGameplayTagContainer |
| `NE_RemoveGameplayTags` | Blueprint | Remove tags | Tags: FGameplayTagContainer |
| `NE_AddFactions` | Blueprint | Add to faction | Factions: TArray<FFactionInfo> |
| `NE_RemoveFactions` | Blueprint | Remove faction | Factions |
| `NE_SetFactionAttitude` | Blueprint | Change attitude | SourceFaction, TargetFaction, Attitude |
| `NE_ShowNotification` | Blueprint | Show UI notice | Message, Duration |
| `NE_AddSaveCheckpoint` | Blueprint | Create save | CheckpointName |
| `NE_PrintString` | Blueprint | Debug print | Message |

### Special: NE_AddGoalToNPC
Located in: `NarrativeArsenal/Public/AI/Activities/NarrativeEvent_AddGoalToNPC.h`

Adds AI goals to NPCs during dialogue/quests.

---

## NarrativeCondition System

### Base Class
```cpp
// Location: NarrativeArsenal/Public/Tales/NarrativeCondition.h

UCLASS(Abstract, Blueprintable, EditInlineNew, AutoExpandCategories = ("Default"))
class UNarrativeCondition : public UObject
{
    // Invert the result (NOT operator)
    UPROPERTY(EditAnywhere, Category = "Conditions")
    bool bNot = false;

    // Who to check: DontTarget, AnyCharacter, OnlyNPCs, OnlyPlayers
    UPROPERTY(EditAnywhere, Category = "Condition")
    EConditionFilter ConditionFilter;

    // How to handle party dialogues
    UPROPERTY(EditAnywhere, Category = "Parties")
    EPartyConditionPolicy PartyConditionPolicy = EPartyConditionPolicy::AnyPlayerPasses;

    // Target selection (similar to events)
    UPROPERTY(EditAnywhere, Category = "Condition", meta = (EditCondition = "ConditionFilter == CF_AnyCharacter"))
    TArray<UCharacterDefinition*> CharacterTargets;

    // Override to implement custom logic
    UFUNCTION(BlueprintNativeEvent)
    bool CheckCondition(APawn* Target, APlayerController* Controller, UTalesComponent* NarrativeComponent);

    // Text shown on node in editor
    UFUNCTION(BlueprintNativeEvent)
    FString GetGraphDisplayText();
};
```

### Condition Filter
```cpp
UENUM(BlueprintType)
enum class EConditionFilter : uint8
{
    CF_DontTarget UMETA(DisplayName="No Target"),      // Time of day, etc.
    CF_AnyCharacter UMETA(DisplayName="Anyone"),
    CF_OnlyNPCs UMETA(DisplayName="Only NPCs"),
    CF_OnlyPlayers UMETA(DisplayName="Only Players")
};
```

### Party Condition Policy
```cpp
UENUM()
enum class EPartyConditionPolicy
{
    AnyPlayerPasses,    // Anyone in party passes = condition passes
    PartyPasses,        // Check party's TalesComponent
    AllPlayersPass,     // Everyone must pass
    PartyLeaderPasses   // Only check leader
};
```

---

## Built-in NarrativeCondition Types

Located in: `NarrativePro/Content/Pro/Core/Tales/Conditions/`

| Asset | Purpose | Properties |
|-------|---------|------------|
| `NC_HasCompletedDataTask` | Check if data task completed | Task, Argument, Quantity |
| `NC_HasDialogueNodePlayed` | Check if dialogue node was reached | DialogueClass, NodeID |
| `NC_IsQuestAtState` | Check quest is at specific state | QuestClass, StateID |
| `NC_IsQuestFailed` | Check if quest failed | QuestClass |
| `NC_IsQuestInProgress` | Check if quest active | QuestClass |
| `NC_IsQuestStartedOrFinished` | Check quest was started | QuestClass |
| `NC_IsQuestSucceeded` | Check if quest succeeded | QuestClass |
| `NC_IsDayTime` | Check if daytime | - |
| `NC_IsTimeInRange` | Check time range | StartHour, EndHour |

---

## Dialogue-Quest Integration Patterns

### Pattern 1: Dialogue Starts Quest

A dialogue option that begins a quest when selected:

```yaml
# In dialogue_blueprints manifest
dialogue_blueprints:
  - name: DBP_QuestGiver
    dialogue_tree:
      nodes:
        - id: offer_quest
          type: npc
          speaker: NPCDef_QuestGiver
          text: "Will you help me find my lost artifact?"
          player_replies: [accept_quest, decline_quest]

        - id: accept_quest
          type: player
          text: "I'll help you."
          option_text: "Accept quest"
          events:
            - type: NE_BeginQuest
              runtime: Start
              properties:
                quest_class: QBP_FindArtifact  # Quest to start
```

### Pattern 2: Dialogue Gated by Quest State

A dialogue option only visible if quest is in progress:

```yaml
dialogue_blueprints:
  - name: DBP_QuestUpdate
    dialogue_tree:
      nodes:
        - id: quest_update
          type: npc
          speaker: NPCDef_QuestGiver
          text: "Have you found the artifact yet?"
          conditions:
            - type: NC_IsQuestInProgress
              properties:
                quest_class: QBP_FindArtifact
          player_replies: [found_it, still_looking]
```

### Pattern 3: Quest References Dialogue

A quest with associated dialogue:

```yaml
quests:
  - name: QBP_FindArtifact
    quest_name: "The Lost Artifact"
    dialogue: DBP_QuestGiver  # Link to dialogue asset
    states:
      - id: TalkToGiver
        branches:
          - destination: SearchRuins
            tasks:
              - task: TalkToCharacter
                argument: QuestGiver
```

### Pattern 4: Quest State Fires Dialogue

Quest state that starts a dialogue when entered:

```yaml
quests:
  - name: QBP_Tutorial
    states:
      - id: MeetMentor
        events:
          - type: NE_BeginDialogue
            runtime: Start
            properties:
              dialogue_class: DBP_MentorIntro
```

### Pattern 5: Conditional Dialogue Branch

Different dialogue based on quest completion:

```yaml
dialogue_blueprints:
  - name: DBP_NPCGreeting
    dialogue_tree:
      nodes:
        # Shown if quest completed
        - id: grateful_greeting
          type: npc
          text: "Thank you for finding my artifact!"
          conditions:
            - type: NC_IsQuestSucceeded
              properties:
                quest_class: QBP_FindArtifact

        # Shown if quest NOT completed (using bNot)
        - id: normal_greeting
          type: npc
          text: "Hello, traveler."
          conditions:
            - type: NC_IsQuestSucceeded
              not: true
              properties:
                quest_class: QBP_FindArtifact
```

---

## Proposed Manifest Schema for Events/Conditions

### Event Schema
```yaml
events:
  - type: NE_BeginQuest          # Event class (built-in or custom)
    runtime: Start               # Start, End, Both
    refire_on_load: false        # For quests only
    filter: OnlyPlayers          # Anyone, OnlyNPCs, OnlyPlayers
    party_policy: Party          # Party, AllPartyMembers, PartyLeader
    conditions:                   # Optional conditions for this event
      - type: NC_HasCompletedDataTask
        properties:
          task: TalkToCharacter
          argument: QuestGiver
    properties:                  # Event-specific properties
      quest_class: QBP_MyQuest
      start_from_id: ""
```

### Condition Schema
```yaml
conditions:
  - type: NC_IsQuestInProgress   # Condition class
    not: false                   # Invert result
    filter: OnlyPlayers          # DontTarget, Anyone, OnlyNPCs, OnlyPlayers
    party_policy: AnyPlayerPasses
    properties:                  # Condition-specific properties
      quest_class: QBP_MyQuest
```

---

## Implementation Considerations

### For v3.8+ DialogueBlueprint Generator

The dialogue tree generation already supports events and conditions on nodes. The current implementation logs them for manual setup. To fully automate:

1. **Create Event Instances:**
```cpp
// Create event instance
UClass* EventClass = LoadClass<UNarrativeEvent>(nullptr, TEXT("/Game/Pro/Core/Tales/Events/NE_BeginQuest.NE_BeginQuest_C"));
if (EventClass)
{
    UNarrativeEvent* Event = NewObject<UNarrativeEvent>(DialogueNode, EventClass);
    // Set properties via reflection
    Event->EventRuntime = EEventRuntime::Start;
    DialogueNode->Events.Add(Event);
}
```

2. **Create Condition Instances:**
```cpp
UClass* ConditionClass = LoadClass<UNarrativeCondition>(nullptr, TEXT("/Game/Pro/Core/Tales/Conditions/NC_IsQuestInProgress.NC_IsQuestInProgress_C"));
if (ConditionClass)
{
    UNarrativeCondition* Condition = NewObject<UNarrativeCondition>(DialogueNode, ConditionClass);
    // Set properties
    DialogueNode->Conditions.Add(Condition);
}
```

### Custom Event/Condition Types

Projects can create custom events/conditions by:
1. Creating a Blueprint class inheriting from `NarrativeEvent` or `NarrativeCondition`
2. Overriding `ExecuteEvent` or `CheckCondition`
3. Referencing by path in manifest

---

## Related Files

```
NarrativeArsenal/Public/Tales/NarrativeNodeBase.h   - Base class with Events/Conditions arrays
NarrativeArsenal/Public/Tales/NarrativeEvent.h      - Event base class
NarrativeArsenal/Public/Tales/NarrativeCondition.h  - Condition base class
NarrativeArsenal/Public/Tales/Quest.h               - UQuest::QuestDialogue property
NarrativeArsenal/Public/Tales/DialogueSM.h          - Dialogue node classes
NarrativeArsenal/Public/Tales/TalesComponent.h      - Quest/Dialogue management API
```

---

## Summary

The Event/Condition system provides powerful integration between Dialogues and Quests:

| Feature | Dialogues | Quests |
|---------|-----------|--------|
| Events | Yes (all nodes) | Yes (states, branches) |
| Conditions | Yes (control visibility) | No (ignored) |
| Fire timing | Start/End of line | State enter/exit, Branch activate/complete |
| Party support | Yes | Yes (via TalesComponent) |

**Key API Methods:**
- `UTalesComponent::BeginQuest()` - Start quest
- `UTalesComponent::BeginDialogue()` - Start dialogue
- `UTalesComponent::IsQuestInProgress()` - Check quest state
- `UTalesComponent::CompleteNarrativeDataTask()` - Progress quests
