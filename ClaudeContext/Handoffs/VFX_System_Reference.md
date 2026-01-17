# VFX System Reference (v4.11)

This document covers the VFX automation system introduced in v4.9 and significantly enhanced in v4.10/v4.11.

## Overview

The VFX system provides comprehensive automation for visual effects assets:
- **Material Instance Constants (MIC_)** - Parameterized material variations
- **VFX Material Expressions** - 50+ expression types including VFX-specific nodes
- **Switch Expressions** - QualitySwitch, ShadingPathSwitch, FeatureLevelSwitch (v4.10)
- **MaterialFunctionCall** - 3-tier resolution for function references (v4.10)
- **Niagara Emitter Overrides** - Per-emitter parameter customization
- **FX Preset Library** - Reusable parameter configurations
- **LOD/Scalability Settings** - Automatic performance optimization
- **Emitter Enable/Disable Controls** - Soft and structural emitter toggling (v4.11)

---

## v4.11 Emitter Enable/Disable Controls

### Overview

v4.11 introduces a two-tier emitter enable/disable system that distinguishes between runtime-safe operations and compile-required structural changes.

### Doctrine: Compile Only for Structural Changes

| Operation Type | Recompile Required? | Method |
|----------------|---------------------|--------|
| User.* parameter changes | No | `UserParams.AddParameter()` |
| Soft emitter enable/disable | No | `{Emitter}.User.Enabled` parameter |
| **Structural emitter enable/disable** | **Yes** | `FNiagaraEmitterHandle::SetIsEnabled()` |

### Manifest Schema

```yaml
niagara_systems:
  - name: NS_FatherCompanion
    template: /Game/VFX/NS_BaseGlow
    emitter_overrides:
      - emitter: CoreGlow
        # Soft enable - sets User.Enabled parameter (no recompile)
        enabled: true
        parameters:
          User.Intensity: 2.0

      - emitter: OuterParticles
        # Structural disable - calls SetIsEnabled() (triggers recompile)
        structural_enabled: false

      - emitter: SparksEmitter
        # Both can be specified (structural takes precedence for visibility)
        enabled: true           # User.Enabled = true (runtime toggle)
        structural_enabled: true # Emitter is structurally enabled
        parameters:
          User.SpawnRate: 50.0
```

### Field Definitions

| Field | Type | Default | Purpose |
|-------|------|---------|---------|
| `enabled` | bool | true | Sets `{Emitter}.User.Enabled` parameter (runtime, no recompile) |
| `structural_enabled` | bool | true | Calls `SetIsEnabled()` on emitter handle (compile-time, triggers recompile) |

### Sentinel Pattern

The generator uses sentinel fields to distinguish "not specified" from "explicitly set to default":

| Sentinel | Controls | Behavior if False |
|----------|----------|-------------------|
| `bHasEnabled` | `enabled:` | Skip soft enable logic |
| `bHasStructuralEnabled` | `structural_enabled:` | Skip structural enable logic |

**YAML not specified → Sentinel stays false → Generator skips that operation**

This prevents default values from triggering unintended operations.

### Generation Flow

```
1. Duplicate template system OR create from scratch
2. Apply property changes (warmup, bounds, LOD, etc.) - no recompile
3. Apply User.* parameters - no recompile
4. For each emitter override:
   a. If bHasEnabled: Set {Emitter}.User.Enabled parameter
   b. If bHasStructuralEnabled: Call SetIsEnabled(), set bNeedsRecompile if changed
5. Compile gating:
   a. bInitialCompileRequired: Always true for new system identity (both from-scratch and duplicates)
   b. bNeedsRecompile: True only if structural emitter changes occurred
   c. If either flag is true: RequestCompile(false) + WaitForCompilationComplete(true, false)
6. Environment-aware safety gate (see below)
7. Save asset
```

### Compile Doctrine

**Initial compile is required for new system identity; additional compile only for structural emitter deltas.**

| Trigger | Condition | Flags Set |
|---------|-----------|-----------|
| From-scratch creation | Always | `bInitialCompileRequired = true` |
| Template duplication | Always (new identity) | `bDuplicatedFromTemplate = true`, `bInitialCompileRequired = true` |
| Structural emitter change | `SetIsEnabled()` changes state | `bNeedsRecompile = true` |
| User.* parameter change | Never | (no flags) |

**Compile Gating Logic:**
```cpp
bool bNeedsCompile = bNeedsRecompile || bInitialCompileRequired;
if (bNeedsCompile) { RequestCompile(false); WaitForCompilationComplete(true, false); }
```

**Call Signature:** `WaitForCompilationComplete(true, false)` includes GPU shaders but no progress dialog. Do not change without retesting headless behavior.

### Safety Mechanisms

