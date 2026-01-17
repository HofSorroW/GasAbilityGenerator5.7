# VFX System Reference (v4.10)

This document covers the VFX automation system introduced in v4.9 and significantly enhanced in v4.10.

## Overview

The VFX system provides comprehensive automation for visual effects assets:
- **Material Instance Constants (MIC_)** - Parameterized material variations
- **VFX Material Expressions** - 50+ expression types including VFX-specific nodes
- **Switch Expressions** - QualitySwitch, ShadingPathSwitch, FeatureLevelSwitch (v4.10)
- **MaterialFunctionCall** - 3-tier resolution for function references (v4.10)
- **Niagara Emitter Overrides** - Per-emitter parameter customization
- **FX Preset Library** - Reusable parameter configurations
- **LOD/Scalability Settings** - Automatic performance optimization

---

## v4.10 Major Enhancements

### Expression Type Tiers

**Tier 0 - Zero-Property Expressions:**
Expressions that require no configuration beyond creation.

| Expression | UE Class | Purpose |
|------------|----------|---------|
| PerInstanceFadeAmount | UMaterialExpressionPerInstanceFadeAmount | Instance fade for LOD |
| PerInstanceRandom | UMaterialExpressionPerInstanceRandom | Per-instance randomization |
| ObjectOrientation | UMaterialExpressionObjectOrientation | Object's orientation vector |
| ObjectRadius | UMaterialExpressionObjectRadius | Bounding sphere radius |
| ObjectPositionWS | UMaterialExpressionObjectPositionWS | World space object position |
| ActorPositionWS | UMaterialExpressionActorPositionWS | World space actor position |
| PreSkinnedPosition | UMaterialExpressionPreSkinnedPosition | Pre-skinning vertex position |
| PreSkinnedNormal | UMaterialExpressionPreSkinnedNormal | Pre-skinning vertex normal |
| VertexNormalWS | UMaterialExpressionVertexNormalWS | World space vertex normal |
| PixelNormalWS | UMaterialExpressionPixelNormalWS | World space pixel normal |
| TwoSidedSign | UMaterialExpressionTwoSidedSign | Front/back face detection |
| VertexTangentWS | UMaterialExpressionVertexTangentWS | World space vertex tangent |
| LightVector | UMaterialExpressionLightVector | Direction to light |
| CameraVector | UMaterialExpressionCameraVector | Direction to camera |
| ReflectionVector | UMaterialExpressionReflectionVectorWS | Reflection direction |
| ParticlePositionWS | UMaterialExpressionParticlePositionWS | Particle world position |
| ParticleRadius | UMaterialExpressionParticleRadius | Particle radius |
| ParticleRelativeTime | UMaterialExpressionParticleRelativeTime | Particle normalized age |
| ParticleMotionBlurFade | UMaterialExpressionParticleMotionBlurFade | Motion blur fade |
| ParticleRandom | UMaterialExpressionParticleRandom | Per-particle random |
| ParticleDirection | UMaterialExpressionParticleDirection | Particle velocity direction |
| ParticleSpeed | UMaterialExpressionParticleSpeed | Particle velocity magnitude |
| ParticleSize | UMaterialExpressionParticleSize | Particle size XY |
| ScreenPosition | UMaterialExpressionScreenPosition | Screen UV coordinates |
| ViewSize | UMaterialExpressionViewSize | Viewport dimensions |
| SceneTexelSize | UMaterialExpressionSceneTexelSize | Scene texture texel size |
| DeltaTime | UMaterialExpressionDeltaTime | Frame delta time |
| RealTime | UMaterialExpressionRealTime | Unpaused real time |
| EyeAdaptation | UMaterialExpressionEyeAdaptation | Auto-exposure value |
| AtmosphericFogColor | UMaterialExpressionAtmosphericFogColor | Atmospheric fog color |
| PrecomputedAOMask | UMaterialExpressionPrecomputedAOMask | Precomputed AO mask |
| GIReplace | UMaterialExpressionGIReplace | GI replacement value |

