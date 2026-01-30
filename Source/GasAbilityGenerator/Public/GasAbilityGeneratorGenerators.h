// GasAbilityGenerator v3.0
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v3.0: Regen/Diff Safety System - metadata tracking, dry run mode, hash-based change detection
// v2.8.3: Function override support for parent class functions (HandleDeath, etc.)
// v2.8.2: CallFunction parameter defaults - applies "param.*" properties to function input pins
// v2.7.0: Added BreakStruct, MakeArray, GetArrayItem node support for weapon form implementation
// v2.6.14: Prefix validation for all asset types (E_, IA_, GA_, BP_, WBP_, M_, MF_, NS_, etc.)
// v2.6.8: EquippableItemGenerator now uses ParentClass from manifest (RangedWeaponItem, MeleeWeaponItem support)
// v2.6.7: Deferred asset retry mechanism for dependency resolution
// v2.6.6: GE assets created as Blueprints for CooldownGameplayEffectClass compatibility
// v2.6.5: Added Niagara System generator
// v2.5.0: Renamed to GasAbilityGenerator for generic UE project compatibility
// v2.4.0: Added inline event graph and variables support for gameplay ability generation
// v2.3.0: Added 12 new asset type generators with dependency-based generation order
// v2.2.0: Added event graph generator - creates Blueprint nodes and connections
// v2.1.9: Added manifest validation - ValidateAgainstManifest() and SetActiveManifest()
// v2.1.8: Added enumeration generator and FindUserDefinedEnum helper

#pragma once

#include "CoreMinimal.h"
#include "Locked/GasAbilityGeneratorTypes.h"
#include "Locked/GasAbilityGeneratorMetadata.h"

// Forward declarations
class UBlueprint;
class UGameplayEffect;
class UGameplayAbility;
class UInputAction;
class UInputMappingContext;
class UBlackboardData;
class UBehaviorTree;
class UMaterial;
class UWidgetBlueprint;
class UWidgetBlueprintFactory;
class UUserDefinedEnum;
class UK2Node;
class UEdGraph;
class UEdGraphPin;

/**
 * v7.8.31: Result of pin connection attempt
 * Distinguishes between actual connections and skipped pure-node exec connections
 */
enum class EConnectResult : uint8
{
	Connected,     // Actual connection was made
	SkippedPure,   // Exec-to-pure connection was skipped (expected behavior)
	Failed         // Connection failed (error)
};

/**
 * v7.5.5: Callback type for generator log messages
 * Used to bridge LogGeneration() with commandlet's LogMessages array
 */
typedef TFunction<void(const FString&)> FGeneratorLogCallback;

/**
 * Base generator class - all generators inherit from this
 */
class GASABILITYGENERATOR_API FGeneratorBase
{
public:
	virtual ~FGeneratorBase() = default;

	/**
	 * v7.5.5: Register a callback to receive LogGeneration messages
	 * The commandlet uses this to capture generator logs in its output file
	 */
	static void SetLogCallback(FGeneratorLogCallback InCallback);

	/**
	 * v7.5.5: Clear the log callback (called when commandlet finishes)
	 */
	static void ClearLogCallback();

	/**
	 * Set the active manifest for validation
	 */
	static void SetActiveManifest(const FManifestData* Manifest);

	/**
	 * Clear the active manifest (disables validation)
	 */
	static void ClearActiveManifest();

	/**
	 * Validate an asset name against the active manifest whitelist
	 */
	static bool ValidateAgainstManifest(
		const FString& AssetName,
		const FString& AssetType,
		FGenerationResult& OutResult);

	/**
	 * Check if an asset already exists on disk
	 */
	static bool DoesAssetExistOnDisk(const FString& AssetPath);

	/**
	 * Check if an asset package is already loaded in memory
	 */
	static bool IsAssetInMemory(const FString& AssetPath);

	/**
	 * Combined existence check - returns early with SKIPPED status if asset exists
	 */
	static bool CheckExistsAndPopulateResult(
		const FString& AssetPath,
		const FString& AssetName,
		const FString& AssetType,
		FGenerationResult& OutResult);

	/**
	 * v2.6.2: Configure gameplay ability tag containers on CDO using reflection
	 */
	static void ConfigureGameplayAbilityTags(
		UGameplayAbility* AbilityCDO,
		UBlueprint* Blueprint,
		const FManifestGameplayAbilityDefinition& Definition);

	/**
	 * v2.6.2: Configure gameplay ability policies on CDO using reflection
	 */
	static void ConfigureGameplayAbilityPolicies(
		UGameplayAbility* AbilityCDO,
		UBlueprint* Blueprint,
		const FManifestGameplayAbilityDefinition& Definition);

	/**
	 * Get the project's Content folder path
	 */
	static FString GetContentPath();

	/**
	 * Find a parent class by name, searching multiple modules
	 */
	static UClass* FindParentClass(const FString& ClassName);

	/**
	 * v4.28: Find fragment class by name for Item Fragments system
	 * Per Item_Generation_Capability_Audit.md: Fragment allowlist is AmmoFragment, PoisonableFragment
	 * Class path format: /Script/NarrativeArsenal.<ClassWithoutUPrefix>
	 */
	static UClass* FindFragmentClass(const FString& FragmentClassName);

	/**
	 * v4.28: Clear fragment warning deduplication cache (called at start of generation session)
	 */
	static void ClearFragmentWarningCache();

