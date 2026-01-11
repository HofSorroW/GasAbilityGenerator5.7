# Blueprint Node and Connection Master Reference v2.1

## PURPOSE

This document catalogs every Blueprint node type, pin configuration, and valid connection pattern used across Father Companion implementation guides. Use this reference to validate that suggested Blueprint connections are buildable without pin mismatches or errors. This version incorporates verified information from Narrative Pro v2.2 source code and Unreal Engine 5.6 documentation.

---

## DOCUMENT INFORMATION

| Field | Value |
|-------|-------|
| Version | 2.1 |
| Engine Version | Unreal Engine 5.6 |
| Plugin Version | Narrative Pro v2.2 |
| Generator Plugin | GasAbilityGenerator v2.5.4+ |
| Last Updated | January 2026 |
| Purpose | Blueprint node validation, pin configuration reference, programmatic generation support |

---

## VERSION 2.1 CHANGES

| Change | Details |
|--------|---------|
| Branch Pin Names | Corrected from True/False to then/else (internal UE5 names) |
| DynamicCast Requirements | Added Section 5.5 documenting NotifyPinConnectionListChanged requirement |
| Programmatic Validation | Added Section 19.4 with asset generator-specific validation checklist |
| Generator Plugin Support | Added GasAbilityGenerator v2.5.4 compatibility notes |

---

## VERSION 2.0 CHANGES

| Change | Details |
|--------|---------|
| Source Code Verification | All nodes verified against Narrative Pro v2.2 C++ source |
| Narrative Pro Nodes | Added Section 4 with all Narrative Pro specific Blueprint nodes |
| AbilityTask Nodes | Added detailed AbilityTask_SpawnProjectile configuration |
| Pin Type Corrections | Corrected several pin type specifications based on source |
| Connection Chains | Added verified connection chains from working implementations |
| Pure vs Callable | Clarified which nodes have execution pins vs pure data nodes |

---

## TABLE OF CONTENTS

