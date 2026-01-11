# NP22Beta Project - Father Companion System

## Overview
This is an Unreal Engine 5.6 project using Narrative Pro Plugin v2.2. The Father Companion is a transformable spider companion with 5 forms and 19 abilities implemented using the Gameplay Ability System (GAS).

## GasAbilityGenerator Plugin (v2.5.0)
Location: `Plugins/GasAbilityGenerator/`

The plugin generates UE5 assets from YAML manifest definitions:
- Enumerations (E_*)
- Gameplay Tags
- Input Actions (IA_*)
- Input Mapping Contexts (IMC_*)
- Gameplay Effects (GE_*)
- Gameplay Abilities (GA_*)
- Actor Blueprints (BP_*)
- Widget Blueprints (WBP_*)
- Blackboards, Behavior Trees, Materials

**v2.5.0 Naming:** All code uses `FManifest*` prefixes (e.g., `FManifestData`, `FManifestGameplayAbilityDefinition`).

## Key Files in ClaudeContext/
Read these files to understand the project:

| File | Purpose |
|------|---------|
| `manifest.yaml` | Single source of truth for all assets |
| `Father_Ability_Generator_Plugin_v7_8_2_Specification.md` | Plugin architecture and workflows |
| `Father_Companion_Technical_Reference_v6_0.md` | GAS patterns, tags, Narrative Pro integration |
| `Father_Companion_Guide_Format_Reference_v3_0.md` | Formatting standards for implementation guides |
| `Father_Companion_System_Design_Document_v2_0.md` | System overview, 5 forms, abilities |
| `DefaultGameplayTags_FatherCompanion_v4_0.ini` | Actual gameplay tag values |
| `NarrativePro_Headers/*.h` | Narrative Pro C++ API reference |

## Build Commands
```bash
# Visual Studio build
MSBuild "NP22Beta.sln" /t:Build /p:Configuration="Development Editor" /p:Platform=Win64

# Unreal Build Tool
"C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat" NP22BetaEditor Win64 Development -Project="C:\Unreal Projects\NP22Beta\NP22Beta.uproject"
```

## Critical Rules

### DO:
- Use Blueprint-only solutions for implementation guides
- Use Narrative Pro native systems (NarrativeGameplayAbility, NarrativeNPCCharacter)
- Follow manifest.yaml as single source of truth
- Generate enumerations BEFORE blueprints that reference them
- Use SetByCaller tags for dynamic damage values
- Check asset existence before generation (skip if exists)

### DO NOT:
- Use GAS Companion plugin (removed from project)
- Use NarrativeCharacter class (use NarrativeNPCCharacter)
- Use UGameplayAbility class (use NarrativeGameplayAbility)
- Write C++ in implementation guides
- Use attribute-based movement speed (use CharacterMovement directly)
- Spawn spider from player BeginPlay (use NPCSpawner system)

## Father Companion Forms
1. **Crawler** - Default, follows player, melee/laser attacks
2. **Armor** - Attaches to chest, protective dome, damage absorption
3. **Exoskeleton** - Attaches to back, speed boost, dash, stealth
4. **Symbiote** - Merges with player, stat boosts, proximity damage
5. **Engineer** - Turret mode, electric traps, auto-targeting

## Plugin Version History
- v2.5.0 - Renamed to GasAbilityGenerator, all FFather* types renamed to FManifest*
- v2.2.0 - Event graph generation (create Blueprint nodes and connections from YAML)
- v2.1.9 - Manifest validation (whitelist enforcement, blocks unlisted assets)
- v2.1.8 - Enumeration generation, enum variable type fix
- v2.1.7 - Variable bleed fix (indent-based subsection exit)
- v2.1.6 - Variable creation for Actor/Widget blueprints
- v2.1.5 - Asset count fix
- v2.0.9 - Skip existing assets, generation guard

## Project Paths
- Project: `C:\Unreal Projects\NP22Beta\`
- Plugin: `C:\Unreal Projects\NP22Beta\Plugins\GasAbilityGenerator\`
- Content: `C:\Unreal Projects\NP22Beta\Content\FatherCompanion\`
- Config: `C:\Unreal Projects\NP22Beta\Config\`

## User Paths
- Desktop: `C:\Users\Erdem\OneDrive\Desktop\` (OneDrive synced)