	/**
	 * Find a user-defined enum by name, searching project content
	 */
	static UEnum* FindUserDefinedEnum(const FString& EnumName, const FString& ProjectRoot);

	// ====================================================================
	// v2.9.1: Generalized Validation Utilities
	// ====================================================================

	/**
	 * Validate that a required string field is not empty
	 * @return true if valid, false if empty (adds error to result)
	 */
	static bool ValidateRequiredField(const FString& AssetName, const FString& FieldName,
		const FString& Value, FPreValidationResult& OutResult);

	/**
	 * Validate that a referenced asset exists (by path or common search paths)
	 * @return true if found, false if not found (adds error to result)
	 */
	static bool ValidateAssetReference(const FString& AssetName, const FString& FieldName,
		const FString& ReferencePath, FPreValidationResult& OutResult);

	/**
	 * Validate parent class exists and is valid for the asset type
	 * @return true if found, false if not found (adds error to result)
	 */
	static bool ValidateParentClass(const FString& AssetName, const FString& ClassName,
		FPreValidationResult& OutResult);

	/**
	 * Validate a float value is within expected range
	 * @return true if in range, adds warning if out of range
	 */
	static bool ValidateFloatRange(const FString& AssetName, const FString& FieldName,
		float Value, float Min, float Max, FPreValidationResult& OutResult);

	/**
	 * Validate asset name follows expected prefix convention
	 * @return true if valid, adds warning if invalid
	 */
	static bool ValidateNamePrefix(const FString& AssetName, const FString& ExpectedPrefix,
		FPreValidationResult& OutResult);

	/**
	 * Validate an array is not empty when required
	 * @return true if not empty or not required, adds error if required and empty
	 */
	static bool ValidateNonEmptyArray(const FString& AssetName, const FString& FieldName,
		int32 ArraySize, bool bRequired, FPreValidationResult& OutResult);

	/**
	 * Run validation and return result - combines all validation in one call
	 * Logs all issues automatically
	 * @return FGenerationResult with Failed status if validation has errors
	 */
	static bool HandleValidationResult(const FPreValidationResult& Validation,
		const FString& AssetName, FGenerationResult& OutResult);

	// ====================================================================
	// v3.0: Regen/Diff Safety - Metadata and Dry Run Support
	// ====================================================================

	/**
	 * Store generator metadata on an asset for regeneration tracking
	 * @param Asset The generated asset (UBlueprint, UDataAsset, etc.)
	 * @param Metadata The metadata to store
	 */
	static void StoreAssetMetadata(UObject* Asset, const FGeneratorMetadata& Metadata);

	/**
	 * Retrieve generator metadata from an asset
	 * @param Asset The asset to query
	 * @return Pointer to metadata if found, nullptr otherwise
	 */
	static UGeneratorAssetMetadata* GetAssetMetadata(UObject* Asset);

	/**
	 * Compute output hash for a Blueprint asset (for manual edit detection)
	 * Hashes node count, node types, variable count, and connection count
	 * @param Blueprint The blueprint to hash
	 * @return Output hash value
	 */
	static uint64 ComputeBlueprintOutputHash(UBlueprint* Blueprint);

	/**
	 * Compute output hash for a DataAsset (for manual edit detection)
	 * @param DataAsset The data asset to hash
	 * @return Output hash value
	 */
	static uint64 ComputeDataAssetOutputHash(UObject* DataAsset);

	/**
	 * Enable/disable dry run mode
	 * In dry run mode, generators compute what would happen without making changes
	 */
	static void SetDryRunMode(bool bEnabled);

	/**
	 * Check if dry run mode is active
	 */
	static bool IsDryRunMode();

	/**
	 * Enable/disable force regeneration mode
	 * In force mode, generators overwrite even if conflicts are detected
	 */
	static void SetForceMode(bool bEnabled);

	/**
	 * Check if force regeneration mode is active
	 */
	static bool IsForceMode();

	/**
	 * Get the accumulated dry run results
	 */
	static const FDryRunSummary& GetDryRunSummary();

	/**
	 * Add a dry run result to the summary
	 */
	static void AddDryRunResult(const FDryRunResult& Result);

	/**
	 * Clear accumulated dry run results
	 */
	static void ClearDryRunSummary();

	/**
	 * Set the manifest path for metadata tracking
	 */
	static void SetManifestPath(const FString& Path);

	/**
	 * Get the current manifest path
	 */
	static const FString& GetManifestPath();

	/**
	 * v3.0: Enhanced existence check with metadata-aware logic
	 * - If asset doesn't exist: returns false (will create)
	 * - If asset exists with no metadata: returns true (skip - likely manual)
	 * - If asset exists with matching hash: returns true (skip - no changes)
	 * - If asset exists with changed hash but no manual edits: returns false (will modify)
	 * - If asset exists with changed hash AND manual edits: returns true (conflict - skip unless force)
	 *
	 * @param AssetPath Path to check
	 * @param AssetName Name of the asset
	 * @param AssetType Type for logging
	 * @param InputHash Hash of current manifest definition
	 * @param OutResult Result to populate
	 * @param OutDryRunStatus Optional dry run status for preview
	 * @return true if should skip generation, false if should proceed
	 */
	static bool CheckExistsWithMetadata(
		const FString& AssetPath,
		const FString& AssetName,
		const FString& AssetType,
		uint64 InputHash,
		FGenerationResult& OutResult,
		EDryRunStatus* OutDryRunStatus = nullptr);

protected:
	/**
	 * Log generation message to editor output
	 */
	static void LogGeneration(const FString& Message);

