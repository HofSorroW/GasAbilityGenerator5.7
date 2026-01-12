// GasAbilityGenerator v3.0
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v3.0: Regen/Diff Safety System - FGeneratorMetadata, EDryRunStatus, FDryRunResult, FDryRunSummary
//       Universal ComputeHash() on all FManifest*Definition structs for change detection
// v2.9.1: FX Validation System - FFXValidationError, FFXExpectedParam, FFXGeneratorMetadata
//         Template integrity validation, descriptor hashing for regeneration safety
// v2.9.0: Data-driven FX architecture - FManifestFXDescriptor for Niagara User param binding
// v2.8.3: Function override support for parent class functions (HandleDeath, etc.)
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
		else if (AssetName.StartsWith(TEXT("MF_"))) Category = TEXT("Material Functions");  // v2.6.12: Must check before M_
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

// ============================================================================
// v2.9.1: Generalized Validation System
// Provides validation utilities for all generators
// ============================================================================

/**
 * Validation error severity levels
 */
enum class EValidationSeverity : uint8
{
	Info,     // Informational message
	Warning,  // Non-fatal issue
	Error,    // Fatal error - blocks generation
};

/**
 * Validation error categories for grouping
 */
enum class EValidationCategory : uint8
{
	Required,      // Required field missing
	Reference,     // Referenced asset not found
	Format,        // Naming/format issue
	TypeMismatch,  // Type incompatibility
	Range,         // Value out of range
	Dependency,    // Missing dependency
	Template,      // Template validation
	Custom,        // Custom validation
};

/**
 * Single validation issue from pre-generation checks
 */
struct FValidationIssue
{
	FString AssetName;           // Asset being validated
	FString FieldName;           // Field with the issue
	FString Message;             // Human-readable message
	EValidationSeverity Severity = EValidationSeverity::Warning;
	EValidationCategory Category = EValidationCategory::Custom;

	FValidationIssue() = default;

	FValidationIssue(const FString& InAsset, const FString& InField, const FString& InMessage,
		EValidationSeverity InSeverity = EValidationSeverity::Warning,
		EValidationCategory InCategory = EValidationCategory::Custom)
		: AssetName(InAsset), FieldName(InField), Message(InMessage)
		, Severity(InSeverity), Category(InCategory) {}

	// Convenience constructors
	static FValidationIssue RequiredField(const FString& Asset, const FString& Field)
	{
		return FValidationIssue(Asset, Field,
			FString::Printf(TEXT("Required field '%s' is empty"), *Field),
			EValidationSeverity::Error, EValidationCategory::Required);
	}

	static FValidationIssue MissingReference(const FString& Asset, const FString& Field, const FString& RefPath)
	{
		return FValidationIssue(Asset, Field,
			FString::Printf(TEXT("Referenced asset not found: %s"), *RefPath),
			EValidationSeverity::Error, EValidationCategory::Reference);
	}

	static FValidationIssue InvalidFormat(const FString& Asset, const FString& Field, const FString& Expected)
	{
		return FValidationIssue(Asset, Field,
			FString::Printf(TEXT("Invalid format. Expected: %s"), *Expected),
			EValidationSeverity::Error, EValidationCategory::Format);
	}

	static FValidationIssue ValueOutOfRange(const FString& Asset, const FString& Field, float Value, float Min, float Max)
	{
		return FValidationIssue(Asset, Field,
			FString::Printf(TEXT("Value %.2f out of range [%.2f, %.2f]"), Value, Min, Max),
			EValidationSeverity::Warning, EValidationCategory::Range);
	}

	// Get log prefix based on severity
	FString GetLogPrefix() const
	{
		switch (Severity)
		{
		case EValidationSeverity::Info: return TEXT("[INFO]");
		case EValidationSeverity::Warning: return TEXT("[WARN]");
		case EValidationSeverity::Error: return TEXT("[ERROR]");
		default: return TEXT("[???]");
		}
	}

	// Format for logging
	FString ToString() const
	{
		return FString::Printf(TEXT("%s %s.%s: %s"), *GetLogPrefix(), *AssetName, *FieldName, *Message);
	}
};

/**
 * Aggregated validation result for pre-generation checks
 */
struct FPreValidationResult
{
	TArray<FValidationIssue> Issues;
	bool bValidated = false;

	// Counts by severity
	int32 InfoCount = 0;
	int32 WarningCount = 0;
	int32 ErrorCount = 0;

	void AddIssue(const FValidationIssue& Issue)
	{
		Issues.Add(Issue);
		switch (Issue.Severity)
		{
		case EValidationSeverity::Info: InfoCount++; break;
		case EValidationSeverity::Warning: WarningCount++; break;
		case EValidationSeverity::Error: ErrorCount++; break;
		}
	}

	// Check if validation passed (no errors)
	bool IsValid() const { return ErrorCount == 0; }

	// Check if there are any issues at all
	bool HasIssues() const { return Issues.Num() > 0; }

	// Get summary string
	FString GetSummary() const
	{
		return FString::Printf(TEXT("Validation: %d errors, %d warnings, %d info"),
			ErrorCount, WarningCount, InfoCount);
	}

	// Log all issues
	void LogIssues() const
	{
		for (const auto& Issue : Issues)
		{
			switch (Issue.Severity)
			{
			case EValidationSeverity::Error:
				UE_LOG(LogTemp, Error, TEXT("[VALIDATE] %s"), *Issue.ToString());
				break;
			case EValidationSeverity::Warning:
				UE_LOG(LogTemp, Warning, TEXT("[VALIDATE] %s"), *Issue.ToString());
				break;
			default:
				UE_LOG(LogTemp, Log, TEXT("[VALIDATE] %s"), *Issue.ToString());
				break;
			}
		}
	}

	void Reset()
	{
		Issues.Empty();
		bValidated = false;
		InfoCount = 0;
		WarningCount = 0;
		ErrorCount = 0;
	}

	// Get issues filtered by category
	TArray<FValidationIssue> GetIssuesByCategory(EValidationCategory Category) const
	{
		return Issues.FilterByPredicate([Category](const FValidationIssue& Issue) {
			return Issue.Category == Category;
		});
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

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		for (const FString& Val : Values)
		{
			Hash ^= GetTypeHash(Val);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

/**
 * Input action definition from manifest
 */
struct FManifestInputActionDefinition
{
	FString Name;
	FString ValueType = TEXT("Boolean");  // Boolean, Axis1D, Axis2D, Axis3D
	FString TriggerType = TEXT("Pressed"); // Pressed, Released, Down, Started, Triggered

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(ValueType) << 8;
		Hash ^= GetTypeHash(TriggerType) << 16;
		return Hash;
	}
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

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(ActionName);
		Hash ^= GetTypeHash(Key) << 8;
		for (const FString& Mod : Modifiers)
		{
			Hash ^= GetTypeHash(Mod);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const FString& Trig : Triggers)
		{
			Hash ^= GetTypeHash(Trig);
			Hash = (Hash << 5) | (Hash >> 59);
		}
		return Hash;
	}
};

/**
 * Input mapping context definition
 */
struct FManifestInputMappingContextDefinition
{
	FString Name;
	TArray<FManifestInputMappingBinding> Bindings;

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		for (const auto& Binding : Bindings)
		{
			Hash ^= Binding.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
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

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Attribute);
		Hash ^= GetTypeHash(Operation) << 4;
		Hash ^= GetTypeHash(MagnitudeType) << 8;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(ScalableFloatValue * 1000.f)) << 12;
		Hash ^= GetTypeHash(SetByCallerTag) << 20;
		return Hash;
	}
};

/**
 * Gameplay effect component definition (UE5.x)
 */
struct FManifestGEComponentDefinition
{
	FString ComponentClass;
	TMap<FString, FString> Properties;

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(ComponentClass);
		for (const auto& Prop : Properties)
		{
			Hash ^= GetTypeHash(Prop.Key);
			Hash ^= GetTypeHash(Prop.Value) << 4;
		}
		return Hash;
	}
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

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		Hash ^= GetTypeHash(DurationPolicy) << 4;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(DurationMagnitude * 1000.f)) << 8;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(Period * 1000.f)) << 16;
		Hash ^= (bExecutePeriodicOnApplication ? 1ULL : 0ULL) << 24;
		Hash ^= static_cast<uint64>(StackLimitCount) << 28;
		Hash ^= static_cast<uint64>(GetTypeHash(StackingType)) << 32;

		// Tags
		for (const FString& Tag : GrantedTags) Hash ^= GetTypeHash(Tag);
		for (const FString& Tag : RemoveGameplayEffectsWithTags) Hash ^= GetTypeHash(Tag) << 2;
		for (const FString& Tag : SetByCallerTags) Hash ^= GetTypeHash(Tag) << 4;
		for (const FString& Exec : ExecutionClasses) Hash ^= GetTypeHash(Exec) << 6;

		// Modifiers and Components
		for (const auto& Mod : Modifiers)
		{
			Hash ^= Mod.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const auto& Comp : Components)
		{
			Hash ^= Comp.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}

		return Hash;
	}
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

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = 0;
		for (const FString& Tag : AbilityTags) Hash ^= GetTypeHash(Tag);
		for (const FString& Tag : CancelAbilitiesWithTag) Hash ^= GetTypeHash(Tag) << 4;
		for (const FString& Tag : ActivationOwnedTags) Hash ^= GetTypeHash(Tag) << 8;
		for (const FString& Tag : ActivationRequiredTags) Hash ^= GetTypeHash(Tag) << 12;
		for (const FString& Tag : ActivationBlockedTags) Hash ^= GetTypeHash(Tag) << 16;
		return Hash;
	}
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

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(Type) << 4;
		Hash ^= GetTypeHash(Class) << 8;
		Hash ^= GetTypeHash(DefaultValue) << 12;
		Hash ^= (bReplicated ? 1ULL : 0ULL) << 16;
		Hash ^= (bInstanceEditable ? 1ULL : 0ULL) << 17;
		return Hash;
	}
};