**Tier 1 - Parameterized Expressions:**
Expressions with configurable properties.

| Expression | Key Properties | Notes |
|------------|----------------|-------|
| Sobol | Index, Seed, ConstIndex, ConstSeed | Sobol sequence generator |
| TemporalSobol | Index, Seed, ConstIndex, ConstSeed | Temporally stable Sobol |
| QualitySwitch | default, low, high, medium, epic | Quality level switching |
| ShadingPathSwitch | default, deferred, forward, mobile | Shading path switching |
| FeatureLevelSwitch | default, es3_1, sm5, sm6 | Feature level switching |
| StaticSwitch | default_value, a, b, value | Compile-time branching |
| StaticBool | default_value | Static boolean value |
| StaticBoolParameter | parameter_name, default_value | Named static bool param |
| MaterialFunctionCall | function | 3-tier function resolution |
| SceneTexture | scene_texture_id | GBuffer/scene access |
| MakeMaterialAttributes | (20+ inputs) | Create material attrs struct |

**Tier 2 - Opt-In Expressions (Static):**
Expressions requiring explicit opt-in due to compile-time behavior.

| Expression | Behavior |
|------------|----------|
| StaticSwitch | Creates shader permutations |
| StaticBool | Compile-time constant |
| StaticBoolParameter | Named compile-time param |

---

### Switch Expression Enum Mappings

**CRITICAL:** These mappings are explicit and validated. Unknown keys produce hard errors.

#### QualitySwitch (EMaterialQualityLevel)

```cpp
// Actual enum order (counterintuitive!)
namespace EMaterialQualityLevel
{
    enum Type : uint8
    {
        Low    = 0,
        High   = 1,  // NOT after Medium!
        Medium = 2,
        Epic   = 3,
        Num    = 4
    };
}
```

| YAML Key | Enum Index | Notes |
|----------|------------|-------|
| `default` | Default input | Always connected |
| `low` | 0 | Low quality |
| `high` | 1 | High quality (index 1, not 2!) |
| `medium` | 2 | Medium quality (index 2, not 1!) |
| `epic` | 3 | Epic quality |

**NO "cinematic" level exists in material quality!** (Only in EPerQualityLevels for scalability)

#### ShadingPathSwitch (ERHIShadingPath)

```cpp
namespace ERHIShadingPath
{
    enum Type : int
    {
        Deferred = 0,
        Forward  = 1,
        Mobile   = 2,
        Num      = 3
    };
}
```

| YAML Key | Enum Index |
|----------|------------|
| `default` | Default input |
| `deferred` | 0 |
| `forward` | 1 |
| `mobile` | 2 |

#### FeatureLevelSwitch (ERHIFeatureLevel)

```cpp
namespace ERHIFeatureLevel
{
    enum Type : int
    {
        ES2_REMOVED = 0,  // DEPRECATED - do not use
        ES3_1       = 1,
        SM4_REMOVED = 2,  // REMOVED - do not use
        SM5         = 3,
        SM6         = 4,
        Num         = 5
    };
}
```

| YAML Key | Enum Index | Notes |
|----------|------------|-------|
| `default` | Default input | Always connected |
| `es3_1` | 1 | Mobile/ES3.1 |
| `sm5` | 3 | DX11/SM5 |
| `sm6` | 4 | DX12/SM6 |
| `es2` | 0 | **DEPRECATED** - produces warning |
| `sm4` | 2 | **REMOVED** - produces warning |

---

### MaterialFunctionCall 3-Tier Resolution

When referencing material functions, the generator uses a three-step lookup:

**Tier 1: Engine Path** (`/Engine/...`)
```yaml
- id: MakeFloat3
  type: MaterialFunctionCall
  function: /Engine/Functions/Engine_MaterialFunctions02/Utility/MakeFloat3
```

**Tier 2: Project Path** (`/Game/...`)
```yaml
- id: CustomWobble
  type: MaterialFunctionCall
  function: /Game/MyProject/Materials/Functions/MF_CustomWobble
```

