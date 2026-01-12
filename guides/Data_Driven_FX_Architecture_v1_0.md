# Data-Driven FX Architecture for Unreal Engine 5 (Niagara)

> **Authoring once. Driving everything with data.**
>
> This document defines a complete, production-ready, data-driven FX pipeline for Unreal Engine 5 using Niagara, designed to work within Epic's intentional API boundaries.

---

## PRIME DIRECTIVE

**Niagara is a simulation VM, not an authoring API.**

If a solution requires:
- editing emitter graphs
- adding modules programmatically
- compiling Niagara from tools

❌ It is invalid.

All variability must live in **data**.

---

## 1. FX TAXONOMY (WHAT YOU ARE BUILDING)

You are not building effects — you are building **FX behaviors** composed of orthogonal dimensions.

### FX Dimensions

| Dimension | Examples |
|-----------|----------|
| Event | Impact, Loop, Burst, Trail |
| Element | Fire, Ice, Electric, Poison |
| Intensity | Low / Medium / High |
| Scale | Small / Medium / Large |
| Context | World, Character, Weapon |
| Quality | Low / High / Cinematic |

FX identity is a **combination**, not a unique graph.

---

## 2. FX DESCRIPTOR (AUTHORITATIVE DATA)

The FX Descriptor is the **single source of truth**.

```cpp
USTRUCT(BlueprintType)
struct FFXDescriptor
{
    GENERATED_BODY()

    // Identity
    UPROPERTY(EditAnywhere)
    FGameplayTag FXTag;

    // Emission
    UPROPERTY(EditAnywhere)
    float SpawnRate;

    UPROPERTY(EditAnywhere)
    FVector2D Lifetime;

    UPROPERTY(EditAnywhere)
    int32 MaxParticles;

    // Appearance
    UPROPERTY(EditAnywhere)
    FLinearColor Color;

    UPROPERTY(EditAnywhere)
    FVector2D Size;

    UPROPERTY(EditAnywhere)
    float Opacity;

    // Motion
    UPROPERTY(EditAnywhere)
    FVector InitialVelocity;

    UPROPERTY(EditAnywhere)
    float NoiseStrength;

    UPROPERTY(EditAnywhere)
    float GravityScale;

    // Composition
    UPROPERTY(EditAnywhere)
    bool bSparks;

    UPROPERTY(EditAnywhere)
    bool bSmoke;

    UPROPERTY(EditAnywhere)
    bool bLight;

    // Rendering
    UPROPERTY(EditAnywhere)
    float Emissive;

    // LOD
    UPROPERTY(EditAnywhere)
    float CullDistance;

    UPROPERTY(EditAnywhere)
    int32 LODLevel;
};
```

### Properties

- Serializable
- Network-safe
- Tool-friendly
- Niagara-agnostic

---

## 3. SEMANTIC → NUMERIC RESOLUTION

Designers author meaning, not numbers.

```cpp
enum class EFXIntensity { Low, Medium, High };
enum class EFXElement { Fire, Ice, Shock };
```

Resolver example:

```cpp
FFXDescriptor ResolveFX(EFXIntensity Intensity, EFXElement Element);
```

Example mapping:

| Semantic | Result |
|----------|--------|
| Fire + High | SpawnRate = 400, Color = Orange, NoiseStrength = 2.5, Emissive = 8 |

All procedural variation happens here.

---

## 4. NIAGARA UBER-EMITTER RULES (NON-NEGOTIABLE)

### Forbidden

- Constants
- Rapid Iteration parameters for gameplay
- Hidden module values
- Logic-driven branching

### Required

- User parameters only
- Struct bindings preferred
- Deterministic execution

---

## 5. UBER-EMITTER GRAPH DESIGN

### Spawn Script

```
SpawnRate = User.SpawnRate
Lifetime = Random(User.Lifetime.X, User.Lifetime.Y)
Velocity = User.InitialVelocity
SpriteSize = Random(User.Size.X, User.Size.Y)
Color = User.Color

if (Particles.ID > User.MaxParticles)
    Kill()
```

### Update Script

```
Velocity += CurlNoise * User.NoiseStrength
Velocity += Gravity * User.GravityScale
Color.A *= User.Opacity
```

No gameplay logic. No conditionals beyond safety clamps.