// ============================================================================
// Event Graph Type Definitions (moved before structs that use them)
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

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		return GetTypeHash(NodeId) ^ (GetTypeHash(PinName) << 16);
	}
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

	/** v3.0: Compute hash for change detection (excludes position - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Id);
		Hash ^= GetTypeHash(Type) << 8;
		// NOTE: PositionX, PositionY, bHasPosition are excluded - presentational only
		for (const auto& Prop : Properties)
		{
			Hash ^= GetTypeHash(Prop.Key);
			Hash ^= GetTypeHash(Prop.Value) << 4;
		}
		return Hash;
	}
};

/**
 * Definition for a connection between two pins
 */
struct FManifestGraphConnectionDefinition
{
	FManifestGraphPinReference From;
	FManifestGraphPinReference To;

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		return From.ComputeHash() ^ (To.ComputeHash() << 32);
	}
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

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(Description) << 8;
		for (const auto& Node : Nodes)
		{
			Hash ^= Node.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61); // Rotate to avoid position-dependence
		}
		for (const auto& Conn : Connections)
		{
			Hash ^= Conn.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59); // Rotate
		}
		return Hash;
	}
};

// ============================================================================

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

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(ParentClass) << 4;
		// NOTE: Folder excluded - presentational only
		Hash ^= GetTypeHash(InstancingPolicy) << 8;
		Hash ^= GetTypeHash(NetExecutionPolicy) << 12;
		Hash ^= GetTypeHash(CooldownGameplayEffectClass) << 16;
		Hash ^= Tags.ComputeHash() << 20;

		// Variables
		for (const auto& Var : Variables)
		{
			Hash ^= Var.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}

		// Event graph
		Hash ^= GetTypeHash(EventGraphName) << 24;
		Hash ^= (bHasInlineEventGraph ? 1ULL : 0ULL) << 28;

		for (const auto& Node : EventGraphNodes)
		{
			Hash ^= Node.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}
		for (const auto& Conn : EventGraphConnections)
		{
			Hash ^= Conn.ComputeHash();
			Hash = (Hash << 7) | (Hash >> 57);
		}

		return Hash;
	}
};

/**
 * Actor blueprint component definition
 */
struct FManifestActorComponentDefinition
{
	FString Name;
	FString Type;
	TMap<FString, FString> Properties;

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(Type) << 8;
		for (const auto& Prop : Properties)
		{
			Hash ^= GetTypeHash(Prop.Key);
			Hash ^= GetTypeHash(Prop.Value) << 4;
		}
		return Hash;
	}
};

/**
 * Function override definition
 * v2.8.3: Used for overriding parent class functions (e.g., HandleDeath)
 */
struct FManifestFunctionOverrideDefinition
{
	FString FunctionName;  // Name of function to override (e.g., "HandleDeath")
	TArray<FManifestGraphNodeDefinition> Nodes;
	TArray<FManifestGraphConnectionDefinition> Connections;
	bool bCallParent = true;  // Whether to call parent implementation

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(FunctionName);
		Hash ^= (bCallParent ? 1ULL : 0ULL) << 8;
		for (const auto& Node : Nodes)
		{
			Hash ^= Node.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const auto& Conn : Connections)
		{
			Hash ^= Conn.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}
		return Hash;
	}
};

/**
 * Actor blueprint definition
 * v2.8.3: Added function_overrides for overriding parent class functions (HandleDeath, etc.)
 * v2.7.6: Added inline event graph support (bHasInlineEventGraph, EventGraphNodes, EventGraphConnections)
 */
struct FManifestActorBlueprintDefinition
{
	FString Name;
	FString ParentClass = TEXT("Actor");
	FString Folder;
	TArray<FManifestActorComponentDefinition> Components;
	TArray<FManifestActorVariableDefinition> Variables;
	// Event graph - can be reference or inline
	FString EventGraphName;  // Reference to event_graphs section
	bool bHasInlineEventGraph = false;

	// Inline event graph data (populated during parsing, used during generation)
	TArray<FManifestGraphNodeDefinition> EventGraphNodes;
	TArray<FManifestGraphConnectionDefinition> EventGraphConnections;

	// v2.8.3: Function overrides for parent class functions
	TArray<FManifestFunctionOverrideDefinition> FunctionOverrides;

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(ParentClass) << 4;
		// NOTE: Folder excluded - presentational only

		// Components
		for (const auto& Comp : Components)
		{
			Hash ^= Comp.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}

		// Variables
		for (const auto& Var : Variables)
		{
			Hash ^= Var.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}

		// Event graph
		Hash ^= GetTypeHash(EventGraphName) << 8;
		Hash ^= (bHasInlineEventGraph ? 1ULL : 0ULL) << 12;

		for (const auto& Node : EventGraphNodes)
		{
			Hash ^= Node.ComputeHash();
			Hash = (Hash << 7) | (Hash >> 57);
		}
		for (const auto& Conn : EventGraphConnections)
		{
			Hash ^= Conn.ComputeHash();
			Hash = (Hash << 11) | (Hash >> 53);
		}

		// Function overrides
		for (const auto& Override : FunctionOverrides)
		{
			Hash ^= Override.ComputeHash();
			Hash = (Hash << 13) | (Hash >> 51);
		}

		return Hash;
	}
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

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(Type) << 4;
		Hash ^= GetTypeHash(DefaultValue) << 8;
		Hash ^= (bInstanceEditable ? 1ULL : 0ULL) << 12;
		Hash ^= (bExposeOnSpawn ? 1ULL : 0ULL) << 13;
		return Hash;
	}
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
	// Event graph - can be reference or inline
	FString EventGraphName;  // Reference to event_graphs section
	bool bHasInlineEventGraph = false;

	// Inline event graph data (populated during parsing, used during generation)
	TArray<FManifestGraphNodeDefinition> EventGraphNodes;
	TArray<FManifestGraphConnectionDefinition> EventGraphConnections;

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(ParentClass) << 4;
		// NOTE: Folder excluded - presentational only
		for (const auto& Var : Variables)
		{
			Hash ^= Var.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		Hash ^= GetTypeHash(EventGraphName) << 8;
		Hash ^= (bHasInlineEventGraph ? 1ULL : 0ULL) << 12;
		for (const auto& Node : EventGraphNodes)
		{
			Hash ^= Node.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}
		for (const auto& Conn : EventGraphConnections)
		{
			Hash ^= Conn.ComputeHash();
			Hash = (Hash << 7) | (Hash >> 57);
		}
		return Hash;
	}
};

/**
 * Blackboard key definition
 */
struct FManifestBlackboardKeyDefinition
{
	FString Name;
	FString Type;  // Bool, Int, Float, String, Name, Vector, Rotator, Object, Class, Enum
	bool bInstanceSynced = false;

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(Type) << 8;
		Hash ^= (bInstanceSynced ? 1ULL : 0ULL) << 16;
		return Hash;
	}
};

/**
 * Blackboard definition
 */
struct FManifestBlackboardDefinition
{
	FString Name;
	TArray<FManifestBlackboardKeyDefinition> Keys;

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		for (const auto& Key : Keys)
		{
			Hash ^= Key.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

/**
 * v3.1: BehaviorTree decorator definition
 */
struct FManifestBTDecoratorDefinition
{
	FString Class;                        // Decorator class (e.g., BTDecorator_Blackboard, custom BP class)
	FString BlackboardKey;                // Blackboard key to check (if applicable)
	FString Operation;                    // Condition operation (IsSet, IsNotSet, etc.)
	TMap<FString, FString> Properties;    // Additional properties

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Class);
		Hash ^= GetTypeHash(BlackboardKey) << 8;
		Hash ^= GetTypeHash(Operation) << 16;
		return Hash;
	}
};

/**
 * v3.1: BehaviorTree service definition
 */
struct FManifestBTServiceDefinition
{
	FString Class;                        // Service class (e.g., BTS_UpdateTarget, custom BP class)
	float Interval = 0.5f;                // Tick interval
	TMap<FString, FString> Properties;    // Additional properties

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Class);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(Interval * 1000.f)) << 8;
		return Hash;
	}
};

/**
 * v3.1: BehaviorTree node definition (task or composite)
 */
