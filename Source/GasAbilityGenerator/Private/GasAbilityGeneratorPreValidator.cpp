// GasAbilityGenerator v4.29 - Phase 4.1 Pre-Validation System with Function Resolution Parity
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#include "GasAbilityGeneratorPreValidator.h"
#include "Locked/GasAbilityGeneratorTypes.h"
#include "GasAbilityGeneratorFunctionResolver.h"  // v4.29: Shared function resolver for parity
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

	// v4.28.1: Diagnostic - prove/disprove tag loading timing hypothesis
	// Query GameplayTagsManager for specific tags to determine if they're loaded at pre-validation time
	{
		UGameplayTagsManager& TagManager = UGameplayTagsManager::Get();

		// Test tags that fail pre-validation
		FGameplayTag TagAlive = TagManager.RequestGameplayTag(FName(TEXT("Father.State.Alive")), false);
		FGameplayTag TagFullyCharged = TagManager.RequestGameplayTag(FName(TEXT("Father.Dome.FullyCharged")), false);

		// Control tags - other Father.* tags from same INI that should work if loading is correct
		FGameplayTag TagAttached = TagManager.RequestGameplayTag(FName(TEXT("Father.State.Attached")), false);
		FGameplayTag TagCrawler = TagManager.RequestGameplayTag(FName(TEXT("Father.Form.Crawler")), false);
		FGameplayTag TagAbility = TagManager.RequestGameplayTag(FName(TEXT("Ability.Father.Attack")), false);

		// Engine tag that definitely exists (control)
		FGameplayTag TagState = TagManager.RequestGameplayTag(FName(TEXT("State.Invulnerable")), false);

		UE_LOG(LogPreValidator, Warning, TEXT("[DIAG] Tag loading state at pre-validation:"));
		UE_LOG(LogPreValidator, Warning, TEXT("[DIAG]   Father.State.Alive: %s"), TagAlive.IsValid() ? TEXT("FOUND") : TEXT("NOT FOUND"));
		UE_LOG(LogPreValidator, Warning, TEXT("[DIAG]   Father.Dome.FullyCharged: %s"), TagFullyCharged.IsValid() ? TEXT("FOUND") : TEXT("NOT FOUND"));
		UE_LOG(LogPreValidator, Warning, TEXT("[DIAG]   Father.State.Attached (control): %s"), TagAttached.IsValid() ? TEXT("FOUND") : TEXT("NOT FOUND"));
		UE_LOG(LogPreValidator, Warning, TEXT("[DIAG]   Father.Form.Crawler (control): %s"), TagCrawler.IsValid() ? TEXT("FOUND") : TEXT("NOT FOUND"));
		UE_LOG(LogPreValidator, Warning, TEXT("[DIAG]   Ability.Father.Attack (control): %s"), TagAbility.IsValid() ? TEXT("FOUND") : TEXT("NOT FOUND"));
		UE_LOG(LogPreValidator, Warning, TEXT("[DIAG]   State.Invulnerable (engine): %s"), TagState.IsValid() ? TEXT("FOUND") : TEXT("NOT FOUND"));
	}

	// Run all validation passes
	ValidateClasses(Data, Report, Cache, ManifestPath);
	ValidateFunctions(Data, Report, Cache, ManifestPath);
	ValidateAttributes(Data, Report, Cache, ManifestPath);
	ValidateAssetReferences(Data, Report, Cache, ManifestPath);
	ValidateTags(Data, Report, Cache, ManifestPath);
	ValidateTokens(Data, Report, Cache, ManifestPath);
	ValidateConnections(Data, Report, Cache, ManifestPath);  // v4.40.2: N2 rule - node ID and pin validation
	// v4.40.3: Extended pre-validation
	ValidateWidgetTree(Data, Report, Cache, ManifestPath);
	ValidateDialogueTree(Data, Report, Cache, ManifestPath);
	ValidateQuestStateMachine(Data, Report, Cache, ManifestPath);
	ValidateBehaviorTreeNodes(Data, Report, Cache, ManifestPath);
	ValidateNPCReferences(Data, Report, Cache, ManifestPath);
	ValidateRedundantCasts(Data, Report, Cache, ManifestPath);  // v7.8.3: Type-aware redundant cast detection

	// Update caching stats
	Report.TotalChecks = Cache.GetHitCount() + Cache.GetMissCount();
	Report.CacheHits = Cache.GetHitCount();

	UE_LOG(LogPreValidator, Log, TEXT("Pre-validation complete: %s"), *Report.GetSummary());

	return Report;
}

// ============================================================================
// Helper Functions
// ============================================================================

