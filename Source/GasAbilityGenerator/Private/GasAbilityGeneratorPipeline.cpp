// GasAbilityGeneratorPipeline.cpp
// v3.9.8: Mesh-to-Item Pipeline implementation
// Copyright (c) Erdem - Second Chance RPG. All Rights Reserved.

#include "GasAbilityGeneratorPipeline.h"
#include "GasAbilityGeneratorModule.h"
#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGasAbilityPipeline, Log, All);
DEFINE_LOG_CATEGORY(LogGasAbilityPipeline);

//=============================================================================
// FPipelineMeshAnalyzer Implementation
//=============================================================================

TArray<FPipelineMeshAnalyzer::FSlotPattern> FPipelineMeshAnalyzer::GetSlotPatterns()
{
	TArray<FSlotPattern> Patterns;

	// Head slot patterns
	{
		FSlotPattern Pattern;
		Pattern.Keywords = { TEXT("helmet"), TEXT("helm"), TEXT("hat"), TEXT("hood"), TEXT("mask"), TEXT("crown"), TEXT("circlet"), TEXT("headband") };
		Pattern.SlotTag = TEXT("Narrative.Equipment.Slot.Mesh.Head");
		Pattern.BaseConfidence = 0.9f;
		Patterns.Add(Pattern);
	}

	// Torso slot patterns
	{
		FSlotPattern Pattern;
		Pattern.Keywords = { TEXT("chest"), TEXT("cuirass"), TEXT("shirt"), TEXT("robe"), TEXT("tunic"), TEXT("armor"), TEXT("vest"), TEXT("jacket"), TEXT("coat"), TEXT("torso") };
		Pattern.SlotTag = TEXT("Narrative.Equipment.Slot.Mesh.Torso");
		Pattern.BaseConfidence = 0.85f;
		Patterns.Add(Pattern);
	}

	// Legs slot patterns
	{
		FSlotPattern Pattern;
		Pattern.Keywords = { TEXT("leg"), TEXT("greave"), TEXT("pants"), TEXT("trousers"), TEXT("legging"), TEXT("skirt"), TEXT("kilt") };
		Pattern.SlotTag = TEXT("Narrative.Equipment.Slot.Mesh.Legs");
		Pattern.BaseConfidence = 0.9f;
		Patterns.Add(Pattern);
	}

	// Feet slot patterns
	{
		FSlotPattern Pattern;
		Pattern.Keywords = { TEXT("boot"), TEXT("shoe"), TEXT("sabaton"), TEXT("sandal"), TEXT("feet"), TEXT("foot") };
		Pattern.SlotTag = TEXT("Narrative.Equipment.Slot.Mesh.Feet");
		Pattern.BaseConfidence = 0.9f;
		Patterns.Add(Pattern);
	}

	// Hands slot patterns
	{
		FSlotPattern Pattern;
		Pattern.Keywords = { TEXT("glove"), TEXT("gauntlet"), TEXT("bracer"), TEXT("hand"), TEXT("mitt"), TEXT("grip") };
		Pattern.SlotTag = TEXT("Narrative.Equipment.Slot.Mesh.Hands");
		Pattern.BaseConfidence = 0.9f;
		Patterns.Add(Pattern);
	}

	// Back slot patterns
	{
		FSlotPattern Pattern;
		Pattern.Keywords = { TEXT("cape"), TEXT("cloak"), TEXT("backpack"), TEXT("back"), TEXT("mantle"), TEXT("quiver") };
		Pattern.SlotTag = TEXT("Narrative.Equipment.Slot.Mesh.Back");
		Pattern.BaseConfidence = 0.85f;
		Patterns.Add(Pattern);
	}

	// Shoulder slot patterns
	{
		FSlotPattern Pattern;
		Pattern.Keywords = { TEXT("shoulder"), TEXT("pauldron"), TEXT("spaulder"), TEXT("epaulet") };
		Pattern.SlotTag = TEXT("Narrative.Equipment.Slot.Mesh.Shoulder");
		Pattern.BaseConfidence = 0.9f;
		Patterns.Add(Pattern);
	}

	// Weapon patterns (lower confidence as these typically use different item types)
	{
		FSlotPattern Pattern;
		Pattern.Keywords = { TEXT("sword"), TEXT("axe"), TEXT("mace"), TEXT("dagger"), TEXT("spear"), TEXT("hammer"), TEXT("staff"), TEXT("bow"), TEXT("crossbow"), TEXT("rifle"), TEXT("gun"), TEXT("pistol") };
		Pattern.SlotTag = TEXT("Narrative.Equipment.Slot.Weapon");
		Pattern.BaseConfidence = 0.7f;
		Patterns.Add(Pattern);
	}

	return Patterns;
}