1. [Node Type Definitions](#section-1-node-type-definitions)
2. [Event Nodes](#section-2-event-nodes)
3. [Gameplay Ability System Nodes](#section-3-gameplay-ability-system-nodes)
4. [Narrative Pro Specific Nodes](#section-4-narrative-pro-specific-nodes)
5. [Cast Nodes](#section-5-cast-nodes)
6. [Flow Control Nodes](#section-6-flow-control-nodes)
7. [Variable Nodes](#section-7-variable-nodes)
8. [Actor and Component Nodes](#section-8-actor-and-component-nodes)
9. [Math and Utility Nodes](#section-9-math-and-utility-nodes)
10. [Timer Nodes](#section-10-timer-nodes)
11. [Collision and Overlap Nodes](#section-11-collision-and-overlap-nodes)
12. [AbilityTask Nodes](#section-12-abilitytask-nodes)
13. [Delegate and Event Binding](#section-13-delegate-and-event-binding)
14. [Movement Nodes](#section-14-movement-nodes)
15. [Animation Nodes](#section-15-animation-nodes)
16. [Effect and Damage Nodes](#section-16-effect-and-damage-nodes)
17. [Common Connection Chains](#section-17-common-connection-chains)
18. [Pin Type Compatibility Rules](#section-18-pin-type-compatibility-rules)
19. [Validation Checklist](#section-19-validation-checklist)
20. [Research Validation Items](#section-20-research-validation-items)

---

## SECTION 1: NODE TYPE DEFINITIONS

### 1.1) Node Categories

| Category | Execution Pins | Description |
|----------|----------------|-------------|
| Event | Output only | Entry points for execution flow |
| Function Call | Input and Output | Execute functions on objects |
| Pure Function | None | Data-only nodes, no execution flow |
| Cast | Input and Output | Type conversion with validation |
| Flow Control | Input and Output | Branch, loop, sequence nodes |
| Variable GET | None | Pure function returning variable value |
| Variable SET | Input and Output | Sets variable and passes execution |
| Async Task | Input and Multiple Outputs | Latent operations with delegate pins |
| Macro | Input and Output | Expanded inline, may have multiple outputs |

### 1.2) Pin Types

| Pin Color | Type | Example Values |
|-----------|------|----------------|
| White | Execution | Flow control |
| Blue | Object Reference | Actor, Component references |
| Light Blue | Soft Object Reference | TSoftObjectPtr, TSoftClassPtr |
| Green | Float | 1.0, 300.0 |
| Cyan | Integer | 0, 1, 100 |
| Red | Boolean | true, false |
| Yellow | String | "SocketName" |
| Purple | Name | FName values |
| Magenta | Struct | FVector, FTransform, FGameplayTag |
| Orange | Enum | EAttachmentRule, ETeamAttitude |
| Dark Cyan | Byte | 0-255 |
| Pink | Text | FText values |

### 1.3) Execution Pin Rules

| Rule | Description |
|------|-------------|
| White to White only | Execution pins only connect to execution pins |
| Direction matters | Output exec connects to input exec |
| Multiple outputs allowed | Branch then/else (editor shows True/False), Sequence Then 0/1/2 |
| Single input per node | Each node has one exec input |
| Pure functions have no exec | Data flows without execution control |

---

## SECTION 2: EVENT NODES

### 2.1) Event ActivateAbility

| Property | Value |
|----------|-------|
| Node Class | UK2Node_Event |
| Category | Gameplay Ability Events |
| Execution Pins | Then (out) |
| Data Pins Output | Actor Info (FGameplayAbilityActorInfo) |

Connection Pattern:

| From Pin | To Node | To Pin |
|----------|---------|--------|
| Then | Any function node | Exec In |
| Actor Info | Get Avatar Actor From Actor Info | Actor Info |
| Actor Info | Get Ability System Component From Actor Info | Actor Info |

### 2.2) Event OnEndAbility

| Property | Value |
|----------|-------|
| Node Class | UK2Node_Event |
| Category | Gameplay Ability Events |
| Execution Pins | Then (out) |
| Data Pins Input | Was Cancelled (bool) |

Connection Pattern:

| From Pin | To Node | To Pin |
|----------|---------|--------|
| Then | Cleanup logic | Exec In |
| Was Cancelled | Branch | Condition |

### 2.3) Event BeginPlay

| Property | Value |
|----------|-------|
| Node Class | UK2Node_Event |
| Category | Actor Events |
| Execution Pins | Then (out) |
| Data Pins | None |

### 2.4) Event EndPlay

| Property | Value |
|----------|-------|
| Node Class | UK2Node_Event |
| Category | Actor Events |
| Execution Pins | Then (out) |
| Data Pins Input | End Play Reason (EEndPlayReason) |

### 2.5) Custom Event

| Property | Value |
|----------|-------|
| Node Class | UK2Node_CustomEvent |
| Category | User-defined |
| Execution Pins | Then (out) |
| Data Pins | User-defined inputs |
| Replication | Can be set to Run on Server/Client/All |

---

## SECTION 3: GAMEPLAY ABILITY SYSTEM NODES

### 3.1) Get Avatar Actor From Actor Info

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Actor Info (FGameplayAbilityActorInfo) |
| Output Pins | Return Value (AActor*) |

Connection Pattern:

| From Pin | To Node | To Pin |
|----------|---------|--------|
| Return Value | Cast To BP_FatherCompanion | Object |
| Return Value | Cast To ANarrativeNPCCharacter | Object |
| Return Value | Get Ability System Component | Actor |

### 3.2) Get Ability System Component (from Actor)

| Property | Value |
|----------|-------|
| Node Type | Pure Function (UAbilitySystemBlueprintLibrary) |
| Execution Pins | None |
| Input Pins | Actor (AActor*) |
| Output Pins | Return Value (UAbilitySystemComponent*) |

Connection Pattern:

| From Pin | To Node | To Pin |
|----------|---------|--------|
| Return Value | Make Outgoing Spec | Target |
| Return Value | Apply Gameplay Effect Spec to Self | Target |
| Return Value | Apply Gameplay Effect Spec to Target | Target |
| Return Value | Give Ability | Target |
| Return Value | Cancel Ability | Target |
| Return Value | Has Matching Gameplay Tag | Target |
| Return Value | Add Loose Gameplay Tag | Target |
| Return Value | Remove Loose Gameplay Tag | Target |

### 3.3) Get Ability System Component From Actor Info

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Actor Info (FGameplayAbilityActorInfo) |
| Output Pins | Return Value (UAbilitySystemComponent*) |

### 3.4) Make Outgoing Spec

| Property | Value |
|----------|-------|
| Node Type | Function (has execution pins) |
| Display Name | Make Outgoing Gameplay Effect Spec |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UAbilitySystemComponent*), Gameplay Effect Class (TSubclassOf UGameplayEffect), Level (float, default 1.0) |
| Output Pins | Return Value (FGameplayEffectSpecHandle) |

Connection Pattern:

| From Pin | To Node | To Pin |
|----------|---------|--------|
| Return Value | Assign Tag Set By Caller Magnitude | Spec Handle |
| Exec Out | Assign Tag Set By Caller Magnitude | Exec In |

### 3.5) Assign Tag Set By Caller Magnitude

| Property | Value |
|----------|-------|
| Node Type | Function |
| Display Name | Assign Tag Set By Caller Magnitude |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Spec Handle (FGameplayEffectSpecHandle), Data Tag (FGameplayTag), Magnitude (float) |
| Output Pins | Return Value (FGameplayEffectSpecHandle) |

Connection Pattern:

| From Pin | To Node | To Pin |
|----------|---------|--------|
| Return Value | Apply Gameplay Effect Spec to Target | Spec Handle |
| Return Value | Apply Gameplay Effect Spec to Self | Spec Handle |
| Exec Out | Apply Gameplay Effect Spec to Target | Exec In |

### 3.6) Apply Gameplay Effect Spec to Target

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UAbilitySystemComponent*), Spec Handle (FGameplayEffectSpecHandle) |
| Output Pins | Return Value (FActiveGameplayEffectHandle) |

Connection Pattern:

| From Pin | To Node | To Pin |
|----------|---------|--------|
| Return Value | SET EffectHandle variable | Value |
| Return Value | Is Valid (struct) | Structure |

### 3.7) Apply Gameplay Effect Spec to Self

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UAbilitySystemComponent*), Spec Handle (FGameplayEffectSpecHandle) |
| Output Pins | Return Value (FActiveGameplayEffectHandle) |

### 3.8) Apply Gameplay Effect to Self

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UAbilitySystemComponent*), Gameplay Effect Class (TSubclassOf UGameplayEffect), Level (float) |
| Output Pins | Return Value (FActiveGameplayEffectHandle) |

### 3.9) Apply Gameplay Effect to Owner

| Property | Value |
|----------|-------|
| Node Type | Function (GameplayAbility context) |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Gameplay Effect Class (TSubclassOf UGameplayEffect) |
| Output Pins | Return Value (FActiveGameplayEffectHandle) |
| Context | Only available inside GameplayAbility graphs |

### 3.10) Remove Active Gameplay Effect

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UAbilitySystemComponent*), Handle (FActiveGameplayEffectHandle), Stacks to Remove (int32, default -1 for all) |
| Output Pins | Return Value (bool) |

### 3.11) Remove Active Gameplay Effects with Granted Tags

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UAbilitySystemComponent*), Tags (FGameplayTagContainer) |
| Output Pins | None |

### 3.12) Give Ability

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UAbilitySystemComponent*), Ability Class (TSubclassOf UGameplayAbility), Level (int32), Input ID (int32, deprecated) |
| Output Pins | Return Value (FGameplayAbilitySpecHandle) |

### 3.13) Clear Ability

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UAbilitySystemComponent*), Handle (FGameplayAbilitySpecHandle) |
| Output Pins | None |

### 3.14) Cancel Ability

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UAbilitySystemComponent*), Handle (FGameplayAbilitySpecHandle) |
| Output Pins | None |

### 3.15) Try Activate Ability by Class

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UAbilitySystemComponent*), Ability Class (TSubclassOf UGameplayAbility) |
| Output Pins | Return Value (bool) |

### 3.16) Has Matching Gameplay Tag

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Target (IGameplayTagAssetInterface or UAbilitySystemComponent*), Tag to Check (FGameplayTag) |
| Output Pins | Return Value (bool) |

Connection Pattern:

| From Pin | To Node | To Pin |
|----------|---------|--------|
| Return Value | Branch | Condition |

### 3.17) Add Loose Gameplay Tag

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UAbilitySystemComponent*), Gameplay Tag (FGameplayTag) |
| Output Pins | None |

### 3.18) Remove Loose Gameplay Tag

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UAbilitySystemComponent*), Gameplay Tag (FGameplayTag) |
| Output Pins | None |

### 3.19) Commit Ability

| Property | Value |
|----------|-------|
| Node Type | Function (GameplayAbility context) |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | None (uses owning ability context) |
| Output Pins | Return Value (bool) |

### 3.20) Commit Ability Cooldown

| Property | Value |
|----------|-------|
| Node Type | Function (GameplayAbility context) |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | None (uses owning ability context) |
| Output Pins | Return Value (bool) |

### 3.21) End Ability

| Property | Value |
|----------|-------|
| Node Type | Function (GameplayAbility context) |
| Execution Pins | Exec (in) only |
| Input Pins | Was Cancelled (bool, default false) |
| Output Pins | None |
| Note | Terminal node - no output execution |

### 3.22) Get Numeric Attribute

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Target (UAbilitySystemComponent*), Attribute (FGameplayAttribute) |
| Output Pins | Return Value (float) |

### 3.23) Make Gameplay Tag Container from Tag

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Single Tag (FGameplayTag) |
| Output Pins | Return Value (FGameplayTagContainer) |

---

## SECTION 4: NARRATIVE PRO SPECIFIC NODES

### 4.1) NarrativeGameplayAbility Nodes

These nodes are available within any GameplayAbility derived from UNarrativeGameplayAbility.

#### 4.1.1) GetOwningController

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeGameplayAbility.h line 40 |
| Execution Pins | None |
| Input Pins | None (self reference) |
| Output Pins | Return Value (AController*) |

#### 4.1.2) GetOwningNarrativeCharacter

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeGameplayAbility.h line 43 |
| Execution Pins | None |
| Input Pins | None (self reference) |
| Output Pins | Return Value (ANarrativeCharacter*) |

#### 4.1.3) GetOwningNarrativeCharacterVisual

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeGameplayAbility.h line 46 |
| Execution Pins | None |
| Input Pins | None (self reference) |
| Output Pins | Return Value (ANarrativeCharacterVisual*) |

#### 4.1.4) GetOwnerEquippedWeapon

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeGameplayAbility.h line 49 |
| Execution Pins | None |
| Input Pins | bMainhand (bool, default true) |
| Output Pins | Return Value (UWeaponItem*) |

#### 4.1.5) IsBot

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeGameplayAbility.h line 53 |
| Execution Pins | None |
| Input Pins | None (self reference) |
| Output Pins | Return Value (bool) |

### 4.2) NarrativeAbilitySystemComponent Nodes

#### 4.2.1) GetAvatarOwner

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeAbilitySystemComponent.h line 99 |
| Execution Pins | None |
| Input Pins | Target (UNarrativeAbilitySystemComponent*) |
| Output Pins | Return Value (AActor*) |

#### 4.2.2) GetCharacterOwner

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeAbilitySystemComponent.h line 104 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UNarrativeAbilitySystemComponent*) |
| Output Pins | Return Value (ANarrativeCharacter*) |

#### 4.2.3) AbilityInputTagPressed

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeAbilitySystemComponent.h line 108 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UNarrativeAbilitySystemComponent*), Input Tag (FGameplayTag) |
| Output Pins | None |

#### 4.2.4) AbilityInputTagReleased

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeAbilitySystemComponent.h line 111 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UNarrativeAbilitySystemComponent*), Input Tag (FGameplayTag) |
| Output Pins | None |

#### 4.2.5) FindAbilitiesWithTag

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeAbilitySystemComponent.h line 116 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UNarrativeAbilitySystemComponent*), Input Tag (FGameplayTag) |
| Output Pins | Out Ability Specs (TArray of FGameplayAbilitySpecHandle) |

#### 4.2.6) IsDead

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeAbilitySystemComponent.h line 153 |
| Execution Pins | None |
| Input Pins | Target (UNarrativeAbilitySystemComponent*) |
| Output Pins | Return Value (bool) |

#### 4.2.7) AddDynamicTagsGameplayEffect

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeAbilitySystemComponent.h line 178 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UNarrativeAbilitySystemComponent*), Tags To Add (FGameplayTagContainer) |
| Output Pins | Return Value (FActiveGameplayEffectHandle) |

#### 4.2.8) ApplyGameplayEffectSpecToTargetData

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeAbilitySystemComponent.h line 184 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UNarrativeAbilitySystemComponent*), Spec Handle (FGameplayEffectSpecHandle), Target Data (FGameplayAbilityTargetDataHandle) |
| Output Pins | Return Value (TArray of FActiveGameplayEffectHandle) |

### 4.3) ArsenalStatics Nodes

#### 4.3.1) GetAttitude

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | ArsenalStatics.h line 81 |
| Execution Pins | None |
| Input Pins | Test Actor (AActor*), Target (AActor*) |
| Output Pins | Return Value (ETeamAttitude::Type) |

Connection Pattern:

| From Pin | To Node | To Pin |
|----------|---------|--------|
| Return Value | Switch on ETeamAttitude | Selection |
| Return Value | Equal (Enum) | A |

#### 4.3.2) IsSameTeam

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | ArsenalStatics.h line 78 |
| Execution Pins | None |
| Input Pins | Test Actor (AActor*), Target (AActor*) |
| Output Pins | Return Value (bool) |

#### 4.3.3) GetActorFactions

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | ArsenalStatics.h line 85 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Actor (AActor*) |
| Output Pins | Return Value (FGameplayTagContainer) |

#### 4.3.4) AddFactionsToActor

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | ArsenalStatics.h line 89 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Actor (AActor*), Factions (FGameplayTagContainer) |
| Output Pins | None |

#### 4.3.5) RemoveFactionsFromActor

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | ArsenalStatics.h line 93 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Actor (AActor*), Factions (FGameplayTagContainer) |
| Output Pins | None |

#### 4.3.6) GetNarrativePlayerCharacter

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | ArsenalStatics.h line 443 |
| Execution Pins | None |
| Input Pins | World Context Object (UObject*), Player Index (int32) |
| Output Pins | Return Value (ANarrativePlayerCharacter*) |

### 4.4) NarrativeCharacter Nodes

#### 4.4.1) AddAbility

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeCharacter.h line 547 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (ANarrativeCharacter*), Ability (TSubclassOf UNarrativeGameplayAbility), Source Object (UObject*, optional) |
| Output Pins | Return Value (FGameplayAbilitySpecHandle) |

#### 4.4.2) GrantAbilities

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeCharacter.h line 551 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (ANarrativeCharacter*), Abilities (TArray of TSubclassOf UNarrativeGameplayAbility), Source Object (UObject*, optional) |
| Output Pins | Return Value (TArray of FGameplayAbilitySpecHandle) |

#### 4.4.3) RemoveAbilities

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeCharacter.h line 555 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (ANarrativeCharacter*), Abilities (TArray of FGameplayAbilitySpecHandle) |
| Output Pins | None |

#### 4.4.4) GetNarrativeCharacterMovement

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeCharacter.h line 577 |
| Execution Pins | None |
| Input Pins | Target (ANarrativeCharacter*) |
| Output Pins | Return Value (UNarrativeCharacterMovement*) |

#### 4.4.5) GetEquipmentComponent

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeCharacter.h line 567 |
| Execution Pins | None |
| Input Pins | Target (ANarrativeCharacter*) |
| Output Pins | Return Value (UEquipmentComponent*) |

#### 4.4.6) GetCharacterVisual

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeCharacter.h line 561 |
| Execution Pins | None |
| Input Pins | Target (ANarrativeCharacter*) |
| Output Pins | Return Value (ANarrativeCharacterVisual*) |

#### 4.4.7) SetRagdoll

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeCharacter.h line 631 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (ANarrativeCharacter*), bWantsRagdoll (bool) |
| Output Pins | None |

#### 4.4.8) RagdollForDuration

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeCharacter.h line 635 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (ANarrativeCharacter*), Duration (float) |
| Output Pins | None |

### 4.5) NarrativeNPCCharacter Nodes

#### 4.5.1) GetNPCDefinition

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeNPCCharacter.h line 326 |
| Execution Pins | None |
| Input Pins | Target (ANarrativeNPCCharacter*) |
| Output Pins | Return Value (UNPCDefinition*) |

#### 4.5.2) GetNPCName

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeNPCCharacter.h line 329 |
| Execution Pins | None |
| Input Pins | Target (ANarrativeNPCCharacter*) |
| Output Pins | Return Value (FText) |

#### 4.5.3) PlayTaggedDialogue

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeNPCCharacter.h line 338 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (ANarrativeNPCCharacter*), Tag (FGameplayTag), Dialogue Instigator (AActor*) |
| Output Pins | None |

#### 4.5.4) GetActivityComponent

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeNPCCharacter.h line 345 |
| Execution Pins | None |
| Input Pins | Target (ANarrativeNPCCharacter*) |
| Output Pins | Return Value (UNPCActivityComponent*) |

#### 4.5.5) GetNPCController

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeNPCCharacter.h line 348 |
| Execution Pins | None |
| Input Pins | Target (ANarrativeNPCCharacter*) |
| Output Pins | Return Value (ANarrativeNPCController*) |

### 4.6) NarrativeNPCController Nodes

#### 4.6.1) GetControlledNPC

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeNPCController.h line 74 |
| Execution Pins | None |
| Input Pins | Target (ANarrativeNPCController*) |
| Output Pins | Return Value (ANarrativeNPCCharacter*) |

#### 4.6.2) GetOwnedNPC

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeNPCController.h line 77 |
| Execution Pins | None |
| Input Pins | Target (ANarrativeNPCController*) |
| Output Pins | Return Value (ANarrativeNPCCharacter*) |

#### 4.6.3) IsAlive

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeNPCController.h line 70 |
| Execution Pins | None |
| Input Pins | Target (ANarrativeNPCController*) |
| Output Pins | Return Value (bool) |

#### 4.6.4) RequestAttackToken

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeNPCController.h line 91 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (ANarrativeNPCController*), Target To Attack (UNarrativeAbilitySystemComponent*) |
| Output Pins | Return Value (bool) |

#### 4.6.5) ReturnToken

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeNPCController.h line 95 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (ANarrativeNPCController*) |
| Output Pins | Return Value (bool) |

### 4.7) NarrativeProjectile Nodes

#### 4.7.1) GetNarrativeCharacter

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeProjectile.h line 22 |
| Execution Pins | None |
| Input Pins | Target (ANarrativeProjectile*) |
| Output Pins | Return Value (ANarrativeCharacter*) |

#### 4.7.2) SetProjectileTargetData

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeProjectile.h line 32 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (ANarrativeProjectile*), Target Handle (FGameplayAbilityTargetDataHandle) |
| Output Pins | None |

### 4.8) NarrativeCombatAbility Nodes

#### 4.8.1) GenerateTargetDataUsingTrace

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeCombatAbility.h line 136 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Trace Data (FCombatTraceData), Trace Start (FTransform), Application Tag (FGameplayTag, optional) |
| Output Pins | None |

#### 4.8.2) GetTargetDataUsingTrace

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeCombatAbility.h line 140 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Trace Data (FCombatTraceData), Trace Start (FTransform) |
| Output Pins | Return Value (FGameplayAbilityTargetDataHandle) |

#### 4.8.3) FinalizeTargetData

| Property | Value |
|----------|-------|
| Node Type | Function |
| Source | NarrativeCombatAbility.h line 145 |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target Data (FGameplayAbilityTargetDataHandle), Application Tag (FGameplayTag) |
| Output Pins | None |

#### 4.8.4) GetAttackDamage

| Property | Value |
|----------|-------|
| Node Type | Pure Function (BlueprintNativeEvent) |
| Source | NarrativeCombatAbility.h line 157 |
| Execution Pins | None |
| Input Pins | None (self reference) |
| Output Pins | Return Value (float) |

#### 4.8.5) GetAbilityWeapon

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeCombatAbility.h line 204 |
| Execution Pins | None |
| Input Pins | None (self reference) |
| Output Pins | Return Value (UWeaponItem*) |

#### 4.8.6) GetAbilityWeaponVisual

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Source | NarrativeCombatAbility.h line 213 |
| Execution Pins | None |
| Input Pins | None (self reference) |
| Output Pins | Return Value (AWeaponVisual*) |

---

## SECTION 5: CAST NODES

### 5.1) Cast To BP_FatherCompanion

| Property | Value |
|----------|-------|
| Node Type | Cast |
| Execution Pins | Exec (in), Success (out), Failed (out) |
| Input Pins | Object (UObject*) |
| Output Pins | As BP Father Companion (BP_FatherCompanion*) |

Connection Pattern:

| From Pin | To Node | To Pin |
|----------|---------|--------|
| As BP Father Companion | Get Owner Player | Target |
| As BP Father Companion | SET FatherRef | Value |

### 5.2) Cast To ANarrativeNPCCharacter

| Property | Value |
|----------|-------|
| Node Type | Cast |
| Execution Pins | Exec (in), Success (out), Failed (out) |
| Input Pins | Object (UObject*) |
| Output Pins | As Narrative NPC Character (ANarrativeNPCCharacter*) |

### 5.3) Cast To ANarrativePlayerCharacter

| Property | Value |
|----------|-------|
| Node Type | Cast |
| Execution Pins | Exec (in), Success (out), Failed (out) |
| Input Pins | Object (UObject*) |
| Output Pins | As Narrative Player Character (ANarrativePlayerCharacter*) |

### 5.4) Cast To Character

| Property | Value |
|----------|-------|
| Node Type | Cast |
| Execution Pins | Exec (in), Success (out), Failed (out) |
| Input Pins | Object (UObject*) |
| Output Pins | As Character (ACharacter*) |

Connection Pattern:

| From Pin | To Node | To Pin |
|----------|---------|--------|
| As Character | Get Character Movement | Target |
| As Character | Launch Character | Target |
| As Character | Get Mesh | Target |

### 5.5) Programmatic Cast Node Connections (Asset Generator Note)

**CRITICAL: DynamicCast Object Pin Connection Requirements**

When connecting pins programmatically (e.g., using `MakeLinkTo()` in C++), DynamicCast nodes require special handling:

| Step | Action | Reason |
|------|--------|--------|
| 1 | Call `MakeLinkTo()` to create the connection | Standard pin linking |
| 2 | Call `NotifyPinConnectionListChanged()` on BOTH nodes | Required for Cast node to update internal state |

**Why This Matters:**
The `MakeLinkTo()` function adds a connection to the pin's LinkedTo array but does NOT trigger the node update callbacks that UE normally calls when making connections in the Blueprint editor. For DynamicCast nodes (UK2Node_DynamicCast), this prevents the Cast node from recognizing what type is connected to its Object input pin.

**Error Without NotifyPinConnectionListChanged:**
```
[Compiler] The type of Object is undetermined. Connect something to Cast To [ClassName] to imply a specific type.
```

**Correct C++ Pattern:**
```cpp
// After MakeLinkTo() call:
FromNode->NotifyPinConnectionListChanged(FromPin);
ToNode->NotifyPinConnectionListChanged(ToPin);
```

This applies to all cast node types:
- Cast To BP_FatherCompanion
- Cast To ANarrativeNPCCharacter
- Cast To ANarrativePlayerCharacter
- Cast To Character
- Any custom cast nodes

---

## SECTION 6: FLOW CONTROL NODES

### 6.1) Branch

| Property | Value |
|----------|-------|
| Node Type | Flow Control |
| Node Class | UK2Node_IfThenElse |
| Execution Pins | Exec (in), then (out), else (out) |
| Input Pins | Condition (bool) |
| Output Pins | None |

**IMPORTANT - Pin Name Aliases:**
| Common Name | Actual Pin Name | Notes |
|-------------|-----------------|-------|
| True | then | UE5 internally uses lowercase `then` |
| False | else | UE5 internally uses lowercase `else` |

When connecting programmatically (e.g., via asset generator plugins), use `then` and `else` as pin names. The Blueprint editor displays these as "True" and "False" but the underlying UK2Node_IfThenElse uses `then`/`else`.

### 6.2) Sequence

| Property | Value |
|----------|-------|
| Node Type | Flow Control |
| Execution Pins | Exec (in), Then 0 (out), Then 1 (out), Then 2 (out), etc. |
| Input Pins | None |
| Output Pins | None |
| Add Pins | Click Add Pin to add more sequence outputs |

### 6.3) For Each Loop

| Property | Value |
|----------|-------|
| Node Type | Flow Control |
| Execution Pins | Exec (in), Loop Body (out), Completed (out) |
| Input Pins | Array (TArray of any type) |
| Output Pins | Array Element (element type), Array Index (int32) |

Connection Pattern:

| From Pin | To Node | To Pin |
|----------|---------|--------|
| Array Element | Cast To Character | Object |
| Array Element | Get Ability System Component | Actor |
| Loop Body | Processing logic | Exec In |
| Completed | Post-loop logic | Exec In |

### 6.4) For Each Loop with Break

| Property | Value |
|----------|-------|
| Node Type | Flow Control |
| Execution Pins | Exec (in), Loop Body (out), Completed (out) |
| Input Pins | Array (TArray of any type) |
| Output Pins | Array Element (element type), Array Index (int32) |
| Break Node | Use Break node to exit early |

### 6.5) Switch on ETeamAttitude

| Property | Value |
|----------|-------|
| Node Type | Flow Control |
| Execution Pins | Exec (in), Friendly (out), Neutral (out), Hostile (out) |
| Input Pins | Selection (ETeamAttitude::Type) |
| Output Pins | None |

### 6.6) Gate

| Property | Value |
|----------|-------|
| Node Type | Flow Control |
| Execution Pins | Enter (in), Exit (out), Open (in), Close (in), Toggle (in) |
| Input Pins | Start Closed (bool) |
| Output Pins | None |

### 6.7) Do Once

| Property | Value |
|----------|-------|
| Node Type | Flow Control |
| Execution Pins | Exec (in), Completed (out), Reset (in) |
| Input Pins | None |
| Output Pins | None |

---

## SECTION 7: VARIABLE NODES

### 7.1) GET Variable

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | None |
| Output Pins | Value (variable type) |
| Note | Drag variable from My Blueprint panel to graph |

### 7.2) SET Variable

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Value (variable type) |
| Output Pins | Value (variable type, pass-through) |

### 7.3) Is Valid (Object)

| Property | Value |
|----------|-------|
| Node Type | Pure Function or Macro |
| Execution Pins | None (pure) or Exec/Is Valid/Is Not Valid (macro) |
| Input Pins | Input Object (UObject*) |
| Output Pins | Return Value (bool) or Is Valid/Is Not Valid execution |

### 7.4) Is Valid (Handle)

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Handle (FGameplayAbilitySpecHandle or FActiveGameplayEffectHandle) |
| Output Pins | Return Value (bool) |

---

## SECTION 8: ACTOR AND COMPONENT NODES

### 8.1) Get Actor Location

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Target (AActor*) |
| Output Pins | Return Value (FVector) |

### 8.2) Set Actor Location

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (AActor*), New Location (FVector), Sweep (bool), Teleport (bool) |
| Output Pins | Sweep Hit Result (FHitResult), Return Value (bool) |

### 8.3) Get Actor Forward Vector

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Target (AActor*) |
| Output Pins | Return Value (FVector) |

### 8.4) Get Actor Rotation

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Target (AActor*) |
| Output Pins | Return Value (FRotator) |

### 8.5) Set Actor Rotation

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (AActor*), New Rotation (FRotator), Teleport Physics (bool) |
| Output Pins | Return Value (bool) |

### 8.6) Attach Actor To Component

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (AActor*), Parent (USceneComponent*), Socket Name (FName), Location Rule (EAttachmentRule), Rotation Rule (EAttachmentRule), Scale Rule (EAttachmentRule), Weld Simulated Bodies (bool) |
| Output Pins | None |

Connection Pattern:

| From Pin | To Node | To Pin |
|----------|---------|--------|
| Target | As BP Father Companion | - |
| Parent | Get Mesh Return Value | - |
| Socket Name | GET ChestSocketName | - |

### 8.7) Detach From Actor

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (AActor*), Location Rule (EDetachmentRule), Rotation Rule (EDetachmentRule), Scale Rule (EDetachmentRule) |
| Output Pins | None |

### 8.8) Get Mesh

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Target (ACharacter*) |
| Output Pins | Return Value (USkeletalMeshComponent*) |

### 8.9) Get Character Movement

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Target (ACharacter*) |
| Output Pins | Return Value (UCharacterMovementComponent*) |

### 8.10) Get Owner

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Target (AActor* or UActorComponent*) |
| Output Pins | Return Value (AActor*) |

### 8.11) Spawn Actor From Class

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Class (TSubclassOf AActor), Spawn Transform (FTransform), Collision Handling Override (ESpawnActorCollisionHandlingMethod), Owner (AActor*), Instigator (APawn*) |
| Output Pins | Return Value (spawned actor type) |

### 8.12) Destroy Actor

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (AActor*) |
| Output Pins | None |

---

## SECTION 9: MATH AND UTILITY NODES

### 9.1) Vector + Vector

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | A (FVector), B (FVector) |
| Output Pins | Return Value (FVector) |

### 9.2) Vector - Vector

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | A (FVector), B (FVector) |
| Output Pins | Return Value (FVector) |

### 9.3) Vector * Float

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Vector (FVector), Float (float) |
| Output Pins | Return Value (FVector) |

### 9.4) Float * Float

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | A (float), B (float) |
| Output Pins | Return Value (float) |

### 9.5) Normalize (Vector)

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | In Vector (FVector) |
| Output Pins | Return Value (FVector) |

### 9.6) Make Literal Name

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Value (string, compile-time) |
| Output Pins | Return Value (FName) |

### 9.7) Make Gameplay Tag

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Tag Name (FName) |
| Output Pins | Return Value (FGameplayTag) |

### 9.8) Make Transform

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Location (FVector), Rotation (FRotator), Scale (FVector) |
| Output Pins | Return Value (FTransform) |

---

## SECTION 10: TIMER NODES

### 10.1) Set Timer by Function Name

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Object (UObject*), Function Name (FName), Time (float), Looping (bool), Initial Start Delay (float) |
| Output Pins | Return Value (FTimerHandle) |

### 10.2) Set Timer by Event

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Event (delegate), Time (float), Looping (bool), Initial Start Delay (float) |
| Output Pins | Return Value (FTimerHandle) |

### 10.3) Clear Timer by Handle

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Handle (FTimerHandle) |
| Output Pins | None |

### 10.4) Clear and Invalidate Timer by Handle

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Handle (FTimerHandle, by reference) |
| Output Pins | None |
| Note | Preferred method - invalidates handle after clearing |

### 10.5) Is Timer Active

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Handle (FTimerHandle) |
| Output Pins | Return Value (bool) |

---

## SECTION 11: COLLISION AND OVERLAP NODES

### 11.1) Sphere Overlap Actors

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | World Context Object (UObject*), Sphere Pos (FVector), Sphere Radius (float), Object Types (array), Actor Class Filter (class), Actors to Ignore (array) |
| Output Pins | Out Actors (TArray of AActor*), Return Value (bool) |

Connection Pattern:

| From Pin | To Node | To Pin |
|----------|---------|--------|
| Out Actors | For Each Loop | Array |

### 11.2) Sphere Overlap Components

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | World Context Object (UObject*), Sphere Pos (FVector), Sphere Radius (float), Object Types (array), Component Class Filter (class), Actors to Ignore (array) |
| Output Pins | Out Components (TArray of UPrimitiveComponent*), Return Value (bool) |

### 11.3) Line Trace By Channel

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | World Context Object (UObject*), Start (FVector), End (FVector), Trace Channel (ETraceTypeQuery), Trace Complex (bool), Actors to Ignore (array), Draw Debug Type (EDrawDebugTrace), Ignore Self (bool) |
| Output Pins | Out Hit (FHitResult), Return Value (bool) |

---

## SECTION 12: ABILITYTASK NODES

### 12.1) Play Montage and Wait

| Property | Value |
|----------|-------|
| Node Type | Async Task |
| Display Name | Play Montage and Wait |
| Execution Pins | Exec (in), On Completed (out), On Blend Out (out), On Interrupted (out), On Cancelled (out) |
| Input Pins | Task Instance Name (FName), Montage to Play (UAnimMontage*), Rate (float, default 1.0), Start Section (FName), Stop when Ability Ends (bool, default true), Anim Root Motion Translation Scale (float, default 1.0) |
| Output Pins | Async Task (UAbilityTask_PlayMontageAndWait*) |

Connection Pattern:

| From Pin | To Node | To Pin |
|----------|---------|--------|
| On Completed | Post-animation logic | Exec In |
| On Cancelled | End Ability | Exec In |
| On Interrupted | End Ability | Exec In |
| On Blend Out | Cleanup logic | Exec In |

### 12.2) Play Montage and Wait for Event

| Property | Value |
|----------|-------|
| Node Type | Async Task |
| Display Name | Play Montage and Wait for Event |
| Execution Pins | Exec (in), Event Received (out), On Blend Out (out), On Interrupted (out), On Cancelled (out) |
| Input Pins | Task Instance Name (FName), Montage to Play (UAnimMontage*), Event Tags (FGameplayTagContainer), Rate (float), Stop when Ability Ends (bool), Anim Root Motion Translation Scale (float) |
| Output Pins | Event Tag (FGameplayTag), Event Data (FGameplayEventData) |

### 12.3) Spawn Projectile (Narrative Pro)

| Property | Value |
|----------|-------|
| Node Type | Async Task |
| Display Name | Spawn Projectile |
| Source | AbilityTask_SpawnProjectile.h line 27 |
| Execution Pins | Exec (in), On Target Data (out), On Destroyed (out) |
| Input Pins | Task Instance Name (FName), Class (TSubclassOf ANarrativeProjectile), Projectile Spawn Transform (FTransform) |
| Output Pins | Data (FGameplayAbilityTargetDataHandle) |

Connection Pattern:

| From Pin | To Node | To Pin |
|----------|---------|--------|
| On Target Data | Apply damage logic | Exec In |
| Data | FinalizeTargetData | Target Data |

### 12.4) Wait Delay

| Property | Value |
|----------|-------|
| Node Type | Async Task |
| Display Name | Wait Delay |
| Execution Pins | Exec (in), On Complete (out) |
| Input Pins | Time (float) |
| Output Pins | None |

### 12.5) Wait Input Press

| Property | Value |
|----------|-------|
| Node Type | Async Task |
| Display Name | Wait Input Press |
| Execution Pins | Exec (in), On Press (out) |
| Input Pins | Test Already Pressed (bool) |
| Output Pins | Time Pressed (float) |

### 12.6) Wait Input Release

| Property | Value |
|----------|-------|
| Node Type | Async Task |
| Display Name | Wait Input Release |
| Execution Pins | Exec (in), On Release (out) |
| Input Pins | Test Already Released (bool) |
| Output Pins | Time Held (float) |

### 12.7) Wait Gameplay Tag Add

| Property | Value |
|----------|-------|
| Node Type | Async Task |
| Display Name | Wait Gameplay Tag Add |
| Execution Pins | Exec (in), On Tag Added (out) |
| Input Pins | Tag (FGameplayTag), Only Trigger Once (bool) |
| Output Pins | None |

### 12.8) Wait Gameplay Tag Remove

| Property | Value |
|----------|-------|
| Node Type | Async Task |
| Display Name | Wait Gameplay Tag Remove |
| Execution Pins | Exec (in), On Tag Removed (out) |
| Input Pins | Tag (FGameplayTag), Only Trigger Once (bool) |
| Output Pins | None |

### 12.9) Move To Location And Wait (Narrative Pro)

| Property | Value |
|----------|-------|
| Node Type | Async Task |
| Display Name | Move To Location And Wait |
| Source | AbilityTask_MoveToLocationAndWait.h line 40 |
| Execution Pins | Exec (in), On Completed (out), On Failed (out) |
| Input Pins | Target Location (FVector) |
| Output Pins | Result (EPathFollowingResult::Type) |

---

## SECTION 13: DELEGATE AND EVENT BINDING

### 13.1) Bind Event (Multicast Delegate)

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Delegate (multicast delegate), Event (Custom Event reference) |
| Output Pins | None |

### 13.2) Unbind Event

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Delegate Handle (FDelegateHandle) |
| Output Pins | None |

### 13.3) OnDied Delegate (NarrativeASC)

| Property | Value |
|----------|-------|
| Delegate Type | FOnDied |
| Parameters | Killed Actor (AActor*), Killed Actor ASC (UNarrativeAbilitySystemComponent*) |
| Source | NarrativeAbilitySystemComponent.h line 56 |

### 13.4) OnDamagedBy Delegate (NarrativeASC)

| Property | Value |
|----------|-------|
| Delegate Type | FOnDamagedBy |
| Parameters | Damage Causer ASC (UNarrativeAbilitySystemComponent*), Damage (float), Spec (FGameplayEffectSpec) |
| Source | NarrativeAbilitySystemComponent.h line 58 |

### 13.5) OnHealedBy Delegate (NarrativeASC)

| Property | Value |
|----------|-------|
| Delegate Type | FOnHealedBy |
| Parameters | Healer (UNarrativeAbilitySystemComponent*), Amount (float), Spec (FGameplayEffectSpec) |
| Source | NarrativeAbilitySystemComponent.h line 57 |

---

## SECTION 14: MOVEMENT NODES

### 14.1) Get Max Walk Speed

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Target (UCharacterMovementComponent*) |
| Output Pins | Return Value (float) |

### 14.2) Set Max Walk Speed

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UCharacterMovementComponent*), Max Walk Speed (float) |
| Output Pins | None |

### 14.3) Launch Character

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (ACharacter*), Launch Velocity (FVector), bXYOverride (bool), bZOverride (bool) |
| Output Pins | None |

### 14.4) Add Movement Input

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (APawn*), World Direction (FVector), Scale Value (float), Force (bool) |
| Output Pins | None |

### 14.5) Set Movement Mode

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UCharacterMovementComponent*), New Movement Mode (EMovementMode), New Custom Mode (uint8) |
| Output Pins | None |

