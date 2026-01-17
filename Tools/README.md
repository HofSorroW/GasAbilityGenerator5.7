# GasAbilityGenerator Automation Tools

## Quick Commands for Claude Code

### Build Plugin
```bash
powershell -ExecutionPolicy Bypass -File "C:\Unreal Projects\NP22B57\Plugins\GasAbilityGenerator\Tools\claude_automation.ps1" -Action build
```

### Run Editor
```bash
powershell -ExecutionPolicy Bypass -File "C:\Unreal Projects\NP22B57\Plugins\GasAbilityGenerator\Tools\claude_automation.ps1" -Action run
```

### Get Generation Logs (after closing editor)
```bash
powershell -ExecutionPolicy Bypass -File "C:\Unreal Projects\NP22B57\Plugins\GasAbilityGenerator\Tools\claude_automation.ps1" -Action logs
```

### Full Cycle (build + run)
```bash
powershell -ExecutionPolicy Bypass -File "C:\Unreal Projects\NP22B57\Plugins\GasAbilityGenerator\Tools\claude_automation.ps1" -Action full
```

## Workflow

1. **Claude fixes code** → Edit source files
2. **Build** → Run `build` action
3. **If errors** → Claude reads errors, fixes, goto step 2
4. **If success** → Run `run` action to launch editor
5. **In Editor** → Generate tags, then generate assets
6. **Close Editor**
7. **Read logs** → Run `logs` action
8. **If errors** → Claude reads logs, fixes, goto step 1

## Log Files

- `Tools/Logs/ubt_latest.log` - Latest UBT build output
- `Tools/Logs/generation_latest.log` - Latest asset generation logs
- `%LOCALAPPDATA%\UnrealEngine\5.7\Saved\Logs\NP22B57.log` - Full UE log

## Error Format

Build errors are formatted for easy parsing:
```
FILE: path/to/file.cpp
LINE: 123
CODE: C2065
MSG:  'identifier': undeclared identifier
---
```

Generation logs are categorized:
- ERRORS - Connection failures, missing nodes
- WARNINGS - Unconnected pins, potential issues
- INFO - Successfully created nodes/connections
