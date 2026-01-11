# Father Ability Generator Plugin Enhancement Research v2.0

## Unreal Engine 5.6 | Narrative Pro v2.2

---

## DOCUMENT INFORMATION

| Field | Value |
|-------|-------|
| Document Type | Research Analysis |
| Engine Version | Unreal Engine 5.6 |
| Plugin Version | Father Ability Generator v1.5.0 |
| Research Date | January 2026 |
| Version | 2.0 |
| Purpose | Research alternative approaches to enable full asset generation for assets currently marked as STRUCTURE only |

---

## EXECUTIVE SUMMARY

This research document analyzes the current limitations of the Father Ability Generator Plugin and investigates alternative approaches within Unreal Engine 5.6 to enhance asset generation capabilities. The goal is to move assets from "STRUCTURE only" (requiring manual logic) to "FULL" auto-generation status.

Version 2.0 includes expanded research on Behavior Trees, Widget Blueprint visual hierarchies, Animation Montage notifies/sections, Dialogue graphs, Niagara VFX, Gameplay Cues, AI Perception, Camera Shakes, and Data Asset patterns.

---

## CURRENT PLUGIN CAPABILITIES ANALYSIS

### Assets Currently at FULL Auto-Generation

| Asset Type | Count | What Plugin Creates |
|------------|-------|---------------------|
| Gameplay Tags | 174 | INI file entries |
| Input Actions | 6 | IA_ assets with value type |
| Input Mapping Context | 1 | IMC_ with key bindings |
| Float Curves | 1 | FC_ with key points |
| Enumerations | 2 | E_ assets with values |
| Gameplay Effects | 34 | GE_ with duration, modifiers, tags, components |
| Gameplay Abilities | 17 | GA_ with Class Defaults only (NO event graph logic) |
| EquippableItems | 4 | BP_ with properties, abilities, equipment slot |
| Item Collections | 1 | IC_ with item references |
| Configurations | 4 | AC_, ActConfig_, NPCDef_, CD_ |
| Behavior Trees | 2 | BT_ structure only |
| Blackboards | 1 | BB_ with key definitions |
| Activities | 1 | BPA_ with properties |
| Narrative Events | 2 | NE_ with event properties |

### Assets Currently at STRUCTURE Only

| Asset Type | Count | What Plugin Creates | What Is Missing |
|------------|-------|---------------------|-----------------|
| Actor Blueprints | 4 | BP_ with components/variables | Event graph logic |
| Widget Blueprints | 3 | WBP_ structure + bindings | Event graph logic, widget hierarchy |
| Animation Assets | 6 | Placeholder montages | Slot tracks, sections, notifies |
| Materials | 1 | Basic material setup | Material expression nodes, connections |
| Dialogue Assets | 1 | Dialogue flow structure | Dialogue graph nodes |
| Player Components | 1 | Component structure | Event graph logic |

---

## RESEARCH FINDINGS: PROGRAMMATIC BLUEPRINT EVENT GRAPH CREATION

### Key Discovery: Event Graph Logic CAN Be Created Programmatically

The research confirms that Unreal Engine provides C++ APIs to programmatically create and populate Blueprint Event Graphs.

### Required C++ APIs

| API | Purpose | Module |
|-----|---------|--------|
| FBlueprintEditorUtils::CreateNewGraph | Create new graph in Blueprint | Kismet |
| FBlueprintEditorUtils::AddUbergraphPage | Add graph as ubergraph page | Kismet |
| FGraphNodeCreator::CreateNode | Create nodes in graph | Engine/EdGraph |
| UK2Node_CallFunction | Create function call nodes | BlueprintGraph |
| UK2Node_Event | Create event nodes | BlueprintGraph |
| UEdGraphNode::FindPin | Get pin references | Engine/EdGraph |
| UEdGraphPin::MakeLinkTo | Connect pins together | Engine/EdGraph |
| FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified | Mark for recompilation | Kismet |
| FBlueprintEditorUtils::AddMemberVariable | Add variables to Blueprint | Kismet |

### Node Creation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | FGraphNodeCreator<UK2Node_CallFunction> Creator(*TargetGraph) | Prepare node creator |
| 2 | CallFuncNode = Creator.CreateNode() | Create the node instance |
| 3 | CallFuncNode->FunctionReference.SetExternalMember(FunctionName, Class) | Set function reference |
| 4 | Creator.Finalize() | Complete node creation |

### Pin Connection Pattern

| Connection Type | Code Pattern |
|-----------------|--------------|
| Execution pins | NodeA->GetThenPin()->MakeLinkTo(NodeB->GetExecPin()) |
| Data pins | NodeA->FindPin(TEXT("ReturnValue"), EGPD_Output)->MakeLinkTo(NodeB->FindPin(TEXT("InputParam"), EGPD_Input)) |
| Cast output | CastNode->FindPin(TEXT("As BP Father Companion"), EGPD_Output)->MakeLinkTo(TargetNode->FindPin(TEXT("Target"), EGPD_Input)) |

### Alternative Node Spawner Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | UFunction* Function = ParentClass->FindFunctionByName(FunctionName) | Get function reference |
| 2 | UBlueprintFunctionNodeSpawner* Spawner = UBlueprintFunctionNodeSpawner::Create(Function) | Create spawner |
| 3 | UEdGraphNode* Node = Spawner->Invoke(EventGraph, IBlueprintNodeBinder::FBindingSet(), Location) | Spawn node at location |

### Upgrade Potential: HIGH

Event graph generation enables FULL auto-generation for:
- 17 Gameplay Abilities (GA_*)
- 4 Actor Blueprints (BP_*)
- 3 Widget Blueprints (WBP_*) - event logic
- 1 Player Component

---

## RESEARCH FINDINGS: BEHAVIOR TREE PROGRAMMATIC CREATION

