# Phase 4.1 Implementation Plan - Pre-Validation System

**Version:** 1.1
**Date:** 2026-01-22
**Status:** APPROVED (Claude-GPT Audit Review Complete)

---

## Executive Summary

This document details the implementation steps for Phase 4.1 (Pre-Validation System) based on the locked specification in `Phase4_Spec_Locked.md`.

**Audit Status:** v1.1 incorporates 6 required changes from GPT audit review.

---

## Audit Changes (v1.1)

| Change | Description |
|--------|-------------|
| R1 | Renamed `FPreValidationError` → `FPreValidationIssue` |
| R2 | Added both `RuleId` (F1, F2) AND `ErrorCode` (E_PREVAL_*) fields |
| R3 | Asset validation uses AssetRegistry only (no TryLoad) |
| R4 | Tag severity policy explicitly encoded with locked comments |
| R5 | AttributeSet scope constrained: default + override only, no global scan |
| R6 | Clarified ErrorCode→Rules family mapping |

---

## Current Architecture Analysis

### Existing Infrastructure (Can Be Reused)

| Component | Location | Purpose |
|-----------|----------|---------|
| `EValidationSeverity` | GasAbilityGeneratorTypes.h:201 | Info/Warning/Error - **EXACT MATCH to spec** |
| `EValidationCategory` | GasAbilityGeneratorTypes.h:211 | Category grouping (Reference, Dependency, etc.) |
| `FGenerationResult` | GasAbilityGeneratorTypes.h:32 | Result tracking with Warnings array |
| `FGenerationSummary` | GasAbilityGeneratorTypes.h:131 | Aggregated results |
| `DetectCircularDependencies()` | GasAbilityGeneratorCommandlet.cpp:210 | Tarjan SCC - **Reusable for dependency graph** |
| `BuildDependencyGraph()` | GasAbilityGeneratorCommandlet.cpp:48 | Dependency edge building |

### Current Generation Flow

```
1. Parse manifest → FManifestData
2. v4.17: DetectCircularDependencies() ← Pre-validation point exists!
3. Generate tags
4. Generate assets (per-generator loops)
```

### Integration Point

**Insert pre-validation between steps 1 and 2:**

```
1. Parse manifest → FManifestData
2. [NEW] FPreValidator::Validate(ManifestData) → FPreValidationReport
3. [NEW] If report.HasBlockingErrors() → Return 1, skip generation
4. DetectCircularDependencies() (existing)
5. Generate tags
6. Generate assets
```

---

## New Components

### 1. FPreValidationIssue and FPreValidationReport (New Structs)

**Location:** `GasAbilityGeneratorTypes.h`

```cpp
// Phase 4.1: Pre-validation issue (R1: renamed from FPreValidationError)
// Represents a single validation issue - severity determines bucket
struct FPreValidationIssue
{
    // R2: Both RuleId AND ErrorCode for disambiguation
    FString RuleId;             // Spec rule: F1, F2, A1, A2, C1, C2, R1-R5, T1, T2, K1, K2
    FString ErrorCode;          // Error code: E_PREVAL_CLASS_NOT_FOUND, etc.
    EValidationSeverity Severity;
    FString Message;

    // Manifest location (for actionable output)
    FString ManifestPath;       // File path
    FString YAMLPath;           // e.g., "gameplay_abilities[0].event_graph.nodes[5]"
    FString ItemId;             // Asset/node ID: GA_FatherAttack, Node_5, etc.

    // Resolution context
    FString AttemptedClass;     // Class we tried to find
    FString AttemptedMember;    // Function/property/asset we tried to find

    // Format for logging: [ErrorCode] RuleId | ItemId | Message | YAMLPath
    FString ToString() const;
};

struct FPreValidationReport
{
    TArray<FPreValidationIssue> Errors;    // Severity::Error - blocks generation
    TArray<FPreValidationIssue> Warnings;  // Severity::Warning - generation proceeds
    TArray<FPreValidationIssue> Infos;     // Severity::Info - informational

    // Caching stats
    int32 TotalChecks = 0;
    int32 CacheHits = 0;

    bool HasBlockingErrors() const { return Errors.Num() > 0; }
    int32 GetErrorCount() const { return Errors.Num(); }
    int32 GetWarningCount() const { return Warnings.Num(); }

    void AddIssue(const FPreValidationIssue& Issue);  // Routes to correct array by severity
    FString GetSummary() const;
    void LogAll() const;
};
```

