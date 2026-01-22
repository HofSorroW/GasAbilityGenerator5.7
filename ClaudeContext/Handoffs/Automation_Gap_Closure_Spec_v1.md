# Automation Gap Closure Specification v1.0

**Created:** 2026-01-22
**Status:** Research Complete, Implementation Ready
**Scope:** Close 23 manual setup items (excluding VFX/Niagara)

---

## Executive Summary

Research identified that **9 of 23 gaps are already automated** with fallback logging. The remaining 14 gaps require implementation work ranging from LOW to HIGH complexity.

| Category | Total Gaps | Already Automated | Needs Work |
|----------|-----------|-------------------|------------|
| Dialogue System | 3 | 1 (PartySpeakerInfo) | 2 |
| Quest System | 6 | 3 (rewards) | 3 |
| Item/Equipment | 8 | 2 (PickupMeshData, TraceData) | 6 |
| NPC/Character | 3 | 2 (NPCTargets, CharacterTargets) | 1 |
| AI/Goals | 1 | 1 (Goal resolution) | 0 |
| **TOTAL** | **21** | **9** | **12** |

---

## Part 1: Already Automated (Fallback Only)

These gaps have complete automation code. The "manual setup required" messages are fallback behaviors when auto-resolution fails.

### 1.1 Quest Rewards (3 items) - VERIFIED COMPLETE

| Reward Type | Generator Location | Search Paths | Status |
|-------------|-------------------|--------------|--------|
| XP (NE_GiveXP) | Lines 22098-22130 | 4 NarrativePro paths | ✅ Complete |
| Currency (BPE_AddCurrency) | Lines 22133-22165 | 4 NarrativePro paths | ✅ Complete |
| Items (BPE_AddItemToInventory) | Lines 22168-22231 | 4 NarrativePro paths | ✅ Complete |

**Action Required:** None - verify asset paths exist in Narrative Pro v2.2 Beta.

### 1.2 NPC/Character Targets (2 items) - VERIFIED COMPLETE

| Property | Generator Location | Status |
|----------|-------------------|--------|
| NPCTargets | Lines 18182-18256 | ✅ Complete (FSoftObjectProperty + FObjectProperty) |
| CharacterTargets | Lines 18259-18307 | ✅ Complete (FSoftObjectProperty only) |

**Minor Fix:** Add FObjectProperty fallback to CharacterTargets (parity with NPCTargets).

### 1.3 Goal Class Resolution - VERIFIED COMPLETE

| Feature | Generator Location | Status |
|---------|-------------------|--------|
| Goal class search | Lines 21350-21395 | ✅ Complete (11 search paths) |

**Action Required:** None - coverage is comprehensive.

### 1.4 Item Struct Properties (2 items) - VERIFIED COMPLETE

| Property | Generator Location | Status |
|----------|-------------------|--------|
| PickupMeshData | Lines 17191-17252 | ✅ Complete |
| TraceData | Lines 17254-17298 | ✅ Complete |

**Gold Standard:** These demonstrate the correct reflection pattern for struct automation.

---

## Part 2: Dependency Ordering Fixes (Quick Wins)

These have complete reflection code but fail due to generation ordering.

### 2.1 EquipmentAbilities

**Location:** `GasAbilityGeneratorGenerators.cpp:16977-17029`

**Problem:** Item generated before referenced abilities exist.

**Current Code (working):**
```cpp
FArrayProperty* AbilitiesArrayProp = CastField<FArrayProperty>(
    CDO->GetClass()->FindPropertyByName(TEXT("EquipmentAbilities")));
if (AbilitiesArrayProp) {
    FScriptArrayHelper ArrayHelper(AbilitiesArrayProp, ...);
    for (const FString& AbilityName : Definition.EquipmentAbilities) {
        UClass* AbilityClass = FindObject<UClass>(nullptr, *AbilityName);
        // Resolution fails if ability not yet generated
    }
}
```

**Solution:** Add deferred resolution pass.

**Manifest Format (unchanged):**
```yaml
equippable_items:
  - name: EI_DragonSword
    equipment_abilities:  # TArray<TSubclassOf<UNarrativeGameplayAbility>>
      - GA_DragonSlash
      - GA_FireBreath
```

**Implementation:**
1. First pass: Generate item without EquipmentAbilities
2. Second pass: Resolve and assign abilities after all GA_ assets exist
3. Use existing deferred retry mechanism (v2.6.7)

