# Dialogue Automation Implementation Handoff

**Date:** 2026-01-13
**Purpose:** Complete research and implementation guide for full dialogue tree automation in GasAbilityGenerator
**Target:** Upgrade DialogueBlueprint generator from Medium to High automation level

---

## Executive Summary

The current `FDialogueBlueprintGenerator` creates DialogueBlueprint assets with configuration properties and speakers, but does NOT create the actual dialogue tree (nodes, connections, text, audio). This document provides complete technical research and implementation code to add full dialogue tree generation from YAML.

**Key Finding:** Full automation is achievable by creating both graph nodes (`UDialogueGraphNode_*`) and data nodes (`UDialogueNode_*`) programmatically, matching what the visual editor does.

---

## Part 1: Research Findings

### 1.1 Dialogue System Architecture

Narrative Pro uses a dual-object pattern for dialogues:

```
UDialogueBlueprint
├── DialogueGraph (UEdGraph)           // Visual editor representation
│   ├── UDialogueGraphNode_Root        // Entry point visual node
│   ├── UDialogueGraphNode_NPC[]       // NPC line visual nodes
│   └── UDialogueGraphNode_Player[]    // Player response visual nodes
│
└── DialogueTemplate (UDialogue)       // Runtime data
    ├── RootDialogue                   // First NPC node
    ├── NPCReplies[]                   // All NPC dialogue nodes
    ├── PlayerReplies[]                // All player response nodes
    └── Speakers[]                     // Speaker configuration (in CDO)
```

### 1.2 Key Classes and Locations

| Class | Location | Purpose |
|-------|----------|---------|
| `UDialogueBlueprint` | NarrativeDialogueEditor/Public/DialogueBlueprint.h | Blueprint asset containing graph + template |
| `UDialogue` | NarrativeArsenal/Public/Tales/Dialogue.h | Runtime dialogue data |
| `UDialogueNode_NPC` | NarrativeArsenal/Public/Tales/DialogueSM.h | NPC dialogue line data |
| `UDialogueNode_Player` | NarrativeArsenal/Public/Tales/DialogueSM.h | Player response data |
| `FDialogueLine` | NarrativeArsenal/Public/Tales/DialogueSM.h | Line content (text, audio, montage) |
| `UDialogueGraph` | NarrativeDialogueEditor/Private/DialogueGraph.h | Visual graph container |
| `UDialogueGraphNode_NPC` | NarrativeDialogueEditor/Private/DialogueGraphNode_NPC.h | Visual NPC node |
| `UDialogueGraphNode_Player` | NarrativeDialogueEditor/Private/DialogueGraphNode_Player.h | Visual player node |

### 1.3 Node Creation Pattern (from DialogueGraph.cpp)

The editor creates nodes via `MakeNPCReply()` and `MakePlayerReply()`:

```cpp
// Source: NarrativeDialogueEditor/Private/DialogueGraph.cpp:224-280

UDialogueNode_NPC* UDialogueGraph::MakeNPCReply(UDialogueGraphNode_NPC* Node, UDialogue* Dialogue)
{
    FSoftClassPath NPCReplyClassPath = GetDefault<UDialogueEditorSettings>()->DefaultNPCDialogueClass;
    UClass* NPCReplyClass = (NPCReplyClassPath.IsValid() ?
        LoadObject<UClass>(NULL, *NPCReplyClassPath.ToString()) :
        UDialogueNode_NPC::StaticClass());

    UDialogueNode_NPC* NPCReply = NewObject<UDialogueNode_NPC>(Dialogue, NPCReplyClass);
    NPCReply->SetFlags(RF_Transactional);
    NPCReply->OwningDialogue = Dialogue;
    Node->DialogueNode = NPCReply;
    Dialogue->NPCReplies.AddUnique(NPCReply);
    NPCReply->SetID(*(DialogueAsset->GetName() + "_" + NPCReply->GetName()));

    return NPCReply;
}

UDialogueNode_Player* UDialogueGraph::MakePlayerReply(UDialogueGraphNode_Player* Node, UDialogue* Dialogue)
{
    FSoftClassPath PlayerReplyClassPath = GetDefault<UDialogueEditorSettings>()->DefaultPlayerDialogueClass;
    UClass* PlayerReplyClass = (PlayerReplyClassPath.IsValid() ?
        LoadObject<UClass>(NULL, *PlayerReplyClassPath.ToString()) :
        UDialogueNode_Player::StaticClass());

    UDialogueNode_Player* PlayerReply = NewObject<UDialogueNode_Player>(Dialogue, PlayerReplyClass);
    PlayerReply->SetFlags(RF_Transactional);
    Node->DialogueNode = PlayerReply;
    PlayerReply->OwningDialogue = Dialogue;
    Dialogue->PlayerReplies.AddUnique(PlayerReply);
    PlayerReply->SetID(*(DialogueAsset->GetName() + "_" + PlayerReply->GetName()));

    return PlayerReply;
}
```

### 1.4 Node Connection Pattern (from DialogueGraph.cpp)

Connections are made by adding to `NPCReplies` and `PlayerReplies` arrays:

```cpp
// Source: NarrativeDialogueEditor/Private/DialogueGraph.cpp:102-136

void UDialogueGraph::PinRewired(UDialogueGraphNode* Node, UEdGraphPin* Pin)
{
    if (Pin->Direction == EEdGraphPinDirection::EGPD_Output)
    {
        UDialogueGraphNode* WiredFrom = Cast<UDialogueGraphNode>(Node);
        if (WiredFrom->DialogueNode)
        {
            // Rebuild replies
            WiredFrom->DialogueNode->NPCReplies.Empty();
            WiredFrom->DialogueNode->PlayerReplies.Empty();

            for (auto& LinkedToPin : Pin->LinkedTo)
            {
                if (UDialogueGraphNode_NPC* WiredToNPC = Cast<UDialogueGraphNode_NPC>(LinkedToPin->GetOwningNode()))
                {
                    WiredFrom->DialogueNode->NPCReplies.AddUnique(
                        CastChecked<UDialogueNode_NPC>(WiredToNPC->DialogueNode));
                }
                else if (UDialogueGraphNode_Player* WiredToPlayer = Cast<UDialogueGraphNode_Player>(LinkedToPin->GetOwningNode()))
                {
                    WiredFrom->DialogueNode->PlayerReplies.AddUnique(
                        CastChecked<UDialogueNode_Player>(WiredToPlayer->DialogueNode));
                }
            }
        }
    }
}
```

### 1.5 Dialogue Line Structure (from DialogueSM.h)

