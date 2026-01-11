# FATHER ABILITY GENERATOR PLUGIN v7.8.2 SPECIFICATION

## Unreal Engine 5.6 + Narrative Pro v2.2

---

## OVERVIEW

This document specifies all assets required for the Father Companion System, the plugin architecture for auto-generating them, and the development pipeline from guide creation to asset generation.

---

## DEVELOPMENT PIPELINE

### A to Z Workflow

| Step | Action | Owner |
|------|--------|-------|
| 1 | Feed Narrative Pro update data | User |
| 2 | Analyze changes, update Technical Reference | Claude |
| 3 | Create/update implementation guides together | User + Claude |
| 4 | Debug and revise guides | User + Claude |
| 5 | Generate YAML data blocks in guides | Claude |
| 6 | Feed guides to plugin | User |
| 7 | Plugin creates assets | Plugin |
| 8 | Debug assets, rinse and repeat | User + Claude |

### Claude Role in Pipeline

| When | What Claude Does |
|------|------------------|
| NP Update | Analyze changes, update Technical Reference |
| Guide Creation | Write implementation guide with user |
| Guide Update | Revise guide, track version |
| YAML Generation | Convert guide to YAML on request |
| Debug Support | Help interpret errors, suggest fixes |
| Bulk Operations | Generate all YAML from all guides |
| Manifest Updates | Add new tags and assets to manifest |

---

## ARCHITECTURE DECISIONS

### Finalized Design Choices

| Item | Decision |
|------|----------|
| Version format | Underscore: 4_3, 4_10 (not decimal) |
| Tags enforcement | Disable [Generate Assets] if new tags pending |
| Manifest maintenance | Claude maintains |
| First time setup | Claude provides complete starter zip |
| Overwrite behavior | Skip existing, log warning |
| Duplicates | Error, must remove one |
| State recovery | Auto-recreate if corrupted |
| Folder path | Remember between sessions |
| Scan | Auto on open |
| Cancel button | None (prevents corruption) |
| Log text | Selectable and copyable |

### Config Persistence

The plugin saves user settings to GEditorPerProjectIni for persistence between sessions.

| Setting | Config Key | Storage Location |
|---------|------------|------------------|
| Guides folder path | GuidesFolderPath | [ProjectDir]/Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini |

Config persistence behavior:

| Event | Action |
|-------|--------|
| Editor startup | Load GuidesFolderPath from config |
| User selects manifest.yaml | Save folder path to config immediately with Flush |
| User uses Settings dialog | Save folder path to config immediately with Flush |
| Editor shutdown | Save GuidesFolderPath to config (backup) |

The GConfig->Flush() call ensures the config is written to disk immediately, preventing data loss if the editor crashes.

Startup logging messages:

| Log Level | Message | Meaning |
|-----------|---------|---------|
| Log | Loaded guides folder: [path] | Path loaded and manifest.yaml exists |
| Warning | Saved guides folder no longer contains manifest.yaml | Path loaded but manifest missing |
| Log | No saved guides folder | First time use or config cleared |

### Single Source of Truth

| Question | Answer |
|----------|--------|
| Where are tags defined? | manifest.yaml |
| Where are assets defined? | manifest.yaml (full) or incremental .yaml (updates) |
| How many files does plugin read? | manifest.yaml + any incremental .yaml files |
| Single source of truth? | Yes - manifest.yaml is the whitelist |

### Incremental YAML Workflow

The plugin supports two workflows: full generation from manifest.yaml and incremental updates from individual .yaml files.

#### Dual Workflow Design

| Scenario | What Claude Provides | File(s) |
|----------|---------------------|---------|
| Update existing asset | Single incremental YAML | GA_FatherArmor_v4_3.yaml |
| Add new asset | Updated full manifest | manifest.yaml (new version) |

#### Update Existing Asset Flow

| Step | Action |
|------|--------|
| 1 | User works with Claude to update GA_FatherArmor guide |
| 2 | User requests YAML for updated asset |
| 3 | Claude provides GA_FatherArmor_v4_3.yaml (single file) |
| 4 | User saves file to guides directory |
| 5 | Plugin detects incremental file, validates against manifest whitelist |
| 6 | Asset exists in manifest -> regenerate asset |
| 7 | On success -> delete incremental .yaml file |

#### Add New Asset Flow

| Step | Action |
|------|--------|
| 1 | User works with Claude to create new GA_FatherWebShot ability |
| 2 | User requests updated manifest with new asset |
| 3 | Claude provides updated manifest.yaml including new asset + tags |
| 4 | User replaces manifest.yaml in guides directory |
| 5 | Plugin detects manifest change, generates new asset |
| 6 | User can now use incremental updates for this asset |

#### Plugin File Detection Logic

| File Pattern | Action |
|--------------|--------|
| manifest.yaml | Full generation (baseline) |
| *.yaml (not manifest.yaml) | Incremental update |

#### Incremental File Validation

| Check | Result |
|-------|--------|
| asset_name in manifest.yaml? | Yes -> Process |
| asset_name in manifest.yaml? | No -> ERROR: Asset not in whitelist |
| asset_type matches manifest? | Yes -> Process |
| asset_type matches manifest? | No -> ERROR: Type mismatch |

#### Incremental File Cleanup

| Generation Result | Action | Reason |
|-------------------|--------|--------|
| Success | Delete .yaml file | Prevent clutter |
| Failed | Keep .yaml file | User can fix and retry |
| manifest.yaml | Never delete | Master whitelist |

#### Incremental YAML File Format

Same structure as manifest section, but single asset:

| Field | Required | Example |
|-------|----------|---------|
| asset_name | Yes | GA_FatherArmor |
| asset_type | Yes | GameplayAbility |
| version | Yes | 4_3 |
| folder | Yes | Abilities/Forms |
| parent_class | Yes | NarrativeGameplayAbility |
| (type-specific fields) | Varies | tags, modifiers, etc. |

#### Example Incremental YAML

| Line | Content |
|------|---------|
| 1 | # Incremental update for GA_FatherArmor |
| 2 | asset_name: GA_FatherArmor |
| 3 | asset_type: GameplayAbility |
| 4 | version: 4_3 |
| 5 | folder: Abilities/Forms |
| 6 | parent_class: NarrativeGameplayAbility |
| 7 | instancing_policy: InstancedPerActor |
| 8 | net_execution_policy: ServerOnly |
| 9 | tags: |
| 10 | (2 spaces)ability_tags: Ability.Father.Armor |
| 11 | (2 spaces)cancel_abilities_with_tag: |
| 12 | (4 spaces)- Ability.Father.Crawler |
| 13 | (4 spaces)- Ability.Father.Exoskeleton |

---

## FILE STRUCTURE

### Guides Folder Layout

| Path | Purpose |
|------|---------|
| /FatherGuides/manifest.yaml | Tags + assets whitelist |
| /FatherGuides/.father_state.json | Plugin-managed state |
| /FatherGuides/GA_FatherArmor_v4_2.md | Armor form ability guide |
| /FatherGuides/GA_FatherCrawler_v3_3.md | Crawler form ability guide |
| /FatherGuides/GA_FatherExoskeleton_v3_10.md | Exoskeleton form ability guide |
| /FatherGuides/GA_FatherSymbiote_v3_5.md | Symbiote form ability guide |
| /FatherGuides/GA_FatherEngineer_v4_3.md | Engineer form ability guide |
| /FatherGuides/GE_FormChangeCooldown_v1_3.md | Form cooldown effect guide |
| /FatherGuides/... | All other guides |

### Manifest Structure

| Section | Content |
|---------|---------|
| project_root | /Game/FatherCompanion |
| tags_ini_path | /Config/DefaultGameplayTags.ini |
| tags | All 174 gameplay tags |
| assets | All 97 non-tag assets with folder locations |

### Manifest Completeness Requirements

The manifest.yaml must contain all gameplay tags and assets for the Father Companion System. Incomplete manifests cause generation failures.

| Requirement | Expected | Failure Mode |
|-------------|----------|--------------|
| Gameplay tags | 174 tags | Only partial tags generated |
| Blackboard definitions | Complete with nested keys | Keys created as separate assets |
| Input actions | 7 actions | Missing input bindings |
| Gameplay abilities | 18 abilities | Abilities not generated |
| Gameplay effects | 14+ effects | Effects not generated |

Tag extraction from reference file:

| Source File | Content |
|-------------|---------|
| DefaultGameplayTags_FatherCompanion_v3_5.ini | All 174 tag definitions |

Verification command to count tags in manifest:

| Check | Expected Result |
|-------|-----------------|
| Tags count | 174 |
| Blackboard entries | 1 (BB_FatherCompanion) |
| Blackboard keys | 9 keys within BB_FatherCompanion |

### State File Structure

| Field | Type | Purpose |
|-------|------|---------|
| last_scan | ISO DateTime | Last folder scan timestamp |
| yaml_file_hash | String | Hash of manifest.yaml |
| assets | Object | Per-asset state entries |
| assets.[name].yaml_version | String | Version from GENERATOR_DATA |
| assets.[name].content_hash | String | Content hash for change detection |
| assets.[name].generated_at | ISO DateTime | Last generation timestamp |
| assets.[name].asset_path | String | UE content path |
| assets.[name].exists | Boolean | Whether asset file exists |

### State Tracking Logic

| Status | Condition |
|--------|-----------|
| NEW | No entry in state file |
| UPDATED | Hash changed since last generation |
| UNCHANGED | Hash matches, asset exists |
| MISSING | Entry exists but asset file not found |

---

## EMBEDDED YAML IN GUIDES

### GENERATOR_DATA Block Format

Each implementation guide contains a GENERATOR_DATA block that the plugin extracts.

| Field | Required | Description |
|-------|----------|-------------|
| asset_name | Yes | Asset name matching manifest |
| asset_type | Yes | See Supported Asset Types table below |
| version | Yes | Underscore format (4_2, not 4.2) |
| folder | Yes | Subfolder in FatherCompanion |
| parent_class | Yes | Parent blueprint class |
| instancing_policy | For GA_ | InstancedPerActor, NonInstanced |
| net_execution_policy | For GA_ | ServerOnly, LocalPredicted |
| cooldown_gameplay_effect_class | Optional | GE_ for cooldown |
| tags | Yes | Tag configuration object |
| references | Optional | Asset references with paths |

#### Supported Asset Types

| asset_type Value | Asset Prefix | Parent Class |
|------------------|--------------|--------------|
| GameplayAbility | GA_ | NarrativeGameplayAbility |
| GameplayEffect | GE_ | GameplayEffect |
| EquippableItem | EI_ | EquippableItem |
| Actor | BP_ | NarrativeNPCCharacter |
| Widget | WBP_ | UserWidget |
| Activity | BPA_ | NarrativeActivityBase |
| Goal | Goal_ | NPCGoalItem |
| GoalGenerator | GoalGenerator_ | NPCGoalGenerator |
| TaggedDialogueSet | TaggedDialogueSet_ | TaggedDialogueSet |
| BTService | BTS_ | BTService_BlueprintBase |
| BTTask | BTT_ | BTTask_BlueprintBase |
| BTDecorator | BTD_ | BTDecorator_BlueprintBase |
| Blackboard | BB_ | BlackboardData |
| BehaviorTree | BT_ | BehaviorTree |
| NPCDefinition | NPCDef_ | NPCDefinition |
| AbilityConfiguration | AC_ | AbilityConfiguration |
| ActivityConfiguration | ActConfig_ | NPCActivityConfiguration |
| InputAction | IA_ | InputAction |
| InputMappingContext | IMC_ | InputMappingContext |
| FloatCurve | FC_ | CurveFloat |
| Enumeration | E_ | UserDefinedEnum |

### Tag Configuration Fields

| Field | Purpose | Example |
|-------|---------|---------|
| ability_tags | Asset identification | Ability.Father.Armor |
| cancel_abilities_with_tag | Mutual exclusion | Ability.Father.Crawler |
| activation_owned_tags | Tags granted during activation | Father.Form.Armor |
| activation_required_tags | Tags required to activate | Father.State.Recruited |
| activation_blocked_tags | Tags that prevent activation | Cooldown.Father.FormChange |

### YAML Generation Strategy

| Scenario | What Claude Does |
|----------|------------------|
| New guide created | Generate YAML for that guide only |
| Guide updated | Update YAML for that guide only |
| Request specific update | Update that one file |
| Request generate all | Generate all (rare, full refresh) |
| NP version affects multiple | Update affected guides only |

---

## PLUGIN USER INTERFACE

### Main Window Elements

| Element | Type | Purpose |
|---------|------|---------|
| Guides Folder | Text + Browse | Path to FatherGuides folder |
| Status Panel | Info Display | manifest.yaml status, guides count, pending tags |
| Assets List | Checkable List | Shows all assets with status indicators |
| Generate Tags | Button | Creates tags in DefaultGameplayTags.ini |
| Generate Selected | Button | Creates checked assets only |
| Generate All | Button | Creates all assets |
| Settings | Button | Plugin configuration |

### v2.0.9 Window Implementation

| Component | Class | Purpose |
|-----------|-------|---------|
| Window Widget | SGasAbilityGeneratorWindow | Main UI container |
| Path Input | SEditableTextBox | Guides folder path display |
| Log Display | SMultiLineEditableTextBox | Generation log output |
| Status Text | STextBlock | Current operation status |
| Buttons | SButton | Generate Tags, Generate Assets, Settings |

#### Window State Variables

| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| GuidesFolderPath | FString | Empty | Loaded from config |
| ManifestData | FManifestData | Empty | Parsed manifest content |
| bIsGenerating | bool | false | Guard flag for re-entry prevention |

#### Button Handler Pattern (v2.0.9)

| Step | Code | Purpose |
|------|------|---------|
| 1 | if (bIsGenerating) { Log("WARNING: Generation in progress"); return; } | Guard check |
| 2 | bIsGenerating = true | Set guard |
| 3 | GenerateAssets() | Perform generation |
| 4 | bIsGenerating = false | Clear guard |
| 5 | return FReply::Handled() | Complete handler |

### Asset Status Indicators

| Indicator | Meaning |
|-----------|---------|
| NEW | Asset not yet generated |
| UPDATED | Content changed since last generation |
| (unchanged) | No changes detected |
| MISSING | State entry exists but asset file not found |

### Log Window Elements

| Element | Type | Purpose |
|---------|------|---------|
| Log Display | Scrollable Text | Shows parsing and generation messages |
| Copy Selected | Button | Copy selected log lines |
| Copy All | Button | Copy entire log |
| Save to File | Button | Export log to file |
| Clear | Button | Clear log display |

### Log Message Format

| Component | Example |
|-----------|---------|
| Timestamp | [14:30:02] |
| Level | INFO, ERROR, OK |
| Asset | GA_FatherArmor |
| Location | Line 245 or GENERATOR_DATA Line 12 |
| Message | Missing required field: parent_class |
| Suggestion | Did you mean: Father.State.Attached? |

### Progress Display

| State | Display |
|-------|---------|
| During generation | 12 / 50 - GA_FatherArmor |
| After generation | Generation complete. Refresh Content Browser. |
| State recovery | State file reset. All assets marked as NEW. |

### Validation Messages

| Scenario | Message |
|----------|---------|
| Typo in asset name | ERROR: GA_SpidorArmor not in manifest |
| Built-in overwrite attempt | ERROR: GE_Invulnerable is built-in, not allowed |
| Missing required field | ERROR: GA_FatherArmor - Missing required field: parent_class |
| Duplicate asset | ERROR: Duplicate asset_name found in multiple files |
| New tags pending | 3 new tags pending. Generate tags first. |
| Tag reference not found | ERROR: Tag Father.State.Atached not in manifest (typo?) |

### Results Dialog

After generation completes, a results dialog displays comprehensive summary information.

#### Dialog Header

| Element | Value | Purpose |
|---------|-------|---------|
| Title | FATHER ABILITY GENERATOR RESULTS | Identifies dialog |
| Plugin Version | 2.1.2 | Currently installed plugin version |
| Manifest Version | 1.3.0 | Version from manifest.yaml header comment |

#### Summary Section

| Counter | Description | Example |
|---------|-------------|---------|
| NEW | Assets successfully created this run | NEW: 6 assets created |
| SKIPPED | Assets that already exist (not overwritten) | SKIPPED: 92 assets (already exist) |
| FAILED | Assets that failed to generate | FAILED: 0 assets |
| TOTAL | Sum of all processed assets | TOTAL: 98 assets processed |

#### Asset Categories

| Category | Asset Types | Indicator |
|----------|-------------|-----------|
| Input Actions | IA_* | + (new), - (skipped), X (failed) |
| Enumerations | E_* | + (new), - (skipped), X (failed) |
| Float Curves | FC_* | + (new), - (skipped), X (failed) |
| Gameplay Effects | GE_* | + (new), - (skipped), X (failed) |
| Gameplay Abilities | GA_* | + (new), - (skipped), X (failed) |
| Equippable Items | EI_* | + (new), - (skipped), X (failed) |
| Blackboards | BB_* | + (new), - (skipped), X (failed) |
| Behavior Trees | BT_* | + (new), - (skipped), X (failed) |
| Activities | BPA_* | + (new), - (skipped), X (failed) |
| Ability Configurations | AC_* | + (new), - (skipped), X (failed) |
| Activity Configurations | ActConfig_* | + (new), - (skipped), X (failed) |
| NPC Definitions | NPCDef_* | + (new), - (skipped), X (failed) |
| Character Definitions | CD_* | + (new), - (skipped), X (failed) |
| Item Collections | IC_* | + (new), - (skipped), X (failed) |
| Narrative Events | NE_* | + (new), - (skipped), X (failed) |
| Actor Blueprints | BP_* | + (new), - (skipped), X (failed) |
| Widget Blueprints | WBP_* | + (new), - (skipped), X (failed) |
| Input Mapping Contexts | IMC_* | + (new), - (skipped), X (failed) |

#### Status Detection Logic

| Status | Condition | Return Pattern |
|--------|-----------|----------------|
| NEW | Asset created successfully | EGenerationStatus::New |
| SKIPPED | Asset already exists on disk or in memory | EGenerationStatus::Skipped |
| FAILED | Generation error occurred | EGenerationStatus::Failed |

#### EGenerationStatus Enum (v2.0.9+)

| Value | Purpose | Counter Incremented |
|-------|---------|---------------------|
| New | Asset was created this run | NewCount++ |
| Skipped | Asset already exists (non-destructive skip) | SkippedCount++ |
| Failed | Generation error occurred | FailedCount++ |

#### FGenerationResult Struct (v2.0.9+)

| Field | Type | Purpose |
|-------|------|---------|
| AssetName | FString | Name of the asset processed |
| Status | EGenerationStatus | Result status (New, Skipped, Failed) |
| Message | FString | Human-readable result message |
| Category | FString | Asset category for grouping in results dialog |

#### FGenerationSummary Struct (v2.0.9+)

| Field | Type | Purpose |
|-------|------|---------|
| Results | TArray of FGenerationResult | All individual results |
| NewCount | int32 | Count of NEW assets |
| SkippedCount | int32 | Count of SKIPPED assets |
| FailedCount | int32 | Count of FAILED assets |

#### CheckExistsAndPopulateResult Helper (v2.0.9+)

All generators use a shared helper function to check existence and populate SKIPPED status:

| Step | Action | Result |
|------|--------|--------|
| 1 | Call FPackageName::DoesPackageExist() | If true, return SKIPPED |
| 2 | Call FindPackage() for memory check | If not null, return SKIPPED |
| 3 | Asset does not exist | Return false, proceed to generate |

| Log Output | Status | Meaning |
|------------|--------|---------|
| Skipping existing [Type] (on disk): [Name] | SKIPPED | Asset file exists in Content |
| Skipping existing [Type] (in memory): [Name] | SKIPPED | Package loaded in editor |
| Created [Type]: [Name] | NEW | Asset generated successfully |

#### Generation Guard (v2.0.9+)

The UI window implements a guard flag to prevent duplicate generation passes:

| Element | Type | Purpose |
|---------|------|---------|
| bIsGenerating | bool | Guard flag initialized to false |
| OnGenerateClicked | Check | If bIsGenerating, log warning and return |
| Generation Start | Set | bIsGenerating = true |
| Generation End | Set | bIsGenerating = false |

| Symptom Without Guard | Cause | v2.0.9 Fix |
|----------------------|-------|------------|
| Duplicate log entries | Button callback triggered twice | Guard flag prevents re-entry |
| Window destroyed twice | UI refresh during generation | Guard blocks concurrent runs |
| Results show NEW instead of SKIPPED | Counter reset between passes | Single pass guaranteed |

#### Results Dialog Debugging

If assets appear in wrong category, check:

| Symptom | Likely Cause | Solution |
|---------|--------------|----------|
| BP_ under Float Curves | ParseCurves missing section exit | Add indent-based section exit logic |
| WBP_ under Float Curves | ParseCurves missing section exit | Add indent-based section exit logic |
| IMC_ under Float Curves | ParseCurves missing section exit | Add indent-based section exit logic |
| No Widget Blueprints category | Wrong section name in parser | Add widget_blueprints: alias |
| Duplicate entries across categories | Multiple parsers claim same asset | Fix section boundary detection |

---

## MANIFEST AS WHITELIST

### Safety Design

Plugin only creates assets listed in manifest. This prevents:

| Scenario | Without Manifest | With Manifest |
|----------|------------------|---------------|
| Typo: GA_SpidorArmor | Creates wrong asset | ERROR: Not in manifest |
| Typo: GE_Invulnerable (NP built-in) | Overwrites NP file | ERROR: Not in manifest |
| Random asset_name: Test123 | Creates junk | ERROR: Not in manifest |
| Correct GA_FatherArmor | Creates | Creates |

### Plugin Validation Flow

| Step | Action |
|------|--------|
| 1 | Read manifest.yaml |
| 2 | Build allowed tags list |
| 3 | Build allowed assets list |
| 4 | Scan guides folder for GENERATOR_DATA |
| 5 | For each guide, check asset_name in manifest |
| 6 | Validate all tag references against manifest |
| 7 | Show errors for any not in manifest |
| 8 | Ready for generation |