FMeshAnalysisResult FPipelineMeshAnalyzer::AnalyzeMesh(const FString& MeshPath)
{
	FMeshAnalysisResult Result;

	// Extract mesh name from path
	FString MeshName = FPaths::GetBaseFilename(MeshPath);

	// Infer slot
	Result.InferredSlot = InferSlotFromName(MeshName, Result.SlotConfidence);

	// Infer type
	Result.InferredType = InferTypeFromMesh(MeshPath, Result.TypeConfidence);

	// Generate names
	Result.SuggestedDisplayName = GenerateDisplayName(MeshName);
	Result.SuggestedAssetName = GenerateItemAssetName(MeshName);

	return Result;
}

FString FPipelineMeshAnalyzer::InferSlotFromName(const FString& MeshName, float& OutConfidence)
{
	OutConfidence = 0.0f;
	FString BestSlot = TEXT("Narrative.Equipment.Slot.Mesh.Torso"); // Default fallback

	// Normalize name for matching
	FString NormalizedName = MeshName.ToLower();

	// Remove common prefixes
	if (NormalizedName.StartsWith(TEXT("sk_")))
	{
		NormalizedName = NormalizedName.Mid(3);
	}
	else if (NormalizedName.StartsWith(TEXT("sm_")))
	{
		NormalizedName = NormalizedName.Mid(3);
	}

	// Check against patterns
	TArray<FSlotPattern> Patterns = GetSlotPatterns();
	for (const FSlotPattern& Pattern : Patterns)
	{
		for (const FString& Keyword : Pattern.Keywords)
		{
			if (NormalizedName.Contains(Keyword))
			{
				// Calculate confidence based on keyword position and match quality
				float Confidence = Pattern.BaseConfidence;

				// Boost confidence if keyword is at the end (more specific)
				if (NormalizedName.EndsWith(Keyword))
				{
					Confidence += 0.05f;
				}

				// Boost confidence if it's a longer keyword (more specific)
				if (Keyword.Len() > 5)
				{
					Confidence += 0.02f;
				}

				// Clamp to max
				Confidence = FMath::Min(Confidence, 0.98f);

				if (Confidence > OutConfidence)
				{
					OutConfidence = Confidence;
					BestSlot = Pattern.SlotTag;
				}
			}
		}
	}

	return BestSlot;
}

EPipelineItemType FPipelineMeshAnalyzer::InferTypeFromMesh(const FString& MeshPath, float& OutConfidence)
{
	OutConfidence = 0.5f; // Default moderate confidence

	FString MeshName = FPaths::GetBaseFilename(MeshPath).ToLower();

	// Check for weapon keywords
	TArray<FString> MeleeKeywords = { TEXT("sword"), TEXT("axe"), TEXT("mace"), TEXT("dagger"), TEXT("spear"), TEXT("hammer"), TEXT("club"), TEXT("blade") };
	TArray<FString> RangedKeywords = { TEXT("bow"), TEXT("crossbow"), TEXT("rifle"), TEXT("gun"), TEXT("pistol"), TEXT("launcher"), TEXT("cannon") };
	TArray<FString> ConsumableKeywords = { TEXT("potion"), TEXT("food"), TEXT("drink"), TEXT("scroll"), TEXT("consumable") };

	for (const FString& Keyword : MeleeKeywords)
	{
		if (MeshName.Contains(Keyword))
		{
			OutConfidence = 0.85f;
			return EPipelineItemType::Weapon_Melee;
		}
	}

	for (const FString& Keyword : RangedKeywords)
	{
		if (MeshName.Contains(Keyword))
		{
			OutConfidence = 0.85f;
			return EPipelineItemType::Weapon_Ranged;
		}
	}

	for (const FString& Keyword : ConsumableKeywords)
	{
		if (MeshName.Contains(Keyword))
		{
			OutConfidence = 0.8f;
			return EPipelineItemType::Consumable;
		}
	}

	// Default to clothing if it looks like armor/clothing
	TArray<FString> ClothingKeywords = { TEXT("armor"), TEXT("helm"), TEXT("boot"), TEXT("glove"), TEXT("chest"), TEXT("leg"), TEXT("cape"), TEXT("robe"), TEXT("shirt"), TEXT("pants") };
	for (const FString& Keyword : ClothingKeywords)
	{
		if (MeshName.Contains(Keyword))
		{
			OutConfidence = 0.85f;
			return EPipelineItemType::Clothing;
		}
	}

	// If has SK_ prefix (skeletal mesh), likely clothing
	if (MeshPath.Contains(TEXT("SK_")))
	{
		OutConfidence = 0.6f;
		return EPipelineItemType::Clothing;
	}

	// Default to generic
	return EPipelineItemType::Generic;
}

