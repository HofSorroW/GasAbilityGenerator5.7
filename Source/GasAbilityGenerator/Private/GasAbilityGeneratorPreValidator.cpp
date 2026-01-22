// GasAbilityGenerator v4.24 - Phase 4.1 Pre-Validation System
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#include "GasAbilityGeneratorPreValidator.h"
#include "Locked/GasAbilityGeneratorTypes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameplayTagsManager.h"
#include "UObject/UObjectIterator.h"

// Log category for pre-validation
DEFINE_LOG_CATEGORY_STATIC(LogPreValidator, Log, All);

// ============================================================================
// FPreValidationCache Implementation
// ============================================================================

bool FPreValidationCache::CheckFunctionExists(const FString& ClassName, const FString& FunctionName, bool& bOutExists)
{
	FString Key = FString::Printf(TEXT("%s::%s"), *ClassName, *FunctionName);
	if (const bool* Found = FunctionCache.Find(Key))
	{
		bOutExists = *Found;
		HitCount++;
		return true;
	}
	MissCount++;
	return false;
}

void FPreValidationCache::CacheFunctionResult(const FString& ClassName, const FString& FunctionName, bool bExists)
{
	FString Key = FString::Printf(TEXT("%s::%s"), *ClassName, *FunctionName);
	FunctionCache.Add(Key, bExists);
}

bool FPreValidationCache::CheckAttributeExists(const FString& AttributeSetClass, const FString& PropertyName, bool& bOutExists)
{
	FString Key = FString::Printf(TEXT("%s::%s"), *AttributeSetClass, *PropertyName);
	if (const bool* Found = AttributeCache.Find(Key))
	{
		bOutExists = *Found;
		HitCount++;
		return true;
	}
	MissCount++;
	return false;
}

void FPreValidationCache::CacheAttributeResult(const FString& AttributeSetClass, const FString& PropertyName, bool bExists)
{
	FString Key = FString::Printf(TEXT("%s::%s"), *AttributeSetClass, *PropertyName);
	AttributeCache.Add(Key, bExists);
}

bool FPreValidationCache::GetCachedClass(const FString& ClassName, UClass*& OutClass)
{
	if (ClassCacheValid.Contains(ClassName))
	{
		OutClass = ClassCache.FindRef(ClassName);
		HitCount++;
		return true;
	}
	MissCount++;
	return false;
}

void FPreValidationCache::CacheClass(const FString& ClassName, UClass* Class)
{
	ClassCache.Add(ClassName, Class);
	ClassCacheValid.Add(ClassName);
}

bool FPreValidationCache::CheckAssetExists(const FString& AssetPath, bool& bOutExists)
{
	if (const bool* Found = AssetCache.Find(AssetPath))
	{
		bOutExists = *Found;
		HitCount++;
		return true;
	}
	MissCount++;
	return false;
}

void FPreValidationCache::CacheAssetResult(const FString& AssetPath, bool bExists)
{
	AssetCache.Add(AssetPath, bExists);
}

bool FPreValidationCache::CheckTagExists(const FString& TagName, bool& bOutExists)
{
	if (const bool* Found = TagCache.Find(TagName))
	{
		bOutExists = *Found;
		HitCount++;
		return true;
	}
	MissCount++;
	return false;
}

void FPreValidationCache::CacheTagResult(const FString& TagName, bool bExists)
{
	TagCache.Add(TagName, bExists);
}

void FPreValidationCache::Clear()
{
	FunctionCache.Empty();
	AttributeCache.Empty();
	ClassCache.Empty();
	ClassCacheValid.Empty();
	AssetCache.Empty();
	TagCache.Empty();
	HitCount = 0;
	MissCount = 0;
}

// ============================================================================
// FPreValidator Implementation
// ============================================================================

// R5: Default AttributeSet class - NO global scan
FString FPreValidator::DefaultAttributeSetClass = TEXT("UNarrativeAttributeSetBase");

FString FPreValidator::GetDefaultAttributeSetClass()
{
	return DefaultAttributeSetClass;
}

