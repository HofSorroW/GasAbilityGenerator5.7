# Quest Pipeline Handoff Document v1.0

**Date:** 2026-01-14
**Purpose:** Batch dialogue/quest creation from spreadsheet data
**Status:** Design Complete, Implementation Pending

---

## Executive Summary

Build a Quest Pipeline Editor that allows batch production of dialogue and quest assets from a single Excel/CSV file. The system will parse spreadsheet data, split it into individual DBP_ (Dialogue Blueprint) assets, and later Quest_ assets, while respecting the v3.0 Regen/Diff Safety System.

---

## Research Findings

### Narrative Pro Architecture (from DBP_Seth / QBP_NarrativeDemoQuest_OpenWorld analysis)

#### Dialogue Structure
- **DialogueNode_NPC**: Blue nodes with speaker, text, audio, animations
- **DialogueNode_Player**: Gold nodes with text, option text for wheel
- **Connections**: npc_replies[] and player_replies[] arrays linking nodes
- **Speakers**: NPCDefinition references with owned tags, camera settings

#### Quest Structure
- **States**: Regular (gray), Success (green), Failure (red)
- **Branches**: Blue nodes with TASKS attached
- **Tasks**: BPT_* instanced objects (FindItem, FinishDialogue, KillEnemy, etc.)
- **Events**: NE_* instanced objects fired at state transitions
- **Variables**: NPCDefinitions, Instanced Goals, Faction Tags, Quest Requirements

#### Building Blocks Catalogue

**Events (NE_*):**
| Event | Purpose |
|-------|---------|
| NE_AddFactions | Add faction tags |
| NE_RemoveFactions | Remove faction tags |
| NE_AddGameplayTags | Add gameplay tags |
| NE_RemoveGameplayTags | Remove gameplay tags |
| NE_AddSaveCheckpoint | Create checkpoint |
| NE_BeginDialogue | Start dialogue |
| NE_BeginQuest | Start quest |
| NE_RestartQuest | Restart quest |
| NE_GiveXP | Grant experience |
| NE_SetFactionAttitude | Set faction relationship |
| NE_ShowNotification | Display notification |

**Tasks (BPT_*):**
| Task | Purpose |
|------|---------|
| BPT_GoToLocation | Navigate to location |
| BPT_FollowNPCToLocation | Follow NPC |
| BPT_FindItem | Find/collect item |
| BPT_TakePickup | Pick up item |
| BPT_KillEnemy | Kill enemies |
| BPT_FinishDialogue | Complete dialogue |
| BPT_PlayDialogueNode | Play specific node |
| BPT_InteractWithObject | Interact with actor |
| BPT_EquipItem | Equip item |
| BPT_AddGoalAndWait | Add goal, wait complete |
| BPT_ClearSpawn | Clear spawned enemies |
| BPT_WaitGameplayEvent | Wait for event |
| BPT_PlayCutsceneAndWait | Play sequence |

**Conditions (NC_*):**
| Condition | Purpose |
|-----------|---------|
| NC_IsQuestSucceeded | Quest completed |
| NC_IsQuestFailed | Quest failed |
| NC_IsQuestInProgress | Quest active |
| NC_IsQuestAtState | Quest at state |
| NC_HasDialogueNodePlayed | Node played X times |
| NC_HasCompletedDataTask | Task completed |
| NC_IsDayTime | Daytime check |
| NC_IsTimeInRange | Time range check |

---

## CSV Format Specification

### File Structure

Single CSV/Excel file containing all dialogues. Each row = one dialogue node.

### Columns

| Column | Required | Type | Description |
|--------|----------|------|-------------|
| Dialogue | Yes | String | Asset name (DBP_*). Groups nodes into dialogues |
| NodeID | Yes | String | Unique ID within dialogue (no spaces, use underscores) |
| Type | Yes | Enum | `NPC` or `PLAYER` |
| Speaker | NPC only | String | NPCDefinition reference (NPCDef_*) |
| Text | Yes | String | Full spoken/displayed text |
| OptionText | Player only | String | Short text for dialogue wheel |
| Replies | Yes | String | Comma-separated NodeIDs, or `END` for terminal |
| Conditions | No | String | NC_* references (Phase 2) |
| Events | No | String | NE_* references (Phase 2) |

