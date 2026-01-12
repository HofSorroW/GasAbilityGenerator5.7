// GasAbilityGeneratorCommandlet.h
// Commandlet for automated asset generation from command line

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GasAbilityGeneratorCommandlet.generated.h"

// Forward declarations
struct FManifestData;
struct FGenerationResult;

/**
 * v2.6.7: Deferred asset info for retry mechanism
 */
struct FDeferredAsset
{
	FString AssetName;
	FString AssetType;  // "ActorBlueprint", "GameplayAbility", "GameplayEffect", etc.
	FString MissingDependency;
	FString MissingDependencyType;
	int32 DefinitionIndex;  // Index in the manifest array for this asset type
	int32 RetryCount = 0;

	FDeferredAsset() : DefinitionIndex(-1) {}
	FDeferredAsset(const FString& InName, const FString& InType, const FString& InMissingDep,
		const FString& InMissingDepType, int32 InIndex)
		: AssetName(InName), AssetType(InType), MissingDependency(InMissingDep)
		, MissingDependencyType(InMissingDepType), DefinitionIndex(InIndex) {}
};

/**
 * Commandlet to generate assets from a YAML manifest without opening the editor UI.
 *
 * Usage:
 *   UnrealEditor.exe ProjectName.uproject -run=GasAbilityGenerator -manifest="path/to/manifest.yaml"
 *
 * Parameters:
 *   -manifest=<path>  : Path to the YAML manifest file (required)
 *   -tags             : Generate gameplay tags
 *   -assets           : Generate assets
 *   -all              : Generate both tags and assets (default)
 *   -output=<path>    : Output log file path (optional)
 *
 * v2.6.7: Automatic dependency resolution with retry mechanism
 */
UCLASS()
class GASABILITYGENERATOR_API UGasAbilityGeneratorCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGasAbilityGeneratorCommandlet();

	virtual int32 Main(const FString& Params) override;

private:
	void GenerateTags(const FManifestData& ManifestData);
	void GenerateAssets(const FManifestData& ManifestData);

	// v2.6.7: Retry deferred assets
	void ProcessDeferredAssets(const FManifestData& ManifestData);
	bool TryGenerateDeferredAsset(const FDeferredAsset& Deferred, const FManifestData& ManifestData, FGenerationResult& OutResult);
	bool IsDependencyResolved(const FString& DependencyName, const FString& DependencyType) const;

	void LogMessage(const FString& Message);
	void LogError(const FString& Message);

	FString OutputLogPath;
	TArray<FString> LogMessages;

	// v2.6.7: Deferred asset tracking
	TArray<FDeferredAsset> DeferredAssets;
	TSet<FString> GeneratedAssets;  // Track successfully generated assets for dependency checking
	static constexpr int32 MaxRetryAttempts = 3;

	// v2.8.4: Verification tracking
	TSet<FString> ProcessedAssets;  // Track ALL processed assets (new + skipped + failed)
	TArray<FString> GenerationDuplicates;  // Track assets processed more than once
	void VerifyGenerationComplete(const TSet<FString>& ExpectedAssets, int32 ExpectedCount, int32 ActualCount);
};