struct FManifestBTNodeDefinition
{
	FString Id;                           // Unique node identifier
	FString Type;                         // Selector, Sequence, SimpleParallel, or Task class name
	FString TaskClass;                    // For task nodes: BTTask_MoveTo, BTTask_Wait, or custom BP class
	FString BlackboardKey;                // Blackboard key for task (if applicable)
	TArray<FString> Children;             // Child node IDs (for composites)
	TArray<FManifestBTDecoratorDefinition> Decorators;  // Decorators on this node
	TArray<FManifestBTServiceDefinition> Services;      // Services on this node (composites only)
	TMap<FString, FString> Properties;    // Additional task properties

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Id);
		Hash ^= GetTypeHash(Type) << 4;
		Hash ^= GetTypeHash(TaskClass) << 8;
		Hash ^= GetTypeHash(BlackboardKey) << 12;
		for (const FString& Child : Children)
		{
			Hash ^= GetTypeHash(Child);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const auto& Decorator : Decorators)
		{
			Hash ^= Decorator.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}
		for (const auto& Service : Services)
		{
			Hash ^= Service.ComputeHash();
			Hash = (Hash << 7) | (Hash >> 57);
		}
		return Hash;
	}
};

/**
 * Behavior tree definition
 * v3.1: Added node tree structure for full programmatic generation
 */
struct FManifestBehaviorTreeDefinition
{
	FString Name;
	FString BlackboardAsset;
	FString Folder;
	FString RootType;                     // Selector or Sequence (default: Selector)
	TArray<FManifestBTNodeDefinition> Nodes;  // All nodes in the tree

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(BlackboardAsset) << 8;
		Hash ^= GetTypeHash(RootType) << 16;
		// NOTE: Folder excluded - presentational only
		for (const auto& Node : Nodes)
		{
			Hash ^= Node.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

/**
 * v2.6.12: Material Expression (node in material graph)
 */
struct FManifestMaterialExpression
{
	FString Id;              // Unique identifier for this node (e.g., "tex_diffuse", "param_color")
	FString Type;            // Expression type: TextureSample, ScalarParameter, VectorParameter, Multiply, Add, Fresnel, etc.
	FString Name;            // Display name (for parameters)
	FString DefaultValue;    // Default value as string (parsed based on type)
	int32 PosX = 0;          // Node position X in graph
	int32 PosY = 0;          // Node position Y in graph
	TMap<FString, FString> Properties;  // Additional properties (e.g., Texture path, Exponent, etc.)

	/** v3.0: Compute hash for change detection (excludes PosX, PosY - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Id);
		Hash ^= GetTypeHash(Type) << 4;
		Hash ^= GetTypeHash(Name) << 8;
		Hash ^= GetTypeHash(DefaultValue) << 12;
		// NOTE: PosX, PosY excluded - presentational only
		for (const auto& Prop : Properties)
		{
			Hash ^= GetTypeHash(Prop.Key);
			Hash ^= GetTypeHash(Prop.Value) << 4;
		}
		return Hash;
	}
};

/**
 * v2.6.12: Material Connection (link between expressions or to material output)
 */
struct FManifestMaterialConnection
{
	FString FromId;          // Source expression ID
	FString FromOutput;      // Source output pin name (e.g., "RGB", "R", "Result")
	FString ToId;            // Target expression ID or "Material" for material outputs
	FString ToInput;         // Target input pin name (e.g., "A", "B", "BaseColor", "Emissive")

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(FromId);
		Hash ^= GetTypeHash(FromOutput) << 4;
		Hash ^= GetTypeHash(ToId) << 8;
		Hash ^= GetTypeHash(ToInput) << 12;
		return Hash;
	}
};

/**
 * v2.6.12: Enhanced Material definition with expression graph support
 */
struct FManifestMaterialDefinition
{
	FString Name;
	FString Folder;
	FString BlendMode = TEXT("Opaque");           // Opaque, Masked, Translucent, Additive, Modulate
	FString ShadingModel = TEXT("DefaultLit");    // DefaultLit, Unlit, Subsurface, etc.
	bool bTwoSided = false;                       // Two-sided rendering
	TMap<FString, FString> Parameters;            // Legacy simple parameters

	// v2.6.12: Expression graph
	TArray<FManifestMaterialExpression> Expressions;   // Nodes in the material graph
	TArray<FManifestMaterialConnection> Connections;   // Connections between nodes

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		Hash ^= GetTypeHash(BlendMode) << 4;
		Hash ^= GetTypeHash(ShadingModel) << 8;
		Hash ^= (bTwoSided ? 1ULL : 0ULL) << 12;
		for (const auto& Param : Parameters)
		{
			Hash ^= GetTypeHash(Param.Key);
			Hash ^= GetTypeHash(Param.Value) << 4;
		}
		for (const auto& Expr : Expressions)
		{
			Hash ^= Expr.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const auto& Conn : Connections)
		{
			Hash ^= Conn.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}
		return Hash;
	}
};

/**
 * v2.6.12: Material Function Input definition
 */
struct FManifestMaterialFunctionInput
{
	FString Name;            // Input name
	FString Type;            // float, float2, float3, float4, texture2d
	FString DefaultValue;    // Default value
	int32 SortPriority = 0;  // Order in input list

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(Type) << 4;
		Hash ^= GetTypeHash(DefaultValue) << 8;
		Hash ^= static_cast<uint64>(SortPriority) << 12;
		return Hash;
	}
};

/**
 * v2.6.12: Material Function Output definition
 */
struct FManifestMaterialFunctionOutput
{
	FString Name;            // Output name
	FString Type;            // float, float2, float3, float4
	int32 SortPriority = 0;  // Order in output list

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(Type) << 4;
		Hash ^= static_cast<uint64>(SortPriority) << 8;
		return Hash;
	}
};

/**
 * v2.6.12: Material Function definition
 */