	/**
	 * v4.40: Safe package save with return value check
	 * Logs errors if save fails and returns success status
	 * @param Package The package to save
	 * @param Asset The asset being saved (for error reporting)
	 * @param PackageFileName The file path to save to
	 * @param AssetName Human-readable name for error messages
	 * @return true if save succeeded, false if failed
	 */
	static bool SafeSavePackage(UPackage* Package, UObject* Asset, const FString& PackageFileName, const FString& AssetName);

	/**
	 * v4.22: Get the active manifest for reference checking
	 * Used to determine if a referenced asset is defined in the manifest (vs external plugin)
	 */
	static const FManifestData* GetActiveManifest() { return ActiveManifest; }

private:
	static const FManifestData* ActiveManifest;

	// v3.0: Dry run and force mode state
	static bool bDryRunMode;
	static bool bForceMode;
	static FDryRunSummary DryRunSummary;
	static FString CurrentManifestPath;

	// v7.5.5: Log callback for commandlet integration
	static FGeneratorLogCallback LogCallback;
};

/**
 * Enumeration Generator - creates UserDefinedEnum assets
 */
class GASABILITYGENERATOR_API FEnumerationGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestEnumerationDefinition& Definition);
};

/**
 * Input Action Generator
 */
class GASABILITYGENERATOR_API FInputActionGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestInputActionDefinition& Definition);
};

/**
 * Input Mapping Context Generator
 */
class GASABILITYGENERATOR_API FInputMappingContextGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestInputMappingContextDefinition& Definition);
};

/**
 * Gameplay Effect Generator
 */
class GASABILITYGENERATOR_API FGameplayEffectGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestGameplayEffectDefinition& Definition);
};

/**
 * Gameplay Ability Generator
 */
class GASABILITYGENERATOR_API FGameplayAbilityGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(
		const FManifestGameplayAbilityDefinition& Definition,
		const FString& ProjectRoot = TEXT(""));

	/**
	 * v4.13: Category C - P1.2 Transition Prelude Validation
	 * Validates that form abilities follow the Option B pattern:
	 * - Required: Father.State.Alive, Father.State.Recruited in activation_required_tags
	 * - Required: Father.State.Dormant in activation_blocked_tags
	 * - Warning: Father.State.Transitioning should be in activation_blocked_tags
	 * - Warning: cancel_abilities_with_tag should not be empty
	 * @param Definition The ability definition to validate
	 * @param OutWarnings Warnings array to populate (pipe-delimited: CODE | ContextPath | Message | SuggestedFix)
	 */
	static void ValidateFormAbility(const FManifestGameplayAbilityDefinition& Definition, TArray<FString>& OutWarnings);
};

/**
 * Actor Blueprint Generator
 */
class GASABILITYGENERATOR_API FActorBlueprintGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(
		const FManifestActorBlueprintDefinition& Definition,
		const FString& ProjectRoot = TEXT(""),
		const FManifestData* ManifestData = nullptr);
};

/**
 * Widget Blueprint Generator
 */
class GASABILITYGENERATOR_API FWidgetBlueprintGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(
		const FManifestWidgetBlueprintDefinition& Definition,
		const FManifestData* ManifestData = nullptr);
};

/**
 * v4.19: Component Blueprint Generator
 * Generates UActorComponent-derived blueprints with variables, event dispatchers, functions, and tick configuration
 */
class GASABILITYGENERATOR_API FComponentBlueprintGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(
		const FManifestComponentBlueprintDefinition& Definition,
		const FString& ProjectRoot = TEXT(""));
};

/**
 * Blackboard Generator
 * v4.14: Added session cache for BT generator to find same-session blackboards
 */
class GASABILITYGENERATOR_API FBlackboardGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestBlackboardDefinition& Definition);

	// v4.14: Session-level lookup for generated blackboards (used by BT generator)
	static UBlackboardData* FindGeneratedBlackboard(const FString& BlackboardName);
	static void RegisterGeneratedBlackboard(const FString& BlackboardName, UBlackboardData* Blackboard);
	static void ClearGeneratedBlackboardsCache();
	static int32 GetCacheSize() { return GeneratedBlackboardsCache.Num(); }

private:
	static TMap<FString, UBlackboardData*> GeneratedBlackboardsCache;
};

/**
 * Behavior Tree Generator
 */
class GASABILITYGENERATOR_API FBehaviorTreeGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestBehaviorTreeDefinition& Definition);
};

/**
 * v2.6.12: Enhanced Material Generator with expression graph support
 * v4.9.1: Added session map for MIC parent material lookup
 * v4.10: Added material function cache, switch expressions, validation system
 */
