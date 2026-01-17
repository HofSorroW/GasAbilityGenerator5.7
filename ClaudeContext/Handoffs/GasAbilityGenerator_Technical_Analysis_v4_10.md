# GasAbilityGenerator v4.10 - Technical Analysis

**Document Version:** 1.0
**Plugin Version:** 4.10.1
**Analysis Date:** January 2026
**Target Engine:** Unreal Engine 5.7

---

## Executive Summary

GasAbilityGenerator is an enterprise-grade asset generation plugin for Unreal Engine 5.7 that transforms YAML manifest definitions into fully-functional UE5 assets. The plugin provides **155 asset generation** capability with comprehensive validation, regeneration safety, and CI/CD integration.

### Key Metrics

| Metric | Value |
|--------|-------|
| Total Lines of Code | 72,241 |
| Source Files | 79 (42 headers, 37 implementations) |
| Generator Classes | 34 |
| Manifest Definition Structs | 80+ |
| Parser Sections | 39 |
| Supported Asset Types | 32+ |
| Material Expression Types | 102 (including aliases) |
| Event Graph Node Types | 18+ |
| Widget Types | 25+ |
| Validation Error Codes | 11 |

---

## 1. System Architecture

### 1.1 High-Level Data Flow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           MANIFEST LAYER                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│  manifest.yaml ──────┬──────> FGasAbilityGeneratorParser                    │
│  DialogueData.csv ───┤              │                                        │
│  *.yaml (incremental)┘              ▼                                        │
│                            FManifestData (80+ structs)                       │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         VALIDATION LAYER                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐   ┌─────────────────┐   ┌─────────────────┐           │
│  │ Pre-Validation  │   │ Material Expr   │   │ Dependency      │           │
│  │ Framework       │   │ 6-Guardrail     │   │ Resolution      │           │
│  │ (All Generators)│   │ (M_, MF_, MIC_) │   │ (Ordering)      │           │
│  └─────────────────┘   └─────────────────┘   └─────────────────┘           │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         GENERATION LAYER                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│  34 Generator Classes (F*Generator)                                          │
│  ┌──────────────┬──────────────┬──────────────┬──────────────┐              │
│  │ GAS Core     │ Materials    │ AI/Behavior  │ Content      │              │
│  │ (E,IA,IMC,   │ (M,MF,MIC)   │ (BB,BT,      │ (EI,IC,NPC,  │              │
│  │  GE,GA)      │              │  Schedule,   │  DBP,Quest)  │              │
│  │              │              │  Goal,BPA)   │              │              │
│  └──────────────┴──────────────┴──────────────┴──────────────┘              │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         METADATA LAYER (v3.0 Regen Safety)                   │
├─────────────────────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────┐   ┌─────────────────────────────┐          │
│  │ UGeneratorAssetMetadata     │   │ UGeneratorMetadataRegistry  │          │
│  │ (IInterface_AssetUserData)  │   │ (Central Registry Fallback) │          │
│  │ - Animation Montages        │   │ - UDataAsset                │          │
│  │ - Dialogue Blueprints       │   │ - UBlueprint                │          │
│  └─────────────────────────────┘   │ - UNiagaraSystem            │          │
│                                     └─────────────────────────────┘          │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         OUTPUT LAYER                                         │
├─────────────────────────────────────────────────────────────────────────────┤
│  /Game/{ProjectName}/                                                        │
│  ├── Abilities/           (GA_*)                                             │
│  ├── Effects/             (GE_*)                                             │
│  ├── Materials/           (M_, MF_, MIC_*)                                   │
│  ├── AI/                  (BB_, BT_, BPA_, Schedule_, Goal_*)                │
│  ├── Items/               (EI_, IC_*)                                        │
│  ├── NPCs/                (NPC_, CD_*)                                       │
│  ├── Dialogue/            (DBP_, TDS_*)                                      │
│  ├── Quests/              (Quest_*)                                          │
│  └── ...                                                                     │
│                                                                              │
│  Reports: /Game/Generated/Reports/ + Saved/GasAbilityGenerator/Reports/      │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 Core Source Files

| File | Lines | Purpose |
|------|-------|---------|
| GasAbilityGeneratorGenerators.cpp | 17,648 | All 34 generator implementations |
| GasAbilityGeneratorParser.cpp | 10,521 | YAML parsing to FManifest* structs |
| GasAbilityGeneratorTypes.h | 5,211 | 80+ manifest definition structs |
| GasAbilityGeneratorCommandlet.cpp | ~1,500 | CLI commandlet execution |
| GasAbilityGeneratorMetadata.cpp | ~800 | Regen/diff tracking system |
| GasAbilityGeneratorReport.cpp | ~600 | Report generation (v4.7) |
| GasAbilityGeneratorPipeline.cpp | ~500 | Mesh-to-Item pipeline |