void FPreValidator::SetDefaultAttributeSetClass(const FString& ClassName)
{
	DefaultAttributeSetClass = ClassName;
}

FPreValidationReport FPreValidator::Validate(const FManifestData& Data, const FString& ManifestPath)
{
	FPreValidationReport Report;
	FPreValidationCache Cache;

	UE_LOG(LogPreValidator, Log, TEXT("Starting pre-validation for manifest: %s"), *ManifestPath);

	// Run all validation passes
	ValidateClasses(Data, Report, Cache, ManifestPath);
	ValidateFunctions(Data, Report, Cache, ManifestPath);
	ValidateAttributes(Data, Report, Cache, ManifestPath);
	ValidateAssetReferences(Data, Report, Cache, ManifestPath);
	ValidateTags(Data, Report, Cache, ManifestPath);
	ValidateTokens(Data, Report, Cache, ManifestPath);

	// Update caching stats
	Report.TotalChecks = Cache.GetHitCount() + Cache.GetMissCount();
	Report.CacheHits = Cache.GetHitCount();

	UE_LOG(LogPreValidator, Log, TEXT("Pre-validation complete: %s"), *Report.GetSummary());

	return Report;
}

// ============================================================================
// Helper Functions
// ============================================================================

UClass* FPreValidator::FindClassByName(const FString& ClassName, FPreValidationCache& Cache)
{
	if (ClassName.IsEmpty())
	{
		return nullptr;
	}

	// Check cache first
	UClass* CachedClass = nullptr;
	if (Cache.GetCachedClass(ClassName, CachedClass))
	{
		return CachedClass;
	}

	UClass* FoundClass = nullptr;

	// Try common patterns for class lookup
	// 1. Try as-is (might be full path)
	FoundClass = FindObject<UClass>(nullptr, *ClassName);

	// 2. Try with /Script/Engine prefix
	if (!FoundClass)
	{
		FString EnginePath = FString::Printf(TEXT("/Script/Engine.%s"), *ClassName);
		FoundClass = FindObject<UClass>(nullptr, *EnginePath);
	}

	// 3. Try with /Script/CoreUObject prefix
	if (!FoundClass)
	{
		FString CorePath = FString::Printf(TEXT("/Script/CoreUObject.%s"), *CorePath);
		FoundClass = FindObject<UClass>(nullptr, *CorePath);
	}

	// 4. Try with /Script/NarrativeArsenal prefix (NarrativePro)
	if (!FoundClass)
	{
		FString NPPath = FString::Printf(TEXT("/Script/NarrativeArsenal.%s"), *ClassName);
		FoundClass = FindObject<UClass>(nullptr, *NPPath);
	}

	// 5. Try with /Script/GameplayAbilities prefix
	if (!FoundClass)
	{
		FString GASPath = FString::Printf(TEXT("/Script/GameplayAbilities.%s"), *ClassName);
		FoundClass = FindObject<UClass>(nullptr, *GASPath);
	}

	// 6. Try with U prefix if not present
	if (!FoundClass && !ClassName.StartsWith(TEXT("U")) && !ClassName.StartsWith(TEXT("A")))
	{
		FString WithU = FString::Printf(TEXT("U%s"), *ClassName);
		FoundClass = FindClassByName(WithU, Cache);
		if (FoundClass)
		{
			// Don't double-cache
			return FoundClass;
		}
	}

	// 7. Try loading the class
	if (!FoundClass)
	{
		FoundClass = LoadClass<UObject>(nullptr, *ClassName);
	}

	// Cache result (even if nullptr)
	Cache.CacheClass(ClassName, FoundClass);
	return FoundClass;
}