### Key Discovery: Behavior Trees Use FBTCompositeChild Arrays, NOT Pins

Behavior Trees differ fundamentally from Blueprint graphs by using FBTCompositeChild arrays rather than pin connections. The runtime classes in AIModule provide everything needed for programmatic creation.

### Required C++ APIs

| API | Purpose | Module |
|-----|---------|--------|
| UBehaviorTree | Main BT asset class | AIModule |
| UBlackboardData | Blackboard asset for keys | AIModule |
| FBlackboardEntry | Individual blackboard key definition | AIModule |
| UBTComposite_Selector | Selector composite node | AIModule |
| UBTComposite_Sequence | Sequence composite node | AIModule |
| FBTCompositeChild | Child connection structure (NOT pins) | AIModule |
| UBTTask_MoveTo | Example task node | AIModule |
| UBTDecorator_* | Decorator node classes | AIModule |
| UBTService_* | Service node classes | AIModule |

### BT Creation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | UBehaviorTree* BT = NewObject<UBehaviorTree>(Package, AssetName, RF_Public OR RF_Standalone) | Create BT asset |
| 2 | BT->BlackboardAsset = NewObject<UBlackboardData>(BT, TEXT("BlackboardData")) | Create embedded blackboard |
| 3 | FBlackboardEntry Entry; Entry.EntryName = FName("TargetActor") | Define blackboard key |
| 4 | Entry.KeyType = NewObject<UBlackboardKeyType_Object>(BT->BlackboardAsset) | Set key type |
| 5 | BT->BlackboardAsset->Keys.Add(Entry) | Add key to blackboard |
| 6 | BT->RootNode = NewObject<UBTComposite_Selector>(BT, TEXT("Root")) | Create root composite |
| 7 | UBTTask_MoveTo* Task = NewObject<UBTTask_MoveTo>(BT->RootNode) | Create task node |
| 8 | FBTCompositeChild ChildEntry; ChildEntry.ChildTask = Task | Create child connection |
| 9 | BT->RootNode->Children.Add(ChildEntry) | Add child to composite |

### Decorator and Service Attachment

| Attachment Type | Pattern | Target |
|-----------------|---------|--------|
| Decorators | FBTCompositeChild.Decorators.Add(DecoratorNode) | Attach to child connection |
| Services | UBTCompositeNode.Services.Add(ServiceNode) | Attach to composite node |

### Protected Property Access

| Property | Access Method |
|----------|---------------|
| BlackboardKey | FStructProperty::SetValue_InContainer() via reflection |
| NodeName | Direct assignment (public) |
| TreeAsset | Automatic via NewObject outer |

### Blackboard Key Types

| Key Type Class | UE Type |
|----------------|---------|
| UBlackboardKeyType_Object | UObject* |
| UBlackboardKeyType_Class | UClass* |
| UBlackboardKeyType_Bool | bool |
| UBlackboardKeyType_Float | float |
| UBlackboardKeyType_Int | int32 |
| UBlackboardKeyType_Vector | FVector |
| UBlackboardKeyType_Rotator | FRotator |
| UBlackboardKeyType_String | FString |
| UBlackboardKeyType_Name | FName |
| UBlackboardKeyType_Enum | uint8 |

### Upgrade Potential: HIGH (Previously UNCERTAIN)

Behavior Tree generation is NOW FULLY POSSIBLE:
- Node hierarchy via FBTCompositeChild arrays
- Decorator attachment via child connection arrays
- Service attachment via composite node arrays
- Blackboard key configuration via reflection

---

## RESEARCH FINDINGS: WIDGET BLUEPRINT VISUAL HIERARCHY CREATION

### Key Discovery: WidgetTree::ConstructWidget Enables Full Visual Hierarchy

The UWidgetTree::ConstructWidget<T>() method is the correct way to create widgets for Widget Blueprints in editor context. This was previously marked as NOT SOLVABLE but IS NOW FULLY POSSIBLE.

### Required C++ APIs

| API | Purpose | Module |
|-----|---------|--------|
| UWidgetBlueprint::WidgetTree | Access widget tree | UMGEditor |
| UWidgetTree::ConstructWidget<T>() | Create widget in tree | UMG |
| UWidgetTree::RootWidget | Set root widget | UMG |
| UPanelWidget::AddChild() | Add child to container | UMG |
| UCanvasPanelSlot | Canvas-specific slot | UMG |
| UHorizontalBoxSlot | HorizontalBox-specific slot | UMG |
| UVerticalBoxSlot | VerticalBox-specific slot | UMG |
| UOverlaySlot | Overlay-specific slot | UMG |
| FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified | Mark for recompilation | Kismet |

### Widget Creation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | UWidgetBlueprint* Blueprint = ... | Get widget blueprint |
| 2 | UCanvasPanel* Root = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName("RootCanvas")) | Create root canvas |
| 3 | Blueprint->WidgetTree->RootWidget = Root | Set as root |
| 4 | UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("MyText")) | Create child widget |
| 5 | UPanelSlot* Slot = Root->AddChild(Text) | Add to parent |
| 6 | UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot) | Get typed slot |
| 7 | CanvasSlot->SetPosition(FVector2D(100.f, 100.f)) | Configure position |
| 8 | CanvasSlot->SetSize(FVector2D(200.f, 50.f)) | Configure size |
| 9 | CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f)) | Configure anchors |
| 10 | FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint) | Mark modified |

### Container Widget Types