### 1.3 Module Dependencies

```cpp
// Build.cs Dependencies
PublicDependencyModuleNames:
  - Core, CoreUObject, Engine, Slate, SlateCore
  - UnrealEd, AssetTools, AssetRegistry
  - BlueprintGraph, Kismet, KismetCompiler
  - GameplayAbilities, GameplayTags, GameplayTasks
  - EnhancedInput, AIModule, Niagara
  - NarrativeArsenal, NarrativePro, NarrativeQuestEditor, NarrativeDialogueEditor
```

---

## 2. Generator Inventory

### 2.1 Complete Generator List (34 Classes)

| # | Generator Class | Prefix | UE5 Asset Type | Automation |
|---|-----------------|--------|----------------|------------|
| 1 | FEnumerationGenerator | E_ | UUserDefinedEnum | Full |
| 2 | FInputActionGenerator | IA_ | UInputAction | Full |
| 3 | FInputMappingContextGenerator | IMC_ | UInputMappingContext | Full |
| 4 | FGameplayEffectGenerator | GE_ | UBlueprint (UGameplayEffect) | High |
| 5 | FGameplayAbilityGenerator | GA_ | UBlueprint (NarrativeGameplayAbility) | High |
| 6 | FActorBlueprintGenerator | BP_ | UBlueprint (AActor subclass) | High |
| 7 | FWidgetBlueprintGenerator | WBP_ | UWidgetBlueprint | High |
| 8 | FDialogueBlueprintGenerator | DBP_ | UDialogueBlueprint | High |
| 9 | FBlackboardGenerator | BB_ | UBlackboardData | High |
| 10 | FBehaviorTreeGenerator | BT_ | UBehaviorTree | High |
| 11 | FMaterialGenerator | M_ | UMaterial | High |
| 12 | FMaterialFunctionGenerator | MF_ | UMaterialFunction | High |
| 13 | FMaterialInstanceGenerator | MIC_ | UMaterialInstanceConstant | High |
| 14 | FFloatCurveGenerator | FC_ | UCurveFloat | Medium |
| 15 | FAnimationMontageGenerator | AM_ | UAnimMontage | Medium |
| 16 | FAnimationNotifyGenerator | NAS_ | UBlueprint (UAnimNotify) | Medium |
| 17 | FNiagaraSystemGenerator | NS_ | UNiagaraSystem | High |
| 18 | FEquippableItemGenerator | EI_ | UBlueprint (EquippableItem/WeaponItem) | High |
| 19 | FActivityGenerator | BPA_ | UBlueprint (NarrativeActivityBase) | High |
| 20 | FAbilityConfigurationGenerator | AC_ | UAbilityConfiguration | High |
| 21 | FActivityConfigurationGenerator | ActConfig_ | UActivityConfiguration | High |
| 22 | FItemCollectionGenerator | IC_ | UItemCollection | High |
| 23 | FNarrativeEventGenerator | NE_ | UNarrativeEvent | High |
| 24 | FGameplayCueGenerator | GC_ | UBlueprint (GameplayCueNotify_*) | Medium |
| 25 | FNPCDefinitionGenerator | NPC_ | UNPCDefinition | High |
| 26 | FCharacterDefinitionGenerator | CD_ | UCharacterDefinition | High |
| 27 | FCharacterAppearanceGenerator | CA_ | UCharacterAppearance | Medium |
| 28 | FTaggedDialogueSetGenerator | TDS_ | UTaggedDialogueSet | High |
| 29 | FTriggerSetGenerator | TS_ | UTriggerSet | High |
| 30 | FActivityScheduleGenerator | Schedule_ | UNPCActivitySchedule | High |
| 31 | FGoalItemGenerator | Goal_ | UBlueprint (NPCGoalItem) | High |
| 32 | FQuestGenerator | Quest_ | UQuestBlueprint | High |
| 33 | FPOIPlacementGenerator | - | APOIActor (level actor) | Medium |
| 34 | FNPCSpawnerPlacementGenerator | - | ANPCSpawner (level actor) | Medium |
| - | FTagGenerator | - | Config file (INI) | Full |
| - | FEventGraphGenerator | - | (Utility - not standalone) | - |

