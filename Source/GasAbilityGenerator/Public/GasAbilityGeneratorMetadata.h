// GasAbilityGenerator v3.0
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.
// v3.0: UGeneratorAssetMetadata - Persistent metadata storage for generated assets

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "GasAbilityGeneratorTypes.h"
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
 * v3.0: Helper functions for working with generator metadata on assets
 */
namespace GeneratorMetadataHelpers
{
	/**
	 * Get generator metadata from an asset (returns nullptr if not found)
	 * Works with any UObject that supports AssetUserData (UBlueprint, UDataAsset, etc.)
	 */
	UGeneratorAssetMetadata* GetMetadata(UObject* Asset);

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
}