| Mechanism | Purpose |
|-----------|---------|
| Template readiness pre-check | If template isn't ready, request compile + wait (best-effort) before duplication |
| Compile gating | Initial compile for new identity + extra compile for structural deltas |
| Environment-aware readiness policy | Strict in editor, headless-aware in -nullrhi |
| Diagnostic logging | ReadyCheck with all state flags for debugging |

### v4.11 Environment-Aware Readiness Policy

**Problem:** `IsReadyToRun()` depends on GPU/render systems that don't initialize under `-nullrhi` headless mode. Niagara systems can report `IsReadyToRun()=false` even after successful compilation.

**Solution:** Tiered policy based on environment:

| Environment | IsReadyToRun=false | Action |
|-------------|-------------------|--------|
| Editor (real RHI) | Fail | **STRICT** - fail before save |
| Headless (-nullrhi) | Pass (if escape hatch applies) | **WARN** - save with warning |

**Escape Hatch Requirements (all must be true):**
- `bIsHeadless == true` (running commandlet with no render capability)
- `bDidAttemptCompile == true` (compilation was requested)
- `EmitterCount > 0` (minimal sanity check - not proof of validity, especially for from-scratch systems)

**Explicit Contract:**

| Environment | Guarantee |
|-------------|-----------|
| **Editor / Real RHI** | Saved systems pass `IsReadyToRun()` or generation fails. **Readiness guaranteed.** |
| **Headless -nullrhi** | HEADLESS-SAVED systems are **authoring artifacts** that must be validated in editor before shipping. **Readiness NOT guaranteed.** |

**Policy B Trade-off:** Policy B trades correctness guarantees for pipeline continuity in headless mode. It does not assert functional validity—a from-scratch system may have emitter handles but be missing required scripts/modules. This is acceptable for testing but requires editor validation before shipping.

**Revert to Policy A:** Once real template systems exist for all NS_ entries OR generation runs under real RHI in CI/editor mode, add `&& bDuplicatedFromTemplate` to the escape hatch condition to restore conservative behavior.

### RESULT Footer Schema (v1)

Commandlet emits machine-parseable footers for reliable wrapper parsing:

```
RESULT: New=<N> Skipped=<N> Failed=<N> Deferred=<N> Total=<N> Headless=<true/false>
RESULT_HEADLESS_SAVED: Count=<N>  # Only present if Count > 0
```

**Schema Contract:**
- **Order**: Keys always appear in the order shown above. Never reorder.
- **Presence**: All keys in RESULT always present, even if 0.
- **Versioning**: If adding new keys (e.g., `Updated=`), do so as a schema bump. Old keys remain.

**Field Definitions:**

| Field | Meaning |
|-------|---------|
| `New` | Assets created successfully in this run |
| `Skipped` | Assets already exist (unchanged, not regenerated) |
| `Failed` | Generation errors (asset not created/saved) |
| `Deferred` | Skipped due to missing prerequisite; will be retried. **Does NOT count as success.** |
| `Total` | New + Skipped + Failed + Deferred |
| `Headless` | `true` if running under `-nullrhi` (no GPU/render) |
| `Count` (RESULT_HEADLESS_SAVED) | Subset of `New` that were saved under headless escape hatch. **Require editor verification before shipping.** |

**Wrapper Guidance:**
- Parse `grep "^RESULT:"` for primary results
- Parse `grep "^RESULT_HEADLESS_SAVED:"` for CI dashboard warnings
- Ignore other log lines for result determination

**Diagnostic Logging:**
```
ReadyCheck: IsReadyToRun=false emitters=1 headless=true did_compile=true from_template=false initial_compile=true
```

**Headless-Saved Marker:**
```
Created Niagara System: NS_Example (HEADLESS-SAVED - verify in editor)
```

**Manifest Guidance:**
```yaml
niagara_systems:
  # From-scratch systems can be generated in editor mode with real RHI (best).
  # In headless -nullrhi they are saved with a warning if escape hatch applies.
  - name: NS_CustomEffect
    effect_type: ambient

  # Template-based systems are ideal for headless mode and regression tests.
  - name: NS_TemplateVariant
    template_system: "/Niagara/DefaultAssets/Templates/Systems/MinimalLightweight.MinimalLightweight"
```

### Use Cases

**Soft Enable (Runtime Toggle):**
```yaml
emitter_overrides:
  - emitter: DebugParticles
    enabled: false  # Can be toggled at runtime via User.Enabled
```
- Good for: Debug emitters, optional visual layers, runtime-togglable effects
- No recompile, fast iteration

**Structural Disable (Permanent Removal):**
```yaml
emitter_overrides:
  - emitter: ExpensiveEmitter
    structural_enabled: false  # Completely removed from system
```
- Good for: LOD variants, platform-specific systems, permanent removal
- Triggers recompile, reduces runtime overhead