| Container Type | Slot Type | Layout Properties |
|----------------|-----------|-------------------|
| UCanvasPanel | UCanvasPanelSlot | Position, Size, Anchors, Alignment, ZOrder |
| UHorizontalBox | UHorizontalBoxSlot | Padding, Size, HAlign, VAlign |
| UVerticalBox | UVerticalBoxSlot | Padding, Size, HAlign, VAlign |
| UOverlay | UOverlaySlot | Padding, HAlign, VAlign |
| UUniformGridPanel | UUniformGridSlot | Row, Column |
| UGridPanel | UGridPanelSlot | Row, Column, RowSpan, ColumnSpan |
| UWidgetSwitcher | UWidgetSwitcherSlot | Padding, HAlign, VAlign |
| UScrollBox | UScrollBoxSlot | Padding, HAlign, VAlign |

### Common Widget Classes

| Widget Class | Purpose |
|--------------|---------|
| UTextBlock | Display text |
| UImage | Display image/texture |
| UButton | Clickable button |
| UProgressBar | Progress bar display |
| USlider | Slider input |
| UCheckBox | Checkbox input |
| UEditableText | Text input |
| USpacer | Layout spacing |
| UBorder | Border container |
| USizeBox | Size constraint container |

### Upgrade Potential: HIGH (Previously NOT SOLVABLE)

Widget Blueprint visual hierarchy generation is NOW FULLY POSSIBLE:
- Root widget assignment via WidgetTree->RootWidget
- Child widget creation via ConstructWidget<T>()
- Parent-child relationships via AddChild()
- Layout configuration via typed slot classes

---

## RESEARCH FINDINGS: ANIMATION MONTAGE PROGRAMMATIC CREATION

### Key Discovery: Notifies Require Link() for Timing

Adding notifies to montages requires the critical FAnimNotifyEvent::Link() call to establish trigger timing within the animation timeline.

### Required C++ APIs

| API | Purpose | Module |
|-----|---------|--------|
| UAnimMontage | Montage asset class | Engine |
| FAnimNotifyEvent | Notify event structure | Engine |
| FAnimNotifyEvent::Link() | CRITICAL - Sets trigger timing | Engine |
| FAnimNotifyEvent::SetDuration() | Set notify state duration | Engine |
| FCompositeSection | Montage section definition | Engine |
| FSlotAnimationTrack | Slot track configuration | Engine |
| FAnimSegment | Animation segment in slot | Engine |
| FAlphaBlend | Blend settings | Engine |

### Notify Creation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | int32 Index = Montage->Notifies.Add(FAnimNotifyEvent()) | Add notify to array |
| 2 | FAnimNotifyEvent& Event = Montage->Notifies[Index] | Get reference |
| 3 | Event.NotifyName = FName("MyNotify") | Set notify name |
| 4 | Event.Guid = FGuid::NewGuid() | REQUIRED for editor stability |
| 5 | Event.Link(Montage, TriggerTimeInSeconds) | CRITICAL - Sets timing |
| 6 | Event.Notify = NewObject<UAnimNotify>(Montage) | Create notify object (instant) |

### Notify State Creation Pattern (Duration-Based)

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1-5 | (Same as above) | Basic setup |
| 6 | Event.NotifyStateClass = NewObject<UMyAnimNotifyState>(Montage) | Create state object |
| 7 | Event.SetDuration(DurationInSeconds) | Set duration |
| 8 | Event.EndLink.Link(Montage, Event.EndLink.GetTime()) | Link end time |

### Section Creation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | FCompositeSection Section | Create section struct |
| 2 | Section.SectionName = FName("Attack1") | Set section name |
| 3 | Section.StartTime = 0.f | Set start time |
| 4 | Section.NextSectionName = FName("Attack2") | Set branch target |
| 5 | Montage->CompositeSections.Add(Section) | Add to montage |

### Slot Track Configuration

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | FSlotAnimationTrack SlotTrack | Create slot track |
| 2 | SlotTrack.SlotName = FName("DefaultSlot") | Set slot name |
| 3 | FAnimSegment Segment | Create segment |
| 4 | Segment.AnimReference = AnimSequence | Set animation reference |
| 5 | Segment.AnimStartTime = 0.f | Set start time |
| 6 | Segment.AnimEndTime = AnimSequence->GetPlayLength() | Set end time |
| 7 | SlotTrack.AnimTrack.AnimSegments.Add(Segment) | Add segment to track |
| 8 | Montage->SlotAnimTracks.Add(SlotTrack) | Add track to montage |

### Blend Settings

| Property | API | Options |
|----------|-----|---------|
| Blend In Time | BlendIn.SetBlendTime(0.25f) | Float seconds |
| Blend Out Time | BlendOut.SetBlendTime(0.25f) | Float seconds |
| Blend Option | SetBlendOption(EAlphaBlendOption::Linear) | Linear, Cubic, Sinusoidal, QuadraticInOut, etc. |

### Upgrade Potential: PARTIAL (Was NOT SOLVABLE for notifies/sections)

Animation Montage generation NOW INCLUDES:
- Notify creation with proper timing via Link()
- Notify State creation with duration
- Section creation with branching
- Slot track configuration
- Blend settings

STILL REQUIRES: Source Animation Sequences (external dependency)

---

## RESEARCH FINDINGS: MATERIAL PROGRAMMATIC CREATION

### Full Material Node Graph Creation IS Possible

| Feature | Can Create | API |
|---------|------------|-----|
| UMaterial asset | YES | UMaterialFactoryNew |
| Material expressions | YES | NewObject<UMaterialExpression*> |
| Expression connections | YES | Expression->Input.Connect() |
| Texture samplers | YES | UMaterialExpressionTextureSample |
| Math nodes | YES | UMaterialExpressionMultiply, etc. |
| Constants | YES | UMaterialExpressionConstant |
| Parameters | YES | UMaterialExpressionScalarParameter |

### Material Creation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | auto MaterialFactory = NewObject<UMaterialFactoryNew>() | Create factory |
| 2 | UMaterial* UnrealMaterial = (UMaterial*)MaterialFactory->FactoryCreateNew(...) | Create material |
| 3 | UMaterialExpressionConstant* Const = NewObject<UMaterialExpressionConstant>(UnrealMaterial) | Create expression |
| 4 | Const->R = 0.0f | Set value |
| 5 | UnrealMaterial->Expressions.Add(Const) | Add to material |
| 6 | UnrealMaterial->Specular.Expression = Const | Connect to output |

