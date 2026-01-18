# LOCKED_SYSTEMS.md (v4.12.5)

## Purpose

This document maps each LOCKED contract to its current implementation files/functions.
Use this for traceability during refactors - code can move, but contracts must remain true.

---

## Contract 1 — Metadata Contract

| Component | File | Function/Class |
|-----------|------|----------------|
| TryGetMetadata | `GasAbilityGeneratorMetadata.h:244-253` | `GeneratorMetadataHelpers::TryGetMetadata()` |
| GetMetadataEx | `GasAbilityGeneratorMetadata.cpp:301-330` | `GeneratorMetadataHelpers::GetMetadataEx()` |
| HasMetadataEx | `GasAbilityGeneratorMetadata.cpp:426-429` | `GeneratorMetadataHelpers::HasMetadataEx()` |
| SetMetadata | `GasAbilityGeneratorMetadata.cpp:353-393` | `GeneratorMetadataHelpers::SetMetadata()` |
| Registry class | `GasAbilityGeneratorMetadata.h:138-176` | `UGeneratorMetadataRegistry` |
| AssetUserData class | `GasAbilityGeneratorMetadata.h:34-84` | `UGeneratorAssetMetadata` |

**Decision Path Implementations (must use registry-aware API):**
- `GasAbilityGeneratorGenerators.cpp:1177-1193` - `CheckExistsWithMetadata()`
- `GasAbilityGeneratorWindow.cpp:108-123` - NPC asset status check

---

## Contract 2 — Regen/Diff Semantics

| Component | File | Function/Class |
|-----------|------|----------------|
| InputHash meaning | `GasAbilityGeneratorTypes.h:3894` | `FGeneratorMetadata::InputHash` |
| OutputHash meaning | `GasAbilityGeneratorTypes.h:3897` | `FGeneratorMetadata::OutputHash` |
| HasInputChanged | `GasAbilityGeneratorTypes.h:3906-3909` | `FGeneratorMetadata::HasInputChanged()` |
| HasOutputChanged | `GasAbilityGeneratorTypes.h:3912-3915` | `FGeneratorMetadata::HasOutputChanged()` |
| ComputeHash (definitions) | Throughout `GasAbilityGeneratorTypes.h` | `FManifest*Definition::ComputeHash()` |
| ComputeBlueprintOutputHash | `GasAbilityGeneratorGenerators.cpp` | `FGeneratorBase::ComputeBlueprintOutputHash()` |
| ComputeDataAssetOutputHash | `GasAbilityGeneratorGenerators.cpp` | `FGeneratorBase::ComputeDataAssetOutputHash()` |

---

## Contract 3 — CONFLICT Gating

| Component | File | Function/Class |
|-----------|------|----------------|
| EDryRunStatus enum | `GasAbilityGeneratorTypes.h:3985-3992` | `EDryRunStatus::Conflicted` |
| Conflict determination | `GasAbilityGeneratorGenerators.cpp:1227-1231` | In `CheckExistsWithMetadata()` |
| Force flag check | `GasAbilityGeneratorCommandlet.cpp` | `bForceRegeneration` flag |
| IsForceMode (definition) | `GasAbilityGeneratorGenerators.cpp:1089` | `FGeneratorBase::IsForceMode()` |
| IsForceMode (call sites) | `GasAbilityGeneratorGenerators.cpp:555,587,1281` | Force-bypass decision points |
| ComputeDryRunStatus | `GasAbilityGeneratorMetadata.cpp:432-479` | `GeneratorMetadataHelpers::ComputeDryRunStatus()` |

---

## Contract 4 — Dry-Run Persistence

| Component | File | Function/Class |
|-----------|------|----------------|
| IsDryRunMode check | `GasAbilityGeneratorGenerators.cpp` | `FGeneratorBase::IsDryRunMode()` |
| Dry-run guard | `GasAbilityGeneratorGenerators.cpp:1246-1259` | Returns before save in dry-run |
| Commandlet dry-run flag | `GasAbilityGeneratorCommandlet.cpp` | `-dryrun` parameter parsing |