**Combined Pattern:**
```yaml
emitter_overrides:
  - emitter: OptionalEffect
    enabled: true              # Runtime default
    structural_enabled: true   # Emitter exists in system
```
- Emitter is structurally present but can be toggled at runtime

### API Reference (UE5.7)

```cpp
// NiagaraEmitterHandle.h:56,65
NIAGARA_API FName GetName() const;
NIAGARA_API bool SetIsEnabled(bool bInIsEnabled, UNiagaraSystem& InOwnerSystem, bool bRecompileIfChanged);

// NiagaraSystem.h:309,438,444
NIAGARA_API TArray<FNiagaraEmitterHandle>& GetEmitterHandles();
NIAGARA_API bool RequestCompile(bool bForce, FNiagaraSystemUpdateContext* = nullptr, const ITargetPlatform* = nullptr);
NIAGARA_API void WaitForCompilationComplete(bool bIncludingGPUShaders = false, bool bShowProgress = true);
NIAGARA_API bool IsReadyToRun() const;
```

### Hash Updates

`FManifestNiagaraEmitterOverride::ComputeHash()` includes all four fields:
```cpp
Hash ^= (bEnabled ? 1ULL : 0ULL) << 8;
Hash ^= (bHasEnabled ? 1ULL : 0ULL) << 9;
Hash ^= (bStructuralEnabled ? 1ULL : 0ULL) << 10;
Hash ^= (bHasStructuralEnabled ? 1ULL : 0ULL) << 11;
```

This ensures regen/diff system detects changes to enable/disable settings.

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
   - Implementation: `FMaterialExprValidationResult::StripQuotesAndTrim()`

2. **Case Hint on User Path**
   - Log case-sensitivity warnings on the user's original input
   - Don't show resolved/normalized paths in case warnings
   - Implementation: `FMaterialExprDiagnostic::UserInput` field stores original input

3. **ContextPath Root Convention**
   - All context paths start with `/` for consistency
   - Format: `/Materials/{AssetName}/{Section}/{ExpressionId}`
   - Implementation: `FMaterialExprValidationResult::MakeContextPath()`

4. **Normalize Once, Reuse Everywhere**
   - Single normalization pass, store result
   - No re-normalization in validation vs. generation
   - Implementation: `TypeLower = NormalizeKey(ExprDef.Type)` called once per expression

5. **Field-Specific Normalizers**
   - `NormalizeKey()` for enum keys (lowercase, trim) - used by switch key validation
   - `NormalizeScalar()` for numeric values (parse, validate range) - available for property validation
   - `NormalizePath()` for asset paths (forward slashes, no trailing) - used by MaterialFunctionCall

6. **Stable Diagnostic Ordering**
   - Sort errors/warnings before emitting
   - Deterministic output for diff-friendly CI
   - Implementation: `FMaterialExprDiagnostic::operator<` sorts by path, then code, then message

### Validation Error Codes

| Code | Severity | Description |
|------|----------|-------------|
| E_UNKNOWN_EXPRESSION_TYPE | Error | Unrecognized expression type (97 known types) |
| E_UNKNOWN_SWITCH_KEY | Error | Unknown key in switch inputs |
| E_FUNCTION_NOT_FOUND | Error | MaterialFunctionCall target not found |
| E_FUNCTION_INSTANCE_BLOCKED | Error | UMaterialFunctionInstance not allowed |
| E_MISSING_REQUIRED_INPUT | Error | Required switch input not connected |
| E_DUPLICATE_EXPRESSION_ID | Error | Duplicate expression ID in material |
| E_EXPRESSION_REFERENCE_INVALID | Error | Connection references non-existent expression |
| W_DEPRECATED_SWITCH_KEY | Warning | Deprecated enum value (es2, sm4) |
| W_FUNCTION_INPUT_MISMATCH | Warning | Function input count doesn't match (reserved - requires asset loading) |
| W_FUNCTION_PATH_NORMALIZED | Warning | Path was auto-corrected |
| W_EXPRESSION_UNUSED | Warning | Expression defined but never connected |

**Naming Convention:** `E_` prefix = Error (fatal, blocks generation), `W_` prefix = Warning (logged but continues)

### 3-Pass Validation Algorithm

