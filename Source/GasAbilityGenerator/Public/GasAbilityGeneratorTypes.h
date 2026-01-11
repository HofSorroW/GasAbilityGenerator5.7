// GasAbilityGenerator v2.6.7
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v2.6.7: Deferred asset retry mechanism for dependency resolution
// v2.6.6: GE assets created as Blueprints for CooldownGameplayEffectClass compatibility
// v2.6.5: Added Niagara System generator
// v2.5.0: Renamed to GasAbilityGenerator for generic UE project compatibility
// v2.4.0: Added inline event graph and variables support for gameplay abilities
// v2.3.0: Added 12 new asset type definitions with dependency-based generation order
// v2.2.0: Added event graph generation - create Blueprint nodes and connections from YAML
// v2.1.9: Added manifest validation - IsAssetInManifest() and BuildAssetWhitelist()
// v2.1.8: Added enumeration generation support

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * Generation result status enum
 */
enum class EGenerationStatus : uint8
{
	New,      // Asset created successfully
	Skipped,  // Asset already exists (not overwritten)
	Failed,   // Generation error occurred
	Deferred  // v2.6.7: Deferred due to missing dependency (will retry)
};

/**
 * Result of a single asset generation
 */
struct FGenerationResult
{
	FString AssetName;
	EGenerationStatus Status = EGenerationStatus::Failed;
	FString Message;
	FString Category;  // For grouping in results dialog

	// v2.6.7: Dependency tracking for retry mechanism
	FString MissingDependency;      // Name of the missing asset (e.g., "BP_FatherCompanion")
	FString MissingDependencyType;  // Type of missing asset (e.g., "ActorBlueprint")
	int32 RetryCount = 0;           // Number of retry attempts

	FGenerationResult() = default;

	FGenerationResult(const FString& InName, EGenerationStatus InStatus, const FString& InMessage = TEXT(""))
		: AssetName(InName)
		, Status(InStatus)
		, Message(InMessage)
	{}

	// v2.6.7: Constructor with dependency info
	FGenerationResult(const FString& InName, EGenerationStatus InStatus, const FString& InMessage,
		const FString& InMissingDep, const FString& InMissingDepType)
		: AssetName(InName)
		, Status(InStatus)
		, Message(InMessage)
		, MissingDependency(InMissingDep)
		, MissingDependencyType(InMissingDepType)
	{}

	// v2.6.7: Check if this result can be retried
	bool CanRetry() const { return Status == EGenerationStatus::Deferred && !MissingDependency.IsEmpty(); }

	// Helper to determine category from asset name prefix
	void DetermineCategory()
	{
		if (AssetName.StartsWith(TEXT("IA_"))) Category = TEXT("Input Actions");
		else if (AssetName.StartsWith(TEXT("IMC_"))) Category = TEXT("Input Mapping Contexts");
		else if (AssetName.StartsWith(TEXT("GE_"))) Category = TEXT("Gameplay Effects");
		else if (AssetName.StartsWith(TEXT("GA_"))) Category = TEXT("Gameplay Abilities");
		else if (AssetName.StartsWith(TEXT("BP_"))) Category = TEXT("Actor Blueprints");
		else if (AssetName.StartsWith(TEXT("WBP_"))) Category = TEXT("Widget Blueprints");
		else if (AssetName.StartsWith(TEXT("DBP_"))) Category = TEXT("Dialogue Blueprints");
		else if (AssetName.StartsWith(TEXT("BB_"))) Category = TEXT("Blackboards");
		else if (AssetName.StartsWith(TEXT("BT_"))) Category = TEXT("Behavior Trees");
		else if (AssetName.StartsWith(TEXT("M_"))) Category = TEXT("Materials");
		else if (AssetName.StartsWith(TEXT("AC_"))) Category = TEXT("Ability Configurations");
		else if (AssetName.StartsWith(TEXT("ActConfig_"))) Category = TEXT("Activity Configurations");
		else if (AssetName.StartsWith(TEXT("NPCDef_"))) Category = TEXT("NPC Definitions");
		else if (AssetName.StartsWith(TEXT("CD_"))) Category = TEXT("Character Definitions");
		else if (AssetName.StartsWith(TEXT("BPA_"))) Category = TEXT("Activities");
		else if (AssetName.StartsWith(TEXT("EI_"))) Category = TEXT("Equippable Items");
		else if (AssetName.StartsWith(TEXT("IC_"))) Category = TEXT("Item Collections");
		else if (AssetName.StartsWith(TEXT("NE_"))) Category = TEXT("Narrative Events");
		else if (AssetName.StartsWith(TEXT("E_"))) Category = TEXT("Enumerations");
		else if (AssetName.StartsWith(TEXT("FC_"))) Category = TEXT("Float Curves");
		else if (AssetName.StartsWith(TEXT("AM_"))) Category = TEXT("Animation Montages");
		else if (AssetName.StartsWith(TEXT("NAS_"))) Category = TEXT("Animation Notifies");
		else if (AssetName.StartsWith(TEXT("NS_"))) Category = TEXT("Niagara Systems");
		else Category = TEXT("Other");
	}
};