### 2.2 Automation Level Definitions

| Level | Definition | Manual Steps |
|-------|------------|--------------|
| **Full** | 100% automated, no manual steps | None |
| **High** | 95%+ automated, minimal manual steps | Visual tweaks, optional features |
| **Medium** | 70-95% automated, some manual steps | Asset linking, visual setup |
| **Low** | <70% automated, significant manual work | Core functionality requires editor |

### 2.3 Generator Capabilities Matrix

#### 2.3.1 GAS Core Generators

| Generator | Event Graphs | Variables | Tags | Replication | Components |
|-----------|--------------|-----------|------|-------------|------------|
| FGameplayEffectGenerator | - | - | GrantedTags | - | GEComponents |
| FGameplayAbilityGenerator | Full | Full | 6 arrays | Net Policy | - |
| FActorBlueprintGenerator | Full | Full | - | Full | Full |
| FWidgetBlueprintGenerator | Full | Full | - | - | Widget Tree |

#### 2.3.2 Material Generators

| Generator | Expressions | Connections | Parameters | Functions | Validation |
|-----------|-------------|-------------|------------|-----------|------------|
| FMaterialGenerator | 102 types | Full | Scalar/Vector | MF calls | 6-Guardrail |
| FMaterialFunctionGenerator | 102 types | Full | Inputs/Outputs | Recursive | 6-Guardrail |
| FMaterialInstanceGenerator | - | - | Override | Parent ref | Session cache |

#### 2.3.3 AI/Behavior Generators

| Generator | Tree Structure | Decorators | Services | Properties |
|-----------|----------------|------------|----------|------------|
| FBlackboardGenerator | - | - | - | 8 key types |
| FBehaviorTreeGenerator | Full | Full | Full | Reflection |
| FActivityScheduleGenerator | - | - | - | Time-based |
| FGoalItemGenerator | - | - | - | Score/Tags |

#### 2.3.4 Content Generators

| Generator | Dialogue Trees | Inventory | Vendor | Schedules | Quest States |
|-----------|----------------|-----------|--------|-----------|--------------|
| FNPCDefinitionGenerator | Links | Loadouts | Full | Array | - |
| FDialogueBlueprintGenerator | Full | - | - | - | - |
| FQuestGenerator | - | Rewards | - | - | Full |
| FEquippableItemGenerator | - | Full | - | - | - |

---

## 3. Validation Systems

### 3.1 Material Expression Validation (v4.10)

The 6-Guardrail validation system prevents hallucinated or malformed material expressions:

#### 3.1.1 Error Code Enumeration

```cpp
enum class EMaterialExprValidationCode : uint8
{
    E_UNKNOWN_EXPRESSION_TYPE,      // Type not in KnownExpressionTypes
    E_UNKNOWN_SWITCH_KEY,           // Invalid QualitySwitch/ShadingPath/FeatureLevel key
    E_FUNCTION_NOT_FOUND,           // MaterialFunction doesn't exist
    E_FUNCTION_INSTANCE_BLOCKED,    // _Inst path used (blocked)
    E_MISSING_REQUIRED_INPUT,       // Switch missing 'default' input
    E_DUPLICATE_EXPRESSION_ID,      // Duplicate expression ID
    E_EXPRESSION_REFERENCE_INVALID, // Connection references non-existent expression
    W_DEPRECATED_SWITCH_KEY,        // Deprecated switch key (normalized)
    W_FUNCTION_INPUT_MISMATCH,      // (Reserved) Function input count mismatch
    W_FUNCTION_PATH_NORMALIZED,     // Path was normalized (warning)
    None                            // No error
};
```

#### 3.1.2 KnownExpressionTypes (102 entries)

