# TSoftClassPtr "_C" Suffix Investigation

**Date:** 2026-01-15
**Version:** v4.2.11
**Status:** Documented - Pending Decision

## Problem Discovery

When syncing NPCDefinitions in the NPC Table Editor, the Blueprint column displayed `BP_NarrativeNPC_C` instead of `BP_NarrativeNPC`.

**Root Cause:** Using `TSoftClassPtr::ToSoftObjectPath()` resolves to the UClass path (with "_C" suffix), not the Blueprint asset path.

**Fix Applied:** Changed to `TSoftClassPtr::ToString()` which returns the raw stored path.

## Deeper Investigation

Found **7 locations** in generators that ADD "_C" suffix when storing to `TSoftClassPtr` properties. This creates inconsistency with how NarrativePro stores these references.

### Evidence

NarrativePro's Seth NPCDefinition stores:
```
/NarrativePro/Pro/Core/AI/BP/BP_NarrativeNPC.BP_NarrativeNPC
```
(NO "_C" suffix)

Our generator creates:
```
/Game/Project/NPCs/BP_Name.BP_Name_C
```
(WITH "_C" suffix - WRONG)

### Affected Locations (GasAbilityGeneratorGenerators.cpp)

| Line | Property | Code Pattern |
|------|----------|--------------|
| 12207-12227 | `NPCDef->NPCClassPath` | Adds "_C" to path |
| 12264-12284 | `NPCDef->Dialogue` | Adds "_C" to path |
| 12411-12416 | Auto-create Dialogue | Adds "_C" to path |
| 12764-12772 | `TaggedDialogueSet.Dialogue` | Adds "_C" to path |
| 11192-11202 | ItemCollection Item | Adds "_C" to path |
| 11213-11223 | ItemCollection Item (2nd) | Adds "_C" to path |
| 12013-12023 | Loot table Item | Adds "_C" to path |

### Why This Matters

1. **Inconsistency:** Generated assets differ from NarrativePro's format
2. **Editor Display:** Paths with "_C" may display incorrectly in property panels
3. **Potential Runtime Issues:** Unknown if UE handles both formats identically

### Key Distinction

**LoadClass/LoadObject (CORRECT to use "_C"):**
```cpp
// Loading a UClass - NEEDS "_C" because you're loading the class object
UClass* Class = LoadClass<T>(nullptr, TEXT("Path/Asset.Asset_C"));
```

**TSoftClassPtr Storage (INCORRECT to use "_C"):**
```cpp
// Storing a reference - should NOT have "_C"
// UE resolves internally when loading
SoftPtr = TSoftClassPtr<T>(FSoftObjectPath(TEXT("Path/Asset.Asset")));
```

### Recommended Fix

Remove "_C" suffix addition from all 7 TSoftClassPtr storage locations.

### Risk Assessment

- **Low Risk:** NarrativePro stores without "_C", so this aligns with expected format
- **Medium Risk:** May break existing generated assets that were stored with "_C"
- **Mitigation:** Regenerate assets after fix, or UE may resolve both formats

### Decision Needed

Proceed with removing "_C" from TSoftClassPtr storage locations?
