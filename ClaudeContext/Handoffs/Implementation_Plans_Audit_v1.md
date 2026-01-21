# Implementation Plans - Audit Document v1.0

**Created:** 2026-01-21
**Purpose:** Detailed implementation plans for remaining TODO items, ready for GPT audit
**Status:** Section 3 IMPLEMENTED (v4.17) - Remaining sections awaiting audit

---

## Table of Contents

1. [P1.2 Transition Validation](#1-p12-transition-validation)
2. [P1.3 Startup Validation](#2-p13-startup-validation)
3. [Circular Dependency Detection](#3-circular-dependency-detection)
4. [FormState Preset Schema](#4-formstate-preset-schema)
5. [Material Circular Reference Detection](#5-material-circular-reference-detection)
6. [Delegate Binding Automation](#6-delegate-binding-automation)
7. [Split Generators.cpp](#7-split-generatorscpp)

---

## 1. P1.2 Transition Validation

### Overview
Validates that form transition state machine flows are valid before generation.

### Problem Statement
Currently, the manifest can define invalid form transitions (e.g., Crawler→Symbiote when Symbiote requires Attached state but Crawler doesn't grant it). These errors only surface at runtime.

### Scope
- **Files to modify:** `GasAbilityGeneratorParser.cpp`
- **Files to add:** None
- **Complexity:** LOW
- **Risk:** NONE - Additive validation only
- **Rare edge case:** No - affects every form change definition

### Implementation Steps

#### Step 1: Define Transition Rules
```cpp
// In GasAbilityGeneratorParser.cpp (static helper)
struct FFormTransitionRule
{
    FString FromForm;
    FString ToForm;
    TArray<FString> RequiredTags;      // Tags that must exist on source
    TArray<FString> BlockedTags;       // Tags that block this transition
};

static TArray<FFormTransitionRule> GetFormTransitionRules()
{
    return {
        // Crawler can transition to any form (base form)
        {TEXT("Crawler"), TEXT("Armor"), {}, {}},
        {TEXT("Crawler"), TEXT("Exoskeleton"), {}, {}},
        {TEXT("Crawler"), TEXT("Symbiote"), {}, {}},
        {TEXT("Crawler"), TEXT("Engineer"), {}, {}},
        // Attached forms can only return to Crawler or switch to other attached
        {TEXT("Armor"), TEXT("Crawler"), {TEXT("Father.State.Attached")}, {}},
        {TEXT("Exoskeleton"), TEXT("Crawler"), {TEXT("Father.State.Attached")}, {}},
        {TEXT("Symbiote"), TEXT("Crawler"), {}, {TEXT("Father.State.SymbioteLocked")}},
        // etc.
    };
}
```

#### Step 2: Add Validation Function
```cpp
// In GasAbilityGeneratorParser.cpp
static void ValidateFormTransitions(const FManifestData& Data)
{
    // Build form ability map
    TMap<FString, const FManifestGameplayAbilityDefinition*> FormAbilities;
    for (const auto& GA : Data.GameplayAbilities)
    {
        if (GA.Name.StartsWith(TEXT("GA_Father")) &&
            (GA.Name.Contains(TEXT("Crawler")) || GA.Name.Contains(TEXT("Armor")) ||
             GA.Name.Contains(TEXT("Exoskeleton")) || GA.Name.Contains(TEXT("Symbiote")) ||
             GA.Name.Contains(TEXT("Engineer"))))
        {
            FormAbilities.Add(GA.Name, &GA);
        }
    }

    // Validate cancel_abilities_with_tag creates valid transitions
    for (const auto& Pair : FormAbilities)
    {
        const auto* GA = Pair.Value;
        for (const FString& CancelTag : GA->Tags.CancelAbilitiesWithTag)
        {
            // Check if cancelled ability's required tags are satisfiable
            // after this ability grants its activation_owned_tags
            // ...
        }
    }
}
```

#### Step 3: Call at Parse End
```cpp
// In ParseManifest(), before return true:
ValidateFormTransitions(OutData);
```

### Output Format
```
[W_TRANSITION_INVALID] GA_FatherSymbiote cancels GA_FatherCrawler but doesn't satisfy required tag 'Father.State.Alive'
```

### Test Cases
1. Valid: Crawler→Armor (no special requirements)
2. Invalid: Direct Symbiote→Engineer (SymbioteLocked blocks)
3. Edge: Circular cancellation (A cancels B, B cancels A)

### Acceptance Criteria
- [ ] All form abilities validated at parse time
- [ ] Warning logged for invalid transitions
- [ ] No false positives on valid manifest
- [ ] Machine-parseable warning format

---

## 2. P1.3 Startup Validation

### Overview
Validates that ability startup requirements (tags, cooldowns, resources) are satisfiable before generation.

### Problem Statement
Abilities can define `activation_required_tags` that are never granted by any GE or other ability, making them permanently unusable. This is only discovered at runtime.

### Scope
- **Files to modify:** `GasAbilityGeneratorParser.cpp`
- **Files to add:** None
- **Complexity:** MEDIUM
- **Risk:** NONE - Additive validation only
- **Rare edge case:** No - runs on every ability definition

### Implementation Steps

#### Step 1: Build Tag Grant Map
```cpp
// Collect all tags that can be granted
static TSet<FString> BuildGrantableTagSet(const FManifestData& Data)
{
    TSet<FString> GrantableTags;

    // From GameplayEffects
    for (const auto& GE : Data.GameplayEffects)
    {
        for (const FString& Tag : GE.GrantedTags)
        {
            GrantableTags.Add(Tag);
        }
    }

    // From Abilities (activation_owned_tags)
    for (const auto& GA : Data.GameplayAbilities)
    {
        for (const FString& Tag : GA.Tags.ActivationOwnedTags)
        {
            GrantableTags.Add(Tag);
        }
    }

    // From Equipment (granted via equip)
    for (const auto& EI : Data.EquippableItems)
    {
        // Equipment modifier GEs grant tags
        // ...
    }

    return GrantableTags;
}
```

#### Step 2: Validate Required Tags
```cpp
static void ValidateAbilityStartupRequirements(const FManifestData& Data)
{
    TSet<FString> GrantableTags = BuildGrantableTagSet(Data);

    for (const auto& GA : Data.GameplayAbilities)
    {
        for (const FString& RequiredTag : GA.Tags.ActivationRequiredTags)
        {
            if (!GrantableTags.Contains(RequiredTag))
            {
                // Check if it's a system tag (State.*, etc.)
                if (!IsSystemTag(RequiredTag))
                {
                    UE_LOG(LogGasAbilityParser, Warning,
                        TEXT("[W_STARTUP_UNSATISFIABLE] %s requires tag '%s' but no GE/ability grants it"),
                        *GA.Name, *RequiredTag);
                }
            }
        }
    }
}

static bool IsSystemTag(const FString& Tag)
{
    // Tags that come from Narrative Pro or engine
    return Tag.StartsWith(TEXT("State.")) ||
           Tag.StartsWith(TEXT("Narrative.")) ||
           Tag.StartsWith(TEXT("Status."));
}
```

#### Step 3: Validate Cooldown GE Exists
```cpp
// In same function
for (const auto& GA : Data.GameplayAbilities)
{
    if (!GA.CooldownGameplayEffectClass.IsEmpty())
    {
        bool bFound = false;
        for (const auto& GE : Data.GameplayEffects)
        {
            if (GE.Name == GA.CooldownGameplayEffectClass)
            {
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            UE_LOG(LogGasAbilityParser, Warning,
                TEXT("[W_COOLDOWN_MISSING] %s references cooldown GE '%s' not in manifest"),
                *GA.Name, *GA.CooldownGameplayEffectClass);
        }
    }
}
```

### Output Format
```
[W_STARTUP_UNSATISFIABLE] GA_FatherMark requires tag 'Father.State.TargetAcquired' but no GE/ability grants it
[W_COOLDOWN_MISSING] GA_FatherAttack references cooldown GE 'GE_NonexistentCooldown' not in manifest
```

### Test Cases
1. Valid: GA requires tag, GE grants it
2. Invalid: GA requires tag, nothing grants it
3. Edge: Tag granted by equipment-only path
4. Edge: System tags (should not warn)

### Acceptance Criteria
- [ ] All activation_required_tags validated
- [ ] All cooldown_gameplay_effect_class references validated
- [ ] System tags excluded from validation
- [ ] Machine-parseable warning format

---

## 3. Circular Dependency Detection ✅ IMPLEMENTED (v4.17)

**Status:** COMPLETE - Commit `fa5fd34`

### Overview
Detects circular dependencies in asset generation order that would cause generation to fail or produce incorrect results.

### Problem Statement
If Asset A references Asset B, and Asset B references Asset A, the generator may fail or produce incomplete assets depending on generation order.

### Scope
- **Files to modify:** `GasAbilityGeneratorCommandlet.cpp` or new file
- **Files to add:** Optional `GasAbilityGeneratorDependencyGraph.h/cpp`
- **Complexity:** MEDIUM
- **Risk:** LOW - Validation only
- **Rare edge case:** Moderate - can occur with complex cross-references

### Implementation Steps

#### Step 1: Build Dependency Graph
```cpp
struct FAssetDependency
{
    FString AssetName;
    FString AssetType;
    TArray<FString> DependsOn;  // Assets this one references
};

static TMap<FString, FAssetDependency> BuildDependencyGraph(const FManifestData& Data)
{
    TMap<FString, FAssetDependency> Graph;

    // GameplayAbilities depend on their cooldown GEs
    for (const auto& GA : Data.GameplayAbilities)
    {
        FAssetDependency Dep;
        Dep.AssetName = GA.Name;
        Dep.AssetType = TEXT("GameplayAbility");

        if (!GA.CooldownGameplayEffectClass.IsEmpty())
        {
            Dep.DependsOn.Add(GA.CooldownGameplayEffectClass);
        }
        // Add other dependencies (parent class, etc.)

        Graph.Add(GA.Name, Dep);
    }

    // BehaviorTrees depend on Blackboards
    for (const auto& BT : Data.BehaviorTrees)
    {
        FAssetDependency Dep;
        Dep.AssetName = BT.Name;
        Dep.AssetType = TEXT("BehaviorTree");

        if (!BT.Blackboard.IsEmpty())
        {
            Dep.DependsOn.Add(BT.Blackboard);
        }

        Graph.Add(BT.Name, Dep);
    }

    // Continue for all asset types...

    return Graph;
}
```

#### Step 2: Detect Cycles (Tarjan's Algorithm)
```cpp
struct FCycleDetector
{
    TMap<FString, int32> Index;
    TMap<FString, int32> LowLink;
    TMap<FString, bool> OnStack;
    TArray<FString> Stack;
    int32 CurrentIndex = 0;
    TArray<TArray<FString>> SCCs;  // Strongly connected components

    void FindCycles(const TMap<FString, FAssetDependency>& Graph)
    {
        for (const auto& Pair : Graph)
        {
            if (!Index.Contains(Pair.Key))
            {
                StrongConnect(Pair.Key, Graph);
            }
        }
    }

    void StrongConnect(const FString& V, const TMap<FString, FAssetDependency>& Graph)
    {
        Index.Add(V, CurrentIndex);
        LowLink.Add(V, CurrentIndex);
        CurrentIndex++;
        Stack.Push(V);
        OnStack.Add(V, true);

        if (const FAssetDependency* Dep = Graph.Find(V))
        {
            for (const FString& W : Dep->DependsOn)
            {
                if (!Index.Contains(W))
                {
                    StrongConnect(W, Graph);
                    LowLink[V] = FMath::Min(LowLink[V], LowLink[W]);
                }
                else if (OnStack.FindRef(W))
                {
                    LowLink[V] = FMath::Min(LowLink[V], Index[W]);
                }
            }
        }

        if (LowLink[V] == Index[V])
        {
            TArray<FString> SCC;
            FString W;
            do
            {
                W = Stack.Pop();
                OnStack[W] = false;
                SCC.Add(W);
            } while (W != V);

            if (SCC.Num() > 1)  // Cycle found
            {
                SCCs.Add(SCC);
            }
        }
    }
};
```

#### Step 3: Report Cycles
```cpp
static void ValidateNoCyclicDependencies(const FManifestData& Data)
{
    auto Graph = BuildDependencyGraph(Data);

    FCycleDetector Detector;
    Detector.FindCycles(Graph);

    for (const auto& Cycle : Detector.SCCs)
    {
        FString CycleStr = FString::Join(Cycle, TEXT(" -> "));
        UE_LOG(LogGasAbilityParser, Error,
            TEXT("[E_CIRCULAR_DEPENDENCY] Cycle detected: %s -> %s"),
            *CycleStr, *Cycle[0]);
    }
}
```

### Output Format
```
[E_CIRCULAR_DEPENDENCY] Cycle detected: GA_FatherAttack -> GE_AttackDamage -> GA_FatherAttack
```

### Test Cases
1. Valid: Linear dependency chain (A→B→C)
2. Invalid: Simple cycle (A→B→A)
3. Invalid: Complex cycle (A→B→C→A)
4. Edge: Self-reference (A→A)

### Acceptance Criteria
- [ ] All asset dependencies mapped
- [ ] Tarjan's algorithm correctly identifies SCCs
- [ ] Cycles reported with full path
- [ ] Generation blocked on cycle detection (Error, not Warning)

---

## 4. FormState Preset Schema

### Overview
Defines reusable form state configurations to reduce manifest duplication.

### Problem Statement
Each form ability (GA_FatherCrawler, GA_FatherArmor, etc.) repeats similar tag configurations. A preset system would reduce duplication and ensure consistency.

### Scope
- **Files to modify:** `GasAbilityGeneratorParser.cpp`, `GasAbilityGeneratorTypes.h`
- **Files to add:** None
- **Complexity:** MEDIUM
- **Risk:** LOW - Additive feature
- **Rare edge case:** No - convenience feature

### Implementation Steps

#### Step 1: Define Preset Struct
```cpp
// In GasAbilityGeneratorTypes.h
USTRUCT()
struct FManifestFormStatePreset
{
    FString Name;                           // e.g., "FormTransition_Base"
    TArray<FString> ActivationRequiredTags; // Tags required to activate
    TArray<FString> ActivationBlockedTags;  // Tags that block activation
    TArray<FString> ActivationOwnedTags;    // Tags granted during activation
    TArray<FString> CancelAbilitiesWithTag; // Tags of abilities to cancel
};
```

#### Step 2: Add Manifest Section
```yaml
# New manifest section
form_state_presets:
  - name: FormTransition_Base
    activation_required_tags:
      - Father.State.Alive
      - Father.State.Recruited
    activation_blocked_tags:
      - Father.State.Dormant
      - Father.State.Transitioning
      - Father.State.SymbioteLocked
      - Cooldown.Father.FormChange

  - name: FormTransition_Attached
    base_preset: FormTransition_Base  # Inheritance
    activation_owned_tags:
      - Father.State.Attached
```

#### Step 3: Apply Preset in Ability
```yaml
gameplay_abilities:
  - name: GA_FatherArmor
    form_state_preset: FormTransition_Attached  # Apply preset
    tags:
      ability_tags:
        - Ability.Father.Armor
      cancel_abilities_with_tag:  # Still allow overrides
        - Ability.Father.Crawler
        - Ability.Father.Exoskeleton
```

#### Step 4: Parser Implementation
```cpp
static void ApplyFormStatePreset(
    FManifestGameplayAbilityDefinition& GA,
    const TMap<FString, FManifestFormStatePreset>& Presets)
{
    if (GA.FormStatePreset.IsEmpty()) return;

    const FManifestFormStatePreset* Preset = Presets.Find(GA.FormStatePreset);
    if (!Preset)
    {
        UE_LOG(LogGasAbilityParser, Warning,
            TEXT("[W_PRESET_NOT_FOUND] %s references preset '%s' not defined"),
            *GA.Name, *GA.FormStatePreset);
        return;
    }

    // Merge preset tags (preset is base, ability-specific overrides)
    for (const FString& Tag : Preset->ActivationRequiredTags)
    {
        GA.Tags.ActivationRequiredTags.AddUnique(Tag);
    }
    // ... continue for other tag arrays
}
```

### Manifest Example (Before/After)

**Before (repetitive):**
```yaml
gameplay_abilities:
  - name: GA_FatherArmor
    tags:
      activation_required_tags:
        - Father.State.Alive
        - Father.State.Recruited
      activation_blocked_tags:
        - Father.State.Dormant
        - Father.State.Transitioning
        - Father.State.SymbioteLocked
        - Cooldown.Father.FormChange
      activation_owned_tags:
        - Father.State.Attached

  - name: GA_FatherExoskeleton
    tags:
      activation_required_tags:
        - Father.State.Alive      # Duplicated
        - Father.State.Recruited  # Duplicated
      activation_blocked_tags:
        - Father.State.Dormant        # Duplicated
        - Father.State.Transitioning  # Duplicated
        - Father.State.SymbioteLocked # Duplicated
        - Cooldown.Father.FormChange  # Duplicated
      activation_owned_tags:
        - Father.State.Attached
```

**After (with presets):**
```yaml
form_state_presets:
  - name: AttachedForm
    activation_required_tags: [Father.State.Alive, Father.State.Recruited]
    activation_blocked_tags: [Father.State.Dormant, Father.State.Transitioning, Father.State.SymbioteLocked, Cooldown.Father.FormChange]
    activation_owned_tags: [Father.State.Attached]

gameplay_abilities:
  - name: GA_FatherArmor
    form_state_preset: AttachedForm
    tags:
      ability_tags: [Ability.Father.Armor]

  - name: GA_FatherExoskeleton
    form_state_preset: AttachedForm
    tags:
      ability_tags: [Ability.Father.Exoskeleton]
```

### Acceptance Criteria
- [ ] Preset section parsed correctly
- [ ] Preset inheritance works (base_preset)
- [ ] Presets merged into abilities at parse time
- [ ] Missing preset warning logged
- [ ] Ability-specific tags override preset tags

---

## 5. Material Circular Reference Detection

### Overview
Detects circular references in material expression graphs that would cause infinite loops or compilation failures.

### Problem Statement
Material expressions can reference each other. If A→B→C→A, the material compiler may hang or crash.

### Scope
- **Files to modify:** `GasAbilityGeneratorGenerators.cpp` (FMaterialGenerator)
- **Files to add:** None
- **Complexity:** MEDIUM
- **Risk:** LOW - Validation only
- **Rare edge case:** Moderate - possible with complex materials

### Implementation Steps

#### Step 1: Build Expression Graph
```cpp
// In FMaterialGenerator, before connecting expressions
static TMap<FString, TArray<FString>> BuildExpressionGraph(
    const FManifestMaterialDefinition& Definition)
{
    TMap<FString, TArray<FString>> Graph;

    for (const auto& Connection : Definition.Connections)
    {
        // Connection: From expression → To expression
        FString FromExpr = Connection.FromExpression;
        FString ToExpr = Connection.ToExpression;

        if (!Graph.Contains(FromExpr))
        {
            Graph.Add(FromExpr, TArray<FString>());
        }
        Graph[FromExpr].Add(ToExpr);
    }

    return Graph;
}
```

#### Step 2: Detect Cycles (DFS)
```cpp
static bool HasCycle(
    const TMap<FString, TArray<FString>>& Graph,
    TArray<FString>& OutCyclePath)
{
    TSet<FString> Visited;
    TSet<FString> RecStack;
    TArray<FString> Path;

    for (const auto& Pair : Graph)
    {
        if (DetectCycleDFS(Pair.Key, Graph, Visited, RecStack, Path, OutCyclePath))
        {
            return true;
        }
    }

    return false;
}

static bool DetectCycleDFS(
    const FString& Node,
    const TMap<FString, TArray<FString>>& Graph,
    TSet<FString>& Visited,
    TSet<FString>& RecStack,
    TArray<FString>& Path,
    TArray<FString>& OutCyclePath)
{
    Visited.Add(Node);
    RecStack.Add(Node);
    Path.Add(Node);

    if (const TArray<FString>* Neighbors = Graph.Find(Node))
    {
        for (const FString& Neighbor : *Neighbors)
        {
            if (!Visited.Contains(Neighbor))
            {
                if (DetectCycleDFS(Neighbor, Graph, Visited, RecStack, Path, OutCyclePath))
                {
                    return true;
                }
            }
            else if (RecStack.Contains(Neighbor))
            {
                // Found cycle - extract path
                int32 CycleStart = Path.Find(Neighbor);
                for (int32 i = CycleStart; i < Path.Num(); i++)
                {
                    OutCyclePath.Add(Path[i]);
                }
                OutCyclePath.Add(Neighbor);  // Complete the cycle
                return true;
            }
        }
    }

    Path.Pop();
    RecStack.Remove(Node);
    return false;
}
```

#### Step 3: Validate Before Generation
```cpp
// In FMaterialGenerator::Generate()
TArray<FString> CyclePath;
if (HasCycle(BuildExpressionGraph(Definition), CyclePath))
{
    FString CycleStr = FString::Join(CyclePath, TEXT(" -> "));
    Result.Errors.Add(FString::Printf(
        TEXT("[E_MATERIAL_CIRCULAR_REF] %s has circular expression reference: %s"),
        *Definition.Name, *CycleStr));
    return Result;
}
```

### Output Format
```
[E_MATERIAL_CIRCULAR_REF] M_FatherCore has circular expression reference: Multiply_0 -> Add_0 -> Multiply_0
```

### Acceptance Criteria
- [ ] Expression graph built from connections
- [ ] DFS cycle detection implemented
- [ ] Cycle path reported in error
- [ ] Generation blocked on cycle detection

---

## 6. Delegate Binding Automation

### Overview
Automates delegate binding generation for OnDied, OnAttributeChanged, and similar events.

### Problem Statement
Reactive abilities (e.g., "when health drops below 50%, activate") require manual Blueprint delegate binding. This is the most time-consuming manual work remaining.

### Scope
- **Files to modify:** `GasAbilityGeneratorTypes.h`, `GasAbilityGeneratorParser.cpp`, `GasAbilityGeneratorGenerators.cpp`
- **Files to add:** None
- **Complexity:** HIGH
- **Risk:** MEDIUM - New node type generation
- **Rare edge case:** No - major feature

### Implementation Steps

#### Step 1: Define Delegate Binding Struct
```cpp
// In GasAbilityGeneratorTypes.h
USTRUCT()
struct FManifestDelegateBinding
{
    FString DelegateName;      // "OnDied", "OnAttributeChanged", "OnDamagedBy"
    FString SourceComponent;   // "OwnerASC", "PlayerASC", "TargetASC"
    FString HandlerEventName;  // Custom Event to call when delegate fires

    // For OnAttributeChanged
    FString AttributeName;     // "Health", "Stamina"

    // For OnDamagedBy
    FString DamageType;        // Optional filter
};
```

#### Step 2: Manifest Syntax
```yaml
gameplay_abilities:
  - name: GA_FatherArmor
    delegate_bindings:
      - delegate: OnDied
        source: OwnerASC
        handler: HandleOwnerDied

      - delegate: OnAttributeChanged
        source: PlayerASC
        attribute: Health
        handler: HandlePlayerHealthChanged

      - delegate: OnDamagedBy
        source: OwnerASC
        handler: HandleDamageTaken
```

#### Step 3: Generate Custom Events
```cpp
// For each delegate binding, create a Custom Event node
UK2Node_CustomEvent* CreateDelegateHandler(
    UEdGraph* Graph,
    const FManifestDelegateBinding& Binding)
{
    UK2Node_CustomEvent* EventNode = NewObject<UK2Node_CustomEvent>(Graph);
    Graph->AddNode(EventNode, false, false);

    EventNode->CustomFunctionName = FName(*Binding.HandlerEventName);

    // Add parameters based on delegate signature
    if (Binding.DelegateName == TEXT("OnAttributeChanged"))
    {
        // Add FOnAttributeChangeData parameter
        // ...
    }
    else if (Binding.DelegateName == TEXT("OnDied"))
    {
        // Add AActor* Killer, AController* KillerController parameters
        // ...
    }

    EventNode->CreateNewGuid();
    EventNode->PostPlacedNewNode();
    EventNode->AllocateDefaultPins();

    return EventNode;
}
```

#### Step 4: Generate Bind/Unbind Nodes
```cpp
// In ActivateAbility event, after getting ASC:
// Create: ASC->OnAttributeChanged.AddDynamic(this, &HandleAttributeChanged)

// In EndAbility event:
// Create: ASC->OnAttributeChanged.RemoveDynamic(this, &HandleAttributeChanged)
```

### Delegate Signature Reference

| Delegate | Source | Signature |
|----------|--------|-----------|
| OnDied | ANarrativeCharacter | `(AActor* Killer, AController* KillerController)` |
| OnAttributeChanged | UAbilitySystemComponent | `(const FOnAttributeChangeData& Data)` |
| OnDamagedBy | UNarrativeAbilitySystemComponent | `(FNarrativeDamageInfo DamageInfo)` |
| OnGameplayEffectApplied | UAbilitySystemComponent | `(UAbilitySystemComponent* Source, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)` |

### Acceptance Criteria
- [ ] Delegate bindings parsed from manifest
- [ ] Custom Event nodes created with correct signatures
- [ ] Bind nodes created in ActivateAbility
- [ ] Unbind nodes created in EndAbility
- [ ] Supports OnDied, OnAttributeChanged, OnDamagedBy at minimum

---

## 7. Split Generators.cpp

### Overview
Refactors the 18,528 LOC `GasAbilityGeneratorGenerators.cpp` file into per-generator modules for maintainability.

### Problem Statement
The file is too large to navigate efficiently. Changes to one generator risk affecting others. Code review is difficult.

### Scope
- **Files to modify:** `GasAbilityGenerator.Build.cs`, `GasAbilityGeneratorGenerators.cpp`
- **Files to add:** ~15 new `.cpp` files
- **Complexity:** HIGH (mechanical, not algorithmic)
- **Risk:** HIGH - Touching all generators
- **Rare edge case:** No - code quality improvement

### Proposed Structure
```
Source/GasAbilityGenerator/Private/Generators/
├── GeneratorBase.cpp           # Shared utilities, logging
├── EnumerationGenerator.cpp    # E_*
├── InputActionGenerator.cpp    # IA_*
├── InputMappingContextGenerator.cpp  # IMC_*
├── GameplayEffectGenerator.cpp # GE_*
├── GameplayAbilityGenerator.cpp # GA_* (largest)
├── ActorBlueprintGenerator.cpp # BP_*
├── WidgetBlueprintGenerator.cpp # WBP_*
├── BlackboardGenerator.cpp     # BB_*
├── BehaviorTreeGenerator.cpp   # BT_*
├── MaterialGenerator.cpp       # M_*, MF_*, MIC_*
├── AnimationGenerator.cpp      # AM_*, NAS_*
├── DialogueGenerator.cpp       # DBP_*
├── ItemGenerator.cpp           # EI_*, IC_*
├── NPCGenerator.cpp            # NPC_*, CD_*, TDS_*
├── NiagaraGenerator.cpp        # NS_*
├── QuestGenerator.cpp          # Quest_*, Goal_*
└── EventGraphGenerator.cpp     # Shared event graph logic
```

### Implementation Steps

#### Step 1: Extract Shared Utilities
```cpp
// GeneratorBase.cpp
namespace GeneratorUtils
{
    void LogGeneration(const FString& Message);
    UPackage* CreatePackageForAsset(const FString& PackagePath);
    bool SaveGeneratedAsset(UObject* Asset, UPackage* Package);
    FString ResolveClassPath(const FString& ClassName);
    // ... other shared functions
}
```

#### Step 2: Extract Per-Generator (Example)
```cpp
// GameplayEffectGenerator.cpp
#include "GeneratorBase.h"

class FGameplayEffectGenerator
{
public:
    static FGenerationResult Generate(
        const FManifestGameplayEffectDefinition& Definition,
        const FString& ProjectRoot,
        UGeneratorMetadataRegistry* Registry);

private:
    static void ConfigureModifiers(UGameplayEffect* Effect, const FManifestGameplayEffectDefinition& Definition);
    static void ConfigureTags(UGameplayEffect* Effect, const FManifestGameplayEffectDefinition& Definition);
    static void ConfigureDuration(UGameplayEffect* Effect, const FManifestGameplayEffectDefinition& Definition);
};
```

#### Step 3: Update Build.cs
```csharp
// Add new source files
PrivateDependencyModuleNames.AddRange(new string[] { ... });

// No changes needed if files are in Private/ subfolder - UBT auto-discovers
```

#### Step 4: Migration Order
1. Extract `GeneratorBase.cpp` (shared utilities)
2. Extract smallest generators first (Enumeration, InputAction)
3. Extract medium generators (Material, Animation)
4. Extract largest generators (GameplayAbility, EventGraph)
5. Remove `GasAbilityGeneratorGenerators.cpp`
6. Update all includes

### Risk Mitigation
- Git branch for refactor
- One generator per commit
- Run full generation test after each extraction
- Keep function signatures identical

### Acceptance Criteria
- [ ] All generators in separate files
- [ ] No functionality changes
- [ ] All 163 assets still generate correctly
- [ ] Compilation time not significantly increased
- [ ] Each file < 2000 LOC

---

## Summary Matrix

| # | Task | Complexity | Risk | Edge Case | Files Changed |
|---|------|------------|------|-----------|---------------|
| 1 | P1.2 Transition Validation | LOW | NONE | No | 1 |
| 2 | P1.3 Startup Validation | MEDIUM | NONE | No | 1 |
| 3 | Circular Dep Detection | MEDIUM | LOW | Moderate | 1-2 |
| 4 | FormState Preset Schema | MEDIUM | LOW | No | 2 |
| 5 | Material Circular Refs | MEDIUM | LOW | Moderate | 1 |
| 6 | Delegate Binding | HIGH | MEDIUM | No | 3 |
| 7 | Split Generators.cpp | HIGH | HIGH | No | ~17 |

---

## Audit Checklist

For each implementation:
- [ ] Does it follow existing code patterns?
- [ ] Is the warning/error format machine-parseable?
- [ ] Does it have clear test cases?
- [ ] Is the risk assessment accurate?
- [ ] Are all edge cases documented?
- [ ] Does it require changes to locked files?
- [ ] Is the complexity estimate reasonable?