### Asset Generation Safeguards

During asset creation, each generator performs existence checks to prevent crashes and data loss.

| Check | Method | Result |
|-------|--------|--------|
| Disk existence | FPackageName::DoesPackageExist() | Skip if asset file exists |
| Memory existence | FindPackage() | Skip if package loaded (even partially) |
| Package preparation | Package->FullyLoad() | Ensure package ready before creation |

| Log Message | Meaning |
|-------------|---------|
| Skipping existing [Type] (on disk): [Name] | Asset file already exists in Content folder |
| Skipping existing [Type] (in memory): [Name] | Package loaded from previous operation or editor |

| Behavior | Description |
|----------|-------------|
| Non-destructive | Never overwrites existing assets |
| Idempotent | Safe to run multiple times |
| Crash-resistant | Handles partially loaded packages |
| Resumable | Continues after editor restart mid-generation |

#### v2.0.9 Generator Return Pattern

All generators MUST follow this pattern for proper status tracking:

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | Build AssetPath from definition | e.g., /Game/FatherCompanion/Abilities/GA_Name |
| 2 | FGenerationResult Result | Declare result struct |
| 3 | if (CheckExistsAndPopulateResult(...)) return Result | Early exit with SKIPPED if exists |
| 4 | (Create asset) | Normal generation logic |
| 5 | Result = FGenerationResult(Name, EGenerationStatus::New, "Created") | Set success status |
| 6 | Result.DetermineCategory() | Auto-categorize by prefix |
| 7 | return Result | Return to caller |

#### CheckExistsAndPopulateResult Implementation

| Parameter | Type | Purpose |
|-----------|------|---------|
| AssetPath | const FString& | Full content path to check |
| AssetName | const FString& | Name for logging and result |
| AssetType | const FString& | Human-readable type for log |
| OutResult | FGenerationResult& | Result to populate if exists |
| Return | bool | true if exists (skip), false if should generate |

| Check Order | Method | If True |
|-------------|--------|---------|
| 1 | DoesAssetExistOnDisk(AssetPath) | Log "on disk", return SKIPPED |
| 2 | IsAssetInMemory(AssetPath) | Log "in memory", return SKIPPED |
| 3 | Neither | return false (proceed to generate) |

#### Category Determination

DetermineCategory() uses asset name prefix to assign category:

| Prefix | Category |
|--------|----------|
| IA_ | Input Actions |
| IMC_ | Input Mapping Contexts |
| GE_ | Gameplay Effects |
| GA_ | Gameplay Abilities |
| BP_ | Actor Blueprints |
| WBP_ | Widget Blueprints |
| BB_ | Blackboards |
| BT_ | Behavior Trees |
| M_ | Materials |
| AC_ | Ability Configurations |
| ActConfig_ | Activity Configurations |
| NPCDef_ | NPC Definitions |
| BPA_ | Activities |
| EI_ | Equippable Items |
| IC_ | Item Collections |
| E_ | Enumerations |
| FC_ | Float Curves |
| (other) | Other |

### YAML Parser Safeguards

The YAML parser implements multiple safeguards to prevent data corruption and duplicate generation.

#### Duplicate Prevention

Each parser function uses predicate-based duplicate checking before adding items to output arrays.

| Parser Function | Check Method | Protected Against |
|-----------------|--------------|-------------------|
| ParseInputActions | ContainsByPredicate by Name | Duplicate input actions |
| ParseEnumerations | ContainsByPredicate by Name | Duplicate enum assets |
| ParseGameplayEffects | ContainsByPredicate by Name | Duplicate GE_ assets |
| ParseCurves | ContainsByPredicate by Name | Duplicate FC_ assets |
| ParseGameplayAbilities | ContainsByPredicate by Name | Duplicate GA_ assets |
| ParseEquippableItems | ContainsByPredicate by Name | Duplicate BP_ form items |
| ParseAbilityConfigurations | ContainsByPredicate by Name | Duplicate AC_ assets |
| ParseActivityConfigurations | ContainsByPredicate by Name | Duplicate ActConfig assets |
| ParseNPCDefinitions | ContainsByPredicate by Name | Duplicate NPCDef assets |
| ParseCharacterDefinitions | ContainsByPredicate by Name | Duplicate CD_ assets |
| ParseItemCollections | ContainsByPredicate by Name | Duplicate IC_ assets |
| ParseNarrativeEvents | ContainsByPredicate by Name | Duplicate NE_ assets |
| ParseBlackboards | ContainsByPredicate by Name | Duplicate BB_ assets |
| ParseBlackboards | StartsWith for keys: detection | Keys section parsing failure |
| ParseBlackboards | Nested key accumulation | Keys created as separate assets |
| ParseActivities | ContainsByPredicate by Name | Duplicate BPA_ assets |
| ParseWidgets | ContainsByPredicate by Name | Duplicate WBP_ assets |
| ParseWidgets | StartsWith for variables: detection | Variables section parsing failure |
| ParseWidgets | Nested variable accumulation | Variables created as separate assets |
| ParseActorBlueprints | ContainsByPredicate by Name | Duplicate BP_ actors |
| ParseActorBlueprints | StartsWith for components: detection | Components section parsing failure |
| ParseActorBlueprints | StartsWith for variables: detection | Variables section parsing failure |
| ParseActorBlueprints | Nested component accumulation | Components created as separate assets |
| ParseActorBlueprints | Nested variable accumulation | Variables created as separate assets |
| ParseBehaviorTrees | ContainsByPredicate by Name | Duplicate BT_ assets |
| ParseInputMappingContexts | ContainsByPredicate by Name | Duplicate IMC_ assets |

| Scenario | Without Safeguard | With Safeguard |
|----------|-------------------|----------------|
| Last item in section | Added twice (end-of-loop + section-end) | Added once |
| Manifest with 93 assets | Generates 105+ with duplicates | Generates exactly 93 |
| Re-running generation | May create duplicate entries | Skips already-added items |

#### Struct Initialization Defaults

All struct properties have explicit default values to prevent uninitialized memory errors.

| Struct | Property | Default Value |
|--------|----------|---------------|
| FManifestCurveKeyDefinition | Time | 0.0f |
| FManifestCurveKeyDefinition | Value | 0.0f |
| FManifestModifierDefinition | ScalableFloatValue | 0.0f |
| FManifestGameplayEffectDefinition | DurationMagnitude | 0.0f |
| FManifestGameplayEffectDefinition | bExecutePeriodicOnApplication | false |
| FManifestGameplayEffectDefinition | StackLimitCount | 0 |
| FManifestItemCollectionEntry | Quantity | 1 |
| FManifestActivityConfigDefinition | RequestInterval | 0.5f |
| FManifestBlackboardKeyDefinition | bInstanceSynced | false |
| FManifestWidgetVariableDefinition | bInstanceEditable | false |
| FManifestWidgetVariableDefinition | bExposeOnSpawn | false |
| FManifestActorVariableDefinition | bReplicated | false |
| FManifestActorVariableDefinition | bInstanceEditable | false |
| FManifestAssetStateEntry | bExists | false |

#### Parent Class Resolution

FindParentClass searches multiple module paths to locate blueprint parent classes.

| Search Order | Module Path | Example Classes |
|--------------|-------------|-----------------|
| 1 | /Script/NarrativePro.{Class} | Core narrative classes |
| 2 | /Script/NarrativeArsenal.{Class} | Abilities, effects, NPCs |
| 3 | /Script/Engine.{Class} | Actor, Pawn, Character |
| 4 | /Script/UMG.{Class} | UserWidget for WBP_ assets |
| 5 | /Game/FatherCompanion/Characters/{Class} | Custom character blueprints |
| 6 | /Game/NarrativePro/Characters/{Class} | Narrative Pro character blueprints |
| 7 | Fallback script path | Default parent if not found |

| Asset Type | Fallback Path |
|------------|---------------|
| Actor Blueprints | /Script/Engine.Actor |
| Widget Blueprints | /Script/UMG.UserWidget |
| Gameplay Abilities | /Script/NarrativeArsenal.NarrativeGameplayAbility |
| Gameplay Effects | /Script/GameplayAbilities.GameplayEffect |
| Activities | /Script/NarrativeArsenal.NarrativeActivityBase |
| Events | /Script/NarrativeArsenal.NarrativeEvent |

#### Section Boundary Detection

Parser correctly identifies section boundaries using indent-level tracking. All parse functions MUST implement section exit logic to prevent cross-section bleeding.

| Element | Detection Method |
|---------|------------------|
| Section start | Line matches section keyword (e.g., gameplay_effects:) |
| Section indent | Stored when section starts (SectionIndent variable) |
| Section end | Non-empty line at lower/equal indent without list marker |
| Item boundary | Line starting with "- name:" at section indent + 2 |

| Edge Case | Handling |
|-----------|----------|
| Empty lines between items | Ignored, parsing continues |
| Comment lines (# ...) | Treated as section boundary if at section indent |
| Nested arrays | Tracked with separate indent variables |
| Final item in file | Added in post-loop cleanup with duplicate check |
| New section encountered | Current item saved, bInSection set to false |

#### Parser Section Exit Pattern (Required)

Every parse function MUST include this pattern immediately after section detection:

| Line | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | int32 SectionIndent = -1; | Track section start indent |
| 2 | SectionIndent = CurrentIndent; (in section start) | Store indent when entering |
| 3 | if (bInSection and CurrentIndent <= SectionIndent and !TrimmedLine.IsEmpty() and !TrimmedLine.StartsWith("-")) | Detect section boundary |
| 4 | Save current item, set bInSection = false, continue | Clean exit from section |

Failure to implement section exit causes:

| Symptom | Root Cause | Affected Assets |
|---------|------------|-----------------|
| Wrong asset types in category | Parser continues into next section | All assets after missing boundary |
| Duplicate asset entries | Same asset parsed by multiple functions | Cross-section assets |
| Incorrect asset counts | Assets counted under wrong category | Summary statistics |

#### Section Name Aliases

Parsers must accept multiple section names for compatibility with different manifest versions.

| Parser Function | Primary Name | Alias Names |
|-----------------|--------------|-------------|
| ParseCurves | float_curves: | curves: |
| ParseWidgets | widgets: | widget_blueprints: |
| ParseActors | actors: | actor_blueprints: |
| ParseAbilities | gameplay_abilities: | abilities: |
| ParseEffects | gameplay_effects: | effects: |
| ParseTags | tags: | gameplay_tags: |
| ParseInputActions | input_actions: | inputs: |
| ParseBlackboards | blackboards: | ai_blackboards: |
| ParseBehaviorTrees | behavior_trees: | ai_behavior_trees: |
| ParseActivities | activities: | ai_activities: |
| ParseGoals | goals: | npc_goals: |
| ParseGoalGenerators | goal_generators: | npc_goal_generators: |
| ParseTaggedDialogueSets | tagged_dialogue_sets: | dialogue_sets: |
| ParseBTServices | bt_services: | behavior_tree_services: |
| ParseBTTasks | bt_tasks: | behavior_tree_tasks: |
| ParseBTDecorators | bt_decorators: | behavior_tree_decorators: |
| ParseEnums | enumerations: | enums: |
| ParseIMCs | input_mapping_contexts: | imcs: |
| ParseEquippableItems | equippable_items: | items: |
| ParseConfigurations | configurations: | configs: |
| ParseEvents | narrative_events: | events: |

#### Common YAML Formatting Issues

These formatting issues have caused generation failures and are now validated by the parser.

| Issue | Symptom | Solution |
|-------|---------|----------|
| Blackboard keys at wrong indent | Keys created as separate BB_ assets | Nest keys under keys: property with proper indentation |
| Missing keys: section marker | No keys added to blackboard | Add keys: line before key definitions |
| Using - name: for keys | Parser treats as new blackboard | Use - key_name: for blackboard keys |
| Incomplete tag list | Only partial tags generated | Extract all tags from reference INI file |
| Missing comment field in tags | Tags may fail validation | Include comment field for each tag |

Blackboard key type mapping:

| key_type Value | UE Blackboard Key Class |
|----------------|------------------------|
| Object | UBlackboardKeyType_Object |
| Bool | UBlackboardKeyType_Bool |
| Float | UBlackboardKeyType_Float |
| Int | UBlackboardKeyType_Int |
| Vector | UBlackboardKeyType_Vector |
| Rotator | UBlackboardKeyType_Rotator |
| String | UBlackboardKeyType_String |
| Name | UBlackboardKeyType_Name |
| Class | UBlackboardKeyType_Class |
| Enum | UBlackboardKeyType_Enum |

#### Known Parser Bugs History

Reference for historical bugs and their fixes to prevent regression.

| Version | Bug ID | Parser | Symptom | Root Cause | Fix Applied |
|---------|--------|--------|---------|------------|-------------|
| v1.2.4 | BUG-001 | ParseBlackboards | Keys created as separate BB_ assets | Used Equals instead of StartsWith for keys: detection | Changed to StartsWith, added key accumulation |
| v1.3.1 | BUG-002 | ParseCurves | BP_, WBP_, IMC_ listed under Float Curves | No section exit logic | Added indent-based section exit with SectionIndent variable |
| v1.3.1 | BUG-003 | ParseWidgets | Widget Blueprints category missing | Parser looked for widgets: not widget_blueprints: | Added widget_blueprints: as accepted section name |
| v2.1.9 | BUG-014 | ParseActorBlueprints | Components and variables created as separate BP_ assets | No nested property detection for components: and variables: subsections | Added subsection tracking with bInComponents, bInVariables flags and indent-based accumulation |
| v2.1.9 | BUG-014 | ParseWidgets | Variables created as separate WBP_ assets | No nested property detection for variables: subsection | Added subsection tracking with bInVariables flag and indent-based accumulation |

#### Parser Verification Checklist

Before releasing new plugin version, verify each parser:

| Check | How to Verify | Expected Result |
|-------|---------------|-----------------|
| Section exit | Run with multi-section manifest | Each section parses only its own assets |
| Section aliases | Test all alias names | All names accepted for each section |
| Duplicate prevention | Run twice without deleting assets | Second run shows all SKIPPED, zero duplicates |
| Final item handling | Check last item in each section | Last item added without duplication |
| Empty section | Test section with no items | No errors, empty array returned |
| Nested properties | Test blackboards with keys | Keys added to parent, not as separate assets |
| Actor nested properties | Test actor blueprints with components and variables | Components and variables added to parent, not as separate assets |
| Widget nested properties | Test widget blueprints with variables | Variables added to parent, not as separate assets |

#### Parser Test Manifest

Use this minimal manifest to verify parser boundaries:

| Section Order | Content | Expected Parse Count |
|---------------|---------|---------------------|
| 1. float_curves | FC_Test1 | 1 curve only |
| 2. actor_blueprints | BP_Test1 (3 components, 5 variables), BP_Test2 | 2 actors only (not 10) |
| 3. widget_blueprints | WBP_Test1 (2 variables) | 1 widget only (not 3) |
| 4. input_mapping_contexts | IMC_Test1 | 1 IMC only |

If BP_ appears under Float Curves category, ParseCurves is missing section exit logic.
If AbilitySystemComponent appears as separate actor blueprint, ParseActorBlueprints is missing nested property handling.

#### BUG-014: Nested Blueprint Properties Fix (v2.1.9)

ParseActorBlueprints and ParseWidgets must handle nested components and variables subsections the same way ParseBlackboards handles nested keys.

| Parser | Nested Subsections | Detection Pattern |
|--------|-------------------|-------------------|
| ParseActorBlueprints | components:, variables: | StartsWith for subsection entry |
| ParseWidgets | variables: | StartsWith for subsection entry |

ParseActorBlueprints Required State Variables:

| Variable | Type | Purpose |
|----------|------|---------|
| bInComponents | bool | Tracks if currently inside components: subsection |
| bInVariables | bool | Tracks if currently inside variables: subsection |
| ComponentsIndent | int32 | Stores indent level where components: was found |
| VariablesIndent | int32 | Stores indent level where variables: was found |

ParseActorBlueprints Subsection Detection Pattern:

| Step | Condition | Action |
|------|-----------|--------|
| 1 | TrimmedLine.StartsWith("components:") | Set bInComponents = true, ComponentsIndent = CurrentIndent |
| 2 | TrimmedLine.StartsWith("variables:") | Set bInVariables = true, VariablesIndent = CurrentIndent |
| 3 | bInComponents and TrimmedLine.StartsWith("- name:") | Create FComponentDefinition, add to CurrentBlueprint.Components |
| 4 | bInVariables and TrimmedLine.StartsWith("- name:") | Create FActorVariableDefinition, add to CurrentBlueprint.Variables |
| 5 | bInComponents and CurrentIndent <= ComponentsIndent and !StartsWith("-") | Set bInComponents = false |
| 6 | bInVariables and CurrentIndent <= VariablesIndent and !StartsWith("-") | Set bInVariables = false |

Correct Parse Result for BP_FatherCompanion:

| Element | Count | Parent |
|---------|-------|--------|
| BP_FatherCompanion | 1 | ActorBlueprints array |
| AbilitySystemComponent | 1 | BP_FatherCompanion.Components array |
| EquipmentComponent | 1 | BP_FatherCompanion.Components array |
| InteractionComponent | 1 | BP_FatherCompanion.Components array |
| OwnerPlayer | 1 | BP_FatherCompanion.Variables array |
| CurrentForm | 1 | BP_FatherCompanion.Variables array |
| IsAttached | 1 | BP_FatherCompanion.Variables array |
| DomeEnergy | 1 | BP_FatherCompanion.Variables array |
| SymbioteCharge | 1 | BP_FatherCompanion.Variables array |

Incorrect Parse Result (before v2.1.9 fix):

| Element | Problem |
|---------|---------|
| BP_FatherCompanion | Created as actor blueprint (correct) |
| AbilitySystemComponent | Created as separate actor blueprint (wrong) |
| OwnerPlayer | Created as separate actor blueprint (wrong) |

Expected Log Output After Fix:

| Log Message | Status |
|-------------|--------|
| Created Actor Blueprint: BP_FatherCompanion (3 components, 5 variables) | Correct |
| Created Actor Blueprint: BP_LaserProjectile (0 components, 2 variables) | Correct |
| Created Actor Blueprint: BP_TurretProjectile (0 components, 2 variables) | Correct |
| Created Actor Blueprint: BP_ElectricTrap (2 components, 3 variables) | Correct |

### Narrative Pro Module Reference

Narrative Pro classes are split across multiple modules. Using incorrect module paths causes generation failures.

| Class | Module | Full Path |
|-------|--------|-----------|
| NarrativeGameplayAbility | NarrativeArsenal | /Script/NarrativeArsenal.NarrativeGameplayAbility |
| NarrativeCombatAbility | NarrativeArsenal | /Script/NarrativeArsenal.NarrativeCombatAbility |
| AbilityConfiguration | NarrativeArsenal | /Script/NarrativeArsenal.AbilityConfiguration |
| NPCActivityConfiguration | NarrativeArsenal | /Script/NarrativeArsenal.NPCActivityConfiguration |
| NPCDefinition | NarrativeArsenal | /Script/NarrativeArsenal.NPCDefinition |
| CharacterDefinition | NarrativeArsenal | /Script/NarrativeArsenal.CharacterDefinition |
| NarrativeEvent | NarrativeArsenal | /Script/NarrativeArsenal.NarrativeEvent |
| NarrativeActivityBase | NarrativeArsenal | /Script/NarrativeArsenal.NarrativeActivityBase |
| NarrativeNPCCharacter | NarrativeArsenal | /Script/NarrativeArsenal.NarrativeNPCCharacter |
| NarrativeProjectile | NarrativeArsenal | /Script/NarrativeArsenal.NarrativeProjectile |
| EquippableItem | NarrativeArsenal | /Script/NarrativeArsenal.EquippableItem |
| NarrativeAttributeSetBase | NarrativeArsenal | /Script/NarrativeArsenal.NarrativeAttributeSetBase |
| NarrativeDamageExecCalc | NarrativeArsenal | /Script/NarrativeArsenal.NarrativeDamageExecCalc |
| NPCGoalItem | NarrativeArsenal | /Script/NarrativeArsenal.NPCGoalItem |
| NPCGoalGenerator | NarrativeArsenal | /Script/NarrativeArsenal.NPCGoalGenerator |
| TaggedDialogueSet | NarrativeArsenal | /Script/NarrativeArsenal.TaggedDialogueSet |
| BTService_BlueprintBase | AIModule | /Script/AIModule.BTService_BlueprintBase |
| BTTask_BlueprintBase | AIModule | /Script/AIModule.BTTask_BlueprintBase |
| BTDecorator_BlueprintBase | AIModule | /Script/AIModule.BTDecorator_BlueprintBase |

| Module | Content |
|--------|---------|
| NarrativeArsenal | Gameplay abilities, effects, items, NPCs, goals, attributes |
| NarrativePro | Core systems, save system, UI, navigation |
| AIModule | Behavior tree nodes, blackboards, AI controllers |

### Parent Class Search Order

The FindParentClass function searches multiple modules to resolve parent class paths. This is necessary because classes like UserWidget exist in the UMG module, not in Narrative Pro modules.

#### Search Order by Asset Type

| Asset Type | Search Order | Final Fallback |
|------------|--------------|----------------|
| Widget Blueprints | NarrativePro -> NarrativeArsenal -> Engine -> UMG | /Script/UMG.UserWidget |
| Actor Blueprints | NarrativePro -> NarrativeArsenal -> Engine | /Script/Engine.Actor |
| Gameplay Abilities | NarrativeArsenal -> GameplayAbilities | /Script/GameplayAbilities.GameplayAbility |
| Gameplay Effects | GameplayAbilities | /Script/GameplayAbilities.GameplayEffect |
| Activities | NarrativeArsenal | /Script/NarrativeArsenal.NarrativeActivityBase |
| Goals | NarrativeArsenal | /Script/NarrativeArsenal.NPCGoalItem |
| Goal Generators | NarrativeArsenal | /Script/NarrativeArsenal.NPCGoalGenerator |
| Tagged Dialogue Sets | NarrativeArsenal | /Script/NarrativeArsenal.TaggedDialogueSet |
| BT Services | AIModule | /Script/AIModule.BTService_BlueprintBase |
| BT Tasks | AIModule | /Script/AIModule.BTTask_BlueprintBase |
| BT Decorators | AIModule | /Script/AIModule.BTDecorator_BlueprintBase |

#### UserWidget Resolution

UserWidget is located in the UMG module, not Engine. The plugin searches in order:

| Order | Module Path | Result |
|-------|-------------|--------|
| 1 | /Script/NarrativePro.UserWidget | Failed to find object |
| 2 | /Script/NarrativeArsenal.UserWidget | Failed to find object |
| 3 | /Script/Engine.UserWidget | Failed to find object |
| 4 | /Script/UMG.UserWidget | SUCCESS |

These "Failed to find object" warnings are expected and logged during normal operation. They indicate the search process, not errors.

#### Expected Log Warnings

During widget blueprint generation, the following warnings are normal:

| Warning | Meaning |
|---------|---------|
| Failed to find object 'Class /Script/NarrativePro.UserWidget' | Search step 1 - expected |
| Failed to find object 'Class /Script/NarrativeArsenal.UserWidget' | Search step 2 - expected |
| Failed to find object 'Class /Script/Engine.UserWidget' | Search step 3 - expected |

These warnings do not indicate failure. The asset is created successfully when the final UMG module search succeeds.

### Plugin Module Dependencies

The FatherAbilityGenerator plugin requires specific UE modules in Build.cs to compile successfully.

#### Required Public Dependencies

| Module | Purpose | Headers/Symbols Used |
|--------|---------|----------------------|
| Core | Core UE types | FString, TArray, UObject |
| CoreUObject | Object system | UClass, UPackage |
| Engine | Engine classes | AActor, UActorComponent |
| InputCore | Input system | Key bindings |
| EnhancedInput | Enhanced Input | UInputAction, UInputMappingContext |
| GameplayAbilities | GAS | UGameplayAbility, UGameplayEffect |
| GameplayTags | Tag system | FGameplayTag, FGameplayTagContainer |
| GameplayTasks | Ability tasks | UAbilityTask |
| AIModule | AI system | UBlackboardData, UBehaviorTree |
| Slate | UI framework | Slate widgets |
| SlateCore | Slate core | Slate types |
| EditorStyle | Editor styling | Editor brushes |
| UnrealEd | Editor utilities | FAssetData, editor commands |
| AssetTools | Asset creation | IAssetTools, UFactory |
| ContentBrowser | Content browser | Asset registration |
| EditorSubsystem | Editor subsystems | UEditorSubsystem |
| Blutility | Editor utilities | Editor utility widgets |
| UMG | UMG runtime | UUserWidget |
| UMGEditor | UMG editor | WidgetBlueprint.h, WidgetBlueprintFactory.h |
| BlueprintGraph | Blueprint graph | UEdGraphSchema_K2::PC_* pin types |
| Kismet | Blueprint utilities | FBlueprintEditorUtils, BlueprintEditorUtils.h |
| PropertyEditor | Property customization | Details panel |
| ToolMenus | Tool menus | Menu registration |
| DesktopPlatform | File dialogs | Open/save dialogs |

#### Event Graph Generation Module Dependencies

To enable programmatic Blueprint Event Graph and Material Graph generation, the plugin requires additional modules.

| Module | Purpose | Headers/Symbols Used |
|--------|---------|----------------------|
| BlueprintGraph | UK2Node classes | UK2Node_CallFunction, UK2Node_Event, UK2Node_DynamicCast |
| Kismet | Blueprint utilities | FBlueprintEditorUtils, CreateNewGraph, AddUbergraphPage |
| KismetCompiler | Blueprint compilation | Compilation after graph modification |
| GraphEditor | Graph manipulation | Node positioning, connection handling |


---

## NEW GENERATOR IMPLEMENTATION: FEventGraphGenerator

### Purpose

Generates complete Blueprint Event Graph logic programmatically using UK2Node APIs, eliminating the need for manual node placement. This enables FULL auto-generation for GA_, BP_, and WBP_ assets.

### Required Headers

| Header | Contents |
|--------|----------|
| BlueprintGraph/Classes/K2Node_CallFunction.h | UK2Node_CallFunction |
| BlueprintGraph/Classes/K2Node_Event.h | UK2Node_Event |
| BlueprintGraph/Classes/K2Node_DynamicCast.h | UK2Node_DynamicCast |
| BlueprintGraph/Classes/K2Node_IfThenElse.h | UK2Node_IfThenElse |
| BlueprintGraph/Classes/K2Node_VariableGet.h | UK2Node_VariableGet |
| BlueprintGraph/Classes/K2Node_VariableSet.h | UK2Node_VariableSet |
| BlueprintGraph/Classes/K2Node_ExecutionSequence.h | UK2Node_ExecutionSequence |
| BlueprintGraph/Classes/K2Node_CustomEvent.h | UK2Node_CustomEvent |
| BlueprintGraph/Classes/K2Node_MakeArray.h | UK2Node_MakeArray |
| BlueprintGraph/Classes/K2Node_ForEachElementInArray.h | UK2Node_ForEachElementInArray |
| Kismet/Public/BlueprintEditorUtils.h | FBlueprintEditorUtils |
| EdGraph/EdGraph.h | UEdGraph |
| EdGraph/EdGraphNode.h | UEdGraphNode |
| EdGraph/EdGraphPin.h | UEdGraphPin |

### Node Creation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(Blueprint) | Get event graph |
| 2 | FGraphNodeCreator<UK2Node_CallFunction> Creator(*EventGraph) | Prepare node creator |
| 3 | UK2Node_CallFunction* Node = Creator.CreateNode() | Create the node instance |
| 4 | Node->FunctionReference.SetExternalMember(FName("GetAvatarActorFromActorInfo"), UGameplayAbility::StaticClass()) | Set function reference |
| 5 | Node->AllocateDefaultPins() | Create pins |
| 6 | Node->NodePosX = 300; Node->NodePosY = 0 | Set position |
| 7 | Creator.Finalize() | Complete node creation |

### Alternative Node Spawner Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | UFunction* Function = ParentClass->FindFunctionByName(FunctionName) | Get function reference |
| 2 | UBlueprintFunctionNodeSpawner* Spawner = UBlueprintFunctionNodeSpawner::Create(Function) | Create spawner |
| 3 | UEdGraphNode* Node = Spawner->Invoke(EventGraph, IBlueprintNodeBinder::FBindingSet(), FVector2D(X, Y)) | Spawn at location |

### Pin Connection Pattern

| Connection Type | Code Pattern |
|-----------------|--------------|
| Execution pins | NodeA->GetThenPin()->MakeLinkTo(NodeB->GetExecPin()) |
| Data pins (by name) | NodeA->FindPin(TEXT("ReturnValue"), EGPD_Output)->MakeLinkTo(NodeB->FindPin(TEXT("Target"), EGPD_Input)) |
| Cast output | CastNode->FindPin(TEXT("AsBP_FatherCompanion"), EGPD_Output)->MakeLinkTo(TargetNode->FindPin(TEXT("Target"), EGPD_Input)) |

### Event Node Creation

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | FGraphNodeCreator<UK2Node_Event> Creator(*EventGraph) | Prepare event creator |
| 2 | UK2Node_Event* EventNode = Creator.CreateNode() | Create event node |
| 3 | EventNode->EventReference.SetExternalMember(FName("K2_ActivateAbility"), UGameplayAbility::StaticClass()) | Set event reference |
| 4 | EventNode->AllocateDefaultPins() | Create pins |
| 5 | Creator.Finalize() | Complete creation |

### Cast Node Creation

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | FGraphNodeCreator<UK2Node_DynamicCast> Creator(*EventGraph) | Prepare cast creator |
| 2 | UK2Node_DynamicCast* CastNode = Creator.CreateNode() | Create cast node |
| 3 | CastNode->TargetType = BP_FatherCompanion::StaticClass() | Set target class |
| 4 | CastNode->AllocateDefaultPins() | Create pins |
| 5 | Creator.Finalize() | Complete creation |

### Variable Node Creation

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName("FatherRef"), FEdGraphPinType()) | Add variable |
| 2 | FGraphNodeCreator<UK2Node_VariableSet> Creator(*EventGraph) | Prepare setter creator |
| 3 | UK2Node_VariableSet* SetNode = Creator.CreateNode() | Create setter node |
| 4 | SetNode->VariableReference.SetSelfMember(FName("FatherRef")) | Set variable reference |
| 5 | SetNode->AllocateDefaultPins() | Create pins |

