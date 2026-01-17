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

	// v4.7: Report system fields for machine-readable audit trail
	FString AssetPath;    // Full asset path (e.g., "/Game/FatherCompanion/Abilities/GA_FatherAttack")
	FString GeneratorId;  // Generator identifier (e.g., "GameplayAbility", "GameplayEffect")

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

	// Helper to determine category from GeneratorId or asset name prefix
	// v4.8.2: Prefer GeneratorId over prefix for accurate categorization (AC_ used for both Ability and Activity configs)
	void DetermineCategory()
	{
		// First check GeneratorId for accurate categorization
		if (!GeneratorId.IsEmpty())
		{
			if (GeneratorId == TEXT("AbilityConfiguration")) { Category = TEXT("Ability Configurations"); return; }
			if (GeneratorId == TEXT("ActivityConfiguration")) { Category = TEXT("Activity Configurations"); return; }
			if (GeneratorId == TEXT("GameplayAbility")) { Category = TEXT("Gameplay Abilities"); return; }
			if (GeneratorId == TEXT("GameplayEffect")) { Category = TEXT("Gameplay Effects"); return; }
			if (GeneratorId == TEXT("ActorBlueprint")) { Category = TEXT("Actor Blueprints"); return; }
			if (GeneratorId == TEXT("WidgetBlueprint")) { Category = TEXT("Widget Blueprints"); return; }
			if (GeneratorId == TEXT("DialogueBlueprint")) { Category = TEXT("Dialogue Blueprints"); return; }
			if (GeneratorId == TEXT("NPCDefinition")) { Category = TEXT("NPC Definitions"); return; }
			if (GeneratorId == TEXT("CharacterDefinition")) { Category = TEXT("Character Definitions"); return; }
			if (GeneratorId == TEXT("NarrativeEvent")) { Category = TEXT("Narrative Events"); return; }
			if (GeneratorId == TEXT("EquippableItem")) { Category = TEXT("Equippable Items"); return; }
			if (GeneratorId == TEXT("Activity")) { Category = TEXT("Activities"); return; }
			if (GeneratorId == TEXT("Blackboard")) { Category = TEXT("Blackboards"); return; }
			if (GeneratorId == TEXT("BehaviorTree")) { Category = TEXT("Behavior Trees"); return; }
			if (GeneratorId == TEXT("NiagaraSystem")) { Category = TEXT("Niagara Systems"); return; }
			// GeneratorId exists but not mapped - fall through to prefix detection
		}

		// Fallback to prefix detection for backwards compatibility
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
		else if (AssetName.StartsWith(TEXT("AC_"))) Category = TEXT("Configurations");  // v4.8.2: Generic - use GeneratorId for specificity
		else if (AssetName.StartsWith(TEXT("NPC_"))) Category = TEXT("NPC Definitions");
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
 * v4.10: Material Expression validation error codes
 * Used by the 6-guardrail validation system for hallucination-proof expression generation
 */
enum class EMaterialExprValidationCode : uint8
{
	// Errors (fatal - block generation)
	E_UNKNOWN_EXPRESSION_TYPE,      // Unrecognized expression type
	E_UNKNOWN_SWITCH_KEY,           // Unknown key in switch inputs
	E_FUNCTION_NOT_FOUND,           // MaterialFunctionCall target not found
	E_FUNCTION_INSTANCE_BLOCKED,    // UMaterialFunctionInstance not allowed (use UMaterialFunction)
	E_MISSING_REQUIRED_INPUT,       // Required switch input not connected
	E_DUPLICATE_EXPRESSION_ID,      // Duplicate expression ID in material
	E_EXPRESSION_REFERENCE_INVALID, // Expression ID referenced but not defined

	// Warnings (non-fatal)
	W_DEPRECATED_SWITCH_KEY,        // Deprecated enum value (es2, sm4)
	W_FUNCTION_INPUT_MISMATCH,      // Function input count doesn't match (reserved - requires asset loading)
	W_FUNCTION_PATH_NORMALIZED,     // Path was auto-corrected
	W_EXPRESSION_UNUSED,            // Expression defined but never connected
};

/**
 * v4.10: Single diagnostic from material expression validation
 */
struct FMaterialExprDiagnostic
{
	EMaterialExprValidationCode Code;
	FString ContextPath;      // Path in manifest (e.g., "/Materials/M_Fire/Expressions/QualityMux")
	FString UserInput;        // Original user input (for case hints)
	FString Message;          // Human-readable message
	FString SuggestedFix;     // Optional fix suggestion
	bool bFatal = false;

	FMaterialExprDiagnostic() = default;

	FMaterialExprDiagnostic(EMaterialExprValidationCode InCode, const FString& InPath, const FString& InMessage, bool InFatal = true)
		: Code(InCode), ContextPath(InPath), Message(InMessage), bFatal(InFatal) {}

	// With user input for case hints (Guardrail #2)
	FMaterialExprDiagnostic(EMaterialExprValidationCode InCode, const FString& InPath, const FString& InUserInput, const FString& InMessage, bool InFatal)
		: Code(InCode), ContextPath(InPath), UserInput(InUserInput), Message(InMessage), bFatal(InFatal) {}

	// Get error code as string for logging
	FString GetCodeString() const
	{
		switch (Code)
		{
		case EMaterialExprValidationCode::E_UNKNOWN_EXPRESSION_TYPE: return TEXT("E_UNKNOWN_EXPRESSION_TYPE");
		case EMaterialExprValidationCode::E_UNKNOWN_SWITCH_KEY: return TEXT("E_UNKNOWN_SWITCH_KEY");
		case EMaterialExprValidationCode::E_FUNCTION_NOT_FOUND: return TEXT("E_FUNCTION_NOT_FOUND");
		case EMaterialExprValidationCode::E_FUNCTION_INSTANCE_BLOCKED: return TEXT("E_FUNCTION_INSTANCE_BLOCKED");
		case EMaterialExprValidationCode::E_MISSING_REQUIRED_INPUT: return TEXT("E_MISSING_REQUIRED_INPUT");
		case EMaterialExprValidationCode::E_DUPLICATE_EXPRESSION_ID: return TEXT("E_DUPLICATE_EXPRESSION_ID");
		case EMaterialExprValidationCode::E_EXPRESSION_REFERENCE_INVALID: return TEXT("E_EXPRESSION_REFERENCE_INVALID");
		case EMaterialExprValidationCode::W_DEPRECATED_SWITCH_KEY: return TEXT("W_DEPRECATED_SWITCH_KEY");
		case EMaterialExprValidationCode::W_FUNCTION_INPUT_MISMATCH: return TEXT("W_FUNCTION_INPUT_MISMATCH");
		case EMaterialExprValidationCode::W_FUNCTION_PATH_NORMALIZED: return TEXT("W_FUNCTION_PATH_NORMALIZED");
		case EMaterialExprValidationCode::W_EXPRESSION_UNUSED: return TEXT("W_EXPRESSION_UNUSED");
		default: return TEXT("UNKNOWN");
		}
	}

	// Format for logging with stable ordering (Guardrail #6)
	FString ToString() const
	{
		return FString::Printf(TEXT("[%s] %s: %s%s"),
			*GetCodeString(),
			*ContextPath,
			*Message,
			SuggestedFix.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" (Fix: %s)"), *SuggestedFix));
	}

	// Compare for stable sorting (Guardrail #6: path, then code, then message)
	bool operator<(const FMaterialExprDiagnostic& Other) const
	{
		if (ContextPath != Other.ContextPath) return ContextPath < Other.ContextPath;
		if (Code != Other.Code) return Code < Other.Code;
		return Message < Other.Message;
	}
};

/**
 * v4.10: Aggregated validation result for material expressions
 * Implements the 6-guardrail validation system
 */
struct FMaterialExprValidationResult
{
	TArray<FMaterialExprDiagnostic> Diagnostics;
	bool bValidated = false;
	int32 ErrorCount = 0;
	int32 WarningCount = 0;

	void AddDiagnostic(const FMaterialExprDiagnostic& Diag)
	{
		Diagnostics.Add(Diag);
		if (Diag.bFatal) ErrorCount++;
		else WarningCount++;
	}

	bool HasErrors() const { return ErrorCount > 0; }
	bool HasWarnings() const { return WarningCount > 0; }
	bool IsValid() const { return ErrorCount == 0; }

	// Guardrail #6: Sort diagnostics for stable output before returning
	void SortDiagnostics()
	{
		Diagnostics.Sort();
	}

	// Get summary string
	FString GetSummary() const
	{
		return FString::Printf(TEXT("Material Expression Validation: %d errors, %d warnings"),
			ErrorCount, WarningCount);
	}

	// =========================================================================
	// Guardrail Helper Functions (6 Guardrails)
	// =========================================================================

	// Guardrail #1: Quote Strip Before Trim
	// Remove surrounding quotes before trimming whitespace
	static FString StripQuotesAndTrim(const FString& Input)
	{
		FString Result = Input;
		// Remove surrounding quotes (single or double)
		if (Result.Len() >= 2)
		{
			if ((Result.StartsWith(TEXT("\"")) && Result.EndsWith(TEXT("\""))) ||
				(Result.StartsWith(TEXT("'")) && Result.EndsWith(TEXT("'"))))
			{
				Result = Result.Mid(1, Result.Len() - 2);
			}
		}
		// Then trim whitespace
		return Result.TrimStartAndEnd();
	}

	// Guardrail #3: ContextPath Root Convention
	// Ensure context path starts with / for consistency
	static FString MakeContextPath(const FString& MaterialName, const FString& Section, const FString& ExpressionId = TEXT(""))
	{
		FString Path = FString::Printf(TEXT("/Materials/%s/%s"), *MaterialName, *Section);
		if (!ExpressionId.IsEmpty())
		{
			Path += FString::Printf(TEXT("/%s"), *ExpressionId);
		}
		return Path;
	}

	// Guardrail #5a: Field-Specific Normalizer - Keys (lowercase, trim)
	// For enum keys in switch expressions
	static FString NormalizeKey(const FString& Key)
	{
		return StripQuotesAndTrim(Key).ToLower();
	}

	// Guardrail #5b: Field-Specific Normalizer - Scalars (parse, validate)
	static bool NormalizeScalar(const FString& Value, float& OutValue, float Min = -FLT_MAX, float Max = FLT_MAX)
	{
		FString Cleaned = StripQuotesAndTrim(Value);
		if (Cleaned.IsEmpty()) return false;
		OutValue = FCString::Atof(*Cleaned);
		return OutValue >= Min && OutValue <= Max;
	}

	// Guardrail #5c: Field-Specific Normalizer - Paths (forward slashes, no trailing)
	static FString NormalizePath(const FString& Path)
	{
		FString Result = StripQuotesAndTrim(Path);
		// Convert backslashes to forward slashes
		Result.ReplaceInline(TEXT("\\"), TEXT("/"));
		// Remove trailing slash
		while (Result.EndsWith(TEXT("/")))
		{
			Result = Result.LeftChop(1);
		}
		return Result;
	}
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
 * v4.8: Added InputTag and bActivateAbilityOnGranted for input binding automation
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

	// v4.8: NarrativeGameplayAbility properties
	FString InputTag;                    // Maps ability to input action (Narrative.Input.*)
	bool bActivateAbilityOnGranted = false;  // Auto-activate when ability is granted

	// Variables defined on the ability Blueprint
	TArray<FManifestActorVariableDefinition> Variables;

	// Inline event graph - stored by name, looked up from EventGraphs array
	FString EventGraphName;
	bool bHasInlineEventGraph = false;

	// Inline event graph data (populated during parsing, used during generation)
	TArray<FManifestGraphNodeDefinition> EventGraphNodes;
	TArray<FManifestGraphConnectionDefinition> EventGraphConnections;

	/** v4.8: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(ParentClass) << 4;
		// NOTE: Folder excluded - presentational only
		Hash ^= GetTypeHash(InstancingPolicy) << 8;
		Hash ^= GetTypeHash(NetExecutionPolicy) << 12;
		Hash ^= GetTypeHash(CooldownGameplayEffectClass) << 16;
		Hash ^= Tags.ComputeHash() << 20;

		// v4.8: Include new NarrativeGameplayAbility properties
		Hash ^= static_cast<uint64>(GetTypeHash(InputTag)) << 32;
		Hash ^= (bActivateAbilityOnGranted ? 1ULL : 0ULL) << 40;

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
 * v4.3: Widget slot configuration for panel children
 * Supports CanvasPanel, VerticalBox, HorizontalBox, Overlay, etc.
 */
struct FManifestWidgetSlotDefinition
{
	// Canvas Panel slot properties
	FString Anchors;              // TopLeft, TopCenter, TopRight, CenterLeft, Center, CenterRight, BottomLeft, BottomCenter, BottomRight, Stretch
	FString Position;             // "X,Y" offset from anchor
	FString Size;                 // "W,H" size override (for Canvas)
	FString Alignment;            // "X,Y" pivot point (0-1)
	bool bAutoSize = true;        // Auto-size to content

