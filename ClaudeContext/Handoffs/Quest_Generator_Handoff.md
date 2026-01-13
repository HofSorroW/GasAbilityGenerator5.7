# Quest Generator Implementation Handoff

**Terminal 2 Research Output** | January 2026

This document provides complete specifications for implementing a Quest Generator (QBP_) in the GasAbilityGenerator plugin.

---

## Executive Summary

Quests in Narrative Pro follow the same Blueprint-based pattern as Dialogues. A `UQuestBlueprint` contains a `UQuestBlueprintGeneratedClass` which stores a `QuestTemplate` (UQuest). The quest defines a state machine with states, branches, and tasks.

**Implementation Complexity: HIGH** - Requires creating Quest state machine nodes, which is significantly more complex than DialogueBlueprint.

---

## Core Class Architecture

### Header Files (Narrative Pro)
```
NarrativeArsenal/Public/Tales/Quest.h              - UQuest class
NarrativeArsenal/Public/Tales/QuestSM.h            - UQuestState, UQuestBranch, UNarrativeTask
NarrativeArsenal/Public/Tales/QuestTask.h          - Base task class
NarrativeArsenal/Public/Tales/NarrativeDataTask.h  - Data task (DataAsset)
NarrativeArsenal/Public/Tales/QuestBlueprintGeneratedClass.h
NarrativeQuestEditor/Public/QuestBlueprint.h       - Editor asset
```

### Class Hierarchy

```
UQuestBlueprint (UBlueprint)
├── QuestGraph: UEdGraph*                    // Visual graph editor
├── QuestTemplate: UQuest*                   // Runtime template
└── GeneratedClass -> UQuestBlueprintGeneratedClass
                      └── QuestTemplate: UQuest*

UQuest (UObject)
├── QuestName: FText
├── QuestDescription: FText
├── bTracked: bool                           // Show navigation markers
├── QuestDialogue: TSubclassOf<UDialogue>    // Associated dialogue
├── QuestStartState: UQuestState*            // Entry point
├── CurrentState: UQuestState*               // Runtime state
├── States: TArray<UQuestState*>             // All states
├── Branches: TArray<UQuestBranch*>          // All branches
├── QuestCompletion: EQuestCompletion        // NotStarted, Started, Succeeded, Failed
├── QuestRequirements: TArray<UQuestRequirement*>
└── InheritableStates: TArray<UQuestState*>  // For child quests

UQuestState (UQuestNode -> UNarrativeNodeBase)
├── Branches: TArray<UQuestBranch*>          // Outgoing transitions
├── StateNodeType: EStateNodeType            // Regular, Success, Failure
├── ID: FName                                // Unique identifier
├── Description: FText                       // State description
├── Events: TArray<UNarrativeEvent*>         // Fire on enter/exit
└── Conditions: TArray<UNarrativeCondition*> // (Dialogues only)

UQuestBranch (UQuestNode -> UNarrativeNodeBase)
├── QuestTasks: TArray<UNarrativeTask*>      // Tasks to complete this branch
├── DestinationState: UQuestState*           // Where branch leads
├── bHidden: bool                            // Hide from UI
├── ID: FName
└── Events: TArray<UNarrativeEvent*>

UNarrativeTask (UObject)
├── RequiredQuantity: int32                  // How many times to complete
├── CurrentProgress: int32                   // Current progress
├── DescriptionOverride: FText               // Custom description
├── bOptional: bool                          // Is task optional
├── bHidden: bool                            // Hide from UI
├── TickInterval: float                      // For polling-based tasks
└── MarkerSettings: FTaskNavigationMarker    // Quest marker config
```

### Enums

```cpp
UENUM()
enum class EQuestCompletion : uint8
{
    NotStarted,
    Started,
    Succeeded,
    Failed
};

UENUM()
enum class EStateNodeType : uint8
{
    Regular,   // Normal state
    Success,   // Quest succeeded
    Failure    // Quest failed
};
```

---

## Quest State Machine Flow

```
[QuestStartState] ──Branch──> [State1] ──Branch──> [SuccessState]
                                   │
                              Branch (optional)
                                   │
                                   v
                              [FailureState]
```

- **States** are quest milestones/checkpoints
- **Branches** are transitions between states with tasks
- **Tasks** are objectives the player must complete (e.g., "Kill 5 enemies")

---

## UNarrativeDataTask (Built-in Task System)

Data Tasks are lightweight, data-driven tasks stored as DataAssets.

```cpp
UCLASS(BlueprintType, Blueprintable)
class UNarrativeDataTask : public UDataAsset
{
    UPROPERTY(EditAnywhere)
    FString TaskName;              // e.g., "FindItem", "KillNPC", "TalkToCharacter"

    UPROPERTY(EditAnywhere)
    FText TaskDescription;

    UPROPERTY(EditAnywhere)
    FString ArgumentName;          // Parameter name, e.g., "Item Name"

    UPROPERTY(EditAnywhere)
    FString TaskCategory;          // Organization category

    UPROPERTY(EditAnywhere)
    FString DefaultArgument;       // Default autofill value

    // Creates "TaskName_Argument" string (e.g., "FindItem_Sword")
    FString MakeTaskString(const FString& Argument) const;
};
```