**Complexity:** LOW
**Files:** `GasAbilityGeneratorCommandlet.cpp` (add deferred item abilities pass)

---

### 2.2 ActivitiesToGrant

**Location:** `GasAbilityGeneratorGenerators.cpp:17093-17154`

**Problem:** Same as EquipmentAbilities - activities may not exist yet.

**Solution:** Same deferred resolution pass.

**Manifest Format (unchanged):**
```yaml
equippable_items:
  - name: EI_CommanderBadge
    activities_to_grant:  # TArray<TSubclassOf<UNPCActivity>>
      - BPA_CommandSquad
      - BPA_IssueTacticalOrders
```

**Complexity:** LOW
**Files:** Same as EquipmentAbilities

---

## Part 3: New Automation Implementation

### 3.1 Dialogue Conditions/Events

**Location:** `DialogueTableConverter.cpp:40`

**Current State:** Phase 2 placeholder - only extracts speaker names.

**Root Cause:** Dialogue conditions/events require:
1. Cross-referencing speaker names to NPC definitions
2. Populating FSpeakerInfo structs with NPCDataAsset pointers
3. Event token parsing from dialogue tree nodes

**Manifest Format (NEW):**
```yaml
dialogue_blueprints:
  - name: DBP_MerchantGreeting
    speakers:
      - speaker_id: Merchant
        npc_definition: NPC_Blacksmith      # Links to NPCDefinition asset
        node_color: "#FF6600"
        owned_tags: [NPC.Speaking]
    dialogue_tree:
      nodes:
        - id: greeting
          type: npc
          speaker: Merchant                  # References speaker_id above
          text: "Welcome to my shop!"
          conditions:                        # Node visibility conditions
            - type: HasGameplayTag
              properties:
                tag: Player.HasMoney
            - type: QuestStateReached
              properties:
                quest: Quest_MerchantIntro
                state: Completed
          events:                           # Events fired during node
            - type: NE_GiveXP
              runtime: End
              properties:
                xp_amount: 50
```

**Implementation:**
1. Parse `speakers[].npc_definition` → resolve to UNPCDefinition asset
2. Populate `FSpeakerInfo.NPCDataAsset` via reflection
3. Parse `conditions[]` → create condition instances via existing `CreateDialogueConditionFromDefinition()`
4. Parse `events[]` → create event instances via existing event resolution

**UE5 API:**
- `FSpeakerInfo` struct (Dialogue.h:30-81)
- `CreateDialogueConditionFromDefinition()` already exists (lines 14836-14884)

**Complexity:** MEDIUM
**Files:** `GasAbilityGeneratorGenerators.cpp`, `GasAbilityGeneratorParser.cpp`

---

### 3.2 DefaultDialogueShot Sequences

**Location:** `GasAbilityGeneratorGenerators.cpp:15452-15483`

**Current State:** Single sequence class works; sequence asset arrays logged only.

**Root Cause:** Complex sequence composition requires understanding UNarrativeDialogueSequence API.

**Manifest Format (NEW):**
```yaml
dialogue_blueprints:
  - name: DBP_CinematicDialogue
    default_dialogue_shot:
      # Option 1: Single sequence class (already works)
      sequence_class: NDS_OverTheShoulder

      # Option 2: Sequence assets with timing (NEW)
      sequence_assets:
        - asset: "/Game/Sequences/DialogueShot_CloseUp"
          blend_time: 0.5
        - asset: "/Game/Sequences/DialogueShot_Wide"
          blend_time: 1.0
```

**Implementation:**
1. Parse `sequence_assets[]` array
2. Load each UNarrativeDialogueSequence asset via LoadObject
3. Populate `TArray<TSoftObjectPtr<UNarrativeDialogueSequence>>` via FScriptArrayHelper

**Complexity:** MEDIUM
**Files:** `GasAbilityGeneratorGenerators.cpp`, `GasAbilityGeneratorTypes.h`

---

### 3.3 BPT_FinishDialogue Task Setup

**Location:** `GasAbilityGeneratorGenerators.cpp:15087-15090`

**Current State:** Logs info for manual setup.

**Root Cause:** Quest tasks are Blueprint classes with complex lifecycle.