// v4.24.2: Get script-safe class name by stripping U/A prefix
// UE5 /Script paths use class names WITHOUT the U/A prefix
// e.g., UNarrativeAttributeSetBase -> /Script/NarrativeArsenal.NarrativeAttributeSetBase
// Safety: Only strip if second char is uppercase (UE naming convention)
// e.g., UUserWidget -> UserWidget (strip U), but UserWidget stays UserWidget (don't strip)
// v4.40.4: Don't strip 'A' from AI-prefixed classes (AIController, AIPerceptionComponent, etc.)
static FString GetScriptClassName(const FString& ClassName)
{
	// Safety: skip if already qualified or contains module separator
	if (ClassName.StartsWith(TEXT("/Script/")) ||
		ClassName.StartsWith(TEXT("/Game/")) ||
		ClassName.Contains(TEXT(".")))
	{
		return ClassName;
	}

	// Safety: skip if too short (need at least 2 chars to check convention)
	if (ClassName.Len() <= 1)
	{
		return ClassName;
	}

	// v4.40.4: Don't strip 'A' prefix from AI-prefixed classes
	// These are actual class names, not Actor prefix patterns
	// AIController, AIPerceptionComponent, AIPerceptionSystem, etc.
	if (ClassName.StartsWith(TEXT("AI"), ESearchCase::CaseSensitive))
	{
		return ClassName;
	}

	// Only strip U/A prefix if second char is uppercase (UE class naming convention)
	// UClassName -> ClassName (strip)
	// AActorName -> ActorName (strip)
	// UserWidget -> UserWidget (keep - 's' is lowercase)
	// Actor -> Actor (keep - 'c' is lowercase)
	const TCHAR SecondChar = ClassName[1];
	const bool bSecondCharIsUpper = FChar::IsUpper(SecondChar);

	if (bSecondCharIsUpper && (ClassName.StartsWith(TEXT("U")) || ClassName.StartsWith(TEXT("A"))))
	{
		return ClassName.RightChop(1);
	}

	return ClassName;
}

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

	// v4.24.2: Get normalized class name for /Script paths (strip U/A prefix)
	const FString ScriptName = GetScriptClassName(ClassName);

	// Try common patterns for class lookup
	// 1. Try as-is (might be full path)
	FoundClass = FindObject<UClass>(nullptr, *ClassName);

	// 2. Try with /Script/Engine prefix (v4.24.2: use normalized name)
	if (!FoundClass)
	{
		FString EnginePath = FString::Printf(TEXT("/Script/Engine.%s"), *ScriptName);
		FoundClass = FindObject<UClass>(nullptr, *EnginePath);
	}

	// 3. Try with /Script/CoreUObject prefix (v4.24.2: use normalized name)
	if (!FoundClass)
	{
		FString CorePath = FString::Printf(TEXT("/Script/CoreUObject.%s"), *ScriptName);
		FoundClass = FindObject<UClass>(nullptr, *CorePath);
	}

	// 4. Try with /Script/NarrativeArsenal prefix (v4.24.2: use normalized name)
	// v4.25: Use StaticLoadClass for plugin modules that may not be loaded in headless mode
	if (!FoundClass)
	{
		FString NPPath = FString::Printf(TEXT("/Script/NarrativeArsenal.%s"), *ScriptName);
		FoundClass = StaticLoadClass(UObject::StaticClass(), nullptr, *NPPath, nullptr, LOAD_None, nullptr);
	}

	// 5. Try with /Script/GameplayAbilities prefix (v4.24.2: use normalized name)
	if (!FoundClass)
	{
		FString GASPath = FString::Printf(TEXT("/Script/GameplayAbilities.%s"), *ScriptName);
		FoundClass = FindObject<UClass>(nullptr, *GASPath);
	}

	// 6. Try with /Script/GameplayTags prefix (v4.24.2: use normalized name)
	if (!FoundClass)
	{
		FString TagsPath = FString::Printf(TEXT("/Script/GameplayTags.%s"), *ScriptName);
		FoundClass = FindObject<UClass>(nullptr, *TagsPath);
	}

	// 7. Try with /Script/Niagara prefix (v4.24.2: use normalized name)
	if (!FoundClass)
	{
		FString NiagaraPath = FString::Printf(TEXT("/Script/Niagara.%s"), *ScriptName);
		FoundClass = FindObject<UClass>(nullptr, *NiagaraPath);
	}

	// 8. Try with /Script/UMG prefix (v4.24.2: use normalized name)
	if (!FoundClass)
	{
		FString UMGPath = FString::Printf(TEXT("/Script/UMG.%s"), *ScriptName);
		FoundClass = FindObject<UClass>(nullptr, *UMGPath);
	}

	// 9. Try with /Script/AIModule prefix (v4.24.2: use normalized name)
	// v4.40.4: Use StaticLoadClass to handle headless mode
	if (!FoundClass)
	{
		FString AIPath = FString::Printf(TEXT("/Script/AIModule.%s"), *ScriptName);
		FoundClass = StaticLoadClass(UObject::StaticClass(), nullptr, *AIPath, nullptr, LOAD_None, nullptr);
	}

	// v4.40.4: Try with A prefix for Actor classes (AActor, ACharacter, AAIController, etc.)
	if (!FoundClass && !ClassName.StartsWith(TEXT("A")) && !ClassName.StartsWith(TEXT("U")))
	{
		FString WithA = FString::Printf(TEXT("A%s"), *ClassName);
		FoundClass = FindClassByName(WithA, Cache);
		if (FoundClass)
		{
			return FoundClass;
		}
	}

	// 10. Try with U prefix if not present
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

	// 11. Try loading the class
	if (!FoundClass)
	{
		FoundClass = LoadClass<UObject>(nullptr, *ClassName);
	}

	// v4.24.2: Diagnostic logging for failed resolution (shows both raw and normalized names)
	if (!FoundClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PRE-VAL] Class resolution failed for '%s' (script name: '%s'). Attempted paths:"), *ClassName, *ScriptName);
		UE_LOG(LogTemp, Warning, TEXT("  - %s (as-is)"), *ClassName);
		UE_LOG(LogTemp, Warning, TEXT("  - /Script/Engine.%s"), *ScriptName);
		UE_LOG(LogTemp, Warning, TEXT("  - /Script/CoreUObject.%s"), *ScriptName);
		UE_LOG(LogTemp, Warning, TEXT("  - /Script/NarrativeArsenal.%s"), *ScriptName);
		UE_LOG(LogTemp, Warning, TEXT("  - /Script/GameplayAbilities.%s"), *ScriptName);
		UE_LOG(LogTemp, Warning, TEXT("  - /Script/GameplayTags.%s"), *ScriptName);
		UE_LOG(LogTemp, Warning, TEXT("  - /Script/Niagara.%s"), *ScriptName);
		UE_LOG(LogTemp, Warning, TEXT("  - /Script/UMG.%s"), *ScriptName);
		UE_LOG(LogTemp, Warning, TEXT("  - /Script/AIModule.%s"), *ScriptName);
		UE_LOG(LogTemp, Warning, TEXT("  - LoadClass<%s> (raw)"), *ClassName);
	}

	// Cache result (even if nullptr)
	Cache.CacheClass(ClassName, FoundClass);
	return FoundClass;
}

bool FPreValidator::FunctionExistsOnClass(UClass* Class, const FString& FunctionName)
{
	// v4.29: Use shared function resolver for parity with Generator
	// Contract: "PreValidator function resolution behavior must be identical to Generator
	// function resolution behavior" - PreValidator_Generator_Parity_Audit_v1.md
	//
	// Old behavior: Tried variants (K2_, BP_) on ANY class
	// New behavior: Uses FGasAbilityGeneratorFunctionResolver::FunctionExists()
	//   which only tries variants in WellKnownFunctions probe
	//
	// Note: We pass the explicit class name. The resolver will:
	//   1. Try WellKnownFunctions with variants (may find elsewhere)
	//   2. Try explicit class with exact name only
	//   3. Try library fallback with exact name only

	if (!Class || FunctionName.IsEmpty())
	{
		return false;
	}

	// Use shared resolver - pass Class name as explicit class
	// ParentClass=nullptr and bTargetSelf=false since we're validating explicit class: nodes
	return FGasAbilityGeneratorFunctionResolver::FunctionExists(
		FunctionName,
		Class->GetName(),
		nullptr,  // No parent class context in current validation flow
		false     // Not target_self
	);
}