---

## 6. RENDERER STRATEGY (NO DYNAMIC ADDITION)

Renderers are pre-authored.

### Renderer Visibility

```
Visible = User.bSmoke
Visible = User.bSparks
Visible = User.bLight
```

### Material Bindings

- Color
- Emissive
- Opacity

Never add or remove renderers at runtime.

---

## 7. MULTI-EMITTER COMPOSITION

Niagara Systems are compositions, not generators.

```
NS_Impact
 ├─ E_Core
 ├─ E_Sparks
 ├─ E_Smoke
 └─ E_LightFlash
```

Each emitter has `User.Enabled`:

```
if (!User.Enabled)
    Kill()
```

---

## 8. RUNTIME BINDING (ONLY CODE PATH)

```cpp
void ApplyFX(UNiagaraComponent* Comp, const FFXDescriptor& FX)
{
    Comp->SetNiagaraVariableFloat("User.SpawnRate", FX.SpawnRate);
    Comp->SetNiagaraVariableVec2("User.Lifetime", FX.Lifetime);
    Comp->SetNiagaraVariableInt("User.MaxParticles", FX.MaxParticles);

    Comp->SetNiagaraVariableLinearColor("User.Color", FX.Color);
    Comp->SetNiagaraVariableVec2("User.Size", FX.Size);
    Comp->SetNiagaraVariableFloat("User.Opacity", FX.Opacity);

    Comp->SetNiagaraVariableVec3("User.InitialVelocity", FX.InitialVelocity);
    Comp->SetNiagaraVariableFloat("User.NoiseStrength", FX.NoiseStrength);
    Comp->SetNiagaraVariableFloat("User.GravityScale", FX.GravityScale);

    Comp->SetNiagaraVariableBool("User.bSparks", FX.bSparks);
    Comp->SetNiagaraVariableBool("User.bSmoke", FX.bSmoke);
    Comp->SetNiagaraVariableBool("User.bLight", FX.bLight);

    Comp->SetNiagaraVariableFloat("User.Emissive", FX.Emissive);
}
```

No graph access. No compilation.

---

## 9. LOD SYSTEM (PARAMETER-DRIVEN)

```cpp
FX.LODLevel =
    Distance < 1000 ? 0 :
    Distance < 3000 ? 1 :
    Distance < 6000 ? 2 : 3;
```

### LOD Scaling

| LOD | Spawn | MaxParticles | Noise | Renderers |
|-----|-------|--------------|-------|-----------|
| 0 | 1.0 | 1.0 | 1.0 | All |
| 1 | 0.6 | 0.6 | 0.5 | No lights |
| 2 | 0.3 | 0.3 | 0.0 | Sprites |
| 3 | 0.0 | 0.0 | 0.0 | Kill |

---

## 10. GPU vs CPU EMITTER RULEBOOK

### GPU Emitters

- >500 particles
- Noise / turbulence
- No per-particle events

### CPU Emitters

- Gameplay callbacks
- Collision logic
- Low counts

Hybrid systems are normal.

---

## 11. EDITOR TOOLING (SUPPORTED ONLY)

### Tool Can Do

- Edit FXDescriptor assets
- Preview Niagara systems
- Validate budgets
- Batch operations

### Tool Must NOT Do

- Touch Niagara graphs
- Add modules
- Compile scripts

### Validation Rules

```
SpawnRate <= 1000
MaxParticles <= 2048
Opacity <= 1.0
```

---

## 12. DEBUGGING & VISUALIZATION

Runtime overlay per FX:

- SpawnRate
- ParticleCount
- LOD
- GPU/CPU
- Cull state

Use:

```
stat Niagara
NiagaraDebug HUD
```

---

## 13. NETWORKING MODEL

### Replicate

- FXTag
- Intensity
- Element
- Seed

### Do NOT Replicate

- Particle state
- Niagara parameters
- Counts

Clients resolve locally.

---

## 14. FAILURE MODES (REAL WORLD)

| Mistake | Result |
|---------|--------|
| Constants in Niagara | Non-data-driven |
| Rapid Iteration abuse | Uneditable |
| No particle caps | Perf spikes |
| Graph-authored logic | Tool bypass |

---

## 15. LONG-TERM MAINTENANCE

