# Implementation Plans - Audit Document v1.1

**Created:** 2026-01-21
**Updated:** 2026-01-21
**Purpose:** Detailed implementation plans for remaining TODO items, ready for GPT audit
**Status:** Sections 1, 3, 8 IMPLEMENTED (v4.17, v4.18, v4.19); Section 9 PENDING AUDIT

---

## Table of Contents

1. [P1.2 Transition Validation](#1-p12-transition-validation) ✅ IMPLEMENTED
2. [P1.3 Startup Validation](#2-p13-startup-validation)
3. [Circular Dependency Detection](#3-circular-dependency-detection) ✅ IMPLEMENTED
4. [FormState Preset Schema](#4-formstate-preset-schema)
5. [Material Circular Reference Detection](#5-material-circular-reference-detection)
6. [Delegate Binding Automation](#6-delegate-binding-automation)
7. [Split Generators.cpp](#7-split-generatorscpp)
8. [ActorComponentBlueprintGenerator](#8-actorcomponentblueprintgenerator-ultimatechargecomponent) ✅ IMPLEMENTED
9. [Delegate Binding Automation](#9-delegate-binding-automation-p21) ⏳ PENDING AUDIT

---

## 1. P1.2 Transition Validation ✅ IMPLEMENTED (v4.18)

**Status:** COMPLETE - Commit `8687f88`
**Audit Status:** GPT approved with 3 refinements (accepted 2026-01-21)

### Overview
Parse-time lint for Father form-transition state machine. Warns early about invalid transitions.

**Mental Model:** "Lint the manifest's form-transition state machine and warn early, not verify the generated asset."

### Problem Statement
Currently, the manifest can define invalid form transitions (e.g., Crawler→Symbiote when Symbiote requires Attached state but Crawler doesn't grant it). These errors only surface at runtime.

### Scope
- **Files to modify:** `GasAbilityGeneratorParser.cpp`
- **Files to add:** None
- **Complexity:** LOW
- **Risk:** NONE - Additive validation only, log-only warnings
- **Project-specific:** YES - Father companion forms only (intentional scope per Rule #9)

### GPT Audit Refinements (Accepted)

| Caveat | Resolution |
|--------|------------|
| Father-specific scope | Intentional. Treat as scoped validation, not general-purpose. |
| Weak keying (substring) | Use tag-based extraction: `Ability.Father.{Form}` → Form name |
| Warning channel | Log-only at parse time. No report integration (architectural mismatch). |

### Implementation Steps

#### Step 1: Extract Form Name from Ability Tags
```cpp
// In GasAbilityGeneratorParser.cpp (static helper)
// Extract form name from Ability.Father.{Form} tag
static FString ExtractFormNameFromTags(const TArray<FString>& AbilityTags)
{
    for (const FString& Tag : AbilityTags)
    {
        if (Tag.StartsWith(TEXT("Ability.Father.")))
        {
            // Ability.Father.Crawler -> Crawler
            FString FormName = Tag.RightChop(15); // len("Ability.Father.")
            // Only accept known form names
            if (FormName == TEXT("Crawler") || FormName == TEXT("Armor") ||
                FormName == TEXT("Exoskeleton") || FormName == TEXT("Symbiote") ||
                FormName == TEXT("Engineer"))
            {
                return FormName;
            }
        }
    }
    return FString(); // Not a form ability
}
```

#### Step 2: Add Validation Function
```cpp
// In GasAbilityGeneratorParser.cpp
static void ValidateFormTransitions(const FManifestData& Data)
{
    // Build Form -> Ability map using tag-based extraction
    TMap<FString, const FManifestGameplayAbilityDefinition*> FormToAbility;
    TMap<FString, FString> AbilityTagToForm; // Ability.Father.X -> Form name

    for (const auto& GA : Data.GameplayAbilities)
    {
        FString FormName = ExtractFormNameFromTags(GA.Tags.AbilityTags);
        if (!FormName.IsEmpty())
        {
            FormToAbility.Add(FormName, &GA);
            FString AbilityTag = FString::Printf(TEXT("Ability.Father.%s"), *FormName);
            AbilityTagToForm.Add(AbilityTag, FormName);
        }
    }

    // No form abilities found - skip validation
    if (FormToAbility.Num() == 0)
    {
        return;
    }

    // Validate each form's cancel_abilities_with_tag
    for (const auto& Pair : FormToAbility)
    {
        const FString& SourceForm = Pair.Key;
        const auto* SourceGA = Pair.Value;

        for (const FString& CancelTag : SourceGA->Tags.CancelAbilitiesWithTag)
        {
            // Is this cancelling another form ability?
            if (FString* TargetFormPtr = AbilityTagToForm.Find(CancelTag))
            {
                const FString& TargetForm = *TargetFormPtr;
                const auto** TargetGAPtr = FormToAbility.Find(TargetForm);
                if (!TargetGAPtr) continue;
                const auto* TargetGA = *TargetGAPtr;

                // Check: Does SourceGA grant tags that TargetGA requires?
                // (When Source cancels Target, Target must be able to reactivate later)
                for (const FString& RequiredTag : TargetGA->Tags.ActivationRequiredTags)
                {
                    // Skip universal tags that are always present
                    if (RequiredTag == TEXT("Father.State.Alive") ||
                        RequiredTag == TEXT("Father.State.Recruited"))
                    {
                        continue;
                    }

                    // Check if source grants this tag via ActivationOwnedTags
                    bool bSourceGrantsTag = SourceGA->Tags.ActivationOwnedTags.Contains(RequiredTag);
                    if (!bSourceGrantsTag)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[W_TRANSITION_INVALID] %s (form %s) cancels %s but target requires '%s' which source doesn't grant"),
                            *SourceGA->Name, *SourceForm, *CancelTag, *RequiredTag);
                    }
                }

                // Check: Does SourceGA have blocked tags that would prevent transition back?
                for (const FString& BlockedTag : SourceGA->Tags.ActivationBlockedTags)
                {
                    // If source grants a tag that blocks target, warn
                    if (TargetGA->Tags.ActivationOwnedTags.Contains(BlockedTag))
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[W_TRANSITION_INVALID] %s (form %s) grants '%s' which blocks reactivation of cancelled form %s"),
                            *SourceGA->Name, *SourceForm, *BlockedTag, *TargetForm);
                    }
                }
            }
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
[W_TRANSITION_INVALID] GA_FatherSymbiote (form Symbiote) cancels Ability.Father.Crawler but target requires 'Father.State.Attached' which source doesn't grant
[W_TRANSITION_INVALID] GA_FatherArmor (form Armor) grants 'Father.State.SymbioteLocked' which blocks reactivation of cancelled form Symbiote
```

### Test Cases
1. **Valid:** Crawler→Armor (no special requirements) - no warning
2. **Valid:** Current manifest (all transitions properly configured) - no warning
3. **Edge:** Non-form ability with `Ability.Father.*` tag - ignored (not in known form list)
4. **Edge:** Form ability without `cancel_abilities_with_tag` - skipped

### Acceptance Criteria
- [x] Form names extracted from `Ability.Father.{Form}` tags (not substring matching)
- [x] Log-only warnings at parse time (no report integration)
- [x] Machine-parseable `[W_TRANSITION_INVALID]` format
- [x] Father-specific scope documented
- [ ] All form abilities validated at parse time
- [ ] No false positives on valid manifest

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

## 8. ActorComponentBlueprintGenerator (UltimateChargeComponent)

**Status:** IMPLEMENTED (v4.19) - Commit 35e477a
**Audit Status:** GPT approved 2026-01-21, Claude counter-audited, final refinements locked

### Overview
New generator for Blueprint ActorComponents with full support for:
- Variables (standard types)
- Event Dispatchers with parameters
- Functions with input/output pins
- Tick configuration (bCanEverTick, TickInterval)

**Primary Use Case:** UltimateChargeComponent - complex component with charge tracking, decay, and events.

### Problem Statement
ActorComponent Blueprints cannot currently be generated. Manual Blueprint work is required for:
- `UltimateChargeComponent` (charge system)
- Future companion components
- Custom gameplay components

### Scope
- **Files to modify:** `GasAbilityGeneratorTypes.h`, `GasAbilityGeneratorParser.cpp`, `GasAbilityGeneratorGenerators.cpp`
- **Files to add:** None
- **Complexity:** HIGH
- **Risk:** MEDIUM - New generator type, complex Blueprint APIs
- **Rare edge case:** No - major feature

### Research Validation Summary

| Topic | Status | Source |
|-------|--------|--------|
| Event Dispatcher Creation | ✅ VALIDATED | `BlueprintEditor.cpp:9615-9652` |
| Event Dispatcher Parameters | ✅ VALIDATED | `K2Node_EditablePinBase::CreateUserDefinedPin()` |
| Function Graph Creation | ✅ VALIDATED | `FBlueprintEditorUtils::AddFunctionGraph<>` |
| Function Input/Output Pins | ✅ VALIDATED | `UK2Node_FunctionEntry::CreateUserDefinedPin()` |
| Function Pure Flag | ✅ VALIDATED | `EntryNode->AddExtraFlags(FUNC_BlueprintPure)` - `K2Node_FunctionEntry.h:131` |
| Headless Compile | ✅ VALIDATED | Plugin uses 18+ compile calls, works with `-nullrhi` |
| Tick Configuration | ✅ VALIDATED | Set on CDO AFTER compile: `PrimaryComponentTick.bCanEverTick` |
| CDO Property Access | ✅ VALIDATED | `Blueprint->GeneratedClass->GetDefaultObject()` AFTER compile |
| AddMemberVariable Marks | ✅ VALIDATED | Already calls `MarkBlueprintAsStructurallyModified` internally - `BlueprintEditorUtils.cpp:4700` |

### Critical Timing Constraint

```
┌─────────────────────────────────────────────────────────────────────────┐
│ CRITICAL: CDO properties set BEFORE compile are LOST                    │
│ Compile recreates the class and CDO - all pre-compile state is wiped   │
│                                                                          │
│ Pattern: Variables → Dispatchers → Functions → COMPILE → CDO props → Save │
└─────────────────────────────────────────────────────────────────────────┘
```

**Source:** `GasAbilityGeneratorGenerators.cpp:2246-2250` comment:
> "Compile FIRST to ensure GeneratedClass and CDO exist before setting properties. CDO properties set before compile will be lost when compile recreates the class"

### Implementation Phases

#### Phase 1: Create Blueprint with UActorComponent Parent

```cpp
// Create ActorComponent Blueprint (parallel to Actor Blueprint pattern)
UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
Factory->ParentClass = UActorComponent::StaticClass();

UBlueprint* Blueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
    UBlueprint::StaticClass(),
    Package,
    FName(*Definition.Name),
    RF_Public | RF_Standalone,
    nullptr,
    GWarn
));
```

#### Phase 2: Add Variables

**Note:** `AddMemberVariable` already calls `MarkBlueprintAsStructurallyModified` internally (`BlueprintEditorUtils.cpp:4700`). Do NOT call it again after adding variables. Only call `MarkBlueprintAsModified` if changing defaults/metadata after creation.

```cpp
// Duplicate guard FIRST
TSet<FName> ExistingVarNames;
for (const auto& Var : Blueprint->NewVariables) { ExistingVarNames.Add(Var.VarName); }

for (const FManifestActorVariableDefinition& VarDef : Definition.Variables)
{
    // Check for duplicates (fail fast)
    if (ExistingVarNames.Contains(FName(*VarDef.Name)))
    {
        Result.Errors.Add(FString::Printf(TEXT("[E_DUPLICATE_VARIABLE] %s"), *VarDef.Name));
        return Result;
    }

    FEdGraphPinType PinType = GetPinTypeFromString(VarDef.Type);
    bool bSuccess = FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VarDef.Name), PinType);
    // AddMemberVariable already calls MarkBlueprintAsStructurallyModified - no extra mark needed

    if (bSuccess)
    {
        int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, FName(*VarDef.Name));
        if (VarIndex != INDEX_NONE)
        {
            if (VarDef.bInstanceEditable)
                Blueprint->NewVariables[VarIndex].PropertyFlags |= CPF_Edit;
            if (VarDef.bReplicated)
                Blueprint->NewVariables[VarIndex].PropertyFlags |= CPF_Net;
            if (!VarDef.DefaultValue.IsEmpty())
                Blueprint->NewVariables[VarIndex].DefaultValue = VarDef.DefaultValue;
            // MarkBlueprintAsModified only needed if we change defaults/metadata here
        }
        ExistingVarNames.Add(FName(*VarDef.Name));
    }
}
```

#### Phase 3: Add Event Dispatchers with Parameters

**Source:** `BlueprintEditor.cpp:9615-9652`

```cpp
// Duplicate guard FIRST
TSet<FName> ExistingDispatcherNames;
for (UEdGraph* Graph : Blueprint->DelegateSignatureGraphs) { ExistingDispatcherNames.Add(Graph->GetFName()); }

for (const FManifestEventDispatcherDefinition& DispatcherDef : Definition.EventDispatchers)
{
    // Check for duplicates (fail fast)
    if (ExistingDispatcherNames.Contains(FName(*DispatcherDef.Name)))
    {
        Result.Errors.Add(FString::Printf(TEXT("[E_DUPLICATE_DISPATCHER] %s"), *DispatcherDef.Name));
        return Result;
    }

    // Step 1: Add multicast delegate member variable
    FEdGraphPinType DelegateType;
    DelegateType.PinCategory = UEdGraphSchema_K2::PC_MCDelegate;
    bool bVarCreated = FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*DispatcherDef.Name), DelegateType);

// Step 2: Create delegate signature graph
UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(
    Blueprint,
    FName(*DispatcherName),
    UEdGraph::StaticClass(),
    UEdGraphSchema_K2::StaticClass()
);

NewGraph->bEditable = false;

const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
K2Schema->CreateDefaultNodesForGraph(*NewGraph);
K2Schema->CreateFunctionGraphTerminators(*NewGraph, (UClass*)nullptr);
K2Schema->AddExtraFunctionFlags(NewGraph, (FUNC_BlueprintCallable | FUNC_BlueprintEvent | FUNC_Public));
K2Schema->MarkFunctionEntryAsEditable(NewGraph, true);  // CRITICAL: enables parameter addition

// Step 3: Add to DelegateSignatureGraphs array
Blueprint->DelegateSignatureGraphs.Add(NewGraph);

// Step 4: Add parameters to entry node
TArray<UK2Node_FunctionEntry*> EntryNodes;
NewGraph->GetNodesOfClass<UK2Node_FunctionEntry>(EntryNodes);
if (EntryNodes.Num() > 0)
{
    UK2Node_FunctionEntry* EntryNode = EntryNodes[0];

    for (const FManifestDispatcherParam& Param : DispatcherDef.Parameters)
    {
        FEdGraphPinType ParamPinType = GetPinTypeFromString(Param.Type);
        EntryNode->CreateUserDefinedPin(
            FName(*Param.Name),
            ParamPinType,
            EGPD_Output  // Output from entry = input parameter
        );
    }
}

FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
```

#### Phase 4: Add Functions with Input/Output Pins

**Reserved Suffixes:** Do not allow function names ending with: `_Implementation`, `_Validate`, `_C`, `__`

```cpp
// Reserved suffix guard
static const TArray<FString> ReservedSuffixes = { TEXT("_Implementation"), TEXT("_Validate"), TEXT("_C"), TEXT("__") };

// Duplicate guard FIRST
TSet<FName> ExistingFuncNames;
for (UEdGraph* Graph : Blueprint->FunctionGraphs) { ExistingFuncNames.Add(Graph->GetFName()); }

for (const FManifestFunctionDefinition& FuncDef : Definition.Functions)
{
    // Check for duplicates (fail fast)
    if (ExistingFuncNames.Contains(FName(*FuncDef.Name)))
    {
        Result.Errors.Add(FString::Printf(TEXT("[E_DUPLICATE_FUNCTION] %s"), *FuncDef.Name));
        return Result;
    }

    // Check for reserved suffixes
    for (const FString& Suffix : ReservedSuffixes)
    {
        if (FuncDef.Name.EndsWith(Suffix))
        {
            Result.Errors.Add(FString::Printf(TEXT("[E_RESERVED_SUFFIX] %s uses reserved suffix %s"), *FuncDef.Name, *Suffix));
            return Result;
        }
    }

    // Step 1: Create function graph
    UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint,
        FName(*FuncDef.Name),
        UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass()
    );

    FBlueprintEditorUtils::AddFunctionGraph(Blueprint, FuncGraph, true, nullptr);

    // Step 2: Get entry node
    TArray<UK2Node_FunctionEntry*> EntryNodes;
    FuncGraph->GetNodesOfClass<UK2Node_FunctionEntry>(EntryNodes);
    if (EntryNodes.Num() > 0)
    {
        UK2Node_FunctionEntry* EntryNode = EntryNodes[0];

        // Step 2a: Set pure flag BEFORE creating pins (affects pin layout)
        if (FuncDef.bPure)
        {
            EntryNode->AddExtraFlags(FUNC_BlueprintPure);
        }
        else
        {
            // Explicitly clear pure flag (deterministic)
            EntryNode->SetExtraFlags(EntryNode->GetExtraFlags() & ~FUNC_BlueprintPure);
        }

        // Step 2b: Add input parameters
        for (const FManifestFunctionParam& Param : FuncDef.InputParams)
        {
            FEdGraphPinType ParamPinType = GetPinTypeFromString(Param.Type);
            EntryNode->CreateUserDefinedPin(
                FName(*Param.Name),
                ParamPinType,
                EGPD_Output  // Output from entry = function input
            );
        }
    }

    // Step 3: Get result node and add output parameters (return values)
    TArray<UK2Node_FunctionResult*> ResultNodes;
    FuncGraph->GetNodesOfClass<UK2Node_FunctionResult>(ResultNodes);
    if (ResultNodes.Num() > 0 && FuncDef.OutputParams.Num() > 0)
    {
        UK2Node_FunctionResult* ResultNode = ResultNodes[0];

        for (const FManifestFunctionParam& Param : FuncDef.OutputParams)
        {
            FEdGraphPinType ParamPinType = GetPinTypeFromString(Param.Type);
            ResultNode->CreateUserDefinedPin(
                FName(*Param.Name),
                ParamPinType,
                EGPD_Input  // Input to result = function output
            );
        }
    }

    ExistingFuncNames.Add(FName(*FuncDef.Name));
}
```

#### Phase 5: Compile (MUST happen before CDO access)

```cpp
// Compile with validation (existing Contract 10 pattern)
FCompilerResultsLog CompileLog;
FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &CompileLog);

if (CompileLog.NumErrors > 0)
{
    // Extract and report errors...
    Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed, ErrorMessage);
    return Result;
}

if (!Blueprint->GeneratedClass)
{
    Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Compile failed - no GeneratedClass"));
    return Result;
}
```

#### Phase 6: Configure Tick on CDO (AFTER compile)

```cpp
// Get CDO - ONLY valid after compile
UActorComponent* ComponentCDO = Cast<UActorComponent>(Blueprint->GeneratedClass->GetDefaultObject());
if (!ComponentCDO)
{
    Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to get ActorComponent CDO"));
    return Result;
}

// Configure tick (must be done on CDO after compile)
if (Definition.bCanEverTick)
{
    ComponentCDO->PrimaryComponentTick.bCanEverTick = true;
    ComponentCDO->PrimaryComponentTick.bStartWithTickEnabled = Definition.bStartWithTickEnabled;

    if (Definition.TickInterval > 0.0f)
    {
        ComponentCDO->PrimaryComponentTick.TickInterval = Definition.TickInterval;
    }
}
else
{
    ComponentCDO->PrimaryComponentTick.bCanEverTick = false;
}

// Mark dirty and save
ComponentCDO->MarkPackageDirty();
```

#### Phase 7: Save Asset

```cpp
// Save package (existing pattern)
FString PackageFilename = FPackageName::LongPackageNameToFilename(
    PackagePath, FPackageName::GetAssetPackageExtension());
FSavePackageArgs SaveArgs;
SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
UPackage::SavePackage(Package, Blueprint, *PackageFilename, SaveArgs);
```

### New Type Definitions

```cpp
// In GasAbilityGeneratorTypes.h

// Event dispatcher parameter
USTRUCT()
struct FManifestDispatcherParam
{
    FString Name;   // "CurrentCharge"
    FString Type;   // "Float", "E_FatherUltimate", "Object"
};

// Event dispatcher definition
USTRUCT()
struct FManifestEventDispatcherDefinition
{
    FString Name;                              // "OnChargeChanged"
    TArray<FManifestDispatcherParam> Parameters;

    uint32 ComputeHash() const;
};

// Function parameter (input or output)
USTRUCT()
struct FManifestFunctionParam
{
    FString Name;   // "Amount"
    FString Type;   // "Float"
};

// Function definition
USTRUCT()
struct FManifestFunctionDefinition
{
    FString Name;                              // "AddCharge"
    TArray<FManifestFunctionParam> InputParams;
    TArray<FManifestFunctionParam> OutputParams;
    bool bPure = false;                        // Pure function flag

    uint32 ComputeHash() const;
};

// Component blueprint definition
USTRUCT()
struct FManifestComponentBlueprintDefinition
{
    FString Name;
    FString Folder;
    FString ParentClass = TEXT("ActorComponent");  // Default parent

    // Variables (reuse existing struct)
    TArray<FManifestActorVariableDefinition> Variables;

    // Event Dispatchers
    TArray<FManifestEventDispatcherDefinition> EventDispatchers;

    // Functions
    TArray<FManifestFunctionDefinition> Functions;

    // Tick configuration
    bool bCanEverTick = false;
    bool bStartWithTickEnabled = false;
    float TickInterval = 0.0f;

    uint32 ComputeHash() const;
};
```

### Manifest Schema

```yaml
component_blueprints:
  - name: UltimateChargeComponent
    folder: Components
    parent_class: ActorComponent

    # Tick configuration
    can_ever_tick: true
    start_with_tick_enabled: true
    tick_interval: 0.1  # 10 times per second

    # Variables
    variables:
      - name: CurrentCharge
        type: Float
        default_value: "0.0"
      - name: bIsChargeReady
        type: Bool
        default_value: "false"
      - name: ReadyUltimateType
        type: E_FatherUltimate
        default_value: "Symbiote"
      - name: ThresholdCurve
        type: RuntimeFloatCurve
        instance_editable: true
        default:
          keys:
            - { time: 1.0, value: 5000.0 }
            - { time: 10.0, value: 10000.0 }
            - { time: 50.0, value: 40000.0 }
          pre_infinity: Constant
          post_infinity: Linear

    # Event Dispatchers
    event_dispatchers:
      - name: OnChargeChanged
        parameters:
          - name: NewCharge
            type: Float
          - name: PreviousCharge
            type: Float
      - name: OnUltimateReady
        parameters:
          - name: UltimateType
            type: E_FatherUltimate
      - name: OnChargeReset
        parameters: []  # No parameters

    # Functions
    functions:
      - name: AddCharge
        input_params:
          - name: Amount
            type: Float
        output_params: []  # Void return

      - name: GetThresholdForLevel
        input_params:
          - name: Level
            type: Int
        output_params:
          - name: Threshold
            type: Float
        pure: true

      - name: ResetCharge
        input_params: []
        output_params: []
```

### Parent Class Resolution Policy

| Input Format | Action |
|--------------|--------|
| Starts with `/Script/` or `/Game/` | Strict path resolve; fail with `[E_PARENT_CLASS_RESOLVE]` if not found |
| Short name in whitelist | Use `::StaticClass()` |
| Short name NOT in whitelist | Fail with `[E_PARENT_CLASS_RESOLVE]` |

**Native Parent Class Whitelist:**
- `ActorComponent` → `UActorComponent::StaticClass()`
- `SceneComponent` → `USceneComponent::StaticClass()`
- `PrimitiveComponent` → `UPrimitiveComponent::StaticClass()`
- `AudioComponent` → `UAudioComponent::StaticClass()`
- `WidgetComponent` → `UWidgetComponent::StaticClass()`

### RuntimeFloatCurve YAML Schema

```yaml
# FRuntimeFloatCurve struct variable with inline keys
- name: ThresholdCurve
  type: RuntimeFloatCurve
  default:
    keys:                          # Sorted by time during parse (determinism)
      - { time: 0.0, value: 0.0 }
      - { time: 1.0, value: 100.0 }
    pre_infinity: Constant         # Constant, Linear, Cycle, CycleWithOffset, Oscillate
    post_infinity: Linear
```

**Constraints:**
- Keys sorted by `time` ascending during parse (deterministic)
- If `default:` omitted → generate empty curve (single key at `{0.0, 0.0}`)
- Pin type: `PC_Struct` with `FRuntimeFloatCurve::StaticStruct()`

### Breadcrumb Logging (7-Phase)

Stable, greppable format for CI parsing:

```
COMP_BP[UltimateChargeComponent] PH1 CreateAsset OK
COMP_BP[UltimateChargeComponent] PH2 Variables Added=4 OK
COMP_BP[UltimateChargeComponent] PH3 Dispatchers Added=3 OK
COMP_BP[UltimateChargeComponent] PH4 Functions Added=4 OK
COMP_BP[UltimateChargeComponent] PH5 Compile OK Errors=0 Warnings=0
COMP_BP[UltimateChargeComponent] PH6 CDO Tick CanEver=true StartEnabled=true Interval=0.1 OK
COMP_BP[UltimateChargeComponent] PH7 Save OK
```

On failure:
```
COMP_BP[UltimateChargeComponent] PH5 Compile FAIL Errors=3 -> see CompileLog
```

**Note:** PH6 (CDO Tick) MUST appear AFTER PH5 (Compile) - this ordering is enforced by the phase structure.

### Test Cases

| # | Test | Expected |
|---|------|----------|
| 1 | Generate component with tick disabled | `bCanEverTick = false` on CDO |
| 2 | Generate component with tick enabled | `bCanEverTick = true`, `bStartWithTickEnabled = true` on CDO |
| 3 | Generate dispatcher with 0 params | Empty signature, compiles cleanly |
| 4 | Generate dispatcher with 2 params | Both params visible on Call node, signature graph has pins |
| 5 | Generate function with return value | Output pin on function node |
| 6 | Generate pure function | Function marked as pure, no exec pins |
| 7 | Compile failure on invalid parent | `[E_PARENT_CLASS_RESOLVE]` error, no asset saved |
| 8 | Duplicate variable name | `[E_DUPLICATE_VARIABLE]` error, fail fast |
| 9 | Duplicate dispatcher name | `[E_DUPLICATE_DISPATCHER]` error, fail fast |
| 10 | Duplicate function name | `[E_DUPLICATE_FUNCTION]` error, fail fast |
| 11 | Function with reserved suffix | `[E_RESERVED_SUFFIX]` error for `_Implementation` etc. |
| 12 | RuntimeFloatCurve variable | Struct pin created, keys sorted by time |
| 13 | Breadcrumb logs emitted | All 7 PH lines present in log |

### Dependencies

Requires module additions to `GasAbilityGenerator.Build.cs`:
- Already present: `BlueprintGraph`, `Kismet`, `KismetCompiler`
- **No new dependencies required**

### Acceptance Criteria

- [ ] Component blueprints generate with correct parent class
- [ ] Variables created with correct types and defaults
- [ ] Event dispatchers have parameters visible in editor
- [ ] Functions have input/output pins visible in editor
- [ ] Pure functions have no exec pins
- [ ] Tick configuration persists on CDO (verified AFTER compile)
- [ ] Compile gate blocks save on error
- [ ] Duplicate guards fail fast with correct error codes
- [ ] Reserved suffix guard active
- [ ] Parent class whitelist enforced
- [ ] Breadcrumb logs emitted for all 7 phases
- [ ] All 13 test cases pass

### Audit Trail

**Initial Plan (Claude):** 7-phase implementation with CDO-after-compile constraint
**GPT Audit:** Approved with 6 refinements
**Claude Counter-Audit:** Validated GPT refinements against UE5 source code
**GPT Final Review:** Accepted with minor adjustments

| Topic | Resolution | Source |
|-------|------------|--------|
| Compile gate | **Not needed** - Contract 10 exists (v4.16) | `GasAbilityGeneratorGenerators.cpp:2246` |
| Tick timing | **CRITICAL** - Must be AFTER compile | `GasAbilityGeneratorGenerators.cpp:2246-2250` |
| AddMemberVariable marking | Already calls `MarkBlueprintAsStructurallyModified` | `BlueprintEditorUtils.cpp:4700` |
| Pure flag API | `AddExtraFlags(FUNC_BlueprintPure)` before pins | `K2Node_FunctionEntry.h:131` |
| Dispatcher params | `CreateUserDefinedPin()` on entry node | `K2Node_EditablePinBase.cpp:162` |
| Function returns | `CreateUserDefinedPin()` on result node | Same API |

---

## 9. Delegate Binding Automation (P2.1)

**Status:** APPROVED
**Audit Status:** Claude-GPT dual audit PASSED (2026-01-21)

### Overview
Automate multicast delegate binding in GameplayAbility event graphs. Enables reactive abilities that respond to OnDied, OnDamagedBy, OnAttributeChanged, etc. without manual Blueprint wiring.

**Primary Use Cases:**
- GA_FatherArmor: OnDamagedBy → redirect damage to Father
- GA_ProtectiveDome: OnDied → trigger dome destruction
- Form abilities: OnAttributeChanged → react to health/resource changes

### Problem Statement
Currently, abilities requiring delegate callbacks (OnDied, OnDamagedBy, OnAttributeChanged) must be wired manually in the Blueprint editor:
1. Get reference to ASC or target object
2. Drag from delegate property → "Bind to Event"
3. Create Custom Event with matching signature
4. Wire cleanup (unbind) in EndAbility

This creates ~8-12 nodes per delegate binding, which cannot be generated from manifest.

### Research Summary

#### Narrative Pro Delegates (NarrativeAbilitySystemComponent.h:56-59)

| Delegate | Signature (exact from header) | Property |
|----------|------------------------------|----------|
| `FOnDied` | `(AActor* KilledActor, UNarrativeAbilitySystemComponent* KilledActorASC)` | `OnDied` |
| `FOnHealedBy` | `(UNarrativeAbilitySystemComponent* Healer, const float Amount, const FGameplayEffectSpec& Spec)` | `OnHealedBy` |
| `FOnDamagedBy` | `(UNarrativeAbilitySystemComponent* DamagerCauserASC, const float Damage, const FGameplayEffectSpec& Spec)` | `OnDamagedBy` |
| `FOnDealtDamage` | `(UNarrativeAbilitySystemComponent* DamagedASC, const float Damage, const FGameplayEffectSpec& Spec)` | `OnDealtDamage` |

**Critical:** `const float` and `const FGameplayEffectSpec&` - affects Blueprint pin generation.

#### GAS Core Delegates (AbilitySystemComponent.h)

| Delegate | Access Pattern | Signature |
|----------|---------------|-----------|
| `OnGameplayEffectAppliedDelegateToSelf` | Direct property | `(UAbilitySystemComponent*, FGameplayEffectSpec, FActiveGameplayEffectHandle)` |
| `OnActiveGameplayEffectAddedDelegateToSelf` | Direct property | Same as above (duration GEs only) |
| `OnGameplayAttributeValueChange` | `GetGameplayAttributeValueChangeDelegate(Attribute)` | `(const FOnAttributeChangeData&)` |

#### Blueprint Node APIs (K2Node_*.h)

| Node Class | Purpose | Key Properties |
|------------|---------|---------------|
| `UK2Node_AddDelegate` | Bind delegate += event | `DelegateReference: FMemberReference` |
| `UK2Node_RemoveDelegate` | Unbind delegate -= event | `DelegateReference: FMemberReference` |
| `UK2Node_CreateDelegate` | Create delegate from function | `SelectedFunctionName: FName` |
| `UK2Node_CustomEvent` | Custom event entry | `CustomFunctionName: FName` |

#### FMemberReference Setup Pattern
```cpp
FMemberReference DelegateRef;
DelegateRef.SetFromField<FMulticastDelegateProperty>(DelegateProperty, bSelfContext, OwnerClass);
// Or for external reference:
DelegateRef.SetExternalMember(PropertyName, OwnerClass);
```

### Scope
- **Files to modify:** `GasAbilityGeneratorTypes.h`, `GasAbilityGeneratorParser.cpp`, `GasAbilityGeneratorGenerators.cpp`
- **Files to add:** None
- **Complexity:** HIGH
- **Risk:** MEDIUM - New K2Node types (AddDelegate, RemoveDelegate, CreateDelegate)
- **Rare edge case:** No - major feature

### Proposed Manifest Syntax

```yaml
gameplay_abilities:
  - name: GA_FatherArmor
    parent_class: NarrativeGameplayAbility
    folder: Abilities/Armor

    # Delegate bindings (auto-wired in ActivateAbility, unbound in EndAbility)
    delegate_bindings:
      - id: BindDamageRedirect
        delegate: OnDamagedBy                    # Property name on source
        source: OwnerASC                         # OwnerASC, PlayerASC, or variable name
        handler: HandleDamageReceived            # Custom Event name (auto-created)
        # Handler signature auto-inferred from delegate

      - id: BindDeath
        delegate: OnDied
        source: FatherASC                        # Reference a Blueprint variable
        handler: HandleFatherDied

    variables:
      - name: FatherASC
        type: Object
        object_class: NarrativeAbilitySystemComponent

    event_graph:
      nodes:
        # Custom events are auto-created from delegate_bindings
        # Parameters match delegate signature
        - id: HandleDamageReceived
          type: CustomEvent
          event_name: HandleDamageReceived
          # Parameters auto-populated: DamagerCauserASC, Damage, Spec

        - id: RedirectDamage
          type: CallFunction
          properties:
            function: ApplyDamageToFather
            # ... damage redirect logic

      connections:
        - from: [HandleDamageReceived, Then]
          to: [RedirectDamage, Exec]
```

### Source Resolution

| Source Value | Resolution | Cast Required |
|--------------|------------|---------------|
| `OwnerASC` | `GetAbilitySystemComponentFromActorInfo(GetOwningActorFromActorInfo())` | **YES** → `UNarrativeAbilitySystemComponent` |
| `PlayerASC` | `GetAbilitySystemComponentFromActorInfo(GetAvatarActorFromActorInfo())` | **YES** → `UNarrativeAbilitySystemComponent` |
| `{VariableName}` | Variable lookup by name | Only if variable type is base `UAbilitySystemComponent` |

**Critical:** `GetAbilitySystemComponentFromActorInfo()` returns `UAbilitySystemComponent*`, but Narrative Pro delegates are on `UNarrativeAbilitySystemComponent`. Cast is mandatory for `OwnerASC`/`PlayerASC` keywords.

### Implementation Phases

#### Phase 1: Type Definitions

```cpp
// In GasAbilityGeneratorTypes.h

// v4.20: Delegate binding definition
struct FManifestDelegateBindingDefinition
{
    FString Id;                      // Unique ID for this binding
    FString Delegate;                // Property name: "OnDied", "OnDamagedBy"
    FString Source;                  // "OwnerASC", "PlayerASC", or variable name
    FString Handler;                 // Custom Event name to create/bind

    uint64 ComputeHash() const
    {
        uint64 Hash = GetTypeHash(Id);
        Hash = HashCombine(Hash, GetTypeHash(Delegate));
        Hash = HashCombine(Hash, GetTypeHash(Source));
        Hash = HashCombine(Hash, GetTypeHash(Handler));
        return Hash;
    }
};

// Add to FManifestGameplayAbilityDefinition:
TArray<FManifestDelegateBindingDefinition> DelegateBindings;
```

#### Phase 2: Parser Support

```cpp
// In GasAbilityGeneratorParser.cpp - ParseGameplayAbilities()

// After parsing other fields, look for delegate_bindings:
if (IsSectionHeader(Lines[LineIndex], TEXT("delegate_bindings")))
{
    ++LineIndex;
    int32 BindingSectionIndent = GetIndentLevel(Lines[LineIndex]);

    while (LineIndex < Lines.Num() && !ShouldExitSection(Lines[LineIndex], BindingSectionIndent))
    {
        if (IsArrayItem(Lines[LineIndex]))
        {
            FManifestDelegateBindingDefinition Binding;
            int32 ItemIndent = GetIndentLevel(Lines[LineIndex]);
            ++LineIndex;

            while (LineIndex < Lines.Num() && GetIndentLevel(Lines[LineIndex]) > ItemIndent)
            {
                FString Key, Value;
                Lines[LineIndex].Split(TEXT(":"), &Key, &Value);
                Key = Key.TrimStartAndEnd();
                Value = StripYamlComment(Value.TrimStartAndEnd());

                if (Key == TEXT("id")) Binding.Id = Value;
                else if (Key == TEXT("delegate")) Binding.Delegate = Value;
                else if (Key == TEXT("source")) Binding.Source = Value;
                else if (Key == TEXT("handler")) Binding.Handler = Value;

                ++LineIndex;
            }

            CurrentAbility.DelegateBindings.Add(Binding);
        }
        else
        {
            ++LineIndex;
        }
    }
}
```

#### Phase 3: Delegate Signature Lookup

```cpp
// Helper: Get delegate signature from property name
static UFunction* GetDelegateSignature(const FString& DelegateName, UClass* SourceClass)
{
    // Find the multicast delegate property
    FMulticastDelegateProperty* DelegateProp = FindFProperty<FMulticastDelegateProperty>(
        SourceClass, FName(*DelegateName));

    if (!DelegateProp)
    {
        return nullptr;
    }

    // Get the signature function
    return DelegateProp->SignatureFunction;
}

// Helper: Create custom event with delegate signature parameters
static UK2Node_CustomEvent* CreateCustomEventFromDelegateSignature(
    UEdGraph* Graph,
    const FString& EventName,
    UFunction* DelegateSignature,
    FVector2D NodePos)
{
    UK2Node_CustomEvent* EventNode = NewObject<UK2Node_CustomEvent>(Graph);
    EventNode->CustomFunctionName = FName(*EventName);
    EventNode->bIsEditable = true;

    Graph->AddNode(EventNode, false, false);
    EventNode->CreateNewGuid();
    EventNode->PostPlacedNewNode();
    EventNode->AllocateDefaultPins();
    EventNode->NodePosX = NodePos.X;
    EventNode->NodePosY = NodePos.Y;

    // Add parameters from delegate signature
    if (DelegateSignature)
    {
        for (TFieldIterator<FProperty> PropIt(DelegateSignature); PropIt; ++PropIt)
        {
            FProperty* Param = *PropIt;
            if (Param->HasAnyPropertyFlags(CPF_Parm) && !Param->HasAnyPropertyFlags(CPF_ReturnParm))
            {
                FEdGraphPinType PinType;
                GetPinTypeFromProperty(Param, PinType);

                EventNode->CreateUserDefinedPin(
                    Param->GetFName(),
                    PinType,
                    EGPD_Output
                );
            }
        }
    }

    return EventNode;
}
```

#### Phase 4: Create Delegate Binding Nodes

**Architecture Decision (Audit-Approved):** Use TWO CreateDelegate nodes per binding - one for ActivateAbility path, one for EndAbility path. Do NOT share delegate outputs across exec paths.

**Node Wiring Specification:**

| Node | Pin | Wire To |
|------|-----|---------|
| `CreateDelegate_A.PN_Self` | Ability Self (handler owner) |
| `CreateDelegate_A.OutputDelegate` | `AddDelegate.Delegate` |
| `AddDelegate.PN_Self` | Source ASC (delegate owner) |
| `CreateDelegate_B.PN_Self` | Ability Self (handler owner) |
| `CreateDelegate_B.OutputDelegate` | `RemoveDelegate.Delegate` |
| `RemoveDelegate.PN_Self` | Source ASC (delegate owner) |

```cpp
// In event graph generation, for each delegate binding:

static void CreateDelegateBindingNodes(
    UBlueprint* Blueprint,
    UEdGraph* EventGraph,
    const FManifestDelegateBindingDefinition& Binding,
    TMap<FString, UK2Node*>& NodeMap,
    FVector2D& CurrentPos)
{
    // 1. Resolve source class for delegate property lookup
    UClass* SourceClass = UNarrativeAbilitySystemComponent::StaticClass();

    // 2. Get delegate signature
    UFunction* DelegateSignature = GetDelegateSignature(Binding.Delegate, SourceClass);
    if (!DelegateSignature)
    {
        UE_LOG(LogGasAbilityGenerator, Error,
            TEXT("[E_DELEGATE_NOT_FOUND] Delegate '%s' not found on %s"),
            *Binding.Delegate, *SourceClass->GetName());
        return;
    }

    // 3. Create Custom Event with matching signature
    UK2Node_CustomEvent* HandlerEvent = FindOrCreateCustomEvent(
        EventGraph, Binding.Handler, DelegateSignature, CurrentPos);
    NodeMap.Add(Binding.Handler, HandlerEvent);
    CurrentPos.Y += 200.0f;

    // 4. Resolve source ASC object (with cast if needed)
    UK2Node* SourceASCNode = nullptr;
    UEdGraphPin* SourceASCPin = nullptr;
    bool bNeedsCast = (Binding.Source == TEXT("OwnerASC") || Binding.Source == TEXT("PlayerASC"));

    if (bNeedsCast)
    {
        // Create GetAbilitySystemComponentFromActorInfo → Cast to UNarrativeAbilitySystemComponent
        UK2Node_CallFunction* GetASCNode = CreateGetASCFromActorInfoNode(EventGraph, Binding.Source, CurrentPos);

        UK2Node_DynamicCast* CastNode = NewObject<UK2Node_DynamicCast>(EventGraph);
        CastNode->TargetType = UNarrativeAbilitySystemComponent::StaticClass();
        EventGraph->AddNode(CastNode, false, false);
        CastNode->CreateNewGuid();
        CastNode->PostPlacedNewNode();
        CastNode->AllocateDefaultPins();

        // Wire GetASC.ReturnValue → Cast.Object
        UEdGraphPin* ASCOutPin = GetASCNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
        UEdGraphPin* CastInPin = CastNode->GetCastSourcePin();
        if (ASCOutPin && CastInPin) ASCOutPin->MakeLinkTo(CastInPin);

        SourceASCNode = CastNode;
        SourceASCPin = CastNode->GetCastResultPin();
        NodeMap.Add(Binding.Id + TEXT("_Cast"), CastNode);
    }
    else
    {
        // Variable lookup - assume already typed correctly
        SourceASCNode = CreateVariableGetNode(EventGraph, Binding.Source, CurrentPos);
        SourceASCPin = SourceASCNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
    }

    // 5. Get Self reference for CreateDelegate nodes
    UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(EventGraph);
    EventGraph->AddNode(SelfNode, false, false);
    SelfNode->CreateNewGuid();
    SelfNode->PostPlacedNewNode();
    SelfNode->AllocateDefaultPins();
    UEdGraphPin* SelfPin = SelfNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
    NodeMap.Add(Binding.Id + TEXT("_Self"), SelfNode);

    // === ACTIVATE PATH ===

    // 6. CreateDelegate_A for ActivateAbility path
    UK2Node_CreateDelegate* CreateDelegateA = NewObject<UK2Node_CreateDelegate>(EventGraph);
    CreateDelegateA->SetFunction(FName(*Binding.Handler));
    EventGraph->AddNode(CreateDelegateA, false, false);
    CreateDelegateA->CreateNewGuid();
    CreateDelegateA->PostPlacedNewNode();
    CreateDelegateA->AllocateDefaultPins();
    NodeMap.Add(Binding.Id + TEXT("_CreateDelegate_A"), CreateDelegateA);

    // Wire CreateDelegate_A.PN_Self → Ability Self
    UEdGraphPin* CreateASelfPin = CreateDelegateA->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input);
    if (SelfPin && CreateASelfPin) SelfPin->MakeLinkTo(CreateASelfPin);

    // 7. AddDelegate node
    UK2Node_AddDelegate* AddDelegateNode = NewObject<UK2Node_AddDelegate>(EventGraph);
    FMemberReference DelegateRef;
    DelegateRef.SetExternalMember(FName(*Binding.Delegate), SourceClass);
    AddDelegateNode->DelegateReference = DelegateRef;
    EventGraph->AddNode(AddDelegateNode, false, false);
    AddDelegateNode->CreateNewGuid();
    AddDelegateNode->PostPlacedNewNode();
    AddDelegateNode->AllocateDefaultPins();
    NodeMap.Add(Binding.Id + TEXT("_AddDelegate"), AddDelegateNode);

    // Wire CreateDelegate_A.OutputDelegate → AddDelegate.Delegate
    UEdGraphPin* DelegateAOut = CreateDelegateA->FindPin(TEXT("OutputDelegate"), EGPD_Output);
    UEdGraphPin* AddDelegateIn = AddDelegateNode->FindPin(TEXT("Delegate"), EGPD_Input);
    if (DelegateAOut && AddDelegateIn) DelegateAOut->MakeLinkTo(AddDelegateIn);

    // Wire SourceASC → AddDelegate.PN_Self (Target)
    UEdGraphPin* AddSelfPin = AddDelegateNode->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input);
    if (SourceASCPin && AddSelfPin) SourceASCPin->MakeLinkTo(AddSelfPin);

    // === END PATH ===

    // 8. CreateDelegate_B for EndAbility path (separate node, same function)
    UK2Node_CreateDelegate* CreateDelegateB = NewObject<UK2Node_CreateDelegate>(EventGraph);
    CreateDelegateB->SetFunction(FName(*Binding.Handler));
    EventGraph->AddNode(CreateDelegateB, false, false);
    CreateDelegateB->CreateNewGuid();
    CreateDelegateB->PostPlacedNewNode();
    CreateDelegateB->AllocateDefaultPins();
    NodeMap.Add(Binding.Id + TEXT("_CreateDelegate_B"), CreateDelegateB);

    // Wire CreateDelegate_B.PN_Self → Ability Self
    UEdGraphPin* CreateBSelfPin = CreateDelegateB->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input);
    if (SelfPin && CreateBSelfPin) SelfPin->MakeLinkTo(CreateBSelfPin);

    // 9. RemoveDelegate node
    UK2Node_RemoveDelegate* RemoveDelegateNode = NewObject<UK2Node_RemoveDelegate>(EventGraph);
    RemoveDelegateNode->DelegateReference = DelegateRef;
    EventGraph->AddNode(RemoveDelegateNode, false, false);
    RemoveDelegateNode->CreateNewGuid();
    RemoveDelegateNode->PostPlacedNewNode();
    RemoveDelegateNode->AllocateDefaultPins();
    NodeMap.Add(Binding.Id + TEXT("_RemoveDelegate"), RemoveDelegateNode);

    // Wire CreateDelegate_B.OutputDelegate → RemoveDelegate.Delegate
    UEdGraphPin* DelegateBOut = CreateDelegateB->FindPin(TEXT("OutputDelegate"), EGPD_Output);
    UEdGraphPin* RemoveDelegateIn = RemoveDelegateNode->FindPin(TEXT("Delegate"), EGPD_Input);
    if (DelegateBOut && RemoveDelegateIn) DelegateBOut->MakeLinkTo(RemoveDelegateIn);

    // Wire SourceASC → RemoveDelegate.PN_Self (Target)
    UEdGraphPin* RemoveSelfPin = RemoveDelegateNode->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input);
    if (SourceASCPin && RemoveSelfPin) SourceASCPin->MakeLinkTo(RemoveSelfPin);

    CurrentPos.Y += 600.0f;
}
```

#### Phase 5: Wire into Ability Lifecycle

```cpp
// In FGameplayAbilityGenerator::Generate(), after creating event graph nodes:

// Find or create ActivateAbility event
UK2Node_Event* ActivateEvent = FindOrCreateEventNode(EventGraph, TEXT("K2_ActivateAbility"));

// Find or create EndAbility event
UK2Node_Event* EndAbilityEvent = FindOrCreateEventNode(EventGraph, TEXT("K2_OnEndAbility"));

// Chain delegate bind nodes after ActivateAbility
UEdGraphPin* LastExecPin = ActivateEvent->FindPin(UEdGraphSchema_K2::PN_Then);

for (const FManifestDelegateBindingDefinition& Binding : Definition.DelegateBindings)
{
    UK2Node_AddDelegate* AddNode = Cast<UK2Node_AddDelegate>(
        NodeMap.FindRef(Binding.Id + TEXT("_AddDelegate")));

    if (AddNode && LastExecPin)
    {
        UEdGraphPin* AddExecIn = AddNode->FindPin(UEdGraphSchema_K2::PN_Execute);
        LastExecPin->MakeLinkTo(AddExecIn);
        LastExecPin = AddNode->FindPin(UEdGraphSchema_K2::PN_Then);
    }
}

// Chain delegate unbind nodes in EndAbility
LastExecPin = EndAbilityEvent->FindPin(UEdGraphSchema_K2::PN_Then);

for (const FManifestDelegateBindingDefinition& Binding : Definition.DelegateBindings)
{
    UK2Node_RemoveDelegate* RemoveNode = Cast<UK2Node_RemoveDelegate>(
        NodeMap.FindRef(Binding.Id + TEXT("_RemoveDelegate")));

    if (RemoveNode && LastExecPin)
    {
        UEdGraphPin* RemoveExecIn = RemoveNode->FindPin(UEdGraphSchema_K2::PN_Execute);
        LastExecPin->MakeLinkTo(RemoveExecIn);
        LastExecPin = RemoveNode->FindPin(UEdGraphSchema_K2::PN_Then);
    }
}
```

### Known Delegate Property Whitelist

| Delegate Name | Source Class | Signature Function |
|---------------|--------------|-------------------|
| `OnDied` | `UNarrativeAbilitySystemComponent` | `FOnDied__DelegateSignature` |
| `OnDamagedBy` | `UNarrativeAbilitySystemComponent` | `FOnDamagedBy__DelegateSignature` |
| `OnHealedBy` | `UNarrativeAbilitySystemComponent` | `FOnHealedBy__DelegateSignature` |
| `OnDealtDamage` | `UNarrativeAbilitySystemComponent` | `FOnDealtDamage__DelegateSignature` |
| `OnGameplayEffectAppliedDelegateToSelf` | `UAbilitySystemComponent` | ASC-specific |

### Error Codes

| Code | Condition | Severity |
|------|-----------|----------|
| `E_DELEGATE_NOT_FOUND` | Delegate property not found on source class | FAIL |
| `E_DELEGATE_HANDLER_EXISTS` | Handler event name already exists (conflict) | FAIL |
| `E_DELEGATE_SOURCE_INVALID` | Source variable not found or wrong type | FAIL |
| `E_DELEGATE_SIGNATURE_MISMATCH` | Handler params don't match delegate signature | FAIL |
| `E_DELEGATE_SELF_PIN_MISSING` | Cannot wire PN_Self pin on delegate node | FAIL |
| `E_DELEGATE_CAST_FAILED` | Cast to UNarrativeAbilitySystemComponent failed | FAIL |

**Severity Rationale:** Signature mismatch is FAIL (not WARN) because Blueprint compilation will fail anyway. Early detection provides clearer error messages.

### Breadcrumb Logging

```
GA_BP[GA_FatherArmor] DELEGATE_BIND id=BindDamageRedirect delegate=OnDamagedBy handler=HandleDamageReceived OK
GA_BP[GA_FatherArmor] DELEGATE_BIND id=BindDeath delegate=OnDied handler=HandleFatherDied OK
GA_BP[GA_FatherArmor] DELEGATE_WIRE ActivateAbility → AddDelegate[2] OK
GA_BP[GA_FatherArmor] DELEGATE_WIRE EndAbility → RemoveDelegate[2] OK
```

### Test Cases

| # | Test | Expected |
|---|------|----------|
| 1 | Bind OnDied to custom event | AddDelegate node created, wired to ActivateAbility |
| 2 | Unbind in EndAbility | RemoveDelegate node created, wired to EndAbility |
| 3 | Handler signature matches delegate | Custom event has correct parameter pins |
| 4 | OwnerASC source resolution | GetAbilitySystemComponentFromActorInfo chain created |
| 5 | Variable source resolution | Variable get node created and connected |
| 6 | Multiple bindings | All bindings chained in order |
| 7 | Invalid delegate name | `[E_DELEGATE_NOT_FOUND]` error |
| 8 | Duplicate handler name | `[E_DELEGATE_HANDLER_EXISTS]` error |
| 9 | Compile with bindings | Blueprint compiles successfully |
| 10 | Runtime: delegate fires | Custom event executes when delegate broadcasts |

### Dependencies

Requires module additions to `GasAbilityGenerator.Build.cs`:
- Already present: `BlueprintGraph`, `Kismet`, `KismetCompiler`
- **No new dependencies required** - K2Node_AddDelegate/RemoveDelegate/CreateDelegate are in BlueprintGraph

### Acceptance Criteria

- [ ] Delegate bindings parsed from manifest
- [ ] Custom events created with correct delegate signature parameters
- [ ] UK2Node_CreateDelegate created and configured
- [ ] UK2Node_AddDelegate created and wired to ActivateAbility
- [ ] UK2Node_RemoveDelegate created and wired to EndAbility
- [ ] Source resolution works for OwnerASC, PlayerASC, and variables
- [ ] Known delegate whitelist validated
- [ ] Error codes emitted for invalid configurations
- [ ] Breadcrumb logs emitted for all binding operations
- [ ] All 10 test cases pass
- [ ] Blueprint compiles without errors

### Risk Mitigation

| Risk | Mitigation |
|------|------------|
| K2Node APIs undocumented | Validated against UE5.7 source: `K2Node_BaseMCDelegate.h`, `K2Node_CreateDelegate.h` |
| Delegate signature mismatch | Use `GetDelegateSignature()` from FMulticastDelegateProperty |
| Pin connection failures | Use existing `TryCreateConnection` pattern with Schema validation |
| Compile failures | Contract 10 (FCompilerResultsLog) catches all compile errors |

### References

**Web Resources:**
- [Custom K2 Nodes with Delegates](https://forums.unrealengine.com/t/custom-k2-nodes-with-delegates/457550)
- [GAS Documentation - Delegate Examples](https://github.com/tranek/GASDocumentation)
- [K2Node Introduction](https://olssondev.github.io/2023-02-13-K2Nodes/)
- [FMemberReference API](https://docs.unrealengine.com/5.2/en-US/API/Runtime/Engine/Engine/FMemberReference/)

**Engine Source:**
- `Engine/Source/Editor/BlueprintGraph/Classes/K2Node_BaseMCDelegate.h`
- `Engine/Source/Editor/BlueprintGraph/Classes/K2Node_AddDelegate.h`
- `Engine/Source/Editor/BlueprintGraph/Classes/K2Node_CreateDelegate.h`
- `Engine/Source/Editor/BlueprintGraph/Private/K2Node_MCDelegate.cpp`
- `Engine/Source/Runtime/Engine/Classes/Engine/MemberReference.h`

**Existing Generator Patterns:**
- `GasAbilityGeneratorGenerators.cpp:9800-10058` - CreateEventNode, CreateCallFunctionNode
- `GasAbilityGeneratorGenerators.cpp:12251` - Delegate binding complexity note

---

## Summary Matrix

| # | Task | Complexity | Risk | Edge Case | Files Changed | Status |
|---|------|------------|------|-----------|---------------|--------|
| 1 | P1.2 Transition Validation | LOW | NONE | No | 1 | ✅ v4.18 |
| 2 | P1.3 Startup Validation | MEDIUM | NONE | No | 1 | Pending |
| 3 | Circular Dep Detection | MEDIUM | LOW | Moderate | 1-2 | ✅ v4.17 |
| 4 | FormState Preset Schema | MEDIUM | LOW | No | 2 | Pending |
| 5 | Material Circular Refs | MEDIUM | LOW | Moderate | 1 | Pending |
| 6 | (moved to Section 9) | - | - | - | - | - |
| 7 | Split Generators.cpp | HIGH | HIGH | No | ~17 | Pending |
| 8 | ActorComponent Generator | HIGH | MEDIUM | No | 3 | ✅ v4.19 |
| 9 | Delegate Binding Automation | HIGH | MEDIUM | No | 3 | ✅ Approved |

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