---

## SECTION 15: ANIMATION NODES

### 15.1) Play Animation (Actor)

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (USkeletalMeshComponent*), Animation (UAnimationAsset*), Looping (bool) |
| Output Pins | None |

### 15.2) Get Anim Instance

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Target (USkeletalMeshComponent*) |
| Output Pins | Return Value (UAnimInstance*) |

### 15.3) Montage Play

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UAnimInstance*), Montage to Play (UAnimMontage*), In Play Rate (float), Return Value Type (EMontagePlayReturnType), In Time To Start Montage At (float), Stop All Montages (bool) |
| Output Pins | Return Value (float - play length) |

### 15.4) Montage Stop

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Target (UAnimInstance*), In Blend Out Time (float), Montage (UAnimMontage*) |
| Output Pins | None |

---

## SECTION 16: EFFECT AND DAMAGE NODES

### 16.1) Get Hit Result Under Cursor By Channel

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Player Controller (APlayerController*), Trace Channel (ETraceTypeQuery), Trace Complex (bool) |
| Output Pins | Hit Result (FHitResult), Return Value (bool) |

### 16.2) Break Hit Result

| Property | Value |
|----------|-------|
| Node Type | Pure Function |
| Execution Pins | None |
| Input Pins | Hit Result (FHitResult) |
| Output Pins | Blocking Hit (bool), Initial Overlap (bool), Time (float), Distance (float), Location (FVector), Impact Point (FVector), Normal (FVector), Impact Normal (FVector), Phys Mat (UPhysicalMaterial*), Hit Actor (AActor*), Hit Component (UPrimitiveComponent*), Hit Bone Name (FName), Hit Item (int32), Element Index (int32), Face Index (int32), Trace Start (FVector), Trace End (FVector) |

