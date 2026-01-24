# Father Companion - Protective Dome Ability Implementation Guide
## VERSION 3.0 - Variable-Based Architecture (Manifest Automated)

**Document Purpose**: Complete reference for the Protective Dome ability for the Father companion in Armor form. This version documents the manifest-automated architecture using BP_FatherCompanion Blueprint variables instead of custom AttributeSets.

**System Overview**: When the father is in Armor form, the player accumulates Dome Energy from incoming damage using an **Energy-Only model** - the player takes full incoming damage (no reduction), while 30% of post-mitigation damage is converted to Dome Energy stored on BP_FatherCompanion. When the dome reaches maximum energy (500), the `Father.Dome.FullyCharged` tag is granted on the Player ASC, enabling the Q key to trigger a burst dealing 75 flat damage to all enemies within 500 units. The burst can also trigger automatically on form exit (T wheel) when fully charged. After bursting, the ability enters a 12-second cooldown.

---

## **DOCUMENT INFORMATION**

| Field | Value |
|-------|-------|
| Ability Name | GA_ProtectiveDome (passive) + GA_DomeBurst (active/auto) |
| Parent Class | NarrativeGameplayAbility |
| Form | Armor |
| Input | Automatic (passive) / Q Key (manual burst) |
| Version | 3.0 |
| Last Updated | January 2026 |
| Architecture | Variable-Based (v3.0) - Full manifest automation |

---

## **AUTHORITY NOTE**

This document is explanatory. Runtime behavior and generation are defined by `manifest.yaml`. In case of conflict, the manifest takes precedence.

---

## **ARCHITECTURE CHANGE LOG (v2.4 → v3.0)**

### What Changed

| Aspect | v2.4 (AS_DomeAttributes) | v3.0 (Variable-Based) |
|--------|--------------------------|----------------------|
| **Energy Storage** | AS_DomeAttributes (custom AttributeSet on Player ASC) | BP_FatherCompanion.DomeEnergy (Blueprint variable) |
| **Creation Method** | Gameplay Blueprint Attributes plugin (manual) | manifest.yaml actor_blueprints (auto-generated) |
| **Dependencies** | External plugin required | None |
| **Automation Status** | NOT automatable | Fully automatable |
| **Damage Detection** | Health attribute delegate on Player ASC | OnDamagedBy delegate binding (v4.21) |
| **Location** | Player ASC | Father Blueprint |

### Why Changed

1. **Decision to use Narrative's own attributes** - Custom AttributeSets require the Gameplay Blueprint Attributes plugin and manual setup
2. **Symbiote pattern alignment** - Symbiote uses UltimateChargeComponent + BP variables, not custom attributes
3. **Full automation** - Variable-based approach enables complete manifest automation
4. **Simpler architecture** - No cross-ASC attribute access needed

---

## **AUTOMATION STATUS**

| Feature | Automation Status | Notes |
|---------|-------------------|-------|
| BP_FatherCompanion variables | Auto-generated | DomeEnergy, MaxDomeEnergy, AbsorptionPercentage |
| GA_ProtectiveDome blueprint | Auto-generated | manifest.yaml gameplay_abilities section |
| GA_DomeBurst blueprint | Auto-generated | manifest.yaml gameplay_abilities section |
| Delegate binding (OnDamagedBy) | Auto-generated | v4.21 delegate_bindings pattern |
| HandleDomeDamageAbsorption logic | Auto-generated | Custom event in event_graph |
| Form exit burst (v4.27) | Auto-generated | EndAbility tries GA_DomeBurst |
| GE_DomeAbsorption | Auto-generated | Tags only (no attribute modifiers) |
| GE_DomeBurstCooldown | Auto-generated | 12s cooldown |
| GE_DomeBurstDamage | Auto-generated | SetByCaller damage |
| VFX (NS_DomeShield, NS_DomeBurst) | Auto-generated | Niagara system stubs |

**Manual Requirements:** None. All dome system assets are generated from manifest.yaml.

---

## **PHASE 1: GAMEPLAY TAGS**

All tags are auto-generated from manifest.yaml `tags` section.

### Required Tags

| Tag Name | Purpose |
|----------|---------|
| Father.Dome.Active | Dome is currently active (Activation Owned Tag on GA_ProtectiveDome) |
| Father.Dome.FullyCharged | Dome has reached 500 energy, burst available |
| Ability.Father.ProtectiveDome | Main dome management ability |
| Ability.Father.DomeBurst | Burst ability triggered at full charge |
| Effect.Father.DomeAbsorption | Effect active while dome is absorbing |
| Cooldown.Father.DomeBurst | Cooldown after dome burst |

