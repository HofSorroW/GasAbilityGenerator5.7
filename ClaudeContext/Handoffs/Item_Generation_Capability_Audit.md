# Item Generation Capability Audit (v1.0)

**Audit Type:** Claude-GPT Dual Audit
**Date:** 2026-01-23
**Status:** LOCKED - GO-READY
**Participants:** Claude (Opus 4.5), GPT (via Erdem relay)

---

## Executive Summary

This document captures the locked decisions from a comprehensive dual-agent audit on extending GasAbilityGenerator's item generation capabilities to support:

1. **Non-equippable item types** (GameplayEffectItem, AmmoItem, WeaponAttachmentItem)
2. **Item Fragments** (instanced subobjects on UNarrativeItem)
3. **Enhanced validation** (demo-dependency traversal, property applicability)

All decisions are consensus-locked between Claude and GPT. No implementation may deviate from these specifications without explicit re-audit.

---

## Table of Contents

1. [Architecture Decisions](#1-architecture-decisions)
2. [Fragment System Specification](#2-fragment-system-specification)
3. [Validation Rules](#3-validation-rules)
4. [Error Code Registry](#4-error-code-registry)
5. [Parser Schema](#5-parser-schema)
6. [Implementation Constraints](#6-implementation-constraints)
7. [Audit Trail](#7-audit-trail)

---

## 1. Architecture Decisions

### 1.1 Generator Architecture (LC-F4)

**Decision:** C3 Frontend + C1 Backend (Hybrid)

| Layer | Approach | Rationale |
|-------|----------|-----------|
| **Manifest (Frontend)** | Separate sections per item branch | Semantic clarity, prevents operator error |
| **Generator (Backend)** | Single generator with parent class branching | Code reuse, existing reflection pattern |
| **Definition Struct** | Superset `FManifestEquippableItemDefinition` | Backwards compatible, minimal refactor |

**Manifest Sections (Option C):**

```yaml
equippable_items:      # UEquippableItem branch (weapons, armor, clothing)
consumable_items:      # UGameplayEffectItem branch
ammo_items:            # UAmmoItem branch
weapon_attachments:    # UWeaponAttachmentItem branch
```

**Evidence:** Existing `FEquippableItemGenerator` (line 15950) already uses parent class branching via `FindParentClass()` and reflection-based property setting. Adding new sections reuses this pattern.

### 1.2 Item Class Hierarchy (Verified)

```
UNarrativeItem (base)
├── UEquippableItem
│   ├── UWeaponItem
│   │   ├── URangedWeaponItem
│   │   ├── UMeleeWeaponItem
│   │   └── UMagicWeaponItem
│   ├── UEquippableItem_Clothing
│   └── UThrowableWeaponItem
├── UGameplayEffectItem        ← NOT UEquippableItem
├── UAmmoItem                  ← NOT UEquippableItem
└── UWeaponAttachmentItem      ← NOT UEquippableItem
```

**Key Insight:** GameplayEffectItem, AmmoItem, and WeaponAttachmentItem inherit directly from UNarrativeItem, NOT UEquippableItem. This is why separate manifest sections are semantically correct.

---

## 2. Fragment System Specification

### 2.1 Fragment Overview

**Definition:** Fragments are instanced subobjects (`UNarrativeItemFragment`) attached to items via the `Fragments` array property. They provide modular capabilities (ammo behavior, poisonability, etc.).

**NarrativePro Fragment Classes (v1 Allowlist):**

| Class | Properties | Settable from Manifest |
|-------|------------|------------------------|
| `UAmmoFragment` | `AmmoDamageOverride` (float), `DamageEffect` (TSubclassOf), `ProjectileClass` (TSubclassOf), `bOverrideTraceData` (bool), `TraceData` (FCombatTraceData) | All 5 |
| `UPoisonableFragment` | `AppliedPoison` (TSubclassOf) | **None** (SaveGame flag) |

### 2.2 Fragment Creation Pattern (Proven)

**Source:** Existing TriggerSet generator (`GasAbilityGeneratorGenerators.cpp:20327`)

```cpp
UNarrativeItemFragment* Fragment = NewObject<UNarrativeItemFragment>(
    ItemAsset,                    // Outer = parent item
    FragmentClass,                // UClass* to instantiate
    NAME_None,                    // No specific name
    RF_Public | RF_Transactional  // Instanced subobject flags
);
```

**Property Setting (Reflection):**

```cpp
if (FFloatProperty* Prop = FindFProperty<FFloatProperty>(FragmentClass, TEXT("AmmoDamageOverride")))
{
    Prop->SetPropertyValue_InContainer(Fragment, Value);
}
```

### 2.3 Fragment Class Resolution (LC-F5, Hygiene-2)

**Precedence Rule:**

1. If input starts with `/Script/` or `/Game/` → resolve as-is (full path)
2. Otherwise → treat as short name, map via allowlist

**Allowlist (v1):**

```cpp
static TMap<FString, FString> FragmentAllowlist = {
    {TEXT("AmmoFragment"), TEXT("/Script/NarrativeArsenal.AmmoFragment")},
    {TEXT("PoisonableFragment"), TEXT("/Script/NarrativeArsenal.PoisonableFragment")}
};
```

**Evidence:** NarrativeArsenal classes consistently use `/Script/NarrativeArsenal.<NameWithoutUPrefix>` pattern (verified via existing plugin code: NPCDefinition, Dialogue, NarrativeItem, etc.).

**Contract:** Allowlist paths must be validated via `LoadClass<UNarrativeItemFragment>` at plugin startup. If resolution fails, it's an allowlist bug (emit `E_FRAGMENT_CLASS_NOT_FOUND`).

### 2.4 Fragment Property Handling

**Property Flag Checks:**

```cpp
// Skip non-settable properties
if (Property->HasAnyPropertyFlags(CPF_Transient | CPF_EditConst | CPF_DisableEditOnInstance))
{
    // E_FRAGMENT_PROPERTY_NOT_SETTABLE (if manifest tries to set)
}

// SaveGame properties are runtime-only
if (Property->HasAnyPropertyFlags(CPF_SaveGame))
{
    // Cannot set from manifest (e.g., PoisonableFragment.AppliedPoison)
}
```

**Abstract Class Guard:**

```cpp
if (FragmentClass->HasAnyClassFlags(CLASS_Abstract))
{
    // E_FRAGMENT_CLASS_ABSTRACT
}
```

### 2.5 Nested Fragment References (LC-F6)

**Status:** CLOSED - Not applicable

**Verification:** Grep for `TObjectPtr<UNarrativeItemFragment>` returned zero results. Neither AmmoFragment nor PoisonableFragment reference other fragments. No topological sort or forward reference resolution needed.

### 2.6 Empty Fragment Handling (LC-F5)

**Decision:** Allow with WARNING

**Rationale:** PoisonableFragment has zero manifest-settable properties (AppliedPoison is SaveGame), but the fragment's value is in capability marking ("this item can be poisoned").

**Behavior:**

- Create the fragment instance
- Emit `W_FRAGMENT_NO_SETTABLE_PROPERTIES` (deduped per session)
- Succeed

**Warning Format (per-session, with count):**

```
W_FRAGMENT_NO_SETTABLE_PROPERTIES | PoisonableFragment | All properties SaveGame/not settable | Used by 12 items
```

---

## 3. Validation Rules

### 3.1 Property Applicability (LC-F4)

**Decision:** Reflection-based validation on resolved ParentClass

**Rule:** If manifest provides a non-sentinel value for field X, and `FindPropertyByName(X)` on ParentClass returns nullptr → `E_ITEM_PROPERTY_NOT_APPLICABLE`

**Implementation:**

```cpp
// After ParentClass is resolved
if (!Definition.WeaponVisualClass.IsEmpty())
{
    if (!ParentClass->FindPropertyByName(TEXT("WeaponVisualClass")))
    {
        // E_ITEM_PROPERTY_NOT_APPLICABLE
        return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
            FString::Printf(TEXT("Property 'WeaponVisualClass' not applicable to parent class '%s'"),
            *ParentClass->GetName()));
    }
}
```

### 3.2 Sentinel Semantics (D3)

**Decision:** Accept current behavior (zero = unset) as locked v1 rule

**Rationale:**
1. Changing to `TOptional<T>` requires touching 55+ fields
2. Zero attack rating, zero base value are edge cases
3. Backwards compatibility with existing manifests

**Sentinel Values:**

| Type | Sentinel (Unset) | Note |
|------|------------------|------|
| `FString` | `""` | Empty string |
| `float` | `0.0f` | Cannot express "explicitly set to zero" |
| `int32` | `0` | Cannot express "explicitly set to zero" |
| `bool` | `false` | Only for fields using `if (bField)` pattern |
| `TArray` | `[]` | Empty array |

**v4.28 Exception - Negative Sentinel for FOV Overrides:**

The following WeaponAttachmentItem float properties use `-1.0f` as sentinel instead of `0.0f`:

| Property | Default | Rationale |
|----------|---------|-----------|
| `FOVOverride` | `-1.0f` | Zero FOV is invalid, but explicit small values (e.g., 0.5) are valid |
| `WeaponRenderFOVOverride` | `-1.0f` | Same reasoning - allows explicit zero or small values |
| `WeaponAimFStopOverride` | `-1.0f` | FStop of 0.0 may be valid for max bokeh effect |

**Generator Check Pattern:**
```cpp
if (Definition.FOVOverride >= 0.0f)  // -1 = unset, 0+ = set
{
    // Apply property
}
```

This is an intentional improvement over the zero-sentinel rule: negative values are physically invalid for FOV/FStop, making `-1` an unambiguous "unset" marker while preserving the ability to explicitly set zero or near-zero values.

**Documentation Note:**

> **N_ITEM_NUMERIC_SENTINEL_LIMITATION (v1):** Zero values are treated as unset for numeric properties controlled by sentinel checks. Use presets or omit field. If explicit zero becomes required, schema v2 will introduce optionals. **Exception:** FOV override properties use -1.0f sentinel.

**Applicability Interaction:** If a field is "unset" by sentinel rules, it doesn't participate in applicability validation.

### 3.3 Demo Dependency Validation (LC-F7)

**Decision:** Explicit fragment reference walk required for PRE-VALIDATION

**Scope Split:**

| Phase | Method | Reason |
|-------|--------|--------|
| **Pre-validation** | Explicit manifest walk | Fragments don't exist yet; AssetRegistry can't see them |
| **Post-save** | AssetRegistry dependencies | Fragments serialized into package; deps captured |

**Pre-validation Rule:**

`I_ITEM_NO_DEMO_DEPENDENCIES` MUST validate:
1. Item asset package dependencies (AssetRegistry)
2. Fragment definitions in manifest (explicit walk)

**Fragment Properties to Scan:**

| Property | Type | Check |
|----------|------|-------|
| `DamageEffect` | TSubclassOf | Must not reference demo GE |
| `ProjectileClass` | TSubclassOf | Must not reference demo projectile |

**Sub-rule:**

> **I_ITEM_NO_DEMO_DEPENDENCIES.FRAGMENTS:** Applies to every fragment definition. Checks TSubclassOf, TSoftClassPtr, TSoftObjectPtr references against forbidden demo patterns. Failure code: `E_ITEM_DEMO_DEPENDENCY_DETECTED` with `Source=Fragment`, `FragmentClass=...`.

---

## 4. Error Code Registry

### 4.1 Fragment-Specific Error Codes

| Code | Severity | Trigger | Payload Fields |
|------|----------|---------|----------------|
| `E_FRAGMENT_CLASS_NOT_FOUND` | FAIL | Fragment class doesn't resolve | ItemName, FragmentClass |
| `E_FRAGMENT_CLASS_ABSTRACT` | FAIL | Fragment class has CLASS_Abstract | ItemName, FragmentClass |
| `E_FRAGMENT_INSTANTIATION_FAILED` | FAIL | NewObject returns nullptr | ItemName, FragmentClass |
| `E_ITEM_PROPERTY_NOT_APPLICABLE` | FAIL | Property/struct member not found on fragment | ItemName, FragmentClass, PropertyPath |
| `W_FRAGMENT_NOT_IN_ALLOWLIST` | WARN | Fragment class not in hardcoded allowlist | FragmentClass |
| `W_FRAGMENT_NO_SETTABLE_PROPERTIES` | WARN | Fragment has zero settable props | FragmentClass, UsageCount |
| `W_FRAGMENT_PROPERTY_TYPE_UNSUPPORTED` | WARN | Property type not handled by reflection | ItemName, FragmentClass, PropertyName |

**Implementation Note:** `E_FRAGMENT_PROPERTY_TYPE_MISMATCH` and `E_FRAGMENT_STRUCT_MEMBER_NOT_FOUND` from initial audit are consolidated into `E_ITEM_PROPERTY_NOT_APPLICABLE` for consistency with existing item property errors.

### 4.2 Item-Specific Error Codes

| Code | Severity | Trigger | Payload Fields |
|------|----------|---------|----------------|
| `E_ITEM_PROPERTY_NOT_APPLICABLE` | FAIL | Field set on incompatible parent_class | ItemName, ParentClass, PropertyName |
| `E_ITEM_PARENT_NOT_FOUND` | FAIL | Parent class doesn't resolve | ItemName, ParentClass |
| `E_ITEM_DEMO_DEPENDENCY_DETECTED` | FAIL | Demo asset referenced | ItemName, Source, DependencyPath |
| `E_FRAGMENTS_NOT_FOUND` | FAIL | Fragments array not found on item class | ItemName, ClassPath |
| `E_GE_CLASS_NOT_FOUND` | FAIL | GameplayEffectClass doesn't resolve | ItemName, GameplayEffectClass |

### 4.3 Error Payload Standardization (Hygiene-1)

**All fragment/item errors include:**

| Field | Description |
|-------|-------------|
| `ItemName` | Name of the item being generated |
| `ParentClass` | Resolved parent class (if applicable) |
| `FragmentClass` | Fragment class name (if applicable) |
| `PropertyPath` | Property path including dot notation (if applicable) |

**Format (pipe-delimited):**

```
E_FRAGMENT_CLASS_NOT_FOUND | EI_PoisonArrow | AmmoItem | UnknownFragment | Fragment class not found
E_ITEM_PROPERTY_NOT_APPLICABLE | EI_LaserRifle | GameplayEffectItem | WeaponVisualClass | Property not applicable to parent class
```

---

## 5. Parser Schema

### 5.1 Fragment Definition Schema (LC-F7)

```yaml
fragments:
  - class: AmmoFragment                    # Required: short name or full path
    properties:                            # Optional: property map
      AmmoDamageOverride: 25.0
      DamageEffect: GE_PoisonDamage
      ProjectileClass: BP_PoisonArrow
      bOverrideTraceData: true
      TraceData.TraceDistance: 5000.0      # Dot notation for struct members
      TraceData.TraceRadius: 5.0
      TraceData.bTraceMulti: false

  - class: PoisonableFragment
    # properties omitted (all SaveGame, not settable)
```

### 5.2 Struct Property Dot Notation (S2)

**Decision:** Flattened dot notation for struct members

**Contract:**

> **C_ITEM_FRAGMENT_PROPERTY_PATHS (v1):** Fragment property keys may be:
> - Direct property names: `AmmoDamageOverride`
> - Struct member paths: `TraceData.TraceDistance`
>
> Dot notation is limited to one struct hop in v1.

**Parsing Rule:**

1. Split key at first `.`
2. Left = top-level UPROPERTY on fragment
3. Right = member name within struct
4. If left is not a struct property → `E_FRAGMENT_PROPERTY_TYPE_MISMATCH`
5. If struct member not found → `E_FRAGMENT_STRUCT_MEMBER_NOT_FOUND`

**Rationale:** Avoids deep parser nesting (7 levels with full YAML nesting vs 4 with dot notation). Current parser handles ~3-4 levels using state machine flags.

### 5.3 Full Item Definition Example

```yaml
equippable_items:
  - name: EI_PoisonArrow
    parent_class: AmmoItem
    folder: Items/Ammo
    display_name: "Poison Arrow"
    description: "An arrow tipped with deadly venom."
    weight: 0.1
    base_value: 5
    stackable: true
    max_stack_size: 99
    item_tags:
      - Item.Ammo.Arrow
      - Item.Poison
    fragments:
      - class: AmmoFragment
        properties:
          AmmoDamageOverride: 15.0
          DamageEffect: GE_PoisonDamage
          bOverrideTraceData: false
      - class: PoisonableFragment
        # No properties (capability marker only)
```

---

## 6. Implementation Constraints

### 6.1 Backwards Compatibility

- Existing `equippable_items:` section continues to work unchanged
- New sections (`consumable_items`, `ammo_items`, `weapon_attachments`) are additive
- Sentinel semantics (zero = unset) preserved

### 6.2 Fragment Allowlist Policy

- v1 hardcodes 2 fragment classes (AmmoFragment, PoisonableFragment)
- Adding new fragments requires plugin update
- Custom Blueprint fragments supported via full path (`/Game/...`)

### 6.3 Future Considerations (Not v1)

| Feature | Deferred Reason |
|---------|-----------------|
| `TOptional<T>` for explicit zero | 55+ field refactor, low priority |
| Nested YAML for struct properties | Parser complexity, dot notation sufficient |
| Multi-hop dot notation (`A.B.C`) | No current use case |

---

## 7. Audit Trail

### 7.1 Challenge Resolution Log

| ID | Challenge | Challenger | Resolution |
|----|-----------|------------|------------|
| LC-F4 | Generator count ambiguity | Claude | Hybrid C3+C1 accepted |
| LC-F5 | Fragment class discovery | Claude | Hardcoded allowlist (2 classes) |
| LC-F6 | Nested fragment references | Claude | CLOSED - not applicable |
| LC-F7 | Demo validation traversal | Claude | Pre-validation explicit walk |
| D1/D2/D3 | Sentinel semantics | Claude | D3 accepted (zero = unset) |
| S1/S2 | Struct property schema | Claude | S2 accepted (dot notation) |
| Hygiene-1 | Error payload fields | GPT | Accepted |
| Hygiene-2 | Fragment class identifier | GPT | Both short name and full path |

### 7.2 Verification Evidence

| Claim | Verification Method | Result |
|-------|---------------------|--------|
| Fragment creation pattern proven | Grep TriggerSet generator | Line 20327 confirms pattern |
| Only 2 fragment classes exist | Glob `**/Fragments/*.h` | AmmoFragment, PoisonableFragment |
| No nested fragment refs | Grep `TObjectPtr<UNarrativeItemFragment>` | Zero results |
| FCombatTraceData is flat | Read struct definition | 3 scalar members, no nesting |
| Class paths use no U prefix | Grep `/Script/NarrativeArsenal.` | Consistent pattern confirmed |

### 7.3 Files Referenced

| File | Purpose |
|------|---------|
| `NarrativeItem.h` | Base item class, Fragments array |
| `EquippableItem.h` | UEquippableItem properties |
| `AmmoFragment.h` | Fragment properties (5 settable) |
| `PoisonableFragment.h` | Fragment properties (0 settable) |
| `GasAbilityGeneratorGenerators.cpp:15950` | Existing FEquippableItemGenerator |
| `GasAbilityGeneratorGenerators.cpp:20327` | Instanced subobject pattern |
| `GasAbilityGeneratorParser.cpp:7071` | TraceData nested parsing |

---

## Appendix A: Locked Contracts Summary

```
LC-F4: Applicability check = reflection on resolved ParentClass; no static map.
LC-F5: W_FRAGMENT_NO_SETTABLE_PROPERTIES dedup per-session with counts.
LC-F6: CLOSED - No nested fragment references exist.
LC-F7: Explicit fragment reference walk is pre-validation only; post-save uses AssetRegistry.
D3: Sentinel semantics (zero = unset) locked for v1. EXCEPTION: FOVOverride fields use -1.0f.
S2: Dot notation for fragment struct properties (one-hop limit).
Hygiene-1: Error payloads include ItemName, ParentClass, FragmentClass, PropertyPath.
Hygiene-2: Fragment class accepts both short name and full path; full path precedence.
N_ITEM_NUMERIC_SENTINEL_LIMITATION: Documented limitation for v1 (FOVOverride exception documented).
C_ITEM_FRAGMENT_PROPERTY_PATHS: Dot notation contract for v1.
I_ITEM_NO_DEMO_DEPENDENCIES.FRAGMENTS: Fragment traversal sub-rule.
```

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| v1.1 | 2026-01-23 | Consistency sync: Added E_FRAGMENT_CLASS_ABSTRACT guard, consolidated error codes (E_ITEM_PROPERTY_NOT_APPLICABLE), documented FOVOverride sentinel exception, added missing error/warning codes to registry |
| v1.0 | 2026-01-23 | Initial audit complete - GO-READY |
