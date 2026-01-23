# PreValidator-Generator Function Resolution Parity Audit v1

**Date**: 2026-01-23
**Auditors**: Claude (Opus 4.5), GPT
**Status**: LOCKED - Implementation Authorized

---

## Executive Summary

PreValidator is more permissive than Generator for function resolution, creating "pass in validator, fail in generator" scenarios. This audit documents the finding, agreed solution, and implementation specification.

---

## Finding: Resolution Mismatch (PROVEN)

### Core Issue

| Component | Variant Behavior |
|-----------|------------------|
| **PreValidator** | Always tries Name, K2_Name, BP_Name on ANY class |
| **Generator** | Tries variants ONLY in WellKnownFunctions probe; all other paths use exact name |

### Evidence

**PreValidator** (`GasAbilityGeneratorPreValidator.cpp:346-373`):
```cpp
bool FPreValidator::FunctionExistsOnClass(UClass* Class, const FString& FunctionName)
{
    // Try exact name first
    UFunction* Function = Class->FindFunctionByName(*FunctionName);
    if (Function) return true;

    // Try with K2_ prefix
    FString K2Name = FString::Printf(TEXT("K2_%s"), *FunctionName);
    Function = Class->FindFunctionByName(*K2Name);
    if (Function) return true;

    // Try with BP_ prefix
    FString BPName = FString::Printf(TEXT("BP_%s"), *FunctionName);
    Function = Class->FindFunctionByName(*BPName);
    return Function != nullptr;
}
```

**Generator** (`GasAbilityGeneratorGenerators.cpp:10407-10495`):
- Lines 10407-10426: Variants tried in WellKnownFunctions lookup ONLY
- Lines 10430-10495: Explicit class, parent chain, library fallback all use exact FunctionName

### Concrete Risk Example

UE5 uses `ScriptName` meta to expose K2_/BP_ functions with Blueprint-visible names:

```cpp
// GameplayAbility.h:321-322
UFUNCTION(BlueprintCallable, DisplayName = "CancelAbility", meta=(ScriptName = "CancelAbility"))
void K2_CancelAbility();  // C++ name used by FindFunctionByName()
```

| Manifest Entry | PreValidator | Generator |
|----------------|--------------|-----------|
| `function: CancelAbility` | **PASS** (tries K2_CancelAbility) | **FAIL** (exact name only in fallback) |

### Affected Functions (Not in WellKnownFunctions)

**UGameplayAbility (14):**
1. CancelAbility → K2_CancelAbility
2. CheckAbilityCooldown → K2_CheckAbilityCooldown
3. CheckAbilityCost → K2_CheckAbilityCost
4. CommitExecute → K2_CommitExecute
5. ShouldAbilityRespondToEvent → K2_ShouldAbilityRespondToEvent
6. ActivateAbilityFromEvent → K2_ActivateAbilityFromEvent
7. EndAbilityLocally → K2_EndAbilityLocally
8. OnEndAbility → K2_OnEndAbility
9. RemoveGameplayEffectFromOwnerWithHandle → BP_RemoveGameplayEffectFromOwnerWithHandle
10. ExecuteGameplayCue → K2_ExecuteGameplayCue
11. ExecuteGameplayCueWithParams → K2_ExecuteGameplayCueWithParams
12. AddGameplayCue → K2_AddGameplayCue
13. AddGameplayCueWithParams → K2_AddGameplayCueWithParams
14. RemoveGameplayCue → K2_RemoveGameplayCue

**UAbilitySystemComponent (3):**
1. InitStats → K2_InitStats
2. GiveAbility → K2_GiveAbility
3. GiveAbilityAndActivateOnce → K2_GiveAbilityAndActivateOnce

### Current Manifest Status

Grep confirms none of these functions are currently used in manifest.yaml. Risk is latent but real for future manifest edits.

---

## Agreed Solution

### Contract (LOCKED)

> "PreValidator function resolution behavior must be identical to Generator function resolution behavior. Both use `FGasAbilityGeneratorFunctionResolver::ResolveFunction()` as single source of truth. No false positives (validator passes, generator fails) AND no false negatives (validator rejects, generator would succeed)."

### Architecture

```
┌─────────────────────────────────────────────────────────┐
│     FGasAbilityGeneratorFunctionResolver (NEW)          │
│  ┌─────────────────────────────────────────────────┐    │
│  │  static WellKnownFunctions (single table)       │    │
│  │  ResolveFunction() - 4-step cascade             │    │
│  │    1. WellKnown probe (variants)                │    │
│  │    2. Explicit class (exact)                    │    │
│  │    3. Parent chain (exact)                      │    │
│  │    4. Library fallback (exact, 13 classes)      │    │
│  └─────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────┘
              │                           │
              ▼                           ▼
┌─────────────────────────┐   ┌─────────────────────────┐
│  Generator              │   │  PreValidator           │
│  (uses ResolveFunction) │   │  (uses ResolveFunction) │
└─────────────────────────┘   └─────────────────────────┘
```

