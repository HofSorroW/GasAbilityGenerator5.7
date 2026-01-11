# Spider Companion System - Project Instructions

## Version Information

- **Unreal Engine Version:** 5.6 (Always use latest)
- **Documentation Website:** https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-6-documentation
- **Plugin:** Narrative Pro Plugin v2.1

---

## Core Requirements

### System Compatibility

- Compatible with Narrative Pro's tag-based input system
- Compatible with Narrative Pro's ability system
- Compatible with Narrative Pro's menu system
- Compatible with all other Narrative Pro systems
- Multiplayer compatible

### Implementation Approach

- Blueprints only (no C++ code in guides)
- Use "Gameplay Blueprint Attributes" plugin for attribute creation when needed
- Design step-by-step guides with extremely granular instructions

---

## Reference Documents

Before creating or updating any implementation guide, always search and read the following documents:

| Document | Purpose |
|----------|---------|
| Spider_Companion_Technical_Reference | C++ source code locations, input tag system architecture, AI Perception system, EquippableItem mechanics, NPCDefinition workflows, SetByCaller tags, Blueprint ability patterns, node connection examples, AbilityConfiguration system, ActivityConfiguration system, Goal system, EQS patterns, combat ability hierarchy, attribute lists, execution calculations, faction system, movement speed handling, replication patterns, GE component configuration, net execution policies, ability lifecycle patterns |
| Spider_Companion_Guide_Format_Reference | Implementation guide formatting standards, numbering hierarchy, property specification format, node connection format, forbidden content rules |
| Spider_Companion_System_Design_Document | Current system overview including abilities across 5 forms (Crawler, Armor, Exoskeleton, Symbiote, Engineer - NO Shield form) |
| DefaultGameplayTags_SpiderCompanion | Actual gameplay tag values and line numbers |
| Spider_Companion_System_Setup_Guide | BP_SpiderCompanion blueprint creation, variable and function setup, player integration, EquippableItem form creation, NPCDefinition configuration |

---

## Granular Step-by-Step Format

Use hierarchical numbering system extending 6+ levels deep:

```
5) Create Gameplay Ability - GA_SpiderArmor
5.1) Create the Ability Blueprint
5.1.1) Duplicate GA_SpiderCrawler or create new GameplayAbility
5.1.2) Name it: GA_SpiderArmor
5.1.3) Open the blueprint
5.2) Configure Ability Properties
5.2.1) Click Class Defaults
5.2.2) Ability Tags:
5.2.2.1) Change to: Ability.Spider.Armor
5.2.3) Cancel Abilities with Tag:
5.2.3.1) Add tag: Ability.Spider.Crawler
5.2.3.2) Add tag: Ability.Spider.Exoskeleton
5.2.3.3) Add tag: Ability.Spider.Shield
5.2.4) Activation Owned Tags:
5.2.4.1) Change to: Spider.Form.Armor
5.2.4.2) Add tag: Spider.State.Attached
5.3) Create Jump and Attach Animation Logic
5.3.1) Add Event ActivateAbility
5.3.2) From Event ActivateAbility:
5.3.2.1) Add Get Avatar Actor From Actor Info node
5.3.3) From Return Value:
5.3.3.1) Add Cast To BP_SpiderCompanion node
5.3.4) From successful cast execution:
5.3.4.1) Add Get Owner Player node
5.3.4.2) Connect As BP Spider Companion to Target
5.3.5) From Get Owner Player Return Value:
5.3.5.1) Add Is Valid node
5.3.5.2) Connect to Input Object
5.3.6) From Is Valid -> Is Valid execution pin:
5.3.6.1) Add Play Montage node (or Play Animation)
5.3.6.2) Target: Connect As BP Spider Companion
5.3.6.3) Montage to Play: Select your jump animation montage
```

### Required Detail Level

- Specific UI navigation like ("Right-click in Content Browser")
- Exact property names and values for every node and setting in blueprint
- Detailed instructions for each node's inputs, outputs and connections to the next node
- Variable type specifications when variables are to be created

---

## Condensed Format Sections

The following 4 sections use condensed format WITHOUT granular steps or UI navigation:

### 1. Gameplay Tag Creation

List only full tag paths without navigation or UI instructions:

```
Effect.Spider.ArmorBoost
Ability.Spider.Armor
Spider.Form.Armor
```

### 2. Tag Configuration (Class Defaults)

Use single markdown table for Ability Tags, Cancel Abilities with Tag, Activation Owned Tags, Activation Required Tags, and Activation Blocked Tags:

| Property | Tags |
|----------|------|
| Ability Tags | Ability.Spider.Armor |
| Cancel Abilities with Tag | Ability.Spider.Crawler, Ability.Spider.Exoskeleton |
| Activation Owned Tags | Spider.Form.Armor, Spider.State.Attached |
| Activation Required Tags | (none) |
| Activation Blocked Tags | (none) |

### 3. Variable Creation

Use single markdown table:

| Variable | Type | Default | Instance Editable |
|----------|------|---------|-------------------|
| CurrentForm | ESpiderForm | Crawler | No |
| OwnerPlayer | Actor Reference | None | No |
| IsAttached | Boolean | False | No |

### 4. GameplayEffect Configuration

Use single markdown table:

| Property | Value |
|----------|-------|
| Duration Policy | Infinite |
| Granted Tags | Spider.State.Armored |
| Modifiers | Attribute: Defense, Operation: Add, Magnitude: 25.0 |

---

## Forbidden Content

### Do NOT Create

