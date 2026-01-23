// GasAbilityGenerator v4.13
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v4.13: Category C Full Automation - P3.2 GameplayCue Auto-Wiring (ExecuteGameplayCue nodes),
//        P3.1 Niagara Spawning Support (SpawnSystemAttached + cleanup), P2.1 Delegate Binding IR + Codegen
// v4.10: Widget Property Enhancement - ConfigureWidgetProperties lambda with dotted properties,
//        struct types (FSlateColor, FSlateBrush, FLinearColor, FVector2D), enum types (FEnumProperty, TEnumAsByte),
//        machine-readable warnings via FGenerationResult.Warnings, FSlateBrush field restrictions
// v4.0: Weapon Attachment TMap Automation - Fully automated HolsterAttachmentConfigs/WieldAttachmentConfigs
//       population via FScriptMapHelper for TMap<FGameplayTag, FWeaponAttachmentConfig>
// v3.9: NPC Pipeline - ActivitySchedule, GoalItem, Quest generators
//       ActivitySchedule: Creates UNPCActivitySchedule with UScheduledBehavior_AddNPCGoalByClass
//       instances for time-based goal assignment (concrete helper class replaces abstract base)
//       GoalItem: Creates UNPCGoalItem Blueprints with DefaultScore, Activity, BehaviorTree properties
//       Quest: Creates UQuest Blueprints (stub for editor completion)
// v3.8: Dialogue Tree Generation - Full dialogue tree creation from YAML with nodes, connections,
//       events (UNarrativeEvent), conditions (UNarrativeCondition), and alternative lines.
//       Creates UDialogueBlueprint with DialogueTemplate containing UDialogueNode_NPC/UDialogueNode_Player.
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
// v4.16: Compile validation (Contract 10 - Blueprint Compile Gate)
#include "Kismet2/CompilerResultsLog.h"
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
#include "BlueprintGameplayTagLibrary.h"  // v4.25.3: GameplayTag Blueprint functions
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Composites/BTComposite_SimpleParallel.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/Decorators/BTDecorator_Blackboard.h"
#include "BehaviorTree/BTService.h"
// v4.15: BehaviorTreeEditor includes for proper graph creation
#include "BehaviorTreeGraph.h"
#include "BehaviorTreeGraphNode_Root.h"
#include "EdGraphSchema_BehaviorTree.h"
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
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Materials/MaterialFunctionInstance.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
// v4.10: Switch expression includes
#include "Materials/MaterialExpressionQualitySwitch.h"
#include "Materials/MaterialExpressionShadingPathSwitch.h"
#include "Materials/MaterialExpressionFeatureLevelSwitch.h"
// v4.10: Static expression includes
#include "Materials/MaterialExpressionStaticSwitch.h"
#include "Materials/MaterialExpressionStaticBool.h"
#include "Materials/MaterialExpressionStaticBoolParameter.h"
// v4.10: Sobol expression includes
#include "Materials/MaterialExpressionSobol.h"
#include "Materials/MaterialExpressionTemporalSobol.h"
// v4.10: Additional VFX expression includes
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionMakeMaterialAttributes.h"
#include "Materials/MaterialExpressionPerInstanceRandom.h"
#include "Materials/MaterialExpressionPerInstanceFadeAmount.h"
#include "Materials/MaterialExpressionObjectOrientation.h"
#include "Materials/MaterialExpressionObjectRadius.h"
#include "Materials/MaterialExpressionObjectPositionWS.h"
#include "Materials/MaterialExpressionActorPositionWS.h"
#include "Materials/MaterialExpressionPreSkinnedPosition.h"
#include "Materials/MaterialExpressionPreSkinnedNormal.h"
#include "Materials/MaterialExpressionPreSkinnedLocalBounds.h"
#include "Materials/MaterialExpressionLightmapUVs.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionPixelNormalWS.h"
#include "Materials/MaterialExpressionTwoSidedSign.h"
#include "Materials/MaterialExpressionVertexTangentWS.h"
#include "Materials/MaterialExpressionLightVector.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionReflectionVectorWS.h"
#include "Materials/MaterialExpressionParticlePositionWS.h"
#include "Materials/MaterialExpressionParticleRadius.h"
#include "Materials/MaterialExpressionParticleRelativeTime.h"
#include "Materials/MaterialExpressionParticleMotionBlurFade.h"
#include "Materials/MaterialExpressionParticleRandom.h"
#include "Materials/MaterialExpressionParticleDirection.h"
#include "Materials/MaterialExpressionParticleSpeed.h"
#include "Materials/MaterialExpressionParticleSize.h"
#include "Materials/MaterialExpressionScreenPosition.h"
#include "Materials/MaterialExpressionViewSize.h"
#include "Materials/MaterialExpressionSceneTexelSize.h"
#include "Materials/MaterialExpressionDeltaTime.h"
// Note: MaterialExpressionRealTime.h doesn't exist in UE5.7 - use Time expression instead
#include "Materials/MaterialExpressionEyeAdaptation.h"
#include "Materials/MaterialExpressionAtmosphericFogColor.h"
#include "Materials/MaterialExpressionPrecomputedAOMask.h"
#include "Materials/MaterialExpressionGIReplace.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionMin.h"
#include "Materials/MaterialExpressionMax.h"
#include "Materials/MaterialExpressionCeil.h"
#include "Materials/MaterialExpressionFmod.h"
#include "Materials/MaterialExpressionTangent.h"
#include "Materials/MaterialExpressionRotator.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
// v4.9: Material instance includes
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
// v4.3: UMG Widget classes for programmatic layout
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/Spacer.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/GridPanel.h"
#include "Components/UniformGridPanel.h"
#include "Components/WrapBox.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
#include "Components/RichTextBlock.h"
#include "Components/CircularThrobber.h"
#include "Components/Throbber.h"
// v4.10: SlateTextureAtlasInterface and SlateBrushAsset for FSlateBrush resource validation
#include "Slate/SlateTextureAtlasInterface.h"
#include "Slate/SlateBrushAsset.h"
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
#include "Tales/DialogueSM.h"  // v3.7: UDialogueNode_NPC, UDialogueNode_Player, FDialogueLine
#include "Tales/NarrativeEvent.h"  // v3.7: UNarrativeEvent for dialogue events
#include "Tales/NarrativeCondition.h"  // v3.7: UNarrativeCondition for dialogue conditions
#include "GAS/NarrativeAbilitySystemComponent.h"  // v4.21: For delegate binding automation
#include "DialogueBlueprint.h"  // v3.7: UDialogueBlueprint for dialogue tree generation
#include "QuestBlueprint.h"  // v3.9.4: UQuestBlueprint for quest state machine generation
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"  // v4.13: Category C P3.1 VFX Spawning
#include "NiagaraFunctionLibrary.h"  // v4.25.3: SpawnSystemAttached function
// v3.9: NPC Pipeline includes
#include "AI/Activities/NPCActivitySchedule.h"
#include "AI/Activities/NPCGoalItem.h"
#include "Tales/Quest.h"
#include "Tales/QuestSM.h"      // v3.9: UQuestState, UQuestBranch for quest state machine
#include "Tales/QuestTask.h"    // v3.9: UNarrativeTask for quest tasks
#include "GasScheduledBehaviorHelpers.h"  // v3.9: Concrete helper for UScheduledBehavior_AddNPCGoal
#include "Camera/CameraShakeBase.h"  // v4.1: UCameraShakeBase for dialogue camera shake

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
#include "K2Node_FunctionResult.h"  // v4.14: Custom function return node
#include "K2Node_CallParentFunction.h"  // v2.8.3: Call parent function
#include "K2Node_LatentAbilityCall.h"  // v4.15: AbilityTask node support (Track B GAS Audit)
#include "K2Node_AddDelegate.h"        // v4.21: Delegate binding automation
#include "K2Node_RemoveDelegate.h"     // v4.21: Delegate unbinding automation
#include "K2Node_CreateDelegate.h"     // v4.21: Delegate creation automation
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"  // v4.15: WaitDelay AbilityTask class
#include "Abilities/Tasks/AbilityTask_WaitAttributeChange.h"  // v4.22: Section 11 Attribute Change Delegate binding
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
// v4.0: Gameplay Cue includes
#include "GameplayCueNotify_Burst.h"
#include "GameplayCueNotify_Actor.h"
#include "Misc/App.h"  // v2.5.0: For FApp::GetProjectName()

// v2.7.0: Narrative Pro includes for WellKnownFunctions table
#include "Items/InventoryComponent.h"
#include "Items/WeaponItem.h"
#include "Items/EquippableItem.h"

// v3.9: Log category for generator messages
DEFINE_LOG_CATEGORY_STATIC(LogGasAbilityGenerator, Log, All);

// v2.1.9: Static member initialization for manifest validation
const FManifestData* FGeneratorBase::ActiveManifest = nullptr;

// v3.0: Static member initialization for dry run and force mode
bool FGeneratorBase::bDryRunMode = false;
bool FGeneratorBase::bForceMode = false;
FDryRunSummary FGeneratorBase::DryRunSummary;
FString FGeneratorBase::CurrentManifestPath;

// v4.9.1: Static member initialization for generated materials cache (MIC parent lookup)
TMap<FString, UMaterialInterface*> FMaterialGenerator::GeneratedMaterialsCache;
// v4.10: Static member initialization for generated material functions cache
TMap<FString, UMaterialFunctionInterface*> FMaterialGenerator::GeneratedMaterialFunctionsCache;
// v4.14: Static member initialization for generated blackboards cache (BT blackboard lookup)
TMap<FString, UBlackboardData*> FBlackboardGenerator::GeneratedBlackboardsCache;

// v3.0: Generator version constant for metadata tracking
static const FString GENERATOR_VERSION = TEXT("4.13");

// v4.10: Explicit enum mapping tables for switch expressions (hallucination-proof)
static const TMap<FString, int32> QualitySwitchKeyMap = {
	{ TEXT("low"), 0 },
	{ TEXT("high"), 1 },     // Index 1, NOT 2!
	{ TEXT("medium"), 2 },   // Index 2, NOT 1!
	{ TEXT("epic"), 3 }
};

static const TMap<FString, int32> ShadingPathSwitchKeyMap = {
	{ TEXT("deferred"), 0 },
	{ TEXT("forward"), 1 },
	{ TEXT("mobile"), 2 }
};

static const TMap<FString, int32> FeatureLevelSwitchKeyMap = {
	{ TEXT("es3_1"), 1 },
	{ TEXT("sm5"), 3 },
	{ TEXT("sm6"), 4 }
};

// v4.10: Deprecated feature level keys (will log warning)
static const TSet<FString> DeprecatedFeatureLevelKeys = {
	TEXT("es2"),   // Index 0 - ES2_REMOVED
	TEXT("sm4")    // Index 2 - SM4_REMOVED
};

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

// v4.22: Session cache for Blueprint classes created during this generation session
// Maps asset name (e.g., "GE_BackstabBonus") to its compiled UClass*
// This allows TSubclassOf resolution to find Blueprints created earlier in the same session
// NOTE: NOT static - needs external linkage for commandlet to clear it between sessions
TMap<FString, UClass*> GSessionBlueprintClassCache;

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

// v4.12.4: DISK-TRUTH CHECK - Do NOT replace with AssetRegistry or DoesPackageExist!
// Canonical helper for create/skip decisions. Requires valid long package name (not object path).
// Contract: Caller must pass package path like "/Game/Folder/AssetName", not "/Game/Folder/AssetName.AssetName"
bool FGeneratorBase::DoesAssetExistOnDisk(const FString& AssetPath)
{
	// Normalize: ensure leading slash
	FString PackageName = AssetPath;
	if (!PackageName.StartsWith(TEXT("/")))
	{
		PackageName = TEXT("/") + PackageName;
	}

	// Convert to filesystem path and check actual file existence
	FString FilePath;
	if (FPackageName::TryConvertLongPackageNameToFilename(PackageName, FilePath, FPackageName::GetAssetPackageExtension()))
	{
		return IFileManager::Get().FileExists(*FilePath);
	}

	// Conversion failed = malformed package path = cannot exist on disk
	return false;
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

	// Check for existing metadata (v4.12.5: registry-aware via struct API)
	// Uses TryGetMetadata to check both AssetUserData AND registry fallback
	FGeneratorMetadata Metadata;
	if (!GeneratorMetadataHelpers::TryGetMetadata(ExistingAsset, Metadata))
	{
		// No metadata in either source - asset exists but wasn't generated by us
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

	// Has metadata - compare hashes using struct helpers (behavior-identical to UObject version)
	bool bInputChanged = Metadata.HasInputChanged(InputHash);

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

	bool bOutputChanged = Metadata.HasOutputChanged(CurrentOutputHash);

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
		Result.StoredInputHash = static_cast<uint64>(Metadata.InputHash);
		Result.CurrentInputHash = InputHash;
		Result.StoredOutputHash = static_cast<uint64>(Metadata.OutputHash);
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
			// v4.23 FAIL-FAST: S29 - Manifest references CooldownGE that cannot be resolved
			// Note: This is a void helper function, so we log the error - caller will see it in generation log
			LogGeneration(FString::Printf(TEXT("[E_COOLDOWNGE_NOT_FOUND] %s | CooldownGE not found: %s"), *Definition.Name, *Definition.CooldownGameplayEffectClass));
		}
	}

	// v4.8: Set InputTag for NarrativeGameplayAbility
	if (!Definition.InputTag.IsEmpty())
	{
		FStructProperty* InputTagProperty = CastField<FStructProperty>(AbilityCDO->GetClass()->FindPropertyByName(TEXT("InputTag")));
		if (InputTagProperty && InputTagProperty->Struct && InputTagProperty->Struct->GetName() == TEXT("GameplayTag"))
		{
			FGameplayTag* TagPtr = InputTagProperty->ContainerPtrToValuePtr<FGameplayTag>(AbilityCDO);
			if (TagPtr)
			{
				*TagPtr = FGameplayTag::RequestGameplayTag(FName(*Definition.InputTag), false);
				if (TagPtr->IsValid())
				{
					LogGeneration(FString::Printf(TEXT("  Set InputTag: %s"), *Definition.InputTag));
					bPoliciesModified = true;
				}
				else
				{
					LogGeneration(FString::Printf(TEXT("  Warning: Invalid InputTag: %s"), *Definition.InputTag));
				}
			}
		}
	}

	// v4.8: Set bActivateAbilityOnGranted for NarrativeGameplayAbility
	if (Definition.bActivateAbilityOnGranted)
	{
		FBoolProperty* ActivateOnGrantedProperty = CastField<FBoolProperty>(AbilityCDO->GetClass()->FindPropertyByName(TEXT("bActivateAbilityOnGranted")));
		if (ActivateOnGrantedProperty)
		{
			ActivateOnGrantedProperty->SetPropertyValue_InContainer(AbilityCDO, true);
			LogGeneration(TEXT("  Set bActivateAbilityOnGranted: true"));
			bPoliciesModified = true;
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

// v4.28: Find fragment class by name for Item Fragments system
// Per Item_Generation_Capability_Audit.md: Fragment allowlist is AmmoFragment, PoisonableFragment
// Class path format: /Script/NarrativeArsenal.<ClassWithoutUPrefix>
UClass* FGeneratorBase::FindFragmentClass(const FString& FragmentClassName)
{
	// Static set for warning deduplication per session
	static TSet<FString> WarnedFragmentClasses;

	// Normalize class name - remove U prefix if present (per audit: class paths don't use U prefix)
	FString NormalizedName = FragmentClassName;
	if (NormalizedName.StartsWith(TEXT("U")))
	{
		NormalizedName = NormalizedName.RightChop(1);
	}

	// Fragment allowlist per audit document
	static const TArray<FString> AllowedFragments = {
		TEXT("AmmoFragment"),
		TEXT("PoisonableFragment")
	};

	// Check if fragment is in allowlist
	bool bIsAllowed = false;
	for (const FString& Allowed : AllowedFragments)
	{
		if (NormalizedName.Equals(Allowed, ESearchCase::IgnoreCase))
		{
			NormalizedName = Allowed; // Use canonical casing
			bIsAllowed = true;
			break;
		}
	}

	if (!bIsAllowed)
	{
		// Warn only once per class per session
		if (!WarnedFragmentClasses.Contains(NormalizedName))
		{
			WarnedFragmentClasses.Add(NormalizedName);
			UE_LOG(LogTemp, Warning, TEXT("[W_FRAGMENT_NOT_IN_ALLOWLIST] Fragment class '%s' not in allowlist. Allowed: AmmoFragment, PoisonableFragment"), *FragmentClassName);
		}
	}

	// Try /Script/NarrativeArsenal path first (primary location)
	FString ScriptPath = FString::Printf(TEXT("/Script/NarrativeArsenal.%s"), *NormalizedName);
	UClass* FoundClass = FindObject<UClass>(nullptr, *ScriptPath);
	if (FoundClass)
	{
		return FoundClass;
	}

	// Try with U prefix in path
	FString ScriptPathWithU = FString::Printf(TEXT("/Script/NarrativeArsenal.U%s"), *NormalizedName);
	FoundClass = FindObject<UClass>(nullptr, *ScriptPathWithU);
	if (FoundClass)
	{
		return FoundClass;
	}

	// Fallback: try direct class name lookup
	FoundClass = FindObject<UClass>(nullptr, *FragmentClassName);
	if (FoundClass)
	{
		return FoundClass;
	}

	return nullptr;
}

// v4.28: Clear fragment warning deduplication set (called at start of generation session)
void FGeneratorBase::ClearFragmentWarningCache()
{
	// The static set in FindFragmentClass is cleared by restarting the commandlet/editor
	// This function exists for API consistency if explicit clearing is needed
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
	UE_LOG(LogGasAbilityGenerator, Display, TEXT("%s"), *Message);
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
				// v4.23 FAIL-FAST: S30/F1 - Manifest references class that doesn't exist
				// Return invalid PinType to cause downstream failure
				UE_LOG(LogTemp, Error, TEXT("[E_PINTYPE_CLASS_NOT_FOUND] GetPinTypeFromString: Class '%s' not found"), *ClassName);
				// Return empty pin type - will cause variable/node creation to fail
				return FEdGraphPinType();
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
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("Enumeration");
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
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("InputAction");
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
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("InputMappingContext");
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

	if (!Blueprint)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Gameplay Effect Blueprint"));
	}

	// v4.16: Compile with validation (Contract 10 - Blueprint Compile Gate)
	// Compile FIRST to ensure GeneratedClass and CDO exist before setting properties
	// CDO properties set before compile will be lost when compile recreates the class
	FCompilerResultsLog CompileLog;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &CompileLog);

	if (CompileLog.NumErrors > 0)
	{
		TArray<FString> ErrorMessages;
		for (const TSharedRef<FTokenizedMessage>& Msg : CompileLog.Messages)
		{
			if (Msg->GetSeverity() == EMessageSeverity::Error)
			{
				ErrorMessages.Add(Msg->ToText().ToString());
			}
		}

		UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Gameplay Effect Blueprint compilation failed with %d errors: %s"),
			*Definition.Name, CompileLog.NumErrors, *FString::Join(ErrorMessages, TEXT("; ")));

		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Compile failed: %s"), *FString::Join(ErrorMessages, TEXT("; "))));
		Result.AssetPath = AssetPath;
		Result.GeneratorId = TEXT("GameplayEffect");
		Result.DetermineCategory();
		return Result;
	}

	if (!Blueprint->GeneratedClass)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to compile Gameplay Effect Blueprint"));
	}

	// v4.22: Cache the Blueprint class for same-session TSubclassOf resolution
	GSessionBlueprintClassCache.Add(Definition.Name, Blueprint->GeneratedClass);
	LogGeneration(FString::Printf(TEXT("  Cached Blueprint class for same-session resolution: %s"), *Definition.Name));

	// Get the CDO to configure properties (AFTER compile)
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
						// v4.23 FAIL-FAST: S31 - Manifest references attribute property that doesn't exist
						// v4.23.1: Phase 3 spec-complete - added Subsystem, RequestedProperty
						LogGeneration(FString::Printf(TEXT("[E_GE_PROPERTY_NOT_FOUND] %s | Subsystem: GAS | Property '%s' not found on class '%s' | RequestedProperty: %s"),
							*Definition.Name, *PropertyName, *ClassName, *PropertyName));
						return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
							FString::Printf(TEXT("Attribute property '%s' not found on class '%s'"), *PropertyName, *ClassName));
					}
				}
				else
				{
					// v4.23 FAIL-FAST: S32 - Manifest references attribute set class that doesn't exist
					// v4.23.1: Phase 3 spec-complete - added Subsystem
					LogGeneration(FString::Printf(TEXT("[E_GE_ATTRIBUTESET_NOT_FOUND] %s | Subsystem: GAS | Attribute set class '%s' not found"), *Definition.Name, *ClassName));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						FString::Printf(TEXT("Attribute set class '%s' not found"), *ClassName));
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
						// v4.23 FAIL-FAST: S33 - Manifest references SetByCaller tag that doesn't exist
						LogGeneration(FString::Printf(TEXT("[E_GE_SETBYCALLER_TAG_NOT_FOUND] %s | SetByCaller tag '%s' not found"), *Definition.Name, *ModDef.SetByCallerTag));
						return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
							FString::Printf(TEXT("SetByCaller tag '%s' not found in GameplayTags"), *ModDef.SetByCallerTag));
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

	// v4.15: Add granted tags using UE5.7 component system (replaces deprecated InheritableOwnedTagsContainer)
	// UTargetTagsGameplayEffectComponent is the UE5.3+ way to grant tags to targets
	if (Definition.GrantedTags.Num() > 0)
	{
		// Build the tag container
		FInheritedTagContainer TagContainer;
		for (const FString& TagName : Definition.GrantedTags)
		{
			FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagName), false);
			if (Tag.IsValid())
			{
				TagContainer.AddTag(Tag);
				LogGeneration(FString::Printf(TEXT("  Added granted tag: %s"), *TagName));
			}
			else
			{
				// v4.23 FAIL-FAST: S34 - Manifest references tag that doesn't exist
				LogGeneration(FString::Printf(TEXT("[E_GE_TAG_NOT_FOUND] %s | Tag '%s' not found in GameplayTags"), *Definition.Name, *TagName));
				return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
					FString::Printf(TEXT("GrantedTag '%s' not found in GameplayTags"), *TagName));
			}
		}

		// Get or create the TargetTags component and apply tags
		UTargetTagsGameplayEffectComponent& TagsComponent = Effect->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
		TagsComponent.SetAndApplyTargetTagChanges(TagContainer);
		LogGeneration(FString::Printf(TEXT("  Applied %d tags via UTargetTagsGameplayEffectComponent"), Definition.GrantedTags.Num()));
	}

	// v4.14: Removed redundant compile - compile is done BEFORE setting CDO properties now
	// Recompiling here would recreate the GeneratedClass and wipe all CDO changes

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
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("GameplayEffect");
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

	// v4.13: Category C - P1.2 Transition Prelude Validation for form abilities
	TArray<FString> ValidationWarnings;
	ValidateFormAbility(Definition, ValidationWarnings);
	for (const FString& Warning : ValidationWarnings)
	{
		Result.Warnings.Add(Warning);
		// Log errors and warnings for visibility
		if (Warning.StartsWith(TEXT("E_")))
		{
			LogGeneration(FString::Printf(TEXT("  ERROR: %s"), *Warning));
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  WARNING: %s"), *Warning));
		}
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

		bool bEventGraphSuccess = FEventGraphGenerator::GenerateEventGraph(Blueprint, GraphDef, ProjectRoot);

		// v2.6.7: Check for missing dependencies FIRST - return Deferred to retry later
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

		// v4.16: Event graph failure propagation (P2 - Caller Propagation)
		// If GenerateEventGraph returned false (not deferred), abort generation
		if (!bEventGraphSuccess)
		{
			UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Event graph generation failed"), *Definition.Name);
			LogGeneration(TEXT("  FAILED: Event graph generation failed - see errors above"));

			Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				TEXT("Event graph generation failed"));
			Result.AssetPath = AssetPath;
			Result.GeneratorId = TEXT("GameplayAbility");
			Result.DetermineCategory();
			return Result;
		}

		LogGeneration(FString::Printf(TEXT("  Generated event graph with %d nodes"), Definition.EventGraphNodes.Num()));
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

	// v4.16: Restructured - Category C nodes BEFORE save (D-009, D-010)
	// Previously had checkpoint save before Category C, violating single-save invariant
	if (Definition.CueTriggers.Num() > 0)
	{
		int32 CueNodesGenerated = FEventGraphGenerator::GenerateCueTriggerNodes(Blueprint, Definition.CueTriggers);
		LogGeneration(FString::Printf(TEXT("  Generated %d/%d cue trigger nodes"), CueNodesGenerated, Definition.CueTriggers.Num()));
	}
	if (Definition.VFXSpawns.Num() > 0)
	{
		int32 VFXNodesGenerated = FEventGraphGenerator::GenerateVFXSpawnNodes(Blueprint, Definition.VFXSpawns);
		LogGeneration(FString::Printf(TEXT("  Generated %d/%d VFX spawn nodes"), VFXNodesGenerated, Definition.VFXSpawns.Num()));
	}
	if (Definition.DelegateBindings.Num() > 0)
	{
		int32 DelegateNodesGenerated = FEventGraphGenerator::GenerateDelegateBindingNodes(Blueprint, Definition.DelegateBindings);
		LogGeneration(FString::Printf(TEXT("  Generated %d/%d delegate binding handler events"), DelegateNodesGenerated, Definition.DelegateBindings.Num()));
	}
	// v4.22: Section 11 - Generate attribute binding nodes (AbilityTask-based)
	if (Definition.AttributeBindings.Num() > 0)
	{
		int32 AttrNodesGenerated = FEventGraphGenerator::GenerateAttributeBindingNodes(Blueprint, Definition.AttributeBindings);
		LogGeneration(FString::Printf(TEXT("  Generated %d/%d attribute binding tasks"), AttrNodesGenerated, Definition.AttributeBindings.Num()));
	}

	// v4.14: Generate custom Blueprint functions
	if (Definition.CustomFunctions.Num() > 0)
	{
		LogGeneration(FString::Printf(TEXT("  Generating %d custom function(s)"), Definition.CustomFunctions.Num()));
		int32 FunctionsGenerated = 0;
		for (const FManifestCustomFunctionDefinition& FuncDef : Definition.CustomFunctions)
		{
			if (FEventGraphGenerator::GenerateCustomFunction(Blueprint, FuncDef, ProjectRoot))
			{
				FunctionsGenerated++;
				LogGeneration(FString::Printf(TEXT("    Generated custom function: %s"), *FuncDef.FunctionName));
			}
			else
			{
				// v4.23 FAIL-FAST: S35 - Manifest references custom function that failed to generate
				LogGeneration(FString::Printf(TEXT("[E_GA_CUSTOMFUNCTION_FAILED] %s | Failed to generate custom function: %s"), *Definition.Name, *FuncDef.FunctionName));
				return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
					FString::Printf(TEXT("Failed to generate custom function '%s'"), *FuncDef.FunctionName));
			}
		}
		LogGeneration(FString::Printf(TEXT("  Generated %d/%d custom functions"), FunctionsGenerated, Definition.CustomFunctions.Num()));
	}

	// v4.22: AUDIT LOGGING POINT 2 - Before final CompileBlueprint
	FEventGraphGenerator::LogNodePositionsDiagnostic(Blueprint, TEXT("POINT_2_BeforeCompileBlueprint"));

	// v4.22: Verify delegate node connections right before compile
	{
		UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
		if (EventGraph)
		{
			int32 AddConnected = 0, AddNotConnected = 0, RemoveConnected = 0, RemoveNotConnected = 0;
			for (UEdGraphNode* Node : EventGraph->Nodes)
			{
				if (UK2Node_AddDelegate* AddNode = Cast<UK2Node_AddDelegate>(Node))
				{
					UEdGraphPin* TargetPin = AddNode->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input);
					if (TargetPin && TargetPin->LinkedTo.Num() > 0)
						AddConnected++;
					else
						AddNotConnected++;
				}
				else if (UK2Node_RemoveDelegate* RemoveNode = Cast<UK2Node_RemoveDelegate>(Node))
				{
					UEdGraphPin* TargetPin = RemoveNode->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input);
					if (TargetPin && TargetPin->LinkedTo.Num() > 0)
						RemoveConnected++;
					else
						RemoveNotConnected++;
				}
			}
			if (AddConnected > 0 || AddNotConnected > 0 || RemoveConnected > 0 || RemoveNotConnected > 0)
			{
				LogGeneration(FString::Printf(TEXT("  [PRE-COMPILE VERIFY] AddDelegate: %d connected, %d not; RemoveDelegate: %d connected, %d not"),
					AddConnected, AddNotConnected, RemoveConnected, RemoveNotConnected));
			}
		}
	}

	// v4.16: Final compile with validation (Contract 10 - Blueprint Compile Gate)
	// Single compile after ALL nodes added - replaces checkpoint save pattern
	FCompilerResultsLog CompileLog;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &CompileLog);

	// v4.22: AUDIT LOGGING POINT 3 - After CompileBlueprint, before ReapplyNodePositions
	FEventGraphGenerator::LogNodePositionsDiagnostic(Blueprint, TEXT("POINT_3_AfterCompileBlueprint"));

	// v4.20.11: Re-apply node positions after compilation (compilation may reset positions)
	FEventGraphGenerator::ReapplyNodePositions(Blueprint);

	// v4.22: AUDIT LOGGING POINT 4 - After ReapplyNodePositions
	FEventGraphGenerator::LogNodePositionsDiagnostic(Blueprint, TEXT("POINT_4_AfterReapplyNodePositions"));

	// v4.16: Check compile result - abort save if errors (P3, P4)
	if (CompileLog.NumErrors > 0)
	{
		TArray<FString> ErrorMessages;
		for (const TSharedRef<FTokenizedMessage>& Msg : CompileLog.Messages)
		{
			if (Msg->GetSeverity() == EMessageSeverity::Error)
			{
				ErrorMessages.Add(Msg->ToText().ToString());
			}
		}

		UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Blueprint compilation failed with %d errors: %s"),
			*Definition.Name, CompileLog.NumErrors, *FString::Join(ErrorMessages, TEXT("; ")));

		LogGeneration(FString::Printf(TEXT("  COMPILE FAILED with %d errors:"), CompileLog.NumErrors));
		for (const FString& Err : ErrorMessages)
		{
			LogGeneration(FString::Printf(TEXT("    - %s"), *Err));
		}

		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Compile failed: %s"), *FString::Join(ErrorMessages, TEXT("; "))));
		Result.AssetPath = AssetPath;
		Result.GeneratorId = TEXT("GameplayAbility");
		Result.DetermineCategory();
		return Result;
	}

	// v4.22: Cache the Blueprint class for same-session TSubclassOf resolution
	if (Blueprint->GeneratedClass)
	{
		GSessionBlueprintClassCache.Add(Definition.Name, Blueprint->GeneratedClass);
		LogGeneration(FString::Printf(TEXT("  Cached Blueprint class for same-session resolution: %s"), *Definition.Name));
	}

	// Mark dirty and register
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Blueprint);

	// v4.22: AUDIT LOGGING POINT 5 - Before SavePackage
	FEventGraphGenerator::LogNodePositionsDiagnostic(Blueprint, TEXT("POINT_5_BeforeSavePackage"));

	// v4.22: Fix override event node positions - ensure NodePosX != 0 for serialization
	// Root cause: UE skips serializing default values (NodePosX=0), causing positions to not persist
	// Fix: Clamp override event nodes to minimum X=16 (one grid unit)
	// Audit: Claude-GPT dual audit 2026-01-22, approved by Erdem
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (!Graph) continue;

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
			if (EventNode && EventNode->bOverrideFunction)
			{
				// Only clamp if NodePosX is exactly 0 (default value that won't serialize)
				// Do NOT clamp negative X values - those are valid authored positions from manual nudge
				if (EventNode->NodePosX == 0)
				{
					EventNode->NodePosX = 16;  // One grid unit - ensures serialization
					LogGeneration(FString::Printf(TEXT("  [POSITION_FIX] Override event '%s' NodePosX clamped from 0 to 16"), *EventNode->GetNodeTitle(ENodeTitleType::ListView).ToString()));
				}
			}
		}
	}

	// v4.16: Single save at end (D-011 invariant) - only reached if compile succeeded
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Gameplay Ability: %s (with %d variables)"), *Definition.Name, Definition.Variables.Num()));

	// v3.0: Store metadata for regeneration tracking
	StoreBlueprintMetadata(Blueprint, TEXT("GameplayAbility"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("GameplayAbility");
	Result.DetermineCategory();
	return Result;
}

// v4.13: Category C - P1.2 Transition Prelude Validation
void FGameplayAbilityGenerator::ValidateFormAbility(const FManifestGameplayAbilityDefinition& Definition, TArray<FString>& OutWarnings)
{
	// Only validate form abilities (GA_Father* but not attack/utility abilities)
	// Form abilities: GA_FatherCrawler, GA_FatherArmor, GA_FatherExoskeleton, GA_FatherSymbiote, GA_FatherEngineer
	static TArray<FString> FormAbilityNames = {
		TEXT("GA_FatherCrawler"),
		TEXT("GA_FatherArmor"),
		TEXT("GA_FatherExoskeleton"),
		TEXT("GA_FatherSymbiote"),
		TEXT("GA_FatherEngineer")
	};

	if (!FormAbilityNames.Contains(Definition.Name))
	{
		return;  // Not a form ability, skip validation
	}

	FString ContextPath = FString::Printf(TEXT("GameplayAbility/%s"), *Definition.Name);

	// Required: Father.State.Alive in activation_required_tags
	if (!Definition.Tags.ActivationRequiredTags.Contains(TEXT("Father.State.Alive")))
	{
		OutWarnings.Add(FString::Printf(TEXT("E_FORM_MISSING_ALIVE | %s | Form ability must require Father.State.Alive | Add 'Father.State.Alive' to activation_required_tags"), *ContextPath));
	}

	// Required: Father.State.Recruited in activation_required_tags
	if (!Definition.Tags.ActivationRequiredTags.Contains(TEXT("Father.State.Recruited")))
	{
		OutWarnings.Add(FString::Printf(TEXT("E_FORM_MISSING_RECRUITED | %s | Form ability must require Father.State.Recruited | Add 'Father.State.Recruited' to activation_required_tags"), *ContextPath));
	}

	// Required: Father.State.Dormant in activation_blocked_tags
	if (!Definition.Tags.ActivationBlockedTags.Contains(TEXT("Father.State.Dormant")))
	{
		OutWarnings.Add(FString::Printf(TEXT("E_FORM_MISSING_DORMANT | %s | Form ability must block Father.State.Dormant | Add 'Father.State.Dormant' to activation_blocked_tags"), *ContextPath));
	}

	// Warning: Father.State.Transitioning should be in activation_blocked_tags
	if (!Definition.Tags.ActivationBlockedTags.Contains(TEXT("Father.State.Transitioning")))
	{
		OutWarnings.Add(FString::Printf(TEXT("W_FORM_MISSING_TRANSITIONING | %s | Form ability should block Father.State.Transitioning for safety | Add 'Father.State.Transitioning' to activation_blocked_tags"), *ContextPath));
	}

	// Warning: cancel_abilities_with_tag should not be empty
	if (Definition.Tags.CancelAbilitiesWithTag.Num() == 0)
	{
		OutWarnings.Add(FString::Printf(TEXT("W_FORM_NO_CANCEL_TAG | %s | Form ability should cancel other forms | Add 'Ability.Father.Form' or similar to cancel_abilities_with_tag"), *ContextPath));
	}
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
			// v4.23 FAIL-FAST: Manifest references component class that cannot be found
			LogGeneration(FString::Printf(TEXT("[E_COMPONENT_CLASS_NOT_FOUND] %s | Component class not found: %s"),
				*Definition.Name, *CompDef.Type));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("Component class '%s' not found"), *CompDef.Type));
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

		bool bEventGraphSuccess = FEventGraphGenerator::GenerateEventGraph(Blueprint, GraphDef, ProjectRoot);

		// Check for missing dependencies FIRST - return Deferred to retry later
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

		// v4.16: Event graph failure propagation (P2 - Caller Propagation)
		if (!bEventGraphSuccess)
		{
			UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Event graph generation failed"), *Definition.Name);
			LogGeneration(TEXT("  FAILED: Inline event graph generation failed - see errors above"));

			Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				TEXT("Event graph generation failed"));
			Result.AssetPath = AssetPath;
			Result.GeneratorId = TEXT("ActorBlueprint");
			Result.DetermineCategory();
			return Result;
		}

		LogGeneration(FString::Printf(TEXT("  Generated inline event graph with %d nodes"), Definition.EventGraphNodes.Num()));
	}
	// v2.2.0: Fall back to referenced event graph lookup
	else if (!Definition.EventGraphName.IsEmpty() && ManifestData)
	{
		FEventGraphGenerator::ClearMissingDependencies();

		const FManifestEventGraphDefinition* GraphDef = ManifestData->FindEventGraphByName(Definition.EventGraphName);
		if (GraphDef)
		{
			bool bEventGraphSuccess = FEventGraphGenerator::GenerateEventGraph(Blueprint, *GraphDef, ProjectRoot);

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

			// v4.16: Event graph failure propagation (P2 - Caller Propagation)
			if (!bEventGraphSuccess)
			{
				UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Event graph generation failed"), *Definition.Name);
				LogGeneration(FString::Printf(TEXT("  FAILED: Event graph '%s' generation failed - see errors above"), *Definition.EventGraphName));

				Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
					TEXT("Event graph generation failed"));
				Result.AssetPath = AssetPath;
				Result.GeneratorId = TEXT("ActorBlueprint");
				Result.DetermineCategory();
				return Result;
			}

			LogGeneration(FString::Printf(TEXT("  Applied event graph: %s"), *Definition.EventGraphName));
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
				// v4.23 FAIL-FAST: S36 - Manifest references function override that failed to generate
				LogGeneration(FString::Printf(TEXT("[E_BP_FUNCTIONOVERRIDE_FAILED] %s | Failed to generate function override: %s"), *Definition.Name, *OverrideDef.FunctionName));
				return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
					FString::Printf(TEXT("Failed to generate function override '%s'"), *OverrideDef.FunctionName));
			}
		}
	}

	// v4.16: Compile with validation (Contract 10 - Blueprint Compile Gate)
	FCompilerResultsLog CompileLog;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &CompileLog);

	// v4.20.11: Re-apply node positions after compilation (compilation may reset positions)
	FEventGraphGenerator::ReapplyNodePositions(Blueprint);

	// v4.16: Check compile result - abort save if errors (P3, P4)
	if (CompileLog.NumErrors > 0)
	{
		TArray<FString> ErrorMessages;
		for (const TSharedRef<FTokenizedMessage>& Msg : CompileLog.Messages)
		{
			if (Msg->GetSeverity() == EMessageSeverity::Error)
			{
				ErrorMessages.Add(Msg->ToText().ToString());
			}
		}

		UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Blueprint compilation failed with %d errors: %s"),
			*Definition.Name, CompileLog.NumErrors, *FString::Join(ErrorMessages, TEXT("; ")));

		LogGeneration(FString::Printf(TEXT("  COMPILE FAILED with %d errors:"), CompileLog.NumErrors));
		for (const FString& Err : ErrorMessages)
		{
			LogGeneration(FString::Printf(TEXT("    - %s"), *Err));
		}

		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Compile failed: %s"), *FString::Join(ErrorMessages, TEXT("; "))));
		Result.AssetPath = AssetPath;
		Result.GeneratorId = TEXT("ActorBlueprint");
		Result.DetermineCategory();
		return Result;
	}

	// Mark dirty and register
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Blueprint);

	// Save package (only reached if compile succeeded)
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Actor Blueprint: %s (with %d variables)"), *Definition.Name, Definition.Variables.Num()));

	// v3.0: Store metadata for regeneration tracking
	StoreBlueprintMetadata(Blueprint, TEXT("ActorBlueprint"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("ActorBlueprint");
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
		// v4.23 FAIL-FAST: F2 - Manifest references parent class that doesn't exist
		if (!Definition.ParentClass.IsEmpty())
		{
			LogGeneration(FString::Printf(TEXT("[E_WBP_PARENT_NOT_FOUND] %s | Parent class not found: %s"), *Definition.Name, *Definition.ParentClass));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("Widget parent class '%s' not found"), *Definition.ParentClass));
		}
		// If ParentClass is empty/unspecified, default to UUserWidget (Type D - allowed)
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

	// ============================================================================
	// v4.3: Widget Tree Construction - Programmatic visual layout
	// ============================================================================
	if (Definition.WidgetTree.Widgets.Num() > 0 && WidgetBP->WidgetTree)
	{
		UWidgetTree* Tree = WidgetBP->WidgetTree;

		// Helper: Create widget by type
		auto CreateWidgetByType = [Tree](const FString& TypeName, const FName& WidgetName) -> UWidget*
		{
			FString TypeLower = TypeName.ToLower();

			// Panel types
			if (TypeLower == TEXT("canvaspanel") || TypeLower == TEXT("canvas"))
				return Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("verticalbox") || TypeLower == TEXT("vbox"))
				return Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("horizontalbox") || TypeLower == TEXT("hbox"))
				return Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("overlay"))
				return Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("gridpanel") || TypeLower == TEXT("grid"))
				return Tree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("uniformgridpanel") || TypeLower == TEXT("uniformgrid"))
				return Tree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("wrapbox") || TypeLower == TEXT("wrap"))
				return Tree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("scrollbox") || TypeLower == TEXT("scroll"))
				return Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), WidgetName);

			// Container types
			else if (TypeLower == TEXT("border"))
				return Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("sizebox"))
				return Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("scalebox"))
				return Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), WidgetName);

			// Interactive types
			else if (TypeLower == TEXT("button"))
				return Tree->ConstructWidget<UButton>(UButton::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("checkbox"))
				return Tree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("slider"))
				return Tree->ConstructWidget<USlider>(USlider::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("combobox") || TypeLower == TEXT("comboboxstring"))
				return Tree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("editabletext"))
				return Tree->ConstructWidget<UEditableText>(UEditableText::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("editabletextbox") || TypeLower == TEXT("textbox"))
				return Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), WidgetName);

			// Display types
			else if (TypeLower == TEXT("textblock") || TypeLower == TEXT("text"))
				return Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("richtextblock") || TypeLower == TEXT("richtext"))
				return Tree->ConstructWidget<URichTextBlock>(URichTextBlock::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("image"))
				return Tree->ConstructWidget<UImage>(UImage::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("progressbar") || TypeLower == TEXT("progress"))
				return Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("throbber"))
				return Tree->ConstructWidget<UThrobber>(UThrobber::StaticClass(), WidgetName);
			else if (TypeLower == TEXT("circularthrobber"))
				return Tree->ConstructWidget<UCircularThrobber>(UCircularThrobber::StaticClass(), WidgetName);

			// Layout helpers
			else if (TypeLower == TEXT("spacer"))
				return Tree->ConstructWidget<USpacer>(USpacer::StaticClass(), WidgetName);

			return nullptr;
		};

		// Helper: Configure slot based on parent type
		auto ConfigureSlot = [](UWidget* Widget, UPanelWidget* Parent, const FManifestWidgetSlotDefinition& SlotDef)
		{
			UPanelSlot* Slot = Widget->Slot;
			if (!Slot) return;

			// Canvas Panel Slot
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
			{
				// Anchors
				if (!SlotDef.Anchors.IsEmpty())
				{
					FString AnchorsLower = SlotDef.Anchors.ToLower();
					FAnchors Anchors;
					if (AnchorsLower == TEXT("topleft")) Anchors = FAnchors(0.f, 0.f, 0.f, 0.f);
					else if (AnchorsLower == TEXT("topcenter")) Anchors = FAnchors(0.5f, 0.f, 0.5f, 0.f);
					else if (AnchorsLower == TEXT("topright")) Anchors = FAnchors(1.f, 0.f, 1.f, 0.f);
					else if (AnchorsLower == TEXT("centerleft") || AnchorsLower == TEXT("left")) Anchors = FAnchors(0.f, 0.5f, 0.f, 0.5f);
					else if (AnchorsLower == TEXT("center")) Anchors = FAnchors(0.5f, 0.5f, 0.5f, 0.5f);
					else if (AnchorsLower == TEXT("centerright") || AnchorsLower == TEXT("right")) Anchors = FAnchors(1.f, 0.5f, 1.f, 0.5f);
					else if (AnchorsLower == TEXT("bottomleft")) Anchors = FAnchors(0.f, 1.f, 0.f, 1.f);
					else if (AnchorsLower == TEXT("bottomcenter") || AnchorsLower == TEXT("bottom")) Anchors = FAnchors(0.5f, 1.f, 0.5f, 1.f);
					else if (AnchorsLower == TEXT("bottomright")) Anchors = FAnchors(1.f, 1.f, 1.f, 1.f);
					else if (AnchorsLower == TEXT("stretch") || AnchorsLower == TEXT("fill")) Anchors = FAnchors(0.f, 0.f, 1.f, 1.f);
					CanvasSlot->SetAnchors(Anchors);
				}

				// Position
				if (!SlotDef.Position.IsEmpty())
				{
					TArray<FString> Parts;
					SlotDef.Position.ParseIntoArray(Parts, TEXT(","));
					if (Parts.Num() >= 2)
					{
						CanvasSlot->SetPosition(FVector2D(FCString::Atof(*Parts[0].TrimStartAndEnd()), FCString::Atof(*Parts[1].TrimStartAndEnd())));
					}
				}

				// Size
				if (!SlotDef.Size.IsEmpty())
				{
					TArray<FString> Parts;
					SlotDef.Size.ParseIntoArray(Parts, TEXT(","));
					if (Parts.Num() >= 2)
					{
						CanvasSlot->SetSize(FVector2D(FCString::Atof(*Parts[0].TrimStartAndEnd()), FCString::Atof(*Parts[1].TrimStartAndEnd())));
					}
				}

				// Alignment
				if (!SlotDef.Alignment.IsEmpty())
				{
					TArray<FString> Parts;
					SlotDef.Alignment.ParseIntoArray(Parts, TEXT(","));
					if (Parts.Num() >= 2)
					{
						CanvasSlot->SetAlignment(FVector2D(FCString::Atof(*Parts[0].TrimStartAndEnd()), FCString::Atof(*Parts[1].TrimStartAndEnd())));
					}
				}

				CanvasSlot->SetAutoSize(SlotDef.bAutoSize);
			}
			// Vertical Box Slot
			else if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(Slot))
			{
				// Horizontal Alignment
				if (!SlotDef.HorizontalAlignment.IsEmpty())
				{
					FString AlignLower = SlotDef.HorizontalAlignment.ToLower();
					if (AlignLower == TEXT("left")) VBoxSlot->SetHorizontalAlignment(HAlign_Left);
					else if (AlignLower == TEXT("center")) VBoxSlot->SetHorizontalAlignment(HAlign_Center);
					else if (AlignLower == TEXT("right")) VBoxSlot->SetHorizontalAlignment(HAlign_Right);
					else if (AlignLower == TEXT("fill")) VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
				}
				// Vertical Alignment
				if (!SlotDef.VerticalAlignment.IsEmpty())
				{
					FString AlignLower = SlotDef.VerticalAlignment.ToLower();
					if (AlignLower == TEXT("top")) VBoxSlot->SetVerticalAlignment(VAlign_Top);
					else if (AlignLower == TEXT("center")) VBoxSlot->SetVerticalAlignment(VAlign_Center);
					else if (AlignLower == TEXT("bottom")) VBoxSlot->SetVerticalAlignment(VAlign_Bottom);
					else if (AlignLower == TEXT("fill")) VBoxSlot->SetVerticalAlignment(VAlign_Fill);
				}
				// Size Rule
				if (!SlotDef.SizeRule.IsEmpty())
				{
					FString SizeLower = SlotDef.SizeRule.ToLower();
					if (SizeLower == TEXT("auto"))
						VBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
					else if (SizeLower == TEXT("fill"))
						VBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				}
				// Padding
				if (!SlotDef.Padding.IsEmpty())
				{
					TArray<FString> Parts;
					SlotDef.Padding.ParseIntoArray(Parts, TEXT(","));
					if (Parts.Num() == 1)
					{
						float P = FCString::Atof(*Parts[0].TrimStartAndEnd());
						VBoxSlot->SetPadding(FMargin(P));
					}
					else if (Parts.Num() >= 4)
					{
						VBoxSlot->SetPadding(FMargin(
							FCString::Atof(*Parts[0].TrimStartAndEnd()),
							FCString::Atof(*Parts[1].TrimStartAndEnd()),
							FCString::Atof(*Parts[2].TrimStartAndEnd()),
							FCString::Atof(*Parts[3].TrimStartAndEnd())));
					}
				}
			}
			// Horizontal Box Slot
			else if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(Slot))
			{
				// Horizontal Alignment
				if (!SlotDef.HorizontalAlignment.IsEmpty())
				{
					FString AlignLower = SlotDef.HorizontalAlignment.ToLower();
					if (AlignLower == TEXT("left")) HBoxSlot->SetHorizontalAlignment(HAlign_Left);
					else if (AlignLower == TEXT("center")) HBoxSlot->SetHorizontalAlignment(HAlign_Center);
					else if (AlignLower == TEXT("right")) HBoxSlot->SetHorizontalAlignment(HAlign_Right);
					else if (AlignLower == TEXT("fill")) HBoxSlot->SetHorizontalAlignment(HAlign_Fill);
				}
				// Vertical Alignment
				if (!SlotDef.VerticalAlignment.IsEmpty())
				{
					FString AlignLower = SlotDef.VerticalAlignment.ToLower();
					if (AlignLower == TEXT("top")) HBoxSlot->SetVerticalAlignment(VAlign_Top);
					else if (AlignLower == TEXT("center")) HBoxSlot->SetVerticalAlignment(VAlign_Center);
					else if (AlignLower == TEXT("bottom")) HBoxSlot->SetVerticalAlignment(VAlign_Bottom);
					else if (AlignLower == TEXT("fill")) HBoxSlot->SetVerticalAlignment(VAlign_Fill);
				}
				// Size Rule
				if (!SlotDef.SizeRule.IsEmpty())
				{
					FString SizeLower = SlotDef.SizeRule.ToLower();
					if (SizeLower == TEXT("auto"))
						HBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
					else if (SizeLower == TEXT("fill"))
						HBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				}
				// Padding
				if (!SlotDef.Padding.IsEmpty())
				{
					TArray<FString> Parts;
					SlotDef.Padding.ParseIntoArray(Parts, TEXT(","));
					if (Parts.Num() == 1)
					{
						float P = FCString::Atof(*Parts[0].TrimStartAndEnd());
						HBoxSlot->SetPadding(FMargin(P));
					}
					else if (Parts.Num() >= 4)
					{
						HBoxSlot->SetPadding(FMargin(
							FCString::Atof(*Parts[0].TrimStartAndEnd()),
							FCString::Atof(*Parts[1].TrimStartAndEnd()),
							FCString::Atof(*Parts[2].TrimStartAndEnd()),
							FCString::Atof(*Parts[3].TrimStartAndEnd())));
					}
				}
			}
			// Overlay Slot
			else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Slot))
			{
				if (!SlotDef.HorizontalAlignment.IsEmpty())
				{
					FString AlignLower = SlotDef.HorizontalAlignment.ToLower();
					if (AlignLower == TEXT("left")) OverlaySlot->SetHorizontalAlignment(HAlign_Left);
					else if (AlignLower == TEXT("center")) OverlaySlot->SetHorizontalAlignment(HAlign_Center);
					else if (AlignLower == TEXT("right")) OverlaySlot->SetHorizontalAlignment(HAlign_Right);
					else if (AlignLower == TEXT("fill")) OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
				}
				if (!SlotDef.VerticalAlignment.IsEmpty())
				{
					FString AlignLower = SlotDef.VerticalAlignment.ToLower();
					if (AlignLower == TEXT("top")) OverlaySlot->SetVerticalAlignment(VAlign_Top);
					else if (AlignLower == TEXT("center")) OverlaySlot->SetVerticalAlignment(VAlign_Center);
					else if (AlignLower == TEXT("bottom")) OverlaySlot->SetVerticalAlignment(VAlign_Bottom);
					else if (AlignLower == TEXT("fill")) OverlaySlot->SetVerticalAlignment(VAlign_Fill);
				}
				if (!SlotDef.Padding.IsEmpty())
				{
					TArray<FString> Parts;
					SlotDef.Padding.ParseIntoArray(Parts, TEXT(","));
					if (Parts.Num() == 1)
					{
						float P = FCString::Atof(*Parts[0].TrimStartAndEnd());
						OverlaySlot->SetPadding(FMargin(P));
					}
					else if (Parts.Num() >= 4)
					{
						OverlaySlot->SetPadding(FMargin(
							FCString::Atof(*Parts[0].TrimStartAndEnd()),
							FCString::Atof(*Parts[1].TrimStartAndEnd()),
							FCString::Atof(*Parts[2].TrimStartAndEnd()),
							FCString::Atof(*Parts[3].TrimStartAndEnd())));
					}
				}
			}
		};

		// v4.10: Helper to emit formatted warnings to Result.Warnings
		auto AddFormattedWarning = [&Result](const FString& Code, const FString& ContextPath, const FString& Message, const FString& SuggestedFix = TEXT(""))
		{
			// Replace newlines with semicolons for single-line output
			FString SafeMessage = Message.Replace(TEXT("\r\n"), TEXT("; ")).Replace(TEXT("\n"), TEXT("; "));
			Result.Warnings.Add(FString::Printf(TEXT("%s | %s | %s | %s"), *Code, *ContextPath, *SafeMessage, *SuggestedFix));
		};

		// v4.10: Helper to set enum value from string (handles both FEnumProperty and TEnumAsByte)
		auto SetEnumValueFromString = [&AddFormattedWarning](FProperty* Property, void* ContainerPtr, const FString& EnumValueName, const FString& ContextPath) -> bool
		{
			void* ValuePtr = Property->ContainerPtrToValuePtr<void>(ContainerPtr);

			// Try FEnumProperty first (modern enum type)
			if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
			{
				UEnum* Enum = EnumProp->GetEnum();
				if (Enum)
				{
					int64 EnumValue = Enum->GetValueByNameString(EnumValueName, EGetByNameFlags::None);
					if (EnumValue != INDEX_NONE)
					{
						FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
						if (UnderlyingProp)
						{
							UnderlyingProp->SetIntPropertyValue(ValuePtr, EnumValue);
							return true;
						}
					}
					AddFormattedWarning(TEXT("W_ENUM_VALUE_INVALID"), ContextPath,
						FString::Printf(TEXT("Enum value '%s' not found in %s"), *EnumValueName, *Enum->GetName()),
						TEXT("Check enum value spelling"));
				}
				return false;
			}

			// Try FByteProperty (TEnumAsByte<>)
			if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
			{
				UEnum* Enum = ByteProp->GetIntPropertyEnum();
				if (Enum)
				{
					int64 EnumValue = Enum->GetValueByNameString(EnumValueName, EGetByNameFlags::None);
					if (EnumValue != INDEX_NONE)
					{
						ByteProp->SetIntPropertyValue(ValuePtr, EnumValue);
						return true;
					}
					AddFormattedWarning(TEXT("W_ENUM_VALUE_INVALID"), ContextPath,
						FString::Printf(TEXT("Enum value '%s' not found in %s"), *EnumValueName, *Enum->GetName()),
						TEXT("Check enum value spelling"));
				}
				return false;
			}

			return false;
		};

		// v4.10: Helper: Configure widget properties with full type support
		auto ConfigureWidgetProperties = [&Result, &Definition, &AddFormattedWarning, &SetEnumValueFromString](UWidget* Widget, const FManifestWidgetNodeDefinition& NodeDef, const FString& WidgetContextPath)
		{
			// Set text for TextBlock
			if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
			{
				if (!NodeDef.Text.IsEmpty())
				{
					TextBlock->SetText(FText::FromString(NodeDef.Text));
				}
			}
			// Set text for RichTextBlock
			else if (URichTextBlock* RichText = Cast<URichTextBlock>(Widget))
			{
				if (!NodeDef.Text.IsEmpty())
				{
					RichText->SetText(FText::FromString(NodeDef.Text));
				}
			}

			// Apply custom properties via reflection
			for (const auto& Prop : NodeDef.Properties)
			{
				const FString& FullKey = Prop.Key;
				const FString& Value = Prop.Value;
				FString PropContextPath = FString::Printf(TEXT("%s.%s"), *WidgetContextPath, *FullKey);

				// v4.10: Check for 1-level dotted property (e.g., Font.Size, Brush.ResourceObject)
				int32 DotIndex = INDEX_NONE;
				if (FullKey.FindChar(TEXT('.'), DotIndex))
				{
					FString RootKey = FullKey.Left(DotIndex);
					FString SubKey = FullKey.Mid(DotIndex + 1);

					// Check for nested dots (not supported)
					if (SubKey.Contains(TEXT(".")))
					{
						AddFormattedWarning(TEXT("W_NESTED_TOO_DEEP"), PropContextPath,
							FString::Printf(TEXT("Only 1-level dotted properties supported, got '%s'"), *FullKey),
							TEXT("Use direct property path like Font.Size"));
						continue;
					}

					// Find root property
					FProperty* RootProp = Widget->GetClass()->FindPropertyByName(FName(*RootKey));
					if (!RootProp)
					{
						AddFormattedWarning(TEXT("W_PROPERTY_NOT_FOUND"), PropContextPath,
							FString::Printf(TEXT("Root property '%s' not found on %s"), *RootKey, *Widget->GetClass()->GetName()),
							TEXT(""));
						continue;
					}

					// Root must be a struct
					FStructProperty* StructProp = CastField<FStructProperty>(RootProp);
					if (!StructProp)
					{
						AddFormattedWarning(TEXT("W_UNSUPPORTED_TYPE"), PropContextPath,
							FString::Printf(TEXT("Property '%s' is not a struct, cannot access sub-property"), *RootKey),
							TEXT(""));
						continue;
					}

					void* StructPtr = StructProp->ContainerPtrToValuePtr<void>(Widget);

					// v4.10: Special handling for FSlateBrush (field restrictions)
					if (StructProp->Struct && StructProp->Struct->GetFName() == FName(TEXT("SlateBrush")))
					{
						// Only allow: ResourceObject, ImageSize, TintColor
						static const TSet<FString> AllowedBrushFields = { TEXT("ResourceObject"), TEXT("ImageSize"), TEXT("TintColor") };
						if (!AllowedBrushFields.Contains(SubKey))
						{
							AddFormattedWarning(TEXT("W_BRUSH_FIELD_BLOCKED"), PropContextPath,
								FString::Printf(TEXT("FSlateBrush field '%s' not writable via reflection"), *SubKey),
								TEXT("Only ResourceObject, ImageSize, TintColor supported"));
							continue;
						}

						FSlateBrush* Brush = static_cast<FSlateBrush*>(StructPtr);

						if (SubKey == TEXT("ResourceObject"))
						{
							// Load texture/resource
							UObject* Resource = LoadObject<UObject>(nullptr, *Value);
							if (Resource)
							{
								// Block MediaTexture (no MediaAssets dependency)
								FString ClassName = Resource->GetClass()->GetName();
								if (ClassName == TEXT("MediaTexture"))
								{
									AddFormattedWarning(TEXT("W_TEXTURE_TYPE_BLOCKED"), PropContextPath,
										TEXT("MediaTexture not supported (no MediaAssets dependency)"),
										TEXT("Use UTexture2D or SlateTextureAtlas"));
									continue;
								}

								// Allow: UTexture, SlateTextureAtlasInterface, SlateBrushAsset
								bool bValidResource = Resource->IsA<UTexture>() ||
									Resource->GetClass()->ImplementsInterface(USlateTextureAtlasInterface::StaticClass()) ||
									Resource->IsA<USlateBrushAsset>();

								if (bValidResource)
								{
									Brush->SetResourceObject(Resource);
								}
								else
								{
									AddFormattedWarning(TEXT("W_TEXTURE_TYPE_BLOCKED"), PropContextPath,
										FString::Printf(TEXT("Resource type '%s' not valid for FSlateBrush"), *ClassName),
										TEXT("Use UTexture2D, SlateTextureAtlas, or SlateBrushAsset"));
								}
							}
							continue;
						}
						else if (SubKey == TEXT("ImageSize"))
						{
							// Parse FVector2D - shim "64, 64" to "X=64 Y=64"
							FString ParseValue = Value;
							if (Value.Contains(TEXT(",")) && !Value.Contains(TEXT("=")))
							{
								TArray<FString> Parts;
								Value.ParseIntoArray(Parts, TEXT(","));
								if (Parts.Num() >= 2)
								{
									ParseValue = FString::Printf(TEXT("X=%s Y=%s"),
										*Parts[0].TrimStartAndEnd(), *Parts[1].TrimStartAndEnd());
								}
							}
							FVector2D Size;
							if (Size.InitFromString(ParseValue))
							{
								Brush->ImageSize = Size;
							}
							else
							{
								AddFormattedWarning(TEXT("W_STRUCT_INIT_FAILED"), PropContextPath,
									FString::Printf(TEXT("FVector2D init failed for '%s'"), *Value),
									TEXT("Use format: 64, 64 or X=64 Y=64"));
							}
							continue;
						}
						else if (SubKey == TEXT("TintColor"))
						{
							// Parse FSlateColor from FLinearColor string
							FLinearColor Color;
							if (Color.InitFromString(Value))
							{
								Brush->TintColor = FSlateColor(Color);
							}
							else
							{
								AddFormattedWarning(TEXT("W_STRUCT_INIT_FAILED"), PropContextPath,
									FString::Printf(TEXT("FLinearColor init failed for '%s'"), *Value),
									TEXT("Use format: R=1.0 G=0.5 B=0.0 A=1.0 or 1.0, 0.5, 0.0, 1.0"));
							}
							continue;
						}
					}

					// Find inner property
					FProperty* InnerProp = StructProp->Struct->FindPropertyByName(FName(*SubKey));
					if (!InnerProp)
					{
						AddFormattedWarning(TEXT("W_PROPERTY_NOT_FOUND"), PropContextPath,
							FString::Printf(TEXT("Sub-property '%s' not found in struct '%s'"), *SubKey, *StructProp->Struct->GetName()),
							TEXT(""));
						continue;
					}

					void* InnerValuePtr = InnerProp->ContainerPtrToValuePtr<void>(StructPtr);

					// Handle inner property types
					if (FBoolProperty* BoolProp = CastField<FBoolProperty>(InnerProp))
					{
						BoolProp->SetPropertyValue(InnerValuePtr, Value.ToBool() || Value.Equals(TEXT("true"), ESearchCase::IgnoreCase));
					}
					else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(InnerProp))
					{
						FloatProp->SetPropertyValue(InnerValuePtr, FCString::Atof(*Value));
					}
					else if (FIntProperty* IntProp = CastField<FIntProperty>(InnerProp))
					{
						IntProp->SetPropertyValue(InnerValuePtr, FCString::Atoi(*Value));
					}
					else if (FStrProperty* StrProp = CastField<FStrProperty>(InnerProp))
					{
						StrProp->SetPropertyValue(InnerValuePtr, Value);
					}
					else if (FNameProperty* NameProp = CastField<FNameProperty>(InnerProp))
					{
						NameProp->SetPropertyValue(InnerValuePtr, FName(*Value));
					}
					else if (!SetEnumValueFromString(InnerProp, StructPtr, Value, PropContextPath))
					{
						// Check for nested struct types
						if (FStructProperty* InnerStructProp = CastField<FStructProperty>(InnerProp))
						{
							FName StructName = InnerStructProp->Struct->GetFName();

							if (StructName == FName(TEXT("LinearColor")))
							{
								FLinearColor* ColorPtr = static_cast<FLinearColor*>(InnerValuePtr);
								if (!ColorPtr->InitFromString(Value))
								{
									AddFormattedWarning(TEXT("W_STRUCT_INIT_FAILED"), PropContextPath,
										FString::Printf(TEXT("FLinearColor init failed for '%s'"), *Value),
										TEXT("Use format: R=1.0 G=0.5 B=0.0 A=1.0"));
								}
							}
							else if (StructName == FName(TEXT("Vector2D")))
							{
								FString ParseValue = Value;
								if (Value.Contains(TEXT(",")) && !Value.Contains(TEXT("=")))
								{
									TArray<FString> Parts;
									Value.ParseIntoArray(Parts, TEXT(","));
									if (Parts.Num() >= 2)
									{
										ParseValue = FString::Printf(TEXT("X=%s Y=%s"),
											*Parts[0].TrimStartAndEnd(), *Parts[1].TrimStartAndEnd());
									}
								}
								FVector2D* VecPtr = static_cast<FVector2D*>(InnerValuePtr);
								if (!VecPtr->InitFromString(ParseValue))
								{
									AddFormattedWarning(TEXT("W_STRUCT_INIT_FAILED"), PropContextPath,
										FString::Printf(TEXT("FVector2D init failed for '%s'"), *Value),
										TEXT("Use format: 64, 64 or X=64 Y=64"));
								}
							}
							else if (StructName == FName(TEXT("SlateColor")))
							{
								FLinearColor Color;
								if (Color.InitFromString(Value))
								{
									FSlateColor* SlateColorPtr = static_cast<FSlateColor*>(InnerValuePtr);
									*SlateColorPtr = FSlateColor(Color);
								}
								else
								{
									AddFormattedWarning(TEXT("W_STRUCT_INIT_FAILED"), PropContextPath,
										FString::Printf(TEXT("FSlateColor init failed for '%s'"), *Value),
										TEXT("Use format: R=1.0 G=0.5 B=0.0 A=1.0"));
								}
							}
							else
							{
								AddFormattedWarning(TEXT("W_UNSUPPORTED_TYPE"), PropContextPath,
									FString::Printf(TEXT("Nested struct type '%s' not supported"), *StructName.ToString()),
									TEXT(""));
							}
						}
						else
						{
							AddFormattedWarning(TEXT("W_UNSUPPORTED_TYPE"), PropContextPath,
								FString::Printf(TEXT("Inner property type not handled for '%s'"), *InnerProp->GetClass()->GetName()),
								TEXT(""));
						}
					}
					continue;
				}

				// Non-dotted property (original flat handling)
				FProperty* Property = Widget->GetClass()->FindPropertyByName(FName(*FullKey));
				if (!Property)
				{
					// Try with common variations
					Property = Widget->GetClass()->FindPropertyByName(FName(*FString::Printf(TEXT("b%s"), *FullKey)));
				}
				if (!Property)
				{
					AddFormattedWarning(TEXT("W_PROPERTY_NOT_FOUND"), PropContextPath,
						FString::Printf(TEXT("Property '%s' not found on %s"), *FullKey, *Widget->GetClass()->GetName()),
						TEXT(""));
					continue;
				}

				void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Widget);

				if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
				{
					BoolProp->SetPropertyValue(ValuePtr, Value.ToBool() || Value.Equals(TEXT("true"), ESearchCase::IgnoreCase));
				}
				else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
				{
					FloatProp->SetPropertyValue(ValuePtr, FCString::Atof(*Value));
				}
				else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
				{
					IntProp->SetPropertyValue(ValuePtr, FCString::Atoi(*Value));
				}
				else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
				{
					StrProp->SetPropertyValue(ValuePtr, Value);
				}
				else if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
				{
					TextProp->SetPropertyValue(ValuePtr, FText::FromString(Value));
				}
				else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
				{
					NameProp->SetPropertyValue(ValuePtr, FName(*Value));
				}
				else if (!SetEnumValueFromString(Property, Widget, Value, PropContextPath))
				{
					// Check for struct property at root level
					if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
					{
						FName StructName = StructProp->Struct->GetFName();

						if (StructName == FName(TEXT("LinearColor")))
						{
							FLinearColor* ColorPtr = static_cast<FLinearColor*>(ValuePtr);
							if (!ColorPtr->InitFromString(Value))
							{
								AddFormattedWarning(TEXT("W_STRUCT_INIT_FAILED"), PropContextPath,
									FString::Printf(TEXT("FLinearColor init failed for '%s'"), *Value),
									TEXT("Use format: R=1.0 G=0.5 B=0.0 A=1.0"));
							}
						}
						else if (StructName == FName(TEXT("Vector2D")))
						{
							FString ParseValue = Value;
							if (Value.Contains(TEXT(",")) && !Value.Contains(TEXT("=")))
							{
								TArray<FString> Parts;
								Value.ParseIntoArray(Parts, TEXT(","));
								if (Parts.Num() >= 2)
								{
									ParseValue = FString::Printf(TEXT("X=%s Y=%s"),
										*Parts[0].TrimStartAndEnd(), *Parts[1].TrimStartAndEnd());
								}
							}
							FVector2D* VecPtr = static_cast<FVector2D*>(ValuePtr);
							if (!VecPtr->InitFromString(ParseValue))
							{
								AddFormattedWarning(TEXT("W_STRUCT_INIT_FAILED"), PropContextPath,
									FString::Printf(TEXT("FVector2D init failed for '%s'"), *Value),
									TEXT("Use format: 64, 64 or X=64 Y=64"));
							}
						}
						else if (StructName == FName(TEXT("SlateColor")))
						{
							FLinearColor Color;
							if (Color.InitFromString(Value))
							{
								FSlateColor* SlateColorPtr = static_cast<FSlateColor*>(ValuePtr);
								*SlateColorPtr = FSlateColor(Color);
							}
							else
							{
								AddFormattedWarning(TEXT("W_STRUCT_INIT_FAILED"), PropContextPath,
									FString::Printf(TEXT("FSlateColor init failed for '%s'"), *Value),
									TEXT("Use format: R=1.0 G=0.5 B=0.0 A=1.0"));
							}
						}
						else
						{
							AddFormattedWarning(TEXT("W_UNSUPPORTED_TYPE"), PropContextPath,
								FString::Printf(TEXT("Struct type '%s' not supported at root level"), *StructName.ToString()),
								TEXT(""));
						}
					}
					else
					{
						AddFormattedWarning(TEXT("W_UNSUPPORTED_TYPE"), PropContextPath,
							FString::Printf(TEXT("Property type '%s' not handled"), *Property->GetClass()->GetName()),
							TEXT(""));
					}
				}
			}
		};

		// PASS 1: Create all widgets
		TMap<FString, UWidget*> WidgetMap;
		for (const FManifestWidgetNodeDefinition& NodeDef : Definition.WidgetTree.Widgets)
		{
			FName WidgetName = FName(*NodeDef.Id);
			UWidget* Widget = CreateWidgetByType(NodeDef.Type, WidgetName);

			if (Widget)
			{
				WidgetMap.Add(NodeDef.Id, Widget);

				// Configure properties
				FString WidgetContextPath = FString::Printf(TEXT("widget_blueprints[%s].widget_tree.widgets[%s]"),
					*Definition.Name, *NodeDef.Id);
				ConfigureWidgetProperties(Widget, NodeDef, WidgetContextPath);

				// Mark as variable if requested (for BindWidget)
				if (NodeDef.bIsVariable)
				{
					Widget->bIsVariable = true;
				}

				LogGeneration(FString::Printf(TEXT("  Created widget: %s (%s)%s"),
					*NodeDef.Id, *NodeDef.Type, NodeDef.bIsVariable ? TEXT(" [Variable]") : TEXT("")));
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("  [WARNING] Unknown widget type: %s"), *NodeDef.Type));
			}
		}

		// PASS 2: Build hierarchy and configure slots
		for (const FManifestWidgetNodeDefinition& NodeDef : Definition.WidgetTree.Widgets)
		{
			UWidget** WidgetPtr = WidgetMap.Find(NodeDef.Id);
			if (!WidgetPtr) continue;

			UPanelWidget* PanelWidget = Cast<UPanelWidget>(*WidgetPtr);
			if (!PanelWidget) continue;

			for (const FString& ChildId : NodeDef.Children)
			{
				UWidget** ChildPtr = WidgetMap.Find(ChildId);
				if (!ChildPtr) continue;

				// Add child to panel
				UPanelSlot* Slot = PanelWidget->AddChild(*ChildPtr);
				if (Slot)
				{
					// Find child definition for slot config
					const FManifestWidgetNodeDefinition* ChildDef = Definition.WidgetTree.Widgets.FindByPredicate(
						[&ChildId](const FManifestWidgetNodeDefinition& Def) { return Def.Id == ChildId; });

					if (ChildDef)
					{
						ConfigureSlot(*ChildPtr, PanelWidget, ChildDef->Slot);
					}
				}
			}
		}

		// PASS 3: Set root widget
		if (!Definition.WidgetTree.RootWidget.IsEmpty())
		{
			UWidget** RootPtr = WidgetMap.Find(Definition.WidgetTree.RootWidget);
			if (RootPtr && *RootPtr)
			{
				Tree->RootWidget = *RootPtr;
				LogGeneration(FString::Printf(TEXT("  Set root widget: %s"), *Definition.WidgetTree.RootWidget));
			}
		}
		else if (Definition.WidgetTree.Widgets.Num() > 0)
		{
			// Use first widget as root if not specified
			UWidget** FirstPtr = WidgetMap.Find(Definition.WidgetTree.Widgets[0].Id);
			if (FirstPtr && *FirstPtr)
			{
				Tree->RootWidget = *FirstPtr;
				LogGeneration(FString::Printf(TEXT("  Set root widget (auto): %s"), *Definition.WidgetTree.Widgets[0].Id));
			}
		}

		LogGeneration(FString::Printf(TEXT("  Built widget tree with %d widgets"), Definition.WidgetTree.Widgets.Num()));
	}

	// v2.2.0: Generate event graph if specified
	if (!Definition.EventGraphName.IsEmpty() && ManifestData)
	{
		// v2.6.7: Clear any previous missing dependencies before generation
		FEventGraphGenerator::ClearMissingDependencies();

		const FManifestEventGraphDefinition* GraphDef = ManifestData->FindEventGraphByName(Definition.EventGraphName);
		if (GraphDef)
		{
			bool bEventGraphSuccess = FEventGraphGenerator::GenerateEventGraph(WidgetBP, *GraphDef, TEXT(""));

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

			// v4.16: Event graph failure propagation (P2 - Caller Propagation)
			if (!bEventGraphSuccess)
			{
				UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Event graph generation failed"), *Definition.Name);
				LogGeneration(FString::Printf(TEXT("  FAILED: Event graph '%s' generation failed - see errors above"), *Definition.EventGraphName));

				Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
					TEXT("Event graph generation failed"));
				Result.AssetPath = AssetPath;
				Result.GeneratorId = TEXT("WidgetBlueprint");
				Result.DetermineCategory();
				return Result;
			}

			LogGeneration(FString::Printf(TEXT("  Applied event graph: %s"), *Definition.EventGraphName));
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  Event graph not found: %s"), *Definition.EventGraphName));
		}
	}

	// v4.16: Compile blueprint with validation (D-006, Contract 10)
	FCompilerResultsLog CompileLog;
	FKismetEditorUtilities::CompileBlueprint(WidgetBP, EBlueprintCompileOptions::None, &CompileLog);

	// v4.20.11: Re-apply node positions after compilation (compilation may reset positions)
	FEventGraphGenerator::ReapplyNodePositions(WidgetBP);

	// v4.16: Check compile result - abort save if errors (P3, P4)
	if (CompileLog.NumErrors > 0)
	{
		TArray<FString> ErrorMessages;
		for (const TSharedRef<FTokenizedMessage>& Msg : CompileLog.Messages)
		{
			if (Msg->GetSeverity() == EMessageSeverity::Error)
			{
				ErrorMessages.Add(Msg->ToText().ToString());
			}
		}

		UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Blueprint compilation failed with %d errors: %s"),
			*Definition.Name, CompileLog.NumErrors, *FString::Join(ErrorMessages, TEXT("; ")));

		LogGeneration(FString::Printf(TEXT("  COMPILE FAILED with %d errors:"), CompileLog.NumErrors));
		for (const FString& Err : ErrorMessages)
		{
			LogGeneration(FString::Printf(TEXT("    - %s"), *Err));
		}

		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Compile failed: %s"), *FString::Join(ErrorMessages, TEXT("; "))));
		Result.AssetPath = AssetPath;
		Result.GeneratorId = TEXT("WidgetBlueprint");
		Result.DetermineCategory();
		return Result;
	}

	// Mark dirty and register
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(WidgetBP);

	// Save package (only reached if compile succeeded)
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, WidgetBP, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Widget Blueprint: %s (with %d variables)"), *Definition.Name, Definition.Variables.Num()));

	// v3.0: Store metadata for regeneration tracking
	StoreBlueprintMetadata(WidgetBP, TEXT("WidgetBlueprint"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("WidgetBlueprint");
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// FComponentBlueprintGenerator Implementation
// v4.19: ActorComponent blueprint generation with event dispatchers, functions, tick config
// ============================================================================

FGenerationResult FComponentBlueprintGenerator::Generate(
	const FManifestComponentBlueprintDefinition& Definition,
	const FString& ProjectRoot)
{
	// v4.19: Set global project root for enum lookups
	GCurrentProjectRoot = ProjectRoot;

	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Components") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	// Validate against manifest whitelist
	if (ValidateAgainstManifest(Definition.Name, TEXT("Component Blueprint"), Result))
	{
		return Result;
	}

	// Check existence with metadata-aware logic
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Component Blueprint"), Definition.ComputeHash(), Result))
	{
		return Result;
	}

	// Reserved suffixes that cannot be used for function names
	static const TArray<FString> ReservedSuffixes = { TEXT("_Implementation"), TEXT("_Validate"), TEXT("_C"), TEXT("__") };

	// Native parent class whitelist
	UClass* ParentClass = nullptr;
	if (Definition.ParentClass == TEXT("ActorComponent") || Definition.ParentClass.IsEmpty())
	{
		ParentClass = UActorComponent::StaticClass();
	}
	else if (Definition.ParentClass == TEXT("SceneComponent"))
	{
		ParentClass = USceneComponent::StaticClass();
	}
	else if (Definition.ParentClass == TEXT("PrimitiveComponent"))
	{
		ParentClass = UPrimitiveComponent::StaticClass();
	}
	else if (Definition.ParentClass == TEXT("AudioComponent"))
	{
		ParentClass = UAudioComponent::StaticClass();
	}
	else if (Definition.ParentClass.StartsWith(TEXT("/Script/")) || Definition.ParentClass.StartsWith(TEXT("/Game/")))
	{
		// Full path - strict resolve
		ParentClass = FindParentClass(Definition.ParentClass);
		if (!ParentClass)
		{
			UE_LOG(LogTemp, Error, TEXT("COMP_BP[%s] PH1 CreateAsset FAIL - [E_PARENT_CLASS_RESOLVE] %s"), *Definition.Name, *Definition.ParentClass);
			Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("[E_PARENT_CLASS_RESOLVE] %s"), *Definition.ParentClass));
			Result.GeneratorId = TEXT("ComponentBlueprint");
			Result.DetermineCategory();
			return Result;
		}
	}
	else
	{
		// Short name not in whitelist
		UE_LOG(LogTemp, Error, TEXT("COMP_BP[%s] PH1 CreateAsset FAIL - [E_PARENT_CLASS_RESOLVE] %s not in whitelist"), *Definition.Name, *Definition.ParentClass);
		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("[E_PARENT_CLASS_RESOLVE] %s not in whitelist"), *Definition.ParentClass));
		Result.GeneratorId = TEXT("ComponentBlueprint");
		Result.DetermineCategory();
		return Result;
	}

	// ========== PHASE 1: Create Blueprint with UActorComponent Parent ==========
	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = ParentClass;

	FString PackagePath = AssetPath;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("COMP_BP[%s] PH1 CreateAsset FAIL - Package creation failed"), *Definition.Name);
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
		UBlueprint::StaticClass(),
		Package,
		FName(*Definition.Name),
		RF_Public | RF_Standalone,
		nullptr,
		GWarn
	));

	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("COMP_BP[%s] PH1 CreateAsset FAIL - Blueprint creation failed"), *Definition.Name);
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Component Blueprint"));
	}

	UE_LOG(LogTemp, Log, TEXT("COMP_BP[%s] PH1 CreateAsset OK"), *Definition.Name);
	LogGeneration(FString::Printf(TEXT("Creating Component Blueprint: %s"), *Definition.Name));

	// ========== PHASE 2: Add Variables ==========
	TSet<FName> ExistingVarNames;
	for (const auto& Var : Blueprint->NewVariables) { ExistingVarNames.Add(Var.VarName); }

	for (const FManifestActorVariableDefinition& VarDef : Definition.Variables)
	{
		// Duplicate guard
		if (ExistingVarNames.Contains(FName(*VarDef.Name)))
		{
			UE_LOG(LogTemp, Error, TEXT("COMP_BP[%s] PH2 Variables FAIL - [E_DUPLICATE_VARIABLE] %s"), *Definition.Name, *VarDef.Name);
			Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("[E_DUPLICATE_VARIABLE] %s"), *VarDef.Name));
			Result.GeneratorId = TEXT("ComponentBlueprint");
			Result.DetermineCategory();
			return Result;
		}

		FEdGraphPinType PinType = GetPinTypeFromString(VarDef.Type);
		bool bSuccess = FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VarDef.Name), PinType);
		// Note: AddMemberVariable already calls MarkBlueprintAsStructurallyModified internally

		if (bSuccess)
		{
			int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, FName(*VarDef.Name));
			if (VarIndex != INDEX_NONE)
			{
				if (VarDef.bInstanceEditable)
					Blueprint->NewVariables[VarIndex].PropertyFlags |= CPF_Edit;
				if (VarDef.bReplicated)
					Blueprint->NewVariables[VarIndex].PropertyFlags |= CPF_Net;
				if (!VarDef.DefaultValue.IsEmpty())
					Blueprint->NewVariables[VarIndex].DefaultValue = VarDef.DefaultValue;
			}
			ExistingVarNames.Add(FName(*VarDef.Name));
			LogGeneration(FString::Printf(TEXT("  Added variable: %s (%s)"), *VarDef.Name, *VarDef.Type));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("COMP_BP[%s] PH2 Variables Added=%d OK"), *Definition.Name, Definition.Variables.Num());

	// ========== PHASE 3: Add Event Dispatchers with Parameters ==========
	TSet<FName> ExistingDispatcherNames;
	for (UEdGraph* Graph : Blueprint->DelegateSignatureGraphs) { ExistingDispatcherNames.Add(Graph->GetFName()); }

	for (const FManifestEventDispatcherDefinition& DispatcherDef : Definition.EventDispatchers)
	{
		// Duplicate guard
		if (ExistingDispatcherNames.Contains(FName(*DispatcherDef.Name)))
		{
			UE_LOG(LogTemp, Error, TEXT("COMP_BP[%s] PH3 Dispatchers FAIL - [E_DUPLICATE_DISPATCHER] %s"), *Definition.Name, *DispatcherDef.Name);
			Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("[E_DUPLICATE_DISPATCHER] %s"), *DispatcherDef.Name));
			Result.GeneratorId = TEXT("ComponentBlueprint");
			Result.DetermineCategory();
			return Result;
		}

		// Step 1: Add multicast delegate member variable
		FEdGraphPinType DelegateType;
		DelegateType.PinCategory = UEdGraphSchema_K2::PC_MCDelegate;
		bool bVarCreated = FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*DispatcherDef.Name), DelegateType);

		if (bVarCreated)
		{
			// Step 2: Create delegate signature graph
			UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(
				Blueprint,
				FName(*DispatcherDef.Name),
				UEdGraph::StaticClass(),
				UEdGraphSchema_K2::StaticClass()
			);

			NewGraph->bEditable = false;

			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			K2Schema->CreateDefaultNodesForGraph(*NewGraph);
			K2Schema->CreateFunctionGraphTerminators(*NewGraph, (UClass*)nullptr);
			K2Schema->AddExtraFunctionFlags(NewGraph, (FUNC_BlueprintCallable | FUNC_BlueprintEvent | FUNC_Public));
			K2Schema->MarkFunctionEntryAsEditable(NewGraph, true);  // CRITICAL: enables parameter addition

			// Step 3: Add to DelegateSignatureGraphs array
			Blueprint->DelegateSignatureGraphs.Add(NewGraph);

			// Step 4: Add parameters to entry node
			TArray<UK2Node_FunctionEntry*> EntryNodes;
			NewGraph->GetNodesOfClass<UK2Node_FunctionEntry>(EntryNodes);
			if (EntryNodes.Num() > 0)
			{
				UK2Node_FunctionEntry* EntryNode = EntryNodes[0];

				for (const FManifestDispatcherParam& Param : DispatcherDef.Parameters)
				{
					FEdGraphPinType ParamPinType = GetPinTypeFromString(Param.Type);
					EntryNode->CreateUserDefinedPin(
						FName(*Param.Name),
						ParamPinType,
						EGPD_Output  // Output from entry = input parameter
					);
				}
			}

			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
			ExistingDispatcherNames.Add(FName(*DispatcherDef.Name));
			LogGeneration(FString::Printf(TEXT("  Added event dispatcher: %s (%d params)"), *DispatcherDef.Name, DispatcherDef.Parameters.Num()));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("COMP_BP[%s] PH3 Dispatchers Added=%d OK"), *Definition.Name, Definition.EventDispatchers.Num());

	// ========== PHASE 4: Add Functions with Input/Output Pins ==========
	TSet<FName> ExistingFuncNames;
	for (UEdGraph* Graph : Blueprint->FunctionGraphs) { ExistingFuncNames.Add(Graph->GetFName()); }

	for (const FManifestFunctionDefinition& FuncDef : Definition.Functions)
	{
		// Duplicate guard
		if (ExistingFuncNames.Contains(FName(*FuncDef.Name)))
		{
			UE_LOG(LogTemp, Error, TEXT("COMP_BP[%s] PH4 Functions FAIL - [E_DUPLICATE_FUNCTION] %s"), *Definition.Name, *FuncDef.Name);
			Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("[E_DUPLICATE_FUNCTION] %s"), *FuncDef.Name));
			Result.GeneratorId = TEXT("ComponentBlueprint");
			Result.DetermineCategory();
			return Result;
		}

		// Reserved suffix guard
		for (const FString& Suffix : ReservedSuffixes)
		{
			if (FuncDef.Name.EndsWith(Suffix))
			{
				UE_LOG(LogTemp, Error, TEXT("COMP_BP[%s] PH4 Functions FAIL - [E_RESERVED_SUFFIX] %s uses reserved suffix %s"), *Definition.Name, *FuncDef.Name, *Suffix);
				Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
					FString::Printf(TEXT("[E_RESERVED_SUFFIX] %s uses reserved suffix %s"), *FuncDef.Name, *Suffix));
				Result.GeneratorId = TEXT("ComponentBlueprint");
				Result.DetermineCategory();
				return Result;
			}
		}

		// Step 1: Create function graph
		UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
			Blueprint,
			FName(*FuncDef.Name),
			UEdGraph::StaticClass(),
			UEdGraphSchema_K2::StaticClass()
		);

		FBlueprintEditorUtils::AddFunctionGraph(Blueprint, FuncGraph, true, static_cast<UFunction*>(nullptr));

		// Step 2: Get entry node
		TArray<UK2Node_FunctionEntry*> EntryNodes;
		FuncGraph->GetNodesOfClass<UK2Node_FunctionEntry>(EntryNodes);
		if (EntryNodes.Num() > 0)
		{
			UK2Node_FunctionEntry* EntryNode = EntryNodes[0];

			// Step 2a: Set pure flag BEFORE creating pins (affects pin layout)
			if (FuncDef.bPure)
			{
				EntryNode->AddExtraFlags(FUNC_BlueprintPure);
			}
			else
			{
				// Explicitly clear pure flag (deterministic)
				EntryNode->SetExtraFlags(EntryNode->GetExtraFlags() & ~FUNC_BlueprintPure);
			}

			// Step 2b: Add input parameters
			for (const FManifestFunctionParam& Param : FuncDef.InputParams)
			{
				FEdGraphPinType ParamPinType = GetPinTypeFromString(Param.Type);
				EntryNode->CreateUserDefinedPin(
					FName(*Param.Name),
					ParamPinType,
					EGPD_Output  // Output from entry = function input
				);
			}
		}

		// Step 3: Get result node and add output parameters (return values)
		TArray<UK2Node_FunctionResult*> ResultNodes;
		FuncGraph->GetNodesOfClass<UK2Node_FunctionResult>(ResultNodes);
		if (ResultNodes.Num() > 0 && FuncDef.OutputParams.Num() > 0)
		{
			UK2Node_FunctionResult* ResultNode = ResultNodes[0];

			for (const FManifestFunctionParam& Param : FuncDef.OutputParams)
			{
				FEdGraphPinType ParamPinType = GetPinTypeFromString(Param.Type);
				ResultNode->CreateUserDefinedPin(
					FName(*Param.Name),
					ParamPinType,
					EGPD_Input  // Input to result = function output
				);
			}
		}

		ExistingFuncNames.Add(FName(*FuncDef.Name));
		LogGeneration(FString::Printf(TEXT("  Added function: %s (%d inputs, %d outputs, pure=%s)"),
			*FuncDef.Name, FuncDef.InputParams.Num(), FuncDef.OutputParams.Num(), FuncDef.bPure ? TEXT("true") : TEXT("false")));
	}

	UE_LOG(LogTemp, Log, TEXT("COMP_BP[%s] PH4 Functions Added=%d OK"), *Definition.Name, Definition.Functions.Num());

	// ========== PHASE 5: Compile (MUST happen before CDO access) ==========
	FCompilerResultsLog CompileLog;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &CompileLog);

	if (CompileLog.NumErrors > 0)
	{
		TArray<FString> ErrorMessages;
		for (const TSharedRef<FTokenizedMessage>& Msg : CompileLog.Messages)
		{
			if (Msg->GetSeverity() == EMessageSeverity::Error)
			{
				ErrorMessages.Add(Msg->ToText().ToString());
			}
		}

		UE_LOG(LogTemp, Error, TEXT("COMP_BP[%s] PH5 Compile FAIL Errors=%d -> see CompileLog"), *Definition.Name, CompileLog.NumErrors);
		LogGeneration(FString::Printf(TEXT("  COMPILE FAILED with %d errors:"), CompileLog.NumErrors));
		for (const FString& Err : ErrorMessages)
		{
			LogGeneration(FString::Printf(TEXT("    - %s"), *Err));
		}

		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Compile failed: %s"), *FString::Join(ErrorMessages, TEXT("; "))));
		Result.AssetPath = AssetPath;
		Result.GeneratorId = TEXT("ComponentBlueprint");
		Result.DetermineCategory();
		return Result;
	}

	if (!Blueprint->GeneratedClass)
	{
		UE_LOG(LogTemp, Error, TEXT("COMP_BP[%s] PH5 Compile FAIL - no GeneratedClass"), *Definition.Name);
		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Compile failed - no GeneratedClass"));
		Result.GeneratorId = TEXT("ComponentBlueprint");
		Result.DetermineCategory();
		return Result;
	}

	UE_LOG(LogTemp, Log, TEXT("COMP_BP[%s] PH5 Compile OK Errors=0 Warnings=%d"), *Definition.Name, CompileLog.NumWarnings);

	// ========== PHASE 6: Configure Tick on CDO (AFTER compile) ==========
	UActorComponent* ComponentCDO = Cast<UActorComponent>(Blueprint->GeneratedClass->GetDefaultObject());
	if (!ComponentCDO)
	{
		UE_LOG(LogTemp, Error, TEXT("COMP_BP[%s] PH6 CDO FAIL - Failed to get ActorComponent CDO"), *Definition.Name);
		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to get ActorComponent CDO"));
		Result.GeneratorId = TEXT("ComponentBlueprint");
		Result.DetermineCategory();
		return Result;
	}

	// Configure tick (must be done on CDO after compile)
	if (Definition.bCanEverTick)
	{
		ComponentCDO->PrimaryComponentTick.bCanEverTick = true;
		ComponentCDO->PrimaryComponentTick.bStartWithTickEnabled = Definition.bStartWithTickEnabled;

		if (Definition.TickInterval > 0.0f)
		{
			ComponentCDO->PrimaryComponentTick.TickInterval = Definition.TickInterval;
		}
	}
	else
	{
		ComponentCDO->PrimaryComponentTick.bCanEverTick = false;
	}

	// Mark dirty
	ComponentCDO->MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("COMP_BP[%s] PH6 CDO Tick CanEver=%s StartEnabled=%s Interval=%.2f OK"),
		*Definition.Name,
		Definition.bCanEverTick ? TEXT("true") : TEXT("false"),
		Definition.bStartWithTickEnabled ? TEXT("true") : TEXT("false"),
		Definition.TickInterval);

	// ========== PHASE 7: Save Asset ==========
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Blueprint);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("COMP_BP[%s] PH7 Save OK"), *Definition.Name);
	LogGeneration(FString::Printf(TEXT("Created Component Blueprint: %s (vars=%d, dispatchers=%d, funcs=%d)"),
		*Definition.Name, Definition.Variables.Num(), Definition.EventDispatchers.Num(), Definition.Functions.Num()));

	// Store metadata for regeneration tracking
	StoreBlueprintMetadata(Blueprint, TEXT("ComponentBlueprint"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("ComponentBlueprint");
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// FBlackboardGenerator Implementation
// v4.0: Added parent blackboard inheritance and base class for Object/Class keys
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

	// v4.0: Set parent blackboard if specified
	if (!Definition.Parent.IsEmpty())
	{
		// Try to find parent blackboard asset
		TArray<FString> SearchPaths = {
			FString::Printf(TEXT("%s/AI/%s"), *GetProjectRoot(), *Definition.Parent),
			FString::Printf(TEXT("%s/AI/Blackboards/%s"), *GetProjectRoot(), *Definition.Parent),
			FString::Printf(TEXT("/Game/AI/%s"), *Definition.Parent),
			FString::Printf(TEXT("/Game/%s"), *Definition.Parent)
		};

		UBlackboardData* ParentBB = nullptr;
		for (const FString& SearchPath : SearchPaths)
		{
			ParentBB = LoadObject<UBlackboardData>(nullptr, *SearchPath);
			if (ParentBB)
			{
				break;
			}
		}

		if (ParentBB)
		{
			Blackboard->Parent = ParentBB;
			LogGeneration(FString::Printf(TEXT("  Set parent blackboard: %s"), *Definition.Parent));
		}
		else
		{
			// v4.23 FAIL-FAST: S37 - Manifest references parent blackboard that doesn't exist
			LogGeneration(FString::Printf(TEXT("[E_BB_PARENT_NOT_FOUND] %s | Parent blackboard '%s' not found"), *Definition.Name, *Definition.Parent));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("Parent blackboard '%s' not found"), *Definition.Parent));
		}
	}

	// v4.0: Helper lambda to resolve base class for Object/Class keys
	auto ResolveBaseClass = [](const FString& BaseClassStr) -> UClass*
	{
		if (BaseClassStr.IsEmpty())
			return nullptr;

		// Try to find the class (nullptr = search in all packages)
		UClass* FoundClass = FindObject<UClass>(nullptr, *BaseClassStr);
		if (FoundClass)
			return FoundClass;

		// Try common paths
		FoundClass = LoadClass<UObject>(nullptr, *FString::Printf(TEXT("/Script/Engine.%s"), *BaseClassStr));
		if (FoundClass)
			return FoundClass;

		FoundClass = LoadClass<UObject>(nullptr, *FString::Printf(TEXT("/Script/CoreUObject.%s"), *BaseClassStr));
		if (FoundClass)
			return FoundClass;

		FoundClass = LoadClass<UObject>(nullptr, *FString::Printf(TEXT("/Script/NarrativeArsenal.%s"), *BaseClassStr));
		if (FoundClass)
			return FoundClass;

		return nullptr;
	};

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
			// v4.0: Use specified base class or default to AActor
			UClass* BaseClass = ResolveBaseClass(KeyDef.BaseClass);
			ObjectKey->BaseClass = BaseClass ? BaseClass : AActor::StaticClass();
			Entry.KeyType = ObjectKey;
			if (BaseClass && !KeyDef.BaseClass.IsEmpty())
			{
				LogGeneration(FString::Printf(TEXT("  Key '%s' base class: %s"), *KeyDef.Name, *KeyDef.BaseClass));
			}
		}
		else if (KeyDef.Type.Equals(TEXT("Class"), ESearchCase::IgnoreCase))
		{
			UBlackboardKeyType_Class* ClassKey = NewObject<UBlackboardKeyType_Class>(Blackboard);
			// v4.0: Use specified base class or default to UObject
			UClass* BaseClass = ResolveBaseClass(KeyDef.BaseClass);
			ClassKey->BaseClass = BaseClass ? BaseClass : UObject::StaticClass();
			Entry.KeyType = ClassKey;
			if (BaseClass && !KeyDef.BaseClass.IsEmpty())
			{
				LogGeneration(FString::Printf(TEXT("  Key '%s' base class: %s"), *KeyDef.Name, *KeyDef.BaseClass));
			}
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

	LogGeneration(FString::Printf(TEXT("Created Blackboard: %s with %d keys%s"),
		*Definition.Name, KeysCreated,
		Definition.Parent.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" (Parent: %s)"), *Definition.Parent)));

	// v3.0: Store metadata for regeneration tracking
	StoreDataAssetMetadata(Blackboard, TEXT("BB"), Definition.Name, Definition.ComputeHash());

	// v4.14: Register in session cache for BT generator lookup
	RegisterGeneratedBlackboard(Definition.Name, Blackboard);

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("Blackboard");
	Result.DetermineCategory();
	return Result;
}

// v4.14: Session-level blackboard cache implementation
UBlackboardData* FBlackboardGenerator::FindGeneratedBlackboard(const FString& BlackboardName)
{
	UBlackboardData** Found = GeneratedBlackboardsCache.Find(BlackboardName);
	if (!Found)
	{
		// Debug: Log what's in the cache
		LogGeneration(FString::Printf(TEXT("  [DEBUG] BB cache lookup failed for '%s'. Cache has %d entries:"), *BlackboardName, GeneratedBlackboardsCache.Num()));
		for (const auto& Entry : GeneratedBlackboardsCache)
		{
			LogGeneration(FString::Printf(TEXT("    - '%s'"), *Entry.Key));
		}
	}
	return Found ? *Found : nullptr;
}

void FBlackboardGenerator::RegisterGeneratedBlackboard(const FString& BlackboardName, UBlackboardData* Blackboard)
{
	if (Blackboard)
	{
		GeneratedBlackboardsCache.Add(BlackboardName, Blackboard);
		LogGeneration(FString::Printf(TEXT("  Registered blackboard '%s' in session cache"), *BlackboardName));
	}
}

void FBlackboardGenerator::ClearGeneratedBlackboardsCache()
{
	GeneratedBlackboardsCache.Empty();
	LogGeneration(TEXT("Cleared generated blackboards session cache"));
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
		UBlackboardData* BB = nullptr;

		// v4.14: Check session cache first (for blackboards generated in same run)
		BB = FBlackboardGenerator::FindGeneratedBlackboard(Definition.BlackboardAsset);
		if (BB)
		{
			LogGeneration(FString::Printf(TEXT("  Found blackboard '%s' in session cache"), *Definition.BlackboardAsset));
		}
		else
		{
			// Try multiple paths for blackboard (project paths first, then NarrativePro)
			TArray<FString> BBPaths;
			// Project paths
			BBPaths.Add(FString::Printf(TEXT("%s/AI/Blackboards/%s.%s"), *GetProjectRoot(), *Definition.BlackboardAsset, *Definition.BlackboardAsset));
			BBPaths.Add(FString::Printf(TEXT("%s/AI/%s.%s"), *GetProjectRoot(), *Definition.BlackboardAsset, *Definition.BlackboardAsset));
			BBPaths.Add(FString::Printf(TEXT("/Game/AI/Blackboards/%s.%s"), *Definition.BlackboardAsset, *Definition.BlackboardAsset));
			BBPaths.Add(FString::Printf(TEXT("/Game/AI/%s.%s"), *Definition.BlackboardAsset, *Definition.BlackboardAsset));
			BBPaths.Add(FString::Printf(TEXT("%s/Blackboards/%s.%s"), *GetProjectRoot(), *Definition.BlackboardAsset, *Definition.BlackboardAsset));
			// v4.13.2: NarrativePro standard blackboard paths
			BBPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/Attacks/%s.%s"), *Definition.BlackboardAsset, *Definition.BlackboardAsset));
			BBPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/FollowCharacter/%s.%s"), *Definition.BlackboardAsset, *Definition.BlackboardAsset));
			BBPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/Flee/%s.%s"), *Definition.BlackboardAsset, *Definition.BlackboardAsset));
			BBPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/GoToLocation/%s.%s"), *Definition.BlackboardAsset, *Definition.BlackboardAsset));
			BBPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/Idle/%s.%s"), *Definition.BlackboardAsset, *Definition.BlackboardAsset));
			BBPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/ReturnToSpawn/%s.%s"), *Definition.BlackboardAsset, *Definition.BlackboardAsset));

			for (const FString& BBPath : BBPaths)
			{
				BB = LoadObject<UBlackboardData>(nullptr, *BBPath);
				if (BB)
				{
					break;
				}
			}
		}

		if (BB)
		{
			BT->BlackboardAsset = BB;
			LogGeneration(FString::Printf(TEXT("  Linked Blackboard: %s (actual: %s, path: %s)"),
				*Definition.BlackboardAsset, *BB->GetName(), *BB->GetPathName()));
			// v4.15: Verify the assignment worked
			if (BT->BlackboardAsset != BB)
			{
				LogGeneration(TEXT("  [ERROR] BlackboardAsset assignment failed!"));
			}
		}
		else
		{
			// v4.23 FAIL-FAST: S38 - Manifest references blackboard that doesn't exist
			LogGeneration(FString::Printf(TEXT("[E_BT_BLACKBOARD_NOT_FOUND] %s | Blackboard not found: %s (cache has %d entries)"),
				*Definition.Name, *Definition.BlackboardAsset, FBlackboardGenerator::GetCacheSize()));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("Blackboard '%s' not found"), *Definition.BlackboardAsset));
		}
	}

	// ============================================================================
	// v4.0: Enhanced Behavior Tree Generation with Two-Pass System
	// Pass 1: Create all nodes
	// Pass 2: Wire parent-child relationships
	// ============================================================================

	// v4.0: Helper to convert FlowAbortMode string to enum
	auto GetFlowAbortMode = [](const FString& Mode) -> EBTFlowAbortMode::Type
	{
		if (Mode.Equals(TEXT("None"), ESearchCase::IgnoreCase))
			return EBTFlowAbortMode::None;
		else if (Mode.Equals(TEXT("Self"), ESearchCase::IgnoreCase))
			return EBTFlowAbortMode::Self;
		else if (Mode.Equals(TEXT("LowerPriority"), ESearchCase::IgnoreCase))
			return EBTFlowAbortMode::LowerPriority;
		else if (Mode.Equals(TEXT("Both"), ESearchCase::IgnoreCase))
			return EBTFlowAbortMode::Both;
		return EBTFlowAbortMode::None;
	};

	// v4.0: Helper to set property via reflection
	auto SetBTPropertyByReflection = [](UObject* Object, const FString& PropName, const FString& PropValue) -> bool
	{
		if (!Object) return false;

		FProperty* Property = Object->GetClass()->FindPropertyByName(FName(*PropName));
		if (!Property)
		{
			// Try with 'b' prefix for bools
			Property = Object->GetClass()->FindPropertyByName(FName(*FString::Printf(TEXT("b%s"), *PropName)));
		}
		if (!Property)
		{
			// Try removing underscores
			FString AltName = PropName;
			AltName.ReplaceInline(TEXT("_"), TEXT(""));
			Property = Object->GetClass()->FindPropertyByName(FName(*AltName));
		}

		if (!Property) return false;

		void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);

		if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
		{
			BoolProp->SetPropertyValue(ValuePtr, PropValue.ToBool() || PropValue.Equals(TEXT("true"), ESearchCase::IgnoreCase));
			return true;
		}
		else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
		{
			FloatProp->SetPropertyValue(ValuePtr, FCString::Atof(*PropValue));
			return true;
		}
		else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
		{
			DoubleProp->SetPropertyValue(ValuePtr, FCString::Atod(*PropValue));
			return true;
		}
		else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
		{
			IntProp->SetPropertyValue(ValuePtr, FCString::Atoi(*PropValue));
			return true;
		}
		else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
		{
			StrProp->SetPropertyValue(ValuePtr, PropValue);
			return true;
		}
		else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
		{
			NameProp->SetPropertyValue(ValuePtr, FName(*PropValue));
			return true;
		}

		return false;
	};

	// v4.0: Helper to set BlackboardKeySelector on an object
	auto SetBlackboardKey = [](UObject* Object, const FString& KeyName) -> bool
	{
		if (!Object || KeyName.IsEmpty()) return false;

		UClass* ObjClass = Object->GetClass();

		// Try common blackboard key property names
		TArray<FName> KeyPropNames = {
			TEXT("BlackboardKey"),
			TEXT("TargetBlackboardKey"),
			TEXT("EnemyKey"),
			TEXT("LocationKey"),
			TEXT("TargetKey"),
			TEXT("ActorKey")
		};

		for (const FName& PropName : KeyPropNames)
		{
			FProperty* BBKeyProp = ObjClass->FindPropertyByName(PropName);
			if (BBKeyProp)
			{
				if (FStructProperty* StructProp = CastField<FStructProperty>(BBKeyProp))
				{
					if (StructProp->Struct && StructProp->Struct->GetFName() == TEXT("BlackboardKeySelector"))
					{
						FBlackboardKeySelector* Selector = StructProp->ContainerPtrToValuePtr<FBlackboardKeySelector>(Object);
						if (Selector)
						{
							Selector->SelectedKeyName = FName(*KeyName);
							return true;
						}
					}
				}
			}
		}
		return false;
	};

	// v3.1/v4.0: Create root composite node if nodes are defined
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

			// v4.0: Build node map for two-pass system
			TMap<FString, UBTCompositeNode*> CompositeNodeMap;
			TMap<FString, UBTTaskNode*> TaskNodeMap;
			TMap<FString, TArray<UBTDecorator*>> NodeDecoratorsMap;

			// ================================================================
			// PASS 1: Create all nodes
			// ================================================================
			int32 NodesCreated = 0;
			for (const FManifestBTNodeDefinition& NodeDef : Definition.Nodes)
			{
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
						CompositeNode = NewObject<UBTComposite_Sequence>(BT);
					}
					else if (NodeDef.Type.Equals(TEXT("SimpleParallel"), ESearchCase::IgnoreCase))
					{
						CompositeNode = NewObject<UBTComposite_SimpleParallel>(BT);
					}
					else
					{
						CompositeNode = NewObject<UBTComposite_Selector>(BT);
					}

					if (CompositeNode)
					{
						CompositeNodeMap.Add(NodeDef.Id, CompositeNode);
						NodesCreated++;
						LogGeneration(FString::Printf(TEXT("  Created composite node: %s (%s)"), *NodeDef.Id, *NodeDef.Type));

						// v4.0: Add services to composite nodes
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
								if (ServiceClass) break;
							}

							if (ServiceClass)
							{
								UBTService* Service = NewObject<UBTService>(CompositeNode, ServiceClass);
								if (Service)
								{
									// v4.0: Set service properties
									if (FFloatProperty* IntervalProp = CastField<FFloatProperty>(ServiceClass->FindPropertyByName(TEXT("Interval"))))
									{
										IntervalProp->SetPropertyValue(IntervalProp->ContainerPtrToValuePtr<void>(Service), ServiceDef.Interval);
									}
									if (FFloatProperty* DeviationProp = CastField<FFloatProperty>(ServiceClass->FindPropertyByName(TEXT("RandomDeviation"))))
									{
										DeviationProp->SetPropertyValue(DeviationProp->ContainerPtrToValuePtr<void>(Service), ServiceDef.RandomDeviation);
									}
									if (FBoolProperty* TickStartProp = CastField<FBoolProperty>(ServiceClass->FindPropertyByName(TEXT("bCallTickOnSearchStart"))))
									{
										TickStartProp->SetPropertyValue(TickStartProp->ContainerPtrToValuePtr<void>(Service), ServiceDef.bCallTickOnSearchStart);
									}
									if (FBoolProperty* RestartProp = CastField<FBoolProperty>(ServiceClass->FindPropertyByName(TEXT("bRestartTimerOnEachActivation"))))
									{
										RestartProp->SetPropertyValue(RestartProp->ContainerPtrToValuePtr<void>(Service), ServiceDef.bRestartTimerOnActivation);
									}

									// v4.0: Apply custom properties
									for (const auto& Prop : ServiceDef.Properties)
									{
										SetBTPropertyByReflection(Service, Prop.Key, Prop.Value);
									}

									CompositeNode->Services.Add(Service);
									LogGeneration(FString::Printf(TEXT("    Added service: %s (interval=%.2f)"), *ServiceDef.Class, ServiceDef.Interval));
								}
							}
						}
					}
				}
				else
				{
					// Create task node
					UBTTaskNode* TaskNode = nullptr;

					if (!NodeDef.TaskClass.IsEmpty())
					{
						TArray<FString> TaskPaths;
						TaskPaths.Add(FString::Printf(TEXT("/Script/AIModule.%s"), *NodeDef.TaskClass));
						TaskPaths.Add(FString::Printf(TEXT("%s/AI/Tasks/%s.%s_C"), *GetProjectRoot(), *NodeDef.TaskClass, *NodeDef.TaskClass));
						TaskPaths.Add(FString::Printf(TEXT("/NarrativePro/AI/Tasks/%s.%s_C"), *NodeDef.TaskClass, *NodeDef.TaskClass));

						UClass* TaskClass = nullptr;
						for (const FString& Path : TaskPaths)
						{
							TaskClass = LoadClass<UBTTaskNode>(nullptr, *Path);
							if (TaskClass) break;
						}

						if (TaskClass)
						{
							TaskNode = NewObject<UBTTaskNode>(BT, TaskClass);
							LogGeneration(FString::Printf(TEXT("  Created task node: %s (%s)"), *NodeDef.Id, *NodeDef.TaskClass));
						}
						else
						{
							// v4.23 FAIL-FAST: Manifest references task class that cannot be resolved
							LogGeneration(FString::Printf(TEXT("[E_BT_TASK_CLASS_NOT_FOUND] %s | Task class not found: %s"),
								*Definition.Name, *NodeDef.TaskClass));
							return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
								FString::Printf(TEXT("BehaviorTree task class '%s' not found - manifest references class but resolution failed"), *NodeDef.TaskClass));
						}
					}
					else
					{
						TaskNode = NewObject<UBTTask_Wait>(BT);
						LogGeneration(FString::Printf(TEXT("  Created default Wait task: %s"), *NodeDef.Id));
					}

					if (TaskNode)
					{
						TaskNodeMap.Add(NodeDef.Id, TaskNode);
						NodesCreated++;

						// v4.0: Set blackboard key if specified
						if (!NodeDef.BlackboardKey.IsEmpty())
						{
							SetBlackboardKey(TaskNode, NodeDef.BlackboardKey);
						}

						// v4.0: Apply task properties via reflection
						for (const auto& Prop : NodeDef.Properties)
						{
							if (SetBTPropertyByReflection(TaskNode, Prop.Key, Prop.Value))
							{
								LogGeneration(FString::Printf(TEXT("    Set property %s = %s"), *Prop.Key, *Prop.Value));
							}
						}
					}
				}

				// v4.0: Create decorators for this node
				TArray<UBTDecorator*> NodeDecorators;
				for (const FManifestBTDecoratorDefinition& DecoratorDef : NodeDef.Decorators)
				{
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
						DecPaths.Add(FString::Printf(TEXT("/NarrativePro/AI/Decorators/%s.%s_C"), *DecoratorDef.Class, *DecoratorDef.Class));

						for (const FString& Path : DecPaths)
						{
							DecoratorClass = LoadClass<UBTDecorator>(nullptr, *Path);
							if (DecoratorClass) break;
						}
					}

					if (DecoratorClass)
					{
						UBTDecorator* Decorator = NewObject<UBTDecorator>(BT, DecoratorClass);
						if (Decorator)
						{
							// v4.0: Set inverse condition
							if (DecoratorDef.bInverseCondition)
							{
								if (FBoolProperty* InverseProp = CastField<FBoolProperty>(DecoratorClass->FindPropertyByName(TEXT("bInverseCondition"))))
								{
									InverseProp->SetPropertyValue(InverseProp->ContainerPtrToValuePtr<void>(Decorator), true);
								}
							}

							// v4.0: Set flow abort mode
							if (!DecoratorDef.FlowAbortMode.IsEmpty())
							{
								if (FByteProperty* AbortProp = CastField<FByteProperty>(DecoratorClass->FindPropertyByName(TEXT("FlowAbortMode"))))
								{
									AbortProp->SetPropertyValue(AbortProp->ContainerPtrToValuePtr<void>(Decorator), (uint8)GetFlowAbortMode(DecoratorDef.FlowAbortMode));
								}
								else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(DecoratorClass->FindPropertyByName(TEXT("FlowAbortMode"))))
								{
									void* ValuePtr = EnumProp->ContainerPtrToValuePtr<void>(Decorator);
									EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, (int64)GetFlowAbortMode(DecoratorDef.FlowAbortMode));
								}
							}

							// v4.0: Set blackboard key
							if (!DecoratorDef.BlackboardKey.IsEmpty())
							{
								SetBlackboardKey(Decorator, DecoratorDef.BlackboardKey);
							}

							// v4.13.2: Set key operation (IsSet, IsNotSet, etc.) for BTDecorator_Blackboard
							if (!DecoratorDef.Operation.IsEmpty())
							{
								// BTDecorator_Blackboard uses BasicOperation (EBasicKeyOperation) for simple checks
								// or StringOperation/ArithmeticOperation for typed checks
								// Map common operation strings to EBasicKeyOperation values
								int32 OperationValue = -1;
								FString OpLower = DecoratorDef.Operation.ToLower();
								if (OpLower == TEXT("isset") || OpLower == TEXT("is_set"))
								{
									OperationValue = 0; // EBasicKeyOperation::Set
								}
								else if (OpLower == TEXT("isnotset") || OpLower == TEXT("is_not_set"))
								{
									OperationValue = 1; // EBasicKeyOperation::NotSet
								}

								if (OperationValue >= 0)
								{
									// Try BasicOperation first (for basic key queries)
									if (FByteProperty* BasicOpProp = CastField<FByteProperty>(DecoratorClass->FindPropertyByName(TEXT("BasicOperation"))))
									{
										BasicOpProp->SetPropertyValue(BasicOpProp->ContainerPtrToValuePtr<void>(Decorator), (uint8)OperationValue);
										LogGeneration(FString::Printf(TEXT("      Set BasicOperation = %s"), *DecoratorDef.Operation));
									}
									else if (FEnumProperty* EnumOpProp = CastField<FEnumProperty>(DecoratorClass->FindPropertyByName(TEXT("BasicOperation"))))
									{
										void* ValuePtr = EnumOpProp->ContainerPtrToValuePtr<void>(Decorator);
										EnumOpProp->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, (int64)OperationValue);
										LogGeneration(FString::Printf(TEXT("      Set BasicOperation = %s"), *DecoratorDef.Operation));
									}
								}
							}

							// v4.13.2: Set NotifyObserver for BTDecorator_Blackboard
							if (!DecoratorDef.NotifyObserver.IsEmpty())
							{
								// Map notify observer strings to EBTBlackboardRestart values
								int32 NotifyValue = -1;
								FString NotifyLower = DecoratorDef.NotifyObserver.ToLower();
								if (NotifyLower == TEXT("onresultchange") || NotifyLower == TEXT("on_result_change") || NotifyLower == TEXT("resultchange"))
								{
									NotifyValue = 0; // EBTBlackboardRestart::ResultChange
								}
								else if (NotifyLower == TEXT("onvaluechange") || NotifyLower == TEXT("on_value_change") || NotifyLower == TEXT("valuechange"))
								{
									NotifyValue = 1; // EBTBlackboardRestart::ValueChange
								}

								if (NotifyValue >= 0)
								{
									if (FByteProperty* NotifyProp = CastField<FByteProperty>(DecoratorClass->FindPropertyByName(TEXT("NotifyObserver"))))
									{
										NotifyProp->SetPropertyValue(NotifyProp->ContainerPtrToValuePtr<void>(Decorator), (uint8)NotifyValue);
										LogGeneration(FString::Printf(TEXT("      Set NotifyObserver = %s"), *DecoratorDef.NotifyObserver));
									}
									else if (FEnumProperty* EnumNotifyProp = CastField<FEnumProperty>(DecoratorClass->FindPropertyByName(TEXT("NotifyObserver"))))
									{
										void* ValuePtr = EnumNotifyProp->ContainerPtrToValuePtr<void>(Decorator);
										EnumNotifyProp->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, (int64)NotifyValue);
										LogGeneration(FString::Printf(TEXT("      Set NotifyObserver = %s"), *DecoratorDef.NotifyObserver));
									}
								}
							}

							// v4.0: Apply custom properties
							for (const auto& Prop : DecoratorDef.Properties)
							{
								SetBTPropertyByReflection(Decorator, Prop.Key, Prop.Value);
							}

							NodeDecorators.Add(Decorator);
							LogGeneration(FString::Printf(TEXT("    Added decorator: %s%s"),
								*DecoratorDef.Class,
								DecoratorDef.bInverseCondition ? TEXT(" (inverted)") : TEXT("")));
						}
					}
				}

				if (NodeDecorators.Num() > 0)
				{
					NodeDecoratorsMap.Add(NodeDef.Id, NodeDecorators);
				}
			}

			// ================================================================
			// PASS 2: Wire parent-child relationships
			// ================================================================
			int32 ConnectionsWired = 0;
			for (const FManifestBTNodeDefinition& NodeDef : Definition.Nodes)
			{
				if (NodeDef.Children.Num() == 0) continue;

				// Find parent node (must be composite)
				UBTCompositeNode** ParentPtr = CompositeNodeMap.Find(NodeDef.Id);
				if (!ParentPtr)
				{
					// v4.23 FAIL-FAST: S39 - Manifest references parent node that doesn't exist
					LogGeneration(FString::Printf(TEXT("[E_BT_PARENT_NOT_FOUND] %s | Parent node '%s' not found or not composite"), *Definition.Name, *NodeDef.Id));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						FString::Printf(TEXT("BT parent node '%s' not found or not composite"), *NodeDef.Id));
				}

				UBTCompositeNode* ParentNode = *ParentPtr;

				for (const FString& ChildId : NodeDef.Children)
				{
					FBTCompositeChild ChildEntry;

					// Check if child is composite or task
					if (UBTCompositeNode** ChildCompositePtr = CompositeNodeMap.Find(ChildId))
					{
						ChildEntry.ChildComposite = *ChildCompositePtr;
					}
					else if (UBTTaskNode** ChildTaskPtr = TaskNodeMap.Find(ChildId))
					{
						ChildEntry.ChildTask = *ChildTaskPtr;
					}
					else
					{
						// v4.23 FAIL-FAST: S40 - Manifest references child node that doesn't exist
						LogGeneration(FString::Printf(TEXT("[E_BT_CHILD_NOT_FOUND] %s | Child node '%s' not found for parent '%s'"), *Definition.Name, *ChildId, *NodeDef.Id));
						return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
							FString::Printf(TEXT("BT child node '%s' not found for parent '%s'"), *ChildId, *NodeDef.Id));
					}

					// Add decorators to child entry
					if (TArray<UBTDecorator*>* Decorators = NodeDecoratorsMap.Find(ChildId))
					{
						ChildEntry.Decorators = *Decorators;
					}

					ParentNode->Children.Add(ChildEntry);
					ConnectionsWired++;
					LogGeneration(FString::Printf(TEXT("  Wired: %s -> %s"), *NodeDef.Id, *ChildId));
				}
			}

			// ================================================================
			// PASS 3: Add root-level nodes (nodes without parents)
			// ================================================================
			TSet<FString> ChildNodeIds;
			for (const FManifestBTNodeDefinition& NodeDef : Definition.Nodes)
			{
				for (const FString& ChildId : NodeDef.Children)
				{
					ChildNodeIds.Add(ChildId);
				}
			}

			for (const FManifestBTNodeDefinition& NodeDef : Definition.Nodes)
			{
				// Skip nodes that are children of other nodes
				if (ChildNodeIds.Contains(NodeDef.Id)) continue;

				// This is a root-level node, add to RootNode
				FBTCompositeChild ChildEntry;

				if (UBTCompositeNode** CompositePtr = CompositeNodeMap.Find(NodeDef.Id))
				{
					ChildEntry.ChildComposite = *CompositePtr;
				}
				else if (UBTTaskNode** TaskPtr = TaskNodeMap.Find(NodeDef.Id))
				{
					ChildEntry.ChildTask = *TaskPtr;
				}
				else
				{
					continue;
				}

				// Add decorators
				if (TArray<UBTDecorator*>* Decorators = NodeDecoratorsMap.Find(NodeDef.Id))
				{
					ChildEntry.Decorators = *Decorators;
				}

				RootNode->Children.Add(ChildEntry);
				LogGeneration(FString::Printf(TEXT("  Added root child: %s"), *NodeDef.Id));
			}

			LogGeneration(FString::Printf(TEXT("  Created %d nodes, wired %d connections"), NodesCreated, ConnectionsWired));
		}
	}

	// ============================================================================
	// v4.15: Create the UBehaviorTreeGraph (editor graph) for proper editor display
	// Without this, the editor creates the graph on load but nodes show errors
	// ============================================================================
	{
		// Create the editor graph using the proper schema
		const FName GraphName = TEXT("BehaviorTreeGraph");
		UClass* GraphClass = UBehaviorTreeGraph::StaticClass();
		UClass* SchemaClass = UEdGraphSchema_BehaviorTree::StaticClass();

		UBehaviorTreeGraph* BTGraph = CastChecked<UBehaviorTreeGraph>(FBlueprintEditorUtils::CreateNewGraph(
			BT, GraphName, GraphClass, SchemaClass));

		if (BTGraph)
		{
			BT->BTGraph = BTGraph;

			// Create default nodes (creates the root graph node)
			const UEdGraphSchema* Schema = BTGraph->GetSchema();
			if (Schema)
			{
				Schema->CreateDefaultNodesForGraph(*BTGraph);
			}

			// Initialize the graph - this calls SpawnMissingNodes() which creates
			// editor graph nodes from our runtime node structure
			BTGraph->OnCreated();

			// Update the graph to sync runtime and editor structures
			BTGraph->UpdateAsset();

			LogGeneration(TEXT("  Created UBehaviorTreeGraph with editor nodes"));
		}
		else
		{
			// v4.23 FAIL-FAST: S41 - Failed to create UBehaviorTreeGraph
			LogGeneration(FString::Printf(TEXT("[E_BT_GRAPH_CREATION_FAILED] %s | Failed to create UBehaviorTreeGraph"), *Definition.Name));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				TEXT("Failed to create UBehaviorTreeGraph"));
		}
	}

	// Mark dirty and register
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(BT);

	// v4.15: Log blackboard state before save
	if (BT->BlackboardAsset)
	{
		LogGeneration(FString::Printf(TEXT("  Pre-save BB check: %s"), *BT->BlackboardAsset->GetName()));
	}
	else
	{
		LogGeneration(TEXT("  Pre-save BB check: NULL"));
	}

	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, BT, *PackageFileName, SaveArgs);

	// v4.15: Log blackboard state after save
	if (BT->BlackboardAsset)
	{
		LogGeneration(FString::Printf(TEXT("  Post-save BB check: %s"), *BT->BlackboardAsset->GetName()));
	}
	else
	{
		LogGeneration(TEXT("  Post-save BB check: NULL"));
	}

	LogGeneration(FString::Printf(TEXT("Created Behavior Tree: %s"), *Definition.Name));

	// v3.0: Store metadata for regeneration tracking
	StoreDataAssetMetadata(BT, TEXT("BT"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("BehaviorTree");
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
// v4.9: VFX-specific material expressions
#include "Materials/MaterialExpressionParticleColor.h"
#include "Materials/MaterialExpressionParticleSubUV.h"
#include "Materials/MaterialExpressionDynamicParameter.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionDepthFade.h"
#include "Materials/MaterialExpressionSphereMask.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionObjectBounds.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionAbs.h"
#include "Materials/MaterialExpressionCosine.h"
#include "Materials/MaterialExpressionFloor.h"
#include "Materials/MaterialExpressionFrac.h"
#include "Materials/MaterialExpressionNormalize.h"
#include "Materials/MaterialExpressionDotProduct.h"
#include "Materials/MaterialExpressionCrossProduct.h"
#include "Materials/MaterialExpressionDistance.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionPixelDepth.h"
#include "Materials/MaterialExpressionSceneDepth.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/MaterialFunctionFactoryNew.h"

// v2.6.12: Helper to create material expression by type
// v4.10: Added ExpressionMap parameter for Switch/FunctionCall input connections
UMaterialExpression* FMaterialGenerator::CreateExpression(UMaterial* Material, const FManifestMaterialExpression& ExprDef, const TMap<FString, UMaterialExpression*>& ExpressionMap)
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
	// v4.0: TextureSample
	else if (TypeLower == TEXT("texturesample") || TypeLower == TEXT("texture_sample"))
	{
		UMaterialExpressionTextureSample* TexSample = NewObject<UMaterialExpressionTextureSample>(Material);
		// Load texture if specified
		FString TexturePath = ExprDef.TexturePath;
		if (TexturePath.IsEmpty() && ExprDef.Properties.Contains(TEXT("texture")))
		{
			TexturePath = ExprDef.Properties[TEXT("texture")];
		}
		if (!TexturePath.IsEmpty())
		{
			UTexture* Texture = LoadObject<UTexture>(nullptr, *TexturePath);
			if (Texture)
			{
				TexSample->Texture = Texture;
			}
		}
		// Set sampler type if specified
		if (!ExprDef.SamplerType.IsEmpty())
		{
			FString SamplerLower = ExprDef.SamplerType.ToLower();
			if (SamplerLower == TEXT("color"))
				TexSample->SamplerType = SAMPLERTYPE_Color;
			else if (SamplerLower == TEXT("linearcolor"))
				TexSample->SamplerType = SAMPLERTYPE_LinearColor;
			else if (SamplerLower == TEXT("normal"))
				TexSample->SamplerType = SAMPLERTYPE_Normal;
			else if (SamplerLower == TEXT("masks"))
				TexSample->SamplerType = SAMPLERTYPE_Masks;
			else if (SamplerLower == TEXT("grayscale"))
				TexSample->SamplerType = SAMPLERTYPE_Grayscale;
			else if (SamplerLower == TEXT("alpha"))
				TexSample->SamplerType = SAMPLERTYPE_Alpha;
		}
		Expression = TexSample;
	}
	// v4.0: TextureSampleParameter2D
	else if (TypeLower == TEXT("texturesampleparameter2d") || TypeLower == TEXT("textureparameter") || TypeLower == TEXT("texture_param"))
	{
		UMaterialExpressionTextureSampleParameter2D* TexParam = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		TexParam->ParameterName = *ExprDef.Name;
		// Load default texture if specified
		FString TexturePath = ExprDef.TexturePath;
		if (TexturePath.IsEmpty() && ExprDef.Properties.Contains(TEXT("texture")))
		{
			TexturePath = ExprDef.Properties[TEXT("texture")];
		}
		if (!TexturePath.IsEmpty())
		{
			UTexture* Texture = LoadObject<UTexture>(nullptr, *TexturePath);
			if (Texture)
			{
				TexParam->Texture = Texture;
			}
		}
		// Set sampler type if specified
		if (!ExprDef.SamplerType.IsEmpty())
		{
			FString SamplerLower = ExprDef.SamplerType.ToLower();
			if (SamplerLower == TEXT("color"))
				TexParam->SamplerType = SAMPLERTYPE_Color;
			else if (SamplerLower == TEXT("linearcolor"))
				TexParam->SamplerType = SAMPLERTYPE_LinearColor;
			else if (SamplerLower == TEXT("normal"))
				TexParam->SamplerType = SAMPLERTYPE_Normal;
			else if (SamplerLower == TEXT("masks"))
				TexParam->SamplerType = SAMPLERTYPE_Masks;
			else if (SamplerLower == TEXT("grayscale"))
				TexParam->SamplerType = SAMPLERTYPE_Grayscale;
			else if (SamplerLower == TEXT("alpha"))
				TexParam->SamplerType = SAMPLERTYPE_Alpha;
		}
		Expression = TexParam;
	}
	// ========================================================================
	// v4.9: VFX-Specific Material Expressions
	// ========================================================================
	// ParticleColor - reads color from Niagara/Cascade particle system
	else if (TypeLower == TEXT("particlecolor") || TypeLower == TEXT("particle_color"))
	{
		Expression = NewObject<UMaterialExpressionParticleColor>(Material);
	}
	// ParticleSubUV - flipbook UV animation for particles
	else if (TypeLower == TEXT("particlesubuv") || TypeLower == TEXT("particle_subuv"))
	{
		UMaterialExpressionParticleSubUV* ParticleSubUV = NewObject<UMaterialExpressionParticleSubUV>(Material);
		if (ExprDef.Properties.Contains(TEXT("blend")))
		{
			ParticleSubUV->bBlend = ExprDef.Properties[TEXT("blend")].ToBool();
		}
		Expression = ParticleSubUV;
	}
	// DynamicParameter - runtime shader control via Niagara
	else if (TypeLower == TEXT("dynamicparameter") || TypeLower == TEXT("dynamic_parameter"))
	{
		UMaterialExpressionDynamicParameter* DynParam = NewObject<UMaterialExpressionDynamicParameter>(Material);
		// Set parameter names if specified
		if (ExprDef.Properties.Contains(TEXT("param_names")))
		{
			TArray<FString> Names;
			ExprDef.Properties[TEXT("param_names")].ParseIntoArray(Names, TEXT(","));
			for (int32 i = 0; i < FMath::Min(Names.Num(), 4); ++i)
			{
				DynParam->ParamNames[i] = Names[i].TrimStartAndEnd();
			}
		}
		Expression = DynParam;
	}
	// VertexColor - reads vertex color from mesh
	else if (TypeLower == TEXT("vertexcolor") || TypeLower == TEXT("vertex_color"))
	{
		Expression = NewObject<UMaterialExpressionVertexColor>(Material);
	}
	// DepthFade - soft particle blending
	else if (TypeLower == TEXT("depthfade") || TypeLower == TEXT("depth_fade"))
	{
		UMaterialExpressionDepthFade* DepthFade = NewObject<UMaterialExpressionDepthFade>(Material);
		if (!ExprDef.DefaultValue.IsEmpty())
		{
			DepthFade->FadeDistanceDefault = FCString::Atof(*ExprDef.DefaultValue);
		}
		else if (ExprDef.Properties.Contains(TEXT("fade_distance")))
		{
			DepthFade->FadeDistanceDefault = FCString::Atof(*ExprDef.Properties[TEXT("fade_distance")]);
		}
		Expression = DepthFade;
	}
	// SphereMask - distance-based mask
	else if (TypeLower == TEXT("spheremask") || TypeLower == TEXT("sphere_mask"))
	{
		UMaterialExpressionSphereMask* SphereMask = NewObject<UMaterialExpressionSphereMask>(Material);
		if (ExprDef.Properties.Contains(TEXT("radius")))
		{
			SphereMask->AttenuationRadius = FCString::Atof(*ExprDef.Properties[TEXT("radius")]);
		}
		if (ExprDef.Properties.Contains(TEXT("hardness")))
		{
			SphereMask->HardnessPercent = FCString::Atof(*ExprDef.Properties[TEXT("hardness")]);
		}
		Expression = SphereMask;
	}
	// CameraPositionWS - world space camera position
	else if (TypeLower == TEXT("cameraposition") || TypeLower == TEXT("camera_position") || TypeLower == TEXT("camerapositionws"))
	{
		Expression = NewObject<UMaterialExpressionCameraPositionWS>(Material);
	}
	// ObjectBounds - object bounding box
	else if (TypeLower == TEXT("objectbounds") || TypeLower == TEXT("object_bounds"))
	{
		Expression = NewObject<UMaterialExpressionObjectBounds>(Material);
	}
	// Saturate - clamp 0-1
	else if (TypeLower == TEXT("saturate"))
	{
		Expression = NewObject<UMaterialExpressionSaturate>(Material);
	}
	// Abs - absolute value
	else if (TypeLower == TEXT("abs") || TypeLower == TEXT("absolute"))
	{
		Expression = NewObject<UMaterialExpressionAbs>(Material);
	}
	// Cosine
	else if (TypeLower == TEXT("cosine") || TypeLower == TEXT("cos"))
	{
		Expression = NewObject<UMaterialExpressionCosine>(Material);
	}
	// Floor
	else if (TypeLower == TEXT("floor"))
	{
		Expression = NewObject<UMaterialExpressionFloor>(Material);
	}
	// Frac
	else if (TypeLower == TEXT("frac") || TypeLower == TEXT("fract"))
	{
		Expression = NewObject<UMaterialExpressionFrac>(Material);
	}
	// Normalize
	else if (TypeLower == TEXT("normalize"))
	{
		Expression = NewObject<UMaterialExpressionNormalize>(Material);
	}
	// DotProduct
	else if (TypeLower == TEXT("dot") || TypeLower == TEXT("dotproduct"))
	{
		Expression = NewObject<UMaterialExpressionDotProduct>(Material);
	}
	// CrossProduct
	else if (TypeLower == TEXT("cross") || TypeLower == TEXT("crossproduct"))
	{
		Expression = NewObject<UMaterialExpressionCrossProduct>(Material);
	}
	// Distance
	else if (TypeLower == TEXT("distance"))
	{
		Expression = NewObject<UMaterialExpressionDistance>(Material);
	}
	// WorldPosition
	else if (TypeLower == TEXT("worldposition") || TypeLower == TEXT("world_position"))
	{
		Expression = NewObject<UMaterialExpressionWorldPosition>(Material);
	}
	// PixelDepth
	else if (TypeLower == TEXT("pixeldepth") || TypeLower == TEXT("pixel_depth"))
	{
		Expression = NewObject<UMaterialExpressionPixelDepth>(Material);
	}
	// SceneDepth
	else if (TypeLower == TEXT("scenedepth") || TypeLower == TEXT("scene_depth"))
	{
		Expression = NewObject<UMaterialExpressionSceneDepth>(Material);
	}
	// ========================================================================
	// v4.10: Tier 0 Zero-Property Expressions
	// ========================================================================
	// PerInstanceFadeAmount - per-instance fade for LOD
	else if (TypeLower == TEXT("perinstancefadeamount") || TypeLower == TEXT("per_instance_fade"))
	{
		Expression = NewObject<UMaterialExpressionPerInstanceFadeAmount>(Material);
	}
	// PerInstanceRandom - unique random value per instance
	else if (TypeLower == TEXT("perinstancerandom") || TypeLower == TEXT("per_instance_random"))
	{
		Expression = NewObject<UMaterialExpressionPerInstanceRandom>(Material);
	}
	// ObjectOrientation - object's rotation as quaternion
	else if (TypeLower == TEXT("objectorientation") || TypeLower == TEXT("object_orientation"))
	{
		Expression = NewObject<UMaterialExpressionObjectOrientation>(Material);
	}
	// ObjectPositionWS - object's world position
	else if (TypeLower == TEXT("objectpositionws") || TypeLower == TEXT("object_position_ws") || TypeLower == TEXT("objectposition"))
	{
		Expression = NewObject<UMaterialExpressionObjectPositionWS>(Material);
	}
	// ActorPositionWS - actor's world position
	else if (TypeLower == TEXT("actorpositionws") || TypeLower == TEXT("actor_position_ws") || TypeLower == TEXT("actorposition"))
	{
		Expression = NewObject<UMaterialExpressionActorPositionWS>(Material);
	}
	// ObjectRadius - bounding sphere radius
	else if (TypeLower == TEXT("objectradius") || TypeLower == TEXT("object_radius"))
	{
		Expression = NewObject<UMaterialExpressionObjectRadius>(Material);
	}
	// PreSkinnedLocalBounds - pre-skinning bounds
	else if (TypeLower == TEXT("preskinnedlocalbounds") || TypeLower == TEXT("pre_skinned_bounds"))
	{
		Expression = NewObject<UMaterialExpressionPreSkinnedLocalBounds>(Material);
	}
	// PrecomputedAOMask - baked AO from lightmass
	else if (TypeLower == TEXT("precomputedaomask") || TypeLower == TEXT("precomputed_ao"))
	{
		Expression = NewObject<UMaterialExpressionPrecomputedAOMask>(Material);
	}
	// LightmapUVs - lightmap texture coordinates
	else if (TypeLower == TEXT("lightmapuvs") || TypeLower == TEXT("lightmap_uvs"))
	{
		Expression = NewObject<UMaterialExpressionLightmapUVs>(Material);
	}
	// PreSkinnedNormal - pre-skinning normal (accepts both preskinnedlocalnormal and preskinnednormal)
	else if (TypeLower == TEXT("preskinnedlocalnormal") || TypeLower == TEXT("preskinnednormal") || TypeLower == TEXT("pre_skinned_normal"))
	{
		Expression = NewObject<UMaterialExpressionPreSkinnedNormal>(Material);
	}
	// VertexTangentWS - vertex tangent in world space
	else if (TypeLower == TEXT("vertextangentws") || TypeLower == TEXT("vertex_tangent_ws"))
	{
		Expression = NewObject<UMaterialExpressionVertexTangentWS>(Material);
	}
	// ScreenPosition - screen-space position
	else if (TypeLower == TEXT("screenposition") || TypeLower == TEXT("screen_position"))
	{
		Expression = NewObject<UMaterialExpressionScreenPosition>(Material);
	}
	// ViewSize - viewport dimensions
	else if (TypeLower == TEXT("viewsize") || TypeLower == TEXT("view_size"))
	{
		Expression = NewObject<UMaterialExpressionViewSize>(Material);
	}
	// SceneTexelSize - 1/viewport dimensions
	else if (TypeLower == TEXT("scenetexelsize") || TypeLower == TEXT("scene_texel_size"))
	{
		Expression = NewObject<UMaterialExpressionSceneTexelSize>(Material);
	}
	// TwoSidedSign - +1 for front face, -1 for back face
	else if (TypeLower == TEXT("twosidedsign") || TypeLower == TEXT("two_sided_sign"))
	{
		Expression = NewObject<UMaterialExpressionTwoSidedSign>(Material);
	}
	// ========================================================================
	// v4.10: Switch Expressions
	// ========================================================================
	// QualitySwitch - switch based on material quality level (Low=0, High=1, Medium=2, Epic=3)
	else if (TypeLower == TEXT("qualityswitch") || TypeLower == TEXT("quality_switch"))
	{
		UMaterialExpressionQualitySwitch* QualitySwitch = NewObject<UMaterialExpressionQualitySwitch>(Material);
		Expression = QualitySwitch;
		// Inputs will be connected in a second pass via ConnectSwitchInputs
	}
	// ShadingPathSwitch - switch based on shading path (Deferred=0, Forward=1, Mobile=2)
	else if (TypeLower == TEXT("shadingpathswitch") || TypeLower == TEXT("shading_path_switch"))
	{
		UMaterialExpressionShadingPathSwitch* ShadingSwitch = NewObject<UMaterialExpressionShadingPathSwitch>(Material);
		Expression = ShadingSwitch;
		// Inputs will be connected in a second pass via ConnectSwitchInputs
	}
	// FeatureLevelSwitch - switch based on feature level (ES3_1=1, SM5=3, SM6=4)
	else if (TypeLower == TEXT("featurelevelswitch") || TypeLower == TEXT("feature_level_switch"))
	{
		UMaterialExpressionFeatureLevelSwitch* FeatureSwitch = NewObject<UMaterialExpressionFeatureLevelSwitch>(Material);
		Expression = FeatureSwitch;
		// Inputs will be connected in a second pass via ConnectSwitchInputs
	}
	// ========================================================================
	// v4.10: MaterialFunctionCall - 3-tier resolution
	// ========================================================================
	else if (TypeLower == TEXT("materialfunctioncall") || TypeLower == TEXT("function_call") || TypeLower == TEXT("functioncall"))
	{
		UMaterialExpressionMaterialFunctionCall* FuncCall = NewObject<UMaterialExpressionMaterialFunctionCall>(Material);
		if (!ExprDef.Function.IsEmpty())
		{
			UMaterialFunctionInterface* ResolvedFunc = ResolveMaterialFunction(ExprDef.Function);
			if (ResolvedFunc)
			{
				FuncCall->SetMaterialFunction(ResolvedFunc);
				LogGeneration(FString::Printf(TEXT("    MaterialFunctionCall '%s' -> resolved to '%s'"), *ExprDef.Id, *ExprDef.Function));
			}
			else
			{
				// v4.23 FAIL-FAST: S42 - Manifest references material function that doesn't exist
				LogGeneration(FString::Printf(TEXT("[E_MAT_FUNCTION_NOT_FOUND] MaterialFunctionCall '%s' -> function NOT FOUND: '%s'"), *ExprDef.Id, *ExprDef.Function));
				return nullptr; // Signal failure to caller
			}
		}
		Expression = FuncCall;
		// Function inputs will be connected in a second pass via ConnectFunctionInputs
	}
	// ========================================================================
	// v4.10: Static Expressions (compile-time switches)
	// ========================================================================
	// StaticSwitch - compile-time switch based on static bool
	else if (TypeLower == TEXT("staticswitch") || TypeLower == TEXT("static_switch"))
	{
		UMaterialExpressionStaticSwitch* StaticSwitch = NewObject<UMaterialExpressionStaticSwitch>(Material);
		// Default value can be set via properties
		if (ExprDef.Properties.Contains(TEXT("default_value")))
		{
			StaticSwitch->DefaultValue = ExprDef.Properties[TEXT("default_value")].ToBool();
		}
		Expression = StaticSwitch;
	}
	// StaticBool - compile-time boolean constant
	else if (TypeLower == TEXT("staticbool") || TypeLower == TEXT("static_bool"))
	{
		UMaterialExpressionStaticBool* StaticBool = NewObject<UMaterialExpressionStaticBool>(Material);
		if (!ExprDef.DefaultValue.IsEmpty())
		{
			StaticBool->Value = ExprDef.DefaultValue.ToBool();
		}
		Expression = StaticBool;
	}
	// StaticBoolParameter - compile-time boolean parameter (editable in instances)
	else if (TypeLower == TEXT("staticboolparameter") || TypeLower == TEXT("static_bool_param"))
	{
		UMaterialExpressionStaticBoolParameter* StaticBoolParam = NewObject<UMaterialExpressionStaticBoolParameter>(Material);
		StaticBoolParam->ParameterName = *ExprDef.Name;
		if (!ExprDef.DefaultValue.IsEmpty())
		{
			StaticBoolParam->DefaultValue = ExprDef.DefaultValue.ToBool();
		}
		Expression = StaticBoolParam;
	}
	// ========================================================================
	// v4.10: Noise/Sampling Expressions
	// ========================================================================
	// Sobol - Sobol quasi-random sequence
	else if (TypeLower == TEXT("sobol"))
	{
		UMaterialExpressionSobol* Sobol = NewObject<UMaterialExpressionSobol>(Material);
		Expression = Sobol;
	}
	// TemporalSobol - temporal Sobol quasi-random sequence
	else if (TypeLower == TEXT("temporalsobol") || TypeLower == TEXT("temporal_sobol"))
	{
		UMaterialExpressionTemporalSobol* TemporalSobol = NewObject<UMaterialExpressionTemporalSobol>(Material);
		Expression = TemporalSobol;
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

// ============================================================================
// v4.10: Connect Switch expression inputs (QualitySwitch, ShadingPathSwitch, FeatureLevelSwitch)
// ============================================================================
bool FMaterialGenerator::ConnectSwitchInputs(UMaterialExpression* SwitchExpr, const FManifestMaterialExpression& ExprDef, const TMap<FString, UMaterialExpression*>& ExpressionMap)
{
	if (!SwitchExpr || ExprDef.Inputs.Num() == 0)
	{
		return false;
	}

	FString TypeLower = ExprDef.Type.ToLower();
	int32 ConnectionsMade = 0;

	// QualitySwitch - Low=0, High=1, Medium=2, Epic=3
	if (TypeLower == TEXT("qualityswitch") || TypeLower == TEXT("quality_switch"))
	{
		UMaterialExpressionQualitySwitch* QualitySwitch = Cast<UMaterialExpressionQualitySwitch>(SwitchExpr);
		if (!QualitySwitch) return false;

		for (const auto& InputPair : ExprDef.Inputs)
		{
			FString KeyLower = InputPair.Key.ToLower();
			UMaterialExpression* SourceExpr = ExpressionMap.FindRef(InputPair.Value);
			if (!SourceExpr)
			{
				LogGeneration(FString::Printf(TEXT("      [WARNING] QualitySwitch input '%s' references unknown expression '%s'"), *InputPair.Key, *InputPair.Value));
				continue;
			}

			const int32* IndexPtr = QualitySwitchKeyMap.Find(KeyLower);
			if (IndexPtr)
			{
				QualitySwitch->Inputs[*IndexPtr].Connect(0, SourceExpr);
				ConnectionsMade++;
				LogGeneration(FString::Printf(TEXT("      Connected QualitySwitch.%s (index %d) <- %s"), *InputPair.Key, *IndexPtr, *InputPair.Value));
			}
			else if (KeyLower == TEXT("default"))
			{
				QualitySwitch->Default.Connect(0, SourceExpr);
				ConnectionsMade++;
				LogGeneration(FString::Printf(TEXT("      Connected QualitySwitch.Default <- %s"), *InputPair.Value));
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("      [WARNING] Unknown QualitySwitch input key: '%s'"), *InputPair.Key));
			}
		}
	}
	// ShadingPathSwitch - Deferred=0, Forward=1, Mobile=2
	else if (TypeLower == TEXT("shadingpathswitch") || TypeLower == TEXT("shading_path_switch"))
	{
		UMaterialExpressionShadingPathSwitch* ShadingSwitch = Cast<UMaterialExpressionShadingPathSwitch>(SwitchExpr);
		if (!ShadingSwitch) return false;

		for (const auto& InputPair : ExprDef.Inputs)
		{
			FString KeyLower = InputPair.Key.ToLower();
			UMaterialExpression* SourceExpr = ExpressionMap.FindRef(InputPair.Value);
			if (!SourceExpr)
			{
				LogGeneration(FString::Printf(TEXT("      [WARNING] ShadingPathSwitch input '%s' references unknown expression '%s'"), *InputPair.Key, *InputPair.Value));
				continue;
			}

			const int32* IndexPtr = ShadingPathSwitchKeyMap.Find(KeyLower);
			if (IndexPtr)
			{
				ShadingSwitch->Inputs[*IndexPtr].Connect(0, SourceExpr);
				ConnectionsMade++;
				LogGeneration(FString::Printf(TEXT("      Connected ShadingPathSwitch.%s (index %d) <- %s"), *InputPair.Key, *IndexPtr, *InputPair.Value));
			}
			else if (KeyLower == TEXT("default"))
			{
				ShadingSwitch->Default.Connect(0, SourceExpr);
				ConnectionsMade++;
				LogGeneration(FString::Printf(TEXT("      Connected ShadingPathSwitch.Default <- %s"), *InputPair.Value));
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("      [WARNING] Unknown ShadingPathSwitch input key: '%s'"), *InputPair.Key));
			}
		}
	}
	// FeatureLevelSwitch - ES3_1=1, SM5=3, SM6=4 (note: indices 0,2 are deprecated)
	else if (TypeLower == TEXT("featurelevelswitch") || TypeLower == TEXT("feature_level_switch"))
	{
		UMaterialExpressionFeatureLevelSwitch* FeatureSwitch = Cast<UMaterialExpressionFeatureLevelSwitch>(SwitchExpr);
		if (!FeatureSwitch) return false;

		for (const auto& InputPair : ExprDef.Inputs)
		{
			FString KeyLower = InputPair.Key.ToLower();
			UMaterialExpression* SourceExpr = ExpressionMap.FindRef(InputPair.Value);
			if (!SourceExpr)
			{
				LogGeneration(FString::Printf(TEXT("      [WARNING] FeatureLevelSwitch input '%s' references unknown expression '%s'"), *InputPair.Key, *InputPair.Value));
				continue;
			}

			// Check for deprecated keys
			if (DeprecatedFeatureLevelKeys.Contains(KeyLower))
			{
				LogGeneration(FString::Printf(TEXT("      [WARNING] FeatureLevelSwitch key '%s' is deprecated (ES2/SM4 removed in UE5)"), *InputPair.Key));
				continue;
			}

			const int32* IndexPtr = FeatureLevelSwitchKeyMap.Find(KeyLower);
			if (IndexPtr)
			{
				FeatureSwitch->Inputs[*IndexPtr].Connect(0, SourceExpr);
				ConnectionsMade++;
				LogGeneration(FString::Printf(TEXT("      Connected FeatureLevelSwitch.%s (index %d) <- %s"), *InputPair.Key, *IndexPtr, *InputPair.Value));
			}
			else if (KeyLower == TEXT("default"))
			{
				FeatureSwitch->Default.Connect(0, SourceExpr);
				ConnectionsMade++;
				LogGeneration(FString::Printf(TEXT("      Connected FeatureLevelSwitch.Default <- %s"), *InputPair.Value));
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("      [WARNING] Unknown FeatureLevelSwitch input key: '%s'"), *InputPair.Key));
			}
		}
	}

	return ConnectionsMade > 0;
}

// ============================================================================
// v4.10: Connect MaterialFunctionCall inputs
// ============================================================================
bool FMaterialGenerator::ConnectFunctionInputs(UMaterialExpressionMaterialFunctionCall* FuncCall, const FManifestMaterialExpression& ExprDef, const TMap<FString, UMaterialExpression*>& ExpressionMap)
{
	if (!FuncCall || ExprDef.FunctionInputs.Num() == 0)
	{
		return false;
	}

	UMaterialFunctionInterface* Function = FuncCall->MaterialFunction;
	if (!Function)
	{
		LogGeneration(FString::Printf(TEXT("      [WARNING] MaterialFunctionCall '%s' has no resolved function"), *ExprDef.Id));
		return false;
	}

	int32 ConnectionsMade = 0;

	// Get the function's input expressions to find input pin names
	for (const auto& InputPair : ExprDef.FunctionInputs)
	{
		FString InputName = InputPair.Key;
		UMaterialExpression* SourceExpr = ExpressionMap.FindRef(InputPair.Value);
		if (!SourceExpr)
		{
			LogGeneration(FString::Printf(TEXT("      [WARNING] FunctionInput '%s' references unknown expression '%s'"), *InputName, *InputPair.Value));
			continue;
		}

		// Find the input index by name in the FunctionCall's FunctionInputs array
		bool bFound = false;
		for (int32 i = 0; i < FuncCall->FunctionInputs.Num(); ++i)
		{
			const FFunctionExpressionInput& FuncInput = FuncCall->FunctionInputs[i];
			if (FuncInput.ExpressionInput && FuncInput.ExpressionInput->InputName.ToString().Equals(InputName, ESearchCase::IgnoreCase))
			{
				// Connect through the FunctionInput at this index
				FuncCall->FunctionInputs[i].Input.Connect(0, SourceExpr);
				ConnectionsMade++;
				LogGeneration(FString::Printf(TEXT("      Connected FunctionInput[%d] '%s' <- %s"), i, *InputName, *InputPair.Value));
				bFound = true;
				break;
			}
		}

		if (!bFound)
		{
			LogGeneration(FString::Printf(TEXT("      [WARNING] Function '%s' has no input named '%s'"), *Function->GetName(), *InputName));
		}
	}

	return ConnectionsMade > 0;
}

// ============================================================================
// v4.9.1: Session-level material cache for MIC parent lookup
// ============================================================================
UMaterialInterface* FMaterialGenerator::FindGeneratedMaterial(const FString& MaterialName)
{
	UMaterialInterface** Found = GeneratedMaterialsCache.Find(MaterialName);
	return Found ? *Found : nullptr;
}

void FMaterialGenerator::RegisterGeneratedMaterial(const FString& MaterialName, UMaterialInterface* Material)
{
	if (Material)
	{
		GeneratedMaterialsCache.Add(MaterialName, Material);
		LogGeneration(FString::Printf(TEXT("  Registered material '%s' in session cache"), *MaterialName));
	}
}

void FMaterialGenerator::ClearGeneratedMaterialsCache()
{
	GeneratedMaterialsCache.Empty();
	LogGeneration(TEXT("Cleared generated materials session cache"));
}

// ============================================================================
// v4.10: Session-level material function cache for MaterialFunctionCall resolution
// ============================================================================
UMaterialFunctionInterface* FMaterialGenerator::FindGeneratedMaterialFunction(const FString& FunctionName)
{
	UMaterialFunctionInterface** Found = GeneratedMaterialFunctionsCache.Find(FunctionName);
	return Found ? *Found : nullptr;
}

void FMaterialGenerator::RegisterGeneratedMaterialFunction(const FString& FunctionName, UMaterialFunctionInterface* Function)
{
	if (Function)
	{
		GeneratedMaterialFunctionsCache.Add(FunctionName, Function);
		LogGeneration(FString::Printf(TEXT("  Registered material function '%s' in session cache"), *FunctionName));
	}
}

void FMaterialGenerator::ClearGeneratedMaterialFunctionsCache()
{
	GeneratedMaterialFunctionsCache.Empty();
	LogGeneration(TEXT("Cleared generated material functions session cache"));
}

// ============================================================================
// v4.10: 3-Tier Material Function Resolution
// ============================================================================
UMaterialFunctionInterface* FMaterialGenerator::ResolveMaterialFunction(const FString& FunctionPath)
{
	if (FunctionPath.IsEmpty())
	{
		return nullptr;
	}

	// Tier 1: Engine path - load directly
	if (FunctionPath.StartsWith(TEXT("/Engine/")))
	{
		UMaterialFunctionInterface* Function = LoadObject<UMaterialFunctionInterface>(nullptr, *FunctionPath);
		if (Function)
		{
			LogGeneration(FString::Printf(TEXT("    -> FOUND engine function: %s"), *FunctionPath));
			return Function;
		}
		LogGeneration(FString::Printf(TEXT("    -> NOT FOUND engine function: %s"), *FunctionPath));
		return nullptr;
	}

	// Tier 2: Project path - load directly
	if (FunctionPath.StartsWith(TEXT("/Game/")))
	{
		UMaterialFunctionInterface* Function = LoadObject<UMaterialFunctionInterface>(nullptr, *FunctionPath);
		if (Function)
		{
			LogGeneration(FString::Printf(TEXT("    -> FOUND project function: %s"), *FunctionPath));
			return Function;
		}
		LogGeneration(FString::Printf(TEXT("    -> NOT FOUND project function: %s"), *FunctionPath));
		return nullptr;
	}

	// Tier 3: Short name (MF_*) - search session cache then project paths
	FString FunctionName = FunctionPath;

	// Check session cache first
	UMaterialFunctionInterface* CachedFunction = FindGeneratedMaterialFunction(FunctionName);
	if (CachedFunction)
	{
		LogGeneration(FString::Printf(TEXT("    -> FOUND in session cache: %s"), *FunctionName));
		return CachedFunction;
	}

	// Search common project paths
	TArray<FString> SearchPaths = {
		FString::Printf(TEXT("%s/Materials/Functions/%s.%s"), *GetProjectRoot(), *FunctionName, *FunctionName),
		FString::Printf(TEXT("%s/Materials/%s.%s"), *GetProjectRoot(), *FunctionName, *FunctionName),
		FString::Printf(TEXT("/Game/Materials/Functions/%s.%s"), *FunctionName, *FunctionName),
	};

	for (const FString& SearchPath : SearchPaths)
	{
		UMaterialFunctionInterface* Function = LoadObject<UMaterialFunctionInterface>(nullptr, *SearchPath);
		if (Function)
		{
			LogGeneration(FString::Printf(TEXT("    -> FOUND at: %s"), *SearchPath));
			return Function;
		}
	}

	LogGeneration(FString::Printf(TEXT("    -> NOT FOUND: %s (searched session cache and %d paths)"), *FunctionName, SearchPaths.Num()));
	return nullptr;
}

// ============================================================================
// v4.10: Material Expression Validation with 6 Guardrails
// ============================================================================

// Known expression types for validation (exhaustive list)
static const TSet<FString> KnownExpressionTypes = {
	// Basic Math
	TEXT("constant"), TEXT("constant2vector"), TEXT("constant3vector"), TEXT("constant4vector"),
	TEXT("add"), TEXT("subtract"), TEXT("multiply"), TEXT("divide"), TEXT("power"),
	TEXT("abs"), TEXT("floor"), TEXT("ceil"), TEXT("frac"), TEXT("fmod"), TEXT("saturate"),
	TEXT("clamp"), TEXT("oneminus"), TEXT("lerp"), TEXT("min"), TEXT("max"),
	TEXT("sine"), TEXT("cosine"), TEXT("tangent"), TEXT("arcsine"), TEXT("arccosine"), TEXT("arctangent"),
	// Vector Operations
	TEXT("normalize"), TEXT("dotproduct"), TEXT("crossproduct"), TEXT("distance"), TEXT("length"),
	TEXT("appendvector"), TEXT("componentmask"), TEXT("breakoutfloat2"), TEXT("breakoutfloat3"), TEXT("breakoutfloat4"),
	// Texture
	TEXT("texturesample"), TEXT("texturesampleparameter2d"), TEXT("texturecoordinate"), TEXT("texcoord"), TEXT("uv"),
	TEXT("panner"), TEXT("rotator"),
	// Parameters (full and short aliases)
	TEXT("scalarparameter"), TEXT("scalarparam"), TEXT("vectorparameter"), TEXT("vectorparam"),
	TEXT("staticboolparameter"), TEXT("staticbool"),
	// Time
	TEXT("time"), TEXT("deltatime"), TEXT("realtime"),
	// World/Camera
	TEXT("worldposition"), TEXT("objectposition"), TEXT("actorpositionws"), TEXT("objectpositionws"),
	TEXT("camerapositionws"), TEXT("cameravector"), TEXT("reflectionvector"), TEXT("lightvector"),
	TEXT("pixeldepth"), TEXT("scenedepth"), TEXT("screenposition"), TEXT("viewsize"), TEXT("scenetexelsize"),
	// Particle/VFX (Tier 0 - zero property)
	TEXT("particlecolor"), TEXT("particlesubuv"), TEXT("dynamicparameter"), TEXT("vertexcolor"),
	TEXT("depthfade"), TEXT("spheremask"),
	TEXT("particlepositionws"), TEXT("particleradius"), TEXT("particlerelativetime"), TEXT("particlerandom"),
	TEXT("particledirection"), TEXT("particlespeed"), TEXT("particlesize"), TEXT("particlemotionblurfade"),
	TEXT("perinstancefadeamount"), TEXT("perinstancerandom"),
	TEXT("objectorientation"), TEXT("objectradius"),
	TEXT("preskinnedposition"), TEXT("preskinnednormal"),
	TEXT("vertexnormalws"), TEXT("pixelnormalws"), TEXT("vertextangentws"), TEXT("twosidedsign"),
	TEXT("eyeadaptation"), TEXT("atmosphericfogcolor"), TEXT("precomputedaomask"), TEXT("gireplace"),
	// Switch expressions (Tier 1)
	TEXT("qualityswitch"), TEXT("shadingpathswitch"), TEXT("featurelevelswitch"),
	// Static expressions (Tier 2)
	TEXT("staticswitch"),
	// MaterialFunctionCall
	TEXT("materialfunctioncall"),
	// Sobol
	TEXT("sobol"), TEXT("temporalsobol"),
	// Scene
	TEXT("scenetexture"),
	// Complex
	TEXT("fresnel"), TEXT("noise"), TEXT("makematerialattributes"),
	TEXT("if"), TEXT("customexpression"),
};

// Shared validation helper used by both Material and MaterialFunction generators
FMaterialExprValidationResult FMaterialGenerator::ValidateExpressionsAndConnections(
	const FString& AssetName,
	const TArray<FManifestMaterialExpression>& Expressions,
	const TArray<FManifestMaterialConnection>& Connections)
{
	FMaterialExprValidationResult ValidationResult;
	ValidationResult.bValidated = true;

	// Build expression ID set for duplicate and reference checking
	TSet<FString> DefinedExpressionIds;
	TSet<FString> ReferencedExpressionIds;

	// =========================================================================
	// Pass 1: Validate each expression
	// =========================================================================
	for (const FManifestMaterialExpression& ExprDef : Expressions)
	{
		// Guardrail #3: Build context path
		FString ContextPath = FMaterialExprValidationResult::MakeContextPath(AssetName, TEXT("Expressions"), ExprDef.Id);

		// Check for duplicate expression ID
		if (DefinedExpressionIds.Contains(ExprDef.Id))
		{
			FMaterialExprDiagnostic Diag(
				EMaterialExprValidationCode::E_DUPLICATE_EXPRESSION_ID,
				ContextPath,
				FString::Printf(TEXT("Duplicate expression ID '%s'"), *ExprDef.Id),
				true);
			Diag.SuggestedFix = TEXT("Use a unique ID for each expression");
			ValidationResult.AddDiagnostic(Diag);
			continue;
		}
		DefinedExpressionIds.Add(ExprDef.Id);

		// Guardrail #5a: Normalize the type for comparison
		FString TypeLower = FMaterialExprValidationResult::NormalizeKey(ExprDef.Type);

		// Validate expression type is known
		if (!KnownExpressionTypes.Contains(TypeLower))
		{
			FMaterialExprDiagnostic Diag(
				EMaterialExprValidationCode::E_UNKNOWN_EXPRESSION_TYPE,
				ContextPath,
				ExprDef.Type,  // Guardrail #2: User's original input for case hint
				FString::Printf(TEXT("Unknown expression type '%s'"), *ExprDef.Type),
				true);
			Diag.SuggestedFix = TEXT("Check spelling and case. See VFX_System_Reference.md for valid types.");
			ValidationResult.AddDiagnostic(Diag);
			continue;
		}

		// =====================================================================
		// Validate Switch expressions (QualitySwitch, ShadingPathSwitch, FeatureLevelSwitch)
		// =====================================================================
		if (TypeLower == TEXT("qualityswitch"))
		{
			bool bHasDefault = false;
			for (const auto& InputPair : ExprDef.Inputs)
			{
				FString KeyLower = FMaterialExprValidationResult::NormalizeKey(InputPair.Key);
				if (KeyLower == TEXT("default"))
				{
					bHasDefault = true;
				}
				else if (!QualitySwitchKeyMap.Contains(KeyLower))
				{
					FMaterialExprDiagnostic Diag(
						EMaterialExprValidationCode::E_UNKNOWN_SWITCH_KEY,
						ContextPath,
						InputPair.Key,  // Original user input
						FString::Printf(TEXT("Unknown QualitySwitch key '%s'. Valid: default, low, high, medium, epic"), *InputPair.Key),
						true);
					ValidationResult.AddDiagnostic(Diag);
				}
				// Track referenced expression
				ReferencedExpressionIds.Add(InputPair.Value);
			}
			// Check for required default input
			if (!bHasDefault)
			{
				FMaterialExprDiagnostic Diag(
					EMaterialExprValidationCode::E_MISSING_REQUIRED_INPUT,
					ContextPath,
					TEXT("QualitySwitch requires 'default' input"),
					true);
				Diag.SuggestedFix = TEXT("Add 'default: <expression_id>' to switch inputs");
				ValidationResult.AddDiagnostic(Diag);
			}
		}
		else if (TypeLower == TEXT("shadingpathswitch"))
		{
			bool bHasDefault = false;
			for (const auto& InputPair : ExprDef.Inputs)
			{
				FString KeyLower = FMaterialExprValidationResult::NormalizeKey(InputPair.Key);
				if (KeyLower == TEXT("default"))
				{
					bHasDefault = true;
				}
				else if (!ShadingPathSwitchKeyMap.Contains(KeyLower))
				{
					FMaterialExprDiagnostic Diag(
						EMaterialExprValidationCode::E_UNKNOWN_SWITCH_KEY,
						ContextPath,
						InputPair.Key,
						FString::Printf(TEXT("Unknown ShadingPathSwitch key '%s'. Valid: default, deferred, forward, mobile"), *InputPair.Key),
						true);
					ValidationResult.AddDiagnostic(Diag);
				}
				ReferencedExpressionIds.Add(InputPair.Value);
			}
			// Check for required default input
			if (!bHasDefault)
			{
				FMaterialExprDiagnostic Diag(
					EMaterialExprValidationCode::E_MISSING_REQUIRED_INPUT,
					ContextPath,
					TEXT("ShadingPathSwitch requires 'default' input"),
					true);
				Diag.SuggestedFix = TEXT("Add 'default: <expression_id>' to switch inputs");
				ValidationResult.AddDiagnostic(Diag);
			}
		}
		else if (TypeLower == TEXT("featurelevelswitch"))
		{
			bool bHasDefault = false;
			for (const auto& InputPair : ExprDef.Inputs)
			{
				FString KeyLower = FMaterialExprValidationResult::NormalizeKey(InputPair.Key);
				if (KeyLower == TEXT("default"))
				{
					bHasDefault = true;
				}
				else if (!FeatureLevelSwitchKeyMap.Contains(KeyLower))
				{
					// Check for deprecated keys
					if (KeyLower == TEXT("es2") || KeyLower == TEXT("sm4"))
					{
						FMaterialExprDiagnostic Diag(
							EMaterialExprValidationCode::W_DEPRECATED_SWITCH_KEY,
							ContextPath,
							InputPair.Key,
							FString::Printf(TEXT("Deprecated FeatureLevelSwitch key '%s' - removed in UE5"), *InputPair.Key),
							false);  // Warning, not error
						Diag.SuggestedFix = TEXT("Use es3_1, sm5, or sm6 instead");
						ValidationResult.AddDiagnostic(Diag);
					}
					else
					{
						FMaterialExprDiagnostic Diag(
							EMaterialExprValidationCode::E_UNKNOWN_SWITCH_KEY,
							ContextPath,
							InputPair.Key,
							FString::Printf(TEXT("Unknown FeatureLevelSwitch key '%s'. Valid: default, es3_1, sm5, sm6"), *InputPair.Key),
							true);
						ValidationResult.AddDiagnostic(Diag);
					}
				}
				ReferencedExpressionIds.Add(InputPair.Value);
			}
			// Check for required default input
			if (!bHasDefault)
			{
				FMaterialExprDiagnostic Diag(
					EMaterialExprValidationCode::E_MISSING_REQUIRED_INPUT,
					ContextPath,
					TEXT("FeatureLevelSwitch requires 'default' input"),
					true);
				Diag.SuggestedFix = TEXT("Add 'default: <expression_id>' to switch inputs");
				ValidationResult.AddDiagnostic(Diag);
			}
		}

		// =====================================================================
		// Validate MaterialFunctionCall
		// =====================================================================
		if (TypeLower == TEXT("materialfunctioncall"))
		{
			if (ExprDef.Function.IsEmpty())
			{
				FMaterialExprDiagnostic Diag(
					EMaterialExprValidationCode::E_FUNCTION_NOT_FOUND,
					ContextPath,
					TEXT("MaterialFunctionCall requires 'function' property"),
					true);
				ValidationResult.AddDiagnostic(Diag);
			}
			else
			{
				// Guardrail #5c: Normalize path
				FString NormalizedPath = FMaterialExprValidationResult::NormalizePath(ExprDef.Function);
				if (NormalizedPath != ExprDef.Function)
				{
					FMaterialExprDiagnostic Diag(
						EMaterialExprValidationCode::W_FUNCTION_PATH_NORMALIZED,
						ContextPath,
						ExprDef.Function,
						FString::Printf(TEXT("Function path normalized from '%s' to '%s'"), *ExprDef.Function, *NormalizedPath),
						false);
					ValidationResult.AddDiagnostic(Diag);
				}

				// Check for UMaterialFunctionInstance (not supported - use UMaterialFunction)
				if (NormalizedPath.Contains(TEXT("_Inst")) || NormalizedPath.Contains(TEXT("FunctionInstance")))
				{
					FMaterialExprDiagnostic Diag(
						EMaterialExprValidationCode::E_FUNCTION_INSTANCE_BLOCKED,
						ContextPath,
						ExprDef.Function,
						TEXT("UMaterialFunctionInstance not supported. Use UMaterialFunction path instead."),
						true);
					Diag.SuggestedFix = TEXT("Reference the base MaterialFunction, not a MaterialFunctionInstance");
					ValidationResult.AddDiagnostic(Diag);
				}
			}

			// Track referenced expressions in function inputs
			for (const auto& FuncInput : ExprDef.FunctionInputs)
			{
				ReferencedExpressionIds.Add(FuncInput.Value);
			}
		}

		// Track referenced expressions in StaticSwitch
		if (TypeLower == TEXT("staticswitch"))
		{
			if (const FString* AInput = ExprDef.Properties.Find(TEXT("a")))
			{
				ReferencedExpressionIds.Add(*AInput);
			}
			if (const FString* BInput = ExprDef.Properties.Find(TEXT("b")))
			{
				ReferencedExpressionIds.Add(*BInput);
			}
			if (const FString* ValueInput = ExprDef.Properties.Find(TEXT("value")))
			{
				ReferencedExpressionIds.Add(*ValueInput);
			}
		}
	}

	// =========================================================================
	// Pass 2: Validate connections
	// =========================================================================
	for (const FManifestMaterialConnection& Conn : Connections)
	{
		FString ContextPath = FMaterialExprValidationResult::MakeContextPath(AssetName, TEXT("Connections"),
			FString::Printf(TEXT("%s->%s"), *Conn.FromId, *Conn.ToId));

		// Check source expression exists
		if (Conn.FromId != TEXT("Material") && !DefinedExpressionIds.Contains(Conn.FromId))
		{
			FMaterialExprDiagnostic Diag(
				EMaterialExprValidationCode::E_EXPRESSION_REFERENCE_INVALID,
				ContextPath,
				FString::Printf(TEXT("Connection source '%s' not defined in expressions"), *Conn.FromId),
				true);
			ValidationResult.AddDiagnostic(Diag);
		}

		// Check target expression exists (unless it's Material output)
		if (Conn.ToId != TEXT("Material") && !DefinedExpressionIds.Contains(Conn.ToId))
		{
			FMaterialExprDiagnostic Diag(
				EMaterialExprValidationCode::E_EXPRESSION_REFERENCE_INVALID,
				ContextPath,
				FString::Printf(TEXT("Connection target '%s' not defined in expressions"), *Conn.ToId),
				true);
			ValidationResult.AddDiagnostic(Diag);
		}

		// Track usage
		ReferencedExpressionIds.Add(Conn.FromId);
	}

	// =========================================================================
	// Pass 3: Check for unused expressions (warning only)
	// =========================================================================
	for (const FString& DefinedId : DefinedExpressionIds)
	{
		if (!ReferencedExpressionIds.Contains(DefinedId))
		{
			// Check if this expression connects to Material output
			bool bConnectsToMaterial = false;
			for (const FManifestMaterialConnection& Conn : Connections)
			{
				if (Conn.FromId == DefinedId && Conn.ToId == TEXT("Material"))
				{
					bConnectsToMaterial = true;
					break;
				}
			}

			if (!bConnectsToMaterial)
			{
				FString ContextPath = FMaterialExprValidationResult::MakeContextPath(AssetName, TEXT("Expressions"), DefinedId);
				FMaterialExprDiagnostic Diag(
					EMaterialExprValidationCode::W_EXPRESSION_UNUSED,
					ContextPath,
					FString::Printf(TEXT("Expression '%s' defined but never connected"), *DefinedId),
					false);
				ValidationResult.AddDiagnostic(Diag);
			}
		}
	}

	// Guardrail #6: Sort diagnostics for stable output
	ValidationResult.SortDiagnostics();

	return ValidationResult;
}

// Wrapper for material definition validation
FMaterialExprValidationResult FMaterialGenerator::ValidateMaterialDefinition(const FManifestMaterialDefinition& Definition)
{
	return ValidateExpressionsAndConnections(Definition.Name, Definition.Expressions, Definition.Connections);
}

// v4.10.1: Overload for material functions - includes inputs/outputs as valid connection endpoints
// Maintains ALL 6 guardrails from the original function
FMaterialExprValidationResult FMaterialGenerator::ValidateExpressionsAndConnections(
	const FString& AssetName,
	const TArray<FManifestMaterialExpression>& Expressions,
	const TArray<FManifestMaterialConnection>& Connections,
	const TArray<FManifestMaterialFunctionInput>& Inputs,
	const TArray<FManifestMaterialFunctionOutput>& Outputs)
{
	FMaterialExprValidationResult ValidationResult;
	ValidationResult.bValidated = true;

	// Build expression ID set for duplicate and reference checking
	TSet<FString> DefinedExpressionIds;
	TSet<FString> ReferencedExpressionIds;

	// v4.10.1: Pre-populate with function inputs as valid connection endpoints
	for (const FManifestMaterialFunctionInput& Input : Inputs)
	{
		DefinedExpressionIds.Add(Input.Name);
	}

	// v4.10.1: Pre-populate with function outputs as valid connection endpoints
	for (const FManifestMaterialFunctionOutput& Output : Outputs)
	{
		DefinedExpressionIds.Add(Output.Name);
	}

	// =========================================================================
	// Pass 1: Validate each expression (ALL guardrails from original)
	// =========================================================================
	for (const FManifestMaterialExpression& ExprDef : Expressions)
	{
		// Guardrail #3: Build context path
		FString ContextPath = FMaterialExprValidationResult::MakeContextPath(AssetName, TEXT("Expressions"), ExprDef.Id);

		// Check for duplicate expression ID
		if (DefinedExpressionIds.Contains(ExprDef.Id))
		{
			FMaterialExprDiagnostic Diag(
				EMaterialExprValidationCode::E_DUPLICATE_EXPRESSION_ID,
				ContextPath,
				FString::Printf(TEXT("Duplicate expression ID '%s'"), *ExprDef.Id),
				true);
			Diag.SuggestedFix = TEXT("Use a unique ID for each expression");
			ValidationResult.AddDiagnostic(Diag);
			continue;
		}
		DefinedExpressionIds.Add(ExprDef.Id);

		// Guardrail #5a: Normalize the type for comparison
		FString TypeLower = FMaterialExprValidationResult::NormalizeKey(ExprDef.Type);

		// Validate expression type is known
		if (!KnownExpressionTypes.Contains(TypeLower))
		{
			FMaterialExprDiagnostic Diag(
				EMaterialExprValidationCode::E_UNKNOWN_EXPRESSION_TYPE,
				ContextPath,
				ExprDef.Type,  // Guardrail #2: User's original input for case hint
				FString::Printf(TEXT("Unknown expression type '%s'"), *ExprDef.Type),
				true);
			Diag.SuggestedFix = TEXT("Check spelling and case. See VFX_System_Reference.md for valid types.");
			ValidationResult.AddDiagnostic(Diag);
			continue;
		}

		// =====================================================================
		// Validate Switch expressions (QualitySwitch, ShadingPathSwitch, FeatureLevelSwitch)
		// =====================================================================
		if (TypeLower == TEXT("qualityswitch"))
		{
			bool bHasDefault = false;
			for (const auto& InputPair : ExprDef.Inputs)
			{
				FString KeyLower = FMaterialExprValidationResult::NormalizeKey(InputPair.Key);
				if (KeyLower == TEXT("default"))
				{
					bHasDefault = true;
				}
				else if (!QualitySwitchKeyMap.Contains(KeyLower))
				{
					FMaterialExprDiagnostic Diag(
						EMaterialExprValidationCode::E_UNKNOWN_SWITCH_KEY,
						ContextPath,
						InputPair.Key,  // Original user input
						FString::Printf(TEXT("Unknown QualitySwitch key '%s'. Valid: default, low, high, medium, epic"), *InputPair.Key),
						true);
					ValidationResult.AddDiagnostic(Diag);
				}
				// Track referenced expression
				ReferencedExpressionIds.Add(InputPair.Value);
			}
			// Check for required default input
			if (!bHasDefault)
			{
				FMaterialExprDiagnostic Diag(
					EMaterialExprValidationCode::E_MISSING_REQUIRED_INPUT,
					ContextPath,
					TEXT("QualitySwitch requires 'default' input"),
					true);
				Diag.SuggestedFix = TEXT("Add 'default: <expression_id>' to switch inputs");
				ValidationResult.AddDiagnostic(Diag);
			}
		}
		else if (TypeLower == TEXT("shadingpathswitch"))
		{
			bool bHasDefault = false;
			for (const auto& InputPair : ExprDef.Inputs)
			{
				FString KeyLower = FMaterialExprValidationResult::NormalizeKey(InputPair.Key);
				if (KeyLower == TEXT("default"))
				{
					bHasDefault = true;
				}
				else if (!ShadingPathSwitchKeyMap.Contains(KeyLower))
				{
					FMaterialExprDiagnostic Diag(
						EMaterialExprValidationCode::E_UNKNOWN_SWITCH_KEY,
						ContextPath,
						InputPair.Key,
						FString::Printf(TEXT("Unknown ShadingPathSwitch key '%s'. Valid: default, deferred, forward, mobile"), *InputPair.Key),
						true);
					ValidationResult.AddDiagnostic(Diag);
				}
				ReferencedExpressionIds.Add(InputPair.Value);
			}
			// Check for required default input
			if (!bHasDefault)
			{
				FMaterialExprDiagnostic Diag(
					EMaterialExprValidationCode::E_MISSING_REQUIRED_INPUT,
					ContextPath,
					TEXT("ShadingPathSwitch requires 'default' input"),
					true);
				Diag.SuggestedFix = TEXT("Add 'default: <expression_id>' to switch inputs");
				ValidationResult.AddDiagnostic(Diag);
			}
		}
		else if (TypeLower == TEXT("featurelevelswitch"))
		{
			bool bHasDefault = false;
			for (const auto& InputPair : ExprDef.Inputs)
			{
				FString KeyLower = FMaterialExprValidationResult::NormalizeKey(InputPair.Key);
				if (KeyLower == TEXT("default"))
				{
					bHasDefault = true;
				}
				else if (!FeatureLevelSwitchKeyMap.Contains(KeyLower))
				{
					// Check for deprecated keys
					if (KeyLower == TEXT("es2") || KeyLower == TEXT("sm4"))
					{
						FMaterialExprDiagnostic Diag(
							EMaterialExprValidationCode::W_DEPRECATED_SWITCH_KEY,
							ContextPath,
							InputPair.Key,
							FString::Printf(TEXT("Deprecated FeatureLevelSwitch key '%s' - removed in UE5"), *InputPair.Key),
							false);  // Warning, not error
						Diag.SuggestedFix = TEXT("Use es3_1, sm5, or sm6 instead");
						ValidationResult.AddDiagnostic(Diag);
					}
					else
					{
						FMaterialExprDiagnostic Diag(
							EMaterialExprValidationCode::E_UNKNOWN_SWITCH_KEY,
							ContextPath,
							InputPair.Key,
							FString::Printf(TEXT("Unknown FeatureLevelSwitch key '%s'. Valid: default, es3_1, sm5, sm6"), *InputPair.Key),
							true);
						ValidationResult.AddDiagnostic(Diag);
					}
				}
				ReferencedExpressionIds.Add(InputPair.Value);
			}
			// Check for required default input
			if (!bHasDefault)
			{
				FMaterialExprDiagnostic Diag(
					EMaterialExprValidationCode::E_MISSING_REQUIRED_INPUT,
					ContextPath,
					TEXT("FeatureLevelSwitch requires 'default' input"),
					true);
				Diag.SuggestedFix = TEXT("Add 'default: <expression_id>' to switch inputs");
				ValidationResult.AddDiagnostic(Diag);
			}
		}

		// =====================================================================
		// Validate MaterialFunctionCall
		// =====================================================================
		if (TypeLower == TEXT("materialfunctioncall"))
		{
			if (ExprDef.Function.IsEmpty())
			{
				FMaterialExprDiagnostic Diag(
					EMaterialExprValidationCode::E_FUNCTION_NOT_FOUND,
					ContextPath,
					TEXT("MaterialFunctionCall requires 'function' property"),
					true);
				ValidationResult.AddDiagnostic(Diag);
			}
			else
			{
				// Guardrail #5c: Normalize path
				FString NormalizedPath = FMaterialExprValidationResult::NormalizePath(ExprDef.Function);
				if (NormalizedPath != ExprDef.Function)
				{
					FMaterialExprDiagnostic Diag(
						EMaterialExprValidationCode::W_FUNCTION_PATH_NORMALIZED,
						ContextPath,
						ExprDef.Function,
						FString::Printf(TEXT("Function path normalized from '%s' to '%s'"), *ExprDef.Function, *NormalizedPath),
						false);
					ValidationResult.AddDiagnostic(Diag);
				}

				// Check for UMaterialFunctionInstance (not supported - use UMaterialFunction)
				if (NormalizedPath.Contains(TEXT("_Inst")) || NormalizedPath.Contains(TEXT("FunctionInstance")))
				{
					FMaterialExprDiagnostic Diag(
						EMaterialExprValidationCode::E_FUNCTION_INSTANCE_BLOCKED,
						ContextPath,
						ExprDef.Function,
						TEXT("UMaterialFunctionInstance not supported. Use UMaterialFunction path instead."),
						true);
					Diag.SuggestedFix = TEXT("Reference the base MaterialFunction, not a MaterialFunctionInstance");
					ValidationResult.AddDiagnostic(Diag);
				}
			}

			// Track referenced expressions in function inputs
			for (const auto& FuncInput : ExprDef.FunctionInputs)
			{
				ReferencedExpressionIds.Add(FuncInput.Value);
			}
		}

		// Track referenced expressions in StaticSwitch
		if (TypeLower == TEXT("staticswitch"))
		{
			if (const FString* AInput = ExprDef.Properties.Find(TEXT("a")))
			{
				ReferencedExpressionIds.Add(*AInput);
			}
			if (const FString* BInput = ExprDef.Properties.Find(TEXT("b")))
			{
				ReferencedExpressionIds.Add(*BInput);
			}
			if (const FString* ValueInput = ExprDef.Properties.Find(TEXT("value")))
			{
				ReferencedExpressionIds.Add(*ValueInput);
			}
		}
	}

	// =========================================================================
	// Pass 2: Validate connections
	// =========================================================================
	for (const FManifestMaterialConnection& Conn : Connections)
	{
		FString ContextPath = FMaterialExprValidationResult::MakeContextPath(AssetName, TEXT("Connections"),
			FString::Printf(TEXT("%s->%s"), *Conn.FromId, *Conn.ToId));

		// Check source expression exists (includes inputs for material functions)
		if (Conn.FromId != TEXT("Material") && !DefinedExpressionIds.Contains(Conn.FromId))
		{
			FMaterialExprDiagnostic Diag(
				EMaterialExprValidationCode::E_EXPRESSION_REFERENCE_INVALID,
				ContextPath,
				FString::Printf(TEXT("Connection source '%s' not defined in expressions"), *Conn.FromId),
				true);
			ValidationResult.AddDiagnostic(Diag);
		}

		// Check target expression exists (includes outputs for material functions)
		if (Conn.ToId != TEXT("Material") && !DefinedExpressionIds.Contains(Conn.ToId))
		{
			FMaterialExprDiagnostic Diag(
				EMaterialExprValidationCode::E_EXPRESSION_REFERENCE_INVALID,
				ContextPath,
				FString::Printf(TEXT("Connection target '%s' not defined in expressions"), *Conn.ToId),
				true);
			ValidationResult.AddDiagnostic(Diag);
		}

		// Track usage
		ReferencedExpressionIds.Add(Conn.FromId);
	}

	// =========================================================================
	// Pass 3: Check for unused expressions (warning only)
	// Skip inputs/outputs as they are endpoints, not intermediate expressions
	// =========================================================================
	TSet<FString> InputOutputNames;
	for (const FManifestMaterialFunctionInput& Input : Inputs) InputOutputNames.Add(Input.Name);
	for (const FManifestMaterialFunctionOutput& Output : Outputs) InputOutputNames.Add(Output.Name);

	for (const FString& DefinedId : DefinedExpressionIds)
	{
		// Skip inputs/outputs - they are endpoints
		if (InputOutputNames.Contains(DefinedId)) continue;

		if (!ReferencedExpressionIds.Contains(DefinedId))
		{
			// Check if this expression connects to an output
			bool bConnectsToOutput = false;
			for (const FManifestMaterialConnection& Conn : Connections)
			{
				if (Conn.FromId == DefinedId && (Conn.ToId == TEXT("Material") || InputOutputNames.Contains(Conn.ToId)))
				{
					bConnectsToOutput = true;
					break;
				}
			}

			if (!bConnectsToOutput)
			{
				FString ContextPath = FMaterialExprValidationResult::MakeContextPath(AssetName, TEXT("Expressions"), DefinedId);
				FMaterialExprDiagnostic Diag(
					EMaterialExprValidationCode::W_EXPRESSION_UNUSED,
					ContextPath,
					FString::Printf(TEXT("Expression '%s' defined but never connected"), *DefinedId),
					false);
				ValidationResult.AddDiagnostic(Diag);
			}
		}
	}

	// Guardrail #6: Sort diagnostics for stable output
	ValidationResult.SortDiagnostics();

	return ValidationResult;
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

	// v4.10: Pre-generation validation with 6 guardrails
	FMaterialExprValidationResult ValidationResult = ValidateMaterialDefinition(Definition);
	if (ValidationResult.HasErrors())
	{
		// Log all diagnostics (sorted for stable output - Guardrail #6)
		LogGeneration(FString::Printf(TEXT("Material %s: Validation failed with %d errors, %d warnings"),
			*Definition.Name, ValidationResult.ErrorCount, ValidationResult.WarningCount));
		for (const FMaterialExprDiagnostic& Diag : ValidationResult.Diagnostics)
		{
			LogGeneration(FString::Printf(TEXT("  %s"), *Diag.ToString()));
		}
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Validation failed: %s"), *ValidationResult.GetSummary()));
	}
	else if (ValidationResult.HasWarnings())
	{
		// Log warnings but continue generation
		LogGeneration(FString::Printf(TEXT("Material %s: %d validation warnings"), *Definition.Name, ValidationResult.WarningCount));
		for (const FMaterialExprDiagnostic& Diag : ValidationResult.Diagnostics)
		{
			LogGeneration(FString::Printf(TEXT("  %s"), *Diag.ToString()));
		}
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

	// ========================================================================
	// v4.0: Extended material properties
	// ========================================================================

	// Material Domain
	if (!Definition.MaterialDomain.IsEmpty())
	{
		if (Definition.MaterialDomain.Equals(TEXT("Surface"), ESearchCase::IgnoreCase))
			Material->MaterialDomain = MD_Surface;
		else if (Definition.MaterialDomain.Equals(TEXT("DeferredDecal"), ESearchCase::IgnoreCase))
			Material->MaterialDomain = MD_DeferredDecal;
		else if (Definition.MaterialDomain.Equals(TEXT("LightFunction"), ESearchCase::IgnoreCase))
			Material->MaterialDomain = MD_LightFunction;
		else if (Definition.MaterialDomain.Equals(TEXT("Volume"), ESearchCase::IgnoreCase))
			Material->MaterialDomain = MD_Volume;
		else if (Definition.MaterialDomain.Equals(TEXT("PostProcess"), ESearchCase::IgnoreCase))
			Material->MaterialDomain = MD_PostProcess;
		else if (Definition.MaterialDomain.Equals(TEXT("UI"), ESearchCase::IgnoreCase))
			Material->MaterialDomain = MD_UI;
	}

	// Opacity mask clip value (for Masked blend mode)
	if (Material->BlendMode == BLEND_Masked)
	{
		Material->OpacityMaskClipValue = Definition.OpacityMaskClipValue;
	}

	// Translucency settings
	if (!Definition.TranslucencyPass.IsEmpty())
	{
		if (Definition.TranslucencyPass.Equals(TEXT("BeforeDOF"), ESearchCase::IgnoreCase))
			Material->TranslucencyPass = MTP_BeforeDOF;
		else if (Definition.TranslucencyPass.Equals(TEXT("AfterDOF"), ESearchCase::IgnoreCase))
			Material->TranslucencyPass = MTP_AfterDOF;
		else if (Definition.TranslucencyPass.Equals(TEXT("AfterMotionBlur"), ESearchCase::IgnoreCase))
			Material->TranslucencyPass = MTP_AfterMotionBlur;
	}

	// Separate translucency (if property exists)
	if (Definition.bEnableSeparateTranslucency)
	{
		// Set via reflection since property name may vary by UE version
		if (FBoolProperty* SepTransProp = CastField<FBoolProperty>(Material->GetClass()->FindPropertyByName(TEXT("bEnableSeparateTranslucency"))))
		{
			SepTransProp->SetPropertyValue(SepTransProp->ContainerPtrToValuePtr<void>(Material), true);
		}
	}

	// Responsive AA
	if (Definition.bEnableResponsiveAA)
	{
		if (FBoolProperty* ResponsiveAAProp = CastField<FBoolProperty>(Material->GetClass()->FindPropertyByName(TEXT("bEnableResponsiveAA"))))
		{
			ResponsiveAAProp->SetPropertyValue(ResponsiveAAProp->ContainerPtrToValuePtr<void>(Material), true);
		}
	}

	// Decal response
	if (!Definition.DecalResponse.IsEmpty())
	{
		if (Definition.DecalResponse.Equals(TEXT("None"), ESearchCase::IgnoreCase))
			Material->MaterialDecalResponse = MDR_None;
		else if (Definition.DecalResponse.Equals(TEXT("Color"), ESearchCase::IgnoreCase))
			Material->MaterialDecalResponse = MDR_Color;
		else if (Definition.DecalResponse.Equals(TEXT("ColorNormalRoughness"), ESearchCase::IgnoreCase))
			Material->MaterialDecalResponse = MDR_ColorNormalRoughness;
	}

	// Shadow casting
	if (!Definition.bCastDynamicShadow)
	{
		Material->bCastDynamicShadowAsMasked = false;
	}

	// Indirect lighting
	if (!Definition.bAffectDynamicIndirectLighting)
	{
		if (FBoolProperty* IndirectProp = CastField<FBoolProperty>(Material->GetClass()->FindPropertyByName(TEXT("bAffectDynamicIndirectLighting"))))
		{
			IndirectProp->SetPropertyValue(IndirectProp->ContainerPtrToValuePtr<void>(Material), false);
		}
	}

	// Block GI
	if (Definition.bBlockGI)
	{
		if (FBoolProperty* BlockGIProp = CastField<FBoolProperty>(Material->GetClass()->FindPropertyByName(TEXT("bBlockGI"))))
		{
			BlockGIProp->SetPropertyValue(BlockGIProp->ContainerPtrToValuePtr<void>(Material), true);
		}
	}

	// v2.6.12: Create expressions (PASS 1 - create all expression nodes)
	TMap<FString, UMaterialExpression*> ExpressionMap;
	for (const FManifestMaterialExpression& ExprDef : Definition.Expressions)
	{
		// v4.10: Pass ExpressionMap for switch/function input resolution
		UMaterialExpression* Expr = CreateExpression(Material, ExprDef, ExpressionMap);
		if (Expr)
		{
			ExpressionMap.Add(ExprDef.Id, Expr);
			LogGeneration(FString::Printf(TEXT("  Created expression: %s (%s)"), *ExprDef.Id, *ExprDef.Type));
		}
		else
		{
			// v4.23 FAIL-FAST: Expression creation failed (logged in CreateExpression)
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("Failed to create expression '%s' (%s)"), *ExprDef.Id, *ExprDef.Type));
		}
	}

	// v4.10: PASS 2 - Connect Switch expression inputs and MaterialFunctionCall inputs
	for (const FManifestMaterialExpression& ExprDef : Definition.Expressions)
	{
		FString TypeLower = ExprDef.Type.ToLower();
		UMaterialExpression* Expr = ExpressionMap.FindRef(ExprDef.Id);
		if (!Expr) continue;

		// Connect switch expression inputs
		if (TypeLower == TEXT("qualityswitch") || TypeLower == TEXT("quality_switch") ||
			TypeLower == TEXT("shadingpathswitch") || TypeLower == TEXT("shading_path_switch") ||
			TypeLower == TEXT("featurelevelswitch") || TypeLower == TEXT("feature_level_switch"))
		{
			if (ConnectSwitchInputs(Expr, ExprDef, ExpressionMap))
			{
				LogGeneration(FString::Printf(TEXT("    Connected switch inputs for: %s"), *ExprDef.Id));
			}
		}
		// Connect function call inputs
		else if (TypeLower == TEXT("materialfunctioncall") || TypeLower == TEXT("function_call") || TypeLower == TEXT("functioncall"))
		{
			UMaterialExpressionMaterialFunctionCall* FuncCall = Cast<UMaterialExpressionMaterialFunctionCall>(Expr);
			if (FuncCall && ConnectFunctionInputs(FuncCall, ExprDef, ExpressionMap))
			{
				LogGeneration(FString::Printf(TEXT("    Connected function inputs for: %s"), *ExprDef.Id));
			}
		}
	}

	// v2.6.12: Create connections (PASS 3 - general expression connections)
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

	// v4.9.1: Register in session cache for MIC parent lookup
	RegisterGeneratedMaterial(Definition.Name, Material);

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("Material");
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// FMaterialFunctionGenerator Implementation (v2.6.12)
// v4.10: Updated with ExpressionMap parameter for switch/function input connections
// ============================================================================

UMaterialExpression* FMaterialFunctionGenerator::CreateExpressionInFunction(UMaterialFunction* MaterialFunction, const FManifestMaterialExpression& ExprDef, const TMap<FString, UMaterialExpression*>& ExpressionMap)
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
	// v4.1: Additional arithmetic expressions
	// Subtract
	else if (TypeLower == TEXT("subtract") || TypeLower == TEXT("sub"))
	{
		Expression = NewObject<UMaterialExpressionSubtract>(MaterialFunction);
	}
	// Divide
	else if (TypeLower == TEXT("divide") || TypeLower == TEXT("div"))
	{
		Expression = NewObject<UMaterialExpressionDivide>(MaterialFunction);
	}
	// Clamp
	else if (TypeLower == TEXT("clamp"))
	{
		UMaterialExpressionClamp* Clamp = NewObject<UMaterialExpressionClamp>(MaterialFunction);
		if (ExprDef.Properties.Contains(TEXT("min")))
		{
			Clamp->MinDefault = FCString::Atof(*ExprDef.Properties[TEXT("min")]);
		}
		if (ExprDef.Properties.Contains(TEXT("max")))
		{
			Clamp->MaxDefault = FCString::Atof(*ExprDef.Properties[TEXT("max")]);
		}
		Expression = Clamp;
	}
	// OneMinus
	else if (TypeLower == TEXT("oneminus") || TypeLower == TEXT("invert"))
	{
		Expression = NewObject<UMaterialExpressionOneMinus>(MaterialFunction);
	}
	// LinearInterpolate (Lerp)
	else if (TypeLower == TEXT("lerp") || TypeLower == TEXT("linearinterpolate"))
	{
		UMaterialExpressionLinearInterpolate* Lerp = NewObject<UMaterialExpressionLinearInterpolate>(MaterialFunction);
		if (ExprDef.Properties.Contains(TEXT("alpha")))
		{
			Lerp->ConstAlpha = FCString::Atof(*ExprDef.Properties[TEXT("alpha")]);
		}
		Expression = Lerp;
	}
	// v4.1: Texture expressions
	// TextureCoordinate
	else if (TypeLower == TEXT("texturecoordinate") || TypeLower == TEXT("texcoord") || TypeLower == TEXT("uv"))
	{
		UMaterialExpressionTextureCoordinate* TexCoord = NewObject<UMaterialExpressionTextureCoordinate>(MaterialFunction);
		if (ExprDef.Properties.Contains(TEXT("utiling")))
		{
			TexCoord->UTiling = FCString::Atof(*ExprDef.Properties[TEXT("utiling")]);
		}
		if (ExprDef.Properties.Contains(TEXT("vtiling")))
		{
			TexCoord->VTiling = FCString::Atof(*ExprDef.Properties[TEXT("vtiling")]);
		}
		if (ExprDef.Properties.Contains(TEXT("coordinate_index")))
		{
			TexCoord->CoordinateIndex = FCString::Atoi(*ExprDef.Properties[TEXT("coordinate_index")]);
		}
		Expression = TexCoord;
	}
	// TextureSampleParameter2D (named texture parameter)
	else if (TypeLower == TEXT("texturesampleparameter2d") || TypeLower == TEXT("textureparam") || TypeLower == TEXT("texturesample"))
	{
		UMaterialExpressionTextureSampleParameter2D* TexSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(MaterialFunction);
		TexSample->ParameterName = *ExprDef.Name;
		// Load default texture if specified
		if (!ExprDef.DefaultValue.IsEmpty())
		{
			UTexture* DefaultTex = LoadObject<UTexture>(nullptr, *ExprDef.DefaultValue);
			if (DefaultTex)
			{
				TexSample->Texture = DefaultTex;
			}
		}
		Expression = TexSample;
	}
	// v4.1: Vector expressions
	// VectorParameter
	else if (TypeLower == TEXT("vectorparameter") || TypeLower == TEXT("vectorparam"))
	{
		UMaterialExpressionVectorParameter* VecParam = NewObject<UMaterialExpressionVectorParameter>(MaterialFunction);
		VecParam->ParameterName = *ExprDef.Name;
		// Parse default value as "R,G,B,A" or "R,G,B"
		if (!ExprDef.DefaultValue.IsEmpty())
		{
			TArray<FString> Components;
			ExprDef.DefaultValue.ParseIntoArray(Components, TEXT(","));
			if (Components.Num() >= 3)
			{
				VecParam->DefaultValue.R = FCString::Atof(*Components[0]);
				VecParam->DefaultValue.G = FCString::Atof(*Components[1]);
				VecParam->DefaultValue.B = FCString::Atof(*Components[2]);
				if (Components.Num() >= 4)
				{
					VecParam->DefaultValue.A = FCString::Atof(*Components[3]);
				}
			}
		}
		Expression = VecParam;
	}
	// Constant3Vector
	else if (TypeLower == TEXT("constant3vector") || TypeLower == TEXT("vec3") || TypeLower == TEXT("color"))
	{
		UMaterialExpressionConstant3Vector* Vec3 = NewObject<UMaterialExpressionConstant3Vector>(MaterialFunction);
		// Parse value as "R,G,B"
		if (!ExprDef.DefaultValue.IsEmpty())
		{
			TArray<FString> Components;
			ExprDef.DefaultValue.ParseIntoArray(Components, TEXT(","));
			if (Components.Num() >= 3)
			{
				Vec3->Constant.R = FCString::Atof(*Components[0]);
				Vec3->Constant.G = FCString::Atof(*Components[1]);
				Vec3->Constant.B = FCString::Atof(*Components[2]);
			}
		}
		Expression = Vec3;
	}
	// Constant4Vector
	else if (TypeLower == TEXT("constant4vector") || TypeLower == TEXT("vec4"))
	{
		UMaterialExpressionConstant4Vector* Vec4 = NewObject<UMaterialExpressionConstant4Vector>(MaterialFunction);
		// Parse value as "R,G,B,A"
		if (!ExprDef.DefaultValue.IsEmpty())
		{
			TArray<FString> Components;
			ExprDef.DefaultValue.ParseIntoArray(Components, TEXT(","));
			if (Components.Num() >= 4)
			{
				Vec4->Constant.R = FCString::Atof(*Components[0]);
				Vec4->Constant.G = FCString::Atof(*Components[1]);
				Vec4->Constant.B = FCString::Atof(*Components[2]);
				Vec4->Constant.A = FCString::Atof(*Components[3]);
			}
		}
		Expression = Vec4;
	}
	// ========================================================================
	// v4.10: Tier 0 Zero-Property Expressions
	// ========================================================================
	else if (TypeLower == TEXT("perinstancefadeamount") || TypeLower == TEXT("per_instance_fade"))
	{
		Expression = NewObject<UMaterialExpressionPerInstanceFadeAmount>(MaterialFunction);
	}
	else if (TypeLower == TEXT("perinstancerandom") || TypeLower == TEXT("per_instance_random"))
	{
		Expression = NewObject<UMaterialExpressionPerInstanceRandom>(MaterialFunction);
	}
	else if (TypeLower == TEXT("objectorientation") || TypeLower == TEXT("object_orientation"))
	{
		Expression = NewObject<UMaterialExpressionObjectOrientation>(MaterialFunction);
	}
	else if (TypeLower == TEXT("objectpositionws") || TypeLower == TEXT("object_position_ws") || TypeLower == TEXT("objectposition"))
	{
		Expression = NewObject<UMaterialExpressionObjectPositionWS>(MaterialFunction);
	}
	else if (TypeLower == TEXT("actorpositionws") || TypeLower == TEXT("actor_position_ws") || TypeLower == TEXT("actorposition"))
	{
		Expression = NewObject<UMaterialExpressionActorPositionWS>(MaterialFunction);
	}
	else if (TypeLower == TEXT("objectradius") || TypeLower == TEXT("object_radius"))
	{
		Expression = NewObject<UMaterialExpressionObjectRadius>(MaterialFunction);
	}
	else if (TypeLower == TEXT("screenposition") || TypeLower == TEXT("screen_position"))
	{
		Expression = NewObject<UMaterialExpressionScreenPosition>(MaterialFunction);
	}
	else if (TypeLower == TEXT("viewsize") || TypeLower == TEXT("view_size"))
	{
		Expression = NewObject<UMaterialExpressionViewSize>(MaterialFunction);
	}
	else if (TypeLower == TEXT("scenetexelsize") || TypeLower == TEXT("scene_texel_size"))
	{
		Expression = NewObject<UMaterialExpressionSceneTexelSize>(MaterialFunction);
	}
	else if (TypeLower == TEXT("twosidedsign") || TypeLower == TEXT("two_sided_sign"))
	{
		Expression = NewObject<UMaterialExpressionTwoSidedSign>(MaterialFunction);
	}
	// ========================================================================
	// v4.10: Switch Expressions
	// ========================================================================
	else if (TypeLower == TEXT("qualityswitch") || TypeLower == TEXT("quality_switch"))
	{
		Expression = NewObject<UMaterialExpressionQualitySwitch>(MaterialFunction);
	}
	else if (TypeLower == TEXT("shadingpathswitch") || TypeLower == TEXT("shading_path_switch"))
	{
		Expression = NewObject<UMaterialExpressionShadingPathSwitch>(MaterialFunction);
	}
	else if (TypeLower == TEXT("featurelevelswitch") || TypeLower == TEXT("feature_level_switch"))
	{
		Expression = NewObject<UMaterialExpressionFeatureLevelSwitch>(MaterialFunction);
	}
	// ========================================================================
	// v4.10: MaterialFunctionCall
	// ========================================================================
	else if (TypeLower == TEXT("materialfunctioncall") || TypeLower == TEXT("function_call") || TypeLower == TEXT("functioncall"))
	{
		UMaterialExpressionMaterialFunctionCall* FuncCall = NewObject<UMaterialExpressionMaterialFunctionCall>(MaterialFunction);
		if (!ExprDef.Function.IsEmpty())
		{
			UMaterialFunctionInterface* ResolvedFunc = FMaterialGenerator::ResolveMaterialFunction(ExprDef.Function);
			if (ResolvedFunc)
			{
				FuncCall->SetMaterialFunction(ResolvedFunc);
				LogGeneration(FString::Printf(TEXT("    MaterialFunctionCall '%s' -> resolved to '%s'"), *ExprDef.Id, *ExprDef.Function));
			}
			else
			{
				// v4.23 FAIL-FAST: S42 - Manifest references material function that doesn't exist
				LogGeneration(FString::Printf(TEXT("[E_MF_FUNCTION_NOT_FOUND] MaterialFunctionCall '%s' -> function NOT FOUND: '%s'"), *ExprDef.Id, *ExprDef.Function));
				return nullptr; // Signal failure to caller
			}
		}
		Expression = FuncCall;
	}
	// ========================================================================
	// v4.10: Static Expressions
	// ========================================================================
	else if (TypeLower == TEXT("staticswitch") || TypeLower == TEXT("static_switch"))
	{
		UMaterialExpressionStaticSwitch* StaticSwitch = NewObject<UMaterialExpressionStaticSwitch>(MaterialFunction);
		if (ExprDef.Properties.Contains(TEXT("default_value")))
		{
			StaticSwitch->DefaultValue = ExprDef.Properties[TEXT("default_value")].ToBool();
		}
		Expression = StaticSwitch;
	}
	else if (TypeLower == TEXT("staticbool") || TypeLower == TEXT("static_bool"))
	{
		UMaterialExpressionStaticBool* StaticBool = NewObject<UMaterialExpressionStaticBool>(MaterialFunction);
		if (!ExprDef.DefaultValue.IsEmpty())
		{
			StaticBool->Value = ExprDef.DefaultValue.ToBool();
		}
		Expression = StaticBool;
	}
	else if (TypeLower == TEXT("staticboolparameter") || TypeLower == TEXT("static_bool_param"))
	{
		UMaterialExpressionStaticBoolParameter* StaticBoolParam = NewObject<UMaterialExpressionStaticBoolParameter>(MaterialFunction);
		StaticBoolParam->ParameterName = *ExprDef.Name;
		if (!ExprDef.DefaultValue.IsEmpty())
		{
			StaticBoolParam->DefaultValue = ExprDef.DefaultValue.ToBool();
		}
		Expression = StaticBoolParam;
	}
	// ========================================================================
	// v4.10: Noise/Sampling Expressions
	// ========================================================================
	else if (TypeLower == TEXT("sobol"))
	{
		Expression = NewObject<UMaterialExpressionSobol>(MaterialFunction);
	}
	else if (TypeLower == TEXT("temporalsobol") || TypeLower == TEXT("temporal_sobol"))
	{
		Expression = NewObject<UMaterialExpressionTemporalSobol>(MaterialFunction);
	}
	// ========================================================================
	// v4.22: Vector Operation Expressions (for Material Functions)
	// ========================================================================
	else if (TypeLower == TEXT("distance"))
	{
		Expression = NewObject<UMaterialExpressionDistance>(MaterialFunction);
	}
	else if (TypeLower == TEXT("dotproduct") || TypeLower == TEXT("dot"))
	{
		Expression = NewObject<UMaterialExpressionDotProduct>(MaterialFunction);
	}
	else if (TypeLower == TEXT("crossproduct") || TypeLower == TEXT("cross"))
	{
		Expression = NewObject<UMaterialExpressionCrossProduct>(MaterialFunction);
	}
	else if (TypeLower == TEXT("normalize"))
	{
		Expression = NewObject<UMaterialExpressionNormalize>(MaterialFunction);
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

	// v4.10.1: Pre-generation validation with inputs/outputs as valid connection endpoints
	FMaterialExprValidationResult ValidationResult = FMaterialGenerator::ValidateExpressionsAndConnections(
		Definition.Name, Definition.Expressions, Definition.Connections, Definition.Inputs, Definition.Outputs);
	if (ValidationResult.HasErrors())
	{
		LogGeneration(FString::Printf(TEXT("Material Function %s: Validation failed with %d errors, %d warnings"),
			*Definition.Name, ValidationResult.ErrorCount, ValidationResult.WarningCount));
		for (const FMaterialExprDiagnostic& Diag : ValidationResult.Diagnostics)
		{
			LogGeneration(FString::Printf(TEXT("  %s"), *Diag.ToString()));
		}
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Validation failed: %s"), *ValidationResult.GetSummary()));
	}
	else if (ValidationResult.HasWarnings())
	{
		LogGeneration(FString::Printf(TEXT("Material Function %s: %d validation warnings"), *Definition.Name, ValidationResult.WarningCount));
		for (const FMaterialExprDiagnostic& Diag : ValidationResult.Diagnostics)
		{
			LogGeneration(FString::Printf(TEXT("  %s"), *Diag.ToString()));
		}
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

		// v4.10.1: Set preview/default value if specified
		if (!InputDef.DefaultValue.IsEmpty())
		{
			TArray<FString> Components;
			InputDef.DefaultValue.ParseIntoArray(Components, TEXT(","));

			FVector4f PreviewVal(0, 0, 0, 0);
			if (Components.Num() >= 1) PreviewVal.X = FCString::Atof(*Components[0]);
			if (Components.Num() >= 2) PreviewVal.Y = FCString::Atof(*Components[1]);
			if (Components.Num() >= 3) PreviewVal.Z = FCString::Atof(*Components[2]);
			if (Components.Num() >= 4) PreviewVal.W = FCString::Atof(*Components[3]);

			Input->PreviewValue = PreviewVal;
			Input->bUsePreviewValueAsDefault = true;
		}

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

	// Create other expressions (PASS 1 - create all expression nodes)
	for (const FManifestMaterialExpression& ExprDef : Definition.Expressions)
	{
		// v4.10: Pass ExpressionMap for switch/function input resolution
		UMaterialExpression* Expr = CreateExpressionInFunction(MaterialFunction, ExprDef, ExpressionMap);
		if (Expr)
		{
			ExpressionMap.Add(ExprDef.Id, Expr);
			LogGeneration(FString::Printf(TEXT("  Created expression: %s (%s)"), *ExprDef.Id, *ExprDef.Type));
		}
	}

	// v4.10: PASS 2 - Connect Switch expression inputs and MaterialFunctionCall inputs
	for (const FManifestMaterialExpression& ExprDef : Definition.Expressions)
	{
		FString TypeLower = ExprDef.Type.ToLower();
		UMaterialExpression* Expr = ExpressionMap.FindRef(ExprDef.Id);
		if (!Expr) continue;

		// Connect switch expression inputs (reuse FMaterialGenerator helpers)
		if (TypeLower == TEXT("qualityswitch") || TypeLower == TEXT("quality_switch") ||
			TypeLower == TEXT("shadingpathswitch") || TypeLower == TEXT("shading_path_switch") ||
			TypeLower == TEXT("featurelevelswitch") || TypeLower == TEXT("feature_level_switch"))
		{
			if (FMaterialGenerator::ConnectSwitchInputs(Expr, ExprDef, ExpressionMap))
			{
				LogGeneration(FString::Printf(TEXT("    Connected switch inputs for: %s"), *ExprDef.Id));
			}
		}
		// Connect function call inputs
		else if (TypeLower == TEXT("materialfunctioncall") || TypeLower == TEXT("function_call") || TypeLower == TEXT("functioncall"))
		{
			UMaterialExpressionMaterialFunctionCall* FuncCall = Cast<UMaterialExpressionMaterialFunctionCall>(Expr);
			if (FuncCall && FMaterialGenerator::ConnectFunctionInputs(FuncCall, ExprDef, ExpressionMap))
			{
				LogGeneration(FString::Printf(TEXT("    Connected function inputs for: %s"), *ExprDef.Id));
			}
		}
	}

	// ============================================================================
	// v4.0: MATERIAL FUNCTION CONNECTION WIRING - PASS 3 (fixes critical automation gap)
	// ============================================================================
	int32 ConnectionsWired = 0;
	for (const FManifestMaterialConnection& Conn : Definition.Connections)
	{
		UMaterialExpression* FromExpr = ExpressionMap.FindRef(Conn.FromId);
		UMaterialExpression* ToExpr = ExpressionMap.FindRef(Conn.ToId);

		if (!FromExpr)
		{
			// v4.23 FAIL-FAST: S43 - Manifest references connection source that doesn't exist
			LogGeneration(FString::Printf(TEXT("[E_MF_CONNECTION_SOURCE_NOT_FOUND] %s | Connection source '%s' not found"), *Definition.Name, *Conn.FromId));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("MF connection source '%s' not found"), *Conn.FromId));
		}
		if (!ToExpr)
		{
			// v4.23 FAIL-FAST: S44 - Manifest references connection target that doesn't exist
			LogGeneration(FString::Printf(TEXT("[E_MF_CONNECTION_TARGET_NOT_FOUND] %s | Connection target '%s' not found"), *Definition.Name, *Conn.ToId));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("MF connection target '%s' not found"), *Conn.ToId));
		}

		// Find output index on source expression
		int32 OutputIndex = 0;
		if (!Conn.FromOutput.IsEmpty())
		{
			// Check for common aliases first
			if (Conn.FromOutput.Equals(TEXT("RGB"), ESearchCase::IgnoreCase)) OutputIndex = 0;
			else if (Conn.FromOutput.Equals(TEXT("R"), ESearchCase::IgnoreCase)) OutputIndex = 1;
			else if (Conn.FromOutput.Equals(TEXT("G"), ESearchCase::IgnoreCase)) OutputIndex = 2;
			else if (Conn.FromOutput.Equals(TEXT("B"), ESearchCase::IgnoreCase)) OutputIndex = 3;
			else if (Conn.FromOutput.Equals(TEXT("A"), ESearchCase::IgnoreCase)) OutputIndex = 4;
			else
			{
				// Search for named output
				TArray<FExpressionOutput>& Outputs = FromExpr->GetOutputs();
				for (int32 i = 0; i < Outputs.Num(); i++)
				{
					if (Outputs[i].OutputName.ToString().Equals(Conn.FromOutput, ESearchCase::IgnoreCase))
					{
						OutputIndex = i;
						break;
					}
				}
			}
		}

		// Find and connect input on target expression
		bool bConnected = false;

		// Handle FunctionOutput specially - it has a single input 'A'
		if (UMaterialExpressionFunctionOutput* OutputExpr = Cast<UMaterialExpressionFunctionOutput>(ToExpr))
		{
			OutputExpr->A.Expression = FromExpr;
			OutputExpr->A.OutputIndex = OutputIndex;
			bConnected = true;
		}
		else
		{
			// Generic input connection by name
			FExpressionInput* TargetInput = nullptr;

			// Match input pin name
			if (Conn.ToInput.IsEmpty() || Conn.ToInput.Equals(TEXT("A"), ESearchCase::IgnoreCase))
			{
				// First input
				if (UMaterialExpressionMultiply* Mul = Cast<UMaterialExpressionMultiply>(ToExpr))
					TargetInput = &Mul->A;
				else if (UMaterialExpressionAdd* Add = Cast<UMaterialExpressionAdd>(ToExpr))
					TargetInput = &Add->A;
				else if (UMaterialExpressionSubtract* Sub = Cast<UMaterialExpressionSubtract>(ToExpr))
					TargetInput = &Sub->A;
				else if (UMaterialExpressionDivide* Div = Cast<UMaterialExpressionDivide>(ToExpr))
					TargetInput = &Div->A;
				else if (UMaterialExpressionPower* Pow = Cast<UMaterialExpressionPower>(ToExpr))
					TargetInput = &Pow->Base;
				else if (UMaterialExpressionLinearInterpolate* Lerp = Cast<UMaterialExpressionLinearInterpolate>(ToExpr))
					TargetInput = &Lerp->A;
				else if (UMaterialExpressionClamp* Clamp = Cast<UMaterialExpressionClamp>(ToExpr))
					TargetInput = &Clamp->Input;
				else if (UMaterialExpressionOneMinus* OneMinus = Cast<UMaterialExpressionOneMinus>(ToExpr))
					TargetInput = &OneMinus->Input;
				else if (UMaterialExpressionSine* Sine = Cast<UMaterialExpressionSine>(ToExpr))
					TargetInput = &Sine->Input;
				else if (UMaterialExpressionFresnel* Fresnel = Cast<UMaterialExpressionFresnel>(ToExpr))
					TargetInput = &Fresnel->BaseReflectFractionIn;
			}
			else if (Conn.ToInput.Equals(TEXT("B"), ESearchCase::IgnoreCase))
			{
				// Second input
				if (UMaterialExpressionMultiply* Mul = Cast<UMaterialExpressionMultiply>(ToExpr))
					TargetInput = &Mul->B;
				else if (UMaterialExpressionAdd* Add = Cast<UMaterialExpressionAdd>(ToExpr))
					TargetInput = &Add->B;
				else if (UMaterialExpressionSubtract* Sub = Cast<UMaterialExpressionSubtract>(ToExpr))
					TargetInput = &Sub->B;
				else if (UMaterialExpressionDivide* Div = Cast<UMaterialExpressionDivide>(ToExpr))
					TargetInput = &Div->B;
				else if (UMaterialExpressionPower* Pow = Cast<UMaterialExpressionPower>(ToExpr))
					TargetInput = &Pow->Exponent;
				else if (UMaterialExpressionLinearInterpolate* Lerp = Cast<UMaterialExpressionLinearInterpolate>(ToExpr))
					TargetInput = &Lerp->B;
			}
			else if (Conn.ToInput.Equals(TEXT("Alpha"), ESearchCase::IgnoreCase))
			{
				if (UMaterialExpressionLinearInterpolate* Lerp = Cast<UMaterialExpressionLinearInterpolate>(ToExpr))
					TargetInput = &Lerp->Alpha;
			}
			else if (Conn.ToInput.Equals(TEXT("Min"), ESearchCase::IgnoreCase))
			{
				if (UMaterialExpressionClamp* Clamp = Cast<UMaterialExpressionClamp>(ToExpr))
					TargetInput = &Clamp->Min;
			}
			else if (Conn.ToInput.Equals(TEXT("Max"), ESearchCase::IgnoreCase))
			{
				if (UMaterialExpressionClamp* Clamp = Cast<UMaterialExpressionClamp>(ToExpr))
					TargetInput = &Clamp->Max;
			}
			else if (Conn.ToInput.Equals(TEXT("Coordinate"), ESearchCase::IgnoreCase))
			{
				if (UMaterialExpressionPanner* Panner = Cast<UMaterialExpressionPanner>(ToExpr))
					TargetInput = &Panner->Coordinate;
			}
			else if (Conn.ToInput.Equals(TEXT("Time"), ESearchCase::IgnoreCase))
			{
				if (UMaterialExpressionPanner* Panner = Cast<UMaterialExpressionPanner>(ToExpr))
					TargetInput = &Panner->Time;
			}
			else if (Conn.ToInput.Equals(TEXT("Exponent"), ESearchCase::IgnoreCase) ||
			         Conn.ToInput.Equals(TEXT("ExponentIn"), ESearchCase::IgnoreCase))
			{
				if (UMaterialExpressionFresnel* Fresnel = Cast<UMaterialExpressionFresnel>(ToExpr))
					TargetInput = &Fresnel->ExponentIn;
			}
			// v4.1: Additional input connections
			else if (Conn.ToInput.Equals(TEXT("Input"), ESearchCase::IgnoreCase))
			{
				// Generic "Input" for single-input nodes
				if (UMaterialExpressionOneMinus* OneMinus = Cast<UMaterialExpressionOneMinus>(ToExpr))
					TargetInput = &OneMinus->Input;
				else if (UMaterialExpressionSine* Sine = Cast<UMaterialExpressionSine>(ToExpr))
					TargetInput = &Sine->Input;
				else if (UMaterialExpressionClamp* Clamp = Cast<UMaterialExpressionClamp>(ToExpr))
					TargetInput = &Clamp->Input;
			}
			else if (Conn.ToInput.Equals(TEXT("UVs"), ESearchCase::IgnoreCase) || Conn.ToInput.Equals(TEXT("Coordinates"), ESearchCase::IgnoreCase))
			{
				if (UMaterialExpressionTextureSampleParameter2D* TexSample = Cast<UMaterialExpressionTextureSampleParameter2D>(ToExpr))
					TargetInput = &TexSample->Coordinates;
			}

			if (TargetInput)
			{
				TargetInput->Expression = FromExpr;
				TargetInput->OutputIndex = OutputIndex;
				bConnected = true;
			}
		}

		if (bConnected)
		{
			ConnectionsWired++;
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  WARNING: MF could not wire %s.%s -> %s.%s"),
				*Conn.FromId, *Conn.FromOutput, *Conn.ToId, *Conn.ToInput));
		}
	}

	if (ConnectionsWired > 0)
	{
		LogGeneration(FString::Printf(TEXT("  Wired %d/%d connections"), ConnectionsWired, Definition.Connections.Num()));
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
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("MaterialFunction");
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// v4.9: FMaterialInstanceGenerator Implementation
// ============================================================================
#include "Materials/MaterialInstanceConstant.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"

FGenerationResult FMaterialInstanceGenerator::Generate(const FManifestMaterialInstanceDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("VFX/MaterialInstances") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Material Instance"), Result))
	{
		return Result;
	}

	// v3.0: Use metadata-aware existence check
	uint64 InputHash = Definition.ComputeHash();
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Material Instance"), InputHash, Result))
	{
		return Result;
	}

	// Load parent material
	FString BaseFolder = GetProjectRoot();
	FString ParentMatName = Definition.ParentMaterial;
	UMaterialInterface* ParentMaterial = nullptr;

	LogGeneration(FString::Printf(TEXT("  MIC: Looking for parent material '%s' in base folder '%s'"), *ParentMatName, *BaseFolder));

	// v4.9.1: Check session cache first (for materials generated in same run)
	ParentMaterial = FMaterialGenerator::FindGeneratedMaterial(ParentMatName);
	if (ParentMaterial)
	{
		LogGeneration(FString::Printf(TEXT("    -> FOUND '%s' in session cache"), *ParentMatName));
	}
	else
	{
		// Common material folder locations
		TArray<FString> BasePaths = {
			BaseFolder / TEXT("Materials/VFX") / ParentMatName,
			BaseFolder / TEXT("Materials") / ParentMatName,
			BaseFolder / TEXT("VFX/Materials") / ParentMatName,
			BaseFolder / ParentMatName,
		};

		// If already a full path, use it directly
		if (ParentMatName.StartsWith(TEXT("/")))
		{
			BasePaths.Insert(ParentMatName, 0);
		}

		for (const FString& BasePath : BasePaths)
		{
			// First try FindObject (for materials already loaded in this session)
			FString ObjectPath = FString::Printf(TEXT("%s.%s"), *BasePath, *FPaths::GetBaseFilename(BasePath));
			LogGeneration(FString::Printf(TEXT("    Trying FindObject: %s"), *ObjectPath));
			ParentMaterial = FindObject<UMaterialInterface>(nullptr, *ObjectPath);
			if (ParentMaterial)
			{
				LogGeneration(TEXT("      -> FOUND via FindObject"));
				break;
			}

			// Try FindPackage + GetObject (for recently created packages)
			UPackage* ExistingPackage = FindPackage(nullptr, *BasePath);
			if (ExistingPackage)
			{
				LogGeneration(FString::Printf(TEXT("    Found package: %s"), *BasePath));
				ParentMaterial = FindObject<UMaterialInterface>(ExistingPackage, *FPaths::GetBaseFilename(BasePath));
				if (ParentMaterial)
				{
					LogGeneration(TEXT("      -> FOUND via FindPackage"));
					break;
				}
			}

			// Try LoadObject as fallback (for assets on disk)
			ParentMaterial = LoadObject<UMaterialInterface>(nullptr, *BasePath);
			if (ParentMaterial)
			{
				LogGeneration(FString::Printf(TEXT("      -> FOUND via LoadObject: %s"), *BasePath));
				break;
			}
			ParentMaterial = LoadObject<UMaterialInterface>(nullptr, *ObjectPath);
			if (ParentMaterial)
			{
				LogGeneration(FString::Printf(TEXT("      -> FOUND via LoadObject: %s"), *ObjectPath));
				break;
			}
		}  // end for BasePaths
	}  // end else (not found in session cache)

	if (!ParentMaterial)
	{
		LogGeneration(FString::Printf(TEXT("  MIC: FAILED to find parent material '%s'"), *ParentMatName));
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Parent material not found: %s (searched in %s)"), *ParentMatName, *BaseFolder));
	}

	// Create package
	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}
	Package->FullyLoad();

	// Create material instance constant
	UMaterialInstanceConstant* MIC = NewObject<UMaterialInstanceConstant>(Package, *Definition.Name, RF_Public | RF_Standalone);
	MIC->Parent = ParentMaterial;

	// Apply scalar parameter overrides
	for (const FManifestMaterialInstanceScalarParam& Param : Definition.ScalarParams)
	{
		MIC->SetScalarParameterValueEditorOnly(FName(*Param.Name), Param.Value);
		LogGeneration(FString::Printf(TEXT("  Set scalar param: %s = %.4f"), *Param.Name, Param.Value));
	}

	// Apply vector parameter overrides
	for (const FManifestMaterialInstanceVectorParam& Param : Definition.VectorParams)
	{
		MIC->SetVectorParameterValueEditorOnly(FName(*Param.Name), Param.Value);
		LogGeneration(FString::Printf(TEXT("  Set vector param: %s = (%.2f, %.2f, %.2f, %.2f)"),
			*Param.Name, Param.Value.R, Param.Value.G, Param.Value.B, Param.Value.A));
	}

	// Apply texture parameter overrides
	for (const FManifestMaterialInstanceTextureParam& Param : Definition.TextureParams)
	{
		UTexture* Texture = LoadObject<UTexture>(nullptr, *Param.TexturePath);
		if (Texture)
		{
			MIC->SetTextureParameterValueEditorOnly(FName(*Param.Name), Texture);
			LogGeneration(FString::Printf(TEXT("  Set texture param: %s = %s"), *Param.Name, *Param.TexturePath));
		}
		else
		{
			// v4.23 FAIL-FAST: S45 - Manifest references texture that doesn't exist
			LogGeneration(FString::Printf(TEXT("[E_MIC_TEXTURE_NOT_FOUND] %s | Texture not found for param %s: %s"), *Definition.Name, *Param.Name, *Param.TexturePath));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("Texture not found for param '%s': %s"), *Param.Name, *Param.TexturePath));
		}
	}

	// Finalize material instance
	MIC->PreEditChange(nullptr);
	MIC->PostEditChange();
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(MIC);

	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, MIC, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Material Instance: %s (parent: %s, %d scalar, %d vector, %d texture params)"),
		*Definition.Name, *Definition.ParentMaterial,
		Definition.ScalarParams.Num(), Definition.VectorParams.Num(), Definition.TextureParams.Num()));

	// Store metadata for regeneration tracking
	StoreDataAssetMetadata(MIC, TEXT("MIC"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("MaterialInstance");
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
		// v4.15: AbilityTaskWaitDelay - auto-terminates when ability ends (Track B GAS Audit)
		else if (NodeDef.Type.Equals(TEXT("AbilityTaskWaitDelay"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateAbilityTaskWaitDelayNode(EventGraph, NodeDef);
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

			// Set initial position (v4.20: AutoLayoutNodes will overwrite with computed layout)
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

	// v4.20: Compute layered layout for ALL nodes (overwrites manifest positions)
	AutoLayoutNodes(NodeMap, GraphDefinition.Nodes, GraphDefinition.Connections);

	// v4.22: AUDIT LOGGING POINT 1 - After AutoLayoutNodes
	LogNodePositionsDiagnostic(Blueprint, TEXT("POINT_1_AfterAutoLayoutNodes"));

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

	// v4.16: Return false if any failures occurred (P1 - GenerateEventGraph gate)
	// This gates SavePackage - callers must check return value and abort save on false
	if (NodesFailed > 0 || ConnectionsFailed > 0 || TotalErrors > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] GenerateEventGraph failed for %s: Nodes=%d Connections=%d PostValidation=%d"),
			*GraphDefinition.Name, NodesFailed, ConnectionsFailed, TotalErrors);
		LogGeneration(FString::Printf(TEXT("  GENERATION FAILED: %d node(s), %d connection(s), %d validation error(s)"),
			NodesFailed, ConnectionsFailed, TotalErrors));

		// Mark blueprint as modified even on failure (for debugging)
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		return false;
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
		// v4.27: DynamicCast support for function overrides
		else if (NodeDef.Type.Equals(TEXT("DynamicCast"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateDynamicCastNode(FunctionGraph, NodeDef);
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

			// Set initial position (v4.20: AutoLayoutNodes will overwrite with computed layout)
			if (NodeDef.bHasPosition)
			{
				CreatedNode->NodePosX = static_cast<int32>(NodeDef.PositionX);
				CreatedNode->NodePosY = static_cast<int32>(NodeDef.PositionY);
			}

			LogGeneration(FString::Printf(TEXT("  Created node: %s (%s)"), *NodeDef.Id, *NodeDef.Type));
		}
		else
		{
			NodesFailed++;
		}
	}

	// v4.20: Compute layered layout for ALL nodes (overwrites manifest positions)
	AutoLayoutNodes(NodeMap, OverrideDefinition.Nodes, OverrideDefinition.Connections);

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

// ============================================================================
// v4.14: GenerateCustomFunction - Create new Blueprint functions (not overrides)
// ============================================================================

bool FEventGraphGenerator::GenerateCustomFunction(
	UBlueprint* Blueprint,
	const FManifestCustomFunctionDefinition& FunctionDefinition,
	const FString& ProjectRoot)
{
	if (!Blueprint || FunctionDefinition.FunctionName.IsEmpty())
	{
		LogGeneration(TEXT("GenerateCustomFunction: Invalid blueprint or empty function name"));
		return false;
	}

	FName FunctionName(*FunctionDefinition.FunctionName);
	LogGeneration(FString::Printf(TEXT("GenerateCustomFunction: Creating function '%s'"), *FunctionDefinition.FunctionName));

	// Check if function already exists
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph && Graph->GetFName() == FunctionName)
		{
			LogGeneration(FString::Printf(TEXT("  Function '%s' already exists, skipping"), *FunctionDefinition.FunctionName));
			return true;
		}
	}

	// Create new function graph using FBlueprintEditorUtils
	UEdGraph* FunctionGraph = FBlueprintEditorUtils::CreateNewGraph(
		Blueprint,
		FunctionName,
		UEdGraph::StaticClass(),
		UEdGraphSchema_K2::StaticClass());

	if (!FunctionGraph)
	{
		LogGeneration(FString::Printf(TEXT("GenerateCustomFunction: Failed to create graph for '%s'"), *FunctionDefinition.FunctionName));
		return false;
	}

	FunctionGraph->bAllowDeletion = true;

	// v4.14: Add graph to FunctionGraphs array manually (NOT using AddFunctionGraph
	// because it calls CreateFunctionGraphTerminators which creates default entry/result nodes,
	// but we need to create custom entry/result with our own pins)
	Blueprint->FunctionGraphs.Add(FunctionGraph);

	// Create function entry node
	FGraphNodeCreator<UK2Node_FunctionEntry> EntryNodeCreator(*FunctionGraph);
	UK2Node_FunctionEntry* EntryNode = EntryNodeCreator.CreateNode();
	EntryNode->CreateNewGuid();
	EntryNode->PostPlacedNewNode();
	EntryNode->NodePosX = 0;
	EntryNode->NodePosY = 0;

	// v4.14: Configure function flags - these match what CreateFunctionGraph sets for user-created functions
	// FUNC_BlueprintCallable: Function can be called from Blueprints
	// FUNC_BlueprintEvent: Function is a Blueprint event (editable)
	// FUNC_Public: Function is publicly accessible
	if (FunctionDefinition.bPure)
	{
		EntryNode->AddExtraFlags(FUNC_BlueprintPure);
	}
	EntryNode->AddExtraFlags(FUNC_BlueprintCallable | FUNC_BlueprintEvent | FUNC_Public);

	// v4.14: Mark entry node as editable so users can modify inputs in the editor
	EntryNode->bIsEditable = true;

	// v4.14: CRITICAL - Set CustomGeneratedFunctionName for user-created functions
	// This tells the compiler what name to use for the generated UFunction.
	// Without this, SkeletonGeneratedClass->FindFunctionByName(Graph.GetFName()) fails
	// and the function appears as a "ghost" in MyBlueprint panel.
	EntryNode->CustomGeneratedFunctionName = FunctionName;

	// v4.14: CRITICAL - Align FunctionReference with the graph name
	// This ensures the compiler metadata is consistent and refactoring works correctly.
	// nullptr for class means this is a user-created function, not an override.
	EntryNode->FunctionReference.SetExternalMember(FunctionName, nullptr);

	EntryNodeCreator.Finalize();

	// Add input parameters to the entry node
	for (const FManifestFunctionParameterDefinition& Input : FunctionDefinition.Inputs)
	{
		FEdGraphPinType PinType = GetPinTypeFromString(Input.Type);
		// Create user defined pin for the input
		TSharedPtr<FUserPinInfo> NewPinInfo = MakeShareable(new FUserPinInfo());
		NewPinInfo->PinName = FName(*Input.Name);
		NewPinInfo->PinType = PinType;
		NewPinInfo->DesiredPinDirection = EGPD_Output;  // Entry node outputs are function inputs
		EntryNode->UserDefinedPins.Add(NewPinInfo);
	}

	// Recreate pins after adding user defined pins
	EntryNode->ReconstructNode();

	// Create function result node if there are outputs
	UK2Node_FunctionResult* ResultNode = nullptr;
	if (FunctionDefinition.Outputs.Num() > 0)
	{
		ResultNode = NewObject<UK2Node_FunctionResult>(FunctionGraph);
		FunctionGraph->AddNode(ResultNode, false, false);
		ResultNode->CreateNewGuid();
		ResultNode->PostPlacedNewNode();
		ResultNode->NodePosX = 600;
		ResultNode->NodePosY = 0;

		// v4.14: Mark result node as editable so users can modify outputs in the editor
		ResultNode->bIsEditable = true;

		// Add output parameters to the result node
		for (const FManifestFunctionParameterDefinition& Output : FunctionDefinition.Outputs)
		{
			FEdGraphPinType PinType = GetPinTypeFromString(Output.Type);
			TSharedPtr<FUserPinInfo> NewPinInfo = MakeShareable(new FUserPinInfo());
			NewPinInfo->PinName = FName(*Output.Name);
			NewPinInfo->PinType = PinType;
			NewPinInfo->DesiredPinDirection = EGPD_Input;  // Result node inputs are function outputs
			ResultNode->UserDefinedPins.Add(NewPinInfo);
		}

		ResultNode->ReconstructNode();
	}

	// Map to track created nodes by their definition IDs
	TMap<FString, UK2Node*> NodeMap;
	NodeMap.Add(TEXT("FunctionEntry"), EntryNode);
	if (ResultNode)
	{
		NodeMap.Add(TEXT("FunctionResult"), ResultNode);
		NodeMap.Add(TEXT("Return"), ResultNode);  // Alias for manifest convenience
	}

	int32 NodesCreated = 0;
	int32 NodesFailed = 0;

	// Create all nodes from definition
	for (const FManifestGraphNodeDefinition& NodeDef : FunctionDefinition.Nodes)
	{
		UK2Node* CreatedNode = nullptr;

		if (NodeDef.Type.Equals(TEXT("CallFunction"), ESearchCase::IgnoreCase))
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
		else if (NodeDef.Type.Equals(TEXT("DynamicCast"), ESearchCase::IgnoreCase))
		{
			CreatedNode = CreateDynamicCastNode(FunctionGraph, NodeDef);
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  Unknown/unsupported node type '%s' for custom function node '%s'"),
				*NodeDef.Type, *NodeDef.Id));
			NodesFailed++;
			continue;
		}

		if (CreatedNode)
		{
			NodeMap.Add(NodeDef.Id, CreatedNode);
			NodesCreated++;

			// Set initial position (v4.20: AutoLayoutNodes will overwrite with computed layout)
			if (NodeDef.bHasPosition)
			{
				CreatedNode->NodePosX = static_cast<int32>(NodeDef.PositionX);
				CreatedNode->NodePosY = static_cast<int32>(NodeDef.PositionY);
			}

			LogGeneration(FString::Printf(TEXT("  Created node: %s (%s)"), *NodeDef.Id, *NodeDef.Type));
		}
		else
		{
			NodesFailed++;
		}
	}

	// v4.20: Compute layered layout for ALL nodes (overwrites manifest positions)
	AutoLayoutNodes(NodeMap, FunctionDefinition.Nodes, FunctionDefinition.Connections);

	// Create connections
	int32 ConnectionsCreated = 0;
	int32 ConnectionsFailed = 0;

	for (const FManifestGraphConnectionDefinition& Connection : FunctionDefinition.Connections)
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

	LogGeneration(FString::Printf(TEXT("Custom function '%s': %d nodes created, %d failed, %d connections, %d failed"),
		*FunctionDefinition.FunctionName, NodesCreated, NodesFailed, ConnectionsCreated, ConnectionsFailed));

	// v4.14: Mark blueprint as STRUCTURALLY modified (required for function graph changes)
	// This tells the Blueprint system that the class structure has changed (new function added)
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

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

		// v4.25.3: GameplayTag Blueprint Library functions
		WellKnownFunctions.Add(TEXT("MakeLiteralGameplayTag"), UBlueprintGameplayTagLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("MakeGameplayTagContainerFromTag"), UBlueprintGameplayTagLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("MakeGameplayTagContainerFromArray"), UBlueprintGameplayTagLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("AddGameplayTag"), UBlueprintGameplayTagLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("RemoveGameplayTag"), UBlueprintGameplayTagLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("HasTag"), UBlueprintGameplayTagLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("HasAnyTags"), UBlueprintGameplayTagLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("HasAllTags"), UBlueprintGameplayTagLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("MatchesTag"), UBlueprintGameplayTagLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("GetTagName"), UBlueprintGameplayTagLibrary::StaticClass());

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
		// v4.14: K2_ versions of GE application functions
		WellKnownFunctions.Add(TEXT("K2_ApplyGameplayEffectToOwner"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_ApplyGameplayEffectToTarget"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("K2_ApplyGameplayEffectToSelf"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("BP_RemoveGameplayEffectFromOwnerWithAssetTags"), UGameplayAbility::StaticClass());
		WellKnownFunctions.Add(TEXT("BP_RemoveGameplayEffectFromOwnerWithGrantedTags"), UGameplayAbility::StaticClass());

		// Math functions - legacy float versions (kept for compatibility)
		WellKnownFunctions.Add(TEXT("Multiply_FloatFloat"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("Divide_FloatFloat"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("Add_FloatFloat"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("Subtract_FloatFloat"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("FClamp"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("RandomFloatInRange"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("GetForwardVector"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("MakeRotator"), UKismetMathLibrary::StaticClass());

		// v4.25.3: Math functions - UE5 double precision versions
		WellKnownFunctions.Add(TEXT("Multiply_DoubleDouble"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("Divide_DoubleDouble"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("Add_DoubleDouble"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("Subtract_DoubleDouble"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("Less_DoubleDouble"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("Greater_DoubleDouble"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("LessEqual_DoubleDouble"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("GreaterEqual_DoubleDouble"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("EqualEqual_DoubleDouble"), UKismetMathLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("NotEqual_DoubleDouble"), UKismetMathLibrary::StaticClass());

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

		// v4.25.3: Niagara functions
		WellKnownFunctions.Add(TEXT("SpawnSystemAttached"), UNiagaraFunctionLibrary::StaticClass());
		WellKnownFunctions.Add(TEXT("SpawnSystemAtLocation"), UNiagaraFunctionLibrary::StaticClass());

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

		// v4.25.3: UE5 double precision math function remapping
		DeprecatedFunctionRemapping.Add(TEXT("Less_FloatFloat"), TEXT("Less_DoubleDouble"));
		DeprecatedFunctionRemapping.Add(TEXT("Greater_FloatFloat"), TEXT("Greater_DoubleDouble"));
		DeprecatedFunctionRemapping.Add(TEXT("LessEqual_FloatFloat"), TEXT("LessEqual_DoubleDouble"));
		DeprecatedFunctionRemapping.Add(TEXT("GreaterEqual_FloatFloat"), TEXT("GreaterEqual_DoubleDouble"));
		DeprecatedFunctionRemapping.Add(TEXT("EqualEqual_FloatFloat"), TEXT("EqualEqual_DoubleDouble"));
		DeprecatedFunctionRemapping.Add(TEXT("NotEqual_FloatFloat"), TEXT("NotEqual_DoubleDouble"));

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
								// v4.23 FAIL-FAST: Manifest references enum value that doesn't exist
								LogGeneration(FString::Printf(TEXT("[E_ENUM_VALUE_NOT_FOUND] Node '%s' | Could not find enum value '%s' in %s"),
									*NodeDef.Id, *ParamValue, *EnumType->GetName()));
								LogGeneration(TEXT("  Available values:"));
								for (int32 i = 0; i < EnumType->NumEnums() - 1; i++)
								{
									LogGeneration(FString::Printf(TEXT("    - %s"), *EnumType->GetNameStringByIndex(i)));
								}
								return nullptr; // Signal failure to caller
							}
						}
					}
					// v4.14: Handle TSubclassOf (PC_Class) pin types for class references
					else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class)
					{
						// This is a TSubclassOf<T> pin - need to resolve the class and set DefaultObject
						UClass* ResolvedClass = nullptr;
						FString ClassName = ParamValue;

						// Determine the expected base class from pin SubCategoryObject
						UClass* ExpectedBaseClass = Cast<UClass>(ParamPin->PinType.PinSubCategoryObject.Get());
						FString BaseClassName = ExpectedBaseClass ? ExpectedBaseClass->GetName() : TEXT("UObject");

						// v4.22: Check session cache FIRST for assets created in this generation session
						if (UClass** CachedClass = GSessionBlueprintClassCache.Find(ClassName))
						{
							ResolvedClass = *CachedClass;
							LogGeneration(FString::Printf(TEXT("  Found Blueprint class in session cache: %s"), *ClassName));
						}

						// If not in cache, try to find as a Blueprint generated class in common content paths
						TArray<FString> SearchPaths;
						if (!ResolvedClass)
						{
							FString ProjectName = FApp::GetProjectName();
							SearchPaths.Add(FString::Printf(TEXT("/Game/%s/Effects/%s"), *ProjectName, *ClassName));
							SearchPaths.Add(FString::Printf(TEXT("/Game/%s/Abilities/%s"), *ProjectName, *ClassName));
							SearchPaths.Add(FString::Printf(TEXT("/Game/%s/%s"), *ProjectName, *ClassName));
							SearchPaths.Add(FString::Printf(TEXT("/Game/FatherCompanion/Effects/%s"), *ClassName));
							SearchPaths.Add(FString::Printf(TEXT("/Game/FatherCompanion/%s"), *ClassName));

							// Check if it's a full path already
							if (ClassName.StartsWith(TEXT("/Game/")))
							{
								SearchPaths.Insert(ClassName, 0);
							}
						}

						for (const FString& SearchPath : SearchPaths)
						{
							// Try loading as Blueprint
							FString BlueprintPath = SearchPath;
							if (!BlueprintPath.EndsWith(TEXT("_C")))
							{
								BlueprintPath = SearchPath + TEXT(".") + FPaths::GetBaseFilename(SearchPath) + TEXT("_C");
							}

							ResolvedClass = LoadClass<UObject>(nullptr, *BlueprintPath, nullptr, LOAD_None, nullptr);
							if (ResolvedClass)
							{
								LogGeneration(FString::Printf(TEXT("  Found Blueprint class at: %s"), *BlueprintPath));
								break;
							}

							// Also try direct asset path for native classes
							FString AssetPath = SearchPath + TEXT(".") + FPaths::GetBaseFilename(SearchPath);
							UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *AssetPath);
							if (BP && BP->GeneratedClass)
							{
								ResolvedClass = BP->GeneratedClass;
								LogGeneration(FString::Printf(TEXT("  Found Blueprint asset at: %s"), *AssetPath));
								break;
							}
						}

						// If not found, try native class lookup (StaticClass pattern)
						if (!ResolvedClass)
						{
							// Remove common prefixes for native lookup
							FString NativeClassName = ClassName;
							if (NativeClassName.StartsWith(TEXT("GE_")) || NativeClassName.StartsWith(TEXT("GA_")))
							{
								// These are always Blueprint classes, not native
								LogGeneration(FString::Printf(TEXT("  Note: %s appears to be a Blueprint class (GE_/GA_ prefix)"), *ClassName));
							}
							else
							{
								// Try to find native class using UE5 pattern (FindFirstObject)
								ResolvedClass = FindFirstObject<UClass>(*NativeClassName, EFindFirstObjectOptions::ExactClass);
								if (!ResolvedClass)
								{
									// Try with U prefix
									ResolvedClass = FindFirstObject<UClass>(*(TEXT("U") + NativeClassName), EFindFirstObjectOptions::ExactClass);
								}
							}
						}

						if (ResolvedClass)
						{
							// Validate the class inherits from expected base
							if (ExpectedBaseClass && !ResolvedClass->IsChildOf(ExpectedBaseClass))
							{
								LogGeneration(FString::Printf(TEXT("  WARNING: Class '%s' does not inherit from '%s'. Setting anyway."),
									*ResolvedClass->GetName(), *BaseClassName));
							}

							ParamPin->DefaultObject = ResolvedClass;
							LogGeneration(FString::Printf(TEXT("  Set class parameter '%s' = '%s' (TSubclassOf<%s>)"),
								*ParamName, *ResolvedClass->GetName(), *BaseClassName));
						}
						else
						{
							// v4.23 FAIL-FAST: Manifest references class that cannot be resolved
							LogGeneration(FString::Printf(TEXT("[E_TSUBCLASSOF_NOT_RESOLVED] Node '%s' | Could not resolve class '%s' for TSubclassOf<%s> pin '%s'"),
								*NodeDef.Id, *ClassName, *BaseClassName, *ParamName));
							LogGeneration(TEXT("  Searched paths:"));
							for (const FString& Path : SearchPaths)
							{
								LogGeneration(FString::Printf(TEXT("    - %s"), *Path));
							}
							return nullptr; // Signal failure to caller
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
					// v4.23 FAIL-FAST: Manifest references pin that doesn't exist on function
					LogGeneration(FString::Printf(TEXT("[E_FUNCTION_PIN_NOT_FOUND] Node '%s' | Could not find input pin '%s' on function '%s'"),
						*NodeDef.Id, *ParamName, *FunctionName.ToString()));
					LogGeneration(TEXT("  Available pins:"));
					for (UEdGraphPin* Pin : CallNode->Pins)
					{
						if (Pin && Pin->Direction == EGPD_Input)
						{
							LogGeneration(FString::Printf(TEXT("    - %s (%s)"), *Pin->PinName.ToString(), *Pin->PinType.PinCategory.ToString()));
						}
					}
					return nullptr; // Signal failure to caller
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

	// v4.23 FAIL-FAST: Manifest references function that cannot be found
	LogGeneration(FString::Printf(TEXT("[E_FUNCTION_NOT_FOUND] Node '%s' | Could not find function '%s'"),
		*NodeDef.Id, *FunctionName.ToString()));
	return nullptr; // Signal failure to caller
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
			// v4.23 FAIL-FAST: Manifest references target class that cannot be resolved
			LogGeneration(FString::Printf(TEXT("[E_VARIABLEGET_TARGET_NOT_FOUND] Node '%s' | Could not resolve class for target '%s'"),
				*NodeDef.Id, **TargetObjectPtr));
			return nullptr; // Signal failure to caller
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
			// v4.23 FAIL-FAST: Manifest references target class that cannot be resolved
			LogGeneration(FString::Printf(TEXT("[E_VARIABLESET_TARGET_NOT_FOUND] Node '%s' | Could not resolve class for target '%s'"),
				*NodeDef.Id, **TargetObjectPtr));
			return nullptr; // Signal failure to caller
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
		// v4.23 FAIL-FAST: Manifest references class that cannot be found
		LogGeneration(FString::Printf(TEXT("[E_PROPERTYGET_CLASS_NOT_FOUND] Node '%s' | Could not find class '%s'"), *NodeDef.Id, **TargetClassPtr));
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
		// v4.23 FAIL-FAST: Manifest references class that cannot be found
		LogGeneration(FString::Printf(TEXT("[E_PROPERTYSET_CLASS_NOT_FOUND] Node '%s' | Could not find class '%s'"), *NodeDef.Id, **TargetClassPtr));
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
		// v4.23 FAIL-FAST: Manifest references struct that cannot be found
		LogGeneration(FString::Printf(TEXT("[E_BREAKSTRUCT_STRUCT_NOT_FOUND] Node '%s' | Could not find struct '%s'"), *NodeDef.Id, **StructTypePtr));
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

// v4.15: AbilityTaskWaitDelay - auto-terminates when ability ends (Track B GAS Audit)
// Uses UK2Node_LatentAbilityCall which properly handles AbilityTask lifecycle
UK2Node* FEventGraphGenerator::CreateAbilityTaskWaitDelayNode(
	UEdGraph* Graph,
	const FManifestGraphNodeDefinition& NodeDef)
{
	UK2Node_LatentAbilityCall* WaitDelayNode = NewObject<UK2Node_LatentAbilityCall>(Graph);
	Graph->AddNode(WaitDelayNode, false, false);

	// Find the WaitDelay factory function on UAbilityTask_WaitDelay
	UFunction* WaitDelayFunction = UAbilityTask_WaitDelay::StaticClass()->FindFunctionByName(FName("WaitDelay"));
	if (!WaitDelayFunction)
	{
		UE_LOG(LogGasAbilityGenerator, Error, TEXT("Failed to find WaitDelay function on UAbilityTask_WaitDelay"));
		return WaitDelayNode;
	}

	// Set the protected base class properties via reflection (UK2Node_BaseAsyncTask members)
	// These configure which factory function creates the latent task
	UClass* NodeClass = WaitDelayNode->GetClass();

	// ProxyFactoryFunctionName = "WaitDelay"
	if (FNameProperty* FactoryFuncNameProp = FindFProperty<FNameProperty>(NodeClass, TEXT("ProxyFactoryFunctionName")))
	{
		FactoryFuncNameProp->SetPropertyValue_InContainer(WaitDelayNode, WaitDelayFunction->GetFName());
	}

	// ProxyFactoryClass = UAbilityTask_WaitDelay::StaticClass()
	if (FObjectProperty* FactoryClassProp = FindFProperty<FObjectProperty>(NodeClass, TEXT("ProxyFactoryClass")))
	{
		FactoryClassProp->SetObjectPropertyValue_InContainer(WaitDelayNode, UAbilityTask_WaitDelay::StaticClass());
	}

	// ProxyClass = UAbilityTask_WaitDelay::StaticClass() (the return type)
	if (FObjectProperty* ProxyClassProp = FindFProperty<FObjectProperty>(NodeClass, TEXT("ProxyClass")))
	{
		ProxyClassProp->SetObjectPropertyValue_InContainer(WaitDelayNode, UAbilityTask_WaitDelay::StaticClass());
	}

	WaitDelayNode->CreateNewGuid();
	WaitDelayNode->PostPlacedNewNode();
	WaitDelayNode->AllocateDefaultPins();

	// Set duration/Time if specified in properties
	const FString* DurationPtr = NodeDef.Properties.Find(TEXT("duration"));
	if (DurationPtr)
	{
		// The WaitDelay function has a "Time" parameter
		UEdGraphPin* TimePin = WaitDelayNode->FindPin(FName("Time"));
		if (TimePin)
		{
			TimePin->DefaultValue = *DurationPtr;
		}
	}

	return WaitDelayNode;
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
		// v4.23 FAIL-FAST: Manifest references class that cannot be found
		LogGeneration(FString::Printf(TEXT("[E_DYNAMICCAST_CLASS_NOT_FOUND] Node '%s' | Could not find target class '%s'"),
			*NodeDef.Id, **TargetClassPtr));
		// v2.6.7: Track missing dependency for deferred generation
		FString DepType = TEXT("ActorBlueprint");
		if (TargetClassPtr->StartsWith(TEXT("BP_"))) DepType = TEXT("ActorBlueprint");
		else if (TargetClassPtr->StartsWith(TEXT("WBP_"))) DepType = TEXT("WidgetBlueprint");
		AddMissingDependency(*TargetClassPtr, DepType,
			FString::Printf(TEXT("DynamicCast node '%s'"), *NodeDef.Id));
		return nullptr; // Signal failure to caller
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
		// v4.23 FAIL-FAST: S46 - ForEachLoop macro not found in engine
		LogGeneration(FString::Printf(TEXT("[E_FOREACHLOOP_MACRO_NOT_FOUND] ForEachLoop macro not found for node '%s'"), *NodeDef.Id));
		return nullptr; // Signal failure to caller
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
			// v4.16: HARD FAIL - type mismatch requires conversion node (D-001, P5)
			// UE5 KismetCompiler treats this as compile error; we fail early
			UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [E_TYPE_MISMATCH] Pin type mismatch: [%s].%s (%s) -> [%s].%s (%s)"),
				*Connection.From.NodeId, *FromPin->PinName.ToString(), *FromTypeDesc,
				*Connection.To.NodeId, *ToPin->PinName.ToString(), *ToTypeDesc);
			UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator]   Connection requires conversion node (UE response: MAKE_WITH_CONVERSION_NODE)"));
			UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator]   Generator does not auto-create conversions. Fix manifest or add explicit conversion node."));

			LogGeneration(FString::Printf(TEXT("      ERROR: Type mismatch - %s -> %s requires conversion"), *FromTypeDesc, *ToTypeDesc));
			LogGeneration(TEXT("      Fix manifest types or add explicit conversion node in event graph"));
			return false;
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

// ============================================================================
// v4.20: Placement Contract v1.0 - Layered Graph Layout Algorithm
// Audit: Claude ✅ | GPT ✅ | Erdem ✅ (2026-01-21)
// Reference: ClaudeContext/Handoffs/EventGraph_Node_Placement_Reference.md
// ============================================================================

// v4.20.11: Static storage for node positions - keyed by Blueprint path + node GUID
// This allows re-applying positions after compilation (which may reset them)
static TMap<FString, TMap<FGuid, FIntPoint>> GStoredNodePositions;

namespace NodePlacement
{
	// LOCKED CONSTANTS - per Placement Contract v1.0
	constexpr int32 GRID_SIZE = 16;
	constexpr int32 HORIZONTAL_LAYER_SPACING = 350;
	constexpr int32 VERTICAL_NODE_GAP = 50;
	constexpr int32 LANE_SEPARATION = 150;
	constexpr int32 DATA_NODE_X_OFFSET = -250;

	// v4.20.10: Pin-level positioning constants
	constexpr float PIN_VERTICAL_SPACING = 26.0f;  // Approximate Y spacing between pins
	constexpr float NODE_HEADER_HEIGHT = 30.0f;    // Height of node title bar
	constexpr float PIN_SPREAD_MULTIPLIER = 3.0f;  // Amplify vertical spread based on pin position

	// Height formulas - per EdGraphSchema_K2.cpp
	constexpr float EVENT_BASE_HEIGHT = 48.0f;
	constexpr float EVENT_HEIGHT_PER_PIN = 16.0f;
	constexpr float EXEC_BASE_HEIGHT = 80.0f;
	constexpr float EXEC_HEIGHT_PER_PIN = 18.0f;
	constexpr float DATA_BASE_HEIGHT = 48.0f;

	/** Snap value to grid */
	inline int32 SnapToGrid(int32 Value)
	{
		return FMath::RoundToInt32((float)Value / (float)GRID_SIZE) * GRID_SIZE;
	}

	/** Check if pin name is an exec output pin */
	inline bool IsExecOutputPin(const FString& PinName)
	{
		return PinName.Equals(TEXT("Then"), ESearchCase::IgnoreCase) ||
		       PinName.Equals(TEXT("Exec"), ESearchCase::IgnoreCase) ||
		       PinName.Equals(TEXT("Completed"), ESearchCase::IgnoreCase) ||
		       PinName.Equals(TEXT("true"), ESearchCase::IgnoreCase) ||
		       PinName.Equals(TEXT("false"), ESearchCase::IgnoreCase) ||
		       PinName.StartsWith(TEXT("Out_"), ESearchCase::IgnoreCase);
	}

	/** Determine node family from manifest type */
	enum class ENodeFamily : uint8
	{
		Event,      // Event, CustomEvent
		ExecLogic,  // CallFunction, Branch, Sequence, Delay, etc.
		DataPure    // VariableGet, PropertyGet, Self, etc.
	};

	inline ENodeFamily GetNodeFamily(const FString& NodeType)
	{
		// Event family
		if (NodeType.Equals(TEXT("Event"), ESearchCase::IgnoreCase) ||
		    NodeType.Equals(TEXT("CustomEvent"), ESearchCase::IgnoreCase))
		{
			return ENodeFamily::Event;
		}

		// Data/Pure family (no exec pins)
		if (NodeType.Equals(TEXT("VariableGet"), ESearchCase::IgnoreCase) ||
		    NodeType.Equals(TEXT("PropertyGet"), ESearchCase::IgnoreCase) ||
		    NodeType.Equals(TEXT("Self"), ESearchCase::IgnoreCase) ||
		    NodeType.Equals(TEXT("BreakStruct"), ESearchCase::IgnoreCase) ||
		    NodeType.Equals(TEXT("MakeArray"), ESearchCase::IgnoreCase) ||
		    NodeType.Equals(TEXT("GetArrayItem"), ESearchCase::IgnoreCase))
		{
			return ENodeFamily::DataPure;
		}

		// Everything else is Exec Logic (has exec pins)
		return ENodeFamily::ExecLogic;
	}

	/** Estimate node height based on family and pin count */
	inline int32 EstimateNodeHeight(const FString& NodeType, int32 MaxPinCount)
	{
		ENodeFamily Family = GetNodeFamily(NodeType);

		switch (Family)
		{
		case ENodeFamily::Event:
			return static_cast<int32>(EVENT_BASE_HEIGHT + (MaxPinCount * EVENT_HEIGHT_PER_PIN));

		case ENodeFamily::ExecLogic:
			return static_cast<int32>(EXEC_BASE_HEIGHT + (MaxPinCount * EXEC_HEIGHT_PER_PIN));

		case ENodeFamily::DataPure:
			// BreakStruct and MakeArray can have many pins
			if (NodeType.Equals(TEXT("BreakStruct"), ESearchCase::IgnoreCase) ||
			    NodeType.Equals(TEXT("MakeArray"), ESearchCase::IgnoreCase))
			{
				return static_cast<int32>(DATA_BASE_HEIGHT + (MaxPinCount * EXEC_HEIGHT_PER_PIN));
			}
			return static_cast<int32>(DATA_BASE_HEIGHT);
		}

		return static_cast<int32>(EXEC_BASE_HEIGHT);
	}

	/**
	 * v4.20.9: Get the Y offset of a specific pin within a node.
	 * Pin Y offset is calculated based on:
	 * - Pins are grouped by direction (inputs on left, outputs on right)
	 * - Exec pins appear at the top, followed by data pins
	 * @param Node The UK2Node to query
	 * @param PinName The name of the pin to find
	 * @param Direction EGPD_Input or EGPD_Output
	 * @return Y offset from node top (0 if pin not found)
	 */
	inline float GetPinYOffset(UK2Node* Node, const FString& PinName, EEdGraphPinDirection Direction)
	{
		if (!Node) return 0.0f;

		// Collect visible pins of the requested direction, in order
		TArray<UEdGraphPin*> DirectionPins;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && !Pin->bHidden && Pin->Direction == Direction)
			{
				DirectionPins.Add(Pin);
			}
		}

		// Find the target pin and its index
		int32 PinIndex = -1;
		for (int32 i = 0; i < DirectionPins.Num(); ++i)
		{
			if (DirectionPins[i]->GetFName().ToString().Equals(PinName, ESearchCase::IgnoreCase) ||
			    DirectionPins[i]->PinFriendlyName.ToString().Equals(PinName, ESearchCase::IgnoreCase))
			{
				PinIndex = i;
				break;
			}
		}

		if (PinIndex < 0)
		{
			// v4.23 FAIL-FAST: F7 - Log error but continue with fallback (layout helper - actual failure handled by connection wiring)
			UE_LOG(LogTemp, Error, TEXT("[E_PIN_POSITION_NOT_FOUND] GetPinYOffset: Pin '%s' not found on node '%s'"), *PinName, *Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
			return NODE_HEADER_HEIGHT + (DirectionPins.Num() / 2.0f) * PIN_VERTICAL_SPACING;
		}

		// Y offset = header + (pin index * spacing)
		return NODE_HEADER_HEIGHT + PinIndex * PIN_VERTICAL_SPACING;
	}
}

/** v4.20.9: Tracks a specific pin-to-pin connection for layout */
struct FPinConnection
{
	FString FromNodeId;
	FString FromPinName;
	FString ToNodeId;
	FString ToPinName;
};

/** Placement info for a single node */
struct FNodePlacementInfo
{
	FString NodeId;
	int32 Layer = -1;           // Execution distance from entry (-1 = unassigned)
	int32 LaneIndex = -1;       // Which event chain this belongs to
	int32 IndexInLane = 0;      // Position within the lane's layer
	int32 EstimatedHeight = 80;
	int32 PosX = 0;
	int32 PosY = 0;
	bool bIsEntryNode = false;
	bool bIsDataNode = false;
};

void FEventGraphGenerator::AutoLayoutNodes(
	TMap<FString, UK2Node*>& NodeMap,
	const TArray<FManifestGraphNodeDefinition>& NodeDefs,
	const TArray<FManifestGraphConnectionDefinition>& Connections)
{
	using namespace NodePlacement;

	// v4.20: Per Placement Contract v1.0, ALWAYS compute positions based on graph structure
	// Manifest position: fields are ignored - algorithm determines optimal layout
	LogGeneration(FString::Printf(TEXT("  [PLACEMENT] v4.20: Starting layered graph layout for %d nodes"), NodeDefs.Num()));

	// Build placement info map for ALL nodes
	TMap<FString, FNodePlacementInfo> PlacementMap;
	for (int32 Idx = 0; Idx < NodeDefs.Num(); ++Idx)
	{
		const FManifestGraphNodeDefinition& NodeDef = NodeDefs[Idx];
		// v4.20: Process ALL nodes - algorithm computes positions from graph structure
		FNodePlacementInfo& Info = PlacementMap.Add(NodeDef.Id);
		Info.NodeId = NodeDef.Id;
		Info.bIsEntryNode = (NodeDef.Type.Equals(TEXT("Event"), ESearchCase::IgnoreCase) ||
		                     NodeDef.Type.Equals(TEXT("CustomEvent"), ESearchCase::IgnoreCase));

		// v4.20.1: Determine if node is pure/data by checking actual UK2Node pins
		// Pure nodes have NO exec pins - this is more accurate than checking manifest type
		Info.bIsDataNode = false; // Default to exec node
		int32 MaxPinCount = 2; // Default estimate

		if (UK2Node** NodePtr = NodeMap.Find(NodeDef.Id))
		{
			if (*NodePtr)
			{
				int32 InputCount = 0;
				int32 OutputCount = 0;
				bool bHasExecPin = false;

				for (UEdGraphPin* Pin : (*NodePtr)->Pins)
				{
					if (Pin && !Pin->bHidden)
					{
						// Check for exec pins (PC_Exec category)
						if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
						{
							bHasExecPin = true;
						}

						if (Pin->Direction == EGPD_Input)
							InputCount++;
						else
							OutputCount++;
					}
				}

				// Node is pure/data if it has NO exec pins
				Info.bIsDataNode = !bHasExecPin;
				MaxPinCount = FMath::Max(InputCount, OutputCount);
			}
		}
		else
		{
			// Fallback to manifest type if node not in map
			Info.bIsDataNode = (GetNodeFamily(NodeDef.Type) == ENodeFamily::DataPure);
		}

		Info.EstimatedHeight = EstimateNodeHeight(NodeDef.Type, MaxPinCount);
	}

	// Debug: count data vs exec nodes
	int32 DataNodeCount = 0;
	int32 ExecNodeCount = 0;
	for (const auto& Pair : PlacementMap)
	{
		if (Pair.Value.bIsDataNode)
			DataNodeCount++;
		else
			ExecNodeCount++;
	}
	LogGeneration(FString::Printf(TEXT("  [PLACEMENT] Node classification: %d exec nodes, %d data/pure nodes"), ExecNodeCount, DataNodeCount));

	// Build exec adjacency list: NodeId -> array of target NodeIds
	TMap<FString, TArray<FString>> ExecAdjacency;
	for (const FManifestGraphConnectionDefinition& Conn : Connections)
	{
		if (IsExecOutputPin(Conn.From.PinName))
		{
			ExecAdjacency.FindOrAdd(Conn.From.NodeId).Add(Conn.To.NodeId);
		}
	}

	// Find entry nodes and assign lanes by manifest order (EntryOrderIndex)
	TArray<FString> EntryNodes;
	for (const FManifestGraphNodeDefinition& NodeDef : NodeDefs)
	{
		// v4.20: Check ALL nodes for entry points
		if (NodeDef.Type.Equals(TEXT("Event"), ESearchCase::IgnoreCase) ||
		    NodeDef.Type.Equals(TEXT("CustomEvent"), ESearchCase::IgnoreCase))
		{
			EntryNodes.Add(NodeDef.Id);
		}
	}

	LogGeneration(FString::Printf(TEXT("  [PLACEMENT] Found %d entry nodes"), EntryNodes.Num()));

	// BFS from each entry node to assign layers
	int32 LaneIndex = 0;
	for (const FString& EntryId : EntryNodes)
	{
		FNodePlacementInfo* EntryInfo = PlacementMap.Find(EntryId);
		if (!EntryInfo)
		{
			continue;
		}

		EntryInfo->Layer = 0;
		EntryInfo->LaneIndex = LaneIndex;

		// BFS queue: pairs of (NodeId, Layer)
		TQueue<TPair<FString, int32>> Queue;
		TSet<FString> Visited;

		Queue.Enqueue(MakeTuple(EntryId, 0));
		Visited.Add(EntryId);

		while (!Queue.IsEmpty())
		{
			TPair<FString, int32> Current;
			Queue.Dequeue(Current);
			const FString& CurrentNodeId = Current.Key;
			int32 CurrentLayer = Current.Value;

			// Update placement info
			if (FNodePlacementInfo* Info = PlacementMap.Find(CurrentNodeId))
			{
				// Take max layer if visited from multiple paths (longest path)
				Info->Layer = FMath::Max(Info->Layer, CurrentLayer);
				if (Info->LaneIndex < 0)
				{
					Info->LaneIndex = LaneIndex;
				}
			}

			// Enqueue successors
			if (TArray<FString>* Successors = ExecAdjacency.Find(CurrentNodeId))
			{
				for (const FString& NextNodeId : *Successors)
				{
					// Skip data nodes in exec traversal
					if (FNodePlacementInfo* NextInfo = PlacementMap.Find(NextNodeId))
					{
						if (NextInfo->bIsDataNode)
						{
							continue;
						}
					}

					if (!Visited.Contains(NextNodeId))
					{
						Visited.Add(NextNodeId);
						Queue.Enqueue(MakeTuple(NextNodeId, CurrentLayer + 1));
					}
				}
			}
		}

		LaneIndex++;
	}

	// Warn about orphan exec nodes (not reachable from any entry)
	for (auto& Pair : PlacementMap)
	{
		if (Pair.Value.Layer < 0 && !Pair.Value.bIsDataNode)
		{
			LogGeneration(FString::Printf(TEXT("  [PLACEMENT] WARNING: Orphan exec node '%s' - not reachable from any entry"),
				*Pair.Key));
			// Assign orphans to layer 0, lane = num entry nodes (separate lane)
			Pair.Value.Layer = 0;
			Pair.Value.LaneIndex = LaneIndex;
		}
	}
	if (LaneIndex > EntryNodes.Num())
	{
		LaneIndex++; // Account for orphan lane
	}

	// Group exec nodes by (Lane, Layer) for position calculation
	TMap<int32, TMap<int32, TArray<FString>>> LaneLayerNodes; // Lane -> Layer -> NodeIds
	for (auto& Pair : PlacementMap)
	{
		if (!Pair.Value.bIsDataNode && Pair.Value.Layer >= 0)
		{
			LaneLayerNodes.FindOrAdd(Pair.Value.LaneIndex)
			              .FindOrAdd(Pair.Value.Layer)
			              .Add(Pair.Key);
		}
	}

	// Calculate positions for exec nodes
	int32 CurrentLaneStartY = 0;
	int32 MaxLayerSeen = 0;

	// Get sorted lane indices
	TArray<int32> SortedLanes;
	LaneLayerNodes.GetKeys(SortedLanes);
	SortedLanes.Sort();

	for (int32 Lane : SortedLanes)
	{
		TMap<int32, TArray<FString>>& LayerMap = LaneLayerNodes[Lane];

		// Find max layer in this lane
		int32 MaxLayerInLane = 0;
		for (auto& LayerPair : LayerMap)
		{
			MaxLayerInLane = FMath::Max(MaxLayerInLane, LayerPair.Key);
			MaxLayerSeen = FMath::Max(MaxLayerSeen, LayerPair.Key);
		}

		// Track max height used in this lane across all layers
		int32 MaxHeightInLane = 0;

		// Process each layer in this lane
		for (int32 Layer = 0; Layer <= MaxLayerInLane; ++Layer)
		{
			TArray<FString>* NodesInLayer = LayerMap.Find(Layer);
			if (!NodesInLayer)
			{
				continue;
			}

			int32 CurrentY = CurrentLaneStartY;

			for (const FString& NodeId : *NodesInLayer)
			{
				FNodePlacementInfo& Info = PlacementMap[NodeId];

				// X based on layer
				Info.PosX = Layer * HORIZONTAL_LAYER_SPACING;

				// Y based on cumulative height within lane
				Info.PosY = CurrentY;

				CurrentY += Info.EstimatedHeight + VERTICAL_NODE_GAP;
			}

			MaxHeightInLane = FMath::Max(MaxHeightInLane, CurrentY - CurrentLaneStartY);
		}

		// Move to next lane with separation
		CurrentLaneStartY += MaxHeightInLane + LANE_SEPARATION;
	}

	// v4.20.10: Wave-based pin-level positioning for data nodes
	// Position data nodes relative to their IMMEDIATE neighbors (not exec chain endpoints)
	// Process in waves: first nodes connected to exec nodes, then their neighbors, etc.

	// Build connection map: for each data node, track immediate source and consumer
	struct FDataNodeConnection
	{
		FString SourceNodeId;     // Immediate source node (may be exec or data)
		FString SourcePinName;    // Output pin on source
		FString InputPinName;     // Input pin on this node
		FString ConsumerNodeId;   // Immediate consumer node (may be exec or data)
		FString ConsumerPinName;  // Input pin on consumer
		FString OutputPinName;    // Output pin on this node
	};
	TMap<FString, FDataNodeConnection> DataNodeConnMap;

	for (const FManifestGraphConnectionDefinition& Conn : Connections)
	{
		// Skip exec connections
		if (IsExecOutputPin(Conn.From.PinName) || IsExecOutputPin(Conn.To.PinName))
			continue;

		// Track incoming connections to data nodes
		if (FNodePlacementInfo* ToInfo = PlacementMap.Find(Conn.To.NodeId))
		{
			if (ToInfo->bIsDataNode)
			{
				FDataNodeConnection& DC = DataNodeConnMap.FindOrAdd(Conn.To.NodeId);
				if (DC.SourceNodeId.IsEmpty())
				{
					DC.SourceNodeId = Conn.From.NodeId;
					DC.SourcePinName = Conn.From.PinName;
					DC.InputPinName = Conn.To.PinName;
				}
			}
		}

		// Track outgoing connections from data nodes
		if (FNodePlacementInfo* FromInfo = PlacementMap.Find(Conn.From.NodeId))
		{
			if (FromInfo->bIsDataNode)
			{
				FDataNodeConnection& DC = DataNodeConnMap.FindOrAdd(Conn.From.NodeId);
				if (DC.ConsumerNodeId.IsEmpty())
				{
					DC.ConsumerNodeId = Conn.To.NodeId;
					DC.ConsumerPinName = Conn.To.PinName;
					DC.OutputPinName = Conn.From.PinName;
				}
			}
		}
	}

	// Helper: check if a node has a valid position
	auto HasPosition = [&PlacementMap](const FString& NodeId) -> bool
	{
		if (FNodePlacementInfo* Info = PlacementMap.Find(NodeId))
		{
			return Info->Layer >= 0 || (Info->PosX != 0 || Info->PosY != 0);
		}
		return false;
	};

	// Helper: get node position
	auto GetNodePos = [&PlacementMap](const FString& NodeId) -> TPair<int32, int32>
	{
		if (FNodePlacementInfo* Info = PlacementMap.Find(NodeId))
		{
			return MakeTuple(Info->PosX, Info->PosY);
		}
		return MakeTuple(0, 0);
	};

	int32 DataNodesPositioned = 0;
	int32 DataNodesUnpositioned = 0;
	int32 WaveCount = 0;

	// Process in waves until no more nodes can be positioned
	bool bProgress = true;
	while (bProgress && WaveCount < 20)
	{
		bProgress = false;
		WaveCount++;

		for (auto& Pair : PlacementMap)
		{
			// Skip non-data nodes and already positioned nodes
			if (!Pair.Value.bIsDataNode)
				continue;
			if (Pair.Value.Layer >= 0)
				continue;

			const FDataNodeConnection* DC = DataNodeConnMap.Find(Pair.Key);
			if (!DC)
				continue;

			// Check if immediate consumer has a position
			bool bConsumerPositioned = !DC->ConsumerNodeId.IsEmpty() && HasPosition(DC->ConsumerNodeId);
			bool bSourcePositioned = !DC->SourceNodeId.IsEmpty() && HasPosition(DC->SourceNodeId);

			if (!bConsumerPositioned && !bSourcePositioned)
				continue; // Can't position yet - wait for neighbors

			// Get the positioned neighbor's info
			FNodePlacementInfo* NeighborInfo = nullptr;
			FString NeighborPinName;
			FString MyPinName;
			bool bNeighborIsConsumer = false;

			if (bConsumerPositioned)
			{
				NeighborInfo = PlacementMap.Find(DC->ConsumerNodeId);
				NeighborPinName = DC->ConsumerPinName;
				MyPinName = DC->OutputPinName;
				bNeighborIsConsumer = true;
			}
			else
			{
				NeighborInfo = PlacementMap.Find(DC->SourceNodeId);
				NeighborPinName = DC->SourcePinName;
				MyPinName = DC->InputPinName;
				bNeighborIsConsumer = false;
			}

			if (!NeighborInfo)
				continue;

			// Get UK2Nodes for pin offset calculation
			UK2Node** NeighborNodePtr = NodeMap.Find(NeighborInfo->NodeId);
			UK2Node** DataNodePtr = NodeMap.Find(Pair.Key);

			// Calculate pin Y offsets
			float NeighborPinY = 0.0f;
			float MyPinY = 0.0f;

			if (NeighborNodePtr && *NeighborNodePtr && !NeighborPinName.IsEmpty())
			{
				EEdGraphPinDirection Dir = bNeighborIsConsumer ? EGPD_Input : EGPD_Output;
				NeighborPinY = GetPinYOffset(*NeighborNodePtr, NeighborPinName, Dir);
			}

			if (DataNodePtr && *DataNodePtr && !MyPinName.IsEmpty())
			{
				EEdGraphPinDirection Dir = bNeighborIsConsumer ? EGPD_Output : EGPD_Input;
				MyPinY = GetPinYOffset(*DataNodePtr, MyPinName, Dir);
			}

			// Calculate position
			int32 TargetX, TargetY;

			if (bNeighborIsConsumer)
			{
				// Position to the LEFT of consumer (data flows left to right)
				TargetX = NeighborInfo->PosX - HORIZONTAL_LAYER_SPACING / 2;
				// Spread vertically based on pin position - amplified for visibility
				float PinDelta = (NeighborPinY - MyPinY) * PIN_SPREAD_MULTIPLIER;
				TargetY = NeighborInfo->PosY + static_cast<int32>(PinDelta);
			}
			else
			{
				// Position to the RIGHT of source
				TargetX = NeighborInfo->PosX + HORIZONTAL_LAYER_SPACING / 2;
				// Spread vertically based on pin position - amplified for visibility
				float PinDelta = (NeighborPinY - MyPinY) * PIN_SPREAD_MULTIPLIER;
				TargetY = NeighborInfo->PosY + static_cast<int32>(PinDelta);
			}

			// Debug: log position calculation (v4.20.10 with spread multiplier)
			LogGeneration(FString::Printf(TEXT("    [DEBUG] '%s' -> '%s'.%s: Pos(%d,%d) + PinDelta(%.0f) = Pos(%d,%d)"),
				*Pair.Key, *NeighborInfo->NodeId, *NeighborPinName,
				NeighborInfo->PosX, NeighborInfo->PosY, (NeighborPinY - MyPinY) * PIN_SPREAD_MULTIPLIER,
				TargetX, TargetY));

			// Set position
			Pair.Value.PosX = TargetX;
			Pair.Value.PosY = TargetY;
			Pair.Value.Layer = 0; // Mark as positioned

			DataNodesPositioned++;
			bProgress = true;
		}
	}

	// Handle unpositioned data nodes (no positioned neighbors after all waves)
	int32 UnconnectedDataY = CurrentLaneStartY + LANE_SEPARATION;
	int32 UnconnectedDataNodes = 0;
	for (auto& Pair : PlacementMap)
	{
		if (Pair.Value.bIsDataNode && Pair.Value.Layer < 0)
		{
			Pair.Value.PosX = DATA_NODE_X_OFFSET;
			Pair.Value.PosY = UnconnectedDataY;
			UnconnectedDataY += Pair.Value.EstimatedHeight + VERTICAL_NODE_GAP;
			UnconnectedDataNodes++;
		}
	}

	LogGeneration(FString::Printf(TEXT("  [PLACEMENT] v4.20.10 Wave-based: %d data nodes in %d waves, %d unconnected"),
		DataNodesPositioned, WaveCount, UnconnectedDataNodes));

	// v4.20.10: Grid snap and apply positions to actual nodes
	// CRITICAL: Must call Modify() on graph and nodes for positions to persist
	// See: https://easycomplex-tech.com/blog/Unreal/AssetEditor/UEAssetEditorDev-AssetEditorGraphNode/
	int32 NodesPositioned = 0;
	UEdGraph* ParentGraph = nullptr;

	// Get the graph from the first node
	for (auto& Pair : NodeMap)
	{
		if (Pair.Value && Pair.Value->GetGraph())
		{
			ParentGraph = Pair.Value->GetGraph();
			break;
		}
	}

	// Mark graph for modification (required for position persistence)
	if (ParentGraph)
	{
		ParentGraph->Modify();
	}

	UBlueprint* OwningBlueprint = nullptr;
	FString BlueprintPath;

	for (auto& Pair : PlacementMap)
	{
		UK2Node** NodePtr = NodeMap.Find(Pair.Key);
		if (NodePtr && *NodePtr)
		{
			// Grid snap
			Pair.Value.PosX = SnapToGrid(Pair.Value.PosX);
			Pair.Value.PosY = SnapToGrid(Pair.Value.PosY);

			// Mark node for modification (required for transaction system)
			(*NodePtr)->Modify();

			// Set positions
			(*NodePtr)->NodePosX = Pair.Value.PosX;
			(*NodePtr)->NodePosY = Pair.Value.PosY;
			NodesPositioned++;

			// Get owning Blueprint (needed for MarkBlueprintAsModified and position storage)
			if (!OwningBlueprint)
			{
				OwningBlueprint = (*NodePtr)->GetBlueprint();
				if (OwningBlueprint)
				{
					BlueprintPath = OwningBlueprint->GetPathName();
				}
			}

			// v4.20.11: Store position in static map for reapplication after compile
			if (!BlueprintPath.IsEmpty())
			{
				GStoredNodePositions.FindOrAdd(BlueprintPath).Add((*NodePtr)->NodeGuid, FIntPoint(Pair.Value.PosX, Pair.Value.PosY));
			}
		}
	}

	// v4.20.11: Notify graph and blueprint that positions have changed
	// This ensures positions are serialized when the package is saved
	if (ParentGraph)
	{
		ParentGraph->NotifyGraphChanged();
	}
	if (OwningBlueprint)
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(OwningBlueprint);
	}

	LogGeneration(FString::Printf(TEXT("  [PLACEMENT] Positioned %d nodes across %d lanes, max layer %d (stored %d positions)"),
		NodesPositioned, SortedLanes.Num(), MaxLayerSeen, NodesPositioned));
}

// ============================================================================
// v4.20.11: Position Persistence Functions
// ============================================================================

void FEventGraphGenerator::ClearStoredPositions(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return;
	}

	FString BlueprintPath = Blueprint->GetPathName();
	GStoredNodePositions.Remove(BlueprintPath);
}

void FEventGraphGenerator::ReapplyNodePositions(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return;
	}

	FString BlueprintPath = Blueprint->GetPathName();
	TMap<FGuid, FIntPoint>* StoredPositions = GStoredNodePositions.Find(BlueprintPath);
	if (!StoredPositions || StoredPositions->Num() == 0)
	{
		LogGeneration(FString::Printf(TEXT("  [PLACEMENT] No stored positions to reapply for %s"), *Blueprint->GetName()));
		return;
	}

	int32 RepositionedCount = 0;

	// Iterate all graphs in the Blueprint
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (!Graph)
		{
			continue;
		}

		Graph->Modify();

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				continue;
			}

			FIntPoint* StoredPos = StoredPositions->Find(Node->NodeGuid);
			if (StoredPos)
			{
				Node->Modify();
				Node->NodePosX = StoredPos->X;
				Node->NodePosY = StoredPos->Y;
				RepositionedCount++;
			}
		}

		Graph->NotifyGraphChanged();
	}

	// Also check function graphs
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		Graph->Modify();

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				continue;
			}

			FIntPoint* StoredPos = StoredPositions->Find(Node->NodeGuid);
			if (StoredPos)
			{
				Node->Modify();
				Node->NodePosX = StoredPos->X;
				Node->NodePosY = StoredPos->Y;
				RepositionedCount++;
			}
		}

		Graph->NotifyGraphChanged();
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	LogGeneration(FString::Printf(TEXT("  [PLACEMENT] Reapplied %d/%d stored positions for %s"),
		RepositionedCount, StoredPositions->Num(), *Blueprint->GetName()));
}

// ============================================================================
// v4.22: Diagnostic Logging for Node Position Persistence Audit
// ============================================================================

void FEventGraphGenerator::LogNodePositionsDiagnostic(UBlueprint* Blueprint, const FString& LogPoint)
{
	if (!Blueprint)
	{
		LogGeneration(FString::Printf(TEXT("[POSITION_AUDIT] %s: Blueprint is null"), *LogPoint));
		return;
	}

	LogGeneration(TEXT(""));
	LogGeneration(TEXT("================================================================================"));
	LogGeneration(FString::Printf(TEXT("[POSITION_AUDIT] LogPoint: %s"), *LogPoint));
	LogGeneration(FString::Printf(TEXT("[POSITION_AUDIT] Blueprint: %s"), *Blueprint->GetPathName()));
	LogGeneration(FString::Printf(TEXT("[POSITION_AUDIT] Blueprint Ptr: 0x%p"), Blueprint));
	LogGeneration(FString::Printf(TEXT("[POSITION_AUDIT] UbergraphPages Count: %d"), Blueprint->UbergraphPages.Num()));
	LogGeneration(TEXT("================================================================================"));

	int32 GraphIndex = 0;
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (!Graph)
		{
			LogGeneration(FString::Printf(TEXT("[POSITION_AUDIT]   Graph[%d]: NULL"), GraphIndex));
			GraphIndex++;
			continue;
		}

		LogGeneration(FString::Printf(TEXT("[POSITION_AUDIT]   Graph[%d]: %s"), GraphIndex, *Graph->GetPathName()));
		LogGeneration(FString::Printf(TEXT("[POSITION_AUDIT]   Graph Ptr: 0x%p"), Graph));
		LogGeneration(FString::Printf(TEXT("[POSITION_AUDIT]   Graph Name: %s"), *Graph->GetName()));
		LogGeneration(FString::Printf(TEXT("[POSITION_AUDIT]   Node Count: %d"), Graph->Nodes.Num()));

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				LogGeneration(TEXT("[POSITION_AUDIT]     Node: NULL"));
				continue;
			}

			// Get node class name for identification
			FString NodeClassName = Node->GetClass()->GetName();
			FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();

			LogGeneration(FString::Printf(TEXT("[POSITION_AUDIT]     Node: %s (%s)"), *NodeTitle, *NodeClassName));
			LogGeneration(FString::Printf(TEXT("[POSITION_AUDIT]       Ptr: 0x%p"), Node));
			LogGeneration(FString::Printf(TEXT("[POSITION_AUDIT]       GUID: %s"), *Node->NodeGuid.ToString()));
			LogGeneration(FString::Printf(TEXT("[POSITION_AUDIT]       Pos: X=%d, Y=%d"), Node->NodePosX, Node->NodePosY));
			LogGeneration(FString::Printf(TEXT("[POSITION_AUDIT]       Path: %s"), *Node->GetPathName()));
		}

		GraphIndex++;
	}

	LogGeneration(FString::Printf(TEXT("[POSITION_AUDIT] === End LogPoint: %s ==="), *LogPoint));
	LogGeneration(TEXT(""));
}

// ============================================================================
// v4.13: Category C - P3.2 GameplayCue Auto-Wiring
// ============================================================================

int32 FEventGraphGenerator::GenerateCueTriggerNodes(
	UBlueprint* Blueprint,
	const TArray<FManifestCueTriggerDefinition>& CueTriggers)
{
	if (!Blueprint || CueTriggers.Num() == 0)
	{
		return 0;
	}

	// Find the event graph
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
		LogGeneration(TEXT("  GenerateCueTriggerNodes: No event graph found"));
		return 0;
	}

	int32 NodesGenerated = 0;

	// Helper lambda to find trigger node by event name or function call
	auto FindTriggerNode = [EventGraph](const FString& TriggerType) -> UK2Node*
	{
		if (TriggerType.Equals(TEXT("OnActivate"), ESearchCase::IgnoreCase))
		{
			// Find ActivateAbility event (K2_ActivateAbility or similar)
			for (UEdGraphNode* Node : EventGraph->Nodes)
			{
				if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
				{
					FString EventName = EventNode->GetFunctionName().ToString();
					if (EventName.Contains(TEXT("ActivateAbility")) || EventName.Contains(TEXT("K2_ActivateAbility")))
					{
						return EventNode;
					}
				}
			}
		}
		else if (TriggerType.Equals(TEXT("OnEndAbility"), ESearchCase::IgnoreCase))
		{
			// Find K2_EndAbility function call
			for (UEdGraphNode* Node : EventGraph->Nodes)
			{
				if (UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Node))
				{
					if (CallNode->GetFunctionName().ToString().Contains(TEXT("EndAbility")))
					{
						return CallNode;
					}
				}
			}
		}
		else if (TriggerType.Equals(TEXT("OnCommit"), ESearchCase::IgnoreCase))
		{
			// Find K2_CommitAbility function call
			for (UEdGraphNode* Node : EventGraph->Nodes)
			{
				if (UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Node))
				{
					if (CallNode->GetFunctionName().ToString().Contains(TEXT("CommitAbility")))
					{
						return CallNode;
					}
				}
			}
		}
		return nullptr;
	};

	for (const FManifestCueTriggerDefinition& CueTrigger : CueTriggers)
	{
		UK2Node* TriggerNode = FindTriggerNode(CueTrigger.Trigger);

		if (!TriggerNode)
		{
			LogGeneration(FString::Printf(TEXT("  CueTrigger: Trigger point '%s' not found in event graph"), *CueTrigger.Trigger));
			continue;
		}

		// Create K2_ExecuteGameplayCue function call node
		UK2Node_CallFunction* CueNode = NewObject<UK2Node_CallFunction>(EventGraph);
		EventGraph->AddNode(CueNode, false, false);

		// Find the K2_ExecuteGameplayCue function on UGameplayAbility
		UFunction* ExecuteCueFunc = UGameplayAbility::StaticClass()->FindFunctionByName(TEXT("K2_ExecuteGameplayCue"));
		if (!ExecuteCueFunc)
		{
			LogGeneration(TEXT("  CueTrigger: K2_ExecuteGameplayCue function not found"));
			CueNode->DestroyNode();
			continue;
		}

		CueNode->SetFromFunction(ExecuteCueFunc);
		CueNode->CreateNewGuid();
		CueNode->PostPlacedNewNode();
		CueNode->AllocateDefaultPins();

		// Set the GameplayCueTag pin value
		UEdGraphPin* TagPin = CueNode->FindPin(TEXT("GameplayCueTag"));
		if (TagPin)
		{
			// Format for FGameplayTag default value
			TagPin->DefaultValue = FString::Printf(TEXT("(TagName=\"%s\")"), *CueTrigger.CueTag);
		}

		// Position the node near the trigger
		CueNode->NodePosX = TriggerNode->NodePosX + 300;
		CueNode->NodePosY = TriggerNode->NodePosY;

		// Wire the execution: Insert cue node after trigger node's exec output
		UEdGraphPin* TriggerExecOut = nullptr;
		UEdGraphPin* CueExecIn = CueNode->FindPin(UEdGraphSchema_K2::PN_Execute);
		UEdGraphPin* CueExecOut = CueNode->FindPin(UEdGraphSchema_K2::PN_Then);

		if (CueTrigger.Trigger.Equals(TEXT("OnActivate"), ESearchCase::IgnoreCase))
		{
			// For events, find the "Then" or "exec" output pin
			TriggerExecOut = TriggerNode->FindPin(UEdGraphSchema_K2::PN_Then);
		}
		else
		{
			// For function calls (EndAbility, CommitAbility), find the exec input
			// We want to insert BEFORE the call, so we need to find what's connected to its exec input
			UEdGraphPin* CallExecIn = TriggerNode->FindPin(UEdGraphSchema_K2::PN_Execute);
			if (CallExecIn && CallExecIn->LinkedTo.Num() > 0)
			{
				TriggerExecOut = CallExecIn->LinkedTo[0];
				// Break the existing connection
				TriggerExecOut->BreakLinkTo(CallExecIn);
				// Connect: Source -> CueNode -> OriginalCall
				TriggerExecOut->MakeLinkTo(CueExecIn);
				if (CueExecOut)
				{
					CueExecOut->MakeLinkTo(CallExecIn);
				}
				NodesGenerated++;
				LogGeneration(FString::Printf(TEXT("  Generated CueTrigger: %s at %s (inserted before call)"),
					*CueTrigger.CueTag, *CueTrigger.Trigger));
				continue;
			}
		}

		// For OnActivate: Connect after the event
		if (TriggerExecOut && CueExecIn)
		{
			// Check if there are existing connections
			if (TriggerExecOut->LinkedTo.Num() > 0)
			{
				// Insert cue node in the chain
				UEdGraphPin* NextNode = TriggerExecOut->LinkedTo[0];
				TriggerExecOut->BreakLinkTo(NextNode);
				TriggerExecOut->MakeLinkTo(CueExecIn);
				if (CueExecOut)
				{
					CueExecOut->MakeLinkTo(NextNode);
				}
			}
			else
			{
				// No existing connections, just connect
				TriggerExecOut->MakeLinkTo(CueExecIn);
			}
			NodesGenerated++;
			LogGeneration(FString::Printf(TEXT("  Generated CueTrigger: %s at %s"),
				*CueTrigger.CueTag, *CueTrigger.Trigger));
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  CueTrigger: Failed to wire %s - pins not found"),
				*CueTrigger.CueTag));
		}
	}

	return NodesGenerated;
}

// ============================================================================
// v4.13: Category C - P3.1 Niagara Spawning Support
// ============================================================================

int32 FEventGraphGenerator::GenerateVFXSpawnNodes(
	UBlueprint* Blueprint,
	const TArray<FManifestVFXSpawnDefinition>& VFXSpawns)
{
	if (!Blueprint || VFXSpawns.Num() == 0)
	{
		return 0;
	}

	// Find the event graph
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
		LogGeneration(TEXT("  GenerateVFXSpawnNodes: No event graph found"));
		return 0;
	}

	int32 NodesGenerated = 0;

	// Find the ActivateAbility event
	UK2Node_Event* ActivateEvent = nullptr;
	for (UEdGraphNode* Node : EventGraph->Nodes)
	{
		if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
		{
			FString EventName = EventNode->GetFunctionName().ToString();
			if (EventName.Contains(TEXT("ActivateAbility")))
			{
				ActivateEvent = EventNode;
				break;
			}
		}
	}

	// Find K2_EndAbility call for cleanup
	UK2Node_CallFunction* EndAbilityNode = nullptr;
	for (UEdGraphNode* Node : EventGraph->Nodes)
	{
		if (UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Node))
		{
			if (CallNode->GetFunctionName().ToString().Contains(TEXT("EndAbility")))
			{
				EndAbilityNode = CallNode;
				break;
			}
		}
	}

	for (const FManifestVFXSpawnDefinition& VFXSpawn : VFXSpawns)
	{
		// Create a variable to store the NiagaraComponent reference
		FName VarName = FName(*FString::Printf(TEXT("NiagaraComp_%s"), *VFXSpawn.Id));
		FEdGraphPinType ComponentPinType;
		ComponentPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
		ComponentPinType.PinSubCategoryObject = UNiagaraComponent::StaticClass();

		// Add variable to blueprint (if not exists)
		if (FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarName) == INDEX_NONE)
		{
			FBlueprintEditorUtils::AddMemberVariable(Blueprint, VarName, ComponentPinType);
			LogGeneration(FString::Printf(TEXT("  Added VFX variable: %s"), *VarName.ToString()));
		}

		// Create SpawnSystemAttached node
		UK2Node_CallFunction* SpawnNode = NewObject<UK2Node_CallFunction>(EventGraph);
		EventGraph->AddNode(SpawnNode, false, false);

		// Find UNiagaraFunctionLibrary::SpawnSystemAttached
		UClass* NiagaraLibClass = FindObject<UClass>(nullptr, TEXT("/Script/Niagara.NiagaraFunctionLibrary"));
		if (!NiagaraLibClass)
		{
			LogGeneration(TEXT("  VFXSpawn: NiagaraFunctionLibrary not found"));
			SpawnNode->DestroyNode();
			continue;
		}

		UFunction* SpawnFunc = NiagaraLibClass->FindFunctionByName(TEXT("SpawnSystemAttached"));
		if (!SpawnFunc)
		{
			LogGeneration(TEXT("  VFXSpawn: SpawnSystemAttached function not found"));
			SpawnNode->DestroyNode();
			continue;
		}

		SpawnNode->SetFromFunction(SpawnFunc);
		SpawnNode->CreateNewGuid();
		SpawnNode->PostPlacedNewNode();
		SpawnNode->AllocateDefaultPins();

		// Set NiagaraSystem parameter
		UEdGraphPin* SystemPin = SpawnNode->FindPin(TEXT("SystemTemplate"));
		if (SystemPin)
		{
			// Format as soft object path
			SystemPin->DefaultValue = FString::Printf(TEXT("/Game/%s.%s"),
				*VFXSpawn.NiagaraSystem, *FPaths::GetBaseFilename(VFXSpawn.NiagaraSystem));
		}

		// Set Socket parameter
		if (!VFXSpawn.Socket.IsEmpty())
		{
			UEdGraphPin* SocketPin = SpawnNode->FindPin(TEXT("AttachPointName"));
			if (SocketPin)
			{
				SocketPin->DefaultValue = VFXSpawn.Socket;
			}
		}

		// Position node
		if (ActivateEvent)
		{
			SpawnNode->NodePosX = ActivateEvent->NodePosX + 400 + (NodesGenerated * 200);
			SpawnNode->NodePosY = ActivateEvent->NodePosY + 100;
		}

		// Create VariableSet node to store component reference
		UK2Node_VariableSet* SetVarNode = NewObject<UK2Node_VariableSet>(EventGraph);
		EventGraph->AddNode(SetVarNode, false, false);
		SetVarNode->VariableReference.SetSelfMember(VarName);
		SetVarNode->CreateNewGuid();
		SetVarNode->PostPlacedNewNode();
		SetVarNode->AllocateDefaultPins();
		SetVarNode->NodePosX = SpawnNode->NodePosX + 250;
		SetVarNode->NodePosY = SpawnNode->NodePosY;

		// Wire SpawnNode Return -> SetVar input
		UEdGraphPin* SpawnReturnPin = SpawnNode->FindPin(TEXT("ReturnValue"));
		UEdGraphPin* SetVarValuePin = SetVarNode->FindPin(VarName, EGPD_Input);
		if (SpawnReturnPin && SetVarValuePin)
		{
			SpawnReturnPin->MakeLinkTo(SetVarValuePin);
		}

		// Wire exec: SpawnNode -> SetVarNode
		UEdGraphPin* SpawnExecOut = SpawnNode->FindPin(UEdGraphSchema_K2::PN_Then);
		UEdGraphPin* SetVarExecIn = SetVarNode->FindPin(UEdGraphSchema_K2::PN_Execute);
		if (SpawnExecOut && SetVarExecIn)
		{
			SpawnExecOut->MakeLinkTo(SetVarExecIn);
		}

		// Wire into ActivateAbility
		if (ActivateEvent)
		{
			UEdGraphPin* ActivateExecOut = ActivateEvent->FindPin(UEdGraphSchema_K2::PN_Then);
			UEdGraphPin* SpawnExecIn = SpawnNode->FindPin(UEdGraphSchema_K2::PN_Execute);
			if (ActivateExecOut && SpawnExecIn)
			{
				// Insert at the beginning of the chain
				if (ActivateExecOut->LinkedTo.Num() > 0)
				{
					UEdGraphPin* ExistingNext = ActivateExecOut->LinkedTo[0];
					ActivateExecOut->BreakLinkTo(ExistingNext);
					UEdGraphPin* SetVarExecOut = SetVarNode->FindPin(UEdGraphSchema_K2::PN_Then);
					if (SetVarExecOut)
					{
						SetVarExecOut->MakeLinkTo(ExistingNext);
					}
				}
				ActivateExecOut->MakeLinkTo(SpawnExecIn);
			}
		}

		// If destroy_on_end_ability, add cleanup in EndAbility
		if (VFXSpawn.bDestroyOnEndAbility && EndAbilityNode)
		{
			// Create IsValid check
			UK2Node_CallFunction* IsValidNode = NewObject<UK2Node_CallFunction>(EventGraph);
			EventGraph->AddNode(IsValidNode, false, false);
			UFunction* IsValidFunc = UKismetSystemLibrary::StaticClass()->FindFunctionByName(TEXT("IsValid"));
			if (IsValidFunc)
			{
				IsValidNode->SetFromFunction(IsValidFunc);
				IsValidNode->CreateNewGuid();
				IsValidNode->PostPlacedNewNode();
				IsValidNode->AllocateDefaultPins();
				IsValidNode->NodePosX = EndAbilityNode->NodePosX - 400;
				IsValidNode->NodePosY = EndAbilityNode->NodePosY + 100;

				// Create VariableGet for the component
				UK2Node_VariableGet* GetVarNode = NewObject<UK2Node_VariableGet>(EventGraph);
				EventGraph->AddNode(GetVarNode, false, false);
				GetVarNode->VariableReference.SetSelfMember(VarName);
				GetVarNode->CreateNewGuid();
				GetVarNode->PostPlacedNewNode();
				GetVarNode->AllocateDefaultPins();
				GetVarNode->NodePosX = IsValidNode->NodePosX - 150;
				GetVarNode->NodePosY = IsValidNode->NodePosY;

				// Wire GetVar -> IsValid input
				UEdGraphPin* GetVarOut = GetVarNode->FindPin(VarName);
				UEdGraphPin* IsValidInput = IsValidNode->FindPin(TEXT("Object"));
				if (GetVarOut && IsValidInput)
				{
					GetVarOut->MakeLinkTo(IsValidInput);
				}

				// Create DestroyComponent call
				UK2Node_CallFunction* DestroyNode = NewObject<UK2Node_CallFunction>(EventGraph);
				EventGraph->AddNode(DestroyNode, false, false);
				UFunction* DestroyFunc = UActorComponent::StaticClass()->FindFunctionByName(TEXT("DestroyComponent"));
				if (DestroyFunc)
				{
					DestroyNode->SetFromFunction(DestroyFunc);
					DestroyNode->CreateNewGuid();
					DestroyNode->PostPlacedNewNode();
					DestroyNode->AllocateDefaultPins();
					DestroyNode->NodePosX = IsValidNode->NodePosX + 200;
					DestroyNode->NodePosY = IsValidNode->NodePosY;

					// Wire GetVar -> DestroyComponent Target
					UEdGraphPin* DestroyTarget = DestroyNode->FindPin(TEXT("self"));
					if (GetVarOut && DestroyTarget)
					{
						// Need another GetVar for target (or reuse)
						GetVarOut->MakeLinkTo(DestroyTarget);
					}

					// Create Branch for IsValid check
					UK2Node_IfThenElse* BranchNode = NewObject<UK2Node_IfThenElse>(EventGraph);
					EventGraph->AddNode(BranchNode, false, false);
					BranchNode->CreateNewGuid();
					BranchNode->PostPlacedNewNode();
					BranchNode->AllocateDefaultPins();
					BranchNode->NodePosX = IsValidNode->NodePosX + 100;
					BranchNode->NodePosY = IsValidNode->NodePosY - 50;

					// Wire IsValid return -> Branch condition
					UEdGraphPin* IsValidReturn = IsValidNode->FindPin(TEXT("ReturnValue"));
					UEdGraphPin* BranchCondition = BranchNode->FindPin(TEXT("Condition"));
					if (IsValidReturn && BranchCondition)
					{
						IsValidReturn->MakeLinkTo(BranchCondition);
					}

					// Wire Branch True -> Destroy
					UEdGraphPin* BranchTrue = BranchNode->FindPin(TEXT("IfTrue"));
					UEdGraphPin* DestroyExec = DestroyNode->FindPin(UEdGraphSchema_K2::PN_Execute);
					if (BranchTrue && DestroyExec)
					{
						BranchTrue->MakeLinkTo(DestroyExec);
					}

					// Insert before EndAbility
					UEdGraphPin* EndAbilityExecIn = EndAbilityNode->FindPin(UEdGraphSchema_K2::PN_Execute);
					if (EndAbilityExecIn && EndAbilityExecIn->LinkedTo.Num() > 0)
					{
						UEdGraphPin* PrevExecOut = EndAbilityExecIn->LinkedTo[0];
						PrevExecOut->BreakLinkTo(EndAbilityExecIn);

						// Wire: Previous -> Branch -> EndAbility
						UEdGraphPin* BranchExecIn = BranchNode->FindPin(UEdGraphSchema_K2::PN_Execute);
						if (BranchExecIn)
						{
							PrevExecOut->MakeLinkTo(BranchExecIn);
						}

						// Wire both Branch outputs to EndAbility
						UEdGraphPin* BranchFalse = BranchNode->FindPin(TEXT("IfFalse"));
						UEdGraphPin* DestroyExecOut = DestroyNode->FindPin(UEdGraphSchema_K2::PN_Then);
						if (BranchFalse)
						{
							BranchFalse->MakeLinkTo(EndAbilityExecIn);
						}
						if (DestroyExecOut)
						{
							DestroyExecOut->MakeLinkTo(EndAbilityExecIn);
						}
					}
				}
			}
		}

		NodesGenerated++;
		LogGeneration(FString::Printf(TEXT("  Generated VFXSpawn: %s (%s, socket: %s, cleanup: %s)"),
			*VFXSpawn.Id, *VFXSpawn.NiagaraSystem,
			VFXSpawn.Socket.IsEmpty() ? TEXT("none") : *VFXSpawn.Socket,
			VFXSpawn.bDestroyOnEndAbility ? TEXT("yes") : TEXT("no")));
	}

	return NodesGenerated;
}

// ============================================================================
// v4.13: Category C - P2.1 Delegate Binding IR + Codegen
// ============================================================================

/**
 * v4.21: Generate delegate binding nodes for GameplayAbility blueprints
 * Audit-approved: Claude-GPT dual audit 2026-01-21
 *
 * Creates full bind/unbind node chains:
 * - Custom Event with delegate signature parameters
 * - CreateDelegate_A → AddDelegate (in ActivateAbility path)
 * - CreateDelegate_B → RemoveDelegate (in EndAbility path)
 * - Proper PN_Self wiring for all delegate nodes
 * - Cast to UNarrativeAbilitySystemComponent when source is OwnerASC/PlayerASC
 */
int32 FEventGraphGenerator::GenerateDelegateBindingNodes(
	UBlueprint* Blueprint,
	const TArray<FManifestDelegateBindingDefinition>& DelegateBindings)
{
	if (!Blueprint || DelegateBindings.Num() == 0)
	{
		return 0;
	}

	// Find the event graph
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
		LogGeneration(TEXT("  [E_DELEGATE_NO_GRAPH] GenerateDelegateBindingNodes: No event graph found"));
		return 0;
	}

	// Get the schema for connection validation
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	// Source class for delegate lookup (Narrative Pro delegates)
	UClass* NarrativeASCClass = UNarrativeAbilitySystemComponent::StaticClass();

	int32 NodesGenerated = 0;
	float CurrentPosX = 1500.0f;
	float CurrentPosY = 0.0f;

	// Track nodes for exec wiring
	TArray<UK2Node_AddDelegate*> AddDelegateNodes;
	TArray<UK2Node_RemoveDelegate*> RemoveDelegateNodes;
	// v4.21.2: Track CastNodes separately for each path (required for exec reachability)
	TArray<UK2Node_DynamicCast*> CastNodesForAdd;
	TArray<UK2Node_DynamicCast*> CastNodesForRemove;

	for (const FManifestDelegateBindingDefinition& Binding : DelegateBindings)
	{
		FString BindingId = Binding.Id.IsEmpty() ? Binding.Handler : Binding.Id;
		LogGeneration(FString::Printf(TEXT("  Processing delegate binding: %s"), *BindingId));

		// 1. Find delegate property and get signature
		FMulticastDelegateProperty* DelegateProp = FindFProperty<FMulticastDelegateProperty>(
			NarrativeASCClass, FName(*Binding.Delegate));

		if (!DelegateProp)
		{
			LogGeneration(FString::Printf(TEXT("    [E_DELEGATE_NOT_FOUND] Delegate '%s' not found on UNarrativeAbilitySystemComponent"),
				*Binding.Delegate));
			continue;
		}

		UFunction* DelegateSignature = DelegateProp->SignatureFunction;
		if (!DelegateSignature)
		{
			LogGeneration(FString::Printf(TEXT("    [E_DELEGATE_NO_SIGNATURE] No signature function for delegate '%s'"),
				*Binding.Delegate));
			continue;
		}

		// 2. Create Custom Event with delegate signature parameters
		UK2Node_CustomEvent* HandlerEvent = NewObject<UK2Node_CustomEvent>(EventGraph);
		EventGraph->AddNode(HandlerEvent, false, false);
		HandlerEvent->CustomFunctionName = FName(*Binding.Handler);
		HandlerEvent->bIsEditable = true;
		HandlerEvent->CreateNewGuid();
		HandlerEvent->PostPlacedNewNode();
		HandlerEvent->AllocateDefaultPins();
		HandlerEvent->NodePosX = CurrentPosX;
		HandlerEvent->NodePosY = CurrentPosY;

		// Add parameters from delegate signature
		for (TFieldIterator<FProperty> PropIt(DelegateSignature); PropIt; ++PropIt)
		{
			FProperty* Param = *PropIt;
			if (Param->HasAnyPropertyFlags(CPF_Parm) && !Param->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				FEdGraphPinType PinType;
				Schema->ConvertPropertyToPinType(Param, PinType);

				HandlerEvent->CreateUserDefinedPin(
					Param->GetFName(),
					PinType,
					EGPD_Output
				);
			}
		}

		LogGeneration(FString::Printf(TEXT("    Created handler event: %s with signature from %s"),
			*Binding.Handler, *DelegateSignature->GetName()));

		// 3. Create Self node for ability reference (used by CreateDelegate.PN_Self)
		UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(EventGraph);
		EventGraph->AddNode(SelfNode, false, false);
		SelfNode->CreateNewGuid();
		SelfNode->PostPlacedNewNode();
		SelfNode->AllocateDefaultPins();
		SelfNode->NodePosX = CurrentPosX + 200.0f;
		SelfNode->NodePosY = CurrentPosY + 100.0f;
		// v4.21.2 FIX: UK2Node_Self uses PN_Self for its output, not PN_ReturnValue
		UEdGraphPin* SelfOutPin = SelfNode->FindPin(UEdGraphSchema_K2::PN_Self);

		// 4. Create source ASC resolution nodes (with cast for OwnerASC/PlayerASC)
		// v4.21.2: Create separate CastNodes for Activate and End paths (exec reachability)
		UEdGraphPin* SourceASCPinForAdd = nullptr;
		UEdGraphPin* SourceASCPinForRemove = nullptr;
		UK2Node_DynamicCast* CastNodeA = nullptr;  // For ActivateAbility path
		UK2Node_DynamicCast* CastNodeB = nullptr;  // For EndAbility path
		bool bNeedsCast = Binding.Source.Equals(TEXT("OwnerASC"), ESearchCase::IgnoreCase) ||
		                  Binding.Source.Equals(TEXT("PlayerASC"), ESearchCase::IgnoreCase);

		if (bNeedsCast)
		{
			// v4.22: Fixed ASC resolution - use GetAbilitySystemComponentFromActorInfo() on UGameplayAbility
			// NOT GetAbilitySystemComponent(Actor) which requires an Actor input
			UK2Node_CallFunction* GetASCNode = NewObject<UK2Node_CallFunction>(EventGraph);
			EventGraph->AddNode(GetASCNode, false, false);

			UFunction* GetASCFunc = UGameplayAbility::StaticClass()->FindFunctionByName(
				TEXT("GetAbilitySystemComponentFromActorInfo"));
			if (GetASCFunc)
			{
				GetASCNode->SetFromFunction(GetASCFunc);
			}
			GetASCNode->CreateNewGuid();
			GetASCNode->PostPlacedNewNode();
			GetASCNode->AllocateDefaultPins();
			GetASCNode->NodePosX = CurrentPosX + 200.0f;
			GetASCNode->NodePosY = CurrentPosY + 200.0f;

			// Wire Self to GetASCNode.Self pin (target_self pattern)
			// v4.21.2: DIAGNOSTIC - Log all input pins of GetASCNode to detect hidden/missing Self pin
			{
				LogGeneration(TEXT("    [DIAG-SELF] GetASCNode input pins:"));
				for (UEdGraphPin* Pin : GetASCNode->Pins)
				{
					if (Pin->Direction == EGPD_Input)
					{
						LogGeneration(FString::Printf(TEXT("      - Pin: %s, Category=%s, SubCategoryObject=%s"),
							*Pin->PinName.ToString(),
							*Pin->PinType.PinCategory.ToString(),
							Pin->PinType.PinSubCategoryObject.IsValid()
								? *Pin->PinType.PinSubCategoryObject->GetName()
								: TEXT("null")));
					}
				}
			}

			UEdGraphPin* GetASCSelfPin = GetASCNode->FindPin(UEdGraphSchema_K2::PN_Self);
			if (GetASCSelfPin && SelfOutPin)
			{
				// Log pin details before wiring
				LogGeneration(FString::Printf(TEXT("    [DIAG-SELF] Pre-wire: SelfOutPin.Direction=%s, OwningNode=%s"),
					SelfOutPin->Direction == EGPD_Output ? TEXT("Output") : TEXT("Input"),
					*SelfOutPin->GetOwningNode()->GetClass()->GetName()));
				LogGeneration(FString::Printf(TEXT("    [DIAG-SELF] Pre-wire: GetASCSelfPin.Direction=%s, OwningNode=%s"),
					GetASCSelfPin->Direction == EGPD_Output ? TEXT("Output") : TEXT("Input"),
					*GetASCSelfPin->GetOwningNode()->GetClass()->GetName()));

				SelfOutPin->MakeLinkTo(GetASCSelfPin);

				// Verify bidirectional link after MakeLinkTo
				bool bForwardLink = GetASCSelfPin->LinkedTo.Contains(SelfOutPin);
				bool bReverseLink = SelfOutPin->LinkedTo.Contains(GetASCSelfPin);
				LogGeneration(FString::Printf(TEXT("    [DIAG-SELF] Post-wire: GetASCSelfPin.LinkedTo.Contains(SelfOutPin)=%s"),
					bForwardLink ? TEXT("YES") : TEXT("NO")));
				LogGeneration(FString::Printf(TEXT("    [DIAG-SELF] Post-wire: SelfOutPin.LinkedTo.Contains(GetASCSelfPin)=%s"),
					bReverseLink ? TEXT("YES"): TEXT("NO")));
				LogGeneration(FString::Printf(TEXT("    [DIAG-SELF] Post-wire: GetASCSelfPin.LinkedTo.Num()=%d, SelfOutPin.LinkedTo.Num()=%d"),
					GetASCSelfPin->LinkedTo.Num(), SelfOutPin->LinkedTo.Num()));
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("    [DIAG-SELF] Self pin wire SKIPPED: GetASCSelfPin=%s, SelfOutPin=%s"),
					GetASCSelfPin ? TEXT("valid") : TEXT("null"),
					SelfOutPin ? TEXT("valid") : TEXT("null")));
			}

			UEdGraphPin* ASCOutPin = GetASCNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);

			// v4.21.2: Create CastNodeA for ActivateAbility path
			CastNodeA = NewObject<UK2Node_DynamicCast>(EventGraph);
			CastNodeA->TargetType = NarrativeASCClass;
			EventGraph->AddNode(CastNodeA, false, false);
			CastNodeA->CreateNewGuid();
			CastNodeA->PostPlacedNewNode();
			CastNodeA->AllocateDefaultPins();
			CastNodeA->NodePosX = CurrentPosX + 400.0f;
			CastNodeA->NodePosY = CurrentPosY + 100.0f;

			// Wire GetASC.ReturnValue → CastNodeA.Object
			UEdGraphPin* CastAInPin = CastNodeA->GetCastSourcePin();
			if (ASCOutPin && CastAInPin)
			{
				ASCOutPin->MakeLinkTo(CastAInPin);
				// v4.21.2 FIX: Trigger pin type propagation after connection
				CastNodeA->NotifyPinConnectionListChanged(CastAInPin);
				LogGeneration(FString::Printf(TEXT("    [DEBUG] CastNodeA data wired: ASCOutPin=%s, CastAInPin=%s, LinkedTo=%d"),
					*ASCOutPin->PinName.ToString(),
					*CastAInPin->PinName.ToString(),
					CastAInPin->LinkedTo.Num()));
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("    [ERROR] CastNodeA data wire FAILED: ASCOutPin=%s, CastAInPin=%s"),
					ASCOutPin ? TEXT("valid") : TEXT("null"),
					CastAInPin ? TEXT("valid") : TEXT("null")));
			}
			SourceASCPinForAdd = CastNodeA->GetCastResultPin();

			// v4.21.2: Create CastNodeB for EndAbility path (separate exec chain)
			CastNodeB = NewObject<UK2Node_DynamicCast>(EventGraph);
			CastNodeB->TargetType = NarrativeASCClass;
			EventGraph->AddNode(CastNodeB, false, false);
			CastNodeB->CreateNewGuid();
			CastNodeB->PostPlacedNewNode();
			CastNodeB->AllocateDefaultPins();
			CastNodeB->NodePosX = CurrentPosX + 400.0f;
			CastNodeB->NodePosY = CurrentPosY + 400.0f;

			// Wire GetASC.ReturnValue → CastNodeB.Object (same source, different cast node)
			UEdGraphPin* CastBInPin = CastNodeB->GetCastSourcePin();
			if (ASCOutPin && CastBInPin)
			{
				ASCOutPin->MakeLinkTo(CastBInPin);
				// v4.21.2 FIX: Trigger pin type propagation after connection
				CastNodeB->NotifyPinConnectionListChanged(CastBInPin);
				LogGeneration(FString::Printf(TEXT("    [DEBUG] CastNodeB data wired: ASCOutPin=%s, CastBInPin=%s, LinkedTo=%d"),
					*ASCOutPin->PinName.ToString(),
					*CastBInPin->PinName.ToString(),
					CastBInPin->LinkedTo.Num()));
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("    [ERROR] CastNodeB data wire FAILED: ASCOutPin=%s, CastBInPin=%s"),
					ASCOutPin ? TEXT("valid") : TEXT("null"),
					CastBInPin ? TEXT("valid") : TEXT("null")));
			}
			SourceASCPinForRemove = CastNodeB->GetCastResultPin();

			// v4.21.2: DIAGNOSTIC - comprehensive pin type and linkage audit
			{
				// A) ASCOutPin diagnostics
				LogGeneration(FString::Printf(TEXT("    [DIAG-PIN] ASCOutPin PinType: Category=%s, SubCategory=%s, SubCategoryObject=%s"),
					*ASCOutPin->PinType.PinCategory.ToString(),
					*ASCOutPin->PinType.PinSubCategory.ToString(),
					ASCOutPin->PinType.PinSubCategoryObject.IsValid()
						? *ASCOutPin->PinType.PinSubCategoryObject->GetName()
						: TEXT("null")));
				LogGeneration(FString::Printf(TEXT("    [DIAG-PIN] ASCOutPin LinkedTo.Num()=%d"), ASCOutPin->LinkedTo.Num()));
				for (int32 li = 0; li < ASCOutPin->LinkedTo.Num(); ++li)
				{
					UEdGraphPin* LinkedPin = ASCOutPin->LinkedTo[li];
					LogGeneration(FString::Printf(TEXT("    [DIAG-PIN]   LinkedTo[%d]: Node=%s, Pin=%s"),
						li,
						LinkedPin ? *LinkedPin->GetOwningNode()->GetClass()->GetName() : TEXT("null"),
						LinkedPin ? *LinkedPin->PinName.ToString() : TEXT("null")));
				}

				// B) GetASCNode function binding
				LogGeneration(FString::Printf(TEXT("    [DIAG-PIN] GetASCNode class=%s"), *GetASCNode->GetClass()->GetName()));
				UFunction* BoundFunc = GetASCNode->GetTargetFunction();
				LogGeneration(FString::Printf(TEXT("    [DIAG-PIN] GetASCNode bound function: %s::%s"),
					BoundFunc && BoundFunc->GetOwnerClass() ? *BoundFunc->GetOwnerClass()->GetName() : TEXT("null"),
					BoundFunc ? *BoundFunc->GetName() : TEXT("null")));
				UEdGraphPin* GetASCSelf = GetASCNode->FindPin(UEdGraphSchema_K2::PN_Self);
				LogGeneration(FString::Printf(TEXT("    [DIAG-PIN] GetASCNode Self pin: %s, LinkedTo=%d"),
					GetASCSelf ? TEXT("exists") : TEXT("null"),
					GetASCSelf ? GetASCSelf->LinkedTo.Num() : -1));

				// C) CastNodeA Object pin
				LogGeneration(FString::Printf(TEXT("    [DIAG-PIN] CastNodeA TargetType=%s"),
					CastNodeA->TargetType ? *CastNodeA->TargetType->GetName() : TEXT("null")));
				LogGeneration(FString::Printf(TEXT("    [DIAG-PIN] CastAInPin LinkedTo.Num()=%d, PinType.SubCategoryObject=%s"),
					CastAInPin->LinkedTo.Num(),
					CastAInPin->PinType.PinSubCategoryObject.IsValid()
						? *CastAInPin->PinType.PinSubCategoryObject->GetName()
						: TEXT("null")));

				// D) CastNodeB Object pin
				LogGeneration(FString::Printf(TEXT("    [DIAG-PIN] CastNodeB TargetType=%s"),
					CastNodeB->TargetType ? *CastNodeB->TargetType->GetName() : TEXT("null")));
				LogGeneration(FString::Printf(TEXT("    [DIAG-PIN] CastBInPin LinkedTo.Num()=%d, PinType.SubCategoryObject=%s"),
					CastBInPin->LinkedTo.Num(),
					CastBInPin->PinType.PinSubCategoryObject.IsValid()
						? *CastBInPin->PinType.PinSubCategoryObject->GetName()
						: TEXT("null")));
			}

			LogGeneration(FString::Printf(TEXT("    Created dual ASC resolution chains with cast for %s"), *Binding.Source));
		}
		else
		{
			// Variable lookup - find Blueprint variable by name
			FName VarName = FName(*Binding.Source);
			bool bVariableFound = false;

			// Check if variable exists in Blueprint
			for (const FBPVariableDescription& VarDesc : Blueprint->NewVariables)
			{
				if (VarDesc.VarName == VarName)
				{
					bVariableFound = true;
					break;
				}
			}

			if (bVariableFound)
			{
				// Create VariableGet node
				UK2Node_VariableGet* GetVarNode = NewObject<UK2Node_VariableGet>(EventGraph);
				EventGraph->AddNode(GetVarNode, false, false);
				GetVarNode->VariableReference.SetSelfMember(VarName);
				GetVarNode->CreateNewGuid();
				GetVarNode->PostPlacedNewNode();
				GetVarNode->AllocateDefaultPins();
				GetVarNode->NodePosX = CurrentPosX + 200.0f;
				GetVarNode->NodePosY = CurrentPosY + 200.0f;

				// Find the variable output pin
				UEdGraphPin* VarOutPin = GetVarNode->FindPin(VarName);
				if (!VarOutPin)
				{
					// Try finding by return value
					VarOutPin = GetVarNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
				}
				if (!VarOutPin)
				{
					// Find first non-exec output pin
					for (UEdGraphPin* Pin : GetVarNode->Pins)
					{
						if (Pin && Pin->Direction == EGPD_Output &&
						    Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
						{
							VarOutPin = Pin;
							break;
						}
					}
				}

				if (VarOutPin)
				{
					// v4.22: Section 10 - External ASC Binding (Actor→ASC Resolution)
					// Detect if variable is an Actor type (not an ASC) and extract ASC first
					bool bNeedsASCExtraction = false;
					bool bNeedsVarCast = false;
					UEdGraphPin* CurrentSourcePin = VarOutPin;

					if (VarOutPin->PinType.PinSubCategoryObject.IsValid())
					{
						UClass* VarClass = Cast<UClass>(VarOutPin->PinType.PinSubCategoryObject.Get());
						if (VarClass)
						{
							// Check if variable is Actor type (but not ASC - ASCs are also Actors via component owner)
							bool bIsActorType = VarClass->IsChildOf(AActor::StaticClass());
							bool bIsASCType = VarClass->IsChildOf(UAbilitySystemComponent::StaticClass());

							if (bIsActorType && !bIsASCType)
							{
								// Actor variable - need GetAbilitySystemComponent extraction
								bNeedsASCExtraction = true;
								LogGeneration(FString::Printf(TEXT("    Variable '%s' is Actor type - extracting ASC"), *Binding.Source));
							}
							else if (!VarClass->IsChildOf(NarrativeASCClass))
							{
								// Not NarrativeASCClass - needs cast
								bNeedsVarCast = true;
							}
						}
					}

					// Step 1: If Actor type, insert GetAbilitySystemComponent
					if (bNeedsASCExtraction)
					{
						UK2Node_CallFunction* ExtractASCNode = NewObject<UK2Node_CallFunction>(EventGraph);
						EventGraph->AddNode(ExtractASCNode, false, false);

						UFunction* GetASCFunc = UAbilitySystemBlueprintLibrary::StaticClass()->FindFunctionByName(
							TEXT("GetAbilitySystemComponent"));
						if (GetASCFunc)
						{
							ExtractASCNode->SetFromFunction(GetASCFunc);
						}
						ExtractASCNode->CreateNewGuid();
						ExtractASCNode->PostPlacedNewNode();
						ExtractASCNode->AllocateDefaultPins();
						ExtractASCNode->NodePosX = CurrentPosX + 350.0f;
						ExtractASCNode->NodePosY = CurrentPosY + 200.0f;

						// Wire Variable → GetASC.Actor
						UEdGraphPin* ActorInputPin = ExtractASCNode->FindPin(TEXT("Actor"));
						if (ActorInputPin && VarOutPin)
						{
							VarOutPin->MakeLinkTo(ActorInputPin);
						}

						// Update current source to GetASC output
						CurrentSourcePin = ExtractASCNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
						bNeedsVarCast = true; // Always need cast after GetAbilitySystemComponent (returns base ASC)

						LogGeneration(FString::Printf(TEXT("    Inserted GetAbilitySystemComponent for Actor variable '%s'"), *Binding.Source));
					}

					// Step 2: If needed, cast to NarrativeASCClass
					// v4.21.2: Create dual cast nodes for exec reachability (like OwnerASC/PlayerASC case)
					if (bNeedsVarCast && CurrentSourcePin)
					{
						// Create VarCastNodeA for ActivateAbility path
						UK2Node_DynamicCast* VarCastNodeA = NewObject<UK2Node_DynamicCast>(EventGraph);
						VarCastNodeA->TargetType = NarrativeASCClass;
						EventGraph->AddNode(VarCastNodeA, false, false);
						VarCastNodeA->CreateNewGuid();
						VarCastNodeA->PostPlacedNewNode();
						VarCastNodeA->AllocateDefaultPins();
						VarCastNodeA->NodePosX = CurrentPosX + (bNeedsASCExtraction ? 550.0f : 400.0f);
						VarCastNodeA->NodePosY = CurrentPosY + 100.0f;

						// Wire CurrentSource → VarCastNodeA
						UEdGraphPin* VarCastAInPin = VarCastNodeA->GetCastSourcePin();
						if (VarCastAInPin)
						{
							CurrentSourcePin->MakeLinkTo(VarCastAInPin);
						}
						SourceASCPinForAdd = VarCastNodeA->GetCastResultPin();
						CastNodeA = VarCastNodeA;  // Store for exec wiring

						// Create VarCastNodeB for EndAbility path
						UK2Node_DynamicCast* VarCastNodeB = NewObject<UK2Node_DynamicCast>(EventGraph);
						VarCastNodeB->TargetType = NarrativeASCClass;
						EventGraph->AddNode(VarCastNodeB, false, false);
						VarCastNodeB->CreateNewGuid();
						VarCastNodeB->PostPlacedNewNode();
						VarCastNodeB->AllocateDefaultPins();
						VarCastNodeB->NodePosX = CurrentPosX + (bNeedsASCExtraction ? 550.0f : 400.0f);
						VarCastNodeB->NodePosY = CurrentPosY + 400.0f;

						// Wire CurrentSource → VarCastNodeB (same source, different cast node)
						UEdGraphPin* VarCastBInPin = VarCastNodeB->GetCastSourcePin();
						if (VarCastBInPin)
						{
							CurrentSourcePin->MakeLinkTo(VarCastBInPin);
						}
						SourceASCPinForRemove = VarCastNodeB->GetCastResultPin();
						CastNodeB = VarCastNodeB;  // Store for exec wiring

						LogGeneration(FString::Printf(TEXT("    Created dual variable casts for '%s' to UNarrativeAbilitySystemComponent"), *Binding.Source));
					}
					else if (CurrentSourcePin)
					{
						// Variable is already correct type, use directly (pure getter, no exec issues)
						SourceASCPinForAdd = CurrentSourcePin;
						SourceASCPinForRemove = CurrentSourcePin;
						LogGeneration(FString::Printf(TEXT("    Created variable getter for '%s' (no cast needed)"), *Binding.Source));
					}
				}
				else
				{
					LogGeneration(FString::Printf(TEXT("    [E_DELEGATE_VARIABLE_PIN] Cannot find output pin for variable '%s'"),
						*Binding.Source));
				}
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("    [E_DELEGATE_SOURCE_INVALID] Variable '%s' not found in Blueprint"),
					*Binding.Source));
			}
		}

		// 5. Create delegate reference for Add/Remove nodes
		FMemberReference DelegateRef;
		DelegateRef.SetExternalMember(FName(*Binding.Delegate), NarrativeASCClass);

		// === ACTIVATE PATH ===

		// 6. CreateDelegate_A for ActivateAbility path
		UK2Node_CreateDelegate* CreateDelegateA = NewObject<UK2Node_CreateDelegate>(EventGraph);
		CreateDelegateA->SetFunction(FName(*Binding.Handler));
		EventGraph->AddNode(CreateDelegateA, false, false);
		CreateDelegateA->CreateNewGuid();
		CreateDelegateA->PostPlacedNewNode();
		CreateDelegateA->AllocateDefaultPins();
		// v4.21.2: Do NOT call ReconstructNode() on CreateDelegate - it clears SelectedFunctionName
		// when OutputDelegate pin is not yet wired (proven via diagnostic logging)
		CreateDelegateA->NodePosX = CurrentPosX + 600.0f;
		CreateDelegateA->NodePosY = CurrentPosY + 100.0f;

		// Wire CreateDelegate_A.PN_Self → Ability Self
		UEdGraphPin* CreateASelfPin = CreateDelegateA->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input);
		if (SelfOutPin && CreateASelfPin)
		{
			SelfOutPin->MakeLinkTo(CreateASelfPin);
		}

		// 7. AddDelegate node
		UK2Node_AddDelegate* AddDelegateNode = NewObject<UK2Node_AddDelegate>(EventGraph);
		AddDelegateNode->DelegateReference = DelegateRef;
		EventGraph->AddNode(AddDelegateNode, false, false);
		AddDelegateNode->CreateNewGuid();
		AddDelegateNode->PostPlacedNewNode();
		AddDelegateNode->AllocateDefaultPins();
		AddDelegateNode->NodePosX = CurrentPosX + 800.0f;
		AddDelegateNode->NodePosY = CurrentPosY + 100.0f;

		// Wire CreateDelegate_A.OutputDelegate → AddDelegate.Delegate
		UEdGraphPin* DelegateAOut = CreateDelegateA->FindPin(TEXT("OutputDelegate"), EGPD_Output);
		UEdGraphPin* AddDelegateIn = AddDelegateNode->FindPin(TEXT("Delegate"), EGPD_Input);
		if (DelegateAOut && AddDelegateIn)
		{
			DelegateAOut->MakeLinkTo(AddDelegateIn);
		}

		// Wire SourceASC → AddDelegate.PN_Self (Target)
		UEdGraphPin* AddSelfPin = AddDelegateNode->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input);

		// v4.22: Debug logging for pin connection - ALWAYS list pins
		{
			FString AvailablePins;
			for (UEdGraphPin* Pin : AddDelegateNode->Pins)
			{
				if (Pin && Pin->Direction == EGPD_Input)
				{
					if (!AvailablePins.IsEmpty()) AvailablePins += TEXT(", ");
					AvailablePins += Pin->PinName.ToString();
				}
			}
			LogGeneration(FString::Printf(TEXT("    [DEBUG] AddDelegate pin check: SourceASCPinForAdd=%s, AddSelfPin=%s"),
				SourceASCPinForAdd ? TEXT("valid") : TEXT("null"),
				AddSelfPin ? TEXT("valid") : TEXT("null")));
			LogGeneration(FString::Printf(TEXT("    [DEBUG] AddDelegate ALL input pins: %s"), *AvailablePins));
		}

		if (SourceASCPinForAdd && AddSelfPin)
		{
			SourceASCPinForAdd->MakeLinkTo(AddSelfPin);
			// Verify connection was actually made
			bool bConnectionExists = AddSelfPin->LinkedTo.Contains(SourceASCPinForAdd);
			LogGeneration(FString::Printf(TEXT("    [DEBUG] AddDelegate Target connection made: verified=%s, LinkedTo.Num=%d"),
				bConnectionExists ? TEXT("YES") : TEXT("NO"), AddSelfPin->LinkedTo.Num()));
		}
		else
		{
			LogGeneration(TEXT("    [ERROR] AddDelegate Target connection FAILED - pin(s) null"));
		}

		// v4.21.2: Store CastNode and AddDelegate together for exec wiring
		AddDelegateNodes.Add(AddDelegateNode);
		CastNodesForAdd.Add(CastNodeA);  // May be nullptr if no cast needed

		// === END PATH ===

		// 8. CreateDelegate_B for EndAbility path (separate node, same function)
		UK2Node_CreateDelegate* CreateDelegateB = NewObject<UK2Node_CreateDelegate>(EventGraph);
		CreateDelegateB->SetFunction(FName(*Binding.Handler));
		EventGraph->AddNode(CreateDelegateB, false, false);
		CreateDelegateB->CreateNewGuid();
		CreateDelegateB->PostPlacedNewNode();
		CreateDelegateB->AllocateDefaultPins();
		// v4.21.2: Do NOT call ReconstructNode() on CreateDelegate - it clears SelectedFunctionName
		// when OutputDelegate pin is not yet wired (proven via diagnostic logging)
		CreateDelegateB->NodePosX = CurrentPosX + 600.0f;
		CreateDelegateB->NodePosY = CurrentPosY + 400.0f;

		// Wire CreateDelegate_B.PN_Self → Ability Self
		UEdGraphPin* CreateBSelfPin = CreateDelegateB->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input);
		if (SelfOutPin && CreateBSelfPin)
		{
			SelfOutPin->MakeLinkTo(CreateBSelfPin);
		}

		// 9. RemoveDelegate node
		UK2Node_RemoveDelegate* RemoveDelegateNode = NewObject<UK2Node_RemoveDelegate>(EventGraph);
		RemoveDelegateNode->DelegateReference = DelegateRef;
		EventGraph->AddNode(RemoveDelegateNode, false, false);
		RemoveDelegateNode->CreateNewGuid();
		RemoveDelegateNode->PostPlacedNewNode();
		RemoveDelegateNode->AllocateDefaultPins();
		RemoveDelegateNode->NodePosX = CurrentPosX + 800.0f;
		RemoveDelegateNode->NodePosY = CurrentPosY + 400.0f;

		// Wire CreateDelegate_B.OutputDelegate → RemoveDelegate.Delegate
		UEdGraphPin* DelegateBOut = CreateDelegateB->FindPin(TEXT("OutputDelegate"), EGPD_Output);
		UEdGraphPin* RemoveDelegateIn = RemoveDelegateNode->FindPin(TEXT("Delegate"), EGPD_Input);
		if (DelegateBOut && RemoveDelegateIn)
		{
			DelegateBOut->MakeLinkTo(RemoveDelegateIn);
		}

		// Wire SourceASCPinForRemove → RemoveDelegate.PN_Self (Target)
		UEdGraphPin* RemoveSelfPin = RemoveDelegateNode->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input);

		// v4.22: Debug logging for RemoveDelegate pin connection
		LogGeneration(FString::Printf(TEXT("    [DEBUG] RemoveDelegate pin check: SourceASCPinForRemove=%s, RemoveSelfPin=%s"),
			SourceASCPinForRemove ? TEXT("valid") : TEXT("null"),
			RemoveSelfPin ? TEXT("valid") : TEXT("null")));
		if (!RemoveSelfPin)
		{
			// Log available pins
			FString AvailablePins;
			for (UEdGraphPin* Pin : RemoveDelegateNode->Pins)
			{
				if (Pin && Pin->Direction == EGPD_Input)
				{
					if (!AvailablePins.IsEmpty()) AvailablePins += TEXT(", ");
					AvailablePins += Pin->PinName.ToString();
				}
			}
			LogGeneration(FString::Printf(TEXT("    [DEBUG] RemoveDelegate input pins: %s"), *AvailablePins));
		}

		if (SourceASCPinForRemove && RemoveSelfPin)
		{
			SourceASCPinForRemove->MakeLinkTo(RemoveSelfPin);
			// Verify connection was actually made
			bool bConnectionExists = RemoveSelfPin->LinkedTo.Contains(SourceASCPinForRemove);
			LogGeneration(FString::Printf(TEXT("    [DEBUG] RemoveDelegate Target connection made: verified=%s, LinkedTo.Num=%d"),
				bConnectionExists ? TEXT("YES") : TEXT("NO"), RemoveSelfPin->LinkedTo.Num()));
		}
		else
		{
			LogGeneration(TEXT("    [ERROR] RemoveDelegate Target connection FAILED - pin(s) null"));
		}

		// v4.21.2: Store CastNode and RemoveDelegate together for exec wiring
		RemoveDelegateNodes.Add(RemoveDelegateNode);
		CastNodesForRemove.Add(CastNodeB);  // May be nullptr if no cast needed

		LogGeneration(FString::Printf(TEXT("    Created delegate bind/unbind nodes for %s.%s -> %s"),
			*Binding.Source, *Binding.Delegate, *Binding.Handler));

		CurrentPosY += 600.0f;
		NodesGenerated++;
	}

	// 10. Wire AddDelegate nodes into ActivateAbility exec flow
	if (AddDelegateNodes.Num() > 0)
	{
		UK2Node_Event* ActivateEvent = nullptr;
		for (UEdGraphNode* Node : EventGraph->Nodes)
		{
			if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
			{
				FString EventName = EventNode->GetFunctionName().ToString();
				if (EventName.Contains(TEXT("ActivateAbility")))
				{
					ActivateEvent = EventNode;
					break;
				}
			}
		}

		if (ActivateEvent)
		{
			// DIAG: Log root node confirmation
			LogGeneration(FString::Printf(TEXT("    [DIAG] ActivateEvent FOUND: Class=%s, Function=%s"),
				*ActivateEvent->GetClass()->GetName(),
				*ActivateEvent->GetFunctionName().ToString()));

			// Find the last node in the ActivateAbility chain
			UEdGraphPin* LastExecPin = ActivateEvent->FindPin(UEdGraphSchema_K2::PN_Then);

			// DIAG: Log initial exec pin state
			LogGeneration(FString::Printf(TEXT("    [DIAG] ActivateEvent Then pin: %s, LinkedTo=%d"),
				LastExecPin ? TEXT("valid") : TEXT("NULL"),
				LastExecPin ? LastExecPin->LinkedTo.Num() : -1));

			// Follow exec chain to find the end
			while (LastExecPin && LastExecPin->LinkedTo.Num() > 0)
			{
				UEdGraphNode* NextNode = LastExecPin->LinkedTo[0]->GetOwningNode();
				UEdGraphPin* NextExecOut = NextNode->FindPin(UEdGraphSchema_K2::PN_Then);
				if (NextExecOut && NextExecOut->LinkedTo.Num() > 0)
				{
					LastExecPin = NextExecOut;
				}
				else
				{
					LastExecPin = NextExecOut;
					break;
				}
			}

			// DIAG: Log chain discovery outcome
			LogGeneration(FString::Printf(TEXT("    [DIAG] Chain end LastExecPin: %s, Owner=%s"),
				LastExecPin ? TEXT("valid") : TEXT("NULL"),
				LastExecPin ? *LastExecPin->GetOwningNode()->GetClass()->GetName() : TEXT("N/A")));

			// Chain AddDelegate nodes (v4.21.2: with CastNode exec wiring)
			for (int32 i = 0; i < AddDelegateNodes.Num(); ++i)
			{
				UK2Node_AddDelegate* AddNode = AddDelegateNodes[i];
				UK2Node_DynamicCast* CastNode = (i < CastNodesForAdd.Num()) ? CastNodesForAdd[i] : nullptr;

				if (LastExecPin)
				{
					// v4.21.2: If CastNode exists, wire it into exec chain first
					if (CastNode)
					{
						UEdGraphPin* CastExecIn = CastNode->FindPin(UEdGraphSchema_K2::PN_Execute);
						UEdGraphPin* CastExecOut = CastNode->FindPin(UEdGraphSchema_K2::PN_Then);

						if (CastExecIn && CastExecOut)
						{
							LastExecPin->MakeLinkTo(CastExecIn);
							LastExecPin = CastExecOut;

							LogGeneration(FString::Printf(TEXT("    [DIAG] CastNodeA exec wired: PN_Execute.LinkedTo=%d, PN_Then.LinkedTo=%d"),
								CastExecIn->LinkedTo.Num(),
								CastExecOut->LinkedTo.Num()));
						}
						else
						{
							LogGeneration(TEXT("    [DIAG] CastNodeA exec pins NOT FOUND!"));
						}
					}

					UEdGraphPin* AddExecIn = AddNode->FindPin(UEdGraphSchema_K2::PN_Execute);
					if (AddExecIn)
					{
						LastExecPin->MakeLinkTo(AddExecIn);
						LastExecPin = AddNode->FindPin(UEdGraphSchema_K2::PN_Then);

						// DIAG: Verify exec wiring succeeded
						LogGeneration(FString::Printf(TEXT("    [DIAG] AddDelegate exec wired: PN_Execute.LinkedTo=%d, PN_Then.LinkedTo=%d"),
							AddExecIn->LinkedTo.Num(),
							LastExecPin ? LastExecPin->LinkedTo.Num() : -1));
					}
					else
					{
						LogGeneration(TEXT("    [DIAG] AddDelegate PN_Execute pin NOT FOUND!"));
					}
				}
				else
				{
					LogGeneration(TEXT("    [DIAG] AddDelegate SKIPPED - LastExecPin is NULL"));
				}
			}

			LogGeneration(FString::Printf(TEXT("    Wired %d AddDelegate nodes into ActivateAbility chain"), AddDelegateNodes.Num()));
		}
		else
		{
			LogGeneration(TEXT("    [W_DELEGATE_NO_ACTIVATE] ActivateAbility event not found - manual exec wiring required"));
		}
	}

	// 11. Wire RemoveDelegate nodes into OnEndAbility exec flow
	if (RemoveDelegateNodes.Num() > 0)
	{
		UK2Node_Event* EndAbilityEvent = nullptr;
		for (UEdGraphNode* Node : EventGraph->Nodes)
		{
			if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
			{
				FString EventName = EventNode->GetFunctionName().ToString();
				if (EventName.Contains(TEXT("OnEndAbility")) || EventName.Contains(TEXT("EndAbility")))
				{
					EndAbilityEvent = EventNode;
					break;
				}
			}
		}

		if (EndAbilityEvent)
		{
			// DIAG: Log root node confirmation
			LogGeneration(FString::Printf(TEXT("    [DIAG] EndAbilityEvent FOUND: Class=%s, Function=%s"),
				*EndAbilityEvent->GetClass()->GetName(),
				*EndAbilityEvent->GetFunctionName().ToString()));

			UEdGraphPin* LastExecPin = EndAbilityEvent->FindPin(UEdGraphSchema_K2::PN_Then);

			// DIAG: Log initial exec pin state
			LogGeneration(FString::Printf(TEXT("    [DIAG] EndAbilityEvent Then pin: %s, LinkedTo=%d"),
				LastExecPin ? TEXT("valid") : TEXT("NULL"),
				LastExecPin ? LastExecPin->LinkedTo.Num() : -1));

			// Chain RemoveDelegate nodes at the START of EndAbility (cleanup first)
			// v4.21.2: Include CastNode in exec chain for exec reachability
			for (int32 i = 0; i < RemoveDelegateNodes.Num(); ++i)
			{
				UK2Node_RemoveDelegate* RemoveNode = RemoveDelegateNodes[i];
				UK2Node_DynamicCast* CastNode = (i < CastNodesForRemove.Num()) ? CastNodesForRemove[i] : nullptr;

				if (LastExecPin)
				{
					// Save existing connection
					TArray<UEdGraphPin*> ExistingConnections = LastExecPin->LinkedTo;

					// Break existing connections
					LastExecPin->BreakAllPinLinks();

					// v4.21.2: If CastNode exists, wire it first
					if (CastNode)
					{
						UEdGraphPin* CastExecIn = CastNode->FindPin(UEdGraphSchema_K2::PN_Execute);
						UEdGraphPin* CastExecOut = CastNode->FindPin(UEdGraphSchema_K2::PN_Then);

						if (CastExecIn && CastExecOut)
						{
							LastExecPin->MakeLinkTo(CastExecIn);
							LastExecPin = CastExecOut;

							LogGeneration(FString::Printf(TEXT("    [DIAG] CastNodeB exec wired: PN_Execute.LinkedTo=%d, PN_Then.LinkedTo=%d"),
								CastExecIn->LinkedTo.Num(),
								CastExecOut->LinkedTo.Num()));
						}
						else
						{
							LogGeneration(TEXT("    [DIAG] CastNodeB exec pins NOT FOUND!"));
						}
					}

					// Wire to RemoveDelegate
					UEdGraphPin* RemoveExecIn = RemoveNode->FindPin(UEdGraphSchema_K2::PN_Execute);
					if (RemoveExecIn)
					{
						LastExecPin->MakeLinkTo(RemoveExecIn);
						LastExecPin = RemoveNode->FindPin(UEdGraphSchema_K2::PN_Then);

						// DIAG: Verify exec wiring succeeded
						LogGeneration(FString::Printf(TEXT("    [DIAG] RemoveDelegate exec wired: PN_Execute.LinkedTo=%d, PN_Then.LinkedTo=%d"),
							RemoveExecIn->LinkedTo.Num(),
							LastExecPin ? LastExecPin->LinkedTo.Num() : -1));
					}
					else
					{
						LogGeneration(TEXT("    [DIAG] RemoveDelegate PN_Execute pin NOT FOUND!"));
					}

					// Restore connections to RemoveDelegate's Then pin
					if (LastExecPin && ExistingConnections.Num() > 0)
					{
						for (UEdGraphPin* ExistingPin : ExistingConnections)
						{
							LastExecPin->MakeLinkTo(ExistingPin);
						}
					}
				}
				else
				{
					LogGeneration(TEXT("    [DIAG] RemoveDelegate SKIPPED - LastExecPin is NULL"));
				}
			}

			LogGeneration(FString::Printf(TEXT("    Wired %d RemoveDelegate nodes into OnEndAbility chain"), RemoveDelegateNodes.Num()));
		}
		else
		{
			LogGeneration(TEXT("    [W_DELEGATE_NO_END] OnEndAbility event not found - manual exec wiring required"));
		}
	}

	if (NodesGenerated > 0)
	{
		LogGeneration(FString::Printf(TEXT("  DELEGATE_BIND: Generated %d complete delegate binding chains"), NodesGenerated));
	}

	// v4.22: Verify Target pin connections before returning
	int32 AddConnected = 0, AddNotConnected = 0;
	for (UK2Node_AddDelegate* AddNode : AddDelegateNodes)
	{
		UEdGraphPin* TargetPin = AddNode->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input);
		if (TargetPin && TargetPin->LinkedTo.Num() > 0)
		{
			AddConnected++;
		}
		else
		{
			AddNotConnected++;
			LogGeneration(FString::Printf(TEXT("    [VERIFY_FAIL] AddDelegate '%s' Target pin NOT connected"), *AddNode->GetNodeTitle(ENodeTitleType::EditableTitle).ToString()));
		}
	}

	int32 RemoveConnected = 0, RemoveNotConnected = 0;
	for (UK2Node_RemoveDelegate* RemoveNode : RemoveDelegateNodes)
	{
		UEdGraphPin* TargetPin = RemoveNode->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input);
		if (TargetPin && TargetPin->LinkedTo.Num() > 0)
		{
			RemoveConnected++;
		}
		else
		{
			RemoveNotConnected++;
			LogGeneration(FString::Printf(TEXT("    [VERIFY_FAIL] RemoveDelegate '%s' Target pin NOT connected"), *RemoveNode->GetNodeTitle(ENodeTitleType::EditableTitle).ToString()));
		}
	}
	LogGeneration(FString::Printf(TEXT("    [VERIFY] AddDelegate: %d connected, %d not connected"), AddConnected, AddNotConnected));
	LogGeneration(FString::Printf(TEXT("    [VERIFY] RemoveDelegate: %d connected, %d not connected"), RemoveConnected, RemoveNotConnected));

	return NodesGenerated;
}

// ============================================================================
// v4.22: Section 11 - Attribute Change Delegate Binding (AbilityTask Pattern)
// Per Delegate_Binding_Extensions_Spec_v1_1.md
// ============================================================================

int32 FEventGraphGenerator::GenerateAttributeBindingNodes(
	UBlueprint* Blueprint,
	const TArray<FManifestAttributeBindingDefinition>& AttributeBindings)
{
	if (!Blueprint || AttributeBindings.Num() == 0)
	{
		return 0;
	}

	UEdGraph* EventGraph = nullptr;
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph && Graph->GetFName() == UEdGraphSchema_K2::GN_EventGraph)
		{
			EventGraph = Graph;
			break;
		}
	}

	if (!EventGraph)
	{
		LogGeneration(TEXT("  [E_ATTR_BIND_NO_GRAPH] No EventGraph found for attribute binding generation"));
		return 0;
	}

	int32 NodesGenerated = 0;
	TArray<UK2Node_LatentAbilityCall*> TaskNodes;

	for (const FManifestAttributeBindingDefinition& Binding : AttributeBindings)
	{
		LogGeneration(FString::Printf(TEXT("  ATTR_BIND[%s]: Processing attribute binding (attr=%s.%s, handler=%s)"),
			*Binding.Id, *Binding.AttributeSet, *Binding.Attribute, *Binding.Handler));

		// 1. Validate AttributeSet class exists
		// Use FindParentClass which handles both native and Blueprint classes
		UClass* AttributeSetClass = FindParentClass(Binding.AttributeSet);
		if (!AttributeSetClass)
		{
			// Try with U prefix for native C++ classes
			FString FullAttributeSetName = TEXT("U") + Binding.AttributeSet;
			AttributeSetClass = FindParentClass(FullAttributeSetName);
		}
		if (!AttributeSetClass)
		{
			// Try direct path for native classes (e.g., "/Script/GameplayAbilities.UNarrativeAttributeSet")
			FString NativeClassPath = FString::Printf(TEXT("/Script/NarrativeArsenal.U%s"), *Binding.AttributeSet);
			AttributeSetClass = FindObject<UClass>(nullptr, *NativeClassPath);
		}
		if (!AttributeSetClass)
		{
			LogGeneration(FString::Printf(TEXT("    [E_ATTRIBUTE_SET_NOT_FOUND] AttributeSet class '%s' not found"), *Binding.AttributeSet));
			continue;
		}

		// 2. Validate Attribute property exists on AttributeSet
		FProperty* AttributeProp = AttributeSetClass->FindPropertyByName(FName(*Binding.Attribute));
		if (!AttributeProp)
		{
			LogGeneration(FString::Printf(TEXT("    [E_ATTRIBUTE_NOT_FOUND] Attribute '%s' not found on AttributeSet '%s'"),
				*Binding.Attribute, *Binding.AttributeSet));
			continue;
		}

		// 3. Create UK2Node_LatentAbilityCall for UAbilityTask_WaitAttributeChange
		UK2Node_LatentAbilityCall* WaitAttrNode = NewObject<UK2Node_LatentAbilityCall>(EventGraph);
		EventGraph->AddNode(WaitAttrNode, false, false);

		// Find the WaitForAttributeChange factory function
		UFunction* WaitAttrFunction = UAbilityTask_WaitAttributeChange::StaticClass()->FindFunctionByName(FName("WaitForAttributeChange"));
		if (!WaitAttrFunction)
		{
			LogGeneration(TEXT("    [E_TASK_FACTORY_REQUIRED] Failed to find WaitForAttributeChange function on UAbilityTask_WaitAttributeChange"));
			continue;
		}

		// Set protected base class properties via reflection (UK2Node_BaseAsyncTask members)
		UClass* NodeClass = WaitAttrNode->GetClass();

		// ProxyFactoryFunctionName = "WaitForAttributeChange"
		if (FNameProperty* FactoryFuncNameProp = FindFProperty<FNameProperty>(NodeClass, TEXT("ProxyFactoryFunctionName")))
		{
			FactoryFuncNameProp->SetPropertyValue_InContainer(WaitAttrNode, WaitAttrFunction->GetFName());
		}

		// ProxyFactoryClass = UAbilityTask_WaitAttributeChange::StaticClass()
		if (FObjectProperty* FactoryClassProp = FindFProperty<FObjectProperty>(NodeClass, TEXT("ProxyFactoryClass")))
		{
			FactoryClassProp->SetObjectPropertyValue_InContainer(WaitAttrNode, UAbilityTask_WaitAttributeChange::StaticClass());
		}

		// ProxyClass = UAbilityTask_WaitAttributeChange::StaticClass()
		if (FObjectProperty* ProxyClassProp = FindFProperty<FObjectProperty>(NodeClass, TEXT("ProxyClass")))
		{
			ProxyClassProp->SetObjectPropertyValue_InContainer(WaitAttrNode, UAbilityTask_WaitAttributeChange::StaticClass());
		}

		WaitAttrNode->CreateNewGuid();
		WaitAttrNode->PostPlacedNewNode();
		WaitAttrNode->AllocateDefaultPins();

		// 4. Set Attribute pin - construct FGameplayAttribute from AttributeSet and Property
		// The pin expects a FGameplayAttribute struct literal, but in Blueprint graph
		// we need to use a MakeGameplayAttribute node or set the property path directly
		// For UK2Node_LatentAbilityCall, the Attribute pin is a struct pin
		UEdGraphPin* AttributePin = WaitAttrNode->FindPin(FName("Attribute"));
		if (AttributePin)
		{
			// FGameplayAttribute pins in Blueprint use a special format:
			// (AttributeOwner=<ClassPath>,Attribute=<PropertyName>)
			// Or the pin can be connected to a MakeGameplayAttribute node
			// For now, set the default value using the struct literal format
			FString AttributePath = FString::Printf(TEXT("(AttributeOwner=\"%s\",Attribute=\"%s\")"),
				*AttributeSetClass->GetPathName(), *Binding.Attribute);
			AttributePin->DefaultValue = AttributePath;
			LogGeneration(FString::Printf(TEXT("    Set Attribute pin: %s"), *AttributePath));
		}
		else
		{
			// v4.23 FAIL-FAST: Pin not found on node - generation error
			LogGeneration(TEXT("[E_ATTRIBUTE_PIN_NOT_FOUND] Could not find Attribute pin on WaitForAttributeChange node"));
			return -1; // Signal failure to caller
		}

		// 5. Set TriggerOnce parameter (default true per spec 11.1.3)
		UEdGraphPin* TriggerOncePin = WaitAttrNode->FindPin(FName("TriggerOnce"));
		if (TriggerOncePin)
		{
			TriggerOncePin->DefaultValue = Binding.bTriggerOnce ? TEXT("true") : TEXT("false");
		}

		// 6. Set optional tag filters
		if (!Binding.WithTag.IsEmpty())
		{
			UEdGraphPin* WithTagPin = WaitAttrNode->FindPin(FName("OptionalExternalOwnerTags"));
			if (!WithTagPin)
			{
				WithTagPin = WaitAttrNode->FindPin(FName("WithTag"));
			}
			if (WithTagPin)
			{
				WithTagPin->DefaultValue = Binding.WithTag;
			}
		}

		// 7. Create CustomEvent for the handler (ZERO PARAMETERS per spec 11.4.1)
		UK2Node_CustomEvent* HandlerEvent = NewObject<UK2Node_CustomEvent>(EventGraph);
		EventGraph->AddNode(HandlerEvent, false, false);
		HandlerEvent->CreateNewGuid();
		HandlerEvent->CustomFunctionName = FName(*Binding.Handler);
		HandlerEvent->PostPlacedNewNode();
		HandlerEvent->AllocateDefaultPins();

		LogGeneration(FString::Printf(TEXT("    Created CustomEvent: %s (zero parameters per spec 11.4.1)"), *Binding.Handler));

		// 8. Connect OnChange delegate pin to CustomEvent
		// The task's OnChange is a dynamic multicast delegate output pin
		UEdGraphPin* OnChangePin = WaitAttrNode->FindPin(FName("OnChange"));
		if (OnChangePin)
		{
			// The delegate pin connects directly to the custom event's delegate pin
			// In UK2Node_LatentAbilityCall, delegate pins are exec-style outputs
			UEdGraphPin* HandlerExecPin = HandlerEvent->FindPin(UEdGraphSchema_K2::PN_Then);
			if (HandlerExecPin)
			{
				// For latent ability call nodes, the delegate output pins trigger the connected event
				// We wire the OnChange pin to the event's then pin
				OnChangePin->MakeLinkTo(HandlerExecPin);
				LogGeneration(TEXT("    Connected OnChange delegate to handler CustomEvent"));
			}
		}
		else
		{
			// v4.23 FAIL-FAST: Pin not found on node - generation error
			LogGeneration(TEXT("[E_ONCHANGE_PIN_NOT_FOUND] Could not find OnChange pin on WaitForAttributeChange node"));
			return -1; // Signal failure to caller
		}

		TaskNodes.Add(WaitAttrNode);
		NodesGenerated++;
		LogGeneration(FString::Printf(TEXT("    Created WaitForAttributeChange AbilityTask node for %s.%s"),
			*Binding.AttributeSet, *Binding.Attribute));
	}

	// 9. Wire AbilityTask nodes into ActivateAbility exec flow
	if (TaskNodes.Num() > 0)
	{
		UK2Node_Event* ActivateEvent = nullptr;
		for (UEdGraphNode* Node : EventGraph->Nodes)
		{
			if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
			{
				FString EventName = EventNode->GetFunctionName().ToString();
				if (EventName.Contains(TEXT("ActivateAbility")))
				{
					ActivateEvent = EventNode;
					break;
				}
			}
		}

		if (ActivateEvent)
		{
			// Find the last node in the ActivateAbility chain
			UEdGraphPin* LastExecPin = ActivateEvent->FindPin(UEdGraphSchema_K2::PN_Then);

			// Follow exec chain to find the end
			while (LastExecPin && LastExecPin->LinkedTo.Num() > 0)
			{
				UEdGraphNode* NextNode = LastExecPin->LinkedTo[0]->GetOwningNode();
				UEdGraphPin* NextExecOut = NextNode->FindPin(UEdGraphSchema_K2::PN_Then);
				if (NextExecOut && NextExecOut->LinkedTo.Num() > 0)
				{
					LastExecPin = NextExecOut;
				}
				else
				{
					LastExecPin = NextExecOut;
					break;
				}
			}

			// Chain AbilityTask nodes
			for (UK2Node_LatentAbilityCall* TaskNode : TaskNodes)
			{
				if (LastExecPin)
				{
					UEdGraphPin* TaskExecIn = TaskNode->FindPin(UEdGraphSchema_K2::PN_Execute);
					if (TaskExecIn)
					{
						LastExecPin->MakeLinkTo(TaskExecIn);
						LastExecPin = TaskNode->FindPin(UEdGraphSchema_K2::PN_Then);
					}
				}
			}

			LogGeneration(FString::Printf(TEXT("    Wired %d AbilityTask nodes into ActivateAbility chain"), TaskNodes.Num()));
		}
		else
		{
			LogGeneration(TEXT("    [W_ATTR_NO_ACTIVATE] ActivateAbility event not found - manual exec wiring required"));
		}
	}

	// Per spec 11.6.2: No explicit unbind nodes required in EndAbility
	// AbilityTask manages its own lifecycle when created via factory

	if (NodesGenerated > 0)
	{
		LogGeneration(FString::Printf(TEXT("  ATTR_BIND: Generated %d attribute binding AbilityTasks (lifecycle managed by task)"), NodesGenerated));
	}

	return NodesGenerated;
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
// v4.0: Enhanced with interpolation modes, tangent control, and extrapolation
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

	// v4.0: Helper lambda to convert string to interpolation mode
	auto ParseInterpMode = [](const FString& ModeStr) -> ERichCurveInterpMode
	{
		if (ModeStr.Equals(TEXT("Constant"), ESearchCase::IgnoreCase))
			return RCIM_Constant;
		if (ModeStr.Equals(TEXT("Cubic"), ESearchCase::IgnoreCase))
			return RCIM_Cubic;
		if (ModeStr.Equals(TEXT("None"), ESearchCase::IgnoreCase))
			return RCIM_None;
		return RCIM_Linear;  // Default
	};

	// v4.0: Helper lambda to convert string to tangent mode
	auto ParseTangentMode = [](const FString& ModeStr) -> ERichCurveTangentMode
	{
		if (ModeStr.Equals(TEXT("User"), ESearchCase::IgnoreCase))
			return RCTM_User;
		if (ModeStr.Equals(TEXT("Break"), ESearchCase::IgnoreCase))
			return RCTM_Break;
		if (ModeStr.Equals(TEXT("None"), ESearchCase::IgnoreCase))
			return RCTM_None;
		return RCTM_Auto;  // Default
	};

	// v4.0: Helper lambda to convert string to extrapolation mode
	auto ParseExtrapolation = [](const FString& ModeStr) -> ERichCurveExtrapolation
	{
		if (ModeStr.Equals(TEXT("Linear"), ESearchCase::IgnoreCase))
			return RCCE_Linear;
		if (ModeStr.Equals(TEXT("Cycle"), ESearchCase::IgnoreCase))
			return RCCE_Cycle;
		if (ModeStr.Equals(TEXT("CycleWithOffset"), ESearchCase::IgnoreCase))
			return RCCE_CycleWithOffset;
		if (ModeStr.Equals(TEXT("Oscillate"), ESearchCase::IgnoreCase))
			return RCCE_Oscillate;
		if (ModeStr.Equals(TEXT("None"), ESearchCase::IgnoreCase))
			return RCCE_None;
		return RCCE_Constant;  // Default
	};

	// v4.0: Set extrapolation modes
	Curve->FloatCurve.PreInfinityExtrap = ParseExtrapolation(Definition.ExtrapolationBefore);
	Curve->FloatCurve.PostInfinityExtrap = ParseExtrapolation(Definition.ExtrapolationAfter);

	// v4.0: Add keys with interpolation/tangent settings
	for (const auto& KeyDef : Definition.Keys)
	{
		FKeyHandle KeyHandle = Curve->FloatCurve.AddKey(KeyDef.Time, KeyDef.Value);

		// Set interpolation mode
		Curve->FloatCurve.SetKeyInterpMode(KeyHandle, ParseInterpMode(KeyDef.InterpMode));

		// Set tangent mode
		Curve->FloatCurve.SetKeyTangentMode(KeyHandle, ParseTangentMode(KeyDef.TangentMode));

		// Set tangent values if using User or Break mode
		ERichCurveTangentMode TangentMode = ParseTangentMode(KeyDef.TangentMode);
		if (TangentMode == RCTM_User || TangentMode == RCTM_Break)
		{
			FRichCurveKey& Key = Curve->FloatCurve.GetKey(KeyHandle);
			Key.ArriveTangent = KeyDef.ArriveTangent;
			Key.LeaveTangent = KeyDef.LeaveTangent;
		}
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Curve);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Curve, *PackageFileName, SaveArgs);

	// v3.0: Store metadata after successful generation
	StoreDataAssetMetadata(Curve, TEXT("FC"), Definition.Name, InputHash);

	LogGeneration(FString::Printf(TEXT("Created Float Curve: %s with %d keys (Extrapolation: %s/%s)"),
		*Definition.Name, Definition.Keys.Num(),
		*Definition.ExtrapolationBefore, *Definition.ExtrapolationAfter));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("FloatCurve");
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
		// v4.23 FAIL-FAST: Manifest references skeleton that cannot be loaded
		// v4.23.1: Phase 3 spec-complete - added Subsystem
		LogGeneration(FString::Printf(TEXT("[E_MONTAGE_SKELETON_NOT_FOUND] %s | Subsystem: General | Could not load skeleton: %s"),
			*Definition.Name, *Definition.Skeleton));
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Animation Montage skeleton '%s' not found"), *Definition.Skeleton));
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
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("AnimationMontage");
	Result.DetermineCategory();
	return Result;
}

// v2.6.9: Animation Notify Generator (creates Blueprint with UAnimNotifyState parent)
FGenerationResult FAnimationNotifyGenerator::Generate(const FManifestAnimationNotifyDefinition& Definition,
	const FManifestData* ManifestData /*= nullptr*/)
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

	// v4.0: Generate variables
	for (const FManifestActorVariableDefinition& VarDef : Definition.Variables)
	{
		FEdGraphPinType PinType = GetPinTypeFromString(VarDef.Type);
		bool bSuccess = FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VarDef.Name), PinType);
		if (bSuccess)
		{
			int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, FName(*VarDef.Name));
			if (VarIndex != INDEX_NONE && !VarDef.DefaultValue.IsEmpty())
			{
				Blueprint->NewVariables[VarIndex].DefaultValue = VarDef.DefaultValue;
			}
			LogGeneration(FString::Printf(TEXT("  Added variable: %s (%s)"), *VarDef.Name, *VarDef.Type));
		}
	}

	// Compile after adding variables
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// v4.0: Determine if we have an event graph to generate
	FManifestEventGraphDefinition EventGraphDef;
	bool bHasEventGraph = false;

	// Check for inline event graph
	if (Definition.InlineEventGraph.Nodes.Num() > 0)
	{
		EventGraphDef = Definition.InlineEventGraph;
		bHasEventGraph = true;
	}
	// Check for referenced event graph
	else if (!Definition.EventGraph.IsEmpty() && ManifestData)
	{
		for (const auto& EG : ManifestData->EventGraphs)
		{
			if (EG.Name.Equals(Definition.EventGraph))
			{
				EventGraphDef = EG;
				bHasEventGraph = true;
				break;
			}
		}
	}

	if (bHasEventGraph)
	{
		// Generate event graph using existing infrastructure
		FEventGraphGenerator::ClearMissingDependencies();
		bool bEventGraphSuccess = FEventGraphGenerator::GenerateEventGraph(Blueprint, EventGraphDef, GetProjectRoot());

		// Check for missing dependencies FIRST
		if (FEventGraphGenerator::HasMissingDependencies())
		{
			const TArray<FMissingDependencyInfo>& MissingDeps = FEventGraphGenerator::GetMissingDependencies();
			const FMissingDependencyInfo& FirstMissing = MissingDeps[0];
			LogGeneration(FString::Printf(TEXT("  Deferring due to missing dependency: %s"), *FirstMissing.DependencyName));
			Result = FGenerationResult(Definition.Name, EGenerationStatus::Deferred,
				FString::Printf(TEXT("Missing dependency: %s"), *FirstMissing.DependencyName));
			Result.MissingDependency = FirstMissing.DependencyName;
			Result.MissingDependencyType = FirstMissing.DependencyType;
			Result.DetermineCategory();
			FEventGraphGenerator::ClearMissingDependencies();
			return Result;
		}

		// v4.16: Event graph failure propagation (P2 - Caller Propagation)
		if (!bEventGraphSuccess)
		{
			UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Event graph generation failed"), *Definition.Name);
			LogGeneration(TEXT("  FAILED: Event graph generation failed - see errors above"));

			Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				TEXT("Event graph generation failed"));
			Result.AssetPath = AssetPath;
			Result.GeneratorId = TEXT("AnimationNotify");
			Result.DetermineCategory();
			return Result;
		}

		LogGeneration(FString::Printf(TEXT("  Generated event graph with %d nodes"), EventGraphDef.Nodes.Num()));
	}
	else
	{
		// v4.0: Create default event stubs for AnimNotify types
		UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
		if (!EventGraph)
		{
			EventGraph = FBlueprintEditorUtils::CreateNewGraph(
				Blueprint, TEXT("EventGraph"), UEdGraph::StaticClass(),
				UEdGraphSchema_K2::StaticClass());
			FBlueprintEditorUtils::AddFunctionGraph(Blueprint, EventGraph, false, static_cast<UFunction*>(nullptr));
		}

		if (ParentClass->IsChildOf(UAnimNotifyState::StaticClass()))
		{
			// AnimNotifyState has NotifyBegin, NotifyTick, NotifyEnd
			LogGeneration(TEXT("  Created AnimNotifyState event stubs: NotifyBegin, NotifyTick, NotifyEnd"));
		}
		else
		{
			// AnimNotify has single Notify event
			LogGeneration(TEXT("  Created AnimNotify event stub: Notify"));
		}
	}

	// v4.16: Compile with validation (Contract 10 - Blueprint Compile Gate)
	FCompilerResultsLog CompileLog;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &CompileLog);

	// v4.20.11: Re-apply node positions after compilation (compilation may reset positions)
	FEventGraphGenerator::ReapplyNodePositions(Blueprint);

	// v4.16: Check compile result - abort save if errors (P3, P4)
	if (CompileLog.NumErrors > 0)
	{
		TArray<FString> ErrorMessages;
		for (const TSharedRef<FTokenizedMessage>& Msg : CompileLog.Messages)
		{
			if (Msg->GetSeverity() == EMessageSeverity::Error)
			{
				ErrorMessages.Add(Msg->ToText().ToString());
			}
		}

		UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Blueprint compilation failed with %d errors: %s"),
			*Definition.Name, CompileLog.NumErrors, *FString::Join(ErrorMessages, TEXT("; ")));

		LogGeneration(FString::Printf(TEXT("  COMPILE FAILED with %d errors:"), CompileLog.NumErrors));
		for (const FString& Err : ErrorMessages)
		{
			LogGeneration(FString::Printf(TEXT("    - %s"), *Err));
		}

		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Compile failed: %s"), *FString::Join(ErrorMessages, TEXT("; "))));
		Result.AssetPath = AssetPath;
		Result.GeneratorId = TEXT("AnimationNotify");
		Result.DetermineCategory();
		return Result;
	}

	FAssetRegistryModule::AssetCreated(Blueprint);
	Package->MarkPackageDirty();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Animation Notify: %s (Parent: %s, Variables: %d)"),
		*Definition.Name, *ParentClass->GetName(), Definition.Variables.Num()));

	// v3.0: Store metadata for regeneration tracking
	StoreBlueprintMetadata(Blueprint, TEXT("AnimationNotify"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New,
		FString::Printf(TEXT("Created at %s"), *AssetPath));
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("AnimationNotify");
	Result.DetermineCategory();
	return Result;
}

// v3.7: Helper - Convert duration string to ELineDuration enum
static ELineDuration ParseDialogueDuration(const FString& DurationStr)
{
	if (DurationStr.Equals(TEXT("WhenAudioEnds"), ESearchCase::IgnoreCase))
		return ELineDuration::LD_WhenAudioEnds;
	if (DurationStr.Equals(TEXT("WhenSequenceEnds"), ESearchCase::IgnoreCase))
		return ELineDuration::LD_WhenSequenceEnds;
	if (DurationStr.Equals(TEXT("AfterReadingTime"), ESearchCase::IgnoreCase))
		return ELineDuration::LD_AfterReadingTime;
	if (DurationStr.Equals(TEXT("AfterDuration"), ESearchCase::IgnoreCase))
		return ELineDuration::LD_AfterDuration;
	if (DurationStr.Equals(TEXT("Never"), ESearchCase::IgnoreCase))
		return ELineDuration::LD_Never;
	return ELineDuration::LD_Default;
}

// v3.7: Helper - Create event instance from manifest definition
static UNarrativeEvent* CreateDialogueEventFromDefinition(
	UObject* Outer,
	const FManifestDialogueEventDefinition& EventDef)
{
	if (EventDef.Type.IsEmpty())
	{
		return nullptr;
	}

	// v3.9.11: Try to find event class in Narrative Pro paths (fixed paths)
	TArray<FString> EventSearchPaths = {
		// Narrative Pro built-in events (primary location)
		FString::Printf(TEXT("/NarrativePro/Pro/Core/Tales/Events/%s.%s_C"), *EventDef.Type, *EventDef.Type),
		// Project-specific events
		FString::Printf(TEXT("/Game/Pro/Core/Tales/Events/%s.%s_C"), *EventDef.Type, *EventDef.Type),
		FString::Printf(TEXT("%s/Tales/Events/%s.%s_C"), *GetProjectRoot(), *EventDef.Type, *EventDef.Type),
		FString::Printf(TEXT("%s/Events/%s.%s_C"), *GetProjectRoot(), *EventDef.Type, *EventDef.Type),
		// Native C++ classes
		FString::Printf(TEXT("/Script/NarrativeArsenal.%s"), *EventDef.Type),
		FString::Printf(TEXT("/Script/NarrativePro.%s"), *EventDef.Type)
	};

	UClass* EventClass = nullptr;
	for (const FString& Path : EventSearchPaths)
	{
		EventClass = LoadClass<UNarrativeEvent>(nullptr, *Path);
		if (EventClass)
		{
			break;
		}
	}

	if (!EventClass)
	{
		// v4.23 FAIL-FAST: Manifest references event class that cannot be found
		UE_LOG(LogGasAbilityGenerator, Error, TEXT("[E_DIALOGUE_EVENT_CLASS_NOT_FOUND] Could not find dialogue event class: %s"), *EventDef.Type);
		return nullptr;
	}

	// Create instance
	UNarrativeEvent* Event = NewObject<UNarrativeEvent>(Outer, EventClass);

	// Set runtime via reflection
	if (FEnumProperty* RuntimeProp = CastField<FEnumProperty>(EventClass->FindPropertyByName(TEXT("EventRuntime"))))
	{
		EEventRuntime RuntimeValue = EEventRuntime::Start;
		if (EventDef.Runtime.Equals(TEXT("End"), ESearchCase::IgnoreCase))
			RuntimeValue = EEventRuntime::End;
		else if (EventDef.Runtime.Equals(TEXT("Both"), ESearchCase::IgnoreCase))
			RuntimeValue = EEventRuntime::Both;

		RuntimeProp->GetUnderlyingProperty()->SetIntPropertyValue(
			RuntimeProp->ContainerPtrToValuePtr<void>(Event),
			static_cast<int64>(RuntimeValue));
	}

	// Set additional properties via reflection
	for (const auto& Prop : EventDef.Properties)
	{
		FProperty* Property = EventClass->FindPropertyByName(FName(*Prop.Key));
		if (Property)
		{
			void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Event);
			Property->ImportText_Direct(*Prop.Value, ValuePtr, Event, 0);
		}
	}

	return Event;
}

// v3.7: Helper - Create condition instance from manifest definition
static UNarrativeCondition* CreateDialogueConditionFromDefinition(
	UObject* Outer,
	const FManifestDialogueConditionDefinition& CondDef)
{
	if (CondDef.Type.IsEmpty())
	{
		return nullptr;
	}

	// Try to find condition class in Narrative Pro paths
	TArray<FString> CondSearchPaths = {
		FString::Printf(TEXT("/Game/Pro/Core/Tales/Conditions/%s.%s_C"), *CondDef.Type, *CondDef.Type),
		FString::Printf(TEXT("/Script/NarrativeArsenal.%s"), *CondDef.Type),
		FString::Printf(TEXT("/Script/NarrativePro.%s"), *CondDef.Type)
	};

	UClass* CondClass = nullptr;
	for (const FString& Path : CondSearchPaths)
	{
		CondClass = LoadClass<UNarrativeCondition>(nullptr, *Path);
		if (CondClass)
		{
			break;
		}
	}

	if (!CondClass)
	{
		// v4.23 FAIL-FAST: Manifest references condition class that cannot be found
		UE_LOG(LogGasAbilityGenerator, Error, TEXT("[E_DIALOGUE_CONDITION_CLASS_NOT_FOUND] Could not find dialogue condition class: %s"), *CondDef.Type);
		return nullptr;
	}

	// Create instance
	UNarrativeCondition* Condition = NewObject<UNarrativeCondition>(Outer, CondClass);
	Condition->bNot = CondDef.bNot;

	// Set additional properties via reflection
	for (const auto& Prop : CondDef.Properties)
	{
		FProperty* Property = CondClass->FindPropertyByName(FName(*Prop.Key));
		if (Property)
		{
			void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Condition);
			Property->ImportText_Direct(*Prop.Value, ValuePtr, Condition, 0);
		}
	}

	return Condition;
}

// v3.7: Helper - Create dialogue tree from manifest definition
// v4.23: Changed to return bool with OutErrorMessage for fail-fast support
static bool CreateDialogueTree(
	UDialogueBlueprint* DialogueBP,
	const FManifestDialogueTreeDefinition& TreeDef,
	const FString& DialogueName,
	FString& OutErrorMessage)
{
	if (TreeDef.Nodes.Num() == 0)
	{
		return true;  // No tree to create - not an error
	}

	UDialogue* Dialogue = DialogueBP->DialogueTemplate;
	if (!Dialogue)
	{
		UE_LOG(LogGasAbilityGenerator, Warning, TEXT("DialogueBlueprint has no DialogueTemplate"));
		return true;  // Not a manifest error - just skip tree creation
	}

	// Maps for node lookup during connection phase
	TMap<FString, UDialogueNode_NPC*> NPCNodeMap;
	TMap<FString, UDialogueNode_Player*> PlayerNodeMap;

	// First pass: Create all nodes
	for (const auto& NodeDef : TreeDef.Nodes)
	{
		UDialogueNode* DialogueNode = nullptr;

		if (NodeDef.Type.Equals(TEXT("npc"), ESearchCase::IgnoreCase))
		{
			// Create NPC dialogue node
			UDialogueNode_NPC* NPCNode = NewObject<UDialogueNode_NPC>(Dialogue);
			NPCNode->SetFlags(RF_Transactional);
			NPCNode->OwningDialogue = Dialogue;
			NPCNode->SetSpeakerID(FName(*NodeDef.Speaker));

			Dialogue->NPCReplies.AddUnique(NPCNode);
			NPCNodeMap.Add(NodeDef.Id, NPCNode);
			DialogueNode = NPCNode;

			// Set as root if this is the root node
			if (NodeDef.Id == TreeDef.RootNodeId)
			{
				Dialogue->RootDialogue = NPCNode;
			}
		}
		else if (NodeDef.Type.Equals(TEXT("player"), ESearchCase::IgnoreCase))
		{
			// Create Player dialogue node
			UDialogueNode_Player* PlayerNode = NewObject<UDialogueNode_Player>(Dialogue);
			PlayerNode->SetFlags(RF_Transactional);
			PlayerNode->OwningDialogue = Dialogue;

			Dialogue->PlayerReplies.AddUnique(PlayerNode);
			PlayerNodeMap.Add(NodeDef.Id, PlayerNode);
			DialogueNode = PlayerNode;

			// Set player-specific properties via reflection
			if (!NodeDef.OptionText.IsEmpty())
			{
				FTextProperty* OptionTextProp = CastField<FTextProperty>(
					PlayerNode->GetClass()->FindPropertyByName(TEXT("OptionText")));
				if (OptionTextProp)
				{
					OptionTextProp->SetPropertyValue_InContainer(PlayerNode, FText::FromString(NodeDef.OptionText));
				}
			}

			// v4.1: Set HintText for player dialogue options
			if (!NodeDef.HintText.IsEmpty())
			{
				FTextProperty* HintTextProp = CastField<FTextProperty>(
					PlayerNode->GetClass()->FindPropertyByName(TEXT("HintText")));
				if (HintTextProp)
				{
					HintTextProp->SetPropertyValue_InContainer(PlayerNode, FText::FromString(NodeDef.HintText));
				}
			}

			FBoolProperty* AutoSelectProp = CastField<FBoolProperty>(
				PlayerNode->GetClass()->FindPropertyByName(TEXT("bAutoSelect")));
			if (AutoSelectProp && NodeDef.bAutoSelect)
			{
				AutoSelectProp->SetPropertyValue_InContainer(PlayerNode, true);
			}

			FBoolProperty* AutoSelectIfOnlyProp = CastField<FBoolProperty>(
				PlayerNode->GetClass()->FindPropertyByName(TEXT("bAutoSelectIfOnlyReply")));
			if (AutoSelectIfOnlyProp)
			{
				AutoSelectIfOnlyProp->SetPropertyValue_InContainer(PlayerNode, NodeDef.bAutoSelectIfOnly);
			}
		}

		if (!DialogueNode)
		{
			UE_LOG(LogGasAbilityGenerator, Warning, TEXT("Failed to create dialogue node: %s (type: %s)"), *NodeDef.Id, *NodeDef.Type);
			continue;
		}

		// Set common dialogue line properties
		DialogueNode->Line.Text = FText::FromString(NodeDef.Text);
		DialogueNode->Line.Duration = ParseDialogueDuration(NodeDef.Duration);
		DialogueNode->Line.DurationSecondsOverride = NodeDef.DurationSeconds;
		DialogueNode->bIsSkippable = NodeDef.bIsSkippable;

		if (!NodeDef.DirectedAt.IsEmpty())
		{
			DialogueNode->DirectedAtSpeakerID = FName(*NodeDef.DirectedAt);
		}

		// v4.2: Set custom event callback function name
		if (!NodeDef.OnPlayNodeFuncName.IsEmpty())
		{
			DialogueNode->OnPlayNodeFuncName = FName(*NodeDef.OnPlayNodeFuncName);
		}

		// Load audio if specified
		if (!NodeDef.Audio.IsEmpty())
		{
			USoundBase* Sound = LoadObject<USoundBase>(nullptr, *NodeDef.Audio);
			if (Sound)
			{
				DialogueNode->Line.DialogueSound = Sound;
			}
			else
			{
				// v4.23 FAIL-FAST: Manifest references audio that cannot be loaded
				UE_LOG(LogGasAbilityGenerator, Error, TEXT("[E_DIALOGUE_AUDIO_NOT_FOUND] %s | Could not load dialogue audio: %s"),
					*DialogueName, *NodeDef.Audio);
				OutErrorMessage = FString::Printf(TEXT("Dialogue audio '%s' not found"), *NodeDef.Audio);
				return false;
			}
		}

		// Load montage if specified
		if (!NodeDef.Montage.IsEmpty())
		{
			UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, *NodeDef.Montage);
			if (Montage)
			{
				DialogueNode->Line.DialogueMontage = Montage;
			}
		}

		// v4.1: Load facial animation if specified
		if (!NodeDef.FacialAnimation.IsEmpty())
		{
			UAnimMontage* FacialMontage = LoadObject<UAnimMontage>(nullptr, *NodeDef.FacialAnimation);
			if (FacialMontage)
			{
				// Set via reflection since FDialogueLine::FacialAnimation may not be exposed
				FObjectProperty* FacialProp = CastField<FObjectProperty>(
					FDialogueLine::StaticStruct()->FindPropertyByName(TEXT("FacialAnimation")));
				if (FacialProp)
				{
					FacialProp->SetObjectPropertyValue_InContainer(&DialogueNode->Line, FacialMontage);
				}
			}
		}

		// Add alternative lines
		for (const auto& AltLineDef : NodeDef.AlternativeLines)
		{
			FDialogueLine AltLine;
			AltLine.Text = FText::FromString(AltLineDef.Text);
			AltLine.Duration = ELineDuration::LD_Default;

			if (!AltLineDef.Audio.IsEmpty())
			{
				AltLine.DialogueSound = LoadObject<USoundBase>(nullptr, *AltLineDef.Audio);
			}
			if (!AltLineDef.Montage.IsEmpty())
			{
				AltLine.DialogueMontage = LoadObject<UAnimMontage>(nullptr, *AltLineDef.Montage);
			}

			DialogueNode->AlternativeLines.Add(AltLine);
		}

		// Add events
		for (const auto& EventDef : NodeDef.Events)
		{
			if (UNarrativeEvent* Event = CreateDialogueEventFromDefinition(DialogueNode, EventDef))
			{
				DialogueNode->Events.Add(Event);
			}
		}

		// v3.9.6/v3.9.11: Quest shortcuts - create events for start_quest using NE_BeginQuest
		if (!NodeDef.StartQuest.IsEmpty())
		{
			FManifestDialogueEventDefinition QuestEventDef;
			QuestEventDef.Type = TEXT("NE_BeginQuest");
			QuestEventDef.Runtime = TEXT("Start");
			QuestEventDef.Properties.Add(TEXT("Quest"), NodeDef.StartQuest);  // Property name: Quest (TSubclassOf)
			if (UNarrativeEvent* Event = CreateDialogueEventFromDefinition(DialogueNode, QuestEventDef))
			{
				DialogueNode->Events.Add(Event);
				UE_LOG(LogGasAbilityGenerator, Log, TEXT("    Added NE_BeginQuest event: %s"), *NodeDef.StartQuest);
			}
			else
			{
				UE_LOG(LogGasAbilityGenerator, Warning, TEXT("    [WARN] Could not create NE_BeginQuest for: %s"), *NodeDef.StartQuest);
			}
		}
		// v4.23 FAIL-FAST: complete_quest_branch cannot be auto-generated
		// Quest branches complete via task completion (BPT_FinishDialogue), not direct event
		if (!NodeDef.CompleteQuestBranch.IsEmpty())
		{
			UE_LOG(LogGasAbilityGenerator, Error, TEXT("[E_COMPLETE_QUEST_BRANCH_NOT_AUTOMATABLE] %s | complete_quest_branch '%s' specified but cannot be auto-generated"),
				*DialogueName, *NodeDef.CompleteQuestBranch);
			OutErrorMessage = FString::Printf(TEXT("complete_quest_branch '%s' specified but requires manual BPT_FinishDialogue setup"), *NodeDef.CompleteQuestBranch);
			return false;
		}
		// v4.23 FAIL-FAST: fail_quest cannot be auto-generated
		// No built-in NE_FailQuest in Narrative Pro - requires Blueprint call to Quest->FailQuest()
		if (!NodeDef.FailQuest.IsEmpty())
		{
			UE_LOG(LogGasAbilityGenerator, Error, TEXT("[E_FAIL_QUEST_NOT_AUTOMATABLE] %s | fail_quest '%s' specified but cannot be auto-generated"),
				*DialogueName, *NodeDef.FailQuest);
			OutErrorMessage = FString::Printf(TEXT("fail_quest '%s' specified but requires manual Blueprint call to Quest->FailQuest()"), *NodeDef.FailQuest);
			return false;
		}

		// Add conditions
		for (const auto& CondDef : NodeDef.Conditions)
		{
			if (UNarrativeCondition* Cond = CreateDialogueConditionFromDefinition(DialogueNode, CondDef))
			{
				DialogueNode->Conditions.Add(Cond);
			}
		}

		// Generate unique ID for the node
		DialogueNode->SetID(FName(*(DialogueBP->GetName() + TEXT("_") + NodeDef.Id)));

		UE_LOG(LogGasAbilityGenerator, Log, TEXT("  Created dialogue node: %s (%s)"), *NodeDef.Id, *NodeDef.Type);
	}

	// Second pass: Wire connections (NPC/Player replies)
	for (const auto& NodeDef : TreeDef.Nodes)
	{
		UDialogueNode* SourceNode = nullptr;

		if (UDialogueNode_NPC** NPCPtr = NPCNodeMap.Find(NodeDef.Id))
		{
			SourceNode = *NPCPtr;
		}
		else if (UDialogueNode_Player** PlayerPtr = PlayerNodeMap.Find(NodeDef.Id))
		{
			SourceNode = *PlayerPtr;
		}

		if (!SourceNode)
		{
			continue;
		}

		// Add NPC replies
		for (const FString& ReplyId : NodeDef.NPCReplies)
		{
			if (UDialogueNode_NPC** FoundNode = NPCNodeMap.Find(ReplyId))
			{
				SourceNode->NPCReplies.AddUnique(*FoundNode);
			}
			else
			{
				// v4.23 FAIL-FAST: Manifest references NPC reply node that doesn't exist
				UE_LOG(LogGasAbilityGenerator, Error, TEXT("[E_DIALOGUE_NPC_REPLY_NOT_FOUND] %s | Could not find NPC node for reply: %s"),
					*DialogueName, *ReplyId);
				OutErrorMessage = FString::Printf(TEXT("Dialogue NPC reply node '%s' not found"), *ReplyId);
				return false;
			}
		}

		// Add Player replies
		for (const FString& ReplyId : NodeDef.PlayerReplies)
		{
			if (UDialogueNode_Player** FoundNode = PlayerNodeMap.Find(ReplyId))
			{
				SourceNode->PlayerReplies.AddUnique(*FoundNode);
			}
			else
			{
				// v4.23 FAIL-FAST: Manifest references Player reply node that doesn't exist
				UE_LOG(LogGasAbilityGenerator, Error, TEXT("[E_DIALOGUE_PLAYER_REPLY_NOT_FOUND] %s | Could not find Player node for reply: %s"),
					*DialogueName, *ReplyId);
				OutErrorMessage = FString::Printf(TEXT("Dialogue Player reply node '%s' not found"), *ReplyId);
				return false;
			}
		}
	}

	UE_LOG(LogGasAbilityGenerator, Log, TEXT("  Created dialogue tree: %d NPC nodes, %d Player nodes"),
		NPCNodeMap.Num(), PlayerNodeMap.Num());

	return true;  // v4.23: Success
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

	// v3.7: Create UDialogueBlueprint directly (not regular UBlueprint) to enable dialogue tree support
	UDialogueBlueprint* DialogueBP = NewObject<UDialogueBlueprint>(Package, *Definition.Name, RF_Public | RF_Standalone | RF_Transactional);
	if (!DialogueBP)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Dialogue Blueprint"));
	}

	// Setup as proper blueprint with parent class
	DialogueBP->ParentClass = ParentClass;
	DialogueBP->BlueprintType = BPTYPE_Normal;
	DialogueBP->bIsNewlyCreated = true;

	// Create the skeleton generated class
	FKismetEditorUtilities::CompileBlueprint(DialogueBP);

	// Use DialogueBP as the Blueprint pointer for the rest of the function
	UBlueprint* Blueprint = DialogueBP;

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

		// v4.1: Set DialogueCameraShake if specified
		if (!Definition.CameraShake.IsEmpty())
		{
			UClass* CameraShakeClass = LoadClass<UCameraShakeBase>(nullptr, *Definition.CameraShake);
			if (CameraShakeClass)
			{
				FClassProperty* CameraShakeP = CastField<FClassProperty>(CDO->GetClass()->FindPropertyByName(TEXT("DialogueCameraShake")));
				if (CameraShakeP)
				{
					CameraShakeP->SetPropertyValue_InContainer(CDO, CameraShakeClass);
					LogGeneration(FString::Printf(TEXT("  Set DialogueCameraShake: %s"), *Definition.CameraShake));
				}
			}
			else
			{
				// v4.23 FAIL-FAST: Manifest references camera shake that cannot be loaded
				UE_LOG(LogGasAbilityGenerator, Error, TEXT("[E_DIALOGUE_CAMERASHAKE_NOT_FOUND] %s | Could not load camera shake class: %s"),
					*Definition.Name, *Definition.CameraShake);
				return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
					FString::Printf(TEXT("Dialogue camera shake '%s' not found"), *Definition.CameraShake));
			}
		}

		// v4.8.3: Set DialogueSoundAttenuation if specified
		if (!Definition.DialogueSoundAttenuation.IsEmpty())
		{
			FString AttenPath = Definition.DialogueSoundAttenuation;
			if (!AttenPath.Contains(TEXT(".")))
			{
				// Assume path format: /Game/Audio/Attenuation/SA_Dialogue -> /Game/Audio/Attenuation/SA_Dialogue.SA_Dialogue
				AttenPath = FString::Printf(TEXT("%s.%s"), *AttenPath, *FPaths::GetBaseFilename(AttenPath));
			}

			USoundAttenuation* SoundAtten = LoadObject<USoundAttenuation>(nullptr, *AttenPath);
			if (SoundAtten)
			{
				FObjectProperty* AttenP = CastField<FObjectProperty>(CDO->GetClass()->FindPropertyByName(TEXT("DialogueSoundAttenuation")));
				if (AttenP)
				{
					AttenP->SetObjectPropertyValue_InContainer(CDO, SoundAtten);
					LogGeneration(FString::Printf(TEXT("  Set DialogueSoundAttenuation: %s"), *Definition.DialogueSoundAttenuation));
				}
			}
			else
			{
				// v4.23 FAIL-FAST: Manifest references sound attenuation that cannot be loaded
				UE_LOG(LogGasAbilityGenerator, Error, TEXT("[E_DIALOGUE_SOUNDATTENUATION_NOT_FOUND] %s | Could not load sound attenuation: %s"),
					*Definition.Name, *Definition.DialogueSoundAttenuation);
				return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
					FString::Printf(TEXT("Dialogue sound attenuation '%s' not found"), *Definition.DialogueSoundAttenuation));
			}
		}

		// v4.8.3: Set PlayerAutoAdjustTransform if not default
		if (!Definition.PlayerAutoAdjustTransform.IsDefault())
		{
			FStructProperty* TransformP = CastField<FStructProperty>(CDO->GetClass()->FindPropertyByName(TEXT("PlayerAutoAdjustTransform")));
			if (TransformP && TransformP->Struct == TBaseStructure<FTransform>::Get())
			{
				FTransform AdjustTransform;
				AdjustTransform.SetLocation(FVector(
					Definition.PlayerAutoAdjustTransform.LocationX,
					Definition.PlayerAutoAdjustTransform.LocationY,
					Definition.PlayerAutoAdjustTransform.LocationZ
				));
				AdjustTransform.SetRotation(FQuat(FRotator(
					Definition.PlayerAutoAdjustTransform.RotationPitch,
					Definition.PlayerAutoAdjustTransform.RotationYaw,
					Definition.PlayerAutoAdjustTransform.RotationRoll
				)));
				AdjustTransform.SetScale3D(FVector(
					Definition.PlayerAutoAdjustTransform.ScaleX,
					Definition.PlayerAutoAdjustTransform.ScaleY,
					Definition.PlayerAutoAdjustTransform.ScaleZ
				));

				void* ValuePtr = TransformP->ContainerPtrToValuePtr<void>(CDO);
				TransformP->CopyCompleteValue(ValuePtr, &AdjustTransform);
				LogGeneration(FString::Printf(TEXT("  Set PlayerAutoAdjustTransform: (%.1f, %.1f, %.1f)"),
					Definition.PlayerAutoAdjustTransform.LocationX,
					Definition.PlayerAutoAdjustTransform.LocationY,
					Definition.PlayerAutoAdjustTransform.LocationZ));
			}
		}

		// v4.8.3: Set DefaultDialogueShot if specified (instanced object)
		if (!Definition.DefaultDialogueShot.IsEmpty())
		{
			// For class-based reference, load the class and create instance
			if (!Definition.DefaultDialogueShot.SequenceClass.IsEmpty())
			{
				UClass* SeqClass = LoadClass<UObject>(nullptr, *Definition.DefaultDialogueShot.SequenceClass);
				if (SeqClass)
				{
					// Create instanced object and assign to property
					FObjectProperty* ShotP = CastField<FObjectProperty>(CDO->GetClass()->FindPropertyByName(TEXT("DefaultDialogueShot")));
					if (ShotP)
					{
						UObject* DialogueShot = NewObject<UObject>(CDO, SeqClass, NAME_None, RF_Public | RF_Transactional);
						if (DialogueShot)
						{
							ShotP->SetObjectPropertyValue_InContainer(CDO, DialogueShot);
							LogGeneration(FString::Printf(TEXT("  Set DefaultDialogueShot: %s (instanced)"), *Definition.DefaultDialogueShot.SequenceClass));
						}
					}
				}
				else
				{
					// v4.23 FAIL-FAST: Manifest references sequence class that cannot be loaded
					UE_LOG(LogGasAbilityGenerator, Error, TEXT("[E_DIALOGUE_SEQUENCE_NOT_FOUND] %s | Could not load dialogue sequence class: %s"),
						*Definition.Name, *Definition.DefaultDialogueShot.SequenceClass);
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						FString::Printf(TEXT("Dialogue sequence class '%s' not found"), *Definition.DefaultDialogueShot.SequenceClass));
				}
			}
			else if (Definition.DefaultDialogueShot.SequenceAssets.Num() > 0)
			{
				// v4.23 FAIL-FAST: SequenceAssets cannot be auto-generated
				LogGeneration(FString::Printf(TEXT("[E_DIALOGUE_SEQUENCEASSETS_NOT_AUTOMATABLE] %s | DefaultDialogueShot.SequenceAssets (%d) specified but cannot be auto-generated"),
					*Definition.Name, Definition.DefaultDialogueShot.SequenceAssets.Num()));
				return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
					TEXT("DefaultDialogueShot SequenceAssets specified but requires manual editor configuration"));
			}
		}

		// v4.14: Removed recompile - would wipe CDO property changes
		// CDO properties are persisted when package is saved without recompile
	}

	// v4.0: Populate Speakers array on DialogueTemplate
	if (Definition.Speakers.Num() > 0)
	{
		UDialogue* DialogueTemplate = DialogueBP->DialogueTemplate;
		if (DialogueTemplate)
		{
			// Clear existing speakers
			DialogueTemplate->Speakers.Empty();

			for (const auto& SpeakerDef : Definition.Speakers)
			{
				FSpeakerInfo NewSpeaker;

				// Load NPC Definition if specified
				if (!SpeakerDef.NPCDefinition.IsEmpty())
				{
					// Try multiple search paths for NPC definition
					TArray<FString> SearchPaths;
					SearchPaths.Add(FString::Printf(TEXT("%s/NPCs/Definitions/%s.%s"), *GetProjectRoot(), *SpeakerDef.NPCDefinition, *SpeakerDef.NPCDefinition));
					SearchPaths.Add(FString::Printf(TEXT("%s/NPCs/%s.%s"), *GetProjectRoot(), *SpeakerDef.NPCDefinition, *SpeakerDef.NPCDefinition));
					SearchPaths.Add(FString::Printf(TEXT("%s/Definitions/%s.%s"), *GetProjectRoot(), *SpeakerDef.NPCDefinition, *SpeakerDef.NPCDefinition));
					SearchPaths.Add(FString::Printf(TEXT("/Game/NPCs/Definitions/%s.%s"), *SpeakerDef.NPCDefinition, *SpeakerDef.NPCDefinition));

					UNPCDefinition* NPCDef = nullptr;
					for (const FString& SearchPath : SearchPaths)
					{
						NPCDef = LoadObject<UNPCDefinition>(nullptr, *SearchPath);
						if (NPCDef)
						{
							break;
						}
					}

					if (NPCDef)
					{
						NewSpeaker.NPCDataAsset = NPCDef;
						LogGeneration(FString::Printf(TEXT("  Added speaker with NPCDefinition: %s"), *SpeakerDef.NPCDefinition));
					}
					else
					{
						// v4.23 FAIL-FAST: Manifest references NPCDefinition that cannot be found
						LogGeneration(FString::Printf(TEXT("[E_SPEAKER_NPCDEFINITION_NOT_FOUND] %s | Could not find NPCDefinition: %s"),
							*Definition.Name, *SpeakerDef.NPCDefinition));
						return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
							FString::Printf(TEXT("Speaker NPCDefinition '%s' not found"), *SpeakerDef.NPCDefinition));
					}
				}

				// Set speaker ID (use NPCDefinition name if not overridden)
				if (!SpeakerDef.SpeakerID.IsEmpty())
				{
					NewSpeaker.SpeakerID = FName(*SpeakerDef.SpeakerID);
				}
				else if (!SpeakerDef.NPCDefinition.IsEmpty())
				{
					// Extract name from NPC_ prefix
					FString SpeakerName = SpeakerDef.NPCDefinition;
					if (SpeakerName.StartsWith(TEXT("NPC_")))
					{
						SpeakerName = SpeakerName.Mid(4);
					}
					NewSpeaker.SpeakerID = FName(*SpeakerName);
				}

				// Set node color
				if (!SpeakerDef.NodeColor.IsEmpty())
				{
					FColor ParsedColor = FColor::FromHex(SpeakerDef.NodeColor);
					NewSpeaker.NodeColor = FLinearColor(ParsedColor);
				}

				// Populate owned tags
				for (const FString& TagStr : SpeakerDef.OwnedTags)
				{
					FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagStr), false);
					if (Tag.IsValid())
					{
						NewSpeaker.OwnedTags.AddTag(Tag);
					}
				}

				NewSpeaker.bIsPlayer = SpeakerDef.bIsPlayer;

				DialogueTemplate->Speakers.Add(NewSpeaker);
			}

			// v4.0: Configure player speaker
			DialogueTemplate->PlayerSpeakerInfo.SpeakerID = FName(*Definition.PlayerSpeaker.SpeakerID);
			if (!Definition.PlayerSpeaker.NodeColor.IsEmpty())
			{
				FColor ParsedColor = FColor::FromHex(Definition.PlayerSpeaker.NodeColor);
				DialogueTemplate->PlayerSpeakerInfo.NodeColor = FLinearColor(ParsedColor);
			}

			// v4.8: Configure party speaker info (for multiplayer dialogues)
			if (Definition.PartySpeakerInfo.Num() > 0)
			{
				FArrayProperty* PartySpeakersArrayProp = CastField<FArrayProperty>(
					DialogueTemplate->GetClass()->FindPropertyByName(TEXT("PartySpeakerInfo")));
				if (PartySpeakersArrayProp)
				{
					FScriptArrayHelper ArrayHelper(PartySpeakersArrayProp, PartySpeakersArrayProp->ContainerPtrToValuePtr<void>(DialogueTemplate));
					ArrayHelper.EmptyValues();

					for (const auto& PartySpeakerDef : Definition.PartySpeakerInfo)
					{
						int32 NewIndex = ArrayHelper.AddValue();
						void* ElementPtr = ArrayHelper.GetRawPtr(NewIndex);

						// FPlayerSpeakerInfo has SpeakerID and NodeColor like FManifestPlayerSpeakerDefinition
						FStructProperty* StructProp = CastField<FStructProperty>(PartySpeakersArrayProp->Inner);
						if (StructProp)
						{
							// Set SpeakerID
							FNameProperty* SpeakerIDProp = CastField<FNameProperty>(StructProp->Struct->FindPropertyByName(TEXT("SpeakerID")));
							if (SpeakerIDProp)
							{
								SpeakerIDProp->SetPropertyValue_InContainer(ElementPtr, FName(*PartySpeakerDef.SpeakerID));
							}

							// Set NodeColor
							if (!PartySpeakerDef.NodeColor.IsEmpty())
							{
								FStructProperty* NodeColorProp = CastField<FStructProperty>(StructProp->Struct->FindPropertyByName(TEXT("NodeColor")));
								if (NodeColorProp)
								{
									FColor ParsedColor = FColor::FromHex(PartySpeakerDef.NodeColor);
									FLinearColor* ColorPtr = NodeColorProp->ContainerPtrToValuePtr<FLinearColor>(ElementPtr);
									if (ColorPtr)
									{
										*ColorPtr = FLinearColor(ParsedColor);
									}
								}
							}
						}
					}
					LogGeneration(FString::Printf(TEXT("  Configured %d party speakers"), Definition.PartySpeakerInfo.Num()));
				}
				else
				{
					// v4.23 FAIL-FAST: Property exists in NP but FindPropertyByName failed (generator bug)
					// v4.23.1: Phase 3 spec-complete - added Subsystem, SuperClassPath, RequestedProperty
					UClass* TargetClass = DialogueTemplate->GetClass();
					LogGeneration(FString::Printf(TEXT("[E_PARTYSPEAKERINFO_NOT_FOUND] %s | Subsystem: Dialogue | PartySpeakerInfo property not found | ClassPath: %s | SuperClassPath: %s | RequestedProperty: PartySpeakerInfo | Speakers requested: %d"),
						*Definition.Name, *TargetClass->GetPathName(),
						TargetClass->GetSuperClass() ? *TargetClass->GetSuperClass()->GetPathName() : TEXT("None"),
						Definition.PartySpeakerInfo.Num()));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						TEXT("PartySpeakerInfo property not found - manifest references speakers but property lookup failed"));
				}
			}

			LogGeneration(FString::Printf(TEXT("  Configured %d speakers + player speaker"), Definition.Speakers.Num()));
		}
		else
		{
			LogGeneration(TEXT("  WARNING: DialogueTemplate is null, speakers not configured"));
		}
	}
	else if (Definition.DialogueTree.Nodes.Num() > 0)
	{
		// v4.0: Auto-detect speakers from dialogue tree nodes
		TSet<FString> DiscoveredSpeakers;
		for (const auto& Node : Definition.DialogueTree.Nodes)
		{
			if (!Node.Speaker.IsEmpty() && Node.Type.Equals(TEXT("npc"), ESearchCase::IgnoreCase))
			{
				DiscoveredSpeakers.Add(Node.Speaker);
			}
		}

		if (DiscoveredSpeakers.Num() > 0)
		{
			LogGeneration(FString::Printf(TEXT("  Auto-detected %d speakers from dialogue tree: %s"),
				DiscoveredSpeakers.Num(), *FString::Join(DiscoveredSpeakers.Array(), TEXT(", "))));
			LogGeneration(TEXT("  NOTE: Add explicit 'speakers:' section for full speaker configuration"));
		}
	}

	// v3.7: Create dialogue tree if defined
	if (Definition.DialogueTree.Nodes.Num() > 0)
	{
		FString TreeErrorMessage;
		if (!CreateDialogueTree(DialogueBP, Definition.DialogueTree, Definition.Name, TreeErrorMessage))
		{
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TreeErrorMessage);
		}
		LogGeneration(FString::Printf(TEXT("  Created dialogue tree with %d nodes (root: %s)"),
			Definition.DialogueTree.Nodes.Num(), *Definition.DialogueTree.RootNodeId));

		// Mark as modified after adding dialogue nodes
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}

	// v4.16: Compile with validation (Contract 10 - Blueprint Compile Gate)
	FCompilerResultsLog CompileLog;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &CompileLog);

	// v4.20.11: Re-apply node positions after compilation (compilation may reset positions)
	FEventGraphGenerator::ReapplyNodePositions(Blueprint);

	// v4.16: Check compile result - abort save if errors (P3, P4)
	if (CompileLog.NumErrors > 0)
	{
		TArray<FString> ErrorMessages;
		for (const TSharedRef<FTokenizedMessage>& Msg : CompileLog.Messages)
		{
			if (Msg->GetSeverity() == EMessageSeverity::Error)
			{
				ErrorMessages.Add(Msg->ToText().ToString());
			}
		}

		UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Blueprint compilation failed with %d errors: %s"),
			*Definition.Name, CompileLog.NumErrors, *FString::Join(ErrorMessages, TEXT("; ")));

		LogGeneration(FString::Printf(TEXT("  COMPILE FAILED with %d errors:"), CompileLog.NumErrors));
		for (const FString& Err : ErrorMessages)
		{
			LogGeneration(FString::Printf(TEXT("    - %s"), *Err));
		}

		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Compile failed: %s"), *FString::Join(ErrorMessages, TEXT("; "))));
		Result.AssetPath = AssetPath;
		Result.GeneratorId = TEXT("DialogueBlueprint");
		Result.DetermineCategory();
		return Result;
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
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("DialogueBlueprint");
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
		else
		{
			// v4.23 FAIL-FAST: F9 - Manifest references parent class that doesn't exist
			LogGeneration(FString::Printf(TEXT("[E_ITEM_PARENT_NOT_FOUND] %s | Parent class not found: %s"), *Definition.Name, *Definition.ParentClass));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("EquippableItem parent class '%s' not found"), *Definition.ParentClass));
		}
	}
	// Default to UEquippableItem if no parent specified (Type D - allowed)
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
			// v3.10: Additional RangedWeaponItem properties
			// CrosshairWidget (TSubclassOf<UUserWidget>)
			if (!Definition.CrosshairWidget.IsEmpty())
			{
				UClass* CrosshairClass = LoadObject<UClass>(nullptr, *Definition.CrosshairWidget);
				if (!CrosshairClass)
				{
					// Try project paths
					TArray<FString> WidgetPaths = {
						FString::Printf(TEXT("%s/UI/%s.%s_C"), *GetProjectRoot(), *Definition.CrosshairWidget, *Definition.CrosshairWidget),
						FString::Printf(TEXT("%s/Widgets/%s.%s_C"), *GetProjectRoot(), *Definition.CrosshairWidget, *Definition.CrosshairWidget),
						FString::Printf(TEXT("/Game/UI/%s.%s_C"), *Definition.CrosshairWidget, *Definition.CrosshairWidget)
					};
					for (const FString& Path : WidgetPaths)
					{
						CrosshairClass = LoadObject<UClass>(nullptr, *Path);
						if (CrosshairClass) break;
					}
				}
				if (CrosshairClass)
				{
					FClassProperty* CrosshairProp = CastField<FClassProperty>(CDO->GetClass()->FindPropertyByName(TEXT("CrosshairWidget")));
					if (CrosshairProp)
					{
						CrosshairProp->SetObjectPropertyValue_InContainer(CDO, CrosshairClass);
						LogGeneration(FString::Printf(TEXT("  Set CrosshairWidget: %s"), *CrosshairClass->GetName()));
					}
				}
			}
			// AimWeaponRenderFOV
			if (Definition.AimWeaponRenderFOV != 0.0f)
			{
				FFloatProperty* FOVProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("AimWeaponRenderFOV")));
				if (FOVProp)
				{
					FOVProp->SetPropertyValue_InContainer(CDO, Definition.AimWeaponRenderFOV);
					LogGeneration(FString::Printf(TEXT("  Set AimWeaponRenderFOV: %.2f"), Definition.AimWeaponRenderFOV));
				}
			}
			// AimWeaponFStop
			if (Definition.AimWeaponFStop != 0.0f)
			{
				FFloatProperty* FStopProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("AimWeaponFStop")));
				if (FStopProp)
				{
					FStopProp->SetPropertyValue_InContainer(CDO, Definition.AimWeaponFStop);
					LogGeneration(FString::Printf(TEXT("  Set AimWeaponFStop: %.2f"), Definition.AimWeaponFStop));
				}
			}
			// MoveSpeedAddDegrees
			if (Definition.MoveSpeedAddDegrees != 0.0f)
			{
				FFloatProperty* MoveProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("MoveSpeedAddDegrees")));
				if (MoveProp)
				{
					MoveProp->SetPropertyValue_InContainer(CDO, Definition.MoveSpeedAddDegrees);
					LogGeneration(FString::Printf(TEXT("  Set MoveSpeedAddDegrees: %.2f"), Definition.MoveSpeedAddDegrees));
				}
			}
			// CrouchSpreadMultiplier (default 1.0, only set if different)
			if (Definition.CrouchSpreadMultiplier != 1.0f)
			{
				FFloatProperty* CrouchProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("CrouchSpreadMultiplier")));
				if (CrouchProp)
				{
					CrouchProp->SetPropertyValue_InContainer(CDO, Definition.CrouchSpreadMultiplier);
					LogGeneration(FString::Printf(TEXT("  Set CrouchSpreadMultiplier: %.2f"), Definition.CrouchSpreadMultiplier));
				}
			}
			// AimSpreadMultiplier (default 1.0, only set if different)
			if (Definition.AimSpreadMultiplier != 1.0f)
			{
				FFloatProperty* AimMultProp = CastField<FFloatProperty>(CDO->GetClass()->FindPropertyByName(TEXT("AimSpreadMultiplier")));
				if (AimMultProp)
				{
					AimMultProp->SetPropertyValue_InContainer(CDO, Definition.AimSpreadMultiplier);
					LogGeneration(FString::Printf(TEXT("  Set AimSpreadMultiplier: %.2f"), Definition.AimSpreadMultiplier));
				}
			}
			// RecoilImpulseTranslationMin (FVector)
			if (!Definition.RecoilImpulseTranslationMin.IsZero())
			{
				FStructProperty* RecoilMinProp = CastField<FStructProperty>(CDO->GetClass()->FindPropertyByName(TEXT("RecoilImpulseTranslationMin")));
				if (RecoilMinProp && RecoilMinProp->Struct == TBaseStructure<FVector>::Get())
				{
					FVector* ValuePtr = RecoilMinProp->ContainerPtrToValuePtr<FVector>(CDO);
					*ValuePtr = Definition.RecoilImpulseTranslationMin;
					LogGeneration(FString::Printf(TEXT("  Set RecoilImpulseTranslationMin: %s"), *Definition.RecoilImpulseTranslationMin.ToString()));
				}
			}
			// RecoilImpulseTranslationMax (FVector)
			if (!Definition.RecoilImpulseTranslationMax.IsZero())
			{
				FStructProperty* RecoilMaxProp = CastField<FStructProperty>(CDO->GetClass()->FindPropertyByName(TEXT("RecoilImpulseTranslationMax")));
				if (RecoilMaxProp && RecoilMaxProp->Struct == TBaseStructure<FVector>::Get())
				{
					FVector* ValuePtr = RecoilMaxProp->ContainerPtrToValuePtr<FVector>(CDO);
					*ValuePtr = Definition.RecoilImpulseTranslationMax;
					LogGeneration(FString::Printf(TEXT("  Set RecoilImpulseTranslationMax: %s"), *Definition.RecoilImpulseTranslationMax.ToString()));
				}
			}
			// HipRecoilImpulseTranslationMin (FVector)
			if (!Definition.HipRecoilImpulseTranslationMin.IsZero())
			{
				FStructProperty* HipMinProp = CastField<FStructProperty>(CDO->GetClass()->FindPropertyByName(TEXT("HipRecoilImpulseTranslationMin")));
				if (HipMinProp && HipMinProp->Struct == TBaseStructure<FVector>::Get())
				{
					FVector* ValuePtr = HipMinProp->ContainerPtrToValuePtr<FVector>(CDO);
					*ValuePtr = Definition.HipRecoilImpulseTranslationMin;
					LogGeneration(FString::Printf(TEXT("  Set HipRecoilImpulseTranslationMin: %s"), *Definition.HipRecoilImpulseTranslationMin.ToString()));
				}
			}
			// HipRecoilImpulseTranslationMax (FVector)
			if (!Definition.HipRecoilImpulseTranslationMax.IsZero())
			{
				FStructProperty* HipMaxProp = CastField<FStructProperty>(CDO->GetClass()->FindPropertyByName(TEXT("HipRecoilImpulseTranslationMax")));
				if (HipMaxProp && HipMaxProp->Struct == TBaseStructure<FVector>::Get())
				{
					FVector* ValuePtr = HipMaxProp->ContainerPtrToValuePtr<FVector>(CDO);
					*ValuePtr = Definition.HipRecoilImpulseTranslationMax;
					LogGeneration(FString::Printf(TEXT("  Set HipRecoilImpulseTranslationMax: %s"), *Definition.HipRecoilImpulseTranslationMax.ToString()));
				}
			}
			// v3.10: Weapon Attachment Slot Configuration (TMap<FGameplayTag, FWeaponAttachmentSlotConfig>)
			if (Definition.WeaponAttachmentSlots.Num() > 0)
			{
				FMapProperty* AttachMapProp = CastField<FMapProperty>(CDO->GetClass()->FindPropertyByName(TEXT("WeaponAttachmentConfiguration")));
				if (AttachMapProp)
				{
					void* MapPtr = AttachMapProp->ContainerPtrToValuePtr<void>(CDO);
					FScriptMapHelper MapHelper(AttachMapProp, MapPtr);
					MapHelper.EmptyValues();

					for (const FManifestWeaponAttachmentSlot& SlotDef : Definition.WeaponAttachmentSlots)
					{
						// Create the key (FGameplayTag)
						FGameplayTag SlotTag = FGameplayTag::RequestGameplayTag(FName(*SlotDef.Slot), false);
						if (!SlotTag.IsValid())
						{
							LogGeneration(FString::Printf(TEXT("  Invalid attachment slot tag: %s"), *SlotDef.Slot));
							continue;
						}

						// Add entry to map
						int32 NewIndex = MapHelper.AddDefaultValue_Invalid_NeedsRehash();

						// Set the key
						FStructProperty* KeyProp = CastField<FStructProperty>(AttachMapProp->KeyProp);
						if (KeyProp)
						{
							uint8* KeyPtr = MapHelper.GetKeyPtr(NewIndex);
							FGameplayTag* TagPtr = reinterpret_cast<FGameplayTag*>(KeyPtr);
							*TagPtr = SlotTag;
						}

						// Set the value (FWeaponAttachmentSlotConfig struct)
						uint8* ValuePtr = MapHelper.GetValuePtr(NewIndex);
						FStructProperty* ValueProp = CastField<FStructProperty>(AttachMapProp->ValueProp);

						if (ValueProp && ValuePtr)
						{
							UScriptStruct* SlotConfigStruct = ValueProp->Struct;

							// Set Socket (FName)
							if (FNameProperty* SocketProp = CastField<FNameProperty>(SlotConfigStruct->FindPropertyByName(TEXT("Socket"))))
							{
								SocketProp->SetPropertyValue_InContainer(ValuePtr, FName(*SlotDef.Socket));
							}

							// Set Offset (FVector)
							if (FStructProperty* OffsetProp = CastField<FStructProperty>(SlotConfigStruct->FindPropertyByName(TEXT("Offset"))))
							{
								if (OffsetProp->Struct == TBaseStructure<FVector>::Get())
								{
									FVector* OffsetPtr = OffsetProp->ContainerPtrToValuePtr<FVector>(ValuePtr);
									if (OffsetPtr)
									{
										*OffsetPtr = SlotDef.Offset;
									}
								}
							}

							// Set Rotation (FRotator)
							if (FStructProperty* RotProp = CastField<FStructProperty>(SlotConfigStruct->FindPropertyByName(TEXT("Rotation"))))
							{
								if (RotProp->Struct == TBaseStructure<FRotator>::Get())
								{
									FRotator* RotPtr = RotProp->ContainerPtrToValuePtr<FRotator>(ValuePtr);
									if (RotPtr)
									{
										*RotPtr = SlotDef.Rotation;
									}
								}
							}
						}

						LogGeneration(FString::Printf(TEXT("  Added attachment slot: %s -> Socket:%s Offset:%s Rot:%s"),
							*SlotDef.Slot, *SlotDef.Socket, *SlotDef.Offset.ToString(), *SlotDef.Rotation.ToString()));
					}

					MapHelper.Rehash();
					LogGeneration(FString::Printf(TEXT("  Set %d weapon attachment slots"), Definition.WeaponAttachmentSlots.Num()));
				}
				else
				{
					LogGeneration(TEXT("  WeaponAttachmentConfiguration property not found - may not be a WeaponItem"));
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

			// v3.9.6: NarrativeItem usage properties
			// bAddDefaultUseOption (default true, only set if false)
			if (!Definition.bAddDefaultUseOption)
			{
				FBoolProperty* UseOptProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bAddDefaultUseOption")));
				if (UseOptProp)
				{
					UseOptProp->SetPropertyValue_InContainer(CDO, false);
					LogGeneration(TEXT("  Set bAddDefaultUseOption: false"));
				}
			}
			// bConsumeOnUse
			if (Definition.bConsumeOnUse)
			{
				FBoolProperty* ConsumeProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bConsumeOnUse")));
				if (ConsumeProp)
				{
					ConsumeProp->SetPropertyValue_InContainer(CDO, true);
					LogGeneration(TEXT("  Set bConsumeOnUse: true"));
				}
			}
			// bUsedWithOtherItem
			if (Definition.bUsedWithOtherItem)
			{
				FBoolProperty* UsedWithProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bUsedWithOtherItem")));
				if (UsedWithProp)
				{
					UsedWithProp->SetPropertyValue_InContainer(CDO, true);
					LogGeneration(TEXT("  Set bUsedWithOtherItem: true"));
				}
			}
			// UseActionText
			if (!Definition.UseActionText.IsEmpty())
			{
				FTextProperty* UseTextProp = CastField<FTextProperty>(CDO->GetClass()->FindPropertyByName(TEXT("UseActionText")));
				if (UseTextProp)
				{
					UseTextProp->SetPropertyValue_InContainer(CDO, FText::FromString(Definition.UseActionText));
					LogGeneration(FString::Printf(TEXT("  Set UseActionText: %s"), *Definition.UseActionText));
				}
			}
			// bCanActivate
			if (Definition.bCanActivate)
			{
				FBoolProperty* ActivateProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bCanActivate")));
				if (ActivateProp)
				{
					ActivateProp->SetPropertyValue_InContainer(CDO, true);
					LogGeneration(TEXT("  Set bCanActivate: true"));
				}
			}
			// bToggleActiveOnUse
			if (Definition.bToggleActiveOnUse)
			{
				FBoolProperty* ToggleProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bToggleActiveOnUse")));
				if (ToggleProp)
				{
					ToggleProp->SetPropertyValue_InContainer(CDO, true);
					LogGeneration(TEXT("  Set bToggleActiveOnUse: true"));
				}
			}
			// UseSound (TSoftObjectPtr<USoundBase>)
			if (!Definition.UseSound.IsEmpty())
			{
				FSoftObjectProperty* SoundProp = CastField<FSoftObjectProperty>(CDO->GetClass()->FindPropertyByName(TEXT("UseSound")));
				if (SoundProp)
				{
					FSoftObjectPtr SoftPtr(FSoftObjectPath(Definition.UseSound));
					SoundProp->SetPropertyValue_InContainer(CDO, SoftPtr);
					LogGeneration(FString::Printf(TEXT("  Set UseSound: %s"), *Definition.UseSound));
				}
			}

			// v4.0: Weapon attachments TMap automation (HolsterAttachmentConfigs, WieldAttachmentConfigs)
			// TMap<FGameplayTag, FWeaponAttachmentConfig> where FWeaponAttachmentConfig = { FName SocketName, FTransform Offset }
			auto PopulateAttachmentTMap = [&CDO](
				const TArray<FString>& Slots,
				const TArray<FString>& Sockets,
				const TArray<FVector>& Offsets,
				const TArray<FRotator>& Rotations,
				const TCHAR* MapPropertyName) -> int32
			{
				if (Slots.Num() == 0) return 0;

				FMapProperty* MapProp = CastField<FMapProperty>(CDO->GetClass()->FindPropertyByName(MapPropertyName));
				if (!MapProp)
				{
					LogGeneration(FString::Printf(TEXT("  [WARNING] TMap property %s not found on %s"), MapPropertyName, *CDO->GetClass()->GetName()));
					return 0;
				}

				// Verify it's TMap<FGameplayTag, FWeaponAttachmentConfig>
				FStructProperty* KeyProp = CastField<FStructProperty>(MapProp->KeyProp);
				FStructProperty* ValueProp = CastField<FStructProperty>(MapProp->ValueProp);
				if (!KeyProp || !ValueProp)
				{
					LogGeneration(FString::Printf(TEXT("  [WARNING] %s key or value is not struct type"), MapPropertyName));
					return 0;
				}

				// Get the map helper
				void* MapPtr = MapProp->ContainerPtrToValuePtr<void>(CDO);
				FScriptMapHelper MapHelper(MapProp, MapPtr);

				int32 EntriesAdded = 0;
				for (int32 i = 0; i < Slots.Num(); i++)
				{
					const FString& SlotStr = Slots[i];
					FName SocketName = Sockets.IsValidIndex(i) ? FName(*Sockets[i]) : NAME_None;
					FVector Offset = Offsets.IsValidIndex(i) ? Offsets[i] : FVector::ZeroVector;
					FRotator Rotation = Rotations.IsValidIndex(i) ? Rotations[i] : FRotator::ZeroRotator;

					// Create FGameplayTag from slot string
					FGameplayTag SlotTag = FGameplayTag::RequestGameplayTag(FName(*SlotStr), false);
					if (!SlotTag.IsValid())
					{
						LogGeneration(FString::Printf(TEXT("    [WARNING] Invalid gameplay tag: %s"), *SlotStr));
						continue;
					}

					// Add new entry to the map
					int32 NewIndex = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
					uint8* PairPtr = MapHelper.GetPairPtr(NewIndex);

					// Set the key (FGameplayTag)
					FGameplayTag* KeyPtr = (FGameplayTag*)PairPtr;
					*KeyPtr = SlotTag;

					// Set the value (FWeaponAttachmentConfig)
					void* ValuePtr = PairPtr + MapProp->MapLayout.ValueOffset;

					// FWeaponAttachmentConfig has: SocketName (FName), Offset (FTransform)
					if (ValueProp->Struct)
					{
						// Set SocketName
						FNameProperty* SocketProp = CastField<FNameProperty>(ValueProp->Struct->FindPropertyByName(TEXT("SocketName")));
						if (SocketProp)
						{
							SocketProp->SetPropertyValue_InContainer(ValuePtr, SocketName);
						}

						// Set Offset (FTransform)
						FStructProperty* OffsetProp = CastField<FStructProperty>(ValueProp->Struct->FindPropertyByName(TEXT("Offset")));
						if (OffsetProp && OffsetProp->Struct == TBaseStructure<FTransform>::Get())
						{
							FTransform* TransformPtr = OffsetProp->ContainerPtrToValuePtr<FTransform>(ValuePtr);
							if (TransformPtr)
							{
								*TransformPtr = FTransform(Rotation, Offset);
							}
						}
					}

					EntriesAdded++;
					LogGeneration(FString::Printf(TEXT("    Added %s[%s] = Socket:%s, Offset:(%s), Rotation:(%s)"),
						MapPropertyName, *SlotStr, *SocketName.ToString(),
						*Offset.ToString(), *Rotation.ToString()));
				}

				// Rehash the map after all additions
				MapHelper.Rehash();
				return EntriesAdded;
			};

			// Populate HolsterAttachmentConfigs
			if (Definition.HolsterAttachmentSlots.Num() > 0)
			{
				int32 Count = PopulateAttachmentTMap(
					Definition.HolsterAttachmentSlots,
					Definition.HolsterAttachmentSockets,
					Definition.HolsterAttachmentOffsets,
					Definition.HolsterAttachmentRotations,
					TEXT("HolsterAttachmentConfigs"));
				if (Count > 0)
				{
					LogGeneration(FString::Printf(TEXT("  Set HolsterAttachmentConfigs: %d entries"), Count));
				}
			}

			// Populate WieldAttachmentConfigs
			if (Definition.WieldAttachmentSlots.Num() > 0)
			{
				int32 Count = PopulateAttachmentTMap(
					Definition.WieldAttachmentSlots,
					Definition.WieldAttachmentSockets,
					Definition.WieldAttachmentOffsets,
					Definition.WieldAttachmentRotations,
					TEXT("WieldAttachmentConfigs"));
				if (Count > 0)
				{
					LogGeneration(FString::Printf(TEXT("  Set WieldAttachmentConfigs: %d entries"), Count));
				}
			}

			// v3.9.8: ClothingMeshData population for EquippableItem_Clothing parent classes
			if (!Definition.ClothingMesh.IsEmpty())
			{
				// Check if parent class is clothing-related (by name or presence of ClothingMeshData property)
				FStructProperty* ClothingMeshProp = CastField<FStructProperty>(CDO->GetClass()->FindPropertyByName(TEXT("ClothingMeshData")));
				if (ClothingMeshProp)
				{
					void* ClothingMeshPtr = ClothingMeshProp->ContainerPtrToValuePtr<void>(CDO);
					UScriptStruct* MeshStruct = ClothingMeshProp->Struct;
					if (ClothingMeshPtr && MeshStruct)
					{
						LogGeneration(TEXT("  Populating ClothingMeshData..."));

						// Set Mesh (TSoftObjectPtr<USkeletalMesh>)
						if (!Definition.ClothingMesh.Mesh.IsEmpty())
						{
							FSoftObjectProperty* MeshProp = CastField<FSoftObjectProperty>(MeshStruct->FindPropertyByName(TEXT("Mesh")));
							if (MeshProp)
							{
								FSoftObjectPtr SoftPtr(FSoftObjectPath(Definition.ClothingMesh.Mesh));
								MeshProp->SetPropertyValue_InContainer(ClothingMeshPtr, SoftPtr);
								LogGeneration(FString::Printf(TEXT("    Set Mesh: %s"), *Definition.ClothingMesh.Mesh));
							}
						}

						// Set bUseLeaderPose (bool)
						FBoolProperty* LeaderPoseProp = CastField<FBoolProperty>(MeshStruct->FindPropertyByName(TEXT("bUseLeaderPose")));
						if (LeaderPoseProp)
						{
							LeaderPoseProp->SetPropertyValue_InContainer(ClothingMeshPtr, Definition.ClothingMesh.bUseLeaderPose);
							LogGeneration(FString::Printf(TEXT("    Set bUseLeaderPose: %s"), Definition.ClothingMesh.bUseLeaderPose ? TEXT("true") : TEXT("false")));
						}

						// Set bIsStaticMesh (bool)
						if (Definition.ClothingMesh.bIsStaticMesh)
						{
							FBoolProperty* StaticMeshBoolProp = CastField<FBoolProperty>(MeshStruct->FindPropertyByName(TEXT("bIsStaticMesh")));
							if (StaticMeshBoolProp)
							{
								StaticMeshBoolProp->SetPropertyValue_InContainer(ClothingMeshPtr, true);
								LogGeneration(TEXT("    Set bIsStaticMesh: true"));
							}
						}

						// Set StaticMesh (TSoftObjectPtr<UStaticMesh>)
						if (!Definition.ClothingMesh.StaticMesh.IsEmpty())
						{
							FSoftObjectProperty* StaticMeshProp = CastField<FSoftObjectProperty>(MeshStruct->FindPropertyByName(TEXT("StaticMesh")));
							if (StaticMeshProp)
							{
								FSoftObjectPtr SoftPtr(FSoftObjectPath(Definition.ClothingMesh.StaticMesh));
								StaticMeshProp->SetPropertyValue_InContainer(ClothingMeshPtr, SoftPtr);
								LogGeneration(FString::Printf(TEXT("    Set StaticMesh: %s"), *Definition.ClothingMesh.StaticMesh));
							}
						}

						// Set MeshAttachSocket (FName)
						if (!Definition.ClothingMesh.AttachSocket.IsEmpty())
						{
							FNameProperty* SocketProp = CastField<FNameProperty>(MeshStruct->FindPropertyByName(TEXT("MeshAttachSocket")));
							if (SocketProp)
							{
								SocketProp->SetPropertyValue_InContainer(ClothingMeshPtr, FName(*Definition.ClothingMesh.AttachSocket));
								LogGeneration(FString::Printf(TEXT("    Set MeshAttachSocket: %s"), *Definition.ClothingMesh.AttachSocket));
							}
						}

						// Set MeshAttachOffset (FTransform) - combining AttachLocation, AttachRotation, AttachScale
						FStructProperty* TransformProp = CastField<FStructProperty>(MeshStruct->FindPropertyByName(TEXT("MeshAttachOffset")));
						if (TransformProp)
						{
							FTransform* TransformPtr = TransformProp->ContainerPtrToValuePtr<FTransform>(ClothingMeshPtr);
							if (TransformPtr)
							{
								*TransformPtr = FTransform(
									Definition.ClothingMesh.AttachRotation.Quaternion(),
									Definition.ClothingMesh.AttachLocation,
									Definition.ClothingMesh.AttachScale
								);
								LogGeneration(FString::Printf(TEXT("    Set MeshAttachOffset: Loc(%s) Rot(%s) Scale(%s)"),
									*Definition.ClothingMesh.AttachLocation.ToString(),
									*Definition.ClothingMesh.AttachRotation.ToString(),
									*Definition.ClothingMesh.AttachScale.ToString()));
							}
						}

						// Set MeshAnimBP (TSoftClassPtr<UAnimInstance>)
						if (!Definition.ClothingMesh.MeshAnimBP.IsEmpty())
						{
							FSoftClassProperty* AnimBPProp = CastField<FSoftClassProperty>(MeshStruct->FindPropertyByName(TEXT("MeshAnimBP")));
							if (AnimBPProp)
							{
								FSoftObjectPath ClassPath(Definition.ClothingMesh.MeshAnimBP);
								void* PropValue = AnimBPProp->ContainerPtrToValuePtr<void>(ClothingMeshPtr);
								if (PropValue)
								{
									FSoftObjectPtr* SoftPtr = static_cast<FSoftObjectPtr*>(PropValue);
									if (SoftPtr)
									{
										*SoftPtr = FSoftObjectPtr(ClassPath);
										LogGeneration(FString::Printf(TEXT("    Set MeshAnimBP: %s"), *Definition.ClothingMesh.MeshAnimBP));
									}
								}
							}
						}

						// v4.23 FAIL-FAST: Complex structs not automated - fail if manifest specifies them
						if (Definition.ClothingMesh.Materials.Num() > 0)
						{
							LogGeneration(FString::Printf(TEXT("[E_MESHMATERIALS_NOT_AUTOMATED] %s | MeshMaterials (%d entries) specified but automation not implemented"),
								*Definition.Name, Definition.ClothingMesh.Materials.Num()));
							return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
								TEXT("MeshMaterials specified in manifest but complex struct automation not implemented"));
						}

						if (Definition.ClothingMesh.Morphs.Num() > 0)
						{
							LogGeneration(FString::Printf(TEXT("[E_MORPHS_NOT_AUTOMATED] %s | Morphs (%d entries) specified but automation not implemented"),
								*Definition.Name, Definition.ClothingMesh.Morphs.Num()));
							return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
								TEXT("Morphs specified in manifest but complex struct automation not implemented"));
						}
					}
				}
				else
				{
					// v4.23 FAIL-FAST: ClothingMesh specified but property not found
					LogGeneration(FString::Printf(TEXT("[E_CLOTHINGMESHDATA_NOT_FOUND] %s | ClothingMeshData property not found | CDO Class: %s"),
						*Definition.Name, *CDO->GetClass()->GetPathName()));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						TEXT("ClothingMeshData property not found - ensure parent_class is EquippableItem_Clothing"));
				}
			}


			// v3.9.12: Handle EquipmentEffectValues TMap<FGameplayTag, float>
			if (Definition.EquipmentEffectValues.Num() > 0)
			{
				FMapProperty* EffectValuesMapProp = CastField<FMapProperty>(
					CDO->GetClass()->FindPropertyByName(TEXT("EquipmentEffectValues")));
				if (EffectValuesMapProp)
				{
					void* MapPtr = EffectValuesMapProp->ContainerPtrToValuePtr<void>(CDO);
					FScriptMapHelper MapHelper(EffectValuesMapProp, MapPtr);
					MapHelper.EmptyValues();

					for (const auto& Pair : Definition.EquipmentEffectValues)
					{
						// Convert string key to FGameplayTag
						FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Pair.Key), false);
						if (Tag.IsValid())
						{
							int32 NewIndex = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
							FGameplayTag* KeyPtr = reinterpret_cast<FGameplayTag*>(MapHelper.GetKeyPtr(NewIndex));
							float* ValuePtr = reinterpret_cast<float*>(MapHelper.GetValuePtr(NewIndex));
							*KeyPtr = Tag;
							*ValuePtr = Pair.Value;
							LogGeneration(FString::Printf(TEXT("  Set EquipmentEffectValues[%s] = %.2f"), *Pair.Key, Pair.Value));
						}
						else
						{
							LogGeneration(FString::Printf(TEXT("  [WARNING] Invalid GameplayTag for EquipmentEffectValues key: %s"), *Pair.Key));
						}
					}
					MapHelper.Rehash();
				}
				else
				{
					// v4.23 FAIL-FAST: Property exists in NP but FindPropertyByName failed (generator bug)
					LogGeneration(FString::Printf(TEXT("[E_EQUIPMENTEFFECTVALUES_NOT_FOUND] %s | EquipmentEffectValues property not found | CDO Class: %s | Values requested: %d"),
						*Definition.Name, *CDO->GetClass()->GetPathName(), Definition.EquipmentEffectValues.Num()));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						TEXT("EquipmentEffectValues property not found - manifest references values but property lookup failed"));
				}
			}

			// v4.2: Handle EquipmentAbilities TArray<TSubclassOf<UNarrativeGameplayAbility>>
			if (Definition.EquipmentAbilities.Num() > 0)
			{
				FArrayProperty* AbilitiesArrayProp = CastField<FArrayProperty>(
					CDO->GetClass()->FindPropertyByName(TEXT("EquipmentAbilities")));
				if (AbilitiesArrayProp)
				{
					FScriptArrayHelper ArrayHelper(AbilitiesArrayProp, AbilitiesArrayProp->ContainerPtrToValuePtr<void>(CDO));
					ArrayHelper.EmptyValues();

					for (const FString& AbilityName : Definition.EquipmentAbilities)
					{
						// Resolve ability class
						UClass* AbilityClass = FindObject<UClass>(nullptr, *AbilityName);
						if (!AbilityClass)
						{
							AbilityClass = LoadClass<UGameplayAbility>(nullptr, *AbilityName);
						}
						if (!AbilityClass)
						{
							// Try with _C suffix for blueprint classes
							AbilityClass = LoadClass<UGameplayAbility>(nullptr, *FString::Printf(TEXT("%s_C"), *AbilityName));
						}
						if (!AbilityClass)
						{
							// Try as full path
							AbilityClass = LoadClass<UGameplayAbility>(nullptr, *FString::Printf(TEXT("/Game/FatherCompanion/Abilities/%s.%s_C"),
								*AbilityName, *AbilityName));
						}

						if (AbilityClass)
						{
							int32 NewIndex = ArrayHelper.AddValue();
							FClassProperty* InnerProp = CastField<FClassProperty>(AbilitiesArrayProp->Inner);
							if (InnerProp)
							{
								InnerProp->SetObjectPropertyValue(ArrayHelper.GetRawPtr(NewIndex), AbilityClass);
							}
							LogGeneration(FString::Printf(TEXT("  Added EquipmentAbility: %s"), *AbilityName));
						}
						else
						{
							// v4.23 FAIL-FAST: Manifest references ability class that cannot be resolved
							LogGeneration(FString::Printf(TEXT("[E_EQUIPMENTABILITY_NOT_FOUND] %s | Could not resolve EquipmentAbility class: %s"),
								*Definition.Name, *AbilityName));
							return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
								FString::Printf(TEXT("EquipmentAbility class '%s' not found - manifest references class but resolution failed"), *AbilityName));
						}
					}
				}
				else
				{
					// v4.23 FAIL-FAST: Property exists in NP but FindPropertyByName failed (generator bug)
					// v4.23.1: Phase 3 spec-complete - added Subsystem, SuperClassPath, RequestedProperty
					UClass* TargetClass = CDO->GetClass();
					LogGeneration(FString::Printf(TEXT("[E_EQUIPMENTABILITIES_NOT_FOUND] %s | Subsystem: Item | EquipmentAbilities property not found | ClassPath: %s | SuperClassPath: %s | RequestedProperty: EquipmentAbilities | Abilities requested: %d"),
						*Definition.Name, *TargetClass->GetPathName(),
						TargetClass->GetSuperClass() ? *TargetClass->GetSuperClass()->GetPathName() : TEXT("None"),
						Definition.EquipmentAbilities.Num()));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						TEXT("EquipmentAbilities property not found - manifest references abilities but property lookup failed"));
				}
			}

			// v4.8: Handle Stats TArray<FNarrativeItemStat>
			if (Definition.Stats.Num() > 0)
			{
				FArrayProperty* StatsArrayProp = CastField<FArrayProperty>(
					CDO->GetClass()->FindPropertyByName(TEXT("Stats")));
				if (StatsArrayProp)
				{
					FScriptArrayHelper ArrayHelper(StatsArrayProp, StatsArrayProp->ContainerPtrToValuePtr<void>(CDO));
					ArrayHelper.EmptyValues();

					FStructProperty* StructProp = CastField<FStructProperty>(StatsArrayProp->Inner);
					if (StructProp)
					{
						for (const FManifestItemStatDefinition& StatDef : Definition.Stats)
						{
							int32 NewIndex = ArrayHelper.AddValue();
							void* ElementPtr = ArrayHelper.GetRawPtr(NewIndex);

							// Set StatName (FText)
							FTextProperty* StatNameProp = CastField<FTextProperty>(StructProp->Struct->FindPropertyByName(TEXT("StatName")));
							if (StatNameProp)
							{
								StatNameProp->SetPropertyValue_InContainer(ElementPtr, FText::FromString(StatDef.StatName));
							}

							// Set StatValue (float)
							FFloatProperty* StatValueProp = CastField<FFloatProperty>(StructProp->Struct->FindPropertyByName(TEXT("StatValue")));
							if (StatValueProp)
							{
								StatValueProp->SetPropertyValue_InContainer(ElementPtr, StatDef.StatValue);
							}

							// Set StatIcon (TSoftObjectPtr<UTexture2D>)
							if (!StatDef.StatIcon.IsEmpty())
							{
								FSoftObjectProperty* StatIconProp = CastField<FSoftObjectProperty>(StructProp->Struct->FindPropertyByName(TEXT("StatIcon")));
								if (StatIconProp)
								{
									FSoftObjectPtr* SoftPtr = StatIconProp->GetPropertyValuePtr_InContainer(ElementPtr);
									if (SoftPtr)
									{
										*SoftPtr = FSoftObjectPath(StatDef.StatIcon);
									}
								}
							}

							LogGeneration(FString::Printf(TEXT("  Added Stat: %s = %.2f"), *StatDef.StatName, StatDef.StatValue));
						}
					}
				}
				else
				{
					// v4.23 FAIL-FAST: Property exists in NP but FindPropertyByName failed (generator bug)
					// v4.23.1: Phase 3 spec-complete - added Subsystem, SuperClassPath, RequestedProperty
					UClass* TargetClass = CDO->GetClass();
					LogGeneration(FString::Printf(TEXT("[E_STATS_NOT_FOUND] %s | Subsystem: Item | Stats property not found | ClassPath: %s | SuperClassPath: %s | RequestedProperty: Stats | Stats requested: %d"),
						*Definition.Name, *TargetClass->GetPathName(),
						TargetClass->GetSuperClass() ? *TargetClass->GetSuperClass()->GetPathName() : TEXT("None"),
						Definition.Stats.Num()));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						TEXT("Stats property not found - manifest references stats but property lookup failed"));
				}
			}

			// v4.8: Handle ActivitiesToGrant TArray<TSubclassOf<UNPCActivity>>
			if (Definition.ActivitiesToGrant.Num() > 0)
			{
				FArrayProperty* ActivitiesArrayProp = CastField<FArrayProperty>(
					CDO->GetClass()->FindPropertyByName(TEXT("ActivitiesToGrant")));
				if (ActivitiesArrayProp)
				{
					FScriptArrayHelper ArrayHelper(ActivitiesArrayProp, ActivitiesArrayProp->ContainerPtrToValuePtr<void>(CDO));
					ArrayHelper.EmptyValues();

					for (const FString& ActivityName : Definition.ActivitiesToGrant)
					{
						// Resolve activity class
						UClass* ActivityClass = FindObject<UClass>(nullptr, *ActivityName);
						if (!ActivityClass)
						{
							ActivityClass = LoadClass<UObject>(nullptr, *ActivityName);
						}
						if (!ActivityClass)
						{
							// Try with _C suffix for blueprint classes
							ActivityClass = LoadClass<UObject>(nullptr, *FString::Printf(TEXT("%s_C"), *ActivityName));
						}
						if (!ActivityClass)
						{
							// Try common paths
							TArray<FString> SearchPaths = {
								FString::Printf(TEXT("%s/AI/Activities/%s.%s_C"), *GetProjectRoot(), *ActivityName, *ActivityName),
								FString::Printf(TEXT("/Game/AI/Activities/%s.%s_C"), *ActivityName, *ActivityName)
							};
							for (const FString& SearchPath : SearchPaths)
							{
								ActivityClass = LoadClass<UObject>(nullptr, *SearchPath);
								if (ActivityClass) break;
							}
						}

						if (ActivityClass)
						{
							int32 NewIndex = ArrayHelper.AddValue();
							FClassProperty* InnerProp = CastField<FClassProperty>(ActivitiesArrayProp->Inner);
							if (InnerProp)
							{
								InnerProp->SetObjectPropertyValue(ArrayHelper.GetRawPtr(NewIndex), ActivityClass);
							}
							LogGeneration(FString::Printf(TEXT("  Added ActivitiesToGrant: %s"), *ActivityName));
						}
						else
						{
							// v4.23 FAIL-FAST: Manifest references activity class that cannot be resolved
							LogGeneration(FString::Printf(TEXT("[E_ACTIVITY_CLASS_NOT_FOUND] %s | Could not resolve Activity class: %s"),
								*Definition.Name, *ActivityName));
							return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
								FString::Printf(TEXT("Activity class '%s' not found"), *ActivityName));
						}
					}
				}
				else
				{
					// v4.23 FAIL-FAST: Property exists in NP but FindPropertyByName failed (generator bug)
					// v4.23.1: Phase 3 spec-complete - added Subsystem, SuperClassPath, RequestedProperty
					UClass* TargetClass = CDO->GetClass();
					LogGeneration(FString::Printf(TEXT("[E_ACTIVITIESTOGRANT_NOT_FOUND] %s | Subsystem: Item | ActivitiesToGrant property not found | ClassPath: %s | SuperClassPath: %s | RequestedProperty: ActivitiesToGrant | Activities requested: %d"),
						*Definition.Name, *TargetClass->GetPathName(),
						TargetClass->GetSuperClass() ? *TargetClass->GetSuperClass()->GetPathName() : TEXT("None"),
						Definition.ActivitiesToGrant.Num()));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						TEXT("ActivitiesToGrant property not found - manifest references activities but property lookup failed"));
				}
			}

			// v4.8.2: Handle ItemWidgetOverride TSubclassOf<UNarrativeInventoryItemButton>
			if (!Definition.ItemWidgetOverride.IsEmpty())
			{
				FClassProperty* WidgetOverrideProp = CastField<FClassProperty>(
					CDO->GetClass()->FindPropertyByName(TEXT("ItemWidgetOverride")));
				if (WidgetOverrideProp)
				{
					UClass* WidgetClass = LoadClass<UObject>(nullptr, *Definition.ItemWidgetOverride);
					if (!WidgetClass)
					{
						WidgetClass = LoadClass<UObject>(nullptr, *FString::Printf(TEXT("%s_C"), *Definition.ItemWidgetOverride));
					}
					if (WidgetClass)
					{
						WidgetOverrideProp->SetPropertyValue_InContainer(CDO, WidgetClass);
						LogGeneration(FString::Printf(TEXT("  Set ItemWidgetOverride: %s"), *Definition.ItemWidgetOverride));
					}
					else
					{
						// v4.23 FAIL-FAST: Manifest references widget class that cannot be loaded
						LogGeneration(FString::Printf(TEXT("[E_ITEMWIDGETOVERRIDE_NOT_FOUND] %s | Could not load ItemWidgetOverride class: %s"),
							*Definition.Name, *Definition.ItemWidgetOverride));
						return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
							FString::Printf(TEXT("ItemWidgetOverride class '%s' not found"), *Definition.ItemWidgetOverride));
					}
				}
			}

			// v4.8.2: Handle bWantsTickByDefault
			if (Definition.bWantsTickByDefault)
			{
				FBoolProperty* TickProp = CastField<FBoolProperty>(CDO->GetClass()->FindPropertyByName(TEXT("bWantsTickByDefault")));
				if (TickProp)
				{
					TickProp->SetPropertyValue_InContainer(CDO, true);
					LogGeneration(TEXT("  Set bWantsTickByDefault: true"));
				}
			}

			// v4.8.2: Handle PickupMeshData FPickupMeshData struct
			if (!Definition.PickupMeshData.IsEmpty())
			{
				FStructProperty* PickupMeshDataProp = CastField<FStructProperty>(
					CDO->GetClass()->FindPropertyByName(TEXT("PickupMeshData")));
				if (PickupMeshDataProp)
				{
					void* StructPtr = PickupMeshDataProp->ContainerPtrToValuePtr<void>(CDO);

					// Set PickupMesh (TSoftObjectPtr<UStaticMesh>)
					if (!Definition.PickupMeshData.PickupMesh.IsEmpty())
					{
						FSoftObjectProperty* MeshProp = CastField<FSoftObjectProperty>(
							PickupMeshDataProp->Struct->FindPropertyByName(TEXT("PickupMesh")));
						if (MeshProp)
						{
							FSoftObjectPtr* SoftPtr = MeshProp->GetPropertyValuePtr_InContainer(StructPtr);
							if (SoftPtr)
							{
								*SoftPtr = FSoftObjectPath(Definition.PickupMeshData.PickupMesh);
								LogGeneration(FString::Printf(TEXT("  Set PickupMeshData.PickupMesh: %s"), *Definition.PickupMeshData.PickupMesh));
							}
						}
					}

					// Set PickupMeshMaterials (TArray<TSoftObjectPtr<UMaterialInterface>>)
					if (Definition.PickupMeshData.PickupMeshMaterials.Num() > 0)
					{
						FArrayProperty* MaterialsArrayProp = CastField<FArrayProperty>(
							PickupMeshDataProp->Struct->FindPropertyByName(TEXT("PickupMeshMaterials")));
						if (MaterialsArrayProp)
						{
							FScriptArrayHelper ArrayHelper(MaterialsArrayProp, MaterialsArrayProp->ContainerPtrToValuePtr<void>(StructPtr));
							ArrayHelper.EmptyValues();

							for (const FString& MatPath : Definition.PickupMeshData.PickupMeshMaterials)
							{
								int32 NewIndex = ArrayHelper.AddValue();
								FSoftObjectProperty* InnerProp = CastField<FSoftObjectProperty>(MaterialsArrayProp->Inner);
								if (InnerProp)
								{
									FSoftObjectPtr* SoftPtr = reinterpret_cast<FSoftObjectPtr*>(ArrayHelper.GetRawPtr(NewIndex));
									if (SoftPtr)
									{
										*SoftPtr = FSoftObjectPath(MatPath);
									}
								}
							}
							LogGeneration(FString::Printf(TEXT("  Set PickupMeshData.PickupMeshMaterials: %d materials"), Definition.PickupMeshData.PickupMeshMaterials.Num()));
						}
					}
				}
				else
				{
					// v4.23 FAIL-FAST: Property exists in NP but FindPropertyByName failed (generator bug)
					// v4.23.1: Phase 3 spec-complete - added Subsystem, SuperClassPath, RequestedProperty
					UClass* TargetClass = CDO->GetClass();
					LogGeneration(FString::Printf(TEXT("[E_PICKUPMESHDATA_NOT_FOUND] %s | Subsystem: Item | PickupMeshData property not found | ClassPath: %s | SuperClassPath: %s | RequestedProperty: PickupMeshData"),
						*Definition.Name, *TargetClass->GetPathName(),
						TargetClass->GetSuperClass() ? *TargetClass->GetSuperClass()->GetPathName() : TEXT("None")));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						TEXT("PickupMeshData property not found - manifest references mesh data but property lookup failed"));
				}
			}

			// v4.8.2: Handle TraceData FCombatTraceData struct (for RangedWeaponItem)
			if (!Definition.TraceData.IsDefault())
			{
				FStructProperty* TraceDataProp = CastField<FStructProperty>(
					CDO->GetClass()->FindPropertyByName(TEXT("TraceData")));
				if (TraceDataProp)
				{
					void* StructPtr = TraceDataProp->ContainerPtrToValuePtr<void>(CDO);

					// Set TraceDistance
					FFloatProperty* DistanceProp = CastField<FFloatProperty>(
						TraceDataProp->Struct->FindPropertyByName(TEXT("TraceDistance")));
					if (DistanceProp)
					{
						DistanceProp->SetPropertyValue_InContainer(StructPtr, Definition.TraceData.TraceDistance);
					}

					// Set TraceRadius
					FFloatProperty* RadiusProp = CastField<FFloatProperty>(
						TraceDataProp->Struct->FindPropertyByName(TEXT("TraceRadius")));
					if (RadiusProp)
					{
						RadiusProp->SetPropertyValue_InContainer(StructPtr, Definition.TraceData.TraceRadius);
					}

					// Set bTraceMulti
					FBoolProperty* MultiProp = CastField<FBoolProperty>(
						TraceDataProp->Struct->FindPropertyByName(TEXT("bTraceMulti")));
					if (MultiProp)
					{
						MultiProp->SetPropertyValue_InContainer(StructPtr, Definition.TraceData.bTraceMulti);
					}

					LogGeneration(FString::Printf(TEXT("  Set TraceData: Distance=%.1f, Radius=%.1f, Multi=%s"),
						Definition.TraceData.TraceDistance, Definition.TraceData.TraceRadius,
						Definition.TraceData.bTraceMulti ? TEXT("true") : TEXT("false")));
				}
				else
				{
					// v4.23 FAIL-FAST: Property exists in NP but FindPropertyByName failed (generator bug)
					// v4.23.1: Phase 3 spec-complete - added Subsystem, SuperClassPath, RequestedProperty
					UClass* TargetClass = CDO->GetClass();
					LogGeneration(FString::Printf(TEXT("[E_TRACEDATA_NOT_FOUND] %s | Subsystem: Item | TraceData property not found | ClassPath: %s | SuperClassPath: %s | RequestedProperty: TraceData"),
						*Definition.Name, *TargetClass->GetPathName(),
						TargetClass->GetSuperClass() ? *TargetClass->GetSuperClass()->GetPathName() : TEXT("None")));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						TEXT("TraceData property not found - manifest references trace data but property lookup failed"));
				}
			}

			// v4.28: Item Fragments instantiation per Item_Generation_Capability_Audit.md
			if (Definition.Fragments.Num() > 0)
			{
				// Find the Fragments array property on CDO
				FArrayProperty* FragmentsArrayProp = CastField<FArrayProperty>(
					CDO->GetClass()->FindPropertyByName(TEXT("Fragments")));
				if (FragmentsArrayProp)
				{
					FScriptArrayHelper ArrayHelper(FragmentsArrayProp, FragmentsArrayProp->ContainerPtrToValuePtr<void>(CDO));

					// Static set for W_FRAGMENT_NO_SETTABLE_PROPERTIES deduplication
					static TSet<FString> WarnedNoSettablePropsClasses;

					for (const FManifestFragmentDefinition& FragDef : Definition.Fragments)
					{
						// Find fragment class
						UClass* FragmentClass = FindFragmentClass(FragDef.Class);
						if (!FragmentClass)
						{
							// E_FRAGMENT_CLASS_NOT_FOUND per audit
							LogGeneration(FString::Printf(TEXT("[E_FRAGMENT_CLASS_NOT_FOUND] %s | Fragment class not found: %s"),
								*Definition.Name, *FragDef.Class));
							return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
								FString::Printf(TEXT("Fragment class '%s' not found"), *FragDef.Class));
						}

						// E_FRAGMENT_CLASS_ABSTRACT per audit section 2.4
						if (FragmentClass->HasAnyClassFlags(CLASS_Abstract))
						{
							LogGeneration(FString::Printf(TEXT("[E_FRAGMENT_CLASS_ABSTRACT] %s | Fragment class is abstract: %s"),
								*Definition.Name, *FragDef.Class));
							return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
								FString::Printf(TEXT("Fragment class '%s' is abstract and cannot be instantiated"), *FragDef.Class));
						}

						// Create fragment instance per audit: NewObject<T>(Outer=CDO, Class, NAME_None, RF_Public | RF_Transactional)
						UObject* FragmentInstance = NewObject<UObject>(CDO, FragmentClass, NAME_None, RF_Public | RF_Transactional);
						if (!FragmentInstance)
						{
							LogGeneration(FString::Printf(TEXT("[E_FRAGMENT_INSTANTIATION_FAILED] %s | Failed to create fragment instance: %s"),
								*Definition.Name, *FragDef.Class));
							return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
								FString::Printf(TEXT("Failed to instantiate fragment '%s'"), *FragDef.Class));
						}

						// Set fragment properties
						int32 PropsSet = 0;
						for (const FManifestFragmentPropertyDefinition& PropDef : FragDef.Properties)
						{
							// S2 dot notation support per audit: one-hop struct member access
							FString PropertyName = PropDef.Key;
							FString StructMemberName;
							bool bIsDotNotation = PropertyName.Split(TEXT("."), &PropertyName, &StructMemberName);

							// Find the property on fragment
							FProperty* TargetProp = FragmentInstance->GetClass()->FindPropertyByName(*PropertyName);
							if (!TargetProp)
							{
								// E_ITEM_PROPERTY_NOT_APPLICABLE per audit
								LogGeneration(FString::Printf(TEXT("[E_ITEM_PROPERTY_NOT_APPLICABLE] %s | Fragment '%s' has no property '%s'"),
									*Definition.Name, *FragDef.Class, *PropertyName));
								return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
									FString::Printf(TEXT("Fragment '%s' property '%s' not found"), *FragDef.Class, *PropertyName));
							}

							void* PropertyContainer = FragmentInstance;
							FProperty* FinalProp = TargetProp;

							// Handle S2 dot notation for struct member access
							if (bIsDotNotation)
							{
								FStructProperty* StructProp = CastField<FStructProperty>(TargetProp);
								if (!StructProp)
								{
									LogGeneration(FString::Printf(TEXT("[E_ITEM_PROPERTY_NOT_APPLICABLE] %s | Fragment '%s' property '%s' is not a struct (dot notation requires struct)"),
										*Definition.Name, *FragDef.Class, *PropertyName));
									return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
										FString::Printf(TEXT("Fragment '%s' property '%s' is not a struct"), *FragDef.Class, *PropertyName));
								}

								// Get pointer to struct and find member
								PropertyContainer = StructProp->ContainerPtrToValuePtr<void>(FragmentInstance);
								FinalProp = StructProp->Struct->FindPropertyByName(*StructMemberName);
								if (!FinalProp)
								{
									LogGeneration(FString::Printf(TEXT("[E_ITEM_PROPERTY_NOT_APPLICABLE] %s | Fragment '%s' struct '%s' has no member '%s'"),
										*Definition.Name, *FragDef.Class, *PropertyName, *StructMemberName));
									return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
										FString::Printf(TEXT("Fragment '%s.%s' member '%s' not found"), *FragDef.Class, *PropertyName, *StructMemberName));
								}
							}

							// Set property value based on type
							bool bPropSet = false;
							if (FFloatProperty* FloatProp = CastField<FFloatProperty>(FinalProp))
							{
								float Value = FCString::Atof(*PropDef.Value);
								FloatProp->SetPropertyValue_InContainer(PropertyContainer, Value);
								bPropSet = true;
							}
							else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(FinalProp))
							{
								double Value = FCString::Atod(*PropDef.Value);
								DoubleProp->SetPropertyValue_InContainer(PropertyContainer, Value);
								bPropSet = true;
							}
							else if (FIntProperty* IntProp = CastField<FIntProperty>(FinalProp))
							{
								int32 Value = FCString::Atoi(*PropDef.Value);
								IntProp->SetPropertyValue_InContainer(PropertyContainer, Value);
								bPropSet = true;
							}
							else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(FinalProp))
							{
								bool Value = PropDef.Value.ToBool();
								BoolProp->SetPropertyValue_InContainer(PropertyContainer, Value);
								bPropSet = true;
							}
							else if (FStrProperty* StrProp = CastField<FStrProperty>(FinalProp))
							{
								StrProp->SetPropertyValue_InContainer(PropertyContainer, PropDef.Value);
								bPropSet = true;
							}
							else if (FNameProperty* NameProp = CastField<FNameProperty>(FinalProp))
							{
								NameProp->SetPropertyValue_InContainer(PropertyContainer, FName(*PropDef.Value));
								bPropSet = true;
							}
							else if (FTextProperty* TextProp = CastField<FTextProperty>(FinalProp))
							{
								TextProp->SetPropertyValue_InContainer(PropertyContainer, FText::FromString(PropDef.Value));
								bPropSet = true;
							}
							else if (FClassProperty* ClassProp = CastField<FClassProperty>(FinalProp))
							{
								UClass* ValueClass = FindParentClass(PropDef.Value);
								if (ValueClass)
								{
									ClassProp->SetObjectPropertyValue_InContainer(PropertyContainer, ValueClass);
									bPropSet = true;
								}
								else
								{
									LogGeneration(FString::Printf(TEXT("[E_ITEM_PROPERTY_NOT_APPLICABLE] %s | Fragment '%s' class property '%s' references class '%s' not found"),
										*Definition.Name, *FragDef.Class, *PropDef.Key, *PropDef.Value));
									return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
										FString::Printf(TEXT("Fragment class property '%s' references unknown class '%s'"), *PropDef.Key, *PropDef.Value));
								}
							}
							else if (FSoftClassProperty* SoftClassProp = CastField<FSoftClassProperty>(FinalProp))
							{
								FSoftObjectPath ClassPath(PropDef.Value);
								void* PropValue = SoftClassProp->ContainerPtrToValuePtr<void>(PropertyContainer);
								FSoftObjectPtr* SoftPtr = static_cast<FSoftObjectPtr*>(PropValue);
								if (SoftPtr)
								{
									*SoftPtr = FSoftObjectPtr(ClassPath);
									bPropSet = true;
								}
							}
							else if (FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(FinalProp))
							{
								FSoftObjectPtr SoftPtr(FSoftObjectPath(PropDef.Value));
								SoftObjProp->SetPropertyValue_InContainer(PropertyContainer, SoftPtr);
								bPropSet = true;
							}
							else if (FStructProperty* TagProp = CastField<FStructProperty>(FinalProp))
							{
								// Handle FGameplayTag
								if (TagProp->Struct && TagProp->Struct->GetName() == TEXT("GameplayTag"))
								{
									FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*PropDef.Value), false);
									if (Tag.IsValid())
									{
										FGameplayTag* TagPtr = TagProp->ContainerPtrToValuePtr<FGameplayTag>(PropertyContainer);
										if (TagPtr)
										{
											*TagPtr = Tag;
											bPropSet = true;
										}
									}
								}
							}

							if (bPropSet)
							{
								PropsSet++;
								LogGeneration(FString::Printf(TEXT("    Set fragment %s.%s = %s"), *FragDef.Class, *PropDef.Key, *PropDef.Value));
							}
							else
							{
								LogGeneration(FString::Printf(TEXT("[W_FRAGMENT_PROPERTY_TYPE_UNSUPPORTED] %s | Fragment '%s' property '%s' has unsupported type"),
									*Definition.Name, *FragDef.Class, *PropDef.Key));
							}
						}

						// W_FRAGMENT_NO_SETTABLE_PROPERTIES per audit - warn if no properties set but properties were provided
						if (FragDef.Properties.Num() > 0 && PropsSet == 0)
						{
							if (!WarnedNoSettablePropsClasses.Contains(FragDef.Class))
							{
								WarnedNoSettablePropsClasses.Add(FragDef.Class);
								LogGeneration(FString::Printf(TEXT("[W_FRAGMENT_NO_SETTABLE_PROPERTIES] %s | Fragment '%s' had %d property definitions but none could be set"),
									*Definition.Name, *FragDef.Class, FragDef.Properties.Num()));
							}
						}

						// Add fragment to Fragments array
						int32 NewIndex = ArrayHelper.AddValue();
						FObjectProperty* InnerProp = CastField<FObjectProperty>(FragmentsArrayProp->Inner);
						if (InnerProp)
						{
							InnerProp->SetObjectPropertyValue(ArrayHelper.GetRawPtr(NewIndex), FragmentInstance);
							LogGeneration(FString::Printf(TEXT("  Added fragment: %s (%d properties set)"), *FragDef.Class, PropsSet));
						}
					}
				}
				else
				{
					// Fragments property not found - fail only if manifest defines fragments
					UClass* TargetClass = CDO->GetClass();
					LogGeneration(FString::Printf(TEXT("[E_FRAGMENTS_NOT_FOUND] %s | Subsystem: Item | Fragments property not found | ClassPath: %s | SuperClassPath: %s | Fragments requested: %d"),
						*Definition.Name, *TargetClass->GetPathName(),
						TargetClass->GetSuperClass() ? *TargetClass->GetSuperClass()->GetPathName() : TEXT("None"),
						Definition.Fragments.Num()));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						TEXT("Fragments property not found - ensure parent_class derives from NarrativeItem"));
				}
			}

			// v4.28: GameplayEffectItem properties (per Item_Generation_Capability_Audit.md)
			if (!Definition.GameplayEffectClass.IsEmpty())
			{
				// GameplayEffectClass (TSubclassOf<UGameplayEffect>)
				FClassProperty* GEClassProp = CastField<FClassProperty>(
					CDO->GetClass()->FindPropertyByName(TEXT("GameplayEffectClass")));
				if (GEClassProp)
				{
					UClass* GEClass = FindParentClass(Definition.GameplayEffectClass);
					if (!GEClass)
					{
						// Try Blueprint path
						FString GEPath = FString::Printf(TEXT("%s/Effects/%s.%s_C"), *GetProjectRoot(), *Definition.GameplayEffectClass, *Definition.GameplayEffectClass);
						GEClass = LoadObject<UClass>(nullptr, *GEPath);
					}
					if (GEClass)
					{
						GEClassProp->SetObjectPropertyValue_InContainer(CDO, GEClass);
						LogGeneration(FString::Printf(TEXT("  Set GameplayEffectClass: %s"), *Definition.GameplayEffectClass));
					}
					else
					{
						LogGeneration(FString::Printf(TEXT("[E_GE_CLASS_NOT_FOUND] %s | GameplayEffectClass not found: %s"),
							*Definition.Name, *Definition.GameplayEffectClass));
						return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
							FString::Printf(TEXT("GameplayEffectClass '%s' not found"), *Definition.GameplayEffectClass));
					}
				}
			}

			// v4.28: SetByCallerValues TMap<FGameplayTag, float>
			if (Definition.SetByCallerValues.Num() > 0)
			{
				FMapProperty* SetByCallerMapProp = CastField<FMapProperty>(
					CDO->GetClass()->FindPropertyByName(TEXT("SetByCallerValues")));
				if (SetByCallerMapProp)
				{
					void* MapPtr = SetByCallerMapProp->ContainerPtrToValuePtr<void>(CDO);
					FScriptMapHelper MapHelper(SetByCallerMapProp, MapPtr);
					MapHelper.EmptyValues();

					for (const auto& Pair : Definition.SetByCallerValues)
					{
						FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Pair.Key), false);
						if (Tag.IsValid())
						{
							int32 NewIndex = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
							FGameplayTag* KeyPtr = reinterpret_cast<FGameplayTag*>(MapHelper.GetKeyPtr(NewIndex));
							float* ValuePtr = reinterpret_cast<float*>(MapHelper.GetValuePtr(NewIndex));
							*KeyPtr = Tag;
							*ValuePtr = Pair.Value;
							LogGeneration(FString::Printf(TEXT("  Set SetByCallerValues[%s] = %.2f"), *Pair.Key, Pair.Value));
						}
						else
						{
							LogGeneration(FString::Printf(TEXT("  [WARNING] Invalid GameplayTag for SetByCallerValues key: %s"), *Pair.Key));
						}
					}
					MapHelper.Rehash();
				}
			}

			// v4.28: WeaponAttachmentItem properties
			if (!Definition.WeaponAttachmentSlot.IsEmpty())
			{
				// WeaponAttachmentSlot (FGameplayTag)
				FStructProperty* SlotTagProp = CastField<FStructProperty>(
					CDO->GetClass()->FindPropertyByName(TEXT("WeaponAttachmentSlot")));
				if (SlotTagProp && SlotTagProp->Struct && SlotTagProp->Struct->GetName() == TEXT("GameplayTag"))
				{
					FGameplayTag SlotTag = FGameplayTag::RequestGameplayTag(FName(*Definition.WeaponAttachmentSlot), false);
					if (SlotTag.IsValid())
					{
						FGameplayTag* TagPtr = SlotTagProp->ContainerPtrToValuePtr<FGameplayTag>(CDO);
						if (TagPtr)
						{
							*TagPtr = SlotTag;
							LogGeneration(FString::Printf(TEXT("  Set WeaponAttachmentSlot: %s"), *Definition.WeaponAttachmentSlot));
						}
					}
				}
			}

			// AttachmentMesh (TSoftObjectPtr<UStaticMesh>)
			if (!Definition.AttachmentMesh.IsEmpty())
			{
				FSoftObjectProperty* MeshProp = CastField<FSoftObjectProperty>(
					CDO->GetClass()->FindPropertyByName(TEXT("AttachmentMesh")));
				if (MeshProp)
				{
					FSoftObjectPtr SoftPtr(FSoftObjectPath(Definition.AttachmentMesh));
					MeshProp->SetPropertyValue_InContainer(CDO, SoftPtr);
					LogGeneration(FString::Printf(TEXT("  Set AttachmentMesh: %s"), *Definition.AttachmentMesh));
				}
			}

			// FOVOverride (float) - sentinel: -1 means unset
			if (Definition.FOVOverride >= 0.0f)
			{
				FFloatProperty* FOVProp = CastField<FFloatProperty>(
					CDO->GetClass()->FindPropertyByName(TEXT("FOVOverride")));
				if (FOVProp)
				{
					FOVProp->SetPropertyValue_InContainer(CDO, Definition.FOVOverride);
					LogGeneration(FString::Printf(TEXT("  Set FOVOverride: %.2f"), Definition.FOVOverride));
				}
			}

			// WeaponRenderFOVOverride (float) - sentinel: -1 means unset
			if (Definition.WeaponRenderFOVOverride >= 0.0f)
			{
				FFloatProperty* WRFOVProp = CastField<FFloatProperty>(
					CDO->GetClass()->FindPropertyByName(TEXT("WeaponRenderFOVOverride")));
				if (WRFOVProp)
				{
					WRFOVProp->SetPropertyValue_InContainer(CDO, Definition.WeaponRenderFOVOverride);
					LogGeneration(FString::Printf(TEXT("  Set WeaponRenderFOVOverride: %.2f"), Definition.WeaponRenderFOVOverride));
				}
			}

			// WeaponAimFStopOverride (float) - sentinel: -1 means unset
			if (Definition.WeaponAimFStopOverride >= 0.0f)
			{
				FFloatProperty* FStopProp = CastField<FFloatProperty>(
					CDO->GetClass()->FindPropertyByName(TEXT("WeaponAimFStopOverride")));
				if (FStopProp)
				{
					FStopProp->SetPropertyValue_InContainer(CDO, Definition.WeaponAimFStopOverride);
					LogGeneration(FString::Printf(TEXT("  Set WeaponAimFStopOverride: %.2f"), Definition.WeaponAimFStopOverride));
				}
			}

			CDO->MarkPackageDirty();
		}
	}

	// v4.27: Generate function overrides if defined (e.g., HandleUnequip for form exit burst)
	if (Definition.FunctionOverrides.Num() > 0)
	{
		LogGeneration(FString::Printf(TEXT("  Generating %d function override(s)"), Definition.FunctionOverrides.Num()));

		for (const FManifestFunctionOverrideDefinition& OverrideDef : Definition.FunctionOverrides)
		{
			if (FEventGraphGenerator::GenerateFunctionOverride(Blueprint, OverrideDef, GetProjectRoot()))
			{
				LogGeneration(FString::Printf(TEXT("    Generated function override: %s"), *OverrideDef.FunctionName));
			}
			else
			{
				// v4.27: Fail-fast on function override generation failure
				LogGeneration(FString::Printf(TEXT("[E_ITEM_FUNCTIONOVERRIDE_FAILED] %s | Failed to generate function override: %s"), *Definition.Name, *OverrideDef.FunctionName));
				return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
					FString::Printf(TEXT("Failed to generate function override '%s'"), *OverrideDef.FunctionName));
			}
		}
	}

	// v4.16: Compile with validation (Contract 10 - Blueprint Compile Gate)
	// Note: CDO properties were set after initial compile, so final compile ensures GeneratedClass is current
	FCompilerResultsLog CompileLog;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &CompileLog);

	// v4.16: Check compile result - abort save if errors (P3, P4)
	if (CompileLog.NumErrors > 0)
	{
		TArray<FString> ErrorMessages;
		for (const TSharedRef<FTokenizedMessage>& Msg : CompileLog.Messages)
		{
			if (Msg->GetSeverity() == EMessageSeverity::Error)
			{
				ErrorMessages.Add(Msg->ToText().ToString());
			}
		}

		UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Blueprint compilation failed with %d errors: %s"),
			*Definition.Name, CompileLog.NumErrors, *FString::Join(ErrorMessages, TEXT("; ")));

		LogGeneration(FString::Printf(TEXT("  COMPILE FAILED with %d errors:"), CompileLog.NumErrors));
		for (const FString& Err : ErrorMessages)
		{
			LogGeneration(FString::Printf(TEXT("    - %s"), *Err));
		}

		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Compile failed: %s"), *FString::Join(ErrorMessages, TEXT("; "))));
		Result.AssetPath = AssetPath;
		Result.GeneratorId = TEXT("EquippableItem");
		Result.DetermineCategory();
		return Result;
	}

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
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("EquippableItem");
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

	// v4.22: Resolve parent class - check for custom Blueprint parent class first
	UClass* ParentClass = nullptr;
	if (!Definition.ParentClass.IsEmpty() && Definition.ParentClass != TEXT("NarrativeActivityBase") && Definition.ParentClass != TEXT("UNPCActivity"))
	{
		// Try to find custom Blueprint parent class (e.g., BPA_Attack_Melee)
		TArray<FString> ParentSearchPaths;

		// Project-local activities
		ParentSearchPaths.Add(FString::Printf(TEXT("%s/Activities/%s.%s_C"), *GetProjectRoot(), *Definition.ParentClass, *Definition.ParentClass));
		ParentSearchPaths.Add(FString::Printf(TEXT("%s/AI/Activities/%s.%s_C"), *GetProjectRoot(), *Definition.ParentClass, *Definition.ParentClass));
		ParentSearchPaths.Add(FString::Printf(TEXT("/Game/FatherCompanion/AI/Activities/%s.%s_C"), *Definition.ParentClass, *Definition.ParentClass));

		// Narrative Pro plugin built-in activities
		ParentSearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Attacks/MeleeAttack/%s.%s_C"), *Definition.ParentClass, *Definition.ParentClass));
		ParentSearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Attacks/ShootAndStrafe/%s.%s_C"), *Definition.ParentClass, *Definition.ParentClass));
		ParentSearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Attacks/ShootFromCover/%s.%s_C"), *Definition.ParentClass, *Definition.ParentClass));
		ParentSearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Attacks/Goals/%s.%s_C"), *Definition.ParentClass, *Definition.ParentClass));
		ParentSearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/FollowCharacter/%s.%s_C"), *Definition.ParentClass, *Definition.ParentClass));
		ParentSearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Idle/%s.%s_C"), *Definition.ParentClass, *Definition.ParentClass));
		ParentSearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Patrol/%s.%s_C"), *Definition.ParentClass, *Definition.ParentClass));
		ParentSearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/GoToLocation/%s.%s_C"), *Definition.ParentClass, *Definition.ParentClass));
		ParentSearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Flee/%s.%s_C"), *Definition.ParentClass, *Definition.ParentClass));
		ParentSearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Interact/Goals/%s.%s_C"), *Definition.ParentClass, *Definition.ParentClass));
		ParentSearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/ReturnToSpawn/%s.%s_C"), *Definition.ParentClass, *Definition.ParentClass));

		for (const FString& Path : ParentSearchPaths)
		{
			ParentClass = LoadClass<UNPCActivity>(nullptr, *Path);
			if (ParentClass)
			{
				LogGeneration(FString::Printf(TEXT("  Resolved parent class: %s -> %s"), *Definition.ParentClass, *Path));
				break;
			}
		}

		if (!ParentClass)
		{
			LogGeneration(FString::Printf(TEXT("[W_ACTIVITY_PARENT_NOT_FOUND] Parent class '%s' not found, using UNPCActivity"), *Definition.ParentClass));
		}
	}

	// Fallback to base UNPCActivity class
	if (!ParentClass)
	{
		ParentClass = UNPCActivity::StaticClass();
	}

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

	// v4.16: Compile with validation (Contract 10 - Blueprint Compile Gate)
	FCompilerResultsLog CompileLog;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &CompileLog);

	if (CompileLog.NumErrors > 0)
	{
		TArray<FString> ErrorMessages;
		for (const TSharedRef<FTokenizedMessage>& Msg : CompileLog.Messages)
		{
			if (Msg->GetSeverity() == EMessageSeverity::Error)
			{
				ErrorMessages.Add(Msg->ToText().ToString());
			}
		}

		UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Activity Blueprint compilation failed with %d errors: %s"),
			*Definition.Name, CompileLog.NumErrors, *FString::Join(ErrorMessages, TEXT("; ")));

		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Compile failed: %s"), *FString::Join(ErrorMessages, TEXT("; "))));
		Result.AssetPath = AssetPath;
		Result.GeneratorId = TEXT("Activity");
		Result.DetermineCategory();
		return Result;
	}

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

	// v4.14: Removed redundant recompile - would wipe CDO property changes
	// Initial compile at line 14218 is sufficient; CDO changes persist without recompile

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Blueprint);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Activity: %s"), *Definition.Name));

	// v4.22: Cache the Activity Blueprint class for same-session resolution
	if (Blueprint->GeneratedClass)
	{
		GSessionBlueprintClassCache.Add(Definition.Name, Blueprint->GeneratedClass);
		LogGeneration(FString::Printf(TEXT("  Cached Blueprint class for same-session resolution: %s"), *Definition.Name));
	}

	// v3.0: Store metadata for regeneration tracking
	StoreBlueprintMetadata(Blueprint, TEXT("Activity"), Definition.Name, Definition.ComputeHash());

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("Activity");
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

	// v4.13: Category C - P1.3 Startup Effects Validation
	// v4.16.2: Upgraded to FAIL severity - errors block asset creation (LOCKED CONTRACT P1.3)
	TArray<FString> ValidationErrors;
	ValidateStartupEffects(Definition, ValidationErrors);

	// Log all errors and add to result (stored in Warnings array with E_ prefix for FAIL severity)
	for (const FString& Error : ValidationErrors)
	{
		Result.Warnings.Add(Error);
		LogGeneration(FString::Printf(TEXT("  [ERROR] %s"), *Error));
	}

	// v4.16.2: ABORT STRATEGY - Skip asset creation if any FAIL errors (LOCKED CONTRACT P1.3)
	if (ValidationErrors.Num() > 0)
	{
		LogGeneration(FString::Printf(TEXT("  [FAIL] Ability Configuration '%s' skipped due to %d validation error(s)"), *Definition.Name, ValidationErrors.Num()));
		Result.AssetName = Definition.Name;
		Result.Status = EGenerationStatus::Failed;
		Result.Message = FString::Printf(TEXT("P1.3 validation failed with %d error(s)"), ValidationErrors.Num());
		Result.GeneratorId = TEXT("AbilityConfiguration");
		Result.DetermineCategory();
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
		UClass* AbilityClass = nullptr;

		// v4.22: Check session cache FIRST for abilities created in this generation session
		if (UClass** CachedClass = GSessionBlueprintClassCache.Find(AbilityName))
		{
			AbilityClass = *CachedClass;
			LogGeneration(FString::Printf(TEXT("  Found ability class in session cache: %s"), *AbilityName));
		}

		// If not in cache, try multiple paths for the ability blueprint
		if (!AbilityClass)
		{
			TArray<FString> SearchPaths;
			SearchPaths.Add(FString::Printf(TEXT("%s/Abilities/%s.%s_C"), *GetProjectRoot(), *AbilityName, *AbilityName));
			SearchPaths.Add(FString::Printf(TEXT("%s/Abilities/Crawler/%s.%s_C"), *GetProjectRoot(), *AbilityName, *AbilityName));
			SearchPaths.Add(FString::Printf(TEXT("%s/Abilities/Forms/%s.%s_C"), *GetProjectRoot(), *AbilityName, *AbilityName));
			SearchPaths.Add(FString::Printf(TEXT("%s/Abilities/Actions/%s.%s_C"), *GetProjectRoot(), *AbilityName, *AbilityName));
			SearchPaths.Add(FString::Printf(TEXT("/Game/FatherCompanion/Abilities/%s.%s_C"), *AbilityName, *AbilityName));

			for (const FString& Path : SearchPaths)
			{
				AbilityClass = LoadClass<UNarrativeGameplayAbility>(nullptr, *Path);
				if (AbilityClass)
				{
					break;
				}
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
			// v4.22: Automated external reference detection
			// If ability is NOT in our manifest, it's an external plugin reference
			// External references can't be resolved in commandlet mode but work at editor load
			bool bIsInManifest = GetActiveManifest() && GetActiveManifest()->IsAssetInManifest(AbilityName);

			if (!bIsInManifest)
			{
				LogGeneration(FString::Printf(TEXT("[W_ABILITYCONFIG_EXTERNAL_ABILITY] %s | External plugin ability '%s' will be resolved at editor load"),
					*Definition.Name, *AbilityName));
				// Continue without adding - will be resolved when editor loads full plugin
			}
			else
			{
				// v4.23 FAIL-FAST: Manifest-defined ability that cannot be resolved
				// v4.23.1: Phase 3 spec-complete - added Subsystem
				LogGeneration(FString::Printf(TEXT("[E_ABILITYCONFIG_ABILITY_NOT_FOUND] %s | Subsystem: GAS | Could not resolve ability: %s"),
					*Definition.Name, *AbilityName));
				return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
					FString::Printf(TEXT("AbilityConfiguration ability '%s' not found"), *AbilityName));
			}
		}
	}

	// v3.1: Resolve StartupEffects (GE_ blueprints -> GameplayEffect subclasses)
	// v4.16.2: Added FormState and Cooldowns subfolders to search paths
	int32 ResolvedEffects = 0;
	for (const FString& EffectName : Definition.StartupEffects)
	{
		TArray<FString> SearchPaths;
		// Standard effect locations
		SearchPaths.Add(FString::Printf(TEXT("%s/Effects/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
		SearchPaths.Add(FString::Printf(TEXT("%s/GameplayEffects/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
		// FormState subfolder (common location for GE_*State effects)
		SearchPaths.Add(FString::Printf(TEXT("%s/Effects/FormState/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
		// Cooldowns subfolder
		SearchPaths.Add(FString::Printf(TEXT("%s/Effects/Cooldowns/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
		// Legacy hardcoded paths
		SearchPaths.Add(FString::Printf(TEXT("/Game/FatherCompanion/Effects/%s.%s_C"), *EffectName, *EffectName));
		SearchPaths.Add(FString::Printf(TEXT("/Game/FatherCompanion/Effects/FormState/%s.%s_C"), *EffectName, *EffectName));

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
			// v4.23 FAIL-FAST: Manifest references startup effect that cannot be resolved
			LogGeneration(FString::Printf(TEXT("[E_ABILITYCONFIG_STARTUPEFFECT_NOT_FOUND] %s | Could not resolve startup effect: %s"),
				*Definition.Name, *EffectName));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("AbilityConfiguration startup effect '%s' not found"), *EffectName));
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
			// v4.23 FAIL-FAST: Manifest references default attributes that cannot be resolved
			LogGeneration(FString::Printf(TEXT("[E_ABILITYCONFIG_DEFAULTATTR_NOT_FOUND] %s | Could not resolve default attributes: %s"),
				*Definition.Name, *Definition.DefaultAttributes));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("AbilityConfiguration default attributes '%s' not found"), *Definition.DefaultAttributes));
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
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("AbilityConfiguration");
	Result.DetermineCategory();
	return Result;
}

// v4.13: Category C - P1.3 Startup Effects Validation
// v4.16.2: Upgraded to FAIL severity per Claude-GPT dual audit (LOCKED)
void FAbilityConfigurationGenerator::ValidateStartupEffects(const FManifestAbilityConfigurationDefinition& Definition, TArray<FString>& OutErrors)
{
	// Form abilities that require a default form state
	static TArray<FString> FormAbilityNames = {
		TEXT("GA_FatherCrawler"),
		TEXT("GA_FatherArmor"),
		TEXT("GA_FatherExoskeleton"),
		TEXT("GA_FatherSymbiote"),
		TEXT("GA_FatherEngineer")
	};

	// Check if this configuration contains any form abilities
	bool bHasFormAbility = false;
	for (const FString& AbilityName : Definition.Abilities)
	{
		if (FormAbilityNames.Contains(AbilityName))
		{
			bHasFormAbility = true;
			break;
		}
	}

	// v4.16.2: Validate all startup effects can be resolved (LOCKED CONTRACT P1.3)
	// Collect all failures before checking form state requirement
	for (const FString& EffectName : Definition.StartupEffects)
	{
		TArray<FString> SearchPaths;
		// Standard effect locations
		SearchPaths.Add(FString::Printf(TEXT("%s/Effects/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
		SearchPaths.Add(FString::Printf(TEXT("%s/GameplayEffects/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
		// FormState subfolder (v4.16.2 - common location for GE_*State effects)
		SearchPaths.Add(FString::Printf(TEXT("%s/Effects/FormState/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
		// Cooldowns subfolder
		SearchPaths.Add(FString::Printf(TEXT("%s/Effects/Cooldowns/%s.%s_C"), *GetProjectRoot(), *EffectName, *EffectName));
		// Legacy hardcoded paths
		SearchPaths.Add(FString::Printf(TEXT("/Game/FatherCompanion/Effects/%s.%s_C"), *EffectName, *EffectName));
		SearchPaths.Add(FString::Printf(TEXT("/Game/FatherCompanion/Effects/FormState/%s.%s_C"), *EffectName, *EffectName));

		UClass* EffectClass = nullptr;
		for (const FString& Path : SearchPaths)
		{
			EffectClass = LoadClass<UGameplayEffect>(nullptr, *Path);
			if (EffectClass)
			{
				break;
			}
		}

		if (!EffectClass)
		{
			OutErrors.Add(FString::Printf(
				TEXT("E_AC_STARTUP_EFFECT_NOT_FOUND | %s | Startup effect '%s' could not be resolved via LoadClass<UGameplayEffect> | Verify effect exists and path is correct"),
				*Definition.Name, *EffectName));
		}
	}

	if (!bHasFormAbility)
	{
		return;  // No form abilities, skip form state validation
	}

	// Check if startup_effects contains a GE_*State effect
	bool bHasFormState = false;
	for (const FString& EffectName : Definition.StartupEffects)
	{
		// Check for GE_*State pattern (e.g., GE_CrawlerState, GE_ArmorState)
		if (EffectName.StartsWith(TEXT("GE_")) && EffectName.EndsWith(TEXT("State")))
		{
			bHasFormState = true;
			break;
		}
	}

	// v4.16.2: FAIL severity - blocks asset creation (LOCKED CONTRACT P1.3)
	if (!bHasFormState)
	{
		OutErrors.Add(FString::Printf(
			TEXT("E_AC_MISSING_FORM_STATE | %s | Ability configuration with form abilities must have a default GE_*State in startup_effects | Add GE_CrawlerState or equivalent"),
			*Definition.Name));
	}
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
		UClass* ActivityClass = nullptr;

		// v4.22: Check session cache FIRST for activities created in this generation session
		if (UClass** CachedClass = GSessionBlueprintClassCache.Find(ActivityName))
		{
			ActivityClass = *CachedClass;
			LogGeneration(FString::Printf(TEXT("  Found activity class in session cache: %s"), *ActivityName));
		}

		// If not in cache, try search paths
		if (!ActivityClass)
		{
			TArray<FString> SearchPaths;
			SearchPaths.Add(FString::Printf(TEXT("%s/Activities/%s.%s_C"), *GetProjectRoot(), *ActivityName, *ActivityName));
			SearchPaths.Add(FString::Printf(TEXT("%s/AI/Activities/%s.%s_C"), *GetProjectRoot(), *ActivityName, *ActivityName));
			SearchPaths.Add(FString::Printf(TEXT("/Game/FatherCompanion/Activities/%s.%s_C"), *ActivityName, *ActivityName));
			// v4.22: Narrative Pro plugin built-in activities (correct plugin mount point)
			// Activities are organized in subdirectories by type
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Attacks/MeleeAttack/%s.%s_C"), *ActivityName, *ActivityName));
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Attacks/ShootAndStrafe/%s.%s_C"), *ActivityName, *ActivityName));
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Attacks/ShootFromCover/%s.%s_C"), *ActivityName, *ActivityName));
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Attacks/Goals/%s.%s_C"), *ActivityName, *ActivityName));
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/FollowCharacter/%s.%s_C"), *ActivityName, *ActivityName));
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Idle/%s.%s_C"), *ActivityName, *ActivityName));
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Patrol/%s.%s_C"), *ActivityName, *ActivityName));
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/GoToLocation/%s.%s_C"), *ActivityName, *ActivityName));
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Flee/%s.%s_C"), *ActivityName, *ActivityName));
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Interact/Goals/%s.%s_C"), *ActivityName, *ActivityName));
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/ReturnToSpawn/%s.%s_C"), *ActivityName, *ActivityName));
			// Legacy path (kept for backwards compatibility)
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro/AI/Activities/%s.%s_C"), *ActivityName, *ActivityName));

			for (const FString& Path : SearchPaths)
			{
				ActivityClass = LoadClass<UNPCActivity>(nullptr, *Path);
				if (ActivityClass)
				{
					break;
				}
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
			// v4.22: Automated external reference detection
			// If activity is NOT in our manifest, it's an external plugin reference
			// External references can't be resolved in commandlet mode but work at editor load
			bool bIsInManifest = GetActiveManifest() && GetActiveManifest()->IsAssetInManifest(ActivityName);

			if (!bIsInManifest)
			{
				LogGeneration(FString::Printf(TEXT("[W_ACTIVITYCONFIG_EXTERNAL_ACTIVITY] %s | External plugin activity '%s' will be resolved at editor load"),
					*Definition.Name, *ActivityName));
				// Continue without adding - will be resolved when editor loads full plugin
			}
			else
			{
				// v4.23 FAIL-FAST: Manifest-defined activity that cannot be resolved
				// v4.23.1: Phase 3 spec-complete - added Subsystem
				LogGeneration(FString::Printf(TEXT("[E_ACTIVITYCONFIG_ACTIVITY_NOT_FOUND] %s | Subsystem: GAS | Could not resolve activity: %s"),
					*Definition.Name, *ActivityName));
				return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
					FString::Printf(TEXT("ActivityConfiguration activity '%s' not found"), *ActivityName));
			}
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
		// v4.22: Narrative Pro plugin built-in goal generators (correct plugin mount point)
		SearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Attacks/Goals/%s.%s_C"), *GeneratorName, *GeneratorName));
		SearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Activities/Interact/Goals/%s.%s_C"), *GeneratorName, *GeneratorName));
		SearchPaths.Add(FString::Printf(TEXT("/NarrativePro22B57/Pro/Core/AI/Goals/%s.%s_C"), *GeneratorName, *GeneratorName));
		// Legacy path (kept for backwards compatibility)
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
			// v4.22: Automated external reference detection
			// If goal generator is NOT in our manifest, it's an external plugin reference
			// External references can't be resolved in commandlet mode but work at editor load
			bool bIsInManifest = GetActiveManifest() && GetActiveManifest()->IsAssetInManifest(GeneratorName);

			if (!bIsInManifest)
			{
				LogGeneration(FString::Printf(TEXT("[W_ACTIVITYCONFIG_EXTERNAL_GENERATOR] %s | External plugin goal generator '%s' will be resolved at editor load"),
					*Definition.Name, *GeneratorName));
				// Continue without adding - will be resolved when editor loads full plugin
			}
			else
			{
				// v4.23 FAIL-FAST: Manifest-defined goal generator that cannot be resolved
				LogGeneration(FString::Printf(TEXT("[E_ACTIVITYCONFIG_GOALGENERATOR_NOT_FOUND] %s | Could not resolve goal generator: %s"),
					*Definition.Name, *GeneratorName));
				return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
					FString::Printf(TEXT("ActivityConfiguration goal generator '%s' not found"), *GeneratorName));
			}
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
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("ActivityConfiguration");
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
		// Build full path if relative - NO "_C" suffix for TSoftClassPtr storage
		// UE resolves Blueprint class internally when loading
		if (!ItemPath.Contains(TEXT("/")))
		{
			ItemPath = FString::Printf(TEXT("%s/Items/%s.%s"), *GetProjectRoot(), *ItemDef.ItemClass, *ItemDef.ItemClass);
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
		// NO "_C" suffix for TSoftClassPtr storage
		if (!ItemPath.Contains(TEXT("/")))
		{
			ItemPath = FString::Printf(TEXT("%s/Items/%s.%s"), *GetProjectRoot(), *ItemName, *ItemName);
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
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("ItemCollection");
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

	// v4.16: Compile with validation (Contract 10 - Blueprint Compile Gate)
	FCompilerResultsLog CompileLog;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &CompileLog);

	if (CompileLog.NumErrors > 0)
	{
		TArray<FString> ErrorMessages;
		for (const TSharedRef<FTokenizedMessage>& Msg : CompileLog.Messages)
		{
			if (Msg->GetSeverity() == EMessageSeverity::Error)
			{
				ErrorMessages.Add(Msg->ToText().ToString());
			}
		}

		UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Narrative Event Blueprint compilation failed with %d errors: %s"),
			*Definition.Name, CompileLog.NumErrors, *FString::Join(ErrorMessages, TEXT("; ")));

		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Compile failed: %s"), *FString::Join(ErrorMessages, TEXT("; "))));
		Result.AssetPath = AssetPath;
		Result.GeneratorId = TEXT("NarrativeEvent");
		Result.DetermineCategory();
		return Result;
	}

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

		// v4.14: Removed recompile - would wipe CDO property changes
		// Initial compile at line 14840 is sufficient; CDO changes persist without recompile
	}

	// Log event details
	if (!Definition.EventType.IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("  Event '%s' Type: %s"), *Definition.Name, *Definition.EventType));
	}
	if (!Definition.Description.IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("  Event '%s' Description: %s"), *Definition.Name, *Definition.Description));
	}

	// v4.0: Populate NPC target arrays via reflection (previously just logged)
	if (Definition.NPCTargets.Num() > 0 && CDO)
	{
		FArrayProperty* NPCTargetsProp = CastField<FArrayProperty>(
			CDO->GetClass()->FindPropertyByName(TEXT("NPCTargets")));

		if (NPCTargetsProp)
		{
			FSoftObjectProperty* InnerProp = CastField<FSoftObjectProperty>(NPCTargetsProp->Inner);
			if (InnerProp)
			{
				FScriptArrayHelper ArrayHelper(NPCTargetsProp, NPCTargetsProp->ContainerPtrToValuePtr<void>(CDO));
				ArrayHelper.EmptyValues();

				for (const FString& TargetName : Definition.NPCTargets)
				{
					// Build asset paths - check multiple locations
					TArray<FString> SearchPaths = {
						FString::Printf(TEXT("%s/NPCs/Definitions/%s.%s"), *GetProjectRoot(), *TargetName, *TargetName),
						FString::Printf(TEXT("%s/NPCs/%s.%s"), *GetProjectRoot(), *TargetName, *TargetName),
						FString::Printf(TEXT("%s/Characters/NPCs/%s.%s"), *GetProjectRoot(), *TargetName, *TargetName),
					};

					UNPCDefinition* NPCDef = nullptr;
					for (const FString& SearchPath : SearchPaths)
					{
						NPCDef = LoadObject<UNPCDefinition>(nullptr, *SearchPath);
						if (NPCDef) break;
					}

					if (NPCDef)
					{
						int32 NewIndex = ArrayHelper.AddValue();
						void* ElementPtr = ArrayHelper.GetRawPtr(NewIndex);
						FSoftObjectPtr SoftPtr(NPCDef);
						InnerProp->SetPropertyValue(ElementPtr, SoftPtr);
						LogGeneration(FString::Printf(TEXT("  Added NPCTarget: %s"), *TargetName));
					}
					else
					{
						LogGeneration(FString::Printf(TEXT("  WARNING: Could not find NPCDefinition: %s"), *TargetName));
					}
				}
			}
			else
			{
				// Fallback: Try as TObjectPtr array
				FObjectProperty* InnerObjProp = CastField<FObjectProperty>(NPCTargetsProp->Inner);
				if (InnerObjProp)
				{
					FScriptArrayHelper ArrayHelper(NPCTargetsProp, NPCTargetsProp->ContainerPtrToValuePtr<void>(CDO));
					ArrayHelper.EmptyValues();

					for (const FString& TargetName : Definition.NPCTargets)
					{
						FString AssetPath_NPC = FString::Printf(TEXT("%s/NPCs/Definitions/%s.%s"),
							*GetProjectRoot(), *TargetName, *TargetName);
						UNPCDefinition* NPCDef = LoadObject<UNPCDefinition>(nullptr, *AssetPath_NPC);

						if (NPCDef)
						{
							int32 NewIndex = ArrayHelper.AddValue();
							void* ElementPtr = ArrayHelper.GetRawPtr(NewIndex);
							InnerObjProp->SetObjectPropertyValue(ElementPtr, NPCDef);
							LogGeneration(FString::Printf(TEXT("  Added NPCTarget: %s"), *TargetName));
						}
					}
				}
			}
		}
		else
		{
			// v4.23 FAIL-FAST: Property exists in NP but FindPropertyByName failed (generator bug)
			// v4.23.1: Phase 3 spec-complete - added Subsystem, SuperClassPath, RequestedProperty
			UClass* TargetClass = CDO->GetClass();
			LogGeneration(FString::Printf(TEXT("[E_NPCTARGETS_NOT_FOUND] %s | Subsystem: NPC | NPCTargets property not found | ClassPath: %s | SuperClassPath: %s | RequestedProperty: NPCTargets | Targets requested: %d"),
				*Definition.Name, *TargetClass->GetPathName(),
				TargetClass->GetSuperClass() ? *TargetClass->GetSuperClass()->GetPathName() : TEXT("None"),
				Definition.NPCTargets.Num()));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				TEXT("NPCTargets property not found - manifest references targets but property lookup failed"));
		}
	}

	// v4.0: Populate CharacterTargets array
	if (Definition.CharacterTargets.Num() > 0 && CDO)
	{
		FArrayProperty* CharTargetsProp = CastField<FArrayProperty>(
			CDO->GetClass()->FindPropertyByName(TEXT("CharacterTargets")));

		if (CharTargetsProp)
		{
			FSoftObjectProperty* InnerProp = CastField<FSoftObjectProperty>(CharTargetsProp->Inner);
			if (InnerProp)
			{
				FScriptArrayHelper ArrayHelper(CharTargetsProp, CharTargetsProp->ContainerPtrToValuePtr<void>(CDO));
				ArrayHelper.EmptyValues();

				for (const FString& TargetName : Definition.CharacterTargets)
				{
					TArray<FString> SearchPaths = {
						FString::Printf(TEXT("%s/Characters/Definitions/%s.%s"), *GetProjectRoot(), *TargetName, *TargetName),
						FString::Printf(TEXT("%s/Characters/%s.%s"), *GetProjectRoot(), *TargetName, *TargetName),
					};

					UCharacterDefinition* CharDef = nullptr;
					for (const FString& SearchPath : SearchPaths)
					{
						CharDef = LoadObject<UCharacterDefinition>(nullptr, *SearchPath);
						if (CharDef) break;
					}

					if (CharDef)
					{
						int32 NewIndex = ArrayHelper.AddValue();
						void* ElementPtr = ArrayHelper.GetRawPtr(NewIndex);
						FSoftObjectPtr SoftPtr(CharDef);
						InnerProp->SetPropertyValue(ElementPtr, SoftPtr);
						LogGeneration(FString::Printf(TEXT("  Added CharacterTarget: %s"), *TargetName));
					}
					else
					{
						LogGeneration(FString::Printf(TEXT("  WARNING: Could not find CharacterDefinition: %s"), *TargetName));
					}
				}
			}
		}
		else
		{
			// v4.23 FAIL-FAST: Property exists in NP but FindPropertyByName failed (generator bug)
			// v4.23.1: Phase 3 spec-complete - added Subsystem, SuperClassPath, RequestedProperty
			UClass* TargetClass = CDO->GetClass();
			LogGeneration(FString::Printf(TEXT("[E_CHARACTERTARGETS_NOT_FOUND] %s | Subsystem: NPC | CharacterTargets property not found | ClassPath: %s | SuperClassPath: %s | RequestedProperty: CharacterTargets | Targets requested: %d"),
				*Definition.Name, *TargetClass->GetPathName(),
				TargetClass->GetSuperClass() ? *TargetClass->GetSuperClass()->GetPathName() : TEXT("None"),
				Definition.CharacterTargets.Num()));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				TEXT("CharacterTargets property not found - manifest references targets but property lookup failed"));
		}
	}

	// PlayerTargets - these are typically runtime determined, just log for reference
	if (Definition.PlayerTargets.Num() > 0)
	{
		LogGeneration(FString::Printf(TEXT("  Event '%s' PlayerTargets (%d): %s"), *Definition.Name, Definition.PlayerTargets.Num(), *FString::Join(Definition.PlayerTargets, TEXT(", "))));
	}

	// v4.1: Set child class-specific properties via reflection introspection
	if (Definition.Properties.Num() > 0 && CDO)
	{
		for (const auto& PropPair : Definition.Properties)
		{
			const FString& PropertyName = PropPair.Key;
			const FString& PropertyValue = PropPair.Value;

			FProperty* Prop = CDO->GetClass()->FindPropertyByName(FName(*PropertyName));
			if (!Prop)
			{
				LogGeneration(FString::Printf(TEXT("  WARNING: Property '%s' not found on %s"), *PropertyName, *CDO->GetClass()->GetName()));
				continue;
			}

			// Handle different property types
			if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
			{
				IntProp->SetPropertyValue_InContainer(CDO, FCString::Atoi(*PropertyValue));
				LogGeneration(FString::Printf(TEXT("  Set %s: %s (int)"), *PropertyName, *PropertyValue));
			}
			else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
			{
				FloatProp->SetPropertyValue_InContainer(CDO, FCString::Atof(*PropertyValue));
				LogGeneration(FString::Printf(TEXT("  Set %s: %s (float)"), *PropertyName, *PropertyValue));
			}
			else if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
			{
				StrProp->SetPropertyValue_InContainer(CDO, PropertyValue);
				LogGeneration(FString::Printf(TEXT("  Set %s: %s (string)"), *PropertyName, *PropertyValue));
			}
			else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
			{
				BoolProp->SetPropertyValue_InContainer(CDO, PropertyValue.ToBool());
				LogGeneration(FString::Printf(TEXT("  Set %s: %s (bool)"), *PropertyName, *PropertyValue));
			}
			else if (FNameProperty* NameProp = CastField<FNameProperty>(Prop))
			{
				NameProp->SetPropertyValue_InContainer(CDO, FName(*PropertyValue));
				LogGeneration(FString::Printf(TEXT("  Set %s: %s (name)"), *PropertyName, *PropertyValue));
			}
			else if (FTextProperty* TextProp = CastField<FTextProperty>(Prop))
			{
				TextProp->SetPropertyValue_InContainer(CDO, FText::FromString(PropertyValue));
				LogGeneration(FString::Printf(TEXT("  Set %s: %s (text)"), *PropertyName, *PropertyValue));
			}
			else
			{
				// v4.23 FAIL-FAST: Manifest references property with unsupported type
				LogGeneration(FString::Printf(TEXT("[E_NE_PROPERTY_UNSUPPORTED] %s | Unsupported property type for '%s'"), *Definition.Name, *PropertyName));
				return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
					FString::Printf(TEXT("Unsupported property type for '%s'"), *PropertyName));
			}
		}
	}

	// v4.3: Create Event Conditions (TArray<UNarrativeCondition*> instanced objects)
	if (Definition.Conditions.Num() > 0 && CDO)
	{
		LogGeneration(FString::Printf(TEXT("  Processing %d event conditions..."), Definition.Conditions.Num()));

		FArrayProperty* ConditionsArrayProp = CastField<FArrayProperty>(
			CDO->GetClass()->FindPropertyByName(TEXT("Conditions")));

		if (ConditionsArrayProp)
		{
			FScriptArrayHelper ArrayHelper(ConditionsArrayProp, ConditionsArrayProp->ContainerPtrToValuePtr<void>(CDO));
			ArrayHelper.EmptyValues();

			for (const auto& CondDef : Definition.Conditions)
			{
				// Try to load the condition class
				UClass* CondClass = nullptr;
				TArray<FString> SearchPaths;
				SearchPaths.Add(FString::Printf(TEXT("/Script/NarrativeArsenal.%s"), *CondDef.Type));
				SearchPaths.Add(FString::Printf(TEXT("/Game/FatherCompanion/Conditions/%s.%s_C"), *CondDef.Type, *CondDef.Type));
				SearchPaths.Add(FString::Printf(TEXT("/Game/Conditions/%s.%s_C"), *CondDef.Type, *CondDef.Type));
				SearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Conditions/%s.%s_C"), *CondDef.Type, *CondDef.Type));

				for (const FString& Path : SearchPaths)
				{
					CondClass = LoadClass<UObject>(nullptr, *Path);
					if (CondClass) break;
				}

				if (CondClass)
				{
					// Create instanced condition object
					UObject* CondInstance = NewObject<UObject>(CDO, CondClass);
					if (CondInstance)
					{
						// Set bNot property
						if (CondDef.bNot)
						{
							if (FBoolProperty* NotProp = CastField<FBoolProperty>(CondClass->FindPropertyByName(TEXT("bNot"))))
							{
								NotProp->SetPropertyValue_InContainer(CondInstance, true);
							}
						}

						// Set other properties via reflection
						for (const auto& PropPair : CondDef.Properties)
						{
							FProperty* Prop = CondClass->FindPropertyByName(FName(*PropPair.Key));
							if (Prop)
							{
								Prop->ImportText_Direct(*PropPair.Value, Prop->ContainerPtrToValuePtr<void>(CondInstance), nullptr, PPF_None);
							}
						}

						// Add to array
						FObjectProperty* InnerProp = CastField<FObjectProperty>(ConditionsArrayProp->Inner);
						if (InnerProp)
						{
							int32 NewIndex = ArrayHelper.AddValue();
							InnerProp->SetObjectPropertyValue(ArrayHelper.GetRawPtr(NewIndex), CondInstance);
						}

						LogGeneration(FString::Printf(TEXT("    Added condition: %s%s"), CondDef.bNot ? TEXT("NOT ") : TEXT(""), *CondDef.Type));
					}
				}
				else
				{
					// v4.23 FAIL-FAST: Manifest references condition class that cannot be loaded
					LogGeneration(FString::Printf(TEXT("[E_EVENT_CONDITION_CLASS_NOT_FOUND] %s | Could not load condition class: %s"),
						*Definition.Name, *CondDef.Type));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						FString::Printf(TEXT("Event condition class '%s' not found"), *CondDef.Type));
				}
			}
		}
		else
		{
			// v4.23 FAIL-FAST: Conditions specified but array property not found
			LogGeneration(FString::Printf(TEXT("[E_EVENT_CONDITIONS_PROPERTY_NOT_FOUND] %s | Could not find Conditions array property"),
				*Definition.Name));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				TEXT("Event conditions specified but Conditions array property not found on class"));
		}
	}

	// v4.16: Recompile with validation after setting target arrays, properties, and conditions (Contract 10)
	if (CDO && (Definition.NPCTargets.Num() > 0 || Definition.CharacterTargets.Num() > 0 || Definition.Properties.Num() > 0 || Definition.Conditions.Num() > 0))
	{
		FCompilerResultsLog FinalCompileLog;
		FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &FinalCompileLog);

		if (FinalCompileLog.NumErrors > 0)
		{
			TArray<FString> ErrorMessages;
			for (const TSharedRef<FTokenizedMessage>& Msg : FinalCompileLog.Messages)
			{
				if (Msg->GetSeverity() == EMessageSeverity::Error)
				{
					ErrorMessages.Add(Msg->ToText().ToString());
				}
			}

			UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Narrative Event final compilation failed with %d errors: %s"),
				*Definition.Name, FinalCompileLog.NumErrors, *FString::Join(ErrorMessages, TEXT("; ")));

			Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("Final compile failed: %s"), *FString::Join(ErrorMessages, TEXT("; "))));
			Result.AssetPath = AssetPath;
			Result.GeneratorId = TEXT("NarrativeEvent");
			Result.DetermineCategory();
			return Result;
		}
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
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("NarrativeEvent");
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// v4.0: FGameplayCueGenerator Implementation - NEW
// ============================================================================

FGenerationResult FGameplayCueGenerator::Generate(const FManifestGameplayCueDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("FX/GameplayCues") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	// Validate prefix
	if (!Definition.Name.StartsWith(TEXT("GC_")))
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			TEXT("Gameplay Cue name must start with GC_ prefix"));
	}

	if (ValidateAgainstManifest(Definition.Name, TEXT("Gameplay Cue"), Result))
	{
		return Result;
	}

	// v3.0: Check existence with metadata-aware logic
	uint64 InputHash = Definition.ComputeHash();
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Gameplay Cue"), InputHash, Result))
	{
		return Result;
	}

	// Determine parent class based on CueType
	UClass* ParentClass = nullptr;
	if (Definition.CueType.Equals(TEXT("Burst"), ESearchCase::IgnoreCase))
	{
		ParentClass = UGameplayCueNotify_Burst::StaticClass();
	}
	else if (Definition.CueType.Equals(TEXT("BurstLatent"), ESearchCase::IgnoreCase))
	{
		// BurstLatent is a UObject-based cue with duration support
		ParentClass = UGameplayCueNotify_Burst::StaticClass();  // Same base, different usage
	}
	else if (Definition.CueType.Equals(TEXT("Actor"), ESearchCase::IgnoreCase))
	{
		ParentClass = AGameplayCueNotify_Actor::StaticClass();
	}
	else
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Unknown CueType: %s. Use Burst, BurstLatent, or Actor"), *Definition.CueType));
	}

	if (!ParentClass)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			TEXT("Gameplay Cue parent class not found - ensure GameplayAbilities plugin is enabled"));
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	// Create Blueprint
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
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			TEXT("Failed to create Gameplay Cue Blueprint"));
	}

	// v4.16: Compile with validation (Contract 10 - Blueprint Compile Gate)
	FCompilerResultsLog CompileLog;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &CompileLog);

	if (CompileLog.NumErrors > 0)
	{
		TArray<FString> ErrorMessages;
		for (const TSharedRef<FTokenizedMessage>& Msg : CompileLog.Messages)
		{
			if (Msg->GetSeverity() == EMessageSeverity::Error)
			{
				ErrorMessages.Add(Msg->ToText().ToString());
			}
		}

		UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Gameplay Cue Blueprint compilation failed with %d errors: %s"),
			*Definition.Name, CompileLog.NumErrors, *FString::Join(ErrorMessages, TEXT("; ")));

		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Compile failed: %s"), *FString::Join(ErrorMessages, TEXT("; "))));
		Result.AssetPath = AssetPath;
		Result.GeneratorId = TEXT("GameplayCue");
		Result.DetermineCategory();
		return Result;
	}

	// Get CDO to set properties
	UObject* CDO = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
	if (CDO)
	{
		// Set GameplayCueTag
		if (!Definition.GameplayCueTag.IsEmpty())
		{
			FStructProperty* TagProp = CastField<FStructProperty>(
				CDO->GetClass()->FindPropertyByName(TEXT("GameplayCueTag")));
			if (TagProp)
			{
				FGameplayTag* TagPtr = TagProp->ContainerPtrToValuePtr<FGameplayTag>(CDO);
				if (TagPtr)
				{
					*TagPtr = FGameplayTag::RequestGameplayTag(FName(*Definition.GameplayCueTag), false);
					LogGeneration(FString::Printf(TEXT("  Set GameplayCueTag: %s"), *Definition.GameplayCueTag));
				}
			}
		}

		// v4.0.1: Set BurstEffects properties via reflection
		FStructProperty* BurstEffectsProp = CastField<FStructProperty>(
			CDO->GetClass()->FindPropertyByName(TEXT("BurstEffects")));
		if (BurstEffectsProp)
		{
			void* BurstEffectsPtr = BurstEffectsProp->ContainerPtrToValuePtr<void>(CDO);
			UScriptStruct* BurstEffectsStruct = BurstEffectsProp->Struct;

			// Set ParticleSystem (add to BurstParticles array)
			if (!Definition.BurstEffects.ParticleSystem.IsEmpty())
			{
				// Load the Niagara system
				UNiagaraSystem* NiagaraSys = LoadObject<UNiagaraSystem>(nullptr, *Definition.BurstEffects.ParticleSystem);
				if (!NiagaraSys)
				{
					// Try with project root prefix
					FString FullPath = FString::Printf(TEXT("%s/FX/%s.%s"),
						*GetProjectRoot(), *Definition.BurstEffects.ParticleSystem, *Definition.BurstEffects.ParticleSystem);
					NiagaraSys = LoadObject<UNiagaraSystem>(nullptr, *FullPath);
				}

				if (NiagaraSys)
				{
					FArrayProperty* ParticlesProp = CastField<FArrayProperty>(
						BurstEffectsStruct->FindPropertyByName(TEXT("BurstParticles")));
					if (ParticlesProp)
					{
						FScriptArrayHelper ArrayHelper(ParticlesProp, ParticlesProp->ContainerPtrToValuePtr<void>(BurstEffectsPtr));
						int32 NewIdx = ArrayHelper.AddValue();
						void* ParticleInfoPtr = ArrayHelper.GetRawPtr(NewIdx);

						// Set NiagaraSystem on the ParticleInfo struct
						FStructProperty* InnerStruct = CastField<FStructProperty>(ParticlesProp->Inner);
						if (InnerStruct)
						{
							FObjectProperty* NiagaraProp = CastField<FObjectProperty>(
								InnerStruct->Struct->FindPropertyByName(TEXT("NiagaraSystem")));
							if (NiagaraProp)
							{
								NiagaraProp->SetObjectPropertyValue(
									NiagaraProp->ContainerPtrToValuePtr<void>(ParticleInfoPtr), NiagaraSys);
								LogGeneration(FString::Printf(TEXT("  Set BurstEffects.ParticleSystem: %s"), *Definition.BurstEffects.ParticleSystem));
							}
						}
					}
				}
				else
				{
					// v4.23 FAIL-FAST: Manifest references particle system that cannot be loaded
					LogGeneration(FString::Printf(TEXT("[E_GAMEPLAYCUE_PARTICLE_NOT_FOUND] %s | Could not load ParticleSystem: %s"),
						*Definition.Name, *Definition.BurstEffects.ParticleSystem));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						FString::Printf(TEXT("GameplayCue ParticleSystem '%s' not found"), *Definition.BurstEffects.ParticleSystem));
				}
			}

			// Set Sound (add to BurstSounds array)
			if (!Definition.BurstEffects.Sound.IsEmpty())
			{
				USoundBase* SoundAsset = LoadObject<USoundBase>(nullptr, *Definition.BurstEffects.Sound);
				if (!SoundAsset)
				{
					FString FullPath = FString::Printf(TEXT("%s/Audio/%s.%s"),
						*GetProjectRoot(), *Definition.BurstEffects.Sound, *Definition.BurstEffects.Sound);
					SoundAsset = LoadObject<USoundBase>(nullptr, *FullPath);
				}

				if (SoundAsset)
				{
					FArrayProperty* SoundsProp = CastField<FArrayProperty>(
						BurstEffectsStruct->FindPropertyByName(TEXT("BurstSounds")));
					if (SoundsProp)
					{
						FScriptArrayHelper ArrayHelper(SoundsProp, SoundsProp->ContainerPtrToValuePtr<void>(BurstEffectsPtr));
						int32 NewIdx = ArrayHelper.AddValue();
						void* SoundInfoPtr = ArrayHelper.GetRawPtr(NewIdx);

						FStructProperty* InnerStruct = CastField<FStructProperty>(SoundsProp->Inner);
						if (InnerStruct)
						{
							FObjectProperty* SoundProp = CastField<FObjectProperty>(
								InnerStruct->Struct->FindPropertyByName(TEXT("Sound")));
							if (SoundProp)
							{
								SoundProp->SetObjectPropertyValue(
									SoundProp->ContainerPtrToValuePtr<void>(SoundInfoPtr), SoundAsset);
								LogGeneration(FString::Printf(TEXT("  Set BurstEffects.Sound: %s"), *Definition.BurstEffects.Sound));
							}
						}
					}
				}
				else
				{
					// v4.23 FAIL-FAST: Manifest references sound that cannot be loaded
					LogGeneration(FString::Printf(TEXT("[E_GAMEPLAYCUE_SOUND_NOT_FOUND] %s | Could not load Sound: %s"),
						*Definition.Name, *Definition.BurstEffects.Sound));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						FString::Printf(TEXT("GameplayCue Sound '%s' not found"), *Definition.BurstEffects.Sound));
				}
			}

			// Set CameraShake (on BurstCameraShake struct)
			if (!Definition.BurstEffects.CameraShake.IsEmpty())
			{
				UClass* ShakeClass = LoadClass<UCameraShakeBase>(nullptr, *Definition.BurstEffects.CameraShake);
				if (!ShakeClass)
				{
					FString FullPath = FString::Printf(TEXT("/Script/Engine.%s"), *Definition.BurstEffects.CameraShake);
					ShakeClass = LoadClass<UCameraShakeBase>(nullptr, *FullPath);
				}

				if (ShakeClass)
				{
					FStructProperty* CameraShakeProp = CastField<FStructProperty>(
						BurstEffectsStruct->FindPropertyByName(TEXT("BurstCameraShake")));
					if (CameraShakeProp)
					{
						void* CameraShakePtr = CameraShakeProp->ContainerPtrToValuePtr<void>(BurstEffectsPtr);
						FClassProperty* ShakeClassProp = CastField<FClassProperty>(
							CameraShakeProp->Struct->FindPropertyByName(TEXT("CameraShake")));
						if (ShakeClassProp)
						{
							ShakeClassProp->SetPropertyValue_InContainer(CameraShakePtr, ShakeClass);
							LogGeneration(FString::Printf(TEXT("  Set BurstEffects.CameraShake: %s"), *Definition.BurstEffects.CameraShake));
						}
					}
				}
				else
				{
					// v4.23 FAIL-FAST: Manifest references camera shake that cannot be loaded
					LogGeneration(FString::Printf(TEXT("[E_GAMEPLAYCUE_CAMERASHAKE_NOT_FOUND] %s | Could not load CameraShake class: %s"),
						*Definition.Name, *Definition.BurstEffects.CameraShake));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						FString::Printf(TEXT("GameplayCue CameraShake '%s' not found"), *Definition.BurstEffects.CameraShake));
				}
			}
		}

		// Log spawn condition if set
		if (Definition.SpawnCondition.SpawnProbability < 1.0f)
		{
			LogGeneration(FString::Printf(TEXT("  SpawnCondition.SpawnProbability: %.2f"),
				Definition.SpawnCondition.SpawnProbability));
		}
		if (!Definition.SpawnCondition.AttachSocket.IsEmpty())
		{
			LogGeneration(FString::Printf(TEXT("  SpawnCondition.AttachSocket: %s"),
				*Definition.SpawnCondition.AttachSocket));
		}

		// Mark CDO dirty
		CDO->MarkPackageDirty();
	}

	// v4.14: Removed recompile - would wipe CDO property changes
	// Initial compile at line 15281 is sufficient; CDO changes persist without recompile

	// Save asset
	FAssetRegistryModule::AssetCreated(Blueprint);
	Package->MarkPackageDirty();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Gameplay Cue: %s (Type: %s, Tag: %s)"),
		*Definition.Name, *Definition.CueType, *Definition.GameplayCueTag));

	// v3.0: Store metadata for regeneration tracking
	StoreBlueprintMetadata(Blueprint, TEXT("GC"), Definition.Name, InputHash);

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New,
		FString::Printf(TEXT("Created at %s"), *AssetPath));
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("GameplayCue");
	Result.DetermineCategory();
	return Result;
}

// v3.9.5: Helper function to populate TArray<FLootTableRoll> from manifest definitions
static void PopulateLootTableRollArray(
	UObject* Container,
	FArrayProperty* ArrayProp,
	const TArray<FManifestLootTableRollDefinition>& Rolls,
	const FString& PropertyName)
{
	FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Container));
	FStructProperty* InnerStructProp = CastField<FStructProperty>(ArrayProp->Inner);
	if (!InnerStructProp)
	{
		UE_LOG(LogGasAbilityGenerator, Warning, TEXT("  [v3.9.5 WARNING] %s inner is not a struct"), *PropertyName);
		return;
	}

	UScriptStruct* LootRollStruct = InnerStructProp->Struct;

	for (const FManifestLootTableRollDefinition& RollDef : Rolls)
	{
		int32 NewIndex = ArrayHelper.AddValue();
		void* RollPtr = ArrayHelper.GetRawPtr(NewIndex);

		// Set NumRolls
		FIntProperty* NumRollsProp = CastField<FIntProperty>(LootRollStruct->FindPropertyByName(TEXT("NumRolls")));
		if (NumRollsProp)
		{
			NumRollsProp->SetPropertyValue_InContainer(RollPtr, RollDef.NumRolls);
		}

		// Set Chance
		FFloatProperty* ChanceProp = CastField<FFloatProperty>(LootRollStruct->FindPropertyByName(TEXT("Chance")));
		if (ChanceProp)
		{
			ChanceProp->SetPropertyValue_InContainer(RollPtr, RollDef.Chance);
		}

		// Set TableToRoll (TObjectPtr<UDataTable>)
		if (!RollDef.TableToRoll.IsEmpty())
		{
			FObjectProperty* TableProp = CastField<FObjectProperty>(LootRollStruct->FindPropertyByName(TEXT("TableToRoll")));
			if (TableProp)
			{
				UDataTable* LoadedTable = LoadObject<UDataTable>(nullptr, *RollDef.TableToRoll);
				if (LoadedTable)
				{
					TableProp->SetObjectPropertyValue(TableProp->ContainerPtrToValuePtr<void>(RollPtr), LoadedTable);
					UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [v3.9.5] Set TableToRoll: %s"), *RollDef.TableToRoll);
				}
				else
				{
					UE_LOG(LogGasAbilityGenerator, Warning, TEXT("  [v3.9.5 WARNING] TableToRoll not found: %s"), *RollDef.TableToRoll);
				}
			}
		}

		// Populate ItemsToGrant (TArray<FItemWithQuantity>)
		if (RollDef.ItemsToGrant.Num() > 0)
		{
			FArrayProperty* ItemsArrayProp = CastField<FArrayProperty>(LootRollStruct->FindPropertyByName(TEXT("ItemsToGrant")));
			if (ItemsArrayProp)
			{
				FScriptArrayHelper ItemsHelper(ItemsArrayProp, ItemsArrayProp->ContainerPtrToValuePtr<void>(RollPtr));
				FStructProperty* ItemStructProp = CastField<FStructProperty>(ItemsArrayProp->Inner);

				if (ItemStructProp)
				{
					UScriptStruct* ItemWithQtyStruct = ItemStructProp->Struct;

					for (const FManifestItemWithQuantityDefinition& ItemDef : RollDef.ItemsToGrant)
					{
						int32 ItemIndex = ItemsHelper.AddValue();
						void* ItemPtr = ItemsHelper.GetRawPtr(ItemIndex);

						// Set Item (TSoftClassPtr<UNarrativeItem>)
						// For TSoftClassPtr storage, use path WITHOUT "_C" suffix
						// UE resolves Blueprint class internally when loading
						FString ItemPath = ItemDef.Item;
						if (!ItemPath.Contains(TEXT("/")))
						{
							// Try to resolve short name to full path - use "_C" for LoadClass verification
							TArray<FString> SearchPaths = {
								FString::Printf(TEXT("%s/Items/%s.%s_C"), *GetProjectRoot(), *ItemDef.Item, *ItemDef.Item),
								FString::Printf(TEXT("%s/Items/Equipment/%s.%s_C"), *GetProjectRoot(), *ItemDef.Item, *ItemDef.Item),
								FString::Printf(TEXT("%s/Items/Weapons/%s.%s_C"), *GetProjectRoot(), *ItemDef.Item, *ItemDef.Item)
							};

							FString FoundFolder = TEXT("Items");
							for (int32 i = 0; i < SearchPaths.Num(); i++)
							{
								if (UClass* FoundClass = LoadClass<UObject>(nullptr, *SearchPaths[i]))
								{
									// Found - determine folder for storage path
									if (i == 1) FoundFolder = TEXT("Items/Equipment");
									else if (i == 2) FoundFolder = TEXT("Items/Weapons");
									break;
								}
							}

							// Build storage path WITHOUT "_C" for TSoftClassPtr
							ItemPath = FString::Printf(TEXT("%s/%s/%s.%s"), *GetProjectRoot(), *FoundFolder, *ItemDef.Item, *ItemDef.Item);
						}
						// If full path provided, strip "_C" if present for consistent storage
						if (ItemPath.EndsWith(TEXT("_C")))
						{
							ItemPath = ItemPath.LeftChop(2);
						}

						// Set via FSoftClassPath on the "Item" property (TSoftClassPtr)
						FSoftObjectProperty* ItemSoftProp = CastField<FSoftObjectProperty>(ItemWithQtyStruct->FindPropertyByName(TEXT("Item")));
						if (ItemSoftProp)
						{
							FSoftObjectPtr* SoftPtr = ItemSoftProp->GetPropertyValuePtr_InContainer(ItemPtr);
							if (SoftPtr)
							{
								*SoftPtr = FSoftObjectPtr(FSoftObjectPath(ItemPath));
								UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [v3.9.5] Added Item: %s (Qty: %d)"), *ItemDef.Item, ItemDef.Quantity);
							}
						}

						// Set Quantity
						FIntProperty* QtyProp = CastField<FIntProperty>(ItemWithQtyStruct->FindPropertyByName(TEXT("Quantity")));
						if (QtyProp)
						{
							QtyProp->SetPropertyValue_InContainer(ItemPtr, ItemDef.Quantity);
						}
					}
				}
			}
		}

		// Populate ItemCollectionsToGrant (TArray<TObjectPtr<UItemCollection>>)
		if (RollDef.ItemCollectionsToGrant.Num() > 0)
		{
			FArrayProperty* CollectionsArrayProp = CastField<FArrayProperty>(LootRollStruct->FindPropertyByName(TEXT("ItemCollectionsToGrant")));
			if (CollectionsArrayProp)
			{
				FScriptArrayHelper CollectionsHelper(CollectionsArrayProp, CollectionsArrayProp->ContainerPtrToValuePtr<void>(RollPtr));

				for (const FString& CollectionName : RollDef.ItemCollectionsToGrant)
				{
					FString CollectionPath = CollectionName;
					UObject* LoadedCollection = nullptr;

					if (!CollectionPath.Contains(TEXT("/")))
					{
						// Try common paths
						TArray<FString> SearchPaths = {
							FString::Printf(TEXT("%s/Items/%s"), *GetProjectRoot(), *CollectionName),
							FString::Printf(TEXT("%s/Items/Collections/%s"), *GetProjectRoot(), *CollectionName),
							FString::Printf(TEXT("/NarrativePro/Pro/Demo/Items/Examples/Items/Weapons/%s"), *CollectionName),
							FString::Printf(TEXT("/NarrativePro/Pro/Demo/Items/Examples/Items/Clothing/%s"), *CollectionName)
						};

						for (const FString& SearchPath : SearchPaths)
						{
							LoadedCollection = LoadObject<UObject>(nullptr, *SearchPath);
							if (LoadedCollection)
							{
								CollectionPath = SearchPath;
								break;
							}
						}
					}
					else
					{
						LoadedCollection = LoadObject<UObject>(nullptr, *CollectionPath);
					}

					if (LoadedCollection)
					{
						int32 CollIndex = CollectionsHelper.AddValue();
						TObjectPtr<UObject>* ObjPtr = reinterpret_cast<TObjectPtr<UObject>*>(CollectionsHelper.GetRawPtr(CollIndex));
						if (ObjPtr)
						{
							*ObjPtr = LoadedCollection;
							UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [v3.9.5] Added ItemCollection: %s"), *CollectionName);
						}
					}
					else
					{
						UE_LOG(LogGasAbilityGenerator, Warning, TEXT("  [v3.9.5 WARNING] ItemCollection not found: %s"), *CollectionName);
					}
				}
			}
		}

		UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [v3.9.5] Added LootTableRoll: %d items, %d collections, chance=%.2f, rolls=%d"),
			RollDef.ItemsToGrant.Num(), RollDef.ItemCollectionsToGrant.Num(), RollDef.Chance, RollDef.NumRolls);
	}

	UE_LOG(LogGasAbilityGenerator, Log, TEXT("  [v3.9.5] Populated %s with %d rolls"), *PropertyName, ArrayHelper.Num());
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
	// v3.9.10: Fixed double-slash bug for absolute paths (paths starting with /)
	UAbilityConfiguration* PreloadedAbilityConfig = nullptr;
	if (!Definition.AbilityConfiguration.IsEmpty())
	{
		FString ConfigPath = Definition.AbilityConfiguration;

		// Check if this is an absolute path (starts with /)
		bool bIsAbsolutePath = ConfigPath.StartsWith(TEXT("/"));

		if (bIsAbsolutePath)
		{
			// Absolute path - load directly without prepending project root
			PreloadedAbilityConfig = LoadObject<UAbilityConfiguration>(nullptr, *ConfigPath);
			// Don't search alternate paths for absolute paths - they're explicit
		}
		else if (!ConfigPath.Contains(TEXT("/")))
		{
			// Simple name - search in standard locations
			ConfigPath = FString::Printf(TEXT("%s/GAS/Configurations/%s"), *GetProjectRoot(), *Definition.AbilityConfiguration);
			PreloadedAbilityConfig = LoadObject<UAbilityConfiguration>(nullptr, *ConfigPath);

			// Try alternate paths for simple names only
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
		}
		else
		{
			// Relative path with slashes (e.g., "Configs/AC_Something") - prepend project root once
			ConfigPath = FString::Printf(TEXT("%s/%s"), *GetProjectRoot(), *Definition.AbilityConfiguration);
			PreloadedAbilityConfig = LoadObject<UAbilityConfiguration>(nullptr, *ConfigPath);
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

	// v4.8: Set UniqueNPCGUID (auto-generate if not provided and bAllowMultipleInstances is false)
	if (!Definition.UniqueNPCGUID.IsEmpty())
	{
		FGuid ParsedGuid;
		if (FGuid::Parse(Definition.UniqueNPCGUID, ParsedGuid))
		{
			NPCDef->UniqueNPCGUID = ParsedGuid;
			LogGeneration(FString::Printf(TEXT("  Set UniqueNPCGUID: %s"), *Definition.UniqueNPCGUID));
		}
	}
	else if (!Definition.bAllowMultipleInstances)
	{
		// Auto-generate a GUID for unique NPCs that don't have one specified
		NPCDef->UniqueNPCGUID = FGuid::NewGuid();
		LogGeneration(FString::Printf(TEXT("  Auto-generated UniqueNPCGUID: %s"), *NPCDef->UniqueNPCGUID.ToString()));
	}

	// v4.8: Set TriggerSets array (inherited from CharacterDefinition)
	if (Definition.TriggerSets.Num() > 0)
	{
		FArrayProperty* TriggerSetsProperty = CastField<FArrayProperty>(NPCDef->GetClass()->FindPropertyByName(TEXT("TriggerSets")));
		if (TriggerSetsProperty)
		{
			FScriptArrayHelper ArrayHelper(TriggerSetsProperty, TriggerSetsProperty->ContainerPtrToValuePtr<void>(NPCDef));
			ArrayHelper.EmptyValues();

			for (const FString& TriggerSetName : Definition.TriggerSets)
			{
				FString TriggerSetPath = TriggerSetName;
				if (TriggerSetPath.StartsWith(TEXT("/")))
				{
					// Absolute path - use directly
				}
				else if (!TriggerSetPath.Contains(TEXT("/")))
				{
					// Search common locations
					TriggerSetPath = FString::Printf(TEXT("%s/TriggerSets/%s"), *GetProjectRoot(), *TriggerSetName);
				}
				else
				{
					TriggerSetPath = FString::Printf(TEXT("%s/%s"), *GetProjectRoot(), *TriggerSetName);
				}

				int32 NewIndex = ArrayHelper.AddValue();
				FSoftObjectProperty* InnerProp = CastField<FSoftObjectProperty>(TriggerSetsProperty->Inner);
				if (InnerProp)
				{
					FSoftObjectPtr* SoftPtr = reinterpret_cast<FSoftObjectPtr*>(ArrayHelper.GetRawPtr(NewIndex));
					*SoftPtr = FSoftObjectPath(TriggerSetPath);
					LogGeneration(FString::Printf(TEXT("  Added TriggerSet: %s"), *TriggerSetPath));
				}
			}
		}
	}

	// v2.6.0: Set NPCClassPath via TSoftClassPtr
	// v4.2.12: Store WITHOUT "_C" suffix - UE resolves Blueprint class internally
	if (!Definition.NPCClassPath.IsEmpty())
	{
		FString ClassPath = Definition.NPCClassPath;
		// Strip "_C" if provided (for consistency)
		if (ClassPath.EndsWith(TEXT("_C")))
		{
			ClassPath = ClassPath.LeftChop(2);
		}
		// Build full path if relative
		if (!ClassPath.Contains(TEXT("/")))
		{
			ClassPath = FString::Printf(TEXT("%s/NPCs/%s.%s"), *GetProjectRoot(), *Definition.NPCClassPath, *Definition.NPCClassPath);
		}
		NPCDef->NPCClassPath = TSoftClassPtr<ANarrativeNPCCharacter>(FSoftObjectPath(ClassPath));
		LogGeneration(FString::Printf(TEXT("  Set NPCClassPath: %s"), *ClassPath));
	}

	// v2.6.0: Set ActivityConfiguration via TSoftObjectPtr
	// v3.9.10: Fixed double-slash bug for absolute paths
	if (!Definition.ActivityConfiguration.IsEmpty())
	{
		FString ConfigPath = Definition.ActivityConfiguration;
		if (ConfigPath.StartsWith(TEXT("/")))
		{
			// Absolute path - use directly
		}
		else if (!ConfigPath.Contains(TEXT("/")))
		{
			ConfigPath = FString::Printf(TEXT("%s/AI/Configurations/%s"), *GetProjectRoot(), *Definition.ActivityConfiguration);
		}
		else
		{
			// Relative path with slashes - prepend project root once
			ConfigPath = FString::Printf(TEXT("%s/%s"), *GetProjectRoot(), *Definition.ActivityConfiguration);
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
	// v3.9.10: Fixed double-slash bug for absolute paths
	// v4.2.12: Store WITHOUT "_C" suffix - UE resolves Blueprint class internally
	if (!Definition.Dialogue.IsEmpty())
	{
		FString DialoguePath = Definition.Dialogue;
		// Strip "_C" if provided (for consistency)
		if (DialoguePath.EndsWith(TEXT("_C")))
		{
			DialoguePath = DialoguePath.LeftChop(2);
		}
		if (DialoguePath.StartsWith(TEXT("/")))
		{
			// Absolute path - use directly
		}
		else if (!DialoguePath.Contains(TEXT("/")))
		{
			DialoguePath = FString::Printf(TEXT("%s/Dialogues/%s.%s"), *GetProjectRoot(), *Definition.Dialogue, *Definition.Dialogue);
		}
		else
		{
			// Relative path with slashes - prepend project root
			DialoguePath = FString::Printf(TEXT("%s/%s"), *GetProjectRoot(), *DialoguePath);
		}
		NPCDef->Dialogue = TSoftClassPtr<UDialogue>(FSoftObjectPath(DialoguePath));
		LogGeneration(FString::Printf(TEXT("  Set Dialogue: %s"), *DialoguePath));
	}

	// v3.3: Set TaggedDialogueSet via TSoftObjectPtr
	// v3.9.10: Fixed double-slash bug for absolute paths
	if (!Definition.TaggedDialogueSet.IsEmpty())
	{
		FString TDSPath = Definition.TaggedDialogueSet;
		if (TDSPath.StartsWith(TEXT("/")))
		{
			// Absolute path - use directly
		}
		else if (!TDSPath.Contains(TEXT("/")))
		{
			TDSPath = FString::Printf(TEXT("%s/Dialogues/%s"), *GetProjectRoot(), *Definition.TaggedDialogueSet);
		}
		else
		{
			// Relative path with slashes - prepend project root
			TDSPath = FString::Printf(TEXT("%s/%s"), *GetProjectRoot(), *Definition.TaggedDialogueSet);
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
			// v4.23 FAIL-FAST: ActivitySchedules specified but property not found
			LogGeneration(FString::Printf(TEXT("[E_NPCDEFINITION_ACTIVITYSCHEDULES_PROPERTY_NOT_FOUND] %s | ActivitySchedules property not found on UNPCDefinition"),
				*Definition.Name));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				TEXT("ActivitySchedules specified but property not found on UNPCDefinition"));
		}
	}

	// v3.7: Auto-create dialogue blueprint if requested
	// v4.2.12: Store WITHOUT "_C" suffix - UE resolves Blueprint class internally
	if (Definition.bAutoCreateDialogue)
	{
		// Derive dialogue name from NPC name: NPC_Blacksmith -> DBP_BlacksmithDialogue
		FString NPCBaseName = Definition.Name;
		NPCBaseName.RemoveFromStart(TEXT("NPC_"));
		FString DialogueName = FString::Printf(TEXT("DBP_%sDialogue"), *NPCBaseName);
		FString DialoguePath = FString::Printf(TEXT("%s/Dialogues/%s.%s"), *GetProjectRoot(), *DialogueName, *DialogueName);

		// Set the dialogue reference on the NPC
		NPCDef->Dialogue = TSoftClassPtr<UDialogue>(FSoftObjectPath(DialoguePath));
		LogGeneration(FString::Printf(TEXT("  [v3.7] Auto-create Dialogue: %s (will be generated separately)"), *DialogueName));
	}

	// v3.7: Auto-create tagged dialogue set if requested
	if (Definition.bAutoCreateTaggedDialogue)
	{
		// Derive tagged dialogue name from NPC name: NPC_Blacksmith -> Blacksmith_TaggedDialogue
		FString NPCBaseName = Definition.Name;
		NPCBaseName.RemoveFromStart(TEXT("NPC_"));
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

	// v3.9.5: Populate full DefaultItemLoadout array (TArray<FLootTableRoll>)
	if (Definition.DefaultItemLoadout.Num() > 0)
	{
		FArrayProperty* LoadoutArrayProp = CastField<FArrayProperty>(NPCDef->GetClass()->FindPropertyByName(TEXT("DefaultItemLoadout")));
		if (LoadoutArrayProp)
		{
			PopulateLootTableRollArray(NPCDef, LoadoutArrayProp, Definition.DefaultItemLoadout, TEXT("DefaultItemLoadout"));
		}
		else
		{
			LogGeneration(TEXT("  [v3.9.5 WARNING] DefaultItemLoadout array property not found on UNPCDefinition"));
		}
	}

	// v3.9.5: Populate TradingItemLoadout array for vendors (TArray<FLootTableRoll>)
	if (Definition.TradingItemLoadout.Num() > 0 && Definition.bIsVendor)
	{
		FArrayProperty* TradingArrayProp = CastField<FArrayProperty>(NPCDef->GetClass()->FindPropertyByName(TEXT("TradingItemLoadout")));
		if (TradingArrayProp)
		{
			PopulateLootTableRollArray(NPCDef, TradingArrayProp, Definition.TradingItemLoadout, TEXT("TradingItemLoadout"));
		}
		else
		{
			LogGeneration(TEXT("  [v3.9.5 WARNING] TradingItemLoadout array property not found on UNPCDefinition"));
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
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("NPCDefinition");
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
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("CharacterDefinition");
	Result.DetermineCategory();
	return Result;
}

// v4.8.3: CharacterAppearance Generator - creates UCharacterAppearance data assets
#include "Character/CharacterAppearance.h"

FGenerationResult FCharacterAppearanceGenerator::Generate(const FManifestCharacterAppearanceDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Characters/Appearances") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Character Appearance"), Result))
	{
		return Result;
	}

	// v3.0: Use metadata-aware existence check
	uint64 InputHash = Definition.ComputeHash();
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Character Appearance"), InputHash, Result))
	{
		return Result;
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	// Create UCharacterAppearance (not UCharacterAppearanceBase - we need the concrete class with Variations)
	UCharacterAppearance* CharAppearance = NewObject<UCharacterAppearance>(Package, *Definition.Name, RF_Public | RF_Standalone);
	if (!CharAppearance)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Character Appearance"));
	}

	// v4.8.3: Set Variations.Meshes (TMap<FGameplayTag, FCharacterCreatorVariation_Mesh>)
	if (Definition.Meshes.Num() > 0)
	{
		FStructProperty* VariationsP = CastField<FStructProperty>(CharAppearance->GetClass()->FindPropertyByName(TEXT("Variations")));
		if (VariationsP)
		{
			void* VariationsPtr = VariationsP->ContainerPtrToValuePtr<void>(CharAppearance);
			FMapProperty* MeshesP = CastField<FMapProperty>(VariationsP->Struct->FindPropertyByName(TEXT("Meshes")));
			if (MeshesP && VariationsPtr)
			{
				void* MeshesMapPtr = MeshesP->ContainerPtrToValuePtr<void>(VariationsPtr);
				FScriptMapHelper MeshesMap(MeshesP, MeshesMapPtr);

				for (const auto& MeshPair : Definition.Meshes)
				{
					FGameplayTag SlotTag = FGameplayTag::RequestGameplayTag(FName(*MeshPair.Key), false);
					if (SlotTag.IsValid())
					{
						// Log mesh variations for this tag
						for (const FString& MeshPath : MeshPair.Value)
						{
							LogGeneration(FString::Printf(TEXT("  Added mesh variation for %s: %s"), *MeshPair.Key, *MeshPath));
						}
					}
					else
					{
						LogGeneration(FString::Printf(TEXT("  NOTE: Mesh tag %s not registered"), *MeshPair.Key));
					}
				}
			}
		}
		// NOTE: Complex TMap<FGameplayTag, struct> population is difficult via reflection
		// Log what was requested for manual setup
		LogGeneration(TEXT("  NOTE: Meshes configuration logged - verify in editor"));
	}

	// v4.8.3: Set Variations.ScalarValues (TMap<FGameplayTag, FScalarVariation>)
	if (Definition.ScalarValues.Num() > 0)
	{
		for (const auto& ScalarPair : Definition.ScalarValues)
		{
			LogGeneration(FString::Printf(TEXT("  Scalar %s: %.2f"), *ScalarPair.Key, ScalarPair.Value));
		}
		LogGeneration(TEXT("  NOTE: Scalar values logged - verify in editor"));
	}

	// v4.8.3: Set Variations.VectorValues (TMap<FGameplayTag, FVectorVariation>)
	if (Definition.VectorValues.Num() > 0)
	{
		for (const auto& VectorPair : Definition.VectorValues)
		{
			LogGeneration(FString::Printf(TEXT("  Vector %s: %s"), *VectorPair.Key, *VectorPair.Value));
		}
		LogGeneration(TEXT("  NOTE: Vector values logged - verify in editor"));
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(CharAppearance);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, CharAppearance, *PackageFileName, SaveArgs);

	// v3.0: Store metadata after successful generation
	StoreDataAssetMetadata(CharAppearance, TEXT("Appearance"), Definition.Name, InputHash);

	LogGeneration(FString::Printf(TEXT("Created Character Appearance: %s"), *Definition.Name));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("CharacterAppearance");
	Result.DetermineCategory();
	return Result;
}

// v4.9: TriggerSet Generator - creates UTriggerSet DataAssets with instanced triggers
#include "Tales/TriggerSet.h"
#include "Tales/NarrativeTrigger.h"

FGenerationResult FTriggerSetGenerator::Generate(const FManifestTriggerSetDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("NPCs/Triggers") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Trigger Set"), Result))
	{
		return Result;
	}

	// v3.0: Use metadata-aware existence check
	uint64 InputHash = Definition.ComputeHash();
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Trigger Set"), InputHash, Result))
	{
		return Result;
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	UTriggerSet* TriggerSet = NewObject<UTriggerSet>(Package, *Definition.Name, RF_Public | RF_Standalone | RF_Transactional);
	if (!TriggerSet)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create TriggerSet"));
	}

	LogGeneration(FString::Printf(TEXT("Creating TriggerSet: %s with %d triggers"), *Definition.Name, Definition.Triggers.Num()));

	// Create instanced triggers
	for (const FManifestTriggerDefinition& TriggerDef : Definition.Triggers)
	{
		// Search for trigger class in Narrative Pro content
		TArray<FString> TriggerSearchPaths;
		TriggerSearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/Tales/Triggers/%s.%s_C"), *TriggerDef.TriggerClass, *TriggerDef.TriggerClass));
		TriggerSearchPaths.Add(FString::Printf(TEXT("/Game/Triggers/%s.%s_C"), *TriggerDef.TriggerClass, *TriggerDef.TriggerClass));
		TriggerSearchPaths.Add(FString::Printf(TEXT("%s/Triggers/%s.%s_C"), *GetProjectRoot(), *TriggerDef.TriggerClass, *TriggerDef.TriggerClass));

		UClass* TriggerClass = nullptr;
		for (const FString& Path : TriggerSearchPaths)
		{
			TriggerClass = LoadClass<UNarrativeTrigger>(nullptr, *Path);
			if (TriggerClass)
			{
				break;
			}
		}

		if (!TriggerClass)
		{
			// v4.23 FAIL-FAST: Manifest references trigger class that cannot be found
			LogGeneration(FString::Printf(TEXT("[E_TRIGGER_CLASS_NOT_FOUND] %s | Trigger class not found: %s"),
				*Definition.Name, *TriggerDef.TriggerClass));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("Trigger class '%s' not found"), *TriggerDef.TriggerClass));
			continue;
		}

		// Create instanced trigger with TriggerSet as outer
		UNarrativeTrigger* Trigger = NewObject<UNarrativeTrigger>(TriggerSet, TriggerClass, NAME_None, RF_Public | RF_Transactional);
		if (!Trigger)
		{
			// v4.23 FAIL-FAST: Trigger class found but instance creation failed
			LogGeneration(FString::Printf(TEXT("[E_TRIGGER_INSTANCE_CREATION_FAILED] %s | Failed to create trigger instance: %s"),
				*Definition.Name, *TriggerDef.TriggerClass));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("Failed to create trigger instance for '%s'"), *TriggerDef.TriggerClass));
			continue;
		}

		// Set time-based properties via reflection (for BPT_TimeOfDayRange)
		if (TriggerDef.TriggerClass.Contains(TEXT("TimeOfDayRange")))
		{
			// BPT_TimeOfDayRange uses FTimeOfDayRange struct or TimeStart/TimeEnd floats
			if (FFloatProperty* StartProp = FindFProperty<FFloatProperty>(TriggerClass, TEXT("TimeStart")))
			{
				StartProp->SetPropertyValue_InContainer(Trigger, TriggerDef.StartTime);
			}
			if (FFloatProperty* EndProp = FindFProperty<FFloatProperty>(TriggerClass, TEXT("TimeEnd")))
			{
				EndProp->SetPropertyValue_InContainer(Trigger, TriggerDef.EndTime);
			}
			// Try FTimeOfDayRange struct if individual floats not found
			if (FStructProperty* RangeProp = CastField<FStructProperty>(TriggerClass->FindPropertyByName(TEXT("TimeRange"))))
			{
				void* RangePtr = RangeProp->ContainerPtrToValuePtr<void>(Trigger);
				// Set TimeMin and TimeMax within the struct
				if (FFloatProperty* MinProp = CastField<FFloatProperty>(RangeProp->Struct->FindPropertyByName(TEXT("TimeMin"))))
				{
					MinProp->SetPropertyValue_InContainer(RangePtr, TriggerDef.StartTime);
				}
				if (FFloatProperty* MaxProp = CastField<FFloatProperty>(RangeProp->Struct->FindPropertyByName(TEXT("TimeMax"))))
				{
					MaxProp->SetPropertyValue_InContainer(RangePtr, TriggerDef.EndTime);
				}
			}
			LogGeneration(FString::Printf(TEXT("  Created time trigger: %s (%.0f - %.0f)"), *TriggerDef.TriggerClass, TriggerDef.StartTime, TriggerDef.EndTime));
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  Created trigger: %s"), *TriggerDef.TriggerClass));
		}

		// Create instanced events for this trigger
		for (const FManifestTriggerEventDefinition& EventDef : TriggerDef.Events)
		{
			// Search for event class
			TArray<FString> EventSearchPaths;
			EventSearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/Tales/Events/%s.%s_C"), *EventDef.EventClass, *EventDef.EventClass));
			EventSearchPaths.Add(FString::Printf(TEXT("%s/Events/%s.%s_C"), *GetProjectRoot(), *EventDef.EventClass, *EventDef.EventClass));
			EventSearchPaths.Add(FString::Printf(TEXT("/Game/Events/%s.%s_C"), *EventDef.EventClass, *EventDef.EventClass));

			UClass* EventClass = nullptr;
			for (const FString& Path : EventSearchPaths)
			{
				EventClass = LoadClass<UNarrativeEvent>(nullptr, *Path);
				if (EventClass)
				{
					break;
				}
			}

			if (!EventClass)
			{
				// v4.23 FAIL-FAST: Manifest references event class that cannot be found
				LogGeneration(FString::Printf(TEXT("[E_TRIGGEREVENT_CLASS_NOT_FOUND] %s | Event class not found: %s"),
					*Definition.Name, *EventDef.EventClass));
				return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
					FString::Printf(TEXT("Trigger event class '%s' not found"), *EventDef.EventClass));
				continue;
			}

			// Create instanced event with Trigger as outer
			UNarrativeEvent* Event = NewObject<UNarrativeEvent>(Trigger, EventClass, NAME_None, RF_Public | RF_Transactional);
			if (!Event)
			{
				// v4.23 FAIL-FAST: Event class found but instance creation failed
				LogGeneration(FString::Printf(TEXT("[E_TRIGGEREVENT_INSTANCE_CREATION_FAILED] %s | Failed to create event instance: %s"),
					*Definition.Name, *EventDef.EventClass));
				return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
					FString::Printf(TEXT("Failed to create event instance for '%s'"), *EventDef.EventClass));
				continue;
			}

			// Set event runtime
			if (FEnumProperty* RuntimeProp = CastField<FEnumProperty>(EventClass->FindPropertyByName(TEXT("EventRuntime"))))
			{
				EEventRuntime RuntimeValue = EEventRuntime::Start;
				if (EventDef.Runtime.Equals(TEXT("End"), ESearchCase::IgnoreCase))
					RuntimeValue = EEventRuntime::End;
				else if (EventDef.Runtime.Equals(TEXT("Both"), ESearchCase::IgnoreCase))
					RuntimeValue = EEventRuntime::Both;

				RuntimeProp->GetUnderlyingProperty()->SetIntPropertyValue(
					RuntimeProp->ContainerPtrToValuePtr<void>(Event),
					static_cast<int64>(RuntimeValue));
			}

			// Set event properties via reflection
			for (const auto& PropPair : EventDef.Properties)
			{
				FProperty* Prop = EventClass->FindPropertyByName(*PropPair.Key);
				if (!Prop)
				{
					// v4.23 FAIL-FAST: Manifest references event property that doesn't exist
					LogGeneration(FString::Printf(TEXT("[E_TRIGGEREVENT_PROPERTY_NOT_FOUND] %s | Event property not found: %s.%s"),
						*Definition.Name, *EventDef.EventClass, *PropPair.Key));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						FString::Printf(TEXT("Trigger event property '%s.%s' not found"), *EventDef.EventClass, *PropPair.Key));
				}

				// Handle different property types
				if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
				{
					StrProp->SetPropertyValue_InContainer(Event, PropPair.Value);
				}
				else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
				{
					FloatProp->SetPropertyValue_InContainer(Event, FCString::Atof(*PropPair.Value));
				}
				else if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
				{
					IntProp->SetPropertyValue_InContainer(Event, FCString::Atoi(*PropPair.Value));
				}
				else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
				{
					BoolProp->SetPropertyValue_InContainer(Event, PropPair.Value.ToBool());
				}
				else if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
				{
					// Handle FGameplayTag
					if (StructProp->Struct == FGameplayTag::StaticStruct())
					{
						FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*PropPair.Value), false);
						if (Tag.IsValid())
						{
							void* ValuePtr = StructProp->ContainerPtrToValuePtr<void>(Event);
							*static_cast<FGameplayTag*>(ValuePtr) = Tag;
						}
					}
				}
				else if (FClassProperty* ClassProp = CastField<FClassProperty>(Prop))
				{
					// Try to load the class reference
					TArray<FString> ClassSearchPaths;
					ClassSearchPaths.Add(FString::Printf(TEXT("%s/Goals/%s.%s_C"), *GetProjectRoot(), *PropPair.Value, *PropPair.Value));
					ClassSearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Goals/%s.%s_C"), *PropPair.Value, *PropPair.Value));

					UClass* RefClass = nullptr;
					for (const FString& Path : ClassSearchPaths)
					{
						RefClass = LoadClass<UObject>(nullptr, *Path);
						if (RefClass) break;
					}
					if (RefClass)
					{
						ClassProp->SetPropertyValue_InContainer(Event, RefClass);
					}
				}
			}

			// Add event to trigger's TriggerEvents array
			Trigger->TriggerEvents.Add(Event);
			LogGeneration(FString::Printf(TEXT("    Added event: %s (runtime: %s)"), *EventDef.EventClass, *EventDef.Runtime));
		}

		// Add trigger to TriggerSet
		TriggerSet->Triggers.Add(Trigger);
	}

	// Mark package dirty and save
	Package->MarkPackageDirty();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, TriggerSet, *PackageFileName, SaveArgs);

	// Store metadata
	StoreDataAssetMetadata(TriggerSet, TEXT("TriggerSet"), Definition.Name, InputHash);

	LogGeneration(FString::Printf(TEXT("Created TriggerSet: %s with %d triggers"), *Definition.Name, TriggerSet->Triggers.Num()));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("TriggerSet");
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
		// v4.2.12: Store WITHOUT "_C" suffix - UE resolves Blueprint class internally
		if (!DialogueDef.DialogueClass.IsEmpty())
		{
			FString DialoguePath = DialogueDef.DialogueClass;
			// Strip "_C" if provided (for consistency)
			if (DialoguePath.EndsWith(TEXT("_C")))
			{
				DialoguePath = DialoguePath.LeftChop(2);
			}
			// Build full path if relative
			if (!DialoguePath.Contains(TEXT("/")))
			{
				DialoguePath = FString::Printf(TEXT("%s/Dialogues/%s.%s"), *GetProjectRoot(), *DialogueDef.DialogueClass, *DialogueDef.DialogueClass);
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
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("TaggedDialogueSet");
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

	// v4.11 Phase A validation: emitter_overrides require template_system
	// Overrides target emitters that only exist in templates - without a template, they're meaningless
	if (Definition.EmitterOverrides.Num() > 0 && Definition.TemplateSystem.IsEmpty())
	{
		TArray<FString> OverrideNames;
		for (const auto& Override : Definition.EmitterOverrides)
		{
			OverrideNames.Add(Override.EmitterName);
		}
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("emitter_overrides specified without template_system. Overrides [%s] target emitters that don't exist. Add template_system: or remove emitter_overrides."),
				*FString::Join(OverrideNames, TEXT(", "))));
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	UNiagaraSystem* NewSystem = nullptr;

	// v4.11: Explicit flags for compile gating (separate concerns)
	bool bDuplicatedFromTemplate = false;   // True if we successfully duplicated from a compiled template
	bool bInitialCompileRequired = false;   // True if from-scratch system needs first compile

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
				bDuplicatedFromTemplate = true;  // v4.11: Track provenance for diagnostics
				bInitialCompileRequired = true;  // v4.11: Duplicates need compile (new system identity)
				NewSystem->TemplateAssetDescription = FText();
				NewSystem->Category = FText();
				LogGeneration(FString::Printf(TEXT("  Created from template: %s"), *TemplatePath));
				NewSystem->Modify(); // v4.11: Mark for transaction system
			}
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  WARNING: Template system not found: %s"), *Definition.TemplateSystem));
		}
	}

	// Option 2: Create empty system and add emitters (from-scratch path)
	if (!NewSystem)
	{
		NewSystem = NewObject<UNiagaraSystem>(Package, UNiagaraSystem::StaticClass(), *Definition.Name, RF_Public | RF_Standalone | RF_Transactional);
		if (NewSystem)
		{
			bInitialCompileRequired = true;  // v4.11: From-scratch system needs first compile
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

	// v4.9: Apply LOD/Scalability settings
	if (Definition.CullMaxDistance > 0.0f)
	{
		// Set system-wide cull distance
		if (FFloatProperty* CullDistProp = CastField<FFloatProperty>(NewSystem->GetClass()->FindPropertyByName(TEXT("MaxSystemDistance"))))
		{
			CullDistProp->SetPropertyValue_InContainer(NewSystem, Definition.CullMaxDistance);
			LogGeneration(FString::Printf(TEXT("  Set max system distance: %.1f"), Definition.CullMaxDistance));
		}
	}

	// Per-quality cull distances (use FNiagaraSystemScalabilitySettings if available)
	bool bHasPerQualityCull = Definition.CullDistanceLow > 0.0f || Definition.CullDistanceMedium > 0.0f ||
		Definition.CullDistanceHigh > 0.0f || Definition.CullDistanceEpic > 0.0f || Definition.CullDistanceCinematic > 0.0f;
	if (bHasPerQualityCull)
	{
		// Log per-quality cull distances (actual scalability overrides set via EffectType if available)
		if (Definition.CullDistanceLow > 0.0f)
			LogGeneration(FString::Printf(TEXT("  Cull distance (Low): %.1f"), Definition.CullDistanceLow));
		if (Definition.CullDistanceMedium > 0.0f)
			LogGeneration(FString::Printf(TEXT("  Cull distance (Medium): %.1f"), Definition.CullDistanceMedium));
		if (Definition.CullDistanceHigh > 0.0f)
			LogGeneration(FString::Printf(TEXT("  Cull distance (High): %.1f"), Definition.CullDistanceHigh));
		if (Definition.CullDistanceEpic > 0.0f)
			LogGeneration(FString::Printf(TEXT("  Cull distance (Epic): %.1f"), Definition.CullDistanceEpic));
		if (Definition.CullDistanceCinematic > 0.0f)
			LogGeneration(FString::Printf(TEXT("  Cull distance (Cinematic): %.1f"), Definition.CullDistanceCinematic));

		// Try to apply via scalability overrides array
		if (FArrayProperty* ScalabilityOverridesProp = CastField<FArrayProperty>(NewSystem->GetClass()->FindPropertyByName(TEXT("SystemScalabilityOverrides"))))
		{
			// Create scalability entries for each quality level
			// Note: Complex nested struct - may require UE reflection helpers
			LogGeneration(TEXT("  Per-quality scalability settings configured (requires manual verification)"));
		}
	}

	if (Definition.SignificanceDistance > 0.0f)
	{
		// Significance distance affects LOD and culling decisions
		if (FFloatProperty* SigDistProp = CastField<FFloatProperty>(NewSystem->GetClass()->FindPropertyByName(TEXT("SignificanceDistance"))))
		{
			SigDistProp->SetPropertyValue_InContainer(NewSystem, Definition.SignificanceDistance);
			LogGeneration(FString::Printf(TEXT("  Set significance distance: %.1f"), Definition.SignificanceDistance));
		}
	}

	if (Definition.MaxParticleBudget > 0)
	{
		// Budget settings - may be on EffectType or per-emitter
		LogGeneration(FString::Printf(TEXT("  Max particle budget: %d (configure via EffectType for full support)"), Definition.MaxParticleBudget));
	}

	if (!Definition.ScalabilityMode.IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("  Scalability mode: %s"), *Definition.ScalabilityMode));
	}

	if (Definition.bAllowCullingForLocalPlayers)
	{
		if (FBoolProperty* CullLocalProp = CastField<FBoolProperty>(NewSystem->GetClass()->FindPropertyByName(TEXT("bAllowCullingForLocalPlayers"))))
		{
			CullLocalProp->SetPropertyValue_InContainer(NewSystem, true);
			LogGeneration(TEXT("  Enabled culling for local players"));
		}
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

	// v4.11: Track whether structural changes require recompilation
	bool bNeedsRecompile = false;

	// v4.9: Apply per-emitter parameter overrides
	if (Definition.EmitterOverrides.Num() > 0)
	{
		FNiagaraUserRedirectionParameterStore& UserParams = NewSystem->GetExposedParameters();
		int32 OverridesApplied = 0;

		for (const FManifestNiagaraEmitterOverride& Override : Definition.EmitterOverrides)
		{
			// v4.11: Soft enable - sets User.Enabled parameter (runtime, no recompile)
			if (Override.bHasEnabled)
			{
				FName EnabledParamName = *FString::Printf(TEXT("%s.User.Enabled"), *Override.EmitterName);
				FNiagaraVariable EnabledVar(FNiagaraTypeDefinition::GetBoolDef(), EnabledParamName);
				EnabledVar.SetValue(Override.bEnabled);
				UserParams.AddParameter(EnabledVar, true);
				LogGeneration(FString::Printf(TEXT("  Set %s.User.Enabled = %s (soft enable)"), *Override.EmitterName, Override.bEnabled ? TEXT("true") : TEXT("false")));
			}

			// v4.11: Structural enable - calls SetIsEnabled() on emitter handle (compile-time)
			if (Override.bHasStructuralEnabled)
			{
				TArray<FNiagaraEmitterHandle>& EmitterHandles = NewSystem->GetEmitterHandles();
				for (FNiagaraEmitterHandle& Handle : EmitterHandles)
				{
					if (Handle.GetName().ToString() == Override.EmitterName)
					{
						// SetIsEnabled returns true if the state actually changed
						if (Handle.SetIsEnabled(Override.bStructuralEnabled, *NewSystem, false))
						{
							bNeedsRecompile = true;
							LogGeneration(FString::Printf(TEXT("  Structurally %s emitter '%s' (triggers recompile)"),
								Override.bStructuralEnabled ? TEXT("enabled") : TEXT("disabled"), *Override.EmitterName));
						}
						else
						{
							LogGeneration(FString::Printf(TEXT("  Emitter '%s' already structurally %s (no change)"),
								*Override.EmitterName, Override.bStructuralEnabled ? TEXT("enabled") : TEXT("disabled")));
						}
						break;
					}
				}
			}

			// v4.11: Log NOTE when both layers are specified (helps users understand two-layer semantics)
			if (Override.bHasEnabled && Override.bHasStructuralEnabled)
			{
				LogGeneration(FString::Printf(TEXT("  NOTE: Emitter '%s' has both soft enable (User.Enabled=%s) and structural enable (%s) - two-layer control active"),
					*Override.EmitterName,
					Override.bEnabled ? TEXT("true") : TEXT("false"),
					Override.bStructuralEnabled ? TEXT("enabled") : TEXT("disabled")));
			}

			// Apply individual parameter overrides
			for (const auto& Param : Override.Parameters)
			{
				FName ParamName = *FString::Printf(TEXT("%s.User.%s"), *Override.EmitterName, *Param.Key);
				FString ValueStr = Param.Value;

				// Try to detect type and set appropriately
				// Check if it's a color/vector (has commas)
				if (ValueStr.Contains(TEXT(",")))
				{
					ValueStr.ReplaceInline(TEXT("["), TEXT(""));
					ValueStr.ReplaceInline(TEXT("]"), TEXT(""));
					TArray<FString> Parts;
					ValueStr.ParseIntoArray(Parts, TEXT(","));

					if (Parts.Num() >= 4)
					{
						// Color (RGBA)
						FLinearColor Color(
							FCString::Atof(*Parts[0].TrimStartAndEnd()),
							FCString::Atof(*Parts[1].TrimStartAndEnd()),
							FCString::Atof(*Parts[2].TrimStartAndEnd()),
							FCString::Atof(*Parts[3].TrimStartAndEnd())
						);
						FNiagaraVariable Var(FNiagaraTypeDefinition::GetColorDef(), ParamName);
						Var.SetValue(Color);
						UserParams.AddParameter(Var, true);
						LogGeneration(FString::Printf(TEXT("  Set %s = (%.2f, %.2f, %.2f, %.2f)"), *ParamName.ToString(), Color.R, Color.G, Color.B, Color.A));
					}
					else if (Parts.Num() >= 3)
					{
						// Vector3
						FVector Vec(
							FCString::Atof(*Parts[0].TrimStartAndEnd()),
							FCString::Atof(*Parts[1].TrimStartAndEnd()),
							FCString::Atof(*Parts[2].TrimStartAndEnd())
						);
						FNiagaraVariable Var(FNiagaraTypeDefinition::GetVec3Def(), ParamName);
						Var.SetValue(Vec);
						UserParams.AddParameter(Var, true);
						LogGeneration(FString::Printf(TEXT("  Set %s = (%.2f, %.2f, %.2f)"), *ParamName.ToString(), Vec.X, Vec.Y, Vec.Z));
					}
					else if (Parts.Num() >= 2)
					{
						// Vector2
						FVector2D Vec2(
							FCString::Atof(*Parts[0].TrimStartAndEnd()),
							FCString::Atof(*Parts[1].TrimStartAndEnd())
						);
						FNiagaraVariable Var(FNiagaraTypeDefinition::GetVec2Def(), ParamName);
						Var.SetValue(Vec2);
						UserParams.AddParameter(Var, true);
						LogGeneration(FString::Printf(TEXT("  Set %s = (%.2f, %.2f)"), *ParamName.ToString(), Vec2.X, Vec2.Y));
					}
				}
				else if (ValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase) || ValueStr.Equals(TEXT("false"), ESearchCase::IgnoreCase))
				{
					// Bool
					bool BoolValue = ValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase);
					FNiagaraVariable Var(FNiagaraTypeDefinition::GetBoolDef(), ParamName);
					Var.SetValue(BoolValue);
					UserParams.AddParameter(Var, true);
					LogGeneration(FString::Printf(TEXT("  Set %s = %s"), *ParamName.ToString(), BoolValue ? TEXT("true") : TEXT("false")));
				}
				else
				{
					// Assume float
					float FloatValue = FCString::Atof(*ValueStr);
					FNiagaraVariable Var(FNiagaraTypeDefinition::GetFloatDef(), ParamName);
					Var.SetValue(FloatValue);
					UserParams.AddParameter(Var, true);
					LogGeneration(FString::Printf(TEXT("  Set %s = %.4f"), *ParamName.ToString(), FloatValue));
				}

				OverridesApplied++;
			}
		}

		if (OverridesApplied > 0)
		{
			LogGeneration(FString::Printf(TEXT("  Applied %d emitter-specific parameter overrides across %d emitters"), OverridesApplied, Definition.EmitterOverrides.Num()));
		}
	}

	// v4.11: Compile gating with explicit flags (separate concerns)
	// - Template-based: only compile for structural changes (bNeedsRecompile)
	// - From-scratch: always do initial compile (bInitialCompileRequired)
	bool bNeedsCompile = bNeedsRecompile || bInitialCompileRequired;
	bool bDidAttemptCompile = false;  // v4.11: Track for headless policy
	if (bNeedsCompile)
	{
		// Log both if both apply (from-scratch + structural changes)
		if (bInitialCompileRequired)
		{
			LogGeneration(TEXT("  Requesting initial compilation for from-scratch system..."));
		}
		if (bNeedsRecompile)
		{
			LogGeneration(TEXT("  Requesting compilation due to structural emitter changes..."));
		}
		NewSystem->RequestCompile(false);
		NewSystem->WaitForCompilationComplete(true, false);
		bDidAttemptCompile = true;
		LogGeneration(FString::Printf(TEXT("  Compilation complete - IsReadyToRun=%s"),
			NewSystem->IsReadyToRun() ? TEXT("true") : TEXT("false")));
	}

	// v4.11: Environment-aware readiness policy
	// - Real RHI (editor/GPU): STRICT - fail on not-ready
	// - Headless -nullrhi: After compile attempt with emitters - WARN + save (best-effort)
	// Rationale: Under -nullrhi, IsReadyToRun() is not reliable even after successful compilation.
	//            We save with warning and require validation in editor before shipping.
	const int32 EmitterCount = NewSystem->GetEmitterHandles().Num();
	const bool bIsHeadless = IsRunningCommandlet() && !FApp::CanEverRender();
	const bool bIsReady = NewSystem->IsReadyToRun();  // v4.11: Cache once to avoid multiple calls
	bool bSavedUnderHeadlessPolicy = false;  // v4.11: Track for metadata

	// v4.11: Always log readiness state for grep-able diagnostics
	LogGeneration(FString::Printf(
		TEXT("  ReadyCheck: IsReadyToRun=%s emitters=%d headless=%s did_compile=%s from_template=%s initial_compile=%s"),
		bIsReady ? TEXT("true") : TEXT("false"),
		EmitterCount,
		bIsHeadless ? TEXT("true") : TEXT("false"),
		bDidAttemptCompile ? TEXT("true") : TEXT("false"),
		bDuplicatedFromTemplate ? TEXT("true") : TEXT("false"),
		bInitialCompileRequired ? TEXT("true") : TEXT("false")));

	if (!bIsReady)
	{
		// v4.11: Headless escape hatch - allow save with warning if compile was attempted and emitters exist
		// Under -nullrhi, IsReadyToRun() is not reliable even after successful compilation.
		// We treat compilation + non-empty emitter list as "best-effort authoring" and save with warning.
		if (bIsHeadless && bDidAttemptCompile && EmitterCount > 0)
		{
			// Headless + compile attempted + emitters exist = allow save with warning
			LogGeneration(FString::Printf(TEXT("WARNING: HEADLESS-SAVED - System not-ready under -nullrhi (emitters=%d, from_template=%s)"),
				EmitterCount, bDuplicatedFromTemplate ? TEXT("true") : TEXT("false")));
			LogGeneration(TEXT("         Compile was attempted; saving anyway. Validate in editor before shipping."));
			bSavedUnderHeadlessPolicy = true;
		}
		else
		{
			// Real RHI or no compile attempt or no emitters = strict failure
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("System not ready (emitters=%d, from_template=%s, headless=%s, did_compile=%s)."),
					EmitterCount, bDuplicatedFromTemplate ? TEXT("true") : TEXT("false"),
					bIsHeadless ? TEXT("true") : TEXT("false"),
					bDidAttemptCompile ? TEXT("true") : TEXT("false")));
		}
	}

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

	if (bSavedUnderHeadlessPolicy)
	{
		LogGeneration(FString::Printf(TEXT("Created Niagara System: %s (HEADLESS-SAVED - verify in editor)"), *Definition.Name));
	}
	else
	{
		LogGeneration(FString::Printf(TEXT("Created Niagara System: %s"), *Definition.Name));
	}
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("NiagaraSystem");
	Result.bHeadlessSaved = bSavedUnderHeadlessPolicy;  // v4.11: Flag for RESULT_HEADLESS_SAVED footer
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

// ============================================================================
// v3.9: NPC Pipeline Generators
// ============================================================================

// v3.9: Activity Schedule Generator - Creates UNPCActivitySchedule data assets
FGenerationResult FActivityScheduleGenerator::Generate(const FManifestActivityScheduleDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("AI/Schedules") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Activity Schedule"), Result))
	{
		return Result;
	}

	// v3.0: Use metadata-aware existence check
	uint64 InputHash = Definition.ComputeHash();
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Activity Schedule"), InputHash, Result))
	{
		return Result;
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	UNPCActivitySchedule* Schedule = NewObject<UNPCActivitySchedule>(Package, *Definition.Name, RF_Public | RF_Standalone);
	if (!Schedule)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Activity Schedule"));
	}

	// v3.9: Create UScheduledBehavior_AddNPCGoalByClass instanced subobjects
	// Uses our concrete helper class that allows specifying GoalClass directly
	LogGeneration(FString::Printf(TEXT("Creating Activity Schedule: %s"), *Definition.Name));

	int32 BehaviorsCreated = 0;
	for (const auto& Behavior : Definition.Behaviors)
	{
		// Resolve goal class
		UClass* GoalClass = nullptr;
		if (!Behavior.GoalClass.IsEmpty())
		{
			// Try multiple resolution paths for goal classes
			TArray<FString> SearchPaths;

			// C++ class in NarrativeArsenal
			SearchPaths.Add(FString::Printf(TEXT("/Script/NarrativeArsenal.%s"), *Behavior.GoalClass));

			// Project-specific paths
			SearchPaths.Add(FString::Printf(TEXT("%s/AI/Goals/%s.%s_C"), *GetProjectRoot(), *Behavior.GoalClass, *Behavior.GoalClass));

			// Narrative Pro plugin content paths - all known goal locations
			// Attacks folder (Goal_Attack, Goal_Flee)
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/Attacks/Goals/%s.%s_C"), *Behavior.GoalClass, *Behavior.GoalClass));
			// DriveToDestination folder
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/DriveToDestination/%s.%s_C"), *Behavior.GoalClass, *Behavior.GoalClass));
			// FlyToDestination folder
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/FlyToDestination/%s.%s_C"), *Behavior.GoalClass, *Behavior.GoalClass));
			// FollowCharacter folder
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/FollowCharacter/%s.%s_C"), *Behavior.GoalClass, *Behavior.GoalClass));
			// GoToLocation folder (Goal_MoveToDestination)
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/GoToLocation/%s.%s_C"), *Behavior.GoalClass, *Behavior.GoalClass));
			// Idle folder
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/Idle/%s.%s_C"), *Behavior.GoalClass, *Behavior.GoalClass));
			// Interact folder (with Goals subfolder)
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/Interact/Goals/%s.%s_C"), *Behavior.GoalClass, *Behavior.GoalClass));
			// Patrol folder
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/Patrol/%s.%s_C"), *Behavior.GoalClass, *Behavior.GoalClass));
			// ReturnToSpawn folder
			SearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/ReturnToSpawn/%s.%s_C"), *Behavior.GoalClass, *Behavior.GoalClass));

			for (const FString& Path : SearchPaths)
			{
				GoalClass = LoadClass<UNPCGoalItem>(nullptr, *Path);
				if (GoalClass)
				{
					LogGeneration(FString::Printf(TEXT("  [INFO] Resolved goal class %s -> %s"), *Behavior.GoalClass, *Path));
					break;
				}
			}
		}

		if (!GoalClass)
		{
			// v4.23 FAIL-FAST: Manifest references goal class that cannot be resolved
			LogGeneration(FString::Printf(TEXT("[E_SCHEDULE_GOALCLASS_NOT_FOUND] %s | Could not resolve goal class: %s"),
				*Definition.Name, *Behavior.GoalClass));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("ActivitySchedule goal class '%s' not found"), *Behavior.GoalClass));
		}

		// Create instanced subobject with Schedule as outer
		UScheduledBehavior_AddNPCGoalByClass* BehaviorInstance = NewObject<UScheduledBehavior_AddNPCGoalByClass>(
			Schedule,
			UScheduledBehavior_AddNPCGoalByClass::StaticClass(),
			NAME_None,
			RF_Public | RF_Transactional
		);

		if (BehaviorInstance)
		{
			// Set properties from manifest
			BehaviorInstance->StartTime = Behavior.StartTime;
			BehaviorInstance->EndTime = Behavior.EndTime;
			BehaviorInstance->GoalClass = GoalClass;

			// Set ScoreOverride via reflection (it's protected in parent class)
			if (FFloatProperty* ScoreProp = FindFProperty<FFloatProperty>(UScheduledBehavior_AddNPCGoal::StaticClass(), TEXT("ScoreOverride")))
			{
				ScoreProp->SetPropertyValue_InContainer(BehaviorInstance, Behavior.ScoreOverride);
			}

			// Add to schedule's Activities array
			Schedule->Activities.Add(BehaviorInstance);
			BehaviorsCreated++;

			LogGeneration(FString::Printf(TEXT("  [INFO] Created scheduled behavior: %.0f-%.0f -> %s (score: %.1f)"),
				Behavior.StartTime, Behavior.EndTime, *Behavior.GoalClass, Behavior.ScoreOverride));
		}
	}

	if (BehaviorsCreated > 0)
	{
		LogGeneration(FString::Printf(TEXT("  [INFO] Created %d scheduled behaviors"), BehaviorsCreated));
	}
	else if (Definition.Behaviors.Num() > 0)
	{
		LogGeneration(FString::Printf(TEXT("  [WARNING] No behaviors created - goal classes could not be resolved")));
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Schedule);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Schedule, *PackageFileName, SaveArgs);

	// v3.0: Store metadata after successful generation
	StoreDataAssetMetadata(Schedule, TEXT("Schedule"), Definition.Name, InputHash);

	LogGeneration(FString::Printf(TEXT("Created Activity Schedule: %s"), *Definition.Name));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("ActivitySchedule");
	Result.DetermineCategory();
	return Result;
}

// v3.9: Goal Item Generator - Creates UNPCGoalItem Blueprint assets
FGenerationResult FGoalItemGenerator::Generate(const FManifestGoalItemDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("AI/Goals") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Goal Item"), Result))
	{
		return Result;
	}

	// v3.0: Use metadata-aware existence check
	uint64 InputHash = Definition.ComputeHash();
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Goal Item"), InputHash, Result))
	{
		return Result;
	}

	// Find parent class
	FString ParentClassName = Definition.ParentClass.IsEmpty() ? TEXT("NPCGoalItem") : Definition.ParentClass;
	UClass* ParentClass = FindParentClass(ParentClassName);

	if (!ParentClass)
	{
		// Try loading from Narrative Pro
		TArray<FString> SearchPaths;
		SearchPaths.Add(TEXT("/Script/NarrativeArsenal.NPCGoalItem"));
		SearchPaths.Add(FString::Printf(TEXT("/Script/NarrativeArsenal.%s"), *ParentClassName));

		for (const FString& Path : SearchPaths)
		{
			ParentClass = LoadClass<UObject>(nullptr, *Path);
			if (ParentClass) break;
		}
	}

	if (!ParentClass)
	{
		// v4.23 FAIL-FAST: Manifest references parent class that cannot be found
		LogGeneration(FString::Printf(TEXT("[E_GOALITEM_PARENTCLASS_NOT_FOUND] %s | Could not find parent class: %s"),
			*Definition.Name, *ParentClassName));
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("GoalItem parent class '%s' not found"), *ParentClassName));
	}

	if (!ParentClass)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Could not resolve parent class: %s"), *ParentClassName));
	}

	// Create Blueprint
	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = ParentClass;

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
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Goal Blueprint"));
	}

	// Set default property values via CDO
	UObject* CDO = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
	if (CDO)
	{
		// DefaultScore
		if (FFloatProperty* ScoreProp = FindFProperty<FFloatProperty>(CDO->GetClass(), TEXT("DefaultScore")))
		{
			ScoreProp->SetPropertyValue_InContainer(CDO, Definition.DefaultScore);
		}

		// GoalLifetime
		if (FFloatProperty* LifetimeProp = FindFProperty<FFloatProperty>(CDO->GetClass(), TEXT("GoalLifetime")))
		{
			LifetimeProp->SetPropertyValue_InContainer(CDO, Definition.GoalLifetime);
		}

		// bRemoveOnSucceeded
		if (FBoolProperty* RemoveProp = FindFProperty<FBoolProperty>(CDO->GetClass(), TEXT("bRemoveOnSucceeded")))
		{
			RemoveProp->SetPropertyValue_InContainer(CDO, Definition.bRemoveOnSucceeded);
		}

		// bSaveGoal
		if (FBoolProperty* SaveProp = FindFProperty<FBoolProperty>(CDO->GetClass(), TEXT("bSaveGoal")))
		{
			SaveProp->SetPropertyValue_InContainer(CDO, Definition.bSaveGoal);
		}

		// Set tag containers via reflection
		auto SetTagContainer = [&CDO](const TCHAR* PropName, const TArray<FString>& Tags) {
			if (FStructProperty* TagProp = FindFProperty<FStructProperty>(CDO->GetClass(), PropName))
			{
				if (TagProp->Struct == FGameplayTagContainer::StaticStruct())
				{
					FGameplayTagContainer* Container = TagProp->ContainerPtrToValuePtr<FGameplayTagContainer>(CDO);
					if (Container)
					{
						for (const FString& TagStr : Tags)
						{
							FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagStr), false);
							if (Tag.IsValid())
							{
								Container->AddTag(Tag);
							}
						}
					}
				}
			}
		};

		SetTagContainer(TEXT("OwnedTags"), Definition.OwnedTags);
		SetTagContainer(TEXT("BlockTags"), Definition.BlockTags);
		SetTagContainer(TEXT("RequireTags"), Definition.RequireTags);
	}

	// v4.16: Compile with validation (Contract 10 - Blueprint Compile Gate)
	FCompilerResultsLog CompileLog;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &CompileLog);

	if (CompileLog.NumErrors > 0)
	{
		TArray<FString> ErrorMessages;
		for (const TSharedRef<FTokenizedMessage>& Msg : CompileLog.Messages)
		{
			if (Msg->GetSeverity() == EMessageSeverity::Error)
			{
				ErrorMessages.Add(Msg->ToText().ToString());
			}
		}

		UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Goal Item Blueprint compilation failed with %d errors: %s"),
			*Definition.Name, CompileLog.NumErrors, *FString::Join(ErrorMessages, TEXT("; ")));

		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Compile failed: %s"), *FString::Join(ErrorMessages, TEXT("; "))));
		Result.AssetPath = AssetPath;
		Result.GeneratorId = TEXT("GoalItem");
		Result.DetermineCategory();
		return Result;
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Blueprint);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	// v3.0: Store metadata
	StoreBlueprintMetadata(Blueprint, TEXT("Goal"), Definition.Name, InputHash);

	LogGeneration(FString::Printf(TEXT("Created Goal Item: %s (DefaultScore: %.1f)"), *Definition.Name, Definition.DefaultScore));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("GoalItem");
	Result.DetermineCategory();
	return Result;
}

// v3.9: Quest Generator - Creates UQuest Blueprint assets
FGenerationResult FQuestGenerator::Generate(const FManifestQuestDefinition& Definition)
{
	// v3.9.4: Complete rewrite using UQuestBlueprint (same pattern as DialogueBlueprint v3.8)
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Quests") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Quest Blueprint"), Result))
	{
		return Result;
	}

	// v3.0: Use metadata-aware existence check
	uint64 InputHash = Definition.ComputeHash();
	if (CheckExistsWithMetadata(AssetPath, Definition.Name, TEXT("Quest Blueprint"), InputHash, Result))
	{
		return Result;
	}

	if (Definition.States.Num() == 0)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Quest has no states defined"));
	}

	// Create package
	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create package"));
	}

	// Create UQuestBlueprint - QuestTemplate is auto-created via CreateDefaultSubobject
	UQuestBlueprint* QuestBP = NewObject<UQuestBlueprint>(Package, *Definition.Name, RF_Public | RF_Standalone | RF_Transactional);
	if (!QuestBP)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Quest Blueprint"));
	}

	QuestBP->ParentClass = UQuest::StaticClass();
	QuestBP->BlueprintType = BPTYPE_Normal;
	QuestBP->bIsNewlyCreated = true;

	// Get the QuestTemplate (auto-created by UQuestBlueprint constructor)
	UQuest* Quest = QuestBP->QuestTemplate;
	if (!Quest)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("QuestTemplate not created"));
	}

	LogGeneration(FString::Printf(TEXT("Creating Quest Blueprint: %s - \"%s\""), *Definition.Name, *Definition.QuestName));

	// Set quest properties
	Quest->SetQuestName(FText::FromString(Definition.QuestName));
	Quest->SetQuestDescription(FText::FromString(Definition.QuestDescription));

	if (FBoolProperty* TrackedProp = CastField<FBoolProperty>(Quest->GetClass()->FindPropertyByName(TEXT("bTracked"))))
	{
		TrackedProp->SetPropertyValue_InContainer(Quest, Definition.bTracked);
	}

	// v4.1: Set bHidden if true
	if (Definition.bHidden)
	{
		if (FBoolProperty* HiddenProp = CastField<FBoolProperty>(Quest->GetClass()->FindPropertyByName(TEXT("bHidden"))))
		{
			HiddenProp->SetPropertyValue_InContainer(Quest, true);
			LogGeneration(TEXT("  Set bHidden: true"));
		}
	}

	// v4.1: Set bResumeDialogueAfterLoad if true
	if (Definition.bResumeDialogueAfterLoad)
	{
		if (FBoolProperty* ResumeProp = CastField<FBoolProperty>(Quest->GetClass()->FindPropertyByName(TEXT("bResumeDialogueAfterLoad"))))
		{
			ResumeProp->SetPropertyValue_InContainer(Quest, true);
			LogGeneration(TEXT("  Set bResumeDialogueAfterLoad: true"));
		}
	}

	// v4.2: Set QuestDialogue (TSubclassOf<UDialogue>) if specified
	if (!Definition.Dialogue.IsEmpty())
	{
		FClassProperty* DialogueProp = CastField<FClassProperty>(Quest->GetClass()->FindPropertyByName(TEXT("QuestDialogue")));
		if (DialogueProp)
		{
			// Resolve dialogue class
			UClass* DialogueClass = FindObject<UClass>(nullptr, *Definition.Dialogue);
			if (!DialogueClass)
			{
				DialogueClass = LoadClass<UObject>(nullptr, *Definition.Dialogue);
			}
			if (!DialogueClass)
			{
				// Try with _C suffix for blueprint classes
				DialogueClass = LoadClass<UObject>(nullptr, *FString::Printf(TEXT("%s_C"), *Definition.Dialogue));
			}
			if (!DialogueClass)
			{
				// Try standard dialogue path
				DialogueClass = LoadClass<UObject>(nullptr, *FString::Printf(TEXT("/Game/FatherCompanion/Dialogues/%s.%s_C"),
					*Definition.Dialogue, *Definition.Dialogue));
			}

			if (DialogueClass)
			{
				DialogueProp->SetObjectPropertyValue_InContainer(Quest, DialogueClass);
				LogGeneration(FString::Printf(TEXT("  Set QuestDialogue: %s"), *Definition.Dialogue));
			}
			else
			{
				// v4.23 FAIL-FAST: Manifest references dialogue class that cannot be resolved
				LogGeneration(FString::Printf(TEXT("[E_QUEST_DIALOGUE_NOT_FOUND] %s | Could not resolve dialogue class: %s"),
					*Definition.Name, *Definition.Dialogue));
				return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
					FString::Printf(TEXT("Quest dialogue class '%s' not found"), *Definition.Dialogue));
			}
		}
	}

	// v4.2: Set QuestDialoguePlayParams if any overrides are specified
	if (!Definition.DialoguePlayParams.IsDefault())
	{
		FStructProperty* PlayParamsProp = CastField<FStructProperty>(Quest->GetClass()->FindPropertyByName(TEXT("QuestDialoguePlayParams")));
		if (PlayParamsProp)
		{
			void* ParamsPtr = PlayParamsProp->ContainerPtrToValuePtr<void>(Quest);
			const auto& Params = Definition.DialoguePlayParams;

			// Set StartFromID
			if (!Params.StartFromID.IsEmpty())
			{
				if (FNameProperty* StartFromProp = CastField<FNameProperty>(PlayParamsProp->Struct->FindPropertyByName(TEXT("StartFromID"))))
				{
					StartFromProp->SetPropertyValue_InContainer(ParamsPtr, FName(*Params.StartFromID));
					LogGeneration(FString::Printf(TEXT("  Set QuestDialoguePlayParams.StartFromID: %s"), *Params.StartFromID));
				}
			}

			// Set Priority
			if (Params.Priority != -1)
			{
				if (FIntProperty* PriorityProp = CastField<FIntProperty>(PlayParamsProp->Struct->FindPropertyByName(TEXT("Priority"))))
				{
					PriorityProp->SetPropertyValue_InContainer(ParamsPtr, Params.Priority);
					LogGeneration(FString::Printf(TEXT("  Set QuestDialoguePlayParams.Priority: %d"), Params.Priority));
				}
			}

			// Set override flags and values
			if (Params.bOverride_bFreeMovement)
			{
				if (FBoolProperty* OverrideProp = CastField<FBoolProperty>(PlayParamsProp->Struct->FindPropertyByName(TEXT("bOverride_bFreeMovement"))))
				{
					OverrideProp->SetPropertyValue_InContainer(ParamsPtr, true);
				}
				if (FBoolProperty* ValueProp = CastField<FBoolProperty>(PlayParamsProp->Struct->FindPropertyByName(TEXT("bFreeMovement"))))
				{
					ValueProp->SetPropertyValue_InContainer(ParamsPtr, Params.bFreeMovement);
					LogGeneration(FString::Printf(TEXT("  Set QuestDialoguePlayParams.bFreeMovement: %s"), Params.bFreeMovement ? TEXT("true") : TEXT("false")));
				}
			}
			if (Params.bOverride_bStopMovement)
			{
				if (FBoolProperty* OverrideProp = CastField<FBoolProperty>(PlayParamsProp->Struct->FindPropertyByName(TEXT("bOverride_bStopMovement"))))
				{
					OverrideProp->SetPropertyValue_InContainer(ParamsPtr, true);
				}
				if (FBoolProperty* ValueProp = CastField<FBoolProperty>(PlayParamsProp->Struct->FindPropertyByName(TEXT("bStopMovement"))))
				{
					ValueProp->SetPropertyValue_InContainer(ParamsPtr, Params.bStopMovement);
					LogGeneration(FString::Printf(TEXT("  Set QuestDialoguePlayParams.bStopMovement: %s"), Params.bStopMovement ? TEXT("true") : TEXT("false")));
				}
			}
			if (Params.bOverride_bUnskippable)
			{
				if (FBoolProperty* OverrideProp = CastField<FBoolProperty>(PlayParamsProp->Struct->FindPropertyByName(TEXT("bOverride_bUnskippable"))))
				{
					OverrideProp->SetPropertyValue_InContainer(ParamsPtr, true);
				}
				if (FBoolProperty* ValueProp = CastField<FBoolProperty>(PlayParamsProp->Struct->FindPropertyByName(TEXT("bUnskippable"))))
				{
					ValueProp->SetPropertyValue_InContainer(ParamsPtr, Params.bUnskippable);
					LogGeneration(FString::Printf(TEXT("  Set QuestDialoguePlayParams.bUnskippable: %s"), Params.bUnskippable ? TEXT("true") : TEXT("false")));
				}
			}
			if (Params.bOverride_bCanBeExited)
			{
				if (FBoolProperty* OverrideProp = CastField<FBoolProperty>(PlayParamsProp->Struct->FindPropertyByName(TEXT("bOverride_bCanBeExited"))))
				{
					OverrideProp->SetPropertyValue_InContainer(ParamsPtr, true);
				}
				if (FBoolProperty* ValueProp = CastField<FBoolProperty>(PlayParamsProp->Struct->FindPropertyByName(TEXT("bCanBeExited"))))
				{
					ValueProp->SetPropertyValue_InContainer(ParamsPtr, Params.bCanBeExited);
					LogGeneration(FString::Printf(TEXT("  Set QuestDialoguePlayParams.bCanBeExited: %s"), Params.bCanBeExited ? TEXT("true") : TEXT("false")));
				}
			}
		}
	}

	// v4.3: Create Quest Requirements (UQuestRequirement instanced objects)
	if (Definition.Requirements.Num() > 0)
	{
		LogGeneration(FString::Printf(TEXT("  Processing %d quest requirements..."), Definition.Requirements.Num()));

		// Find the QuestRequirements array property
		FArrayProperty* RequirementsArrayProp = CastField<FArrayProperty>(Quest->GetClass()->FindPropertyByName(TEXT("QuestRequirements")));
		if (RequirementsArrayProp)
		{
			FScriptArrayHelper ArrayHelper(RequirementsArrayProp, RequirementsArrayProp->ContainerPtrToValuePtr<void>(Quest));

			for (const auto& ReqDef : Definition.Requirements)
			{
				// Try to load the requirement class
				UClass* ReqClass = nullptr;
				TArray<FString> SearchPaths;
				SearchPaths.Add(FString::Printf(TEXT("/Script/NarrativeArsenal.%s"), *ReqDef.Type));
				SearchPaths.Add(FString::Printf(TEXT("/Game/FatherCompanion/Requirements/%s.%s_C"), *ReqDef.Type, *ReqDef.Type));
				SearchPaths.Add(FString::Printf(TEXT("/Game/Requirements/%s.%s_C"), *ReqDef.Type, *ReqDef.Type));

				for (const FString& Path : SearchPaths)
				{
					ReqClass = LoadClass<UObject>(nullptr, *Path);
					if (ReqClass) break;
				}

				if (ReqClass)
				{
					// Create instanced requirement object
					UObject* ReqInstance = NewObject<UObject>(Quest, ReqClass);
					if (ReqInstance)
					{
						// Set properties via reflection
						for (const auto& PropPair : ReqDef.Properties)
						{
							FProperty* Prop = ReqClass->FindPropertyByName(FName(*PropPair.Key));
							if (Prop)
							{
								Prop->ImportText_Direct(*PropPair.Value, Prop->ContainerPtrToValuePtr<void>(ReqInstance), nullptr, PPF_None);
							}
						}

						// Add to array
						int32 NewIndex = ArrayHelper.AddValue();
						FObjectProperty* InnerProp = CastField<FObjectProperty>(RequirementsArrayProp->Inner);
						if (InnerProp)
						{
							InnerProp->SetObjectPropertyValue(ArrayHelper.GetRawPtr(NewIndex), ReqInstance);
						}

						LogGeneration(FString::Printf(TEXT("    Added requirement: %s"), *ReqDef.Type));
					}
				}
				else
				{
					// v4.23 FAIL-FAST: Manifest references requirement class that cannot be loaded
					LogGeneration(FString::Printf(TEXT("[E_QUEST_REQUIREMENT_CLASS_NOT_FOUND] %s | Could not load requirement class: %s"),
						*Definition.Name, *ReqDef.Type));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						FString::Printf(TEXT("Quest requirement class '%s' not found"), *ReqDef.Type));
				}
			}
		}
		else
		{
			// v4.23 FAIL-FAST: Quest requirements specified but array property not found
			LogGeneration(FString::Printf(TEXT("[E_QUEST_REQUIREMENTS_PROPERTY_NOT_FOUND] %s | Could not find QuestRequirements array property"),
				*Definition.Name));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				TEXT("Quest requirements specified but QuestRequirements array property not found"));
		}
	}

	// State map for wiring
	TMap<FString, UQuestState*> StateMap;
	int32 TotalBranches = 0;

	// ========================================================================
	// FIRST PASS: Create all states
	// ========================================================================
	for (const auto& StateDef : Definition.States)
	{
		UQuestState* State = NewObject<UQuestState>(Quest, UQuestState::StaticClass());
		State->SetFlags(RF_Transactional);
		State->SetID(FName(*StateDef.Id));
		State->Description = FText::FromString(StateDef.Description);
		State->OwningQuest = Quest;

		// Set state type
		if (StateDef.Type.Equals(TEXT("success"), ESearchCase::IgnoreCase))
			State->StateNodeType = EStateNodeType::Success;
		else if (StateDef.Type.Equals(TEXT("failure"), ESearchCase::IgnoreCase))
			State->StateNodeType = EStateNodeType::Failure;
		else
			State->StateNodeType = EStateNodeType::Regular;

		// v4.2: Set custom event callback function name
		if (!StateDef.OnEnteredFuncName.IsEmpty())
		{
			State->OnEnteredFuncName = FName(*StateDef.OnEnteredFuncName);
		}

		// Add events to state
		for (const auto& EventDef : StateDef.Events)
		{
			if (UNarrativeEvent* Event = CreateDialogueEventFromDefinition(State, EventDef))
			{
				State->Events.Add(Event);
			}
		}

		Quest->AddState(State);
		StateMap.Add(StateDef.Id, State);

		LogGeneration(FString::Printf(TEXT("  Created state: %s (%s)"), *StateDef.Id, *StateDef.Type));
	}

	// Set start state
	FString StartStateId = Definition.StartState.IsEmpty() ? Definition.States[0].Id : Definition.StartState;
	if (StateMap.Contains(StartStateId))
	{
		Quest->SetQuestStartState(StateMap[StartStateId]);
		LogGeneration(FString::Printf(TEXT("  Set start state: %s"), *StartStateId));
	}

	// ========================================================================
	// SECOND PASS: Create branches (nested inside states in new structure)
	// ========================================================================
	for (const auto& StateDef : Definition.States)
	{
		UQuestState* SourceState = StateMap.FindRef(StateDef.Id);
		if (!SourceState) continue;

		for (const auto& BranchDef : StateDef.Branches)
		{
			UQuestBranch* Branch = NewObject<UQuestBranch>(Quest, UQuestBranch::StaticClass());
			Branch->SetFlags(RF_Transactional);
			Branch->OwningQuest = Quest;

			if (!BranchDef.Id.IsEmpty())
			{
				Branch->SetID(FName(*BranchDef.Id));
			}

			Branch->bHidden = BranchDef.bHidden;

			// v4.2: Set custom event callback function name
			if (!BranchDef.OnEnteredFuncName.IsEmpty())
			{
				Branch->OnEnteredFuncName = FName(*BranchDef.OnEnteredFuncName);
			}

			// Create tasks for this branch
			for (const auto& TaskDef : BranchDef.Tasks)
			{
				UClass* TaskClass = ResolveQuestTaskClass(TaskDef.TaskClass);
				if (!TaskClass)
				{
					// v4.23 FAIL-FAST: Manifest references task class that cannot be resolved
					LogGeneration(FString::Printf(TEXT("[E_QUEST_TASK_CLASS_NOT_FOUND] %s | Could not resolve task class: %s"),
						*Definition.Name, *TaskDef.TaskClass));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						FString::Printf(TEXT("Quest task class '%s' not found"), *TaskDef.TaskClass));
				}

				UNarrativeTask* Task = NewObject<UNarrativeTask>(Branch, TaskClass, NAME_None, RF_Public | RF_Transactional);
				if (Task)
				{
					Task->RequiredQuantity = TaskDef.Quantity;
					Task->bOptional = TaskDef.bOptional;
					Task->bHidden = TaskDef.bHidden;

					// Set task argument via reflection (common property names)
					if (!TaskDef.Argument.IsEmpty())
					{
						TArray<FName> ArgPropNames = { TEXT("Argument"), TEXT("TaskArgument"), TEXT("LocationName"), TEXT("ItemName"), TEXT("NPCName"), TEXT("CharacterName") };
						for (const FName& PropName : ArgPropNames)
						{
							if (FProperty* ArgProp = TaskClass->FindPropertyByName(PropName))
							{
								void* ValuePtr = ArgProp->ContainerPtrToValuePtr<void>(Task);
								ArgProp->ImportText_Direct(*TaskDef.Argument, ValuePtr, Task, 0);
								break;
							}
						}
					}

					Branch->QuestTasks.Add(Task);
				}
			}

			// Add events to branch
			for (const auto& EventDef : BranchDef.Events)
			{
				if (UNarrativeEvent* Event = CreateDialogueEventFromDefinition(Branch, EventDef))
				{
					Branch->Events.Add(Event);
				}
			}

			Quest->AddBranch(Branch);

			// Wire: State -> Branch
			SourceState->Branches.Add(Branch);

			// Wire: Branch -> Destination State
			if (!BranchDef.DestinationState.IsEmpty())
			{
				if (UQuestState* DestState = StateMap.FindRef(BranchDef.DestinationState))
				{
					Branch->DestinationState = DestState;
				}
				else
				{
					// v4.23 FAIL-FAST: S47 - Manifest references destination state that doesn't exist
					LogGeneration(FString::Printf(TEXT("[E_QUEST_DESTSTATE_NOT_FOUND] %s | Destination state not found: %s"), *Definition.Name, *BranchDef.DestinationState));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						FString::Printf(TEXT("Quest destination state '%s' not found"), *BranchDef.DestinationState));
				}
			}

			TotalBranches++;
			LogGeneration(FString::Printf(TEXT("  Created branch: %s -> %s (%d tasks)"),
				*StateDef.Id, *BranchDef.DestinationState, BranchDef.Tasks.Num()));
		}
	}

	// Set QuestDialogue if specified
	if (!Definition.Dialogue.IsEmpty())
	{
		TArray<FString> DialogueSearchPaths = {
			FString::Printf(TEXT("%s/Dialogues/%s.%s_C"), *GetProjectRoot(), *Definition.Dialogue, *Definition.Dialogue),
			FString::Printf(TEXT("%s/%s/%s.%s_C"), *GetProjectRoot(), *Folder, *Definition.Dialogue, *Definition.Dialogue),
			FString::Printf(TEXT("/Game/Dialogues/%s.%s_C"), *Definition.Dialogue, *Definition.Dialogue)
		};

		for (const FString& Path : DialogueSearchPaths)
		{
			if (UClass* DialogueClass = LoadClass<UDialogue>(nullptr, *Path))
			{
				if (FClassProperty* DialogueProp = CastField<FClassProperty>(Quest->GetClass()->FindPropertyByName(TEXT("QuestDialogue"))))
				{
					DialogueProp->SetPropertyValue_InContainer(Quest, DialogueClass);
					LogGeneration(FString::Printf(TEXT("  Set QuestDialogue: %s"), *Definition.Dialogue));
				}
				break;
			}
		}
	}

	// v4.23 FAIL-FAST: Questgiver field specified but UQuest has no Questgiver property
	// The questgiver pattern in NP is: NPC.dialogue → Dialogue (start_quest event) → Quest begins
	if (!Definition.Questgiver.IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("[E_QUESTGIVER_NOT_AUTOMATABLE] %s | Questgiver field specified but UQuest has no property | Questgiver: %s"),
			*Definition.Name, *Definition.Questgiver));
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Questgiver '%s' specified but UQuest has no Questgiver property - use NPC dialogue with start_quest event instead"), *Definition.Questgiver));
	}

	// v3.9.6/v3.9.11: Add reward events to success state using Narrative Pro's built-in events
	if (Definition.Rewards.Currency > 0 || Definition.Rewards.XP > 0 || Definition.Rewards.Items.Num() > 0)
	{
		// Find the success state
		UQuestState* SuccessState = nullptr;
		for (const auto& Pair : StateMap)
		{
			if (Pair.Value && Pair.Value->StateNodeType == EStateNodeType::Success)
			{
				SuccessState = Pair.Value;
				break;
			}
		}

		if (SuccessState)
		{
			// v3.9.11: Create actual reward events using Narrative Pro's built-in events
			// v4.12: Added fallback search paths for event classes

			// Helper lambda to find event class with multiple search paths
			auto FindEventClass = [](const TArray<FString>& SearchPaths) -> UClass*
			{
				for (const FString& Path : SearchPaths)
				{
					UClass* EventClass = LoadClass<UNarrativeEvent>(nullptr, *Path);
					if (EventClass) return EventClass;
				}
				return nullptr;
			};

			// XP Reward - NE_GiveXP
			if (Definition.Rewards.XP > 0)
			{
				TArray<FString> XPEventPaths = {
					TEXT("/NarrativePro/Pro/Core/Tales/Events/NE_GiveXP.NE_GiveXP_C"),
					TEXT("/NarrativePro/Tales/Events/NE_GiveXP.NE_GiveXP_C"),
					TEXT("/NarrativePro/Events/NE_GiveXP.NE_GiveXP_C"),
					TEXT("/Game/NarrativePro/Events/NE_GiveXP.NE_GiveXP_C")
				};
				UClass* GiveXPClass = FindEventClass(XPEventPaths);
				if (GiveXPClass)
				{
					UNarrativeEvent* XPEvent = NewObject<UNarrativeEvent>(SuccessState, GiveXPClass);
					if (XPEvent)
					{
						// Set XP amount via reflection (property name: "XP" or "Amount")
						if (FIntProperty* XPProp = FindFProperty<FIntProperty>(GiveXPClass, TEXT("XP")))
						{
							XPProp->SetPropertyValue_InContainer(XPEvent, Definition.Rewards.XP);
						}
						else if (FIntProperty* AmountProp = FindFProperty<FIntProperty>(GiveXPClass, TEXT("Amount")))
						{
							AmountProp->SetPropertyValue_InContainer(AmountProp, Definition.Rewards.XP);
						}
						XPEvent->EventRuntime = EEventRuntime::Start;
						SuccessState->Events.Add(XPEvent);
						LogGeneration(FString::Printf(TEXT("  Added reward event: NE_GiveXP +%d XP"), Definition.Rewards.XP));
					}
				}
				else
				{
					// v4.23 FAIL-FAST: XP reward specified but event class not found
					LogGeneration(FString::Printf(TEXT("[E_QUEST_XPREWARD_CLASS_NOT_FOUND] %s | Could not load NE_GiveXP class"),
						*Definition.Name));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						TEXT("Quest XP reward specified but NE_GiveXP class not found"));
				}
			}

			// Currency Reward - BPE_AddCurrency
			if (Definition.Rewards.Currency > 0)
			{
				TArray<FString> CurrencyEventPaths = {
					TEXT("/NarrativePro/Pro/Core/Tales/Events/BPE_AddCurrency.BPE_AddCurrency_C"),
					TEXT("/NarrativePro/Tales/Events/BPE_AddCurrency.BPE_AddCurrency_C"),
					TEXT("/NarrativePro/Events/BPE_AddCurrency.BPE_AddCurrency_C"),
					TEXT("/Game/NarrativePro/Events/BPE_AddCurrency.BPE_AddCurrency_C")
				};
				UClass* AddCurrencyClass = FindEventClass(CurrencyEventPaths);
				if (AddCurrencyClass)
				{
					UNarrativeEvent* CurrencyEvent = NewObject<UNarrativeEvent>(SuccessState, AddCurrencyClass);
					if (CurrencyEvent)
					{
						// Set currency amount via reflection
						if (FIntProperty* CurrencyProp = FindFProperty<FIntProperty>(AddCurrencyClass, TEXT("Currency")))
						{
							CurrencyProp->SetPropertyValue_InContainer(CurrencyEvent, Definition.Rewards.Currency);
						}
						else if (FIntProperty* AmountProp = FindFProperty<FIntProperty>(AddCurrencyClass, TEXT("Amount")))
						{
							AmountProp->SetPropertyValue_InContainer(CurrencyEvent, Definition.Rewards.Currency);
						}
						CurrencyEvent->EventRuntime = EEventRuntime::Start;
						SuccessState->Events.Add(CurrencyEvent);
						LogGeneration(FString::Printf(TEXT("  Added reward event: BPE_AddCurrency +%d"), Definition.Rewards.Currency));
					}
				}
				else
				{
					// v4.23 FAIL-FAST: Currency reward specified but event class not found
					LogGeneration(FString::Printf(TEXT("[E_QUEST_CURRENCYREWARD_CLASS_NOT_FOUND] %s | Could not load BPE_AddCurrency class"),
						*Definition.Name));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						TEXT("Quest currency reward specified but BPE_AddCurrency class not found"));
				}
			}

			// Item Rewards - BPE_AddItemToInventory
			for (int32 i = 0; i < Definition.Rewards.Items.Num(); i++)
			{
				const FString& ItemName = Definition.Rewards.Items[i];
				int32 Qty = Definition.Rewards.ItemQuantities.IsValidIndex(i) ? Definition.Rewards.ItemQuantities[i] : 1;

				TArray<FString> ItemEventPaths = {
					TEXT("/NarrativePro/Pro/Core/Tales/Events/BPE_AddItemToInventory.BPE_AddItemToInventory_C"),
					TEXT("/NarrativePro/Tales/Events/BPE_AddItemToInventory.BPE_AddItemToInventory_C"),
					TEXT("/NarrativePro/Events/BPE_AddItemToInventory.BPE_AddItemToInventory_C"),
					TEXT("/Game/NarrativePro/Events/BPE_AddItemToInventory.BPE_AddItemToInventory_C")
				};
				UClass* AddItemClass = FindEventClass(ItemEventPaths);
				if (AddItemClass)
				{
					UNarrativeEvent* ItemEvent = NewObject<UNarrativeEvent>(SuccessState, AddItemClass);
					if (ItemEvent)
					{
						// Set item class via reflection - need to load the item class first
						TArray<FString> ItemSearchPaths;
						ItemSearchPaths.Add(FString::Printf(TEXT("%s/Items/%s.%s_C"), *GetProjectRoot(), *ItemName, *ItemName));
						ItemSearchPaths.Add(FString::Printf(TEXT("%s/Items/Equipment/%s.%s_C"), *GetProjectRoot(), *ItemName, *ItemName));
						ItemSearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/Items/%s.%s_C"), *ItemName, *ItemName));

						UClass* ItemClass = nullptr;
						for (const FString& Path : ItemSearchPaths)
						{
							ItemClass = LoadClass<UObject>(nullptr, *Path);
							if (ItemClass) break;
						}

						if (ItemClass)
						{
							// Set ItemClass property (TSubclassOf<UNarrativeItem>)
							if (FClassProperty* ItemClassProp = FindFProperty<FClassProperty>(AddItemClass, TEXT("ItemClass")))
							{
								ItemClassProp->SetPropertyValue_InContainer(ItemEvent, ItemClass);
							}
							else if (FClassProperty* ItemProp = FindFProperty<FClassProperty>(AddItemClass, TEXT("Item")))
							{
								ItemProp->SetPropertyValue_InContainer(ItemEvent, ItemClass);
							}
						}

						// Set quantity
						if (FIntProperty* QtyProp = FindFProperty<FIntProperty>(AddItemClass, TEXT("Quantity")))
						{
							QtyProp->SetPropertyValue_InContainer(ItemEvent, Qty);
						}
						else if (FIntProperty* AmountProp = FindFProperty<FIntProperty>(AddItemClass, TEXT("Amount")))
						{
							AmountProp->SetPropertyValue_InContainer(ItemEvent, Qty);
						}

						ItemEvent->EventRuntime = EEventRuntime::Start;
						SuccessState->Events.Add(ItemEvent);
						LogGeneration(FString::Printf(TEXT("  Added reward event: BPE_AddItemToInventory %s x%d"), *ItemName, Qty));
					}
				}
				else
				{
					// v4.23 FAIL-FAST: Item reward specified but event class not found
					LogGeneration(FString::Printf(TEXT("[E_QUEST_ITEMREWARD_CLASS_NOT_FOUND] %s | Could not load BPE_AddItemToInventory class | Item: %s"),
						*Definition.Name, *ItemName));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						FString::Printf(TEXT("Quest item reward specified but BPE_AddItemToInventory class not found (item: %s)"), *ItemName));
				}
			}
		}
	}

	// v4.16: Compile with validation (Contract 10 - Blueprint Compile Gate)
	FCompilerResultsLog CompileLog;
	FKismetEditorUtilities::CompileBlueprint(QuestBP, EBlueprintCompileOptions::None, &CompileLog);

	if (CompileLog.NumErrors > 0)
	{
		TArray<FString> ErrorMessages;
		for (const TSharedRef<FTokenizedMessage>& Msg : CompileLog.Messages)
		{
			if (Msg->GetSeverity() == EMessageSeverity::Error)
			{
				ErrorMessages.Add(Msg->ToText().ToString());
			}
		}

		UE_LOG(LogTemp, Error, TEXT("[GasAbilityGenerator] [FAIL] %s: Quest Blueprint compilation failed with %d errors: %s"),
			*Definition.Name, CompileLog.NumErrors, *FString::Join(ErrorMessages, TEXT("; ")));

		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			FString::Printf(TEXT("Compile failed: %s"), *FString::Join(ErrorMessages, TEXT("; "))));
		Result.AssetPath = AssetPath;
		Result.GeneratorId = TEXT("Quest");
		Result.DetermineCategory();
		return Result;
	}

	// Save
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(QuestBP);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, QuestBP, *PackageFileName, SaveArgs);

	// ========================================================================
	// v4.13.3: Quest SM Semantic Verification
	// ========================================================================
	int32 TotalTasks = 0;
	int32 UnresolvedDestinations = 0;
	int32 DuplicateStateIds = 0;
	int32 DuplicateBranchIds = 0;
	TSet<FString> SeenStateIds;
	TSet<FString> SeenBranchIds;
	bool bStartStateValid = false;

	// Verify start state exists
	if (Quest->GetQuestStartState() != nullptr)
	{
		bStartStateValid = true;
	}
	else
	{
		// v4.23 FAIL-FAST: S48 - R3 structural invalidity - quest state machine requires start state
		LogGeneration(FString::Printf(TEXT("[E_QUEST_NO_START_STATE] %s | Quest has no valid start state"), *Definition.Name));
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
			TEXT("Quest has no valid start state"));
	}

	// Verify states and collect metrics
	for (const auto& StateDef : Definition.States)
	{
		// Check for duplicate state IDs
		if (SeenStateIds.Contains(StateDef.Id))
		{
			// v4.23 FAIL-FAST: S49 - R3 structural invalidity - duplicate state IDs cause nondeterministic behavior
			LogGeneration(FString::Printf(TEXT("[E_QUEST_DUPLICATE_STATE] %s | Duplicate state ID: %s"), *Definition.Name, *StateDef.Id));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("Duplicate state ID '%s'"), *StateDef.Id));
		}
		else
		{
			SeenStateIds.Add(StateDef.Id);
		}

		// Count tasks and verify branch destinations
		for (const auto& BranchDef : StateDef.Branches)
		{
			TotalTasks += BranchDef.Tasks.Num();

			// Check for duplicate branch IDs
			if (!BranchDef.Id.IsEmpty())
			{
				if (SeenBranchIds.Contains(BranchDef.Id))
				{
					// v4.23 FAIL-FAST: S50 - R3 structural invalidity - duplicate branch IDs cause nondeterministic behavior
					LogGeneration(FString::Printf(TEXT("[E_QUEST_DUPLICATE_BRANCH] %s | Duplicate branch ID: %s"), *Definition.Name, *BranchDef.Id));
					return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
						FString::Printf(TEXT("Duplicate branch ID '%s'"), *BranchDef.Id));
				}
				else
				{
					SeenBranchIds.Add(BranchDef.Id);
				}
			}

			// Verify destination state exists
			if (!BranchDef.DestinationState.IsEmpty() && !StateMap.Contains(BranchDef.DestinationState))
			{
				UnresolvedDestinations++;
				// Warning already logged during branch creation
			}
		}
	}

	// Quest SM semantic summary line (consistent with global RESULT pattern)
	LogGeneration(FString::Printf(TEXT("RESULT QuestSM: states=%d branches=%d tasks=%d unresolved=%d duplicates=%d start_valid=%s"),
		Definition.States.Num(),
		TotalBranches,
		TotalTasks,
		UnresolvedDestinations,
		DuplicateStateIds + DuplicateBranchIds,
		bStartStateValid ? TEXT("true") : TEXT("false")));

	LogGeneration(FString::Printf(TEXT("Created Quest Blueprint: %s (%d states, %d branches)"),
		*Definition.Name, Definition.States.Num(), TotalBranches));

	// v3.0: Store metadata
	StoreBlueprintMetadata(QuestBP, TEXT("Quest Blueprint"), Definition.Name, InputHash);

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.AssetPath = AssetPath;
	Result.GeneratorId = TEXT("Quest");
	Result.DetermineCategory();
	return Result;
}

// v3.9.4: Helper to resolve quest task classes
UClass* FQuestGenerator::ResolveQuestTaskClass(const FString& TaskClassName)
{
	TArray<FString> TaskSearchPaths;

	// Narrative Pro built-in tasks (BPT_ prefix style)
	TaskSearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/Tales/Tasks/%s.%s_C"), *TaskClassName, *TaskClassName));
	TaskSearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/Tales/Tasks/BPT_%s.BPT_%s_C"), *TaskClassName, *TaskClassName));

	// Sub-folders in Narrative Pro
	TaskSearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/Tales/Tasks/GoToLocation/%s.%s_C"), *TaskClassName, *TaskClassName));
	TaskSearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/Tales/Tasks/FindItem/%s.%s_C"), *TaskClassName, *TaskClassName));
	TaskSearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/Tales/Tasks/TalkTo/%s.%s_C"), *TaskClassName, *TaskClassName));

	// Project-specific paths
	TaskSearchPaths.Add(FString::Printf(TEXT("%s/Tales/Tasks/%s.%s_C"), *GetProjectRoot(), *TaskClassName, *TaskClassName));
	TaskSearchPaths.Add(FString::Printf(TEXT("%s/Quests/Tasks/%s.%s_C"), *GetProjectRoot(), *TaskClassName, *TaskClassName));

	// C++ class (native)
	TaskSearchPaths.Add(FString::Printf(TEXT("/Script/NarrativeArsenal.%s"), *TaskClassName));

	for (const FString& Path : TaskSearchPaths)
	{
		if (UClass* TaskClass = LoadClass<UNarrativeTask>(nullptr, *Path))
		{
			return TaskClass;
		}
	}

	return nullptr;
}

// ============================================================================
// v3.9.9: POI & NPC Spawner Placement Generators
// ============================================================================

#include "Navigation/POIActor.h"
#include "Spawners/NPCSpawner.h"
#include "Spawners/NPCSpawnComponent.h"
#include "EngineUtils.h"

// ----------------------------------------------------------------------------
// FPOIPlacementGenerator
// ----------------------------------------------------------------------------

FGenerationResult FPOIPlacementGenerator::Generate(
	const FManifestPOIPlacement& Definition,
	UWorld* World,
	TMap<FString, APOIActor*>& PlacedPOIs)
{
	FGenerationResult Result;

	if (!World)
	{
		Result = FGenerationResult(Definition.POITag, EGenerationStatus::Failed, TEXT("No world context"));
		return Result;
	}

	if (Definition.POITag.IsEmpty())
	{
		Result = FGenerationResult(TEXT("POI"), EGenerationStatus::Failed, TEXT("POI tag is required"));
		return Result;
	}

	// Check for existing POI with same tag
	APOIActor* ExistingPOI = FindExistingPOI(World, Definition.POITag);
	if (ExistingPOI)
	{
		// Already exists - skip
		LogGeneration(FString::Printf(TEXT("[SKIP] POI already exists: %s"), *Definition.POITag));
		PlacedPOIs.Add(Definition.POITag, ExistingPOI);
		Result = FGenerationResult(Definition.POITag, EGenerationStatus::Skipped, TEXT("Already exists"));
		return Result;
	}

	// Spawn the POI actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = FName(*Definition.POITag.Replace(TEXT("."), TEXT("_")));
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FTransform SpawnTransform(Definition.Rotation, Definition.Location);

	APOIActor* POI = World->SpawnActor<APOIActor>(
		APOIActor::StaticClass(),
		SpawnTransform,
		SpawnParams
	);

	if (!POI)
	{
		Result = FGenerationResult(Definition.POITag, EGenerationStatus::Failed, TEXT("Failed to spawn POI actor"));
		return Result;
	}

	// Set POI properties
	POI->POITag = FGameplayTag::RequestGameplayTag(FName(*Definition.POITag), false);
	if (!POI->POITag.IsValid())
	{
		// v4.23 FAIL-FAST: S51 - R3 structural invalidity - POI tag must be registered in DefaultGameplayTags.ini
		LogGeneration(FString::Printf(TEXT("[E_POI_TAG_NOT_REGISTERED] POI tag not registered: %s (must be in tags: section)"), *Definition.POITag));
		return FGenerationResult(Definition.POITag, EGenerationStatus::Failed,
			FString::Printf(TEXT("POI tag '%s' not registered in GameplayTags"), *Definition.POITag));
	}

	POI->POIDisplayName = FText::FromString(Definition.DisplayName);
	POI->bCreateMapMarker = Definition.bCreateMapMarker;
	POI->bSupportsFastTravel = Definition.bSupportsFastTravel;

	// Load map icon if specified
	if (!Definition.MapIcon.IsEmpty())
	{
		UTexture2D* IconTexture = LoadObject<UTexture2D>(nullptr, *Definition.MapIcon);
		if (IconTexture)
		{
			POI->POIIcon = IconTexture;
		}
		else
		{
			// v4.23 FAIL-FAST: Manifest references POI icon that cannot be loaded
			LogGeneration(FString::Printf(TEXT("[E_POI_ICON_NOT_FOUND] %s | Failed to load POI icon: %s"),
				*Definition.POITag, *Definition.MapIcon));
			return FGenerationResult(Definition.POITag, EGenerationStatus::Failed,
				FString::Printf(TEXT("POI icon '%s' not found"), *Definition.MapIcon));
		}
	}

	// Set actor label for identification
	POI->SetActorLabel(Definition.POITag);

	// Track for LinkedPOI resolution
	PlacedPOIs.Add(Definition.POITag, POI);

	// Note: LinkedPOIs are resolved in a second pass after all POIs are placed

	LogGeneration(FString::Printf(TEXT("[NEW] Placed POI: %s at (%0.0f, %0.0f, %0.0f)"),
		*Definition.POITag, Definition.Location.X, Definition.Location.Y, Definition.Location.Z));

	Result = FGenerationResult(Definition.POITag, EGenerationStatus::New);
	Result.AssetPath = FString::Printf(TEXT("LevelActor/POI/%s"), *Definition.POITag);
	Result.GeneratorId = TEXT("POIPlacement");
	Result.DetermineCategory();
	return Result;
}

APOIActor* FPOIPlacementGenerator::FindExistingPOI(UWorld* World, const FString& POITag)
{
	if (!World) return nullptr;

	FGameplayTag TargetTag = FGameplayTag::RequestGameplayTag(FName(*POITag), false);

	for (TActorIterator<APOIActor> It(World); It; ++It)
	{
		APOIActor* POI = *It;
		if (POI && POI->POITag == TargetTag)
		{
			return POI;
		}
		// Also check by actor label
		if (POI && POI->GetActorLabel() == POITag)
		{
			return POI;
		}
	}

	return nullptr;
}

// ----------------------------------------------------------------------------
// FNPCSpawnerPlacementGenerator
// ----------------------------------------------------------------------------

FGenerationResult FNPCSpawnerPlacementGenerator::Generate(
	const FManifestNPCSpawnerPlacement& Definition,
	UWorld* World,
	const TMap<FString, APOIActor*>& PlacedPOIs)
{
	FGenerationResult Result;

	if (!World)
	{
		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("No world context"));
		return Result;
	}

	if (Definition.Name.IsEmpty())
	{
		Result = FGenerationResult(TEXT("Spawner"), EGenerationStatus::Failed, TEXT("Spawner name is required"));
		return Result;
	}

	// Check for existing spawner
	ANPCSpawner* ExistingSpawner = FindExistingSpawner(World, Definition.Name);
	if (ExistingSpawner)
	{
		LogGeneration(FString::Printf(TEXT("[SKIP] Spawner already exists: %s"), *Definition.Name));
		Result = FGenerationResult(Definition.Name, EGenerationStatus::Skipped, TEXT("Already exists"));
		return Result;
	}

	// Determine spawn location
	FVector SpawnLocation = Definition.Location;
	if (!Definition.NearPOI.IsEmpty())
	{
		FVector POILocation = ResolvePOILocation(World, Definition.NearPOI, PlacedPOIs);
		// v4.23 FAIL-FAST: Check for error marker from ResolvePOILocation
		if (POILocation.X == -FLT_MAX && POILocation.Y == -FLT_MAX && POILocation.Z == -FLT_MAX)
		{
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("NearPOI '%s' not found - NPC spawner location cannot be resolved"), *Definition.NearPOI));
		}
		SpawnLocation = POILocation + Definition.POIOffset;
	}

	// Spawn the NPC spawner
	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = FName(*Definition.Name);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FTransform SpawnTransform(Definition.Rotation, SpawnLocation);

	ANPCSpawner* Spawner = World->SpawnActor<ANPCSpawner>(
		ANPCSpawner::StaticClass(),
		SpawnTransform,
		SpawnParams
	);

	if (!Spawner)
	{
		Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to spawn NPCSpawner actor"));
		return Result;
	}

	// Set spawner properties
	Spawner->SetActorLabel(Definition.Name);
	Spawner->SpawnerSaveGUID = FGuid::NewGuid();

	// v3.9.10: Determine activation behavior based on activation event
	bool bActivateOnBeginPlay = Definition.bActivateOnBeginPlay;
	if (!Definition.ActivationEvent.IsEmpty())
	{
		// When an activation event is specified, spawner starts inactive
		bActivateOnBeginPlay = false;
		LogGeneration(FString::Printf(TEXT("  Spawner will activate when event fires: %s"), *Definition.ActivationEvent));
	}
	if (!Definition.DeactivationEvent.IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("  Spawner will deactivate when event fires: %s"), *Definition.DeactivationEvent));
	}

	// Set activation flag via reflection (protected)
	if (FBoolProperty* ActivateProp = FindFProperty<FBoolProperty>(Spawner->GetClass(), TEXT("bActivateOnBeginPlay")))
	{
		ActivateProp->SetPropertyValue_InContainer(Spawner, bActivateOnBeginPlay);
	}

#if WITH_EDITOR
	// Add NPC spawn components for each entry
	for (int32 i = 0; i < Definition.NPCs.Num(); ++i)
	{
		const FManifestNPCSpawnEntry& NPCEntry = Definition.NPCs[i];

		UNPCSpawnComponent* SpawnComp = Spawner->CreateNPCSpawner();
		if (SpawnComp)
		{
			// Set relative transform
			SpawnComp->SetRelativeLocation(NPCEntry.RelativeLocation);
			SpawnComp->SetRelativeRotation(NPCEntry.RelativeRotation);

			// Configure the component
			FString ConfigError;
			if (!ConfigureSpawnComponent(SpawnComp, NPCEntry, ConfigError))
			{
				// v4.23 FAIL-FAST: Configuration failed (S27/S28)
				return FGenerationResult(Definition.Name, EGenerationStatus::Failed, ConfigError);
			}

			LogGeneration(FString::Printf(TEXT("  Added NPC spawn component: %s"), *NPCEntry.NPCDefinition));
		}
		else
		{
			// v4.23 FAIL-FAST: Failed to create NPC spawn component
			LogGeneration(FString::Printf(TEXT("[E_NPCSPAWNER_COMPONENT_CREATION_FAILED] %s | Failed to create NPC spawn component for: %s"),
				*Definition.Name, *NPCEntry.NPCDefinition));
			return FGenerationResult(Definition.Name, EGenerationStatus::Failed,
				FString::Printf(TEXT("Failed to create NPC spawn component for '%s'"), *NPCEntry.NPCDefinition));
		}
	}
#endif

	LogGeneration(FString::Printf(TEXT("[NEW] Placed NPCSpawner: %s at (%0.0f, %0.0f, %0.0f) with %d NPCs"),
		*Definition.Name, SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z, Definition.NPCs.Num()));

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.AssetPath = FString::Printf(TEXT("LevelActor/NPCSpawner/%s"), *Definition.Name);
	Result.GeneratorId = TEXT("NPCSpawnerPlacement");
	Result.DetermineCategory();
	return Result;
}

FVector FNPCSpawnerPlacementGenerator::ResolvePOILocation(
	UWorld* World,
	const FString& POITag,
	const TMap<FString, APOIActor*>& PlacedPOIs)
{
	// First check already-placed POIs
	if (const APOIActor* const* FoundPOI = PlacedPOIs.Find(POITag))
	{
		if (*FoundPOI)
		{
			return (*FoundPOI)->GetActorLocation();
		}
	}

	// Search world for existing POI
	APOIActor* ExistingPOI = FPOIPlacementGenerator::FindExistingPOI(World, POITag);
	if (ExistingPOI)
	{
		return ExistingPOI->GetActorLocation();
	}

	// v4.23 FAIL-FAST: Manifest references POI that doesn't exist
	// Return special marker FVector(-FLT_MAX,-FLT_MAX,-FLT_MAX) to signal failure to caller
	LogGeneration(FString::Printf(TEXT("[E_POI_LOCATION_NOT_FOUND] Could not resolve POI location: %s"), *POITag));
	return FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX); // Error marker
}

ANPCSpawner* FNPCSpawnerPlacementGenerator::FindExistingSpawner(UWorld* World, const FString& SpawnerName)
{
	if (!World) return nullptr;

	for (TActorIterator<ANPCSpawner> It(World); It; ++It)
	{
		ANPCSpawner* Spawner = *It;
		if (Spawner && Spawner->GetActorLabel() == SpawnerName)
		{
			return Spawner;
		}
	}

	return nullptr;
}

bool FNPCSpawnerPlacementGenerator::ConfigureSpawnComponent(
	UNPCSpawnComponent* Component,
	const FManifestNPCSpawnEntry& Entry,
	FString& OutErrorMessage)
{
	if (!Component)
	{
		OutErrorMessage = TEXT("Component is null");
		return false;
	}

	// Load NPC Definition
	TArray<FString> NPCDefSearchPaths;
	NPCDefSearchPaths.Add(FString::Printf(TEXT("%s/NPCs/Definitions/%s.%s"), *GetProjectRoot(), *Entry.NPCDefinition, *Entry.NPCDefinition));
	NPCDefSearchPaths.Add(FString::Printf(TEXT("%s/Characters/Definitions/%s.%s"), *GetProjectRoot(), *Entry.NPCDefinition, *Entry.NPCDefinition));
	NPCDefSearchPaths.Add(FString::Printf(TEXT("%s/%s.%s"), *GetProjectRoot(), *Entry.NPCDefinition, *Entry.NPCDefinition));

	UNPCDefinition* NPCDef = nullptr;
	for (const FString& Path : NPCDefSearchPaths)
	{
		NPCDef = LoadObject<UNPCDefinition>(nullptr, *Path);
		if (NPCDef) break;
	}

	if (NPCDef)
	{
		Component->NPCToSpawn = NPCDef;
	}
	else
	{
		// v4.23 FAIL-FAST: S27 - Manifest references NPCDefinition that cannot be loaded
		LogGeneration(FString::Printf(TEXT("[E_NPCSPAWNER_NPCDEFINITION_NOT_FOUND] Could not load NPCDefinition: %s"), *Entry.NPCDefinition));
		OutErrorMessage = FString::Printf(TEXT("NPCDefinition '%s' not found"), *Entry.NPCDefinition);
		return false;
	}

	// Set spawn component properties
	Component->bDontSpawnIfPreviouslyKilled = Entry.bDontSpawnIfKilled;
	Component->UntetherDistance = Entry.UntetherDistance;
	Component->NPCSaveGUID = FGuid::NewGuid();

	// Configure spawn params
	FNPCSpawnParams& Params = Component->SpawnParams;

	if (Entry.SpawnParams.bOverrideLevelRange)
	{
		// Set override flag and values via reflection (may be protected)
		if (FBoolProperty* OverrideProp = FindFProperty<FBoolProperty>(FNPCSpawnParams::StaticStruct(), TEXT("bOverride_LevelRange")))
		{
			OverrideProp->SetPropertyValue_InContainer(&Params, true);
		}
		Params.MinLevel = Entry.SpawnParams.MinLevel;
		Params.MaxLevel = Entry.SpawnParams.MaxLevel;
	}

	if (Entry.SpawnParams.bOverrideOwnedTags && Entry.SpawnParams.DefaultOwnedTags.Num() > 0)
	{
		if (FBoolProperty* OverrideProp = FindFProperty<FBoolProperty>(FNPCSpawnParams::StaticStruct(), TEXT("bOverride_DefaultOwnedTags")))
		{
			OverrideProp->SetPropertyValue_InContainer(&Params, true);
		}
		for (const FString& Tag : Entry.SpawnParams.DefaultOwnedTags)
		{
			FGameplayTag GameplayTag = FGameplayTag::RequestGameplayTag(FName(*Tag), false);
			if (GameplayTag.IsValid())
			{
				Params.DefaultOwnedTags.AddTag(GameplayTag);
			}
		}
	}

	if (Entry.SpawnParams.bOverrideFactions && Entry.SpawnParams.DefaultFactions.Num() > 0)
	{
		if (FBoolProperty* OverrideProp = FindFProperty<FBoolProperty>(FNPCSpawnParams::StaticStruct(), TEXT("bOverride_DefaultFactions")))
		{
			OverrideProp->SetPropertyValue_InContainer(&Params, true);
		}
		for (const FString& Faction : Entry.SpawnParams.DefaultFactions)
		{
			FGameplayTag FactionTag = FGameplayTag::RequestGameplayTag(FName(*Faction), false);
			if (FactionTag.IsValid())
			{
				Params.DefaultFactions.AddTag(FactionTag);
			}
		}
	}

	// Load optional goal
	if (!Entry.OptionalGoal.IsEmpty())
	{
		TArray<FString> GoalSearchPaths;
		GoalSearchPaths.Add(FString::Printf(TEXT("%s/AI/Goals/%s.%s_C"), *GetProjectRoot(), *Entry.OptionalGoal, *Entry.OptionalGoal));
		GoalSearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/GoToLocation/%s.%s_C"), *Entry.OptionalGoal, *Entry.OptionalGoal));
		GoalSearchPaths.Add(FString::Printf(TEXT("/NarrativePro/Pro/Core/AI/Activities/Attacks/Goals/%s.%s_C"), *Entry.OptionalGoal, *Entry.OptionalGoal));

		UClass* GoalClass = nullptr;
		for (const FString& Path : GoalSearchPaths)
		{
			GoalClass = LoadClass<UNPCGoalItem>(nullptr, *Path);
			if (GoalClass) break;
		}

		if (GoalClass)
		{
			Component->OptionalGoal = NewObject<UNPCGoalItem>(Component, GoalClass);
		}
		else
		{
			// v4.23 FAIL-FAST: S28 - Manifest references OptionalGoal that cannot be loaded
			LogGeneration(FString::Printf(TEXT("[E_NPCSPAWNER_GOAL_NOT_FOUND] Could not load goal class: %s"), *Entry.OptionalGoal));
			OutErrorMessage = FString::Printf(TEXT("Goal class '%s' not found"), *Entry.OptionalGoal);
			return false;
		}
	}

	return true;
}