---

## **PHASE 2: BP_FATHERCOMPANION VARIABLES**

Auto-generated from manifest.yaml `actor_blueprints` section.

### Dome Energy Variables

| Variable | Type | Default | Replicated | Purpose |
|----------|------|---------|------------|---------|
| DomeEnergy | Float | 0.0 | Yes | Current accumulated dome energy |
| MaxDomeEnergy | Float | 500.0 | No | Maximum dome capacity |
| AbsorptionPercentage | Float | 0.3 | No | % of damage converted to energy (30%) |

### Manifest Definition

```yaml
# BP_FatherCompanion variables (manifest.yaml lines 7141-7156)
actor_blueprints:
  - name: BP_FatherCompanion
    folder: Characters
    parent_class: NarrativeNPCCharacter
    variables:
      - name: DomeEnergy
        type: Float
        default: 0.0
        replicated: true
      - name: MaxDomeEnergy
        type: Float
        default: 500.0
        replicated: false
      - name: AbsorptionPercentage
        type: Float
        default: 0.3
        replicated: false
```

---

## **PHASE 3: GA_PROTECTIVEDOME ABILITY**

Auto-generated from manifest.yaml `gameplay_abilities` section.

### Ability Configuration

| Property | Value |
|----------|-------|
| Parent Class | NarrativeGameplayAbility |
| Instancing Policy | Instanced Per Actor |
| Net Execution Policy | Local Predicted |
| Ability Tags | Ability.Father.ProtectiveDome |
| Activation Owned Tags | Father.Dome.Active |

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| PlayerRef | NarrativePlayerCharacter | Cached player reference |
| FatherRef | BP_FatherCompanion | Cached father reference |
| DomeAbsorptionHandle | ActiveGameplayEffectHandle | Handle for GE_DomeAbsorption removal |

### Delegate Binding (v4.21 Pattern)

```yaml
delegate_bindings:
  - id: BindDomeDamageAbsorption
    delegate: OnDamagedBy              # Fires when player takes damage
    source: OwnerASC                   # Player's ASC (ability runs on player)
    handler: HandleDomeDamageAbsorption # Custom Event for absorption logic
```

**Note:** The generator creates RemoveDelegate nodes in EndAbility automatically via `bUnbindOnEnd = true` default.

### Event Graph: Activation Flow

```
Event_ActivateAbility
    → GetOwningActor
    → CastToFather (BP_FatherCompanion)
    → SetPlayerRef (from OwnerPlayer property)
    → SetFatherRef (from cast result)
    → MakeOutgoingGameplayEffectSpec (GE_DomeAbsorption)
    → BP_ApplyGameplayEffectSpecToSelf
    → SetDomeAbsorptionHandle
```

### Event Graph: HandleDomeDamageAbsorption (Damage Handler)

This custom event is called when the player takes damage via the OnDamagedBy delegate binding.

```
HandleDomeDamageAbsorption(DamageCauserASC, Damage, Spec)
    → GetFatherRef → IsValid → Branch
    [Valid Father]
    → Damage * AbsorptionPercentage (0.3) = AbsorbedDamage
    → GetCurrentEnergy (Father.DomeEnergy)
    → CurrentEnergy + AbsorbedDamage = NewEnergy
    → Clamp(NewEnergy, 0, MaxDomeEnergy)
    → SetDomeEnergy on Father
    → NewEnergy >= MaxDomeEnergy? → Branch
    [Fully Charged]
    → AddLooseGameplayTags (Father.Dome.FullyCharged) to Player
```

### Event Graph: EndAbility Flow (v4.27 Form Exit Burst)

```
Event_OnEndAbility
    → TryActivateAbilityByClass(GA_DomeBurst)  # Only succeeds if FullyCharged tag present
    → RemoveLooseGameplayTags (Father.Dome.FullyCharged)
    → RemoveActiveGameplayEffect (DomeAbsorptionHandle)
```

---

## **PHASE 4: GA_DOMEBURST ABILITY**

Auto-generated from manifest.yaml `gameplay_abilities` section.

### Ability Configuration

