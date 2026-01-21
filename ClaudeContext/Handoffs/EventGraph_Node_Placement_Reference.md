# Event Graph Node Placement Reference

**Version:** 2.0
**Created:** 2026-01-21
**Updated:** 2026-01-21
**Purpose:** Comprehensive reference for Blueprint node sizing and placement algorithms
**Status:** LOCKED CONTRACT - Implementation Approved

---

## PLACEMENT CONTRACT v1.0 (LOCKED)

> **Audit Status:** Claude ✅ | GPT ✅ | Erdem ✅ (2026-01-21)
>
> This contract is LOCKED. Any changes require explicit approval from Erdem.

### Root Cause

**File:** `GasAbilityGeneratorGenerators.cpp:9510-9511`
```cpp
CreatedNode->NodePosX = 200 + (NodesCreated * 250);
CreatedNode->NodePosY = 0;  // ALL NODES AT Y=0 - CAUSES OVERLAP
```

### Manifest Type → Family Mapping (LOCKED)

| Manifest `type:` | UK2Node Class | Family |
|------------------|---------------|--------|
| `Event` | UK2Node_Event | **Event** |
| `CustomEvent` | UK2Node_CustomEvent | **Event** |
| `CallFunction` | UK2Node_CallFunction | **Exec Logic** |
| `Branch` | UK2Node_IfThenElse | **Exec Logic** |
| `Sequence` | UK2Node_ExecutionSequence | **Exec Logic** |
| `Delay` | UK2Node_Delay | **Exec Logic** |
| `AbilityTaskWaitDelay` | UK2Node_LatentAbilityCall | **Exec Logic** |
| `DynamicCast` | UK2Node_DynamicCast | **Exec Logic** |
| `SpawnActor` | UK2Node_SpawnActor | **Exec Logic** |
| `ForEachLoop` | UK2Node_ForEachElementInEnum | **Exec Logic** |
| `VariableSet` | UK2Node_VariableSet | **Exec Logic** |
| `PrintString` | UK2Node_CallFunction | **Exec Logic** |
| `VariableGet` | UK2Node_VariableGet | **Data/Pure** |
| `PropertyGet` | UK2Node_CallFunction (pure) | **Data/Pure** |
| `Self` | UK2Node_Self | **Data/Pure** |
| `BreakStruct` | UK2Node_BreakStruct | **Data/Pure** (pin-scaled) |
| `MakeArray` | UK2Node_MakeArray | **Data/Pure** (pin-scaled) |
| `GetArrayItem` | UK2Node_GetArrayItem | **Data/Pure** |

### Height Formulas (LOCKED)

| Family | Formula | Source |
|--------|---------|--------|
| **Event** | `48 + (MaxPins × 16)` | EdGraphSchema_K2.cpp:7025-7029 |
| **Exec Logic** | `80 + (MaxPins × 18)` | EdGraphSchema_K2.cpp:7019-7023 |
| **Data/Pure** (default) | `48` (flat) | EdGraphSchema_K2.cpp:7015 |
| **Data/Pure** (BreakStruct, MakeArray) | `48 + (MaxPins × 18)` | Variable output pin count |

### Placement Constants (LOCKED)

| Constant | Value | Source |
|----------|-------|--------|
| `GRID_SIZE` | 16 | EditorStyleClasses.cpp:53 |
| `HORIZONTAL_LAYER_SPACING` | 350 | Audit consensus |
| `VERTICAL_NODE_GAP` | 50 | K2Node_PromotableOperator.cpp:1085 |
| `LANE_SEPARATION` | 150 | Audit consensus |
| `DATA_NODE_X_OFFSET` | -250 | Places data nodes left of consumers |

### Exec Pin Names (LOCKED)

Outbound exec flow pins for BFS traversal:
```
Then, Exec, Completed, true, false, Out_*
```

### EntryOrderIndex Definition (LOCKED)

```
EntryOrderIndex = manifest event_graph.nodes array index for Event/CustomEvent nodes
```

Purpose: Deterministic tie-breaker for stable layout ordering.

### Algorithm (LOCKED)

1. **Identify Entry Points:** Nodes with `type: Event` or `type: CustomEvent`
2. **Assign Lanes:** Each entry point gets unique lane by EntryOrderIndex
3. **BFS Layer Assignment:** Follow exec pins, Layer = distance from entry
4. **Calculate Positions:**
   - `X = Layer × HORIZONTAL_LAYER_SPACING`
   - `Y = (LaneIndex × LANE_SEPARATION) + cumulative node heights`