### 2. FPreValidationCache (New Class)

**Location:** `GasAbilityGeneratorPreValidator.h` (new file)

```cpp
// Caches reflection lookups per unique key
class FPreValidationCache
{
public:
    // Function existence: (ClassName, FunctionName) -> exists?
    bool CheckFunctionExists(const FString& ClassName, const FString& FunctionName, bool& bOutExists);
    void CacheFunctionResult(const FString& ClassName, const FString& FunctionName, bool bExists);

    // Attribute existence: (AttributeSetClass, PropertyName) -> exists?
    bool CheckAttributeExists(const FString& AttributeSetClass, const FString& PropertyName, bool& bOutExists);
    void CacheAttributeResult(const FString& AttributeSetClass, const FString& PropertyName, bool bExists);

    // Class existence: ClassName -> UClass*
    UClass* GetCachedClass(const FString& ClassName);
    void CacheClass(const FString& ClassName, UClass* Class);

    // R3: Asset existence via AssetRegistry only (no TryLoad)
    bool CheckAssetExists(const FString& AssetPath, bool& bOutExists);
    void CacheAssetResult(const FString& AssetPath, bool bExists);

    void Clear();
    int32 GetHitCount() const { return HitCount; }
    int32 GetMissCount() const { return MissCount; }

private:
    TMap<FString, bool> FunctionCache;      // "ClassName::FunctionName" -> exists
    TMap<FString, bool> AttributeCache;     // "SetClass::PropertyName" -> exists
    TMap<FString, UClass*> ClassCache;      // ClassName -> UClass*
    TMap<FString, bool> AssetCache;         // AssetPath -> exists (AssetRegistry)
    int32 HitCount = 0;
    int32 MissCount = 0;
};
```

### 3. FPreValidator (New Class)

**Location:** `GasAbilityGeneratorPreValidator.h/.cpp` (new files)

```cpp
class FPreValidator
{
public:
    // Main entry point
    static FPreValidationReport Validate(const FManifestData& Data, const FString& ManifestPath);

    // R5: AttributeSet configuration (default + override only, NO global scan)
    static FString GetDefaultAttributeSetClass();
    static void SetDefaultAttributeSetClass(const FString& ClassName);

private:
    // Rule implementations
    static void ValidateFunctions(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache);
    static void ValidateAttributes(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache);
    static void ValidateClasses(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache);
    static void ValidateAssetReferences(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache);
    static void ValidateTags(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache);
    static void ValidateTokens(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache);

    // Helpers
    static UClass* FindClassByName(const FString& ClassName, FPreValidationCache& Cache);
    static bool FunctionExistsOnClass(UClass* Class, const FString& FunctionName);
    static bool AttributeExistsOnSet(UClass* AttributeSetClass, const FString& AttributeName);
    static bool AssetExistsInRegistry(const FString& AssetPath);  // R3: AssetRegistry only
    static bool TagIsRegistered(const FString& TagName);

    // R5: Default is UNarrativeAttributeSetBase - NO global scan
    static FString DefaultAttributeSetClass;
};
```

---

## Implementation Steps

### Step 1: Create New Files

| File | Purpose |
|------|---------|
| `Public/GasAbilityGeneratorPreValidator.h` | FPreValidator, FPreValidationCache declarations |
| `Private/GasAbilityGeneratorPreValidator.cpp` | Implementation |

### Step 2: Add Types to GasAbilityGeneratorTypes.h

Add `FPreValidationIssue` and `FPreValidationReport` structs.

### Step 3: Implement FPreValidationCache

Simple caching layer with `TMap<FString, bool>` for each check type.

### Step 4: Implement Validation Rules