bool FPreValidator::FunctionExistsOnClass(UClass* Class, const FString& FunctionName)
{
	if (!Class || FunctionName.IsEmpty())
	{
		return false;
	}

	// Try exact name first
	UFunction* Function = Class->FindFunctionByName(*FunctionName);
	if (Function)
	{
		return true;
	}

	// Try with K2_ prefix (Blueprint-callable functions)
	FString K2Name = FString::Printf(TEXT("K2_%s"), *FunctionName);
	Function = Class->FindFunctionByName(*K2Name);
	if (Function)
	{
		return true;
	}

	// Try with BP_ prefix
	FString BPName = FString::Printf(TEXT("BP_%s"), *FunctionName);
	Function = Class->FindFunctionByName(*BPName);

	return Function != nullptr;
}

bool FPreValidator::AttributeExistsOnSet(UClass* AttributeSetClass, const FString& AttributeName)
{
	if (!AttributeSetClass || AttributeName.IsEmpty())
	{
		return false;
	}

	// Look for FGameplayAttributeData property with the given name
	FProperty* Property = AttributeSetClass->FindPropertyByName(*AttributeName);
	if (Property)
	{
		// Verify it's an FGameplayAttributeData
		if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
		{
			static UScriptStruct* AttributeDataStruct = nullptr;
			if (!AttributeDataStruct)
			{
				AttributeDataStruct = FindObject<UScriptStruct>(nullptr, TEXT("/Script/GameplayAbilities.GameplayAttributeData"));
			}
			if (AttributeDataStruct && StructProp->Struct == AttributeDataStruct)
			{
				return true;
			}
		}
	}

	return false;
}

// R3: AssetRegistry-only implementation - NO TryLoad
bool FPreValidator::AssetExistsInRegistry(const FString& AssetPath)
{
	if (AssetPath.IsEmpty())
	{
		return false;
	}

	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	FAssetData AssetData = Registry.GetAssetByObjectPath(FSoftObjectPath(AssetPath));
	return AssetData.IsValid();
}

bool FPreValidator::TagIsRegistered(const FString& TagName)
{
	if (TagName.IsEmpty())
	{
		return false;
	}

	FGameplayTag Tag = UGameplayTagsManager::Get().RequestGameplayTag(FName(*TagName), /*ErrorIfNotFound=*/false);
	return Tag.IsValid();
}

// ============================================================================
// Validation Rule Implementations
// ============================================================================

