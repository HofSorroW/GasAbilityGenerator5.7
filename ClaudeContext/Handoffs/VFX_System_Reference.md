# VFX System Reference (v4.9.1)

This document covers the VFX automation system introduced in v4.9 and refined in v4.9.1.

## Overview

The VFX system provides comprehensive automation for visual effects assets:
- **Material Instance Constants (MIC_)** - Parameterized material variations
- **VFX Material Expressions** - 20+ particle-specific expression types
- **Niagara Emitter Overrides** - Per-emitter parameter customization
- **FX Preset Library** - Reusable parameter configurations
- **LOD/Scalability Settings** - Automatic performance optimization

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

private:
    static TMap<FString, UMaterialInterface*> GeneratedMaterialsCache;
};
```

### Lookup Chain

MIC generator uses a three-step lookup:

1. **Session Cache** - Check `FMaterialGenerator::FindGeneratedMaterial()` first
2. **FindObject** - Search loaded packages
3. **LoadObject** - Load from disk

```cpp
// In FMaterialInstanceGenerator::Generate()
ParentMaterial = FMaterialGenerator::FindGeneratedMaterial(ParentMatName);
if (ParentMaterial)
{
    LogGeneration(TEXT("    -> FOUND in session cache"));
}
else
{
    // Fallback to FindObject/LoadObject chain...
}
```

### Cache Lifecycle

Cache is cleared at the start of each generation session:

```cpp
// Commandlet (GasAbilityGeneratorCommandlet.cpp:520)
FMaterialGenerator::ClearGeneratedMaterialsCache();

// Editor Window (GasAbilityGeneratorWindow.cpp:658)
FMaterialGenerator::ClearGeneratedMaterialsCache();
```

### Thread Safety

The cache is static but generation is single-threaded within each session. No mutex required for current usage pattern.

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

### Bidirectional Leak Detection

```cpp
// VerifyGenerationComplete() in commandlet

// Detect missing assets (expected but not generated)
for (const FString& ExpectedName : ExpectedAssets)
{
    if (!ProcessedAssets.Contains(ExpectedName))
    {
        MissingAssets.Add(ExpectedName);  // LEAK: expected but not generated
    }
}

// Detect unexpected assets (generated but not expected)
for (const FString& ProcessedName : ProcessedAssets)
{
    if (!ExpectedAssets.Contains(ProcessedName))
    {
        UnexpectedAssets.Add(ProcessedName);  // LEAK: generated but not expected
    }
}
```

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

## VFX Material Expressions

Added 20+ expression types to FMaterialGenerator:

| Expression | Purpose |
|------------|---------|
| ParticleColor | Per-particle vertex color |
| ParticleSubUV | Flipbook animation |
| DynamicParameter | Runtime parameter control |
| VertexColor | Mesh vertex colors |
| DepthFade | Soft particle edges |
| SphereMask | Radial falloff |
| CameraPositionWS | Camera-relative effects |
| ObjectBounds | Object-space calculations |
| Saturate | Clamp 0-1 |
| Abs | Absolute value |
| Cosine | Trigonometric |
| Floor | Round down |
| Frac | Fractional part |
| Normalize | Unit vector |
| DotProduct | Vector projection |
| CrossProduct | Perpendicular vector |
| Distance | Point distance |
| WorldPosition | World coordinates |
| PixelDepth | Scene depth at pixel |
| SceneDepth | Scene depth buffer |

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
    cull_distance_cinematic: 6000.0
    significance_distance: 1000.0
    max_particle_budget: 10000
    scalability_mode: Self  # Self, System, or Component
    allow_culling_for_local_players: false
```

---

## Files Modified (v4.9.1)

| File | Changes |
|------|---------|
| GasAbilityGeneratorGenerators.h | Added session cache methods to FMaterialGenerator |
| GasAbilityGeneratorGenerators.cpp | Implemented cache, MIC lookup chain |
| GasAbilityGeneratorTypes.h | Fixed whitelist alignment |
| GasAbilityGeneratorCommandlet.cpp | Added cache clearing, MIC log |
| GasAbilityGeneratorWindow.cpp | Added cache clearing |

---

## Verification

Current manifest: **155 assets expected = 155 processed**

```
--- Verification (Whitelist Check) ---
Expected (from whitelist): 155
Processed: 155

VERIFICATION PASSED: All whitelist assets processed, counts match, no duplicates
```