| Category | Types | Count |
|----------|-------|-------|
| Basic Math | constant, constant2/3/4vector, add, subtract, multiply, divide, power, abs, floor, ceil, frac, fmod, saturate, clamp, oneminus, lerp, min, max, sine, cosine, tangent, arcsine, arccosine, arctangent | 26 |
| Vector Operations | normalize, dotproduct, crossproduct, distance, length, appendvector, componentmask, breakoutfloat2/3/4 | 10 |
| Texture | texturesample, texturesampleparameter2d, texturecoordinate, texcoord, uv, panner, rotator | 7 |
| Parameters | scalarparameter, scalarparam, vectorparameter, vectorparam, staticboolparameter, staticbool | 6 |
| Time | time, deltatime, realtime | 3 |
| World/Camera | worldposition, objectposition, actorpositionws, objectpositionws, camerapositionws, cameravector, reflectionvector, lightvector, pixeldepth, scenedepth, screenposition, viewsize, scenetexelsize | 13 |
| Particle/VFX | particlecolor, particlesubuv, dynamicparameter, vertexcolor, depthfade, spheremask, particlepositionws, particleradius, particlerelativetime, particlerandom, particledirection, particlespeed, particlesize, particlemotionblurfade, perinstancefadeamount, perinstancerandom, objectorientation, objectradius, preskinnedposition, preskinnednormal, vertexnormalws, pixelnormalws, vertextangentws, twosidedsign, eyeadaptation, atmosphericfogcolor, precomputedaomask, gireplace | 28 |
| Switch | qualityswitch, shadingpathswitch, featurelevelswitch, staticswitch | 4 |
| Functions | materialfunctioncall | 1 |
| Special | sobol, temporalsobol, scenetexture, fresnel, noise, makematerialattributes, if, customexpression | 8 |
| **Total** | | **102** |

#### 3.1.3 6-Guardrail System

| Guardrail | Purpose | Implementation |
|-----------|---------|----------------|
| 1. Quote Strip | Remove surrounding quotes from values | `Value.TrimQuotes()` before processing |
| 2. Case-Insensitive | Normalize type/key comparisons | `ToLower()` on all lookups |
| 3. ContextPath Root | Standardize error paths | `/Materials/{Asset}/Expressions/{id}` |
| 4. Contextual Errors | Detect hallucinations with details | Original input + suggested fix |
| 5. Field Normalizers | Normalize specific field formats | Switch keys, scalar values, paths |
| 6. Stable Ordering | Deterministic validation output | Sorted diagnostics array |

#### 3.1.4 3-Pass Validation Algorithm

```
Pass 1: Expression Validation
  - Check expression type against KnownExpressionTypes
  - Validate switch keys (QualitySwitch, ShadingPathSwitch, FeatureLevelSwitch)
  - Validate MaterialFunctionCall paths (block _Inst, verify existence)
  - Check for duplicate expression IDs
  - Validate required inputs (switch 'default')

Pass 2: Connection Validation
  - Verify all referenced expression IDs exist
  - Build connection graph for unused detection

Pass 3: Unused Detection
  - Flag expressions with no inbound/outbound connections
  - Generate W_EXPRESSION_UNUSED warnings
```

### 3.2 Pre-Validation Framework

Generic validation framework used by all generators:

```cpp
struct FPreValidationResult
{
    bool bValid;
    int32 ErrorCount;
    int32 WarningCount;
    int32 InfoCount;
    TArray<FValidationIssue> Issues;
};

enum class EValidationSeverity { Info, Warning, Error };
enum class EValidationCategory { Required, Reference, Format, TypeMismatch, Range, Dependency, Template, Custom };
```

**Static Helpers in FGeneratorBase:**
- `ValidateRequiredField()` - Check non-empty required fields
- `ValidateAssetReference()` - Verify asset paths resolve
- `ValidateParentClass()` - Check parent class exists
- `ValidateFloatRange()` - Range validation for numeric values
- `ValidateNamePrefix()` - Enforce naming conventions
- `ValidateNonEmptyArray()` - Array must have elements

### 3.3 Niagara FX Validation (v2.9.1)

```cpp
struct FFXValidationResult
{
    bool bTemplateValid;
    bool bDescriptorValid;
    TArray<FFXValidationError> Errors;
    TArray<FFXExpectedParam> MissingParams;
};
```

---

## 4. Metadata & Regen Safety System (v3.0/3.1)

### 4.1 Dual-Layer Storage

**Layer 1: UGeneratorAssetMetadata (IInterface_AssetUserData)**
- Attached directly to assets supporting AssetUserData interface
- Used for: Animation Montages, Dialogue Blueprints, etc.

**Layer 2: UGeneratorMetadataRegistry (Central Fallback)**
- DataAsset at `/Game/{Project}/GeneratorMetadataRegistry`
- Used for: UDataAsset, UBlueprint, UNiagaraSystem (no AssetUserData support)

### 4.2 Metadata Fields

```cpp
USTRUCT()
struct FGeneratorMetadataEntry
{
    FString GeneratorId;        // "GameplayAbility", "Material", etc.
    FString ManifestPath;       // Path to manifest file
    FString ManifestAssetKey;   // Asset identifier in manifest
    uint64 InputHash;           // Hash of manifest definition
    uint64 OutputHash;          // Hash of generated asset content
    FString GeneratorVersion;   // Plugin version at generation
    FDateTime Timestamp;        // Generation timestamp
    TArray<FString> Dependencies;  // Dependent asset paths
    bool bIsGenerated;          // Flag for generator-created assets
};
```

