# Father Companion Guide Format Reference
## For Claude - Implementation Guide Formatting Standards
## Version 3.0

---

## DOCUMENT INFORMATION

| Field | Value |
|-------|-------|
| Document Type | Format Reference |
| Last Updated | January 2026 |
| Version | 3.0 |
| Purpose | Formatting standards for all Father Companion implementation guides |

---

## DOCUMENT STRUCTURE

### Required Sections (In Order)

| Section | Purpose |
|---------|---------|
| Title Block | Guide name, version, date, engine/plugin versions |
| DOCUMENT INFORMATION | Table with guide metadata |
| TABLE OF CONTENTS | Phase list only |
| INTRODUCTION | Brief ability overview tables |
| QUICK REFERENCE | Summary tables (tags, variables, assets) |
| PHASE 1-N | Implementation steps |
| CHANGELOG | Version history at end |

### Phase Structure Template

| Phase | Typical Name | Contents |
|-------|--------------|----------|
| PHASE 1 | VERIFY GAMEPLAY TAGS | Tag existence checks |
| PHASE 2 | BLUEPRINT CREATION | Asset creation |
| PHASE 3 | ABILITY PROPERTIES | Class Defaults configuration |
| PHASE 4 | ABILITY LOGIC | Event Graph nodes |
| PHASE 5 | GAMEPLAY EFFECTS | GE creation (if needed) |
| PHASE 6 | INTEGRATION | System hookup |
| PHASE 7 | INPUT CONFIGURATION | Input setup (if applicable) |

---

## HEADER FORMAT

### Header Hierarchy

| Level | Markdown | Example |
|-------|----------|---------|
| Phase Title | ## **PHASE X: NAME** | ## **PHASE 1: VERIFY GAMEPLAY TAGS** |
| Main Step | ### **X) Step Name** | ### **1) Open Project Settings** |
| Sub-Step | #### X.X) Name | #### 1.1) Navigate to Tags |
| Detail | - X.X.X) Instruction | - 1.1.1) Click Add New Tag |

### Bold Usage

| Element | Bold? | Example |
|---------|-------|---------|
| Phase headers | Yes | ## **PHASE 1: NAME** |
| Main step headers | Yes | ### **1) Step Name** |
| Sub-step headers | No | #### 1.1) Sub-Step |
| Inline emphasis | Minimal | Only for critical values |

---

## NUMBERING FORMAT

### Hierarchy Depth

| Depth | Format | When to Use |
|-------|--------|-------------|
| Level 1 | X) | Main steps within a phase |
| Level 2 | X.X) | Sub-steps |
| Level 3 | X.X.X) | Granular instructions |
| Level 4 | X.X.X.X) | Property configurations |
| Level 5+ | X.X.X.X.X) | Deeply nested details |

### Examples

| Depth | Example |
|-------|---------|
| 1 | 5) Create Gameplay Ability |
| 2 | 5.1) Create Blueprint |
| 3 | 5.1.1) Right-click in Content Browser |
| 4 | 5.2.2.1) Add tag: Ability.Father.Armor |
| 5 | 5.3.8.1.1) Socket Name: FatherChestSocket |

### Numbering Restarts

| Scope | Behavior |
|-------|----------|
| Each Phase | Numbering restarts at 1) |
| Sub-levels | Continue from parent (1.1, 1.2, 1.3) |

---

## NODE CONNECTION FORMAT

### Execution Flow Descriptions

| Connection Type | Format |
|-----------------|--------|
| Basic Flow | From [Node] execution pin -> [Next Node] |
| Output Pin | From [Node] [Pin Name] -> [Target] [Input Pin] |
| Cast Success | From Cast successful execution -> [Next Node] |
| Cast Fail | From Cast failed execution -> [Next Node] |
| Branch True | From Branch True execution -> [Next Node] |
| Branch False | From Branch False execution -> [Next Node] |
| Sequence | From Sequence Then 0 -> [Next Node] |

### Node Instruction Format

| Step | Format |
|------|--------|
| Add Node | Add [Node Name] node |
| Connect | Connect [Source] to [Target] |
| Configure | [Property]: [Value] |

### Example Node Sequence

