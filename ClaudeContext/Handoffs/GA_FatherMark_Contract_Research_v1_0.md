# GA_FatherMark Contract Research v1.0
## UE/Narrative Pro Pattern Investigation
## Date: January 2026

---

## Executive Summary

Before locking contracts for GA_FatherMark, both auditors must agree on implementation patterns. This document presents research findings from:
- UE5.7 GAS Documentation
- Narrative Pro Source Code
- Epic Games Best Practices

**Key Finding:** The current "mailbox" pattern (PendingMarkTarget) is non-standard. GAS best practice is `SendGameplayEventToActor` with target data in payload.

---

## Sources Consulted

| Source | URL/Location | Relevance |
|--------|--------------|-----------|
| GAS Documentation (tranek) | https://github.com/tranek/GASDocumentation | Best practices |
| Epic GAS Docs (UE 5.7) | https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-abilities-in-unreal-engine | Official reference |
| Widget Components Docs | https://dev.epicgames.com/documentation/en-us/unreal-engine/widget-components-in-unreal-engine | UI patterns |
| Epic Forums | https://forums.unrealengine.com/t/solved-widgetcomponent-in-world-space-always-visible-to-player/136782 | Wall visibility |
| NarrativeAbilitySystemComponent.cpp | NarrativePro22B57/Source/ | Token system |

---

## Proposal 1: C_MARK_MAILBOX_SAFETY

### Current Implementation (Mailbox Pattern)
```
BP_FatherCompanion.MarkEnemy(Target)
  → Set PendingMarkTarget = Target
  → TryActivateAbilityByClass(GA_FatherMark)
GA_FatherMark.Activate
  → Read PendingMarkTarget from owner
  → Copy to local TargetEnemy
  → Apply GE
```

### GAS Best Practice (GameplayEvent Pattern)
```
BP_FatherCompanion.MarkEnemy(Target)
  → Create FGameplayEventData Payload
  → Payload.Target = Target  (TARGET IN PAYLOAD)
  → SendGameplayEventToActor(Father, EventTag, Payload)
GA_FatherMark (triggered by event)
  → Read Target from event payload
  → Apply GE
```

### Comparison

| Aspect | Mailbox | GameplayEvent |
|--------|---------|---------------|
| Target passing | External variable | In payload |
| Race condition | Possible if same-frame calls | None |
| GAS Standard | Non-standard | Best practice ✅ |
| Replication | Manual handling needed | Built-in support |

### Evidence from Narrative Pro

NarrativeAbilitySystemComponent.cpp uses GameplayEvent pattern for death handling:

```cpp
// Lines 120-131
FGameplayEventData Payload;
Payload.EventTag = FNarrativeGameplayTags::Get().GameplayEvent_Death;
Payload.Instigator = DamageInstigator;
Payload.Target = GetAvatarActor();
Payload.OptionalObject = DamageEffectSpec.Def;
Payload.ContextHandle = DamageEffectSpec.GetEffectContext();
Payload.InstigatorTags = *DamageEffectSpec.CapturedSourceTags.GetAggregatedTags();
Payload.TargetTags = *DamageEffectSpec.CapturedTargetTags.GetAggregatedTags();
Payload.EventMagnitude = DamageMagnitude;
HandleGameplayEvent(Payload.EventTag, &Payload);
```

### Recommendation

**Do NOT lock the mailbox pattern.** Instead:
1. Refactor to use SendGameplayEventToActor
2. Add trigger tag `GameplayEvent.Father.Mark` to GA_FatherMark
3. Pass target in FGameplayEventData.Target
4. Then lock the GameplayEvent pattern as the standard

---

## Proposal 2: C_MARK_MAX_CONCURRENCY_3

### Reference Pattern: Narrative Pro Attack Token System

NarrativeAbilitySystemComponent.cpp (lines 389-549) implements a "limited concurrent slots with rotation" pattern that is directly applicable to mark limiting.

### Token System Components

| Component | Code Location | Mark System Equivalent |
|-----------|---------------|------------------------|
| Max limit | `GetNumAttackTokens()` | `MaxMarks` variable |
| Tracking array | `GrantedAttackTokens` | `MarkedEnemies` |
| Claim check | `TryClaimToken()` line 393 | Before applying mark |
| Steal/rotate | `CanStealToken()` line 492 | Remove oldest valid |
| Validation | `ShouldImmediatelyStealToken()` line 487 | Check null/dead |
| Cleanup | `ReturnToken()` line 467 | On enemy death |

### Key Code Patterns

**Validation (line 487-490):**
```cpp
bool UNarrativeAbilitySystemComponent::ShouldImmediatelyStealToken(const FAttackToken& Token) const
{
    return !Token.Owner || !Token.Owner->IsAlive() || !Token.Owner->GetPawn();
}
```

**Limit Check (line 441):**
```cpp
if (GrantedAttackTokens.Num() < GetNumAttackTokens())
{
    // Safe to add
}
```