void FPreValidator::ValidateClasses(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath)
{
	// Validate parent classes for GameplayAbilities
	for (int32 i = 0; i < Data.GameplayAbilities.Num(); i++)
	{
		const auto& GA = Data.GameplayAbilities[i];
		if (!GA.ParentClass.IsEmpty())
		{
			UClass* ParentClass = FindClassByName(GA.ParentClass, Cache);
			if (!ParentClass)
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("C2");
				Issue.ErrorCode = TEXT("E_PREVAL_CLASS_NOT_FOUND");
				Issue.Severity = EValidationSeverity::Error;
				Issue.Message = FString::Printf(TEXT("Parent class '%s' not found"), *GA.ParentClass);
				Issue.ItemId = GA.Name;
				Issue.YAMLPath = FString::Printf(TEXT("gameplay_abilities[%d].parent_class"), i);
				Issue.ManifestPath = ManifestPath;
				Issue.AttemptedClass = GA.ParentClass;
				Report.AddIssue(Issue);
			}
		}
	}

	// Validate parent classes for ActorBlueprints
	for (int32 i = 0; i < Data.ActorBlueprints.Num(); i++)
	{
		const auto& BP = Data.ActorBlueprints[i];
		if (!BP.ParentClass.IsEmpty())
		{
			UClass* ParentClass = FindClassByName(BP.ParentClass, Cache);
			if (!ParentClass)
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("C2");
				Issue.ErrorCode = TEXT("E_PREVAL_CLASS_NOT_FOUND");
				Issue.Severity = EValidationSeverity::Error;
				Issue.Message = FString::Printf(TEXT("Parent class '%s' not found"), *BP.ParentClass);
				Issue.ItemId = BP.Name;
				Issue.YAMLPath = FString::Printf(TEXT("actor_blueprints[%d].parent_class"), i);
				Issue.ManifestPath = ManifestPath;
				Issue.AttemptedClass = BP.ParentClass;
				Report.AddIssue(Issue);
			}
		}
	}

	// Validate parent classes for WidgetBlueprints
	for (int32 i = 0; i < Data.WidgetBlueprints.Num(); i++)
	{
		const auto& WBP = Data.WidgetBlueprints[i];
		if (!WBP.ParentClass.IsEmpty())
		{
			UClass* ParentClass = FindClassByName(WBP.ParentClass, Cache);
			if (!ParentClass)
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("C2");
				Issue.ErrorCode = TEXT("E_PREVAL_CLASS_NOT_FOUND");
				Issue.Severity = EValidationSeverity::Error;
				Issue.Message = FString::Printf(TEXT("Parent class '%s' not found"), *WBP.ParentClass);
				Issue.ItemId = WBP.Name;
				Issue.YAMLPath = FString::Printf(TEXT("widget_blueprints[%d].parent_class"), i);
				Issue.ManifestPath = ManifestPath;
				Issue.AttemptedClass = WBP.ParentClass;
				Report.AddIssue(Issue);
			}
		}
	}

	// Validate parent classes for DialogueBlueprints
	for (int32 i = 0; i < Data.DialogueBlueprints.Num(); i++)
	{
		const auto& DBP = Data.DialogueBlueprints[i];
		if (!DBP.ParentClass.IsEmpty())
		{
			UClass* ParentClass = FindClassByName(DBP.ParentClass, Cache);
			if (!ParentClass)
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("C2");
				Issue.ErrorCode = TEXT("E_PREVAL_CLASS_NOT_FOUND");
				Issue.Severity = EValidationSeverity::Error;
				Issue.Message = FString::Printf(TEXT("Parent class '%s' not found"), *DBP.ParentClass);
				Issue.ItemId = DBP.Name;
				Issue.YAMLPath = FString::Printf(TEXT("dialogue_blueprints[%d].parent_class"), i);
				Issue.ManifestPath = ManifestPath;
				Issue.AttemptedClass = DBP.ParentClass;
				Report.AddIssue(Issue);
			}
		}
	}
}