class GASABILITYGENERATOR_API FMaterialGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestMaterialDefinition& Definition);

	// v4.9.1: Session-level lookup for generated materials (accessible by MIC generator)
	static UMaterialInterface* FindGeneratedMaterial(const FString& MaterialName);
	static void RegisterGeneratedMaterial(const FString& MaterialName, UMaterialInterface* Material);
	static void ClearGeneratedMaterialsCache();

	// v4.10: Session-level lookup for generated material functions
	static class UMaterialFunctionInterface* FindGeneratedMaterialFunction(const FString& FunctionName);
	static void RegisterGeneratedMaterialFunction(const FString& FunctionName, class UMaterialFunctionInterface* Function);
	static void ClearGeneratedMaterialFunctionsCache();

	// v4.10: 3-tier material function resolution
	// 1. /Engine/... - load directly from engine
	// 2. /Game/... - load directly from project
	// 3. MF_Name - search session cache then project paths
	static class UMaterialFunctionInterface* ResolveMaterialFunction(const FString& FunctionPath);

	// v4.10: Helper to connect switch expression inputs (public for FMaterialFunctionGenerator access)
	static bool ConnectSwitchInputs(UMaterialExpression* SwitchExpr, const FManifestMaterialExpression& ExprDef, const TMap<FString, UMaterialExpression*>& ExpressionMap);
	// v4.10: Helper to connect material function inputs (public for FMaterialFunctionGenerator access)
	static bool ConnectFunctionInputs(class UMaterialExpressionMaterialFunctionCall* FuncCall, const FManifestMaterialExpression& ExprDef, const TMap<FString, UMaterialExpression*>& ExpressionMap);

	// v4.10: Pre-generation validation with 6 guardrails
	// Validates expression types, switch keys, function references, duplicate IDs, connection validity
	static FMaterialExprValidationResult ValidateMaterialDefinition(const FManifestMaterialDefinition& Definition);

	// v4.10: Shared helper for validating expressions and connections arrays
	// Used by both FMaterialGenerator and FMaterialFunctionGenerator
	static FMaterialExprValidationResult ValidateExpressionsAndConnections(
		const FString& AssetName,
		const TArray<FManifestMaterialExpression>& Expressions,
		const TArray<FManifestMaterialConnection>& Connections);

	// v4.10.1: Overload for material functions that includes inputs/outputs as valid connection endpoints
	static FMaterialExprValidationResult ValidateExpressionsAndConnections(
		const FString& AssetName,
		const TArray<FManifestMaterialExpression>& Expressions,
		const TArray<FManifestMaterialConnection>& Connections,
		const TArray<FManifestMaterialFunctionInput>& Inputs,
		const TArray<FManifestMaterialFunctionOutput>& Outputs);

private:
	// v2.6.12: Helper to create material expression by type
	static UMaterialExpression* CreateExpression(UMaterial* Material, const FManifestMaterialExpression& ExprDef, const TMap<FString, UMaterialExpression*>& ExpressionMap);
	// v2.6.12: Helper to connect expressions
	static bool ConnectExpressions(UMaterial* Material, const TMap<FString, UMaterialExpression*>& ExpressionMap, const FManifestMaterialConnection& Connection);

	// v4.9.1: Session map for materials generated in current run
	static TMap<FString, UMaterialInterface*> GeneratedMaterialsCache;
	// v4.10: Session map for material functions generated in current run
	static TMap<FString, class UMaterialFunctionInterface*> GeneratedMaterialFunctionsCache;
};

/**
 * v2.6.12: Material Function Generator
 * v4.10: Now registers functions in session cache for MaterialFunctionCall resolution
 */
class GASABILITYGENERATOR_API FMaterialFunctionGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestMaterialFunctionDefinition& Definition);

private:
	// v2.6.12: Helper to create material expression in function
	static UMaterialExpression* CreateExpressionInFunction(class UMaterialFunction* MaterialFunction, const FManifestMaterialExpression& ExprDef, const TMap<FString, UMaterialExpression*>& ExpressionMap);
};

/**
 * v4.9: Material Instance Constant Generator - creates parameterized material instances
 */
class GASABILITYGENERATOR_API FMaterialInstanceGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestMaterialInstanceDefinition& Definition);
};

/**
 * Tag Generator - writes tags to DefaultGameplayTags.ini
 */
class GASABILITYGENERATOR_API FTagGenerator : public FGeneratorBase
{
public:
	static FGenerationSummary GenerateTags(const TArray<FString>& Tags, const FString& TagsIniPath);
};

/**
 * v2.6.7: Missing dependency info for deferred generation
 */
struct FMissingDependencyInfo
{
	FString DependencyName;  // e.g., "BP_FatherCompanion"
	FString DependencyType;  // e.g., "ActorBlueprint"
	FString Context;         // e.g., "PropertyGet node 'GetOwnerPlayer'"

	FMissingDependencyInfo() = default;
	FMissingDependencyInfo(const FString& InName, const FString& InType, const FString& InContext)
		: DependencyName(InName), DependencyType(InType), Context(InContext) {}
};

/**
 * Event Graph Generator - creates Blueprint nodes and connections from definitions
 * v2.7.7: Added pre-generation validation
 */
class GASABILITYGENERATOR_API FEventGraphGenerator : public FGeneratorBase
{
public:
	/**
	 * v2.7.7: Pre-generation validation for event graph definitions
	 * Returns true if validation passes, logs errors for issues found
	 */
	static bool ValidateEventGraph(
		const FManifestEventGraphDefinition& GraphDefinition,
		const FString& ContextName,
		TArray<FString>& OutErrors);

	/**
	 * v2.7.7: Validate actor blueprint event graph before generation
	 * Checks for NPC blueprints that should have event graphs
	 */
	static bool ValidateActorBlueprintEventGraph(
		const FManifestActorBlueprintDefinition& Definition,
		TArray<FString>& OutErrors);

	/**
	 * v4.40: Validate delegate bindings before generation
	 * Checks that delegate names are known and sources are valid variables
	 * @param DelegateBindings Array of delegate binding definitions to validate
	 * @param Variables Array of available variables to validate source references
	 * @param BlueprintName Name of the blueprint for error context
	 * @param OutErrors Output array for validation error messages
	 * @return true if all bindings are valid, false if any validation failed
	 */
	static bool ValidateDelegateBindings(
		const TArray<FManifestDelegateBindingDefinition>& DelegateBindings,
		const TArray<FManifestActorVariableDefinition>& Variables,
		const FString& BlueprintName,
		TArray<FString>& OutErrors);

