// GasAbilityGenerator v4.24 - Phase 4.1 Pre-Validation System
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
//
// Pre-validates manifest references BEFORE generation starts.
// Implements reflection-based semantic checks per Phase4_Spec_Locked.md.

#pragma once

#include "CoreMinimal.h"
#include "Locked/GasAbilityGeneratorTypes.h"

// Forward declarations
struct FManifestData;

/**
 * Phase 4.1: Pre-validation cache
 * Caches reflection lookups per unique key to avoid repeated expensive operations.
 */
class GASABILITYGENERATOR_API FPreValidationCache
{
public:
	// Function existence: (ClassName, FunctionName) -> exists?
	// Returns true if cache hit, sets bOutExists to cached value
	bool CheckFunctionExists(const FString& ClassName, const FString& FunctionName, bool& bOutExists);
	void CacheFunctionResult(const FString& ClassName, const FString& FunctionName, bool bExists);

	// Attribute existence: (AttributeSetClass, PropertyName) -> exists?
	bool CheckAttributeExists(const FString& AttributeSetClass, const FString& PropertyName, bool& bOutExists);
	void CacheAttributeResult(const FString& AttributeSetClass, const FString& PropertyName, bool bExists);

	// Class existence: ClassName -> UClass* (nullptr if not found)
	// Returns true if cache hit
	bool GetCachedClass(const FString& ClassName, UClass*& OutClass);
	void CacheClass(const FString& ClassName, UClass* Class);

	// R3: Asset existence via AssetRegistry only (no TryLoad)
	bool CheckAssetExists(const FString& AssetPath, bool& bOutExists);
	void CacheAssetResult(const FString& AssetPath, bool bExists);

	// Tag existence
	bool CheckTagExists(const FString& TagName, bool& bOutExists);
	void CacheTagResult(const FString& TagName, bool bExists);

	void Clear();
	int32 GetHitCount() const { return HitCount; }
	int32 GetMissCount() const { return MissCount; }

private:
	TMap<FString, bool> FunctionCache;      // "ClassName::FunctionName" -> exists
	TMap<FString, bool> AttributeCache;     // "SetClass::PropertyName" -> exists
	TMap<FString, UClass*> ClassCache;      // ClassName -> UClass* (nullptr stored for not found)
	TSet<FString> ClassCacheValid;          // Track which class lookups were performed
	TMap<FString, bool> AssetCache;         // AssetPath -> exists (AssetRegistry)
	TMap<FString, bool> TagCache;           // TagName -> exists

	int32 HitCount = 0;
	int32 MissCount = 0;
};

/**
 * Phase 4.1: Pre-validator
 * Validates manifest references before generation starts.
 *
 * Implements rules per Phase4_Spec_Locked.md:
 * - F1, F2: Function validation (class exists, function exists on class)
 * - A1, A2: Attribute validation (AttributeSet exists, attribute exists on set)
 * - C1, C2: Class validation (UClass paths resolve, parent class exists)
 * - R1-R5: Asset reference validation (skeletons, textures, material functions, etc.)
 * - T1, T2: Tag validation (GameplayTags registered, SetByCaller tags exist)
 * - K1, K2: Token validation (supported types, valid property references)
 */
class GASABILITYGENERATOR_API FPreValidator
{
public:
	/**
	 * Main entry point - validates entire manifest
	 * @param Data The parsed manifest data
	 * @param ManifestPath Path to manifest file (for error reporting)
	 * @return Report with all validation issues
	 */
	static FPreValidationReport Validate(const FManifestData& Data, const FString& ManifestPath);

	// R5: AttributeSet configuration (default + override only, NO global scan)
	static FString GetDefaultAttributeSetClass();
	static void SetDefaultAttributeSetClass(const FString& ClassName);

private:
	// Rule implementations - each validates a category of references
	static void ValidateClasses(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath);
	static void ValidateFunctions(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath);
	static void ValidateAttributes(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath);
	static void ValidateAssetReferences(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath);
	static void ValidateTags(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath);
	static void ValidateTokens(const FManifestData& Data, FPreValidationReport& Report, FPreValidationCache& Cache, const FString& ManifestPath);

	// Helper functions
	static UClass* FindClassByName(const FString& ClassName, FPreValidationCache& Cache);
	static bool FunctionExistsOnClass(UClass* Class, const FString& FunctionName);
	static bool AttributeExistsOnSet(UClass* AttributeSetClass, const FString& AttributeName);
	static bool AssetExistsInRegistry(const FString& AssetPath);  // R3: AssetRegistry only
	static bool TagIsRegistered(const FString& TagName);

	// R5: Default is UNarrativeAttributeSetBase - NO global scan
	static FString DefaultAttributeSetClass;
};
