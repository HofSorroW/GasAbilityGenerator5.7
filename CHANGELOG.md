# GasAbilityGenerator Changelog

Full version history for versions prior to v4.14. For recent versions, see `CLAUDE.md`.

---

## v7.5.8 - Contract 22 Compliance Fix (Pre-Validation Gate)

**Root Cause:** Delegate pre-validation had two contract violations:
1. GA validator received empty variables array instead of `Definition.Variables`
2. Pre-validation errors logged but generation continued (soft-fail)

**Fix:** Contract 22 Compliance
- Pass `Definition.Variables` to validator so it can resolve `FatherASC` and other GA-defined variables
- Pre-validation errors now return `EGenerationStatus::Failed` and abort generation (hard-fail)
- Both GA generator and Actor BP generator enforce the gate

**Contract Reference:**
- Contract 22 (C_PIN_CONNECTION_FAILURE_GATE): "Soft-fail behavior FORBIDDEN"
- Contract 10 (Blueprint Compile Gate): "MUST NOT save" on errors

**Result:** 194/194 assets generate successfully with proper contract compliance.

---

## v7.5.7 - Track E Delegate Binding Fix (Contract 10.1)

**Root Cause:** The v4.16 "final compile with validation" was reconstructing `UK2Node_CreateDelegate` nodes and clearing `SelectedFunctionName` after the two-pass delegate binding had correctly set it.

**Fix:** Contract 10.1 - Conditional Final Compile
- Skip final compile if delegate bindings were created
- Two-pass binding already performs skeleton sync compile
- Preserves `SelectedFunctionName` on CreateDelegate nodes

**Complete Fix Chain (v7.5.5 → v7.5.7):**
- **v7.5.5**: Logging bridge - `FGeneratorLogCallback` forwards `LogGeneration()` to commandlet output
- **v7.5.6**: Two-pass delegate binding - Pass 1 creates handlers, single compile, Pass 2 creates nodes
- **v7.5.7**: Contract 10.1 - Skip final compile for abilities with delegate bindings

**New Runtime Module:** `GasAbilityGeneratorRuntime` with `UDamageEventBridge` component for Track-E delegate routing. Bridge intercepts raw delegates with `const FGameplayEffectSpec&` and broadcasts safe `FDamageEventSummary` struct.

**Fixed Abilities (5):**
- GA_FatherCrawler (OnDamagedBy, OnDealtDamage)
- GA_FatherArmor (OnDied)
- GA_FatherSymbiote (OnDamagedBy, OnHealedBy)
- GA_ProtectiveDome (OnDied)
- GA_StealthField (OnDamagedBy, OnDealtDamage)

**Result:** 194/194 assets now generate successfully (was 189/194).

See: `ClaudeContext/Handoffs/Delegate_Binding_Crash_Audit_v7_3.md`

---

## v6.9 - GA_Backstab + GA_FatherEngineer Generation Fixes

**Pin Resolution Improvements:**
- Improved DynamicCast output pin matching to handle UE's space-separated pin names (e.g., manifest `AsNPCActivityComponent` now matches UE pin `AsNPCActivity Component`)
- Added fuzzy pin name comparison that normalizes spaces and underscores for reliable matching across all As* cast output patterns

**TSubclassOf Search Paths:**
- Added Effects subfolders to Blueprint class resolution: FormState, Cooldowns, Damage, Stats, Utility
- Fixes GE_EngineerState and similar form state effects not being found for TSubclassOf pins

**NarrativePro Blackboard Paths:**
- Fixed plugin content mount path: `/NarrativePro/` (based on .uplugin name) not `/NarrativePro22B57/` (folder name)
- BB_Attack, BB_FollowCharacter, and other Narrative Pro blackboards now properly resolved

**Manifest Fix (CheckBackstabCondition):**
- Added `CastToActivityComp` node between GetActivityComp and GetCurrentGoal
- GetComponentByClass returns UActorComponent, needs explicit cast to NPCActivityComponent before calling NPCActivityComponent methods

