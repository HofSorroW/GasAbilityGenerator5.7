# Save System Integration Research (P2.2)

**Created:** 2026-01-28
**Status:** RESEARCH COMPLETE - Awaiting user review of NP documentation
**Decision:** PENDING - User to watch https://www.youtube.com/watch?v=loI9uFbSDwM

---

## Executive Summary

Research into Narrative Pro's save system reveals that **most GAS and NPC state is automatically persisted**. The proposed `on_load:` event graph feature may not be necessary.

**Key Finding:** Narrative Pro's `UNarrativeAbilitySystemComponent` and `ANarrativeNPCCharacter` both implement save interfaces that handle core state automatically.

---

## 1. GAS (Gameplay Ability System) Save/Load

### Architecture

| Component | Purpose |
|-----------|---------|
| `UNarrativeSaveSubsystem` | World subsystem coordinating all save/load |
| `UNarrativeSave` | Save file object (extends USaveGame) |
| `INarrativeSavableComponent` | Interface for savable components |
| `UNarrativeAbilitySystemComponent` | ASC with automatic attribute save/load |

### What IS Automatically Saved (Attributes)

The ASC has a configurable array:
```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Saving")
TArray<FGameplayAttribute> AttributesToSave;
```

Attributes with `NarrativeSaveAttribute` metadata in `UNarrativeAttributeSetBase`:

| Attribute | Auto-Saved |
|-----------|------------|
| Health | ✅ Yes |
| MaxHealth | ✅ Yes |
| Stamina | ✅ Yes |
| MaxStamina | ✅ Yes |
| StaminaRegenRate | ✅ Yes |
| XP | ✅ Yes |
| AttackRating | ❌ No |
| Armor | ❌ No |
| AttackDamage | ❌ No |
| StealthRating | ❌ No |

### What IS NOT Saved (GAS)

| Data | Persisted? | Impact |
|------|------------|--------|
| Active GameplayEffects | ❌ No | Buffs/debuffs lost on load |
| Granted Abilities | ❌ No | Must re-grant from AbilityConfiguration |
| Gameplay Tags | ❌ No | State tags lost (e.g., `Father.State.Attached`) |
| Ability Variables | ❌ No | GA-local state lost |
| Ability Cooldowns | ❌ No | Cooldown timers reset |

### Implementation Code Reference

**PrepareForSave** (`NarrativeAbilitySystemComponent.cpp:671-712`):
```cpp
void UNarrativeAbilitySystemComponent::PrepareForSave_Implementation()
{
    TArray<FGameplayAttribute> OurAttributes;
    GetAllAttributes(OurAttributes);
    SavedAttributes.Empty();

    for (auto& Attribute : OurAttributes)
    {
        if (!AttributesToSave.Contains(Attribute))
            continue;  // Only save configured attributes

        float AttributeValue = GetFloatAttributeFromASC(this, Attribute);
        SavedAttributes.Add({Attribute.AttributeName, AttributeValue});
    }
}
```

**Load** (`NarrativeAbilitySystemComponent.cpp:714-740`):
```cpp
void UNarrativeAbilitySystemComponent::Load_Implementation()
{
    for (auto& SavedAttr : SavedAttributes)
    {
        FGameplayAttribute Attribute = FindAttribute(SavedAttr.AttributeName);
        ApplyModToAttribute(Attribute, EGameplayModOp::Override, SavedAttr.Value);
    }
}
```

---

## 2. NPC Save/Load

### ANarrativeNPCCharacter Implements INarrativeSavableActor

```cpp
// NarrativeNPCCharacter.h:197
class ANarrativeNPCCharacter : public ANarrativeCharacter, public INarrativeSavableActor
```

### What IS Automatically Saved (NPCs)

| State | Saved? | Storage |
|-------|--------|---------|
| Position/Transform | ✅ Yes | `FNarrativeActorRecord::Transform` |
| Health/Attributes | ✅ Yes | ASC + `AttributesToSave` |
| Inventory Items | ✅ Yes | `InventoryComponent` SaveGame vars |
| Equipment/Weapons | ✅ Yes | `EquipmentComponent` SaveGame vars |
| NPC Level | ✅ Yes | `UPROPERTY(SaveGame)` |
| Factions | ✅ Yes | `UPROPERTY(SaveGame)` tags |
| AI Controller State | ✅ Yes | Separate `AICRecord` |
| Appearance | ✅ Yes | Applied at load from definition |
| Death Status | ✅ Yes | `bWasKilled` on spawner |
| Spawn Info | ✅ Yes | `FNPCSpawnInfo` struct |

### What IS NOT Saved (NPCs)

| State | Saved? | Impact |
|-------|--------|--------|
| Current Activity | ❌ No | Activity restarts on load |
| Current Goal | ❌ No | Goal restarts on load |
| Activity Progress | ❌ No | Schedule position lost |
| Dialogue History | ❌ No | "Has talked to" not on NPC* |
| Ability Cooldowns | ❌ No | Reset on load |
| Tagged Dialogue Cooldowns | ❌ No | Barks can repeat |

*Dialogue/quest state tracked by Tales system on PlayerController, not on NPC.

### NPC Spawner Respawn Control

