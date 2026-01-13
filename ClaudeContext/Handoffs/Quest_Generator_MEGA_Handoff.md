# Quest Generator MEGA Handoff

**Terminal 2 Complete Research** | January 2026 | Ready for Implementation

This document contains ALL research, architecture, and implementation code for adding Quest Blueprint generation (QBP_) to GasAbilityGenerator.

---

## TABLE OF CONTENTS

1. [Executive Summary](#1-executive-summary)
2. [Key Discovery: No Graph Nodes Required](#2-key-discovery)
3. [Narrative Pro Quest Architecture](#3-quest-architecture)
4. [Event & Condition Systems](#4-event-condition-systems)
5. [Dialogue-Quest Integration](#5-dialogue-quest-integration)
6. [Implementation Code](#6-implementation-code)
7. [Manifest YAML Schema](#7-manifest-yaml-schema)
8. [Build Configuration](#8-build-configuration)
9. [Testing Checklist](#9-testing-checklist)

---

## 1. EXECUTIVE SUMMARY

### What We're Building
A `FQuestBlueprintGenerator` that creates complete quest state machines from YAML, following the same pattern as DialogueBlueprint v3.8.

### Complexity Level
**MEDIUM** - Same pattern as DialogueBlueprint, no graph nodes needed.

### Files to Modify
| File | Changes |
|------|---------|
| `GasAbilityGeneratorTypes.h` | Add 4 new structs |
| `GasAbilityGeneratorParser.cpp` | Add YAML parsing |
| `GasAbilityGeneratorGenerators.h` | Add generator class |
| `GasAbilityGeneratorGenerators.cpp` | Add ~250 lines implementation |
| `GasAbilityGeneratorCommandlet.cpp` | Add generation call |
| `GasAbilityGeneratorWindow.cpp` | Add generation call |
| `GasAbilityGenerator.Build.cs` | Add module dependency |

---

## 2. KEY DISCOVERY

### No Graph Nodes Required!

Both `UDialogueBlueprint` and `UQuestBlueprint` auto-create their templates:

```cpp
// From QuestBlueprint.cpp
UQuestBlueprint::UQuestBlueprint(const FObjectInitializer& ObjectInitializer)
{
    QuestTemplate = CreateDefaultSubobject<UQuest>(TEXT("QuestTemplate"));
    QuestTemplate->SetFlags(RF_Transactional | RF_ArchetypeObject);
}
```

The compiler duplicates the template regardless of graph state:

```cpp
// From QuestBlueprintCompiler.cpp line 201
UQuest* NewQuestTemplate = Cast<UQuest>(StaticDuplicateObject(QuestBP->QuestTemplate, BPGClass, ...));
BPGClass->SetQuestTemplate(NewQuestTemplate);
```

**Result**: Work directly on `QuestTemplate`, skip graph nodes entirely. Quest works at runtime, editor shows empty graph (acceptable trade-off).

---

## 3. QUEST ARCHITECTURE

### Header File Locations
```
NarrativeArsenal/Public/Tales/Quest.h                    - UQuest
NarrativeArsenal/Public/Tales/QuestSM.h                  - UQuestState, UQuestBranch
NarrativeArsenal/Public/Tales/QuestTask.h                - UNarrativeTask
NarrativeArsenal/Public/Tales/QuestBlueprintGeneratedClass.h
NarrativeQuestEditor/Public/QuestBlueprint.h             - UQuestBlueprint
```

### Class Hierarchy

```
UQuestBlueprint (Editor Asset)
├── QuestGraph: UEdGraph*              // Visual editor (we skip this)
├── QuestTemplate: UQuest*             // Runtime template (we populate this)
└── GeneratedClass -> UQuestBlueprintGeneratedClass
                      └── QuestTemplate (duplicated at compile)

UQuest (Runtime)
├── QuestName: FText
├── QuestDescription: FText
├── bTracked: bool                     // Navigation markers
├── QuestDialogue: TSubclassOf<UDialogue>
├── QuestStartState: UQuestState*      // Entry point
├── CurrentState: UQuestState*         // Runtime tracking
├── States: TArray<UQuestState*>
├── Branches: TArray<UQuestBranch*>
├── QuestCompletion: EQuestCompletion  // NotStarted, Started, Succeeded, Failed
└── QuestRequirements: TArray<UQuestRequirement*>

UQuestState (Milestone/Checkpoint)
├── ID: FName                          // Unique identifier
├── Description: FText
├── StateNodeType: EStateNodeType      // Regular, Success, Failure
├── Branches: TArray<UQuestBranch*>    // Outgoing transitions
├── Events: TArray<UNarrativeEvent*>
└── Conditions: TArray<UNarrativeCondition*>  // (Dialogues only)

UQuestBranch (Transition)
├── ID: FName
├── DestinationState: UQuestState*     // Where branch leads
├── QuestTasks: TArray<UNarrativeTask*>
├── bHidden: bool
└── Events: TArray<UNarrativeEvent*>

UNarrativeTask (Objective)
├── RequiredQuantity: int32
├── CurrentProgress: int32
├── DescriptionOverride: FText
├── bOptional: bool
├── bHidden: bool
├── TickInterval: float
└── MarkerSettings: FTaskNavigationMarker
```

### State Machine Flow

```
[QuestStartState] ──Branch(Tasks)──> [State1] ──Branch(Tasks)──> [SuccessState]
                                         │
                                    Branch (optional)
                                         │
                                         v
                                    [FailureState]
```

### Key UQuest Methods

```cpp
// From Quest.cpp
void UQuest::AddState(UQuestState* State) { States.Add(State); }
void UQuest::AddBranch(UQuestBranch* Branch) { Branches.Add(Branch); }
void UQuest::SetQuestStartState(UQuestState* State) { QuestStartState = State; }
void UQuest::SetQuestName(const FText& NewName) { QuestName = NewName; }
void UQuest::SetQuestDescription(const FText& NewDescription) { QuestDescription = NewDescription; }
```

### Validation (from Quest.cpp:81)
```cpp
// A valid quest requires:
if (States.Num() == 0 || Branches.Num() == 0 || !QuestStartState)
{
    return false;  // Invalid quest
}
```

### Enums

```cpp
UENUM()
enum class EQuestCompletion : uint8
{
    QC_NotStarted,
    QC_Started,
    QC_Succeded,  // Note: typo in original
    QC_Failed
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

## 4. EVENT & CONDITION SYSTEMS

### UNarrativeEvent (Base Class)

Location: `NarrativeArsenal/Public/Tales/NarrativeEvent.h`

```cpp
UCLASS(Abstract, Blueprintable, EditInlineNew)
class UNarrativeEvent : public UObject
{
    // When to fire: Start, End, Both
    UPROPERTY(EditAnywhere)
    EEventRuntime EventRuntime;

    // Who receives: Anyone, OnlyNPCs, OnlyPlayers
    UPROPERTY(EditAnywhere)
    EEventFilter EventFilter;

    // Party handling: Party, AllPartyMembers, PartyLeader
    UPROPERTY(EditAnywhere)
    EPartyEventPolicy PartyEventPolicy;

    // QUESTS ONLY: Fire again on save load?
    UPROPERTY(EditAnywhere)
    bool bRefireOnLoad;

    // Conditions that must pass for event to fire
    UPROPERTY(EditAnywhere, Instanced)
    TArray<UNarrativeCondition*> Conditions;

    // Target arrays based on EventFilter
    TArray<UCharacterDefinition*> CharacterTargets;
    TArray<UNPCDefinition*> NPCTargets;
    TArray<UPlayerDefinition*> PlayerTargets;

    // Override to implement
    virtual void ExecuteEvent_Implementation(APawn* Target, APlayerController* Controller, UTalesComponent* NarrativeComponent);
};
```

### Event Runtime Enum

```cpp
UENUM(BlueprintType)
enum class EEventRuntime : uint8
{
    Start,  // State entered / Branch activated / Line starts
    End,    // State exited / Branch completed / Line ends
    Both    // Execute twice
};
```

### Built-in Events (NarrativePro/Content/Pro/Core/Tales/Events/)

| Asset | Purpose | Key Properties |
|-------|---------|----------------|
| `NE_BeginQuest` | Start a quest | QuestClass |
| `NE_RestartQuest` | Restart quest | QuestClass, StartFromID |
| `NE_BeginDialogue` | Start dialogue | DialogueClass |
| `NE_GiveXP` | Award XP | XPAmount |
| `NE_AddGameplayTags` | Add tags | Tags |
| `NE_RemoveGameplayTags` | Remove tags | Tags |
| `NE_AddFactions` | Add to faction | Factions |
| `NE_RemoveFactions` | Remove faction | Factions |
| `NE_SetFactionAttitude` | Change attitude | SourceFaction, TargetFaction, Attitude |
| `NE_ShowNotification` | UI notification | Message, Duration |
| `NE_AddSaveCheckpoint` | Create save | CheckpointName |
| `NE_PrintString` | Debug | Message |

### UNarrativeCondition (Base Class)

Location: `NarrativeArsenal/Public/Tales/NarrativeCondition.h`

```cpp
UCLASS(Abstract, Blueprintable, EditInlineNew)
class UNarrativeCondition : public UObject
{
    // Invert result (NOT operator)
    UPROPERTY(EditAnywhere)
    bool bNot = false;

    // Who to check: DontTarget, AnyCharacter, OnlyNPCs, OnlyPlayers
    UPROPERTY(EditAnywhere)
    EConditionFilter ConditionFilter;

    // Party handling
    UPROPERTY(EditAnywhere)
    EPartyConditionPolicy PartyConditionPolicy;

    // Override to implement
    virtual bool CheckCondition_Implementation(APawn* Target, APlayerController* Controller, UTalesComponent* NarrativeComponent);
};
```

### Built-in Conditions (NarrativePro/Content/Pro/Core/Tales/Conditions/)

| Asset | Purpose | Properties |
|-------|---------|------------|
| `NC_HasCompletedDataTask` | Task completed? | Task, Argument, Quantity |
| `NC_HasDialogueNodePlayed` | Node played? | DialogueClass, NodeID |
| `NC_IsQuestAtState` | Quest at state? | QuestClass, StateID |
| `NC_IsQuestFailed` | Quest failed? | QuestClass |
| `NC_IsQuestInProgress` | Quest active? | QuestClass |
| `NC_IsQuestStartedOrFinished` | Quest started? | QuestClass |
| `NC_IsQuestSucceeded` | Quest succeeded? | QuestClass |
| `NC_IsDayTime` | Is daytime? | - |
| `NC_IsTimeInRange` | Time range? | StartHour, EndHour |

### UNarrativeNodeBase (Shared Base)

Both dialogue and quest nodes inherit from this:

```cpp
// Location: NarrativeArsenal/Public/Tales/NarrativeNodeBase.h
UCLASS()
class UNarrativeNodeBase : public UObject
{
    // Conditions for visibility (dialogues only currently)
    UPROPERTY(EditAnywhere, Instanced)
    TArray<UNarrativeCondition*> Conditions;

    // Events that fire when node reached
    UPROPERTY(EditAnywhere, Instanced)
    TArray<UNarrativeEvent*> Events;

    // Unique identifier
    UPROPERTY(EditAnywhere)
    FName ID;

    void ProcessEvents(APawn*, APlayerController*, UTalesComponent*, EEventRuntime);
    bool AreConditionsMet(APawn*, APlayerController*, UTalesComponent*);
};
```

---

## 5. DIALOGUE-QUEST INTEGRATION

### Quest Properties for Dialogue

```cpp
// In UQuest
UPROPERTY(EditAnywhere)
TSubclassOf<UDialogue> QuestDialogue;  // Associated dialogue

UPROPERTY(EditAnywhere)
FDialoguePlayParams QuestDialoguePlayParams;  // Play settings
```

### Integration Patterns

**Pattern 1: Dialogue Starts Quest**
```yaml
dialogue_blueprints:
  - name: DBP_QuestGiver
    dialogue_tree:
      nodes:
        - id: accept_quest
          type: player
          text: "I'll help you."
          events:
            - type: NE_BeginQuest
              runtime: Start
              properties:
                quest_class: QBP_FindArtifact
```

**Pattern 2: Dialogue Gated by Quest State**
```yaml
dialogue_blueprints:
  - name: DBP_QuestUpdate
    dialogue_tree:
      nodes:
        - id: quest_update
          type: npc
          text: "Have you found it yet?"
          conditions:
            - type: NC_IsQuestInProgress
              properties:
                quest_class: QBP_FindArtifact
```

**Pattern 3: Quest References Dialogue**
```yaml
quest_blueprints:
  - name: QBP_FindArtifact
    dialogue: DBP_QuestGiver  # Associated dialogue
```

**Pattern 4: Quest State Fires Dialogue**
```yaml
quest_blueprints:
  - name: QBP_Tutorial
    states:
      - id: MeetMentor
        events:
          - type: NE_BeginDialogue
            runtime: Start
            properties:
              dialogue_class: DBP_MentorIntro
```

### TalesComponent API

```cpp
// Quest management
UQuest* BeginQuest(TSubclassOf<UQuest> QuestClass, FName StartFromID = NAME_None);
bool RestartQuest(TSubclassOf<UQuest> QuestClass, FName StartFromID = NAME_None);
bool ForgetQuest(TSubclassOf<UQuest> QuestClass);

// Quest state checks
bool IsQuestStartedOrFinished(TSubclassOf<UQuest>) const;
bool IsQuestInProgress(TSubclassOf<UQuest>) const;
bool IsQuestSucceeded(TSubclassOf<UQuest>) const;
bool IsQuestFailed(TSubclassOf<UQuest>) const;

// Task completion
bool CompleteNarrativeDataTask(const UNarrativeDataTask* Task, const FString& Argument, int32 Quantity = 1);

// Dialogue
UDialogue* BeginDialogue(TSubclassOf<UDialogue> DialogueClass, FDialoguePlayParams PlayParams);
```

---

## 6. IMPLEMENTATION CODE

### 6.1 Struct Definitions (GasAbilityGeneratorTypes.h)

Add after `FManifestDialogueBlueprintDefinition`:

```cpp
// ============================================================================
// QUEST BLUEPRINT DEFINITIONS
// ============================================================================

USTRUCT()
struct FManifestQuestTaskDefinition
{
    GENERATED_BODY()

    UPROPERTY()
    FString TaskClass;  // Task blueprint class (e.g., "GoToLocation", "FindItem")

    UPROPERTY()
    FString Argument;   // Task argument (e.g., location name, item name)

    UPROPERTY()
    int32 Quantity = 1;

    UPROPERTY()
    bool bOptional = false;

    UPROPERTY()
    bool bHidden = false;

    FString ComputeHash() const
    {
        return FString::Printf(TEXT("%s_%s_%d_%d_%d"),
            *TaskClass, *Argument, Quantity, bOptional ? 1 : 0, bHidden ? 1 : 0);
    }
};

USTRUCT()
struct FManifestQuestBranchDefinition
{
    GENERATED_BODY()

    UPROPERTY()
    FString Id;

    UPROPERTY()
    FString DestinationState;

    UPROPERTY()
    TArray<FManifestQuestTaskDefinition> Tasks;

    UPROPERTY()
    TArray<FManifestDialogueEventDefinition> Events;  // Reuse existing

    UPROPERTY()
    bool bHidden = false;

    FString ComputeHash() const
    {
        FString Hash = FString::Printf(TEXT("%s_%s_%d"), *Id, *DestinationState, bHidden ? 1 : 0);
        for (const auto& Task : Tasks) Hash += Task.ComputeHash();
        for (const auto& Event : Events) Hash += Event.ComputeHash();
        return Hash;
    }
};

USTRUCT()
struct FManifestQuestStateDefinition
{
    GENERATED_BODY()

    UPROPERTY()
    FString Id;

    UPROPERTY()
    FString Description;

    UPROPERTY()
    FString Type = TEXT("regular");  // "regular", "success", "failure"

    UPROPERTY()
    TArray<FManifestQuestBranchDefinition> Branches;

    UPROPERTY()
    TArray<FManifestDialogueEventDefinition> Events;

    FString ComputeHash() const
    {
        FString Hash = FString::Printf(TEXT("%s_%s_%s"), *Id, *Description, *Type);
        for (const auto& Branch : Branches) Hash += Branch.ComputeHash();
        for (const auto& Event : Events) Hash += Event.ComputeHash();
        return Hash;
    }
};

USTRUCT()
struct FManifestQuestBlueprintDefinition
{
    GENERATED_BODY()

    UPROPERTY()
    FString Name;

    UPROPERTY()
    FString Folder;

    UPROPERTY()
    FString QuestName;

    UPROPERTY()
    FString QuestDescription;

    UPROPERTY()
    bool bTracked = true;

    UPROPERTY()
    FString Dialogue;  // Associated dialogue (optional)

    UPROPERTY()
    FString StartState;

    UPROPERTY()
    TArray<FManifestQuestStateDefinition> States;

    FString ComputeHash() const
    {
        FString Hash = FString::Printf(TEXT("QBP_%s_%s_%s_%d_%s_%s"),
            *Name, *QuestName, *QuestDescription, bTracked ? 1 : 0, *Dialogue, *StartState);
        for (const auto& State : States) Hash += State.ComputeHash();
        return FMD5::HashAnsiString(*Hash);
    }
};
```

### 6.2 Add to FManifestData (GasAbilityGeneratorTypes.h)

```cpp
// In FManifestData struct:
UPROPERTY()
TArray<FManifestQuestBlueprintDefinition> QuestBlueprints;
```

### 6.3 Generator Header (GasAbilityGeneratorGenerators.h)

```cpp
// Add after FTaggedDialogueSetGenerator:

// ============================================================================
// Quest Blueprint Generator - Creates UQuestBlueprint with full state machine
// ============================================================================
class GASABILITYGENERATOR_API FQuestBlueprintGenerator : public FGeneratorBase
{
public:
    static FGenerationResult Generate(
        const FManifestQuestBlueprintDefinition& Definition,
        const FString& ProjectRoot,
        const FManifestData* ManifestData = nullptr);
};
```

### 6.4 Parser (GasAbilityGeneratorParser.cpp)

Add after `dialogue_blueprints` parsing:

```cpp
// ============================================================================
// Parse quest_blueprints
// ============================================================================
if (const YAML::Node& QuestBlueprintsNode = Root["quest_blueprints"])
{
    for (const auto& QuestNode : QuestBlueprintsNode)
    {
        FManifestQuestBlueprintDefinition QuestDef;
        QuestDef.Name = UTF8_TO_TCHAR(QuestNode["name"].as<std::string>("").c_str());
        QuestDef.Folder = UTF8_TO_TCHAR(QuestNode["folder"].as<std::string>("Quests").c_str());
        QuestDef.QuestName = UTF8_TO_TCHAR(QuestNode["quest_name"].as<std::string>("").c_str());
        QuestDef.QuestDescription = UTF8_TO_TCHAR(QuestNode["quest_description"].as<std::string>("").c_str());
        QuestDef.bTracked = QuestNode["tracked"].as<bool>(true);
        QuestDef.Dialogue = UTF8_TO_TCHAR(QuestNode["dialogue"].as<std::string>("").c_str());
        QuestDef.StartState = UTF8_TO_TCHAR(QuestNode["start_state"].as<std::string>("").c_str());

        // Parse states
        if (const YAML::Node& StatesNode = QuestNode["states"])
        {
            for (const auto& StateNode : StatesNode)
            {
                FManifestQuestStateDefinition StateDef;
                StateDef.Id = UTF8_TO_TCHAR(StateNode["id"].as<std::string>("").c_str());
                StateDef.Description = UTF8_TO_TCHAR(StateNode["description"].as<std::string>("").c_str());
                StateDef.Type = UTF8_TO_TCHAR(StateNode["type"].as<std::string>("regular").c_str());

                // Parse branches
                if (const YAML::Node& BranchesNode = StateNode["branches"])
                {
                    for (const auto& BranchNode : BranchesNode)
                    {
                        FManifestQuestBranchDefinition BranchDef;
                        BranchDef.Id = UTF8_TO_TCHAR(BranchNode["id"].as<std::string>("").c_str());
                        BranchDef.DestinationState = UTF8_TO_TCHAR(BranchNode["destination"].as<std::string>("").c_str());
                        BranchDef.bHidden = BranchNode["hidden"].as<bool>(false);

                        // Parse tasks
                        if (const YAML::Node& TasksNode = BranchNode["tasks"])
                        {
                            for (const auto& TaskNode : TasksNode)
                            {
                                FManifestQuestTaskDefinition TaskDef;
                                TaskDef.TaskClass = UTF8_TO_TCHAR(TaskNode["task"].as<std::string>("").c_str());
                                TaskDef.Argument = UTF8_TO_TCHAR(TaskNode["argument"].as<std::string>("").c_str());
                                TaskDef.Quantity = TaskNode["quantity"].as<int>(1);
                                TaskDef.bOptional = TaskNode["optional"].as<bool>(false);
                                TaskDef.bHidden = TaskNode["hidden"].as<bool>(false);
                                BranchDef.Tasks.Add(TaskDef);
                            }
                        }

                        // Parse branch events
                        if (const YAML::Node& EventsNode = BranchNode["events"])
                        {
                            for (const auto& EventNode : EventsNode)
                            {
                                FManifestDialogueEventDefinition EventDef;
                                EventDef.Type = UTF8_TO_TCHAR(EventNode["type"].as<std::string>("").c_str());
                                EventDef.Runtime = UTF8_TO_TCHAR(EventNode["runtime"].as<std::string>("Start").c_str());
                                if (const YAML::Node& PropsNode = EventNode["properties"])
                                {
                                    for (const auto& Prop : PropsNode)
                                    {
                                        FString Key = UTF8_TO_TCHAR(Prop.first.as<std::string>().c_str());
                                        FString Value = UTF8_TO_TCHAR(Prop.second.as<std::string>().c_str());
                                        EventDef.Properties.Add(Key, Value);
                                    }
                                }
                                BranchDef.Events.Add(EventDef);
                            }
                        }

                        StateDef.Branches.Add(BranchDef);
                    }
                }

                // Parse state events
                if (const YAML::Node& EventsNode = StateNode["events"])
                {
                    for (const auto& EventNode : EventsNode)
                    {
                        FManifestDialogueEventDefinition EventDef;
                        EventDef.Type = UTF8_TO_TCHAR(EventNode["type"].as<std::string>("").c_str());
                        EventDef.Runtime = UTF8_TO_TCHAR(EventNode["runtime"].as<std::string>("Start").c_str());
                        if (const YAML::Node& PropsNode = EventNode["properties"])
                        {
                            for (const auto& Prop : PropsNode)
                            {
                                FString Key = UTF8_TO_TCHAR(Prop.first.as<std::string>().c_str());
                                FString Value = UTF8_TO_TCHAR(Prop.second.as<std::string>().c_str());
                                EventDef.Properties.Add(Key, Value);
                            }
                        }
                        StateDef.Events.Add(EventDef);
                    }
                }

                QuestDef.States.Add(StateDef);
            }
        }

        OutManifestData.QuestBlueprints.Add(QuestDef);
    }
}
```

### 6.5 Generator Implementation (GasAbilityGeneratorGenerators.cpp)

Add includes at top:

```cpp
#include "Tales/Quest.h"
#include "Tales/QuestSM.h"
#include "Tales/QuestTask.h"
#include "Tales/QuestBlueprintGeneratedClass.h"
#include "QuestBlueprint.h"  // From NarrativeQuestEditor
```

Add helper function:

```cpp
// ============================================================================
// Helper: Create Quest Task from Definition
// ============================================================================
static UNarrativeTask* CreateQuestTask(UObject* Outer, const FManifestQuestTaskDefinition& TaskDef)
{
    if (TaskDef.TaskClass.IsEmpty())
    {
        return nullptr;
    }

    // Search paths for task classes
    TArray<FString> TaskSearchPaths = {
        FString::Printf(TEXT("/Game/%s/Quests/Tasks/%s.%s_C"), *GetProjectName(), *TaskDef.TaskClass, *TaskDef.TaskClass),
        FString::Printf(TEXT("/Game/Pro/Core/Quests/Tasks/%s.%s_C"), *TaskDef.TaskClass, *TaskDef.TaskClass),
        FString::Printf(TEXT("/NarrativePro/Quests/Tasks/%s.%s_C"), *TaskDef.TaskClass, *TaskDef.TaskClass),
        FString::Printf(TEXT("/Script/NarrativeArsenal.%s"), *TaskDef.TaskClass)
    };

    UClass* TaskClass = nullptr;
    for (const FString& Path : TaskSearchPaths)
    {
        TaskClass = LoadClass<UNarrativeTask>(nullptr, *Path);
        if (TaskClass) break;
    }

    if (!TaskClass)
    {
        UE_LOG(LogGasAbilityGenerator, Warning, TEXT("Could not find quest task class: %s"), *TaskDef.TaskClass);
        return nullptr;
    }

    UNarrativeTask* Task = NewObject<UNarrativeTask>(Outer, TaskClass);
    Task->SetFlags(RF_Transactional);
    Task->RequiredQuantity = TaskDef.Quantity;
    Task->bOptional = TaskDef.bOptional;
    Task->bHidden = TaskDef.bHidden;

    // Set argument via reflection
    if (!TaskDef.Argument.IsEmpty())
    {
        TArray<FName> ArgPropNames = { TEXT("Argument"), TEXT("TaskArgument"), TEXT("LocationName"), TEXT("ItemName"), TEXT("NPCName") };
        for (const FName& PropName : ArgPropNames)
        {
            if (FProperty* ArgProp = TaskClass->FindPropertyByName(PropName))
            {
                void* ValuePtr = ArgProp->ContainerPtrToValuePtr<void>(Task);
                ArgProp->ImportText(*TaskDef.Argument, ValuePtr, 0, Task);
                break;
            }
        }
    }

    return Task;
}
```

Add main generator:

```cpp
// ============================================================================
// Quest Blueprint Generator - Full State Machine Creation
// ============================================================================
FGenerationResult FQuestBlueprintGenerator::Generate(
    const FManifestQuestBlueprintDefinition& Definition,
    const FString& ProjectRoot,
    const FManifestData* ManifestData)
{
    FString Folder = Definition.Folder.IsEmpty() ? TEXT("Quests") : Definition.Folder;
    FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
    FGenerationResult Result;

    if (ValidateAgainstManifest(Definition.Name, TEXT("Quest Blueprint"), Result))
    {
        return Result;
    }

    if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Quest Blueprint"), Definition.ComputeHash(), Result))
    {
        return Result;
    }

    if (Definition.States.Num() == 0)
    {
        return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Quest has no states defined"));
    }

    UPackage* Package = CreatePackage(*AssetPath);
    if (!Package)
    {
        return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
    }

    // Create UQuestBlueprint - QuestTemplate auto-created via CreateDefaultSubobject
    UQuestBlueprint* QuestBP = NewObject<UQuestBlueprint>(Package, *Definition.Name, RF_Public | RF_Standalone | RF_Transactional);
    if (!QuestBP)
    {
        return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Quest Blueprint"));
    }

    QuestBP->ParentClass = UQuest::StaticClass();
    QuestBP->BlueprintType = BPTYPE_Normal;
    QuestBP->bIsNewlyCreated = true;

    UQuest* Quest = QuestBP->QuestTemplate;
    if (!Quest)
    {
        return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("QuestTemplate not created"));
    }

    // Set quest properties
    Quest->SetQuestName(FText::FromString(Definition.QuestName));
    Quest->SetQuestDescription(FText::FromString(Definition.QuestDescription));

    if (FBoolProperty* TrackedProp = CastField<FBoolProperty>(Quest->GetClass()->FindPropertyByName(TEXT("bTracked"))))
    {
        TrackedProp->SetPropertyValue_InContainer(Quest, Definition.bTracked);
    }

    // Maps for wiring
    TMap<FString, UQuestState*> StateMap;

    // ========================================================================
    // FIRST PASS: Create all states
    // ========================================================================
    for (const auto& StateDef : Definition.States)
    {
        UQuestState* State = NewObject<UQuestState>(Quest, UQuestState::StaticClass());
        State->SetFlags(RF_Transactional);
        State->SetID(FName(*StateDef.Id));
        State->Description = FText::FromString(StateDef.Description);

        if (StateDef.Type.Equals(TEXT("success"), ESearchCase::IgnoreCase))
            State->StateNodeType = EStateNodeType::Success;
        else if (StateDef.Type.Equals(TEXT("failure"), ESearchCase::IgnoreCase))
            State->StateNodeType = EStateNodeType::Failure;
        else
            State->StateNodeType = EStateNodeType::Regular;

        // Add events
        for (const auto& EventDef : StateDef.Events)
        {
            if (UNarrativeEvent* Event = CreateDialogueEventFromDefinition(State, EventDef))
            {
                State->Events.Add(Event);
            }
        }

        Quest->AddState(State);
        StateMap.Add(StateDef.Id, State);

        // Set start state
        if (StateDef.Id == Definition.StartState || (Definition.StartState.IsEmpty() && StateMap.Num() == 1))
        {
            Quest->SetQuestStartState(State);
        }

        LogGeneration(FString::Printf(TEXT("  Created quest state: %s (%s)"), *StateDef.Id, *StateDef.Type));
    }

    // ========================================================================
    // SECOND PASS: Create branches and wire connections
    // ========================================================================
    for (const auto& StateDef : Definition.States)
    {
        UQuestState* SourceState = StateMap.FindRef(StateDef.Id);
        if (!SourceState) continue;

        for (const auto& BranchDef : StateDef.Branches)
        {
            UQuestBranch* Branch = NewObject<UQuestBranch>(Quest, UQuestBranch::StaticClass());
            Branch->SetFlags(RF_Transactional);

            if (!BranchDef.Id.IsEmpty())
            {
                Branch->SetID(FName(*BranchDef.Id));
            }

            Branch->bHidden = BranchDef.bHidden;

            // Create tasks
            for (const auto& TaskDef : BranchDef.Tasks)
            {
                if (UNarrativeTask* Task = CreateQuestTask(Branch, TaskDef))
                {
                    Branch->QuestTasks.Add(Task);
                }
            }

            // Add events
            for (const auto& EventDef : BranchDef.Events)
            {
                if (UNarrativeEvent* Event = CreateDialogueEventFromDefinition(Branch, EventDef))
                {
                    Branch->Events.Add(Event);
                }
            }

            Quest->AddBranch(Branch);

            // Wire: State -> Branch
            SourceState->Branches.Add(Branch);

            // Wire: Branch -> Destination State
            if (!BranchDef.DestinationState.IsEmpty())
            {
                if (UQuestState* DestState = StateMap.FindRef(BranchDef.DestinationState))
                {
                    Branch->DestinationState = DestState;
                }
                else
                {
                    UE_LOG(LogGasAbilityGenerator, Warning, TEXT("Quest %s: Destination state not found: %s"),
                        *Definition.Name, *BranchDef.DestinationState);
                }
            }

            LogGeneration(FString::Printf(TEXT("  Created branch: %s -> %s (%d tasks)"),
                *StateDef.Id, *BranchDef.DestinationState, BranchDef.Tasks.Num()));
        }
    }

    // Set QuestDialogue if specified
    if (!Definition.Dialogue.IsEmpty())
    {
        FString DialoguePath = FString::Printf(TEXT("%s/Dialogues/%s.%s_C"),
            *GetProjectRoot(), *Definition.Dialogue, *Definition.Dialogue);
        if (UClass* DialogueClass = LoadClass<UDialogue>(nullptr, *DialoguePath))
        {
            if (FClassProperty* DialogueProp = CastField<FClassProperty>(Quest->GetClass()->FindPropertyByName(TEXT("QuestDialogue"))))
            {
                DialogueProp->SetPropertyValue_InContainer(Quest, DialogueClass);
                LogGeneration(FString::Printf(TEXT("  Set QuestDialogue: %s"), *Definition.Dialogue));
            }
        }
    }

    // Compile
    FKismetEditorUtilities::CompileBlueprint(QuestBP);

    // Save
    Package->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(QuestBP);

    FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, QuestBP, *PackageFileName, SaveArgs);

    LogGeneration(FString::Printf(TEXT("Created Quest Blueprint: %s (%d states, %d branches)"),
        *Definition.Name, Quest->GetStates().Num(), Quest->GetBranches().Num()));

    StoreBlueprintMetadata(QuestBP, TEXT("QuestBlueprint"), Definition.Name, Definition.ComputeHash());

    Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
    Result.DetermineCategory();
    return Result;
}
```

### 6.6 Commandlet (GasAbilityGeneratorCommandlet.cpp)

Add after dialogue_blueprints generation:

```cpp
// Generate Quest Blueprints
for (const auto& Definition : ManifestData.QuestBlueprints)
{
    FGenerationResult Result = FQuestBlueprintGenerator::Generate(Definition, ManifestData.ProjectRoot, &ManifestData);
    ProcessGenerationResult(Result, GeneratedAssets, SkippedAssets, FailedAssets);
}
```

### 6.7 Window (GasAbilityGeneratorWindow.cpp)

Add in generation handler:

```cpp
// Generate Quest Blueprints
for (const auto& Definition : ManifestData.QuestBlueprints)
{
    FGenerationResult Result = FQuestBlueprintGenerator::Generate(Definition, ManifestData.ProjectRoot, &ManifestData);
    // Handle result...
}
```

---

## 7. MANIFEST YAML SCHEMA

```yaml
quest_blueprints:
  - name: QBP_FindAncientRelic           # Asset name (required)
    folder: Quests/MainStory             # Output folder
    quest_name: "Find the Ancient Relic" # Display name
    quest_description: "Search the ruins for the legendary artifact."
    tracked: true                        # Show navigation markers
    dialogue: DBP_RelicQuestDialogue     # Associated dialogue (optional)
    start_state: Start                   # ID of starting state

    states:
      - id: Start                        # Unique state ID (required)
        description: "Begin the search"
        type: regular                    # regular, success, failure
        events:                          # Events fired on state enter
          - type: NE_ShowNotification
            runtime: Start               # Start, End, Both
            properties:
              message: "New Quest Started!"
        branches:                        # Outgoing transitions
          - destination: GatherClues     # Target state ID
            tasks:
              - task: TalkToCharacter    # Task class name
                argument: Archaeologist  # Task argument
                quantity: 1
                optional: false
                hidden: false

      - id: GatherClues
        description: "Gather clues about the location"
        type: regular
        branches:
          - destination: RetrieveRelic
            tasks:
              - task: FindItem
                argument: AncientScroll
                quantity: 3

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
        description: "You found the ancient relic!"
        type: success
        events:
          - type: NE_GiveXP
            runtime: Start
            properties:
              xp_amount: 500

      - id: QuestFailed
        description: "The relic was destroyed"
        type: failure
```

---

## 8. BUILD CONFIGURATION

### GasAbilityGenerator.Build.cs

Add to `PrivateDependencyModuleNames`:

```csharp
PrivateDependencyModuleNames.AddRange(new string[] {
    // ... existing ...
    "NarrativeQuestEditor"  // For UQuestBlueprint
});
```

---

## 9. TESTING CHECKLIST

After implementation:

- [ ] Build compiles without errors
- [ ] YAML parsing works for `quest_blueprints` section
- [ ] Quest with single state/branch generates
- [ ] Quest with multiple states/branches generates
- [ ] State types (regular/success/failure) set correctly
- [ ] Branch wiring (State->Branch->State) correct
- [ ] Tasks created and attached to branches
- [ ] Events attached to states and branches
- [ ] QuestDialogue property set when specified
- [ ] v3.0 metadata stored for regen tracking
- [ ] Quest functions at runtime (start, progress, complete)

---

## SUMMARY

This implementation follows the **exact same pattern** as DialogueBlueprint v3.8:

1. Create `UQuestBlueprint` (QuestTemplate auto-created)
2. Work directly on `QuestTemplate`
3. Create states/branches with Quest as Outer
4. Wire connections via arrays and pointers
5. Create tasks with Branch as Outer
6. Attach events/conditions to nodes
7. Compile blueprint
8. **No graph nodes needed** - quest works at runtime

The visual editor will show an empty graph, but the quest executes correctly because the compiler duplicates `QuestTemplate` into the generated class regardless of graph state.

---

**End of MEGA Handoff - Ready for Implementation**