```cpp
// Source: NarrativeArsenal/Public/Tales/DialogueSM.h:50-117

USTRUCT(BlueprintType)
struct FDialogueLine
{
    GENERATED_BODY()

    // The dialogue text
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line", meta = (MultiLine = true))
    FText Text;

    // Duration type (Default, WhenAudioEnds, WhenSequenceEnds, AfterReadingTime, AfterDuration, Never)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line")
    ELineDuration Duration;

    // Override duration in seconds (when Duration == LD_AfterDuration)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line")
    float DurationSecondsOverride;

    // Audio to play
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line")
    TObjectPtr<class USoundBase> DialogueSound;

    // Body animation montage
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line")
    TObjectPtr<class UAnimMontage> DialogueMontage;

    // Facial animation montage
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line")
    TObjectPtr<class UAnimMontage> FacialAnimation;

    // Camera shot for this line
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Dialogue Line")
    TObjectPtr<class UNarrativeDialogueSequence> Shot;

    // Conditions for this line to be selected
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Dialogue Line")
    TArray<TObjectPtr<class UNarrativeCondition>> Conditions;
};
```

### 1.6 Duration Enum

```cpp
// Source: NarrativeArsenal/Public/Tales/DialogueSM.h:30-48

UENUM(BlueprintType)
enum class ELineDuration : uint8
{
    LD_Default UMETA(DisplayName="Default"),
    LD_WhenAudioEnds UMETA(DisplayName = "When Audio Ends"),
    LD_WhenSequenceEnds UMETA(DisplayName = "When Sequence Ends"),
    LD_AfterReadingTime UMETA(DisplayName = "After Reading Time"),
    LD_AfterDuration UMETA(DisplayName = "After X Seconds"),
    LD_Never UMETA(DisplayName = "Never")
};
```

### 1.7 Graph Node Creation (from DialogueGraphEditor.cpp)

The editor creates graph nodes and triggers NodeAdded:

```cpp
// Source: NarrativeDialogueEditor/Private/DialogueGraphEditor.cpp:459

Node = NewObject<UDialogueGraphNode>(DialogueGraph, DialogueNodeClass);

// Then via FDialogueSchemaAction_NewNode::PerformAction():
NodeTemplate->Rename(NULL, ParentGraph, REN_NonTransactional);
ParentGraph->AddNode(NodeTemplate, true);
NodeTemplate->CreateNewGuid();
NodeTemplate->PostPlacedNewNode();
NodeTemplate->AllocateDefaultPins();
NodeTemplate->AutowireNewNode(FromPin);
```

### 1.8 Setting Line Content (from DialogueGraphEditor.cpp)

```cpp
// Source: NarrativeDialogueEditor/Private/DialogueGraphEditor.cpp:500-501

Node->DialogueNode->Line.Text = FText::FromString(LineText);
Node->DialogueNode->GenerateIDFromText();

// Source: DialogueGraphEditor.cpp:1225-1240
NewGraphNode->DialogueNode->Line.DialogueSound = SoundWave;

if (UDialogueNode_NPC* NPCNode = Cast<UDialogueNode_NPC>(NewGraphNode->DialogueNode))
{
    NPCNode->SetSpeakerID(SpeakerID);
}
```

---

## Part 2: Proposed YAML Schema

### 2.1 Extended Manifest Definition

Add to `GasAbilityGeneratorTypes.h`:

```cpp
/**
 * Dialogue node definition for YAML
 */
struct FManifestDialogueNodeDefinition
{
    FString Id;                          // Unique node identifier
    FString Type;                        // "npc" or "player"
    FString Speaker;                     // Speaker ID (for NPC nodes)
    FString Text;                        // Dialogue line text
    FString OptionText;                  // Short option text (player nodes)
    FString Audio;                       // Audio asset path
    FString Montage;                     // Animation montage path
    FString Duration;                    // "Default", "WhenAudioEnds", "AfterDuration", etc.
    float DurationSeconds = 0.0f;        // Duration override
    bool bAutoSelect = false;            // Auto-select this option
    bool bAutoSelectIfOnly = true;       // Auto-select if only option
    bool bIsSkippable = true;            // Can skip this line
    FString DirectedAt;                  // Speaker ID this line is directed at
    TArray<FString> NPCReplies;          // IDs of NPC nodes that follow
    TArray<FString> PlayerReplies;       // IDs of player nodes that follow
    TArray<FManifestDialogueLineDefinition> AlternativeLines;  // Random alternatives

    uint64 ComputeHash() const
    {
        uint64 Hash = GetTypeHash(Id);
        Hash ^= GetTypeHash(Type) << 4;
        Hash ^= GetTypeHash(Speaker) << 8;
        Hash ^= GetTypeHash(Text) << 12;
        Hash ^= GetTypeHash(Audio) << 16;
        for (const auto& Reply : NPCReplies) Hash ^= GetTypeHash(Reply);
        for (const auto& Reply : PlayerReplies) Hash ^= GetTypeHash(Reply);
        return Hash;
    }
};

/**
 * Alternative dialogue line (for variety)
 */
struct FManifestDialogueLineDefinition
{
    FString Text;
    FString Audio;
    FString Montage;
};

/**
 * Dialogue tree definition
 */
struct FManifestDialogueTreeDefinition
{
    FString RootNodeId;                  // ID of the starting node
    TArray<FManifestDialogueNodeDefinition> Nodes;

    uint64 ComputeHash() const
    {
        uint64 Hash = GetTypeHash(RootNodeId);
        for (const auto& Node : Nodes)
        {
            Hash ^= Node.ComputeHash();
            Hash = (Hash << 3) | (Hash >> 61);
        }
        return Hash;
    }
};
```

### 2.2 Updated DialogueBlueprint Definition

Modify `FManifestDialogueBlueprintDefinition` in `GasAbilityGeneratorTypes.h`:

```cpp
struct FManifestDialogueBlueprintDefinition
{
    FString Name;
    FString ParentClass = TEXT("NarrativeDialogue");
    FString Folder;
    TArray<FManifestActorVariableDefinition> Variables;
    FString EventGraphName;

    // v3.2: Dialogue configuration properties
    bool bFreeMovement = true;
    bool bUnskippable = false;
    bool bCanBeExited = true;
    bool bShowCinematicBars = false;
    bool bAutoRotateSpeakers = true;
    bool bAutoStopMovement = false;
    int32 Priority = 0;
    float EndDialogueDist = 0.0f;

    // v3.5: Additional UDialogue properties
    FString DefaultHeadBoneName;
    float DialogueBlendOutTime = 0.5f;
    bool bAdjustPlayerTransform = false;

    // v3.2: Speakers configuration
    TArray<FManifestDialogueSpeakerDefinition> Speakers;

    // v3.7 NEW: Full dialogue tree
    FManifestDialogueTreeDefinition DialogueTree;

    uint64 ComputeHash() const
    {
        uint64 Hash = GetTypeHash(Name);
        Hash ^= GetTypeHash(ParentClass) << 4;
        // ... existing hash code ...
        Hash ^= DialogueTree.ComputeHash() << 32;  // Add tree to hash
        return Hash;
    }
};
```

