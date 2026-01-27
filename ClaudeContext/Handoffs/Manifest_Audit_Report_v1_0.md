# MANIFEST AUDIT REPORT - CURRENT SITUATION (2026-01-27)

## Document Info
- **Version:** v1.0
- **Date:** 2026-01-27
- **Auditors:** Claude, GPT
- **Arbiter:** Erdem

---

## AUDIT PARTICIPANTS
- **Claude** (Auditor)
- **GPT** (Auditor)
- **Erdem** (Arbiter)

## AUDIT RULES
1. Both sides are auditors - challenge and validate
2. Require evidence from UE, Narrative Pro, or plugin code
3. Don't accept claims by default
4. No coding unless Erdem approves
5. Naming conventions align with Narrative Pro

---

## SECTION 1: INVENTORY BASELINE (VERIFIED)

| Asset Type | Count | Prefix | Status |
|------------|-------|--------|--------|
| AbilityConfigurations | 11 | AC_ | Verified |
| ActivityConfigurations | 11 | AC_ | Verified |
| GameplayAbilities | 21 | GA_ | Verified |
| GameplayEffects | 42 | GE_ | Verified |
| BehaviorTrees | 5 | BT_ | Verified |
| BTServices | 4 | BTS_ | Verified |
| **Total Assets** | **194** | - | All generate |

**Generation Result (Latest Run):**
```
RESULT: New=1 Skipped=193 Failed=0 Deferred=0 Cascaded=0 CascadeRoots=0 Total=194
```

---

## SECTION 2: CROSS-DOCUMENT CONSISTENCY

### 2.1 Design v2.8 <-> Manifest Alignment

| Item | In Design v2.8 | In Manifest | Status |
|------|----------------|-------------|--------|
| GA_ProtectiveDome | Lines 195, 247, 766 | Present | ALIGNED |
| GA_StealthField | Lines 332, 771 | Present | ALIGNED |
| GA_CoreLaser | Not Father system | Present | OUT-OF-SCOPE (Warden) |
| GA_Death | Narrative Pro built-in | Present | OUT-OF-SCOPE (NP default) |

**Verdict:** Only 2 abilities (GA_CoreLaser, GA_Death) are outside Father design scope. Both are intentional.

### 2.2 Naming Convention: AC_ vs ActConfig_

| Source | Convention | Evidence |
|--------|------------|----------|
| Tech Ref v6.8 | `ActConfig_` | Multiple occurrences in doc |
| Narrative Pro Plugin | `AC_` | Zero `ActConfig_*.uasset` files exist |
| Manifest | `AC_` | All 22 configurations use AC_ |
| **Project Decision** | `AC_` | Erdem confirmed: "AC_ is correct, inline with narrative" |

**Verdict:** Tech Ref v6.8 contains ERRATA. Project uses `AC_` aligned with Narrative Pro.

---

## SECTION 3: PHASE A - NARRATIVE PRO CLASS CONSISTENCY

### 3.1 Classes Used (578 references analyzed)

**Narrative Pro Classes (CORRECTLY USED):**

| Class | Usage | Status |
|-------|-------|--------|
| NarrativeGameplayAbility | 21 | All GA_ parent |
| NarrativeNPCCharacter | 9 | NPC BP parent |
| NarrativePlayerCharacter | 10 | Player references |
| NarrativeProjectile | 4 | Projectile parent |
| NarrativeActivityBase | 5 | BPA_ parent |
| NarrativeInventoryComponent | 8 | Inventory system |
| NPCActivityComponent | 6 | AI activities |
| NPCGoalItem / NPCGoalGenerator | 8 | Goal system |
| NarrativeDamageExecCalc | 5 | Damage calculation |
| ArsenalStatics | 4 | Settings access |
| Dialogue (UDialogue) | 8 | Dialogue parent |

**GAS Core Classes (Required - No Narrative Wrapper):**

| Class | Usage | Reason |
|-------|-------|--------|
| AbilitySystemComponent | 12 | Base GAS functions only |
| AbilitySystemBlueprintLibrary | 33 | GAS BP library |
| BlackboardComponent | 7 | UE AI standard |

