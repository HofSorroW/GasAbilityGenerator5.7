// GasAbilityGeneratorPipeline.h
// v3.9.8: Mesh-to-Item Pipeline utilities
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GasAbilityGeneratorTypes.h"

/**
 * FPipelineMeshAnalyzer - Static utility class for analyzing meshes and inferring item properties
 *
 * Provides heuristics-based analysis to determine:
 * - Equipment slot from mesh asset name
 * - Item type from mesh properties
 * - Suggested display names from asset names
 */
class GASABILITYGENERATOR_API FPipelineMeshAnalyzer
{
public:
	/**
	 * Analyze a mesh asset and infer item properties
	 * @param MeshPath - Full path to the mesh asset
	 * @return Analysis result with inferred type, slot, and names
	 */
	static FMeshAnalysisResult AnalyzeMesh(const FString& MeshPath);

	/**
	 * Infer equipment slot from mesh asset name
	 * @param MeshName - Name of the mesh asset (without path)
	 * @param OutConfidence - Confidence level of the inference (0.0 - 1.0)
	 * @return Gameplay tag string for the inferred slot
	 */
	static FString InferSlotFromName(const FString& MeshName, float& OutConfidence);

	/**
	 * Infer item type from mesh asset
	 * @param MeshPath - Full path to the mesh asset
	 * @param OutConfidence - Confidence level of the inference (0.0 - 1.0)
	 * @return Inferred pipeline item type
	 */
	static EPipelineItemType InferTypeFromMesh(const FString& MeshPath, float& OutConfidence);

	/**
	 * Generate a human-readable display name from asset name
	 * @param AssetName - Asset name (e.g., "SK_IronHelmet")
	 * @return Human-readable name (e.g., "Iron Helmet")
	 */
	static FString GenerateDisplayName(const FString& AssetName);

	/**
	 * Generate a standardized item asset name from mesh name
	 * @param MeshName - Source mesh name (e.g., "SK_IronHelmet")
	 * @return Item asset name (e.g., "EI_IronHelmet")
	 */
	static FString GenerateItemAssetName(const FString& MeshName);

private:
	// Slot inference patterns
	struct FSlotPattern
	{
		TArray<FString> Keywords;
		FString SlotTag;
		float BaseConfidence;
	};

	static TArray<FSlotPattern> GetSlotPatterns();
};

/**
 * FPipelineProcessor - Orchestrates the full Mesh-to-Item pipeline
 *
 * Pipeline flow:
 * 1. Mesh → BI_C_ Item (via FEquippableItemGenerator)
 * 2. Item → IC_ Collection (via FItemCollectionGenerator)
 * 3. Collection → NPC Loadout (via FNPCDefinitionGenerator)
 */
class GASABILITYGENERATOR_API FPipelineProcessor
{
public:
	FPipelineProcessor();

	/**
	 * Process a single mesh through the full pipeline
	 * @param Input - Pipeline input configuration
	 * @return Processing result with generated asset paths
	 */
	FPipelineProcessResult ProcessMesh(const FPipelineMeshInput& Input);

	/**
	 * Process multiple meshes through the pipeline
	 * @param Inputs - Array of pipeline input configurations
	 * @return Array of processing results
	 */
	TArray<FPipelineProcessResult> ProcessMeshes(const TArray<FPipelineMeshInput>& Inputs);

	/**
	 * Set the project root for asset generation
	 * @param ProjectRoot - Project root path (e.g., "/Game/FatherCompanion")
	 */
	void SetProjectRoot(const FString& InProjectRoot) { ProjectRoot = InProjectRoot; }

	/**
	 * Get the current project root
	 */
	FString GetProjectRoot() const { return ProjectRoot; }

private:
	FString ProjectRoot;

	/**
	 * Generate an equippable item from mesh input
	 * @param Input - Pipeline input configuration
	 * @param OutResult - Result to populate with generation status
	 * @return True if item was generated successfully
	 */
	bool GenerateItemFromMesh(const FPipelineMeshInput& Input, FPipelineProcessResult& OutResult);

	/**
	 * Add generated item to a collection (creates collection if needed)
	 * @param ItemPath - Path to the generated item
	 * @param CollectionName - Name of the target collection
	 * @param OutResult - Result to populate with collection status
	 * @return True if item was added to collection successfully
	 */
	bool AddToCollection(const FString& ItemPath, const FString& CollectionName, FPipelineProcessResult& OutResult);

	/**
	 * Update NPC loadout with collection
	 * @param CollectionPath - Path to the collection
	 * @param LoadoutName - Name of the NPC definition to update
	 * @param OutResult - Result to populate with loadout status
	 * @return True if loadout was updated successfully
	 */
	bool AddToLoadout(const FString& CollectionPath, const FString& LoadoutName, FPipelineProcessResult& OutResult);

	/**
	 * Log a pipeline message
	 */
	void LogPipeline(const FString& Message);
};
