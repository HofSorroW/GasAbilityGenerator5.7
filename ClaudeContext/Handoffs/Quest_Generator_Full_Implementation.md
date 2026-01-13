# Quest Generator Full Implementation Handoff

**Terminal 2 Research Complete** | Ready for Terminal 1 Implementation

This document contains ALL code and specifications needed to implement the Quest Generator (QBP_) in GasAbilityGenerator.

---

## KEY INSIGHT: No Graph Nodes Required!

The DialogueBlueprint v3.8 generator works directly on `DialogueTemplate` without creating graph nodes. The same pattern works for Quests:

- `UQuestBlueprint` auto-creates `QuestTemplate` via `CreateDefaultSubobject`
- Work directly on `QuestTemplate` (create states, branches, tasks)
- Compiler duplicates `QuestTemplate` into generated class
- Editor shows empty graph, but quest works at runtime

---

## 1. Add to GasAbilityGeneratorTypes.h

```cpp
// ============================================================================
// QUEST DEFINITIONS (Add after FManifestDialogueBlueprintDefinition)
// ============================================================================

// Quest task definition
USTRUCT()
struct FManifestQuestTaskDefinition
{
    GENERATED_BODY()

    UPROPERTY()
    FString TaskClass;  // Blueprint task class path or name (e.g., "GoToLocation", "FindItem")

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

// Quest branch definition (transition between states)
USTRUCT()
struct FManifestQuestBranchDefinition
{
    GENERATED_BODY()

    UPROPERTY()
    FString Id;  // Optional branch ID

    UPROPERTY()
    FString DestinationState;  // State ID this branch leads to

    UPROPERTY()
    TArray<FManifestQuestTaskDefinition> Tasks;

    UPROPERTY()
    TArray<FManifestDialogueEventDefinition> Events;  // Reuse dialogue event def

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

// Quest state definition
USTRUCT()
struct FManifestQuestStateDefinition
{
    GENERATED_BODY()

    UPROPERTY()
    FString Id;  // Unique state identifier (required)

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

// Main quest blueprint definition
USTRUCT()
struct FManifestQuestBlueprintDefinition
{
    GENERATED_BODY()

    UPROPERTY()
    FString Name;  // Asset name (e.g., "QBP_FindArtifact")

    UPROPERTY()
    FString Folder;

    UPROPERTY()
    FString QuestName;  // Display name

    UPROPERTY()
    FString QuestDescription;

    UPROPERTY()
    bool bTracked = true;  // Show navigation markers

    UPROPERTY()
    FString Dialogue;  // Associated dialogue asset (optional)

    UPROPERTY()
    FString StartState;  // ID of starting state

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

---

## 2. Add to FManifestData in GasAbilityGeneratorTypes.h

```cpp
// In FManifestData struct, add:
UPROPERTY()
TArray<FManifestQuestBlueprintDefinition> QuestBlueprints;
```

---

## 3. Add to GasAbilityGeneratorGenerators.h

```cpp
// ============================================================================
// Quest Blueprint Generator
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

---

## 4. Add to GasAbilityGeneratorParser.cpp

In `ParseManifest()` function, add after dialogue_blueprints parsing:

```cpp
// Parse quest_blueprints
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

                        // Parse branch events (reuse dialogue event parsing)
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

---

## 5. Main Generator Implementation (GasAbilityGeneratorGenerators.cpp)

Add these includes at top:

```cpp
#include "Tales/Quest.h"
#include "Tales/QuestSM.h"
#include "Tales/QuestTask.h"
#include "Tales/QuestBlueprintGeneratedClass.h"
#include "QuestBlueprint.h"  // From NarrativeQuestEditor module
```

Add the generator function:

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

    // v3.0: Check existence with metadata-aware logic
    if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Quest Blueprint"), Definition.ComputeHash(), Result))
    {
        return Result;
    }

    // Validate we have states
    if (Definition.States.Num() == 0)
    {
        return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Quest has no states defined"));
    }

    UPackage* Package = CreatePackage(*AssetPath);
    if (!Package)
    {
        return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
    }

    // Create UQuestBlueprint - QuestTemplate is auto-created via CreateDefaultSubobject
    UQuestBlueprint* QuestBP = NewObject<UQuestBlueprint>(Package, *Definition.Name, RF_Public | RF_Standalone | RF_Transactional);
    if (!QuestBP)
    {
        return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Quest Blueprint"));
    }

    // Setup blueprint properties
    QuestBP->ParentClass = UQuest::StaticClass();
    QuestBP->BlueprintType = BPTYPE_Normal;
    QuestBP->bIsNewlyCreated = true;

    // Get the QuestTemplate (auto-created by UQuestBlueprint constructor)
    UQuest* Quest = QuestBP->QuestTemplate;
    if (!Quest)
    {
        return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("QuestTemplate not created"));
    }

    // Set quest properties
    Quest->SetQuestName(FText::FromString(Definition.QuestName));
    Quest->SetQuestDescription(FText::FromString(Definition.QuestDescription));

    // Set bTracked via reflection
    FBoolProperty* TrackedProp = CastField<FBoolProperty>(Quest->GetClass()->FindPropertyByName(TEXT("bTracked")));
    if (TrackedProp)
    {
        TrackedProp->SetPropertyValue_InContainer(Quest, Definition.bTracked);
    }

    // Maps for wiring connections
    TMap<FString, UQuestState*> StateMap;
    TMap<FString, UQuestBranch*> BranchMap;

    // ========================================================================
    // FIRST PASS: Create all states
    // ========================================================================
    for (const auto& StateDef : Definition.States)
    {
        UQuestState* State = NewObject<UQuestState>(Quest, UQuestState::StaticClass());
        State->SetFlags(RF_Transactional);

        // Set state ID
        State->SetID(FName(*StateDef.Id));

        // Set description
        State->Description = FText::FromString(StateDef.Description);

        // Set state type
        if (StateDef.Type.Equals(TEXT("success"), ESearchCase::IgnoreCase))
        {
            State->StateNodeType = EStateNodeType::Success;
        }
        else if (StateDef.Type.Equals(TEXT("failure"), ESearchCase::IgnoreCase))
        {
            State->StateNodeType = EStateNodeType::Failure;
        }
        else
        {
            State->StateNodeType = EStateNodeType::Regular;
        }

        // Add events to state
        for (const auto& EventDef : StateDef.Events)
        {
            if (UNarrativeEvent* Event = CreateDialogueEventFromDefinition(State, EventDef))
            {
                State->Events.Add(Event);
            }
        }

        // Add state to quest
        Quest->AddState(State);
        StateMap.Add(StateDef.Id, State);

        // Set as start state if matching
        if (StateDef.Id == Definition.StartState ||
            (Definition.StartState.IsEmpty() && StateMap.Num() == 1))
        {
            Quest->SetQuestStartState(State);
        }

        LogGeneration(FString::Printf(TEXT("  Created quest state: %s (%s)"),
            *StateDef.Id, *StateDef.Type));
    }

    // ========================================================================
    // SECOND PASS: Create branches and wire connections
    // ========================================================================
    for (const auto& StateDef : Definition.States)
    {
        UQuestState* SourceState = StateMap.FindRef(StateDef.Id);
        if (!SourceState)
        {
            continue;
        }

        for (const auto& BranchDef : StateDef.Branches)
        {
            UQuestBranch* Branch = NewObject<UQuestBranch>(Quest, UQuestBranch::StaticClass());
            Branch->SetFlags(RF_Transactional);

            // Set branch ID if provided
            if (!BranchDef.Id.IsEmpty())
            {
                Branch->SetID(FName(*BranchDef.Id));
            }

            Branch->bHidden = BranchDef.bHidden;

            // Create tasks for this branch
            for (const auto& TaskDef : BranchDef.Tasks)
            {
                UNarrativeTask* Task = CreateQuestTask(Branch, TaskDef);
                if (Task)
                {
                    Branch->QuestTasks.Add(Task);
                }
            }

            // Add events to branch
            for (const auto& EventDef : BranchDef.Events)
            {
                if (UNarrativeEvent* Event = CreateDialogueEventFromDefinition(Branch, EventDef))
                {
                    Branch->Events.Add(Event);
                }
            }

            // Add branch to quest
            Quest->AddBranch(Branch);

            // Wire: State -> Branch
            SourceState->Branches.Add(Branch);

            // Wire: Branch -> Destination State (deferred, need all states created first)
            if (!BranchDef.DestinationState.IsEmpty())
            {
                if (UQuestState* DestState = StateMap.FindRef(BranchDef.DestinationState))
                {
                    Branch->DestinationState = DestState;
                }
                else
                {
                    UE_LOG(LogGasAbilityGenerator, Warning,
                        TEXT("Quest %s: Branch destination state not found: %s"),
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
        // Try to find dialogue class
        FString DialoguePath = FString::Printf(TEXT("%s/Dialogues/%s.%s_C"),
            *GetProjectRoot(), *Definition.Dialogue, *Definition.Dialogue);
        UClass* DialogueClass = LoadClass<UDialogue>(nullptr, *DialoguePath);

        if (DialogueClass)
        {
            // Set via reflection (QuestDialogue is TSubclassOf<UDialogue>)
            FClassProperty* DialogueProp = CastField<FClassProperty>(
                Quest->GetClass()->FindPropertyByName(TEXT("QuestDialogue")));
            if (DialogueProp)
            {
                DialogueProp->SetPropertyValue_InContainer(Quest, DialogueClass);
                LogGeneration(FString::Printf(TEXT("  Set QuestDialogue: %s"), *Definition.Dialogue));
            }
        }
        else
        {
            UE_LOG(LogGasAbilityGenerator, Warning,
                TEXT("Quest %s: Could not find dialogue class: %s"),
                *Definition.Name, *Definition.Dialogue);
        }
    }

    // Compile the blueprint
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

    // Store metadata for regeneration tracking
    StoreBlueprintMetadata(QuestBP, TEXT("QuestBlueprint"), Definition.Name, Definition.ComputeHash());

    Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
    Result.DetermineCategory();
    return Result;
}

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
        // Project-specific tasks
        FString::Printf(TEXT("/Game/%s/Quests/Tasks/%s.%s_C"), *GetProjectName(), *TaskDef.TaskClass, *TaskDef.TaskClass),
        // Narrative Pro built-in tasks
        FString::Printf(TEXT("/NarrativePro/Core/Quests/Tasks/%s.%s_C"), *TaskDef.TaskClass, *TaskDef.TaskClass),
        FString::Printf(TEXT("/Game/Pro/Core/Quests/Tasks/%s.%s_C"), *TaskDef.TaskClass, *TaskDef.TaskClass),
        // Native class
        FString::Printf(TEXT("/Script/NarrativeArsenal.%s"), *TaskDef.TaskClass)
    };

    UClass* TaskClass = nullptr;
    for (const FString& Path : TaskSearchPaths)
    {
        TaskClass = LoadClass<UNarrativeTask>(nullptr, *Path);
        if (TaskClass)
        {
            break;
        }
    }

    if (!TaskClass)
    {
        UE_LOG(LogGasAbilityGenerator, Warning, TEXT("Could not find quest task class: %s"), *TaskDef.TaskClass);
        return nullptr;
    }

    // Create task instance
    UNarrativeTask* Task = NewObject<UNarrativeTask>(Outer, TaskClass);
    Task->SetFlags(RF_Transactional);

    // Set common properties
    Task->RequiredQuantity = TaskDef.Quantity;
    Task->bOptional = TaskDef.bOptional;
    Task->bHidden = TaskDef.bHidden;

    // Set task-specific argument via reflection (varies by task type)
    // Common pattern: tasks have an "Argument" or specific property like "LocationName", "ItemClass", etc.
    if (!TaskDef.Argument.IsEmpty())
    {
        // Try common argument property names
        TArray<FName> ArgPropNames = {
            TEXT("Argument"),
            TEXT("TaskArgument"),
            TEXT("LocationName"),
            TEXT("ItemName"),
            TEXT("NPCName"),
            TEXT("TargetName")
        };

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

---

## 6. Add to Commandlet (GasAbilityGeneratorCommandlet.cpp)

In the generation loop, add after dialogue_blueprints:

```cpp
// Generate Quest Blueprints
for (const auto& Definition : ManifestData.QuestBlueprints)
{
    FGenerationResult Result = FQuestBlueprintGenerator::Generate(Definition, ManifestData.ProjectRoot, &ManifestData);
    ProcessGenerationResult(Result, GeneratedAssets, SkippedAssets, FailedAssets);
}
```

---

## 7. Add to Window (GasAbilityGeneratorWindow.cpp)

In the generation button handler, add:

```cpp
// Generate Quest Blueprints
for (const auto& Definition : ManifestData.QuestBlueprints)
{
    FGenerationResult Result = FQuestBlueprintGenerator::Generate(Definition, ManifestData.ProjectRoot, &ManifestData);
    // Handle result...
}
```

---

## 8. Build.cs Dependency

Add to `PrivateDependencyModuleNames`:

```csharp
PrivateDependencyModuleNames.Add("NarrativeQuestEditor");
```

---

## 9. Example Manifest YAML

```yaml
quest_blueprints:
  - name: QBP_FindAncientRelic
    folder: Quests/MainStory
    quest_name: "Find the Ancient Relic"
    quest_description: "Search the ruins for the legendary artifact."
    tracked: true
    dialogue: DBP_RelicQuestDialogue
    start_state: Start

    states:
      - id: Start
        description: "Begin the search for the relic"
        type: regular
        events:
          - type: NE_ShowNotification
            runtime: Start
            properties:
              message: "New Quest: Find the Ancient Relic"
        branches:
          - destination: GatherClues
            tasks:
              - task: TalkToCharacter
                argument: Archaeologist
                quantity: 1

      - id: GatherClues
        description: "Gather clues about the relic's location"
        type: regular
        branches:
          - destination: RetrieveRelic
            tasks:
              - task: FindItem
                argument: AncientScroll
                quantity: 3

      - id: RetrieveRelic
        description: "Retrieve the relic from the ruins"
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