| Property | Value |
|----------|-------|
| Parent Class | NarrativeGameplayAbility |
| Instancing Policy | Instanced Per Actor |
| Net Execution Policy | Local Predicted |
| Input Tag | Narrative.Input.Ability1 (Q key) |
| Ability Tags | Ability.Father.DomeBurst |
| Activation Owned Tags | Father.State.Attacking |
| Activation Required Tags | Father.Dome.Active, Father.Dome.FullyCharged |
| Activation Blocked Tags | Cooldown.Father.DomeBurst |
| Cooldown GE | GE_DomeBurstCooldown |

### Variables

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| PlayerRef | NarrativePlayerCharacter | None | Player for location/targeting |
| BurstRadius | Float | 500.0 | AOE radius in units |
| BurstDamage | Float | 75.0 | Flat damage (no scaling) |

### Event Graph: Burst Flow

```
Event_ActivateAbility
    → GetOwningActor → CastToFather
    → GetOwnerPlayer → SetPlayerRef
    → GetPlayerLocation
    → SphereOverlapActors (radius: BurstRadius, type: Pawn)
    → ForEach Actor:
        → HasTag(Enemy)? → Branch
        [Is Enemy]
        → GetASC on enemy
        → MakeOutgoingGameplayEffectSpec (GE_DomeBurstDamage)
        → AssignTagSetByCallerMagnitude (Data.Damage.DomeBurst, BurstDamage)
        → ApplyGameplayEffectSpec to enemy
    → CommitAbilityCooldown
    → EndAbility
```

---

## **PHASE 5: GAMEPLAY EFFECTS**

All auto-generated from manifest.yaml `gameplay_effects` section.

### GE_DomeAbsorption

| Property | Value |
|----------|-------|
| Duration Policy | Infinite |
| Granted Tags | Father.Dome.Active, Effect.Father.DomeAbsorption |

**Note:** This effect grants state tags only. Damage absorption logic is handled by the delegate binding and HandleDomeDamageAbsorption event.

### GE_DomeBurstCooldown

| Property | Value |
|----------|-------|
| Duration Policy | Has Duration |
| Duration Magnitude | 12.0 seconds |
| Granted Tags | Cooldown.Father.DomeBurst |

### GE_DomeBurstDamage

| Property | Value |
|----------|-------|
| Duration Policy | Instant |
| Executions | NarrativeDamageExecCalc |
| SetByCaller Tag | Data.Damage.DomeBurst |

---

## **PHASE 6: EQUIPMENT INTEGRATION**

GA_ProtectiveDome and GA_DomeBurst are granted via EI_FatherArmorForm equipment.

### EI_FatherArmorForm Configuration

```yaml
equippable_items:
  - name: EI_FatherArmorForm
    folder: Items/Forms
    parent_class: EquippableItem
    equipment_slot: Narrative.Equipment.Slot.FatherForm
    abilities_to_grant:
      - GA_FatherArmor
      - GA_ProtectiveDome
      - GA_DomeBurst
    equipment_modifier_ge: GE_EquipmentModifier_FatherArmor
    armor_rating: 50.0
```

**Note:** When armor form is unequipped, GA_ProtectiveDome ends and tries to activate GA_DomeBurst via EndAbility (v4.27 Decisions 22-24).

---

## **PHASE 7: ENERGY RESET**

### When Dome Bursts (GA_DomeBurst)

Energy reset happens automatically when GA_DomeBurst activates:
1. GA_DomeBurst activation requires Father.Dome.FullyCharged tag
2. After burst loop completes, energy is at max (500)
3. When form exits, GA_ProtectiveDome.EndAbility clears the FullyCharged tag
4. On next form entry, DomeEnergy remains on Father but FullyCharged tag is gone

### Manual Energy Reset (If Needed)

To reset energy without bursting (e.g., on player death), add a node to set Father.DomeEnergy = 0:

```
Get Father Reference
    → PropertySet (DomeEnergy = 0.0) on BP_FatherCompanion
```

---

## **VARIABLE SUMMARY**

### GA_ProtectiveDome Variables

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| PlayerRef | NarrativePlayerCharacter | None | Cached player reference |
| FatherRef | BP_FatherCompanion | None | Cached father reference |
| DomeAbsorptionHandle | ActiveGameplayEffectHandle | None | GE cleanup handle |

### GA_DomeBurst Variables

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| PlayerRef | NarrativePlayerCharacter | None | No |
| BurstRadius | Float | 500.0 | Yes |
| BurstDamage | Float | 75.0 | Yes |