**Manifest Format (enhanced):**
```yaml
quests:
  - name: Quest_MerchantFavor
    branches:
      - id: TalkToMerchant
        from_state: Start
        to_state: InProgress
        tasks:
          - task_class: BPT_FinishDialogue
            properties:
              dialogue: DBP_MerchantQuestStart    # Auto-resolved to UDialogue
          - task_class: BPT_FindItem
            quantity: 5
            properties:
              item_class: EI_IronOre              # Auto-resolved to UNarrativeItem
          - task_class: BPT_Move
            properties:
              target_location: POI_Blacksmith     # Resolved to FVector or POI reference
```

**Implementation:**
1. Load task class via existing multi-path search
2. Create task instance: `NewObject<UNarrativeTask>(Branch, TaskClass)`
3. Set properties via reflection (`Property->ImportText_Direct()`)
4. Add task to branch (requires finding AddTask method on UQuestBranch)

**UE5 API Research:**
- `UNarrativeTask` is base class (Quest.h:8)
- Task lifecycle: PreExecute, Execute, OnTaskCompleted
- Need to verify UQuestBranch has AddTask() or similar

**Complexity:** MEDIUM-HIGH
**Files:** `GasAbilityGeneratorGenerators.cpp`

---

### 3.4 Quest Fail Setup

**Location:** `GasAbilityGeneratorGenerators.cpp:15092-15096`

**Current State:** No NE_FailQuest event exists in Narrative Pro.

**Root Cause:** `FailQuest()` is an instance method on UQuest, not a spawnable event.

**Manifest Format (NEW):**
```yaml
dialogue_blueprints:
  - name: DBP_QuestGiverAngry
    dialogue_tree:
      nodes:
        - id: refuse_quest
          type: player
          text: "I refuse to help you!"
          fail_quest: Quest_MerchantFavor       # Triggers Quest->FailQuest()
          fail_message: "You refused to help the merchant."
```

**Implementation Options:**

**Option A: Custom Event Blueprint**
1. Create `NE_FailQuest` Blueprint in project at generation time
2. Set `QuestToFail` property via reflection
3. Add to dialogue node events

**Option B: Dialogue Event Graph Node**
1. Generate CallFunction node targeting quest instance
2. Wire to `FailQuest(FText Message)` function
3. Complex: requires quest reference resolution at dialogue runtime

**Recommendation:** Option A is cleaner and follows existing event patterns.

**Complexity:** MEDIUM
**Files:** `GasAbilityGeneratorGenerators.cpp`, create NE_FailQuest template

---

### 3.5 Questgiver Linking

**Location:** `GasAbilityGeneratorGenerators.cpp:22062-22066`

**Current State:** Logged for documentation only.

**Root Cause:** Questgiver is implicit relationship:
- NPC has dialogue
- Dialogue has start_quest event
- Quest has no Questgiver property

**Manifest Format (enhanced):**
```yaml
quests:
  - name: Quest_ForgeSupplies
    questgiver: NPC_Blacksmith              # Documentation + auto-link

npc_definitions:
  - name: NPC_Blacksmith
    dialogue: DBP_BlacksmithMain            # NPC's dialogue
    # Auto-creates: DBP_BlacksmithMain contains start_quest: Quest_ForgeSupplies
```

**Implementation:**
1. When generating quest with `questgiver` field:
   - Find NPC definition by name
   - Get NPC's dialogue asset
   - Find first suitable dialogue node (greeting or quest-offer node)
   - Add `start_quest` event pointing to this quest
2. Or: Generate placeholder dialogue with start_quest if `auto_create_dialogue: true`

**Complexity:** MEDIUM
**Files:** `GasAbilityGeneratorGenerators.cpp` (quest and NPC generators)

---

### 3.6 MeshMaterials (Complex Struct)

**Location:** `GasAbilityGeneratorGenerators.cpp:16892-16924`

**Current State:** Logs entries but doesn't populate.

**Root Cause:** Requires gameplay tag validation before binding.

**Manifest Format (NEW):**
```yaml
equippable_items:
  - name: EI_DragonArmor
    clothing_mesh:
      mesh: "/Game/Meshes/SK_DragonArmor"
      materials:
        - material: "/Game/Materials/M_DragonScale"
          vector_params:
            - tag: CharacterCreator.Color.Primary     # FGameplayTag VectorTagID
              value: [0.8, 0.2, 0.1, 1.0]             # FLinearColor
          scalar_params:
            - tag: CharacterCreator.Roughness         # FGameplayTag ScalarTagID
              value: 0.3
```