struct FManifestMaterialFunctionDefinition
{
	FString Name;
	FString Folder;
	FString Description;                              // Function description
	bool bExposeToLibrary = true;                     // Show in material function library
	TArray<FManifestMaterialFunctionInput> Inputs;    // Function inputs
	TArray<FManifestMaterialFunctionOutput> Outputs;  // Function outputs
	TArray<FManifestMaterialExpression> Expressions;  // Internal expression nodes
	TArray<FManifestMaterialConnection> Connections;  // Internal connections

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		Hash ^= GetTypeHash(Description) << 4;
		Hash ^= (bExposeToLibrary ? 1ULL : 0ULL) << 8;
		for (const auto& In : Inputs)
		{
			Hash ^= In.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const auto& Out : Outputs)
		{
			Hash ^= Out.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}
		for (const auto& Expr : Expressions)
		{
			Hash ^= Expr.ComputeHash();
			Hash = (Hash << 7) | (Hash >> 57);
		}
		for (const auto& Conn : Connections)
		{
			Hash ^= Conn.ComputeHash();
			Hash = (Hash << 11) | (Hash >> 53);
		}
		return Hash;
	}
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

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		for (const auto& Key : Keys)
		{
			Hash ^= static_cast<uint64>(FMath::RoundToInt(Key.Key * 1000.f));
			Hash ^= static_cast<uint64>(FMath::RoundToInt(Key.Value * 1000.f)) << 32;
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
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

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		Hash ^= GetTypeHash(Skeleton) << 8;
		for (const FString& Section : Sections)
		{
			Hash ^= GetTypeHash(Section);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

/**
 * Animation notify state definition
 */
struct FManifestAnimationNotifyDefinition
{
	FString Name;
	FString Folder;
	FString NotifyClass;

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		Hash ^= GetTypeHash(NotifyClass) << 8;
		return Hash;
	}
};

/**
 * v3.2: Dialogue speaker definition for UDialogue::Speakers array
 */
struct FManifestDialogueSpeakerDefinition
{
	FString NPCDefinition;  // Reference to NPCDef_ asset
	FString NodeColor;      // Hex color e.g. "#0066FF"
	TArray<FString> OwnedTags;  // Tags applied during dialogue

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(NPCDefinition);
		Hash ^= GetTypeHash(NodeColor) << 4;
		for (const auto& Tag : OwnedTags)
		{
			Hash ^= GetTypeHash(Tag);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

/**
 * Dialogue blueprint definition (follows actor blueprint pattern)
 * v3.2: Added configuration properties and speakers array
 */
struct FManifestDialogueBlueprintDefinition
{
	FString Name;
	FString ParentClass = TEXT("NarrativeDialogue");
	FString Folder;
	TArray<FManifestActorVariableDefinition> Variables;
	FString EventGraphName;

	// v3.2: Dialogue configuration properties
	bool bFreeMovement = true;       // Player can still control character
	bool bUnskippable = false;       // Lines cannot be skipped
	bool bCanBeExited = true;        // Player can exit dialogue with ESC
	bool bShowCinematicBars = false; // Show letterbox bars
	bool bAutoRotateSpeakers = true; // Auto-face current speaker
	bool bAutoStopMovement = false;  // Stop movement when dialogue starts
	int32 Priority = 0;              // Lower = more important
	float EndDialogueDist = 0.0f;    // Auto-end if player > distance (0 = disabled)

	// v3.5: Additional UDialogue properties
	FString DefaultHeadBoneName;     // Bone to aim camera at (default: "head")
	float DialogueBlendOutTime = 0.5f; // Camera blend out duration
	bool bAdjustPlayerTransform = false; // Move player to face speaker

	// v3.2: Speakers configuration
	TArray<FManifestDialogueSpeakerDefinition> Speakers;

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(ParentClass) << 4;
		// NOTE: Folder excluded - presentational only
		for (const auto& Var : Variables)
		{
			Hash ^= Var.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		Hash ^= GetTypeHash(EventGraphName) << 8;
		// v3.2: Include new configuration in hash
		Hash ^= (bFreeMovement ? 1ULL : 0ULL) << 12;
		Hash ^= (bUnskippable ? 1ULL : 0ULL) << 13;
		Hash ^= (bCanBeExited ? 1ULL : 0ULL) << 14;
		Hash ^= (bShowCinematicBars ? 1ULL : 0ULL) << 15;
		Hash ^= (bAutoRotateSpeakers ? 1ULL : 0ULL) << 16;
		Hash ^= (bAutoStopMovement ? 1ULL : 0ULL) << 17;
		Hash ^= GetTypeHash(Priority) << 18;
		Hash ^= GetTypeHash(static_cast<int32>(EndDialogueDist)) << 22;
		// v3.5: Additional properties
		Hash ^= GetTypeHash(DefaultHeadBoneName) << 26;
		Hash ^= GetTypeHash(static_cast<int32>(DialogueBlendOutTime * 1000.f)) << 30;
		Hash ^= (bAdjustPlayerTransform ? 1ULL : 0ULL) << 34;
		for (const auto& Speaker : Speakers)
		{
			Hash ^= Speaker.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}
		return Hash;
	}
};

/**
 * Equippable item definition
 * v3.3: Enhanced with full NarrativeItem + EquippableItem property support
 * v3.4: Added WeaponItem and RangedWeaponItem property support
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

	// v3.3: EquippableItem stat properties
	float AttackRating = 0.0f;           // Bonus attack damage
	float ArmorRating = 0.0f;            // Damage reduction
	float StealthRating = 0.0f;          // Stealth bonus

	// v3.3: NarrativeItem properties
	FString Thumbnail;                   // TSoftObjectPtr<UTexture2D> - inventory icon
	float Weight = 0.0f;                 // Item weight in kg
	int32 BaseValue = 0;                 // Gold/currency value
	float BaseScore = 0.0f;              // AI priority score
	TArray<FString> ItemTags;            // FGameplayTagContainer - item categorization
	bool bStackable = false;             // Can stack in inventory
	int32 MaxStackSize = 1;              // Max stack size (if stackable)
	float UseRechargeDuration = 0.0f;    // Cooldown between uses

	// v3.4: WeaponItem properties (when parent is WeaponItem/MeleeWeaponItem/RangedWeaponItem)
	FString WeaponVisualClass;           // TSoftClassPtr<AWeaponVisual> - visual actor to spawn
	FString WeaponHand;                  // EWeaponHandRule: TwoHanded, MainHand, OffHand, DualWieldable
	TArray<FString> WeaponAbilities;     // Abilities granted when wielded alone
	TArray<FString> MainhandAbilities;   // Abilities when wielded in mainhand
	TArray<FString> OffhandAbilities;    // Abilities when wielded in offhand
	bool bPawnFollowsControlRotation = false;   // Pawn follows camera rotation
	bool bPawnOrientsRotationToMovement = true; // Pawn orients to velocity
	float AttackDamage = 0.0f;           // Base weapon damage
	float HeavyAttackDamageMultiplier = 1.5f;   // Heavy attack multiplier
	bool bAllowManualReload = true;      // Can be manually reloaded
	FString RequiredAmmo;                // TSubclassOf<UNarrativeItem> - ammo class
	bool bBotsConsumeAmmo = true;        // Bots consume ammo
	float BotAttackRange = 1000.0f;      // Bot attack range
	int32 ClipSize = 0;                  // Magazine size (0 = no clip)

	// v3.4: RangedWeaponItem properties
	float AimFOVPct = 0.75f;             // Aim zoom (1=no zoom, 0.1=max zoom)
	float BaseSpreadDegrees = 0.0f;      // Base accuracy spread
	float MaxSpreadDegrees = 5.0f;       // Maximum spread
	float SpreadFireBump = 0.5f;         // Spread increase per shot
	float SpreadDecreaseSpeed = 5.0f;    // Spread recovery speed

	/** v3.4: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		Hash ^= GetTypeHash(ParentClass) << 4;
		Hash ^= GetTypeHash(DisplayName) << 8;
		Hash ^= GetTypeHash(Description) << 12;
		Hash ^= GetTypeHash(EquipmentSlot) << 16;
		Hash ^= GetTypeHash(EquipmentModifierGE) << 20;
		for (const FString& Ability : AbilitiesToGrant)
		{
			Hash ^= GetTypeHash(Ability);
			Hash = (Hash << 3) | (Hash >> 61);
		}

		// v3.3: Hash NarrativeItem/EquippableItem properties
		Hash ^= static_cast<uint64>(FMath::RoundToInt(AttackRating * 100.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(ArmorRating * 100.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(StealthRating * 100.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= GetTypeHash(Thumbnail);
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(Weight * 100.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(BaseValue);
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(BaseScore * 100.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= (bStackable ? 1ULL : 0ULL);
		Hash ^= static_cast<uint64>(MaxStackSize) << 4;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(UseRechargeDuration * 100.f)) << 12;
		for (const FString& Tag : ItemTags)
		{
			Hash ^= GetTypeHash(Tag);
			Hash = (Hash << 3) | (Hash >> 61);
		}

		// v3.4: Hash WeaponItem properties
		Hash ^= GetTypeHash(WeaponVisualClass);
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= GetTypeHash(WeaponHand);
		Hash = (Hash << 5) | (Hash >> 59);
		for (const FString& Ability : WeaponAbilities)
		{
			Hash ^= GetTypeHash(Ability);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const FString& Ability : MainhandAbilities)
		{
			Hash ^= GetTypeHash(Ability);
			Hash = (Hash << 4) | (Hash >> 60);
		}
		for (const FString& Ability : OffhandAbilities)
		{
			Hash ^= GetTypeHash(Ability);
			Hash = (Hash << 5) | (Hash >> 59);
		}
		Hash ^= (bPawnFollowsControlRotation ? 1ULL : 0ULL) << 6;
		Hash ^= (bPawnOrientsRotationToMovement ? 1ULL : 0ULL) << 7;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(AttackDamage * 100.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(HeavyAttackDamageMultiplier * 100.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= (bAllowManualReload ? 1ULL : 0ULL) << 8;
		Hash ^= GetTypeHash(RequiredAmmo);
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= (bBotsConsumeAmmo ? 1ULL : 0ULL) << 9;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(BotAttackRange));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(ClipSize) << 10;

		// v3.4: Hash RangedWeaponItem properties
		Hash ^= static_cast<uint64>(FMath::RoundToInt(AimFOVPct * 100.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(BaseSpreadDegrees * 100.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(MaxSpreadDegrees * 100.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(SpreadFireBump * 100.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(SpreadDecreaseSpeed * 100.f));
		Hash = (Hash << 5) | (Hash >> 59);

		return Hash;
	}
};

/**
 * Activity definition
 * v3.3: Added NarrativeActivityBase and NPCActivity properties
 */
struct FManifestActivityDefinition
{
	FString Name;
	FString Folder;
	FString ParentClass;
	FString BehaviorTree;
	FString Description;

	// v3.3: NarrativeActivityBase properties
	FString ActivityName;                // FText - Display name for activity
	TArray<FString> OwnedTags;           // Tags granted when activity starts
	TArray<FString> BlockTags;           // Tags that block activity from running
	TArray<FString> RequireTags;         // Tags required before running

	// v3.3: NPCActivity properties
	FString SupportedGoalType;           // TSubclassOf<UNPCGoalItem> - Goal class this activity supports
	bool bIsInterruptable = true;        // Whether activity can be interrupted
	bool bSaveActivity = false;          // Whether activity should be saved to disk

	/** v3.3: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		Hash ^= GetTypeHash(ParentClass) << 4;
		Hash ^= GetTypeHash(BehaviorTree) << 8;
		Hash ^= GetTypeHash(Description) << 12;

		// v3.3: Hash new properties
		Hash ^= GetTypeHash(ActivityName) << 16;
		Hash ^= GetTypeHash(SupportedGoalType) << 20;
		Hash ^= (bIsInterruptable ? 1ULL : 0ULL) << 24;
		Hash ^= (bSaveActivity ? 1ULL : 0ULL) << 25;
		for (const FString& Tag : OwnedTags)
		{
			Hash ^= GetTypeHash(Tag);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const FString& Tag : BlockTags)
		{
			Hash ^= GetTypeHash(Tag);
			Hash = (Hash << 5) | (Hash >> 59);
		}
		for (const FString& Tag : RequireTags)
		{
			Hash ^= GetTypeHash(Tag);
			Hash = (Hash << 7) | (Hash >> 57);
		}
		return Hash;
	}
};

/**
 * Ability configuration definition
 * v3.1: Added DefaultAttributes, StartupEffects for full TSubclassOf population
 */
struct FManifestAbilityConfigurationDefinition
{
	FString Name;
	FString Folder;
	TArray<FString> Abilities;           // GA_ ability blueprints to grant
	TArray<FString> StartupEffects;      // GE_ effects applied once on startup
	FString DefaultAttributes;           // GE_ effect for default attributes

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		for (const FString& Ability : Abilities)
		{
			Hash ^= GetTypeHash(Ability);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const FString& Effect : StartupEffects)
		{
			Hash ^= GetTypeHash(Effect);
			Hash = (Hash << 5) | (Hash >> 59);
		}
		Hash ^= GetTypeHash(DefaultAttributes) << 16;
		return Hash;
	}
};

/**
 * Activity configuration definition
 * v3.1: Added GoalGenerators for full TSubclassOf population
 */
struct FManifestActivityConfigurationDefinition
{
	FString Name;
	FString Folder;
	float RescoreInterval = 1.0f;
	FString DefaultActivity;
	TArray<FString> Activities;           // BPA_ activity blueprints
	TArray<FString> GoalGenerators;       // GoalGenerator_ classes

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		Hash ^= static_cast<uint64>(FMath::RoundToInt(RescoreInterval * 1000.f)) << 8;
		Hash ^= GetTypeHash(DefaultActivity) << 16;
		for (const FString& Activity : Activities)
		{
			Hash ^= GetTypeHash(Activity);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const FString& Generator : GoalGenerators)
		{
			Hash ^= GetTypeHash(Generator);
			Hash = (Hash << 5) | (Hash >> 59);
		}
		return Hash;
	}
};

/**
 * Item with quantity for item collections
 */
struct FManifestItemWithQuantity
{
	FString ItemClass;       // Item class path or name
	int32 Quantity = 1;      // Quantity of this item

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(ItemClass);
		Hash ^= static_cast<uint64>(Quantity) << 32;
		return Hash;
	}
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

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		for (const FString& Item : Items)
		{
			Hash ^= GetTypeHash(Item);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const auto& ItemQ : ItemsWithQuantity)
		{
			Hash ^= ItemQ.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}
		return Hash;
	}
};

