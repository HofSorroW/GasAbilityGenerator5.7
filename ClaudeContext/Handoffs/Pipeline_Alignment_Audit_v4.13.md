# GasAbilityGenerator Pipeline Alignment Audit v4.13

**Audit Date:** January 2026
**Scope:** Complete Manifest → Parser → Generator alignment for all 32+ asset types

---

## Executive Summary

This audit traces every field from manifest YAML through parser to generator to identify:
- Fields parsed but not used by generators
- Fields documented but not parsed
- Fields in generator structs but never populated
- Naming mismatches between manifest, docs, and code

### Critical Findings

| Severity | Count | Impact |
|----------|-------|--------|
| **CRITICAL** | 0 | None |
| **HIGH** | 3 | Fields parsed but never used |
| **MEDIUM** | 5 | Naming inconsistencies |
| **LOW** | 4 | Documentation gaps |

---

## HIGH SEVERITY Issues

### H1. BT Decorator `Operation` Field - PARSED BUT NOT USED

**Location:** `FBehaviorTreeGenerator::Generate()`

**Problem:**
- Parser parses `operation:` into `CurrentDecorator.Operation` (line 2498-2500)
- Generator NEVER reads `DecoratorDef.Operation`
- This field is for `BTDecorator_Blackboard`'s KeyQuery (IsSet, IsNotSet, etc.)

**Manifest Example:**
```yaml
decorators:
  - class: BTDecorator_Blackboard
    blackboard_key: AttackTarget
    operation: IsSet      # <-- PARSED BUT NEVER USED
```

**Impact:** BTDecorator_Blackboard nodes never have their KeyQuery configured. They default to whatever UE5 defaults to (likely IsSet, but not guaranteed).

**Fix Required:**
```cpp
// In FBehaviorTreeGenerator::Generate(), add after line ~4621:
// v4.13.2: Set KeyQuery operation for BTDecorator_Blackboard
if (!DecoratorDef.Operation.IsEmpty() && DecoratorClass == UBTDecorator_Blackboard::StaticClass())
{
    UBTDecorator_Blackboard* BBDecorator = Cast<UBTDecorator_Blackboard>(Decorator);
    if (BBDecorator)
    {
        // Map operation string to enum
        if (DecoratorDef.Operation.Equals(TEXT("IsSet"), ESearchCase::IgnoreCase))
        {
            // Set KeyQuery via reflection or direct property access
        }
        // ... other operations
    }
}
```

---

### H2. ActivitySchedule `Location` Field - **RESOLVED (Documentation Fixed)**

**Investigation Result:**
- `UScheduledBehavior_AddNPCGoal` (Narrative Pro) has NO location property
- `UScheduledBehavior_AddNPCGoalByClass` (our helper) only has: `GoalClass`, inherited `ScoreOverride`, `bReselect`
- The `location:` field was **aspirational documentation**, not a real feature

**Resolution:**
CLAUDE.md already fixed (lines 778-786):
- Removed `location:` field from activity_schedules example
- Added NOTE explaining Narrative Pro handles locations via goal-specific classes:
  - `Goal_MoveToDestination` (GoToLocation) - walk to a point
  - `Goal_DriveToDestination` - drive a vehicle to a point
  - `Goal_FlyToDestination` - fly to a point
- Solution: Create goal subclasses with preset destinations (e.g., Goal_GoToForge)

---

### H3. Quest States/Branches - PARTIALLY IMPLEMENTED

**Location:** `FQuestGenerator::Generate()`

**Problem:**
- Parser parses `states:` and `branches:` arrays with tasks, events, conditions
- Generator creates `UQuestBlueprint` but does NOT create state machine nodes
- Dialogue, DialoguePlayParams ARE applied to the CDO
- State machine (UQuestState, UQuestBranch) is NOT generated

**Manifest Example:**
```yaml
states:
  - id: Start
    type: regular
    description: "Talk to the blacksmith"
branches:
  - id: AcceptQuest
    from_state: Start
    to_state: Gathering
    tasks:
      - task_class: BPT_FinishDialogue
        properties:
          dialogue: DBP_BlacksmithQuest
```

**Impact:** ~~Quest state machines require manual setup in editor.~~ **RESOLVED in v4.13+**

**Status:** Quest generator now fully automates runtime state machine:
- `Quest->AddState(State)` for each state
- `Quest->AddBranch(Branch)` for each branch
- `Quest->SetQuestStartState()` for initial state
- UEdGraph visual nodes (editor UI only) are not generated but not required for runtime.

---

## MEDIUM SEVERITY Issues

### M1. Manifest Uses `key_query`, Parser Expects `operation`

**Manifest (actual usage):**
```yaml
decorators:
  - class: BTDecorator_Blackboard
    blackboard_key: AttackTarget
    key_query: IsSet           # <-- USED IN MANIFEST
```

