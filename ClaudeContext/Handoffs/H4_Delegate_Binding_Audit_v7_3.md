# H4 Delegate Binding Crash Audit - v7.3

**Date:** 2026-01-27
**Auditors:** Claude + GPT (Dual Audit Session)
**Status:** Implementation Approved

---

## Executive Summary

EdGraphNode.h:586 assertion failure when opening GA_ProtectiveDome was traced to missing C2 skeleton sync in the Actor Blueprint delegate binding pipeline. The crash occurs because BP_FatherCompanion (which GA_ProtectiveDome references via DynamicCast) has delegate bindings generated without proper skeleton synchronization, causing CreateDelegate handler resolution to fail during asset load.

---

## Root Cause Analysis

### H4 Root Cause (Confirmed)
`UK2Node_CreateDelegate::SetFunction(FName)` stores only the handler name; resolution occurs later against the Skeleton Generated Class. If the handler CustomEvent is not registered in skeleton before CreateDelegate is created, the blueprint can be saved in an invalid state that crashes on open/compile.

### Pipeline Audit Results

| Pipeline | CustomEvent | CreateDelegate | C2 Status | C1 Status |
|----------|-------------|----------------|-----------|-----------|
| `GenerateDelegateBindingNodes` (GA) | Line 14599 | Lines 15039, 15104 | ✅ Has C2 | ✅ Has ValidatedMakeLinkTo |
| `GenerateActorDelegateBindingNodes` (Actor BP) | Line 15564 | Lines 15666, 15768 | ❌ MISSING | ❌ 14 direct MakeLinkTo |
| `CreateCustomEventNode` (helper) | Line 10851 | NONE | N/A | N/A |
| `GenerateAttributeBindingNodes` | Line 16127 | NONE (uses LatentAbilityCall) | N/A | N/A |

### Additional Finding: CreateDelegate Ordering Issue

| Path | Order | Status |
|------|-------|--------|
| GA | `SetFunction` → `AllocateDefaultPins` | ✅ Correct |
| Actor BP | `AllocateDefaultPins` → `SetFunction` | ⚠️ Divergent |

---

## Agreed Fix Plan

### MUST-FIX (Proven Root Cause)

| # | Item | Location | Description |
|---|------|----------|-------------|
| 1 | C2 skeleton sync | `GenerateActorDelegateBindingNodes` | Inside `if (!HandlerEvent)` block, after `CreateUserDefinedPin` loop |
| 2 | C1 validated connections | `GenerateActorDelegateBindingNodes` | Replace all 14 `MakeLinkTo` calls with `ValidatedMakeLinkTo` |
| 3 | Failure tracking | `GenerateActorDelegateBindingNodes` | Add `int32 ConnectionFailures = 0;` counter |
| 4 | Negative return | `GenerateActorDelegateBindingNodes` | Return `-ConnectionFailures` on failure |
| 5 | Caller save gate | `FActorBlueprintGenerator::Generate` line 3486 | Add negative-return check |

### INCLUDE IN SAME CHANGE-SET (Recommended Hardening)

| # | Item | Detail |
|---|------|--------|
| 6 | CreateDelegate ordering | Both CreateDelegateA/B: `SetFunction` immediately after `NewObject`, before `AddNode`/`AllocateDefaultPins` |

### GA PATH (Revert)

| # | Item | Detail |
|---|------|--------|
| 7 | Revert C2 implementation | Change `CompileBlueprint(..., RegenerateSkeletonOnly)` back to `MarkBlueprintAsStructurallyModified(Blueprint)` |

---

## Constraints (Audit-Approved)

1. **Do NOT call `ReconstructNode()` on CreateDelegate** - Clears SelectedFunctionName before OutputDelegate wiring
2. **C2 sync must occur BEFORE any CreateDelegate node is created**
3. **Both CreateDelegateA and CreateDelegateB must be updated identically** - Prevents Add/Remove path drift

---

## MakeLinkTo Calls to Replace (14 total)

### Data Links (10)
| Line | Connection |
|------|------------|
| 15636 | VarGetA → GetASC.Actor |
| 15656 | ASC.ReturnValue → CastA.Object |
| 15680 | Self → CreateDelegateA.PN_Self |
| 15698 | CreateDelegateA.OutputDelegate → AddDelegate.Delegate |
| 15705 | SourceASC → AddDelegate.PN_Self |
| 15744 | VarGetB → GetASC.Actor |
| 15762 | ASC.ReturnValue → CastB.Object |
| 15780 | Self → CreateDelegateB.PN_Self |
| 15798 | CreateDelegateB.OutputDelegate → RemoveDelegate.Delegate |
| 15805 | SourceASC → RemoveDelegate.PN_Self |

### Exec Links (4)
| Line | Connection |
|------|------------|
| 15877 | BeginPlay.Then → Cast.Execute |
| 15886 | Cast.Then → AddDelegate.Execute |
| 15952 | EndPlay.Then → Cast.Execute |
| 15960 | Cast.Then → RemoveDelegate.Execute |

---

## Locked Contracts Affected

- **Contract 22 (C1):** Pin Connection Failure Gate - Must be applied to Actor BP path
- **Contract 23 (C2):** Skeleton Sync Before CreateDelegate - Must be applied to Actor BP path

---

## Test Plan

1. Clean assets folder (`/Content/FatherCompanion/`)
2. Build plugin
3. Run generation cycle
4. Open BP_FatherCompanion directly - should not crash
5. Open GA_ProtectiveDome - should not crash
6. Verify no FAIL entries in generation log

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| v7.3 | 2026-01-27 | Initial audit document - Actor BP pipeline fixes |