bool FPreValidator::AttributeExistsOnSet(UClass* AttributeSetClass, const FString& AttributeName)
{
	if (!AttributeSetClass || AttributeName.IsEmpty())
	{
		return false;
	}

	// Parse attribute name - format is "SetClass.AttributeName" or just "AttributeName"
	FString ParsedAttributeName = AttributeName;
	int32 DotIndex;
	if (AttributeName.FindLastChar('.', DotIndex))
	{
		// Extract just the attribute name after the last dot
		ParsedAttributeName = AttributeName.Mid(DotIndex + 1);
	}

	// Look for FGameplayAttributeData property with the given name
	FProperty* Property = AttributeSetClass->FindPropertyByName(*ParsedAttributeName);
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

				// v4.40.1: Check if class is an in-manifest Blueprint (will be generated during this run)
				// Skip validation for these - function will be resolved during generation phase
				bool bIsInManifestBlueprint = false;
				for (const auto& BP : Data.ActorBlueprints)
				{
					if (ClassName.Equals(BP.Name, ESearchCase::IgnoreCase))
					{
						bIsInManifestBlueprint = true;
						break;
					}
				}
				if (!bIsInManifestBlueprint)
				{
					for (const auto& GA : Data.GameplayAbilities)
					{
						if (ClassName.Equals(GA.Name, ESearchCase::IgnoreCase))
						{
							bIsInManifestBlueprint = true;
							break;
						}
					}
				}
				if (!bIsInManifestBlueprint)
				{
					for (const auto& WBP : Data.WidgetBlueprints)
					{
						if (ClassName.Equals(WBP.Name, ESearchCase::IgnoreCase))
						{
							bIsInManifestBlueprint = true;
							break;
						}
					}
				}

				if (bIsInManifestBlueprint)
				{
					// Skip validation - class will be generated before this Blueprint
					UE_LOG(LogTemp, Display, TEXT("[PRE-VAL] Deferring validation: %s.%s calls %s.%s (in-manifest Blueprint)"),
						*OwnerName, *Node.Id, *ClassName, *FunctionName);
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

// v4.40.2: Validate event graph connections (N2 rule)
// Checks that connection endpoints reference existing nodes
void FPreValidator::ValidateConnections(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath)
{
	// Helper lambda to validate connections in an event graph
	auto ValidateEventGraphConnections = [&](const TArray<FManifestGraphNodeDefinition>& Nodes,
	                                         const TArray<FManifestGraphConnectionDefinition>& Connections,
	                                         const FString& OwnerName, const FString& YAMLPrefix)
	{
		// Build a set of node IDs for quick lookup
		TSet<FString> NodeIds;
		for (const auto& Node : Nodes)
		{
			NodeIds.Add(Node.Id);
		}

		for (int32 ConnIdx = 0; ConnIdx < Connections.Num(); ConnIdx++)
		{
			const auto& Conn = Connections[ConnIdx];

			// Check that From node exists
			if (!NodeIds.Contains(Conn.From.NodeId))
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("N2");
				Issue.ErrorCode = TEXT("E_PREVAL_CONNECTION_FROM_NOT_FOUND");
				Issue.Severity = EValidationSeverity::Error;
				Issue.Message = FString::Printf(TEXT("Connection references non-existent source node '%s'"), *Conn.From.NodeId);
				Issue.ItemId = FString::Printf(TEXT("%s.connection[%d]"), *OwnerName, ConnIdx);
				Issue.YAMLPath = FString::Printf(TEXT("%s.event_graph.connections[%d].from"), *YAMLPrefix, ConnIdx);
				Issue.ManifestPath = ManifestPath;
				Report.AddIssue(Issue);
			}

			// Check that To node exists
			if (!NodeIds.Contains(Conn.To.NodeId))
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("N2");
				Issue.ErrorCode = TEXT("E_PREVAL_CONNECTION_TO_NOT_FOUND");
				Issue.Severity = EValidationSeverity::Error;
				Issue.Message = FString::Printf(TEXT("Connection references non-existent target node '%s'"), *Conn.To.NodeId);
				Issue.ItemId = FString::Printf(TEXT("%s.connection[%d]"), *OwnerName, ConnIdx);
				Issue.YAMLPath = FString::Printf(TEXT("%s.event_graph.connections[%d].to"), *YAMLPrefix, ConnIdx);
				Issue.ManifestPath = ManifestPath;
				Report.AddIssue(Issue);
			}

			// Check that pin names are not empty
			if (Conn.From.PinName.IsEmpty())
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("N2");
				Issue.ErrorCode = TEXT("E_PREVAL_CONNECTION_PIN_EMPTY");
				Issue.Severity = EValidationSeverity::Error;
				Issue.Message = FString::Printf(TEXT("Connection from '%s' has empty source pin name"), *Conn.From.NodeId);
				Issue.ItemId = FString::Printf(TEXT("%s.connection[%d]"), *OwnerName, ConnIdx);
				Issue.YAMLPath = FString::Printf(TEXT("%s.event_graph.connections[%d].from[1]"), *YAMLPrefix, ConnIdx);
				Issue.ManifestPath = ManifestPath;
				Report.AddIssue(Issue);
			}

			if (Conn.To.PinName.IsEmpty())
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("N2");
				Issue.ErrorCode = TEXT("E_PREVAL_CONNECTION_PIN_EMPTY");
				Issue.Severity = EValidationSeverity::Error;
				Issue.Message = FString::Printf(TEXT("Connection to '%s' has empty target pin name"), *Conn.To.NodeId);
				Issue.ItemId = FString::Printf(TEXT("%s.connection[%d]"), *OwnerName, ConnIdx);
				Issue.YAMLPath = FString::Printf(TEXT("%s.event_graph.connections[%d].to[1]"), *YAMLPrefix, ConnIdx);
				Issue.ManifestPath = ManifestPath;
				Report.AddIssue(Issue);
			}
		}
	};

	// Validate GameplayAbility connections
	for (int32 i = 0; i < Data.GameplayAbilities.Num(); i++)
	{
		const auto& GA = Data.GameplayAbilities[i];
		ValidateEventGraphConnections(GA.EventGraphNodes, GA.EventGraphConnections, GA.Name, FString::Printf(TEXT("gameplay_abilities[%d]"), i));
	}

	// Validate ActorBlueprint connections
	for (int32 i = 0; i < Data.ActorBlueprints.Num(); i++)
	{
		const auto& BP = Data.ActorBlueprints[i];
		ValidateEventGraphConnections(BP.EventGraphNodes, BP.EventGraphConnections, BP.Name, FString::Printf(TEXT("actor_blueprints[%d]"), i));
	}

	// Validate WidgetBlueprint connections
	for (int32 i = 0; i < Data.WidgetBlueprints.Num(); i++)
	{
		const auto& WBP = Data.WidgetBlueprints[i];
		ValidateEventGraphConnections(WBP.EventGraphNodes, WBP.EventGraphConnections, WBP.Name, FString::Printf(TEXT("widget_blueprints[%d]"), i));
	}

	// v4.40.3: N1 - Validate node required inputs at pre-validation time
	// Known node types with required inputs
	auto ValidateNodeRequiredInputs = [&](const TArray<FManifestGraphNodeDefinition>& Nodes,
	                                      const TArray<FManifestGraphConnectionDefinition>& Connections,
	                                      const FString& OwnerName, const FString& YAMLPrefix)
	{
		// Build map of connections TO each node's pins
		TMap<FString, TSet<FString>> NodePinConnections; // NodeId -> connected pin names
		for (const auto& Conn : Connections)
		{
			FString Key = Conn.To.NodeId;
			if (!NodePinConnections.Contains(Key))
			{
				NodePinConnections.Add(Key, TSet<FString>());
			}
			NodePinConnections[Key].Add(Conn.To.PinName.ToLower());
		}

		// Check required inputs for known node types
		for (int32 j = 0; j < Nodes.Num(); j++)
		{
			const auto& Node = Nodes[j];
			TSet<FString>* ConnectedPins = NodePinConnections.Find(Node.Id);

			// Branch requires Condition
			if (Node.Type.Equals(TEXT("Branch"), ESearchCase::IgnoreCase))
			{
				if (!ConnectedPins || !ConnectedPins->Contains(TEXT("condition")))
				{
					FPreValidationIssue Issue;
					Issue.RuleId = TEXT("N1");
					Issue.ErrorCode = TEXT("E_PREVAL_NODE_MISSING_INPUT");
					Issue.Severity = EValidationSeverity::Warning;
					Issue.Message = FString::Printf(TEXT("Branch node '%s' has no connection to Condition pin"), *Node.Id);
					Issue.ItemId = OwnerName;
					Issue.YAMLPath = FString::Printf(TEXT("%s.event_graph.nodes[%d]"), *YAMLPrefix, j);
					Issue.ManifestPath = ManifestPath;
					Report.AddIssue(Issue);
				}
			}

			// VariableSet requires Value (unless object type which can be cleared)
			// v4.40.4: Also check for variable_name as pin name (Blueprint uses variable name as pin name)
			if (Node.Type.Equals(TEXT("VariableSet"), ESearchCase::IgnoreCase))
			{
				bool bHasValueConnection = false;
				if (ConnectedPins)
				{
					// Check for "value" pin (generic name)
					if (ConnectedPins->Contains(TEXT("value")))
					{
						bHasValueConnection = true;
					}
					// Check for node ID as pin name
					else if (ConnectedPins->Contains(Node.Id.ToLower()))
					{
						bHasValueConnection = true;
					}
					// v4.40.4: Check for variable_name from properties (Blueprint pin name = variable name)
					else
					{
						const FString* VariableName = Node.Properties.Find(TEXT("variable_name"));
						if (VariableName && ConnectedPins->Contains(VariableName->ToLower()))
						{
							bHasValueConnection = true;
						}
					}
				}

				if (!bHasValueConnection)
				{
					FPreValidationIssue Issue;
					Issue.RuleId = TEXT("N1");
					Issue.ErrorCode = TEXT("E_PREVAL_NODE_MISSING_INPUT");
					Issue.Severity = EValidationSeverity::Warning;
					Issue.Message = FString::Printf(TEXT("VariableSet node '%s' has no connection to Value pin - variable will be set to default"), *Node.Id);
					Issue.ItemId = OwnerName;
					Issue.YAMLPath = FString::Printf(TEXT("%s.event_graph.nodes[%d]"), *YAMLPrefix, j);
					Issue.ManifestPath = ManifestPath;
					Report.AddIssue(Issue);
				}
			}

			// DynamicCast requires Object
			if (Node.Type.Equals(TEXT("DynamicCast"), ESearchCase::IgnoreCase))
			{
				if (!ConnectedPins || !ConnectedPins->Contains(TEXT("object")))
				{
					FPreValidationIssue Issue;
					Issue.RuleId = TEXT("N1");
					Issue.ErrorCode = TEXT("E_PREVAL_NODE_MISSING_INPUT");
					Issue.Severity = EValidationSeverity::Warning;
					Issue.Message = FString::Printf(TEXT("DynamicCast node '%s' has no connection to Object pin"), *Node.Id);
					Issue.ItemId = OwnerName;
					Issue.YAMLPath = FString::Printf(TEXT("%s.event_graph.nodes[%d]"), *YAMLPrefix, j);
					Issue.ManifestPath = ManifestPath;
					Report.AddIssue(Issue);
				}
			}

			// ForEachLoop requires Array
			if (Node.Type.Equals(TEXT("ForEachLoop"), ESearchCase::IgnoreCase))
			{
				if (!ConnectedPins || !ConnectedPins->Contains(TEXT("array")))
				{
					FPreValidationIssue Issue;
					Issue.RuleId = TEXT("N1");
					Issue.ErrorCode = TEXT("E_PREVAL_NODE_MISSING_INPUT");
					Issue.Severity = EValidationSeverity::Warning;
					Issue.Message = FString::Printf(TEXT("ForEachLoop node '%s' has no connection to Array pin"), *Node.Id);
					Issue.ItemId = OwnerName;
					Issue.YAMLPath = FString::Printf(TEXT("%s.event_graph.nodes[%d]"), *YAMLPrefix, j);
					Issue.ManifestPath = ManifestPath;
					Report.AddIssue(Issue);
				}
			}

			// SpawnActor requires Class
			if (Node.Type.Equals(TEXT("SpawnActor"), ESearchCase::IgnoreCase) ||
			    Node.Type.Equals(TEXT("SpawnActorFromClass"), ESearchCase::IgnoreCase))
			{
				if (!ConnectedPins || !ConnectedPins->Contains(TEXT("class")))
				{
					FPreValidationIssue Issue;
					Issue.RuleId = TEXT("N1");
					Issue.ErrorCode = TEXT("E_PREVAL_NODE_MISSING_INPUT");
					Issue.Severity = EValidationSeverity::Warning;
					Issue.Message = FString::Printf(TEXT("SpawnActor node '%s' has no connection to Class pin"), *Node.Id);
					Issue.ItemId = OwnerName;
					Issue.YAMLPath = FString::Printf(TEXT("%s.event_graph.nodes[%d]"), *YAMLPrefix, j);
					Issue.ManifestPath = ManifestPath;
					Report.AddIssue(Issue);
				}
			}
		}
	};

	// Run N1 validation on all event graphs
	for (int32 i = 0; i < Data.GameplayAbilities.Num(); i++)
	{
		const auto& GA = Data.GameplayAbilities[i];
		ValidateNodeRequiredInputs(GA.EventGraphNodes, GA.EventGraphConnections, GA.Name, FString::Printf(TEXT("gameplay_abilities[%d]"), i));
	}
	for (int32 i = 0; i < Data.ActorBlueprints.Num(); i++)
	{
		const auto& BP = Data.ActorBlueprints[i];
		ValidateNodeRequiredInputs(BP.EventGraphNodes, BP.EventGraphConnections, BP.Name, FString::Printf(TEXT("actor_blueprints[%d]"), i));
	}
	for (int32 i = 0; i < Data.WidgetBlueprints.Num(); i++)
	{
		const auto& WBP = Data.WidgetBlueprints[i];
		ValidateNodeRequiredInputs(WBP.EventGraphNodes, WBP.EventGraphConnections, WBP.Name, FString::Printf(TEXT("widget_blueprints[%d]"), i));
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
	// K1, K2: Token validation - v4.40.3
	// Validates dialogue tokens like {player_name}, {npc_name}, etc.

	// Known supported tokens
	static TSet<FString> KnownTokens = {
		TEXT("player_name"), TEXT("npc_name"), TEXT("speaker_name"),
		TEXT("target_name"), TEXT("item_name"), TEXT("quest_name"),
		TEXT("location_name"), TEXT("faction_name"), TEXT("time"),
		TEXT("date"), TEXT("gold"), TEXT("level"), TEXT("health"),
		TEXT("mana"), TEXT("stamina"), TEXT("experience")
	};

	// Helper to extract and validate tokens from text
	auto ValidateTextTokens = [&](const FString& Text, const FString& ItemId, const FString& YAMLPath)
	{
		// Find all {token} patterns
		int32 StartIdx = 0;
		while ((StartIdx = Text.Find(TEXT("{"), ESearchCase::IgnoreCase, ESearchDir::FromStart, StartIdx)) != INDEX_NONE)
		{
			int32 EndIdx = Text.Find(TEXT("}"), ESearchCase::IgnoreCase, ESearchDir::FromStart, StartIdx);
			if (EndIdx == INDEX_NONE) break;

			FString Token = Text.Mid(StartIdx + 1, EndIdx - StartIdx - 1);
			if (!Token.IsEmpty() && !KnownTokens.Contains(Token.ToLower()))
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("K1");
				Issue.ErrorCode = TEXT("E_PREVAL_TOKEN_UNKNOWN");
				Issue.Severity = EValidationSeverity::Warning;
				Issue.Message = FString::Printf(TEXT("Unknown dialogue token '{%s}' - may not resolve at runtime"), *Token);
				Issue.ItemId = ItemId;
				Issue.YAMLPath = YAMLPath;
				Issue.ManifestPath = ManifestPath;
				Issue.AttemptedMember = Token;
				Report.AddIssue(Issue);
			}
			StartIdx = EndIdx + 1;
		}
	};

	// Validate tokens in dialogue blueprints
	for (int32 i = 0; i < Data.DialogueBlueprints.Num(); i++)
	{
		const auto& DBP = Data.DialogueBlueprints[i];
		for (int32 j = 0; j < DBP.DialogueTree.Nodes.Num(); j++)
		{
			const auto& Node = DBP.DialogueTree.Nodes[j];
			ValidateTextTokens(Node.Text, DBP.Name, FString::Printf(TEXT("dialogue_blueprints[%d].dialogue_tree.nodes[%d].text"), i, j));
			ValidateTextTokens(Node.OptionText, DBP.Name, FString::Printf(TEXT("dialogue_blueprints[%d].dialogue_tree.nodes[%d].option_text"), i, j));
		}
	}
}