/**
 * Narrative event definition
 * v3.2: Added event configuration properties (runtime, filter, party policy, targets)
 */
struct FManifestNarrativeEventDefinition
{
	FString Name;
	FString Folder;
	FString ParentClass;
	FString EventTag;
	FString EventType;
	FString Description;

	// v3.2: Event configuration properties
	FString EventRuntime = TEXT("Start");     // "Start", "End", "Both"
	FString EventFilter = TEXT("Anyone");     // "Anyone", "OnlyNPCs", "OnlyPlayers"
	FString PartyEventPolicy = TEXT("Party"); // "Party", "AllPartyMembers", "PartyLeader"
	bool bRefireOnLoad = false;               // Re-fire when game loads

	// v3.2: Target configuration
	TArray<FString> NPCTargets;               // NPCDefinition references for OnlyNPCs filter
	TArray<FString> CharacterTargets;         // CharacterDefinition refs for Anyone filter
	TArray<FString> PlayerTargets;            // PlayerDefinition refs for OnlyPlayers filter

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		Hash ^= GetTypeHash(ParentClass) << 4;
		Hash ^= GetTypeHash(EventTag) << 8;
		Hash ^= GetTypeHash(EventType) << 12;
		Hash ^= GetTypeHash(Description) << 16;
		// v3.2: Include new configuration in hash
		Hash ^= GetTypeHash(EventRuntime) << 20;
		Hash ^= GetTypeHash(EventFilter) << 24;
		Hash ^= GetTypeHash(PartyEventPolicy) << 28;
		Hash ^= (bRefireOnLoad ? 1ULL : 0ULL) << 32;
		for (const auto& Target : NPCTargets)
		{
			Hash ^= GetTypeHash(Target);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const auto& Target : CharacterTargets)
		{
			Hash ^= GetTypeHash(Target);
			Hash = (Hash << 5) | (Hash >> 59);
		}
		for (const auto& Target : PlayerTargets)
		{
			Hash ^= GetTypeHash(Target);
			Hash = (Hash << 7) | (Hash >> 57);
		}
		return Hash;
	}
};

/**
 * NPC definition - maps to UNPCDefinition data asset
 * v3.3: Enhanced with full Narrative Pro property support
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

	// v3.3: Dialogue properties
	FString Dialogue;                    // TSoftClassPtr<UDialogue> - main NPC dialogue
	FString TaggedDialogueSet;           // TSoftObjectPtr<UTaggedDialogueSet> - bark dialogues

	// v3.3: Vendor properties (when bIsVendor = true)
	int32 TradingCurrency = 0;           // Starting vendor gold
	float BuyItemPercentage = 0.5f;      // Percentage of value vendor pays
	float SellItemPercentage = 1.5f;     // Percentage of value vendor charges
	FString ShopFriendlyName;            // Display name for shop UI

	// v3.3: CharacterDefinition inherited properties
	FString DefaultAppearance;           // TSoftObjectPtr<UCharacterAppearanceBase>
	int32 DefaultCurrency = 0;           // Starting character gold
	TArray<FString> DefaultOwnedTags;    // FGameplayTagContainer - State tags
	TArray<FString> DefaultFactions;     // FGameplayTagContainer - Faction tags
	float AttackPriority = 1.0f;         // AI targeting priority

	/** v3.3: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		Hash ^= GetTypeHash(NPCID) << 4;
		Hash ^= GetTypeHash(NPCName) << 8;
		Hash ^= GetTypeHash(NPCClassPath) << 12;
		Hash ^= GetTypeHash(AbilityConfiguration) << 16;
		Hash ^= GetTypeHash(ActivityConfiguration) << 20;
		Hash ^= static_cast<uint64>(MinLevel) << 24;
		Hash ^= static_cast<uint64>(MaxLevel) << 32;
		Hash ^= (bAllowMultipleInstances ? 1ULL : 0ULL) << 40;
		Hash ^= (bIsVendor ? 1ULL : 0ULL) << 41;

		// v3.3: Hash new properties (using rotation to avoid overflow)
		Hash ^= GetTypeHash(Dialogue);
		Hash = (Hash << 7) | (Hash >> 57);
		Hash ^= GetTypeHash(TaggedDialogueSet);
		Hash = (Hash << 7) | (Hash >> 57);
		Hash ^= static_cast<uint64>(TradingCurrency);
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(BuyItemPercentage * 1000.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(SellItemPercentage * 1000.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= GetTypeHash(ShopFriendlyName);
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= GetTypeHash(DefaultAppearance);
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(DefaultCurrency);
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(AttackPriority * 1000.f));
		for (const FString& Tag : DefaultOwnedTags)
		{
			Hash ^= GetTypeHash(Tag);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const FString& Faction : DefaultFactions)
		{
			Hash ^= GetTypeHash(Faction);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

/**
 * Character definition - maps to UCharacterDefinition data asset
 * v3.5: Enhanced with full UCharacterDefinition property support
 */
struct FManifestCharacterDefinitionDefinition
{
	FString Name;
	FString Folder;

	// v3.5: Changed to arrays for proper FGameplayTagContainer support
	TArray<FString> DefaultOwnedTags;    // Tags granted to character (State.Invulnerable, etc.)
	TArray<FString> DefaultFactions;     // Faction tags (Narrative.Factions.Friendly, etc.)

	int32 DefaultCurrency = 0;
	float AttackPriority = 1.0f;

	// v3.5: Additional UCharacterDefinition properties
	FString DefaultAppearance;           // TSoftObjectPtr<UCharacterAppearanceBase>
	TArray<FString> TriggerSets;         // TArray<TSoftObjectPtr<UTriggerSet>>
	FString AbilityConfiguration;        // TObjectPtr<UAbilityConfiguration>

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		for (const FString& Tag : DefaultOwnedTags)
		{
			Hash ^= GetTypeHash(Tag);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const FString& Faction : DefaultFactions)
		{
			Hash ^= GetTypeHash(Faction);
			Hash = (Hash << 5) | (Hash >> 59);
		}
		Hash ^= static_cast<uint64>(DefaultCurrency) << 16;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(AttackPriority * 1000.f)) << 32;
		Hash ^= GetTypeHash(DefaultAppearance) << 4;
		for (const FString& TriggerSet : TriggerSets)
		{
			Hash ^= GetTypeHash(TriggerSet);
			Hash = (Hash << 7) | (Hash >> 57);
		}
		Hash ^= GetTypeHash(AbilityConfiguration) << 8;
		return Hash;
	}
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

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Tag);
		Hash ^= GetTypeHash(DialogueClass) << 4;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(Cooldown * 1000.f)) << 8;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(MaxDistance * 10.f)) << 24;
		for (const FString& ReqTag : RequiredTags)
		{
			Hash ^= GetTypeHash(ReqTag);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const FString& BlkTag : BlockedTags)
		{
			Hash ^= GetTypeHash(BlkTag);
			Hash = (Hash << 5) | (Hash >> 59);
		}
		return Hash;
	}
};

