# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

GasAbilityGenerator is an Unreal Engine 5.7 Editor plugin (v2.6.0) that generates UE5 assets from YAML manifest definitions. It works with any UE project using Gameplay Ability System (GAS).

**Recent Changes:**
- v2.6.0: Maximized automation using reflection for protected Narrative Pro members
- v2.5.0: Renamed from SpiderAbilityGenerator for generic UE project compatibility
- All classes use `FGasAbilityGenerator*` prefix, structs use `FManifest*` prefix
- **Auto-detects project name** - no need to set `project_root` in manifest

## Using with Any Project

### Automatic Project Detection
The plugin automatically detects your Unreal project name and creates assets under `/Game/{ProjectName}/`. No configuration required!

For example, if your project is named "NP22Beta":
- Assets are created at `/Game/NP22Beta/{folder}/{asset_name}`
- Enums created at `/Game/NP22Beta/Enums/E_MyEnum`
- Abilities created at `/Game/NP22Beta/Abilities/GA_MyAbility`

### Manifest Structure
```yaml
# Optional: Override auto-detected project root (usually not needed)
# project_root: /Game/CustomPath

# Optional: Path to tags ini file
tags_ini_path: Config/DefaultGameplayTags.ini

# Define assets - they'll be created under the auto-detected project folder
gameplay_abilities:
  - name: GA_MyAbility
    parent_class: GameplayAbility
    folder: Abilities  # Creates at /Game/{ProjectName}/Abilities/GA_MyAbility

enumerations:
  - name: E_MyEnum
    folder: Enums  # Creates at /Game/{ProjectName}/Enums/E_MyEnum
    values:
      - Value1
      - Value2
```

### Folder Structure
Assets are created at: `/Game/{ProjectName}/{folder}/{asset_name}`

- Project name is auto-detected from Unreal Engine (e.g., "NP22Beta", "MyGame")
- Optional: Set `project_root: /Game/CustomPath` to override auto-detection
- The `folder` field in each asset definition creates subfolders

## Build Commands

```bash
# Visual Studio build (replace with your project paths)
MSBuild "YourProject.sln" /t:Build /p:Configuration="Development Editor" /p:Platform=Win64

# Unreal Build Tool
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" YourProjectEditor Win64 Development -Project="Path\To\YourProject.uproject"
```

## Commandlet Usage

Generate assets from command line without launching the editor UI:
```bash
"C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "Path\To\YourProject.uproject" -run=GasAbilityGenerator -manifest="Path\To\manifest.yaml"
```

## Architecture

### Module Flow
```
manifest.yaml → Parser → FManifestData → Generators → UE5 Assets
```

### Key Classes (Source/GasAbilityGenerator/)

| Class | Location | Purpose |
|-------|----------|---------|
| `FGasAbilityGeneratorModule` | Private/GasAbilityGeneratorModule.cpp | Plugin lifecycle, menu registration |
| `FGasAbilityGeneratorParser` | Private/GasAbilityGeneratorParser.cpp | YAML parsing |
| `SGasAbilityGeneratorWindow` | Private/GasAbilityGeneratorWindow.cpp | Slate UI |
| `UGasAbilityGeneratorCommandlet` | Private/GasAbilityGeneratorCommandlet.cpp | Command-line generation |
| Generators | Private/GasAbilityGeneratorGenerators.cpp | 25+ asset generators |

### Supported Asset Types
- Enumerations (`E_*`), Gameplay Tags
- Input Actions (`IA_*`), Input Mapping Contexts (`IMC_*`)
- Gameplay Effects (`GE_*`), Gameplay Abilities (`GA_*`)
- Actor Blueprints (`BP_*`), Widget Blueprints (`WBP_*`), Dialogue Blueprints (`DBP_*`)
- Blackboards (`BB_*`), Behavior Trees (`BT_*`), Materials (`M_*`)
- Float Curves (`FC_*`), Animation Montages (`AM_*`), Animation Notifies (`NAS_*`)
- Equippable Items (`EI_*`), Item Collections (`IC_*`)
- NPC Definitions (`NPCDef_*`), Character Definitions (`CD_*`)
- Ability Configurations (`AC_*`), Activity Configurations (`ActConfig_*`)
- Activities (`BPA_*`), Narrative Events (`NE_*`), Tagged Dialogue Sets