**Tier 3: Manifest Lookup** (`MF_Name`)
```yaml
- id: EnergyPulse
  type: MaterialFunctionCall
  function: MF_EnergyPulse  # Resolves to generated function
```

**Resolution Order:**
1. If starts with `/Engine/` → Load directly from engine
2. If starts with `/Game/` → Load directly from project
3. Otherwise → Search session cache, then project paths

**Validation Rules:**
- `UMaterialFunctionInstance` is **blocked** (meta tag DisallowedClasses)
- Only `UMaterialFunction` (via `UMaterialFunctionInterface`) is valid
- Use `SetMaterialFunction()` method only (not `UpdateFromFunctionResource`)
- Validate inputs/outputs match via `GetInputsAndOutputs()`

---

### Manifest Structure (v4.10)

```yaml
materials:
  - name: M_AdvancedVFX
    folder: Materials/VFX
    blend_mode: Translucent
    shading_model: Unlit
    expressions:
      # Tier 0 - Zero property
      - id: ParticlePos
        type: ParticlePositionWS

      - id: InstanceRandom
        type: PerInstanceRandom

      # Tier 1 - Parameterized
      - id: QualityMux
        type: QualitySwitch
        inputs:
          default: DefaultColor
          low: LowColor
          medium: MediumColor
          high: HighColor
          epic: EpicColor

      - id: ShadingMux
        type: ShadingPathSwitch
        inputs:
          default: DeferredResult
          deferred: DeferredResult
          forward: ForwardResult
          mobile: MobileResult

      - id: FeatureMux
        type: FeatureLevelSwitch
        inputs:
          default: SM5Result
          es3_1: MobileResult
          sm5: SM5Result
          sm6: SM6Result

      # MaterialFunctionCall
      - id: EnergyPulse
        type: MaterialFunctionCall
        function: MF_EnergyPulse
        function_inputs:
          Intensity: IntensityParam
          Color: ColorParam

      # Sobol expressions
      - id: SobolNoise
        type: Sobol
        properties:
          const_index: 0
          const_seed: "0.5, 0.5"

      - id: TemporalNoise
        type: TemporalSobol
        properties:
          const_index: 0
          const_seed: "0.0, 0.0"

      # Static expressions (Tier 2 - opt-in)
      - id: UseHighQuality
        type: StaticBoolParameter
        name: UseHighQuality
        properties:
          default_value: true

      - id: QualityBranch
        type: StaticSwitch
        properties:
          default_value: true

    connections:
      - from: QualityMux
        to: Material
        to_input: EmissiveColor
```

---

## v4.9.1 Session Cache System

### Problem Solved

Material Instance Constants (MIC) require parent materials to exist before creation. When both the parent material and MIC are defined in the same manifest, the MIC generator couldn't find the parent material via `FindObject`/`LoadObject` because the Asset Registry hadn't been updated yet.

### Solution: Session-Level TMap Cache

```cpp
// GasAbilityGeneratorGenerators.h
class GASABILITYGENERATOR_API FMaterialGenerator : public FGeneratorBase
{
public:
    static FGenerationResult Generate(const FManifestMaterialDefinition& Definition);

    // v4.9.1: Session-level lookup for generated materials
    static UMaterialInterface* FindGeneratedMaterial(const FString& MaterialName);
    static void RegisterGeneratedMaterial(const FString& MaterialName, UMaterialInterface* Material);
    static void ClearGeneratedMaterialsCache();

    // v4.10: Session-level lookup for generated material functions
    static UMaterialFunctionInterface* FindGeneratedMaterialFunction(const FString& FunctionName);
    static void RegisterGeneratedMaterialFunction(const FString& FunctionName, UMaterialFunctionInterface* Function);
    static void ClearGeneratedMaterialFunctionsCache();

private:
    static TMap<FString, UMaterialInterface*> GeneratedMaterialsCache;
    static TMap<FString, UMaterialFunctionInterface*> GeneratedMaterialFunctionsCache;
};
```

### Lookup Chain (Materials)

1. **Session Cache** - Check `FMaterialGenerator::FindGeneratedMaterial()` first
2. **FindObject** - Search loaded packages
3. **LoadObject** - Load from disk