void FPreValidator::ValidateFunctions(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath)
{
	// Helper lambda to validate event graph nodes
	auto ValidateEventGraphNodes = [&](const TArray<FManifestGraphNodeDefinition>& Nodes, const FString& OwnerName, const FString& YAMLPrefix)
	{
		for (int32 NodeIdx = 0; NodeIdx < Nodes.Num(); NodeIdx++)
		{
			const auto& Node = Nodes[NodeIdx];

			// Only validate CallFunction nodes
			if (Node.Type.Equals(TEXT("CallFunction"), ESearchCase::IgnoreCase))
			{
				FString ClassName = Node.Properties.FindRef(TEXT("class"));
				FString FunctionName = Node.Properties.FindRef(TEXT("function"));

				if (ClassName.IsEmpty() || FunctionName.IsEmpty())
				{
					continue;  // Let generation handle missing required fields
				}

				// Check cache first
				bool bExists = false;
				if (Cache.CheckFunctionExists(ClassName, FunctionName, bExists))
				{
					if (!bExists)
					{
						FPreValidationIssue Issue;
						Issue.RuleId = TEXT("F2");
						Issue.ErrorCode = TEXT("E_PREVAL_FUNCTION_NOT_FOUND");
						Issue.Severity = EValidationSeverity::Error;
						Issue.Message = FString::Printf(TEXT("Function '%s' not found on class '%s'"), *FunctionName, *ClassName);
						Issue.ItemId = FString::Printf(TEXT("%s.%s"), *OwnerName, *Node.Id);
						Issue.YAMLPath = FString::Printf(TEXT("%s.event_graph.nodes[%d]"), *YAMLPrefix, NodeIdx);
						Issue.ManifestPath = ManifestPath;
						Issue.AttemptedClass = ClassName;
						Issue.AttemptedMember = FunctionName;
						Report.AddIssue(Issue);
					}
					continue;
				}

				// Resolve class
				UClass* Class = FindClassByName(ClassName, Cache);
				if (!Class)
				{
					// Class not found - this is a C1 error
					FPreValidationIssue Issue;
					Issue.RuleId = TEXT("C1");
					Issue.ErrorCode = TEXT("E_PREVAL_CLASS_NOT_FOUND");
					Issue.Severity = EValidationSeverity::Error;
					Issue.Message = FString::Printf(TEXT("Class '%s' not found for function call"), *ClassName);
					Issue.ItemId = FString::Printf(TEXT("%s.%s"), *OwnerName, *Node.Id);
					Issue.YAMLPath = FString::Printf(TEXT("%s.event_graph.nodes[%d].properties.class"), *YAMLPrefix, NodeIdx);
					Issue.ManifestPath = ManifestPath;
					Issue.AttemptedClass = ClassName;
					Report.AddIssue(Issue);

					Cache.CacheFunctionResult(ClassName, FunctionName, false);
					continue;
				}

				// Check if function exists
				bool bFunctionExists = FunctionExistsOnClass(Class, FunctionName);
				Cache.CacheFunctionResult(ClassName, FunctionName, bFunctionExists);

				if (!bFunctionExists)
				{
					FPreValidationIssue Issue;
					Issue.RuleId = TEXT("F2");
					Issue.ErrorCode = TEXT("E_PREVAL_FUNCTION_NOT_FOUND");
					Issue.Severity = EValidationSeverity::Error;
					Issue.Message = FString::Printf(TEXT("Function '%s' not found on class '%s'"), *FunctionName, *ClassName);
					Issue.ItemId = FString::Printf(TEXT("%s.%s"), *OwnerName, *Node.Id);
					Issue.YAMLPath = FString::Printf(TEXT("%s.event_graph.nodes[%d]"), *YAMLPrefix, NodeIdx);
					Issue.ManifestPath = ManifestPath;
					Issue.AttemptedClass = ClassName;
					Issue.AttemptedMember = FunctionName;
					Report.AddIssue(Issue);
				}
			}
		}
	};

	// Validate GameplayAbility event graphs
	for (int32 i = 0; i < Data.GameplayAbilities.Num(); i++)
	{
		const auto& GA = Data.GameplayAbilities[i];
		ValidateEventGraphNodes(GA.EventGraphNodes, GA.Name, FString::Printf(TEXT("gameplay_abilities[%d]"), i));
	}

	// Validate ActorBlueprint event graphs
	for (int32 i = 0; i < Data.ActorBlueprints.Num(); i++)
	{
		const auto& BP = Data.ActorBlueprints[i];
		ValidateEventGraphNodes(BP.EventGraphNodes, BP.Name, FString::Printf(TEXT("actor_blueprints[%d]"), i));
	}

	// Validate WidgetBlueprint event graphs
	for (int32 i = 0; i < Data.WidgetBlueprints.Num(); i++)
	{
		const auto& WBP = Data.WidgetBlueprints[i];
		ValidateEventGraphNodes(WBP.EventGraphNodes, WBP.Name, FString::Printf(TEXT("widget_blueprints[%d]"), i));
	}
}