```
Pass 1: Validate each expression
├── Check for duplicate expression ID → E_DUPLICATE_EXPRESSION_ID
├── Normalize type via NormalizeKey()
├── Validate type in KnownExpressionTypes → E_UNKNOWN_EXPRESSION_TYPE
├── For QualitySwitch:
│   ├── Validate keys → E_UNKNOWN_SWITCH_KEY
│   └── Check 'default' input exists → E_MISSING_REQUIRED_INPUT
├── For ShadingPathSwitch:
│   ├── Validate keys → E_UNKNOWN_SWITCH_KEY
│   └── Check 'default' input exists → E_MISSING_REQUIRED_INPUT
├── For FeatureLevelSwitch:
│   ├── Validate keys → E_UNKNOWN_SWITCH_KEY or W_DEPRECATED_SWITCH_KEY
│   └── Check 'default' input exists → E_MISSING_REQUIRED_INPUT
├── For MaterialFunctionCall:
│   ├── Validate function path → E_FUNCTION_NOT_FOUND, W_FUNCTION_PATH_NORMALIZED
│   └── Check for FunctionInstance in path → E_FUNCTION_INSTANCE_BLOCKED
└── Track referenced expression IDs for Pass 3

Pass 2: Validate connections
├── Check source expression exists → E_EXPRESSION_REFERENCE_INVALID
├── Check target expression exists (unless "Material") → E_EXPRESSION_REFERENCE_INVALID
└── Track connected expression IDs for Pass 3

Pass 3: Check for unused expressions
└── Warn if expression defined but never referenced → W_EXPRESSION_UNUSED

Final: Sort diagnostics (Guardrail #6)
```

### Hallucination-Proof Pattern

All enum mappings use explicit tables with hard errors for unknown keys:

```cpp
// QualitySwitch key mapping (GasAbilityGeneratorGenerators.cpp:340-345)
static const TMap<FString, int32> QualitySwitchKeyMap = {
    { TEXT("low"), 0 },
    { TEXT("high"), 1 },     // Index 1, NOT 2!
    { TEXT("medium"), 2 },   // Index 2, NOT 1!
    { TEXT("epic"), 3 }
};

// Validate key (GasAbilityGeneratorGenerators.cpp:5378-5387)
FString KeyLower = FMaterialExprValidationResult::NormalizeKey(InputPair.Key);
if (KeyLower != TEXT("default") && !QualitySwitchKeyMap.Contains(KeyLower))
{
    FMaterialExprDiagnostic Diag(
        EMaterialExprValidationCode::E_UNKNOWN_SWITCH_KEY,
        ContextPath,
        InputPair.Key,  // Guardrail #2: User's original input
        FString::Printf(TEXT("Unknown QualitySwitch key '%s'. Valid: default, low, high, medium, epic"), *InputPair.Key),
        true);  // fatal = true
    ValidationResult.AddDiagnostic(Diag);
}
```

### Known Expression Types (97)

The validation system maintains an exhaustive `KnownExpressionTypes` set:

| Category | Types |
|----------|-------|
| Basic Math | constant, constant2vector, constant3vector, constant4vector, add, subtract, multiply, divide, power, abs, floor, ceil, frac, fmod, saturate, clamp, oneminus, lerp, min, max, sine, cosine, tangent, arcsine, arccosine, arctangent |
| Vector Ops | normalize, dotproduct, crossproduct, distance, length, appendvector, componentmask, breakoutfloat2, breakoutfloat3, breakoutfloat4 |
| Texture | texturesample, texturesampleparameter2d, texturecoordinate, panner, rotator |
| Parameters | scalarparameter, vectorparameter, staticboolparameter, staticbool |
| Time | time, deltatime, realtime |
| World/Camera | worldposition, objectposition, actorpositionws, objectpositionws, camerapositionws, cameravector, reflectionvector, lightvector, pixeldepth, scenedepth, screenposition, viewsize, scenetexelsize |
| Particle/VFX | particlecolor, particlesubuv, dynamicparameter, vertexcolor, depthfade, spheremask, particlepositionws, particleradius, particlerelativetime, particlerandom, particledirection, particlespeed, particlesize, particlemotionblurfade |
| Tier 0 | perinstancefadeamount, perinstancerandom, objectorientation, objectradius, preskinnedposition, preskinnednormal, vertexnormalws, pixelnormalws, vertextangentws, twosidedsign, eyeadaptation, atmosphericfogcolor, precomputedaomask, gireplace |
| Switch | qualityswitch, shadingpathswitch, featurelevelswitch, staticswitch |
| Functions | materialfunctioncall |
| Sobol | sobol, temporalsobol |
| Scene | scenetexture |
| Complex | fresnel, noise, makematerialattributes, if, customexpression |

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

## Files Modified

### v4.11

| File | Changes |
|------|---------|
| GasAbilityGeneratorTypes.h | Added `bHasEnabled`, `bStructuralEnabled`, `bHasStructuralEnabled` to `FManifestNiagaraEmitterOverride`, updated `ComputeHash()` |
| GasAbilityGeneratorParser.cpp | Set `bHasEnabled=true` when parsing `enabled:`, added `structural_enabled:` parsing block |
| GasAbilityGeneratorGenerators.cpp | Added `Modify()` call, `bNeedsRecompile` flag, conditional soft/structural enable, conditional compile, `IsReadyToRun()` gate before save |

### v4.10

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