FString FPipelineMeshAnalyzer::GenerateDisplayName(const FString& AssetName)
{
	FString Name = AssetName;

	// Remove common prefixes
	if (Name.StartsWith(TEXT("SK_")))
	{
		Name = Name.Mid(3);
	}
	else if (Name.StartsWith(TEXT("SM_")))
	{
		Name = Name.Mid(3);
	}
	else if (Name.StartsWith(TEXT("EI_")))
	{
		Name = Name.Mid(3);
	}
	else if (Name.StartsWith(TEXT("BI_C_")))
	{
		Name = Name.Mid(5);
	}

	// Insert spaces before uppercase letters (CamelCase to "Camel Case")
	FString Result;
	for (int32 i = 0; i < Name.Len(); i++)
	{
		TCHAR Char = Name[i];

		// Check if we need to insert space
		if (i > 0)
		{
			bool bIsUpper = FChar::IsUpper(Char);
			bool bPrevIsLower = FChar::IsLower(Name[i - 1]);
			bool bIsDigit = FChar::IsDigit(Char);
			bool bPrevIsAlpha = FChar::IsAlpha(Name[i - 1]);

			// Insert space before uppercase following lowercase, or digit following alpha
			if ((bIsUpper && bPrevIsLower) || (bIsDigit && bPrevIsAlpha))
			{
				Result += TEXT(' ');
			}
		}

		// Replace underscores with spaces
		if (Char == TEXT('_'))
		{
			Result += TEXT(' ');
		}
		else
		{
			Result += Char;
		}
	}

	// Clean up multiple spaces
	while (Result.Contains(TEXT("  ")))
	{
		Result.ReplaceInline(TEXT("  "), TEXT(" "));
	}

	return Result.TrimStartAndEnd();
}

FString FPipelineMeshAnalyzer::GenerateItemAssetName(const FString& MeshName)
{
	FString Name = MeshName;

	// Remove mesh prefixes
	if (Name.StartsWith(TEXT("SK_")))
	{
		Name = Name.Mid(3);
	}
	else if (Name.StartsWith(TEXT("SM_")))
	{
		Name = Name.Mid(3);
	}

	// Add EI_ prefix (Equippable Item)
	return FString::Printf(TEXT("EI_%s"), *Name);
}

//=============================================================================
// FPipelineProcessor Implementation
//=============================================================================

FPipelineProcessor::FPipelineProcessor()
	: ProjectRoot(TEXT("/Game"))
{
}

FPipelineProcessResult FPipelineProcessor::ProcessMesh(const FPipelineMeshInput& Input)
{
	FPipelineProcessResult Result;
	Result.bSuccess = true;

	LogPipeline(FString::Printf(TEXT("Processing mesh: %s"), *Input.MeshPath));

	// Step 1: Generate item from mesh
	if (!GenerateItemFromMesh(Input, Result))
	{
		Result.bSuccess = false;
		return Result;
	}

	// Step 2: Add to collection if specified
	if (!Input.TargetCollection.IsEmpty() && !Result.GeneratedItemPath.IsEmpty())
	{
		if (!AddToCollection(Result.GeneratedItemPath, Input.TargetCollection, Result))
		{
			Result.Warnings.Add(FString::Printf(TEXT("Failed to add item to collection: %s"), *Input.TargetCollection));
		}
	}

	// Step 3: Add collection to loadout if specified
	if (!Input.TargetLoadout.IsEmpty() && !Result.GeneratedCollectionPath.IsEmpty())
	{
		if (!AddToLoadout(Result.GeneratedCollectionPath, Input.TargetLoadout, Result))
		{
			Result.Warnings.Add(FString::Printf(TEXT("Failed to add collection to loadout: %s"), *Input.TargetLoadout));
		}
	}

	return Result;
}

TArray<FPipelineProcessResult> FPipelineProcessor::ProcessMeshes(const TArray<FPipelineMeshInput>& Inputs)
{
	TArray<FPipelineProcessResult> Results;
	Results.Reserve(Inputs.Num());

	for (const FPipelineMeshInput& Input : Inputs)
	{
		Results.Add(ProcessMesh(Input));
	}

	return Results;
}