| Step | Instruction |
|------|-------------|
| 5.3.1) | Add Event ActivateAbility node |
| 5.3.2) | From Event ActivateAbility execution: |
| 5.3.2.1) | Add Get Avatar Actor From Actor Info node |
| 5.3.3) | From Return Value: |
| 5.3.3.1) | Add Cast To BP_FatherCompanion node |
| 5.3.3.2) | Connect Return Value to Object input |
| 5.3.4) | From successful cast execution: |
| 5.3.4.1) | Add Is Valid node |

---

## PROPERTY SPECIFICATION FORMAT

### Single Property

| Format | Example |
|--------|---------|
| Property: Value | Instancing Policy: Instanced Per Actor |
| Property: Check/Uncheck | Replicate Input Directly: Uncheck |
| Property: [Specific Value] | InputTag: Narrative.Input.Father.FormChange |

### Property List Format

- X.X.1) Property Name: Value
- X.X.2) Property Name: Value
- X.X.3) Property Name: Check (or Uncheck)

### Tag Array Format

| Scenario | Format |
|----------|--------|
| Single Tag | - X.X.X) Add tag: Tag.Name |
| Multiple Tags | - X.X.X) Add tag: Tag.Name.One |
| | - X.X.X) Add tag: Tag.Name.Two |
| Empty Array | - X.X.X) LEAVE EMPTY |

---

## TABLE FORMATS

### Variable Configuration Table

| Variable | Type | Default | Replicated | Rep Notify | Rep Condition | Instance Editable |
|----------|------|---------|------------|------------|---------------|-------------------|
| VarName | Type | Value | Yes/No | Yes/No | Condition | Yes/No |

### Replication Condition Values

| Condition | When to Use |
|-----------|-------------|
| None | Default - always replicate when dirty |
| Owner Only | Only replicate to owning connection |
| Skip Owner | Replicate to all except owner |
| Initial Only | Only replicate on initial replication |
| Custom | Custom condition in C++ |

### Tag Configuration Table

| Tag Type | Tag |
|----------|-----|
| Ability Tags | Tag.Value |
| Activation Required | Tag.Value |
| Activation Blocked | Tag.Value |
| InputTag | Tag.Value |

### Assets Created Table

| Asset | Type | Location |
|-------|------|----------|
| AssetName | Asset Type | /Game/Path/ |

### Gameplay Effect Component Table (UE 5.6)

| Component | Property | Value |
|-----------|----------|-------|
| Component Name | Property | Value |

---

## UI NAVIGATION FORMAT

### Content Browser

| Action | Format |
|--------|--------|
| Navigate | Content Browser -> /Game/Path/ |
| Create | Right-click in Content Browser |
| Open | Double-click [AssetName] |

### Blueprint Editor

| Action | Format |
|--------|--------|
| Open Defaults | Click Class Defaults button |
| Add Node | Right-click in Event Graph |
| Search | Type [NodeName] in context menu |

### Project Settings

| Action | Format |
|--------|--------|
| Open | Edit -> Project Settings |
| Navigate | Project Settings -> [Category] -> [Subcategory] |

### What NOT to Write

| Avoid | Reason |
|-------|--------|
| "Context menu appears" | Obvious UE behavior |
| "Window opens" | Obvious UE behavior |
| "Node is created" | Obvious UE behavior |
| "Panel displays" | Obvious UE behavior |

---

## QUICK REFERENCE SECTION

### Location

Always placed AFTER Introduction, BEFORE Phase 1

### Required Tables

| Table | Purpose |
|-------|---------|
| Tag Configuration | All tags used by ability |
| Variable Summary | All variables created |
| Gameplay Effects | GEs created (if any) |
| Assets Created | All new assets |

### Optional Tables

| Table | When to Include |
|-------|-----------------|
| Node Flow Summary | Complex abilities (10+ nodes) |
| Integration Points | Multi-system abilities |

---

## ABILITY LIFECYCLE DOCUMENTATION PATTERNS

### When to Document Each Pattern

| Ability Type | When Ability Ends | State Maintained By |
|--------------|-------------------|---------------------|
| Form Ability | After activation completes | GE_*State effect |
| Action Ability | After action completes | N/A (one-shot) |
| Passive Ability | Never (runs continuously) | Ability stays active |
| Utility Ability | After utility function | N/A (one-shot) |

### Form Ability Documentation Pattern

| Phase | What to Document |
|-------|------------------|
| PHASE 4 | Apply GE_*State effect |
| PHASE 4 | Set state variables (CurrentForm, etc.) |
| PHASE 4 | Commit Ability (if needed) |
| PHASE 4 | End Ability - document that ability ends, GE maintains state |