### Blueprint Finalization

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint) | Mark modified |
| 2 | FKismetEditorUtilities::CompileBlueprint(Blueprint) | Compile |
| 3 | Blueprint->PostEditChange() | Notify change |
| 4 | Blueprint->MarkPackageDirty() | Mark for save |

### Auto-Layout Algorithm

| Rule | Value | Purpose |
|------|-------|---------|
| Horizontal spacing | 300 pixels | Between sequential nodes |
| Vertical spacing | 100 pixels | Between parallel branches |
| Event start position | (0, 0) | Anchor point |
| Branch offset | +100 Y per branch | Parallel path separation |
| Comment block margin | 50 pixels | Space around grouped nodes |

---

## YAML SCHEMA: event_graph (FULL SPECIFICATION)

### Schema Definition

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| event_graph | Object | Yes | Root event graph definition |
| event_graph.nodes | Array | Yes | List of node definitions |
| event_graph.connections | Array | Yes | List of pin connections |
| event_graph.variables | Array | No | Local variables to create |

### Node Definition Schema

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| id | String | Yes | Unique node identifier for connections |
| type | String | Yes | Node type (see Supported Node Types) |
| position | Array | No | [X, Y] position in graph (auto-layout if omitted) |
| properties | Object | No | Type-specific properties |

### Supported Node Types

| type Value | UK2Node Class | Properties |
|------------|---------------|------------|
| Event | UK2Node_Event | event_name, event_class |
| CallFunction | UK2Node_CallFunction | function, class, target_self |
| DynamicCast | UK2Node_DynamicCast | target_class |
| Branch | UK2Node_IfThenElse | - |
| VariableGet | UK2Node_VariableGet | variable_name |
| VariableSet | UK2Node_VariableSet | variable_name |
| Sequence | UK2Node_ExecutionSequence | num_outputs |
| ForEachLoop | UK2Node_ForEachElementInArray | - |
| MakeArray | UK2Node_MakeArray | element_type, num_elements |
| CustomEvent | UK2Node_CustomEvent | event_name |
| Delay | UK2Node_Delay | duration |
| SpawnActor | UK2Node_SpawnActorFromClass | class |
| MakeLiteralFloat | UK2Node_MakeLiteralFloat | value |
| MakeLiteralInt | UK2Node_MakeLiteralInt | value |
| MakeLiteralBool | UK2Node_MakeLiteralBool | value |
| MakeLiteralString | UK2Node_MakeLiteralString | value |

### Connection Definition Schema

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| from | Array | Yes | [node_id, pin_name] source |
| to | Array | Yes | [node_id, pin_name] destination |

### Pin Name Reference (Common Nodes)

| Node Type | Output Pins | Input Pins |
|-----------|-------------|------------|
| Event | Then, ActorInfo | - |
| CallFunction | Exec, ReturnValue, [output params] | Exec, Target, [input params] |
| DynamicCast | Exec, CastFailed, As[ClassName] | Exec, Object |
| Branch | True, False | Exec, Condition |
| VariableGet | [VariableName] | - |
| VariableSet | Exec, [VariableName] | Exec, [VariableName] |
| Sequence | Then 0, Then 1, Then 2... | Exec |
| ForEachLoop | LoopBody, Completed, ArrayElement, ArrayIndex | Exec, Array |

### Variable Definition Schema

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| name | String | Yes | Variable name |
| type | String | Yes | Pin type (Object, Float, Int, Bool, String, Vector, Rotator, Transform) |
| class | String | For Object | Object class path |
| default_value | Various | No | Default value |
| instance_editable | Bool | No | Expose in details panel |
| replicated | Bool | No | Replicate to clients |

### Example: GA_FatherArmor Event Graph YAML

| Line | Content |
|------|---------|
| 1 | event_graph: |
| 2 | (2)variables: |
| 3 | (4)- name: FatherRef |
| 4 | (6)type: Object |
| 5 | (6)class: BP_FatherCompanion |
| 6 | (4)- name: PlayerRef |
| 7 | (6)type: Object |
| 8 | (6)class: NarrativePlayerCharacter |
| 9 | (2)nodes: |
| 10 | (4)- id: Event_Activate |
| 11 | (6)type: Event |
| 12 | (6)properties: |
| 13 | (8)event_name: K2_ActivateAbility |
| 14 | (8)event_class: GameplayAbility |
| 15 | (6)position: [0, 0] |
| 16 | (4)- id: GetAvatarActor |
| 17 | (6)type: CallFunction |
| 18 | (6)properties: |
| 19 | (8)function: GetAvatarActorFromActorInfo |
| 20 | (8)class: GameplayAbility |
| 21 | (8)target_self: true |
| 22 | (6)position: [300, 0] |
| 23 | (4)- id: CastToFather |
| 24 | (6)type: DynamicCast |
| 25 | (6)properties: |
| 26 | (8)target_class: BP_FatherCompanion |
| 27 | (6)position: [600, 0] |
| 28 | (4)- id: GetOwnerPlayer |
| 29 | (6)type: CallFunction |
| 30 | (6)properties: |
| 31 | (8)function: GetOwnerPlayer |
| 32 | (8)class: BP_FatherCompanion |
| 33 | (6)position: [900, 0] |
| 34 | (4)- id: IsValid_Player |
| 35 | (6)type: CallFunction |
| 36 | (6)properties: |
| 37 | (8)function: IsValid |
| 38 | (8)class: KismetSystemLibrary |
| 39 | (6)position: [1200, 0] |
| 40 | (4)- id: Branch_Valid |
| 41 | (6)type: Branch |
| 42 | (6)position: [1500, 0] |
| 43 | (4)- id: SetFatherRef |
| 44 | (6)type: VariableSet |
| 45 | (6)properties: |
| 46 | (8)variable_name: FatherRef |
| 47 | (6)position: [1800, 0] |
| 48 | (4)- id: ApplyArmorGE |
| 49 | (6)type: CallFunction |
| 50 | (6)properties: |
| 51 | (8)function: ApplyGameplayEffectToOwner |
| 52 | (8)class: GameplayAbility |
| 53 | (6)position: [2100, 0] |
| 54 | (4)- id: CommitAbility |
| 55 | (6)type: CallFunction |
| 56 | (6)properties: |
| 57 | (8)function: CommitAbility |
| 58 | (8)class: GameplayAbility |
| 59 | (6)position: [2400, 0] |
| 60 | (2)connections: |
| 61 | (4)- from: [Event_Activate, Then] |
| 62 | (6)to: [GetAvatarActor, Exec] |
| 63 | (4)- from: [GetAvatarActor, Exec] |
| 64 | (6)to: [CastToFather, Exec] |
| 65 | (4)- from: [GetAvatarActor, ReturnValue] |
| 66 | (6)to: [CastToFather, Object] |
| 67 | (4)- from: [CastToFather, Exec] |
| 68 | (6)to: [GetOwnerPlayer, Exec] |
| 69 | (4)- from: [CastToFather, AsBP_FatherCompanion] |
| 70 | (6)to: [GetOwnerPlayer, Target] |
| 71 | (4)- from: [CastToFather, AsBP_FatherCompanion] |
| 72 | (6)to: [SetFatherRef, FatherRef] |
| 73 | (4)- from: [GetOwnerPlayer, Exec] |
| 74 | (6)to: [IsValid_Player, Exec] |
| 75 | (4)- from: [GetOwnerPlayer, ReturnValue] |
| 76 | (6)to: [IsValid_Player, InputObject] |
| 77 | (4)- from: [IsValid_Player, Exec] |
| 78 | (6)to: [Branch_Valid, Exec] |
| 79 | (4)- from: [IsValid_Player, ReturnValue] |
| 80 | (6)to: [Branch_Valid, Condition] |
| 81 | (4)- from: [Branch_Valid, True] |
| 82 | (6)to: [SetFatherRef, Exec] |
| 83 | (4)- from: [SetFatherRef, Exec] |
| 84 | (6)to: [ApplyArmorGE, Exec] |
| 85 | (4)- from: [ApplyArmorGE, Exec] |
| 86 | (6)to: [CommitAbility, Exec] |

---

## NEW GENERATOR IMPLEMENTATION: FBehaviorTreeGenerator

### Purpose

Generates complete Behavior Tree node hierarchies using FBTCompositeChild arrays instead of pin-based connections.

### Key Discovery

Behavior Trees differ fundamentally from Blueprint graphs by using FBTCompositeChild arrays rather than pin connections. The runtime classes in AIModule provide everything needed for programmatic creation.

### Required Headers

| Header | Contents |
|--------|----------|
| BehaviorTree/BehaviorTree.h | UBehaviorTree |
| BehaviorTree/BlackboardData.h | UBlackboardData |
| BehaviorTree/Blackboard/BlackboardKeyType_Object.h | UBlackboardKeyType_Object |
| BehaviorTree/Blackboard/BlackboardKeyType_Bool.h | UBlackboardKeyType_Bool |
| BehaviorTree/Blackboard/BlackboardKeyType_Float.h | UBlackboardKeyType_Float |
| BehaviorTree/Blackboard/BlackboardKeyType_Vector.h | UBlackboardKeyType_Vector |
| BehaviorTree/Composites/BTComposite_Selector.h | UBTComposite_Selector |
| BehaviorTree/Composites/BTComposite_Sequence.h | UBTComposite_Sequence |
| BehaviorTree/Tasks/BTTask_MoveTo.h | UBTTask_MoveTo |
| BehaviorTree/BTFunctionLibrary.h | BT utilities |

### BT Creation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | UBehaviorTree* BT = NewObject<UBehaviorTree>(Package, AssetName, RF_Public OR RF_Standalone) | Create BT asset |
| 2 | BT->BlackboardAsset = NewObject<UBlackboardData>(BT, TEXT("BlackboardData")) | Create embedded blackboard |
| 3 | FBlackboardEntry Entry; Entry.EntryName = FName("TargetActor") | Define blackboard key |
| 4 | Entry.KeyType = NewObject<UBlackboardKeyType_Object>(BT->BlackboardAsset) | Set key type |
| 5 | BT->BlackboardAsset->Keys.Add(Entry) | Add key to blackboard |
| 6 | BT->RootNode = NewObject<UBTComposite_Selector>(BT, TEXT("Root")) | Create root composite |
| 7 | UBTTask_MoveTo* Task = NewObject<UBTTask_MoveTo>(BT->RootNode) | Create task node |
| 8 | FBTCompositeChild ChildEntry; ChildEntry.ChildTask = Task | Create child connection |
| 9 | BT->RootNode->Children.Add(ChildEntry) | Add child to composite |

### Decorator and Service Attachment

| Attachment Type | Pattern | Target |
|-----------------|---------|--------|
| Decorators | FBTCompositeChild.Decorators.Add(DecoratorNode) | Attach to child connection |
| Services | UBTCompositeNode::Services.Add(ServiceNode) | Attach to composite node |

### Common BT Node Classes

| Node Type | Class Name | Properties |
|-----------|------------|------------|
| Selector | BTComposite_Selector | - |
| Sequence | BTComposite_Sequence | - |
| Simple Parallel | BTComposite_SimpleParallel | FinishMode |
| Move To | BTTask_MoveTo | BlackboardKey, AcceptableRadius |
| Wait | BTTask_Wait | WaitTime |
| Run Behavior | BTTask_RunBehavior | BehaviorAsset |
| Activate Ability | BTTask_ActivateAbilityByClass | AbilityClass |
| Blackboard | BTDecorator_Blackboard | BlackboardKey, KeyValue |
| Cooldown | BTDecorator_Cooldown | CooldownTime |
| Loop | BTDecorator_Loop | NumLoops |
| Time Limit | BTDecorator_TimeLimit | TimeLimit |
| Default Focus | BTService_DefaultFocus | FocusOnKey |
| Run EQS | BTService_RunEQS | QueryTemplate |

---

## YAML SCHEMA: behavior_tree_graph (FULL SPECIFICATION)

### Schema Definition

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| behavior_tree_graph | Object | Yes | Root BT definition |
| behavior_tree_graph.blackboard_keys | Array | Yes | Blackboard key definitions |
| behavior_tree_graph.root_node | Object | Yes | Root composite node |

### Blackboard Key Schema

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| name | String | Yes | Key name |
| type | String | Yes | Object, Bool, Float, Int, Vector, Rotator, String, Name, Enum |
| base_class | String | For Object | Base class for Object keys |

### Node Schema

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| type | String | Yes | Selector, Sequence, SimpleParallel, Task |
| class | String | Yes | Specific node class |
| children | Array | For Composites | Child nodes |
| decorators | Array | No | Attached decorators |
| services | Array | For Composites | Attached services |
| properties | Object | No | Node-specific properties |

### Example: BT_FatherFollow YAML

| Line | Content |
|------|---------|
| 1 | behavior_tree_graph: |
| 2 | (2)blackboard_keys: |
| 3 | (4)- name: FollowTarget |
| 4 | (6)type: Object |
| 5 | (6)base_class: Actor |
| 6 | (4)- name: FollowDistance |
| 7 | (6)type: Float |
| 8 | (4)- name: IsAttached |
| 9 | (6)type: Bool |
| 10 | (2)root_node: |
| 11 | (4)type: Selector |
| 12 | (4)class: BTComposite_Selector |
| 13 | (4)services: |
| 14 | (6)- class: BTService_DefaultFocus |
| 15 | (8)properties: |
| 16 | (10)focus_on_key: FollowTarget |
| 17 | (4)children: |
| 18 | (6)- type: Sequence |
| 19 | (8)class: BTComposite_Sequence |
| 20 | (8)decorators: |
| 21 | (10)- class: BTDecorator_Blackboard |
| 22 | (12)properties: |
| 23 | (14)key_name: IsAttached |
| 24 | (14)key_value: false |
| 25 | (8)children: |
| 26 | (10)- type: Task |
| 27 | (12)class: BTTask_MoveTo |
| 28 | (12)properties: |
| 29 | (14)blackboard_key: FollowTarget |
| 30 | (14)acceptable_radius: 100.0 |