**UE Core Classes (Acceptable for Upcasting):**

| Class | Usage | Reason |
|-------|-------|--------|
| Actor | 30 | Generic actor refs |
| Character | 18 | Base class upcasting |
| CharacterMovementComponent | 14 | Base CMC functions |
| Pawn / Controller / AIController | 7 | Standard UE types |

### 3.2 Parallel System Risk Assessment

| System | Status | Evidence |
|--------|--------|----------|
| Abilities | NO PARALLEL | All use NarrativeGameplayAbility |
| Characters | NO PARALLEL | NarrativeNPCCharacter / NarrativePlayerCharacter |
| Inventory | NO PARALLEL | NarrativeInventoryComponent |
| AI/Activities | NO PARALLEL | NPCActivityComponent, NarrativeActivityBase |
| Goals | NO PARALLEL | NPCGoalItem, NPCGoalGenerator |
| Damage | NO PARALLEL | NarrativeDamageExecCalc |
| ASC Access | ACCEPTABLE | Uses base GetASC, only calls base GAS functions |

**Verdict:** No parallel systems detected. All major systems use Narrative Pro classes.

### 3.3 ArsenalStatics Usage Note

ArsenalStatics provides `GetNarrativeAbilitySystemComponent(Actor)` but manifest uses base `GetAbilitySystemComponent`.

**Assessment:** ACCEPTABLE because all ASC functions called are base GAS:
- AddLooseGameplayTags
- RemoveLooseGameplayTags
- HasMatchingGameplayTag
- TryActivateAbilityByClass
- BP_ApplyGameplayEffectToSelf

**Contract:** If future needs require NarrativeASC-specific functions, must switch to ArsenalStatics accessor.

---

## SECTION 4: PHASE B - NODE/PIN VALIDATION

### 4.1 Node Type Inventory (964 nodes)

| Node Type | Count | UE5.7 K2Node | Status |
|-----------|-------|--------------|--------|
| CallFunction | 462 | UK2Node_CallFunction | VALID |
| VariableGet | 119 | UK2Node_VariableGet | VALID |
| Branch | 76 | UK2Node_IfThenElse | VALID |
| VariableSet | 66 | UK2Node_VariableSet | VALID |
| PropertyGet | 53 | UK2Node_VariableGet | VALID |
| Event | 49 | UK2Node_Event | VALID |
| DynamicCast | 46 | UK2Node_DynamicCast | VALID |
| PropertySet | 26 | UK2Node_VariableSet | VALID |
| PrintString | 15 | UK2Node_CallFunction | VALID |
| CustomEvent | 13 | UK2Node_CustomEvent | VALID |
| MakeArray | 8 | UK2Node_MakeArray | VALID |
| AbilityTaskWaitDelay | 7 | UK2Node_LatentGameplayTaskCall | VALID |
| Sequence | 6 | UK2Node_ExecutionSequence | VALID |
| Self | 6 | UK2Node_Self | VALID |
| ForEachLoop | 4 | UK2Node_ForEachElementInArray | VALID |
| SpawnActor | 3 | UK2Node_SpawnActor | VALID |
| GetArrayItem | 2 | UK2Node_GetArrayItem | VALID |
| BreakStruct | 2 | UK2Node_BreakStruct | VALID |
| Delay | 1 | UK2Node_Delay | VALID |

**Verdict:** All 16 node types are valid UE5.7 K2Nodes.

### 4.2 Pin Name Compliance (LOCKED_WORKFLOW Rule 4)

**Canonical Pins (High Frequency - All Correct):**

| Pin | Count | Canonical | Status |
|-----|-------|-----------|--------|
| Exec | 734 | Yes | COMPLIANT |
| ReturnValue | 317 | Yes | COMPLIANT |
| Target | 195 | Yes | COMPLIANT |
| Then | 142 | Yes | COMPLIANT |
| Object | 76 | Yes | COMPLIANT |
| Condition | 76 | Yes | COMPLIANT |
| True | 66 | Yes | COMPLIANT |
| False | 15 | Yes | COMPLIANT |
| AsBP_FatherCompanion | 62 | Yes (no space) | COMPLIANT |
| CastFailed | 3 | Yes (no space) | COMPLIANT |
| LoopBody | 5 | Yes (no space) | COMPLIANT |
| ArrayElement | 6 | Yes (no space) | COMPLIANT |