Niagara can change. Your data does not.

- Swap Uber-Emitters freely
- Keep FXDescriptors stable
- Tooling remains valid

---

## FX PRODUCTION ADDENDUM

---

## 16. PLATFORM BUDGET TABLES

### Particle & Simulation Budgets

| Platform | Max Particles / FX | Max Active FX | GPU Emitters | CPU Emitters | Notes |
|----------|-------------------|---------------|--------------|--------------|-------|
| High-end PC | 10,000 | 32 | Preferred | Limited | Async compute OK |
| Console Gen9 | 6,000 | 24 | Preferred | Limited | Stable frame pacing |
| Console Gen8 | 3,500 | 16 | Mixed | Limited | Avoid heavy noise |
| Mobile High | 1,500 | 8 | GPU-lite | Minimal | Sprite-only |
| Mobile Low | 800 | 4 | No | Minimal | Kill at distance |

### LOD Scaling Multipliers

| LOD | Spawn | Lifetime | Noise | Renderers |
|-----|-------|----------|-------|-----------|
| 0 | 1.0 | 1.0 | 1.0 | All |
| 1 | 0.6 | 0.8 | 0.5 | No lights |
| 2 | 0.3 | 0.6 | 0.0 | Sprites only |
| 3 | 0.0 | 0.0 | 0.0 | Kill |

---

## 17. NAMING CONVENTIONS

### Assets

- Niagara System: `NS_<Event>_<Theme>` (e.g., `NS_Impact_Fire`)
- Emitter: `E_<Role>` (e.g., `E_Sparks`, `E_Smoke`)
- FX Descriptor: `FXD_<Event>_<Theme>_<Intensity>`

### User Parameters

- Scalars: `User.SpawnRate`, `User.Opacity`
- Vectors: `User.InitialVelocity`, `User.Size`
- Bools: `User.bSmoke`, `User.bSparks`

### Gameplay Tags

- `FX.Event.Impact`
- `FX.Element.Fire`
- `FX.Intensity.High`

---

## 18. TEAM WORKFLOW

### Authoring Flow

1. Artist builds Uber-Emitter once
2. Tech artist exposes User parameters only
3. Designer authors FXDescriptor assets
4. Engineer binds data at runtime

### Review Gates

- **Art Review:** Visual correctness, no constants
- **Tech Review:** Budget validation, LOD correctness
- **QA:** Stress tests, platform variance

### Validation Rules (Editor Tool)

- SpawnRate <= platform cap
- MaxParticles <= platform cap
- No constants in spawn/update
- Renderer visibility bound to User bools

### Debug & QA Checklist

- [ ] stat Niagara within budget
- [ ] Niagara Debug HUD enabled
- [ ] LOD transitions smooth
- [ ] No emitter compilation at runtime
- [ ] Network clients resolve FX locally

---

## 19. EDITOR TOOL UX SPECIFICATION

### Tool Goals

- Author and validate FXDescriptor assets
- Preview Niagara output without graph access
- Enforce platform budgets automatically
- Provide immediate, non-destructive feedback

### Main Panels

#### A. FX Descriptor Editor Panel

- FX Tag (GameplayTag picker)
- Intensity / Element / Context selectors
- Auto-resolved numeric fields (read-only toggle)
- Manual override fields (advanced users)

Visual cues:
- 🟢 Green: within budget
- 🟡 Yellow: near budget
- 🔴 Red: invalid / blocked

#### B. Live Preview Panel

- Embedded Niagara viewport
- Camera distance slider (drives LOD)
- Platform selector (PC / Console / Mobile)
- Time scale control

Preview rules:
- Never recompiles Niagara
- Rebinds User parameters only
- Hot-reload safe

#### C. Validation Panel

- Particle count estimate
- GPU / CPU emitter breakdown
- Renderer count
- Memory estimate

Blocking errors:
- SpawnRate exceeds platform cap
- MaxParticles exceeds platform cap
- Missing required User parameters

Warnings:
- High noise on mobile
- Too many active renderers

### Batch Operations

- Re-evaluate all FXDescriptors
- Platform downgrade pass
- LOD sanity pass
- CSV export for QA

---

## 20. FXDESCRIPTOR → NIAGARA BINDING SCHEMA

### Binding Principles