### 4.3 Hash Computation

**Input Hash (Manifest → uint64):**
- All `FManifest*Definition` structs implement `uint64 ComputeHash()`
- Excludes presentational fields (Folder, EditorPositions)
- Rotation-based XOR for position-independence

**Output Hash (Asset → uint64):**
- `ComputeDataAssetOutputHash()` - Hash DataAsset properties
- `ComputeBlueprintOutputHash()` - Hash nodes, connections, variables, components

### 4.4 Generation Status Matrix

| InputHash Changed | OutputHash Changed | Status | Action |
|-------------------|-------------------|--------|--------|
| N/A (no asset) | N/A | CREATE | Generate new asset |
| Yes | No | MODIFY | Safe to regenerate |
| No | No | SKIP | Do nothing |
| Yes | Yes | CONFLICT | Requires `--force` |
| No | Yes | SKIP | Manual edit preserved |

### 4.5 Dry-Run System

```cpp
enum class EDryRunStatus
{
    WillCreate,    // New asset
    WillModify,    // Manifest changed, no manual edits
    WillSkip,      // No changes needed
    Conflicted,    // Both changed - needs --force
    PolicySkip     // Policy prevents regeneration
};

struct FDryRunResult
{
    EDryRunStatus Status;
    TArray<FString> ManifestChanges;   // Changed fields
    TArray<FString> AssetChanges;      // Detected manual edits
    uint64 StoredInputHash;
    uint64 CurrentInputHash;
    uint64 StoredOutputHash;
    uint64 CurrentOutputHash;
};
```

---

## 5. Parser Architecture

### 5.1 Section Parsers (39 Total)

| Category | Sections |
|----------|----------|
| Core GAS | `tags`, `enumerations`, `input_actions`, `input_mapping_contexts`, `gameplay_effects`, `gameplay_abilities` |
| Blueprints | `actor_blueprints`, `widget_blueprints`, `event_graphs` |
| Materials | `materials`, `material_functions`, `material_instances` |
| Animation | `float_curves`, `animation_montages`, `animation_notifies` |
| AI/Behavior | `blackboards`, `behavior_trees`, `activities`, `activity_schedules`, `goal_items` |
| Content | `equippable_items`, `item_collections`, `dialogue_blueprints`, `tagged_dialogue_sets` |
| Configuration | `ability_configurations`, `activity_configurations`, `character_definitions`, `character_appearances`, `trigger_sets` |
| Narrative | `npc_definitions`, `narrative_events`, `gameplay_cues`, `quests` |
| VFX | `niagara_systems`, `fx_presets` |
| Pipeline | `pipeline_config`, `pipeline_items`, `pipeline_collections`, `pipeline_loadouts` |
| Level | `poi_placements`, `npc_spawner_placements` |

### 5.2 Parsing Flow

```
ParseManifest(FilePath)
  │
  ├─ Read file content
  ├─ Strip BOM, normalize line endings
  │
  ├─ foreach line:
  │    │
  │    ├─ StripYamlComment()      // v3.9.10 inline comments
  │    ├─ GetIndentLevel()
  │    ├─ IsSectionHeader()
  │    │
  │    └─ Route to appropriate ParseXxx() function
  │         │
  │         ├─ ParseTags()
  │         ├─ ParseEnumerations()
  │         ├─ ParseGameplayAbilities()
  │         │   └─ ParseAbilityTags()
  │         │   └─ ParseEventGraph()
  │         │   └─ ParseVariables()
  │         ├─ ... (39 section parsers)
  │         │
  │         └─ Return to main loop
  │
  └─ Return FManifestData
```

### 5.3 Helper Functions

| Function | Purpose |
|----------|---------|
| `StripYamlComment()` | Remove inline `# comments` |
| `GetLineValue()` | Extract value after `:` |
| `GetIndentLevel()` | Calculate indent depth |
| `IsArrayItem()` | Detect `- ` array items |
| `GetArrayItemValue()` | Extract array item content |
| `IsSectionHeader()` | Detect `section_name:` |
| `ShouldExitSection()` | Detect section boundary |
| `ParseVectorFromString()` | Parse `"X, Y, Z"` to FVector |
| `ParseRotatorFromString()` | Parse `"P, Y, R"` to FRotator |

---

## 6. Blueprint Generation