/**
 * Aggregated generation results
 */
struct FGenerationSummary
{
	TArray<FGenerationResult> Results;
	int32 NewCount = 0;
	int32 SkippedCount = 0;
	int32 FailedCount = 0;
	int32 DeferredCount = 0;  // v2.6.7: Track deferred assets

	void AddResult(const FGenerationResult& Result)
	{
		Results.Add(Result);

		switch (Result.Status)
		{
		case EGenerationStatus::New:
			NewCount++;
			break;
		case EGenerationStatus::Skipped:
			SkippedCount++;
			break;
		case EGenerationStatus::Failed:
			FailedCount++;
			break;
		case EGenerationStatus::Deferred:
			DeferredCount++;
			break;
		}
	}

	int32 GetTotal() const { return NewCount + SkippedCount + FailedCount + DeferredCount; }

	// v2.6.7: Get all deferred results that can be retried
	TArray<FGenerationResult> GetDeferredResults() const
	{
		TArray<FGenerationResult> Deferred;
		for (const auto& Result : Results)
		{
			if (Result.CanRetry())
			{
				Deferred.Add(Result);
			}
		}
		return Deferred;
	}

	void Reset()
	{
		Results.Empty();
		NewCount = 0;
		SkippedCount = 0;
		FailedCount = 0;
		DeferredCount = 0;
	}
};

/**
 * Enumeration definition from manifest
 */
struct FManifestEnumerationDefinition
{
	FString Name;
	FString Folder;
	TArray<FString> Values;
};

/**
 * Input action definition from manifest
 */
struct FManifestInputActionDefinition
{
	FString Name;
	FString ValueType = TEXT("Boolean");  // Boolean, Axis1D, Axis2D, Axis3D
	FString TriggerType = TEXT("Pressed"); // Pressed, Released, Down, Started, Triggered
};

/**
 * Input mapping binding definition
 */
struct FManifestInputMappingBinding
{
	FString ActionName;
	FString Key;
	TArray<FString> Modifiers;
	TArray<FString> Triggers;
};

/**
 * Input mapping context definition
 */
struct FManifestInputMappingContextDefinition
{
	FString Name;
	TArray<FManifestInputMappingBinding> Bindings;
};

/**
 * Modifier definition for gameplay effects
 */
struct FManifestModifierDefinition
{
	FString Attribute;
	FString Operation = TEXT("Additive");  // Additive, Multiply, Override
	FString MagnitudeType = TEXT("ScalableFloat");  // ScalableFloat, SetByCaller
	float ScalableFloatValue = 0.0f;
	FString SetByCallerTag;
};

/**
 * Gameplay effect component definition (UE5.x)
 */
struct FManifestGEComponentDefinition
{
	FString ComponentClass;
	TMap<FString, FString> Properties;
};

/**
 * Gameplay effect definition
 */