- Additional "read me" files other than the guide itself
- Visual blueprint implementations in documents
- Testing, bug fixing, troubleshooting, best practice sections
- "What's New in Version X" sections
- "CRITICAL REMINDERS" sections
- "COMPLETION SUMMARY" sections
- "Next Steps" sections
- Appendix sections

### Do NOT Use

- Warning phrases or emojis like "WARNING CRITICAL", "WARNING IMPORTANT NOTE", "WARNING", "[WARNING] CRITICAL CONCEPT"
- GAS Companion plugin - use Narrative Pro's native systems only
- NarrativeCharacter class - use NarrativeNPCCharacter for spider
- UGameplayAbility class - use NarrativeGameplayAbility for all abilities
- WeaponItem for equipping abilities
- Attribute-based movement speed changes - use CharacterMovement component directly
- Spawn spider from player BeginPlay - use NPCSpawner system
- C++ code in guides - use "Gameplay Blueprint Attributes" plugin for attribute creation when needed
- Code blocks (```) for any content in design documents - always use markdown tables for numbered sequences, flows, and structured data
- UTF-8 special characters (-> ' ' " ") - always use ASCII equivalents (-> ' ")

---

## Formatting Rules

### Character Encoding

- Never use UTF-8 special characters
- Use ASCII equivalents:
  - Arrow: -> (not ->)
  - Degree: degrees (not deg)
  - Quotes: ' " (not ' ' " ")

### Code Blocks

- Never use code blocks (```) for any content in design documents
- Always use markdown tables for numbered sequences, flows, and structured data

### Obvious Feedback Statements

Remove statements like:
- "Node appears"
- "Context menu appears"
- "Menu opens"
- Users understand standard Unreal Engine behavior

### Explanatory Notes

- Remove notes explaining WHAT (e.g., "Note: This is a pure function", "Note: Execution flows left to right")
- Only keep notes explaining WHY

---

## Implementation Guidelines

### Animation Integration

When abilities involve state transitions (attach/detach), always integrate animation system using Play Montage with placeholder animation name (like AM_SpiderDetach) for professional polish, even if animation assets will be created later.

### Actor Detachment

When detaching actors from player, always calculate spawn offset behind player using negative forward vector:
```
Player Location + Forward * -200
```
This prevents collision during forward movement.

### First-Time Implementation

- Always assume first-time implementation
- Never include conditional "if exists, skip/create" logic
- Never include cleanup sections for removing previously added assets

### Form Section Updates

Before updating any form section in design documents, read the corresponding implementation guide to ensure all features (especially stat boosts and parameters) are included.

### Cancelled Abilities

When cancelled abilities are mentioned, search and remove them from ALL tables and sections in the document, not just one location.

### Version Verification

- Before finalizing any document update, compare line count and section headings against the previous version
- Ensure no important content or sections were accidentally omitted
- Perform comprehensive verification check for code blocks, special characters, and missing content before declaring production-ready

---

## SpiderAbilityGenerator Plugin Rules

### General Guidelines

- Always consult Father_Ability_Generator_Plugin_Specification.md before creating or updating any plugin code, YAML schemas, or generator logic
- Always update the latest existing plugin version rather than creating from scratch
- Check current version number and increment appropriately:
  - Fixes: v1.3.2 -> v1.3.3
  - Features: v1.3.2 -> v1.4.0

### Asset Existence Checking

- Always check if asset exists before attempting to create using FPackageName::DoesPackageExist() and FindPackage()
- Log messages:
  - "Skipping existing [Type] (on disk): [Name]"
  - "Skipping existing [Type] (in memory): [Name]"

### Manifest and State Files

- Use manifest.yaml as single source of truth for all asset definitions and tags
- Use separate .father_state.json file for tracking generation state and content hashes
- State file location: /FatherGuides/.father_state.json

### Tag Generation

- TagGenerator must read tags from manifest.yaml
- Write tags to DefaultGameplayTags.ini
- Check for existing tags before appending new ones

### State File Contents

Track per asset:
- yaml_file_hash
- content_hash
- yaml_version
- generated_at timestamps
- asset_path
- exists boolean

### Code Generation

- Never generate C++ code
- All assets must be Blueprint-based
- Use programmatic asset creation APIs

### Asset Validation

- Validate all asset_name references against manifest whitelist before generation
- Display "ERROR: [asset_name] not in manifest" for assets not in whitelist
- Prevents typos and accidental overwrites of built-in assets

### Four-State Asset Tracking

| State | Description |
|-------|-------------|
| NEW | No entry in state file |
| UPDATED | Hash changed since last generation |
| UNCHANGED | Hash matches, asset exists |
| MISSING | Entry exists but asset file not found |

### Hash Calculation

- Calculate content_hash on each asset definition
- Store in .father_state.json
- Detect changes between generation runs

### Dual Workflow Support

| Workflow | File | Behavior |
|----------|------|----------|
| Full Generation | manifest.yaml | Generate all assets |
| Incremental Update | Individual *.yaml (not manifest.yaml) | Update single assets |

### File Cleanup

- Delete incremental *.yaml files on successful generation (prevents clutter)
- Keep incremental *.yaml files on failure (allows fix and retry)
- Never delete manifest.yaml

### Results Dialog

Display after generation:
- NEW: Assets created
- SKIPPED: Assets already exist
- FAILED: Generation errors
- TOTAL: Assets processed

### Input File Types

The plugin only reads two file types:
1. manifest.yaml - Full asset/tag definitions
2. Incremental *.yaml files - Single asset updates

Important: Implementation guide .md files are documentation for humans and Claude, NOT input for the plugin. Never parse .md files directly or hardcode asset properties in plugin code.