	// Box slot properties (Vertical/Horizontal)
	FString HorizontalAlignment;  // Left, Center, Right, Fill
	FString VerticalAlignment;    // Top, Center, Bottom, Fill
	FString SizeRule;             // Auto, Fill
	float FillWeight = 1.0f;      // Weight when using Fill

	// Padding (all slot types)
	FString Padding;              // "L,T,R,B" or single value for all sides

	uint64 ComputeHash() const
	{
		uint64 Hash = static_cast<uint64>(GetTypeHash(Anchors));
		Hash ^= static_cast<uint64>(GetTypeHash(Position)) << 4;
		Hash ^= static_cast<uint64>(GetTypeHash(Size)) << 8;
		Hash ^= static_cast<uint64>(GetTypeHash(Alignment)) << 12;
		Hash ^= (bAutoSize ? 1ULL : 0ULL) << 16;
		Hash ^= static_cast<uint64>(GetTypeHash(HorizontalAlignment)) << 20;
		Hash ^= static_cast<uint64>(GetTypeHash(VerticalAlignment)) << 24;
		Hash ^= static_cast<uint64>(GetTypeHash(SizeRule)) << 28;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(FillWeight * 1000.f)) << 32;
		Hash ^= static_cast<uint64>(GetTypeHash(Padding)) << 40;
		return Hash;
	}
};

/**
 * v4.3: Widget node definition for widget tree construction
 * Supports: CanvasPanel, VerticalBox, HorizontalBox, Overlay, Border, Button, TextBlock, Image, ProgressBar, etc.
 */
struct FManifestWidgetNodeDefinition
{
	FString Id;                   // Unique ID for this widget (becomes variable name if bIsVariable=true)
	FString Type;                 // Widget class: CanvasPanel, VerticalBox, HorizontalBox, Overlay, Border, Button, TextBlock, Image, ProgressBar, Spacer, etc.
	FString Name;                 // Display name in editor
	bool bIsVariable = false;     // Expose as Blueprint variable (BindWidget)

	// Slot configuration (how this widget sits in its parent)
	FManifestWidgetSlotDefinition Slot;

	// Common properties (set via reflection)
	TMap<FString, FString> Properties;  // e.g., Text, ColorAndOpacity, Brush, Visibility, ToolTipText

	// Child widgets (for panel types)
	TArray<FString> Children;     // IDs of child widgets

	// Specialized properties
	FString Text;                 // For TextBlock
	FString ImagePath;            // For Image (texture path)
	FString StyleClass;           // For styled widgets

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Id);
		Hash ^= GetTypeHash(Type) << 4;
		Hash ^= GetTypeHash(Name) << 8;
		Hash ^= (bIsVariable ? 1ULL : 0ULL) << 12;
		Hash ^= Slot.ComputeHash() << 16;
		for (const auto& Prop : Properties)
		{
			Hash ^= GetTypeHash(Prop.Key);
			Hash ^= GetTypeHash(Prop.Value) << 4;
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const FString& Child : Children)
		{
			Hash ^= GetTypeHash(Child);
			Hash = (Hash << 5) | (Hash >> 59);
		}
		Hash ^= GetTypeHash(Text) << 20;
		Hash ^= GetTypeHash(ImagePath) << 24;
		Hash ^= GetTypeHash(StyleClass) << 28;
		return Hash;
	}
};

/**
 * v4.3: Widget tree definition for full visual layout
 */
struct FManifestWidgetTreeDefinition
{
	FString RootWidget;           // ID of root widget (usually a CanvasPanel)
	TArray<FManifestWidgetNodeDefinition> Widgets;

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(RootWidget);
		for (const auto& Widget : Widgets)
		{
			Hash ^= Widget.ComputeHash();
			Hash = (Hash << 7) | (Hash >> 57);
		}
		return Hash;
	}
};

/**
 * Widget blueprint definition
 * v4.3: Added WidgetTree for full visual layout automation
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

	// v4.3: Widget tree for visual layout
	FManifestWidgetTreeDefinition WidgetTree;

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
		// v4.3: Include widget tree in hash
		Hash ^= WidgetTree.ComputeHash();
		return Hash;
	}
};

/**
 * Blackboard key definition
 * v4.0: Added BaseClass for Object/Class key types
 */
struct FManifestBlackboardKeyDefinition
{
	FString Name;
	FString Type;  // Bool, Int, Float, String, Name, Vector, Rotator, Object, Class, Enum
	bool bInstanceSynced = false;

	// v4.0: Base class for Object/Class key types (e.g., "Actor", "Pawn", "NarrativeNPCCharacter")
	// Only used when Type is "Object" or "Class"
	FString BaseClass;

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(Type) << 8;
		Hash ^= (bInstanceSynced ? 1ULL : 0ULL) << 16;
		Hash ^= GetTypeHash(BaseClass) << 24;  // v4.0
		return Hash;
	}
};

/**
 * Blackboard definition
 * v4.0: Added Parent blackboard for inheritance
 */
struct FManifestBlackboardDefinition
{
	FString Name;
	TArray<FManifestBlackboardKeyDefinition> Keys;

	// v4.0: Parent blackboard for inheritance (keys are inherited from parent)
	FString Parent;

	/** v3.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(Parent) << 4;  // v4.0
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
 * v4.0: Added bInverseCondition, FlowAbortMode for enhanced decorator configuration
 */
struct FManifestBTDecoratorDefinition
{
	FString Class;                        // Decorator class (e.g., BTDecorator_Blackboard, custom BP class)
	FString BlackboardKey;                // Blackboard key to check (if applicable)
	FString Operation;                    // Condition operation (IsSet, IsNotSet, etc.)
	TMap<FString, FString> Properties;    // Additional properties

	// v4.0: Enhanced decorator configuration
	bool bInverseCondition = false;       // Invert the decorator condition
	FString FlowAbortMode;                // None, Self, LowerPriority, Both

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Class);
		Hash ^= GetTypeHash(BlackboardKey) << 8;
		Hash ^= GetTypeHash(Operation) << 16;
		Hash ^= static_cast<uint64>(bInverseCondition ? 1 : 0) << 24;
		Hash ^= GetTypeHash(FlowAbortMode) << 28;
		for (const auto& Prop : Properties)
		{
			Hash ^= GetTypeHash(Prop.Key);
			Hash ^= GetTypeHash(Prop.Value) << 4;
		}
		return Hash;
	}
};

/**
 * v3.1: BehaviorTree service definition
 * v4.0: Added RandomDeviation, bCallTickOnSearchStart, bRestartTimerOnActivation
 */
struct FManifestBTServiceDefinition
{
	FString Class;                        // Service class (e.g., BTS_UpdateTarget, custom BP class)
	float Interval = 0.5f;                // Tick interval
	TMap<FString, FString> Properties;    // Additional properties

	// v4.0: Enhanced service configuration
	float RandomDeviation = 0.0f;         // Random deviation for interval
	bool bCallTickOnSearchStart = false;  // Call tick when search starts
	bool bRestartTimerOnActivation = true; // Restart timer on activation

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Class);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(Interval * 1000.f)) << 8;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(RandomDeviation * 1000.f)) << 16;
		Hash ^= static_cast<uint64>(bCallTickOnSearchStart ? 1 : 0) << 24;
		Hash ^= static_cast<uint64>(bRestartTimerOnActivation ? 1 : 0) << 28;
		for (const auto& Prop : Properties)
		{
			Hash ^= GetTypeHash(Prop.Key);
			Hash ^= GetTypeHash(Prop.Value) << 4;
		}
		return Hash;
	}
};

/**
 * v3.1: BehaviorTree node definition (task or composite)
 * v4.0: Properties now included in hash for change detection
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
	TMap<FString, FString> Properties;    // Additional task properties (v4.0: set via reflection)

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
		// v4.0: Include properties in hash
		for (const auto& Prop : Properties)
		{
			Hash ^= GetTypeHash(Prop.Key);
			Hash ^= GetTypeHash(Prop.Value) << 4;
			Hash = (Hash << 2) | (Hash >> 62);
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
 * v4.0: Added TexturePath, SamplerType for texture expressions
 */
struct FManifestMaterialExpression
{
	FString Id;              // Unique identifier for this node (e.g., "tex_diffuse", "param_color")
	FString Type;            // Expression type: TextureSample, ScalarParameter, VectorParameter, Multiply, Add, Fresnel, etc.
	FString Name;            // Display name (for parameters)
	FString DefaultValue;    // Default value as string (parsed based on type)
	int32 PosX = 0;          // Node position X in graph
	int32 PosY = 0;          // Node position Y in graph
	TMap<FString, FString> Properties;  // Additional properties (e.g., Exponent, etc.)

	// v4.0: Texture-specific properties
	FString TexturePath;     // Path to texture asset (e.g., "/Game/Textures/T_Fire")
	FString SamplerType;     // Color, LinearColor, Normal, Masks, Grayscale, Alpha, DistanceFieldFont

	// v4.10: Switch expression inputs (QualitySwitch, ShadingPathSwitch, FeatureLevelSwitch)
	// Keys: "default", "low", "medium", "high", "epic" (Quality)
	//       "default", "deferred", "forward", "mobile" (ShadingPath)
	//       "default", "es3_1", "sm5", "sm6" (FeatureLevel)
	// Values: Expression ID to connect to that input
	TMap<FString, FString> Inputs;

	// v4.10: MaterialFunctionCall properties
	FString Function;        // Function path: /Engine/..., /Game/..., or MF_Name (3-tier resolution)
	TMap<FString, FString> FunctionInputs;  // Maps function input name to expression ID

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
		// v4.0: Include texture properties in hash
		Hash ^= GetTypeHash(TexturePath) << 16;
		Hash ^= GetTypeHash(SamplerType) << 20;
		// v4.10: Include switch inputs in hash (use prime multiplication to avoid collisions)
		uint32 InputsHash = 0;
		for (const auto& Input : Inputs)
		{
			InputsHash = InputsHash * 31 + GetTypeHash(Input.Key);
			InputsHash = InputsHash * 37 + GetTypeHash(Input.Value);
		}
		Hash ^= InputsHash;
		// v4.10: Include function call properties in hash
		uint32 FuncHash = GetTypeHash(Function);
		for (const auto& FuncInput : FunctionInputs)
		{
			FuncHash = FuncHash * 41 + GetTypeHash(FuncInput.Key);
			FuncHash = FuncHash * 43 + GetTypeHash(FuncInput.Value);
		}
		Hash ^= FuncHash << 4;  // Slight offset to reduce XOR collisions
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
 * v4.0: Added MaterialDomain, CullMode, translucency/decal/lighting properties
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

	// v4.0: Extended material properties
	FString MaterialDomain = TEXT("Surface");     // Surface, DeferredDecal, LightFunction, Volume, PostProcess, UI
	FString CullMode;                             // None, Front, Back (empty = default for blend mode)
	float OpacityMaskClipValue = 0.333f;          // Opacity mask clip threshold
	FString TranslucencyPass;                     // BeforeDOF, AfterDOF, AfterMotionBlur
	bool bEnableSeparateTranslucency = false;     // Render in separate translucency pass
	bool bEnableResponsiveAA = false;             // Enable responsive AA for translucent objects
	FString DecalResponse;                        // None, Color, Normal, ColorNormalRoughness, etc.
	bool bCastDynamicShadow = true;               // Cast dynamic shadows
	bool bAffectDynamicIndirectLighting = true;   // Affect indirect lighting
	bool bBlockGI = false;                        // Block global illumination

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
		// v4.0: Include extended properties
		Hash ^= GetTypeHash(MaterialDomain) << 16;
		Hash ^= GetTypeHash(CullMode) << 20;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(OpacityMaskClipValue * 1000.f)) << 24;
		Hash ^= GetTypeHash(TranslucencyPass) << 28;
		Hash ^= (bEnableSeparateTranslucency ? 1ULL : 0ULL) << 32;
		Hash ^= (bEnableResponsiveAA ? 1ULL : 0ULL) << 33;
		Hash ^= static_cast<uint64>(GetTypeHash(DecalResponse)) << 36;
		Hash ^= (bCastDynamicShadow ? 1ULL : 0ULL) << 40;
		Hash ^= (bAffectDynamicIndirectLighting ? 1ULL : 0ULL) << 41;
		Hash ^= (bBlockGI ? 1ULL : 0ULL) << 42;
		return Hash;
	}
};

/**
 * v4.9: Material Instance Scalar Parameter Override
 */
struct FManifestMaterialInstanceScalarParam
{
	FString Name;
	float Value = 0.0f;

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(Value * 10000.f)) << 8;
		return Hash;
	}
};

