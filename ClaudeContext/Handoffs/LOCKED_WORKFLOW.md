# LOCKED_WORKFLOW.md (v1.0)

## Purpose

This document defines **mandatory workflow rules** for manifest authoring and error handling in GasAbilityGenerator.
These rules eliminate the "guess → rerun → guess" anti-pattern that wastes cycles and produces dead-on-arrival assets.

**Rule:** All agents (Claude, GPT, or human) editing manifest.yaml MUST follow these rules. Violations require explicit justification.

---

## Definitions

### Manifest Pattern
A working node/connection/function usage that already exists in `manifest.yaml` and generates successfully.

### BP-Visible
A function or property that is callable from Blueprint (has `BlueprintCallable`, `BlueprintPure`, `BlueprintReadOnly`, or `BlueprintReadWrite` specifier).

### Correctness Blocker
An error that prevents asset generation or produces non-functional assets. Must be fixed before proceeding.

### Polish Issue
A warning or style issue that does not prevent generation. Tracked as P3, not blocking.

---

## LOCKED RULE 1 — Manifest-First Lookup Order

### Invariant
Before adding or editing ANY manifest node, connection, function call, or pin reference, the agent MUST follow this lookup order:

1. **Manifest-first:** Search `manifest.yaml` for an existing working instance of the same:
   - Node type (CallFunction, DynamicCast, Branch, etc.)
   - Function name
   - Pin name pattern
   - Delegate binding style

2. **Engine-second:** If no manifest pattern exists, verify in engine/plugin headers:
   - Function signature exists
   - Function is BP-visible (`BlueprintCallable`/`BlueprintPure`)
   - Property is BP-visible (not private C++ only)
   - Correct owning class

3. **Web-third:** Only if engine/project sources don't answer the question.

4. **Trial-and-error last:** Only after steps 1-3 are exhausted and documented.

### Proof Artifact Requirement
Every manifest change MUST cite at least one of:
- `[MANIFEST]` - existing working line(s) or node ID(s) from manifest.yaml
- `[ENGINE]` - engine header path + function/property signature
- `[PLUGIN]` - Narrative Pro or plugin header path + signature
- `[WEB]` - URL with relevant documentation

### Forbidden
- Guessing pin names without checking manifest patterns
- Assuming a property is BP-visible without verification
- Repeating known mistakes (Exec vs Then, CastFailed spacing, etc.)
- Using trial-and-error as first resort

### Example
```yaml
# Adding a new GetController call
# [MANIFEST] Line 3516, 9133: function: GetController with pin "OwnerController"
# [PLUGIN] NarrativeNPCCharacter.h - GetController() is BlueprintCallable
- id: GetController
  type: CallFunction
  properties:
    function: GetController
```

---

## LOCKED RULE 2 — Error TODO Queue (Mandatory Consumption)

### Invariant
If a run fails (pre-validation OR generation), the agent MUST produce an **Error TODO Queue** before any retry.

### TODO Queue Format (JSON)
```json
{
  "run_timestamp": "2026-01-27T14:30:00Z",
  "failure_type": "PRE-VAL|GENERATION",
  "blocking_count": 2,
  "errors": [
    {
      "code": "E_PREVAL_ATTRIBUTE_NOT_FOUND",
      "rule_id": "A2",
      "item_id": "GE_Example",
      "yaml_path": "gameplay_effects[5].modifiers[0].attribute",
      "message": "Attribute 'Defense' not found on NarrativeAttributeSetBase",
      "fix_hypothesis": "Use 'Armor' instead of 'Defense'",
      "evidence": "[PRE-VAL] [E_PREVAL_ATTRIBUTE_NOT_FOUND] A2 | GE_Example | ..."
    }
  ]
}
```

### Minimum Text Format (if JSON not practical)
```
=== ERROR TODO QUEUE ===
Failure type: PRE-VAL
Blocking items: 2

[1] E_PREVAL_ATTRIBUTE_NOT_FOUND | GE_Example | gameplay_effects[5]
    Evidence: [PRE-VAL] [E_PREVAL_ATTRIBUTE_NOT_FOUND] A2 | GE_Example | Attribute 'Defense' not found
    Fix hypothesis: Use 'Armor' instead of 'Defense'

[2] E_PREVAL_CLASS_NOT_FOUND | GA_Example | parent_class
    Evidence: [PRE-VAL] [E_PREVAL_CLASS_NOT_FOUND] C1 | GA_Example | Class 'InvalidClass' not found
    Fix hypothesis: Check class name spelling, verify BP-visible
```