### 16.3) Apply Point Damage

| Property | Value |
|----------|-------|
| Node Type | Function |
| Execution Pins | Exec (in), Exec (out) |
| Input Pins | Damaged Actor (AActor*), Base Damage (float), Hit From Direction (FVector), Hit Info (FHitResult), Event Instigator (AController*), Damage Causer (AActor*), Damage Type Class (TSubclassOf UDamageType) |
| Output Pins | Return Value (float) |

---

## SECTION 17: COMMON CONNECTION CHAINS

**Note:** The pin names in these chains reflect what users see in the Blueprint editor (e.g., "True"/"False" for Branch nodes). For programmatic generation via asset generators, use the internal pin names documented in Section 6.1 (`then`/`else` for Branch).

### 17.1) Chain: Avatar Actor to Father Reference

| Step | Node | Output Pin | Next Node | Input Pin |
|------|------|------------|-----------|-----------|
| 1 | Event ActivateAbility | Actor Info | Get Avatar Actor From Actor Info | Actor Info |
| 2 | Get Avatar Actor From Actor Info | Return Value | Cast To BP_FatherCompanion | Object |
| 3 | Cast To BP_FatherCompanion | As BP Father Companion | SET FatherRef | Value |
| 4 | Cast To BP_FatherCompanion | Success | Next operation | Exec In |

