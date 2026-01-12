# Narrative Pro Asset Strategy Log

**Date:** 2026-01-12
**Context:** Discussion about optimizing GasAbilityGenerator to work WITH Narrative Pro rather than duplicating its assets

---

## Key Insight

Instead of generating everything from scratch, we should:
1. **Catalog** what Narrative Pro already provides as ready-made assets
2. **Reference** NP assets directly when they fit our needs
3. **Generate** only project-specific assets that fill gaps

---

## Narrative Pro Built-in Assets (REFERENCE, don't generate)

| Type | Count | Examples | Usage |
|------|-------|----------|-------|
| **GA_** | 46+ | `GA_Melee_Unarmed`, `GA_Attack_Magic_Proj`, `GA_Sprint`, `GA_Jump` | **Subclass** for Father abilities |
| **GE_** | 41+ | `GE_Damage_SetByCaller`, `GE_EquipmentModifier`, `GE_Invulnerable` | **Reference** or copy patterns |
| **BT_** | 15 | `BT_FollowCharacter`, `BT_Attack_Melee`, `BT_Patrol`, `BT_Idle` | **Reference** for Father AI |
| **BB_** | 11 | `BB_FollowCharacter`, `BB_Attack`, `BB_Idle` | **Reference** directly |
| **BPA_** | 17 | `BPA_FollowCharacter`, `BPA_Attack_Melee`, `BPA_Flee`, `BPA_Idle` | **Subclass** for Father activities |
| **AC_** | 15 | `AC_NPC_Default`, `AC_Melee`, `AC_Marksman`, `AC_Pacifist` | **Reference** or customize |
| **Goal_** | 8 | `Goal_Attack`, `Goal_FollowCharacter`, `Goal_Idle`, `Goal_Patrol` | **Reference** directly |
| **GoalGenerator_** | 4 | `GoalGenerator_Attack`, `GoalGenerator_Flee`, `GoalGenerator_Interact` | **Reference** directly |
| **IA_** | 28+ | `IA_Attack`, `IA_Move`, `IA_Ability1`, `IA_Interact` | **Use directly** |
| **IMC_** | 1 | `IMC_Default` | **Use or extend** |
| **BTS_** | 16 | `BTS_SetAIFocus`, `BTS_Attack`, `BTS_AdjustFollowSpeed` | **Reference** directly |
| **BTT_** | 7 | `BTT_EquipWeapon`, `BTT_SetBehaviorTree`, `BTT_SetCrouch` | **Reference** directly |
| **EQS_** | 10 | `EQS_Actor_SensedAttackTarget`, `EQS_Move_MeleeAttack` | **Reference** directly |
| **WBP_** | 50+ | Base widgets, HUD elements, inventory, dialogue UI | **Subclass** for custom UI |

### NP Asset Locations
- Abilities: `/NarrativePro22B57/Content/Pro/Core/Abilities/`
- AI System: `/NarrativePro22B57/Content/Pro/Core/AI/`
- Input: `/NarrativePro22B57/Content/Pro/Core/Data/Input/`
- UI: `/NarrativePro22B57/Content/Pro/Core/UI/`

---

## Gaps Our Generator Fills (GENERATE these)

| Type | What to Generate | Why |
|------|------------------|-----|
| **NPCDef_** | `NPCDef_Father`, enemy definitions | NP has none - project specific |
| **CD_** | Character definitions | Project-specific characters |
| **ActConfig_** | `ActConfig_Father` | NP has none - project specific |
| **Custom GA_** | `GA_FatherAttack`, `GA_FatherLaserShot` | Father-specific (subclass NP's GA_) |
| **Custom GE_** | `GE_FatherFormStats`, form effects | Father-specific configurations |
| **Custom BPA_** | `BPA_FatherFollow`, `BPA_FormationFollow` | Father-specific (subclass NP's BPA_) |
| **EI_** | `EI_FatherCrawlerForm`, form items | Father form equipment |
| **NE_** | `NE_FatherAwakens` | Story events |
| **DBP_** | `DBP_FatherDialogue` | Father conversations |
| **Custom WBP_** | `WBP_FormWheel`, `WBP_MarkIndicator` | Father-specific UI |
| **Tags** | `Father.*`, `Ability.Father.*` | Project gameplay tags |
| **E_** | `E_FatherForm` | Project enumerations |

---

## Code Status: NO DELETION NEEDED

The existing GasAbilityGenerator code is **additive** and works alongside Narrative Pro:

1. **Generators only create what's in manifest.yaml** - if you don't define it, it won't generate
2. **parent_class field supports NP inheritance** - `GA_FatherAttack` can extend `GA_Melee_Unarmed`
3. **Reference vs Generate is a manifest decision**, not a code decision

### Example Hybrid Approach
```yaml
# Reference NP's existing BT directly (no generation)
behavior_trees:
  # Don't add BT_FollowCharacter - use NP's directly

# Generate Father-specific ability that EXTENDS NP's base
gameplay_abilities:
  - name: GA_FatherAttack
    parent_class: GA_Melee_Unarmed    # Inherits NP's motion warping!
    folder: Abilities/Father
    # ... Father-specific configuration

# Reference NP's AC_ configs or create custom
ability_configurations:
  - name: AC_FatherCrawler
    abilities: [GA_FatherAttack, GA_FatherLaserShot]
```

---

## Action Items (TODO)

- [ ] Review current manifest.yaml and identify assets that duplicate NP functionality
- [ ] Update manifest to reference NP assets where appropriate
- [ ] Document which NP base classes Father assets should extend
- [ ] Create a "parent class reference" section in CLAUDE.md mapping Father assets to NP bases

---

## Summary

**Strategy:** Work WITH Narrative Pro, not around it
- **Keep** all generator code (v3.6)
- **Reduce** manifest entries by referencing NP built-ins
- **Generate** only project-specific assets that extend NP's foundation
