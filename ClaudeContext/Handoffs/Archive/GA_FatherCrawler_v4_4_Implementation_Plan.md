# GA_FatherCrawler v4.4 Implementation Plan

> **SUPERSEDED (v4.14.1):** Function names in this document are outdated. `BP_ApplyGameplayEffectToSelf` should be `BP_ApplyGameplayEffectToOwner` with `target_self: true`. See manifest.yaml for current implementation.

## STATUS: Ready for Execution

**Date:** January 2026
**Session:** Option B Form State Architecture Audit Implementation

---

## LOCKED DECISIONS (17 Total - Verified)

All changes must align with these locked decisions:

1. **Option B canonical** - Form identity via GE_*State (infinite-duration GE)
2. **Father.Form.* = orphan tags** - No GE grants these; cannot be used for gating
3. **Form identity via GE_*State** - Effect.Father.FormState.* tags
4. **Detached = absence of Attached** - No Father.State.Detached tag needed
5. **Guide v3.4 stale** - Requires rewrite
6. **Manifest tags correct** - activation_owned_tags removed as comment
7. **Event graph needs prelude** - Switch-back path missing prelude nodes
8. **Generator supports prelude** - v4.12.4 can generate prelude nodes
9. **Prelude is C (manifest)** - Not B (in guide); generator creates nodes
10. **bIsFirstActivation = spawn-vs-switch** - First activation skips transition
11. **VFX vehicle = C; mechanic = locked** - VFX automation is roadmap item
12. **Design Doc = conceptual scope** - Not implementation authority
13. **Option B is NP-canonical** - Matches EquippableItem pattern
14. **Cancellation is standard GAS** - cancel_abilities_with_tag property
15. **GC_ prefix matches NP** - NP uses GC_ for GameplayCues
16. **Cross-ASC gates already removed** - Verified in manifest
17. **Unequip cancellation safe** - ClearAbility cancels before remove

---

## TASK 1: Rewrite GA_FatherCrawler Guide (v3.4 → v4.4)

### Source Template
- `GA_FatherArmor_Implementation_Guide_v4_4.md` (1344 lines)

### Target File
- `GA_FatherCrawler_Implementation_Guide_v3_4.md` → rename to v4.4

### Changes Required

| Section | Old (v3.4) | New (v4.4) |
|---------|------------|------------|
| Header | v3.4, UE 5.6 | v4.4, UE 5.7, Option B reference |
| Form State Architecture | ActivationOwnedTags | GE_CrawlerState (Effect.Father.FormState.Crawler) |
| AUTOMATION VS MANUAL | Missing | Add table matching Armor pattern |
| PHASE 3 Tags | Father.Form.Crawler, Father.State.Detached in Activation Owned Tags | Remove - form identity via GE_CrawlerState |
| PHASE 4A Prelude | Missing | Add: Remove Effect.Father.FormState.*, Apply GE_CrawlerState |
| Quick Reference | ActivationOwnedTags with form tags | GE-based form identity |
| Multiplayer | ReplicateActivationOwnedTags for form identity | ReplicateActivationOwnedTags not needed for form identity |
| Prerequisites | GE_Invulnerable | GE_CrawlerState (Infinite duration) |

### Key Content Changes

**PHASE 3 - Remove Activation Owned Tags for Form Identity:**
```
OLD (v3.4):
  Activation Owned Tags:
    - Father.Form.Crawler
    - Father.State.Detached

NEW (v4.4):
  Activation Owned Tags:
    (none - form identity via GE_CrawlerState)
```

**PHASE 4A - Add Transition Prelude:**
```yaml
NEW (v4.4):
  1) TRANSITION PRELUDE: Remove Prior Form State (Option B)
     - Get Father ASC
     - BP_RemoveGameplayEffectFromOwnerWithGrantedTags(Effect.Father.FormState.*)

  2) TRANSITION PRELUDE: Apply New Form State (Option B)
     - Apply GE_CrawlerState
     - Result: Father has Effect.Father.FormState.Crawler tag
```

**Form State Architecture Table:**
```
| Component | Old (v3.4) | New (v4.4) |
|-----------|------------|------------|
| Form Identity | ActivationOwnedTags | GE_CrawlerState (Infinite-duration GE) |
| Identity Tag | Father.Form.Crawler | Effect.Father.FormState.Crawler |
| Persistence | Evaporates on EndAbility | Persists via infinite GE |
| Transition | Tags auto-swap | Remove old GE, apply new GE |
```