### 6.1 Event Graph Node Types (18+)

| Node Type | Properties | Description |
|-----------|------------|-------------|
| Event | event_name | BeginPlay, Tick, EndPlay, ReceiveBeginPlay |
| CustomEvent | event_name | User-defined events |
| CallFunction | function, class, target_self | Function calls (static/instance) |
| Branch | - | If/Then/Else branching |
| Sequence | num_outputs | Multiple execution outputs |
| VariableGet | variable_name | Read Blueprint variable |
| VariableSet | variable_name | Write Blueprint variable |
| PropertyGet | property_name, target_class | Read property via reflection |
| PropertySet | property_name, target_class | Write property via reflection |
| Delay | duration | Latent delay node |
| PrintString | message, duration | Debug output |
| DynamicCast | target_class | Runtime type casting |
| ForEachLoop | - | Array iteration |
| SpawnActor | actor_class_variable | Spawn actor in world |
| BreakStruct | struct_type | Decompose struct to pins |
| MakeArray | element_type, num_elements | Create array literal |
| GetArrayItem | - | Array index access |
| Self | - | Reference to owning Blueprint |

### 6.2 Pure Function Bypass (v2.5.2+)

Automatically handles exec connections to pure functions:

```yaml
# Manifest pattern (non-semantic exec to pure):
connections:
  - from: [Event_Activate, Then]
    to: [GetAvatarActor, Exec]      # Skipped - pure function
  - from: [GetAvatarActor, Exec]
    to: [CastToFather, Exec]        # Rerouted: Event_Activate → CastToFather
  - from: [GetAvatarActor, ReturnValue]
    to: [CastToFather, Object]      # Normal data flow
```

**Algorithm:**
1. Pre-pass identifies pure nodes (no exec pins)
2. Exec connections targeting pure nodes are skipped
3. Exec chains are rerouted to next impure node
4. Data connections work normally

### 6.3 Widget Tree Construction (v4.3)

**Supported Widget Types (25+):**
- Panels: CanvasPanel, VerticalBox, HorizontalBox, Overlay, UniformGridPanel, GridPanel
- Containers: Border, SizeBox, ScaleBox, ScrollBox, WidgetSwitcher
- Controls: Button, CheckBox, ComboBox, Slider, EditableText, EditableTextBox
- Display: TextBlock, RichTextBlock, Image, ProgressBar, Throbber, CircularThrobber
- Layout: Spacer, NativeWidgetHost

**Three-Pass Construction:**
```
Pass 1: Create all widgets
Pass 2: Build hierarchy (AddChild to panels)
Pass 3: Set root widget on WidgetBlueprint
```

**Slot Configuration:**
```yaml
slot:
  anchors: BottomCenter           # TopLeft, Center, BottomRight, etc.
  alignment: "0.5, 1.0"           # Pivot point
  position: "0, -50"              # Offset from anchor
  size: "200, 50"                 # Widget size
  h_align: Fill                   # Left, Center, Right, Fill
  v_align: Center                 # Top, Center, Bottom, Fill
  size_rule: Fill                 # Auto or Fill
  fill_weight: 1.0                # For Fill size rule
  padding: "10, 5, 10, 5"         # L, T, R, B padding
```

---

## 7. Commandlet Interface

### 7.1 Command Line Parameters

```bash
UnrealEditor-Cmd.exe "Project.uproject" -run=GasAbilityGenerator [options]

Required:
  -manifest="path/to/manifest.yaml"    # Manifest file path

Optional:
  -tags                                 # Generate only gameplay tags
  -assets                               # Generate only assets
  -all                                  # Generate both (default)
  -output="path/to/log.txt"            # Custom log file path
  -dryrun                              # Preview without changes (v3.0)
  -force                               # Override CONFLICT status (v3.0)
  -level="/Game/Maps/World"            # Load level for placement (v3.9.9)
  -dialoguecsv="path/to/dialogue.csv"  # Parse dialogue CSV (v4.0)
```

### 7.2 Execution Flow

