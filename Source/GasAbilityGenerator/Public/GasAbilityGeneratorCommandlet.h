// GasAbilityGeneratorCommandlet.h
// Commandlet for automated asset generation from command line
// v4.25: Dependency ordering with cascade skip logic

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GasAbilityGeneratorCommandlet.generated.h"

// Forward declarations
struct FManifestData;
struct FGenerationResult;
struct FGenerationSummary;
class FDependencyGraph;

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

	// v3.1: Track generation results for exit code
	int32 LastFailedCount = 0;
	int32 LastValidationErrorCount = 0;
	bool bHadParseError = false;

	// v4.7: Report system state
	FString CachedManifestPath;
	int64 CachedManifestHash = 0;
	bool bCachedForceMode = false;

	// v3.9.9: Level actor placement
	void GenerateLevelActors(const FManifestData& ManifestData, UWorld* TargetWorld);
	void SaveWorldPackage(UWorld* World);
	UWorld* LoadedWorld = nullptr;

	// v4.25: Dependency ordering and cascade skip
	void BuildDependencyGraph(const FManifestData& ManifestData);
	bool CheckUpstreamFailure(const FString& AssetName, FGenerationResult& OutCascadeResult);
	void RegisterFailure(const FString& AssetName, const FString& ErrorCode);

	TMap<FString, FString> FailedAssets;     // AssetName -> ErrorCode for cascade lookup
	TSet<FString> CascadeRoots;              // Unique root failures
	TMap<FString, TArray<FString>> AssetDependencies;  // AssetName -> list of its dependencies
	FDependencyGraph* DependencyGraph = nullptr;  // Built per-run, not persisted
	static constexpr int32 MaxCascadeDepth = 16;  // Cap per T2 tighten-up
};