---

## TASK 2: Update Manifest Event Graph

### File
- `ClaudeContext/manifest.yaml`

### Section
- `GA_FatherCrawler.event_graph` (lines 830-900 approximately)

### Current State (lines 830-850)
```yaml
event_graph:
  nodes:
    - id: Event_Activate
    - id: GetAvatarActor
    - id: CastToFather
    - id: SetFatherRef
    # Missing: Transition prelude nodes
```

### Required Additions

Add to `nodes:` array (after Branch on bIsFirstActivation, in False path):
```yaml
# TRANSITION PRELUDE (Option B) - False branch (form switch)
- id: GetFatherASC_Transition
  type: CallFunction
  properties:
    function: GetAbilitySystemComponent
    target_self: false  # Target is FatherRef variable
  position: [900, 200]

- id: RemovePriorFormState
  type: CallFunction
  properties:
    function: BP_RemoveGameplayEffectFromOwnerWithGrantedTags
    class: UAbilitySystemBlueprintLibrary
  position: [1200, 200]

- id: ApplyGE_CrawlerState
  type: CallFunction
  properties:
    function: BP_ApplyGameplayEffectToSelf
    class: UAbilitySystemBlueprintLibrary
  position: [1500, 200]
```

Add to `connections:` array:
```yaml
# Transition prelude connections (False path)
- from: [Branch_FirstActivation, False]
  to: [GetFatherASC_Transition, Exec]
- from: [GetFatherASC_Transition, Exec]
  to: [RemovePriorFormState, Exec]
- from: [FatherRef, Value]
  to: [GetFatherASC_Transition, Target]
- from: [GetFatherASC_Transition, ReturnValue]
  to: [RemovePriorFormState, Target]
- from: [RemovePriorFormState, Exec]
  to: [ApplyGE_CrawlerState, Exec]
- from: [GetFatherASC_Transition, ReturnValue]
  to: [ApplyGE_CrawlerState, Target]
```

---

## TASK 3: Build Project

### Command
```bash
MSBuild "C:\Unreal Projects\NP22B57\NP22B57.sln" /t:Build /p:Configuration="Development Editor" /p:Platform=Win64
```

### Success Criteria
- Build completes with 0 errors
- Warnings acceptable if not related to GA_FatherCrawler

---

## TASK 4: Clean Assets Folder

### Path
```
C:\Unreal Projects\NP22B57\Content\FatherCompanion
```

### Command
```bash
rm -rf "C:/Unreal Projects/NP22B57/Content/FatherCompanion"
```

### Reason
- Asset Registry caches existing assets
- Code changes require fresh regeneration

---

## TASK 5: Regenerate Assets

### Command
```bash
powershell -ExecutionPolicy Bypass -File "C:\Unreal Projects\NP22B57\Plugins\GasAbilityGenerator\Tools\claude_automation.ps1" -Action cycle
```

### Success Criteria
- No generation errors
- GA_FatherCrawler created with transition prelude nodes
- GE_CrawlerState exists and is referenced

---

## EXECUTION ORDER

1. **Write Guide** - GA_FatherCrawler_Implementation_Guide_v4_4.md
2. **Update Manifest** - Add transition prelude nodes to event_graph
3. **Build** - MSBuild
4. **Clean** - Delete /Content/FatherCompanion/
5. **Regenerate** - cycle action
6. **Verify** - Check logs for errors

---

## RISK MITIGATION

| Risk | Mitigation |
|------|------------|
| Manifest syntax error | Validate YAML before build |
| Node connection mismatch | Use existing manifest patterns as reference |
| Missing GE_CrawlerState | Already exists in manifest (verified) |
| Build failure | Check logs, fix C++ errors |
| Generation failure | Check commandlet logs |

---

## VALIDATION CHECKLIST

After completion:
- [ ] Guide v4.4 has AUTOMATION VS MANUAL table
- [ ] Guide v4.4 has Option B transition prelude in PHASE 4A
- [ ] Guide v4.4 removed ActivationOwnedTags for form identity
- [ ] Guide v4.4 references Effect.Father.FormState.Crawler
- [ ] Guide v4.4 UE version is 5.7
- [ ] Manifest has transition prelude nodes
- [ ] Build succeeds
- [ ] Assets regenerate without errors

---

**END OF IMPLEMENTATION PLAN**