// v4.40.3: Pre-validate widget tree structure
void FPreValidator::ValidateWidgetTree(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath)
{
	// Known widget types supported by generator
	static TSet<FString> KnownWidgetTypes = {
		TEXT("CanvasPanel"), TEXT("VerticalBox"), TEXT("HorizontalBox"),
		TEXT("TextBlock"), TEXT("Image"), TEXT("Button"), TEXT("ProgressBar"),
		TEXT("Border"), TEXT("Overlay"), TEXT("SizeBox"), TEXT("Spacer"),
		TEXT("GridPanel"), TEXT("UniformGridPanel"), TEXT("WrapBox"),
		TEXT("ScrollBox"), TEXT("EditableText"), TEXT("EditableTextBox"),
		TEXT("CheckBox"), TEXT("Slider"), TEXT("SpinBox"), TEXT("ComboBox")
	};

	for (int32 i = 0; i < Data.WidgetBlueprints.Num(); i++)
	{
		const auto& WBP = Data.WidgetBlueprints[i];
		TSet<FString> WidgetIds;

		for (int32 j = 0; j < WBP.WidgetTree.Widgets.Num(); j++)
		{
			const auto& Widget = WBP.WidgetTree.Widgets[j];

			// Check for duplicate widget IDs
			if (WidgetIds.Contains(Widget.Id))
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("W1");
				Issue.ErrorCode = TEXT("E_PREVAL_WIDGET_DUPLICATE_ID");
				Issue.Severity = EValidationSeverity::Error;
				Issue.Message = FString::Printf(TEXT("Duplicate widget ID '%s'"), *Widget.Id);
				Issue.ItemId = WBP.Name;
				Issue.YAMLPath = FString::Printf(TEXT("widget_blueprints[%d].widget_tree.widgets[%d].id"), i, j);
				Issue.ManifestPath = ManifestPath;
				Report.AddIssue(Issue);
			}
			WidgetIds.Add(Widget.Id);

			// Check widget type is known
			if (!KnownWidgetTypes.Contains(Widget.Type))
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("W2");
				Issue.ErrorCode = TEXT("E_PREVAL_WIDGET_TYPE_UNKNOWN");
				Issue.Severity = EValidationSeverity::Warning;
				Issue.Message = FString::Printf(TEXT("Unknown widget type '%s' - may fail at generation"), *Widget.Type);
				Issue.ItemId = WBP.Name;
				Issue.YAMLPath = FString::Printf(TEXT("widget_blueprints[%d].widget_tree.widgets[%d].type"), i, j);
				Issue.ManifestPath = ManifestPath;
				Report.AddIssue(Issue);
			}
		}

		// Check children references exist
		for (int32 j = 0; j < WBP.WidgetTree.Widgets.Num(); j++)
		{
			const auto& Widget = WBP.WidgetTree.Widgets[j];
			for (const FString& ChildId : Widget.Children)
			{
				if (!WidgetIds.Contains(ChildId))
				{
					FPreValidationIssue Issue;
					Issue.RuleId = TEXT("W3");
					Issue.ErrorCode = TEXT("E_PREVAL_WIDGET_CHILD_NOT_FOUND");
					Issue.Severity = EValidationSeverity::Error;
					Issue.Message = FString::Printf(TEXT("Widget '%s' references non-existent child '%s'"), *Widget.Id, *ChildId);
					Issue.ItemId = WBP.Name;
					Issue.YAMLPath = FString::Printf(TEXT("widget_blueprints[%d].widget_tree.widgets[%d].children"), i, j);
					Issue.ManifestPath = ManifestPath;
					Report.AddIssue(Issue);
				}
			}
		}
	}
}

