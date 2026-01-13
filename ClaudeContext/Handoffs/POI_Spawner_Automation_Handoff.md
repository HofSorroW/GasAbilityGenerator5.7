# POI & NPC Spawner Automation Handoff

**Version:** 1.0
**Date:** January 2026
**Purpose:** Enable placing POIs, NPCSpawners, and spawning/moving NPCs from YAML manifests or Claude Code

---

## Executive Summary

This document specifies automation for:
1. **Placing POIActors** at coordinates with tags and properties
2. **Placing NPCSpawners** at coordinates with NPC definitions
3. **Spawning NPCs** near POIs or at specific coordinates
4. **Moving NPCs** to POIs or coordinates via goals

---

## Part 1: Narrative Pro System Analysis

### 1.1 POIActor Class

**Header:** `NarrativeArsenal/Public/Navigation/POIActor.h`

```cpp
UCLASS()
class APOIActor : public ATargetPoint
{
    // Tag identifying this POI (Narrative.POIs.*)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Narrative.POIs"))
    FGameplayTag POITag;

    // Whether to create a map marker
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bCreateMapMarker;

    // Whether players can fast travel here
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bSupportsFastTravel;

    // Display name for UI
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText POIDisplayName;

    // Map icon
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UTexture2D> POIIcon;

    // Linked POIs for AI navigation graph (A* pathfinding)
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<TSoftObjectPtr<APOIActor>> LinkedPOIs;

    // Fast travel spawn point
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TObjectPtr<UCapsuleComponent> FastTravelCapsule;
};
```

**Key Points:**
- POIs are placed in levels as actors
- `POITag` uses `Narrative.POIs.*` tag hierarchy
- `LinkedPOIs` creates navigation graph for long-range AI movement
- POIs are cached in `UNavigationSubsystem::POILookupMap`

### 1.2 FPOIData Struct

**Header:** `NarrativeArsenal/Public/Navigation/MapTileBounds.h`

```cpp
USTRUCT(BlueprintType)
struct FPOIData
{
    FGameplayTag POITag;                    // POI identifier
    TArray<FGameplayTag> LinkedPOIs;        // Navigation graph links
    FVector POILocation;                    // World location
    FTransform POIFastTravelSpot;           // Fast travel destination
    bool bNeedsMapMarker;                   // Show on map
    bool bSupportsFastTravel;               // Allow teleporting
    bool bIsDiscoverable;                   // Requires discovery
    TObjectPtr<UTexture2D> MapMarkerIcon;   // Map icon
    FText POIDisplayName;                   // UI name
    FText POISubtitle;                      // UI subtitle
};
```

### 1.3 NPCSpawner System

**Headers:**
- `NarrativeArsenal/Public/Spawners/SpawnerBase.h`
- `NarrativeArsenal/Public/Spawners/NPCSpawner.h`
- `NarrativeArsenal/Public/Spawners/NPCSpawnComponent.h`

```cpp
// Base spawner actor - world-partitioned
UCLASS()
class ASpawnerBase : public AActor, public INarrativeSavableActor
{
    USceneComponent* SpawnerRoot;
    FGuid SpawnerSaveGUID;          // Save system ID
    bool bActivateOnBeginPlay;      // Auto-spawn on level start

    virtual void SpawnActors();
    virtual void RemoveActors();
};

// NPC-specific spawner
UCLASS(Blueprintable, Placeable)
class ANPCSpawner : public ASpawnerBase
{
    void GetSpawnedNPCs(TArray<ANarrativeNPCCharacter*>& OutNPCs);

#if WITH_EDITOR
    UNPCSpawnComponent* CreateNPCSpawner();
#endif
};

// Component that spawns a single NPC
UCLASS()
class UNPCSpawnComponent : public USpawnComponent
{
    // NPC to spawn
    UPROPERTY(EditAnywhere, SaveGame)
    TObjectPtr<UNPCDefinition> NPCToSpawn;

    // Optional spawn parameter overrides
    UPROPERTY(EditAnywhere, SaveGame)
    FNPCSpawnParams SpawnParams;

    // Optional goal to assign on spawn
    UPROPERTY(Instanced, EditAnywhere)
    TObjectPtr<UNPCGoalItem> OptionalGoal;

    // NPC save GUID for persistence
    UPROPERTY(EditAnywhere, SaveGame)
    FGuid NPCSaveGUID;

    // Don't respawn if NPC was killed
    UPROPERTY(EditAnywhere, SaveGame)
    bool bDontSpawnIfPreviouslyKilled;

    // Distance from player before untethering
    UPROPERTY(EditAnywhere)
    float UntetherDistance;

    ANarrativeNPCCharacter* GetSpawnedNPC() const;
};
```