### 17.2) Chain: Father to Player Reference

| Step | Node | Output Pin | Next Node | Input Pin |
|------|------|------------|-----------|-----------|
| 1 | Cast To BP_FatherCompanion | As BP Father Companion | Get Owner Player | Target |
| 2 | Get Owner Player | Return Value | Is Valid | Input Object |
| 3 | Is Valid | Is Valid | Continue operation | Exec In |
| 4 | Get Owner Player | Return Value | SET PlayerRef | Value |

### 17.3) Chain: Movement Speed Modification

| Step | Node | Output Pin | Next Node | Input Pin |
|------|------|------------|-----------|-----------|
| 1 | GET PlayerRef | Value | Get Character Movement | Target |
| 2 | Get Character Movement | Return Value | Get Max Walk Speed | Target |
| 3 | Get Max Walk Speed | Return Value | SET OriginalMaxWalkSpeed | Value |
| 4 | SET OriginalMaxWalkSpeed | Exec Out | Float * Float setup | - |
| 5 | GET OriginalMaxWalkSpeed | Value | Float * Float | A |
| 6 | GET SpeedMultiplier | Value | Float * Float | B |
| 7 | Float * Float | Return Value | Set Max Walk Speed | Max Walk Speed |
| 8 | Get Character Movement | Return Value | Set Max Walk Speed | Target |