/**
 * v4.9: Material Instance Vector Parameter Override
 */
struct FManifestMaterialInstanceVectorParam
{
	FString Name;
	FLinearColor Value = FLinearColor::White;

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(Value.R * 1000.f)) << 8;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(Value.G * 1000.f)) << 16;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(Value.B * 1000.f)) << 24;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(Value.A * 1000.f)) << 32;
		return Hash;
	}
};

/**
 * v4.9: Material Instance Texture Parameter Override
 */
struct FManifestMaterialInstanceTextureParam
{
	FString Name;
	FString TexturePath;

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(TexturePath) << 8;
		return Hash;
	}
};

/**
 * v4.9: Material Instance Constant (MIC_) definition
 * Parameterized instances of parent materials
 */
struct FManifestMaterialInstanceDefinition
{
	FString Name;
	FString Folder;
	FString ParentMaterial;                                      // Path to parent material (M_*)
	TArray<FManifestMaterialInstanceScalarParam> ScalarParams;   // Scalar parameter overrides
	TArray<FManifestMaterialInstanceVectorParam> VectorParams;   // Vector parameter overrides
	TArray<FManifestMaterialInstanceTextureParam> TextureParams; // Texture parameter overrides

	/** v4.9: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(ParentMaterial) << 4;
		for (const auto& Param : ScalarParams)
		{
			Hash ^= Param.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const auto& Param : VectorParams)
		{
			Hash ^= Param.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}
		for (const auto& Param : TextureParams)
		{
			Hash ^= Param.ComputeHash();
			Hash = (Hash << 7) | (Hash >> 57);
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
 * v4.0: Float curve key definition with interpolation and tangent support
 */
struct FManifestFloatCurveKeyDefinition
{
	float Time = 0.0f;
	float Value = 0.0f;

	// v4.0: Interpolation mode (maps to ERichCurveInterpMode)
	// Values: "Linear" (default), "Constant", "Cubic", "None"
	FString InterpMode = TEXT("Linear");

	// v4.0: Tangent mode (maps to ERichCurveTangentMode)
	// Values: "Auto" (default), "User", "Break", "None"
	FString TangentMode = TEXT("Auto");

	// v4.0: Tangent values (only used when TangentMode is User or Break)
	float ArriveTangent = 0.0f;
	float LeaveTangent = 0.0f;

	uint64 ComputeHash() const
	{
		uint64 Hash = static_cast<uint64>(FMath::RoundToInt(Time * 1000.f));
		Hash ^= static_cast<uint64>(FMath::RoundToInt(Value * 1000.f)) << 16;
		Hash ^= static_cast<uint64>(GetTypeHash(InterpMode)) << 32;
		Hash ^= static_cast<uint64>(GetTypeHash(TangentMode)) << 40;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(ArriveTangent * 100.f)) << 48;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(LeaveTangent * 100.f)) << 56;
		return Hash;
	}
};

/**
 * Float curve definition
 * v4.0: Enhanced with interpolation modes, tangent control, and extrapolation
 */
struct FManifestFloatCurveDefinition
{
	FString Name;
	FString Folder;

	// v4.0: Keys with full interpolation/tangent support (replaces TPair array)
	TArray<FManifestFloatCurveKeyDefinition> Keys;

	// v4.0: Extrapolation modes (maps to ERichCurveExtrapolation)
	// Values: "Constant" (default), "Linear", "Cycle", "CycleWithOffset", "Oscillate", "None"
	FString ExtrapolationBefore = TEXT("Constant");
	FString ExtrapolationAfter = TEXT("Constant");

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		Hash ^= GetTypeHash(ExtrapolationBefore) << 4;
		Hash ^= GetTypeHash(ExtrapolationAfter) << 8;
		for (const auto& Key : Keys)
		{
			Hash ^= Key.ComputeHash();
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
 * v4.0: Enhanced with event graph and variables support
 */
struct FManifestAnimationNotifyDefinition
{
	FString Name;
	FString Folder = TEXT("Animations/Notifies");
	FString NotifyClass;  // AnimNotify or AnimNotifyState parent

	// v4.0: Variables for the notify Blueprint
	TArray<FManifestActorVariableDefinition> Variables;

	// v4.0: Event graph definition (inline or reference)
	FString EventGraph;  // Reference to named event_graph
	FManifestEventGraphDefinition InlineEventGraph;  // Inline definition

	/** v3.0: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		Hash ^= GetTypeHash(NotifyClass) << 8;
		// v4.0: Include variables and event graph in hash
		for (const auto& Var : Variables)
		{
			Hash ^= Var.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		Hash ^= GetTypeHash(EventGraph) << 16;
		Hash ^= InlineEventGraph.ComputeHash() << 24;
		return Hash;
	}
};

/**
 * v3.7: Alternative dialogue line for variety in NPC responses
 * v4.1: Added FacialAnimation for face mesh montages
 */
struct FManifestDialogueLineDefinition
{
	FString Text;
	FString Audio;            // Asset path to USoundBase
	FString Montage;          // Asset path to UAnimMontage (body animation)
	FString FacialAnimation;  // v4.1: Asset path to UAnimMontage (face animation)

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Text);
		Hash ^= GetTypeHash(Audio) << 4;
		Hash ^= GetTypeHash(Montage) << 8;
		Hash ^= GetTypeHash(FacialAnimation) << 12;
		return Hash;
	}
};

/**
 * v3.7: Event definition for dialogue nodes (fires on start/end of line)
 */
struct FManifestDialogueEventDefinition
{
	FString Type;                        // Event class name (NE_BeginQuest, etc.)
	FString Runtime = TEXT("Start");     // Start, End, Both
	TMap<FString, FString> Properties;   // Event-specific properties

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Type);
		Hash ^= GetTypeHash(Runtime) << 4;
		for (const auto& Prop : Properties)
		{
			Hash ^= GetTypeHash(Prop.Key);
			Hash ^= GetTypeHash(Prop.Value);
		}
		return Hash;
	}
};

/**
 * v3.7: Condition definition for dialogue nodes (controls node visibility)
 */
struct FManifestDialogueConditionDefinition
{
	FString Type;                        // Condition class name (NC_IsQuestInProgress, etc.)
	bool bNot = false;                   // Invert result
	TMap<FString, FString> Properties;   // Condition-specific properties

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Type);
		Hash ^= bNot ? 1ULL : 0ULL;
		for (const auto& Prop : Properties)
		{
			Hash ^= GetTypeHash(Prop.Key);
			Hash ^= GetTypeHash(Prop.Value);
		}
		return Hash;
	}
};

/**
 * v3.7: Dialogue node definition (NPC or Player node)
 * v3.9.6: Added quest shortcuts (StartQuest, CompleteQuestBranch, FailQuest)
 * v4.1: Added FacialAnimation, HintText for enhanced dialogue automation
 */
struct FManifestDialogueNodeDefinition
{
	FString Id;                          // Unique node identifier
	FString Type;                        // "npc" or "player"
	FString Speaker;                     // Speaker ID (for NPC nodes)
	FString Text;                        // Dialogue line text
	FString OptionText;                  // Short option text (player nodes)
	FString HintText;                    // v4.1: Hint text after option (e.g., "(Lie)", "(Begin Quest)")
	FString Audio;                       // Audio asset path
	FString Montage;                     // Animation montage path (body)
	FString FacialAnimation;             // v4.1: Facial animation montage path
	FString Duration;                    // "Default", "WhenAudioEnds", "AfterDuration", etc.
	float DurationSeconds = 0.0f;        // Duration override
	bool bAutoSelect = false;            // Auto-select this option
	bool bAutoSelectIfOnly = true;       // Auto-select if only option
	bool bIsSkippable = true;            // Can skip this line
	FString DirectedAt;                  // Speaker ID this line is directed at
	TArray<FString> NPCReplies;          // IDs of NPC nodes that follow
	TArray<FString> PlayerReplies;       // IDs of player nodes that follow
	TArray<FManifestDialogueLineDefinition> AlternativeLines;  // Random alternatives
	TArray<FManifestDialogueEventDefinition> Events;           // Events on this node
	TArray<FManifestDialogueConditionDefinition> Conditions;   // Conditions for visibility

	// v3.9.6: Quest shortcuts - auto-generate NarrativeEvents
	FString StartQuest;                  // Quest to start when node selected
	FString CompleteQuestBranch;         // Quest branch to complete
	FString FailQuest;                   // Quest to fail

	// v4.2: Custom event callback - FName of function to call when node plays
	FString OnPlayNodeFuncName;          // Called when node starts/finishes playing

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Id);
		Hash ^= GetTypeHash(Type) << 4;
		Hash ^= GetTypeHash(Speaker) << 8;
		Hash ^= static_cast<uint64>(GetTypeHash(Text)) << 12;
		Hash ^= static_cast<uint64>(GetTypeHash(OptionText)) << 16;
		// v4.1: Include HintText and FacialAnimation in hash
		Hash ^= static_cast<uint64>(GetTypeHash(HintText)) << 18;
		Hash ^= static_cast<uint64>(GetTypeHash(Audio)) << 20;
		Hash ^= static_cast<uint64>(GetTypeHash(Montage)) << 24;
		Hash ^= static_cast<uint64>(GetTypeHash(FacialAnimation)) << 26;
		Hash ^= static_cast<uint64>(GetTypeHash(Duration)) << 28;
		Hash ^= static_cast<uint64>(GetTypeHash(static_cast<int32>(DurationSeconds * 1000.f))) << 32;
		Hash ^= (bAutoSelect ? 1ULL : 0ULL) << 36;
		Hash ^= (bAutoSelectIfOnly ? 1ULL : 0ULL) << 37;
		Hash ^= (bIsSkippable ? 1ULL : 0ULL) << 38;
		Hash ^= static_cast<uint64>(GetTypeHash(DirectedAt)) << 40;
		for (const auto& Reply : NPCReplies) { Hash ^= GetTypeHash(Reply); }
		for (const auto& Reply : PlayerReplies) { Hash ^= GetTypeHash(Reply); }
		for (const auto& Alt : AlternativeLines) { Hash ^= Alt.ComputeHash(); }
		for (const auto& Evt : Events) { Hash ^= Evt.ComputeHash(); }
		for (const auto& Cond : Conditions) { Hash ^= Cond.ComputeHash(); }
		// v3.9.6: Include quest shortcuts in hash
		Hash ^= static_cast<uint64>(GetTypeHash(StartQuest));
		Hash = (Hash << 3) | (Hash >> 61);
		Hash ^= static_cast<uint64>(GetTypeHash(CompleteQuestBranch));
		Hash = (Hash << 3) | (Hash >> 61);
		Hash ^= static_cast<uint64>(GetTypeHash(FailQuest));
		Hash = (Hash << 3) | (Hash >> 61);
		// v4.2: Include custom event callback in hash
		Hash ^= static_cast<uint64>(GetTypeHash(OnPlayNodeFuncName));
		return Hash;
	}
};

/**
 * v3.7: Dialogue tree definition containing all nodes
 */
struct FManifestDialogueTreeDefinition
{
	FString RootNodeId;                  // ID of the starting node
	TArray<FManifestDialogueNodeDefinition> Nodes;

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(RootNodeId);
		for (const auto& Node : Nodes)
		{
			Hash ^= Node.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

/**
 * v3.2: Dialogue speaker definition for UDialogue::Speakers array
 * v4.0: Added SpeakerID and bIsPlayer fields
 */
struct FManifestDialogueSpeakerDefinition
{
	FString NPCDefinition;  // Reference to NPC_ asset
	FString SpeakerID;      // v4.0: Optional override for speaker ID
	FString NodeColor;      // Hex color e.g. "#0066FF"
	TArray<FString> OwnedTags;  // Tags applied during dialogue
	bool bIsPlayer = false; // v4.0: True for player speaker

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(NPCDefinition);
		Hash ^= GetTypeHash(SpeakerID) << 4;
		Hash ^= GetTypeHash(NodeColor) << 8;
		Hash ^= (bIsPlayer ? 1ULL : 0ULL) << 12;
		for (const auto& Tag : OwnedTags)
		{
			Hash ^= GetTypeHash(Tag);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

/**
 * v4.0: Player speaker configuration for dialogue
 */
struct FManifestPlayerSpeakerDefinition
{
	FString SpeakerID = TEXT("Player");
	FString NodeColor = TEXT("#0066FF");
	FString SelectingReplyShot;  // Optional camera shot reference

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(SpeakerID);
		Hash ^= GetTypeHash(NodeColor) << 8;
		Hash ^= GetTypeHash(SelectingReplyShot) << 16;
		return Hash;
	}
};

/**
 * v4.8.3: Transform definition for FTransform properties
 * Used for PlayerAutoAdjustTransform, SpeakerAvatarTransform, etc.
 */
struct FManifestTransformDefinition
{
	float LocationX = 0.0f;
	float LocationY = 0.0f;
	float LocationZ = 0.0f;
	float RotationPitch = 0.0f;  // Y axis
	float RotationYaw = 0.0f;    // Z axis
	float RotationRoll = 0.0f;   // X axis
	float ScaleX = 1.0f;
	float ScaleY = 1.0f;
	float ScaleZ = 1.0f;

	bool IsDefault() const
	{
		return LocationX == 0.0f && LocationY == 0.0f && LocationZ == 0.0f &&
			   RotationPitch == 0.0f && RotationYaw == 0.0f && RotationRoll == 0.0f &&
			   ScaleX == 1.0f && ScaleY == 1.0f && ScaleZ == 1.0f;
	}

	uint64 ComputeHash() const
	{
		uint64 Hash = static_cast<uint64>(FMath::RoundToInt(LocationX * 100.f));
		Hash ^= static_cast<uint64>(FMath::RoundToInt(LocationY * 100.f)) << 8;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(LocationZ * 100.f)) << 16;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(RotationPitch * 100.f)) << 24;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(RotationYaw * 100.f)) << 32;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(RotationRoll * 100.f)) << 40;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(ScaleX * 100.f)) << 48;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(ScaleY * 100.f)) << 52;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(ScaleZ * 100.f)) << 56;
		return Hash;
	}
};

