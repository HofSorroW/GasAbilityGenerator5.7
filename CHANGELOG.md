# GasAbilityGenerator Changelog

Full version history for versions prior to v3.5. For recent versions, see `CLAUDE.md`.

---

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