---

## NEW GENERATOR IMPLEMENTATION: FWidgetTreeGenerator

### Purpose

Generates complete Widget Blueprint visual hierarchies using WidgetTree::ConstructWidget API.

### Key Discovery

The UWidgetTree::ConstructWidget<T>() method is the correct way to create widgets for Widget Blueprints in editor context. NewObject<UWidget>() fails because widgets require the WidgetTree context.

### Required Headers

| Header | Contents |
|--------|----------|
| UMG/Public/Blueprint/WidgetTree.h | UWidgetTree |
| UMG/Public/Blueprint/WidgetBlueprint.h | UWidgetBlueprint |
| UMG/Public/Components/CanvasPanel.h | UCanvasPanel |
| UMG/Public/Components/CanvasPanelSlot.h | UCanvasPanelSlot |
| UMG/Public/Components/VerticalBox.h | UVerticalBox |
| UMG/Public/Components/HorizontalBox.h | UHorizontalBox |
| UMG/Public/Components/TextBlock.h | UTextBlock |
| UMG/Public/Components/Image.h | UImage |
| UMG/Public/Components/ProgressBar.h | UProgressBar |
| UMG/Public/Components/Button.h | UButton |
| UMG/Public/Components/Overlay.h | UOverlay |
| UMG/Public/Components/SizeBox.h | USizeBox |
| UMG/Public/Components/Border.h | UBorder |

### Widget Creation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | UWidgetBlueprint* Blueprint = ... | Get widget blueprint |
| 2 | UCanvasPanel* Root = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName("RootCanvas")) | Create root canvas |
| 3 | Blueprint->WidgetTree->RootWidget = Root | Set as root |
| 4 | UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName("MyText")) | Create child widget |
| 5 | UPanelSlot* Slot = Root->AddChild(Text) | Add to parent |
| 6 | UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot) | Get typed slot |
| 7 | CanvasSlot->SetPosition(FVector2D(100.f, 100.f)) | Configure position |
| 8 | CanvasSlot->SetSize(FVector2D(200.f, 50.f)) | Configure size |
| 9 | CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f)) | Configure anchors |
| 10 | FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint) | Mark modified |

### Container Slot Types

| Container Type | Slot Type | Layout Properties |
|----------------|-----------|-------------------|
| UCanvasPanel | UCanvasPanelSlot | Position, Size, Anchors, Alignment, ZOrder |
| UHorizontalBox | UHorizontalBoxSlot | Padding, Size, HAlign, VAlign |
| UVerticalBox | UVerticalBoxSlot | Padding, Size, HAlign, VAlign |
| UOverlay | UOverlaySlot | Padding, HAlign, VAlign |
| UUniformGridPanel | UUniformGridSlot | Row, Column |
| UGridPanel | UGridPanelSlot | Row, Column, RowSpan, ColumnSpan |
| UWidgetSwitcher | UWidgetSwitcherSlot | Padding, HAlign, VAlign |
| UScrollBox | UScrollBoxSlot | Padding, HAlign, VAlign |

### Common Widget Classes

| Widget Class | Purpose |
|--------------|---------|
| UCanvasPanel | Free-form positioning container |
| UHorizontalBox | Horizontal layout container |
| UVerticalBox | Vertical layout container |
| UOverlay | Stacked overlay container |
| UTextBlock | Display text |
| UImage | Display image/texture |
| UButton | Clickable button |
| UProgressBar | Progress bar display |
| USlider | Slider input |
| UCheckBox | Checkbox input |
| USpacer | Layout spacing |
| UBorder | Border container |
| USizeBox | Size constraint container |

---

## YAML SCHEMA: widget_tree (FULL SPECIFICATION)

### Schema Definition

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| widget_tree | Object | Yes | Root widget tree definition |
| widget_tree.root_widget | Object | Yes | Root widget definition |

### Widget Schema

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| type | String | Yes | Widget class name |
| name | String | Yes | Widget name for binding |
| is_variable | Bool | No | Expose as variable (default false) |
| properties | Object | No | Widget-specific properties |
| slot | Object | No | Parent slot configuration |
| children | Array | For Panels | Child widgets |

### Slot Configuration

| Field | Type | For Container | Description |
|-------|------|---------------|-------------|
| position | Array | Canvas | [X, Y] position |
| size | Array | Canvas | [Width, Height] |
| anchors | Array | Canvas | [MinX, MinY, MaxX, MaxY] |
| alignment | Array | Canvas | [X, Y] pivot (0-1) |
| z_order | Int | Canvas | Draw order |
| padding | Object | All | top, left, right, bottom |
| h_align | String | Box | Left, Center, Right, Fill |
| v_align | String | Box | Top, Center, Bottom, Fill |
| size_rule | String | Box | Auto, Fill |
| fill_weight | Float | Box | Fill ratio |

### Widget-Specific Properties

| Widget | Property | Type | Description |
|--------|----------|------|-------------|
| TextBlock | text | String | Display text |
| TextBlock | font_size | Int | Font size |
| TextBlock | color | Array | [R, G, B, A] text color |
| TextBlock | justification | String | Left, Center, Right |
| Image | brush_size | Array | [Width, Height] |
| Image | color_and_opacity | Array | [R, G, B, A] tint |
| ProgressBar | percent | Float | Fill percent (0-1) |
| ProgressBar | fill_color | Array | [R, G, B, A] |
| Button | is_enabled | Bool | Clickable state |

### Example: WBP_MarkIndicator YAML

| Line | Content |
|------|---------|
| 1 | widget_tree: |
| 2 | (2)root_widget: |
| 3 | (4)type: CanvasPanel |
| 4 | (4)name: RootCanvas |
| 5 | (4)children: |
| 6 | (6)- type: Image |
| 7 | (8)name: MarkIcon |
| 8 | (8)is_variable: true |
| 9 | (8)properties: |
| 10 | (10)brush_size: [32, 32] |
| 11 | (10)color_and_opacity: [1.0, 0.3, 0.3, 1.0] |
| 12 | (8)slot: |
| 13 | (10)position: [0, 0] |
| 14 | (10)size: [32, 32] |
| 15 | (10)anchors: [0.5, 0.5, 0.5, 0.5] |
| 16 | (10)alignment: [0.5, 0.5] |
| 17 | (6)- type: TextBlock |
| 18 | (8)name: TimerText |
| 19 | (8)is_variable: true |
| 20 | (8)properties: |
| 21 | (10)text: "5.0" |
| 22 | (10)font_size: 14 |
| 23 | (10)justification: Center |
| 24 | (10)color: [1.0, 1.0, 1.0, 1.0] |
| 25 | (8)slot: |
| 26 | (10)position: [0, 40] |
| 27 | (10)size: [50, 20] |
| 28 | (10)anchors: [0.5, 0.5, 0.5, 0.5] |
| 29 | (10)alignment: [0.5, 0.5] |

---

## NEW GENERATOR IMPLEMENTATION: FMaterialGraphGenerator

### Purpose

Generates complete Material expression graphs using UMaterialExpression classes.

### Required Headers

| Header | Contents |
|--------|----------|
| Materials/Material.h | UMaterial |
| Materials/MaterialExpressionConstant.h | UMaterialExpressionConstant |
| Materials/MaterialExpressionConstant3Vector.h | UMaterialExpressionConstant3Vector |
| Materials/MaterialExpressionConstant4Vector.h | UMaterialExpressionConstant4Vector |
| Materials/MaterialExpressionScalarParameter.h | UMaterialExpressionScalarParameter |
| Materials/MaterialExpressionVectorParameter.h | UMaterialExpressionVectorParameter |
| Materials/MaterialExpressionTextureSample.h | UMaterialExpressionTextureSample |
| Materials/MaterialExpressionMultiply.h | UMaterialExpressionMultiply |
| Materials/MaterialExpressionAdd.h | UMaterialExpressionAdd |
| Materials/MaterialExpressionLinearInterpolate.h | UMaterialExpressionLinearInterpolate |
| Materials/MaterialExpressionFresnel.h | UMaterialExpressionFresnel |
| Materials/MaterialExpressionTime.h | UMaterialExpressionTime |
| Materials/MaterialExpressionPanner.h | UMaterialExpressionPanner |
| Factories/MaterialFactoryNew.h | UMaterialFactoryNew |

### Material Creation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | auto MaterialFactory = NewObject<UMaterialFactoryNew>() | Create factory |
| 2 | UMaterial* Material = (UMaterial*)MaterialFactory->FactoryCreateNew(UMaterial::StaticClass(), Package, AssetName, RF_Public OR RF_Standalone, nullptr, GWarn) | Create material |
| 3 | UMaterialExpressionConstant3Vector* ColorConst = NewObject<UMaterialExpressionConstant3Vector>(Material) | Create expression |
| 4 | ColorConst->Constant = FLinearColor(1.0f, 0.3f, 0.1f) | Set value |
| 5 | ColorConst->MaterialExpressionEditorX = 0 | Set position X |
| 6 | ColorConst->MaterialExpressionEditorY = 0 | Set position Y |
| 7 | Material->Expressions.Add(ColorConst) | Add to material |
| 8 | Material->BaseColor.Expression = ColorConst | Connect to output |

### Expression Connection Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | UMaterialExpressionMultiply* Mult = NewObject<UMaterialExpressionMultiply>(Material) | Create multiply |
| 2 | Mult->A.Expression = ColorConst | Connect A input |
| 3 | Mult->B.Expression = ScalarParam | Connect B input |
| 4 | Material->EmissiveColor.Expression = Mult | Connect to output |

### Material Output Pins

| Output | Property |
|--------|----------|
| Base Color | Material->BaseColor.Expression |
| Metallic | Material->Metallic.Expression |
| Specular | Material->Specular.Expression |
| Roughness | Material->Roughness.Expression |
| Emissive Color | Material->EmissiveColor.Expression |
| Opacity | Material->Opacity.Expression |
| Opacity Mask | Material->OpacityMask.Expression |
| Normal | Material->Normal.Expression |
| World Position Offset | Material->WorldPositionOffset.Expression |
| Subsurface Color | Material->SubsurfaceColor.Expression |
| Ambient Occlusion | Material->AmbientOcclusion.Expression |

---

## YAML SCHEMA: material_graph (FULL SPECIFICATION)

### Schema Definition

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| material_graph | Object | Yes | Root material definition |
| material_graph.blend_mode | String | No | Opaque, Translucent, Additive, Modulate, Masked |
| material_graph.shading_model | String | No | DefaultLit, Unlit, Subsurface, TwoSidedFoliage |
| material_graph.two_sided | Bool | No | Enable two-sided rendering |
| material_graph.expressions | Array | Yes | Expression node definitions |
| material_graph.connections | Array | Yes | Expression connections |

### Expression Types

| type Value | Class | Properties |
|------------|-------|------------|
| Constant | UMaterialExpressionConstant | r (float) |
| Constant3Vector | UMaterialExpressionConstant3Vector | constant (RGB array) |
| Constant4Vector | UMaterialExpressionConstant4Vector | constant (RGBA array) |
| ScalarParameter | UMaterialExpressionScalarParameter | parameter_name, default_value |
| VectorParameter | UMaterialExpressionVectorParameter | parameter_name, default_value |
| TextureSample | UMaterialExpressionTextureSample | texture (asset path) |
| TextureParameter | UMaterialExpressionTextureObjectParameter | parameter_name, texture |
| Multiply | UMaterialExpressionMultiply | const_a, const_b |
| Add | UMaterialExpressionAdd | const_a, const_b |
| Subtract | UMaterialExpressionSubtract | const_a, const_b |
| Divide | UMaterialExpressionDivide | const_a, const_b |
| Lerp | UMaterialExpressionLinearInterpolate | const_alpha |
| Fresnel | UMaterialExpressionFresnel | exponent, base_reflect_fraction |
| Time | UMaterialExpressionTime | - |
| Panner | UMaterialExpressionPanner | speed_x, speed_y |
| TexCoord | UMaterialExpressionTextureCoordinate | coordinate_index, u_tiling, v_tiling |

### Connection Schema

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| from | String | Yes | Source expression id |
| from_output | String | No | Output pin name (default: first) |
| to | String | Yes | Target (expression id or material output) |
| to_input | String | No | Input pin name (default: first) |

### Material Output Names

| Output Name | Description |
|-------------|-------------|
| BaseColor | Surface color |
| Metallic | Metal vs non-metal |
| Specular | Specular intensity |
| Roughness | Surface roughness |
| EmissiveColor | Glowing/emitting light |
| Opacity | Transparency (for Translucent) |
| OpacityMask | Alpha mask (for Masked) |
| Normal | Normal map input |
| WorldPositionOffset | Vertex offset |

### Example: M_LaserGlow YAML

| Line | Content |
|------|---------|
| 1 | material_graph: |
| 2 | (2)blend_mode: Additive |
| 3 | (2)shading_model: Unlit |
| 4 | (2)expressions: |
| 5 | (4)- type: Constant3Vector |
| 6 | (6)id: BaseColor |
| 7 | (6)properties: |
| 8 | (8)constant: [1.0, 0.3, 0.1] |
| 9 | (6)position: [0, 0] |
| 10 | (4)- type: ScalarParameter |
| 11 | (6)id: EmissiveStrength |
| 12 | (6)properties: |
| 13 | (8)parameter_name: EmissiveStrength |
| 14 | (8)default_value: 10.0 |
| 15 | (6)position: [0, 200] |
| 16 | (4)- type: Multiply |
| 17 | (6)id: EmissiveMult |
| 18 | (6)position: [200, 100] |
| 19 | (4)- type: Time |
| 20 | (6)id: TimeNode |
| 21 | (6)position: [0, 400] |
| 22 | (4)- type: Panner |
| 23 | (6)id: UVPanner |
| 24 | (6)properties: |
| 25 | (8)speed_x: 0.5 |
| 26 | (8)speed_y: 0.0 |
| 27 | (6)position: [200, 400] |
| 28 | (2)connections: |
| 29 | (4)- from: BaseColor |
| 30 | (6)to: EmissiveMult |
| 31 | (6)to_input: A |
| 32 | (4)- from: EmissiveStrength |
| 33 | (6)to: EmissiveMult |
| 34 | (6)to_input: B |
| 35 | (4)- from: EmissiveMult |
| 36 | (6)to: EmissiveColor |
| 37 | (4)- from: BaseColor |
| 38 | (6)to: BaseColor |
| 39 | (4)- from: TimeNode |
| 40 | (6)to: UVPanner |
| 41 | (6)to_input: Time |

---

## NEW GENERATOR IMPLEMENTATION: FMontageConfigGenerator

### Purpose

Generates Animation Montage notifies, sections, and blend settings using FAnimNotifyEvent::Link() API.

### Key Discovery

Adding notifies to montages requires the critical FAnimNotifyEvent::Link() call to establish trigger timing within the animation timeline. Without this call, notifies appear but never trigger.

### Required Headers

| Header | Contents |
|--------|----------|
| Animation/AnimMontage.h | UAnimMontage |
| Animation/AnimNotifies/AnimNotify.h | UAnimNotify |
| Animation/AnimNotifies/AnimNotifyState.h | UAnimNotifyState |
| Animation/AnimTypes.h | FAnimNotifyEvent |

### Notify Creation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | int32 Index = Montage->Notifies.Add(FAnimNotifyEvent()) | Add notify to array |
| 2 | FAnimNotifyEvent& Event = Montage->Notifies[Index] | Get reference |
| 3 | Event.NotifyName = FName("DamageWindow") | Set notify name |
| 4 | Event.Guid = FGuid::NewGuid() | REQUIRED for editor stability |
| 5 | Event.Link(Montage, TriggerTimeInSeconds) | CRITICAL - Sets timing |
| 6 | Event.Notify = NewObject<UAnimNotify>(Montage) | Create notify object (optional) |

### Notify State Creation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1-5 | (Same as above) | Basic setup |
| 6 | Event.NotifyStateClass = UAnimNotifyState::StaticClass() | Set state class |
| 7 | Event.SetDuration(DurationInSeconds) | Set duration |
| 8 | Event.EndLink.Link(Montage, Event.GetTime() + Duration) | Link end time |

### Section Creation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | FCompositeSection Section | Create section struct |
| 2 | Section.SectionName = FName("Attack1") | Set section name |
| 3 | Section.StartTime = 0.f | Set start time |
| 4 | Section.NextSectionName = FName("Attack2") | Set branch target |
| 5 | Montage->CompositeSections.Add(Section) | Add to montage |

### Slot Track Configuration

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | FSlotAnimationTrack SlotTrack | Create slot track |
| 2 | SlotTrack.SlotName = FName("DefaultSlot") | Set slot name |
| 3 | FAnimSegment Segment | Create segment |
| 4 | Segment.AnimReference = AnimSequence | Set animation reference |
| 5 | Segment.AnimStartTime = 0.f | Set start time |
| 6 | Segment.AnimEndTime = AnimSequence->GetPlayLength() | Set end time |
| 7 | SlotTrack.AnimTrack.AnimSegments.Add(Segment) | Add segment to track |
| 8 | Montage->SlotAnimTracks.Add(SlotTrack) | Add track to montage |

### Blend Settings

| Property | API | Options |
|----------|-----|---------|
| Blend In Time | Montage->BlendIn.SetBlendTime(0.25f) | Float seconds |
| Blend Out Time | Montage->BlendOut.SetBlendTime(0.25f) | Float seconds |
| Blend In Option | Montage->BlendIn.SetBlendOption(EAlphaBlendOption::Linear) | Linear, Cubic, Sinusoidal |
| Blend Out Option | Montage->BlendOut.SetBlendOption(EAlphaBlendOption::Linear) | Linear, Cubic, Sinusoidal |

---

## YAML SCHEMA: montage_config (FULL SPECIFICATION)

### Schema Definition

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| montage_config | Object | Yes | Root montage configuration |
| montage_config.slot_name | String | Yes | Animation slot name |
| montage_config.blend_in | Float | No | Blend in time (default 0.25) |
| montage_config.blend_out | Float | No | Blend out time (default 0.25) |
| montage_config.sections | Array | No | Montage sections |
| montage_config.notifies | Array | No | Animation notifies |

### Section Schema

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| name | String | Yes | Section name |
| start_time | Float | Yes | Start time in seconds |
| next_section | String | No | Branch target section name |

### Notify Schema

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| name | String | Yes | Notify name |
| time | Float | Yes | Trigger time in seconds |
| class | String | No | Custom notify class |
| duration | Float | For States | Duration in seconds |
| is_state | Bool | No | true for notify state |

### Example: AM_FatherAttack YAML

| Line | Content |
|------|---------|
| 1 | montage_config: |
| 2 | (2)slot_name: DefaultSlot |
| 3 | (2)blend_in: 0.1 |
| 4 | (2)blend_out: 0.2 |
| 5 | (2)sections: |
| 6 | (4)- name: Attack1 |
| 7 | (6)start_time: 0.0 |
| 8 | (6)next_section: Attack2 |
| 9 | (4)- name: Attack2 |
| 10 | (6)start_time: 0.5 |
| 11 | (6)next_section: Attack3 |
| 12 | (4)- name: Attack3 |
| 13 | (6)start_time: 1.0 |
| 14 | (2)notifies: |
| 15 | (4)- name: DamageWindow |
| 16 | (6)time: 0.2 |
| 17 | (6)is_state: true |
| 18 | (6)duration: 0.15 |
| 19 | (4)- name: DamageWindow |
| 20 | (6)time: 0.7 |
| 21 | (6)is_state: true |
| 22 | (6)duration: 0.15 |
| 23 | (4)- name: ComboReset |
| 24 | (6)time: 1.4 |

---

## NEW GENERATOR IMPLEMENTATION: FGameplayCueGenerator

### Purpose

Generates Gameplay Cue assets (GC_) using appropriate base class based on effect type.

### Base Class Selection

| Effect Type | Base Class | Use Case |
|-------------|------------|----------|
| Burst | UGameplayCueNotify_Static | Hit impacts, instant VFX, non-spawning effects |
| Persistent | AGameplayCueNotify_Actor | Buffs, shields, DOTs, effects needing tick |

### Static Cue Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | class UGC_HitImpact : public UGameplayCueNotify_Static | Create class |
| 2 | GameplayCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Combat.HitImpact")) | Set tag in constructor |
| 3 | virtual bool HandleGameplayCue(...) override | Override handler |
| 4 | if (EventType == EGameplayCueEvent::Executed) SpawnEffect() | Handle event |

### Actor Cue Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | class AGC_ShieldEffect : public AGameplayCueNotify_Actor | Create class |
| 2 | virtual bool OnActive_Implementation(...) override | Handle activation |
| 3 | virtual bool WhileActive_Implementation(...) override | Handle active state |
| 4 | virtual bool OnRemove_Implementation(...) override | Handle removal |

### Tag Requirement

| Requirement | Pattern |
|-------------|---------|
| Tag prefix | GameplayCue.* (required hierarchy) |
| Asset naming | GC_ prefix (e.g., GC_HitImpact) |

---

## YAML SCHEMA: gameplay_cue (FULL SPECIFICATION)

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| gameplay_cue | Object | Yes | Root cue definition |
| gameplay_cue.name | String | Yes | GC_AssetName |
| gameplay_cue.type | String | Yes | Static or Actor |
| gameplay_cue.tag | String | Yes | GameplayCue.Category.Name |
| gameplay_cue.particle_system | String | No | Niagara/Cascade asset path |
| gameplay_cue.sound_cue | String | No | Sound cue asset path |
| gameplay_cue.camera_shake | String | No | Camera shake class/asset |