struct FManifestGameplayEffectDefinition
{
	FString Name;
	FString Folder;
	FString DurationPolicy = TEXT("Instant");  // Instant, HasDuration, Infinite
	float DurationMagnitude = 0.0f;
	float Period = 0.0f;
	bool bExecutePeriodicOnApplication = false;
	TArray<FString> GrantedTags;
	TArray<FString> RemoveGameplayEffectsWithTags;
	TArray<FManifestModifierDefinition> Modifiers;
	TArray<FManifestGEComponentDefinition> Components;
	int32 StackLimitCount = 0;
	FString StackingType;
	TArray<FString> ExecutionClasses;
	TArray<FString> SetByCallerTags;
};

/**
 * Gameplay ability tag configuration
 */
struct FManifestAbilityTagsDefinition
{
	TArray<FString> AbilityTags;
	TArray<FString> CancelAbilitiesWithTag;
	TArray<FString> ActivationOwnedTags;
	TArray<FString> ActivationRequiredTags;
	TArray<FString> ActivationBlockedTags;
};

/**
 * Actor blueprint variable definition
 */
struct FManifestActorVariableDefinition
{
	FString Name;
	FString Type;
	FString Class;  // Class name for Object/Class types
	FString DefaultValue;
	bool bReplicated = false;
	bool bInstanceEditable = false;
};

// Forward declarations for event graph types
struct FManifestGraphPinReference;
struct FManifestGraphNodeDefinition;
struct FManifestGraphConnectionDefinition;
struct FManifestEventGraphDefinition;

/**
 * Gameplay ability definition
 */
struct FManifestGameplayAbilityDefinition
{
	FString Name;
	FString ParentClass = TEXT("GameplayAbility");
	FString Folder;
	FString InstancingPolicy = TEXT("InstancedPerActor");
	FString NetExecutionPolicy = TEXT("ServerOnly");
	FString CooldownGameplayEffectClass;
	FManifestAbilityTagsDefinition Tags;

	// Variables defined on the ability Blueprint
	TArray<FManifestActorVariableDefinition> Variables;

	// Inline event graph - stored by name, looked up from EventGraphs array
	FString EventGraphName;
	bool bHasInlineEventGraph = false;

	// Inline event graph data (populated during parsing, used during generation)
	TArray<FManifestGraphNodeDefinition> EventGraphNodes;
	TArray<FManifestGraphConnectionDefinition> EventGraphConnections;
};

/**
 * Actor blueprint component definition
 */
struct FManifestActorComponentDefinition
{
	FString Name;
	FString Type;
	TMap<FString, FString> Properties;
};

/**
 * Actor blueprint definition
 */
struct FManifestActorBlueprintDefinition
{
	FString Name;
	FString ParentClass = TEXT("Actor");
	FString Folder;
	TArray<FManifestActorComponentDefinition> Components;
	TArray<FManifestActorVariableDefinition> Variables;
	FString EventGraphName;  // Reference to event_graphs section
};

/**
 * Widget blueprint variable definition
 */
struct FManifestWidgetVariableDefinition
{
	FString Name;
	FString Type;
	FString DefaultValue;
	bool bInstanceEditable = false;
	bool bExposeOnSpawn = false;
};

/**
 * Widget blueprint definition
 */
struct FManifestWidgetBlueprintDefinition
{
	FString Name;
	FString ParentClass = TEXT("UserWidget");
	FString Folder;
	TArray<FManifestWidgetVariableDefinition> Variables;
	FString EventGraphName;  // Reference to event_graphs section
};

/**
 * Blackboard key definition
 */
struct FManifestBlackboardKeyDefinition
{
	FString Name;
	FString Type;  // Bool, Int, Float, String, Name, Vector, Rotator, Object, Class, Enum
	bool bInstanceSynced = false;
};

/**
 * Blackboard definition
 */
struct FManifestBlackboardDefinition
{
	FString Name;
	TArray<FManifestBlackboardKeyDefinition> Keys;
};

/**
 * Behavior tree definition
 */
struct FManifestBehaviorTreeDefinition
{
	FString Name;
	FString BlackboardAsset;
	FString Folder;
};

/**
 * Material definition
 */
struct FManifestMaterialDefinition
{
	FString Name;
	FString Folder;
	FString BlendMode = TEXT("Opaque");
	FString ShadingModel = TEXT("DefaultLit");
	TMap<FString, FString> Parameters;
};

// ============================================================================
// Event Graph Definitions
// ============================================================================