```
Main()
  │
  ├─ Parse command line arguments
  ├─ Load manifest file
  │
  ├─ if (-dryrun):
  │    └─ Execute dry-run analysis
  │         └─ Return WillCreate/WillModify/WillSkip/Conflicted per asset
  │
  ├─ if (-tags or -all):
  │    └─ Generate gameplay tags to INI
  │
  ├─ if (-assets or -all):
  │    │
  │    ├─ Sort assets by dependency order
  │    │   1. Enumerations
  │    │   2. Input Actions/Contexts
  │    │   3. Gameplay Effects
  │    │   4. Gameplay Abilities
  │    │   5. Configurations
  │    │   6. Blueprints
  │    │   7. AI (BB, BT, Activities)
  │    │   8. Content (NPCs, Items, Quests)
  │    │   9. World Placement
  │    │
  │    ├─ foreach asset:
  │    │    ├─ Check metadata (CREATE/MODIFY/SKIP/CONFLICT)
  │    │    ├─ if CONFLICT and not -force: skip
  │    │    ├─ Run generator
  │    │    ├─ Update metadata
  │    │    └─ Add to GeneratedAssets set
  │    │
  │    └─ Retry deferred assets (max 3 attempts)
  │
  ├─ if (-level):
  │    └─ Place POI/NPC Spawner actors in level
  │
  └─ Generate report (v4.7)
       ├─ UGenerationReport DataAsset
       └─ JSON mirror file
```

### 7.3 Dependency Resolution (v2.6.7)

```cpp
// Deferred retry mechanism
TSet<FString> GeneratedAssets;
TArray<FDeferredAsset> DeferredAssets;
int32 MaxRetryAttempts = 3;

// On missing dependency:
DeferredAssets.Add({AssetDef, AttemptCount: 0});

// After main generation pass:
while (DeferredAssets.Num() > 0 && AnyProgress)
{
    foreach (Deferred in DeferredAssets)
    {
        if (AllDependenciesIn(GeneratedAssets))
            TryGenerate(Deferred);
        else if (++Deferred.AttemptCount >= MaxRetryAttempts)
            MarkFailed(Deferred);
    }
}
```

---

## 8. Report System (v4.7)

### 8.1 Report Structure

```cpp
UCLASS()
class UGenerationReport : public UDataAsset
{
    FString ManifestPath;
    FDateTime GenerationTime;
    FString GeneratorVersion;
    bool bIsDryRun;

    // Summary counts
    int32 NewCount;
    int32 SkippedCount;
    int32 FailedCount;
    int32 DeferredCount;

    // Dry-run counts
    int32 WillCreateCount;
    int32 WillModifyCount;
    int32 WillSkipCount;
    int32 ConflictedCount;

    TArray<FGenerationReportItem> Items;
    TArray<FGenerationError> Errors;
};

USTRUCT()
struct FGenerationReportItem
{
    FString AssetPath;          // /Game/Project/Folder/AssetName
    FString AssetName;          // AssetName
    FString GeneratorId;        // "GameplayAbility", "Material", etc.
    EGenerationStatus ExecutedStatus;  // New, Skipped, Failed, Deferred
    EDryRunStatus PlannedStatus;       // WillCreate, WillModify, etc.
};

USTRUCT()
struct FGenerationError
{
    FString ErrorCode;          // "E_UNKNOWN_EXPRESSION_TYPE"
    FString ContextPath;        // /Materials/M_Test/Expressions/node1
    FString Message;            // Human-readable error
    FString SuggestedFix;       // Actionable fix suggestion
};
```

### 8.2 Output Locations

| Format | Location | Purpose |
|--------|----------|---------|
| UDataAsset | `/Game/Generated/Reports/GenerationReport_YYYYMMDD_HHMMSS` | In-editor viewing |
| JSON | `Saved/GasAbilityGenerator/Reports/report_YYYYMMDD_HHMMSS.json` | CI/CD integration |
| Console | stdout | Commandlet output |

---

## 9. Table Editors (v4.8)

### 9.1 Editor Inventory

| Editor | File | Columns | Features |
|--------|------|---------|----------|
| NPC Table | SNPCTableEditor.cpp | 17 | Sync from/to assets, XLSX export |
| Dialogue Table | SDialogueTableEditor.cpp | 11 | Tree structure, token validation |
| Quest Table | SQuestTableEditor.cpp | 12 | State machine view, quest grouping |
| Item Table | SItemTableEditor.cpp | 16 | Dynamic visibility, type-based fields |

### 9.2 Common Systems

| System | Purpose |
|--------|---------|
| Validation Cache | `ValidationInputHash` for staleness detection |
| XLSX 3-Way Sync | `LastSyncedHash` for external edit detection |
| Soft Delete | `bDeleted` flag preserves undo capability |
| Generation Tracking | `LastGeneratedHash`, `AreAssetsOutOfDate()` |
| Re-entrancy Guard | `bIsBusy` prevents concurrent operations |
| Status Badges | Color-coded validation/sync indicators |

---

