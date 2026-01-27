# Delegate Binding Crash Audit v1.0

**Date:** 2026-01-27
**Session:** Claude-GPT Dual Audit
**Status:** IN PROGRESS - Root cause identified, fix pending

---

## Executive Summary

Assets with `delegate_bindings` in the manifest crash when loaded via `StaticLoadObject` during `CheckExistsWithMetadata`. The crash is a `FindPinChecked` assertion failure at `EdGraphNode.h:586`.

**Workaround:** Delete all delegate-binding assets before regeneration. They work when regenerated together in a single run, but crash on subsequent loads.

---

## Confirmed Crash Pattern

### Test A Isolation Results

| Test | Result |
|------|--------|
| GA_ProtectiveDome WITHOUT delegate_bindings | SUCCESS (47/47 nodes, 59/59 connections) |
| GA_ProtectiveDome WITH delegate_bindings | SUCCESS as [NEW], CRASH on reload |
| All 6 delegate-binding assets deleted + regenerated | SUCCESS in same run |
| Subsequent run loading those assets | CRASH |

### Crash Location

```
[AUDIT] About to StaticLoadObject: .../GA_ProtectiveDome.GA_ProtectiveDome (CheckExistsWithMetadata)
[Callstack] 0x...UnrealEditor-GasAbilityGenerator.dll!FGeneratorBase::CheckExistsWithMetadata() [...:1193]
```

The crash occurs at line 1193 in `GasAbilityGeneratorGenerators.cpp`:
```cpp
ExistingAsset = StaticLoadObject(UObject::StaticClass(), nullptr, *FullAssetPath);
```

---

## Assets with delegate_bindings

6 assets in manifest.yaml have delegate_bindings:

| Asset | Line | Bindings |
|-------|------|----------|
| GA_FatherCrawler | 862 | OnDied→HandleOwnerDeath |
| GA_FatherArmor | 1327 | OnDied→HandleOwnerDeath |
| GA_FatherSymbiote | 2521 | OnDied→HandleOwnerDeath |
| GA_ProtectiveDome | 4474 | OnDamagedBy→HandleDomeDamageAbsorption, OnDied→HandlePlayerDeathReset |
| GA_StealthField | 5261 | OnDied→HandleOwnerDeath, OnReceivedDamage→HandleStealthBreak |
| BP_FatherCompanion | 7841 | OnDied→HandleDeath |

---

## C2 Contract Status (LOCKED CONTRACT 23)

The C2 contract - skeleton sync before CreateDelegate - is implemented:

```cpp
// Line 14669 (GA pipeline)
FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
LogGeneration(TEXT("    Skeleton sync performed (C2: handler registered before CreateDelegate)"));

// Line 15614 (Actor BP pipeline)
FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
LogGeneration(TEXT("    Skeleton sync performed (C2: handler registered before CreateDelegate)"));
```

**Finding:** C2 is present but crash still occurs. The skeleton sync alone is not sufficient.

---

## Hypothesis: Serialization Issue

The assets generate "successfully" but crash on reload. This suggests:

1. CreateDelegate nodes are created correctly during generation
2. The asset is saved with malformed pin references
3. On reload, `FindPinChecked` fails because expected pins don't exist

**GPT's Theory:** "Do not StaticLoadObject just to read metadata" - the load triggers Blueprint compilation, which exposes the malformed delegate nodes.

---

## AUDIT Trace Added

Line 1192 in `GasAbilityGeneratorGenerators.cpp`:
```cpp
UE_LOG(LogGasAbilityGenerator, Display, TEXT("[AUDIT] About to StaticLoadObject: %s (CheckExistsWithMetadata)"), *FullAssetPath);
```

This identifies which asset is being loaded when crash occurs.

---

## Next Steps (when resuming)

1. **Investigate pin serialization** - Check if CreateDelegate's OutputDelegate pin is properly wired before save
2. **Check ReconstructNode timing** - The code explicitly avoids ReconstructNode() because it clears SelectedFunctionName
3. **Test explicit compile before save** - Force `FKismetEditorUtilities::CompileBlueprint` after delegate binding generation
4. **GPT recommendation** - Replace StaticLoadObject with AssetRegistry tag-based metadata checks

---

## Pre-existing Failures (Unrelated)

4 assets fail for unrelated reasons:
- GoalGenerator_RandomAggression - StopFollowing function issue
- GA_CoreLaser - Event graph issue
- AC_WardenCore, AC_ReturnedStalkerBehavior - Related issues

---

## Session Log Files

- `Tools/Logs/commandlet_output.log` - Contains [AUDIT] traces
- `Tools/Logs/editor_crash.log` - Basic crash info (not detailed)

---

## Locked Decisions

| Contract | Status | Implementation |
|----------|--------|----------------|
| C1 (Pin Validation) | Implemented | `ValidatedMakeLinkTo` 3-step validation |
| C2 (Skeleton Sync) | Implemented but insufficient | `MarkBlueprintAsStructurallyModified` before CreateDelegate |