/**
 * Reference to a specific pin on a node
 */
struct FManifestGraphPinReference
{
	FString NodeId;
	FString PinName;

	FManifestGraphPinReference() = default;

	FManifestGraphPinReference(const FString& InNodeId, const FString& InPinName)
		: NodeId(InNodeId)
		, PinName(InPinName)
	{}
};

/**
 * Supported event graph node types
 */
enum class EManifestGraphNodeType : uint8
{
	Event,           // UK2Node_Event - BeginPlay, Tick, EndPlay, etc.
	CustomEvent,     // UK2Node_CustomEvent - User-defined events
	CallFunction,    // UK2Node_CallFunction - Any function call
	Branch,          // UK2Node_IfThenElse - Conditional branching
	VariableGet,     // UK2Node_VariableGet - Read variable
	VariableSet,     // UK2Node_VariableSet - Write variable
	Sequence,        // UK2Node_ExecutionSequence - Multiple output execution
	Delay,           // CallFunction to Delay
	DynamicCast,     // UK2Node_DynamicCast - Cast to type
	PrintString,     // CallFunction to PrintString
	SpawnActor,      // UK2Node_SpawnActorFromClass
	ForEachLoop,     // UK2Node_MacroInstance for ForEachLoop
	Invalid
};

/**
 * Definition for a single node in an event graph
 */
struct FManifestGraphNodeDefinition
{
	// Unique identifier for referencing in connections
	FString Id;

	// Node type string: Event, CustomEvent, CallFunction, Branch, etc.
	FString Type;

	// Optional position [X, Y] - auto-layout if not specified
	float PositionX = 0.0f;
	float PositionY = 0.0f;
	bool bHasPosition = false;

	// Type-specific properties (key-value pairs)
	TMap<FString, FString> Properties;
};

/**
 * Definition for a connection between two pins
 */
struct FManifestGraphConnectionDefinition
{
	FManifestGraphPinReference From;
	FManifestGraphPinReference To;
};

/**
 * Complete event graph definition
 */
struct FManifestEventGraphDefinition
{
	// Unique name for referencing from blueprints
	FString Name;

	// Optional description
	FString Description;

	// All nodes in the graph
	TArray<FManifestGraphNodeDefinition> Nodes;

	// All connections between nodes
	TArray<FManifestGraphConnectionDefinition> Connections;
};

// ============================================================================
// New Asset Type Definitions
// ============================================================================

/**
 * Float curve definition
 */
struct FManifestFloatCurveDefinition
{
	FString Name;
	FString Folder;
	TArray<TPair<float, float>> Keys;  // Time, Value pairs
};

/**
 * Animation montage definition
 */
struct FManifestAnimationMontageDefinition
{
	FString Name;
	FString Folder;
	FString Skeleton;
	TArray<FString> Sections;
};

/**
 * Animation notify state definition
 */
struct FManifestAnimationNotifyDefinition
{
	FString Name;
	FString Folder;
	FString NotifyClass;
};

/**
 * Dialogue blueprint definition (follows actor blueprint pattern)
 */
struct FManifestDialogueBlueprintDefinition
{
	FString Name;
	FString ParentClass = TEXT("NarrativeDialogue");
	FString Folder;
	TArray<FManifestActorVariableDefinition> Variables;
	FString EventGraphName;
};

/**
 * Equippable item definition
 */
struct FManifestEquippableItemDefinition
{
	FString Name;
	FString Folder;
	FString ParentClass;
	FString DisplayName;
	FString Description;
	FString EquipmentSlot;
	FString EquipmentModifierGE;
	TArray<FString> AbilitiesToGrant;
};

/**
 * Activity definition
 */
struct FManifestActivityDefinition
{
	FString Name;
	FString Folder;
	FString ParentClass;
	FString BehaviorTree;
	FString Description;
};

/**
 * Ability configuration definition
 */
struct FManifestAbilityConfigurationDefinition
{
	FString Name;
	FString Folder;
	TArray<FString> Abilities;
};

/**
 * Activity configuration definition
 */