**All SavePackage calls must be guarded by `!IsDryRunMode()`**

---

## Contract 5 — Disk-Truth Existence

| Component | File | Function/Class |
|-----------|------|----------------|
| DoesAssetExistOnDisk | `GasAbilityGeneratorGenerators.cpp:452-472` | `FGeneratorBase::DoesAssetExistOnDisk()` |
| Window disk check | `GasAbilityGeneratorWindow.cpp` | Uses `IFileManager::Get().FileExists()` |

**Contract:** Long package name format, uses `IFileManager`, NOT AssetRegistry.

---

## Contract 6 — Whitelist Gate

| Component | File | Function/Class |
|-----------|------|----------------|
| BuildAssetWhitelist | `GasAbilityGeneratorCommandlet.cpp` | `BuildAssetWhitelist()` |
| IsAssetInWhitelist | `GasAbilityGeneratorCommandlet.cpp` | `IsAssetInWhitelist()` |
| GetExpectedAssetNames | `GasAbilityGeneratorGenerators.cpp` | `FGeneratorBase::GetExpectedAssetNames()` |
| Verification pass | `GasAbilityGeneratorCommandlet.cpp` | Post-generation whitelist check |

---

## Contract 7 — 3-Way Merge

| Component | File | Function/Class |
|-----------|------|----------------|
| NPC sync engine | `XLSXSupport/NPCXLSXSyncEngine.cpp/h` | `FNPCXLSXSyncEngine` |
| Dialogue sync engine | `XLSXSupport/DialogueXLSXSyncEngine.cpp/h` | `FDialogueXLSXSyncEngine` |
| Quest sync engine | `XLSXSupport/QuestXLSXSyncEngine.cpp/h` | `FQuestXLSXSyncEngine` |
| Item sync engine | `XLSXSupport/ItemXLSXSyncEngine.cpp/h` | `FItemXLSXSyncEngine` |
| Sync status enum | Each engine's Types.h | `E*SyncStatus::Unchanged` (only auto-resolve case) |
| AutoResolveNonConflicts | Each engine | Only resolves `Unchanged` case |

---

## Contract 8 — Headless Safety

| Component | File | Function/Class |
|-----------|------|----------------|
| Headless detection | `GasAbilityGeneratorGenerators.cpp` | `-nullrhi` check |
| Policy B escape | `GasAbilityGeneratorGenerators.cpp:17236` | Niagara headless handling |
| HEADLESS-SAVED warning | `GasAbilityGeneratorGenerators.cpp` | Logged when saving under -nullrhi |
| RESULT_HEADLESS_SAVED | `GasAbilityGeneratorCommandlet.cpp` | Footer reports headless count |

---

## Contract 9 — Reporting Schema

| Component | File | Function/Class |
|-----------|------|----------------|
| FGenerationReportItem | `GasAbilityGeneratorReport.h` | Report item struct |
| FGenerationError | `GasAbilityGeneratorReport.h` | Error struct |
| UGenerationReport | `GasAbilityGeneratorReport.h` | Report DataAsset |
| RESULT footer | `GasAbilityGeneratorCommandlet.cpp` | `RESULT: New=X Skipped=Y Failed=Z...` |
| Exit codes | `GasAbilityGeneratorCommandlet.cpp` | `return 0` (success), `return 1` (failure) |
| JSON export | `GasAbilityGeneratorReport.cpp` | `SaveAsJSON()` |

---

## Refactor Guidelines

When moving/renaming files:

1. **Update this document** with new locations
2. **Verify contracts still hold** by checking:
   - Function signatures unchanged
   - Behavior identical
   - Tests (if any) still pass
3. **Cross-reference with LOCKED_CONTRACTS.md** for invariant definitions

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| v4.12.5 | 2026-01-18 | Initial creation mapping contracts to implementations |
| v4.12.5 | 2026-01-18 | Added IsForceMode() anchors to Contract 3 |