**Pin Drift Found (Style Issues):**

| Pin | Count | Lines | Generator Handling |
|-----|-------|-------|-------------------|
| `then` (lowercase) | 8 | 1746, 1867, 1869, 2475, 3730, 3732, 4412, 5973 | Alias system handles |
| `self` (lowercase) | 6 | 4450, 6009, 6142, 9317, 10334, 10356 | Alias system handles |

**Generator Alias System Evidence:**
```cpp
// GasAbilityGeneratorGenerators.cpp:12910-12913
else if (PinName.Equals(TEXT("Then"), ESearchCase::IgnoreCase))
{
    SearchNames.Add(UEdGraphSchema_K2::PN_Then.ToString());
    SearchNames.Add(TEXT("then"));  // Explicitly handles lowercase
}

// Lines 12922-12927
else if (PinName.Equals(TEXT("Target"), ESearchCase::IgnoreCase) ||
         PinName.Equals(TEXT("Self"), ESearchCase::IgnoreCase))
{
    SearchNames.Add(UEdGraphSchema_K2::PN_Self.ToString());
    SearchNames.Add(TEXT("self"));  // Explicitly handles lowercase
    SearchNames.Add(TEXT("Target"));
}
```

**Classification:** P3 Style Issue (not P0 correctness blocker) - generator handles by design.

### 4.3 Math Function Consistency

| Function Pattern | Count | Contexts |
|-----------------|-------|----------|
| `*_DoubleDouble` | 11 | GA_FatherArmor, GA_FatherExoskeleton, GA_ProtectiveDome, GA_FatherExoskeletonSprint |
| `*_FloatFloat` | 9 | BTS_HealNearbyAllies, BTS_CheckExplosionProximity, GoalGenerator_FatherTargetSelection |

**Breakdown of FloatFloat (9 occurrences):**
- LessEqual_FloatFloat (line 9969)
- Divide_FloatFloat (line 10509)
- Less_FloatFloat (line 10521)
- Multiply_FloatFloat (lines 11899, 11952, 12020)
- Add_FloatFloat (lines 11910, 12031)
- Subtract_FloatFloat (line 11963)

**Assessment:** Author drift across different manifest sections. UE5.7 prefers Double. Not causing failures but inconsistent.

---

## SECTION 5: CURRENT WARNINGS (Non-Blocking)

| Warning Code | Count | Assets Affected | Severity |
|-------------|-------|-----------------|----------|
| E_PREVAL_DIALOGUE_ROOT_NOT_FOUND | 5 | DBP_Returned_* dialogues | P3 - Manual setup |
| E_COOLDOWNGE_NOT_FOUND | 1 | GA_CoreLaser | P3 - Dependency order |
| BTService_BlueprintBase not found | 1 | Headless mode only | P3 - Works at runtime |

**These are warnings, not failures. Generation completes successfully.**

---

## SECTION 6: LOCKED PATTERNS STATUS

### 6.1 Patterns WITH Locked Contracts

| Pattern | Contract | Status |
|---------|----------|--------|
| Delegate bindings (const-ref params) | Track E Native Bridge | LOCKED (v4.31) |
| Form state architecture | Option B (GE-based) | LOCKED |
| Father.Form.* tag gating | Removed per design | LOCKED |
| AC_ naming convention | Narrative Pro alignment | LOCKED (Erdem decision) |

### 6.2 Patterns WITHOUT Locked Contracts (Gap)

| Pattern | Usage | Risk |
|---------|-------|------|
| BT Service tick logic (ReceiveTickAI) | BTS_CheckExplosionProximity, BTS_FormationFollow, etc. | No frequency/rate limiting contract |
| BeginPlay MovementMode overrides | SetMovementMode(Flying) in BP_FatherCompanion | No safeguard contract |