// v4.40.3: Pre-validate dialogue tree structure
void FPreValidator::ValidateDialogueTree(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath)
{
	for (int32 i = 0; i < Data.DialogueBlueprints.Num(); i++)
	{
		const auto& DBP = Data.DialogueBlueprints[i];
		if (DBP.DialogueTree.Nodes.Num() == 0) continue;

		TSet<FString> NodeIds;
		for (const auto& Node : DBP.DialogueTree.Nodes)
		{
			// Check for duplicate node IDs
			if (NodeIds.Contains(Node.Id))
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("DT1");
				Issue.ErrorCode = TEXT("E_PREVAL_DIALOGUE_DUPLICATE_ID");
				Issue.Severity = EValidationSeverity::Error;
				Issue.Message = FString::Printf(TEXT("Duplicate dialogue node ID '%s'"), *Node.Id);
				Issue.ItemId = DBP.Name;
				Issue.YAMLPath = FString::Printf(TEXT("dialogue_blueprints[%d].dialogue_tree"), i);
				Issue.ManifestPath = ManifestPath;
				Report.AddIssue(Issue);
			}
			NodeIds.Add(Node.Id);
		}

		// Check root node exists (WARNING - empty dialogue trees are valid)
		if (!DBP.DialogueTree.RootNodeId.IsEmpty() && NodeIds.Num() > 0 && !NodeIds.Contains(DBP.DialogueTree.RootNodeId))
		{
			FPreValidationIssue Issue;
			Issue.RuleId = TEXT("DT2");
			Issue.ErrorCode = TEXT("E_PREVAL_DIALOGUE_ROOT_NOT_FOUND");
			Issue.Severity = EValidationSeverity::Warning;  // v4.40.3: Downgrade to warning - empty trees are valid
			Issue.Message = FString::Printf(TEXT("Root node '%s' not found in dialogue tree"), *DBP.DialogueTree.RootNodeId);
			Issue.ItemId = DBP.Name;
			Issue.YAMLPath = FString::Printf(TEXT("dialogue_blueprints[%d].dialogue_tree.root"), i);
			Issue.ManifestPath = ManifestPath;
			Report.AddIssue(Issue);
		}

		// Check reply references exist
		for (int32 j = 0; j < DBP.DialogueTree.Nodes.Num(); j++)
		{
			const auto& Node = DBP.DialogueTree.Nodes[j];
			for (const FString& ReplyId : Node.PlayerReplies)
			{
				if (!NodeIds.Contains(ReplyId))
				{
					FPreValidationIssue Issue;
					Issue.RuleId = TEXT("DT3");
					Issue.ErrorCode = TEXT("E_PREVAL_DIALOGUE_REPLY_NOT_FOUND");
					Issue.Severity = EValidationSeverity::Error;
					Issue.Message = FString::Printf(TEXT("Node '%s' references non-existent reply '%s'"), *Node.Id, *ReplyId);
					Issue.ItemId = DBP.Name;
					Issue.YAMLPath = FString::Printf(TEXT("dialogue_blueprints[%d].dialogue_tree.nodes[%d].player_replies"), i, j);
					Issue.ManifestPath = ManifestPath;
					Report.AddIssue(Issue);
				}
			}
			for (const FString& ReplyId : Node.NPCReplies)
			{
				if (!NodeIds.Contains(ReplyId))
				{
					FPreValidationIssue Issue;
					Issue.RuleId = TEXT("DT3");
					Issue.ErrorCode = TEXT("E_PREVAL_DIALOGUE_REPLY_NOT_FOUND");
					Issue.Severity = EValidationSeverity::Error;
					Issue.Message = FString::Printf(TEXT("Node '%s' references non-existent NPC reply '%s'"), *Node.Id, *ReplyId);
					Issue.ItemId = DBP.Name;
					Issue.YAMLPath = FString::Printf(TEXT("dialogue_blueprints[%d].dialogue_tree.nodes[%d].npc_replies"), i, j);
					Issue.ManifestPath = ManifestPath;
					Report.AddIssue(Issue);
				}
			}
		}
	}
}