### Common Material Expression Classes

| Expression Class | Purpose |
|------------------|---------|
| UMaterialExpressionConstant | Single float constant |
| UMaterialExpressionConstant3Vector | RGB color constant |
| UMaterialExpressionConstant4Vector | RGBA color constant |
| UMaterialExpressionScalarParameter | Exposed float parameter |
| UMaterialExpressionVectorParameter | Exposed vector parameter |
| UMaterialExpressionTextureSample | Texture sampler |
| UMaterialExpressionMultiply | Multiplication |
| UMaterialExpressionAdd | Addition |
| UMaterialExpressionLerp | Linear interpolation |
| UMaterialExpressionFresnel | Fresnel effect |
| UMaterialExpressionTime | Time node |
| UMaterialExpressionPanner | UV panning |

### Material Output Pins

| Output | Property |
|--------|----------|
| Base Color | UnrealMaterial->BaseColor.Expression |
| Metallic | UnrealMaterial->Metallic.Expression |
| Specular | UnrealMaterial->Specular.Expression |
| Roughness | UnrealMaterial->Roughness.Expression |
| Emissive Color | UnrealMaterial->EmissiveColor.Expression |
| Opacity | UnrealMaterial->Opacity.Expression |
| Normal | UnrealMaterial->Normal.Expression |
| World Position Offset | UnrealMaterial->WorldPositionOffset.Expression |

### Upgrade Potential: HIGH

Materials CAN be fully auto-generated including:
- All expression nodes
- All node connections
- All parameters
- All texture references

---

## RESEARCH FINDINGS: DIALOGUE GRAPH PROGRAMMATIC CREATION

### Key Discovery: Dialogue Graphs Use Custom UEdGraphNode, NOT UK2Node

Dialogue systems should subclass UEdGraphNode directly (not UK2Node, which is for Blueprint compilation). The pattern involves custom node classes and custom graph schema.

### Architecture Pattern

| Component | Base Class | Purpose |
|-----------|------------|---------|
| Dialogue Node | UEdGraphNode | Individual dialogue line/choice |
| Dialogue Graph | UEdGraph | Contains all dialogue nodes |
| Dialogue Schema | UEdGraphSchema | Connection rules, context menus |
| Dialogue Editor | FAssetEditorToolkit | Custom editor UI |

### Node Creation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | UDialogueNode* Node = NewObject<UDialogueNode>(ParentGraph) | Create node |
| 2 | Node->CreateNewGuid() | Assign unique ID |
| 3 | Node->NodePosX = 200.f | Set position X |
| 4 | Node->NodePosY = 100.f | Set position Y |
| 5 | Node->AllocateDefaultPins() | Create pins |
| 6 | ParentGraph->AddNode(Node, true, true) | Add to graph |

### Pin Connection Pattern

| Connection Type | Code Pattern |
|-----------------|--------------|
| Output to Input | OutputPin->MakeLinkTo(InputPin) |
| Break connection | OutputPin->BreakLinkTo(InputPin) |
| Break all | Pin->BreakAllPinLinks() |

### FlowGraph Plugin Reference

FlowGraph (MIT license, UE5.6 compatible) provides production-ready foundation:
- Each node is single UObject
- Nodes are async/latent by design
- Design-agnostic architecture
- Recommended base for custom dialogue implementations

### Narrative Pro Dialogue Pattern

Narrative Pro's DialogueBlueprint uses similar custom node architecture:
- UDialogueGraphNode base class
- Custom UDialogueGraphSchema
- Node types: NPC, Player, Branch, Event

### Upgrade Potential: MEDIUM

Dialogue graph generation requires:
- Custom UEdGraphNode subclass creation
- Custom schema implementation
- Node factory registration

Can leverage existing Narrative Pro dialogue classes if available in headers.

---

## RESEARCH FINDINGS: NIAGARA VFX PROGRAMMATIC CREATION

### Key Discovery: Limited Programmatic Creation Support

Full programmatic creation of Niagara emitters/systems from scratch is NOT well-supported in C++. Epic recommends alternative approaches.

### Recommended Approaches

| Approach | Description | Feasibility |
|----------|-------------|-------------|
| Template Duplication | Create base systems in editor, duplicate and modify at runtime | HIGH |
| Parameter-Driven | Use exposed User Parameters for runtime customization | HIGH |
| Python Scripting | Use Niagara's Python API for editor automation | MEDIUM |
| Full Programmatic | Create emitters/modules from scratch in C++ | LOW |

### Runtime Parameter Setting

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(...) | Spawn system |
| 2 | NC->SetVariableFloat(FName("MyFloat"), 1.5f) | Set float parameter |
| 3 | NC->SetVariableVec3(FName("MyVector"), FVector(1, 2, 3)) | Set vector parameter |
| 4 | NC->SetVariableLinearColor(FName("MyColor"), FLinearColor::Red) | Set color parameter |
| 5 | NC->SetVariableBool(FName("MyBool"), true) | Set bool parameter |

### Parameter Types

| Setter Method | Parameter Type |
|---------------|----------------|
| SetVariableFloat | Float |
| SetVariableInt | Integer |
| SetVariableBool | Boolean |
| SetVariableVec2 | FVector2D |
| SetVariableVec3 | FVector |
| SetVariableVec4 | FVector4 |
| SetVariableLinearColor | FLinearColor |
| SetVariableQuat | FQuat |
| SetVariableObject | UObject* |
| SetVariableTexture | UTexture* |

### Plugin Approach

For asset generation, create Niagara parameter presets rather than full system generation:
- Define parameter sets in YAML
- Apply parameters to duplicated template systems
- Register preset assets