#### Rule F1/C1/C2: Class Resolution
```cpp
void FPreValidator::ValidateClasses(...)
{
    // For each manifest entry that references a class:
    // - gameplay_abilities[].parent_class
    // - actor_blueprints[].parent_class
    // - event_graph.nodes[].properties.class
    // etc.

    // Use FindObject<UClass> or LoadClass
    // If fails: Add issue with RuleId=C1/C2, ErrorCode=E_PREVAL_CLASS_NOT_FOUND
}
```

#### Rule F2: Function Existence
```cpp
void FPreValidator::ValidateFunctions(...)
{
    // For each CallFunction node in event graphs:
    // 1. Resolve class (using cached result)
    // 2. Call Class->FindFunctionByName(FunctionName)
    // If fails: Add issue with RuleId=F2, ErrorCode=E_PREVAL_FUNCTION_NOT_FOUND
}
```

#### Rule A1/A2: Attribute Resolution
```cpp
void FPreValidator::ValidateAttributes(...)
{
    // R5: AttributeSet resolution (LOCKED per Phase4_Spec_Locked.md)
    // Phase 4.1 validates ONLY:
    //   1. Manifest-specified attributeSetClass (if present)
    //   2. Else: configured default (UNarrativeAttributeSetBase)
    // NO global scan of UAttributeSet subclasses
    // NO ambiguity search

    // For each GE modifier that references an attribute:
    // 1. Get AttributeSetClass (from manifest or default)
    // 2. Load class - if fails: RuleId=A1, ErrorCode=E_PREVAL_ATTRIBUTESET_NOT_FOUND
    // 3. Find property with FGameplayAttributeData type
    // If fails: RuleId=A2, ErrorCode=E_PREVAL_ATTRIBUTE_NOT_FOUND
}
```

#### Rule R1-R5: Asset References
```cpp
void FPreValidator::ValidateAssetReferences(...)
{
    // R3: AssetRegistry only - NO TryLoad
    // Pre-validation must not load assets, only check existence
    // TryLoad deferred to future "deep validate" flag

    // For each SoftObjectPtr reference:
    // - animation_montages[].skeleton
    // - material_instances[].textures[]
    // - materials[].material_functions[]
    // etc.

    // Use IAssetRegistry::GetAssetByObjectPath()
    // If fails: Add issue with RuleId=R1-R5, ErrorCode=E_PREVAL_ASSET_NOT_FOUND
}

// R3: AssetRegistry-only implementation
static bool AssetExistsInRegistry(const FString& AssetPath)
{
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
    FAssetData AssetData = Registry.GetAssetByObjectPath(FSoftObjectPath(AssetPath));
    return AssetData.IsValid();
}
```

#### Rule T1/T2: Tag Validation
```cpp
void FPreValidator::ValidateTags(...)
{
    // R4: Tag severity policy (LOCKED per Phase4_Spec_Locked.md)
    // - T1 (normal tag not registered): WARNING - generation proceeds
    // - T2 (SetByCaller tag missing): ERROR - blocks generation
    // DO NOT CHANGE without spec amendment

    // For each tag reference:
    // Use UGameplayTagsManager::Get().RequestGameplayTag(Name, /*ErrorIfNotFound=*/false)

    if (!Tag.IsValid())
    {
        if (bIsSetByCallerTag)
        {
            // T2: SetByCaller MUST exist - ERROR
            Issue.RuleId = TEXT("T2");
            Issue.ErrorCode = TEXT("E_PREVAL_SETBYCALLER_NOT_FOUND");
            Issue.Severity = EValidationSeverity::Error;
        }
        else
        {
            // T1: Normal tag - WARNING only
            Issue.RuleId = TEXT("T1");
            Issue.ErrorCode = TEXT("E_PREVAL_TAG_NOT_REGISTERED");
            Issue.Severity = EValidationSeverity::Warning;
        }
    }
}
```

### Step 5: Integrate into Commandlet

**GasAbilityGeneratorCommandlet.cpp:Main() - After parsing:**