### 2.3 Example YAML

```yaml
dialogue_blueprints:
  - name: DBP_SethGreeting
    folder: Dialogues/NPCs/Seth
    parent_class: NarrativeDialogue
    free_movement: true
    auto_rotate_speakers: true

    speakers:
      - npc_definition: NPCDef_Seth
        node_color: "#FF6600"

    dialogue_tree:
      root: greeting_1

      nodes:
        # Opening NPC line
        - id: greeting_1
          type: npc
          speaker: Seth
          text: "Hey you! Over here, traveler!"
          audio: /Game/Audio/Dialogue/Seth/Seth_HeyYou
          duration: WhenAudioEnds
          player_replies:
            - response_hello
            - response_busy
            - response_ignore

        # Player responses
        - id: response_hello
          type: player
          text: "Hello there! Nice to meet you."
          option_text: "[Friendly] Hello!"
          npc_replies:
            - intro_friendly

        - id: response_busy
          type: player
          text: "I'm kind of busy right now..."
          option_text: "[Busy] Not now"
          npc_replies:
            - intro_busy

        - id: response_ignore
          type: player
          text: "..."
          option_text: "[Ignore]"
          auto_select_if_only: true
          npc_replies:
            - intro_ignored

        # NPC follow-ups based on player choice
        - id: intro_friendly
          type: npc
          speaker: Seth
          text: "I'm Seth! Welcome to the Narrative Pro demo level."
          audio: /Game/Audio/Dialogue/Seth/Seth_ImSeth
          alternative_lines:
            - text: "The name's Seth. Glad you stopped by!"
              audio: /Game/Audio/Dialogue/Seth/Seth_NiceToMeetYou
          player_replies:
            - ask_about_area
            - say_goodbye

        - id: intro_busy
          type: npc
          speaker: Seth
          text: "Oh, alright then. Come back when you have time!"
          audio: /Game/Audio/Dialogue/Seth/Seth_AlrightThis
          # No replies = dialogue ends

        - id: intro_ignored
          type: npc
          speaker: Seth
          text: "Hmm, not very friendly are you..."
          audio: /Game/Audio/Dialogue/Seth/Seth_Enough

        # Deeper conversation
        - id: ask_about_area
          type: player
          text: "What is this place?"
          option_text: "[Ask] About this area"
          npc_replies:
            - explain_area

        - id: say_goodbye
          type: player
          text: "I should get going."
          option_text: "[Leave]"
          npc_replies:
            - farewell

        - id: explain_area
          type: npc
          speaker: Seth
          text: "This is the demo level. It showcases all of Narrative Pro's features!"
          audio: /Game/Audio/Dialogue/Seth/Seth_TheDemoLevel
          player_replies:
            - say_goodbye

        - id: farewell
          type: npc
          speaker: Seth
          text: "See you around, traveler!"
          audio: /Game/Audio/Dialogue/Seth/Seth_AlrightLets
```

---

## Part 3: Implementation Code

### 3.1 Required Includes

Add to `GasAbilityGeneratorGenerators.cpp`:

```cpp
// Dialogue editor classes (for graph creation)
#include "NarrativeDialogueEditor/Private/DialogueGraph.h"
#include "NarrativeDialogueEditor/Private/DialogueGraphNode.h"
#include "NarrativeDialogueEditor/Private/DialogueGraphNode_NPC.h"
#include "NarrativeDialogueEditor/Private/DialogueGraphNode_Player.h"
#include "NarrativeDialogueEditor/Private/DialogueGraphNode_Root.h"
#include "NarrativeDialogueEditor/Private/DialogueGraphSchema.h"
#include "NarrativeDialogueEditor/Public/DialogueBlueprint.h"

// Dialogue runtime classes
#include "Tales/Dialogue.h"
#include "Tales/DialogueSM.h"
```

### 3.2 Build.cs Update

Add to `GasAbilityGenerator.Build.cs` PrivateDependencyModuleNames:

```csharp
// Dialogue blueprint generation
"NarrativeDialogueEditor",
```

### 3.3 Parser Update

Add to `GasAbilityGeneratorParser.cpp` in `ParseDialogueBlueprints()`:

```cpp
// Parse dialogue tree if present
if (DialogueNode["dialogue_tree"])
{
    YAML::Node TreeNode = DialogueNode["dialogue_tree"];

    if (TreeNode["root"])
    {
        Definition.DialogueTree.RootNodeId = UTF8_TO_TCHAR(TreeNode["root"].as<std::string>().c_str());
    }

    if (TreeNode["nodes"])
    {
        for (const auto& NodeEntry : TreeNode["nodes"])
        {
            FManifestDialogueNodeDefinition NodeDef;

            if (NodeEntry["id"])
                NodeDef.Id = UTF8_TO_TCHAR(NodeEntry["id"].as<std::string>().c_str());
            if (NodeEntry["type"])
                NodeDef.Type = UTF8_TO_TCHAR(NodeEntry["type"].as<std::string>().c_str());
            if (NodeEntry["speaker"])
                NodeDef.Speaker = UTF8_TO_TCHAR(NodeEntry["speaker"].as<std::string>().c_str());
            if (NodeEntry["text"])
                NodeDef.Text = UTF8_TO_TCHAR(NodeEntry["text"].as<std::string>().c_str());
            if (NodeEntry["option_text"])
                NodeDef.OptionText = UTF8_TO_TCHAR(NodeEntry["option_text"].as<std::string>().c_str());
            if (NodeEntry["audio"])
                NodeDef.Audio = UTF8_TO_TCHAR(NodeEntry["audio"].as<std::string>().c_str());
            if (NodeEntry["montage"])
                NodeDef.Montage = UTF8_TO_TCHAR(NodeEntry["montage"].as<std::string>().c_str());
            if (NodeEntry["duration"])
                NodeDef.Duration = UTF8_TO_TCHAR(NodeEntry["duration"].as<std::string>().c_str());
            if (NodeEntry["duration_seconds"])
                NodeDef.DurationSeconds = NodeEntry["duration_seconds"].as<float>();
            if (NodeEntry["auto_select"])
                NodeDef.bAutoSelect = NodeEntry["auto_select"].as<bool>();
            if (NodeEntry["auto_select_if_only"])
                NodeDef.bAutoSelectIfOnly = NodeEntry["auto_select_if_only"].as<bool>();
            if (NodeEntry["skippable"])
                NodeDef.bIsSkippable = NodeEntry["skippable"].as<bool>();
            if (NodeEntry["directed_at"])
                NodeDef.DirectedAt = UTF8_TO_TCHAR(NodeEntry["directed_at"].as<std::string>().c_str());

            // Parse reply arrays
            if (NodeEntry["npc_replies"])
            {
                for (const auto& Reply : NodeEntry["npc_replies"])
                {
                    NodeDef.NPCReplies.Add(UTF8_TO_TCHAR(Reply.as<std::string>().c_str()));
                }
            }
            if (NodeEntry["player_replies"])
            {
                for (const auto& Reply : NodeEntry["player_replies"])
                {
                    NodeDef.PlayerReplies.Add(UTF8_TO_TCHAR(Reply.as<std::string>().c_str()));
                }
            }

            // Parse alternative lines
            if (NodeEntry["alternative_lines"])
            {
                for (const auto& AltLine : NodeEntry["alternative_lines"])
                {
                    FManifestDialogueLineDefinition LineDef;
                    if (AltLine["text"])
                        LineDef.Text = UTF8_TO_TCHAR(AltLine["text"].as<std::string>().c_str());
                    if (AltLine["audio"])
                        LineDef.Audio = UTF8_TO_TCHAR(AltLine["audio"].as<std::string>().c_str());
                    if (AltLine["montage"])
                        LineDef.Montage = UTF8_TO_TCHAR(AltLine["montage"].as<std::string>().c_str());
                    NodeDef.AlternativeLines.Add(LineDef);
                }
            }

            Definition.DialogueTree.Nodes.Add(NodeDef);
        }
    }
}
```