### Example Data

```csv
Dialogue,NodeID,Type,Speaker,Text,OptionText,Replies,Conditions,Events
DBP_Blacksmith,greeting,NPC,NPCDef_Blacksmith,"Welcome to my forge! What can I do for you?",,ask_work;ask_prices;goodbye,,
DBP_Blacksmith,ask_work,PLAYER,,"Do you have any work for me?",Ask about work,work_response,,
DBP_Blacksmith,work_response,NPC,NPCDef_Blacksmith,"Yes! I need iron ore from the mines.",,accept;decline,,
DBP_Blacksmith,accept,PLAYER,,"I'll gather the ore for you.",Accept quest,quest_accepted,,
DBP_Blacksmith,decline,PLAYER,,"Maybe another time.",Decline,decline_response,,
DBP_Blacksmith,quest_accepted,NPC,NPCDef_Blacksmith,"Excellent! Bring me 10 ore and I'll pay you well.",,END,,
DBP_Blacksmith,decline_response,NPC,NPCDef_Blacksmith,"Come back when you're ready.",,greeting,,
DBP_Blacksmith,ask_prices,PLAYER,,"What are your prices?",Prices,prices_response,,
DBP_Blacksmith,prices_response,NPC,NPCDef_Blacksmith,"Fair prices for quality work!",,greeting,,
DBP_Blacksmith,goodbye,PLAYER,,"Nothing, goodbye.",Leave,goodbye_response,,
DBP_Blacksmith,goodbye_response,NPC,NPCDef_Blacksmith,"Safe travels!",,END,,
```

### Special Values

| Value | Meaning |
|-------|---------|
| `END` | Terminal node - dialogue ends |
| `BACK` | Return to previous hub (resolved by parser) |
| `;` | Separator for multiple replies |
| Empty row | Visual separator between dialogues |

### Rules

1. First node of each dialogue = root node
2. NodeIDs must be unique within a dialogue
3. Speaker required for NPC nodes, empty for PLAYER
4. OptionText recommended for PLAYER nodes (falls back to Text if empty)
5. Replies can loop back to earlier nodes

---

## Pipeline Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     DIALOGUE CSV FILE                            │
│  DialogueData.csv (Excel-editable, all dialogues in one file)   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    CSV PARSER / SPLITTER                         │
│  1. Read CSV file                                                │
│  2. Group rows by Dialogue column                                │
│  3. Build node trees with connections                            │
│  4. Validate structure (orphan nodes, circular refs)             │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    v3.0 HASH COMPARISON                          │
│  Input Hash: Hash of CSV rows for each dialogue                  │
│  Output Hash: Hash of existing DBP_ asset                        │
│  Result: CREATE / MODIFY / SKIP / CONFLICT                       │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    DBP GENERATOR (existing v3.8+)                │
│  - Creates UDialogueBlueprint                                    │
│  - Populates DialogueTemplate                                    │
│  - Creates DialogueNode_NPC / DialogueNode_Player                │
│  - Sets up connections (npc_replies, player_replies)             │
│  - Assigns speakers                                              │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    OUTPUT ASSETS                                 │
│  /Game/Project/Dialogues/DBP_Blacksmith.uasset                  │
│  /Game/Project/Dialogues/DBP_Guard.uasset                       │
│  /Game/Project/Dialogues/DBP_Merchant.uasset                    │
└─────────────────────────────────────────────────────────────────┘
```

---

## Implementation Plan

### Phase 1: Basic Dialogue Flow (Current)
- [x] Design CSV format
- [x] Create CSV parser (FDialogueCSVParser) - IMPLEMENTED
- [x] Convert to FManifestDialogueBlueprintDefinition - IMPLEMENTED
- [ ] Test with DBP_Seth equivalent data
- [ ] Verify NPC/Player node creation
- [ ] Verify connections work correctly
- [ ] Verify speaker names display

### Phase 2: Conditions & Events
- [ ] Add Conditions column parsing
- [ ] Add Events column parsing
- [ ] Map to NC_*/NE_* references
- [ ] Test conditional dialogue branches

### Phase 3: Quest Integration
- [ ] Design Quest CSV format
- [ ] Link dialogues to quests (questgiver)
- [ ] Generate Quest_ assets with state machines
- [ ] Connect quest tasks to dialogues

### Phase 4: Editor UI
- [ ] Build Slate table editor (like NPC Table Editor)
- [ ] Import/Export CSV functionality
- [ ] In-editor validation
- [ ] Preview dialogue tree

---

## Code Integration Points

### New Files Created

```
Source/GasAbilityGenerator/
├── Private/
│   └── GasAbilityGeneratorDialogueCSVParser.cpp  ✅ CREATED
├── Public/
│   └── GasAbilityGeneratorDialogueCSVParser.h    ✅ CREATED
ClaudeContext/
├── DialogueData.csv                               ✅ CREATED (sample data)
└── Quest_Pipeline_Handoff_v1_0.md                 ✅ CREATED (this file)
```

### Existing Files to Modify

```
GasAbilityGeneratorTypes.h
  - FManifestDialogueBlueprintDefinition already has dialogue_tree support