struct FManifestActivityConfigurationDefinition
{
	FString Name;
	FString Folder;
	float RescoreInterval = 1.0f;
	FString DefaultActivity;
	TArray<FString> Activities;
};

/**
 * Item with quantity for item collections
 */
struct FManifestItemWithQuantity
{
	FString ItemClass;       // Item class path or name
	int32 Quantity = 1;      // Quantity of this item
};

/**
 * Item collection definition
 */
struct FManifestItemCollectionDefinition
{
	FString Name;
	FString Folder;
	TArray<FString> Items;                        // Simple item names (legacy support)
	TArray<FManifestItemWithQuantity> ItemsWithQuantity;  // v2.6.0: Items with quantities
};

/**
 * Narrative event definition
 */
struct FManifestNarrativeEventDefinition
{
	FString Name;
	FString Folder;
	FString ParentClass;
	FString EventTag;
	FString EventType;
	FString Description;
};

/**
 * NPC definition - maps to UNPCDefinition data asset
 */
struct FManifestNPCDefinitionDefinition
{
	FString Name;
	FString Folder;
	FString NPCID;
	FString NPCName;
	FString NPCClassPath;
	FString AbilityConfiguration;
	FString ActivityConfiguration;
	int32 MinLevel = 1;
	int32 MaxLevel = 1;
	bool bAllowMultipleInstances = true;
	bool bIsVendor = false;
};

/**
 * Character definition - maps to UCharacterDefinition data asset
 */
struct FManifestCharacterDefinitionDefinition
{
	FString Name;
	FString Folder;
	FString DefaultOwnedTags;
	FString DefaultFactions;
	int32 DefaultCurrency = 0;
	float AttackPriority = 1.0f;
};

/**
 * Single tagged dialogue entry within a TaggedDialogueSet
 */
struct FManifestTaggedDialogueEntry
{
	FString Tag;                    // Gameplay tag (e.g., Narrative.TaggedDialogue.Returned.StartFollowing)
	FString DialogueClass;          // Dialogue blueprint class path (soft reference)
	float Cooldown = 30.0f;         // Seconds before can play again
	float MaxDistance = 5000.0f;    // Max distance to trigger
	TArray<FString> RequiredTags;   // Tags NPC must have to play this dialogue
	TArray<FString> BlockedTags;    // Tags that prevent this dialogue
};

/**
 * TaggedDialogueSet definition - maps to UTaggedDialogueSet data asset
 */
struct FManifestTaggedDialogueSetDefinition
{
	FString Name;
	FString Folder;
	TArray<FManifestTaggedDialogueEntry> Dialogues;
};

/**
 * v2.6.5: Niagara System definition - creates UNiagaraSystem assets
 */
struct FManifestNiagaraSystemDefinition
{
	FString Name;
	FString Folder;
	FString TemplateSystem;     // Optional: System to copy from (e.g., NS_DefaultSprite)
	TArray<FString> Emitters;   // Optional: Emitters to add to new system
};

/**
 * Parsed manifest data
 */
struct FManifestData
{
	FString ProjectRoot;
	FString TagsIniPath;
	TArray<FString> Tags;
	TArray<FManifestEnumerationDefinition> Enumerations;
	TArray<FManifestInputActionDefinition> InputActions;
	TArray<FManifestInputMappingContextDefinition> InputMappingContexts;
	TArray<FManifestGameplayEffectDefinition> GameplayEffects;
	TArray<FManifestGameplayAbilityDefinition> GameplayAbilities;
	TArray<FManifestActorBlueprintDefinition> ActorBlueprints;
	TArray<FManifestWidgetBlueprintDefinition> WidgetBlueprints;
	TArray<FManifestBlackboardDefinition> Blackboards;
	TArray<FManifestBehaviorTreeDefinition> BehaviorTrees;
	TArray<FManifestMaterialDefinition> Materials;
	TArray<FManifestEventGraphDefinition> EventGraphs;