## Dependencies

The Build.cs requires these modules:
- **Blueprint Generation:** BlueprintGraph, Kismet, KismetCompiler
- **GAS:** GameplayAbilities, GameplayTags, GameplayTasks
- **Editor:** UnrealEd, AssetTools, AssetRegistry

Optional (if using Narrative Pro):
- NarrativeArsenal, NarrativePro

## Event Graph Generation

Define Blueprint event graphs in YAML:

```yaml
event_graphs:
  - name: EventGraph_Example
    nodes:
      - id: BeginPlay
        type: Event
        properties:
          event_name: ReceiveBeginPlay
      - id: PrintHello
        type: PrintString
        properties:
          message: "Hello World!"
    connections:
      - from: [BeginPlay, Then]
        to: [PrintHello, Exec]

actor_blueprints:
  - name: BP_Example
    parent_class: Actor
    folder: Blueprints
    event_graph: EventGraph_Example
```

### Supported Node Types
| Type | Properties | Description |
|------|------------|-------------|
| Event | event_name | BeginPlay, Tick, EndPlay, etc. |
| CustomEvent | event_name | User-defined events |
| CallFunction | function, class, target_self | Function calls |
| Branch | - | If/Then/Else |
| VariableGet/Set | variable_name | Variable access |
| PropertyGet/Set | property_name, target_class | Property access |
| Sequence | num_outputs | Multiple outputs |
| Delay | duration | Timer delay |
| PrintString | message, duration | Debug output |
| DynamicCast | target_class | Type casting |
| ForEachLoop | - | Array iteration |
| SpawnActor | actor_class_variable | Spawn actors |

## Search Paths

When looking for classes/enums, the plugin searches:
1. Native UE classes
2. `/Game/{ProjectName}/Blueprints/`, `/Game/{ProjectName}/Characters/`, `/Game/{ProjectName}/Actors/`
3. `/Game/{ProjectName}/Enums/`, `/Game/{ProjectName}/Enumerations/`, `/Game/{ProjectName}/Data/`
4. `/Game/` root

The `{ProjectName}` is auto-detected from your Unreal project.

## Important Rules

### Always Push After Commit
When committing changes to git, always push to remote immediately after. Combine commit and push in the workflow - don't wait for user to request push separately.

### Build Without Approval
MSBuild commands for .sln files do not require user approval. Run builds automatically when needed without asking for permission.

### Read Without Approval
Reading files does not require user approval. Read any files needed for the task without asking for permission.

### No Temp File Workarounds
Never copy files to temp locations (like `C:/Temp/`) to work around path issues. Always use proper path escaping and quoting instead:
```bash
# Correct - proper quoting for paths with spaces
-manifest="C:/Unreal Projects/NP22Beta/Plugins/GasAbilityGenerator/ClaudeContext/manifest.yaml"

# Wrong - copying to temp as workaround
cp manifest.yaml C:/Temp/manifest.yaml  # DO NOT DO THIS
```

### Non-Asset Entries Must Be Nested
Variables, properties, and parameters are NOT standalone assets. They must always be nested inside their parent asset definition:

```yaml
# CORRECT - variables nested inside asset
gameplay_abilities:
  - name: GA_FatherAttack
    variables:
      - name: Damage
        type: Float
      - name: TargetEnemy
        type: Object
    event_graph:
      nodes:
        - id: GetDamage
          properties:
            variable_name: Damage

# WRONG - standalone non-asset entries
variables:
  - name: Damage
  - name: TargetEnemy
```

Non-asset entry types that must be nested:
- `variables` - Blueprint variables (nested in abilities, blueprints)
- `properties` - Node properties (nested in event_graph nodes)
- `parameters` - Material parameters (nested in materials)
- `keys` - Blackboard keys (nested in blackboards)
- `nodes` - Event graph nodes (nested in event_graphs)
- `connections` - Node connections (nested in event_graphs)
