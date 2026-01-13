// GasAbilityGenerator v3.3
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v3.3: NPCDefinition, EquippableItem & Activity Enhancement:
//       NPCDefinition: Dialogue, TaggedDialogueSet, vendor properties, inherited properties
//       EquippableItem: DisplayName, Description, AttackRating, ArmorRating, StealthRating,
//         Weight, BaseValue, BaseScore, ItemTags, bStackable, MaxStackSize, UseRechargeDuration, Thumbnail
//       Activity: ActivityName, OwnedTags, BlockTags, RequireTags, SupportedGoalType,
//         bIsInterruptable, bSaveActivity
// v3.0: Regen/Diff Safety System - Universal metadata tracking, dry run mode, hash-based change detection
//       StoreAssetMetadata/GetAssetMetadata, CheckExistsWithMetadata, ComputeBlueprintOutputHash
// v2.9.1: FX Validation System - Template integrity checking, descriptor validation,
//         regeneration safety with metadata tracking and hash comparison
// v2.9.0: Data-driven FX architecture - FXDescriptor for Niagara User param binding
//         Duplicates template systems and applies User.* parameters from manifest
// v2.8.3: Function override support for parent class functions (HandleDeath, etc.)
//         - Creates function graph with entry node and CallParent node
//         - Supports all standard node types in function overrides
// v2.8.2: CallFunction parameter defaults - applies "param.*" properties to function input pins
//         - Supports enum value conversion (e.g., "Flying" -> MOVE_Flying for EMovementMode)
//         - Enhanced pre-generation validation with comprehensive error reporting
// v2.7.5: Added K2_ClearTimerHandle to deprecated function remapping
//         - Added STABLE SECTION markers for critical code protection
//         - Manifest fixes: GetOwnASC for ASC Target, MakeTransform for SpawnActor
// v2.7.4: Comprehensive pre-generation validation with specific error detection
//         - Detects multiple exec connections from same output pin (use Sequence node)
//         - Detects missing ASC Target connections (ApplyGameplayEffectToTarget etc)
//         - Detects unconnected array inputs (Object Types needs MakeArray)
//         - Detects missing Actor pin connections when self is not Actor
//         - Auto-remaps deprecated functions (ClearTimerByHandle -> K2_ClearAndInvalidateTimerHandle)
// v2.7.3: Enhanced pin connection validation with detailed type logging
//         - Shows full type info including class names (e.g., object<NarrativeInventoryComponent>)
//         - Fails connection at generation time when types are incompatible (CONNECT_RESPONSE_DISALLOW)
//         - UE_LOG errors for pin type mismatches to catch issues before Blueprint compile
// v2.7.0: BreakStruct, MakeArray, GetArrayItem node support for weapon form implementation
// v2.6.11: Force scan NarrativePro plugin content in commandlet mode for parent class resolution
// v2.6.10: Enhanced Niagara generator with warmup, bounds, determinism, and pooling settings
// v2.6.9: Animation Notify generator, Narrative Pro path support for parent classes and abilities
// v2.6.8: EquippableItemGenerator now uses ParentClass from manifest (RangedWeaponItem, MeleeWeaponItem support)
// v2.6.7: Deferred asset retry mechanism for dependency resolution
// v2.6.6: GE assets created as Blueprints for CooldownGameplayEffectClass compatibility
// v2.6.5: Added Niagara System generator
// v2.6.0: MAJOR AUTOMATION ENHANCEMENT - Maximized automation for all generators:
//         - Blackboard: Now creates keys from manifest (Bool, Int, Float, Vector, Object, etc.)
//         - Animation Montage: Now loads skeleton and creates montage with sections
//         - Equippable Item: Sets EquippableSlots, EquipmentEffect via soft references
//         - Activity: Sets BehaviourTree via LoadObject
//         - NPC Definition: Sets NPCClassPath, ActivityConfiguration via TSoftClassPtr/TSoftObjectPtr
//         - Item Collection: Sets items with quantities via TSoftClassPtr
//         - Tagged Dialogue Set: Sets Dialogue class via TSoftClassPtr
//         - Added all inactive generators to commandlet loop
// v2.5.4: Fixed DynamicCast node Object pin connection - added NotifyPinConnectionListChanged calls
//         Fixed Branch node pin names (True/False -> then/else aliases)
// v2.5.2: Fixed duplicate event node creation - now checks if event exists before creating
//         Enhanced ConnectPins with type compatibility check and connection verification
//         Added post-generation validation for unconnected pins
//         Added more pin name aliases (Parent, SocketName, Mesh, InputObject)
// v2.5.0: Auto-detects project name from Unreal Engine (no need to set project_root in manifest)
//         All hardcoded paths replaced with GetProjectRoot() for project-agnostic usage
// v2.4.3: Added explicit function lookup table for well-known UE/GAS functions
//         Fixed GetMesh (property access), GetCharacterMovement, GAS functions
//         Added GetComponentByClass, GetSocketLocation, GetSocketRotation support
// v2.4.1: Fixed event graph function/class resolution - now searches Blueprint parent class hierarchy
//         Enhanced FindParentClass to find Blueprint classes via Asset Registry
//         Enhanced FindPinByName with comprehensive pin name aliases
// v2.4.0: Added inline event graph and variables support for gameplay abilities, ForEachLoop node
// v2.3.0: Added 12 new asset type generators with dependency-based generation order
// v2.2.0: Added event graph generator - creates Blueprint nodes and connections
// v2.1.9: Added manifest validation to prevent creating unlisted assets
// v2.1.8: Added enumeration generator and FindUserDefinedEnum helper
// v2.1.6: Fixed variable creation in Actor and Widget Blueprints

#include "GasAbilityGeneratorGenerators.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/BlueprintFactory.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/UserDefinedEnum.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "AttributeSet.h"
#include "ScalableFloat.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/Decorators/BTDecorator_Blackboard.h"
#include "BehaviorTree/BTService.h"
// v2.6.0: Blackboard key types for full key automation
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_String.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Name.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Rotator.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Class.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
// v2.6.0: Animation montage automation
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
// v2.6.9: Animation notify types
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/Material.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "Blueprint/UserWidget.h"
#include "EdGraphSchema_K2.h"
// v2.6.0: Reflection for setting protected properties
#include "UObject/UnrealType.h"

// v3.0: Narrative Pro types for ComputeDataAssetOutputHash
#include "GAS/AbilityConfiguration.h"
#include "AI/Activities/NPCActivityConfiguration.h"
#include "Items/InventoryComponent.h"  // Contains UItemCollection
#include "AI/NPCDefinition.h"
#include "Character/CharacterDefinition.h"
#include "Character/CharacterAppearance.h"  // v3.3: UCharacterAppearanceBase for NPCDefinition
#include "Tales/TaggedDialogueSet.h"
#include "Tales/Dialogue.h"  // v3.3: UDialogue for NPCDefinition
#include "NiagaraSystem.h"

// v2.3.0: Component includes for Actor Blueprint generation
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"

// v2.2.0: Event Graph Generation includes
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_CallFunction.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_MacroInstance.h"  // v2.4.0: ForEachLoop support
#include "K2Node_SpawnActorFromClass.h"  // v2.4.0: SpawnActor support
#include "K2Node_BreakStruct.h"  // v2.7.0: BreakStruct support
#include "K2Node_MakeArray.h"  // v2.7.0: MakeArray support
#include "K2Node_GetArrayItem.h"  // v2.7.0: GetArrayItem support
#include "K2Node_Self.h"  // v2.7.8: Self reference support
#include "K2Node_FunctionEntry.h"  // v2.8.3: Function override entry
#include "K2Node_CallParentFunction.h"  // v2.8.3: Call parent function
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Misc/App.h"  // v2.5.0: For FApp::GetProjectName()

// v2.7.0: Narrative Pro includes for WellKnownFunctions table
#include "Items/InventoryComponent.h"
#include "Items/WeaponItem.h"
#include "Items/EquippableItem.h"

// v2.1.9: Static member initialization for manifest validation
const FManifestData* FGeneratorBase::ActiveManifest = nullptr;

// v3.0: Static member initialization for dry run and force mode
bool FGeneratorBase::bDryRunMode = false;
bool FGeneratorBase::bForceMode = false;
FDryRunSummary FGeneratorBase::DryRunSummary;
FString FGeneratorBase::CurrentManifestPath;

// v3.0: Generator version constant for metadata tracking
static const FString GENERATOR_VERSION = TEXT("3.0");

// v3.0: Helper function to store metadata after successful Blueprint generation
static void StoreBlueprintMetadata(
	UBlueprint* Blueprint,
	const FString& GeneratorId,
	const FString& ManifestAssetKey,
	uint64 InputHash)
{
	if (!Blueprint)
	{
		return;
	}

	FGeneratorMetadata Metadata;
	Metadata.GeneratorId = GeneratorId;
	Metadata.ManifestPath = FGeneratorBase::GetManifestPath();
	Metadata.ManifestAssetKey = ManifestAssetKey;
	Metadata.InputHash = InputHash;
	Metadata.OutputHash = FGeneratorBase::ComputeBlueprintOutputHash(Blueprint);
	Metadata.GeneratorVersion = GENERATOR_VERSION;
	Metadata.Timestamp = FDateTime::Now();
	Metadata.bIsGenerated = true;

	FGeneratorBase::StoreAssetMetadata(Blueprint, Metadata);
}

// v3.0: Helper function to store metadata after successful DataAsset generation
static void StoreDataAssetMetadata(
	UObject* Asset,
	const FString& GeneratorId,
	const FString& ManifestAssetKey,
	uint64 InputHash)
{
	if (!Asset)
	{
		return;
	}

	FGeneratorMetadata Metadata;
	Metadata.GeneratorId = GeneratorId;
	Metadata.ManifestPath = FGeneratorBase::GetManifestPath();
	Metadata.ManifestAssetKey = ManifestAssetKey;
	Metadata.InputHash = InputHash;
	Metadata.OutputHash = FGeneratorBase::ComputeDataAssetOutputHash(Asset);
	Metadata.GeneratorVersion = GENERATOR_VERSION;
	Metadata.Timestamp = FDateTime::Now();
	Metadata.bIsGenerated = true;

	FGeneratorBase::StoreAssetMetadata(Asset, Metadata);
}

// v2.1.8: Global variable to track project root for enum lookups
static FString GCurrentProjectRoot = TEXT("");

// v2.5.0: Helper to get project root - auto-detects from project name if not set
static FString GetProjectRoot()
{
	// If project_root was explicitly set in manifest, use it
	if (!GCurrentProjectRoot.IsEmpty() && GCurrentProjectRoot != TEXT("/Game"))
	{
		return GCurrentProjectRoot;
	}

	// v2.5.0: Auto-detect project name from Unreal Engine
	FString ProjectName = FApp::GetProjectName();
	if (!ProjectName.IsEmpty())
	{
		return FString::Printf(TEXT("/Game/%s"), *ProjectName);
	}

	// Ultimate fallback
	return TEXT("/Game");
}

// ============================================================================
// FGeneratorBase Implementation
// ============================================================================

// v2.1.9: Set the active manifest for validation
// v2.5.5: Also sets GCurrentProjectRoot for consistent path generation across all generators
void FGeneratorBase::SetActiveManifest(const FManifestData* Manifest)
{
	ActiveManifest = Manifest;
	if (ActiveManifest)
	{
		// v2.5.5: Set the global project root from manifest
		if (!ActiveManifest->ProjectRoot.IsEmpty())
		{
			GCurrentProjectRoot = ActiveManifest->ProjectRoot;
			LogGeneration(FString::Printf(TEXT("Project root set to: %s"), *GCurrentProjectRoot));
		}

		// Build the whitelist immediately for efficient lookups
		ActiveManifest->BuildAssetWhitelist();
		LogGeneration(FString::Printf(TEXT("Manifest validation enabled with %d assets in whitelist"),
			ActiveManifest->GetAssetWhitelist().Num()));
	}
}

// v2.1.9: Clear the active manifest
// v2.5.5: Also clears GCurrentProjectRoot
void FGeneratorBase::ClearActiveManifest()
{
	ActiveManifest = nullptr;
	GCurrentProjectRoot = TEXT("");
	LogGeneration(TEXT("Manifest validation disabled, project root cleared"));
}

// v2.1.9: Validate an asset name against the manifest whitelist
bool FGeneratorBase::ValidateAgainstManifest(
	const FString& AssetName,
	const FString& AssetType,
	FGenerationResult& OutResult)
{
	// If no manifest is set, allow all (backwards compatibility)
	if (!ActiveManifest)
	{
		return false; // No validation failure
	}

	// Check if asset is in the whitelist
	if (!ActiveManifest->IsAssetInManifest(AssetName))
	{
		LogGeneration(FString::Printf(TEXT("BLOCKED: %s '%s' not in manifest whitelist"), *AssetType, *AssetName));
		OutResult = FGenerationResult(AssetName, EGenerationStatus::Failed,
			TEXT("Asset not in manifest whitelist"));
		OutResult.DetermineCategory();
		return true; // Validation failed - asset should NOT be created
	}

	return false; // Validation passed - asset can be created
}

bool FGeneratorBase::DoesAssetExistOnDisk(const FString& AssetPath)
{
	// Convert content path to package name
	FString PackageName = AssetPath;
	if (!PackageName.StartsWith(TEXT("/")))
	{
		PackageName = TEXT("/") + PackageName;
	}
	
	return FPackageName::DoesPackageExist(PackageName);
}

bool FGeneratorBase::IsAssetInMemory(const FString& AssetPath)
{
	FString PackageName = AssetPath;
	if (!PackageName.StartsWith(TEXT("/")))
	{
		PackageName = TEXT("/") + PackageName;
	}
	
	return FindPackage(nullptr, *PackageName) != nullptr;
}

bool FGeneratorBase::CheckExistsAndPopulateResult(
	const FString& AssetPath,
	const FString& AssetName,
	const FString& AssetType,
	FGenerationResult& OutResult)
{
	bool bExists = DoesAssetExistOnDisk(AssetPath) || IsAssetInMemory(AssetPath);

	// v3.0: In dry run mode, add results and always return true to skip actual generation
	if (IsDryRunMode())
	{
		FDryRunResult DryRunResult;
		DryRunResult.AssetName = AssetName;
		DryRunResult.AssetPath = AssetPath;
		DryRunResult.GeneratorId = AssetType;

		if (bExists)
		{
			if (IsForceMode())
			{
				// Force mode - will regenerate even though it exists
				DryRunResult.Status = EDryRunStatus::WillModify;
				DryRunResult.Reason = TEXT("Force regeneration");
				LogGeneration(FString::Printf(TEXT("[DRY RUN] MODIFY %s: %s - Force regeneration"), *AssetType, *AssetName));
			}
			else
			{
				// Asset exists - will skip (no metadata check in basic mode)
				DryRunResult.Status = EDryRunStatus::WillSkip;
				DryRunResult.Reason = TEXT("Already exists");
				LogGeneration(FString::Printf(TEXT("[DRY RUN] SKIP %s: %s - Already exists"), *AssetType, *AssetName));
			}
		}
		else
		{
			// Asset doesn't exist - will create
			DryRunResult.Status = EDryRunStatus::WillCreate;
			DryRunResult.Reason = TEXT("New asset");
			LogGeneration(FString::Printf(TEXT("[DRY RUN] CREATE %s: %s"), *AssetType, *AssetName));
		}

		AddDryRunResult(DryRunResult);

		// In dry run mode, always skip actual generation
		OutResult = FGenerationResult(AssetName, EGenerationStatus::Skipped, TEXT("Dry run mode"));
		OutResult.DetermineCategory();
		return true;
	}

	// v3.0: In force mode, regenerate even if asset exists
	if (IsForceMode() && bExists)
	{
		LogGeneration(FString::Printf(TEXT("[FORCE] Regenerating %s: %s"), *AssetType, *AssetName));
		return false; // Don't skip - regenerate
	}

	// Normal mode - check existence and skip if exists
	if (bExists)
	{
		LogGeneration(FString::Printf(TEXT("Skipping existing %s: %s"), *AssetType, *AssetName));
		OutResult = FGenerationResult(AssetName, EGenerationStatus::Skipped, TEXT("Already exists"));
		OutResult.DetermineCategory();
		return true; // Asset exists, should skip
	}

	return false; // Asset doesn't exist, should generate
}

// ============================================================================
// v2.9.1: Generalized Validation Utilities Implementation
// ============================================================================

bool FGeneratorBase::ValidateRequiredField(const FString& AssetName, const FString& FieldName,
	const FString& Value, FPreValidationResult& OutResult)
{
	if (Value.IsEmpty())
	{
		OutResult.AddIssue(FValidationIssue::RequiredField(AssetName, FieldName));
		return false;
	}
	return true;
}

bool FGeneratorBase::ValidateAssetReference(const FString& AssetName, const FString& FieldName,
	const FString& ReferencePath, FPreValidationResult& OutResult)
{
	if (ReferencePath.IsEmpty())
	{
		// Empty reference is valid (optional)
		return true;
	}

	// Try to find the referenced asset
	if (DoesAssetExistOnDisk(ReferencePath))
	{
		return true;
	}

	// Try common search patterns
	TArray<FString> SearchPaths;
	if (!ReferencePath.Contains(TEXT("/")))
	{
		// Short name - search common locations
		SearchPaths.Add(FString::Printf(TEXT("/Game/%s"), *ReferencePath));
		SearchPaths.Add(FString::Printf(TEXT("/Game/Blueprints/%s"), *ReferencePath));
		SearchPaths.Add(FString::Printf(TEXT("/Game/Characters/%s"), *ReferencePath));
		SearchPaths.Add(FString::Printf(TEXT("/Game/VFX/%s"), *ReferencePath));
	}

	for (const FString& Path : SearchPaths)
	{
		if (DoesAssetExistOnDisk(Path))
		{
			return true;
		}
	}

	// Also check if it's a native class
	if (FindParentClass(ReferencePath) != nullptr)
	{
		return true;
	}

	OutResult.AddIssue(FValidationIssue::MissingReference(AssetName, FieldName, ReferencePath));
	return false;
}

bool FGeneratorBase::ValidateParentClass(const FString& AssetName, const FString& ClassName,
	FPreValidationResult& OutResult)
{
	if (ClassName.IsEmpty())
	{
		// Empty means use default parent - valid
		return true;
	}

	UClass* FoundClass = FindParentClass(ClassName);
	if (FoundClass)
	{
		return true;
	}

	// Parent class not found - this is a warning, not error
	// Because FindParentClass may not find all classes (plugins, etc.)
	OutResult.AddIssue(FValidationIssue(AssetName, TEXT("ParentClass"),
		FString::Printf(TEXT("Parent class '%s' not found (may exist in plugin)"), *ClassName),
		EValidationSeverity::Warning, EValidationCategory::Reference));

	return true; // Return true because this is just a warning
}

bool FGeneratorBase::ValidateFloatRange(const FString& AssetName, const FString& FieldName,
	float Value, float Min, float Max, FPreValidationResult& OutResult)
{
	if (Max > Min && (Value < Min || Value > Max))
	{
		OutResult.AddIssue(FValidationIssue::ValueOutOfRange(AssetName, FieldName, Value, Min, Max));
		return false;
	}
	return true;
}

bool FGeneratorBase::ValidateNamePrefix(const FString& AssetName, const FString& ExpectedPrefix,
	FPreValidationResult& OutResult)
{
	if (!ExpectedPrefix.IsEmpty() && !AssetName.StartsWith(ExpectedPrefix))
	{
		OutResult.AddIssue(FValidationIssue(AssetName, TEXT("Name"),
			FString::Printf(TEXT("Asset name should start with '%s'"), *ExpectedPrefix),
			EValidationSeverity::Warning, EValidationCategory::Format));
		return false;
	}
	return true;
}

bool FGeneratorBase::ValidateNonEmptyArray(const FString& AssetName, const FString& FieldName,
	int32 ArraySize, bool bRequired, FPreValidationResult& OutResult)
{
	if (bRequired && ArraySize == 0)
	{
		OutResult.AddIssue(FValidationIssue(AssetName, FieldName,
			TEXT("Array cannot be empty"),
			EValidationSeverity::Error, EValidationCategory::Required));
		return false;
	}
	return true;
}

bool FGeneratorBase::HandleValidationResult(const FPreValidationResult& Validation,
	const FString& AssetName, FGenerationResult& OutResult)
{
	if (Validation.HasIssues())
	{
		Validation.LogIssues();
	}

	if (!Validation.IsValid())
	{
		OutResult = FGenerationResult(AssetName, EGenerationStatus::Failed,
			FString::Printf(TEXT("Pre-validation failed: %s"), *Validation.GetSummary()));
		OutResult.DetermineCategory();
		return false;
	}

	return true;
}

// ============================================================================
// v3.0: Regen/Diff Safety - Metadata and Dry Run Implementation
// ============================================================================

void FGeneratorBase::StoreAssetMetadata(UObject* Asset, const FGeneratorMetadata& Metadata)
{
	GeneratorMetadataHelpers::SetMetadata(Asset, Metadata);
}

UGeneratorAssetMetadata* FGeneratorBase::GetAssetMetadata(UObject* Asset)
{
	return GeneratorMetadataHelpers::GetMetadata(Asset);
}

uint64 FGeneratorBase::ComputeBlueprintOutputHash(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return 0;
	}

	uint64 Hash = 0;

	// Hash basic Blueprint info
	Hash ^= GetTypeHash(Blueprint->GetName());
	Hash ^= GetTypeHash(Blueprint->ParentClass ? Blueprint->ParentClass->GetName() : TEXT("None")) << 8;

	// Hash new variable count
	Hash ^= static_cast<uint64>(Blueprint->NewVariables.Num()) << 16;

	// Hash each variable name and type
	for (const FBPVariableDescription& Var : Blueprint->NewVariables)
	{
		Hash ^= GetTypeHash(Var.VarName.ToString());
		Hash ^= GetTypeHash(Var.VarType.PinCategory.ToString()) << 4;
		Hash = (Hash << 3) | (Hash >> 61);
	}

	// Hash graphs
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (!Graph) continue;

		// Hash node count
		Hash ^= static_cast<uint64>(Graph->Nodes.Num()) << 24;

		// Hash each node's class name
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node) continue;
			Hash ^= GetTypeHash(Node->GetClass()->GetName());
			Hash = (Hash << 5) | (Hash >> 59);

			// Hash pin count
			Hash ^= static_cast<uint64>(Node->Pins.Num()) << 8;
		}
	}

	return Hash;
}

uint64 FGeneratorBase::ComputeDataAssetOutputHash(UObject* DataAsset)
{
	if (!DataAsset)
	{
		return 0;
	}

	uint64 Hash = 0;

	// Hash basic info
	Hash ^= GetTypeHash(DataAsset->GetName());
	Hash ^= GetTypeHash(DataAsset->GetClass()->GetName()) << 8;

	// v3.0: Type-specific hashing for manual edit detection

	// Blackboard - hash keys
	if (UBlackboardData* Blackboard = Cast<UBlackboardData>(DataAsset))
	{
		Hash ^= static_cast<uint64>(Blackboard->Keys.Num()) << 16;
		for (const FBlackboardEntry& Entry : Blackboard->Keys)
		{
			Hash ^= GetTypeHash(Entry.EntryName);
			if (Entry.KeyType)
			{
				Hash ^= GetTypeHash(Entry.KeyType->GetClass()->GetName()) << 4;
			}
		}
		return Hash;
	}

	// GameplayEffect (non-Blueprint) - hash modifiers and tags
	if (UGameplayEffect* Effect = Cast<UGameplayEffect>(DataAsset))
	{
		Hash ^= static_cast<uint64>(Effect->Modifiers.Num()) << 16;
		for (const FGameplayModifierInfo& Mod : Effect->Modifiers)
		{
			Hash ^= GetTypeHash(Mod.Attribute.GetName());
			Hash ^= static_cast<uint64>(Mod.ModifierOp) << 4;
		}
		Hash ^= static_cast<uint64>(Effect->DurationPolicy) << 24;

		// Hash granted tags count
		Hash ^= static_cast<uint64>(Effect->GetGrantedTags().Num()) << 32;
		return Hash;
	}

	// InputAction - hash value type and triggers
	if (UInputAction* InputAction = Cast<UInputAction>(DataAsset))
	{
		Hash ^= static_cast<uint64>(InputAction->ValueType) << 16;
		Hash ^= static_cast<uint64>(InputAction->Triggers.Num()) << 24;
		Hash ^= static_cast<uint64>(InputAction->Modifiers.Num()) << 32;
		return Hash;
	}

	// InputMappingContext - hash mappings
	if (UInputMappingContext* IMC = Cast<UInputMappingContext>(DataAsset))
	{
		Hash ^= static_cast<uint64>(IMC->GetMappings().Num()) << 16;
		for (const FEnhancedActionKeyMapping& Mapping : IMC->GetMappings())
		{
			if (Mapping.Action)
			{
				Hash ^= GetTypeHash(Mapping.Action->GetName());
			}
			Hash ^= GetTypeHash(Mapping.Key.GetFName()) << 4;
		}
		return Hash;
	}

	// BehaviorTree - hash root node and blackboard reference
	if (UBehaviorTree* BT = Cast<UBehaviorTree>(DataAsset))
	{
		Hash ^= BT->RootNode ? 1ULL << 16 : 0ULL;
		if (BT->BlackboardAsset)
		{
			Hash ^= GetTypeHash(BT->BlackboardAsset->GetName()) << 24;
		}
		return Hash;
	}

	// UserDefinedEnum - hash enum values
	if (UUserDefinedEnum* Enum = Cast<UUserDefinedEnum>(DataAsset))
	{
		Hash ^= static_cast<uint64>(Enum->NumEnums()) << 16;
		for (int32 i = 0; i < Enum->NumEnums(); i++)
		{
			Hash ^= GetTypeHash(Enum->GetNameStringByIndex(i));
		}
		return Hash;
	}

	// FloatCurve - hash curve keys
	if (UCurveFloat* Curve = Cast<UCurveFloat>(DataAsset))
	{
		TArray<FRichCurveKey> Keys = Curve->FloatCurve.GetCopyOfKeys();
		Hash ^= static_cast<uint64>(Keys.Num()) << 16;
		for (const FRichCurveKey& Key : Keys)
		{
			Hash ^= GetTypeHash(Key.Time) << 4;
			Hash ^= GetTypeHash(Key.Value) << 8;
		}
		return Hash;
	}

	// AnimMontage - hash sections, slots, blend times
	if (UAnimMontage* Montage = Cast<UAnimMontage>(DataAsset))
	{
		Hash ^= static_cast<uint64>(Montage->SlotAnimTracks.Num()) << 16;
		Hash ^= static_cast<uint64>(Montage->CompositeSections.Num()) << 24;
		// Cast to uint64 before shifting to avoid overflow
		Hash ^= static_cast<uint64>(GetTypeHash(Montage->BlendIn.GetBlendTime())) << 8;
		Hash ^= static_cast<uint64>(GetTypeHash(Montage->BlendOut.GetBlendTime())) << 12;
		for (const FSlotAnimationTrack& Track : Montage->SlotAnimTracks)
		{
			Hash ^= GetTypeHash(Track.SlotName);
		}
		return Hash;
	}

	// Material - hash blend mode, shading models, two-sided
	if (UMaterial* Material = Cast<UMaterial>(DataAsset))
	{
		Hash ^= static_cast<uint64>(Material->GetBlendMode()) << 16;
		Hash ^= static_cast<uint64>(Material->GetShadingModels().GetShadingModelField()) << 24;
		Hash ^= Material->TwoSided ? 1ULL << 32 : 0ULL;
		// Use expression count via reflection
		Hash ^= static_cast<uint64>(Material->GetExpressions().Num()) << 40;
		return Hash;
	}

	// MaterialFunction - hash input/output count
	if (UMaterialFunction* MatFunc = Cast<UMaterialFunction>(DataAsset))
	{
		TArray<FFunctionExpressionInput> Inputs;
		TArray<FFunctionExpressionOutput> Outputs;
		MatFunc->GetInputsAndOutputs(Inputs, Outputs);
		Hash ^= static_cast<uint64>(Inputs.Num()) << 16;
		Hash ^= static_cast<uint64>(Outputs.Num()) << 24;
		// Hash input count only - avoids need for full type definition
		return Hash;
	}

	// AbilityConfiguration - hash abilities list
	if (UAbilityConfiguration* AbilityConfig = Cast<UAbilityConfiguration>(DataAsset))
	{
		Hash ^= static_cast<uint64>(AbilityConfig->DefaultAbilities.Num()) << 16;
		Hash ^= static_cast<uint64>(AbilityConfig->StartupEffects.Num()) << 24;
		Hash ^= AbilityConfig->DefaultAttributes ? 1ULL << 32 : 0ULL;
		for (const TSubclassOf<UGameplayAbility>& Ability : AbilityConfig->DefaultAbilities)
		{
			if (Ability.Get())
			{
				Hash ^= GetTypeHash(Ability.Get()->GetName());
			}
		}
		return Hash;
	}

	// NPCActivityConfiguration - hash activities and rescore interval
	if (UNPCActivityConfiguration* ActivityConfig = Cast<UNPCActivityConfiguration>(DataAsset))
	{
		Hash ^= static_cast<uint64>(GetTypeHash(ActivityConfig->RescoreInterval)) << 16;
		Hash ^= static_cast<uint64>(ActivityConfig->DefaultActivities.Num()) << 24;
		Hash ^= static_cast<uint64>(ActivityConfig->GoalGenerators.Num()) << 32;
		for (const TSubclassOf<UNPCActivity>& Activity : ActivityConfig->DefaultActivities)
		{
			if (Activity.Get())
			{
				Hash ^= GetTypeHash(Activity.Get()->GetName());
			}
		}
		return Hash;
	}

	// ItemCollection - hash items
	if (UItemCollection* ItemColl = Cast<UItemCollection>(DataAsset))
	{
		Hash ^= static_cast<uint64>(ItemColl->Items.Num()) << 16;
		for (const FItemWithQuantity& Item : ItemColl->Items)
		{
			Hash ^= static_cast<uint64>(Item.Quantity) << 4;
			if (!Item.Item.IsNull())
			{
				Hash ^= GetTypeHash(Item.Item.GetAssetName());
			}
		}
		return Hash;
	}

	// NPCDefinition - hash NPC properties
	if (UNPCDefinition* NPCDef = Cast<UNPCDefinition>(DataAsset))
	{
		Hash ^= GetTypeHash(NPCDef->NPCID) << 16;
		Hash ^= GetTypeHash(NPCDef->NPCName.ToString()) << 8;
		Hash ^= static_cast<uint64>(NPCDef->MinLevel) << 24;
		Hash ^= static_cast<uint64>(NPCDef->MaxLevel) << 32;
		Hash ^= NPCDef->bAllowMultipleInstances ? 1ULL << 40 : 0ULL;
		Hash ^= NPCDef->bIsVendor ? 1ULL << 41 : 0ULL;
		if (NPCDef->AbilityConfiguration)
		{
			Hash ^= GetTypeHash(NPCDef->AbilityConfiguration->GetName());
		}
		return Hash;
	}

	// CharacterDefinition - hash character properties
	if (UCharacterDefinition* CharDef = Cast<UCharacterDefinition>(DataAsset))
	{
		Hash ^= static_cast<uint64>(CharDef->DefaultCurrency) << 16;
		Hash ^= static_cast<uint64>(CharDef->AttackPriority) << 24;
		Hash ^= static_cast<uint64>(CharDef->DefaultOwnedTags.Num()) << 32;
		Hash ^= static_cast<uint64>(CharDef->DefaultFactions.Num()) << 40;
		return Hash;
	}

	// TaggedDialogueSet - hash dialogues
	if (UTaggedDialogueSet* DialogueSet = Cast<UTaggedDialogueSet>(DataAsset))
	{
		Hash ^= static_cast<uint64>(DialogueSet->TaggedDialogues.Num()) << 16;
		for (const FTaggedDialogue& Dialogue : DialogueSet->TaggedDialogues)
		{
			Hash ^= GetTypeHash(Dialogue.Tag.GetTagName());
			Hash ^= static_cast<uint64>(GetTypeHash(Dialogue.Cooldown)) << 4;
			Hash ^= static_cast<uint64>(GetTypeHash(Dialogue.MaxDistance)) << 8;
		}
		return Hash;
	}

	// NiagaraSystem - hash emitters and user parameters
	if (UNiagaraSystem* NiagaraSys = Cast<UNiagaraSystem>(DataAsset))
	{
		Hash ^= static_cast<uint64>(NiagaraSys->GetNumEmitters()) << 16;
		Hash ^= static_cast<uint64>(GetTypeHash(NiagaraSys->GetWarmupTime())) << 24;
		// Hash user parameter count via GetUserParameters
		TArray<FNiagaraVariable> UserParamArray;
		NiagaraSys->GetExposedParameters().GetUserParameters(UserParamArray);
		Hash ^= static_cast<uint64>(UserParamArray.Num()) << 32;
		return Hash;
	}

	// Fallback: Hash property count for unknown types
	int32 PropertyCount = 0;
	for (TFieldIterator<FProperty> PropIt(DataAsset->GetClass()); PropIt; ++PropIt)
	{
		PropertyCount++;
	}
	Hash ^= static_cast<uint64>(PropertyCount) << 16;

	return Hash;
}

void FGeneratorBase::SetDryRunMode(bool bEnabled)
{
	bDryRunMode = bEnabled;
	if (bEnabled)
	{
		ClearDryRunSummary();
		LogGeneration(TEXT("Dry run mode enabled - no assets will be modified"));
	}
	else
	{
		LogGeneration(TEXT("Dry run mode disabled"));
	}
}

bool FGeneratorBase::IsDryRunMode()
{
	return bDryRunMode;
}

void FGeneratorBase::SetForceMode(bool bEnabled)
{
	bForceMode = bEnabled;
	if (bEnabled)
	{
		LogGeneration(TEXT("Force mode enabled - will overwrite even on conflicts"));
	}
	else
	{
		LogGeneration(TEXT("Force mode disabled"));
	}
}

bool FGeneratorBase::IsForceMode()
{
	return bForceMode;
}

const FDryRunSummary& FGeneratorBase::GetDryRunSummary()
{
	return DryRunSummary;
}

void FGeneratorBase::AddDryRunResult(const FDryRunResult& Result)
{
	DryRunSummary.AddResult(Result);
	LogGeneration(Result.ToString());
}

void FGeneratorBase::ClearDryRunSummary()
{
	DryRunSummary.Reset();
}

void FGeneratorBase::SetManifestPath(const FString& Path)
{
	CurrentManifestPath = Path;
}

const FString& FGeneratorBase::GetManifestPath()
{
	return CurrentManifestPath;
}

bool FGeneratorBase::CheckExistsWithMetadata(
	const FString& AssetPath,
	const FString& AssetName,
	const FString& AssetType,
	uint64 InputHash,
	FGenerationResult& OutResult,
	EDryRunStatus* OutDryRunStatus)
{
	// First check if asset exists at all
	bool bExistsOnDisk = DoesAssetExistOnDisk(AssetPath);
	bool bExistsInMemory = IsAssetInMemory(AssetPath);

	// Asset doesn't exist - will create
	if (!bExistsOnDisk && !bExistsInMemory)
	{
		if (OutDryRunStatus)
		{
			*OutDryRunStatus = EDryRunStatus::WillCreate;
		}

		if (IsDryRunMode())
		{
			FDryRunResult Result(AssetName, EDryRunStatus::WillCreate, TEXT("New asset"));
			Result.AssetPath = AssetPath;
			Result.CurrentInputHash = InputHash;
			AddDryRunResult(Result);

			OutResult = FGenerationResult(AssetName, EGenerationStatus::Skipped,
				TEXT("[DRY RUN] Would create new asset"));
			OutResult.DetermineCategory();
			return true; // Skip in dry run mode
		}

		return false; // Proceed with generation
	}

	// Asset exists - check metadata
	UObject* ExistingAsset = nullptr;

	// Try to load the asset to check metadata
	FString FullAssetPath = AssetPath + TEXT(".") + FPackageName::GetShortName(AssetPath);
	ExistingAsset = StaticLoadObject(UObject::StaticClass(), nullptr, *FullAssetPath);

	if (!ExistingAsset)
	{
		// Can't load asset to check metadata - use legacy behavior (skip)
		if (OutDryRunStatus)
		{
			*OutDryRunStatus = EDryRunStatus::WillSkip;
		}

		OutResult = FGenerationResult(AssetName, EGenerationStatus::Skipped,
			FString::Printf(TEXT("[%s] %s already exists (couldn't verify metadata)"), *AssetType, *AssetName));
		OutResult.DetermineCategory();
		return true;
	}

	// Check for existing metadata
	UGeneratorAssetMetadata* Metadata = GetAssetMetadata(ExistingAsset);

	if (!Metadata)
	{
		// No metadata - asset exists but wasn't generated by us
		// Skip to avoid overwriting manual assets
		if (OutDryRunStatus)
		{
			*OutDryRunStatus = EDryRunStatus::WillSkip;
		}

		OutResult = FGenerationResult(AssetName, EGenerationStatus::Skipped,
			FString::Printf(TEXT("[%s] %s exists without generator metadata (likely manual)"), *AssetType, *AssetName));
		OutResult.DetermineCategory();
		return true;
	}

	// Has metadata - compare hashes
	bool bInputChanged = Metadata->HasInputChanged(InputHash);

	// Compute current output hash for manual edit detection
	uint64 CurrentOutputHash = 0;
	if (UBlueprint* Blueprint = Cast<UBlueprint>(ExistingAsset))
	{
		CurrentOutputHash = ComputeBlueprintOutputHash(Blueprint);
	}
	else
	{
		CurrentOutputHash = ComputeDataAssetOutputHash(ExistingAsset);
	}

	bool bOutputChanged = Metadata->HasOutputChanged(CurrentOutputHash);

	// Determine status
	EDryRunStatus Status;
	FString Reason;

	if (!bInputChanged && !bOutputChanged)
	{
		// Nothing changed - skip
		Status = EDryRunStatus::WillSkip;
		Reason = TEXT("No changes (hashes match)");
	}
	else if (bInputChanged && !bOutputChanged)
	{
		// Only manifest changed - safe to regenerate
		Status = EDryRunStatus::WillModify;
		Reason = TEXT("Manifest changed, no manual edits");
	}
	else if (bInputChanged && bOutputChanged)
	{
		// Both changed - conflict
		Status = EDryRunStatus::Conflicted;
		Reason = TEXT("CONFLICT: Both manifest AND asset changed");
	}
	else // !bInputChanged && bOutputChanged
	{
		// Only asset edited, manifest unchanged - skip to preserve edits
		Status = EDryRunStatus::WillSkip;
		Reason = TEXT("Asset was manually edited (manifest unchanged)");
	}

	if (OutDryRunStatus)
	{
		*OutDryRunStatus = Status;
	}

	// Handle based on status
	if (IsDryRunMode())
	{
		FDryRunResult Result(AssetName, Status, Reason);
		Result.AssetPath = AssetPath;
		Result.StoredInputHash = static_cast<uint64>(Metadata->InputHash);
		Result.CurrentInputHash = InputHash;
		Result.StoredOutputHash = static_cast<uint64>(Metadata->OutputHash);
		Result.CurrentOutputHash = CurrentOutputHash;
		AddDryRunResult(Result);

		OutResult = FGenerationResult(AssetName, EGenerationStatus::Skipped,
			FString::Printf(TEXT("[DRY RUN] %s"), *Reason));
		OutResult.DetermineCategory();
		return true; // Skip in dry run mode
	}

	// Actual generation mode
	switch (Status)
	{
	case EDryRunStatus::WillCreate:
		// Shouldn't happen here, but proceed
		return false;

	case EDryRunStatus::WillModify:
		// Safe to regenerate
		LogGeneration(FString::Printf(TEXT("[MODIFY] %s - %s"), *AssetName, *Reason));
		return false; // Proceed with generation

	case EDryRunStatus::WillSkip:
		OutResult = FGenerationResult(AssetName, EGenerationStatus::Skipped,
			FString::Printf(TEXT("[%s] %s - %s"), *AssetType, *AssetName, *Reason));
		OutResult.DetermineCategory();
		return true;

	case EDryRunStatus::Conflicted:
		if (IsForceMode())
		{
			LogGeneration(FString::Printf(TEXT("[FORCE] %s - Overwriting despite conflict"), *AssetName));
			return false; // Force regeneration
		}
		else
		{
			OutResult = FGenerationResult(AssetName, EGenerationStatus::Skipped,
				FString::Printf(TEXT("[CONFLICT] %s - %s. Use --force to override"), *AssetName, *Reason));
			OutResult.DetermineCategory();
			return true;
		}

	default:
		return false;
	}
}

// v2.6.2: Helper function to configure gameplay ability tags on CDO
void FGeneratorBase::ConfigureGameplayAbilityTags(
	UGameplayAbility* AbilityCDO,
	UBlueprint* Blueprint,
	const FManifestGameplayAbilityDefinition& Definition)
{
#if WITH_EDITOR
	if (!AbilityCDO || !Blueprint) return;

	bool bTagsModified = false;

	// Debug logging to trace tag parsing
	LogGeneration(FString::Printf(TEXT("  Parsed tags - AbilityTags: %d, CancelAbilitiesWithTag: %d, ActivationOwnedTags: %d, ActivationRequiredTags: %d, ActivationBlockedTags: %d"),
		Definition.Tags.AbilityTags.Num(),
		Definition.Tags.CancelAbilitiesWithTag.Num(),
		Definition.Tags.ActivationOwnedTags.Num(),
		Definition.Tags.ActivationRequiredTags.Num(),
		Definition.Tags.ActivationBlockedTags.Num()));

	// Set AbilityTags using the editor accessor
	if (Definition.Tags.AbilityTags.Num() > 0)
	{
		FGameplayTagContainer& AssetTags = AbilityCDO->EditorGetAssetTags();
		AssetTags.Reset(); // Clear existing tags first
		for (const FString& TagString : Definition.Tags.AbilityTags)
		{
			FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
			if (Tag.IsValid())
			{
				AssetTags.AddTag(Tag);
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("    Warning: Invalid tag '%s'"), *TagString));
			}
		}
		LogGeneration(FString::Printf(TEXT("  Configured %d AbilityTags"), Definition.Tags.AbilityTags.Num()));
		bTagsModified = true;
	}

	// Set CancelAbilitiesWithTag using reflection
	if (Definition.Tags.CancelAbilitiesWithTag.Num() > 0)
	{
		FStructProperty* CancelProperty = CastField<FStructProperty>(AbilityCDO->GetClass()->FindPropertyByName(TEXT("CancelAbilitiesWithTag")));
		if (CancelProperty)
		{
			FGameplayTagContainer* Container = CancelProperty->ContainerPtrToValuePtr<FGameplayTagContainer>(AbilityCDO);
			if (Container)
			{
				Container->Reset(); // Clear existing
				for (const FString& TagString : Definition.Tags.CancelAbilitiesWithTag)
				{
					FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
					if (Tag.IsValid())
					{
						Container->AddTag(Tag);
					}
				}
				LogGeneration(FString::Printf(TEXT("  Configured %d CancelAbilitiesWithTag"), Definition.Tags.CancelAbilitiesWithTag.Num()));
				bTagsModified = true;
			}
		}
	}

	// Set ActivationOwnedTags using reflection
	if (Definition.Tags.ActivationOwnedTags.Num() > 0)
	{
		FStructProperty* OwnedProperty = CastField<FStructProperty>(AbilityCDO->GetClass()->FindPropertyByName(TEXT("ActivationOwnedTags")));
		if (OwnedProperty)
		{
			FGameplayTagContainer* Container = OwnedProperty->ContainerPtrToValuePtr<FGameplayTagContainer>(AbilityCDO);
			if (Container)
			{
				Container->Reset(); // Clear existing
				for (const FString& TagString : Definition.Tags.ActivationOwnedTags)
				{
					FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
					if (Tag.IsValid())
					{
						Container->AddTag(Tag);
					}
				}
				LogGeneration(FString::Printf(TEXT("  Configured %d ActivationOwnedTags"), Definition.Tags.ActivationOwnedTags.Num()));
				bTagsModified = true;
			}
		}
	}

	// Set ActivationRequiredTags using reflection
	if (Definition.Tags.ActivationRequiredTags.Num() > 0)
	{
		FStructProperty* RequiredProperty = CastField<FStructProperty>(AbilityCDO->GetClass()->FindPropertyByName(TEXT("ActivationRequiredTags")));
		if (RequiredProperty)
		{
			FGameplayTagContainer* Container = RequiredProperty->ContainerPtrToValuePtr<FGameplayTagContainer>(AbilityCDO);
			if (Container)
			{
				Container->Reset(); // Clear existing
				for (const FString& TagString : Definition.Tags.ActivationRequiredTags)
				{
					FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
					if (Tag.IsValid())
					{
						Container->AddTag(Tag);
					}
				}
				LogGeneration(FString::Printf(TEXT("  Configured %d ActivationRequiredTags"), Definition.Tags.ActivationRequiredTags.Num()));
				bTagsModified = true;
			}
		}
	}

	// Set ActivationBlockedTags using reflection
	if (Definition.Tags.ActivationBlockedTags.Num() > 0)
	{
		FStructProperty* BlockedProperty = CastField<FStructProperty>(AbilityCDO->GetClass()->FindPropertyByName(TEXT("ActivationBlockedTags")));
		if (BlockedProperty)
		{
			FGameplayTagContainer* Container = BlockedProperty->ContainerPtrToValuePtr<FGameplayTagContainer>(AbilityCDO);
			if (Container)
			{
				Container->Reset(); // Clear existing
				for (const FString& TagString : Definition.Tags.ActivationBlockedTags)
				{
					FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
					if (Tag.IsValid())
					{
						Container->AddTag(Tag);
					}
				}
				LogGeneration(FString::Printf(TEXT("  Configured %d ActivationBlockedTags"), Definition.Tags.ActivationBlockedTags.Num()));
				bTagsModified = true;
			}
		}
	}

	// Save CDO changes without recompiling (avoids triggering event graph errors on existing assets)
	if (bTagsModified)
	{
		// Mark the CDO as modified so changes persist
		AbilityCDO->MarkPackageDirty();
		Blueprint->MarkPackageDirty();

		// Save package without recompile - tags are set directly on CDO
		Blueprint->GetPackage()->MarkPackageDirty();
		FString PackageFileName = FPackageName::LongPackageNameToFilename(Blueprint->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *PackageFileName, SaveArgs);

		LogGeneration(TEXT("  Tags saved (no recompile)"));
	}
#endif
}

// v2.6.2: Helper function to configure gameplay ability policies on CDO
void FGeneratorBase::ConfigureGameplayAbilityPolicies(
	UGameplayAbility* AbilityCDO,
	UBlueprint* Blueprint,
	const FManifestGameplayAbilityDefinition& Definition)
{
	if (!AbilityCDO || !Blueprint) return;

	bool bPoliciesModified = false;

	// Set InstancingPolicy via reflection
	if (!Definition.InstancingPolicy.IsEmpty())
	{
		FEnumProperty* InstancingEnumProperty = CastField<FEnumProperty>(AbilityCDO->GetClass()->FindPropertyByName(TEXT("InstancingPolicy")));
		if (InstancingEnumProperty)
		{
			UEnum* EnumClass = InstancingEnumProperty->GetEnum();
			if (EnumClass)
			{
				int64 EnumValue = EnumClass->GetValueByNameString(Definition.InstancingPolicy);
				if (EnumValue != INDEX_NONE)
				{
					void* ValuePtr = InstancingEnumProperty->ContainerPtrToValuePtr<void>(AbilityCDO);
					InstancingEnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, EnumValue);
					LogGeneration(FString::Printf(TEXT("  Set InstancingPolicy: %s"), *Definition.InstancingPolicy));
					bPoliciesModified = true;
				}
			}
		}
	}

	// Set NetExecutionPolicy via reflection
	if (!Definition.NetExecutionPolicy.IsEmpty())
	{
		FEnumProperty* NetExecProperty = CastField<FEnumProperty>(AbilityCDO->GetClass()->FindPropertyByName(TEXT("NetExecutionPolicy")));
		if (NetExecProperty)
		{
			UEnum* EnumClass = NetExecProperty->GetEnum();
			if (EnumClass)
			{
				int64 EnumValue = EnumClass->GetValueByNameString(Definition.NetExecutionPolicy);
				if (EnumValue != INDEX_NONE)
				{
					void* ValuePtr = NetExecProperty->ContainerPtrToValuePtr<void>(AbilityCDO);
					NetExecProperty->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, EnumValue);
					LogGeneration(FString::Printf(TEXT("  Set NetExecutionPolicy: %s"), *Definition.NetExecutionPolicy));
					bPoliciesModified = true;
				}
			}
		}
	}

	// Set CooldownGameplayEffectClass via reflection
	if (!Definition.CooldownGameplayEffectClass.IsEmpty())
	{
		UClass* EffectClass = nullptr;
		FString EffectPath = Definition.CooldownGameplayEffectClass;

		if (!EffectPath.Contains(TEXT("/")))
		{
			// v2.6.3: Search multiple common paths for cooldown effects
			TArray<FString> SearchPaths = {
				FString::Printf(TEXT("%s/Effects/Cooldowns/%s.%s_C"), *GCurrentProjectRoot, *Definition.CooldownGameplayEffectClass, *Definition.CooldownGameplayEffectClass),
				FString::Printf(TEXT("%s/Effects/%s.%s_C"), *GCurrentProjectRoot, *Definition.CooldownGameplayEffectClass, *Definition.CooldownGameplayEffectClass),
				FString::Printf(TEXT("%s/GameplayEffects/%s.%s_C"), *GCurrentProjectRoot, *Definition.CooldownGameplayEffectClass, *Definition.CooldownGameplayEffectClass),
				FString::Printf(TEXT("%s/GE/%s.%s_C"), *GCurrentProjectRoot, *Definition.CooldownGameplayEffectClass, *Definition.CooldownGameplayEffectClass)
			};

			for (const FString& Path : SearchPaths)
			{
				EffectClass = LoadClass<UGameplayEffect>(nullptr, *Path);
				if (EffectClass)
				{
					EffectPath = Path;
					break;
				}
			}
		}
		else
		{
			EffectClass = LoadClass<UGameplayEffect>(nullptr, *EffectPath);
		}

		if (EffectClass)
		{
			FClassProperty* CooldownProperty = CastField<FClassProperty>(AbilityCDO->GetClass()->FindPropertyByName(TEXT("CooldownGameplayEffectClass")));
			if (CooldownProperty)
			{
				CooldownProperty->SetPropertyValue_InContainer(AbilityCDO, EffectClass);
				LogGeneration(FString::Printf(TEXT("  Set CooldownGameplayEffectClass: %s"), *Definition.CooldownGameplayEffectClass));
				bPoliciesModified = true;
			}
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  Warning: CooldownGE not found: %s (searched multiple paths)"), *Definition.CooldownGameplayEffectClass));
		}
	}

	// Save CDO changes without recompiling (avoids triggering event graph errors on existing assets)
	if (bPoliciesModified)
	{
		AbilityCDO->MarkPackageDirty();
		Blueprint->MarkPackageDirty();
		Blueprint->GetPackage()->MarkPackageDirty();
		FString PackageFileName = FPackageName::LongPackageNameToFilename(Blueprint->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *PackageFileName, SaveArgs);

		LogGeneration(TEXT("  Policies saved (no recompile)"));
	}
}

FString FGeneratorBase::GetContentPath()
{
	return FPaths::ProjectContentDir();
}

UClass* FGeneratorBase::FindParentClass(const FString& ClassName)
{
	// v2.4.1: Enhanced class finding with Blueprint support
	// v2.6.11: Force scan Narrative Pro plugin content for commandlet mode

	// Static flag to ensure we only scan once per session
	static bool bNarrativeProScanned = false;
	if (!bNarrativeProScanned)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		// Scan NarrativePro plugin content paths to ensure Blueprint parent classes are discoverable
		TArray<FString> NarrativeProPaths;
		NarrativeProPaths.Add(TEXT("/NarrativePro/"));
		AssetRegistryModule.Get().ScanPathsSynchronous(NarrativeProPaths, true);
		bNarrativeProScanned = true;
		UE_LOG(LogTemp, Display, TEXT("[GasAbilityGenerator] Scanned NarrativePro content paths for parent class resolution"));
	}

	// First check if it's a Blueprint class name (starts with BP_, WBP_, etc.)
	bool bIsBlueprintClass = ClassName.StartsWith(TEXT("BP_")) ||
	                         ClassName.StartsWith(TEXT("WBP_")) ||
	                         ClassName.StartsWith(TEXT("GA_")) ||
	                         ClassName.StartsWith(TEXT("BT_"));

	// For Blueprint classes, search using Asset Registry
	if (bIsBlueprintClass)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AssetDataList;
		AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), AssetDataList);

		for (const FAssetData& AssetData : AssetDataList)
		{
			if (AssetData.AssetName.ToString() == ClassName)
			{
				UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
				if (Blueprint && Blueprint->GeneratedClass)
				{
					return Blueprint->GeneratedClass;
				}
			}
		}

		// Also try common Blueprint paths - load Blueprint asset and get GeneratedClass
		TArray<FString> BlueprintSearchPaths;
		FString Root = GetProjectRoot();  // v2.5.0: Use auto-detected project root
		BlueprintSearchPaths.Add(FString::Printf(TEXT("%s/Blueprints/%s.%s"), *Root, *ClassName, *ClassName));
		BlueprintSearchPaths.Add(FString::Printf(TEXT("%s/Characters/%s.%s"), *Root, *ClassName, *ClassName));
		BlueprintSearchPaths.Add(FString::Printf(TEXT("%s/Actors/%s.%s"), *Root, *ClassName, *ClassName));
		BlueprintSearchPaths.Add(FString::Printf(TEXT("/Game/%s.%s"), *ClassName, *ClassName));
		// v2.6.9: Add Narrative Pro paths for parent class lookup
		BlueprintSearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/Abilities/GameplayAbilities/Attacks/Melee/%s.%s"), *ClassName, *ClassName));
		BlueprintSearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/Abilities/GameplayAbilities/%s.%s"), *ClassName, *ClassName));
		BlueprintSearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/Abilities/%s.%s"), *ClassName, *ClassName));
		// v2.6.11: Add more NarrativePro paths for common parent classes
		BlueprintSearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/%s.%s"), *ClassName, *ClassName));
		BlueprintSearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/FollowCharacter/%s.%s"), *ClassName, *ClassName));

		for (const FString& Path : BlueprintSearchPaths)
		{
			// v2.6.11: Load Blueprint asset first, then get GeneratedClass
			UBlueprint* LoadedBP = LoadObject<UBlueprint>(nullptr, *Path);
			if (LoadedBP && LoadedBP->GeneratedClass)
			{
				UE_LOG(LogTemp, Display, TEXT("[GasAbilityGenerator] Found Blueprint parent class '%s' at path: %s"), *ClassName, *Path);
				return LoadedBP->GeneratedClass;
			}
		}
	}

	// Search order for native C++ classes per specification
	TArray<FString> ModuleSearchOrder = {
		TEXT("/Script/NarrativePro."),
		TEXT("/Script/NarrativeArsenal."),
		TEXT("/Script/Engine."),
		TEXT("/Script/UMG."),
		TEXT("/Script/GameplayAbilities."),
		TEXT("/Script/AIModule."),
		TEXT("/Script/EnhancedInput."),
		TEXT("/Script/CoreUObject."),
		TEXT("/Script/NavigationSystem."),
		TEXT("/Script/Kismet.")
	};

	// v2.4.2: Handle UE naming conventions - try with A/U prefixes
	TArray<FString> ClassNameVariants;
	ClassNameVariants.Add(ClassName);
	// Add A prefix for Actor classes
	ClassNameVariants.Add(TEXT("A") + ClassName);
	// Add U prefix for UObject classes
	ClassNameVariants.Add(TEXT("U") + ClassName);

	for (const FString& ModulePath : ModuleSearchOrder)
	{
		for (const FString& NameVariant : ClassNameVariants)
		{
			FString FullPath = ModulePath + NameVariant;
			UClass* FoundClass = FindObject<UClass>(nullptr, *FullPath);
			if (FoundClass)
			{
				return FoundClass;
			}
		}
	}

	// Fallback: try loading by class name directly (search all packages)
	UClass* DirectResult = FindObject<UClass>(nullptr, *ClassName);
	if (DirectResult)
	{
		return DirectResult;
	}

	// Final fallback: Search asset registry for any matching Blueprint
	if (!bIsBlueprintClass)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AssetDataList;
		AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), AssetDataList);

		for (const FAssetData& AssetData : AssetDataList)
		{
			if (AssetData.AssetName.ToString() == ClassName)
			{
				UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
				if (Blueprint && Blueprint->GeneratedClass)
				{
					return Blueprint->GeneratedClass;
				}
			}
		}
	}

	return nullptr;
}

// v2.1.8: Find user-defined enum by name
UEnum* FGeneratorBase::FindUserDefinedEnum(const FString& EnumName, const FString& ProjectRoot)
{
	// First try finding in all packages (for native enums)
	UEnum* FoundEnum = FindObject<UEnum>(nullptr, *EnumName);
	if (FoundEnum)
	{
		return FoundEnum;
	}
	
	// Build possible paths for user-defined enums
	TArray<FString> SearchPaths;
	
	// v2.5.0: Use GetProjectRoot() for auto-detection if ProjectRoot not provided
	FString Root = ProjectRoot.IsEmpty() ? GetProjectRoot() : ProjectRoot;
	
	// Common enum locations
	SearchPaths.Add(FString::Printf(TEXT("%s/Enums/%s.%s"), *Root, *EnumName, *EnumName));
	SearchPaths.Add(FString::Printf(TEXT("%s/Enumerations/%s.%s"), *Root, *EnumName, *EnumName));
	SearchPaths.Add(FString::Printf(TEXT("%s/Data/%s.%s"), *Root, *EnumName, *EnumName));
	SearchPaths.Add(FString::Printf(TEXT("%s/%s.%s"), *Root, *EnumName, *EnumName));
	SearchPaths.Add(FString::Printf(TEXT("/Game/%s.%s"), *EnumName, *EnumName));
	
	for (const FString& Path : SearchPaths)
	{
		UUserDefinedEnum* UserEnum = LoadObject<UUserDefinedEnum>(nullptr, *Path);
		if (UserEnum)
		{
			LogGeneration(FString::Printf(TEXT("Found user-defined enum: %s at %s"), *EnumName, *Path));
			return UserEnum;
		}
	}
	
	// Try asset registry search
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByClass(UUserDefinedEnum::StaticClass()->GetClassPathName(), AssetDataList);
	
	for (const FAssetData& AssetData : AssetDataList)
	{
		if (AssetData.AssetName.ToString() == EnumName)
		{
			UUserDefinedEnum* UserEnum = Cast<UUserDefinedEnum>(AssetData.GetAsset());
			if (UserEnum)
			{
				LogGeneration(FString::Printf(TEXT("Found user-defined enum via registry: %s"), *EnumName));
				return UserEnum;
			}
		}
	}
	
	return nullptr;
}

void FGeneratorBase::LogGeneration(const FString& Message)
{
	UE_LOG(LogTemp, Display, TEXT("[GasAbilityGenerator] %s"), *Message);
}

// ============================================================================
// v2.1.8: Helper function to convert type string to FEdGraphPinType
// Updated to use FindUserDefinedEnum for enum types
// ============================================================================

static FEdGraphPinType GetPinTypeFromString(const FString& TypeString)
{
	FEdGraphPinType PinType;
	PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean; // Default
	
	if (TypeString.Equals(TEXT("Bool"), ESearchCase::IgnoreCase) || 
	    TypeString.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	}
	else if (TypeString.Equals(TEXT("Int"), ESearchCase::IgnoreCase) || 
	         TypeString.Equals(TEXT("Integer"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
	}
	else if (TypeString.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
		PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
	}
	else if (TypeString.Equals(TEXT("Double"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
		PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
	}
	else if (TypeString.Equals(TEXT("String"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_String;
	}
	else if (TypeString.Equals(TEXT("Name"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Name;
	}
	else if (TypeString.Equals(TEXT("Text"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Text;
	}
	else if (TypeString.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
	}
	else if (TypeString.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
	}
	else if (TypeString.Equals(TEXT("Transform"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
	}
	else if (TypeString.Equals(TEXT("LinearColor"), ESearchCase::IgnoreCase) ||
	         TypeString.Equals(TEXT("Color"), ESearchCase::IgnoreCase))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
	}
	else if (TypeString.Equals(TEXT("TimerHandle"), ESearchCase::IgnoreCase))
	{
		// v2.4.0: TimerHandle support
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = FindObject<UScriptStruct>(nullptr, TEXT("/Script/Engine.TimerHandle"));
	}
	else if (TypeString.Equals(TEXT("ActiveGameplayEffectHandle"), ESearchCase::IgnoreCase))
	{
		// v2.4.0: GAS ActiveGameplayEffectHandle support
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = FindObject<UScriptStruct>(nullptr, TEXT("/Script/GameplayAbilities.ActiveGameplayEffectHandle"));
	}
	else
	{
		// v2.1.8: Check for user-defined enum first (E_ prefix)
		if (TypeString.StartsWith(TEXT("E_")))
		{
			UEnum* FoundEnum = FGeneratorBase::FindUserDefinedEnum(TypeString, GetProjectRoot());
			if (FoundEnum)
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
				PinType.PinSubCategoryObject = FoundEnum;
				return PinType;
			}
		}

		// v2.4.0: Handle "Object:ClassName" format for typed object references
		// v2.4.5: Added fallback to AActor if class not found
		if (TypeString.StartsWith(TEXT("Object:")) || TypeString.StartsWith(TEXT("object:")))
		{
			FString ClassName = TypeString.Mid(7); // Skip "Object:"
			UClass* FoundClass = FGeneratorBase::FindParentClass(ClassName);
			if (FoundClass)
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
				PinType.PinSubCategoryObject = FoundClass;
				return PinType;
			}
			else
			{
				// v2.4.5: Fallback to AActor if specific class not found (e.g., Blueprint not yet created)
				UE_LOG(LogTemp, Warning, TEXT("GetPinTypeFromString: Class '%s' not found, using AActor as fallback"), *ClassName);
				PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
				PinType.PinSubCategoryObject = AActor::StaticClass();
				return PinType;
			}
		}

		// v2.4.5: Handle plain "Object" type (no class specified) - default to AActor
		if (TypeString.Equals(TEXT("Object"), ESearchCase::IgnoreCase))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
			PinType.PinSubCategoryObject = AActor::StaticClass();
			return PinType;
		}

		// v2.4.0: Handle "Class:ClassName" format for class references
		if (TypeString.StartsWith(TEXT("Class:")) || TypeString.StartsWith(TEXT("class:")))
		{
			FString ClassName = TypeString.Mid(6); // Skip "Class:"
			UClass* FoundClass = FGeneratorBase::FindParentClass(ClassName);
			if (FoundClass)
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Class;
				PinType.PinSubCategoryObject = FoundClass;
				return PinType;
			}
		}

		// Try to find as object reference
		UClass* FoundClass = FGeneratorBase::FindParentClass(TypeString);
		if (FoundClass)
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
			PinType.PinSubCategoryObject = FoundClass;
		}
		else
		{
			// Try as native enum (search all packages)
			UEnum* FoundEnum = FindObject<UEnum>(nullptr, *TypeString);
			if (FoundEnum)
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
				PinType.PinSubCategoryObject = FoundEnum;
			}
		}
	}
	
	return PinType;
}

// ============================================================================
// v2.1.8: FEnumerationGenerator Implementation
// ============================================================================

FGenerationResult FEnumerationGenerator::Generate(const FManifestEnumerationDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Enumerations") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	// v2.1.9: Validate against manifest whitelist FIRST
	if (ValidateAgainstManifest(Definition.Name, TEXT("Enumeration"), Result))
	{
		return Result;
	}

	// v3.0: Check existence with metadata-aware logic
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Enumeration"), Definition.ComputeHash(), Result))
	{
		return Result;
	}

	// Validate we have values
	if (Definition.Values.Num() == 0)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("No enum values defined"));
	}
	
	// Create package
	FString PackagePath = AssetPath;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}
	
	// Create user-defined enum
	UUserDefinedEnum* NewEnum = NewObject<UUserDefinedEnum>(Package, *Definition.Name, RF_Public | RF_Standalone);
	if (!NewEnum)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create UserDefinedEnum"));
	}
	
	// Build enum values array
	TArray<TPair<FName, int64>> EnumValues;
	for (int32 i = 0; i < Definition.Values.Num(); i++)
	{
		// Format: EnumName::ValueName for namespaced enum
		FString QualifiedName = FString::Printf(TEXT("%s::%s"), *Definition.Name, *Definition.Values[i]);
		EnumValues.Add(TPair<FName, int64>(FName(*QualifiedName), i));
	}
	
	// Add MAX entry (standard for UE enums)
	FString MaxName = FString::Printf(TEXT("%s::%s_MAX"), *Definition.Name, *Definition.Name);
	EnumValues.Add(TPair<FName, int64>(FName(*MaxName), Definition.Values.Num()));
	
	// Apply the enum values - UE5.6 requires non-const reference
	NewEnum->SetEnums(EnumValues, UEnum::ECppForm::Namespaced, EEnumFlags::None, true);
	
	// Set display names for each value (using friendly names without namespace prefix)
	for (int32 i = 0; i < Definition.Values.Num(); i++)
	{
		FString DisplayName = Definition.Values[i];
		NewEnum->SetMetaData(TEXT("DisplayName"), *DisplayName, i);
	}
	
	// Mark dirty and register
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewEnum);
	
	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, NewEnum, *PackageFileName, SaveArgs);
	
	LogGeneration(FString::Printf(TEXT("Created Enumeration: %s (with %d values)"), *Definition.Name, Definition.Values.Num()));

	// v3.0: Store metadata for regeneration tracking
	StoreDataAssetMetadata(NewEnum, TEXT("E"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// FInputActionGenerator Implementation
// ============================================================================

FGenerationResult FInputActionGenerator::Generate(const FManifestInputActionDefinition& Definition)
{
	FString AssetPath = FString::Printf(TEXT("%s/Input/%s"), *GetProjectRoot(), *Definition.Name);
	FGenerationResult Result;

	// v2.1.9: Validate against manifest whitelist FIRST
	if (ValidateAgainstManifest(Definition.Name, TEXT("Input Action"), Result))
	{
		return Result;
	}

	// v3.0: Check existence with metadata-aware logic
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Input Action"), Definition.ComputeHash(), Result))
	{
		return Result;
	}

	// Create package
	FString PackagePath = AssetPath;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	// Create input action
	UInputAction* InputAction = NewObject<UInputAction>(Package, *Definition.Name, RF_Public | RF_Standalone);
	if (!InputAction)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create InputAction"));
	}
	
	// Configure value type
	if (Definition.ValueType == TEXT("Axis1D"))
	{
		InputAction->ValueType = EInputActionValueType::Axis1D;
	}
	else if (Definition.ValueType == TEXT("Axis2D"))
	{
		InputAction->ValueType = EInputActionValueType::Axis2D;
	}
	else if (Definition.ValueType == TEXT("Axis3D"))
	{
		InputAction->ValueType = EInputActionValueType::Axis3D;
	}
	else
	{
		InputAction->ValueType = EInputActionValueType::Boolean;
	}
	
	// Mark dirty and register
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(InputAction);
	
	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, InputAction, *PackageFileName, SaveArgs);
	
	LogGeneration(FString::Printf(TEXT("Created Input Action: %s"), *Definition.Name));

	// v3.0: Store metadata for regeneration tracking
	StoreDataAssetMetadata(InputAction, TEXT("IA"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// FInputMappingContextGenerator Implementation
// ============================================================================

FGenerationResult FInputMappingContextGenerator::Generate(const FManifestInputMappingContextDefinition& Definition)
{
	FString AssetPath = FString::Printf(TEXT("%s/Input/%s"), *GetProjectRoot(), *Definition.Name);
	FGenerationResult Result;

	// v2.1.9: Validate against manifest whitelist FIRST
	if (ValidateAgainstManifest(Definition.Name, TEXT("Input Mapping Context"), Result))
	{
		return Result;
	}

	// v3.0: Check existence with metadata-aware logic
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Input Mapping Context"), Definition.ComputeHash(), Result))
	{
		return Result;
	}

	// Create package
	FString PackagePath = AssetPath;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}
	
	// Create input mapping context
	UInputMappingContext* IMC = NewObject<UInputMappingContext>(Package, *Definition.Name, RF_Public | RF_Standalone);
	if (!IMC)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create InputMappingContext"));
	}
	
	// Note: Bindings would be configured here if input actions are already created
	// For now, create the empty structure
	
	// Mark dirty and register
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(IMC);
	
	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, IMC, *PackageFileName, SaveArgs);
	
	LogGeneration(FString::Printf(TEXT("Created Input Mapping Context: %s"), *Definition.Name));

	// v3.0: Store metadata for regeneration tracking
	StoreDataAssetMetadata(IMC, TEXT("IMC"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// FGameplayEffectGenerator Implementation
// ============================================================================

// v2.3.0: Comprehensive GE generator with all fields
FGenerationResult FGameplayEffectGenerator::Generate(const FManifestGameplayEffectDefinition& Definition)
{
	// v2.3.0: Support folder override
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Effects") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	// v2.1.9: Validate against manifest whitelist FIRST
	if (ValidateAgainstManifest(Definition.Name, TEXT("Gameplay Effect"), Result))
	{
		return Result;
	}

	// v3.0: Check existence with metadata-aware logic for MODIFY/CONFLICT detection
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Gameplay Effect"), Definition.ComputeHash(), Result))
	{
		return Result;
	}

	// Create package
	FString PackagePath = AssetPath;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	// v2.6.6 FIX: Create GE as Blueprint class instead of data asset
	// This allows GEs to be used as CooldownGameplayEffectClass (requires TSubclassOf<UGameplayEffect>)
	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = UGameplayEffect::StaticClass();

	UBlueprint* Blueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
		UBlueprint::StaticClass(),
		Package,
		FName(*Definition.Name),
		RF_Public | RF_Standalone,
		nullptr,
		GWarn
	));

	if (!Blueprint || !Blueprint->GeneratedClass)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Gameplay Effect Blueprint"));
	}

	// Get the CDO to configure properties
	UGameplayEffect* Effect = Cast<UGameplayEffect>(Blueprint->GeneratedClass->GetDefaultObject());
	if (!Effect)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to get Gameplay Effect CDO"));
	}

	// Configure duration policy
	if (Definition.DurationPolicy == TEXT("Instant"))
	{
		Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
	}
	else if (Definition.DurationPolicy == TEXT("HasDuration"))
	{
		Effect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	}
	else if (Definition.DurationPolicy == TEXT("Infinite"))
	{
		Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	}

	// v2.3.0: Set period for periodic effects
	if (Definition.Period > 0.0f)
	{
		Effect->Period = Definition.Period;
		Effect->bExecutePeriodicEffectOnApplication = Definition.bExecutePeriodicOnApplication;
	}

	// v2.3.0: Configure stacking
	// Note: UE5.7 deprecates direct StackingType access. SetStackingType() not available yet.
	// Using PRAGMA to suppress deprecation warning until Epic provides the setter.
	if (Definition.StackLimitCount > 0)
	{
		Effect->StackLimitCount = Definition.StackLimitCount;
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		if (Definition.StackingType == TEXT("AggregateBySource"))
		{
			Effect->StackingType = EGameplayEffectStackingType::AggregateBySource;
		}
		else if (Definition.StackingType == TEXT("AggregateByTarget"))
		{
			Effect->StackingType = EGameplayEffectStackingType::AggregateByTarget;
		}
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	// v2.3.0: Add modifiers
	// v2.6.3: Debug logging for modifiers and tags
	LogGeneration(FString::Printf(TEXT("  Parsed GE - Modifiers: %d, GrantedTags: %d, SetByCallerTags: %d"),
		Definition.Modifiers.Num(),
		Definition.GrantedTags.Num(),
		Definition.SetByCallerTags.Num()));

	if (Definition.Modifiers.Num() > 0)
	{
		for (const FManifestModifierDefinition& ModDef : Definition.Modifiers)
		{
			FGameplayModifierInfo ModInfo;

			// v2.3.0 FIX: Look up the actual attribute from "ClassName.PropertyName" format
			// Example: "NarrativeAttributeSetBase.Armor" -> UNarrativeAttributeSetBase::Armor
			FString ClassName, PropertyName;
			if (ModDef.Attribute.Split(TEXT("."), &ClassName, &PropertyName))
			{
				// Find the attribute set class using FindFirstObject (UE5.6 compatible)
				FString FullClassName = FString::Printf(TEXT("U%s"), *ClassName);
				UClass* AttributeSetClass = FindFirstObject<UClass>(*FullClassName, EFindFirstObjectOptions::EnsureIfAmbiguous);

				if (!AttributeSetClass)
				{
					// Try without the U prefix
					AttributeSetClass = FindFirstObject<UClass>(*ClassName, EFindFirstObjectOptions::EnsureIfAmbiguous);
				}

				if (AttributeSetClass)
				{
					// Find the property on the class
					FProperty* Property = AttributeSetClass->FindPropertyByName(FName(*PropertyName));
					if (Property)
					{
						// Create the FGameplayAttribute from the property
						ModInfo.Attribute = FGameplayAttribute(Property);
						LogGeneration(FString::Printf(TEXT("  Found attribute: %s.%s"), *ClassName, *PropertyName));
					}
					else
					{
						LogGeneration(FString::Printf(TEXT("  WARNING: Property '%s' not found on class '%s'"), *PropertyName, *ClassName));
					}
				}
				else
				{
					LogGeneration(FString::Printf(TEXT("  WARNING: Attribute set class '%s' not found"), *ClassName));
				}
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("  WARNING: Invalid attribute format '%s' (expected ClassName.PropertyName)"), *ModDef.Attribute));
			}

			// Set modifier operation
			if (ModDef.Operation == TEXT("Additive") || ModDef.Operation == TEXT("Add"))
			{
				ModInfo.ModifierOp = EGameplayModOp::Additive;
			}
			else if (ModDef.Operation == TEXT("Multiplicative") || ModDef.Operation == TEXT("Multiply"))
			{
				ModInfo.ModifierOp = EGameplayModOp::Multiplicitive;
			}
			else if (ModDef.Operation == TEXT("Division") || ModDef.Operation == TEXT("Divide"))
			{
				ModInfo.ModifierOp = EGameplayModOp::Division;
			}
			else if (ModDef.Operation == TEXT("Override"))
			{
				ModInfo.ModifierOp = EGameplayModOp::Override;
			}

			// v2.4.0: Set magnitude based on MagnitudeType
			if (ModDef.MagnitudeType.Equals(TEXT("SetByCaller"), ESearchCase::IgnoreCase))
			{
				// Use SetByCaller magnitude with gameplay tag
				FSetByCallerFloat SetByCallerMagnitude;
				if (!ModDef.SetByCallerTag.IsEmpty())
				{
					SetByCallerMagnitude.DataTag = FGameplayTag::RequestGameplayTag(FName(*ModDef.SetByCallerTag), false);
					if (!SetByCallerMagnitude.DataTag.IsValid())
					{
						LogGeneration(FString::Printf(TEXT("  WARNING: SetByCaller tag '%s' not found in GameplayTags"), *ModDef.SetByCallerTag));
					}
				}
				ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCallerMagnitude);

				Effect->Modifiers.Add(ModInfo);
				LogGeneration(FString::Printf(TEXT("  Added modifier: %s %s SetByCaller[%s]"),
					*ModDef.Attribute, *ModDef.Operation, *ModDef.SetByCallerTag));
			}
			else
			{
				// Default: Use ScalableFloat magnitude
				ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(ModDef.ScalableFloatValue));

				Effect->Modifiers.Add(ModInfo);
				LogGeneration(FString::Printf(TEXT("  Added modifier: %s %s %.2f"),
					*ModDef.Attribute, *ModDef.Operation, ModDef.ScalableFloatValue));
			}
		}
	}

	// v2.3.0: Add granted tags
	// Note: Using InheritableOwnedTagsContainer for direct GE objects as component-based tags
	// may not serialize properly. This is deprecated but still functional for tag granting.
	if (Definition.GrantedTags.Num() > 0)
	{
		for (const FString& TagName : Definition.GrantedTags)
		{
			FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagName), false);
			if (Tag.IsValid())
			{
				PRAGMA_DISABLE_DEPRECATION_WARNINGS
				Effect->InheritableOwnedTagsContainer.AddTag(Tag);
				PRAGMA_ENABLE_DEPRECATION_WARNINGS
				LogGeneration(FString::Printf(TEXT("  Added granted tag: %s"), *TagName));
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("  WARNING: Tag '%s' not found in GameplayTags"), *TagName));
			}
		}
	}

	// v2.6.6: Compile Blueprint before saving to ensure GeneratedClass is valid
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// Mark dirty and register
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Blueprint);

	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Gameplay Effect Blueprint: %s (Duration: %s, Modifiers: %d, Tags: %d)"),
		*Definition.Name, *Definition.DurationPolicy, Definition.Modifiers.Num(), Definition.GrantedTags.Num()));

	// v3.0: Store metadata for regeneration tracking
	StoreBlueprintMetadata(Blueprint, TEXT("GameplayEffect"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// FGameplayAbilityGenerator Implementation
// ============================================================================

// v2.3.0: Helper to convert string tag names to FGameplayTagContainer
static FGameplayTagContainer CreateTagContainer(const TArray<FString>& TagNames)
{
	FGameplayTagContainer Container;
	for (const FString& TagName : TagNames)
	{
		FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagName), false);
		if (Tag.IsValid())
		{
			Container.AddTag(Tag);
		}
		else
		{
			// If tag doesn't exist, log warning but continue
			UE_LOG(LogTemp, Warning, TEXT("[GasAbilityGenerator] Tag not found: %s - ensure it exists in DefaultGameplayTags.ini"), *TagName);
		}
	}
	return Container;
}

// v2.3.0: Updated to apply tags to the generated ability
// v2.4.0: Added ProjectRoot parameter, variables and inline event graph support
FGenerationResult FGameplayAbilityGenerator::Generate(
	const FManifestGameplayAbilityDefinition& Definition,
	const FString& ProjectRoot)
{
	// v2.4.0: Set global project root for enum lookups
	GCurrentProjectRoot = ProjectRoot;

	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Abilities") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	// v2.1.9: Validate against manifest whitelist FIRST
	if (ValidateAgainstManifest(Definition.Name, TEXT("Gameplay Ability"), Result))
	{
		return Result;
	}

	// v3.0: Check existence with metadata-aware logic for MODIFY/CONFLICT detection
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Gameplay Ability"), Definition.ComputeHash(), Result))
	{
		// v2.6.2: Even when skipping, update tags on existing GA blueprints
		UBlueprint* ExistingBlueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
		if (ExistingBlueprint && ExistingBlueprint->GeneratedClass)
		{
			UGameplayAbility* AbilityCDO = Cast<UGameplayAbility>(ExistingBlueprint->GeneratedClass->GetDefaultObject());
			if (AbilityCDO)
			{
				ConfigureGameplayAbilityTags(AbilityCDO, ExistingBlueprint, Definition);
				ConfigureGameplayAbilityPolicies(AbilityCDO, ExistingBlueprint, Definition);
			}
		}
		return Result;
	}

	// Find parent class
	UClass* ParentClass = FindParentClass(Definition.ParentClass);
	if (!ParentClass)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Parent class not found: %s"), *Definition.ParentClass));
	}

	// Create blueprint factory
	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = ParentClass;

	// Create package
	FString PackagePath = AssetPath;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	// Create blueprint
	UBlueprint* Blueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
		UBlueprint::StaticClass(),
		Package,
		*Definition.Name,
		RF_Public | RF_Standalone,
		nullptr,
		GWarn
	));

	if (!Blueprint)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Gameplay Ability blueprint"));
	}

	// v2.4.0: Add variables from definition BEFORE first compile
	// v2.6.3: Debug logging to trace variable parsing
	LogGeneration(FString::Printf(TEXT("  Parsed variables: %d, EventGraph: %s (%d nodes)"),
		Definition.Variables.Num(),
		Definition.bHasInlineEventGraph ? TEXT("Yes") : TEXT("No"),
		Definition.EventGraphNodes.Num()));

	for (const FManifestActorVariableDefinition& VarDef : Definition.Variables)
	{
		FEdGraphPinType PinType = GetPinTypeFromString(VarDef.Type);

		bool bSuccess = FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VarDef.Name), PinType);

		if (bSuccess)
		{
			int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, FName(*VarDef.Name));
			if (VarIndex != INDEX_NONE)
			{
				if (VarDef.bInstanceEditable)
				{
					Blueprint->NewVariables[VarIndex].PropertyFlags |= CPF_Edit;
				}

				if (VarDef.bReplicated)
				{
					Blueprint->NewVariables[VarIndex].PropertyFlags |= CPF_Net;
				}

				if (!VarDef.DefaultValue.IsEmpty())
				{
					Blueprint->NewVariables[VarIndex].DefaultValue = VarDef.DefaultValue;
				}
			}

			LogGeneration(FString::Printf(TEXT("  Added variable: %s (%s)"), *VarDef.Name, *VarDef.Type));
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  FAILED to add variable: %s (%s)"), *VarDef.Name, *VarDef.Type));
		}
	}

	// Compile blueprint first (after variables are added)
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// v2.4.0: Generate inline event graph if defined
	if (Definition.bHasInlineEventGraph && Definition.EventGraphNodes.Num() > 0)
	{
		// v2.6.7: Clear any previous missing dependencies before generation
		FEventGraphGenerator::ClearMissingDependencies();

		// Build a temporary graph definition from the stored arrays
		FManifestEventGraphDefinition GraphDef;
		GraphDef.Name = Definition.EventGraphName;
		GraphDef.Nodes = Definition.EventGraphNodes;
		GraphDef.Connections = Definition.EventGraphConnections;

		if (FEventGraphGenerator::GenerateEventGraph(Blueprint, GraphDef, ProjectRoot))
		{
			LogGeneration(FString::Printf(TEXT("  Generated event graph with %d nodes"), Definition.EventGraphNodes.Num()));
		}
		else
		{
			LogGeneration(TEXT("  Warning: Failed to generate event graph"));
		}

		// v2.6.7: Check for missing dependencies and return Deferred status if any
		if (FEventGraphGenerator::HasMissingDependencies())
		{
			const TArray<FMissingDependencyInfo>& MissingDeps = FEventGraphGenerator::GetMissingDependencies();
			const FMissingDependencyInfo& FirstMissing = MissingDeps[0];

			LogGeneration(FString::Printf(TEXT("  Deferring due to missing dependency: %s (%s) - %s"),
				*FirstMissing.DependencyName, *FirstMissing.DependencyType, *FirstMissing.Context));

			// Don't save incomplete asset - return Deferred so we can retry after dependency is created
			Result = FGenerationResult(Definition.Name, EGenerationStatus::Deferred,
				FString::Printf(TEXT("Missing dependency: %s"), *FirstMissing.DependencyName));
			Result.MissingDependency = FirstMissing.DependencyName;
			Result.MissingDependencyType = FirstMissing.DependencyType;
			Result.DetermineCategory();

			FEventGraphGenerator::ClearMissingDependencies();
			return Result;
		}
	}

	// v2.6.2: Configure tags and policies using helper functions
	if (Blueprint->GeneratedClass)
	{
		UGameplayAbility* AbilityCDO = Cast<UGameplayAbility>(Blueprint->GeneratedClass->GetDefaultObject());
		if (AbilityCDO)
		{
			ConfigureGameplayAbilityTags(AbilityCDO, Blueprint, Definition);
			ConfigureGameplayAbilityPolicies(AbilityCDO, Blueprint, Definition);
		}
	}

	// Mark dirty and register
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Blueprint);

	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Gameplay Ability: %s (with %d variables)"), *Definition.Name, Definition.Variables.Num()));

	// v3.0: Store metadata for regeneration tracking
	StoreBlueprintMetadata(Blueprint, TEXT("GameplayAbility"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// FActorBlueprintGenerator Implementation
// v2.1.8: Added ProjectRoot parameter for enum lookups
// v2.1.6: Added variable creation
// ============================================================================

FGenerationResult FActorBlueprintGenerator::Generate(
	const FManifestActorBlueprintDefinition& Definition,
	const FString& ProjectRoot,
	const FManifestData* ManifestData)
{
	// v2.1.8: Set global project root for enum lookups
	GCurrentProjectRoot = ProjectRoot;

	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Actors") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	// v2.1.9: Validate against manifest whitelist FIRST
	if (ValidateAgainstManifest(Definition.Name, TEXT("Actor Blueprint"), Result))
	{
		return Result;
	}

	// v2.7.7: Pre-generation validation for event graphs
	TArray<FString> ValidationErrors;
	if (!FEventGraphGenerator::ValidateActorBlueprintEventGraph(Definition, ValidationErrors))
	{
		for (const FString& Error : ValidationErrors)
		{
			UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] PRE-GEN VALIDATION: %s"), *Error);
			LogGeneration(FString::Printf(TEXT("  PRE-GEN ERROR: %s"), *Error));
		}
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Pre-generation validation failed: %d errors"), ValidationErrors.Num()));
	}
	// Log warnings (validation returned true but may have warnings)
	for (const FString& Warning : ValidationErrors)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GasAbilityGenerator] PRE-GEN WARNING: %s"), *Warning);
		LogGeneration(FString::Printf(TEXT("  PRE-GEN WARNING: %s"), *Warning));
	}

	// v3.0: Check existence with metadata-aware logic for MODIFY/CONFLICT detection
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Actor Blueprint"), Definition.ComputeHash(), Result))
	{
		return Result;
	}

	// Find parent class
	UClass* ParentClass = FindParentClass(Definition.ParentClass);
	if (!ParentClass)
	{
		ParentClass = AActor::StaticClass();
	}

	// Create blueprint factory
	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = ParentClass;

	// Create package
	FString PackagePath = AssetPath;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	// Create blueprint
	UBlueprint* Blueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
		UBlueprint::StaticClass(),
		Package,
		*Definition.Name,
		RF_Public | RF_Standalone,
		nullptr,
		GWarn
	));

	if (!Blueprint)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Actor blueprint"));
	}

	// v2.1.6 FIX: Add variables from definition
	for (const FManifestActorVariableDefinition& VarDef : Definition.Variables)
	{
		FEdGraphPinType PinType = GetPinTypeFromString(VarDef.Type);

		bool bSuccess = FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VarDef.Name), PinType);

		if (bSuccess)
		{
			int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, FName(*VarDef.Name));
			if (VarIndex != INDEX_NONE)
			{
				if (VarDef.bInstanceEditable)
				{
					Blueprint->NewVariables[VarIndex].PropertyFlags |= CPF_Edit;
				}

				if (VarDef.bReplicated)
				{
					Blueprint->NewVariables[VarIndex].PropertyFlags |= CPF_Net;
				}

				if (!VarDef.DefaultValue.IsEmpty())
				{
					Blueprint->NewVariables[VarIndex].DefaultValue = VarDef.DefaultValue;
				}
			}

			LogGeneration(FString::Printf(TEXT("  Added variable: %s (%s)"), *VarDef.Name, *VarDef.Type));
		}
	}

	// v2.3.0: Add components from definition
	for (const FManifestActorComponentDefinition& CompDef : Definition.Components)
	{
		// Find component class
		UClass* ComponentClass = nullptr;

		// Check common component types
		if (CompDef.Type == TEXT("SceneComponent"))
		{
			ComponentClass = USceneComponent::StaticClass();
		}
		else if (CompDef.Type == TEXT("StaticMeshComponent"))
		{
			ComponentClass = UStaticMeshComponent::StaticClass();
		}
		else if (CompDef.Type == TEXT("SkeletalMeshComponent"))
		{
			ComponentClass = USkeletalMeshComponent::StaticClass();
		}
		else if (CompDef.Type == TEXT("CapsuleComponent"))
		{
			ComponentClass = UCapsuleComponent::StaticClass();
		}
		else if (CompDef.Type == TEXT("SphereComponent"))
		{
			ComponentClass = USphereComponent::StaticClass();
		}
		else if (CompDef.Type == TEXT("BoxComponent"))
		{
			ComponentClass = UBoxComponent::StaticClass();
		}
		else if (CompDef.Type == TEXT("AudioComponent"))
		{
			ComponentClass = UAudioComponent::StaticClass();
		}
		else if (CompDef.Type == TEXT("PointLightComponent"))
		{
			ComponentClass = UPointLightComponent::StaticClass();
		}
		else if (CompDef.Type == TEXT("SpotLightComponent"))
		{
			ComponentClass = USpotLightComponent::StaticClass();
		}
		else if (CompDef.Type == TEXT("ParticleSystemComponent"))
		{
			ComponentClass = UParticleSystemComponent::StaticClass();
		}
		else if (CompDef.Type == TEXT("NiagaraComponent"))
		{
			ComponentClass = FindObject<UClass>(nullptr, TEXT("/Script/Niagara.NiagaraComponent"));
		}
		else if (CompDef.Type == TEXT("WidgetComponent"))
		{
			ComponentClass = FindObject<UClass>(nullptr, TEXT("/Script/UMG.WidgetComponent"));
		}
		else
		{
			// Try to find custom component class
			ComponentClass = FindParentClass(CompDef.Type);
		}

		if (ComponentClass)
		{
			USCS_Node* NewNode = Blueprint->SimpleConstructionScript->CreateNode(ComponentClass, FName(*CompDef.Name));
			if (NewNode)
			{
				Blueprint->SimpleConstructionScript->AddNode(NewNode);
				LogGeneration(FString::Printf(TEXT("  Added component: %s (%s)"), *CompDef.Name, *CompDef.Type));
			}
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  Warning: Component class not found: %s"), *CompDef.Type));
		}
	}

	// v2.7.6: Generate inline event graph if defined (takes priority over referenced)
	if (Definition.bHasInlineEventGraph && Definition.EventGraphNodes.Num() > 0)
	{
		FEventGraphGenerator::ClearMissingDependencies();

		// Build a temporary graph definition from the stored arrays
		FManifestEventGraphDefinition GraphDef;
		GraphDef.Name = Definition.EventGraphName;
		GraphDef.Nodes = Definition.EventGraphNodes;
		GraphDef.Connections = Definition.EventGraphConnections;

		if (FEventGraphGenerator::GenerateEventGraph(Blueprint, GraphDef, ProjectRoot))
		{
			LogGeneration(FString::Printf(TEXT("  Generated inline event graph with %d nodes"), Definition.EventGraphNodes.Num()));
		}
		else
		{
			LogGeneration(TEXT("  Warning: Failed to generate inline event graph"));
		}

		if (FEventGraphGenerator::HasMissingDependencies())
		{
			const TArray<FMissingDependencyInfo>& MissingDeps = FEventGraphGenerator::GetMissingDependencies();
			const FMissingDependencyInfo& FirstMissing = MissingDeps[0];

			LogGeneration(FString::Printf(TEXT("  Deferring due to missing dependency: %s (%s) - %s"),
				*FirstMissing.DependencyName, *FirstMissing.DependencyType, *FirstMissing.Context));

			Result = FGenerationResult(Definition.Name, EGenerationStatus::Deferred,
				FString::Printf(TEXT("Missing dependency: %s"), *FirstMissing.DependencyName));
			Result.MissingDependency = FirstMissing.DependencyName;
			Result.MissingDependencyType = FirstMissing.DependencyType;
			Result.DetermineCategory();

			FEventGraphGenerator::ClearMissingDependencies();
			return Result;
		}
	}
	// v2.2.0: Fall back to referenced event graph lookup
	else if (!Definition.EventGraphName.IsEmpty() && ManifestData)
	{
		FEventGraphGenerator::ClearMissingDependencies();

		const FManifestEventGraphDefinition* GraphDef = ManifestData->FindEventGraphByName(Definition.EventGraphName);
		if (GraphDef)
		{
			if (FEventGraphGenerator::GenerateEventGraph(Blueprint, *GraphDef, ProjectRoot))
			{
				LogGeneration(FString::Printf(TEXT("  Applied event graph: %s"), *Definition.EventGraphName));
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("  Failed to apply event graph: %s"), *Definition.EventGraphName));
			}

			if (FEventGraphGenerator::HasMissingDependencies())
			{
				const TArray<FMissingDependencyInfo>& MissingDeps = FEventGraphGenerator::GetMissingDependencies();
				const FMissingDependencyInfo& FirstMissing = MissingDeps[0];

				LogGeneration(FString::Printf(TEXT("  Deferring due to missing dependency: %s (%s) - %s"),
					*FirstMissing.DependencyName, *FirstMissing.DependencyType, *FirstMissing.Context));

				Result = FGenerationResult(Definition.Name, EGenerationStatus::Deferred,
					FString::Printf(TEXT("Missing dependency: %s"), *FirstMissing.DependencyName));
				Result.MissingDependency = FirstMissing.DependencyName;
				Result.MissingDependencyType = FirstMissing.DependencyType;
				Result.DetermineCategory();

				FEventGraphGenerator::ClearMissingDependencies();
				return Result;
			}
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  Event graph not found: %s"), *Definition.EventGraphName));
		}
	}

	// v2.8.3: Generate function overrides if defined
	if (Definition.FunctionOverrides.Num() > 0)
	{
		LogGeneration(FString::Printf(TEXT("  Generating %d function override(s)"), Definition.FunctionOverrides.Num()));

		for (const FManifestFunctionOverrideDefinition& OverrideDef : Definition.FunctionOverrides)
		{
			if (FEventGraphGenerator::GenerateFunctionOverride(Blueprint, OverrideDef, ProjectRoot))
			{
				LogGeneration(FString::Printf(TEXT("    Generated function override: %s"), *OverrideDef.FunctionName));
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("    WARNING: Failed to generate function override: %s"), *OverrideDef.FunctionName));
			}
		}
	}

	// Compile blueprint
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// Mark dirty and register
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Blueprint);

	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Actor Blueprint: %s (with %d variables)"), *Definition.Name, Definition.Variables.Num()));

	// v3.0: Store metadata for regeneration tracking
	StoreBlueprintMetadata(Blueprint, TEXT("ActorBlueprint"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// FWidgetBlueprintGenerator Implementation
// v2.2.0: Added event graph generation support
// v2.1.6: Added variable creation
// ============================================================================

FGenerationResult FWidgetBlueprintGenerator::Generate(
	const FManifestWidgetBlueprintDefinition& Definition,
	const FManifestData* ManifestData)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Widgets") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	// v2.1.9: Validate against manifest whitelist FIRST
	if (ValidateAgainstManifest(Definition.Name, TEXT("Widget Blueprint"), Result))
	{
		return Result;
	}

	// v3.0: Check existence with metadata-aware logic for MODIFY/CONFLICT detection
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Widget Blueprint"), Definition.ComputeHash(), Result))
	{
		return Result;
	}

	// v2.1.2: Full widget blueprint creation implementation
	FString PackagePath = AssetPath;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	// Find parent class - search order per specification
	UClass* ParentClass = FindParentClass(Definition.ParentClass);
	if (!ParentClass)
	{
		// Fallback to UUserWidget
		ParentClass = UUserWidget::StaticClass();
	}

	// Create widget blueprint using factory
	UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
	Factory->ParentClass = ParentClass;

	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(
		Factory->FactoryCreateNew(
			UWidgetBlueprint::StaticClass(),
			Package,
			*Definition.Name,
			RF_Public | RF_Standalone,
			nullptr,
			GWarn
		)
	);

	if (!WidgetBP)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Widget Blueprint"));
	}

	// v2.1.6 FIX: Add variables from definition
	for (const FManifestWidgetVariableDefinition& VarDef : Definition.Variables)
	{
		FEdGraphPinType PinType = GetPinTypeFromString(VarDef.Type);

		bool bSuccess = FBlueprintEditorUtils::AddMemberVariable(WidgetBP, FName(*VarDef.Name), PinType);

		if (bSuccess)
		{
			int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(WidgetBP, FName(*VarDef.Name));
			if (VarIndex != INDEX_NONE)
			{
				if (VarDef.bInstanceEditable)
				{
					WidgetBP->NewVariables[VarIndex].PropertyFlags |= CPF_Edit;
				}

				if (VarDef.bExposeOnSpawn)
				{
					WidgetBP->NewVariables[VarIndex].PropertyFlags |= CPF_ExposeOnSpawn;
				}

				if (!VarDef.DefaultValue.IsEmpty())
				{
					WidgetBP->NewVariables[VarIndex].DefaultValue = VarDef.DefaultValue;
				}
			}

			LogGeneration(FString::Printf(TEXT("  Added variable: %s (%s)"), *VarDef.Name, *VarDef.Type));
		}
	}

	// v2.2.0: Generate event graph if specified
	if (!Definition.EventGraphName.IsEmpty() && ManifestData)
	{
		// v2.6.7: Clear any previous missing dependencies before generation
		FEventGraphGenerator::ClearMissingDependencies();

		const FManifestEventGraphDefinition* GraphDef = ManifestData->FindEventGraphByName(Definition.EventGraphName);
		if (GraphDef)
		{
			if (FEventGraphGenerator::GenerateEventGraph(WidgetBP, *GraphDef, TEXT("")))
			{
				LogGeneration(FString::Printf(TEXT("  Applied event graph: %s"), *Definition.EventGraphName));
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("  Failed to apply event graph: %s"), *Definition.EventGraphName));
			}

			// v2.6.7: Check for missing dependencies and return Deferred status if any
			if (FEventGraphGenerator::HasMissingDependencies())
			{
				const TArray<FMissingDependencyInfo>& MissingDeps = FEventGraphGenerator::GetMissingDependencies();
				const FMissingDependencyInfo& FirstMissing = MissingDeps[0];

				LogGeneration(FString::Printf(TEXT("  Deferring due to missing dependency: %s (%s) - %s"),
					*FirstMissing.DependencyName, *FirstMissing.DependencyType, *FirstMissing.Context));

				// Don't save incomplete asset - return Deferred so we can retry after dependency is created
				Result = FGenerationResult(Definition.Name, EGenerationStatus::Deferred,
					FString::Printf(TEXT("Missing dependency: %s"), *FirstMissing.DependencyName));
				Result.MissingDependency = FirstMissing.DependencyName;
				Result.MissingDependencyType = FirstMissing.DependencyType;
				Result.DetermineCategory();

				FEventGraphGenerator::ClearMissingDependencies();
				return Result;
			}
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  Event graph not found: %s"), *Definition.EventGraphName));
		}
	}

	// Mark dirty and register
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(WidgetBP);

	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, WidgetBP, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Widget Blueprint: %s (with %d variables)"), *Definition.Name, Definition.Variables.Num()));

	// v3.0: Store metadata for regeneration tracking
	StoreBlueprintMetadata(WidgetBP, TEXT("WidgetBlueprint"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// FBlackboardGenerator Implementation
// ============================================================================

FGenerationResult FBlackboardGenerator::Generate(const FManifestBlackboardDefinition& Definition)
{
	FString AssetPath = FString::Printf(TEXT("%s/AI/%s"), *GetProjectRoot(), *Definition.Name);
	FGenerationResult Result;

	// v2.1.9: Validate against manifest whitelist FIRST
	if (ValidateAgainstManifest(Definition.Name, TEXT("Blackboard"), Result))
	{
		return Result;
	}

	// v3.0: Check existence with metadata-aware logic
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Blackboard"), Definition.ComputeHash(), Result))
	{
		return Result;
	}

	// Create package
	FString PackagePath = AssetPath;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}
	
	// Create blackboard
	UBlackboardData* Blackboard = NewObject<UBlackboardData>(Package, *Definition.Name, RF_Public | RF_Standalone);
	if (!Blackboard)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Blackboard"));
	}

	// v2.6.0: Add keys from manifest definition
	int32 KeysCreated = 0;
	for (const FManifestBlackboardKeyDefinition& KeyDef : Definition.Keys)
	{
		FBlackboardEntry Entry;
		Entry.EntryName = FName(*KeyDef.Name);
		Entry.bInstanceSynced = KeyDef.bInstanceSynced;

		// Create appropriate key type based on Type string
		if (KeyDef.Type.Equals(TEXT("Bool"), ESearchCase::IgnoreCase) ||
		    KeyDef.Type.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
		{
			Entry.KeyType = NewObject<UBlackboardKeyType_Bool>(Blackboard);
		}
		else if (KeyDef.Type.Equals(TEXT("Int"), ESearchCase::IgnoreCase) ||
		         KeyDef.Type.Equals(TEXT("Integer"), ESearchCase::IgnoreCase))
		{
			Entry.KeyType = NewObject<UBlackboardKeyType_Int>(Blackboard);
		}
		else if (KeyDef.Type.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
		{
			Entry.KeyType = NewObject<UBlackboardKeyType_Float>(Blackboard);
		}
		else if (KeyDef.Type.Equals(TEXT("String"), ESearchCase::IgnoreCase))
		{
			Entry.KeyType = NewObject<UBlackboardKeyType_String>(Blackboard);
		}
		else if (KeyDef.Type.Equals(TEXT("Name"), ESearchCase::IgnoreCase))
		{
			Entry.KeyType = NewObject<UBlackboardKeyType_Name>(Blackboard);
		}
		else if (KeyDef.Type.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
		{
			Entry.KeyType = NewObject<UBlackboardKeyType_Vector>(Blackboard);
		}
		else if (KeyDef.Type.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase))
		{
			Entry.KeyType = NewObject<UBlackboardKeyType_Rotator>(Blackboard);
		}
		else if (KeyDef.Type.Equals(TEXT("Object"), ESearchCase::IgnoreCase) ||
		         KeyDef.Type.Equals(TEXT("Actor"), ESearchCase::IgnoreCase))
		{
			UBlackboardKeyType_Object* ObjectKey = NewObject<UBlackboardKeyType_Object>(Blackboard);
			// Default to AActor base class for Object keys
			ObjectKey->BaseClass = AActor::StaticClass();
			Entry.KeyType = ObjectKey;
		}
		else if (KeyDef.Type.Equals(TEXT("Class"), ESearchCase::IgnoreCase))
		{
			UBlackboardKeyType_Class* ClassKey = NewObject<UBlackboardKeyType_Class>(Blackboard);
			ClassKey->BaseClass = UObject::StaticClass();
			Entry.KeyType = ClassKey;
		}
		else if (KeyDef.Type.Equals(TEXT("Enum"), ESearchCase::IgnoreCase))
		{
			Entry.KeyType = NewObject<UBlackboardKeyType_Enum>(Blackboard);
		}
		else
		{
			// Default to Object type if unknown
			LogGeneration(FString::Printf(TEXT("  Unknown key type '%s' for key '%s', defaulting to Object"),
				*KeyDef.Type, *KeyDef.Name));
			Entry.KeyType = NewObject<UBlackboardKeyType_Object>(Blackboard);
		}

		Blackboard->Keys.Add(Entry);
		KeysCreated++;
		LogGeneration(FString::Printf(TEXT("  Added key: %s (%s)%s"),
			*KeyDef.Name, *KeyDef.Type, KeyDef.bInstanceSynced ? TEXT(" [Synced]") : TEXT("")));
	}

	// Mark dirty and register
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Blackboard);

	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blackboard, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Blackboard: %s with %d keys"), *Definition.Name, KeysCreated));

	// v3.0: Store metadata for regeneration tracking
	StoreDataAssetMetadata(Blackboard, TEXT("BB"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// FBehaviorTreeGenerator Implementation
// v3.1: Enhanced with root composite and node tree generation
// ============================================================================

FGenerationResult FBehaviorTreeGenerator::Generate(const FManifestBehaviorTreeDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("AI") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	// v2.1.9: Validate against manifest whitelist FIRST
	if (ValidateAgainstManifest(Definition.Name, TEXT("Behavior Tree"), Result))
	{
		return Result;
	}

	// v3.0: Check existence with metadata-aware logic
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Behavior Tree"), Definition.ComputeHash(), Result))
	{
		return Result;
	}

	// Create package
	FString PackagePath = AssetPath;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	// Create behavior tree
	UBehaviorTree* BT = NewObject<UBehaviorTree>(Package, *Definition.Name, RF_Public | RF_Standalone);
	if (!BT)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create BehaviorTree"));
	}

	// Link blackboard if specified
	if (!Definition.BlackboardAsset.IsEmpty())
	{
		// Try multiple paths for blackboard
		TArray<FString> BBPaths;
		BBPaths.Add(FString::Printf(TEXT("%s/AI/%s.%s"), *GetProjectRoot(), *Definition.BlackboardAsset, *Definition.BlackboardAsset));
		BBPaths.Add(FString::Printf(TEXT("/Game/AI/%s.%s"), *Definition.BlackboardAsset, *Definition.BlackboardAsset));
		BBPaths.Add(FString::Printf(TEXT("%s/Blackboards/%s.%s"), *GetProjectRoot(), *Definition.BlackboardAsset, *Definition.BlackboardAsset));

		UBlackboardData* BB = nullptr;
		for (const FString& BBPath : BBPaths)
		{
			BB = LoadObject<UBlackboardData>(nullptr, *BBPath);
			if (BB)
			{
				break;
			}
		}

		if (BB)
		{
			BT->BlackboardAsset = BB;
			LogGeneration(FString::Printf(TEXT("  Linked Blackboard: %s"), *Definition.BlackboardAsset));
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  [WARNING] Blackboard not found: %s"), *Definition.BlackboardAsset));
		}
	}

	// v3.1: Create root composite node if nodes are defined
	if (Definition.Nodes.Num() > 0 || !Definition.RootType.IsEmpty())
	{
		// Determine root composite type (default: Selector)
		FString RootType = Definition.RootType.IsEmpty() ? TEXT("Selector") : Definition.RootType;

		UBTCompositeNode* RootNode = nullptr;
		if (RootType.Equals(TEXT("Sequence"), ESearchCase::IgnoreCase))
		{
			RootNode = NewObject<UBTComposite_Sequence>(BT);
			LogGeneration(TEXT("  Created root Sequence node"));
		}
		else
		{
			RootNode = NewObject<UBTComposite_Selector>(BT);
			LogGeneration(TEXT("  Created root Selector node"));
		}

		if (RootNode)
		{
			BT->RootNode = RootNode;

			// v3.1: Build node map for child reference resolution
			TMap<FString, int32> NodeIndexMap;
			for (int32 i = 0; i < Definition.Nodes.Num(); i++)
			{
				NodeIndexMap.Add(Definition.Nodes[i].Id, i);
			}

			// v3.1: Process nodes and build tree structure
			int32 NodesCreated = 0;
			for (const FManifestBTNodeDefinition& NodeDef : Definition.Nodes)
			{
				// Create child entry for root
				FBTCompositeChild ChildEntry;

				// Determine if this is a composite or task node
				bool bIsComposite = NodeDef.Type.Equals(TEXT("Selector"), ESearchCase::IgnoreCase) ||
					NodeDef.Type.Equals(TEXT("Sequence"), ESearchCase::IgnoreCase) ||
					NodeDef.Type.Equals(TEXT("SimpleParallel"), ESearchCase::IgnoreCase);

				if (bIsComposite)
				{
					// Create composite node
					UBTCompositeNode* CompositeNode = nullptr;
					if (NodeDef.Type.Equals(TEXT("Sequence"), ESearchCase::IgnoreCase))
					{
						CompositeNode = NewObject<UBTComposite_Sequence>(RootNode);
					}
					else
					{
						CompositeNode = NewObject<UBTComposite_Selector>(RootNode);
					}

					if (CompositeNode)
					{
						ChildEntry.ChildComposite = CompositeNode;
						NodesCreated++;
						LogGeneration(FString::Printf(TEXT("  Created composite node: %s (%s)"), *NodeDef.Id, *NodeDef.Type));
					}
				}
				else
				{
					// Create task node - try to load specified class or use built-in
					UBTTaskNode* TaskNode = nullptr;

					if (!NodeDef.TaskClass.IsEmpty())
					{
						// Try to load the specified task class
						TArray<FString> TaskPaths;
						TaskPaths.Add(FString::Printf(TEXT("%s/AI/Tasks/%s.%s_C"), *GetProjectRoot(), *NodeDef.TaskClass, *NodeDef.TaskClass));
						TaskPaths.Add(FString::Printf(TEXT("/NarrativePro/AI/Tasks/%s.%s_C"), *NodeDef.TaskClass, *NodeDef.TaskClass));
						TaskPaths.Add(FString::Printf(TEXT("/Script/AIModule.%s"), *NodeDef.TaskClass));

						UClass* TaskClass = nullptr;
						for (const FString& Path : TaskPaths)
						{
							TaskClass = LoadClass<UBTTaskNode>(nullptr, *Path);
							if (TaskClass)
							{
								break;
							}
						}

						if (TaskClass)
						{
							TaskNode = NewObject<UBTTaskNode>(RootNode, TaskClass);
							LogGeneration(FString::Printf(TEXT("  Created task node: %s (%s)"), *NodeDef.Id, *NodeDef.TaskClass));
						}
						else
						{
							LogGeneration(FString::Printf(TEXT("  [WARNING] Task class not found: %s, using BTTask_Wait"), *NodeDef.TaskClass));
							TaskNode = NewObject<UBTTask_Wait>(RootNode);
						}
					}
					else
					{
						// Default to Wait task
						TaskNode = NewObject<UBTTask_Wait>(RootNode);
						LogGeneration(FString::Printf(TEXT("  Created default Wait task: %s"), *NodeDef.Id));
					}

					if (TaskNode)
					{
						ChildEntry.ChildTask = TaskNode;
						NodesCreated++;
					}
				}

				// Add decorators to this child entry
				for (const FManifestBTDecoratorDefinition& DecoratorDef : NodeDef.Decorators)
				{
					// Try to load decorator class
					UClass* DecoratorClass = nullptr;
					if (DecoratorDef.Class.Equals(TEXT("BTDecorator_Blackboard"), ESearchCase::IgnoreCase))
					{
						DecoratorClass = UBTDecorator_Blackboard::StaticClass();
					}
					else
					{
						TArray<FString> DecPaths;
						DecPaths.Add(FString::Printf(TEXT("/Script/AIModule.%s"), *DecoratorDef.Class));
						DecPaths.Add(FString::Printf(TEXT("%s/AI/Decorators/%s.%s_C"), *GetProjectRoot(), *DecoratorDef.Class, *DecoratorDef.Class));

						for (const FString& Path : DecPaths)
						{
							DecoratorClass = LoadClass<UBTDecorator>(nullptr, *Path);
							if (DecoratorClass)
							{
								break;
							}
						}
					}

					if (DecoratorClass)
					{
						UBTDecorator* Decorator = NewObject<UBTDecorator>(RootNode, DecoratorClass);
						if (Decorator)
						{
							ChildEntry.Decorators.Add(Decorator);
							LogGeneration(FString::Printf(TEXT("    Added decorator: %s"), *DecoratorDef.Class));
						}
					}
				}

				// Add services (only to composite nodes)
				if (ChildEntry.ChildComposite)
				{
					for (const FManifestBTServiceDefinition& ServiceDef : NodeDef.Services)
					{
						UClass* ServiceClass = nullptr;
						TArray<FString> SvcPaths;
						SvcPaths.Add(FString::Printf(TEXT("/Script/AIModule.%s"), *ServiceDef.Class));
						SvcPaths.Add(FString::Printf(TEXT("%s/AI/Services/%s.%s_C"), *GetProjectRoot(), *ServiceDef.Class, *ServiceDef.Class));
						SvcPaths.Add(FString::Printf(TEXT("/NarrativePro/AI/Services/%s.%s_C"), *ServiceDef.Class, *ServiceDef.Class));

						for (const FString& Path : SvcPaths)
						{
							ServiceClass = LoadClass<UBTService>(nullptr, *Path);
							if (ServiceClass)
							{
								break;
							}
						}

						if (ServiceClass)
						{
							UBTService* Service = NewObject<UBTService>(ChildEntry.ChildComposite, ServiceClass);
							if (Service)
							{
								ChildEntry.ChildComposite->Services.Add(Service);
								LogGeneration(FString::Printf(TEXT("    Added service: %s"), *ServiceDef.Class));
							}
						}
					}
				}

				// Add child entry to root node
				if (ChildEntry.ChildComposite || ChildEntry.ChildTask)
				{
					RootNode->Children.Add(ChildEntry);
				}
			}

			LogGeneration(FString::Printf(TEXT("  Created %d nodes in behavior tree"), NodesCreated));
		}
	}

	// Mark dirty and register
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(BT);

	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, BT, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Behavior Tree: %s"), *Definition.Name));

	// v3.0: Store metadata for regeneration tracking
	StoreDataAssetMetadata(BT, TEXT("BT"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// FMaterialGenerator Implementation (v2.6.12: Enhanced with expression graph)
// ============================================================================

#include "Materials/Material.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionPower.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionPanner.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/MaterialFunctionFactoryNew.h"

// v2.6.12: Helper to create material expression by type
UMaterialExpression* FMaterialGenerator::CreateExpression(UMaterial* Material, const FManifestMaterialExpression& ExprDef)
{
	UMaterialExpression* Expression = nullptr;
	FString TypeLower = ExprDef.Type.ToLower();

	// Scalar Parameter
	if (TypeLower == TEXT("scalarparam") || TypeLower == TEXT("scalarparameter") || TypeLower == TEXT("float_param"))
	{
		UMaterialExpressionScalarParameter* ScalarParam = NewObject<UMaterialExpressionScalarParameter>(Material);
		ScalarParam->ParameterName = *ExprDef.Name;
		ScalarParam->DefaultValue = FCString::Atof(*ExprDef.DefaultValue);
		Expression = ScalarParam;
	}
	// Vector Parameter
	else if (TypeLower == TEXT("vectorparam") || TypeLower == TEXT("vectorparameter") || TypeLower == TEXT("color_param"))
	{
		UMaterialExpressionVectorParameter* VectorParam = NewObject<UMaterialExpressionVectorParameter>(Material);
		VectorParam->ParameterName = *ExprDef.Name;
		TArray<FString> Parts;
		ExprDef.DefaultValue.ParseIntoArray(Parts, TEXT(","));
		if (Parts.Num() >= 3)
		{
			VectorParam->DefaultValue = FLinearColor(
				FCString::Atof(*Parts[0].TrimStartAndEnd()),
				FCString::Atof(*Parts[1].TrimStartAndEnd()),
				FCString::Atof(*Parts[2].TrimStartAndEnd()),
				Parts.Num() >= 4 ? FCString::Atof(*Parts[3].TrimStartAndEnd()) : 1.0f
			);
		}
		Expression = VectorParam;
	}
	// Constant
	else if (TypeLower == TEXT("constant") || TypeLower == TEXT("scalar"))
	{
		UMaterialExpressionConstant* Constant = NewObject<UMaterialExpressionConstant>(Material);
		Constant->R = FCString::Atof(*ExprDef.DefaultValue);
		Expression = Constant;
	}
	// Constant3Vector
	else if (TypeLower == TEXT("constant3vector") || TypeLower == TEXT("vector3"))
	{
		UMaterialExpressionConstant3Vector* Vec3 = NewObject<UMaterialExpressionConstant3Vector>(Material);
		TArray<FString> Parts;
		ExprDef.DefaultValue.ParseIntoArray(Parts, TEXT(","));
		if (Parts.Num() >= 3)
		{
			Vec3->Constant = FLinearColor(
				FCString::Atof(*Parts[0].TrimStartAndEnd()),
				FCString::Atof(*Parts[1].TrimStartAndEnd()),
				FCString::Atof(*Parts[2].TrimStartAndEnd())
			);
		}
		Expression = Vec3;
	}
	// Multiply
	else if (TypeLower == TEXT("multiply") || TypeLower == TEXT("mul"))
	{
		Expression = NewObject<UMaterialExpressionMultiply>(Material);
	}
	// Add
	else if (TypeLower == TEXT("add"))
	{
		Expression = NewObject<UMaterialExpressionAdd>(Material);
	}
	// Subtract
	else if (TypeLower == TEXT("subtract") || TypeLower == TEXT("sub"))
	{
		Expression = NewObject<UMaterialExpressionSubtract>(Material);
	}
	// Divide
	else if (TypeLower == TEXT("divide") || TypeLower == TEXT("div"))
	{
		Expression = NewObject<UMaterialExpressionDivide>(Material);
	}
	// Power
	else if (TypeLower == TEXT("power") || TypeLower == TEXT("pow"))
	{
		UMaterialExpressionPower* Power = NewObject<UMaterialExpressionPower>(Material);
		if (!ExprDef.DefaultValue.IsEmpty())
		{
			Power->ConstExponent = FCString::Atof(*ExprDef.DefaultValue);
		}
		Expression = Power;
	}
	// Fresnel
	else if (TypeLower == TEXT("fresnel"))
	{
		UMaterialExpressionFresnel* Fresnel = NewObject<UMaterialExpressionFresnel>(Material);
		if (ExprDef.Properties.Contains(TEXT("exponent")))
		{
			Fresnel->Exponent = FCString::Atof(*ExprDef.Properties[TEXT("exponent")]);
		}
		if (ExprDef.Properties.Contains(TEXT("base_reflect_fraction")))
		{
			Fresnel->BaseReflectFraction = FCString::Atof(*ExprDef.Properties[TEXT("base_reflect_fraction")]);
		}
		Expression = Fresnel;
	}
	// OneMinus
	else if (TypeLower == TEXT("oneminus") || TypeLower == TEXT("one_minus"))
	{
		Expression = NewObject<UMaterialExpressionOneMinus>(Material);
	}
	// Clamp
	else if (TypeLower == TEXT("clamp"))
	{
		Expression = NewObject<UMaterialExpressionClamp>(Material);
	}
	// Lerp
	else if (TypeLower == TEXT("lerp") || TypeLower == TEXT("linearinterpolate"))
	{
		Expression = NewObject<UMaterialExpressionLinearInterpolate>(Material);
	}
	// Time
	else if (TypeLower == TEXT("time"))
	{
		Expression = NewObject<UMaterialExpressionTime>(Material);
	}
	// Sine
	else if (TypeLower == TEXT("sine") || TypeLower == TEXT("sin"))
	{
		Expression = NewObject<UMaterialExpressionSine>(Material);
	}
	// TexCoord
	else if (TypeLower == TEXT("texcoord") || TypeLower == TEXT("texturecoordinate"))
	{
		UMaterialExpressionTextureCoordinate* TexCoord = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		if (ExprDef.Properties.Contains(TEXT("tiling_u")))
		{
			TexCoord->UTiling = FCString::Atof(*ExprDef.Properties[TEXT("tiling_u")]);
		}
		if (ExprDef.Properties.Contains(TEXT("tiling_v")))
		{
			TexCoord->VTiling = FCString::Atof(*ExprDef.Properties[TEXT("tiling_v")]);
		}
		Expression = TexCoord;
	}
	// Panner
	else if (TypeLower == TEXT("panner"))
	{
		UMaterialExpressionPanner* Panner = NewObject<UMaterialExpressionPanner>(Material);
		if (ExprDef.Properties.Contains(TEXT("speed_x")))
		{
			Panner->SpeedX = FCString::Atof(*ExprDef.Properties[TEXT("speed_x")]);
		}
		if (ExprDef.Properties.Contains(TEXT("speed_y")))
		{
			Panner->SpeedY = FCString::Atof(*ExprDef.Properties[TEXT("speed_y")]);
		}
		Expression = Panner;
	}

	if (Expression)
	{
		Expression->MaterialExpressionEditorX = ExprDef.PosX;
		Expression->MaterialExpressionEditorY = ExprDef.PosY;
		Material->GetExpressionCollection().AddExpression(Expression);
	}

	return Expression;
}

// v2.6.12: Helper to connect expressions
bool FMaterialGenerator::ConnectExpressions(UMaterial* Material, const TMap<FString, UMaterialExpression*>& ExpressionMap, const FManifestMaterialConnection& Connection)
{
	UMaterialExpression* FromExpr = ExpressionMap.FindRef(Connection.FromId);
	if (!FromExpr) return false;

	// Find output index
	int32 OutputIndex = 0;
	FString FromOutput = Connection.FromOutput.ToLower();
	if (FromOutput == TEXT("r") || FromOutput == TEXT("x")) OutputIndex = 1;
	else if (FromOutput == TEXT("g") || FromOutput == TEXT("y")) OutputIndex = 2;
	else if (FromOutput == TEXT("b") || FromOutput == TEXT("z")) OutputIndex = 3;
	else if (FromOutput == TEXT("a") || FromOutput == TEXT("w")) OutputIndex = 4;

	// Connect to material output or another expression
	if (Connection.ToId.Equals(TEXT("Material"), ESearchCase::IgnoreCase))
	{
		FString ToInput = Connection.ToInput.ToLower();
		FExpressionInput* MaterialInput = nullptr;

		if (ToInput == TEXT("basecolor") || ToInput == TEXT("base_color"))
			MaterialInput = &Material->GetEditorOnlyData()->BaseColor;
		else if (ToInput == TEXT("metallic"))
			MaterialInput = &Material->GetEditorOnlyData()->Metallic;
		else if (ToInput == TEXT("roughness"))
			MaterialInput = &Material->GetEditorOnlyData()->Roughness;
		else if (ToInput == TEXT("emissivecolor") || ToInput == TEXT("emissive"))
			MaterialInput = &Material->GetEditorOnlyData()->EmissiveColor;
		else if (ToInput == TEXT("opacity"))
			MaterialInput = &Material->GetEditorOnlyData()->Opacity;
		else if (ToInput == TEXT("normal"))
			MaterialInput = &Material->GetEditorOnlyData()->Normal;

		if (MaterialInput)
		{
			MaterialInput->Connect(OutputIndex, FromExpr);
			return true;
		}
	}
	else
	{
		UMaterialExpression* ToExpr = ExpressionMap.FindRef(Connection.ToId);
		if (ToExpr)
		{
			FString ToInput = Connection.ToInput.ToLower();
			// Find input by name - this is expression-specific
			// For now support common inputs
			if (ToInput == TEXT("a") && ToExpr->IsA<UMaterialExpressionMultiply>())
			{
				Cast<UMaterialExpressionMultiply>(ToExpr)->A.Connect(OutputIndex, FromExpr);
				return true;
			}
			else if (ToInput == TEXT("b") && ToExpr->IsA<UMaterialExpressionMultiply>())
			{
				Cast<UMaterialExpressionMultiply>(ToExpr)->B.Connect(OutputIndex, FromExpr);
				return true;
			}
			else if (ToInput == TEXT("a") && ToExpr->IsA<UMaterialExpressionAdd>())
			{
				Cast<UMaterialExpressionAdd>(ToExpr)->A.Connect(OutputIndex, FromExpr);
				return true;
			}
			else if (ToInput == TEXT("b") && ToExpr->IsA<UMaterialExpressionAdd>())
			{
				Cast<UMaterialExpressionAdd>(ToExpr)->B.Connect(OutputIndex, FromExpr);
				return true;
			}
			else if (ToInput == TEXT("base") && ToExpr->IsA<UMaterialExpressionPower>())
			{
				Cast<UMaterialExpressionPower>(ToExpr)->Base.Connect(OutputIndex, FromExpr);
				return true;
			}
			else if (ToInput == TEXT("exponent") && ToExpr->IsA<UMaterialExpressionPower>())
			{
				Cast<UMaterialExpressionPower>(ToExpr)->Exponent.Connect(OutputIndex, FromExpr);
				return true;
			}
			else if (ToInput == TEXT("input") && ToExpr->IsA<UMaterialExpressionOneMinus>())
			{
				Cast<UMaterialExpressionOneMinus>(ToExpr)->Input.Connect(OutputIndex, FromExpr);
				return true;
			}
			else if (ToInput == TEXT("a") && ToExpr->IsA<UMaterialExpressionLinearInterpolate>())
			{
				Cast<UMaterialExpressionLinearInterpolate>(ToExpr)->A.Connect(OutputIndex, FromExpr);
				return true;
			}
			else if (ToInput == TEXT("b") && ToExpr->IsA<UMaterialExpressionLinearInterpolate>())
			{
				Cast<UMaterialExpressionLinearInterpolate>(ToExpr)->B.Connect(OutputIndex, FromExpr);
				return true;
			}
			else if (ToInput == TEXT("alpha") && ToExpr->IsA<UMaterialExpressionLinearInterpolate>())
			{
				Cast<UMaterialExpressionLinearInterpolate>(ToExpr)->Alpha.Connect(OutputIndex, FromExpr);
				return true;
			}
		}
	}

	return false;
}

FGenerationResult FMaterialGenerator::Generate(const FManifestMaterialDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Materials") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	// v2.1.9: Validate against manifest whitelist FIRST
	if (ValidateAgainstManifest(Definition.Name, TEXT("Material"), Result))
	{
		return Result;
	}

	// v3.0: Check existence with metadata-aware logic
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Material"), Definition.ComputeHash(), Result))
	{
		return Result;
	}

	// Create package
	FString PackagePath = AssetPath;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	// Create material
	UMaterial* Material = NewObject<UMaterial>(Package, *Definition.Name, RF_Public | RF_Standalone);
	if (!Material)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Material"));
	}

	// Configure blend mode
	if (Definition.BlendMode.Equals(TEXT("Translucent"), ESearchCase::IgnoreCase))
	{
		Material->BlendMode = BLEND_Translucent;
	}
	else if (Definition.BlendMode.Equals(TEXT("Masked"), ESearchCase::IgnoreCase))
	{
		Material->BlendMode = BLEND_Masked;
	}
	else if (Definition.BlendMode.Equals(TEXT("Additive"), ESearchCase::IgnoreCase))
	{
		Material->BlendMode = BLEND_Additive;
	}
	else if (Definition.BlendMode.Equals(TEXT("Modulate"), ESearchCase::IgnoreCase))
	{
		Material->BlendMode = BLEND_Modulate;
	}
	else
	{
		Material->BlendMode = BLEND_Opaque;
	}

	// Configure shading model
	if (Definition.ShadingModel.Equals(TEXT("Unlit"), ESearchCase::IgnoreCase))
	{
		Material->SetShadingModel(MSM_Unlit);
	}
	else if (Definition.ShadingModel.Equals(TEXT("Subsurface"), ESearchCase::IgnoreCase))
	{
		Material->SetShadingModel(MSM_Subsurface);
	}

	// v2.6.12: Two-sided
	if (Definition.bTwoSided)
	{
		Material->TwoSided = true;
	}

	// v2.6.12: Create expressions
	TMap<FString, UMaterialExpression*> ExpressionMap;
	for (const FManifestMaterialExpression& ExprDef : Definition.Expressions)
	{
		UMaterialExpression* Expr = CreateExpression(Material, ExprDef);
		if (Expr)
		{
			ExpressionMap.Add(ExprDef.Id, Expr);
			LogGeneration(FString::Printf(TEXT("  Created expression: %s (%s)"), *ExprDef.Id, *ExprDef.Type));
		}
	}

	// v2.6.12: Create connections
	for (const FManifestMaterialConnection& Conn : Definition.Connections)
	{
		if (ConnectExpressions(Material, ExpressionMap, Conn))
		{
			LogGeneration(FString::Printf(TEXT("  Connected: %s.%s -> %s.%s"), *Conn.FromId, *Conn.FromOutput, *Conn.ToId, *Conn.ToInput));
		}
	}

	// Mark dirty and register
	Material->PreEditChange(nullptr);
	Material->PostEditChange();
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Material);

	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Material, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Material: %s (%d expressions, %d connections)"),
		*Definition.Name, Definition.Expressions.Num(), Definition.Connections.Num()));

	// v3.0: Store metadata for regeneration tracking
	StoreDataAssetMetadata(Material, TEXT("M"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// FMaterialFunctionGenerator Implementation (v2.6.12)
// ============================================================================

UMaterialExpression* FMaterialFunctionGenerator::CreateExpressionInFunction(UMaterialFunction* MaterialFunction, const FManifestMaterialExpression& ExprDef)
{
	// Similar to FMaterialGenerator::CreateExpression but creates in MaterialFunction context
	UMaterialExpression* Expression = nullptr;
	FString TypeLower = ExprDef.Type.ToLower();

	// Function Input
	if (TypeLower == TEXT("functioninput") || TypeLower == TEXT("input"))
	{
		UMaterialExpressionFunctionInput* Input = NewObject<UMaterialExpressionFunctionInput>(MaterialFunction);
		Input->InputName = *ExprDef.Name;
		// Set input type based on property
		if (ExprDef.Properties.Contains(TEXT("input_type")))
		{
			FString InputType = ExprDef.Properties[TEXT("input_type")].ToLower();
			if (InputType == TEXT("scalar") || InputType == TEXT("float"))
				Input->InputType = FunctionInput_Scalar;
			else if (InputType == TEXT("vector2"))
				Input->InputType = FunctionInput_Vector2;
			else if (InputType == TEXT("vector3"))
				Input->InputType = FunctionInput_Vector3;
			else if (InputType == TEXT("vector4"))
				Input->InputType = FunctionInput_Vector4;
		}
		Expression = Input;
	}
	// Function Output
	else if (TypeLower == TEXT("functionoutput") || TypeLower == TEXT("output"))
	{
		UMaterialExpressionFunctionOutput* Output = NewObject<UMaterialExpressionFunctionOutput>(MaterialFunction);
		Output->OutputName = *ExprDef.Name;
		Expression = Output;
	}
	// Scalar Parameter
	else if (TypeLower == TEXT("scalarparam") || TypeLower == TEXT("scalarparameter"))
	{
		UMaterialExpressionScalarParameter* ScalarParam = NewObject<UMaterialExpressionScalarParameter>(MaterialFunction);
		ScalarParam->ParameterName = *ExprDef.Name;
		ScalarParam->DefaultValue = FCString::Atof(*ExprDef.DefaultValue);
		Expression = ScalarParam;
	}
	// Constant
	else if (TypeLower == TEXT("constant") || TypeLower == TEXT("scalar"))
	{
		UMaterialExpressionConstant* Constant = NewObject<UMaterialExpressionConstant>(MaterialFunction);
		Constant->R = FCString::Atof(*ExprDef.DefaultValue);
		Expression = Constant;
	}
	// Multiply
	else if (TypeLower == TEXT("multiply") || TypeLower == TEXT("mul"))
	{
		Expression = NewObject<UMaterialExpressionMultiply>(MaterialFunction);
	}
	// Add
	else if (TypeLower == TEXT("add"))
	{
		Expression = NewObject<UMaterialExpressionAdd>(MaterialFunction);
	}
	// Power
	else if (TypeLower == TEXT("power") || TypeLower == TEXT("pow"))
	{
		UMaterialExpressionPower* Power = NewObject<UMaterialExpressionPower>(MaterialFunction);
		if (!ExprDef.DefaultValue.IsEmpty())
		{
			Power->ConstExponent = FCString::Atof(*ExprDef.DefaultValue);
		}
		Expression = Power;
	}
	// Fresnel
	else if (TypeLower == TEXT("fresnel"))
	{
		UMaterialExpressionFresnel* Fresnel = NewObject<UMaterialExpressionFresnel>(MaterialFunction);
		if (ExprDef.Properties.Contains(TEXT("exponent")))
		{
			Fresnel->Exponent = FCString::Atof(*ExprDef.Properties[TEXT("exponent")]);
		}
		Expression = Fresnel;
	}
	// Sine
	else if (TypeLower == TEXT("sine") || TypeLower == TEXT("sin"))
	{
		Expression = NewObject<UMaterialExpressionSine>(MaterialFunction);
	}
	// Time
	else if (TypeLower == TEXT("time"))
	{
		Expression = NewObject<UMaterialExpressionTime>(MaterialFunction);
	}
	// Panner
	else if (TypeLower == TEXT("panner"))
	{
		UMaterialExpressionPanner* Panner = NewObject<UMaterialExpressionPanner>(MaterialFunction);
		if (ExprDef.Properties.Contains(TEXT("speed_x")))
		{
			Panner->SpeedX = FCString::Atof(*ExprDef.Properties[TEXT("speed_x")]);
		}
		if (ExprDef.Properties.Contains(TEXT("speed_y")))
		{
			Panner->SpeedY = FCString::Atof(*ExprDef.Properties[TEXT("speed_y")]);
		}
		Expression = Panner;
	}

	if (Expression)
	{
		Expression->MaterialExpressionEditorX = ExprDef.PosX;
		Expression->MaterialExpressionEditorY = ExprDef.PosY;
		Expression->Function = MaterialFunction;
		MaterialFunction->GetExpressionCollection().AddExpression(Expression);
	}

	return Expression;
}

FGenerationResult FMaterialFunctionGenerator::Generate(const FManifestMaterialFunctionDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Materials/Functions") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	// Validate against manifest
	if (ValidateAgainstManifest(Definition.Name, TEXT("Material Function"), Result))
	{
		return Result;
	}

	// v3.0: Check existence with metadata-aware logic
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Material Function"), Definition.ComputeHash(), Result))
	{
		return Result;
	}

	// Create package
	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	// Create material function
	UMaterialFunction* MaterialFunction = NewObject<UMaterialFunction>(Package, *Definition.Name, RF_Public | RF_Standalone);
	if (!MaterialFunction)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Material Function"));
	}

	// Set description
	if (!Definition.Description.IsEmpty())
	{
		MaterialFunction->Description = Definition.Description;
	}

	// Set expose to library
	MaterialFunction->bExposeToLibrary = Definition.bExposeToLibrary;

	// Create expressions
	TMap<FString, UMaterialExpression*> ExpressionMap;

	// Create function inputs first
	for (const FManifestMaterialFunctionInput& InputDef : Definition.Inputs)
	{
		UMaterialExpressionFunctionInput* Input = NewObject<UMaterialExpressionFunctionInput>(MaterialFunction);
		Input->InputName = *InputDef.Name;
		Input->SortPriority = InputDef.SortPriority;
		Input->Function = MaterialFunction;

		// Set input type
		FString InputType = InputDef.Type.ToLower();
		if (InputType == TEXT("float") || InputType == TEXT("scalar"))
			Input->InputType = FunctionInput_Scalar;
		else if (InputType == TEXT("float2") || InputType == TEXT("vector2"))
			Input->InputType = FunctionInput_Vector2;
		else if (InputType == TEXT("float3") || InputType == TEXT("vector3"))
			Input->InputType = FunctionInput_Vector3;
		else if (InputType == TEXT("float4") || InputType == TEXT("vector4"))
			Input->InputType = FunctionInput_Vector4;

		MaterialFunction->GetExpressionCollection().AddExpression(Input);
		ExpressionMap.Add(InputDef.Name, Input);
		LogGeneration(FString::Printf(TEXT("  Created input: %s (%s)"), *InputDef.Name, *InputDef.Type));
	}

	// Create function outputs
	for (const FManifestMaterialFunctionOutput& OutputDef : Definition.Outputs)
	{
		UMaterialExpressionFunctionOutput* Output = NewObject<UMaterialExpressionFunctionOutput>(MaterialFunction);
		Output->OutputName = *OutputDef.Name;
		Output->SortPriority = OutputDef.SortPriority;
		Output->Function = MaterialFunction;
		MaterialFunction->GetExpressionCollection().AddExpression(Output);
		ExpressionMap.Add(OutputDef.Name, Output);
		LogGeneration(FString::Printf(TEXT("  Created output: %s"), *OutputDef.Name));
	}

	// Create other expressions
	for (const FManifestMaterialExpression& ExprDef : Definition.Expressions)
	{
		UMaterialExpression* Expr = CreateExpressionInFunction(MaterialFunction, ExprDef);
		if (Expr)
		{
			ExpressionMap.Add(ExprDef.Id, Expr);
			LogGeneration(FString::Printf(TEXT("  Created expression: %s (%s)"), *ExprDef.Id, *ExprDef.Type));
		}
	}

	// Create connections (simplified - function outputs have different connection API)
	// For now, log that connections need manual setup for complex cases
	if (Definition.Connections.Num() > 0)
	{
		LogGeneration(FString::Printf(TEXT("  Note: %d connections defined - complex connections may need manual setup"), Definition.Connections.Num()));
	}

	// Mark dirty and register
	MaterialFunction->PreEditChange(nullptr);
	MaterialFunction->PostEditChange();
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(MaterialFunction);

	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, MaterialFunction, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Material Function: %s (%d inputs, %d outputs)"),
		*Definition.Name, Definition.Inputs.Num(), Definition.Outputs.Num()));

	// v3.0: Store metadata for regeneration tracking
	StoreDataAssetMetadata(MaterialFunction, TEXT("MF"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// FTagGenerator Implementation
// ============================================================================

FGenerationSummary FTagGenerator::GenerateTags(const TArray<FString>& Tags, const FString& TagsIniPath)
{
	FGenerationSummary Summary;
	
	// Read existing file content
	FString ExistingContent;
	bool bFileLoaded = FFileHelper::LoadFileToString(ExistingContent, *TagsIniPath);
	
	// v2.1.3 FIX: Log file read status for debugging
	if (bFileLoaded)
	{
		LogGeneration(FString::Printf(TEXT("Loaded existing tags file: %s (%d bytes)"), *TagsIniPath, ExistingContent.Len()));
	}
	else
	{
		LogGeneration(FString::Printf(TEXT("Tags file not found or empty, will create: %s"), *TagsIniPath));
	}
	
	// Parse existing tags
	// v2.1.3 FIX: Support both old and new tag formats
	// Old format: +GameplayTags=(Tag="TagName")
	// New format (UE5.6): +GameplayTagList=(Tag="TagName",DevComment="")
	TSet<FString> ExistingTags;
	TArray<FString> Lines;
	ExistingContent.ParseIntoArrayLines(Lines);
	for (const FString& Line : Lines)
	{
		FString TagPart;
		if (Line.StartsWith(TEXT("+GameplayTagList=")))
		{
			TagPart = Line.Mid(17); // Skip "+GameplayTagList="
		}
		else if (Line.StartsWith(TEXT("+GameplayTags=")))
		{
			TagPart = Line.Mid(14); // Skip "+GameplayTags="
		}
		else
		{
			continue;
		}
		
		TagPart = TagPart.TrimStartAndEnd();
		if (TagPart.StartsWith(TEXT("(")) && TagPart.EndsWith(TEXT(")")))
		{
			// Parse (Tag="TagName"...) format
			int32 TagStart = TagPart.Find(TEXT("Tag=\""));
			if (TagStart != INDEX_NONE)
			{
				TagStart += 5; // Skip Tag="
				int32 TagEnd = TagPart.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, TagStart);
				if (TagEnd != INDEX_NONE)
				{
					ExistingTags.Add(TagPart.Mid(TagStart, TagEnd - TagStart));
				}
			}
		}
	}
	
	// v2.1.3 FIX: Log existing tags count
	LogGeneration(FString::Printf(TEXT("Found %d existing tags in file"), ExistingTags.Num()));
	
	// Add new tags
	FString NewTagsContent;
	for (const FString& Tag : Tags)
	{
		FGenerationResult Result;
		Result.AssetName = Tag;
		Result.Category = TEXT("Gameplay Tags");
		
		if (ExistingTags.Contains(Tag))
		{
			Result.Status = EGenerationStatus::Skipped;
			Result.Message = TEXT("Already exists");
			LogGeneration(FString::Printf(TEXT("Skipping existing Tag: %s"), *Tag));
		}
		else
		{
			NewTagsContent += FString::Printf(TEXT("+GameplayTags=(Tag=\"%s\")\n"), *Tag);
			Result.Status = EGenerationStatus::New;
			Result.Message = TEXT("Created successfully");
			LogGeneration(FString::Printf(TEXT("Created Tag: %s"), *Tag));
		}
		
		Summary.AddResult(Result);
	}
	
	// Append new tags to file
	if (!NewTagsContent.IsEmpty())
	{
		FString FinalContent = ExistingContent;
		if (!FinalContent.EndsWith(TEXT("\n")))
		{
			FinalContent += TEXT("\n");
		}
		FinalContent += NewTagsContent;

		FFileHelper::SaveStringToFile(FinalContent, *TagsIniPath);
	}

	return Summary;
}

// ============================================================================
// v2.2.0: FEventGraphGenerator Implementation
// ============================================================================

// v2.6.7: Static member for tracking missing dependencies
TArray<FMissingDependencyInfo> FEventGraphGenerator::MissingDependencies;

// v2.6.7: Add a missing dependency to the tracking list
void FEventGraphGenerator::AddMissingDependency(const FString& Name, const FString& Type, const FString& Context)
{
	// Avoid duplicates
	for (const auto& Dep : MissingDependencies)
	{
		if (Dep.DependencyName == Name && Dep.DependencyType == Type)
		{
			return;
		}
	}
	MissingDependencies.Add(FMissingDependencyInfo(Name, Type, Context));
	LogGeneration(FString::Printf(TEXT("  Missing dependency detected: %s (%s) - %s"), *Name, *Type, *Context));
}

// ============================================================================
// v2.7.7: Pre-generation validation for event graphs
// ============================================================================

bool FEventGraphGenerator::ValidateEventGraph(
	const FManifestEventGraphDefinition& GraphDefinition,
	const FString& ContextName,
	TArray<FString>& OutErrors)
{
	bool bValid = true;

	// Check for empty graph
	if (GraphDefinition.Nodes.Num() == 0)
	{
		OutErrors.Add(FString::Printf(TEXT("[%s] Event graph has no nodes defined"), *ContextName));
		bValid = false;
	}

	// Build set of valid node IDs and map node types
	TSet<FString> NodeIds;
	TSet<FString> DuplicateIds;
	TMap<FString, FString> NodeTypes;  // v2.8.2: Map node ID to type for connection validation

	// v2.8.2: Supported node types for validation
	static TSet<FString> SupportedNodeTypes = {
		TEXT("Event"), TEXT("CustomEvent"), TEXT("CallFunction"), TEXT("Branch"),
		TEXT("VariableGet"), TEXT("VariableSet"), TEXT("PropertyGet"), TEXT("PropertySet"),
		TEXT("Sequence"), TEXT("Delay"), TEXT("PrintString"), TEXT("DynamicCast"),
		TEXT("ForEachLoop"), TEXT("SpawnActor"), TEXT("BreakStruct"), TEXT("MakeArray"),
		TEXT("GetArrayItem"), TEXT("Self")
	};

	for (const FManifestGraphNodeDefinition& Node : GraphDefinition.Nodes)
	{
		if (Node.Id.IsEmpty())
		{
			OutErrors.Add(FString::Printf(TEXT("[%s] Node has empty ID"), *ContextName));
			bValid = false;
			continue;
		}

		if (NodeIds.Contains(Node.Id))
		{
			if (!DuplicateIds.Contains(Node.Id))
			{
				OutErrors.Add(FString::Printf(TEXT("[%s] Duplicate node ID: %s"), *ContextName, *Node.Id));
				DuplicateIds.Add(Node.Id);
				bValid = false;
			}
		}
		else
		{
			NodeIds.Add(Node.Id);
			NodeTypes.Add(Node.Id, Node.Type);
		}

		// v2.8.2: Validate node type is supported
		if (!SupportedNodeTypes.Contains(Node.Type))
		{
			OutErrors.Add(FString::Printf(TEXT("[%s] Node '%s' has unsupported type: %s"), *ContextName, *Node.Id, *Node.Type));
			bValid = false;
		}

		// Check required properties based on node type
		if (Node.Type == TEXT("Event"))
		{
			if (!Node.Properties.Contains(TEXT("event_name")))
			{
				OutErrors.Add(FString::Printf(TEXT("[%s] Event node '%s' missing event_name property"), *ContextName, *Node.Id));
				bValid = false;
			}
		}
		else if (Node.Type == TEXT("CustomEvent"))
		{
			if (!Node.Properties.Contains(TEXT("event_name")))
			{
				OutErrors.Add(FString::Printf(TEXT("[%s] CustomEvent node '%s' missing event_name property"), *ContextName, *Node.Id));
				bValid = false;
			}
		}
		else if (Node.Type == TEXT("CallFunction"))
		{
			if (!Node.Properties.Contains(TEXT("function")))
			{
				OutErrors.Add(FString::Printf(TEXT("[%s] CallFunction node '%s' missing function property"), *ContextName, *Node.Id));
				bValid = false;
			}
		}
		else if (Node.Type == TEXT("DynamicCast"))
		{
			if (!Node.Properties.Contains(TEXT("target_class")))
			{
				OutErrors.Add(FString::Printf(TEXT("[%s] DynamicCast node '%s' missing target_class property"), *ContextName, *Node.Id));
				bValid = false;
			}
		}
		else if (Node.Type == TEXT("VariableGet") || Node.Type == TEXT("VariableSet"))
		{
			if (!Node.Properties.Contains(TEXT("variable_name")))
			{
				OutErrors.Add(FString::Printf(TEXT("[%s] Variable node '%s' missing variable_name property"), *ContextName, *Node.Id));
				bValid = false;
			}
		}
		else if (Node.Type == TEXT("PropertyGet") || Node.Type == TEXT("PropertySet"))
		{
			// v2.8.2: Validate PropertyGet/PropertySet nodes
			if (!Node.Properties.Contains(TEXT("property_name")))
			{
				OutErrors.Add(FString::Printf(TEXT("[%s] Property node '%s' missing property_name property"), *ContextName, *Node.Id));
				bValid = false;
			}
		}
		else if (Node.Type == TEXT("BreakStruct"))
		{
			// v2.8.2: Validate BreakStruct nodes
			if (!Node.Properties.Contains(TEXT("struct_type")))
			{
				OutErrors.Add(FString::Printf(TEXT("[%s] BreakStruct node '%s' missing struct_type property"), *ContextName, *Node.Id));
				bValid = false;
			}
		}
		else if (Node.Type == TEXT("MakeArray"))
		{
			// v2.8.2: Validate MakeArray nodes - element_type is optional but recommended
			if (!Node.Properties.Contains(TEXT("element_type")))
			{
				OutErrors.Add(FString::Printf(TEXT("[%s] WARNING: MakeArray node '%s' missing element_type property - will use default"), *ContextName, *Node.Id));
				// Not an error, just a warning
			}
		}
	}

	// v2.8.2: Track exec output connections to detect multiple connections from same pin
	TMap<FString, int32> ExecOutputConnectionCount;

	// Validate connections reference existing nodes
	for (const FManifestGraphConnectionDefinition& Connection : GraphDefinition.Connections)
	{
		if (!NodeIds.Contains(Connection.From.NodeId))
		{
			OutErrors.Add(FString::Printf(TEXT("[%s] Connection references unknown source node: %s"),
				*ContextName, *Connection.From.NodeId));
			bValid = false;
		}
		if (!NodeIds.Contains(Connection.To.NodeId))
		{
			OutErrors.Add(FString::Printf(TEXT("[%s] Connection references unknown target node: %s"),
				*ContextName, *Connection.To.NodeId));
			bValid = false;
		}
		if (Connection.From.PinName.IsEmpty())
		{
			OutErrors.Add(FString::Printf(TEXT("[%s] Connection from %s has empty pin name"),
				*ContextName, *Connection.From.NodeId));
			bValid = false;
		}
		if (Connection.To.PinName.IsEmpty())
		{
			OutErrors.Add(FString::Printf(TEXT("[%s] Connection to %s has empty pin name"),
				*ContextName, *Connection.To.NodeId));
			bValid = false;
		}

		// v2.8.2: Check for multiple exec connections from same output pin
		FString ExecPinKey = Connection.From.NodeId + TEXT(".") + Connection.From.PinName;
		if (Connection.From.PinName == TEXT("Then") || Connection.From.PinName == TEXT("Exec") ||
			Connection.From.PinName == TEXT("Completed") || Connection.From.PinName == TEXT("true") ||
			Connection.From.PinName == TEXT("false"))
		{
			int32& Count = ExecOutputConnectionCount.FindOrAdd(ExecPinKey);
			Count++;
			if (Count > 1)
			{
				OutErrors.Add(FString::Printf(TEXT("[%s] Multiple exec connections from %s.%s - use Sequence node instead"),
					*ContextName, *Connection.From.NodeId, *Connection.From.PinName));
				bValid = false;
			}
		}
	}

	// v2.8.2: Check that Event/CustomEvent nodes have at least one outgoing exec connection
	for (const FManifestGraphNodeDefinition& Node : GraphDefinition.Nodes)
	{
		if (Node.Type == TEXT("Event") || Node.Type == TEXT("CustomEvent"))
		{
			bool bHasExecOutput = false;
			for (const FManifestGraphConnectionDefinition& Connection : GraphDefinition.Connections)
			{
				if (Connection.From.NodeId == Node.Id &&
					(Connection.From.PinName == TEXT("Then") || Connection.From.PinName == TEXT("Exec")))
				{
					bHasExecOutput = true;
					break;
				}
			}
			if (!bHasExecOutput)
			{
				OutErrors.Add(FString::Printf(TEXT("[%s] WARNING: %s node '%s' has no outgoing exec connection - event will do nothing"),
					*ContextName, *Node.Type, *Node.Id));
				// Warning, not error
			}
		}
	}

	return bValid;
}

bool FEventGraphGenerator::ValidateActorBlueprintEventGraph(
	const FManifestActorBlueprintDefinition& Definition,
	TArray<FString>& OutErrors)
{
	bool bValid = true;

	// Check if this is an NPC blueprint that should have an event graph
	if (Definition.ParentClass == TEXT("NarrativeNPCCharacter"))
	{
		if (!Definition.bHasInlineEventGraph && Definition.EventGraphName.IsEmpty())
		{
			OutErrors.Add(FString::Printf(TEXT("[%s] NPC blueprint (NarrativeNPCCharacter) has no event_graph defined - consider adding BeginPlay/EndPlay logic"), 
				*Definition.Name));
			// This is a warning, not an error - don't set bValid = false
		}
	}

	// If has inline event graph, validate it
	if (Definition.bHasInlineEventGraph)
	{
		FManifestEventGraphDefinition GraphDef;
		GraphDef.Name = Definition.EventGraphName;
		GraphDef.Nodes = Definition.EventGraphNodes;
		GraphDef.Connections = Definition.EventGraphConnections;

		if (!ValidateEventGraph(GraphDef, Definition.Name, OutErrors))
		{
			bValid = false;
		}
	}

	return bValid;
}

bool FEventGraphGenerator::GenerateEventGraph(
	UBlueprint* Blueprint,
	const FManifestEventGraphDefinition& GraphDefinition,
	const FString& ProjectRoot)
{
	// v2.6.7: Clear missing dependencies at start of each generation
	ClearMissingDependencies();

	if (!Blueprint)
	{
		LogGeneration(TEXT("EventGraph generation failed: Blueprint is null"));
		return false;
	}

	// Find the event graph (usually the first UberGraph)
	UEdGraph* EventGraph = nullptr;
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph)
		{
			EventGraph = Graph;
			break;
		}
	}

	if (!EventGraph)
	{
		LogGeneration(FString::Printf(TEXT("EventGraph generation failed: No event graph found in Blueprint %s"),
			*Blueprint->GetName()));
		return false;
	}

	LogGeneration(FString::Printf(TEXT("Generating event graph '%s' in Blueprint %s"),
		*GraphDefinition.Name, *Blueprint->GetName()));

	// Map to track created nodes by their definition IDs
	TMap<FString, UK2Node*> NodeMap;
	int32 NodesCreated = 0;
	int32 NodesFailed = 0;
	int32 ConnectionsCreated = 0;
	int32 ConnectionsFailed = 0;

	// Create all nodes first
	for (const FManifestGraphNodeDefinition& NodeDef : GraphDefinition.Nodes)
	{
		UK2Node* CreatedNode = nullptr;

		// Determine node type and create appropriate node
		if (NodeDef.Type.Equals(TEXT("Event"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateEventNode(EventGraph, NodeDef, Blueprint);
		}
		else if (NodeDef.Type.Equals(TEXT("CustomEvent"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateCustomEventNode(EventGraph, NodeDef, Blueprint);
		}
		else if (NodeDef.Type.Equals(TEXT("CallFunction"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateCallFunctionNode(EventGraph, NodeDef, Blueprint);
		}
		else if (NodeDef.Type.Equals(TEXT("Branch"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateBranchNode(EventGraph, NodeDef);
		}
		else if (NodeDef.Type.Equals(TEXT("VariableGet"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateVariableGetNode(EventGraph, NodeDef, Blueprint);
		}
		else if (NodeDef.Type.Equals(TEXT("VariableSet"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateVariableSetNode(EventGraph, NodeDef, Blueprint);
		}
		else if (NodeDef.Type.Equals(TEXT("Sequence"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateSequenceNode(EventGraph, NodeDef);
		}
		else if (NodeDef.Type.Equals(TEXT("Delay"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateDelayNode(EventGraph, NodeDef);
		}
		else if (NodeDef.Type.Equals(TEXT("PrintString"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreatePrintStringNode(EventGraph, NodeDef);
		}
		else if (NodeDef.Type.Equals(TEXT("DynamicCast"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateDynamicCastNode(EventGraph, NodeDef);
		}
		// v2.4.0: ForEachLoop support
		else if (NodeDef.Type.Equals(TEXT("ForEachLoop"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateForEachLoopNode(EventGraph, NodeDef);
		}
		// v2.4.0: SpawnActor support
		else if (NodeDef.Type.Equals(TEXT("SpawnActor"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateSpawnActorNode(EventGraph, NodeDef, Blueprint);
		}
		// v2.4.2: PropertyGet - access properties on external objects (e.g., MaxWalkSpeed on CharacterMovementComponent)
		else if (NodeDef.Type.Equals(TEXT("PropertyGet"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreatePropertyGetNode(EventGraph, NodeDef, Blueprint);
		}
		// v2.4.2: PropertySet - set properties on external objects
		else if (NodeDef.Type.Equals(TEXT("PropertySet"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreatePropertySetNode(EventGraph, NodeDef, Blueprint);
		}
		// v2.7.0: BreakStruct - break a struct into individual members
		else if (NodeDef.Type.Equals(TEXT("BreakStruct"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateBreakStructNode(EventGraph, NodeDef);
		}
		// v2.7.0: MakeArray - create an array from individual elements
		else if (NodeDef.Type.Equals(TEXT("MakeArray"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateMakeArrayNode(EventGraph, NodeDef);
		}
		// v2.7.0: GetArrayItem - access an element at a specific index
		else if (NodeDef.Type.Equals(TEXT("GetArrayItem"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateGetArrayItemNode(EventGraph, NodeDef);
		}
		// v2.7.8: Self - reference to the blueprint self
		else if (NodeDef.Type.Equals(TEXT("Self"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateSelfNode(EventGraph, NodeDef);
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("Unknown node type '%s' for node '%s'"),
				*NodeDef.Type, *NodeDef.Id));
			continue;
		}

		if (CreatedNode)
		{
			NodeMap.Add(NodeDef.Id, CreatedNode);
			NodesCreated++;

			// Set position if specified
			if (NodeDef.bHasPosition)
			{
				CreatedNode->NodePosX = static_cast<int32>(NodeDef.PositionX);
				CreatedNode->NodePosY = static_cast<int32>(NodeDef.PositionY);
			}

			LogGeneration(FString::Printf(TEXT("  Created node: %s (%s)"),
				*NodeDef.Id, *NodeDef.Type));
		}
		else
		{
			NodesFailed++;
		}
	}

	// Auto-layout nodes without explicit positions
	AutoLayoutNodes(NodeMap, GraphDefinition.Nodes);

	// v2.5.2: Pre-process exec connections to reroute around pure nodes
	// Build a map of exec flow: NodeId -> (ExecSourceNodeId, ExecTargetNodeId)
	TMap<FString, FString> ExecSourceMap;  // NodeId -> which node connects TO this node's Exec
	TMap<FString, FString> ExecTargetMap;  // NodeId -> which node this node's Exec connects TO
	TSet<FString> PureNodeIds;             // Set of pure node IDs

	// Identify pure nodes and build exec flow maps
	for (const FManifestGraphConnectionDefinition& Connection : GraphDefinition.Connections)
	{
		bool bIsExecConnection = Connection.From.PinName.Equals(TEXT("Exec"), ESearchCase::IgnoreCase) ||
		                         Connection.From.PinName.Equals(TEXT("Then"), ESearchCase::IgnoreCase) ||
		                         Connection.From.PinName.Equals(TEXT("True"), ESearchCase::IgnoreCase) ||
		                         Connection.To.PinName.Equals(TEXT("Exec"), ESearchCase::IgnoreCase) ||
		                         Connection.To.PinName.Equals(TEXT("Execute"), ESearchCase::IgnoreCase);

		if (bIsExecConnection)
		{
			ExecTargetMap.Add(Connection.From.NodeId, Connection.To.NodeId);
			ExecSourceMap.Add(Connection.To.NodeId, Connection.From.NodeId);

			// Check if either node is pure
			UK2Node* const* FromNodePtr = NodeMap.Find(Connection.From.NodeId);
			UK2Node* const* ToNodePtr = NodeMap.Find(Connection.To.NodeId);

			if (FromNodePtr && *FromNodePtr && (*FromNodePtr)->IsNodePure())
			{
				PureNodeIds.Add(Connection.From.NodeId);
			}
			if (ToNodePtr && *ToNodePtr && (*ToNodePtr)->IsNodePure())
			{
				PureNodeIds.Add(Connection.To.NodeId);
			}
		}
	}

	// Build rerouted exec connections that bypass pure nodes
	TArray<FManifestGraphConnectionDefinition> ReroutedConnections;
	TSet<FString> ProcessedReroutes; // Track which reroutes we've already added

	for (const FString& PureNodeId : PureNodeIds)
	{
		// Find the source (what connects TO this pure node)
		FString CurrentNode = PureNodeId;
		FString SourceNode;

		// Walk backwards to find the first non-pure node
		while (ExecSourceMap.Contains(CurrentNode))
		{
			FString PrevNode = ExecSourceMap[CurrentNode];
			if (!PureNodeIds.Contains(PrevNode))
			{
				SourceNode = PrevNode;
				break;
			}
			CurrentNode = PrevNode;
		}

		// Find the target (what this pure node connects TO)
		CurrentNode = PureNodeId;
		FString TargetNode;

		// Walk forwards to find the first non-pure node
		while (ExecTargetMap.Contains(CurrentNode))
		{
			FString NextNode = ExecTargetMap[CurrentNode];
			if (!PureNodeIds.Contains(NextNode))
			{
				TargetNode = NextNode;
				break;
			}
			CurrentNode = NextNode;
		}

		// Create rerouted connection if we found both ends
		if (!SourceNode.IsEmpty() && !TargetNode.IsEmpty())
		{
			FString RerouteKey = SourceNode + TEXT("->") + TargetNode;
			if (!ProcessedReroutes.Contains(RerouteKey))
			{
				ProcessedReroutes.Add(RerouteKey);

				FManifestGraphConnectionDefinition Reroute;
				Reroute.From.NodeId = SourceNode;
				Reroute.From.PinName = TEXT("Then"); // Most common exec output
				Reroute.To.NodeId = TargetNode;
				Reroute.To.PinName = TEXT("Exec");

				// Check the actual output pin name on the source node
				UK2Node* const* SourceNodePtr = NodeMap.Find(SourceNode);
				if (SourceNodePtr && *SourceNodePtr)
				{
					for (UEdGraphPin* Pin : (*SourceNodePtr)->Pins)
					{
						if (Pin && Pin->Direction == EGPD_Output &&
						    Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec &&
						    Pin->LinkedTo.Num() == 0) // Find unconnected exec output
						{
							Reroute.From.PinName = Pin->PinName.ToString();
							break;
						}
					}
				}

				ReroutedConnections.Add(Reroute);
				LogGeneration(FString::Printf(TEXT("  REROUTE: %s.%s -> %s.%s (bypassing pure nodes)"),
					*Reroute.From.NodeId, *Reroute.From.PinName,
					*Reroute.To.NodeId, *Reroute.To.PinName));
			}
		}
	}

	// Create all connections (skipping exec connections to/from pure nodes - they'll be handled by reroutes)
	for (const FManifestGraphConnectionDefinition& Connection : GraphDefinition.Connections)
	{
		if (ConnectPins(NodeMap, Connection))
		{
			ConnectionsCreated++;
			LogGeneration(FString::Printf(TEXT("  Connected: %s.%s -> %s.%s"),
				*Connection.From.NodeId, *Connection.From.PinName,
				*Connection.To.NodeId, *Connection.To.PinName));
		}
		else
		{
			ConnectionsFailed++;
			LogGeneration(FString::Printf(TEXT("  FAILED to connect: %s.%s -> %s.%s"),
				*Connection.From.NodeId, *Connection.From.PinName,
				*Connection.To.NodeId, *Connection.To.PinName));
		}
	}

	// Apply rerouted exec connections
	for (const FManifestGraphConnectionDefinition& Reroute : ReroutedConnections)
	{
		if (ConnectPins(NodeMap, Reroute))
		{
			ConnectionsCreated++;
			LogGeneration(FString::Printf(TEXT("  Rerouted: %s.%s -> %s.%s"),
				*Reroute.From.NodeId, *Reroute.From.PinName,
				*Reroute.To.NodeId, *Reroute.To.PinName));
		}
	}

	// ============================================================
	// STABLE SECTION: Post-Generation Validation v2.7.5
	// Verified: 2026-01-11 - DO NOT MODIFY WITHOUT TESTING
	// ============================================================
	// v2.7.4: Enhanced post-generation validation with specific error detection
	int32 UnconnectedInputPins = 0;
	int32 UnconnectedExecOutputPins = 0;
	int32 MultipleExecConnections = 0;
	int32 ASCTargetErrors = 0;
	int32 ArrayInputErrors = 0;
	int32 ActorPinErrors = 0;
	LogGeneration(TEXT("  --- Pin Connection Validation ---"));

	// Get Blueprint parent class to check if self is Actor/ASC
	UClass* BlueprintParentClass = Blueprint->ParentClass;
	bool bSelfIsActor = BlueprintParentClass && BlueprintParentClass->IsChildOf(AActor::StaticClass());
	bool bSelfIsASC = BlueprintParentClass && BlueprintParentClass->IsChildOf(UAbilitySystemComponent::StaticClass());

	for (const auto& NodePair : NodeMap)
	{
		UK2Node* Node = NodePair.Value;
		if (!Node) continue;

		const FString& NodeId = NodePair.Key;

		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin) continue;

			// Skip hidden pins
			if (Pin->bHidden) continue;

			FString PinNameStr = Pin->PinName.ToString();
			FString PinCategory = Pin->PinType.PinCategory.ToString();

			// v2.7.4: Check for multiple exec connections from same output (ERROR - causes <Unnamed> error)
			if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				if (Pin->LinkedTo.Num() > 1)
				{
					UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] MULTIPLE EXEC ERROR: [%s].%s has %d connections (max 1 allowed)"),
						*NodeId, *PinNameStr, Pin->LinkedTo.Num());
					LogGeneration(FString::Printf(TEXT("    ERROR: [%s].%s exec output has %d connections - only 1 allowed! Use Sequence node for multiple paths."),
						*NodeId, *PinNameStr, Pin->LinkedTo.Num()));
					MultipleExecConnections++;
				}
			}

			// Check unconnected input pins (potential issues)
			if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() == 0)
			{
				// Skip exec pins for pure functions and pins with default values
				bool bIsExecPin = (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec);
				bool bHasDefaultValue = !Pin->DefaultValue.IsEmpty() || !Pin->DefaultTextValue.IsEmpty() || Pin->DefaultObject != nullptr;

				// v2.6.4: Skip "self" pins on CallFunction nodes - these are implicit for target_self functions
				bool bIsSelfPin = PinNameStr.Equals(TEXT("self"), ESearchCase::IgnoreCase) ||
				                  PinNameStr.Equals(UEdGraphSchema_K2::PN_Self.ToString(), ESearchCase::IgnoreCase);
				if (bIsSelfPin)
				{
					// Self pins are implicit for Blueprint member functions - not an error
					continue;
				}

				// v2.7.4: Check for ASC Target pins that need connection
				if (PinNameStr.Equals(TEXT("Target"), ESearchCase::IgnoreCase) && !bHasDefaultValue)
				{
					// Check if the pin expects AbilitySystemComponent
					FString SubCategoryName;
					if (Pin->PinType.PinSubCategoryObject.IsValid())
					{
						SubCategoryName = Pin->PinType.PinSubCategoryObject->GetName();
					}
					if (SubCategoryName.Contains(TEXT("AbilitySystem")) ||
					    (PinCategory == TEXT("object") && !bSelfIsASC))
					{
						// This might be an ASC function Target - check the node type
						if (UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Node))
						{
							FString FuncName = CallNode->GetFunctionName().ToString();
							if (FuncName.Contains(TEXT("GameplayEffect")) ||
							    FuncName.Contains(TEXT("GameplayAbility")) ||
							    FuncName.Contains(TEXT("SendGameplayEvent")) ||
							    FuncName.Contains(TEXT("AbilitySystem")))
							{
								UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] ASC TARGET ERROR: [%s].Target needs AbilitySystemComponent connection"),
									*NodeId);
								LogGeneration(FString::Printf(TEXT("    ERROR: [%s].Target requires AbilitySystemComponent - use GetAbilitySystemComponent on actor first"),
									*NodeId));
								ASCTargetErrors++;
							}
						}
					}
				}

				// v2.7.4: Check for Actor pins that need connection when self is not Actor
				if (PinNameStr.Equals(TEXT("Actor"), ESearchCase::IgnoreCase) &&
				    PinCategory == TEXT("object") && !bHasDefaultValue && !bSelfIsActor)
				{
					UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] ACTOR PIN ERROR: [%s].Actor needs connection (self is not Actor)"),
						*NodeId);
					LogGeneration(FString::Printf(TEXT("    ERROR: [%s].Actor needs connection - blueprint is not an Actor, must connect GetAvatarActorFromActorInfo or similar"),
						*NodeId));
					ActorPinErrors++;
				}

				// v2.7.4: Check for array input pins that need MakeArray node
				if (Pin->PinType.IsArray() && !bHasDefaultValue)
				{
					UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] ARRAY INPUT ERROR: [%s].%s is an array and needs MakeArray node connection"),
						*NodeId, *PinNameStr);
					LogGeneration(FString::Printf(TEXT("    ERROR: [%s].%s is an array input - must connect a MakeArray node (e.g., for ObjectTypes)"),
						*NodeId, *PinNameStr));
					ArrayInputErrors++;
				}

				// Log other significant unconnected pins
				if (!bIsExecPin && !bHasDefaultValue && !Pin->PinType.PinCategory.IsNone())
				{
					// Only warn about object/struct pins that likely need connections
					if (PinCategory == TEXT("object") || PinCategory == TEXT("struct") ||
					    PinNameStr.Contains(TEXT("Parent")) ||
					    PinNameStr.Contains(TEXT("Socket")) ||
					    PinNameStr.Contains(TEXT("Object")))
					{
						LogGeneration(FString::Printf(TEXT("    WARN: [%s].%s (%s) is not connected"),
							*NodeId, *PinNameStr, *PinCategory));
						UnconnectedInputPins++;
					}
				}
			}

			// Check unconnected execution outputs on non-pure nodes (broken flow)
			if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				if (Pin->LinkedTo.Num() == 0)
				{
					// Don't warn about False branches or CastFailed - those are often intentionally unconnected
					if (!PinNameStr.Equals(TEXT("False"), ESearchCase::IgnoreCase) &&
					    !PinNameStr.Contains(TEXT("Failed"), ESearchCase::IgnoreCase) &&
					    !PinNameStr.Equals(TEXT("Completed"), ESearchCase::IgnoreCase))
					{
						// Only log if this looks like a main execution path
						if (PinNameStr.Equals(TEXT("Then"), ESearchCase::IgnoreCase) ||
						    PinNameStr.Equals(TEXT("True"), ESearchCase::IgnoreCase) ||
						    PinNameStr.StartsWith(TEXT("Then_"), ESearchCase::IgnoreCase))
						{
							LogGeneration(FString::Printf(TEXT("    INFO: [%s].%s execution output not connected (may be intentional)"),
								*NodeId, *Pin->PinName.ToString()));
							UnconnectedExecOutputPins++;
						}
					}
				}
			}
		}
	}

	// Log summary
	LogGeneration(FString::Printf(TEXT("Event graph '%s' summary: Nodes %d/%d, Connections %d/%d"),
		*GraphDefinition.Name,
		NodesCreated, NodesCreated + NodesFailed,
		ConnectionsCreated, ConnectionsCreated + ConnectionsFailed));

	// v2.7.4: Enhanced error reporting
	int32 TotalErrors = MultipleExecConnections + ASCTargetErrors + ArrayInputErrors + ActorPinErrors;
	if (TotalErrors > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] VALIDATION ERRORS in %s:"), *GraphDefinition.Name);
		LogGeneration(TEXT("  --- VALIDATION ERRORS ---"));

		if (MultipleExecConnections > 0)
		{
			UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator]   - Multiple exec connections: %d (use Sequence node)"), MultipleExecConnections);
			LogGeneration(FString::Printf(TEXT("    - EXEC ERROR: %d pin(s) have multiple connections - use Sequence node instead"), MultipleExecConnections));
		}
		if (ASCTargetErrors > 0)
		{
			UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator]   - ASC Target missing: %d (connect GetAbilitySystemComponent)"), ASCTargetErrors);
			LogGeneration(FString::Printf(TEXT("    - ASC ERROR: %d Target pin(s) need AbilitySystemComponent connection"), ASCTargetErrors));
		}
		if (ArrayInputErrors > 0)
		{
			UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator]   - Array inputs missing: %d (connect MakeArray node)"), ArrayInputErrors);
			LogGeneration(FString::Printf(TEXT("    - ARRAY ERROR: %d array input pin(s) need MakeArray node connection"), ArrayInputErrors));
		}
		if (ActorPinErrors > 0)
		{
			UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator]   - Actor pins missing: %d (connect GetAvatarActor)"), ActorPinErrors);
			LogGeneration(FString::Printf(TEXT("    - ACTOR ERROR: %d Actor pin(s) need connection (self is not Actor)"), ActorPinErrors));
		}

		LogGeneration(FString::Printf(TEXT("  TOTAL VALIDATION ERRORS: %d"), TotalErrors));
	}

	// ============================================================
	// END STABLE SECTION: Post-Generation Validation
	// ============================================================
	if (UnconnectedInputPins > 0)
	{
		LogGeneration(FString::Printf(TEXT("  VALIDATION: %d potentially unconnected input pin(s) detected"),
			UnconnectedInputPins));
	}

	if (NodesFailed > 0 || ConnectionsFailed > 0)
	{
		LogGeneration(FString::Printf(TEXT("  WARNING: %d node(s) and %d connection(s) failed!"),
			NodesFailed, ConnectionsFailed));
	}

	// Mark blueprint as modified and compile
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	return true;
}

// ============================================================================
// v2.8.3: Function Override Generation
// ============================================================================
bool FEventGraphGenerator::GenerateFunctionOverride(
	UBlueprint* Blueprint,
	const FManifestFunctionOverrideDefinition& OverrideDefinition,
	const FString& ProjectRoot)
{
	if (!Blueprint)
	{
		LogGeneration(TEXT("Function override generation failed: Blueprint is null"));
		return false;
	}

	LogGeneration(FString::Printf(TEXT("Generating function override '%s' in Blueprint %s"),
		*OverrideDefinition.FunctionName, *Blueprint->GetName()));

	// Find the parent class function to override
	UClass* ParentClass = Blueprint->ParentClass;
	if (!ParentClass)
	{
		LogGeneration(FString::Printf(TEXT("Function override failed: Blueprint '%s' has no parent class"),
			*Blueprint->GetName()));
		return false;
	}

	FName FunctionName(*OverrideDefinition.FunctionName);
	UFunction* ParentFunction = ParentClass->FindFunctionByName(FunctionName);
	if (!ParentFunction)
	{
		LogGeneration(FString::Printf(TEXT("Function override failed: Function '%s' not found in parent class '%s'"),
			*OverrideDefinition.FunctionName, *ParentClass->GetName()));
		return false;
	}

	// Check if function is BlueprintNativeEvent or BlueprintImplementableEvent
	bool bIsBlueprintEvent = (ParentFunction->FunctionFlags & (FUNC_BlueprintEvent | FUNC_Event)) != 0;
	if (!bIsBlueprintEvent)
	{
		LogGeneration(FString::Printf(TEXT("Function override failed: Function '%s' is not a Blueprint event"),
			*OverrideDefinition.FunctionName));
		return false;
	}

	// Find or create the function graph for this override
	UEdGraph* FunctionGraph = nullptr;
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph && Graph->GetFName() == FunctionName)
		{
			FunctionGraph = Graph;
			LogGeneration(FString::Printf(TEXT("  Found existing function graph for '%s'"),
				*OverrideDefinition.FunctionName));
			break;
		}
	}

	if (!FunctionGraph)
	{
		// Create the function override graph
		FunctionGraph = FBlueprintEditorUtils::CreateNewGraph(
			Blueprint,
			FunctionName,
			UEdGraph::StaticClass(),
			UEdGraphSchema_K2::StaticClass());

		if (FunctionGraph)
		{
			Blueprint->FunctionGraphs.Add(FunctionGraph);

			// Create the function entry node
			FGraphNodeCreator<UK2Node_FunctionEntry> EntryNodeCreator(*FunctionGraph);
			UK2Node_FunctionEntry* EntryNode = EntryNodeCreator.CreateNode();
			// v2.8.3: Use FunctionReference.SetExternalMember (replaces deprecated SignatureClass/SignatureName)
			EntryNode->FunctionReference.SetExternalMember(FunctionName, ParentClass);
			EntryNode->NodePosX = 0;
			EntryNode->NodePosY = 0;
			EntryNodeCreator.Finalize();

			// Allocate pins based on the parent function signature
			EntryNode->AllocateDefaultPins();

			LogGeneration(FString::Printf(TEXT("  Created function override graph for '%s'"),
				*OverrideDefinition.FunctionName));
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("Function override failed: Could not create graph for '%s'"),
				*OverrideDefinition.FunctionName));
			return false;
		}
	}

	// Find the entry node
	UK2Node_FunctionEntry* EntryNode = nullptr;
	for (UEdGraphNode* Node : FunctionGraph->Nodes)
	{
		if (UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(Node))
		{
			EntryNode = Entry;
			break;
		}
	}

	if (!EntryNode)
	{
		LogGeneration(FString::Printf(TEXT("Function override failed: No entry node found for '%s'"),
			*OverrideDefinition.FunctionName));
		return false;
	}

	// Map to track created nodes by their definition IDs
	TMap<FString, UK2Node*> NodeMap;

	// Add the entry node to the map with a special ID
	NodeMap.Add(TEXT("FunctionEntry"), EntryNode);

	int32 NodesCreated = 0;
	int32 NodesFailed = 0;

	// Create all nodes (similar to GenerateEventGraph)
	for (const FManifestGraphNodeDefinition& NodeDef : OverrideDefinition.Nodes)
	{
		UK2Node* CreatedNode = nullptr;

		// Handle special node types for function overrides
		if (NodeDef.Type.Equals(TEXT("CallParent"), ESearchCase::IgnoreCase))
		{
			// Create call to parent function node
			UK2Node_CallParentFunction* ParentCallNode = NewObject<UK2Node_CallParentFunction>(FunctionGraph);
			FunctionGraph->AddNode(ParentCallNode, false, false);
			ParentCallNode->SetFromFunction(ParentFunction);
			ParentCallNode->CreateNewGuid();
			ParentCallNode->PostPlacedNewNode();
			ParentCallNode->AllocateDefaultPins();
			CreatedNode = ParentCallNode;
			LogGeneration(FString::Printf(TEXT("  Created CallParent node for '%s'"), *NodeDef.Id));
		}
		// Standard node types (reuse existing creation logic)
		else if (NodeDef.Type.Equals(TEXT("CallFunction"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateCallFunctionNode(FunctionGraph, NodeDef, Blueprint);
		}
		else if (NodeDef.Type.Equals(TEXT("Branch"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateBranchNode(FunctionGraph, NodeDef);
		}
		else if (NodeDef.Type.Equals(TEXT("VariableGet"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateVariableGetNode(FunctionGraph, NodeDef, Blueprint);
		}
		else if (NodeDef.Type.Equals(TEXT("VariableSet"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateVariableSetNode(FunctionGraph, NodeDef, Blueprint);
		}
		else if (NodeDef.Type.Equals(TEXT("Sequence"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateSequenceNode(FunctionGraph, NodeDef);
		}
		else if (NodeDef.Type.Equals(TEXT("PrintString"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreatePrintStringNode(FunctionGraph, NodeDef);
		}
		else if (NodeDef.Type.Equals(TEXT("Self"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateSelfNode(FunctionGraph, NodeDef);
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  Unknown/unsupported node type '%s' for function override node '%s'"),
				*NodeDef.Type, *NodeDef.Id));
			NodesFailed++;
			continue;
		}

		if (CreatedNode)
		{
			NodeMap.Add(NodeDef.Id, CreatedNode);
			NodesCreated++;

			// Set position if specified
			if (NodeDef.bHasPosition)
			{
				CreatedNode->NodePosX = static_cast<int32>(NodeDef.PositionX);
				CreatedNode->NodePosY = static_cast<int32>(NodeDef.PositionY);
			}
			else
			{
				// Default positioning
				CreatedNode->NodePosX = 200 + (NodesCreated * 250);
				CreatedNode->NodePosY = 0;
			}

			LogGeneration(FString::Printf(TEXT("  Created node: %s (%s)"), *NodeDef.Id, *NodeDef.Type));
		}
		else
		{
			NodesFailed++;
		}
	}

	// Create connections
	int32 ConnectionsCreated = 0;
	int32 ConnectionsFailed = 0;

	for (const FManifestGraphConnectionDefinition& Connection : OverrideDefinition.Connections)
	{
		if (ConnectPins(NodeMap, Connection))
		{
			ConnectionsCreated++;
			LogGeneration(FString::Printf(TEXT("  Connected: %s.%s -> %s.%s"),
				*Connection.From.NodeId, *Connection.From.PinName,
				*Connection.To.NodeId, *Connection.To.PinName));
		}
		else
		{
			ConnectionsFailed++;
			LogGeneration(FString::Printf(TEXT("  FAILED to connect: %s.%s -> %s.%s"),
				*Connection.From.NodeId, *Connection.From.PinName,
				*Connection.To.NodeId, *Connection.To.PinName));
		}
	}

	LogGeneration(FString::Printf(TEXT("Function override '%s': %d nodes created, %d failed, %d connections, %d failed"),
		*OverrideDefinition.FunctionName, NodesCreated, NodesFailed, ConnectionsCreated, ConnectionsFailed));

	// Mark blueprint as modified
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	return NodesFailed == 0 && ConnectionsFailed == 0;
}

const FManifestEventGraphDefinition* FEventGraphGenerator::FindEventGraphByName(
	const FManifestData& ManifestData,
	const FString& GraphName)
{
	return ManifestData.FindEventGraphByName(GraphName);
}

UK2Node* FEventGraphGenerator::CreateEventNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef,
	UBlueprint* Blueprint)
{
	// Get event name from properties
	const FString* EventNamePtr = NodeDef.Properties.Find(TEXT("event_name"));
	if (!EventNamePtr || EventNamePtr->IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("Event node '%s' missing event_name property"), *NodeDef.Id));
		return nullptr;
	}

	FName EventName(*(*EventNamePtr));

	// v2.5.2: Check if this event node already exists in the graph to prevent duplicates
	for (UEdGraphNode* ExistingNode : Graph->Nodes)
	{
		if (UK2Node_Event* ExistingEvent = Cast<UK2Node_Event>(ExistingNode))
		{
			// Check if the event references the same function
			if (ExistingEvent->EventReference.GetMemberName() == EventName)
			{
				LogGeneration(FString::Printf(TEXT("Event node '%s' already exists for event '%s' - reusing existing node"),
					*NodeDef.Id, *EventName.ToString()));
				return ExistingEvent;
			}
			// Also check by function name for override events
			if (ExistingEvent->GetFunctionName() == EventName)
			{
				LogGeneration(FString::Printf(TEXT("Event node '%s' already exists for event '%s' (by function name) - reusing existing node"),
					*NodeDef.Id, *EventName.ToString()));
				return ExistingEvent;
			}
		}
	}

	// Create the event node
	UK2Node_Event* EventNode = NewObject<UK2Node_Event>(Graph);
	Graph->AddNode(EventNode, false, false);

	// v2.4.1: Enhanced event resolution - search up the class hierarchy for the event
	// This ensures GAS events like K2_ActivateAbility are found in UGameplayAbility
	UClass* EventOwnerClass = nullptr;
	UClass* SearchClass = Blueprint->ParentClass;

	while (SearchClass && !EventOwnerClass)
	{
		UFunction* EventFunc = SearchClass->FindFunctionByName(EventName);
		if (EventFunc && (EventFunc->FunctionFlags & (FUNC_Event | FUNC_BlueprintEvent)))
		{
			EventOwnerClass = SearchClass;
			break;
		}
		SearchClass = SearchClass->GetSuperClass();
	}

	// Fallback to parent class if not found (will still work for common events)
	if (!EventOwnerClass)
	{
		EventOwnerClass = Blueprint->GeneratedClass ? Blueprint->GeneratedClass : Blueprint->ParentClass;
	}

	if (EventOwnerClass)
	{
		EventNode->EventReference.SetExternalMember(EventName, EventOwnerClass);
	}

	EventNode->bOverrideFunction = true;
	EventNode->CreateNewGuid();
	EventNode->PostPlacedNewNode();
	EventNode->AllocateDefaultPins();

	return EventNode;
}

UK2Node* FEventGraphGenerator::CreateCustomEventNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef,
	UBlueprint* Blueprint)
{
	const FString* EventNamePtr = NodeDef.Properties.Find(TEXT("event_name"));
	if (!EventNamePtr || EventNamePtr->IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("CustomEvent node '%s' missing event_name property"), *NodeDef.Id));
		return nullptr;
	}

	FName CustomEventName(*(*EventNamePtr));

	// v2.5.2: Check if this custom event already exists to prevent duplicates
	for (UEdGraphNode* ExistingNode : Graph->Nodes)
	{
		if (UK2Node_CustomEvent* ExistingCustomEvent = Cast<UK2Node_CustomEvent>(ExistingNode))
		{
			if (ExistingCustomEvent->CustomFunctionName == CustomEventName)
			{
				LogGeneration(FString::Printf(TEXT("CustomEvent node '%s' already exists for event '%s' - reusing existing node"),
					*NodeDef.Id, *CustomEventName.ToString()));
				return ExistingCustomEvent;
			}
		}
	}

	UK2Node_CustomEvent* CustomEventNode = NewObject<UK2Node_CustomEvent>(Graph);
	Graph->AddNode(CustomEventNode, false, false);

	CustomEventNode->CustomFunctionName = CustomEventName;
	CustomEventNode->CreateNewGuid();
	CustomEventNode->PostPlacedNewNode();
	CustomEventNode->AllocateDefaultPins();

	return CustomEventNode;
}

UK2Node* FEventGraphGenerator::CreateCallFunctionNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef,
	UBlueprint* Blueprint)
{
	const FString* FunctionNamePtr = NodeDef.Properties.Find(TEXT("function"));
	if (!FunctionNamePtr || FunctionNamePtr->IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("CallFunction node '%s' missing function property"), *NodeDef.Id));
		return nullptr;
	}

	UK2Node_CallFunction* CallNode = NewObject<UK2Node_CallFunction>(Graph);
	Graph->AddNode(CallNode, false, false);

	FName FunctionName = FName(*(*FunctionNamePtr));
	UFunction* Function = nullptr;

	// Check if target_self is specified - means we should search Blueprint's parent class hierarchy
	const FString* TargetSelfPtr = NodeDef.Properties.Find(TEXT("target_self"));
	bool bTargetSelf = TargetSelfPtr && TargetSelfPtr->ToBool();

	// v2.4.3: Explicit function lookup table for well-known functions
	// Maps function names to the classes where they are defined
	static TMap<FString, UClass*> WellKnownFunctions;
	if (WellKnownFunctions.Num() == 0)
	{
		// Character functions
		WellKnownFunctions.Add(TEXT("GetCharacterMovement"), ACharacter::StaticClass());
		WellKnownFunctions.Add(TEXT("PlayAnimMontage"), ACharacter::StaticClass());
		WellKnownFunctions.Add(TEXT("StopAnimMontage"), ACharacter::StaticClass());
		WellKnownFunctions.Add(TEXT("GetController"), APawn::StaticClass());
		WellKnownFunctions.Add(TEXT("GetPlayerController"), UGameplayStatics::StaticClass());

		// Actor functions
		WellKnownFunctions.Add(TEXT("GetActorLocation"), AActor::StaticClass());
		WellKnownFunctions.Add(TEXT("SetActorLocation"), AActor::StaticClass());
		WellKnownFunctions.Add(TEXT("GetActorRotation"), AActor::StaticClass());
		WellKnownFunctions.Add(TEXT("SetActorRotation"), AActor::StaticClass());
		WellKnownFunctions.Add(TEXT("GetActorForwardVector"), AActor::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_GetActorLocation"), AActor::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_SetActorLocation"), AActor::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_DestroyActor"), AActor::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_AttachToComponent"), AActor::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_DetachFromActor"), AActor::StaticClass());
		WellKnownFunctions.Add(TEXT("SetActorHiddenInGame"), AActor::StaticClass());
		WellKnownFunctions.Add(TEXT("GetComponentByClass"), AActor::StaticClass());

		// Component functions
		WellKnownFunctions.Add(TEXT("GetSocketLocation"), USceneComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("GetSocketRotation"), USceneComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_GetComponentLocation"), USceneComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_SetWorldLocation"), USceneComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("SetVisibility"), USceneComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("SetHiddenInGame"), USceneComponent::StaticClass());

		// GAS functions on AbilitySystemComponent - use K2_ prefixed versions for Blueprint
		WellKnownFunctions.Add(TEXT("ApplyGameplayEffectSpecToSelf"), UAbilitySystemComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_ApplyGameplayEffectSpecToSelf"), UAbilitySystemComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("ApplyGameplayEffectSpecToTarget"), UAbilitySystemComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_ApplyGameplayEffectSpecToTarget"), UAbilitySystemComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("ApplyGameplayEffectToSelf"), UAbilitySystemComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("BP_ApplyGameplayEffectToSelf"), UAbilitySystemComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("ApplyGameplayEffectToTarget"), UAbilitySystemComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("BP_ApplyGameplayEffectToTarget"), UAbilitySystemComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("RemoveActiveGameplayEffect"), UAbilitySystemComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("RemoveActiveGameplayEffectBySourceEffect"), UAbilitySystemComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("GetActiveGameplayEffectStackCount"), UAbilitySystemComponent::StaticClass());

		// GAS Blueprint Library functions
		WellKnownFunctions.Add(TEXT("GetAbilitySystemComponent"), UAbilitySystemBlueprintLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("SendGameplayEventToActor"), UAbilitySystemBlueprintLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("MakeSpecHandle"), UAbilitySystemBlueprintLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("AssignTagSetByCallerMagnitude"), UAbilitySystemBlueprintLibrary::StaticClass());

		// Gameplay Ability functions - these are on the ability itself
		WellKnownFunctions.Add(TEXT("K2_EndAbility"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_CommitAbility"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_CommitAbilityCooldown"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_CommitAbilityCost"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_ActivateAbility"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_CanActivateAbility"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("GetAvatarActorFromActorInfo"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("GetOwningActorFromActorInfo"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("GetAbilitySystemComponentFromActorInfo"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("MakeOutgoingGameplayEffectSpec"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_MakeOutgoingGameplayEffectSpec"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("BP_ApplyGameplayEffectToOwner"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("BP_ApplyGameplayEffectToTarget"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("BP_ApplyGameplayEffectToSelf"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("BP_RemoveGameplayEffectFromOwnerWithAssetTags"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("BP_RemoveGameplayEffectFromOwnerWithGrantedTags"), UGameplayAbility::StaticClass());

		// Math functions
		WellKnownFunctions.Add(TEXT("Multiply_FloatFloat"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("Divide_FloatFloat"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("Add_FloatFloat"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("Subtract_FloatFloat"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("FClamp"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("RandomFloatInRange"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("GetForwardVector"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("MakeRotator"), UKismetMathLibrary::StaticClass());

		// System functions
		WellKnownFunctions.Add(TEXT("Delay"), UKismetSystemLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("PrintString"), UKismetSystemLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("IsValid"), UKismetSystemLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("IsValidClass"), UKismetSystemLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_SetTimer"), UKismetSystemLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_ClearTimer"), UKismetSystemLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_SetTimerDelegate"), UKismetSystemLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_ClearAndInvalidateTimerHandle"), UKismetSystemLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("SphereOverlapActors"), UKismetSystemLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("LineTraceSingle"), UKismetSystemLibrary::StaticClass());

		// Gameplay Statics
		WellKnownFunctions.Add(TEXT("SpawnEmitterAtLocation"), UGameplayStatics::StaticClass());
		WellKnownFunctions.Add(TEXT("SpawnSoundAtLocation"), UGameplayStatics::StaticClass());
		WellKnownFunctions.Add(TEXT("ApplyDamage"), UGameplayStatics::StaticClass());
		WellKnownFunctions.Add(TEXT("GetAllActorsOfClass"), UGameplayStatics::StaticClass());
		WellKnownFunctions.Add(TEXT("GetPlayerCharacter"), UGameplayStatics::StaticClass());
		WellKnownFunctions.Add(TEXT("GetPlayerPawn"), UGameplayStatics::StaticClass());

		// v2.7.0: Narrative Pro Inventory functions
		WellKnownFunctions.Add(TEXT("TryAddItemFromClass"), UNarrativeInventoryComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("RemoveItem"), UNarrativeInventoryComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("ConsumeItem"), UNarrativeInventoryComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("FindItemOfClass"), UNarrativeInventoryComponent::StaticClass());
		WellKnownFunctions.Add(TEXT("GetTotalQuantityOfItem"), UNarrativeInventoryComponent::StaticClass());

		// v2.7.0: Narrative Pro Weapon functions
		WellKnownFunctions.Add(TEXT("WieldInSlot"), UWeaponItem::StaticClass());
		WellKnownFunctions.Add(TEXT("IsWielded"), UWeaponItem::StaticClass());
		WellKnownFunctions.Add(TEXT("IsHolstered"), UWeaponItem::StaticClass());

		// v2.7.0: Narrative Pro Equipment functions
		WellKnownFunctions.Add(TEXT("EquipItem"), UEquippableItem::StaticClass());
		WellKnownFunctions.Add(TEXT("UnequipItem"), UEquippableItem::StaticClass());
		WellKnownFunctions.Add(TEXT("IsEquipped"), UEquippableItem::StaticClass());
	}

	// ============================================================
	// STABLE SECTION: Deprecated Function Remapping v2.7.5
	// Verified: 2026-01-11 - DO NOT MODIFY WITHOUT TESTING
	// ============================================================
	// v2.7.4: Deprecated function remapping table - auto-replace old functions with new versions
	static TMap<FString, FString> DeprecatedFunctionRemapping;
	if (DeprecatedFunctionRemapping.Num() == 0)
	{
		// Timer functions
		DeprecatedFunctionRemapping.Add(TEXT("ClearTimerByHandle"), TEXT("K2_ClearAndInvalidateTimerHandle"));
		DeprecatedFunctionRemapping.Add(TEXT("K2_ClearTimer"), TEXT("K2_ClearAndInvalidateTimerHandle"));
		DeprecatedFunctionRemapping.Add(TEXT("ClearTimer"), TEXT("K2_ClearAndInvalidateTimerHandle"));
		DeprecatedFunctionRemapping.Add(TEXT("K2_ClearTimerHandle"), TEXT("K2_ClearAndInvalidateTimerHandle"));
		LogGeneration(TEXT("Initialized deprecated function remapping table"));
	}
	// ============================================================
	// END STABLE SECTION: Deprecated Function Remapping
	// ============================================================

	// Determine the function owner class
	UClass* FunctionOwner = nullptr;
	const FString* ClassNamePtr = NodeDef.Properties.Find(TEXT("class"));

	// v2.4.3: First check the well-known functions table
	FString FunctionNameStr = FunctionName.ToString();

	// v2.7.4: Check for deprecated function and remap if needed
	if (FString* RemappedName = DeprecatedFunctionRemapping.Find(FunctionNameStr))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GasAbilityGenerator] DEPRECATED FUNCTION: '%s' -> using '%s' instead"),
			*FunctionNameStr, **RemappedName);
		LogGeneration(FString::Printf(TEXT("  NOTE: Remapping deprecated '%s' to '%s'"), *FunctionNameStr, **RemappedName));
		FunctionNameStr = *RemappedName;
		FunctionName = FName(**RemappedName);
	}

	// v2.4.4: Try multiple name variants (original, K2_, BP_)
	TArray<FString> FunctionNameVariants;
	FunctionNameVariants.Add(FunctionNameStr);
	FunctionNameVariants.Add(TEXT("K2_") + FunctionNameStr);
	FunctionNameVariants.Add(TEXT("BP_") + FunctionNameStr);

	// First try well-known table with all variants
	for (const FString& NameVariant : FunctionNameVariants)
	{
		if (UClass** FoundClass = WellKnownFunctions.Find(NameVariant))
		{
			FunctionOwner = *FoundClass;
			Function = FunctionOwner->FindFunctionByName(FName(*NameVariant));
			if (Function)
			{
				LogGeneration(FString::Printf(TEXT("Found function '%s' (variant of '%s') in well-known class '%s'"), *NameVariant, *FunctionNameStr, *FunctionOwner->GetName()));
				FunctionName = FName(*NameVariant);  // Update to the found variant
				break;
			}
		}
	}

	// If not in well-known table, try explicit class specification
	if (!Function && ClassNamePtr && !ClassNamePtr->IsEmpty())
	{
		FunctionOwner = FindParentClass(*ClassNamePtr);
		if (FunctionOwner)
		{
			Function = FunctionOwner->FindFunctionByName(FunctionName);
			if (Function)
			{
				LogGeneration(FString::Printf(TEXT("Found function '%s' in specified class '%s'"), *FunctionNameStr, *FunctionOwner->GetName()));
			}
		}
	}

	// If target_self or no class specified, search Blueprint's parent class hierarchy
	if (!Function && (bTargetSelf || !ClassNamePtr || ClassNamePtr->IsEmpty()))
	{
		if (Blueprint && Blueprint->ParentClass)
		{
			// Search up the class hierarchy
			UClass* SearchClass = Blueprint->ParentClass;
			while (SearchClass && !Function)
			{
				Function = SearchClass->FindFunctionByName(FunctionName);
				if (Function)
				{
					FunctionOwner = SearchClass;
					LogGeneration(FString::Printf(TEXT("Found function '%s' in parent class '%s'"), *FunctionNameStr, *FunctionOwner->GetName()));
					break;
				}
				SearchClass = SearchClass->GetSuperClass();
			}
		}
	}

	// If still not found, search common library and engine classes
	if (!Function)
	{
		// v2.4.2: Extended classes to search in priority order
		TArray<UClass*> SearchClasses = {
			UKismetSystemLibrary::StaticClass(),
			UKismetMathLibrary::StaticClass(),
			UGameplayStatics::StaticClass(),
			UGameplayAbility::StaticClass(),
			UAbilitySystemBlueprintLibrary::StaticClass(),
			UAbilitySystemComponent::StaticClass(),
			ACharacter::StaticClass(),
			AActor::StaticClass(),
			APawn::StaticClass(),
			UCharacterMovementComponent::StaticClass(),
			USceneComponent::StaticClass(),
			USkeletalMeshComponent::StaticClass(),
			UPrimitiveComponent::StaticClass()
		};

		LogGeneration(FString::Printf(TEXT("Searching for function '%s' across %d library classes..."), *FunctionName.ToString(), SearchClasses.Num()));

		for (UClass* SearchClass : SearchClasses)
		{
			Function = SearchClass->FindFunctionByName(FunctionName);
			if (Function)
			{
				FunctionOwner = SearchClass;
				LogGeneration(FString::Printf(TEXT("Found function '%s' in class '%s'"), *FunctionName.ToString(), *SearchClass->GetName()));
				break;
			}
		}

		if (!Function)
		{
			LogGeneration(FString::Printf(TEXT("Function '%s' not found in any library class."), *FunctionName.ToString()));

			// Check if this might be a property access instead of a function
			FString FuncStr = FunctionName.ToString();
			bool bMightBePropertyAccess = FuncStr.StartsWith(TEXT("Get")) || FuncStr.StartsWith(TEXT("Set"));

			if (bMightBePropertyAccess)
			{
				// Extract potential property name (e.g., "GetMaxWalkSpeed" -> "MaxWalkSpeed")
				FString PropertyName = FuncStr.Mid(3); // Remove "Get" or "Set"
				LogGeneration(FString::Printf(TEXT("'%s' might be a property access for '%s'. Consider using VariableGet/VariableSet node with target."), *FuncStr, *PropertyName));

				// Check if the property exists on CharacterMovementComponent
				FProperty* Prop = UCharacterMovementComponent::StaticClass()->FindPropertyByName(FName(*PropertyName));
				if (Prop)
				{
					LogGeneration(FString::Printf(TEXT("Found property '%s' on CharacterMovementComponent. Use VariableGet/VariableSet with component target."), *PropertyName));
				}
			}

			// Log relevant functions for debugging
			LogGeneration(TEXT("Relevant functions in KismetMathLibrary:"));
			for (TFieldIterator<UFunction> FuncIt(UKismetMathLibrary::StaticClass()); FuncIt; ++FuncIt)
			{
				FString FuncName = FuncIt->GetName();
				if (FuncName.Contains(TEXT("Multiply")) || FuncName.Contains(FuncStr))
				{
					LogGeneration(FString::Printf(TEXT("  %s"), *FuncName));
				}
			}

			LogGeneration(TEXT("Relevant functions in Character:"));
			for (TFieldIterator<UFunction> FuncIt(ACharacter::StaticClass()); FuncIt; ++FuncIt)
			{
				FString FuncName = FuncIt->GetName();
				if (FuncName.Contains(TEXT("Movement")) || FuncName.Contains(TEXT("Mesh")) || FuncName.Contains(FuncStr))
				{
					LogGeneration(FString::Printf(TEXT("  %s"), *FuncName));
				}
			}
		}
	}

	if (Function)
	{
		CallNode->SetFromFunction(Function);
		CallNode->CreateNewGuid();
		CallNode->PostPlacedNewNode();
		CallNode->AllocateDefaultPins();

		// v2.8.2: Apply parameter defaults from properties with "param." prefix
		for (const auto& PropPair : NodeDef.Properties)
		{
			if (PropPair.Key.StartsWith(TEXT("param.")))
			{
				FString ParamName = PropPair.Key.Mid(6);  // Remove "param." prefix
				FString ParamValue = PropPair.Value;

				// Find the input pin by name
				UEdGraphPin* ParamPin = nullptr;
				for (UEdGraphPin* Pin : CallNode->Pins)
				{
					if (Pin && Pin->Direction == EGPD_Input &&
						(Pin->PinName.ToString() == ParamName ||
						 Pin->PinName.ToString().Replace(TEXT(" "), TEXT("")) == ParamName.Replace(TEXT(" "), TEXT(""))))
					{
						ParamPin = Pin;
						break;
					}
				}

				if (ParamPin)
				{
					// v2.8.2: Handle different pin types
					FString PinTypeStr = ParamPin->PinType.PinCategory.ToString();

					if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Byte &&
						ParamPin->PinType.PinSubCategoryObject.IsValid())
					{
						// This is an enum - need to convert value to enum constant
						UEnum* EnumType = Cast<UEnum>(ParamPin->PinType.PinSubCategoryObject.Get());
						if (EnumType)
						{
							// Try to find the enum value - check both raw name and with enum prefix
							FString EnumValueName = ParamValue;
							int64 EnumValue = EnumType->GetValueByNameString(EnumValueName);

							// If not found, try with common prefixes
							if (EnumValue == INDEX_NONE)
							{
								// Try MOVE_ prefix for EMovementMode
								EnumValue = EnumType->GetValueByNameString(TEXT("MOVE_") + EnumValueName);
							}
							if (EnumValue == INDEX_NONE)
							{
								// Try full qualified name
								EnumValue = EnumType->GetValueByNameString(EnumType->GetNameStringByIndex(0).Left(EnumType->GetNameStringByIndex(0).Find(TEXT("::")) + 2) + EnumValueName);
							}

							if (EnumValue != INDEX_NONE)
							{
								ParamPin->DefaultValue = EnumType->GetNameStringByValue(EnumValue);
								LogGeneration(FString::Printf(TEXT("  Set enum parameter '%s' = '%s' (value %lld)"),
									*ParamName, *ParamPin->DefaultValue, EnumValue));
							}
							else
							{
								LogGeneration(FString::Printf(TEXT("  WARNING: Could not find enum value '%s' in %s. Available values:"),
									*ParamValue, *EnumType->GetName()));
								for (int32 i = 0; i < EnumType->NumEnums() - 1; i++)
								{
									LogGeneration(FString::Printf(TEXT("    - %s"), *EnumType->GetNameStringByIndex(i)));
								}
							}
						}
					}
					else
					{
						// Standard value types - set directly
						ParamPin->DefaultValue = ParamValue;
						LogGeneration(FString::Printf(TEXT("  Set parameter '%s' = '%s'"), *ParamName, *ParamValue));
					}
				}
				else
				{
					LogGeneration(FString::Printf(TEXT("  WARNING: Could not find input pin '%s' on function '%s'. Available pins:"),
						*ParamName, *FunctionName.ToString()));
					for (UEdGraphPin* Pin : CallNode->Pins)
					{
						if (Pin && Pin->Direction == EGPD_Input)
						{
							LogGeneration(FString::Printf(TEXT("    - %s (%s)"), *Pin->PinName.ToString(), *Pin->PinType.PinCategory.ToString()));
						}
					}
				}
			}
		}

		return CallNode;
	}

	// Function not found - check if this is actually a property access
	FString FuncStr = FunctionName.ToString();
	bool bIsGetter = FuncStr.StartsWith(TEXT("Get"));
	bool bIsSetter = FuncStr.StartsWith(TEXT("Set"));

	if (bIsGetter || bIsSetter)
	{
		// Extract property name (e.g., "GetMaxWalkSpeed" -> "MaxWalkSpeed")
		FString PropertyName = FuncStr.Mid(3);

		// Search for property in relevant classes
		TArray<UClass*> PropertySearchClasses = {
			UCharacterMovementComponent::StaticClass(),
			ACharacter::StaticClass(),
			AActor::StaticClass(),
			APawn::StaticClass()
		};

		for (UClass* SearchClass : PropertySearchClasses)
		{
			FProperty* Prop = SearchClass->FindPropertyByName(FName(*PropertyName));
			if (Prop && Prop->HasAnyPropertyFlags(CPF_BlueprintVisible))
			{
				LogGeneration(FString::Printf(TEXT("Function '%s' not found, but property '%s' exists on '%s'. Use PropertyGet/PropertySet node type with target_class: %s"),
					*FuncStr, *PropertyName, *SearchClass->GetName(), *SearchClass->GetName()));
				break;
			}
		}
	}

	// Log warning with suggestions
	LogGeneration(FString::Printf(TEXT("Could not find a function named '%s' in '%s'."),
		*FunctionName.ToString(), TEXT("KismetSystemLibrary")));
	LogGeneration(FString::Printf(TEXT("Make sure '%s' has been compiled for %s"),
		TEXT("KismetSystemLibrary"), *NodeDef.Id));

	// Still create the node with fallback - it may be a custom function
	if (!FunctionOwner)
	{
		FunctionOwner = UKismetSystemLibrary::StaticClass();
	}
	CallNode->FunctionReference.SetExternalMember(FunctionName, FunctionOwner);
	CallNode->CreateNewGuid();
	CallNode->PostPlacedNewNode();
	CallNode->AllocateDefaultPins();

	return CallNode;
}

UK2Node* FEventGraphGenerator::CreateBranchNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef)
{
	UK2Node_IfThenElse* BranchNode = NewObject<UK2Node_IfThenElse>(Graph);
	Graph->AddNode(BranchNode, false, false);

	BranchNode->CreateNewGuid();
	BranchNode->PostPlacedNewNode();
	BranchNode->AllocateDefaultPins();

	return BranchNode;
}

UK2Node* FEventGraphGenerator::CreateVariableGetNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef,
	UBlueprint* Blueprint)
{
	const FString* VarNamePtr = NodeDef.Properties.Find(TEXT("variable_name"));
	if (!VarNamePtr || VarNamePtr->IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("VariableGet node '%s' missing variable_name property"), *NodeDef.Id));
		return nullptr;
	}

	// v2.6.3: Check if target_object is specified - if so, this is an external property get
	const FString* TargetObjectPtr = NodeDef.Properties.Find(TEXT("target_object"));

	UK2Node_VariableGet* GetNode = NewObject<UK2Node_VariableGet>(Graph);
	Graph->AddNode(GetNode, false, false);

	if (TargetObjectPtr && !TargetObjectPtr->IsEmpty())
	{
		// Getting a variable from an external object
		FName TargetVarName = FName(*(*TargetObjectPtr));
		UClass* TargetClass = nullptr;

		for (const FBPVariableDescription& VarDesc : Blueprint->NewVariables)
		{
			if (VarDesc.VarName == TargetVarName)
			{
				if (VarDesc.VarType.PinSubCategoryObject.IsValid())
				{
					TargetClass = Cast<UClass>(VarDesc.VarType.PinSubCategoryObject.Get());
				}
				break;
			}
		}

		if (TargetClass)
		{
			FName PropertyName = FName(*(*VarNamePtr));
			FProperty* Property = TargetClass->FindPropertyByName(PropertyName);
			if (Property)
			{
				GetNode->VariableReference.SetExternalMember(PropertyName, TargetClass);
				LogGeneration(FString::Printf(TEXT("VariableGet '%s': External get '%s' from '%s' via '%s'"),
					*NodeDef.Id, **VarNamePtr, *TargetClass->GetName(), **TargetObjectPtr));
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("VariableGet '%s': Property '%s' not found on '%s', falling back to self"),
					*NodeDef.Id, **VarNamePtr, *TargetClass->GetName()));
				GetNode->VariableReference.SetSelfMember(FName(*(*VarNamePtr)));
			}
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("VariableGet '%s': Could not resolve class for target '%s', falling back to self"),
				*NodeDef.Id, **TargetObjectPtr));
			GetNode->VariableReference.SetSelfMember(FName(*(*VarNamePtr)));
		}
	}
	else
	{
		GetNode->VariableReference.SetSelfMember(FName(*(*VarNamePtr)));
	}

	GetNode->CreateNewGuid();
	GetNode->PostPlacedNewNode();
	GetNode->AllocateDefaultPins();

	return GetNode;
}

UK2Node* FEventGraphGenerator::CreateVariableSetNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef,
	UBlueprint* Blueprint)
{
	const FString* VarNamePtr = NodeDef.Properties.Find(TEXT("variable_name"));
	if (!VarNamePtr || VarNamePtr->IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("VariableSet node '%s' missing variable_name property"), *NodeDef.Id));
		return nullptr;
	}

	// v2.6.3: Check if target_object is specified - if so, this is an external property set
	const FString* TargetObjectPtr = NodeDef.Properties.Find(TEXT("target_object"));

	UK2Node_VariableSet* SetNode = NewObject<UK2Node_VariableSet>(Graph);
	Graph->AddNode(SetNode, false, false);

	if (TargetObjectPtr && !TargetObjectPtr->IsEmpty())
	{
		// Setting a variable on an external object - need to find the target class
		// The target_object references a variable that holds the object reference
		// We need to find what class that variable is, then set up external member reference

		// First, try to find the variable type from the blueprint
		FName TargetVarName = FName(*(*TargetObjectPtr));
		UClass* TargetClass = nullptr;

		for (const FBPVariableDescription& VarDesc : Blueprint->NewVariables)
		{
			if (VarDesc.VarName == TargetVarName)
			{
				// Found the variable - get its class
				if (VarDesc.VarType.PinSubCategoryObject.IsValid())
				{
					TargetClass = Cast<UClass>(VarDesc.VarType.PinSubCategoryObject.Get());
				}
				break;
			}
		}

		if (TargetClass)
		{
			// Verify property exists on target class
			FName PropertyName = FName(*(*VarNamePtr));
			FProperty* Property = TargetClass->FindPropertyByName(PropertyName);
			if (Property)
			{
				SetNode->VariableReference.SetExternalMember(PropertyName, TargetClass);
				LogGeneration(FString::Printf(TEXT("VariableSet '%s': External set '%s' on '%s' via '%s'"),
					*NodeDef.Id, **VarNamePtr, *TargetClass->GetName(), **TargetObjectPtr));
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("VariableSet '%s': Property '%s' not found on '%s', falling back to self"),
					*NodeDef.Id, **VarNamePtr, *TargetClass->GetName()));
				SetNode->VariableReference.SetSelfMember(FName(*(*VarNamePtr)));
			}
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("VariableSet '%s': Could not resolve class for target '%s', falling back to self"),
				*NodeDef.Id, **TargetObjectPtr));
			SetNode->VariableReference.SetSelfMember(FName(*(*VarNamePtr)));
		}
	}
	else
	{
		// Normal self member set
		SetNode->VariableReference.SetSelfMember(FName(*(*VarNamePtr)));
	}

	SetNode->CreateNewGuid();
	SetNode->PostPlacedNewNode();
	SetNode->AllocateDefaultPins();

	return SetNode;
}

// v2.4.2: PropertyGet - access properties on external objects
UK2Node* FEventGraphGenerator::CreatePropertyGetNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef,
	UBlueprint* Blueprint)
{
	const FString* PropertyNamePtr = NodeDef.Properties.Find(TEXT("property_name"));
	if (!PropertyNamePtr || PropertyNamePtr->IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("PropertyGet node '%s' missing property_name property"), *NodeDef.Id));
		return nullptr;
	}

	const FString* TargetClassPtr = NodeDef.Properties.Find(TEXT("target_class"));
	if (!TargetClassPtr || TargetClassPtr->IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("PropertyGet node '%s' missing target_class property"), *NodeDef.Id));
		return nullptr;
	}

	// Find the target class
	UClass* TargetClass = FindParentClass(*TargetClassPtr);
	if (!TargetClass)
	{
		LogGeneration(FString::Printf(TEXT("PropertyGet node '%s': Could not find class '%s'"), *NodeDef.Id, **TargetClassPtr));
		// v2.6.7: Track missing dependency for deferred generation
		FString DepType = TEXT("ActorBlueprint");
		if (TargetClassPtr->StartsWith(TEXT("WBP_"))) DepType = TEXT("WidgetBlueprint");
		else if (TargetClassPtr->StartsWith(TEXT("GE_"))) DepType = TEXT("GameplayEffect");
		else if (TargetClassPtr->StartsWith(TEXT("GA_"))) DepType = TEXT("GameplayAbility");
		AddMissingDependency(*TargetClassPtr, DepType,
			FString::Printf(TEXT("PropertyGet node '%s'"), *NodeDef.Id));
		return nullptr;
	}

	// Verify property exists on target class
	FName PropertyName = FName(*(*PropertyNamePtr));
	FProperty* Property = TargetClass->FindPropertyByName(PropertyName);
	if (!Property)
	{
		LogGeneration(FString::Printf(TEXT("PropertyGet node '%s': Property '%s' not found on class '%s'"),
			*NodeDef.Id, **PropertyNamePtr, *TargetClass->GetName()));
		// v2.6.7: Property missing on existing class - this could mean the class needs regeneration
		AddMissingDependency(*TargetClassPtr, TEXT("ActorBlueprint"),
			FString::Printf(TEXT("Property '%s' missing on class - may need regeneration"), **PropertyNamePtr));
		return nullptr;
	}

	UK2Node_VariableGet* GetNode = NewObject<UK2Node_VariableGet>(Graph);
	Graph->AddNode(GetNode, false, false);

	// Set up external member reference to the property
	GetNode->VariableReference.SetExternalMember(PropertyName, TargetClass);
	GetNode->CreateNewGuid();
	GetNode->PostPlacedNewNode();
	GetNode->AllocateDefaultPins();

	LogGeneration(FString::Printf(TEXT("PropertyGet node '%s': Created getter for '%s' on '%s'"),
		*NodeDef.Id, **PropertyNamePtr, *TargetClass->GetName()));

	return GetNode;
}

// v2.4.2: PropertySet - set properties on external objects
UK2Node* FEventGraphGenerator::CreatePropertySetNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef,
	UBlueprint* Blueprint)
{
	const FString* PropertyNamePtr = NodeDef.Properties.Find(TEXT("property_name"));
	if (!PropertyNamePtr || PropertyNamePtr->IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("PropertySet node '%s' missing property_name property"), *NodeDef.Id));
		return nullptr;
	}

	const FString* TargetClassPtr = NodeDef.Properties.Find(TEXT("target_class"));
	if (!TargetClassPtr || TargetClassPtr->IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("PropertySet node '%s' missing target_class property"), *NodeDef.Id));
		return nullptr;
	}

	// Find the target class
	UClass* TargetClass = FindParentClass(*TargetClassPtr);
	if (!TargetClass)
	{
		LogGeneration(FString::Printf(TEXT("PropertySet node '%s': Could not find class '%s'"), *NodeDef.Id, **TargetClassPtr));
		return nullptr;
	}

	// Verify property exists on target class
	FName PropertyName = FName(*(*PropertyNamePtr));
	FProperty* Property = TargetClass->FindPropertyByName(PropertyName);
	if (!Property)
	{
		LogGeneration(FString::Printf(TEXT("PropertySet node '%s': Property '%s' not found on class '%s'"),
			*NodeDef.Id, **PropertyNamePtr, *TargetClass->GetName()));
		return nullptr;
	}

	UK2Node_VariableSet* SetNode = NewObject<UK2Node_VariableSet>(Graph);
	Graph->AddNode(SetNode, false, false);

	// Set up external member reference to the property
	SetNode->VariableReference.SetExternalMember(PropertyName, TargetClass);
	SetNode->CreateNewGuid();
	SetNode->PostPlacedNewNode();
	SetNode->AllocateDefaultPins();

	LogGeneration(FString::Printf(TEXT("PropertySet node '%s': Created setter for '%s' on '%s'"),
		*NodeDef.Id, **PropertyNamePtr, *TargetClass->GetName()));

	return SetNode;
}

// v2.7.0: BreakStruct - break a struct into individual member pins
UK2Node* FEventGraphGenerator::CreateBreakStructNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef)
{
	const FString* StructTypePtr = NodeDef.Properties.Find(TEXT("struct_type"));
	if (!StructTypePtr || StructTypePtr->IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("BreakStruct node '%s' missing struct_type property"), *NodeDef.Id));
		return nullptr;
	}

	// Find the struct - try multiple lookup methods
	UScriptStruct* Struct = nullptr;

	// v2.7.0: Well-known structs lookup table (USTRUCTs use StaticStruct(), not FindObject)
	static TMap<FString, UScriptStruct*> WellKnownStructs;
	if (WellKnownStructs.Num() == 0)
	{
		// Narrative Pro structs
		WellKnownStructs.Add(TEXT("FItemAddResult"), FItemAddResult::StaticStruct());
		// Add more structs as needed
		LogGeneration(TEXT("BreakStruct: Initialized well-known structs table"));
	}

	// First check well-known structs (most reliable for USTRUCTs)
	if (UScriptStruct** FoundStruct = WellKnownStructs.Find(*StructTypePtr))
	{
		Struct = *FoundStruct;
		LogGeneration(FString::Printf(TEXT("BreakStruct: Found struct '%s' in well-known table"), *Struct->GetName()));
	}

	// Fallback: try FindObject methods
	if (!Struct)
	{
		Struct = FindObject<UScriptStruct>(nullptr, **StructTypePtr);
	}
	if (!Struct)
	{
		// Try with /Script/ prefix variants
		TArray<FString> StructSearchPaths;
		StructSearchPaths.Add(FString::Printf(TEXT("/Script/NarrativeArsenal.%s"), **StructTypePtr));
		StructSearchPaths.Add(FString::Printf(TEXT("/Script/Engine.%s"), **StructTypePtr));
		StructSearchPaths.Add(FString::Printf(TEXT("/Script/CoreUObject.%s"), **StructTypePtr));
		StructSearchPaths.Add(FString::Printf(TEXT("/Script/GameplayAbilities.%s"), **StructTypePtr));

		for (const FString& Path : StructSearchPaths)
		{
			Struct = FindObject<UScriptStruct>(nullptr, *Path);
			if (Struct)
			{
				LogGeneration(FString::Printf(TEXT("BreakStruct: Found struct at path '%s'"), *Path));
				break;
			}
		}
	}

	if (!Struct)
	{
		LogGeneration(FString::Printf(TEXT("BreakStruct node '%s': Could not find struct '%s'"), *NodeDef.Id, **StructTypePtr));
		return nullptr;
	}

	// Note: CanBeBroken check removed due to linker issues - let node creation handle validation

	UK2Node_BreakStruct* BreakNode = NewObject<UK2Node_BreakStruct>(Graph);
	BreakNode->StructType = Struct;
	Graph->AddNode(BreakNode, false, false);
	BreakNode->CreateNewGuid();
	BreakNode->PostPlacedNewNode();
	BreakNode->AllocateDefaultPins();

	LogGeneration(FString::Printf(TEXT("BreakStruct node '%s': Created for struct '%s'"), *NodeDef.Id, *Struct->GetName()));

	return BreakNode;
}

// v2.7.0: MakeArray - create an array from individual elements
UK2Node* FEventGraphGenerator::CreateMakeArrayNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef)
{
	UK2Node_MakeArray* ArrayNode = NewObject<UK2Node_MakeArray>(Graph);
	Graph->AddNode(ArrayNode, false, false);

	// Get number of inputs (default 1)
	const FString* NumInputsPtr = NodeDef.Properties.Find(TEXT("num_inputs"));
	if (NumInputsPtr)
	{
		ArrayNode->NumInputs = FCString::Atoi(**NumInputsPtr);
	}
	else
	{
		ArrayNode->NumInputs = 1;
	}

	ArrayNode->CreateNewGuid();
	ArrayNode->PostPlacedNewNode();
	ArrayNode->AllocateDefaultPins();

	LogGeneration(FString::Printf(TEXT("MakeArray node '%s': Created with %d inputs"), *NodeDef.Id, ArrayNode->NumInputs));

	return ArrayNode;
}

// v2.7.0: GetArrayItem - access an element at a specific index
UK2Node* FEventGraphGenerator::CreateGetArrayItemNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef)
{
	UK2Node_GetArrayItem* GetItemNode = NewObject<UK2Node_GetArrayItem>(Graph);
	Graph->AddNode(GetItemNode, false, false);

	GetItemNode->CreateNewGuid();
	GetItemNode->PostPlacedNewNode();
	GetItemNode->AllocateDefaultPins();

	LogGeneration(FString::Printf(TEXT("GetArrayItem node '%s': Created"), *NodeDef.Id));

	return GetItemNode;
}

// v2.7.8: Self - reference to the blueprint self
UK2Node* FEventGraphGenerator::CreateSelfNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef)
{
	UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(Graph);
	Graph->AddNode(SelfNode, false, false);

	SelfNode->CreateNewGuid();
	SelfNode->PostPlacedNewNode();
	SelfNode->AllocateDefaultPins();

	LogGeneration(FString::Printf(TEXT("Self node '%s': Created"), *NodeDef.Id));

	return SelfNode;
}

UK2Node* FEventGraphGenerator::CreateSequenceNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef)
{
	UK2Node_ExecutionSequence* SeqNode = NewObject<UK2Node_ExecutionSequence>(Graph);
	Graph->AddNode(SeqNode, false, false);

	SeqNode->CreateNewGuid();
	SeqNode->PostPlacedNewNode();
	SeqNode->AllocateDefaultPins();

	// Check if we need more output pins
	const FString* NumOutputsPtr = NodeDef.Properties.Find(TEXT("num_outputs"));
	if (NumOutputsPtr)
	{
		int32 NumOutputs = FCString::Atoi(**NumOutputsPtr);
		// Count current output pins (execution pins going out)
		int32 CurrentOutputs = 0;
		for (UEdGraphPin* Pin : SeqNode->Pins)
		{
			if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				CurrentOutputs++;
			}
		}
		// Sequence starts with 2 outputs, add more if needed
		while (CurrentOutputs < NumOutputs)
		{
			SeqNode->AddInputPin();
			CurrentOutputs++;
		}
	}

	return SeqNode;
}

UK2Node* FEventGraphGenerator::CreateDelayNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef)
{
	UK2Node_CallFunction* DelayNode = NewObject<UK2Node_CallFunction>(Graph);
	Graph->AddNode(DelayNode, false, false);

	// Use Delay function from UKismetSystemLibrary
	UFunction* DelayFunction = UKismetSystemLibrary::StaticClass()->FindFunctionByName(FName("Delay"));
	if (DelayFunction)
	{
		DelayNode->SetFromFunction(DelayFunction);
	}

	DelayNode->CreateNewGuid();
	DelayNode->PostPlacedNewNode();
	DelayNode->AllocateDefaultPins();

	// Set duration if specified
	const FString* DurationPtr = NodeDef.Properties.Find(TEXT("duration"));
	if (DurationPtr)
	{
		UEdGraphPin* DurationPin = DelayNode->FindPin(FName("Duration"));
		if (DurationPin)
		{
			DurationPin->DefaultValue = *DurationPtr;
		}
	}

	return DelayNode;
}

UK2Node* FEventGraphGenerator::CreatePrintStringNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef)
{
	UK2Node_CallFunction* PrintNode = NewObject<UK2Node_CallFunction>(Graph);
	Graph->AddNode(PrintNode, false, false);

	// Use PrintString from UKismetSystemLibrary
	UFunction* PrintFunction = UKismetSystemLibrary::StaticClass()->FindFunctionByName(FName("PrintString"));
	if (PrintFunction)
	{
		PrintNode->SetFromFunction(PrintFunction);
	}

	PrintNode->CreateNewGuid();
	PrintNode->PostPlacedNewNode();
	PrintNode->AllocateDefaultPins();

	// Set message if specified
	const FString* MessagePtr = NodeDef.Properties.Find(TEXT("message"));
	if (MessagePtr)
	{
		UEdGraphPin* InStringPin = PrintNode->FindPin(FName("InString"));
		if (InStringPin)
		{
			InStringPin->DefaultValue = *MessagePtr;
		}
	}

	// Set duration if specified
	const FString* DurationPtr = NodeDef.Properties.Find(TEXT("duration"));
	if (DurationPtr)
	{
		UEdGraphPin* DurationPin = PrintNode->FindPin(FName("Duration"));
		if (DurationPin)
		{
			DurationPin->DefaultValue = *DurationPtr;
		}
	}

	return PrintNode;
}

UK2Node* FEventGraphGenerator::CreateDynamicCastNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef)
{
	const FString* TargetClassPtr = NodeDef.Properties.Find(TEXT("target_class"));
	if (!TargetClassPtr || TargetClassPtr->IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("DynamicCast node '%s' missing target_class property"), *NodeDef.Id));
		return nullptr;
	}

	UK2Node_DynamicCast* CastNode = NewObject<UK2Node_DynamicCast>(Graph);
	Graph->AddNode(CastNode, false, false);

	// Find target class - v2.4.1: Enhanced with better logging
	UClass* TargetClass = FindParentClass(*TargetClassPtr);
	if (TargetClass)
	{
		CastNode->TargetType = TargetClass;
	}
	else
	{
		LogGeneration(FString::Printf(TEXT("Warning: DynamicCast node '%s' could not find target class '%s'. Cast may not work correctly."),
			*NodeDef.Id, **TargetClassPtr));
		// v2.6.7: Track missing dependency for deferred generation
		FString DepType = TEXT("ActorBlueprint");
		if (TargetClassPtr->StartsWith(TEXT("BP_"))) DepType = TEXT("ActorBlueprint");
		else if (TargetClassPtr->StartsWith(TEXT("WBP_"))) DepType = TEXT("WidgetBlueprint");
		AddMissingDependency(*TargetClassPtr, DepType,
			FString::Printf(TEXT("DynamicCast node '%s'"), *NodeDef.Id));
		// Still create the node, pins will be created but cast won't work until class is found
	}

	CastNode->CreateNewGuid();
	CastNode->PostPlacedNewNode();
	CastNode->AllocateDefaultPins();

	return CastNode;
}

// v2.4.0: Create ForEachLoop macro instance node
UK2Node* FEventGraphGenerator::CreateForEachLoopNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef)
{
	// ForEachLoop is implemented as a macro in UE5
	// We use UK2Node_MacroInstance to instantiate it
	UK2Node_MacroInstance* MacroNode = NewObject<UK2Node_MacroInstance>(Graph);
	Graph->AddNode(MacroNode, false, false);

	// Find the ForEachLoop macro from the standard macro library
	static UBlueprint* MacroLibrary = nullptr;
	static UEdGraph* ForEachLoopMacro = nullptr;

	if (!MacroLibrary)
	{
		// Load the standard macro library
		MacroLibrary = LoadObject<UBlueprint>(nullptr, TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros"));
	}

	if (MacroLibrary && !ForEachLoopMacro)
	{
		// Find ForEachLoop macro
		for (UEdGraph* MacroGraph : MacroLibrary->MacroGraphs)
		{
			if (MacroGraph && MacroGraph->GetFName() == FName(TEXT("ForEachLoop")))
			{
				ForEachLoopMacro = MacroGraph;
				break;
			}
		}
	}

	if (ForEachLoopMacro)
	{
		MacroNode->SetMacroGraph(ForEachLoopMacro);
	}
	else
	{
		LogGeneration(FString::Printf(TEXT("Warning: ForEachLoop macro not found for node '%s'"), *NodeDef.Id));
	}

	MacroNode->CreateNewGuid();
	MacroNode->PostPlacedNewNode();
	MacroNode->AllocateDefaultPins();

	return MacroNode;
}

// v2.4.0: Create SpawnActorFromClass node using CallFunction (more reliable than UK2Node_SpawnActorFromClass)
UK2Node* FEventGraphGenerator::CreateSpawnActorNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef,
	UBlueprint* Blueprint)
{
	// Use CallFunction to call BeginDeferredActorSpawnFromClass which is more reliable
	// than creating UK2Node_SpawnActorFromClass directly
	UK2Node_CallFunction* CallNode = NewObject<UK2Node_CallFunction>(Graph);
	Graph->AddNode(CallNode, false, false);

	// Use UGameplayStatics::BeginDeferredActorSpawnFromClass
	UFunction* SpawnFunc = UGameplayStatics::StaticClass()->FindFunctionByName(
		FName(TEXT("BeginDeferredActorSpawnFromClass")));

	if (SpawnFunc)
	{
		CallNode->SetFromFunction(SpawnFunc);
	}
	else
	{
		// Fallback to setting function reference manually
		CallNode->FunctionReference.SetExternalMember(
			FName(TEXT("BeginDeferredActorSpawnFromClass")),
			UGameplayStatics::StaticClass());
	}

	CallNode->CreateNewGuid();
	CallNode->PostPlacedNewNode();
	CallNode->AllocateDefaultPins();

	// If actor_class_variable is specified, we'll need to connect it later via connections
	// The Class pin will be connected to a variable get node
	const FString* ActorClassVar = NodeDef.Properties.Find(TEXT("actor_class_variable"));
	if (ActorClassVar && !ActorClassVar->IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("  SpawnActor node '%s' will use class from variable '%s'"),
			*NodeDef.Id, **ActorClassVar));
	}

	return CallNode;
}

bool FEventGraphGenerator::ConnectPins(
	const TMap<FString, UK2Node*>& NodeMap,
	const FManifestGraphConnectionDefinition& Connection)
{
	// Find source node
	UK2Node* const* FromNodePtr = NodeMap.Find(Connection.From.NodeId);
	if (!FromNodePtr || !(*FromNodePtr))
	{
		LogGeneration(FString::Printf(TEXT("Connection failed: Source node '%s' not found"),
			*Connection.From.NodeId));
		return false;
	}

	// Find target node
	UK2Node* const* ToNodePtr = NodeMap.Find(Connection.To.NodeId);
	if (!ToNodePtr || !(*ToNodePtr))
	{
		LogGeneration(FString::Printf(TEXT("Connection failed: Target node '%s' not found"),
			*Connection.To.NodeId));
		return false;
	}

	UK2Node* FromNode = *FromNodePtr;
	UK2Node* ToNode = *ToNodePtr;

	// v2.5.2: Check if this is an exec pin connection on pure nodes
	// Pure nodes (like UK2Node_VariableGet, IsValid, math operations) don't have exec pins
	bool bIsExecConnection = Connection.From.PinName.Equals(TEXT("Exec"), ESearchCase::IgnoreCase) ||
	                         Connection.From.PinName.Equals(TEXT("Then"), ESearchCase::IgnoreCase) ||
	                         Connection.To.PinName.Equals(TEXT("Exec"), ESearchCase::IgnoreCase) ||
	                         Connection.To.PinName.Equals(TEXT("Execute"), ESearchCase::IgnoreCase);

	if (bIsExecConnection)
	{
		// Check if either node is a pure node (no exec pins)
		bool bFromNodeIsPure = FromNode->IsNodePure();
		bool bToNodeIsPure = ToNode->IsNodePure();

		if (bFromNodeIsPure || bToNodeIsPure)
		{
			// v2.5.2: Skip exec connections on pure nodes - they'll be handled by the rerouting logic
			// in GenerateEventGraph which creates direct connections that bypass pure nodes
			LogGeneration(FString::Printf(TEXT("    SKIP exec on pure node: %s.%s -> %s.%s"),
				*Connection.From.NodeId, *Connection.From.PinName,
				*Connection.To.NodeId, *Connection.To.PinName));
			return true; // Return true - this is expected, not a failure
		}
	}

	// Find source pin (output)
	UEdGraphPin* FromPin = FindPinByName(FromNode, Connection.From.PinName, EGPD_Output);
	if (!FromPin)
	{
		// List available output pins for debugging
		FString AvailablePins;
		for (UEdGraphPin* Pin : FromNode->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output)
			{
				if (!AvailablePins.IsEmpty()) AvailablePins += TEXT(", ");
				AvailablePins += Pin->PinName.ToString();
			}
		}
		LogGeneration(FString::Printf(TEXT("Connection failed: Output pin '%s' not found on node '%s'. Available outputs: [%s]"),
			*Connection.From.PinName, *Connection.From.NodeId, *AvailablePins));
		return false;
	}

	// Find target pin (input)
	UEdGraphPin* ToPin = FindPinByName(ToNode, Connection.To.PinName, EGPD_Input);
	if (!ToPin)
	{
		// List available input pins for debugging
		FString AvailablePins;
		for (UEdGraphPin* Pin : ToNode->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Input)
			{
				if (!AvailablePins.IsEmpty()) AvailablePins += TEXT(", ");
				AvailablePins += Pin->PinName.ToString();
			}
		}
		LogGeneration(FString::Printf(TEXT("Connection failed: Input pin '%s' not found on node '%s'. Available inputs: [%s]"),
			*Connection.To.PinName, *Connection.To.NodeId, *AvailablePins));
		return false;
	}

	// v2.7.3: Helper lambda to get full pin type description including class names
	auto GetPinTypeDescription = [](UEdGraphPin* Pin) -> FString
	{
		if (!Pin) return TEXT("null");

		FString TypeDesc = Pin->PinType.PinCategory.ToString();

		// For object types, include the actual class name
		if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object ||
		    Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class ||
		    Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Interface ||
		    Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_SoftObject ||
		    Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_SoftClass)
		{
			if (Pin->PinType.PinSubCategoryObject.IsValid())
			{
				TypeDesc += FString::Printf(TEXT("<%s>"), *Pin->PinType.PinSubCategoryObject->GetName());
			}
			else if (!Pin->PinType.PinSubCategory.IsNone())
			{
				TypeDesc += FString::Printf(TEXT("<%s>"), *Pin->PinType.PinSubCategory.ToString());
			}
		}

		// Show if it's a reference or array
		if (Pin->PinType.bIsReference) TypeDesc += TEXT("&");
		if (Pin->PinType.IsArray()) TypeDesc += TEXT("[]");

		return TypeDesc;
	};

	// v2.7.3: Enhanced connection logging with full type information
	FString FromTypeDesc = GetPinTypeDescription(FromPin);
	FString ToTypeDesc = GetPinTypeDescription(ToPin);

	LogGeneration(FString::Printf(TEXT("    Connecting: %s.%s [%s] -> %s.%s [%s]"),
		*Connection.From.NodeId, *FromPin->PinName.ToString(), *FromTypeDesc,
		*Connection.To.NodeId, *ToPin->PinName.ToString(), *ToTypeDesc));

	// Check if pins are already connected
	if (FromPin->LinkedTo.Contains(ToPin))
	{
		LogGeneration(TEXT("      Already connected"));
		return true;
	}

	// v2.7.3: Check pin type compatibility BEFORE attempting connection
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	if (K2Schema)
	{
		FPinConnectionResponse Response = K2Schema->CanCreateConnection(FromPin, ToPin);

		// v2.7.3: Detailed handling based on response type
		if (Response.Response == CONNECT_RESPONSE_DISALLOW)
		{
			// This is a hard error - types are incompatible
			UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] PIN TYPE ERROR: %s.%s -> %s.%s"),
				*Connection.From.NodeId, *Connection.From.PinName,
				*Connection.To.NodeId, *Connection.To.PinName);
			UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator]   From: %s [%s]"),
				*FromPin->PinName.ToString(), *FromTypeDesc);
			UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator]   To: %s [%s]"),
				*ToPin->PinName.ToString(), *ToTypeDesc);
			UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator]   Reason: %s"),
				*Response.Message.ToString());

			LogGeneration(FString::Printf(TEXT("      ERROR: Can't connect pins - %s"), *Response.Message.ToString()));
			LogGeneration(FString::Printf(TEXT("        '%s' is not compatible with '%s'"), *FromTypeDesc, *ToTypeDesc));
			return false;
		}
		else if (Response.Response == CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE)
		{
			// Needs conversion - log as warning but allow
			LogGeneration(FString::Printf(TEXT("      NOTE: Connection requires conversion: %s"), *Response.Message.ToString()));
		}
		else if (Response.Response == CONNECT_RESPONSE_BREAK_OTHERS_A ||
		         Response.Response == CONNECT_RESPONSE_BREAK_OTHERS_B ||
		         Response.Response == CONNECT_RESPONSE_BREAK_OTHERS_AB)
		{
			// Will break other connections - log it
			LogGeneration(FString::Printf(TEXT("      NOTE: Will break existing connections: %s"), *Response.Message.ToString()));
		}
	}

	// Make the connection
	FromPin->MakeLinkTo(ToPin);

	// v2.5.2: Verify the connection was actually made
	bool bConnectionSuccess = FromPin->LinkedTo.Contains(ToPin);
	if (!bConnectionSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] CONNECTION FAILED: %s.%s [%s] -> %s.%s [%s]"),
			*Connection.From.NodeId, *FromPin->PinName.ToString(), *FromTypeDesc,
			*Connection.To.NodeId, *ToPin->PinName.ToString(), *ToTypeDesc);
		LogGeneration(FString::Printf(TEXT("      ERROR: MakeLinkTo failed - connection not established")));
		return false;
	}

	// v2.5.4: Notify nodes about the connection change
	// This is critical for DynamicCast nodes to update their internal state
	// when the Object input pin is connected
	FromNode->NotifyPinConnectionListChanged(FromPin);
	ToNode->NotifyPinConnectionListChanged(ToPin);

	return true;
}

UEdGraphPin* FEventGraphGenerator::FindPinByName(
	UK2Node* Node,
	const FString& PinName,
	EEdGraphPinDirection Direction)
{
	if (!Node)
	{
		return nullptr;
	}

	// v2.4.1: Enhanced pin name resolution with comprehensive aliases

	// Build a list of possible names to search for
	TArray<FString> SearchNames;
	SearchNames.Add(PinName);

	// Handle common aliases - add all possibilities
	if (PinName.Equals(TEXT("Exec"), ESearchCase::IgnoreCase) ||
	    PinName.Equals(TEXT("Execute"), ESearchCase::IgnoreCase))
	{
		SearchNames.Add(UEdGraphSchema_K2::PN_Execute.ToString());
		SearchNames.Add(TEXT("execute"));
	}
	else if (PinName.Equals(TEXT("Then"), ESearchCase::IgnoreCase))
	{
		SearchNames.Add(UEdGraphSchema_K2::PN_Then.ToString());
		SearchNames.Add(TEXT("then"));
	}
	else if (PinName.Equals(TEXT("ReturnValue"), ESearchCase::IgnoreCase) ||
	         PinName.Equals(TEXT("Return"), ESearchCase::IgnoreCase) ||
	         PinName.Equals(TEXT("Result"), ESearchCase::IgnoreCase))
	{
		SearchNames.Add(UEdGraphSchema_K2::PN_ReturnValue.ToString());
		SearchNames.Add(TEXT("ReturnValue"));
	}
	else if (PinName.Equals(TEXT("Target"), ESearchCase::IgnoreCase) ||
	         PinName.Equals(TEXT("Self"), ESearchCase::IgnoreCase))
	{
		SearchNames.Add(UEdGraphSchema_K2::PN_Self.ToString());
		SearchNames.Add(TEXT("self"));
		SearchNames.Add(TEXT("Target"));
	}
	else if (PinName.Equals(TEXT("Object"), ESearchCase::IgnoreCase) ||
	         PinName.Equals(TEXT("InputObject"), ESearchCase::IgnoreCase))
	{
		// v2.5.2: Enhanced Object pin aliases for IsValid, Cast, etc.
		SearchNames.Add(TEXT("Object"));
		SearchNames.Add(TEXT("InObject"));
		SearchNames.Add(TEXT("InputObject"));
	}
	else if (PinName.Equals(TEXT("Value"), ESearchCase::IgnoreCase))
	{
		// For VariableSet nodes
		SearchNames.Add(TEXT("Value"));
		// The actual pin name might be the variable name
	}
	else if (PinName.Equals(TEXT("Condition"), ESearchCase::IgnoreCase))
	{
		SearchNames.Add(TEXT("Condition"));
		SearchNames.Add(TEXT("bCondition"));
	}
	// v2.5.4: Branch node output pin aliases (True/False -> then/else)
	else if (PinName.Equals(TEXT("True"), ESearchCase::IgnoreCase))
	{
		SearchNames.Add(TEXT("then"));
		SearchNames.Add(UEdGraphSchema_K2::PN_Then.ToString());
	}
	else if (PinName.Equals(TEXT("False"), ESearchCase::IgnoreCase))
	{
		SearchNames.Add(TEXT("else"));
		SearchNames.Add(UEdGraphSchema_K2::PN_Else.ToString());
	}
	// v2.5.2: AttachActorToComponent specific pins
	else if (PinName.Equals(TEXT("Parent"), ESearchCase::IgnoreCase))
	{
		SearchNames.Add(TEXT("Parent"));
		SearchNames.Add(TEXT("InParent"));
	}
	else if (PinName.Equals(TEXT("SocketName"), ESearchCase::IgnoreCase) ||
	         PinName.Equals(TEXT("Socket Name"), ESearchCase::IgnoreCase))
	{
		SearchNames.Add(TEXT("SocketName"));
		SearchNames.Add(TEXT("Socket Name"));
		SearchNames.Add(TEXT("InSocketName"));
	}
	// v2.5.2: Mesh/Component output pins
	else if (PinName.Equals(TEXT("Mesh"), ESearchCase::IgnoreCase))
	{
		SearchNames.Add(TEXT("Mesh"));
		SearchNames.Add(TEXT("ReturnValue"));  // GetMesh returns via ReturnValue
	}

	// DynamicCast specific pins
	if (PinName.Equals(TEXT("As"), ESearchCase::IgnoreCase) ||
	    PinName.Equals(TEXT("AsOutput"), ESearchCase::IgnoreCase) ||
	    PinName.Equals(TEXT("CastOutput"), ESearchCase::IgnoreCase))
	{
		// DynamicCast output pin is named after the target class
		// We'll handle this with partial matching below
	}
	else if (PinName.Equals(TEXT("CastFailed"), ESearchCase::IgnoreCase) ||
	         PinName.Equals(TEXT("Failed"), ESearchCase::IgnoreCase))
	{
		SearchNames.Add(TEXT("CastFailed"));
	}

	// ForEachLoop pins
	if (PinName.Equals(TEXT("LoopBody"), ESearchCase::IgnoreCase))
	{
		SearchNames.Add(TEXT("LoopBody"));
		SearchNames.Add(TEXT("Loop Body"));
	}
	else if (PinName.Equals(TEXT("ArrayElement"), ESearchCase::IgnoreCase) ||
	         PinName.Equals(TEXT("Element"), ESearchCase::IgnoreCase))
	{
		SearchNames.Add(TEXT("Array Element"));
		SearchNames.Add(TEXT("ArrayElement"));
	}
	else if (PinName.Equals(TEXT("ArrayIndex"), ESearchCase::IgnoreCase) ||
	         PinName.Equals(TEXT("Index"), ESearchCase::IgnoreCase))
	{
		SearchNames.Add(TEXT("Array Index"));
		SearchNames.Add(TEXT("ArrayIndex"));
	}
	else if (PinName.Equals(TEXT("Array"), ESearchCase::IgnoreCase))
	{
		SearchNames.Add(TEXT("Array"));
		SearchNames.Add(TEXT("InArray"));
	}

	// Sequence pins
	if (PinName.StartsWith(TEXT("Then_"), ESearchCase::IgnoreCase))
	{
		// Extract the index number
		FString IndexStr = PinName.Mid(5);
		int32 Index = FCString::Atoi(*IndexStr);
		SearchNames.Add(FString::Printf(TEXT("then_%d"), Index));
	}

	// First pass: try exact matches for all search names
	for (const FString& SearchName : SearchNames)
	{
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == Direction)
			{
				if (Pin->PinName.ToString().Equals(SearchName, ESearchCase::IgnoreCase))
				{
					return Pin;
				}
			}
		}
	}

	// Second pass: try partial/contains matching
	for (const FString& SearchName : SearchNames)
	{
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == Direction)
			{
				FString PinNameStr = Pin->PinName.ToString();
				// Check if pin name contains the search name or vice versa
				if (PinNameStr.Contains(SearchName, ESearchCase::IgnoreCase) ||
				    SearchName.Contains(PinNameStr, ESearchCase::IgnoreCase))
				{
					return Pin;
				}
			}
		}
	}

	// Third pass: DynamicCast output - v2.5.3: AsBP_ support, exclude exec pins
	if (PinName.Equals(TEXT("As"), ESearchCase::IgnoreCase) ||
	    PinName.Equals(TEXT("AsOutput"), ESearchCase::IgnoreCase) ||
	    PinName.StartsWith(TEXT("As "), ESearchCase::IgnoreCase) ||
	    PinName.StartsWith(TEXT("AsBP"), ESearchCase::IgnoreCase))
	{
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == Direction &&
			    Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
			{
				FString PinNameStr = Pin->PinName.ToString();
				if (PinNameStr.StartsWith(TEXT("As "), ESearchCase::IgnoreCase) ||
				    PinNameStr.StartsWith(TEXT("AsBP"), ESearchCase::IgnoreCase) ||
				    PinNameStr.Equals(TEXT("As"), ESearchCase::IgnoreCase))
				{
					return Pin;
				}
			}
		}
	}

	// Fourth pass: For input direction, if looking for first exec pin
	if (Direction == EGPD_Input &&
	    (PinName.Equals(TEXT("Exec"), ESearchCase::IgnoreCase) ||
	     PinName.Equals(TEXT("Execute"), ESearchCase::IgnoreCase)))
	{
		// Find the first exec input pin
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Input &&
			    Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				return Pin;
			}
		}
	}

	// Fifth pass: For output direction, if looking for first exec pin
	if (Direction == EGPD_Output &&
	    (PinName.Equals(TEXT("Then"), ESearchCase::IgnoreCase) ||
	     PinName.Equals(TEXT("Exec"), ESearchCase::IgnoreCase)))
	{
		// Find the first exec output pin
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output &&
			    Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				return Pin;
			}
		}
	}

	// v2.6.4: Sixth pass: Variable node output pins
	// VariableSet passthrough: output pin is "Output_Get" not the variable name
	// VariableGet: output pin is also "Output_Get" or the variable name
	if (Direction == EGPD_Output)
	{
		UK2Node_VariableSet* VarSetNode = Cast<UK2Node_VariableSet>(Node);
		UK2Node_VariableGet* VarGetNode = Cast<UK2Node_VariableGet>(Node);
		if (VarSetNode || VarGetNode)
		{
			// Try to find the first non-exec output pin (the value output)
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && Pin->Direction == EGPD_Output &&
				    Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
				{
					// Return the first non-exec output - this is the value output
					return Pin;
				}
			}
		}
	}

	return nullptr;
}

void FEventGraphGenerator::AutoLayoutNodes(
	TMap<FString, UK2Node*>& NodeMap,
	const TArray<FManifestGraphNodeDefinition>& NodeDefs)
{
	// Simple auto-layout: place nodes without positions in a grid
	int32 CurrentX = 0;
	int32 CurrentY = 0;
	const int32 NodeSpacingX = 300;
	const int32 NodeSpacingY = 150;
	const int32 MaxNodesPerRow = 5;
	int32 NodesInRow = 0;

	for (const FManifestGraphNodeDefinition& NodeDef : NodeDefs)
	{
		if (!NodeDef.bHasPosition)
		{
			UK2Node** NodePtr = NodeMap.Find(NodeDef.Id);
			if (NodePtr && *NodePtr)
			{
				(*NodePtr)->NodePosX = CurrentX;
				(*NodePtr)->NodePosY = CurrentY;

				CurrentX += NodeSpacingX;
				NodesInRow++;

				if (NodesInRow >= MaxNodesPerRow)
				{
					CurrentX = 0;
					CurrentY += NodeSpacingY;
					NodesInRow = 0;
				}
			}
		}
	}
}

// ============================================================================
// v2.3.0: New Asset Type Generators
// ============================================================================

#include "Curves/CurveFloat.h"

// v2.3.0: Narrative Pro includes for data asset generation
#include "AI/NPCDefinition.h"
#include "Character/CharacterDefinition.h"
// Note: InventoryComponent, WeaponItem, EquippableItem moved to top of file for WellKnownFunctions
#include "GAS/AbilityConfiguration.h"
#include "AI/Activities/NPCActivityConfiguration.h"
#include "AI/Activities/NPCActivity.h"
#include "AI/Activities/NPCGoalGenerator.h"
#include "Items/NarrativeItem.h"
#include "Tales/NarrativeEvent.h"
#include "Tales/Dialogue.h"
#include "UnrealFramework/NarrativeNPCCharacter.h"

// v2.3.0: Float Curve Generator
FGenerationResult FFloatCurveGenerator::Generate(const FManifestFloatCurveDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Curves") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Float Curve"), Result))
	{
		return Result;
	}

	// v3.0: Use metadata-aware existence check
	uint64 InputHash = Definition.ComputeHash();
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Float Curve"), InputHash, Result))
	{
		return Result;
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	UCurveFloat* Curve = NewObject<UCurveFloat>(Package, *Definition.Name, RF_Public | RF_Standalone);
	if (!Curve)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Float Curve"));
	}

	// Add keys to the curve
	for (const auto& Key : Definition.Keys)
	{
		Curve->FloatCurve.AddKey(Key.Key, Key.Value);
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Curve);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Curve, *PackageFileName, SaveArgs);

	// v3.0: Store metadata after successful generation
	StoreDataAssetMetadata(Curve, TEXT("FC"), Definition.Name, InputHash);

	LogGeneration(FString::Printf(TEXT("Created Float Curve: %s with %d keys"), *Definition.Name, Definition.Keys.Num()));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// v2.6.0: Animation Montage Generator - now loads skeleton and creates montage
FGenerationResult FAnimationMontageGenerator::Generate(const FManifestAnimationMontageDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Animations") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Animation Montage"), Result))
	{
		return Result;
	}

	// v3.0: Use metadata-aware existence check
	uint64 InputHash = Definition.ComputeHash();
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Animation Montage"), InputHash, Result))
	{
		return Result;
	}

	// v2.6.0: Load skeleton from path
	USkeleton* Skeleton = nullptr;
	if (!Definition.Skeleton.IsEmpty())
	{
		// Try loading as skeleton directly
		Skeleton = LoadObject<USkeleton>(nullptr, *Definition.Skeleton);

		// If that fails, try loading as skeletal mesh and get its skeleton
		if (!Skeleton)
		{
			USkeletalMesh* SkeletalMesh = LoadObject<USkeletalMesh>(nullptr, *Definition.Skeleton);
			if (SkeletalMesh)
			{
				Skeleton = SkeletalMesh->GetSkeleton();
			}
		}

		// Try common project paths if still not found
		if (!Skeleton)
		{
			TArray<FString> SearchPaths = {
				FString::Printf(TEXT("%s/Characters/Skeletons/%s"), *GetProjectRoot(), *Definition.Skeleton),
				FString::Printf(TEXT("%s/Skeletons/%s"), *GetProjectRoot(), *Definition.Skeleton),
				FString::Printf(TEXT("/Game/Characters/%s"), *Definition.Skeleton),
				FString::Printf(TEXT("/Game/%s"), *Definition.Skeleton)
			};

			for (const FString& SearchPath : SearchPaths)
			{
				Skeleton = LoadObject<USkeleton>(nullptr, *SearchPath);
				if (Skeleton) break;

				USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *SearchPath);
				if (Mesh)
				{
					Skeleton = Mesh->GetSkeleton();
					if (Skeleton) break;
				}
			}
		}
	}

	if (!Skeleton)
	{
		LogGeneration(FString::Printf(TEXT("Animation Montage '%s': Could not load skeleton '%s' - creating empty montage"),
			*Definition.Name, *Definition.Skeleton));
	}

	// Create package
	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	// Create animation montage
	UAnimMontage* Montage = NewObject<UAnimMontage>(Package, *Definition.Name, RF_Public | RF_Standalone);
	if (!Montage)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Animation Montage"));
	}

	// Set skeleton if we have one
	if (Skeleton)
	{
		Montage->SetSkeleton(Skeleton);
		LogGeneration(FString::Printf(TEXT("  Set skeleton: %s"), *Skeleton->GetName()));
	}

	// Add sections from manifest using UE5.6 API
	for (int32 i = 0; i < Definition.Sections.Num(); i++)
	{
		const FString& SectionName = Definition.Sections[i];
		// Use AddAnimCompositeSection which properly initializes the section
		int32 SectionIndex = Montage->AddAnimCompositeSection(FName(*SectionName), 0.0f);
		if (SectionIndex != INDEX_NONE)
		{
			LogGeneration(FString::Printf(TEXT("  Added section: %s"), *SectionName));
		}
	}

	// Add default section if none specified
	if (Definition.Sections.Num() == 0)
	{
		Montage->AddAnimCompositeSection(FName("Default"), 0.0f);
	}

	// Set default slot track
	FSlotAnimationTrack SlotTrack;
	SlotTrack.SlotName = FName("DefaultSlot");
	Montage->SlotAnimTracks.Add(SlotTrack);

	// Set default blend times
	Montage->BlendIn.SetBlendTime(0.25f);
	Montage->BlendOut.SetBlendTime(0.25f);

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Montage);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Montage, *PackageFileName, SaveArgs);

	// v3.0: Store metadata after successful generation
	StoreDataAssetMetadata(Montage, TEXT("AM"), Definition.Name, InputHash);

	LogGeneration(FString::Printf(TEXT("Created Animation Montage: %s with %d sections"),
		*Definition.Name, Definition.Sections.Num()));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// v2.6.9: Animation Notify Generator (creates Blueprint with UAnimNotifyState parent)
FGenerationResult FAnimationNotifyGenerator::Generate(const FManifestAnimationNotifyDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Animations/Notifies") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Animation Notify"), Result))
	{
		return Result;
	}

	// v3.0: Check existence with metadata-aware logic for MODIFY/CONFLICT detection
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Animation Notify"), Definition.ComputeHash(), Result))
	{
		return Result;
	}

	// Determine parent class - use NotifyClass from manifest or default to UAnimNotifyState
	UClass* ParentClass = nullptr;
	if (!Definition.NotifyClass.IsEmpty())
	{
		ParentClass = FindParentClass(Definition.NotifyClass);
	}
	if (!ParentClass)
	{
		// Default to UAnimNotifyState for notify states (NAS_*)
		if (Definition.Name.StartsWith(TEXT("NAS_")))
		{
			ParentClass = UAnimNotifyState::StaticClass();
		}
		else
		{
			// Default to UAnimNotify for simple notifies (AN_*)
			ParentClass = UAnimNotify::StaticClass();
		}
	}

	if (!ParentClass)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Could not find parent class for Animation Notify"));
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
		ParentClass,
		Package,
		*Definition.Name,
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass()
	);

	if (!Blueprint)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Animation Notify Blueprint"));
	}

	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	FAssetRegistryModule::AssetCreated(Blueprint);
	Package->MarkPackageDirty();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Animation Notify: %s (Parent: %s)"), *Definition.Name, *ParentClass->GetName()));

	// v3.0: Store metadata for regeneration tracking
	StoreBlueprintMetadata(Blueprint, TEXT("AnimationNotify"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New,
		FString::Printf(TEXT("Created at %s"), *AssetPath));
	Result.DetermineCategory();
	return Result;
}

// v2.3.0: Dialogue Blueprint Generator
FGenerationResult FDialogueBlueprintGenerator::Generate(
	const FManifestDialogueBlueprintDefinition& Definition,
	const FString& ProjectRoot,
	const FManifestData* ManifestData)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Dialogues") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Dialogue Blueprint"), Result))
	{
		return Result;
	}

	// v3.0: Check existence with metadata-aware logic for MODIFY/CONFLICT detection
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Dialogue Blueprint"), Definition.ComputeHash(), Result))
	{
		return Result;
	}

	// Find parent class
	UClass* ParentClass = FindParentClass(Definition.ParentClass);
	if (!ParentClass)
	{
		ParentClass = FindParentClass(TEXT("NarrativeDialogue"));
	}
	if (!ParentClass)
	{
		ParentClass = AActor::StaticClass();
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = ParentClass;

	UBlueprint* Blueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
		UBlueprint::StaticClass(), Package, *Definition.Name,
		RF_Public | RF_Standalone, nullptr, GWarn));

	if (!Blueprint)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Dialogue Blueprint"));
	}

	// Add variables
	for (const auto& VarDef : Definition.Variables)
	{
		FEdGraphPinType PinType;
		PinType.PinCategory = UEdGraphSchema_K2::PC_Object;

		if (VarDef.Type == TEXT("Boolean") || VarDef.Type == TEXT("bool"))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
		}
		else if (VarDef.Type == TEXT("Integer") || VarDef.Type == TEXT("int"))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
		}
		else if (VarDef.Type == TEXT("Float") || VarDef.Type == TEXT("float"))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
			PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
		}
		else if (VarDef.Type == TEXT("String") || VarDef.Type == TEXT("FString"))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_String;
		}

		FBlueprintEditorUtils::AddMemberVariable(Blueprint, *VarDef.Name, PinType);
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// v3.2: Set dialogue configuration properties on CDO
	UObject* CDO = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
	if (CDO)
	{
		// Set bFreeMovement (default true)
		if (!Definition.bFreeMovement)  // Only set if false (non-default)
		{
			FBoolProperty* FreeMoveP = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bFreeMovement")));
			if (FreeMoveP)
			{
				FreeMoveP->SetPropertyValue_InContainer(CDO, false);
				LogGeneration(TEXT("  Set bFreeMovement: false"));
			}
		}

		// Set bUnskippable (default false)
		if (Definition.bUnskippable)
		{
			FBoolProperty* UnskippableP = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bUnskippable")));
			if (UnskippableP)
			{
				UnskippableP->SetPropertyValue_InContainer(CDO, true);
				LogGeneration(TEXT("  Set bUnskippable: true"));
			}
		}

		// Set bCanBeExited (default true)
		if (!Definition.bCanBeExited)
		{
			FBoolProperty* CanExitP = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bCanBeExited")));
			if (CanExitP)
			{
				CanExitP->SetPropertyValue_InContainer(CDO, false);
				LogGeneration(TEXT("  Set bCanBeExited: false"));
			}
		}

		// Set bShowCinematicBars (default false)
		if (Definition.bShowCinematicBars)
		{
			FBoolProperty* CinematicP = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bShowCinematicBars")));
			if (CinematicP)
			{
				CinematicP->SetPropertyValue_InContainer(CDO, true);
				LogGeneration(TEXT("  Set bShowCinematicBars: true"));
			}
		}

		// Set bAutoRotateSpeakers (default true)
		if (!Definition.bAutoRotateSpeakers)
		{
			FBoolProperty* AutoRotateP = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bAutoRotateSpeakers")));
			if (AutoRotateP)
			{
				AutoRotateP->SetPropertyValue_InContainer(CDO, false);
				LogGeneration(TEXT("  Set bAutoRotateSpeakers: false"));
			}
		}

		// Set bAutoStopMovement (default false)
		if (Definition.bAutoStopMovement)
		{
			FBoolProperty* AutoStopP = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bAutoStopMovement")));
			if (AutoStopP)
			{
				AutoStopP->SetPropertyValue_InContainer(CDO, true);
				LogGeneration(TEXT("  Set bAutoStopMovement: true"));
			}
		}

		// Set Priority (default 0)
		if (Definition.Priority != 0)
		{
			FIntProperty* PriorityP = CastField<FIntProperty>(CDO->GetClass()->FindPropertyByName(TEXT("Priority")));
			if (PriorityP)
			{
				PriorityP->SetPropertyValue_InContainer(CDO, Definition.Priority);
				LogGeneration(FString::Printf(TEXT("  Set Priority: %d"), Definition.Priority));
			}
		}

		// Set EndDialogueDist (default 0.0f)
		if (Definition.EndDialogueDist > 0.0f)
		{
			FFloatProperty* EndDistP = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("EndDialogueDist")));
			if (EndDistP)
			{
				EndDistP->SetPropertyValue_InContainer(CDO, Definition.EndDialogueDist);
				LogGeneration(FString::Printf(TEXT("  Set EndDialogueDist: %.1f"), Definition.EndDialogueDist));
			}
		}

		// v3.5: Set DefaultHeadBoneName (default "head")
		if (!Definition.DefaultHeadBoneName.IsEmpty() && Definition.DefaultHeadBoneName != TEXT("head"))
		{
			FNameProperty* HeadBoneP = CastField<FNameProperty>(CDO->GetClass()->FindPropertyByName(TEXT("DefaultHeadBoneName")));
			if (HeadBoneP)
			{
				HeadBoneP->SetPropertyValue_InContainer(CDO, FName(*Definition.DefaultHeadBoneName));
				LogGeneration(FString::Printf(TEXT("  Set DefaultHeadBoneName: %s"), *Definition.DefaultHeadBoneName));
			}
		}

		// v3.5: Set DialogueBlendOutTime (default 0.5f)
		if (Definition.DialogueBlendOutTime != 0.5f)
		{
			FFloatProperty* BlendOutP = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("DialogueBlendOutTime")));
			if (BlendOutP)
			{
				BlendOutP->SetPropertyValue_InContainer(CDO, Definition.DialogueBlendOutTime);
				LogGeneration(FString::Printf(TEXT("  Set DialogueBlendOutTime: %.2f"), Definition.DialogueBlendOutTime));
			}
		}

		// v3.5: Set bAdjustPlayerTransform (default false)
		if (Definition.bAdjustPlayerTransform)
		{
			FBoolProperty* AdjustTransformP = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bAdjustPlayerTransform")));
			if (AdjustTransformP)
			{
				AdjustTransformP->SetPropertyValue_InContainer(CDO, true);
				LogGeneration(TEXT("  Set bAdjustPlayerTransform: true"));
			}
		}

		// Recompile after setting CDO properties
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
	}

	// v3.2: Log speakers for manual setup (FSpeakerInfo contains soft object refs)
	if (Definition.Speakers.Num() > 0)
	{
		LogGeneration(FString::Printf(TEXT("  Dialogue '%s' has %d speaker(s) to configure manually:"), *Definition.Name, Definition.Speakers.Num()));
		for (int32 i = 0; i < Definition.Speakers.Num(); i++)
		{
			const auto& Speaker = Definition.Speakers[i];
			LogGeneration(FString::Printf(TEXT("    [%d] NPCDefinition: %s, NodeColor: %s, OwnedTags: %d"),
				i, *Speaker.NPCDefinition, *Speaker.NodeColor, Speaker.OwnedTags.Num()));
		}
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Blueprint);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Dialogue Blueprint: %s"), *Definition.Name));

	// v3.0: Store metadata for regeneration tracking
	StoreBlueprintMetadata(Blueprint, TEXT("DialogueBlueprint"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// v2.6.0: Equippable Item Generator - sets slots, effect, and abilities on CDO
// v2.6.8: Fixed to use ParentClass from manifest for RangedWeaponItem, MeleeWeaponItem, etc.
FGenerationResult FEquippableItemGenerator::Generate(const FManifestEquippableItemDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Items") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Equippable Item"), Result))
	{
		return Result;
	}

	// v3.0: Check existence with metadata-aware logic for MODIFY/CONFLICT detection
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Equippable Item"), Definition.ComputeHash(), Result))
	{
		return Result;
	}

	// v2.6.9: Check dependencies BEFORE creating asset (deferred handling)
	// Check equipment_modifier_ge dependency
	UClass* EquipmentEffectClass = nullptr;
	if (!Definition.EquipmentModifierGE.IsEmpty())
	{
		FString GEPath = Definition.EquipmentModifierGE;
		if (!GEPath.Contains(TEXT("/")))
		{
			// Try Blueprint GE path first (v2.6.6 creates GEs as Blueprints)
			GEPath = FString::Printf(TEXT("%s/Effects/Forms/%s.%s_C"), *GetProjectRoot(), *Definition.EquipmentModifierGE, *Definition.EquipmentModifierGE);
		}
		EquipmentEffectClass = LoadObject<UClass>(nullptr, *GEPath);
		if (!EquipmentEffectClass)
		{
			// Try without _C suffix
			GEPath = FString::Printf(TEXT("%s/Effects/Forms/%s"), *GetProjectRoot(), *Definition.EquipmentModifierGE);
			EquipmentEffectClass = LoadObject<UClass>(nullptr, *GEPath);
		}
		if (!EquipmentEffectClass)
		{
			// Try generic Effects folder
			GEPath = FString::Printf(TEXT("%s/Effects/%s.%s_C"), *GetProjectRoot(), *Definition.EquipmentModifierGE, *Definition.EquipmentModifierGE);
			EquipmentEffectClass = LoadObject<UClass>(nullptr, *GEPath);
		}
		if (!EquipmentEffectClass)
		{
			LogGeneration(FString::Printf(TEXT("  Deferring: equipment_modifier_ge '%s' not found yet"), *Definition.EquipmentModifierGE));
			Result = FGenerationResult(Definition.Name, EGenerationStatus::Deferred,
				FString::Printf(TEXT("Missing dependency: %s"), *Definition.EquipmentModifierGE));
			Result.MissingDependency = Definition.EquipmentModifierGE;
			Result.MissingDependencyType = TEXT("GameplayEffect");
			Result.DetermineCategory();
			return Result;
		}
	}

	// Check abilities_to_grant dependencies
	TArray<UClass*> AbilityClasses;
	for (const FString& AbilityName : Definition.AbilitiesToGrant)
	{
		FString AbilityPath = AbilityName;
		if (!AbilityPath.Contains(TEXT("/")))
		{
			AbilityPath = FString::Printf(TEXT("%s/Abilities/Forms/%s.%s_C"), *GetProjectRoot(), *AbilityName, *AbilityName);
		}
		UClass* AbilityClass = LoadObject<UClass>(nullptr, *AbilityPath);
		if (!AbilityClass)
		{
			// Try other ability folders
			AbilityPath = FString::Printf(TEXT("%s/Abilities/Actions/%s.%s_C"), *GetProjectRoot(), *AbilityName, *AbilityName);
			AbilityClass = LoadObject<UClass>(nullptr, *AbilityPath);
		}
		if (!AbilityClass)
		{
			AbilityPath = FString::Printf(TEXT("%s/Abilities/Combat/%s.%s_C"), *GetProjectRoot(), *AbilityName, *AbilityName);
			AbilityClass = LoadObject<UClass>(nullptr, *AbilityPath);
		}
		if (!AbilityClass)
		{
			AbilityPath = FString::Printf(TEXT("%s/Abilities/%s.%s_C"), *GetProjectRoot(), *AbilityName, *AbilityName);
			AbilityClass = LoadObject<UClass>(nullptr, *AbilityPath);
		}
		// v2.6.9: Try Narrative Pro paths for abilities/activities
		if (!AbilityClass)
		{
			AbilityPath = FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/FollowCharacter/%s.%s_C"), *AbilityName, *AbilityName);
			AbilityClass = LoadObject<UClass>(nullptr, *AbilityPath);
		}
		if (!AbilityClass)
		{
			AbilityPath = FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/%s/%s.%s_C"), *AbilityName, *AbilityName, *AbilityName);
			AbilityClass = LoadObject<UClass>(nullptr, *AbilityPath);
		}
		if (!AbilityClass)
		{
			AbilityPath = FString::Printf(TEXT("/NarrativePro/Pro/Core/Abilities/GameplayAbilities/%s.%s_C"), *AbilityName, *AbilityName);
			AbilityClass = LoadObject<UClass>(nullptr, *AbilityPath);
		}
		if (!AbilityClass)
		{
			LogGeneration(FString::Printf(TEXT("  Deferring: ability '%s' not found yet"), *AbilityName));
			Result = FGenerationResult(Definition.Name, EGenerationStatus::Deferred,
				FString::Printf(TEXT("Missing dependency: %s"), *AbilityName));
			Result.MissingDependency = AbilityName;
			Result.MissingDependencyType = TEXT("GameplayAbility");
			Result.DetermineCategory();
			return Result;
		}
		AbilityClasses.Add(AbilityClass);
	}

	// v2.6.8: Use ParentClass from manifest (supports RangedWeaponItem, MeleeWeaponItem, etc.)
	UClass* ParentClass = nullptr;
	if (!Definition.ParentClass.IsEmpty())
	{
		ParentClass = FindParentClass(Definition.ParentClass);
		if (ParentClass)
		{
			LogGeneration(FString::Printf(TEXT("Using parent class: %s"), *Definition.ParentClass));
		}
	}
	// Fallback to UEquippableItem if no parent specified or not found
	if (!ParentClass)
	{
		ParentClass = UEquippableItem::StaticClass();
	}
	if (!ParentClass)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("UEquippableItem class not found"));
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	// Create Blueprint factory
	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = ParentClass;

	UBlueprint* Blueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
		UBlueprint::StaticClass(), Package, *Definition.Name,
		RF_Public | RF_Standalone, nullptr, GWarn));

	if (!Blueprint)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Equippable Item Blueprint"));
	}

	// Compile blueprint first to generate the class
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// v2.6.0: Set properties on the CDO using reflection (properties are protected)
	if (Blueprint->GeneratedClass)
	{
		UObject* CDO = Blueprint->GeneratedClass->GetDefaultObject();
		if (CDO)
		{
			// Set equipment slot tag using reflection
			if (!Definition.EquipmentSlot.IsEmpty())
			{
				FGameplayTag SlotTag = FGameplayTag::RequestGameplayTag(FName(*Definition.EquipmentSlot), false);
				if (SlotTag.IsValid())
				{
					FStructProperty* SlotsProperty = CastField<FStructProperty>(CDO->GetClass()->FindPropertyByName(TEXT("EquippableSlots")));
					if (SlotsProperty)
					{
						FGameplayTagContainer* TagContainer = SlotsProperty->ContainerPtrToValuePtr<FGameplayTagContainer>(CDO);
						if (TagContainer)
						{
							TagContainer->AddTag(SlotTag);
							LogGeneration(FString::Printf(TEXT("  Set EquippableSlot: %s"), *Definition.EquipmentSlot));
						}
					}
				}
				else
				{
					LogGeneration(FString::Printf(TEXT("  WARNING: Invalid slot tag '%s'"), *Definition.EquipmentSlot));
				}
			}

			// v2.6.9: Use pre-loaded EquipmentEffectClass from deferred check
			if (EquipmentEffectClass)
			{
				FClassProperty* EffectProperty = CastField<FClassProperty>(CDO->GetClass()->FindPropertyByName(TEXT("EquipmentEffect")));
				if (EffectProperty)
				{
					EffectProperty->SetObjectPropertyValue_InContainer(CDO, EquipmentEffectClass);
					LogGeneration(FString::Printf(TEXT("  Set EquipmentEffect: %s"), *EquipmentEffectClass->GetName()));
				}
			}

			// v2.6.9: Use pre-loaded AbilityClasses from deferred check
			if (AbilityClasses.Num() > 0)
			{
				FArrayProperty* AbilitiesProperty = CastField<FArrayProperty>(CDO->GetClass()->FindPropertyByName(TEXT("EquipmentAbilities")));
				if (AbilitiesProperty)
				{
					FScriptArrayHelper ArrayHelper(AbilitiesProperty, AbilitiesProperty->ContainerPtrToValuePtr<void>(CDO));
					for (UClass* AbilityClass : AbilityClasses)
					{
						int32 NewIndex = ArrayHelper.AddValue();
						FClassProperty* InnerProp = CastField<FClassProperty>(AbilitiesProperty->Inner);
						if (InnerProp)
						{
							InnerProp->SetObjectPropertyValue(ArrayHelper.GetRawPtr(NewIndex), AbilityClass);
							LogGeneration(FString::Printf(TEXT("  Added ability: %s"), *AbilityClass->GetName()));
						}
					}
				}
			}

			// v3.3: Set EquippableItem stat properties
			if (Definition.AttackRating != 0.0f)
			{
				FFloatProperty* AttackProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("AttackRating")));
				if (AttackProp)
				{
					AttackProp->SetPropertyValue_InContainer(CDO, Definition.AttackRating);
					LogGeneration(FString::Printf(TEXT("  Set AttackRating: %.2f"), Definition.AttackRating));
				}
			}
			if (Definition.ArmorRating != 0.0f)
			{
				FFloatProperty* ArmorProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("ArmorRating")));
				if (ArmorProp)
				{
					ArmorProp->SetPropertyValue_InContainer(CDO, Definition.ArmorRating);
					LogGeneration(FString::Printf(TEXT("  Set ArmorRating: %.2f"), Definition.ArmorRating));
				}
			}
			if (Definition.StealthRating != 0.0f)
			{
				FFloatProperty* StealthProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("StealthRating")));
				if (StealthProp)
				{
					StealthProp->SetPropertyValue_InContainer(CDO, Definition.StealthRating);
					LogGeneration(FString::Printf(TEXT("  Set StealthRating: %.2f"), Definition.StealthRating));
				}
			}

			// v3.3: Set NarrativeItem base properties
			if (!Definition.DisplayName.IsEmpty())
			{
				FTextProperty* DisplayNameProp = CastField<FTextProperty>(CDO->GetClass()->FindPropertyByName(TEXT("DisplayName")));
				if (DisplayNameProp)
				{
					DisplayNameProp->SetPropertyValue_InContainer(CDO, FText::FromString(Definition.DisplayName));
					LogGeneration(FString::Printf(TEXT("  Set DisplayName: %s"), *Definition.DisplayName));
				}
			}
			if (!Definition.Description.IsEmpty())
			{
				FTextProperty* DescProp = CastField<FTextProperty>(CDO->GetClass()->FindPropertyByName(TEXT("Description")));
				if (DescProp)
				{
					DescProp->SetPropertyValue_InContainer(CDO, FText::FromString(Definition.Description));
					LogGeneration(FString::Printf(TEXT("  Set Description: %s"), *Definition.Description));
				}
			}
			if (Definition.Weight != 0.0f)
			{
				FFloatProperty* WeightProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("Weight")));
				if (WeightProp)
				{
					WeightProp->SetPropertyValue_InContainer(CDO, Definition.Weight);
					LogGeneration(FString::Printf(TEXT("  Set Weight: %.2f"), Definition.Weight));
				}
			}
			if (Definition.BaseValue != 0)
			{
				FIntProperty* ValueProp = CastField<FIntProperty>(CDO->GetClass()->FindPropertyByName(TEXT("BaseValue")));
				if (ValueProp)
				{
					ValueProp->SetPropertyValue_InContainer(CDO, Definition.BaseValue);
					LogGeneration(FString::Printf(TEXT("  Set BaseValue: %d"), Definition.BaseValue));
				}
			}
			if (Definition.BaseScore != 0.0f)
			{
				FFloatProperty* ScoreProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("BaseScore")));
				if (ScoreProp)
				{
					ScoreProp->SetPropertyValue_InContainer(CDO, Definition.BaseScore);
					LogGeneration(FString::Printf(TEXT("  Set BaseScore: %.2f"), Definition.BaseScore));
				}
			}
			if (Definition.UseRechargeDuration != 0.0f)
			{
				FFloatProperty* RechargeProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("UseRechargeDuration")));
				if (RechargeProp)
				{
					RechargeProp->SetPropertyValue_InContainer(CDO, Definition.UseRechargeDuration);
					LogGeneration(FString::Printf(TEXT("  Set UseRechargeDuration: %.2f"), Definition.UseRechargeDuration));
				}
			}
			// bStackable and MaxStackSize
			if (Definition.bStackable)
			{
				FBoolProperty* StackableProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bStackable")));
				if (StackableProp)
				{
					StackableProp->SetPropertyValue_InContainer(CDO, true);
					LogGeneration(TEXT("  Set bStackable: true"));
				}
				if (Definition.MaxStackSize > 1)
				{
					FIntProperty* MaxStackProp = CastField<FIntProperty>(CDO->GetClass()->FindPropertyByName(TEXT("MaxStackSize")));
					if (MaxStackProp)
					{
						MaxStackProp->SetPropertyValue_InContainer(CDO, Definition.MaxStackSize);
						LogGeneration(FString::Printf(TEXT("  Set MaxStackSize: %d"), Definition.MaxStackSize));
					}
				}
			}
			// ItemTags
			if (Definition.ItemTags.Num() > 0)
			{
				FStructProperty* ItemTagsProp = CastField<FStructProperty>(CDO->GetClass()->FindPropertyByName(TEXT("ItemTags")));
				if (ItemTagsProp)
				{
					FGameplayTagContainer* TagContainer = ItemTagsProp->ContainerPtrToValuePtr<FGameplayTagContainer>(CDO);
					if (TagContainer)
					{
						for (const FString& TagString : Definition.ItemTags)
						{
							FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
							if (Tag.IsValid())
							{
								TagContainer->AddTag(Tag);
								LogGeneration(FString::Printf(TEXT("  Added ItemTag: %s"), *TagString));
							}
						}
					}
				}
			}
			// Thumbnail (TSoftObjectPtr<UTexture2D>)
			if (!Definition.Thumbnail.IsEmpty())
			{
				FSoftObjectProperty* ThumbnailProp = CastField<FSoftObjectProperty>(CDO->GetClass()->FindPropertyByName(TEXT("Thumbnail")));
				if (ThumbnailProp)
				{
					FSoftObjectPtr SoftPtr(FSoftObjectPath(Definition.Thumbnail));
					ThumbnailProp->SetPropertyValue_InContainer(CDO, SoftPtr);
					LogGeneration(FString::Printf(TEXT("  Set Thumbnail: %s"), *Definition.Thumbnail));
				}
			}

			// v3.4: WeaponItem properties (set if parent is WeaponItem/MeleeWeaponItem/RangedWeaponItem)
			// WeaponVisualClass (TSoftClassPtr<AWeaponVisual>)
			if (!Definition.WeaponVisualClass.IsEmpty())
			{
				FSoftClassProperty* VisualProp = CastField<FSoftClassProperty>(CDO->GetClass()->FindPropertyByName(TEXT("WeaponVisualClass")));
				if (VisualProp)
				{
					FSoftObjectPath ClassPath(Definition.WeaponVisualClass);
					void* PropertyValue = VisualProp->ContainerPtrToValuePtr<void>(CDO);
					if (PropertyValue)
					{
						FSoftObjectPtr* SoftPtr = static_cast<FSoftObjectPtr*>(PropertyValue);
						if (SoftPtr)
						{
							*SoftPtr = FSoftObjectPtr(ClassPath);
							LogGeneration(FString::Printf(TEXT("  Set WeaponVisualClass: %s"), *Definition.WeaponVisualClass));
						}
					}
				}
			}
			// WeaponHand (EWeaponHandRule enum)
			if (!Definition.WeaponHand.IsEmpty())
			{
				FByteProperty* HandProp = CastField<FByteProperty>(CDO->GetClass()->FindPropertyByName(TEXT("WeaponHand")));
				if (HandProp)
				{
					// Map string to enum value
					uint8 HandValue = 0; // TwoHanded
					if (Definition.WeaponHand.Equals(TEXT("MainHand"), ESearchCase::IgnoreCase))
					{
						HandValue = 1;
					}
					else if (Definition.WeaponHand.Equals(TEXT("OffHand"), ESearchCase::IgnoreCase))
					{
						HandValue = 2;
					}
					else if (Definition.WeaponHand.Equals(TEXT("DualWieldable"), ESearchCase::IgnoreCase))
					{
						HandValue = 3;
					}
					HandProp->SetPropertyValue_InContainer(CDO, HandValue);
					LogGeneration(FString::Printf(TEXT("  Set WeaponHand: %s"), *Definition.WeaponHand));
				}
				else
				{
					// Try enum property
					FEnumProperty* EnumHandProp = CastField<FEnumProperty>(CDO->GetClass()->FindPropertyByName(TEXT("WeaponHand")));
					if (EnumHandProp)
					{
						UEnum* EnumClass = EnumHandProp->GetEnum();
						if (EnumClass)
						{
							int64 EnumValue = EnumClass->GetValueByNameString(Definition.WeaponHand);
							if (EnumValue != INDEX_NONE)
							{
								EnumHandProp->GetUnderlyingProperty()->SetIntPropertyValue(EnumHandProp->ContainerPtrToValuePtr<void>(CDO), EnumValue);
								LogGeneration(FString::Printf(TEXT("  Set WeaponHand: %s"), *Definition.WeaponHand));
							}
						}
					}
				}
			}
			// bPawnFollowsControlRotation
			if (Definition.bPawnFollowsControlRotation)
			{
				FBoolProperty* FollowProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bPawnFollowsControlRotation")));
				if (FollowProp)
				{
					FollowProp->SetPropertyValue_InContainer(CDO, true);
					LogGeneration(TEXT("  Set bPawnFollowsControlRotation: true"));
				}
			}
			// bPawnOrientsRotationToMovement (default true, only set if false)
			if (!Definition.bPawnOrientsRotationToMovement)
			{
				FBoolProperty* OrientProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bPawnOrientsRotationToMovement")));
				if (OrientProp)
				{
					OrientProp->SetPropertyValue_InContainer(CDO, false);
					LogGeneration(TEXT("  Set bPawnOrientsRotationToMovement: false"));
				}
			}
			// AttackDamage
			if (Definition.AttackDamage != 0.0f)
			{
				FFloatProperty* DamageProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("AttackDamage")));
				if (DamageProp)
				{
					DamageProp->SetPropertyValue_InContainer(CDO, Definition.AttackDamage);
					LogGeneration(FString::Printf(TEXT("  Set AttackDamage: %.2f"), Definition.AttackDamage));
				}
			}
			// HeavyAttackDamageMultiplier (default 1.5, only set if different)
			if (Definition.HeavyAttackDamageMultiplier != 1.5f)
			{
				FFloatProperty* HeavyProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("HeavyAttackDamageMultiplier")));
				if (HeavyProp)
				{
					HeavyProp->SetPropertyValue_InContainer(CDO, Definition.HeavyAttackDamageMultiplier);
					LogGeneration(FString::Printf(TEXT("  Set HeavyAttackDamageMultiplier: %.2f"), Definition.HeavyAttackDamageMultiplier));
				}
			}
			// bAllowManualReload (default true, only set if false)
			if (!Definition.bAllowManualReload)
			{
				FBoolProperty* ReloadProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bAllowManualReload")));
				if (ReloadProp)
				{
					ReloadProp->SetPropertyValue_InContainer(CDO, false);
					LogGeneration(TEXT("  Set bAllowManualReload: false"));
				}
			}
			// RequiredAmmo (TSubclassOf<UNarrativeItem>)
			if (!Definition.RequiredAmmo.IsEmpty())
			{
				UClass* AmmoClass = LoadClass<UObject>(nullptr, *Definition.RequiredAmmo);
				if (!AmmoClass)
				{
					// Try project paths
					FString AmmoPath = FString::Printf(TEXT("%s/Items/%s.%s_C"), *GetProjectRoot(), *Definition.RequiredAmmo, *Definition.RequiredAmmo);
					AmmoClass = LoadClass<UObject>(nullptr, *AmmoPath);
				}
				if (AmmoClass)
				{
					FClassProperty* AmmoProp = CastField<FClassProperty>(CDO->GetClass()->FindPropertyByName(TEXT("RequiredAmmo")));
					if (AmmoProp)
					{
						AmmoProp->SetObjectPropertyValue_InContainer(CDO, AmmoClass);
						LogGeneration(FString::Printf(TEXT("  Set RequiredAmmo: %s"), *AmmoClass->GetName()));
					}
				}
			}
			// bBotsConsumeAmmo (default true, only set if false)
			if (!Definition.bBotsConsumeAmmo)
			{
				FBoolProperty* ConsumeProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bBotsConsumeAmmo")));
				if (ConsumeProp)
				{
					ConsumeProp->SetPropertyValue_InContainer(CDO, false);
					LogGeneration(TEXT("  Set bBotsConsumeAmmo: false"));
				}
			}
			// BotAttackRange (default 1000, only set if different)
			if (Definition.BotAttackRange != 1000.0f)
			{
				FFloatProperty* RangeProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("BotAttackRange")));
				if (RangeProp)
				{
					RangeProp->SetPropertyValue_InContainer(CDO, Definition.BotAttackRange);
					LogGeneration(FString::Printf(TEXT("  Set BotAttackRange: %.2f"), Definition.BotAttackRange));
				}
			}
			// ClipSize
			if (Definition.ClipSize > 0)
			{
				FIntProperty* ClipProp = CastField<FIntProperty>(CDO->GetClass()->FindPropertyByName(TEXT("ClipSize")));
				if (ClipProp)
				{
					ClipProp->SetPropertyValue_InContainer(CDO, Definition.ClipSize);
					LogGeneration(FString::Printf(TEXT("  Set ClipSize: %d"), Definition.ClipSize));
				}
			}
			// v3.4: RangedWeaponItem properties
			// AimFOVPct (default 0.75, only set if different)
			if (Definition.AimFOVPct != 0.75f)
			{
				FFloatProperty* AimProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("AimFOVPct")));
				if (AimProp)
				{
					AimProp->SetPropertyValue_InContainer(CDO, Definition.AimFOVPct);
					LogGeneration(FString::Printf(TEXT("  Set AimFOVPct: %.2f"), Definition.AimFOVPct));
				}
			}
			// BaseSpreadDegrees
			if (Definition.BaseSpreadDegrees != 0.0f)
			{
				FFloatProperty* SpreadProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("BaseSpreadDegrees")));
				if (SpreadProp)
				{
					SpreadProp->SetPropertyValue_InContainer(CDO, Definition.BaseSpreadDegrees);
					LogGeneration(FString::Printf(TEXT("  Set BaseSpreadDegrees: %.2f"), Definition.BaseSpreadDegrees));
				}
			}
			// MaxSpreadDegrees (default 5.0, only set if different)
			if (Definition.MaxSpreadDegrees != 5.0f)
			{
				FFloatProperty* MaxSpreadProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("MaxSpreadDegrees")));
				if (MaxSpreadProp)
				{
					MaxSpreadProp->SetPropertyValue_InContainer(CDO, Definition.MaxSpreadDegrees);
					LogGeneration(FString::Printf(TEXT("  Set MaxSpreadDegrees: %.2f"), Definition.MaxSpreadDegrees));
				}
			}
			// SpreadFireBump (default 0.5, only set if different)
			if (Definition.SpreadFireBump != 0.5f)
			{
				FFloatProperty* BumpProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("SpreadFireBump")));
				if (BumpProp)
				{
					BumpProp->SetPropertyValue_InContainer(CDO, Definition.SpreadFireBump);
					LogGeneration(FString::Printf(TEXT("  Set SpreadFireBump: %.2f"), Definition.SpreadFireBump));
				}
			}
			// SpreadDecreaseSpeed (default 5.0, only set if different)
			if (Definition.SpreadDecreaseSpeed != 5.0f)
			{
				FFloatProperty* DecreaseProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("SpreadDecreaseSpeed")));
				if (DecreaseProp)
				{
					DecreaseProp->SetPropertyValue_InContainer(CDO, Definition.SpreadDecreaseSpeed);
					LogGeneration(FString::Printf(TEXT("  Set SpreadDecreaseSpeed: %.2f"), Definition.SpreadDecreaseSpeed));
				}
			}
			// v3.4: Weapon ability arrays (TArray<TSubclassOf<UGameplayAbility>>)
			// Helper lambda to load ability classes
			const FString ProjectRoot = GetProjectRoot();
			auto LoadAbilityClassLocal = [&ProjectRoot](const FString& AbilityName) -> UClass*
			{
				// Try direct path first
				UClass* AbilityClass = LoadObject<UClass>(nullptr, *AbilityName);
				if (AbilityClass) return AbilityClass;

				// Try project paths
				TArray<FString> SearchPaths = {
					FString::Printf(TEXT("%s/Abilities/Forms/%s.%s_C"), *ProjectRoot, *AbilityName, *AbilityName),
					FString::Printf(TEXT("%s/Abilities/Actions/%s.%s_C"), *ProjectRoot, *AbilityName, *AbilityName),
					FString::Printf(TEXT("%s/Abilities/Combat/%s.%s_C"), *ProjectRoot, *AbilityName, *AbilityName),
					FString::Printf(TEXT("%s/Abilities/%s.%s_C"), *ProjectRoot, *AbilityName, *AbilityName),
					FString::Printf(TEXT("/NarrativePro/Pro/Core/Abilities/GameplayAbilities/%s.%s_C"), *AbilityName, *AbilityName)
				};
				for (const FString& Path : SearchPaths)
				{
					AbilityClass = LoadObject<UClass>(nullptr, *Path);
					if (AbilityClass) return AbilityClass;
				}
				return nullptr;
			};

			// WeaponAbilities
			if (Definition.WeaponAbilities.Num() > 0)
			{
				FArrayProperty* WeaponAbilityProp = CastField<FArrayProperty>(CDO->GetClass()->FindPropertyByName(TEXT("WeaponAbilities")));
				if (WeaponAbilityProp)
				{
					FScriptArrayHelper ArrayHelper(WeaponAbilityProp, WeaponAbilityProp->ContainerPtrToValuePtr<void>(CDO));
					for (const FString& AbilityName : Definition.WeaponAbilities)
					{
						UClass* AbilityClass = LoadAbilityClassLocal(AbilityName);
						if (AbilityClass)
						{
							int32 NewIndex = ArrayHelper.AddValue();
							FClassProperty* InnerProp = CastField<FClassProperty>(WeaponAbilityProp->Inner);
							if (InnerProp)
							{
								InnerProp->SetObjectPropertyValue(ArrayHelper.GetRawPtr(NewIndex), AbilityClass);
								LogGeneration(FString::Printf(TEXT("  Added WeaponAbility: %s"), *AbilityClass->GetName()));
							}
						}
					}
				}
			}
			// MainhandAbilities
			if (Definition.MainhandAbilities.Num() > 0)
			{
				FArrayProperty* MainhandProp = CastField<FArrayProperty>(CDO->GetClass()->FindPropertyByName(TEXT("MainhandAbilities")));
				if (MainhandProp)
				{
					FScriptArrayHelper ArrayHelper(MainhandProp, MainhandProp->ContainerPtrToValuePtr<void>(CDO));
					for (const FString& AbilityName : Definition.MainhandAbilities)
					{
						UClass* AbilityClass = LoadAbilityClassLocal(AbilityName);
						if (AbilityClass)
						{
							int32 NewIndex = ArrayHelper.AddValue();
							FClassProperty* InnerProp = CastField<FClassProperty>(MainhandProp->Inner);
							if (InnerProp)
							{
								InnerProp->SetObjectPropertyValue(ArrayHelper.GetRawPtr(NewIndex), AbilityClass);
								LogGeneration(FString::Printf(TEXT("  Added MainhandAbility: %s"), *AbilityClass->GetName()));
							}
						}
					}
				}
			}
			// OffhandAbilities
			if (Definition.OffhandAbilities.Num() > 0)
			{
				FArrayProperty* OffhandProp = CastField<FArrayProperty>(CDO->GetClass()->FindPropertyByName(TEXT("OffhandAbilities")));
				if (OffhandProp)
				{
					FScriptArrayHelper ArrayHelper(OffhandProp, OffhandProp->ContainerPtrToValuePtr<void>(CDO));
					for (const FString& AbilityName : Definition.OffhandAbilities)
					{
						UClass* AbilityClass = LoadAbilityClassLocal(AbilityName);
						if (AbilityClass)
						{
							int32 NewIndex = ArrayHelper.AddValue();
							FClassProperty* InnerProp = CastField<FClassProperty>(OffhandProp->Inner);
							if (InnerProp)
							{
								InnerProp->SetObjectPropertyValue(ArrayHelper.GetRawPtr(NewIndex), AbilityClass);
								LogGeneration(FString::Printf(TEXT("  Added OffhandAbility: %s"), *AbilityClass->GetName()));
							}
						}
					}
				}
			}

			CDO->MarkPackageDirty();
		}
	}

	// Recompile to ensure changes are reflected
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Blueprint);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Equippable Item: %s"), *Definition.Name));

	// v3.0: Store metadata for regeneration tracking
	StoreBlueprintMetadata(Blueprint, TEXT("EquippableItem"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// v2.6.0: Activity Generator - sets BehaviourTree on CDO
// v2.6.9: Added deferred handling for BehaviorTree dependency
FGenerationResult FActivityGenerator::Generate(const FManifestActivityDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("AI/Activities") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Activity"), Result))
	{
		return Result;
	}

	// v3.0: Check existence with metadata-aware logic for MODIFY/CONFLICT detection
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Activity"), Definition.ComputeHash(), Result))
	{
		return Result;
	}

	// v2.6.9: Check BehaviorTree dependency BEFORE creating asset
	UBehaviorTree* PreloadedBT = nullptr;
	if (!Definition.BehaviorTree.IsEmpty())
	{
		// Try direct path first
		PreloadedBT = LoadObject<UBehaviorTree>(nullptr, *Definition.BehaviorTree);

		// Try common paths if not found
		if (!PreloadedBT)
		{
			TArray<FString> SearchPaths = {
				FString::Printf(TEXT("%s/AI/BehaviorTrees/%s"), *GetProjectRoot(), *Definition.BehaviorTree),
				FString::Printf(TEXT("%s/AI/%s"), *GetProjectRoot(), *Definition.BehaviorTree),
				FString::Printf(TEXT("/Game/AI/BehaviorTrees/%s"), *Definition.BehaviorTree)
			};

			for (const FString& SearchPath : SearchPaths)
			{
				PreloadedBT = LoadObject<UBehaviorTree>(nullptr, *SearchPath);
				if (PreloadedBT) break;
			}
		}

		// If still not found, defer
		if (!PreloadedBT)
		{
			LogGeneration(FString::Printf(TEXT("  Deferring: BehaviorTree '%s' not found yet"), *Definition.BehaviorTree));
			Result = FGenerationResult(Definition.Name, EGenerationStatus::Deferred,
				FString::Printf(TEXT("Missing dependency: %s"), *Definition.BehaviorTree));
			Result.MissingDependency = Definition.BehaviorTree;
			Result.MissingDependencyType = TEXT("BehaviorTree");
			Result.DetermineCategory();
			return Result;
		}
	}

	// Find UNPCActivity class
	UClass* ParentClass = UNPCActivity::StaticClass();
	if (!ParentClass)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("UNPCActivity class not found"));
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	// Create Blueprint factory
	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = ParentClass;

	UBlueprint* Blueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
		UBlueprint::StaticClass(), Package, *Definition.Name,
		RF_Public | RF_Standalone, nullptr, GWarn));

	if (!Blueprint)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Activity Blueprint"));
	}

	// Compile blueprint first
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// v2.6.9: Use pre-loaded BehaviorTree from deferred check
	if (Blueprint->GeneratedClass)
	{
		UObject* CDO = Blueprint->GeneratedClass->GetDefaultObject();
		if (CDO)
		{
			// Set BehaviourTree
			if (PreloadedBT)
			{
				FObjectProperty* BTProperty = CastField<FObjectProperty>(CDO->GetClass()->FindPropertyByName(TEXT("BehaviourTree")));
				if (BTProperty)
				{
					BTProperty->SetObjectPropertyValue_InContainer(CDO, PreloadedBT);
					LogGeneration(FString::Printf(TEXT("  Set BehaviourTree: %s"), *PreloadedBT->GetName()));
				}
			}

			// v3.3: Set NarrativeActivityBase properties
			if (!Definition.ActivityName.IsEmpty())
			{
				FTextProperty* NameProp = CastField<FTextProperty>(CDO->GetClass()->FindPropertyByName(TEXT("ActivityName")));
				if (NameProp)
				{
					NameProp->SetPropertyValue_InContainer(CDO, FText::FromString(Definition.ActivityName));
					LogGeneration(FString::Printf(TEXT("  Set ActivityName: %s"), *Definition.ActivityName));
				}
			}
			// OwnedTags
			if (Definition.OwnedTags.Num() > 0)
			{
				FStructProperty* TagsProp = CastField<FStructProperty>(CDO->GetClass()->FindPropertyByName(TEXT("OwnedTags")));
				if (TagsProp)
				{
					FGameplayTagContainer* TagContainer = TagsProp->ContainerPtrToValuePtr<FGameplayTagContainer>(CDO);
					if (TagContainer)
					{
						for (const FString& TagString : Definition.OwnedTags)
						{
							FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
							if (Tag.IsValid())
							{
								TagContainer->AddTag(Tag);
								LogGeneration(FString::Printf(TEXT("  Added OwnedTag: %s"), *TagString));
							}
						}
					}
				}
			}
			// BlockTags
			if (Definition.BlockTags.Num() > 0)
			{
				FStructProperty* TagsProp = CastField<FStructProperty>(CDO->GetClass()->FindPropertyByName(TEXT("BlockTags")));
				if (TagsProp)
				{
					FGameplayTagContainer* TagContainer = TagsProp->ContainerPtrToValuePtr<FGameplayTagContainer>(CDO);
					if (TagContainer)
					{
						for (const FString& TagString : Definition.BlockTags)
						{
							FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
							if (Tag.IsValid())
							{
								TagContainer->AddTag(Tag);
								LogGeneration(FString::Printf(TEXT("  Added BlockTag: %s"), *TagString));
							}
						}
					}
				}
			}
			// RequireTags
			if (Definition.RequireTags.Num() > 0)
			{
				FStructProperty* TagsProp = CastField<FStructProperty>(CDO->GetClass()->FindPropertyByName(TEXT("RequireTags")));
				if (TagsProp)
				{
					FGameplayTagContainer* TagContainer = TagsProp->ContainerPtrToValuePtr<FGameplayTagContainer>(CDO);
					if (TagContainer)
					{
						for (const FString& TagString : Definition.RequireTags)
						{
							FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
							if (Tag.IsValid())
							{
								TagContainer->AddTag(Tag);
								LogGeneration(FString::Printf(TEXT("  Added RequireTag: %s"), *TagString));
							}
						}
					}
				}
			}

			// v3.3: Set NPCActivity properties
			// SupportedGoalType
			if (!Definition.SupportedGoalType.IsEmpty())
			{
				UClass* GoalClass = FindParentClass(Definition.SupportedGoalType);
				if (GoalClass)
				{
					FClassProperty* GoalProp = CastField<FClassProperty>(CDO->GetClass()->FindPropertyByName(TEXT("SupportedGoalType")));
					if (GoalProp)
					{
						GoalProp->SetObjectPropertyValue_InContainer(CDO, GoalClass);
						LogGeneration(FString::Printf(TEXT("  Set SupportedGoalType: %s"), *Definition.SupportedGoalType));
					}
				}
			}
			// bIsInterruptable (default is true, only set if false)
			if (!Definition.bIsInterruptable)
			{
				FBoolProperty* InterruptProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bIsInterruptable")));
				if (InterruptProp)
				{
					InterruptProp->SetPropertyValue_InContainer(CDO, false);
					LogGeneration(TEXT("  Set bIsInterruptable: false"));
				}
			}
			// bSaveActivity
			if (Definition.bSaveActivity)
			{
				FBoolProperty* SaveProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bSaveActivity")));
				if (SaveProp)
				{
					SaveProp->SetPropertyValue_InContainer(CDO, true);
					LogGeneration(TEXT("  Set bSaveActivity: true"));
				}
			}

			CDO->MarkPackageDirty();
		}
	}

	// Recompile
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Blueprint);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Activity: %s"), *Definition.Name));

	// v3.0: Store metadata for regeneration tracking
	StoreBlueprintMetadata(Blueprint, TEXT("Activity"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// v2.3.0: Ability Configuration Generator
// v3.1: Now populates DefaultAbilities, StartupEffects, and DefaultAttributes via class resolution
FGenerationResult FAbilityConfigurationGenerator::Generate(const FManifestAbilityConfigurationDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Configurations") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Ability Configuration"), Result))
	{
		return Result;
	}

	// v3.0: Use metadata-aware existence check
	uint64 InputHash = Definition.ComputeHash();
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Ability Configuration"), InputHash, Result))
	{
		return Result;
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	UAbilityConfiguration* AbilityConfig = NewObject<UAbilityConfiguration>(Package, *Definition.Name, RF_Public | RF_Standalone);
	if (!AbilityConfig)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Ability Configuration"));
	}

	// v3.1: Resolve DefaultAbilities (GA_ blueprints -> NarrativeGameplayAbility subclasses)
	int32 ResolvedAbilities = 0;
	for (const FString& AbilityName : Definition.Abilities)
	{
		// Try multiple paths for the ability blueprint
		TArray<FString> SearchPaths;
		SearchPaths.Add(FString::Printf(TEXT("%s/Abilities/%s.%s_C"), *GetProjectRoot(), *AbilityName, *AbilityName));
		SearchPaths.Add(FString::Printf(TEXT("%s/Abilities/Crawler/%s.%s_C"), *GetProjectRoot(), *AbilityName, *AbilityName));
		SearchPaths.Add(FString::Printf(TEXT("%s/Abilities/Forms/%s.%s_C"), *GetProjectRoot(), *AbilityName, *AbilityName));
		SearchPaths.Add(FString::Printf(TEXT("/Game/FatherCompanion/Abilities/%s.%s_C"), *AbilityName, *AbilityName));

		UClass* AbilityClass = nullptr;
		for (const FString& Path : SearchPaths)
		{
			AbilityClass = LoadClass<UNarrativeGameplayAbility>(nullptr, *Path);
			if (AbilityClass)
			{
				break;
			}
		}

		if (AbilityClass)
		{
			AbilityConfig->DefaultAbilities.Add(AbilityClass);
			ResolvedAbilities++;
			LogGeneration(FString::Printf(TEXT("  Resolved ability: %s"), *AbilityName));
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  [WARNING] Could not resolve ability: %s (will need manual assignment)"), *AbilityName));
		}
	}

	// v3.1: Resolve StartupEffects (GE_ blueprints -> GameplayEffect subclasses)
	int32 ResolvedEffects = 0;
	for (const FString& EffectName : Definition.StartupEffects)
	{
		TArray<FString> SearchPaths;
		SearchPaths.Add(FString::Printf(TEXT("%s/Effects/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
		SearchPaths.Add(FString::Printf(TEXT("%s/GameplayEffects/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
		SearchPaths.Add(FString::Printf(TEXT("/Game/FatherCompanion/Effects/%s.%s_C"), *EffectName, *EffectName));

		UClass* EffectClass = nullptr;
		for (const FString& Path : SearchPaths)
		{
			EffectClass = LoadClass<UGameplayEffect>(nullptr, *Path);
			if (EffectClass)
			{
				break;
			}
		}

		if (EffectClass)
		{
			AbilityConfig->StartupEffects.Add(EffectClass);
			ResolvedEffects++;
			LogGeneration(FString::Printf(TEXT("  Resolved startup effect: %s"), *EffectName));
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  [WARNING] Could not resolve startup effect: %s"), *EffectName));
		}
	}

	// v3.1: Resolve DefaultAttributes (single GE_ for attribute initialization)
	if (!Definition.DefaultAttributes.IsEmpty())
	{
		TArray<FString> SearchPaths;
		SearchPaths.Add(FString::Printf(TEXT("%s/Effects/%s.%s_C"), *GetProjectRoot(), *Definition.DefaultAttributes, *Definition.DefaultAttributes));
		SearchPaths.Add(FString::Printf(TEXT("%s/GameplayEffects/%s.%s_C"), *GetProjectRoot(), *Definition.DefaultAttributes, *Definition.DefaultAttributes));
		SearchPaths.Add(FString::Printf(TEXT("/Game/FatherCompanion/Effects/%s.%s_C"), *Definition.DefaultAttributes, *Definition.DefaultAttributes));

		UClass* AttrClass = nullptr;
		for (const FString& Path : SearchPaths)
		{
			AttrClass = LoadClass<UGameplayEffect>(nullptr, *Path);
			if (AttrClass)
			{
				break;
			}
		}

		if (AttrClass)
		{
			AbilityConfig->DefaultAttributes = AttrClass;
			LogGeneration(FString::Printf(TEXT("  Resolved default attributes: %s"), *Definition.DefaultAttributes));
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  [WARNING] Could not resolve default attributes: %s"), *Definition.DefaultAttributes));
		}
	}

	LogGeneration(FString::Printf(TEXT("  Summary: %d/%d abilities, %d/%d effects resolved"),
		ResolvedAbilities, Definition.Abilities.Num(),
		ResolvedEffects, Definition.StartupEffects.Num()));

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(AbilityConfig);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, AbilityConfig, *PackageFileName, SaveArgs);

	// v3.0: Store metadata after successful generation
	StoreDataAssetMetadata(AbilityConfig, TEXT("AC"), Definition.Name, InputHash);

	LogGeneration(FString::Printf(TEXT("Created Ability Configuration: %s"), *Definition.Name));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// v2.3.0: Activity Configuration Generator - creates UNPCActivityConfiguration data assets
// v3.1: Now populates DefaultActivities and GoalGenerators via class resolution
FGenerationResult FActivityConfigurationGenerator::Generate(const FManifestActivityConfigurationDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Configurations") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Activity Configuration"), Result))
	{
		return Result;
	}

	// v3.0: Use metadata-aware existence check
	uint64 InputHash = Definition.ComputeHash();
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Activity Configuration"), InputHash, Result))
	{
		return Result;
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	UNPCActivityConfiguration* ActivityConfig = NewObject<UNPCActivityConfiguration>(Package, *Definition.Name, RF_Public | RF_Standalone);
	if (!ActivityConfig)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Activity Configuration"));
	}

	// Set rescore interval if specified
	ActivityConfig->RescoreInterval = Definition.RescoreInterval;

	// v3.1: Resolve DefaultActivities (BPA_ blueprints -> UNPCActivity subclasses)
	int32 ResolvedActivities = 0;
	for (const FString& ActivityName : Definition.Activities)
	{
		TArray<FString> SearchPaths;
		SearchPaths.Add(FString::Printf(TEXT("%s/Activities/%s.%s_C"), *GetProjectRoot(), *ActivityName, *ActivityName));
		SearchPaths.Add(FString::Printf(TEXT("%s/AI/Activities/%s.%s_C"), *GetProjectRoot(), *ActivityName, *ActivityName));
		SearchPaths.Add(FString::Printf(TEXT("/Game/FatherCompanion/Activities/%s.%s_C"), *ActivityName, *ActivityName));
		// Also check Narrative Pro plugin for built-in activities
		SearchPaths.Add(FString::Printf(TEXT("/NarrativePro/AI/Activities/%s.%s_C"), *ActivityName, *ActivityName));

		UClass* ActivityClass = nullptr;
		for (const FString& Path : SearchPaths)
		{
			ActivityClass = LoadClass<UNPCActivity>(nullptr, *Path);
			if (ActivityClass)
			{
				break;
			}
		}

		if (ActivityClass)
		{
			ActivityConfig->DefaultActivities.Add(ActivityClass);
			ResolvedActivities++;
			LogGeneration(FString::Printf(TEXT("  Resolved activity: %s"), *ActivityName));
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  [WARNING] Could not resolve activity: %s (will need manual assignment)"), *ActivityName));
		}
	}

	// v3.1: Resolve GoalGenerators (GoalGenerator_ blueprints -> UNPCGoalGenerator subclasses)
	int32 ResolvedGenerators = 0;
	for (const FString& GeneratorName : Definition.GoalGenerators)
	{
		TArray<FString> SearchPaths;
		SearchPaths.Add(FString::Printf(TEXT("%s/Goals/%s.%s_C"), *GetProjectRoot(), *GeneratorName, *GeneratorName));
		SearchPaths.Add(FString::Printf(TEXT("%s/AI/Goals/%s.%s_C"), *GetProjectRoot(), *GeneratorName, *GeneratorName));
		SearchPaths.Add(FString::Printf(TEXT("/Game/FatherCompanion/Goals/%s.%s_C"), *GeneratorName, *GeneratorName));
		// Also check Narrative Pro plugin for built-in generators
		SearchPaths.Add(FString::Printf(TEXT("/NarrativePro/AI/Goals/%s.%s_C"), *GeneratorName, *GeneratorName));

		UClass* GeneratorClass = nullptr;
		for (const FString& Path : SearchPaths)
		{
			GeneratorClass = LoadClass<UNPCGoalGenerator>(nullptr, *Path);
			if (GeneratorClass)
			{
				break;
			}
		}

		if (GeneratorClass)
		{
			ActivityConfig->GoalGenerators.Add(GeneratorClass);
			ResolvedGenerators++;
			LogGeneration(FString::Printf(TEXT("  Resolved goal generator: %s"), *GeneratorName));
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  [WARNING] Could not resolve goal generator: %s"), *GeneratorName));
		}
	}

	LogGeneration(FString::Printf(TEXT("  Summary: %d/%d activities, %d/%d generators resolved"),
		ResolvedActivities, Definition.Activities.Num(),
		ResolvedGenerators, Definition.GoalGenerators.Num()));

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(ActivityConfig);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, ActivityConfig, *PackageFileName, SaveArgs);

	// v3.0: Store metadata after successful generation
	StoreDataAssetMetadata(ActivityConfig, TEXT("ActConfig"), Definition.Name, InputHash);

	LogGeneration(FString::Printf(TEXT("Created Activity Configuration: %s"), *Definition.Name));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// v2.3.0: Item Collection Generator - creates UItemCollection data assets
FGenerationResult FItemCollectionGenerator::Generate(const FManifestItemCollectionDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Items/Collections") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Item Collection"), Result))
	{
		return Result;
	}

	// v3.0: Use metadata-aware existence check
	uint64 InputHash = Definition.ComputeHash();
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Item Collection"), InputHash, Result))
	{
		return Result;
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	UItemCollection* ItemColl = NewObject<UItemCollection>(Package, *Definition.Name, RF_Public | RF_Standalone);
	if (!ItemColl)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Item Collection"));
	}

	// v2.6.0: Add items with quantities via TSoftClassPtr
	int32 ItemsAdded = 0;

	// Process items with quantities (new format)
	for (const FManifestItemWithQuantity& ItemDef : Definition.ItemsWithQuantity)
	{
		FString ItemPath = ItemDef.ItemClass;
		// Build full path if relative
		if (!ItemPath.Contains(TEXT("/")))
		{
			ItemPath = FString::Printf(TEXT("%s/Items/%s.%s_C"), *GetProjectRoot(), *ItemDef.ItemClass, *ItemDef.ItemClass);
		}
		else if (!ItemPath.EndsWith(TEXT("_C")))
		{
			ItemPath = ItemPath + TEXT("_C");
		}

		FItemWithQuantity NewItem;
		NewItem.Item = TSoftClassPtr<UNarrativeItem>(FSoftObjectPath(ItemPath));
		NewItem.Quantity = ItemDef.Quantity;
		ItemColl->Items.Add(NewItem);
		ItemsAdded++;
		LogGeneration(FString::Printf(TEXT("  Added item: %s x%d"), *ItemDef.ItemClass, ItemDef.Quantity));
	}

	// Process simple item names (legacy format) with quantity 1
	for (const FString& ItemName : Definition.Items)
	{
		FString ItemPath = ItemName;
		if (!ItemPath.Contains(TEXT("/")))
		{
			ItemPath = FString::Printf(TEXT("%s/Items/%s.%s_C"), *GetProjectRoot(), *ItemName, *ItemName);
		}
		else if (!ItemPath.EndsWith(TEXT("_C")))
		{
			ItemPath = ItemPath + TEXT("_C");
		}

		FItemWithQuantity NewItem;
		NewItem.Item = TSoftClassPtr<UNarrativeItem>(FSoftObjectPath(ItemPath));
		NewItem.Quantity = 1;
		ItemColl->Items.Add(NewItem);
		ItemsAdded++;
		LogGeneration(FString::Printf(TEXT("  Added item: %s x1"), *ItemName));
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(ItemColl);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, ItemColl, *PackageFileName, SaveArgs);

	// v3.0: Store metadata after successful generation
	StoreDataAssetMetadata(ItemColl, TEXT("IC"), Definition.Name, InputHash);

	LogGeneration(FString::Printf(TEXT("Created Item Collection: %s with %d items"), *Definition.Name, ItemsAdded));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// v2.3.0: Narrative Event Generator - creates UNarrativeEvent Blueprint assets
FGenerationResult FNarrativeEventGenerator::Generate(const FManifestNarrativeEventDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Events") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Narrative Event"), Result))
	{
		return Result;
	}

	// v3.0: Use metadata-aware existence check
	uint64 InputHash = Definition.ComputeHash();
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Narrative Event"), InputHash, Result))
	{
		return Result;
	}

	// Find UNarrativeEvent class
	UClass* ParentClass = UNarrativeEvent::StaticClass();
	if (!ParentClass)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("UNarrativeEvent class not found"));
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	// Create Blueprint factory
	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = ParentClass;

	UBlueprint* Blueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
		UBlueprint::StaticClass(), Package, *Definition.Name,
		RF_Public | RF_Standalone, nullptr, GWarn));

	if (!Blueprint)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Narrative Event Blueprint"));
	}

	// Compile blueprint
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// v3.2: Set event configuration properties on CDO
	UObject* CDO = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
	if (CDO)
	{
		// Set EventRuntime enum via reflection
		if (!Definition.EventRuntime.IsEmpty())
		{
			FByteProperty* RuntimeProp = CastField<FByteProperty>(CDO->GetClass()->FindPropertyByName(TEXT("EventRuntime")));
			if (RuntimeProp)
			{
				int32 RuntimeValue = 0;  // Default: Start
				if (Definition.EventRuntime.Equals(TEXT("End"), ESearchCase::IgnoreCase)) RuntimeValue = 1;
				else if (Definition.EventRuntime.Equals(TEXT("Both"), ESearchCase::IgnoreCase)) RuntimeValue = 2;
				RuntimeProp->SetPropertyValue_InContainer(CDO, static_cast<uint8>(RuntimeValue));
				LogGeneration(FString::Printf(TEXT("  Set EventRuntime: %s"), *Definition.EventRuntime));
			}
		}

		// Set EventFilter enum via reflection
		if (!Definition.EventFilter.IsEmpty())
		{
			FByteProperty* FilterProp = CastField<FByteProperty>(CDO->GetClass()->FindPropertyByName(TEXT("EventFilter")));
			if (FilterProp)
			{
				int32 FilterValue = 0;  // Default: Anyone
				if (Definition.EventFilter.Equals(TEXT("OnlyNPCs"), ESearchCase::IgnoreCase)) FilterValue = 1;
				else if (Definition.EventFilter.Equals(TEXT("OnlyPlayers"), ESearchCase::IgnoreCase)) FilterValue = 2;
				FilterProp->SetPropertyValue_InContainer(CDO, static_cast<uint8>(FilterValue));
				LogGeneration(FString::Printf(TEXT("  Set EventFilter: %s"), *Definition.EventFilter));
			}
		}

		// Set PartyEventPolicy enum via reflection
		if (!Definition.PartyEventPolicy.IsEmpty())
		{
			FByteProperty* PartyProp = CastField<FByteProperty>(CDO->GetClass()->FindPropertyByName(TEXT("PartyEventPolicy")));
			if (PartyProp)
			{
				int32 PartyValue = 0;  // Default: Party
				if (Definition.PartyEventPolicy.Equals(TEXT("AllPartyMembers"), ESearchCase::IgnoreCase)) PartyValue = 1;
				else if (Definition.PartyEventPolicy.Equals(TEXT("PartyLeader"), ESearchCase::IgnoreCase)) PartyValue = 2;
				PartyProp->SetPropertyValue_InContainer(CDO, static_cast<uint8>(PartyValue));
				LogGeneration(FString::Printf(TEXT("  Set PartyEventPolicy: %s"), *Definition.PartyEventPolicy));
			}
		}

		// Set bRefireOnLoad bool property
		if (Definition.bRefireOnLoad)
		{
			FBoolProperty* RefireProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bRefireOnLoad")));
			if (RefireProp)
			{
				RefireProp->SetPropertyValue_InContainer(CDO, true);
				LogGeneration(TEXT("  Set bRefireOnLoad: true"));
			}
		}

		// Recompile after setting CDO properties
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
	}

	// Log event details for manual setup
	if (!Definition.EventType.IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("  Event '%s' Type: %s"), *Definition.Name, *Definition.EventType));
	}
	if (!Definition.Description.IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("  Event '%s' Description: %s"), *Definition.Name, *Definition.Description));
	}
	// v3.2: Log target arrays for manual setup (soft object references require manual setup)
	if (Definition.NPCTargets.Num() > 0)
	{
		LogGeneration(FString::Printf(TEXT("  Event '%s' NPCTargets (%d): %s"), *Definition.Name, Definition.NPCTargets.Num(), *FString::Join(Definition.NPCTargets, TEXT(", "))));
	}
	if (Definition.CharacterTargets.Num() > 0)
	{
		LogGeneration(FString::Printf(TEXT("  Event '%s' CharacterTargets (%d): %s"), *Definition.Name, Definition.CharacterTargets.Num(), *FString::Join(Definition.CharacterTargets, TEXT(", "))));
	}
	if (Definition.PlayerTargets.Num() > 0)
	{
		LogGeneration(FString::Printf(TEXT("  Event '%s' PlayerTargets (%d): %s"), *Definition.Name, Definition.PlayerTargets.Num(), *FString::Join(Definition.PlayerTargets, TEXT(", "))));
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Blueprint);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	// v3.0: Store metadata after successful generation
	StoreBlueprintMetadata(Blueprint, TEXT("NE"), Definition.Name, InputHash);

	LogGeneration(FString::Printf(TEXT("Created Narrative Event: %s"), *Definition.Name));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// v2.3.0: NPC Definition Generator - creates UNPCDefinition data assets
// v2.6.9: Added deferred handling for AbilityConfiguration dependency
FGenerationResult FNPCDefinitionGenerator::Generate(const FManifestNPCDefinitionDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("NPCs/Definitions") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("NPC Definition"), Result))
	{
		return Result;
	}

	// v3.0: Use metadata-aware existence check
	uint64 InputHash = Definition.ComputeHash();
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("NPC Definition"), InputHash, Result))
	{
		return Result;
	}

	// v2.6.9: Check AbilityConfiguration dependency BEFORE creating asset
	UAbilityConfiguration* PreloadedAbilityConfig = nullptr;
	if (!Definition.AbilityConfiguration.IsEmpty())
	{
		FString ConfigPath = Definition.AbilityConfiguration;
		if (!ConfigPath.Contains(TEXT("/")))
		{
			ConfigPath = FString::Printf(TEXT("%s/GAS/Configurations/%s"), *GetProjectRoot(), *Definition.AbilityConfiguration);
		}
		PreloadedAbilityConfig = LoadObject<UAbilityConfiguration>(nullptr, *ConfigPath);

		// Try alternate paths
		if (!PreloadedAbilityConfig)
		{
			TArray<FString> SearchPaths = {
				FString::Printf(TEXT("%s/Configs/%s"), *GetProjectRoot(), *Definition.AbilityConfiguration),
				FString::Printf(TEXT("%s/Abilities/Configurations/%s"), *GetProjectRoot(), *Definition.AbilityConfiguration),
				FString::Printf(TEXT("%s/AI/Configurations/%s"), *GetProjectRoot(), *Definition.AbilityConfiguration)
			};

			for (const FString& SearchPath : SearchPaths)
			{
				PreloadedAbilityConfig = LoadObject<UAbilityConfiguration>(nullptr, *SearchPath);
				if (PreloadedAbilityConfig) break;
			}
		}

		// If still not found, defer
		if (!PreloadedAbilityConfig)
		{
			LogGeneration(FString::Printf(TEXT("  Deferring: AbilityConfiguration '%s' not found yet"), *Definition.AbilityConfiguration));
			Result = FGenerationResult(Definition.Name, EGenerationStatus::Deferred,
				FString::Printf(TEXT("Missing dependency: %s"), *Definition.AbilityConfiguration));
			Result.MissingDependency = Definition.AbilityConfiguration;
			Result.MissingDependencyType = TEXT("AbilityConfiguration");
			Result.DetermineCategory();
			return Result;
		}
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	UNPCDefinition* NPCDef = NewObject<UNPCDefinition>(Package, *Definition.Name, RF_Public | RF_Standalone);
	if (!NPCDef)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create NPC Definition"));
	}

	// Set properties from definition
	NPCDef->NPCID = FName(*Definition.NPCID);
	NPCDef->NPCName = FText::FromString(Definition.NPCName);
	NPCDef->MinLevel = Definition.MinLevel;
	NPCDef->MaxLevel = Definition.MaxLevel;
	NPCDef->bAllowMultipleInstances = Definition.bAllowMultipleInstances;
	NPCDef->bIsVendor = Definition.bIsVendor;

	// v2.6.0: Set NPCClassPath via TSoftClassPtr
	if (!Definition.NPCClassPath.IsEmpty())
	{
		FString ClassPath = Definition.NPCClassPath;
		// Ensure it has _C suffix for Blueprint classes
		if (!ClassPath.EndsWith(TEXT("_C")))
		{
			// Check if it's a relative path
			if (!ClassPath.Contains(TEXT("/")))
			{
				ClassPath = FString::Printf(TEXT("%s/NPCs/%s.%s_C"), *GetProjectRoot(), *Definition.NPCClassPath, *Definition.NPCClassPath);
			}
			else if (!ClassPath.EndsWith(TEXT("_C")))
			{
				// Add _C suffix to the asset name
				ClassPath = ClassPath + TEXT("_C");
			}
		}
		NPCDef->NPCClassPath = TSoftClassPtr<ANarrativeNPCCharacter>(FSoftObjectPath(ClassPath));
		LogGeneration(FString::Printf(TEXT("  Set NPCClassPath: %s"), *ClassPath));
	}

	// v2.6.0: Set ActivityConfiguration via TSoftObjectPtr
	if (!Definition.ActivityConfiguration.IsEmpty())
	{
		FString ConfigPath = Definition.ActivityConfiguration;
		if (!ConfigPath.Contains(TEXT("/")))
		{
			ConfigPath = FString::Printf(TEXT("%s/AI/Configurations/%s"), *GetProjectRoot(), *Definition.ActivityConfiguration);
		}
		NPCDef->ActivityConfiguration = TSoftObjectPtr<UNPCActivityConfiguration>(FSoftObjectPath(ConfigPath));
		LogGeneration(FString::Printf(TEXT("  Set ActivityConfiguration: %s"), *ConfigPath));
	}

	// v2.6.9: Use pre-loaded AbilityConfiguration from deferred check
	if (PreloadedAbilityConfig)
	{
		NPCDef->AbilityConfiguration = PreloadedAbilityConfig;
		LogGeneration(FString::Printf(TEXT("  Set AbilityConfiguration: %s"), *PreloadedAbilityConfig->GetName()));
	}

	// v3.3: Set Dialogue via TSoftClassPtr
	if (!Definition.Dialogue.IsEmpty())
	{
		FString DialoguePath = Definition.Dialogue;
		if (!DialoguePath.Contains(TEXT("/")))
		{
			DialoguePath = FString::Printf(TEXT("%s/Dialogues/%s.%s_C"), *GetProjectRoot(), *Definition.Dialogue, *Definition.Dialogue);
		}
		else if (!DialoguePath.EndsWith(TEXT("_C")))
		{
			DialoguePath = DialoguePath + TEXT("_C");
		}
		NPCDef->Dialogue = TSoftClassPtr<UDialogue>(FSoftObjectPath(DialoguePath));
		LogGeneration(FString::Printf(TEXT("  Set Dialogue: %s"), *DialoguePath));
	}

	// v3.3: Set TaggedDialogueSet via TSoftObjectPtr
	if (!Definition.TaggedDialogueSet.IsEmpty())
	{
		FString TDSPath = Definition.TaggedDialogueSet;
		if (!TDSPath.Contains(TEXT("/")))
		{
			TDSPath = FString::Printf(TEXT("%s/Dialogues/%s"), *GetProjectRoot(), *Definition.TaggedDialogueSet);
		}
		NPCDef->TaggedDialogueSet = TSoftObjectPtr<UTaggedDialogueSet>(FSoftObjectPath(TDSPath));
		LogGeneration(FString::Printf(TEXT("  Set TaggedDialogueSet: %s"), *TDSPath));
	}

	// v3.3: Set vendor properties
	if (Definition.bIsVendor)
	{
		NPCDef->TradingCurrency = Definition.TradingCurrency;
		NPCDef->BuyItemPercentage = Definition.BuyItemPercentage;
		NPCDef->SellItemPercentage = Definition.SellItemPercentage;
		if (!Definition.ShopFriendlyName.IsEmpty())
		{
			NPCDef->ShopFriendlyName = FText::FromString(Definition.ShopFriendlyName);
		}
		LogGeneration(FString::Printf(TEXT("  Vendor config: Currency=%d, Buy=%.1f%%, Sell=%.1f%%"),
			Definition.TradingCurrency, Definition.BuyItemPercentage * 100.f, Definition.SellItemPercentage * 100.f));
	}

	// v3.3: Set CharacterDefinition inherited properties
	NPCDef->DefaultCurrency = Definition.DefaultCurrency;
	NPCDef->AttackPriority = Definition.AttackPriority;

	// v3.3: Set DefaultAppearance via TSoftObjectPtr
	if (!Definition.DefaultAppearance.IsEmpty())
	{
		FString AppearancePath = Definition.DefaultAppearance;
		if (!AppearancePath.Contains(TEXT("/")))
		{
			AppearancePath = FString::Printf(TEXT("%s/Appearances/%s"), *GetProjectRoot(), *Definition.DefaultAppearance);
		}
		NPCDef->DefaultAppearance = TSoftObjectPtr<UCharacterAppearanceBase>(FSoftObjectPath(AppearancePath));
		LogGeneration(FString::Printf(TEXT("  Set DefaultAppearance: %s"), *AppearancePath));
	}

	// v3.3: Set DefaultOwnedTags
	if (Definition.DefaultOwnedTags.Num() > 0)
	{
		for (const FString& TagString : Definition.DefaultOwnedTags)
		{
			FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
			if (Tag.IsValid())
			{
				NPCDef->DefaultOwnedTags.AddTag(Tag);
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("  [WARNING] Tag not found: %s"), *TagString));
			}
		}
		LogGeneration(FString::Printf(TEXT("  Set %d DefaultOwnedTags"), NPCDef->DefaultOwnedTags.Num()));
	}

	// v3.3: Set DefaultFactions
	if (Definition.DefaultFactions.Num() > 0)
	{
		for (const FString& FactionString : Definition.DefaultFactions)
		{
			FGameplayTag Faction = FGameplayTag::RequestGameplayTag(FName(*FactionString), false);
			if (Faction.IsValid())
			{
				NPCDef->DefaultFactions.AddTag(Faction);
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("  [WARNING] Faction tag not found: %s"), *FactionString));
			}
		}
		LogGeneration(FString::Printf(TEXT("  Set %d DefaultFactions"), NPCDef->DefaultFactions.Num()));
	}

	// v3.6: Set ActivitySchedules array (TArray<TSoftObjectPtr<UNPCActivitySchedule>>)
	if (Definition.ActivitySchedules.Num() > 0)
	{
		FArrayProperty* SchedulesProperty = CastField<FArrayProperty>(NPCDef->GetClass()->FindPropertyByName(TEXT("ActivitySchedules")));
		if (SchedulesProperty)
		{
			FScriptArrayHelper ArrayHelper(SchedulesProperty, SchedulesProperty->ContainerPtrToValuePtr<void>(NPCDef));
			for (const FString& SchedulePath : Definition.ActivitySchedules)
			{
				FString FullPath = SchedulePath;
				if (!FullPath.Contains(TEXT("/")))
				{
					FullPath = FString::Printf(TEXT("%s/AI/Schedules/%s"), *GetProjectRoot(), *SchedulePath);
				}
				FSoftObjectPath SoftPath(FullPath);
				int32 NewIndex = ArrayHelper.AddValue();
				FSoftObjectPtr* SoftPtr = reinterpret_cast<FSoftObjectPtr*>(ArrayHelper.GetRawPtr(NewIndex));
				if (SoftPtr)
				{
					*SoftPtr = FSoftObjectPtr(SoftPath);
					LogGeneration(FString::Printf(TEXT("  Added ActivitySchedule: %s"), *SchedulePath));
				}
			}
		}
		else
		{
			LogGeneration(TEXT("  [WARNING] ActivitySchedules property not found on UNPCDefinition"));
		}
	}

	// v3.7: Auto-create dialogue blueprint if requested
	if (Definition.bAutoCreateDialogue)
	{
		// Derive dialogue name from NPC name: NPCDef_Blacksmith -> DBP_BlacksmithDialogue
		FString NPCBaseName = Definition.Name;
		NPCBaseName.RemoveFromStart(TEXT("NPCDef_"));
		FString DialogueName = FString::Printf(TEXT("DBP_%sDialogue"), *NPCBaseName);
		FString DialoguePath = FString::Printf(TEXT("%s/Dialogues/%s.%s_C"), *GetProjectRoot(), *DialogueName, *DialogueName);

		// Set the dialogue reference on the NPC
		NPCDef->Dialogue = TSoftClassPtr<UDialogue>(FSoftObjectPath(DialoguePath));
		LogGeneration(FString::Printf(TEXT("  [v3.7] Auto-create Dialogue: %s (will be generated separately)"), *DialogueName));
	}

	// v3.7: Auto-create tagged dialogue set if requested
	if (Definition.bAutoCreateTaggedDialogue)
	{
		// Derive tagged dialogue name from NPC name: NPCDef_Blacksmith -> Blacksmith_TaggedDialogue
		FString NPCBaseName = Definition.Name;
		NPCBaseName.RemoveFromStart(TEXT("NPCDef_"));
		FString TDSName = FString::Printf(TEXT("%s_TaggedDialogue"), *NPCBaseName);
		FString TDSPath = FString::Printf(TEXT("%s/Dialogues/%s"), *GetProjectRoot(), *TDSName);

		// Set the tagged dialogue set reference on the NPC
		NPCDef->TaggedDialogueSet = TSoftObjectPtr<UTaggedDialogueSet>(FSoftObjectPath(TDSPath));
		LogGeneration(FString::Printf(TEXT("  [v3.7] Auto-create TaggedDialogueSet: %s (will be generated separately)"), *TDSName));
	}

	// v3.7: Auto-populate DefaultItemLoadout with item collections if requested
	if (Definition.bAutoCreateItemLoadout && Definition.DefaultItemLoadoutCollections.Num() > 0)
	{
		// DefaultItemLoadout is FLootTableRoll struct containing ItemCollectionsToGrant (TArray<TObjectPtr<UItemCollection>>)
		FStructProperty* ItemLoadoutProp = CastField<FStructProperty>(NPCDef->GetClass()->FindPropertyByName(TEXT("DefaultItemLoadout")));
		if (ItemLoadoutProp)
		{
			void* ItemLoadoutPtr = ItemLoadoutProp->ContainerPtrToValuePtr<void>(NPCDef);

			// Find the ItemCollectionsToGrant array inside the FLootTableRoll struct
			FArrayProperty* CollectionsArrayProp = CastField<FArrayProperty>(ItemLoadoutProp->Struct->FindPropertyByName(TEXT("ItemCollectionsToGrant")));
			if (CollectionsArrayProp)
			{
				FScriptArrayHelper ArrayHelper(CollectionsArrayProp, CollectionsArrayProp->ContainerPtrToValuePtr<void>(ItemLoadoutPtr));

				for (const FString& CollectionName : Definition.DefaultItemLoadoutCollections)
				{
					// Try to load the item collection
					FString CollectionPath = CollectionName;
					if (!CollectionPath.Contains(TEXT("/")))
					{
						// Try common paths
						TArray<FString> SearchPaths = {
							FString::Printf(TEXT("%s/Items/%s"), *GetProjectRoot(), *CollectionName),
							FString::Printf(TEXT("%s/Items/Collections/%s"), *GetProjectRoot(), *CollectionName),
							FString::Printf(TEXT("/NarrativePro/Pro/Demo/Items/Examples/Items/Weapons/%s"), *CollectionName),
							FString::Printf(TEXT("/NarrativePro/Pro/Demo/Items/Examples/Items/Clothing/%s"), *CollectionName)
						};

						UObject* LoadedCollection = nullptr;
						for (const FString& SearchPath : SearchPaths)
						{
							LoadedCollection = LoadObject<UObject>(nullptr, *SearchPath);
							if (LoadedCollection)
							{
								CollectionPath = SearchPath;
								break;
							}
						}

						if (LoadedCollection)
						{
							int32 NewIndex = ArrayHelper.AddValue();
							TObjectPtr<UObject>* ObjPtr = reinterpret_cast<TObjectPtr<UObject>*>(ArrayHelper.GetRawPtr(NewIndex));
							if (ObjPtr)
							{
								*ObjPtr = LoadedCollection;
								LogGeneration(FString::Printf(TEXT("  [v3.7] Added ItemCollection: %s"), *CollectionName));
							}
						}
						else
						{
							LogGeneration(FString::Printf(TEXT("  [v3.7 WARNING] ItemCollection not found: %s"), *CollectionName));
						}
					}
					else
					{
						// Full path provided
						UObject* LoadedCollection = LoadObject<UObject>(nullptr, *CollectionPath);
						if (LoadedCollection)
						{
							int32 NewIndex = ArrayHelper.AddValue();
							TObjectPtr<UObject>* ObjPtr = reinterpret_cast<TObjectPtr<UObject>*>(ArrayHelper.GetRawPtr(NewIndex));
							if (ObjPtr)
							{
								*ObjPtr = LoadedCollection;
								LogGeneration(FString::Printf(TEXT("  [v3.7] Added ItemCollection: %s"), *CollectionName));
							}
						}
						else
						{
							LogGeneration(FString::Printf(TEXT("  [v3.7 WARNING] ItemCollection not found: %s"), *CollectionPath));
						}
					}
				}
				LogGeneration(FString::Printf(TEXT("  [v3.7] Set %d DefaultItemLoadout collections"), ArrayHelper.Num()));
			}
			else
			{
				LogGeneration(TEXT("  [v3.7 WARNING] ItemCollectionsToGrant property not found in FLootTableRoll"));
			}
		}
		else
		{
			LogGeneration(TEXT("  [v3.7 WARNING] DefaultItemLoadout property not found on UNPCDefinition"));
		}
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NPCDef);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, NPCDef, *PackageFileName, SaveArgs);

	// v3.0: Store metadata after successful generation
	StoreDataAssetMetadata(NPCDef, TEXT("NPCDef"), Definition.Name, InputHash);

	LogGeneration(FString::Printf(TEXT("Created NPC Definition: %s"), *Definition.Name));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// v2.3.0: Character Definition Generator - creates UCharacterDefinition data assets
FGenerationResult FCharacterDefinitionGenerator::Generate(const FManifestCharacterDefinitionDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Characters/Definitions") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Character Definition"), Result))
	{
		return Result;
	}

	// v3.0: Use metadata-aware existence check
	uint64 InputHash = Definition.ComputeHash();
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Character Definition"), InputHash, Result))
	{
		return Result;
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	UCharacterDefinition* CharDef = NewObject<UCharacterDefinition>(Package, *Definition.Name, RF_Public | RF_Standalone);
	if (!CharDef)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Character Definition"));
	}

	// Set properties from definition
	CharDef->DefaultCurrency = Definition.DefaultCurrency;
	CharDef->AttackPriority = Definition.AttackPriority;

	// v3.5: Parse tags from array
	if (Definition.DefaultOwnedTags.Num() > 0)
	{
		for (const FString& TagString : Definition.DefaultOwnedTags)
		{
			FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString.TrimStartAndEnd()), false);
			if (Tag.IsValid())
			{
				CharDef->DefaultOwnedTags.AddTag(Tag);
				LogGeneration(FString::Printf(TEXT("  Added DefaultOwnedTag: %s"), *TagString));
			}
		}
	}

	// v3.5: Parse factions from array
	if (Definition.DefaultFactions.Num() > 0)
	{
		for (const FString& FactionString : Definition.DefaultFactions)
		{
			FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*FactionString.TrimStartAndEnd()), false);
			if (Tag.IsValid())
			{
				CharDef->DefaultFactions.AddTag(Tag);
				LogGeneration(FString::Printf(TEXT("  Added DefaultFaction: %s"), *FactionString));
			}
		}
	}

	// v3.5: Set DefaultAppearance (TSoftObjectPtr<UCharacterAppearanceBase>) via reflection
	if (!Definition.DefaultAppearance.IsEmpty())
	{
		FSoftObjectProperty* AppearanceProp = CastField<FSoftObjectProperty>(CharDef->GetClass()->FindPropertyByName(TEXT("DefaultAppearance")));
		if (AppearanceProp)
		{
			FSoftObjectPath AppearancePath(Definition.DefaultAppearance);
			void* PropertyValue = AppearanceProp->ContainerPtrToValuePtr<void>(CharDef);
			FSoftObjectPtr* SoftPtr = reinterpret_cast<FSoftObjectPtr*>(PropertyValue);
			if (SoftPtr)
			{
				*SoftPtr = FSoftObjectPtr(AppearancePath);
				LogGeneration(FString::Printf(TEXT("  Set DefaultAppearance: %s"), *Definition.DefaultAppearance));
			}
		}
	}

	// v3.5: Set TriggerSets array (TArray<TSoftObjectPtr<UTriggerSet>>)
	if (Definition.TriggerSets.Num() > 0)
	{
		// Use reflection to access TriggerSets array
		FArrayProperty* TriggerSetsProperty = CastField<FArrayProperty>(CharDef->GetClass()->FindPropertyByName(TEXT("TriggerSets")));
		if (TriggerSetsProperty)
		{
			FScriptArrayHelper ArrayHelper(TriggerSetsProperty, TriggerSetsProperty->ContainerPtrToValuePtr<void>(CharDef));
			for (const FString& TriggerSetPath : Definition.TriggerSets)
			{
				FSoftObjectPath SoftPath(TriggerSetPath);
				int32 NewIndex = ArrayHelper.AddValue();
				FSoftObjectProperty* InnerProp = CastField<FSoftObjectProperty>(TriggerSetsProperty->Inner);
				if (InnerProp)
				{
					FSoftObjectPtr* SoftPtr = reinterpret_cast<FSoftObjectPtr*>(ArrayHelper.GetRawPtr(NewIndex));
					if (SoftPtr)
					{
						*SoftPtr = FSoftObjectPtr(SoftPath);
						LogGeneration(FString::Printf(TEXT("  Added TriggerSet: %s"), *TriggerSetPath));
					}
				}
			}
		}
	}

	// v3.5: Set AbilityConfiguration (TObjectPtr<UAbilityConfiguration>)
	if (!Definition.AbilityConfiguration.IsEmpty())
	{
		UObject* AbilityConfig = LoadObject<UObject>(nullptr, *Definition.AbilityConfiguration);
		if (!AbilityConfig)
		{
			// Try common paths
			FString ConfigPath = FString::Printf(TEXT("%s/AbilityConfigs/%s"), *GetProjectRoot(), *Definition.AbilityConfiguration);
			AbilityConfig = LoadObject<UObject>(nullptr, *ConfigPath);
		}
		if (AbilityConfig)
		{
			FObjectProperty* ConfigProp = CastField<FObjectProperty>(CharDef->GetClass()->FindPropertyByName(TEXT("AbilityConfiguration")));
			if (ConfigProp)
			{
				ConfigProp->SetObjectPropertyValue_InContainer(CharDef, AbilityConfig);
				LogGeneration(FString::Printf(TEXT("  Set AbilityConfiguration: %s"), *AbilityConfig->GetName()));
			}
		}
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(CharDef);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, CharDef, *PackageFileName, SaveArgs);

	// v3.0: Store metadata after successful generation
	StoreDataAssetMetadata(CharDef, TEXT("CD"), Definition.Name, InputHash);

	LogGeneration(FString::Printf(TEXT("Created Character Definition: %s"), *Definition.Name));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// v2.5.7: TaggedDialogueSet Generator - creates UTaggedDialogueSet data assets
#include "Tales/TaggedDialogueSet.h"

FGenerationResult FTaggedDialogueSetGenerator::Generate(const FManifestTaggedDialogueSetDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Dialogue/TaggedSets") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Tagged Dialogue Set"), Result))
	{
		return Result;
	}

	// v3.0: Use metadata-aware existence check
	uint64 InputHash = Definition.ComputeHash();
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Tagged Dialogue Set"), InputHash, Result))
	{
		return Result;
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	UTaggedDialogueSet* DialogueSet = NewObject<UTaggedDialogueSet>(Package, *Definition.Name, RF_Public | RF_Standalone);
	if (!DialogueSet)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Tagged Dialogue Set"));
	}

	// v2.6.0: Add dialogue entries with soft class references
	for (const FManifestTaggedDialogueEntry& DialogueDef : Definition.Dialogues)
	{
		FTaggedDialogue NewDialogue;

		// Set the tag
		if (!DialogueDef.Tag.IsEmpty())
		{
			NewDialogue.Tag = FGameplayTag::RequestGameplayTag(FName(*DialogueDef.Tag), false);
		}

		// Set cooldown and max distance
		NewDialogue.Cooldown = DialogueDef.Cooldown;
		NewDialogue.MaxDistance = DialogueDef.MaxDistance;

		// v2.6.0: Set Dialogue class via TSoftClassPtr
		if (!DialogueDef.DialogueClass.IsEmpty())
		{
			FString DialoguePath = DialogueDef.DialogueClass;
			// Build full path if relative
			if (!DialoguePath.Contains(TEXT("/")))
			{
				DialoguePath = FString::Printf(TEXT("%s/Dialogues/%s.%s_C"), *GetProjectRoot(), *DialogueDef.DialogueClass, *DialogueDef.DialogueClass);
			}
			else if (!DialoguePath.EndsWith(TEXT("_C")))
			{
				DialoguePath = DialoguePath + TEXT("_C");
			}
			NewDialogue.Dialogue = TSoftClassPtr<UDialogue>(FSoftObjectPath(DialoguePath));
			LogGeneration(FString::Printf(TEXT("  Set dialogue '%s' -> %s"), *DialogueDef.Tag, *DialoguePath));
		}

		// Parse required tags
		for (const FString& TagString : DialogueDef.RequiredTags)
		{
			FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
			if (Tag.IsValid())
			{
				NewDialogue.RequiredTags.AddTag(Tag);
			}
		}

		// Parse blocked tags
		for (const FString& TagString : DialogueDef.BlockedTags)
		{
			FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
			if (Tag.IsValid())
			{
				NewDialogue.BlockedTags.AddTag(Tag);
			}
		}

		DialogueSet->TaggedDialogues.Add(NewDialogue);
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(DialogueSet);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, DialogueSet, *PackageFileName, SaveArgs);

	// v3.0: Store metadata after successful generation
	StoreDataAssetMetadata(DialogueSet, TEXT("TaggedDialogue"), Definition.Name, InputHash);

	LogGeneration(FString::Printf(TEXT("Created Tagged Dialogue Set: %s with %d dialogues"),
		*Definition.Name, Definition.Dialogues.Num()));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// v2.6.10: Niagara System Generator (Enhanced)
// Supports: warmup, fixed bounds, determinism, pooling, effect types
// ============================================================================

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraTypes.h"
#include "NiagaraParameterStore.h"

FGenerationResult FNiagaraSystemGenerator::Generate(const FManifestNiagaraSystemDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("VFX/NiagaraSystems") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Niagara System"), Result))
	{
		return Result;
	}

	// v3.0: Use metadata-aware existence check
	uint64 InputHash = Definition.ComputeHash();
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Niagara System"), InputHash, Result))
	{
		return Result;
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	UNiagaraSystem* NewSystem = nullptr;

	// Option 1: Copy from template system
	if (!Definition.TemplateSystem.IsEmpty())
	{
		// Try to find template system
		FString TemplatePath = Definition.TemplateSystem;
		if (!TemplatePath.Contains(TEXT("/")))
		{
			// Search common locations
			TArray<FString> SearchPaths = {
				FString::Printf(TEXT("%s/VFX/%s"), *GetProjectRoot(), *Definition.TemplateSystem),
				FString::Printf(TEXT("%s/VFX/NiagaraSystems/%s"), *GetProjectRoot(), *Definition.TemplateSystem),
				FString::Printf(TEXT("/Game/VFX/%s"), *Definition.TemplateSystem),
				FString::Printf(TEXT("/Engine/Niagara/Templates/%s"), *Definition.TemplateSystem)
			};

			for (const FString& SearchPath : SearchPaths)
			{
				UNiagaraSystem* TemplateSystem = LoadObject<UNiagaraSystem>(nullptr, *SearchPath);
				if (TemplateSystem)
				{
					TemplatePath = SearchPath;
					break;
				}
			}
		}

		UNiagaraSystem* TemplateSystem = LoadObject<UNiagaraSystem>(nullptr, *TemplatePath);
		if (TemplateSystem)
		{
			// v2.9.1: Validate template and descriptor before proceeding
			if (Definition.FXDescriptor.HasData())
			{
				FFXValidationResult TemplateValidation;
				if (!ValidateTemplate(TemplatePath, TemplateValidation))
				{
					// Log errors but continue (warnings are non-fatal)
					for (const auto& Error : TemplateValidation.Errors)
					{
						if (Error.bFatal)
						{
							UE_LOG(LogTemp, Error, TEXT("[FXGEN][ERROR] %s: %s"), *Error.AssetPath, *Error.Message);
						}
					}
					// Fatal errors prevent generation
					if (TemplateValidation.HasFatalErrors())
					{
						return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
							FString::Printf(TEXT("Template validation failed: %s"), *TemplateValidation.GetSummary()));
					}
				}

				// Validate descriptor against template
				FFXValidationResult DescriptorValidation;
				if (!ValidateDescriptor(Definition, TemplateSystem, DescriptorValidation))
				{
					for (const auto& Error : DescriptorValidation.Errors)
					{
						UE_LOG(LogTemp, Error, TEXT("[FXGEN][ERROR] %s: %s"), *Error.ParamName, *Error.Message);
					}
					if (DescriptorValidation.HasFatalErrors())
					{
						return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
							FString::Printf(TEXT("Descriptor validation failed: %s"), *DescriptorValidation.GetSummary()));
					}
				}
			}

			if (!TemplateSystem->IsReadyToRun())
			{
				TemplateSystem->WaitForCompilationComplete();
			}

			NewSystem = Cast<UNiagaraSystem>(StaticDuplicateObject(TemplateSystem, Package, *Definition.Name, RF_Public | RF_Standalone));
			if (NewSystem)
			{
				NewSystem->TemplateAssetDescription = FText();
				NewSystem->Category = FText();
				LogGeneration(FString::Printf(TEXT("  Created from template: %s"), *TemplatePath));
			}
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  WARNING: Template system not found: %s"), *Definition.TemplateSystem));
		}
	}

	// Option 2: Create empty system and add emitters
	if (!NewSystem)
	{
		NewSystem = NewObject<UNiagaraSystem>(Package, UNiagaraSystem::StaticClass(), *Definition.Name, RF_Public | RF_Standalone | RF_Transactional);
		if (NewSystem)
		{
			// Initialize the system with default scripts
			UNiagaraSystemFactoryNew::InitializeSystem(NewSystem, true);

			// Add emitters if specified
			for (const FString& EmitterName : Definition.Emitters)
			{
				FString EmitterPath = EmitterName;
				if (!EmitterPath.Contains(TEXT("/")))
				{
					// Search common locations
					TArray<FString> SearchPaths = {
						FString::Printf(TEXT("%s/VFX/Emitters/%s"), *GetProjectRoot(), *EmitterName),
						FString::Printf(TEXT("%s/VFX/%s"), *GetProjectRoot(), *EmitterName),
						FString::Printf(TEXT("/Game/VFX/Emitters/%s"), *EmitterName),
						FString::Printf(TEXT("/Engine/Niagara/Templates/Emitters/%s"), *EmitterName)
					};

					for (const FString& SearchPath : SearchPaths)
					{
						UNiagaraEmitter* TestEmitter = LoadObject<UNiagaraEmitter>(nullptr, *SearchPath);
						if (TestEmitter)
						{
							EmitterPath = SearchPath;
							break;
						}
					}
				}

				UNiagaraEmitter* Emitter = LoadObject<UNiagaraEmitter>(nullptr, *EmitterPath);
				if (Emitter)
				{
					FVersionedNiagaraEmitter VersionedEmitter(Emitter, Emitter->GetExposedVersion().VersionGuid);
					FNiagaraEditorUtilities::AddEmitterToSystem(*NewSystem, *Emitter, VersionedEmitter.Version);
					LogGeneration(FString::Printf(TEXT("  Added emitter: %s"), *EmitterPath));
				}
				else
				{
					LogGeneration(FString::Printf(TEXT("  WARNING: Emitter not found: %s"), *EmitterName));
				}
			}

			// v2.8.4: Add default emitter based on effect_type if no emitters specified
			if (Definition.Emitters.Num() == 0 && !Definition.EffectType.IsEmpty())
			{
				FString DefaultEmitterPath;
				FString EffectTypeLower = Definition.EffectType.ToLower();

				if (EffectTypeLower == TEXT("ambient") || EffectTypeLower == TEXT("looping"))
				{
					DefaultEmitterPath = TEXT("/Niagara/DefaultAssets/Templates/Emitters/Fountain");
				}
				else if (EffectTypeLower == TEXT("burst") || EffectTypeLower == TEXT("explosion"))
				{
					DefaultEmitterPath = TEXT("/Niagara/DefaultAssets/Templates/Emitters/SimpleSpriteBurst");
				}
				else if (EffectTypeLower == TEXT("beam") || EffectTypeLower == TEXT("laser"))
				{
					DefaultEmitterPath = TEXT("/Niagara/DefaultAssets/Templates/Emitters/DynamicBeam");
				}
				else if (EffectTypeLower == TEXT("ribbon") || EffectTypeLower == TEXT("trail"))
				{
					DefaultEmitterPath = TEXT("/Niagara/DefaultAssets/Templates/Emitters/LocationBasedRibbon");
				}
				else
				{
					// Default fallback - simple sprite burst
					DefaultEmitterPath = TEXT("/Niagara/DefaultAssets/Templates/Emitters/SimpleSpriteBurst");
				}

				UNiagaraEmitter* DefaultEmitter = LoadObject<UNiagaraEmitter>(nullptr, *DefaultEmitterPath);
				if (DefaultEmitter)
				{
					FVersionedNiagaraEmitter VersionedEmitter(DefaultEmitter, DefaultEmitter->GetExposedVersion().VersionGuid);
					FNiagaraEditorUtilities::AddEmitterToSystem(*NewSystem, *DefaultEmitter, VersionedEmitter.Version);
					LogGeneration(FString::Printf(TEXT("  Added default emitter for effect_type '%s': %s"), *Definition.EffectType, *DefaultEmitterPath));
				}
				else
				{
					LogGeneration(FString::Printf(TEXT("  WARNING: Default emitter not found: %s"), *DefaultEmitterPath));
				}
			}
		}
	}

	if (!NewSystem)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Niagara System"));
	}

	// v2.6.10: Apply warmup settings
	if (Definition.WarmupTime > 0.0f)
	{
		NewSystem->SetWarmupTime(Definition.WarmupTime);
		LogGeneration(FString::Printf(TEXT("  Set warmup time: %.2f seconds"), Definition.WarmupTime));
	}
	if (Definition.WarmupTickCount > 0)
	{
		// Access via reflection since WarmupTickCount may be protected
		if (FIntProperty* TickCountProp = CastField<FIntProperty>(NewSystem->GetClass()->FindPropertyByName(TEXT("WarmupTickCount"))))
		{
			TickCountProp->SetPropertyValue_InContainer(NewSystem, Definition.WarmupTickCount);
			LogGeneration(FString::Printf(TEXT("  Set warmup tick count: %d"), Definition.WarmupTickCount));
		}
	}
	if (Definition.WarmupTickDelta != 0.0333f && Definition.WarmupTime > 0.0f)
	{
		if (FFloatProperty* TickDeltaProp = CastField<FFloatProperty>(NewSystem->GetClass()->FindPropertyByName(TEXT("WarmupTickDelta"))))
		{
			TickDeltaProp->SetPropertyValue_InContainer(NewSystem, Definition.WarmupTickDelta);
			LogGeneration(FString::Printf(TEXT("  Set warmup tick delta: %.4f"), Definition.WarmupTickDelta));
		}
	}

	// v2.6.10: Apply fixed bounds settings
	if (Definition.bFixedBounds)
	{
		FBox Bounds(Definition.BoundsMin, Definition.BoundsMax);
		NewSystem->SetFixedBounds(Bounds);

		// Set the bFixedBounds flag via reflection
		if (FBoolProperty* FixedBoundsProp = CastField<FBoolProperty>(NewSystem->GetClass()->FindPropertyByName(TEXT("bFixedBounds"))))
		{
			FixedBoundsProp->SetPropertyValue_InContainer(NewSystem, true);
		}
		LogGeneration(FString::Printf(TEXT("  Set fixed bounds: (%.1f,%.1f,%.1f) to (%.1f,%.1f,%.1f)"),
			Definition.BoundsMin.X, Definition.BoundsMin.Y, Definition.BoundsMin.Z,
			Definition.BoundsMax.X, Definition.BoundsMax.Y, Definition.BoundsMax.Z));
	}

	// v2.6.10: Apply determinism settings
	if (Definition.bDeterminism)
	{
		if (FBoolProperty* DeterminismProp = CastField<FBoolProperty>(NewSystem->GetClass()->FindPropertyByName(TEXT("bDeterminism"))))
		{
			DeterminismProp->SetPropertyValue_InContainer(NewSystem, true);
			LogGeneration(TEXT("  Enabled deterministic simulation"));
		}
		if (Definition.RandomSeed != 0)
		{
			if (FIntProperty* SeedProp = CastField<FIntProperty>(NewSystem->GetClass()->FindPropertyByName(TEXT("RandomSeed"))))
			{
				SeedProp->SetPropertyValue_InContainer(NewSystem, Definition.RandomSeed);
				LogGeneration(FString::Printf(TEXT("  Set random seed: %d"), Definition.RandomSeed));
			}
		}
	}

	// v2.6.10: Apply pooling settings
	if (Definition.MaxPoolSize > 0)
	{
		if (FIntProperty* PoolSizeProp = CastField<FIntProperty>(NewSystem->GetClass()->FindPropertyByName(TEXT("MaxPoolSize"))))
		{
			PoolSizeProp->SetPropertyValue_InContainer(NewSystem, Definition.MaxPoolSize);
			LogGeneration(FString::Printf(TEXT("  Set max pool size: %d"), Definition.MaxPoolSize));
		}
	}
	if (!Definition.PoolingMethod.IsEmpty())
	{
		// PoolingMethod is an enum - try to set via byte property
		if (FByteProperty* PoolMethodProp = CastField<FByteProperty>(NewSystem->GetClass()->FindPropertyByName(TEXT("PoolingMethod"))))
		{
			int32 MethodValue = 0;
			if (Definition.PoolingMethod.Equals(TEXT("None"), ESearchCase::IgnoreCase)) MethodValue = 0;
			else if (Definition.PoolingMethod.Equals(TEXT("AutoRelease"), ESearchCase::IgnoreCase)) MethodValue = 1;
			else if (Definition.PoolingMethod.Equals(TEXT("ManualRelease"), ESearchCase::IgnoreCase)) MethodValue = 2;
			else if (Definition.PoolingMethod.Equals(TEXT("FreeInWorld"), ESearchCase::IgnoreCase)) MethodValue = 3;

			PoolMethodProp->SetPropertyValue_InContainer(NewSystem, (uint8)MethodValue);
			LogGeneration(FString::Printf(TEXT("  Set pooling method: %s"), *Definition.PoolingMethod));
		}
	}

	// v2.6.10: Log effect type (informational - helps with organization)
	if (!Definition.EffectType.IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("  Effect type: %s"), *Definition.EffectType));
	}

	// v2.6.11: Add user parameters
	if (Definition.UserParameters.Num() > 0)
	{
		FNiagaraUserRedirectionParameterStore& UserParams = NewSystem->GetExposedParameters();

		for (const FManifestNiagaraUserParameter& UserParam : Definition.UserParameters)
		{
			FString ParamType = UserParam.Type.ToLower();
			FName ParamName = *FString::Printf(TEXT("User.%s"), *UserParam.Name);

			if (ParamType == TEXT("float"))
			{
				FNiagaraVariable Var(FNiagaraTypeDefinition::GetFloatDef(), ParamName);
				float DefaultValue = FCString::Atof(*UserParam.DefaultValue);
				Var.SetValue(DefaultValue);
				UserParams.AddParameter(Var, true);
				LogGeneration(FString::Printf(TEXT("  Added user parameter: %s (float) = %.2f"), *UserParam.Name, DefaultValue));
			}
			else if (ParamType == TEXT("int") || ParamType == TEXT("int32"))
			{
				FNiagaraVariable Var(FNiagaraTypeDefinition::GetIntDef(), ParamName);
				int32 DefaultValue = FCString::Atoi(*UserParam.DefaultValue);
				Var.SetValue(DefaultValue);
				UserParams.AddParameter(Var, true);
				LogGeneration(FString::Printf(TEXT("  Added user parameter: %s (int) = %d"), *UserParam.Name, DefaultValue));
			}
			else if (ParamType == TEXT("bool"))
			{
				FNiagaraVariable Var(FNiagaraTypeDefinition::GetBoolDef(), ParamName);
				bool DefaultValue = UserParam.DefaultValue.ToBool();
				Var.SetValue(DefaultValue);
				UserParams.AddParameter(Var, true);
				LogGeneration(FString::Printf(TEXT("  Added user parameter: %s (bool) = %s"), *UserParam.Name, DefaultValue ? TEXT("true") : TEXT("false")));
			}
			else if (ParamType == TEXT("vector") || ParamType == TEXT("vector3") || ParamType == TEXT("fvector"))
			{
				FNiagaraVariable Var(FNiagaraTypeDefinition::GetVec3Def(), ParamName);
				TArray<FString> Parts;
				UserParam.DefaultValue.ParseIntoArray(Parts, TEXT(","));
				FVector DefaultValue = FVector::ZeroVector;
				if (Parts.Num() >= 3)
				{
					DefaultValue = FVector(
						FCString::Atof(*Parts[0].TrimStartAndEnd()),
						FCString::Atof(*Parts[1].TrimStartAndEnd()),
						FCString::Atof(*Parts[2].TrimStartAndEnd())
					);
				}
				Var.SetValue(DefaultValue);
				UserParams.AddParameter(Var, true);
				LogGeneration(FString::Printf(TEXT("  Added user parameter: %s (vector) = (%.2f,%.2f,%.2f)"),
					*UserParam.Name, DefaultValue.X, DefaultValue.Y, DefaultValue.Z));
			}
			else if (ParamType == TEXT("color") || ParamType == TEXT("linear_color") || ParamType == TEXT("linearcolor") || ParamType == TEXT("flinearcolor"))
			{
				FNiagaraVariable Var(FNiagaraTypeDefinition::GetColorDef(), ParamName);
				TArray<FString> Parts;
				UserParam.DefaultValue.ParseIntoArray(Parts, TEXT(","));
				FLinearColor DefaultValue = FLinearColor::White;
				if (Parts.Num() >= 3)
				{
					DefaultValue.R = FCString::Atof(*Parts[0].TrimStartAndEnd());
					DefaultValue.G = FCString::Atof(*Parts[1].TrimStartAndEnd());
					DefaultValue.B = FCString::Atof(*Parts[2].TrimStartAndEnd());
					if (Parts.Num() >= 4)
					{
						DefaultValue.A = FCString::Atof(*Parts[3].TrimStartAndEnd());
					}
				}
				Var.SetValue(DefaultValue);
				UserParams.AddParameter(Var, true);
				LogGeneration(FString::Printf(TEXT("  Added user parameter: %s (color) = (%.2f,%.2f,%.2f,%.2f)"),
					*UserParam.Name, DefaultValue.R, DefaultValue.G, DefaultValue.B, DefaultValue.A));
			}
			else if (ParamType == TEXT("vector2") || ParamType == TEXT("vector2d") || ParamType == TEXT("fvector2d"))
			{
				FNiagaraVariable Var(FNiagaraTypeDefinition::GetVec2Def(), ParamName);
				TArray<FString> Parts;
				UserParam.DefaultValue.ParseIntoArray(Parts, TEXT(","));
				FVector2D DefaultValue = FVector2D::ZeroVector;
				if (Parts.Num() >= 2)
				{
					DefaultValue = FVector2D(
						FCString::Atof(*Parts[0].TrimStartAndEnd()),
						FCString::Atof(*Parts[1].TrimStartAndEnd())
					);
				}
				Var.SetValue(DefaultValue);
				UserParams.AddParameter(Var, true);
				LogGeneration(FString::Printf(TEXT("  Added user parameter: %s (vector2) = (%.2f,%.2f)"),
					*UserParam.Name, DefaultValue.X, DefaultValue.Y));
			}
			else if (ParamType == TEXT("vector4") || ParamType == TEXT("fvector4"))
			{
				FNiagaraVariable Var(FNiagaraTypeDefinition::GetVec4Def(), ParamName);
				TArray<FString> Parts;
				UserParam.DefaultValue.ParseIntoArray(Parts, TEXT(","));
				FVector4 DefaultValue = FVector4::Zero();
				if (Parts.Num() >= 4)
				{
					DefaultValue = FVector4(
						FCString::Atof(*Parts[0].TrimStartAndEnd()),
						FCString::Atof(*Parts[1].TrimStartAndEnd()),
						FCString::Atof(*Parts[2].TrimStartAndEnd()),
						FCString::Atof(*Parts[3].TrimStartAndEnd())
					);
				}
				Var.SetValue(DefaultValue);
				UserParams.AddParameter(Var, true);
				LogGeneration(FString::Printf(TEXT("  Added user parameter: %s (vector4) = (%.2f,%.2f,%.2f,%.2f)"),
					*UserParam.Name, DefaultValue.X, DefaultValue.Y, DefaultValue.Z, DefaultValue.W));
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("  WARNING: Unknown parameter type '%s' for %s, defaulting to float"), *ParamType, *UserParam.Name));
				FNiagaraVariable Var(FNiagaraTypeDefinition::GetFloatDef(), ParamName);
				float DefaultValue = FCString::Atof(*UserParam.DefaultValue);
				Var.SetValue(DefaultValue);
				UserParams.AddParameter(Var, true);
			}
		}

		LogGeneration(FString::Printf(TEXT("  Added %d user parameters"), Definition.UserParameters.Num()));
	}

	// v2.9.0: Apply FX Descriptor values to existing User parameters
	// This sets values on User.* parameters that were defined in the template system
	if (Definition.FXDescriptor.HasData())
	{
		FNiagaraUserRedirectionParameterStore& UserParams = NewSystem->GetExposedParameters();
		int32 ParamsApplied = 0;

		// Helper lambda to set a float parameter (adds if not exists)
		auto SetFloatParam = [&](const FString& ParamName, float Value) {
			FName FullName = *FString::Printf(TEXT("User.%s"), *ParamName);
			FNiagaraVariable Var(FNiagaraTypeDefinition::GetFloatDef(), FullName);
			Var.SetValue(Value);
			UserParams.AddParameter(Var, true);
			ParamsApplied++;
		};

		// Helper lambda to set a bool parameter (adds if not exists)
		auto SetBoolParam = [&](const FString& ParamName, bool Value) {
			FName FullName = *FString::Printf(TEXT("User.%s"), *ParamName);
			FNiagaraVariable Var(FNiagaraTypeDefinition::GetBoolDef(), FullName);
			Var.SetValue(Value);
			UserParams.AddParameter(Var, true);
			ParamsApplied++;
		};

		// Helper lambda to set an int parameter (adds if not exists)
		auto SetIntParam = [&](const FString& ParamName, int32 Value) {
			FName FullName = *FString::Printf(TEXT("User.%s"), *ParamName);
			FNiagaraVariable Var(FNiagaraTypeDefinition::GetIntDef(), FullName);
			Var.SetValue(Value);
			UserParams.AddParameter(Var, true);
			ParamsApplied++;
		};

		// Helper lambda to set a vector parameter (adds if not exists)
		auto SetVectorParam = [&](const FString& ParamName, const FVector& Value) {
			FName FullName = *FString::Printf(TEXT("User.%s"), *ParamName);
			FNiagaraVariable Var(FNiagaraTypeDefinition::GetVec3Def(), FullName);
			Var.SetValue(Value);
			UserParams.AddParameter(Var, true);
			ParamsApplied++;
		};

		// Helper lambda to set a color parameter (adds if not exists)
		auto SetColorParam = [&](const FString& ParamName, const FLinearColor& Value) {
			FName FullName = *FString::Printf(TEXT("User.%s"), *ParamName);
			FNiagaraVariable Var(FNiagaraTypeDefinition::GetColorDef(), FullName);
			Var.SetValue(Value);
			UserParams.AddParameter(Var, true);
			ParamsApplied++;
		};

		// Apply emitter toggles
		SetBoolParam(TEXT("Particles.Enabled"), Definition.FXDescriptor.bParticlesEnabled);
		SetBoolParam(TEXT("Burst.Enabled"), Definition.FXDescriptor.bBurstEnabled);
		SetBoolParam(TEXT("Beam.Enabled"), Definition.FXDescriptor.bBeamEnabled);
		SetBoolParam(TEXT("Ribbon.Enabled"), Definition.FXDescriptor.bRibbonEnabled);
		SetBoolParam(TEXT("Light.Enabled"), Definition.FXDescriptor.bLightEnabled);
		SetBoolParam(TEXT("Smoke.Enabled"), Definition.FXDescriptor.bSmokeEnabled);
		SetBoolParam(TEXT("Sparks.Enabled"), Definition.FXDescriptor.bSparkEnabled);

		// Apply core emission parameters
		SetFloatParam(TEXT("SpawnRate"), Definition.FXDescriptor.SpawnRate);
		SetFloatParam(TEXT("LifetimeMin"), Definition.FXDescriptor.LifetimeMin);
		SetFloatParam(TEXT("LifetimeMax"), Definition.FXDescriptor.LifetimeMax);
		SetIntParam(TEXT("MaxParticles"), Definition.FXDescriptor.MaxParticles);

		// Apply appearance parameters
		SetColorParam(TEXT("Color"), Definition.FXDescriptor.Color);
		SetFloatParam(TEXT("SizeMin"), Definition.FXDescriptor.SizeMin);
		SetFloatParam(TEXT("SizeMax"), Definition.FXDescriptor.SizeMax);
		SetFloatParam(TEXT("Opacity"), Definition.FXDescriptor.Opacity);
		SetFloatParam(TEXT("Emissive"), Definition.FXDescriptor.Emissive);

		// Apply motion parameters
		SetVectorParam(TEXT("InitialVelocity"), Definition.FXDescriptor.InitialVelocity);
		SetFloatParam(TEXT("NoiseStrength"), Definition.FXDescriptor.NoiseStrength);
		SetFloatParam(TEXT("GravityScale"), Definition.FXDescriptor.GravityScale);

		// Apply beam-specific parameters
		SetFloatParam(TEXT("BeamLength"), Definition.FXDescriptor.BeamLength);
		SetFloatParam(TEXT("BeamWidth"), Definition.FXDescriptor.BeamWidth);

		// Apply ribbon-specific parameters
		SetFloatParam(TEXT("RibbonWidth"), Definition.FXDescriptor.RibbonWidth);

		// Apply light-specific parameters
		SetFloatParam(TEXT("LightIntensity"), Definition.FXDescriptor.LightIntensity);
		SetFloatParam(TEXT("LightRadius"), Definition.FXDescriptor.LightRadius);

		// Apply LOD parameters
		SetFloatParam(TEXT("CullDistance"), Definition.FXDescriptor.CullDistance);
		SetIntParam(TEXT("LODLevel"), Definition.FXDescriptor.LODLevel);

		LogGeneration(FString::Printf(TEXT("  Applied FX Descriptor: %d parameters set"), ParamsApplied));

		// Log enabled emitters
		TArray<FString> EnabledEmitters;
		if (Definition.FXDescriptor.bParticlesEnabled) EnabledEmitters.Add(TEXT("Particles"));
		if (Definition.FXDescriptor.bBurstEnabled) EnabledEmitters.Add(TEXT("Burst"));
		if (Definition.FXDescriptor.bBeamEnabled) EnabledEmitters.Add(TEXT("Beam"));
		if (Definition.FXDescriptor.bRibbonEnabled) EnabledEmitters.Add(TEXT("Ribbon"));
		if (Definition.FXDescriptor.bLightEnabled) EnabledEmitters.Add(TEXT("Light"));
		if (Definition.FXDescriptor.bSmokeEnabled) EnabledEmitters.Add(TEXT("Smoke"));
		if (Definition.FXDescriptor.bSparkEnabled) EnabledEmitters.Add(TEXT("Sparks"));

		if (EnabledEmitters.Num() > 0)
		{
			LogGeneration(FString::Printf(TEXT("  Enabled emitters: %s"), *FString::Join(EnabledEmitters, TEXT(", "))));
		}
	}

	// Request compilation
	NewSystem->RequestCompile(false);

	// Mark dirty and register
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewSystem);

	// Save the package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, NewSystem, *PackageFileName, SaveArgs);

	// v2.9.1: Store generator metadata for regeneration safety (FX-specific)
	StoreGeneratorMetadata(NewSystem, Definition);

	// v3.0: Store standard asset metadata for regen/diff system
	StoreDataAssetMetadata(NewSystem, TEXT("NS"), Definition.Name, InputHash);

	LogGeneration(FString::Printf(TEXT("Created Niagara System: %s"), *Definition.Name));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// v2.9.1: FNiagaraSystemGenerator Validation Implementation
// ============================================================================

// Static member initialization
TArray<FFXExpectedParam> FNiagaraSystemGenerator::CachedExpectedParams;
bool FNiagaraSystemGenerator::bExpectedParamsBuilt = false;

void FNiagaraSystemGenerator::BuildExpectedParameters()
{
	if (bExpectedParamsBuilt) return;

	CachedExpectedParams.Empty();

	// Emitter toggles (all optional, but if present must be bool)
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.Particles.Enabled"), EFXParamType::Bool, false));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.Burst.Enabled"), EFXParamType::Bool, false));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.Beam.Enabled"), EFXParamType::Bool, false));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.Ribbon.Enabled"), EFXParamType::Bool, false));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.Light.Enabled"), EFXParamType::Bool, false));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.Smoke.Enabled"), EFXParamType::Bool, false));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.Sparks.Enabled"), EFXParamType::Bool, false));

	// Core emission parameters (required for most effects)
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.SpawnRate"), EFXParamType::Float, true, 0.f, 10000.f));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.LifetimeMin"), EFXParamType::Float, false, 0.001f, 60.f));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.LifetimeMax"), EFXParamType::Float, false, 0.001f, 60.f));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.MaxParticles"), EFXParamType::Int, false, 1.f, 100000.f));

	// Appearance parameters
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.Color"), EFXParamType::Color, false));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.SizeMin"), EFXParamType::Float, false, 0.f, 10000.f));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.SizeMax"), EFXParamType::Float, false, 0.f, 10000.f));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.Opacity"), EFXParamType::Float, false, 0.f, 1.f));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.Emissive"), EFXParamType::Float, false, 0.f, 100.f));

	// Motion parameters
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.InitialVelocity"), EFXParamType::Vec3, false));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.NoiseStrength"), EFXParamType::Float, false, 0.f, 1000.f));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.GravityScale"), EFXParamType::Float, false, -10.f, 10.f));

	// Beam-specific
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.BeamLength"), EFXParamType::Float, false, 0.f, 10000.f));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.BeamWidth"), EFXParamType::Float, false, 0.f, 1000.f));

	// Ribbon-specific
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.RibbonWidth"), EFXParamType::Float, false, 0.f, 1000.f));

	// Light-specific
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.LightIntensity"), EFXParamType::Float, false, 0.f, 100000.f));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.LightRadius"), EFXParamType::Float, false, 0.f, 10000.f));

	// LOD
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.CullDistance"), EFXParamType::Float, false, 0.f, 100000.f));
	CachedExpectedParams.Add(FFXExpectedParam(TEXT("User.LODLevel"), EFXParamType::Int, false, 0.f, 4.f));

	bExpectedParamsBuilt = true;
}

TArray<FFXExpectedParam> FNiagaraSystemGenerator::GetExpectedParameters()
{
	BuildExpectedParameters();
	return CachedExpectedParams;
}

bool FNiagaraSystemGenerator::ValidateTemplate(const FString& TemplatePath, FFXValidationResult& OutResult)
{
	OutResult.Reset();

	// Phase A.1: Check template loads
	UNiagaraSystem* TemplateSystem = LoadObject<UNiagaraSystem>(nullptr, *TemplatePath);
	if (!TemplateSystem)
	{
		OutResult.bTemplateValid = false;
		OutResult.AddError(FFXValidationError(TemplatePath, TEXT("Template system failed to load"), true));
		return false;
	}

	// Phase A.2: Get exposed parameters
	const FNiagaraUserRedirectionParameterStore& UserParams = TemplateSystem->GetExposedParameters();
	TArray<FNiagaraVariable> AllParams;
	UserParams.GetParameters(AllParams);

	if (AllParams.Num() == 0)
	{
		OutResult.AddError(FFXValidationError(TemplatePath, TEXT("Template has no exposed User parameters"), true));
		OutResult.bTemplateValid = false;
		return false;
	}

	// Phase A.3: Check for duplicate parameter names
	TSet<FName> SeenNames;
	for (const FNiagaraVariableBase& Param : AllParams)
	{
		if (SeenNames.Contains(Param.GetName()))
		{
			OutResult.AddError(FFXValidationError(TemplatePath, Param.GetName().ToString(),
				FString::Printf(TEXT("Duplicate User parameter: %s"), *Param.GetName().ToString()), true));
		}
		SeenNames.Add(Param.GetName());
	}

	// Phase A.4: Validate expected parameters exist
	BuildExpectedParameters();
	for (const FFXExpectedParam& Expected : CachedExpectedParams)
	{
		bool bFound = false;
		for (const FNiagaraVariableBase& Param : AllParams)
		{
			if (Param.GetName() == Expected.Name)
			{
				bFound = true;
				OutResult.ParamsValidated++;

				// Check type match
				EFXParamType ActualType = EFXParamType::Unknown;
				const FNiagaraTypeDefinition& TypeDef = Param.GetType();
				if (TypeDef == FNiagaraTypeDefinition::GetFloatDef()) ActualType = EFXParamType::Float;
				else if (TypeDef == FNiagaraTypeDefinition::GetIntDef()) ActualType = EFXParamType::Int;
				else if (TypeDef == FNiagaraTypeDefinition::GetBoolDef()) ActualType = EFXParamType::Bool;
				else if (TypeDef == FNiagaraTypeDefinition::GetVec2Def()) ActualType = EFXParamType::Vec2;
				else if (TypeDef == FNiagaraTypeDefinition::GetVec3Def()) ActualType = EFXParamType::Vec3;
				else if (TypeDef == FNiagaraTypeDefinition::GetColorDef()) ActualType = EFXParamType::Color;

				if (ActualType != Expected.Type && Expected.Type != EFXParamType::Unknown)
				{
					OutResult.ParamsTypeMismatch++;
					OutResult.AddError(FFXValidationError(TemplatePath, Expected.Name.ToString(),
						FString::Printf(TEXT("Type mismatch for %s: expected %d, got %d"),
							*Expected.Name.ToString(), (int)Expected.Type, (int)ActualType),
						Expected.bRequired));
				}
				break;
			}
		}

		if (!bFound)
		{
			OutResult.ParamsMissing++;
			if (Expected.bRequired)
			{
				OutResult.AddError(FFXValidationError(TemplatePath, Expected.Name.ToString(),
					FString::Printf(TEXT("Missing required User parameter: %s"), *Expected.Name.ToString()), true));
			}
			else
			{
				// Warning for optional params
				OutResult.AddError(FFXValidationError(TemplatePath, Expected.Name.ToString(),
					FString::Printf(TEXT("Missing optional User parameter: %s"), *Expected.Name.ToString()), false));
			}
		}
	}

	OutResult.bTemplateValid = !OutResult.HasFatalErrors();

	// Log validation summary
	UE_LOG(LogTemp, Log, TEXT("[FXGEN] Template validation for %s: %s"),
		*TemplatePath, *OutResult.GetSummary());

	return OutResult.bTemplateValid;
}

bool FNiagaraSystemGenerator::ValidateDescriptor(const FManifestNiagaraSystemDefinition& Definition,
	UNiagaraSystem* TemplateSystem, FFXValidationResult& OutResult)
{
	OutResult.Reset();

	if (!TemplateSystem)
	{
		OutResult.AddError(FFXValidationError(Definition.Name, TEXT("Template system is null"), true));
		return false;
	}

	if (!Definition.FXDescriptor.HasData())
	{
		// No FX descriptor, nothing to validate
		return true;
	}

	const FNiagaraUserRedirectionParameterStore& UserParams = TemplateSystem->GetExposedParameters();
	TArray<FNiagaraVariable> AllParams;
	UserParams.GetParameters(AllParams);

	// Build a map of parameter name -> type for quick lookup
	TMap<FName, EFXParamType> ParamTypeMap;
	for (const FNiagaraVariableBase& Param : AllParams)
	{
		EFXParamType Type = EFXParamType::Unknown;
		const FNiagaraTypeDefinition& TypeDef = Param.GetType();
		if (TypeDef == FNiagaraTypeDefinition::GetFloatDef()) Type = EFXParamType::Float;
		else if (TypeDef == FNiagaraTypeDefinition::GetIntDef()) Type = EFXParamType::Int;
		else if (TypeDef == FNiagaraTypeDefinition::GetBoolDef()) Type = EFXParamType::Bool;
		else if (TypeDef == FNiagaraTypeDefinition::GetVec2Def()) Type = EFXParamType::Vec2;
		else if (TypeDef == FNiagaraTypeDefinition::GetVec3Def()) Type = EFXParamType::Vec3;
		else if (TypeDef == FNiagaraTypeDefinition::GetColorDef()) Type = EFXParamType::Color;
		ParamTypeMap.Add(Param.GetName(), Type);
	}

	// Lambda to validate a parameter
	auto ValidateParam = [&](FName ParamName, EFXParamType ExpectedType, bool bRequired) {
		const EFXParamType* FoundType = ParamTypeMap.Find(ParamName);
		if (!FoundType)
		{
			OutResult.ParamsMissing++;
			if (bRequired)
			{
				OutResult.AddError(FFXValidationError(Definition.Name, ParamName.ToString(),
					FString::Printf(TEXT("Descriptor references missing parameter: %s"), *ParamName.ToString()), true));
			}
			return;
		}
		OutResult.ParamsValidated++;

		if (*FoundType != ExpectedType && *FoundType != EFXParamType::Unknown)
		{
			OutResult.ParamsTypeMismatch++;
			OutResult.AddError(FFXValidationError(Definition.Name, ParamName.ToString(),
				FString::Printf(TEXT("Descriptor type mismatch for %s"), *ParamName.ToString()), bRequired));
		}
	};

	// Validate all descriptor parameters that are being set
	// Emitter toggles
	if (Definition.FXDescriptor.bParticlesEnabled)
		ValidateParam(TEXT("User.Particles.Enabled"), EFXParamType::Bool, false);
	if (Definition.FXDescriptor.bBurstEnabled)
		ValidateParam(TEXT("User.Burst.Enabled"), EFXParamType::Bool, false);
	if (Definition.FXDescriptor.bBeamEnabled)
		ValidateParam(TEXT("User.Beam.Enabled"), EFXParamType::Bool, false);
	if (Definition.FXDescriptor.bRibbonEnabled)
		ValidateParam(TEXT("User.Ribbon.Enabled"), EFXParamType::Bool, false);
	if (Definition.FXDescriptor.bLightEnabled)
		ValidateParam(TEXT("User.Light.Enabled"), EFXParamType::Bool, false);

	// Core parameters are always validated since they're always set
	ValidateParam(TEXT("User.SpawnRate"), EFXParamType::Float, true);
	ValidateParam(TEXT("User.Color"), EFXParamType::Color, false);

	// Log validation summary
	UE_LOG(LogTemp, Log, TEXT("[FXGEN] Descriptor validation for %s: %s"),
		*Definition.Name, *OutResult.GetSummary());

	return !OutResult.HasFatalErrors();
}

bool FNiagaraSystemGenerator::DetectManualEdit(UNiagaraSystem* ExistingSystem,
	const FManifestNiagaraSystemDefinition& Definition)
{
	if (!ExistingSystem) return false;

	// Get stored metadata
	FFXGeneratorMetadata Metadata = GetGeneratorMetadata(ExistingSystem);

	// If no metadata, this wasn't generated by us - treat as manual
	if (!Metadata.bIsGenerated)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FXGEN] Asset %s has no generator metadata - may be manually created"),
			*Definition.Name);
		return true;
	}

	// Check if template changed
	if (Metadata.SourceTemplate != Definition.TemplateSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FXGEN] Asset %s template changed: was %s, now %s"),
			*Definition.Name, *Metadata.SourceTemplate, *Definition.TemplateSystem);
		return true;
	}

	// Check parameter count difference (sign of manual modification)
	const FNiagaraUserRedirectionParameterStore& UserParams = ExistingSystem->GetExposedParameters();
	TArray<FNiagaraVariable> AllParams;
	UserParams.GetParameters(AllParams);

	// If significantly more params than expected, likely modified
	if (AllParams.Num() > GetExpectedParameters().Num() + 5)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FXGEN] Asset %s has %d params, expected ~%d - may be manually modified"),
			*Definition.Name, AllParams.Num(), GetExpectedParameters().Num());
		return true;
	}

	return false;
}

void FNiagaraSystemGenerator::StoreGeneratorMetadata(UNiagaraSystem* System,
	const FManifestNiagaraSystemDefinition& Definition)
{
	if (!System) return;

	FFXGeneratorMetadata Metadata;
	Metadata.bIsGenerated = true;
	Metadata.SourceTemplate = Definition.TemplateSystem;
	Metadata.DescriptorHash = Definition.FXDescriptor.ComputeHash();
	Metadata.GeneratedTime = FDateTime::Now();

	// Store as asset user data string
	// UNiagaraSystem inherits from UObject, so we use the package metadata
	UPackage* Package = System->GetOutermost();
	if (Package)
	{
		// Store metadata string for logging
		FString MetadataStr = Metadata.ToString();

		// Note: For full metadata persistence, we could use asset registry tags
		// or store in a sidecar file. For now, we rely on parameter pattern detection.
		UE_LOG(LogTemp, Log, TEXT("[FXGEN] Stored generator metadata for %s: Hash=%llu, Info=%s"),
			*System->GetName(), Metadata.DescriptorHash, *MetadataStr);
	}
}

FFXGeneratorMetadata FNiagaraSystemGenerator::GetGeneratorMetadata(UNiagaraSystem* System)
{
	FFXGeneratorMetadata EmptyMetadata;
	if (!System) return EmptyMetadata;

	// For now, we detect generated assets by checking for our known parameter patterns
	// A more robust implementation would use asset user data or registry tags

	const FNiagaraUserRedirectionParameterStore& UserParams = System->GetExposedParameters();
	TArray<FNiagaraVariable> AllParams;
	UserParams.GetParameters(AllParams);

	// Check for presence of our standard parameters as a heuristic
	bool bHasParticlesEnabled = false;
	bool bHasSpawnRate = false;
	for (const FNiagaraVariableBase& Param : AllParams)
	{
		FString ParamName = Param.GetName().ToString();
		if (ParamName.Contains(TEXT("Particles.Enabled"))) bHasParticlesEnabled = true;
		if (ParamName.Contains(TEXT("SpawnRate"))) bHasSpawnRate = true;
	}

	if (bHasParticlesEnabled && bHasSpawnRate)
	{
		// Looks like a generated asset
		FFXGeneratorMetadata Metadata;
		Metadata.bIsGenerated = true;
		return Metadata;
	}

	return EmptyMetadata;
}
