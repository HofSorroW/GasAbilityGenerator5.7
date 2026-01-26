// GasAbilityGenerator v3.1
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v3.0: UGeneratorAssetMetadata - Persistent metadata storage for generated assets
// v3.1: UGeneratorMetadataRegistry - Central registry for assets that don't support AssetUserData
// v4.16.1: Hash collision detection for metadata integrity

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "Engine/DataAsset.h"
#include "Locked/GasAbilityGeneratorTypes.h"
#include "GasAbilityGeneratorMetadata.generated.h"

/**
 * v3.0: Persistent metadata attached to generated assets via UAssetUserData
 *
 * This class stores generator metadata (input hash, output hash, generator version, etc.)
 * directly on the generated asset. This enables:
 * - Change detection: Know if manifest definition changed since generation
 * - Manual edit detection: Know if asset was modified after generation
 * - Smart regeneration: Only regenerate when needed
 * - Dry run mode: Preview what would change before generating
 *
 * Usage:
 *   // Store metadata on asset
 *   UGeneratorAssetMetadata* Meta = NewObject<UGeneratorAssetMetadata>(Asset);
 *   Meta->Metadata = GeneratedMetadata;
 *   Asset->AddAssetUserData(Meta);
 *
 *   // Retrieve metadata from asset
 *   UGeneratorAssetMetadata* Meta = Asset->GetAssetUserData<UGeneratorAssetMetadata>();
 *   if (Meta) { ... use Meta->Metadata ... }
 */
UCLASS()
class GASABILITYGENERATOR_API UGeneratorAssetMetadata : public UAssetUserData
{
	GENERATED_BODY()

public:
	UGeneratorAssetMetadata();

	/** The generator metadata stored on this asset */
	UPROPERTY(EditAnywhere, Category = "Generator")
	FString GeneratorId;

	UPROPERTY(EditAnywhere, Category = "Generator")
	FString ManifestPath;

	UPROPERTY(EditAnywhere, Category = "Generator")
	FString ManifestAssetKey;

	UPROPERTY(EditAnywhere, Category = "Generator")
	int64 InputHash;

	UPROPERTY(EditAnywhere, Category = "Generator")
	int64 OutputHash;

	UPROPERTY(EditAnywhere, Category = "Generator")
	FString GeneratorVersion;

	UPROPERTY(EditAnywhere, Category = "Generator")
	FDateTime Timestamp;

	UPROPERTY(EditAnywhere, Category = "Generator")
	TArray<FString> Dependencies;

	UPROPERTY(EditAnywhere, Category = "Generator")
	bool bIsGenerated;

	/** Convert to FGeneratorMetadata struct */
	FGeneratorMetadata ToMetadata() const;

	/** Set from FGeneratorMetadata struct */
	void FromMetadata(const FGeneratorMetadata& InMetadata);

	/** Check if manifest definition changed since generation */
	bool HasInputChanged(uint64 NewInputHash) const;

	/** Check if asset was manually edited since generation */
	bool HasOutputChanged(uint64 CurrentOutputHash) const;

	/** Get formatted summary string for logging */
	FString GetSummary() const;
};

/**
 * v3.1: Serializable metadata record for the registry
 */
USTRUCT(BlueprintType)
struct GASABILITYGENERATOR_API FGeneratorMetadataRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Generator")
	FString GeneratorId;

	UPROPERTY(EditAnywhere, Category = "Generator")
	FString ManifestPath;

	UPROPERTY(EditAnywhere, Category = "Generator")
	FString ManifestAssetKey;

	UPROPERTY(EditAnywhere, Category = "Generator")
	int64 InputHash = 0;

	UPROPERTY(EditAnywhere, Category = "Generator")
	int64 OutputHash = 0;

	UPROPERTY(EditAnywhere, Category = "Generator")
	FString GeneratorVersion;

	UPROPERTY(EditAnywhere, Category = "Generator")
	FDateTime Timestamp;

	UPROPERTY(EditAnywhere, Category = "Generator")
	TArray<FString> Dependencies;

	UPROPERTY(EditAnywhere, Category = "Generator")
	bool bIsGenerated = false;

	/** Convert to FGeneratorMetadata struct */
	FGeneratorMetadata ToMetadata() const;

	/** Set from FGeneratorMetadata struct */
	void FromMetadata(const FGeneratorMetadata& InMetadata);
};

/**
 * v3.1: Central registry for storing metadata of assets that don't support UAssetUserData
 *
 * This includes:
 * - All DataAssets (UDataAsset, UPrimaryDataAsset)
 * - All Blueprints (UBlueprint and subclasses)
 * - Niagara Systems
 *
 * The registry is stored as a single asset at /Game/{Project}/GeneratorMetadataRegistry
 */