### Upgrade Potential: LOW

Niagara systems are best handled through:
- Template duplication
- Parameter preset generation
- NOT full programmatic creation

---

## RESEARCH FINDINGS: GAMEPLAY CUE PROGRAMMATIC CREATION

### Key Discovery: Two Distinct Base Classes

Gameplay Cues have two base classes for different use cases.

### UGameplayCueNotify_Static (UObject-Based)

| Feature | Description |
|---------|-------------|
| Base Class | UObject |
| Use Case | Burst effects (hit impacts, instant VFX) |
| State | No state |
| Tick | No tick |
| Lifecycle | Fire-and-forget |

### AGameplayCueNotify_Actor (Actor-Based)

| Feature | Description |
|---------|-------------|
| Base Class | AActor |
| Use Case | Looping/persistent effects (buffs, shields, DOTs) |
| State | Has state |
| Tick | Can tick |
| Lifecycle | OnActive, WhileActive, OnRemove events |

### Static Cue Implementation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | class UGC_HitImpact : public UGameplayCueNotify_Static | Create class |
| 2 | virtual bool HandleGameplayCue(...) override | Override handler |
| 3 | if (EventType == EGameplayCueEvent::Executed) | Check event type |
| 4 | UGameplayStatics::SpawnEmitterAtLocation(...) | Spawn effect |
| 5 | return true | Mark handled |

### Actor Cue Implementation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | class AGC_ShieldEffect : public AGameplayCueNotify_Actor | Create class |
| 2 | virtual bool OnActive_Implementation(...) override | Handle activation |
| 3 | virtual bool WhileActive_Implementation(...) override | Handle active state |
| 4 | virtual bool OnRemove_Implementation(...) override | Handle removal |

### Tag Configuration

| Requirement | Pattern |
|-------------|---------|
| Tag prefix | GameplayCue.* (required) |
| Constructor | GameplayCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Combat.HitImpact")) |
| Native macro | UE_DEFINE_GAMEPLAY_TAG(TAG_GC_HitImpact, "GameplayCue.Combat.HitImpact") |

### Asset Naming Convention

| Asset Type | Prefix | Example |
|------------|--------|---------|
| Static Cue | GC_ | GC_HitImpact |
| Actor Cue | GC_ | GC_ShieldEffect |

### Upgrade Potential: HIGH

Gameplay Cue generation is FULLY POSSIBLE:
- Static cues for burst effects
- Actor cues for persistent effects
- Tag configuration via constructor
- Effect spawning via standard APIs

---

## RESEARCH FINDINGS: AI PERCEPTION PROGRAMMATIC CONFIGURATION

### Key Discovery: FAISenseConfig Subclasses Enable Full Configuration

Create sense configs in constructor, then register with ConfigureSense().

### Required C++ APIs

| API | Purpose | Module |
|-----|---------|--------|
| UAIPerceptionComponent | Main perception component | AIModule |
| UAISenseConfig_Sight | Sight sense configuration | AIModule |
| UAISenseConfig_Hearing | Hearing sense configuration | AIModule |
| UAISenseConfig_Damage | Damage sense configuration | AIModule |
| UAISenseConfig_Prediction | Prediction sense configuration | AIModule |
| UAISenseConfig_Team | Team sense configuration | AIModule |
| ConfigureSense() | Register sense with component | AIModule |
| SetDominantSense() | Set primary sense | AIModule |

### Sight Configuration

| Property | Type | Example |
|----------|------|---------|
| SightRadius | float | 1000.f |
| LoseSightRadius | float | 1200.f |
| PeripheralVisionAngleDegrees | float | 90.f |
| DetectionByAffiliation.bDetectEnemies | bool | true |
| DetectionByAffiliation.bDetectNeutrals | bool | false |
| DetectionByAffiliation.bDetectFriendlies | bool | false |
| MaxAge | float | 5.f |
| AutoSuccessRangeFromLastSeenLocation | float | -1.f |
| PointOfViewBackwardOffset | float | 0.f |
| NearClippingRadius | float | 0.f |

### Hearing Configuration

| Property | Type | Example |
|----------|------|---------|
| HearingRange | float | 2000.f |
| LoSHearingRange | float | 0.f |
| bUseLoSHearing | bool | false |
| DetectionByAffiliation.bDetectEnemies | bool | true |
| MaxAge | float | 5.f |

### Damage Configuration

| Property | Type | Example |
|----------|------|---------|
| MaxAge | float | 5.f |

### Configuration Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception")) | Create component |
| 2 | UAISenseConfig_Sight* Sight = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig")) | Create sight config |
| 3 | Sight->SightRadius = 1000.f | Set properties |
| 4 | PerceptionComponent->ConfigureSense(*Sight) | Register sense |
| 5 | PerceptionComponent->SetDominantSense(Sight->GetSenseImplementation()) | Set dominant |

### Upgrade Potential: HIGH

AI Perception configuration is FULLY POSSIBLE:
- All sense types configurable
- Affiliation filtering
- Range and angle settings
- Age and caching settings

---

## RESEARCH FINDINGS: CAMERA SHAKE PROGRAMMATIC CREATION

### Key Discovery: Pattern-Based Architecture in UE5

The modern system uses UCameraShakeBase with pluggable UCameraShakePattern subclasses.

### Required C++ APIs

| API | Purpose | Module |
|-----|---------|--------|
| UCameraShakeBase | Base shake class | GameplayCameras |
| UCameraShakePattern | Pluggable pattern | GameplayCameras |
| UPerlinNoiseCameraShakePattern | Perlin noise pattern | GameplayCameras |
| UWaveOscillatorCameraShakePattern | Wave oscillator pattern | GameplayCameras |
| UCompositeCameraShakePattern | Multiple patterns combined | GameplayCameras |
| USequenceCameraShakePattern | Sequential patterns | GameplayCameras |
| UMatineeCameraShake | Legacy shake class | GameplayCameras |