/**
 * v4.8.3: Dialogue sequence definition for instanced UNarrativeDialogueSequence
 * Used for DefaultDialogueShot, DefaultSpeakerShot, SelectingReplyShot
 */
struct FManifestDialogueSequenceDefinition
{
	FString SequenceClass;           // TSubclassOf<UNarrativeDialogueSequence> for class-based reference
	TArray<FString> SequenceAssets;  // TArray<ULevelSequence*> for inline instanced creation
	FString AnchorOriginRule;        // Disabled, ConversationCenter, Speaker, Listener, Custom
	float AnchorOriginNudgeX = 0.0f;
	float AnchorOriginNudgeY = 0.0f;
	float AnchorOriginNudgeZ = 0.0f;
	FString AnchorRotationRule;      // AnchorActorForwardVector, Conversation
	bool bUse180DegreeRule = false;
	float UnitsY180DegreeRule = 0.0f;
	float DegreesYaw180DegreeRule = 0.0f;

	bool IsEmpty() const { return SequenceClass.IsEmpty() && SequenceAssets.Num() == 0; }

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(SequenceClass);
		for (const FString& Asset : SequenceAssets)
		{
			Hash ^= GetTypeHash(Asset);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		Hash ^= static_cast<uint64>(GetTypeHash(AnchorOriginRule)) << 8;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(AnchorOriginNudgeX * 100.f)) << 16;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(AnchorOriginNudgeY * 100.f)) << 24;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(AnchorOriginNudgeZ * 100.f)) << 32;
		Hash ^= static_cast<uint64>(GetTypeHash(AnchorRotationRule)) << 40;
		Hash ^= (bUse180DegreeRule ? 1ULL : 0ULL) << 48;
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

	// v4.1: Camera shake for dialogue
	FString CameraShake;             // UCameraShakeBase class reference for dialogue camera

	// v4.8.3: Sound attenuation for dialogue audio
	FString DialogueSoundAttenuation;  // USoundAttenuation* asset path

	// v4.8.3: Player auto-adjust transform (for 1-on-1 dialogues)
	FManifestTransformDefinition PlayerAutoAdjustTransform;

	// v4.8.3: Default dialogue camera shot (instanced UNarrativeDialogueSequence)
	FManifestDialogueSequenceDefinition DefaultDialogueShot;

	// v3.2: Speakers configuration
	TArray<FManifestDialogueSpeakerDefinition> Speakers;

	// v4.0: Player speaker configuration
	FManifestPlayerSpeakerDefinition PlayerSpeaker;

	// v4.8: Party speaker configurations (for multiplayer dialogues)
	TArray<FManifestPlayerSpeakerDefinition> PartySpeakerInfo;

	// v3.7: Full dialogue tree
	FManifestDialogueTreeDefinition DialogueTree;

	/** v4.8: Compute hash for change detection (excludes Folder - presentational only) */
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
		// v4.1: Include camera shake in hash
		Hash ^= static_cast<uint64>(GetTypeHash(CameraShake)) << 36;
		// v4.8.3: Include new dialogue properties
		Hash ^= static_cast<uint64>(GetTypeHash(DialogueSoundAttenuation)) << 38;
		if (!PlayerAutoAdjustTransform.IsDefault())
		{
			Hash ^= PlayerAutoAdjustTransform.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		if (!DefaultDialogueShot.IsEmpty())
		{
			Hash ^= DefaultDialogueShot.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}
		for (const auto& Speaker : Speakers)
		{
			Hash ^= Speaker.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}
		// v4.0: Include player speaker in hash
		Hash ^= PlayerSpeaker.ComputeHash() << 40;
		// v4.8: Include party speaker info in hash
		for (const auto& PartySpeaker : PartySpeakerInfo)
		{
			Hash ^= PartySpeaker.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		// v3.7: Include dialogue tree in hash
		Hash ^= DialogueTree.ComputeHash() << 44;
		return Hash;
	}
};

// ============================================================================
// v3.9.8: Item Pipeline Types
// Mesh-to-Item automation with clothing mesh support
// ============================================================================

/**
 * Pipeline item type for mesh analysis
 */
enum class EPipelineItemType : uint8
{
	Clothing,       // UEquippableItem_Clothing
	Weapon_Melee,   // UMeleeWeaponItem
	Weapon_Ranged,  // URangedWeaponItem
	Consumable,     // UConsumableItem
	Generic         // UNarrativeItem
};

/**
 * Mesh analysis result from pipeline analyzer
 */
struct FMeshAnalysisResult
{
	EPipelineItemType InferredType = EPipelineItemType::Generic;
	FString InferredSlot;           // Gameplay tag string
	FString SuggestedDisplayName;
	FString SuggestedAssetName;
	float SlotConfidence = 0.0f;
	float TypeConfidence = 0.0f;
};

/**
 * Pipeline mesh input definition
 */
struct FPipelineMeshInput
{
	FString MeshPath;
	EPipelineItemType ItemType = EPipelineItemType::Clothing;
	FString EquipmentSlot;          // Override slot
	FString DisplayName;            // Override name
	FString TargetCollection;
	FString TargetLoadout;
};

/**
 * Pipeline processing result
 */
struct FPipelineProcessResult
{
	bool bSuccess = false;
	FString GeneratedItemPath;
	FString GeneratedCollectionPath;
	TArray<FString> Warnings;
	TArray<FString> Errors;
};

// ============================================================================
// v3.9.8: Clothing Mesh Configuration Structs
// Maps to Narrative Pro's FCharacterCreatorAttribute_Mesh structure
// ============================================================================

/**
 * Material parameter binding (maps to FCreatorMeshMaterialParam_Vector/Scalar)
 */
struct FManifestMaterialParamBinding
{
	FString ParameterName;          // Material parameter name
	FString TagId;                  // Gameplay tag (Narrative.CharacterCreator.Vectors.*)

	uint64 ComputeHash() const
	{
		return GetTypeHash(ParameterName) ^ (GetTypeHash(TagId) << 16);
	}
};

/**
 * Clothing material definition (maps to FCreatorMeshMaterial)
 */
struct FManifestClothingMaterial
{
	FString Material;               // TSoftObjectPtr<UMaterialInterface>
	TArray<FManifestMaterialParamBinding> VectorParams;
	TArray<FManifestMaterialParamBinding> ScalarParams;

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Material);
		for (const auto& VP : VectorParams)
		{
			Hash ^= VP.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const auto& SP : ScalarParams)
		{
			Hash ^= SP.ComputeHash();
			Hash = (Hash << 4) | (Hash >> 60);
		}
		return Hash;
	}
};

/**
 * Clothing morph definition (maps to FCreatorMeshMorph)
 */
struct FManifestClothingMorph
{
	FString ScalarTag;              // Gameplay tag for scalar value
	TArray<FString> MorphNames;     // Morph target names

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(ScalarTag);
		for (const FString& Name : MorphNames)
		{
			Hash ^= GetTypeHash(Name);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

/**
 * Complete clothing mesh configuration (maps to FCharacterCreatorAttribute_Mesh)
 */
struct FManifestClothingMeshConfig
{
	FString Mesh;                   // TSoftObjectPtr<USkeletalMesh>
	bool bUseLeaderPose = true;
	bool bIsStaticMesh = false;
	FString StaticMesh;             // TSoftObjectPtr<UStaticMesh>
	FString AttachSocket;           // FName MeshAttachSocket
	FVector AttachLocation = FVector::ZeroVector;
	FRotator AttachRotation = FRotator::ZeroRotator;
	FVector AttachScale = FVector::OneVector;
	FString MeshAnimBP;             // TSoftClassPtr<UAnimInstance>
	TArray<FManifestClothingMaterial> Materials;
	TArray<FManifestClothingMorph> Morphs;

	bool IsEmpty() const { return Mesh.IsEmpty() && StaticMesh.IsEmpty(); }

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Mesh);
		Hash ^= (bUseLeaderPose ? 1ULL : 0ULL) << 4;
		Hash ^= (bIsStaticMesh ? 1ULL : 0ULL) << 5;
		Hash ^= GetTypeHash(StaticMesh) << 8;
		Hash ^= GetTypeHash(AttachSocket) << 16;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(AttachLocation.X * 10.f));
		Hash ^= static_cast<uint64>(FMath::RoundToInt(AttachLocation.Y * 10.f)) << 10;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(AttachLocation.Z * 10.f)) << 20;
		Hash ^= GetTypeHash(MeshAnimBP) << 24;
		for (const auto& Mat : Materials)
		{
			Hash ^= Mat.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const auto& Morph : Morphs)
		{
			Hash ^= Morph.ComputeHash();
			Hash = (Hash << 4) | (Hash >> 60);
		}
		return Hash;
	}
};

/**
 * v3.10: Weapon attachment slot configuration for TMap automation
 * Maps to TMap<FGameplayTag, FWeaponAttachmentSlotConfig>
 */
struct FManifestWeaponAttachmentSlot
{
	FString Slot;      // GameplayTag string e.g. "Narrative.Equipment.Weapon.AttachSlot.Sight"
	FString Socket;    // Socket name on mesh
	FVector Offset = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Slot);
		Hash ^= GetTypeHash(Socket) << 4;
		Hash ^= GetTypeHash(Offset.ToString()) << 8;
		Hash ^= GetTypeHash(Rotation.ToString()) << 12;
		return Hash;
	}
};

/**
 * v4.8.2: Pickup mesh data for dropped items
 * Maps to FPickupMeshData - defines visual when item is dropped in world
 */
struct FManifestPickupMeshDataDefinition
{
	FString PickupMesh;                      // TSoftObjectPtr<UStaticMesh> - the 3D model
	TArray<FString> PickupMeshMaterials;     // TArray<TSoftObjectPtr<UMaterialInterface>> - override materials

	bool IsEmpty() const { return PickupMesh.IsEmpty() && PickupMeshMaterials.Num() == 0; }

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(PickupMesh);
		for (const FString& Mat : PickupMeshMaterials)
		{
			Hash ^= GetTypeHash(Mat);
			Hash = (Hash << 5) | (Hash >> 59);
		}
		return Hash;
	}
};

/**
 * v4.8.2: Combat trace data for ranged weapons
 * Maps to FCombatTraceData - configures how weapon detects hits
 */
struct FManifestCombatTraceDataDefinition
{
	float TraceDistance = 500.0f;    // How far the weapon shoots
	float TraceRadius = 0.0f;        // Trace width (0 = line trace, >0 = sphere trace)
	bool bTraceMulti = false;        // Hit multiple targets or just first

	bool IsDefault() const { return TraceDistance == 500.0f && TraceRadius == 0.0f && !bTraceMulti; }

	uint64 ComputeHash() const
	{
		uint64 Hash = static_cast<uint64>(FMath::RoundToInt(TraceDistance * 10.f));
		Hash ^= static_cast<uint64>(FMath::RoundToInt(TraceRadius * 100.f)) << 16;
		Hash ^= (bTraceMulti ? 1ULL : 0ULL) << 32;
		return Hash;
	}
};

/**
 * v4.8: Item stat definition for UI display
 * Maps to FNarrativeItemStat used in item tooltips
 */
struct FManifestItemStatDefinition
{
	FString StatName;        // Display name (e.g., "Damage", "Fire Rate")
	float StatValue = 0.0f;  // Stat value
	FString StatIcon;        // TSoftObjectPtr<UTexture2D> - icon path

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(StatName);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(StatValue * 1000.f)) << 8;
		Hash ^= GetTypeHash(StatIcon) << 16;
		return Hash;
	}
};