	/**
	 * Generate event graph nodes and connections in a Blueprint
	 */
	static bool GenerateEventGraph(
		UBlueprint* Blueprint,
		const FManifestEventGraphDefinition& GraphDefinition,
		const FString& ProjectRoot);

	/**
	 * v2.8.3: Generate function override graph in a Blueprint
	 * Creates a function override with entry node, custom logic, and optional parent call
	 */
	static bool GenerateFunctionOverride(
		UBlueprint* Blueprint,
		const FManifestFunctionOverrideDefinition& OverrideDefinition,
		const FString& ProjectRoot);

	/**
	 * v4.14: Generate a new custom Blueprint function (not an override)
	 * Creates a function graph with UK2Node_FunctionEntry and UK2Node_FunctionResult
	 * @param Blueprint The blueprint to add the function to
	 * @param FunctionDefinition The function definition with inputs, outputs, nodes, and connections
	 * @param ProjectRoot Project root path for asset lookups
	 * @return true if function was created successfully
	 */
	static bool GenerateCustomFunction(
		UBlueprint* Blueprint,
		const FManifestCustomFunctionDefinition& FunctionDefinition,
		const FString& ProjectRoot);

	/**
	 * Find an event graph definition by name in manifest data
	 */
	static const FManifestEventGraphDefinition* FindEventGraphByName(
		const FManifestData& ManifestData,
		const FString& GraphName);

	/**
	 * v4.13: Category C - P3.2 GameplayCue Auto-Wiring
	 * Generates ExecuteGameplayCue nodes at specified trigger points in an ability's event graph.
	 * Trigger points: OnActivate (ActivateAbility event), OnEndAbility (K2_EndAbility), OnCommit (K2_CommitAbility)
	 * @param Blueprint The ability blueprint to modify
	 * @param CueTriggers Array of cue trigger definitions
	 * @return Number of cue trigger nodes successfully generated
	 */
	static int32 GenerateCueTriggerNodes(
		UBlueprint* Blueprint,
		const TArray<FManifestCueTriggerDefinition>& CueTriggers);

	/**
	 * v4.13: Category C - P3.1 Niagara Spawning Support
	 * Generates SpawnSystemAttached nodes for VFX with optional cleanup on EndAbility.
	 * @param Blueprint The ability blueprint to modify
	 * @param VFXSpawns Array of VFX spawn definitions
	 * @return Number of VFX spawn nodes successfully generated
	 */
	static int32 GenerateVFXSpawnNodes(
		UBlueprint* Blueprint,
		const TArray<FManifestVFXSpawnDefinition>& VFXSpawns);

	/**
	 * v4.13: Category C - P2.1 Delegate Binding IR + Codegen
	 * Generates custom event handlers and delegate binding nodes.
	 * @param Blueprint The ability blueprint to modify
	 * @param DelegateBindings Array of delegate binding definitions
	 * @return Number of delegate binding nodes successfully generated
	 */
	static int32 GenerateDelegateBindingNodes(
		UBlueprint* Blueprint,
		const TArray<FManifestDelegateBindingDefinition>& DelegateBindings);

	// v7.7: Track E REMOVED - RequiresNativeBridge, GenerateBridgeBasedBinding, GenerateBridgeBindingNodes deleted
	// GA_ abilities now use AbilityTasks (WaitAttributeChange, WaitGameplayEffectApplied) instead
	// See: ClaudeContext/Handoffs/Track_E_Removal_Audit_v7_7.md

	/**
	 * v4.33: Actor Blueprint Delegate Binding (External ASC Binding pattern)
	 * Generates delegate binding nodes for actor blueprints using BeginPlay/EndPlay.
	 * Implements Section 10 (External ASC Binding): Actor variable → GetAbilitySystemComponent → Cast to NASC → Bind.
	 * @param Blueprint The actor blueprint to modify
	 * @param DelegateBindings Array of delegate binding definitions
	 * @param Variables Array of actor variables (for resolving Actor-type sources)
	 * @return Number of delegate binding nodes successfully generated
	 */
	static int32 GenerateActorDelegateBindingNodes(
		UBlueprint* Blueprint,
		const TArray<FManifestDelegateBindingDefinition>& DelegateBindings,
		const TArray<FManifestActorVariableDefinition>& Variables);

	/**
	 * v4.22: Section 11 - Attribute Change Delegate Binding via AbilityTask
	 * Generates AbilityTask_WaitAttributeChange nodes with handler custom events.
	 * @param Blueprint The ability blueprint to modify
	 * @param AttributeBindings Array of attribute binding definitions
	 * @return Number of attribute binding tasks successfully generated
	 */
	static int32 GenerateAttributeBindingNodes(
		UBlueprint* Blueprint,
		const TArray<FManifestAttributeBindingDefinition>& AttributeBindings);

	// v2.6.7: Missing dependency tracking for deferred generation
	static const TArray<FMissingDependencyInfo>& GetMissingDependencies() { return MissingDependencies; }
	static bool HasMissingDependencies() { return MissingDependencies.Num() > 0; }
	static void ClearMissingDependencies() { MissingDependencies.Empty(); }