### Perlin Noise Pattern Configuration

| Property | Subproperty | Type | Example |
|----------|-------------|------|---------|
| X | Amplitude | float | 10.f |
| X | Frequency | float | 10.f |
| Y | Amplitude | float | 10.f |
| Y | Frequency | float | 10.f |
| Z | Amplitude | float | 10.f |
| Z | Frequency | float | 10.f |
| Pitch | Amplitude | float | 10.f |
| Pitch | Frequency | float | 10.f |
| Yaw | Amplitude | float | 10.f |
| Yaw | Frequency | float | 10.f |
| Roll | Amplitude | float | 10.f |
| Roll | Frequency | float | 10.f |
| FOV | Amplitude | float | 5.f |
| FOV | Frequency | float | 5.f |
| Duration | - | float | 2.f |
| BlendInTime | - | float | 0.2f |
| BlendOutTime | - | float | 0.5f |

### Creation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | UMyCameraShake::UMyCameraShake() : UCameraShakeBase(FObjectInitializer::Get()) | Constructor |
| 2 | UPerlinNoiseCameraShakePattern* Pattern = CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(TEXT("Pattern")) | Create pattern |
| 3 | Pattern->X.Amplitude = 10.f | Set X amplitude |
| 4 | Pattern->Duration = 2.f | Set duration |
| 5 | SetRootShakePattern(Pattern) | Assign pattern |

### Legacy UMatineeCameraShake

Still functional for simpler use cases:

| Property | Type | Description |
|----------|------|-------------|
| OscillationDuration | float | Duration in seconds |
| OscillationBlendInTime | float | Blend in duration |
| OscillationBlendOutTime | float | Blend out duration |
| RotOscillation.Pitch | FOscillator | Pitch oscillation |
| RotOscillation.Yaw | FOscillator | Yaw oscillation |
| RotOscillation.Roll | FOscillator | Roll oscillation |
| LocOscillation.X | FOscillator | X location oscillation |
| LocOscillation.Y | FOscillator | Y location oscillation |
| LocOscillation.Z | FOscillator | Z location oscillation |
| FOVOscillation | FOscillator | FOV oscillation |

### Upgrade Potential: HIGH

Camera Shake generation is FULLY POSSIBLE:
- Modern pattern-based system
- Legacy oscillator system
- All parameters configurable
- Multiple pattern types

---

## RESEARCH FINDINGS: DATA ASSET PROGRAMMATIC CREATION

### Key Discovery: Reflection-Based Property Population

For generator plugins, UPrimaryDataAsset provides Asset Manager integration. Property population uses FProperty reflection.

### Required C++ APIs

| API | Purpose | Module |
|-----|---------|--------|
| UPrimaryDataAsset | Base data asset with Asset Manager integration | Engine |
| UDataAsset | Simple data asset base | Engine |
| FProperty | Property reflection | CoreUObject |
| FIntProperty | Integer property | CoreUObject |
| FFloatProperty | Float property | CoreUObject |
| FStrProperty | String property | CoreUObject |
| FArrayProperty | Array property | CoreUObject |
| FStructProperty | Struct property | CoreUObject |
| FObjectProperty | Object reference property | CoreUObject |

### Property Population Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | FProperty* Prop = Object->GetClass()->FindPropertyByName(FName("MyProperty")) | Find property |
| 2 | void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Object) | Get value pointer |
| 3 | if (FIntProperty* IntProp = CastField<FIntProperty>(Prop)) | Check property type |
| 4 | IntProp->SetPropertyValue(ValuePtr, 42) | Set integer value |

### Property Type Setters

| Property Type | Cast Type | Set Method |
|---------------|-----------|------------|
| int32 | FIntProperty | SetPropertyValue(ValuePtr, IntValue) |
| float | FFloatProperty | SetPropertyValue(ValuePtr, FloatValue) |
| double | FDoubleProperty | SetPropertyValue(ValuePtr, DoubleValue) |
| bool | FBoolProperty | SetPropertyValue(ValuePtr, BoolValue) |
| FString | FStrProperty | SetPropertyValue(ValuePtr, StringValue) |
| FName | FNameProperty | SetPropertyValue(ValuePtr, NameValue) |
| FText | FTextProperty | SetPropertyValue(ValuePtr, TextValue) |
| UObject* | FObjectProperty | SetPropertyValue(ValuePtr, ObjectPtr) |
| UClass* | FClassProperty | SetPropertyValue(ValuePtr, ClassPtr) |

### Array Property Population

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop)) | Check array type |
| 2 | FScriptArrayHelper Helper(ArrayProp, ValuePtr) | Create helper |
| 3 | Helper.AddValue() | Add element |
| 4 | void* ElementPtr = Helper.GetRawPtr(Index) | Get element pointer |
| 5 | (Set element using inner property type) | Set element value |

### Struct Property Population

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | if (FStructProperty* StructProp = CastField<FStructProperty>(Prop)) | Check struct type |
| 2 | void* StructPtr = StructProp->ContainerPtrToValuePtr<void>(Object) | Get struct pointer |
| 3 | UScriptStruct* ScriptStruct = StructProp->Struct | Get struct definition |
| 4 | FProperty* MemberProp = ScriptStruct->FindPropertyByName(...) | Find member |
| 5 | (Set member using appropriate property type) | Set member value |

### Upgrade Potential: HIGH

Data Asset generation is FULLY POSSIBLE:
- Any UPROPERTY accessible via reflection
- Arrays and structs supported
- Object references supported
- Full Asset Manager integration with UPrimaryDataAsset

---

## UPDATED ASSETS UPGRADE POTENTIAL

### Can Be Upgraded to FULL Auto-Generation