void FPreValidator::ValidateAttributes(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath)
{
	// R5: AttributeSet resolution (LOCKED per Phase4_Spec_Locked.md)
	// Phase 4.1 validates ONLY:
	//   1. Manifest-specified attributeSetClass (if present)
	//   2. Else: configured default (UNarrativeAttributeSetBase)
	// NO global scan of UAttributeSet subclasses
	// NO ambiguity search

	for (int32 i = 0; i < Data.GameplayEffects.Num(); i++)
	{
		const auto& GE = Data.GameplayEffects[i];

		for (int32 j = 0; j < GE.Modifiers.Num(); j++)
		{
			const auto& Mod = GE.Modifiers[j];
			if (Mod.Attribute.IsEmpty())
			{
				continue;
			}

			// R5: Use default AttributeSet class (LOCKED per Phase4_Spec_Locked.md)
			// The manifest modifier struct doesn't have per-modifier AttributeSetClass override
			// Future: could add attributeSetClass field to FManifestModifierDefinition
			FString AttributeSetClass = DefaultAttributeSetClass;

			// Check cache
			bool bExists = false;
			if (Cache.CheckAttributeExists(AttributeSetClass, Mod.Attribute, bExists))
			{
				if (!bExists)
				{
					FPreValidationIssue Issue;
					Issue.RuleId = TEXT("A2");
					Issue.ErrorCode = TEXT("E_PREVAL_ATTRIBUTE_NOT_FOUND");
					Issue.Severity = EValidationSeverity::Error;
					Issue.Message = FString::Printf(TEXT("Attribute '%s' not found on AttributeSet '%s'"), *Mod.Attribute, *AttributeSetClass);
					Issue.ItemId = GE.Name;
					Issue.YAMLPath = FString::Printf(TEXT("gameplay_effects[%d].modifiers[%d].attribute"), i, j);
					Issue.ManifestPath = ManifestPath;
					Issue.AttemptedClass = AttributeSetClass;
					Issue.AttemptedMember = Mod.Attribute;
					Report.AddIssue(Issue);
				}
				continue;
			}

			// Resolve AttributeSet class
			UClass* SetClass = FindClassByName(AttributeSetClass, Cache);
			if (!SetClass)
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("A1");
				Issue.ErrorCode = TEXT("E_PREVAL_ATTRIBUTESET_NOT_FOUND");
				Issue.Severity = EValidationSeverity::Error;
				Issue.Message = FString::Printf(TEXT("AttributeSet class '%s' not found"), *AttributeSetClass);
				Issue.ItemId = GE.Name;
				Issue.YAMLPath = FString::Printf(TEXT("gameplay_effects[%d].modifiers[%d]"), i, j);
				Issue.ManifestPath = ManifestPath;
				Issue.AttemptedClass = AttributeSetClass;
				Report.AddIssue(Issue);

				Cache.CacheAttributeResult(AttributeSetClass, Mod.Attribute, false);
				continue;
			}

			// Check if attribute exists
			bool bAttrExists = AttributeExistsOnSet(SetClass, Mod.Attribute);
			Cache.CacheAttributeResult(AttributeSetClass, Mod.Attribute, bAttrExists);

			if (!bAttrExists)
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("A2");
				Issue.ErrorCode = TEXT("E_PREVAL_ATTRIBUTE_NOT_FOUND");
				Issue.Severity = EValidationSeverity::Error;
				Issue.Message = FString::Printf(TEXT("Attribute '%s' not found on AttributeSet '%s'"), *Mod.Attribute, *AttributeSetClass);
				Issue.ItemId = GE.Name;
				Issue.YAMLPath = FString::Printf(TEXT("gameplay_effects[%d].modifiers[%d].attribute"), i, j);
				Issue.ManifestPath = ManifestPath;
				Issue.AttemptedClass = AttributeSetClass;
				Issue.AttemptedMember = Mod.Attribute;
				Report.AddIssue(Issue);
			}
		}
	}
}

