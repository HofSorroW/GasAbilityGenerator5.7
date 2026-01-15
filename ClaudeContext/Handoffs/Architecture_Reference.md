# Architecture Reference

**Consolidated:** 2026-01-15
**Status:** Design documents for future enhancements

This document consolidates the Design Compiler and Spec DataAsset architecture handoffs.

---

## Overview

Two complementary approaches for improved authoring workflow:
1. **Design Compiler** - Pre-processing stage for high-level game design specs
2. **Spec DataAssets** - Native UE workflow with Content Browser integration

Both are deferred for future implementation after core generators are complete.

---

## 1. Design Compiler Architecture

### Purpose
Transform high-level, human-friendly game design specs into the flat manifest.yaml consumed by GasAbilityGenerator.

### Pipeline
```
DesignSpec/                    ┌─────────────────┐
├── globals/                   │    Compiler     │
├── items/                 ──► │  (resolve refs, │ ──► manifest.yaml ──► Generators
├── abilities/                 │   templates)    │
├── npcs/                      └─────────────────┘
└── quests/
```

### Features
- **Include System** - Split manifest across multiple files
- **Templates & Inheritance** - Base templates with override support
- **Variables & Constants** - $CONSTANT substitution
- **Conditional Blocks** - Build variants (demo vs full)
- **Pre-Generation Validation** - Reference checking, localization completeness

### Implementation Options
1. Python Compiler (~500-800 lines) - Recommended
2. C++ Compiler (in plugin) - More complex
3. Lightweight Include Only (~50 lines) - Good intermediate step

---

## 2. Spec DataAsset Workflow

### Purpose
Replace YAML-centric workflow with native UE DataAssets + Content Browser right-click generation.

### Benefits
- Zero UI development (uses Details panel)
- Free dropdowns (TSubclassOf asset pickers)
- Free validation (UPROPERTY meta tags)
- Free undo/redo (FScopedTransaction)
- Batch support via commandlet

### Spec Types
| Spec | Generates |
|------|-----------|
| UNPCPackageSpec | NPCDef_, AC_, ActConfig_, Goal_, Schedule_, DBP_, Quest_ |
| UQuestSpec | Quest_ |
| UItemSpec | EI_ |

### Template Inheritance
```
Specs/Templates/
└── NPC_Template_Merchant.uasset    ← Base template
    bIsVendor = true
    Schedule = [Work 6-18]

Specs/NPCs/
└── NPC_Blacksmith_Spec.uasset      ← Inherits
    ParentTemplate = NPC_Template_Merchant
    BaseName = "Blacksmith"          ← Override
```

### Context Menu Actions
- **Generate NPC Assets** - Creates all defined assets
- **Validate Spec** - Check for errors without generating
- **Preview Generation Plan** - Show what would be created

---

## Implementation Status

| Feature | Status | Priority |
|---------|--------|----------|
| Design Compiler | Deferred | Low |
| Spec DataAssets | Deferred | Low |
| Include System Only | Ready | Medium |

**Prerequisites:**
1. Complete core Narrative Pro generators
2. Manifest exceeds ~2000 lines
3. Multiple designers need simultaneous editing

---

## Original Documents (Consolidated)

- Design_Compiler_Architecture_Handoff.md (deleted)
- Spec_DataAsset_UX_Handoff.md (deleted)