```cpp
// NPCSpawnComponent.h
UPROPERTY(SaveGame)
bool bDontSpawnIfPreviouslyKilled = true;  // Default

UPROPERTY(SaveGame)
bool bWasKilled = false;  // Set when NPC dies
```

- `bDontSpawnIfPreviouslyKilled = true` → Dead NPCs stay dead
- `bDontSpawnIfPreviouslyKilled = false` → Dead NPCs respawn

### FNarrativeActorRecord Structure

What gets stored per NPC in save file:

```cpp
struct FNarrativeActorRecord
{
    FGuid ActorGUID;                        // Unique identifier
    FName ActorName;                        // Debug name
    FTransform Transform;                   // Position/rotation/scale
    bool bDestroyed;                        // Is destroyed?
    bool bNetStartup;                       // Level-placed vs spawned
    bool bNeedsDynamicSpawn;                // Needs respawn on load
    TSoftClassPtr<AActor> ActorSoftClass;   // Class for respawn
    TArray<FNarrativeSaveComponent> SavedComponents;
    TArray<uint8> ByteData;                 // All SaveGame properties
};
```

---

## 3. Father Companion Implications

### Current State Persistence

| Father State | Auto-Saved? | Notes |
|--------------|-------------|-------|
| Position | ✅ Yes | Transform saved |
| Health | ✅ Yes | If in `AttributesToSave` |
| Current Form | ❌ No | **MUST ADD** as SaveGame property |
| Form-specific abilities | ❌ No | Re-granted from AbilityConfiguration |
| Attached state | ❌ No | Tag lost, must reconstruct |
| Dome energy | ❌ No | Lost on load |
| Stealth timer | ❌ No | Lost on load |

### Recommended Solution

**Option A: SaveGame Property (Simplest)**
Add to `BP_FatherCompanion`:
```cpp
UPROPERTY(SaveGame, BlueprintReadWrite)
EFatherForm CurrentForm = EFatherForm::Crawler;
```

On BeginPlay/Load:
1. Read `CurrentForm` (auto-restored by save system)
2. Apply matching `AbilityConfiguration` (AC_FatherCrawler, etc.)
3. Tags restored via startup effects

**Option B: Custom Attribute**
Add `FatherForm` attribute to a custom AttributeSet with `NarrativeSaveAttribute` metadata.

**Option C: on_load Event Graph (Complex)**
Add generator support for `on_load:` section in abilities.
- More flexible but more work
- Probably overkill for Father's needs

---

## 4. Decision Matrix

| Approach | Complexity | Generator Work | Recommendation |
|----------|------------|----------------|----------------|
| SaveGame property on BP | LOW | None | **RECOMMENDED** |
| Custom attribute | LOW | None | Alternative |
| on_load event graphs | HIGH | Significant | Not needed |

---

## 5. Pending Actions

1. **User Action:** Watch Narrative Pro save system video
   - URL: https://www.youtube.com/watch?v=loI9uFbSDwM
   - May reveal additional capabilities or patterns

2. **After Video Review:** Decide on:
   - Is SaveGame property sufficient for Father?
   - Do we need on_load event graph support?
   - Any other save considerations from video?

3. **If SaveGame Property Chosen:**
   - Add `CurrentForm` property to BP_FatherCompanion
   - Add form restoration logic to BeginPlay
   - No generator changes needed
   - Close P2.2 as "Not Needed"

4. **If on_load Support Needed:**
   - Design manifest schema for `on_load:` section
   - Implement in FGameplayAbilityGenerator
   - Hook into INarrativeSavableComponent::Load

---

## 6. Key Source Files

| File | Content |
|------|---------|
| `NarrativeSaveSystem/Public/NarrativeSave.h` | FNarrativeActorRecord structure |
| `NarrativeSaveSystem/Public/NarrativeSavableComponent.h` | INarrativeSavableComponent interface |
| `NarrativeSaveSystem/Public/NarrativeSavableActor.h` | INarrativeSavableActor interface |
| `NarrativeSaveSystem/Public/Subsystems/NarrativeSaveSubsystem.h` | Save coordination |
| `NarrativeArsenal/Public/GAS/NarrativeAbilitySystemComponent.h` | ASC save/load |
| `NarrativeArsenal/Private/GAS/NarrativeAbilitySystemComponent.cpp` | Lines 671-740 |
| `NarrativeArsenal/Public/GAS/NarrativeAttributeSetBase.h` | Attribute definitions |
| `NarrativeArsenal/Public/UnrealFramework/NarrativeNPCCharacter.h` | NPC save interface |
| `NarrativeArsenal/Public/Spawners/NPCSpawnComponent.h` | Spawner persistence |

---

## 7. Conclusion

**Narrative Pro's save system is comprehensive.** Core state (position, health, inventory, equipment, factions) is automatically persisted for both player abilities and NPCs.

**The main gap for Father** is form persistence, which can be solved with a simple `SaveGame` property - no generator enhancement required.

**Recommendation:** Close P2.2 as "Not Needed" after video review confirms no additional requirements.

---

## Document History

| Date | Change |
|------|--------|
| 2026-01-28 | Created from Claude research session |
| 2026-01-28 | Pending user review of NP video |