### 3.4 Generator Implementation

Add helper function and update `FDialogueBlueprintGenerator::Generate()`:

```cpp
// Helper: Convert duration string to enum
static ELineDuration ParseDuration(const FString& DurationStr)
{
    if (DurationStr.Equals(TEXT("WhenAudioEnds"), ESearchCase::IgnoreCase))
        return ELineDuration::LD_WhenAudioEnds;
    if (DurationStr.Equals(TEXT("WhenSequenceEnds"), ESearchCase::IgnoreCase))
        return ELineDuration::LD_WhenSequenceEnds;
    if (DurationStr.Equals(TEXT("AfterReadingTime"), ESearchCase::IgnoreCase))
        return ELineDuration::LD_AfterReadingTime;
    if (DurationStr.Equals(TEXT("AfterDuration"), ESearchCase::IgnoreCase))
        return ELineDuration::LD_AfterDuration;
    if (DurationStr.Equals(TEXT("Never"), ESearchCase::IgnoreCase))
        return ELineDuration::LD_Never;
    return ELineDuration::LD_Default;
}

// Helper: Create dialogue tree from manifest
static void CreateDialogueTree(
    UDialogueBlueprint* DialogueBP,
    const FManifestDialogueTreeDefinition& TreeDef)
{
    if (TreeDef.Nodes.Num() == 0)
    {
        return;  // No tree to create
    }

    UDialogue* Dialogue = DialogueBP->DialogueTemplate;
    UDialogueGraph* DialogueGraph = Cast<UDialogueGraph>(DialogueBP->DialogueGraph);

    if (!Dialogue || !DialogueGraph)
    {
        UE_LOG(LogGasAbilityGenerator, Warning, TEXT("Missing Dialogue or DialogueGraph"));
        return;
    }

    // Maps for node lookup
    TMap<FString, UDialogueNode_NPC*> NPCNodeMap;
    TMap<FString, UDialogueNode_Player*> PlayerNodeMap;
    TMap<FString, UDialogueGraphNode*> GraphNodeMap;

    // Layout positioning
    float CurrentX = 400.0f;
    float CurrentY = 0.0f;
    const float NodeSpacingX = 500.0f;
    const float NodeSpacingY = 200.0f;

    // First pass: Create all nodes
    for (const auto& NodeDef : TreeDef.Nodes)
    {
        UDialogueGraphNode* GraphNode = nullptr;
        UDialogueNode* DialogueNode = nullptr;

        if (NodeDef.Type.Equals(TEXT("npc"), ESearchCase::IgnoreCase))
        {
            // Create NPC graph node
            UDialogueGraphNode_NPC* NPCGraphNode = NewObject<UDialogueGraphNode_NPC>(DialogueGraph);
            NPCGraphNode->SetFlags(RF_Transactional);
            NPCGraphNode->Rename(nullptr, DialogueGraph, REN_NonTransactional);

            // Create NPC dialogue node
            UDialogueNode_NPC* NPCDialogueNode = NewObject<UDialogueNode_NPC>(Dialogue);
            NPCDialogueNode->SetFlags(RF_Transactional);
            NPCDialogueNode->OwningDialogue = Dialogue;
            NPCDialogueNode->SetSpeakerID(FName(*NodeDef.Speaker));

            // Link them
            NPCGraphNode->DialogueNode = NPCDialogueNode;
            Dialogue->NPCReplies.AddUnique(NPCDialogueNode);

            GraphNode = NPCGraphNode;
            DialogueNode = NPCDialogueNode;
            NPCNodeMap.Add(NodeDef.Id, NPCDialogueNode);

            // Set as root if this is the root node
            if (NodeDef.Id == TreeDef.RootNodeId)
            {
                Dialogue->RootDialogue = NPCDialogueNode;
            }
        }
        else if (NodeDef.Type.Equals(TEXT("player"), ESearchCase::IgnoreCase))
        {
            // Create Player graph node
            UDialogueGraphNode_Player* PlayerGraphNode = NewObject<UDialogueGraphNode_Player>(DialogueGraph);
            PlayerGraphNode->SetFlags(RF_Transactional);
            PlayerGraphNode->Rename(nullptr, DialogueGraph, REN_NonTransactional);

            // Create Player dialogue node
            UDialogueNode_Player* PlayerDialogueNode = NewObject<UDialogueNode_Player>(Dialogue);
            PlayerDialogueNode->SetFlags(RF_Transactional);
            PlayerDialogueNode->OwningDialogue = Dialogue;

            // Link them
            PlayerGraphNode->DialogueNode = PlayerDialogueNode;
            Dialogue->PlayerReplies.AddUnique(PlayerDialogueNode);

            GraphNode = PlayerGraphNode;
            DialogueNode = PlayerDialogueNode;
            PlayerNodeMap.Add(NodeDef.Id, PlayerDialogueNode);
        }

        if (!GraphNode || !DialogueNode)
        {
            UE_LOG(LogGasAbilityGenerator, Warning, TEXT("Failed to create node: %s"), *NodeDef.Id);
            continue;
        }

        // Add to graph
        DialogueGraph->AddNode(GraphNode, true);
        GraphNode->CreateNewGuid();
        GraphNode->AllocateDefaultPins();

        // Position node
        GraphNode->NodePosX = CurrentX;
        GraphNode->NodePosY = CurrentY;
        CurrentY += NodeSpacingY;
        if (CurrentY > 1000.0f)
        {
            CurrentY = 0.0f;
            CurrentX += NodeSpacingX;
        }

        // Set dialogue line content
        DialogueNode->Line.Text = FText::FromString(NodeDef.Text);
        DialogueNode->Line.Duration = ParseDuration(NodeDef.Duration);
        DialogueNode->Line.DurationSecondsOverride = NodeDef.DurationSeconds;
        DialogueNode->bIsSkippable = NodeDef.bIsSkippable;

        if (!NodeDef.DirectedAt.IsEmpty())
        {
            DialogueNode->DirectedAtSpeakerID = FName(*NodeDef.DirectedAt);
        }

        // Load audio if specified
        if (!NodeDef.Audio.IsEmpty())
        {
            USoundBase* Sound = LoadObject<USoundBase>(nullptr, *NodeDef.Audio);
            if (Sound)
            {
                DialogueNode->Line.DialogueSound = Sound;
            }
            else
            {
                UE_LOG(LogGasAbilityGenerator, Warning, TEXT("Could not load audio: %s"), *NodeDef.Audio);
            }
        }

        // Load montage if specified
        if (!NodeDef.Montage.IsEmpty())
        {
            UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, *NodeDef.Montage);
            if (Montage)
            {
                DialogueNode->Line.DialogueMontage = Montage;
            }
        }

        // Add alternative lines
        for (const auto& AltLineDef : NodeDef.AlternativeLines)
        {
            FDialogueLine AltLine;
            AltLine.Text = FText::FromString(AltLineDef.Text);
            AltLine.Duration = ELineDuration::LD_Default;

            if (!AltLineDef.Audio.IsEmpty())
            {
                AltLine.DialogueSound = LoadObject<USoundBase>(nullptr, *AltLineDef.Audio);
            }
            if (!AltLineDef.Montage.IsEmpty())
            {
                AltLine.DialogueMontage = LoadObject<UAnimMontage>(nullptr, *AltLineDef.Montage);
            }

            DialogueNode->AlternativeLines.Add(AltLine);
        }

        // Set player-specific properties
        if (UDialogueNode_Player* PlayerNode = Cast<UDialogueNode_Player>(DialogueNode))
        {
            // Note: OptionText and bAutoSelect are protected, need reflection or accessor
            // For now, use reflection:
            if (!NodeDef.OptionText.IsEmpty())
            {
                FTextProperty* OptionTextProp = CastField<FTextProperty>(
                    PlayerNode->GetClass()->FindPropertyByName(TEXT("OptionText")));
                if (OptionTextProp)
                {
                    OptionTextProp->SetPropertyValue_InContainer(PlayerNode, FText::FromString(NodeDef.OptionText));
                }
            }

            FBoolProperty* AutoSelectProp = CastField<FBoolProperty>(
                PlayerNode->GetClass()->FindPropertyByName(TEXT("bAutoSelect")));
            if (AutoSelectProp)
            {
                AutoSelectProp->SetPropertyValue_InContainer(PlayerNode, NodeDef.bAutoSelect);
            }

            FBoolProperty* AutoSelectIfOnlyProp = CastField<FBoolProperty>(
                PlayerNode->GetClass()->FindPropertyByName(TEXT("bAutoSelectIfOnlyReply")));
            if (AutoSelectIfOnlyProp)
            {
                AutoSelectIfOnlyProp->SetPropertyValue_InContainer(PlayerNode, NodeDef.bAutoSelectIfOnly);
            }
        }

        // Generate unique ID
        DialogueNode->SetID(FName(*(DialogueBP->GetName() + TEXT("_") + NodeDef.Id)));

        GraphNodeMap.Add(NodeDef.Id, GraphNode);

        UE_LOG(LogGasAbilityGenerator, Log, TEXT("  Created dialogue node: %s (%s)"),
            *NodeDef.Id, *NodeDef.Type);
    }

    // Second pass: Wire connections
    for (const auto& NodeDef : TreeDef.Nodes)
    {
        UDialogueNode* SourceNode = nullptr;

        if (NPCNodeMap.Contains(NodeDef.Id))
        {
            SourceNode = NPCNodeMap[NodeDef.Id];
        }
        else if (PlayerNodeMap.Contains(NodeDef.Id))
        {
            SourceNode = PlayerNodeMap[NodeDef.Id];
        }

        if (!SourceNode)
        {
            continue;
        }

        // Add NPC replies
        for (const FString& ReplyId : NodeDef.NPCReplies)
        {
            if (UDialogueNode_NPC** FoundNode = NPCNodeMap.Find(ReplyId))
            {
                SourceNode->NPCReplies.AddUnique(*FoundNode);
            }
            else
            {
                UE_LOG(LogGasAbilityGenerator, Warning,
                    TEXT("Could not find NPC node for reply: %s"), *ReplyId);
            }
        }

        // Add Player replies
        for (const FString& ReplyId : NodeDef.PlayerReplies)
        {
            if (UDialogueNode_Player** FoundNode = PlayerNodeMap.Find(ReplyId))
            {
                SourceNode->PlayerReplies.AddUnique(*FoundNode);
            }
            else
            {
                UE_LOG(LogGasAbilityGenerator, Warning,
                    TEXT("Could not find Player node for reply: %s"), *ReplyId);
            }
        }
    }

    UE_LOG(LogGasAbilityGenerator, Log, TEXT("  Created %d NPC nodes, %d Player nodes"),
        NPCNodeMap.Num(), PlayerNodeMap.Num());
}
```