### 1.4 FNPCSpawnParams Struct

**Header:** `NarrativeArsenal/Public/UnrealFramework/NarrativeNPCCharacter.h`

```cpp
USTRUCT(BlueprintType)
struct FNPCSpawnParams
{
    // Override NPC name
    uint8 bOverride_NPCName : 1;
    FText NPCName;

    // Override level range
    uint8 bOverride_LevelRange : 1;
    int32 MinLevel = 1;
    int32 MaxLevel = 1;

    // Override factions
    uint8 bOverride_DefaultFactions : 1;
    FGameplayTagContainer DefaultFactions;

    // Override owned tags
    uint8 bOverride_DefaultOwnedTags : 1;
    FGameplayTagContainer DefaultOwnedTags;

    // Override activity configuration
    uint8 bOverride_ActivityConfiguration : 1;
    TObjectPtr<UActivityConfiguration> ActivityConfiguration;

    // Override item loadout
    uint8 bOverride_DefaultItemLoadout : 1;
    FDefaultItemLoadout DefaultItemLoadout;

    // Override appearance
    uint8 bOverride_DefaultAppearance : 1;
    TObjectPtr<UObject> DefaultAppearance;

    // Override trigger sets
    uint8 bOverride_TriggerSets : 1;
    TArray<UNPCTriggerSet*> TriggerSets;

    // Override random seed
    uint8 bOverride_CharacterRandomSeed : 1;
    int32 CharacterRandomSeed = -1;

    // Optional idle sequence
    TObjectPtr<ULevelSequence> OptionalIdleSequence;
};
```

### 1.5 NPC Subsystems

**UNarrativeCharacterSubsystem** - World subsystem for NPC management

```cpp
UCLASS()
class UNarrativeCharacterSubsystem : public UWorldSubsystem
{
    // Spawn NPC at transform
    UFUNCTION(BlueprintCallable)
    ANarrativeNPCCharacter* SpawnNPC(
        UNPCDefinition* NPCData,
        FTransform Transform = FTransform(),
        FNPCSpawnParams SpawnParams = FNPCSpawnParams()
    );

    // Find existing or spawn new
    UFUNCTION(BlueprintCallable)
    ANarrativeNPCCharacter* FindOrSpawnNPC(
        UNPCDefinition* NPCData,
        FTransform Transform = FTransform()
    );

    // Find NPC by ID (efficient TMap lookup)
    UFUNCTION(BlueprintCallable)
    ANarrativeNPCCharacter* FindNPCByID(const FName& NPCID) const;

    // Find NPC by definition
    ANarrativeNPCCharacter* FindNPC(const UNPCDefinition* NPCData) const;

    // Destroy NPC
    UFUNCTION(BlueprintCallable)
    bool DestroyNPC(ANarrativeNPCCharacter* NPC);

    // Event when NPC spawned
    UPROPERTY(BlueprintAssignable)
    FOnRequestedNPCSpawned OnNPCSpawned;
};
```

**UNavigationSubsystem** - World subsystem for POI management

```cpp
UCLASS()
class UNavigationSubsystem : public UWorldSubsystem
{
    // Get POI by tag
    UFUNCTION(BlueprintCallable)
    bool GetPointOfInterest(
        FPOIData& OutPointOfInterest,
        const FGameplayTag& POITag
    );

    // Quick lookup map
    UPROPERTY(BlueprintReadOnly)
    TMap<FGameplayTag, FPOIData> POILookupMap;
};
```

### 1.6 Provider System

**Transform Providers** - Get locations from various sources

```cpp
// Get transform from POI tag
UCLASS(DisplayName = "Point of Interest (Transform)")
class UNarrativeTransformProvider_POI : public UNarrativeTransformProvider
{
    UPROPERTY(EditAnywhere, meta = (Categories = "Narrative.POIs"))
    FGameplayTag POITag;

    virtual FTransform ProvideTransform_Implementation(const UObject* WorldContextObject) const override;
};

// Hardcoded transform
UCLASS(DisplayName = "Specified Transform")
class UNarrativeTransformProvider_SpecifiedTransform : public UNarrativeTransformProvider
{
    UPROPERTY(EditAnywhere)
    FTransform SpecifiedTransform;
};

// Find NPC by definition
UCLASS(DisplayName = "Find NPC")
class UNarrativeActorProvider_NPC : public UNarrativeActorProvider
{
    UPROPERTY(EditAnywhere)
    TObjectPtr<UNPCDefinition> NPCDefinition;
};
```

