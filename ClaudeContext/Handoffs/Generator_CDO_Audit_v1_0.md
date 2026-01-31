# Generator CDO Property Audit - Complete Report v1.0

**Date:** 2026-01-31
**Scope:** Systematic audit of ALL generators against Narrative Pro / UE5 headers
**Method:** Read actual C++ headers, compare to generator code, document gaps

---

## Progress Summary

### ✅ COMPLETED (Build Verified)

| Fix | Description | Status |
|-----|-------------|--------|
| Schedule_.bReselect | Set bReselect on scheduled behaviors | ✅ Done |
| BPC_.bAutoActivate | Added auto_activate field for Component Blueprints | ✅ Done |
| **BPC_ Generator** | **NEW: Blueprint Condition generator (UNarrativeCondition)** | ✅ Done |

### 🔄 PENDING (Not Yet Implemented)

| Fix | Priority | Description |
|-----|----------|-------------|
| GA_ tags | **HIGH** | 7 missing properties (CostGE, Block/Source/Target tags, NetSecurityPolicy) |
| GE_ tags | **HIGH** | 4 missing properties (App/Removal/Ongoing tag requirements, GrantedAbilities) |
| NE_ Conditions | MEDIUM | Populate Conditions array |
| Quest_ InheritableStates | MEDIUM | Add InheritableStates field |

---

## Executive Summary

| Generator | Status | Gaps Found | Priority |
|-----------|--------|------------|----------|
| AC_ (Ability) | **COMPLETE** | 0 | - |
| AC_ (Activity) | **COMPLETE** | 0 | - |
| AM_ (Animation Montage) | SKIPPED | - | User request |
| BPA_ (Activity) | **COMPLETE** | 0 | - |
| **BPC_ (Blueprint Condition)** | **NEW GENERATOR** | N/A | Added v7.8.52 |
| BPC_ (Component Blueprint) | **FIXED** | 0 | bAutoActivate added |
| CD_ (Character Definition) | **FIXED** | 0 | - |
| DBP_ (Dialogue Blueprint) | **COMPLETE** | 0 | - |
| EI_ (Equippable Item) | **COMPLETE** | 0 | - |
| GA_ (Gameplay Ability) | **GAPS** | 7 | **HIGH** |
| GE_ (Gameplay Effect) | **GAPS** | 4 | **HIGH** |
| Goal_ (Goal Item) | **COMPLETE** | 0 | - |
| NE_ (Narrative Event) | **GAPS** | 1 | MEDIUM |
| NPC_ (NPC Definition) | **COMPLETE** | 0 | - |
| Quest_ | **GAPS** | 1 | MEDIUM |
| Schedule_ | **FIXED** | 0 | bReselect added |
| Tagged Dialogue Sets | **COMPLETE** | 0 | - |

---

## Completed Fixes Detail

### 1. Schedule_.bReselect (v7.8.52)
**File:** `GasAbilityGeneratorGenerators.cpp:28633`
```cpp
// v7.8.52: Set bReselect via reflection (triggers activity reselection when behavior starts)
if (FBoolProperty* ReselectProp = FindFProperty<FBoolProperty>(UScheduledBehavior_AddNPCGoal::StaticClass(), TEXT("bReselect")))
{
    ReselectProp->SetPropertyValue_InContainer(BehaviorInstance, Behavior.bReselect);
}
```

### 2. BPC_.bAutoActivate (v7.8.52)
**Files Modified:**
- `GasAbilityGeneratorTypes.h:2163` - Added field to struct
- `GasAbilityGeneratorParser.cpp:3737` - Added YAML parsing
- `GasAbilityGeneratorGenerators.cpp:6339` - Set on CDO

**YAML Syntax:**
```yaml
component_blueprints:
  - name: BPC_Example
    auto_activate: true
```

### 3. NEW: BPC_ Blueprint Condition Generator (v7.8.52)

**Purpose:** Generate UNarrativeCondition subclass Blueprints for dialogue/quest conditions

**Files Modified:**
- `GasAbilityGeneratorTypes.h:2206` - Added FManifestBlueprintConditionDefinition struct
- `GasAbilityGeneratorTypes.h:6610` - Added BlueprintConditions array to FManifestData
- `GasAbilityGeneratorParser.h:55` - Added ParseBlueprintConditions declaration
- `GasAbilityGeneratorParser.cpp:297` - Added section detection
- `GasAbilityGeneratorParser.cpp:4029` - Added full parsing function
- `GasAbilityGeneratorGenerators.h:459` - Added FBlueprintConditionGenerator class
- `GasAbilityGeneratorGenerators.cpp:6380` - Added full generator implementation
- `GasAbilityGeneratorCommandlet.cpp:1193` - Added generation loop
- `GasAbilityGeneratorCommandlet.cpp:2117` - Added retry handling