void FPreValidator::ValidateAssetReferences(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath)
{
	// R3: AssetRegistry only - NO TryLoad
	// Pre-validation must not load assets, only check existence

	// R1: Validate skeleton assets for animation montages
	for (int32 i = 0; i < Data.AnimationMontages.Num(); i++)
	{
		const auto& AM = Data.AnimationMontages[i];
		if (!AM.Skeleton.IsEmpty())
		{
			bool bExists = false;
			if (!Cache.CheckAssetExists(AM.Skeleton, bExists))
			{
				bExists = AssetExistsInRegistry(AM.Skeleton);
				Cache.CacheAssetResult(AM.Skeleton, bExists);
			}

			if (!bExists)
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("R1");
				Issue.ErrorCode = TEXT("E_PREVAL_ASSET_NOT_FOUND");
				Issue.Severity = EValidationSeverity::Error;
				Issue.Message = FString::Printf(TEXT("Skeleton asset not found: %s"), *AM.Skeleton);
				Issue.ItemId = AM.Name;
				Issue.YAMLPath = FString::Printf(TEXT("animation_montages[%d].skeleton"), i);
				Issue.ManifestPath = ManifestPath;
				Issue.AttemptedMember = AM.Skeleton;
				Report.AddIssue(Issue);
			}
		}
	}

	// R3: Validate MaterialFunction assets for materials
	for (int32 i = 0; i < Data.Materials.Num(); i++)
	{
		const auto& Mat = Data.Materials[i];
		for (int32 j = 0; j < Mat.Expressions.Num(); j++)
		{
			const auto& Expr = Mat.Expressions[j];
			if (Expr.Type.Equals(TEXT("MaterialFunctionCall"), ESearchCase::IgnoreCase))
			{
				FString FunctionPath = Expr.Properties.FindRef(TEXT("function"));
				if (!FunctionPath.IsEmpty())
				{
					bool bExists = false;
					if (!Cache.CheckAssetExists(FunctionPath, bExists))
					{
						bExists = AssetExistsInRegistry(FunctionPath);
						Cache.CacheAssetResult(FunctionPath, bExists);
					}

					if (!bExists)
					{
						FPreValidationIssue Issue;
						Issue.RuleId = TEXT("R3");
						Issue.ErrorCode = TEXT("E_PREVAL_ASSET_NOT_FOUND");
						Issue.Severity = EValidationSeverity::Error;
						Issue.Message = FString::Printf(TEXT("MaterialFunction not found: %s"), *FunctionPath);
						Issue.ItemId = Mat.Name;
						Issue.YAMLPath = FString::Printf(TEXT("materials[%d].expressions[%d].properties.function"), i, j);
						Issue.ManifestPath = ManifestPath;
						Issue.AttemptedMember = FunctionPath;
						Report.AddIssue(Issue);
					}
				}
			}
		}
	}

	// R2: Validate texture assets for material instances
	for (int32 i = 0; i < Data.MaterialInstances.Num(); i++)
	{
		const auto& MIC = Data.MaterialInstances[i];
		for (int32 j = 0; j < MIC.TextureParams.Num(); j++)
		{
			const auto& TexParam = MIC.TextureParams[j];
			if (!TexParam.TexturePath.IsEmpty())
			{
				bool bExists = false;
				if (!Cache.CheckAssetExists(TexParam.TexturePath, bExists))
				{
					bExists = AssetExistsInRegistry(TexParam.TexturePath);
					Cache.CacheAssetResult(TexParam.TexturePath, bExists);
				}

				if (!bExists)
				{
					FPreValidationIssue Issue;
					Issue.RuleId = TEXT("R2");
					Issue.ErrorCode = TEXT("E_PREVAL_ASSET_NOT_FOUND");
					Issue.Severity = EValidationSeverity::Error;
					Issue.Message = FString::Printf(TEXT("Texture asset not found: %s"), *TexParam.TexturePath);
					Issue.ItemId = MIC.Name;
					Issue.YAMLPath = FString::Printf(TEXT("material_instances[%d].texture_params[%d].texture"), i, j);
					Issue.ManifestPath = ManifestPath;
					Issue.AttemptedMember = TexParam.TexturePath;
					Report.AddIssue(Issue);
				}
			}
		}
	}
}