GasAbilityGeneratorGenerators.cpp
  - FDialogueBlueprintGenerator already creates DBP_ with dialogue trees (v3.8)

GasAbilityGeneratorParser.cpp
  - Add ParseDialogueCSV() method
  - Add CSV to manifest conversion
```

### Key Structs (Already Exist)

```cpp
// From GasAbilityGeneratorTypes.h
struct FManifestDialogueNodeDefinition
{
    FString ID;
    FString Type;           // "npc" or "player"
    FString Speaker;        // NPCDefinition reference
    FString Text;
    FString OptionText;
    TArray<FString> NPCReplies;
    TArray<FString> PlayerReplies;
    TArray<FManifestDialogueEventDefinition> Events;
    TArray<FManifestDialogueConditionDefinition> Conditions;
};

struct FManifestDialogueTreeDefinition
{
    FString Root;
    TArray<FManifestDialogueNodeDefinition> Nodes;
};
```

---

## v3.0 Hash System Integration

### Input Hash Calculation

```cpp
FString ComputeDialogueCSVHash(const TArray<FDialogueCSVRow>& Rows)
{
    FString Combined;
    for (const auto& Row : Rows)
    {
        Combined += Row.NodeID + Row.Type + Row.Speaker + Row.Text +
                    Row.OptionText + Row.Replies;
    }
    return FMD5::HashAnsiString(*Combined);
}
```

### Storage

- Hash stored in UGeneratorMetadataRegistry (DBP_ uses Blueprint path)
- Key: `/Game/Project/Dialogues/DBP_Blacksmith`
- Value: `{ InputHash, OutputHash, Timestamp }`

---

## Testing Strategy

### Test Case 1: Simple Linear Dialogue
```
NPC: "Hello" -> PLAYER: "Hi" -> NPC: "Goodbye" -> END
```

### Test Case 2: Branching Dialogue
```
NPC: "Choose" -> PLAYER: "Option A" -> NPC: "Response A" -> END
             -> PLAYER: "Option B" -> NPC: "Response B" -> END
```

### Test Case 3: Looping Dialogue
```
NPC: "Main menu" -> PLAYER: "Ask question" -> NPC: "Answer" -> "Main menu"
                -> PLAYER: "Leave" -> END
```

### Test Case 4: DBP_Seth Recreation
- Recreate DBP_Seth structure from CSV
- Compare output to original
- Verify all connections match

---

## Open Questions

1. **Root node detection**: First row per dialogue = root? Or explicit marker?
2. **Audio/Animation columns**: Add later or placeholder now?
3. **Multi-speaker dialogues**: How to handle conversations with 3+ NPCs?
4. **Localization**: String table IDs instead of raw text?

---

## Integration Code

### Adding CSV Support to Commandlet

Add to `GasAbilityGeneratorCommandlet.cpp` after manifest parsing (around line 184):

```cpp
#include "GasAbilityGeneratorDialogueCSVParser.h"

// ... after ParseManifest() succeeds ...