### 3.5 Integrate into Generator

Update `FDialogueBlueprintGenerator::Generate()` to call the tree creation:

```cpp
// After blueprint creation and configuration (around line 7280):

// v3.7: Create dialogue tree if defined
if (Definition.DialogueTree.Nodes.Num() > 0)
{
    UDialogueBlueprint* DialogueBP = Cast<UDialogueBlueprint>(Blueprint);
    if (DialogueBP)
    {
        CreateDialogueTree(DialogueBP, Definition.DialogueTree);
        LogGeneration(FString::Printf(TEXT("  Created dialogue tree with %d nodes"),
            Definition.DialogueTree.Nodes.Num()));
    }
}

// Recompile blueprint
FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
FKismetEditorUtilities::CompileBlueprint(Blueprint);
```

---

## Part 4: Testing Plan

### 4.1 Basic Test Case

1. Add simple 3-node dialogue to manifest:
```yaml
dialogue_blueprints:
  - name: DBP_TestDialogue
    folder: Test
    speakers:
      - npc_definition: NPCDef_TestNPC
    dialogue_tree:
      root: greeting
      nodes:
        - id: greeting
          type: npc
          speaker: TestNPC
          text: "Hello!"
          player_replies: [response]
        - id: response
          type: player
          text: "Hi there!"
          npc_replies: [farewell]
        - id: farewell
          type: npc
          speaker: TestNPC
          text: "Goodbye!"
```

2. Run generator
3. Open DBP_TestDialogue in editor
4. Verify:
   - 3 nodes visible in graph
   - Connections correct
   - Text appears in nodes
   - Can play dialogue in-game

