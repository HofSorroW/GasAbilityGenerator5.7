# UE5.6 Programmatic Asset Creation APIs for Plugin Development

Complete working assets in Unreal Engine 5.6 require manipulating graph structures, factory patterns, and specialized APIs beyond simple UPROPERTY assignment. This report documents the critical C++ APIs for creating Behavior Trees, Animation Montages, Widget Blueprints, Niagara systems, Gameplay Cues, and other complex asset types programmatically—enabling the Father Ability Generator Plugin to advance from structure-level generation to fully functional asset output.

## Behavior Trees use parent-child arrays, not pins

Behavior Trees differ fundamentally from Blueprint graphs by using **FBTCompositeChild** arrays rather than pin connections. The runtime classes in `AIModule` provide everything needed for programmatic creation without touching editor-only graph APIs.

**Core creation pattern** involves creating the asset, populating Blackboard keys, building the node hierarchy, and connecting children via array manipulation:

```cpp
// Create BT and Blackboard
UBehaviorTree* BT = NewObject<UBehaviorTree>(Package, AssetName, RF_Public | RF_Standalone);
BT->BlackboardAsset = NewObject<UBlackboardData>(BT, TEXT("BlackboardData"));

// Add Blackboard keys
FBlackboardEntry Entry;
Entry.EntryName = FName("TargetActor");
Entry.KeyType = NewObject<UBlackboardKeyType_Object>(BT->BlackboardAsset);
BT->BlackboardAsset->Keys.Add(Entry);

// Create root composite
BT->RootNode = NewObject<UBTComposite_Selector>(BT, TEXT("Root"));

// Add children via FBTCompositeChild (NOT pins)
UBTTask_MoveTo* Task = NewObject<UBTTask_MoveTo>(BT->RootNode);
FBTCompositeChild ChildEntry;
ChildEntry.ChildTask = Task;
BT->RootNode->Children.Add(ChildEntry);
```

**Decorators attach to child connections**, not nodes directly. Add them to `FBTCompositeChild.Decorators` array. **Services attach to composite nodes** via the `UBTCompositeNode.Services` array. Protected properties like `BlackboardKey` require reflection-based setters using `FStructProperty::SetValue_InContainer()`.

Key headers: `BehaviorTree/BehaviorTree.h`, `BehaviorTree/Composites/BTComposite_Selector.h`, `BehaviorTree/Tasks/BTTask_MoveTo.h`

## Animation Montage notifies require Link() for timing

Adding notifies to montages requires the critical `FAnimNotifyEvent::Link()` call to establish trigger timing within the animation timeline:

```cpp
int32 Index = Montage->Notifies.Add(FAnimNotifyEvent());
FAnimNotifyEvent& Event = Montage->Notifies[Index];
Event.NotifyName = FName("MyNotify");
Event.Guid = FGuid::NewGuid();  // Required for editor stability
Event.Link(Montage, TriggerTimeInSeconds);  // CRITICAL - sets timing

// For UAnimNotifyState (duration-based)
Event.NotifyStateClass = NewObject<UMyAnimNotifyState>(Montage);
Event.SetDuration(DurationInSeconds);
Event.EndLink.Link(Montage, Event.EndLink.GetTime());
```

**Montage sections** use `FCompositeSection` with `NextSectionName` for branching:

```cpp
FCompositeSection Section;
Section.SectionName = FName("Attack1");
Section.StartTime = 0.f;
Section.NextSectionName = FName("Attack2");  // Branch target
Montage->CompositeSections.Add(Section);
```

**Slot tracks** configure via `FSlotAnimationTrack` and `FAnimSegment` for animation assignment. **Blend settings** use `FAlphaBlend` with `SetBlendTime()` and `SetBlendOption()` (Linear, Cubic, Sinusoidal, etc.).

## Widget Blueprints require WidgetTree::ConstructWidget

The `UWidgetTree::ConstructWidget<T>()` method is the **only correct way** to create widgets for Widget Blueprints in editor context:

```cpp
UWidgetBlueprint* Blueprint = ...; // Get from package
UCanvasPanel* Root = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
    UCanvasPanel::StaticClass(), FName("RootCanvas"));
Blueprint->WidgetTree->RootWidget = Root;

// Add children through slot APIs
UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
    UTextBlock::StaticClass(), FName("MyText"));
UPanelSlot* Slot = Root->AddChild(Text);

// Configure slot (Canvas-specific)
if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
{
    CanvasSlot->SetPosition(FVector2D(100.f, 100.f));
    CanvasSlot->SetSize(FVector2D(200.f, 50.f));
    CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
}

// Mark modified
FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
```

Container types have specialized slots: `UCanvasPanelSlot`, `UHorizontalBoxSlot`, `UVerticalBoxSlot`, `UOverlaySlot`. Each provides type-specific layout properties.

**UE5.6 addition**: `UWidgetBlueprintExtension` class enables custom per-blueprint compilation logic and data storage.

## Dialogue graphs use custom UEdGraphNode, not UK2Node

Dialogue systems should subclass `UEdGraphNode` directly (not `UK2Node`, which is for Blueprint compilation). The pattern involves:

- **Custom UEdGraphNode** subclass for dialogue line/choice nodes
- **Custom UEdGraphSchema** for connection rules and context menus
- **Graph panel factories** for visual customization

```cpp
// Create node programmatically
UDialogueNode* Node = NewObject<UDialogueNode>(ParentGraph);
Node->CreateNewGuid();
Node->NodePosX = 200.f;
Node->AllocateDefaultPins();
ParentGraph->AddNode(Node, true, true);

// Connect nodes via pins
OutputPin->MakeLinkTo(InputPin);
```

The **FlowGraph plugin** (MIT license, UE5.6 compatible) provides a production-ready foundation: each node is a single UObject, nodes are async/latent by design, and it's design-agnostic. This is the recommended base for custom dialogue implementations.

## Niagara systems have limited programmatic creation support

**Critical limitation**: Full programmatic creation of Niagara emitters/systems from scratch is **not well-supported** in C++. Epic's recommended approaches:

1. **Template duplication**: Create base systems in editor, duplicate and modify at runtime
2. **Parameter-driven effects**: Use exposed User Parameters for runtime customization
3. **Python scripting**: Use Niagara's Python API for editor automation

Runtime parameter setting is fully supported:

```cpp
UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(...);
NC->SetVariableFloat(FName("MyFloat"), 1.5f);
NC->SetVariableVec3(FName("MyVector"), FVector(1, 2, 3));
NC->SetVariableLinearColor(FName("MyColor"), FLinearColor::Red);
```

For plugin asset generation, the practical approach is creating Niagara **parameter presets** rather than full system generation.

## Gameplay Cues have two distinct base classes

**UGameplayCueNotify_Static** (UObject-based) handles **burst effects**—hit impacts, instant VFX. No state, no tick, fire-and-forget:

```cpp
virtual bool HandleGameplayCue(AActor* Target, EGameplayCueEvent::Type EventType, 
    const FGameplayCueParameters& Params) override
{
    if (EventType == EGameplayCueEvent::Executed)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ParticleSystem, Params.Location);
    }
    return true;
}
```

**AGameplayCueNotify_Actor** (Actor-based) handles **looping/persistent effects**—buffs, shields, DOTs. Has state, ticks, and lifecycle events (OnActive, WhileActive, OnRemove).

**Tag configuration** requires tags starting with `GameplayCue.` hierarchy:

```cpp
// In constructor
GameplayCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Combat.HitImpact"));

// Or use native tag macros
UE_DEFINE_GAMEPLAY_TAG(TAG_GC_HitImpact, "GameplayCue.Combat.HitImpact");
```

Asset naming convention: `GC_` prefix (e.g., `GC_HitImpact`).

## AI Perception configures through FAISenseConfig subclasses

Create sense configs in constructor, then register with `ConfigureSense()`:

```cpp
// In AIController constructor
PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));

// Sight configuration
UAISenseConfig_Sight* Sight = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
Sight->SightRadius = 1000.f;
Sight->LoseSightRadius = 1200.f;
Sight->PeripheralVisionAngleDegrees = 90.f;
Sight->DetectionByAffiliation.bDetectEnemies = true;
Sight->SetMaxAge(5.f);
PerceptionComponent->ConfigureSense(*Sight);

// Hearing configuration
UAISenseConfig_Hearing* Hearing = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
Hearing->HearingRange = 2000.f;
PerceptionComponent->ConfigureSense(*Hearing);

PerceptionComponent->SetDominantSense(Sight->GetSenseImplementation());
```