	/** v4.20.11: Re-apply stored node positions after compilation
	 *  Called after CompileBlueprint to restore positions that may have been lost
	 */
	static void ReapplyNodePositions(UBlueprint* Blueprint);

	/** v4.20.11: Clear stored positions for a blueprint (call when starting new generation) */
	static void ClearStoredPositions(UBlueprint* Blueprint);

	/** v4.22: Diagnostic logging for node position persistence audit
	 *  Logs NodeGuid, NodePosX/Y, node pointer, graph pointer for all nodes in Blueprint
	 *  @param Blueprint The blueprint to log positions for
	 *  @param LogPoint Identifier for the logging point (e.g., "AfterAutoLayout", "BeforeCompile")
	 */
	static void LogNodePositionsDiagnostic(UBlueprint* Blueprint, const FString& LogPoint);

private:
	// v2.6.7: Track missing dependencies during event graph generation
	static TArray<FMissingDependencyInfo> MissingDependencies;
	static void AddMissingDependency(const FString& Name, const FString& Type, const FString& Context);

	static UK2Node* CreateEventNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef,
		UBlueprint* Blueprint);

	static UK2Node* CreateCustomEventNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef,
		UBlueprint* Blueprint);

	static UK2Node* CreateCallFunctionNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef,
		UBlueprint* Blueprint);

	static UK2Node* CreateBranchNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	static UK2Node* CreateVariableGetNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef,
		UBlueprint* Blueprint);

	static UK2Node* CreateVariableSetNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef,
		UBlueprint* Blueprint);

	static UK2Node* CreatePropertyGetNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef,
		UBlueprint* Blueprint);

	static UK2Node* CreatePropertySetNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef,
		UBlueprint* Blueprint);

	static UK2Node* CreateSequenceNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	static UK2Node* CreateDelayNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	// v4.15: AbilityTaskWaitDelay - auto-terminates when ability ends (Track B GAS Audit)
	static UK2Node* CreateAbilityTaskWaitDelayNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	// v7.8: Path B AbilityTasks - BP-safe replacements for Track E delegate bindings
	// WaitGameplayEvent - listens for gameplay events (e.g., GameplayEvent.Death)
	static UK2Node* CreateAbilityTaskWaitGameplayEventNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	// WaitGameplayEffectAppliedToSelf - detects when GE is applied to owner (damage received)
	static UK2Node* CreateAbilityTaskWaitGEAppliedToSelfNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	// WaitGameplayEffectAppliedToTarget - detects when owner applies GE to target (damage dealt)
	static UK2Node* CreateAbilityTaskWaitGEAppliedToTargetNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	// v7.8.46: Phase 7 - SpawnProjectile task for projectile abilities (Contract 25)
	// Creates AbilityTask_SpawnProjectile node with OnTargetData/OnDestroyed delegate outputs
	// Properties:
	//   - task_instance_name: Name for the task instance (default: "SpawnProjectile")
	//   - class_variable: Variable name containing TSubclassOf<ANarrativeProjectile>
	// Output pins:
	//   - OnTargetData: Fires when projectile hits and broadcasts target data
	//   - OnDestroyed: Fires when projectile is destroyed without hitting
	static UK2Node* CreateAbilityTaskSpawnProjectileNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	// v7.8.0: Contract 25 - AbilityAsync_WaitAttributeChanged for damage-scaled energy absorption
	// Tracks attribute changes and provides NewValue/OldValue for damage calculation
	static UK2Node* CreateAbilityAsyncWaitAttributeChangedNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	static UK2Node* CreatePrintStringNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	static UK2Node* CreateDynamicCastNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	// v7.8.19: AddDelegate - bind CustomEvent to multicast delegate (e.g., OnPerceptionUpdated)
	// Per LOCKED RULE 7: Generator serves the design - support delegate binding pattern
	static UK2Node* CreateAddDelegateNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef,
		UBlueprint* Blueprint,
		TMap<FString, UK2Node*>& NodeMap);

	static UK2Node* CreateForEachLoopNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	// v7.8.35: Unified ForLoop/ForEachLoop creation for function overrides
	static UK2Node* CreateForLoopOrEachNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	static UK2Node* CreateSpawnActorNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef,
		UBlueprint* Blueprint);

	// v7.8.14: ConstructObjectFromClass - create UObject instance (for goals, etc.)
	static UK2Node* CreateConstructObjectFromClassNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef,
		UBlueprint* Blueprint);

	// v4.34: SpawnNPC - spawn NPC via NarrativeCharacterSubsystem (proper NPC initialization)
	static UK2Node* CreateSpawnNPCNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef,
		UBlueprint* Blueprint,
		TMap<FString, UK2Node*>& OutNodeMap);

	// v7.8.40: FindCharacterFromSubsystem - look up character by CharacterDefinition via NarrativeCharacterSubsystem
	static UK2Node* CreateFindCharacterFromSubsystemNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef,
		UBlueprint* Blueprint,
		TMap<FString, UK2Node*>& OutNodeMap);

	// v2.7.0: BreakStruct - break a struct into individual member pins
	static UK2Node* CreateBreakStructNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	// v2.7.0: MakeArray - create an array from individual elements
	static UK2Node* CreateMakeArrayNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	// v2.7.0: GetArrayItem - access an element at a specific index
	static UK2Node* CreateGetArrayItemNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	// v2.7.8: Self - reference to the blueprint self
	static UK2Node* CreateSelfNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	/** v4.40: MakeLiteralBool - create a boolean literal value for VariableSet nodes */
	static UK2Node* CreateMakeLiteralBoolNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	/** v4.40: MakeLiteralFloat - create a float literal value for VariableSet nodes */
	static UK2Node* CreateMakeLiteralFloatNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	/** v4.40: MakeLiteralInt - create an integer literal value for VariableSet nodes */
	static UK2Node* CreateMakeLiteralIntNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	/** v7.8.14: MakeLiteralByte - create a byte literal value for enum comparisons */
	static UK2Node* CreateMakeLiteralByteNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	/** v4.40: MakeLiteralString - create a string literal value */
	static UK2Node* CreateMakeLiteralStringNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	static EConnectResult ConnectPins(
		const TMap<FString, UK2Node*>& NodeMap,
		const FManifestGraphConnectionDefinition& Connection);

	/** v7.2: C1 - LOCKED CONTRACT 22 - Validated pin connection for direct pin-to-pin wiring
	 * Implements 3-step gate: pin existence, schema approval, link integrity
	 * Returns false if any check fails (caller must abort save)
	 * @param FromPin Source pin (must be non-null)
	 * @param ToPin Target pin (must be non-null)
	 * @param ContextInfo Debug context string (e.g., "CreateDelegate->AddDelegate")
	 * @param OutFailureCount Incremented on failure for tracking
	 */
	static bool ValidatedMakeLinkTo(
		UEdGraphPin* FromPin,
		UEdGraphPin* ToPin,
		const FString& ContextInfo,
		int32& OutFailureCount);

	static UEdGraphPin* FindPinByName(
		UK2Node* Node,
		const FString& PinName,
		EEdGraphPinDirection Direction);

	/** v4.20: Layered graph layout algorithm per Placement Contract v1.0 */
	static void AutoLayoutNodes(
		TMap<FString, UK2Node*>& NodeMap,
		const TArray<FManifestGraphNodeDefinition>& NodeDefs,
		const TArray<FManifestGraphConnectionDefinition>& Connections);
};