**Common Task Types (create as DataAssets):**
- `FindItem` - Find/obtain an item
- `KillNPC` - Kill specific NPC
- `TalkToCharacter` - Talk to an NPC
- `ReachLocation` - Reach a location
- `CompleteQuest` - Complete another quest

---

## NarrativeEvent Integration

Events are attached to States and Branches via `Events` array (from UNarrativeNodeBase).

### Event Timing
```cpp
enum class EEventRuntime : uint8
{
    Start,  // State entered / Branch activated
    End,    // State exited / Branch completed
    Both    // Both times
};
```

### Built-in Events (Blueprints in NarrativePro/Content/Pro/Core/Tales/Events/)
| Event | Purpose |
|-------|---------|
| `NE_BeginQuest` | Start another quest |
| `NE_RestartQuest` | Restart a quest |
| `NE_BeginDialogue` | Start a dialogue |
| `NE_GiveXP` | Award experience points |
| `NE_AddGameplayTags` | Add tags to character |
| `NE_RemoveGameplayTags` | Remove tags |
| `NE_AddFactions` | Add to faction |
| `NE_RemoveFactions` | Remove from faction |
| `NE_SetFactionAttitude` | Change faction relationship |
| `NE_ShowNotification` | Show UI notification |
| `NE_AddSaveCheckpoint` | Create save point |

### Event Properties
```cpp
UCLASS(Abstract, Blueprintable, EditInlineNew)
class UNarrativeEvent : public UObject
{
    UPROPERTY(EditAnywhere)
    bool bRefireOnLoad;           // Fire again when loading save?

    UPROPERTY(EditAnywhere)
    EEventRuntime EventRuntime;   // When to execute

    UPROPERTY(EditAnywhere)
    EEventFilter EventFilter;     // Anyone, OnlyNPCs, OnlyPlayers

    UPROPERTY(EditAnywhere)
    EPartyEventPolicy PartyEventPolicy;  // Party, AllPartyMembers, PartyLeader

    UPROPERTY(EditAnywhere, Instanced)
    TArray<UNarrativeCondition*> Conditions;  // Fire only if conditions met

    // Override this to implement custom behavior
    virtual void ExecuteEvent_Implementation(APawn* Target, APlayerController* Controller, UTalesComponent* NarrativeComponent);
};
```

---

## TalesComponent API (Quest Management)

The `UTalesComponent` on PlayerController manages quests.

```cpp
// Start a quest
UQuest* BeginQuest(TSubclassOf<UQuest> QuestClass, FName StartFromID = NAME_None);

// Quest state checks
bool IsQuestStartedOrFinished(TSubclassOf<UQuest> QuestClass) const;
bool IsQuestInProgress(TSubclassOf<UQuest> QuestClass) const;
bool IsQuestSucceeded(TSubclassOf<UQuest> QuestClass) const;
bool IsQuestFailed(TSubclassOf<UQuest> QuestClass) const;
bool IsQuestFinished(TSubclassOf<UQuest> QuestClass) const;

// Quest management
bool RestartQuest(TSubclassOf<UQuest> QuestClass, FName StartFromID = NAME_None);
bool ForgetQuest(TSubclassOf<UQuest> QuestClass);
UQuest* GetQuestInstance(TSubclassOf<UQuest> QuestClass) const;

// Task completion - drives quest progression
bool CompleteNarrativeDataTask(const UNarrativeDataTask* Task, const FString& Argument, int32 Quantity = 1);
bool HasCompletedTask(const UNarrativeDataTask* Task, const FString& Name, int32 Quantity = 1);
int32 GetNumberOfTimesTaskWasCompleted(const UNarrativeDataTask* Task, const FString& Name);

// Quest queries
TArray<UQuest*> GetFailedQuests() const;
TArray<UQuest*> GetSucceededQuests() const;
TArray<UQuest*> GetInProgressQuests() const;
TArray<UQuest*> GetAllQuests() const;
```

### Delegates (for UI/Systems)
```cpp
FOnQuestStarted OnQuestStarted;
FOnQuestSucceeded OnQuestSucceeded;
FOnQuestFailed OnQuestFailed;
FOnQuestForgotten OnQuestForgotten;
FOnQuestRestarted OnQuestRestarted;
FOnQuestNewState OnQuestNewState;
FOnQuestBranchCompleted OnQuestBranchCompleted;
FOnQuestTaskCompleted OnQuestTaskCompleted;
FOnQuestTaskProgressChanged OnQuestTaskProgressChanged;
```

---

## Proposed Manifest Schema