## Key Classes Reference

| Class | Location | Purpose |
|-------|----------|---------|
| `UQuestBlueprint` | NarrativeQuestEditor | Blueprint asset, auto-creates QuestTemplate |
| `UQuest` | NarrativeArsenal/Tales/Quest.h | Runtime quest, holds States/Branches |
| `UQuestState` | NarrativeArsenal/Tales/QuestSM.h | Quest milestone/checkpoint |
| `UQuestBranch` | NarrativeArsenal/Tales/QuestSM.h | Transition with tasks |
| `UNarrativeTask` | NarrativeArsenal/Tales/QuestTask.h | Abstract task base class |
| `UQuestBlueprintGeneratedClass` | NarrativeArsenal/Tales/ | Generated class with template |

## Key Methods

```cpp
// UQuest methods
Quest->SetQuestName(FText)
Quest->SetQuestDescription(FText)
Quest->AddState(UQuestState*)
Quest->AddBranch(UQuestBranch*)
Quest->SetQuestStartState(UQuestState*)

// UQuestState
State->SetID(FName)
State->StateNodeType = EStateNodeType::Regular/Success/Failure
State->Description = FText
State->Branches.Add(UQuestBranch*)
State->Events.Add(UNarrativeEvent*)

// UQuestBranch
Branch->SetID(FName)
Branch->DestinationState = UQuestState*
Branch->QuestTasks.Add(UNarrativeTask*)
Branch->Events.Add(UNarrativeEvent*)
Branch->bHidden = bool
```

## Validation Requirements

A valid quest needs:
1. At least one state (`States.Num() > 0`)
2. At least one branch (`Branches.Num() > 0`)
3. A valid start state (`QuestStartState != nullptr`)

---

## Summary

This implementation follows the exact same pattern as DialogueBlueprint v3.8:
1. Create `UQuestBlueprint` (QuestTemplate auto-created)
2. Work directly on `QuestTemplate`
3. Create states/branches with Quest as Outer
4. Wire connections via arrays and pointers
5. Compile blueprint
6. No graph nodes needed - quest works at runtime

The editor will show an empty visual graph, but the quest functions correctly because the compiler duplicates `QuestTemplate` into the generated class.