### 1.7 Goal System for NPC Movement

**Existing Assets:**
- `Goal_MoveToDestination` - `/Game/NarrativePro/Pro/Core/AI/Activities/GoToLocation/`
- `BPA_MoveToDestination` - Activity for moving to location
- `BT_MoveToDestination` - Behavior tree for movement
- `BB_GoToLocation` - Blackboard with destination key

**UNarrativeEvent_AddGoalToNPC** - Add goals via narrative events

```cpp
UCLASS(DisplayName="AI: Add Goal To NPC")
class UNarrativeEvent_AddGoalToNPC : public UNarrativeEvent
{
    UPROPERTY(Instanced, EditDefaultsOnly)
    TObjectPtr<UNPCGoalItem> GoalToAdd;
};
```

### 1.8 Long-Range Navigation

**UAITask_LongMove** - Uses POI links for A* pathfinding across world partition

```cpp
UCLASS()
class UAITask_LongMove : public UAITask
{
    // Run long-range move using POI graph
    UFUNCTION(BlueprintCallable, DisplayName = "Long Range Move To Location")
    static UAITask_LongMove* RunLongMove(
        AAIController* Controller,
        FVector GoalLocation,
        float AcceptanceRadius = -1.f,
        EAIOptionFlag::Type StopOnOverlap = EAIOptionFlag::Default,
        EAIOptionFlag::Type AcceptPartialPath = EAIOptionFlag::Default,
        bool bLockAILogic = true,
        EAIOptionFlag::Type ProjectGoalOnNavigation = EAIOptionFlag::Default,
        EAIOptionFlag::Type RequireNavigableEndLocation = EAIOptionFlag::Default
    );

    // A* pathfinding between POIs
    virtual bool CalculatePath(
        const FPOIData* StartingPOI,
        const FPOIData* EndingPOI,
        TArray<const FPOIData*>& OutPath
    );

    UPROPERTY(BlueprintAssignable)
    FMoveTaskCompleted OnMoveFinished;
};
```

---

## Part 2: Proposed Automation Features

### 2.1 POI Placement from YAML

```yaml
# manifest.yaml

# Define POI tags first
tags:
  - tag: Narrative.POIs.Town.Blacksmith
    comment: Blacksmith shop location
  - tag: Narrative.POIs.Town.Tavern
    comment: Town tavern
  - tag: Narrative.POIs.Town.Market
    comment: Market square

# POI placements (runtime commandlet creates level actors)
poi_placements:
  - poi_tag: Narrative.POIs.Town.Blacksmith
    location: [1200.0, 3400.0, 100.0]        # X, Y, Z
    rotation: [0.0, 0.0, 90.0]               # Roll, Pitch, Yaw
    display_name: "Garrett's Forge"
    create_map_marker: true
    supports_fast_travel: true
    map_icon: /Game/UI/Icons/T_Blacksmith
    # Link to nearby POIs for AI navigation graph
    linked_pois:
      - Narrative.POIs.Town.Tavern
      - Narrative.POIs.Town.Market

  - poi_tag: Narrative.POIs.Town.Tavern
    location: [800.0, 2800.0, 100.0]
    display_name: "The Rusty Anchor"
    create_map_marker: true
    supports_fast_travel: false
    linked_pois:
      - Narrative.POIs.Town.Blacksmith
```

### 2.2 NPCSpawner Placement from YAML

```yaml
# NPC spawner placements
npc_spawner_placements:
  - name: Spawner_TownGuard_Gate
    location: [500.0, 1000.0, 100.0]
    rotation: [0.0, 0.0, 180.0]
    activate_on_begin_play: true
    npcs:
      - npc_definition: NPCDef_TownGuard
        spawn_params:
          override_level_range: true
          min_level: 5
          max_level: 10
        dont_spawn_if_killed: true
        optional_goal: Goal_Patrol

  - name: Spawner_Blacksmith
    location: [1200.0, 3400.0, 100.0]   # Same as POI
    near_poi: Narrative.POIs.Town.Blacksmith  # Or use POI location
    npcs:
      - npc_definition: NPCDef_Blacksmith
        spawn_params:
          override_owned_tags: true
          default_owned_tags: [State.Invulnerable]
```