// ============================================================================
// New Asset Type Generators
// ============================================================================

class GASABILITYGENERATOR_API FFloatCurveGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestFloatCurveDefinition& Definition);
};

class GASABILITYGENERATOR_API FAnimationMontageGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestAnimationMontageDefinition& Definition);
};

class GASABILITYGENERATOR_API FAnimationNotifyGenerator : public FGeneratorBase
{
public:
	// v4.0: Added ManifestData parameter for event graph reference lookup
	static FGenerationResult Generate(
		const FManifestAnimationNotifyDefinition& Definition,
		const FManifestData* ManifestData = nullptr);
};

class GASABILITYGENERATOR_API FDialogueBlueprintGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(
		const FManifestDialogueBlueprintDefinition& Definition,
		const FString& ProjectRoot = TEXT(""),
		const FManifestData* ManifestData = nullptr);
};

class GASABILITYGENERATOR_API FEquippableItemGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestEquippableItemDefinition& Definition);
};

class GASABILITYGENERATOR_API FActivityGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestActivityDefinition& Definition);
};

class GASABILITYGENERATOR_API FAbilityConfigurationGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestAbilityConfigurationDefinition& Definition);

	/**
	 * v4.13: Category C - P1.3 Startup Effects Validation
	 * Validates that ability configurations with form abilities have a default form state:
	 * - Warning: If abilities contains GA_Father* forms, startup_effects should contain GE_*State
	 * @param Definition The ability configuration definition to validate
	 * @param OutWarnings Warnings array to populate (pipe-delimited: CODE | ContextPath | Message | SuggestedFix)
	 */
	static void ValidateStartupEffects(const FManifestAbilityConfigurationDefinition& Definition, TArray<FString>& OutWarnings);
};

class GASABILITYGENERATOR_API FActivityConfigurationGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestActivityConfigurationDefinition& Definition);
};

class GASABILITYGENERATOR_API FItemCollectionGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestItemCollectionDefinition& Definition);
};

class GASABILITYGENERATOR_API FNarrativeEventGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestNarrativeEventDefinition& Definition);
};

// v4.0: Gameplay Cue Generator - creates GC_ prefixed Blueprints
class GASABILITYGENERATOR_API FGameplayCueGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestGameplayCueDefinition& Definition);
};

class GASABILITYGENERATOR_API FNPCDefinitionGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestNPCDefinitionDefinition& Definition);
};

class GASABILITYGENERATOR_API FCharacterDefinitionGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestCharacterDefinitionDefinition& Definition);
};

// v4.8.3: CharacterAppearance Generator - creates UCharacterAppearance data assets
class GASABILITYGENERATOR_API FCharacterAppearanceGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestCharacterAppearanceDefinition& Definition);
};

/**
 * v4.9: TriggerSet Generator - creates UTriggerSet DataAssets with instanced triggers
 * Supports BPT_TimeOfDayRange, BPT_Always, and other UNarrativeTrigger subclasses
 * Each trigger can contain instanced UNarrativeEvent objects
 */
class GASABILITYGENERATOR_API FTriggerSetGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestTriggerSetDefinition& Definition);
};

// v2.5.7: TaggedDialogueSet Generator - creates UTaggedDialogueSet data assets
class GASABILITYGENERATOR_API FTaggedDialogueSetGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestTaggedDialogueSetDefinition& Definition);
};