### Action Ability Documentation Pattern

| Phase | What to Document |
|-------|------------------|
| PHASE 4 | Perform action logic |
| PHASE 4 | Apply duration GE (if any) |
| PHASE 4 | Commit Ability |
| PHASE 4 | End Ability |

### Passive Ability Documentation Pattern

| Phase | What to Document |
|-------|------------------|
| PHASE 4 | Setup listening logic |
| PHASE 4 | Do NOT end ability |
| PHASE 4 | Document cleanup in On End Ability event |

### Lifecycle Summary Table Format

| Ability Type | Commit | End | Active |
|--------------|--------|-----|--------|
| Form | No | Yes | No (GE maintains) |
| Action | Yes (if cost) | Yes | No |
| Passive | No | No | Yes |
| Utility | Optional | Yes | No |

---

## DETACH SEQUENCE DOCUMENTATION PATTERN

When documenting abilities that spawn actor behind player:

### Spawn Offset Calculation Format

| Step | Format |
|------|--------|
| X.X.1) | Add Get Actor Location node |
| X.X.1.1) | Target: Player Character |
| X.X.2) | Add Get Actor Forward Vector node |
| X.X.2.1) | Target: Player Character |
| X.X.3) | Add Multiply (Vector * Float) node |
| X.X.3.1) | Vector: Forward Vector |
| X.X.3.2) | Float: -200.0 (negative for behind) |
| X.X.4) | Add Add (Vector + Vector) node |
| X.X.4.1) | A: Actor Location |
| X.X.4.2) | B: Multiplied Vector |
| X.X.5) | Result: Spawn location behind player |

### Required Documentation Note

Always include: "Negative offset (-200) prevents collision during forward movement"

---

## ANIMATION INTEGRATION DOCUMENTATION PATTERN

Always document animation for professional polish:

### Play Montage Format

| Step | Content |
|------|---------|
| X.X.1) | Add Play Montage and Wait node |
| X.X.2) | Montage to Play: AM_AbilityName (placeholder) |
| X.X.3) | From On Completed: |
| X.X.3.1) | Continue with next logic |
| X.X.4) | From On Interrupted: |
| X.X.4.1) | Handle interruption (optional cleanup) |

### Animation Montage Naming Convention

| Ability Type | Naming Pattern |
|--------------|----------------|
| Form Change | AM_FatherFormName |
| Attack | AM_FatherAttackType |
| Special | AM_FatherAbilityName |

---

## DAMAGE ABILITY DOCUMENTATION PATTERN

When documenting abilities that deal damage:

### Required Documentation Elements

| Element | What to Document |
|---------|------------------|
| GE Creation | Create GE with Executions Component |
| Execution Class | NarrativeDamageExecCalc |
| SetByCaller | Data.Damage tag and magnitude |

### Never Document These Approaches

| Forbidden Approach | Document Instead |
|--------------------|------------------|
| Set Health directly | Use NarrativeDamageExecCalc |
| Subtract from Health | Use Gameplay Effect with Execution |

---

## CHANGELOG FORMAT

### Location

Always at END of document, after last phase

### Table Format

| Version | Date | Changes |
|---------|------|---------|
| X.X | Month Year | Brief change descriptions |

### Change Description Style

| Style | Example |
|-------|---------|
| Added | Added Phase 5 for GE creation |
| Fixed | Fixed InputTag from X to Y |
| Updated | Updated node flow for multiplayer |
| Removed | Removed deprecated section |
| Changed | Changed parent class to X |

---

## CHARACTER ENCODING RULES

### Required ASCII Replacements

| Forbidden | Replacement |
|-----------|-------------|
| -> (unicode arrow) | -> (hyphen + greater than) |
| ' ' (smart quotes) | ' " (straight quotes) |
| degree symbol | "degrees" text |
| checkbox symbols | "Check" / "Uncheck" text |
| bullet symbols | - (hyphen) |
| em-dash | -- (double hyphen) |
| ellipsis | ... (three periods) |

### Verification

Before finalizing any document, search for:
- Unicode arrows
- Smart quotes
- Special symbols
- Non-ASCII characters

---

## FORBIDDEN CONTENT

### Never Include These Sections