### Lookup Chain (Material Functions - v4.10)

1. **Session Cache** - Check `FMaterialGenerator::FindGeneratedMaterialFunction()` first
2. **Engine Path** - If starts with `/Engine/`, load directly
3. **Project Path** - If starts with `/Game/`, load directly
4. **Manifest Search** - Search common project paths for `MF_Name`

---

## Validation System (v4.10)

### Six Guardrails

1. **Quote Strip Before Trim**
   - Remove surrounding quotes before trimming whitespace
   - Handles YAML quoted strings correctly

2. **Case Hint on User Path**
   - Log case-sensitivity warnings on the user's original input
   - Don't show resolved/normalized paths in case warnings

3. **ContextPath Root Convention**
   - All context paths start with `/` for consistency
   - e.g., `/Materials/M_FatherCore/Expressions/QualityMux`

4. **Normalize Once, Reuse Everywhere**
   - Single normalization pass, store result
   - No re-normalization in validation vs. generation

5. **Field-Specific Normalizers**
   - `NormalizeKey()` for enum keys (lowercase, trim)
   - `NormalizeScalar()` for numeric values (parse, validate range)
   - `NormalizePath()` for asset paths (forward slashes, no trailing)

6. **Stable Diagnostic Ordering**
   - Sort errors/warnings before emitting
   - Deterministic output for diff-friendly CI

### Validation Error Codes

| Code | Severity | Description |
|------|----------|-------------|
| E_UNKNOWN_EXPRESSION_TYPE | Error | Unrecognized expression type |
| E_UNKNOWN_SWITCH_KEY | Error | Unknown key in switch inputs |
| E_DEPRECATED_SWITCH_KEY | Warning | Deprecated enum value (es2, sm4) |
| E_FUNCTION_NOT_FOUND | Error | MaterialFunctionCall target not found |
| E_FUNCTION_INSTANCE_BLOCKED | Error | UMaterialFunctionInstance not allowed |
| E_FUNCTION_INPUT_MISMATCH | Warning | Function input count doesn't match |
| E_MISSING_REQUIRED_INPUT | Error | Required switch input not connected |
| E_DUPLICATE_EXPRESSION_ID | Error | Duplicate expression ID in material |
| W_FUNCTION_PATH_NORMALIZED | Warning | Path was auto-corrected |

### Hallucination-Proof Pattern

All enum mappings use explicit tables with hard errors for unknown keys:

```cpp
// QualitySwitch key mapping
static const TMap<FString, int32> QualitySwitchKeyMap = {
    { TEXT("low"), 0 },
    { TEXT("high"), 1 },     // Index 1, not 2!
    { TEXT("medium"), 2 },   // Index 2, not 1!
    { TEXT("epic"), 3 }
};

// Validate key
FString NormalizedKey = Key.ToLower().TrimStartAndEnd();
if (!QualitySwitchKeyMap.Contains(NormalizedKey))
{
    EmitError(E_UNKNOWN_SWITCH_KEY, ContextPath,
        FString::Printf(TEXT("Unknown quality level '%s'. Valid: low, high, medium, epic"), *Key));
    return false;
}
```

---

## Whitelist Verification System

### v4.9.1 Alignment Fixes

The verification system compares expected assets (from manifest) against processed assets (from generators).

**Arrays Tracked (32 total):**