**Implementation:**
1. Validate tags exist via `FGameplayTag::RequestGameplayTag(TagName, false)` - returns empty if not found
2. Log warning if tag doesn't exist (non-blocking)
3. Create `FCreatorMeshMaterial` struct via reflection
4. Populate nested arrays via `FScriptArrayHelper`

**UE5 API ([FGameplayTag Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/GameplayTags/FGameplayTag)):**
```cpp
FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), /*ErrorIfNotFound=*/false);
if (!Tag.IsValid()) {
    LogGeneration(FString::Printf(TEXT("  [WARN] Tag not found: %s"), *TagString));
}
```

**Complexity:** MEDIUM
**Files:** `GasAbilityGeneratorGenerators.cpp`, `GasAbilityGeneratorTypes.h`

---

### 3.7 Morphs (Complex Struct)

**Location:** `GasAbilityGeneratorGenerators.cpp:16912-16924`

**Current State:** Logs entries but doesn't populate.

**Root Cause:** Requires morph target validation against skeletal mesh.

**Manifest Format (NEW):**
```yaml
equippable_items:
  - name: EI_FlexibleArmor
    clothing_mesh:
      mesh: "/Game/Meshes/SK_FlexArmor"
      morphs:
        - scalar_tag: CharacterCreator.Build.Muscular   # FGameplayTag
          morph_names:                                   # TArray<FName>
            - "Muscular_Arms"
            - "Muscular_Chest"
        - scalar_tag: CharacterCreator.Build.Slim
          morph_names:
            - "Slim_Waist"
```

