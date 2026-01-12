# Niagara Uber-Emitter Setup Guide

> **One-time setup for data-driven FX pipeline**
>
> This guide walks through creating parameterized Uber-Emitters that the GasAbilityGenerator can control via manifest data.

---

## Overview

| Phase | Who | Time | Result |
|-------|-----|------|--------|
| Phase 1 | Artist (You) | 2-3 hours | `NS_FatherFXTemplate` with Uber-Emitters |
| Phase 2 | Generator | Automated | Per-effect systems with custom values |

After completing this guide, the generator can create unlimited VFX variants by duplicating your template and setting User parameters.

---

## Prerequisites

- Unreal Engine 5.7
- Project: NP22B57
- Content folder access

---

## Step 1: Create Project VFX Folders

In Content Browser, create:

```
Content/
└── FatherCompanion/
    └── VFX/
        ├── Emitters/          ← Uber-Emitters go here
        ├── Systems/           ← Template system goes here
        └── Materials/         ← VFX materials (optional)
```

---

## Step 2: Duplicate Engine Emitters

For each emitter type, duplicate from engine templates:

### 2.1 Particle Emitter (Sprites)

1. Open Content Browser
2. Navigate to: `Engine/Plugins/FX/Niagara/Content/DefaultAssets/Templates/Emitters/`
3. Find: `Fountain`
4. Right-click → **Duplicate**
5. Save to: `/Game/FatherCompanion/VFX/Emitters/`
6. Rename to: `E_FatherParticle`

### 2.2 Burst Emitter

1. Find: `SimpleSpriteBurst`
2. Duplicate → Save as: `E_FatherBurst`

### 2.3 Beam Emitter

1. Find: `DynamicBeam`
2. Duplicate → Save as: `E_FatherBeam`

### 2.4 Ribbon/Trail Emitter

1. Find: `LocationBasedRibbon`
2. Duplicate → Save as: `E_FatherRibbon`

### 2.5 Light Emitter (Optional)

1. Find: `OmnidirectionalBurst` or create new
2. Duplicate → Save as: `E_FatherLight`

**Result:** 5 emitter assets in `/Game/FatherCompanion/VFX/Emitters/`

---

## Step 3: Add User Parameters to Each Emitter

Open each emitter in Niagara Editor and add these User parameters.

### 3.1 Open Emitter

1. Double-click `E_FatherParticle`
2. Niagara Editor opens

### 3.2 Add User Parameters

1. In **Parameters** panel (left side)
2. Click **+** next to "User Exposed"
3. Add each parameter:

| Parameter Name | Type | Default Value | Purpose |
|----------------|------|---------------|---------|
| `User.Enabled` | bool | true | Kill switch |
| `User.SpawnRate` | float | 100.0 | Particles per second |
| `User.LifetimeMin` | float | 1.0 | Min particle life |
| `User.LifetimeMax` | float | 2.0 | Max particle life |
| `User.MaxParticles` | int32 | 500 | Particle cap |
| `User.Color` | LinearColor | (1,1,1,1) | Particle color |
| `User.SizeMin` | float | 5.0 | Min particle size |
| `User.SizeMax` | float | 10.0 | Max particle size |
| `User.Opacity` | float | 1.0 | Alpha multiplier |
| `User.InitialVelocity` | Vector | (0,0,0) | Starting velocity |
| `User.NoiseStrength` | float | 0.0 | Curl noise intensity |
| `User.GravityScale` | float | 0.0 | Gravity multiplier |
| `User.Emissive` | float | 1.0 | Glow intensity |

### 3.3 Parameter Naming Convention

- Always prefix with `User.`
- Use PascalCase after prefix
- Examples: `User.SpawnRate`, `User.MaxParticles`, `User.Color`

---

## Step 4: Wire User Parameters to Modules

This is the critical step - connecting User params to existing module inputs.

### 4.1 Wire SpawnRate

1. In **Emitter Update** section
2. Find **Spawn Rate** module
3. Click the **SpawnRate** input
4. Change from constant to: **Link Inputs → User → User.SpawnRate**

### 4.2 Wire Initialize Particle

1. In **Particle Spawn** section
2. Find **Initialize Particle** module
3. Wire each property:

| Module Property | Link To |
|-----------------|---------|
| Lifetime | `Random Range(User.LifetimeMin, User.LifetimeMax)` |
| Color | `User.Color` |
| Sprite Size | `Random Range(User.SizeMin, User.SizeMax)` |
| Velocity | `User.InitialVelocity` |

### 4.3 Wire Update Modules

1. In **Particle Update** section
2. Find **Curl Noise Force** (or add if missing)
3. Wire Strength to: `User.NoiseStrength`
4. Find **Gravity Force** (or add if missing)
5. Wire Scale to: `User.GravityScale`

### 4.4 Wire Color/Opacity (if separate module)

1. Find **Scale Color** or **Color** module in Update
2. Wire Alpha multiplier to: `User.Opacity`

---

## Step 5: Add Kill Switch Logic

This allows the generator to disable emitters via `User.Enabled = false`.

### 5.1 Add Kill Logic to Particle Spawn

1. In **Particle Spawn** section
2. Add new module: **Particle State → Kill Particles**
3. Set condition:

```
Kill Particle if:
  NOT User.Enabled
  OR
  Particles.ID > User.MaxParticles
```

### 5.2 Alternative: Scratch Pad Module

1. Add **Scratch Pad** module to Particle Spawn
2. Add this logic:

```hlsl
// Kill switch
if (!User.Enabled)
{
    Engine.Alive = false;
}

// Particle cap
if (Particles.ID > User.MaxParticles)
{
    Engine.Alive = false;
}
```

---

## Step 6: Configure Renderer

### 6.1 Sprite Renderer Setup

1. In **Render** section
2. Select **Sprite Renderer**
3. Ensure Material is set (use default or custom)
4. Wire Emissive to material parameter if available

### 6.2 Renderer Visibility (Optional)

If you want per-renderer control:

1. Add `User.RendererEnabled` bool parameter
2. In Renderer settings, wire Visibility to this param

---

## Step 7: Repeat for All Emitters

Apply Steps 3-6 to each emitter:

| Emitter | Special Considerations |
|---------|----------------------|
| `E_FatherParticle` | Standard sprite particles |
| `E_FatherBurst` | Higher SpawnRate default, shorter Lifetime |
| `E_FatherBeam` | Add `User.BeamLength`, `User.BeamWidth` |
| `E_FatherRibbon` | Add `User.RibbonWidth`, uses trail logic |
| `E_FatherLight` | Add `User.LightIntensity`, `User.LightRadius` |

---

## Step 8: Create Template System

### 8.1 Create New System

1. Right-click in `/Game/FatherCompanion/VFX/Systems/`
2. **Niagara System → New system from selected emitter(s)**
3. Select all 5 emitters
4. Name: `NS_FatherFXTemplate`

### 8.2 Organize Emitters

In the System, rename emitters for clarity:

```
NS_FatherFXTemplate
 ├─ Particles (E_FatherParticle)
 ├─ Burst (E_FatherBurst)
 ├─ Beam (E_FatherBeam)
 ├─ Ribbon (E_FatherRibbon)
 └─ Light (E_FatherLight)
```

### 8.3 Set Default States

For each emitter in the system:
- Set `User.Enabled = false` (generator will enable as needed)
- Verify all User params are exposed at system level

### 8.4 Verify System User Parameters

1. Select the System (root node)
2. Check **User Exposed** parameters
3. All emitter User params should be visible with emitter prefix:
   - `Particles.User.Enabled`
   - `Particles.User.SpawnRate`
   - `Burst.User.Enabled`
   - etc.

---

## Step 9: Test the Template

### 9.1 Quick Test

1. Drag `NS_FatherFXTemplate` into a level
2. In Details panel, find User Parameters
3. Toggle `Particles.User.Enabled = true`
4. Adjust `Particles.User.SpawnRate`
5. Verify particles respond to changes

### 9.2 Validate All Emitters

Test each emitter:
- Enable it
- Adjust SpawnRate, Color, Size
- Verify kill switch works (Enabled = false stops particles)

---

## Step 10: Save and Lock

1. Save all emitter assets
2. Save `NS_FatherFXTemplate`
3. Consider marking as "Locked" in source control
4. Document any custom parameters added

---

## Final Checklist

- [ ] 5 Uber-Emitters created in `/Game/FatherCompanion/VFX/Emitters/`
- [ ] All emitters have User.* parameters
- [ ] All modules wired to User params (no hardcoded constants)
- [ ] Kill switch logic added to each emitter
- [ ] `NS_FatherFXTemplate` created with all emitters
- [ ] All emitters disabled by default (User.Enabled = false)
- [ ] System-level User params visible and working
- [ ] Quick test passed - parameters affect visuals

---

## User Parameter Reference

### Standard Parameters (All Emitters)

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `User.Enabled` | bool | false | Master on/off |
| `User.SpawnRate` | float | 100 | Particles/sec |
| `User.LifetimeMin` | float | 1.0 | Min life (seconds) |
| `User.LifetimeMax` | float | 2.0 | Max life (seconds) |
| `User.MaxParticles` | int32 | 500 | Hard cap |
| `User.Color` | LinearColor | White | RGBA color |
| `User.SizeMin` | float | 5.0 | Min size |
| `User.SizeMax` | float | 10.0 | Max size |
| `User.Opacity` | float | 1.0 | Alpha multiplier |
| `User.InitialVelocity` | Vector | (0,0,0) | Starting velocity |
| `User.NoiseStrength` | float | 0.0 | Turbulence |
| `User.GravityScale` | float | 0.0 | Gravity effect |
| `User.Emissive` | float | 1.0 | Glow intensity |

### Beam-Specific Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `User.BeamLength` | float | 100.0 | Beam length |
| `User.BeamWidth` | float | 10.0 | Beam thickness |

### Ribbon-Specific Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `User.RibbonWidth` | float | 20.0 | Trail width |

### Light-Specific Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `User.LightIntensity` | float | 5000 | Light brightness |
| `User.LightRadius` | float | 200 | Light range |

---

## What Happens Next

Once this template exists, the GasAbilityGenerator can:

1. Read `fx_descriptor` from manifest.yaml
2. Duplicate `NS_FatherFXTemplate`
3. Set User parameter values
4. Save as new system (e.g., `NS_FatherFormTransition`)

You never touch Niagara again for new effects - just edit YAML.

---

## Troubleshooting

### User params not showing at system level

- Ensure param is marked "User Exposed" in emitter
- Re-add emitter to system if needed

### Particles don't respond to changes

- Check module is wired to User param (not constant)
- Verify parameter name matches exactly

### Kill switch not working

- Ensure kill logic runs in Particle Spawn, not Update
- Check condition syntax

### Performance issues

- Lower MaxParticles
- Reduce SpawnRate
- Use GPU emitters for high particle counts

---

*Version 1.0 - January 2026*
*For use with GasAbilityGenerator v2.9.0+*