/**
 * TaggedDialogueSet definition - maps to UTaggedDialogueSet data asset
 */
struct FManifestTaggedDialogueSetDefinition
{
	FString Name;
	FString Folder;
	TArray<FManifestTaggedDialogueEntry> Dialogues;

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		for (const auto& Dialogue : Dialogues)
		{
			Hash ^= Dialogue.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

/**
 * v2.6.11: Niagara User Parameter definition
 * Represents a user-exposed parameter in a Niagara System
 */
struct FManifestNiagaraUserParameter
{
	FString Name;           // Parameter name (e.g., CoreScale, AlertLevel)
	FString Type;           // float, int, bool, vector, color, linear_color
	FString DefaultValue;   // Default value as string (parsed based on type)

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(Type) << 8;
		Hash ^= GetTypeHash(DefaultValue) << 16;
		return Hash;
	}
};

/**
 * v2.9.0: FX Descriptor for data-driven Niagara parameter binding
 * Maps directly to User.* parameters in Uber-Emitters
 * See: Data_Driven_FX_Architecture_v1_0.md
 */
struct FManifestFXDescriptor
{
	// Emitter toggles (which emitters to enable)
	bool bParticlesEnabled = false;
	bool bBurstEnabled = false;
	bool bBeamEnabled = false;
	bool bRibbonEnabled = false;
	bool bLightEnabled = false;
	bool bSmokeEnabled = false;
	bool bSparkEnabled = false;

	// Core emission
	float SpawnRate = 100.f;
	float LifetimeMin = 1.0f;
	float LifetimeMax = 2.0f;
	int32 MaxParticles = 500;

	// Appearance
	FLinearColor Color = FLinearColor::White;
	float SizeMin = 5.f;
	float SizeMax = 10.f;
	float Opacity = 1.0f;
	float Emissive = 1.0f;

	// Motion
	FVector InitialVelocity = FVector::ZeroVector;
	float NoiseStrength = 0.f;
	float GravityScale = 0.f;

	// Beam-specific
	float BeamLength = 100.f;
	float BeamWidth = 10.f;

	// Ribbon-specific
	float RibbonWidth = 20.f;

	// Light-specific
	float LightIntensity = 5000.f;
	float LightRadius = 200.f;

	// LOD
	float CullDistance = 5000.f;
	int32 LODLevel = 0;

	// Check if any FX descriptor data was provided
	bool HasData() const
	{
		return bParticlesEnabled || bBurstEnabled || bBeamEnabled ||
		       bRibbonEnabled || bLightEnabled || bSmokeEnabled || bSparkEnabled ||
		       SpawnRate != 100.f || !Color.Equals(FLinearColor::White);
	}

	/**
	 * v2.9.1: Compute stable hash for regeneration detection
	 * Only hashes semantic inputs, not runtime noise
	 */
	uint64 ComputeHash() const
	{
		uint64 Hash = 0;

		// Hash emitter toggles (bit flags)
		uint32 EmitterFlags = 0;
		if (bParticlesEnabled) EmitterFlags |= (1 << 0);
		if (bBurstEnabled) EmitterFlags |= (1 << 1);
		if (bBeamEnabled) EmitterFlags |= (1 << 2);
		if (bRibbonEnabled) EmitterFlags |= (1 << 3);
		if (bLightEnabled) EmitterFlags |= (1 << 4);
		if (bSmokeEnabled) EmitterFlags |= (1 << 5);
		if (bSparkEnabled) EmitterFlags |= (1 << 6);
		Hash ^= static_cast<uint64>(EmitterFlags);

		// Hash core emission (convert floats to stable ints)
		Hash ^= static_cast<uint64>(FMath::RoundToInt(SpawnRate * 100.f)) << 8;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(LifetimeMin * 1000.f)) << 16;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(LifetimeMax * 1000.f)) << 24;
		Hash ^= static_cast<uint64>(MaxParticles) << 32;

		// Hash appearance (color as packed RGBA)
		uint32 PackedColor =
			(FMath::Clamp(FMath::RoundToInt(Color.R * 255.f), 0, 255)) |
			(FMath::Clamp(FMath::RoundToInt(Color.G * 255.f), 0, 255) << 8) |
			(FMath::Clamp(FMath::RoundToInt(Color.B * 255.f), 0, 255) << 16) |
			(FMath::Clamp(FMath::RoundToInt(Color.A * 255.f), 0, 255) << 24);
		Hash ^= static_cast<uint64>(PackedColor) << 40;

		// Hash sizes and motion
		Hash ^= static_cast<uint64>(FMath::RoundToInt(SizeMin * 100.f));
		Hash ^= static_cast<uint64>(FMath::RoundToInt(SizeMax * 100.f)) << 16;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(Opacity * 1000.f)) << 32;

		return Hash;
	}
};

// ============================================================================
// v2.9.1: FX Validation System
// Validates template integrity and descriptor-to-system parameter matching
// ============================================================================

/**
 * FX parameter type enum for type-safe validation
 */
enum class EFXParamType : uint8
{
	Float,
	Int,
	Bool,
	Vec2,
	Vec3,
	Color,
	Unknown
};

/**
 * Expected parameter definition for template validation
 */
struct FFXExpectedParam
{
	FName Name;           // Full parameter name (e.g., "User.SpawnRate")
	EFXParamType Type;    // Expected type
	bool bRequired;       // If true, missing param is fatal error
	float MinValue = 0.f; // Optional min value for range validation (floats/ints)
	float MaxValue = 0.f; // Optional max value (0 = no limit)

	FFXExpectedParam() = default;
	FFXExpectedParam(FName InName, EFXParamType InType, bool InRequired = false)
		: Name(InName), Type(InType), bRequired(InRequired) {}
	FFXExpectedParam(FName InName, EFXParamType InType, bool InRequired, float InMin, float InMax)
		: Name(InName), Type(InType), bRequired(InRequired), MinValue(InMin), MaxValue(InMax) {}
};

/**
 * Single validation error from FX generation
 */
struct FFXValidationError
{
	FString AssetPath;    // Path to the asset with the error
	FString ParamName;    // Parameter name involved (if applicable)
	FString Message;      // Human-readable error message
	bool bFatal;          // If true, generation should abort

	FFXValidationError() : bFatal(false) {}
	FFXValidationError(const FString& InPath, const FString& InMessage, bool InFatal = false)
		: AssetPath(InPath), Message(InMessage), bFatal(InFatal) {}
	FFXValidationError(const FString& InPath, const FString& InParam, const FString& InMessage, bool InFatal)
		: AssetPath(InPath), ParamName(InParam), Message(InMessage), bFatal(InFatal) {}
};

/**
 * Aggregated validation result for FX generation
 */
struct FFXValidationResult
{
	TArray<FFXValidationError> Errors;
	TArray<FFXValidationError> Warnings;
	bool bTemplateValid = true;
	int32 ParamsValidated = 0;
	int32 ParamsMissing = 0;
	int32 ParamsTypeMismatch = 0;

	bool HasFatalErrors() const
	{
		for (const auto& Error : Errors)
		{
			if (Error.bFatal) return true;
		}
		return false;
	}

	void AddError(const FFXValidationError& Error)
	{
		if (Error.bFatal)
		{
			Errors.Add(Error);
		}
		else
		{
			Warnings.Add(Error);
		}
	}

	FString GetSummary() const
	{
		return FString::Printf(TEXT("Validated: %d, Missing: %d, TypeMismatch: %d, Errors: %d, Warnings: %d"),
			ParamsValidated, ParamsMissing, ParamsTypeMismatch, Errors.Num(), Warnings.Num());
	}

	void Reset()
	{
		Errors.Empty();
		Warnings.Empty();
		bTemplateValid = true;
		ParamsValidated = 0;
		ParamsMissing = 0;
		ParamsTypeMismatch = 0;
	}
};

/**
 * Generator metadata stored on generated assets for regeneration safety
 * v2.9.1: Tracks source template, descriptor hash, and generator version
 */
struct FFXGeneratorMetadata
{
	FString GeneratorVersion = TEXT("2.9.1");  // Plugin version that generated this asset
	FString SourceTemplate;                     // Path to template system used
	uint64 DescriptorHash = 0;                  // Hash of FManifestFXDescriptor at generation time
	FDateTime GeneratedTime;                    // When the asset was generated
	bool bIsGenerated = false;                  // True if this asset was generator-created

	FFXGeneratorMetadata() : GeneratedTime(FDateTime::Now()) {}

	/**
	 * Check if the descriptor has changed since generation
	 */
	bool HasDescriptorChanged(uint64 NewHash) const
	{
		return DescriptorHash != NewHash;
	}