| Array | Generator | Notes |
|-------|-----------|-------|
| Enumerations | FEnumerationGenerator | |
| InputActions | FInputActionGenerator | |
| InputMappingContexts | FInputMappingContextGenerator | |
| GameplayEffects | FGameplayEffectGenerator | |
| GameplayAbilities | FGameplayAbilityGenerator | |
| ActorBlueprints | FActorBlueprintGenerator | |
| WidgetBlueprints | FWidgetBlueprintGenerator | |
| Blackboards | FBlackboardGenerator | |
| BehaviorTrees | FBehaviorTreeGenerator | |
| Materials | FMaterialGenerator | |
| FloatCurves | FFloatCurveGenerator | |
| AnimationMontages | FAnimationMontageGenerator | |
| AnimationNotifies | FAnimationNotifyGenerator | |
| DialogueBlueprints | FDialogueBlueprintGenerator | |
| EquippableItems | FEquippableItemGenerator | |
| Activities | FActivityGenerator | |
| AbilityConfigurations | FAbilityConfigurationGenerator | |
| ActivityConfigurations | FActivityConfigurationGenerator | |
| ItemCollections | FItemCollectionGenerator | |
| NarrativeEvents | FNarrativeEventGenerator | |
| GameplayCues | FGameplayCueGenerator | v4.0 |
| NPCDefinitions | FNPCDefinitionGenerator | |
| CharacterDefinitions | FCharacterDefinitionGenerator | |
| TaggedDialogueSets | FTaggedDialogueSetGenerator | |
| NiagaraSystems | FNiagaraSystemGenerator | |
| MaterialFunctions | FMaterialFunctionGenerator | |
| MaterialInstances | FMaterialInstanceGenerator | v4.9 |
| ActivitySchedules | FActivityScheduleGenerator | v3.9 |
| GoalItems | FGoalItemGenerator | v3.9 |
| Quests | FQuestGenerator | v3.9 |
| CharacterAppearances | FCharacterAppearanceGenerator | v4.8.3 |
| TriggerSets | FTriggerSetGenerator | v4.9 |

**Excluded from Expected Counts:**

| Array | Reason |
|-------|--------|
| EventGraphs | Embedded in abilities/blueprints, not standalone |
| FXPresets | Config data applied to NiagaraSystems, not assets |
| POIPlacements | Level actors, not content assets |
| NPCSpawnerPlacements | Level actors, not content assets |

---

## Material Instance Constants (MIC_)

### Manifest Structure

```yaml
material_instances:
  - name: MIC_FatherCore_Alert
    folder: Materials/Instances
    parent_material: M_FatherCore
    scalar_params:
      - name: PulseSpeed
        value: 2.0
      - name: Intensity
        value: 1.5
    vector_params:
      - name: CoreColor
        value: [1.0, 0.3, 0.0, 1.0]  # Orange alert color
    texture_params:
      - name: DetailTexture
        value: /Game/Textures/T_AlertPattern
```

### Generated Asset

Creates `UMaterialInstanceConstant` with:
- Parent material reference
- Scalar parameter overrides
- Vector parameter overrides (LinearColor)
- Texture parameter overrides

---

## VFX Material Expressions (Full List)

### Basic Math
| Expression | Purpose |
|------------|---------|
| Add | Addition |
| Subtract | Subtraction |
| Multiply | Multiplication |
| Divide | Division |
| Power | Exponentiation |
| Abs | Absolute value |
| Floor | Round down |
| Ceil | Round up |
| Frac | Fractional part |
| Fmod | Floating modulo |
| Saturate | Clamp 0-1 |
| Clamp | Clamp to range |
| OneMinus | 1 - x |
| Lerp | Linear interpolate |
| Min | Minimum |
| Max | Maximum |
| Sine | Sin function |
| Cosine | Cos function |
| Tangent | Tan function |

### Vector Operations
| Expression | Purpose |
|------------|---------|
| Normalize | Unit vector |
| DotProduct | Vector dot |
| CrossProduct | Vector cross |
| Distance | Point distance |
| Length | Vector magnitude |
| AppendVector | Combine components |
| ComponentMask | Extract channels |
| BreakOutFloat2 | Split float2 |
| BreakOutFloat3 | Split float3 |
| BreakOutFloat4 | Split float4 |

### Texture
| Expression | Purpose |
|------------|---------|
| TextureSample | Sample texture |
| TextureSampleParameter2D | Parameterized sample |
| TextureCoordinate | UV coordinates |
| Panner | Animated UV |
| Rotator | Rotating UV |