**Result:** All 156 assets now generate successfully (0 failures).

---

## v4.27 - Dependency Sort Order Audit

- v4.27.1 - Dependency Sort Order Audit: Claude-GPT dual-agent audit of asset generation ordering. **Findings:** (1) TopologicalSort exists but never called - dependency graph informational only, (2) Materials phase (9) runs before MaterialFunctions phase (10) - dormant bug, (3) GA→GA manifest order issue proven (GA_ProtectiveDome → GA_DomeBurst TSubclassOf failure). **Quick-fix applied:** Moved material_functions section before materials in manifest (hygiene). **Backlog:** Consume TopologicalSort result in generation loops, swap M/MF phase order in commandlet. Verification: 156/156 assets. See `ClaudeContext/Handoffs/Dependency_Sort_Order_Audit_v1.md`.

---

## v4.13.x - v4.7

- v4.13.3 - Quest SM Semantic Verification: Added quest-specific state machine verification after generation. Validates: start state exists and is valid, detects duplicate State/Branch IDs, counts unresolved destination states, counts total tasks. Emits `RESULT QuestSM: states=N branches=M tasks=T unresolved=0 duplicates=0 start_valid=true` summary line per quest (consistent with global RESULT pattern). Warnings logged for each issue found; no hard-fail. Follows GPT recommendation for graph correctness verification while staying consistent with existing plugin patterns.
- v4.12.4 - Full Sync-from-Assets Implementation: Item Table now extracts EquipmentAbilities from UEquippableItem assets via reflection (TArray<TSubclassOf<UNarrativeGameplayAbility>> → comma-separated ability names). Quest Table now extracts per-state tasks, rewards, and parent branch relationships from UQuest assets. New FQuestAssetData fields: StateTasks, StateRewards, StateParentBranch TMap collections. Sync helpers ConvertStateNodeType() and SerializeTasks() use UE5 reflection to extract task class names and properties. Both ItemAssetSync and QuestAssetSync PopulateRowsFromAssets() now fully populate table rows from asset data. Also fixed AutoBuildAndTest.ps1 paths (NP22Beta → NP22B57, UE_5.6 → UE_5.7) and aligned version across .uplugin, module header, and CLAUDE.md.
- v4.12.3 - Table Editor Button Parity & Base Row Reconstruction: All 4 table editors (NPC, Dialogue, Quest, Item) now have consistent button sets: Save Table, Sync XLSX with 3-way merge. **NPC Table:** Added missing `OnSyncXLSXClicked()` for 3-way merge support. **Dialogue Table:** Added missing `OnSaveClicked()` with proper UPackage handling. **Base Row Fix:** Both NPC and Dialogue tables now properly reconstruct base rows using `LastSyncedHash != 0` filter (matching Quest/Item pattern). This ensures accurate 3-way merge delta detection.
- v4.12.2 - Validation-in-Sync-Dialog Fix: All 4 sync engines (NPC, Quest, Item, Dialogue) now call validators in `CompareSources()` and populate `ValidationStatus`/`ValidationMessages` on sync entries. Completes the validation-in-sync-dialog feature - users see actual validation errors/warnings before making approval decisions. UI components (CreateValidationCell, Apply button gating, row highlighting) were added in v4.12.1 but validation data was not being populated.
- v4.12.1 - Table Editor UI for Validation-in-Sync: Added validation column, error legend, summary counts, and Apply button gating to all 4 sync dialogs. Added CreateValidationCell() implementations.
- v4.11 - Niagara Emitter Controls & Environment-Aware Readiness: Two-tier emitter enable/disable system with headless-safe generation. **Soft Enable** (`enabled:`) sets `{Emitter}.User.Enabled` parameter - runtime toggle, no recompile. **Structural Enable** (`structural_enabled:`) calls `FNiagaraEmitterHandle::SetIsEnabled()` - compile-time change, triggers recompile only when state changes. **Sentinel Pattern:** `bHasEnabled`/`bHasStructuralEnabled` distinguish "not specified" from "explicitly set" - prevents unintended operations from defaults. **Compile Doctrine:** Initial compile required for new system identity; additional compile only for structural emitter deltas. Uses `WaitForCompilationComplete(true, false)` (GPU shaders, no progress dialog). **Environment-Aware Readiness Policy:** `IsReadyToRun()` is unreliable under `-nullrhi` headless mode. Policy B escape hatch: if `headless + compile_attempted + emitters > 0`, save with WARNING instead of failing. Editor mode remains strict (fail on not-ready). **RESULT Footer:** Commandlet emits machine-parseable footers: `RESULT: New=<N> Skipped=<N> Failed=<N> Deferred=<N> Total=<N> Headless=<true/false>` (schema v1, fixed order, all keys always present) and `RESULT_HEADLESS_SAVED: Count=<N>` (only if Count > 0). Deferred = skipped due to missing prerequisite, does NOT count as success. **Explicit Contract:** Editor = readiness guaranteed; Headless = authoring artifacts requiring editor validation before shipping. **Hash Updates:** `FManifestNiagaraEmitterOverride::ComputeHash()` includes all four enable fields. See `ClaudeContext/Handoffs/VFX_System_Reference.md`.
- v4.10 - Widget Property Enhancement & Material Validation: **Widget Property Enhancement:** ConfigureWidgetProperties lambda now supports 1-level dotted properties (Font.Size, Brush.ResourceObject), struct types (FSlateColor, FSlateBrush, FLinearColor, FVector2D), and enum types (FEnumProperty, TEnumAsByte via GetIntPropertyEnum). FSlateBrush field restrictions allow only ResourceObject, ImageSize, TintColor. MediaTexture blocked without MediaAssets dependency. SlateTextureAtlasInterface validated via ImplementsInterface. FVector2D shim converts "64, 64" to "X=64 Y=64" format. New `TArray<FString> Warnings` field in FGenerationResult for machine-readable warnings (pipe-delimited: CODE | ContextPath | Message | SuggestedFix). FromGenerationResult() splits E_*/W_* prefixes into Errors/Warnings arrays (CRITICAL: CullEmpty=false preserves empty SuggestedFix). Warning codes: W_UNSUPPORTED_TYPE, W_PROPERTY_NOT_FOUND, W_NESTED_TOO_DEEP, W_BRUSH_FIELD_BLOCKED, W_TEXTURE_TYPE_BLOCKED, W_STRUCT_INIT_FAILED, W_ENUM_VALUE_INVALID. **Material Expression Validation:** Hallucination-proof validation system with 6 guardrails. New types: EMaterialExprValidationCode enum (11 error codes), FMaterialExprDiagnostic struct, FMaterialExprValidationResult. **6 Guardrails:** (1) Quote Strip Before Trim, (2) Case Hint on User Path, (3) ContextPath Root Convention, (4) Normalize Once Reuse Everywhere, (5) Field-Specific Normalizers, (6) Stable Diagnostic Ordering. **3-Pass Algorithm:** Pass 1 validates expressions, Pass 2 validates connections, Pass 3 warns on unused. Error codes: E_UNKNOWN_EXPRESSION_TYPE (97 types), E_UNKNOWN_SWITCH_KEY, E_FUNCTION_NOT_FOUND, E_FUNCTION_INSTANCE_BLOCKED, E_MISSING_REQUIRED_INPUT, E_DUPLICATE_EXPRESSION_ID, E_EXPRESSION_REFERENCE_INVALID, W_DEPRECATED_SWITCH_KEY, W_FUNCTION_PATH_NORMALIZED, W_EXPRESSION_UNUSED. See `ClaudeContext/Handoffs/VFX_System_Reference.md`.
- v4.9.1 - MIC Session Cache & Whitelist Alignment: Fixed Material Instance Constant (MIC) generation failing when parent materials were created in the same session. Added session-level TMap cache to FMaterialGenerator that stores generated materials by name. MIC generator now checks session cache first before FindObject/LoadObject fallback chain. Cache cleared at start of each generation session (commandlet and editor window). Fixed whitelist function alignment: removed FXPresets from expected counts (config data applied to NiagaraSystems, not standalone assets), added missing arrays (GameplayCues, CharacterAppearances, MaterialInstances) to GetExpectedAssetNames and GetTotalAssetCount. Verification system now catches bidirectional leaks: assets expected but not generated, or generated but not expected. All 32 generators properly tracked with 155/155 verification passing. See `ClaudeContext/Handoffs/VFX_System_Reference.md`.
- v4.9 - VFX System Enhancements: Comprehensive VFX automation improvements. (1) **Material Instance Constants** (MIC_): New FMaterialInstanceGenerator creates UMaterialInstanceConstant assets from parent materials with scalar, vector, and texture parameter overrides. New manifest section `material_instances:` with `parent_material`, `scalar_params`, `vector_params`, `texture_params` support. (2) **VFX Material Expressions**: Added 20+ VFX-specific expression types to FMaterialGenerator including ParticleColor, ParticleSubUV, DynamicParameter, VertexColor, DepthFade, SphereMask, CameraPositionWS, ObjectBounds, Saturate, Abs, Cosine, Floor, Frac, Normalize, DotProduct, CrossProduct, Distance, WorldPosition, PixelDepth, SceneDepth. (3) **Emitter-Specific Overrides**: Niagara systems now support per-emitter parameter overrides via new `emitter_overrides:` section with `emitter`, `enabled`, and `parameters` map. Allows disabling emitters or setting emitter-specific User.* values without duplicating template systems. (4) **FX Preset Library**: New `fx_presets:` manifest section defines reusable Niagara parameter configurations. Presets can inherit from other presets via `base_preset:` field. Niagara systems reference presets via `preset:` field; preset parameters are merged with user_parameters (manifest user_parameters override preset values). (5) **LOD/Scalability Settings**: Niagara systems now support `cull_distance`, `cull_distance_low/medium/high/epic/cinematic`, `significance_distance`, `max_particle_budget`, `scalability_mode`, `allow_culling_for_local_players` for automatic LOD configuration. New struct FManifestFXPresetDefinition and FManifestNiagaraEmitterOverride with ComputeHash support for regen safety.
- v4.8 - Quest and Item Table Editors: Two new table editors following NPC/Dialogue patterns. Quest Table Editor (SQuestTableEditor) with 12 columns, quest grouping, state machine view, token-based Tasks/Events/Conditions/Rewards columns. Item Table Editor (SItemTableEditor) with 16 columns, dynamic visibility based on ItemType, type-specific fields (AttackRating for weapons, ArmorRating for armor, WeaponConfig for ranged). Both editors include validation cache, XLSX 3-way sync, soft delete, generation tracking, re-entrancy guards. Replaces old SQuestEditor test code. New files: QuestTableEditor/ and ItemTableEditor/ directories with converters, validators, and editor widgets. See `ClaudeContext/Handoffs/v4.8_Coverage_Expansion_Handoff.md`.
- v4.7 - Machine-Readable Report System: UGenerationReport UDataAsset + JSON mirror for CI/CD and debugging. FGenerationReportItem with assetPath, assetName, generatorId, executedStatus. FGenerationError with errorCode, contextPath, message, suggestedFix. Reports saved to `/Game/Generated/Reports/` and `Saved/GasAbilityGenerator/Reports/`. AssetPath and GeneratorId populated in all 32+ generators. Supports real-run and dry-run reports. See `ClaudeContext/Handoffs/v4.7_Report_System_Reference.md`.

