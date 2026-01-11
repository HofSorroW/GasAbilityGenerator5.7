// GasAbilityGenerator v2.6.7
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
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
#include "GasAbilityGeneratorTypes.h"

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
 * Base generator class - all generators inherit from this
 */
class GASABILITYGENERATOR_API FGeneratorBase
{
public:
	virtual ~FGeneratorBase() = default;

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
	 * Find a user-defined enum by name, searching project content
	 */
	static UEnum* FindUserDefinedEnum(const FString& EnumName, const FString& ProjectRoot);

protected:
	/**
	 * Log generation message to editor output
	 */
	static void LogGeneration(const FString& Message);

private:
	static const FManifestData* ActiveManifest;
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
 * Blackboard Generator
 */
class GASABILITYGENERATOR_API FBlackboardGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestBlackboardDefinition& Definition);
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
 * Material Generator
 */
class GASABILITYGENERATOR_API FMaterialGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestMaterialDefinition& Definition);
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
 */
class GASABILITYGENERATOR_API FEventGraphGenerator : public FGeneratorBase
{
public:
	/**
	 * Generate event graph nodes and connections in a Blueprint
	 */
	static bool GenerateEventGraph(
		UBlueprint* Blueprint,
		const FManifestEventGraphDefinition& GraphDefinition,
		const FString& ProjectRoot);

	/**
	 * Find an event graph definition by name in manifest data
	 */
	static const FManifestEventGraphDefinition* FindEventGraphByName(
		const FManifestData& ManifestData,
		const FString& GraphName);

	// v2.6.7: Missing dependency tracking for deferred generation
	static const TArray<FMissingDependencyInfo>& GetMissingDependencies() { return MissingDependencies; }
	static bool HasMissingDependencies() { return MissingDependencies.Num() > 0; }
	static void ClearMissingDependencies() { MissingDependencies.Empty(); }

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

	static UK2Node* CreatePrintStringNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	static UK2Node* CreateDynamicCastNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	static UK2Node* CreateForEachLoopNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef);

	static UK2Node* CreateSpawnActorNode(
		UEdGraph* Graph,
		const FManifestGraphNodeDefinition& NodeDef,
		UBlueprint* Blueprint);

	static bool ConnectPins(
		const TMap<FString, UK2Node*>& NodeMap,
		const FManifestGraphConnectionDefinition& Connection);

	static UEdGraphPin* FindPinByName(
		UK2Node* Node,
		const FString& PinName,
		EEdGraphPinDirection Direction);

	static void AutoLayoutNodes(
		TMap<FString, UK2Node*>& NodeMap,
		const TArray<FManifestGraphNodeDefinition>& NodeDefs);
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
	static FGenerationResult Generate(const FManifestAnimationNotifyDefinition& Definition);
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

// v2.5.7: TaggedDialogueSet Generator - creates UTaggedDialogueSet data assets
class GASABILITYGENERATOR_API FTaggedDialogueSetGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestTaggedDialogueSetDefinition& Definition);
};

// v2.6.5: Niagara System Generator - creates UNiagaraSystem assets
class GASABILITYGENERATOR_API FNiagaraSystemGenerator : public FGeneratorBase
{
public:
	static FGenerationResult Generate(const FManifestNiagaraSystemDefinition& Definition);
};