	/**
	 * Serialize to string for storage in asset metadata
	 */
	FString ToString() const
	{
		return FString::Printf(TEXT("Ver:%s|Tmpl:%s|Hash:%llu|Time:%s|Gen:%d"),
			*GeneratorVersion, *SourceTemplate, DescriptorHash,
			*GeneratedTime.ToString(), bIsGenerated ? 1 : 0);
	}

	/**
	 * Parse from string stored in asset metadata
	 */
	static FFXGeneratorMetadata FromString(const FString& Str)
	{
		FFXGeneratorMetadata Meta;
		TArray<FString> Parts;
		Str.ParseIntoArray(Parts, TEXT("|"));

		for (const FString& Part : Parts)
		{
			FString Key, Value;
			if (Part.Split(TEXT(":"), &Key, &Value))
			{
				if (Key == TEXT("Ver")) Meta.GeneratorVersion = Value;
				else if (Key == TEXT("Tmpl")) Meta.SourceTemplate = Value;
				else if (Key == TEXT("Hash")) Meta.DescriptorHash = FCString::Strtoui64(*Value, nullptr, 10);
				else if (Key == TEXT("Time")) FDateTime::Parse(Value, Meta.GeneratedTime);
				else if (Key == TEXT("Gen")) Meta.bIsGenerated = (Value == TEXT("1"));
			}
		}
		return Meta;
	}
};

// ============================================================================
// v3.0: Regen/Diff Safety System
// Universal metadata tracking and dry-run support for all generators
// ============================================================================

/**
 * v3.0: Universal generator metadata for regeneration tracking
 * Stored on each generated asset via UAssetUserData
 */
struct FGeneratorMetadata
{
	FString GeneratorId;           // "GA", "GE", "BP", "WBP", etc.
	FString ManifestPath;          // Full path to manifest.yaml
	FString ManifestAssetKey;      // Asset name in manifest (stable identifier)
	uint64 InputHash = 0;          // Hash of manifest definition at generation time
	uint64 OutputHash = 0;         // Hash of generated content (for manual edit detection)
	FString GeneratorVersion;      // Plugin version (e.g., "3.0.0")
	FDateTime Timestamp;           // When asset was last generated
	TArray<FString> Dependencies;  // Assets this asset depends on
	bool bIsGenerated = false;     // True if this asset was generator-created

	FGeneratorMetadata() : Timestamp(FDateTime::Now()) {}

	/** Check if manifest definition changed since generation */
	bool HasInputChanged(uint64 NewInputHash) const
	{
		return InputHash != NewInputHash;
	}

	/** Check if asset was manually edited since generation */
	bool HasOutputChanged(uint64 CurrentOutputHash) const
	{
		return OutputHash != CurrentOutputHash;
	}

	/** Serialize to string for storage in asset metadata */
	FString ToString() const
	{
		FString DepsStr = FString::Join(Dependencies, TEXT(";"));
		return FString::Printf(TEXT("GenId:%s|Path:%s|Key:%s|InHash:%llu|OutHash:%llu|Ver:%s|Time:%s|Deps:%s|Gen:%d"),
			*GeneratorId, *ManifestPath, *ManifestAssetKey,
			InputHash, OutputHash, *GeneratorVersion,
			*Timestamp.ToString(), *DepsStr, bIsGenerated ? 1 : 0);
	}

	/** Parse from serialized string stored in asset metadata */
	static FGeneratorMetadata FromString(const FString& Str)
	{
		FGeneratorMetadata Meta;
		TArray<FString> Parts;
		Str.ParseIntoArray(Parts, TEXT("|"));

		for (const FString& Part : Parts)
		{
			FString Key, Value;
			if (Part.Split(TEXT(":"), &Key, &Value))
			{
				if (Key == TEXT("GenId")) Meta.GeneratorId = Value;
				else if (Key == TEXT("Path")) Meta.ManifestPath = Value;
				else if (Key == TEXT("Key")) Meta.ManifestAssetKey = Value;
				else if (Key == TEXT("InHash")) Meta.InputHash = FCString::Strtoui64(*Value, nullptr, 10);
				else if (Key == TEXT("OutHash")) Meta.OutputHash = FCString::Strtoui64(*Value, nullptr, 10);
				else if (Key == TEXT("Ver")) Meta.GeneratorVersion = Value;
				else if (Key == TEXT("Time")) FDateTime::Parse(Value, Meta.Timestamp);
				else if (Key == TEXT("Deps")) Value.ParseIntoArray(Meta.Dependencies, TEXT(";"));
				else if (Key == TEXT("Gen")) Meta.bIsGenerated = (Value == TEXT("1"));
			}
		}
		return Meta;
	}
};

/**
 * v3.0: Regeneration policy for generator assets
 * Determines how the generator handles existing assets
 */
enum class ERegenPolicy : uint8
{
	/** Always regenerate (overwrite) - use with --force flag */
	Strict,

	/** Skip if manual edit detected - universal default for all generators */
	SkipIfModified,

	/** Future: Attempt to merge changes */
	Merge UMETA(Hidden)
};

/**
 * v3.0: Get default regen policy for a generator type
 * SkipIfModified is the universal default for all asset types.
 * This ensures manual edits are never overwritten without explicit --force flag.
 */
inline ERegenPolicy GetDefaultRegenPolicy(const FString& GeneratorId)
{
	// SkipIfModified is the universal default for all generators
	// Never overwrites manual edits without explicit --force flag
	return ERegenPolicy::SkipIfModified;
}

/**
 * v3.0: Dry run status for preview mode
 */
enum class EDryRunStatus : uint8
{
	WillCreate,      // New asset, will be created
	WillModify,      // Manifest changed, no manual edits, will regenerate
	WillSkip,        // No changes needed (hashes match)
	Conflicted,      // Manifest changed AND asset was manually edited
	PolicySkip,      // Policy prevents regeneration (manual edits preserved)
};

/**
 * v3.0: Dry run result for a single asset
 */
struct FDryRunResult
{
	FString AssetName;
	FString AssetPath;
	FString GeneratorId;
	EDryRunStatus Status = EDryRunStatus::WillSkip;
	FString Reason;

	// Hash comparison info
	uint64 StoredInputHash = 0;
	uint64 CurrentInputHash = 0;
	uint64 StoredOutputHash = 0;
	uint64 CurrentOutputHash = 0;

	// v3.0: Detailed change tracking for conflicts
	TArray<FString> ManifestChanges;  // Fields that changed in manifest (e.g., "Tags.AbilityTags", "Variables")
	TArray<FString> AssetChanges;     // Detected manual edits (e.g., "Event graph modified", "Components added")

	// v3.0: Policy tracking
	ERegenPolicy Policy = ERegenPolicy::SkipIfModified;

	FDryRunResult() = default;

	FDryRunResult(const FString& InName, EDryRunStatus InStatus, const FString& InReason = TEXT(""))
		: AssetName(InName), Status(InStatus), Reason(InReason) {}

	FString GetStatusString() const
	{
		switch (Status)
		{
		case EDryRunStatus::WillCreate: return TEXT("CREATE");
		case EDryRunStatus::WillModify: return TEXT("MODIFY");
		case EDryRunStatus::WillSkip: return TEXT("SKIP");
		case EDryRunStatus::Conflicted: return TEXT("CONFLICT");
		case EDryRunStatus::PolicySkip: return TEXT("POLICY_SKIP");
		default: return TEXT("UNKNOWN");
		}
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("[%s] %s%s"),
			*GetStatusString(), *AssetName,
			Reason.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" - %s"), *Reason));
	}

	/** Get detailed conflict description */
	FString GetConflictDetails() const
	{
		if (Status != EDryRunStatus::Conflicted)
		{
			return TEXT("");
		}

		FString Details;
		if (ManifestChanges.Num() > 0)
		{
			Details += TEXT("  Manifest changes: ") + FString::Join(ManifestChanges, TEXT(", ")) + TEXT("\n");
		}
		if (AssetChanges.Num() > 0)
		{
			Details += TEXT("  Manual edits: ") + FString::Join(AssetChanges, TEXT(", ")) + TEXT("\n");
		}
		Details += TEXT("  Action: Use -force to override or resolve manually");
		return Details;
	}
};

/**
 * v3.0: Aggregated dry run results summary
 */
struct FDryRunSummary
{
	TArray<FDryRunResult> Results;
	int32 CreateCount = 0;
	int32 ModifyCount = 0;
	int32 SkipCount = 0;
	int32 ConflictCount = 0;
	int32 PolicySkipCount = 0;

	void AddResult(const FDryRunResult& Result)
	{
		Results.Add(Result);
		switch (Result.Status)
		{
		case EDryRunStatus::WillCreate: CreateCount++; break;
		case EDryRunStatus::WillModify: ModifyCount++; break;
		case EDryRunStatus::WillSkip: SkipCount++; break;
		case EDryRunStatus::Conflicted: ConflictCount++; break;
		case EDryRunStatus::PolicySkip: PolicySkipCount++; break;
		}
	}

	bool HasConflicts() const { return ConflictCount > 0; }
	int32 GetTotal() const { return CreateCount + ModifyCount + SkipCount + ConflictCount + PolicySkipCount; }