- One-way data flow (Descriptor → Niagara)
- No hidden defaults
- All Niagara variability via User namespace

### Canonical Mapping Table

| FXDescriptor Field | Niagara Variable | Type |
|-------------------|------------------|------|
| SpawnRate | User.SpawnRate | float |
| Lifetime | User.Lifetime | vec2 |
| MaxParticles | User.MaxParticles | int |
| Color | User.Color | linear color |
| Size | User.Size | vec2 |
| Opacity | User.Opacity | float |
| InitialVelocity | User.InitialVelocity | vec3 |
| NoiseStrength | User.NoiseStrength | float |
| GravityScale | User.GravityScale | float |
| bSparks | User.bSparks | bool |
| bSmoke | User.bSmoke | bool |
| bLight | User.bLight | bool |
| Emissive | User.Emissive | float |
| CullDistance | User.CullDistance | float |
| LODLevel | User.LODLevel | int |

### Binding Rules

- Missing Niagara variable → hard error
- Type mismatch → hard error
- Unused Niagara User param → warning

### Runtime Binding Contract

- All bindings occur before `Activate()`
- No mid-frame mutation of structural params
- LOD changes allowed at runtime

### Versioning Strategy

- FXDescriptor versioned independently
- Niagara systems treated as replaceable
- Deprecated fields supported via resolver

### Debug Validation Overlay

For each active FX instance:
- Descriptor ID
- Bound parameter count
- LOD level
- Estimated particle count

---

## 21. EDITOR TOOL IMPLEMENTATION OUTLINE

### Technology Choices

- Editor Utility Widget (EUW) for UI
- C++ Editor Module for validation + runtime hooks
- Optional Blutility for batch ops

### Core Systems

- FXDescriptor asset scanner
- Platform profile resolver
- Validation engine (pure C++)
- Niagara preview binder (User params only)

### Execution Rules

- Tool never touches Niagara graphs
- Tool never recompiles emitters
- Tool operates only on data assets

---

## 22. AUTOMATED FX BUDGET CI VALIDATION

### Purpose

Catch FX regressions before artists ever run the game.

### CI Flow

1. Headless editor launch
2. Load all FXDescriptor assets
3. Resolve worst-case parameters
4. Validate against platform budgets
5. Fail build on hard errors

### Output

- JSON + CSV reports
- Per-platform summaries
- Trend diffs (regressions highlighted)

---

## 23. FXDESCRIPTOR INHERITANCE & OVERRIDES

### Hierarchy Model

1. Base Descriptor (semantic defaults)
2. Variant Descriptor (element/style)
3. Instance Override (runtime tweaks)

### Merge Rules

- Scalars: override
- Ranges: clamp
- Bools: OR / AND (per rule)

### Use Cases

- Weapon skins
- Biome variants
- Difficulty scaling

---

## 24. PROCEDURAL VARIATION SYSTEM (SAFE)

### Deterministic Seeds

- Seed derived from GameplayTag + Instance ID
- Passed as `User.Seed`

### Allowed Variation

- Lifetime jitter
- Color hue shift
- Size variance

### Forbidden

- Structural changes
- Renderer toggling via randomness

---

## 25. SAMPLE PROJECT FOLDER LAYOUT

```
/FX
 ├─ Niagara
 │   ├─ Systems
 │   └─ Emitters
 ├─ Descriptors
 │   ├─ Base
 │   ├─ Variants
 │   └─ Instances
 ├─ Data
 │   └─ PlatformProfiles
 └─ Tools
     ├─ Editor
     └─ Validation
```

---

## 26. FINAL PRODUCTION RULES (NON-NEGOTIABLE)

1. No Niagara graph mutation outside Niagara Editor
2. No gameplay logic inside emitters
3. All runtime variability via User parameters
4. Descriptors are authoritative
5. Tools validate, never generate FX
6. CI enforces budgets

---

## 27. NIAGARA MODULE AUTHORING GUIDELINES (FUTURE-PROOF)

These rules define how to write custom Niagara modules that remain compatible with data-driven FX pipelines, tooling, and future engine upgrades.

### 27.1 Core Philosophy

Niagara modules are **pure math blocks**, not behaviors.

A module must:
- Consume inputs
- Produce outputs
- Contain no gameplay meaning