### Hard Constraint
- NO rerun until TODO queue is empty
- Each error must have a fix hypothesis before retry
- Deferred items require explicit justification

### Forbidden
- Retrying without reading error output
- Ignoring pre-validation errors
- "Trying something different" without addressing logged errors

---

## LOCKED RULE 3 — Fix Completion Criteria

### Invariant
A fix is ONLY considered complete when:

1. The specific error line **disappears** from logs, AND
2. One of these success conditions is met:
   - Asset generates with **full connections** (e.g., `Connections 45/45`)
   - Asset is **correctly blocked** with documented reason (intentional skip)
   - Error is **reclassified** as P3 polish (not correctness blocker)

### Success Log Pattern
```
Event graph 'GA_Example' summary: Nodes 23/23, Connections 45/45
```

### Forbidden
- Claiming "fixed" while error still appears in logs
- Ignoring connection failures (e.g., `Connections 44/45`)
- Proceeding with partial success without investigation

---

## LOCKED RULE 4 — Canonical Pin Naming

### Invariant
Pin names MUST use canonical spellings from existing manifest patterns or engine sources.

### Canonical Pin Reference

| Category | Canonical Name | Aliases Accepted | Notes |
|----------|---------------|------------------|-------|
| Exec Input | `Exec` | `Execute`, `execute` | Input execution pin |
| Exec Output | `Then` | `then` | Output execution pin |
| Return Value | `ReturnValue` | `Return`, `Result` | Function output |
| Self/Target | `Target` | `Self`, `self` | Object reference |
| Cast Input | `Object` | `InObject`, `InputObject` | Cast/IsValid input |
| Cast Output | `As<ClassName>` | - | No spaces: `AsBP_FatherCompanion` |
| Cast Failed | `CastFailed` | - | Not `Cast Failed` (no space) |
| Branch Condition | `Condition` | `bCondition` | Boolean input |
| Branch True | `True` | `IfTrue` | True execution path |
| Branch False | `False` | `IfFalse` | False execution path |
| Loop Body | `LoopBody` | `Loop Body` | ForEach execution |
| Array Element | `Array Element` | `ArrayElement` | ForEach output |

### Node-Specific Pins
If a pin differs on a specific node (e.g., `Target Data`, `TargetActor`, `OwnerController`), the deviation MUST be backed by:
- Manifest precedent (cite line number), OR
- Engine signature (cite header path)

### Forbidden
- Inventing pin names without evidence
- Using inconsistent casing (`exec` vs `Exec`)
- Adding spaces where none exist (`CastFailed` not `Cast Failed`)

---

## LOCKED RULE 5 — Error Classification

### Invariant
Errors are classified into two categories with different handling requirements.

### Correctness Blockers (MUST FIX)
| Error Type | Example | Action |
|------------|---------|--------|
| Pre-validation errors | `E_PREVAL_*` | Fix before generation |
| Pin not found | `Connection failed: Output pin 'X' not found` | Fix pin name |
| Type mismatch | `CONNECT_RESPONSE_DISALLOW` | Fix connection types |
| Missing asset refs | `E_PREVAL_ASSET_NOT_FOUND` | Create or fix path |

### Type Warnings (Pre-validation - SHOULD FIX)
| Issue Type | Example | Action |
|------------|---------|--------|
| Redundant cast | `E_PREVAL_REDUNDANT_CAST` "ReturnValue is already type X" | Remove unnecessary cast |

### Polish Issues (P3 - Track, Don't Block)
| Issue Type | Example | Action |
|------------|---------|--------|
| Style warnings | Unused variable | Log, continue |
| Editor warnings | Non-blocking UE warnings | Log, continue |

### Forbidden
- Treating correctness blockers as polish
- Ignoring connection failures as "warnings"
- Shipping assets with unresolved blockers