```cpp
// Parse manifest
if (!FGasAbilityGeneratorParser::ParseManifest(ManifestContent, ManifestData))
{
    // ... existing error handling
}

// [NEW] Phase 4.1: Pre-validation
LogMessage(TEXT("--- Pre-Validation ---"));
FPreValidationReport PreValReport = FPreValidator::Validate(ManifestData, ManifestPath);
PreValReport.LogAll();

if (PreValReport.HasBlockingErrors())
{
    LogError(FString::Printf(TEXT("[PRE-VALIDATION FAILED] %d errors, %d warnings"),
        PreValReport.GetErrorCount(), PreValReport.GetWarningCount()));
    LogError(TEXT("Fix manifest errors before generation can proceed."));
    return 1;  // Block generation
}

if (PreValReport.GetWarningCount() > 0)
{
    LogMessage(FString::Printf(TEXT("[PRE-VALIDATION] %d warnings (generation proceeding)"),
        PreValReport.GetWarningCount()));
}

// Continue with existing circular dependency check...
TSet<FString> AssetsInCycles = DetectCircularDependencies(ManifestData);
```

### Step 6: Integrate into Editor Window

**SGasAbilityGeneratorWindow.cpp - Before generation:**

Same pattern as commandlet - call `FPreValidator::Validate()` and check for blocking errors.

### Step 7: Update Build.cs

Add any needed module dependencies (should be minimal - using existing UE reflection).

---

## File Changes Summary

| File | Change Type | Description |
|------|-------------|-------------|
| `GasAbilityGeneratorTypes.h` | MODIFY | Add FPreValidationIssue, FPreValidationReport |
| `GasAbilityGeneratorPreValidator.h` | NEW | FPreValidator, FPreValidationCache declarations |
| `GasAbilityGeneratorPreValidator.cpp` | NEW | Implementation |
| `GasAbilityGeneratorCommandlet.cpp` | MODIFY | Add pre-validation call |
| `GasAbilityGeneratorWindow.cpp` | MODIFY | Add pre-validation call |
| `GasAbilityGenerator.Build.cs` | MODIFY | Add dependencies if needed |

---

## Error Code to Rule Family Mapping (R6)

One error code can cover multiple related rules. The `RuleId` field disambiguates which specific rule triggered within the family.

| ErrorCode | Rules Covered | Description |
|-----------|---------------|-------------|
| E_PREVAL_CLASS_NOT_FOUND | C1, C2, F1 | UClass doesn't exist |
| E_PREVAL_FUNCTION_NOT_FOUND | F2 | Function doesn't exist on class |
| E_PREVAL_ATTRIBUTESET_NOT_FOUND | A1 | AttributeSet class doesn't exist |
| E_PREVAL_ATTRIBUTE_NOT_FOUND | A2 | Attribute doesn't exist on set |
| E_PREVAL_ASSET_NOT_FOUND | R1, R2, R3, R4, R5 | Asset not in registry |
| E_PREVAL_TAG_NOT_REGISTERED | T1 | Tag not registered (WARNING) |
| E_PREVAL_SETBYCALLER_NOT_FOUND | T2 | SetByCaller tag missing (ERROR) |
| E_PREVAL_TOKEN_UNSUPPORTED | K1 | Token type not supported |
| E_PREVAL_TOKEN_PROPERTY_INVALID | K2 | Token property invalid |

**Log format:** `[ErrorCode] RuleId | ItemId | Message | YAMLPath`

**Example:**
```
[E_PREVAL_FUNCTION_NOT_FOUND] F2 | GA_FatherCrawler.SpawnSystemAttached | Function 'SpawnSystemAttached' not found on class 'NiagaraFunctionLibrary' | gameplay_abilities[0].event_graph.nodes[5]
```

---

## Validation Rules Mapping to Spec