---

## NEW GENERATOR IMPLEMENTATION: FCameraShakeGenerator

### Purpose

Generates Camera Shake assets using the modern pattern-based architecture in UE5.

### Available Pattern Classes

| Pattern Class | Description |
|---------------|-------------|
| UPerlinNoiseCameraShakePattern | Perlin noise oscillation |
| UWaveOscillatorCameraShakePattern | Wave-based oscillation |
| UCompositeCameraShakePattern | Multiple patterns combined |
| USequenceCameraShakePattern | Sequential patterns |

### Perlin Noise Pattern Configuration

| Property | Subproperty | Type |
|----------|-------------|------|
| X | Amplitude, Frequency | float |
| Y | Amplitude, Frequency | float |
| Z | Amplitude, Frequency | float |
| Pitch | Amplitude, Frequency | float |
| Yaw | Amplitude, Frequency | float |
| Roll | Amplitude, Frequency | float |
| FOV | Amplitude, Frequency | float |
| Duration | - | float |
| BlendInTime | - | float |
| BlendOutTime | - | float |

### Creation Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | UMyCameraShake::UMyCameraShake() : UCameraShakeBase(FObjectInitializer::Get()) | Constructor |
| 2 | UPerlinNoiseCameraShakePattern* Pattern = CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(TEXT("Pattern")) | Create pattern |
| 3 | Pattern->X.Amplitude = 10.f | Set X amplitude |
| 4 | Pattern->Duration = 2.f | Set duration |
| 5 | SetRootShakePattern(Pattern) | Assign pattern |

---

## YAML SCHEMA: camera_shake (FULL SPECIFICATION)

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| camera_shake | Object | Yes | Root shake definition |
| camera_shake.name | String | Yes | CS_AssetName |
| camera_shake.pattern_type | String | Yes | PerlinNoise, WaveOscillator |
| camera_shake.duration | Float | Yes | Shake duration |
| camera_shake.blend_in | Float | No | Blend in time |
| camera_shake.blend_out | Float | No | Blend out time |
| camera_shake.location | Object | No | Location oscillation |
| camera_shake.rotation | Object | No | Rotation oscillation |
| camera_shake.fov | Object | No | FOV oscillation |

### Oscillation Properties

| Field | Type | Description |
|-------|------|-------------|
| x/y/z (location) | Object | amplitude, frequency |
| pitch/yaw/roll (rotation) | Object | amplitude, frequency |
| fov | Object | amplitude, frequency |

### Example: CS_HitImpact YAML

| Line | Content |
|------|---------|
| 1 | camera_shake: |
| 2 | (2)name: CS_HitImpact |
| 3 | (2)pattern_type: PerlinNoise |
| 4 | (2)duration: 0.3 |
| 5 | (2)blend_in: 0.05 |
| 6 | (2)blend_out: 0.1 |
| 7 | (2)location: |
| 8 | (4)x: |
| 9 | (6)amplitude: 5.0 |
| 10 | (6)frequency: 25.0 |
| 11 | (4)y: |
| 12 | (6)amplitude: 5.0 |
| 13 | (6)frequency: 25.0 |
| 14 | (2)rotation: |
| 15 | (4)pitch: |
| 16 | (6)amplitude: 3.0 |
| 17 | (6)frequency: 15.0 |

---

## NEW GENERATOR IMPLEMENTATION: FAIPerceptionGenerator

### Purpose

Configures AI Perception components with sense configurations using FAISenseConfig subclasses.

### Available Sense Configs

| Sense Config Class | Sense Type |
|--------------------|------------|
| UAISenseConfig_Sight | Visual perception |
| UAISenseConfig_Hearing | Audio perception |
| UAISenseConfig_Damage | Damage perception |
| UAISenseConfig_Prediction | Movement prediction |
| UAISenseConfig_Team | Team awareness |

### Sight Configuration Properties

| Property | Type | Description |
|----------|------|-------------|
| SightRadius | float | Detection range |
| LoseSightRadius | float | Range to lose target |
| PeripheralVisionAngleDegrees | float | Field of view angle |
| DetectionByAffiliation.bDetectEnemies | bool | Detect enemies |
| DetectionByAffiliation.bDetectNeutrals | bool | Detect neutrals |
| DetectionByAffiliation.bDetectFriendlies | bool | Detect friendlies |
| MaxAge | float | Stimulus memory duration |

### Hearing Configuration Properties

| Property | Type | Description |
|----------|------|-------------|
| HearingRange | float | Detection range |
| LoSHearingRange | float | Line of sight range |
| bUseLoSHearing | bool | Enable LoS hearing |
| MaxAge | float | Stimulus memory duration |

### Configuration Pattern

| Step | Code Pattern | Purpose |
|------|--------------|---------|
| 1 | PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception")) | Create component |
| 2 | UAISenseConfig_Sight* Sight = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig")) | Create sight config |
| 3 | Sight->SightRadius = 1000.f | Set properties |
| 4 | PerceptionComponent->ConfigureSense(*Sight) | Register sense |
| 5 | PerceptionComponent->SetDominantSense(Sight->GetSenseImplementation()) | Set dominant |

---

## YAML SCHEMA: ai_perception (FULL SPECIFICATION)

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| ai_perception | Object | Yes | Root perception definition |
| ai_perception.senses | Array | Yes | Sense configurations |
| ai_perception.dominant_sense | String | No | Primary sense type |

### Sense Schema

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| type | String | Yes | Sight, Hearing, Damage, Prediction, Team |
| properties | Object | Yes | Sense-specific properties |

### Sight Properties

| Property | Type | Default |
|----------|------|---------|
| sight_radius | Float | 1000.0 |
| lose_sight_radius | Float | 1200.0 |
| peripheral_vision_angle | Float | 90.0 |
| detect_enemies | Bool | true |
| detect_neutrals | Bool | false |
| detect_friendlies | Bool | false |
| max_age | Float | 5.0 |

### Hearing Properties

| Property | Type | Default |
|----------|------|---------|
| hearing_range | Float | 2000.0 |
| detect_enemies | Bool | true |
| max_age | Float | 5.0 |

### Example: BP_FatherCompanion AI Perception YAML

| Line | Content |
|------|---------|
| 1 | ai_perception: |
| 2 | (2)dominant_sense: Sight |
| 3 | (2)senses: |
| 4 | (4)- type: Sight |
| 5 | (6)properties: |
| 6 | (8)sight_radius: 2000.0 |
| 7 | (8)lose_sight_radius: 2500.0 |
| 8 | (8)peripheral_vision_angle: 90.0 |
| 9 | (8)detect_enemies: true |
| 10 | (8)detect_neutrals: false |
| 11 | (8)detect_friendlies: false |
| 12 | (8)max_age: 5.0 |
| 13 | (4)- type: Damage |
| 14 | (6)properties: |
| 15 | (8)max_age: 5.0 |

---

## GENERATOR CLASS HIERARCHY (v7.8.0)

| Class | Base | Purpose | Assets |
|-------|------|---------|--------|
| FAssetGeneratorBase | - | Base class for all generators | - |
| FTagGenerator | Base | Gameplay tags in INI file | 174 tags |
| FInputAssetGenerator | Base | Input actions and mapping contexts | 8 input assets |
| FGameplayEffectGenerator | Base | Gameplay effects with components | 34 GE_ assets |
| FGameplayAbilityGenerator | Base | Abilities with Class Defaults + Event Graph | 18 GA_ assets |
| FActorBlueprintGenerator | Base | Actor blueprints with components + Event Graph | 4 BP_ actors |
| FWidgetBlueprintGenerator | Base | Widget blueprints with tree + Event Graph | 3 WBP_ widgets |
| FBehaviorTreeGenerator | Base | Behavior trees with full node hierarchy | 2 BT_ assets |
| FMaterialGenerator | Base | Materials with expression graphs | 1 M_ material |
| FMontageConfigGenerator | Base | Montage notifies and sections | 7 AM_/NAS_ assets |
| FConfigurationGenerator | Base | AbilityConfig, ActivityConfig, NPCDef | 4 configs |
| FDataAssetGenerator | Base | Generic data asset population | Various |
| FEventGraphGenerator | NEW v7.8.0 | Blueprint event graph logic | All BP_ assets |
| FMaterialGraphGenerator | NEW v7.8.0 | Material expression nodes | M_ assets |
| FWidgetTreeGenerator | NEW v7.8.0 | Widget visual hierarchy | WBP_ assets |
| FGameplayCueGenerator | NEW v7.8.0 | Gameplay cue assets | GC_ assets |
| FCameraShakeGenerator | NEW v7.8.0 | Camera shake patterns | CS_ assets |
| FAIPerceptionGenerator | NEW v7.8.0 | AI perception configuration | BP_ with AI |

---

## IMPLEMENTATION PHASES (v7.8.0)

### Phase B: YAML Schema Extension (17 hours)

| Task | Description | Time |
|------|-------------|------|
| B.1 | Implement event_graph schema parser | 3 hours |
| B.2 | Implement behavior_tree_graph schema parser | 2 hours |
| B.3 | Implement widget_tree schema parser | 2 hours |
| B.4 | Implement material_graph schema parser | 2 hours |
| B.5 | Implement montage_config schema parser | 1 hour |
| B.6 | Implement gameplay_cue schema parser | 1 hour |
| B.7 | Implement camera_shake schema parser | 1 hour |
| B.8 | Implement ai_perception schema parser | 1 hour |
| B.9 | Add schema validation for all types | 4 hours |

### Phase C: Generator Implementation (34 hours)

| Task | Description | Time |
|------|-------------|------|
| C.1 | Implement FEventGraphGenerator class | 10 hours |
| C.2 | Implement FBehaviorTreeGenerator class | 6 hours |
| C.3 | Implement FWidgetTreeGenerator class | 6 hours |
| C.4 | Implement FMaterialGraphGenerator class | 4 hours |
| C.5 | Implement FMontageConfigGenerator class | 3 hours |
| C.6 | Implement FGameplayCueGenerator class | 2 hours |
| C.7 | Implement FCameraShakeGenerator class | 1 hour |
| C.8 | Add node position auto-layout algorithm | 2 hours |

### Phase D: Integration Testing (11 hours)

| Task | Description | Time |
|------|-------------|------|
| D.1 | Test GA_FatherArmor full generation | 2 hours |
| D.2 | Test BT_FatherFollow full generation | 2 hours |
| D.3 | Test WBP_MarkIndicator full generation | 2 hours |
| D.4 | Test M_LaserGlow full generation | 1 hour |
| D.5 | Test AM_FatherAttack config generation | 1 hour |
| D.6 | Test full regeneration cycle | 2 hours |
| D.7 | Performance testing | 1 hour |

### Total Implementation Estimate

| Phase | Hours |
|-------|-------|
| Phase B (Schema) | 17 hours |
| Phase C (Generators) | 34 hours |
| Phase D (Testing) | 11 hours |
| TOTAL | 62 hours |


## COMPLETE ASSET INVENTORY

### 1. Gameplay Tags

| Category | Count | Example Tags |
|----------|-------|--------------|
| Ability.Father.* | 25 | Ability.Father.Armor, Ability.Father.Crawler |
| Cooldown.Father.* | 10 | Cooldown.Father.FormChange, Cooldown.Father.DomeBurst |
| Damage.Father.* | 7 | Damage.Father.Base, Damage.Father.LaserShot |
| Effect.Father.* | 24 | Effect.Father.ArmorBoost, Effect.Father.FormState.* |
| Narrative.Equipment.Slot.FatherForm | 1 | Narrative.Equipment.Slot.FatherForm |
| Narrative.Factions.* | 2 | Narrative.Factions.Heroes, Narrative.Factions.Companions |
| Narrative.Input.Companion.* | 5 | Narrative.Input.Companion.FormWheel |
| Narrative.Input.Father.* | 6 | Narrative.Input.Father.Ability1, Narrative.Input.Father.Attack |
| Narrative.TaggedDialogue.Father.* | 6 | Narrative.TaggedDialogue.Father.Recruit |
| Player.State.Stealth | 1 | Player.State.Stealth |
| Father.Dome.* | 4 | Father.Dome.Active, Father.Dome.Charging |
| Father.Form.* | 6 | Father.Form.Crawler, Father.Form.Armor |
| Father.State.* | 19 | Father.State.Attached, Father.State.Recruited |
| State.* | 3 | State.Invulnerable, State.Father.Transitioning |
| Status.Immune.* | 2 | Status.Immune.Damage, Status.Immune.Knockback |
| Symbiote.Charge.Ready | 1 | Symbiote.Charge.Ready |
| TOTAL | 174 | |

---

### 2. Enumerations (E_)

| Asset | Values | Location | Auto-Gen |
|-------|--------|----------|----------|
| E_FatherForm | Crawler, Armor, Exoskeleton, Symbiote, Engineer | /Content/FatherCompanion/Data/ | FULL |
| EFatherUltimate | Symbiote, Sword, Rifle, Bow | /Content/FatherCompanion/Data/ | FULL |
| TOTAL | 2 | | |

---

### 3. Actor Blueprints (BP_)

| Asset | Parent Class | Purpose | Auto-Gen |
|-------|--------------|---------|----------|
| BP_FatherCompanion | NarrativeNPCCharacter | Main father character | STRUCTURE |
| BP_LaserProjectile | NarrativeProjectile | Laser shot projectile | STRUCTURE |
| BP_TurretProjectile | NarrativeProjectile | Turret energy projectile | STRUCTURE |
| BP_FatherElectricTrap | Actor | Electric trap actor | STRUCTURE |
| TOTAL | 4 | | |

---

### 4. Gameplay Abilities (GA_)

#### 4.1) Form Abilities (Baseline - AC_FatherCompanion_Default)

| Asset | Parent Class | Purpose | Auto-Gen |
|-------|--------------|---------|----------|
| GA_FatherCrawler | NarrativeGameplayAbility | Default following mode | FULL |
| GA_FatherArmor | NarrativeGameplayAbility | Chest attachment, +armor | FULL |
| GA_FatherExoskeleton | NarrativeGameplayAbility | Back attachment, +speed | FULL |
| GA_FatherSymbiote | NarrativeGameplayAbility | Full body merge, berserker | FULL |
| GA_FatherEngineer | NarrativeGameplayAbility | Turret deployment | FULL |
| Subtotal | 5 | | |

#### 4.2) Combat/AI Abilities (Baseline - AC_FatherCompanion_Default)

| Asset | Parent Class | Purpose | Auto-Gen |
|-------|--------------|---------|----------|
| GA_FatherAttack | NarrativeCombatAbility | Melee attack | FULL |
| GA_FatherLaserShot | NarrativeCombatAbility | Ranged attack | FULL |
| GA_FatherMark | NarrativeGameplayAbility | Enemy marking | FULL |
| GA_FatherSacrifice | NarrativeGameplayAbility | Emergency player save | FULL |
| Subtotal | 4 | | |

#### 4.3) Form-Specific Action Abilities (via EquippableItem)

| Asset | Form | Parent Class | Auto-Gen |
|-------|------|--------------|----------|
| GA_ProtectiveDome | Armor | NarrativeGameplayAbility | FULL |
| GA_DomeBurst | Armor | NarrativeGameplayAbility | FULL |
| GA_ExoskeletonDash | Exoskeleton | NarrativeGameplayAbility | FULL |
| GA_ExoskeletonSprint | Exoskeleton | NarrativeGameplayAbility | FULL |
| GA_StealthField | Exoskeleton | NarrativeGameplayAbility | FULL |
| GA_Backstab | Exoskeleton | NarrativeGameplayAbility | FULL |
| GA_ProximityStrike | Symbiote | NarrativeGameplayAbility | FULL |
| GA_TurretShoot | Engineer | NarrativeCombatAbility | FULL |
| GA_ElectricTrap | Engineer | NarrativeCombatAbility | FULL |
| Subtotal | 9 | | |

| GA_ TOTAL | 18 | | |

---

### 5. Gameplay Effects (GE_)

#### 5.1) Form State Effects

| Asset | Duration | Purpose | Auto-Gen |
|-------|----------|---------|----------|
| GE_FormChangeCooldown | 15 seconds | Shared form cooldown | FULL |
| GE_Invulnerable | 5 seconds | Transition protection | USES BUILT-IN |
| GE_GrantRecruitedTag | Infinite | Grants Father.State.Recruited | FULL |
| Subtotal | 2 custom | | |

#### 5.2) Equipment Modifier Effects (Child GEs)

| Asset | Parent | Form | Modifiers | Auto-Gen |
|-------|--------|------|-----------|----------|
| GE_EquipmentModifier_FatherArmor | GE_EquipmentModifier | Armor | +50 Armor Rating | FULL |
| GE_EquipmentModifier_FatherExoskeleton | GE_EquipmentModifier | Exoskeleton | +10 Attack Rating | FULL |
| GE_EquipmentModifier_FatherSymbiote | GE_EquipmentModifier | Symbiote | +100 Attack, Infinite Stamina | FULL |
| GE_EquipmentModifier_FatherEngineer | GE_EquipmentModifier | Engineer | None | FULL |
| Subtotal | 4 | | | |

#### 5.3) Dome System Effects

| Asset | Duration | Purpose | Auto-Gen |
|-------|----------|---------|----------|
| GE_DomeAbsorption | Infinite | 30% damage absorption | FULL |
| GE_DomeEnergyIncrease | Instant | SetByCaller energy add | FULL |
| GE_DomeInitialize | Instant | Initial attribute values | FULL |
| GE_DomeBurstDamage | Instant | SetByCaller AOE damage | FULL |
| GE_DomeBurstCooldown | 20 seconds | Cooldown tag | FULL |
| GE_DomeEnergyReset | Instant | Reset energy to 0 | FULL |
| Subtotal | 6 | | |

#### 5.4) Combat Damage Effects

| Asset | Duration | Purpose | Auto-Gen |
|-------|----------|---------|----------|
| GE_FatherAttackDamage | Instant | Melee damage | FULL |
| GE_LaserDamage | Instant | Laser shot damage | FULL |
| GE_FatherMark | 5 seconds | Enemy mark tag | FULL |
| GE_MarkDamageBonus | 5 seconds | +25% damage to marked | FULL |
| GE_ProximityDamage | Instant | AOE damage | FULL |
| GE_TurretDamage | Instant | Turret projectile damage | FULL |
| GE_ElectricTrapDamage | Instant | Trap damage + slow | FULL |
| GE_ExoskeletonDashDamage | Instant | Dash damage | FULL |
| Subtotal | 8 | | |

#### 5.5) Cooldown Effects

| Asset | Duration | Purpose | Auto-Gen |
|-------|----------|---------|----------|
| GE_ExoskeletonDashCooldown | 3 seconds | Dash cooldown | FULL |
| GE_LaserCooldown | 2 seconds | Laser shot cooldown | FULL |
| GE_TurretShootCooldown | 0.5 seconds | Turret fire rate | FULL |
| GE_StealthCooldown | 15 seconds | Stealth ability cooldown | FULL |
| GE_ElectricTrapCooldown | 8 seconds | Trap ability cooldown | FULL |
| Subtotal | 5 | | |

#### 5.6) Sprint/Stealth Effects

| Asset | Duration | Purpose | Auto-Gen |
|-------|----------|---------|----------|
| GE_SprintSpeed | Infinite | +100% speed during sprint | FULL |
| GE_SprintJump | Infinite | +50% jump during sprint | FULL |
| GE_SprintPush | Instant | Enemy knockback | FULL |
| GE_StealthActive | Infinite | Invisibility tag | FULL |
| GE_StealthDamageBonus | Instant | +200% backstab damage | FULL |
| GE_StealthSpeedReduction | Infinite | -30% speed while stealth | FULL |
| Subtotal | 6 | | |

#### 5.7) Symbiote Effects

| Asset | Duration | Purpose | Auto-Gen |
|-------|----------|---------|----------|
| GE_SymbioteLock | 30 seconds | Prevents re-entry | FULL |
| Subtotal | 1 | | |

#### 5.8) Sacrifice Effects

| Asset | Duration | Purpose | Auto-Gen |
|-------|----------|---------|----------|
| GE_SacrificeInvulnerability | 8 seconds | Player invulnerability | FULL |
| GE_FatherOffline | Infinite | Father dormant state tags | FULL |
| Subtotal | 2 | | |

| GE_ TOTAL | 34 custom | | |

---

### 6. EquippableItems (BP_)

| Asset | Parent Class | Form | Abilities Granted | Auto-Gen |
|-------|--------------|------|-------------------|----------|
| BP_FatherArmorForm | EquippableItem | Armor | GA_ProtectiveDome, GA_DomeBurst | FULL |
| BP_FatherExoskeletonForm | EquippableItem | Exoskeleton | GA_FatherExoskeletonDash, GA_FatherExoskeletonSprint, GA_StealthField | FULL |
| BP_FatherSymbioteForm | EquippableItem | Symbiote | GA_ProximityStrike | FULL |
| BP_FatherEngineerForm | EquippableItem | Engineer | GA_TurretShoot, GA_FatherElectricTrap | FULL |
| TOTAL | 4 | | | |

---

### 7. Item Collections (IC_)

| Asset | Contents | Purpose | Auto-Gen |
|-------|----------|---------|----------|
| IC_FatherForms | 4 EquippableItems | Default Item Loadout | FULL |
| TOTAL | 1 | | |

---

### 8. Configuration Assets

| Asset | Type | Purpose | Auto-Gen |
|-------|------|---------|----------|
| AC_FatherCompanion_Default | AbilityConfiguration | 9 custom + 1 built-in abilities | FULL |
| ActConfig_FatherCompanion | ActivityConfiguration | BPA_FatherFollow, BPA_Attack_Melee, BPA_Idle | FULL |
| NPCDef_FatherCompanion | NPCDefinition | Character class, configs, loadout | FULL |
| CD_FatherCompanion | CharacterDefinition | Save/Load persistence | FULL |
| TOTAL | 4 | | |

---

### 9. AI Assets

#### 9.1) Behavior Trees (BT_)