---

## LOCKED RULE 6 — P1 Escalation Trigger

### Invariant
If the agent encounters **3 or more repeated pin-name or node-type errors** in a single session despite following P0 rules:

1. Document the pattern (which pins/nodes keep failing)
2. Escalate to P1 (alias-based pre-validation enhancement)
3. Do NOT continue brute-forcing

### Escalation Format
```
=== P1 ESCALATION REQUEST ===
Repeated errors: 3+
Pattern: [describe the recurring mistake]
Evidence:
  - Run 1: [error]
  - Run 2: [same error]
  - Run 3: [same error]
Recommendation: Add to pre-validator alias check
```

### Forbidden
- Continuing past 3 repeated errors without escalation
- Blaming "engine quirks" for systematic mistakes

---

## LOCKED RULE 7 — NEVER Simplify Abilities (v7.8.0)

### Invariant
Abilities in the manifest MUST match their implementation guides exactly. The generator serves the design, not the other way around.

### If Generator Limitation Prevents Implementation

1. **STOP** - Do not simplify or work around the limitation
2. **ENHANCE** - Add generator support for the required pattern/node type
3. **VERIFY** - Ensure the ability matches the guide's specified gameplay mechanics

### Forbidden Simplifications

| Forbidden | Correct Action |
|-----------|----------------|
| Replacing damage-scaled values with fixed amounts | Add attribute tracking support |
| Removing attribute tracking due to complexity | Implement required AbilityTask/Async support |
| Skipping validation to avoid new node types | Add required node type to generator |
| Using "close enough" alternatives that change gameplay | Match guide exactly |
| Hardcoding dynamic values | Implement proper data flow |

### Examples

**FORBIDDEN:**
```yaml
# Guide: "30% of damage becomes energy"
# WRONG: Fixed energy per hit (simplification)
param.Value: 50.0
```

**REQUIRED:**
```yaml
# Guide: "30% of damage becomes energy"
# CORRECT: Calculate actual 30% of damage received
- id: GetDamageParam  # From OnDamagedBy delegate
- id: MultiplyBy30Percent  # Damage * 0.3
- id: AddToEnergy  # Add calculated amount
```

### Enforcement

- If a guide specifies damage scaling → ability MUST use damage scaling
- If a guide specifies attribute tracking → ability MUST track the attribute
- Numerical values in guides are requirements, not suggestions

### Reference

- Contract: `LOCKED_CONTRACTS.md` — Contract 25 (C_NEVER_SIMPLIFY_ABILITIES)
- Design Guides: `ClaudeContext/Handoffs/Father_Companion_*.md`

---

## Quick Reference Card

### Before ANY Manifest Edit
```
1. grep manifest.yaml for existing pattern
2. If not found, check engine/plugin headers for BP visibility
3. If still unclear, web search
4. Trial-and-error ONLY as documented last resort
5. Cite evidence: [MANIFEST], [ENGINE], [PLUGIN], or [WEB]
```

### On ANY Failure
```
1. Read Tools/Logs/commandlet_output.log
2. Extract ALL [PRE-VAL] and [FAIL] lines
3. Create Error TODO Queue with fix hypotheses
4. Fix each item
5. Verify fix (error disappears, connections complete)
6. Only then retry
```

### Canonical Pins (Memory Aid)
```
EXEC: Exec (in), Then (out)
DATA: ReturnValue, Target, Object, Condition
CAST: Object (in), As<Class> (out), CastFailed
BRANCH: Condition, True, False
```

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| v1.0 | 2026-01-27 | Initial locked workflow rules (Claude-GPT audit convergence) |
| v1.1 | 2026-01-28 | Added LOCKED RULE 7 — NEVER Simplify Abilities: Abilities MUST match implementation guides exactly. Generator serves the design, not the other way around. |

---

## References

- `LOCKED_CONTRACTS.md` - Data safety contracts
- `CLAUDE.md` - Plugin-specific guidance
- `manifest.yaml` - Source of truth for working patterns
- `GasAbilityGeneratorTypes.h:600` - Error format definition
- `GasAbilityGeneratorGenerators.cpp:12887` - Pin alias system