### 4.2 Advanced Test Cases

- Branching dialogue (multiple player responses)
- Alternative lines
- Audio references
- Animation montages
- Auto-select options

---

## Part 5: Version Notes

### v3.7 Changelog

```
- v3.7 - Dialogue Tree Generation: FDialogueBlueprintGenerator now supports full
  dialogue tree creation from YAML. New manifest properties: dialogue_tree with
  nodes array supporting npc/player types, text, audio, montage, duration,
  alternative_lines, and connection arrays (npc_replies/player_replies). Creates
  both graph nodes (UDialogueGraphNode_*) and data nodes (UDialogueNode_*) for
  full editor compatibility. Upgrades DBP_ from Medium to High automation level.
```

---

## Part 6: File Locations Summary

| File | Changes |
|------|---------|
| `GasAbilityGeneratorTypes.h` | Add `FManifestDialogueNodeDefinition`, `FManifestDialogueLineDefinition`, `FManifestDialogueTreeDefinition` structs; update `FManifestDialogueBlueprintDefinition` |
| `GasAbilityGeneratorParser.cpp` | Add dialogue tree parsing in `ParseDialogueBlueprints()` |
| `GasAbilityGeneratorGenerators.cpp` | Add `CreateDialogueTree()` helper, update `FDialogueBlueprintGenerator::Generate()` |
| `GasAbilityGenerator.Build.cs` | Add `NarrativeDialogueEditor` to PrivateDependencyModuleNames |
| `CLAUDE.md` | Update version history and supported features |

---

## Part 7: Potential Issues and Mitigations

### 7.1 Private Headers

The dialogue graph classes are in `Private/` folders. Solutions:
- Use `#include "NarrativeDialogueEditor/Private/..."` (works if in same solution)
- Or create nodes without graph (Option A fallback)
- Or add friend declarations in Narrative Pro

### 7.2 Protected Members

`UDialogueNode_Player::OptionText` and `bAutoSelect` are protected. Solutions:
- Use reflection (shown in code above)
- Or request accessor methods from Narrative Pro maintainer

### 7.3 Graph Node Positioning

Current implementation uses simple linear layout. Future enhancement:
- Implement tree layout algorithm
- Or use Narrative Pro's vertical/horizontal wiring settings

---

## Part 8: Events, Conditions, Triggers & Goals System

This section covers the advanced dialogue automation: events that fire during dialogue, conditions that control node visibility, triggers for NPC behaviors, and goals for AI activities.

### 8.1 System Overview

```
UDialogueNode (NPC or Player)
├── Conditions[]              // UNarrativeCondition* - controls if node appears
├── Events[]                  // UNarrativeEvent* - fires when node is reached
└── Line
    └── Conditions[]          // Per-line conditions for alternative selection

UNarrativeTrigger             // Time/event-based activation
└── TriggerEvents[]           // UNarrativeEvent* - fires on activate/deactivate

UTriggerSet (DataAsset)       // Container for triggers
└── Triggers[]                // UNarrativeTrigger* instances

UNPCGoalItem                  // AI goal to accomplish
└── (Activity configuration)
```

### 8.2 UNarrativeEvent - Events on Dialogue Nodes

**Location:** `NarrativeArsenal/Public/Tales/NarrativeEvent.h`

Events fire when dialogue nodes are reached (start or end of line).

```cpp
UCLASS(Abstract, Blueprintable, EditInlineNew)
class UNarrativeEvent : public UObject
{
    // When to fire: Start, End, or Both
    UPROPERTY(EditAnywhere)
    EEventRuntime EventRuntime;

    // Who to run on: Anyone, OnlyNPCs, OnlyPlayers
    UPROPERTY(EditAnywhere)
    EEventFilter EventFilter;

    // Party handling
    UPROPERTY(EditAnywhere)
    EPartyEventPolicy PartyEventPolicy;

    // Conditions that must pass for event to fire
    UPROPERTY(EditAnywhere, Instanced)
    TArray<UNarrativeCondition*> Conditions;

    // Override to implement event logic
    virtual void ExecuteEvent(APawn* Target, APlayerController* Controller, UTalesComponent* NarrativeComponent);
};
```

**Built-in Events (Blueprint assets in Pro/Core/Tales/Events/):**

| Asset | Purpose |
|-------|---------|
| `NE_BeginQuest` | Start a quest |
| `NE_BeginDialogue` | Start another dialogue |
| `NE_GiveXP` | Award experience points |
| `NE_AddGameplayTags` | Add tags to character |
| `NE_RemoveGameplayTags` | Remove tags from character |
| `NE_AddFactions` | Add faction membership |
| `NE_RemoveFactions` | Remove faction membership |
| `NE_SetFactionAttitude` | Change faction relationship |
| `NE_ShowNotification` | Display UI notification |
| `NE_AddSaveCheckpoint` | Create save point |
| `NE_RestartQuest` | Restart a quest |
| `NE_PrintString` | Debug output |

**C++ Events:**

| Class | Location | Purpose |
|-------|----------|---------|
| `UNarrativeEvent_AddGoalToNPC` | AI/Activities/ | Add AI goal to NPC |
| `UNarrativeEvent_AddGoalMulti` | AI/Activities/ | Add goals to multiple NPCs |

### 8.3 UNarrativeCondition - Conditional Node Visibility

**Location:** `NarrativeArsenal/Public/Tales/NarrativeCondition.h`

Conditions determine if a dialogue node appears as an option.

```cpp
UCLASS(Abstract, Blueprintable, EditInlineNew)
class UNarrativeCondition : public UObject
{
    // Invert the result
    UPROPERTY(EditAnywhere)
    bool bNot = false;

    // Who to check: DontTarget, Anyone, OnlyNPCs, OnlyPlayers
    UPROPERTY(EditAnywhere)
    EConditionFilter ConditionFilter;

    // Override to implement condition check
    virtual bool CheckCondition(APawn* Target, APlayerController* Controller, UTalesComponent* NarrativeComponent);
};
```

**Built-in Conditions (Blueprint assets in Pro/Core/Tales/Conditions/):**

| Asset | Purpose |
|-------|---------|
| `NC_IsQuestInProgress` | Check if quest is active |
| `NC_IsQuestAtState` | Check quest is at specific state |
| `NC_IsQuestSucceeded` | Check if quest completed successfully |
| `NC_IsQuestFailed` | Check if quest failed |
| `NC_IsQuestStartedOrFinished` | Check quest has been started |
| `NC_HasCompletedDataTask` | Check if data task done |
| `NC_HasDialogueNodePlayed` | Check if dialogue node was reached before |
| `NC_IsDayTime` | Check if daytime |
| `NC_IsTimeInRange` | Check if time in range |