**Recommendation:** Consider locking these patterns after P0 exercises complete.

---

## SECTION 7: AUDIT DISPUTE RESOLUTION

| Dispute | Claude Position | GPT Position | Resolution |
|---------|-----------------|--------------|------------|
| `self` count | 6 occurrences | 5 occurrences | **Claude correct** (grep verified) |
| `*_FloatFloat` count | 6 occurrences | 9 occurrences | **GPT correct** (Claude's grep was too narrow) |
| `then`/`self` severity | P3 Style Issue | P0 Correctness Risk | **Claude position** (alias system handles by design, 0 failures) |
| Duplicate YAML keys | Allowed (APPEND parser) | "Silent overwrite" risk | **Claude correct** (parser verified) |
| Delegate binding crash risk | Safe (Track E complete) | Crash trigger | **Claude correct** (v4.31 bridge verified) |

---

## SECTION 8: SUMMARY

### What's Working
- **194/194 assets generate successfully** (0 failures)
- **No parallel systems** - all use Narrative Pro classes
- **All node types valid** for UE5.7
- **All canonical pins compliant** with LOCKED_WORKFLOW
- **Alias system handles** lowercase pin drift

### What Needs Attention (P3 - Non-Blocking)
- 5 dialogue root node warnings (manual setup items)
- 1 cooldown GE dependency warning
- Float/Double math function inconsistency (style)
- 14 lowercase pin occurrences (style drift)

### What's Locked
- Track E Native Bridge for delegate bindings
- AC_ naming convention (Narrative Pro aligned)
- Form state GE-based architecture
- No parallel systems policy

---

## APPENDIX A: PIN DRIFT LOCATIONS (For Future Cleanup)

### `then` (lowercase) - 8 occurrences
```
Line 1746: - from: [Branch_Valid, then]
Line 1867: - from: [Branch_WasCancelled, then]
Line 1869: - from: [Branch_End, then]
Line 2475: - from: [Branch_WasCancelled, then]
Line 3730: - from: [Branch_WasCancelled, then]
Line 3732: - from: [Branch_Valid_End, then]
Line 4412: - from: [Branch_Enemy, then]
Line 5973: - from: [Branch_Enemy, then]
```

### `self` (lowercase) - 6 occurrences
```
Line 4450: to: [ApplyGE, self]
Line 6009: to: [ApplyGE, self]
Line 6142: to: [ApplyGE, self]
Line 9317: to: [ApplyDamageGE, self]
Line 10334: - from: [SelfForTimer, self]
Line 10356: - from: [SelfRef, self]
```

---

## APPENDIX B: FLOAT/DOUBLE MATH FUNCTION LOCATIONS

### `*_FloatFloat` - 9 occurrences
```
Line 9969:  function: LessEqual_FloatFloat
Line 10509: function: Divide_FloatFloat
Line 10521: function: Less_FloatFloat
Line 11899: function: Multiply_FloatFloat
Line 11910: function: Add_FloatFloat
Line 11952: function: Multiply_FloatFloat
Line 11963: function: Subtract_FloatFloat
Line 12020: function: Multiply_FloatFloat
Line 12031: function: Add_FloatFloat
```

### `*_DoubleDouble` - 11 occurrences
```
Line 1618:  function: Multiply_DoubleDouble
Line 2187:  function: Multiply_DoubleDouble
Line 2216:  function: Multiply_DoubleDouble
Line 4650:  function: Multiply_DoubleDouble
Line 4663:  function: Add_DoubleDouble
Line 4697:  function: GreaterEqual_DoubleDouble
Line 5161:  function: Multiply_DoubleDouble
Line 12291: function: Less_DoubleDouble
Line 12384: function: Less_DoubleDouble
Line 12456: function: Less_DoubleDouble
Line 12548: function: Less_DoubleDouble
```

---

## VERSION HISTORY

| Version | Date | Changes |
|---------|------|---------|
| v1.0 | 2026-01-27 | Initial audit report (Claude-GPT convergence) |