	FString GetSummary() const
	{
		if (PolicySkipCount > 0)
		{
			return FString::Printf(TEXT("Dry Run: %d CREATE, %d MODIFY, %d SKIP, %d CONFLICT, %d POLICY_SKIP"),
				CreateCount, ModifyCount, SkipCount, ConflictCount, PolicySkipCount);
		}
		return FString::Printf(TEXT("Dry Run: %d CREATE, %d MODIFY, %d SKIP, %d CONFLICT"),
			CreateCount, ModifyCount, SkipCount, ConflictCount);
	}

	void Reset()
	{
		Results.Empty();
		CreateCount = 0;
		ModifyCount = 0;
		SkipCount = 0;
		ConflictCount = 0;
		PolicySkipCount = 0;
	}

	/** Get results filtered by status */
	TArray<FDryRunResult> GetResultsByStatus(EDryRunStatus Status) const
	{
		return Results.FilterByPredicate([Status](const FDryRunResult& Result) {
			return Result.Status == Status;
		});
	}

	/** Get detailed conflict report */
	FString GetConflictReport() const
	{
		if (ConflictCount == 0)
		{
			return TEXT("");
		}

		FString Report = FString::Printf(TEXT("--- CONFLICTS (%d assets require attention) ---\n"), ConflictCount);
		for (const FDryRunResult& Result : Results)
		{
			if (Result.Status == EDryRunStatus::Conflicted)
			{
				Report += FString::Printf(TEXT("[CONFLICT] %s\n"), *Result.AssetName);
				Report += Result.GetConflictDetails();
				Report += TEXT("\n");
			}
		}
		return Report;
	}
};

/**
 * v2.6.10: Niagara System definition - creates UNiagaraSystem assets
 * Enhanced with warmup, bounds, determinism, and effect type settings
 * v2.6.11: Added user parameters support
 * v2.9.0: Added FX descriptor for data-driven parameter binding
 */
struct FManifestNiagaraSystemDefinition
{
	FString Name;
	FString Folder;
	FString TemplateSystem;     // Optional: System to copy from (e.g., NS_DefaultSprite)
	TArray<FString> Emitters;   // Optional: Emitters to add to new system

	// v2.6.10: Warmup settings
	float WarmupTime = 0.0f;           // Warmup time in seconds before system becomes visible
	int32 WarmupTickCount = 0;         // Number of ticks to process for warmup
	float WarmupTickDelta = 0.0333f;   // Delta time per warmup tick (default ~30fps)

	// v2.6.10: Bounds settings
	bool bFixedBounds = false;         // Use fixed bounds instead of dynamic
	FVector BoundsMin = FVector(-100.f);  // Min corner of fixed bounds
	FVector BoundsMax = FVector(100.f);   // Max corner of fixed bounds

	// v2.6.10: Determinism settings
	bool bDeterminism = false;         // Enable deterministic simulation
	int32 RandomSeed = 0;              // Random seed for deterministic mode

	// v2.6.10: Effect type settings
	FString EffectType;                // beam, burst, trail, ambient, impact, projectile
	FString PoolingMethod;             // None, AutoRelease, ManualRelease, FreeInWorld
	int32 MaxPoolSize = 0;             // Maximum pool size (0 = no pooling)

	// v2.6.11: User parameters
	TArray<FManifestNiagaraUserParameter> UserParameters;  // User-exposed parameters

	// v2.9.0: FX Descriptor for data-driven parameter binding
	FManifestFXDescriptor FXDescriptor;

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		Hash ^= GetTypeHash(TemplateSystem) << 4;
		for (const FString& Emitter : Emitters)
		{
			Hash ^= GetTypeHash(Emitter);
			Hash = (Hash << 3) | (Hash >> 61);
		}

		// Warmup settings
		Hash ^= static_cast<uint64>(FMath::RoundToInt(WarmupTime * 1000.f)) << 8;
		Hash ^= static_cast<uint64>(WarmupTickCount) << 16;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(WarmupTickDelta * 10000.f)) << 24;

		// Bounds settings
		Hash ^= (bFixedBounds ? 1ULL : 0ULL) << 32;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(BoundsMin.X * 10.f)) << 33;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(BoundsMax.X * 10.f)) << 40;

		// Determinism settings
		Hash ^= (bDeterminism ? 1ULL : 0ULL) << 48;
		Hash ^= static_cast<uint64>(RandomSeed) << 49;

		// Effect type settings
		Hash ^= GetTypeHash(EffectType);
		Hash ^= GetTypeHash(PoolingMethod) << 4;
		Hash ^= static_cast<uint64>(MaxPoolSize) << 56;

		// User parameters
		for (const auto& Param : UserParameters)
		{
			Hash ^= Param.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}

		// FX Descriptor
		Hash ^= FXDescriptor.ComputeHash();

		return Hash;
	}
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
	TArray<FManifestMaterialFunctionDefinition> MaterialFunctions;  // v2.6.12: Material functions

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
		for (const auto& Def : MaterialFunctions) AssetWhitelist.Add(Def.Name);  // v2.6.12

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
	 * v2.8.4: Get total expected asset count (excludes EventGraphs which are embedded, not standalone)
	 */
	int32 GetExpectedAssetCount() const
	{
		return Enumerations.Num()
			+ InputActions.Num()
			+ InputMappingContexts.Num()
			+ GameplayEffects.Num()
			+ GameplayAbilities.Num()
			+ ActorBlueprints.Num()
			+ WidgetBlueprints.Num()
			+ Blackboards.Num()
			+ BehaviorTrees.Num()
			+ Materials.Num()
			// EventGraphs are embedded in other assets, not standalone
			+ FloatCurves.Num()
			+ AnimationMontages.Num()
			+ AnimationNotifies.Num()
			+ DialogueBlueprints.Num()
			+ EquippableItems.Num()
			+ Activities.Num()
			+ AbilityConfigurations.Num()
			+ ActivityConfigurations.Num()
			+ ItemCollections.Num()
			+ NarrativeEvents.Num()
			+ NPCDefinitions.Num()
			+ CharacterDefinitions.Num()
			+ TaggedDialogueSets.Num()
			+ NiagaraSystems.Num()
			+ MaterialFunctions.Num();
	}

	/**
	 * v2.8.4: Get all expected asset names for verification
	 * Also detects duplicates via OutDuplicates
	 */
	TSet<FString> GetExpectedAssetNames(TArray<FString>* OutDuplicates = nullptr) const
	{
		TSet<FString> Names;
		auto AddWithDupeCheck = [&Names, OutDuplicates](const FString& Name) {
			if (Names.Contains(Name))
			{
				if (OutDuplicates) OutDuplicates->AddUnique(Name);
			}
			else
			{
				Names.Add(Name);
			}
		};

		for (const auto& Def : Enumerations) AddWithDupeCheck(Def.Name);
		for (const auto& Def : InputActions) AddWithDupeCheck(Def.Name);
		for (const auto& Def : InputMappingContexts) AddWithDupeCheck(Def.Name);
		for (const auto& Def : GameplayEffects) AddWithDupeCheck(Def.Name);
		for (const auto& Def : GameplayAbilities) AddWithDupeCheck(Def.Name);
		for (const auto& Def : ActorBlueprints) AddWithDupeCheck(Def.Name);
		for (const auto& Def : WidgetBlueprints) AddWithDupeCheck(Def.Name);
		for (const auto& Def : Blackboards) AddWithDupeCheck(Def.Name);
		for (const auto& Def : BehaviorTrees) AddWithDupeCheck(Def.Name);
		for (const auto& Def : Materials) AddWithDupeCheck(Def.Name);
		// EventGraphs excluded - embedded in other assets
		for (const auto& Def : FloatCurves) AddWithDupeCheck(Def.Name);
		for (const auto& Def : AnimationMontages) AddWithDupeCheck(Def.Name);
		for (const auto& Def : AnimationNotifies) AddWithDupeCheck(Def.Name);
		for (const auto& Def : DialogueBlueprints) AddWithDupeCheck(Def.Name);
		for (const auto& Def : EquippableItems) AddWithDupeCheck(Def.Name);
		for (const auto& Def : Activities) AddWithDupeCheck(Def.Name);
		for (const auto& Def : AbilityConfigurations) AddWithDupeCheck(Def.Name);
		for (const auto& Def : ActivityConfigurations) AddWithDupeCheck(Def.Name);
		for (const auto& Def : ItemCollections) AddWithDupeCheck(Def.Name);
		for (const auto& Def : NarrativeEvents) AddWithDupeCheck(Def.Name);
		for (const auto& Def : NPCDefinitions) AddWithDupeCheck(Def.Name);
		for (const auto& Def : CharacterDefinitions) AddWithDupeCheck(Def.Name);
		for (const auto& Def : TaggedDialogueSets) AddWithDupeCheck(Def.Name);
		for (const auto& Def : NiagaraSystems) AddWithDupeCheck(Def.Name);
		for (const auto& Def : MaterialFunctions) AddWithDupeCheck(Def.Name);
		return Names;
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
		Total += MaterialFunctions.Num();  // v2.6.12

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