### Implementation Order

| Phase | Action | Purpose |
|-------|--------|---------|
| **Option 1** | Create `FGasAbilityGeneratorFunctionResolver` | Single source of truth, identical behavior contract |
| **Option 2** | Add 17 missing entries | UX improvement - allows Blueprint-visible names |

---

## API Specification

### FResolvedFunction Struct

```cpp
struct FResolvedFunction
{
    UFunction* Function = nullptr;
    UClass* OwnerClass = nullptr;
    bool bFound = false;
};
```

### ResolveFunction Signature

```cpp
static FResolvedFunction ResolveFunction(
    const FString& FunctionName,
    const FString& ExplicitClassName,  // From manifest class: field
    UClass* ParentClass,               // For parent chain walk
    bool bTargetSelf                   // Uses ability's parent chain
);
```

### Resolution Order (Must Match Generator Exactly)

1. **WellKnownFunctions probe** - Try variants (Name, K2_Name, BP_Name)
2. **Explicit class** - If class: specified, exact name only
3. **Parent chain** - If target_self or from parent, exact name only
4. **Library fallback** - 13 classes in order, exact name only:
   - UKismetSystemLibrary
   - UKismetMathLibrary
   - UGameplayStatics
   - UGameplayAbility
   - UAbilitySystemBlueprintLibrary
   - UAbilitySystemComponent
   - ACharacter
   - AActor
   - APawn
   - UCharacterMovementComponent
   - USceneComponent
   - USkeletalMeshComponent
   - UPrimitiveComponent

---

## Files to Create/Modify

| File | Action |
|------|--------|
| `Public/GasAbilityGeneratorFunctionResolver.h` | **NEW** |
| `Private/GasAbilityGeneratorFunctionResolver.cpp` | **NEW** |
| `Private/GasAbilityGeneratorGenerators.cpp` | **MODIFY** |
| `Private/GasAbilityGeneratorPreValidator.cpp` | **MODIFY** |

---

## 17-Entry Addition (Option 2)

```cpp
// UGameplayAbility (14)
WellKnownFunctions.Add(TEXT("K2_CancelAbility"), UGameplayAbility::StaticClass());
WellKnownFunctions.Add(TEXT("K2_CheckAbilityCooldown"), UGameplayAbility::StaticClass());
WellKnownFunctions.Add(TEXT("K2_CheckAbilityCost"), UGameplayAbility::StaticClass());
WellKnownFunctions.Add(TEXT("K2_CommitExecute"), UGameplayAbility::StaticClass());
WellKnownFunctions.Add(TEXT("K2_ShouldAbilityRespondToEvent"), UGameplayAbility::StaticClass());
WellKnownFunctions.Add(TEXT("K2_ActivateAbilityFromEvent"), UGameplayAbility::StaticClass());
WellKnownFunctions.Add(TEXT("K2_EndAbilityLocally"), UGameplayAbility::StaticClass());
WellKnownFunctions.Add(TEXT("K2_OnEndAbility"), UGameplayAbility::StaticClass());
WellKnownFunctions.Add(TEXT("BP_RemoveGameplayEffectFromOwnerWithHandle"), UGameplayAbility::StaticClass());
WellKnownFunctions.Add(TEXT("K2_ExecuteGameplayCue"), UGameplayAbility::StaticClass());
WellKnownFunctions.Add(TEXT("K2_ExecuteGameplayCueWithParams"), UGameplayAbility::StaticClass());
WellKnownFunctions.Add(TEXT("K2_AddGameplayCue"), UGameplayAbility::StaticClass());
WellKnownFunctions.Add(TEXT("K2_AddGameplayCueWithParams"), UGameplayAbility::StaticClass());
WellKnownFunctions.Add(TEXT("K2_RemoveGameplayCue"), UGameplayAbility::StaticClass());

// UAbilitySystemComponent (3)
WellKnownFunctions.Add(TEXT("K2_InitStats"), UAbilitySystemComponent::StaticClass());
WellKnownFunctions.Add(TEXT("K2_GiveAbility"), UAbilitySystemComponent::StaticClass());
WellKnownFunctions.Add(TEXT("K2_GiveAbilityAndActivateOnce"), UAbilitySystemComponent::StaticClass());
```

---

## Acceptance Test

1. Create manifest node: `function: CancelAbility` (not in WellKnownFunctions)
2. Run PreValidator → Expected: **FAIL**
3. Run Generator → Expected: **FAIL**
4. Add `K2_CancelAbility` to WellKnownFunctions (Option 2)
5. Run PreValidator → Expected: **PASS**
6. Run Generator → Expected: **PASS**

This proves "validator == generator" behavior.

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| v1 | 2026-01-23 | Initial audit - Claude–GPT dual audit consensus |