5. **Position Data Nodes:** Place near first consumer, X offset = DATA_NODE_X_OFFSET
6. **Grid Snap:** All positions to GRID_SIZE (16)

### Acceptance Tests (LOCKED)

| # | Test | Criterion |
|---|------|-----------|
| 1 | **Determinism** | Same manifest → identical positions across runs |
| 2 | **Grid Alignment** | All NodePosX, NodePosY divisible by 16 |
| 3 | **No Overlap** | No two nodes share bounding box intersection |
| 4 | **Exec Monotonicity** | X(Node) < X(Successor) for all exec connections |
| 5 | **Lane Separation** | Different entry chains have Y gap ≥ 150 |
| 6 | **Orphan Warnings** | Exec nodes unreachable from entry logged as warnings |

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [UE5 Built-in Utilities](#2-ue5-built-in-utilities)
3. [Node Size Constants Catalogue](#3-node-size-constants-catalogue)
4. [Padding and Spacing Constants](#4-padding-and-spacing-constants)
5. [Grid System](#5-grid-system)
6. [Node Positioning Algorithms](#6-node-positioning-algorithms)
7. [Proposed Spacing Logic](#7-proposed-spacing-logic)
8. [Implementation Plan](#8-implementation-plan)
9. [Source References](#9-source-references)

---

## 1. Executive Summary

### Key Findings

1. **UE5 provides `UEdGraphSchema_K2::EstimateNodeHeight()`** - A built-in function that estimates node height based on node type and pin count. This is the primary utility we should use.

2. **No built-in auto-layout algorithm** - UE5 does NOT have a full auto-arrange algorithm for Blueprint graphs. Layout is manual or relies on grid snapping.

3. **Node dimensions are computed at render time** by Slate widgets, but can be estimated before rendering using the constants documented below.

4. **Graph Formatter plugin** (open source) exists as a reference implementation using the Sugiyama/layered graph drawing algorithm.

### Recommendation

Implement a **layered graph layout algorithm** using:
- `EstimateNodeHeight()` for vertical sizing
- Hardcoded width estimates based on node type
- Topological sort for horizontal layer assignment
- Separate vertical lanes for each event chain

---

## 2. UE5 Built-in Utilities

### 2.1 EstimateNodeHeight()

**Location:** `Engine/Source/Editor/BlueprintGraph/Classes/EdGraphSchema_K2.h:1205`

```cpp
/** Calculates an estimated height for the specified node */
static UE_API float EstimateNodeHeight( UEdGraphNode* Node );
```

**Implementation:** `Engine/Source/Editor/BlueprintGraph/Private/EdGraphSchema_K2.cpp:7009-7067`

```cpp
/**
 * Attempts to best-guess the height of the node. This is necessary because we don't know the actual
 * size of the node until the next Slate tick
 *
 * @param Node The node to guess the height of
 * @return The estimated height of the specified node
 */
float UEdGraphSchema_K2::EstimateNodeHeight( UEdGraphNode* Node )
{
    float HeightEstimate = 0.0f;

    if ( Node != NULL )
    {
        float BaseNodeHeight = 48.0f;
        bool bConsiderNodePins = false;
        float HeightPerPin = 18.0f;

        if ( Node->IsA( UK2Node_CallFunction::StaticClass() ) )
        {
            BaseNodeHeight = 80.0f;
            bConsiderNodePins = true;
            HeightPerPin = 18.0f;
        }
        else if ( Node->IsA( UK2Node_Event::StaticClass() ) )
        {
            BaseNodeHeight = 48.0f;
            bConsiderNodePins = true;
            HeightPerPin = 16.0f;
        }

        HeightEstimate = BaseNodeHeight;

        if ( bConsiderNodePins )
        {
            int32 NumInputPins = 0;
            int32 NumOutputPins = 0;

            for ( int32 PinIndex = 0; PinIndex < Node->Pins.Num(); PinIndex++ )
            {
                UEdGraphPin* CurrentPin = Node->Pins[PinIndex];
                if ( CurrentPin != NULL && !CurrentPin->bHidden )
                {
                    switch ( CurrentPin->Direction )
                    {
                        case EGPD_Input:  NumInputPins++;  break;
                        case EGPD_Output: NumOutputPins++; break;
                    }
                }
            }

            float MaxNumPins = float(FMath::Max<int32>( NumInputPins, NumOutputPins ));
            HeightEstimate += MaxNumPins * HeightPerPin;
        }
    }

    return HeightEstimate;
}
```

**Usage Pattern:** (from `BlueprintFunctionNodeSpawner.cpp:175`)
```cpp
float const EstimatedFuncNodeHeight = UEdGraphSchema_K2::EstimateNodeHeight(InputNode);
```

### 2.2 GetSnapGridSize()

**Location:** `Engine/Source/Editor/GraphEditor/Public/SNodePanel.h:862`

```cpp
/** Get the grid snap size */
static UE_API uint32 GetSnapGridSize();
```

**Implementation:** `Engine/Source/Editor/GraphEditor/Private/SNodePanel.cpp:1404-1407`

```cpp
uint32 SNodePanel::GetSnapGridSize()
{
    return GetDefault<UEditorStyleSettings>()->GridSnapSize;
}
```

**Default Value:** `16` pixels (from `EditorStyleClasses.cpp:53`)

**Usage Pattern:**
```cpp
Node->SnapToGrid(SNodePanel::GetSnapGridSize());
// Or manually:
Node->SnapToGrid(GetDefault<UEditorStyleSettings>()->GridSnapSize);
```

### 2.3 PositionThisNodeBetweenOtherNodes()

**Location:** `Engine/Source/Editor/GraphEditor/Private/SGraphNode.cpp:1420-1477`

**Purpose:** Positions a node between two other nodes (or sets of nodes) along an imaginary line.

**Three Overloads:**
1. Single previous + single next node
2. Set of previous + set of next nodes
3. Direct coordinates (PrevPos, NextPos)

**Algorithm (simplified):**
```cpp
void SGraphNode::PositionThisNodeBetweenOtherNodes(
    const FVector2f& PrevPos,
    const FVector2f& NextPos,
    float HeightAboveWire) const
{
    const FVector2f DesiredNodeSize = GetDesiredSize();

    FVector2f DeltaPos(NextPos - PrevPos);
    if (DeltaPos.IsNearlyZero())
    {
        DeltaPos = FVector2f(10.0f, 0.0f);
    }

    // Normal perpendicular to the connection line
    const FVector2f Normal = FVector2f(DeltaPos.Y, -DeltaPos.X).GetSafeNormal();

    // Position at midpoint, offset by HeightAboveWire along normal
    const FVector2f NewCenter = PrevPos + (0.5f * DeltaPos) + (HeightAboveWire * Normal);

    // Adjust for node size (corner position, not center)
    const FVector2f NewCorner = NewCenter - (0.5f * DesiredNodeSize);

    GraphNode->SetPosition(NewCorner);
}
```

**Note:** Requires Slate widgets to exist. Not directly usable during asset generation, but algorithm can be replicated.

### 2.4 CalculateApproximateNodeBoundaries()

**Location:** `Engine/Source/Editor/UnrealEd/Public/EdGraphUtilities.h:130`

```cpp
static UNREALED_API FIntRect CalculateApproximateNodeBoundaries(
    const TArray<UEdGraphNode*>& Nodes
);
```

**Purpose:** Calculates bounding box for a set of nodes. Used for zoom-to-fit.

---

## 3. Node Size Constants Catalogue

### 3.1 Height Constants (from UE5 Source)

| Node Type | Base Height | Per-Pin Height | Source |
|-----------|-------------|----------------|--------|
| **Generic (default)** | 48.0f | N/A | EdGraphSchema_K2.cpp:7015 |
| **UK2Node_CallFunction** | 80.0f | 18.0f | EdGraphSchema_K2.cpp:7019-7023 |
| **UK2Node_Event** | 48.0f | 16.0f | EdGraphSchema_K2.cpp:7025-7029 |
| **Variable Node (literal)** | 48.0f | N/A | BlueprintFunctionNodeSpawner.cpp:174 |
| **Literal Reference** | 48.0f | N/A | EdGraphSchema_K2_Actions.cpp:44 |

### 3.2 Width Constants (from UE5 Source)

| Node Type | Estimated Width | Source |
|-----------|-----------------|--------|
| **Variable Node** | 224.0f | BlueprintFunctionNodeSpawner.cpp:170 |
| **Literal Reference** | 224.0f | EdGraphSchema_K2_Actions.cpp:41 |
| **Function Node (typical)** | 280.0f | Observed in SGraphPinStruct.cpp:123 |

### 3.3 Height Estimation Formula

For nodes that consider pins:
```
EstimatedHeight = BaseHeight + (MaxPinCount * HeightPerPin)

Where:
  MaxPinCount = Max(NumVisibleInputPins, NumVisibleOutputPins)
```

**Examples:**

| Node | Base | Pins (In/Out) | HeightPerPin | Estimated Height |
|------|------|---------------|--------------|------------------|
| Event BeginPlay | 48 | 0/1 | 16 | 48 + (1 * 16) = 64 |
| Event with 3 outputs | 48 | 0/3 | 16 | 48 + (3 * 16) = 96 |
| Simple Function (2 in, 1 out) | 80 | 2/1 | 18 | 80 + (2 * 18) = 116 |
| Complex Function (5 in, 3 out) | 80 | 5/3 | 18 | 80 + (5 * 18) = 170 |
| Branch | 80 | 1/2 | 18 | 80 + (2 * 18) = 116 |
| Variable Get | 48 | 0/1 | N/A | 48 |

### 3.4 Extended Node Type Mapping

Based on UK2Node class hierarchy, here's the complete mapping:

| Node Class | Parent | Treatment |
|------------|--------|-----------|
| UK2Node_Event | UK2Node | Base 48, PerPin 16 |
| UK2Node_CustomEvent | UK2Node_Event | Base 48, PerPin 16 |
| UK2Node_CallFunction | UK2Node | Base 80, PerPin 18 |
| UK2Node_CallParentFunction | UK2Node_CallFunction | Base 80, PerPin 18 |
| UK2Node_CallArrayFunction | UK2Node_CallFunction | Base 80, PerPin 18 |
| UK2Node_CallDataTableFunction | UK2Node_CallFunction | Base 80, PerPin 18 |
| UK2Node_VariableGet | UK2Node | Base 48 (no pin scaling) |
| UK2Node_VariableSet | UK2Node | Base 48 (no pin scaling) |
| UK2Node_IfThenElse (Branch) | UK2Node | Base 80, PerPin 18 |
| UK2Node_ExecutionSequence | UK2Node | Base 80, PerPin 18 |
| UK2Node_MacroInstance | UK2Node | Base 80, PerPin 18 |
| UK2Node_DynamicCast | UK2Node | Base 80, PerPin 18 |
| UK2Node_SpawnActor | UK2Node_CallFunction | Base 80, PerPin 18 |
| UK2Node_Delay (Latent) | UK2Node | Base 80, PerPin 18 |
| UK2Node_Self | UK2Node | Base 48 |
| UK2Node_MakeArray | UK2Node | Base 80, PerPin 18 |
| UK2Node_GetArrayItem | UK2Node | Base 80, PerPin 18 |
| UK2Node_BreakStruct | UK2Node | Base 80, PerPin 18 |
| UK2Node_MakeStruct | UK2Node | Base 80, PerPin 18 |

---

## 4. Padding and Spacing Constants

### 4.1 Node Internal Padding

**Source:** `Engine/Source/Editor/GraphEditor/Private/KismetNodes/SGraphNodeK2Base.cpp:155-199`

| Constant | Value | Purpose |
|----------|-------|---------|
| MinNodePadding | 55.0f | Minimum horizontal padding |
| MaxNodePadding | 180.0f | Maximum horizontal padding |
| PaddingIncrementSize | 20.0f | Per-character increment |
| PinPaddingTop (impure nodes) | 8.0f | Extra top padding for exec pin alignment |

**Title-based padding formula:**
```cpp
// For compact pure nodes
int32 HeadTitleLength = NodeTitle.Get()->GetHeadTitle().ToString().Len();
float PinPaddingRight = FMath::Clamp(
    MinNodePadding + ((float)HeadTitleLength * PaddingIncrementSize),
    MinNodePadding,
    MaxNodePadding
);
```

### 4.2 Inter-Node Spacing (UE5 Conventions)

**Source:** Various files in BlueprintGraph

| Spacing Type | Value | Source |
|--------------|-------|--------|
| Horizontal (node to node) | 256.0f | EdGraphSchema_K2.cpp:3386, 3424, 3479 |
| Horizontal (literal offset) | 224.0f | EdGraphSchema_K2_Actions.cpp:41 |
| Vertical (stacked nodes) | 80.0f | Kismet/BlueprintEditor.cpp:4028 |
| Vertical (chained operators) | 50.0f | K2Node_PromotableOperator.cpp:1085 |
| Push-off distance | 60 | EdGraphSchema_K2_Actions.cpp:38 |

### 4.3 Recommended Spacing for Generation

Based on UE5 source patterns:

| Dimension | Recommended Value | Rationale |
|-----------|-------------------|-----------|
| **Horizontal Layer Spacing** | 300-400 pixels | Accounts for typical node width + readable gap |
| **Vertical Node Spacing** | EstimatedHeight + 100 pixels | Prevents overlap, allows for wire routing |
| **Event Chain Separation** | 400+ pixels | Clear visual separation between execution chains |
| **Data Node Offset** | 224 pixels left | Places variable getters near consumers |

---

## 5. Grid System

### 5.1 Grid Configuration

| Property | Default Value | Source |
|----------|---------------|--------|
| GridSnapSize | 16 | EditorStyleClasses.cpp:53 |
| GraphSmallestGridSize | 8.0f | SNodePanel.cpp:1425 |

### 5.2 Grid Snapping Usage

```cpp
// Method 1: Use node's built-in snap
Node->SnapToGrid(SNodePanel::GetSnapGridSize());

// Method 2: Use editor settings directly
Node->SnapToGrid(GetDefault<UEditorStyleSettings>()->GridSnapSize);

// Method 3: Manual calculation
uint32 GridSize = 16; // Default
FVector2f SnappedPos(
    FMath::RoundToFloat(Position.X / GridSize) * GridSize,
    FMath::RoundToFloat(Position.Y / GridSize) * GridSize
);
```

### 5.3 Grid Snap Formula

```cpp
float FMath::GridSnap(float Location, float Grid)
{
    if (Grid == 0.f)
        return Location;
    return FMath::RoundToFloat(Location / Grid) * Grid;
}
```

---

## 6. Node Positioning Algorithms

### 6.1 Current Generator Behavior (Problem)

The current GasAbilityGenerator places nodes without considering:
- Execution flow distance from entry events
- Separation of different event chains
- Actual node dimensions
- Wire crossing minimization

Result: Overlapping nodes, wire spaghetti, unreadable graphs.

### 6.2 Sugiyama/Layered Graph Drawing Algorithm

The standard algorithm for DAG visualization (used by Graph Formatter plugin):

**Phase 1: Cycle Removal**
- Blueprint graphs are DAGs (no cycles in exec flow)
- Skip this phase for our use case

**Phase 2: Layer Assignment**
- Assign each node to a horizontal layer based on longest path from source
- Entry events = Layer 0
- Each subsequent exec connection = +1 layer

**Phase 3: Crossing Minimization**
- Reorder nodes within each layer to minimize edge crossings
- NP-hard problem, use heuristics (barycenter, median)

**Phase 4: Coordinate Assignment**
- X = Layer * HorizontalSpacing
- Y = Cumulative height of nodes above in same layer

### 6.3 Simplified Algorithm for Generator

For our use case, a simpler approach:

```
1. IDENTIFY entry points (Event nodes, CustomEvent nodes)
2. FOR EACH entry point:
   a. BFS/DFS traverse execution flow
   b. Assign layer number = distance from entry
   c. Track which "lane" (entry point) each node belongs to
3. CALCULATE positions:
   a. X = Layer * HORIZONTAL_SPACING
   b. Y = (LaneIndex * LANE_HEIGHT) + (NodeIndexInLayer * NodeHeight)
4. SNAP to grid
```

---

## 7. Proposed Spacing Logic

### 7.1 Constants Definition

```cpp
namespace NodePlacement
{
    // Horizontal spacing between layers (execution flow direction)
    constexpr float HORIZONTAL_LAYER_SPACING = 350.0f;

    // Vertical spacing between nodes in the same layer
    constexpr float VERTICAL_NODE_GAP = 50.0f;

    // Vertical spacing between different event chains (lanes)
    constexpr float LANE_SEPARATION = 150.0f;

    // Offset for pure/data nodes relative to their consumers
    constexpr float DATA_NODE_X_OFFSET = -250.0f;
    constexpr float DATA_NODE_Y_OFFSET = 0.0f;

    // Grid snap size
    constexpr uint32 GRID_SIZE = 16;

    // Starting position for first event
    constexpr float START_X = 0.0f;
    constexpr float START_Y = 0.0f;
}
```

### 7.2 Height Estimation Function

```cpp
float EstimateNodeHeight(const FManifestEventGraphNodeDefinition& NodeDef)
{
    // Count visible pins based on node type
    int32 NumInputPins = 0;
    int32 NumOutputPins = 0;

    // Exec pins
    if (NodeDef.Type != "VariableGet" && NodeDef.Type != "Self")
    {
        NumInputPins++;  // Exec in
        NumOutputPins++; // Exec out (Then)
    }

    // Type-specific pins
    if (NodeDef.Type == "Branch")
    {
        NumInputPins++;   // Condition
        NumOutputPins++;  // False (True already counted)
    }
    else if (NodeDef.Type == "CallFunction")
    {
        // Count from properties/parameters
        NumInputPins += NodeDef.InputPinCount;
        NumOutputPins += NodeDef.OutputPinCount;
    }
    // ... etc for other types

    // Apply formula
    float BaseHeight = IsEventNode(NodeDef) ? 48.0f : 80.0f;
    float HeightPerPin = IsEventNode(NodeDef) ? 16.0f : 18.0f;

    int32 MaxPins = FMath::Max(NumInputPins, NumOutputPins);

    return BaseHeight + (MaxPins * HeightPerPin);
}
```

### 7.3 Width Estimation Function

```cpp
float EstimateNodeWidth(const FManifestEventGraphNodeDefinition& NodeDef)
{
    // Base widths by type
    if (NodeDef.Type == "VariableGet" || NodeDef.Type == "VariableSet")
    {
        return 224.0f;
    }
    else if (NodeDef.Type == "Self")
    {
        return 150.0f;
    }
    else if (NodeDef.Type == "Event" || NodeDef.Type == "CustomEvent")
    {
        // Events tend to be wider due to title
        return 250.0f + (NodeDef.EventName.Len() * 8.0f); // Approximate
    }
    else
    {
        // Function nodes - base + title adjustment
        float BaseWidth = 280.0f;
        // Could adjust based on function name length
        return BaseWidth;
    }
}
```

### 7.4 Layer Assignment Algorithm

```cpp
struct FNodePlacementInfo
{
    FString NodeId;
    int32 Layer;           // Execution distance from entry
    int32 LaneIndex;       // Which event chain this belongs to
    int32 IndexInLayer;    // Position within the layer
    float EstimatedHeight;
    float EstimatedWidth;
    FVector2D Position;
};

void AssignLayers(
    const TArray<FManifestEventGraphNodeDefinition>& Nodes,
    const TArray<FManifestConnectionDefinition>& Connections,
    TMap<FString, FNodePlacementInfo>& OutPlacements)
{
    // 1. Find all entry points (Events, CustomEvents)
    TArray<FString> EntryNodes;
    for (const auto& Node : Nodes)
    {
        if (Node.Type == "Event" || Node.Type == "CustomEvent")
        {
            EntryNodes.Add(Node.Id);
        }
    }

    // 2. Build adjacency list for exec connections only
    TMap<FString, TArray<FString>> ExecGraph;
    for (const auto& Conn : Connections)
    {
        // Only follow exec flow (Then, Exec pins)
        if (Conn.FromPin == "Then" || Conn.FromPin == "Exec" ||
            Conn.FromPin == "True" || Conn.FromPin == "False" ||
            Conn.FromPin.StartsWith("Out_"))
        {
            ExecGraph.FindOrAdd(Conn.FromNode).Add(Conn.ToNode);
        }
    }

    // 3. BFS from each entry point
    int32 LaneIndex = 0;
    for (const FString& Entry : EntryNodes)
    {
        TQueue<TPair<FString, int32>> Queue; // NodeId, Layer
        TSet<FString> Visited;

        Queue.Enqueue(MakeTuple(Entry, 0));
        Visited.Add(Entry);

        while (!Queue.IsEmpty())
        {
            TPair<FString, int32> Current;
            Queue.Dequeue(Current);

            FNodePlacementInfo& Info = OutPlacements.FindOrAdd(Current.Key);
            Info.NodeId = Current.Key;
            Info.Layer = FMath::Max(Info.Layer, Current.Value); // Take max if visited from multiple paths
            Info.LaneIndex = LaneIndex;

            // Enqueue successors
            if (TArray<FString>* Successors = ExecGraph.Find(Current.Key))
            {
                for (const FString& Next : *Successors)
                {
                    if (!Visited.Contains(Next))
                    {
                        Visited.Add(Next);
                        Queue.Enqueue(MakeTuple(Next, Current.Value + 1));
                    }
                }
            }
        }

        LaneIndex++;
    }
}
```

### 7.5 Position Calculation Algorithm

```cpp
void CalculatePositions(
    const TArray<FManifestEventGraphNodeDefinition>& Nodes,
    TMap<FString, FNodePlacementInfo>& Placements)
{
    // 1. Estimate sizes
    for (const auto& Node : Nodes)
    {
        if (FNodePlacementInfo* Info = Placements.Find(Node.Id))
        {
            Info->EstimatedHeight = EstimateNodeHeight(Node);
            Info->EstimatedWidth = EstimateNodeWidth(Node);
        }
    }

    // 2. Group by layer and lane
    TMap<int32, TMap<int32, TArray<FString>>> LayerLaneNodes; // Layer -> Lane -> NodeIds
    for (auto& Pair : Placements)
    {
        LayerLaneNodes.FindOrAdd(Pair.Value.Layer)
            .FindOrAdd(Pair.Value.LaneIndex)
            .Add(Pair.Key);
    }

    // 3. Calculate positions
    float CurrentLaneY = NodePlacement::START_Y;

    // Process lanes
    TArray<int32> Lanes;
    // Collect all unique lanes across all layers
    for (auto& LayerPair : LayerLaneNodes)
    {
        for (auto& LanePair : LayerPair.Value)
        {
            Lanes.AddUnique(LanePair.Key);
        }
    }
    Lanes.Sort();

    // Track max height per lane for lane separation
    TMap<int32, float> LaneMaxHeight;

    for (int32 Lane : Lanes)
    {
        float LaneStartY = CurrentLaneY;
        float MaxHeightInLane = 0.0f;

        // Process each layer in this lane
        int32 MaxLayer = 0;
        for (auto& LayerPair : LayerLaneNodes)
        {
            MaxLayer = FMath::Max(MaxLayer, LayerPair.Key);
        }

        for (int32 Layer = 0; Layer <= MaxLayer; Layer++)
        {
            if (TMap<int32, TArray<FString>>* LaneMap = LayerLaneNodes.Find(Layer))
            {
                if (TArray<FString>* NodesInLayerLane = LaneMap->Find(Lane))
                {
                    float CurrentY = LaneStartY;

                    for (const FString& NodeId : *NodesInLayerLane)
                    {
                        FNodePlacementInfo& Info = Placements[NodeId];

                        // X based on layer
                        Info.Position.X = NodePlacement::START_X +
                            (Layer * NodePlacement::HORIZONTAL_LAYER_SPACING);

                        // Y based on cumulative height
                        Info.Position.Y = CurrentY;

                        CurrentY += Info.EstimatedHeight + NodePlacement::VERTICAL_NODE_GAP;
                        MaxHeightInLane = FMath::Max(MaxHeightInLane, CurrentY - LaneStartY);
                    }
                }
            }
        }

        // Move to next lane
        CurrentLaneY += MaxHeightInLane + NodePlacement::LANE_SEPARATION;
    }

    // 4. Snap to grid
    for (auto& Pair : Placements)
    {
        Pair.Value.Position.X = FMath::GridSnap(Pair.Value.Position.X, (float)NodePlacement::GRID_SIZE);
        Pair.Value.Position.Y = FMath::GridSnap(Pair.Value.Position.Y, (float)NodePlacement::GRID_SIZE);
    }
}
```

### 7.6 Data Node Positioning

Pure functions and variable getters should be placed near their consumers:

```cpp
void PositionDataNodes(
    const TArray<FManifestEventGraphNodeDefinition>& Nodes,
    const TArray<FManifestConnectionDefinition>& Connections,
    TMap<FString, FNodePlacementInfo>& Placements)
{
    // Find nodes not in exec flow (pure nodes, variable getters)
    TSet<FString> DataNodes;
    for (const auto& Node : Nodes)
    {
        if (!Placements.Contains(Node.Id))
        {
            DataNodes.Add(Node.Id);
        }
    }

    // Position relative to first consumer
    for (const FString& DataNodeId : DataNodes)
    {
        // Find first node that consumes this data node's output
        for (const auto& Conn : Connections)
        {
            if (Conn.FromNode == DataNodeId)
            {
                if (FNodePlacementInfo* ConsumerInfo = Placements.Find(Conn.ToNode))
                {
                    FNodePlacementInfo& DataInfo = Placements.Add(DataNodeId);
                    DataInfo.NodeId = DataNodeId;

                    // Position to the left of consumer
                    DataInfo.Position.X = ConsumerInfo->Position.X +
                        NodePlacement::DATA_NODE_X_OFFSET;
                    DataInfo.Position.Y = ConsumerInfo->Position.Y;

                    // Snap to grid
                    DataInfo.Position.X = FMath::GridSnap(
                        DataInfo.Position.X, (float)NodePlacement::GRID_SIZE);
                    DataInfo.Position.Y = FMath::GridSnap(
                        DataInfo.Position.Y, (float)NodePlacement::GRID_SIZE);

                    break;
                }
            }
        }
    }
}
```

---

## 8. Implementation Plan

### 8.1 Phase 1: Add Size Estimation

1. Create `FNodePlacementInfo` struct in `GasAbilityGeneratorTypes.h`
2. Implement `EstimateNodeHeight()` and `EstimateNodeWidth()` helper functions
3. Use UE5's `UEdGraphSchema_K2::EstimateNodeHeight()` where possible

### 8.2 Phase 2: Add Layer Assignment

1. Implement BFS-based layer assignment
2. Handle multiple entry points (separate lanes)
3. Handle pure/data nodes separately

### 8.3 Phase 3: Add Position Calculation

1. Implement position calculation with proper spacing
2. Add grid snapping
3. Handle edge cases (disconnected nodes, self-loops)

### 8.4 Phase 4: Integration

1. Modify event graph generation to use new placement system
2. Apply positions to `UEdGraphNode::NodePosX` and `NodePosY`
3. Test with complex graphs (Father abilities)

### 8.5 Testing Checklist

- [ ] Single event chain layouts correctly
- [ ] Multiple event chains are vertically separated
- [ ] No node overlaps
- [ ] Pure functions positioned near consumers
- [ ] Grid alignment is consistent
- [ ] Complex graphs (10+ nodes) remain readable

---

## 9. Source References

### 9.1 UE5 Engine Source Files

| File | Key Contents |
|------|--------------|
| `Editor/BlueprintGraph/Classes/EdGraphSchema_K2.h:1205` | `EstimateNodeHeight()` declaration |
| `Editor/BlueprintGraph/Private/EdGraphSchema_K2.cpp:7009-7067` | `EstimateNodeHeight()` implementation |
| `Editor/BlueprintGraph/Private/BlueprintFunctionNodeSpawner.cpp:168-181` | `CalculateBindingPosition()` with size constants |
| `Editor/BlueprintGraph/Private/EdGraphSchema_K2_Actions.cpp:38-44` | NodeDistance, literal spacing constants |
| `Editor/BlueprintGraph/Private/EdGraphSchema_K2.cpp:3386,3424,3479` | 256px horizontal spacing pattern |
| `Editor/GraphEditor/Private/KismetNodes/SGraphNodeK2Base.cpp:155-199` | Padding constants |
| `Editor/GraphEditor/Private/SGraphNode.cpp:1420-1477` | `PositionThisNodeBetweenOtherNodes()` |
| `Editor/GraphEditor/Public/SNodePanel.h:862` | `GetSnapGridSize()` declaration |
| `Editor/GraphEditor/Private/SNodePanel.cpp:1404-1407` | `GetSnapGridSize()` implementation |
| `Editor/UnrealEd/Private/Settings/EditorStyleClasses.cpp:53` | Default GridSnapSize = 16 |

### 9.2 External References

| Resource | URL/Location |
|----------|--------------|
| Graph Formatter Plugin (Open Source) | https://github.com/howaajin/graphformatter |
| Graph Formatter Fork | https://github.com/ericblade/unreal-graphformatter |
| Sugiyama Algorithm | https://en.wikipedia.org/wiki/Layered_graph_drawing |
| UE5 API: UEdGraphNode | https://docs.unrealengine.com/5.0/en-US/API/Runtime/Engine/EdGraph/UEdGraphNode/ |
| UE5 API: SGraphNode | https://docs.unrealengine.com/4.27/en-US/API/Editor/GraphEditor/SGraphNode/ |

### 9.3 Related Plugin Documentation

| File | Purpose |
|------|---------|
| `ClaudeContext/Handoffs/Generator_Implementation_Reference.md` | Generator patterns |
| `ClaudeContext/Handoffs/Architecture_Reference.md` | Plugin architecture |

---

## Appendix A: Quick Reference Card

### Height Formula
```
Height = BaseHeight + (Max(InputPins, OutputPins) * HeightPerPin)
```

### Size Constants
| Node Type | Base Height | Per-Pin | Typical Width |
|-----------|-------------|---------|---------------|
| Event | 48 | 16 | 250 |
| Function | 80 | 18 | 280 |
| Variable | 48 | - | 224 |

### Spacing Constants
| Dimension | Value |
|-----------|-------|
| Horizontal Layer | 350 |
| Vertical Gap | 50 |
| Lane Separation | 150 |
| Grid Size | 16 |

### Quick Width Estimates
| Node Type | Width |
|-----------|-------|
| Variable Get/Set | 224 |
| Self | 150 |
| Event | 250+ |
| Function Call | 280 |
| Branch | 200 |
| Sequence | 180 |

---

*Document generated from UE 5.7 source code analysis*