### 2.3 Runtime NPC Spawning Commands

```yaml
# Direct NPC spawn commands (for quests, events, etc.)
npc_spawn_commands:
  - command: spawn_npc
    npc_definition: NPCDef_Messenger
    location: [100.0, 200.0, 0.0]
    # OR
    near_poi: Narrative.POIs.Town.Tavern
    poi_offset: [50.0, 0.0, 0.0]          # Offset from POI
    spawn_params:
      override_npc_name: true
      npc_name: "Royal Messenger"

  - command: move_npc_to_poi
    npc_definition: NPCDef_Blacksmith
    target_poi: Narrative.POIs.Town.Market
    goal_score: 100.0                      # Priority score

  - command: move_npc_to_location
    npc_definition: NPCDef_Guard
    target_location: [800.0, 1200.0, 100.0]
```

---

## Part 3: Implementation Specifications

### 3.1 New Manifest Structs

```cpp
// GasAbilityGeneratorTypes.h

// POI placement definition
USTRUCT()
struct FManifestPOIPlacement
{
    GENERATED_BODY()

    UPROPERTY()
    FString POITag;                         // Narrative.POIs.*

    UPROPERTY()
    FVector Location;

    UPROPERTY()
    FRotator Rotation;

    UPROPERTY()
    FString DisplayName;

    UPROPERTY()
    bool bCreateMapMarker = true;

    UPROPERTY()
    bool bSupportsFastTravel = false;

    UPROPERTY()
    FString MapIcon;                        // Texture path

    UPROPERTY()
    TArray<FString> LinkedPOIs;             // Other POI tags
};

// NPC spawn entry for spawner
USTRUCT()
struct FManifestNPCSpawnEntry
{
    GENERATED_BODY()

    UPROPERTY()
    FString NPCDefinition;                  // NPCDef_* name

    UPROPERTY()
    FManifestNPCSpawnParams SpawnParams;

    UPROPERTY()
    bool bDontSpawnIfKilled = false;

    UPROPERTY()
    FString OptionalGoal;                   // Goal_* class name

    UPROPERTY()
    float UntetherDistance = 5000.0f;
};

// NPC spawner placement
USTRUCT()
struct FManifestNPCSpawnerPlacement
{
    GENERATED_BODY()

    UPROPERTY()
    FString Name;                           // Spawner_*

    UPROPERTY()
    FVector Location;

    UPROPERTY()
    FRotator Rotation;

    UPROPERTY()
    FString NearPOI;                        // Optional: use POI location

    UPROPERTY()
    bool bActivateOnBeginPlay = true;

    UPROPERTY()
    TArray<FManifestNPCSpawnEntry> NPCs;
};

// Runtime spawn command
USTRUCT()
struct FManifestNPCSpawnCommand
{
    GENERATED_BODY()

    UPROPERTY()
    FString CommandType;                    // spawn_npc, move_npc_to_poi, etc.

    UPROPERTY()
    FString NPCDefinition;

    UPROPERTY()
    FVector Location;

    UPROPERTY()
    FString NearPOI;

    UPROPERTY()
    FVector POIOffset;

    UPROPERTY()
    FString TargetPOI;                      // For move commands

    UPROPERTY()
    FVector TargetLocation;                 // For move commands

    UPROPERTY()
    float GoalScore = 100.0f;

    UPROPERTY()
    FManifestNPCSpawnParams SpawnParams;
};
```

### 3.2 New Generator Classes