/**
 * Equippable item definition
 * v3.3: Enhanced with full NarrativeItem + EquippableItem property support
 * v3.4: Added WeaponItem and RangedWeaponItem property support
 * v3.9.8: Added ClothingMesh for EquippableItem_Clothing support
 * v3.10: Added WeaponAttachmentSlots TMap support
 * v4.8: Added Stats array and ActivitiesToGrant for NarrativeItem
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

	// v3.10: Additional RangedWeaponItem properties
	FString CrosshairWidget;             // TSubclassOf<UUserWidget> - Custom crosshair widget class
	float AimWeaponRenderFOV = 0.0f;     // FOV for weapon render texture when aiming
	float AimWeaponFStop = 0.0f;         // F-stop for weapon DOF when aiming
	float MoveSpeedAddDegrees = 0.0f;    // Spread added based on movement speed
	float CrouchSpreadMultiplier = 1.0f; // Spread multiplier when crouching
	float AimSpreadMultiplier = 1.0f;    // Spread multiplier when aiming
	FVector RecoilImpulseTranslationMin = FVector::ZeroVector;  // Min recoil translation when aiming
	FVector RecoilImpulseTranslationMax = FVector::ZeroVector;  // Max recoil translation when aiming
	FVector HipRecoilImpulseTranslationMin = FVector::ZeroVector; // Min recoil translation from hip
	FVector HipRecoilImpulseTranslationMax = FVector::ZeroVector; // Max recoil translation from hip

	// v3.10: Weapon attachment slots TMap automation (replaces parallel array approach)
	TArray<FManifestWeaponAttachmentSlot> WeaponAttachmentSlots;

	// v3.9.6: NarrativeItem usage properties
	bool bAddDefaultUseOption = true;    // Add default "Use" option to context menu
	bool bConsumeOnUse = false;          // Consume item when used
	bool bUsedWithOtherItem = false;     // Item is used with another item
	FString UseActionText;               // Custom use action text (e.g., "Drink", "Read")
	bool bCanActivate = false;           // Item can be activated
	bool bToggleActiveOnUse = false;     // Toggle active state on use
	FString UseSound;                    // Sound to play on use (asset path)

	// v3.9.6: Weapon attachment configurations
	TArray<FString> HolsterAttachmentSlots;     // Slot tags for holster configs
	TArray<FString> HolsterAttachmentSockets;   // Socket names (parallel array)
	TArray<FVector> HolsterAttachmentOffsets;   // Location offsets (parallel array)
	TArray<FRotator> HolsterAttachmentRotations; // Rotation offsets (parallel array)
	TArray<FString> WieldAttachmentSlots;       // Slot tags for wield configs
	TArray<FString> WieldAttachmentSockets;     // Socket names (parallel array)
	TArray<FVector> WieldAttachmentOffsets;     // Location offsets (parallel array)
	TArray<FRotator> WieldAttachmentRotations;  // Rotation offsets (parallel array)

	// v3.9.8: Clothing mesh configuration (for EquippableItem_Clothing parent class)
	FManifestClothingMeshConfig ClothingMesh;

	// v3.9.12: Equipment effect values TMap<FGameplayTag, float>
	// Used for SetByCaller armor, damage, and other stat values
	TMap<FString, float> EquipmentEffectValues;

	// v4.2: Equipment abilities - granted when item is equipped, removed when unequipped
	TArray<FString> EquipmentAbilities;  // TArray<TSubclassOf<UNarrativeGameplayAbility>>

	// v4.8: NarrativeItem additional properties
	TArray<FManifestItemStatDefinition> Stats;  // TArray<FNarrativeItemStat> - UI stat display
	TArray<FString> ActivitiesToGrant;          // TArray<TSubclassOf<UNPCActivity>> - AI activities from item

	// v4.8.2: Additional NarrativeItem properties
	FString ItemWidgetOverride;                 // TSubclassOf<UNarrativeInventoryItemButton> - custom inventory UI
	bool bWantsTickByDefault = false;           // Enable item ticking for per-frame updates
	FManifestPickupMeshDataDefinition PickupMeshData;  // FPickupMeshData - visual when dropped in world

	// v4.8.2: RangedWeaponItem trace configuration
	FManifestCombatTraceDataDefinition TraceData;      // FCombatTraceData - how weapon detects hits

	/** v4.8.2: Compute hash for change detection (excludes Folder - presentational only) */
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

		// v3.10: Hash additional RangedWeaponItem properties
		Hash ^= GetTypeHash(CrosshairWidget);
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(AimWeaponRenderFOV * 100.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(AimWeaponFStop * 100.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(MoveSpeedAddDegrees * 100.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(CrouchSpreadMultiplier * 100.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= static_cast<uint64>(FMath::RoundToInt(AimSpreadMultiplier * 100.f));
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= GetTypeHash(RecoilImpulseTranslationMin.ToString());
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= GetTypeHash(RecoilImpulseTranslationMax.ToString());
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= GetTypeHash(HipRecoilImpulseTranslationMin.ToString());
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= GetTypeHash(HipRecoilImpulseTranslationMax.ToString());
		Hash = (Hash << 5) | (Hash >> 59);

		// v3.10: Hash weapon attachment slots TMap
		for (const FManifestWeaponAttachmentSlot& Slot : WeaponAttachmentSlots)
		{
			Hash ^= Slot.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}

		// v3.9.6: Hash usage properties
		Hash ^= (bAddDefaultUseOption ? 1ULL : 0ULL) << 11;
		Hash ^= (bConsumeOnUse ? 1ULL : 0ULL) << 12;
		Hash ^= (bUsedWithOtherItem ? 1ULL : 0ULL) << 13;
		Hash ^= GetTypeHash(UseActionText);
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= (bCanActivate ? 1ULL : 0ULL) << 14;
		Hash ^= (bToggleActiveOnUse ? 1ULL : 0ULL) << 15;
		Hash ^= GetTypeHash(UseSound);
		Hash = (Hash << 5) | (Hash >> 59);

		// v3.9.6: Hash weapon attachment configs
		for (int32 i = 0; i < HolsterAttachmentSlots.Num(); i++)
		{
			Hash ^= GetTypeHash(HolsterAttachmentSlots[i]);
			if (HolsterAttachmentSockets.IsValidIndex(i)) Hash ^= GetTypeHash(HolsterAttachmentSockets[i]);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (int32 i = 0; i < WieldAttachmentSlots.Num(); i++)
		{
			Hash ^= GetTypeHash(WieldAttachmentSlots[i]);
			if (WieldAttachmentSockets.IsValidIndex(i)) Hash ^= GetTypeHash(WieldAttachmentSockets[i]);
			Hash = (Hash << 3) | (Hash >> 61);
		}

		// v3.9.8: Hash clothing mesh config
		if (!ClothingMesh.IsEmpty())
		{
			Hash ^= ClothingMesh.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}


		// v3.9.12: Hash equipment effect values
		for (const auto& Pair : EquipmentEffectValues)
		{
			Hash ^= GetTypeHash(Pair.Key);
			Hash ^= static_cast<uint64>(FMath::RoundToInt(Pair.Value * 100.f));
			Hash = (Hash << 4) | (Hash >> 60);
		}

		// v4.2: Hash equipment abilities
		for (const FString& Ability : EquipmentAbilities)
		{
			Hash ^= GetTypeHash(Ability);
			Hash = (Hash << 3) | (Hash >> 61);
		}

		// v4.8: Hash stats array and activities to grant
		for (const auto& Stat : Stats)
		{
			Hash ^= Stat.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}
		for (const FString& Activity : ActivitiesToGrant)
		{
			Hash ^= GetTypeHash(Activity);
			Hash = (Hash << 3) | (Hash >> 61);
		}

		// v4.8.2: Hash additional NarrativeItem properties
		Hash ^= GetTypeHash(ItemWidgetOverride);
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= (bWantsTickByDefault ? 1ULL : 0ULL) << 16;
		if (!PickupMeshData.IsEmpty())
		{
			Hash ^= PickupMeshData.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}

		// v4.8.2: Hash TraceData for RangedWeaponItem
		if (!TraceData.IsDefault())
		{
			Hash ^= TraceData.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}

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

	// v4.1: Child class property introspection
	// Maps property name to value string for child-specific properties
	// e.g., NE_GiveXP: {"XPAmount": "100"}, NE_AddCurrency: {"Currency": "500"}
	TMap<FString, FString> Properties;

	// v4.3: Event conditions - event only fires if all conditions pass
	// Reuses dialogue condition format: Type, bNot, Properties
	TArray<FManifestDialogueConditionDefinition> Conditions;

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
		// v4.1: Include child class properties in hash
		for (const auto& Pair : Properties)
		{
			Hash ^= GetTypeHash(Pair.Key);
			Hash ^= GetTypeHash(Pair.Value);
			Hash = (Hash << 5) | (Hash >> 59);
		}
		// v4.3: Include event conditions in hash
		for (const auto& Condition : Conditions)
		{
			Hash ^= Condition.ComputeHash();
			Hash = (Hash << 4) | (Hash >> 60);
		}
		return Hash;
	}
};

// ============================================================================
// v4.0: GAMEPLAY CUE DEFINITIONS (NEW)
// ============================================================================

/**
 * v4.0: Gameplay Cue spawn condition configuration
 */
struct FManifestGameplayCueSpawnCondition
{
	FString AttachPolicy = TEXT("AttachToTarget");  // AttachToTarget, DoNotAttach
	FString AttachSocket = TEXT("");
	float SpawnProbability = 1.0f;

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(AttachPolicy);
		Hash ^= GetTypeHash(AttachSocket) << 8;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(SpawnProbability * 1000.f)) << 16;
		return Hash;
	}
};

/**
 * v4.0: Gameplay Cue placement configuration
 */
struct FManifestGameplayCuePlacement
{
	FString SocketName = TEXT("");
	bool bAttachToOwner = true;
	FVector RelativeOffset = FVector::ZeroVector;
	FRotator RelativeRotation = FRotator::ZeroRotator;

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(SocketName);
		Hash ^= (bAttachToOwner ? 1ULL : 0ULL) << 8;
		Hash ^= GetTypeHash(RelativeOffset.ToString()) << 16;
		Hash ^= static_cast<uint64>(GetTypeHash(RelativeRotation.ToString())) << 32;
		return Hash;
	}
};

/**
 * v4.0: Gameplay Cue burst effects configuration
 */
struct FManifestGameplayCueBurstEffects
{
	FString ParticleSystem = TEXT("");   // NS_ asset reference
	FString Sound = TEXT("");             // Sound asset reference
	FString CameraShake = TEXT("");       // Camera shake class

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(ParticleSystem);
		Hash ^= GetTypeHash(Sound) << 16;
		Hash ^= static_cast<uint64>(GetTypeHash(CameraShake)) << 32;
		return Hash;
	}
};

/**
 * v4.0: Gameplay Cue definition
 * Generates GC_ prefixed Blueprints inheriting from GameplayCueNotify_Burst/BurstLatent/Actor
 */
struct FManifestGameplayCueDefinition
{
	FString Name;
	FString Folder = TEXT("FX/GameplayCues");

	// Cue type determines parent class:
	// - "Burst" -> UGameplayCueNotify_Burst (one-off, instant)
	// - "BurstLatent" -> UGameplayCueNotify_BurstLatent (one-off with duration)
	// - "Actor" -> AGameplayCueNotify_Actor (persistent, looping)
	FString CueType = TEXT("Burst");

	// The gameplay cue tag this responds to (e.g., GameplayCue.Father.Attack)
	FString GameplayCueTag;

	// Optional configuration
	FManifestGameplayCueSpawnCondition SpawnCondition;
	FManifestGameplayCuePlacement Placement;
	FManifestGameplayCueBurstEffects BurstEffects;

	/** v4.0: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		Hash ^= GetTypeHash(CueType) << 4;
		Hash ^= GetTypeHash(GameplayCueTag) << 8;
		Hash ^= SpawnCondition.ComputeHash() << 16;
		Hash ^= Placement.ComputeHash() << 24;
		Hash ^= BurstEffects.ComputeHash() << 32;
		return Hash;
	}
};

/**
 * v3.9.5: Item with quantity - maps to FItemWithQuantity
 * Used in loot table rolls for specifying individual items
 */
struct FManifestItemWithQuantityDefinition
{
	FString Item;           // Item class path (e.g., "EI_IronSword" or full path)
	int32 Quantity = 1;     // Amount of this item to grant