UCLASS()
class GASABILITYGENERATOR_API UGeneratorMetadataRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	UGeneratorMetadataRegistry();

	/** Map of asset paths to their generator metadata */
	UPROPERTY(EditAnywhere, Category = "Generator")
	TMap<FString, FGeneratorMetadataRecord> Records;

	/** Get metadata for an asset path (returns nullptr if not found) */
	FGeneratorMetadataRecord* GetRecord(const FString& AssetPath);

	/** Set metadata for an asset path */
	void SetRecord(const FString& AssetPath, const FGeneratorMetadataRecord& Record);

	/** Remove metadata for an asset path */
	void RemoveRecord(const FString& AssetPath);

	/** Check if registry has metadata for an asset path */
	bool HasRecord(const FString& AssetPath) const;

	/** Get all asset paths in the registry */
	TArray<FString> GetAllAssetPaths() const;

	/** Clear all records */
	void ClearAllRecords();

	// Static helper to get/create the singleton registry
	static UGeneratorMetadataRegistry* GetOrCreateRegistry();
	static UGeneratorMetadataRegistry* GetRegistry();
	static void SaveRegistry();
	static void ClearRegistryCache();

	/**
	 * v4.16.1: Hash collision detection for metadata integrity
	 * Detects when two different assets produce the same InputHash
	 * This helps identify hash function weaknesses or manifest issues
	 */
	void CheckHashCollision(int64 Hash, const FString& AssetPath);

	/** Clear the collision map (call at start of generation session) */
	void ClearCollisionMap();

private:
	static TWeakObjectPtr<UGeneratorMetadataRegistry> CachedRegistry;

	/** v4.40: Thread-safe cache access */
	static FCriticalSection CacheAccessLock;

	/** v4.16.1: Transient map for collision detection (Hash string -> first asset path) */
	TMap<FString, FString> HashToAssetMap;
};

/**
 * v3.0: Helper functions for working with generator metadata on assets
 * v3.1: Now uses registry fallback for unsupported asset types
 */
namespace GeneratorMetadataHelpers
{
	/**
	 * Get generator metadata from an asset via AssetUserData (returns nullptr if not found or unsupported)
	 * Only works with assets implementing IInterface_AssetUserData
	 */
	UGeneratorAssetMetadata* GetMetadata(UObject* Asset);

	/**
	 * v3.1: Get generator metadata from an asset, checking both AssetUserData and registry
	 * This is the preferred method for retrieving metadata
	 */
	FGeneratorMetadata GetMetadataEx(UObject* Asset);

	/**
	 * v3.1: Check if we have metadata from either AssetUserData or registry
	 */
	bool HasMetadataEx(UObject* Asset);

	/**
	 * Store generator metadata on an asset
	 * Creates new metadata object if needed, updates existing if present
	 */
	void SetMetadata(UObject* Asset, const FGeneratorMetadata& Metadata);

	/**
	 * Remove generator metadata from an asset
	 */
	void RemoveMetadata(UObject* Asset);

	/**
	 * Check if an asset has generator metadata
	 */
	bool HasMetadata(UObject* Asset);

	/**
	 * Determine dry run status for an asset based on metadata comparison
	 * @param Asset - The existing asset (or nullptr if not found)
	 * @param NewInputHash - Hash of current manifest definition
	 * @param ComputeCurrentOutputHash - Function to compute current asset's output hash
	 * @return Dry run status indicating what would happen on generation
	 */
	EDryRunStatus ComputeDryRunStatus(
		UObject* Asset,
		uint64 NewInputHash,
		TFunction<uint64(UObject*)> ComputeCurrentOutputHash = nullptr);

	/**
	 * v3.1: Save the metadata registry after generation completes
	 * Call this at the end of asset generation to persist registry changes
	 */
	void SaveRegistryIfNeeded();

	/**
	 * v4.12.5: Safe metadata retrieval with explicit success/failure
	 * Single lookup - checks both AssetUserData and registry
	 * Returns false if neither source has metadata for this asset
	 *
	 * Usage:
	 *   FGeneratorMetadata Meta;
	 *   if (!TryGetMetadata(Asset, Meta)) { // treat as manual }
	 */
	inline bool TryGetMetadata(UObject* Asset, FGeneratorMetadata& OutMetadata)
	{
		if (!Asset)
		{
			OutMetadata = FGeneratorMetadata();
			return false;
		}
		OutMetadata = GetMetadataEx(Asset);
		return OutMetadata.bIsGenerated;
	}
}