| Rule | Spec Section | Implementation Function | ErrorCode |
|------|--------------|------------------------|-----------|
| F1 | Class resolves to UClass | `ValidateClasses()` | E_PREVAL_CLASS_NOT_FOUND |
| F2 | Function exists on UClass | `ValidateFunctions()` | E_PREVAL_FUNCTION_NOT_FOUND |
| A1 | AttributeSet class resolution | `ValidateAttributes()` | E_PREVAL_ATTRIBUTESET_NOT_FOUND |
| A2 | Property exists on AttributeSet | `ValidateAttributes()` | E_PREVAL_ATTRIBUTE_NOT_FOUND |
| C1 | All referenced UClass paths resolve | `ValidateClasses()` | E_PREVAL_CLASS_NOT_FOUND |
| C2 | Parent class exists for BP generation | `ValidateClasses()` | E_PREVAL_CLASS_NOT_FOUND |
| R1 | Skeleton assets exist | `ValidateAssetReferences()` | E_PREVAL_ASSET_NOT_FOUND |
| R2 | Texture assets exist | `ValidateAssetReferences()` | E_PREVAL_ASSET_NOT_FOUND |
| R3 | MaterialFunction assets exist | `ValidateAssetReferences()` | E_PREVAL_ASSET_NOT_FOUND |
| R4 | Referenced NPCDefinitions exist | `ValidateAssetReferences()` | E_PREVAL_ASSET_NOT_FOUND |
| R5 | SoftObjectPath references resolve | `ValidateAssetReferences()` | E_PREVAL_ASSET_NOT_FOUND |
| T1 | GameplayTags are registered | `ValidateTags()` | E_PREVAL_TAG_NOT_REGISTERED |
| T2 | SetByCaller tags exist | `ValidateTags()` | E_PREVAL_SETBYCALLER_NOT_FOUND |
| K1 | Token types are supported | `ValidateTokens()` | E_PREVAL_TOKEN_UNSUPPORTED |
| K2 | Token property references are valid | `ValidateTokens()` | E_PREVAL_TOKEN_PROPERTY_INVALID |

---

## Acceptance Criteria for Phase 4.1

Phase 4.1 is **DONE** when:

1. **Pre-validation produces correct issues for Phase 2 dataset:**
   - 55 function-not-found errors become pre-validation errors (no generation attempted)
   - 6 skeleton-not-found errors become pre-validation errors
   - All detected BEFORE generation starts

2. **Blocking policy works correctly:**
   - Errors block generation (return code 1)
   - Warnings do not block (generation proceeds)
   - Info is logged but does not affect flow

3. **Report format is actionable:**
   - Every issue includes: `[ErrorCode] RuleId | ItemId | Message | YAMLPath`
   - Caching stats reported (TotalChecks, CacheHits)

4. **Same dataset comparison:**
   - Run Phase 2 manifest through pre-validation
   - Pre-validation errors should match Phase 2 generation errors
   - But pre-validation catches them BEFORE any asset generation

5. **Performance acceptable:**
   - Pre-validation completes in < 5 seconds for current manifest
   - Cache hit ratio > 50% for repeated lookups

---

## Estimated Scope

| Component | Lines of Code (Estimate) |
|-----------|--------------------------|
| FPreValidationIssue/Report structs | ~100 |
| FPreValidationCache | ~120 |
| FPreValidator class | ~450 |
| Commandlet integration | ~30 |
| Window integration | ~30 |
| **Total** | **~730** |

---

## Locked Constraints (DO NOT CHANGE)

```cpp
// R4: Tag severity policy (LOCKED per Phase4_Spec_Locked.md)
// - T1 (normal tag not registered): WARNING - generation proceeds
// - T2 (SetByCaller tag missing): ERROR - blocks generation
// DO NOT CHANGE without spec amendment

// R5: AttributeSet resolution (LOCKED per Phase4_Spec_Locked.md)
// Phase 4.1 validates ONLY:
//   1. Manifest-specified attributeSetClass (if present)
//   2. Else: configured default (UNarrativeAttributeSetBase)
// NO global scan of UAttributeSet subclasses
// NO ambiguity search
// Ambiguity-fail logic deferred to future multi-set support

// R3: Asset validation (LOCKED per Phase4_Spec_Locked.md)
// Pre-validation uses AssetRegistry only - NO TryLoad
// TryLoad deferred to future "deep validate" flag
```

---

## Approval Checklist

- [x] Implementation plan reviewed (GPT audit)
- [x] R1-R6 changes incorporated
- [x] Scope acceptable
- [x] Error codes approved
- [x] Integration points confirmed
- [ ] **Erdem authorization to proceed**

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-22 | Claude | Initial draft |
| 1.1 | 2026-01-22 | Claude | Incorporated GPT audit R1-R6 changes, added Acceptance Criteria |