	/** v3.9.5: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Item);
		Hash ^= static_cast<uint64>(Quantity) << 8;
		return Hash;
	}
};

/**
 * v3.9.5: Loot table roll - maps to FLootTableRoll
 * Defines a single roll in DefaultItemLoadout or TradingItemLoadout
 */
struct FManifestLootTableRollDefinition
{
	TArray<FManifestItemWithQuantityDefinition> ItemsToGrant;  // Individual items with quantities
	TArray<FString> ItemCollectionsToGrant;                     // Item collection references (IC_*)
	FString TableToRoll;                                        // Optional DataTable path for dynamic rolling
	int32 NumRolls = 1;                                         // Number of times to roll (default 1)
	float Chance = 1.0f;                                        // Chance of each roll succeeding (0.0-1.0)

	/** v3.9.5: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = 0;
		for (const auto& ItemDef : ItemsToGrant)
		{
			Hash ^= ItemDef.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const FString& Collection : ItemCollectionsToGrant)
		{
			Hash ^= GetTypeHash(Collection);
			Hash = (Hash << 5) | (Hash >> 59);
		}
		Hash ^= GetTypeHash(TableToRoll);
		Hash = (Hash << 7) | (Hash >> 57);
		Hash ^= static_cast<uint64>(NumRolls) << 16;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(Chance * 1000.f)) << 24;
		return Hash;
	}
};

/**
 * NPC definition - maps to UNPCDefinition data asset
 * v3.3: Enhanced with full Narrative Pro property support
 * v3.9.5: Added DefaultItemLoadout and TradingItemLoadout arrays
 * v4.8: Added UniqueNPCGUID and TriggerSets support
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

	// v4.8: Unique NPC GUID for save system (auto-generated if bAllowMultipleInstances=false)
	FString UniqueNPCGUID;               // FGuid - save system identifier for unique NPCs

	// v4.8: Trigger sets (inherited from CharacterDefinition)
	TArray<FString> TriggerSets;         // TArray<TSoftObjectPtr<UTriggerSet>>

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

	// v3.6: Activity schedule support
	TArray<FString> ActivitySchedules;   // TArray<TSoftObjectPtr<UNPCActivitySchedule>>

	// v3.7: Auto-create related assets (full NPC package)
	bool bAutoCreateDialogue = false;           // Create DBP_{Name} dialogue blueprint
	bool bAutoCreateTaggedDialogue = false;     // Create {Name}_TaggedDialogue set
	bool bAutoCreateItemLoadout = false;        // Create ItemLoadout_{Name} DataTable
	TArray<FString> DefaultItemLoadoutCollections;  // v3.7: Simple item collections (legacy, converted to loot rolls)

	// v3.9.5: Full loot table roll support for item loadouts
	TArray<FManifestLootTableRollDefinition> DefaultItemLoadout;  // Items granted at spawn (from CharacterDefinition)
	TArray<FManifestLootTableRollDefinition> TradingItemLoadout;  // Vendor inventory items (NPCDefinition only)

	/** v4.8: Compute hash for change detection (excludes Folder - presentational only) */
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