### 17.4) Chain: SetByCaller Damage Application

| Step | Node | Output Pin | Next Node | Input Pin |
|------|------|------------|-----------|-----------|
| 1 | GET FatherASC | Value | Make Outgoing Spec | Target |
| 2 | Make Outgoing Spec | Return Value | Assign Tag Set By Caller Magnitude | Spec Handle |
| 3 | Make Outgoing Spec | Exec Out | Assign Tag Set By Caller Magnitude | Exec In |
| 4 | GET DamageAmount | Value | Assign Tag Set By Caller Magnitude | Magnitude |
| 5 | Assign Tag Set By Caller Magnitude | Return Value | Apply Gameplay Effect Spec to Target | Spec Handle |
| 6 | Assign Tag Set By Caller Magnitude | Exec Out | Apply Gameplay Effect Spec to Target | Exec In |
| 7 | GET TargetASC | Value | Apply Gameplay Effect Spec to Target | Target |

### 17.5) Chain: Sphere Overlap Damage Loop

| Step | Node | Output Pin | Next Node | Input Pin |
|------|------|------------|-----------|-----------|
| 1 | GET PlayerRef | Value | Get Actor Location | Target |
| 2 | Get Actor Location | Return Value | Sphere Overlap Actors | Sphere Pos |
| 3 | GET DamageRadius | Value | Sphere Overlap Actors | Sphere Radius |
| 4 | Sphere Overlap Actors | Out Actors | For Each Loop | Array |
| 5 | Sphere Overlap Actors | Exec Out | For Each Loop | Exec In |
| 6 | For Each Loop | Array Element | Get Ability System Component | Actor |
| 7 | For Each Loop | Loop Body | Has Matching Gameplay Tag | - |
| 8 | Has Matching Gameplay Tag | Return Value | Branch | Condition |
| 9 | Branch | True | Make Outgoing Spec | Exec In |

