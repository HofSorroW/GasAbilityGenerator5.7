# LOCKED_CONTRACTS.md (v4.16)

## Purpose

This document defines **Tier 1 (LOCKED)** contracts for GasAbilityGenerator.
These contracts are data-safety and CI-safety invariants. Refactors are allowed only if invariants remain true.

**Rule:** If a change touches a LOCKED contract or its implementation, it requires an explicit review session and a full regression pass.

---

## Definitions

### Decision Path
Any code path that determines: **create / skip / modify / conflict / fail** (including UI status logic that influences user actions).

### Persist Changes
Any of:
- `SavePackage` (or equivalent editor save APIs)
- `MarkPackageDirty`
- Asset creation/overwrite via factories/AssetTools
- Writing metadata to assets or registry that changes future behavior

### Generated Asset
An asset is "generated" iff generator metadata exists and indicates generated (`bIsGenerated=true`) from either:
- On-asset `UAssetUserData` (`UGeneratorAssetMetadata`)
- Central registry (`UGeneratorMetadataRegistry`)

---

## LOCKED CONTRACT 1 — Metadata Contract (Registry + AssetUserData)

### Invariant
- Metadata **read** must be registry-aware via:
  - `GetMetadataEx()` / `HasMetadataEx()` / `TryGetMetadata()`
- Metadata **write** must store to the correct source:
  - Asset supports `IInterface_AssetUserData` → write `UGeneratorAssetMetadata`
  - Otherwise → write `UGeneratorMetadataRegistry`

### Forbidden
- Using `GetMetadata()` or `GetAssetMetadata()` in decision paths
- Treating "no `UGeneratorAssetMetadata*`" as "manual" without checking registry

### Allowed
- Using `GetMetadata()` for purely informational UI (non-decision) or debugging

---

## LOCKED CONTRACT 2 — Regen/Diff Semantics (InputHash / OutputHash)

### Invariant
- `InputHash` represents the generator input definition state.
- `OutputHash` represents the generated asset's output state used to detect manual edits.
- A "conflict" is determined by the established rules (input changed vs output changed as implemented).

### Forbidden
- "Simplifying" by skipping fields in hash computations to reduce noise
- Changing what `InputHash`/`OutputHash` mean without versioned migration and explicit approval

---

## LOCKED CONTRACT 3 — CONFLICT Gating and Force Semantics

### Invariant
- `EDryRunStatus::Conflicted` (or equivalent) **blocks generation** unless `--force` is explicitly enabled.
- Force behavior must remain consistent with documented meaning.

### Forbidden
- Auto-resolving conflicts
- Converting conflict into warning without `--force`
- Silently proceeding with overwrite in conflict scenarios

---

## LOCKED CONTRACT 4 — Dry-Run Persistence Contract

### Invariant
In dry-run mode, the system **MUST NOT** persist changes:
- No `SavePackage`
- No `MarkPackageDirty`
- No asset creation/overwrite
- No metadata writes that affect future runs

### Allowed
- Loading assets
- Computing hashes
- Producing reports/logs
- Determining what would happen

---

## LOCKED CONTRACT 5 — Disk-Truth Asset Existence Contract

### Invariant
- Existence checks used for **create/skip decisions** must use disk-truth (`DoesAssetExistOnDisk`).
- Callers must pass **long package name**:
  - ✅ `/Game/Folder/AssetName`
  - ❌ `/Game/Folder/AssetName.AssetName`

### Forbidden
- Replacing with AssetRegistry existence checks for create/skip decisions
- Passing object paths where package paths are required

---

## LOCKED CONTRACT 6 — Manifest Whitelist Gate (Anti-Asset-Spam)

### Invariant
- Asset creation/modification must respect the manifest whitelist.
- Assets not in whitelist must not be generated (except explicitly documented exceptions).

### Forbidden
- Disabling whitelist checks to "make generation work"
- Creating assets outside the declared scope of the manifest

---

## LOCKED CONTRACT 7 — 3-Way Merge Contract (XLSX Sync)

### Invariant
- Conflicts in 3-way merge require **explicit user choice**.
- No silent auto-apply except the clearly-defined "unchanged" case.

### Forbidden
- Auto-applying non-trivial merge cases
- Updating `LastSyncedHash` without a successful, user-approved sync

---

## LOCKED CONTRACT 8 — Headless Safety (Policy Boundaries)

### Invariant
- Headless runs must correctly mark/save/report assets requiring editor verification (e.g., `bHeadlessSaved`).
- Any "escape hatch" policy (like Policy B) must produce warnings and be reported.

### Forbidden
- Treating headless save as clean success without reporting
- Suppressing verification requirements in headless mode

---

## LOCKED CONTRACT 9 — Reporting Schema (CI Interface)

### Invariant
- Report item fields and `RESULT` footer format are **stable**.
- Exit code meanings are stable:
  - `0` = success
  - non-zero = failure (as defined)

### Forbidden
- Changing `RESULT` footer keys/format without coordinated CI update
- Removing required report fields or altering meanings

---

## LOCKED CONTRACT 10 — Blueprint Compile Gate (v4.16 Graph Validation)

### Invariant
- Every Blueprint-generating generator must call `CompileBlueprint()` with `FCompilerResultsLog`
- If `FCompilerResultsLog.NumErrors > 0`, the asset **MUST NOT** be saved
- Each Blueprint generator may call `SavePackage` **at most once**, and only after passing validation gates

### Forbidden
- Saving a Blueprint without compile validation
- Checkpoint saves (multiple `SavePackage` calls per asset)
- Ignoring compile errors and proceeding to save
- Treating `CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE` as warning (must be hard fail)

### Allowed
- Multiple `CompileBlueprint()` calls if intermediate compiles are needed for CDO access
- Collecting all errors before failing (fail after enumeration, not on first error)

### Reference
- Audit: `ClaudeContext/Handoffs/Graph_Validation_Audit_v1.md`
- Implementation: `ClaudeContext/Handoffs/Graph_Validation_Implementation_v4.16.md`
- Decisions: D-006, D-008, D-009, D-010, D-011

---

## Enforcement

### Code Review Rule
Any change that touches a LOCKED implementation must:
1. Reference this document and the specific contract IDs
2. Include a regression checklist confirming invariants remain true

### Mechanical Lock (choose at least one)
- CI hash gate for this file + critical locked headers
- "Locked/ requires explicit approval" rule
- Minimal automation tests for Contracts 1, 4, 5, 9

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| v4.12.5 | 2026-01-18 | Initial creation after GPT validation session |
| v4.16 | 2026-01-21 | Added Contract 10 — Blueprint Compile Gate (GPT audit) |