**YAML Syntax:**
```yaml
blueprint_conditions:
  - name: BPC_HasCustomTag
    folder: Conditions
    parent_class: NarrativeCondition  # or any existing BPC_ class
    not: false                        # Invert result
    condition_filter: AnyCharacter    # DontTarget, AnyCharacter, OnlyNPCs, OnlyPlayers
    party_policy: AnyPlayerPasses     # AnyPlayerPasses, PartyPasses, AllPlayersPass, PartyLeaderPasses

    # Target arrays (depends on filter)
    npc_targets:
      - NPC_Father

    # Custom properties (set via reflection)
    properties:
      GameplayTag: "Father.State.Alive"
      MinLevel: "5"

    # Variables for custom logic
    variables:
      - name: TagToCheck
        type: GameplayTag
        instance_editable: true
```

**Existing Narrative Pro BPC_ Classes (can be used as parent_class):**
- BPC_DifficultyCheck
- BPC_HasFollowGoalFor
- BPC_HasForm
- BPC_HasInteractGoalFor
- BPC_HasItem
- BPC_HasPerk
- BPC_HasWeaponOut
- BPC_IsItemEquipped
- BPC_IsOccupyingInteractable
- BPC_LevelCheck

---

## REMAINING: HIGH PRIORITY GAPS

### 1. GA_ (Gameplay Ability Generator) - 7 Properties Missing

**Source:** `NarrativeGameplayAbility.h`, `GameplayAbility.h`

| Property | Type | Purpose |
|----------|------|---------|
| `CostGameplayEffectClass` | TSubclassOf<UGameplayEffect> | Ability resource cost |
| `BlockAbilitiesWithTag` | FGameplayTagContainer | Tags that block this ability |
| `SourceBlockedTags` | FGameplayTagContainer | Source tags blocking activation |
| `SourceRequiredTags` | FGameplayTagContainer | Source tags required for activation |
| `TargetBlockedTags` | FGameplayTagContainer | Target tags blocking activation |
| `TargetRequiredTags` | FGameplayTagContainer | Target tags required for activation |
| `NetSecurityPolicy` | EGameplayAbilityNetSecurityPolicy | Network security policy |

**YAML Syntax (to be implemented):**
```yaml
gameplay_abilities:
  - name: GA_Example
    cost_gameplay_effect: GE_ManaCost
    block_abilities_with_tag:
      - Ability.Exclusive
    source_required_tags:
      - State.Alive
    source_blocked_tags:
      - State.Stunned
    target_required_tags:
      - State.Alive
    target_blocked_tags:
      - State.Invulnerable
    net_security_policy: ServerOnlyExecution
```

---

### 2. GE_ (Gameplay Effect Generator) - 4 Properties Missing

**Source:** `GameplayEffect.h`

| Property | Type | Purpose |
|----------|------|---------|
| `ApplicationTagRequirements` | FGameplayTagRequirements | Tags required/blocked for application |
| `RemovalTagRequirements` | FGameplayTagRequirements | Tags that trigger removal |
| `OngoingTagRequirements` | FGameplayTagRequirements | Tags required while active |
| `GrantedAbilities` | TArray<FGameplayAbilitySpecDef> | Abilities granted by this effect |

**YAML Syntax (to be implemented):**
```yaml
gameplay_effects:
  - name: GE_Example
    application_tag_requirements:
      require_tags:
        - State.Alive
      ignore_tags:
        - State.Immune
    removal_tag_requirements:
      require_tags:
        - State.Dead
    ongoing_tag_requirements:
      require_tags:
        - State.InCombat
    granted_abilities:
      - ability_class: GA_Dash
        level: 1
        input_policy: OnInputPressed
```

---

## REMAINING: MEDIUM PRIORITY GAPS

### 3. NE_ (Narrative Event Generator) - 1 Property Missing

| Property | Type | Purpose |
|----------|------|---------|
| `Conditions` | TArray<UNarrativeCondition*> | Conditions for event firing |

**Note:** Now that BPC_ generator exists, conditions can be generated and referenced.

---

### 4. Quest_ (Quest Generator) - 1 Property Missing

| Property | Type | Purpose |
|----------|------|---------|
| `InheritableStates` | TArray<UQuestState*> | States that can be inherited by child quests |

---

## Files to Modify (for remaining fixes)

| File | Changes |
|------|---------|
| `GasAbilityGeneratorTypes.h` | Add GA_ tag containers, GE_ tag requirements structs |
| `GasAbilityGeneratorParser.cpp` | Parse new YAML fields |
| `GasAbilityGeneratorGenerators.cpp` | Set CDO properties for all gaps |

---

## Implementation Order (Recommended)

1. ~~Schedule_.bReselect~~ ✅ Done
2. ~~BPC_.bAutoActivate~~ ✅ Done
3. ~~BPC_ Generator~~ ✅ Done
4. **GA_ tag containers** - High impact, common use case
5. **GE_ tag requirements** - High impact, effect conditions
6. **NE_ Conditions** - Medium complexity, now easier with BPC_ generator
7. **Quest_ InheritableStates** - Medium complexity
8. **GE_ GrantedAbilities** - Complex, requires FGameplayAbilitySpecDef population

---

## Verification Checklist

After implementation, verify each fix with:
1. `cycle` action builds successfully
2. Create test asset in manifest using new syntax
3. Generate and verify CDO properties in editor
4. Check no regressions in existing assets