| Asset | Previous Status | New Status | Blocking Issues | Solution |
|-------|-----------------|------------|-----------------|----------|
| GA_* (17 abilities) | STRUCTURE | FULL | Event graph logic | UK2Node APIs |
| BP_FatherCompanion | STRUCTURE | FULL | Event graph logic | UK2Node APIs |
| BP_TurretProjectile | STRUCTURE | FULL | Event graph logic | UK2Node APIs |
| BP_* (4 actors) | STRUCTURE | FULL | Event graph logic | UK2Node APIs |
| M_LaserGlow | STRUCTURE | FULL | Material expressions | UMaterialExpression APIs |
| WBP_* (3 widgets) | STRUCTURE | FULL | Visual hierarchy + event logic | WidgetTree::ConstructWidget |
| BT_* (2 behavior trees) | STRUCTURE | FULL | Node hierarchy | FBTCompositeChild arrays |
| SymbioteChargeComponent | STRUCTURE | FULL | Event graph logic | UK2Node APIs |

### Partially Upgradeable

| Asset | Previous Status | New Status | What Can Be Generated | What Still Requires Manual Work |
|-------|-----------------|------------|----------------------|--------------------------------|
| AM_* (6 montages) | STRUCTURE | PARTIAL | Notifies, sections, blend settings | Source animation sequences |
| NAS_FatherAttack | STRUCTURE | PARTIAL | Animation set structure | Source animations |

### Cannot Be Upgraded (External Dependencies)

| Asset | Status | Blocking Issue | Reason |
|-------|--------|----------------|--------|
| Animation Sequences | N/A | External asset | Requires 3D animation data |
| Skeleton/Rig | N/A | External asset | Requires rigged mesh |

---

## UPDATED IMPACT ASSESSMENT

| Metric | Before v1.0 Research | After v1.0 Research | After v2.0 Research |
|--------|---------------------|---------------------|---------------------|
| FULL Auto-Gen Assets | 248 | 270 | 278 |
| STRUCTURE Only Assets | 18 | 10 | 2 |
| PARTIAL Assets | 0 | 0 | 8 |
| Manual Implementation Work | 100% | 50-60% | 15-25% |
| Affected Asset Types | 0 | 22 | 30 |

### Detailed Breakdown After v2.0

| Level | Count | Assets |
|-------|-------|--------|
| FULL | 278 | Tags, GE, GA, BP actors, WBP, BT, M, Input, Configs, etc. |
| PARTIAL | 8 | AM_* (6), NAS_* (1), DBP_* (1) |
| REQUIRES PLUGIN | 1 | AS_DomeAttributes |
| USES BUILT-IN | 8 | GE_Invulnerable, BPA_Attack_Melee, etc. |

---

## NEW YAML SCHEMA EXTENSIONS

### behavior_tree_graph Schema

| Field | Type | Description |
|-------|------|-------------|
| blackboard_keys | Array | Blackboard key definitions |
| blackboard_keys[].name | String | Key name |
| blackboard_keys[].type | String | Object, Bool, Float, Int, Vector, etc. |
| blackboard_keys[].base_class | String | Base class for Object keys |
| root_node | Object | Root composite node |
| root_node.type | String | Selector, Sequence, SimpleParallel |
| root_node.children | Array | Child nodes |
| children[].type | String | Task, Composite, Decorator, Service |
| children[].class | String | Specific node class |
| children[].decorators | Array | Attached decorators |
| children[].services | Array | Attached services (composites only) |

### widget_tree Schema

| Field | Type | Description |
|-------|------|-------------|
| root_widget | Object | Root widget definition |
| root_widget.type | String | CanvasPanel, VerticalBox, etc. |
| root_widget.name | String | Widget name |
| root_widget.children | Array | Child widgets |
| children[].type | String | Widget class |
| children[].name | String | Widget name |
| children[].slot | Object | Slot configuration |
| slot.position | Array | [X, Y] for canvas |
| slot.size | Array | [Width, Height] for canvas |
| slot.anchors | Array | [MinX, MinY, MaxX, MaxY] |
| slot.alignment | Array | [X, Y] alignment |
| slot.padding | Object | Padding values |

### montage_config Schema

| Field | Type | Description |
|-------|------|-------------|
| slot_name | String | Animation slot name |
| blend_in | Float | Blend in time |
| blend_out | Float | Blend out time |
| sections | Array | Montage sections |
| sections[].name | String | Section name |
| sections[].start_time | Float | Start time |
| sections[].next_section | String | Branch target |
| notifies | Array | Animation notifies |
| notifies[].name | String | Notify name |
| notifies[].time | Float | Trigger time |
| notifies[].class | String | Notify class |
| notifies[].duration | Float | Duration (for states) |

### gameplay_cue Schema

| Field | Type | Description |
|-------|------|-------------|
| name | String | GC_AssetName |
| type | String | Static or Actor |
| tag | String | GameplayCue.Category.Name |
| particle_system | String | Particle system reference |
| sound_cue | String | Sound cue reference |
| camera_shake | String | Camera shake reference |

### camera_shake Schema

| Field | Type | Description |
|-------|------|-------------|
| name | String | CS_AssetName |
| pattern_type | String | PerlinNoise, WaveOscillator, Composite |
| duration | Float | Shake duration |
| blend_in | Float | Blend in time |
| blend_out | Float | Blend out time |
| location | Object | Location oscillation |
| rotation | Object | Rotation oscillation |
| fov | Object | FOV oscillation |

### ai_perception Schema

| Field | Type | Description |
|-------|------|-------------|
| senses | Array | Sense configurations |
| senses[].type | String | Sight, Hearing, Damage, Prediction, Team |
| senses[].properties | Object | Sense-specific properties |
| dominant_sense | String | Primary sense type |

---

## UPDATED IMPLEMENTATION PHASES

