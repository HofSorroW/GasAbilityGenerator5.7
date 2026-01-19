# Generator Roadmap - Category C Features

**Version:** 1.0
**Date:** January 2026
**Status:** ROADMAP (Not Implemented)
**Related:** Form_State_Architecture_Audit_v1_0.md

---

## Overview

This document formalizes Category C automation features identified during the Form State Architecture Audit. Category C features are:
- **Not blockers** - Current generator works without them
- **Should be added** - Would improve developer experience
- **Feasible** - No engine blockers, implementation is straightforward

---

## Priority Tiers

| Tier | Timeline | Description |
|------|----------|-------------|
| P1 | Immediate | Core form state package support |
| P2 | Major Features | Delegate binding, save system integration |
| P3 | Enhancements | VFX automation, gameplay cues |

---

## P1: FormState Package Support (Immediate)

### P1.1 Manifest Schema Preset

**Goal:** Reduce boilerplate for form state GE definitions

**Current (Manual):**
```yaml
gameplay_effects:
  - name: GE_ArmorState
    folder: Effects/FormState
    duration_policy: Infinite
    granted_tags:
      - Effect.Father.FormState.Armor
      - State.Invulnerable
    asset_tag: Effect.Father.FormState.Armor
```

**Proposed (Preset):**
```yaml
form_state_effects:
  - form: Armor
    invulnerable: true  # Attached form
  - form: Crawler
    invulnerable: false  # Independent form
```

**Generator Expansion:**
```
form_state_effects[i] → GE_{form}State
  - folder: Effects/FormState
  - duration_policy: Infinite
  - granted_tags: [Effect.Father.FormState.{form}]
  - asset_tag: Effect.Father.FormState.{form}
  - if invulnerable: granted_tags.append(State.Invulnerable)
```

**Implementation:**
1. Add `FManifestFormStateEffectDefinition` struct
2. Add parser block for `form_state_effects:`
3. Expand to GE definitions before generation pass

### P1.2 Transition Prelude Validation

**Goal:** Detect manifest errors where form abilities don't follow Option B pattern

**Validation Rules:**
1. Every form ability must have `cancel_abilities_with_tag` for other forms
2. Every form ability must have `activation_required_tags` including `Father.State.Alive`, `Father.State.Recruited`
3. Every form ability must have `activation_blocked_tags` including `Father.State.Dormant`, `Father.State.Transitioning`

**Implementation:**
1. Add post-parse validation pass
2. Emit warnings for missing tags
3. Optionally emit errors in strict mode

### P1.3 Startup Effects Validation

**Goal:** Detect missing default form state

**Validation Rules:**
1. If `ability_configurations` contains form abilities (GA_Father*), at least one must have `startup_effects` with a GE_*State
2. Warn if no default form state is configured

**Implementation:**
1. Add validation in AbilityConfiguration generator
2. Emit warning with suggested fix

---

## P2: Major Features

### P2.1 Delegate Binding IR + Codegen

**Goal:** Automate delegate binding for OnAttributeChange, OnDied, OnDamagedBy, etc.

**Current Limitation:** Generator cannot create delegate binding nodes in Blueprint

**Proposed Manifest Syntax:**
```yaml
gameplay_abilities:
  - name: GA_FatherArmor
    delegate_bindings:
      - delegate: OnDied
        source: OwnerASC
        handler: HandleOwnerDied  # Custom Event name
      - delegate: OnAttributeChanged
        source: PlayerASC
        attribute: Health
        handler: HandleHealthChanged
```

**Required Generator Changes:**
1. Add `FManifestDelegateBinding` struct
2. Parse `delegate_bindings:` section
3. Create Custom Event nodes for handlers
4. Create BindToDelegate / AddDynamic nodes in ActivateAbility
5. Create Unbind nodes in EndAbility

**Complexity:** Major Feature - requires new node type generation

### P2.2 Save System Load Override

**Goal:** Support attribute/state restoration on game load

**Conditional:** Only needed if Narrative Pro's `NarrativeSavableComponent` doesn't persist ASC state

**Proposed Manifest Syntax:**
```yaml
gameplay_abilities:
  - name: GA_FatherArmor
    on_load:
      nodes:
        - id: RestoreFormState
          type: CallFunction
          properties:
            function: BP_ApplyGameplayEffectToSelf
            # ... restore GE
```

**Implementation:**
1. Add `on_load:` event_graph section (separate from main graph)
2. Hook into NarrativeSavableComponent.Load_Implementation

**Complexity:** Medium - depends on NP save system research

---

## P3: Enhancements

### P3.1 Niagara Spawning Support

**Status:** Generator supports Niagara system assets, not spawning

**Current:** GameplayCues are preferred for VFX (decoupled, data-driven)

**Proposed (Optional):**
```yaml
vfx_spawns:
  - id: FormTransitionVFX
    niagara_system: NS_FormTransition
    attach_to: Owner
    socket: spine_01
```

**Generator would:**
1. Create SpawnSystemAttached node
2. Store NiagaraComponent handle for cleanup
3. Deactivate in EndAbility

**Complexity:** Medium - requires NiagaraComponent lifecycle management

### P3.2 GameplayCue Auto-Wiring

**Status:** Tags exist, cue assets created manually

**Proposed:**
```yaml
gameplay_cues:
  - name: GC_FatherFormTransition
    tag: GameplayCue.Father.FormTransition
    parent_class: GameplayCueNotify_Burst
    # VFX config...

gameplay_abilities:
  - name: GA_FatherArmor
    cues:
      - trigger: OnActivate
        cue_tag: GameplayCue.Father.FormTransition
```

**Generator would:**
1. Create GameplayCue blueprint assets
2. Insert ExecuteGameplayCue nodes in ability graphs

**Complexity:** Medium - requires cue asset generation + node wiring

---

## Implementation Priority

| Feature | Priority | Effort | Value | Recommended |
|---------|----------|--------|-------|-------------|
| P1.1 FormState Preset | P1 | Low | Medium | Yes |
| P1.2 Transition Validation | P1 | Low | High | Yes |
| P1.3 Startup Validation | P1 | Low | High | Yes |
| P2.1 Delegate Binding | P2 | High | High | Future |
| P2.2 Load Override | P2 | Medium | Conditional | Research |
| P3.1 Niagara Spawning | P3 | Medium | Low | Optional |
| P3.2 GameplayCue Wiring | P3 | Medium | Medium | Optional |

---

## Acceptance Criteria

### P1 Features (When Implemented)
- [ ] `form_state_effects:` section parsed correctly
- [ ] Generated GE_*State assets have correct GrantedTags
- [ ] Validation emits warnings for missing activation tags
- [ ] Validation emits warnings for missing startup_effects
- [ ] All warnings include actionable fix suggestions

### P2 Features (When Implemented)
- [ ] Delegate bindings create valid Blueprint nodes
- [ ] Bound delegates are unbound in EndAbility
- [ ] Load override triggers on NarrativeSavableComponent.Load

### P3 Features (When Implemented)
- [ ] GameplayCue assets generated from manifest
- [ ] ExecuteGameplayCue nodes wired correctly
- [ ] VFX cleanup handled in EndAbility

---

## Notes

1. **P1 features are validation-only** - no code generation changes, just warnings
2. **P2 delegate binding is the biggest gap** - would eliminate most manual Blueprint work
3. **P3 features are nice-to-have** - GameplayCues can be created manually with low friction
4. **All features maintain backwards compatibility** - existing manifests work unchanged