// v4.40.3: Pre-validate quest state machine
void FPreValidator::ValidateQuestStateMachine(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath)
{
	for (int32 i = 0; i < Data.Quests.Num(); i++)
	{
		const auto& Quest = Data.Quests[i];

		TSet<FString> StateIds;
		for (const auto& State : Quest.States)
		{
			// Check for duplicate state IDs
			if (StateIds.Contains(State.Id))
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("Q1");
				Issue.ErrorCode = TEXT("E_PREVAL_QUEST_DUPLICATE_STATE");
				Issue.Severity = EValidationSeverity::Error;
				Issue.Message = FString::Printf(TEXT("Duplicate quest state ID '%s'"), *State.Id);
				Issue.ItemId = Quest.Name;
				Issue.YAMLPath = FString::Printf(TEXT("quests[%d].states"), i);
				Issue.ManifestPath = ManifestPath;
				Report.AddIssue(Issue);
			}
			StateIds.Add(State.Id);
		}

		// Check branch references (branches are nested inside states)
		for (int32 j = 0; j < Quest.States.Num(); j++)
		{
			const auto& State = Quest.States[j];
			for (int32 k = 0; k < State.Branches.Num(); k++)
			{
				const auto& Branch = State.Branches[k];
				// Branch goes FROM current state TO DestinationState
				if (!Branch.DestinationState.IsEmpty() && !StateIds.Contains(Branch.DestinationState))
				{
					FPreValidationIssue Issue;
					Issue.RuleId = TEXT("Q2");
					Issue.ErrorCode = TEXT("E_PREVAL_QUEST_STATE_NOT_FOUND");
					Issue.Severity = EValidationSeverity::Error;
					Issue.Message = FString::Printf(TEXT("Branch '%s' in state '%s' references non-existent destination_state '%s'"),
						Branch.Id.IsEmpty() ? TEXT("(unnamed)") : *Branch.Id, *State.Id, *Branch.DestinationState);
					Issue.ItemId = Quest.Name;
					Issue.YAMLPath = FString::Printf(TEXT("quests[%d].states[%d].branches[%d].destination_state"), i, j, k);
					Issue.ManifestPath = ManifestPath;
					Report.AddIssue(Issue);
				}
			}
		}
	}
}

// v4.40.3: Pre-validate behavior tree nodes
void FPreValidator::ValidateBehaviorTreeNodes(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath)
{
	for (int32 i = 0; i < Data.BehaviorTrees.Num(); i++)
	{
		const auto& BT = Data.BehaviorTrees[i];

		TSet<FString> NodeIds;
		for (int32 j = 0; j < BT.Nodes.Num(); j++)
		{
			const auto& Node = BT.Nodes[j];

			// Check for duplicate node IDs
			if (NodeIds.Contains(Node.Id))
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("BT1");
				Issue.ErrorCode = TEXT("E_PREVAL_BT_DUPLICATE_ID");
				Issue.Severity = EValidationSeverity::Error;
				Issue.Message = FString::Printf(TEXT("Duplicate behavior tree node ID '%s'"), *Node.Id);
				Issue.ItemId = BT.Name;
				Issue.YAMLPath = FString::Printf(TEXT("behavior_trees[%d].nodes[%d]"), i, j);
				Issue.ManifestPath = ManifestPath;
				Report.AddIssue(Issue);
			}
			NodeIds.Add(Node.Id);

			// Check task class exists if specified
			if (!Node.TaskClass.IsEmpty())
			{
				UClass* TaskClass = FindClassByName(Node.TaskClass, Cache);
				if (!TaskClass)
				{
					FPreValidationIssue Issue;
					Issue.RuleId = TEXT("BT2");
					Issue.ErrorCode = TEXT("E_PREVAL_BT_TASK_NOT_FOUND");
					Issue.Severity = EValidationSeverity::Warning;
					Issue.Message = FString::Printf(TEXT("Task class '%s' not found"), *Node.TaskClass);
					Issue.ItemId = BT.Name;
					Issue.YAMLPath = FString::Printf(TEXT("behavior_trees[%d].nodes[%d].task_class"), i, j);
					Issue.ManifestPath = ManifestPath;
					Report.AddIssue(Issue);
				}
			}
		}
	}
}