If a module answers "when", "why", or "should", it is wrong.

### 27.2 Allowed Inputs

✔ User Parameters
✔ Emitter Parameters
✔ Particle Attributes
✔ Deterministic functions (noise, math)

❌ No hardcoded constants (except clamps)
❌ No hidden defaults
❌ No editor-only assumptions

### 27.3 Parameter Exposure Rules

Every tunable value must be:
- Exposed as an input pin
- Bound to `User.*` by default

Example (GOOD):
```
Input: User.NoiseStrength
Input: User.GravityScale
```

Example (BAD):
```
NoiseStrength = 3.5
```

### 27.4 Naming Conventions

#### Modules

`NM_<Category>_<Action>`

- `NM_Motion_ApplyGravity`
- `NM_Motion_CurlNoise`

#### Inputs

- Scalars: Strength, Scale, Multiplier
- Vectors: Direction, Velocity

Avoid gameplay terms like Damage, Hit, Weapon.

### 27.5 Determinism Rules

Modules must be:
- Frame-rate independent
- Seeded only via explicit input

Allowed:
```
Noise(Position, User.Seed)
```

Forbidden:
- Implicit time-based randomness
- Engine frame counters

### 27.6 GPU / CPU Compatibility

Every custom module must declare:
- CPU compatible ✔ / ✖
- GPU compatible ✔ / ✖

Rules:
- No CPU-only nodes in GPU modules
- No data interface misuse
- Test both paths explicitly

### 27.7 No Structural Logic

Forbidden inside modules:
- Spawning particles
- Killing particles (except safety clamp)
- Enabling/disabling renderers

Structure belongs to systems, not modules.

### 27.8 Versioning Strategy

Modules must be:
- Versioned explicitly
- Backward compatible

Use Niagara versioning to:
- Add inputs without breaking graphs
- Deprecate behavior safely

Never change meaning of existing pins.

### 27.9 Performance Guidelines

- Avoid expensive math in Update if possible
- Prefer Spawn-time cost over Update-time cost
- Clamp all user-driven values

Rule of thumb: If it runs per-particle per-frame, it must be cheap.

### 27.10 Tooling Compatibility Contract

Modules must:
- Function with zero editor context
- Be previewable via User params only
- Survive headless validation

If a module requires a special tool to behave, it is invalid.

### 27.11 Validation Checklist (Module Review)

- [ ] No hardcoded constants
- [ ] All inputs exposed
- [ ] User.* bindings default
- [ ] Deterministic
- [ ] GPU/CPU declared
- [ ] No structural logic
- [ ] Versioned
- [ ] Performance tested

### 27.12 Golden Rule

**Modules do math. Systems do structure. Data does meaning.**

---

## 28. CUSTOM HLSL GUIDELINES INSIDE NIAGARA

Custom HLSL is the **last resort**, not the default. These rules ensure HLSL stays safe, portable, and tool-compatible.

### 28.1 When Custom HLSL Is Allowed

✔ Missing Niagara node functionality
✔ Performance-critical math (profiling proven)
✔ GPU-specific optimizations

❌ To hide logic
❌ To bypass User parameters
❌ To encode gameplay meaning

### 28.2 Mandatory Input Rules

All HLSL parameters must be:
- Explicit inputs
- Bound from `User.*`, `Emitter.*`, or particle attributes

Forbidden:
```hlsl
float Strength = 5.0;
```

Required:
```hlsl
float Strength = User_NoiseStrength;
```

### 28.3 Determinism & Randomness

Allowed:
- Seeded noise
- Hash-based randomness using `User.Seed`

Forbidden:
- Time-based randomness
- Frame counters
- Global engine state

---

## FINAL MENTAL MODEL

> **Niagara renders data. Data defines behavior. Tools enforce truth.**

If Niagara disappeared tomorrow, your FX system would still exist.

---

## FINAL CHECKLIST

- [x] No graph mutation
- [x] No dynamic emitters
- [x] All values via User params
- [x] Descriptor authoritative
- [x] Validation enforced
- [x] LOD parameterized
- [x] GPU/CPU separated
- [x] Network-safe
- [x] Scales to AAA

---

*This document is mandatory for production sign-off.*

*Version 1.0 - January 2026*