### 8.4 How Events/Conditions Are Processed

From `NarrativeNodeBase.cpp`:

```cpp
// Events fire at Start or End of node
void UNarrativeNodeBase::ProcessEvents(APawn* Pawn, APlayerController* Controller,
    UTalesComponent* NarrativeComponent, const EEventRuntime Runtime)
{
    for (auto& Event : Events)
    {
        if (Event)
        {
            const bool bShouldFire = (Event->EventRuntime == Runtime || Event->EventRuntime == EEventRuntime::Both);
            if (bShouldFire)
            {
                Event->ExecuteEvent(Pawn, Controller, NarrativeComponent);
            }
        }
    }
}

// Conditions checked before showing node
bool UNarrativeNodeBase::AreConditionsMet(APawn* Pawn, APlayerController* Controller,
    UTalesComponent* NarrativeComponent)
{
    for (auto& Cond : Conditions)
    {
        if (Cond)
        {
            if (Cond->CheckCondition(Pawn, Controller, NarrativeComponent) == Cond->bNot)
            {
                return false;  // Condition failed
            }
        }
    }
    return true;  // All conditions passed
}
```

### 8.5 UNarrativeTrigger - NPC Behavior Triggers

**Location:** `NarrativeArsenal/Public/Tales/NarrativeTrigger.h`

Triggers activate/deactivate based on game conditions and fire events.

```cpp
UCLASS(Abstract, Blueprintable, EditInlineNew)
class UNarrativeTrigger : public UObject
{
    // Events to fire when trigger activates/deactivates
    UPROPERTY(EditAnywhere, Instanced)
    TArray<UNarrativeEvent*> TriggerEvents;

    UPROPERTY()
    ANarrativeCharacter* OwnerCharacter;

    // Whether currently active
    UPROPERTY(EditAnywhere)
    bool bIsActive;

    virtual void Initialize();  // Bind delegates, timers
    virtual bool IsActive();
    virtual void Activate();    // Called when trigger fires
    virtual void Deactivate();
};
```

### 8.6 UTriggerSet - Trigger Containers

**Location:** `NarrativeArsenal/Public/Tales/TriggerSet.h`

```cpp
UCLASS()
class UTriggerSet : public UDataAsset
{
    // The triggers and their events
    UPROPERTY(EditAnywhere, Instanced)
    TArray<UNarrativeTrigger*> Triggers;
};
```

TriggerSets are assigned to characters via `CharacterDefinition.TriggerSets`.

### 8.7 Goals System - AI Activities from Dialogue

**Location:** `NarrativeArsenal/Public/AI/Activities/`

Goals tell NPCs what to do. Can be issued via dialogue events.

**Built-in Goals (Pro/Core/AI/Activities/):**

| Asset | Purpose |
|-------|---------|
| `Goal_Attack` | Attack a target |
| `Goal_Flee` | Run away |
| `Goal_FollowCharacter` | Follow someone |
| `Goal_MoveToDestination` | Go to location |
| `Goal_Patrol` | Patrol route |
| `Goal_Idle` | Do nothing |
| `Goal_Interact` | Interact with object |
| `Goal_DriveToDestination` | Drive vehicle to location |

**Adding Goals via Dialogue:**

```cpp
// UNarrativeEvent_AddGoalToNPC
UCLASS(DisplayName="AI: Add Goal To NPC")
class UNarrativeEvent_AddGoalToNPC : public UNarrativeEvent
{
    // The goal to add
    UPROPERTY(Instanced, EditDefaultsOnly)
    UNPCGoalItem* GoalToAdd;

    virtual void ExecuteEvent_Implementation(...) override
    {
        // Adds goal to NPC's activity system
    }
};
```

### 8.8 YAML Schema for Events & Conditions

```yaml
dialogue_blueprints:
  - name: DBP_QuestGiver
    folder: Dialogues
    speakers:
      - npc_definition: NPCDef_QuestGiver

    dialogue_tree:
      root: greeting
      nodes:
        # Greeting changes based on quest state
        - id: greeting
          type: npc
          speaker: QuestGiver
          text: "Ah, you're back! Did you find the artifact?"
          conditions:
            - type: NC_IsQuestInProgress
              quest: Q_FindArtifact
          player_replies: [yes_found, not_yet]

        - id: greeting_new
          type: npc
          speaker: QuestGiver
          text: "Hello traveler! I need your help."
          conditions:
            - type: NC_IsQuestInProgress
              quest: Q_FindArtifact
              not: true  # Inverted - show if quest NOT in progress
          player_replies: [accept_quest, decline]

        # Player accepts quest
        - id: accept_quest
          type: player
          text: "I'll help you."
          option_text: "[Accept Quest]"
          events:
            - type: NE_BeginQuest
              quest: Q_FindArtifact
              runtime: End  # Fire after line plays
          npc_replies: [quest_accepted]

        # Player returns with artifact
        - id: yes_found
          type: player
          text: "Yes, here it is."
          option_text: "[Give Artifact]"
          conditions:
            - type: NC_HasItem
              item: Item_Artifact
          events:
            - type: NE_RemoveItem
              item: Item_Artifact
            - type: NE_GiveXP
              amount: 500
            - type: NE_CompleteQuest
              quest: Q_FindArtifact
          npc_replies: [quest_complete]

        # NPC gives follower goal
        - id: follow_me
          type: npc
          speaker: QuestGiver
          text: "Come, follow me to the temple."
          events:
            - type: NE_AddGoalToNPC
              npc: NPCDef_QuestGiver
              goal: Goal_FollowCharacter
              target: Player
              runtime: Start
          player_replies: [lead_on]
```

### 8.9 Implementation Code for Events/Conditions

**Struct Definitions (add to GasAbilityGeneratorTypes.h):**

```cpp
/**
 * Event reference for dialogue nodes
 */
struct FManifestDialogueEventDefinition
{
    FString Type;                    // Event class name (NE_BeginQuest, etc.)
    FString Runtime = TEXT("Start"); // Start, End, Both
    TMap<FString, FString> Properties;  // Event-specific properties

    uint64 ComputeHash() const
    {
        uint64 Hash = GetTypeHash(Type);
        Hash ^= GetTypeHash(Runtime) << 4;
        for (const auto& Prop : Properties)
        {
            Hash ^= GetTypeHash(Prop.Key);
            Hash ^= GetTypeHash(Prop.Value);
        }
        return Hash;
    }
};

/**
 * Condition reference for dialogue nodes
 */
struct FManifestDialogueConditionDefinition
{
    FString Type;                    // Condition class name (NC_IsQuestInProgress, etc.)
    bool bNot = false;               // Invert result
    TMap<FString, FString> Properties;  // Condition-specific properties

    uint64 ComputeHash() const
    {
        uint64 Hash = GetTypeHash(Type);
        Hash ^= bNot ? 1 : 0;
        for (const auto& Prop : Properties)
        {
            Hash ^= GetTypeHash(Prop.Key);
            Hash ^= GetTypeHash(Prop.Value);
        }
        return Hash;
    }
};
```

