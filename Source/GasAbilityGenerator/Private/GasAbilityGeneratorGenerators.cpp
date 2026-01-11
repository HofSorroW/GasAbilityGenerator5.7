// GasAbilityGenerator v2.6.5
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
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

// v2.1.9: Static member initialization for manifest validation
const FManifestData* FGeneratorBase::ActiveManifest = nullptr;

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
	// Check disk first
	if (DoesAssetExistOnDisk(AssetPath))
	{
		LogGeneration(FString::Printf(TEXT("Skipping existing %s (on disk): %s"), *AssetType, *AssetName));
		OutResult = FGenerationResult(AssetName, EGenerationStatus::Skipped, TEXT("Already exists on disk"));
		OutResult.DetermineCategory();
		return true; // Asset exists, should skip
	}
	
	// Check memory
	if (IsAssetInMemory(AssetPath))
	{
		LogGeneration(FString::Printf(TEXT("Skipping existing %s (in memory): %s"), *AssetType, *AssetName));
		OutResult = FGenerationResult(AssetName, EGenerationStatus::Skipped, TEXT("Already loaded in memory"));
		OutResult.DetermineCategory();
		return true; // Asset exists, should skip
	}
	
	return false; // Asset doesn't exist, should generate
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

		// Also try common Blueprint paths
		TArray<FString> BlueprintSearchPaths;
		FString Root = GetProjectRoot();  // v2.5.0: Use auto-detected project root
		BlueprintSearchPaths.Add(FString::Printf(TEXT("%s/Blueprints/%s.%s_C"), *Root, *ClassName, *ClassName));
		BlueprintSearchPaths.Add(FString::Printf(TEXT("%s/Characters/%s.%s_C"), *Root, *ClassName, *ClassName));
		BlueprintSearchPaths.Add(FString::Printf(TEXT("%s/Actors/%s.%s_C"), *Root, *ClassName, *ClassName));
		BlueprintSearchPaths.Add(FString::Printf(TEXT("/Game/%s.%s_C"), *ClassName, *ClassName));

		for (const FString& Path : BlueprintSearchPaths)
		{
			UClass* FoundClass = LoadObject<UClass>(nullptr, *Path);
			if (FoundClass)
			{
				return FoundClass;
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

	// Check existence and return SKIPPED if exists
	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Enumeration"), Result))
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

	// v2.0.9 FIX: Check existence and return SKIPPED if exists
	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Input Action"), Result))
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

	// v2.0.9 FIX: Check existence and return SKIPPED if exists
	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Input Mapping Context"), Result))
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

	// v2.0.9 FIX: Check existence and return SKIPPED if exists
	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Gameplay Effect"), Result))
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

	// v2.0.9 FIX: Check existence - but for GAs, configure tags on existing assets before skipping
	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Gameplay Ability"), Result))
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

	// v2.0.9 FIX: Check existence and return SKIPPED if exists
	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Actor Blueprint"), Result))
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

	// v2.2.0: Generate event graph if specified
	if (!Definition.EventGraphName.IsEmpty() && ManifestData)
	{
		// v2.6.7: Clear any previous missing dependencies before generation
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

	// v2.0.9 FIX: Check existence and return SKIPPED if exists
	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Widget Blueprint"), Result))
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

	// v2.0.9 FIX: Check existence and return SKIPPED if exists
	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Blackboard"), Result))
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

	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// FBehaviorTreeGenerator Implementation
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

	// v2.0.9 FIX: Check existence and return SKIPPED if exists
	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Behavior Tree"), Result))
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
		FString BBPath = FString::Printf(TEXT("/Game/AI/%s.%s"), 
			*Definition.BlackboardAsset, *Definition.BlackboardAsset);
		UBlackboardData* BB = LoadObject<UBlackboardData>(nullptr, *BBPath);
		if (BB)
		{
			BT->BlackboardAsset = BB;
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
	
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New, TEXT("Created successfully"));
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// FMaterialGenerator Implementation
// ============================================================================

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

	// v2.0.9 FIX: Check existence and return SKIPPED if exists
	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Material"), Result))
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
	if (Definition.BlendMode == TEXT("Translucent"))
	{
		Material->BlendMode = BLEND_Translucent;
	}
	else if (Definition.BlendMode == TEXT("Masked"))
	{
		Material->BlendMode = BLEND_Masked;
	}
	else if (Definition.BlendMode == TEXT("Additive"))
	{
		Material->BlendMode = BLEND_Additive;
	}
	else
	{
		Material->BlendMode = BLEND_Opaque;
	}
	
	// Mark dirty and register
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Material);
	
	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Material, *PackageFileName, SaveArgs);
	
	LogGeneration(FString::Printf(TEXT("Created Material: %s"), *Definition.Name));
	
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

	// v2.5.2: Post-generation validation - check for unconnected required pins
	int32 UnconnectedInputPins = 0;
	int32 UnconnectedExecOutputPins = 0;
	LogGeneration(TEXT("  --- Pin Connection Validation ---"));

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

			// Check unconnected input pins (potential issues)
			if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() == 0)
			{
				// Skip exec pins for pure functions and pins with default values
				bool bIsExecPin = (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec);
				bool bHasDefaultValue = !Pin->DefaultValue.IsEmpty() || !Pin->DefaultTextValue.IsEmpty() || Pin->DefaultObject != nullptr;

				// v2.6.4: Skip "self" pins on CallFunction nodes - these are implicit for target_self functions
				FString PinNameStr = Pin->PinName.ToString();
				bool bIsSelfPin = PinNameStr.Equals(TEXT("self"), ESearchCase::IgnoreCase) ||
				                  PinNameStr.Equals(UEdGraphSchema_K2::PN_Self.ToString(), ESearchCase::IgnoreCase);
				if (bIsSelfPin)
				{
					// Self pins are implicit for Blueprint member functions - not an error
					continue;
				}

				// Log significant unconnected pins
				if (!bIsExecPin && !bHasDefaultValue && !Pin->PinType.PinCategory.IsNone())
				{
					// Only warn about object/struct pins that likely need connections
					FString PinCategory = Pin->PinType.PinCategory.ToString();
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
					FString PinNameStr = Pin->PinName.ToString();
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
	}

	// Determine the function owner class
	UClass* FunctionOwner = nullptr;
	const FString* ClassNamePtr = NodeDef.Properties.Find(TEXT("class"));

	// v2.4.3: First check the well-known functions table
	FString FunctionNameStr = FunctionName.ToString();

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

	// v2.5.2: Enhanced connection with validation and detailed logging
	LogGeneration(FString::Printf(TEXT("    Attempting connection: %s.%s [%s] -> %s.%s [%s]"),
		*Connection.From.NodeId, *FromPin->PinName.ToString(), *FromPin->PinType.PinCategory.ToString(),
		*Connection.To.NodeId, *ToPin->PinName.ToString(), *ToPin->PinType.PinCategory.ToString()));

	// Check if pins are already connected
	if (FromPin->LinkedTo.Contains(ToPin))
	{
		LogGeneration(TEXT("    Pins already connected"));
		return true;
	}

	// Check pin type compatibility before connecting
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	if (K2Schema)
	{
		FPinConnectionResponse Response = K2Schema->CanCreateConnection(FromPin, ToPin);
		if (Response.Response != CONNECT_RESPONSE_MAKE)
		{
			LogGeneration(FString::Printf(TEXT("    WARNING: Pin types may not be compatible: %s"),
				*Response.Message.ToString()));
			// Still try to connect - sometimes it works despite warnings
		}
	}

	// Make the connection
	FromPin->MakeLinkTo(ToPin);

	// v2.5.2: Verify the connection was actually made
	bool bConnectionSuccess = FromPin->LinkedTo.Contains(ToPin);
	if (!bConnectionSuccess)
	{
		LogGeneration(FString::Printf(TEXT("    ERROR: Connection was NOT established between %s.%s and %s.%s"),
			*Connection.From.NodeId, *FromPin->PinName.ToString(),
			*Connection.To.NodeId, *ToPin->PinName.ToString()));
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
#include "Items/InventoryComponent.h"  // Contains UItemCollection
#include "GAS/AbilityConfiguration.h"
#include "AI/Activities/NPCActivityConfiguration.h"
#include "AI/Activities/NPCActivity.h"
#include "Items/EquippableItem.h"
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

	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Float Curve"), Result))
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

	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Animation Montage"), Result))
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

	LogGeneration(FString::Printf(TEXT("Created Animation Montage: %s with %d sections"),
		*Definition.Name, Definition.Sections.Num()));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// v2.3.0: Animation Notify Generator (placeholder)
FGenerationResult FAnimationNotifyGenerator::Generate(const FManifestAnimationNotifyDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Animations/Notifies") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Animation Notify"), Result))
	{
		return Result;
	}

	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Animation Notify"), Result))
	{
		return Result;
	}

	LogGeneration(FString::Printf(TEXT("Animation Notify '%s' - create manually in editor"), *Definition.Name));

	Result = FGenerationResult(Definition.Name, EGenerationStatus::Failed,
		TEXT("Animation Notifies require manual creation"));
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

	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Dialogue Blueprint"), Result))
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

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Blueprint);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Dialogue Blueprint: %s"), *Definition.Name));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// v2.6.0: Equippable Item Generator - sets slots, effect, and abilities on CDO
FGenerationResult FEquippableItemGenerator::Generate(const FManifestEquippableItemDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Items") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Equippable Item"), Result))
	{
		return Result;
	}

	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Equippable Item"), Result))
	{
		return Result;
	}

	// Find UEquippableItem class
	UClass* ParentClass = UEquippableItem::StaticClass();
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

			// Set equipment effect using reflection
			if (!Definition.EquipmentModifierGE.IsEmpty())
			{
				FString GEPath = Definition.EquipmentModifierGE;
				if (!GEPath.Contains(TEXT("/")))
				{
					GEPath = FString::Printf(TEXT("%s/Effects/%s.%s_C"), *GetProjectRoot(), *Definition.EquipmentModifierGE, *Definition.EquipmentModifierGE);
				}
				UClass* GEClass = LoadObject<UClass>(nullptr, *GEPath);
				if (!GEClass)
				{
					GEPath = FString::Printf(TEXT("%s/Effects/%s"), *GetProjectRoot(), *Definition.EquipmentModifierGE);
					GEClass = LoadObject<UClass>(nullptr, *GEPath);
				}
				if (GEClass && GEClass->IsChildOf(UGameplayEffect::StaticClass()))
				{
					FClassProperty* EffectProperty = CastField<FClassProperty>(CDO->GetClass()->FindPropertyByName(TEXT("EquipmentEffect")));
					if (EffectProperty)
					{
						EffectProperty->SetObjectPropertyValue_InContainer(CDO, GEClass);
						LogGeneration(FString::Printf(TEXT("  Set EquipmentEffect: %s"), *GEClass->GetName()));
					}
				}
				else
				{
					LogGeneration(FString::Printf(TEXT("  INFO: EquipmentEffect '%s' will need manual setup"), *Definition.EquipmentModifierGE));
				}
			}

			// Set abilities using reflection
			FArrayProperty* AbilitiesProperty = CastField<FArrayProperty>(CDO->GetClass()->FindPropertyByName(TEXT("EquipmentAbilities")));
			if (AbilitiesProperty)
			{
				FScriptArrayHelper ArrayHelper(AbilitiesProperty, AbilitiesProperty->ContainerPtrToValuePtr<void>(CDO));
				for (const FString& AbilityName : Definition.AbilitiesToGrant)
				{
					FString AbilityPath = AbilityName;
					if (!AbilityPath.Contains(TEXT("/")))
					{
						AbilityPath = FString::Printf(TEXT("%s/Abilities/%s.%s_C"), *GetProjectRoot(), *AbilityName, *AbilityName);
					}
					UClass* AbilityClass = LoadObject<UClass>(nullptr, *AbilityPath);
					if (AbilityClass)
					{
						int32 NewIndex = ArrayHelper.AddValue();
						FClassProperty* InnerProp = CastField<FClassProperty>(AbilitiesProperty->Inner);
						if (InnerProp)
						{
							InnerProp->SetObjectPropertyValue(ArrayHelper.GetRawPtr(NewIndex), AbilityClass);
							LogGeneration(FString::Printf(TEXT("  Added ability: %s"), *AbilityClass->GetName()));
						}
					}
					else
					{
						LogGeneration(FString::Printf(TEXT("  INFO: Ability '%s' will need manual setup"), *AbilityName));
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
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// v2.6.0: Activity Generator - sets BehaviourTree on CDO
FGenerationResult FActivityGenerator::Generate(const FManifestActivityDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("AI/Activities") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Activity"), Result))
	{
		return Result;
	}

	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Activity"), Result))
	{
		return Result;
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

	// v2.6.0: Set BehaviourTree on the CDO using reflection (property is protected)
	if (Blueprint->GeneratedClass && !Definition.BehaviorTree.IsEmpty())
	{
		UObject* CDO = Blueprint->GeneratedClass->GetDefaultObject();
		if (CDO)
		{
			// Try to load the behavior tree
			FString BTPath = Definition.BehaviorTree;
			UBehaviorTree* BT = nullptr;

			// Try direct path first
			BT = LoadObject<UBehaviorTree>(nullptr, *BTPath);

			// Try common paths if not found
			if (!BT)
			{
				TArray<FString> SearchPaths = {
					FString::Printf(TEXT("%s/AI/BehaviorTrees/%s"), *GetProjectRoot(), *Definition.BehaviorTree),
					FString::Printf(TEXT("%s/AI/%s"), *GetProjectRoot(), *Definition.BehaviorTree),
					FString::Printf(TEXT("/Game/AI/BehaviorTrees/%s"), *Definition.BehaviorTree)
				};

				for (const FString& SearchPath : SearchPaths)
				{
					BT = LoadObject<UBehaviorTree>(nullptr, *SearchPath);
					if (BT) break;
				}
			}

			if (BT)
			{
				// Use reflection to set the protected BehaviourTree property
				FObjectProperty* BTProperty = CastField<FObjectProperty>(CDO->GetClass()->FindPropertyByName(TEXT("BehaviourTree")));
				if (BTProperty)
				{
					BTProperty->SetObjectPropertyValue_InContainer(CDO, BT);
					LogGeneration(FString::Printf(TEXT("  Set BehaviourTree: %s"), *BT->GetName()));
				}
			}
			else
			{
				LogGeneration(FString::Printf(TEXT("  INFO: BehaviorTree '%s' will need manual setup"), *Definition.BehaviorTree));
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
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// v2.3.0: Ability Configuration Generator (placeholder)
FGenerationResult FAbilityConfigurationGenerator::Generate(const FManifestAbilityConfigurationDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Configurations") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Ability Configuration"), Result))
	{
		return Result;
	}

	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Ability Configuration"), Result))
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

	// Log abilities that need to be set manually (require class references)
	if (Definition.Abilities.Num() > 0)
	{
		LogGeneration(FString::Printf(TEXT("  Ability Config '%s' - add %d abilities manually:"),
			*Definition.Name, Definition.Abilities.Num()));
		for (const FString& Ability : Definition.Abilities)
		{
			LogGeneration(FString::Printf(TEXT("    - %s"), *Ability));
		}
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(AbilityConfig);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, AbilityConfig, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Ability Configuration: %s"), *Definition.Name));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// v2.3.0: Activity Configuration Generator - creates UNPCActivityConfiguration data assets
FGenerationResult FActivityConfigurationGenerator::Generate(const FManifestActivityConfigurationDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("Configurations") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Activity Configuration"), Result))
	{
		return Result;
	}

	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Activity Configuration"), Result))
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

	// Log activities that need to be set manually (require class references)
	if (Definition.Activities.Num() > 0)
	{
		LogGeneration(FString::Printf(TEXT("  Activity Config '%s' - add %d activities manually:"),
			*Definition.Name, Definition.Activities.Num()));
		for (const FString& Activity : Definition.Activities)
		{
			LogGeneration(FString::Printf(TEXT("    - %s"), *Activity));
		}
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(ActivityConfig);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, ActivityConfig, *PackageFileName, SaveArgs);

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

	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Item Collection"), Result))
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

	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Narrative Event"), Result))
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

	// Log event details for manual setup
	if (!Definition.EventType.IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("  Event '%s' Type: %s"), *Definition.Name, *Definition.EventType));
	}
	if (!Definition.Description.IsEmpty())
	{
		LogGeneration(FString::Printf(TEXT("  Event '%s' Description: %s"), *Definition.Name, *Definition.Description));
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Blueprint);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

	LogGeneration(FString::Printf(TEXT("Created Narrative Event: %s"), *Definition.Name));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// v2.3.0: NPC Definition Generator - creates UNPCDefinition data assets
FGenerationResult FNPCDefinitionGenerator::Generate(const FManifestNPCDefinitionDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("NPCs/Definitions") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("NPC Definition"), Result))
	{
		return Result;
	}

	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("NPC Definition"), Result))
	{
		return Result;
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

	// v2.6.0: Set AbilityConfiguration if specified
	if (!Definition.AbilityConfiguration.IsEmpty())
	{
		FString ConfigPath = Definition.AbilityConfiguration;
		if (!ConfigPath.Contains(TEXT("/")))
		{
			ConfigPath = FString::Printf(TEXT("%s/GAS/Configurations/%s"), *GetProjectRoot(), *Definition.AbilityConfiguration);
		}
		UAbilityConfiguration* AbilityConfig = LoadObject<UAbilityConfiguration>(nullptr, *ConfigPath);
		if (AbilityConfig)
		{
			NPCDef->AbilityConfiguration = AbilityConfig;
			LogGeneration(FString::Printf(TEXT("  Set AbilityConfiguration: %s"), *AbilityConfig->GetName()));
		}
		else
		{
			LogGeneration(FString::Printf(TEXT("  INFO: AbilityConfiguration '%s' not found, will need manual setup"), *Definition.AbilityConfiguration));
		}
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NPCDef);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, NPCDef, *PackageFileName, SaveArgs);

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

	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Character Definition"), Result))
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

	// Parse tags
	if (!Definition.DefaultOwnedTags.IsEmpty())
	{
		TArray<FString> TagStrings;
		Definition.DefaultOwnedTags.ParseIntoArray(TagStrings, TEXT(","), true);
		for (const FString& TagString : TagStrings)
		{
			FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString.TrimStartAndEnd()), false);
			if (Tag.IsValid())
			{
				CharDef->DefaultOwnedTags.AddTag(Tag);
			}
		}
	}

	// Parse factions
	if (!Definition.DefaultFactions.IsEmpty())
	{
		TArray<FString> FactionStrings;
		Definition.DefaultFactions.ParseIntoArray(FactionStrings, TEXT(","), true);
		for (const FString& FactionString : FactionStrings)
		{
			FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*FactionString.TrimStartAndEnd()), false);
			if (Tag.IsValid())
			{
				CharDef->DefaultFactions.AddTag(Tag);
			}
		}
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(CharDef);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, CharDef, *PackageFileName, SaveArgs);

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

	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Tagged Dialogue Set"), Result))
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

	LogGeneration(FString::Printf(TEXT("Created Tagged Dialogue Set: %s with %d dialogues"),
		*Definition.Name, Definition.Dialogues.Num()));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}

// ============================================================================
// v2.6.5: Niagara System Generator
// ============================================================================

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraEditorUtilities.h"

FGenerationResult FNiagaraSystemGenerator::Generate(const FManifestNiagaraSystemDefinition& Definition)
{
	FString Folder = Definition.Folder.IsEmpty() ? TEXT("VFX/NiagaraSystems") : Definition.Folder;
	FString AssetPath = FString::Printf(TEXT("%s/%s/%s"), *GetProjectRoot(), *Folder, *Definition.Name);
	FGenerationResult Result;

	if (ValidateAgainstManifest(Definition.Name, TEXT("Niagara System"), Result))
	{
		return Result;
	}

	if (CheckExistsAndPopulateResult(AssetPath, Definition.Name, TEXT("Niagara System"), Result))
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
		}
	}

	if (!NewSystem)
	{
		return FGenerationResult(Definition.Name, EGenerationStatus::Failed, TEXT("Failed to create Niagara System"));
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

	LogGeneration(FString::Printf(TEXT("Created Niagara System: %s"), *Definition.Name));
	Result = FGenerationResult(Definition.Name, EGenerationStatus::New);
	Result.DetermineCategory();
	return Result;
}