	// New asset types
	TArray<FManifestFloatCurveDefinition> FloatCurves;
	TArray<FManifestAnimationMontageDefinition> AnimationMontages;
	TArray<FManifestAnimationNotifyDefinition> AnimationNotifies;
	TArray<FManifestDialogueBlueprintDefinition> DialogueBlueprints;
	TArray<FManifestEquippableItemDefinition> EquippableItems;
	TArray<FManifestActivityDefinition> Activities;
	TArray<FManifestAbilityConfigurationDefinition> AbilityConfigurations;
	TArray<FManifestActivityConfigurationDefinition> ActivityConfigurations;
	TArray<FManifestItemCollectionDefinition> ItemCollections;
	TArray<FManifestNarrativeEventDefinition> NarrativeEvents;
	TArray<FManifestNPCDefinitionDefinition> NPCDefinitions;
	TArray<FManifestCharacterDefinitionDefinition> CharacterDefinitions;
	TArray<FManifestTaggedDialogueSetDefinition> TaggedDialogueSets;
	TArray<FManifestNiagaraSystemDefinition> NiagaraSystems;  // v2.6.5: Niagara VFX systems

	// Cached whitelist of all asset names for validation
	mutable TSet<FString> AssetWhitelist;
	mutable bool bWhitelistBuilt = false;

	/**
	 * Build the asset whitelist from all manifest arrays
	 */
	void BuildAssetWhitelist() const
	{
		AssetWhitelist.Empty();

		for (const auto& Def : Enumerations) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : InputActions) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : InputMappingContexts) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : GameplayEffects) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : GameplayAbilities) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : ActorBlueprints) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : WidgetBlueprints) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : Blackboards) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : BehaviorTrees) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : Materials) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : EventGraphs) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : FloatCurves) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : AnimationMontages) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : AnimationNotifies) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : DialogueBlueprints) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : EquippableItems) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : Activities) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : AbilityConfigurations) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : ActivityConfigurations) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : ItemCollections) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : NarrativeEvents) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : NPCDefinitions) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : CharacterDefinitions) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : TaggedDialogueSets) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : NiagaraSystems) AssetWhitelist.Add(Def.Name);

		bWhitelistBuilt = true;
	}

	/**
	 * Check if an asset name is in the manifest whitelist
	 */
	bool IsAssetInManifest(const FString& AssetName) const
	{
		if (!bWhitelistBuilt)
		{
			BuildAssetWhitelist();
		}
		return AssetWhitelist.Contains(AssetName);
	}

	/**
	 * Get the whitelist for debugging/logging
	 */
	const TSet<FString>& GetAssetWhitelist() const
	{
		if (!bWhitelistBuilt)
		{
			BuildAssetWhitelist();
		}
		return AssetWhitelist;
	}

	/**
	 * Find an event graph definition by name
	 */
	const FManifestEventGraphDefinition* FindEventGraphByName(const FString& GraphName) const
	{
		for (const auto& Graph : EventGraphs)
		{
			if (Graph.Name == GraphName)
			{
				return &Graph;
			}
		}
		return nullptr;
	}

	/**
	 * Get event graph count for display
	 */
	int32 GetEventGraphCount() const
	{
		return EventGraphs.Num();
	}

	/**
	 * Calculate total asset count
	 */
	int32 GetTotalAssetCount() const
	{
		int32 Total = 0;

		Total += Enumerations.Num();
		Total += InputActions.Num();
		Total += InputMappingContexts.Num();
		Total += GameplayEffects.Num();
		Total += GameplayAbilities.Num();
		Total += ActorBlueprints.Num();
		Total += WidgetBlueprints.Num();
		Total += Blackboards.Num();
		Total += BehaviorTrees.Num();
		Total += Materials.Num();
		Total += FloatCurves.Num();
		Total += AnimationMontages.Num();
		Total += AnimationNotifies.Num();
		Total += DialogueBlueprints.Num();
		Total += EquippableItems.Num();
		Total += Activities.Num();
		Total += AbilityConfigurations.Num();
		Total += ActivityConfigurations.Num();
		Total += ItemCollections.Num();
		Total += NarrativeEvents.Num();
		Total += NPCDefinitions.Num();
		Total += CharacterDefinitions.Num();
		Total += NiagaraSystems.Num();

		return Total;
	}

	/**
	 * Get tag count separately for display
	 */
	int32 GetTagCount() const
	{
		return Tags.Num();
	}
};