bool FPipelineProcessor::GenerateItemFromMesh(const FPipelineMeshInput& Input, FPipelineProcessResult& OutResult)
{
	// Analyze mesh if needed
	FMeshAnalysisResult Analysis;
	if (Input.EquipmentSlot.IsEmpty() || Input.DisplayName.IsEmpty())
	{
		Analysis = FPipelineMeshAnalyzer::AnalyzeMesh(Input.MeshPath);
	}

	// Create equippable item definition
	FManifestEquippableItemDefinition ItemDef;

	// Use provided name or generate from mesh
	FString MeshName = FPaths::GetBaseFilename(Input.MeshPath);
	ItemDef.Name = FPipelineMeshAnalyzer::GenerateItemAssetName(MeshName);

	// Set folder
	ItemDef.Folder = TEXT("Items/Generated");

	// Set parent class based on item type
	switch (Input.ItemType)
	{
	case EPipelineItemType::Clothing:
		ItemDef.ParentClass = TEXT("EquippableItem_Clothing");
		break;
	case EPipelineItemType::Weapon_Melee:
		ItemDef.ParentClass = TEXT("MeleeWeaponItem");
		break;
	case EPipelineItemType::Weapon_Ranged:
		ItemDef.ParentClass = TEXT("RangedWeaponItem");
		break;
	case EPipelineItemType::Consumable:
		ItemDef.ParentClass = TEXT("ConsumableItem");
		break;
	default:
		ItemDef.ParentClass = TEXT("EquippableItem");
		break;
	}

	// Set equipment slot
	ItemDef.EquipmentSlot = Input.EquipmentSlot.IsEmpty() ? Analysis.InferredSlot : Input.EquipmentSlot;

	// Set display name
	ItemDef.DisplayName = Input.DisplayName.IsEmpty() ? Analysis.SuggestedDisplayName : Input.DisplayName;

	// Set clothing mesh data
	ItemDef.ClothingMesh.Mesh = Input.MeshPath;
	ItemDef.ClothingMesh.bUseLeaderPose = true;

	// Log what we're creating
	LogPipeline(FString::Printf(TEXT("  Creating item: %s"), *ItemDef.Name));
	LogPipeline(FString::Printf(TEXT("    Parent: %s"), *ItemDef.ParentClass));
	LogPipeline(FString::Printf(TEXT("    Slot: %s"), *ItemDef.EquipmentSlot));
	LogPipeline(FString::Printf(TEXT("    Display: %s"), *ItemDef.DisplayName));
	LogPipeline(FString::Printf(TEXT("    Mesh: %s"), *ItemDef.ClothingMesh.Mesh));

	// Store generated path
	OutResult.GeneratedItemPath = FString::Printf(TEXT("%s/%s/%s"), *ProjectRoot, *ItemDef.Folder, *ItemDef.Name);

	// Note: Actual generation would use FEquippableItemGenerator here
	// For now, log that we've prepared the definition
	LogPipeline(FString::Printf(TEXT("  [INFO] Item definition prepared: %s"), *OutResult.GeneratedItemPath));
	LogPipeline(TEXT("  [INFO] Use FEquippableItemGenerator to create the asset"));

	return true;
}

bool FPipelineProcessor::AddToCollection(const FString& ItemPath, const FString& CollectionName, FPipelineProcessResult& OutResult)
{
	// Prepare collection path
	FString CollectionPath = FString::Printf(TEXT("%s/Items/Collections/%s"), *ProjectRoot, *CollectionName);

	LogPipeline(FString::Printf(TEXT("  Adding to collection: %s"), *CollectionName));
	LogPipeline(FString::Printf(TEXT("    Item: %s"), *ItemPath));

	// Store generated path
	OutResult.GeneratedCollectionPath = CollectionPath;

	// Note: Actual collection update would use FItemCollectionGenerator here
	LogPipeline(TEXT("  [INFO] Collection update prepared"));
	LogPipeline(TEXT("  [INFO] Use FItemCollectionGenerator to create/update the collection"));

	return true;
}

bool FPipelineProcessor::AddToLoadout(const FString& CollectionPath, const FString& LoadoutName, FPipelineProcessResult& OutResult)
{
	LogPipeline(FString::Printf(TEXT("  Adding to loadout: %s"), *LoadoutName));
	LogPipeline(FString::Printf(TEXT("    Collection: %s"), *CollectionPath));

	// Note: Actual loadout update would modify NPC definition's DefaultItemLoadout
	LogPipeline(TEXT("  [INFO] Loadout update prepared"));
	LogPipeline(TEXT("  [INFO] Update NPC definition's DefaultItemLoadout.ItemCollectionsToGrant"));

	return true;
}

void FPipelineProcessor::LogPipeline(const FString& Message)
{
	UE_LOG(LogGasAbilityPipeline, Log, TEXT("[Pipeline] %s"), *Message);
}