## 10. Limitations & Manual Steps

### 10.1 Known Limitations

| Area | Limitation | Workaround |
|------|------------|------------|
| Material Functions | Cannot resolve input count at parse time | W_FUNCTION_INPUT_MISMATCH reserved |
| Blueprint Inheritance | Some parent class properties not accessible | Use reflection-compatible properties |
| Niagara | Template required for duplication | Provide valid NS_ template |
| World Partition | Requires `-level` flag for placement | Manual placement without flag |
| Animation | Skeleton must exist | Pre-create skeleton assets |
| Plugin Classes | Generator can't resolve plugin paths | Classes work at runtime |

### 10.2 Manual Steps by Asset Type

| Asset Type | Manual Steps Required |
|------------|----------------------|
| GE_ | None (Full) |
| GA_ | Optional: Visual debugging, breakpoints |
| BP_ | Optional: Visual layout in editor |
| WBP_ | Optional: Visual polish, styling |
| M_ | Optional: Preview in Material Editor |
| NS_ | Requires: Valid template asset |
| AM_ | Requires: Skeleton asset exists |
| DBP_ | Optional: Test dialogue flow |
| Quest_ | Optional: Test quest progression |
| Level Actors | Optional: Fine-tune positions |

---

## 11. Performance Characteristics

### 11.1 Generation Metrics (155 Assets)

| Phase | Duration | Notes |
|-------|----------|-------|
| Manifest Parse | ~0.5s | YAML parsing to structs |
| Tag Generation | ~0.1s | INI file writes |
| Asset Generation | ~5-6s | All 155 assets |
| Report Generation | ~0.1s | JSON + DataAsset |
| **Total** | ~6s | Full regeneration |

### 11.2 Memory Usage

| Component | Approximate Size |
|-----------|------------------|
| FManifestData | ~2-5 MB (155 assets) |
| Generated Assets | ~50-100 MB (on disk) |
| Metadata Registry | ~100 KB |
| Session Caches | ~1 MB |

---

## 12. Version History Highlights

| Version | Key Features |
|---------|--------------|
| v4.10 | Material validation 6-guardrail system, 102 expression types |
| v4.8 | Quest Table Editor, Item Table Editor |
| v4.7 | Machine-readable report system, JSON export |
| v4.3 | Widget Blueprint layout automation (25+ widgets) |
| v4.0 | Quest pipeline, CSV dialogue parsing |
| v3.9 | Activity schedules, goals, quests, level placement |
| v3.8 | Full dialogue tree generation |
| v3.7 | NPC auto-create related assets |
| v3.5 | CharacterDefinition, DialogueBlueprint enhancements |
| v3.0 | Regen/Diff safety system, dry-run |
| v2.5 | Pure function bypass |

---

## Appendix A: Manifest Quick Reference

```yaml
# Project Configuration
project_root: /Game/FatherCompanion
tags_ini_path: Config/DefaultGameplayTags.ini

# Core GAS
tags:
  - tag: Ability.Example
    comment: Description

enumerations:
  - name: E_Example
    folder: Enums
    values: [Value1, Value2]

gameplay_effects:
  - name: GE_Example
    folder: Effects
    duration_policy: Instant
    modifiers:
      - attribute: Health
        operation: Additive
        magnitude: 50.0

gameplay_abilities:
  - name: GA_Example
    parent_class: NarrativeGameplayAbility
    folder: Abilities
    instancing_policy: InstancedPerActor
    net_execution_policy: ServerOnly
    tags:
      ability_tags: [Ability.Example]
    variables:
      - name: Damage
        type: Float
        default_value: "25.0"
    event_graph: ExampleGraph

# Materials (v4.10)
materials:
  - name: M_Example
    folder: Materials
    expressions:
      - id: color
        type: VectorParam
        properties:
          parameter_name: BaseColor
          default: "1.0, 0.5, 0.0, 1.0"
      - id: output
        type: MakeMaterialAttributes
    connections:
      - from: [color, RGBA]
        to: [output, BaseColor]
    output_connection:
      from: [output, Result]
      to: MaterialAttributes

# AI/Behavior
behavior_trees:
  - name: BT_Example
    blackboard: BB_Example
    folder: AI
    root_type: Selector
    nodes:
      - id: task1
        type: Task
        task_class: BTTask_MoveTo

# Content
npc_definitions:
  - name: NPC_Example
    folder: NPCs
    npc_name: Example NPC
    dialogue: DBP_Example
    auto_create_dialogue: true
```

---

**Document End**