### Particle/VFX
| Expression | Purpose |
|------------|---------|
| ParticleColor | Particle vertex color |
| ParticleSubUV | Flipbook animation |
| DynamicParameter | Niagara control |
| VertexColor | Mesh vertex colors |
| DepthFade | Soft edges |
| SphereMask | Radial falloff |
| ParticlePositionWS | Particle position |
| ParticleRadius | Particle size |
| ParticleRelativeTime | Particle age |
| ParticleRandom | Per-particle random |
| ParticleDirection | Velocity direction |
| ParticleSpeed | Velocity magnitude |
| ParticleSize | Particle size XY |
| ParticleMotionBlurFade | Motion blur |

### World/Camera
| Expression | Purpose |
|------------|---------|
| WorldPosition | World coordinates |
| ObjectPosition | Object origin |
| ActorPositionWS | Actor origin |
| CameraPositionWS | Camera position |
| CameraVector | To-camera direction |
| ReflectionVector | Reflection dir |
| LightVector | To-light direction |
| PixelDepth | Pixel scene depth |
| SceneDepth | Scene depth buffer |

### Constants
| Expression | Purpose |
|------------|---------|
| Constant | Float constant |
| Constant2Vector | Float2 constant |
| Constant3Vector | Float3 constant |
| Constant4Vector | Float4 constant |
| ScalarParameter | Float parameter |
| VectorParameter | Color parameter |
| Time | Game time |
| DeltaTime | Frame time |
| RealTime | Unpaused time |

---

## Niagara Emitter Overrides

Per-emitter parameter customization without duplicating templates:

```yaml
niagara_systems:
  - name: NS_FatherCompanion
    template: /Game/VFX/NS_BaseGlow
    emitter_overrides:
      - emitter: CoreGlow
        enabled: true
        parameters:
          User.Color: [0.2, 0.5, 1.0, 1.0]
          User.Intensity: 2.0
      - emitter: OuterParticles
        enabled: false  # Disable this emitter
```

---

## FX Preset Library

Reusable parameter configurations with inheritance:

```yaml
fx_presets:
  - name: Preset_BaseGlow
    parameters:
      User.Intensity: 1.0
      User.FadeTime: 0.5

  - name: Preset_AlertGlow
    base_preset: Preset_BaseGlow  # Inherits parameters
    parameters:
      User.Intensity: 2.5  # Override
      User.PulseSpeed: 3.0  # Add new

niagara_systems:
  - name: NS_AlertEffect
    template: /Game/VFX/NS_BaseGlow
    preset: Preset_AlertGlow  # Apply preset
    user_parameters:
      User.Color: [1.0, 0.0, 0.0, 1.0]  # Override preset
```

Inheritance resolution order:
1. Base presets (recursive)
2. Child preset parameters
3. Manifest `user_parameters` (highest priority)

---

## LOD/Scalability Settings

Automatic performance optimization:

```yaml
niagara_systems:
  - name: NS_HighDetailEffect
    template: /Game/VFX/NS_Particles
    cull_distance: 5000.0
    cull_distance_low: 2000.0
    cull_distance_medium: 3000.0
    cull_distance_high: 4000.0
    cull_distance_epic: 5000.0
    significance_distance: 1000.0
    max_particle_budget: 10000
    scalability_mode: Self  # Self, System, or Component
    allow_culling_for_local_players: false
```

---

## Files Modified (v4.10)

| File | Changes |
|------|---------|
| GasAbilityGeneratorGenerators.h | Added material function cache, expression registry |
| GasAbilityGeneratorGenerators.cpp | Rewritten expression system, switch expressions, validation |
| GasAbilityGeneratorTypes.h | New switch input structs, function call fields |
| GasAbilityGeneratorParser.cpp | Parse switch inputs, function_inputs |
| GasAbilityGeneratorCommandlet.cpp | Updated cache clearing |
| GasAbilityGeneratorWindow.cpp | Updated cache clearing |

---

## Verification

Current manifest: **155 assets expected = 155 processed**

```
--- Verification (Whitelist Check) ---
Expected (from whitelist): 155
Processed: 155

VERIFICATION PASSED: All whitelist assets processed, counts match, no duplicates
```