// v2.6.5: Niagara System Generator - creates UNiagaraSystem assets
// v2.9.1: Added validation system for template integrity and regeneration safety
class GASABILITYGENERATOR_API FNiagaraSystemGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestNiagaraSystemDefinition& Definition);

	// v2.9.1: Validation functions
	/**
	 * Validate template system integrity (Phase A validation)
	 * Checks: system loads, has expected User params, no duplicates
	 * @param TemplatePath Path to the template Niagara system
	 * @param OutResult Validation result with errors/warnings
	 * @return true if template is valid for use
	 */
	static bool ValidateTemplate(const FString& TemplatePath, FFXValidationResult& OutResult);

	/**
	 * Validate FX descriptor against template parameters (Phase B validation)
	 * Checks: required params exist, types match, values in range
	 * @param Definition The Niagara system definition from manifest
	 * @param TemplateSystem The loaded template system
	 * @param OutResult Validation result with errors/warnings
	 * @return true if descriptor is valid for this template
	 */
	static bool ValidateDescriptor(const FManifestNiagaraSystemDefinition& Definition,
		class UNiagaraSystem* TemplateSystem, FFXValidationResult& OutResult);

	/**
	 * Check if existing asset was manually edited (regeneration safety)
	 * @param ExistingSystem The existing generated asset
	 * @param Definition The current manifest definition
	 * @return true if asset appears to have been manually modified
	 */
	static bool DetectManualEdit(class UNiagaraSystem* ExistingSystem,
		const FManifestNiagaraSystemDefinition& Definition);

	/**
	 * Get expected parameter definitions for template validation
	 * Returns the canonical list of User.* parameters that should exist
	 */
	static TArray<FFXExpectedParam> GetExpectedParameters();

	/**
	 * Store generator metadata on asset for regeneration tracking
	 */
	static void StoreGeneratorMetadata(class UNiagaraSystem* System,
		const FManifestNiagaraSystemDefinition& Definition);

	/**
	 * Retrieve generator metadata from existing asset
	 * @return Metadata if found, empty metadata if not a generated asset
	 */
	static FFXGeneratorMetadata GetGeneratorMetadata(class UNiagaraSystem* System);

private:
	// Cached expected parameters (built once)
	static TArray<FFXExpectedParam> CachedExpectedParams;
	static bool bExpectedParamsBuilt;

	// Build the expected parameter list
	static void BuildExpectedParameters();
};

// ============================================================================
// v3.9: NPC Pipeline Generators
// ============================================================================

/**
 * v3.9: Activity Schedule Generator
 * Creates UNPCActivitySchedule data assets for NPC daily routines
 */
class GASABILITYGENERATOR_API FActivityScheduleGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestActivityScheduleDefinition& Definition);
};

/**
 * v3.9: Goal Item Generator
 * Creates UNPCGoalItem Blueprint assets for NPC AI objectives
 */
class GASABILITYGENERATOR_API FGoalItemGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestGoalItemDefinition& Definition);
};

/**
 * v3.9.4: Quest Blueprint Generator
 * Creates UQuestBlueprint assets with full state machine (same pattern as DialogueBlueprint v3.8)
 * Works directly on QuestTemplate - visual editor shows empty graph but quest functions at runtime
 */
class GASABILITYGENERATOR_API FQuestGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestQuestDefinition& Definition);

private:
	// v3.9.4: Helper to resolve quest task classes from multiple search paths
	static UClass* ResolveQuestTaskClass(const FString& TaskClassName);
};

/**
 * v3.9.9: POI Placement Generator
 * Places APOIActor instances in World Partition levels
 */
class GASABILITYGENERATOR_API FPOIPlacementGenerator : public FGeneratorBase
{
public:
	/**
	 * Place a POI actor in the world
	 * @param Definition - POI placement definition from manifest
	 * @param World - Target world to spawn in
	 * @param PlacedPOIs - Map of already placed POIs for LinkedPOI resolution
	 * @return Generation result with spawned actor info
	 */
	static FGenerationResult Generate(
		const FManifestPOIPlacement& Definition,
		UWorld* World,
		TMap<FString, class APOIActor*>& PlacedPOIs
	);

	/**
	 * Find existing POI actor by tag
	 * Made public so FNPCSpawnerPlacementGenerator can use it for NearPOI resolution
	 */
	static class APOIActor* FindExistingPOI(UWorld* World, const FString& POITag);
};

/**
 * v3.9.9: NPC Spawner Placement Generator
 * Places ANPCSpawner actors with configured UNPCSpawnComponents in World Partition levels
 */
class GASABILITYGENERATOR_API FNPCSpawnerPlacementGenerator : public FGeneratorBase
{
public:
	/**
	 * Place an NPC spawner actor in the world
	 * @param Definition - Spawner placement definition from manifest
	 * @param World - Target world to spawn in
	 * @param PlacedPOIs - Map of placed POIs for NearPOI resolution
	 * @return Generation result with spawned actor info
	 */
	static FGenerationResult Generate(
		const FManifestNPCSpawnerPlacement& Definition,
		UWorld* World,
		const TMap<FString, class APOIActor*>& PlacedPOIs
	);

private:
	static FVector ResolvePOILocation(UWorld* World, const FString& POITag, const TMap<FString, class APOIActor*>& PlacedPOIs);
	static class ANPCSpawner* FindExistingSpawner(UWorld* World, const FString& SpawnerName);
	static bool ConfigureSpawnComponent(class UNPCSpawnComponent* Component, const FManifestNPCSpawnEntry& Entry, FString& OutErrorMessage);
};