| Section Type | Reason |
|--------------|--------|
| Testing sections | Not implementation |
| Bug fixing sections | Not implementation |
| Troubleshooting sections | Not implementation |
| Best practice sections | Not implementation |
| README files | Guide is self-contained |
| "What's New" sections | Use CHANGELOG |
| "CRITICAL REMINDERS" | Integrate naturally |
| "COMPLETION SUMMARY" | Guide ends after phases |
| "Next Steps" sections | Out of scope |
| Appendix sections | Content in phases |

### Never Include These Elements

| Element | Reason |
|---------|--------|
| Code blocks (```) | Use markdown tables |
| Warning emojis | No emojis |
| "IMPORTANT NOTE" callouts | Integrate naturally |
| "[WARNING] CRITICAL" | No warning patterns |
| Visual blueprint diagrams | Text descriptions only |
| Conditional "if exists" logic | Assume first-time |
| Explanatory node type notes | Only explain WHY |

### Never Include These Phrases

| Phrase | Why Forbidden |
|--------|---------------|
| "Node appears" | Obvious behavior |
| "Menu opens" | Obvious behavior |
| "This is a pure function" | Explains WHAT not WHY |
| "Note: Execution flows left to right" | Obvious behavior |
| "If this already exists, skip" | Assume first-time |
| "If missing, create" | Assume first-time |

---

## NOTES AND EXPLANATIONS

### When to Include Notes

| Include | Avoid |
|---------|-------|
| WHY something is configured | WHAT a node does |
| Multiplayer implications | Basic node functionality |
| Form-specific requirements | Generic UE concepts |
| Integration dependencies | Obvious connections |

### Note Format

| Format | Example |
|--------|---------|
| Inline | Note: Using Server Only for AI-owned ability |
| After step | This ensures proper replication to clients |

---

## DOCUMENT LENGTH GUIDELINES

### Typical Lengths

| Guide Type | Approximate Lines |
|------------|-------------------|
| Simple Ability | 400-600 |
| Standard Ability | 800-1200 |
| Complex Ability | 1500-2500 |
| System Guide | 2000-4000 |

### If Document Seems Short

| Check | Action |
|-------|--------|
| Missing phases | Add required phases |
| Missing granularity | Add sub-steps |
| Missing tables | Add Quick Reference |

### If Document Seems Long

| Check | Action |
|-------|--------|
| Redundant explanations | Remove obvious notes |
| Duplicate content | Consolidate |
| Testing sections | Remove (forbidden) |

---

## PRE-SUBMISSION CHECKLIST

### Structure Verification

| Check | Requirement |
|-------|-------------|
| Title block present | Yes |
| Document Information table | Yes |
| Table of Contents | Yes |
| Quick Reference section | Yes |
| All phases numbered | Yes |
| Changelog at end | Yes |

### Format Verification

| Check | Requirement |
|-------|-------------|
| No code blocks | Converted to tables |
| No UTF-8 special chars | ASCII only |
| No forbidden sections | Removed |
| No emojis | None present |
| No conditional logic | First-time assumption |

### Content Verification

| Check | Requirement |
|-------|-------------|
| Hierarchical numbering | Consistent |
| Node connections explicit | Pin names included |
| Properties specified | Values provided |
| Tables formatted | Proper markdown |

---

## RELATED DOCUMENTS

| Document | Use For |
|----------|---------|
| Father_Companion_Technical_Reference | Technical details, tag values, C++ sources, patterns |
| Father_Companion_System_Design_Document | System overview, ability lists, form specifications |
| DefaultGameplayTags_FatherCompanion | Actual tag values |
| BP_FatherCompanion_Setup_Guide | Father character setup |

---

## CHANGELOG

| Version | Date | Changes |
|---------|------|---------|
| 3.0 | January 2026 | Renamed from Spider to Father throughout entire document. All references updated. |
| 2.2 | December 2025 | Restructured as format-only document. Added Documentation Patterns sections (Ability Lifecycle, Detach Sequence, Animation Integration, Damage). Added Replication Condition to variable tables. Added pre-submission checklist. Moved technical content (tag values, specific configurations) to Technical Reference. |
| 2.1 | December 2025 | Added document length guidelines. Updated forbidden content list. |
| 2.0 | November 2025 | Major restructure. Removed all code blocks - converted to tables. Added comprehensive forbidden content. Updated to ASCII-only encoding. |
| 1.0 | November 2025 | Initial format reference. |

---

**END OF FORMAT REFERENCE**