```cpp
// GasAbilityGeneratorGenerators.h

// POI placement generator (creates level actors)
class FPOIPlacementGenerator
{
public:
    static bool Generate(
        const FManifestPOIPlacement& Definition,
        const FString& ProjectRoot,
        UWorld* World                        // Requires world context
    );

private:
    // Spawn APOIActor in world at location
    static APOIActor* SpawnPOIActor(
        UWorld* World,
        const FManifestPOIPlacement& Definition
    );

    // Update AMapTileBounds POI cache
    static void UpdatePOICache(
        UWorld* World,
        APOIActor* POI
    );
};

// NPC Spawner placement generator
class FNPCSpawnerPlacementGenerator
{
public:
    static bool Generate(
        const FManifestNPCSpawnerPlacement& Definition,
        const FString& ProjectRoot,
        UWorld* World
    );

private:
    // Spawn ANPCSpawner with configured components
    static ANPCSpawner* SpawnNPCSpawner(
        UWorld* World,
        const FManifestNPCSpawnerPlacement& Definition
    );

    // Add UNPCSpawnComponent for each NPC entry
    static void AddSpawnComponents(
        ANPCSpawner* Spawner,
        const TArray<FManifestNPCSpawnEntry>& NPCs
    );
};

// Runtime NPC command executor
class FNPCCommandExecutor
{
public:
    // Execute spawn/move commands
    static bool ExecuteCommand(
        const FManifestNPCSpawnCommand& Command,
        UWorld* World
    );

private:
    // Spawn NPC at location
    static ANarrativeNPCCharacter* SpawnNPC(
        UWorld* World,
        UNPCDefinition* NPCDef,
        const FTransform& Transform,
        const FNPCSpawnParams& Params
    );

    // Add goal to move NPC to location
    static bool MoveNPCToLocation(
        ANarrativeNPCCharacter* NPC,
        const FVector& TargetLocation,
        float GoalScore
    );

    // Add goal to move NPC to POI
    static bool MoveNPCToPOI(
        ANarrativeNPCCharacter* NPC,
        const FGameplayTag& POITag,
        float GoalScore
    );
};
```

### 3.3 Claude Code Conversation Patterns

**Intent Detection:**

```cpp
enum class EPOISpawnerIntent
{
    PlacePOI,           // "place a POI at 100,200,0 called Blacksmith"
    PlaceSpawner,       // "create a spawner for guards at the gate"
    SpawnNPC,           // "spawn the blacksmith near the forge POI"
    MoveNPC,            // "move the guard to the market"
    LinkPOIs,           // "link the blacksmith POI to the tavern"
    FindPOI,            // "where is the blacksmith POI?"
    FindNPC,            // "where is the blacksmith NPC?"
    Unknown
};
```

**Example Conversations:**

```
User: place a POI at coordinates 1200, 3400, 100 called "Blacksmith Forge" with tag Narrative.POIs.Town.Blacksmith

Claude: I'll create a POI placement for the Blacksmith Forge.

POI Created:
- Tag: Narrative.POIs.Town.Blacksmith
- Location: (1200, 3400, 100)
- Display Name: "Blacksmith Forge"
- Map Marker: Yes
- Fast Travel: No (default)

Would you like to:
1. Enable fast travel for this POI
2. Link it to other POIs for NPC navigation
3. Create an NPC spawner at this location
```

```
User: spawn the blacksmith NPC near the forge POI

Claude: I'll spawn NPCDef_Blacksmith near POI Narrative.POIs.Town.Blacksmith.

Looking up POI location... Found at (1200, 3400, 100)

Spawning NPC:
- Definition: NPCDef_Blacksmith
- Location: (1250, 3400, 100) [50 units offset]
- Spawn Params: Using defaults from NPCDefinition

NPC spawned successfully. The blacksmith is now at the forge.
```

```
User: move the town guard to the market POI

Claude: I'll add a movement goal to the town guard.

Finding NPC: NPCDef_TownGuard... Found
Finding POI: Narrative.POIs.Town.Market... Location: (800, 2800, 100)

Adding Goal_MoveToDestination:
- Target: (800, 2800, 100)
- Score: 100 (high priority)

The guard is now moving to the market.
```

### 3.4 Editor UI Extension

Add to SGasAbilityGeneratorWindow:

```
+----------------------------------------------------------+
| POI & Spawner Placement                                  |
+----------------------------------------------------------+
| [POI Placement] [Spawner Placement] [NPC Commands]       |
+----------------------------------------------------------+
| POI Tag: [Narrative.POIs.Town.______]                    |
|                                                          |
| Location:                                                |
| X: [1200.0____] Y: [3400.0____] Z: [100.0_____]         |
|                                                          |
| Display Name: [Blacksmith Forge_______]                  |
|                                                          |
| [x] Create Map Marker                                    |
| [ ] Supports Fast Travel                                 |
| Map Icon: [________________________] [Browse]            |
|                                                          |
| Linked POIs:                                             |
| [+] [Narrative.POIs.Town.Tavern        ] [-]            |
| [+] [Narrative.POIs.Town.Market        ] [-]            |
|                                                          |
| [Place POI in Level]                                     |
+----------------------------------------------------------+
```

---

## Part 4: Technical Considerations

### 4.1 World Context Requirement