		// v4.8: Hash UniqueNPCGUID and TriggerSets
		Hash ^= static_cast<uint64>(GetTypeHash(UniqueNPCGUID)) << 45;
		for (const FString& TriggerSet : TriggerSets)
		{
			Hash ^= GetTypeHash(TriggerSet);
			Hash = (Hash << 5) | (Hash >> 59);
		}

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
		// v3.6: Hash activity schedules
		for (const FString& Schedule : ActivitySchedules)
		{
			Hash ^= GetTypeHash(Schedule);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		// v3.7: Hash auto-create flags and item loadout collections
		Hash ^= (bAutoCreateDialogue ? 1ULL : 0ULL) << 42;
		Hash ^= (bAutoCreateTaggedDialogue ? 1ULL : 0ULL) << 43;
		Hash ^= (bAutoCreateItemLoadout ? 1ULL : 0ULL) << 44;
		for (const FString& Collection : DefaultItemLoadoutCollections)
		{
			Hash ^= GetTypeHash(Collection);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		// v3.9.5: Hash full loot table rolls
		for (const auto& Roll : DefaultItemLoadout)
		{
			Hash ^= Roll.ComputeHash();
			Hash = (Hash << 7) | (Hash >> 57);
		}
		for (const auto& Roll : TradingItemLoadout)
		{
			Hash ^= Roll.ComputeHash();
			Hash = (Hash << 7) | (Hash >> 57);
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
 * v4.8.3: Character appearance definition - maps to UCharacterAppearance data asset
 * Generates visual customization assets for NPCs and characters
 */
struct FManifestCharacterAppearanceDefinition
{
	FString Name;
	FString Folder;

	// Character mesh attributes (Narrative.Equipment.Slot.Mesh.*)
	TMap<FString, TArray<FString>> Meshes;  // Tag -> array of mesh paths

	// Scalar parameters (Narrative.CharacterCreator.Scalars.*)
	TMap<FString, float> ScalarValues;      // Tag -> min/max values (simplified: just uses value as both min/max)

	// Vector/color parameters (Narrative.CharacterCreator.Vectors.*)
	TMap<FString, FString> VectorValues;    // Tag -> hex color or swatch reference

	/** v4.8.3: Compute hash for change detection (excludes Folder - presentational only) */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		for (const auto& MeshPair : Meshes)
		{
			Hash ^= GetTypeHash(MeshPair.Key);
			for (const FString& MeshPath : MeshPair.Value)
			{
				Hash ^= GetTypeHash(MeshPath);
				Hash = (Hash << 3) | (Hash >> 61);
			}
		}
		for (const auto& ScalarPair : ScalarValues)
		{
			Hash ^= GetTypeHash(ScalarPair.Key);
			Hash ^= static_cast<uint64>(FMath::RoundToInt(ScalarPair.Value * 1000.f)) << 8;
			Hash = (Hash << 5) | (Hash >> 59);
		}
		for (const auto& VectorPair : VectorValues)
		{
			Hash ^= GetTypeHash(VectorPair.Key);
			Hash ^= GetTypeHash(VectorPair.Value) << 16;
			Hash = (Hash << 7) | (Hash >> 57);
		}
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
 * v4.9: Per-emitter parameter overrides for Niagara systems
 * Allows setting specific User.* parameters on individual emitters
 */
struct FManifestNiagaraEmitterOverride
{
	FString EmitterName;    // Name of emitter in system (e.g., "Particles", "Burst")
	bool bEnabled = true;   // Enable/disable this emitter
	TMap<FString, FString> Parameters;  // Parameter name -> value (as string)

	/** v4.9: Compute hash for change detection */
	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(EmitterName);
		Hash ^= (bEnabled ? 1ULL : 0ULL) << 8;
		for (const auto& Param : Parameters)
		{
			Hash ^= GetTypeHash(Param.Key);
			Hash ^= GetTypeHash(Param.Value) << 4;
			Hash = (Hash << 3) | (Hash >> 61);
		}
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
 * v4.9: FX Preset definition for reusable Niagara parameter configurations
 * Defines a named set of parameters that can be applied to multiple Niagara systems
 */
struct FManifestFXPresetDefinition
{
	FString Name;                              // Preset name (e.g., "Preset_Fire")
	FString Folder;                            // Optional folder for organization
	TMap<FString, FString> Parameters;         // Parameter key-value pairs (e.g., "User.Color" -> "[1,0,0,1]")
	FString BasePreset;                        // Optional: Inherit from another preset

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(BasePreset) << 4;
		for (const auto& Pair : Parameters)
		{
			Hash ^= GetTypeHash(Pair.Key);
			Hash ^= GetTypeHash(Pair.Value);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

/**
 * v2.6.10: Niagara System definition - creates UNiagaraSystem assets
 * Enhanced with warmup, bounds, determinism, and effect type settings
 * v2.6.11: Added user parameters support
 * v2.9.0: Added FX descriptor for data-driven parameter binding
 * v4.9: Added preset support for reusable parameter configurations
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

	// v4.9: FX Preset reference (parameters applied before user_parameters)
	FString Preset;  // Name of FX preset to use as base (e.g., "Preset_Fire")

	// v4.9: Per-emitter parameter overrides
	TArray<FManifestNiagaraEmitterOverride> EmitterOverrides;

	// v4.9: LOD/Scalability settings
	float CullDistanceLow = 0.0f;       // Cull distance for Low quality (0 = no cull)
	float CullDistanceMedium = 0.0f;    // Cull distance for Medium quality
	float CullDistanceHigh = 0.0f;      // Cull distance for High quality
	float CullDistanceEpic = 0.0f;      // Cull distance for Epic quality
	float CullDistanceCinematic = 0.0f; // Cull distance for Cinematic quality
	float CullMaxDistance = 0.0f;       // Single cull distance (overrides per-quality if set)
	float SignificanceDistance = 0.0f;  // Distance for significance calculations
	int32 MaxParticleBudget = 0;        // Max particle count budget (0 = unlimited)
	FString ScalabilityMode;            // Low, Medium, High, Epic, Cinematic
	bool bAllowCullingForLocalPlayers = false;  // Whether to cull for local player's effects

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

		// v4.9: Emitter overrides
		for (const auto& Override : EmitterOverrides)
		{
			Hash ^= Override.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}

		// Effect type settings
		Hash ^= GetTypeHash(EffectType);
		Hash ^= GetTypeHash(PoolingMethod) << 4;
		Hash ^= static_cast<uint64>(MaxPoolSize) << 56;

		// v4.9: FX Preset
		Hash ^= GetTypeHash(Preset);

		// User parameters
		for (const auto& Param : UserParameters)
		{
			Hash ^= Param.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}

		// FX Descriptor
		Hash ^= FXDescriptor.ComputeHash();

		// v4.9: LOD/Scalability settings
		Hash ^= static_cast<uint64>(FMath::RoundToInt(CullDistanceLow * 10.f));
		Hash ^= static_cast<uint64>(FMath::RoundToInt(CullDistanceMedium * 10.f)) << 8;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(CullDistanceHigh * 10.f)) << 16;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(CullDistanceEpic * 10.f)) << 24;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(CullDistanceCinematic * 10.f)) << 32;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(CullMaxDistance * 10.f)) << 40;
		Hash ^= static_cast<uint64>(MaxParticleBudget) << 48;
		Hash ^= GetTypeHash(ScalabilityMode);
		Hash ^= (bAllowCullingForLocalPlayers ? 1ULL : 0ULL) << 56;

		return Hash;
	}
};

/**
 * v3.9: Scheduled behavior definition for NPC activity schedules
 * Maps to UScheduledBehavior_AddNPCGoal entries in UNPCActivitySchedule
 */
struct FManifestScheduledBehaviorDefinition
{
	float StartTime = 0.0f;              // Start hour (24h format, e.g., 6.0 = 6:00 AM)
	float EndTime = 0.0f;                // End hour (24h format, e.g., 18.0 = 6:00 PM)
	FString GoalClass;                   // Goal class to add (e.g., "Goal_Work")
	float ScoreOverride = 0.0f;          // Optional score override (0 = use goal's default)
	bool bReselect = false;              // Trigger activity reselection when goal added
	FString Location;                    // Optional: Location tag or actor name

	uint64 ComputeHash() const
	{
		uint64 Hash = static_cast<uint64>(FMath::RoundToInt(StartTime * 100.f));
		Hash ^= static_cast<uint64>(FMath::RoundToInt(EndTime * 100.f)) << 16;
		Hash ^= static_cast<uint64>(GetTypeHash(GoalClass)) << 32;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(ScoreOverride * 100.f)) << 40;
		Hash ^= (bReselect ? 1ULL : 0ULL) << 48;
		Hash ^= static_cast<uint64>(GetTypeHash(Location)) << 52;
		return Hash;
	}
};

/**
 * v3.9: Activity schedule definition for NPC daily routines
 * Generates UNPCActivitySchedule data assets
 */
struct FManifestActivityScheduleDefinition
{
	FString Name;                        // Asset name (e.g., "Schedule_BlacksmithDay")
	FString Folder;                      // Output folder
	TArray<FManifestScheduledBehaviorDefinition> Behaviors;  // Time-based behaviors

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		for (const auto& Behavior : Behaviors)
		{
			Hash ^= Behavior.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

/**
 * v3.9: Goal item definition for NPC AI objectives
 * Generates UNPCGoalItem Blueprint assets
 */
struct FManifestGoalItemDefinition
{
	FString Name;                        // Asset name (e.g., "Goal_DefendForge")
	FString Folder;                      // Output folder
	FString ParentClass;                 // Parent class (default: "NPCGoalItem")
	float DefaultScore = 50.0f;          // Default priority score
	float GoalLifetime = -1.0f;          // Expiry time (-1 = never expires)
	bool bRemoveOnSucceeded = true;      // Auto-remove when completed
	bool bSaveGoal = false;              // Persist across saves
	TArray<FString> OwnedTags;           // Tags granted while active
	TArray<FString> BlockTags;           // Tags that block this goal
	TArray<FString> RequireTags;         // Tags required to act on goal

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		Hash ^= GetTypeHash(ParentClass) << 4;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(DefaultScore * 100.f)) << 8;
		Hash ^= static_cast<uint64>(FMath::RoundToInt(GoalLifetime * 100.f)) << 16;
		Hash ^= (bRemoveOnSucceeded ? 1ULL : 0ULL) << 24;
		Hash ^= (bSaveGoal ? 1ULL : 0ULL) << 25;
		for (const auto& Tag : OwnedTags) { Hash ^= GetTypeHash(Tag); Hash = (Hash << 3) | (Hash >> 61); }
		for (const auto& Tag : BlockTags) { Hash ^= GetTypeHash(Tag); Hash = (Hash << 4) | (Hash >> 60); }
		for (const auto& Tag : RequireTags) { Hash ^= GetTypeHash(Tag); Hash = (Hash << 5) | (Hash >> 59); }
		return Hash;
	}
};

/**
 * v4.3: Quest requirement definition - dynamic constraints on quests
 * Maps to UQuestRequirement instanced objects
 * Example: QR_StayNear (fail if too far from NPC), QR_KeepAlive (fail if NPC dies)
 */
struct FManifestQuestRequirementDefinition
{
	FString Type;                        // Requirement class name (e.g., "QR_StayNear", "QR_KeepAlive")
	TMap<FString, FString> Properties;   // Requirement-specific properties

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Type);
		for (const auto& Prop : Properties)
		{
			Hash ^= GetTypeHash(Prop.Key);
			Hash ^= GetTypeHash(Prop.Value);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

/**
 * v3.9.4: Quest task definition - objective within a branch
 * Based on UNarrativeTask properties
 */
struct FManifestQuestTaskDefinition
{
	FString TaskClass;                   // Task class (e.g., "GoToLocation", "FindItem", "TalkToCharacter")
	FString Argument;                    // Task argument (location name, item name, NPC name)
	int32 Quantity = 1;                  // Required quantity
	bool bOptional = false;              // Task is optional
	bool bHidden = false;                // Hide from UI

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(TaskClass);
		Hash ^= GetTypeHash(Argument) << 4;
		Hash ^= static_cast<uint64>(Quantity) << 8;
		Hash ^= (bOptional ? 1ULL : 0ULL) << 16;
		Hash ^= (bHidden ? 1ULL : 0ULL) << 17;
		return Hash;
	}
};

/**
 * v3.9.4: Quest branch definition - transition between states
 * Each state has outgoing branches leading to destination states
 */
struct FManifestQuestBranchDefinition
{
	FString Id;                          // Branch ID (optional)
	FString DestinationState;            // Target state ID
	TArray<FManifestQuestTaskDefinition> Tasks;  // Tasks required to take branch
	TArray<FManifestDialogueEventDefinition> Events;  // Events fired on branch completion
	bool bHidden = false;                // Hide from UI

	// v4.2: Custom event callback - FName of function to call when branch is taken
	FString OnEnteredFuncName;           // Called when branch is taken

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Id);
		Hash ^= GetTypeHash(DestinationState) << 4;
		Hash ^= (bHidden ? 1ULL : 0ULL) << 8;
		for (const auto& Task : Tasks)
		{
			Hash ^= Task.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const auto& Event : Events)
		{
			Hash ^= Event.ComputeHash();
			Hash = (Hash << 2) | (Hash >> 62);
		}
		// v4.2: Include custom event callback in hash
		Hash ^= static_cast<uint64>(GetTypeHash(OnEnteredFuncName));
		return Hash;
	}
};

/**
 * v3.9.4: Quest state definition - node in quest state machine
 * States contain branches that lead to other states
 */
struct FManifestQuestStateDefinition
{
	FString Id;                          // State ID (required)
	FString Description;                 // State description for journal
	FString Type = TEXT("regular");      // "regular", "success", "failure"
	TArray<FManifestQuestBranchDefinition> Branches;  // Outgoing transitions
	TArray<FManifestDialogueEventDefinition> Events;  // Events fired when state is entered

	// v4.2: Custom event callback - FName of function to call when state is entered/exited
	FString OnEnteredFuncName;           // Called when state is activated/deactivated

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Id);
		Hash ^= GetTypeHash(Description) << 4;
		Hash ^= GetTypeHash(Type) << 8;
		for (const auto& Branch : Branches)
		{
			Hash ^= Branch.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		for (const auto& Event : Events)
		{
			Hash ^= Event.ComputeHash();
			Hash = (Hash << 2) | (Hash >> 62);
		}
		// v4.2: Include custom event callback in hash
		Hash ^= static_cast<uint64>(GetTypeHash(OnEnteredFuncName));
		return Hash;
	}
};

/**
 * v4.2: Quest dialogue play params - parameters for playing quest dialogue
 * Maps to Narrative Pro's FDialoguePlayParams struct
 */
struct FManifestDialoguePlayParamsDefinition
{
	FString StartFromID;                 // Dialogue node ID to start from (empty = root)
	int32 Priority = -1;                 // Dialogue priority (-1 = use dialogue default)
	bool bOverride_bFreeMovement = false;
	bool bFreeMovement = true;           // Allow movement during dialogue
	bool bOverride_bStopMovement = false;
	bool bStopMovement = false;          // Stop NPC movement
	bool bOverride_bUnskippable = false;
	bool bUnskippable = false;           // Cannot skip lines
	bool bOverride_bCanBeExited = false;
	bool bCanBeExited = true;            // Can exit via ESC

	bool IsDefault() const
	{
		return StartFromID.IsEmpty() && Priority == -1 &&
		       !bOverride_bFreeMovement && !bOverride_bStopMovement &&
		       !bOverride_bUnskippable && !bOverride_bCanBeExited;
	}

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(StartFromID);
		Hash ^= static_cast<uint64>(Priority) << 8;
		Hash ^= (bOverride_bFreeMovement ? 1ULL : 0ULL) << 16;
		Hash ^= (bFreeMovement ? 1ULL : 0ULL) << 17;
		Hash ^= (bOverride_bStopMovement ? 1ULL : 0ULL) << 18;
		Hash ^= (bStopMovement ? 1ULL : 0ULL) << 19;
		Hash ^= (bOverride_bUnskippable ? 1ULL : 0ULL) << 20;
		Hash ^= (bUnskippable ? 1ULL : 0ULL) << 21;
		Hash ^= (bOverride_bCanBeExited ? 1ULL : 0ULL) << 22;
		Hash ^= (bCanBeExited ? 1ULL : 0ULL) << 23;
		return Hash;
	}
};

/**
 * v3.9.6: Quest reward definition - currency, XP, and items granted on completion
 */
struct FManifestQuestRewardDefinition
{
	int32 Currency = 0;                  // Currency to grant
	int32 XP = 0;                        // Experience points to grant
	TArray<FString> Items;               // Item class names to grant
	TArray<int32> ItemQuantities;        // Parallel array of quantities

	uint64 ComputeHash() const
	{
		uint64 Hash = static_cast<uint64>(Currency);
		Hash ^= static_cast<uint64>(XP) << 16;
		for (int32 i = 0; i < Items.Num(); i++)
		{
			Hash ^= GetTypeHash(Items[i]);
			int32 Qty = ItemQuantities.IsValidIndex(i) ? ItemQuantities[i] : 1;
			Hash ^= static_cast<uint64>(Qty) << 8;
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

/**
 * v3.9.4: Quest Blueprint definition - creates UQuestBlueprint with full state machine
 * Following the same pattern as DialogueBlueprint v3.8
 * v3.9.6: Added Questgiver and Rewards
 */
struct FManifestQuestDefinition
{
	FString Name;                        // Asset name (e.g., "QBP_FindArtifact")
	FString Folder;                      // Output folder
	FString QuestName;                   // Display name
	FString QuestDescription;            // Journal description
	bool bTracked = true;                // Show navigation markers
	FString Dialogue;                    // Associated dialogue asset - maps to QuestDialogue (TSubclassOf<UDialogue>)
	FString StartState;                  // ID of starting state (defaults to first state)

	// v4.1: Quest visibility and dialogue control
	bool bHidden = false;                // Quest hidden from journal until discovered
	bool bResumeDialogueAfterLoad = false;  // Resume quest dialogue on save load

	// v4.2: Quest dialogue play params - how to play the quest dialogue
	FManifestDialoguePlayParamsDefinition DialoguePlayParams;

	// v3.9.6: Questgiver and rewards
	FString Questgiver;                  // NPC_ who gives this quest
	FManifestQuestRewardDefinition Rewards;  // Rewards on completion

	// v4.3: Quest requirements - dynamic constraints (fail conditions)
	TArray<FManifestQuestRequirementDefinition> Requirements;

	TArray<FManifestQuestStateDefinition> States;

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		// NOTE: Folder excluded - presentational only
		Hash ^= GetTypeHash(QuestName) << 4;
		Hash ^= GetTypeHash(QuestDescription) << 8;
		Hash ^= (bTracked ? 1ULL : 0ULL) << 12;
		Hash ^= GetTypeHash(Dialogue) << 16;
		Hash ^= GetTypeHash(StartState) << 20;
		// v4.1: Include quest visibility and dialogue control in hash
		Hash ^= (bHidden ? 1ULL : 0ULL) << 22;
		Hash ^= (bResumeDialogueAfterLoad ? 1ULL : 0ULL) << 23;
		// v4.2: Include dialogue play params in hash
		Hash ^= DialoguePlayParams.ComputeHash();
		Hash = (Hash << 5) | (Hash >> 59);
		// v3.9.6: Include questgiver and rewards in hash
		Hash ^= GetTypeHash(Questgiver) << 24;
		Hash ^= Rewards.ComputeHash() << 28;
		// v4.3: Include requirements in hash
		for (const auto& Req : Requirements)
		{
			Hash ^= Req.ComputeHash();
			Hash = (Hash << 4) | (Hash >> 60);
		}
		for (const auto& State : States)
		{
			Hash ^= State.ComputeHash();
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

/**
 * v3.9.8: Pipeline configuration
 */
struct FManifestPipelineConfig
{
	FString DefaultFolder = TEXT("Items/Generated");
	bool bAutoCreateCollections = true;
	bool bAutoCreateLoadouts = false;

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(DefaultFolder);
		Hash ^= bAutoCreateCollections ? 1 : 0;
		Hash ^= bAutoCreateLoadouts ? 2 : 0;
		return Hash;
	}
};

/**
 * v3.9.8: Pipeline item definition (mesh-to-item entry)
 */
struct FManifestPipelineItemDefinition
{
	FString Mesh;                    // Source mesh path
	FString Name;                    // Generated item name (optional, auto-generated if empty)
	FString Type;                    // Item type: Clothing, Weapon_Melee, Weapon_Ranged, Consumable, Generic
	FString Slot;                    // Equipment slot tag
	FString DisplayName;             // Human-readable name
	FString Folder;                  // Target folder
	FManifestClothingMeshConfig ClothingMesh;  // Clothing mesh config
	TMap<FString, FString> Stats;    // Additional stats (armor_rating, attack_rating, etc.)
	FString TargetCollection;        // Target collection to add item to

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Mesh);
		Hash ^= GetTypeHash(Name);
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= GetTypeHash(Type);
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= GetTypeHash(Slot);
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= GetTypeHash(DisplayName);
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= GetTypeHash(Folder);
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= ClothingMesh.ComputeHash();
		Hash = (Hash << 5) | (Hash >> 59);
		Hash ^= GetTypeHash(TargetCollection);
		for (const auto& Stat : Stats)
		{
			Hash ^= GetTypeHash(Stat.Key);
			Hash ^= GetTypeHash(Stat.Value);
			Hash = (Hash << 3) | (Hash >> 61);
		}

		return Hash;
	}
};

/**
 * v3.9.8: Pipeline collection definition (auto-generated from items)
 */
struct FManifestPipelineCollectionDefinition
{
	FString Name;                    // Collection name (IC_ prefix)
	FString Folder;                  // Target folder
	TArray<FString> Items;           // Items to include (auto-populated from pipeline_items with matching TargetCollection)

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(Folder) << 16;
		for (const FString& Item : Items)
		{
			Hash ^= GetTypeHash(Item);
			Hash = (Hash << 5) | (Hash >> 59);
		}
		return Hash;
	}
};

/**
 * v3.9.8: Pipeline loadout definition (NPC loadout assignment)
 */
struct FManifestPipelineLoadoutDefinition
{
	FString NPCDefinition;           // Target NPC definition name
	TArray<FString> Collections;     // Item collections to add to loadout

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(NPCDefinition);
		for (const FString& Collection : Collections)
		{
			Hash ^= GetTypeHash(Collection);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

/**
 * v3.9.9: POI Placement Definition
 * Places APOIActor instances in World Partition levels
 */
struct FManifestPOIPlacement
{
	FString POITag;                      // Narrative.POIs.* tag
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	FString DisplayName;                 // UI display name
	bool bCreateMapMarker = true;
	bool bSupportsFastTravel = false;
	FString MapIcon;                     // Texture path for map icon
	TArray<FString> LinkedPOIs;          // Other POI tags for navigation graph

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(POITag);
		Hash ^= GetTypeHash(FMath::RoundToInt(Location.X));
		Hash ^= GetTypeHash(FMath::RoundToInt(Location.Y));
		Hash ^= GetTypeHash(FMath::RoundToInt(Location.Z));
		Hash ^= GetTypeHash(FMath::RoundToInt(Rotation.Pitch));
		Hash ^= GetTypeHash(FMath::RoundToInt(Rotation.Yaw));
		Hash ^= GetTypeHash(FMath::RoundToInt(Rotation.Roll));
		Hash ^= GetTypeHash(DisplayName);
		Hash ^= (bCreateMapMarker ? 1ULL : 0ULL) << 1;
		Hash ^= (bSupportsFastTravel ? 1ULL : 0ULL) << 2;
		Hash ^= GetTypeHash(MapIcon);
		for (const FString& Link : LinkedPOIs)
		{
			Hash ^= GetTypeHash(Link);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

/**
 * v3.9.9: NPC Spawn Parameters (mirrors FNPCSpawnParams from Narrative Pro)
 */
struct FManifestNPCSpawnParams
{
	bool bOverrideLevelRange = false;
	int32 MinLevel = 1;
	int32 MaxLevel = 1;
	bool bOverrideOwnedTags = false;
	TArray<FString> DefaultOwnedTags;
	bool bOverrideFactions = false;
	TArray<FString> DefaultFactions;
	bool bOverrideActivityConfiguration = false;
	FString ActivityConfiguration;       // AC_*Behavior name (UNPCActivityConfiguration)
	bool bOverrideAppearance = false;
	FString DefaultAppearance;           // Appearance asset path

	uint64 ComputeHash() const
	{
		uint64 Hash = 0;
		if (bOverrideLevelRange) Hash ^= (uint64(MinLevel) << 8) | uint64(MaxLevel);
		for (const FString& Tag : DefaultOwnedTags) { Hash ^= GetTypeHash(Tag); }
		for (const FString& Faction : DefaultFactions) { Hash ^= GetTypeHash(Faction); }
		Hash ^= GetTypeHash(ActivityConfiguration);
		Hash ^= GetTypeHash(DefaultAppearance);
		return Hash;
	}
};

/**
 * v3.9.9: Single NPC entry in a spawner
 */
struct FManifestNPCSpawnEntry
{
	FString NPCDefinition;               // NPC_* name
	FVector RelativeLocation = FVector::ZeroVector;  // Offset from spawner
	FRotator RelativeRotation = FRotator::ZeroRotator;
	FManifestNPCSpawnParams SpawnParams;
	bool bDontSpawnIfKilled = false;
	FString OptionalGoal;                // Goal_* class name
	float UntetherDistance = 5000.0f;

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(NPCDefinition);
		Hash ^= GetTypeHash(FMath::RoundToInt(RelativeLocation.X * 10.f));
		Hash ^= GetTypeHash(FMath::RoundToInt(RelativeLocation.Y * 10.f));
		Hash ^= GetTypeHash(FMath::RoundToInt(RelativeLocation.Z * 10.f));
		Hash ^= SpawnParams.ComputeHash();
		Hash ^= (bDontSpawnIfKilled ? 1ULL : 0ULL) << 4;
		Hash ^= GetTypeHash(OptionalGoal);
		Hash ^= GetTypeHash(FMath::RoundToInt(UntetherDistance));
		return Hash;
	}
};

/**
 * v3.9.9: NPC Spawner Placement Definition
 * Places ANPCSpawner actors with configured UNPCSpawnComponents
 */
struct FManifestNPCSpawnerPlacement
{
	FString Name;                        // Unique spawner name (actor label)
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	FString NearPOI;                     // Optional: use POI location instead of Location
	FVector POIOffset = FVector::ZeroVector;  // Offset from POI if using NearPOI
	bool bActivateOnBeginPlay = true;

	// v3.9.10: Spawner activation via NarrativeEvent
	FString ActivationEvent;             // Optional: NE_* name that enables this spawner when fired
	FString DeactivationEvent;           // Optional: NE_* name that disables this spawner when fired

	TArray<FManifestNPCSpawnEntry> NPCs;

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(FMath::RoundToInt(Location.X));
		Hash ^= GetTypeHash(FMath::RoundToInt(Location.Y));
		Hash ^= GetTypeHash(FMath::RoundToInt(Location.Z));
		Hash ^= GetTypeHash(NearPOI);
		Hash ^= GetTypeHash(FMath::RoundToInt(POIOffset.X));
		Hash ^= GetTypeHash(FMath::RoundToInt(POIOffset.Y));
		Hash ^= GetTypeHash(FMath::RoundToInt(POIOffset.Z));
		Hash ^= (bActivateOnBeginPlay ? 1ULL : 0ULL) << 5;
		Hash ^= GetTypeHash(ActivationEvent);
		Hash ^= GetTypeHash(DeactivationEvent);
		for (const FManifestNPCSpawnEntry& NPC : NPCs)
		{
			Hash ^= NPC.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}
		return Hash;
	}
};

// v4.9: TriggerSet event definition (instanced UNarrativeEvent inside trigger)
struct FManifestTriggerEventDefinition
{
	FString EventClass;                      // NE_* or BPE_* class name
	FString Runtime = TEXT("Start");         // Start, End, Both
	TMap<FString, FString> Properties;       // Property name -> value

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(EventClass);
		Hash ^= GetTypeHash(Runtime);
		for (const auto& Pair : Properties)
		{
			Hash ^= GetTypeHash(Pair.Key);
			Hash ^= GetTypeHash(Pair.Value);
			Hash = (Hash << 3) | (Hash >> 61);
		}
		return Hash;
	}
};

// v4.9: Trigger definition (instanced UNarrativeTrigger inside TriggerSet)
struct FManifestTriggerDefinition
{
	FString TriggerClass;                    // BPT_TimeOfDayRange, BPT_Always, etc.
	float StartTime = 0.0f;                  // For time-based triggers (0-2400)
	float EndTime = 2400.0f;                 // For time-based triggers (0-2400)
	TArray<FManifestTriggerEventDefinition> Events;  // Events to fire when trigger activates

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(TriggerClass);
		Hash ^= GetTypeHash(FMath::RoundToInt(StartTime * 10));
		Hash ^= GetTypeHash(FMath::RoundToInt(EndTime * 10));
		for (const auto& Event : Events)
		{
			Hash ^= Event.ComputeHash();
			Hash = (Hash << 5) | (Hash >> 59);
		}
		return Hash;
	}
};

// v4.9: TriggerSet DataAsset definition
struct FManifestTriggerSetDefinition
{
	FString Name;
	FString Folder;
	TArray<FManifestTriggerDefinition> Triggers;

	uint64 ComputeHash() const
	{
		uint64 Hash = GetTypeHash(Name);
		Hash ^= GetTypeHash(Folder);
		for (const auto& Trigger : Triggers)
		{
			Hash ^= Trigger.ComputeHash();
			Hash = (Hash << 7) | (Hash >> 57);
		}
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
	TArray<FManifestMaterialInstanceDefinition> MaterialInstances; // v4.9: Material Instance Constants
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
	TArray<FManifestGameplayCueDefinition> GameplayCues;  // v4.0: Gameplay Cues
	TArray<FManifestNPCDefinitionDefinition> NPCDefinitions;
	TArray<FManifestCharacterDefinitionDefinition> CharacterDefinitions;
	TArray<FManifestTaggedDialogueSetDefinition> TaggedDialogueSets;
	TArray<FManifestNiagaraSystemDefinition> NiagaraSystems;  // v2.6.5: Niagara VFX systems
	TArray<FManifestFXPresetDefinition> FXPresets;  // v4.9: Reusable Niagara parameter presets
	TArray<FManifestMaterialFunctionDefinition> MaterialFunctions;  // v2.6.12: Material functions

	// v3.9: NPC Pipeline - Schedules, Goals, Quests
	TArray<FManifestActivityScheduleDefinition> ActivitySchedules;
	TArray<FManifestGoalItemDefinition> GoalItems;
	TArray<FManifestQuestDefinition> Quests;

	// v4.8.3: Character Appearances
	TArray<FManifestCharacterAppearanceDefinition> CharacterAppearances;

	// v4.9: TriggerSets (event-driven NPC behaviors)
	TArray<FManifestTriggerSetDefinition> TriggerSets;

	// v3.9.8: Mesh-to-Item Pipeline
	FManifestPipelineConfig PipelineConfig;
	TArray<FManifestPipelineItemDefinition> PipelineItems;
	TArray<FManifestPipelineCollectionDefinition> PipelineCollections;
	TArray<FManifestPipelineLoadoutDefinition> PipelineLoadouts;

	// v3.9.9: POI & NPC Spawner Placements (level actors, not assets)
	TArray<FManifestPOIPlacement> POIPlacements;
	TArray<FManifestNPCSpawnerPlacement> NPCSpawnerPlacements;

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
		for (const auto& Def : GameplayCues) AssetWhitelist.Add(Def.Name);  // v4.0
		for (const auto& Def : NPCDefinitions) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : CharacterDefinitions) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : TaggedDialogueSets) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : NiagaraSystems) AssetWhitelist.Add(Def.Name);
		// FXPresets excluded from whitelist - config data referenced by NiagaraSystems, not standalone assets
		for (const auto& Def : MaterialInstances) AssetWhitelist.Add(Def.Name);  // v4.9
		for (const auto& Def : MaterialFunctions) AssetWhitelist.Add(Def.Name);  // v2.6.12
		// v3.9: NPC Pipeline
		for (const auto& Def : ActivitySchedules) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : GoalItems) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : Quests) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : CharacterAppearances) AssetWhitelist.Add(Def.Name);
		for (const auto& Def : TriggerSets) AssetWhitelist.Add(Def.Name);  // v4.9

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
			+ GameplayCues.Num()  // v4.0
			+ NPCDefinitions.Num()
			+ CharacterDefinitions.Num()
			+ TaggedDialogueSets.Num()
			+ NiagaraSystems.Num()
			+ MaterialFunctions.Num()
			// v4.9: VFX system - FXPresets excluded (config data, not standalone assets)
			+ MaterialInstances.Num()
			// v3.9: NPC Pipeline
			+ ActivitySchedules.Num()
			+ GoalItems.Num()
			+ Quests.Num()
			+ CharacterAppearances.Num()
			// v4.9: TriggerSets
			+ TriggerSets.Num();
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
		for (const auto& Def : GameplayCues) AddWithDupeCheck(Def.Name);  // v4.0
		for (const auto& Def : NPCDefinitions) AddWithDupeCheck(Def.Name);
		for (const auto& Def : CharacterDefinitions) AddWithDupeCheck(Def.Name);
		for (const auto& Def : TaggedDialogueSets) AddWithDupeCheck(Def.Name);
		for (const auto& Def : NiagaraSystems) AddWithDupeCheck(Def.Name);
		for (const auto& Def : MaterialFunctions) AddWithDupeCheck(Def.Name);
		// v4.9: VFX system - FXPresets excluded (config data, not standalone assets)
		for (const auto& Def : MaterialInstances) AddWithDupeCheck(Def.Name);
		// v3.9: NPC Pipeline
		for (const auto& Def : ActivitySchedules) AddWithDupeCheck(Def.Name);
		for (const auto& Def : GoalItems) AddWithDupeCheck(Def.Name);
		for (const auto& Def : Quests) AddWithDupeCheck(Def.Name);
		for (const auto& Def : CharacterAppearances) AddWithDupeCheck(Def.Name);
		// v4.9: TriggerSets
		for (const auto& Def : TriggerSets) AddWithDupeCheck(Def.Name);
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
		Total += GameplayCues.Num();  // v4.0
		Total += NPCDefinitions.Num();
		Total += CharacterDefinitions.Num();
		Total += TaggedDialogueSets.Num();
		Total += NiagaraSystems.Num();
		Total += MaterialFunctions.Num();  // v2.6.12
		// v4.9: VFX system - FXPresets excluded (config data, not standalone assets)
		Total += MaterialInstances.Num();
		// v3.9: NPC Pipeline
		Total += ActivitySchedules.Num();
		Total += GoalItems.Num();
		Total += Quests.Num();
		Total += CharacterAppearances.Num();
		// v4.9: TriggerSets
		Total += TriggerSets.Num();

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