| Asset | Blackboard | Purpose | Auto-Gen |
|-------|------------|---------|----------|
| BT_FatherFollow | BB_FollowCharacter | Custom follow (no AI focus stare) | FULL |
| BT_FatherEngineer | BB_FatherEngineer | Engineer turret combat | FULL |
| Subtotal | 2 | | |

#### 9.2) Blackboards (BB_)

| Asset | Parent | Purpose | Auto-Gen |
|-------|--------|---------|----------|
| BB_FatherEngineer | BB_Attack | Engineer turret targeting | FULL |
| Subtotal | 1 | | |

#### 9.3) Activities (BPA_)

| Asset | Parent | Behavior Tree | Auto-Gen |
|-------|--------|---------------|----------|
| BPA_FatherFollow | BPA_FollowCharacter | BT_FatherFollow | FULL |
| Subtotal | 1 | | |

#### 9.4) Built-in Assets (NO GENERATION NEEDED)

| Asset | Type | Source |
|-------|------|--------|
| BB_FollowCharacter | Blackboard | Narrative Pro built-in |
| BB_Attack | Blackboard | Narrative Pro built-in |
| BPA_Attack_Melee | Activity | Narrative Pro built-in |
| BPA_Idle | Activity | Narrative Pro built-in |
| GoalGenerator_Attack | Goal Generator | Narrative Pro built-in |
| BTTask_ActivateAbilityByClass | BT Task | Narrative Pro built-in |
| GA_Death | GameplayAbility | Narrative Pro built-in |

| AI TOTAL (custom) | 4 | | |

---

### 10. Widget Blueprints (WBP_)

| Asset | Parent Class | Purpose | Auto-Gen |
|-------|--------------|---------|----------|
| WBP_MarkIndicator | UserWidget | Enemy mark display | FULL |
| WBP_UltimatePanel | UserWidget | Ultimate selection UI | FULL |
| WBP_UltimateIcon | UserWidget | Individual ultimate icon | FULL |
| TOTAL | 3 | | |

---

### 11. Input Assets

| Asset | Type | Purpose | Auto-Gen |
|-------|------|---------|----------|
| IA_FatherFormWheel | InputAction | Form selection wheel | FULL |
| IA_FatherAbility1 | InputAction | Primary ability | FULL |
| IA_FatherAbility2 | InputAction | Secondary ability | FULL |
| IA_FatherAbility3 | InputAction | Tertiary ability | FULL |
| IA_FatherAttack | InputAction | Father attack command | FULL |
| IA_FatherRecall | InputAction | Recall to Crawler | FULL |
| IMC_FatherCompanion | InputMappingContext | Key bindings | FULL |
| TOTAL | 7 | | |

---

### 12. Curve Assets

| Asset | Type | Purpose | Auto-Gen |
|-------|------|---------|----------|
| FC_UltimateThreshold | FloatCurve | Ultimate charge thresholds | FULL |
| TOTAL | 1 | | |

---

### 13. Narrative Events (NE_)

| Asset | Type | Purpose | Auto-Gen |
|-------|------|---------|----------|
| NE_SetFatherOwner | NarrativeEvent | Set father owner reference | FULL |
| NE_ClearFatherOwner | NarrativeEvent | Clear father owner reference | FULL |
| TOTAL | 2 | | |

---

### 14. Player Components (UActorComponent)

| Asset | Type | Purpose | Auto-Gen |
|-------|------|---------|----------|
| SymbioteChargeComponent | ActorComponent | Track damage for ultimate | FULL |
| TOTAL | 1 | | |

---

### 15. Attribute Sets

| Asset | Type | Purpose | Auto-Gen |
|-------|------|---------|----------|
| AS_DomeAttributes | NarrativeAttributeSetBase | DomeEnergy, MaxDomeEnergy | REQUIRES GBP Plugin |
| TOTAL | 1 | | |

---

### 16. Dialogue Assets

| Asset | Type | Purpose | Auto-Gen |
|-------|------|---------|----------|
| DBP_FatherCompanion | DialogueBlueprint | Father recruitment dialogue | STRUCTURE |
| TOTAL | 1 | | |

---

### 17. Animation Assets

| Asset | Type | Purpose | Auto-Gen |
|-------|------|---------|----------|
| NAS_FatherAttack | NarrativeAnimSet | Attack combo animation set | PARTIAL |
| AM_FatherAttack | AnimMontage | Attack animation | PARTIAL |
| AM_FatherThrowWeb | AnimMontage | Web throw animation | PARTIAL |
| AM_FatherSacrifice | AnimMontage | Sacrifice animation | PARTIAL |
| AM_FatherReactivate | AnimMontage | Reactivation animation | PARTIAL |
| AM_FatherDetach | AnimMontage | Detach transition animation | PARTIAL |
| TOTAL | 6 | | |

Note: PARTIAL = Notifies, sections, blend settings generatable; source animation sequences required externally.

---

### 18. Material Assets

| Asset | Type | Purpose | Auto-Gen |
|-------|------|---------|----------|
| M_LaserGlow | Material | Laser projectile glow | STRUCTURE |
| TOTAL | 1 | | |

---

## ASSET SUMMARY BY TYPE

| Type | Prefix | Count | Auto-Gen Level |
|------|--------|-------|----------------|
| Gameplay Tags | - | 174 | FULL |
| Input Actions | IA_ | 7 | FULL |
| Input Mapping Context | IMC_ | 1 | FULL |
| Enumerations | E_ | 2 | FULL |
| Float Curves | FC_ | 3 | FULL |
| Gameplay Effects | GE_ | 34 | FULL |
| Gameplay Abilities | GA_ | 18 | FULL |
| EquippableItems | EI_ | 4 | FULL |
| Blackboards | BB_ | 2 | FULL |
| Behavior Trees | BT_ | 2 | FULL |
| Activities | BPA_ | 1 | FULL |
| Ability Configurations | AC_ | 1 | FULL |
| Activity Configurations | ActConfig_ | 1 | FULL |
| NPC Definitions | NPCDef_ | 1 | FULL |
| Character Definitions | CD_ | 1 | FULL |
| Item Collections | IC_ | 1 | FULL |
| Narrative Events | NE_ | 2 | FULL |
| Actor Blueprints | BP_ | 4 | FULL |
| Widgets | WBP_ | 3 | FULL |
| Animation Montages | AM_ | 6 | PARTIAL |
| Animation Notifies | NAS_ | 1 | PARTIAL |
| Materials | M_ | 1 | FULL |
| Dialogue | DBP_ | 1 | PARTIAL |
| NON-TAG SUBTOTAL | | 97 | |
| GRAND TOTAL (Tags + Assets) | | 271 | |

---

## AUTO-GENERATION LEVELS

| Level | Description | Manual Work Required |
|-------|-------------|----------------------|
| FULL | Complete working asset | None |
| PARTIAL | Most features auto-generated | External dependencies (animations) |
| STRUCTURE | Blueprint with variables/components | Event graph logic |
| USES BUILT-IN | Reference existing NP asset | None |
| REQUIRES PLUGIN | Needs external plugin | Gameplay Blueprint Attributes |

### Assets by Generation Level (v7.8.2)

| Level | Count | Examples |
|-------|-------|----------|
| FULL | 262 | GA_*, GE_*, BP_*, WBP_*, BT_*, BB_*, M_*, EquippableItems, Configs, Tags, Input |
| PARTIAL | 8 | AM_* (6), NAS_* (1), DBP_* (1) - notifies/sections generatable, external deps required |
| USES BUILT-IN | 8 | GE_Invulnerable, BPA_Attack_Melee, GA_Death, BB_FollowCharacter |

### Generation Level Progression

| Version | FULL | PARTIAL | STRUCTURE | Notes |
|---------|------|---------|-----------|-------|
| v7.5.5 (Original) | 248 | 0 | 18 | Before Event Graph research |
| v7.6.0 (Event Graph) | 270 | 0 | 10 | After Event Graph APIs discovered |
| v7.7.0 (Expanded) | 259 | 7 | 0 | After BT, Widget, Montage APIs discovered |
| v7.8.2 (Current) | 262 | 8 | 0 | Final count verification, manifest sync |


### Upgrade Path Summary (v7.7.0)

| Asset Type | v7.5.5 Status | v7.7.0 Status | Solution Found |
|------------|---------------|---------------|----------------|
| Gameplay Abilities (GA_*) | STRUCTURE | FULL | UK2Node Event Graph APIs |
| Actor Blueprints (BP_*) | STRUCTURE | FULL | UK2Node Event Graph APIs |
| Materials (M_*) | STRUCTURE | FULL | UMaterialExpression APIs |
| Widget Blueprints (WBP_*) | STRUCTURE | FULL | WidgetTree::ConstructWidget |
| Behavior Trees (BT_*) | STRUCTURE | FULL | FBTCompositeChild arrays |
| Player Components | STRUCTURE | FULL | UK2Node Event Graph APIs |
| Animation Montages (AM_*) | STRUCTURE | PARTIAL | FAnimNotifyEvent::Link() |
| Dialogue Blueprints (DBP_*) | STRUCTURE | PARTIAL | Custom UEdGraphNode |

---

## IMPLEMENTATION PHASES

### Phase 1: Foundation (Easy) - 185 assets

| Asset Type | Count | Complexity | Notes |
|------------|-------|------------|-------|
| Gameplay Tags | 174 | Very Easy | Write to INI file |
| Input Actions | 7 | Easy | Value type property only |
| Input Mapping Context | 1 | Easy | Key bindings list |
| Float Curves | 3 | Easy | Key points only |

---

### Phase 2: Data Layer (Easy) - ~9 assets

| Asset Type | Count | Complexity | Notes |
|------------|-------|------------|-------|
| Enumerations | 2 | Easy | Values list only |
| Item Collections | 1 | Easy | Item references |
| Narrative Events | 2 | Easy | Event properties |
| Configurations (AC_) | 1 | Easy | Ability references |
| Configurations (ActConfig_) | 1 | Medium | Activity references |
| Configurations (NPCDef_) | 1 | Medium | Full NPC setup |
| Configurations (CD_) | 1 | Easy | Character identity |

---

### Phase 3: Gameplay Effects (Medium) - 34 assets

| Asset Type | Count | Complexity | Notes |
|------------|-------|------------|-------|
| Gameplay Effects | 34 | Medium | Duration, modifiers, tags, components |

Sub-phases:

| Sub-Phase | Count | Description |
|-----------|-------|-------------|
| 3A | 7 | Duration/Cooldown effects |
| 3B | 8 | Damage effects with SetByCaller |
| 3C | 4 | Equipment Modifier child effects |
| 3D | 6 | Sprint/Stealth effects |
| 3E | 6 | Dome system effects |
| 3F | 3 | Symbiote/Sacrifice effects |

---

### Phase 4: EquippableItems (Medium) - 4 assets

| Asset Type | Count | Complexity | Notes |
|------------|-------|------------|-------|
| EquippableItems | 4 | Medium | Properties, abilities, equipment slot |

---

### Phase 5: Gameplay Abilities (Medium-Hard) - 18 assets

| Asset Type | Count | Complexity | Notes |
|------------|-------|------------|-------|
| Form Abilities | 5 | Medium | Class Defaults + basic graph |
| Combat Abilities | 4 | Medium-Hard | Full event graph with targeting |
| Action Abilities | 9 | Medium-Hard | Complex logic, timers, effects |

Sub-phases:

| Sub-Phase | Count | Description |
|-----------|-------|-------------|
| 5A | 5 | Form abilities (GA_FatherCrawler, etc.) |
| 5B | 4 | Combat abilities (GA_FatherAttack, GA_FatherLaserShot) |
| 5C | 9 | Action abilities (GA_ProtectiveDome, etc.) |

---

### Phase 6: AI Integration (Medium) - 5 assets

| Asset Type | Count | Complexity | Notes |
|------------|-------|------------|-------|
| Behavior Trees | 2 | Medium | Tree structure |
| Blackboards | 2 | Easy | Key definitions |
| Activities | 1 | Easy | Properties, BT reference |

---

### Phase 7: UI and Actors (Hard) - 7 assets

| Asset Type | Count | Complexity | Notes |
|------------|-------|------------|-------|
| Widget Blueprints | 3 | Hard | Structure + bindings |
| Actor Blueprints | 4 | Hard | Components + variables |

---

### Phase 8: Support Assets (PARTIAL) - 9 assets

| Asset Type | Count | Complexity | Notes |
|------------|-------|------------|-------|
| Animation Montages | 6 | PARTIAL | Placeholder montages |
| Animation Notifies | 1 | PARTIAL | Animation notify set |
| Material Assets | 1 | FULL | Basic material setup |
| Dialogue Assets | 1 | PARTIAL | Dialogue flow structure |

---

## PHASE SUMMARY

| Phase | Name | Assets | Effort | Priority |
|-------|------|--------|--------|----------|
| 1 | Foundation | 185 | Easy | HIGH |
| 2 | Data Layer | 9 | Easy | HIGH |
| 3 | Gameplay Effects | 34 | Medium | HIGH |
| 4 | EquippableItems | 4 | Medium | MEDIUM |
| 5 | Gameplay Abilities | 18 | Medium-Hard | HIGH |
| 6 | AI Integration | 5 | Medium | MEDIUM |
| 7 | UI and Actors | 7 | Hard | LOW |
| 8 | Support Assets | 9 | PARTIAL | LOW |
| **TOTAL** | | **271** | | |

---

## DEPENDENCIES

### Required Plugins

| Plugin | Purpose | Required For |
|--------|---------|--------------|
| Narrative Pro v2.2 | Core gameplay systems | All assets |
| Gameplay Blueprint Attributes | Attribute creation without C++ | AS_DomeAttributes |

### Build Dependencies

| Phase | Depends On |
|-------|------------|
| Phase 2 | Phase 1 (tags must exist) |
| Phase 3 | Phase 1 (tags for GE granted tags) |
| Phase 4 | Phase 3 (equipment modifier GEs), Phase 5A (abilities to grant) |
| Phase 5 | Phase 3 (cooldown GEs, damage GEs), Phase 1 (tags) |
| Phase 6 | Phase 5 (abilities for AI to activate) |
| Phase 7 | Phase 5 (abilities), Phase 3 (effects) |
| Phase 8 | Phase 5 (abilities reference animations) |

---

## RECOMMENDED BUILD ORDER

| Step | Phase | Assets |
|------|-------|--------|
| 1 | Phase 1 | Tags, Input Actions, IMC, Curves |
| 2 | Phase 3 | All Gameplay Effects |
| 3 | Phase 5A | Form Abilities (GA_FatherCrawler, etc.) |
| 4 | Phase 5B | Combat Abilities (GA_FatherAttack, GA_FatherLaserShot) |
| 5 | Phase 4 | EquippableItems |
| 6 | Phase 5C | Action Abilities (GA_ProtectiveDome, etc.) |
| 7 | Phase 2 | Configurations, NPCDefinition, CharacterDefinition |
| 8 | Phase 6 | AI (BT_FatherFollow, BT_FatherEngineer, BPA_FatherFollow) |
| 9 | Phase 7 | Widgets, Actor Blueprints (structure only) |
| 10 | Phase 8 | Animation, Material, Dialogue assets (structure only) |

---

## YAML SCHEMA PREVIEW

### Metadata Section

| Field | Value | Description |
|-------|-------|-------------|
| version | 1.0 | Schema version |
| project | SecondChance | Project name |
| target_path | /Content/FatherCompanion/ | Base content path |

### Tags Schema

| Field | Type | Example |
|-------|------|---------|
| path | String | Ability.Father.Crawler |
| comment | String | Crawler form ability tag |

### Input Actions Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | IA_FatherFormWheel |
| value_type | String | Digital |

### Input Mapping Context Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | IMC_FatherCompanion |
| mappings[].action | String | IA_FatherFormWheel |
| mappings[].key | String | Tab |
| mappings[].modifiers | Array | [] |

### Curves Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | FC_UltimateThreshold |
| keys[].time | Float | 0.0, 50.0, 100.0 |
| keys[].value | Float | 0.0, 250.0, 500.0 |

### Enumerations Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | E_FatherForm |
| values | Array | Crawler, Armor, Exoskeleton, Symbiote, Engineer |

### Configurations Schema - AbilityConfiguration

| Field | Type | Example |
|-------|------|---------|
| type | String | AbilityConfiguration |
| name | String | AC_FatherCompanion_Default |
| default_attributes | String | GE_DefaultNPCAttributes |
| abilities | Array | GA_FatherCrawler, GA_FatherArmor, ... |

### Configurations Schema - ActivityConfiguration

| Field | Type | Example |
|-------|------|---------|
| type | String | ActivityConfiguration |
| name | String | ActConfig_FatherCompanion |
| request_interval | Float | 0.5 |
| activities | Array | BPA_FatherFollow, BPA_Attack_Melee, BPA_Idle |
| goal_generators | Array | GoalGenerator_Attack |

### Configurations Schema - NPCDefinition

| Field | Type | Example |
|-------|------|---------|
| type | String | NPCDefinition |
| name | String | NPCDef_FatherCompanion |
| character_class | String | BP_FatherCompanion |
| ability_configuration | String | AC_FatherCompanion_Default |
| activity_configuration | String | ActConfig_FatherCompanion |
| default_item_loadout[].collection | String | IC_FatherForms |

### Gameplay Effects Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | GE_FormChangeCooldown |
| duration_policy | String | HasDuration, Instant, Infinite |
| duration_magnitude | Float | 15.0 |
| granted_tags | Array | Cooldown.Father.FormChange |
| executions[].class | String | NarrativeDamageExecCalc |
| set_by_caller_tags | Array | Damage.Father.Base |

### Equippable Items Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | BP_FatherArmorForm |
| equipment_slot | String | Narrative.Equipment.Slot.FatherForm |
| equipment_modifier | String | GE_EquipmentModifier_FatherArmor |
| abilities | Array | GA_ProtectiveDome, GA_DomeBurst |

### Gameplay Abilities Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | GA_FatherCrawler |
| parent_class | String | NarrativeGameplayAbility |
| ability_tags | String | Ability.Father.Crawler |
| cancel_abilities_with_tag | Array | Ability.Father.Armor, ... |
| activation_owned_tags | Array | Father.Form.Crawler, Father.State.Detached |
| activation_required_tags | Array | Father.State.Alive, Father.State.Recruited |
| net_execution_policy | String | ServerOnly |
| instancing_policy | String | InstancedPerActor |

### Behavior Trees Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | BT_FatherFollow |
| blackboard | String | BB_FollowCharacter |
| based_on | String | BT_FollowCharacter |
| remove_services | Array | BTS_SetAIFocus |
| keep_services | Array | BTS_AdjustFollowSpeed |

### Blackboards Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | BB_FatherCompanion |
| parent | String | BB_Attack |
| keys | Array | Nested key definitions |
| keys[].key_name | String | OwnerPlayer |
| keys[].key_type | String | Object, Bool, Float, Int, Vector |
| keys[].base_class | String | /Script/Engine.Pawn (for Object types) |
| keys[].instance_synced | Boolean | true |

#### Blackboards YAML Format Requirements

Blackboard keys must be nested under the keys property of the parent blackboard. Each key must use key_name and key_type fields.

Correct format:

| Line | Content |
|------|---------|
| 1 | blackboards: |
| 2 | (2 spaces) - name: BB_FatherCompanion |
| 3 | (4 spaces) keys: |
| 4 | (6 spaces) - key_name: OwnerPlayer |
| 5 | (8 spaces) key_type: Object |
| 6 | (8 spaces) base_class: /Script/Engine.Pawn |
| 7 | (6 spaces) - key_name: TargetEnemy |
| 8 | (8 spaces) key_type: Object |
| 9 | (6 spaces) - key_name: IsAttached |
| 10 | (8 spaces) key_type: Bool |

Incorrect format (creates separate blackboard assets for each key):

| Line | Content | Problem |
|------|---------|---------|
| 1 | blackboards: | |
| 2 | (2 spaces) - name: BB_FatherCompanion | |
| 3 | (2 spaces) - name: OwnerPlayer | Treated as new blackboard |
| 4 | (2 spaces) - name: TargetEnemy | Treated as new blackboard |

The parser detects keys: section start and then looks for - key_name: entries at the appropriate indent level.

### Activities Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | BPA_FatherFollow |
| parent_class | String | BPA_FollowCharacter |
| activity_name | String | Father Follow |
| behavior_tree | String | BT_FatherFollow |
| supported_goal_type | String | Goal_FollowCharacter |
| owned_tags | Array | Narrative.State.NPC.Activity.Following |

### Goals Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | Goal_Alert |
| parent_class | String | NPCGoalItem |
| variables[].name | String | AlertLocation |
| variables[].type | String | Vector, Float, Actor, Boolean, Integer |
| variables[].default | Any | 0.0 |

### Goal Generators Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | GoalGenerator_Alert |
| parent_class | String | NPCGoalGenerator |
| variables[].name | String | DetectionRange |
| variables[].type | String | Float, TimerHandle, GameplayTag, Actor |
| variables[].default | Any | 800.0 |

### Tagged Dialogue Sets Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | TaggedDialogueSet_Returned |
| dialogues[].tag | String | Narrative.TaggedDialogue.Returned.StartFollowing |
| dialogues[].dialogue | String | DBP_Returned_StartFollow |
| dialogues[].cooldown | Float | 30.0 |
| dialogues[].max_distance | Float | 5000.0 |

### BT Services Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | BTS_CheckExplosionProximity |
| parent_class | String | BTService_BlueprintBase |
| interval | Float | 0.5 |
| random_deviation | Float | 0.1 |
| variables[].name | String | ExplosionRange |
| variables[].type | String | Float, Boolean, Actor, Vector |
| variables[].default | Any | 300.0 |

### BT Tasks Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | BTT_TriggerExplosion |
| parent_class | String | BTTask_BlueprintBase |
| variables[].name | String | ExplosionDamage |
| variables[].type | String | Float, Actor, GameplayTag |
| variables[].default | Any | 100.0 |

### BT Decorators Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | BTD_IsTargetInRange |
| parent_class | String | BTDecorator_BlueprintBase |
| variables[].name | String | CheckRange |
| variables[].type | String | Float, Boolean |
| variables[].default | Any | 500.0 |