**Parser (expected):**
```cpp
else if (TrimmedLine.StartsWith(TEXT("operation:")))  // <-- EXPECTS "operation:"
{
    CurrentDecorator.Operation = GetLineValue(TrimmedLine);
}
```

**Fix:** Add alias support in parser:
```cpp
else if (TrimmedLine.StartsWith(TEXT("operation:")) || TrimmedLine.StartsWith(TEXT("key_query:")))
```

---

### M2. Manifest Uses `notify_observer`, Parser Doesn't Handle It

**Manifest (actual usage):**
```yaml
decorators:
  - class: BTDecorator_Blackboard
    blackboard_key: AttackTarget
    key_query: IsSet
    notify_observer: OnResultChange    # <-- NOT PARSED
```

**Impact:** NotifyObserver setting is ignored. Decorators may not trigger re-evaluation correctly.

**Fix:** Add parser support and generator property setting.

---

### M3. ActivityConfiguration `rescore_interval` vs `request_interval`

**CLAUDE.md Documentation:**
```yaml
activity_configurations:
  - name: AC_FatherBehavior
    rescore_interval: 1.0    # <-- DOCUMENTED
```

**Parser Code:**
```cpp
else if (TrimmedLine.StartsWith(TEXT("request_interval:")) || TrimmedLine.StartsWith(TEXT("rescore_interval:")))
```

**Status:** Parser already handles both aliases. **NO FIX NEEDED** - documentation is accurate.

---

### M4. Dialogue `Speakers` Array - Logged But Not Applied

**Problem:**
- Parser parses `speakers:` array
- Generator logs speakers but does NOT configure UDialogue::Speakers array

**From CLAUDE.md:**
```yaml
speakers:                                     # Logged for manual setup
  - npc_definition: NPC_Father
    node_color: "#FF6600"
```

**Status:** Documented as "logged for manual setup". This is intentional, not a bug.

---

### M5. Dialogue `DialogueTree` - Parsed But Requires Manual Setup

**Problem:**
- Parser parses entire `dialogue_tree:` structure
- Generator creates UDialogueBlueprint but does NOT create dialogue nodes

**Status:** Documented as requiring manual setup. The dialogue node system is complex and may intentionally require editor work.

---

## LOW SEVERITY Issues

### L1. Activity `Description` Field - Parsed But Not Applied

**Struct has:**
```cpp
FString Description;
```

**Generator does NOT apply to asset.** NarrativeActivityBase may not have a Description property.

---

### L2. BT Service/Decorator Properties Sections - Documentation Gap

**Parser supports:**
```yaml
services:
  - class: BTS_Example
    interval: 0.5
    properties:           # <-- PARSER HANDLES THIS
      SomeKey: SomeValue
```

**CLAUDE.md does NOT document the nested `properties:` section for services and decorators.**

---

### L3. Quest `Questgiver` Field - Documentation Only

**Parser parses `questgiver:` but generator does NOT use it.**

**From CLAUDE.md:**
```yaml
questgiver: NPC_Blacksmith    # Documentation only - links to NPC whose dialogue starts this quest
```

**Status:** Correctly documented as documentation-only. The questgiver pattern is:
NPC (NPCDefinition.dialogue) → Dialogue (with start_quest event) → Quest begins

---

### L4. ItemTableEditor `EquipmentAbilities` vs Parser Inconsistency

**v4.12.4 added:**
- Item Table Editor extracts `EquipmentAbilities` from `UEquippableItem` assets

**Parser parses:**
- `equipment_abilities:` array (v4.2)
- `abilities_to_grant:` array (older)

**Status:** Both work. Parser handles both field names.

---

## Asset Type Coverage Summary

| Asset Type | Parser | Generator | Alignment |
|------------|--------|-----------|-----------|
| Tags | ✅ | ✅ | ✅ Full |
| Enumerations | ✅ | ✅ | ✅ Full |
| InputActions | ✅ | ✅ | ✅ Full |
| InputMappingContexts | ✅ | ✅ | ✅ Full |
| GameplayEffects | ✅ | ✅ | ✅ Full |
| GameplayAbilities | ✅ | ✅ | ✅ Full |
| ActorBlueprints | ✅ | ✅ | ✅ Full |
| WidgetBlueprints | ✅ | ✅ | ✅ Full |
| Blackboards | ✅ | ✅ | ✅ Full |
| **BehaviorTrees** | ✅ | ⚠️ | **H1, M1, M2** |
| Materials | ✅ | ✅ | ✅ Full |
| MaterialFunctions | ✅ | ✅ | ✅ Full |
| MaterialInstances | ✅ | ✅ | ✅ Full |
| FloatCurves | ✅ | ✅ | ✅ Full |
| AnimationMontages | ✅ | ✅ | ✅ Full |
| AnimationNotifies | ✅ | ✅ | ✅ Full |
| DialogueBlueprints | ✅ | ⚠️ | **M4, M5** (intentional) |
| EquippableItems | ✅ | ✅ | ✅ Full |
| Activities | ✅ | ⚠️ | **L1** (minor) |
| AbilityConfigurations | ✅ | ✅ | ✅ Full |
| ActivityConfigurations | ✅ | ✅ | ✅ Full |
| ItemCollections | ✅ | ✅ | ✅ Full |
| NarrativeEvents | ✅ | ✅ | ✅ Full |
| GameplayCues | ✅ | ✅ | ✅ Full |
| NPCDefinitions | ✅ | ✅ | ✅ Full |
| CharacterDefinitions | ✅ | ✅ | ✅ Full |
| TaggedDialogueSets | ✅ | ✅ | ✅ Full |
| NiagaraSystems | ✅ | ✅ | ✅ Full |
| **ActivitySchedules** | ✅ | ⚠️ | **H2** |
| GoalItems | ✅ | ✅ | ✅ Full |
| **Quests** | ✅ | ⚠️ | **H3** (partial) |