// v4.0: Parse dialogue CSV if provided
FString DialogueCSVPath;
if (FParse::Value(*Params, TEXT("-dialoguecsv="), DialogueCSVPath))
{
    DialogueCSVPath = DialogueCSVPath.TrimQuotes();
    if (!FPaths::IsRelative(DialogueCSVPath))
    {
        // Absolute path
    }
    else
    {
        // Relative to manifest directory
        DialogueCSVPath = FPaths::GetPath(ManifestPath) / DialogueCSVPath;
    }

    if (FPaths::FileExists(DialogueCSVPath))
    {
        LogMessage(FString::Printf(TEXT("Parsing dialogue CSV: %s"), *DialogueCSVPath));

        TArray<FManifestDialogueBlueprintDefinition> CSVDialogues;
        if (FDialogueCSVParser::ParseCSVFile(DialogueCSVPath, CSVDialogues))
        {
            LogMessage(FString::Printf(TEXT("Loaded %d dialogues from CSV"), CSVDialogues.Num()));

            // Append to manifest data (CSV dialogues take precedence)
            for (const auto& Dialogue : CSVDialogues)
            {
                // Remove existing definition with same name
                ManifestData.DialogueBlueprints.RemoveAll([&](const FManifestDialogueBlueprintDefinition& Existing) {
                    return Existing.Name == Dialogue.Name;
                });
                ManifestData.DialogueBlueprints.Add(Dialogue);
            }
        }
        else
        {
            LogError(FString::Printf(TEXT("WARNING: Failed to parse dialogue CSV: %s"), *DialogueCSVPath));
        }
    }
    else
    {
        LogError(FString::Printf(TEXT("WARNING: Dialogue CSV not found: %s"), *DialogueCSVPath));
    }
}
```

### Command Line Usage

```bash
# Generate from manifest + CSV
UnrealEditor-Cmd.exe "Project.uproject" -run=GasAbilityGenerator \
    -manifest="ClaudeContext/manifest.yaml" \
    -dialoguecsv="ClaudeContext/DialogueData.csv"

# CSV-only mode (empty manifest)
UnrealEditor-Cmd.exe "Project.uproject" -run=GasAbilityGenerator \
    -manifest="ClaudeContext/empty_manifest.yaml" \
    -dialoguecsv="ClaudeContext/DialogueData.csv"
```

## Next Steps for Implementation

1. ~~Create `FDialogueCSVParser` class~~ ✅ DONE
2. ~~Add `ParseDialogueCSV()` to parser~~ ✅ DONE
3. ~~Create sample `DialogueData.csv` with test dialogues~~ ✅ DONE
4. **Add integration code to commandlet**
5. Test generation pipeline
6. Verify in Unreal Editor

---

## References

- Screenshots analyzed: DBP_Seth (7), QBP_NarrativeDemoQuest_OpenWorld (6), Events (6), Conditions (2)
- Existing generators: `FDialogueBlueprintGenerator` in GasAbilityGeneratorGenerators.cpp
- v3.8 dialogue tree support: Full node graph generation with events/conditions

---

## Handoff Summary for Next Terminal

### What's Done:
1. ✅ Full research on Narrative Pro dialogue/quest structure
2. ✅ CSV format designed and documented
3. ✅ Sample data created (`DialogueData.csv`)
4. ✅ Parser class created (`FDialogueCSVParser`)
5. ✅ Integration code documented

### Files Created:
- `ClaudeContext/Quest_Pipeline_Handoff_v1_0.md` - This document
- `ClaudeContext/DialogueData.csv` - Sample dialogue data (3 dialogues)
- `Source/.../GasAbilityGeneratorDialogueCSVParser.h` - Parser header
- `Source/.../GasAbilityGeneratorDialogueCSVParser.cpp` - Parser implementation

### What Needs To Be Done:
1. **Add integration code** to `GasAbilityGeneratorCommandlet.cpp` (code provided above)
2. **Build and test** the plugin
3. **Verify generated dialogues** match DBP_Seth structure
4. **Fix any issues** with speaker names, node connections

### Testing Command:
```bash
powershell -File "Tools/claude_automation.ps1" -Action cycle
# Or with dialogue CSV:
# Add -dialoguecsv parameter support first, then test
```

### Key Design Decisions:
- CSV uses semicolon (`;`) for reply lists (not comma, which is the CSV delimiter)
- First node per dialogue = root node
- Speaker IDs extracted from NPCDef_* prefix
- Events/Conditions use `Type:Value` format
- v3.0 hash computed per dialogue for change detection
