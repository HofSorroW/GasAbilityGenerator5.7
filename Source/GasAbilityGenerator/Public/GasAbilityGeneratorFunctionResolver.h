// GasAbilityGenerator v4.31 - Function Resolution Parity System
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
//
// Shared function resolver for Generator and PreValidator.
// Implements identical resolution behavior per PreValidator_Generator_Parity_Audit_v1.md.
// Contract: "PreValidator function resolution behavior must be identical to Generator
// function resolution behavior. Both use FGasAbilityGeneratorFunctionResolver::ResolveFunction()
// as single source of truth. No false positives AND no false negatives."

#pragma once

#include "CoreMinimal.h"

/**
 * Result of function resolution.
 * Contains both the UFunction pointer and the owning class for node creation.
 */
struct GASABILITYGENERATOR_API FResolvedFunction
{
	/** The resolved function, or nullptr if not found */
	UFunction* Function = nullptr;

	/** The class that owns/defines the function */
	UClass* OwnerClass = nullptr;

	/** Whether resolution succeeded */
	bool bFound = false;

	/** Resolution path that found the function (for debugging) */
	FString ResolutionPath;
};

/**
 * Shared function resolver for Generator and PreValidator.
 *
 * Resolution Order (must match exactly for parity):
 * 1. WellKnownFunctions probe - tries variants (Name, K2_Name, BP_Name)
 * 2. Explicit class - if class: specified, exact name only
 * 3. Parent chain - if target_self or from parent, exact name only
 * 4. Library fallback - 13 classes in order, exact name only
 * 5. Blueprint FunctionGraph - search current Blueprint's custom functions (v4.31)
 *
 * Note: Step 5 only applies when Blueprint is provided (Generator context).
 * PreValidator passes nullptr since Blueprint doesn't exist yet.
 *
 * @see PreValidator_Generator_Parity_Audit_v1.md
 */
class GASABILITYGENERATOR_API FGasAbilityGeneratorFunctionResolver
{
public:
	/**
	 * Main entry point - resolves a function using the 5-step cascade.
	 * Both Generator and PreValidator must use this method.
	 *
	 * @param FunctionName The function name from manifest
	 * @param ExplicitClassName The class: field from manifest (may be empty)
	 * @param ParentClass The Blueprint's parent class for parent chain walk
	 * @param bTargetSelf Whether target_self: true was specified
	 * @param Blueprint Optional - the Blueprint being generated (for custom function lookup)
	 * @return FResolvedFunction with Function, OwnerClass, and bFound
	 */
	static FResolvedFunction ResolveFunction(
		const FString& FunctionName,
		const FString& ExplicitClassName,
		UClass* ParentClass,
		bool bTargetSelf,
		class UBlueprint* Blueprint = nullptr
	);

	/**
	 * Check if a function exists using the same resolution logic.
	 * Convenience wrapper for PreValidator.
	 *
	 * @param FunctionName The function name from manifest
	 * @param ExplicitClassName The class: field from manifest (may be empty)
	 * @param ParentClass The Blueprint's parent class
	 * @param bTargetSelf Whether target_self: true was specified
	 * @return true if function can be resolved
	 */
	static bool FunctionExists(
		const FString& FunctionName,
		const FString& ExplicitClassName,
		UClass* ParentClass,
		bool bTargetSelf
	);

	/**
	 * Find a class by name. Uses same lookup paths as generator.
	 * Checks native classes first, then searches common Blueprint paths.
	 *
	 * @param ClassName The class name (may include U/A prefix or not)
	 * @return UClass* or nullptr if not found
	 */
	static UClass* FindClassByName(const FString& ClassName);

private:
	/** Initialize WellKnownFunctions table if not already done */
	static void EnsureWellKnownFunctionsInitialized();

	/** Step 1: Probe WellKnownFunctions with variants */
	static FResolvedFunction ResolveViaWellKnown(const FString& FunctionName);

	/** Step 2: Try explicit class with exact name */
	static FResolvedFunction ResolveViaExplicitClass(const FString& FunctionName, const FString& ClassName);

	/** Step 3: Walk parent class chain with exact name */
	static FResolvedFunction ResolveViaParentChain(const FString& FunctionName, UClass* ParentClass);

	/** Step 4: Try library fallback classes with exact name */
	static FResolvedFunction ResolveViaLibraryFallback(const FString& FunctionName);

	/** Step 5: Search Blueprint's FunctionGraphs for custom functions (v4.31) */
	static FResolvedFunction ResolveViaBlueprintFunctionGraph(const FString& FunctionName, class UBlueprint* Blueprint);

	/** The WellKnownFunctions table - maps function names to owning classes */
	static TMap<FString, UClass*> WellKnownFunctions;

	/** Flag to track initialization */
	static bool bWellKnownFunctionsInitialized;
};