**Implementation:**
1. Load skeletal mesh via `LoadObject<USkeletalMesh>`
2. Get morph target names via `Mesh->K2_GetAllMorphTargetNames()` ([UE5 API](https://docs.unrealengine.com/5.0/en-US/API/Runtime/Engine/Engine/USkeletalMesh/K2_GetAllMorphTargetNames/))
3. Validate each `morph_name` exists in mesh's morph targets
4. Log warning if morph not found (non-blocking)
5. Validate tag via `FGameplayTag::RequestGameplayTag()`
6. Populate `FCreatorMeshMorph` struct via reflection

**UE5 API:**
```cpp
USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *MeshPath);
if (Mesh) {
    TArray<FString> MorphNames = Mesh->K2_GetAllMorphTargetNames();
    // Validate manifest morph names against MorphNames
}
```

**Complexity:** MEDIUM
**Files:** `GasAbilityGeneratorGenerators.cpp`, `GasAbilityGeneratorTypes.h`

---

### 3.8 Stats Property

**Location:** `GasAbilityGeneratorGenerators.cpp:17031-17091`

**Current State:** Property lookup fails, falls back to logging.

**Root Cause:** Fundamental struct mismatch:
- **Manifest:** `{ name, numeric_value, icon }` (display-oriented)
- **Narrative Pro:** `{ display_name (FText), string_variable (computed), tooltip (FText) }` (computation-oriented)

**Manifest Format (NEW - aligned with Narrative Pro):**
```yaml
equippable_items:
  - name: EI_DragonSword
    stats:
      - display_name: "Attack Power"          # FText
        string_variable: "AttackRating"       # FString - variable name for computation
        tooltip: "Base weapon damage"         # FText
      - display_name: "Fire Damage"
        string_variable: "FireDamageBonus"
        tooltip: "Additional fire damage per hit"
```

**Implementation:**
1. Find `Stats` property on CDO (TArray<FNarrativeItemStat>)
2. Create array via `FScriptArrayHelper`
3. For each stat:
   - Set `StatDisplayName` (FText) via `FTextProperty`
   - Set `StringVariable` (FString) via `FStrProperty`
   - Set `StatTooltip` (FText) via `FTextProperty`

**Note:** The old manifest format (`name`, `value`, `icon`) is incompatible. This is a breaking change for stats.

**Complexity:** MEDIUM
**Files:** `GasAbilityGeneratorGenerators.cpp`, `GasAbilityGeneratorTypes.h`, `GasAbilityGeneratorParser.cpp`

---

### 3.9 NPC Blueprint Assignment

**Location:** `NPCTableValidator.cpp:173`

**Current State:** Validator warns but doesn't auto-assign.

**Root Cause:** Design choice - blueprints are project-specific.

**Manifest Format (NEW):**
```yaml
npc_definitions:
  - name: NPC_Blacksmith
    npc_blueprint: BP_Blacksmith              # Existing: manual reference
    auto_create_blueprint: true               # NEW: auto-generate if not found
    blueprint_parent: NarrativeNPCCharacter   # Parent class for auto-creation
```

**Implementation Options:**

**Option A: Auto-Link by Name Convention**
1. If `npc_blueprint` not specified, search for `BP_{NPCName}` or `BP_NPC_{NPCName}`
2. If found, auto-assign
3. If not found, log warning (existing behavior)

**Option B: Auto-Create Blueprint**
1. If `auto_create_blueprint: true` and blueprint not found:
2. Create new Blueprint with specified parent class
3. Set basic properties (mesh, animations if specified)
4. Log info about created blueprint

**Recommendation:** Option A first (low risk), Option B as follow-up.

**Complexity:** LOW (Option A), MEDIUM (Option B)
**Files:** `GasAbilityGeneratorGenerators.cpp` (NPC generator)

---

## Part 4: Implementation Priority

### Phase 1: Quick Wins (1-2 days)

| Item | Complexity | Impact |
|------|------------|--------|
| EquipmentAbilities deferred pass | LOW | HIGH - unblocks item abilities |
| ActivitiesToGrant deferred pass | LOW | HIGH - unblocks item activities |
| CharacterTargets FObjectProperty fallback | LOW | LOW - parity fix |
| NPC Blueprint auto-link (Option A) | LOW | MEDIUM - reduces manual setup |

### Phase 2: Core Automation (3-5 days)

| Item | Complexity | Impact |
|------|------------|--------|
| MeshMaterials with tag validation | MEDIUM | HIGH - character creator |
| Morphs with mesh validation | MEDIUM | HIGH - character creator |
| Dialogue conditions/events | MEDIUM | HIGH - dialogue system |
| Stats property (breaking change) | MEDIUM | MEDIUM - item display |

### Phase 3: Quest Pipeline (3-5 days)

| Item | Complexity | Impact |
|------|------------|--------|
| BPT_FinishDialogue task setup | MEDIUM-HIGH | HIGH - quest automation |
| Quest fail setup (NE_FailQuest) | MEDIUM | MEDIUM - quest branching |
| Questgiver linking | MEDIUM | MEDIUM - NPC-quest integration |
| DefaultDialogueShot sequences | MEDIUM | LOW - cinematic dialogues |

---

## Part 5: Manifest Schema Summary

### New Fields Added

```yaml
# dialogue_blueprints
speakers:
  - speaker_id: string
    npc_definition: string           # NEW: Links to NPC asset
dialogue_tree:
  nodes:
    conditions:
      - type: string
        properties: object           # ENHANCED: Full condition support
    events:
      - type: string
        runtime: Start|End|Both
        properties: object           # ENHANCED: Full event support
    fail_quest: string               # NEW: Quest to fail
    fail_message: string             # NEW: Fail message

# quests
branches:
  tasks:
    - task_class: string
      quantity: int                  # For countable tasks
      properties: object             # NEW: Full task property support

# equippable_items
clothing_mesh:
  materials:                         # NEW: Full material support
    - material: string
      vector_params:
        - tag: string
          value: [r, g, b, a]
      scalar_params:
        - tag: string
          value: float
  morphs:                            # NEW: Full morph support
    - scalar_tag: string
      morph_names: [string]

stats:                               # CHANGED: Aligned with Narrative Pro
  - display_name: string             # FText
    string_variable: string          # Computation variable
    tooltip: string                  # FText

# npc_definitions
auto_create_blueprint: bool          # NEW: Auto-generate BP if not found
blueprint_parent: string             # Parent class for auto-creation
```

---

## References

- [FGameplayTag UE5.7 Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/GameplayTags/FGameplayTag)
- [FGameplayTagContainer UE5.7 Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/GameplayTags/FGameplayTagContainer)
- [USkeletalMesh::K2_GetAllMorphTargetNames](https://docs.unrealengine.com/5.0/en-US/API/Runtime/Engine/Engine/USkeletalMesh/K2_GetAllMorphTargetNames/)
- [Using Gameplay Tags in UE](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-tags-in-unreal-engine)
- [Narrative Tools](https://www.narrativetools.io/) - Narrative Pro documentation

---

## Document History

| Date | Version | Change |
|------|---------|--------|
| 2026-01-22 | 1.0 | Initial research and specification |