// v4.40.3: Pre-validate NPC/Character references
void FPreValidator::ValidateNPCReferences(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath)
{
	// Build sets of defined assets for cross-reference checking
	TSet<FString> DefinedAbilityConfigs;
	TSet<FString> DefinedActivityConfigs;
	TSet<FString> DefinedDialogues;
	TSet<FString> DefinedSchedules;
	TSet<FString> DefinedNPCs;

	for (const auto& AC : Data.AbilityConfigurations) DefinedAbilityConfigs.Add(AC.Name);
	for (const auto& AC : Data.ActivityConfigurations) DefinedActivityConfigs.Add(AC.Name);
	for (const auto& DBP : Data.DialogueBlueprints) DefinedDialogues.Add(DBP.Name);
	for (const auto& Sched : Data.ActivitySchedules) DefinedSchedules.Add(Sched.Name);
	for (const auto& NPC : Data.NPCDefinitions) DefinedNPCs.Add(NPC.Name);

	// Validate NPC references
	for (int32 i = 0; i < Data.NPCDefinitions.Num(); i++)
	{
		const auto& NPC = Data.NPCDefinitions[i];

		// Check ability configuration reference
		if (!NPC.AbilityConfiguration.IsEmpty() && !DefinedAbilityConfigs.Contains(NPC.AbilityConfiguration))
		{
			// Check if asset exists in registry
			FString AssetPath = FString::Printf(TEXT("/Game/FatherCompanion/AbilityConfigs/%s"), *NPC.AbilityConfiguration);
			if (!AssetExistsInRegistry(AssetPath))
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("NPC1");
				Issue.ErrorCode = TEXT("E_PREVAL_NPC_AC_NOT_FOUND");
				Issue.Severity = EValidationSeverity::Warning;
				Issue.Message = FString::Printf(TEXT("AbilityConfiguration '%s' not found in manifest or asset registry"), *NPC.AbilityConfiguration);
				Issue.ItemId = NPC.Name;
				Issue.YAMLPath = FString::Printf(TEXT("npc_definitions[%d].ability_configuration"), i);
				Issue.ManifestPath = ManifestPath;
				Report.AddIssue(Issue);
			}
		}

		// Check activity configuration reference
		if (!NPC.ActivityConfiguration.IsEmpty() && !DefinedActivityConfigs.Contains(NPC.ActivityConfiguration))
		{
			FString AssetPath = FString::Printf(TEXT("/Game/FatherCompanion/ActivityConfigs/%s"), *NPC.ActivityConfiguration);
			if (!AssetExistsInRegistry(AssetPath))
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("NPC2");
				Issue.ErrorCode = TEXT("E_PREVAL_NPC_ACTC_NOT_FOUND");
				Issue.Severity = EValidationSeverity::Warning;
				Issue.Message = FString::Printf(TEXT("ActivityConfiguration '%s' not found in manifest or asset registry"), *NPC.ActivityConfiguration);
				Issue.ItemId = NPC.Name;
				Issue.YAMLPath = FString::Printf(TEXT("npc_definitions[%d].activity_configuration"), i);
				Issue.ManifestPath = ManifestPath;
				Report.AddIssue(Issue);
			}
		}

		// Check dialogue reference
		if (!NPC.Dialogue.IsEmpty() && !DefinedDialogues.Contains(NPC.Dialogue))
		{
			FString AssetPath = FString::Printf(TEXT("/Game/FatherCompanion/Dialogues/%s"), *NPC.Dialogue);
			if (!AssetExistsInRegistry(AssetPath))
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("NPC3");
				Issue.ErrorCode = TEXT("E_PREVAL_NPC_DIALOGUE_NOT_FOUND");
				Issue.Severity = EValidationSeverity::Warning;
				Issue.Message = FString::Printf(TEXT("Dialogue '%s' not found in manifest or asset registry"), *NPC.Dialogue);
				Issue.ItemId = NPC.Name;
				Issue.YAMLPath = FString::Printf(TEXT("npc_definitions[%d].dialogue"), i);
				Issue.ManifestPath = ManifestPath;
				Report.AddIssue(Issue);
			}
		}
	}

	// Validate dialogue speaker references
	for (int32 i = 0; i < Data.DialogueBlueprints.Num(); i++)
	{
		const auto& DBP = Data.DialogueBlueprints[i];
		for (int32 j = 0; j < DBP.Speakers.Num(); j++)
		{
			const auto& Speaker = DBP.Speakers[j];
			if (!Speaker.NPCDefinition.IsEmpty() && !DefinedNPCs.Contains(Speaker.NPCDefinition))
			{
				FString AssetPath = FString::Printf(TEXT("/Game/FatherCompanion/NPCs/%s"), *Speaker.NPCDefinition);
				if (!AssetExistsInRegistry(AssetPath))
				{
					FPreValidationIssue Issue;
					Issue.RuleId = TEXT("NPC4");
					Issue.ErrorCode = TEXT("E_PREVAL_SPEAKER_NPC_NOT_FOUND");
					Issue.Severity = EValidationSeverity::Warning;
					Issue.Message = FString::Printf(TEXT("Speaker NPC '%s' not found in manifest or asset registry"), *Speaker.NPCDefinition);
					Issue.ItemId = DBP.Name;
					Issue.YAMLPath = FString::Printf(TEXT("dialogue_blueprints[%d].speakers[%d].npc_definition"), i, j);
					Issue.ManifestPath = ManifestPath;
					Report.AddIssue(Issue);
				}
			}
		}
	}
}

// ============================================================================
// v7.8.3: Type-Aware Redundant Cast Detection
// ============================================================================
// Detects when DynamicCast is used on a value that already has the target type.
// Example: GetActivityComponent (returns NPCActivityComponent*) -> CastToNPCActivityComponent = REDUNDANT
// Note: GetComponentByClass returns ActorComponent* (base type), so cast IS needed there.