### Phase A: Research Validation (COMPLETED)

| Task | Description | Status |
|------|-------------|--------|
| A.1 | Test UK2Node_CallFunction spawning | DONE |
| A.2 | Validate pin connection MakeLinkTo patterns | DONE |
| A.3 | Test material expression creation | DONE |
| A.4 | Document working patterns | DONE |
| A.5 | Test FBTCompositeChild patterns | DONE (v2.0) |
| A.6 | Test WidgetTree::ConstructWidget | DONE (v2.0) |
| A.7 | Test FAnimNotifyEvent::Link | DONE (v2.0) |

### Phase B: YAML Schema Extension

| Task | Description | Time Estimate |
|------|-------------|---------------|
| B.1 | Design event_graph YAML schema | 2 hours |
| B.2 | Design material_graph YAML schema | 1 hour |
| B.3 | Design behavior_tree_graph YAML schema | 2 hours |
| B.4 | Design widget_tree YAML schema | 2 hours |
| B.5 | Design montage_config YAML schema | 1 hour |
| B.6 | Update YAML parser for new sections | 6 hours |
| B.7 | Add validation for all new schemas | 3 hours |
| TOTAL | | 17 hours |

### Phase C: Generator Implementation

| Task | Description | Time Estimate |
|------|-------------|---------------|
| C.1 | Implement EventGraphGenerator class | 8 hours |
| C.2 | Implement MaterialGraphGenerator class | 4 hours |
| C.3 | Implement BehaviorTreeGenerator class | 6 hours |
| C.4 | Implement WidgetTreeGenerator class | 6 hours |
| C.5 | Implement MontageConfigGenerator class | 4 hours |
| C.6 | Add node position auto-layout | 4 hours |
| C.7 | Add compilation/validation step | 2 hours |
| TOTAL | | 34 hours |

### Phase D: Integration Testing

| Task | Description | Time Estimate |
|------|-------------|---------------|
| D.1 | Test GA_FatherArmor event graph generation | 2 hours |
| D.2 | Test M_LaserGlow material generation | 1 hour |
| D.3 | Test BT_FatherFollow generation | 2 hours |
| D.4 | Test WBP_MarkIndicator generation | 2 hours |
| D.5 | Test AM_FatherAttack config generation | 1 hour |
| D.6 | Test full regeneration cycle | 2 hours |
| D.7 | Performance testing | 1 hour |
| TOTAL | | 11 hours |

### Total Implementation Estimate

| Phase | Hours |
|-------|-------|
| Phase A (Research) | COMPLETED |
| Phase B (Schema) | 17 hours |
| Phase C (Generators) | 34 hours |
| Phase D (Testing) | 11 hours |
| TOTAL | 62 hours |

---

## REQUIRED MODULE DEPENDENCIES

### Current Dependencies

| Module | Purpose |
|--------|---------|
| Core | Core UE types |
| CoreUObject | Object system |
| Engine | Engine classes |
| UnrealEd | Editor utilities |
| GameplayAbilities | GAS |
| GameplayTags | Tag system |
| AIModule | AI system |
| Slate | UI framework |
| SlateCore | Slate core |
| UMG | UMG runtime |
| UMGEditor | UMG editor |
| BlueprintGraph | UK2Node classes |
| Kismet | FBlueprintEditorUtils |

### New Dependencies for v2.0

| Module | Purpose | Required For |
|--------|---------|--------------|
| GameplayCameras | Camera shake patterns | CS_* assets |
| Niagara | VFX system | Parameter presets |
| NiagaraEditor | Niagara editor | System duplication |

### Build.cs Update

| Array | Modules to Add |
|-------|----------------|
| PublicDependencyModuleNames | GameplayCameras, Niagara |
| PrivateDependencyModuleNames | NiagaraEditor, BehaviorTreeEditor |

---

## REFERENCES

| Source | URL | Date |
|--------|-----|------|
| Epic Forums: Create Blueprint Graph | forums.unrealengine.com/t/is-it-possible-to-create-a-blueprint-graph-with-code/2024453 | Sept 2024 |
| Epic Forums: Event Graph C++ | forums.unrealengine.com/t/how-to-create-and-populate-an-event-graph-in-c/2025873 | Sept 2024 |
| Custom K2Nodes Tutorial | unrealist.org/custom-blueprint-nodes | Nov 2022 |
| K2Node Introduction | olssondev.github.io/2023-02-13-K2Nodes | Feb 2023 |
| Material Creation C++ | isaratech.com/ue4-programmatically-create-a-new-material-and-inner-nodes | Feb 2019 |
| UE5.6 API Reference | dev.epicgames.com/documentation/en-us/unreal-engine/API | Current |
| FBTCompositeChild Decorators | docs.unrealengine.com/5.3/en-US/API/Runtime/AIModule/BehaviorTree/FBTCompositeChild/Decorators | Current |
| AI Controller and Behavior Tree | unrealcode.net/AIDev | Current |
| Create AnimNotify in C++ | forums.unrealengine.com/t/create-animnotify-in-c/301214 | 2018 |
| UAnimMontage Examples | cpp.hotexamples.com/examples/-/UAnimMontage | Current |
| Create UWidget at Runtime | goldensyrupgames.com/blog/2016-10-06-create-a-uwidget-at-runtime | 2016 |
| UMatineeCameraShake API | docs.unrealengine.com/5.0/en-US/API/Plugins/GameplayCameras/UMatineeCameraShake | Current |
| Gameplay Ability System Guide | ukustra.medium.com | Current |

---

**END OF FATHER ABILITY GENERATOR PLUGIN ENHANCEMENT RESEARCH v2.0**

**Previous FULL Auto-Generated: 248**
**After v1.0 Research FULL: 270**
**After v2.0 Research FULL: 278**
**Manual Work Reduction: 75-85%**