### 17.6) Chain: Attachment Sequence

| Step | Node | Output Pin | Next Node | Input Pin |
|------|------|------------|-----------|-----------|
| 1 | Play Montage and Wait | On Completed | Attach Actor To Component | Exec In |
| 2 | Cast To BP_FatherCompanion | As BP Father Companion | Attach Actor To Component | Target |
| 3 | GET PlayerRef | Value | Get Mesh | Target |
| 4 | Get Mesh | Return Value | Attach Actor To Component | Parent |
| 5 | GET ChestSocketName | Value | Attach Actor To Component | Socket Name |

### 17.7) Chain: Timer-Based Periodic Action

| Step | Node | Output Pin | Next Node | Input Pin |
|------|------|------------|-----------|-----------|
| 1 | Event ActivateAbility | Then | Set Timer by Function Name | Exec In |
| 2 | Set Timer by Function Name | Return Value | SET TimerHandle | Value |
| 3 | Custom Event (PeriodicCheck) | Then | Has Authority | - |
| 4 | Has Authority | Return Value | Branch | Condition |
| 5 | Branch | True | Sphere Overlap Actors | Exec In |

### 17.8) Chain: Cross-Actor Ability Granting

| Step | Node | Output Pin | Next Node | Input Pin |
|------|------|------------|-----------|-----------|
| 1 | GET PlayerRef | Value | Get Ability System Component | Actor |
| 2 | Get Ability System Component | Return Value | Give Ability | Target |
| 3 | Give Ability | Return Value | SET AbilityHandle | Value |
| 4 | Give Ability | Exec Out | Next operation | Exec In |

### 17.9) Chain: Cross-Actor Ability Cleanup

| Step | Node | Output Pin | Next Node | Input Pin |
|------|------|------------|-----------|-----------|
| 1 | GET PlayerRef | Value | Is Valid | Input Object |
| 2 | Is Valid | Is Valid | Get Ability System Component | - |
| 3 | GET PlayerRef | Value | Get Ability System Component | Actor |
| 4 | Get Ability System Component | Return Value | Clear Ability | Target |
| 5 | GET AbilityHandle | Value | Clear Ability | Handle |

### 17.10) Chain: Knockback Calculation

| Step | Node | Output Pin | Next Node | Input Pin |
|------|------|------------|-----------|-----------|
| 1 | For Each Loop | Array Element | Get Actor Location | Target |
| 2 | Get Actor Location (enemy) | Return Value | Vector - Vector | A |
| 3 | Get Actor Location (player) | Return Value | Vector - Vector | B |
| 4 | Vector - Vector | Return Value | Normalize | In Vector |
| 5 | Normalize | Return Value | Vector * Float | Vector |
| 6 | GET KnockbackForce | Value | Vector * Float | Float |
| 7 | Vector * Float | Return Value | Launch Character | Launch Velocity |
| 8 | Cast To Character | As Character | Launch Character | Target |

### 17.11) Chain: Faction Check with GetAttitude

| Step | Node | Output Pin | Next Node | Input Pin |
|------|------|------------|-----------|-----------|
| 1 | GET FatherRef | Value | GetAttitude | Test Actor |
| 2 | For Each Loop | Array Element | GetAttitude | Target |
| 3 | GetAttitude | Return Value | Equal (Enum) | A |
| 4 | Make Literal Enum (Hostile) | - | Equal (Enum) | B |
| 5 | Equal (Enum) | Return Value | Branch | Condition |
| 6 | Branch | True | Apply damage logic | Exec In |

### 17.12) Chain: Detach with Position Offset

| Step | Node | Output Pin | Next Node | Input Pin |
|------|------|------------|-----------|-----------|
| 1 | GET FatherRef | Value | Detach From Actor | Target |
| 2 | GET PlayerRef | Value | Get Actor Location | Target |
| 3 | GET PlayerRef | Value | Get Actor Forward Vector | Target |
| 4 | Get Actor Forward Vector | Return Value | Vector * Float | Vector |
| 5 | Float literal (-200) | - | Vector * Float | Float |
| 6 | Get Actor Location | Return Value | Vector + Vector | A |
| 7 | Vector * Float | Return Value | Vector + Vector | B |
| 8 | Vector + Vector | Return Value | Set Actor Location | New Location |
| 9 | GET FatherRef | Value | Set Actor Location | Target |

### 17.13) Chain: Narrative Pro Projectile Spawn

| Step | Node | Output Pin | Next Node | Input Pin |
|------|------|------------|-----------|-----------|
| 1 | Event ActivateAbility | Then | Make Transform | - |
| 2 | GetOwningNarrativeCharacter | Return Value | Get Actor Location | Target |
| 3 | Get Actor Location | Return Value | Make Transform | Location |
| 4 | Make Transform | Return Value | Spawn Projectile | Projectile Spawn Transform |
| 5 | GET ProjectileClass | Value | Spawn Projectile | Class |
| 6 | Spawn Projectile | On Target Data | FinalizeTargetData | Exec In |
| 7 | Spawn Projectile | Data | FinalizeTargetData | Target Data |

---

## SECTION 18: PIN TYPE COMPATIBILITY RULES

### 18.1) Execution Pin Rules

| Rule | Description |
|------|-------------|
| White to White only | Execution pins only connect to execution pins |
| Direction matters | Output exec connects to input exec |
| Multiple outputs allowed | Branch then/else (editor shows True/False), Sequence Then 0/1/2 |
| Single input per node | Each node has one exec input |

### 18.2) Data Pin Compatibility

| From Type | Compatible With |
|-----------|-----------------|
| AActor* | Cast nodes, Get Component nodes, Is Valid |
| ANarrativeCharacter* | AActor* (upcast), ANarrativeNPCCharacter* cast |
| ANarrativeNPCCharacter* | ANarrativeCharacter* (upcast), AActor* (upcast) |
| UAbilitySystemComponent* | All GAS function Target pins |
| UNarrativeAbilitySystemComponent* | UAbilitySystemComponent* (upcast) |
| ACharacter* | Movement functions, Launch Character |
| FGameplayEffectSpecHandle | Assign SetByCaller, Apply GE Spec nodes |
| FActiveGameplayEffectHandle | Remove Active GE, variable storage |
| FGameplayAbilitySpecHandle | Cancel Ability, Clear Ability, Set Remove Ability On End |
| FTimerHandle | Clear Timer nodes |
| FVector | Location pins, Math operations |
| float | Math operations, property pins |
| bool | Branch Condition, checkbox properties |
| int32 | Array indices, Stacks to Remove |
| TArray | For Each Loop Array, Length |

### 18.3) Implicit Type Conversions

| From | To | Method |
|------|----|--------|
| Child class | Parent class | Automatic upcast |
| AActor* | UObject* | Automatic |
| ACharacter* | AActor* | Automatic |
| ANarrativeNPCCharacter* | ANarrativeCharacter* | Automatic |
| ANarrativeCharacter* | AActor* | Automatic |
| int32 | float | Automatic |
| float | int32 | Truncation (use Floor/Round for control) |
| UNarrativeAbilitySystemComponent* | UAbilitySystemComponent* | Automatic |