## v4.6.x and Earlier

- v4.6.1 - NPC Table Editor status bar and UX refinements. See `ClaudeContext/Handoffs/v4.6_UX_Safety_System_ProcessMap.md`.
- v4.6 - UX Safety System: Auto-save before generate, soft delete, validation gate, generation tracking, hash-based staleness detection. See `ClaudeContext/Handoffs/v4.6_UX_Safety_System_ProcessMap.md`.
- v4.5.x - Validation cache system, XLSX-only format, NPC/Dialogue editor alignment. See `ClaudeContext/Handoffs/Table_Editors_Reference.md`.
- v4.4.x - Validated Token System with preview window, Apply to Assets, Sync from Assets. See `ClaudeContext/Handoffs/Table_Editors_Reference.md`.
- v4.3 - Widget Blueprint Layout Automation: FWidgetBlueprintGenerator now supports full widget tree construction from YAML via `widget_tree` manifest section. New structs: FManifestWidgetSlotDefinition (anchors, position, size, alignment, h_align, v_align, size_rule, fill_weight, padding), FManifestWidgetNodeDefinition (id, type, name, is_variable, slot, properties, children, text, image_path, style_class), FManifestWidgetTreeDefinition (root_widget, widgets array). Supports 23 widget types: CanvasPanel, VerticalBox, HorizontalBox, Overlay, GridPanel, UniformGridPanel, WrapBox, ScrollBox, Border, SizeBox, ScaleBox, Button, CheckBox, Slider, ComboBox, EditableText, EditableTextBox, TextBlock, RichTextBlock, Image, ProgressBar, Throbber, CircularThrobber, Spacer. Three-pass construction: create widgets, build hierarchy via panel AddChild, set root. Slot configuration supports Canvas/VBox/HBox/Overlay panels with proper UCanvasPanelSlot/UVerticalBoxSlot/UHorizontalBoxSlot/UOverlaySlot population. Properties set via reflection. Upgrades WBP_ from Medium to High automation level.
- v4.0 - Quest Pipeline & CSV Dialogue: New `-dialoguecsv="..."` commandlet parameter for batch dialogue generation from Excel/CSV files. FDialogueCSVParser class parses CSV with columns: Dialogue, NodeID, Type, Speaker, Text, OptionText, Replies, Conditions, Events. Supports multiple dialogues per file (grouped by Dialogue column), automatic root node detection, NPC/Player node type resolution, reply connection validation, event/condition parsing (NE_*/NC_* format). CSV dialogues override YAML definitions with same name. Enables production-scale dialogue authoring via spreadsheets. Full v3.0 Regen/Diff Safety integration. Sample data: `ClaudeContext/DialogueData.csv`.
- v3.9.9 - POI & NPC Spawner Automation: Level actor placement for World Partition worlds. New `-level="/Game/..."` commandlet parameter loads a world for actor placement. FPOIPlacementGenerator places APOIActor instances with POITag, DisplayName, MapIcon, LinkedPOIs (A* navigation). FNPCSpawnerPlacementGenerator places ANPCSpawner actors with UNPCSpawnComponent entries, supporting NearPOI resolution and spawn parameter configuration. New manifest sections: `poi_placements:` (poi_tag, location, rotation, display_name, map_icon, linked_pois) and `npc_spawner_placements:` (name, location/near_poi, npcs array with npc_definition, spawn_params, optional_goal). Idempotency via actor label/POI tag matching. World auto-saved after new placements.
- v3.9.8 - Item Pipeline: Mesh-to-Item automation with clothing mesh support for auto-generating EI_ assets from mesh files
- v3.9.7 - Parser & Manifest Fixes: Fixed ParseMaterialFunctions multi-item parsing bug where subsection flags (bInExpressions, etc.) prevented subsequent items from being recognized. Added indent-based detection to properly identify new items regardless of subsection state. Fixed manifest BP_FatherRifleWeapon/BP_FatherSwordWeapon prefix to EI_ (RangedWeaponItem/MeleeWeaponItem are UObject data assets, not Actors). Consolidated 33 prefixed manifest sections (warden_tags:, guard_formation_goals:, etc.) into standard section names with comments. Increases parsed asset count from 151 to 155.
- v3.9.6 - Automation Gaps Implementation: (1) Quest Rewards & Questgiver - `FManifestQuestRewardDefinition` struct with Currency/XP/Items, `questgiver` (documentation field - UQuest has no Questgiver property; the actual questgiver is the NPC whose dialogue has a `start_quest` event) and `rewards` fields on quest definitions, rewards auto-created on success states using NE_GiveXP/BPE_AddCurrency/BPE_AddItemToInventory events. (2) Dialogue Quest Shortcuts - `start_quest`, `complete_quest_branch`, `fail_quest` fields on dialogue nodes auto-generate NE_BeginQuest/NE_CompleteQuestBranch/NE_FailQuest events. (3) Item Usage Properties - `bAddDefaultUseOption`, `bConsumeOnUse`, `bUsedWithOtherItem`, `UseActionText`, `bCanActivate`, `bToggleActiveOnUse`, `UseSound` for NarrativeItem configuration. (4) Weapon Attachments - `holster_attachments`/`wield_attachments` arrays with slot/socket/offset/rotation entries (logged for manual TMap setup due to complex nested structure).
- v3.9.5 - NPC Item Loadout Automation: Full loot table roll support for NPCDefinitions. New manifest properties `default_item_loadout` and `trading_item_loadout` accept arrays of loot table rolls, each containing `items` (item class + quantity), `item_collections`, `table_to_roll`, `num_rolls`, and `chance`. Maps to Narrative Pro's `TArray<FLootTableRoll>` via reflection with proper population of nested `FItemWithQuantity` and item collection arrays. Enables complete NPC inventory definition from YAML.
- v3.9.4 - Quest Blueprint Generator (UQuestBlueprint): Complete rewrite using UQuestBlueprint (same pattern as DialogueBlueprint v3.8). Works directly on QuestTemplate (auto-created by constructor). Branches now nested inside states (cleaner YAML structure). Events supported on both states and branches. Task argument resolution via reflection. Added NarrativeQuestEditor module dependency. Visual editor shows empty graph but quest functions at runtime.
- v3.9.3 - Quest State Machine Generator: FQuestGenerator now creates complete quest state machines with UQuestState nodes, UQuestBranch transitions, and UNarrativeTask instances. States support Regular/Success/Failure types. Branches connect states and contain instanced tasks (BPT_FindItem, BPT_FinishDialogue, BPT_Move, etc.). Task properties set via reflection. Full automation - no manual editor setup required for basic quest flows.
- v3.9.2 - Goal Search Paths: Added all Narrative Pro goal locations to FActivityScheduleGenerator search paths (Attacks/Goals/, DriveToDestination/, FollowCharacter/, GoToLocation/, Idle/, Interact/Goals/, Patrol/, ReturnToSpawn/).
- v3.9.1 - Schedule Behavior Helper: Added UScheduledBehavior_AddNPCGoalByClass concrete helper class allowing goal class specification via property instead of Blueprint override. FActivityScheduleGenerator now creates fully functional scheduled behaviors.
- v3.9 - NPC Pipeline (Schedules, Goals, Quests): Three new generators for comprehensive NPC content creation. FActivityScheduleGenerator creates Schedule_ assets (UNPCActivitySchedule DataAssets) with fully populated scheduled behaviors. Time format: 0-2400 where 100 = 1 hour. FGoalItemGenerator creates Goal_ assets (UNPCGoalItem Blueprints) for AI objectives with DefaultScore, GoalLifetime, RemoveOnSucceeded, SaveGoal, OwnedTags, BlockTags, RequireTags. FQuestGenerator creates Quest_ assets (UQuest Blueprints) with full state machine support. New manifest sections: activity_schedules, goal_items/goals, quests. Full v3.0 Regen/Diff Safety System integration.
- v3.8 - Dialogue Tree Generation: DialogueBlueprint (DBP_) now supports full dialogue tree creation from YAML. New manifest property `dialogue_tree` with `root` and `nodes` array. Each node supports: id, type (npc/player), speaker, text, option_text, audio, montage, duration, duration_seconds, auto_select, auto_select_if_only, skippable, directed_at, npc_replies[], player_replies[], alternative_lines[], events[], conditions[]. Events support type, runtime (Start/End/Both), and properties map. Conditions support type, not (invert), and properties map. Creates UDialogueBlueprint with proper DialogueTemplate containing UDialogueNode_NPC/UDialogueNode_Player nodes and FDialogueLine data. Full v3.0 Regen/Diff Safety System integration. Upgrades DBP_ to High automation level with full editor compatibility.
- v3.7 - NPC Auto-Create Related Assets: NPCDefinition gains auto_create_dialogue, auto_create_tagged_dialogue, and auto_create_item_loadout flags for one-manifest NPC package generation. When enabled: auto_create_dialogue creates DBP_{NPCName}Dialogue, auto_create_tagged_dialogue creates {NPCName}_TaggedDialogue, auto_create_item_loadout populates DefaultItemLoadout.ItemCollectionsToGrant with specified item collections. New manifest field default_item_loadout_collections for specifying item collections to grant. All v3.0 Regen/Diff Safety System hash safeguards included via updated ComputeHash().
- v3.6 - NPCDefinition ActivitySchedules: NPCDefinition (NPC_) gains ActivitySchedules array support (TArray<TSoftObjectPtr<UNPCActivitySchedule>>) for defining NPC daily routines. Also adds YAML list parsing support for DefaultOwnedTags, DefaultFactions, and ActivitySchedules arrays in npc_definitions.
- v3.5 - CharacterDefinition & DialogueBlueprint Enhancement: CharacterDefinition (CD_) gains full property support - DefaultOwnedTags/DefaultFactions now properly populate FGameplayTagContainer from arrays, added DefaultAppearance (TSoftObjectPtr), TriggerSets (array via FScriptArrayHelper reflection), AbilityConfiguration (FObjectProperty). DialogueBlueprint gains 3 additional UDialogue properties - DefaultHeadBoneName (FName), DialogueBlendOutTime (float), bAdjustPlayerTransform (bool). Both CD_ and DBP_ upgraded to High automation level.