```yaml
quests:
  - name: QBP_FindAncientRelic
    folder: Quests/MainStory
    quest_name: "Find the Ancient Relic"
    quest_description: "Search the ruins for the legendary relic."
    tracked: true                              # Show navigation markers
    dialogue: DBP_RelicQuestDialogue           # Associated dialogue (optional)

    # Quest state machine
    states:
      - id: Start
        description: "Begin the search"
        type: regular                          # regular, success, failure
        events:
          - type: NE_ShowNotification
            runtime: Start
            properties:
              message: "Quest Started!"
        branches:
          - destination: FindClues
            tasks:
              - task: TalkToCharacter
                argument: ArchaeologistNPC
                quantity: 1

      - id: FindClues
        description: "Gather clues about the relic"
        type: regular
        branches:
          - destination: RetrieveRelic
            tasks:
              - task: FindItem
                argument: AncientScroll
                quantity: 3
                optional: false

      - id: RetrieveRelic
        description: "Retrieve the relic"
        type: regular
        branches:
          - destination: QuestComplete
            tasks:
              - task: FindItem
                argument: AncientRelic
                quantity: 1
          - destination: QuestFailed
            hidden: true
            tasks:
              - task: RelicDestroyed
                argument: AncientRelic

      - id: QuestComplete
        description: "Quest completed successfully"
        type: success
        events:
          - type: NE_GiveXP
            runtime: Start
            properties:
              xp_amount: 500

      - id: QuestFailed
        description: "The relic was destroyed"
        type: failure

    # Entry point
    start_state: Start
```

---

## Implementation Strategy

### Option A: Full Quest Generator (Complex)
Create quests programmatically with state machine nodes.

**Challenges:**
- `UQuestBlueprint` requires `QuestGraph` (UEdGraph) setup
- States/Branches need graph node positions for editor
- Complex dependency resolution for task DataAssets

**Implementation Steps:**
1. Create `FQuestBlueprintGenerator` class
2. Use `FKismetEditorUtilities::CreateBlueprint()` with QuestBlueprint types
3. Create UQuestState/UQuestBranch objects manually
4. Wire DestinationState pointers
5. Create/reference UNarrativeDataTask assets
6. Setup QuestGraph if editor compatibility needed

### Option B: Stub Generator (Recommended)
Generate quest assets with metadata only, require manual graph editing.

```cpp
FGenerationResult FQuestBlueprintGenerator::Generate(const FManifestQuestDefinition& Definition)
{
    // Create UQuestBlueprint
    UQuestBlueprint* QuestBP = NewObject<UQuestBlueprint>(Package, *Definition.Name, RF_Public | RF_Standalone | RF_Transactional);
    QuestBP->ParentClass = UQuest::StaticClass();
    QuestBP->BlueprintType = BPTYPE_Normal;

    FKismetEditorUtilities::CompileBlueprint(QuestBP);

    // Set CDO properties
    UObject* CDO = QuestBP->GeneratedClass->GetDefaultObject();
    if (CDO)
    {
        // Set QuestName via reflection
        FTextProperty* NameP = CastField<FTextProperty>(CDO->GetClass()->FindPropertyByName(TEXT("QuestName")));
        if (NameP) NameP->SetPropertyValue_InContainer(CDO, FText::FromString(Definition.QuestName));

        // Set QuestDescription
        FTextProperty* DescP = CastField<FTextProperty>(CDO->GetClass()->FindPropertyByName(TEXT("QuestDescription")));
        if (DescP) DescP->SetPropertyValue_InContainer(CDO, FText::FromString(Definition.QuestDescription));

        // Set bTracked
        FBoolProperty* TrackedP = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bTracked")));
        if (TrackedP) TrackedP->SetPropertyValue_InContainer(CDO, Definition.bTracked);
    }

    // Recompile and save...
}
```

---

## Dependencies

### Build.cs Requirements
```cpp
// Already included for DialogueBlueprint
PublicDependencyModuleNames.AddRange(new string[] {
    "NarrativeArsenal",
    "BlueprintGraph",
    "Kismet",
    "KismetCompiler"
});

// Add for Quest support
PrivateDependencyModuleNames.Add("NarrativeQuestEditor");  // For UQuestBlueprint
```

### Include Headers
```cpp
#include "Tales/Quest.h"
#include "Tales/QuestSM.h"
#include "Tales/QuestBlueprintGeneratedClass.h"
#include "QuestBlueprint.h"  // From NarrativeQuestEditor
```

---

## Recommendations

1. **Start with Option B (Stub Generator)** - Creates basic quest assets with name/description that can be completed in the quest editor. This follows the pattern already established for DialogueBlueprint before v3.8.

2. **Quest Editor Workflow** - Users would:
   - Define quest metadata in manifest
   - Generate stub asset
   - Open in Quest Editor to design state machine visually

3. **Future Enhancement** - Full state machine generation (Option A) could be added later, similar to how v3.8 added full dialogue tree generation to DialogueBlueprint.

4. **Data Task Generator** - Consider adding `FNarrativeDataTaskGenerator` to create common task types automatically.

---

## Related Files to Study

```
GasAbilityGeneratorGenerators.cpp:7431  - FDialogueBlueprintGenerator::Generate (pattern reference)
GasAbilityGeneratorTypes.h              - FManifestDialogueBlueprintDefinition (schema pattern)
QuestAssetFactory.cpp                   - Official quest creation pattern
QuestBlueprintCompiler.cpp              - How quests compile from graph
```