**Update FManifestDialogueNodeDefinition:**

```cpp
struct FManifestDialogueNodeDefinition
{
    // ... existing fields ...

    // Events that fire when this node is reached
    TArray<FManifestDialogueEventDefinition> Events;

    // Conditions that must pass for node to appear
    TArray<FManifestDialogueConditionDefinition> Conditions;

    uint64 ComputeHash() const
    {
        // ... existing hash code ...
        for (const auto& Event : Events) Hash ^= Event.ComputeHash();
        for (const auto& Cond : Conditions) Hash ^= Cond.ComputeHash();
        return Hash;
    }
};
```

**Generator Implementation:**

```cpp
// Create event instance from definition
static UNarrativeEvent* CreateEventFromDefinition(
    UDialogueNode* Node,
    const FManifestDialogueEventDefinition& EventDef)
{
    // Find event class by name
    FString EventClassName = TEXT("/Game/Pro/Core/Tales/Events/") + EventDef.Type;
    UClass* EventClass = LoadClass<UNarrativeEvent>(nullptr, *EventClassName);

    if (!EventClass)
    {
        // Try as C++ class
        EventClass = FindObject<UClass>(ANY_PACKAGE, *EventDef.Type);
    }

    if (!EventClass)
    {
        UE_LOG(LogGasAbilityGenerator, Warning,
            TEXT("Could not find event class: %s"), *EventDef.Type);
        return nullptr;
    }

    // Create instance
    UNarrativeEvent* Event = NewObject<UNarrativeEvent>(Node, EventClass);

    // Set runtime
    if (EventDef.Runtime.Equals(TEXT("End"), ESearchCase::IgnoreCase))
        Event->EventRuntime = EEventRuntime::End;
    else if (EventDef.Runtime.Equals(TEXT("Both"), ESearchCase::IgnoreCase))
        Event->EventRuntime = EEventRuntime::Both;
    else
        Event->EventRuntime = EEventRuntime::Start;

    // Set properties via reflection
    for (const auto& Prop : EventDef.Properties)
    {
        FProperty* Property = EventClass->FindPropertyByName(FName(*Prop.Key));
        if (Property)
        {
            void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Event);
            Property->ImportText(*Prop.Value, ValuePtr, 0, Event);
        }
    }

    return Event;
}

// Create condition instance from definition
static UNarrativeCondition* CreateConditionFromDefinition(
    UDialogueNode* Node,
    const FManifestDialogueConditionDefinition& CondDef)
{
    // Find condition class
    FString CondClassName = TEXT("/Game/Pro/Core/Tales/Conditions/") + CondDef.Type;
    UClass* CondClass = LoadClass<UNarrativeCondition>(nullptr, *CondClassName);

    if (!CondClass)
    {
        CondClass = FindObject<UClass>(ANY_PACKAGE, *CondDef.Type);
    }

    if (!CondClass)
    {
        UE_LOG(LogGasAbilityGenerator, Warning,
            TEXT("Could not find condition class: %s"), *CondDef.Type);
        return nullptr;
    }

    UNarrativeCondition* Condition = NewObject<UNarrativeCondition>(Node, CondClass);
    Condition->bNot = CondDef.bNot;

    // Set properties via reflection
    for (const auto& Prop : CondDef.Properties)
    {
        FProperty* Property = CondClass->FindPropertyByName(FName(*Prop.Key));
        if (Property)
        {
            void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Condition);
            Property->ImportText(*Prop.Value, ValuePtr, 0, Condition);
        }
    }

    return Condition;
}

// In CreateDialogueTree(), after creating node:
// Add events
for (const auto& EventDef : NodeDef.Events)
{
    if (UNarrativeEvent* Event = CreateEventFromDefinition(DialogueNode, EventDef))
    {
        DialogueNode->Events.Add(Event);
    }
}

// Add conditions
for (const auto& CondDef : NodeDef.Conditions)
{
    if (UNarrativeCondition* Cond = CreateConditionFromDefinition(DialogueNode, CondDef))
    {
        DialogueNode->Conditions.Add(Cond);
    }
}
```

### 8.10 Complete YAML Example with Events/Conditions

```yaml
dialogue_blueprints:
  - name: DBP_MerchantDialogue
    folder: Dialogues/NPCs
    speakers:
      - npc_definition: NPCDef_Merchant
        node_color: "#00AA00"

    dialogue_tree:
      root: greeting

      nodes:
        # Check if player has completed intro quest
        - id: greeting
          type: npc
          speaker: Merchant
          text: "Welcome back, friend! What can I get for you today?"
          conditions:
            - type: NC_IsQuestSucceeded
              properties:
                Quest: /Game/Quests/Q_IntroQuest
          player_replies: [buy, sell, gossip, leave]

        # First-time greeting (quest not done)
        - id: greeting_new
          type: npc
          speaker: Merchant
          text: "Hmm, I don't know you. Come back after you've proven yourself."
          conditions:
            - type: NC_IsQuestSucceeded
              not: true
              properties:
                Quest: /Game/Quests/Q_IntroQuest
          # No replies - ends dialogue

        # Open shop
        - id: buy
          type: player
          text: "Show me what you have."
          option_text: "[Shop]"
          events:
            - type: NE_OpenShop
              runtime: End
              properties:
                ShopInventory: /Game/Items/ShopInventory_Merchant
          # Dialogue ends, shop opens

        # Gossip option - gives quest
        - id: gossip
          type: player
          text: "Heard any rumors lately?"
          option_text: "[Gossip]"
          npc_replies: [gossip_response]

        - id: gossip_response
          type: npc
          speaker: Merchant
          text: "Actually yes... there's trouble at the old mine. Someone should investigate."
          events:
            - type: NE_BeginQuest
              runtime: End
              properties:
                Quest: /Game/Quests/Q_MineInvestigation
            - type: NE_ShowNotification
              runtime: End
              properties:
                Title: "New Quest"
                Message: "Investigate the old mine"
          player_replies: [accept_mine, decline_mine]

        - id: accept_mine
          type: player
          text: "I'll check it out."
          option_text: "[Accept]"
          events:
            - type: NE_AddGameplayTags
              runtime: Start
              properties:
                Tags: Quest.Mine.Accepted
          npc_replies: [good_luck]

        - id: good_luck
          type: npc
          speaker: Merchant
          text: "Be careful out there!"

        - id: leave
          type: player
          text: "I should go."
          option_text: "[Leave]"
          auto_select_if_only: true
          npc_replies: [farewell]

        - id: farewell
          type: npc
          speaker: Merchant
          text: "Safe travels!"
```

---

**End of Handoff Document**