---

## Recommended Fixes (Priority Order)

### Priority 1: BT Decorator Operation (H1 + M1 + M2)

**Parser fix (GasAbilityGeneratorParser.cpp:~2498):**
```cpp
// Add alias for key_query
else if (bInDecorators && (TrimmedLine.StartsWith(TEXT("operation:")) || TrimmedLine.StartsWith(TEXT("key_query:"))))
{
    CurrentDecorator.Operation = GetLineValue(TrimmedLine);
}
// Add notify_observer parsing
else if (bInDecorators && TrimmedLine.StartsWith(TEXT("notify_observer:")))
{
    CurrentDecorator.Properties.Add(TEXT("NotifyObserver"), GetLineValue(TrimmedLine));
}
```

**Generator fix (GasAbilityGeneratorGenerators.cpp:~4621):**
```cpp
// Set KeyQuery for BTDecorator_Blackboard
if (!DecoratorDef.Operation.IsEmpty())
{
    // BTDecorator_Blackboard uses GetKeyQuery() which reads from BlackboardKey.SelectedKeyType
    // Need to set the key query type via reflection
    if (FByteProperty* KeyQueryProp = CastField<FByteProperty>(DecoratorClass->FindPropertyByName(TEXT("BasicOperation"))))
    {
        // Map operation string to EBasicKeyOperation::Type
        uint8 QueryValue = 0; // IsSet
        if (DecoratorDef.Operation.Equals(TEXT("IsNotSet"), ESearchCase::IgnoreCase)) QueryValue = 1;
        KeyQueryProp->SetPropertyValue(KeyQueryProp->ContainerPtrToValuePtr<void>(Decorator), QueryValue);
    }
}
```

### Priority 2: ActivitySchedule Location (H2) - **RESOLVED**

**Resolution:** Investigated `UScheduledBehavior_AddNPCGoal` and `UScheduledBehavior_AddNPCGoalByClass` - neither has a location property. The `location:` field was aspirational documentation. CLAUDE.md already fixed to remove the field and add NOTE about goal-specific location classes.

### Priority 3: Quest State Machine (H3) - **RESOLVED**

**Resolution:** Full state machine generation implemented in v4.13+. Generator uses correct UQuest APIs:
- `Quest->AddState(State)` creates UQuestState objects
- `Quest->AddBranch(Branch)` creates UQuestBranch objects
- `Quest->SetQuestStartState()` sets initial state
- Runtime state machine fully functional without UEdGraph visual nodes

---

## Validation Checklist

After fixes are applied, verify:

- [x] BT with `key_query: IsSet` generates decorator with correct KeyQuery **(RESOLVED v4.13.2)**
- [x] BT with `notify_observer: OnResultChange` generates decorator with correct observer **(RESOLVED v4.13.2)**
- [x] Activity schedules `location:` field - doc updated to remove unsupported field **(RESOLVED)**
- [x] Quest generator creates full state machine **(RESOLVED v4.13+)**

---

## Conclusion

The GasAbilityGenerator plugin has **excellent overall alignment** between manifest, parser, and generator.

**Status Update (v4.13.2):**
- ~~**3 HIGH severity**~~ → **ALL HIGH SEVERITY RESOLVED** ✅
  - ✅ H1: BT decorator fields - **RESOLVED** in v4.13.2 (key_query alias, notify_observer, BasicOperation, NotifyObserver)
  - ✅ H2: ActivitySchedule location - **RESOLVED** (doc fixed - field was aspirational, not a real NP feature)
  - ✅ H3: Quest state machine - **RESOLVED** in v4.13+ (full AddState/AddBranch/SetQuestStartState automation)
- **5 MEDIUM severity:** Mostly naming inconsistencies (some already handled as aliases)
- **4 LOW severity:** Documentation gaps and intentional limitations

**All HIGH severity issues resolved.** Remaining work is MEDIUM/LOW severity cleanup.

---

*Report generated by Claude Code Pipeline Alignment Audit*