Unlike asset generators, POI and Spawner placement requires a loaded world:
- **Editor Mode:** Use `GEditor->GetEditorWorldContext().World()`
- **Commandlet Mode:** May need to load a specific level
- **PIE Mode:** Use `GEngine->GetWorldFromContextObject()`

```cpp
UWorld* GetTargetWorld()
{
#if WITH_EDITOR
    if (GEditor)
    {
        return GEditor->GetEditorWorldContext().World();
    }
#endif
    return nullptr;
}
```

### 4.2 Level Streaming / World Partition

POIs and Spawners work with World Partition:
- Actors placed in persistent level or streaming cells
- `LinkedPOIs` enable navigation when navmesh isn't fully loaded
- Spawners respect streaming - spawn when cell loads

### 4.3 Save System Integration

Both POIs and Spawners integrate with Narrative save system:
- `SpawnerSaveGUID` persists spawner state
- `NPCSaveGUID` persists individual NPC state
- `bDontSpawnIfPreviouslyKilled` prevents respawning killed NPCs

### 4.4 Subsystem Access

```cpp
// Get navigation subsystem for POI lookup
UNavigationSubsystem* NavSub = World->GetSubsystem<UNavigationSubsystem>();
FPOIData POIData;
if (NavSub->GetPointOfInterest(POIData, POITag))
{
    // Use POIData.POILocation
}

// Get character subsystem for NPC spawning
UNarrativeCharacterSubsystem* CharSub = World->GetSubsystem<UNarrativeCharacterSubsystem>();
ANarrativeNPCCharacter* NPC = CharSub->SpawnNPC(NPCDef, Transform, SpawnParams);
```

---

## Part 5: Implementation Priority

### Phase 1: POI Tag Registration
1. Parse `poi_placements` from manifest
2. Auto-register POI tags in DefaultGameplayTags.ini
3. Store POI data for later placement

### Phase 2: Editor UI for Manual Placement
1. Add POI/Spawner tabs to generator window
2. Coordinate input fields
3. "Place in Level" button that spawns actor

### Phase 3: NPC Spawner Generator
1. Create ANPCSpawner actor
2. Add UNPCSpawnComponent for each NPC entry
3. Configure spawn parameters and goals

### Phase 4: Runtime Commands
1. Spawn NPC at location/POI
2. Move NPC to location/POI via goals
3. Query NPC/POI locations

### Phase 5: Claude Code Integration
1. Intent detection for POI/Spawner commands
2. Coordinate extraction from natural language
3. Interactive placement workflow

---

## Part 6: Testing Checklist

- [ ] POI tag auto-registration
- [ ] POI placement in editor world
- [ ] POI navigation subsystem caching
- [ ] NPCSpawner creation with components
- [ ] NPC spawn at coordinates
- [ ] NPC spawn near POI with offset
- [ ] Goal_MoveToDestination assignment
- [ ] Long-range navigation via LinkedPOIs
- [ ] World Partition streaming behavior
- [ ] Save/load spawner state
- [ ] Killed NPC respawn prevention
- [ ] Editor UI POI tab
- [ ] Editor UI Spawner tab
- [ ] Claude Code POI placement
- [ ] Claude Code NPC commands

---

## References

### Narrative Pro Headers
- `NarrativeArsenal/Public/Navigation/POIActor.h`
- `NarrativeArsenal/Public/Navigation/NavigationSubsystem.h`
- `NarrativeArsenal/Public/Navigation/MapTileBounds.h` (FPOIData)
- `NarrativeArsenal/Public/Navigation/NarrativeNavigationComponent.h`
- `NarrativeArsenal/Public/Spawners/SpawnerBase.h`
- `NarrativeArsenal/Public/Spawners/NPCSpawner.h`
- `NarrativeArsenal/Public/Spawners/NPCSpawnComponent.h`
- `NarrativeArsenal/Public/AI/NarrativeCharacterSubsystem.h`
- `NarrativeArsenal/Public/NarrativeActorProvider.h`
- `NarrativeArsenal/Public/AI/Activities/NPCGoalItem.h`
- `NarrativeArsenal/Public/AI/AITask_LongMove.h`

### Narrative Pro Content
- `/Game/NarrativePro/Pro/Core/AI/Activities/GoToLocation/` - Movement activity
- `Goal_MoveToDestination.uasset`
- `BPA_MoveToDestination.uasset`
- `BT_MoveToDestination.uasset`
- `BB_GoToLocation.uasset`