void FPreValidator::ValidateTags(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath)
{
	// R4: Tag severity policy (LOCKED per Phase4_Spec_Locked.md)
	// - T1 (normal tag not registered): WARNING - generation proceeds
	// - T2 (SetByCaller tag missing): ERROR - blocks generation
	// DO NOT CHANGE without spec amendment

	// Helper to validate a tag
	auto ValidateTag = [&](const FString& TagName, bool bIsSetByCaller, const FString& ItemId, const FString& YAMLPath)
	{
		if (TagName.IsEmpty())
		{
			return;
		}

		bool bExists = false;
		if (!Cache.CheckTagExists(TagName, bExists))
		{
			bExists = TagIsRegistered(TagName);
			Cache.CacheTagResult(TagName, bExists);
		}

		if (!bExists)
		{
			FPreValidationIssue Issue;
			if (bIsSetByCaller)
			{
				// T2: SetByCaller MUST exist - ERROR
				Issue.RuleId = TEXT("T2");
				Issue.ErrorCode = TEXT("E_PREVAL_SETBYCALLER_NOT_FOUND");
				Issue.Severity = EValidationSeverity::Error;
				Issue.Message = FString::Printf(TEXT("SetByCaller tag not registered: %s"), *TagName);
			}
			else
			{
				// T1: Normal tag - WARNING only
				Issue.RuleId = TEXT("T1");
				Issue.ErrorCode = TEXT("E_PREVAL_TAG_NOT_REGISTERED");
				Issue.Severity = EValidationSeverity::Warning;
				Issue.Message = FString::Printf(TEXT("GameplayTag not registered: %s"), *TagName);
			}
			Issue.ItemId = ItemId;
			Issue.YAMLPath = YAMLPath;
			Issue.ManifestPath = ManifestPath;
			Issue.AttemptedMember = TagName;
			Report.AddIssue(Issue);
		}
	};

	// Validate tags in GameplayEffects (SetByCaller magnitudes)
	for (int32 i = 0; i < Data.GameplayEffects.Num(); i++)
	{
		const auto& GE = Data.GameplayEffects[i];
		for (int32 j = 0; j < GE.Modifiers.Num(); j++)
		{
			const auto& Mod = GE.Modifiers[j];
			if (!Mod.SetByCallerTag.IsEmpty())
			{
				ValidateTag(Mod.SetByCallerTag, true, GE.Name,
					FString::Printf(TEXT("gameplay_effects[%d].modifiers[%d].set_by_caller_tag"), i, j));
			}
		}
	}

	// Validate ability tags in GameplayAbilities
	// Tags are nested in GA.Tags struct (FManifestAbilityTagsDefinition)
	for (int32 i = 0; i < Data.GameplayAbilities.Num(); i++)
	{
		const auto& GA = Data.GameplayAbilities[i];
		for (int32 j = 0; j < GA.Tags.AbilityTags.Num(); j++)
		{
			ValidateTag(GA.Tags.AbilityTags[j], false, GA.Name,
				FString::Printf(TEXT("gameplay_abilities[%d].tags.ability_tags[%d]"), i, j));
		}
		for (int32 j = 0; j < GA.Tags.CancelAbilitiesWithTag.Num(); j++)
		{
			ValidateTag(GA.Tags.CancelAbilitiesWithTag[j], false, GA.Name,
				FString::Printf(TEXT("gameplay_abilities[%d].tags.cancel_abilities_with_tag[%d]"), i, j));
		}
		for (int32 j = 0; j < GA.Tags.ActivationOwnedTags.Num(); j++)
		{
			ValidateTag(GA.Tags.ActivationOwnedTags[j], false, GA.Name,
				FString::Printf(TEXT("gameplay_abilities[%d].tags.activation_owned_tags[%d]"), i, j));
		}
		for (int32 j = 0; j < GA.Tags.ActivationRequiredTags.Num(); j++)
		{
			ValidateTag(GA.Tags.ActivationRequiredTags[j], false, GA.Name,
				FString::Printf(TEXT("gameplay_abilities[%d].tags.activation_required_tags[%d]"), i, j));
		}
		for (int32 j = 0; j < GA.Tags.ActivationBlockedTags.Num(); j++)
		{
			ValidateTag(GA.Tags.ActivationBlockedTags[j], false, GA.Name,
				FString::Printf(TEXT("gameplay_abilities[%d].tags.activation_blocked_tags[%d]"), i, j));
		}
	}
}

void FPreValidator::ValidateTokens(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath)
{
	// K1, K2: Token validation
	// Currently a placeholder - token validation is specific to dialogue system
	// Will be implemented when dialogue token registry is integrated

	// For now, no token validation (tokens are validated during dialogue generation)
}