### Widgets Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | WBP_MarkIndicator |
| variables[].name | String | IconImage |
| variables[].type | String | Image, Float, WBP_UltimateIcon |
| variables[].default | Any | 5.0 |

### Actor Blueprints Schema

| Field | Type | Example |
|-------|------|---------|
| name | String | BP_FatherCompanion |
| parent_class | String | NarrativeNPCCharacter |
| components[].name | String | AIPerception |
| components[].type | String | AIPerceptionComponent |
| variables[].name | String | OwnerPlayer |
| variables[].type | String | NarrativePlayerCharacter |
| variables[].replicated | Boolean | true |

---

## VERIFICATION CHECKLIST

### Phase 1 Complete When:

| Item | Status |
|------|--------|
| 174 tags in DefaultGameplayTags.ini | |
| 6 Input Action assets in /Content/FatherCompanion/Input/ | |
| IMC_FatherCompanion with all bindings | |
| FC_UltimateThreshold curve asset | |

### Phase 2 Complete When:

| Item | Status |
|------|--------|
| E_FatherForm and EFatherUltimate enums | |
| IC_FatherForms with 4 items | |
| NE_SetFatherOwner and NE_ClearFatherOwner | |
| AC_FatherCompanion_Default with 10 abilities | |
| ActConfig_FatherCompanion with activities | |
| NPCDef_FatherCompanion configured | |
| CD_FatherCompanion configured | |

### Phase 3 Complete When:

| Item | Status |
|------|--------|
| All 34 GE_ assets created | |
| Correct duration policies | |
| SetByCaller tags configured | |
| Granted tags assigned | |

### Phase 4 Complete When:

| Item | Status |
|------|--------|
| 4 EquippableItem blueprints | |
| Correct equipment slot | |
| Abilities arrays populated | |
| Equipment modifier GEs assigned | |

### Phase 5 Complete When:

| Item | Status |
|------|--------|
| 18 GA_ blueprints created | |
| Class Defaults configured | |
| Tag configurations complete | |
| Event graphs functional (or marked for manual) | |

### Phase 6 Complete When:

| Item | Status |
|------|--------|
| BT_FatherFollow behavior tree | |
| BT_FatherEngineer behavior tree | |
| BB_FatherEngineer blackboard | |
| BPA_FatherFollow activity | |
| ActConfig updated with activities | |

### Phase 7 Complete When:

| Item | Status |
|------|--------|
| 3 WBP_ widgets with structure | |
| 4 BP_ actors with components | |
| Variables and bindings set | |
| Logic marked for manual implementation | |

### Phase 8 Complete When:

| Item | Status |
|------|--------|
| NAS_FatherAttack animation set | |
| 5 AM_ animation montages | |
| M_LaserGlow material | |
| DBP_FatherCompanion dialogue | |

---

## FIRST TIME SETUP

Claude provides a complete starter zip containing:

| File | Purpose |
|------|---------|
| manifest.yaml | Complete with all tags and assets |
| GA_FatherArmor_v4_2.md | Armor form guide |
| GA_FatherCrawler_v3_3.md | Crawler form guide |
| GA_FatherExoskeleton_v3_10.md | Exoskeleton form guide |
| GA_FatherSymbiote_v3_5.md | Symbiote form guide |
| GA_FatherEngineer_v4_3.md | Engineer form guide |
| ... | All implementation guides |

User extracts and points plugin to folder.

---

## AC_FATHERCOMPANION_DEFAULT CONTENTS

| Index | Ability | Source |
|-------|---------|--------|
| 0 | GA_FatherCrawler | Custom |
| 1 | GA_FatherArmor | Custom |
| 2 | GA_FatherExoskeleton | Custom |
| 3 | GA_FatherSymbiote | Custom |
| 4 | GA_FatherEngineer | Custom |
| 5 | GA_FatherAttack | Custom |
| 6 | GA_FatherLaserShot | Custom |
| 7 | GA_FatherMark | Custom |
| 8 | GA_FatherSacrifice | Custom |
| 9 | GA_Death | Built-in (referenced) |

---

## CHANGELOG

### v7.8.2 Plugin Bug Fixes (v2.1.0 - v2.1.9)

| Version | Change | Description |
|---------|--------|-------------|
| v2.1.0 | Copy Log button | Added clipboard copy functionality for log output |
| v2.1.0 | Clear Log button | Added button to clear log panel |
| v2.1.0 | ApplicationCore module | Added dependency for FWindowsPlatformApplicationMisc::ClipboardCopy |
| v2.1.1 | Tag parser fix | Extract tag value from "- tag: VALUE" manifest format |
| v2.1.1 | Widget Blueprint generator | Implemented actual asset creation (was placeholder) |
| v2.1.1 | UMGEditor integration | Added WidgetBlueprintFactory for proper widget creation |
| v2.1.2 | TagsIniPath resolution | Convert relative path to absolute for tag existence checks |
| v2.1.2 | Tag SKIPPED status | Tags now correctly show SKIPPED on subsequent runs |
| v2.1.3 | ParseManifest array clearing | Clear all arrays before populating to prevent duplicates on re-parse |
| v2.1.3 | Duplicate asset prevention | Ensure each asset only generated once per run |
| v2.1.4 | UE5.6 tag format support | Parse both +GameplayTags= and +GameplayTagList= formats |
| v2.1.4 | Tag existence check fix | Correctly detect existing tags in UE5.6 format |
| v2.1.5 | Asset count display fix | GetTotalAssetCount() now includes all parsed categories |
| v2.1.5 | Initialization dialog accuracy | Shows correct "174 tags, 70 assets" instead of "174 tags, 59 assets" |
| v2.1.9 | ParseActorBlueprints nested properties | Added bInComponents and bInVariables tracking for nested subsections |
| v2.1.9 | ParseWidgets nested properties | Added bInVariables tracking for nested variables subsection |
| v2.1.9 | Nested property accumulation | Components and variables now added to parent blueprint, not as separate assets |

v2.1.x Bug Summary:

| Bug ID | Version Fixed | Symptom | Root Cause |
|--------|---------------|---------|------------|
| BUG-008 | v2.1.1 | Tags stored with "tag:" prefix | Parser returned raw YAML key-value |
| BUG-009 | v2.1.1 | Widget blueprints show NEW every run | Generator was placeholder (no actual creation) |
| BUG-010 | v2.1.2 | Tags show NEW every run | TagsIniPath not resolved to absolute path |
| BUG-011 | v2.1.3 | Duplicate assets generated on re-run | Arrays not cleared before ParseManifest |
| BUG-012 | v2.1.4 | Tags always NEW in UE5.6 | Parser did not recognize +GameplayTagList= format |
| BUG-013 | v2.1.5 | Init shows 59 assets, generates 70 | GetTotalAssetCount() missing categories |
| BUG-014 | v2.1.9 | Components and variables created as separate BP_ assets | ParseActorBlueprints and ParseWidgets missing nested property detection |

v2.1.5 Asset Count Fix:

| Category Missing from Count | Count |
|-----------------------------|-------|
| Materials | 1 |
| Blackboards | 2 |
| Behavior Trees | 2 |
| Widget Blueprints | 3 |
| Actor Blueprints | 4 |
| Total Missing | 12 |

v2.1.5 Complete Fix:

| Component | Issue | Fix |
|-----------|-------|-----|
| FManifestData | No GetTotalAssetCount() function | Added function that sums all parsed arrays |
| LoadManifest() | Used partial sum for display | Now uses GetTotalAssetCount() |
| Initialization dialog | Showed 59 instead of 70 | Correctly shows 70 assets |

v2.1.2 Complete Fix Chain:

| Component | Issue | Fix |
|-----------|-------|-----|
| ParseTags() | Returns "tag: Ability.X" | Strip "tag:" prefix if present |
| FWidgetBlueprintGenerator | Placeholder code | Full UWidgetBlueprintFactory implementation |
| GenerateTags() | Relative path "Config/X.ini" | FPaths::Combine(FPaths::ProjectDir(), TagsIniPath) |

### v7.7.0 Expanded Programmatic Asset Creation Research

| Change | Description |
|--------|-------------|
| Added Behavior Tree Generation Enhancement section | FBTCompositeChild arrays, decorator/service attachment |
| Added Widget Blueprint Visual Hierarchy section | WidgetTree::ConstructWidget API discovery |
| Added Animation Montage Enhancement section | FAnimNotifyEvent::Link() for notify timing |
| Added Gameplay Cue Generation section | Static vs Actor cue patterns |
| Added Camera Shake Generation section | Pattern-based architecture |
| Added AI Perception Configuration section | FAISenseConfig subclass patterns |
| Added Niagara VFX Generation section | Template duplication approach |
| Added behavior_tree_graph YAML schema | Full BT node hierarchy definition |
| Added widget_tree YAML schema | Visual widget hierarchy definition |
| Added montage_config YAML schema | Notifies, sections, blend settings |
| Added gameplay_cue YAML schema | Static and Actor cue definition |
| Added camera_shake YAML schema | Pattern-based shake definition |
| Added ai_perception YAML schema | Sense configuration definition |
| Updated External Dependencies | Widget hierarchy and BT nodes NOW SOLVABLE |
| Updated Assets by Generation Level | FULL: 259, PARTIAL: 7, STRUCTURE: 0 |
| Added new module dependencies | GameplayCameras, Niagara, NiagaraEditor, BehaviorTreeEditor |

| Research Finding (v7.7.0) | Previous Status | New Status |
|---------------------------|-----------------|------------|
| Behavior Tree node hierarchy | UNCERTAIN | FULLY POSSIBLE via FBTCompositeChild |
| Widget Blueprint visual hierarchy | NOT SOLVABLE | FULLY POSSIBLE via WidgetTree::ConstructWidget |
| Animation Montage notifies/sections | NOT SOLVABLE | FULLY POSSIBLE via FAnimNotifyEvent::Link() |
| Dialogue Blueprint nodes | NOT SOLVABLE | PARTIALLY POSSIBLE via custom UEdGraphNode |
| Niagara VFX systems | NOT ATTEMPTED | LIMITED (use template duplication) |
| Gameplay Cues | NOT ATTEMPTED | FULLY POSSIBLE via UGameplayCueNotify classes |
| Camera Shakes | NOT ATTEMPTED | FULLY POSSIBLE via UCameraShakePattern classes |
| AI Perception config | NOT ATTEMPTED | FULLY POSSIBLE via FAISenseConfig classes |

| Impact Metric | v7.6.0 | v7.7.0 |
|---------------|--------|--------|
| FULL assets | 270 | 259 |
| PARTIAL assets | 0 | 7 |
| STRUCTURE assets | 10 | 0 |
| Manual work reduction | 40-50% | 75-85% |

| Implementation Time Update | v7.6.0 | v7.7.0 |
|----------------------------|--------|--------|
| Phase B (Schema Extension) | 9 hours | 17 hours |
| Phase C (Generator Implementation) | 18 hours | 34 hours |
| Phase D (Integration Testing) | 6 hours | 11 hours |
| Total Estimate | 43 hours | 62 hours |

### v7.6.0 Event Graph Generation Enhancement Research

| Change | Description |
|--------|-------------|
| Added Event Graph Generation Enhancement section | Research findings on programmatic Blueprint Event Graph creation |
| Added event_graph YAML schema | Schema for defining event graph nodes and connections |
| Added material_graph YAML schema | Schema for defining material expression nodes and connections |
| Added C++ API documentation | UK2Node, FBlueprintEditorUtils, UMaterialExpression APIs |
| Added Implementation Phases | 4-phase roadmap with 43-hour estimate |
| Added Assets Upgrade Potential table | 22 assets (17 GA + 4 BP + 1 M) can be upgraded to FULL |
| Updated AUTO-GENERATION LEVELS | Added UPGRADEABLE level and future projections |
| Added Event Graph Generation Module Dependencies | BlueprintGraph, Kismet, KismetCompiler, GraphEditor modules |
| Added References section | Research sources and documentation links |

| Research Finding | Feasibility |
|------------------|-------------|
| Blueprint Event Graph creation | FULLY POSSIBLE via UK2Node classes |
| Material Graph creation | FULLY POSSIBLE via UMaterialExpression classes |
| Widget Event Logic | PARTIALLY POSSIBLE (visual hierarchy requires Designer) |
| Animation Montages | NOT POSSIBLE (external animation dependency) |
| Dialogue Blueprints | NOT POSSIBLE (specialized graph editor) |

| Impact Metric | Value |
|---------------|-------|
| FULL assets before | 248 |
| FULL assets after | 270 |
| STRUCTURE assets before | 18 |
| STRUCTURE assets after | 10 |
| Manual work reduction | 40-50% |

### v7.5.5 NPC Asset Type Support

| Change | Description |
|--------|-------------|
| Added Goals (NPCGoalItem) | Blueprint generation with variables |
| Added GoalGenerators (NPCGoalGenerator) | Blueprint generation with variables |
| Added TaggedDialogueSets | Data Asset generation |
| Added BT Services | Blueprint generation with interval settings |
| Added BT Tasks | Blueprint generation |
| Added BT Decorators | Blueprint generation |
| Fixed struct forward reference | Moved FManifestActorVariableDefinition before dependent structs |
| Added BlueprintEditorUtils include | Required for FBlueprintEditorUtils::AddMemberVariable |
| Added Kismet module dependency | Required for BlueprintEditorUtils.h |
| Updated Plugin to v1.5.0 | Full NPC system asset support |

Plugin v1.5.0 New YAML Sections:
Plugin v1.5.0 New YAML Sections:

| Section | Asset Type | Parent Class |
|---------|------------|--------------|
| goals: | Goal Blueprint | NPCGoalItem |
| goal_generators: | GoalGenerator Blueprint | NPCGoalGenerator |
| tagged_dialogue_sets: | Data Asset | TaggedDialogueSet |
| bt_services: | BT Service Blueprint | BTService_BlueprintBase |
| bt_tasks: | BT Task Blueprint | BTTask_BlueprintBase |
| bt_decorators: | BT Decorator Blueprint | BTDecorator_BlueprintBase |

NPC Systems Now Generatable:

| NPC System | Assets Generated |
|------------|------------------|
| Possessed Exploder | Actor, NPCDef, AbilityConfig, ActivityConfig, GE |
| Gatherer Scout | Actor, Goal, GoalGenerator, Activity, NPCDef, Configs |
| Warden Husk | Actors (2), NPCDefs (2), GEs, Configs |
| Support Buffer | Actor, Activity, NPCDef, Configs, GE |
| Random Aggression Stalker | GoalGenerator, TaggedDialogueSet, NPCDef, Configs |

Output Paths for New Asset Types:

| Asset Type | Output Path |
|------------|-------------|
| Goals | /Game/FatherCompanion/AI/Goals/ |
| Goal Generators | /Game/FatherCompanion/AI/GoalGenerators/ |
| Tagged Dialogue Sets | /Game/FatherCompanion/Dialogue/ |
| BT Services | /Game/FatherCompanion/AI/BTNodes/ |
| BT Tasks | /Game/FatherCompanion/AI/BTNodes/ |
| BT Decorators | /Game/FatherCompanion/AI/BTNodes/ |

### v7.5.4 Incremental YAML Implementation

| Change | Description |
|--------|-------------|
| Finalized Incremental YAML Workflow | Plugin v1.4.0 now scans for *.yaml files |
| Added manifest whitelist validation | Incremental files validated against manifest before generation |
| Added automatic cleanup | Success deletes file, failure keeps for retry |
| Updated Single Source of Truth | Manifest is whitelist, incrementals are updates |

Plugin v1.4.0 Implementation:

| Function | Purpose |
|----------|---------|
| ScanForIncrementalFiles() | Find *.yaml excluding manifest.yaml |
| ValidateIncrementalFile() | Check asset_name exists in manifest |
| ProcessIncrementalFile() | Parse and generate single asset |
| CleanupIncrementalFile() | Delete on success |

Incremental File Processing Flow:

| Step | Action | On Error |
|------|--------|----------|
| 1 | Scan directory for *.yaml (not manifest.yaml) | Continue |
| 2 | Parse each incremental file | Log error, keep file |
| 3 | Validate asset_name in manifest whitelist | Log error, keep file |
| 4 | Generate/update asset | Log error, keep file |
| 5 | Delete incremental file | Continue |

### v7.5.3 Incremental YAML Workflow and Parent Class Documentation

| Change | Description |
|--------|-------------|
| Added Incremental YAML Workflow section | Documents dual workflow for updates vs new assets |
| Added Update Existing Asset Flow | Single .yaml file for asset updates |
| Added Add New Asset Flow | Full manifest replacement for new assets |
| Added Incremental File Validation | Whitelist validation against manifest |
| Added Incremental File Cleanup | Delete on success, keep on failure |
| Added Parent Class Search Order section | Documents FindParentClass module search sequence |
| Added UserWidget Resolution table | Shows 4-step search order for widget parent class |
| Added Expected Log Warnings table | Documents normal "Failed to find object" warnings during search |

Incremental Workflow Summary:

| Request Type | Claude Provides | Plugin Action |
|--------------|-----------------|---------------|
| Update GA_FatherArmor | GA_FatherArmor_v4_3.yaml | Regenerate, delete .yaml on success |
| Add new ability | Updated manifest.yaml | Full generation of new asset |

Plugin v1.4.0 Required Changes:

| Feature | Implementation |
|---------|----------------|
| Scan for *.yaml | IFileManager::Get().FindFiles() excluding manifest.yaml |
| Validate against manifest | Check asset_name exists in manifest whitelist |
| Generate single asset | Reuse existing generator functions |
| Cleanup on success | IFileManager::Get().Delete() incremental file |

Log Warnings Clarification:

| Warning Type | Status | Explanation |
|--------------|--------|-------------|
| Failed to find object 'Class /Script/NarrativePro.UserWidget' | Normal | Search step 1 of 4 |
| Failed to find object 'Class /Script/NarrativeArsenal.UserWidget' | Normal | Search step 2 of 4 |
| Failed to find object 'Class /Script/Engine.UserWidget' | Normal | Search step 3 of 4 |
| Asset created successfully | Success | Found in /Script/UMG.UserWidget |

These warnings are informational only and do not indicate asset generation failure.

### v7.5.2 Parser Section Exit and Results Dialog Fixes

| Change | Description |
|--------|-------------|
| Fixed ParseCurves section bleeding | Added indent-based section exit logic to prevent cross-section parsing |
| Fixed ParseWidgets section name | Added widget_blueprints: as alias for widgets: |
| Added Results Dialog improvements | Shows plugin version, manifest version, NEW/SKIPPED/FAILED summary with categorized lists |
| Added SKIPPED status detection | All generators now return SKIPPED status for existing assets |
| Added Parser Section Exit Pattern | Documented required code pattern for all parse functions |
| Added Section Name Aliases table | Reference for all parser section name variants |
| Added Results Dialog section | Full documentation of new results dialog format |
| Added Results Dialog Debugging | Troubleshooting table for category mismatch issues |

| Plugin Version | Bug | Symptom | Root Cause | Solution |
|----------------|-----|---------|------------|----------|
| v1.3.1 | ParseCurves section bleeding | BP_, WBP_, IMC_ assets listed under Float Curves | No section exit logic (bInSection never set to false) | Add indent-based section boundary detection |
| v1.3.1 | ParseWidgets wrong section name | Widget Blueprints category missing from results | Parser looked for widgets: but manifest uses widget_blueprints: | Add widget_blueprints: as accepted section name |
| v1.3.1 | Unclear results dialog | Could not distinguish new vs existing assets | No status differentiation in output | Add SKIPPED return status and categorized display |

Parser Section Exit Fix Details:

| Before (Broken) | After (Fixed) |
|-----------------|---------------|
| bInSection = true when section starts | bInSection = true, SectionIndent = CurrentIndent when section starts |
| No section exit condition | if (bInSection and CurrentIndent <= SectionIndent and !TrimmedLine.IsEmpty() and !TrimmedLine.StartsWith("-")) then exit section |
| Parser continues into next section | Parser cleanly exits when new section encountered |

Results Dialog Format Change:

| Element | v1.3.1 | v1.3.2 |
|---------|--------|--------|
| Header | None | Plugin Version + Manifest Version |
| Summary | None | NEW/SKIPPED/FAILED counts |
| Categorization | Flat list | Grouped by asset type with visual indicators |
| Status indicators | None | + (new), - (skipped), X (failed) |

### v7.5.1 Build System and Config Persistence Fixes

| Change | Description |
|--------|-------------|
| Added UMGEditor module | Required for WidgetBlueprint.h and WidgetBlueprintFactory.h |
| Added BlueprintGraph module | Required for UEdGraphSchema_K2::PC_* pin type constants |
| Added Plugin Module Dependencies section | Complete module reference with headers and symbols |
| Added Common Build Errors table | Error codes, messages, and solutions |
| Added UEdGraphSchema_K2 Pin Type Constants | Reference table for all PC_* constants |
| Fixed config persistence | GConfig->Flush() called immediately after path changes |
| Added startup logging | Logs loaded path and validation status on editor start |
| Improved Settings dialog | Shows warning if manifest.yaml not found in selected folder |

| Plugin Version | Error Fixed | Solution |
|----------------|-------------|----------|
| v1.2.1 | C1083: Cannot open include file WidgetBlueprint.h | Add UMGEditor module |
| v1.2.2 | LNK2019: Unresolved external UEdGraphSchema_K2::PC_* | Add BlueprintGraph module |
| v1.2.3 | Config not persisted between sessions | Add GConfig->Flush() after every path change |

### v7.5 Blackboard Parsing and Manifest Fixes

| Change | Description |
|--------|-------------|
| Fixed blackboard keys parsing | Changed keys: detection from Equals to StartsWith |
| Added nested key accumulation | Keys now properly added to parent blackboard instead of creating separate assets |
| Added manifest completeness docs | Documented requirement for all 174 tags |
| Added Blackboards Schema section | Full YAML format specification for blackboard definitions |
| Added Common YAML Formatting Issues | Table of known issues and solutions |
| Added key type mapping table | UE blackboard key class mapping |
| Updated ParseBlackboards safeguards | Added StartsWith and key accumulation protections |