void FPreValidator::ValidateRedundantCasts(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath)
{
	// Build registry of known function return types (specific types, not base classes)
	// Only include functions that return specific types (not base ActorComponent, Actor, etc.)
	static TMap<FString, FString> KnownReturnTypes;
	if (KnownReturnTypes.Num() == 0)
	{
		// Narrative Pro specific getters
		KnownReturnTypes.Add(TEXT("GetActivityComponent"), TEXT("NPCActivityComponent"));
		KnownReturnTypes.Add(TEXT("GetTalesComponent"), TEXT("TalesComponent"));
		KnownReturnTypes.Add(TEXT("GetInventoryComponent"), TEXT("InventoryComponent"));
		KnownReturnTypes.Add(TEXT("GetInteractionComponent"), TEXT("InteractionComponent"));
		KnownReturnTypes.Add(TEXT("GetAbilitySystemComponent"), TEXT("AbilitySystemComponent"));
		KnownReturnTypes.Add(TEXT("GetNPCComponent"), TEXT("NPCComponent"));

		// UE native specific getters
		KnownReturnTypes.Add(TEXT("GetCharacterMovement"), TEXT("CharacterMovementComponent"));
		KnownReturnTypes.Add(TEXT("GetMovementComponent"), TEXT("MovementComponent"));
		KnownReturnTypes.Add(TEXT("GetCapsuleComponent"), TEXT("CapsuleComponent"));
		KnownReturnTypes.Add(TEXT("GetMesh"), TEXT("SkeletalMeshComponent"));

		// Note: GetComponentByClass returns ActorComponent* in Blueprint - cast IS needed
		// Note: GetController returns AController* (base) - NOT in this list
	}

	// Helper lambda to check redundant casts in an event graph
	// Takes pre-built VariableTypes map since different asset types have different variable structs
	auto ValidateEventGraphRedundantCasts = [&](const TArray<FManifestGraphNodeDefinition>& Nodes,
	                                            const TArray<FManifestGraphConnectionDefinition>& Connections,
	                                            const TMap<FString, FString>& VariableTypes,
	                                            const FString& OwnerName, const FString& YAMLPrefix)
	{
		// Step 1: Build node map for quick lookup
		TMap<FString, const FManifestGraphNodeDefinition*> NodeMap;
		for (const auto& Node : Nodes)
		{
			NodeMap.Add(Node.Id, &Node);
		}

		// Step 2: Build connection map (what connects to each node's Object pin)
		// Key: "NodeId:PinName" -> Value: "SourceNodeId:SourcePinName"
		TMap<FString, FString> IncomingConnections;
		for (const auto& Conn : Connections)
		{
			FString Key = FString::Printf(TEXT("%s:%s"), *Conn.To.NodeId, *Conn.To.PinName.ToLower());
			FString Value = FString::Printf(TEXT("%s:%s"), *Conn.From.NodeId, *Conn.From.PinName);
			IncomingConnections.Add(Key, Value);
		}

		// Step 3: For each DynamicCast node, check for redundancy
		for (int32 j = 0; j < Nodes.Num(); j++)
		{
			const auto& Node = Nodes[j];
			if (!Node.Type.Equals(TEXT("DynamicCast"), ESearchCase::IgnoreCase))
			{
				continue;
			}

			// Get the target class of this cast
			FString CastTargetClass = Node.Properties.FindRef(TEXT("target_class"));
			if (CastTargetClass.IsEmpty())
			{
				continue;
			}

			// Normalize class name (remove common prefixes)
			FString NormalizedTarget = CastTargetClass;
			if (NormalizedTarget.StartsWith(TEXT("U")) || NormalizedTarget.StartsWith(TEXT("A")))
			{
				// Only strip if second char is uppercase (UE naming convention)
				if (NormalizedTarget.Len() > 1 && FChar::IsUpper(NormalizedTarget[1]))
				{
					NormalizedTarget = NormalizedTarget.RightChop(1);
				}
			}

			// Find what connects to the Object pin of this DynamicCast
			FString ObjectPinKey = FString::Printf(TEXT("%s:object"), *Node.Id);
			FString* SourceConnection = IncomingConnections.Find(ObjectPinKey);
			if (!SourceConnection)
			{
				continue;  // No connection to Object pin
			}

			// Parse source: "NodeId:PinName"
			FString SourceNodeId, SourcePinName;
			if (!SourceConnection->Split(TEXT(":"), &SourceNodeId, &SourcePinName))
			{
				continue;
			}

			// Find the source node
			const FManifestGraphNodeDefinition** SourceNodePtr = NodeMap.Find(SourceNodeId);
			if (!SourceNodePtr)
			{
				continue;
			}
			const FManifestGraphNodeDefinition& SourceNode = **SourceNodePtr;

			// Determine the output type of the source node
			FString SourceOutputType;

			if (SourceNode.Type.Equals(TEXT("CallFunction"), ESearchCase::IgnoreCase))
			{
				FString FunctionName = SourceNode.Properties.FindRef(TEXT("function"));

				// NOTE on GetComponentByClass: UE5.7 compiler warns the cast is "redundant" because at RUNTIME
				// the value is already the correct type. However, the Blueprint PIN TYPE is still ActorComponent*,
				// so the cast IS required for Blueprint pin type matching. We intentionally do NOT detect
				// GetComponentByClass here - the UE5 compiler warning is cosmetic and the cast must stay.

				// Check static registry for known return types (functions that return specific types)
				FString* KnownType = KnownReturnTypes.Find(FunctionName);
				if (KnownType)
				{
					SourceOutputType = *KnownType;
				}
			}
			else if (SourceNode.Type.Equals(TEXT("PropertyGet"), ESearchCase::IgnoreCase))
			{
				// PropertyGet: target_class defines the return type for component properties
				FString PropertyTargetClass = SourceNode.Properties.FindRef(TEXT("target_class"));
				if (!PropertyTargetClass.IsEmpty())
				{
					// The property getter typically returns the target_class type
					// But we need the property name to be sure
					FString PropertyName = SourceNode.Properties.FindRef(TEXT("property_name"));
					// For now, treat target_class as the output type if it's a component
					if (PropertyTargetClass.Contains(TEXT("Component")))
					{
						SourceOutputType = PropertyTargetClass;
					}
				}
			}
			else if (SourceNode.Type.Equals(TEXT("VariableGet"), ESearchCase::IgnoreCase))
			{
				// VariableGet: check our variable type map
				FString VarName = SourceNode.Properties.FindRef(TEXT("variable_name"));
				const FString* VarType = VariableTypes.Find(VarName);
				if (VarType)
				{
					SourceOutputType = *VarType;
				}
			}
			else if (SourceNode.Type.Equals(TEXT("DynamicCast"), ESearchCase::IgnoreCase))
			{
				// Chained cast: output type is the target_class of the source cast
				SourceOutputType = SourceNode.Properties.FindRef(TEXT("target_class"));
			}

			// Skip if we couldn't determine source type
			if (SourceOutputType.IsEmpty())
			{
				continue;
			}

			// Normalize source type
			FString NormalizedSource = SourceOutputType;
			if (NormalizedSource.StartsWith(TEXT("U")) || NormalizedSource.StartsWith(TEXT("A")))
			{
				if (NormalizedSource.Len() > 1 && FChar::IsUpper(NormalizedSource[1]))
				{
					NormalizedSource = NormalizedSource.RightChop(1);
				}
			}

			// Compare: if source type equals or inherits from target type, cast is redundant
			// For exact match only (inheritance check would require full class hierarchy)
			if (NormalizedSource.Equals(NormalizedTarget, ESearchCase::IgnoreCase))
			{
				FPreValidationIssue Issue;
				Issue.RuleId = TEXT("RC1");
				Issue.ErrorCode = TEXT("E_PREVAL_REDUNDANT_CAST");
				Issue.Severity = EValidationSeverity::Warning;
				Issue.Message = FString::Printf(TEXT("'%s' is already a '%s', you don't need Cast To %s"),
					*SourcePinName, *SourceOutputType, *CastTargetClass);
				Issue.ItemId = FString::Printf(TEXT("%s.%s"), *OwnerName, *Node.Id);
				Issue.YAMLPath = FString::Printf(TEXT("%s.event_graph.nodes[%d]"), *YAMLPrefix, j);
				Issue.ManifestPath = ManifestPath;
				Report.AddIssue(Issue);
			}
		}
	};

	// Validate GameplayAbility event graphs
	for (int32 i = 0; i < Data.GameplayAbilities.Num(); i++)
	{
		const auto& GA = Data.GameplayAbilities[i];
		// Build variable type map from FManifestActorVariableDefinition
		TMap<FString, FString> VarTypes;
		for (const auto& Var : GA.Variables)
		{
			if (!Var.Class.IsEmpty())
			{
				VarTypes.Add(Var.Name, Var.Class);
			}
		}
		ValidateEventGraphRedundantCasts(GA.EventGraphNodes, GA.EventGraphConnections, VarTypes,
			GA.Name, FString::Printf(TEXT("gameplay_abilities[%d]"), i));
	}

	// Validate ActorBlueprint event graphs
	for (int32 i = 0; i < Data.ActorBlueprints.Num(); i++)
	{
		const auto& BP = Data.ActorBlueprints[i];
		// Build variable type map from FManifestActorVariableDefinition
		TMap<FString, FString> VarTypes;
		for (const auto& Var : BP.Variables)
		{
			if (!Var.Class.IsEmpty())
			{
				VarTypes.Add(Var.Name, Var.Class);
			}
		}
		ValidateEventGraphRedundantCasts(BP.EventGraphNodes, BP.EventGraphConnections, VarTypes,
			BP.Name, FString::Printf(TEXT("actor_blueprints[%d]"), i));
	}

	// Validate WidgetBlueprint event graphs
	// Note: FManifestWidgetVariableDefinition doesn't have a Class field, so no type tracking
	for (int32 i = 0; i < Data.WidgetBlueprints.Num(); i++)
	{
		const auto& WBP = Data.WidgetBlueprints[i];
		TMap<FString, FString> VarTypes;  // Empty - widgets don't track object types
		ValidateEventGraphRedundantCasts(WBP.EventGraphNodes, WBP.EventGraphConnections, VarTypes,
			WBP.Name, FString::Printf(TEXT("widget_blueprints[%d]"), i));
	}
}