Available sense configs: `FAISenseConfig_Sight`, `FAISenseConfig_Hearing`, `FAISenseConfig_Damage`, `FAISenseConfig_Prediction`, `FAISenseConfig_Team`.

## Camera Shakes use pattern-based architecture in UE5

The modern system uses `UCameraShakeBase` with pluggable `UCameraShakePattern` subclasses:

```cpp
UMyCameraShake::UMyCameraShake() : UCameraShakeBase(FObjectInitializer::Get())
{
    UPerlinNoiseCameraShakePattern* Pattern = 
        CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(TEXT("Pattern"));
    
    Pattern->X.Amplitude = 10.f;
    Pattern->X.Frequency = 10.f;
    Pattern->Pitch.Amplitude = 10.f;
    Pattern->Duration = 2.f;
    
    SetRootShakePattern(Pattern);
}
```

Available patterns: `UPerlinNoiseCameraShakePattern`, `UWaveOscillatorCameraShakePattern`, `UCompositeCameraShakePattern`, `USequenceCameraShakePattern`. The legacy `UMatineeCameraShake` remains functional for simpler use cases.

## Data Assets use reflection for dynamic property population

For generator plugins, **UPrimaryDataAsset** provides Asset Manager integration. Property population uses `FProperty` reflection:

```cpp
// Find and set property via reflection
FProperty* Prop = Object->GetClass()->FindPropertyByName(FName("MyProperty"));
void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Object);

if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
    IntProp->SetPropertyValue(ValuePtr, 42);
else if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
    StrProp->SetPropertyValue(ValuePtr, FString("Value"));

// For arrays
if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
{
    FScriptArrayHelper Helper(ArrayProp, ValuePtr);
    Helper.AddValue();
    // Set element...
}
```

## Asset Factory patterns enable editor integration

The standard programmatic asset creation pipeline:

```cpp
// 1. Create package
UPackage* Package = CreatePackage(*PackagePath);

// 2. Create object
UMyAsset* Asset = NewObject<UMyAsset>(Package, AssetName, RF_Public | RF_Standalone);

// 3. Register with Asset Registry
FAssetRegistryModule::AssetCreated(Asset);

// 4. Mark and save
Package->MarkPackageDirty();
FSavePackageArgs SaveArgs;
SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
UPackage::SavePackage(Package, Asset, *PackageFileName, SaveArgs);
```

**IAssetTools** provides higher-level creation with automatic factory selection:

```cpp
IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
UObject* Asset = Tools.CreateAsset(AssetName, PackagePath, UMyAsset::StaticClass(), nullptr);
```

Specialized factories exist for each asset type: `UBehaviorTreeFactory`, `UWidgetBlueprintFactory`, `UAnimBlueprintFactory`, `UDataAssetFactory`.

## Module dependencies for plugin development

```cpp
// Build.cs
PublicDependencyModuleNames.AddRange(new string[] { 
    "Core", "CoreUObject", "Engine", "AIModule", "Niagara", "GameplayAbilities", "UMG" 
});

PrivateDependencyModuleNames.AddRange(new string[] { 
    "UnrealEd", "AssetTools", "BehaviorTreeEditor", "UMGEditor", "BlueprintGraph"
});
```

## Conclusion

Advancing the Father Ability Generator Plugin from structure-level to complete asset generation requires understanding that each asset type has **distinct architectural patterns**: Behavior Trees use array-based child connections, Widget Blueprints require `WidgetTree::ConstructWidget`, Animation Montages need `Link()` for timing, and Niagara systems are best handled through parameterized templates rather than full programmatic creation.

The key insight across all systems is that **editor context creation differs significantly from runtime creation**—assets must be properly packaged, registered with the Asset Registry, and marked dirty for persistence. Using the correct factory pattern (`IAssetTools::CreateAsset` or direct `NewObject` with package management) ensures generated assets integrate properly with Unreal's tooling ecosystem.

For dialogue graphs and other custom node-based systems, building on `UEdGraphNode` with custom schemas—or leveraging FlowGraph as a foundation—provides the most maintainable architecture. The reflection-based property system enables dynamic configuration of any UPROPERTY, making generated Data Assets fully customizable at creation time.