| Bug Fixed | Cause | Solution |
|-----------|-------|----------|
| Only 22 tags generated | Incomplete sample manifest | Extract all tags from reference INI |
| Keys created as separate assets | Wrong YAML format + parser used Equals | Correct nested format + StartsWith detection |

### v7.4 Parser Safeguards

| Change | Description |
|--------|-------------|
| Fixed asset count summary | Corrected FULL from ~232 to 248, STRUCTURE from ~28 to 18, USES BUILT-IN from 7 to 8 |
| Added duplicate prevention | All 18 parser functions check ContainsByPredicate before adding |
| Added struct initializers | 14 struct properties have explicit default values |
| Added UMG module path | FindParentClass searches /Script/UMG for UserWidget |
| Fixed float curves parser | Section boundary detection corrected |
| Added YAML Parser Safeguards section | Documentation for all parser protections |

### v7 Changes from v6

| Change | Description |
|--------|-------------|
| Added BP_TurretProjectile | Missing turret projectile actor |
| Added CD_FatherCompanion | CharacterDefinition for save/load persistence |
| Added DBP_FatherCompanion | DialogueBlueprint for recruitment |
| Added 5 cooldown GEs | GE_LaserCooldown, GE_TurretShootCooldown, GE_StealthCooldown, GE_ElectricTrapCooldown, moved GE_ExoskeletonDashCooldown |
| Added 2 sacrifice GEs | GE_SacrificeInvulnerability, GE_FatherOffline |
| Added BB_FatherEngineer | Blackboard for engineer turret AI |
| Added BT_FatherEngineer | Behavior tree for engineer combat |
| Added 6 animation assets | NAS_FatherAttack, AM_FatherAttack, AM_FatherThrowWeb, AM_FatherSacrifice, AM_FatherReactivate, AM_FatherDetach |
| Added M_LaserGlow | Material for laser projectile |
| Renamed UltimateChargeComponent | Changed to SymbioteChargeComponent |
| Updated GE_ total | Changed from 28 to 34 |
| Updated asset grand total | Changed from 249 to 267 |
| Reorganized GE sections | Split Combat into Damage and Cooldown subsections |
| Clarified AC_ contents | 9 custom + 1 built-in (GA_Death) |
| Converted code blocks | All code blocks converted to markdown tables |
| Restored State File Structure | State file format and tracking logic |
| Restored EMBEDDED YAML IN GUIDES | GENERATOR_DATA block format and fields |
| Restored PLUGIN USER INTERFACE | Main window, log window, validation messages |
| Restored MANIFEST AS WHITELIST | Safety design and validation flow |
| Restored YAML SCHEMA PREVIEW | All schema types as table format |

### v7.8.2 Manifest Count Verification and Sync

| Change | Description |
|--------|-------------|
| Verified manifest asset counts | All counts cross-checked against manifest.yaml |
| Updated ASSET SUMMARY BY TYPE | Corrected all counts to match manifest |
| Added GA_Backstab | Missing Exoskeleton ability (18 total GAs) |
| Fixed GA_ ability names | GA_ExoskeletonDash, GA_ExoskeletonSprint, GA_ElectricTrap |
| Updated Phase counts | Foundation: 185, AI Integration: 5, Abilities: 18, Support: 9 |
| Added NON-TAG SUBTOTAL | Explicit count of 97 non-tag assets |
| Updated GRAND TOTAL | Changed from ~267 to 271 (174 tags + 97 assets) |
| Synced manifest.yaml header | Version 1.5.0 with correct counts |
| Added v2.1.3-v2.1.5 bug fixes | ParseManifest clearing, UE5.6 tag format, asset count display |
| Updated Generator Classes table | Exact counts for all generator outputs |

| Count Correction | Old Value | New Value | Reason |
|-----------------|-----------|-----------|--------|
| Total Tags | ~174 | 174 | Exact count |
| Input Actions | 6 | 7 | IA_FatherFormWheel added |
| Float Curves | 1 | 3 | FC_DomeAbsorption, FC_SymbioteCharge, FC_StealthFade |
| Gameplay Abilities | 17 | 18 | GA_Backstab added |
| Blackboards | 1 | 2 | BB_FatherEngineer separate |
| Animation Assets | 6 | 7 | NAS_FatherAttack counted separately |
| Non-Tag Total | ~93 | 97 | All categories counted |
| Grand Total | ~267 | 271 | 174 + 97 |


### v7.8.1 Result Counting and Duplicate Generation Fixes

| Change | Description |
|--------|-------------|
| Fixed result counter bug | Generators now return EGenerationStatus enum instead of bool |
| Fixed duplicate generation passes | Added bIsGenerating guard flag in UI window |
| Added FGenerationResult struct | Proper tracking of asset name, status, message, and category |
| Added FGenerationSummary struct | Aggregates results with accurate NEW/SKIPPED/FAILED counts |
| Added CheckExistsAndPopulateResult helper | Shared existence check returns SKIPPED status properly |
| Fixed parser section exit | ShouldExitSection now correctly detects new section headers |
| Updated plugin version | v2.0.9 with all bug fixes |

| Plugin Version | Bug | Symptom | Root Cause | Solution |
|----------------|-----|---------|------------|----------|
| v2.0.8 | Result counter wrong | Log shows SKIPPED but results show NEW: 69 SKIPPED: 0 | Generators returned bool, not status | Return EGenerationStatus::Skipped |
| v2.0.8 | Duplicate generation passes | Skip messages appear twice in log | UI callback triggered multiple times | Add bIsGenerating guard flag |
| v2.0.8 | Window destroyed twice | LogSlate shows window destroyed multiple times | No re-entry protection | Guard flag prevents concurrent runs |

v2.0.9 Bug Fix Implementation Details:

| Component | Before (v2.0.8) | After (v2.0.9) |
|-----------|-----------------|----------------|
| Generator Return | bool success | EGenerationStatus enum |
| Existence Check | Log skip, return true | Return FGenerationResult with Skipped status |
| Counter Logic | if (success) NewCount++ | switch (Status) case New/Skipped/Failed |
| UI Button | Direct call to GenerateAssets() | Check bIsGenerating first |
| Results Dialog | Counts all as NEW | Accurate NEW/SKIPPED/FAILED breakdown |

Generator Status Return Pattern (v2.0.9):

| Scenario | Return Value | Result |
|----------|--------------|--------|
| Asset exists on disk | FGenerationResult(Name, EGenerationStatus::Skipped, "Already exists on disk") | SkippedCount++ |
| Asset in memory | FGenerationResult(Name, EGenerationStatus::Skipped, "Already loaded in memory") | SkippedCount++ |
| Asset created | FGenerationResult(Name, EGenerationStatus::New, "Created successfully") | NewCount++ |
| Error occurred | FGenerationResult(Name, EGenerationStatus::Failed, ErrorMessage) | FailedCount++ |

Files Modified in v2.0.9:

| File | Changes |
|------|---------|
| GasAbilityGeneratorTypes.h | Added EGenerationStatus, FGenerationResult, FGenerationSummary |
| GasAbilityGeneratorGenerators.h | Updated all generators to return FGenerationResult |
| GasAbilityGeneratorGenerators.cpp | Implemented CheckExistsAndPopulateResult helper |
| GasAbilityGeneratorWindow.h | Added bIsGenerating guard flag |
| GasAbilityGeneratorWindow.cpp | Check guard in button handlers, use FGenerationSummary |
| GasAbilityGeneratorParser.cpp | Fixed ShouldExitSection logic |

### v7.8.0 Generator Implementation Update

| Change | Description |
|--------|-------------|
| Added FEventGraphGenerator specification | Complete UK2Node API patterns, node creation, pin connections, auto-layout |
| Added FBehaviorTreeGenerator specification | FBTCompositeChild patterns, decorator/service attachment, blackboard keys |
| Added FWidgetTreeGenerator specification | WidgetTree::ConstructWidget patterns, slot configuration, widget properties |
| Added FMaterialGraphGenerator specification | UMaterialExpression patterns, output connections, material properties |
| Added FMontageConfigGenerator specification | FAnimNotifyEvent::Link patterns, section creation, blend settings |
| Added FGameplayCueGenerator specification | Static vs Actor cue patterns, tag requirements |
| Added FCameraShakeGenerator specification | UCameraShakePattern patterns, oscillation properties |
| Added FAIPerceptionGenerator specification | FAISenseConfig patterns, perception setup |
| Added complete YAML schemas | 8 new schemas with full field definitions and examples |
| Added YAML examples | Real Father Companion asset examples for each schema type |
| Updated module dependencies | Added all required headers and modules for new generators |
| Added implementation phases | 62 hours total estimate across Phase B, C, D |
| Updated generator class hierarchy | 18 total generators (6 new) |
| Updated asset status table | All generators mapped to asset types |

Research Impact (v7.7.0 -> v7.8.0):

| Finding | Previous Status | New Status |
|---------|-----------------|------------|
| Event Graph Creation | Research Only | Full Implementation Spec |
| Behavior Tree Generation | FBTCompositeChild Documented | Full Implementation Spec |
| Widget Tree Generation | ConstructWidget Documented | Full Implementation Spec |
| Material Graph Generation | Expression APIs Documented | Full Implementation Spec |
| Montage Configuration | Link() API Documented | Full Implementation Spec |
| Gameplay Cue Generation | Base Classes Documented | Full Implementation Spec |
| Camera Shake Generation | Pattern Classes Documented | Full Implementation Spec |
| AI Perception Configuration | Sense Configs Documented | Full Implementation Spec |

---

**END OF FATHER ABILITY GENERATOR PLUGIN v7.8.2 SPECIFICATION**

**Total Tags: 174**
**Total Non-Tag Assets: 97**
**GRAND TOTAL: 271**
**FULL Auto-Generated: 262**
**PARTIAL (External Dependencies): 8**
**STRUCTURE Only: 0**
**Built-in References: 8**

**v2.1.9 Bug Fix: Nested blueprint properties (components/variables) created as separate assets (BUG-014)**
**v2.1.5 Bug Fix: Asset count display discrepancy (showed 59 instead of 70)**
**v7.8.2 Bug Fixes: 3 additional bugs (BUG-008, BUG-009, BUG-010)**
**v7.8.2 Plugin Version: 2.1.9**
**v7.8.1 Bug Fixes: 3 critical bugs (BUG-005, BUG-006, BUG-007)**
**v7.8.1 New Structs: 3 (EGenerationStatus, FGenerationResult, FGenerationSummary)**
**v7.8.0 New Generators: 6**
**v7.8.0 New YAML Schemas: 8**
**Total Generator Classes: 18**
**Total Implementation Estimate: 62 hours**
**Manual Work Reduction: 85-95%**

---

## KNOWN BUGS AND FIXES REFERENCE

This section documents all known bugs and their fixes for quick reference.

### Bug History by Plugin Version

| Version | Bug ID | Status | Description |
|---------|--------|--------|-------------|
| v1.2.4 | BUG-001 | FIXED | ParseBlackboards keys created as separate assets |
| v1.3.1 | BUG-002 | FIXED | ParseCurves section bleeding into other sections |
| v1.3.1 | BUG-003 | FIXED | ParseWidgets wrong section name |
| v1.3.1 | BUG-004 | FIXED | Results dialog unclear (no status differentiation) |
| v2.0.8 | BUG-005 | FIXED v2.0.9 | Result counter shows NEW for SKIPPED assets |
| v2.0.8 | BUG-006 | FIXED v2.0.9 | Duplicate generation passes in single run |
| v2.0.8 | BUG-007 | FIXED v2.0.9 | Window destroyed multiple times |
| v2.1.0 | BUG-008 | FIXED v2.1.1 | Tags stored with "tag:" prefix from manifest format |
| v2.1.0 | BUG-009 | FIXED v2.1.1 | Widget Blueprint generator was placeholder code |
| v2.1.1 | BUG-010 | FIXED v2.1.2 | TagsIniPath not resolved to absolute path |
| v2.1.8 | BUG-014 | FIXED v2.1.9 | ParseActorBlueprints and ParseWidgets nested properties created as separate assets |

### BUG-005: Result Counter Bug (v2.0.8)

| Symptom | User sees "NEW: 69 SKIPPED: 0" when all assets already exist |
|---------|-------------------------------------------------------------|
| Log Shows | Correct "Skipping existing..." messages for all 69 assets |
| Dialog Shows | Wrong "NEW: 69" instead of "SKIPPED: 69" |
| Root Cause | Generators returned bool, counter treated all true as NEW |
| Fix | Return EGenerationStatus enum, counter switches on status |

### BUG-006: Duplicate Generation Passes (v2.0.8)

| Symptom | Skip messages appear in duplicate blocks in log |
|---------|------------------------------------------------|
| Log Shows | Same assets logged twice with skip messages |
| Root Cause | UI button callback triggered multiple times |
| Evidence | Window destroyed twice per specification log |
| Fix | Added bIsGenerating guard flag in window class |

### BUG-007: Window Destroyed Multiple Times (v2.0.8)

| Symptom | LogSlate shows "Window being destroyed" multiple times |
|---------|-------------------------------------------------------|
| Root Cause | No protection against concurrent generation runs |
| Fix | Guard flag blocks button handler during active generation |

### BUG-008: Tag Parser Prefix Bug (v2.1.0)

| Symptom | Tags stored as "tag: Ability.Father.Armor" instead of "Ability.Father.Armor" |
|---------|------------------------------------------------------------------------------|
| Log Shows | All 174 tags show as NEW on every run |
| INI File | Contains malformed entries: +GameplayTags=(Tag="tag: Ability.Father.Armor") |
| Root Cause | GetArrayItemValue() returns raw "tag: VALUE" from manifest YAML |
| Fix | ParseTags() strips "tag:" prefix before storing: Tag = RawValue.Mid(4).TrimStart() |

### BUG-009: Widget Blueprint Placeholder (v2.1.0)

| Symptom | Widget blueprints show [NEW] on every run, never [SKIP] |
|---------|----------------------------------------------------------|
| Content Browser | No WBP_ assets created in FatherCompanion/UI/ folder |
| Log Shows | "Created Widget Blueprint: WBP_X" but no actual asset |
| Root Cause | FWidgetBlueprintGenerator::Generate() was placeholder with no creation code |
| Fix | Full implementation using UWidgetBlueprintFactory::FactoryCreateNew() |

### BUG-010: TagsIniPath Resolution Bug (v2.1.1)

| Symptom | Tags show [NEW] on every run despite correct format |
|---------|-----------------------------------------------------|
| Manifest Value | tags_ini_path: Config/DefaultGameplayTags.ini |
| FFileHelper | LoadFileToString() fails with relative path |
| ExistingTags Set | Empty because file not found |
| Root Cause | Relative path not resolved to absolute before file read |
| Fix | FPaths::Combine(FPaths::ProjectDir(), TagsIniPath) for Config/ prefix |

v2.1.2 Path Resolution Logic:

| Input Path | Resolution | Output |
|------------|------------|--------|
| Config/DefaultGameplayTags.ini | Project-relative | C:/Project/Config/DefaultGameplayTags.ini |
| DefaultGameplayTags.ini | Config directory | C:/Project/Config/DefaultGameplayTags.ini |
| C:/Absolute/Path.ini | Already absolute | C:/Absolute/Path.ini |

### BUG-014: Nested Blueprint Properties Bug (v2.1.8)

| Symptom | Components and variables created as separate actor/widget blueprints |
|---------|---------------------------------------------------------------------|
| Log Shows | "Created Actor Blueprint: AbilitySystemComponent" instead of component |
| Log Shows | "Created Actor Blueprint: OwnerPlayer" instead of variable |
| Expected | 4 actor blueprints (BP_FatherCompanion, BP_LaserProjectile, BP_TurretProjectile, BP_ElectricTrap) |
| Actual | 17+ actor blueprints including components and variables as separate assets |
| Root Cause | ParseActorBlueprints and ParseWidgets lack nested property detection for components: and variables: |
| Fix | Added bInComponents, bInVariables flags with indent-based subsection tracking |

v2.1.9 Nested Property Fix Pattern:

| Step | Detection | Action |
|------|-----------|--------|
| 1 | TrimmedLine.StartsWith("components:") | Set bInComponents = true, store ComponentsIndent |
| 2 | TrimmedLine.StartsWith("variables:") | Set bInVariables = true, store VariablesIndent |
| 3 | bInComponents and "- name:" | Add to CurrentBlueprint.Components, not ActorBlueprints |
| 4 | bInVariables and "- name:" | Add to CurrentBlueprint.Variables, not ActorBlueprints |
| 5 | CurrentIndent <= ComponentsIndent | Exit components subsection, set bInComponents = false |
| 6 | CurrentIndent <= VariablesIndent | Exit variables subsection, set bInVariables = false |

v2.1.9 Expected Results:

| Blueprint | Components | Variables | Total Items in Parent |
|-----------|------------|-----------|----------------------|
| BP_FatherCompanion | 3 | 5 | 8 nested in parent |
| BP_LaserProjectile | 0 | 2 | 2 nested in parent |
| BP_TurretProjectile | 0 | 2 | 2 nested in parent |
| BP_ElectricTrap | 2 | 3 | 5 nested in parent |
| Total | 5 | 12 | 17 nested properties |

v2.1.9 Correct Log Output:

| Log Message | Meaning |
|-------------|---------|
| Created Actor Blueprint: BP_FatherCompanion (3 components, 5 variables) | Parent with nested counts |
| Created Actor Blueprint: BP_LaserProjectile (0 components, 2 variables) | Parent with nested counts |
| Created Widget Blueprint: WBP_MarkIndicator (2 variables) | Widget with nested variable count |

### Verification Checklist After Fix

| Check | Expected Result | Pass/Fail |
|-------|-----------------|-----------|
| Generate with existing assets | SKIPPED count matches asset count | |
| Generate with new assets | NEW count matches created count | |
| Generate twice quickly | Second click ignored with warning | |
| Log messages | Each asset logged exactly once | |
| Window lifecycle | Destroyed exactly once on close | |
| Tag format in INI | No "tag:" prefix in Tag="" value | |
| Widget assets on disk | WBP_ files exist in Content/FatherCompanion/UI/ | |
| Second run tags | All 174 tags show SKIPPED | |
| Second run widgets | All 3 WBP_ show SKIPPED | |

### Quick Diagnostic Commands

| Symptom | Check | Solution |
|---------|-------|----------|
| Wrong counts | Verify generator returns EGenerationStatus | Update to v2.0.9+ |
| Duplicate logs | Check for bIsGenerating guard | Add guard flag |
| Section bleeding | Verify ShouldExitSection logic | Check indent comparison |
| Wrong category | Check DetermineCategory prefix | Add missing prefix case |
| Tags always NEW | Check TagsIniPath is absolute | Update to v2.1.2+ |
| Tag format wrong | Check ParseTags strips "tag:" | Update to v2.1.1+ |
| Widgets always NEW | Check FWidgetBlueprintGenerator implementation | Update to v2.1.1+ |
| Duplicates on re-run | Check arrays cleared in ParseManifest | Update to v2.1.3+ |
| Tags always NEW in UE5.6 | Check both +GameplayTags= and +GameplayTagList= parsing | Update to v2.1.4+ |
| Init count wrong | Check GetTotalAssetCount() includes all categories | Update to v2.1.5+ |
| Components as separate assets | Check ParseActorBlueprints bInComponents flag | Update to v2.1.9+ |
| Variables as separate assets | Check ParseActorBlueprints/ParseWidgets bInVariables flag | Update to v2.1.9+ |

### v2.1.5 Foolproofing Summary

| Component | Foolproofing Added |
|-----------|-------------------|
| Tag parser | Handles both "- tag: VALUE" and "- VALUE" formats |
| Path resolution | Handles relative, Config-prefixed, and absolute paths |
| Widget generator | Full implementation with proper error handling |
| Existence checks | Checks both disk and memory before creation |
| Log output | Copyable via Copy Log button for debugging |
| ParseManifest | Clears all arrays before populating |
| UE5.6 tag parsing | Supports both +GameplayTags= and +GameplayTagList= formats |
| Asset count display | GetTotalAssetCount() includes all 9 parsed categories |

### GetTotalAssetCount() Categories (v2.1.5)

| Category | Array | Included |
|----------|-------|----------|
| Input Actions | InputActions | Yes |
| Input Mapping Contexts | InputMappingContexts | Yes |
| Gameplay Effects | GameplayEffects | Yes |
| Gameplay Abilities | GameplayAbilities | Yes |
| Actor Blueprints | ActorBlueprints | Yes |
| Widget Blueprints | WidgetBlueprints | Yes |
| Blackboards | Blackboards | Yes |
| Behavior Trees | BehaviorTrees | Yes |
| Materials | Materials | Yes |

### v2.1.9 Nested Property Fix Summary

| Component | Fix Applied |
|-----------|-------------|
| ParseActorBlueprints | Added bInComponents flag for components: subsection detection |
| ParseActorBlueprints | Added bInVariables flag for variables: subsection detection |
| ParseActorBlueprints | Added ComponentsIndent and VariablesIndent tracking |
| ParseActorBlueprints | Nested components accumulated into CurrentBlueprint.Components |
| ParseActorBlueprints | Nested variables accumulated into CurrentBlueprint.Variables |
| ParseWidgets | Added bInVariables flag for variables: subsection detection |
| ParseWidgets | Added VariablesIndent tracking |
| ParseWidgets | Nested variables accumulated into CurrentWidget.Variables |

v2.1.9 Expected Asset Counts:

| Section | Before v2.1.9 | After v2.1.9 | Difference |
|---------|---------------|--------------|------------|
| Actor Blueprints | 17 (4 blueprints + 13 nested) | 4 blueprints | -13 incorrect assets |
| Widget Blueprints | 9 (3 widgets + 6 nested) | 3 widgets | -6 incorrect assets |
| Total | 26 | 7 | -19 incorrect assets |