### 18.4) Required Explicit Casts

| From | To | Node Required |
|------|----|---------------|
| AActor* | BP_FatherCompanion* | Cast To BP_FatherCompanion |
| AActor* | ANarrativeNPCCharacter* | Cast To NarrativeNPCCharacter |
| AActor* | ANarrativePlayerCharacter* | Cast To NarrativePlayerCharacter |
| AActor* | ACharacter* | Cast To Character |
| UObject* | Specific class | Cast To [ClassName] |
| FGameplayAbilityActorInfo | AActor* | Get Avatar Actor From Actor Info |
| FGameplayAbilityActorInfo | UAbilitySystemComponent* | Get Ability System Component From Actor Info |

---

## SECTION 19: VALIDATION CHECKLIST

### 19.1) Before Building Blueprint

| Check | Description |
|-------|-------------|
| Execution flow | White pins connect in logical order |
| Type matching | Output pin type matches input pin type |
| Pure functions | No exec pins on pure functions |
| Reference validity | Is Valid check before dereferencing |
| Handle storage | Store handles for cleanup |

### 19.2) Common Errors to Avoid

| Error | Cause | Solution |
|-------|-------|----------|
| Cannot connect exec to data | Connecting white to colored pin | Use SET variable or function with exec |
| Types not compatible | Mismatched data types | Add conversion or cast node |
| Cannot find pin | Node is pure function | Remove exec, use data pin only |
| Target requires ASC | Wrong object to GAS node | Add Get Ability System Component |
| Spec Handle invalid | Not connected through chain | Connect Make Outgoing Spec result through chain |
| Cast failed at runtime | Object not expected type | Add Is Valid check, handle failure case |
| Handle not stored | Effect/ability cannot be cleaned up | Store handle in variable for later removal |
| Timer keeps firing | Timer not cleared in EndAbility | Clear timer in cleanup logic |

### 19.3) Multiplayer Validation

| Check | Description |
|-------|-------------|
| Authority check | Server-only operations have Has Authority gate |
| Replication | Variables marked RepNotify where needed |
| Net Execution Policy | Correct policy for ability type |
| Cross-actor operations | Server Only for NPC-owned abilities |

### 19.4) Programmatic Blueprint Generation Validation

When generating Blueprints programmatically (e.g., via asset generator plugins), validate:

| Check | Description |
|-------|-------------|
| NotifyPinConnectionListChanged | Call on both nodes after MakeLinkTo() for Cast nodes |
| Branch pin names | Use `then`/`else`, not `True`/`False` |
| Pin name case sensitivity | Some pins are case-sensitive (e.g., `then` not `Then`) |
| Pure node detection | Check IsPureFunc() before creating execution connections |
| Node reconstruction | Call ReconstructNode() if node internals need updating |

| Common Error | Cause | Solution |
|--------------|-------|----------|
| Object type undetermined (Cast) | MakeLinkTo without NotifyPinConnectionListChanged | Call NotifyPinConnectionListChanged on both nodes |
| Pin not found: True/False (Branch) | Using display names instead of internal names | Use `then`/`else` for Branch output pins |
| Orphan pins after connection | Node state not updated after dynamic changes | Call ReconstructNode() or NotifyPinConnectionListChanged |
| Compile error on valid connections | Connection added but node not aware | Always notify nodes of connection changes |

---

## SECTION 20: RESEARCH VALIDATION ITEMS

### 20.1) Verified Nodes from Narrative Pro Source

| Node | Source File | Line | Status |
|------|-------------|------|--------|
| GetOwningController | NarrativeGameplayAbility.h | 40 | Verified |
| GetOwningNarrativeCharacter | NarrativeGameplayAbility.h | 43 | Verified |
| GetOwningNarrativeCharacterVisual | NarrativeGameplayAbility.h | 46 | Verified |
| GetOwnerEquippedWeapon | NarrativeGameplayAbility.h | 49 | Verified |
| IsBot | NarrativeGameplayAbility.h | 53 | Verified |
| GetAvatarOwner | NarrativeAbilitySystemComponent.h | 99 | Verified |
| GetCharacterOwner | NarrativeAbilitySystemComponent.h | 104 | Verified |
| IsDead | NarrativeAbilitySystemComponent.h | 153 | Verified |
| GetAttitude | ArsenalStatics.h | 81 | Verified |
| IsSameTeam | ArsenalStatics.h | 78 | Verified |
| Spawn Projectile | AbilityTask_SpawnProjectile.h | 27 | Verified |
| Move To Location And Wait | AbilityTask_MoveToLocationAndWait.h | 40 | Verified |
| GetNarrativeCharacter (Projectile) | NarrativeProjectile.h | 22 | Verified |
| SetProjectileTargetData | NarrativeProjectile.h | 32 | Verified |
| GenerateTargetDataUsingTrace | NarrativeCombatAbility.h | 136 | Verified |
| GetAttackDamage | NarrativeCombatAbility.h | 157 | Verified |
| GetAbilityWeapon | NarrativeCombatAbility.h | 204 | Verified |

### 20.2) Pin Configuration Verification

| Node | Pin Name | Pin Type | Verified |
|------|----------|----------|----------|
| Event ActivateAbility | Actor Info | FGameplayAbilityActorInfo | Yes |
| Make Outgoing Spec | Target | UAbilitySystemComponent* | Yes |
| Make Outgoing Spec | Gameplay Effect Class | TSubclassOf | Yes |
| Make Outgoing Spec | Return Value | FGameplayEffectSpecHandle | Yes |
| Assign Tag Set By Caller Magnitude | Data Tag | FGameplayTag | Yes |
| Assign Tag Set By Caller Magnitude | Magnitude | float | Yes |
| Apply Gameplay Effect Spec to Target | Spec Handle | FGameplayEffectSpecHandle | Yes |
| For Each Loop | Array Element | Element type | Yes |
| Sphere Overlap Actors | Out Actors | TArray AActor* | Yes |
| GetAttitude | Return Value | ETeamAttitude::Type | Yes |
| Spawn Projectile | On Target Data | Delegate output | Yes |
| Spawn Projectile | Data | FGameplayAbilityTargetDataHandle | Yes |

### 20.3) Connection Validity Verification

| From Node | From Pin | To Node | To Pin | Verified |
|-----------|----------|---------|--------|----------|
| Event ActivateAbility | Actor Info | Get Avatar Actor From Actor Info | Actor Info | Yes |
| Get Avatar Actor From Actor Info | Return Value | Cast To BP_FatherCompanion | Object | Yes |
| Make Outgoing Spec | Return Value | Assign Tag Set By Caller Magnitude | Spec Handle | Yes |
| Assign Tag Set By Caller Magnitude | Return Value | Apply Gameplay Effect Spec to Target | Spec Handle | Yes |
| Sphere Overlap Actors | Out Actors | For Each Loop | Array | Yes |
| For Each Loop | Array Element | Cast To Character | Object | Yes |
| Get Character Movement | Return Value | Set Max Walk Speed | Target | Yes |
| GetAttitude | Return Value | Equal (Enum) | A | Yes |
| Spawn Projectile | Data | FinalizeTargetData | Target Data | Yes |

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 2.1 | January 2026 | **Programmatic generation fixes:** Corrected Branch node pin names from True/False to then/else (Section 6.1). Added Section 5.5 documenting DynamicCast NotifyPinConnectionListChanged requirements. Added Section 19.4 with programmatic Blueprint generation validation checklist. These fixes address errors discovered during GasAbilityGenerator v2.5.4 development. |
| 2.0 | January 2026 | Major update with Narrative Pro v2.2 source code verification. Added Section 4 with all Narrative Pro specific nodes. Verified all node configurations against C++ headers. Added AbilityTask_SpawnProjectile details. Added connection chains for projectile spawning and faction checking. |
| 1.0 | January 2026 | Initial comprehensive reference covering all nodes from Father Companion implementation guides |

---

**END OF REFERENCE**