## v3.4 and Earlier

- v3.4 - WeaponItem Property Support: EquippableItem generator now supports full WeaponItem/MeleeWeaponItem/RangedWeaponItem property chain when using those parent classes. New properties: WeaponVisualClass (TSoftClassPtr), WeaponHand (EWeaponHandRule enum), WeaponAbilities/MainhandAbilities/OffhandAbilities arrays (TSubclassOf<UGameplayAbility>), bPawnFollowsControlRotation, bPawnOrientsRotationToMovement, AttackDamage, HeavyAttackDamageMultiplier, bAllowManualReload, RequiredAmmo, bBotsConsumeAmmo, BotAttackRange, ClipSize, AimFOVPct, BaseSpreadDegrees, MaxSpreadDegrees, SpreadFireBump, SpreadDecreaseSpeed. Enables complete weapon definition from YAML.
- v3.3 - NPCDefinition, EquippableItem & Activity Enhancement: NPCDefinition gains full CharacterDefinition property support (Dialogue, TaggedDialogueSet, vendor properties, inherited properties). EquippableItem gains full NarrativeItem property support (DisplayName, Description, AttackRating, ArmorRating, StealthRating, Weight, BaseValue, BaseScore, ItemTags, bStackable, MaxStackSize, UseRechargeDuration, Thumbnail). Activity gains full NarrativeActivityBase/NPCActivity property support (ActivityName, OwnedTags, BlockTags, RequireTags, SupportedGoalType, bIsInterruptable, bSaveActivity). EI_, NPCDef_, and BPA_ upgraded to High automation level.
- v3.2 - NE_ & DBP_ Enhancements: NarrativeEvent now sets EventRuntime/EventFilter/PartyEventPolicy/bRefireOnLoad via reflection, DialogueBlueprint sets bFreeMovement/bUnskippable/bCanBeExited/bShowCinematicBars/bAutoRotateSpeakers/bAutoStopMovement/Priority/EndDialogueDist via reflection, both types upgraded from Stub to Medium automation level
- v3.1.1 - Robustness Fixes: Commandlet exit code returns non-zero on failures (CI/CD support), dedicated LogGasAbilityGenerator log category, try/catch for YAML parsing exceptions, UGeneratorMetadataRegistry fallback for UDataAsset/UBlueprint/UNiagaraSystem (which don't support IInterface_AssetUserData in UE5.7)
- v3.1 - TSubclassOf Resolution & BT Node Trees: AbilityConfiguration now populates DefaultAbilities/StartupEffects/DefaultAttributes via LoadClass<>, ActivityConfiguration populates DefaultActivities/GoalGenerators, BehaviorTree creates root composite node (Selector/Sequence) and populates Children with tasks/decorators/services
- v3.0 - Regen/Diff Safety System: Per-asset metadata via UGeneratorAssetMetadata (UAssetUserData), input hash (manifest definition) + output hash (asset content) change detection, --dryrun preview mode, --force conflict override, universal SkipIfModified policy for all 25 generators, ComputeDataAssetOutputHash for 17 DataAsset types, ComputeBlueprintOutputHash for 9 Blueprint types
- v2.9.1 - FX Validation System: Template integrity validation, descriptor-to-system validation, regeneration safety with metadata tracking and descriptor hashing
- v2.9.0 - Data-driven FX architecture: FManifestFXDescriptor for Niagara User param binding, template duplication workflow
- v2.8.4 - Whitelist-based verification system with duplicate detection for fresh generations
- v2.8.3 - Function override support for event graphs (e.g., ReceiveTick override)
- v2.8.2 - CallFunction parameter defaults with enum conversion (e.g., SetMovementMode NewMovementMode: Flying)
- v2.8.1 - NPC event_graphs with descriptive messages and Self node fix
- v2.8.0 - Flying NPC event_graphs for BP_WardenCore and BP_PossessedExploder
- v2.7.8 - Self node support for event graphs
- v2.7.7 - Pre-generation validation for event graph definitions
- v2.7.6 - Inline event graph support for actor blueprints
- v2.7.0 - BreakStruct, MakeArray, GetArrayItem node support for weapon form implementation
- v2.6.14 - Prefix validation for all asset types
- v2.6.12 - Material expression graph support, Material Function generator
- v2.6.11 - Force scan NarrativePro plugin content for Blueprint parent class resolution in commandlet mode
- v2.6.7 - Deferred asset retry mechanism for dependency resolution
- v2.6.6 - GE assets created as Blueprints for CooldownGameplayEffectClass compatibility
- v2.6.5 - Niagara System generator
- v2.6.3 - Tags configured on existing GA assets during SKIP
- v2.6.2 - Gameplay ability tag/policy configuration via reflection
- v2.6.0 - Item collections with quantities, reflection for protected members
- v2.5.7 - TaggedDialogueSet generator
- v2.5.0 - Renamed to GasAbilityGenerator, FManifest* prefix convention
- v2.4.0 - Inline event graph and variables support for gameplay abilities
- v2.3.0 - 12 new asset type generators with dependency-based generation order
- v2.2.0 - Event graph generation (Blueprint nodes and connections from YAML)
- v2.1.9 - Manifest validation (whitelist enforcement)
- v2.1.8 - Enumeration generation, enum variable type fix