### BP_FatherCompanion Dome Variables

| Variable | Type | Default | Replicated |
|----------|------|---------|------------|
| DomeEnergy | Float | 0.0 | Yes |
| MaxDomeEnergy | Float | 500.0 | No |
| AbsorptionPercentage | Float | 0.3 | No |

---

## **TAG CONFIGURATION SUMMARY**

### GA_ProtectiveDome Tags

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.ProtectiveDome |
| Activation Owned Tags | Father.Dome.Active |

### GA_DomeBurst Tags

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Father.DomeBurst |
| Activation Required | Father.Dome.Active, Father.Dome.FullyCharged |
| Activation Owned | Father.State.Attacking |
| Activation Blocked | Cooldown.Father.DomeBurst |
| Input Tag | Narrative.Input.Ability1 |

---

## **DAMAGE MODEL REFERENCE**

| Parameter | Value | Notes |
|-----------|-------|-------|
| Absorption Rate | 30% | Player takes full damage, 30% converts to energy |
| Max Energy | 500 | Father.Dome.FullyCharged granted at max |
| Burst Damage | 75 (flat) | Does not scale with energy |
| Burst Radius | 500 units | AOE around player |
| Cooldown | 12 seconds | After burst |
| Trigger | FULL only | Father.Dome.FullyCharged required |

---

## **COMPARISON: v2.4 vs v3.0**

### v2.4 (AS_DomeAttributes - Deprecated)

```
Player ASC
├── AS_DomeAttributes (custom)
│   ├── DomeEnergy
│   ├── MaxDomeEnergy
│   └── AbsorptionPercentage
└── Health attribute delegate → OnHealthChanged → Calculate absorption
```

**Problems:**
- Required Gameplay Blueprint Attributes plugin
- Manual AttributeSet creation
- Cross-ASC complexity (dome owned by player, father abilities)
- Not automatable via manifest

### v3.0 (Variable-Based - Current)

```
BP_FatherCompanion
├── DomeEnergy (variable)
├── MaxDomeEnergy (variable)
└── AbsorptionPercentage (variable)

Player ASC
└── OnDamagedBy delegate → HandleDomeDamageAbsorption → Update Father.DomeEnergy
```

**Benefits:**
- No external plugin dependency
- Fully manifest-automated
- Simpler architecture
- Follows Symbiote pattern (component/variable-based tracking)

---

## **RELATED DOCUMENTS**

| Document | Version | Purpose |
|----------|---------|---------|
| manifest.yaml | Current | Single source of truth |
| GA_DomeBurst_Implementation_Guide | v2.11 | Separate burst ability details |
| Father_Companion_Technical_Reference | v6.0 | Complete system reference |
| Delegate_Binding_Extensions_Spec | v1.1 (LOCKED) | v4.21 delegate pattern |
| Father_Companion_GAS_Audit_Locked_Decisions | v2.0 | GAS patterns and decisions |

---

## **CHANGELOG**

### Version 3.0 - January 2026 (Variable-Based Architecture)

| Change | Description |
|--------|-------------|
| Architecture | Complete rewrite from AS_DomeAttributes to BP_FatherCompanion variables |
| Removed | All Gameplay Blueprint Attributes plugin references |
| Removed | AS_DomeAttributes creation (PHASE 3 in v2.4) |
| Removed | Health attribute delegate approach |
| Added | BP_FatherCompanion.DomeEnergy, MaxDomeEnergy, AbsorptionPercentage |
| Added | HandleDomeDamageAbsorption custom event via delegate binding |
| Added | Full manifest definitions for all dome assets |
| Added | Automation status section |
| Updated | Energy location: Player ASC → Father Blueprint |
| Updated | Damage detection: Health delegate → OnDamagedBy delegate binding |

### Version 2.4 - January 2026 (Decisions 22-24) - DEPRECATED

| Change | Description |
|--------|-------------|
| Decision 22 | Form exit burst via TryActivateAbilityByClass(GA_DomeBurst) |
| Decision 24 | Father.Dome.FullyCharged added to GA_DomeBurst activation_required_tags |
| Damage Model | Energy-Only model - player takes full damage |

---

**END OF IMPLEMENTATION GUIDE**

**VERSION 3.0 - Variable-Based Architecture (Manifest Automated)**

**Compatible with Unreal Engine 5.7 + Narrative Pro v2.2**

**Blueprint-Only Implementation - Full Manifest Automation**