### Recommendation

Lock contract C_MARK_MAX_CONCURRENCY_3 with explicit reference to Token System pattern:

```
PASS if:
1. MarkedEnemies.Num() checked against MaxMarks before adding
2. When at limit, oldest VALID entry removed (skip null/dead)
3. Validation uses pattern: !Actor || !IsValid(Actor) || Actor.IsDead

FAIL if:
- Exceeds limit without removal
- Removes arbitrary (non-oldest) entry
- Doesn't validate existing entries
```

---

## Proposal 3: C_MARK_REFRESH_SEMANTICS

### GE Stacking Handles Duration

GAS Gameplay Effects have built-in stacking policies:

| Policy | Behavior | Use Case |
|--------|----------|----------|
| Aggregate by Source | Separate stacks per source | DOTs from different casters |
| Aggregate by Target | Single stack, count increases | Stack-based buffs |
| **Refresh on Successful Application** | Resets duration | **Mark system** ✅ |
| Override | Replaces entirely | Strongest-wins buffs |

### GE_MarkEffect Configuration

From manifest (lines 819-831):
```yaml
- name: GE_MarkEffect
  duration_policy: HasDuration
  duration_setbycaller: Data.Mark.Duration
  # Stacking should be: Refresh on Successful Application
```

### Widget Refresh (Separate Concern)

GE duration refresh is automatic via stacking policy. Widget refresh is separate:
1. Find existing widget in MarkWidgetMap
2. Call refresh function on widget
3. Do NOT spawn duplicate

### Recommendation

Contract should verify:
```
PASS if:
1. GE stacking policy = Refresh on Successful Application
2. Re-mark flow finds existing widget (not spawn new)
3. Widget.RefreshMark() called on re-hit
4. MarkWidgetMap lookup before spawn

FAIL if:
- Duplicate widgets for same target
- Duration doesn't extend on re-hit
```

---

## Proposal 4: C_MARK_UI_WALL_VISIBLE

### Widget Component Space Modes

| Mode | Visibility | Occlusion |
|------|------------|-----------|
| **Screen** | Always on top | None (through walls) ✅ |
| World | In 3D space | By geometry |

### Epic Documentation Quote

> "Screen space widgets always render on top (visible through everything)"

### Implementation Options

**Option A: Screen Space Widget Component**
```cpp
WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
WidgetComponent->SetDrawSize(FVector2D(32, 32));
```

**Option B: HUD Element Tracking World Position**
- Widget in player HUD
- Project 3D position to screen each frame
- More control but more complex

### Recommendation

For simplicity, use Screen Space Widget Component:
```
PASS if:
1. WidgetComponent created with SetWidgetSpace(Screen)
2. OR HUD-based tracking with proper occlusion handling

FAIL if:
- World Space widget without custom depth/stencil
- No visibility-through-walls guarantee
```

---

## Current Manifest Gap Analysis

| Feature | Contract Requires | Manifest Has |
|---------|-------------------|--------------|
| GameplayEvent trigger | Target in payload | Mailbox pattern |
| Max marks check | Array length vs limit | Variables declared, not used |
| Oldest removal | Validation + removal | Not implemented |
| Widget spawning | AddComponent + configure | Not implemented |
| Widget refresh | Find + call refresh | Not implemented |
| OnDied binding | Delegate cleanup | Not implemented |

### Critical Question

**All proposed contracts would immediately FAIL on current manifest.**

Options:
1. Complete manifest first, then lock contracts
2. Lock contracts as "aspirational" (force implementation)
3. Revise contracts to match current (simpler) implementation

---

## Questions for Cross-Examination

### To GPT:

1. **Mailbox vs GameplayEvent:** Do you agree SendGameplayEventToActor is superior? Should we recommend architecture change before locking?

2. **Token System Reference:** Should C_MARK_MAX_CONCURRENCY_3 explicitly cite the Attack Token pattern as implementation model?

3. **Manifest First:** Given all contracts would FAIL current manifest, should we complete implementation before locking?

### To Erdem:

1. **Architecture Decision:** Keep mailbox pattern or refactor to GameplayEvent?

2. **Manifest Completeness:** Is manifest intentionally simplified, or should mark limits/widgets be added?

3. **Contract Timing:** Lock now (aspirational) or after implementation?

---

## References

- [GAS Documentation - tranek/GitHub](https://github.com/tranek/GASDocumentation)
- [Epic GAS Documentation (UE 5.7)](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-abilities-in-unreal-engine)
- [Widget Components Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/widget-components-in-unreal-engine)
- [Epic Forums - Widget Visibility](https://forums.unrealengine.com/t/solved-widgetcomponent-in-world-space-always-visible-to-player/136782)

---

**END OF RESEARCH DOCUMENT**

**Status:** Awaiting GPT cross-examination and Erdem decisions
**Next Action:** GPT review of findings, then alignment on implementation approach
