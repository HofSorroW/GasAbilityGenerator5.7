# TSoftClassPtr "_C" Suffix Investigation

**Date:** 2026-01-15
**Version:** v4.2.12
**Status:** FIXED - All 7 locations corrected

## Problem Discovery

When syncing NPCDefinitions in the NPC Table Editor, the Blueprint column displayed `BP_NarrativeNPC_C` instead of `BP_NarrativeNPC`.

**Root Cause:** Using `TSoftClassPtr::ToSoftObjectPath()` resolves to the UClass path (with "_C" suffix), not the Blueprint asset path.

**Fix Applied:** Changed to `TSoftClassPtr::ToString()` which returns the raw stored path.

## Deeper Investigation

Found **7 locations** in generators that were incorrectly adding "_C" suffix when storing to `TSoftClassPtr` properties. This created inconsistency with how NarrativePro stores these references.

### Evidence

NarrativePro's Seth NPCDefinition stores:
```
/NarrativePro/Pro/Core/AI/BP/BP_NarrativeNPC.BP_NarrativeNPC
```
(NO "_C" suffix)

Our generator was creating:
```
/Game/Project/NPCs/BP_Name.BP_Name_C
```
(WITH "_C" suffix - WRONG)

### Fixed Locations (GasAbilityGeneratorGenerators.cpp)

| Line | Property | Status |
|------|----------|--------|
| 12207-12227 | `NPCDef->NPCClassPath` | FIXED - strips "_C" if present |
| 12264-12284 | `NPCDef->Dialogue` | FIXED - strips "_C" if present |
| 12411-12416 | Auto-create Dialogue | FIXED - builds path without "_C" |
| 12764-12772 | `TaggedDialogueSet.Dialogue` | FIXED - strips "_C" if present |
| 11192-11202 | ItemCollection Item | FIXED - builds path without "_C" |
| 11213-11223 | ItemCollection Item (2nd) | FIXED - builds path without "_C" |
| 12013-12023 | Loot table Item | FIXED - strips "_C" if present |

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

### Resolution (v4.2.12)

**FIXED** - All 7 TSoftClassPtr storage locations now store paths WITHOUT "_C" suffix.

**Test Results:**
- 159 assets generated successfully
- 0 failures
- 1 deferred (unrelated manifest dependency issue)

**Changes Made:**
- ItemCollection: 2 locations
- NPCDefinition: NPCClassPath, Dialogue, auto-create Dialogue (3 locations)
- TaggedDialogueSet: Dialogue (1 location)
- Loot table items: 1 location

---

## NPC Table Editor Display Fix (v4.2.13)

**Issue:** After v4.2.12 generator fix, NPC Table Editor Blueprint column still showed "_C" suffix.

**Root Cause:** Using `GetAssetName()` on FSoftObjectPath from TSoftClassPtr returns the ObjectName (UClass name with "_C"), not the Blueprint asset name.

**Key Technical Insight:**
```
FSoftObjectPath format: "/Package/Path/BP_Name.BP_Name_C"
                        ├─ GetAssetPath() -> "/Package/Path/BP_Name" (Blueprint asset)
                        └─ GetAssetName() -> "BP_Name_C" (UClass object)
```

**Fix Applied:** Use `FPaths::GetBaseFilename(Path.GetAssetPath().ToString())` to extract Blueprint name.

### Fixed Locations (SNPCTableEditor.cpp)

| Location | Fix Applied |
|----------|-------------|
| `GetAssetDisplayName()` helper | Added `bIsClassPath` parameter for TSoftClassPtr paths |
| Blueprint cell display lambda | Changed to use `GetAssetPath()` |
| `GetColumnValue()` Blueprint | Passes `true` to `GetAssetDisplayName()` |
| Confirmation dialogs (2x) | Use `GetAssetPath()` for display |
| CSV export Blueprint column | Use `GetAssetPath()` |

### Codebase Audit (v4.2.13)

**Audited Files:**
- `SNPCTableEditor.cpp` - All GetAssetName() calls reviewed
- `GasAbilityGeneratorGenerators.cpp` - All TSoftClassPtr operations reviewed
- `SDialogueTableEditor.cpp` - No GetAssetName() usage (clean)
- `SQuestEditor.cpp` - No asset path issues (clean)

**Findings:**
- `GetAssetName()` on DataAsset paths (TSoftObjectPtr) is correct
- `GetAssetName()` on Blueprint paths (TSoftClassPtr) returns "_C" - must use GetAssetPath()
- All generator storage locations correctly strip "_C" (v4.2.12 fix verified)
- Hash functions using GetAssetName() are consistent with stored data (acceptable)

**No Additional Issues Found.**
